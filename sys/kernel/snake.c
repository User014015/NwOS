#include "snake.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

static unsigned int last_tick_ms;
static unsigned int move_interval_ms;

#define SNAKE_MAX_LEN 500

typedef struct { int x, y; } spoint_t;

static spoint_t snake[SNAKE_MAX_LEN];
static int      snake_len;
static int      dir_x, dir_y;
static int      next_dir_x, next_dir_y;
static int      food_x, food_y;
static int      score;
static int      high_score = 0;
static int      game_over;

static unsigned short last_tick;
static unsigned int  move_interval_us;

static unsigned int rng_state = 1;

static unsigned int rnd(void) {
    rng_state = rng_state * 1103515245u + 12345u;
    return (rng_state >> 16) & 0x7FFF;
}

static void spawn_food(void) {
    for (int tries = 0; tries < 300; tries++) {
        int fx = rnd() % SNAKE_GRID_W;
        int fy = rnd() % SNAKE_GRID_H;
        int busy = 0;
        for (int i = 0; i < snake_len; i++)
            if (snake[i].x == fx && snake[i].y == fy) { busy = 1; break; }
        if (!busy) { food_x = fx; food_y = fy; return; }
    }
    food_x = 0; food_y = 0;
}

void snake_init(void) {
    rng_state = ((unsigned int)timer_read() << 8) | 0x5A5A;
    if (rng_state == 0) rng_state = 1;

    snake_len = 4;
    int cx = SNAKE_GRID_W / 2;
    int cy = SNAKE_GRID_H / 2;
    for (int i = 0; i < snake_len; i++) {
        snake[i].x = cx - i;
        snake[i].y = cy;
    }
    dir_x = 1; dir_y = 0;
    next_dir_x = 1; next_dir_y = 0;
    score = 0;
    game_over = 0;
    move_interval_ms = 180;
    last_tick_ms = timer_ms();

    spawn_food();
    last_tick = timer_read();
}

int snake_is_over(void)        { return game_over; }
int snake_get_score(void)      { return score; }
int snake_get_high_score(void) { return high_score; }

void snake_handle_key(unsigned char key) {
    if (game_over) {
        if (key == ' ') snake_init();
        return;
    }

    int nx = next_dir_x, ny = next_dir_y;
    switch (key) {
        case KEY_UP:    if (dir_y != 1)  { nx = 0;  ny = -1; } break;
        case KEY_DOWN:  if (dir_y != -1) { nx = 0;  ny = 1;  } break;
        case KEY_LEFT:  if (dir_x != 1)  { nx = -1; ny = 0;  } break;
        case KEY_RIGHT: if (dir_x != -1) { nx = 1;  ny = 0;  } break;
        default: return;
    }
    next_dir_x = nx;
    next_dir_y = ny;
}

int snake_update(void) {
    if (game_over) return 0;

    unsigned int now_ms = timer_ms();
    if (now_ms - last_tick_ms < move_interval_ms) return 0;
    last_tick_ms = now_ms;

    unsigned short now = timer_read();
    unsigned int elapsed = timer_elapsed_us(last_tick, now);
    if (elapsed < move_interval_us) return 0;
    last_tick = now;

    dir_x = next_dir_x;
    dir_y = next_dir_y;

    int nx = snake[0].x + dir_x;
    int ny = snake[0].y + dir_y;
    if (nx < 0 || nx >= SNAKE_GRID_W || ny < 0 || ny >= SNAKE_GRID_H) {
        game_over = 1;
        if (score > high_score) high_score = score;
        return 1;
    }
    int will_grow = (nx == food_x && ny == food_y);
    int check_len = will_grow ? snake_len : snake_len - 1;
    for (int i = 0; i < check_len; i++) {
        if (snake[i].x == nx && snake[i].y == ny) {
            game_over = 1;
            if (score > high_score) high_score = score;
            return 1;
        }
    }
    if (will_grow) {
        score += 10;
        unsigned int mi = 180u - (unsigned int)(score / 10);
        if (mi < 80u) mi = 80u;
        move_interval_ms = mi;
        spawn_food();
    } else {
        for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i-1];
    }
    snake[0].x = nx;
    snake[0].y = ny;
    return 1;
}
static void draw_int(int x, int y, int v, unsigned char fg, unsigned char bg) {
    char buf[16]; int k = 0;
    unsigned int u;
    if (v < 0) { buf[k++] = '-'; u = (unsigned int)(-v); }
    else       { u = (unsigned int)v; }
    char tmp[12]; int t = 0;
    if (u == 0) tmp[t++] = '0';
    while (u) { tmp[t++] = '0' + (u % 10); u /= 10; }
    while (t) buf[k++] = tmp[--t];
    buf[k] = 0;
    gfx_puts(x, y, buf, fg, bg);
}
void snake_draw(void) {
    int top = SNAKE_TOP;
    gfx_rect(0, top, 640, SNAKE_GRID_H * SNAKE_CELL, THEME_BG);
    unsigned char grid_color = (THEME_BG == WHITE) ? LIGHT_GRAY : DARK_GRAY;
    for (int y = 0; y < SNAKE_GRID_H; y++)
        for (int x = 0; x < SNAKE_GRID_W; x++)
            gfx_putpixel(x * SNAKE_CELL, top + y * SNAKE_CELL, grid_color);
    int fx = food_x * SNAKE_CELL;
    int fy = top + food_y * SNAKE_CELL;
    gfx_rect(fx + 3, fy + 3, SNAKE_CELL - 6, SNAKE_CELL - 6, RED);
    for (int i = 0; i < snake_len; i++) {
        int sx = snake[i].x * SNAKE_CELL;
        int sy = top + snake[i].y * SNAKE_CELL;
        unsigned char c = (i == 0) ? LIGHT_GREEN : GREEN;
        gfx_rect(sx + 1, sy + 1, SNAKE_CELL - 2, SNAKE_CELL - 2, c);
    }
    gfx_rect(0, 32, 640, 32, THEME_BG);
    gfx_puts(8,   40, "SCORE:", THEME_FG, THEME_BG);
    draw_int(70,  40, score,      THEME_FG, THEME_BG);
    gfx_puts(180, 40, "HIGH:",  THEME_FG, THEME_BG);
    draw_int(238, 40, high_score, THEME_FG, THEME_BG);
    gfx_puts(360, 40, "ARROWS = move  |  SPACE = restart  |  ESC = back",
             THEME_FG, THEME_BG);
    if (game_over) {
        int bx = 180, by = 200, bw = 280, bh = 110;
        gfx_rect(bx, by, bw, bh, THEME_FG);
        gfx_rect(bx + 2, by + 2, bw - 4, bh - 4, THEME_BAR);
        gfx_puts(bx + 70, by + 15, "GAME OVER", YELLOW, THEME_BAR);
        gfx_puts(bx + 30, by + 40, "Score:", WHITE, THEME_BAR);
        draw_int(bx + 100, by + 40, score, WHITE, THEME_BAR);
        gfx_puts(bx + 30, by + 60, "High:", WHITE, THEME_BAR);
        draw_int(bx + 100, by + 60, high_score, WHITE, THEME_BAR);
        gfx_puts(bx + 30, by + 85, "SPACE = restart", LIGHT_CYAN, THEME_BAR);
    }
}