#!/usr/bin/env bash
set -e # Останавливать выполнение при любой ошибке

echo "=== Building NwOS ==="

TARGET="i686-unknown-none-elf"
CFLAGS="--target=${TARGET} -ffreestanding -nostdlib -nostdinc \
 -fno-stack-protector -fno-pic -fno-pie -fno-PIC -fno-PIE \
 -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables \
 -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -mno-sse4 -mno-sse4.1 -mno-sse4.2 \
 -mno-avx -mno-avx2 -mno-mmx -mno-80387 -mno-red-zone -mgeneral-regs-only \
 -O2 -Wall -Wextra -I sys/kernel"

# Проверка наличия необходимых утилит
command -v nasm >/dev/null 2>&1 || { echo "[ERROR] nasm not in PATH. Install: sudo pacman -S nasm"; exit 1; }
command -v clang >/dev/null 2>&1 || { echo "[ERROR] clang not in PATH. Install: sudo pacman -S clang"; exit 1; }
command -v ld.lld >/dev/null 2>&1 || { echo "[ERROR] ld.lld not in PATH. Install: sudo pacman -S lld"; exit 1; }

mkdir -p build

echo "[0/6] Verifying target triple..."
clang --target=${TARGET} -print-target-triple > build/triple.txt
if ! grep -q "i686-unknown-none-elf" build/triple.txt; then
    echo "[WARN] Triple check failed. clang reported:"
    cat build/triple.txt
    echo "       Trying anyway..."
fi

echo "[1/7] Assembling boot sector..."
nasm -f bin sys/boot/boot.asm -o build/boot.bin

echo "[2/7] Assembling kernel entry + IDT stub..."
nasm -f elf32 sys/kernel/kernel_entry.asm -o build/kernel_entry.o

if [ -f sys/kernel/idt.asm ]; then
    nasm -f elf32 sys/kernel/idt.asm -o build/idt.o
fi

echo "Compiling asm console.."
nasm -f elf32 sys/asm_console/asm_console.asm -o build/asm_console.o

echo "[2.5/7] Packing sys/rootfs..."
python3 sys/make_fsdata.py

clang ${CFLAGS} -c sys/kernel/fsdata.c -o build/fsdata.o

echo "[3/7] Compiling C sources..."
clang ${CFLAGS} -c sys/kernel/kernel.c   -o build/kernel.o
echo "compiling graphics..."
clang ${CFLAGS} -c sys/kernel/graphics.c -o build/graphics.o
echo "compiling keyboard..."
clang ${CFLAGS} -c sys/kernel/keyboard.c -o build/keyboard.o
echo "compiling mouse..."
clang ${CFLAGS} -c sys/kernel/mouse.c    -o build/mouse.o
echo "compiling shell.."
clang ${CFLAGS} -c sys/kernel/shell.c    -o build/shell.o
echo "compiling timer.."
clang ${CFLAGS} -c sys/kernel/timer.c    -o build/timer.o
echo "compiling metrics.."
clang ${CFLAGS} -c sys/kernel/metrics.c  -o build/metrics.o
echo "compiling snake..."
clang ${CFLAGS} -c sys/kernel/snake.c    -o build/snake.o
echo "compiling demo3d.."
clang ${CFLAGS} -c sys/kernel/demo3d.c   -o build/demo3d.o
echo "compiling raycast..."
clang ${CFLAGS} -c sys/kernel/raycast.c  -o build/raycast.o
echo "compiling talons.."
clang ${CFLAGS} -c sys/kernel/talons.c   -o build/talons.o
echo "compiling chat..."
clang ${CFLAGS} -c sys/kernel/chat.c     -o build/chat.o
echo "compiling filesystem..."
clang ${CFLAGS} -c sys/kernel/fs.c       -o build/fs.o
echo "compiling editor.."
clang ${CFLAGS} -c sys/kernel/editor.c   -o build/editor.o
echo "compiling k panic.."
clang ${CFLAGS} -c sys/kernel/panic.c    -o build/panic.o
echo "compiling delay.."
clang ${CFLAGS} -c sys/kernel/delay.c    -o build/delay.o
echo "compiling ata.o.."
clang ${CFLAGS} -c sys/kernel/ata.c      -o build/ata.o
echo "compiling time.."
clang ${CFLAGS} -c sys/kernel/time.c     -o build/time.o

if [ -f sys/kernel/idt.c ]; then
    clang ${CFLAGS} -c sys/kernel/idt.c -o build/idt_c.o
fi

echo "      Checking object file format..."
clang --target=${TARGET} -c -x c /dev/null -o build/_probe.o 2>/dev/null || true
grep -q "ELF" build/kernel.o 2>/dev/null || true

echo "[4/7] Linking with ld.lld (ELF32)..."
OBJS=(
    build/kernel_entry.o
    build/asm_console.o
    build/kernel.o
    build/graphics.o
    build/keyboard.o
    build/mouse.o
    build/shell.o
    build/timer.o
    build/metrics.o
    build/snake.o
    build/demo3d.o
    build/raycast.o
    build/talons.o
    build/chat.o
    build/fs.o
    build/editor.o
    build/fsdata.o
    build/panic.o
    build/delay.o
    build/ata.o
    build/time.o
)

[ -f build/idt.o ] && OBJS+=(build/idt.o)
[ -f build/idt_c.o ] && OBJS+=(build/idt_c.o)

ld.lld -m elf_i386 -T linker.ld -nostdlib -o build/kernel.elf "${OBJS[@]}"

echo "[5/7] Extracting flat binary..."
if command -v llvm-objcopy >/dev/null 2>&1; then
    llvm-objcopy -O binary build/kernel.elf build/kernel.bin
else
    objcopy -O binary build/kernel.elf build/kernel.bin
fi

KBSIZE=$(stat -c%s build/kernel.bin 2>/dev/null || stat -f%z build/kernel.bin)
echo "      kernel.bin size: ${KBSIZE} bytes"

echo "[6/7] Creating bootable image..."
python3 makeimg.py --boot build/boot.bin --kernel build/kernel.bin --out build/os.img

echo ""
echo "=== Build complete ==="
echo "  boot.bin:   $(stat -c%s build/boot.bin) bytes"
echo "  kernel.bin: ${KBSIZE} bytes"
echo "  image:      build/os.img"