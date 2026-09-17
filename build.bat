@echo off

setlocal

echo === Building NwOS ===
echo.

echo [1/14] Assembling bootloader...

nasm -f bin sys/boot/boot.asm -o boot.bin

if errorlevel 1 goto error


echo [2/14] Assembling kernel entry...

nasm -f elf32 sys/kernel/kernel_entry.asm -o kernel_entry.o

if errorlevel 1 goto error


echo [3/14] Compiling kernel...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/kernel.c -o kernel.o
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/Games/slot.c -o slot.o
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/Games/game_memory.c -o gamememory.o
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/fileStats/filestats.c -o filestats.o
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/baseconverter/baseconverter.c -o baseconverter.o

if errorlevel 1 goto error


echo [4/14] Compiling random word...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/wordgenerator/wordgenerator.c -o wordgenerator.o

if errorlevel 1 goto error


echo [5/14] Compiling text redactor...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/Apps/textredactor/textredactor.c -o textredactor.o

if errorlevel 1 goto error


echo [6/14] Compiling keyboard driver...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/keyboard.c -o keyboard.o

if errorlevel 1 goto error


echo [7/14] Compiling disk driver...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c drivers/disk.c -o disk.o

if errorlevel 1 goto error


echo [8/14] Compiling kernel panic...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/panic/kernelpanic.c -o kernelpanic.o

if errorlevel 1 goto error


echo [9/14] Compiling NWO loader...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/nwo_loader.c -o nwo_loader.o

if errorlevel 1 goto error


echo [10/14] Compiling NWO VM...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/nwoloader/nwo_vm.c -o nwo_vm.o

if errorlevel 1 goto error


echo [11/14] Compiling NWO runner...

clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -c sys/kernel/nwoloader/nwo_runner.c -o nwo_runner.o

if errorlevel 1 goto error


echo [12/14] Linking...
ld.lld -m elf_i386 -T linker.ld kernel_entry.o kernel.o keyboard.o disk.o kernelpanic.o wordgenerator.o textredactor.o nwo_loader.o nwo_vm.o nwo_runner.o slot.o gamememory.o filestats.o baseconverter.o -o kernel.elf

if errorlevel 1 goto error


echo [13/14] Creating kernel binary...

llvm-objcopy -O binary kernel.elf kernel.bin

if errorlevel 1 goto error


echo [14/14] Creating NwOS.img...

REM 4096 sectors = 2 MB.
REM Kernel is loaded around LBA 1-130.
REM Filesystem currently starts at LBA 200.
REM Everything is contained in this virtual image.

dd if=/dev/zero of=NwOS.img bs=512 count=4096

if errorlevel 1 goto error


dd if=boot.bin of=NwOS.img bs=512 count=1 conv=notrunc

if errorlevel 1 goto error


dd if=kernel.bin of=NwOS.img bs=512 seek=1 conv=notrunc

if errorlevel 1 goto error


echo.
echo ============================
echo       BUILD SUCCESS
echo ============================

dir boot.bin kernel.bin NwOS.img

goto end


:error

echo.
echo ============================
echo       BUILDING FAILED
echo ============================

pause

exit /b 1


:end

pause