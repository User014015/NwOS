#ifndef SNAKE_H
#define SNAKE_H

#define SNAKE_GRID_W  40
#define SNAKE_GRID_H  24
#define SNAKE_CELL    16
#define SNAKE_TOP     64

void snake_init(void);
int  snake_update(void);
void snake_handle_key(unsigned char key);
void snake_draw(void);
int  snake_is_over(void);
int  snake_get_score(void);
int  snake_get_high_score(void);

#endif