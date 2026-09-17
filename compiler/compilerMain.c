#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"


static char* read_file(const char* path)
{
    FILE* file;
    long size;
    char* buffer;

    file = fopen(path, "rb");

    if (file == NULL)
    {
        fprintf(stderr,
                "NwC: cannot open '%s'\n",
                path);

        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    if (size < 0)
    {
        fprintf(stderr,
                "NwC: invalid file size\n");

        fclose(file);
        return NULL;
    }

    buffer = malloc((size_t)size + 1);

    if (buffer == NULL)
    {
        fprintf(stderr,
                "NwC: out of memory\n");

        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, (size_t)size, file)
        != (size_t)size)
    {
        fprintf(stderr,
                "NwC: failed to read '%s'\n",
                path);

        free(buffer);
        fclose(file);

        return NULL;
    }

    buffer[size] = '\0';

    fclose(file);

    return buffer;
}


static void print_usage(void)
{
    printf("NwC 0.1 - NwOS Compiler\n\n");
    printf("Test compiler made as 'second kernel', it too simple to lear\n");

    printf("Usage:\n");
    printf("  nwc <input.nw>\n");
    printf("  nwc <input.nw> -o <output.nwo>\n");
    printf("\n");

    printf("Examples:\n");
    printf("  nwc nw\\game.nw\n");
    printf("  nwc nw\\game.nw -o sys\\game.nwo\n");
}


int main(int argc, char** argv)
{
    const char* input_path;
    const char* output_path = NULL;

    char default_output[512];

    char* source;

    Lexer lexer;
    Parser parser;

    ASTNode* ast;

    if (argc < 2)
    {
        print_usage();
        return 1;
    }

    input_path = argv[1];


    if (argc >= 4 &&
        strcmp(argv[2], "-o") == 0)
    {
        output_path = argv[3];
    }

    if (output_path == NULL)
    {
        const char* dot;

        dot = strrchr(input_path, '.');

        if (dot != NULL)
        {
            size_t base_length =
                (size_t)(dot - input_path);

            if (base_length >=
                sizeof(default_output) - 5)
            {
                fprintf(stderr,
                        "NwC: path too long\n");

                return 1;
            }

            memcpy(
                default_output,
                input_path,
                base_length
            );

            strcpy(
                default_output + base_length,
                ".nwo"
            );

            output_path = default_output;
        }
        else
        {
            snprintf(
                default_output,
                sizeof(default_output),
                "%s.nwo",
                input_path
            );

            output_path = default_output;
        }
    }
    printf("NwC 0.1\n");
    printf("Input : %s\n", input_path);
    printf("Output: %s\n\n", output_path);

    printf("Lexing...   ");

    source = read_file(input_path);

    if (source == NULL)
        return 1;

    printf("OK\n");

    printf("Parsing...  ");

    lexer_init(
        &lexer,
        source
    );

    parser_init(
        &parser,
        &lexer
    );

    ast = parser_parse(
        &parser
    );

    if (ast == NULL ||
        parser.error_count != 0)
    {
        printf("FAILED\n");

        ast_free(ast);
        free(source);

        return 1;
    }

    printf("OK\n");

    printf("Codegen...   ");

    if (!codegen_write_nwo(
            ast,
            output_path,
            0))
    {
        printf("FAILED\n");

        ast_free(ast);
        free(source);

        return 1;
    }

    printf("OK\n");

    printf("\nCreated: %s\n",
           output_path);

    ast_free(ast);
    free(source);

    return 0;
}