ENTRY(_start)
SECTIONS
{
    . = 0x10000;

    .text : ALIGN(16) {
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(16) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(16) {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(16) {
        PROVIDE(__bss_start = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        PROVIDE(__bss_end = .);
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note*)
        *(.comment)
    }

    kernel_end = .;
}