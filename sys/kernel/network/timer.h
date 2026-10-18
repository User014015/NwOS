#ifndef TIMER_H
#define TIMER_H

void timer_init(void);                 /* safe to call more than once   */
unsigned long long timer_us(void);     /* microseconds since timer_init */
unsigned long long timer_ms(void);     /* milliseconds since timer_init */

#endif