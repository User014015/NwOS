#ifndef NWOS_KEYBOARD_H
#define NWOS_KEYBOARD_H

#define KEY_ENTER      0x1001
#define KEY_BACKSPACE  0x1002
#define KEY_TAB        0x1003
#define KEY_ESC        0x1004

#define KEY_UP         0x1101
#define KEY_DOWN       0x1102
#define KEY_LEFT       0x1103
#define KEY_RIGHT      0x1104

#define KEY_F1         0x1201
#define KEY_F2         0x1202
#define KEY_F3         0x1203
#define KEY_F4         0x1204
#define KEY_F5         0x1205
#define KEY_F6         0x1206
#define KEY_F7         0x1207
#define KEY_F8         0x1208
#define KEY_F9         0x1209
#define KEY_F10        0x120A
#define KEY_F11        0x120B
#define KEY_F12        0x120C

int keyboard_getkey(void);

#endif