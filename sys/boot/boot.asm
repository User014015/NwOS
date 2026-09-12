bits 16
org 0x7C00

start:
    cli
    cld

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    ; --- DEBUG CHECKPOINT 1: segments set up ---
    mov ah, 0x0E
    mov al, '1'
    mov bh, 0
    int 0x10

; =========================================================
; Switch to 80x50 text mode (8x8 font). Must happen here, in
; real mode - BIOS video services (int 0x10) aren't available
; once we're in protected mode. kernel.c's terminal assumes
; HEIGHT = 50 rows; without this the hardware stays in the
; BIOS default 80x25 mode and output gets rendered below the
; visible screen (looks like a black screen, nothing ever
; appears).
; =========================================================

    mov ax, 0x0003      ; AH=00 set video mode, AL=03 80x25 16-color text
    int 0x10

    mov ax, 0x1112      ; AH=11 font ops, AL=12 load 8x8 font -> 80x50
    xor bx, bx
    int 0x10

    ; --- DEBUG CHECKPOINT 2: video mode switched ---
    mov ah, 0x0E
    mov al, '2'
    mov bh, 0
    int 0x10


; =========================================================
; Loading message
; =========================================================

    mov si, loading_message

print_loading:
    lodsb
    test al, al
    jz check_edd

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp print_loading


; =========================================================
; Check EDD
; =========================================================

check_edd:

    mov dl, [boot_drive]

    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13

    jc disk_error

    cmp bx, 0xAA55
    jne disk_error

    test cx, 1
    jz disk_error

    ; --- DEBUG CHECKPOINT 3: EDD check passed ---
    mov ah, 0x0E
    mov al, '3'
    mov bh, 0
    int 0x10


; =========================================================
; Reset disk
; =========================================================

    xor ah, ah
    mov dl, [boot_drive]
    int 0x13

    jc disk_error

    ; --- DEBUG CHECKPOINT 4: disk reset done ---
    mov ah, 0x0E
    mov al, '4'
    mov bh, 0
    int 0x10


; =========================================================
; Load kernel
;
; Load 50 sectors (25 KB) - kernel.bin is currently exactly 40
; sectors (20,480 bytes), so this covers it with a little room
; to grow. Deliberately kept small and far from the ~120-sector
; real-mode 64 KB segment-boundary limit (buffer is 0000:1000)
; while we isolate a hang inside the int 0x13 AH=42h call at
; larger sector counts.
; LBA 1
; Destination 0000:1000
; =========================================================


    xor ax, ax
    mov ds, ax
    mov es, ax

    mov si, disk_address_packet
    mov dl, [boot_drive]

    mov ah, 0x42
    int 0x13

    jc disk_error

    ; --- DEBUG CHECKPOINT 5: kernel read succeeded ---
    mov ah, 0x0E
    mov al, '5'
    mov bh, 0
    int 0x10


; =========================================================
; Kernel loaded message
; =========================================================

    mov si, loaded_message

print_loaded:
    lodsb
    test al, al
    jz enter_protected_mode

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp print_loaded


; =========================================================
; Disk error
; =========================================================

disk_error:

    mov si, error_message

print_error:
    lodsb
    test al, al
    jz boot_hang

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp print_error


boot_hang:
    cli
    hlt
    jmp boot_hang


; =========================================================
; Protected mode
; =========================================================

enter_protected_mode:

    ; --- DEBUG CHECKPOINT 6: about to enter protected mode ---
    mov ah, 0x0E
    mov al, '6'
    mov bh, 0
    int 0x10

    cli

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode


; =========================================================
; 32-bit mode
; =========================================================

bits 32

protected_mode:

    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000


    ; -----------------------------------------------------
    ; Diagnostic:
    ; write "PM OK" directly to VGA memory
    ; -----------------------------------------------------

    mov edi, 0xB8000

    mov word [edi + 0],  'P' | (0x0A << 8)
    mov word [edi + 2],  'M' | (0x0A << 8)
    mov word [edi + 4],  ' ' | (0x0A << 8)
    mov word [edi + 6],  'O' | (0x0A << 8)
    mov word [edi + 8],  'K' | (0x0A << 8)


    ; -----------------------------------------------------
    ; Jump to kernel entry at 0x1000
    ; -----------------------------------------------------

    mov eax, 0x1000
    jmp eax


protected_hang:
    cli
    hlt
    jmp protected_hang


; =========================================================
; 16-bit data
; =========================================================

bits 16

boot_drive:
    db 0

loading_message:
    db "Loading NwOS kernel...", 13, 10, 0

loaded_message:
    db "Kernel loaded!", 13, 10, 0

error_message:
    db "DISK ERROR!", 13, 10, 0


; =========================================================
; EDD Disk Address Packet
; =========================================================

disk_address_packet:

    db 0x10
    db 0x00

    dw 50

    dw 0x1000
    dw 0x0000

    dd 1
    dd 0


; =========================================================
; GDT
; =========================================================

gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start


; =========================================================
; Boot signature
; =========================================================

times 510 - ($ - $$) db 0

dw 0xAA55