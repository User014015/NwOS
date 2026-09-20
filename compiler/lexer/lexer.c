#include "nwc_runtime.h"
#include "lexer.h"


/* =========================================================
   Small internal helpers
   ========================================================= */

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}


static int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}


static int is_alnum(char c)
{
    return is_letter(c) || is_digit(c);
}


static char current_char(Lexer* lexer)
{
    return lexer->source[lexer->position];
}


static char peek_char(Lexer* lexer)
{
    if (lexer->source[lexer->position + 1] == '\0')
        return '\0';

    return lexer->source[lexer->position + 1];
}


static void advance(Lexer* lexer)
{
    if (current_char(lexer) == '\n')
    {
        lexer->line++;
        lexer->column = 1;
    }
    else
    {
        lexer->column++;
    }

    lexer->position++;
}


static void token_add_char(Token* token, char c)
{
    unsigned int length = 0;

    while (token->text[length] != '\0')
        length++;

    if (length < NWC_MAX_TOKEN_TEXT - 1)
    {
        token->text[length] = c;
        token->text[length + 1] = '\0';
    }
}


static Token make_token(TokenType type, int line, int column)
{
    Token token;

    token.type = type;
    token.line = line;
    token.column = column;
    token.text[0] = '\0';

    return token;
}


/* =========================================================
   Skip whitespace and comments
   ========================================================= */

static void skip_whitespace(Lexer* lexer)
{
    while (1)
    {
        char c = current_char(lexer);

        if (c == ' ' ||
            c == '\t' ||
            c == '\r' ||
            c == '\n')
        {
            advance(lexer);
            continue;
        }

        break;
    }
}


/* =========================================================
   Read identifier / keyword
   ========================================================= */

static Token read_identifier(Lexer* lexer)
{
    int line = lexer->line;
    int column = lexer->column;

    Token token = make_token(TOKEN_IDENTIFIER, line, column);

    while (is_alnum(current_char(lexer)))
    {
        token_add_char(&token, current_char(lexer));
        advance(lexer);
    }


    /* =====================================================
       Keywords
       ===================================================== */

    if (strcmp(token.text, "return") == 0)
        token.type = TOKEN_RETURN;

    else if (strcmp(token.text, "true") == 0)
        token.type = TOKEN_TRUE;

    else if (strcmp(token.text, "false") == 0)
        token.type = TOKEN_FALSE;

    else if (strcmp(token.text, "flt") == 0)
        token.type = TOKEN_FLOAT_TYPE;

    else if (strcmp(token.text, "bol") == 0)
        token.type = TOKEN_BOOL_TYPE;

    else if (strcmp(token.text, "lng") == 0)
        token.type = TOKEN_LONG_TYPE;

    else if (strcmp(token.text, "char") == 0)
        token.type = TOKEN_CHAR_TYPE;

    else if (strcmp(token.text, "str") == 0)
        token.type = TOKEN_STRING_TYPE;

    else if (strcmp(token.text, "nw") == 0)
        token.type = TOKEN_NAMESPACE;

    else if (strcmp(token.text, "out") == 0)
        token.type = TOKEN_OUT;

    else if (strcmp(token.text, "endl") == 0)
        token.type = TOKEN_ENDL;

    else if (strcmp(token.text, "cin") == 0)
        token.type = TOKEN_CIN;

    else if (strcmp(token.text, "line") == 0)
        token.type = TOKEN_LINE;


    return token;
}


/* =========================================================
   Read numbers
   ========================================================= */

static Token read_number(Lexer* lexer)
{
    int line = lexer->line;
    int column = lexer->column;

    Token token = make_token(TOKEN_NUMBER, line, column);

    while (is_digit(current_char(lexer)))
    {
        token_add_char(&token, current_char(lexer));
        advance(lexer);
    }


    /*
     * Float:
     *
     * 1.3
     * 1.3f
     */

    if (current_char(lexer) == '.')
    {
        token.type = TOKEN_FLOAT;

        token_add_char(&token, '.');
        advance(lexer);

        while (is_digit(current_char(lexer)))
        {
            token_add_char(&token, current_char(lexer));
            advance(lexer);
        }

        if (current_char(lexer) == 'f')
        {
            token_add_char(&token, 'f');
            advance(lexer);
        }
    }


    return token;
}


/* =========================================================
   Read string
   ========================================================= */

static Token read_string(Lexer* lexer)
{
    int line = lexer->line;
    int column = lexer->column;

    Token token = make_token(TOKEN_STRING, line, column);

    /* Skip opening quote */
    advance(lexer);

    while (current_char(lexer) != '\0' &&
           current_char(lexer) != '"')
    {
        /*
         * Escape sequences.
         *
         * We keep them inside the token for now.
         * The parser/code generator will interpret them later.
         */

        if (current_char(lexer) == '\\')
        {
            token_add_char(&token, current_char(lexer));
            advance(lexer);

            if (current_char(lexer) != '\0')
            {
                token_add_char(&token, current_char(lexer));
                advance(lexer);
            }

            continue;
        }

        token_add_char(&token, current_char(lexer));
        advance(lexer);
    }

    if (current_char(lexer) == '"')
    {
        advance(lexer);
    }
    else
    {
        token.type = TOKEN_ERROR;
    }

    return token;
}


/* =========================================================
   Read character
   ========================================================= */

static Token read_char(Lexer* lexer)
{
    int line = lexer->line;
    int column = lexer->column;

    Token token = make_token(TOKEN_CHAR, line, column);

    /* Skip opening ' */
    advance(lexer);

    if (current_char(lexer) == '\\')
    {
        token_add_char(&token, current_char(lexer));
        advance(lexer);

        if (current_char(lexer) != '\0')
        {
            token_add_char(&token, current_char(lexer));
            advance(lexer);
        }
    }
    else if (current_char(lexer) != '\0')
    {
        token_add_char(&token, current_char(lexer));
        advance(lexer);
    }

    if (current_char(lexer) == '\'')
    {
        advance(lexer);
    }
    else
    {
        token.type = TOKEN_ERROR;
    }

    return token;
}


/* =========================================================
   Token type name (for debug/error printing)
   ========================================================= */

const char* token_type_name(TokenType type)
{
    switch (type)
    {
        case TOKEN_EOF:            return "EOF";

        case TOKEN_INCLUDE:        return "INCLUDE";
        case TOKEN_HEADER:         return "HEADER";

        case TOKEN_INT_TYPE:       return "INT_TYPE";
        case TOKEN_FLOAT_TYPE:     return "FLOAT_TYPE";
        case TOKEN_BOOL_TYPE:      return "BOOL_TYPE";
        case TOKEN_LONG_TYPE:      return "LONG_TYPE";
        case TOKEN_CHAR_TYPE:      return "CHAR_TYPE";
        case TOKEN_STRING_TYPE:    return "STRING_TYPE";

        case TOKEN_RETURN:         return "RETURN";
        case TOKEN_TRUE:           return "TRUE";
        case TOKEN_FALSE:          return "FALSE";

        case TOKEN_IDENTIFIER:     return "IDENTIFIER";
        case TOKEN_NUMBER:         return "NUMBER";
        case TOKEN_FLOAT:          return "FLOAT";
        case TOKEN_STRING:         return "STRING";
        case TOKEN_CHAR:           return "CHAR";

        case TOKEN_NAMESPACE:      return "NAMESPACE";
        case TOKEN_SCOPE:          return "SCOPE";
        case TOKEN_OUT:            return "OUT";
        case TOKEN_ENDL:           return "ENDL";
        case TOKEN_CIN:            return "CIN";
        case TOKEN_LINE:           return "LINE";

        case TOKEN_SHIFT_LEFT:     return "SHIFT_LEFT";
        case TOKEN_SHIFT_RIGHT:    return "SHIFT_RIGHT";

        case TOKEN_PLUS:           return "PLUS";
        case TOKEN_MINUS:          return "MINUS";
        case TOKEN_STAR:           return "STAR";
        case TOKEN_SLASH:          return "SLASH";
        case TOKEN_PERCENT:        return "PERCENT";
        case TOKEN_ASSIGN:         return "ASSIGN";
        case TOKEN_PLUS_EQUAL:     return "PLUS_EQUAL";
        case TOKEN_MINUS_EQUAL:    return "MINUS_EQUAL";
        case TOKEN_STAR_EQUAL:     return "STAR_EQUAL";
        case TOKEN_SLASH_EQUAL:    return "SLASH_EQUAL";
        case TOKEN_AND_AND:        return "AND_AND";
        case TOKEN_OR_OR:          return "OR_OR";
        case TOKEN_BANG:           return "BANG";

        case TOKEN_EQUAL_EQUAL:    return "EQUAL_EQUAL";
        case TOKEN_NOT_EQUAL:      return "NOT_EQUAL";
        case TOKEN_LESS:           return "LESS";
        case TOKEN_GREATER:        return "GREATER";
        case TOKEN_LESS_EQUAL:     return "LESS_EQUAL";
        case TOKEN_GREATER_EQUAL:  return "GREATER_EQUAL";

        case TOKEN_LPAREN:         return "LPAREN";
        case TOKEN_RPAREN:         return "RPAREN";
        case TOKEN_LBRACE:         return "LBRACE";
        case TOKEN_RBRACE:         return "RBRACE";
        case TOKEN_LBRACKET:       return "LBRACKET";
        case TOKEN_RBRACKET:       return "RBRACKET";
        case TOKEN_COMMA:          return "COMMA";
        case TOKEN_SEMICOLON:      return "SEMICOLON";

        case TOKEN_ERROR:          return "ERROR";

        default:                  return "UNKNOWN";
    }
}


/* =========================================================
   Lexer initialization
   ========================================================= */

void lexer_init(Lexer* lexer, const char* source)
{
    lexer->source = source;
    lexer->position = 0;

    lexer->line = 1;
    lexer->column = 1;

    lexer->last_type = TOKEN_EOF;
}


/* =========================================================
   Main lexer
   ========================================================= */

static Token lexer_next_impl(Lexer* lexer)
{
    skip_whitespace(lexer);

    int line = lexer->line;
    int column = lexer->column;

    char c = current_char(lexer);

    /* EOF */

    if (c == '\0')
    {
        return make_token(TOKEN_EOF, line, column);
    }


    /* =====================================================
       #include
       ===================================================== */

    if (c == '#')
    {
        Token token = make_token(TOKEN_INCLUDE, line, column);

        token_add_char(&token, c);
        advance(lexer);

        while (is_letter(current_char(lexer)))
        {
            token_add_char(&token, current_char(lexer));
            advance(lexer);
        }

        if (strcmp(token.text, "#include") != 0)
            token.type = TOKEN_ERROR;

        return token;
    }


    /* =====================================================
       Header <nwc.h>

       Only valid directly after a #include token - otherwise
       '<' is the start of <, <=, or << (handled further down).
       Without this check, a bare '<' anywhere (e.g. "x < 10" or
       "out << x") would be swallowed as a runaway header token
       hunting for a '>' that may never come, eating the rest of
       the file.
       ===================================================== */

    if (c == '<' && lexer->last_type == TOKEN_INCLUDE)
    {
        Token token = make_token(TOKEN_HEADER, line, column);

        token_add_char(&token, c);
        advance(lexer);

        while (current_char(lexer) != '\0' &&
               current_char(lexer) != '>')
        {
            token_add_char(&token, current_char(lexer));
            advance(lexer);
        }

        if (current_char(lexer) == '>')
        {
            token_add_char(&token, '>');
            advance(lexer);
        }
        else
        {
            token.type = TOKEN_ERROR;
        }

        return token;
    }


    /* =====================================================
       Identifier / keyword
       ===================================================== */

    if (is_letter(c))
    {
        return read_identifier(lexer);
    }


    /* =====================================================
       Number
       ===================================================== */

    if (is_digit(c))
    {
        /*
         * NwLang special integer type:
         *
         * 1A
         */

        if (c == '1' && peek_char(lexer) == 'A')
        {
            Token token = make_token(
                TOKEN_INT_TYPE,
                line,
                column
            );

            token_add_char(&token, '1');
            advance(lexer);

            token_add_char(&token, 'A');
            advance(lexer);

            return token;
        }

        return read_number(lexer);
    }


    /* =====================================================
       String
       ===================================================== */

    if (c == '"')
    {
        return read_string(lexer);
    }


    /* =====================================================
       Character
       ===================================================== */

    if (c == '\'')
    {
        return read_char(lexer);
    }


    /* =====================================================
       ::
       ===================================================== */

    if (c == ':' && peek_char(lexer) == ':')
    {
        Token token = make_token(TOKEN_SCOPE, line, column);

        token_add_char(&token, ':');
        advance(lexer);

        token_add_char(&token, ':');
        advance(lexer);

        return token;
    }


    /* =====================================================
       <<
       ===================================================== */

    if (c == '<' && peek_char(lexer) == '<')
    {
        Token token = make_token(
            TOKEN_SHIFT_LEFT,
            line,
            column
        );

        token_add_char(&token, '<');
        advance(lexer);

        token_add_char(&token, '<');
        advance(lexer);

        return token;
    }


    /* =====================================================
       >>
       ===================================================== */

    if (c == '>' && peek_char(lexer) == '>')
    {
        Token token = make_token(
            TOKEN_SHIFT_RIGHT,
            line,
            column
        );

        token_add_char(&token, '>');
        advance(lexer);

        token_add_char(&token, '>');
        advance(lexer);

        return token;
    }


    /* =====================================================
       ==
       ===================================================== */

    if (c == '=' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_EQUAL_EQUAL,
            line,
            column
        );

        token_add_char(&token, '=');
        advance(lexer);

        token_add_char(&token, '=');
        advance(lexer);

        return token;
    }


    /* =====================================================
       !=
       ===================================================== */

    if (c == '!' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_NOT_EQUAL,
            line,
            column
        );

        token_add_char(&token, '!');
        advance(lexer);

        token_add_char(&token, '=');
        advance(lexer);

        return token;
    }


    /* =====================================================
       <=
       ===================================================== */

    if (c == '<' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_LESS_EQUAL,
            line,
            column
        );

        token_add_char(&token, '<');
        advance(lexer);

        token_add_char(&token, '=');
        advance(lexer);

        return token;
    }


    /* =====================================================
       >=
       ===================================================== */

    if (c == '>' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_GREATER_EQUAL,
            line,
            column
        );

        token_add_char(&token, '>');
        advance(lexer);

        token_add_char(&token, '=');
        advance(lexer);

        return token;
    }



    /* =====================================================
       Compound assignment
       ===================================================== */

    if (c == '+' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_PLUS_EQUAL,
            line,
            column
        );

        token_add_char(&token, '+');
        advance(lexer);
        token_add_char(&token, '=');
        advance(lexer);
        return token;
    }

    if (c == '-' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_MINUS_EQUAL,
            line,
            column
        );

        token_add_char(&token, '-');
        advance(lexer);
        token_add_char(&token, '=');
        advance(lexer);
        return token;
    }

    if (c == '*' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_STAR_EQUAL,
            line,
            column
        );

        token_add_char(&token, '*');
        advance(lexer);
        token_add_char(&token, '=');
        advance(lexer);
        return token;
    }

    if (c == '/' && peek_char(lexer) == '=')
    {
        Token token = make_token(
            TOKEN_SLASH_EQUAL,
            line,
            column
        );

        token_add_char(&token, '/');
        advance(lexer);
        token_add_char(&token, '=');
        advance(lexer);
        return token;
    }

    if (c == '%' )
    {
        Token token = make_token(
            TOKEN_PERCENT,
            line,
            column
        );

        token_add_char(&token, '%');
        advance(lexer);
        return token;
    }

    /* =====================================================
       Logical operators
       ===================================================== */

    if (c == '&' && peek_char(lexer) == '&')
    {
        Token token = make_token(
            TOKEN_AND_AND,
            line,
            column
        );

        token_add_char(&token, '&');
        advance(lexer);
        token_add_char(&token, '&');
        advance(lexer);
        return token;
    }

    if (c == '|' && peek_char(lexer) == '|')
    {
        Token token = make_token(
            TOKEN_OR_OR,
            line,
            column
        );

        token_add_char(&token, '|');
        advance(lexer);
        token_add_char(&token, '|');
        advance(lexer);
        return token;
    }

    if (c == '!' )
    {
        Token token = make_token(
            TOKEN_BANG,
            line,
            column
        );

        token_add_char(&token, '!');
        advance(lexer);
        return token;
    }

    /* =====================================================
       Single-character tokens
       ===================================================== */

    Token token;

    switch (c)
    {
        case '=':
            token = make_token(TOKEN_ASSIGN, line, column);
            break;

        case '+':
            token = make_token(TOKEN_PLUS, line, column);
            break;

        case '-':
            token = make_token(TOKEN_MINUS, line, column);
            break;

        case '*':
            token = make_token(TOKEN_STAR, line, column);
            break;

        case '/':
            token = make_token(TOKEN_SLASH, line, column);
            break;

        case '<':
            token = make_token(TOKEN_LESS, line, column);
            break;

        case '>':
            token = make_token(TOKEN_GREATER, line, column);
            break;

        case '(':
            token = make_token(TOKEN_LPAREN, line, column);
            break;

        case ')':
            token = make_token(TOKEN_RPAREN, line, column);
            break;

        case '{':
            token = make_token(TOKEN_LBRACE, line, column);
            break;

        case '}':
            token = make_token(TOKEN_RBRACE, line, column);
            break;

        case '[':
            token = make_token(TOKEN_LBRACKET, line, column);
            break;

        case ']':
            token = make_token(TOKEN_RBRACKET, line, column);
            break;

        case ',':
            token = make_token(TOKEN_COMMA, line, column);
            break;

        case ';':
            token = make_token(TOKEN_SEMICOLON, line, column);
            break;

        default:
            token = make_token(TOKEN_ERROR, line, column);
            break;
    }

    token_add_char(&token, c);
    advance(lexer);

    return token;
}


/* =========================================================
   Public entry point - wraps lexer_next_impl so last_type is
   always kept up to date, regardless of which of the many
   return points inside it produced the token.
   ========================================================= */

Token lexer_next(Lexer* lexer)
{
    Token token = lexer_next_impl(lexer);

    lexer->last_type = token.type;

    return token;
}