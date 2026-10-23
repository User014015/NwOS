#ifndef MOUSE_H
#define MOUSE_H

void mouse_init(void);
void mouse_set_bounds(int width, int height);
void mouse_handle_byte(unsigned char byte);
int mouse_poll(void);
int mouse_x(void);
int mouse_y(void);
int mouse_buttons(void);

#endif
