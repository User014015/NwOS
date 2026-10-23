#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"


/* o====================================o
   Format .nwo (Nw Object)            
     NWOB\0           4 byte  (magic)
     version           1 byte
     reserve           3 byte
     code size      4 byte  (LE, uint32)
     data size    4 byets  (LE, uint32)
     point     4 bytes  (LE, uint32)
   o====================================o */

#define NWO_MAGIC       "NWOB"
#define NWO_VERSION     1
#define NWO_FLAG_NORMAL   0x00
#define NWO_FLAG_LOWLEVEL 0x01

#define NWO_MAX_LOOPS 16
#define NWO_MAX_BREAKS 64

typedef struct
{
    FILE* file;

    unsigned int code_size;
    unsigned int data_size;

    int error;

    long loop_start[NWO_MAX_LOOPS];
    int loop_break_base[NWO_MAX_LOOPS];
    int loop_depth;

    long break_patch[NWO_MAX_BREAKS];
    int break_count;

} Codegen;

static void cg_error(Codegen* cg, const char* msg)
{
    fprintf(stderr, "NwC codegen: %s\n", msg);
    cg->error = 1;
}
static void cg_emit_byte(Codegen* cg, unsigned char b)
{
    if (cg->file == NULL)
        return;

    if (fputc(b, cg->file) == EOF)
    {
        cg_error(cg, "write failed");
        return;
    }

    cg->code_size++;
}
static void cg_emit_u32(Codegen* cg, unsigned int v)
{
    cg_emit_byte(cg, (unsigned char)(v & 0xFF));
    cg_emit_byte(cg, (unsigned char)((v >> 8) & 0xFF));
    cg_emit_byte(cg, (unsigned char)((v >> 16) & 0xFF));
    cg_emit_byte(cg, (unsigned char)((v >> 24) & 0xFF));
}
static void cg_emit_u64(Codegen* cg, unsigned long long v)
{
    cg_emit_u32(cg, (unsigned int)(v & 0xFFFFFFFFu));
    cg_emit_u32(cg, (unsigned int)((v >> 32) & 0xFFFFFFFFu));
}
static void cg_emit_double(Codegen* cg, double d)
{
    unsigned long long bits;

    memcpy(&bits, &d, sizeof(bits));

    cg_emit_u64(cg, bits);
}
static void cg_emit_string(Codegen* cg, const char* s)
{
    unsigned int len;

    if (s == NULL)
        s = "";

    len = (unsigned int)strlen(s);

    cg_emit_u32(cg, len);

    for (unsigned int i = 0; i < len; i++)
        cg_emit_byte(cg, (unsigned char)s[i]);
}

static long cg_emit_jump(Codegen* cg, unsigned char opcode)
{
    long position;

    cg_emit_byte(cg, opcode);

    position = ftell(cg->file);

    cg_emit_u32(cg, 0);

    return position;
}

static void cg_patch_jump(Codegen* cg, long position, long target)
{
    unsigned char bytes[4];
    long saved;
    unsigned int value = (unsigned int)target;

    if (cg->file == NULL || position < 0)
        return;

    saved = ftell(cg->file);

    bytes[0] = (unsigned char)(value & 0xFF);
    bytes[1] = (unsigned char)((value >> 8) & 0xFF);
    bytes[2] = (unsigned char)((value >> 16) & 0xFF);
    bytes[3] = (unsigned char)((value >> 24) & 0xFF);

    if (fseek(cg->file, position, SEEK_SET) == 0)
    {
        if (fwrite(bytes, 1, 4, cg->file) != 4)
            cg_error(cg, "jump patch failed");
    }

    if (saved >= 0)
        fseek(cg->file, saved, SEEK_SET);
}

typedef enum
{
    OP_NOP          = 0x00,

    /* Стек */
    OP_PUSH_INT     = 0x10,   /* u64 */
    OP_PUSH_FLOAT   = 0x11,   /* double (8 byte) */
    OP_PUSH_STRING  = 0x12,   /* u32 len + bytes */
    OP_PUSH_CHAR    = 0x13,   /* u32 codepoint */
    OP_PUSH_BOOL    = 0x14,   /* u8 0/1 */
    OP_POP          = 0x15,
    OP_LOAD         = 0x20,   /* u32 len + name */
    OP_STORE        = 0x21,   /* u32 len + name */
    OP_DECLARE      = 0x22,   /* u32 len + name, u8 type */
    OP_DECLARE_ARR  = 0x23,   /* u32 len + name, u8 type, u32 size */
    OP_ADD          = 0x30,
    OP_SUB          = 0x31,
    OP_MUL          = 0x32,
    OP_DIV          = 0x33,
    OP_EQ           = 0x40,
    OP_NEQ          = 0x41,
    OP_LT           = 0x42,
    OP_GT           = 0x43,
    OP_LTE          = 0x44,
    OP_GTE          = 0x45,
    OP_OUT          = 0x50,   /* output upper */
    OP_OUT_ENDL     = 0x51,
    OP_IN           = 0x52,
    OP_LINE_IN      = 0x53,   /* u32 count, then names */
    OP_IN_STORE     = 0x54,   /* u32 len + name */
    OP_TIME         = 0x60,
    OP_COLOR        = 0x61,
    OP_GETKEY       = 0x62,
    OP_CLEAR        = 0x63,
    OP_RECT         = 0x64,
    OP_COLOR_PUSH   = 0x65,
    OP_COLOR_POP    = 0x66,
    OP_SLEEP        = 0x67,
    OP_GETKEY_FILTER= 0x68,
    OP_ASM_HLT      = 0x69,
    OP_DRAW_FRAME   = 0x6A,
    OP_DRAW_CUBE    = 0x6B,
    OP_DRAW_WINDOW  = 0x6C,
    OP_DRAW_TEXT    = 0x6D,
    OP_MOUSE_X      = 0x6E,
    OP_MOUSE_Y      = 0x6F,
    OP_FUNC_BEGIN   = 0x70,   /* u32 name, u8 ret_type, params */
    OP_FUNC_END     = 0x71,
    OP_CALL         = 0x72,   /* u32 name */
    OP_RETURN       = 0x73,
    OP_RETURN_VOID  = 0x74,
    OP_POWEROFF     = 0x75,
    OP_REBOOT       = 0x76,
    OP_MOUSE_BUTTONS= 0x77,
    OP_WALLPAPER    = 0x78,
    OP_MOD          = 0x34,

    OP_AND          = 0x46,
    OP_OR           = 0x47,
    OP_NOT          = 0x48,
    OP_STRCMP       = 0x49,
    OP_STRNCMP      = 0x4A,

    OP_JUMP         = 0x80,
    OP_JUMP_FALSE   = 0x81,

    OP_DUP          = 0x16,

    OP_BREAK        = 0x82,
    OP_CONTINUE     = 0x83,

    OP_HALT         = 0xFF

} OpCode;

static void cg_node(Codegen* cg, ASTNode* node);
static void cg_expression(Codegen* cg, ASTNode* node);
static void cg_statement(Codegen* cg, ASTNode* node);
static int cg_call(Codegen* cg, ASTNode* node);

static unsigned char cg_type_byte(ValueType t)
{
    switch (t)
    {
        case TYPE_INT:       return 1;
        case TYPE_FLOAT:     return 2;
        case TYPE_BOOL:      return 3;
        case TYPE_LONG:      return 4;
        case TYPE_LONG_LONG: return 5;
        case TYPE_CHAR:      return 6;
        case TYPE_STRING:    return 7;
        case TYPE_VOID:      return 0;
        default:             return 0;
    }
}

static int cg_text_equals_ci(const char* a, const char* b)
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


static int cg_key_constant(const char* name, long long* value)
{
    struct KeyName
    {
        const char* name;
        long long value;
    };

    static const struct KeyName keys[] = {
        { "NONE", 0 },
        { "ESC", 255 },
        { "ENTER", 256 },
        { "BACKSPACE", 257 },
        { "TAB", 258 },
        { "UP", 259 },
        { "DOWN", 260 },
        { "LEFT", 261 },
        { "RIGHT", 262 },
        { "SUPER_S", 263 },
        { "SPACE", 32 },
        { "KEY_NONE", 0 },
        { "KEY_ESC", 255 },
        { "KEY_ENTER", 256 },
        { "KEY_BACKSPACE", 257 },
        { "KEY_TAB", 258 },
        { "KEY_UP", 259 },
        { "KEY_DOWN", 260 },
        { "KEY_LEFT", 261 },
        { "KEY_RIGHT", 262 },
        { "KEY_SUPER_S", 263 },
        { "KEY_SPACE", 32 }
    };

    unsigned int i;

    if (name == NULL)
        return 0;

    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    {
        if (strcmp(name, keys[i].name) == 0)
        {
            *value = keys[i].value;
            return 1;
        }
    }

    return 0;
}


static int cg_color_value(Codegen* cg, const char* name)
{
    static const struct
    {
        const char* name;
        int value;
    } colors[] = {
        { "BLACK",           0 },
        { "BLUE",            1 },
        { "GREEN",           2 },
        { "CYAN",            3 },
        { "RED",             4 },
        { "MAGENTA",         5 },
        { "BROWN",           6 },
        { "LIGHT_GRAY",      7 },
        { "LIGHT_GREY",      7 },
        { "GRAY",            7 },
        { "GREY",            7 },
        { "DARK_GRAY",       8 },
        { "DARK_GREY",       8 },
        { "LIGHT_BLUE",      9 },
        { "LIGHT_GREEN",    10 },
        { "LIGHT_CYAN",     11 },
        { "LIGHT_RED",      12 },
        { "LIGHT_MAGENTA",  13 },
        { "YELLOW",         14 },
        { "WHITE",          15 },
        { "DARK_RED",        4 },
        { "DARK_BLUE",       1 },
        { "DARK_GREEN",      2 },
        { "DARK_CYAN",       3 },
        { "DARK_MAGENTA",    5 },
        { "DARK_BROWN",      6 },
        { "DARK_YELLOW",    14 }
    };

    unsigned int i;

    for (i = 0;
         i < sizeof(colors) / sizeof(colors[0]);
         i++)
    {
        if (cg_text_equals_ci(name, colors[i].name))
            return colors[i].value;
    }

    fprintf(
        stderr,
        "NwC codegen: unknown color '%s'\n",
        name
    );

    cg->error = 1;

    return -1;
}


static void cg_binary(Codegen* cg, ASTNode* node)
{
    const char* op = node->text;

    if (op == NULL)
    {
        cg_error(cg, "binary op without text");
        return;
    }

    cg_expression(cg, node->left);
    cg_expression(cg, node->right);

    if (strcmp(op, "+") == 0)      cg_emit_byte(cg, OP_ADD);
    else if (strcmp(op, "-") == 0) cg_emit_byte(cg, OP_SUB);
    else if (strcmp(op, "*") == 0) cg_emit_byte(cg, OP_MUL);
    else if (strcmp(op, "/") == 0) cg_emit_byte(cg, OP_DIV);
    else if (strcmp(op, "==") == 0) cg_emit_byte(cg, OP_EQ);
    else if (strcmp(op, "!=") == 0) cg_emit_byte(cg, OP_NEQ);
    else if (strcmp(op, "<") == 0)  cg_emit_byte(cg, OP_LT);
    else if (strcmp(op, ">") == 0)  cg_emit_byte(cg, OP_GT);
    else if (strcmp(op, "<=") == 0) cg_emit_byte(cg, OP_LTE);
    else if (strcmp(op, ">=") == 0) cg_emit_byte(cg, OP_GTE);
    else
    {
        cg_error(cg, "unknown binary operator");
    }
}


static void cg_expression(Codegen* cg, ASTNode* node)
{
    if (node == NULL)
    {
        cg_error(cg, "null expression");
        return;
    }

    switch (node->type)
    {
        case AST_NUMBER:
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)node->integer_value);
            break;

        case AST_FLOAT:
            cg_emit_byte(cg, OP_PUSH_FLOAT);
            cg_emit_double(cg, node->float_value);
            break;

        case AST_STRING:
            cg_emit_byte(cg, OP_PUSH_STRING);
            cg_emit_string(cg, node->text);
            break;

        case AST_CHAR:
            cg_emit_byte(cg, OP_PUSH_CHAR);
            if (node->text != NULL && node->text[0] != '\0')
                cg_emit_u32(cg, (unsigned int)(unsigned char)node->text[0]);
            else
                cg_emit_u32(cg, 0);
            break;

        case AST_BOOLEAN:
            cg_emit_byte(cg, OP_PUSH_BOOL);
            cg_emit_byte(cg, (unsigned char)(node->integer_value ? 1 : 0));
            break;

        case AST_IDENTIFIER:
        {
            long long key_value;

            if (cg_key_constant(node->text, &key_value))
            {
                cg_emit_byte(cg, OP_PUSH_INT);
                cg_emit_u64(cg, (unsigned long long)key_value);
            }
            else
            {
                cg_emit_byte(cg, OP_LOAD);
                cg_emit_string(cg, node->text);
            }

            break;
        }

        case AST_BINARY_OPERATION:
            cg_binary(cg, node);
            break;

        case AST_EXPRESSION:
            if (node->text != NULL &&
                strcmp(node->text, "nw::endl") == 0)
            {
                cg_emit_byte(cg, OP_OUT_ENDL);
                return;
            }

            if (node->text != NULL &&
                strcmp(node->text, "nw::cin") == 0)
            {
                cg_emit_byte(cg, OP_IN);
                return;
            }

            cg_error(cg, "unsupported special expression");
            break;

        case AST_CALL:
            if (node->text != NULL &&
                strcmp(node->text, "strcmp") == 0)
            {
                ASTNode* arg = node->child;

                if (arg == NULL || arg->next == NULL ||
                    arg->next->next != NULL)
                {
                    cg_error(cg, "strcmp needs two arguments");
                    break;
                }

                cg_expression(cg, arg);
                cg_expression(cg, arg->next);
                cg_emit_byte(cg, OP_STRCMP);
                break;
            }

            if (node->text != NULL &&
                strcmp(node->text, "strncmp") == 0)
            {
                ASTNode* arg = node->child;

                if (arg == NULL || arg->next == NULL ||
                    arg->next->next == NULL ||
                    arg->next->next->next != NULL)
                {
                    cg_error(cg, "strncmp needs three arguments");
                    break;
                }

                cg_expression(cg, arg);
                cg_expression(cg, arg->next);
                cg_expression(cg, arg->next->next);
                cg_emit_byte(cg, OP_STRNCMP);
                break;
            }

            if (!cg_call(cg, node))
            {
                cg_error(cg, "call does not return a value");
            }
            break;

        default:
            cg_error(cg, "unsupported expression node");
            break;
    }
}

static void cg_variable_decl(Codegen* cg, ASTNode* node)
{
    if (node->type == AST_ARRAY_DECLARATION)
    {
        cg_emit_byte(cg, OP_DECLARE_ARR);
        cg_emit_string(cg, node->text);
        cg_emit_byte(cg, cg_type_byte(node->value_type));
        cg_emit_u32(cg, node->array_size);
    }
    else
    {
        cg_emit_byte(cg, OP_DECLARE);
        cg_emit_string(cg, node->text);
        cg_emit_byte(cg, cg_type_byte(node->value_type));
    }
    if (node->child != NULL)
    {
        cg_expression(cg, node->child);

        cg_emit_byte(cg, OP_STORE);
        cg_emit_string(cg, node->text);
    }
}


static void cg_output(Codegen* cg, ASTNode* node)
{
    ASTNode* item = node->child;
    int colored = node->text != NULL;

    if (colored)
    {
        int color = cg_color_value(cg, node->text);

        if (color < 0)
            return;

        cg_emit_byte(cg, OP_COLOR_PUSH);
        cg_emit_byte(cg, OP_PUSH_INT);
        cg_emit_u64(cg, (unsigned long long)color);
        cg_emit_byte(cg, OP_COLOR);
    }

    while (item != NULL)
    {
        if (item->type == AST_EXPRESSION &&
            item->text != NULL &&
            strcmp(item->text, "nw::endl") == 0)
        {
            cg_emit_byte(cg, OP_OUT_ENDL);
        }
        else
        {
            cg_expression(cg, item);
            cg_emit_byte(cg, OP_OUT);
        }

        item = item->next;
    }

    if (colored)
    {
        if (!node->integer_value)
            cg_emit_byte(cg, OP_OUT_ENDL);

        cg_emit_byte(cg, OP_COLOR_POP);
    }
}


static int cg_rgb_color_value(const char* name)
{
    static const struct
    {
        const char* name;
        int value;
    } colors[] = {
        { "BLACK",         0x000000 },
        { "WHITE",         0xFFFFFF },
        { "RED",           0xFF0000 },
        { "DARK_RED",      0x800000 },
        { "LIGHT_RED",     0xFF8080 },
        { "GREEN",         0x00FF00 },
        { "DARK_GREEN",    0x008000 },
        { "LIGHT_GREEN",   0x80FF80 },
        { "BLUE",          0x0000FF },
        { "DARK_BLUE",     0x000080 },
        { "LIGHT_BLUE",    0x8080FF },
        { "CYAN",          0x00FFFF },
        { "DARK_CYAN",     0x008080 },
        { "LIGHT_CYAN",    0x80FFFF },
        { "MAGENTA",       0xFF00FF },
        { "DARK_MAGENTA",  0x800080 },
        { "LIGHT_MAGENTA", 0xFF80FF },
        { "YELLOW",        0xFFFF00 },
        { "BROWN",         0x808000 },
        { "ORANGE",        0xFFA500 },
        { "PURPLE",        0x800080 },
        { "PINK",          0xFFC0CB },
        { "GRAY",          0x808080 },
        { "GREY",          0x808080 },
        { "LIGHT_GRAY",    0xC0C0C0 },
        { "LIGHT_GREY",    0xC0C0C0 },
        { "DARK_GRAY",     0x404040 },
        { "DARK_GREY",     0x404040 },
        { "NAVY",          0x000080 },
        { "TEAL",          0x008080 },
        { "LIME",          0x00FF00 },
        { "MAROON",        0x800000 }
    };

    unsigned int i;

    if (name == NULL)
        return -1;

    for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++)
    {
        if (cg_text_equals_ci(name, colors[i].name))
            return colors[i].value;
    }

    return -1;
}


static void cg_input(Codegen* cg, ASTNode* node)
{
    if (node->child != NULL &&
        node->child->type == AST_IDENTIFIER)
    {
        cg_emit_byte(cg, OP_IN_STORE);
        cg_emit_string(cg, node->child->text);
    }
    else
    {
        cg_error(cg, "nw::cin requires identifier target");
    }
}


static void cg_line_input(Codegen* cg, ASTNode* node)
{
    ASTNode* arg = node->child;

    if (arg == NULL)
    {
        cg_error(cg, "nw::line needs arguments");
        return;
    }
    ASTNode* target = NULL;

    while (arg != NULL)
    {
        if (arg->type == AST_IDENTIFIER)
            target = arg;

        arg = arg->next;
    }

    if (target == NULL)
    {
        cg_error(cg, "nw::line needs a variable target");
        return;
    }

    cg_emit_byte(cg, OP_LINE_IN);
    cg_emit_u32(cg, 1);
    cg_emit_string(cg, target->text);
}


static void cg_return(Codegen* cg, ASTNode* node)
{
    if (node->child != NULL)
    {
        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_RETURN);
    }
    else
    {
        cg_emit_byte(cg, OP_RETURN_VOID);
    }
}


static int cg_call(Codegen* cg, ASTNode* node)
{
    if (node->text == NULL)
    {
        cg_error(cg, "call without name");
        return 0;
    }

    if (strcmp(node->text, "nw::time") == 0)
    {
        cg_emit_byte(cg, OP_TIME);
        return 0;
    }

    if (strcmp(node->text, "nw::color") == 0)
    {
        if (node->child != NULL)
            cg_expression(cg, node->child);

        cg_emit_byte(cg, OP_COLOR);
        return 0;
    }

    if (strcmp(node->text, "nw::getkey") == 0)
    {
        if (node->child != NULL)
        {
            cg_expression(cg, node->child);
            cg_emit_byte(cg, OP_GETKEY_FILTER);
        }
        else
        {
            cg_emit_byte(cg, OP_GETKEY);
        }

        return 1;
    }

    if (strcmp(node->text, "nw::mouse_x") == 0)
    {
        cg_emit_byte(cg, OP_MOUSE_X);
        return 1;
    }

    if (strcmp(node->text, "nw::mouse_y") == 0)
    {
        cg_emit_byte(cg, OP_MOUSE_Y);
        return 1;
    }

    if (strcmp(node->text, "nw::mouse_buttons") == 0)
    {
        cg_emit_byte(cg, OP_MOUSE_BUTTONS);
        return 1;
    }

    if (strcmp(node->text, "nw::poweroff") == 0)
    {
        cg_emit_byte(cg, OP_POWEROFF);
        return 0;
    }

    if (strcmp(node->text, "nw::reboot") == 0)
    {
        cg_emit_byte(cg, OP_REBOOT);
        return 0;
    }

    if (strcmp(node->text, "nw::clear") == 0 ||
        strcmp(node->text, "clear_screen") == 0 ||
        strcmp(node->text, "nw::clear_screen") == 0)
    {
        cg_emit_byte(cg, OP_CLEAR);
        return 0;
    }

    if (strcmp(node->text, "sleep") == 0)
    {
        if (node->child == NULL)
        {
            cg_error(cg, "sleep needs milliseconds");
            return 0;
        }

        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_SLEEP);
        return 0;
    }

    if (strcmp(node->text, "nw::window") == 0)
    {
        ASTNode* arg = node->child;
        int count = 0;

        while (arg != NULL)
        {
            count++;
            arg = arg->next;
        }

        if (count != 6)
        {
            cg_error(cg, "nw::window needs x, y, width, height, color, title");
            return 0;
        }

        arg = node->child;

        while (arg != NULL)
        {
            cg_expression(cg, arg);
            arg = arg->next;
        }

        cg_emit_byte(cg, OP_DRAW_WINDOW);
        return 0;
    }

    if (strcmp(node->text, "nw::text") == 0)
    {
        ASTNode* arg = node->child;
        int count = 0;

        while (arg != NULL)
        {
            count++;
            arg = arg->next;
        }

        if (count != 4)
        {
            cg_error(cg, "nw::text needs x, y, color, text");
            return 0;
        }

        arg = node->child;

        while (arg != NULL)
        {
            cg_expression(cg, arg);
            arg = arg->next;
        }

        cg_emit_byte(cg, OP_DRAW_TEXT);
        return 0;
    }

    if (strcmp(node->text, "nw::wallpaper") == 0)
    {
        cg_emit_byte(cg, OP_WALLPAPER);
        return 0;
    }

    if (strcmp(node->text, "nw::rect") == 0)
    {
        ASTNode* arg = node->child;
        int count = 0;

        while (arg != NULL)
        {
            count++;
            arg = arg->next;
        }

        if (count != 5)
        {
            cg_error(cg, "nw::rect needs x, y, width, height, color");
            return 0;
        }

        arg = node->child;

        while (arg != NULL)
        {
            cg_expression(cg, arg);
            arg = arg->next;
        }

        cg_emit_byte(cg, OP_RECT);
        return 0;
    }

    if (node->text != NULL && strncmp(node->text, "nw::draw:", 9) == 0)
    {
        const char* shape = node->text + 9;
        ASTNode* arg = node->child;
        long long x = 0, y = 0, x2 = -1, y2 = -1;
        long long w = 100, h = 100;
        int has_x2 = 0, has_y2 = 0;
        int color = -1;

        if (node->text_color_name != NULL)
            color = cg_rgb_color_value(node->text_color_name);

        while (arg != NULL)
        {
            if (arg->text != NULL && strncmp(arg->text, "K:", 2) == 0)
            {
                const char* p = arg->text + 2;
                const char* v = strchr(p, '=');

                if (v != NULL)
                {
                    long long n = strtoll(v + 1, NULL, 0);

                    if (strncmp(p, "x2=", 3) == 0) { x2 = n; has_x2 = 1; }
                    else if (strncmp(p, "y2=", 3) == 0) { y2 = n; has_y2 = 1; }
                    else if (strncmp(p, "x=", 2) == 0) x = n;
                    else if (strncmp(p, "y=", 2) == 0) y = n;
                    else if (strncmp(p, "w=", 2) == 0) w = n;
                    else if (strncmp(p, "h=", 2) == 0) h = n;
                    else if (strncmp(p, "width=", 6) == 0) w = n;
                    else if (strncmp(p, "height=", 7) == 0) h = n;
                    else if (strncmp(p, "size=", 5) == 0) w = h = n;
                }
            }

            arg = arg->next;
        }

        if (has_x2 && x2 >= x) w = x2 - x;
        if (has_y2 && y2 >= y) h = y2 - y;

        int no_window = (node->draw_flags & 1) != 0;

        if (color < 0)
        {
            if (node->text_color_name != NULL)
            {
                cg_error(cg, "unknown draw color");
                return 0;
            }

            if (cg_text_equals_ci(shape, "CUBE"))
                color = 0x0000FF;
            else if (cg_text_equals_ci(shape, "FRAME") && !no_window)
                color = 0x000080;
            else
                color = 0xFFFFFF;
        }

        if (cg_text_equals_ci(shape, "FRAME") && !no_window)
        {
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)x);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)y);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)w);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)h);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)color);
            cg_emit_byte(cg, OP_PUSH_STRING);
            cg_emit_string(cg, node->text_label != NULL ? node->text_label : "");
            cg_emit_byte(cg, OP_DRAW_WINDOW);
            return 0;
        }

        if (cg_text_equals_ci(shape, "FRAME") ||
            cg_text_equals_ci(shape, "RECT"))
        {
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)x);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)y);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)w);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)h);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)color);
            cg_emit_byte(cg, OP_DRAW_FRAME);
            return 0;
        }

        if (cg_text_equals_ci(shape, "TEXT"))
        {
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)x);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)y);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)color);
            cg_emit_byte(cg, OP_PUSH_STRING);
            cg_emit_string(cg, node->text_label != NULL ? node->text_label : "");
            cg_emit_byte(cg, OP_DRAW_TEXT);
            return 0;
        }

        if (cg_text_equals_ci(shape, "CUBE"))
        {
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)h);
            cg_emit_byte(cg, OP_PUSH_INT);
            cg_emit_u64(cg, (unsigned long long)color);
            cg_emit_byte(cg, OP_DRAW_CUBE);
            return 0;
        }

        cg_error(cg, "unknown draw shape");
        return 0;
    }

    {
        ASTNode* arg = node->child;
        int count = 0;

        while (arg != NULL)
        {
            cg_expression(cg, arg);
            count++;
            arg = arg->next;
        }

        cg_emit_byte(cg, OP_CALL);
        cg_emit_string(cg, node->text);
        cg_emit_u32(cg, (unsigned int)count);
    }

    return 1;
}

static void cg_if(Codegen* cg, ASTNode* node)
{
    long false_jump;

    cg_expression(cg, node->child);

    false_jump = cg_emit_jump(cg, OP_JUMP_FALSE);

    cg_statement(cg, node->left);

    if (node->right != NULL)
    {
        long end_jump = cg_emit_jump(cg, OP_JUMP);

        cg_patch_jump(cg, false_jump, ftell(cg->file));

        cg_statement(cg, node->right);

        cg_patch_jump(cg, end_jump, ftell(cg->file));
    }
    else
    {
        cg_patch_jump(cg, false_jump, ftell(cg->file));
    }
}

static void cg_while(Codegen* cg, ASTNode* node)
{
    int depth = cg->loop_depth;
    int break_base;
    long loop_start;
    long exit_jump;
    long loop_end;
    int i;

    if (depth >= NWO_MAX_LOOPS)
    {
        cg_error(cg, "too many nested loops");
        return;
    }

    loop_start = ftell(cg->file);
    break_base = cg->break_count;

    cg->loop_start[depth] = loop_start;
    cg->loop_break_base[depth] = break_base;
    cg->loop_depth = depth + 1;

    cg_expression(cg, node->child);

    exit_jump = cg_emit_jump(cg, OP_JUMP_FALSE);

    cg_statement(cg, node->left);

    cg_emit_byte(cg, OP_JUMP);
    cg_emit_u32(cg, (unsigned int)loop_start);

    loop_end = ftell(cg->file);

    cg_patch_jump(cg, exit_jump, loop_end);

    for (i = break_base; i < cg->break_count; i++)
        cg_patch_jump(cg, cg->break_patch[i], loop_end);

    cg->break_count = break_base;
    cg->loop_depth = depth;
}

static void cg_statement(Codegen* cg, ASTNode* node)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
        case AST_VARIABLE_DECLARATION:
        case AST_ARRAY_DECLARATION:
            cg_variable_decl(cg, node);
            break;

        case AST_OUTPUT:
            cg_output(cg, node);
            break;

        case AST_INPUT:
            cg_input(cg, node);
            break;

        case AST_LINE_INPUT:
            cg_line_input(cg, node);
            break;

        case AST_RETURN:
            cg_return(cg, node);
            break;

        case AST_ASSIGNMENT:
            cg_expression(cg, node->child);
            cg_emit_byte(cg, OP_STORE);
            cg_emit_string(cg, node->text);
            break;

        case AST_ASM:
            if (node->text != NULL &&
                strcmp(node->text, "hlt") == 0)
            {
                cg_emit_byte(cg, OP_ASM_HLT);
            }
            else
            {
                cg_error(cg, "unsupported asm instruction");
            }
            break;

        case AST_IF:
            cg_if(cg, node);
            break;

        case AST_WHILE:
            cg_while(cg, node);
            break;

        case AST_BREAK:
            if (cg->loop_depth == 0)
            {
                cg_error(cg, "break outside loop");
                break;
            }

            if (cg->break_count >= NWO_MAX_BREAKS)
            {
                cg_error(cg, "too many break statements");
                break;
            }

            cg->break_patch[cg->break_count++] =
                cg_emit_jump(cg, OP_JUMP);
            break;

        case AST_CONTINUE:
            if (cg->loop_depth == 0)
            {
                cg_error(cg, "continue outside loop");
                break;
            }

            cg_emit_byte(cg, OP_JUMP);
            cg_emit_u32(
                cg,
                (unsigned int)
                cg->loop_start[cg->loop_depth - 1]
            );
            break;

        case AST_CALL:
            if (cg_call(cg, node))
                cg_emit_byte(cg, OP_POP);
            break;

        case AST_BLOCK:
            cg_node(cg, node->child);
            break;

        default:
            cg_expression(cg, node);
            cg_emit_byte(cg, OP_POP);
            break;
    }
}


static void cg_function(Codegen* cg, ASTNode* node)
{
    ASTNode* param;
    int param_count = 0;

    if (node->text == NULL)
    {
        cg_error(cg, "function without name");
        return;
    }

    param = node->left;

    while (param != NULL)
    {
        param_count++;
        param = param->next;
    }

    if (param_count > 16)
    {
        cg_error(cg, "too many function parameters (max 16)");
        return;
    }

    cg_emit_byte(cg, OP_FUNC_BEGIN);
    cg_emit_string(cg, node->text);
    cg_emit_byte(cg, cg_type_byte(node->value_type));
    cg_emit_byte(cg, (unsigned char)param_count);

    param = node->left;

    while (param != NULL)
    {
        cg_emit_string(cg, param->text);
        cg_emit_byte(cg, cg_type_byte(param->value_type));
        cg_emit_byte(
            cg,
            (param->type == AST_ARRAY_DECLARATION ||
             param->array_size > 0) ? 1 : 0
        );
        param = param->next;
    }

    /* body */
    if (node->child != NULL &&
        node->child->type == AST_BLOCK)
    {
        cg_node(cg, node->child->child);
    }
    else
    {
        cg_node(cg, node->child);
    }

    cg_emit_byte(cg, OP_FUNC_END);
}

static void cg_node(Codegen* cg, ASTNode* node)
{
    while (node != NULL && !cg->error)
    {
        switch (node->type)
        {
            case AST_INCLUDE:
                break;

            case AST_FUNCTION:
                cg_function(cg, node);
                break;

            case AST_BLOCK:
                cg_node(cg, node->child);
                break;

            default:
                cg_statement(cg, node);
                break;
        }

        node = node->next;
    }
}

static int cg_write_header(
    FILE* f,
    unsigned int code_size,
    unsigned int data_size)
{
    unsigned char version = NWO_VERSION;
    unsigned char reserved[3] = { 0, 0, 0 };

    if (fwrite(NWO_MAGIC, 1, 4, f) != 4) return 0;
    if (fwrite(&version, 1, 1, f) != 1)  return 0;
    if (fwrite(reserved, 1, 3, f) != 3)  return 0;

    unsigned char b[4];

    b[0] = (unsigned char)(code_size & 0xFF);
    b[1] = (unsigned char)((code_size >> 8) & 0xFF);
    b[2] = (unsigned char)((code_size >> 16) & 0xFF);
    b[3] = (unsigned char)((code_size >> 24) & 0xFF);
    if (fwrite(b, 1, 4, f) != 4) return 0;

    b[0] = (unsigned char)(data_size & 0xFF);
    b[1] = (unsigned char)((data_size >> 8) & 0xFF);
    b[2] = (unsigned char)((data_size >> 16) & 0xFF);
    b[3] = (unsigned char)((data_size >> 24) & 0xFF);
    if (fwrite(b, 1, 4, f) != 4) return 0;

    b[0] = b[1] = b[2] = b[3] = 0;
    if (fwrite(b, 1, 4, f) != 4) return 0;

    return 1;
}

int codegen_write_nwo(ASTNode* ast, const char* path, int flags)
{
    Codegen cg;
    FILE* f;
    FILE* code_tmp;

    (void)flags;

    if (ast == NULL || path == NULL)
    {
        fprintf(stderr, "NwC codegen: invalid arguments\n");
        return 0;
    }

    if (ast->type != AST_PROGRAM)
    {
        fprintf(stderr, "NwC codegen: expected program node\n");
        return 0;
    }

    code_tmp = tmpfile();

    if (code_tmp == NULL)
    {
        fprintf(stderr, "NwC codegen: cannot create temp file\n");
        return 0;
    }

    cg.file = code_tmp;
    cg.code_size = 0;
    cg.data_size = 0;
    cg.error = 0;
    cg.loop_depth = 0;
    cg.break_count = 0;

    cg_node(&cg, ast->child);

    if (cg.error)
    {
        fclose(code_tmp);
        return 0;
    }
    cg_emit_byte(&cg, OP_HALT);

    if (cg.error)
    {
        fclose(code_tmp);
        return 0;
    }
    f = fopen(path, "wb");

    if (f == NULL)
    {
        fprintf(stderr,
                "NwC codegen: cannot open '%s' for writing\n",
                path);

        fclose(code_tmp);
        return 0;
    }
    if (!cg_write_header(f, cg.code_size, cg.data_size))
    {
        fprintf(stderr,
                "NwC codegen: failed to write header\n");

        fclose(f);
        fclose(code_tmp);
        return 0;
    }
    if (fseek(code_tmp, 0, SEEK_SET) != 0)
    {
        fclose(f);
        fclose(code_tmp);
        return 0;
    }

    {
        unsigned char buf[4096];
        size_t n;

        while ((n = fread(buf, 1, sizeof(buf), code_tmp)) > 0)
        {
            if (fwrite(buf, 1, n, f) != n)
            {
                fprintf(stderr,
                        "NwC codegen: write failed\n");

                fclose(f);
                fclose(code_tmp);
                return 0;
            }
        }
    }

    fclose(f);
    fclose(code_tmp);

    return 1;
}