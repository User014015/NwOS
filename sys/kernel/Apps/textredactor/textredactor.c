#include "textredactor.h"
#include "../../kernel.h"
#include "../../../../compiler/nwc_kernel.h"
#include "../../../../compiler/nwc_runtime.h"

#include "../../../../drivers/keyboard.h"

void print(const char* text);
void print_int(int number);
void print_error(const char* text);
void print_success(const char* text);
void putchar_os(char c);

void clear(void);

void read_line(char* buffer, int max);

int starts_with(const char* text, const char* prefix);
int atoi_simple(const char* text);

int fs_find(const char* name);

int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
);

int fs_write_bytes(
    const char* name,
    const unsigned char* data,
    unsigned int size
);

void fs_create(const char* name);
void fs_write(const char* name, const char* text);

#define REDACTOR_MAX_LINES       128
#define REDACTOR_LINE_SIZE       121
#define REDACTOR_SOURCE_SIZE     16384
#define REDACTOR_NWO_SIZE        131072

#define EDITOR_INPUT_F5          (-1001)

static char editor_buffer[
    REDACTOR_MAX_LINES
][
    REDACTOR_LINE_SIZE
];

static int editor_lines = 0;

static void editor_clear_buffer(void)
{
    int line;

    for (line = 0;
         line < REDACTOR_MAX_LINES;
         line++)
    {
        editor_buffer[line][0] = '\0';
    }

    editor_lines = 0;
}

static void editor_load_file(const char* filename)
{
    static char file_data[REDACTOR_SOURCE_SIZE + 1];

    unsigned int data_size = 0;
    unsigned int i = 0;
    int line = 0;
    int column;

    editor_clear_buffer();

    if (!fs_read_text(
            filename,
            file_data,
            sizeof(file_data)))
    {
        return;
    }

    while (
        i < REDACTOR_SOURCE_SIZE &&
        file_data[i] != '\0' &&
        line < REDACTOR_MAX_LINES
    )
    {
        column = 0;

        while (
            i < REDACTOR_SOURCE_SIZE &&
            file_data[i] != '\0' &&
            file_data[i] != '\n' &&
            column < REDACTOR_LINE_SIZE - 1
        )
        {
            editor_buffer[line][column] =
                file_data[i];

            column++;
            i++;
        }

        editor_buffer[line][column] = '\0';
        line++;

        if (file_data[i] == '\n')
            i++;
    }

    data_size = (unsigned int)line;
    editor_lines = (int)data_size;
}

static void editor_show(const char* filename)
{
    int i;

    clear();

    print("========================================\n");
    print("              NwOS EDITOR\n");
    print("========================================\n");
    print("File: ");
    print(filename);
    print("\n\n");

    if (editor_lines == 0)
    {
        print("  [empty]\n");
    }
    else
    {
        for (i = 0; i < editor_lines; i++)
        {
            print_int(i + 1);
            print(" | ");
            print(editor_buffer[i]);
            putchar_os('\n');
        }
    }

    print("\n");
    print("F5              - compile to .nwo\n");
    print("sys/exit        - exit without saving\n");
    print("sys/exit/save   - save and exit\n");
    print("sys/edit <N>    - edit an existing line\n");
    print("----------------------------------------\n");
}

static int editor_build_text(
    char* output,
    unsigned int max_size)
{
    unsigned int position = 0;
    int i;
    int j;

    if (output == 0 || max_size == 0)
        return 0;

    for (i = 0; i < editor_lines; i++)
    {
        j = 0;

        while (editor_buffer[i][j] != '\0')
        {
            if (position >= max_size - 1)
            {
                output[position] = '\0';
                return 0;
            }

            output[position++] =
                editor_buffer[i][j++];
        }

        if (i < editor_lines - 1)
        {
            if (position >= max_size - 1)
            {
                output[position] = '\0';
                return 0;
            }

            output[position++] = '\n';
        }
    }

    output[position] = '\0';
    return 1;
}

static void editor_make_nwo_name(
    const char* filename,
    char* output,
    unsigned int output_size)
{
    unsigned int length = 0;
    unsigned int i;

    if (output_size == 0)
        return;

    while (
        filename != 0 &&
        filename[length] != '\0' &&
        length < output_size - 1
    )
    {
        length++;
    }

    if (length >= 3 &&
        filename[length - 3] == '.' &&
        filename[length - 2] == 'n' &&
        filename[length - 1] == 'w')
    {
        if (length - 3 >= output_size - 4)
            length = output_size - 4;

        for (i = 0; i < length - 3; i++)
            output[i] = filename[i];

        output[length - 3] = '.';
        output[length - 2] = 'n';
        output[length - 1] = 'w';
        output[length] = 'o';
        output[length + 1] = '\0';
        return;
    }

    if (length >= output_size - 5)
        length = output_size - 5;

    for (i = 0; i < length; i++)
        output[i] = filename[i];

    output[length] = '.';
    output[length + 1] = 'n';
    output[length + 2] = 'w';
    output[length + 3] = 'o';
    output[length + 4] = '\0';
}

static int editor_save(const char* filename)
{
    static char output[REDACTOR_SOURCE_SIZE + 1];

    unsigned int size = 0;

    if (!editor_build_text(
            output,
            sizeof(output)))
    {
        print_error(
            "Editor: source is larger than 16383 bytes.\n"
        );

        return 0;
    }

    if (fs_write_bytes(
            filename,
            (const unsigned char*)output,
            (unsigned int)strlen(output)))
    {
        return 1;
    }

    return 0;
}

static void editor_wait_key(void)
{
    int key;

    print("\nPress any key to continue...");

    while (1)
    {
        key = keyboard_getkey();

        if (key != 0)
            return;
    }
}

static int editor_compile(const char* filename)
{
    static char source[REDACTOR_SOURCE_SIZE + 1];
    static unsigned char nwo[REDACTOR_NWO_SIZE];

    char nwo_name[128];

    unsigned int source_size = 0;
    unsigned int nwo_size = 0;
    int i;

    clear();

    print("========================================\n");
    print("             NwC COMPILER\n");
    print("========================================\n");
    print("Input : ");
    print(filename);
    print("\n\n");

    if (!editor_build_text(
            source,
            sizeof(source)))
    {
        print_error("Compilation failed: source is too large.\n");
        editor_wait_key();
        return 0;
    }

    while (
        source[source_size] != '\0' &&
        source_size < sizeof(source) - 1
    )
    {
        source_size++;
    }

    print("Lexing / parsing / codegen...\n\n");

    if (!nwc_compile_source(
            source,
            source_size,
            nwo,
            sizeof(nwo),
            &nwo_size))
    {
        print_error("\nCompilation failed.\n");
        editor_wait_key();
        return 0;
    }

    editor_make_nwo_name(
        filename,
        nwo_name,
        sizeof(nwo_name)
    );

    if (!fs_write_bytes(
            nwo_name,
            nwo,
            nwo_size))
    {
        print_error("\nCould not write generated .nwo file.\n");
        editor_wait_key();
        return 0;
    }

    print_success("Compilation successful.\n\n");

    print("Generated: ");
    print(nwo_name);
    print("\nSize: ");
    print_int((int)nwo_size);
    print(" bytes\n");

    print("\nThe .nwo file is ready for NwOS run.\n");

    editor_wait_key();

    (void)i;
    return 1;
}

static int editor_read_line(
    char* buffer,
    int max)
{
    int length = 0;
    int key;

    if (max <= 0)
        return 0;

    buffer[0] = '\0';

    while (1)
    {
        key = keyboard_getkey();

        if (key == KEY_F5)
        {
            buffer[length] = '\0';
            return EDITOR_INPUT_F5;
        }

        if (key == KEY_ENTER)
        {
            buffer[length] = '\0';
            putchar_os('\n');
            return 1;
        }

        if (key == KEY_BACKSPACE)
        {
            if (length > 0)
            {
                length--;
                buffer[length] = '\0';
                putchar_os('\b');
            }

            continue;
        }

        if (key >= 0 && key < 256)
        {
            if (length < max - 1)
            {
                buffer[length++] = (char)key;
                putchar_os((char)key);
            }
        }
    }
}

void redactor(const char* filename)
{
    char input[REDACTOR_LINE_SIZE];

    editor_load_file(filename);

    while (1)
    {
        editor_show(filename);

        if (editor_lines >= REDACTOR_MAX_LINES)
        {
            print_error(
                "Maximum number of lines reached.\n"
            );

            print(
                "Use F5, sys/exit, sys/exit/save, "
                "or sys/edit <N>.\n"
            );

            print("> ");

            if (editor_read_line(
                    input,
                    REDACTOR_LINE_SIZE) == EDITOR_INPUT_F5)
            {
                editor_compile(filename);
                continue;
            }
        }
        else
        {
            print_int(editor_lines + 1);
            print(" | ");

            if (editor_read_line(
                    input,
                    REDACTOR_LINE_SIZE) == EDITOR_INPUT_F5)
            {
                editor_compile(filename);
                continue;
            }
        }

        if (strcmp(input, "sys/exit") == 0)
        {
            clear();

            print(
                "Editor closed without saving.\n"
            );

            return;
        }

        if (strcmp(input, "sys/exit/save") == 0)
        {
            if (editor_save(filename))
            {
                clear();

                print(
                    "File saved.\n"
                );

                print(
                    "Editor closed.\n"
                );

                return;
            }

            continue;
        }

        if (starts_with(input, "sys/edit "))
        {
            int line_number =
                atoi_simple(input + 9);

            if (line_number < 1 ||
                line_number > editor_lines)
            {
                print_error(
                    "\nInvalid line number.\n"
                );

                print(
                    "Press enter to continue."
                );

                {
                    char dummy[4];
                    read_line(dummy, 4);
                }

                continue;
            }

            print("\nCurrent line ");
            print_int(line_number);
            print(": ");
            print(editor_buffer[line_number - 1]);
            putchar_os('\n');

            print("New content: ");

            {
                char edit_input[REDACTOR_LINE_SIZE];

                if (editor_read_line(
                        edit_input,
                        REDACTOR_LINE_SIZE) ==
                    EDITOR_INPUT_F5)
                {
                    editor_compile(filename);
                    continue;
                }

                {
                    int i = 0;

                    while (
                        edit_input[i] != '\0' &&
                        i < REDACTOR_LINE_SIZE - 1
                    )
                    {
                        editor_buffer[line_number - 1][i] =
                            edit_input[i];

                        i++;
                    }

                    editor_buffer[line_number - 1][i] =
                        '\0';
                }
            }

            continue;
        }

        if (editor_lines >= REDACTOR_MAX_LINES)
            continue;

        {
            int i = 0;

            while (
                input[i] != '\0' &&
                i < REDACTOR_LINE_SIZE - 1
            )
            {
                editor_buffer[editor_lines][i] =
                    input[i];

                i++;
            }

            editor_buffer[editor_lines][i] =
                '\0';

            editor_lines++;
        }
    }
}
