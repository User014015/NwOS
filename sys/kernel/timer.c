#include "timer.h"
#include "io.h"

volatile unsigned int timer_ticks = 0;

void irq0_handler(void) {
    timer_ticks++;
    outb(0x20, 0x20);
}

static unsigned short read_pit_raw(void) {
    outb(0x43, 0x00);
    unsigned char lo = inb(0x40);
    unsigned char hi = inb(0x40);
    return ((unsigned short)hi << 8) | lo;
}

void timer_init(void) {
    outb(0x43, 0x36);
    unsigned int div = 1193;
    outb(0x40, (unsigned char)(div & 0xFF));
    outb(0x40, (unsigned char)((div >> 8) & 0xFF));

    timer_ticks = 0;

    unsigned char mask = inb(0x21);
    mask &= ~0x01;
    outb(0x21, mask);
}

unsigned int timer_us(void) {
    unsigned int t;
    __asm__ volatile ("cli");
    t = timer_ticks;
    __asm__ volatile ("sti");
    return t * 1000u;
}

unsigned int timer_ms(void) {
    return timer_us() / 1000u;
}

unsigned short timer_read(void) {
    return read_pit_raw();
}

unsigned int timer_elapsed_us(unsigned short prev, unsigned short now) {
    unsigned int diff;
    if (prev >= now) diff = prev - now;
    else             diff = prev + 65536u - now;
    return diff * 838u / 1000u;
}