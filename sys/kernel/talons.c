#include "talons.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

#define HMW 256
#define HMH 256
#define HM_MASK 255

static unsigned char heightmap[HMH][HMW];

static int cam_x, cam_y, cam_z, cam_a, cam_speed;
static int horizon, bob_phase;
static int fishing_state;
static unsigned int fishing_bite_at;
static unsigned int fishing_result_at;
static int fish_count;
static int fishing_result_ok;

static unsigned int rng_t = 1;
static unsigned int rnd_t(void) {
    rng_t = rng_t * 1103515245u + 12345u;
    return (rng_t >> 16) & 0x7FFF;
}

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
static int fmul(int a, int b) { return (a * b) >> 8; }
static void put_height(int x, int y, int h) {
    if ((unsigned)x >= HMW || (unsigned)y >= HMH) return;
    if (h < 0) h = 0;
    if (h > 255) h = 255;
    heightmap[y][x] = (unsigned char)h;
}

static void carve_lake(int cx, int cy, int r) {
    int r2 = r * r;
    for (int y = cy - r - 3; y <= cy + r + 3; y++) {
        for (int x = cx - r - 3; x <= cx + r + 3; x++) {
            if ((unsigned)x >= HMW || (unsigned)y >= HMH) continue;
            int dx = x - cx, dy = y - cy;
            int d2 = dx*dx + dy*dy;
            if (d2 <= r2) {
                int t = (r2 - d2) / (r + 1);
                put_height(x, y, 45 - t / 3);
            }
        }
    }
}

static void carve_river(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0, dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int steps = adx > ady ? adx : ady;
    if (steps == 0) return;
    int phase = 0;
    for (int i = 0; i <= steps; i++) {
        int x = x0 + (dx * i) / steps;
        int y = y0 + (dy * i) / steps;
        int jit = (fsin(phase * 16) * 5) >> 8;
        if (adx > ady) y += jit; else x += jit;
        phase++;
        for (int ky = -1; ky <= 1; ky++)
            for (int kx = -1; kx <= 1; kx++)
                put_height(x + kx, y + ky, 48);
    }
}

static void generate_terrain(void) {
    for (int y = 0; y < HMH; y++) {
        for (int x = 0; x < HMW; x++) {
            int h = 130;
            h += (fsin((x*3  + y*2 ) & 255) * 30) >> 8;
            h += (fsin((x*7  + y*5 ) & 255) * 22) >> 8;
            h += (fsin((x*13 + y*11) & 255) * 16) >> 8;
            h += (fsin((x*29 + y*23) & 255) * 11) >> 8;
            h += (fsin((x*57 + y*43) & 255) *  7) >> 8;
            if (h < 0) h = 0;
            if (h > 255) h = 255;
            heightmap[y][x] = (unsigned char)h;
        }
    }
    carve_lake( 60,  60, 22);
    carve_lake(200,  60, 26);
    carve_lake( 60, 200, 24);
    carve_lake(200, 200, 28);
    carve_lake(128, 128, 20);

    carve_river( 60,  60, 128, 128);
    carve_river(128, 128, 200,  60);
    carve_river( 60, 200, 128, 128);
    carve_river(128, 128, 200, 200);
    carve_river(  0, 128,  60,  60);
    carve_river(200,  60, 255, 128);
    carve_river( 60, 200,   0, 255);
    carve_river(200, 200, 255, 200);
    carve_river(128,   0, 128, 128);
    carve_river(128, 128, 128, 255);
}

void talons_init(void) {
    rng_t = ((unsigned int)timer_read() << 7) | 0x5A5A;
    if (rng_t == 0) rng_t = 1;

    generate_terrain();
    cam_x = 128 << 8;
    cam_y = 100 << 8;
    cam_z = 140;
    cam_a = 64;
    cam_speed = 2;
    horizon = 240;
    bob_phase = 0;
    fishing_state = 0;
    fish_count = 0;
    fishing_result_ok = 0;
}

static int over_water(void) {
    int mx = (cam_x >> 8) & HM_MASK;
    int my = (cam_y >> 8) & HM_MASK;
    return heightmap[my][mx] < 70;
}

void talons_handle_key(unsigned char key) {
    switch (key) {
        case KEY_LEFT:  cam_a = (cam_a + 252) & 255; break;
        case KEY_RIGHT: cam_a = (cam_a + 4)   & 255; break;
        case KEY_UP:    cam_z += 3; if (cam_z > 240) cam_z = 240; break;
        case KEY_DOWN:  cam_z -= 3; if (cam_z <  60) cam_z =  60; break;
        case 'w': case 'W': cam_speed++; if (cam_speed > 6) cam_speed = 6; break;
        case 's': case 'S': cam_speed--; if (cam_speed < 1) cam_speed = 1; break;

        case 'f': case 'F':
            if (fishing_state == 0) {
                if (over_water()) {
                    fishing_state = 1;
                    fishing_bite_at = timer_ms() + 800u + (rnd_t() % 1500u);
                }
            } else if (fishing_state == 2) {
                fishing_state = 3;
                fishing_result_ok = 1;
                fish_count++;
                fishing_result_at = timer_ms() + 1500u;
            }
            break;
    }
}

int talons_update(void) {
    bob_phase = (bob_phase + 2) & 255;
    if (fishing_state == 0) {
        int speed_fx = cam_speed << 8;
        cam_x += fmul(fcos(cam_a), speed_fx);
        cam_y += fmul(fsin(cam_a), speed_fx);

        int world_size = HMH << 8;
        while (cam_x < 0)            cam_x += world_size;
        while (cam_x >= world_size)  cam_x -= world_size;
        while (cam_y < 0)            cam_y += world_size;
        while (cam_y >= world_size)  cam_y -= world_size;
    }

    int bob = (fsin(bob_phase * 2) * 4) >> 8;
    horizon = 240 + bob;
    unsigned int now = timer_ms();
    if (fishing_state == 1 && now >= fishing_bite_at) {
        fishing_state = 2;
        fishing_result_at = now + 900u;
    }
    if (fishing_state == 2 && now >= fishing_result_at) {
        fishing_state = 3;
        fishing_result_ok = 0;
        fishing_result_at = now + 1200u;
    }
    if (fishing_state == 3 && now >= fishing_result_at) {
        fishing_state = 0;
    }

    return 1;
}
static unsigned char terrain_color(int h) {
    if (h < 52)  return BLUE;
    if (h < 66)  return LIGHT_BLUE;
    if (h < 74)  return CYAN;
    if (h < 82)  return YELLOW;
    if (h < 95)  return LIGHT_GREEN;
    if (h < 175) return GREEN;
    if (h < 210) return LIGHT_GREEN;
    if (h < 228) return BROWN;
    if (h < 245) return DARK_GRAY;
    return WHITE;
}

static unsigned char apply_fog(unsigned char c, int d) {
    if (d < 80) return c;
    if (d < 140) {
        if (c == GREEN)       return DARK_GRAY;
        if (c == LIGHT_GREEN) return GREEN;
        if (c == BROWN)       return DARK_GRAY;
    }
    if (c == LIGHT_BLUE || c == CYAN) return LIGHT_CYAN;
    if (c == GREEN || c == LIGHT_GREEN) return LIGHT_GREEN;
    return LIGHT_GRAY;
}

static void draw_sky(void) {
    for (int y = 32; y < 240; y++) {
        unsigned char c;
        if (y < 80)       c = LIGHT_BLUE;
        else if (y < 140) c = CYAN;
        else if (y < 200) c = LIGHT_CYAN;
        else              c = WHITE;
        gfx_hline(0, y, 640, c);
    }
}

static void draw_fishing(void) {
    if (fishing_state == 0) return;

    int cx = 320;
    int top = horizon + 20;
    if (top < 32) top = 32;
    if (top > 420) top = 420;

    int wiggle = 0;
    if (fishing_state == 2) wiggle = (fsin(bob_phase * 8) * 8) >> 8;

    for (int y = top; y < 430; y++) {
        int offset = 0;
        if (fishing_state == 2) offset = (wiggle * (y - top)) / 100;
        gfx_putpixel(cx + offset, y, BLACK);
        gfx_putpixel(cx + offset + 1, y, WHITE);
    }

    if (fishing_state == 1) {
        gfx_puts(220, 60, "Waiting for a bite...", WHITE, BLACK);
    } else if (fishing_state == 2) {
        gfx_puts(260, 60, "BITE! Press F", YELLOW, RED);
    } else if (fishing_state == 3) {
        if (fishing_result_ok)
            gfx_puts(230, 60, "Caught! +1 fish", LIGHT_GREEN, BLACK);
        else
            gfx_puts(230, 60, "Missed...", LIGHT_RED, BLACK);
    }
}
// render
void talons_draw(void) {
    draw_sky();

    int h_center = 32 + 208;

    for (int col = 0; col < 640; col++) {
        int ray_ang = cam_a + ((col - 320) * 17 >> 8);
        ray_ang &= 255;

        int dx = fcos(ray_ang);
        int dy = fsin(ray_ang);

        int y_min = 448;
        int reached = 0;
        int d = 200;
        int guard = 0;

        while (d >= 2 && guard++ < 400) {
            int wx = cam_x + d * dx;
            int wy = cam_y + d * dy;
            int mx = (wx >> 8) & HM_MASK;
            int my = (wy >> 8) & HM_MASK;
            int h  = heightmap[my][mx];

            int dh = h - cam_z;
            int sy = horizon - (dh * 500) / d;
            if (sy < -50) sy = -50;
            if (sy > 500) sy = 500;

            if (!reached || sy < y_min) {
                int ys = sy < 32 ? 32 : sy;
                int ye = y_min;
                if (ye > 448) ye = 448;
                if (ys < ye) {
                    unsigned char c = apply_fog(terrain_color(h), d);
                    gfx_vline_fast(col, ys, ye, c);
                }
                y_min = sy;
                reached = 1;
            }

            if (y_min <= 32) break;
            int step;
            if (d < 25)       step = 1;
            else if (d < 50)  step = 2;
            else if (d < 90)  step = 3;
            else if (d < 140) step = 5;
            else              step = 8;
            d -= step;
        }
    }
    int cx = 320, cy = h_center;
    for (int i = -6; i <= 6; i++) {
        if (i != 0) {
            gfx_putpixel(cx + i, cy, WHITE);
            gfx_putpixel(cx, cy + i, WHITE);
        }
    }

    draw_fishing();

    // hud
    gfx_rect(0, 448, 640, 32, THEME_BAR);

    char buf[64];
    int i = 0;
    const char *p;
    int v;

    p = "ALT "; while (*p) buf[i++] = *p++;
    v = cam_z;
    { char t[8]; int tt = 0; if (v == 0) t[tt++] = '0';
      while (v > 0) { t[tt++] = '0' + (v % 10); v /= 10; }
      while (tt) buf[i++] = t[--tt]; }

    buf[i++] = ' '; buf[i++] = ' ';
    p = "SPD "; while (*p) buf[i++] = *p++;
    buf[i++] = '0' + cam_speed;

    buf[i++] = ' '; buf[i++] = ' ';
    p = "FISH "; while (*p) buf[i++] = *p++;
    v = fish_count;
    { char t[8]; int tt = 0; if (v == 0) t[tt++] = '0';
      while (v > 0) { t[tt++] = '0' + (v % 10); v /= 10; }
      while (tt) buf[i++] = t[--tt]; }

    if (over_water()) {
        buf[i++] = ' '; buf[i++] = ' ';
        p = "[WATER]"; while (*p) buf[i++] = *p++;
    }
    buf[i] = 0;

    gfx_puts(8, 456, buf, THEME_BAR_FG, THEME_BAR);

    const char *hint = "WASD speed | ARROWS | F=fish | ESC";
    int hl = 0; while (hint[hl]) hl++;
    gfx_puts(640 - hl*8 - 8, 456, hint, THEME_BAR_FG, THEME_BAR);
}