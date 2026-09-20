#include "metrics.h"
#include "timer.h"
extern char kernel_end;

#define FRAME_BUDGET_US  16667
#define RAM_BUDGET_KB    1024

static unsigned short frame_start = 0;
static unsigned int   frame_us_avg = 0;
static int            frame_samples = 0;

static unsigned int cpu_pct = 0;
static unsigned int ram_pct = 0;
static unsigned int ram_used_kb = 0;

void metrics_init(void) {
    unsigned int used = (unsigned int)&kernel_end - 0x10000u;
    ram_used_kb = used / 1024u;
    ram_pct = ram_used_kb * 100u / RAM_BUDGET_KB;
    if (ram_pct > 100) ram_pct = 100;
}

void metrics_frame_begin(void) {
    frame_start = timer_read();
}

void metrics_frame_end(void) {
    unsigned int us = timer_elapsed_us(frame_start, timer_read());

    if (frame_samples < 16) {
        frame_us_avg = (frame_us_avg * (unsigned)frame_samples + us)
                       / (unsigned)(frame_samples + 1);
        frame_samples++;
    } else {
        frame_us_avg = (frame_us_avg * 15u + us) / 16u;
    }

    cpu_pct = frame_us_avg * 100u / FRAME_BUDGET_US;
    if (cpu_pct > 100) cpu_pct = 100;
}

unsigned int metrics_cpu_pct(void)     { return cpu_pct;     }
unsigned int metrics_ram_pct(void)     { return ram_pct;     }
unsigned int metrics_ram_used_kb(void) { return ram_used_kb; }