#include "delay.h"

static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

void delay_ms(unsigned int ms) {
    for (unsigned int i = 0; i < ms; i++) {
        for (int j = 0; j < 1000; j++) {
            io_wait();
        }
    }
}