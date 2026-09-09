#include "drivers/keyboard.h"
#include "drivers/disk.h"

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
void game_coin(void);
void game_dice(void);
void game_higher_lower(void);
void game_math(void);
void fs_save_directory(void);
void fs_load(void);
void *memcpy(void *dest, const void *src, unsigned int n);

void games_menu(void);
int strncmp(const char* a, const char* b, int n);

void reboot(void);

typedef unsigned short uint16_t;

#define WIDTH 80
#define HEIGHT 25

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 256

/*
 * Disk-backed filesystem layout.
 *
 * boot.asm now loads up to 130 sectors for the kernel (LBA 1-130),
 * so the filesystem area starts at LBA 200 to leave a safe margin
 * for the kernel to keep growing. Make sure the .img is padded to
 * at least (FS_DATA_LBA + MAX_FILES) sectors, i.e. 218 sectors /
 * ~109 KB - padding to a full 1.44MB floppy (2880 sectors) is a
 * safe, simple choice with plenty of headroom.
 *
 *   LBA 200-201 : directory table (used flag, name, size per file)
 *   LBA 202-217 : file data, one 512-byte sector per file slot
 */
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

/*
 * base_color is the user's chosen "default" terminal color
 * (set via the color command). All the print_* helpers and
 * the shell prompt restore to base_color instead of a
 * hardcoded gray, so the chosen color actually sticks.
 */
static unsigned char base_color = COLOR_LIGHT_GRAY;

void fs_load(void);
void fs_save_directory(void);

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

/*
 * Sets both the active color and the persistent base color,
 * so future resets (prompt, print_error/success/title, etc.)
 * fall back to this color instead of gray.
 */
void set_base_color(unsigned char color)
{
    base_color = color;
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


void games(void)
{
    print("\nGames:\n");

    print("  guess - Guess the Number\n");
    print("  guessh - Guess number (hard)\n");
    print("  rps   - Rock Paper Scissors\n");
    print("  word - Guess word\n");
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
    print("2026.08.27");
    print("v.1.3.6");
    set_color(base_color);
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

        if (strcmp(answer, "rust") == 0)
        {
            print_success("Correct! You win!\n");
            return;
        }

        print_error("Wrong answer!\n");
    }

    print_error("The answer was: rust\n");
}

void nwfetch(void)
{
    print_title("\n");

    print("       __  __       ");
    print_success("   NwOS\n");

    print("      |  \\|  |      ");
    print_success("   Version: 1.3.6\n");

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


/* =========================
   Filesystem (stored on disk)
   ========================= */

typedef struct __attribute__((packed))
{
    unsigned char used;
    char name[MAX_FILENAME];
    unsigned int size;
} DirEntry;

static DirEntry directory[MAX_FILES];

/*
 * Loads the directory table from disk into memory. Call once
 * at boot so files created in earlier sessions show up again.
 */
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

        /*
         * A blank / never-formatted image reads back as garbage,
         * not necessarily zero. Only trust "used" if it's exactly
         * 1, so noise on disk doesn't look like real files.
         */
        if (directory[i].used != 1)
        {
            directory[i].used = 0;
            directory[i].name[0] = '\0';
            directory[i].size = 0;
        }
    }
}

/*
 * Writes the whole directory table back to disk. Called after
 * every create/write/edit/delete so changes survive a reboot.
 */
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

/*
 * Wipes the filesystem area on disk. Use this once on a fresh
 * image, or to recover from a corrupted directory table.
 */
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
        else
        {
            print_error("\nInvalid choice.\n");
        }
    }
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
        print_error("Text is too long. Maximum is 255 characters.\n");
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

// colors

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
            print("  create <file> - create file\n");
            print("  read <file> - read file\n");
            print("  write <file> <txt> - write file\n");
            print("  edit <file> <txt> - edit file\n");
            print("  delete <file> - delete file\n");
            print("  reboot - restart NwOS\n");
            set_color(base_color);
        }

        else if (strcmp(command, "clear") == 0)
        {
            clear();
        }

        else if (strcmp(command, "about") == 0)
        {
            set_color(COLOR_WHITE);
            print("====NwOS 1.3.6====\n");
            print("Name: NwOS\n");
            print("Version: v1.3.6\n");
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
        else if (strncmp(command, "edit ", 5) == 0)
        {
            char* separator = command + 5;

            while (*separator != ' ' &&
                   *separator != '\0')
            {
                separator++;
            }

            if (*separator == '\0')
            {
                print_error("Usage: edit <file> <text>\n");
            }
            else
            {
                *separator = '\0';

                fs_edit(command + 5,
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
    fs_load();

    print("================================\n");
    print("        Welcome to NwOS 1.3.6\n");
    print("================================\n");
    set_color(COLOR_GREEN);
    print("Keyboard: OK\n");
    set_color(COLOR_LIGHT_GRAY);
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