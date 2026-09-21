#include "keyboard.h"
#include "io.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

#define QSIZE 32
static char queue[QSIZE];
static int  q_head = 0, q_tail = 0;

static int shift_l = 0, shift_r = 0, caps = 0;
static int extended = 0;

static const char keymap_lo[128] = {
    0,   0x1B, '1','2','3','4','5','6','7','8','9','0','-','=', 0x08,
    0x09,'q','w','e','r','t','y','u','i','o','p','[',']', 0x0A,
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0,   ' '
};

static const char keymap_hi[128] = {
    0,   0x1B, '!','@','#','$','%','^','&','*','(',')','_','+', 0x08,
    0x09,'Q','W','E','R','T','Y','U','I','O','P','{','}', 0x0A,
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0,   ' '
};

static void push(char c) {
    if (c == 0) return;
    int next = (q_head + 1) % QSIZE;
    if (next == q_tail) return;
    queue[q_head] = c;
    q_head = next;
}

static char pop(void) {
    if (q_head == q_tail) return 0;
    char c = queue[q_tail];
    q_tail = (q_tail + 1) % QSIZE;
    return c;
}

static void process_scancode(unsigned char sc) {
    if (extended) {
        extended = 0;
        if (sc & 0x80) return;
        switch (sc) {
            case 0x48: push(KEY_UP);     return;
            case 0x50: push(KEY_DOWN);   return;
            case 0x4B: push(KEY_LEFT);   return;
            case 0x4D: push(KEY_RIGHT);  return;
            case 0x47: push(KEY_HOME);   return;
            case 0x4F: push(KEY_END);    return;
            case 0x53: push(KEY_DELETE); return;
            case 0x49: push(KEY_PGUP);   return;
            case 0x51: push(KEY_PGDN);   return;
            default: return;
        }
    }

    if (sc == 0xE0) { extended = 1; return; }
    if (sc & 0x80) {
        unsigned char code = sc & 0x7F;
        if (code == 0x2A) shift_l = 0;
        if (code == 0x36) shift_r = 0;
        return;
    }
    if (sc == 0x2A) { shift_l = 1; return; }
    if (sc == 0x36) { shift_r = 1; return; }
    if (sc == 0x3A) { caps = !caps; return; }

    if (sc >= 128) return;
    char lo = keymap_lo[sc];
    if (!lo) return;

    int sh = shift_l || shift_r;
    int use_hi = 0;
    if (lo >= 'a' && lo <= 'z')            use_hi = sh ^ caps;
    else if (lo != 0x1B && lo != 0x08 && lo != 0x09 && lo != 0x0A && lo != ' ')
        use_hi = sh;

    push(use_hi ? keymap_hi[sc] : lo);
}
static void kbd_poll(void) {
    while (1) {
        unsigned char st = inb(KBD_STATUS);
        if (!(st & 0x01)) break;
        if (st & 0x20) { (void)inb(KBD_DATA); continue; }
        process_scancode((unsigned char)inb(KBD_DATA));
    }
}

void keyboard_init(void) {
    for (int i = 0; i < 1000 && (inb(KBD_STATUS) & 1); i++)
        (void)inb(KBD_DATA);
    outb(0x64, 0x20);
    int t = 100000;
    while (t-- && !(inb(KBD_STATUS) & 1)) __asm__ volatile("pause");
    unsigned char cfg = inb(0x60);
    cfg &= ~0x03;
    cfg &= ~0x30;
    outb(0x64, 0x60);
    outb(0x60, cfg);

    shift_l = shift_r = caps = 0;
    extended = 0;
    q_head = q_tail = 0;
}

int keyboard_has_input(void) {
    kbd_poll();
    return q_head != q_tail;
}

char keyboard_getchar(void) {
    kbd_poll();
    extern unsigned char g_last_key_debug;
    char c = pop();
    g_last_key_debug = (unsigned char)c;
    return c;
}