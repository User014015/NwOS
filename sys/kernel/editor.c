#include "editor.h"
#include "graphics.h"
#include "keyboard.h"
#include "fs.h"

#define EDITOR_MAX_SIZE 4096
#define VIS_LINES 24
#define VIS_COLS  78
#define CHAR_W 8
#define CHAR_H 16
#define EDIT_TOP  40
#define EDIT_LEFT 8

#if KEY_ESC != 0x1B
#error "KEY_ESC is not 0x1B in editor.c!"
#endif

static char buffer[EDITOR_MAX_SIZE];
static int  buf_len;
static int  cursor;
static int  scroll_line;
static char current_file[FS_NAME_LEN];
static int  modified;
static int  want_exit;
static int  status_timer;

static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static int cursor_line(void) {
    int line = 0;
    for (int i = 0; i < cursor; i++) if (buffer[i] == '\n') line++;
    return line;
}
static int cursor_col(void) {
    int col = 0;
    for (int i = cursor - 1; i >= 0 && buffer[i] != '\n'; i--) col++;
    return col;
}
static int line_start(int ln) {
    int line = 0, i = 0;
    while (i < buf_len && line < ln) {
        if (buffer[i] == '\n') line++;
        i++;
    }
    return i;
}
static int line_length(int ln) {
    int s = line_start(ln), e = s;
    while (e < buf_len && buffer[e] != '\n') e++;
    return e - s;
}
static int total_lines(void) {
    int n = 1;
    for (int i = 0; i < buf_len; i++) if (buffer[i] == '\n') n++;
    return n;
}

static void editor_save(void) {
    if (!modified) return;
    fs_write(current_file, buffer, buf_len);
    modified = 0;
    status_timer = 60;
}

void editor_open(const char *filename) {
    extern int g_buf_at_open;
    g_buf_at_open = -999;
    extern int g_open_count;
    g_open_count++;
    str_copy(current_file, filename, FS_NAME_LEN);
    int n = fs_read(current_file, buffer, EDITOR_MAX_SIZE - 1);
    if (n < 0) { buf_len = 0; buffer[0] = 0; }
    else       { buf_len = n; }
    cursor = 0;
    scroll_line = 0;
    modified = 0;
    want_exit = 0;
    status_timer = 0;
    g_buf_at_open = buf_len;
}

const char *editor_filename(void) { return current_file; }
int  editor_want_quit(void)       { return want_exit; }
int editor_want_quit_debug(void) { return want_exit; }
void editor_clear_quit(void)      { want_exit = 0; }

void editor_handle_key(unsigned char c) {
    extern int g_buf_before_ek, g_buf_after_ek, g_ekh_count;
    extern unsigned char g_last_key_debug;
    g_buf_before_ek = buf_len;
    g_ekh_count++;

    unsigned char u = (unsigned char)c;
    g_last_key_debug = u;

    if (cursor < 0)       cursor = 0;
    if (cursor > buf_len) cursor = buf_len;

    if (u == KEY_ESC) {
        editor_save();
        want_exit = 1;
        g_buf_after_ek = buf_len;
        return;
    }

    int cur_line = cursor_line();

    if (u == KEY_LEFT)  { if (cursor > 0) cursor--; g_buf_after_ek = buf_len; return; }
    if (u == KEY_RIGHT) { if (cursor < buf_len) cursor++; g_buf_after_ek = buf_len; return; }

    if (u == KEY_UP) {
        if (cur_line > 0) {
            int col = cursor_col();
            int ps  = line_start(cur_line - 1);
            int pl  = line_length(cur_line - 1);
            if (col > pl) col = pl;
            cursor = ps + col;
        }
        g_buf_after_ek = buf_len; return;
    }
    if (u == KEY_DOWN) {
        int total = total_lines();
        if (cur_line < total - 1) {
            int col = cursor_col();
            int ns  = line_start(cur_line + 1);
            int nl  = line_length(cur_line + 1);
            if (col > nl) col = nl;
            cursor = ns + col;
        }
        g_buf_after_ek = buf_len; return;
    }
    if (u == KEY_HOME) { cursor = line_start(cur_line); g_buf_after_ek = buf_len; return; }
    if (u == KEY_END)  { cursor = line_start(cur_line) + line_length(cur_line); g_buf_after_ek = buf_len; return; }

    if (u == KEY_PGUP) {
        int nl = cur_line - VIS_LINES;
        if (nl < 0) nl = 0;
        cursor = line_start(nl);
        g_buf_after_ek = buf_len; return;
    }
    if (u == KEY_PGDN) {
        int total = total_lines();
        int nl = cur_line + VIS_LINES;
        if (nl >= total) nl = total - 1;
        cursor = line_start(nl);
        g_buf_after_ek = buf_len; return;
    }

    if (u == KEY_BACKSPACE) {
        if (cursor > 0) {
            for (int i = cursor - 1; i < buf_len - 1; i++) buffer[i] = buffer[i + 1];
            buf_len--; cursor--; modified = 1;
        }
        g_buf_after_ek = buf_len; return;
    }
    if (u == KEY_DELETE) {
        if (cursor < buf_len) {
            for (int i = cursor; i < buf_len - 1; i++) buffer[i] = buffer[i + 1];
            buf_len--; modified = 1;
        }
        g_buf_after_ek = buf_len; return;
    }

    if (u == KEY_ENTER) {
        if (buf_len < EDITOR_MAX_SIZE - 1) {
            for (int i = buf_len; i > cursor; i--) buffer[i] = buffer[i - 1];
            buffer[cursor] = '\n';
            buf_len++; cursor++; modified = 1;
        }
        g_buf_after_ek = buf_len; return;
    }

    if (u == KEY_TAB) {
        for (int k = 0; k < 4 && buf_len < EDITOR_MAX_SIZE - 1; k++) {
            for (int i = buf_len; i > cursor; i--) buffer[i] = buffer[i - 1];
            buffer[cursor] = ' ';
            buf_len++; cursor++;
        }
        modified = 1;
        g_buf_after_ek = buf_len; return;
    }

    if (u >= 0x20 && u < 0x7F) {
        if (buf_len < EDITOR_MAX_SIZE - 1) {
            for (int i = buf_len; i > cursor; i--) buffer[i] = buffer[i - 1];
            buffer[cursor] = c;
            buf_len++; cursor++; modified = 1;
        }
    }
    g_buf_after_ek = buf_len;
}

static void draw_line_num(int y, int n) {
    char b[8]; int k = 0;
    if (n == 0) b[k++] = '0';
    while (n > 0) { b[k++] = '0' + (n % 10); n /= 10; }
    while (k < 4) b[k++] = ' ';
    while (k) gfx_putchar(EDIT_LEFT, y, b[--k], LIGHT_GRAY, THEME_BG);
}

void editor_draw(void) {
    extern int g_dbg_buflen;
    g_dbg_buflen = buf_len;
    gfx_clear(THEME_BG);
    gfx_rect(0, 0, 640, 32, THEME_BAR);
    const char *title = "NwOS 2.0.0  |  Editor";
    gfx_puts(8, 8, title, THEME_BAR_FG, THEME_BAR);
    char fn[64];
    int i = 0;
    const char *pre = "file: ";
    while (*pre) fn[i++] = *pre++;
    const char *name = current_file;
    while (*name && i < 60) fn[i++] = *name++;
    if (modified) { fn[i++] = ' '; fn[i++] = '*'; }
    fn[i] = 0;
    gfx_puts(300, 8, fn, THEME_BAR_FG, THEME_BAR);
    int cur_line = cursor_line();
    if (cur_line < scroll_line) scroll_line = cur_line;
    if (cur_line >= scroll_line + VIS_LINES) scroll_line = cur_line - VIS_LINES + 1;
    if (scroll_line < 0) scroll_line = 0;
    int total = total_lines();
    for (int i = 0; i < VIS_LINES; i++) {
        int ln = scroll_line + i;
        int y = EDIT_TOP + i * CHAR_H;
        if (ln >= total) break;
        draw_line_num(y, ln + 1);

        int s = line_start(ln);
        int l = line_length(ln);
        for (int c = 0; c < l && c < VIS_COLS; c++) {
            gfx_putchar(EDIT_LEFT + 40 + c * CHAR_W, y, buffer[s + c], THEME_FG, THEME_BG);
        }
    }
    int vy = cur_line - scroll_line;
    if (vy >= 0 && vy < VIS_LINES) {
        int col = cursor_col();
        int cx = EDIT_LEFT + 40 + col * CHAR_W;
        int cy = EDIT_TOP + vy * CHAR_H;
        if (col < VIS_COLS) {
            unsigned char cc = (cursor < buf_len && buffer[cursor] != '\n') ? LIGHT_GRAY : LIGHT_GREEN;
            gfx_rect(cx, cy, CHAR_W, CHAR_H, cc);
            if (cursor < buf_len && buffer[cursor] != '\n')
                gfx_putchar(cx, cy, buffer[cursor], BLACK, LIGHT_GRAY);
        }
    }
    gfx_rect(0, 440, 640, 16, THEME_BAR);
    char sb[48]; int k = 0;
    const char *p = "Ln ";
    while (*p) sb[k++] = *p++;
    int cl = cur_line + 1;
    if (cl > 99) cl = 99;
    sb[k++] = '0' + (cl / 10); sb[k++] = '0' + (cl % 10);
    sb[k++] = ' ';
    p = "Col ";
    while (*p) sb[k++] = *p++;
    int cc2 = cursor_col() + 1;
    if (cc2 > 99) cc2 = 99;
    sb[k++] = '0' + (cc2 / 10); sb[k++] = '0' + (cc2 % 10);
    sb[k++] = ' ';
    sb[k++] = ' ';
    if (status_timer > 0) {
        p = "SAVED";
        while (*p) sb[k++] = *p++;
        status_timer--;
    }
    sb[k] = 0;
    gfx_puts(8, 444, sb, THEME_BAR_FG, THEME_BAR);
    gfx_rect(0, 456, 640, 24, THEME_BAR);
    gfx_puts(8, 460, "Arrows | Tab | Enter | Backspace | Del | ESC = save+exit",
             THEME_BAR_FG, THEME_BAR);
}