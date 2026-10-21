#include "../../drivers/keyboard.h"
#include "../../drivers/disk.h"
#include "../../include/kernelpanic.h"

#include "Apps/wordgenerator/wordgenerator.h"
#include "Apps/textredactor/textredactor.h"
#include "nwoloader/nwo_vm.h"
#include "nwoloader/nwo_runner.h"

#include "Apps/Games/slot.h"
#include "Apps/Games/game_memory.h"
#include "Apps/Games/snake.h"
#include "kernel.h"

#include "Apps/fileStats/filestats.h"
#include "Apps/baseconverter/baseconverter.h"
#include "rootfs_data.h"

#include "../../include/gui_embed.h"

#include "network/net.h"

#include "nwo_loader.h"

#include "network/timer.h"
#include "zones.h"

#include "../../include/LogoNwOS.h"

#include "fb.h"

#include <stdint.h>

// NWOS VERSIONS
#define OS_NAME            "NwOS"
#define OS_VERSION_MAJOR   1
#define OS_VERSION_MINOR   4
#define OS_VERSION_PATCH   5
#define OS_MADE_DATE      "2026.08.27"

#define OS_STR_HELPER(x)   #x
#define OS_STR(x)          OS_STR_HELPER(x)

#define OS_VERSION_STR     OS_STR(OS_VERSION_MAJOR) "." \
                          OS_STR(OS_VERSION_MINOR) "." \
                          OS_STR(OS_VERSION_PATCH)

void disk_death(void);
void game_snake(void);

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
void fs_rmdir(const char* name);
void fs_rm(const char* args);
void clear(void);
void unknow_panic(unsigned int code);
void nwo_run(const char* filename);
void nwf_cpu_name(char* out);
unsigned int nwf_ram_mb(void);
void read_line(char* buffer, int max);

unsigned int rand_simple(void);
void set_p_shell(void);
void print_shell_p(void);
void change_p_color(void);
static void cmd_more_help(void);
void print_title(const char* text);
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

void fs_set_drive(int idx);
int  fs_get_drive(void);
void fs_format_disk(int drive_idx);
void installer(void);

static void print_size_bytes(unsigned long long bytes);

static int installer_cancelled = 0;

static void cmd_install(void);

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






void generate_num(void);

void games_menu(void);
int strncmp(const char* a, const char* b, int n);
void nwo_run(const char* filename);


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
#define FS_SEQ_OFFSET  4           /* install sequence inside the magic sector */

static DirEntry directory[MAX_FILES];
static uint16_t current_dir = 0;

static unsigned char text_color = COLOR_LIGHT_GRAY;
static unsigned char base_color = COLOR_LIGHT_GRAY;
static unsigned char prompt_color = COLOR_CYAN;

static int timezone_offset = 0; /* UTC+0 by default */
static int custom_time_offset = 0;
static int use_custom_time = 0;
static int custom_day = -1;
static int custom_month = -1;
static int custom_year = -1;

void fs_load(void);
void fs_save_directory(void);

int shell_p = 0;
char invite_user[28] = OS_NAME "> ";

#define SETTINGS_FILE "sys.conf"
#define SETTINGS_MAX  2048

char cfg_user[32] = "user";
char cfg_os[32] = OS_NAME;

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

    for (int row = 0; row < pad; row++)
        for (int x = 0; x < WIDTH; x++)
            fb_draw_char(x, row, ' ', COLOR_LIGHT_GRAY);

    for (int i = 0; i < shown; i++)
    {
        int idx = (line_start + first + i) % SCROLLBACK_LINES;
        for (int x = 0; x < WIDTH; x++)
        {
            uint16_t cell = line_buffer[idx][x];
            fb_draw_char(x, pad + i, cell & 0xFF, (cell >> 8) & 0xFF);
        }
    }
    
    int cy = pad + shown - 1;
    if (cy >= 0 && cy < HEIGHT) {
        fb_draw_char(cursor_col, cy, '_', COLOR_WHITE);
    }
}
static void cmd_mem(void)
{
    unsigned int ram_mb = nwf_ram_mb();
    print_title("Memory Information:\n");
    print("  Total RAM: ");
    print_int((int)ram_mb);
    print(" MiB\n");
    print("  Memory model: 32-bit\n");
    putchar_os('\n');
}

static void cmd_cpu(void)
{
    char cpu_name[49];
    nwf_cpu_name(cpu_name);
    print_title("CPU Information:\n");
    print("  CPU: ");
    print(cpu_name);
    putchar_os('\n');
    print("  Architecture: x86_64\n");
    putchar_os('\n');
}

static void cmd_uptime(void)
{
    unsigned long long total_sec = timer_ms() / 1000;
    unsigned long long up_m = total_sec / 60;
    unsigned long long up_h = up_m / 60;
    up_m %= 60;
    total_sec %= 60;
    print_title("Uptime:\n");
    print("  ");
    if (up_h > 0) { print_int((int)up_h); print("h "); }
    if (up_m > 0) { print_int((int)up_m); print("m "); }
    print_int((int)total_sec); print("s\n");
    putchar_os('\n');
}

static void cmd_zones(void)
{
    print_title("Time Zones (all 24h worldwide):\n");
    for (int i = 0; i < TIME_ZONE_COUNT; i++)
    {
        print_int(i + 1);
        print(". ");
        print(time_zone_names[i]);
        if (timezone_offset == time_zone_offsets[i])
            print(" [ACTIVE]");
        putchar_os('\n');
    }
    putchar_os('\n');
    print("Use 'timezone <num>' to set zone.\n");
    print("Use 'date <DD> <MM> <YYYY>' to set custom date.\n");
    print("Use 'date reset' to reset date to RTC.\n\n");
}

static void cmd_timezone(const char* args)
{
    if (args[0] == '\0')
    {
        print("Usage: timezone <num> (1-");
        print_int(TIME_ZONE_COUNT);
        print(")\n");
        return;
    }
    int num = atoi_simple(args);
    if (num < 1 || num > TIME_ZONE_COUNT)
    {
        print_error("Invalid timezone number.\n");
        return;
    }
    timezone_offset = time_zone_offsets[num - 1];
    print_success("Timezone set to: ");
    print(time_zone_names[num - 1]);
    putchar_os('\n');
}

static void cmd_date_set(const char* args)
{
    if (args[0] == '\0')
    {
        print("Usage: date <DD> <MM> <YYYY> or date reset\n");
        return;
    }
    if (strcmp(args, "reset") == 0)
    {
        custom_day = -1;
        custom_month = -1;
        custom_year = -1;
        print_success("Date reset to RTC.\n");
        return;
    }
    char d[16], m[16], y[16];
    int i = 0, j = 0;
    while (args[i] == ' ') i++;
    while (args[i] != ' ' && args[i] != '\0' && j < 15) { d[j++] = args[i++]; }
    d[j] = '\0';
    while (args[i] == ' ') i++;
    j = 0;
    while (args[i] != ' ' && args[i] != '\0' && j < 15) { m[j++] = args[i++]; }
    m[j] = '\0';
    while (args[i] == ' ') i++;
    j = 0;
    while (args[i] != '\0' && j < 15) { y[j++] = args[i++]; }
    y[j] = '\0';
    
    int dd = atoi_simple(d);
    int mm = atoi_simple(m);
    int yy = atoi_simple(y);
    
    if (dd < 1 || dd > 31 || mm < 1 || mm > 12 || yy < 0)
    {
        print_error("Invalid date.\n");
        return;
    }
    
    custom_day = dd;
    custom_month = mm;
    custom_year = yy;
    print_success("Date set to ");
    print_int(dd); print("."); print_int(mm); print("."); print_int(yy);
    print(" (OS will display this date).\n");
}

static void cmd_time_cmd(void)
{
    int hour, min, sec;
    int day, month, year;
    get_time(&hour, &min, &sec);
    get_date(&day, &month, &year);
    print_title("Current Time/Date:\n");
    print("  Time: ");
    print_two_digits(hour); putchar_os(':');
    print_two_digits(min); putchar_os(':');
    print_two_digits(sec);
    putchar_os('\n');
    print("  Date: ");
    print_two_digits(day); putchar_os('.');
    print_two_digits(month); putchar_os('.');
    print_int(year);
    putchar_os('\n');
    print("  Zone: UTC");
    if (timezone_offset >= 0) putchar_os('+');
    print_int(timezone_offset);
    putchar_os('\n');
    if (custom_day != -1)
        print("  (Custom date active)\n");
    putchar_os('\n');
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

void wait_key(void)
{
    print("\nPress any key to continue...\n");
    keyboard_getkey();
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
            wait_key();
            return;
        }

        int number = atoi_simple(buffer);

        if (number == secret)
        {
            print_success("Correct! You win!\n");
            wait_key();
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
    wait_key();
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
            wait_key();
            return;
        }

        int number = atoi_simple(bufferh);

        if (number == secreth)
        {
            print_success("Correct! You win!\n");
            wait_key();
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
    wait_key();
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
    wait_key();
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
    wait_key();
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
    wait_key();
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
            wait_key();
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
    wait_key();
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
            wait_key();
            return;
        }

        if (ttt_board_full(board))
        {
            ttt_print_board(board);
            print("It's a draw!\n\n");
            wait_key();
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
            wait_key();
            return;
        }

        if (ttt_board_full(board))
        {
            ttt_print_board(board);
            print("It's a draw!\n\n");
            wait_key();
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

#define SIZE_KB 1024ULL
#define SIZE_MB (1024ULL * 1024ULL)
#define SIZE_GB (1024ULL * 1024ULL * 1024ULL)

void print_uint64(unsigned long long value)
{
    char buffer[24];
    int i = 0;

    if (value == 0)
    {
        putchar_os('0');
        return;
    }

    while (value > 0 && i < 24)
    {
        buffer[i++] = (char)('0' + (int)(value % 10));
        value /= 10;
    }

    while (i > 0)
    {
        i--;
        putchar_os(buffer[i]);
    }
}

static void print_size_unit(
    unsigned long long bytes,
    unsigned long long unit,
    int decimals,
    const char* suffix
)
{
    static const unsigned long long pow10[3] = { 1ULL, 10ULL, 100ULL };

    unsigned long long whole = bytes / unit;
    unsigned long long frac = 0;

    if (decimals > 0 && decimals <= 3)
    {
        frac = ((bytes % unit) * pow10[decimals - 1]) / unit;
    }

    print_uint64(whole);

    if (decimals > 0)
    {
        putchar_os('.');

        for (int i = decimals - 1; i >= 0; i--)
        {
            putchar_os((char)('0' + (int)((frac / pow10[i]) % 10)));
        }
    }

    print(" ");
    print(suffix);
}

static void print_size_human(unsigned long long bytes)
{
    if (bytes >= SIZE_GB)
        print_size_unit(bytes, SIZE_GB, 2, "GB");
    else if (bytes >= SIZE_MB)
        print_size_unit(bytes, SIZE_MB, 2, "MB");
    else if (bytes >= SIZE_KB)
        print_size_unit(bytes, SIZE_KB, 2, "KB");
    else
    {
        print_uint64(bytes);
        print(" B");
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
    print(OS_MADE_DATE "\n");
    print("v." OS_VERSION_STR "\n");
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
            wait_key();
            return;
        }

        print_error("Wrong answer!\n");
    }

    print_error("The answer was: Rust\n");
    wait_key();
}
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
    if (use_custom_time) {
        /* For custom time, just use RTC + offsets if set */
    }
    while (rtc_read(0x0A) & 0x80); 
    
    *second = bcd_to_bin(rtc_read(0x00));
    *minute = bcd_to_bin(rtc_read(0x02));
    *hour   = bcd_to_bin(rtc_read(0x04));
    
    int total = *hour + timezone_offset + custom_time_offset;
    while (total < 0) total += 24;
    while (total >= 24) total -= 24;
    *hour = total;
}

void get_date(int* day, int* month, int* year) {
    while (rtc_read(0x0A) & 0x80);
    
    if (custom_day != -1 && custom_month != -1 && custom_year != -1) {
        *day = custom_day;
        *month = custom_month;
        *year = custom_year;
        return;
    }
    
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

void nwf_cpu_name(char* out)
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

unsigned int nwf_ram_mb(void)
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
                set_color(COLOR_WHITE); print(cfg_os); print(" "); print(OS_VERSION_STR); print(" x86_64");
                break;

            case 3:
                set_color(COLOR_CYAN);  print("Kernel: ");
                set_color(COLOR_WHITE); print(OS_NAME " 64-bit");
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
                set_color(COLOR_WHITE); print(OS_NAME " Console");
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

static int fs_current_drive = 0;
static int fs_quiet = 0;

void fs_set_drive(int idx)
{
    if (idx < 0 || idx >= ata_drive_count()) return;
    fs_current_drive = idx;
}

int fs_get_drive(void)
{
    return fs_current_drive;
}

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
        if (ata_read_sector_d(fs_current_drive, FS_DIR_LBA + s, buffer + s * 512) != 0) {
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
        ata_write_sector_d(fs_current_drive, FS_DIR_LBA + s, buffer + s * 512);
    }
}
static int fs_drive_has_fs(int idx)
{
    unsigned char sector[512];

    if (idx < 0 || idx >= ata_drive_count()) return 0;
    if (ata_read_sector_d(idx, FS_MAGIC_LBA, sector) != 0) return 0;

    return *(unsigned int*)sector == FS_MAGIC_VALUE;
}

static const char* disk_position_name(ata_drive_t* d)
{
    if (d->channel == 0 && d->master) return "primary master";
    if (d->channel == 0)               return "primary slave";
    if (d->master)                     return "secondary master";

    return "secondary slave";
}

int fs_drive_index_by_letter(char letter)
{
    int n = ata_drive_count();

    if (letter >= 'a' && letter <= 'z')
        letter = (char)(letter - 'a' + 'A');

    for (int i = 0; i < n; i++)
    {
        ata_drive_t* d = ata_get_drive(i);

        if (d && d->letter == letter)
            return i;
    }

    return -1;
}

char fs_get_drive_letter(void)
{
    ata_drive_t* d = ata_get_drive(fs_current_drive);

    if (d) return d->letter;

    return 'C';
}

/*
 * Switches the active disk (the one every file operation works on).
 * The new disk must contain a NwOS filesystem, otherwise nothing changes.
 * Returns 0 on success, -1 on a bad index and -2 if the disk has no filesystem.
 */
int fs_change_drive(int idx)
{
    if (idx < 0 || idx >= ata_drive_count()) return -1;

    if (!fs_drive_has_fs(idx)) return -2;

    if (idx != fs_current_drive)
    {
        fs_current_drive = idx;
        current_dir = 0;

        fs_load();
    }

    return 0;
}

int fs_change_drive_letter(char letter)
{
    int idx = fs_drive_index_by_letter(letter);

    if (idx < 0) return -1;

    return fs_change_drive(idx);
}

static unsigned char fs_scratch[FS_DIR_SECTORS * 512];

static int fs_read_directory_of(int idx, DirEntry* out)
{
    if (idx < 0 || idx >= ata_drive_count()) return -1;

    for (int s = 0; s < FS_DIR_SECTORS; s++)
    {
        if (ata_read_sector_d(idx, FS_DIR_LBA + s, fs_scratch + s * 512) != 0)
            return -1;
    }

    DirEntry* disk_entries = (DirEntry*)fs_scratch;

    for (int i = 0; i < MAX_FILES; i++)
    {
        out[i] = disk_entries[i];

        if (out[i].used != 1)
        {
            out[i].used = 0;
            out[i].name[0] = '\0';
            out[i].size = 0;
        }
    }

    return 0;
}

static unsigned long long fs_drive_used_bytes(int idx, unsigned int* file_count)
{
    static DirEntry entries[MAX_FILES];

    unsigned long long total = 0;
    unsigned int files = 0;

    if (file_count) *file_count = 0;

    if (fs_read_directory_of(idx, entries) != 0) return 0;

    for (int i = 1; i < MAX_FILES; i++)
    {
        if (!entries[i].used) continue;
        if (entries[i].type != FS_TYPE_FILE) continue;

        total += entries[i].size;
        files++;
    }

    if (file_count) *file_count = files;

    return total;
}

static unsigned int fs_drive_entry_count(int idx)
{
    static DirEntry entries[MAX_FILES];

    unsigned int count = 0;

    if (fs_read_directory_of(idx, entries) != 0) return 0;

    for (int i = 1; i < MAX_FILES; i++)
    {
        if (entries[i].used) count++;
    }

    return count;
}

static unsigned int fs_drive_seq(int idx)
{
    unsigned char sector[512];

    if (!fs_drive_has_fs(idx)) return 0;

    if (ata_read_sector_d(idx, FS_MAGIC_LBA, sector) != 0) return 0;

    return *(unsigned int*)(sector + FS_SEQ_OFFSET);
}

static unsigned int fs_max_seq(void)
{
    unsigned int max = 0;

    for (int i = 0; i < ata_drive_count(); i++)
    {
        unsigned int seq = fs_drive_seq(i);

        if (seq > max) max = seq;
    }

    return max;
}

static int fs_find_drive_with_fs(void)
{
    int n = ata_drive_count();
    int best = -1;
    unsigned int best_seq = 0;

    for (int i = 0; i < n; i++)
    {
        if (!fs_drive_has_fs(i)) continue;

        unsigned int seq = fs_drive_seq(i);

        if (best < 0 || seq > best_seq)
        {
            best = i;
            best_seq = seq;
        }
    }

    return best;
}

static int fs_is_formatted(void) {
    unsigned char sector[512];
    if (ata_read_sector_d(fs_current_drive, FS_MAGIC_LBA, sector) != 0) return 0;
    return *(unsigned int*)sector == FS_MAGIC_VALUE;
}

static void fs_mark_formatted(unsigned int seq) {
    unsigned char sector[512];
    for (int k = 0; k < 512; k++) sector[k] = 0;
    *(unsigned int*)sector = FS_MAGIC_VALUE;
    *(unsigned int*)(sector + FS_SEQ_OFFSET) = seq;
    ata_write_sector_d(fs_current_drive, FS_MAGIC_LBA, sector);
}

static void fs_wipe_magic(void) {
    unsigned char sector[512];
    for (int k = 0; k < 512; k++) sector[k] = 0;
    *(unsigned int*)sector = 0xDEADBEEF;
    ata_write_sector_d(fs_current_drive, FS_MAGIC_LBA, sector);
}

int fs_load_status(void) {
    if (ata_drive_count() == 0) return 0;

    int drive = fs_find_drive_with_fs();

    if (drive < 0) return 0;

    fs_set_drive(drive);
    
    fs_load();
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && directory[i].parent == 0 &&
            strcmp(directory[i].name, "death.bsod") == 0) {
            return 2;
        }
    }
    return 1;
}
void fs_format_disk(int drive_idx) {
    unsigned char zero[512];
    for (int k = 0; k < 512; k++) zero[k] = 0;

    fs_set_drive(drive_idx);
    fs_quiet = 1;

    unsigned int seq = fs_max_seq() + 1;

    for (int i = 0; i < MAX_FILES; i++) {
        ata_write_sector_d(fs_current_drive, FS_DATA_LBA + i, zero);
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

        print("    [");
        print_int(i + 1);
        print("/");
        print_int(ROOTFS_FILE_COUNT);
        print("] ");
        print(path);
        print("  (");
        print_int((int)size);
        print(" bytes)\n");

        if (fs_find(file_name) == -1) {
            fs_create(file_name);
            int idx = fs_find(file_name);
            if (idx != -1) {
                unsigned char sector[512];
                for (int k = 0; k < 512; k++) sector[k] = 0;
                for (unsigned int k = 0; k < size && k < 511; k++) sector[k] = data[k];
                ata_write_sector_d(fs_current_drive, FS_DATA_LBA + idx, sector);
                directory[idx].size = size;
            }
        }
    }

    current_dir = 0;

    fs_mark_formatted(seq);
    fs_save_directory();

    fs_quiet = 0;
    
    print_success("Filesystem formatted. Rootfs installed.\n");
}

void fs_format(void) {
    fs_format_disk(fs_current_drive);
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
            ata_write_sector_d(fs_current_drive, FS_DATA_LBA + i, empty);

            fs_save_directory();
            if (!fs_quiet) print_success("File created.\n");
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
            if (!fs_quiet) print_success("Directory created.\n");
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

#define MENU_INNER 34

static void menu_border(void)
{
    print("                    +");
    for (int i = 0; i < MENU_INNER; i++) putchar_os('-');
    print("+\n");
}

static void menu_line(const char* text)
{
    print("                    |");
    int n = 0;
    while (text[n]) { putchar_os(text[n]); n++; }
    while (n < MENU_INNER) { putchar_os(' '); n++; }
    print("|\n");
}

void games_menu(void)
{
    char choice[8];

    while (1)
    {
        clear();

        print("\n\n");
        set_color(COLOR_CYAN);
        menu_border();
        menu_line("            NWOS ARCADE");
        menu_border();
        set_color(base_color);
        print("\n");

        menu_border();
        menu_line("           GAMES MENU");
        menu_border();
        menu_line("  [1]  Guess Number");
        menu_line("  [2]  Guess Word");
        menu_line("  [3]  Rock Paper Scissors");
        menu_line("  [4]  Coin Flip");
        menu_line("  [5]  Dice Roll");
        menu_line("  [6]  Higher / Lower");
        menu_line("  [7]  Math Quiz");
        menu_line("  [8]  Hangman");
        menu_line("  [9]  Tic-Tac-Toe");
        menu_line("  [10] Memory Match");
        menu_line("  [11] Slot Machine");
        menu_line("  [12] Snake");
        menu_line("  [13] Guess Hard");
        menu_border();
        menu_line("  [0]  Back to Shell");
        menu_border();
        print("\n");

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
        else if (strcmp(choice, "12") == 0)
        {
            clear();
            game_snake();
        }
        else if (strcmp(choice, "13") == 0)
        {
            clear();
            game_guessh();
        }
        else
        {
            print_error("\nInvalid choice.\n");
            sleep(1000);
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

    ata_write_sector_d(fs_current_drive, FS_DATA_LBA + index, buffer);
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

    ata_read_sector_d(fs_current_drive, FS_DATA_LBA + index, buffer);
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

    if (directory[index].type == FS_TYPE_DIR)
    {
        print_error("Is a directory. Use 'rmdir' to remove it.\n");
        return;
    }

    directory[index].used = 0;

    directory[index].name[0] = '\0';
    directory[index].size = 0;

    fs_save_directory();

    print_success("File deleted.\n");
}

static uint16_t fs_removed[MAX_FILES];
static int fs_removed_count = 0;

static void fs_entry_release(int index)
{
    if (fs_removed_count < MAX_FILES)
        fs_removed[fs_removed_count++] = (uint16_t)index;

    directory[index].used = 0;
    directory[index].type = 0;
    directory[index].parent = 0;
    directory[index].name[0] = '\0';
    directory[index].size = 0;
}

/* frees a directory together with everything inside it */
static void fs_entry_release_tree(int index, int depth)
{
    if (depth > 32) return;

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (i != index && directory[i].used &&
            directory[i].parent == index)
        {
            fs_entry_release_tree(i, depth + 1);
        }
    }

    fs_entry_release(index);
}

/* if the shell was inside the removed tree, go back to its parent */
static void fs_release_finish(uint16_t fallback)
{
    for (int i = 0; i < fs_removed_count; i++)
    {
        if (current_dir == fs_removed[i])
        {
            current_dir = fallback;
            return;
        }
    }
}

/* copies a name into buf and drops the trailing slashes */
static int fs_name_copy(const char* name, char* buf, int max)
{
    int n = 0;

    while (name[n] != '\0' && n < max - 1) n++;

    while (n > 0 && name[n - 1] == '/') n--;

    for (int i = 0; i < n; i++) buf[i] = name[i];

    buf[n] = '\0';

    return n;
}

void fs_rmdir(const char* name)
{
    char buf[MAX_FILENAME + 1];

    if (name == 0 || name[0] == '\0')
    {
        print_error("Usage: rmdir <directory>\n");
        return;
    }

    if (fs_name_copy(name, buf, sizeof(buf)) == 0)
    {
        print_error("Cannot remove the root directory.\n");
        return;
    }

    int index = fs_find(buf);

    if (index == -1)
    {
        print_error("Directory not found.\n");
        return;
    }

    if (index == 0)
    {
        print_error("Cannot remove the root directory.\n");
        return;
    }

    if (directory[index].type != FS_TYPE_DIR)
    {
        print_error("Not a directory. Use 'rm' to remove files.\n");
        return;
    }

    uint16_t parent = directory[index].parent;

    fs_removed_count = 0;
    fs_entry_release_tree(index, 0);
    fs_release_finish(parent);

    fs_save_directory();

    print_success("Directory removed.\n");
}

void fs_rm(const char* args)
{
    char buf[MAX_FILENAME + 1];
    int recursive = 0;
    int force = 0;
    int i = 0;
    int n = 0;

    if (args == 0) args = "";

    while (args[i] == ' ') i++;

    while (args[i] == '-')
    {
        i++;

        while (args[i] != '\0' && args[i] != ' ')
        {
            if (args[i] == 'r' || args[i] == 'R') recursive = 1;
            else if (args[i] == 'f' || args[i] == 'F') force = 1;
            i++;
        }

        while (args[i] == ' ') i++;
    }

    while (args[i] != '\0' && args[i] != ' ' && n < MAX_FILENAME)
    {
        buf[n++] = args[i];
        i++;
    }

    buf[n] = '\0';

    while (n > 0 && buf[n - 1] == '/') buf[--n] = '\0';

    if (n == 0)
    {
        print_error("Usage: rm [-r] [-f] <file>\n");
        return;
    }

    int index = fs_find(buf);

    if (index == -1)
    {
        if (force) return;
        print_error("No such file or directory.\n");
        return;
    }

    if (index == 0)
    {
        print_error("Cannot remove the root directory.\n");
        return;
    }

    if (directory[index].type == FS_TYPE_DIR)
    {
        if (!recursive)
        {
            print_error("Is a directory. Use 'rm -r' or 'rmdir'.\n");
            return;
        }

        uint16_t parent = directory[index].parent;

        fs_removed_count = 0;
        fs_entry_release_tree(index, 0);
        fs_release_finish(parent);

        fs_save_directory();

        print_success("Directory removed.\n");
        return;
    }

    fs_removed_count = 0;
    fs_entry_release(index);

    fs_save_directory();

    print_success("File removed.\n");
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

    ata_read_sector_d(
        fs_current_drive,
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

    ata_read_sector_d(
        fs_current_drive,
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
static void cmd_mem(void);
static void cmd_cpu(void);
static void cmd_uptime(void);
static void cmd_time_cmd(void);
static void cmd_zones(void);
static void cmd_timezone(const char* args);
static void cmd_date_set(const char* args);
static void cmd_more_help(void);
static void cmd_drive(const char* args);
static void cmd_drive_status(void);
static void cmd_disk_info(const char* args);


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

    for (int i = 0; i < n; i++) {
        ata_drive_t* d = ata_get_drive(i);
        unsigned long long total = (unsigned long long)d->sectors * 512ULL;

        if (i == fs_current_drive) set_color(COLOR_LIGHT_GREEN);
        else set_color(base_color);

        putchar_os(d->letter);
        print(":");

        if (i == fs_current_drive) print(" *");
        else print("  ");

        set_color(base_color);

        print("  ");
        print(disk_position_name(d));
        print("  ");

        print_uint64(total);
        print(" B  ");

        print_size_human(total);

        if (fs_drive_has_fs(i))
        {
            set_color(COLOR_LIGHT_CYAN);
            print("  [");
            print(OS_NAME);
            print("]");
            set_color(base_color);
        }

        putchar_os('\n');
    }

    putchar_os('\n');

    print("  * = active disk\n");
    print("  'drive <letter|number>' switches the active disk\n");
    print("  'disk <letter|number>' shows the detailed size of one disk\n");
    putchar_os('\n');
}

static int resolve_drive_arg(const char* args)
{
    int n = ata_drive_count();

    if (args == 0 || args[0] == '\0') return fs_current_drive;

    char c = args[0];

    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
    {
        return fs_drive_index_by_letter(c);
    }

    if (c >= '0' && c <= '9')
    {
        int num = atoi_simple(args);

        if (num >= 1 && num <= n) return num - 1;
    }

    return -1;
}

static void cmd_drive(const char* args)
{
    if (ata_drive_count() == 0)
    {
        print_error("No ATA disks detected.\n");
        return;
    }

    if (args == 0 || args[0] == '\0')
    {
        ata_drive_t* d = ata_get_drive(fs_current_drive);
        unsigned long long total = (unsigned long long)d->sectors * 512ULL;

        print_title("\n=== ACTIVE DISK ===\n\n");
        print("  Disk:   ");
        putchar_os(d->letter);
        print(" (");
        print(disk_position_name(d));
        print(")\n");
        print("  Model:  ");
        print(d->model);
        putchar_os('\n');
        print("  Bytes:  ");
        print_uint64(total);
        print(" B  (");
        print_size_human(total);
        print(")\n\n");

        print("Usage: drive <letter|number>\n");
        print("  drive D  - switch to the disk with the letter D\n");
        print("  drive 2  - switch to the second detected disk\n");
        putchar_os('\n');
        return;
    }

    int idx = resolve_drive_arg(args);

    if (idx < 0)
    {
        print_error("Unknown disk. Type 'disks' to see the list.\n");
        return;
    }

    if (idx == fs_current_drive)
    {
        print_success("Already on that disk (");
        putchar_os(fs_get_drive_letter());
        print(").\n");
        return;
    }

    int res = fs_change_drive(idx);

    if (res == -2)
    {
        print_error("No " OS_NAME " filesystem on that disk (use 'install').\n");
        return;
    }

    if (res != 0)
    {
        print_error("Cannot switch to that disk.\n");
        return;
    }

    ata_drive_t* d = ata_get_drive(fs_current_drive);

    print_success("Switched to ");
    putchar_os(d->letter);
    print(": (");
    print(disk_position_name(d));
    print(")\n");
    print("  New files are now created on ");
    putchar_os(d->letter);
    print(":\n");
}

static void cmd_drive_status(void)
{
    cmd_drive("");
}

static void cmd_disk_info(const char* args)
{
    if (ata_drive_count() == 0)
    {
        print_error("No ATA disks detected.\n");
        return;
    }

    int idx = resolve_drive_arg(args);

    if (idx < 0)
    {
        print_error("Unknown disk. Type 'disks' to see the list.\n");
        return;
    }

    ata_drive_t* d = ata_get_drive(idx);

    if (d == 0)
    {
        print_error("Unknown disk.\n");
        return;
    }

    unsigned long long total = (unsigned long long)d->sectors * 512ULL;

    print_title("\n=== DISK ");
    putchar_os(d->letter);
    print(" ===\n\n");

    print("  Position:   ");
    print(disk_position_name(d));
    putchar_os('\n');

    print("  Model:      ");
    print(d->model);
    putchar_os('\n');

    print("  Sectors:    ");
    print_uint64(d->sectors);
    putchar_os('\n');

    print("  Bytes:      ");
    print_uint64(total);
    print(" B\n");

    print("  Kilobytes:  ");
    print_size_unit(total, SIZE_KB, 2, "KB");
    putchar_os('\n');

    print("  Megabytes:  ");
    print_size_unit(total, SIZE_MB, 2, "MB");
    putchar_os('\n');

    print("  Gigabytes:  ");
    print_size_unit(total, SIZE_GB, 2, "GB");
    putchar_os('\n');

    if (fs_drive_has_fs(idx))
    {
        unsigned int files = 0;
        unsigned long long used = fs_drive_used_bytes(idx, &files);
        unsigned int entries = fs_drive_entry_count(idx);
        unsigned long long overhead =
            (unsigned long long)(1 + FS_DIR_SECTORS + MAX_FILES) * 512ULL;
        unsigned long long free = (total > overhead) ? (total - overhead) : 0;

        print("  Filesystem: ");
        print(OS_NAME);
        putchar_os('\n');

        print("  Entries:    ");
        print_uint64(entries);
        print(" (");
        print_uint64(files);
        print(" files)\n");

        print("  File data:  ");
        print_uint64(used);
        print(" B  (");
        print_size_unit(used, SIZE_KB, 2, "KB");
        print(")\n");

        print("  Overhead:   ");
        print_uint64(overhead);
        print(" B  (");
        print_size_unit(overhead, SIZE_KB, 2, "KB");
        print(")\n");

        print("  Free space: ");
        print_uint64(free);
        print(" B  (");
        print_size_unit(free, SIZE_MB, 2, "MB");
        print(")\n");
    }
    else
    {
        print("  Filesystem: none (use 'install' to create one)\n");
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
        ata_write_sector_d(fs_current_drive, FS_DATA_LBA + i, zero);
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
    ata_write_sector_d(fs_current_drive, FS_DATA_LBA + 1, sector);
    
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
    /* the joke commands are exact matches, so they have to stay in front
       of "rm" (which is matched by prefix) */
    CMD    ("rm -rf /",     death_screen,    0),
    CMD    ("rm /",         death_screen,    0),
    CMD    ("rm disk/",     disk_death,      0),
    CMD_ARG("rm",           fs_rm,           "rm [-r] <file> - remove file (-r = directory)"),
    CMD_ARG("rmdir",        fs_rmdir,        "rmdir <dir> - remove directory with its content"),
    CMD    ("reboot",       reboot,          "reboot - restart " OS_NAME),
    CMD_ARG("run",          nwo_run,         "run <file.nwo> - run NWO application"),
    CMD    ("apps",         apps_list,       "apps - List of apps"),
    CMD    ("generate num", generate_num,    "generate num - generate num"),
    CMD_ARG("hexview",      app_hexview,     "hexview <file> - hex byte map"),
    CMD_ARG("nwocheck",     app_nwo_info,    "nwocheck <file> - check OPcodes"),
    CMD    ("random word",  say,             "random word - generate word"),
    CMD    ("chat",         chat,            "chat - chat with computer"),
    CMD    ("change invite",set_p_shell,     "change invite - Change invite ('" OS_NAME ">')"),
    CMD    ("change color", change_p_color,  "change color - Change prompt color"),
    CMD    ("settings",     cmd_settings,    "settings - show configuration"),
    CMD    ("mem",          cmd_mem,         "mem    - memory info"),
    CMD    ("cpu",          cmd_cpu,         0                        ),
    CMD    ("uptime",       cmd_uptime,      0                        ),
    CMD    ("time",         cmd_time_cmd,    "time   - show time/date"),
    CMD    ("zones",        cmd_zones,       "zones  - list all time zones"),
    CMD_ARG("timezone",    cmd_timezone,    "timezone <num> - set timezone"),
    CMD_ARG("date",        cmd_date_set,    "date   - set custom date"),
    CMD    ("more",         cmd_more_help,   "more   - additional commands"),
    CMD    ("disks",        cmd_disks,       "disks  - list all ATA disks"),
    CMD    ("drive",        cmd_drive_status, 0),
    CMD_ARG("drive",        cmd_drive,       "drive <C|D|n> - switch the active disk"),
    CMD_ARG("chdrive",      cmd_drive,       0),
    CMD_ARG("disk",         cmd_disk_info,   "disk <C|D|n> - detailed size of one disk"),
    CMD    ("install",      cmd_install,      "install - run the installation wizard"),
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
    CMD    ("tictactoe",    game_tictactoe, 0),
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
    print("\nType 'more' for additional commands.\n");
}

static void cmd_more_help(void)
{
    set_color(COLOR_GREEN);
    print("More commands:\n");
    print("  mem      - show memory info\n");
    print("  cpu      - show CPU info\n");
    print("  uptime   - show uptime\n");
    print("  time     - show current time/date\n");
    print("  zones    - list all time zones\n");
    print("  timezone <num> - set timezone\n");
    print("  date <DD> <MM> <YYYY> - set custom date\n");
    print("  date reset - reset date\n");
    print("  nwfetch  - system info\n");
    print("  reboot   - reboot system\n");
    print("  clear    - clear screen\n");
    print("  disks    - list all ATA disks\n");
    print("  drive <C|D|n> - switch the active disk\n");
    print("  disk <C|D|n>  - detailed size of one disk\n");
    putchar_os('\n');
    set_color(base_color);
}

static void cmd_about(void)
{
    set_color(COLOR_WHITE);
    print("====" OS_NAME " " OS_VERSION_STR "====\n");
    print("Name: " OS_NAME "\n");
    print("Version: v" OS_VERSION_STR "\n");
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

static void trim_whitespace(char* s)
{
    if (!s)
        return;

    while (*s == ' ' || *s == '\t')
        s++;

    char* p = s;
    while (*p)
        p++;
    while (p > s && (p[-1] == ' ' || p[-1] == '\t'))
        p--;
    *p = '\0';

    /* move back to start */
    if (p == s)
        return;
    char* d = s;
    char* src = s;
    while (*src == ' ' || *src == '\t')
        src++;
    while (*src)
    {
        *d++ = *src++;
    }
    *d = '\0';
}

static int execute_single_command(char* command)
{
    if (command[0] == '\0')
        return 1;

    for (unsigned int i = 0; i < COMMAND_COUNT; i++)
    {
        const Command* c = &commands[i];

        if (c->run != 0)
        {
            /* exact-word command */
            if (strcmp(command, c->name) == 0)
            {
                c->run();
                return 1;
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
            return 1;
        }

        if (command[n] == '\0')
        {
            print_error("Usage: ");
            print_error(c->help ? c->help : c->name);
            putchar_os('\n');
            return 0;
        }
    }

    print_error("Unknown command. Type 'help'.\n");
    return 0;
}

static void shell_execute(const char* command)
{
    if (command[0] == '\0')
        return;

    char buf[256];
    int j = 0;
    for (int i = 0; command[i] != '\0' && j < 255; i++)
    {
        if (command[i] == '&' && command[i + 1] == '&')
        {
            buf[j] = '\0';
            char cmd[128];
            int k = 0;
            for (int t = 0; buf[t] != '\0' && k < 127; t++)
            {
                cmd[k++] = buf[t];
            }
            cmd[k] = '\0';

            /* trim */
            int start = 0;
            while (cmd[start] == ' ' || cmd[start] == '\t') start++;
            int end = k - 1;
            while (end >= start && (cmd[end] == ' ' || cmd[end] == '\t')) end--;
            char trimmed[128];
            int tt = 0;
            for (int t = start; t <= end && tt < 127; t++) trimmed[tt++] = cmd[t];
            trimmed[tt] = '\0';

            if (execute_single_command(trimmed) == 0)
                return;
            i++;
            j = 0;
            continue;
        }
        buf[j++] = command[i];
    }
    buf[j] = '\0';

    {
        char cmd[128];
        int k = 0;
        for (int t = 0; buf[t] != '\0' && k < 127; t++) cmd[k++] = buf[t];
        cmd[k] = '\0';
        int start = 0;
        while (cmd[start] == ' ' || cmd[start] == '\t') start++;
        int end = k - 1;
        while (end >= start && (cmd[end] == ' ' || cmd[end] == '\t')) end--;
        char trimmed[128];
        int tt = 0;
        for (int t = start; t <= end && tt < 127; t++) trimmed[tt++] = cmd[t];
        trimmed[tt] = '\0';
        execute_single_command(trimmed);
    }
}

void set_p_shell(void)
{
    int choose_p = 0;
    print("1.Default (" OS_NAME "> )\n2.D disk (D:/>)\n3.C disk (C:/>)\n4.Custom\n5.Current disk (");
    putchar_os(fs_get_drive_letter());
    print(":/>)\n");
    print("|\n");
    print("----> ");
    choose_p = read_int();
    if (choose_p == 1) {
        print_success("Set to 'Default'\n");
        shell_p = 0;
        cfg_strcpy(invite_user, OS_NAME "> ", 28);
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
    } else if (choose_p == 5) {
        print_success("Set to 'Current disk'\n");
        shell_p = 3;
        cfg_strcpy(invite_user, "drive", 28);
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

        /* the version is a build-time constant (OS_VERSION_STR), it is
           not taken from the settings file */

        if (cfg_find_key(buffer, "prompt", value, sizeof(value)))
        {
            cfg_strcpy(invite_user, value, 28);

            if (strcmp(value, "drive") == 0)
                shell_p = 3;
            else if (strcmp(value, "D:/> ") == 0)
                shell_p = 1;
            else if (strcmp(value, "C:/> ") == 0)
                shell_p = 2;
            else if (strcmp(value, OS_NAME "> ") == 0)
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
    cfg_append(buffer, &pos, sizeof(buffer), "version", OS_VERSION_STR);
    if (shell_p == 3)
        cfg_append(buffer, &pos, sizeof(buffer), "prompt", "drive");
    else
        cfg_append(buffer, &pos, sizeof(buffer), "prompt", invite_user);
    cfg_append(buffer, &pos, sizeof(buffer), "prompt_color", color_names[prompt_color & 0x0F]);
    cfg_append(buffer, &pos, sizeof(buffer), "color", color_names[base_color & 0x0F]);
    cfg_append(buffer, &pos, sizeof(buffer), "help_color", color_names[text_color & 0x0F]);

    fs_write(SETTINGS_FILE, buffer);

    current_dir = prev_dir;
}

static const char* shell_prompt_text(void)
{
    static char drive_prompt[8];

    if (shell_p == 0) return OS_NAME "> ";
    if (shell_p == 1) return "D:/> ";
    if (shell_p == 2) return "C:/> ";

    if (shell_p == 3)
    {
        drive_prompt[0] = fs_get_drive_letter();
        drive_prompt[1] = ':';
        drive_prompt[2] = '/';
        drive_prompt[3] = '>';
        drive_prompt[4] = ' ';
        drive_prompt[5] = '\0';

        return drive_prompt;
    }

    return invite_user;
}

void cmd_settings(void)
{
    print_title("\n=== SETTINGS ===\n\n");
    print("user        = "); print(cfg_user);        putchar_os('\n');
    print("os          = "); print(cfg_os);          putchar_os('\n');
    print("version     = "); print(OS_VERSION_STR);  putchar_os('\n');
    print("prompt      = "); print(shell_prompt_text()); putchar_os('\n');
    print("prompt_color= "); print(color_names[prompt_color & 0x0F]); putchar_os('\n');
    print("color       = "); print(color_names[base_color & 0x0F]);   putchar_os('\n');
    print("help_color  = "); print(color_names[text_color & 0x0F]);   putchar_os('\n');
    print("disk        = "); putchar_os(fs_get_drive_letter());      putchar_os('\n');
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
    print(shell_prompt_text());
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

static void print_size_bytes(unsigned long long bytes)
{
    if (bytes >= SIZE_GB)
        print_size_unit(bytes, SIZE_GB, 2, "GB");
    else if (bytes >= SIZE_MB)
        print_size_unit(bytes, SIZE_MB, 2, "MB");
    else if (bytes >= SIZE_KB)
        print_size_unit(bytes, SIZE_KB, 2, "KB");
    else
    {
        print_uint64(bytes);
        print(" bytes");
    }
}

static unsigned long long rootfs_total_size(void)
{
    unsigned long long total = 0;

    for (int i = 0; i < ROOTFS_FILE_COUNT; i++)
    {
        total += rootfs_files[i].size;
    }

    return total;
}

static unsigned long long install_disk_usage(void)
{
    /* magic sector + directory sectors + one data sector per slot */
    return (unsigned long long)(1 + FS_DIR_SECTORS + MAX_FILES) * 512ULL;
}

static void installer_print_disks(int selected)
{
    int n = ata_drive_count();

    for (int i = 0; i < n; i++)
    {
        ata_drive_t* d = ata_get_drive(i);

        print("    ");

        if (selected == i) set_color(COLOR_LIGHT_GREEN);
        else set_color(base_color);

        print_int(i + 1);
        print(") ");

        putchar_os(d->letter);
        print(":  ");

        set_color(COLOR_WHITE);
        print(d->model);
        set_color(base_color);

        print("   ");
        print_size_bytes((unsigned long long)d->sectors * 512ULL);

        if (fs_drive_has_fs(i))
        {
            set_color(COLOR_LIGHT_RED);
            print("  [has " OS_NAME "]");
            set_color(base_color);
        }

        if (selected == i) set_color(COLOR_LIGHT_GREEN);
        else set_color(base_color);

        putchar_os('\n');
    }
}

static void cmd_install(void)
{
    installer_cancelled = 0;

    installer();

    if (installer_cancelled)
    {
        print("Installation cancelled. The system keeps running.\n");
    }
}

void installer(void) {
    clear();
    fb_init();
    set_color(COLOR_CYAN);
    print("\n");
    print("  +======================================+\n");
    print("  |     " OS_NAME " Installation Wizard " OS_VERSION_STR "   |\n");
    print("  +======================================+\n\n");
    set_color(base_color);
    
    print("  Welcome! No " OS_NAME " filesystem was found.\n\n");
    
    int n = ata_drive_count();

    if (n == 0)
    {
        print_error("  No ATA disks detected!\n");
        print("  Add one to QEMU: -drive file=disk.img,format=raw,if=ide,index=0\n");
        installer_cancelled = 1;
        return;
    }

    print_title("  Detected disks:\n");
    installer_print_disks(-1);

    print_title("\n  Installation size:\n");
    print("    Files:       ");
    print_int(ROOTFS_FILE_COUNT);
    print("\n    Payload:     ");
    print_size_bytes(rootfs_total_size());
    print("\n    Disk usage:  ");
    print_size_bytes(install_disk_usage());
    print(" (");
    print_int(1 + FS_DIR_SECTORS + MAX_FILES);
    print(" sectors)\n");

    char buf[32];
    int sel = -1;

    while (sel < 0)
    {
        print("\n  Select target disk (1-");
        print_int(n);
        print("), default 1: ");

        read_line(buf, 32);

        if (buf[0] == '\0')
        {
            sel = 1;
            continue;
        }

        int value = atoi_simple(buf);

        if (value < 1 || value > n)
        {
            print_error("  Invalid disk number.\n");
            continue;
        }

        sel = value;
    }

    ata_drive_t* target = ata_get_drive(sel - 1);

    if (target == 0)
    {
        print_error("\n  Disk is not available.\n");
        installer_cancelled = 1;
        return;
    }

    if (target->sectors < (unsigned int)(1 + FS_DIR_SECTORS + MAX_FILES))
    {
        print_error("\n  Disk is too small for " OS_NAME ".\n");
        print("  Required: ");
        print_size_bytes(install_disk_usage());
        print(", available: ");
        print_size_bytes((unsigned long long)target->sectors * 512ULL);
        print("\n");
        installer_cancelled = 1;
        return;
    }

    print("\n  Target disk:\n");
    installer_print_disks(sel - 1);

    if (fs_drive_has_fs(sel - 1))
    {
        print_error("\n  WARNING: this disk already contains an " OS_NAME " installation.\n");
    }

    print("\n  ALL data on ");
    putchar_os(target->letter);
    print(": will be erased!\n  Continue? (y/n): ");

    read_line(buf, 32);
    
    if (buf[0] != 'y' && buf[0] != 'Y') {
        print_error("\n  Cancelled. Nothing was changed.\n");
        installer_cancelled = 1;
        return;
    }
    
    print_success("\n\n  Installing to ");
    putchar_os(target->letter);
    print(": ...\n");
    sleep(500);

    fs_format_disk(sel - 1);
    
    print_success("\n  Installation complete!\n");
    print("  " OS_NAME " was installed on ");
    putchar_os(target->letter);
    print(":\n");
    print("    Files:       ");
    print_int(ROOTFS_FILE_COUNT);
    print("\n    Payload:     ");
    print_size_bytes(rootfs_total_size());
    print("\n    Disk usage:  ");
    print_size_bytes(install_disk_usage());
    print("\n");
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

        /* the installer reboots on success, so we only get here if it was cancelled */
        while(1) __asm__ volatile("hlt");
        return;
    }
    
    if (status == 2) {
        dead_boot();
    }

    print("CLEARING...\n");
    startingNWOS();
    settings_load();
    print("Starting " OS_NAME "...\n");
    sleep(500);
    print_success("Kernel Started!\n");
    print("Clearing..\n");

    sleep(1500);

    clear();

    fb_draw_image(288, 198, LogoNwOS, LOGO_NWOS_WIDTH, LOGO_NWOS_HEIGHT);

    sleep(1500);

    print("================================\n");
    print("        Welcome to " OS_NAME " " OS_VERSION_STR "\n");
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
