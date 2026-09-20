#include "../kernel.h"
#include "nwo_vm.h"
void print(const char* text);
void print_int(int number);
void putchar_os(char c);
void read_line(char* buffer, int max);
void clear(void);
void set_color(unsigned char color);
void delay(unsigned int count);
int keyboard_getkey(void);
int random_range(int min, int max);
static unsigned int nwo_strlen(const char *s);
static int nwo_strcmp(const char *a, const char *b);
static void nwo_memcpy(void *dst, const void *src, unsigned int size);
static void nwo_memset(void *dst, int value, unsigned int size);
static int nwo_memcmp(const void *a, const void *b, unsigned int size);

#define NWO_STACK_SIZE      128
#define NWO_VARIABLES       32
#define NWO_NAME_SIZE       32
#define NWO_INPUT_SIZE      256
#define NWO_MAX_CALL_DEPTH  16
#define NWO_MAX_PARAMS      32

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

typedef enum
{
    VM_NONE = 0,
    VM_INT,
    VM_FLOAT,
    VM_BOOL,
    VM_CHAR,
    VM_STRING

} VmValueType;

typedef struct
{
    VmValueType type;

    unsigned int string_length;

    union
    {
        int integer;
        double floating;
        unsigned int boolean;
        unsigned int character;
        const char* string;
    } as;

} VmValue;

typedef struct
{
    int used;
    char name[NWO_NAME_SIZE];
    unsigned char declared_type;
    VmValue value;

} VmVariable;

typedef struct
{
    unsigned int body_ip;
    unsigned char return_type;
    unsigned int param_count;
    char param_names[NWO_MAX_PARAMS][NWO_NAME_SIZE];
    unsigned char param_types[NWO_MAX_PARAMS];

} VmFunctionInfo;

typedef struct
{
    unsigned int return_ip;
    int saved_sp;
    unsigned char return_type;
    VmVariable saved_variables[NWO_VARIABLES];

} VmCallFrame;

typedef struct
{
    NwoProgram* program;
    unsigned int ip;

    VmValue stack[NWO_STACK_SIZE];
    int sp;

    VmVariable variables[NWO_VARIABLES];

    int call_sp;
    int running;
    int result;

} NwoVM;

static unsigned int nwo_strlen(const char *s)
{
    unsigned int n = 0;

    if (!s)
        return 0;

    while (s[n])
        n++;

    return n;
}

static int nwo_strcmp(const char *a, const char *b)
{
    unsigned int i = 0;

    if (!a) a = "";
    if (!b) b = "";

    while (a[i] && b[i] && a[i] == b[i])
        i++;

    return (unsigned char)a[i] - (unsigned char)b[i];
}

static void nwo_memcpy(void *dst, const void *src, unsigned int size)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (unsigned int i = 0; i < size; i++)
        d[i] = s[i];
}

static void nwo_memset(void *dst, int value, unsigned int size)
{
    unsigned char *d = (unsigned char *)dst;

    for (unsigned int i = 0; i < size; i++)
        d[i] = (unsigned char)value;
}

static int nwo_memcmp(const void *a, const void *b, unsigned int size)
{
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;

    for (unsigned int i = 0; i < size; i++)
    {
        if (x[i] != y[i])
            return (int)x[i] - (int)y[i];
    }

    return 0;
}

static VmCallFrame vm_frames[NWO_MAX_CALL_DEPTH];

static char vm_string_storage[NWO_MAX_CALL_DEPTH + 1]
                            [NWO_VARIABLES]
                            [NWO_INPUT_SIZE];

static int vm_push(NwoVM* vm, VmValue value)
{
    if (vm->sp >= NWO_STACK_SIZE)
    {
        print("[NWO VM] Stack overflow.\n");
        return 0;
    }

    vm->stack[vm->sp++] = value;
    return 1;
}

static int vm_pop(NwoVM* vm, VmValue* value)
{
    if (vm->sp <= 0)
    {
        print("[NWO VM] Stack underflow.\n");
        return 0;
    }

    vm->sp--;

    if (value != 0)
        *value = vm->stack[vm->sp];

    return 1;
}

static int vm_read_u8(NwoVM* vm, unsigned char* value)
{
    if (vm->ip >= vm->program->code_size)
    {
        print("[NWO VM] Unexpected end of code.\n");
        return 0;
    }

    if (value != 0)
        *value = vm->program->code[vm->ip];

    vm->ip++;
    return 1;
}

static int vm_read_u32(NwoVM* vm, unsigned int* value)
{
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;

    if (!vm_read_u8(vm, &b0) ||
        !vm_read_u8(vm, &b1) ||
        !vm_read_u8(vm, &b2) ||
        !vm_read_u8(vm, &b3))
        return 0;

    if (value != 0)
    {
        *value = ((unsigned int)b0) |
                 ((unsigned int)b1 << 8) |
                 ((unsigned int)b2 << 16) |
                 ((unsigned int)b3 << 24);
    }

    return 1;
}

static int vm_read_u64(NwoVM* vm, unsigned long long* value)
{
    unsigned int low;
    unsigned int high;

    if (!vm_read_u32(vm, &low) ||
        !vm_read_u32(vm, &high))
        return 0;

    if (value != 0)
    {
        *value = ((unsigned long long)low) |
                  ((unsigned long long)high << 32);
    }

    return 1;
}

static int vm_read_string(NwoVM* vm,
                          const char** value,
                          unsigned int* length)
{
    unsigned int len;
    unsigned int start;

    if (!vm_read_u32(vm, &len))
        return 0;

    start = vm->ip;

    if (start > vm->program->code_size ||
        len > vm->program->code_size - start)
    {
        print("[NWO VM] Invalid string length.\n");
        return 0;
    }

    if (value != 0)
        *value = (const char*)(vm->program->code + start);

    if (length != 0)
        *length = len;

    vm->ip += len;
    return 1;
}

static unsigned int code_read_u32(const unsigned char* code,
                                  unsigned int code_size,
                                  unsigned int* ip,
                                  int* ok)
{
    unsigned int p;
    unsigned int value;

    if (code == 0 || ip == 0 || ok == 0)
        return 0;

    p = *ip;

    if (p > code_size || code_size - p < 4)
    {
        *ok = 0;
        return 0;
    }

    value = ((unsigned int)code[p]) |
            ((unsigned int)code[p + 1] << 8) |
            ((unsigned int)code[p + 2] << 16) |
            ((unsigned int)code[p + 3] << 24);

    *ip = p + 4;
    return value;
}

static int code_skip_bytes(const unsigned char* code,
                           unsigned int code_size,
                           unsigned int* ip,
                           unsigned int count)
{
    if (*ip > code_size ||
        count > code_size - *ip)
        return 0;

    *ip += count;
    return 1;
}
static int code_skip_instruction(const unsigned char* code,
                                 unsigned int code_size,
                                 unsigned int* ip,
                                 unsigned char opcode)
{
    unsigned int len;
    unsigned int count;
    unsigned int i;
    int ok = 1;

    switch (opcode)
    {
        case OP_NOP:
        case OP_POP:
        case OP_DUP:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_EQ:
        case OP_NEQ:
        case OP_LT:
        case OP_GT:
        case OP_LTE:
        case OP_GTE:
        case OP_AND:
        case OP_OR:
        case OP_NOT:
        case OP_OUT:
        case OP_OUT_ENDL:
        case OP_IN:
        case OP_TIME:
        case OP_COLOR:
        case OP_GETKEY:
        case OP_CLEAR:
        case OP_RETURN:
        case OP_RETURN_VOID:
        case OP_FUNC_END:
        case OP_HALT:
            return 1;

        case OP_PUSH_INT:
        case OP_PUSH_FLOAT:
            return code_skip_bytes(code, code_size, ip, 8);

        case OP_PUSH_STRING:
        {
            len = code_read_u32(code, code_size, ip, &ok);
            if (!ok)
                return 0;
            return code_skip_bytes(code, code_size, ip, len);
        }

        case OP_PUSH_CHAR:
            return code_skip_bytes(code, code_size, ip, 4);

        case OP_PUSH_BOOL:
            return code_skip_bytes(code, code_size, ip, 1);

        case OP_LOAD:
        case OP_STORE:
        case OP_CALL:
        {
            len = code_read_u32(code, code_size, ip, &ok);
            if (!ok || !code_skip_bytes(code, code_size, ip, len))
                return 0;

            if (opcode == OP_CALL)
            {
                (void)code_read_u32(code, code_size, ip, &ok);
                if (!ok)
                    return 0;
            }

            return 1;
        }

        case OP_DECLARE:
        {
            len = code_read_u32(code, code_size, ip, &ok);
            if (!ok || !code_skip_bytes(code, code_size, ip, len))
                return 0;

            return code_skip_bytes(code, code_size, ip, 1);
        }

        case OP_DECLARE_ARR:
        {
            len = code_read_u32(code, code_size, ip, &ok);
            if (!ok || !code_skip_bytes(code, code_size, ip, len))
                return 0;

            if (!code_skip_bytes(code, code_size, ip, 1))
                return 0;

            return code_skip_bytes(code, code_size, ip, 4);
        }

        case OP_LINE_IN:
        {
            count = code_read_u32(code, code_size, ip, &ok);
            if (!ok || count > NWO_VARIABLES)
                return 0;

            for (i = 0; i < count; i++)
            {
                len = code_read_u32(code, code_size, ip, &ok);
                if (!ok || len >= NWO_NAME_SIZE ||
                    !code_skip_bytes(code, code_size, ip, len))
                    return 0;
            }

            return 1;
        }

        case OP_RANDOM:
            return 1;

        case OP_FUNC_BEGIN:
        {
            count = 0;

            len = code_read_u32(code, code_size, ip, &ok);
            if (!ok || !code_skip_bytes(code, code_size, ip, len))
                return 0;

            if (!code_skip_bytes(code, code_size, ip, 1))
                return 0;

            count = code_read_u32(code, code_size, ip, &ok);
            if (!ok || count > NWO_MAX_PARAMS)
                return 0;

            for (i = 0; i < count; i++)
            {
                len = code_read_u32(code, code_size, ip, &ok);
                if (!ok || len >= NWO_NAME_SIZE ||
                    !code_skip_bytes(code, code_size, ip, len))
                    return 0;

                if (!code_skip_bytes(code, code_size, ip, 1))
                    return 0;
            }

            return 1;
        }

        case OP_JUMP:
        case OP_JUMP_FALSE:
        case OP_BREAK:
        case OP_CONTINUE:
            return code_skip_bytes(code, code_size, ip, 4);

        default:
            return 0;
    }
}

static int vm_get_string_name(NwoVM* vm,
                              char* destination,
                              unsigned int* length)
{
    const char* name;
    unsigned int len;
    unsigned int i;

    if (!vm_read_string(vm, &name, &len))
        return 0;

    if (len >= NWO_NAME_SIZE)
    {
        print("[NWO VM] Name too long.\n");
        return 0;
    }

    for (i = 0; i < len; i++)
        destination[i] = name[i];

    destination[len] = '\0';

    if (length != 0)
        *length = len;

    return 1;
}

static void vm_clear_variables(NwoVM* vm)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        vm->variables[i].used = 0;
        vm->variables[i].name[0] = '\0';
        vm->variables[i].declared_type = 0;
        vm->variables[i].value.type = VM_NONE;
        vm->variables[i].value.string_length = 0;
        vm->variables[i].value.as.integer = 0;
    }
}

static int vm_name_equals(const char* a, const char* b)
{
    int i = 0;

    if (a == 0 || b == 0)
        return 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;
        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static int vm_find_variable(NwoVM* vm, const char* name)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        if (vm->variables[i].used &&
            vm_name_equals(vm->variables[i].name, name))
            return i;
    }

    return -1;
}

static int vm_declare_variable(NwoVM* vm,
                               const char* name,
                               unsigned char type)
{
    int i;
    int slot = -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    i = vm_find_variable(vm, name);
    if (i != -1)
        return i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        if (!vm->variables[i].used)
        {
            slot = i;
            break;
        }
    }

    if (slot == -1)
    {
        print("[NWO VM] Too many variables.\n");
        return -1;
    }

    vm->variables[slot].used = 1;
    vm->variables[slot].declared_type = type;

    i = 0;
    while (name[i] != '\0' && i < NWO_NAME_SIZE - 1)
    {
        vm->variables[slot].name[i] = name[i];
        i++;
    }
    vm->variables[slot].name[i] = '\0';

    vm->variables[slot].value.type = VM_NONE;
    vm->variables[slot].value.string_length = 0;
    vm->variables[slot].value.as.integer = 0;

    return slot;
}

static int vm_to_int(const VmValue* value)
{
    if (value == 0)
        return 0;

    switch (value->type)
    {
        case VM_INT:   return value->as.integer;
        case VM_BOOL:  return value->as.boolean ? 1 : 0;
        case VM_CHAR:  return (int)value->as.character;
        case VM_FLOAT: return (int)value->as.floating;
        default:       return 0;
    }
}

static int vm_string_equals(const VmValue* a, const VmValue* b)
{
    unsigned int i;

    if (a == 0 || b == 0 ||
        a->type != VM_STRING || b->type != VM_STRING)
        return 0;

    if (a->string_length != b->string_length)
        return 0;

    if (a->as.string == 0 || b->as.string == 0)
        return a->as.string == b->as.string;

    for (i = 0; i < a->string_length; i++)
    {
        if (a->as.string[i] != b->as.string[i])
            return 0;
    }

    return 1;
}

static int vm_values_equal(const VmValue* a, const VmValue* b)
{
    if (a == 0 || b == 0)
        return 0;

    if (a->type == VM_STRING || b->type == VM_STRING)
    {
        if (a->type != VM_STRING || b->type != VM_STRING)
            return 0;
        return vm_string_equals(a, b);
    }

    return vm_to_int(a) == vm_to_int(b);
}

static int vm_binary_arithmetic(NwoVM* vm, unsigned char opcode)
{
    VmValue left;
    VmValue right;
    VmValue result;
    int a;
    int b;

    if (!vm_pop(vm, &right) || !vm_pop(vm, &left))
        return 0;

    a = vm_to_int(&left);
    b = vm_to_int(&right);

    result.type = VM_INT;
    result.string_length = 0;
    result.as.integer = 0;

    switch (opcode)
    {
        case OP_ADD:
            result.as.integer = a + b;
            break;

        case OP_SUB:
            result.as.integer = a - b;
            break;

        case OP_MUL:
            result.as.integer = a * b;
            break;

        case OP_DIV:
            if (b == 0)
            {
                print("[NWO VM] Division by zero.\n");
                return 0;
            }
            result.as.integer = a / b;
            break;

        case OP_MOD:
            if (b == 0)
            {
                print("[NWO VM] Modulo by zero.\n");
                return 0;
            }
            result.as.integer = a % b;
            break;

        default:
            print("[NWO VM] Invalid arithmetic opcode.\n");
            return 0;
    }

    return vm_push(vm, result);
}

static int vm_compare(NwoVM* vm, unsigned char opcode)
{
    VmValue left;
    VmValue right;
    VmValue result;
    int a;
    int b;
    int r = 0;

    if (!vm_pop(vm, &right) || !vm_pop(vm, &left))
        return 0;

    switch (opcode)
    {
        case OP_EQ:
            r = vm_values_equal(&left, &right);
            break;

        case OP_NEQ:
            r = !vm_values_equal(&left, &right);
            break;

        default:
            if (left.type == VM_STRING || right.type == VM_STRING)
            {
                print("[NWO VM] Only == and != are supported for strings.\n");
                return 0;
            }

            a = vm_to_int(&left);
            b = vm_to_int(&right);

            switch (opcode)
            {
                case OP_LT:  r = a < b;  break;
                case OP_GT:  r = a > b;  break;
                case OP_LTE: r = a <= b; break;
                case OP_GTE: r = a >= b; break;
                default:
                    print("[NWO VM] Invalid comparison opcode.\n");
                    return 0;
            }
            break;
    }

    result.type = VM_BOOL;
    result.string_length = 0;
    result.as.boolean = r ? 1 : 0;

    return vm_push(vm, result);
}

static int vm_find_function(NwoProgram* program,
                            const char* wanted_name,
                            VmFunctionInfo* info)
{
    unsigned int ip = 0;
    unsigned int len;
    unsigned int count;
    unsigned int i;
    unsigned int name_start;
    unsigned int name_length;
    int ok;

    if (program == 0 || program->code == 0 ||
        wanted_name == 0 || info == 0)
        return 0;

    nwo_memset(info, 0, sizeof(*info));

    while (ip < program->code_size)
    {
        unsigned int instruction_start = ip;
        unsigned char opcode = program->code[ip++];

        if (opcode == OP_FUNC_BEGIN)
        {
            ok = 1;

            len = code_read_u32(program->code,
                                program->code_size,
                                &ip,
                                &ok);
            if (!ok || len >= 128 ||
                !code_skip_bytes(program->code,
                                 program->code_size,
                                 &ip,
                                 len))
                return 0;

            name_start = ip - len;
            name_length = len;

            if (ip >= program->code_size)
                return 0;

            info->return_type = program->code[ip++];

            count = code_read_u32(program->code,
                                  program->code_size,
                                  &ip,
                                  &ok);
            if (!ok || count > NWO_MAX_PARAMS)
                return 0;

            info->param_count = count;

            for (i = 0; i < count; i++)
            {
                len = code_read_u32(program->code,
                                    program->code_size,
                                    &ip,
                                    &ok);
                if (!ok || len >= NWO_NAME_SIZE ||
                    !code_skip_bytes(program->code,
                                     program->code_size,
                                     &ip,
                                     len))
                    return 0;

                {
                    unsigned int param_start = ip - len;
                    unsigned int j;

                    for (j = 0; j < len; j++)
                        info->param_names[i][j] =
                            (char)program->code[param_start + j];
                    info->param_names[i][len] = '\0';
                }

                if (ip >= program->code_size)
                    return 0;

                info->param_types[i] = program->code[ip++];
            }

            if (name_length == nwo_strlen(wanted_name) &&
                nwo_memcmp(program->code + name_start,
                           wanted_name,
                           name_length) == 0)
            {
                info->body_ip = ip;
                return 1;
            }

            (void)instruction_start;
            continue;
        }

        if (!code_skip_instruction(program->code,
                                   program->code_size,
                                   &ip,
                                   opcode))
            return 0;
    }

    return 0;
}

static int vm_find_main(NwoProgram* program, unsigned int* main_ip)
{
    VmFunctionInfo info;

    if (!vm_find_function(program, "main", &info))
        return 0;

    if (main_ip != 0)
        *main_ip = info.body_ip;

    return 1;
}

static VmValue vm_default_value(unsigned char type)
{
    VmValue value;

    value.type = VM_INT;
    value.string_length = 0;
    value.as.integer = 0;

    switch (type)
    {
        case 3:
            value.type = VM_BOOL;
            value.as.boolean = 0;
            break;

        case 6:
            value.type = VM_CHAR;
            value.as.character = 0;
            break;

        case 7:
            value.type = VM_STRING;
            value.as.string = "";
            value.string_length = 0;
            break;

        case 2:
            value.type = VM_FLOAT;
            value.as.floating = 0.0;
            break;

        default:
            value.type = VM_INT;
            value.as.integer = 0;
            break;
    }

    return value;
}

static int vm_return_value(NwoVM* vm, VmValue value, int has_value)
{
    VmCallFrame* frame;

    if (vm->call_sp <= 0)
    {
        vm->result = has_value ? vm_to_int(&value) : 0;
        vm->running = 0;
        return 1;
    }

    frame = &vm_frames[vm->call_sp - 1];

    nwo_memcpy(vm->variables,
              frame->saved_variables,
              sizeof(vm->variables));

    vm->sp = frame->saved_sp;
    vm->ip = frame->return_ip;
    vm->call_sp--;

    if (!has_value)
    {
        VmValue none;
        none.type = VM_NONE;
        none.string_length = 0;
        none.as.integer = 0;
        return vm_push(vm, none);
    }

    return vm_push(vm, value);
}

static int vm_call(NwoVM* vm, const char* name, unsigned int argc)
{
    VmFunctionInfo info;
    VmValue arguments[NWO_MAX_PARAMS];
    VmCallFrame* frame;
    int saved_sp;
    unsigned int i;
    int arg_index;
    int variable_index;
    unsigned int depth;

    if (!vm_find_function(vm->program, name, &info))
    {
        print("[NWO VM] Function not found: ");
        print(name);
        putchar_os('\n');
        return 0;
    }

    if (argc != info.param_count)
    {
        print("[NWO VM] Wrong argument count for function: ");
        print(name);
        putchar_os('\n');
        return 0;
    }

    if (vm->call_sp >= NWO_MAX_CALL_DEPTH)
    {
        print("[NWO VM] Call stack overflow.\n");
        return 0;
    }

    if (vm->sp < (int)argc)
    {
        print("[NWO VM] Not enough call arguments on stack.\n");
        return 0;
    }

    saved_sp = vm->sp - (int)argc;

    for (i = 0; i < argc; i++)
    {
        arg_index = (int)argc - 1 - (int)i;
        arguments[arg_index] = vm->stack[vm->sp - 1 - (int)i];
    }

    frame = &vm_frames[vm->call_sp];
    frame->return_ip = vm->ip;
    frame->saved_sp = saved_sp;
    frame->return_type = info.return_type;

    nwo_memcpy(frame->saved_variables,
              vm->variables,
              sizeof(vm->variables));

    depth = (unsigned int)vm->call_sp + 1;

    vm->sp = saved_sp;
    vm->call_sp++;
    vm_clear_variables(vm);

    for (i = 0; i < argc; i++)
    {
        variable_index = vm_declare_variable(
            vm,
            info.param_names[i],
            info.param_types[i]
        );

        if (variable_index < 0)
            return 0;

        vm->variables[variable_index].value = arguments[i];
    }

    (void)depth;
    vm->ip = info.body_ip;
    return 1;
}

static void vm_print_string(const char* string, unsigned int length)
{
    unsigned int i;

    if (string == 0)
        return;

    for (i = 0; i < length; i++)
        putchar_os(string[i]);
}

static void vm_output(const VmValue* value)
{
    if (value == 0)
        return;

    switch (value->type)
    {
        case VM_INT:
            print_int(value->as.integer);
            break;

        case VM_FLOAT:
            print_int((int)value->as.floating);
            break;

        case VM_BOOL:
            if (value->as.boolean)
                print("true");
            else
                print("false");
            break;

        case VM_CHAR:
            putchar_os((char)value->as.character);
            break;

        case VM_STRING:
            vm_print_string(value->as.string, value->string_length);
            break;

        default:
            break;
    }
}

static int vm_store_string_input(NwoVM* vm,
                                 const char* variable_name,
                                 unsigned int depth)
{
    char buffer[NWO_INPUT_SIZE];
    int index;
    unsigned int i;
    unsigned int length;

    if (depth > NWO_MAX_CALL_DEPTH)
        depth = NWO_MAX_CALL_DEPTH;

    read_line(buffer, NWO_INPUT_SIZE);

    length = 0;
    while (buffer[length] != '\0' && length < NWO_INPUT_SIZE - 1)
        length++;

    for (i = 0; i < length; i++)
        vm_string_storage[depth][0][i] = buffer[i];

    vm_string_storage[depth][0][length] = '\0';

    index = vm_find_variable(vm, variable_name);
    if (index < 0)
    {
        index = vm_declare_variable(vm, variable_name, 7);
        if (index < 0)
            return 0;
    }
    for (i = 0; i < length; i++)
        vm_string_storage[depth][index][i] =
            vm_string_storage[depth][0][i];

    vm_string_storage[depth][index][length] = '\0';

    vm->variables[index].value.type = VM_STRING;
    vm->variables[index].value.as.string =
        vm_string_storage[depth][index];
    vm->variables[index].value.string_length = length;

    return 1;
}

int nwo_execute(NwoProgram* program)
{
    NwoVM vm;
    unsigned int main_ip;

    if (program == 0 ||
        program->code == 0 ||
        program->code_size == 0)
    {
        print("[NWO VM] Empty program.\n");
        return -1;
    }

    if (!vm_find_main(program, &main_ip))
    {
        print("[NWO VM] main() not found or NWO metadata is invalid.\n");
        return -1;
    }

    nwo_memset(&vm, 0, sizeof(vm));
    nwo_memset(vm_frames, 0, sizeof(vm_frames));
    nwo_memset(vm_string_storage, 0, sizeof(vm_string_storage));

    vm.program = program;
    vm.ip = main_ip;
    vm.running = 1;
    vm.result = 0;
    vm.sp = 0;
    vm.call_sp = 0;

    vm_clear_variables(&vm);

    while (vm.running)
    {
        unsigned char opcode;

        if (!vm_read_u8(&vm, &opcode))
            return -1;

        switch (opcode)
        {
            case OP_NOP:
                break;

            case OP_PUSH_INT:
            {
                unsigned long long raw;
                VmValue value;

                if (!vm_read_u64(&vm, &raw))
                    return -1;

                value.type = VM_INT;
                value.string_length = 0;
                value.as.integer = (int)raw;

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_PUSH_FLOAT:
            {
                unsigned long long bits;
                double value;
                VmValue vm_value;
                unsigned char* dst;
                unsigned char* src;
                int i;

                if (!vm_read_u64(&vm, &bits))
                    return -1;

                dst = (unsigned char*)&value;
                src = (unsigned char*)&bits;

                for (i = 0; i < 8; i++)
                    dst[i] = src[i];

                vm_value.type = VM_FLOAT;
                vm_value.string_length = 0;
                vm_value.as.floating = value;

                if (!vm_push(&vm, vm_value))
                    return -1;
                break;
            }

            case OP_PUSH_STRING:
            {
                const char* string;
                unsigned int length;
                VmValue value;

                if (!vm_read_string(&vm, &string, &length))
                    return -1;

                value.type = VM_STRING;
                value.as.string = string;
                value.string_length = length;

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_PUSH_CHAR:
            {
                unsigned int character;
                VmValue value;

                if (!vm_read_u32(&vm, &character))
                    return -1;

                value.type = VM_CHAR;
                value.string_length = 0;
                value.as.character = character;

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_PUSH_BOOL:
            {
                unsigned char boolean;
                VmValue value;

                if (!vm_read_u8(&vm, &boolean))
                    return -1;

                value.type = VM_BOOL;
                value.string_length = 0;
                value.as.boolean = boolean ? 1 : 0;

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_POP:
            {
                VmValue unused;
                if (!vm_pop(&vm, &unused))
                    return -1;
                break;
            }

            case OP_DUP:
                if (vm.sp <= 0)
                {
                    print("[NWO VM] Stack underflow on DUP.\n");
                    return -1;
                }
                if (!vm_push(&vm, vm.stack[vm.sp - 1]))
                    return -1;
                break;

            case OP_DECLARE:
            {
                char name[NWO_NAME_SIZE];
                unsigned int len;
                unsigned char type;

                if (!vm_get_string_name(&vm, name, &len))
                    return -1;
                (void)len;

                if (!vm_read_u8(&vm, &type))
                    return -1;

                if (vm_declare_variable(&vm, name, type) < 0)
                    return -1;
                break;
            }

            case OP_DECLARE_ARR:
            {
                char name[NWO_NAME_SIZE];
                unsigned int len;
                unsigned char type;
                unsigned int size;

                if (!vm_get_string_name(&vm, name, &len))
                    return -1;
                (void)len;

                if (!vm_read_u8(&vm, &type) ||
                    !vm_read_u32(&vm, &size))
                    return -1;
                (void)size;

                if (vm_declare_variable(&vm, name, type) < 0)
                    return -1;
                break;
            }

            case OP_LOAD:
            {
                char name[NWO_NAME_SIZE];
                unsigned int len;
                int index;
                VmValue value;

                if (!vm_get_string_name(&vm, name, &len))
                    return -1;
                (void)len;

                index = vm_find_variable(&vm, name);
                if (index < 0)
                {
                    print("[NWO VM] Unknown variable: ");
                    print(name);
                    putchar_os('\n');
                    return -1;
                }

                value = vm.variables[index].value;

                if (value.type == VM_NONE)
                    value = vm_default_value(vm.variables[index].declared_type);

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_STORE:
            {
                char name[NWO_NAME_SIZE];
                unsigned int len;
                int index;
                VmValue value;

                if (!vm_pop(&vm, &value))
                    return -1;

                if (!vm_get_string_name(&vm, name, &len))
                    return -1;
                (void)len;

                index = vm_find_variable(&vm, name);
                if (index < 0)
                {
                    index = vm_declare_variable(&vm, name, 1);
                    if (index < 0)
                        return -1;
                }

                vm.variables[index].value = value;
                break;
            }

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_MOD:
                if (!vm_binary_arithmetic(&vm, opcode))
                    return -1;
                break;

            case OP_EQ:
            case OP_NEQ:
            case OP_LT:
            case OP_GT:
            case OP_LTE:
            case OP_GTE:
                if (!vm_compare(&vm, opcode))
                    return -1;
                break;

            case OP_AND:
            case OP_OR:
            {
                VmValue left;
                VmValue right;
                VmValue result;
                int a;
                int b;

                if (!vm_pop(&vm, &right) ||
                    !vm_pop(&vm, &left))
                    return -1;

                a = vm_to_int(&left) != 0;
                b = vm_to_int(&right) != 0;

                result.type = VM_BOOL;
                result.string_length = 0;
                result.as.boolean =
                    opcode == OP_AND ? (a && b) : (a || b);

                if (!vm_push(&vm, result))
                    return -1;
                break;
            }

            case OP_NOT:
            {
                VmValue value;
                VmValue result;

                if (!vm_pop(&vm, &value))
                    return -1;

                result.type = VM_BOOL;
                result.string_length = 0;
                result.as.boolean = vm_to_int(&value) ? 0 : 1;

                if (!vm_push(&vm, result))
                    return -1;
                break;
            }

            case OP_OUT:
            {
                VmValue value;
                if (!vm_pop(&vm, &value))
                    return -1;
                vm_output(&value);
                break;
            }

            case OP_OUT_ENDL:
                putchar_os('\n');
                break;

            case OP_IN:
            {
                char buffer[NWO_INPUT_SIZE];
                int sign = 1;
                int i = 0;
                int result = 0;
                VmValue value;

                print("> ");
                read_line(buffer, NWO_INPUT_SIZE);

                if (buffer[0] == '-')
                {
                    sign = -1;
                    i = 1;
                }

                while (buffer[i] >= '0' && buffer[i] <= '9')
                {
                    result = result * 10 + (buffer[i] - '0');
                    i++;
                }

                value.type = VM_INT;
                value.string_length = 0;
                value.as.integer = result * sign;

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_LINE_IN:
            {
                unsigned int count;
                unsigned int i;
                char name[NWO_NAME_SIZE];
                unsigned int len;

                if (!vm_read_u32(&vm, &count) ||
                    count > NWO_VARIABLES)
                    return -1;

                for (i = 0; i < count; i++)
                {
                    if (!vm_get_string_name(&vm, name, &len))
                        return -1;
                    (void)len;

                    if (!vm_store_string_input(
                            &vm,
                            name,
                            (unsigned int)vm.call_sp))
                        return -1;
                }
                break;
            }

            case OP_TIME:
            {
                VmValue value;
                int milliseconds;

                if (!vm_pop(&vm, &value))
                    return -1;

                milliseconds = vm_to_int(&value);
                if (milliseconds < 0)
                    milliseconds = 0;

                delay((unsigned int)milliseconds);
                break;
            }

            case OP_COLOR:
            {
                VmValue value;

                if (!vm_pop(&vm, &value))
                    return -1;

                set_color((unsigned char)vm_to_int(&value));
                break;
            }

            case OP_GETKEY:
            {
                VmValue value;

                value.type = VM_INT;
                value.string_length = 0;
                value.as.integer = keyboard_getkey();

                if (!vm_push(&vm, value))
                    return -1;
                break;
            }

            case OP_CLEAR:
                clear();
                break;

            case OP_RANDOM:
            {
                VmValue min_value;
                VmValue max_value;
                VmValue result;
                int min;
                int max;

                if (!vm_pop(&vm, &max_value) ||
                    !vm_pop(&vm, &min_value))
                    return -1;

                min = vm_to_int(&min_value);
                max = vm_to_int(&max_value);

                if (max < min)
                {
                    int temp = min;
                    min = max;
                    max = temp;
                }

                result.type = VM_INT;
                result.string_length = 0;
                result.as.integer = random_range(min, max);

                if (!vm_push(&vm, result))
                    return -1;
                break;
            }

            case OP_JUMP:
            case OP_BREAK:
            case OP_CONTINUE:
            {
                unsigned int target;

                if (!vm_read_u32(&vm, &target))
                    return -1;

                if (target >= program->code_size)
                {
                    print("[NWO VM] Invalid jump target.\n");
                    return -1;
                }

                vm.ip = target;
                break;
            }

            case OP_JUMP_FALSE:
            {
                VmValue condition;
                unsigned int target;

                if (!vm_pop(&vm, &condition) ||
                    !vm_read_u32(&vm, &target))
                    return -1;

                if (target >= program->code_size)
                {
                    print("[NWO VM] Invalid conditional jump target.\n");
                    return -1;
                }

                if (vm_to_int(&condition) == 0)
                    vm.ip = target;

                break;
            }

            case OP_CALL:
            {
                char name[NWO_NAME_SIZE];
                unsigned int len;
                unsigned int argc;

                if (!vm_get_string_name(&vm, name, &len) ||
                    !vm_read_u32(&vm, &argc))
                    return -1;

                (void)len;

                if (!vm_call(&vm, name, argc))
                    return -1;
                break;
            }

            case OP_RETURN:
            {
                VmValue value;

                if (!vm_pop(&vm, &value))
                    return -1;

                if (!vm_return_value(&vm, value, 1))
                    return -1;
                break;
            }

            case OP_RETURN_VOID:
                if (!vm_return_value(&vm,
                                     vm_default_value(0),
                                     0))
                    return -1;
                break;

            case OP_FUNC_END:
            {
                VmValue value;
                int has_value = 0;

                if (vm.call_sp > 0)
                {
                    VmCallFrame* frame = &vm_frames[vm.call_sp - 1];

                    if (frame->return_type != 0)
                    {
                        value = vm_default_value(frame->return_type);
                        has_value = 1;
                    }
                }
                else
                {
                    value = vm_default_value(0);
                }

                if (!vm_return_value(&vm, value, has_value))
                    return -1;
                break;
            }

            case OP_HALT:
                vm.running = 0;
                vm.result = 0;
                break;

            default:
                print("[NWO VM] Unknown opcode: ");
                print_int((int)opcode);
                putchar_os('\n');
                return -1;
        }
    }

    return vm.result;
}