#include "ata.h"

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(unsigned short port, unsigned short val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void ata_wait_ready(void) {
    while ((inb(0x1F7) & 0x80) != 0);
}

static void ata_wait_drq(void) {
    while ((inb(0x1F7) & 0x08) == 0);
}

void ata_init(void) {
    outb(0x1F6, 0xA0);
}

int ata_read_sector(unsigned int lba, unsigned char *buffer) {
    ata_wait_ready();

    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x20);

    ata_wait_ready();
    ata_wait_drq();

    unsigned short *ptr = (unsigned short *)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
    return 0;
}

int ata_write_sector(unsigned int lba, const unsigned char *buffer) {
    ata_wait_ready();

    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F7, 0x30);

    ata_wait_ready();
    ata_wait_drq();

    const unsigned short *ptr = (const unsigned short *)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);
    }

    outb(0x1F7, 0xE7);
    ata_wait_ready();

    return 0;
}