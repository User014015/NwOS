#include "game_memory.h"
#include "../../kernel.h"
void game_memory(void)
{
    char input[32];
    int score = 0;
    int min_val = 10;
    int max_val = 99;

    print_success("\n=== MEMORY TEST ===\n");
    print("Memorize the number shown on screen.\n");
    print("Type 'quit' when entering answer to exit.\n\n");

    while (1)
    {
        int target = random_range(min_val, max_val);

        print("Level ");
        print_int(score + 1);
        print(" - Remember this number: ");
        print_int(target);
        print("\nPress Enter when ready...");
        read_line(input, 32);

        clear();

        print("Enter the number: ");
        read_line(input, 32);

        if (strcmp(input, "quit") == 0)
        {
            break;
        }

        int guess = atoi_simple(input);
        if (guess == target)
        {
            score++;
            print_success("Correct!\n\n");
            if (max_val < 100000000)
            {
                min_val *= 10;
                max_val = max_val * 10 + 9;
            }
        }
        else
        {
            print_error("Wrong! The correct number was: ");
            print_int(target);
            print("\n");
            break;
        }
    }

    print("Game over! Total score: ");
    print_int(score);
    print("\n\n");
}