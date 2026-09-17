#include "keyboard.h"

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64

#define KEYBOARD_STATUS_OUTPUT_FULL  0x01

#define KEYBOARD_RELEASE             0x80


static int shift_left  = 0;
static int shift_right = 0;
static int caps_lock   = 0;
static unsigned char kb_inb(
    unsigned short port)
{
    unsigned char value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}
static char keymap[128] =
{
    0,

    27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=',
    '\b',

    '\t',

    'q','w','e','r','t','y','u','i','o','p',
    '[',']',
    '\n',

    0,

    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',

    0,

    '\\',

    'z','x','c','v','b','n','m',
    ',','.','/',

    0,

    '*',

    0,

    ' ',

    0,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0
};
static char shifted_keymap[128] =
{
    0,

    27,

    '!','@','#','$','%','^','&','*','(',')',
    '_','+',

    '\b',

    '\t',

    'Q','W','E','R','T','Y','U','I','O','P',

    '{','}',

    '\n',

    0,

    'A','S','D','F','G','H','J','K','L',

    ':','"','~',

    0,

    '|',

    'Z','X','C','V','B','N','M',

    '<','>','?',

    0,

    '*',

    0,

    ' ',

    0,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0
};

void keyboard_init(void)
{
    shift_left  = 0;
    shift_right = 0;
    caps_lock   = 0;
}

static unsigned char keyboard_read_scancode(void)
{
    while (!(kb_inb(KEYBOARD_STATUS_PORT) &
             KEYBOARD_STATUS_OUTPUT_FULL))
    {
        /* wait */
    }

    return kb_inb(KEYBOARD_DATA_PORT);
}

int keyboard_getkey(void)
{
    unsigned char scancode;
    char c;

    while (1)
    {
        scancode = keyboard_read_scancode();

        if (scancode == 0xE0)
        {
            unsigned char ext =
                keyboard_read_scancode();

            (void)ext;

            continue;
        }
        if (scancode & KEYBOARD_RELEASE)
        {
            unsigned char make =
                scancode & 0x7F;

            if (make == 0x2A)
                shift_left = 0;

            else if (make == 0x36)
                shift_right = 0;

            continue;
        }
        if (scancode == 0x2A)
        {
            shift_left = 1;
            continue;
        }

        if (scancode == 0x36)
        {
            shift_right = 1;
            continue;
        }
        if (scancode == 0x3A)
        {
            caps_lock = !caps_lock;
            continue;
        }
        if (scancode == 0x0E)
            return KEY_BACKSPACE;

        if (scancode == 0x0F)
            return KEY_TAB;

        if (scancode == 0x1C)
            return KEY_ENTER;

        if (scancode >= 128)
            continue;

        if (shift_left || shift_right)
            c = shifted_keymap[scancode];
        else
            c = keymap[scancode];

        if (c == 0)
            continue;

        if (c >= 'a' && c <= 'z')
        {
            if (caps_lock &&
                !(shift_left || shift_right))
            {
                c = (char)(c - 'a' + 'A');
            }
        }

        if (c >= 'A' && c <= 'Z')
        {
            if (caps_lock &&
                (shift_left || shift_right))
            {
                c = (char)(c - 'A' + 'a');
            }
        }

        return (int)c;
    }
}