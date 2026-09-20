@echo off
qemu-system-i386 ^
  -drive format=raw,if=floppy,file=build\os.img ^
  -m 128M -vga std ^
  -no-reboot -no-shutdown ^
  -serial stdio ^
  -d int,cpu_reset -D qemu.log