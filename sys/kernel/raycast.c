#include "raycast.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

#define MAP_W 23
#define MAP_H 23

#define PLAY_TOP 32
#define PLAY_BOTTOM 448
#define PLAY_H (PLAY_BOTTOM - PLAY_TOP)
#define PLAY_CY (PLAY_TOP + PLAY_H / 2)
#define LINE_K_CONST (PLAY_H / 2 * 256)

static unsigned char map[MAP_H][MAP_W];
static int px, py, pa;
static int exit_x, exit_y;
static int won, dead;
static unsigned int steps;
static unsigned int start_ms, final_ms;
static int player_hp;
static int ammo;
static int score;
static unsigned int last_damage_ms;
static int damage_flash;

typedef struct {
    int x, y;
    int state;
    int hp;
    int type;
    unsigned int last_attack;
    int anim;
} sprite_t;

#define MAX_SPRITES 16
static sprite_t sprites[MAX_SPRITES];
static int sprite_count;
static int zbuf[640];
static unsigned int rng = 1;
static unsigned int rnd(void) {
    rng = rng * 1103515245u + 12345u;
    return (rng >> 16) & 0x7FFF;
}
#define FONE 256
static int fmul(int a, int b) { return (a * b) >> 8; }

static const short sintab64[64] = {
       0,   98,  195,  290,  383,  471,  556,  634,
     707,  773,  831,  882,  924,  957,  981,  995,
    1000,  995,  981,  957,  924,  882,  831,  773,
     707,  634,  556,  471,  383,  290,  195,   98,
       0,  -98, -195, -290, -383, -471, -556, -634,
    -707, -773, -831, -882, -924, -957, -981, -995,
   -1000, -995, -981, -957, -924, -882, -831, -773,
    -707, -634, -556, -471, -383, -290, -195,  -98
};

static int fsin(int a) {
    int idx  = (a >> 2) & 63;
    int frac = a & 3;
    int v1 = sintab64[idx];
    int v2 = sintab64[(idx + 1) & 63];
    return (v1 * (4 - frac) + v2 * frac) * 64 / 1000;
}
static int fcos(int a) { return fsin((a + 64) & 255); }
static void gen_maze(void) {
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map[y][x] = 1;

    int sx[512], sy[512], sp = 0;
    int cx = 1, cy = 1;
    map[cy][cx] = 0;
    sx[sp] = cx; sy[sp] = cy; sp++;

    const int dx4[4] = { 2, -2, 0, 0 };
    const int dy4[4] = { 0, 0, 2, -2 };

    while (sp > 0) {
        cx = sx[sp-1]; cy = sy[sp-1];
        int dirs[4], n = 0;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx4[d];
            int ny = cy + dy4[d];
            if (nx > 0 && nx < MAP_W-1 && ny > 0 && ny < MAP_H-1 && map[ny][nx] == 1)
                dirs[n++] = d;
        }
        if (n == 0) { sp--; continue; }
        int d = dirs[rnd() % n];
        int nx = cx + dx4[d], ny = cy + dy4[d];
        map[ny][nx] = 0;
        map[cy + dy4[d]/2][cx + dx4[d]/2] = 0;
        sx[sp] = nx; sy[sp] = ny; sp++;
    }
    static int dist[MAP_H][MAP_W];
    static int qx[1024], qy[1024];
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            dist[y][x] = -1;

    int head = 0, tail = 0;
    qx[tail] = 1; qy[tail] = 1; tail++;
    dist[1][1] = 0;

    const int bdx[4] = { 1, -1, 0, 0 };
    const int bdy[4] = { 0, 0, 1, -1 };

    while (head < tail) {
        int cx2 = qx[head], cy2 = qy[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = cx2 + bdx[d];
            int ny = cy2 + bdy[d];
            if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) continue;
            if (map[ny][nx] != 0) continue;
            if (dist[ny][nx] >= 0) continue;
            dist[ny][nx] = dist[cy2][cx2] + 1;
            qx[tail] = nx; qy[tail] = ny; tail++;
        }
    }

    int best = -1;
    exit_x = 1; exit_y = 1;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            if (dist[y][x] > best) { best = dist[y][x]; exit_x = x; exit_y = y; }
}
static void add_sprite(int wx, int wy, int type) {
    if (sprite_count >= MAX_SPRITES) return;
    sprites[sprite_count].x = (wx << 8) + 128;
    sprites[sprite_count].y = (wy << 8) + 128;
    sprites[sprite_count].state = 1;
    sprites[sprite_count].hp = (type == 1) ? 50 : 1;
    sprites[sprite_count].type = type;
    sprites[sprite_count].last_attack = 0;
    sprites[sprite_count].anim = 0;
    sprite_count++;
}

static void spawn_all(void) {
    sprite_count = 0;

    int enemies = 5 + rnd() % 3;
    for (int i = 0; i < enemies; i++) {
        for (int t = 0; t < 200; t++) {
            int mx = 1 + rnd() % (MAP_W - 2);
            int my = 1 + rnd() % (MAP_H - 2);
            if (map[my][mx] != 0) continue;
            int dx = mx - 1, dy = my - 1;
            if (dx*dx + dy*dy < 64) continue;
            add_sprite(mx, my, 1);
            break;
        }
    }

    int health = 2 + rnd() % 2;
    for (int i = 0; i < health; i++) {
        for (int t = 0; t < 100; t++) {
            int mx = 1 + rnd() % (MAP_W - 2);
            int my = 1 + rnd() % (MAP_H - 2);
            if (map[my][mx] != 0) continue;
            add_sprite(mx, my, 2);
            break;
        }
    }

    int ammo_pk = 3 + rnd() % 3;
    for (int i = 0; i < ammo_pk; i++) {
        for (int t = 0; t < 100; t++) {
            int mx = 1 + rnd() % (MAP_W - 2);
            int my = 1 + rnd() % (MAP_H - 2);
            if (map[my][mx] != 0) continue;
            add_sprite(mx, my, 3);
            break;
        }
    }
}
void raycast_init(void) {
    rng = ((unsigned int)timer_read() << 8) | 0x5A5A;
    if (rng == 0) rng = 1;

    gen_maze();
    spawn_all();

    px = (1 << 8) + 128;
    py = (1 << 8) + 128;
    pa = 0;
    won = 0; dead = 0;
    steps = 0;
    start_ms = timer_ms();
    final_ms = 0;

    player_hp = 100;
    ammo = 30;
    score = 0;
    damage_flash = 0;
    last_damage_ms = 0;
}

static void try_move(int dx, int dy) {
    int nx = px + dx;
    int mx = nx >> 8, my = py >> 8;
    if (mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H && map[my][mx] == 0)
        px = nx;

    int ny = py + dy;
    mx = px >> 8; my = ny >> 8;
    if (mx >= 0 && mx < MAP_W && my >= 0 && my < MAP_H && map[my][mx] == 0)
        py = ny;

    if (!won && !dead && (px >> 8) == exit_x && (py >> 8) == exit_y) {
        won = 1;
        final_ms = timer_ms() - start_ms;
    }
}
static void shoot(void) {
    if (won || dead || ammo <= 0) return;
    ammo--;
    damage_flash = 0;

    int best_dist = 0x7FFFFFFF;
    int best_idx = -1;

    for (int i = 0; i < sprite_count; i++) {
        if (!sprites[i].state || sprites[i].type != 1) continue;

        int dx = sprites[i].x - px;
        int dy = sprites[i].y - py;
        int depth = fmul(dx, fcos(pa)) + fmul(dy, fsin(pa));
        if (depth < 64) continue;

        int lateral = fmul(dx, fcos((pa+64)&255)) + fmul(dy, fsin((pa+64)&255));
        int screen_cx = 320 + (lateral * 320) / depth;
        int sprite_h = LINE_K_CONST / depth;
        int half_w = sprite_h / 2;
        if (half_w < 8) half_w = 8;

        if (screen_cx + half_w > 320 && screen_cx - half_w < 320) {
            if (depth < best_dist) {
                best_dist = depth;
                best_idx = i;
            }
        }
    }

    if (best_idx >= 0) {
        sprites[best_idx].hp -= 25;
        if (sprites[best_idx].hp <= 0) {
            sprites[best_idx].state = 0;
            score += 100;
        }
    }
}

void raycast_handle_key(unsigned char key) {
    if (won || dead) {
        if (key == ' ') raycast_init();
        return;
    }

    int move = 38;
    int dx, dy;
    int moved = 0;

    switch (key) {
        case 'w': case 'W': case KEY_UP:
            dx = fmul(fcos(pa), move); dy = fmul(fsin(pa), move);
            try_move(dx, dy); moved = 1; break;
        case 's': case 'S': case KEY_DOWN:
            dx = -fmul(fcos(pa), move); dy = -fmul(fsin(pa), move);
            try_move(dx, dy); moved = 1; break;
        case 'a': case 'A':
            dx = fmul(fcos((pa+192)&255), move); dy = fmul(fsin((pa+192)&255), move);
            try_move(dx, dy); moved = 1; break;
        case 'd': case 'D':
            dx = fmul(fcos((pa+64)&255), move); dy = fmul(fsin((pa+64)&255), move);
            try_move(dx, dy); moved = 1; break;
        case KEY_LEFT:  pa = (pa + 252) & 255; moved = 1; break;
        case KEY_RIGHT: pa = (pa + 4)   & 255; moved = 1; break;
        case ' ':       shoot(); break;
    }
    if (moved) steps++;
}
int raycast_update(void) {
    if (won || dead) return 0;

    unsigned int now = timer_ms();
    int changed = 0;

    for (int i = 0; i < sprite_count; i++) {
        if (!sprites[i].state) continue;

        /* Подбор предметов */
        if (sprites[i].type == 2 || sprites[i].type == 3) {
            int dx = sprites[i].x - px;
            int dy = sprites[i].y - py;
            int d2 = fmul(dx, dx) + fmul(dy, dy);
            if (d2 < (32 << 8) * (32 << 8) / 256) {
                if (sprites[i].type == 2) {
                    player_hp += 25;
                    if (player_hp > 100) player_hp = 100;
                } else {
                    ammo += 10;
                }
                sprites[i].state = 0;
                changed = 1;
            }
            continue;
        }
        int dx = px - sprites[i].x;
        int dy = py - sprites[i].y;
        int d2 = fmul(dx, dx) + fmul(dy, dy);
        if (d2 > (200 << 8) * (200 << 8) / 256) continue;

        sprites[i].anim = (sprites[i].anim + 1) & 63;

        int speed = (1 << 8) * 1;
        int sx_step = (dx > 0) ? speed : -speed;
        int sy_step = (dy > 0) ? speed : -speed;

        int nx = sprites[i].x + sx_step;
        int mx = nx >> 8, my = sprites[i].y >> 8;
        if (mx > 0 && mx < MAP_W-1 && my > 0 && my < MAP_H-1 && map[my][mx] == 0)
            sprites[i].x = nx;

        int ny = sprites[i].y + sy_step;
        mx = sprites[i].x >> 8; my = ny >> 8;
        if (mx > 0 && mx < MAP_W-1 && my > 0 && my < MAP_H-1 && map[my][mx] == 0)
            sprites[i].y = ny;

        changed = 1;
        if (d2 < (48 << 8) * (48 << 8) / 256) {
            if (now - sprites[i].last_attack >= 900) {
                sprites[i].last_attack = now;
                player_hp -= 7;
                last_damage_ms = now;
                damage_flash = 4;
                if (player_hp <= 0) {
                    player_hp = 0;
                    dead = 1;
                    final_ms = timer_ms() - start_ms;
                }
            }
        }
    }

    if (damage_flash > 0) damage_flash--;
    return changed;
}
#define PLAY_TOP    32
#define PLAY_BOTTOM 448
#define PLAY_H      (PLAY_BOTTOM - PLAY_TOP)
#define PLAY_CY     (PLAY_TOP + PLAY_H / 2)
#define LINE_K_CONST (PLAY_H / 2 * 256)

#define LINE_K LINE_K_CONST

static unsigned char wall_color(int cell, int side) {
    switch (cell) {
        case 1: return side ? DARK_GRAY  : LIGHT_GRAY;
        case 2: return side ? BROWN      : YELLOW;
        case 3: return side ? LIGHT_GRAY : WHITE;
        case 4: return side ? LIGHT_RED  : RED;
        case 5: return side ? GREEN      : LIGHT_GREEN;
    }
    return LIGHT_GRAY;
}

static int cast_column(int col, int *out_cell, int *out_side) {
    int ray_ang = pa + ((col - 320) * 17 >> 8);
    ray_ang &= 255;

    int dx = fcos(ray_ang);
    int dy = fsin(ray_ang);

    int map_x = px >> 8;
    int map_y = py >> 8;

    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int ddx = (adx == 0) ? 0x7FFFFFFF : ((FONE << 8) / adx);
    int ddy = (ady == 0) ? 0x7FFFFFFF : ((FONE << 8) / ady);

    int step_x, step_y, sd_x, sd_y;
    if (dx < 0) { step_x = -1; sd_x = fmul(px - (map_x << 8), ddx); }
    else        { step_x =  1; sd_x = fmul(((map_x + 1) << 8) - px, ddx); }
    if (dy < 0) { step_y = -1; sd_y = fmul(py - (map_y << 8), ddy); }
    else        { step_y =  1; sd_y = fmul(((map_y + 1) << 8) - py, ddy); }

    int side = 0, cell = 1;
    for (int i = 0; i < 64; i++) {
        if (sd_x < sd_y) { sd_x += ddx; map_x += step_x; side = 0; }
        else             { sd_y += ddy; map_y += step_y; side = 1; }

        if (map_x < 0 || map_x >= MAP_W || map_y < 0 || map_y >= MAP_H) {
            cell = 1; break;
        }
        if (map_x == exit_x && map_y == exit_y) { cell = 5; break; }
        if (map[map_y][map_x] != 0) { cell = map[map_y][map_x]; break; }
    }

    *out_cell = cell;
    *out_side = side;
    return side ? (sd_y - ddy) : (sd_x - ddx);
}
static const unsigned char spr_enemy[8] = {
    0x3C, 0x7E, 0xE7, 0xFF, 0xDB, 0xFF, 0x7E, 0x3C
};
static const unsigned char spr_health[8] = {
    0x18, 0x18, 0x7E, 0x7E, 0x7E, 0x7E, 0x18, 0x18
};
static const unsigned char spr_ammo[8] = {
    0x00, 0x24, 0x24, 0x24, 0x7E, 0x7E, 0x7E, 0x00
};

static unsigned char sprite_pixel(int type, int tx, int ty, int anim) {
    if (tx < 0 || tx > 7 || ty < 0 || ty > 7) return 0xFF;
    const unsigned char *spr = (type == 1) ? spr_enemy :
                               (type == 2) ? spr_health : spr_ammo;
    int b = (spr[ty] >> (7 - tx)) & 1;
    if (!b) return 0xFF;

    switch (type) {
        case 1: {
            if (((anim >> 3) & 1) && ty >= 2 && ty <= 4 && tx >= 2 && tx <= 5)
                return YELLOW;
            return LIGHT_RED;
        }
        case 2: return LIGHT_GREEN;
        case 3: return YELLOW;
    }
    return WHITE;
}

static void draw_sprites(void) {
    for (int i = 0; i < sprite_count; i++) {
        if (!sprites[i].state) continue;

        int dx = sprites[i].x - px;
        int dy = sprites[i].y - py;

        int depth = fmul(dx, fcos(pa)) + fmul(dy, fsin(pa));
        if (depth < 64) continue;

        int lateral = fmul(dx, fcos((pa+64)&255)) + fmul(dy, fsin((pa+64)&255));

        int screen_cx = 320 + (lateral * 320) / depth;
        int sprite_h = LINE_K / depth;
        if (sprite_h < 4) continue;
        if (sprite_h > PLAY_H * 2) sprite_h = PLAY_H * 2;
        int sprite_w = sprite_h;

        int sl = screen_cx - sprite_w / 2;
        int sr = screen_cx + sprite_w / 2;
        if (sr < 0 || sl >= 640) continue;

        int top = PLAY_CY - sprite_h / 2;
        int bot = PLAY_CY + sprite_h / 2;

        for (int col = sl; col < sr; col++) {
            if (col < 0 || col >= 640) continue;
            if (zbuf[col] < depth) continue;

            int tx = ((col - sl) * 8) / sprite_w;
            for (int y = top; y < bot; y++) {
                if (y < PLAY_TOP || y >= PLAY_BOTTOM) continue;
                int ty = ((y - top) * 8) / sprite_h;
                unsigned char c = sprite_pixel(sprites[i].type, tx, ty, sprites[i].anim);
                if (c == 0xFF) continue;
                gfx_putpixel(col, y, c);
            }
        }
    }
}
#define MM_CELL 4
static void draw_minimap(void) {
    const int MX = 6, MY = 36;
    int mw = MAP_W * MM_CELL, mh = MAP_H * MM_CELL;

    gfx_rect(MX - 2, MY - 2, mw + 4, mh + 4, BLACK);
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            unsigned char c = map[y][x] ? LIGHT_GRAY : BLACK;
            if (x == exit_x && y == exit_y) c = GREEN;
            gfx_rect(MX + x*MM_CELL, MY + y*MM_CELL, MM_CELL, MM_CELL, c);
        }
    for (int i = 0; i < sprite_count; i++) {
        if (!sprites[i].state) continue;
        int mx = sprites[i].x >> 8;
        int my = sprites[i].y >> 8;
        unsigned char c = (sprites[i].type == 1) ? RED :
                          (sprites[i].type == 2) ? LIGHT_GREEN : YELLOW;
        gfx_rect(MX + mx*MM_CELL, MY + my*MM_CELL, MM_CELL, MM_CELL, c);
    }

    int ppx = MX + (px >> 8) * MM_CELL;
    int ppy = MY + (py >> 8) * MM_CELL;
    gfx_rect(ppx, ppy, MM_CELL, MM_CELL, LIGHT_CYAN);
}
static void draw_hud_bottom(void) {
    gfx_rect(0, 448, 640, 32, THEME_BAR);
    int hp_w = player_hp * 100 / 100;
    gfx_rect(8, 456, 104, 16, BLACK);
    gfx_rect(10, 458, hp_w, 12, player_hp > 40 ? LIGHT_GREEN : RED);
    gfx_puts(120, 456, "HP", WHITE, THEME_BAR);
    char buf[24];
    int i = 0;
    const char *p = "AMMO ";
    while (*p) buf[i++] = *p++;
    {
        int v = ammo;
        char t[6]; int tt = 0;
        if (v == 0) t[tt++] = '0';
        while (v) { t[tt++] = '0' + (v % 10); v /= 10; }
        while (tt) buf[i++] = t[--tt];
    }
    buf[i] = 0;
    gfx_puts(170, 456, buf, WHITE, THEME_BAR);
    i = 0;
    p = "SCORE ";
    while (*p) buf[i++] = *p++;
    {
        int v = score;
        char t[8]; int tt = 0;
        if (v == 0) t[tt++] = '0';
        while (v) { t[tt++] = '0' + (v % 10); v /= 10; }
        while (tt) buf[i++] = t[--tt];
    }
    buf[i] = 0;
    gfx_puts(300, 456, buf, YELLOW, THEME_BAR);

    const char *hint = "WASD | ARROWS | SPACE=shoot | ESC";
    int hl = 0; while (hint[hl]) hl++;
    gfx_puts(640 - hl*8 - 8, 456, hint, WHITE, THEME_BAR);
}
static void draw_crosshair(void) {
    int cx = 320, cy = PLAY_CY;
    unsigned char c = (damage_flash && (damage_flash & 1)) ? RED : WHITE;
    for (int i = -8; i <= 8; i++) {
        if (i > -3 && i < 3) continue;
        gfx_putpixel(cx + i, cy, c);
        gfx_putpixel(cx, cy + i, c);
    }
    gfx_putpixel(cx, cy, c);
}
static void draw_win_overlay(void) {
    int bx = 140, by = 170, bw = 360, bh = 130;
    gfx_rect(bx, by, bw, bh, BLACK);
    gfx_rect(bx + 2, by + 2, bw - 4, bh - 4, LIGHT_GREEN);

    gfx_puts(bx + 86, by + 16, "YOU ESCAPED!", BLACK, LIGHT_GREEN);

    unsigned int secs = final_ms / 1000u;
    char buf[32]; int i = 0;
    const char *p = "Time:  ";
    while (*p) buf[i++] = *p++;
    buf[i++] = '0' + (secs / 600);
    buf[i++] = ':';
    buf[i++] = '0' + ((secs / 60) % 10);
    buf[i++] = '0' + (secs % 10);
    buf[i] = 0;
    gfx_puts(bx + 30, by + 50, buf, BLACK, LIGHT_GREEN);

    i = 0;
    p = "Score: ";
    while (*p) buf[i++] = *p++;
    {
        int v = score;
        char t[8]; int tt = 0;
        if (v == 0) t[tt++] = '0';
        while (v) { t[tt++] = '0' + (v % 10); v /= 10; }
        while (tt) buf[i++] = t[--tt];
    }
    buf[i] = 0;
    gfx_puts(bx + 30, by + 70, buf, BLACK, LIGHT_GREEN);

    gfx_puts(bx + 20, by + 100, "SPACE = new maze | ESC = games", BLACK, LIGHT_GREEN);
}

static void draw_dead_overlay(void) {
    int bx = 160, by = 170, bw = 320, bh = 130;
    gfx_rect(bx, by, bw, bh, BLACK);
    gfx_rect(bx + 2, by + 2, bw - 4, bh - 4, RED);

    gfx_puts(bx + 116, by + 16, "YOU DIED", WHITE, RED);

    char buf[32]; int i = 0;
    const char *p = "Score: ";
    while (*p) buf[i++] = *p++;
    {
        int v = score;
        char t[8]; int tt = 0;
        if (v == 0) t[tt++] = '0';
        while (v) { t[tt++] = '0' + (v % 10); v /= 10; }
        while (tt) buf[i++] = t[--tt];
    }
    buf[i] = 0;
    gfx_puts(bx + 60, by + 55, buf, WHITE, RED);

    gfx_puts(bx + 40, by + 100, "SPACE = restart | ESC = games", WHITE, RED);
}
void raycast_draw(void) {
    unsigned char ceil_c  = (THEME_BG == WHITE) ? LIGHT_GRAY : DARK_GRAY;
    unsigned char floor_c = (THEME_BG == WHITE) ? BROWN      : BLACK;

    gfx_rect(0, PLAY_TOP, 640, PLAY_H / 2, ceil_c);
    gfx_rect(0, PLAY_CY,  640, PLAY_H / 2, floor_c);
    for (int col = 0; col < 640; col++) {
        int cell, side;
        int perp = cast_column(col, &cell, &side);
        if (perp < 1) perp = 1;
        zbuf[col] = perp;

        int line_h = LINE_K / perp;
        if (cell == 5) line_h = line_h / 2;
        if (line_h > PLAY_H) line_h = PLAY_H;
        if (line_h < 1) line_h = 1;

        int ds = PLAY_CY - line_h / 2;
        int de = PLAY_CY + line_h / 2;
        if (ds < PLAY_TOP)    ds = PLAY_TOP;
        if (de > PLAY_BOTTOM) de = PLAY_BOTTOM;

        unsigned char c = (cell == 5) ? ((col & 1) ? LIGHT_GREEN : GREEN)
                                      : wall_color(cell, side);
        gfx_vline_fast(col, ds, de, c);
    }
    draw_sprites();
    if (damage_flash) {
        for (int i = 0; i < 6; i++) {
            gfx_rect(0, PLAY_TOP + i, 640, 1, RED);
            gfx_rect(0, PLAY_BOTTOM - 1 - i, 640, 1, RED);
        }
    }

    draw_minimap();
    draw_hud_bottom();
    draw_crosshair();

    if (won)  draw_win_overlay();
    if (dead) draw_dead_overlay();
}