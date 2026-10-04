#include "fs.h"
#include "fs.h"
#include "ata.h"
#include "rootfs.h"
#include "fs.h"
#include "ata.h"

#define FS_METADATA_LBA 100
#define FS_DATA_LBA_START 10

typedef struct {
    const char *path;
    int is_dir;
    const char *data;
    int size;
} init_fs_entry_t;

typedef struct {
    int used;
    int type;
    int parent;
    char name[FS_NAME_LEN];
    char data[FS_MAX_SIZE];
    int size;
} fs_node_t;

extern const init_fs_entry_t builtin_fs[];
static fs_node_t nodes[FS_MAX_NODES];
static int root = 0;
static int current_dir = 0;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { 
        if (*a++ != *b++) return 0; 
    }
    return *a == *b;
}

static void str_copy_n(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { 
        dst[i] = src[i]; 
        i++; 
    }
    dst[i] = 0;
}

static int str_len(const char *s) { 
    int n = 0; 
    while (s[n]) n++; 
    return n; 
}

#define FS_METADATA_LBA 2

void fs_sync_to_disk(void) {
    unsigned char *ptr = (unsigned char *)nodes;
    for (unsigned int i = 0; i < sizeof(nodes); i += ATA_SECTOR_SIZE) {
        ata_write_sector(FS_METADATA_LBA + (i / ATA_SECTOR_SIZE), ptr + i);
    }
}

void fs_load_from_disk(void) {
    unsigned char *ptr = (unsigned char *)nodes;
    for (unsigned int i = 0; i < sizeof(nodes); i += ATA_SECTOR_SIZE) {
        ata_read_sector(FS_METADATA_LBA + (i / ATA_SECTOR_SIZE), ptr + i);
    }
}

void fs_init(void) {
    for (int i = 0; i < FS_MAX_NODES; i++) nodes[i].used = 0;

    root = 0;
    nodes[root].used   = 1;
    nodes[root].type   = FS_DIR;
    nodes[root].parent = -1;
    str_copy_n(nodes[root].name, "/", FS_NAME_LEN);

    current_dir = root;

    /* Базовые директории */
    fs_mkdir("/sys");
    fs_mkdir("/home");
    fs_mkdir("/tmp");
    fs_mkdir("/saved");

    /* Загружаем файлы из rootfs_entries[] */
    for (int i = 0; i < rootfs_count; i++) {
        const rootfs_entry_t *e = &rootfs_entries[i];

        char full[FS_NAME_LEN * 8];
        int j = 0;
        full[j++] = '/';
        for (int k = 0; e->path[k] && j < (int)sizeof(full) - 1; k++)
            full[j++] = e->path[k];
        full[j] = 0;

        fs_write(full, e->data, e->size);
    }
}

static int alloc_node(void) {
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (!nodes[i].used) return i;
    }
    return -1;
}

static int find_child(int parent, const char *name) {
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent && str_eq(nodes[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int walk(const char *path) {
    if (!path || !*path) return current_dir;

    int cur;
    const char *p = path;

    if (*p == '/') { 
        cur = root; 
        p++; 
    } else {
        cur = current_dir;
    }

    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;

        char name[FS_NAME_LEN];
        int n = 0;
        while (*p && *p != '/') {
            if (n < FS_NAME_LEN - 1) name[n++] = *p;
            p++;
        }
        name[n] = 0;

        if (str_eq(name, "."))  continue;
        if (str_eq(name, "..")) {
            if (nodes[cur].parent >= 0) cur = nodes[cur].parent;
            continue;
        }

        if (nodes[cur].type != FS_DIR) return -1;

        int child = find_child(cur, name);
        if (child < 0) return -1;
        cur = child;
    }
    return cur;
}

static int walk_create(const char *path, int final_type) {
    if (!path || !*path) return -1;

    int cur;
    const char *p = path;

    if (*p == '/') { 
        cur = root; 
        p++; 
    } else {
        cur = current_dir;
    }

    while (*p) {
        while (*p == '/') p++;
        if (!*p) break;

        char name[FS_NAME_LEN];
        int n = 0;
        while (*p && *p != '/') {
            if (n < FS_NAME_LEN - 1) name[n++] = *p;
            p++;
        }
        name[n] = 0;

        if (str_eq(name, "."))  continue;
        if (str_eq(name, "..")) {
            if (nodes[cur].parent >= 0) cur = nodes[cur].parent;
            continue;
        }

        if (nodes[cur].type != FS_DIR) return -1;

        int child = find_child(cur, name);
        
        const char *next = p;
        while (*next == '/') next++;
        int is_last = (*next == 0);

        if (child < 0) {
            int type = is_last ? final_type : FS_DIR;
            int idx = alloc_node();
            if (idx < 0) return -1;

            str_copy_n(nodes[idx].name, name, FS_NAME_LEN);
            nodes[idx].type   = type;
            nodes[idx].parent = cur;
            nodes[idx].used   = 1;
            nodes[idx].size   = 0;
            child = idx;
        } else {
            if (!is_last && nodes[child].type != FS_DIR) return -1;
        }
        cur = child;
    }
    return cur;
}

int fs_exists(const char *path) { 
    return walk(path) >= 0; 
}

int fs_is_dir(const char *path) {
    int i = walk(path);
    return (i >= 0 && nodes[i].type == FS_DIR);
}

int fs_read(const char *path, char *out, int max) {
    int i = walk(path);
    if (i < 0 || nodes[i].type != FS_FILE) return -1;
    
    int n = nodes[i].size;
    if (n > max - 1) n = max - 1;
    
    for (int j = 0; j < n; j++) out[j] = nodes[i].data[j];
    out[n] = 0;
    return n;
}

int fs_write(const char *path, const char *data, int size) {
    if (!data) return -1;
    if (size == 0) size = str_len(data);
    if (size > FS_MAX_SIZE) size = FS_MAX_SIZE;

    int i = walk_create(path, FS_FILE);
    if (i < 0 || nodes[i].type != FS_FILE) return -1;

    for (int j = 0; j < size; j++) nodes[i].data[j] = data[j];
    nodes[i].size = size;
    fs_sync_to_disk();
    return size;
}

int fs_mkdir(const char *path) {
    int i = walk_create(path, FS_DIR);
    if (i < 0 || nodes[i].type != FS_DIR) return -1;
    return 0;
}

int fs_touch(const char *path) {
    int i = walk_create(path, FS_FILE);
    if (i < 0 || nodes[i].type != FS_FILE) return -1;
    return 0;
}

int fs_delete(const char *path) {
    int i = walk(path);
    if (i < 0) return -1;
    if (i == root) return -2;

    if (nodes[i].type == FS_DIR) {
        for (int k = 0; k < FS_MAX_NODES; k++) {
            if (nodes[k].used && nodes[k].parent == i) return -3;
        }
    }
    nodes[i].used = 0;
    return 0;
}

int fs_count(const char *path) {
    int i = walk(path);
    if (i < 0 || nodes[i].type != FS_DIR) return -1;
    
    int n = 0;
    for (int k = 0; k < FS_MAX_NODES; k++) {
        if (nodes[k].used && nodes[k].parent == i) n++;
    }
    return n;
}

static int child_at(int parent, int idx) {
    int n = 0;
    for (int k = 0; k < FS_MAX_NODES; k++) {
        if (nodes[k].used && nodes[k].parent == parent) {
            if (n == idx) return k;
            n++;
        }
    }
    return -1;
}

const char *fs_name_at(const char *path, int idx) {
    int i = walk(path);
    if (i < 0) return 0;
    int c = child_at(i, idx);
    return (c < 0) ? 0 : nodes[c].name;
}

int fs_type_at(const char *path, int idx) {
    int i = walk(path);
    if (i < 0) return -1;
    int c = child_at(i, idx);
    return (c < 0) ? -1 : nodes[c].type;
}

int fs_size_at(const char *path, int idx) {
    int i = walk(path);
    if (i < 0) return -1;
    int c = child_at(i, idx);
    return (c < 0) ? -1 : nodes[c].size;
}

int fs_cd(const char *path) {
    int i = walk(path);
    if (i < 0 || nodes[i].type != FS_DIR) return -1;
    current_dir = i;
    return 0;
}

static void build_path(int idx, char *out, int max) {
    if (idx == root) { 
        str_copy_n(out, "/", max); 
        return; 
    }

    char parent[128];
    build_path(nodes[idx].parent, parent, sizeof(parent));
    int plen = str_len(parent);

    if (plen > 0 && parent[plen - 1] != '/') {
        if (plen < (int)sizeof(parent) - 1) { 
            parent[plen++] = '/'; 
            parent[plen] = 0; 
        }
    }
    int olen = 0;
    while (parent[olen] && olen < max - 1) { 
        out[olen] = parent[olen]; 
        olen++; 
    }
    int k = 0;
    while (nodes[idx].name[k] && olen < max - 1) {
        out[olen++] = nodes[idx].name[k++];
    }
    out[olen] = 0;
}

const char *fs_pwd(void) {
    static char buf[128];
    build_path(current_dir, buf, sizeof(buf));
    return buf;
}

// type 'rm /' to get fun

int fs_delete_recursive(const char *path) {
    int i = walk(path);
    if (i < 0) return -1;

    if (i == root) {
        for (int k = 0; k < FS_MAX_NODES; k++) {
            if (k != root) nodes[k].used = 0;
        }
        return 0;
    }

    if (nodes[i].type == FS_DIR) {
        for (int k = 0; k < FS_MAX_NODES; k++) {
            if (nodes[k].used && nodes[k].parent == i) {
                nodes[k].used = 0;
            }
        }
    }
    nodes[i].used = 0;
    return 0;
}
// CNF

int cnf_get(const char *path, const char *key, char *out, int max) {
    static char buf[FS_MAX_SIZE];
    int n = fs_read(path, buf, FS_MAX_SIZE);
    if (n < 0) return -1;

    int klen = 0;
    while (key[klen]) klen++;

    int i = 0;
    while (i < n) {
        if (buf[i] == '\n' || buf[i] == '#' || buf[i] == '\r') {
            while (i < n && buf[i] != '\n') i++;
            if (i < n) i++;
            continue;
        }

        int match = 1;
        for (int k = 0; k < klen; k++) {
            if (i + k >= n || buf[i + k] != key[k]) { match = 0; break; }
        }
        if (!match || i + klen >= n || buf[i + klen] != '=') {
            while (i < n && buf[i] != '\n') i++;
            if (i < n) i++;
            continue;
        }

        i += klen + 1;
        int v = 0;
        while (i < n && buf[i] != '\n' && buf[i] != '\r' && v < max - 1)
            out[v++] = buf[i++];
        out[v] = 0;
        return v;
    }
    return -1;
}

int cnf_set(const char *path, const char *key, const char *value) {
    static char buf[FS_MAX_SIZE];
    int n = fs_read(path, buf, FS_MAX_SIZE);
    if (n < 0) n = 0;

    int klen = 0;
    while (key[klen]) klen++;
    int vlen = 0;
    while (value[vlen]) vlen++;

    char out_buf[FS_MAX_SIZE];
    int out = 0, i = 0, replaced = 0;

    while (i < n) {
        int ls = i;
        while (i < n && buf[i] != '\n') i++;
        int le = i;
        if (i < n) i++;

        int match = (le - ls >= klen + 1);
        if (match) {
            for (int k = 0; k < klen; k++)
                if (buf[ls + k] != key[k]) { match = 0; break; }
            if (match && buf[ls + klen] != '=') match = 0;
        }

        if (match && !replaced) {
            for (int k = 0; k < klen; k++) if (out < FS_MAX_SIZE-1) out_buf[out++] = key[k];
            if (out < FS_MAX_SIZE-1) out_buf[out++] = '=';
            for (int k = 0; k < vlen; k++) if (out < FS_MAX_SIZE-1) out_buf[out++] = value[k];
            if (out < FS_MAX_SIZE-1) out_buf[out++] = '\n';
            replaced = 1;
        } else {
            for (int k = ls; k < le; k++) if (out < FS_MAX_SIZE-1) out_buf[out++] = buf[k];
            if (out < FS_MAX_SIZE-1) out_buf[out++] = '\n';
        }
    }

    if (!replaced) {
        for (int k = 0; k < klen; k++) if (out < FS_MAX_SIZE-1) out_buf[out++] = key[k];
        if (out < FS_MAX_SIZE-1) out_buf[out++] = '=';
        for (int k = 0; k < vlen; k++) if (out < FS_MAX_SIZE-1) out_buf[out++] = value[k];
        if (out < FS_MAX_SIZE-1) out_buf[out++] = '\n';
    }

    out_buf[out] = 0;
    fs_write(path, out_buf, out);
    return 0;
}

int fs_wipe_all(void) {
    int removed = 0;
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && i != root) {
            nodes[i].used = 0;
            removed++;
        }
    }
    current_dir = root;
    return removed;
}