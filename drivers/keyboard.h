#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NONE       0
#define KEY_ESC        255
#define KEY_ENTER      256
#define KEY_BACKSPACE  257
#define KEY_TAB        258

#define KEY_UP         259
#define KEY_DOWN       260
#define KEY_LEFT       261
#define KEY_RIGHT      262

#define KEY_SUPER_S    263

void keyboard_init(void);
int keyboard_getkey(void);

#endif