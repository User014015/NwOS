#include "e1000.h"
#include "pci.h"
#include "timer.h"

/* ---- registers --------------------------------------------------------- */
#define REG_CTRL    0x0000
#define REG_STATUS  0x0008
#define REG_EERD    0x0014
#define REG_ICR     0x00C0
#define REG_IMC     0x00D8
#define REG_RCTL    0x0100
#define REG_TCTL    0x0400
#define REG_TIPG    0x0410
#define REG_RDBAL   0x2800
#define REG_RDBAH   0x2804
#define REG_RDLEN   0x2808
#define REG_RDH     0x2810
#define REG_RDT     0x2818
#define REG_TDBAL   0x3800
#define REG_TDBAH   0x3804
#define REG_TDLEN   0x3808
#define REG_TDH     0x3810
#define REG_TDT     0x3818
#define REG_MTA     0x5200
#define REG_RAL0    0x5400
#define REG_RAH0    0x5404

#define CTRL_ASDE   (1u << 5)
#define CTRL_SLU    (1u << 6)
#define CTRL_RST    (1u << 26)

#define STATUS_LU   (1u << 1)

#define RCTL_EN     (1u << 1)
#define RCTL_BAM    (1u << 15)
#define RCTL_SECRC  (1u << 26)

#define TCTL_EN     (1u << 1)
#define TCTL_PSP    (1u << 3)

#define TX_CMD_EOP  0x01
#define TX_CMD_IFCS 0x02
#define TX_CMD_RS   0x08
#define DESC_DD     0x01

#define RX_N   16
#define TX_N   8
#define BUF_SZ 2048

struct rx_desc
{
    unsigned long long addr;
    unsigned short     length;
    unsigned short     csum;
    unsigned char      status;
    unsigned char      errors;
    unsigned short     special;
} __attribute__((packed));

struct tx_desc
{
    unsigned long long addr;
    unsigned short     length;
    unsigned char      cso;
    unsigned char      cmd;
    unsigned char      status;
    unsigned char      css;
    unsigned short     special;
} __attribute__((packed));

static volatile struct rx_desc rx_ring[RX_N] __attribute__((aligned(128)));
static volatile struct tx_desc tx_ring[TX_N] __attribute__((aligned(128)));
static unsigned char rx_buf[RX_N][BUF_SZ] __attribute__((aligned(16)));
static unsigned char tx_buf[TX_N][BUF_SZ] __attribute__((aligned(16)));

static unsigned long mmio = 0;
static unsigned char mac[6];
static unsigned int rx_cur = 0;
static unsigned int tx_tail = 0;

static unsigned int rd(unsigned int reg)
{
    return *(volatile unsigned int*)(mmio + reg);
}

static void wr(unsigned int reg, unsigned int value)
{
    *(volatile unsigned int*)(mmio + reg) = value;
}

static int eeprom_read(unsigned int word, unsigned short* out)
{
    wr(REG_EERD, 1u | (word << 8));

    unsigned long long t = timer_ms();
    unsigned int v;

    while (!((v = rd(REG_EERD)) & 0x10))
    {
        if (timer_ms() - t > 50)
            return -1;
    }

    *out = (unsigned short)(v >> 16);
    return 0;
}

static void delay_ms(unsigned int ms)
{
    unsigned long long t = timer_ms();
    while (timer_ms() - t < ms)
        __asm__ volatile ("pause");
}

int e1000_init(void)
{
    pci_dev_t dev;

    timer_init();

    if (!pci_find(0x8086, 0x100E, &dev) &&     /* 82540EM: QEMU default */
        !pci_find(0x8086, 0x100F, &dev) &&     /* 82545EM               */
        !pci_find(0x8086, 0x1004, &dev))       /* 82543GC               */
    {
        return E1000_ERR_NO_DEVICE;
    }

    unsigned int cmd = pci_read32(dev.bus, dev.dev, dev.func, 0x04) & 0xFFFF;
    pci_write32(dev.bus, dev.dev, dev.func, 0x04, cmd | 0x06);

    unsigned int bar0 = pci_read32(dev.bus, dev.dev, dev.func, 0x10);

    if (bar0 & 1)
        return E1000_ERR_BAD_BAR;

    mmio = (unsigned long)(bar0 & ~0xFu);

    if (mmio == 0)
        return E1000_ERR_BAD_BAR;

    wr(REG_IMC, 0xFFFFFFFFu);
    wr(REG_CTRL, rd(REG_CTRL) | CTRL_RST);
    delay_ms(10);

    unsigned long long t = timer_ms();
    while (rd(REG_CTRL) & CTRL_RST)
    {
        if (timer_ms() - t > 200)
            return E1000_ERR_RESET;
    }

    wr(REG_IMC, 0xFFFFFFFFu);
    (void)rd(REG_ICR);

    wr(REG_CTRL, (rd(REG_CTRL) | CTRL_SLU | CTRL_ASDE)
                 & ~((1u << 3) | (1u << 30) | (1u << 31)));

    unsigned short w0, w1, w2;

    if (eeprom_read(0, &w0) == 0 && eeprom_read(1, &w1) == 0 &&
        eeprom_read(2, &w2) == 0 && (w0 | w1 | w2) != 0)
    {
        mac[0] = (unsigned char)(w0 & 0xFF); mac[1] = (unsigned char)(w0 >> 8);
        mac[2] = (unsigned char)(w1 & 0xFF); mac[3] = (unsigned char)(w1 >> 8);
        mac[4] = (unsigned char)(w2 & 0xFF); mac[5] = (unsigned char)(w2 >> 8);
    }
    else
    {
        unsigned int ral = rd(REG_RAL0);
        unsigned int rah = rd(REG_RAH0);

        mac[0] = (unsigned char)(ral);        mac[1] = (unsigned char)(ral >> 8);
        mac[2] = (unsigned char)(ral >> 16);  mac[3] = (unsigned char)(ral >> 24);
        mac[4] = (unsigned char)(rah);        mac[5] = (unsigned char)(rah >> 8);

        if ((ral | (rah & 0xFFFF)) == 0)
            return E1000_ERR_NO_MAC;
    }

    wr(REG_RAL0, (unsigned int)mac[0] | ((unsigned int)mac[1] << 8) |
                 ((unsigned int)mac[2] << 16) | ((unsigned int)mac[3] << 24));
    wr(REG_RAH0, (unsigned int)mac[4] | ((unsigned int)mac[5] << 8) | (1u << 31));

    for (unsigned int i = 0; i < 128; i++)
        wr(REG_MTA + i * 4, 0);

    for (unsigned int i = 0; i < RX_N; i++)
    {
        rx_ring[i].addr    = (unsigned long long)(unsigned long)rx_buf[i];
        rx_ring[i].length  = 0;
        rx_ring[i].csum    = 0;
        rx_ring[i].status  = 0;
        rx_ring[i].errors  = 0;
        rx_ring[i].special = 0;
    }

    rx_cur = 0;

    wr(REG_RDBAL, (unsigned int)(unsigned long)rx_ring);
    wr(REG_RDBAH, 0);
    wr(REG_RDLEN, RX_N * (unsigned int)sizeof(struct rx_desc));
    wr(REG_RDH, 0);
    wr(REG_RDT, RX_N - 1);
    wr(REG_RCTL, RCTL_EN | RCTL_BAM | RCTL_SECRC);

    for (unsigned int i = 0; i < TX_N; i++)
    {
        tx_ring[i].addr    = (unsigned long long)(unsigned long)tx_buf[i];
        tx_ring[i].length  = 0;
        tx_ring[i].cso     = 0;
        tx_ring[i].cmd     = 0;
        tx_ring[i].status  = DESC_DD;
        tx_ring[i].css     = 0;
        tx_ring[i].special = 0;
    }

    tx_tail = 0;

    wr(REG_TDBAL, (unsigned int)(unsigned long)tx_ring);
    wr(REG_TDBAH, 0);
    wr(REG_TDLEN, TX_N * (unsigned int)sizeof(struct tx_desc));
    wr(REG_TDH, 0);
    wr(REG_TDT, 0);
    wr(REG_TIPG, 0x0060200A);
    wr(REG_TCTL, TCTL_EN | TCTL_PSP | (0x10u << 4) | (0x40u << 12));

    return 0;
}

int e1000_link_up(void)
{
    if (mmio == 0)
        return 0;

    return (rd(REG_STATUS) & STATUS_LU) ? 1 : 0;
}

const unsigned char* e1000_mac(void)
{
    return mac;
}

int e1000_send(const void* frame, unsigned int len)
{
    if (mmio == 0 || len == 0 || len > 1600)
        return -1;

    unsigned int i = tx_tail;
    volatile struct tx_desc* d = &tx_ring[i];

    unsigned long long t = timer_ms();

    while (!(d->status & DESC_DD))
    {
        if (timer_ms() - t > 100)
            return -2;
    }

    const unsigned char* src = (const unsigned char*)frame;

    for (unsigned int k = 0; k < len; k++)
        tx_buf[i][k] = src[k];

    d->addr   = (unsigned long long)(unsigned long)tx_buf[i];
    d->length = (unsigned short)len;
    d->cso    = 0;
    d->css    = 0;
    d->special = 0;
    d->status = 0;
    d->cmd    = TX_CMD_EOP | TX_CMD_IFCS | TX_CMD_RS;

    __asm__ volatile ("" : : : "memory");

    tx_tail = (i + 1) % TX_N;
    wr(REG_TDT, tx_tail);

    return 0;
}

int e1000_recv(void* buf, unsigned int max)
{
    if (mmio == 0)
        return 0;

    volatile struct rx_desc* d = &rx_ring[rx_cur];

    if (!(d->status & DESC_DD))
        return 0;

    unsigned int len = d->length;
    int result = 0;

    if (d->errors == 0 && len > 0)
    {
        if (len > max)
            len = max;

        unsigned char* dst = (unsigned char*)buf;

        for (unsigned int k = 0; k < len; k++)
            dst[k] = rx_buf[rx_cur][k];

        result = (int)len;
    }

    d->status = 0;
    d->errors = 0;
    __asm__ volatile ("" : : : "memory");

    wr(REG_RDT, rx_cur);
    rx_cur = (rx_cur + 1) % RX_N;

    return result;
}