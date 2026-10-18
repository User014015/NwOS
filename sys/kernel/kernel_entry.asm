; ============================================================
; NwOS kernel entry - 32-bit protected mode -> 64-bit long mode
;
; boot.asm (unchanged) loads the kernel to 0x10000, switches to
; 32-bit protected mode and jumps here.  This stub:
;   1. checks that the CPU supports long mode
;   2. builds identity-mapped page tables (first 4 GiB, 2 MiB pages)
;   3. enables PAE + EFER.LME + paging, loads a 64-bit GDT
;   4. far-jumps into 64-bit code, enables SSE, zeroes .bss
;   5. calls kernel_main() with a 16-byte aligned stack
;
; Assemble with:  nasm -f elf64 kernel_entry.asm
; ============================================================

global _start
extern kernel_main
extern __bss_start
extern __bss_end

PT_TOTAL_SIZE   equ 4096 * 6        ; PML4 + PDPT + 4 page directories
PTE_PRESENT_RW  equ 0x03
PDE_2M_RW       equ 0x83            ; present | writable | 2 MiB page
PDE_UNCACHED    equ 0x10            ; PCD (used for the MMIO hole)
MMIO_FIRST_PDE  equ 1536            ; 1536 * 2 MiB = 0xC0000000

section .text.start progbits alloc exec nowrite align=16

; ------------------------------------------------------------
; 32-bit part
; ------------------------------------------------------------
bits 32

_start:
    cli
    cld
    mov esp, 0x90000                ; temporary stack, 64-bit stack set later

    ; ---- is CPUID available? (can we flip EFLAGS.ID, bit 21) ----
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

    ; ---- does the CPU support long mode? ----
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29               ; LM bit
    jz .no_long_mode

    ; ---- clear the page-table area ----
    mov edi, pml4
    mov ecx, PT_TOTAL_SIZE / 4
    xor eax, eax
    rep stosd

    ; ---- PML4[0] -> PDPT ----
    mov eax, pdpt
    or  eax, PTE_PRESENT_RW
    mov [pml4], eax

    ; ---- PDPT[0..3] -> PD0..PD3 ----
    xor ecx, ecx
.fill_pdpt:
    mov eax, ecx
    shl eax, 12                     ; * 4096
    add eax, pd
    or  eax, PTE_PRESENT_RW
    mov [pdpt + ecx * 8], eax
    inc ecx
    cmp ecx, 4
    jne .fill_pdpt

    ; ---- 2048 x 2 MiB pages = identity map of 0 .. 4 GiB ----
    ; Everything from 0xC0000000 up is mapped uncached: that is where
    ; PCI BARs / framebuffers / NIC registers live.
    xor ecx, ecx
.fill_pd:
    mov eax, ecx
    shl eax, 21                     ; * 2 MiB
    or  eax, PDE_2M_RW
    cmp ecx, MMIO_FIRST_PDE
    jb .cacheable
    or  eax, PDE_UNCACHED
.cacheable:
    mov [pd + ecx * 8], eax
    inc ecx
    cmp ecx, 2048
    jne .fill_pd

    ; ---- enable long mode ----
    mov eax, pml4
    mov cr3, eax

    mov eax, cr4
    or  eax, 1 << 5                 ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080             ; IA32_EFER
    rdmsr
    or  eax, 1 << 8                 ; LME
    wrmsr

    lgdt [gdt64_ptr]

    mov eax, cr0
    or  eax, 1 << 31                ; PG  (PE is already set)
    mov cr0, eax

    push dword 0x08                 ; 64-bit code selector
    push dword long_mode_entry
    retf

.no_long_mode:
    mov edi, 0xB8000 + 160 * 4      ; row 4 of the text screen
    mov esi, msg_no_long_mode
    mov ah, 0x4F                    ; white on red
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