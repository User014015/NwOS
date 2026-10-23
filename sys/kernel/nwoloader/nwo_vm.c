#include "nwo_vm.h"

void print(const char* text);
void print_int(int number);
void putchar_os(char c);

void read_line(char* buffer, int max);

void clear(void);
void set_color(unsigned char color);

int keyboard_getkey(void);
int mouse_x(void);
int mouse_y(void);
int mouse_buttons(void);
void poweroff(void);
void reboot(void);
void get_time(int* hour, int* minute, int* second);
void fb_draw_rect(int x, int y, int w, int h, unsigned int color);
void fb_draw_frame(int x, int y, int w, int h, unsigned int color);
void fb_draw_cube_default(unsigned int color);
void fb_draw_window(int x, int y, int w, int h, unsigned int color, const char* title);
void fb_draw_text(int x, int y, const char* text, unsigned int color);
void fb_draw_wallpaper(void);
void graphics_begin(void);
void sleep(unsigned int ms);

#define NWO_STACK_SIZE      128
#define NWO_VARIABLES       32
#define NWO_NAME_SIZE       32
#define NWO_INPUT_SIZE      256

#define NWO_MAX_CALL_DEPTH  16
#define NWO_MAX_PARAMS      16

static char vm_string_storage[NWO_VARIABLES][NWO_INPUT_SIZE];

#define OP_NOP          0x00

#define OP_PUSH_INT     0x10
#define OP_PUSH_FLOAT   0x11
#define OP_PUSH_STRING  0x12
#define OP_PUSH_CHAR    0x13
#define OP_PUSH_BOOL    0x14
#define OP_POP          0x15

#define OP_LOAD         0x20
#define OP_STORE        0x21
#define OP_DECLARE      0x22
#define OP_DECLARE_ARR  0x23

#define OP_ADD          0x30
#define OP_SUB          0x31
#define OP_MUL          0x32
#define OP_DIV          0x33

#define OP_EQ           0x40
#define OP_NEQ          0x41
#define OP_LT           0x42
#define OP_GT           0x43
#define OP_LTE          0x44
#define OP_GTE          0x45

#define OP_STRCMP       0x49
#define OP_STRNCMP      0x4A

#define OP_OUT          0x50
#define OP_OUT_ENDL     0x51
#define OP_IN           0x52
#define OP_LINE_IN      0x53
#define OP_IN_STORE     0x54

#define OP_TIME         0x60
#define OP_COLOR        0x61
#define OP_GETKEY       0x62
#define OP_CLEAR        0x63
#define OP_RECT         0x64
#define OP_COLOR_PUSH   0x65
#define OP_COLOR_POP    0x66
#define OP_SLEEP        0x67
#define OP_GETKEY_FILTER 0x68
#define OP_ASM_HLT      0x69
#define OP_DRAW_FRAME   0x6A
#define OP_DRAW_CUBE    0x6B
#define OP_DRAW_WINDOW  0x6C
#define OP_DRAW_TEXT    0x6D
#define OP_MOUSE_X      0x6E
#define OP_MOUSE_Y      0x6F

#define OP_FUNC_BEGIN   0x70
#define OP_FUNC_END     0x71
#define OP_CALL         0x72
#define OP_RETURN       0x73
#define OP_RETURN_VOID  0x74
#define OP_POWEROFF     0x75
#define OP_REBOOT       0x76
#define OP_MOUSE_BUTTONS 0x77
#define OP_WALLPAPER    0x78

#define OP_JUMP         0x80
#define OP_JUMP_FALSE   0x81

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

    int is_array;

    int scope;

    VmValue value;

} VmVariable;

typedef struct
{
    unsigned int return_ip;

} VmCallFrame;

typedef struct
{
    unsigned int body_ip;
    unsigned int param_count;

    char param_names[NWO_MAX_PARAMS][NWO_NAME_SIZE];
    unsigned char param_types[NWO_MAX_PARAMS];
    unsigned char param_arrays[NWO_MAX_PARAMS];

} VmFunctionInfo;

typedef struct
{
    NwoProgram* program;

    unsigned int ip;

    VmValue stack[NWO_STACK_SIZE];
    int sp;

    VmVariable variables[NWO_VARIABLES];

    VmCallFrame call_stack[NWO_MAX_CALL_DEPTH];
    int call_sp;
    int scope_depth;

    int running;

    int result;

    unsigned char text_color;

} NwoVM;


static int vm_push(
    NwoVM* vm,
    VmValue value)
{
    if (vm->sp >= NWO_STACK_SIZE)
    {
        print("[NWO VM] Stack overflow.\n");
        return 0;
    }

    vm->stack[vm->sp] = value;
    vm->sp++;

    return 1;
}


static int vm_pop(
    NwoVM* vm,
    VmValue* value)
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


static int vm_read_u8(
    NwoVM* vm,
    unsigned char* value)
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


static int vm_read_u32(
    NwoVM* vm,
    unsigned int* value)
{
    unsigned char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;

    if (!vm_read_u8(vm, &b0))
        return 0;

    if (!vm_read_u8(vm, &b1))
        return 0;

    if (!vm_read_u8(vm, &b2))
        return 0;

    if (!vm_read_u8(vm, &b3))
        return 0;

    if (value != 0)
    {
        *value =
            ((unsigned int)b0) |
            ((unsigned int)b1 << 8) |
            ((unsigned int)b2 << 16) |
            ((unsigned int)b3 << 24);
    }

    return 1;
}


static int vm_read_u64(
    NwoVM* vm,
    unsigned long long* value)
{
    unsigned int low;
    unsigned int high;

    if (!vm_read_u32(vm, &low))
        return 0;

    if (!vm_read_u32(vm, &high))
        return 0;

    if (value != 0)
    {
        *value =
            ((unsigned long long)low) |
            ((unsigned long long)high << 32);
    }

    return 1;
}
static int vm_read_string(
    NwoVM* vm,
    const char** value,
    unsigned int* length)
{
    unsigned int len;
    unsigned int start;

    if (!vm_read_u32(vm, &len))
        return 0;

    start = vm->ip;

    if (start > vm->program->code_size)
        return 0;

    if (len >
        vm->program->code_size - start)
    {
        print("[NWO VM] Invalid string length.\n");
        return 0;
    }

    if (value != 0)
    {
        *value =
            (const char*)
            (vm->program->code + start);
    }

    if (length != 0)
        *length = len;

    vm->ip += len;

    return 1;
}
static int vm_string_equals(
    const char* a,
    unsigned int a_len,
    const char* b,
    unsigned int b_len)
{
    unsigned int i;

    if (a == 0 || b == 0)
        return 0;

    if (a_len != b_len)
        return 0;

    for (i = 0; i < a_len; i++)
    {
        if (a[i] != b[i])
            return 0;
    }

    return 1;
}

static void vm_clear_variables(
    NwoVM* vm)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        vm->variables[i].used = 0;
        vm->variables[i].declared_type = 0;
        vm->variables[i].is_array = 0;
        vm->variables[i].scope = 0;

        vm->variables[i].name[0] = '\0';

        vm->variables[i].value.type =
            VM_NONE;

        vm->variables[i].value.string_length = 0;
        vm->variables[i].value.as.integer = 0;
    }
}


static void vm_release_scope(
    NwoVM* vm,
    int scope)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        if (vm->variables[i].used &&
            vm->variables[i].scope == scope)
        {
            vm->variables[i].used = 0;
            vm->variables[i].declared_type = 0;
            vm->variables[i].is_array = 0;
            vm->variables[i].name[0] = '\0';
            vm->variables[i].value.type = VM_NONE;
            vm->variables[i].value.string_length = 0;
            vm->variables[i].value.as.integer = 0;
        }
    }
}


static int vm_name_equals(
    const char* a,
    const char* b)
{
    int i = 0;

    if (a == 0 || b == 0)
        return 0;

    while (a[i] != '\0' &&
           b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == '\0' &&
           b[i] == '\0';
}


static int vm_find_variable_in_scope(
    NwoVM* vm,
    const char* name,
    int scope)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        if (vm->variables[i].used &&
            vm->variables[i].scope == scope &&
            vm_name_equals(
                vm->variables[i].name,
                name))
        {
            return i;
        }
    }

    return -1;
}


static int vm_find_variable(
    NwoVM* vm,
    const char* name)
{
    int i = vm_find_variable_in_scope(
        vm,
        name,
        vm->scope_depth);

    if (i != -1)
        return i;

    if (vm->scope_depth > 0)
        return vm_find_variable_in_scope(vm, name, 0);

    return -1;
}


static int vm_declare_variable(
    NwoVM* vm,
    const char* name,
    unsigned char type)
{
    int i;
    int slot = -1;

    if (name == 0)
        return -1;
    i = vm_find_variable_in_scope(vm, name, vm->scope_depth);
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
    vm->variables[slot].is_array = 0;
    vm->variables[slot].scope = vm->scope_depth;

    i = 0;

    while (name[i] != '\0' &&
           i < NWO_NAME_SIZE - 1)
    {
        vm->variables[slot].name[i] =
            name[i];

        i++;
    }

    vm->variables[slot].name[i] = '\0';

    vm->variables[slot].value.type =
        VM_NONE;

    vm->variables[slot].value.string_length =
        0;

    vm->variables[slot].value.as.integer =
        0;

    return slot;
}

static int vm_to_int(
    VmValue* value)
{
    if (value == 0)
        return 0;

    switch (value->type)
    {
        case VM_INT:
            return value->as.integer;

        case VM_BOOL:
            return value->as.boolean ? 1 : 0;

        case VM_CHAR:
            return (int)value->as.character;

        case VM_FLOAT:
            return (int)value->as.floating;

        default:
            return 0;
    }
}

static int vm_string_compare(
    VmValue* a,
    VmValue* b)
{
    const unsigned char* left;
    const unsigned char* right;
    unsigned int left_len;
    unsigned int right_len;
    unsigned int count;
    unsigned int i;

    if (a == 0 || b == 0)
        return 0;

    if (a->type != VM_STRING || b->type != VM_STRING)
        return vm_to_int(a) - vm_to_int(b);

    left = (const unsigned char*)a->as.string;
    right = (const unsigned char*)b->as.string;

    if (left == 0)
        left = (const unsigned char*)"";
    if (right == 0)
        right = (const unsigned char*)"";

    left_len = a->string_length;
    right_len = b->string_length;

    count = left_len < right_len ? left_len : right_len;

    for (i = 0; i < count; i++)
    {
        if (left[i] != right[i])
            return (int)left[i] - (int)right[i];
    }

    if (left_len == right_len)
        return 0;

    return left_len < right_len ? -1 : 1;
}

static int vm_string_compare_n(
    VmValue* a,
    VmValue* b,
    int n)
{
    const unsigned char* left;
    const unsigned char* right;
    unsigned int left_len;
    unsigned int right_len;
    unsigned int count;
    unsigned int i;

    if (a == 0 || b == 0 || n <= 0)
        return 0;

    if (a->type != VM_STRING || b->type != VM_STRING)
        return vm_to_int(a) - vm_to_int(b);

    left = (const unsigned char*)a->as.string;
    right = (const unsigned char*)b->as.string;

    if (left == 0)
        left = (const unsigned char*)"";
    if (right == 0)
        right = (const unsigned char*)"";

    left_len = a->string_length;
    right_len = b->string_length;

    count = left_len < right_len ? left_len : right_len;

    if ((unsigned int)n < count)
        count = (unsigned int)n;

    for (i = 0; i < count; i++)
    {
        if (left[i] != right[i])
            return (int)left[i] - (int)right[i];
    }

    if ((unsigned int)n <= count)
        return 0;

    if (left_len == right_len)
        return 0;

    return left_len < right_len ? -1 : 1;
}

static int vm_values_equal(
    VmValue* a,
    VmValue* b)
{
    if (a == 0 || b == 0)
        return 0;

    if (a->type == VM_STRING ||
        b->type == VM_STRING)
    {
        return vm_string_compare(a, b) == 0;
    }

    return vm_to_int(a) == vm_to_int(b);
}

static int vm_binary_arithmetic(
    NwoVM* vm,
    unsigned char opcode)
{
    VmValue left;
    VmValue right;
    VmValue result;

    int a;
    int b;

    if (!vm_pop(vm, &right))
        return 0;

    if (!vm_pop(vm, &left))
        return 0;

    a = vm_to_int(&left);
    b = vm_to_int(&right);

    result.type = VM_INT;
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

        default:
            print("[NWO VM] Invalid arithmetic opcode.\n");
            return 0;
    }

    return vm_push(vm, result);
}

static int vm_compare(
    NwoVM* vm,
    unsigned char opcode)
{
    VmValue left;
    VmValue right;
    VmValue result;

    int a;
    int b;
    int r = 0;

    if (!vm_pop(vm, &right))
        return 0;

    if (!vm_pop(vm, &left))
        return 0;
    a = vm_to_int(&left);
    b = vm_to_int(&right);

    switch (opcode)
    {
        case OP_EQ:
            r = vm_values_equal(&left, &right);
            break;

        case OP_NEQ:
            r = !vm_values_equal(&left, &right);
            break;

        case OP_LT:
            r = a < b;
            break;

        case OP_GT:
            r = a > b;
            break;

        case OP_LTE:
            r = a <= b;
            break;

        case OP_GTE:
            r = a >= b;
            break;

        default:
            print("[NWO VM] Invalid comparison opcode.\n");
            return 0;
    }

    result.type = VM_BOOL;
    result.as.boolean =
        r ? 1 : 0;

    return vm_push(vm, result);
}

static void vm_print_string(
    const char* string,
    unsigned int length)
{
    unsigned int i;

    if (string == 0)
        return;

    for (i = 0; i < length; i++)
        putchar_os(string[i]);
}

static void vm_output(
    VmValue* value)
{
    if (value == 0)
        return;

    switch (value->type)
    {
        case VM_INT:
            print_int(value->as.integer);
            break;

        case VM_BOOL:
            if (value->as.boolean)
                print("true");
            else
                print("false");
            break;

        case VM_CHAR:
            putchar_os(
                (char)value->as.character);
            break;

        case VM_STRING:
            vm_print_string(
                value->as.string,
                value->string_length);

            break;

        case VM_FLOAT:
            print_int(
                (int)value->as.floating);
            break;

        default:
            break;
    }
}

static int vm_skip_name(
    NwoProgram* program,
    unsigned int* ip)
{
    unsigned int len;

    if (*ip + 4 > program->code_size)
        return 0;

    len =
        ((unsigned int)program->code[*ip]) |
        ((unsigned int)program->code[*ip + 1] << 8) |
        ((unsigned int)program->code[*ip + 2] << 16) |
        ((unsigned int)program->code[*ip + 3] << 24);

    *ip += 4;

    if (len > program->code_size - *ip)
        return 0;

    *ip += len;

    return 1;
}

static int vm_code_name_equals(
    NwoProgram* program,
    unsigned int start,
    unsigned int len,
    const char* target)
{
    unsigned int i;

    if (len >= NWO_NAME_SIZE)
        return 0;

    for (i = 0; i < len; i++)
    {
        if (program->code[start + i] !=
            (unsigned char)target[i])
            return 0;
    }

    return target[len] == '\0';
}

static int vm_find_function(
    NwoProgram* program,
    const char* target,
    VmFunctionInfo* info)
{
    unsigned int ip = 0;

    while (ip < program->code_size)
    {
        unsigned char opcode = program->code[ip++];

        switch (opcode)
        {
            case OP_FUNC_BEGIN:
            {
                unsigned int len;
                unsigned int name_start;
                unsigned char return_type;
                unsigned char param_count;
                unsigned int i;
                VmFunctionInfo found;

                if (ip + 4 > program->code_size)
                    return 0;

                len =
                    ((unsigned int)program->code[ip]) |
                    ((unsigned int)program->code[ip + 1] << 8) |
                    ((unsigned int)program->code[ip + 2] << 16) |
                    ((unsigned int)program->code[ip + 3] << 24);

                ip += 4;
                name_start = ip;

                if (len > program->code_size - ip)
                    return 0;

                ip += len;

                if (ip >= program->code_size)
                    return 0;

                return_type = program->code[ip++];
                (void)return_type;

                if (ip >= program->code_size)
                    return 0;

                param_count = program->code[ip++];

                if (param_count > NWO_MAX_PARAMS)
                    return 0;

                found.body_ip = 0;
                found.param_count = param_count;

                for (i = 0; i < param_count; i++)
                {
                    unsigned int plen;
                    unsigned int pstart;
                    unsigned int j;

                    if (ip + 4 > program->code_size)
                        return 0;

                    plen =
                        ((unsigned int)program->code[ip]) |
                        ((unsigned int)program->code[ip + 1] << 8) |
                        ((unsigned int)program->code[ip + 2] << 16) |
                        ((unsigned int)program->code[ip + 3] << 24);

                    ip += 4;
                    pstart = ip;

                    if (plen > program->code_size - ip)
                        return 0;

                    ip += plen;

                    if (ip >= program->code_size)
                        return 0;

                    found.param_types[i] = program->code[ip++];

                    if (ip >= program->code_size)
                        return 0;

                    found.param_arrays[i] = program->code[ip++];

                    for (j = 0;
                         j < plen &&
                         j < NWO_NAME_SIZE - 1;
                         j++)
                    {
                        found.param_names[i][j] =
                            (char)program->code[pstart + j];
                    }

                    found.param_names[i][j] = '\0';
                }

                if (vm_code_name_equals(
                        program,
                        name_start,
                        len,
                        target))
                {
                    found.body_ip = ip;
                    *info = found;
                    return 1;
                }

                break;
            }

            case OP_PUSH_INT:
            case OP_PUSH_FLOAT:
                if (ip + 8 > program->code_size)
                    return 0;
                ip += 8;
                break;

            case OP_PUSH_CHAR:
                if (ip + 4 > program->code_size)
                    return 0;
                ip += 4;
                break;

            case OP_PUSH_BOOL:
                if (ip + 1 > program->code_size)
                    return 0;
                ip += 1;
                break;

            case OP_PUSH_STRING:
            case OP_LOAD:
            case OP_STORE:
            case OP_IN_STORE:
                if (!vm_skip_name(program, &ip))
                    return 0;
                break;

            case OP_CALL:
                if (!vm_skip_name(program, &ip))
                    return 0;
                if (ip + 4 > program->code_size)
                    return 0;
                ip += 4;
                break;

            case OP_DECLARE:
                if (!vm_skip_name(program, &ip))
                    return 0;
                if (ip + 1 > program->code_size)
                    return 0;
                ip += 1;
                break;

            case OP_DECLARE_ARR:
                if (!vm_skip_name(program, &ip))
                    return 0;
                if (ip + 5 > program->code_size)
                    return 0;
                ip += 5;
                break;

            case OP_LINE_IN:
            {
                unsigned int count;
                unsigned int i;

                if (ip + 4 > program->code_size)
                    return 0;

                count =
                    ((unsigned int)program->code[ip]) |
                    ((unsigned int)program->code[ip + 1] << 8) |
                    ((unsigned int)program->code[ip + 2] << 16) |
                    ((unsigned int)program->code[ip + 3] << 24);

                ip += 4;

                for (i = 0; i < count; i++)
                {
                    if (!vm_skip_name(program, &ip))
                        return 0;
                }

                break;
            }

            case OP_JUMP:
            case OP_JUMP_FALSE:
                if (ip + 4 > program->code_size)
                    return 0;
                ip += 4;
                break;

            default:
                break;
        }
    }

    return 0;
}

static int vm_find_main(
    NwoProgram* program,
    unsigned int* main_ip)
{
    VmFunctionInfo info;

    if (!vm_find_function(program, "main", &info))
        return 0;

    *main_ip = info.body_ip;

    return 1;
}

int nwo_execute(
    NwoProgram* program)
{
    NwoVM vm;

    unsigned int main_ip;

    if (program == 0)
        return -1;

    if (program->code == 0 ||
        program->code_size == 0)
    {
        print("[NWO VM] Empty program.\n");
        return -1;
    }
    vm.program = program;

    vm.ip = 0;

    vm.sp = 0;
    vm.call_sp = 0;
    vm.scope_depth = 0;

    vm.running = 1;
    vm.result = 0;

    vm_clear_variables(&vm);

    vm.text_color = 7;
    set_color(vm.text_color);

    if (!vm_find_main(
            program,
            &main_ip))
    {
        print("[NWO VM] main() not found.\n");
        return -1;
    }
    vm.ip = main_ip;


    while (vm.running)
    {
        unsigned char opcode;

        if (!vm_read_u8(
                &vm,
                &opcode))
        {
            return -1;
        }


        switch (opcode)
        {
            case OP_NOP:
                break;

            case OP_PUSH_INT:
            {
                unsigned long long raw;
                VmValue value;

                if (!vm_read_u64(
                        &vm,
                        &raw))
                {
                    return -1;
                }

                value.type =
                    VM_INT;

                value.as.integer =
                    (int)raw;

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_PUSH_FLOAT:
            {
                unsigned long long bits;
                double value;
                VmValue vm_value;

                if (!vm_read_u64(
                        &vm,
                        &bits))
                {
                    return -1;
                }

                {
                    unsigned char* dst =
                        (unsigned char*)&value;

                    unsigned char* src =
                        (unsigned char*)&bits;

                    int i;

                    for (i = 0; i < 8; i++)
                        dst[i] = src[i];
                }

                vm_value.type =
                    VM_FLOAT;

                vm_value.as.floating =
                    value;

                if (!vm_push(
                        &vm,
                        vm_value))
                {
                    return -1;
                }

                break;
            }
            case OP_PUSH_STRING:
            {
                const char* string;
                unsigned int length;

                VmValue value;

                if (!vm_read_string(
                        &vm,
                        &string,
                        &length))
                {
                    return -1;
                }
                (void)length;

                value.type =
                    VM_STRING;

                value.string_length =
                    length;

                value.as.string =
                    string;

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_PUSH_CHAR:
            {
                unsigned int character;

                VmValue value;

                if (!vm_read_u32(
                        &vm,
                        &character))
                {
                    return -1;
                }

                value.type =
                    VM_CHAR;

                value.as.character =
                    character;

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_PUSH_BOOL:
            {
                unsigned char boolean;

                VmValue value;

                if (!vm_read_u8(
                        &vm,
                        &boolean))
                {
                    return -1;
                }

                value.type =
                    VM_BOOL;

                value.as.boolean =
                    boolean ? 1 : 0;

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_POP:
            {
                VmValue unused;

                if (!vm_pop(
                        &vm,
                        &unused))
                {
                    return -1;
                }

                break;
            }

            case OP_DECLARE:
            {
                unsigned int len;
                const char* name;

                unsigned char type;

                char temp[NWO_NAME_SIZE];

                unsigned int i;

                if (!vm_read_u32(
                        &vm,
                        &len))
                {
                    return -1;
                }

                if (len >= NWO_NAME_SIZE)
                {
                    print(
                        "[NWO VM] Variable name too long.\n");

                    return -1;
                }

                if (vm.ip + len >
                    program->code_size)
                {
                    return -1;
                }

                name =
                    (const char*)
                    (program->code + vm.ip);

                vm.ip += len;

                if (!vm_read_u8(
                        &vm,
                        &type))
                {
                    return -1;
                }

                for (i = 0;
                     i < len;
                     i++)
                {
                    temp[i] = name[i];
                }

                temp[len] = '\0';

                if (vm_declare_variable(
                        &vm,
                        temp,
                        type) == -1)
                {
                    return -1;
                }

                break;
            }

            case OP_DECLARE_ARR:
            {
                unsigned int len;
                const char* name;
                unsigned char type;
                unsigned int size;
                char temp[NWO_NAME_SIZE];
                unsigned int i;
                int index;

                if (!vm_read_u32(&vm, &len))
                    return -1;

                if (len >= NWO_NAME_SIZE)
                {
                    print(
                        "[NWO VM] Variable name too long.\n");
                    return -1;
                }

                if (vm.ip + len > program->code_size)
                    return -1;

                name = (const char*)(program->code + vm.ip);
                vm.ip += len;

                if (!vm_read_u8(&vm, &type))
                    return -1;

                if (!vm_read_u32(&vm, &size))
                    return -1;

                (void)size;

                for (i = 0; i < len; i++)
                    temp[i] = name[i];

                temp[len] = '\0';

                index = vm_declare_variable(&vm, temp, type);

                if (index == -1)
                    return -1;

                vm.variables[index].is_array = 1;

                break;
            }

            case OP_LOAD:
            {
                unsigned int len;
                const char* name;

                char temp[NWO_NAME_SIZE];

                unsigned int i;

                int index;

                VmValue value;

                if (!vm_read_u32(
                        &vm,
                        &len))
                {
                    return -1;
                }

                if (len >= NWO_NAME_SIZE)
                {
                    print(
                        "[NWO VM] Variable name too long.\n");

                    return -1;
                }

                if (vm.ip + len >
                    program->code_size)
                {
                    return -1;
                }

                name =
                    (const char*)
                    (program->code + vm.ip);

                vm.ip += len;

                for (i = 0;
                     i < len;
                     i++)
                {
                    temp[i] = name[i];
                }

                temp[len] = '\0';

                index =
                    vm_find_variable(
                        &vm,
                        temp);

                if (index == -1)
                {
                    print(
                        "[NWO VM] Unknown variable: ");

                    print(temp);
                    putchar_os('\n');

                    return -1;
                }

                value =
                    vm.variables[index].value;
                if (value.type == VM_NONE)
                {
                    value.type =
                        VM_INT;

                    value.as.integer =
                        0;
                }

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_STORE:
            {
                unsigned int len;
                const char* name;

                char temp[NWO_NAME_SIZE];

                unsigned int i;

                int index;

                VmValue value;
                if (!vm_pop(
                        &vm,
                        &value))
                {
                    return -1;
                }

                if (!vm_read_u32(
                        &vm,
                        &len))
                {
                    return -1;
                }

                if (len >= NWO_NAME_SIZE)
                {
                    print(
                        "[NWO VM] Variable name too long.\n");

                    return -1;
                }

                if (vm.ip + len >
                    program->code_size)
                {
                    return -1;
                }

                name =
                    (const char*)
                    (program->code + vm.ip);

                vm.ip += len;

                for (i = 0;
                     i < len;
                     i++)
                {
                    temp[i] =
                        name[i];
                }

                temp[len] = '\0';

                index =
                    vm_find_variable(
                        &vm,
                        temp);

                if (index == -1)
                {
                    index =
                        vm_declare_variable(
                            &vm,
                            temp,
                            1);
                }

                if (index == -1)
                    return -1;

                vm.variables[index].value =
                    value;

                break;
            }

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:

                if (!vm_binary_arithmetic(
                        &vm,
                        opcode))
                {
                    return -1;
                }
                break;

            case OP_EQ:
            case OP_NEQ:
            case OP_LT:
            case OP_GT:
            case OP_LTE:
            case OP_GTE:

                if (!vm_compare(
                        &vm,
                        opcode))
                {
                    return -1;
                }

                break;

            case OP_STRCMP:
            {
                VmValue right;
                VmValue left;
                VmValue result;

                if (!vm_pop(&vm, &right))
                    return -1;

                if (!vm_pop(&vm, &left))
                    return -1;

                result.type = VM_INT;
                result.string_length = 0;
                result.as.integer =
                    vm_string_compare(&left, &right);

                if (!vm_push(&vm, result))
                    return -1;

                break;
            }

            case OP_STRNCMP:
            {
                VmValue count;
                VmValue right;
                VmValue left;
                VmValue result;

                if (!vm_pop(&vm, &count))
                    return -1;

                if (!vm_pop(&vm, &right))
                    return -1;

                if (!vm_pop(&vm, &left))
                    return -1;

                result.type = VM_INT;
                result.string_length = 0;
                result.as.integer =
                    vm_string_compare_n(
                        &left,
                        &right,
                        vm_to_int(&count));

                if (!vm_push(&vm, result))
                    return -1;

                break;
            }

            case OP_OUT:
            {
                VmValue value;

                if (!vm_pop(
                        &vm,
                        &value))
                {
                    return -1;
                }

                vm_output(&value);

                break;
            }
            case OP_OUT_ENDL:
                putchar_os('\n');
                break;

            case OP_IN:
            {
                char buffer[NWO_INPUT_SIZE];

                int result = 0;
                int sign = 1;
                int i = 0;

                VmValue value;

                print("> ");

                read_line(
                    buffer,
                    NWO_INPUT_SIZE);

                if (buffer[0] == '-')
                {
                    sign = -1;
                    i = 1;
                }

                while (buffer[i] >= '0' &&
                       buffer[i] <= '9')
                {
                    result =
                        result * 10 +
                        (buffer[i] - '0');

                    i++;
                }

                value.type =
                    VM_INT;

                value.as.integer =
                    result * sign;

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_IN_STORE:
            {
                unsigned int len;
                const char* name;
                char temp[NWO_NAME_SIZE];
                char buffer[NWO_INPUT_SIZE];
                unsigned int i;
                int index;
                VmVariable* variable;

                if (!vm_read_u32(&vm, &len))
                    return -1;

                if (len >= NWO_NAME_SIZE)
                {
                    print(
                        "[NWO VM] Variable name too long.\n");
                    return -1;
                }

                if (vm.ip + len > program->code_size)
                    return -1;

                name = (const char*)(program->code + vm.ip);
                vm.ip += len;

                for (i = 0; i < len; i++)
                    temp[i] = name[i];

                temp[len] = '\0';

                read_line(buffer, NWO_INPUT_SIZE);

                index = vm_find_variable(&vm, temp);

                if (index == -1)
                {
                    index = vm_declare_variable(&vm, temp, 1);

                    if (index == -1)
                        return -1;
                }

                variable = &vm.variables[index];

                if (variable->declared_type == 7 ||
                    variable->is_array)
                {
                    unsigned int length = 0;

                    while (buffer[length] != '\0' &&
                           length < NWO_INPUT_SIZE - 1)
                    {
                        vm_string_storage[index][length] =
                            buffer[length];
                        length++;
                    }

                    vm_string_storage[index][length] = '\0';

                    variable->value.type = VM_STRING;
                    variable->value.string_length = length;
                    variable->value.as.string =
                        vm_string_storage[index];
                }
                else
                {
                    int result = 0;
                    int sign = 1;
                    int pos = 0;

                    if (buffer[0] == '-')
                    {
                        sign = -1;
                        pos = 1;
                    }

                    while (buffer[pos] >= '0' &&
                           buffer[pos] <= '9')
                    {
                        result = result * 10 +
                            (buffer[pos] - '0');
                        pos++;
                    }

                    variable->value.type = VM_INT;
                    variable->value.string_length = 0;
                    variable->value.as.integer =
                        result * sign;
                }

                break;
            }

            case OP_LINE_IN:
            {
                unsigned int count;
                unsigned int item;

                static char line_storage[NWO_INPUT_SIZE];

                if (!vm_read_u32(
                        &vm,
                        &count))
                {
                    return -1;
                }

                for (item = 0;
                     item < count;
                     item++)
                {
                    unsigned int len;
                    const char* name;

                    char temp[NWO_NAME_SIZE];

                    unsigned int i;

                    int index;

                    if (!vm_read_u32(
                            &vm,
                            &len))
                    {
                        return -1;
                    }

                    if (len >= NWO_NAME_SIZE)
                    {
                        print(
                            "[NWO VM] Variable name too long.\n");

                        return -1;
                    }

                    if (vm.ip + len >
                        program->code_size)
                    {
                        return -1;
                    }

                    name =
                        (const char*)
                        (program->code + vm.ip);

                    vm.ip += len;

                    for (i = 0;
                         i < len;
                         i++)
                    {
                        temp[i] =
                            name[i];
                    }

                    temp[len] = '\0';

                    read_line(
                        line_storage,
                        NWO_INPUT_SIZE);

                    index =
                        vm_find_variable(
                            &vm,
                            temp);

                    if (index == -1)
                    {
                        index =
                            vm_declare_variable(
                                &vm,
                                temp,
                                7);
                    }

                    if (index == -1)
                        return -1;

                    vm.variables[index].value.type =
                        VM_STRING;

                    {
                        unsigned int length = 0;

                        while (line_storage[length] != '\0' &&
                               length < NWO_INPUT_SIZE - 1)
                        {
                            length++;
                        }

                        vm.variables[index].value.string_length =
                            length;
                    }

                    vm.variables[index].value.as.string =
                        line_storage;
                }

                break;
            }
            case OP_RECT:
            {
                VmValue color;
                VmValue height;
                VmValue width;
                VmValue y;
                VmValue x;

                if (!vm_pop(&vm, &color))
                    return -1;

                if (!vm_pop(&vm, &height))
                    return -1;

                if (!vm_pop(&vm, &width))
                    return -1;

                if (!vm_pop(&vm, &y))
                    return -1;

                if (!vm_pop(&vm, &x))
                    return -1;

                graphics_begin();

                fb_draw_rect(
                    vm_to_int(&x),
                    vm_to_int(&y),
                    vm_to_int(&width),
                    vm_to_int(&height),
                    (unsigned int)vm_to_int(&color));

                break;
            }

            case OP_DRAW_FRAME:
            {
                VmValue color;
                VmValue height;
                VmValue width;
                VmValue y;
                VmValue x;

                if (!vm_pop(&vm, &color))
                    return -1;
                if (!vm_pop(&vm, &height))
                    return -1;
                if (!vm_pop(&vm, &width))
                    return -1;
                if (!vm_pop(&vm, &y))
                    return -1;
                if (!vm_pop(&vm, &x))
                    return -1;

                graphics_begin();

                fb_draw_frame(
                    vm_to_int(&x),
                    vm_to_int(&y),
                    vm_to_int(&width),
                    vm_to_int(&height),
                    (unsigned int)vm_to_int(&color));

                break;
            }

            case OP_DRAW_CUBE:
            {
                VmValue color;
                VmValue size;

                if (!vm_pop(&vm, &color))
                    return -1;
                if (!vm_pop(&vm, &size))
                    return -1;

                (void)size;

                graphics_begin();
                fb_draw_cube_default((unsigned int)vm_to_int(&color));

                break;
            }

            case OP_DRAW_WINDOW:
            {
                VmValue title;
                VmValue color;
                VmValue height;
                VmValue width;
                VmValue y;
                VmValue x;

                if (!vm_pop(&vm, &title))
                    return -1;
                if (!vm_pop(&vm, &color))
                    return -1;
                if (!vm_pop(&vm, &height))
                    return -1;
                if (!vm_pop(&vm, &width))
                    return -1;
                if (!vm_pop(&vm, &y))
                    return -1;
                if (!vm_pop(&vm, &x))
                    return -1;

                graphics_begin();

                fb_draw_window(
                    vm_to_int(&x),
                    vm_to_int(&y),
                    vm_to_int(&width),
                    vm_to_int(&height),
                    (unsigned int)vm_to_int(&color),
                    title.type == VM_STRING ? title.as.string : "");

                break;
            }

            case OP_DRAW_TEXT:
            {
                VmValue text;
                VmValue color;
                VmValue y;
                VmValue x;

                if (!vm_pop(&vm, &text))
                    return -1;
                if (!vm_pop(&vm, &color))
                    return -1;
                if (!vm_pop(&vm, &y))
                    return -1;
                if (!vm_pop(&vm, &x))
                    return -1;

                graphics_begin();

                fb_draw_text(
                    vm_to_int(&x),
                    vm_to_int(&y),
                    text.type == VM_STRING ? text.as.string : "",
                    (unsigned int)vm_to_int(&color));

                break;
            }

            case OP_COLOR_PUSH:
            {
                VmValue value;

                value.type = VM_INT;
                value.string_length = 0;
                value.as.integer = (int)vm.text_color;

                if (!vm_push(&vm, value))
                    return -1;

                break;
            }

            case OP_COLOR_POP:
            {
                VmValue value;

                if (!vm_pop(&vm, &value))
                    return -1;

                vm.text_color =
                    (unsigned char)vm_to_int(&value);

                set_color(vm.text_color);

                break;
            }

            case OP_COLOR:
            {
                VmValue value;

                if (!vm_pop(
                        &vm,
                        &value))
                {
                    return -1;
                }

                vm.text_color =
                    (unsigned char)vm_to_int(&value);

                set_color(vm.text_color);

                break;
            }

            case OP_CLEAR:
                clear();
                break;

            case OP_SLEEP:
            {
                VmValue value;

                if (!vm_pop(&vm, &value))
                    return -1;

                sleep((unsigned int)vm_to_int(&value));

                break;
            }

            case OP_GETKEY:
            {
                VmValue value;

                value.type =
                    VM_INT;

                value.as.integer =
                    keyboard_getkey();

                if (!vm_push(
                        &vm,
                        value))
                {
                    return -1;
                }

                break;
            }

            case OP_WALLPAPER:
                graphics_begin();
                fb_draw_wallpaper();
                break;

            case OP_MOUSE_X:
            case OP_MOUSE_Y:
            case OP_MOUSE_BUTTONS:
            {
                VmValue value;
                int result = 0;

                if (opcode == OP_MOUSE_X)
                    result = mouse_x();
                else if (opcode == OP_MOUSE_Y)
                    result = mouse_y();
                else
                    result = mouse_buttons();

                value.type = VM_INT;
                value.as.integer = result;

                if (!vm_push(&vm, value))
                    return -1;

                break;
            }

            case OP_POWEROFF:
                poweroff();
                break;

            case OP_REBOOT:
                reboot();
                break;

            case OP_GETKEY_FILTER:
            {
                VmValue filter;
                VmValue value;
                int key;

                if (!vm_pop(&vm, &filter))
                    return -1;

                do
                {
                    key = keyboard_getkey();
                }
                while (key != vm_to_int(&filter));

                value.type = VM_INT;
                value.as.integer = key;

                if (!vm_push(&vm, value))
                    return -1;

                break;
            }

            case OP_ASM_HLT:
#ifdef NWC_HOST_TEST
                print("[hlt]");
#else
                __asm__ volatile("hlt");
#endif
                break;

            case OP_TIME:
            {
                int hour;
                int minute;
                int second;

                get_time(
                    &hour,
                    &minute,
                    &second);

                if (hour < 10)
                    putchar_os('0');
                print_int(hour);
                putchar_os(':');

                if (minute < 10)
                    putchar_os('0');
                print_int(minute);
                putchar_os(':');

                if (second < 10)
                    putchar_os('0');
                print_int(second);
                putchar_os('\n');

                break;
            }

            case OP_CALL:
            {
                unsigned int len;
                unsigned int argc;
                unsigned int i;
                char name[NWO_NAME_SIZE];
                VmFunctionInfo info;

                if (!vm_read_u32(&vm, &len))
                    return -1;

                if (len >= NWO_NAME_SIZE ||
                    vm.ip + len > program->code_size)
                {
                    print("[NWO VM] Invalid function name.\n");
                    return -1;
                }

                for (i = 0; i < len; i++)
                    name[i] = (char)program->code[vm.ip + i];

                name[len] = '\0';
                vm.ip += len;

                if (!vm_read_u32(&vm, &argc))
                    return -1;

                if (!vm_find_function(program, name, &info))
                {
                    print("[NWO VM] Unknown function: ");
                    print(name);
                    putchar_os('\n');
                    return -1;
                }

                if (argc != info.param_count)
                {
                    print("[NWO VM] Wrong argument count for ");
                    print(name);
                    putchar_os('\n');
                    return -1;
                }

                if (vm.call_sp >= NWO_MAX_CALL_DEPTH)
                {
                    print("[NWO VM] Call stack overflow.\n");
                    return -1;
                }

                vm.call_stack[vm.call_sp].return_ip = vm.ip;
                vm.call_sp++;
                vm.scope_depth = vm.call_sp;

                for (i = info.param_count; i > 0; i--)
                {
                    VmValue value;
                    int index;

                    if (!vm_pop(&vm, &value))
                        return -1;

                    index = vm_declare_variable(
                        &vm,
                        info.param_names[i - 1],
                        info.param_types[i - 1]);

                    if (index == -1)
                        return -1;

                    vm.variables[index].is_array =
                        info.param_arrays[i - 1];
                    vm.variables[index].value = value;
                }

                vm.ip = info.body_ip;

                break;
            }


            case OP_JUMP:
            {
                unsigned int target;

                if (!vm_read_u32(&vm, &target))
                    return -1;

                if (target > program->code_size)
                    return -1;

                vm.ip = target;

                break;
            }

            case OP_JUMP_FALSE:
            {
                unsigned int target;
                VmValue value;

                if (!vm_read_u32(&vm, &target))
                    return -1;

                if (!vm_pop(&vm, &value))
                    return -1;

                if (target > program->code_size)
                    return -1;

                if (!vm_to_int(&value))
                    vm.ip = target;

                break;
            }

            case OP_FUNC_END:
            {
                VmValue value;

                value.type = VM_INT;
                value.as.integer = 0;

                if (vm.call_sp > 0)
                {
                    int finished_scope = vm.call_sp;

                    vm.call_sp--;
                    vm.scope_depth = vm.call_sp;
                    vm_release_scope(&vm, finished_scope);
                    vm.ip = vm.call_stack[vm.call_sp].return_ip;

                    if (!vm_push(&vm, value))
                        return -1;
                }
                else
                {
                    vm.running = 0;
                    vm.result = 0;
                }

                break;
            }


            case OP_RETURN:
            {
                VmValue value;
                VmValue result;

                if (!vm_pop(
                        &vm,
                        &value))
                {
                    return -1;
                }

                result.type = VM_INT;
                result.as.integer = vm_to_int(&value);

                if (vm.call_sp > 0)
                {
                    int finished_scope = vm.call_sp;

                    vm.call_sp--;
                    vm.scope_depth = vm.call_sp;
                    vm_release_scope(&vm, finished_scope);
                    vm.ip = vm.call_stack[vm.call_sp].return_ip;

                    if (!vm_push(&vm, result))
                        return -1;
                }
                else
                {
                    vm.result = result.as.integer;
                    vm.running = 0;
                }

                break;
            }

            case OP_RETURN_VOID:
            {
                VmValue value;

                value.type = VM_INT;
                value.as.integer = 0;

                if (vm.call_sp > 0)
                {
                    int finished_scope = vm.call_sp;

                    vm.call_sp--;
                    vm.scope_depth = vm.call_sp;
                    vm_release_scope(&vm, finished_scope);
                    vm.ip = vm.call_stack[vm.call_sp].return_ip;

                    if (!vm_push(&vm, value))
                        return -1;
                }
                else
                {
                    vm.result = 0;
                    vm.running = 0;
                }

                break;
            }

            case OP_HALT:

                vm.result = 0;
                vm.running = 0;

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