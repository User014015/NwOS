#ifndef DISK_H
#define DISK_H

void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);

void outw(unsigned short port, unsigned short value);
unsigned short inw(unsigned short port);
void ata_read_sector(unsigned int lba, unsigned char* buffer);
void ata_write_sector(unsigned int lba, const unsigned char* buffer);

#endif