#!/usr/bin/env bash
set -e

echo "Building GUI..."
clang --target=x86_64-unknown-none-elf -m64 -ffreestanding -nostdlib -fno-pic -fno-pie \
      -mno-sse -mno-sse2 -mno-avx -mgeneral-regs-only -mno-red-zone \
      -c gui/gui.c -o gui/gui.o

# Линкуем напрямую в сырой бинарник (без ELF-заголовков и нулей)
ld.lld -m elf_x86_64 --oformat binary -Ttext 0x400000 gui/gui.o -o gui/gui.bin

# Проверяем размер (он должен быть маленьким, например 1-5 КБ)
SIZE=$(stat -c%s gui/gui.bin)
echo "gui.bin size: $SIZE bytes"
if [ "$SIZE" -gt 100000 ]; then
    echo "ERROR: gui.bin is too big! Something went wrong with linking."
    exit 1
fi

xxd -i -n gui_bin gui/gui.bin > include/gui_embed.h