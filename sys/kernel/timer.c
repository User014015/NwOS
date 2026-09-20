#include "timer.h"
#include "io.h"

static unsigned short prev_pit = 0;
static unsigned int   total_us = 0;
static int initialized = 0;

static unsigned short read_pit(void) {
    outb(0x43, 0x00);
    unsigned char lo = inb(0x40);
    unsigned char hi = inb(0x40);
    return ((unsigned short)hi << 8) | lo;
}

void timer_init(void) {
    prev_pit = read_pit();
    total_us = 0;
    initialized = 1;
}

void timer_poll(void) {
    if (!initialized) { timer_init(); return; }
    unsigned short now = read_pit();
    unsigned int diff;
    if (prev_pit >= now) diff = prev_pit - now;
    else                  diff = prev_pit + 65536u - now;
    prev_pit = now;
    total_us += (diff * 838u) / 1000u;
}

unsigned int timer_us(void) { timer_poll(); return total_us; }
unsigned int timer_ms(void) { return timer_us() / 1000u; }
unsigned short timer_read(void) { return read_pit(); }

unsigned int timer_elapsed_us(unsigned short prev, unsigned short now) {
    unsigned int diff;
    if (prev >= now) diff = prev - now;
    else             diff = prev + 65536u - now;
    return diff * 838u / 1000u;
}