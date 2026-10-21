#include "../../../include/kernelpanic.h"

#include "../kernel.h"
#include "../../../drivers/keyboard.h"

void reboot(void);

// Подключаем наш фреймбуфер (поправь путь, если нужно)
// Предполагается, что fb.h лежит в sys/kernel/fb.h
#include "../fb.h" 

extern void print_int(int number);
extern void putchar_os(char c);

#define COLOR_LIGHT_RED  12
#define COLOR_YELLOW     14
#define COLOR_WHITE      15
#define COLOR_CYAN       11

#define PANIC_COLOR      0x1F 
#define PANIC_BG         0x1F

static int p_x = 0;
static int p_y = 0;

static void panic_clear(void) {
    p_x = 0;
    p_y = 0;
    
    if (fb_is_available()) {
        for (int y = 0; y < 50; y++) {
            for (int x = 0; x < 80; x++) {
                fb_draw_char(x, y, ' ', PANIC_BG);
            }
        }
    }
}

static void panic_putc(char c) {
    if (c == '\n') {
        p_x = 0;
        p_y++;
        return;
    }
    
    if (fb_is_available()) {
        fb_draw_char(p_x, p_y, c, PANIC_COLOR);
    }
    
    p_x++;
    if (p_x >= 80) {
        p_x = 0;
        p_y++;
    }
}

static void panic_puts(const char* s) {
    while (*s) {
        panic_putc(*s++);
    }
}

static void panic_hex(unsigned int v) {
    const char* hex = "0123456789ABCDEF";
    panic_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        panic_putc(hex[(v >> i) & 0xF]);
    }
}

static void panic_int(int v) {
    char buf[12];
    int i = 0;
    if (v == 0) { panic_putc('0'); return; }
    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i > 0) panic_putc(buf[--i]);
}

static void set_panic_color(unsigned char fg, unsigned char bg) {
    (void)fg; (void)bg;
}

static void panic_safe(const char* message, unsigned int code, const char* file, int line)
{
}

static void panic_fatal(const char* message, unsigned int code, const char* file, int line)
{
    __asm__ volatile ("cli");

    panic_clear();

    panic_puts("\n\n");
    panic_puts("  +============================================================================+\n");
    panic_puts("  |                                                                            |\n");
    panic_puts("  |                          !!! KERNEL PANIC !!!                              |\n");
    panic_puts("  |                                                                            |\n");
    panic_puts("  +============================================================================+\n\n");

    panic_puts("   ERROR: ");
    panic_puts(message);
    panic_puts("\n\n");

    panic_puts("   CODE:  ");
    panic_hex(code);
    panic_puts("\n\n");

    panic_puts("   AT:    ");
    panic_puts(file);
    panic_puts(":");
    panic_int(line);
    panic_puts("\n\n");

    panic_puts("  +============================================================================+\n\n");
    
    if (code == 0x01111111) {
        panic_puts("   * You just deleted the root filesystem. Why? *\n\n");
    }

    panic_puts("   The system has halted to prevent data corruption.\n");
    panic_puts("   Please restart your computer or virtual machine.\n\n");

    panic_puts("   * Press Ctrl+Alt+Del in QEMU to restart. *\n");

    for (;;)
    {
        int k = keyboard_getkey();
        if (k == 'r' || k == 'R') {
            reboot();
        }
        __asm__ volatile ("hlt");
    }
}

void unknow_panic(unsigned int code) {
    print("Unknow error\nMSG:");
    print_int(DISK_DEATH);
    while (1)
    {
        __asm__ volatile ("hlt");
    }
}

void kernel_panic(const char* message, unsigned int code, const char* file, int line, PanicLevel level)
{
    if (level == PANIC_FATAL)
    {
        panic_fatal(message, code, file, line);
    }
    else
    {
        extern void set_color(unsigned char);
        extern void set_base_color(unsigned char);
        extern void print(const char*);
        set_color(COLOR_YELLOW);
        print("\n[!] KERNEL WARNING: ");
        set_color(COLOR_WHITE);
        print(message);
        print(" (Code: ");
        print_int(code);
        print(")\n");
        set_base_color(7);
    }
}
