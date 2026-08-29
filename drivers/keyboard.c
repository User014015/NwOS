#include "keyboard.h"

static int extended_key = 0;

static int shift_pressed = 0;


/*
 * Regular layout
 */
static const char keyboard_map[128] =
{
    0, 27,

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
    ' '
};


/*
 * Layout with squeezing shift
 */
static const char keyboard_shift_map[128] =
{
    0, 27,

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
    '<','>','/',


    0,
    '*',
    0,
    ' '
};


static unsigned char inb(unsigned short port)
{
    unsigned char value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

int keyboard_getkey(void)
{
    /*
     * Waiting for pressing\unpressing
     */
    while (!(inb(0x64) & 1))
    {
    }

    unsigned char scancode = inb(0x60);

    if (scancode == 0xE0)
    {
        extended_key = 1;
        return KEY_NONE;
    }

    if (extended_key)
    {
        extended_key = 0;

        if (scancode & 0x80)
            return KEY_NONE;

        if (scancode == 0x48)
            return KEY_UP;

        if (scancode == 0x50)
            return KEY_DOWN;

        if (scancode == 0x4B)
            return KEY_LEFT;

        if (scancode == 0x4D)
            return KEY_RIGHT;
    }
    if (scancode == 0x2A)
    {
        shift_pressed = 1;
        return KEY_NONE;
    }

    if (scancode == 0xAA)
    {
        shift_pressed = 0;
        return KEY_NONE;
    }
    if (scancode == 0x36)
    {
        shift_pressed = 1;
        return KEY_NONE;
    }

    if (scancode == 0xB6)
    {
        shift_pressed = 0;
        return KEY_NONE;
    }
    if (scancode & 0x80)
        return KEY_NONE;


    /*
     * Special keys
     */
    if (scancode == 0x1C)
        return KEY_ENTER;

    if (scancode == 0x0E)
        return KEY_BACKSPACE;


    /*
     * symbol
     */
    if (scancode < 128)
    {
        if (shift_pressed)
            return keyboard_shift_map[scancode];

        return keyboard_map[scancode];
    }

    return KEY_NONE;
}