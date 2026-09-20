#ifndef METRICS_H
#define METRICS_H

void metrics_init(void);
void metrics_frame_begin(void);
void metrics_frame_end(void);

unsigned int metrics_cpu_pct(void);
unsigned int metrics_ram_pct(void);
unsigned int metrics_ram_used_kb(void);

#endif