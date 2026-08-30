#include "drivers/keyboard.h"

/* Prototypes functions */

void putchar_os(char c);

void print(const char* text);

void clear(void);

void read_line(char* buffer, int max);

unsigned int rand_simple(void);

int random_range(int min, int max);

void print_int(int number);
void game_guess(void);
void game_rps(void);
void game_word(void);

typedef unsigned short uint16_t;

#define WIDTH 80
#define HEIGHT 25

#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA      5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GRAY   7
#define COLOR_DARK_GRAY    8
#define COLOR_LIGHT_BLUE   9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN  11
#define COLOR_LIGHT_RED   12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW      14
#define COLOR_WHITE       15

static unsigned char text_color = COLOR_LIGHT_GRAY;

void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);

unsigned char rtc_read(unsigned char reg);
unsigned char bcd_to_bin(unsigned char value);

void get_time(int* hour, int* minute, int* second);
void get_date(int* day, int* month, int* year);

void print_two_digits(int value);

/* =========================
   VGA
   ========================= */

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;


/* =========================
   Terminal
   ========================= */

static int cursor_x = 0;
static int cursor_y = 0;

#define COMMAND_HISTORY_SIZE 16
#define COMMAND_SIZE 128

static char command_history[COMMAND_HISTORY_SIZE][COMMAND_SIZE];

static int history_count = 0;
static int history_position = 0;

void scroll(void)
{
    for (int y = 1; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            video[(y - 1) * WIDTH + x] =
                video[y * WIDTH + x];
        }
    }

    /*
     * Clear last string.
     */
    for (int x = 0; x < WIDTH; x++)
    {
        video[(HEIGHT - 1) * WIDTH + x] =
            (uint16_t)' ' | 0x0F00;
    }

    cursor_y = HEIGHT - 1;
}

void save_command(const char* command)
{
    if (command[0] == '\0')
        return;

    if (history_count >= COMMAND_HISTORY_SIZE)
    {
        for (int i = 1; i < COMMAND_HISTORY_SIZE; i++)
        {
            for (int j = 0; j < COMMAND_SIZE; j++)
            {
                command_history[i - 1][j] =
                    command_history[i][j];
            }
        }

        history_count = COMMAND_HISTORY_SIZE - 1;
    }

    /*
     * Copy command.
     */
    int i = 0;

    while (command[i] != '\0' &&
           i < COMMAND_SIZE - 1)
    {
        command_history[history_count][i] =
            command[i];

        i++;
    }

    command_history[history_count][i] = '\0';

    history_count++;

    history_position = history_count;
}


/*
 * Clear screen.
 */
void clear(void)
{
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            video[y * WIDTH + x] =
                (uint16_t)' ' |
                ((uint16_t)COLOR_LIGHT_GRAY << 8);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void clear_input(int length)
{
    while (length > 0)
    {
        putchar_os('\b');
        length--;
    }
}


/*
 * Typing one char
 */
void putchar_os(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;

        if (cursor_y >= HEIGHT)
            scroll();

        return;
    }

    if (c == '\b')
    {
        if (cursor_x > 0)
        {
            cursor_x--;

            video[cursor_y * WIDTH + cursor_x] =
                (uint16_t)' ' |
                ((uint16_t)text_color << 8);
        }

        return;
    }

    video[cursor_y * WIDTH + cursor_x] =
        (uint16_t)c |
        ((uint16_t)text_color << 8);

    cursor_x++;

    if (cursor_x >= WIDTH)
    {
        cursor_x = 0;
        cursor_y++;

        if (cursor_y >= HEIGHT)
            scroll();
    }
}


/*
 * Typing text.
 */
void print(const char* text)
{
    while (*text)
    {
        putchar_os(*text);
        text++;
    }
}

void set_color(unsigned char color)
{
    text_color = color;
}

/* =========================
   Strings
   ========================= */

int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }

    return *a - *b;
}


int starts_with(const char* text, const char* prefix)
{
    while (*prefix)
    {
        if (*text != *prefix)
            return 0;

        text++;
        prefix++;
    }

    return 1;
}


int atoi_simple(const char* text)
{
    int value = 0;

    while (*text >= '0' && *text <= '9')
    {
        value = value * 10 + (*text - '0');
        text++;
    }

    return value;
}


/* =========================
   Input
   ========================= */

void read_line(char* buffer, int max)
{
    int length = 0;

    history_position = history_count;

    while (1)
    {
        int key = keyboard_getkey();

        if (key == KEY_ENTER)
        {
            buffer[length] = '\0';

            save_command(buffer);

            putchar_os('\n');

            return;
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

        /*
         * UP
         */
        if (key == KEY_UP)
        {
            if (history_count > 0 &&
                history_position > 0)
            {
                history_position--;

                clear_input(length);

                length = 0;

                while (length < COMMAND_SIZE &&
                    command_history[history_position][length] != '\0' &&
                    length < max - 1)
                {
                    buffer[length] =
                        command_history[history_position][length];

                    putchar_os(buffer[length]);

                    length++;
                }

                buffer[length] = '\0';
            }

            continue;
        }

        /*
         * DOWN
         */
        if (key == KEY_DOWN)
        {
            if (history_position < history_count)
            {
                history_position++;

                clear_input(length);

                length = 0;

                if (history_position < history_count)
                {
                    while (command_history[history_position][length] != '\0' &&
                           length < max - 1)
                    {
                        buffer[length] =
                            command_history[history_position][length];

                        putchar_os(buffer[length]);

                        length++;
                    }
                }

                buffer[length] = '\0';
            }

            continue;
        }

        /*
         * Symbol
         */
        if (key > 0 && key < 256)
        {
            if (length < max - 1)
            {
                buffer[length] = (char)key;

                length++;

                putchar_os((char)key);
            }
        }
    }
}

void print_error(const char* text)
{
    set_color(COLOR_LIGHT_RED);
    print(text);
    set_color(COLOR_LIGHT_GRAY);
}

void print_success(const char* text)
{
    set_color(COLOR_LIGHT_GREEN);
    print(text);
    set_color(COLOR_LIGHT_GRAY);
}

void print_title(const char* text)
{
    set_color(COLOR_YELLOW);
    print(text);
    set_color(COLOR_LIGHT_GRAY);
}


/* =========================
   Game
   ========================= */

static unsigned int random_seed = 123456789;

int random_range(int min, int max)
{
    return min +
        (rand_simple() % (max - min + 1));
}

unsigned int rand_simple(void)
{
    random_seed =
        random_seed * 1103515245 + 12345;

    return random_seed;
}

void random_init(void)
{
    unsigned int seed;

    __asm__ volatile (
        "rdtsc"
        : "=a"(seed)
        :
        : "edx"
    );

    random_seed ^= seed;
}

void game_guess(void)
{
    char buffer[32];

    int secret = random_range(1, 100);

    print("\n=== GUESS THE NUMBER ===\n");
    print("I chose a number from 1 to 100.\n");
    print_error("Type quit to exit.\n\n");

    while (1)
    {
        print("Your guess: ");

        read_line(buffer, 32);

        if (strcmp(buffer, "quit") == 0)
        {
            print("You left the game.\n");
            return;
        }

        int number = atoi_simple(buffer);

        if (number == secret)
        {
            print_success("Correct! You win!\n");
            return;
        }

        if (number < secret)
            print("Too small!\n");
        else
            print("Too big!\n");
    }
}

void game_rps(void)
{
    char choice[32];

    int player_score = 0;
    int computer_score = 0;

    print_title("\n=== ROCK PAPER SCISSORS ===\n");
    print("First to 3 wins!\n");
    print("Type rock, paper or scissors.\n");
    print("Type quit to exit.\n\n");

    while (player_score < 3 &&
           computer_score < 3)
    {
        print("Your choice: ");
        read_line(choice, 32);

        if (strcmp(choice, "quit") == 0)
            return;

        int computer = random_range(0, 2);

        print("Computer: ");

        if (computer == 0)
            print("rock\n");
        else if (computer == 1)
            print("paper\n");
        else
            print("scissors\n");


        if (strcmp(choice, "rock") == 0)
        {
            if (computer == 0)
            {
                print("Draw!\n");
            }
            else if (computer == 1)
            {
                print_error("You lose!\n");
                computer_score++;
            }
            else
            {
                print_success("You win!\n");
                player_score++;
            }
        }

        else if (strcmp(choice, "paper") == 0)
        {
            if (computer == 0)
            {
                print_success("You win!\n");
                player_score++;
            }
            else if (computer == 1)
            {
                print("Draw!\n");
            }
            else
            {
                print_error("You lose!\n");
                computer_score++;
            }
        }

        else if (strcmp(choice, "scissors") == 0)
        {
            if (computer == 0)
            {
                print_error("You lose!\n");
                computer_score++;
            }
            else if (computer == 1)
            {
                print_success("You win!\n");
                player_score++;
            }
            else
            {
                print("Draw!\n");
            }
        }

        else
        {
            print_error("Unknown choice!\n");
        }

        print("Score: ");
        print_int(player_score);

        print(" - ");

        print_int(computer_score);

        print("\n\n");
    }

    if (player_score == 3)
        print_success("You won the match!\n");
    else
        print_error("Computer won the match!\n");
}


void games(void)
{
    print("\nGames:\n");

    print("  guess - Guess the Number\n");
    print("  rps   - Rock Paper Scissors\n");
    print("  word - Guess word\n");

    print("\n");
}

void testH(void)
{
    print("Hello!\n");
}

int read_int(void)
{
    char buffer[32];

    read_line(buffer, 32);

    return atoi_simple(buffer);
}

void print_int(int number)
{
    char buffer[12];
    int i = 0;

    if (number == 0)
    {
        putchar_os('0');
        return;
    }

    if (number < 0)
    {
        putchar_os('-');
        number = -number;
    }

    while (number > 0)
    {
        buffer[i] = '0' + (number % 10);
        number /= 10;
        i++;
    }

    while (i > 0)
    {
        i--;
        putchar_os(buffer[i]);
    }
}

void Calc(void)
{
    int a;
    int b;
    char operation[4];

    set_color(COLOR_GREEN);

    print("\n=== Calculator ===\n");

    set_color(COLOR_LIGHT_GRAY);

    print("First number: ");
    a = read_int();

    print("Operation (+ - * /): ");
    read_line(operation, 4);

    print("Second number: ");
    b = read_int();

    print_success("Result: ");

    if (operation[0] == '+')
    {
        print_int(a + b);
    }
    else if (operation[0] == '-')
    {
        print_int(a - b);
    }
    else if (operation[0] == '*')
    {
        print_int(a * b);
    }
    else if (operation[0] == '/')
    {
        if (b == 0)
        {
            print_error("Error: division by zero!");
            putchar_os('\n');
            return;
        }

        print_int(a / b);
    }
    else
    {
        print_error("Unknown operation!");
    }

    putchar_os('\n');
}

void dateCr()
{
    set_color(COLOR_WHITE);
    print("2026.08.27");
    print("v.1.2");
    set_color(COLOR_LIGHT_GRAY);
}

// guess lang (not random)

void game_word(void)
{
    char answer[32];

    print_title("\n=== GUESS THE WORD ===\n");
    print("Hint: programming language.\n");
    print("You have 3 attempts.\n\n");

    for (int attempt = 1; attempt <= 3; attempt++)
    {
        print("Answer: ");
        read_line(answer, 32);

        if (strcmp(answer, "python") == 0)
        {
            print_success("Correct! You win!\n");
            return;
        }

        print_error("Wrong answer!\n");
    }

    print("The answer was: c\n");
}

void nwfetch(void)
{
    print_title("\n");

    print("       __  __       ");
    print("   NwOS\n");

    print("      |  \\|  |      ");
    print("   Version: 1.3\n");

    print("      | |\\| |      ");
    print("   Arch: x86\n");

    print("      | |\\| |      ");
    print("   Kernel: C\n");

    print("      |_|  |_|      ");
    print("   Bootloader: NASM\n");

    print("\n");

    print("  Memory model: 32-bit\n");
    print("  Display: VGA text mode\n");
    print("  Keyboard: PS/2\n");
    print("\n");
}

/* =========================
   Shell
   ========================= */

void shell(void)
{
    char command[128];

    while (1)
    {
        set_color(COLOR_CYAN);
        print("NwOS> ");
        set_color(COLOR_LIGHT_GRAY);

        read_line(command, 128);

        if (strcmp(command, "help") == 0)
        {
            set_color(COLOR_GREEN);
            print("Commands:\n");
            print("  help   - show commands\n");
            print("  echo   - print text\n");
            print("  clear  - clear screen\n");
            print("  about  - OS information\n");
            print("  nwfetch - system info\n");
            print("  games  - list games\n");
            print("  calc   - Calculator\n");
            set_color(COLOR_LIGHT_GRAY);
        }

        else if (strcmp(command, "clear") == 0)
        {
            clear();
        }

        else if (strcmp(command, "about") == 0)
        {
            set_color(COLOR_WHITE);
            print("====NwOS 1.3====\n");
            print("Name: NwOS\n");
            print("Version: v1.3\n");
            print("Arch: x86\n");
            print("Display: VGA text mode\n");
            print("PS/2 keyboard\n");
            print("================\n");
            set_color(COLOR_LIGHT_GRAY);
        }

        else if (starts_with(command, "echo "))
        {
            print(command + 5);
            putchar_os('\n');
        }

        else if (strcmp(command, "games") == 0)
        {
            games();
        }
        
        else if (strcmp(command, "nwfetch") == 0)
        {
            nwfetch();
        }

        else if (strcmp(command, "date?") == 0)
        {
            dateCr();
        }

        else if (strcmp(command, "testH") == 0)
        {
            testH();
        }
        else if (strcmp(command, "calc") == 0)
        {
            Calc();
        }
        else if (strcmp(command, "word") == 0)
        {
            game_word();
        }

        else if (strcmp(command, "guess") == 0)
        {
            game_guess();
        }
        else if (strcmp(command, "rps") == 0)
        {
            game_rps();
        }

        else if (command[0] == '\0')
        {
        }

        else
        {
            print_error("Unknown command. Type 'help'.\n");
        }
    }
}


/* =========================
   Kernel
   ========================= */

void kernel_main(void)
{
    clear();

    random_init();

    print("================================\n");
    print("        Welcome to NwOS 1.3\n");
    print("================================\n");
    print("Keyboard: OK\n");
    print("Type 'help' for commands.\n\n");
    set_color(COLOR_GREEN);
    print("==================");
    set_color(COLOR_BLUE);
    print("==================\n");
    set_color(COLOR_LIGHT_GRAY);

    shell();

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
