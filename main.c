static void int13h()
{
    asm volatile ("mov   $0x0013, %%ax\n"
                  "int   $0x10\n"
                  "mov   $0xA000, %%ax\n"
                  "mov   %%ax, %%es\n"
                  ::: "ax");
}

typedef unsigned char u8;
typedef unsigned short u16;
typedef short i16;
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
static NOINLINE void vga_pixel(i16 x, i16 y, u8 c)
{
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
static void draw_line(i16 x1, i16 y1, i16 x2, i16 y2, u8 c)
{
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

#define MAX_ITERATIONS 12

u16 mandelbrot(float _x, float _y){
    float x = 0, y = 0;
    u16 iteration = 0;
    while (x*x + y*y <= 2*2 && iteration < MAX_ITERATIONS)
    {
        float xtemp = x*x - y*y + _x;
        y = 2*x*y + _y;
        x = xtemp;
        iteration++;
    }
    return iteration;
}

void main(void) {
    // u8 s[] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
    // print(s, sizeof(s));
    volatile u16 out = mandelbrot(2.53, 6.32);
    asm("nop");

    // TODO: compare to c output in float

    int13h();
    // sdraw_line(20, 50, 100, 140, 4);
}

/* 1. could save space by making main a noreturn function
      and placing a hlt at the end so it doesn't have to
      generate function callconv instructions */