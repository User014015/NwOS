  _   _           ____   _____ 
 | \ | |         / __ \ / ____|
 |  \| |_      _| |  | | (___  
 | . ` \ \ /\ / / |  | |\___ \ 
 | |\  |\ V  V /| |__| |____) |
 |_| \_| \_/\_/  \____/|_____/ 

 A x86 operating-system

# NwOS

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

## Build
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

## Run

Run,
type command help for help
