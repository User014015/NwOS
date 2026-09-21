#include "graphics.h"
#include "keyboard.h"
#include "mouse.h"
#include "io.h"
#include "shell.h"
#include "metrics.h"
#include "timer.h"
#include "snake.h"
#include "demo3d.h"
#include "raycast.h"
#include "talons.h"
#include "chat.h"
#include "fs.h"
#include "editor.h"

static void redraw(void);
static void draw_topbar(void);
unsigned char g_last_key_debug = 0;
int g_after_buflen = 0;
int g_ekh_count = 0;

int g_buf_at_open   = 0;
int g_buf_before_ek = 0;
int g_buf_after_ek  = 0;

typedef enum {
    SCR_WELCOME, SCR_MENU, SCR_GAMES, SCR_SHELL,
    SCR_CALC, SCR_GAME, SCR_SNAKE, SCR_DEMO3D, SCR_RAYCAST, SCR_TALONS,
    SCR_CHAT, SCR_EDITOR
} screen_t;
static screen_t current = SCR_WELCOME;

// Welcome
static const char *welcome_items[] = { "Games", "Editor", "Shell", "Chat", "Reboot" };
#define WELCOME_N 5
static int welcome_sel = 0;
#define WELCOME_BX 120
#define WELCOME_BY 110
#define WELCOME_BW 400
#define WELCOME_BH 50
#define WELCOME_BGAP 10

// Menu
static const char *menu_items[] = { "Shell", "Editor", "Compiler", "Games", "Help", "About", "Reboot" };
#define MENU_N 7
static int menu_sel = 0;
#define MENU_MX 80
#define MENU_MY 80
#define MENU_MW 480
#define MENU_MH 320
#define MENU_ROW_H 32

// Games menu
static const char *games_items[] = { "Snake", "3D Demo", "Castle NwOS", "Talons", "Back" };
#define GAMES_N 5
static int games_sel = 0;
#define GAMES_MX 80
#define GAMES_MY 100
#define GAMES_MW 480
#define GAMES_MH 260
#define GAMES_ROW_H 44

// calcualtor

static char calc_buf[32];
static int  calc_len = 0;
static int  calc_result = 0;
static int  calc_result_valid = 0;



static char game_name[32] = "?";

static int cursor_x = 320, cursor_y = 240;

static unsigned char g_last_key = 0;
static int           g_last_want = 0;

int g_open_count = 0;

int g_dbg_buflen = 0;

static unsigned int g_fps = 0;
void shell_goto_menu(void)    { current = SCR_MENU;    }
void shell_apply_theme(void) { redraw(); }
void shell_goto_welcome(void) { current = SCR_WELCOME; }
void shell_run_calc(void) {
    current = SCR_CALC;
    calc_len = 0; calc_buf[0] = 0; calc_result_valid = 0;
}
void shell_run_chat(void) {
    chat_init();
    current = SCR_CHAT;
}
void shell_run_game2d(const char *name) {
    const char *needle = "snake";
    const char *s = name;
    int is_snake = 1;
    while (*needle) { if (*s++ != *needle++) { is_snake = 0; break; } }
    if (is_snake && *s == 0) {
        snake_init();
        current = SCR_SNAKE;
        return;
    }
    current = SCR_GAME;
    int i = 0;
    while (name[i] && i < 31) { game_name[i] = name[i]; i++; }
    game_name[i] = 0;
}
void shell_run_editor(const char *name) {
    editor_open(name);
    current = SCR_EDITOR;
    extern int editor_want_quit_debug(void);
    g_last_want = editor_want_quit_debug();
}
static const unsigned char cursor_shape[18][11] = {
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,0,0,0,0,0},
    {0,1,1,0,0,0,0,0,0,0,0},
    {0,1,0,1,0,0,0,0,0,0,0},
    {0,1,0,0,1,0,0,0,0,0,0},
    {0,1,0,0,0,1,0,0,0,0,0},
    {0,0,0,0,0,0,1,0,0,0,0},
    {0,0,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,0,1,0,0},
    {0,0,0,0,0,0,0,0,0,1,0},
    {0,0,0,0,0,0,0,0,0,0,1},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0},
};
static void draw_cursor(int x, int y) {
    unsigned char fg = (THEME_BG == WHITE) ? BLACK : WHITE;
    for (int ry = 0; ry < 18; ry++) {
        const unsigned char *row = cursor_shape[ry];
        for (int rx = 0; rx < 11; rx++) {
            if (row[rx]) gfx_putpixel(x + rx, y + ry, fg);
        }
    }
}
static int str_eq_local(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}

void shell_run_game(const char *name) {
    if      (str_eq_local(name, "snake"))   { snake_init();   current = SCR_SNAKE;   }
    else if (str_eq_local(name, "talons"))  { talons_init();  current = SCR_TALONS;  }
    else if (str_eq_local(name, "castle"))  { raycast_init(); current = SCR_RAYCAST; }
    else if (str_eq_local(name, "demo3d"))  { demo3d_init();  current = SCR_DEMO3D;  }
}
static void draw_welcome(void) {
    gfx_clear(THEME_BG);
    gfx_rect(0, 0, 640, 32, THEME_BAR);
    gfx_puts(8, 8, "NwOS 2.0.2  |  Welcome", THEME_BAR_FG, THEME_BAR);

    gfx_puts(120, 40, "=== WELCOME TO NwOS ===", YELLOW, THEME_BG);
    gfx_puts(190, 60, "Pick an option below", THEME_FG, THEME_BG);

    for (int i = 0; i < WELCOME_N; i++) {
        int y = WELCOME_BY + i * (WELCOME_BH + WELCOME_BGAP);
        int sel = (i == welcome_sel);
        unsigned char bg = sel ? THEME_SEL : THEME_BTN;
        unsigned char fg = sel ? THEME_SEL_FG : THEME_BTN_FG;

        gfx_rect(WELCOME_BX, y, WELCOME_BW, WELCOME_BH, THEME_FG);
        gfx_rect(WELCOME_BX + 2, y + 2, WELCOME_BW - 4, WELCOME_BH - 4, bg);
        gfx_puts(WELCOME_BX + 24, y + 16, welcome_items[i], fg, bg);
        if (sel) gfx_puts(WELCOME_BX + 6, y + 16, ">", THEME_FG, THEME_BG);
    }

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "ARROWS/ENTER  |  MOUSE CLICK", THEME_BAR_FG, THEME_BAR);
}

static void draw_menu(void) {
    gfx_clear(THEME_BG);
    gfx_rect(0, 0, 640, 32, THEME_BAR);
    gfx_puts(8, 8, "NwOS  |  Personal Menu", THEME_BAR_FG, THEME_BAR);

    gfx_rect(MENU_MX, MENU_MY, MENU_MW, MENU_MH, THEME_FG);          /* рамка */
    gfx_rect(MENU_MX + 2, MENU_MY + 2, MENU_MW - 4, MENU_MH - 4, THEME_BG);
    gfx_puts(MENU_MX + 20, MENU_MY + 8, "====== PERSONAL MENU ======", YELLOW, THEME_BG);

    for (int i = 0; i < MENU_N; i++) {
        int y = MENU_MY + 40 + i * MENU_ROW_H;
        int sel = (i == menu_sel);
        unsigned char fg = sel ? THEME_SEL_FG : THEME_FG;
        unsigned char bg = sel ? THEME_SEL : THEME_BG;
        if (sel) gfx_rect(MENU_MX + 16, y - 2, 440, 24, bg);
        gfx_puts(MENU_MX + 24, y, menu_items[i], fg, bg);
        if (sel) gfx_puts(MENU_MX + 8, y, ">", THEME_FG, THEME_BG);
    }

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "ARROWS/ENTER  |  ESC = SHELL", THEME_BAR_FG, THEME_BAR);
}

static void draw_games(void) {
    gfx_rect(GAMES_MX, GAMES_MY, GAMES_MW, GAMES_MH, THEME_FG);
    gfx_rect(GAMES_MX + 2, GAMES_MY + 2, GAMES_MW - 4, GAMES_MH - 4, THEME_BG);
    gfx_puts(GAMES_MX + 20, GAMES_MY + 8, "======== GAMES ========", YELLOW, THEME_BG);

    for (int i = 0; i < GAMES_N; i++) {
        int y = GAMES_MY + 40 + i * GAMES_ROW_H;
        int sel = (i == games_sel);
        unsigned char fg = sel ? THEME_SEL_FG : THEME_FG;
        unsigned char bg = sel ? THEME_SEL : THEME_BG;
        if (sel) gfx_rect(GAMES_MX + 16, y - 2, 440, 32, bg);
        gfx_puts(GAMES_MX + 24, y, games_items[i], fg, bg);
        if (sel) gfx_puts(GAMES_MX + 8, y, ">", THEME_FG, THEME_BG);
    }

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "ARROWS/ENTER  |  ESC = WELCOME", THEME_BAR_FG, THEME_BAR);
}

static void int_to_str(int v, char *out) {
    int k = 0;
    unsigned int u;
    if (v < 0) { out[k++] = '-'; u = (unsigned int)(-v); }
    else       { u = (unsigned int)v; }
    char tmp[16]; int tl = 0;
    if (u == 0) tmp[tl++] = '0';
    while (u) { tmp[tl++] = '0' + (u % 10); u /= 10; }
    while (tl) out[k++] = tmp[--tl];
    out[k] = 0;
}

static int parse_calc(const char *s) {
    int a = 0, b = 0, i = 0, neg_a = 0, neg_b = 0;
    while (s[i] == ' ') i++;
    if (s[i] == '-') { neg_a = 1; i++; }
    while (s[i] >= '0' && s[i] <= '9') a = a * 10 + (s[i++] - '0');
    if (neg_a) a = -a;
    while (s[i] == ' ') i++;
    char op = s[i++];
    while (s[i] == ' ') i++;
    if (s[i] == '-') { neg_b = 1; i++; }
    while (s[i] >= '0' && s[i] <= '9') b = b * 10 + (s[i++] - '0');
    if (neg_b) b = -b;
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b ? a / b : 0;
    }
    return 0;
}

static void draw_calc(void) {
    gfx_clear(THEME_BG);

    gfx_puts(60, 60, "Format: A op B   (e.g. 12+7, -5*3, 100/4)", THEME_FG, THEME_BG);
    gfx_puts(60, 80, "Operators: +  -  *  /    ENTER = solve", THEME_FG, THEME_BG);

    gfx_rect(60, 130, 520, 40, THEME_FG);
    gfx_rect(62, 132, 516, 36, THEME_BG);
    gfx_puts(70, 140, ">", LIGHT_GREEN, THEME_BG);
    gfx_puts(86, 140, calc_buf, THEME_FG, THEME_BG);
    gfx_rect(86 + calc_len * 8, 140, 6, 18, LIGHT_GREEN);

    if (calc_result_valid) {
        char out[32];
        int_to_str(calc_result, out);
        gfx_puts(70, 200, "Result:", RED, THEME_BG);
        gfx_puts(150, 200, out, THEME_FG, THEME_BG);
    }

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "TYPE EXPRESSION  |  ENTER = SOLVE  |  ESC = WELCOME", THEME_BAR_FG, THEME_BAR);
}

static void draw_game(void) {
    gfx_clear(BLACK);
    gfx_rect(0, 0, 640, 32, BLUE);
    gfx_puts(8, 8, "NwOS  |  Game", WHITE, BLUE);

    gfx_puts(60, 100, "Running game:", LIGHT_GRAY, BLACK);
    gfx_puts(60, 120, game_name, YELLOW, BLACK);
    gfx_puts(60, 160, "(2D game placeholder - not implemented yet)", DARK_GRAY, BLACK);

    gfx_rect(0, 448, 640, 32, BLUE);
    gfx_puts(8, 456, "ESC = back to shell", WHITE, BLUE);
}
static void redraw(void) {
    gfx_clear(THEME_BG);

    switch (current) {
        case SCR_WELCOME: draw_welcome(); break;
        case SCR_MENU:    draw_menu();    break;
        case SCR_SHELL:   shell_draw();   break;
        case SCR_CALC:    draw_calc();    break;
        case SCR_GAME:    draw_game();    break;
        case SCR_GAMES: draw_games(); break;
        case SCR_SNAKE: snake_draw(); break;
        case SCR_DEMO3D: demo3d_draw();  break;
        case SCR_RAYCAST: raycast_draw(); break;
        case SCR_TALONS: talons_draw(); break;
        case SCR_CHAT: chat_draw(); break;
        case SCR_EDITOR: editor_draw(); break;
    }
    draw_cursor(cursor_x, cursor_y);
    draw_topbar();
    gfx_flip();
}
static void on_click(int mx, int my) {
    if (current == SCR_WELCOME) {
        for (int i = 0; i < WELCOME_N; i++) {
            int y = WELCOME_BY + i * (WELCOME_BH + WELCOME_BGAP);
            if (mx >= WELCOME_BX && mx <= WELCOME_BX + WELCOME_BW &&
                my >= y && my <= y + WELCOME_BH) {
                welcome_sel = i;
                if (i == 0) current = SCR_MENU;
                else if (i == 1) shell_run_calc();
                else if (i == 2) current = SCR_SHELL;
                else if (i == 3) current = SCR_MENU;
                redraw();
                return;
            }
        }
    } else if (current == SCR_MENU) {
        for (int i = 0; i < MENU_N; i++) {
            int y = MENU_MY + 40 + i * MENU_ROW_H;
            if (mx >= MENU_MX + 16 && mx <= MENU_MX + 456 &&
                my >= y - 2 && my <= y + 22) {
                menu_sel = i;
                if (i == 0) current = SCR_SHELL;
                redraw();
                return;
            }
        }
    }
}

static int u2s(unsigned int v, char *out) {
    char tmp[12]; int t = 0;
    if (v == 0) tmp[t++] = '0';
    while (v) { tmp[t++] = '0' + (v % 10); v /= 10; }
    int i = 0;
    while (t) out[i++] = tmp[--t];
    out[i] = 0;
    return i;
}

static void draw_topbar(void) {
    gfx_rect(0, 0, 640, 32, THEME_BAR);

    const char *title = "NwOS 2.0.2";
    switch (current) {
        case SCR_WELCOME: title = "NwOS 2.0.2  |  Welcome";       break;
        case SCR_MENU:    title = "NwOS 2.0.2  |  Personal Menu"; break;
        case SCR_SHELL:   title = "NwOS 2.0.2  |  Shell";         break;
        case SCR_CALC:    title = "NwOS 2.0.2  |  Calculator";    break;
        case SCR_GAME:    title = "NwOS 2.0.2  |  Game";          break;
        case SCR_GAMES:   title = "NwOS 2.0.2  |  Games";         break;
        case SCR_SNAKE:   title = "NwOS 2.0.2  |  Snake";         break;
        case SCR_DEMO3D:  title = "NwOS 2.0.2  |  3D Demo";       break;
        case SCR_RAYCAST: title = "NwOS 2.0.2  |  Castle NwOS";   break;
        case SCR_TALONS:  title = "NwOS 2.0.2  |  Talons";        break;
        case SCR_CHAT:    title = "NwOS 2.0.2  |  Chat";          break;
        case SCR_EDITOR:  title = "NwOS 2.0.2  |  Editor";        break;
    }
    gfx_puts(8, 8, title, THEME_BAR_FG, THEME_BAR);
    char buf[64];
    int i = 0;
    const char *p = "CPU ";
    while (*p) buf[i++] = *p++;
    i += u2s(metrics_cpu_pct(), buf + i);
    buf[i++] = '%'; buf[i++] = ' '; buf[i++] = ' ';
    p = "RAM ";
    while (*p) buf[i++] = *p++;
    i += u2s(metrics_ram_pct(), buf + i);
    buf[i++] = '%'; buf[i++] = ' '; buf[i++] = ' ';
    p = "FPS ";
    while (*p) buf[i++] = *p++;
    i += u2s(g_fps, buf + i);
    buf[i] = 0;

    int tw = i * 8;
    gfx_puts(640 - tw - 8, 8, buf, THEME_BAR_FG, THEME_BAR);
    extern unsigned char g_last_key_debug;
    char o4[16]; int k4 = 0;
    const char *p4 = "K:";
    while (*p4) o4[k4++] = *p4++;
    const char *hex = "0123456789ABCDEF";
    unsigned int kv = g_last_key_debug;
    o4[k4++] = hex[(kv >> 4) & 0xF];
    o4[k4++] = hex[kv & 0xF];
    o4[k4] = 0;
    gfx_puts(360, 8, o4, THEME_BAR_FG, THEME_BAR);
}
void kernel_main(void) {
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    gfx_init();
    keyboard_init();
    mouse_init();
    timer_init();
    metrics_init();
    shell_init();
    fs_init();
    shell_apply_theme();
    redraw();

    mouse_state_t m = {320, 240, 0, 0, 0, 0, 0};
    unsigned int frame_count     = 0;
    unsigned int fps_start_ms    = timer_ms();
    unsigned int metric_last_ms  = timer_ms();

    while (1) {
        timer_poll();    
        int dirty = 0;
        if (current == SCR_DEMO3D) {
            if (demo3d_update()) dirty = 1;
        }
        if (current == SCR_SNAKE) {
            if (snake_update()) dirty = 1;
        }
        if (current == SCR_TALONS) {
            if (talons_update()) dirty = 1;
        }
        if (current == SCR_RAYCAST) {
            if (raycast_update()) dirty = 1;
        }
        metrics_frame_begin();

        mouse_update(&m);
        if (m.x != cursor_x || m.y != cursor_y) {
            cursor_x = m.x;
            cursor_y = m.y;
            dirty = 1;
        }
        if (m.wheel != 0) {
            int dir = (m.wheel > 0) ? 1 : -1;
            if (current == SCR_SHELL) { shell_scroll(dir); dirty = 1; }
            if (current == SCR_CHAT)  { chat_scroll(dir);  dirty = 1; }
            m.wheel = 0;
        }
        if (m.left_pressed) {
            on_click(m.x, m.y);
        }

        if (keyboard_has_input()) {
            char c = keyboard_getchar();
            unsigned char u = (unsigned char)c;
            g_last_key = u;

            if (u == 0) {
            }
            else if (current == SCR_EDITOR) {
                editor_handle_key(c);
                if (editor_want_quit()) {
                    editor_clear_quit();
                    current = SCR_WELCOME;
                }
                dirty = 1;
            }
            else if (current == SCR_CHAT) {
                chat_handle_key(c);
                dirty = 1;
            }
            else if (current == SCR_SHELL) {
                shell_handle_key(c);
                dirty = 1;
            }
            else if (u == KEY_ESC) {
                if (current == SCR_MENU)         current = SCR_SHELL;
                else if (current == SCR_GAME)    current = SCR_SHELL;
                else if (current == SCR_SNAKE)   current = SCR_GAMES;
                else if (current == SCR_GAMES)   current = SCR_WELCOME;
                else if (current == SCR_TALONS)  current = SCR_GAMES;
                else if (current == SCR_RAYCAST) current = SCR_GAMES;
                else if (current == SCR_DEMO3D)  current = SCR_GAMES;
                else                             current = SCR_WELCOME;
                dirty = 1;
            }
            else {
                switch (current) {
                    case SCR_WELCOME:
                        if (u == KEY_UP && welcome_sel > 0)             { welcome_sel--; dirty = 1; }
                        if (u == KEY_DOWN && welcome_sel < WELCOME_N-1) { welcome_sel++; dirty = 1; }
                        if (u == KEY_ENTER) {
                            switch (welcome_sel) {
                                case 0: games_sel = 0; current = SCR_GAMES; break;
                                case 1: shell_run_editor("readme.txt");     break;
                                case 2: current = SCR_SHELL;                break;
                                case 3: shell_run_chat();                   break;
                                case 4: gfx_clear(BLACK); gfx_flip();       break;
                            }
                            dirty = 1;
                        }
                        break;

                    case SCR_MENU:
                        if (u == KEY_UP && menu_sel > 0)           { menu_sel--; dirty = 1; }
                        if (u == KEY_DOWN && menu_sel < MENU_N - 1) { menu_sel++; dirty = 1; }
                        if (u == KEY_ENTER && menu_sel == 0)       { current = SCR_SHELL; dirty = 1; }
                        break;

                    case SCR_GAMES:
                        if (u == KEY_UP   && games_sel > 0)         { games_sel--; dirty = 1; }
                        if (u == KEY_DOWN && games_sel < GAMES_N-1) { games_sel++; dirty = 1; }
                        if (u == KEY_ENTER) {
                            if      (games_sel == 0) { snake_init();   current = SCR_SNAKE;   }
                            else if (games_sel == 1) { demo3d_init();  current = SCR_DEMO3D;  }
                            else if (games_sel == 2) { raycast_init(); current = SCR_RAYCAST; }
                            else if (games_sel == 3) { talons_init();  current = SCR_TALONS;  }
                            else if (games_sel == 4) { current = SCR_WELCOME; }
                            dirty = 1;
                        }
                        break;

                    case SCR_CALC:
                        if (u == KEY_ENTER) {
                            calc_result = parse_calc(calc_buf);
                            calc_result_valid = 1;
                            calc_len = 0; calc_buf[0] = 0;
                            dirty = 1;
                        } else if (u == KEY_BACKSPACE) {
                            if (calc_len > 0) calc_buf[--calc_len] = 0;
                            dirty = 1;
                        } else if (u >= 0x20 && u < 0x7F && calc_len < 31) {
                            calc_buf[calc_len++] = c;
                            calc_buf[calc_len] = 0;
                            dirty = 1;
                        }
                        break;

                    case SCR_SNAKE:
                        if (u == ' ') {
                            if (snake_is_over()) { snake_init(); dirty = 1; }
                        } else if (u == KEY_UP || u == KEY_DOWN || u == KEY_LEFT || u == KEY_RIGHT) {
                            snake_handle_key(u);
                        }
                        break;

                    case SCR_TALONS:
                        if (u == KEY_UP || u == KEY_DOWN || u == KEY_LEFT || u == KEY_RIGHT ||
                            u == 'w' || u == 'W' || u == 's' || u == 'S' ||
                            u == 'f' || u == 'F') {
                            talons_handle_key(u);
                        }
                        break;

                    case SCR_RAYCAST:
                        if (u == 'w' || u == 'W' || u == 's' || u == 'S' ||
                            u == 'a' || u == 'A' || u == 'd' || u == 'D' ||
                            u == ' ' ||
                            u == KEY_UP || u == KEY_DOWN || u == KEY_LEFT || u == KEY_RIGHT) {
                            raycast_handle_key(u);
                            dirty = 1;
                        }
                        break;

                    case SCR_DEMO3D:
                        break;

                    case SCR_GAME:
                    case SCR_CHAT:
                    case SCR_EDITOR:
                    case SCR_SHELL:
                        break;
                }
            }
        }

        if (current == SCR_SNAKE) {
            if (snake_update()) dirty = 1;
        }

        metrics_frame_end();
        frame_count++;

        unsigned int now_ms = timer_ms();
        if (now_ms - fps_start_ms >= 1000) {
            g_fps = frame_count;
            frame_count = 0;
            fps_start_ms = now_ms;
            dirty = 1;
        }
        if (now_ms - metric_last_ms >= 250) {
            metric_last_ms = now_ms;
            dirty = 1;
        }

        if (dirty) {
            redraw();
            frame_count++;
        }
    }
}