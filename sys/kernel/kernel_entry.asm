[BITS 32]
global _start
extern kernel_main
extern idt_init
extern __bss_start
extern __bss_end

section .text
_start:
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8
    mov al, 3
    out dx, al
    mov dx, 0x3F9
    xor al, al
    out dx, al
    mov dx, 0x3FB
    mov al, 3
    out dx, al
    mov dx, 0x3FA
    mov al, 0xC7
    out dx, al
    mov dx, 0x3FC
    mov al, 0x0B
    out dx, al
    mov dx, 0x3F8
    mov al, 'K'
    out dx, al
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    ; jbe .bss_skip
    xor eax, eax
    rep stosb
.bss_skip:

    mov dx, 0x3F8
    mov al, 'B'
    out dx, al

        mov esp, 0x300000

    mov dx, 0x3F8
    mov al, 'S'
    out dx, al

    call idt_init

    mov dx, 0x3F8
    mov al, 'I'
    out dx, al

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: