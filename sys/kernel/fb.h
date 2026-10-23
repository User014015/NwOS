#ifndef FB_H
#define FB_H

#include <stdint.h>

void fb_init(void);
int fb_set_mode(uint32_t width, uint32_t height);
void fb_draw_char(int x, int y, char c, uint8_t color);
void fb_draw_char_transparent(int x, int y, char c, uint8_t color);
int  fb_is_available(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);
void fb_clear(uint32_t color);
void fb_draw_image(int x, int y, const uint16_t* data, int width, int height);
void fb_draw_wallpaper(void);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void fb_draw_frame(int x, int y, int w, int h, uint32_t color);
void fb_draw_cube_default(uint32_t color);
void fb_draw_cube(int x, int y, int size, uint32_t color);
void fb_draw_window(int x, int y, int w, int h, uint32_t color, const char* title);
void fb_draw_text(int x, int y, const char* text, uint32_t color);

#endif