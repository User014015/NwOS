#ifndef PANIC_H
#define PANIC_H

void kernel_panic_safe(const char *msg);
void kernel_panic_fatal(const char *msg);

#endif