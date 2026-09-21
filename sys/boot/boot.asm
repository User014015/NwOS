[BITS 16]
[ORG 0x7C00]

KERNEL_SEG   equ 0x1000
KERNEL_SECT equ 127 ; SECTORS

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl
    mov ax, 0x0012
    int 0x10
    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, KERNEL_SECT
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc  disk_error
    in  al, 0x92
    or  al, 2
    and al, 0xFE
    out 0x92, al
    lgdt [gdt_descriptor]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp CODE_SEG:pm_entry

[BITS 32]
pm_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    jmp 0x10000

[BITS 16]
disk_error:
    mov si, err_msg
.next:
    lodsb
    test al, al
    jz  .hang
    mov ah, 0x0E
    int 0x10
    jmp .next
.hang:
    cli
    hlt
    jmp .hang

boot_drive: db 0
err_msg:    db "Disk read error!", 0

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start   ; = 0x08
DATA_SEG equ gdt_data - gdt_start   ; = 0x10

times 510-($-$$) db 0
dw 0xAA55