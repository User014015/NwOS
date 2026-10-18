#ifndef NWC_CODEGEN_H
#define NWC_CODEGEN_H

#include "../parser/parser.h"

int codegen_write_nwo(ASTNode* ast, const char* path, int flags);

#endif