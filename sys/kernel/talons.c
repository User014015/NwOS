#include "talons.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

#define HMW 512
#define HMH 512
#define HM_MASK 511

static unsigned char heightmap[HMH][HMW];

static int cam_x, cam_y, cam_z, cam_a, cam_speed;
static int cam_pitch;
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
    for (int y = cy - r - 2; y <= cy + r + 2; y++) {
        for (int x = cx - r - 2; x <= cx + r + 2; x++) {
            if ((unsigned)x >= HMW || (unsigned)y >= HMH) continue;
            int dx = x - cx, dy = y - cy;
            int d2 = dx*dx + dy*dy;
            if (d2 <= r2) {
                int h = 42 + (d2 * 26) / r2;
                put_height(x, y, h);
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
        int jit = (fsin(phase * 16) * 4) >> 8;
        if (adx > ady) y += jit; else x += jit;
        phase++;
        for (int ky = -2; ky <= 2; ky++)
            for (int kx = -2; kx <= 2; kx++)
                put_height(x + kx, y + ky, 50);
    }
}

static void generate_terrain(void) {
    for (int y = 0; y < HMH; y++) {
        for (int x = 0; x < HMW; x++) {
            int h = 100;
            h += (fsin((x*3  + y*2 ) & 255) * 12) >> 8;
            h += (fsin((x*5  + y*7 ) & 255) * 10) >> 8;
            h += (fsin((x*11 + y*13) & 255) *  8) >> 8;
            h += (fsin((x*23 + y*17) & 255) *  5) >> 8;
            if (h < 0) h = 0;
            if (h > 255) h = 255;
            heightmap[y][x] = (unsigned char)h;
        }
    }

    carve_lake( 80,  80, 40);
    carve_lake(260, 100, 45);
    carve_lake(430,  80, 35);
    carve_lake(130, 270, 42);
    carve_lake(340, 300, 50);
    carve_lake(470, 380, 38);
    carve_lake(200, 430, 45);
    carve_lake(450, 220, 32);
    carve_lake( 60, 400, 35);
    carve_lake(280, 200, 28);

    carve_river( 80,  80, 130, 270);
    carve_river(130, 270, 200, 430);
    carve_river(260, 100, 280, 200);
    carve_river(280, 200, 340, 300);
    carve_river(430,  80, 450, 220);
    carve_river(450, 220, 470, 380);
    carve_river(340, 300, 470, 380);
    carve_river(200, 430, 340, 300);
    carve_river( 60, 400, 200, 430);
}

void talons_init(void) {
    rng_t = ((unsigned int)timer_read() << 7) | 0x5A5A;
    if (rng_t == 0) rng_t = 1;

    generate_terrain();
    cam_x = 200 << 8;
    cam_y = 220 << 8;
    cam_z = 130;
    cam_a = 0;
    cam_speed = 3;
    cam_pitch = 0;
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
        case KEY_LEFT:   cam_a = (cam_a + 252) & 255; break;
        case KEY_RIGHT:  cam_a = (cam_a + 4)   & 255; break;
        case KEY_UP:     cam_pitch += 6; if (cam_pitch >  100) cam_pitch =  100; break;
        case KEY_DOWN:   cam_pitch -= 6; if (cam_pitch < -100) cam_pitch = -100; break;
        case KEY_PGUP:   cam_z += 4; if (cam_z > 200) cam_z = 200; break;
        case KEY_PGDN:   cam_z -= 4; if (cam_z <  80) cam_z =  80; break;
        case 'w': case 'W': cam_speed++; if (cam_speed > 8) cam_speed = 8; break;
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

    int bob = (fsin(bob_phase * 2) * 3) >> 8;
    horizon = 240 + cam_pitch / 2 + bob;

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
    if (h < 70)  return BLUE;
    if (h < 110) return LIGHT_GREEN;
    return GREEN;
}

static void draw_sky(void) {
    for (int y = 32; y < 448; y++) gfx_hline(0, y, 640, LIGHT_CYAN);
}

static void draw_fishing(void) {
    if (fishing_state == 0) return;

    int cx = 320;
    int top = horizon + 20;
    if (top < 32)  top = 32;
    if (top > 420) top = 420;

    int wiggle = 0;
    if (fishing_state == 2) wiggle = (fsin(bob_phase * 8) * 8) >> 8;

    for (int y = top; y < 430; y++) {
        int offset = 0;
        if (fishing_state == 2) offset = (wiggle * (y - top)) / 100;
        gfx_putpixel(cx + offset,     y, BLACK);
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

void talons_draw(void) {
    draw_sky();

    int h_center = 32 + 208;

    for (int col = 0; col < 640; col++) {
        int ray_ang = cam_a + ((col - 320) * 10 >> 8);
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
                    unsigned char c = terrain_color(h);
                    gfx_vline_fast(col, ys, ye, c);
                }
                y_min = sy;
                reached = 1;
            }

            if (y_min <= 32) break;

            int step;
            if (d < 40)       step = 1;
            else if (d < 80)  step = 2;
            else if (d < 130) step = 3;
            else if (d < 180) step = 4;
            else              step = 5;
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

    gfx_rect(0, 448, 640, 32, THEME_BAR);

    char buf[80]; int i = 0;
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
    p = "PIT "; while (*p) buf[i++] = *p++;
    v = cam_pitch;
    if (v < 0) { buf[i++] = '-'; v = -v; } else { buf[i++] = '+'; }
    if (v > 99) v = 99;
    buf[i++] = '0' + (v / 10);
    buf[i++] = '0' + (v % 10);

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

    const char *hint = "W/S=spd  U/D=pitch  PGUP/PGDN=alt  F=fish";
    int hl = 0; while (hint[hl]) hl++;
    gfx_puts(640 - hl * 8 - 8, 456, hint, THEME_BAR_FG, THEME_BAR);
}