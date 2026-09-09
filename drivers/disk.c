#include "disk.h"

/* =========================
   Disk I/O (ATA PIO, primary master, LBA28)
   ========================= */

#define ATA_DATA        0x1F0
#define ATA_FEATURES    0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_FLUSH   0xE7

#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRQ  0x08

void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
    unsigned char value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

void outw(unsigned short port, unsigned short value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

unsigned short inw(unsigned short port)
{
    unsigned short value;

    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));

    return value;
}

static void ata_wait_bsy(void)
{
    while (inb(ATA_STATUS) & ATA_STATUS_BSY)
    {
    }
}

static void ata_wait_drq(void)
{
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ))
    {
    }
}

static void ata_select(unsigned int lba)
{
    outb(ATA_DRIVE_HEAD, (unsigned char)(0xE0 | ((lba >> 24) & 0x0F)));
    outb(ATA_FEATURES, 0);
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO,  (unsigned char)(lba & 0xFF));
    outb(ATA_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HI,  (unsigned char)((lba >> 16) & 0xFF));
}

void ata_read_sector(unsigned int lba, unsigned char* buffer)
{
    ata_wait_bsy();
    ata_select(lba);

    outb(ATA_COMMAND, ATA_CMD_READ);

    ata_wait_bsy();
    ata_wait_drq();

    unsigned short* buf16 = (unsigned short*)buffer;

    for (int i = 0; i < 256; i++)
    {
        buf16[i] = inw(ATA_DATA);
    }
}

void ata_write_sector(unsigned int lba, const unsigned char* buffer)
{
    ata_wait_bsy();
    ata_select(lba);

    outb(ATA_COMMAND, ATA_CMD_WRITE);

    ata_wait_bsy();
    ata_wait_drq();

    const unsigned short* buf16 = (const unsigned short*)buffer;

    for (int i = 0; i < 256; i++)
    {
        outw(ATA_DATA, buf16[i]);
    }

    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_wait_bsy();
}