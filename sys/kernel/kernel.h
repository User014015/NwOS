#ifndef KERNEL_H
#define KERNEL_H

#define FS_DIR_LBA       200
#define FS_DIR_SECTORS   10
#define FS_DATA_LBA      (FS_DIR_LBA + FS_DIR_SECTORS)

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

int atoi_simple(const char* text);

int fs_read_binary(const char* name, unsigned char* buffer, unsigned int max_size);

int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
);

void fs_set_drive(int idx);

int  fs_get_drive(void);

/* switch the active disk (file operations then use that disk) */
int  fs_change_drive(int idx);
int  fs_change_drive_letter(char letter);
int  fs_drive_index_by_letter(char letter);
char fs_get_drive_letter(void);

void fs_format_disk(int drive_idx);

void installer(void);

#endif
