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

# boot.asm loads this many sectors of kernel.bin (filesystem starts at LBA 200)
MAX_KERNEL_SECTORS=199

# -I lets net.c / e1000.c / pci.c / timer.c find each other's headers
# wherever they live (drivers/, sys/net/ or sys/kernel/network/).
CFLAGS="--target=$TARGET -m64 -mno-red-zone -mcmodel=kernel -ffreestanding \
-fno-stack-protector -fno-pic -fno-pie -fno-builtin -nostdlib \
-fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -Os \
-I drivers -I sys/net -I sys/kernel/network"

# Every C file that goes into the kernel. Compile and link both use this
# list, so they can never disagree.
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
)

mkdir -p "$BUILD"
rm -f "$BUILD"/*.o "$BUILD"/kernel.elf "$BUILD"/kernel.bin    # no stale objects

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

step "Creating kernel binary..."
llvm-objcopy -O binary "$BUILD/kernel.elf" "$BUILD/kernel.bin" || fail

SIZE=$(stat -c%s "$BUILD/kernel.bin")
LIMIT=$(( MAX_KERNEL_SECTORS * 512 ))
echo "kernel.bin: $SIZE bytes (limit $LIMIT)"
if [ "$SIZE" -gt "$LIMIT" ]; then
    echo "ERROR: kernel is bigger than the $MAX_KERNEL_SECTORS sectors boot.asm loads"
    echo "       (anything past that would be cut off, or overwrite the filesystem at LBA 200)."
    fail
fi

step "Creating NwOS.img..."
dd if=/dev/zero of=NwOS.img bs=512 count=4096 status=none || fail
dd if="$BUILD/boot.bin"   of=NwOS.img bs=512 count=1 conv=notrunc status=none || fail
dd if="$BUILD/kernel.bin" of=NwOS.img bs=512 seek=1  conv=notrunc status=none || fail

echo
echo "============================"
echo "      BUILD SUCCESS"
echo "============================"
ls -l "$BUILD/boot.bin" "$BUILD/kernel.bin" NwOS.img
echo
read -rp "Press Enter to continue..."