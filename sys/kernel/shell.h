#ifndef SHELL_H
#define SHELL_H
void shell_init(void);
void shell_draw(void);
void shell_handle_key(char c);
static void push_line(const char *s);
void shell_scroll(int delta);

void shell_goto_menu(void);
void shell_goto_welcome(void);
void shell_run_calc(void);
void shell_run_game2d(const char *name);
void shell_apply_theme(void);
void shell_run_game(const char *name);
void shell_run_chat(void);
void shell_run_editor(const char *name);
#endif