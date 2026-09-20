#ifndef NWC_CODEGEN_H
#define NWC_CODEGEN_H

#include "parser.h"

#define NWO_FLAG_NORMAL   0x00
#define NWO_FLAG_LOWLEVEL 0x01

void codegen_begin_output(
    unsigned char* buffer,
    unsigned int capacity,
    unsigned int* size_out
);

int codegen_write_nwo(
    ASTNode* ast,
    const char* path,
    int flags
);

#endif
