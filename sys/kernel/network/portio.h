#ifndef PORTIO_H
#define PORTIO_H

static inline void pio_outb(unsigned short port, unsigned char v)
{
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline unsigned char pio_inb(unsigned short port)
{
    unsigned char v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline void pio_outl(unsigned short port, unsigned int v)
{
    __asm__ volatile ("outl %0, %1" : : "a"(v), "Nd"(port));
}

static inline unsigned int pio_inl(unsigned short port)
{
    unsigned int v;
    __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

#endif