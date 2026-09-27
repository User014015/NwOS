#ifndef ATA_H
#define ATA_H

#define ATA_SECTOR_SIZE 512

void ata_init(void);
int  ata_read_sector(unsigned int lba, unsigned char *buffer);
int  ata_write_sector(unsigned int lba, const unsigned char *buffer);

#endif