#include "demo3d.h"
#include "graphics.h"
#include "timer.h"

static const int sintab[64] = {
    0,   98,  195,  290,  383,  471,  556,  634,
    707, 773,  831,  882,  924,  957,  981,  995,
    1000, 995, 981,  957,  924,  882,  831,  773,
    707, 634,  556,  471,  383,  290,  195,   98,
    0,  -98, -195, -290, -383, -471, -556, -634,
    -707,-773,-831, -882, -924, -957, -981, -995,
    -1000,-995,-981,-957,-924,-882,-831,-773,
    -707,-634,-556,-471,-383,-290,-195, -98
};

static int isin(unsigned int a) { return sintab[a & 63]; }
static int icos(unsigned int a) { return sintab[(a + 16) & 63]; }

static unsigned int angle_y = 0;
static unsigned int angle_x = 0;
static unsigned int last_tick_ms = 0;

void demo3d_init(void) {
    angle_y = 0;
    angle_x = 0;
    last_tick_ms = timer_ms();
}

int demo3d_update(void) {
    unsigned int now = timer_ms();
    if (now - last_tick_ms < 33) return 0;
    last_tick_ms = now;
    angle_y = (angle_y + 1) & 63;
    angle_x = (angle_x + 1) & 63;
    return 1;
}

static void line3d(int x0, int y0, int x1, int y1, unsigned char c) {
    int dx = x1 - x0; if (dx < 0) dx = -dx;
    int dy = y1 - y0; if (dy < 0) dy = -dy;
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (1) {
        gfx_putpixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

static int project(int x, int y, int z, int *out_x, int *out_y) {
    if (z < 100) return 0;
    *out_x = 320 + (x * 500) / z;
    *out_y = 240 - (y * 500) / z;
    return 1;
}

static void rot_y(int *x, int *z, unsigned int ang) {
    int s = isin(ang), c = icos(ang);
    int nx = (*x * c + *z * s) / 1000;
    int nz = (-(*x) * s + *z * c) / 1000;
    *x = nx; *z = nz;
}

static void rot_x(int *y, int *z, unsigned int ang) {
    int s = isin(ang), c = icos(ang);
    int ny = (*y * c - *z * s) / 1000;
    int nz = (*y * s + *z * c) / 1000;
    *y = ny; *z = nz;
}
static const int cube_verts[8][3] = {
    {-300,-300,-300}, { 300,-300,-300}, { 300, 300,-300}, {-300, 300,-300},
    {-300,-300, 300}, { 300,-300, 300}, { 300, 300, 300}, {-300, 300, 300}
};
static const int cube_edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

static void draw_cube(int cx, int cy, int cz) {
    int sx[8], sy[8], ok[8];
    for (int i = 0; i < 8; i++) {
        int x = cube_verts[i][0];
        int y = cube_verts[i][1];
        int z = cube_verts[i][2];
        rot_y(&x, &z, angle_y);
        rot_x(&y, &z, angle_x);
        x += cx; y += cy; z += cz;
        ok[i] = project(x, y, z, &sx[i], &sy[i]);
    }
    for (int e = 0; e < 12; e++) {
        int a = cube_edges[e][0];
        int b = cube_edges[e][1];
        if (ok[a] && ok[b]) line3d(sx[a], sy[a], sx[b], sy[b], LIGHT_CYAN);
    }
}

static void draw_floor(void) {
    const int tile = 400;
    const int y0 = -400;
    unsigned char c1 = (THEME_BG == WHITE) ? LIGHT_GRAY : DARK_GRAY;

    for (int zi = 0; zi < 5; zi++) {
        for (int xi = -3; xi <= 3; xi++) {
            int x  = xi * tile;
            int z0 = 800 + zi * tile;
            int z1 = z0 + tile;

            int px0, py0, px1, py1, px2, py2, px3, py3;
            if (!project(x,        y0, z0, &px0, &py0)) continue;
            if (!project(x + tile, y0, z0, &px1, &py1)) continue;
            if (!project(x + tile, y0, z1, &px2, &py2)) continue;
            if (!project(x,        y0, z1, &px3, &py3)) continue;

            unsigned char col = ((xi + zi) & 1) ? THEME_FG : c1;
            line3d(px0, py0, px1, py1, col);
            line3d(px1, py1, px2, py2, col);
            line3d(px2, py2, px3, py3, col);
            line3d(px3, py3, px0, py0, col);
        }
    }
}

void demo3d_draw(void) {
    gfx_clear(THEME_BG);
    draw_floor();
    draw_cube(0, 0, 1500);

    gfx_rect(0, 32, 640, 32, THEME_BG);
    gfx_puts(8, 40, "3D DEMO", YELLOW, THEME_BG);
    gfx_puts(120, 40, "Rotating cube + tile floor", THEME_FG, THEME_BG);
    gfx_puts(500, 40, "ESC = back", THEME_FG, THEME_BG);
}