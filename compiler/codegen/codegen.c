#include "nwc_runtime.h"
#include "codegen.h"

#define NWO_MAGIC       "NWOB"
#define NWO_VERSION     1

#define NWC_CODE_CAPACITY 131072
#define NWC_MAX_LOOP_DEPTH 32
#define NWC_MAX_PATCHES 128

#define OP_NOP          0x00

#define OP_PUSH_INT     0x10
#define OP_PUSH_FLOAT   0x11
#define OP_PUSH_STRING  0x12
#define OP_PUSH_CHAR    0x13
#define OP_PUSH_BOOL    0x14
#define OP_POP          0x15
#define OP_DUP          0x16

#define OP_LOAD         0x20
#define OP_STORE        0x21
#define OP_DECLARE      0x22
#define OP_DECLARE_ARR  0x23

#define OP_ADD          0x30
#define OP_SUB          0x31
#define OP_MUL          0x32
#define OP_DIV          0x33
#define OP_MOD          0x34

#define OP_EQ           0x40
#define OP_NEQ          0x41
#define OP_LT           0x42
#define OP_GT           0x43
#define OP_LTE          0x44
#define OP_GTE          0x45
#define OP_AND          0x46
#define OP_OR           0x47
#define OP_NOT          0x48

#define OP_OUT          0x50
#define OP_OUT_ENDL     0x51
#define OP_IN           0x52
#define OP_LINE_IN      0x53

#define OP_TIME         0x60
#define OP_COLOR        0x61
#define OP_GETKEY       0x62
#define OP_CLEAR        0x63
#define OP_RANDOM       0x64

#define OP_FUNC_BEGIN   0x70
#define OP_FUNC_END     0x71
#define OP_CALL         0x72
#define OP_RETURN       0x73
#define OP_RETURN_VOID  0x74

#define OP_JUMP         0x80
#define OP_JUMP_FALSE   0x81
#define OP_BREAK        0x82
#define OP_CONTINUE     0x83

#define OP_HALT         0xFF

typedef struct
{
    unsigned char code[NWC_CODE_CAPACITY];
    unsigned int size;
    int error;
    const char* error_message;
    int loop_depth;
} Codegen;

typedef struct
{
    int continue_patch_count;
    unsigned int continue_patches[NWC_MAX_PATCHES];

    int break_patch_count;
    unsigned int break_patches[NWC_MAX_PATCHES];
} LoopContext;

static LoopContext loop_stack[NWC_MAX_LOOP_DEPTH];

static unsigned char* output_buffer = 0;
static unsigned int output_capacity = 0;
static unsigned int* output_size = 0;

static void cg_error(Codegen* cg, const char* message)
{
    if (!cg->error)
    {
        cg->error = 1;
        cg->error_message = message;
    }
}

static int cg_emit_byte(Codegen* cg, unsigned char value)
{
    if (cg->size >= NWC_CODE_CAPACITY)
    {
        cg_error(cg, "NWO code is too large");
        return 0;
    }

    cg->code[cg->size++] = value;
    return 1;
}

static int cg_emit_u32(Codegen* cg, unsigned int value)
{
    return cg_emit_byte(cg, (unsigned char)(value & 0xFFu)) &&
           cg_emit_byte(cg, (unsigned char)((value >> 8) & 0xFFu)) &&
           cg_emit_byte(cg, (unsigned char)((value >> 16) & 0xFFu)) &&
           cg_emit_byte(cg, (unsigned char)((value >> 24) & 0xFFu));
}

static int cg_emit_u64(Codegen* cg, unsigned long long value)
{
    return cg_emit_u32(cg, (unsigned int)(value & 0xFFFFFFFFu)) &&
           cg_emit_u32(cg, (unsigned int)(value >> 32));
}

static int cg_emit_double(Codegen* cg, double value)
{
    unsigned long long bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return cg_emit_u64(cg, bits);
}

static int cg_emit_string(Codegen* cg, const char* text)
{
    unsigned int length;
    unsigned int i;

    if (text == 0)
        text = "";

    length = (unsigned int)strlen(text);

    if (length > 0xFFFFFFFFu - 4u)
    {
        cg_error(cg, "string is too large");
        return 0;
    }

    if (!cg_emit_u32(cg, length))
        return 0;

    for (i = 0; i < length; i++)
    {
        if (!cg_emit_byte(cg, (unsigned char)text[i]))
            return 0;
    }

    return 1;
}

static unsigned char cg_type_byte(ValueType type)
{
    switch (type)
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

static int cg_patch_u32(Codegen* cg, unsigned int operand_pos, unsigned int value)
{
    if (operand_pos + 4u > cg->size)
    {
        cg_error(cg, "invalid jump patch position");
        return 0;
    }

    cg->code[operand_pos + 0] =
        (unsigned char)(value & 0xFFu);
    cg->code[operand_pos + 1] =
        (unsigned char)((value >> 8) & 0xFFu);
    cg->code[operand_pos + 2] =
        (unsigned char)((value >> 16) & 0xFFu);
    cg->code[operand_pos + 3] =
        (unsigned char)((value >> 24) & 0xFFu);

    return 1;
}

static unsigned int cg_emit_jump(Codegen* cg, unsigned char opcode)
{
    unsigned int operand_pos;

    cg_emit_byte(cg, opcode);
    operand_pos = cg->size;
    cg_emit_u32(cg, 0);

    return operand_pos;
}

static int cg_loop_push(Codegen* cg)
{
    if (cg->loop_depth >= NWC_MAX_LOOP_DEPTH)
    {
        cg_error(cg, "loop nesting is too deep");
        return 0;
    }

    loop_stack[cg->loop_depth].continue_patch_count = 0;
    loop_stack[cg->loop_depth].break_patch_count = 0;
    cg->loop_depth++;
    return 1;
}

static void cg_loop_pop(Codegen* cg, unsigned int continue_target, unsigned int break_target)
{
    LoopContext* loop;

    if (cg->loop_depth <= 0)
        return;

    loop = &loop_stack[cg->loop_depth - 1];

    {
        int i;

        for (i = 0; i < loop->continue_patch_count; i++)
            cg_patch_u32(
                cg,
                loop->continue_patches[i],
                continue_target
            );

        for (i = 0; i < loop->break_patch_count; i++)
            cg_patch_u32(
                cg,
                loop->break_patches[i],
                break_target
            );
    }

    cg->loop_depth--;
}

static void cg_expression(Codegen* cg, ASTNode* node);
static void cg_statement(Codegen* cg, ASTNode* node);
static void cg_node(Codegen* cg, ASTNode* node);

static int cg_count_list(ASTNode* node)
{
    int count = 0;

    while (node != 0)
    {
        count++;
        node = node->next;
    }

    return count;
}

static void cg_binary(Codegen* cg, ASTNode* node)
{
    const char* op = node->text;

    if (op == 0)
    {
        cg_error(cg, "binary operation has no operator");
        return;
    }

    cg_expression(cg, node->left);
    cg_expression(cg, node->right);

    if (cg->error)
        return;

    if (strcmp(op, "+") == 0)        cg_emit_byte(cg, OP_ADD);
    else if (strcmp(op, "-") == 0)   cg_emit_byte(cg, OP_SUB);
    else if (strcmp(op, "*") == 0)   cg_emit_byte(cg, OP_MUL);
    else if (strcmp(op, "/") == 0)   cg_emit_byte(cg, OP_DIV);
    else if (strcmp(op, "%") == 0)   cg_emit_byte(cg, OP_MOD);
    else if (strcmp(op, "==") == 0)  cg_emit_byte(cg, OP_EQ);
    else if (strcmp(op, "!=") == 0)  cg_emit_byte(cg, OP_NEQ);
    else if (strcmp(op, "<") == 0)   cg_emit_byte(cg, OP_LT);
    else if (strcmp(op, ">") == 0)   cg_emit_byte(cg, OP_GT);
    else if (strcmp(op, "<=") == 0)  cg_emit_byte(cg, OP_LTE);
    else if (strcmp(op, ">=") == 0)  cg_emit_byte(cg, OP_GTE);
    else if (strcmp(op, "&&") == 0)  cg_emit_byte(cg, OP_AND);
    else if (strcmp(op, "||") == 0)  cg_emit_byte(cg, OP_OR);
    else
        cg_error(cg, "unknown binary operator");
}

static void cg_call(Codegen* cg, ASTNode* node, int* has_value)
{
    const char* name = node->text;
    ASTNode* arg = node->child;
    unsigned int argc = 0;

    *has_value = 0;

    if (name == 0)
    {
        cg_error(cg, "call has no name");
        return;
    }

    if (strcmp(name, "nw::time") == 0)
    {
        if (node->child == 0 ||
            node->child->next != 0)
        {
            cg_error(cg, "nw::time expects one argument");
            return;
        }

        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_TIME);
        return;
    }

    if (strcmp(name, "nw::color") == 0)
    {
        if (node->child == 0 ||
            node->child->next != 0)
        {
            cg_error(cg, "nw::color expects one argument");
            return;
        }

        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_COLOR);
        return;
    }

    if (strcmp(name, "nw::getkey") == 0)
    {
        if (node->child != 0)
        {
            cg_error(cg, "nw::getkey expects no arguments");
            return;
        }

        cg_emit_byte(cg, OP_GETKEY);
        *has_value = 1;
        return;
    }

    if (strcmp(name, "nw::clear") == 0)
    {
        if (node->child != 0)
        {
            cg_error(cg, "nw::clear expects no arguments");
            return;
        }

        cg_emit_byte(cg, OP_CLEAR);
        return;
    }

    if (strcmp(name, "nw::random") == 0)
    {
        if (node->child == 0 ||
            node->child->next == 0 ||
            node->child->next->next != 0)
        {
            cg_error(cg, "nw::random expects two arguments");
            return;
        }

        cg_expression(cg, node->child);
        cg_expression(cg, node->child->next);
        cg_emit_byte(cg, OP_RANDOM);
        *has_value = 1;
        return;
    }

    while (arg != 0)
    {
        cg_expression(cg, arg);
        argc++;
        arg = arg->next;
    }

    if (argc > 0xFFFFFFFFu)
    {
        cg_error(cg, "too many function arguments");
        return;
    }

    cg_emit_byte(cg, OP_CALL);
    cg_emit_string(cg, name);
    cg_emit_u32(cg, argc);
    *has_value = 1;
}

static void cg_expression(Codegen* cg, ASTNode* node)
{
    int has_value;

    if (node == 0)
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

            if (node->text != 0 &&
                node->text[0] != '\0')
            {
                cg_emit_u32(
                    cg,
                    (unsigned int)(unsigned char)node->text[0]
                );
            }
            else
            {
                cg_emit_u32(cg, 0);
            }
            break;

        case AST_BOOLEAN:
            cg_emit_byte(cg, OP_PUSH_BOOL);
            cg_emit_byte(
                cg,
                (unsigned char)(node->integer_value ? 1 : 0)
            );
            break;

        case AST_IDENTIFIER:
            cg_emit_byte(cg, OP_LOAD);
            cg_emit_string(cg, node->text);
            break;

        case AST_BINARY_OPERATION:
            cg_binary(cg, node);
            break;

        case AST_UNARY_OPERATION:
            if (node->text != 0 &&
                strcmp(node->text, "-") == 0)
            {
                cg_emit_byte(cg, OP_PUSH_INT);
                cg_emit_u64(cg, 0);
                cg_expression(cg, node->child);
                cg_emit_byte(cg, OP_SUB);
            }
            else if (node->text != 0 &&
                     strcmp(node->text, "!") == 0)
            {
                cg_expression(cg, node->child);
                cg_emit_byte(cg, OP_NOT);
            }
            else
            {
                cg_error(cg, "unknown unary operator");
            }
            break;

        case AST_CALL:
            cg_call(cg, node, &has_value);
            break;

        case AST_EXPRESSION:
            if (node->text != 0 &&
                strcmp(node->text, "nw::endl") == 0)
            {
                cg_emit_byte(cg, OP_OUT_ENDL);
            }
            else if (node->text != 0 &&
                     strcmp(node->text, "nw::cin") == 0)
            {
                cg_emit_byte(cg, OP_IN);
            }
            else
            {
                cg_error(cg, "unsupported special expression");
            }
            break;

        default:
            cg_error(cg, "unsupported expression node");
            break;
    }
}

static void cg_variable_decl(Codegen* cg, ASTNode* node)
{
    if (node->text == 0)
    {
        cg_error(cg, "variable has no name");
        return;
    }

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

    if (node->child != 0)
    {
        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_STORE);
        cg_emit_string(cg, node->text);
    }
}

static void cg_assignment(Codegen* cg, ASTNode* node)
{
    const char* op = node->text;
    ASTNode* left = node->left;

    if (left == 0 || left->text == 0)
    {
        cg_error(cg, "assignment target is missing");
        return;
    }

    if (op == 0)
        op = "=";

    if (strcmp(op, "=") == 0)
    {
        cg_expression(cg, node->right);
    }
    else
    {
        cg_emit_byte(cg, OP_LOAD);
        cg_emit_string(cg, left->text);
        cg_expression(cg, node->right);

        if (strcmp(op, "+=") == 0)       cg_emit_byte(cg, OP_ADD);
        else if (strcmp(op, "-=") == 0)  cg_emit_byte(cg, OP_SUB);
        else if (strcmp(op, "*=") == 0)  cg_emit_byte(cg, OP_MUL);
        else if (strcmp(op, "/=") == 0)  cg_emit_byte(cg, OP_DIV);
        else
        {
            cg_error(cg, "unknown assignment operator");
            return;
        }
    }

    cg_emit_byte(cg, OP_STORE);
    cg_emit_string(cg, left->text);
}

static void cg_output(Codegen* cg, ASTNode* node)
{
    ASTNode* item = node->child;

    while (item != 0)
    {
        if (item->type == AST_EXPRESSION &&
            item->text != 0 &&
            strcmp(item->text, "nw::endl") == 0)
        {
            cg_emit_byte(cg, OP_OUT_ENDL);
        }
        else
        {
            cg_expression(cg, item);

            if (!(item->type == AST_CALL &&
                  item->text != 0 &&
                  (strcmp(item->text, "nw::time") == 0 ||
                   strcmp(item->text, "nw::color") == 0 ||
                   strcmp(item->text, "nw::clear") == 0)))
            {
                cg_emit_byte(cg, OP_OUT);
            }
        }

        item = item->next;
    }
}

static void cg_input(Codegen* cg, ASTNode* node)
{
    if (node->child == 0 ||
        node->child->type != AST_IDENTIFIER)
    {
        cg_error(cg, "nw::cin requires an identifier target");
        return;
    }

    cg_emit_byte(cg, OP_IN);
    cg_emit_byte(cg, OP_STORE);
    cg_emit_string(cg, node->child->text);
}

static void cg_line_input(Codegen* cg, ASTNode* node)
{
    ASTNode* target = 0;
    ASTNode* item = node->child;

    while (item != 0)
    {
        if (item->type == AST_IDENTIFIER)
            target = item;
        item = item->next;
    }

    if (target == 0)
    {
        cg_error(cg, "nw::line requires a target variable");
        return;
    }

    cg_emit_byte(cg, OP_LINE_IN);
    cg_emit_u32(cg, 1);
    cg_emit_string(cg, target->text);
}

static void cg_return(Codegen* cg, ASTNode* node)
{
    if (node->child != 0)
    {
        cg_expression(cg, node->child);
        cg_emit_byte(cg, OP_RETURN);
    }
    else
    {
        cg_emit_byte(cg, OP_RETURN_VOID);
    }
}

static void cg_if(Codegen* cg, ASTNode* node)
{
    unsigned int false_jump;
    unsigned int end_jump;

    cg_expression(cg, node->condition);

    false_jump = cg_emit_jump(cg, OP_JUMP_FALSE);

    if (node->then_branch != 0)
        cg_statement(cg, node->then_branch);

    if (node->else_branch != 0)
    {
        end_jump = cg_emit_jump(cg, OP_JUMP);

        cg_patch_u32(
            cg,
            false_jump,
            cg->size
        );

        cg_statement(cg, node->else_branch);

        cg_patch_u32(
            cg,
            end_jump,
            cg->size
        );
    }
    else
    {
        cg_patch_u32(
            cg,
            false_jump,
            cg->size
        );
    }
}

static void cg_while(Codegen* cg, ASTNode* node)
{
    unsigned int condition_target;
    unsigned int false_jump;
    unsigned int end_target;

    if (!cg_loop_push(cg))
        return;

    condition_target = cg->size;

    cg_expression(cg, node->while_condition);
    false_jump = cg_emit_jump(cg, OP_JUMP_FALSE);

    if (node->while_body != 0)
        cg_statement(cg, node->while_body);

    cg_emit_byte(cg, OP_JUMP);
    cg_emit_u32(cg, condition_target);

    end_target = cg->size;
    cg_patch_u32(cg, false_jump, end_target);
    cg_loop_pop(cg, condition_target, end_target);
}

static void cg_for(Codegen* cg, ASTNode* node)
{
    unsigned int condition_target;
    unsigned int false_jump = 0;
    unsigned int increment_target;
    unsigned int end_target;

    if (!cg_loop_push(cg))
        return;

    if (node->for_init != 0)
        cg_statement(cg, node->for_init);

    condition_target = cg->size;

    if (node->for_condition != 0)
    {
        cg_expression(cg, node->for_condition);
        false_jump = cg_emit_jump(cg, OP_JUMP_FALSE);
    }

    if (node->for_body != 0)
        cg_statement(cg, node->for_body);

    increment_target = cg->size;

    if (node->for_increment != 0)
        cg_statement(cg, node->for_increment);

    cg_emit_byte(cg, OP_JUMP);
    cg_emit_u32(cg, condition_target);

    end_target = cg->size;

    if (node->for_condition != 0)
        cg_patch_u32(cg, false_jump, end_target);

    cg_loop_pop(cg, increment_target, end_target);
}

static void cg_break(Codegen* cg)
{
    unsigned int patch;

    if (cg->loop_depth <= 0)
    {
        cg_error(cg, "break used outside a loop");
        return;
    }

    patch = cg_emit_jump(cg, OP_BREAK);

    {
        LoopContext* loop =
            &loop_stack[cg->loop_depth - 1];

        if (loop->break_patch_count >= NWC_MAX_PATCHES)
        {
            cg_error(cg, "too many break statements");
            return;
        }

        loop->break_patches[
            loop->break_patch_count++
        ] = patch;
    }
}

static void cg_continue(Codegen* cg)
{
    unsigned int patch;

    if (cg->loop_depth <= 0)
    {
        cg_error(cg, "continue used outside a loop");
        return;
    }

    patch = cg_emit_jump(cg, OP_CONTINUE);

    {
        LoopContext* loop =
            &loop_stack[cg->loop_depth - 1];

        if (loop->continue_patch_count >= NWC_MAX_PATCHES)
        {
            cg_error(cg, "too many continue statements");
            return;
        }

        loop->continue_patches[
            loop->continue_patch_count++
        ] = patch;
    }
}

static void cg_statement(Codegen* cg, ASTNode* node)
{
    int has_value;

    if (node == 0)
        return;

    switch (node->type)
    {
        case AST_VARIABLE_DECLARATION:
        case AST_ARRAY_DECLARATION:
            cg_variable_decl(cg, node);
            break;

        case AST_ASSIGNMENT:
            cg_assignment(cg, node);
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
            cg_call(cg, node, &has_value);

            if (has_value)
                cg_emit_byte(cg, OP_POP);
            break;

        case AST_IF:
            cg_if(cg, node);
            break;

        case AST_WHILE:
            cg_while(cg, node);
            break;

        case AST_FOR:
            cg_for(cg, node);
            break;

        case AST_BREAK:
            cg_break(cg);
            break;

        case AST_CONTINUE:
            cg_continue(cg);
            break;

        case AST_BLOCK:
            cg_node(cg, node->child);
            break;

        default:
            cg_expression(cg, node);
            if (!cg->error)
                cg_emit_byte(cg, OP_POP);
            break;
    }
}

static void cg_function(Codegen* cg, ASTNode* node)
{
    ASTNode* parameter;
    int parameter_count;

    if (node->text == 0)
    {
        cg_error(cg, "function has no name");
        return;
    }

    cg_emit_byte(cg, OP_FUNC_BEGIN);
    cg_emit_string(cg, node->text);
    cg_emit_byte(cg, cg_type_byte(node->value_type));

    parameter_count =
        cg_count_list(node->parameters);

    cg_emit_u32(cg, (unsigned int)parameter_count);

    parameter = node->parameters;

    while (parameter != 0)
    {
        cg_emit_string(cg, parameter->text);
        cg_emit_byte(cg, cg_type_byte(parameter->value_type));
        parameter = parameter->next;
    }

    if (node->child != 0 &&
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
    while (node != 0 && !cg->error)
    {
        if (node->type == AST_INCLUDE)
        {
            /* Headers are compile-time markers only. */
        }
        else if (node->type == AST_FUNCTION)
        {
            cg_function(cg, node);
        }
        else if (node->type == AST_BLOCK)
        {
            cg_node(cg, node->child);
        }
        else
        {
            cg_statement(cg, node);
        }

        node = node->next;
    }
}

static int cg_write_header(
    unsigned char* buffer,
    unsigned int capacity,
    unsigned int* position,
    unsigned int code_size,
    unsigned int data_size)
{
    unsigned char header[20];

    header[0] = 'N';
    header[1] = 'W';
    header[2] = 'O';
    header[3] = 'B';
    header[4] = NWO_VERSION;
    header[5] = 0;
    header[6] = 0;
    header[7] = 0;

    header[8]  = (unsigned char)(code_size & 0xFFu);
    header[9]  = (unsigned char)((code_size >> 8) & 0xFFu);
    header[10] = (unsigned char)((code_size >> 16) & 0xFFu);
    header[11] = (unsigned char)((code_size >> 24) & 0xFFu);

    header[12] = (unsigned char)(data_size & 0xFFu);
    header[13] = (unsigned char)((data_size >> 8) & 0xFFu);
    header[14] = (unsigned char)((data_size >> 16) & 0xFFu);
    header[15] = (unsigned char)((data_size >> 24) & 0xFFu);

    header[16] = 0;
    header[17] = 0;
    header[18] = 0;
    header[19] = 0;

    if (*position + sizeof(header) > capacity)
        return 0;

    memcpy(buffer + *position, header, sizeof(header));
    *position += sizeof(header);
    return 1;
}

void codegen_begin_output(
    unsigned char* buffer,
    unsigned int capacity,
    unsigned int* size_out)
{
    output_buffer = buffer;
    output_capacity = capacity;
    output_size = size_out;

    if (output_size != 0)
        *output_size = 0;
}

int codegen_write_nwo(ASTNode* ast, const char* path, int flags)
{
    Codegen cg;
    unsigned int position = 0;

    (void)path;
    (void)flags;

    cg.size = 0;
    cg.error = 0;
    cg.error_message = 0;
    cg.loop_depth = 0;

    if (ast == 0 ||
        ast->type != AST_PROGRAM)
    {
        printf("NwC codegen: expected program node\n");
        return 0;
    }

    if (output_buffer == 0 ||
        output_capacity < 20)
    {
        printf("NwC codegen: output buffer is not configured\n");
        return 0;
    }

    cg_node(&cg, ast->child);

    if (cg.error)
    {
        printf("NwC codegen: ");
        printf("%s\n", cg.error_message);
        return 0;
    }

    cg_emit_byte(&cg, OP_HALT);

    if (!cg_write_header(
            output_buffer,
            output_capacity,
            &position,
            cg.size,
            0))
    {
        printf("NwC codegen: output buffer overflow\n");
        return 0;
    }

    if (position + cg.size > output_capacity)
    {
        printf("NwC codegen: output buffer overflow\n");
        return 0;
    }

    memcpy(
        output_buffer + position,
        cg.code,
        cg.size
    );

    position += cg.size;

    if (output_size != 0)
        *output_size = position;

    nwc_output_begin(output_buffer, output_capacity, output_size);
    nwc_output_write(output_buffer, position);

    return 1;
}
