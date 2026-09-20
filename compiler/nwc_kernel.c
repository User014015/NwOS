#include "nwc_kernel.h"
#include "nwc_runtime.h"

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

extern void print(const char* text);
extern void print_int(int number);
extern void putchar_os(char c);

static char source_buffer[NWC_MAX_SOURCE_SIZE + 1];

int nwc_compile_source(
    const char* source,
    unsigned int source_size,
    unsigned char* nwo_buffer,
    unsigned int nwo_capacity,
    unsigned int* nwo_size)
{
    Lexer lexer;
    Parser parser;
    ASTNode* ast;

    if (nwo_size != 0)
        *nwo_size = 0;

    if (source == 0 || nwo_buffer == 0)
    {
        print("NwC: invalid compiler arguments.\n");
        return 0;
    }

    if (source_size > NWC_MAX_SOURCE_SIZE)
    {
        print("NwC: source is too large for the in-kernel compiler.\n");
        return 0;
    }

    nwc_arena_reset();

    memcpy(source_buffer, source, source_size);
    source_buffer[source_size] = '\0';

    lexer_init(&lexer, source_buffer);
    parser_init(&parser, &lexer);

    ast = parser_parse(&parser);

    if (ast == 0 || parser.error_count != 0)
    {
        print("NwC: parsing failed.\n");
        ast_free(ast);
        nwc_arena_reset();
        return 0;
    }

    codegen_begin_output(
        nwo_buffer,
        nwo_capacity,
        nwo_size
    );

    if (!codegen_write_nwo(
            ast,
            "memory.nwo",
            NWO_FLAG_NORMAL))
    {
        print("NwC: code generation failed.\n");
        ast_free(ast);
        nwc_arena_reset();
        return 0;
    }

    ast_free(ast);
    nwc_arena_reset();

    return 1;
}
