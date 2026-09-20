#include "../../drivers/keyboard.h"
#include "../../drivers/disk.h"
#include "../../include/kernelpanic.h"

#include "Apps/wordgenerator/wordgenerator.h"
#include "Apps/textredactor/textredactor.h"
#include "nwoloader/nwo_vm.h"
#include "nwoloader/nwo_runner.h"

#include "Apps/Games/slot.h"
#include "Apps/Games/game_memory.h"
#include "kernel.h"

#include "Apps/fileStats/filestats.h"
#include "Apps/baseconverter/baseconverter.h"

#include "nwo_loader.h"

/*
* MADE BY: User014015
* Created: 28 july 2026 (v.0.1)
* Name: NwOS
* Github: https://github.com/User014015/NwOS
* License MIT: (github file: LICENSE)
*/

void putchar_os(char c);

void print(const char* text);

void clear(void);

void nwo_run(const char* filename);

void read_line(char* buffer, int max);

unsigned int rand_simple(void);

void fs_load(void);
void fs_save_directory(void);
int random_range(int min, int max);
void print_error(const char* text);
void print_success(const char* text);

void *memcpy(void *dest, const void *src, unsigned int n);

void app_nwo_info(const char* filename);
void app_hexview(const char* filename);
static void print_hex_byte(unsigned char b);

void print_int(int number);

void game_guess(void);
void game_rps(void);
void game_word(void);
void game_coin(void);
void game_dice(void);
void game_higher_lower(void);
void game_math(void);
void game_hangman(void);
void game_tictactoe(void);

void game_slots(void);
void game_memory(void);

void app_base_converter(void);
void app_file_stats(void);

void generate_num(void);

void games_menu(void);
int strncmp(const char* a, const char* b, int n);
void nwo_run(const char* filename);

void redactor(const char* filename);
int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
);
int fs_find(const char* name);

int fs_read_binary(
    const char* name,
    unsigned char* buffer,
    unsigned int max_size);

void reboot(void);

typedef unsigned short uint16_t;

#define WIDTH 80
#define HEIGHT 50

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 512

#define FS_DIR_LBA     200
#define FS_DIR_SECTORS 2
#define FS_DATA_LBA    (FS_DIR_LBA + FS_DIR_SECTORS)

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
static unsigned char base_color = COLOR_LIGHT_GRAY;

void fs_load(void);
void fs_save_directory(void);

unsigned char rtc_read(unsigned char reg);
unsigned char bcd_to_bin(unsigned char value);

void get_time(int* hour, int* minute, int* second);
void get_date(int* day, int* month, int* year);

void print_two_digits(int value);

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

#define SCROLLBACK_LINES 300

static uint16_t line_buffer[SCROLLBACK_LINES][WIDTH];

static int line_start = 0;
static int line_count = 0;

static int cursor_col = 0;
static int view_offset = 0;  
void terminal_new_line(void)
{
    int idx;

    if (line_count < SCROLLBACK_LINES)
    {
        idx = (line_start + line_count) % SCROLLBACK_LINES;
        line_count++;
    }
    else
    {
        idx = line_start;
        line_start = (line_start + 1) % SCROLLBACK_LINES;
    }

    for (int x = 0; x < WIDTH; x++)
    {
        line_buffer[idx][x] =
            (uint16_t)' ' | ((uint16_t)COLOR_LIGHT_GRAY << 8);
    }

    cursor_col = 0;
}

void *memcpy(void *dest, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (unsigned int i = 0; i < n; i++)
    {
        d[i] = s[i];
    }

    return dest;
}
void terminal_set_cell(int col, char c)
{
    int idx = (line_start + line_count - 1) % SCROLLBACK_LINES;

    line_buffer[idx][col] =
        (uint16_t)c | ((uint16_t)text_color << 8);
}
void render_view(void)
{
    int shown = 0;
    int pad = HEIGHT;
    int first = 0;

    if (line_count > 0)
    {
        int last = (line_count - 1) - view_offset;

        if (last < 0)
            last = 0;

        first = last - (HEIGHT - 1);

        if (first < 0)
            first = 0;

        shown = last - first + 1;
        pad = HEIGHT - shown;
    }

    for (int row = 0; row < pad; row++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            video[row * WIDTH + x] =
                (uint16_t)' ' | ((uint16_t)COLOR_LIGHT_GRAY << 8);
        }
    }

    for (int i = 0; i < shown; i++)
    {
        int idx = (line_start + first + i) % SCROLLBACK_LINES;

        for (int x = 0; x < WIDTH; x++)
        {
            video[(pad + i) * WIDTH + x] = line_buffer[idx][x];
        }
    }
}
void clear(void)
{
    line_start = 0;
    line_count = 0;
    view_offset = 0;

    terminal_new_line();
    render_view();
}
void scroll_view_up(void)
{
    int max_offset = line_count > HEIGHT ? line_count - HEIGHT : 0;

    if (view_offset < max_offset)
    {
        view_offset++;
        render_view();
    }
}
void scroll_view_down(void)
{
    if (view_offset > 0)
    {
        view_offset--;
        render_view();
    }
}
void putchar_os(char c)
{
    view_offset = 0;

    if (c == '\n')
    {
        terminal_new_line();
        render_view();
        return;
    }

    if (c == '\b')
    {
        if (cursor_col > 0)
        {
            cursor_col--;
            terminal_set_cell(cursor_col, ' ');
        }

        render_view();
        return;
    }

    terminal_set_cell(cursor_col, c);
    cursor_col++;

    if (cursor_col >= WIDTH)
    {
        terminal_new_line();
    }

    render_view();
}
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
void set_base_color(unsigned char color)
{
    base_color = color;
    text_color = color;
}
int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }

    return *a - *b;
}

int strncmp(const char* a, const char* b, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (a[i] != b[i])
        {
            return (unsigned char)a[i] -
                   (unsigned char)b[i];
        }

        if (a[i] == '\0')
        {
            return 0;
        }
    }

    return 0;
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

void read_line(char* buffer, int max)
{
    int length = 0;

    while (1)
    {
        int key = keyboard_getkey();

        if (key == KEY_TAB)
        {
            for (int i = 0; i < 4; i++)
            {
                if (length >= max - 1)
                    break;

                buffer[length++] = ' ';
                putchar_os(' ');
            }

            continue;
        }

        if (key == KEY_ENTER)
        {
            buffer[length] = '\0';

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
        if (key == KEY_UP)
        {
            scroll_view_up();
            continue;
        }

        if (key == KEY_DOWN)
        {
            scroll_view_down();
            continue;
        }
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
    set_color(base_color);
}

void print_success(const char* text)
{
    set_color(COLOR_LIGHT_GREEN);
    print(text);
    set_color(base_color);
}

void print_title(const char* text)
{
    set_color(COLOR_YELLOW);
    print(text);
    set_color(base_color);
}

void delay(unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        __asm__ volatile ("nop");
    }
}

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
        print("your guess: ");

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
        {
            print("Too small!\n");
        }
        else {
            print("Too big!\n");
        }
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

void game_guessh(void)
{
    char bufferh[32];

    int secreth = random_range(1, 250);

    print("\n=== GUESS THE NUMBER HARD===\n");
    print("I chose a number from 1 to 250.\n");
    print_error("Type quit to exit.\n\n");

    while (1)
    {
        print("your guess: ");

        read_line(bufferh, 32);

        if (strcmp(bufferh, "quit") == 0)
        {
            print("You left the game.\n");
            return;
        }

        int number = atoi_simple(bufferh);

        if (number == secreth)
        {
            print_success("Correct! You win!\n");
            return;
        }

        if (number < secreth)
            print("Too small!\n");
        else
            print("Too big!\n");
    }
}

void game_coin(void)
{
    char input[16];
    int result;

    print_title("\n=== COIN FLIP ===\n\n");

    print("Choose:\n");
    print("1 - Heads\n");
    print("2 - Tails\n\n");

    print("Your choice: ");
    read_line(input, 16);

    if (strcmp(input, "1") != 0 &&
        strcmp(input, "2") != 0)
    {
        print_error("Invalid choice.\n");
        return;
    }

    result = rand_simple() % 2;

    print("\nCoin: ");

    if (result == 0)
        print("Heads\n");
    else
        print("Tails\n");

    if ((result == 0 && input[0] == '1') ||
        (result == 1 && input[0] == '2'))
    {
        print_success("You win!\n");
    }
    else
    {
        print_error("You lose!\n");
    }

    putchar_os('\n');
}

void game_dice(void)
{
    int dice;

    print_title("\n=== DICE ===\n\n");

    dice = (rand_simple() % 6) + 1;

    print_success("You rolled: ");
    print_int(dice);
    putchar_os('\n');

    putchar_os('\n');
}

void game_higher_lower(void)
{
    char input[16];

    int current;
    int next;

    int score = 0;

    print_title("\n=== HIGHER OR LOWER ===\n\n");

    current = (rand_simple() % 100) + 1;

    while (1)
    {
        print("Current number: ");
        print_int(current);
        putchar_os('\n');

        print("Will the next number be higher or lower?\n");
        print("1 - Higher\n");
        print("2 - Lower\n");
        print("0 - Quit\n\n");

        print("Your choice: ");
        read_line(input, 16);

        if (strcmp(input, "0") == 0)
            break;

        if (strcmp(input, "1") != 0 &&
            strcmp(input, "2") != 0)
        {
            print_error("Invalid choice.\n\n");
            continue;
        }

        next = (rand_simple() % 100) + 1;

        print("Next number: ");
        print_int(next);
        putchar_os('\n');

        if (next == current)
        {
            print("Same number! No points.\n\n");
            continue;
        }

        if ((input[0] == '1' && next > current) ||
            (input[0] == '2' && next < current))
        {
            score++;

            print_success("Correct!\n");
        }
        else
        {
            print_error("Wrong!\n");
        }

        print("Score: ");
        print_int(score);
        putchar_os('\n');
        putchar_os('\n');

        current = next;
    }

    print("Final score: ");
    print_int(score);
    putchar_os('\n');
}

void game_math(void)
{
    char input[32];

    int a;
    int b;
    int operation;
    int answer;
    int user_answer;

    int score = 0;

    print_title("\n=== MATH QUIZ ===\n\n");

    for (int round = 0; round < 5; round++)
    {
        a = (rand_simple() % 20) + 1;
        b = (rand_simple() % 20) + 1;

        operation = rand_simple() % 2;

        print("Question ");
        print_int(round + 1);
        print(": ");

        if (operation == 0)
        {
            answer = a + b;

            print_int(a);
            print(" + ");
            print_int(b);
        }
        else
        {
            if (a < b)
            {
                int temp = a;
                a = b;
                b = temp;
            }

            answer = a - b;

            print_int(a);
            print(" - ");
            print_int(b);
        }

        print(" = ");

        read_line(input, 32);

        user_answer = atoi_simple(input);

        if (user_answer == answer)
        {
            print_success("Correct!\n");
            score++;
        }
        else
        {
            print_error("Wrong! Answer: ");
            print_int(answer);
            putchar_os('\n');
        }
    }

    print("\nScore: ");
    print_int(score);
    print("/5\n\n");
}


char to_upper_letter(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return (char)(c - 'a' + 'A');
    }

    return c;
}

void game_hangman(void)
{
    static const char* words[] =
    {
        "KERNEL", "MEMORY", "POINTER", "COMPILE",
        "BINARY", "ARRAY", "LINKER", "BOOTLOADER"
    };

    const int word_count = 8;

    const char* word = words[random_range(0, word_count - 1)];

    int length = 0;

    while (word[length] != '\0')
    {
        length++;
    }

    char revealed[32];

    for (int i = 0; i < length; i++)
    {
        revealed[i] = '_';
    }

    revealed[length] = '\0';

    char guessed[26];

    for (int i = 0; i < 26; i++)
    {
        guessed[i] = 0;
    }

    int lives = 6;
    int found = 0;

    print_title("\n=== HANGMAN ===\n");
    print("Guess the word, one letter at a time.\n");
    print_error("Type quit to exit.\n\n");

    while (lives > 0 && found < length)
    {
        print("Word: ");
        print(revealed);
        print("\nLives: ");
        print_int(lives);
        print("\n\nGuess a letter: ");

        char input[8];

        read_line(input, 8);

        if (strcmp(input, "quit") == 0)
        {
            print("You left the game. The word was: ");
            print(word);
            putchar_os('\n');
            return;
        }

        char c = to_upper_letter(input[0]);

        if (c < 'A' || c > 'Z')
        {
            print_error("Please enter a single letter.\n\n");
            continue;
        }

        int idx = c - 'A';

        if (guessed[idx])
        {
            print_error("Already guessed that letter.\n\n");
            continue;
        }

        guessed[idx] = 1;

        int hit = 0;

        for (int i = 0; i < length; i++)
        {
            if (word[i] == c)
            {
                revealed[i] = c;
                hit = 1;
                found++;
            }
        }

        if (hit)
        {
            print_success("Good guess!\n\n");
        }
        else
        {
            lives--;
            print_error("Wrong!\n\n");
        }
    }

    if (found == length)
    {
        print_success("You win! The word was: ");
        print(word);
        putchar_os('\n');
    }
    else
    {
        print_error("You lose! The word was: ");
        print(word);
        putchar_os('\n');
    }
}


void ttt_print_board(char board[9])
{
    print("\n");

    for (int row = 0; row < 3; row++)
    {
        print(" ");
        putchar_os(board[row * 3]);
        print(" | ");
        putchar_os(board[row * 3 + 1]);
        print(" | ");
        putchar_os(board[row * 3 + 2]);
        print("\n");

        if (row < 2)
        {
            print("---+---+---\n");
        }
    }

    print("\n");
}

int ttt_check_win(char board[9], char player)
{
    int wins[8][3] =
    {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
        {0, 4, 8}, {2, 4, 6}
    };

    for (int i = 0; i < 8; i++)
    {
        if (board[wins[i][0]] == player &&
            board[wins[i][1]] == player &&
            board[wins[i][2]] == player)
        {
            return 1;
        }
    }

    return 0;
}

int ttt_board_full(char board[9])
{
    for (int i = 0; i < 9; i++)
    {
        if (board[i] != 'X' && board[i] != 'O')
        {
            return 0;
        }
    }

    return 1;
}

void game_tictactoe(void)
{
    char board[9];

    for (int i = 0; i < 9; i++)
    {
        board[i] = (char)('1' + i);
    }

    print_title("\n=== TIC-TAC-TOE ===\n");
    print("You are X, computer is O.\n");
    print("Enter a number 1-9 to place your mark.\n");

    while (1)
    {
        ttt_print_board(board);

        char input[8];

        print("Your move: ");
        read_line(input, 8);

        int move = atoi_simple(input);

        if (move < 1 || move > 9 ||
            board[move - 1] == 'X' || board[move - 1] == 'O')
        {
            print_error("Invalid move.\n");
            continue;
        }

        board[move - 1] = 'X';

        if (ttt_check_win(board, 'X'))
        {
            ttt_print_board(board);
            print_success("You win!\n\n");
            return;
        }

        if (ttt_board_full(board))
        {
            ttt_print_board(board);
            print("It's a draw!\n\n");
            return;
        }

        int move_o;

        do
        {
            move_o = random_range(0, 8);
        }
        while (board[move_o] == 'X' || board[move_o] == 'O');

        board[move_o] = 'O';

        if (ttt_check_win(board, 'O'))
        {
            ttt_print_board(board);
            print_error("Computer wins!\n\n");
            return;
        }

        if (ttt_board_full(board))
        {
            ttt_print_board(board);
            print("It's a draw!\n\n");
            return;
        }
    }
}


void games(void)
{
    print("\nGames:\n");

    print("  guess - Guess the Number\n");
    print("  guessh - Guess number (hard)\n");
    print("  rps   - Rock Paper Scissors\n");
    print("  word - Guess word\n");
    print("  hangman - Hangman\n");
    print("  tictactoe - Tic-Tac-Toe\n");
    print("  game coin - flip a coin\n");
    print("  game dice - roll a dice\n");
    print("  game highlow - guess higher or lower\n");
    print("  game math - game math\n");

    print("\n");
}

void testH(void)
{
    print_success("Hello!\n");
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

    set_color(base_color);

    print("First number: ");
    a = read_int();

    print("Operation (+ - * ^ /): ");
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
    else if (operation[0] == '^')
    {
        print_int(a ^ b);
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
    print("2026.08.27\n");
    print("v.1.4.2\n");
    set_color(base_color);
}

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

        if (strcmp(answer, "Rust") == 0)
        {
            print_success("Correct! You win!\n");
            return;
        }

        print_error("Wrong answer!\n");
    }

    print_error("The answer was: Rust\n");
}

void nwfetch(void)
{
    print_title("\n");

    print("       __  __       ");
    print_success("   NwOS\n");

    print("      |  \\|  |      ");
    print_success("   Version: 1.4.2\n");

    print("      | |\\| |      ");
    print_success("   Arch: x86\n");

    print("      | |\\| |      ");
    print_success("   Kernel: C\n");

    print("      |_|  |_|      ");
    print_success("   Bootloader: NASM\n");

    print("\n");

    set_color(COLOR_YELLOW);

    print("  Memory model: 32-bit\n");
    print("  Display: VGA text mode\n");
    print("  Keyboard: PS/2\n");
    print("\n");

    set_color(base_color);
}
void reboot(void)
{
    unsigned char status;

    do
    {
        __asm__ volatile (
            "inb %%dx, %%al"
            : "=a"(status)
            : "d"((unsigned short)0x64)
        );
    }
    while (status & 0x02);

    __asm__ volatile (
        "outb %%al, %%dx"
        :
        : "a"((unsigned char)0xFE),
          "d"((unsigned short)0x64)
    );

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}

typedef struct __attribute__((packed))
{
    unsigned char used;
    char name[MAX_FILENAME];
    unsigned int size;
} DirEntry;

static DirEntry directory[MAX_FILES];
void fs_load(void)
{
    unsigned char buffer[FS_DIR_SECTORS * 512];

    for (int s = 0; s < FS_DIR_SECTORS; s++)
    {
        ata_read_sector(FS_DIR_LBA + s, buffer + s * 512);
    }

    DirEntry* disk_entries = (DirEntry*)buffer;

    for (int i = 0; i < MAX_FILES; i++)
    {
        directory[i] = disk_entries[i];
        if (directory[i].used != 1)
        {
            directory[i].used = 0;
            directory[i].name[0] = '\0';
            directory[i].size = 0;
        }
    }
}
void fs_save_directory(void)
{
    unsigned char buffer[FS_DIR_SECTORS * 512];

    DirEntry* disk_entries = (DirEntry*)buffer;

    for (int i = 0; i < MAX_FILES; i++)
    {
        disk_entries[i] = directory[i];
    }

    for (int s = 0; s < FS_DIR_SECTORS; s++)
    {
        ata_write_sector(FS_DIR_LBA + s, buffer + s * 512);
    }
}
void fs_format(void)
{
    unsigned char empty[512];

    for (int i = 0; i < 512; i++)
    {
        empty[i] = 0;
    }

    for (int i = 0; i < MAX_FILES; i++)
    {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].size = 0;

        ata_write_sector(FS_DATA_LBA + i, empty);
    }

    fs_save_directory();

    print_success("Filesystem formatted.\n");
}

int fs_find(const char* name)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used &&
            strcmp(directory[i].name, name) == 0)
        {
            return i;
        }
    }

    return -1;
}

int fs_read_binary(
    const char* name,
    unsigned char* buffer,
    unsigned int max_size
);
static void print_hex_byte(unsigned char b)
{
    const char hex[] = "0123456789ABCDEF";
    putchar_os(hex[(b >> 4) & 0x0F]);
    putchar_os(hex[b & 0x0F]);
}
void app_hexview(const char* filename)
{
    static unsigned char buffer[512];

    if (!filename || filename[0] == '\0')
    {
        print_error("Usage: hexview <filename>\n");
        return;
    }

    if (!fs_read_binary(filename, buffer, sizeof(buffer)))
    {
        print_error("File not found or read error.\n");
        return;
    }

    print_success("\n--- HEX DUMP: ");
    print(filename);
    print(" ---\n\n");

    print("OFFSET   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  | ASCII\n");
    print("-------  -----------------------------------------------  | ----------------\n");

    for (int offset = 0; offset < 512; offset += 16)
    {
        print_hex_byte((offset >> 8) & 0xFF);
        print_hex_byte(offset & 0xFF);
        print("     ");
        for (int i = 0; i < 16; i++)
        {
            print_hex_byte(buffer[offset + i]);
            putchar_os(' ');
        }

        print(" | ");
        for (int i = 0; i < 16; i++)
        {
            unsigned char c = buffer[offset + i];
            if (c >= 32 && c <= 126)
            {
                putchar_os((char)c);
            }
            else
            {
                putchar_os('.');
            }
        }

        putchar_os('\n');
    }

    print_success("\n--- END OF DUMP ---\n\n");
}
void app_nwo_info(const char* filename)
{
    static unsigned char buffer[512];

    if (!filename || filename[0] == '\0')
    {
        print_error("Usage: nwocheck <file.nwo>\n");
        return;
    }

    if (!fs_read_binary(filename, buffer, sizeof(buffer)))
    {
        print_error("Failed to read NWO binary.\n");
        return;
    }

    print_success("\n=== NWO EXECUTABLE INSPECTOR ===\n");
    print("File: ");
    print(filename);
    print("\n\nFirst 16 Opcodes / Raw Bytes:\n[ ");

    for (int i = 0; i < 16; i++)
    {
        print_hex_byte(buffer[i]);
        putchar_os(' ');
    }
    print("]\n\n");

    print("Entry Opcode / Byte: 0x");
    print_hex_byte(buffer[0]);
    print("\nStatus: Valid binary image loaded into memory.\n");
    print("===============================\n\n");
}

void fs_create(const char* name)
{
    if (name[0] == '\0')
    {
        print_error("Filename is empty.\n");
        return;
    }

    if (fs_find(name) != -1)
    {
        print_error("File already exists.\n");
        return;
    }

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (!directory[i].used)
        {
            directory[i].used = 1;

            int j = 0;

            while (name[j] != '\0' &&
                   j < MAX_FILENAME - 1)
            {
                directory[i].name[j] = name[j];
                j++;
            }

            directory[i].name[j] = '\0';
            directory[i].size = 0;

            unsigned char empty[512];

            for (int k = 0; k < 512; k++)
            {
                empty[k] = 0;
            }

            ata_write_sector(FS_DATA_LBA + i, empty);

            fs_save_directory();

            print_success("File created.\n");
            return;
        }
    }

    print_error("No free file slots.\n");
}

void fs_list(void)
{
    print_title("\n=== FILES ===\n\n");

    int found = 0;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used)
        {
            print("  ");
            print(directory[i].name);

            print("  (");
            print_int((int)directory[i].size);
            print(" bytes)");

            putchar_os('\n');

            found = 1;
        }
    }

    if (!found)
    {
        print("  No files.\n");
    }

    putchar_os('\n');
}

void generate_num(void) {
    while (1) {
        unsigned int random_num = random_range(1, 100);
        print("Generated random number: ");
        set_color(COLOR_GREEN);
        print_int(random_num);
        set_color(base_color);
        print("\n");
        char ag[5];
        print("Do you want again? y/n\n");
        read_line(ag, 5);
        if (strcmp(ag, "y") == 0) { }
        else if (strcmp(ag, "n") == 0) {
            break;
        }
        else {
            print_error("Invalid command! automaticaly 'y'\n");
        }
    }
}

void games_menu(void)
{
    char choice[8];

    while (1)
    {
        clear();

        print("\n\n");
        set_color(COLOR_YELLOW);
        print("                    NwOS GAMES\n\n");
        set_color(base_color);


        print("              +----------------------+\n");
        print("              |      GAMES MENU      |\n");
        print("              +----------------------+\n");
        print("              |  1. Guess Number     |\n");
        print("              |  2. Word Game        |\n");
        print("              |  3. Rock Paper Sc.   |\n");
        print("              |  4. Coin Flip        |\n");
        print("              |  5. Dice             |\n");
        print("              |  6. Higher / Lower   |\n");
        print("              |  7. Math Quiz        |\n");
        print("              |  8. Hangman          |\n");
        print("              |  9. Tic-Tac-Toe      |\n");
        print("              |  10. Game memory     |\n");
        print("              |  11. Game Slot       |\n");
        print("              |                      |\n");
        print("              |  0. Back             |\n");
        print("              +----------------------+\n\n");

        print("                 Select: ");

        read_line(choice, 8);

        if (strcmp(choice, "0") == 0)
        {
            clear();
            return;
        }
        else if (strcmp(choice, "1") == 0)
        {
            clear();
            game_guess();
        }
        else if (strcmp(choice, "2") == 0)
        {
            clear();
            game_word();
        }
        else if (strcmp(choice, "3") == 0)
        {
            clear();
            game_rps();
        }
        else if (strcmp(choice, "4") == 0)
        {
            clear();
            game_coin();
        }
        else if (strcmp(choice, "5") == 0)
        {
            clear();
            game_dice();
        }
        else if (strcmp(choice, "6") == 0)
        {
            clear();
            game_higher_lower();
        }
        else if (strcmp(choice, "7") == 0)
        {
            clear();
            game_math();
        }
        else if (strcmp(choice, "8") == 0)
        {
            clear();
            game_hangman();
        }
        else if (strcmp(choice, "9") == 0)
        {
            clear();
            game_tictactoe();
        }
        else if (strcmp(choice, "10") == 0)
        {
            clear();
            game_memory();
        }
        else if (strcmp(choice, "11") == 0)
        {
            clear();
            game_slots();
        }
        else
        {
            print_error("\nInvalid choice.\n");
        }
    }
}

void apps_list() {
    clear();
    print("                 o-------------------------o\n");
    print("                 |                         |\n");
    print("                 |        1.Filestats      |\n");
    print("                 |        2.base converter |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |       0.exit            |\n");
    print("                 o-------------------------o\n");
    char choose[5];
    read_line(choose, 5);
    if (strcmp(choose, "1") == 0) { clear(); app_file_stats(); }
    else if (strcmp(choose, "2") == 0) { clear(); app_base_converter(); }
    else if (strcmp(choose, "0") == 0) {}
}

void fs_store_data(int index, const char* text, const char* success_message)
{
    unsigned char buffer[512];

    int i = 0;

    while (text[i] != '\0' &&
           i < MAX_FILE_SIZE - 1)
    {
        buffer[i] = text[i];
        i++;
    }

    buffer[i] = '\0';

    for (int k = i + 1; k < 512; k++)
    {
        buffer[k] = 0;
    }

    directory[index].size = (unsigned int)i;

    ata_write_sector(FS_DATA_LBA + index, buffer);
    fs_save_directory();

    if (text[i] != '\0')
    {
        print_error("Text is too long. Maximum is 512 characters.\n");
        return;
    }

    print_success(success_message);
}

void fs_write(const char* name, const char* text)
{
    int index = fs_find(name);

    if (index == -1)
    {
        print_error("File not found.\n");
        return;
    }

    fs_store_data(index, text, "File written.\n");
}

void fs_read(const char* name)
{
    int index = fs_find(name);

    if (index == -1)
    {
        print_error("File not found.\n");
        return;
    }

    unsigned char buffer[512];

    ata_read_sector(FS_DATA_LBA + index, buffer);
    buffer[511] = '\0';

    print_title("\n--- ");
    print(directory[index].name);
    print(" ---\n");

    print((const char*)buffer);

    putchar_os('\n');

    print_title("--- END ---\n\n");
}

void fs_edit(const char* name, const char* text)
{
    int index = fs_find(name);

    if (index == -1)
    {
        print_error("File not found.\n");
        return;
    }

    fs_store_data(index, text, "File edited.\n");
}

void fs_delete(const char* name)
{
    int index = fs_find(name);

    if (index == -1)
    {
        print_error("File not found.\n");
        return;
    }

    directory[index].used = 0;

    directory[index].name[0] = '\0';
    directory[index].size = 0;

    fs_save_directory();

    print_success("File deleted.\n");
}

int fs_read_text(
    const char* name,
    char* buffer,
    unsigned int max_size
)
{
    int index;
    unsigned char sector[512];
    unsigned int size;
    unsigned int copy_size;

    if (buffer == 0 || max_size == 0)
        return 0;

    buffer[0] = '\0';

    index = fs_find(name);

    if (index == -1)
        return 0;

    ata_read_sector(
        FS_DATA_LBA + index,
        sector
    );

    size = directory[index].size;
    if (size >= max_size)
        copy_size = max_size - 1;
    else
        copy_size = size;

    for (unsigned int i = 0; i < copy_size; i++)
    {
        buffer[i] = (char)sector[i];
    }

    buffer[copy_size] = '\0';

    return 1;
}

int fs_read_binary(
    const char* name,
    unsigned char* buffer,
    unsigned int max_size)
{
    int index;

    if (name == 0 ||
        buffer == 0 ||
        max_size == 0)
    {
        return 0;
    }

    index = fs_find(name);

    if (index == -1)
        return 0;
    if (max_size > 512)
        max_size = 512;

    ata_read_sector(
        FS_DATA_LBA + (unsigned int)index,
        buffer);

    return 1;
}

void t_colorgreen(void)
{
    set_base_color(COLOR_GREEN);
    print("setted color to green\n");
}
void t_colorred(void)
{
    set_base_color(COLOR_RED);
    print("setted color to red\n");
}
void t_colorgray(void)
{
    set_base_color(COLOR_LIGHT_GRAY);
    print("setted color to gray\n");
}
void t_colorblue(void)
{
    set_base_color(COLOR_BLUE);
    print("setted color to blue\n");
}
void t_colorcyan(void)
{
    set_base_color(COLOR_CYAN);
    print("setted color to cyan\n");
}

void t_colorsetter(void)
{
    int chooseT;
    print("1.green\n");
    print("2.red\n");
    print("3.gray (Default)\n");
    print("4.blue\n");
    print("5.cyan\n");
    chooseT = read_int();
    if (chooseT == 1)
    {
        t_colorgreen();
    }
    if (chooseT == 2)
    {
        t_colorred();
    }
    if (chooseT == 3)
    {
        t_colorgray();
    }
    if (chooseT == 4)
    {
        t_colorblue();
    }
    if (chooseT == 5)
    {
        t_colorcyan();
    }
}

void shell(void)
{
    char command[128];

    while (1)
    {
        set_color(COLOR_CYAN);
        print("NwOS> ");
        set_color(base_color);

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
            print("  color  - change color\n");
            print("  fs      - list files\n");
            print("  format  - erase all files\n");
            print("  panic safe|fatal - test kernel panic\n");
            print("  create <file> - create file\n");
            print("  read <file> - read file\n");
            print("  edit <file> - edit a file\n");
            print("  delete <file> - delete file\n");
            print("  reboot - restart NwOS\n");
            print("  run <file.nwo> - run NWO application\n");
            print("  apps - List of apps\n");
            print("  generate num - generate num\n");
            print("  hexview - hex byte map\n");
            print("  nwocheck - check OPcodes\n");
            print("  random word - generate word\n");
            print("  chat - chat with computer\n");
            set_color(base_color);
        }

        else if (strcmp(command, "clear") == 0)
        {
            clear();
        }

        else if (strcmp(command, "about") == 0)
        {
            set_color(COLOR_WHITE);
            print("====NwOS 1.4.2====\n");
            print("Name: NwOS\n");
            print("Version: v1.4.2\n");
            print("Arch: x86\n");
            print("Display: VGA text mode\n");
            print("PS/2 keyboard\n");
            print("================\n");
            set_color(base_color);
        }

        else if (starts_with(command, "echo "))
        {
            print(command + 5);
            putchar_os('\n');
        }

        else if (strcmp(command, "games") == 0)
        {
            games_menu();
        }
        
        else if (strcmp(command, "nwfetch") == 0)
        {
            nwfetch();
        }

        else if (strcmp(command, "datec") == 0)
        {
            dateCr();
        }
        else if (strcmp(command, "games old") == 0)
        {
            games();
        }

        else if (strcmp(command, "testH") == 0)
        {
            testH();
        }
        else if (strcmp(command, "fs") == 0)
        {
            fs_list();
        }
        else if (strncmp(command, "run ", 4) == 0)
        {
            nwo_run(command + 4);
        }
        else if (strcmp(command, "format") == 0)
        {
            fs_format();
        }
        else if (strcmp(command, "panic safe") == 0)
        {
            KPANIC_SAFE("Manual test panic (safe)");
        }
        else if (strcmp(command, "panic fatal") == 0)
        {
            KPANIC_FATAL("Manual test panic (fatal)");
        }
        else if (strncmp(command, "create ", 7) == 0)
        {
            fs_create(command + 7);
        }
        else if (strncmp(command, "read ", 5) == 0)
        {
            fs_read(command + 5);
        }
        else if (strncmp(command, "delete ", 7) == 0)
        {
            fs_delete(command + 7);
        }
        else if (strncmp(command, "hexview ", 8) == 0)
        {
            app_hexview(command + 8);
        }
        else if (strncmp(command, "nwocheck ", 9) == 0)
        {
            app_nwo_info(command + 9);
        }
        else if (strncmp(command, "write ", 6) == 0)
        {
            char* separator = command + 6;

            while (*separator != ' ' &&
                    *separator != '\0')
            {
                separator++;
            }

            if (*separator == '\0')
            {
                print_error("Usage: write <file> <text>\n");
            }
            else
            {
                *separator = '\0';

                fs_write(command + 6,
                        separator + 1);
            }
        }
        else if (strcmp(command, "calc") == 0)
        {
            Calc();
        }
        else if (strcmp(command, "guessh") == 0)
        {
            game_guessh();
        }
        else if (strncmp(command, "edit ", 5) == 0)
        {
            redactor(command + 5);
        }
        else if (strcmp(command, "color") == 0)
        {
            t_colorsetter();
        }
        else if (strcmp(command, "reboot") == 0)
        {
            reboot();
        }
        else if (strcmp(command, "game coin") == 0)
        {
            game_coin();
        }
        else if (strcmp(command, "game dice") == 0)
        {
            game_dice();
        }
        else if (strcmp(command, "generate num") == 0)
        {
            generate_num();
        }
        else if (strcmp(command, "apps") == 0)
        {
            apps_list();
        }
        else if (strcmp(command, "game highlow") == 0)
        {
            game_higher_lower();
        }
        else if (strcmp(command, "game math") == 0)
        {
            game_math();
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
        else if (strcmp(command, "hangman") == 0)
        {
            game_hangman();
        }
        else if (strcmp(command, "tictactoe") == 0)
        {
            game_tictactoe();
        }
        else if (strcmp(command, "random word") == 0)
        {
            say();
        }
        else if (strcmp(command, "chat") == 0)
        {
            chat();
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

void kernel_main(void)
{
    clear();

    print("CLEARING...\n");

    random_init();
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("RANDOM\n");
    fs_load();
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("FILESYSTEM\n");
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("KEYBOARD\n");
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("COMPILER\n");

    print("================================\n");
    print("        Welcome to NwOS 1.4.2\n");
    print("================================\n");
    print("Type 'help' for commands.\n\n");
    set_color(COLOR_GREEN);
    print("================");
    set_color(COLOR_BLUE);
    print("================\n");
    set_color(COLOR_LIGHT_GRAY);

    shell();

    while (1)
    {
        __asm__ volatile ("hlt");
    }
}