#include "mouse.h"
#include "io.h"

#define MOUSE_DATA   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_CMD    0x64

static unsigned char cycle = 0;
static signed char   packet[3];
static int mouse_x = 320, mouse_y = 240;
static int cur_left = 0, cur_right = 0;
static int prev_left = 0, prev_right = 0;

static void mouse_wait_write(void) {
    int t = 100000;
    while (t-- && (inb(MOUSE_STATUS) & 2)) __asm__ volatile("pause");
}
static void mouse_wait_read(void) {
    int t = 100000;
    while (t-- && !(inb(MOUSE_STATUS) & 1)) __asm__ volatile("pause");
}

static void mouse_write(unsigned char d) {
    mouse_wait_write(); outb(MOUSE_CMD, 0xD4);
    mouse_wait_write(); outb(MOUSE_DATA, d);
}
static unsigned char mouse_read(void) {
    mouse_wait_read();
    return inb(MOUSE_DATA);
}

void mouse_init(void) {
    for (int i = 0; i < 100 && (inb(MOUSE_STATUS) & 1); i++) (void)inb(MOUSE_DATA);

    mouse_wait_write(); outb(MOUSE_CMD, 0xA8);
    mouse_wait_write(); outb(MOUSE_CMD, 0x20);
    mouse_wait_read();
    unsigned char status = inb(MOUSE_DATA);
    status &= ~0x03;
    status &= ~0x20;
    mouse_wait_write(); outb(MOUSE_CMD, 0x60);
    mouse_wait_write(); outb(MOUSE_DATA, status);
    mouse_write(0xF6); (void)mouse_read();
    mouse_write(0xF4); (void)mouse_read();
    for (int i = 0; i < 20 && (inb(MOUSE_STATUS) & 1); i++) (void)inb(MOUSE_DATA);

    cycle = 0;
    cur_left = cur_right = prev_left = prev_right = 0;
}

void mouse_update(mouse_state_t *state) {
    while ((inb(MOUSE_STATUS) & 0x21) == 0x21) {
        unsigned char data = (unsigned char)inb(MOUSE_DATA);

        if (cycle == 0 && !(data & 0x08)) continue;

        packet[cycle++] = (signed char)data;

        if (cycle == 3) {
            cycle = 0;
            int dx = packet[1];
            int dy = packet[2];
            mouse_x += dx;
            mouse_y -= dy;

            if (mouse_x < 0)   mouse_x = 0;
            if (mouse_x > 639) mouse_x = 639;
            if (mouse_y < 0)   mouse_y = 0;
            if (mouse_y > 479) mouse_y = 479;

            cur_left  = packet[0] & 1;
            cur_right = packet[0] & 2;
        }
    }

    state->x = mouse_x;
    state->y = mouse_y;
    state->left  = cur_left;
    state->right = cur_right;
    state->left_pressed  = cur_left  && !prev_left;
    state->right_pressed = cur_right && !prev_right;
    prev_left  = cur_left;
    prev_right = cur_right;
}