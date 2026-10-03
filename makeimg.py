import argparse
import os
import sys

FLOPPY_SIZE = 1474560
SECTOR_SIZE = 512
BOOT_SECTORS = 1

def build_image(boot_path, kernel_path, out_path, size=FLOPPY_SIZE):
    if not os.path.exists(boot_path):
        print(f"[ERROR] {boot_path} not found", file=sys.stderr)
        return 1

    with open(boot_path, "rb") as f:
        boot_data = f.read()

    if not os.path.exists(kernel_path):
        print(f"[ERROR] {kernel_path} not found", file=sys.stderr)
        return 1

    with open(kernel_path, "rb") as f:
        kernel_data = f.read()

    if len(boot_data) > SECTOR_SIZE:
        print(f"[WARNING] boot.bin is {len(boot_data)} bytes "
              f"(> {SECTOR_SIZE}). Truncating to first sector.",
              file=sys.stderr)
        boot_data = boot_data[:SECTOR_SIZE]

    if len(boot_data) < SECTOR_SIZE:
        boot_data = boot_data + b"\x00" * (SECTOR_SIZE - len(boot_data))

    max_kernel = size - SECTOR_SIZE
    if len(kernel_data) > max_kernel:
        print(f"[ERROR] kernel.bin is {len(kernel_data)} bytes, "
              f"max is {max_kernel} ({max_kernel // SECTOR_SIZE} sectors)",
              file=sys.stderr)
        return 1

    kernel_sectors = (len(kernel_data) + SECTOR_SIZE - 1) // SECTOR_SIZE
    print(f"     boot.bin:   {len(boot_data)} bytes (1 sector)")
    print(f"     kernel.bin: {len(kernel_data)} bytes ({kernel_sectors} sectors)")
    print(f"     total:      {SECTOR_SIZE + len(kernel_data)} bytes")

    image = bytearray(size)
    image[0:SECTOR_SIZE] = boot_data
    image[SECTOR_SIZE:SECTOR_SIZE + len(kernel_data)] = kernel_data

    with open(out_path, "wb") as f:
        f.write(image)

    print(f"[OK] {out_path} created: {size} bytes")
    return 0

def main():
    parser = argparse.ArgumentParser(description="Build NwOS floppy image")
    parser.add_argument("--boot",   default="build/boot.bin")
    parser.add_argument("--kernel", default="build/kernel.bin")
    parser.add_argument("--out",    default="build/os.img")
    parser.add_argument("--size",   type=int, default=FLOPPY_SIZE,
                        help="Image size in bytes (default: 1474560)")
    args = parser.parse_args()

    return build_image(args.boot, args.kernel, args.out, args.size)

if __name__ == "__main__":
    sys.exit(main())