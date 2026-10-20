#ifndef FB_H
#define FB_H

#include <stdint.h>

void fb_init(void);
void fb_draw_char(int x, int y, char c, uint8_t color);
int  fb_is_available(void);
void fb_draw_image(int x, int y, const uint16_t* data, int width, int height);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);

#endif