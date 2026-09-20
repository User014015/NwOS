#ifndef MOUSE_H
#define MOUSE_H

typedef struct {
    int x, y;
    int left, right;
    int left_pressed, right_pressed;
    int wheel;
} mouse_state_t;

void mouse_init(void);
void mouse_update(mouse_state_t *state);

#endif