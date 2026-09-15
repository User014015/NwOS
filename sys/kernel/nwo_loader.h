#ifndef NWO_LOADER_H
#define NWO_LOADER_H

typedef struct
{
    unsigned char version;
    unsigned char flags;

    unsigned int code_size;
    unsigned int data_size;
    unsigned int entry;

    unsigned char* code;
    unsigned char* data;

    unsigned int total_size;

} NwoProgram;

int nwo_load(const char* filename, NwoProgram* program);

void nwo_unload(NwoProgram* program);

int nwo_validate(const unsigned char* buffer,
                 unsigned int size);

#endif