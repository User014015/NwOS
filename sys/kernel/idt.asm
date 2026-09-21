[BITS 32]
global idt_load
global isr_default
global irq0_stub

extern irq0_handler

section .text
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

isr_default:
    pusha
    mov edi, 0xB8000 + 158
    mov word [edi], 0x4F45
    popa
    iret

irq0_stub:
    pusha
    call irq0_handler
    popa
    iret