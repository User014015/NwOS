ENTRY(_start)
SECTIONS
{
    . = 1M;

    .multiboot_header : ALIGN(8)
    {
        KEEP(*(.multiboot_header))
    }

    .text : ALIGN(16)
    {
        *(.text.start)
        *(.text*)
    }

    .rodata : ALIGN(16) { *(.rodata*) }
    .data : ALIGN(16) { *(.data*) }

    .bss : ALIGN(16)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }
}