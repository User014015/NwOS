#ifndef KERNELPANIC_H
#define KERNELPANIC_H

typedef enum
{
    PANIC_SAFE = 0,
    PANIC_FATAL = 1
} PanicLevel;

#define ERR_NONE                0x00200000
#define ERR_DEVICE_NOT_FOUND    0x00000711
#define ERR_DEVICE_TIMEOUT      0x00001000
#define ERR_DISK_READ           0x00088821
#define ERR_DISK_WRITE          0x00000400
#define ERR_FS_CORRUPT          0x01110000
#define ERR_OUT_OF_MEMORY       0x00203331

#define ERR_CPU_EXCEPTION       0x00006642
#define ERR_DIVIDE_BY_ZERO      0x00000877
#define ERR_PAGE_FAULT          0x00000898
#define ERR_BAD_STATE           0x00010013
#define ERR_TRIPLE_FAULT        0x05400068

#define DISK_DEATH              0x19992888

#define ERR_UNKNOW              0x00000000

#define ERR_DEATH               0x01111111

void kernel_panic(const char* message, unsigned int code, const char* file, int line, PanicLevel level);

#define KPANIC_SAFE(code, msg)  kernel_panic((msg), (code), __FILE__, __LINE__, PANIC_SAFE)
#define KPANIC_FATAL(code, msg) kernel_panic((msg), (code), __FILE__, __LINE__, PANIC_FATAL)

#endif
