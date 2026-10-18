/* NwOS 64-bit kernel linker script
 *
 * The kernel is loaded flat at 0x10000 by boot.asm and identity-mapped
 * (virtual address == physical address).  .bss, the page tables and the
 * stack are NOLOAD so they do not end up inside kernel.bin.
 */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : ALIGN(16)
    {
        KEEP(*(.text.start))
        *(.text .text.*)
    }

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(16)
    {
        *(.data .data.*)
    }

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }

    .pagetables (NOLOAD) : ALIGN(4096)
    {
        *(.pagetables)
    }

    .stack (NOLOAD) : ALIGN(16)
    {
        *(.stack)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame_hdr)
    }
}
