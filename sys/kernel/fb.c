#include "fb.h"
#include "wallpaper_data.h"

extern uint32_t multiboot_info_ptr;

static uint32_t* fb_addr = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static int fb_bpp = 0;

static const uint8_t font8x8_basic[128][8] = {
    [' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!']={0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    ['"']={0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    ['#']={0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    ['$']={0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    ['%']={0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    ['&']={0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    ['\'']={0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    ['(']={0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    [')']={0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    ['*']={0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['+']={0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    [',']={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    ['-']={0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    ['.']={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    ['/']={0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    ['0']={0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    ['1']={0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    ['2']={0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    ['3']={0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    ['4']={0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    ['5']={0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    ['6']={0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    ['7']={0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    ['8']={0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    ['9']={0x1E,0x33,0x33,0x3F,0x30,0x18,0x0E,0x00},
    [':']={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    [';']={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    ['<']={0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    ['=']={0x00,0x00,0x3F,0x00,0x3F,0x00,0x00,0x00},
    ['>']={0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    ['?']={0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    ['@']={0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    ['A']={0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    ['B']={0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    ['C']={0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    ['D']={0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    ['E']={0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    ['F']={0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    ['G']={0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    ['H']={0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    ['I']={0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['J']={0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    ['K']={0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    ['L']={0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    ['M']={0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    ['N']={0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    ['O']={0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    ['P']={0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    ['Q']={0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    ['R']={0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    ['S']={0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    ['T']={0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['U']={0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    ['V']={0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['W']={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X']={0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
    ['Y']={0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    ['Z']={0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    ['[']={0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    ['\\']={0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    [']']={0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    ['^']={0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    ['_']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    ['`']={0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    ['a']={0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    ['b']={0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    ['c']={0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    ['d']={0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00},
    ['e']={0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00},
    ['f']={0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00},
    ['g']={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
    ['h']={0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    ['i']={0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    ['j']={0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    ['k']={0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    ['l']={0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['m']={0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    ['n']={0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    ['o']={0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
    ['p']={0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    ['q']={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    ['r']={0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    ['s']={0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
    ['t']={0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    ['u']={0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    ['v']={0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['w']={0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    ['x']={0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    ['y']={0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    ['z']={0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    ['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    ['|']={0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    ['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    ['~']={0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
};

static uint32_t vga_to_rgb(uint8_t color) {
    static const uint32_t pal[16] = {
        0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
        0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
    };
    return pal[color & 0x0F];
}

void fb_init(void) {
    if (!multiboot_info_ptr) return;
    uint8_t* ptr = (uint8_t*)(uintptr_t)multiboot_info_ptr;
    uint32_t total_size = *(uint32_t*)ptr;
    uint8_t* tag = ptr + 8;
    
    while (tag < ptr + total_size) {
        uint32_t type = *(uint32_t*)tag;
        uint32_t size = *(uint32_t*)(tag + 4);
        
        if (type == 0) break;
        
        if (type == 8) {
            uint64_t addr = *(uint64_t*)(tag + 8);
            fb_addr   = (uint32_t*)(uintptr_t)addr;
            fb_pitch  = *(uint32_t*)(tag + 16);
            fb_width  = *(uint32_t*)(tag + 20);
            fb_height = *(uint32_t*)(tag + 24);
            fb_bpp    = *(uint8_t*)(tag + 28);
            break;
        }
        tag += (size + 7) & ~7;
    }
}

static void fb_outw(unsigned short port, unsigned short value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static unsigned short fb_inw(unsigned short port)
{
    unsigned short value;

    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static void fb_dispi_write(unsigned short index, unsigned short value)
{
    fb_outw(0x1CE, index);
    fb_outw(0x1CF, value);
}

static unsigned short fb_dispi_read(unsigned short index)
{
    fb_outw(0x1CE, index);
    return fb_inw(0x1CF);
}

int fb_set_mode(uint32_t width, uint32_t height)
{
    if (!fb_addr || width == 0 || height == 0)
        return 0;

    unsigned short id = fb_dispi_read(0x00);

    if (id < 0xB0C0)
        return 0;

    fb_dispi_write(0x04, 0x00);
    fb_dispi_write(0x01, (unsigned short)width);
    fb_dispi_write(0x02, (unsigned short)height);
    fb_dispi_write(0x03, 32);
    fb_dispi_write(0x06, (unsigned short)width);
    fb_dispi_write(0x07, (unsigned short)height);
    fb_dispi_write(0x04, 0x41);

    fb_width  = width;
    fb_height = height;
    fb_pitch  = width * 4;
    fb_bpp    = 32;

    fb_clear(0x000000);

    return 1;
}

int fb_is_available(void) {
    return fb_addr != 0 && fb_bpp == 32;
}

uint32_t fb_get_width(void) {
    return fb_width;
}

uint32_t fb_get_height(void) {
    return fb_height;
}

void fb_clear(uint32_t color) {
    if (!fb_addr) return;

    for (uint32_t y = 0; y < fb_height; y++) {
        uint32_t* row = (uint32_t*)((uint8_t*)fb_addr + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; x++)
            row[x] = color;
    }
}

void fb_draw_char(int x, int y, char c, uint8_t color) {
    if (!fb_addr) return;
    
    int px_start = x * 8;
    int py_start = y * 8;
    
    uint32_t fg_rgb = vga_to_rgb(color & 0x0F);
    uint32_t bg_rgb = vga_to_rgb((color >> 4) & 0x0F);
    
    const uint8_t* glyph = font8x8_basic[(uint8_t)c & 0x7F];
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int px = px_start + col;
            int py = py_start + row;
            if (px >= (int)fb_width || py >= (int)fb_height || px < 0 || py < 0) continue;
            
            uint32_t color_rgb = ((glyph[row] >> col) & 1) ? fg_rgb : bg_rgb;
            uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + py * fb_pitch + px * 4);
            *pixel = color_rgb;
        }
    }
}

void fb_draw_char_transparent(int x, int y, char c, uint8_t color) {
    if (!fb_addr) return;

    int px_start = x * 8;
    int py_start = y * 8;

    uint32_t fg_rgb = vga_to_rgb(color & 0x0F);

    const uint8_t* glyph = font8x8_basic[(uint8_t)c & 0x7F];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (!((glyph[row] >> col) & 1)) continue;

            int px = px_start + col;
            int py = py_start + row;
            if (px >= (int)fb_width || py >= (int)fb_height || px < 0 || py < 0) continue;

            uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + py * fb_pitch + px * 4);
            *pixel = fg_rgb;
        }
    }
}

void fb_draw_image(int x, int y, const uint16_t* data, int width, int height) {
    if (!fb_addr) return;

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= (int)fb_width || py >= (int)fb_height || px < 0 || py < 0) continue;

            uint16_t rgb565 = data[j * width + i];
            
            uint8_t r = (rgb565 >> 11) & 0x1F;
            uint8_t g = (rgb565 >> 5)  & 0x3F;
            uint8_t b =  rgb565        & 0x1F;

            uint8_t r8 = (r * 255) / 31;
            uint8_t g8 = (g * 255) / 63;
            uint8_t b8 = (b * 255) / 31;

            uint32_t color = (r8 << 16) | (g8 << 8) | b8;

            uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + py * fb_pitch + px * 4);
            *pixel = color;
        }
    }
}

void fb_draw_wallpaper(void) {
#if WALLPAPER_PRESENT
    if (!fb_addr) return;

    if (WALLPAPER_WIDTH == (int)fb_width &&
        WALLPAPER_HEIGHT == (int)fb_height)
    {
        fb_draw_image(0, 0, wallpaper_pixels,
                      WALLPAPER_WIDTH, WALLPAPER_HEIGHT);
        return;
    }

    for (uint32_t y = 0; y < fb_height; y++)
    {
        uint32_t sy = (uint32_t)((uint64_t)y * WALLPAPER_HEIGHT / fb_height);

        for (uint32_t x = 0; x < fb_width; x++)
        {
            uint32_t sx = (uint32_t)((uint64_t)x * WALLPAPER_WIDTH / fb_width);
            uint16_t rgb565 = wallpaper_pixels[sy * WALLPAPER_WIDTH + sx];

            uint8_t r = (rgb565 >> 11) & 0x1F;
            uint8_t g = (rgb565 >> 5)  & 0x3F;
            uint8_t b =  rgb565        & 0x1F;

            uint8_t r8 = (r * 255) / 31;
            uint8_t g8 = (g * 255) / 63;
            uint8_t b8 = (b * 255) / 31;

            uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + y * fb_pitch + x * 4);
            *pixel = (r8 << 16) | (g8 << 8) | b8;
        }
    }
#endif
}

void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (!fb_addr) return;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int px = x + i;
            int py = y + j;
            if (px >= (int)fb_width || py >= (int)fb_height || px < 0 || py < 0) continue;
            uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + py * fb_pitch + px * 4);
            *pixel = color;
        }
    }
}

static void fb_put_pixel(int x, int y, uint32_t color) {
    if (!fb_addr) return;
    if (x < 0 || y < 0 || x >= (int)fb_width || y >= (int)fb_height) return;
    uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + y * fb_pitch + x * 4);
    *pixel = color;
}

static int fb_abs(int v) {
    return v < 0 ? -v : v;
}

void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = fb_abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -fb_abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void fb_draw_frame(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;

    int x2 = x + w - 1;
    int y2 = y + h - 1;

    fb_draw_line(x, y, x2, y, color);
    fb_draw_line(x, y2, x2, y2, color);
    fb_draw_line(x, y, x, y2, color);
    fb_draw_line(x2, y, x2, y2, color);
}

void fb_draw_cube(int x, int y, int size, uint32_t color) {
    if (size <= 0) return;

    int d = size / 4;

    int ax = x,           ay = y;
    int bx = x + size,    by = y;
    int cx = x + size,    cy = y + size;
    int dx = x,           dy = y + size;

    int ex = x + d,       ey = y - d;
    int fx = x + size + d, fy = y - d;
    int gx = x + size + d, gy = y + size - d;
    int hx = x + d,       hy = y + size - d;

    fb_draw_line(ax, ay, bx, by, color);
    fb_draw_line(bx, by, cx, cy, color);
    fb_draw_line(cx, cy, dx, dy, color);
    fb_draw_line(dx, dy, ax, ay, color);

    fb_draw_line(ex, ey, fx, fy, color);
    fb_draw_line(fx, fy, gx, gy, color);
    fb_draw_line(gx, gy, hx, hy, color);
    fb_draw_line(hx, hy, ex, ey, color);

    fb_draw_line(ax, ay, ex, ey, color);
    fb_draw_line(bx, by, fx, fy, color);
    fb_draw_line(cx, cy, gx, gy, color);
    fb_draw_line(dx, dy, hx, hy, color);
}

void fb_draw_cube_default(uint32_t color) {
    int size = (int)(fb_width / 4);
    if (fb_height / 4 < size) size = (int)(fb_height / 4);
    if (size < 32) size = 32;

    int x = (int)(fb_width / 2) - size / 2;
    int y = (int)(fb_height / 2) - size / 2 + size / 8;

    fb_draw_cube(x, y, size, color);
}
void fb_draw_text(int x, int y, const char* text, uint32_t color) {
    if (text == 0) return;

    int cx = x;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n') {
            cx = x;
            y += 8;
            continue;
        }

        unsigned char ch = (unsigned char)text[i] & 0x7F;
        const uint8_t* glyph = font8x8_basic[ch];

        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (!((glyph[row] >> col) & 1)) continue;

                int px = cx + col;
                int py = y + row;
                if (px < 0 || py < 0 || px >= (int)fb_width || py >= (int)fb_height)
                    continue;

                uint32_t* pixel = (uint32_t*)((uint8_t*)fb_addr + py * fb_pitch + px * 4);
                *pixel = color;
            }
        }

        cx += 8;
    }
}

void fb_draw_window(int x, int y, int w, int h, uint32_t color, const char* title) {
    if (w < 24 || h < 24) return;

    const uint32_t face = 0x00C0C0C0;
    const uint32_t light = 0x00FFFFFF;
    const uint32_t dark = 0x00404040;
    const uint32_t shadow = 0x00808080;

    fb_draw_rect(x, y, w, h, face);

    fb_draw_line(x, y, x + w - 1, y, light);
    fb_draw_line(x, y, x, y + h - 1, light);
    fb_draw_line(x, y + h - 1, x + w - 1, y + h - 1, dark);
    fb_draw_line(x + w - 1, y, x + w - 1, y + h - 1, dark);

    fb_draw_line(x + 2, y + 2, x + w - 3, y + 2, shadow);
    fb_draw_line(x + 2, y + 2, x + 2, y + h - 3, shadow);

    int title_y = y + 3;
    int title_h = 20;
    int title_x = x + 3;
    int title_w = w - 6;

    if (title_w > 0) {
        fb_draw_rect(title_x, title_y, title_w, title_h, color);

        fb_draw_line(title_x, title_y, title_x + title_w - 1, title_y, light);
        fb_draw_line(title_x, title_y, title_x, title_y + title_h - 1, light);
        fb_draw_line(title_x, title_y + title_h - 1,
                     title_x + title_w - 1, title_y + title_h - 1, dark);

        if (title != 0 && title[0] != '\0')
            fb_draw_text(title_x + 5, title_y + 6, title, 0x00FFFFFF);

        int bx = x + w - 20;
        int by = y + 7;

        fb_draw_rect(bx, by, 14, 14, face);
        fb_draw_line(bx, by, bx + 13, by, light);
        fb_draw_line(bx, by, bx, by + 13, light);
        fb_draw_line(bx, by + 13, bx + 13, by + 13, dark);
        fb_draw_line(bx + 13, by, bx + 13, by + 13, dark);

        fb_draw_line(bx + 4, by + 4, bx + 9, by + 9, 0x00000000);
        fb_draw_line(bx + 9, by + 4, bx + 4, by + 9, 0x00000000);
    }
}
