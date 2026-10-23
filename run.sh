#!/usr/bin/env bash

if [ ! -f img/disk.img ]; then
    echo "Creating 10MB disk.img for filesystem tests..."
    qemu-img create -f raw img/disk.img 10M
fi


qemu-system-x86_64 \
    -boot d \
    -cdrom NwOS.iso\
    -m 128M \
    -nic user,model=e1000 \
    -drive file=img/disk.img,format=raw,if=ide,index=0 \
    -display sdl,grab-mod=rctrl