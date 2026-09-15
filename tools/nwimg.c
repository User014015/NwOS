#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang tools/nwimg.c -o tools/nwimg.exe
//
// tools\nwimg.exe add NwOS.img nw/test.nw NwDev/test.nw

// import nwo
// tools\nwimg.exe add NwOS.img nw\test.nwo NwApps/test.nwo

// import nw
// tools\nwimg.exe add NwOS.img nw\test.nw NwDev/test.nw


/*
 * ============================================================
 * NwOS image tool
 * ============================================================
 *
 * Current NwOS filesystem:
 *
 *   LBA 200-201 : directory
 *   LBA 202+    : file data
 *
 * Current directory entry:
 *
 *   used      1 byte
 *   name     32 bytes
 *   size      4 bytes
 *
 * One file currently occupies one 512-byte sector.
 *
 * This tool ONLY operates on NwOS.img files.
 * It never opens physical Windows disks.
 */


/* ============================================================
   Filesystem constants
   ============================================================ */

#define SECTOR_SIZE     512

#define FS_DIR_LBA      200
#define FS_DIR_SECTORS  2
#define FS_DATA_LBA     202

#define MAX_FILES       16
#define MAX_FILENAME    32


/* ============================================================
   Directory entry
   ============================================================ */

#pragma pack(push, 1)

typedef struct
{
    unsigned char used;

    char name[MAX_FILENAME];

    unsigned int size;

} DirEntry;

#pragma pack(pop)


/* ============================================================
   Helpers
   ============================================================ */

static void write_u32_le(
    FILE* f,
    unsigned int value)
{
    unsigned char b[4];

    b[0] = (unsigned char)(value & 0xFF);
    b[1] = (unsigned char)((value >> 8) & 0xFF);
    b[2] = (unsigned char)((value >> 16) & 0xFF);
    b[3] = (unsigned char)((value >> 24) & 0xFF);

    fwrite(b, 1, 4, f);
}


static unsigned int read_u32_le(
    const unsigned char* b)
{
    return
        ((unsigned int)b[0]) |
        ((unsigned int)b[1] << 8) |
        ((unsigned int)b[2] << 16) |
        ((unsigned int)b[3] << 24);
}


/* ============================================================
   Read whole directory
   ============================================================ */

static int read_directory(
    FILE* image,
    DirEntry* directory)
{
    unsigned char buffer[
        FS_DIR_SECTORS * SECTOR_SIZE
    ];

    unsigned long offset;

    offset =
        (unsigned long)FS_DIR_LBA *
        SECTOR_SIZE;

    if (fseek(
            image,
            (long)offset,
            SEEK_SET) != 0)
    {
        return 0;
    }

    if (fread(
            buffer,
            1,
            sizeof(buffer),
            image) != sizeof(buffer))
    {
        return 0;
    }

    memcpy(
        directory,
        buffer,
        sizeof(DirEntry) * MAX_FILES);

    return 1;
}


/* ============================================================
   Write whole directory
   ============================================================ */

static int write_directory(
    FILE* image,
    DirEntry* directory)
{
    unsigned char buffer[
        FS_DIR_SECTORS * SECTOR_SIZE
    ];

    unsigned long offset;

    memset(
        buffer,
        0,
        sizeof(buffer));

    memcpy(
        buffer,
        directory,
        sizeof(DirEntry) * MAX_FILES);

    offset =
        (unsigned long)FS_DIR_LBA *
        SECTOR_SIZE;

    if (fseek(
            image,
            (long)offset,
            SEEK_SET) != 0)
    {
        return 0;
    }

    if (fwrite(
            buffer,
            1,
            sizeof(buffer),
            image) != sizeof(buffer))
    {
        return 0;
    }

    fflush(image);

    return 1;
}


/* ============================================================
   Find entry
   ============================================================ */

static int find_entry(
    DirEntry* directory,
    const char* name)
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used == 1 &&
            strcmp(
                directory[i].name,
                name) == 0)
        {
            return i;
        }
    }

    return -1;
}


/* ============================================================
   Find free slot
   ============================================================ */

static int find_free_entry(
    DirEntry* directory)
{
    int i;

    for (i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used != 1)
            return i;
    }

    return -1;
}


/* ============================================================
   Read host file
   ============================================================ */

static unsigned char* read_host_file(
    const char* path,
    unsigned int* size)
{
    FILE* f;
    long file_size;

    unsigned char* data;

    f = fopen(path, "rb");

    if (f == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot open '%s'\n",
            path);

        return NULL;
    }

    if (fseek(
            f,
            0,
            SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }

    file_size = ftell(f);

    if (file_size < 0)
    {
        fclose(f);
        return NULL;
    }

    if (file_size > SECTOR_SIZE)
    {
        fprintf(
            stderr,
            "nwimg: file is too large.\n");

        fprintf(
            stderr,
            "Current filesystem limit: %d bytes.\n",
            SECTOR_SIZE);

        fclose(f);
        return NULL;
    }

    if (fseek(
            f,
            0,
            SEEK_SET) != 0)
    {
        fclose(f);
        return NULL;
    }

    data =
        (unsigned char*)malloc(
            SECTOR_SIZE);

    if (data == NULL)
    {
        fclose(f);
        return NULL;
    }

    memset(
        data,
        0,
        SECTOR_SIZE);

    if (fread(
            data,
            1,
            (size_t)file_size,
            f) != (size_t)file_size)
    {
        free(data);
        fclose(f);
        return NULL;
    }

    fclose(f);

    *size =
        (unsigned int)file_size;

    return data;
}


/* ============================================================
   ADD
   ============================================================ */

static int add_file(
    const char* image_path,
    const char* host_path,
    const char* nwos_name)
{
    FILE* image;

    DirEntry directory[MAX_FILES];

    unsigned char* data;

    unsigned int size;

    int index;

    unsigned long offset;

    image =
        fopen(
            image_path,
            "rb+");

    if (image == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot open image '%s'\n",
            image_path);

        return 0;
    }

    if (!read_directory(
            image,
            directory))
    {
        fprintf(
            stderr,
            "nwimg: cannot read filesystem directory.\n");

        fclose(image);
        return 0;
    }

    /*
     * Do not overwrite an existing file silently.
     */
    index =
        find_entry(
            directory,
            nwos_name);

    if (index != -1)
    {
        fprintf(
            stderr,
            "nwimg: file already exists: %s\n",
            nwos_name);

        fclose(image);
        return 0;
    }

    /*
     * Find free slot.
     */
    index =
        find_free_entry(
            directory);

    if (index == -1)
    {
        fprintf(
            stderr,
            "nwimg: no free file slots.\n");

        fclose(image);
        return 0;
    }

    /*
     * Read host file.
     */
    data =
        read_host_file(
            host_path,
            &size);

    if (data == NULL)
    {
        fclose(image);
        return 0;
    }

    /*
     * Name must fit.
     */
    if (strlen(nwos_name) >=
        MAX_FILENAME)
    {
        fprintf(
            stderr,
            "nwimg: filename is too long.\n");

        free(data);
        fclose(image);
        return 0;
    }

    /*
     * Prepare directory entry.
     */
    memset(
        &directory[index],
        0,
        sizeof(DirEntry));

    directory[index].used = 1;

    strcpy(
        directory[index].name,
        nwos_name);

    directory[index].size =
        size;

    /*
     * Current filesystem:
     *
     *     slot 0 -> LBA 202
     *     slot 1 -> LBA 203
     *     ...
     */
    offset =
        ((unsigned long)FS_DATA_LBA +
         (unsigned long)index)
        * SECTOR_SIZE;

    if (fseek(
            image,
            (long)offset,
            SEEK_SET) != 0)
    {
        fprintf(
            stderr,
            "nwimg: cannot seek to data sector.\n");

        free(data);
        fclose(image);
        return 0;
    }

    if (fwrite(
            data,
            1,
            SECTOR_SIZE,
            image) != SECTOR_SIZE)
    {
        fprintf(
            stderr,
            "nwimg: failed to write file data.\n");

        free(data);
        fclose(image);
        return 0;
    }

    /*
     * Save directory.
     */
    if (!write_directory(
            image,
            directory))
    {
        fprintf(
            stderr,
            "nwimg: failed to write directory.\n");

        free(data);
        fclose(image);
        return 0;
    }

    fclose(image);

    free(data);

    printf(
        "Added '%s' as '%s' (%u bytes)\n",
        host_path,
        nwos_name,
        size);

    return 1;
}


/* ============================================================
   EXTRACT
   ============================================================ */

static int extract_file(
    const char* image_path,
    const char* nwos_name,
    const char* host_path)
{
    FILE* image;
    FILE* output;

    DirEntry directory[MAX_FILES];

    unsigned char data[SECTOR_SIZE];

    unsigned int size;

    int index;

    unsigned long offset;

    image =
        fopen(
            image_path,
            "rb");

    if (image == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot open image '%s'\n",
            image_path);

        return 0;
    }

    if (!read_directory(
            image,
            directory))
    {
        fprintf(
            stderr,
            "nwimg: cannot read directory.\n");

        fclose(image);
        return 0;
    }

    index =
        find_entry(
            directory,
            nwos_name);

    if (index == -1)
    {
        fprintf(
            stderr,
            "nwimg: file not found: %s\n",
            nwos_name);

        fclose(image);
        return 0;
    }

    size =
        directory[index].size;

    if (size > SECTOR_SIZE)
    {
        fprintf(
            stderr,
            "nwimg: invalid file size in image.\n");

        fclose(image);
        return 0;
    }

    offset =
        ((unsigned long)FS_DATA_LBA +
         (unsigned long)index)
        * SECTOR_SIZE;

    if (fseek(
            image,
            (long)offset,
            SEEK_SET) != 0)
    {
        fclose(image);
        return 0;
    }

    if (fread(
            data,
            1,
            SECTOR_SIZE,
            image) != SECTOR_SIZE)
    {
        fprintf(
            stderr,
            "nwimg: cannot read data sector.\n");

        fclose(image);
        return 0;
    }

    fclose(image);

    output =
        fopen(
            host_path,
            "wb");

    if (output == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot create '%s'\n",
            host_path);

        return 0;
    }

    if (fwrite(
            data,
            1,
            size,
            output) != size)
    {
        fprintf(
            stderr,
            "nwimg: write failed.\n");

        fclose(output);
        return 0;
    }

    fclose(output);

    printf(
        "Extracted '%s' -> '%s' (%u bytes)\n",
        nwos_name,
        host_path,
        size);

    return 1;
}


/* ============================================================
   LIST
   ============================================================ */

static int list_files(
    const char* image_path)
{
    FILE* image;

    DirEntry directory[MAX_FILES];

    int i;
    int found = 0;

    image =
        fopen(
            image_path,
            "rb");

    if (image == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot open image '%s'\n",
            image_path);

        return 0;
    }

    if (!read_directory(
            image,
            directory))
    {
        fprintf(
            stderr,
            "nwimg: cannot read directory.\n");

        fclose(image);
        return 0;
    }

    fclose(image);

    printf(
        "Files in %s:\n\n",
        image_path);

    for (i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used == 1)
        {
            printf(
                "  %-32s %u bytes\n",
                directory[i].name,
                directory[i].size);

            found = 1;
        }
    }

    if (!found)
        printf("  No files.\n");

    return 1;
}


/* ============================================================
   DELETE
   ============================================================ */

static int delete_file(
    const char* image_path,
    const char* nwos_name)
{
    FILE* image;

    DirEntry directory[MAX_FILES];

    int index;

    unsigned char empty[SECTOR_SIZE];

    unsigned long offset;

    image =
        fopen(
            image_path,
            "rb+");

    if (image == NULL)
    {
        fprintf(
            stderr,
            "nwimg: cannot open image '%s'\n",
            image_path);

        return 0;
    }

    if (!read_directory(
            image,
            directory))
    {
        fclose(image);
        return 0;
    }

    index =
        find_entry(
            directory,
            nwos_name);

    if (index == -1)
    {
        fprintf(
            stderr,
            "nwimg: file not found: %s\n",
            nwos_name);

        fclose(image);
        return 0;
    }

    /*
     * Clear data sector.
     */
    memset(
        empty,
        0,
        sizeof(empty));

    offset =
        ((unsigned long)FS_DATA_LBA +
         (unsigned long)index)
        * SECTOR_SIZE;

    if (fseek(
            image,
            (long)offset,
            SEEK_SET) != 0)
    {
        fclose(image);
        return 0;
    }

    fwrite(
        empty,
        1,
        SECTOR_SIZE,
        image);

    /*
     * Clear directory entry.
     */
    memset(
        &directory[index],
        0,
        sizeof(DirEntry));

    if (!write_directory(
            image,
            directory))
    {
        fclose(image);
        return 0;
    }

    fclose(image);

    printf(
        "Deleted '%s'\n",
        nwos_name);

    return 1;
}


/* ============================================================
   Help
   ============================================================ */

static void usage(void)
{
    printf("\n");
    printf("NwOS Image Tool\n");
    printf("\n");

    printf("Usage:\n");

    printf(
        "  nwimg add <image> <host> <NwOS name>\n");

    printf(
        "  nwimg extract <image> <NwOS name> <host>\n");

    printf(
        "  nwimg list <image>\n");

    printf(
        "  nwimg delete <image> <NwOS name>\n");

    printf("\n");

    printf("Examples:\n");

    printf(
        "  nwimg add NwOS.img nw/test.nwo NwApps/test.nwo\n");

    printf(
        "  nwimg extract NwOS.img NwApps/test.nwo test.nwo\n");

    printf(
        "  nwimg add NwOS.img nw/test.nw NwDev/test.nw\n");

    printf(
        "  nwimg extract NwOS.img NwDev/test.nw test.nw\n");

    printf(
        "  nwimg list NwOS.img\n");

    printf("\n");
}


/* ============================================================
   Main
   ============================================================ */

int main(
    int argc,
    char** argv)
{
    if (argc < 2)
    {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "add") == 0)
    {
        if (argc != 5)
        {
            usage();
            return 1;
        }

        return add_file(
            argv[2],
            argv[3],
            argv[4]) ? 0 : 1;
    }

    if (strcmp(argv[1], "extract") == 0)
    {
        if (argc != 5)
        {
            usage();
            return 1;
        }

        return extract_file(
            argv[2],
            argv[3],
            argv[4]) ? 0 : 1;
    }

    if (strcmp(argv[1], "list") == 0)
    {
        if (argc != 3)
        {
            usage();
            return 1;
        }

        return list_files(
            argv[2]) ? 0 : 1;
    }

    if (strcmp(argv[1], "delete") == 0)
    {
        if (argc != 4)
        {
            usage();
            return 1;
        }

        return delete_file(
            argv[2],
            argv[3]) ? 0 : 1;
    }

    usage();

    return 1;
}