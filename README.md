```text
  _   _           ____   _____ 
 | \ | |         / __ \ / ____|
 |  \| |_      _| |  | | (___  
 | . ` \ \ /\ / / |  | |\___ \ 
 | |\  |\ V  V /| |__| |____) |
 |_| \_| \_/\_/  \____/|_____/ 
```
# NwOS

<img width="699" height="399" alt="sys" src="https://github.com/user-attachments/assets/cb43dc54-5446-405c-bbc6-68a7951a5541" />


A small x86 operating system written in C and NASM.

## Features

- VGA text mode
- PS/2 keyboard
- Command shell
- Calculator
- Command history
- Games
- Colors
- Time
- Automatic terminal scrolling

## Build from virtual box
build:

make sure that you have Oracle Virtualbox

Download .img

open cmd

type this:

cd "C:\road\to\VirtualBox"

then type this:

VBoxManage convertfromraw "C:\road\to\nwos.img" "C:\road\to\NwOS.vdi" --format VDI

it gonna create VDI file

then you need to create a new machine named "NwOS" and put vdi as hard drive

## Build from qemu

download qemu from ucrt

then in ucrt run this:

cd C:/Road/to/nwos.img

and then enter this:

qemu-system-i386 -drive file=nwos.img,format=raw,index=0,media=disk

## Run

Run,
type command help for help
