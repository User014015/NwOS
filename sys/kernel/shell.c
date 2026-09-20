#include "shell.h"
#include "graphics.h"
#include "keyboard.h"

#define BUF_MAX 128
#define LINE_MAX 22

static char buf[BUF_MAX];
static int  len = 0;
static char lines[LINE_MAX][BUF_MAX];
static int  line_count = 0;
extern unsigned char THEME_BG, THEME_FG, THEME_BAR, THEME_BAR_FG;
extern unsigned char THEME_BTN, THEME_BTN_FG, THEME_SEL, THEME_SEL_FG;

static void push_line(const char *s) {
    if (line_count == LINE_MAX) {
        for (int i = 1; i < LINE_MAX; i++)
            for (int j = 0; j < BUF_MAX; j++) lines[i-1][j] = lines[i][j];
        line_count--;
    }
    int i = 0;
    while (s[i] && i < BUF_MAX - 1) { lines[line_count][i] = s[i]; i++; }
    lines[line_count][i] = 0;
    line_count++;
}

static int starts_with(const char *s, const char *p) {
    while (*p) { if (*s++ != *p++) return 0; }
    return 1;
}
static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}

static void cmd_help(void) {
    push_line("Commands:");
    push_line("  help                 - this help");
    push_line("  clear                - clear screen");
    push_line("  echo <text>          - print text");
    push_line("  about                - about MyOS");
    push_line("  sys/menu             - open Personal Menu");
    push_line("  sys/welcome          - back to Welcome");
    push_line("  calc                 - calculator");
    push_line("  games2D(\"name\")      - run 2D game");
    push_line("  help2d               - 2D games list");
    push_line("  help3d               - 3D games info");
    push_line("  theme light/dark      - switch theme");
    push_line("  snake                - play Snake");
    push_line("  run <game>           - Run game (3D)");
}

static void cmd_about(void) {
    push_line("NwOS v2.0.0");
    push_line("Copyright (c) 2026 User014015"); // mit
    push_line("Kernel: C + NASM, clang + ld.lld");
    push_line("VGA 640x480x16, PS/2 keyboard + mouse");
}

static void cmd_help2d(void) {
    push_line("2D Games available:");
    push_line("1.  snake");
    push_line("2.  tetris");
    push_line("3.  pong");
    push_line("Run: games2D(\"snake\")");
}

static void cmd_help3d(void) {
    push_line("1.  3D Demo");
    push_line("2.  Castle");
    push_line("3.  Talons");
    push_line("Run:");
}
extern void shell_apply_theme(void);

extern unsigned char THEME_BG, THEME_FG, THEME_BAR, THEME_BAR_FG;
extern unsigned char THEME_BTN, THEME_BTN_FG, THEME_SEL, THEME_SEL_FG;
extern void shell_apply_theme(void);

static int current_theme = 1;

static void set_theme_light(void) {
    THEME_BG       = WHITE;
    THEME_FG       = BLACK;
    THEME_BAR      = BLUE;
    THEME_BAR_FG   = WHITE;
    THEME_BTN      = LIGHT_GRAY;
    THEME_BTN_FG   = BLACK;
    THEME_SEL      = LIGHT_CYAN;
    THEME_SEL_FG   = BLACK;
    current_theme  = 1;
}

static void set_theme_dark(void) {
    THEME_BG       = BLACK;
    THEME_FG       = WHITE;
    THEME_BAR      = BLUE;
    THEME_BAR_FG   = WHITE;
    THEME_BTN      = DARK_GRAY;
    THEME_BTN_FG   = WHITE;
    THEME_SEL      = LIGHT_CYAN;
    THEME_SEL_FG   = BLACK;
    current_theme  = 0;
}

static void try_theme(const char *s) {
    s += 6;
    while (*s == ' ') s++;
    if (str_eq(s, "light") || str_eq(s, "white")) {
        set_theme_light();
        push_line("Theme: light");
    } else if (str_eq(s, "dark") || str_eq(s, "black")) {
        set_theme_dark();
        push_line("Theme: dark");
    } else {
        push_line("Usage: theme light | theme dark");
        return;
    }
    shell_apply_theme();
}
static void try_games2d(const char *s) {
    const char *p = "games2D(";
    if (!starts_with(s, p)) { push_line("Usage: games2D(\"snake\")"); return; }
    s += 8;
    if (*s != '"') { push_line("Usage: games2D(\"snake\")"); return; }
    s++;
    char name[32]; int i = 0;
    while (*s && *s != '"' && i < 31) name[i++] = *s++;
    name[i] = 0;
    if (*s != '"') { push_line("Missing closing quote"); return; }

    char msg[BUF_MAX];
    int k = 0;
    const char *pre = "Running 2D game: ";
    while (*pre) msg[k++] = *pre++;
    for (int j = 0; name[j] && k < BUF_MAX - 1; j++) msg[k++] = name[j];
    msg[k] = 0;
    push_line(msg);

    shell_run_game2d(name);
}
static int try_run(const char *s) {
    s += 4;
    while (*s == ' ') s++;
    if (*s == 0) { push_line("Usage: run snake|talons|castle|demo3d"); return 1; }

    if (str_eq(s, "snake")  || str_eq(s, "talons") ||
        str_eq(s, "castle") || str_eq(s, "demo3d")) {
        char msg[BUF_MAX];
        int k = 0;
        const char *pre = "Running: ";
        while (*pre) msg[k++] = *pre++;
        for (int j = 0; s[j] && k < BUF_MAX - 1; j++) msg[k++] = s[j];
        msg[k] = 0;
        push_line(msg);
        shell_run_game(s);
        return 1;
    }
    push_line("Unknown game. Try: run snake|talons|castle|demo3d");
    return 1;
}
static void try_echo(const char *s) {
    s += 5;
    while (*s == ' ') s++;
    push_line(s);
}

static void run_command(void) {
    char cmd[BUF_MAX];
    int i = 0;
    while (i < len && i < BUF_MAX - 1) { cmd[i] = buf[i]; i++; }
    cmd[i] = 0;
    char echo[BUF_MAX];
    echo[0] = '>'; echo[1] = ' ';
    for (int j = 0; cmd[j] && j < BUF_MAX - 3; j++) echo[j+2] = cmd[j];
    echo[len + 2] = 0;
    push_line(echo);

    if (cmd[0] == 0) { /* no */ }
    else if (str_eq(cmd, "help"))         cmd_help();
    else if (str_eq(cmd, "clear"))        { line_count = 0; }
    else if (str_eq(cmd, "about"))        cmd_about();
    else if (str_eq(cmd, "sys/menu"))     shell_goto_menu();
    else if (str_eq(cmd, "sys/welcome"))  shell_goto_welcome();
    else if (str_eq(cmd, "calc"))         shell_run_calc();
    else if (str_eq(cmd, "help2d"))       cmd_help2d();
    else if (str_eq(cmd, "help3d"))       cmd_help3d();
    else if (starts_with(cmd, "echo "))   try_echo(cmd);
    else if (str_eq(cmd, "echo"))         push_line("");
    else if (starts_with(cmd, "games2D(")) try_games2d(cmd);
    else if (starts_with(cmd, "theme "))  try_theme(cmd);
    else if (str_eq(cmd, "snake"))          shell_run_game2d("snake");
    else if (str_eq(cmd, "theme"))        push_line("Usage: theme light | theme dark");
    else if (starts_with(cmd, "run "))    try_run(cmd);
    else if (str_eq(cmd, "run"))          push_line("Usage: run snake|talons|castle|demo3d");
    else if (str_eq(cmd, "talons"))       { push_line("Running: talons");  shell_run_game("talons"); }
    else if (str_eq(cmd, "castle"))       { push_line("Running: castle");  shell_run_game("castle"); }
    else if (str_eq(cmd, "demo3d"))       { push_line("Running: demo3d");  shell_run_game("demo3d"); }
    else push_line("Unknown. Type 'help'.");

    len = 0; buf[0] = 0;
}

void shell_init(void) {
    set_theme_light();
    len = 0; buf[0] = 0;
    line_count = 0;
    push_line("NwOS Shell v2.0.0");
    push_line("Copyright (c) 2026 User014015");
    push_line("Type 'help' for commands.");
    push_line("");
}

void shell_draw(void) {
    gfx_clear(THEME_BG);
    gfx_rect(0, 0, 640, 32, THEME_BAR);
    gfx_puts(8, 8, "NwOS 2.0.0  |  Shell", THEME_BAR_FG, THEME_BAR);

    int y = 40;
    int first = (line_count > LINE_MAX - 4) ? line_count - (LINE_MAX - 4) : 0;
    for (int i = first; i < line_count; i++) {
        gfx_puts(8, y, lines[i], THEME_FG, THEME_BG);
        y += 16;
    }

    int cy = 40 + (LINE_MAX - 3) * 16;
    gfx_puts(8, cy, ">", LIGHT_GREEN, THEME_BG);
    gfx_puts(24, cy, buf, THEME_FG, THEME_BG);
    gfx_rect(24 + len * 8, cy, 6, 18, LIGHT_GRAY);

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "Type commands. ESC = Welcome.", THEME_BAR_FG, THEME_BAR);
}

void shell_handle_key(char c) {
    unsigned char u = (unsigned char)c;
    if (u == KEY_ENTER) {
        run_command();
    } else if (u == KEY_BACKSPACE) {
        if (len > 0) buf[--len] = 0;
    } else if (u >= 0x20 && u < 0x7F && len < BUF_MAX - 1) {
        buf[len++] = c;
        buf[len] = 0;
    }
}