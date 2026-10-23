#!/usr/bin/env python3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "build" / "wallpaper_data.h"
WIDTH = 1280
HEIGHT = 720
CANDIDATES = [
    ROOT / "sys" / "wallpaper.png",
    ROOT / "sys" / "wallpapers.png",
    ROOT / "wallpaper.png",
    ROOT / "wallpapers.png",
]


def fallback(reason):
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(
        "#ifndef NWOS_WALLPAPER_DATA_H\n"
        "#define NWOS_WALLPAPER_DATA_H\n\n"
        "#define WALLPAPER_PRESENT 0\n"
        "#define WALLPAPER_WIDTH 0\n"
        "#define WALLPAPER_HEIGHT 0\n\n"
        "#endif\n"
    )
    print(f"wallpaper: {reason}; using plain desktop background")


def main():
    source = next((p for p in CANDIDATES if p.is_file()), None)
    if source is None:
        fallback("no sys/wallpaper.png")
        return 0

    try:
        from PIL import Image, ImageOps
    except ImportError:
        fallback("Pillow is not installed")
        return 0

    try:
        image = Image.open(source)
        image = image.convert("RGBA")
        background = Image.new("RGBA", image.size, (0, 128, 128, 255))
        image = Image.alpha_composite(background, image).convert("RGB")
        resampling = getattr(Image, "Resampling", Image).LANCZOS
        image = ImageOps.fit(image, (WIDTH, HEIGHT), method=resampling)
    except Exception as exc:
        fallback(f"cannot read {source.relative_to(ROOT)}: {exc}")
        return 0

    pixels = []
    for r, g, b in image.getdata():
        pixels.append(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w") as f:
        f.write("#ifndef NWOS_WALLPAPER_DATA_H\n")
        f.write("#define NWOS_WALLPAPER_DATA_H\n\n")
        f.write("#define WALLPAPER_PRESENT 1\n")
        f.write(f"#define WALLPAPER_WIDTH {WIDTH}\n")
        f.write(f"#define WALLPAPER_HEIGHT {HEIGHT}\n\n")
        f.write(
            f"static const unsigned short wallpaper_pixels"
            f"[{WIDTH * HEIGHT}] = {{\n"
        )
        for i in range(0, len(pixels), 16):
            row = ", ".join(f"0x{v:04X}" for v in pixels[i:i + 16])
            f.write("    " + row + ",\n")
        f.write("};\n\n#endif\n")

    print(f"wallpaper: {source.relative_to(ROOT)} -> {WIDTH}x{HEIGHT} RGB565")
    return 0


if __name__ == "__main__":
    sys.exit(main())
