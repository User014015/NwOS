bits 32

global _start
extern kernel_main
extern __bss_start
extern __bss_end

section .text.start

_start:
    cli

    mov esp, 0x90000

    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call kernel_main

kernel_entry_hang:
    cli
    hlt
    jmp kernel_entry_hang