@echo off

echo Deleting binary trash...
rm -f NwOS.img
rm -f keyboard.o
rm -f kernelpanic.o 
rm -f kernel.o 
rm -f kernel.elf 
rm -f kernel.bin 
rm -f kernel_entry.o 
rm -f boot.bin
rm -f disk.o  

echo Deleted all compiled files!
goto end

:end
pause