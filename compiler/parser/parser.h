#ifndef NWC_PARSER_H
#define NWC_PARSER_H

#include "../lexer/lexer.h"


/* =========================================================
   AST node types
   ========================================================= */

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


/* =========================================================
   Variable types
   ========================================================= */

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


/* =========================================================
   AST node
   ========================================================= */

typedef struct ASTNode
{
    ASTNodeType type;

    int line;
    int column;

    /*
     * Generic text storage.
     *
     * Examples:
     *   variable name
     *   function name
     *   string contents
     *   include name
     */

    char* text;

    /*
     * Numeric values
     */

    long long integer_value;
    double float_value;

    /*
     * Type information
     */

    ValueType value_type;

    /*
     * Array size
     */

    unsigned int array_size;

    /*
     * Child nodes
     */

    struct ASTNode* left;
    struct ASTNode* right;

    struct ASTNode* child;

    struct ASTNode* next;

} ASTNode;


/* =========================================================
   Parser
   ========================================================= */

typedef struct
{
    Lexer* lexer;

    Token current;
    Token previous;

    int error_count;

} Parser;


/* =========================================================
   API
   ========================================================= */

void parser_init(Parser* parser, Lexer* lexer);

ASTNode* parser_parse(Parser* parser);

void ast_free(ASTNode* node);

void ast_print(ASTNode* node, int depth);

#endif