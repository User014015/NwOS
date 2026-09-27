@echo off
if not exist disk.img (
    qemu-img create -f raw disk.img 10M
)

qemu-system-i386 ^
  -drive format=raw,if=floppy,file=build\os.img ^
  -drive format=raw,if=ide,file=disk.img,index=0,media=disk ^
  -m 256M -vga std ^
  -no-reboot -no-shutdown ^
  -serial stdio