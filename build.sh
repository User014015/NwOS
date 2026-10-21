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
    sys/kernel/Apps/Games/snake.c
)

mkdir -p "$BUILD"
mkdir -p img
rm -f "$BUILD"/*.o "$BUILD"/kernel.elf "$BUILD"/kernel.bin
rm -f NwOS.iso

STEPS=$(( ${#SOURCES[@]} + 5 ))
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

step "Preparing ISO tree..."

rm -rf iso
mkdir -p iso/boot/limine
mkdir -p iso/EFI/BOOT

cp "$BUILD/kernel.elf" iso/boot/kernel.elf

cp /usr/share/limine/limine-bios.sys        iso/boot/limine/ || fail
cp /usr/share/limine/limine-bios-cd.bin     iso/boot/limine/ || fail
cp /usr/share/limine/limine-uefi-cd.bin     iso/boot/limine/ || fail
cp /usr/share/limine/BOOTX64.EFI            iso/EFI/BOOT/    || fail
cp /usr/share/limine/BOOTIA32.EFI           iso/EFI/BOOT/    || fail

cat > iso/boot/limine/limine.conf <<EOF
timeout: 0

/NwOS
    protocol: multiboot2
    path: boot():/boot/kernel.elf
    framebuffer: 640x400x32
EOF

step "Building ISO with xorriso..."

xorriso -as mkisofs -R -r -J \
    -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    -hfsplus -apm-block-size 2048 \
    --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    iso -o NwOS.iso || fail

if [ -x /usr/share/limine/limine ]; then
    /usr/share/limine/limine bios-install NwOS.iso || fail
fi

if [ ! -f NwOS.iso ]; then
    echo "ERROR: ISO creation failed"
    fail
fi

echo
echo "============================"
echo "      BUILD SUCCESS"
echo "============================"
ls -lh NwOS.iso "$BUILD/kernel.elf"
echo
echo "ISO contents in /boot/limine:"
ls -la iso/boot/limine/