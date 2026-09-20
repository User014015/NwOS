#ifndef TIMER_H
#define TIMER_H

void timer_init(void);
void timer_poll(void);
unsigned int timer_ms(void);
unsigned int timer_us(void);
unsigned short timer_read(void);
unsigned int   timer_elapsed_us(unsigned short prev, unsigned short now);

#endif