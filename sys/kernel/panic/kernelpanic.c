#include "../../../include/kernelpanic.h"

extern void print(const char* text);
extern void print_int(int number);
extern void putchar_os(char c);
extern void set_color(unsigned char color);
extern void set_base_color(unsigned char color);
extern void clear(void);

#define PANIC_COLOR_LIGHT_GRAY 7
#define PANIC_COLOR_LIGHT_RED  12
#define PANIC_COLOR_YELLOW     14
#define PANIC_COLOR_WHITE      15

static void panic_print_location(const char* file, int line)
{
    print("  Location: ");
    print(file);
    print(":");
    print_int(line);
    putchar_os('\n');
}

static void panic_safe(const char* message, const char* file, int line)
{
    set_color(PANIC_COLOR_YELLOW);
    print("\n[!] KERNEL WARNING\n");
    print("  ");
    print(message);
    putchar_os('\n');

    panic_print_location(file, line);

    print("  Continuing, but this may be unstable.\n\n");

    set_base_color(PANIC_COLOR_LIGHT_GRAY);
}

static void panic_fatal(const char* message, const char* file, int line)
{
    __asm__ volatile ("cli");

    clear();
    set_color(PANIC_COLOR_WHITE);

    print("================================================================================\n");
    print("                              !!! KERNEL PANIC !!!\n");
    print("================================================================================\n\n");

    set_color(PANIC_COLOR_LIGHT_RED);
    print("  ");
    print(message);
    putchar_os('\n');

    set_color(PANIC_COLOR_WHITE);
    panic_print_location(file, line);

    print("\n  The system has halted to avoid further damage.\n");
    print("  Please restart the machine manually.\n");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

void kernel_panic(const char* message, const char* file, int line, PanicLevel level)
{
    if (level == PANIC_FATAL)
    {
        panic_fatal(message, file, line);
    }
    else
    {
        panic_safe(message, file, line);
    }
}