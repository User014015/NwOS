#ifndef DISK_H
#define DISK_H

/*
 * Minimal ATA PIO driver, primary bus, master drive, LBA28.
 * Used by the kernel's disk-backed filesystem (see fs_* in kernel.c).
 *
 * NOTE: this only works once the CPU is in protected mode, i.e.
 * from the kernel. BIOS int 0x13 (used by boot.asm) is real-mode
 * only and is unrelated to this driver.
 */

void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);

void outw(unsigned short port, unsigned short value);
unsigned short inw(unsigned short port);

/*
 * Reads one 512-byte sector at the given LBA into buffer.
 * buffer must be at least 512 bytes.
 */
void ata_read_sector(unsigned int lba, unsigned char* buffer);

/*
 * Writes one 512-byte sector at the given LBA from buffer,
 * then flushes the drive's write cache.
 */
void ata_write_sector(unsigned int lba, const unsigned char* buffer);

#endif