set -e

if [ ! -f disk.img ]; then
    echo "Creating disk.img..."
    qemu-img create -f raw disk.img 10M
fi

qemu-system-i386 -drive format=raw,if=floppy,file=build/os.img -drive format=raw,if=ide,file=disk.img,index=0,media=disk -m 512M -vga std -no-reboot -no-shutdown -serial stdio