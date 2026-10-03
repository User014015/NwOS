#include "chat.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

#define BUF_MAX  128
#define LINE_MAX 40

static char buf[BUF_MAX];
static int  len = 0;
static char lines[LINE_MAX][BUF_MAX];
static int  line_count = 0;
static int  scroll_offset = 0;

static unsigned int rng = 1;
static unsigned int rnd(void) {
    rng = rng * 1103515245u + 12345u;
    return (rng >> 16) & 0x7FFF;
}

static void push_line(const char *s) {
    if (line_count == LINE_MAX) {
        for (int i = 1; i < LINE_MAX; i++) {
            for (int j = 0; j < BUF_MAX; j++) lines[i-1][j] = lines[i][j];
        }
        line_count--;
    }
    int i = 0;
    while (s[i] && i < BUF_MAX - 1) { lines[line_count][i] = s[i]; i++; }
    lines[line_count][i] = 0;
    line_count++;
}

#define ARR_SIZE(x) (int)(sizeof(x)/sizeof(x[0]))

static const char *randwords[] = {
    "Hello", "Bye", "When", "Are", "You",
    "We", "His", "a", "world", "Earth",
    "human", "mars", "hair", "dog",
    "cat", "animal", "cosmic", "computer",
    "god", "today", "void", "function",
    "words", "car", "OS", "humanoid",
    "people", "bad", "virus", "invasion",
    "good", "yes", "i am", "am", "an",
    "china", "random", "were", "country",
    "name", "names", "knife", "cool",
    "cold", "main", "trojan"
};

static void gen_reply(void) {
    int s_idx = rnd() % ARR_SIZE(randwords);
    int s_idx2 = rnd() % ARR_SIZE(randwords);
    int s_idx3 = rnd() % ARR_SIZE(randwords);
    int s_idx4 = rnd() % ARR_SIZE(randwords);
    int s_idx5 = rnd() % ARR_SIZE(randwords);
    int s_idx6 = rnd() % ARR_SIZE(randwords);

    const char *w1 = randwords[s_idx];
    const char *w2 = randwords[s_idx2];
    const char *w3 = randwords[s_idx3];
    const char *w4 = randwords[s_idx4];
    const char *w5 = randwords[s_idx5];
    const char *w6 = randwords[s_idx6];

    char reply[BUF_MAX];
    int pos = 0;

    while (*w1 && pos < BUF_MAX - 10) reply[pos++] = *w1++;
    reply[pos++] = ' ';
    while (*w2 && pos < BUF_MAX - 2) reply[pos++] = *w2++;
    reply[pos++] = ' ';
    while (*w3 && pos < BUF_MAX - 2) reply[pos++] = *w3++;
    reply[pos++] = ' ';
    while (*w4 && pos < BUF_MAX - 2) reply[pos++] = *w4++;
    reply[pos]=0;
    while (*w5 && pos < BUF_MAX - 2) reply[pos++] = *w5++;
    reply[pos]=0;
    while (*w5 && pos < BUF_MAX - 2) reply[pos++]=*w5++;
    reply[pos]=0;

    char full[BUF_MAX];
    int f = 0;
    const char *tag = "< ";
    while (*tag) full[f++] = *tag++;
    for (int i = 0; reply[i] && f < BUF_MAX - 1; i++) full[f++] = reply[i];
    full[f] = 0;

    push_line(full);
}

void chat_init(void) {
    len = 0; buf[0] = 0;
    line_count = 0;
    scroll_offset = 0;
    rng = ((unsigned int)timer_read() << 8) | 0xDEAD;
    if (rng == 0) rng = 1;

    push_line("=== NwOS Chat ===");
    push_line("Type anything");
    push_line("");
}

void chat_handle_key(unsigned char c) {
    unsigned char u = (unsigned char)c;

    if (u == KEY_ENTER) {
        if (len > 0) {
            char echo[BUF_MAX + 4];
            echo[0] = '>'; echo[1] = ' ';
            for (int i = 0; i < len; i++) echo[i+2] = buf[i];
            echo[len+2] = 0;
            
            push_line(echo);
            gen_reply();
            
            len = 0;
            buf[0] = 0;
            scroll_offset = 0;
        }
    } else if (u == KEY_BACKSPACE) {
        if (len > 0) buf[--len] = 0;
    } else if (u >= 0x20 && u < 0x7F && len < BUF_MAX - 1) {
        buf[len++] = c;
        buf[len] = 0;
    }
}

void chat_scroll(int delta) {
    scroll_offset += delta;
    if (scroll_offset < 0) scroll_offset = 0;
    int max = line_count - 1;
    if (max < 0) max = 0;
    if (scroll_offset > max) scroll_offset = max;
}

void chat_draw(void) {
    gfx_clear(THEME_BG);
    
    gfx_rect(0, 0, 640, 32, THEME_BAR);
    gfx_puts(8, 8, "NwOS  |  Chat", THEME_BAR_FG, THEME_BAR);

    int visible = 22;
    int end   = line_count - scroll_offset;
    int start = end - visible;
    if (start < 0) start = 0;

    int y = 40;
    for (int i = start; i < end && i < line_count; i++) {
        unsigned char fg = THEME_FG;
        if      (lines[i][0] == '>') fg = BLUE;
        else if (lines[i][0] == '<') fg = LIGHT_GREEN;
        gfx_puts(8, y, lines[i], fg, THEME_BG);
        y += 16;
    }

    if (scroll_offset > 0) {
        gfx_puts(550, 40, "[SCROLL]", RED, THEME_BG);
    }

    int cy = 440;
    gfx_puts(8, cy, "You:", BLUE, THEME_BG);
    gfx_puts(56, cy, buf, THEME_FG, THEME_BG);
    gfx_rect(56 + len * 8, cy, 6, 16, LIGHT_GRAY);

    gfx_rect(0, 456, 640, 24, THEME_BAR);
    gfx_puts(8, 460, "ENTER = send | MOUSE WHEEL = scroll",
             THEME_BAR_FG, THEME_BAR);
}