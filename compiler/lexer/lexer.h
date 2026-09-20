#ifndef NWC_LEXER_H
#define NWC_LEXER_H

#define NWC_MAX_TOKEN_TEXT 256

typedef enum
{
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    TOKEN_INCLUDE,
    TOKEN_HEADER,

    TOKEN_INT_TYPE,
    TOKEN_FLOAT_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_LONG_TYPE,
    TOKEN_CHAR_TYPE,
    TOKEN_STRING_TYPE,

    TOKEN_RETURN,
    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,

    TOKEN_NAMESPACE,
    TOKEN_SCOPE,
    TOKEN_OUT,
    TOKEN_ENDL,
    TOKEN_CIN,
    TOKEN_LINE,

    TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,

    TOKEN_ASSIGN,
    TOKEN_PLUS_EQUAL,
    TOKEN_MINUS_EQUAL,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH_EQUAL,

    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,

    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_BANG,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON

} TokenType;

typedef struct
{
    TokenType type;
    int line;
    int column;
    char text[NWC_MAX_TOKEN_TEXT];

} Token;

typedef struct
{
    const char* source;
    unsigned int position;
    int line;
    int column;
    TokenType last_type;

} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next(Lexer* lexer);
const char* token_type_name(TokenType type);

#endif
