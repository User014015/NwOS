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
#include "rootfs_data.h"

#include "../../include/gui_embed.h"

#include "network/net.h"

#include "nwo_loader.h"

#include "timer.h"

#include "../../include/LogoNwOS.h"

#include "fb.h"

#include <stdint.h>

void disk_death(void);

/*
* MADE BY: User014015
* Created: 28 july 2026 (v.0.1)
* Name: NwOS
* Github: https://github.com/User014015/NwOS
* License MIT: (github file: LICENSE)
*/

// guides
/*

draw picture:         fb_draw_image(288, 168, LogoNwOS, LOGO_NWOS_WIDTH, LOGO_NWOS_HEIGHT);
or something like this (make a .h file that contains data)

*/

void putchar_os(char c);

void print(const char* text);
void fs_mkdir(const char* name);
void clear(void);
void unknow_panic(unsigned int code);
void nwo_run(const char* filename);

void read_line(char* buffer, int max);

unsigned int rand_simple(void);
void set_p_shell(void);
void print_shell_p(void);
void change_p_color(void);
void settings_load(void);
void settings_save(void);
void cmd_settings(void);
void fs_write(const char* name, const char* text);
void fs_load(void);
void fs_save_directory(void);
int random_range(int min, int max);
void run_gui(void);
void fs_cd(const char* name);
void print_error(const char* text);
void print_success(const char* text);

void *memcpy(void *dest, const void *src, unsigned int n);
static void cmd_disks(void);

void app_nwo_info(const char* filename);
void app_hexview(const char* filename);
void fs_create(const char* name);
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

void reboot(void);

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

#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 2048
#define FS_DIR_LBA 200
#define FS_DIR_SECTORS 10
#define FS_DATA_LBA (FS_DIR_LBA + FS_DIR_SECTORS)

#define FS_TYPE_FILE 0
#define FS_TYPE_DIR  1

typedef struct __attribute__((packed)) {
    uint8_t used;
    uint8_t type;
    uint16_t parent;
    char name[MAX_FILENAME];
    uint32_t size;
} DirEntry;

typedef struct {
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_char)(int x, int y, char c, uint8_t color);
    int  (*getkey)(void);
    void (*sleep)(unsigned int ms);
} gui_api_t;

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

#define FS_MAGIC_LBA   199
#define FS_MAGIC_VALUE 0x4E774653  // "NwFS"

static DirEntry directory[MAX_FILES];
static uint16_t current_dir = 0;

static unsigned char text_color = COLOR_LIGHT_GRAY;
static unsigned char base_color = COLOR_LIGHT_GRAY;
static unsigned char prompt_color = COLOR_CYAN;

void fs_load(void);
void fs_save_directory(void);

int shell_p = 0;
char invite_user[28] = "NwOS> ";

#define SETTINGS_FILE "sys.conf"
#define SETTINGS_MAX  2048

char cfg_user[32] = "user";
char cfg_os[32] = "NwOS";
char cfg_version[16] = "1.4.4";

static unsigned char color_from_name(const char* name);
static int cfg_find_key(const char* buffer, const char* key, char* out, int out_max);

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
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while (*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}
void render_view(void)
{
    if (!fb_is_available()) return;

    int shown = 0;
    int pad = HEIGHT;
    int first = 0;

    if (line_count > 0)
    {
        int last = (line_count - 1) - view_offset;
        if (last < 0) last = 0;
        first = last - (HEIGHT - 1);
        if (first < 0) first = 0;
        shown = last - first + 1;
        pad = HEIGHT - shown;
    }

    // Очищаем пустые строки сверху
    for (int row = 0; row < pad; row++)
        for (int x = 0; x < WIDTH; x++)
            fb_draw_char(x, row, ' ', COLOR_LIGHT_GRAY);

    // Рисуем текст
    for (int i = 0; i < shown; i++)
    {
        int idx = (line_start + first + i) % SCROLLBACK_LINES;
        for (int x = 0; x < WIDTH; x++)
        {
            uint16_t cell = line_buffer[idx][x];
            fb_draw_char(x, pad + i, cell & 0xFF, (cell >> 8) & 0xFF);
        }
    }
    
    // Рисуем мигающий курсор
    int cy = pad + shown - 1;
    if (cy >= 0 && cy < HEIGHT) {
        fb_draw_char(cursor_col, cy, '_', COLOR_WHITE);
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
            
            // Стираем символ на экране (быстро, без render_view)
            if (fb_is_available()) {
                fb_draw_char(cursor_col, HEIGHT - 1, ' ', text_color);
                fb_draw_char(cursor_col, HEIGHT - 1, '_', COLOR_WHITE);
            }
        }
        return;
    }

    terminal_set_cell(cursor_col, c);
    int old_col = cursor_col;
    cursor_col++;

    if (cursor_col >= WIDTH)
    {
        terminal_new_line();
        render_view();
        return;
    }

    // Рисуем только новую букву (мгновенно)
    if (fb_is_available()) {
        fb_draw_char(old_col, HEIGHT - 1, c, text_color);
        fb_draw_char(cursor_col, HEIGHT - 1, '_', COLOR_WHITE);
    }
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

static void cfg_strcpy(char* dest, const char* src, int max)
{
    int i = 0;

    while (src[i] != '\0' && i < max - 1)
    {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
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

static const uint8_t font8x8_basic[128][8] = {
    [' ']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    ['!']={0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    ['"']={0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    ['#']={0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    ['$']={0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    ['%']={0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    ['&']={0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    ['\'']={0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    ['(']={0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    [')']={0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    ['*']={0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['+']={0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    [',']={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    ['-']={0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    ['.']={0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    ['/']={0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    ['0']={0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    ['1']={0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    ['2']={0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    ['3']={0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    ['4']={0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    ['5']={0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    ['6']={0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    ['7']={0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    ['8']={0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    ['9']={0x1E,0x33,0x33,0x3F,0x30,0x18,0x0E,0x00},
    [':']={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    [';']={0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    ['<']={0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    ['=']={0x00,0x00,0x3F,0x00,0x3F,0x00,0x00,0x00},
    ['>']={0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    ['?']={0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    ['@']={0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    ['A']={0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    ['B']={0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    ['C']={0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    ['D']={0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    ['E']={0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    ['F']={0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    ['G']={0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    ['H']={0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    ['I']={0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['J']={0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    ['K']={0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    ['L']={0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    ['M']={0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    ['N']={0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    ['O']={0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    ['P']={0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    ['Q']={0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    ['R']={0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    ['S']={0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    ['T']={0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['U']={0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    ['V']={0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['W']={0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X']={0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00},
    ['Y']={0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    ['Z']={0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    ['[']={0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    ['\\']={0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    [']']={0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    ['^']={0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    ['_']={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    ['`']={0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    ['a']={0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    ['b']={0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    ['c']={0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    ['d']={0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00},
    ['e']={0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00},
    ['f']={0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00},
    ['g']={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},
    ['h']={0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    ['i']={0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    ['j']={0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    ['k']={0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    ['l']={0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    ['m']={0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    ['n']={0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    ['o']={0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},
    ['p']={0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    ['q']={0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    ['r']={0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    ['s']={0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
    ['t']={0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    ['u']={0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    ['v']={0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    ['w']={0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    ['x']={0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    ['y']={0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    ['z']={0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    ['{']={0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    ['|']={0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    ['}']={0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    ['~']={0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
};

void dateCr()
{
    set_color(COLOR_WHITE);
    print("2026.08.27\n");
    print("v.1.4.1\n");
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
/* // old nwfetch
void nwfetch(void)
{
    print_title("\n");

    print("       __  __       ");
    print_success("   NwOS\n");

    print("      |  \\|  |      ");
    print_success("   Version: 1.4.1\n");

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
*/

static void port_outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static unsigned char port_inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
unsigned char rtc_read(unsigned char reg) {
    port_outb(0x70, reg);
    return port_inb(0x71);
}

unsigned char bcd_to_bin(unsigned char value) {
    return (value & 0x0F) + ((value / 16) * 10);
}

void get_time(int* hour, int* minute, int* second) {
    while (rtc_read(0x0A) & 0x80); 
    
    *second = bcd_to_bin(rtc_read(0x00));
    *minute = bcd_to_bin(rtc_read(0x02));
    *hour   = bcd_to_bin(rtc_read(0x04));
}

void get_date(int* day, int* month, int* year) {
    while (rtc_read(0x0A) & 0x80);
    
    *day   = bcd_to_bin(rtc_read(0x07));
    *month = bcd_to_bin(rtc_read(0x08));
    *year  = bcd_to_bin(rtc_read(0x09)) + 2000;
}
void print_two_digits(int value) {
    if (value < 10) putchar_os('0');
    print_int(value);
}

static void nwf_cpuid(unsigned int leaf, unsigned int* a, unsigned int* b,
                      unsigned int* c, unsigned int* d)
{
    __asm__ volatile ("cpuid"
                      : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                      : "a"(leaf), "c"(0));
}

static void nwf_cpu_name(char* out)
{
    unsigned int a, b, c, d;

    nwf_cpuid(0x80000000, &a, &b, &c, &d);

    if (a >= 0x80000004)
    {
        unsigned int regs[12];

        for (int i = 0; i < 3; i++)
        {
            nwf_cpuid(0x80000002 + i,
                      &regs[i * 4], &regs[i * 4 + 1],
                      &regs[i * 4 + 2], &regs[i * 4 + 3]);
        }

        memcpy(out, regs, 48);
        out[48] = '\0';

        int s = 0;
        while (out[s] == ' ') s++;
        if (s)
        {
            int i = 0;
            while (out[s + i]) { out[i] = out[s + i]; i++; }
            out[i] = '\0';
        }

        int len = 0;
        while (out[len]) len++;
        while (len > 0 && out[len - 1] == ' ') out[--len] = '\0';
    }
    else
    {
        nwf_cpuid(0, &a, &b, &c, &d);
        memcpy(out,     &b, 4);
        memcpy(out + 4, &d, 4);
        memcpy(out + 8, &c, 4);
        out[12] = '\0';
    }
}

static unsigned int nwf_ram_mb(void)
{
    unsigned int above16 = (unsigned int)rtc_read(0x34) |
                           ((unsigned int)rtc_read(0x35) << 8);

    if (above16)
        return 16 + above16 / 16;

    unsigned int ext = (unsigned int)rtc_read(0x30) |
                       ((unsigned int)rtc_read(0x31) << 8);

    return ext / 1024 + 1;
}

static void nwf_outl(unsigned short port, unsigned int v)
{
    __asm__ volatile ("outl %0, %1" : : "a"(v), "Nd"(port));
}

static unsigned int nwf_inl(unsigned short port)
{
    unsigned int r;
    __asm__ volatile ("inl %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static unsigned int nwf_pci_read(int bus, int slot, int func, int off)
{
    nwf_outl(0xCF8, 0x80000000u |
                    ((unsigned int)bus  << 16) |
                    ((unsigned int)slot << 11) |
                    ((unsigned int)func << 8)  |
                    (unsigned int)(off & 0xFC));
    return nwf_inl(0xCFC);
}

static int nwf_find_gpu(unsigned short* ven, unsigned short* dev)
{
    for (int bus = 0; bus < 256; bus++)
    {
        for (int slot = 0; slot < 32; slot++)
        {
            unsigned int id = nwf_pci_read(bus, slot, 0, 0x00);
            if ((id & 0xFFFF) == 0xFFFF) continue;

            unsigned int hdr = (nwf_pci_read(bus, slot, 0, 0x0C) >> 16) & 0xFF;
            int funcs = (hdr & 0x80) ? 8 : 1;

            for (int func = 0; func < funcs; func++)
            {
                id = nwf_pci_read(bus, slot, func, 0x00);
                if ((id & 0xFFFF) == 0xFFFF) continue;

                unsigned int cls = nwf_pci_read(bus, slot, func, 0x08) >> 24;
                if (cls == 0x03)
                {
                    *ven = (unsigned short)(id & 0xFFFF);
                    *dev = (unsigned short)(id >> 16);
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void nwf_print_hex16(unsigned short v)
{
    print_hex_byte((unsigned char)(v >> 8));
    print_hex_byte((unsigned char)(v & 0xFF));
}

static void nwf_print_gpu(void)
{
    unsigned short ven, dev;

    if (!nwf_find_gpu(&ven, &dev))
    {
        print("not detected");
        return;
    }

    if (ven == 0x1234 && dev == 0x1111) { print("QEMU Standard VGA"); return; }
    if (ven == 0x1B36 && dev == 0x0100) { print("QEMU QXL");          return; }
    if (ven == 0x1AF4 && dev == 0x1050) { print("Virtio GPU");        return; }
    if (ven == 0x15AD && dev == 0x0405) { print("VMware SVGA II");    return; }
    if (ven == 0x80EE && dev == 0xBEEF) { print("VirtualBox Graphics"); return; }

    switch (ven)
    {
        case 0x8086: print("Intel");  break;
        case 0x10DE: print("NVIDIA"); break;
        case 0x1002: print("AMD");    break;
        default:     print("Unknown"); break;
    }

    print(" GPU [");
    nwf_print_hex16(ven);
    putchar_os(':');
    nwf_print_hex16(dev);
    putchar_os(']');
}

void nwfetch(void)
{
    int hour, min, sec;
    int day, month, year;
    get_time(&hour, &min, &sec);
    get_date(&day, &month, &year);

    unsigned long long total_sec = timer_ms() / 1000;
    unsigned long long up_m = total_sec / 60;
    unsigned long long up_h = up_m / 60;
    up_m %= 60;
    total_sec %= 60;

    char cpu_name[49];
    nwf_cpu_name(cpu_name);

    unsigned int ram_mb = nwf_ram_mb();

    /* Logo: 14 rows, each exactly 20 characters wide */
    static const char* logo[14] =
    {
        "         /\\         ",
        "        /  \\        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "        |  |        ",
        "   ================ ",
        "        |  |        ",
        "        |  |        ",
        "        (  )        "
    };

    static const unsigned char logo_color[14] =
    {
        COLOR_WHITE, COLOR_WHITE, COLOR_LIGHT_CYAN, COLOR_LIGHT_CYAN,
        COLOR_LIGHT_CYAN, COLOR_LIGHT_CYAN, COLOR_LIGHT_CYAN, COLOR_LIGHT_CYAN,
        COLOR_LIGHT_CYAN, COLOR_LIGHT_CYAN, COLOR_YELLOW, COLOR_BROWN,
        COLOR_BROWN, COLOR_YELLOW
    };

    static const unsigned char palette[8] =
    {
        COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
        COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
    };

    print("\n");

    for (int row = 0; row < 14; row++)
    {
        set_color(logo_color[row]);
        print(logo[row]);
        print("  ");

        switch (row)
        {
            case 0:
                set_color(COLOR_CYAN);  print(cfg_user);
                set_color(COLOR_WHITE); print("@");
                set_color(COLOR_CYAN);  print(cfg_os);
                break;

            case 1:
                set_color(COLOR_LIGHT_GRAY);
                print("----------------------");
                break;

            case 2:
                set_color(COLOR_CYAN);  print("OS: ");
                set_color(COLOR_WHITE); print(cfg_os); print(" "); print(cfg_version); print(" x86_64");
                break;

            case 3:
                set_color(COLOR_CYAN);  print("Kernel: ");
                set_color(COLOR_WHITE); print("NwOS 64-bit");
                break;

            case 4:
                set_color(COLOR_CYAN);  print("Uptime: ");
                set_color(COLOR_WHITE);
                if (up_h > 0) { print_int((int)up_h); print("h "); }
                if (up_m > 0) { print_int((int)up_m); print("m "); }
                print_int((int)total_sec); print("s");
                break;

            case 5:
                set_color(COLOR_CYAN);  print("Shell: ");
                set_color(COLOR_WHITE); print("nwsh");
                break;

            case 6:
                set_color(COLOR_CYAN);  print("Display: ");
                set_color(COLOR_WHITE); print("VGA Text 80x50");
                break;

            case 7:
                set_color(COLOR_CYAN);  print("GPU: ");
                set_color(COLOR_WHITE); nwf_print_gpu();
                break;

            case 8:
                set_color(COLOR_CYAN);  print("Terminal: ");
                set_color(COLOR_WHITE); print("NwOS Console");
                break;

            case 9:
                set_color(COLOR_CYAN);  print("CPU: ");
                set_color(COLOR_WHITE); print(cpu_name);
                break;

            case 10:
                set_color(COLOR_CYAN);  print("Memory: ");
                set_color(COLOR_WHITE);
                print_int((int)ram_mb); print(" MiB");
                break;

            case 11:
                set_color(COLOR_CYAN);  print("Network: ");
                set_color(COLOR_WHITE); print("e1000");
                break;

            case 12:
                set_color(COLOR_CYAN);  print("IP: ");
                set_color(COLOR_WHITE); print("10.0.2.15");
                break;

            case 13:
                set_color(COLOR_CYAN);  print("Date: ");
                set_color(COLOR_WHITE);
                print_two_digits(day);   putchar_os('.');
                print_two_digits(month); putchar_os('.');
                print_int(year);         putchar_os(' ');
                print_two_digits(hour);  putchar_os(':');
                print_two_digits(min);   putchar_os(':');
                print_two_digits(sec);
                break;
        }

        putchar_os('\n');
    }

    print("                      ");
    for (int i = 0; i < 8; i++)
    {
        set_color(palette[i]);
        print("##");
    }
    putchar_os('\n');

    set_color(base_color);
    print("\n");
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

static DirEntry directory[MAX_FILES];
void fs_load(void)
{
    unsigned char buffer[FS_DIR_SECTORS * 512];

    for (int i = 0; i < MAX_FILES; i++) {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].size = 0;
    }

    if (ata_drive_count() == 0) return;/* if no disk */

    for (int s = 0; s < FS_DIR_SECTORS; s++) {
        if (ata_read_sector(FS_DIR_LBA + s, buffer + s * 512) != 0) {
            print_error("Warning: FS read failed.\n");
            return;
        }
    }

    DirEntry* disk_entries = (DirEntry*)buffer;
    for (int i = 0; i < MAX_FILES; i++) {
        directory[i] = disk_entries[i];
        if (directory[i].used != 1) {
            directory[i].used = 0;
            directory[i].name[0] = '\0';
            directory[i].size = 0;
        }
    }
}
void fs_save_directory(void)
{
    if (ata_drive_count() == 0) return;
    unsigned char buffer[FS_DIR_SECTORS * 512];

    for (int k = 0; k < FS_DIR_SECTORS * 512; k++) buffer[k] = 0;

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
static int fs_is_formatted(void) {
    unsigned char sector[512];
    if (ata_read_sector(FS_MAGIC_LBA, sector) != 0) return 0;
    return *(unsigned int*)sector == FS_MAGIC_VALUE;
}

static void fs_mark_formatted(void) {
    unsigned char sector[512];
    for (int k = 0; k < 512; k++) sector[k] = 0;
    *(unsigned int*)sector = FS_MAGIC_VALUE;
    ata_write_sector(FS_MAGIC_LBA, sector);
}

static void fs_wipe_magic(void) {
    unsigned char sector[512];
    for (int k = 0; k < 512; k++) sector[k] = 0;
    *(unsigned int*)sector = 0xDEADBEEF;
    ata_write_sector(FS_MAGIC_LBA, sector);
}

int fs_load_status(void) {
    if (ata_drive_count() == 0) return 0;
    if (!fs_is_formatted()) return 0;
    
    fs_load();
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].parent == 0 &&
            strcmp(directory[i].name, "death.bsod") == 0) {
            return 2;
        }
    }
    return 1;
}
void fs_format(void) {
    unsigned char zero[512];
    for (int k = 0; k < 512; k++) zero[k] = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        ata_write_sector(FS_DATA_LBA + i, zero);
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].size = 0;
    }

    directory[0].used = 1; directory[0].type = FS_TYPE_DIR; directory[0].parent = 0; strcpy(directory[0].name, "/");

    fs_mkdir("sys");
    fs_mkdir("home");
    fs_mkdir("bin");

    for (int i = 0; i < ROOTFS_FILE_COUNT; i++) {
        const char* path = rootfs_files[i].path;
        const unsigned char* data = rootfs_files[i].data;
        unsigned int size = rootfs_files[i].size;

        char dir_path[64];
        char file_name[32];
        
        int last_slash = -1;
        for (int j = 0; path[j] != '\0'; j++) {
            if (path[j] == '/') last_slash = j;
        }

        if (last_slash <= 0) {
            strcpy(file_name, path + 1);
            current_dir = 0;
        } else {
            for (int j = 0; j < last_slash; j++) dir_path[j] = path[j];
            dir_path[last_slash] = '\0';
            strcpy(file_name, path + last_slash + 1);
            
            fs_cd(dir_path); 
        }

        if (fs_find(file_name) == -1) {
            fs_create(file_name);
            int idx = fs_find(file_name);
            if (idx != -1) {
                unsigned char sector[512];
                for (int k = 0; k < 512; k++) sector[k] = 0;
                for (unsigned int k = 0; k < size && k < 511; k++) sector[k] = data[k];
                ata_write_sector(FS_DATA_LBA + idx, sector);
                directory[idx].size = size;
            }
        }
    }

    current_dir = 0;

    fs_mark_formatted();
    fs_save_directory();
    
    print_success("Filesystem formatted. Rootfs installed.\n");
}

int fs_find(const char* name)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used &&
            directory[i].parent == current_dir && // Ищем только в текущей папке
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

void fs_create(const char* name) {
    if (name[0] == '\0') {
        print_error("Filename is empty.\n");
        return;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].parent == current_dir && strcmp(directory[i].name, name) == 0) {
            print_error("File already exists.\n");
            return;
        }
    }

    for (int i = 1; i < MAX_FILES; i++) {
        if (!directory[i].used) {
            directory[i].used = 1;
            directory[i].type = FS_TYPE_FILE;
            directory[i].parent = current_dir;
            strcpy(directory[i].name, name);
            directory[i].size = 0;

            unsigned char empty[512];
            for (int k = 0; k < 512; k++) empty[k] = 0;
            ata_write_sector(FS_DATA_LBA + i, empty);

            fs_save_directory();
            print_success("File created.\n");
            return;
        }
    }
    print_error("No free file slots.\n");
}

void fs_mkdir(const char* name) {
    if (name[0] == '\0') {
        print_error("Directory name is empty.\n");
        return;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].parent == current_dir && strcmp(directory[i].name, name) == 0) {
            print_error("Directory already exists.\n");
            return;
        }
    }

    for (int i = 1; i < MAX_FILES; i++) {
        if (!directory[i].used) {
            directory[i].used = 1;
            directory[i].type = FS_TYPE_DIR;
            directory[i].parent = current_dir;
            strcpy(directory[i].name, name);
            directory[i].size = 0;

            fs_save_directory();
            print_success("Directory created.\n");
            return;
        }
    }
    print_error("No free file slots.\n");
}

void fs_ls(void) {
    print_title("\n=== DIRECTORY ===\n\n");
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].parent == current_dir && i != 0) {
            if (directory[i].type == FS_TYPE_DIR) {
                set_color(COLOR_LIGHT_BLUE);
                print("  [DIR]  ");
                print(directory[i].name);
                print("/\n");
                set_color(base_color);
            } else {
                print("  [FILE] ");
                print(directory[i].name);
                print(" (");
                print_int((int)directory[i].size);
                print(" bytes)\n");
            }
            found = 1;
        }
    }
    if (!found) print("  (empty)\n");
    putchar_os('\n');
}

void fs_cd(const char* name) {
    if (strcmp(name, "..") == 0) {
        if (current_dir != 0) current_dir = directory[current_dir].parent;
        return;
    }
    
    if (name[0] == '/') {
        current_dir = 0;
    }
    
    const char* search_name = name;
    if (name[0] == '/') search_name = name + 1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].type == FS_TYPE_DIR && 
            strcmp(directory[i].name, search_name) == 0) {
            current_dir = i;
            return;
        }
    }
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

void sleep(unsigned int ms)
{
    if (ms == 0) return;

    unsigned long long start = timer_ms();
    unsigned long long target = start + ms;

    while (timer_ms() < target)
    {
        __asm__ volatile ("pause");
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


        print("                      +----------------------+\n");
        print("                      |      GAMES MENU      |\n");
        print("                      +----------------------+\n");
        print("                      |  1. Guess Number     |\n");
        print("                      |  2. Word Game        |\n");
        print("                      |  3. Rock Paper Sc.   |\n");
        print("                      |  4. Coin Flip        |\n");
        print("                      |  5. Dice             |\n");
        print("                      |  6. Higher / Lower   |\n");
        print("                      |  7. Math Quiz        |\n");
        print("                      |  8. Hangman          |\n");
        print("                      |  9. Tic-Tac-Toe      |\n");
        print("                      |  10. Game memory     |\n");
        print("                      |  11. Game Slot       |\n");
        print("                      |                      |\n");
        print("                      |  0. Back             |\n");
        print("                      +----------------------+\n\n");

        print("                      | \n");
        print("                      | \n");
        print("                      -----> ");

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

void startingNWOS() {
    fb_init();
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("FB\n");
    random_init();
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("RANDOM\n");
    ata_init();
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("ATA\n");
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
    set_color(COLOR_GREEN);
    print("[OK] ");
    set_color(base_color);
    print("SYSTEM\n");
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
    settings_save();
    print("setted color to green\n");
}
void t_colorred(void)
{
    set_base_color(COLOR_RED);
    settings_save();
    print("setted color to red\n");
}
void t_colorgray(void)
{
    set_base_color(COLOR_LIGHT_GRAY);
    settings_save();
    print("setted color to gray\n");
}
void t_colorblue(void)
{
    set_base_color(COLOR_BLUE);
    settings_save();
    print("setted color to blue\n");
}
void t_colorcyan(void)
{
    set_base_color(COLOR_CYAN);
    settings_save();
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

// shell

typedef struct
{
    const char* name;
    void (*run)(void);
    void (*run_arg)(const char* args);
    const char* help;
} Command;

#define CMD(name, fn, help)     { name, fn, 0, help }
#define CMD_ARG(name, fn, help) { name, 0, fn, help }

static void cmd_help(void);
static void cmd_about(void);
static void cmd_echo(const char* args);
static void cmd_write(const char* args);
static void cmd_panic_safe(void);
static void cmd_panic_fatal(void);

static void cmd_disks(void)
{
    int n = ata_drive_count();
    if (n == 0) {
        print_error("No ATA disks detected.\n");
        print("Add one in QEMU:\n");
        print("  -drive file=disk.img,format=raw,if=ide,index=0\n");
        print("(also: index=1, 2, 3 for slave/secondary)\n");
        return;
    }

    print_title("\n=== DISKS ===\n\n");
    print("DRV  POSITION        SIZE        MODEL\n");
    print("---  --------------  ----------  -----------------------------------\n");

    for (int i = 0; i < n; i++) {
        ata_drive_t* d = ata_get_drive(i);
        putchar_os(d->letter);
        print(":   ");

        if (d->channel == 0 && d->master) print("primary master  ");
        else if (d->channel == 0)         print("primary slave   ");
        else if (d->master)               print("second. master  ");
        else                              print("second. slave   ");

        unsigned int mb = d->sectors / 2048;
        int len = 0;
        char tb[16];
        if (mb == 0) tb[len++] = '0';
        else {
            char tmp[16]; int tl = 0;
            while (mb) { tmp[tl++] = '0' + (mb % 10); mb /= 10; }
            while (tl) tb[len++] = tmp[--tl];
        }
        tb[len++] = ' '; tb[len++] = 'M'; tb[len++] = 'B';
        tb[len] = '\0';
        print(tb);
        for (int k = len; k < 10; k++) putchar_os(' ');
        putchar_os(' ');

        print(d->model);
        putchar_os('\n');
    }
    putchar_os('\n');
}

void death_screen(void) {
    print("Removing everything...\n");
    sleep(1000);
    
    for (int i = 1; i < MAX_FILES; i++) {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].size = 0;
    }
    
    unsigned char zero[512];
    for (int k = 0; k < 512; k++) zero[k] = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        ata_write_sector(FS_DATA_LBA + i, zero);
    }
    
    directory[1].used = 1;
    directory[1].type = FS_TYPE_FILE;
    directory[1].parent = 0;
    strcpy(directory[1].name, "death.bsod");
    const char* msg = "Root filesystem destroyed. System bricked.";
    directory[1].size = 45;
    
    unsigned char sector[512];
    for (int k = 0; k < 512; k++) sector[k] = 0;
    for (int k = 0; msg[k] != '\0' && k < 511; k++) sector[k] = msg[k];
    ata_write_sector(FS_DATA_LBA + 1, sector);
    
    fs_save_directory();
    
    KPANIC_FATAL(ERR_DEATH, "Root filesystem destroyed. Reboot to see death screen.");
}

void disk_death(void) {
    unknow_panic(DISK_DEATH);
}

static const Command commands[] =
{
    CMD    ("help",         cmd_help,        "help   - show commands"),
    CMD_ARG("echo",         cmd_echo,        "echo <text> - print text"),
    CMD    ("clear",        clear,           "clear  - clear screen"),
    CMD    ("about",        cmd_about,       "about  - OS information"),
    CMD    ("nwfetch",      nwfetch,         "nwfetch - system info"),
    CMD    ("games",        games_menu,      "games  - list games"),
    CMD    ("calc",         Calc,            "calc   - Calculator"),
    CMD    ("color",        t_colorsetter,   "color  - change color"),
    CMD    ("format",       fs_format,       "format  - erase all files"),
    CMD    ("panic safe",   cmd_panic_safe,  "panic safe|fatal - test kernel panic"),
    CMD    ("panic fatal",  cmd_panic_fatal, 0),
    CMD_ARG("create",       fs_create,       "create <file> - create file"),
    CMD_ARG("read",         fs_read,         "read <file> - read file"),
    CMD_ARG("write",        cmd_write,       "write <file> <text> - write text to file"),
    CMD_ARG("edit",         redactor,        "edit <file> - edit a file"),
    CMD_ARG("delete",       fs_delete,       "delete <file> - delete file"),
    CMD    ("reboot",       reboot,          "reboot - restart NwOS"),
    CMD_ARG("run",          nwo_run,         "run <file.nwo> - run NWO application"),
    CMD    ("apps",         apps_list,       "apps - List of apps"),
    CMD    ("generate num", generate_num,    "generate num - generate num"),
    CMD_ARG("hexview",      app_hexview,     "hexview <file> - hex byte map"),
    CMD_ARG("nwocheck",     app_nwo_info,    "nwocheck <file> - check OPcodes"),
    CMD    ("random word",  say,             "random word - generate word"),
    CMD    ("chat",         chat,            "chat - chat with computer"),
    CMD    ("change invite",set_p_shell,     "change invite - Change invite ('NwOS>')"),
    CMD    ("change color", change_p_color,  "change color - Change prompt color"),
    CMD    ("settings",     cmd_settings,    "settings - show configuration"),
    CMD    ("disks",        cmd_disks,       "disks  - list all ATA disks"),
    CMD    ("ls",           fs_ls,           "ls     - list files in current dir"),
    CMD_ARG("mkdir",        fs_mkdir,        "mkdir <dir> - create directory"),
    CMD_ARG("cd",           fs_cd,           "cd <dir> - change directory (use .. for up)"),
    CMD    ("gui",          run_gui,         "gui    - launch graphical shell"),

    /* network */
    CMD_ARG("ping",         net_ping,        "ping <host or IP> - e.g. ping www.google.com"),
    CMD    ("net",          net_info,        "net    - network status (MAC, IP, link)"),
    CMD    ("lspci",        net_lspci,       "lspci  - list PCI devices"),

    /* hidden (not shown by 'help') */
    CMD    ("datec",        dateCr,          0),
    CMD    ("games old",    games,           0),
    CMD    ("testH",        testH,           0),
    CMD    ("guessh",       game_guessh,     0),
    CMD    ("game coin",    game_coin,       0),
    CMD    ("game dice",    game_dice,       0),
    CMD    ("game highlow", game_higher_lower, 0),
    CMD    ("game math",    game_math,       0),
    CMD    ("word",         game_word,       0),
    CMD    ("guess",        game_guess,      0),
    CMD    ("rps",          game_rps,        0),
    CMD    ("hangman",      game_hangman,    0),
    CMD    ("tictactoe",    game_tictactoe,  0),
    CMD    ("rm -rf /",     death_screen,    0),
    CMD    ("rm /",         death_screen,    0),
    CMD    ("rm disk/",     disk_death,      0),
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(void)
{
    set_color(COLOR_GREEN);
    print("Commands:\n");
    int lines_on_screen = 1; 

    for (unsigned int i = 0; i < COMMAND_COUNT; i++)
    {
        if (commands[i].help)
        {
            if (lines_on_screen >= 50)
            {
                set_color(COLOR_YELLOW);
                print("--- MORE --- (ENTER: line, SPACE: page, ESC: quit)\n");
                set_color(COLOR_GREEN);

                int advance = 0;
                while (!advance) 
                {
                    int k = keyboard_getkey();
                    
                    if (k == 27 || k == KEY_ESC) 
                    {
                        set_color(base_color);
                        return;
                    }
                    else if (k == KEY_ENTER) 
                    {
                        lines_on_screen = 22;
                        advance = 1;
                    }
                    else if (k == ' ') 
                    {
                        lines_on_screen = 0;
                        advance = 1;
                    }
                }
                
                set_color(COLOR_DARK_GRAY);
                print("--- continued ---\n");
                set_color(COLOR_GREEN);
                lines_on_screen++;
            }

            print("  ");
            print(commands[i].help);
            putchar_os('\n');
            lines_on_screen++;
        }
    }
    set_color(base_color);
}

static void cmd_about(void)
{
    set_color(COLOR_WHITE);
    print("====NwOS 1.4.4====\n");
    print("Name: NwOS\n");
    print("Version: v1.4.4\n");
    print("Arch: x86-64\n");
    print("Display: VGA text mode\n");
    print("PS/2 keyboard\n");
    print("Network: e1000 (ping)\n");
    print("================\n");
    set_color(base_color);
}

static void cmd_echo(const char* args)
{
    print(args);
    putchar_os('\n');
}

static void cmd_write(const char* args)
{
    char name[128];
    int i = 0;

    while (args[i] != ' ' && args[i] != '\0' && i < 127)
    {
        name[i] = args[i];
        i++;
    }

    name[i] = '\0';

    if (args[i] != ' ')
    {
        print_error("Usage: write <file> <text>\n");
        return;
    }

    fs_write(name, args + i + 1);
}

static void cmd_panic_safe(void)
{
    KPANIC_SAFE(ERR_NONE, "Manual test panic (safe)");
}

static void cmd_panic_fatal(void)
{
    KPANIC_FATAL(ERR_BAD_STATE, "Manual test panic (fatal)");
}

static int command_name_length(const char* s)
{
    int n = 0;

    while (s[n] != '\0')
        n++;

    return n;
}

static void shell_execute(const char* command)
{
    if (command[0] == '\0')
        return;

    for (unsigned int i = 0; i < COMMAND_COUNT; i++)
    {
        const Command* c = &commands[i];

        if (c->run != 0)
        {
            /* exact-word command */
            if (strcmp(command, c->name) == 0)
            {
                c->run();
                return;
            }

            continue;
        }

        /* command that takes text: "<name> <args>" */
        int n = command_name_length(c->name);

        if (strncmp(command, c->name, n) != 0)
            continue;

        if (command[n] == ' ')
        {
            c->run_arg(command + n + 1);
            return;
        }

        if (command[n] == '\0')
        {
            print_error("Usage: ");
            print_error(c->help ? c->help : c->name);
            putchar_os('\n');
            return;
        }
    }

    print_error("Unknown command. Type 'help'.\n");
}

void set_p_shell(void)
{
    int choose_p = 0;
    print("1.Default (NwOS> )\n2.D disk (D:/>)\n3.C disk (C:/>)\n4.Custom\n");
    print("|\n");
    print("----> ");
    choose_p = read_int();
    if (choose_p == 1) {
        print_success("Set to 'Default'\n");
        shell_p = 0;
        cfg_strcpy(invite_user, "NwOS> ", 28);
    } else if (choose_p == 2) {
        print_success("Set to 'D disk'\n");
        shell_p = 1;
        cfg_strcpy(invite_user, "D:/> ", 28);
    } else if (choose_p == 3) {
        print_success("Set to 'C disk'\n");
        shell_p = 2;
        cfg_strcpy(invite_user, "C:/> ", 28);
    } else if (choose_p == 4) {
        print("Invite: ");
        read_line(invite_user, 28);
        shell_p = 9;
        print_success("Set to custom invite\n");
    } else {
        print_error("Invalid choose!\n");
        return;
    }

    settings_save();
}

static const char* const color_names[16] = {
    "black", "blue", "green", "cyan",
    "red", "magenta", "brown", "light_gray",
    "dark_gray", "light_blue", "light_green", "light_cyan",
    "light_red", "light_magenta", "yellow", "white"
};

static unsigned char color_from_name(const char* name)
{
    for (int i = 0; i < 16; i++)
    {
        if (strcmp(name, color_names[i]) == 0)
            return (unsigned char)i;
    }

    return COLOR_LIGHT_GRAY;
}

static int cfg_find_key(const char* buffer, const char* key, char* out, int out_max)
{
    int klen = 0;
    while (key[klen] != '\0') klen++;

    int i = 0;
    while (buffer[i] != '\0')
    {
        int start = i;

        while (buffer[i] != '\0' && buffer[i] != '\n')
            i++;

        if (i - start > klen && buffer[start + klen] == '=')
        {
            int match = 1;

            for (int k = 0; k < klen; k++)
            {
                if (buffer[start + k] != key[k])
                {
                    match = 0;
                    break;
                }
            }

            if (match)
            {
                int j = start + klen + 1;
                int o = 0;

                while (j < i && buffer[j] != '\r' && o < out_max - 1)
                    out[o++] = buffer[j++];

                out[o] = '\0';
                return 1;
            }
        }

        if (buffer[i] == '\n') i++;
    }

    return 0;
}

void settings_load(void)
{
    char buffer[SETTINGS_MAX];
    char value[64];
    int prev_dir = current_dir;
    int sys_dir = -1;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used && directory[i].type == FS_TYPE_DIR &&
            directory[i].parent == 0 && strcmp(directory[i].name, "sys") == 0)
        {
            sys_dir = i;
            break;
        }
    }

    if (sys_dir == -1)
        return;

    current_dir = sys_dir;

    if (fs_read_text(SETTINGS_FILE, buffer, sizeof(buffer)))
    {
        if (cfg_find_key(buffer, "name", value, sizeof(value)))
            cfg_strcpy(cfg_user, value, sizeof(cfg_user));

        if (cfg_find_key(buffer, "OS", value, sizeof(value)))
            cfg_strcpy(cfg_os, value, sizeof(cfg_os));

        if (cfg_find_key(buffer, "version", value, sizeof(value)))
            cfg_strcpy(cfg_version, value, sizeof(cfg_version));

        if (cfg_find_key(buffer, "prompt", value, sizeof(value)))
        {
            cfg_strcpy(invite_user, value, 28);

            if (strcmp(value, "D:/> ") == 0)
                shell_p = 1;
            else if (strcmp(value, "C:/> ") == 0)
                shell_p = 2;
            else if (strcmp(value, "NwOS> ") == 0)
                shell_p = 0;
            else
                shell_p = 9;
        }

        if (cfg_find_key(buffer, "prompt_color", value, sizeof(value)))
            prompt_color = color_from_name(value);

        if (cfg_find_key(buffer, "color", value, sizeof(value)))
            base_color = color_from_name(value);

        if (cfg_find_key(buffer, "help_color", value, sizeof(value)))
            text_color = color_from_name(value);
    }

    current_dir = prev_dir;
}

static void cfg_append(char* out, int* pos, int max, const char* key, const char* value)
{
    int i = 0;

    while (key[i] != '\0' && *pos < max - 1)
        out[(*pos)++] = key[i++];

    if (*pos < max - 1) out[(*pos)++] = '=';

    i = 0;
    while (value[i] != '\0' && *pos < max - 1)
        out[(*pos)++] = value[i++];

    if (*pos < max - 1) out[(*pos)++] = '\n';

    out[*pos] = '\0';
}

void settings_save(void)
{
    char buffer[SETTINGS_MAX];
    int pos = 0;
    int prev_dir = current_dir;
    int sys_dir = -1;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory[i].used && directory[i].type == FS_TYPE_DIR &&
            directory[i].parent == 0 && strcmp(directory[i].name, "sys") == 0)
        {
            sys_dir = i;
            break;
        }
    }

    if (sys_dir == -1)
        return;

    current_dir = sys_dir;

    buffer[0] = '\0';

    cfg_append(buffer, &pos, sizeof(buffer), "name", cfg_user);
    cfg_append(buffer, &pos, sizeof(buffer), "OS", cfg_os);
    cfg_append(buffer, &pos, sizeof(buffer), "version", cfg_version);
    cfg_append(buffer, &pos, sizeof(buffer), "prompt", invite_user);
    cfg_append(buffer, &pos, sizeof(buffer), "prompt_color", color_names[prompt_color & 0x0F]);
    cfg_append(buffer, &pos, sizeof(buffer), "color", color_names[base_color & 0x0F]);
    cfg_append(buffer, &pos, sizeof(buffer), "help_color", color_names[text_color & 0x0F]);

    fs_write(SETTINGS_FILE, buffer);

    current_dir = prev_dir;
}

void cmd_settings(void)
{
    print_title("\n=== SETTINGS ===\n\n");
    print("user        = "); print(cfg_user);        putchar_os('\n');
    print("os          = "); print(cfg_os);          putchar_os('\n');
    print("version     = "); print(cfg_version);     putchar_os('\n');
    print("prompt      = "); print(invite_user);     putchar_os('\n');
    print("prompt_color= "); print(color_names[prompt_color & 0x0F]); putchar_os('\n');
    print("color       = "); print(color_names[base_color & 0x0F]);   putchar_os('\n');
    print("help_color  = "); print(color_names[text_color & 0x0F]);   putchar_os('\n');
    putchar_os('\n');
}

void change_p_color(void)
{
    int choose_c;

    print("Prompt color:\n");

    set_color(COLOR_CYAN);           print("1. cyan (default)\n");
    set_color(COLOR_RED);            print("2. red\n");
    set_color(COLOR_GREEN);          print("3. green\n");
    set_color(COLOR_BLUE);           print("4. blue\n");
    set_color(COLOR_MAGENTA);        print("5. magenta\n");
    set_color(COLOR_BROWN);          print("6. brown\n");
    set_color(COLOR_LIGHT_GRAY);     print("7. light gray\n");
    set_color(COLOR_DARK_GRAY);      print("8. dark gray\n");
    set_color(COLOR_LIGHT_BLUE);     print("9. light blue\n");
    set_color(COLOR_LIGHT_GREEN);    print("10. light green\n");
    set_color(COLOR_LIGHT_CYAN);     print("11. light cyan\n");
    set_color(COLOR_LIGHT_RED);      print("12. light red\n");
    set_color(COLOR_LIGHT_MAGENTA);  print("13. light magenta\n");
    set_color(COLOR_YELLOW);         print("14. yellow\n");
    set_color(COLOR_WHITE);          print("15. white\n");
    set_color(COLOR_BLACK);          print("16. black\n");

    set_color(base_color);
    print("Color: ");
    choose_c = read_int();

    if (choose_c < 1 || choose_c > 16)
    {
        print_error("Invalid choose!\n");
        return;
    }

    prompt_color = (unsigned char)(choose_c - 1);

    settings_save();

    print_success("Prompt color set\n");
    print("Preview: ");
    set_color(prompt_color);
    print_shell_p();
    set_color(base_color);
    putchar_os('\n');
}

void print_shell_p(void)
{
    if (shell_p == 0) {
        print("NwOS> ");
    } else if (shell_p == 1) {
        print("D:/> ");
    } else if (shell_p == 2) {
        print("C:/> ");
    } else if (shell_p == 9) {
        print(invite_user);
    }
}

void shell(void)
{
    char command[128];

    while (1)
    {
        set_color(prompt_color);
        print_shell_p();
        set_color(base_color);

        read_line(command, 128);

        shell_execute(command);
    }
}

void run_gui(void) {
    if (gui_bin_len == 0) {
        print_error("Error: GUI binary is empty!\n");
        return;
    }
    
    print("Loading GUI (");
    print_int(gui_bin_len);
    print(" bytes)...\n");
    sleep(500);

    uint8_t* gui_mem = (uint8_t*)0x400000;
    for (unsigned int i = 0; i < gui_bin_len; i++) {
        gui_mem[i] = gui_bin[i];
    }
    
    gui_api_t api;
    api.draw_rect = fb_draw_rect;
    api.draw_char = fb_draw_char;
    api.getkey = keyboard_getkey;
    api.sleep = sleep;

    extern unsigned char inb(unsigned short port);
    while (inb(0x64) & 1) {
        (void)inb(0x60);
    }

    __asm__ volatile ("cli");
    
    typedef void (*gui_entry_t)(gui_api_t*);
    gui_entry_t gui_start = (gui_entry_t)0x400000;
    
    gui_start(&api);

    __asm__ volatile ("sti");
    
    clear();
    print("GUI closed.\n");
}

void installer(void) {
    clear();
    fb_init();
    set_color(COLOR_CYAN);
    print("\n");
    print("  +======================================+\n");
    print("  |     NwOS Installation Wizard 1.4.4   |\n");
    print("  +======================================+\n\n");
    set_color(base_color);
    
    print("  Welcome! No filesystem was found on the primary disk.\n\n");
    
    int n = ata_drive_count();
    print_title("  Detected disks:\n");
    for (int i = 0; i < n; i++) {
        ata_drive_t* d = ata_get_drive(i);
        print("    ");
        putchar_os(d->letter);
        print(": ");
        print(d->model);
        print(" (");
        print_int(d->sectors / 2048);
        print(" MB)\n");
    }
    
    print("\n  This wizard will erase ALL data on C: drive.\n");
    print("  Continue? (y/n): ");
    
    char buf[8];
    read_line(buf, 8);
    
    if (buf[0] != 'y' && buf[0] != 'Y') {
        print_error("\n  Cancelled. System halted.\n");
        while(1) __asm__ volatile("hlt");
    }
    
    print_success("\n\n  Formatting C: ...\n");
    sleep(500);
    fs_format();
    
    print_success("\n  Installation complete!\n");
    print("  Rebooting in 3 seconds...\n");
    sleep(3000);
    reboot();
}

void dead_boot(void) {
    clear();
    
    if (fb_is_available()) {
        fb_draw_image((640 - LOGO_NWOS_WIDTH) / 2, 40, LogoNwOS, LOGO_NWOS_WIDTH, LOGO_NWOS_HEIGHT);
    }
    
    while (1) __asm__ volatile("hlt");
}

void kernel_main(void) {
    fb_init();
    
    if (!fb_is_available()) {
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        
        for (int i = 0; i < 80 * 25; i++) vga[i] = 0x4F20;

        const char* msg1 = " FATAL ERROR: Framebuffer not found! ";
        for (int i = 0; msg1[i]; i++) vga[2 * 80 + 10 + i] = (uint16_t)msg1[i] | (0x4F << 8);

        const char* msg2 = " Check GRUB config (set gfxpayload=640x400x32). ";
        for (int i = 0; msg2[i]; i++) vga[4 * 80 + 5 + i] = (uint16_t)msg2[i] | (0x4F << 8);
        
        const char* msg3 = " If booting from HDD, try NwOS.iso instead. ";
        for (int i = 0; msg3[i]; i++) vga[6 * 80 + 5 + i] = (uint16_t)msg3[i] | (0x4F << 8);

        while (1) __asm__ volatile ("hlt");
    }

    ata_init();

    if (ata_drive_count() == 0) {
        clear();
        print_error("\n\nNo ATA disks detected!\n\n");
        print("Add one to QEMU: -drive file=disk.img,format=raw,if=ide,index=0\n");
        while(1) __asm__ volatile("hlt");
    }

    clear();

    int status = fs_load_status();

    if (status == 0) {
        installer();
        return;
    }
    
    if (status == 2) {
        dead_boot();
    }

    print("CLEARING...\n");
    startingNWOS();
    settings_load();
    print("Starting NwOS...\n");
    sleep(500);
    print_success("Kernel Started!\n");
    print("Clearing..\n");

    sleep(1500);

    clear();

    fb_draw_image(288, 198, LogoNwOS, LOGO_NWOS_WIDTH, LOGO_NWOS_HEIGHT);

    sleep(1500);

    print("================================\n");
    print("        Welcome to NwOS 1.4.4\n");
    print("================================\n\n");
    set_color(COLOR_LIGHT_GREEN);
    print("copyright (c) User014015  2026\n\n");
    set_color(COLOR_LIGHT_GRAY);
    print("Github: https://github.com/User014015/NwOS"); print("\n");
    print("Scroll Up/Down: ⬆️⬇️\n");
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
