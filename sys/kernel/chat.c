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
        for (int i = 1; i < LINE_MAX; i++)
            for (int j = 0; j < BUF_MAX; j++) lines[i-1][j] = lines[i][j];
        line_count--;
    }
    int i = 0;
    while (s[i] && i < BUF_MAX - 1) { lines[line_count][i] = s[i]; i++; }
    lines[line_count][i] = 0;
    line_count++;
}
static const char *tech_subj[] = {"the compiler","a kernel","the CPU","a pointer","the driver","this OS"};
static const char *tech_verb[] = {"compiled","crashed","loaded","parsed","mapped","interrupted"};
static const char *tech_obj[]  = {"the buffer","a new module","the video memory","a device","the stack","the shell"};

static const char *nat_subj[]  = {"the wind","a river","the mountain","an eagle","the sun","a tree"};
static const char *nat_verb[]  = {"whispers to","flies over","touches","warms","shakes","kisses"};
static const char *nat_obj[]   = {"the valley","a lake","the clouds","the forest","a stone","the sky"};

static const char *food_subj[] = {"the chef","a soup","the pizza","a cook","the baker","an apple"};
static const char *food_verb[] = {"tastes","smells","cooks","burns","loves","cools"};
static const char *food_obj[]  = {"of garlic","like home","the oven","in the pan","the sauce","with cheese"};

static const char *spc_subj[]  = {"a star","the comet","a galaxy","the moon","an asteroid","a nebula"};
static const char *spc_verb[]  = {"shines on","orbits","collides with","illuminates","passes","drifts past"};
static const char *spc_obj[]   = {"the void","a black hole","the planet","the nebula","a sun","the cosmos"};

#define NW(x) (int)(sizeof(x)/sizeof(x[0]))

static int str_has(const char *s, const char *sub) {
    for (; *s; s++) {
        const char *a = s, *b = sub;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*b) return 1;
    }
    return 0;
}

static int pick_theme(const char *msg) {
    if (str_has(msg,"cod")||str_has(msg,"prog")||str_has(msg,"kern")||
        str_has(msg,"comp")||str_has(msg,"nw")||str_has(msg,"os")||str_has(msg,"bug"))
        return 0;
    if (str_has(msg,"tre")||str_has(msg,"for")||str_has(msg,"riv")||str_has(msg,"natur")||
        str_has(msg,"sky")||str_has(msg,"bird")||str_has(msg,"sun")||str_has(msg,"lake"))
        return 1;
    if (str_has(msg,"foo")||str_has(msg,"pizz")||str_has(msg,"eat")||str_has(msg,"cook")||
        str_has(msg,"tast")||str_has(msg,"hung"))
        return 2;
    if (str_has(msg,"spac")||str_has(msg,"star")||str_has(msg,"plane")||str_has(msg,"moon")||
        str_has(msg,"gal"))
        return 3;
    return rnd() % 4;
}

static void gen_reply(const char *msg) {
    int t = pick_theme(msg);
    const char **S, **V, **O;
    int ns, nv, no;
    if      (t == 0) { S=tech_subj; V=tech_verb; O=tech_obj;  ns=NW(tech_subj); nv=NW(tech_verb); no=NW(tech_obj);  }
    else if (t == 1) { S=nat_subj;  V=nat_verb;  O=nat_obj;   ns=NW(nat_subj);  nv=NW(nat_verb);  no=NW(nat_obj);   }
    else if (t == 2) { S=food_subj; V=food_verb; O=food_obj;  ns=NW(food_subj); nv=NW(food_verb); no=NW(food_obj);  }
    else             { S=spc_subj;  V=spc_verb;  O=spc_obj;   ns=NW(spc_subj);  nv=NW(spc_verb);  no=NW(spc_obj);   }

    char reply[BUF_MAX];
    int k = 0;
    const char *parts[3];
    parts[0] = S[rnd() % ns];
    parts[1] = V[rnd() % nv];
    parts[2] = O[rnd() % no];
    for (int p = 0; p < 3; p++) {
        const char *w = parts[p];
        while (*w && k < BUF_MAX-2) reply[k++] = *w++;
        if (p < 2) reply[k++] = ' ';
    }
    reply[k] = 0;
    char full[BUF_MAX];
    int f = 0;
    const char *pre = "< ";
    while (*pre) full[f++] = *pre++;
    for (int i = 0; reply[i] && f < BUF_MAX-1; i++) full[f++] = reply[i];
    full[f] = 0;
    push_line(full);
}

void chat_init(void) {
    len = 0; buf[0] = 0;
    line_count = 0;
    scroll_offset = 0;
    rng = ((unsigned int)timer_read() << 8) | 0xDEAD;
    if (rng == 0) rng = 1;

    push_line("NwOS Chat v1.0");
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
            gen_reply(buf);
            len = 0;
            buf[0] = 0;
            scroll_offset = 0;
        }
    } else if (u == KEY_BACKSPACE) {
        if (len > 0) buf[--len] = 0;
    } else if (u >= 0x20 && u < 0x7F && len < BUF_MAX-1) {
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
    gfx_puts(8, 8, "NwOS 2.0.3  |  Chat", THEME_BAR_FG, THEME_BAR);

    int visible = 22;
    int end   = line_count - scroll_offset;
    int start = end - visible;
    if (start < 0) start = 0;

    int y = 40;
    for (int i = start; i < end && i < line_count; i++) {
        unsigned char fg = THEME_FG;
        if      (lines[i][0] == '>') fg = LIGHT_GREEN;
        else if (lines[i][0] == '<') fg = BLACK;
        gfx_puts(8, y, lines[i], fg, THEME_BG);
        y += 16;
    }

    if (scroll_offset > 0) {
        gfx_puts(600, 40, "[SCROLL]", RED, THEME_BG);
    }

    int cy = 440;
    gfx_puts(8, cy, "You:", LIGHT_GREEN, THEME_BG);
    gfx_puts(56, cy, buf, THEME_FG, THEME_BG);
    gfx_rect(56 + len * 8, cy, 6, 16, LIGHT_GRAY);

    gfx_rect(0, 456, 640, 24, THEME_BAR);
    gfx_puts(8, 460, "TYPE message | ENTER = send | MOUSE WHEEL = scroll | ESC = back",
             THEME_BAR_FG, THEME_BAR);
}