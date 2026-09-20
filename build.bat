@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo === Building NwOS ===
echo.

rem F5 integration needs kernel_f5.c because it provides fs_write_bytes().
set "KERNEL_SRC=sys/kernel/kernel.c"

if not exist "%KERNEL_SRC%" (
    echo ERROR: %KERNEL_SRC% was not found.
    echo Put kernel_f5.c in sys/kernel/ or merge it into kernel.c.
    goto error
)

echo [1/19] Assembling bootloader...
nasm -f bin sys/boot/boot.asm -o boot.bin
if errorlevel 1 goto error

echo [2/19] Assembling kernel entry...
nasm -f elf32 sys/kernel/kernel_entry.asm -o kernel_entry.o
if errorlevel 1 goto error

echo [3/19] Compiling kernel and applications...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c "%KERNEL_SRC%" -o kernel.o
if errorlevel 1 goto error
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/Games/slot.c -o slot.o
if errorlevel 1 goto error
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/Games/game_memory.c -o gamememory.o
if errorlevel 1 goto error
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/fileStats/filestats.c -o filestats.o
if errorlevel 1 goto error
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/baseconverter/baseconverter.c -o baseconverter.o
if errorlevel 1 goto error

echo [4/19] Compiling random word...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/wordgenerator/wordgenerator.c -o wordgenerator.o
if errorlevel 1 goto error

echo [5/19] Compiling text redactor...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/Apps/textredactor/textredactor.c -o textredactor.o
if errorlevel 1 goto error

echo [6/19] Compiling keyboard driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c drivers/keyboard.c -o keyboard.o
if errorlevel 1 goto error

echo [7/19] Compiling disk driver...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c drivers/disk.c -o disk.o
if errorlevel 1 goto error

echo [8/19] Compiling kernel panic...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/panic/kernelpanic.c -o kernelpanic.o
if errorlevel 1 goto error

echo [9/19] Compiling NwC runtime...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c compiler/nwc_runtime.c -o nwc_runtime.o
if errorlevel 1 goto error

echo [10/19] Compiling NwC lexer...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c compiler/lexer/lexer.c -o nwc_lexer.o
if errorlevel 1 goto error

echo [11/19] Compiling NwC parser...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c compiler/parser/parser.c -o nwc_parser.o
if errorlevel 1 goto error

echo [12/19] Compiling NwC codegen...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c compiler/codegen/codegen.c -o nwc_codegen.o
if errorlevel 1 goto error

echo [13/19] Compiling NwC kernel bridge...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c compiler/nwc_kernel.c -o nwc_kernel.o
if errorlevel 1 goto error

echo [14/19] Compiling NWO loader...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/nwo_loader.c -o nwo_loader.o
if errorlevel 1 goto error

echo [15/19] Compiling NWO VM...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/nwoloader/nwo_vm.c -o nwo_vm.o
if errorlevel 1 goto error

echo [16/19] Compiling NWO runner...
clang --target=i386-pc-none-elf -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-builtin -c sys/kernel/nwoloader/nwo_runner.c -o nwo_runner.o
if errorlevel 1 goto error

echo [17/19] Linking...
ld.lld -m elf_i386 -T linker.ld ^
 kernel_entry.o kernel.o keyboard.o disk.o kernelpanic.o ^
 wordgenerator.o textredactor.o slot.o gamememory.o filestats.o baseconverter.o ^
 nwc_runtime.o nwc_lexer.o nwc_parser.o nwc_codegen.o nwc_kernel.o ^
 nwo_loader.o nwo_vm.o nwo_runner.o -o kernel.elf
if errorlevel 1 goto error

echo [18/19] Creating kernel binary...
llvm-objcopy -O binary kernel.elf kernel.bin
if errorlevel 1 goto error

for %%A in ("kernel.bin") do set "KERNEL_BYTES=%%~zA"
set /a KERNEL_SECTORS=(KERNEL_BYTES+511)/512
echo Kernel size: !KERNEL_BYTES! bytes
if !KERNEL_SECTORS! GTR 130 (
 echo ERROR: kernel.bin is larger than the 130 sectors loaded by boot.asm.
 echo Increase the DAP sector count in sys/boot/boot.asm.
 goto error
)

echo [19/19] Creating NwOS.img...
REM 4096 sectors = 2 MB.
REM Kernel is loaded at LBA 1; filesystem starts at LBA 200.
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
echo.
dir boot.bin kernel.bin NwOS.img
goto end

:error
echo.
echo ============================
echo       BUILDING FAILED
echo ============================
echo.
pause
exit /b 1

:end
pause
