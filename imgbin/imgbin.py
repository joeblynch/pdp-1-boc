#!/usr/bin/env python3
"""
imgbin.py - convert between an 8-pixel-high 1-bit bitmap and a raw binary file.

Usage
-----
    python3 imgbin.py <source file> <destination file>

Rules
-----
* If <source file> opens as a bitmap image, it is decoded into a binary file.
  - The bitmap must be 8 px tall and 1-bit (mode “1”).  
  - Column *x* holds byte *x*; y = 0 is the LSB, y = 7 the MSB.  
  - Black pixels (value 0) → bit 1, white pixels (value 255) → bit 0.

* Otherwise the source is treated as a binary file and turned into a bitmap.  
  - Height = 8 px, width = number of bytes.  
  - Bit 1 → black pixel, bit 0 → white pixel.

Requires Pillow (`pip install pillow`).
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:   # pragma: no cover
    sys.exit("Pillow not found - install with:  pip install pillow")

HEIGHT = 8                # fixed bitmap height
BITMAP_MODE = "1"         # 1-bit pixels: 0=black, 255=white


def bin_to_img(src: Path, dst: Path) -> None:
    """Create a bitmap from a raw binary file."""
    data = src.read_bytes()
    width = len(data)

    img = Image.new(BITMAP_MODE, (width, HEIGHT), 255)  # start all-white
    px = img.load()

    for x, byte in enumerate(data):
        for y in range(HEIGHT):
            if (byte >> y) & 1:          # bit set?
                px[x, y] = 0             # draw black pixel

    img.save(dst, format="BMP")
    print(f"[ok] wrote {dst} ({width}-byte bitmap)")


def img_to_bin(src: Path, dst: Path) -> None:
    """Extract a raw binary file from an 8-pixel-high bitmap."""
    img = Image.open(src)
    if img.mode != BITMAP_MODE:
        img = img.convert(BITMAP_MODE)

    width, height = img.size
    if height != HEIGHT:
        sys.exit("error: bitmap height must be exactly 8 pixels")

    px = img.load()
    out = bytearray()

    for x in range(width):
        byte = 0
        for y in range(HEIGHT):
            bit = 1 if px[x, y] == 0 else 0   # black → 1
            byte |= bit << y
        out.append(byte)

    dst.write_bytes(out)
    print(f"[ok] wrote {dst} ({len(out)} bytes)")


def main() -> None:
    if len(sys.argv) != 3 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(1)

    src, dst = map(Path, sys.argv[1:3])

    # Treat source as image if Pillow can open it, otherwise as binary
    try:
        with Image.open(src):
            img_to_bin(src, dst)
    except (OSError, FileNotFoundError):
        bin_to_img(src, dst)


if __name__ == "__main__":
    main()
