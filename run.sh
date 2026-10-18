#!/usr/bin/env bash
echo Launching in qemu..
qemu-system-x86_64 -drive file=NwOS.img,format=raw,index=0,media=disk