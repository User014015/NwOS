#include "slot.h"
#include "../../kernel.h"
void game_slots(void)
{
    char input[16];
    int balance = 100;
    const char symbols[] = {'7', '$', '*', '#', '@', '&'};
    int num_symbols = 6;

    print_success("\n=== SLOT MACHINE ===\n");
    print("Starting balance: 100 coins.\n");
    print("Press Enter to spin (cost 10 coins), or type 'quit' to exit.\n\n");

    while (balance > 0)
    {
        print("Balance: ");
        print_int(balance);
        print(" | Action: ");
        read_line(input, 16);

        if (strcmp(input, "quit") == 0)
        {
            print("Game over! Final balance: ");
            print_int(balance);
            print("\n");
            return;
        }

        if (balance < 10)
        {
            print_error("Not enough coins to spin!\n");
            break;
        }

        balance -= 10;

        int s1 = random_range(0, num_symbols - 1);
        int s2 = random_range(0, num_symbols - 1);
        int s3 = random_range(0, num_symbols - 1);

        print("\n  [ ");
        putchar_os(symbols[s1]);
        print(" | ");
        putchar_os(symbols[s2]);
        print(" | ");
        putchar_os(symbols[s3]);
        print(" ]  ");

        if (s1 == s2 && s2 == s3)
        {
            print_success("JACKPOT! You won 100 coins!\n\n");
            balance += 100;
        }
        else if (s1 == s2 || s2 == s3 || s1 == s3)
        {
            print_success("Match 2! You won 25 coins!\n\n");
            balance += 25;
        }
        else
        {
            print_error("No match. Try again!\n\n");
        }
    }

    if (balance <= 0)
    {
        print_error("You ran out of coins!\n");
    }
}