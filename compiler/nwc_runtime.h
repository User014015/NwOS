#ifndef NWC_RUNTIME_H
#define NWC_RUNTIME_H

typedef unsigned int size_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void free(void* ptr);

long long strtoll(const char* text, char** endptr, int base);
double strtod(const char* text, char** endptr);

size_t strlen(const char* text);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t n);
char* strcpy(char* dst, const char* src);
const char* strrchr(const char* text, int ch);
void* memcpy(void* dst, const void* src, size_t n);
void* memset(void* dst, int value, size_t n);

int printf(const char* format, ...);
int fprintf(void* stream, const char* format, ...);
extern void* stderr;

void nwc_arena_reset(void);

void nwc_output_begin(
    unsigned char* buffer,
    unsigned int capacity,
    unsigned int* size_out
);

int nwc_output_write(
    const void* data,
    unsigned int size
);

unsigned int nwc_output_size(void);

#endif
