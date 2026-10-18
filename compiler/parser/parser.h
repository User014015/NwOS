#ifndef NWC_PARSER_H
#define NWC_PARSER_H

#include "../lexer/lexer.h"

typedef enum
{
    AST_PROGRAM = 0,
    AST_INCLUDE,

    AST_FUNCTION,
    AST_BLOCK,

    AST_VARIABLE_DECLARATION,
    AST_ARRAY_DECLARATION,

    AST_RETURN,

    AST_OUTPUT,
    AST_INPUT,
    AST_LINE_INPUT,

    AST_EXPRESSION,

    AST_NUMBER,
    AST_FLOAT,
    AST_STRING,
    AST_CHAR,
    AST_BOOLEAN,
    AST_IDENTIFIER,

    AST_BINARY_OPERATION,

    AST_CALL

} ASTNodeType;

typedef enum
{
    TYPE_INT = 0,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_LONG,
    TYPE_LONG_LONG,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_UNKNOWN

} ValueType;

typedef struct ASTNode
{
    ASTNodeType type;

    int line;
    int column;

    char* text;
    long long integer_value;
    double float_value;

    ValueType value_type;

    unsigned int array_size;

    struct ASTNode* left;
    struct ASTNode* right;

    struct ASTNode* child;

    struct ASTNode* next;

} ASTNode;

typedef struct
{
    Lexer* lexer;

    Token current;
    Token previous;

    int error_count;

} Parser;

void parser_init(Parser* parser, Lexer* lexer);

ASTNode* parser_parse(Parser* parser);

void ast_free(ASTNode* node);

void ast_print(ASTNode* node, int depth);

#endif