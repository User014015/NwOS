#ifndef CHAT_H
#define CHAT_H

void chat_init(void);
void chat_draw(void);
void chat_handle_key(unsigned char c);
void chat_scroll(int delta);

#endif