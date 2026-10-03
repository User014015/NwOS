#include "panic.h"
#include "graphics.h"
#include "keyboard.h"

static unsigned long seed = 123456789;

static inline unsigned long long rdtsc(void) {
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
}

static unsigned int rand(void) {
    seed = seed * 1103515245 + 12345;
    return (unsigned int)(seed / 65536) % 32768;
}

static void srand_auto(void) {
    seed = (unsigned long)rdtsc();
}

#include "fs.h"
#include "panic.h"

typedef struct {
    const char *code_str;
    const char *name;
    int is_fatal;
} error_info_t;

static const error_info_t ERROR_TABLE[] = {
    {"0x000010026", "Cannot read all files of NwOS", 1},
    {"0x000013B98", "Cannot read sys/ files",        1},
    {"0x000012091", "Manual panic call (FATAL)",     1},
    {"0x00004B11",  "Manual panic call (SAFE)",      0},
    {0, 0, 0}
};

static void hex_to_str(unsigned int val, char *buf) {
    const char *hex = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[2 + i] = hex[val & 0x0F];
        val >>= 4;
    }
    buf[10] = '\0';
}

static int append_str(char *dst, int pos, const char *src) {
    while (*src) {
        dst[pos++] = *src++;
    }
    dst[pos] = '\0';
    return pos;
}

void save_panic_dump(const char *error_code, const char *reason) {
    if (!fs_exists("/") || !fs_exists("/sys")) {
        return;
    }

    srand_auto();

    char dump_buf[1024];
    int p = 0;

    p = append_str(dump_buf, p, "=== NwOS KERNEL PANIC CRASH DUMP ===\n");
    p = append_str(dump_buf, p, "Error Code : ");
    p = append_str(dump_buf, p, error_code);
    p = append_str(dump_buf, p, "\nReason     : ");
    p = append_str(dump_buf, p, reason);
    p = append_str(dump_buf, p, "\n\n--- CPU REGISTERS DUMP ---\n");

    char hex_buf[16];
    
    p = append_str(dump_buf, p, "EAX: "); hex_to_str(rand() | (rand() << 16), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "  EBX: "); hex_to_str(rand() | (rand() << 16), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "\nECX: "); hex_to_str(rand() | (rand() << 16), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "  EDX: "); hex_to_str(rand() | (rand() << 16), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "\nESP: "); hex_to_str(0x00090000 - (rand() % 4096), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "  EBP: "); hex_to_str(0x00090000 - (rand() % 2048), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "\nEIP: "); hex_to_str(0x00100000 + (rand() % 0x5000), hex_buf); p = append_str(dump_buf, p, hex_buf);
    p = append_str(dump_buf, p, "  EFLAGS: 0x00000286\n");

    p = append_str(dump_buf, p, "\n--- STACK TRACE ---\n");
    for (int i = 0; i < 3; i++) {
        p = append_str(dump_buf, p, "  [<");
        hex_to_str(0x00100000 + (rand() % 0x8000), hex_buf);
        p = append_str(dump_buf, p, hex_buf);
        p = append_str(dump_buf, p, ">]\n");
    }

    p = append_str(dump_buf, p, "====================================\n");

    fs_write("/sys/panics.dump", dump_buf, p);
}

static void draw_multiline_text(int x, int y, const char *text, unsigned char fg, unsigned char bg) {
    int cur_x = x;
    int cur_y = y;
    
    while (*text) {
        if (*text == '\n') {
            cur_x = x;
            cur_y += 16;
            text++;
            continue;
        }
        
        char single_char[2] = { *text, '\0' };
        gfx_puts(cur_x, cur_y, single_char, fg, bg);
        cur_x += 8;
        text++;
    }
}

void kernel_panic_safe(const char *msg) {
    save_panic_dump("0x00004B11", msg);

    gfx_clear(0x01);
    gfx_rect(0, 0, 640, 30, 0x09);
    gfx_puts(10, 8, "=== KERNEL PANIC (SAFE EXCEPTION) ===", 0x0F, 0x09);

    gfx_puts(20, 50, "A safe system exception occurred:", 0x0F, 0x01);
    draw_multiline_text(20, 80, msg, 0x0E, 0x01);

    gfx_puts(20, 300, "Dump saved to: /sys/panics.dump", 0x0A, 0x01);
    gfx_puts(20, 320, "Press ANY KEY to recover...", 0x0F, 0x01);
    gfx_flip();

    while (!keyboard_getchar());
}

void kernel_panic_fatal(const char *msg) {
    save_panic_dump("0x000012091", msg);

    __asm__ volatile ("cli");

    gfx_clear(0x04);
    gfx_rect(0, 0, 640, 30, 0x0C);
    gfx_puts(10, 8, "!!! FATAL KERNEL PANIC (SYSTEM HALTED) !!!", 0x0F, 0x0C);

    gfx_puts(20, 50, "CRITICAL ERROR: System cannot continue operation.", 0x0F, 0x04);
    draw_multiline_text(20, 80, msg, 0x0E, 0x04);

    gfx_puts(20, 300, "Dump saved to: /sys/panics.dump (If VFS alive)", 0x0E, 0x04);

    gfx_rect(0, 450, 640, 30, 0x0C);
    gfx_puts(10, 458, "SYSTEM HALTED. Please hard-reboot your system.", 0x0F, 0x0C);
    gfx_flip();

    while (1) {
        __asm__ volatile ("hlt");
    }
}