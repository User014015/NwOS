#ifndef DISK_H
#define DISK_H

#define MAX_DRIVES 4

typedef struct {
    int exists;
    int channel;
    int master;
    unsigned int sectors;
    char model[41];
    char letter;
} ata_drive_t;

void ata_init(void);
int  ata_drive_count(void);
ata_drive_t* ata_get_drive(int index);

int ata_read_sector_d (int drive_idx, unsigned int lba, unsigned char* buffer);
int ata_write_sector_d(int drive_idx, unsigned int lba, const unsigned char* buffer);

int ata_read_sector (unsigned int lba, unsigned char* buffer);
int ata_write_sector(unsigned int lba, const unsigned char* buffer);

#endif