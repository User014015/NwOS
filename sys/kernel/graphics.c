#include "graphics.h"
#include "io.h"
#include "font.h"

static volatile unsigned char *vga = (volatile unsigned char *)VGA_MEMORY;
static unsigned char backbuf[SCREEN_WIDTH * SCREEN_HEIGHT];

static void vga_set_map_mask(unsigned char m)         { outb(0x3C4, 0x02); outb(0x3C5, m); }
static void vga_set_bit_mask(unsigned char m)         { outb(0x3CE, 0x08); outb(0x3CF, m); }
static void __attribute__((unused)) vga_set_setreset(unsigned char v) { outb(0x3CE, 0x00); outb(0x3CF, v); }
static void vga_set_enable_setreset(unsigned char m)  { outb(0x3CE, 0x01); outb(0x3CF, m); }

unsigned char THEME_BG       = WHITE;
unsigned char THEME_FG       = BLACK;
unsigned char THEME_BAR      = BLUE;
unsigned char THEME_BAR_FG   = WHITE;
unsigned char THEME_BTN      = LIGHT_GRAY;
unsigned char THEME_BTN_FG   = BLACK;
unsigned char THEME_SEL      = LIGHT_CYAN;
unsigned char THEME_SEL_FG   = BLACK;

void gfx_init(void) {}

void gfx_clear(unsigned char color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
        backbuf[i] = color & 0x0F;
}

void gfx_putpixel(int x, int y, unsigned char color) {
    if ((unsigned)x >= SCREEN_WIDTH || (unsigned)y >= SCREEN_HEIGHT) return;
    backbuf[y * SCREEN_WIDTH + x] = color & 0x0F;
}

void gfx_rect(int x, int y, int w, int h, unsigned char color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH  - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        unsigned char *row = &backbuf[(y + j) * SCREEN_WIDTH + x];
        for (int i = 0; i < w; i++) row[i] = color & 0x0F;
    }
}

void gfx_vline_fast(int x, int y0, int y1, unsigned char color) {
    if ((unsigned)x >= SCREEN_WIDTH) return;
    if (y0 < 0) y0 = 0;
    if (y1 > SCREEN_HEIGHT) y1 = SCREEN_HEIGHT;
    if (y1 <= y0) return;
    unsigned char *p = &backbuf[y0 * SCREEN_WIDTH + x];
    int n = y1 - y0;
    while (n-- > 0) { *p = color; p += SCREEN_WIDTH; }
}

void gfx_hline(int x, int y, int w, unsigned char c) { gfx_rect(x, y, w, 1, c); }
void gfx_vline(int x, int y, int h, unsigned char c) { gfx_rect(x, y, 1, h, c); }

void gfx_putchar(int x, int y, char c, unsigned char fg, unsigned char bg) {
    const unsigned char *g = font8x8_basic[(unsigned char)c];
    int row, col;
    for (row = 0; row < 8; row = row + 1) {
        unsigned char line = g[row];
        for (col = 0; col < 8; col = col + 1) {
            unsigned char px = (line >> col) & 1;
            int screen_x = x + col;
            int screen_y = y + row;
            if (px == 1) {
                gfx_putpixel(screen_x, screen_y, fg);
            } else {
                gfx_putpixel(screen_x, screen_y, bg);
            }
        }
    }
}

void gfx_flip_rows(int y0, int y1) {
    if (y0 < 0) y0 = 0;
    if (y1 > SCREEN_HEIGHT) y1 = SCREEN_HEIGHT;
    if (y1 <= y0) return;

    for (int p = 0; p < 4; p++) {
        vga_set_map_mask(1 << p);
        vga_set_bit_mask(0xFF);
        vga_set_enable_setreset(0x00);
        for (int y = y0; y < y1; y++) {
            const unsigned char *src = &backbuf[y * SCREEN_WIDTH];
            unsigned char *dst = (unsigned char*)vga + y * BYTES_PER_ROW;
            for (int bx = 0; bx < BYTES_PER_ROW; bx++) {
                unsigned char byte = 0;
                int o = bx * 8;
                if ((src[o+0] >> p) & 1) byte |= 0x80;
                if ((src[o+1] >> p) & 1) byte |= 0x40;
                if ((src[o+2] >> p) & 1) byte |= 0x20;
                if ((src[o+3] >> p) & 1) byte |= 0x10;
                if ((src[o+4] >> p) & 1) byte |= 0x08;
                if ((src[o+5] >> p) & 1) byte |= 0x04;
                if ((src[o+6] >> p) & 1) byte |= 0x02;
                if ((src[o+7] >> p) & 1) byte |= 0x01;
                *dst++ = byte;
            }
        }
    }
    vga_set_map_mask(0x0F);
    vga_set_enable_setreset(0x00);
    vga_set_bit_mask(0xFF);
}

void gfx_puts(int x, int y, const char *s, unsigned char fg, unsigned char bg) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y += 10; }
        else            { gfx_putchar(cx, y, *s, fg, bg); cx += 8; }
        s++;
    }
}

void gfx_flip(void) {
    for (int p = 0; p < 4; p++) {
        vga_set_map_mask(1 << p);
        vga_set_bit_mask(0xFF);
        vga_set_enable_setreset(0x00);

        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            const unsigned char *src = &backbuf[y * SCREEN_WIDTH];
            unsigned char *dst = (unsigned char*)vga + y * BYTES_PER_ROW;
            for (int bx = 0; bx < BYTES_PER_ROW; bx++) {
                unsigned char byte = 0;
                for (int i = 0; i < 8; i++) {
                    if ((src[bx * 8 + i] >> p) & 1) byte |= 0x80 >> i;
                }
                *dst++ = byte;
            }
        }
    }
    vga_set_map_mask(0x0F);
    vga_set_enable_setreset(0x00);
    vga_set_bit_mask(0xFF);
}