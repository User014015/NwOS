section .multiboot_header progbits alloc noexec nowrite align=8
align 8
header_start:
    dd 0xE85250D6
    dd 0
    dd header_end - header_start
    dd 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))

    align 8
    dw 5
    dw 0
    dd 24
    dd 640
    dd 400
    dd 32

    align 8
    dw 0
    dw 0
    dd 8
header_end:

global _start
extern kernel_main
extern __bss_start
extern __bss_end

PT_TOTAL_SIZE   equ 4096 * 6
PTE_PRESENT_RW  equ 0x03
PDE_2M_RW       equ 0x83
PDE_UNCACHED    equ 0x10
MMIO_FIRST_PDE  equ 1536

section .text.start progbits alloc exec nowrite align=16

bits 32

section .data
global multiboot_info_ptr
multiboot_info_ptr: dd 0

_start:
    mov [multiboot_info_ptr], ebx
    cli
    cld
    mov esp, 0x90000

    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_long_mode

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    mov edi, pml4
    mov ecx, PT_TOTAL_SIZE / 4
    xor eax, eax
    rep stosd

    mov eax, pdpt
    or  eax, PTE_PRESENT_RW
    mov [pml4], eax

    xor ecx, ecx
.fill_pdpt:
    mov eax, ecx
    shl eax, 12
    add eax, pd
    or  eax, PTE_PRESENT_RW
    mov [pdpt + ecx * 8], eax
    inc ecx
    cmp ecx, 4
    jne .fill_pdpt

    xor ecx, ecx
.fill_pd:
    mov eax, ecx
    shl eax, 21
    or  eax, PDE_2M_RW
    cmp ecx, MMIO_FIRST_PDE
    jb .cacheable
    or  eax, PDE_UNCACHED
.cacheable:
    mov [pd + ecx * 8], eax
    inc ecx
    cmp ecx, 2048
    jne .fill_pd

    mov eax, pml4
    mov cr3, eax

    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    lgdt [gdt64_ptr]

    mov eax, cr0
    or  eax, 1 << 31
    mov cr0, eax

    push dword 0x08
    push dword long_mode_entry
    retf

.no_long_mode:
    mov edi, 0xB8000 + 160 * 4
    mov esi, msg_no_long_mode
    mov ah, 0x4F
.print:
    lodsb
    test al, al
    jz .halt
    mov [edi], ax
    add edi, 2
    jmp .print
.halt:
    cli
    hlt
    jmp .halt

msg_no_long_mode:
    db "NwOS: this CPU has no 64-bit mode (QEMU: use qemu-system-x86_64)", 0

align 16
gdt64:
    dq 0x0000000000000000
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dd gdt64

bits 64

align 16
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor eax, eax
    mov fs, ax
    mov gs, ax

    lea rsp, [rel stack_top]
    cld

    fninit
    mov rax, cr0
    and ax, 0xFFFB
    or  ax, 0x0002
    mov cr0, rax
    mov rax, cr4
    or  ax, (1 << 9) | (1 << 10)
    mov cr4, rax

    lea rdi, [rel __bss_start]
    lea rcx, [rel __bss_end]
    sub rcx, rdi
    xor eax, eax
    rep stosb

    xor ebp, ebp
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
section .pagetables nobits alloc noexec write align=4096
pml4:   resb 4096
pdpt:   resb 4096
pd:     resb 4096 * 4

section .stack nobits alloc noexec write align=16
stack_bottom:
        resb 128 * 1024
stack_top: