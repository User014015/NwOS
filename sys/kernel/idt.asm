[BITS 32]
global idt_load
global isr_default

section .text
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr_default:
    pusha
    mov al, 0x20
    out 0x20, al
    out 0xA0, al
    popa
    iret