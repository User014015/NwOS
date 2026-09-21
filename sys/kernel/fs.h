#ifndef FS_H
#define FS_H

#define FS_MAX_NODES  64
#define FS_NAME_LEN   32
#define FS_MAX_SIZE   4096

#define FS_FILE 0
#define FS_DIR  1

void fs_init(void);

int  fs_exists(const char *path);
int  fs_is_dir(const char *path);

int  fs_read(const char *path, char *out, int max);
int  fs_write(const char *path, const char *data, int size);
int  fs_mkdir(const char *path);
int  fs_touch(const char *path);
int  fs_delete(const char *path);

int  fs_count(const char *path);
const char *fs_name_at(const char *path, int idx);
int  fs_type_at(const char *path, int idx);
int  fs_size_at(const char *path, int idx);

int  fs_cd(const char *path);
const char *fs_pwd(void);

#endif