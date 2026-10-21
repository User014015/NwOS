#include "../../../../drivers/keyboard.h"
#include "snake.h"
#include "../../kernel.h"
#include <stdint.h>

extern void print(const char* text);
extern void putchar_os(char c);
extern void print_int(int number);
extern void print_success(const char* text);
extern void print_error(const char* text);
extern void wait_key(void);
extern void clear(void);
extern int keyboard_getkey(void);
extern void delay(unsigned int count);
extern int random_range(int min, int max);

#define SNAKE_WIDTH 40
#define SNAKE_HEIGHT 20
#define MAX_SNAKE 400

typedef struct {
    int x;
    int y;
} Point;

void game_snake(void)
{
    Point snake[MAX_SNAKE];
    int len = 3;
    int dir_x = 1;
    int dir_y = 0;
    int food_x, food_y;
    int score = 0;
    int game_over = 0;
    
    snake[0] = (Point){20, 10};
    snake[1] = (Point){19, 10};
    snake[2] = (Point){18, 10};
    
    food_x = random_range(5, SNAKE_WIDTH - 5);
    food_y = random_range(5, SNAKE_HEIGHT - 5);
    
    clear();
    print("\n=== SNAKE ===\n");
    print("Use WASD or Arrow keys to move. q to quit.\n\n");
    
    while (!game_over) {
        /* draw */
        clear();
        print("=== SNAKE === Score: ");
        print_int(score);
        print("\n");
        
        /* draw border */
        for (int x = 0; x < SNAKE_WIDTH; x++) {
            putchar_os('#');
        }
        putchar_os('\n');
        
        for (int y = 0; y < SNAKE_HEIGHT; y++) {
            putchar_os('#');
            for (int x = 0; x < SNAKE_WIDTH; x++) {
                int is_snake = 0;
                int is_head = 0;
                for (int i = 0; i < len; i++) {
                    if (snake[i].x == x && snake[i].y == y) {
                        is_snake = 1;
                        if (i == 0) is_head = 1;
                        break;
                    }
                }
                if (is_snake) {
                    if (is_head) putchar_os('O');
                    else putchar_os('o');
                } else if (x == food_x && y == food_y) {
                    putchar_os('*');
                } else {
                    putchar_os(' ');
                }
            }
            putchar_os('#');
            putchar_os('\n');
        }
        
        for (int x = 0; x < SNAKE_WIDTH; x++) {
            putchar_os('#');
        }
        putchar_os('\n');
        
        /* input */
        int key = keyboard_getkey();
        if (key == 'q' || key == 'Q') {
            game_over = 1;
            break;
        }
        if (key == 'w' || key == 'W' || key == KEY_UP) {
            if (dir_y != 1) { dir_x = 0; dir_y = -1; }
        } else if (key == 's' || key == 'S' || key == KEY_DOWN) {
            if (dir_y != -1) { dir_x = 0; dir_y = 1; }
        } else if (key == 'a' || key == 'A' || key == KEY_LEFT) {
            if (dir_x != 1) { dir_x = -1; dir_y = 0; }
        } else if (key == 'd' || key == 'D' || key == KEY_RIGHT) {
            if (dir_x != -1) { dir_x = 1; dir_y = 0; }
        }
        
        /* move */
        for (int i = len - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0].x += dir_x;
        snake[0].y += dir_y;
        
        /* collision */
        if (snake[0].x < 0 || snake[0].x >= SNAKE_WIDTH || snake[0].y < 0 || snake[0].y >= SNAKE_HEIGHT) {
            game_over = 1;
            break;
        }
        for (int i = 1; i < len; i++) {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                game_over = 1;
                break;
            }
        }
        
        /* food */
        if (snake[0].x == food_x && snake[0].y == food_y) {
            if (len < MAX_SNAKE - 1) len++;
            score += 10;
            food_x = random_range(1, SNAKE_WIDTH - 2);
            food_y = random_range(1, SNAKE_HEIGHT - 2);
        }
        
        delay(200000);
    }
    
    print_error("\nGame Over! Final Score: ");
    print_int(score);
    print("\n");
    wait_key();
}
