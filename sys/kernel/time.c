#include "shell.h"
#include "delay.h"
#include "time.h"
// timer
void time(int seconds) {
    if (seconds <= 0) return;
    if (seconds > 3600) seconds = 3600;
    for (int i = 0; i < seconds; i++)
        delay_ms(1000);
}