#include "nwo_loader.h"
int fs_find(const char* name);

void ata_read_sector(
    unsigned int lba,
    unsigned char* buffer);

#define NWO_FS_DATA_LBA      202
#define NWO_FS_SECTOR_SIZE   512

#define NWO_HEADER_SIZE      20

#define NWO_MAGIC_0          'N'
#define NWO_MAGIC_1          'W'
#define NWO_MAGIC_2          'O'
#define NWO_MAGIC_3          'B'

#define NWO_VERSION_1        1

#define NWO_MAX_FILE_SIZE    512
#define NWO_MAX_CODE_SIZE    480

static unsigned char nwo_file_buffer[NWO_MAX_FILE_SIZE];

static unsigned int nwo_read_u32(
    const unsigned char* p)
{
    return
        ((unsigned int)p[0]) |
        ((unsigned int)p[1] << 8) |
        ((unsigned int)p[2] << 16) |
        ((unsigned int)p[3] << 24);
}

int nwo_validate(
    const unsigned char* buffer,
    unsigned int size)
{
    unsigned int code_size;
    unsigned int data_size;
    unsigned int entry;
    unsigned int total_required;

    if (buffer == 0)
        return 0;

    if (size < NWO_HEADER_SIZE)
        return 0;
    if (buffer[0] != NWO_MAGIC_0 ||
        buffer[1] != NWO_MAGIC_1 ||
        buffer[2] != NWO_MAGIC_2 ||
        buffer[3] != NWO_MAGIC_3)
    {
        return 0;
    }
    if (buffer[4] != NWO_VERSION_1)
        return 0;
    code_size = nwo_read_u32(buffer + 8);
    data_size = nwo_read_u32(buffer + 12);
    entry     = nwo_read_u32(buffer + 16);
    if (code_size > NWO_MAX_FILE_SIZE)
        return 0;

    if (data_size > NWO_MAX_FILE_SIZE)
        return 0;

    total_required =
        NWO_HEADER_SIZE +
        code_size +
        data_size;
    if (total_required > size)
        return 0;
    if (code_size == 0)
    {
        if (entry != 0)
            return 0;
    }
    else
    {
        if (entry >= code_size)
            return 0;
    }

    return 1;
}
int nwo_load(
    const char* filename,
    NwoProgram* program)
{
    int index;
    unsigned int file_size;
    unsigned int code_size;
    unsigned int data_size;
    unsigned int entry;

    if (filename == 0 || program == 0)
        return 0;
    program->version    = 0;
    program->flags      = 0;
    program->code_size  = 0;
    program->data_size  = 0;
    program->entry      = 0;
    program->code       = 0;
    program->data       = 0;
    program->total_size = 0;

    index = fs_find(filename);

    if (index == -1)
        return 0;

    ata_read_sector(
        NWO_FS_DATA_LBA + (unsigned int)index,
        nwo_file_buffer);

    file_size = NWO_HEADER_SIZE;

    if (!nwo_validate(
            nwo_file_buffer,
            NWO_MAX_FILE_SIZE))
    {
        return 0;
    }

    program->version = nwo_file_buffer[4];
    program->flags   = nwo_file_buffer[5];

    code_size = nwo_read_u32(
        nwo_file_buffer + 8);

    data_size = nwo_read_u32(
        nwo_file_buffer + 12);

    entry = nwo_read_u32(
        nwo_file_buffer + 16);

    file_size =
        NWO_HEADER_SIZE +
        code_size +
        data_size;

    if (file_size > NWO_MAX_FILE_SIZE)
        return 0;

    program->code =
        nwo_file_buffer + NWO_HEADER_SIZE;

    program->data =
        program->code + code_size;

    program->code_size  = code_size;
    program->data_size  = data_size;
    program->entry      = entry;
    program->total_size = file_size;

    return 1;
}

void nwo_unload(
    NwoProgram* program)
{
    if (program == 0)
        return;

    program->version    = 0;
    program->flags      = 0;
    program->code_size  = 0;
    program->data_size  = 0;
    program->entry      = 0;
    program->code       = 0;
    program->data       = 0;
    program->total_size = 0;
}