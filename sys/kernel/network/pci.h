#ifndef PCI_H
#define PCI_H

typedef struct
{
    unsigned char  bus, dev, func;
    unsigned short vendor, device;
    unsigned char  class_code, subclass;
} pci_dev_t;

unsigned int pci_read32(unsigned char bus, unsigned char dev,
                        unsigned char func, unsigned char offset);
void pci_write32(unsigned char bus, unsigned char dev,
                 unsigned char func, unsigned char offset, unsigned int value);

int pci_scan(pci_dev_t* list, int max);

int pci_find(unsigned short vendor, unsigned short device, pci_dev_t* out);

#endif