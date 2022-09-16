typedef unsigned char u8;
typedef unsigned short u16;
typedef short i16;
typedef long double f80;

#define VGA_WIDTH 320
#define VGA_HEIGHT 200

static inline i16 abs(i16 a)
{
    return a < 0 ? -a : a;
}

static void vga_pixel(i16 x, i16 y, u8 c)
{
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        u16 pos = y * 320 + x;
        asm volatile (
            "mov %%al, %%es:(%%bx)"
            :: "b"(pos), "a"(c)
        );
    }
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