static void int13h()
{
    asm volatile ("mov   $0x0013, %%ax\n"
                  "int   $0x10\n"
                  "mov   $0xA000, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
}

static void int16h()
{
    asm volatile ("mov   $0x0000, %%ax\n"
                  "int   $0x16\n"
                  "mov   $0xA000, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
}

/* TODO: Proper VSYNC with double buffering.
         The only issue is the constant segment swiching to
         memcpy two data locations */

/* static void switch_segment()
{
    asm volatile ("mov   $0xA000, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
} */

typedef unsigned char u8;
typedef unsigned short u16;
typedef short i16;
typedef long double f80; // x86 FTW!!

#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define NOINLINE __attribute__((noinline))

static inline i16 abs(i16 a)
{
    return a < 0 ? -a : a;
}

/* adding "-fno-inline-functions-called-once -fno-inline-small-functions" 
   fixes the program by not inlining functions such as these. it may be
   due to the inline assembly messing with regs they arent supposed to.
   (tagging this function as inline manually instantly breaks it)       
   
   instead of doing the above i am letting the compiler choose what to
   inline and not allowing it to inline this single function              */
static NOINLINE void vga_pixel(i16 x, i16 y, u8 c) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT)
        asm volatile ("imul  $320, %%bx\n"
                      "add   %%ax, %%bx\n"
                      "mov   %%cl, %%es:(%%bx)\n"
                      :: "a"(x), "b"(y), "c"(c)
                      : "dx");
}

/* not tagging this function as static will write it out in its entirety.
   this is far from ideal as calling this function will not allow it to
   be inlined and its arguments precomputed. not inlining this function 
   will literally leave you with 100 to 200 bytes left. 
   some serious out of the box thinking is required to get out of this one. */
static void draw_line(i16 x1, i16 y1, i16 x2, i16 y2, u8 c) {
    i16 dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    i16 dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    i16 err = (dx > dy ? dx : -dy) / 2, e2;
    for(;;) {
        vga_pixel(x1, y1, c);
        if (x1 == x2 && y1 == y2)
            break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y1 += sy;
        }
    }
}

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

static inline u8 inp(u16 port) {
    u8 ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

static inline void outp(u16 port, u8 value)
{
   asm volatile ("outb %%al,%%dx": :"d" (port), "a" (value) : "al", "dx");
}

static u8 pd = 0; 

#define PALETTE_INDEX 0x03c8
#define PALETTE_DATA 0x03c9

static void write_palette(){
    outp(PALETTE_INDEX, 0);
    for (u16 i = 0; i < 256; i++)
    {
        outp(PALETTE_DATA, pd+(i/4));
        outp(PALETTE_DATA, pd+(i/4));
        outp(PALETTE_DATA, pd+(i/4));
    }
    pd++;
    if (pd == 64){
        pd = 0;
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

static f80 x_scale = 4.00;
static f80 y_scale = 4.00;

static const f80 x_shift = -0.555;
static const f80 y_shift = -0.5558;
/* making a constant static means the compiler will
   automatically inline it to locations where it is used.
   this is exactly like a `#define variable 0`            */

static void draw_mandel(void) {
    const f80 mandelX_offset = (x_scale + x_scale) / VGA_WIDTH;
    const f80 mandelY_offset = (y_scale + y_scale) / VGA_HEIGHT;

    f80 mandelX = -x_scale + x_shift;
    f80 mandelY = -y_scale + y_shift;

    i16 offset = 0;
    for (i16 y = 0; y < VGA_HEIGHT; y++){
        for (i16 x = 0; x < VGA_WIDTH; x++){
            u16 a = calculate_mandel_mul_optimised(mandelX, mandelY);
            asm (
                "mov %1, %%bx\n"
                "mov %0, %%es:(%%bx)\n"
                ::"r"(a), "r"(offset) // try inverting the number for prettier results in a closer zoom
                : "bx"
            );
            offset++;
            mandelX += mandelX_offset;
        }
        mandelX = -x_scale + y_shift;
        mandelY += mandelY_offset;
    }
}

void main(void) {
    int13h();      // enter 256 colour VGA mode

    draw_mandel(); // draw first iteration
    int16h();      // block waiting for a keypress
    for (i16 i = 0; i < 29; i++)
    {
        x_scale *= 0.4;
        y_scale *= 0.4;
        draw_mandel();
    }

    for(;;) asm("hlt");
}