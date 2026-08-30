bbits 16
org 0x7C00

start:
    cli

    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; The BIOS passes the boot disk number to DL
    mov [boot_drive], dl

    ; Checking BIOS Extensions (EDD) support
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]

    int 0x13
    jc disk_error

    cmp bx, 0xAA55
    jne disk_error

    ; Loading kernel.bin
    ;
    ; LBA 1 = second sector of the disk
    ; Load 16 sectors
    ; to physical address 0x1000

    mov si, disk_address_packet
    mov dl, [boot_drive]
    mov ah, 0x42

    int 0x13
    jc disk_error

    ; protected mode
    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode


; --------------------------------
; Disk Error
; --------------------------------

disk_error:
    mov si, error_message

.print:
    lodsb

    or al, al
    jz .hang

    mov ah, 0x0E
    int 0x10

    jmp .print

.hang:
    cli
    hlt
    jmp .hang


; --------------------------------
; Protected mode
; --------------------------------

bits 32

protected_mode:

    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    ; kernel.bin загружен по адресу 0x1000
    call 0x1000

.hang32:
    cli
    hlt
    jmp .hang32


; --------------------------------
; Variables
; --------------------------------

bits 16

boot_drive db 0

error_message db "Disk error!", 0


; --------------------------------
; BIOS Extended Disk Address Packet
; --------------------------------

disk_address_packet:

    db 0x10             ; size DAP = 16 bytes
    db 0                ; reserved

    dw 16               ; num of sectors

    dw 0x1000           ; offset load
    dw 0x0000           ; segment load

    dd 1                ; LBA = 1
    dd 0                ; the upper part of the LBA


; --------------------------------
; GDT
; --------------------------------

gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


; --------------------------------
; Boot signature
; --------------------------------

times 510 - ($ - $$) db 0

dw 0xAA55