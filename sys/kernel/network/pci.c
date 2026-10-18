#include "pci.h"
#include "portio.h"

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

static unsigned int cfg_addr(unsigned char bus, unsigned char dev,
                             unsigned char func, unsigned char off)
{
    return 0x80000000u
         | ((unsigned int)bus  << 16)
         | ((unsigned int)dev  << 11)
         | ((unsigned int)func << 8)
         | (unsigned int)(off & 0xFC);
}

unsigned int pci_read32(unsigned char bus, unsigned char dev,
                        unsigned char func, unsigned char offset)
{
    pio_outl(PCI_ADDR, cfg_addr(bus, dev, func, offset));
    return pio_inl(PCI_DATA);
}

void pci_write32(unsigned char bus, unsigned char dev,
                 unsigned char func, unsigned char offset, unsigned int value)
{
    pio_outl(PCI_ADDR, cfg_addr(bus, dev, func, offset));
    pio_outl(PCI_DATA, value);
}

int pci_scan(pci_dev_t* list, int max)
{
    int count = 0;

    for (int bus = 0; bus < 256; bus++)
    {
        for (int dev = 0; dev < 32; dev++)
        {
            unsigned int id0 = pci_read32((unsigned char)bus, (unsigned char)dev, 0, 0x00);

            if ((id0 & 0xFFFF) == 0xFFFF)
                continue;

            unsigned int header = pci_read32((unsigned char)bus, (unsigned char)dev, 0, 0x0C);
            int funcs = ((header >> 16) & 0x80) ? 8 : 1;

            for (int fn = 0; fn < funcs; fn++)
            {
                unsigned int id = pci_read32((unsigned char)bus, (unsigned char)dev,
                                             (unsigned char)fn, 0x00);

                if ((id & 0xFFFF) == 0xFFFF)
                    continue;

                if (count < max)
                {
                    unsigned int cls = pci_read32((unsigned char)bus, (unsigned char)dev,
                                                  (unsigned char)fn, 0x08);

                    list[count].bus        = (unsigned char)bus;
                    list[count].dev        = (unsigned char)dev;
                    list[count].func       = (unsigned char)fn;
                    list[count].vendor     = (unsigned short)(id & 0xFFFF);
                    list[count].device     = (unsigned short)(id >> 16);
                    list[count].class_code = (unsigned char)(cls >> 24);
                    list[count].subclass   = (unsigned char)(cls >> 16);
                }

                count++;
            }
        }
    }

    return count > max ? max : count;
}

int pci_find(unsigned short vendor, unsigned short device, pci_dev_t* out)
{
    pci_dev_t list[32];
    int n = pci_scan(list, 32);

    for (int i = 0; i < n; i++)
    {
        if (list[i].vendor == vendor && list[i].device == device)
        {
            *out = list[i];
            return 1;
        }
    }

    return 0;
}