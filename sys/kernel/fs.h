#ifndef FS_H
#define FS_H

#define FS_MAX_FILES 32
#define FS_NAME_LEN  32
#define FS_MAX_SIZE  4096

void fs_init(void);
int  fs_count(void);
const char *fs_name(int idx);
int  fs_size(int idx);
int  fs_read(const char *name, char *out, int max);
int  fs_write(const char *name, const char *data, int size);
int  fs_delete(const char *name);
int  fs_exists(const char *name);

#endif