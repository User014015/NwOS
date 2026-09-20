#ifndef NWC_KERNEL_H
#define NWC_KERNEL_H

#define NWC_MAX_SOURCE_SIZE 16384
#define NWC_MAX_NWO_SIZE    131072

int nwc_compile_source(
    const char* source,
    unsigned int source_size,
    unsigned char* nwo_buffer,
    unsigned int nwo_capacity,
    unsigned int* nwo_size
);

#endif
