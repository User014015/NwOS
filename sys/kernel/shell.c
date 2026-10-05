#include "fs.h"
#include "shell.h"
#include "graphics.h"
#include "keyboard.h"
#include "panic.h"
#include "delay.h"
#include "time.h"
static int shell_ask(const char *question);

#define BUF_MAX 128
#define LINE_MAX 22
#define MAX_SHELL 50

static char buf[BUF_MAX];
static char buf_shell[MAX_SHELL][BUF_MAX];
static int  len = 0;
static char lines[LINE_MAX][BUF_MAX];
static int  line_count = 0;
static int scroll_offset = 0;

static int len_cursor[MAX_SHELL];
static int ptr_shell = 0;
static int ptr_cursor = 0;

static int shell_force = 0;

// sudo
static int shell_ask(const char *question) {
    if (shell_force) {
        push_line("[sudo] confirmation skipped");
        return 1;
    }

    push_line("");
    push_line(question);
    push_line("Type 'y' + Enter to confirm, anything else cancels.");

    shell_draw();
    gfx_flip();

    char answer[8];
    int n = 0;

    while (1) {
        char c = keyboard_getchar();
        if (c == 0) continue;
        if (c == '\n') { answer[n] = 0; break; }
        if (c == '\b') { if (n > 0) n--; continue; }
        if (n < 7 && c >= 0x20 && c < 0x7F) answer[n++] = c;
    }

    char echo[16];
    int e = 0;
    echo[e++] = '>';
    echo[e++] = ' ';
    for (int i = 0; i < n && e < 14; i++) echo[e++] = answer[i];
    echo[e] = 0;
    push_line(echo);

    return (n == 1 && (answer[0] == 'y' || answer[0] == 'Y'));
}

static void wait() {
    for (volatile unsigned long i = 0; i < 25000000UL; i++)
        __asm__ volatile ("pause");
}

extern unsigned char THEME_BG, THEME_FG, THEME_BAR, THEME_BAR_FG;
extern unsigned char THEME_BTN, THEME_BTN_FG, THEME_SEL, THEME_SEL_FG;
void kernel_panic_root_deleted(void);
static void died_delete(void);

static void push_line(const char *s);
static const char *parse_arg(const char *s, const char *cmd);
static void copy_arg(const char *s, char *out, int max);
static int starts_with(const char *s, const char *p);

extern void kernel_panic_safe(const char *msg);
extern void kernel_panic_fatal(const char *msg);
static void sys_error_delete(void);

int debug = 0; // 0 = false, 1 = true

static int atoi(const char *s) {
    int sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return v * sign;
}

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

static const char *parse_arg(const char *s, const char *cmd) {
    int cl = 0; while (cmd[cl]) cl++;
    s += cl;
    while (*s == ' ') s++;
    if (*s == '(') s++;
    if (*s == '"') s++;
    return s;
}

static int line_completed = 1;

static void print_char(char c) {
    if (c == '\n') {
        line_completed = 1;
        return;
    }

    if (line_completed || line_count == 0) {
        push_line("");
        line_completed = 0;
    }

    int current_idx = line_count - 1;
    int cur_len = 0;
    while (lines[current_idx][cur_len]) {
        cur_len++;
    }

    if (cur_len >= BUF_MAX - 1) {
        push_line("");
        current_idx = line_count - 1;
        cur_len = 0;
    }

    lines[current_idx][cur_len] = c;
    lines[current_idx][cur_len + 1] = 0;
}

static void print(const char *textt) {
    while (*textt) {
        print_char(*textt);
        textt++;
    }
}

static void copy_arg(const char *s, char *out, int max) {
    while (*s == ' ') s++;
    if (*s == '"') s++;
    int i = 0;
    while (*s && *s != '"' && *s != ')' && i < max - 1)
        out[i++] = *s++;
    out[i] = 0;
}

static void cmd_dir(const char *path) {
    if (!path || !*path) path = fs_pwd();
    int n = fs_count(path);
    if (n < 0) { push_line("Not a directory"); return; }
    if (n == 0) { push_line("(empty)"); return; }
    for (int i = 0; i < n; i++) {
        const char *nm = fs_name_at(path, i);
        int t  = fs_type_at(path, i);
        int sz = fs_size_at(path, i);
        char b[BUF_MAX]; int k = 0;
        while (*nm && k < BUF_MAX - 16) b[k++] = *nm++;
        if (t == FS_DIR) {
            b[k++] = '/';
        } else {
            while (k < 20) b[k++] = ' ';
            char t2[8]; int tt = 0;
            if (sz == 0) t2[tt++] = '0';
            while (sz) { t2[tt++] = '0' + (sz % 10); sz /= 10; }
            while (tt) b[k++] = t2[--tt];
            b[k++] = ' '; b[k++] = 'B';
        }
        b[k] = 0;
        push_line(b);
    }
}

static void cmd_cd(const char *path) {
    if (fs_cd(path) < 0) push_line("Not found");
    else                 push_line(fs_pwd());
}

static void cmd_mkdir(const char *path) {
    if (fs_mkdir(path) < 0) push_line("Failed");
    else                    push_line("Created");
}

static void cmd_touch(const char *path) {
    if (fs_touch(path) < 0) push_line("Failed");
    else                    push_line("Created");
}

static void cmd_rm(const char *path) {
    // rm /, sudo/no sudo
    if (path[0] == '/' && path[1] == 0) {
        if (!shell_ask("rm: remove ALL files from root filesystem?")) {
            push_line("Cancelled.");
            return;
        }

        push_line("rm: WARNING: system may become unstable");
        push_line("rm: proceeding anyway...");

        int removed = fs_wipe_all();

        char buf2[64]; int b = 0;
        const char *p = "rm: removed ";
        while (*p) buf2[b++] = *p++;
        if (removed == 0) buf2[b++] = '0';
        else {
            char t[12]; int tt = 0;
            while (removed) { t[tt++] = '0' + (removed % 10); removed /= 10; }
            while (tt) buf2[b++] = t[--tt];
        }
        p = " entries";
        while (*p) buf2[b++] = *p++;
        buf2[b] = 0;
        push_line(buf2);

        kernel_panic_root_deleted();
        return;
    }

    /* ─── Обычное удаление ─── */
    int r = fs_delete(path);
    if (r == 0)       push_line("Removed");
    else if (r == -2) push_line("Cannot delete root");
    else if (r == -3) push_line("Directory not empty");
    else              push_line("Not found");
}

static void cmd_pwd(void) {
    push_line(fs_pwd());
}

static void cmd_cat(const char *path) {
    static char fbuf[FS_MAX_SIZE + 1];
    int n = fs_read(path, fbuf, FS_MAX_SIZE);
    if (n < 0) { push_line("File not found"); return; }
    int s = 0;
    for (int i = 0; i <= n; i++) {
        if (i == n || fbuf[i] == '\n') {
            int len = i - s;
            if (len > BUF_MAX - 1) len = BUF_MAX - 1;
            char tmp[BUF_MAX];
            for (int j = 0; j < len; j++) tmp[j] = fbuf[s + j];
            tmp[len] = 0;
            push_line(tmp);
            s = i + 1;
        }
    }
}

static void cmd_cnf_get(const char *args) {
    char path[64], key[32];
    int i = 0, j = 0;
    while (*args == ' ') args++;
    while (*args && *args != ' ' && i < 63) path[i++] = *args++;
    path[i] = 0;
    while (*args == ' ') args++;
    while (*args && *args != ' ' && j < 31) key[j++] = *args++;
    key[j] = 0;

    if (!path[0] || !key[0]) {
        push_line("Usage: cnf get <file> <key>");
        return;
    }

    char value[128];
    if (cnf_get(path, key, value, sizeof(value)) < 0) {
        push_line("Key not found");
        return;
    }

    char out[160];
    int k = 0;
    while (key[k] && k < 60) { out[k] = key[k]; k++; }
    out[k++] = '='; out[k++] = ' ';
    for (int m = 0; value[m] && k < 158; m++) out[k++] = value[m];
    out[k] = 0;
    push_line(out);
}

static void cmd_cnf_set(const char *args) {
    char path[64], key[32], value[128];
    int i = 0, j = 0, k = 0;
    while (*args == ' ') args++;
    while (*args && *args != ' ' && i < 63) path[i++] = *args++;
    path[i] = 0;
    while (*args == ' ') args++;
    while (*args && *args != ' ' && j < 31) key[j++] = *args++;
    key[j] = 0;
    while (*args == ' ') args++;
    while (*args && k < 127) value[k++] = *args++;
    value[k] = 0;

    if (!path[0] || !key[0] || !value[0]) {
        push_line("Usage: cnf set <file> <key> <value>");
        return;
    }

    cnf_set(path, key, value);
    push_line("OK");
}

static void nwfetch(void) {
    print("   ..      "); print("NwOS v2.0.7\n");
    print("  /  \\     "); print("-------------------\n");
    print("  |  |     "); print("Kernel:     C\n");
    print("  |  |     "); print("Video:      VGA\n");
    print("  |  |     "); print("Resolution: 640x480\n");
    print("  ----     "); print("Bootloader: NASM\n");
    print("  |  |     "); print("Arch:       32-bit (x86)\n");
    print("  ---      "); print("Shell:      v2.0.7\n");
}

static void cmd_help(void) {
    push_line("Commands:");
    push_line("  help                 - this help");
    push_line("  clear                - clear screen");
    push_line("  echo <text>          - print text");
    push_line("  about                - about NwOS");
    push_line("  history              - history command");
    push_line("  sys/menu             - open Personal Menu");
    push_line("  sys/welcome          - back to Welcome");
    push_line("  calc                 - calculator");
    push_line("  games2D(\"name\")      - run 2D game");
    push_line("  help2d               - 2D games list");
    push_line("  help3d               - 3D games info");
    push_line("  theme light/dark     - switch theme");
    push_line("  snake                - play Snake");
    push_line("  run <game>           - Run game (3D)");
    push_line("  chat                 - Chat with Computer");
    push_line("  nwfetch              - Neofetch");
    push_line("  more                 - More commands");
}

static void current_ver(void) {
    push_line("\nCurrent version: 2.0.7\n");
}

static void HelpMore2(void) {
    push_line("ver                    - Check version");
}

static void HelpMore(void) {
    push_line("--- MORE COMMANDS ---");
    push_line("  ls                   - list files");
    push_line("  cat <file>           - print file");
    push_line("  edit <file>          - open in editor");
    push_line("  touch <file>         - create empty file");
    push_line("  rm <file>            - delete file");
    push_line("  dir / dir(\"path\")    - list directory");
    push_line("  cd(\"path\")           - change directory");
    push_line("  pwd                  - print current path");
    push_line("  mkdir(\"path\")        - create directory");
    push_line("  asm                  - ASM console");
    push_line("  panic                - safe panic");
    push_line("  panic --fatal        - fatal panic");
    push_line("  timer NUM             - wait for NUM");
    push_line("  more2                 - More commands (2)");
    push_line("  sudo <cmd>           - run command without confirmations");
}

static void cmd_about(void) {
    push_line("NwOS v2.0.7");
    push_line("Copyright (c) 2026 User014015");
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
static int current_theme = 1;

static void set_theme_light(void) {
    THEME_BG       = WHITE;
    THEME_FG       = BLUE;
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

static void died_delete(void) {
    delay_ms(4500); // 1.5 seconds
    push_line("Fatal error: cannot read files.");
    shell_draw();
    delay_ms(3900);

    kernel_panic_fatal("CRITICAL_SYS_FAULT: Cannot read system files!\nError Code: 0x000010026\nModule: VFS_ROOT_WIPE\nReason: Critical system directories cannot be readen\n");
}

static void sys_error_delete(void) {
    delay_ms(5000);
    push_line("Fatal error: Cannot read sys files!\n");
    delay_ms(4500);
    push_line("Fatal error: Cannot find sys/kernel!\n");
    delay_ms(4500);
    push_line("Fatal error: unknow");
    delay_ms(1990);
    kernel_panic_fatal("CRITICAL_SYS_FAULT: Cannot read sys/ files!\nError Code: 0x000013B98\nModule: VFS_SYS_WIPE\nReason: Critical system directory cannot be readen\n");
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

int history(){
    for (int i = 0; i < ptr_shell; i++){
        push_line(buf_shell[i]);
    }
}

static void run_command(void) {
    if (starts_with(buf, "sudo ")) {
        char saved[BUF_MAX];
        int saved_len = len;
        for (int i = 0; i <= len && i < BUF_MAX; i++) saved[i] = buf[i];

        int src = 5;
        int dst = 0;
        while (buf[src] && dst < BUF_MAX - 1) buf[dst++] = buf[src++];
        buf[dst] = 0;
        len = dst;

        int save_force = shell_force;
        shell_force = 1;

        run_command();

        shell_force = save_force;
        for (int i = 0; i <= saved_len && i < BUF_MAX; i++) buf[i] = saved[i];
        len = saved_len;
        return;
    }
    if (str_eq(buf, "sudo")) {
        push_line("Usage: sudo <command>");
        return;
    }

    char cmd[BUF_MAX];
    int i = 0;
    while (i < len && i < BUF_MAX - 1) { cmd[i] = buf[i]; i++; }
    cmd[i] = 0;

    char echo[BUF_MAX];
    echo[0] = '>'; echo[1] = ' ';
    for (int j = 0; cmd[j] && j < BUF_MAX - 3; j++) echo[j+2] = cmd[j];
    echo[len + 2] = 0;
    push_line(echo);

    if (cmd[0] == 0) { /* void */ }

    else if (str_eq(cmd, "help"))         cmd_help();
    else if (str_eq(cmd, "clear"))        { line_count = 0; }
    else if (str_eq(cmd, "about"))        cmd_about();
    else if (str_eq(cmd, "history"))        history();
    else if (str_eq(cmd, "sys/menu"))     shell_goto_menu();
    else if (str_eq(cmd, "sys/welcome"))  shell_goto_welcome();
    else if (str_eq(cmd, "calc"))         shell_run_calc();
    else if (str_eq(cmd, "help2d"))       cmd_help2d();
    else if (str_eq(cmd, "help3d"))       cmd_help3d();
    else if (starts_with(cmd, "echo "))   try_echo(cmd);
    else if (str_eq(cmd, "echo"))         push_line("");
    else if (starts_with(cmd, "games2D(")) try_games2d(cmd);
    else if (starts_with(cmd, "theme "))  try_theme(cmd);
    else if (str_eq(cmd, "snake"))        shell_run_game2d("snake");
    else if (str_eq(cmd, "theme"))        push_line("Usage: theme light | theme dark");
    else if (starts_with(cmd, "run "))    try_run(cmd);
    else if (str_eq(cmd, "run"))          push_line("Usage: run snake|talons|castle|demo3d");
    else if (str_eq(cmd, "talons"))       { push_line("Running: talons");  shell_run_game("talons"); }
    else if (str_eq(cmd, "castle"))       { push_line("Running: castle");  shell_run_game("castle"); }
    else if (str_eq(cmd, "demo3d"))       { push_line("Running: demo3d");  shell_run_game("demo3d"); }
    else if (str_eq(cmd, "chat"))         shell_run_chat();
    else if (str_eq(cmd, "more"))         HelpMore();
    else if (str_eq(cmd, "nwfetch"))      nwfetch();
    else if (str_eq(cmd, "dir") || str_eq(cmd, "ls")) { cmd_dir(fs_pwd()); }
    else if (starts_with(cmd, "dir("))   { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 4, a, sizeof(a)); cmd_dir(a); }
    else if (starts_with(cmd, "cd("))    { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 3, a, sizeof(a)); cmd_cd(a); }
    else if (starts_with(cmd, "cd "))    { cmd_cd(cmd + 3); }
    else if (starts_with(cmd, "mkdir(")) { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 6, a, sizeof(a)); cmd_mkdir(a); }
    else if (starts_with(cmd, "touch(")) { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 6, a, sizeof(a)); cmd_touch(a); }
    else if (starts_with(cmd, "touch ")) { cmd_touch(cmd + 6); }
    else if (str_eq(cmd, "rm sys/") || str_eq(cmd, "rm sys")) { sys_error_delete(); }
    else if (starts_with(cmd, "rm("))    { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 3, a, sizeof(a)); cmd_rm(a); }
    else if (starts_with(cmd, "rm "))    { cmd_rm(cmd + 3); }
    else if (str_eq(cmd, "pwd"))         { cmd_pwd(); }
    else if (starts_with(cmd, "cat("))   { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 4, a, sizeof(a)); cmd_cat(a); }
    else if (starts_with(cmd, "cat "))   { cmd_cat(cmd + 4); }
    else if (starts_with(cmd, "edit("))  { char a[FS_NAME_LEN * 4]; copy_arg(cmd + 5, a, sizeof(a)); shell_run_editor(a); }
    else if (str_eq(cmd, "debug on")) { debug = 1; push_line("Debug: True"); }
    else if (str_eq(cmd, "debug off")) { debug = 0; push_line("Debug: false"); }
    else if (starts_with(cmd, "edit "))  { shell_run_editor(cmd + 5); }
    else if (str_eq(cmd, "asm"))         { push_line("Entering ASM Console..."); shell_run_asmconsole(); return; }
    else if (starts_with(cmd, "timer ")) { int sec = atoi(cmd + 6); time(sec); }
    else if (starts_with(cmd, "cnf get ")) cmd_cnf_get(cmd + 8);
    else if (starts_with(cmd, "cnf set ")) cmd_cnf_set(cmd + 8);
    else if (str_eq(cmd, "ver")) { current_ver(); }
    else if (str_eq(cmd, "more2")) { HelpMore2(); }
    else if (str_eq(cmd, "panic")) {
        if (debug == 1) {
            push_line("Manual calling \"Safe panic\"..");
            kernel_panic_safe("SHELL_FAULT: User executed manual panic command.\nError Code: 0x00004B11\nSystem Status: Normal\nUser Action: Tested Panic Handler");
        } else {
            kernel_panic_safe("SHELL_FAULT: User executed manual panic command.\nError Code: 0x00004B11\nSystem Status: Normal\nUser Action: Tested Panic Handler");
        }
    }
    else if (str_eq(cmd, "panic --fatal") || str_eq(cmd, "panic -f")) {
        if (debug == 1) {
            push_line("Manual calling \"Critical panic\"..");
            delay_ms(6700);
            kernel_panic_fatal("CRITICAL_SHELL_FAULT: User triggered manual fatal panic!\nError Code: 0x000012091\nDamage Level: Severe System Corruption\nReason: Shell override command invocation.");
        } else {
            kernel_panic_fatal("CRITICAL_SHELL_FAULT: User triggered manual fatal panic!\nError Code: 0x000012091\nDamage Level: Severe System Corruption\nReason: Shell override command invocation.");
        }
    }
    else {
        push_line("Unknown. Type 'help'.");
    }

    len = 0; buf[0] = 0;
}

void shell_init(void) {
    set_theme_light();
    len = 0; buf[0] = 0;
    line_count = 0;
    push_line("NwOS Shell v2.0.7");
    push_line("Copyright (c) 2026 User014015");
    push_line("Type 'help' for commands.");
    push_line("");
}

void shell_draw(void) {
    gfx_clear(THEME_BG);

    gfx_rect(0, 0, 640, 32, THEME_BAR);
    gfx_puts(8, 8, "NwOS 2.0.7  |  Shell", THEME_BAR_FG, THEME_BAR);

    int visible = 22;
    int end   = line_count - scroll_offset;
    int start = end - visible;
    if (start < 0) start = 0;

    int y = 40;
    int printed_lines = 0;
    for (int i = start; i < end && i < line_count; i++) {
        gfx_puts(8, y, lines[i], THEME_FG, THEME_BG);
        y += 16;
        printed_lines++;
    }

    int cy = 40 + printed_lines * 16;
    if (cy > 424) cy = 424;

    gfx_puts(8, cy, ">", BLUE, THEME_BG);
    gfx_puts(24, cy, buf, BLUE, THEME_BG); // theme fg
    gfx_rect(24 + len * 8, cy, 6, 18, BLUE); // light gray basic

    gfx_rect(0, 448, 640, 32, THEME_BAR);
    gfx_puts(8, 456, "Type commands. ESC = Welcome.", THEME_BAR_FG, THEME_BAR);
}

void shell_handle_key(char c) {
    unsigned char u = (unsigned char)c;
    if (u == KEY_ENTER) {
        if (buf[0] == 0){
            buf_shell[ptr_shell][0] = 0;
        } else{
            for (int i = 0; i < len; i++){
                buf_shell[ptr_shell][i] = buf[i];
            }
        }
        buf_shell[ptr_shell][len] = 0;
        len_cursor[ptr_shell] = len;
        if (ptr_shell >= MAX_SHELL-1){
            for (int i = 0; i < MAX_SHELL; i++){
                for (int a = 0; a < len_cursor[ptr_cursor]; a++){
                    buf_shell[i][a] = buf_shell[i+1][a];
                }
                len_cursor[i] = len_cursor[i+1];
            }
        }
        ptr_shell++;
        ptr_cursor = ptr_shell;
        scroll_offset = 0;
        run_command();
    } else if (u == KEY_BACKSPACE) {
        if (len > 0) buf[--len] = 0;
    } else if (u == KEY_UP){
        if (ptr_cursor > 0){
            ptr_cursor--;
        }
        for (int i = 0; i < BUF_MAX; i++){
            buf[i] = 0;
        }
        for (int i = 0; i < len_cursor[ptr_cursor]; i++){
            buf[i] = buf_shell[ptr_cursor][i];
        }
        len = len_cursor[ptr_cursor];
    } else if (u == KEY_DOWN){
        if (ptr_cursor < ptr_shell){
            ptr_cursor++;
        }
        for (int i = 0; i < BUF_MAX; i++){
            buf[i] = 0;
        }
        for (int i = 0; i < len_cursor[ptr_cursor]; i++){
            buf[i] = buf_shell[ptr_cursor][i];
        }
        len = len_cursor[ptr_cursor];

    } else if (u >= 0x20 && u < 0x7F && len < BUF_MAX - 1) {
        buf[len++] = c;
        buf[len] = 0;
    }
}

void shell_scroll(int delta) {
    scroll_offset += delta;
    if (scroll_offset < 0) scroll_offset = 0;
    int max = line_count - 1;
    if (max < 0) max = 0;
    if (scroll_offset > max) scroll_offset = max;
}

void kernel_panic_root_deleted(void) {
    push_line("[ OK ] Removing sys/boot.cnf");
    push_line("[ OK ] Removing sys/settings.cnf");
    push_line("[ OK ] Removing sys/theme.cnf");
    push_line("[ OK ] Removing sys/version.cnf");
    push_line("[ OK ] Removing home/readme.nw");

    shell_draw();
    gfx_flip();

    wait();

    kernel_panic_fatal(
        "CRITICAL_SYS_FAULT: Cannot read system files\n"
        "Path: sys/\n"
        "Error code: 0x000010026\n"
        "Damage: Damaged disk, Lost of data\n");
}