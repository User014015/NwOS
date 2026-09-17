#include "baseconverter.h"
#include "../../kernel.h"
void app_base_converter(void)
{
    char input[32];

    print_success("\n=== BASE CONVERTER ===\n");
    print("Enter decimal number: ");
    read_line(input, 32);

    if (input[0] == '\0')
    {
        print_error("Invalid input.\n");
        return;
    }

    int num = atoi_simple(input);

    if (num < 0)
    {
        print_error("Negative numbers are not supported.\n");
        return;
    }

    print("\nDecimal: ");
    print_int(num);
    print("\nBinary:  0b");
    if (num == 0)
    {
        putchar_os('0');
    }
    else
    {
        int started = 0;
        for (int i = 31; i >= 0; i--)
        {
            int bit = (num >> i) & 1;
            if (bit) started = 1;
            if (started)
            {
                putchar_os(bit ? '1' : '0');
            }
        }
    }
    print("\nHex:     0x");
    if (num == 0)
    {
        putchar_os('0');
    }
    else
    {
        const char hex_chars[] = "0123456789ABCDEF";
        char hex_buf[9];
        int pos = 0;
        unsigned int temp = (unsigned int)num;

        while (temp > 0)
        {
            hex_buf[pos++] = hex_chars[temp % 16];
            temp /= 16;
        }

        for (int i = pos - 1; i >= 0; i--)
        {
            putchar_os(hex_buf[i]);
        }
    }

    print("\n\n");
}