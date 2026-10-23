#ifndef KERNEL_H
#define KERNEL_H

#define FS_DIR_LBA       200
#define FS_DIR_SECTORS   10
#define FS_DATA_LBA      (FS_DIR_LBA + FS_DIR_SECTORS)

/* VERSIONS + NAME */

#define OS_NAME            "NwOS"
#define OS_VERSION_MAJOR   1
#define OS_VERSION_MINOR   4
#define OS_VERSION_PATCH   5
#define OS_MADE_DATE      "2026.08.27"

#define OS_STR_HELPER(x)   #x
#define OS_STR(x)          OS_STR_HELPER(x)

#define OS_VERSION_STR     OS_STR(OS_VERSION_MAJOR) "." \
                          OS_STR(OS_VERSION_MINOR) "." \
                          OS_STR(OS_VERSION_PATCH)

void print(const char* text);
int random_range(int min, int max);
void print_int(int number);
int strcmp(const char* a, const char* b);
void print_int(int number);
void print_error(const char* text);
void print_success(const char* text);
void putchar_os(char c);
void change_p_color(void);
void print_shell_p(void);
void shell(void);



#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA      5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GRAY   7
#define COLOR_DARK_GRAY    8
#define COLOR_LIGHT_BLUE   9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN  11
#define COLOR_LIGHT_RED   12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW      14
#define COLOR_WHITE       15




void clear(void);

void read_line(char* buffer, int max);

int fs_find(const char* name);

int atoi_simple(const char* text);
void fs_store_data(int index, const char* text, const char* success_message);
void fs_write(const char* name, const char* text);
void fs_read(const char* name);
void fs_edit(const char* name, const char* text);
void fs_delete(const char* name);
void fs_create(const char* name);
void fs_mkdir(const char* name);
void fs_ls(void);
void fs_cd(const char* name);
void app_file_stats(void);
void app_base_converter(void);
void apps_list();
int fs_read_binary(const char* name, unsigned char* buffer, unsigned int max_size);
unsigned int fs_get_file_size(const char* name);

int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
);

void fs_set_drive(int idx);

int  fs_get_drive(void);
void dead_boot(void);
/* switch the active disk (file operations then use that disk) */
int  fs_change_drive(int idx);
int  fs_change_drive_letter(char letter);
int  fs_drive_index_by_letter(char letter);
char fs_get_drive_letter(void);

void fs_format_disk(int drive_idx);

void installer(void);

#endif
