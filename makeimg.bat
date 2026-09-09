@echo off

echo Creating NwOS.img...

dd if=/dev/zero of=NwOS.img bs=512 count=4096

if errorlevel 1 (
    echo ERROR: failed to create image
    pause
    exit /b 1
)

dd if=boot.bin of=NwOS.img bs=512 count=1 conv=notrunc

if errorlevel 1 (
    echo ERROR: failed to write bootloader
    pause
    exit /b 1
)

dd if=kernel.bin of=NwOS.img bs=512 seek=1 conv=notrunc

if errorlevel 1 (
    echo ERROR: failed to write kernel
    pause
    exit /b 1
)

echo.
echo NwOS.img created successfully!
echo.

dir NwOS.img

pause