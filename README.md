```text
  _   _           ____   _____ 
 | \ | |         / __ \ / ____|
 |  \| |_      _| |  | | (___  
 | . ` \ \ /\ / / |  | |\___ \ 
 | |\  |\ V  V /| |__| |____) |
 |_| \_| \_/\_/  \____/|_____/ 
```
# NwOS

<img width="718" height="402" alt="sys" src="https://github.com/user-attachments/assets/c5d27037-071e-4448-bf87-04b05b9481ad" />

A small x86 operating system written in C and NASM.
(Unsupports real hardware, use qemu)

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
- Save files (disk)
- Kernel panic Safe/Fatal
- Compiler

## Compiler

to compile File you need:

learn NwC (tutorial file: NWCTutorial)

## Games
- 1.Guess Number
- 2.Word Game
- 3.Rock Paper Scissors
- 4.Coin Flip
- 5.Dice
- 6.Higher / Lower
- 7.Math Quiz
- 8.Hangman
- 9.Tic Tac Toe

## Build from qemu

Download Qemu

Then type this:

cd C:/Path/to/nwos.img

and then enter this:

qemu-system-i386 -drive file=NwOS.img,format=raw,index=0,media=disk

## Run

Run,

type command help for help

## NwC

Nwc - its a compiler that works in NwOS

## Text redactor + compiler

NwOS — F5 editor/compiler integration

This package adds:
- PS/2 keyboard F5 support (KEY_F5)
- larger filesystem slots (32 sectors/file, 16 KiB/file)
- fs_read_text/fs_read_bytes/fs_write_bytes
- freestanding in-kernel NwC compiler
- compiler integration into TextRedactor
- F5 in TextRedactor

The supplied keyboard.c is a full polling PS/2 Set 1 keyboard driver.
The file supplied by the user as "keyboard.c" is actually the ATA disk driver,
so it is deliberately NOT modified by the F5 patch.

The in-kernel compiler uses the same lexer/parser architecture and emits the
NWO format expected by the current NwOS VM, including function parameter
metadata, assignments, comparisons, logical operations, jumps, loops,
break/continue, built-in nw:: calls and user function calls.

Before testing the new filesystem layout, run:
    format

For safe QEMU testing, use -snapshot so disk changes go to temporary storage.


![NwC Logo](docs/nwc-logo.svg)