#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define NWC_MOD_NONZERO  1u
#define NWC_MOD_CONST    2u
#define NWC_MOD_NEEDED   4u

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
*
* Update:
* Compiles into .nwo and can be launched
* Run from /bin/ or /sys/
*/

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


static int text_equals_ci(const char* a, const char* b)
{
    int i = 0;

    if (a == NULL || b == NULL)
        return 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'A' && ca <= 'Z')
            ca = (char)(ca - 'A' + 'a');

        if (cb >= 'A' && cb <= 'Z')
            cb = (char)(cb - 'A' + 'a');

        if (ca != cb)
            return 0;

        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}


static int parser_is_int_keyword(Parser* parser)
{
    return parser_is(parser, TOKEN_IDENTIFIER) &&
           text_equals_ci(parser->current.text, "int");
}


static int parser_is_void_keyword(Parser* parser)
{
    return parser_is(parser, TOKEN_IDENTIFIER) &&
           text_equals_ci(parser->current.text, "void");
}


static void parser_error_unsupported_type(Parser* parser, int is_int)
{
    if (is_int)
    {
        parser_error(
            parser,
            "Int is not supported by NwC, Do you mean '1A'?"
        );
    }
    else
    {
        parser_error(
            parser,
            "Void is not supported by NwC, Do you mean '0A'?"
        );
    }
}


static void parser_skip_statement_invalid(Parser* parser)
{
    while (!parser_is(parser, TOKEN_EOF) &&
           !parser_is(parser, TOKEN_SEMICOLON) &&
           !parser_is(parser, TOKEN_RBRACE))
    {
        parser_advance(parser);
    }

    parser_match(parser, TOKEN_SEMICOLON);
}


static void parser_skip_top_level_invalid(Parser* parser)
{
    while (!parser_is(parser, TOKEN_EOF) &&
           !parser_is(parser, TOKEN_INCLUDE) &&
           !parser_is(parser, TOKEN_INT_TYPE) &&
           !parser_is(parser, TOKEN_VOID_TYPE) &&
           !parser_is_int_keyword(parser) &&
           !parser_is_void_keyword(parser))
    {
        parser_advance(parser);
    }
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

static ValueType parse_type(Parser* parser)
{
    if (parser_match(parser, TOKEN_INT_TYPE))
        return TYPE_INT;

    if (parser_match(parser, TOKEN_VOID_TYPE))
        return TYPE_VOID;

    if (parser_match(parser, TOKEN_FLOAT_TYPE))
        return TYPE_FLOAT;

    if (parser_match(parser, TOKEN_BOOL_TYPE))
        return TYPE_BOOL;

    if (parser_match(parser, TOKEN_LONG_TYPE))
    {
        if (parser_is(parser, TOKEN_LONG_TYPE))
        {
            parser_advance(parser);
            return TYPE_LONG_LONG;
        }
        return TYPE_LONG;
    }
    if (parser_match(parser, TOKEN_CHAR_TYPE))
        return TYPE_CHAR;
    if (parser_match(parser, TOKEN_STRING_TYPE))
        return TYPE_STRING;
    return TYPE_UNKNOWN;
}
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_block(Parser* parser);


static ASTNode* parse_primary(Parser* parser)
{
    ASTNode* node;
    if (parser_is(parser, TOKEN_NUMBER))
    {
        node = ast_new(
            AST_NUMBER,
            parser->current.line,
            parser->current.column
        );

        node->integer_value =
            strtoll(parser->current.text, NULL, 0);

        node->value_type = TYPE_INT;

        parser_advance(parser);

        return node;
    }
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

    if (parser_is(parser, TOKEN_NAMESPACE))
    {
        parser_advance(parser);
        parser_expect(parser, TOKEN_SCOPE);

        if (parser_is(parser, TOKEN_GETKEY))
        {
            node = ast_new(
                AST_CALL,
                parser->previous.line,
                parser->previous.column
            );

            node->text = copy_text("nw::getkey");

            parser_advance(parser);

            if (parser_match(parser, TOKEN_LPAREN))
            {
                if (!parser_is(parser, TOKEN_RPAREN))
                    node->child = parse_expression(parser);

                parser_expect(parser, TOKEN_RPAREN);
            }

            return node;
        }

        if (parser_is(parser, TOKEN_IDENTIFIER))
        {
            const char* name = parser->current.text;

            if (strcmp(name, "mouse_x") == 0 ||
                strcmp(name, "mouse_y") == 0 ||
                strcmp(name, "mouse_buttons") == 0)
            {
                char builtin[64];

                snprintf(builtin, sizeof(builtin), "nw::%s", name);

                node = ast_new(
                    AST_CALL,
                    parser->current.line,
                    parser->current.column
                );

                node->text = copy_text(builtin);

                parser_advance(parser);
                parser_expect(parser, TOKEN_LPAREN);
                parser_expect(parser, TOKEN_RPAREN);

                return node;
            }
        }

        parser_error(
            parser,
            "unsupported nw expression"
        );

        return NULL;
    }

    if (parser_is(parser, TOKEN_IDENTIFIER))
    {
        node = ast_new(
            AST_IDENTIFIER,
            parser->current.line,
            parser->current.column
        );

        node->text = copy_text(parser->current.text);

        parser_advance(parser);

        if (parser_match(parser, TOKEN_LPAREN))
        {
            ASTNode* first = NULL;
            ASTNode* last = NULL;

            node->type = AST_CALL;

            while (!parser_is(parser, TOKEN_RPAREN) &&
                   !parser_is(parser, TOKEN_EOF))
            {
                ast_append(
                    &first,
                    &last,
                    parse_expression(parser)
                );

                if (!parser_match(parser, TOKEN_COMMA))
                    break;
            }

            parser_expect(parser, TOKEN_RPAREN);

            node->child = first;
        }

        return node;
    }

    if (parser_match(parser, TOKEN_LPAREN))
    {
        node = parse_expression(parser);

        parser_expect(parser, TOKEN_RPAREN);

        return node;
    }


    parser_error(parser, "expected expression");

    return NULL;
}

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

static int parser_is_modifier(Parser* parser)
{
    return parser_is(parser, TOKEN_NONZERO) ||
           parser_is(parser, TOKEN_CONST) ||
           parser_is(parser, TOKEN_NEEDED);
}

static unsigned int parse_modifiers(Parser* parser)
{
    unsigned int modifiers = 0;

    while (parser_is_modifier(parser))
    {
        if (parser_is(parser, TOKEN_NONZERO))
            modifiers |= NWC_MOD_NONZERO;
        else if (parser_is(parser, TOKEN_CONST))
            modifiers |= NWC_MOD_CONST;
        else if (parser_is(parser, TOKEN_NEEDED))
            modifiers |= NWC_MOD_NEEDED;

        parser_advance(parser);
    }

    return modifiers;
}

static ASTNode* parse_variable(Parser* parser)
{
    ASTNode* node;

    ValueType type;

    int line = parser->current.line;
    int column = parser->current.column;

    unsigned int modifiers = parse_modifiers(parser);

    type = parse_type(parser);

    if (type == TYPE_UNKNOWN)
        return NULL;

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
    node->modifiers = modifiers;

    node->text =
        copy_text(parser->current.text);

    parser_advance(parser);

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

static int parse_nw_endl(Parser* parser)
{
    if (!parser_is(parser, TOKEN_NAMESPACE))
        return 0;

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

static ASTNode* parse_output(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_NAMESPACE);

    parser_expect(parser, TOKEN_SCOPE);
    parser_expect(parser, TOKEN_OUT);

    node = ast_new(
        AST_OUTPUT,
        line,
        column
    );

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

static int parse_color_tail(Parser* parser, char** color_out)
{
    if (!parser_match(parser, TOKEN_LOGICAL_NOT))
    {
        parser_error(
            parser,
            "expected '!' before COLOR"
        );

        return 0;
    }

    if (!(parser_is(parser, TOKEN_IDENTIFIER) &&
          text_equals_ci(parser->current.text, "COLOR")))
    {
        parser_error(
            parser,
            "expected COLOR after '!'"
        );

        return 0;
    }

    parser_advance(parser);

    if (!parser_match(parser, TOKEN_COLON) &&
        !parser_match(parser, TOKEN_ASSIGN))
    {
        parser_error(
            parser,
            "expected ':' or '=' before color name"
        );

        return 0;
    }

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(
            parser,
            "expected color name after ':' or '='"
        );

        return 0;
    }

    free(*color_out);
    *color_out = copy_text(parser->current.text);

    parser_advance(parser);

    return 1;
}


static ASTNode* parse_output_statement(
    Parser* parser,
    int line,
    int column)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;
    int colored = 0;

    parser_expect(parser, TOKEN_OUT);

    node = ast_new(
        AST_OUTPUT,
        line,
        column
    );

    if (parser_match(parser, TOKEN_LOGICAL_NOT))
    {
        if (parser_is(parser, TOKEN_IDENTIFIER) &&
            text_equals_ci(parser->current.text, "COLOR"))
        {
            parser_advance(parser);
            colored = 1;
        }
        else
        {
            parser_error(
                parser,
                "expected COLOR after nw::out!"
            );
        }
    }

    while (parser_match(parser, TOKEN_SHIFT_LEFT))
    {
        ASTNode* expression;

        if (colored && parser_is(parser, TOKEN_NO_ENDL))
        {
            parser_advance(parser);

            parse_color_tail(parser, &node->text);

            node->integer_value = 1;

            break;
        }

        if (parser_is(parser, TOKEN_NAMESPACE))
        {
            Lexer saved_lexer = *parser->lexer;
            Token saved_current = parser->current;
            int is_endl = 0;
            int has_color = 0;

            parser_advance(parser);

            if (parser_match(parser, TOKEN_SCOPE) &&
                parser_match(parser, TOKEN_ENDL))
            {
                is_endl = 1;
            }

            if (is_endl)
            {
                if (parser_is(parser, TOKEN_LOGICAL_NOT))
                    has_color = parse_color_tail(parser, &node->text);

                if (colored)
                {
                    if (!has_color)
                    {
                        parser_error(
                            parser,
                            "colored output requires endl!COLOR:NAME or no_endl!COLOR:NAME"
                        );
                    }

                    break;
                }

                if (has_color)
                {
                    parser_error(
                        parser,
                        "color marker requires nw::out!COLOR"
                    );

                    free(node->text);
                    node->text = NULL;
                }
                else
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
                }

                continue;
            }

            *parser->lexer = saved_lexer;
            parser->current = saved_current;
        }

        expression = parse_expression(parser);

        ast_append(
            &first,
            &last,
            expression
        );
    }

    if (colored && node->text == NULL)
    {
        parser_error(
            parser,
            "colored output requires endl!COLOR:NAME or no_endl!COLOR:NAME"
        );
    }

    node->child = first;

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

    if (parser_is(parser, TOKEN_IDENTIFIER))
    {
        const char* name = parser->current.text;

        if (strcmp(name, "window") == 0 ||
            strcmp(name, "text") == 0)
        {
            char builtin[64];
            ASTNode* node;
            ASTNode* first = NULL;
            ASTNode* last = NULL;

            snprintf(builtin, sizeof(builtin), "nw::%s", name);

            node = ast_new(AST_CALL, line, column);
            node->text = copy_text(builtin);

            parser_advance(parser);
            parser_expect(parser, TOKEN_LPAREN);

            while (!parser_is(parser, TOKEN_RPAREN) &&
                   !parser_is(parser, TOKEN_EOF))
            {
                ast_append(&first, &last, parse_expression(parser));
                if (!parser_match(parser, TOKEN_COMMA))
                    break;
            }

            parser_expect(parser, TOKEN_RPAREN);
            parser_expect(parser, TOKEN_SEMICOLON);
            node->child = first;

            return node;
        }

        if (strcmp(name, "mouse_x") == 0 ||
            strcmp(name, "mouse_y") == 0 ||
            strcmp(name, "mouse_buttons") == 0 ||
            strcmp(name, "poweroff") == 0 ||
            strcmp(name, "reboot") == 0 ||
            strcmp(name, "wallpaper") == 0)
        {
            char builtin[64];
            ASTNode* node;

            snprintf(builtin, sizeof(builtin), "nw::%s", name);

            node = ast_new(AST_CALL, line, column);
            node->text = copy_text(builtin);

            parser_advance(parser);
            parser_expect(parser, TOKEN_LPAREN);
            parser_expect(parser, TOKEN_RPAREN);
            parser_expect(parser, TOKEN_SEMICOLON);

            return node;
        }
    }

    if (parser_is(parser, TOKEN_OUT))
    {
        return parse_output_statement(
            parser,
            line,
            column
        );
    }

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

            if (parser_is(parser, TOKEN_NAMESPACE))
            {
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

        if (!parser_is(parser, TOKEN_RPAREN))
        {
            parser_error(
                parser,
                "nw::time takes no arguments"
            );

            while (!parser_is(parser, TOKEN_RPAREN) &&
                   !parser_is(parser, TOKEN_EOF))
            {
                parser_advance(parser);
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

        return node;
    }

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

        if (!parser_is(parser, TOKEN_RPAREN))
            node->child = parse_expression(parser);

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

    if (parser_is(parser, TOKEN_RECT))
    {
        ASTNode* node;
        ASTNode* first = NULL;
        ASTNode* last = NULL;

        parser_advance(parser);
        node = ast_new(AST_CALL, line, column);
        node->text = copy_text("nw::rect");

        parser_expect(parser, TOKEN_LPAREN);

        while (!parser_is(parser, TOKEN_RPAREN) && !parser_is(parser, TOKEN_EOF)) {
            ast_append(&first, &last, parse_expression(parser));
            if (!parser_match(parser, TOKEN_COMMA)) break;
        }

        parser_expect(parser, TOKEN_RPAREN);
        parser_expect(parser, TOKEN_SEMICOLON);
        node->child = first;
        return node;
    }

    if (parser_is(parser, TOKEN_DRAW))
    {
        ASTNode* node;
        ASTNode* first = NULL;
        ASTNode* last = NULL;

        parser_advance(parser);

        if (parser_match(parser, TOKEN_LOGICAL_NOT))
        {
            if (!parser_is(parser, TOKEN_IDENTIFIER))
            {
                parser_error(parser, "expected draw shape");
                return NULL;
            }
            char shape[64];
            strcpy(shape, parser->current.text);
            parser_advance(parser);
            char* name = (char*)malloc(128);
            snprintf(name, 128, "nw::draw:%s", shape);
            node = ast_new(AST_CALL, line, column);
            node->text = name;
        }
        else
        {
            parser_error(parser, "expected draw!SHAPE");
            return NULL;
        }

        int draw_ended = 0;

        if (parser_match(parser, TOKEN_SHIFT_LEFT))
        {
            while (1)
            {
                if (parser_is(parser, TOKEN_IDENTIFIER))
                {
                    char key[64];
                    strcpy(key, parser->current.text);
                    parser_advance(parser);

                    if (strcmp(key, "frame") == 0)
                    {
                        if (parser_match(parser, TOKEN_SCOPE) &&
                            parser_is(parser, TOKEN_IDENTIFIER) &&
                            text_equals_ci(parser->current.text, "no"))
                        {
                            parser_advance(parser);

                            if (parser_match(parser, TOKEN_SCOPE) &&
                                parser_is(parser, TOKEN_IDENTIFIER) &&
                                text_equals_ci(parser->current.text, "window"))
                            {
                                parser_advance(parser);
                                node->draw_flags |= 1;
                            }
                        }

                        continue;
                    }

                    if (text_equals_ci(key, "TITLE"))
                    {
                        if (parser_match(parser, TOKEN_ASSIGN) &&
                            parser_is(parser, TOKEN_STRING))
                        {
                            free(node->text_label);
                            node->text_label = copy_text(parser->current.text);
                            parser_advance(parser);
                        }

                        continue;
                    }

                    if (parser_match(parser, TOKEN_ASSIGN))
                    {
                        ASTNode* pair = ast_new(AST_EXPRESSION, parser->current.line, parser->current.column);
                        char* txt = (char*)malloc(256);
                        if (parser_is(parser, TOKEN_IDENTIFIER))
                        {
                            snprintf(txt, 256, "K:%s=%s", key, parser->current.text);
                            parser_advance(parser);
                        }
                        else if (parser_is(parser, TOKEN_NUMBER))
                        {
                            snprintf(txt, 256, "K:%s=%s", key, parser->current.text);
                            parser_advance(parser);
                        }
                        else
                        {
                            free(txt);
                            txt = (char*)malloc(256);
                            snprintf(txt, 256, "K:%s", key);
                        }
                        pair->text = txt;
                        ast_append(&first, &last, pair);
                    }
                    else if (strcmp(key, "auto") == 0 && parser_match(parser, TOKEN_SCOPE) && parser_match(parser, TOKEN_SCOPE))
                    {
                        if (parser_is(parser, TOKEN_IDENTIFIER))
                        {
                            ASTNode* pair = ast_new(AST_EXPRESSION, parser->current.line, parser->current.column);
                            char* txt = (char*)malloc(256);
                            snprintf(txt, 256, "AUTO:%s", parser->current.text);
                            parser_advance(parser);
                            pair->text = txt;
                            ast_append(&first, &last, pair);
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (parser_is(parser, TOKEN_COMMA))
                {
                    parser_advance(parser);
                    continue;
                }
                else if (parser_is(parser, TOKEN_LOGICAL_NOT))
                {
                    char* c = NULL;
                    int has_color = parse_color_tail(parser, &c);
                    node->text_color_name = c;
                    (void)has_color;
                    continue;
                }
                else if (parser_match(parser, TOKEN_SHIFT_LEFT))
                {
                    continue;
                }
                else if (parser_is(parser, TOKEN_STRING))
                {
                    free(node->text_label);
                    node->text_label = copy_text(parser->current.text);
                    parser_advance(parser);
                    continue;
                }
                else if (parser_is(parser, TOKEN_SEMICOLON))
                {
                    parser_advance(parser);

                    if (parser_is(parser, TOKEN_IDENTIFIER) ||
                        parser_is(parser, TOKEN_LOGICAL_NOT) ||
                        parser_is(parser, TOKEN_SHIFT_LEFT) ||
                        parser_is(parser, TOKEN_COMMA))
                        continue;

                    draw_ended = 1;
                    break;
                }
                else
                {
                    parser_advance(parser);
                    continue;
                }
            }
        }

        if (!draw_ended)
            parser_expect(parser, TOKEN_SEMICOLON);

        node->child = first;
        return node;
    }


    parser_error(
        parser,
        "unknown nw command"
    );

    return NULL;
}

static ASTNode* parse_if(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_IF);
    parser_expect(parser, TOKEN_LPAREN);

    node = ast_new(AST_IF, line, column);
    node->child = parse_expression(parser);

    parser_expect(parser, TOKEN_RPAREN);

    node->left = parse_statement(parser);

    if (parser_match(parser, TOKEN_ELSE))
        node->right = parse_statement(parser);

    return node;
}

static ASTNode* parse_while(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);

    node = ast_new(AST_WHILE, line, column);
    node->child = parse_expression(parser);

    parser_expect(parser, TOKEN_RPAREN);

    node->left = parse_statement(parser);

    return node;
}

static ASTNode* parse_assignment_statement(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    node = ast_new(AST_ASSIGNMENT, line, column);
    node->text = copy_text(parser->current.text);

    parser_advance(parser);
    parser_expect(parser, TOKEN_ASSIGN);
    node->child = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_asm_statement(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_advance(parser);

    node = ast_new(AST_ASM, line, column);

    parser_expect(parser, TOKEN_LPAREN);

    if (!parser_is(parser, TOKEN_STRING))
    {
        parser_error(parser, "expected asm instruction string");
        return node;
    }

    node->text = copy_text(parser->current.text);
    parser_advance(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_call_statement(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_advance(parser);

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(parser, "expected function name after call");
        return NULL;
    }

    node = ast_new(AST_CALL, line, column);
    node->text = copy_text(parser->current.text);

    parser_advance(parser);
    parser_expect(parser, TOKEN_LPAREN);

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ast_append(
            &first,
            &last,
            parse_expression(parser)
        );

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }

    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    node->child = first;

    return node;
}

static ASTNode* parse_sleep_statement(Parser* parser)
{
    ASTNode* node;

    int line = parser->current.line;
    int column = parser->current.column;

    parser_advance(parser);

    node = ast_new(AST_CALL, line, column);
    node->text = copy_text("sleep");

    parser_expect(parser, TOKEN_LPAREN);
    node->child = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_statement(Parser* parser)
{
    if (parser_is_int_keyword(parser))
    {
        parser_error_unsupported_type(parser, 1);
        parser_advance(parser);
        parser_skip_statement_invalid(parser);
        return NULL;
    }

    if (parser_is_void_keyword(parser))
    {
        parser_error_unsupported_type(parser, 0);
        parser_advance(parser);
        parser_skip_statement_invalid(parser);
        return NULL;
    }

    if (
        parser_is(parser, TOKEN_INT_TYPE) ||
        parser_is(parser, TOKEN_FLOAT_TYPE) ||
        parser_is(parser, TOKEN_BOOL_TYPE) ||
        parser_is(parser, TOKEN_LONG_TYPE) ||
        parser_is(parser, TOKEN_CHAR_TYPE) ||
        parser_is(parser, TOKEN_STRING_TYPE) ||
        parser_is_modifier(parser)
    )
    {
        return parse_variable(parser);
    }

    if (parser_is(parser, TOKEN_IDENTIFIER) &&
        text_equals_ci(parser->current.text, "clear_screen"))
    {
        ASTNode* node = parse_expression(parser);
        parser_expect(parser, TOKEN_SEMICOLON);
        return node;
    }

    if (parser_is(parser, TOKEN_IDENTIFIER))
        return parse_assignment_statement(parser);

    if (parser_is(parser, TOKEN_ASM))
        return parse_asm_statement(parser);
    if (parser_is(parser, TOKEN_RETURN))
    {
        return parse_return(parser);
    }

    if (parser_is(parser, TOKEN_IF))
    {
        return parse_if(parser);
    }

    if (parser_is(parser, TOKEN_WHILE))
    {
        return parse_while(parser);
    }

    if (parser_is(parser, TOKEN_BREAK))
    {
        ASTNode* node = ast_new(
            AST_BREAK,
            parser->current.line,
            parser->current.column
        );

        parser_advance(parser);
        parser_expect(parser, TOKEN_SEMICOLON);

        return node;
    }

    if (parser_is(parser, TOKEN_CONTINUE))
    {
        ASTNode* node = ast_new(
            AST_CONTINUE,
            parser->current.line,
            parser->current.column
        );

        parser_advance(parser);
        parser_expect(parser, TOKEN_SEMICOLON);

        return node;
    }

    if (parser_is(parser, TOKEN_CALL))
        return parse_call_statement(parser);

    if (parser_is(parser, TOKEN_SLEEP))
        return parse_sleep_statement(parser);

    if (parser_is(parser, TOKEN_LBRACE))
    {
        return parse_block(parser);
    }

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

static ASTNode* parse_parameter(Parser* parser)
{
    ASTNode* node;
    ValueType type;
    int line = parser->current.line;
    int column = parser->current.column;

    unsigned int modifiers = parse_modifiers(parser);

    type = parse_type(parser);

    if (type == TYPE_UNKNOWN)
    {
        parser_error(parser, "expected parameter type");
        return NULL;
    }

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(parser, "expected parameter name");
        return NULL;
    }

    node = ast_new(
        AST_VARIABLE_DECLARATION,
        line,
        column
    );

    node->value_type = type;
    node->modifiers = modifiers;
    node->text = copy_text(parser->current.text);

    parser_advance(parser);

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

        parser_expect(parser, TOKEN_RBRACKET);
    }

    return node;
}

static ASTNode* parse_function(Parser* parser)
{
    ASTNode* node;
    ASTNode* params_first = NULL;
    ASTNode* params_last = NULL;

    ValueType return_type;

    int line = parser->current.line;
    int column = parser->current.column;

    if (parser_is(parser, TOKEN_INT_TYPE))
    {
        return_type = TYPE_INT;
        parser_advance(parser);
    }
    else if (parser_is(parser, TOKEN_VOID_TYPE))
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

    parser_expect(
        parser,
        TOKEN_LPAREN
    );

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* parameter = parse_parameter(parser);

        if (parameter != NULL)
        {
            ast_append(
                &params_first,
                &params_last,
                parameter
            );
        }
        else
        {
            parser_advance(parser);
        }

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }


    parser_expect(
        parser,
        TOKEN_RPAREN
    );

    node->left = params_first;

    node->child =
        parse_block(parser);

    return node;
}

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
        if (parser_is(parser, TOKEN_INCLUDE))
        {
            node = parse_include(parser);
        }
        else if (parser_is_int_keyword(parser))
        {
            parser_error_unsupported_type(parser, 1);
            parser_advance(parser);
            parser_skip_top_level_invalid(parser);
            continue;
        }
        else if (parser_is_void_keyword(parser))
        {
            parser_error_unsupported_type(parser, 0);
            parser_advance(parser);
            parser_skip_top_level_invalid(parser);
            continue;
        }
        else if (
            parser_is(parser, TOKEN_INT_TYPE) ||
            parser_is(parser, TOKEN_VOID_TYPE)
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

void parser_init(Parser* parser, Lexer* lexer)
{
    parser->lexer = lexer;

    parser->error_count = 0;

    parser->current.type = TOKEN_EOF;
    parser->previous.type = TOKEN_EOF;

    parser_advance(parser);
}
ASTNode* parser_parse(Parser* parser)
{
    return parse_program(parser);
}

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
    free(node->text_color_name);
    free(node->text_label);
    free(node);

    ast_free(next);
}

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

        case AST_ASSIGNMENT:
            return "ASSIGNMENT";

        case AST_ASM:
            return "ASM";

        case AST_IF:
            return "IF";

        case AST_WHILE:
            return "WHILE";

        case AST_BREAK:
            return "BREAK";

        case AST_CONTINUE:
            return "CONTINUE";

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