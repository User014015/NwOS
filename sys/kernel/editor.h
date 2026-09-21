#ifndef EDITOR_H
#define EDITOR_H

void editor_open(const char *filename);
void editor_draw(void);
void editor_handle_key(unsigned char c);
int  editor_want_quit(void);
void editor_clear_quit(void);
const char *editor_filename(void);

#endif