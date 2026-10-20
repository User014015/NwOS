#!/usr/bin/env bash

fail() {
    echo
    echo "============================"
    echo "      BUILDING FAILED"
    echo "============================"
    read -rp "Press Enter to continue..."
    exit 1
}

cd "$(dirname "$0")" || fail

echo "=== Building NwOS (x86-64) ==="
echo

BUILD=build
TARGET=x86_64-unknown-none-elf

MAX_KERNEL_SECTORS=199

CFLAGS="--target=$TARGET -m64 -mno-red-zone -mcmodel=kernel -ffreestanding \
-fno-stack-protector -fno-pic -fno-pie -fno-builtin -nostdlib \
-fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Os \
-I drivers -I sys/net -I sys/kernel/network"

SOURCES=(
    sys/kernel/kernel.c
    sys/kernel/nwo_loader.c
    sys/kernel/panic/kernelpanic.c
    sys/kernel/nwoloader/nwo_vm.c
    sys/kernel/nwoloader/nwo_runner.c
    sys/kernel/Apps/Games/slot.c
    sys/kernel/Apps/Games/game_memory.c
    sys/kernel/Apps/fileStats/filestats.c
    sys/kernel/Apps/baseconverter/baseconverter.c
    sys/kernel/Apps/wordgenerator/wordgenerator.c
    sys/kernel/Apps/textredactor/textredactor.c
    drivers/keyboard.c
    drivers/disk.c
    sys/kernel/network/e1000.c
    sys/kernel/network/pci.c
    sys/kernel/network/timer.c
    sys/kernel/network/net.c
    sys/kernel/fb.c
)

mkdir -p "$BUILD"
mkdir -p img
rm -f "$BUILD"/*.o "$BUILD"/kernel.elf "$BUILD"/kernel.bin

STEPS=$(( ${#SOURCES[@]} + 6 ))
N=0
step() { N=$((N + 1)); echo "[$N/$STEPS] $1"; }

step "Assembling bootloader..."
nasm -f bin sys/boot/boot.asm -o "$BUILD/boot.bin" || fail

step "Assembling kernel entry..."
nasm -f elf64 sys/kernel/kernel_entry.asm -o "$BUILD/kernel_entry.o" || fail

OBJS=("$BUILD/kernel_entry.o")

for src in "${SOURCES[@]}"; do
    step "Compiling $src"

    if [ ! -f "$src" ]; then
        echo "ERROR: missing source file: $src"
        fail
    fi

    obj="$BUILD/$(basename "${src%.c}").o"
    clang $CFLAGS -c "$src" -o "$obj" || fail
    OBJS+=("$obj")
done

step "Linking..."
ld.lld -m elf_x86_64 -nostdlib -z noexecstack --gc-sections -T linker.ld \
    "${OBJS[@]}" -o "$BUILD/kernel.elf" || fail

step "Creating ISO (text mode)..."
rm -rf iso
mkdir -p iso/boot/grub
cp "$BUILD/kernel.elf" iso/boot/kernel.elf

cat > iso/boot/grub/grub.cfg <<EOF
set timeout=0
set default=0
set gfxpayload=text

menuentry "NwOS (text mode)" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o NwOS.iso iso || fail

step "Creating ISO (GUI mode)..."
cat > iso/boot/grub/grub.cfg <<EOF
set timeout=0
set default=0
set gfxpayload=640x400x32

menuentry "NwOS (GUI mode)" {
    multiboot2 /boot/kernel.elf
    boot
}
EOF

grub-mkrescue -o NwOS-gui.iso iso || fail

echo
echo "============================"
echo "      BUILD SUCCESS"
echo "============================"
ls -lh NwOS.iso NwOS-gui.iso "$BUILD/kernel.elf"