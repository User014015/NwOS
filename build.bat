@echo off
set local

echo === Building NwOS ===

echo [1/6] Assembling kernel entry...
nasm -f elf32 kernel_entry.asm -o kernel_entry.o

if errorlevel 1 goto error 

echo [2/6] Compiling kernel...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c kernel.c -o kernel.o
if errorlevel 1 goto error 

echo [3/6] Compiling keyboard driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/keyboard.c -o keyboard.o
if errorlevel 1 goto error

echo [4/6] Compiling disk driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/disk.c -o disk.o
if errorlevel 1 goto error

echo [5/6] Linking...
ld.lld -m elf_i386 -T linker.ld kernel_entry.o kernel.o keyboard.o disk.o -o kernel.elf
if errorlevel 1 goto error

echo [6/6] Creating kernel binary...
llvm-objcopy -O binary kernel.elf kernel.bin
if errorlevel 1 goto error

echo .
echo ============================
echo BUILD SUCCESS
echo ============================

dir boot.bin kernel.bin

goto end

:error

echo .
echo ===========================
echo BUILDING FAILED
echo ===========================

pause
exit /b 1

:end
pause