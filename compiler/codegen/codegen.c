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

typedef struct
{
    FILE* file;

    unsigned int code_size;
    unsigned int data_size;

    int error;

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
    OP_TIME         = 0x60,
    OP_COLOR        = 0x61,
    OP_GETKEY       = 0x62,
    OP_CLEAR        = 0x63,
    OP_FUNC_BEGIN   = 0x70,   /* u32 name, u8 ret_type */
    OP_FUNC_END     = 0x71,
    OP_CALL         = 0x72,   /* u32 name */
    OP_RETURN       = 0x73,
    OP_RETURN_VOID  = 0x74,

    /* program */
    OP_HALT         = 0xFF

} OpCode;

static void cg_node(Codegen* cg, ASTNode* node);
static void cg_expression(Codegen* cg, ASTNode* node);

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

static void cg_binary(Codegen* cg, ASTNode* node)
{
    const char* op = node->text;

    if (op == NULL)
    {
        cg_error(cg, "binary op without text");
        return;
    }

    /* левый, правый, потом операция */
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
            cg_emit_byte(cg, OP_LOAD);
            cg_emit_string(cg, node->text);
            break;

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
            cg_emit_byte(cg, OP_CALL);
            cg_emit_string(cg, node->text);
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
}


static void cg_input(Codegen* cg, ASTNode* node)
{
    cg_emit_byte(cg, OP_IN);

    if (node->child != NULL &&
        node->child->type == AST_IDENTIFIER)
    {
        cg_emit_byte(cg, OP_STORE);
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


static void cg_call(Codegen* cg, ASTNode* node)
{
    if (node->text == NULL)
    {
        cg_error(cg, "call without name");
        return;
    }

    if (strcmp(node->text, "nw::time") == 0)
    {
        cg_emit_byte(cg, OP_TIME);
        return;
    }

    if (strcmp(node->text, "nw::color") == 0)
    {
        if (node->child != NULL)
            cg_expression(cg, node->child);

        cg_emit_byte(cg, OP_COLOR);
        return;
    }

    if (strcmp(node->text, "nw::getkey") == 0)
    {
        cg_emit_byte(cg, OP_GETKEY);
        return;
    }

    if (strcmp(node->text, "nw::clear") == 0)
    {
        cg_emit_byte(cg, OP_CLEAR);
        return;
    }
    cg_emit_byte(cg, OP_CALL);
    cg_emit_string(cg, node->text);
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

        case AST_CALL:
            cg_call(cg, node);
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


// functions

static void cg_function(Codegen* cg, ASTNode* node)
{
    if (node->text == NULL)
    {
        cg_error(cg, "function without name");
        return;
    }

    cg_emit_byte(cg, OP_FUNC_BEGIN);
    cg_emit_string(cg, node->text);
    cg_emit_byte(cg, cg_type_byte(node->value_type));

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

    /* entry point = 0 */
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