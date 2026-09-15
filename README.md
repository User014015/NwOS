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
- Compiler!

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
