ENTRY(_start)

SECTIONS
{
    /*
     * Kernel load address. Must match boot.asm's disk read
     * destination and its protected-mode jump target exactly.
     * Moved from 0x1000 to 0x10000: at 0x1000, the kernel had
     * only ~54 sectors of room before colliding with the boot
     * sector's own code at 0x7C00. kernel.bin outgrew that
     * (58 sectors needed as of this fix), so the load address
     * moved to somewhere with real headroom instead.
     */
    . = 0x10000;

    .text :
    {
        *(.text.start)
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    /*
     * .bss is NOLOAD: it has no file content by definition (it's
     * all zero), so it must NOT be baked into kernel.bin. Without
     * NOLOAD, objcopy -O binary pads the raw binary out by the
     * full size of .bss - tens of KB once static buffers like the
     * terminal scrollback exist - silently pushing the kernel
     * past however many sectors boot.asm loads. kernel_entry.asm
     * zeroes this region manually at startup using the symbols
     * below, since nothing loads it from disk anymore.
     */
    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}