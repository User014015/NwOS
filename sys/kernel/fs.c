#include "fs.h"
#include "string.h"

typedef struct {
    char name[FS_NAME_LEN];
    char data[FS_MAX_SIZE];
    int  size;
    int  used;
} fs_file_t;

static fs_file_t files[FS_MAX_FILES];
static int       file_count = 0;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}
static int str_len(const char *s) {
    int n = 0; while (s[n]) n++; return n;
}

static int find(const char *name) {
    for (int i = 0; i < file_count; i++)
        if (files[i].used && str_eq(files[i].name, name)) return i;
    return -1;
}

int fs_write(const char *name, const char *data, int size) {
    if (size == 0) size = str_len(data);
    if (size > FS_MAX_SIZE) size = FS_MAX_SIZE;

    int idx = find(name);
    if (idx < 0) {
        if (file_count >= FS_MAX_FILES) return -2;
        idx = file_count++;
        files[idx].used = 1;
        int i = 0;
        while (name[i] && i < FS_NAME_LEN - 1) { files[idx].name[i] = name[i]; i++; }
        files[idx].name[i] = 0;
    }
    for (int j = 0; j < size; j++) files[idx].data[j] = data[j];
    files[idx].size = size;
    return size;
}

void fs_init(void) {
    file_count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) files[i].used = 0;

    const char *r =
        "Welcome to NwOS 2.0.1!\n"
        "\n"
        "This is a demo text file in RAM FS.\n"
        "Edit me with the Editor app.\n"
        "Changes are saved to RAM only.\n"
        "\n"
        "Try:\n"
        "  - move cursor with arrow keys\n"
        "  - type any text\n"
        "  - ESC to save and exit\n"
        "\n"
        "Have fun!\n";
    fs_write("readme.txt", r, 0);

    const char *h =
        "Hello, world!\n"
        "This is hello.txt.\n"
        "Try editing it in the Editor.\n";
    fs_write("hello.txt", h, 0);
}

int fs_count(void)        { return file_count; }
int fs_exists(const char *name) { return find(name) >= 0; }

const char *fs_name(int idx) {
    if (idx < 0 || idx >= file_count) return 0;
    return files[idx].name;
}
int fs_size(int idx) {
    if (idx < 0 || idx >= file_count) return 0;
    return files[idx].size;
}

int fs_read(const char *name, char *out, int max) {
    int i = find(name);
    if (i < 0) return -1;
    int n = files[i].size;
    if (n > max - 1) n = max - 1;
    for (int j = 0; j < n; j++) out[j] = files[i].data[j];
    out[n] = 0;
    return n;
}

int fs_delete(const char *name) {
    int i = find(name);
    if (i < 0) return -1;
    for (int j = i; j < file_count - 1; j++) {
        for (int k = 0; k < FS_NAME_LEN; k++)
            files[j].name[k] = files[j+1].name[k];
        for (int k = 0; k < files[j+1].size; k++)
            files[j].data[k] = files[j+1].data[k];
        files[j].size = files[j+1].size;
        files[j].used = files[j+1].used;
    }
    file_count--;
    return 0;
}