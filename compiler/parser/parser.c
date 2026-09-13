#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/*
* File.nw --> File.nwo
* gonna be available in kernel.c
* and also add .lnw (low level lang)
* to make second kernel.c (or just drivers)
* but lnw gonna be trusted only by sustem
* only expert people gonna uset his
* btw it gives pc control
* ----
* COMPILER:
* compile exe
* ./compiler/compilerMain.c
* compile nw
* ./compiler/nwc.exe /nw/file.nw
* now idk btw it just shows info about file and 
*                    not even compile into .nwo
*/


/* =========================================================
   helpers
   ========================================================= */

static void parser_advance(Parser* parser)
{
    parser->previous = parser->current;
    parser->current = lexer_next(parser->lexer);
}


static int parser_is(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}


static int parser_match(Parser* parser, TokenType type)
{
    if (!parser_is(parser, type))
        return 0;

    parser_advance(parser);
    return 1;
}


static void parser_error(Parser* parser, const char* message)
{
    fprintf(
        stderr,
        "NwC parser error at %d:%d: %s\n",
        parser->current.line,
        parser->current.column,
        message
    );

    parser->error_count++;
}


static void parser_expect(Parser* parser, TokenType type)
{
    if (!parser_match(parser, type))
    {
        fprintf(
            stderr,
            "NwC parser error at %d:%d: expected %s, got %s\n",
            parser->current.line,
            parser->current.column,
            token_type_name(type),
            token_type_name(parser->current.type)
        );

        parser->error_count++;
    }
}

/* =========================================================
   AST allocation
   ========================================================= */

static ASTNode* ast_new(ASTNodeType type, int line, int column)
{
    ASTNode* node;

    node = (ASTNode*)calloc(1, sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    node->type = type;
    node->line = line;
    node->column = column;

    node->value_type = TYPE_UNKNOWN;

    return node;
}


static void ast_append(ASTNode** first, ASTNode** last, ASTNode* node)
{
    if (node == NULL)
        return;

    if (*first == NULL)
    {
        *first = node;
        *last = node;
        return;
    }

    (*last)->next = node;
    *last = node;
}


static char* copy_text(const char* text)
{
    size_t length;
    char* result;

    if (text == NULL)
        return NULL;

    length = strlen(text);

    result = (char*)malloc(length + 1);

    if (result == NULL)
        return NULL;

    memcpy(result, text, length + 1);

    return result;
}


/* =========================================================
   Type parsing
   ========================================================= */

static ValueType parse_type(Parser* parser)
{
    /*
     * 1A (int)
     * writes like 1A
     */

    if (parser_match(parser, TOKEN_INT_TYPE))
        return TYPE_INT;

    /*
     * flt (float, good for calculators)
     */

    if (parser_match(parser, TOKEN_FLOAT_TYPE))
        return TYPE_FLOAT;

    /*
     * bol (bool true or false)
     */

    if (parser_match(parser, TOKEN_BOOL_TYPE))
        return TYPE_BOOL;

    /*
     * lng (long in c++)
     */

    if (parser_match(parser, TOKEN_LONG_TYPE))
    {
        /*
         * Support:
         *
         * lng value;
         * lng lng value;
         */

        if (parser_is(parser, TOKEN_LONG_TYPE))
        {
            parser_advance(parser);
            return TYPE_LONG_LONG;
        }

        return TYPE_LONG;
    }

    /*
     * char (like in c language)
     */

    if (parser_match(parser, TOKEN_CHAR_TYPE))
        return TYPE_CHAR;

    /*
     * str (like string in c++)
     */

    if (parser_match(parser, TOKEN_STRING_TYPE))
        return TYPE_STRING;

    return TYPE_UNKNOWN;
}


/* =========================================================
   Primary expressions
   ========================================================= */

static ASTNode* parse_expression(Parser* parser);


static ASTNode* parse_primary(Parser* parser)
{
    ASTNode* node;

    /*
     * number
     */

    if (parser_is(parser, TOKEN_NUMBER))
    {
        node = ast_new(
            AST_NUMBER,
            parser->current.line,
            parser->current.column
        );

        node->integer_value =
            strtoll(parser->current.text, NULL, 10);

        node->value_type = TYPE_INT;

        parser_advance(parser);

        return node;
    }


    /*
     * float
     */

    if (parser_is(parser, TOKEN_FLOAT))
    {
        node = ast_new(
            AST_FLOAT,
            parser->current.line,
            parser->current.column
        );

        node->float_value =
            strtod(parser->current.text, NULL);

        node->value_type = TYPE_FLOAT;

        parser_advance(parser);

        return node;
    }


    /*
     * Str
     */

    if (parser_is(parser, TOKEN_STRING))
    {
        node = ast_new(
            AST_STRING,
            parser->current.line,
            parser->current.column
        );

        node->text = copy_text(parser->current.text);
        node->value_type = TYPE_STRING;

        parser_advance(parser);

        return node;
    }


    /*
     * Character
     */

    if (parser_is(parser, TOKEN_CHAR))
    {
        node = ast_new(
            AST_CHAR,
            parser->current.line,
            parser->current.column
        );

        node->text = copy_text(parser->current.text);
        node->value_type = TYPE_CHAR;

        parser_advance(parser);

        return node;
    }


    /*
     * true
     */

    if (parser_match(parser, TOKEN_TRUE))
    {
        node = ast_new(
            AST_BOOLEAN,
            parser->previous.line,
            parser->previous.column
        );

        node->integer_value = 1;
        node->value_type = TYPE_BOOL;

        return node;
    }


    /*
     * false
     */

    if (parser_match(parser, TOKEN_FALSE))
    {
        node = ast_new(
            AST_BOOLEAN,
            parser->previous.line,
            parser->previous.column
        );

        node->integer_value = 0;
        node->value_type = TYPE_BOOL;

        return node;
    }


    /*
     * Identifier
     */

    if (parser_is(parser, TOKEN_IDENTIFIER))
    {
        node = ast_new(
            AST_IDENTIFIER,
            parser->current.line,
            parser->current.column
        );

        node->text = copy_text(parser->current.text);

        parser_advance(parser);

        return node;
    }


    /*
     * Parenthesized expression
     */

    if (parser_match(parser, TOKEN_LPAREN))
    {
        node = parse_expression(parser);

        parser_expect(parser, TOKEN_RPAREN);

        return node;
    }


    parser_error(parser, "expected expression");

    return NULL;
}


/* =========================================================
   binary expressions
   ========================================================= */

static ASTNode* parse_expression(Parser* parser)
{
    ASTNode* left;
    ASTNode* right;
    ASTNode* operation;

    left = parse_primary(parser);

    if (left == NULL)
        return NULL;


    while (
        parser_is(parser, TOKEN_PLUS) ||
        parser_is(parser, TOKEN_MINUS) ||
        parser_is(parser, TOKEN_STAR) ||
        parser_is(parser, TOKEN_SLASH) ||
        parser_is(parser, TOKEN_EQUAL_EQUAL) ||
        parser_is(parser, TOKEN_NOT_EQUAL) ||
        parser_is(parser, TOKEN_LESS) ||
        parser_is(parser, TOKEN_GREATER) ||
        parser_is(parser, TOKEN_LESS_EQUAL) ||
        parser_is(parser, TOKEN_GREATER_EQUAL)
    )
    {
        TokenType operator_type;

        operator_type = parser->current.type;

        operation = ast_new(
            AST_BINARY_OPERATION,
            parser->current.line,
            parser->current.column
        );

        operation->text =
            copy_text(parser->current.text);

        parser_advance(parser);

        right = parse_primary(parser);

        operation->left = left;
        operation->right = right;

        left = operation;
    }

    return left;
}


/* =========================================================
   Include
   ========================================================= */

static ASTNode* parse_include(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_INCLUDE);

    node = ast_new(
        AST_INCLUDE,
        line,
        column
    );

    if (parser_is(parser, TOKEN_HEADER))
    {
        node->text = copy_text(parser->current.text);
        parser_advance(parser);
    }
    else
    {
        parser_error(
            parser,
            "expected header after #include"
        );
    }

    return node;
}


/* =========================================================
   Variable declaration
   ========================================================= */

static ASTNode* parse_variable(Parser* parser)
{
    ASTNode* node;

    ValueType type;

    int line = parser->current.line;
    int column = parser->current.column;

    type = parse_type(parser);

    if (type == TYPE_UNKNOWN)
        return NULL;


    /*
     * Variable name
     */

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(
            parser,
            "expected variable name"
        );

        return NULL;
    }


    node = ast_new(
        AST_VARIABLE_DECLARATION,
        line,
        column
    );

    node->value_type = type;

    node->text =
        copy_text(parser->current.text);

    parser_advance(parser);


    /*
     * Array:
     *
     * char text[4];
     */

    if (parser_match(parser, TOKEN_LBRACKET))
    {
        ASTNode* size_node;

        node->type = AST_ARRAY_DECLARATION;

        size_node = parse_primary(parser);

        if (size_node != NULL &&
            size_node->type == AST_NUMBER)
        {
            node->array_size =
                (unsigned int)size_node->integer_value;
        }

        ast_free(size_node);

        parser_expect(
            parser,
            TOKEN_RBRACKET
        );
    }


    /*
     * Optional initializer
     *
     * 1A (int) x = 10;
     * str name = "Hello";
     */

    if (parser_match(parser, TOKEN_ASSIGN))
    {
        node->child = parse_expression(parser);
    }


    parser_expect(
        parser,
        TOKEN_SEMICOLON
    );

    return node;
}


/* =========================================================
   nw::endl
   ========================================================= */

/*like end of string*/

static int parse_nw_endl(Parser* parser)
{
    if (!parser_is(parser, TOKEN_NAMESPACE))
        return 0;

    /*
     * save current state
     */

    Token saved_namespace = parser->current;

    parser_advance(parser);

    if (!parser_match(parser, TOKEN_SCOPE))
    {
        parser->current = saved_namespace;
        return 0;
    }

    if (!parser_match(parser, TOKEN_ENDL))
    {
        return 0;
    }

    return 1;
}


/* =========================================================
   nw::out
   ========================================================= */

static ASTNode* parse_output(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    int line = parser->current.line;
    int column = parser->current.column;

    /*
     * nw
     */

    parser_expect(parser, TOKEN_NAMESPACE);

    parser_expect(parser, TOKEN_SCOPE);

    /*
     * out
     */

    parser_expect(parser, TOKEN_OUT);

    node = ast_new(
        AST_OUTPUT,
        line,
        column
    );


    /*
     * nw::out << expression
     */

    while (parser_match(parser, TOKEN_SHIFT_LEFT))
    {
        ASTNode* expression;

        if (parse_nw_endl(parser))
        {
            ASTNode* endl_node;

            endl_node = ast_new(
                AST_EXPRESSION,
                parser->previous.line,
                parser->previous.column
            );

            endl_node->text =
                copy_text("nw::endl");

            ast_append(
                &first,
                &last,
                endl_node
            );

            continue;
        }

        expression = parse_expression(parser);

        ast_append(
            &first,
            &last,
            expression
        );
    }

    node->child = first;

    parser_expect(
        parser,
        TOKEN_SEMICOLON
    );

    return node;
}


/* =========================================================
   nw::cin
   ========================================================= */

static ASTNode* parse_input(Parser* parser)
{
    ASTNode* node;
    ASTNode* expression;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_NAMESPACE);
    parser_expect(parser, TOKEN_SCOPE);
    parser_expect(parser, TOKEN_CIN);

    node = ast_new(
        AST_INPUT,
        line,
        column
    );

    parser_expect(
        parser,
        TOKEN_SHIFT_RIGHT
    );

    expression = parse_expression(parser);

    node->child = expression;

    parser_expect(
        parser,
        TOKEN_SEMICOLON
    );

    return node;
}


/* =========================================================
   nw::line(nw::cin, variable)
   ========================================================= */

static ASTNode* parse_line_input(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_NAMESPACE);
    parser_expect(parser, TOKEN_SCOPE);
    parser_expect(parser, TOKEN_LINE);

    node = ast_new(
        AST_LINE_INPUT,
        line,
        column
    );

    parser_expect(parser, TOKEN_LPAREN);

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* argument;

        argument = parse_expression(parser);

        ast_append(
            &first,
            &last,
            argument
        );

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }

    parser_expect(
        parser,
        TOKEN_RPAREN
    );

    parser_expect(
        parser,
        TOKEN_SEMICOLON
    );

    node->child = first;

    return node;
}


/* =========================================================
   Return
   ========================================================= */

static ASTNode* parse_return(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_RETURN);

    node = ast_new(
        AST_RETURN,
        line,
        column
    );

    if (!parser_is(parser, TOKEN_SEMICOLON))
    {
        node->child = parse_expression(parser);
    }

    parser_expect(
        parser,
        TOKEN_SEMICOLON
    );

    return node;
}

static ASTNode* parse_nw_statement(Parser* parser)
{
    if (!parser_is(parser, TOKEN_NAMESPACE))
    {
        parser_error(
            parser,
            "expected namespace"
        );

        return NULL;
    }

    int line = parser->current.line;
    int column = parser->current.column;

    parser_advance(parser);

    parser_expect(parser, TOKEN_SCOPE);


    /*
     * =====================================================
     * nw::out
     * =====================================================
     */

    if (parser_is(parser, TOKEN_OUT))
    {
        ASTNode* node;
        ASTNode* first = NULL;
        ASTNode* last = NULL;

        parser_advance(parser);

        node = ast_new(
            AST_OUTPUT,
            line,
            column
        );

        while (parser_match(
            parser,
            TOKEN_SHIFT_LEFT))
        {
            ASTNode* expression;

            /*
             * nw::endl
             */

            if (parser_is(parser, TOKEN_NAMESPACE))
            {
                Token saved_lexer_current =
                    parser->current;

                /*
                 * parse
                 *
                 * nw :: endl
                 */

                parser_advance(parser);

                if (parser_is(parser, TOKEN_SCOPE))
                {
                    parser_advance(parser);

                    if (parser_is(parser, TOKEN_ENDL))
                    {
                        ASTNode* endl_node;

                        endl_node = ast_new(
                            AST_EXPRESSION,
                            parser->current.line,
                            parser->current.column
                        );

                        endl_node->text =
                            copy_text("nw::endl");

                        parser_advance(parser);

                        ast_append(
                            &first,
                            &last,
                            endl_node
                        );

                        continue;
                    }
                }

                /*
                 * not endl
                 *
                 * restore only current token
                 */

                parser->current =
                    saved_lexer_current;
            }

            expression = parse_expression(parser);

            ast_append(
                &first,
                &last,
                expression
            );
        }

        node->child = first;

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    /*
     * =====================================================
     * nw::cin
     * =====================================================
     */

    if (parser_is(parser, TOKEN_CIN))
    {
        ASTNode* node;

        parser_advance(parser);

        node = ast_new(
            AST_INPUT,
            line,
            column
        );

        parser_expect(
            parser,
            TOKEN_SHIFT_RIGHT
        );

        node->child =
            parse_expression(parser);

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    /*
     * =====================================================
     * nw::line(...)
     * =====================================================
     */

    if (parser_is(parser, TOKEN_LINE))
    {
        ASTNode* node;
        ASTNode* first = NULL;
        ASTNode* last = NULL;

        parser_advance(parser);

        node = ast_new(
            AST_LINE_INPUT,
            line,
            column
        );

        parser_expect(
            parser,
            TOKEN_LPAREN
        );

        while (
            !parser_is(parser, TOKEN_RPAREN) &&
            !parser_is(parser, TOKEN_EOF)
        )
        {
            ASTNode* argument;

            /*
             * nw::cin inside nw::line()
             */

            if (parser_is(parser, TOKEN_NAMESPACE))
            {
                /*
                 * currently support:
                 *
                 * nw::line(nw::cin, name)
                 *
                 * for now represent nw::cin
                 * as special expression
                 */

                parser_advance(parser);
                parser_expect(
                    parser,
                    TOKEN_SCOPE
                );

                if (parser_is(parser, TOKEN_CIN))
                {
                    argument = ast_new(
                        AST_EXPRESSION,
                        parser->current.line,
                        parser->current.column
                    );

                    argument->text =
                        copy_text("nw::cin");

                    parser_advance(parser);
                }
                else
                {
                    parser_error(
                        parser,
                        "expected cin after nw::"
                    );

                    argument = NULL;
                }
            }
            else
            {
                argument =
                    parse_expression(parser);
            }

            ast_append(
                &first,
                &last,
                argument
            );

            if (!parser_match(
                parser,
                TOKEN_COMMA))
            {
                break;
            }
        }

        parser_expect(
            parser,
            TOKEN_RPAREN
        );

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        node->child = first;

        return node;
    }


    /*
     * =====================================================
     * nw::time(...)
     * =====================================================
     */

    if (parser_is(parser, TOKEN_TIME))
    {
        ASTNode* node;

        parser_advance(parser);

        node = ast_new(
            AST_CALL,
            line,
            column
        );

        node->text =
            copy_text("nw::time");

        parser_expect(
            parser,
            TOKEN_LPAREN
        );

        node->child =
            parse_expression(parser);

        parser_expect(
            parser,
            TOKEN_RPAREN
        );

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    /*
     * =====================================================
     * nw::color(...)
     * =====================================================
     */

    if (parser_is(parser, TOKEN_COLOR))
    {
        ASTNode* node;

        parser_advance(parser);

        node = ast_new(
            AST_CALL,
            line,
            column
        );

        node->text =
            copy_text("nw::color");

        parser_expect(
            parser,
            TOKEN_LPAREN
        );

        node->child =
            parse_expression(parser);

        parser_expect(
            parser,
            TOKEN_RPAREN
        );

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    /*
     * =====================================================
     * nw::getkey()
     * =====================================================
     */

    if (parser_is(parser, TOKEN_GETKEY))
    {
        ASTNode* node;

        parser_advance(parser);

        node = ast_new(
            AST_CALL,
            line,
            column
        );

        node->text =
            copy_text("nw::getkey");

        parser_expect(
            parser,
            TOKEN_LPAREN
        );

        parser_expect(
            parser,
            TOKEN_RPAREN
        );

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    /*
     * =====================================================
     * nw::clear()
     * =====================================================
     */

    if (parser_is(parser, TOKEN_CLEAR))
    {
        ASTNode* node;

        parser_advance(parser);

        node = ast_new(
            AST_CALL,
            line,
            column
        );

        node->text =
            copy_text("nw::clear");

        parser_expect(
            parser,
            TOKEN_LPAREN
        );

        parser_expect(
            parser,
            TOKEN_RPAREN
        );

        parser_expect(
            parser,
            TOKEN_SEMICOLON
        );

        return node;
    }


    parser_error(
        parser,
        "unknown nw command"
    );

    return NULL;
}


/* =========================================================
   Statements
   ========================================================= */

static ASTNode* parse_statement(Parser* parser)
{
    /*
     * var
     */

    if (
        parser_is(parser, TOKEN_INT_TYPE) ||
        parser_is(parser, TOKEN_FLOAT_TYPE) ||
        parser_is(parser, TOKEN_BOOL_TYPE) ||
        parser_is(parser, TOKEN_LONG_TYPE) ||
        parser_is(parser, TOKEN_CHAR_TYPE) ||
        parser_is(parser, TOKEN_STRING_TYPE)
    )
    {
        return parse_variable(parser);
    }


    /*
     * return
     */

    if (parser_is(parser, TOKEN_RETURN))
    {
        return parse_return(parser);
    }


    /*
     * nw::
     */

    if (parser_is(parser, TOKEN_NAMESPACE))
    {
        return parse_nw_statement(parser);
    }


    parser_error(
        parser,
        "unknown statement"
    );

    parser_advance(parser);

    return NULL;
}


/* =========================================================
   block
   ========================================================= */

static ASTNode* parse_block(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(
        parser,
        TOKEN_LBRACE
    );

    node = ast_new(
        AST_BLOCK,
        line,
        column
    );


    while (!parser_is(parser, TOKEN_RBRACE) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* statement;

        statement = parse_statement(parser);

        ast_append(
            &first,
            &last,
            statement
        );
    }


    parser_expect(
        parser,
        TOKEN_RBRACE
    );

    node->child = first;

    return node;
}


/* =========================================================
   function
   ========================================================= */

static ASTNode* parse_function(Parser* parser)
{
    ASTNode* node;

    ValueType return_type;

    int line = parser->current.line;
    int column = parser->current.column;


    /*
     * return type
     *
     * for now:
     *
     * 1A main()
     *
     * and
     *
     * void foo()
     */

    if (parser_is(parser, TOKEN_INT_TYPE))
    {
        return_type = TYPE_INT;
        parser_advance(parser);
    }
    else if (
        parser_is(parser, TOKEN_IDENTIFIER) &&
        strcmp(parser->current.text, "void") == 0
    )
    {
        return_type = TYPE_VOID;
        parser_advance(parser);
    }
    else
    {
        parser_error(
            parser,
            "expected function return type"
        );

        return NULL;
    }


    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(
            parser,
            "expected function name"
        );

        return NULL;
    }


    node = ast_new(
        AST_FUNCTION,
        line,
        column
    );

    node->value_type = return_type;

    node->text =
        copy_text(parser->current.text);

    parser_advance(parser);


    /*
     * parameters
     */

    parser_expect(
        parser,
        TOKEN_LPAREN
    );


    /*
     * for now accept an empty parameter list.
     *
     * i add:
     *
     * 1A a, 1A b
     *
     * later.
     */

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        parser_advance(parser);

        if (parser_is(parser, TOKEN_COMMA))
            parser_advance(parser);
    }


    parser_expect(
        parser,
        TOKEN_RPAREN
    );


    /*
     * func body
     */

    node->child =
        parse_block(parser);

    return node;
}


/* =========================================================
   program
   ========================================================= */

static ASTNode* parse_program(Parser* parser)
{
    ASTNode* program;

    ASTNode* first = NULL;
    ASTNode* last = NULL;

    program = ast_new(
        AST_PROGRAM,
        1,
        1
    );


    while (!parser_is(parser, TOKEN_EOF))
    {
        ASTNode* node;

        /*
         * #include
         */

        if (parser_is(parser, TOKEN_INCLUDE))
        {
            node = parse_include(parser);
        }

        /*
         * func
         *
         * 1A main()
         */

        else if (
            parser_is(parser, TOKEN_INT_TYPE) ||
            (
                parser_is(parser, TOKEN_IDENTIFIER) &&
                strcmp(parser->current.text, "void") == 0
            )
        )
        {
            node = parse_function(parser);
        }

        else
        {
            parser_error(
                parser,
                "expected include or function"
            );

            parser_advance(parser);

            continue;
        }


        ast_append(
            &first,
            &last,
            node
        );
    }


    program->child = first;

    return program;
}


/* =========================================================
   parser initialization
   ========================================================= */

void parser_init(Parser* parser, Lexer* lexer)
{
    parser->lexer = lexer;

    parser->error_count = 0;

    parser->current.type = TOKEN_EOF;
    parser->previous.type = TOKEN_EOF;

    parser_advance(parser);
}


/* =========================================================
   public parser
   ========================================================= */

ASTNode* parser_parse(Parser* parser)
{
    return parse_program(parser);
}


/* =========================================================
   ast free
   ========================================================= */

void ast_free(ASTNode* node)
{
    ASTNode* next;

    if (node == NULL)
        return;

    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->child);

    next = node->next;

    free(node->text);
    free(node);

    ast_free(next);
}


/* =========================================================
   ast debug output
   ========================================================= */

static const char* ast_type_name(ASTNodeType type)
{
    switch (type)
    {
        case AST_PROGRAM:
            return "PROGRAM";

        case AST_INCLUDE:
            return "INCLUDE";

        case AST_FUNCTION:
            return "FUNCTION";

        case AST_BLOCK:
            return "BLOCK";

        case AST_VARIABLE_DECLARATION:
            return "VARIABLE";

        case AST_ARRAY_DECLARATION:
            return "ARRAY";

        case AST_RETURN:
            return "RETURN";

        case AST_OUTPUT:
            return "OUTPUT";

        case AST_INPUT:
            return "INPUT";

        case AST_LINE_INPUT:
            return "LINE_INPUT";

        case AST_EXPRESSION:
            return "EXPRESSION";

        case AST_NUMBER:
            return "NUMBER";

        case AST_FLOAT:
            return "FLOAT";

        case AST_STRING:
            return "STRING";

        case AST_CHAR:
            return "CHAR";

        case AST_BOOLEAN:
            return "BOOLEAN";

        case AST_IDENTIFIER:
            return "IDENTIFIER";

        case AST_BINARY_OPERATION:
            return "BINARY_OPERATION";

        case AST_CALL:
            return "CALL";

        default:
            return "UNKNOWN";
    }
}


static void ast_print_indent(int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");
}

void ast_print(ASTNode* node, int depth)
{
    while (node != NULL)
    {
        ast_print_indent(depth);

        printf(
            "%s",
            ast_type_name(node->type)
        );

        if (node->text != NULL)
        {
            printf(
                " [%s]",
                node->text
            );
        }

        if (node->type == AST_NUMBER)
        {
            printf(
                " = %lld",
                node->integer_value
            );
        }

        if (node->type == AST_FLOAT)
        {
            printf(
                " = %f",
                node->float_value
            );
        }

        if (node->type == AST_ARRAY_DECLARATION)
        {
            printf(
                " [%u]",
                node->array_size
            );
        }

        printf("\n");

        if (node->child != NULL)
            ast_print(node->child, depth + 1);

        if (node->left != NULL)
            ast_print(node->left, depth + 1);

        if (node->right != NULL)
            ast_print(node->right, depth + 1);

        node = node->next;
    }
}