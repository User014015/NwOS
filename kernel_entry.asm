bits 32

global _start
extern kernel_main

section .text

_start:
    cli

    call kernel_main

.hang:
    hlt
    jmp .hang