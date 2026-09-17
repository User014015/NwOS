#include "filestats.h"
#include "../../kernel.h"
void app_file_stats(void)
{
    char filename[32];
    static char buffer[512];

    print_success("\n=== FILE STATS ANALYZER ===\n");
    print("Enter filename: ");
    read_line(filename, 32);

    if (filename[0] == '\0')
    {
        print_error("Filename cannot be empty.\n");
        return;
    }

    if (!fs_read_text(filename, buffer, sizeof(buffer)))
    {
        print_error("Failed to read file or file does not exist.\n");
        return;
    }

    int chars = 0;
    int words = 0;
    int lines = 0;
    int in_word = 0;

    for (int i = 0; buffer[i] != '\0'; i++)
    {
        chars++;

        if (buffer[i] == '\n')
        {
            lines++;
        }

        if (buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t' || buffer[i] == '\r')
        {
            in_word = 0;
        }
        else if (!in_word)
        {
            in_word = 1;
            words++;
        }
    }

    if (chars > 0 && buffer[chars - 1] != '\n')
    {
        lines++;
    }

    print_success("\n--- File Statistics ---\n");
    print("File: ");
    print(filename);
    print("\nCharacters: ");
    print_int(chars);
    print("\nWords: ");
    print_int(words);
    print("\nLines: ");
    print_int(lines);
    print("\n-----------------------\n\n");
}