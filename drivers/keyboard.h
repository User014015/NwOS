#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_NONE       0
#define KEY_ENTER      256
#define KEY_BACKSPACE  257

#define KEY_UP         258
#define KEY_DOWN       259
#define KEY_LEFT       260
#define KEY_RIGHT      261

int keyboard_getkey(void);

#endif