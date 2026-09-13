#ifndef NWC_LEXER_H
#define NWC_LEXER_H

#define NWC_MAX_TOKEN_TEXT 128

typedef enum
{
    TOKEN_EOF = 0,

    /* Preprocessor */
    TOKEN_INCLUDE,
    TOKEN_HEADER,

    /* Types */
    TOKEN_INT_TYPE,       /* 1A */
    TOKEN_FLOAT_TYPE,     /* flt */
    TOKEN_BOOL_TYPE,      /* bol */
    TOKEN_LONG_TYPE,      /* lng */
    TOKEN_CHAR_TYPE,      /* char */
    TOKEN_STRING_TYPE,    /* str */

    /* Keywords */
    TOKEN_RETURN,
    TOKEN_TRUE,
    TOKEN_FALSE,

    /* Control flow - needed for any real driver logic (polling
       loops, status-bit checks) */
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,

    /* Identifiers / literals */
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_CHAR,

    /* nw namespace */
    TOKEN_NAMESPACE,      /* nw */
    TOKEN_OUT,
    TOKEN_ENDL,
    TOKEN_CIN,
    TOKEN_LINE,
    TOKEN_TIME,
    TOKEN_COLOR,
    TOKEN_GETKEY,
    TOKEN_CLEAR,
    TOKEN_SCOPE,

    /* low namespace - direct hardware access. Only legal in
       trusted .lnw files; compilerMain.c rejects these tokens
       in a plain .nw file before they ever reach a parser. */
    TOKEN_LOW,             /* low */
    TOKEN_OUT8,            /* out8  - outb(port, value)  */
    TOKEN_IN8,             /* in8   - inb(port)           */
    TOKEN_OUT16,           /* out16 - outw(port, value)  */
    TOKEN_IN16,            /* in16  - inw(port)           */
    TOKEN_MEMORY_WRITE8,   /* memory_write8(address, value) */
    TOKEN_MEMORY_READ8,    /* memory_read8(address)         */
    TOKEN_CLI,             /* cli - disable interrupts */
    TOKEN_STI,             /* sti - enable interrupts  */

    /* Operators */
    TOKEN_SHIFT_LEFT,     /* << */
    TOKEN_SHIFT_RIGHT,    /* >> */

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_ASSIGN,         /* = */

    /* Comparison */
    TOKEN_EQUAL_EQUAL,    /* == */
    TOKEN_NOT_EQUAL,      /* != */
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,

    /* Logical - for combining conditions in if/while */
    TOKEN_LOGICAL_AND,    /* && */
    TOKEN_LOGICAL_OR,     /* || */
    TOKEN_LOGICAL_NOT,    /* !  */

    /* Bitwise - essential for register/status-bit manipulation,
       e.g. "status & 0x80" to test a busy flag */
    TOKEN_BIT_AND,        /* &  */
    TOKEN_BIT_OR,         /* |  */
    TOKEN_BIT_XOR,        /* ^  */
    TOKEN_BIT_NOT,        /* ~  */

    /* Punctuation */
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    /* Errors */
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

    /*
     * Type of the last token returned. Needed so '<' can be told
     * apart: it only starts a #include <...> header right after
     * a TOKEN_INCLUDE, otherwise it's the start of <, <=, or <<.
     */
    TokenType last_type;

} Lexer;


void lexer_init(Lexer* lexer, const char* source);

Token lexer_next(Lexer* lexer);

const char* token_type_name(TokenType type);

#endif