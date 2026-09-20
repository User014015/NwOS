#include "keyboard.h"

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_TIMEOUT      100000

static int shift_left = 0;
static int shift_right = 0;
static int caps_lock = 0;

static unsigned char kb_inb(unsigned short port)
{
    unsigned char value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static int kb_has_data(void)
{
    return (kb_inb(KEYBOARD_STATUS_PORT) & 0x01) != 0;
}

static unsigned char kb_read_scancode(void)
{
    unsigned int timeout = KEYBOARD_TIMEOUT;

    while (!kb_has_data())
    {
        if (timeout == 0)
            return 0;

        timeout--;
    }

    return kb_inb(KEYBOARD_DATA_PORT);
}

static char keyboard_base_char(unsigned char code)
{
    static const char map[128] =
    {
        [0x01] = 27,

        [0x02] = '1',
        [0x03] = '2',
        [0x04] = '3',
        [0x05] = '4',
        [0x06] = '5',
        [0x07] = '6',
        [0x08] = '7',
        [0x09] = '8',
        [0x0A] = '9',
        [0x0B] = '0',
        [0x0C] = '-',
        [0x0D] = '=',
        [0x0E] = '\b',
        [0x0F] = '\t',

        [0x10] = 'q',
        [0x11] = 'w',
        [0x12] = 'e',
        [0x13] = 'r',
        [0x14] = 't',
        [0x15] = 'y',
        [0x16] = 'u',
        [0x17] = 'i',
        [0x18] = 'o',
        [0x19] = 'p',
        [0x1A] = '[',
        [0x1B] = ']',

        [0x1C] = '\n',

        [0x1E] = 'a',
        [0x1F] = 's',
        [0x20] = 'd',
        [0x21] = 'f',
        [0x22] = 'g',
        [0x23] = 'h',
        [0x24] = 'j',
        [0x25] = 'k',
        [0x26] = 'l',
        [0x27] = ';',
        [0x28] = '\'',
        [0x29] = '`',

        [0x2B] = '\\',

        [0x2C] = 'z',
        [0x2D] = 'x',
        [0x2E] = 'c',
        [0x2F] = 'v',
        [0x30] = 'b',
        [0x31] = 'n',
        [0x32] = 'm',
        [0x33] = ',',
        [0x34] = '.',
        [0x35] = '/',

        [0x39] = ' '
    };

    return map[code];
}

static char keyboard_shift_char(unsigned char code)
{
    static const char map[128] =
    {
        [0x02] = '!',
        [0x03] = '@',
        [0x04] = '#',
        [0x05] = '$',
        [0x06] = '%',
        [0x07] = '^',
        [0x08] = '&',
        [0x09] = '*',
        [0x0A] = '(',
        [0x0B] = ')',
        [0x0C] = '_',
        [0x0D] = '+',

        [0x1A] = '{',
        [0x1B] = '}',

        [0x27] = ':',
        [0x28] = '"',
        [0x29] = '~',

        [0x2B] = '|',

        [0x33] = '<',
        [0x34] = '>',
        [0x35] = '?'
    };

    return map[code];
}

static char uppercase_letter(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - 'a' + 'A');

    return c;
}

static int keyboard_special(unsigned char code)
{
    switch (code)
    {
        case 0x3B: return KEY_F1;
        case 0x3C: return KEY_F2;
        case 0x3D: return KEY_F3;
        case 0x3E: return KEY_F4;
        case 0x3F: return KEY_F5;
        case 0x40: return KEY_F6;
        case 0x41: return KEY_F7;
        case 0x42: return KEY_F8;
        case 0x43: return KEY_F9;
        case 0x44: return KEY_F10;
        case 0x57: return KEY_F11;
        case 0x58: return KEY_F12;

        default:
            return 0;
    }
}

int keyboard_getkey(void)
{
    static int extended = 0;

    unsigned char code;
    unsigned char make_code;
    int special;
    char c;
    int shifted;

    code = kb_read_scancode();

    if (code == 0)
        return 0;

    if (code == 0xE0)
    {
        extended = 1;
        return 0;
    }

    if (extended)
    {
        extended = 0;

        if (code & 0x80)
            return 0;

        switch (code)
        {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;

            case 0x1C:
                return KEY_ENTER;

            case 0x35:
                return '/';

            default:
                return 0;
        }
    }

    make_code = (unsigned char)(code & 0x7F);

    if (code & 0x80)
    {
        if (make_code == 0x2A)
            shift_left = 0;

        if (make_code == 0x36)
            shift_right = 0;

        return 0;
    }

    if (make_code == 0x2A)
    {
        shift_left = 1;
        return 0;
    }

    if (make_code == 0x36)
    {
        shift_right = 1;
        return 0;
    }

    if (make_code == 0x3A)
    {
        caps_lock = !caps_lock;
        return 0;
    }

    if (make_code == 0x0E)
        return KEY_BACKSPACE;

    if (make_code == 0x1C)
        return KEY_ENTER;

    if (make_code == 0x0F)
        return KEY_TAB;

    if (make_code == 0x01)
        return KEY_ESC;

    special = keyboard_special(make_code);

    if (special != 0)
        return special;

    c = keyboard_base_char(make_code);

    if (c == '\0')
        return 0;

    shifted = shift_left || shift_right;

    if (c >= 'a' && c <= 'z')
    {
        if (caps_lock ^ shifted)
            c = uppercase_letter(c);

        return (unsigned char)c;
    }

    if (shifted)
    {
        char shifted_char =
            keyboard_shift_char(make_code);

        if (shifted_char != '\0')
            c = shifted_char;
    }

    if (c == '\n')
        return KEY_ENTER;

    if (c == '\b')
        return KEY_BACKSPACE;

    if (c == '\t')
        return KEY_TAB;

    return (unsigned char)c;
}