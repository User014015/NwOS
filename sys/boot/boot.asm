bits 16
org  0x7C00

STAGE2_SEG      equ 0x07E0
STAGE2_LBA      equ 1
STAGE2_SECTORS  equ 8
PART_COUNT_OFF  equ 458

start:
    cli   
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    cld
    sti

    mov [boot_drive], dl

    mov si, msg_banner
    call print_str

    mov dl, [boot_drive]
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc  .no_edd
    cmp bx, 0xAA55
    jne .no_edd
    test cx, 1
    jz  .no_edd

    mov dl, [boot_drive]
    xor ah, ah
    int 0x13

    mov word  [dap + 2], STAGE2_SECTORS
    mov word  [dap + 4], 0                  
    mov word  [dap + 6], STAGE2_SEG
    mov dword [dap + 8], STAGE2_LBA
    mov dword [dap + 12], 0

    mov si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc  disk_error

    mov si, msg_ok
    call print_str

    mov dl, [boot_drive]
    jmp 0x0000:0x7E00

.no_edd:
    mov si, msg_no_edd
    call print_str
    jmp hang

disk_error:
    mov si, msg_err
    call print_str

hang:
    cli
    hlt
    jmp hang

print_str:
    lodsb
    test al, al
    jz   .done
    mov  ah, 0x0E
    mov  bh, 0
    int  0x10
    jmp  print_str
.done:
    ret

boot_drive: db 0

align 2
dap:
    db 0x10
    db 0
    dw 0
    dw 0
    dw 0
    dd 0
    dd 0

msg_banner: db "NwOS MBR bootloader", 13, 10, 0
msg_ok:     db "Loading stage2...", 13, 10, 0
msg_no_edd: db "ERR: no INT13 extensions (EDD).", 13, 10
            db "Need a BIOS from 1998+ (or QEMU).", 13, 10, 0
msg_err:    db "ERR: disk read failed.", 13, 10, 0

times 440 - ($ - $$) db 0

disk_sig:   db 'N', 'w', 'O', 'S'
reserved:   dw 0

part_table:
    db 0x80                   
    db 0x00, 0x02, 0x00      
    db 0x83 
    db 0xFE, 0xFF, 0xFF
    dd 1
    dd 2047

    times 48 db 0

dw 0xAA55 