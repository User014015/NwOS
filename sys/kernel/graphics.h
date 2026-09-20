#ifndef GRAPHICS_H
#define GRAPHICS_H

#define VGA_MEMORY    0xA0000
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define BYTES_PER_ROW (SCREEN_WIDTH / 8)

extern unsigned char THEME_BG;
extern unsigned char THEME_FG;
extern unsigned char THEME_BAR;
extern unsigned char THEME_BAR_FG;
extern unsigned char THEME_BTN;
extern unsigned char THEME_BTN_FG;
extern unsigned char THEME_SEL;
extern unsigned char THEME_SEL_FG;

enum {
    BLACK=0, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHT_GRAY,
    DARK_GRAY, LIGHT_BLUE, LIGHT_GREEN, LIGHT_CYAN, LIGHT_RED,
    LIGHT_MAGENTA, YELLOW, WHITE
};

void gfx_init(void);
void gfx_clear(unsigned char color);
void gfx_putpixel(int x, int y, unsigned char color);
void gfx_putchar(int x, int y, char c, unsigned char fg, unsigned char bg);
void gfx_puts(int x, int y, const char *str, unsigned char fg, unsigned char bg);
void gfx_rect(int x, int y, int w, int h, unsigned char color);
void gfx_hline(int x, int y, int w, unsigned char color);
void gfx_vline(int x, int y, int h, unsigned char color);
void gfx_flip(void);
void gfx_vline_fast(int x, int y0, int y1, unsigned char color);
void gfx_flip_rows(int y0, int y1);

#endif