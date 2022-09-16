static void segment_to_buffer()
{
    asm volatile ("mov   $0x07E00, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
}

static void segment_to_vram()
{
    asm volatile ("mov   $0xA000, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
}

static void segment_memcpy()
{
    asm volatile ("mov   $0x07E00, %%ax\n"
                  "mov   %%ax, %%ds\n"
                  ::: "ax");

    // DS:SI into ES:DI
    // buffer into vram

    asm volatile ("mov $0, %%si\n"
                  "mov $0, %%di\n"
                  "mov $16000, %%cx\n"
                  "rep movsd\n"
                  ::: "ax", "cx", "si", "di");
    
    // move by 32 bit each

    asm volatile ("mov   $0x0, %%ax\n"
                  "mov   %%ax, %%ds\n"
                  ::: "ax");
}

static void int13h()
{
    asm volatile ("mov   $0x0013, %%ax\n"
                  "int   $0x10\n"
                  ::: "ax");
    segment_to_vram();
}

static void int16h()
{
    asm volatile ("mov   $0x00, %%ah\n"
                  "int   $0x16\n"
                  ::: "ah");
}

typedef unsigned char u8;
typedef unsigned short u16;
typedef short i16;
typedef long double f80; // x86 FTW!!

#define VGA_WIDTH 320
#define VGA_HEIGHT 200

#define MAX_ITERATIONS 255

static u16 calculate_mandel_naive(f80 _x, f80 _y) {
    f80 x = 0, y = 0;
    u16 iteration = 0;
    while (x*x + y*y <= 2*2 && iteration < MAX_ITERATIONS)
    {
        f80 xtemp = x*x - y*y + _x;
        y = 2*x*y + _y;
        x = xtemp;
        iteration++;
    }
    return iteration;
}

// https://wiki.osdev.org/Inline_Assembly/Examples#OUTx
static inline u8 inp(u16 port)
{
    u8 ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void outp(u16 port, u8 val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

#define VGA_INPUT_STATUS 0x03DA
#define VGA_BLANK 0x8

static void wait_for_vblank()
{
    while ( (inp(VGA_INPUT_STATUS) & VGA_BLANK));
    while (!(inp(VGA_INPUT_STATUS) & VGA_BLANK));
}

#define PALETTE_INDEX 0x03c8
#define PALETTE_DATA 0x03c9


static u8 rand(void){
    static u8 rnd = 15;
    rnd *= 1999;
    return rnd;
}

static void write_palette(){
    outp(PALETTE_INDEX, 0);
    /* for (u16 i = 0; i < 256; i++)
    {
        outp(PALETTE_DATA, rand());
        outp(PALETTE_DATA, rand());
        outp(PALETTE_DATA, rand());
    } */
    /* for (u16 i = 0; i < 256; i++)
    {
        outp(PALETTE_DATA, i / 4);
        outp(PALETTE_DATA, i / 4);
        outp(PALETTE_DATA, i / 4);
    } */
    for (u16 i = 0; i < 256; i++)
    {
        outp(PALETTE_DATA, i % 20 * (64 / 20));
        outp(PALETTE_DATA, i % 20 * (64 / 20));
        outp(PALETTE_DATA, i % 20 * (64 / 20));
    }
}


static u16 calculate_mandel_mul_optimised(f80 _x, f80 _y) {
    f80 x2 = 0, y2 = 0, x = 0, y = 0;
    u16 iterations = 0;

    while (x2 + y2 <= 4 && iterations < MAX_ITERATIONS)
    {
        y = 2 * x * y + _y;
        x = x2 - y2 + _x;
        x2 = x * x;
        y2 = y * y;
        
        iterations++;
    }
    return iterations;
}

static f80 scale = 4.00;

static const f80 x_shift = -1.78;
static const f80 y_shift = -0;

static void draw_mandel(void) {
    const f80 mandelX_offset = (scale + scale) / VGA_WIDTH;
    const f80 mandelY_offset = (scale + scale) / VGA_HEIGHT;

    f80 mandelX = -scale + x_shift;
    f80 mandelY = -scale + y_shift;

    i16 offset = 0;
    for (i16 y = 0; y < VGA_HEIGHT; y++){
        for (i16 x = 0; x < VGA_WIDTH; x++){
            u16 a = calculate_mandel_naive(mandelX, mandelY);
            asm volatile (
                "mov %0, %%es:(%%bx)\n"
                ::"r"(a), "b"(offset)
            );
            offset++;
            mandelX += mandelX_offset;
        }
        mandelX = -scale + x_shift;
        mandelY += mandelY_offset;
    }
}

static void display(const char *s){
    asm volatile ("mov   $0x02, %%ah\n"
                  "mov   $0x00, %%bh\n"
                  "mov   $0x00, %%dx\n"
                  "int   $0x10\n"
                  ::: "ah", "bh", "dx");
    while(*s){
        asm volatile ("int  $0x10" :: "a"(0x0E00 | *s), "b"(255));
        s++;
    }
}

static u8 getch()
{
    u8 ret;
    asm volatile ("mov   $0x00, %%ah\n"
                  "int   $0x16\n"
                  "mov   %%al, %0"
                  : "=r"(ret)
                  :: "ax");
    return ret;
}

#define USE_DOUBLE_BUFFERING

void main(void) {
    int13h();      // enter 256 colour VGA mode
    write_palette();
#ifdef USE_DOUBLE_BUFFERING
    for (;;)
    {
        segment_to_buffer();
        draw_mandel();
        wait_for_vblank();
        segment_to_vram();
        segment_memcpy();
        scale *= 0.4;
        int16h();  // block waiting for a keypress
        display("loading...");
    }
#else
    for (;;)
    {
        draw_mandel();
        x_scale *= 0.4;
        y_scale *= 0.4;
        if (getch() == 'i') {
            invpal = !invpal;
        };
    }
#endif
    for(;;);
}