#!/usr/bin/env python3
"""
Render a binary file as a 1-inch paper-tape hole pattern (Flexowriter / FIPS-26)
in SVG.  This version draws the tape **vertically**, top-to-bottom, after a
true 90 ° clockwise rotation of the original horizontal image.
"""

import sys
import argparse


def main():
    # ─────────────────────────  Tape geometry (constants) ─────────────────────────
    tape_height = 1.0          # physical tape width (1 inch)
    hole_spacing_y = 0.1       # spacing between adjacent hole rows
    hole_spacing_x = 0.1       # spacing between adjacent bytes
    feed_hole_position = 3     # index of the feed hole row (0-based from top)
    top_hole_center = 0.392 - hole_spacing_y * feed_hole_position
    code_hole_diam = 0.072     # diameter of data-bit holes
    feed_hole_diam = 0.046     # diameter of feed hole
    # stroke_width = 0.01        # tape border (inches)

    # ────────────────────────────  CLI arguments  ────────────────────────────────
    parser = argparse.ArgumentParser(
        description="Render a binary file as a paper-tape hole pattern in SVG."
    )
    parser.add_argument("input_file", help="Path to the binary file to read")
    parser.add_argument(
        "--start-padding", type=float, default=0,
        help="Padding at the start of the tape (in inches)"
    )
    parser.add_argument(
        "--end-padding", type=float, default=0,
        help="Padding at the end of the tape (in inches)"
    )
    parser.add_argument(
        "--output-file", "-o", default="output.svg",
        help="SVG file to write output into"
    )
    parser.add_argument(
        "--start-byte", type=int, default=0,
        help="Zero-based index of the first byte to draw (inclusive)"
    )
    parser.add_argument(
        "--end-byte", type=int, default=None,
        help="Zero-based index of the last byte to draw (inclusive). "
             "If not set, all bytes from `start-byte` to EOF are drawn."
    )
    args = parser.parse_args()

    # ──────────────────────────────  Read data  ──────────────────────────────────
    with open(args.input_file, "rb") as f:
        data = f.read()

    total_bytes = len(data)
    if total_bytes == 0:
        print("No data in file; nothing to render.")
        sys.exit(0)

    start_index = max(args.start_byte, 0)
    end_index = (
        total_bytes - 1
        if args.end_byte is None or args.end_byte >= total_bytes
        else args.end_byte
    )
    if start_index > end_index:
        print(
            f"Warning: start-byte ({start_index}) > end-byte ({end_index}). "
            "No bytes will be drawn."
        )
        sys.exit(0)

    data_slice = data[start_index:end_index + 1]
    num_bytes = len(data_slice)

    # ─────────────────────  *** ROTATION CHANGES BEGIN ***  ──────────────────────
    # The tape’s *length* (formerly width) now grows with the number of bytes;
    # its cross-sectional width is the fixed 1-inch band.
    tape_length = args.start_padding + num_bytes * hole_spacing_x + args.end_padding
    tape_width_fixed = tape_height          # exactly 1 inch

    svg_parts = []

    # SVG header
    svg_parts.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'width="{tape_width_fixed}in" height="{tape_length}in" '
        f'viewBox="0 0 {tape_width_fixed} {tape_length}">\n'
    )

    # Tape background
    svg_parts.append(
        f'  <rect x="0" y="0" width="{tape_width_fixed}" height="{tape_length}" fill="#abc" />\n'
    )

    # Hole-centre helper:
    #   • swap axes          (old Y → new X, old X → new Y)
    #   • mirror horizontally (so bottom row → left column, top row → right)
    max_hole_index = 8        # highest row index (MSB)
    def circle_center(byte_index: int, hole_index: int):
        # Column X across the 1-inch width (left ← MSB … LSB → right)
        col = max_hole_index - hole_index
        cx = top_hole_center + col * hole_spacing_y
        # Row Y down the tape’s length (top → bottom, byte order)
        cy = args.start_padding + (byte_index + 0.5) * hole_spacing_x
        return cx, cy
    # ─────────────────────  ***  ROTATION CHANGES END  ***  ──────────────────────

    # ─────────────────────────────  Draw holes  ──────────────────────────────────
    for column_index, byte_val in enumerate(data_slice):
        for hole_index in range(9):          # 0…8, where 3 is the feed hole
            cx, cy = circle_center(column_index, hole_index)

            if hole_index == feed_hole_position:        # always drill feed hole
                r = feed_hole_diam / 2.0
                svg_parts.append(
                    f'  <circle cx="{cx:.3f}" cy="{cy:.3f}" '
                    f'r="{r:.3f}" fill="#071727" />\n'
                )
            else:
                # Determine which data bit corresponds to this hole
                bit_num = (
                    hole_index if hole_index < feed_hole_position
                    else hole_index - 1
                )
                if (byte_val >> bit_num) & 1:
                    r = code_hole_diam / 2.0
                    svg_parts.append(
                        f'  <circle cx="{cx:.3f}" cy="{cy:.3f}" '
                        f'r="{r:.3f}" fill="#071727" />\n'
                    )

    # ─────────────────────────────  Finish up  ───────────────────────────────────
    svg_parts.append("</svg>\n")

    with open(args.output_file, "w", encoding="utf-8") as out_svg:
        out_svg.writelines(svg_parts)

    print(f"SVG paper tape written to {args.output_file}")
    print(f"Bytes drawn: {start_index} through {end_index} (inclusive).")


if __name__ == "__main__":
    main()
