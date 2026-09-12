@echo off
setlocal

echo === Building NwOS ===

echo [1/9] Assembling bootloader...
nasm -f bin sys/boot/boot.asm -o boot.bin
if errorlevel 1 goto error

echo [2/9] Assembling kernel entry...
nasm -f elf32 sys/kernel/kernel_entry.asm -o kernel_entry.o
if errorlevel 1 goto error

echo [3/9] Compiling kernel...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/kernel.c -o kernel.o
if errorlevel 1 goto error

echo [4/9] Compiling keyboard driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/keyboard.c -o keyboard.o
if errorlevel 1 goto error

echo [5/9] Compiling disk driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/disk.c -o disk.o
if errorlevel 1 goto error

echo [6/9] Compiling kernel panic...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/panic/kernelpanic.c -o kernelpanic.o
if errorlevel 1 goto error

echo [7/9] Linking...
ld.lld -m elf_i386 -T linker.ld kernel_entry.o kernel.o keyboard.o disk.o kernelpanic.o -o kernel.elf
if errorlevel 1 goto error

echo [8/9] Creating kernel binary...
llvm-objcopy -O binary kernel.elf kernel.bin
if errorlevel 1 goto error

echo [9/9] Creating NwOS.img...

REM 4096 sectors = 2 MB. Comfortably covers the kernel's 130-sector
REM boot load (LBA 1-130) and the filesystem area (LBA 200-217),
REM with plenty of headroom for both to keep growing.
dd if=/dev/zero of=NwOS.img bs=512 count=4096
if errorlevel 1 goto error

dd if=boot.bin of=NwOS.img bs=512 count=1 conv=notrunc
if errorlevel 1 goto error

dd if=kernel.bin of=NwOS.img bs=512 seek=1 conv=notrunc
if errorlevel 1 goto error

echo.
echo ============================
echo BUILD SUCCESS
echo ============================

dir boot.bin kernel.bin NwOS.img

goto end

:error

echo.
echo ===========================
echo BUILDING FAILED
echo ===========================

pause
exit /b 1

:end
pause