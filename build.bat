@echo off
setlocal enabledelayedexpansion

echo === Building NwOS ===
set TARGET=i686-unknown-none-elf
set CFLAGS=--target=%TARGET% -ffreestanding -nostdlib -nostdinc ^
 -fno-stack-protector -fno-pic -fno-pie -fno-PIC -fno-PIE ^
 -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables ^
 -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4 -mno-sse4.1 -mno-sse4.2 ^
 -mno-avx -mno-avx2 -mno-mmx -mno-80387 -mno-red-zone -mgeneral-regs-only ^
 -O2 -Wall -Wextra -I sys\kernel

where nasm >nul 2>nul || (echo [ERROR] nasm not in PATH & exit /b 1)
where clang >nul 2>nul || (echo [ERROR] clang not in PATH & exit /b 1)
where ld.lld >nul 2>nul || (echo [ERROR] ld.lld not in PATH. Install: pacman -S mingw-w64-ucrt-x86_64-lld & exit /b 1)

if not exist build mkdir build

echo [0/6] Verifying target triple...
clang --target=%TARGET% -print-target-triple > build\triple.txt
findstr /C:"i686-unknown-none-elf" build\triple.txt >nul
if errorlevel 1 (
    echo [WARN] Triple check failed. clang reported:
    type build\triple.txt
    echo        Trying anyway...
)

echo [1/6] Assembling boot sector...
nasm -f bin sys\boot\boot.asm -o build\boot.bin
if errorlevel 1 goto :error

echo [2/6] Assembling kernel entry + IDT stub...
nasm -f elf32 sys\kernel\kernel_entry.asm -o build\kernel_entry.o
if errorlevel 1 goto :error
if exist sys\kernel\idt.asm (
    nasm -f elf32 sys\kernel\idt.asm -o build\idt.o
    if errorlevel 1 goto :error
)

echo [3/6] Compiling C sources...
clang %CFLAGS% -c sys\kernel\kernel.c   -o build\kernel.o
if errorlevel 1 goto :error
clang %CFLAGS% -c sys\kernel\graphics.c -o build\graphics.o
if errorlevel 1 goto :error
clang %CFLAGS% -c sys\kernel\keyboard.c -o build\keyboard.o
if errorlevel 1 goto :error
clang %CFLAGS% -c sys\kernel\mouse.c    -o build\mouse.o
if errorlevel 1 goto :error
clang %CFLAGS% -c sys\kernel\shell.c    -o build\shell.o
clang %CFLAGS% -c sys\kernel\timer.c   -o build\timer.o
clang %CFLAGS% -c sys\kernel\metrics.c -o build\metrics.o
clang %CFLAGS% -c sys\kernel\snake.c -o build\snake.o
clang %CFLAGS% -c sys\kernel\demo3d.c  -o build\demo3d.o
clang %CFLAGS% -c sys\kernel\raycast.c -o build\raycast.o
clang %CFLAGS% -c sys\kernel\talons.c -o build\talons.o
clang %CFLAGS% -c sys\kernel\chat.c -o build\chat.o
if errorlevel 1 goto :error
if errorlevel 1 goto :error
if exist sys\kernel\idt.c (
    clang %CFLAGS% -c sys\kernel\idt.c  -o build\idt_c.o
    if errorlevel 1 goto :error
)
echo      Checking object file format...
clang --target=%TARGET% -c -x c nul -o build\_probe.o 2>nul
for %%F in (build\kernel.o) do (
    findstr /M /C:"ELF" "%%F" >nul 2>nul
)

echo [4/6] Linking with ld.lld (ELF32)...
set OBJS=build\kernel_entry.o build\kernel.o build\graphics.o build\keyboard.o build\mouse.o build\shell.o build\timer.o build\metrics.o build/snake.o build/demo3d.o build/raycast.o build/talons.o build/chat.o
if exist build\idt.o   set OBJS=!OBJS! build\idt.o
if exist build\idt_c.o set OBJS=!OBJS! build\idt_c.o

ld.lld -m elf_i386 -T linker.ld -nostdlib -o build\kernel.elf !OBJS!
if errorlevel 1 goto :error

echo [5/6] Extracting flat binary...
llvm-objcopy -O binary build\kernel.elf build\kernel.bin 2>nul
if errorlevel 1 (
    objcopy -O binary build\kernel.elf build\kernel.bin
    if errorlevel 1 goto :error
)

for %%F in (build\kernel.bin) do set KBSIZE=%%~zF
echo      kernel.bin size: %KBSIZE% bytes

echo [6/6] Creating bootable image...
copy /b build\boot.bin + build\kernel.bin build\os.img >nul
if errorlevel 1 goto :error

echo.
echo === Build complete ===
for %%F in (build\boot.bin)   do echo   boot.bin:   %%~zF bytes
for %%F in (build\kernel.bin) do echo   kernel.bin: %%~zF bytes
echo   image:      build\os.img
goto :eof

echo.
echo === Build complete ===
for %%F in (build\boot.bin)   do echo   boot.bin:   %%~zF bytes
for %%F in (build\kernel.bin) do echo   kernel.bin: %%~zF bytes  ^(first bytes: !HEAD!^)
echo   image:      build\os.img
goto :eof

:error
echo.
echo [ERROR] Build failed!
exit /b 1