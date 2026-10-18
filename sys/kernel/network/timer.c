#include "timer.h"
#include "portio.h"

static unsigned long long tsc_per_ms = 0;
static unsigned long long tsc_base   = 0;

static unsigned long long read_tsc(void)
{
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
}

void timer_init(void)
{
    if (tsc_per_ms != 0)
        return;

    unsigned char v = pio_inb(0x61);

    pio_outb(0x61, (unsigned char)(v & 0xFC));     /* gate low, speaker off */
    pio_outb(0x43, 0xB0);                          /* ch2, lo/hi, mode 0    */
    pio_outb(0x42, (unsigned char)(11932 & 0xFF));
    pio_outb(0x42, (unsigned char)(11932 >> 8));
    pio_outb(0x61, (unsigned char)((v & 0xFC) | 0x01));   /* gate high: go  */

    unsigned long long t0 = read_tsc();
    unsigned int guard = 0;

    while (!(pio_inb(0x61) & 0x20))
    {
        if (++guard > 5000000u)
            break;
    }

    unsigned long long t1 = read_tsc();

    pio_outb(0x61, (unsigned char)(v & 0xFC));

    if (guard > 5000000u)
        tsc_per_ms = 2000000ull;           /* PIT did not answer: assume 2 GHz */
    else
        tsc_per_ms = (t1 - t0) / 10;

    if (tsc_per_ms < 1000)                 /* implausible, avoid divide by 0   */
        tsc_per_ms = 2000000ull;

    tsc_base = read_tsc();
}

unsigned long long timer_us(void)
{
    if (tsc_per_ms == 0)
        timer_init();

    return ((read_tsc() - tsc_base) * 1000ull) / tsc_per_ms;
}

unsigned long long timer_ms(void)
{
    if (tsc_per_ms == 0)
        timer_init();

    return (read_tsc() - tsc_base) / tsc_per_ms;
}