#ifndef ROOTFS_H
#define ROOTFS_H

typedef struct {
    const char *path;
    const char *data;
    int size;
} rootfs_entry_t;

extern const rootfs_entry_t rootfs_entries[];
extern const int rootfs_count;

#endif