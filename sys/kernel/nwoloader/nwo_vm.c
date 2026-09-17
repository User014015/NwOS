#include "nwo_vm.h"

void print(const char* text);
void print_int(int number);
void putchar_os(char c);

void read_line(char* buffer, int max);

void clear(void);
void set_color(unsigned char color);

int keyboard_getkey(void);

#define NWO_STACK_SIZE      128
#define NWO_VARIABLES       32
#define NWO_NAME_SIZE       32
#define NWO_INPUT_SIZE      256

#define NWO_MAX_CALL_DEPTH  16

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

#define OP_OUT          0x50
#define OP_OUT_ENDL     0x51
#define OP_IN           0x52
#define OP_LINE_IN      0x53

#define OP_TIME         0x60
#define OP_COLOR        0x61
#define OP_GETKEY       0x62
#define OP_CLEAR        0x63

#define OP_FUNC_BEGIN   0x70
#define OP_FUNC_END     0x71
#define OP_CALL         0x72
#define OP_RETURN       0x73
#define OP_RETURN_VOID  0x74

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
    unsigned int return_ip;

} VmCallFrame;

typedef struct
{
    NwoProgram* program;

    unsigned int ip;

    VmValue stack[NWO_STACK_SIZE];
    int sp;

    VmVariable variables[NWO_VARIABLES];

    VmCallFrame call_stack[NWO_MAX_CALL_DEPTH];
    int call_sp;

    int running;

    int result;

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

        vm->variables[i].name[0] = '\0';

        vm->variables[i].value.type =
            VM_NONE;

        vm->variables[i].value.as.integer = 0;
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


static int vm_find_variable(
    NwoVM* vm,
    const char* name)
{
    int i;

    for (i = 0; i < NWO_VARIABLES; i++)
    {
        if (vm->variables[i].used &&
            vm_name_equals(
                vm->variables[i].name,
                name))
        {
            return i;
        }
    }

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

static int vm_values_equal(
    VmValue* a,
    VmValue* b)
{
    if (a == 0 || b == 0)
        return 0;
    if (a->type == VM_STRING ||
        b->type == VM_STRING)
    {
        return 0;
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
            if (value->as.string != 0)
                print(value->as.string);

            break;

        case VM_FLOAT:
            print_int(
                (int)value->as.floating);
            break;

        default:
            break;
    }
}

static int vm_find_main(
    NwoProgram* program,
    unsigned int* main_ip)
{
    unsigned int ip = 0;

    while (ip < program->code_size)
    {
        unsigned char opcode =
            program->code[ip];

        ip++;

        switch (opcode)
        {
            case OP_FUNC_BEGIN:
            {
                unsigned int name_len;
                unsigned int name_start;

                unsigned char return_type;
                if (ip + 4 >
                    program->code_size)
                {
                    return 0;
                }

                name_len =
                    ((unsigned int)
                        program->code[ip]) |
                    ((unsigned int)
                        program->code[ip + 1] << 8) |
                    ((unsigned int)
                        program->code[ip + 2] << 16) |
                    ((unsigned int)
                        program->code[ip + 3] << 24);

                ip += 4;

                name_start = ip;

                if (name_len >
                    program->code_size - ip)
                {
                    return 0;
                }

                ip += name_len;

                if (ip >= program->code_size)
                    return 0;

                return_type =
                    program->code[ip];

                ip++;

                (void)return_type;

                if (name_len == 4 &&
                    program->code[name_start + 0] == 'm' &&
                    program->code[name_start + 1] == 'a' &&
                    program->code[name_start + 2] == 'i' &&
                    program->code[name_start + 3] == 'n')
                {
                    if (main_ip != 0)
                        *main_ip = ip;

                    return 1;
                }

                break;
            }


            case OP_PUSH_INT:
                if (ip + 8 >
                    program->code_size)
                    return 0;

                ip += 8;
                break;


            case OP_PUSH_FLOAT:
                if (ip + 8 >
                    program->code_size)
                    return 0;

                ip += 8;
                break;


            case OP_PUSH_STRING:
            {
                unsigned int len;

                if (ip + 4 >
                    program->code_size)
                    return 0;

                len =
                    ((unsigned int)
                        program->code[ip]) |
                    ((unsigned int)
                        program->code[ip + 1] << 8) |
                    ((unsigned int)
                        program->code[ip + 2] << 16) |
                    ((unsigned int)
                        program->code[ip + 3] << 24);

                ip += 4;

                if (len >
                    program->code_size - ip)
                    return 0;

                ip += len;

                break;
            }


            case OP_PUSH_CHAR:

                if (ip + 4 >
                    program->code_size)
                    return 0;

                ip += 4;
                break;


            case OP_PUSH_BOOL:

                if (ip + 1 >
                    program->code_size)
                    return 0;

                ip += 1;
                break;


            case OP_LOAD:
            case OP_STORE:
            case OP_CALL:
            {
                unsigned int len;

                if (ip + 4 >
                    program->code_size)
                    return 0;

                len =
                    ((unsigned int)
                        program->code[ip]) |
                    ((unsigned int)
                        program->code[ip + 1] << 8) |
                    ((unsigned int)
                        program->code[ip + 2] << 16) |
                    ((unsigned int)
                        program->code[ip + 3] << 24);

                ip += 4;

                if (len >
                    program->code_size - ip)
                    return 0;

                ip += len;

                break;
            }


            case OP_DECLARE:
            {
                unsigned int len;

                if (ip + 4 >
                    program->code_size)
                    return 0;

                len =
                    ((unsigned int)
                        program->code[ip]) |
                    ((unsigned int)
                        program->code[ip + 1] << 8) |
                    ((unsigned int)
                        program->code[ip + 2] << 16) |
                    ((unsigned int)
                        program->code[ip + 3] << 24);

                ip += 4;

                if (len >
                    program->code_size - ip)
                    return 0;

                ip += len;

                if (ip + 1 >
                    program->code_size)
                    return 0;

                ip += 1;

                break;
            }


            case OP_DECLARE_ARR:
            {
                unsigned int len;

                if (ip + 4 >
                    program->code_size)
                    return 0;

                len =
                    ((unsigned int)
                        program->code[ip]) |
                    ((unsigned int)
                        program->code[ip + 1] << 8) |
                    ((unsigned int)
                        program->code[ip + 2] << 16) |
                    ((unsigned int)
                        program->code[ip + 3] << 24);

                ip += 4;

                if (len >
                    program->code_size - ip)
                    return 0;

                ip += len;

                if (ip + 1 >
                    program->code_size)
                    return 0;

                ip += 1;

                if (ip + 4 >
                    program->code_size)
                    return 0;

                ip += 4;

                break;
            }


            default:
                break;
        }

        if (ip > program->code_size)
            return 0;
    }

    return 0;
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

    vm.running = 1;
    vm.result = 0;

    vm_clear_variables(&vm);

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

                    vm.variables[index].value.as.string =
                        line_storage;
                }

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

                set_color(
                    (unsigned char)
                    vm_to_int(&value));

                break;
            }

            case OP_CLEAR:
                clear();
                break;
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

            case OP_TIME:
                break;

            case OP_CALL:
            {
                unsigned int len;

                if (!vm_read_u32(
                        &vm,
                        &len))
                {
                    return -1;
                }

                if (len >
                    program->code_size - vm.ip)
                {
                    return -1;
                }

                vm.ip += len;

                print(
                    "[NWO VM] CALL is not implemented yet.\n");

                return -1;
            }



            case OP_FUNC_END:
                vm.running = 0;
                vm.result = 0;

                break;


            case OP_RETURN:
            {
                VmValue value;

                if (!vm_pop(
                        &vm,
                        &value))
                {
                    return -1;
                }

                vm.result =
                    vm_to_int(&value);

                vm.running = 0;

                break;
            }

            case OP_RETURN_VOID:

                vm.result = 0;
                vm.running = 0;

                break;

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