#ifndef KERNELPANIC_H
#define KERNELPANIC_H

/*
 * Kernel panic severity levels.
 *
 *   PANIC_SAFE  - something is wrong, but core state (memory,
 *                 disk, CPU) is still trustworthy. Prints a
 *                 warning banner and RETURNS control to the
 *                 caller, which should abandon whatever it was
 *                 doing (e.g. abort the current command) and
 *                 keep the system running.
 *
 *   PANIC_FATAL - core state can no longer be trusted (e.g. the
 *                 disk stopped responding mid-transfer, memory
 *                 looks corrupted). Takes over the screen,
 *                 disables interrupts and halts the CPU forever.
 *                 Does NOT return - the only way out is a
 *                 hardware reset.
 */
typedef enum
{
    PANIC_SAFE = 0,
    PANIC_FATAL = 1
} PanicLevel;

/*
 * Raises a kernel panic. Prefer the KPANIC_SAFE / KPANIC_FATAL
 * macros below over calling this directly, so file/line are
 * filled in automatically.
 */
void kernel_panic(const char* message, const char* file, int line, PanicLevel level);

#define KPANIC_SAFE(msg)  kernel_panic((msg), __FILE__, __LINE__, PANIC_SAFE)
#define KPANIC_FATAL(msg) kernel_panic((msg), __FILE__, __LINE__, PANIC_FATAL)

#endif