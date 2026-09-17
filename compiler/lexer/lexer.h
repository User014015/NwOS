#ifndef NWC_LEXER_H
#define NWC_LEXER_H

#define NWC_MAX_TOKEN_TEXT 128

typedef enum
{
    TOKEN_EOF = 0,
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
	
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,

    TOKEN_NAMESPACE, /* nw */
    TOKEN_OUT,
    TOKEN_ENDL,
    TOKEN_CIN,
    TOKEN_LINE,
    TOKEN_TIME,
    TOKEN_COLOR,
    TOKEN_GETKEY,
    TOKEN_CLEAR,
    TOKEN_SCOPE,
	
    TOKEN_LOW,             /* low */
    TOKEN_OUT8,
    TOKEN_IN8,
    TOKEN_OUT16,
    TOKEN_IN16,
    TOKEN_MEMORY_WRITE8,
    TOKEN_MEMORY_READ8,
    TOKEN_CLI,
    TOKEN_STI,

    TOKEN_SHIFT_LEFT,     /* << */
    TOKEN_SHIFT_RIGHT,    /* >> */

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_ASSIGN,         /* = */

    TOKEN_EQUAL_EQUAL,    /* == */
    TOKEN_NOT_EQUAL,      /* != */
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,

    TOKEN_LOGICAL_AND,    /* && */
    TOKEN_LOGICAL_OR,     /* || */
    TOKEN_LOGICAL_NOT,    /* !  */

    TOKEN_BIT_AND,        /* &  */
    TOKEN_BIT_OR,         /* |  */
    TOKEN_BIT_XOR,        /* ^  */
    TOKEN_BIT_NOT,        /* ~  */

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_ERROR

} TokenType;


typedef struct
{
    TokenType type;

    char text[NWC_MAX_TOKEN_TEXT];

    int line;
    int column;

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