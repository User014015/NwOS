#include "disk.h"
#include "../include/kernelpanic.h"

#define ATA_TIMEOUT 100000

#define ATA_PRIMARY_BASE    0x1F0
#define ATA_PRIMARY_CTRL    0x3F6
#define ATA_SECONDARY_BASE  0x170
#define ATA_SECONDARY_CTRL  0x376

#define ATA_REG_DATA       0
#define ATA_REG_ERROR      1
#define ATA_REG_FEATURES   1
#define ATA_REG_SECCOUNT   2
#define ATA_REG_LBA_LO     3
#define ATA_REG_LBA_MID    4
#define ATA_REG_LBA_HI     5
#define ATA_REG_DRIVE      6
#define ATA_REG_STATUS     7
#define ATA_REG_COMMAND    7

#define ATA_CMD_READ     0x20
#define ATA_CMD_WRITE    0x30
#define ATA_CMD_FLUSH    0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY   0x80
#define ATA_SR_DRDY  0x40
#define ATA_SR_DF    0x20
#define ATA_SR_DRQ   0x08
#define ATA_SR_ERR   0x01

static ata_drive_t drives[MAX_DRIVES];
static int drive_count = 0;

void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
unsigned char inb(unsigned short port) {
    unsigned char v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
void outw(unsigned short port, unsigned short value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}
unsigned short inw(unsigned short port) {
    unsigned short v;
    __asm__ volatile ("inw %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static unsigned short base_port(int ch) {
    return ch == 0 ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
}
static unsigned short ctrl_port(int ch) {
    return ch == 0 ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;
}

static void ata_delay_400ns(int ch) {
    for (int i = 0; i < 4; i++) (void)inb(ctrl_port(ch));
}

static int ata_wait(int ch, unsigned char mask, unsigned char want) {
    unsigned short base = base_port(ch);
    for (unsigned int i = 0; i < ATA_TIMEOUT; i++) {
        unsigned char s = inb(base + ATA_REG_STATUS);
        if (s & ATA_SR_ERR) return -1;
        if (s & ATA_SR_DF)  return -1;
        if ((s & mask) == want) return 0;
    }
    return -2;
}

static int ata_identify(int ch, int master, unsigned short* id) {
    unsigned short base = base_port(ch);

    outb(base + ATA_REG_DRIVE, master ? 0xA0 : 0xB0);
    ata_delay_400ns(ch);

    outb(base + ATA_REG_SECCOUNT, 0);
    outb(base + ATA_REG_LBA_LO, 0);
    outb(base + ATA_REG_LBA_MID, 0);
    outb(base + ATA_REG_LBA_HI, 0);

    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay_400ns(ch);

    if (inb(base + ATA_REG_STATUS) == 0) return -1;

    if (ata_wait(ch, ATA_SR_BSY, 0) != 0) return -1;

    if (inb(base + ATA_REG_LBA_MID) != 0 || inb(base + ATA_REG_LBA_HI) != 0)
        return -1;

    if (ata_wait(ch, ATA_SR_DRQ, ATA_SR_DRQ) != 0) return -1;

    for (int i = 0; i < 256; i++)
        id[i] = inw(base + ATA_REG_DATA);

    return 0;
}

static void probe(int ch, int master, char letter) {
    if (drive_count >= MAX_DRIVES) return;

    unsigned short id[256];
    if (ata_identify(ch, master, id) != 0) return;

    ata_drive_t* d = &drives[drive_count];
    d->exists  = 1;
    d->channel = ch;
    d->master  = master;
    d->letter  = letter;
    d->sectors = ((unsigned int)id[61] << 16) | id[60];

    for (int i = 0; i < 20; i++) {
        d->model[i*2]   = (char)(id[27 + i] >> 8);
        d->model[i*2+1] = (char)(id[27 + i] & 0xFF);
    }
    d->model[40] = '\0';
    for (int i = 39; i >= 0 && d->model[i] == ' '; i--) d->model[i] = '\0';

    drive_count++;
}

void ata_init(void) {
    drive_count = 0;
    for (int i = 0; i < MAX_DRIVES; i++) drives[i].exists = 0;

    char letters[4] = { 'C', 'D', 'E', 'F' };
    probe(0, 1, letters[0]);    /* primary master    */
    probe(0, 0, letters[1]);    /* primary slave     */
    probe(1, 1, letters[2]);    /* secondary master  */
    probe(1, 0, letters[3]);    /* secondary slave   */
}

int ata_drive_count(void) { return drive_count; }

ata_drive_t* ata_get_drive(int index) {
    if (index < 0 || index >= drive_count) return 0;
    return &drives[index];
}

int ata_read_sector_d(int idx, unsigned int lba, unsigned char* buf) {
    if (idx < 0 || idx >= drive_count) return -1;
    ata_drive_t* d = &drives[idx];
    unsigned short base = base_port(d->channel);

    if (ata_wait(d->channel, ATA_SR_BSY, 0) != 0) return -1;

    outb(base + ATA_REG_DRIVE,
         (unsigned char)(0xE0 | (d->master ? 0x00 : 0x10) | ((lba >> 24) & 0x0F)));
    outb(base + ATA_REG_FEATURES, 0);
    outb(base + ATA_REG_SECCOUNT, 1);
    outb(base + ATA_REG_LBA_LO,  (unsigned char)(lba & 0xFF));
    outb(base + ATA_REG_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    outb(base + ATA_REG_LBA_HI,  (unsigned char)((lba >> 16) & 0xFF));

    outb(base + ATA_REG_COMMAND, ATA_CMD_READ);
    ata_delay_400ns(d->channel);

    if (ata_wait(d->channel, ATA_SR_BSY, 0) != 0) return -1;
    if (ata_wait(d->channel, ATA_SR_DRQ, ATA_SR_DRQ) != 0) return -1;

    unsigned short* b16 = (unsigned short*)buf;
    for (int i = 0; i < 256; i++) b16[i] = inw(base + ATA_REG_DATA);
    return 0;
}

int ata_write_sector_d(int idx, unsigned int lba, const unsigned char* buf) {
    if (idx < 0 || idx >= drive_count) return -1;
    ata_drive_t* d = &drives[idx];
    unsigned short base = base_port(d->channel);

    if (ata_wait(d->channel, ATA_SR_BSY, 0) != 0) return -1;

    outb(base + ATA_REG_DRIVE,
         (unsigned char)(0xE0 | (d->master ? 0x00 : 0x10) | ((lba >> 24) & 0x0F)));
    outb(base + ATA_REG_FEATURES, 0);
    outb(base + ATA_REG_SECCOUNT, 1);
    outb(base + ATA_REG_LBA_LO,  (unsigned char)(lba & 0xFF));
    outb(base + ATA_REG_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    outb(base + ATA_REG_LBA_HI,  (unsigned char)((lba >> 16) & 0xFF));

    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE);
    ata_delay_400ns(d->channel);

    if (ata_wait(d->channel, ATA_SR_BSY, 0) != 0) return -1;
    if (ata_wait(d->channel, ATA_SR_DRQ, ATA_SR_DRQ) != 0) return -1;

    const unsigned short* b16 = (const unsigned short*)buf;
    for (int i = 0; i < 256; i++) outw(base + ATA_REG_DATA, b16[i]);

    outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH);
    if (ata_wait(d->channel, ATA_SR_BSY, 0) != 0) return -1;
    return 0;
}

int ata_read_sector(unsigned int lba, unsigned char* buffer) {
    if (drive_count == 0) return -1;
    return ata_read_sector_d(0, lba, buffer);
}

int ata_write_sector(unsigned int lba, const unsigned char* buffer) {
    if (drive_count == 0) return -1;
    return ata_write_sector_d(0, lba, buffer);
}