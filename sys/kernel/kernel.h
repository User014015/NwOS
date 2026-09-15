#ifndef KERNEL_H
#define KERNEL_H

void print(const char* text);
int random_range(int min, int max);
void print_int(int number);
int strcmp(const char* a, const char* b);
void print_int(int number);
void print_error(const char* text);
void print_success(const char* text);
void putchar_os(char c);

void clear(void);

void read_line(char* buffer, int max);

int fs_find(const char* name);

int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
);

#endif