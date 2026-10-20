#include <stdint.h>

/*
* Old GUI
* Has many errors while loading
* check .nw version
*/


typedef struct {
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_char)(int x, int y, char c, uint8_t color);
    int  (*getkey)(void);
    void (*sleep)(unsigned int ms);
} gui_api_t;

static gui_api_t* api;

void draw_string(int x, int y, const char* str, uint32_t color) {
    while (*str) {
        api->draw_char(x, y, *str, (color == 0x00FFFFFF) ? 15 : 0);
        x += 8;
        str++;
    }
}

void _start(gui_api_t* passed_api) {
    api = passed_api;

    api->draw_rect(0, 0, 640, 400, 0x000080FF);
    api->draw_rect(0, 380, 640, 20, 0x00202020);

    int win_x = 100, win_y = 80, win_w = 300, win_h = 200;
    api->draw_rect(win_x + 5, win_y + 5, win_w, win_h, 0x00000000);
    api->draw_rect(win_x, win_y, win_w, win_h, 0x00E0E0E0);
    api->draw_rect(win_x, win_y, win_w, 20, 0x000000AA);

    draw_string(win_x + 5, win_y + 5, "NwOS GUI Window", 0x00FFFFFF);
    draw_string(win_x + 20, win_y + 40, "Welcome to the GUI!", 0x00000000);
    draw_string(win_x + 20, win_y + 60, "This is a C program", 0x00000000);
    draw_string(win_x + 20, win_y + 80, "running in Ring 0.", 0x00000000);
    draw_string(win_x + 20, win_y + 120, "Press ESC to exit.", 0x00000000);

    while (1) {
        int key = api->getkey();
        if (key == 27) {
            break;
        }
    }
}