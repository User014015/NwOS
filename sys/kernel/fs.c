#include "fs.h"

typedef struct {
    char name[FS_NAME_LEN];
    int  type;
    int  parent;
    int  used;
    int  size;
    char data[FS_MAX_SIZE];
} fs_node_t;

static fs_node_t nodes[FS_MAX_NODES];
static int       root;
static int       current_dir;

static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}
static void str_copy_n(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}
static int str_len(const char *s) { int n = 0; while (s[n]) n++; return n; }

static int alloc_node(void) {
    for (int i = 0; i < FS_MAX_NODES; i++)
        if (!nodes[i].used) return i;
    return -1;
}

static int find_child(int parent, const char *name) {
    for (int i = 0; i < FS_MAX_NODES; i++)
        if (nodes[i].used && nodes[i].parent == parent &&
            str_eq(nodes[i].name, name)) return i;
    return -1;
}

static int walk(const char *path) {
    if (!path || !*path) return -1;

    int cur;
    const char *p = path;

    if (*p == '/') { cur = root; p++; }
    else            cur = current_dir;

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

    if (*p == '/') { cur = root; p++; }
    else            cur = current_dir;

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
        if (child < 0) {
            int type = (*p == 0) ? final_type : FS_DIR;
            int idx = alloc_node();
            if (idx < 0) return -1;
            str_copy_n(nodes[idx].name, name, FS_NAME_LEN);
            nodes[idx].type   = type;
            nodes[idx].parent = cur;
            nodes[idx].used   = 1;
            nodes[idx].size   = 0;
            child = idx;
        } else {
            if (*p != 0 && nodes[child].type != FS_DIR) return -1;
        }
        cur = child;
    }
    return cur;
}

int fs_exists(const char *path) { return walk(path) >= 0; }

int fs_is_dir(const char *path) {
    int i = walk(path);
    return i >= 0 && nodes[i].type == FS_DIR;
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
    if (size == 0) size = str_len(data);
    if (size > FS_MAX_SIZE) size = FS_MAX_SIZE;

    int i = walk_create(path, FS_FILE);
    if (i < 0 || nodes[i].type != FS_FILE) return -1;

    for (int j = 0; j < size; j++) nodes[i].data[j] = data[j];
    nodes[i].size = size;
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
        for (int k = 0; k < FS_MAX_NODES; k++)
            if (nodes[k].used && nodes[k].parent == i) return -3;
    }
    nodes[i].used = 0;
    return 0;
}

int fs_count(const char *path) {
    int i = walk(path);
    if (i < 0 || nodes[i].type != FS_DIR) return -1;
    int n = 0;
    for (int k = 0; k < FS_MAX_NODES; k++)
        if (nodes[k].used && nodes[k].parent == i) n++;
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
    if (idx == root) { str_copy_n(out, "/", max); return; }

    char parent[128];
    build_path(nodes[idx].parent, parent, sizeof(parent));
    int plen = str_len(parent);

    if (plen > 0 && parent[plen - 1] != '/') {
        if (plen < (int)sizeof(parent) - 1) { parent[plen++] = '/'; parent[plen] = 0; }
    }
    int olen = 0;
    while (parent[olen] && olen < max - 1) { out[olen] = parent[olen]; olen++; }
    int k = 0;
    while (nodes[idx].name[k] && olen < max - 1) out[olen++] = nodes[idx].name[k++];
    out[olen] = 0;
}

const char *fs_pwd(void) {
    static char buf[128];
    build_path(current_dir, buf, sizeof(buf));
    return buf;
}

void fs_init(void) {
    for (int i = 0; i < FS_MAX_NODES; i++) nodes[i].used = 0;

    root = 0;
    nodes[root].used   = 1;
    nodes[root].type   = FS_DIR;
    nodes[root].parent = -1;
    str_copy_n(nodes[root].name, "/", FS_NAME_LEN);

    current_dir = root;

    fs_mkdir("/sys");
    fs_mkdir("/home");
    fs_mkdir("/tmp");

    fs_write("/sys/version.txt", "NwOS 2.0.1\n", 0);
    fs_write("/sys/kernel.bin",  "(kernel binary placeholder)\n", 0);

    fs_write("/home/readme.txt",
        "Welcome to NwOS 2.0!\n"
        "\n"
        "This is a RAM filesystem. Files are stored in memory\n"
        "and lost on reboot. On-disk FS is coming later.\n"
        "\n"
        "Try:\n"
        "  dir                - list current directory\n"
        "  cd(\"sys\")          - go to /sys\n"
        "  mkdir(\"stuff\")     - create directory\n"
        "  touch(\"file.nw\")   - create empty file\n"
        "  edit(\"file.nw\")    - open in editor\n"
        "  cat(\"file.nw\")     - print contents\n"
        "  pwd                - current path\n"
        "\n"
        "File extensions for NwC:\n"
        "  .nw   - NwC source\n"
        "  .nwo  - compiled binary\n"
        "  .lnw  - low-level NwC\n"
        "\n", 0);

    fs_write("/home/hello.nw",
        "// hello.nw - NwC example\n"
        "print(\"Hello, world!\");\n", 0);
}