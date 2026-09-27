[BITS 32]

global asm_console_entry
global asm_reboot

extern gfx_clear
extern gfx_putchar
extern gfx_flip
extern keyboard_getchar
extern THEME_BG
extern THEME_FG

%define CH_ESC   0x1B
%define CH_ENTER 0x0A
%define CH_BS    0x08

%define CHAR_W   8
%define CHAR_H   10
%define MARGIN   8

section .data
s_banner:    db "=== NwOS ASM Console v0.1 ===", 0
s_hint:      db "Commands: help, clear, about, echo X, reboot, exit", 0
s_prompt:    db "asm> ", 0
s_help1:     db "  help    - this list", 0
s_help2:     db "  clear   - clear screen", 0
s_help3:     db "  about   - about this console", 0
s_help4:     db "  echo X  - print X", 0
s_help5:     db "  reboot  - reboot PC", 0
s_help6:     db "  exit    - return to GUI", 0
s_about1:    db "NwOS ASM Console - REPL in pure x86 asm.", 0
s_about2:    db "Uses extern C functions for graphics and keyboard.", 0
s_unknown:   db "Unknown command. Type 'help'.", 0
s_bye:       db "Returning to GUI...", 0
s_reboot:    db "Rebooting now...", 0

c_help:      db "help", 0
c_clear:     db "clear", 0
c_about:     db "about", 0
c_reboot:    db "reboot", 0
c_exit:      db "exit", 0
c_echo:      db "echo ", 0

section .bss
buf:    resb 128
cur_x:  resd 1
cur_y:  resd 1

section .text

asm_console_entry:
    pushad
    pushfd

    call con_clear
    mov esi, s_banner
    call con_puts_nl
    mov esi, s_hint
    call con_puts_nl

.repl:
    mov esi, s_prompt
    call con_puts
    call gfx_flip

    call read_line
    cmp eax, 0xFFFFFFFF
    je  .do_exit
    test eax, eax
    jz  .repl

    mov esi, buf
    mov edi, c_help
    call str_eq
    test al, al
    jnz .do_help

    mov edi, c_clear
    call str_eq
    test al, al
    jnz .do_clear

    mov edi, c_about
    call str_eq
    test al, al
    jnz .do_about

    mov edi, c_reboot
    call str_eq
    test al, al
    jnz .do_reboot

    mov edi, c_exit
    call str_eq
    test al, al
    jnz .do_exit

    mov esi, buf
    mov edi, c_echo
    call str_starts
    test al, al
    jnz .do_echo

    mov esi, s_unknown
    call con_puts_nl
    jmp .repl

.do_help:
    mov esi, s_help1; call con_puts_nl
    mov esi, s_help2; call con_puts_nl
    mov esi, s_help3; call con_puts_nl
    mov esi, s_help4; call con_puts_nl
    mov esi, s_help5; call con_puts_nl
    mov esi, s_help6; call con_puts_nl
    jmp .repl

.do_clear:
    call con_clear
    jmp .repl

.do_about:
    mov esi, s_about1; call con_puts_nl
    mov esi, s_about2; call con_puts_nl
    jmp .repl

.do_echo:
    mov esi, buf
    add esi, 5
    call con_puts_nl
    jmp .repl

.do_reboot:
    mov esi, s_reboot
    call con_puts_nl
    call gfx_flip
    call asm_reboot
    jmp .repl

.do_exit:
    mov esi, s_bye
    call con_puts_nl
    call gfx_flip
    popfd
    popad
    ret

con_clear:
    pushad
    movzx eax, byte [THEME_BG]
    push eax
    call gfx_clear
    add esp, 4
    mov dword [cur_x], MARGIN
    mov dword [cur_y], MARGIN
    call gfx_flip
    popad
    ret

con_putchar:
    pushad
    movzx ecx, al
    cmp ecx, CH_ENTER
    je  .newline
    cmp ecx, CH_BS
    je  .backspace

    movzx eax, byte [THEME_BG]
    push eax
    movzx eax, byte [THEME_FG]
    push eax
    push ecx
    push dword [cur_y]
    push dword [cur_x]
    call gfx_putchar
    add esp, 20

    mov eax, [cur_x]
    add eax, CHAR_W
    mov [cur_x], eax
    cmp eax, 640 - CHAR_W
    jl  .done

.newline:
    mov dword [cur_x], MARGIN
    mov eax, [cur_y]
    add eax, CHAR_H
    mov [cur_y], eax
    cmp eax, 480 - 20
    jl  .done
    call con_clear
.done:
    popad
    ret

.backspace:
    mov eax, [cur_x]
    cmp eax, MARGIN
    jle .done
    sub eax, CHAR_W
    mov [cur_x], eax
    movzx eax, byte [THEME_BG]
    push eax
    push eax
    push dword 32
    push dword [cur_y]
    push dword [cur_x]
    call gfx_putchar
    add esp, 20
    popad
    ret

con_puts:
    pushad
.loop:
    lodsb
    test al, al
    jz  .done
    call con_putchar
    jmp .loop
.done:
    popad
    ret

con_puts_nl:
    call con_puts
    mov al, CH_ENTER
    call con_putchar
    call gfx_flip
    ret

read_line:
    pushad
    lea edi, [buf]
    xor ecx, ecx
.loop:
    call keyboard_getchar
    test al, al
    jz  .loop

    cmp al, CH_ESC
    je  .on_esc
    cmp al, CH_ENTER
    je  .on_enter
    cmp al, CH_BS
    je  .on_bs

    cmp ecx, 126
    jge .loop

    mov [edi + ecx], al
    inc ecx
    call con_putchar
    call gfx_flip
    jmp .loop

.on_bs:
    test ecx, ecx
    jz  .loop
    dec ecx
    mov al, CH_BS
    call con_putchar
    call gfx_flip
    jmp .loop

.on_enter:
    mov byte [edi + ecx], 0
    mov al, CH_ENTER
    call con_putchar
    call gfx_flip
    mov [esp + 28], ecx
    popad
    ret

.on_esc:
    popad
    mov eax, 0xFFFFFFFF
    ret

str_eq:
    push esi
    push edi
.loop:
    mov al, [esi]
    cmp al, [edi]
    jne .no
    test al, al
    jz  .yes
    inc esi
    inc edi
    jmp .loop
.yes:
    pop edi
    pop esi
    mov al, 1
    ret
.no:
    pop edi
    pop esi
    xor al, al
    ret

str_starts:
    push esi
    push edi
.loop:
    mov al, [edi]
    test al, al
    jz  .yes
    cmp al, [esi]
    jne .no
    inc esi
    inc edi
    jmp .loop
.yes:
    pop edi
    pop esi
    mov al, 1
    ret
.no:
    pop edi
    pop esi
    xor al, al
    ret

asm_reboot:
    cli
.wait:
    in  al, 0x64
    test al, 2
    jnz .wait
    mov al, 0xFE
    out 0x64, al
    mov eax, 0
    mov cr0, eax
    hlt
    jmp $