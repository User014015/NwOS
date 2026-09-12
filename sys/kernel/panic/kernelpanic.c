#include "../../../include/kernelpanic.h"

/*
 * Terminal primitives live in kernel.c. kernelpanic.c is a
 * separate translation unit, so pull them in as externs rather
 * than duplicating the VGA/scrollback logic here.
 */
extern void print(const char* text);
extern void print_int(int number);
extern void putchar_os(char c);
extern void set_color(unsigned char color);
extern void set_base_color(unsigned char color);
extern void clear(void);

/*
 * Mirrors the palette in kernel.c. Kept local (rather than a
 * shared header) so this file only depends on kernel_panic's own
 * small interface.
 */
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

/*
 * SAFE panic: prints an inline warning (no screen wipe) and
 * returns. Use this when something went wrong but there's no
 * reason to believe continuing will make things worse - e.g. an
 * operation was aborted safely, or a sanity check caught a bug
 * that has no lasting effect on system state.
 */
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

/*
 * FATAL panic: the kernel can no longer trust its own state.
 * Takes over the whole screen, disables interrupts and halts the
 * CPU for good.
 */
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