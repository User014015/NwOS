#include "textredactor.h"
#include "../../kernel.h"

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

void fs_create(const char* name);
void fs_write(const char* name, const char* text);

#define REDACTOR_MAX_LINES 32
#define REDACTOR_LINE_SIZE 121


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
    char file_data[512];

    int i = 0;
    int line = 0;
    int column = 0;

    editor_clear_buffer();
    if (!fs_read_text(
            filename,
            file_data,
            sizeof(file_data)))
    {
        return;
    }
    while (
        file_data[i] != '\0' &&
        line < REDACTOR_MAX_LINES
    )
    {
        column = 0;

        while (
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

    editor_lines = line;
}


static void editor_show(const char* filename)
{
    int i;

    clear();

    print("================================\n");
    print("          NwOS EDITOR\n");
    print("================================\n");
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
    print("sys/exit       - exit without saving\n");
    print("sys/exit/save  - save and exit\n");
    print("sys/edit <N>   - edit an existing line\n");
    print("--------------------------------\n");
}
void putchar_os(char c);
static int editor_build_text(
    char* output,
    unsigned int max_size)
{
    unsigned int position = 0;
    int i;
    int j;

    if (max_size == 0)
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

            output[position] =
                editor_buffer[i][j];

            position++;
            j++;
        }
        if (i < editor_lines - 1)
        {
            if (position >= max_size - 1)
            {
                output[position] = '\0';
                return 0;
            }

            output[position] = '\n';
            position++;
        }
    }

    output[position] = '\0';

    return 1;
}

static int editor_save(const char* filename)
{
    char output[512];

    if (!editor_build_text(
            output,
            sizeof(output)))
    {
        print_error(
            "Editor: file is larger than "
            "511 bytes.\n"
        );

        return 0;
    }

    if (fs_find(filename) == -1)
    {
        fs_create(filename);
    }

    fs_write(filename, output);

    return 1;
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
                "Use sys/exit, sys/exit/save, or sys/edit <N>.\n"
            );

            print("> ");

            read_line(
                input,
                REDACTOR_LINE_SIZE
            );
        }
        else
        {
            print_int(editor_lines + 1);
            print(" | ");

            read_line(
                input,
                REDACTOR_LINE_SIZE
            );
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
            int line_number = atoi_simple(input + 9);

            if (line_number < 1 || line_number > editor_lines)
            {
                print_error(
                    "\nInvalid line number.\n"
                );

                print(
                    "Press enter to continue."
                );

                char dummy[4];
                read_line(dummy, 4);

                continue;
            }

            print("\nCurrent line ");
            print_int(line_number);
            print(": ");
            print(editor_buffer[line_number - 1]);
            putchar_os('\n');

            print("New content: ");

            char edit_input[REDACTOR_LINE_SIZE];

            read_line(
                edit_input,
                REDACTOR_LINE_SIZE
            );

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

            continue;
        }

        if (editor_lines >= REDACTOR_MAX_LINES)
        {
            continue;
        }


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