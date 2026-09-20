#include "nwc_runtime.h"
#include "parser.h"

/*
 * NwC parser
 *
 * This version matches the expanded ASTNode from the new parser.h.
 *
 * Current lexer compatibility:
 *   - control-flow keywords (if/else/while/for/break/continue)
 *     are currently recognized by identifier text, so lexer.c does
 *     not need the new TOKEN_* names yet.
 *   - assignment supports =, +=, -=, *= and /=.
 *   - expressions support %, && and ||.
 *
 * Expression parsing uses recursive descent + precedence parsing.
 */

/* ============================================================
   Parser helpers
   ============================================================ */

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

static int parser_is_identifier_text(Parser* parser, const char* text)
{
    if (!parser_is(parser, TOKEN_IDENTIFIER))
        return 0;

    if (parser->current.text == NULL)
        return 0;

    return strcmp(parser->current.text, text) == 0;
}

static int parser_match_identifier(Parser* parser, const char* text)
{
    if (!parser_is_identifier_text(parser, text))
        return 0;

    parser_advance(parser);
    return 1;
}

/* ============================================================
   AST allocation helpers
   ============================================================ */

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

static char* make_qualified_name(const char* name)
{
    size_t len;
    char* result;

    if (name == NULL)
        return NULL;

    len = strlen(name);

    result = (char*)malloc(len + 4);

    if (result == NULL)
        return NULL;

    result[0] = 'n';
    result[1] = 'w';
    result[2] = ':';
    result[3] = ':';

    memcpy(result + 4, name, len + 1);

    return result;
}

/* ============================================================
   Types
   ============================================================ */

static ValueType parse_type(Parser* parser)
{
    if (parser_match(parser, TOKEN_INT_TYPE))
        return TYPE_INT;

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

static ValueType parse_function_return_type(Parser* parser)
{
    ValueType type;

    type = parse_type(parser);

    if (type != TYPE_UNKNOWN)
        return type;

    if (parser_is_identifier_text(parser, "void"))
    {
        parser_advance(parser);
        return TYPE_VOID;
    }

    return TYPE_UNKNOWN;
}

static int starts_type(Parser* parser)
{
    return parser_is(parser, TOKEN_INT_TYPE) ||
           parser_is(parser, TOKEN_FLOAT_TYPE) ||
           parser_is(parser, TOKEN_BOOL_TYPE) ||
           parser_is(parser, TOKEN_LONG_TYPE) ||
           parser_is(parser, TOKEN_CHAR_TYPE) ||
           parser_is(parser, TOKEN_STRING_TYPE);
}

/* ============================================================
   Forward declarations
   ============================================================ */

static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_block(Parser* parser);

/* ============================================================
   Expressions
   ============================================================ */

static int binary_precedence(TokenType type)
{
    switch (type)
    {
        /* Equality */
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_NOT_EQUAL:
            return 10;

        /* Relational */
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_LESS_EQUAL:
        case TOKEN_GREATER_EQUAL:
            return 20;

        /* Addition */
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 30;

        /* Multiplication / remainder */
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
            return 40;

        /* Logical AND */
        case TOKEN_AND_AND:
            return 5;

        /* Logical OR */
        case TOKEN_OR_OR:
            return 3;

        default:
            return -1;
    }
}

static ASTNode* parse_primary(Parser* parser);
static ASTNode* parse_unary(Parser* parser);
static ASTNode* parse_binary_rhs(Parser* parser, int min_precedence, ASTNode* left);

static const char* binary_operator_text(TokenType type)
{
    switch (type)
    {
        case TOKEN_EQUAL_EQUAL:    return "==";
        case TOKEN_NOT_EQUAL:      return "!=";
        case TOKEN_LESS:           return "<";
        case TOKEN_GREATER:        return ">";
        case TOKEN_LESS_EQUAL:     return "<=";
        case TOKEN_GREATER_EQUAL:  return ">=";
        case TOKEN_PLUS:           return "+";
        case TOKEN_MINUS:          return "-";
        case TOKEN_STAR:           return "*";
        case TOKEN_SLASH:          return "/";
        default:                   return "?";
    }
}

static ASTNode* parse_call_arguments(Parser* parser)
{
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    parser_expect(parser, TOKEN_LPAREN);

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* argument;

        argument = parse_expression(parser);

        if (argument != NULL)
            ast_append(&first, &last, argument);

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }

    parser_expect(parser, TOKEN_RPAREN);

    return first;
}

static ASTNode* parse_qualified_expression(Parser* parser)
{
    ASTNode* node;
    char name_buffer[128];
    int line;
    int column;

    line = parser->current.line;
    column = parser->current.column;

    parser_expect(parser, TOKEN_NAMESPACE);
    parser_expect(parser, TOKEN_SCOPE);

    if (parser_is(parser, TOKEN_ENDL))
    {
        node = ast_new(AST_EXPRESSION, line, column);
        if (node != NULL)
            node->text = copy_text("nw::endl");

        parser_advance(parser);
        return node;
    }

    if (parser_is(parser, TOKEN_CIN))
    {
        node = ast_new(AST_EXPRESSION, line, column);
        if (node != NULL)
            node->text = copy_text("nw::cin");

        parser_advance(parser);
        return node;
    }

    if (parser_is(parser, TOKEN_OUT))
    {
        parser_error(parser, "nw::out cannot be used as an expression");
        parser_advance(parser);
        return NULL;
    }

    if (parser_is(parser, TOKEN_LINE))
    {
        parser_error(parser, "nw::line cannot be used as an expression");
        parser_advance(parser);
        return NULL;
    }

    if (parser_is(parser, TOKEN_IDENTIFIER))
    {
        size_t i;
        const char* name = parser->current.text;

        i = 0;
        while (name[i] != '\0' && i < sizeof(name_buffer) - 5)
        {
            name_buffer[i] = name[i];
            i++;
        }
        name_buffer[i] = '\0';

        parser_advance(parser);
    }
    else
    {
        parser_error(parser, "expected name after nw::");
        return NULL;
    }

    node = ast_new(AST_CALL, line, column);

    if (node == NULL)
        return NULL;

    node->text = make_qualified_name(name_buffer);

    if (node->text == NULL)
    {
        ast_free(node);
        return NULL;
    }

    if (parser_is(parser, TOKEN_LPAREN))
        node->child = parse_call_arguments(parser);

    return node;
}

static ASTNode* parse_identifier_or_call(Parser* parser)
{
    ASTNode* node;
    int line = parser->current.line;
    int column = parser->current.column;
    char* name;

    name = copy_text(parser->current.text);

    if (name == NULL)
    {
        parser_error(parser, "out of memory while copying identifier");
        parser_advance(parser);
        return NULL;
    }

    parser_advance(parser);

    if (parser_is(parser, TOKEN_LPAREN))
    {
        node = ast_new(AST_CALL, line, column);

        if (node == NULL)
        {
            free(name);
            return NULL;
        }

        node->text = name;
        node->child = parse_call_arguments(parser);
        return node;
    }

    node = ast_new(AST_IDENTIFIER, line, column);

    if (node == NULL)
    {
        free(name);
        return NULL;
    }

    node->text = name;

    return node;
}

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

        if (node == NULL)
            return NULL;

        node->integer_value =
            strtoll(parser->current.text, NULL, 10);
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

        if (node == NULL)
            return NULL;

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

        if (node == NULL)
            return NULL;

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

        if (node == NULL)
            return NULL;

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

        if (node == NULL)
            return NULL;

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

        if (node == NULL)
            return NULL;

        node->integer_value = 0;
        node->value_type = TYPE_BOOL;
        return node;
    }

    if (parser_is(parser, TOKEN_IDENTIFIER))
        return parse_identifier_or_call(parser);

    if (parser_is(parser, TOKEN_NAMESPACE))
        return parse_qualified_expression(parser);

    if (parser_match(parser, TOKEN_LPAREN))
    {
        node = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return node;
    }

    parser_error(parser, "expected expression");
    return NULL;
}

static ASTNode* parse_unary(Parser* parser)
{
    ASTNode* node;
    ASTNode* operand;

    if (parser_is(parser, TOKEN_MINUS))
    {
        int line = parser->current.line;
        int column = parser->current.column;

        parser_advance(parser);

        operand = parse_unary(parser);

        node = ast_new(AST_UNARY_OPERATION, line, column);

        if (node == NULL)
        {
            ast_free(operand);
            return NULL;
        }

        node->text = copy_text("-");
        node->child = operand;

        return node;
    }

    return parse_primary(parser);
}

static ASTNode* make_binary_node(
    Parser* parser,
    const char* op,
    ASTNode* left,
    ASTNode* right,
    int line,
    int column)
{
    ASTNode* node;

    node = ast_new(AST_BINARY_OPERATION, line, column);

    if (node == NULL)
    {
        ast_free(left);
        ast_free(right);
        return NULL;
    }

    node->text = copy_text(op);
    node->left = left;
    node->right = right;

    return node;
}

static ASTNode* parse_binary_rhs(
    Parser* parser,
    int min_precedence,
    ASTNode* left)
{
    while (1)
    {
        int precedence =
            binary_precedence(parser->current.type);

        ASTNode* right;
        ASTNode* operation;
        TokenType operator_type;
        const char* operator_text;
        int line;
        int column;

        if (precedence < min_precedence)
            return left;

        operator_type = parser->current.type;
        operator_text = binary_operator_text(operator_type);
        line = parser->current.line;
        column = parser->current.column;

        parser_advance(parser);

        right = parse_unary(parser);

        if (right == NULL)
        {
            ast_free(left);
            return NULL;
        }

        while (1)
        {
            int next_precedence =
                binary_precedence(parser->current.type);

            if (next_precedence > precedence)
            {
                right = parse_binary_rhs(
                    parser,
                    precedence + 1,
                    right
                );

                if (right == NULL)
                {
                    ast_free(left);
                    return NULL;
                }
            }
            else
            {
                break;
            }
        }

        operation = make_binary_node(
            parser,
            operator_text,
            left,
            right,
            line,
            column
        );

        if (operation == NULL)
            return NULL;

        left = operation;
    }
}

static ASTNode* parse_expression(Parser* parser)
{
    ASTNode* left;

    left = parse_unary(parser);

    if (left == NULL)
        return NULL;

    return parse_binary_rhs(parser, 0, left);
}

/* ============================================================
   Variable declarations
   ============================================================ */

static ASTNode* parse_variable(Parser* parser, int require_semicolon)
{
    ASTNode* node;
    ValueType type;
    int line = parser->current.line;
    int column = parser->current.column;

    type = parse_type(parser);

    if (type == TYPE_UNKNOWN)
    {
        parser_error(parser, "expected variable type");
        return NULL;
    }

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(parser, "expected variable name");
        return NULL;
    }

    node = ast_new(AST_VARIABLE_DECLARATION, line, column);

    if (node == NULL)
        return NULL;

    node->value_type = type;
    node->text = copy_text(parser->current.text);

    parser_advance(parser);

    if (parser_match(parser, TOKEN_LBRACKET))
    {
        ASTNode* size_node;

        node->type = AST_ARRAY_DECLARATION;

        size_node = parse_expression(parser);

        if (size_node != NULL && size_node->type == AST_NUMBER)
        {
            if (size_node->integer_value > 0)
                node->array_size =
                    (unsigned int)size_node->integer_value;
        }

        ast_free(size_node);

        parser_expect(parser, TOKEN_RBRACKET);
    }

    if (parser_match(parser, TOKEN_ASSIGN))
        node->child = parse_expression(parser);

    if (require_semicolon)
        parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

/* ============================================================
   Assignment / expression statements
   ============================================================ */

static const char* assignment_operator_text(TokenType type)
{
    switch (type)
    {
        case TOKEN_ASSIGN:       return "=";
        case TOKEN_PLUS_EQUAL:   return "+=";
        case TOKEN_MINUS_EQUAL:  return "-=";
        case TOKEN_STAR_EQUAL:   return "*=";
        case TOKEN_SLASH_EQUAL:  return "/=";
        default:                 return NULL;
    }
}

static int is_assignment_operator(TokenType type)
{
    return type == TOKEN_ASSIGN ||
           type == TOKEN_PLUS_EQUAL ||
           type == TOKEN_MINUS_EQUAL ||
           type == TOKEN_STAR_EQUAL ||
           type == TOKEN_SLASH_EQUAL;
}

static ASTNode* parse_assignment_after_name(
    Parser* parser,
    const char* name,
    int line,
    int column,
    int require_semicolon)
{
    ASTNode* node;
    const char* operator_text;

    if (!is_assignment_operator(parser->current.type))
    {
        parser_error(parser, "expected assignment operator after variable name");
        return NULL;
    }

    operator_text = assignment_operator_text(parser->current.type);
    parser_advance(parser);

    node = ast_new(AST_ASSIGNMENT, line, column);

    if (node == NULL)
        return NULL;

    node->text = copy_text(operator_text);

    node->left = ast_new(AST_IDENTIFIER, line, column);

    if (node->left == NULL)
    {
        ast_free(node);
        return NULL;
    }

    node->left->text = copy_text(name);

    node->right = parse_expression(parser);

    if (node->right == NULL)
    {
        ast_free(node);
        return NULL;
    }

    if (require_semicolon)
        parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_assignment_statement(Parser* parser, int require_semicolon)
{
    ASTNode* node;
    char* name;
    int line;
    int column;

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(parser, "expected variable name");
        return NULL;
    }

    line = parser->current.line;
    column = parser->current.column;
    name = copy_text(parser->current.text);

    if (name == NULL)
    {
        parser_error(parser, "out of memory while copying identifier");
        parser_advance(parser);
        return NULL;
    }

    parser_advance(parser);

    if (parser_is(parser, TOKEN_LPAREN))
    {
        node = ast_new(AST_CALL, line, column);

        if (node == NULL)
        {
            free(name);
            return NULL;
        }

        node->text = name;
        node->child = parse_call_arguments(parser);

        if (require_semicolon)
            parser_expect(parser, TOKEN_SEMICOLON);

        return node;
    }

    node = parse_assignment_after_name(
        parser,
        name,
        line,
        column,
        require_semicolon
    );

    free(name);

    return node;
}

/* ============================================================
   Nw namespace statements
   ============================================================ */

static ASTNode* parse_output(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;
    int line = parser->previous.line;
    int column = parser->previous.column;

    node = ast_new(AST_OUTPUT, line, column);

    if (node == NULL)
        return NULL;

    while (parser_match(parser, TOKEN_SHIFT_LEFT))
    {
        ASTNode* expression = parse_expression(parser);

        if (expression == NULL)
            break;

        ast_append(&first, &last, expression);
    }

    node->child = first;

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_input(Parser* parser)
{
    ASTNode* node;
    int line = parser->previous.line;
    int column = parser->previous.column;

    node = ast_new(AST_INPUT, line, column);

    if (node == NULL)
        return NULL;

    parser_expect(parser, TOKEN_SHIFT_RIGHT);

    node->child = parse_expression(parser);

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_line_input(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;
    int line = parser->previous.line;
    int column = parser->previous.column;

    node = ast_new(AST_LINE_INPUT, line, column);

    if (node == NULL)
        return NULL;

    parser_expect(parser, TOKEN_LPAREN);

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* argument = parse_expression(parser);

        if (argument != NULL)
            ast_append(&first, &last, argument);

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }

    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_SEMICOLON);

    node->child = first;

    return node;
}

static ASTNode* parse_nw_statement(Parser* parser)
{
    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_NAMESPACE);
    parser_expect(parser, TOKEN_SCOPE);

    if (parser_is(parser, TOKEN_OUT))
    {
        parser_advance(parser);
        return parse_output(parser);
    }

    if (parser_is(parser, TOKEN_CIN))
    {
        parser_advance(parser);
        return parse_input(parser);
    }

    if (parser_is(parser, TOKEN_LINE))
    {
        parser_advance(parser);
        return parse_line_input(parser);
    }

    if (parser_is(parser, TOKEN_ENDL))
    {
        parser_error(parser, "nw::endl can only be used with nw::out");
        parser_advance(parser);
        parser_expect(parser, TOKEN_SEMICOLON);
        return NULL;
    }

    if (parser_is(parser, TOKEN_IDENTIFIER))
    {
        ASTNode* node;
        char* qualified_name;
        size_t len;

        len = strlen(parser->current.text);
        qualified_name = (char*)malloc(len + 5);

        if (qualified_name == NULL)
        {
            parser_error(parser, "out of memory while building nw:: call");
            parser_advance(parser);
            return NULL;
        }

        qualified_name[0] = 'n';
        qualified_name[1] = 'w';
        qualified_name[2] = ':';
        qualified_name[3] = ':';
        memcpy(qualified_name + 4,
               parser->current.text,
               len + 1);

        parser_advance(parser);

        node = ast_new(AST_CALL, line, column);

        if (node == NULL)
        {
            free(qualified_name);
            return NULL;
        }

        node->text = qualified_name;

        if (parser_is(parser, TOKEN_LPAREN))
            node->child = parse_call_arguments(parser);
        else
            parser_error(parser, "expected '(' after nw:: function");

        parser_expect(parser, TOKEN_SEMICOLON);

        return node;
    }

    parser_error(parser, "unknown nw:: command");

    if (!parser_is(parser, TOKEN_EOF))
        parser_advance(parser);

    return NULL;
}

/* ============================================================
   Control flow
   ============================================================ */

static ASTNode* parse_if(Parser* parser)
{
    ASTNode* node;
    int line;
    int column;

    line = parser->previous.line;
    column = parser->previous.column;

    node = ast_new(AST_IF, line, column);

    if (node == NULL)
        return NULL;

    parser_expect(parser, TOKEN_LPAREN);
    node->condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);

    if (parser_is(parser, TOKEN_LBRACE))
        node->then_branch = parse_block(parser);
    else
        node->then_branch = parse_statement(parser);

    if (parser_match_identifier(parser, "else"))
    {
        if (parser_is_identifier_text(parser, "if"))
            node->else_branch = parse_if(parser);
        else if (parser_is(parser, TOKEN_LBRACE))
            node->else_branch = parse_block(parser);
        else
            node->else_branch = parse_statement(parser);
    }

    return node;
}

static ASTNode* parse_while(Parser* parser)
{
    ASTNode* node;
    int line;
    int column;

    line = parser->previous.line;
    column = parser->previous.column;

    node = ast_new(AST_WHILE, line, column);

    if (node == NULL)
        return NULL;

    parser_expect(parser, TOKEN_LPAREN);
    node->while_condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);

    if (parser_is(parser, TOKEN_LBRACE))
        node->while_body = parse_block(parser);
    else
        node->while_body = parse_statement(parser);

    return node;
}

static ASTNode* parse_for(Parser* parser)
{
    ASTNode* node;
    int line;
    int column;

    line = parser->previous.line;
    column = parser->previous.column;

    node = ast_new(AST_FOR, line, column);

    if (node == NULL)
        return NULL;

    parser_expect(parser, TOKEN_LPAREN);

    /* init */
    if (!parser_is(parser, TOKEN_SEMICOLON))
    {
        if (starts_type(parser))
        {
            node->for_init = parse_variable(parser, 1);
        }
        else if (parser_is(parser, TOKEN_IDENTIFIER))
        {
            node->for_init = parse_assignment_statement(parser, 1);
        }
        else
        {
            parser_error(parser, "invalid for-loop initializer");
        }
    }
    else
    {
        parser_advance(parser);
    }

    /* condition */
    if (!parser_is(parser, TOKEN_SEMICOLON))
        node->for_condition = parse_expression(parser);

    parser_expect(parser, TOKEN_SEMICOLON);

    /* increment */
    if (!parser_is(parser, TOKEN_RPAREN))
    {
        if (parser_is(parser, TOKEN_IDENTIFIER))
        {
            node->for_increment =
                parse_assignment_statement(parser, 0);
        }
        else
        {
            parser_error(parser, "invalid for-loop increment");

            while (!parser_is(parser, TOKEN_RPAREN) &&
                   !parser_is(parser, TOKEN_EOF))
            {
                parser_advance(parser);
            }
        }
    }

    parser_expect(parser, TOKEN_RPAREN);

    if (parser_is(parser, TOKEN_LBRACE))
        node->for_body = parse_block(parser);
    else
        node->for_body = parse_statement(parser);

    return node;
}

static ASTNode* parse_break(Parser* parser)
{
    ASTNode* node;

    node = ast_new(
        AST_BREAK,
        parser->previous.line,
        parser->previous.column
    );

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

static ASTNode* parse_continue(Parser* parser)
{
    ASTNode* node;

    node = ast_new(
        AST_CONTINUE,
        parser->previous.line,
        parser->previous.column
    );

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

/* ============================================================
   Return
   ============================================================ */

static ASTNode* parse_return(Parser* parser)
{
    ASTNode* node;
    int line = parser->previous.line;
    int column = parser->previous.column;

    node = ast_new(AST_RETURN, line, column);

    if (node == NULL)
        return NULL;

    if (!parser_is(parser, TOKEN_SEMICOLON))
        node->child = parse_expression(parser);

    parser_expect(parser, TOKEN_SEMICOLON);

    return node;
}

/* ============================================================
   General statements
   ============================================================ */

static ASTNode* parse_statement(Parser* parser)
{
    if (starts_type(parser))
        return parse_variable(parser, 1);

    if (parser_is(parser, TOKEN_RETURN))
    {
        parser_advance(parser);
        return parse_return(parser);
    }

    if (parser_is_identifier_text(parser, "if"))
    {
        parser_advance(parser);
        return parse_if(parser);
    }

    if (parser_is_identifier_text(parser, "while"))
    {
        parser_advance(parser);
        return parse_while(parser);
    }

    if (parser_is_identifier_text(parser, "for"))
    {
        parser_advance(parser);
        return parse_for(parser);
    }

    if (parser_is_identifier_text(parser, "break"))
    {
        parser_advance(parser);
        return parse_break(parser);
    }

    if (parser_is_identifier_text(parser, "continue"))
    {
        parser_advance(parser);
        return parse_continue(parser);
    }

    if (parser_is(parser, TOKEN_NAMESPACE))
        return parse_nw_statement(parser);

    if (parser_is(parser, TOKEN_IDENTIFIER))
        return parse_assignment_statement(parser, 1);

    parser_error(parser, "unknown statement");

    if (!parser_is(parser, TOKEN_EOF))
        parser_advance(parser);

    return NULL;
}

/* ============================================================
   Block
   ============================================================ */

static ASTNode* parse_block(Parser* parser)
{
    ASTNode* node;
    ASTNode* first = NULL;
    ASTNode* last = NULL;
    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_LBRACE);

    node = ast_new(AST_BLOCK, line, column);

    if (node == NULL)
        return NULL;

    while (!parser_is(parser, TOKEN_RBRACE) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* statement = parse_statement(parser);

        if (statement != NULL)
            ast_append(&first, &last, statement);
    }

    parser_expect(parser, TOKEN_RBRACE);

    node->child = first;

    return node;
}

/* ============================================================
   Function parameters
   ============================================================ */

static ASTNode* parse_parameter(Parser* parser)
{
    ASTNode* node;
    ValueType type;
    int line;
    int column;

    line = parser->current.line;
    column = parser->current.column;

    /* Support old syntax: void, 1A a */
    if (parser_is_identifier_text(parser, "void"))
    {
        parser_advance(parser);

        if (parser_is(parser, TOKEN_COMMA) ||
            parser_is(parser, TOKEN_RPAREN))
        {
            return NULL;
        }

        parser_error(parser, "void must be the only empty parameter");
        return NULL;
    }

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

    node = ast_new(AST_PARAMETER, line, column);

    if (node == NULL)
        return NULL;

    node->value_type = type;
    node->text = copy_text(parser->current.text);

    parser_advance(parser);

    return node;
}

static ASTNode* parse_parameters(Parser* parser)
{
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    parser_expect(parser, TOKEN_LPAREN);

    if (parser_is(parser, TOKEN_RPAREN))
    {
        parser_advance(parser);
        return NULL;
    }

    if (parser_is_identifier_text(parser, "void"))
    {
        parser_advance(parser);

        if (!parser_is(parser, TOKEN_RPAREN))
        {
            parser_expect(parser, TOKEN_COMMA);
        }
    }

    while (!parser_is(parser, TOKEN_RPAREN) &&
           !parser_is(parser, TOKEN_EOF))
    {
        ASTNode* parameter = parse_parameter(parser);

        if (parameter != NULL)
            ast_append(&first, &last, parameter);

        if (!parser_match(parser, TOKEN_COMMA))
            break;
    }

    parser_expect(parser, TOKEN_RPAREN);

    return first;
}

/* ============================================================
   Function
   ============================================================ */

static ASTNode* parse_function(Parser* parser)
{
    ASTNode* node;
    ValueType return_type;
    int line = parser->current.line;
    int column = parser->current.column;

    return_type = parse_function_return_type(parser);

    if (return_type == TYPE_UNKNOWN)
    {
        parser_error(parser, "expected function return type");
        return NULL;
    }

    if (!parser_is(parser, TOKEN_IDENTIFIER))
    {
        parser_error(parser, "expected function name");
        return NULL;
    }

    node = ast_new(AST_FUNCTION, line, column);

    if (node == NULL)
        return NULL;

    node->value_type = return_type;
    node->text = copy_text(parser->current.text);

    parser_advance(parser);

    node->parameters = parse_parameters(parser);
    node->child = parse_block(parser);

    return node;
}

/* ============================================================
   Program
   ============================================================ */

static ASTNode* parse_include(Parser* parser)
{
    ASTNode* node;
    int line = parser->current.line;
    int column = parser->current.column;

    parser_expect(parser, TOKEN_INCLUDE);

    node = ast_new(AST_INCLUDE, line, column);

    if (node == NULL)
        return NULL;

    if (parser_is(parser, TOKEN_HEADER))
    {
        node->text = copy_text(parser->current.text);
        parser_advance(parser);
    }
    else
    {
        parser_error(parser, "expected header after #include");
    }

    return node;
}

static ASTNode* parse_program(Parser* parser)
{
    ASTNode* program;
    ASTNode* first = NULL;
    ASTNode* last = NULL;

    program = ast_new(AST_PROGRAM, 1, 1);

    if (program == NULL)
        return NULL;

    while (!parser_is(parser, TOKEN_EOF))
    {
        ASTNode* node = NULL;

        if (parser_is(parser, TOKEN_INCLUDE))
        {
            node = parse_include(parser);
        }
        else if (starts_type(parser) ||
                 parser_is_identifier_text(parser, "void"))
        {
            node = parse_function(parser);
        }
        else
        {
            parser_error(parser, "expected include or function");
            parser_advance(parser);
            continue;
        }

        if (node != NULL)
            ast_append(&first, &last, node);
    }

    program->child = first;

    return program;
}

/* ============================================================
   Public parser API
   ============================================================ */

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
    if (parser == NULL || parser->lexer == NULL)
        return NULL;

    return parse_program(parser);
}

/* ============================================================
   AST destruction
   ============================================================ */

void ast_free(ASTNode* node)
{
    ASTNode* next;

    if (node == NULL)
        return;

    /* Expression tree */
    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->child);

    /* Control-flow branches */
    ast_free(node->condition);
    ast_free(node->then_branch);
    ast_free(node->else_branch);

    ast_free(node->while_condition);
    ast_free(node->while_body);

    ast_free(node->for_init);
    ast_free(node->for_condition);
    ast_free(node->for_increment);
    ast_free(node->for_body);

    /* Function parameter list */
    ast_free(node->parameters);

    next = node->next;

    free(node->text);
    free(node);

    ast_free(next);
}

/* ============================================================
   AST debug printer
   ============================================================ */

static const char* ast_type_name(ASTNodeType type)
{
    switch (type)
    {
        case AST_PROGRAM:               return "PROGRAM";
        case AST_INCLUDE:               return "INCLUDE";
        case AST_FUNCTION:              return "FUNCTION";
        case AST_PARAMETER:             return "PARAMETER";
        case AST_BLOCK:                 return "BLOCK";
        case AST_VARIABLE_DECLARATION:  return "VARIABLE";
        case AST_ARRAY_DECLARATION:     return "ARRAY";
        case AST_RETURN:                return "RETURN";
        case AST_IF:                    return "IF";
        case AST_WHILE:                 return "WHILE";
        case AST_FOR:                   return "FOR";
        case AST_BREAK:                 return "BREAK";
        case AST_CONTINUE:              return "CONTINUE";
        case AST_ASSIGNMENT:            return "ASSIGNMENT";
        case AST_OUTPUT:                return "OUTPUT";
        case AST_INPUT:                 return "INPUT";
        case AST_LINE_INPUT:            return "LINE_INPUT";
        case AST_EXPRESSION:            return "EXPRESSION";
        case AST_NUMBER:                return "NUMBER";
        case AST_FLOAT:                 return "FLOAT";
        case AST_STRING:                return "STRING";
        case AST_CHAR:                  return "CHAR";
        case AST_BOOLEAN:               return "BOOLEAN";
        case AST_IDENTIFIER:            return "IDENTIFIER";
        case AST_BINARY_OPERATION:      return "BINARY_OPERATION";
        case AST_UNARY_OPERATION:       return "UNARY_OPERATION";
        case AST_CALL:                  return "CALL";
        default:                        return "UNKNOWN";
    }
}

static void ast_print_indent(int depth)
{
    int i;

    for (i = 0; i < depth; i++)
        printf("  ");
}

static void ast_print_single(ASTNode* node, int depth)
{
    if (node == NULL)
        return;

    ast_print_indent(depth);

    printf("%s", ast_type_name(node->type));

    if (node->text != NULL)
        printf(" [%s]", node->text);

    if (node->type == AST_NUMBER)
        printf(" = %lld", node->integer_value);

    if (node->type == AST_FLOAT)
        printf(" = %f", node->float_value);

    if (node->type == AST_ARRAY_DECLARATION)
        printf(" [%u]", node->array_size);

    printf("\n");

    if (node->type == AST_IF)
    {
        ast_print_indent(depth + 1);
        printf("condition:\n");
        ast_print(node->condition, depth + 2);

        ast_print_indent(depth + 1);
        printf("then:\n");
        ast_print(node->then_branch, depth + 2);

        if (node->else_branch != NULL)
        {
            ast_print_indent(depth + 1);
            printf("else:\n");
            ast_print(node->else_branch, depth + 2);
        }

        return;
    }

    if (node->type == AST_WHILE)
    {
        ast_print_indent(depth + 1);
        printf("condition:\n");
        ast_print(node->while_condition, depth + 2);

        ast_print_indent(depth + 1);
        printf("body:\n");
        ast_print(node->while_body, depth + 2);

        return;
    }

    if (node->type == AST_FOR)
    {
        ast_print_indent(depth + 1);
        printf("init:\n");
        ast_print(node->for_init, depth + 2);

        ast_print_indent(depth + 1);
        printf("condition:\n");
        ast_print(node->for_condition, depth + 2);

        ast_print_indent(depth + 1);
        printf("increment:\n");
        ast_print(node->for_increment, depth + 2);

        ast_print_indent(depth + 1);
        printf("body:\n");
        ast_print(node->for_body, depth + 2);

        return;
    }

    if (node->type == AST_FUNCTION)
    {
        if (node->parameters != NULL)
        {
            ast_print_indent(depth + 1);
            printf("parameters:\n");
            ast_print(node->parameters, depth + 2);
        }

        if (node->child != NULL)
        {
            ast_print_indent(depth + 1);
            printf("body:\n");
            ast_print(node->child, depth + 2);
        }

        return;
    }

    if (node->type == AST_ASSIGNMENT)
    {
        if (node->left != NULL)
        {
            ast_print_indent(depth + 1);
            printf("left:\n");
            ast_print(node->left, depth + 2);
        }

        if (node->right != NULL)
        {
            ast_print_indent(depth + 1);
            printf("right:\n");
            ast_print(node->right, depth + 2);
        }

        return;
    }

    if (node->type == AST_CALL)
    {
        if (node->child != NULL)
        {
            ast_print_indent(depth + 1);
            printf("arguments:\n");
            ast_print(node->child, depth + 2);
        }

        return;
    }

    if (node->child != NULL)
        ast_print(node->child, depth + 1);

    if (node->left != NULL)
        ast_print(node->left, depth + 1);

    if (node->right != NULL)
        ast_print(node->right, depth + 1);
}

void ast_print(ASTNode* node, int depth)
{
    while (node != NULL)
    {
        ast_print_single(node, depth);
        node = node->next;
    }
}
