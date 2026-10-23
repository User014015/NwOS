#include "mouse.h"

unsigned int fb_get_width(void);
unsigned int fb_get_height(void);

#define PS2_DATA_PORT          0x60
#define PS2_STATUS_PORT        0x64
#define PS2_COMMAND_PORT       0x64

#define PS2_STATUS_OUTPUT      0x01
#define PS2_STATUS_INPUT       0x02
#define PS2_STATUS_AUX         0x20

#define PS2_CMD_ENABLE_AUX     0xA8
#define PS2_CMD_READ_CONFIG    0x20
#define PS2_CMD_WRITE_CONFIG   0x60
#define PS2_CMD_WRITE_AUX      0xD4

#define PS2_MOUSE_DEFAULTS     0xF6
#define PS2_MOUSE_ENABLE       0xF4

static int mouse_pos_x = 320;
static int mouse_pos_y = 200;
static int mouse_max_x = 639;
static int mouse_max_y = 399;
static int mouse_button_state = 0;
static int mouse_cycle = 0;
static unsigned char mouse_packet[3];

static unsigned char ps2_inb(unsigned short port)
{
    unsigned char value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static void ps2_outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int ps2_wait_input(void)
{
    for (unsigned int i = 0; i < 100000; i++)
    {
        if (!(ps2_inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT))
            return 1;
    }

    return 0;
}

static int ps2_wait_output(void)
{
    for (unsigned int i = 0; i < 100000; i++)
    {
        if (ps2_inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT)
            return 1;
    }

    return 0;
}

static void mouse_write(unsigned char value)
{
    ps2_wait_input();
    ps2_outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_AUX);
    ps2_wait_input();
    ps2_outb(PS2_DATA_PORT, value);
}

static int mouse_read_ack(void)
{
    if (!ps2_wait_output())
        return 0;

    return ps2_inb(PS2_DATA_PORT) == 0xFA;
}

void mouse_set_bounds(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    mouse_max_x = width - 1;
    mouse_max_y = height - 1;
    mouse_pos_x = width / 2;
    mouse_pos_y = height / 2;
    mouse_cycle = 0;
}

void mouse_init(void)
{
    unsigned char config;

    unsigned int width = fb_get_width();
    unsigned int height = fb_get_height();

    if (width == 0 || height == 0) {
        width = 640;
        height = 400;
    }

    mouse_cycle = 0;
    mouse_button_state = 0;
    mouse_set_bounds((int)width, (int)height);

    ps2_wait_input();
    ps2_outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_AUX);

    ps2_wait_input();
    ps2_outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);

    if (!ps2_wait_output())
        return;

    config = ps2_inb(PS2_DATA_PORT);
    config &= (unsigned char)~0x02;
    config &= (unsigned char)~PS2_STATUS_AUX;

    ps2_wait_input();
    ps2_outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    ps2_wait_input();
    ps2_outb(PS2_DATA_PORT, config);

    mouse_write(PS2_MOUSE_DEFAULTS);
    mouse_read_ack();

    mouse_write(PS2_MOUSE_ENABLE);
    mouse_read_ack();

    while (ps2_inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT)
        (void)ps2_inb(PS2_DATA_PORT);

    mouse_cycle = 0;
}

void mouse_handle_byte(unsigned char byte)
{
    if (mouse_cycle == 0 && !(byte & 0x08))
        return;

    mouse_packet[mouse_cycle++] = byte;

    if (mouse_cycle < 3)
        return;

    mouse_cycle = 0;

    if (!(mouse_packet[0] & 0x08))
        return;

    int dx = mouse_packet[1];
    int dy = mouse_packet[2];

    if (mouse_packet[0] & 0x10)
        dx -= 256;

    if (mouse_packet[0] & 0x20)
        dy -= 256;

    mouse_pos_x += dx;
    mouse_pos_y -= dy;

    if (mouse_pos_x < 0)
        mouse_pos_x = 0;
    if (mouse_pos_y < 0)
        mouse_pos_y = 0;
    if (mouse_pos_x > mouse_max_x)
        mouse_pos_x = mouse_max_x;
    if (mouse_pos_y > mouse_max_y)
        mouse_pos_y = mouse_max_y;

    mouse_button_state = mouse_packet[0] & 0x07;
}

int mouse_poll(void)
{
    int updated = 0;

    while (ps2_inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT)
    {
        if (!(ps2_inb(PS2_STATUS_PORT) & PS2_STATUS_AUX))
        {
            (void)ps2_inb(PS2_DATA_PORT);
            continue;
        }

        mouse_handle_byte(ps2_inb(PS2_DATA_PORT));
        updated = 1;
    }

    return updated;
}

int mouse_x(void)
{
    mouse_poll();
    return mouse_pos_x;
}

int mouse_y(void)
{
    mouse_poll();
    return mouse_pos_y;
}

int mouse_buttons(void)
{
    mouse_poll();
    return mouse_button_state;
}
