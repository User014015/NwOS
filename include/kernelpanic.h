#ifndef KERNELPANIC_H
#define KERNELPANIC_H
typedef enum
{
    PANIC_SAFE = 0,
    PANIC_FATAL = 1
} PanicLevel;
void kernel_panic(const char* message, const char* file, int line, PanicLevel level);

#define KPANIC_SAFE(msg)  kernel_panic((msg), __FILE__, __LINE__, PANIC_SAFE)
#define KPANIC_FATAL(msg) kernel_panic((msg), __FILE__, __LINE__, PANIC_FATAL)

#endif