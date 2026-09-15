#include "../nwo_loader.h"
#include "nwo_vm.h"

void print(const char* text);
void print_int(int number);
void putchar_os(char c);

void print_error(const char* text);
void print_success(const char* text);

void nwo_run(const char* filename)
{
    NwoProgram program;
    int result;

    if (filename == 0 || filename[0] == '\0')
    {
        print_error("Usage: run <file.nwo>\n");
        return;
    }

    print("NWO: loading ");
    print(filename);
    print("...\n");

    if (!nwo_load(filename, &program))
    {
        print_error("Cannot load NWO program.\n");
        return;
    }

    print("NWO: executing...\n\n");

    result = nwo_execute(&program);

    nwo_unload(&program);

    if (result < 0)
    {
        print_error("\nNWO program failed.\n");
        return;
    }

    print("\n");
    print_success("Program finished. Return code: ");
    print_int(result);
    putchar_os('\n');
}