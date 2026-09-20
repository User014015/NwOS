#include "nwc_runtime.h"

#include <stdarg.h>

/*
 * libc-like functions strlen/strcmp/strncmp/memcpy are supplied by
 * the main NwOS kernel.c. Do not duplicate them here, or the linker
 * will report multiple definitions.
 */

extern void print(const char* text);
extern void print_int(int number);
extern void putchar_os(char c);

#define NWC_ARENA_SIZE 262144

static unsigned char nwc_arena[NWC_ARENA_SIZE];
static unsigned int nwc_arena_pos = 0;

static unsigned char* nwc_output_buffer = 0;
static unsigned int nwc_output_capacity = 0;
static unsigned int nwc_output_pos = 0;
static unsigned int* nwc_output_size_ptr = 0;

static void nwc_write_char(char c)
{
    putchar_os(c);
}

static void nwc_print_uint(unsigned int value)
{
    char buffer[16];
    int pos = 0;

    if (value == 0)
    {
        nwc_write_char('0');
        return;
    }

    while (value != 0 && pos < (int)sizeof(buffer))
    {
        buffer[pos++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (pos > 0)
        nwc_write_char(buffer[--pos]);
}

static void nwc_print_signed(long long value)
{
    unsigned long long magnitude;

    if (value < 0)
    {
        nwc_write_char('-');
        magnitude = (unsigned long long)(-(value + 1));
        magnitude += 1;
    }
    else
    {
        magnitude = (unsigned long long)value;
    }

    if (magnitude == 0)
    {
        nwc_write_char('0');
        return;
    }

    {
        char buffer[32];
        int pos = 0;

        while (magnitude != 0 && pos < (int)sizeof(buffer))
        {
            buffer[pos++] =
                (char)('0' + (magnitude % 10));
            magnitude /= 10;
        }

        while (pos > 0)
            nwc_write_char(buffer[--pos]);
    }
}

static void nwc_vprintf(const char* format, va_list ap)
{
    unsigned int i = 0;

    if (format == 0)
        return;

    while (format[i] != '\0')
    {
        if (format[i] != '%')
        {
            nwc_write_char(format[i]);
            i++;
            continue;
        }

        i++;

        if (format[i] == '%')
        {
            nwc_write_char('%');
            i++;
            continue;
        }

        if (format[i] == 's')
        {
            const char* text = va_arg(ap, const char*);

            if (text != 0)
                print(text);

            i++;
            continue;
        }

        if (format[i] == 'c')
        {
            int value = va_arg(ap, int);
            nwc_write_char((char)value);
            i++;
            continue;
        }

        if (format[i] == 'd' || format[i] == 'i')
        {
            int value = va_arg(ap, int);
            nwc_print_signed((long long)value);
            i++;
            continue;
        }

        if (format[i] == 'u')
        {
            unsigned int value =
                va_arg(ap, unsigned int);
            nwc_print_uint(value);
            i++;
            continue;
        }

        if (format[i] == 'l' &&
            format[i + 1] == 'l' &&
            (format[i + 2] == 'd' ||
             format[i + 2] == 'i'))
        {
            long long value = va_arg(ap, long long);
            nwc_print_signed(value);
            i += 3;
            continue;
        }

        if (format[i] == 'f')
        {
            /* AST debug output only needs a readable value.
               The in-kernel compiler itself does not use %f. */
            int value = (int)va_arg(ap, double);
            nwc_print_signed((long long)value);
            i++;
            continue;
        }

        nwc_write_char('%');
        nwc_write_char(format[i]);
        i++;
    }
}

void* stderr = 0;

int printf(const char* format, ...)
{
    va_list ap;

    va_start(ap, format);
    nwc_vprintf(format, ap);
    va_end(ap);

    return 0;
}

int fprintf(void* stream, const char* format, ...)
{
    va_list ap;

    (void)stream;

    va_start(ap, format);
    nwc_vprintf(format, ap);
    va_end(ap);

    return 0;
}

static unsigned int nwc_align8(unsigned int value)
{
    return (value + 7u) & ~7u;
}

void* malloc(size_t size)
{
    unsigned int start;
    unsigned int aligned;

    if (size == 0)
        return 0;

    start = nwc_align8(nwc_arena_pos);
    aligned = nwc_align8((unsigned int)size);

    if (start > NWC_ARENA_SIZE ||
        aligned > NWC_ARENA_SIZE - start)
        return 0;

    nwc_arena_pos = start + aligned;

    return &nwc_arena[start];
}

void* calloc(size_t count, size_t size)
{
    unsigned int total;
    unsigned char* memory;
    unsigned int i;

    if (count == 0 || size == 0)
        return 0;

    if ((unsigned int)count >
        0xFFFFFFFFu / (unsigned int)size)
        return 0;

    total = (unsigned int)count * (unsigned int)size;

    memory = (unsigned char*)malloc(total);

    if (memory == 0)
        return 0;

    for (i = 0; i < total; i++)
        memory[i] = 0;

    return memory;
}

void free(void* ptr)
{
    (void)ptr;
}

long long strtoll(const char* text, char** endptr, int base)
{
    long long value = 0;
    int sign = 1;
    unsigned int i = 0;

    if (base != 10 || text == 0)
    {
        if (endptr != 0)
            *endptr = (char*)text;
        return 0;
    }

    while (text[i] == ' ' ||
           text[i] == '\t' ||
           text[i] == '\r' ||
           text[i] == '\n')
    {
        i++;
    }

    if (text[i] == '-')
    {
        sign = -1;
        i++;
    }
    else if (text[i] == '+')
    {
        i++;
    }

    while (text[i] >= '0' && text[i] <= '9')
    {
        value = value * 10 + (long long)(text[i] - '0');
        i++;
    }

    if (endptr != 0)
        *endptr = (char*)(text + i);

    return sign < 0 ? -value : value;
}

double strtod(const char* text, char** endptr)
{
    long long whole = 0;
    long long fraction = 0;
    long long scale = 1;
    int sign = 1;
    unsigned int i = 0;
    double value;

    if (text == 0)
    {
        if (endptr != 0)
            *endptr = 0;
        return 0.0;
    }

    while (text[i] == ' ' ||
           text[i] == '\t' ||
           text[i] == '\r' ||
           text[i] == '\n')
    {
        i++;
    }

    if (text[i] == '-')
    {
        sign = -1;
        i++;
    }
    else if (text[i] == '+')
    {
        i++;
    }

    while (text[i] >= '0' && text[i] <= '9')
    {
        whole = whole * 10 + (text[i] - '0');
        i++;
    }

    if (text[i] == '.')
    {
        i++;

        while (text[i] >= '0' && text[i] <= '9')
        {
            fraction = fraction * 10 + (text[i] - '0');
            scale *= 10;
            i++;
        }
    }

    value = (double)whole +
            (double)fraction / (double)scale;

    if (endptr != 0)
        *endptr = (char*)(text + i);

    return sign < 0 ? -value : value;
}

char* strcpy(char* dst, const char* src)
{
    unsigned int i = 0;

    if (dst == 0)
        return dst;

    if (src == 0)
        src = "";

    while (src[i] != '\0')
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

const char* strrchr(const char* text, int ch)
{
    const char* last = 0;

    if (text == 0)
        return 0;

    while (*text != '\0')
    {
        if ((unsigned char)*text ==
            (unsigned char)ch)
        {
            last = text;
        }

        text++;
    }

    if (ch == '\0')
        return text;

    return last;
}

void* memset(void* dst, int value, size_t n)
{
    unsigned char* d = (unsigned char*)dst;
    unsigned int i;

    if (dst == 0)
        return dst;

    for (i = 0; i < (unsigned int)n; i++)
        d[i] = (unsigned char)value;

    return dst;
}

void nwc_arena_reset(void)
{
    nwc_arena_pos = 0;
}

void nwc_output_begin(
    unsigned char* buffer,
    unsigned int capacity,
    unsigned int* size_out)
{
    nwc_output_buffer = buffer;
    nwc_output_capacity = capacity;
    nwc_output_pos = 0;
    nwc_output_size_ptr = size_out;

    if (nwc_output_size_ptr != 0)
        *nwc_output_size_ptr = 0;
}

int nwc_output_write(const void* data, unsigned int size)
{
    const unsigned char* src =
        (const unsigned char*)data;
    unsigned int i;

    if (size == 0)
        return 1;

    if (nwc_output_buffer == 0 ||
        size > nwc_output_capacity - nwc_output_pos)
        return 0;

    for (i = 0; i < size; i++)
        nwc_output_buffer[nwc_output_pos + i] =
            src[i];

    nwc_output_pos += size;

    if (nwc_output_size_ptr != 0)
        *nwc_output_size_ptr = nwc_output_pos;

    return 1;
}

unsigned int nwc_output_size(void)
{
    return nwc_output_pos;
}
