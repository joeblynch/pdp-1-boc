#!/usr/bin/env python3

import sys
import argparse

def main():
    # Tape geometry (FIPS-26 / ANSI X3.18-1967 / Flexowriter)
    tape_height = 1.0  # inches
    hole_spacing_y = 0.1  # vertical spacing between adjacent hole centers
    hole_spacing_x = 0.1     # horizontal spacing between adjacent bytes
    feed_hole_position = 3   # the index of the feed hole (0-based from the top)
    top_hole_center = 0.392 - hole_spacing_y * feed_hole_position  # distance from top edge of tape to first (top) hole center
    code_hole_diam = 0.072   # diameter of data bit holes (when bit=1)
    feed_hole_diam = 0.046   # diameter of feed hole
    tape_fold_len = 85  # frames
    
    stroke_width = 0.01      # border around the tape


    parser = argparse.ArgumentParser(
        description="Render a binary file as a paper-tape hole pattern in SVG."
    )
    parser.add_argument("input_file", help="Path to the binary file to read")
    parser.add_argument(
        "--start-padding", type=float, default=0.25,
        help="Padding at the start of the tape (in inches)"
    )
    parser.add_argument(
        "--end-padding", type=float, default=0.25,
        help="Padding at the end of the tape (in inches)"
    )
    parser.add_argument(
        "--output-file", "-o", default="output.svg",
        help="SVG file to write output into"
    )
    parser.add_argument(
        "--start-byte", type=int, default=0,
        help="Zero-based index of the first byte to draw (inclusive)."
    )
    parser.add_argument(
        "--end-byte", type=int, default=None,
        help="Zero-based index of the last byte to draw (inclusive). If not set, all bytes from `start-byte` to the end of file are drawn."
    )

    args = parser.parse_args()

    # Read all data from the binary file
    with open(args.input_file, "rb") as f:
        data = f.read()

    total_bytes = len(data)
    if total_bytes == 0:
        print("No data in file; nothing to render.")
        sys.exit(0)

    # Determine the actual start and end byte indices (inclusive)
    start_index = max(args.start_byte, 0)
    if args.end_byte is None or args.end_byte >= total_bytes:
        end_index = total_bytes - 1
    else:
        end_index = args.end_byte

    if start_index > end_index:
        print(
            f"Warning: start-byte ({start_index}) > end-byte ({end_index}). "
            "No bytes will be drawn."
        )
        # We can either exit or proceed with an empty slice. Here, let's exit.
        sys.exit(0)

    # Slice the data to get the bytes we want to draw
    data_slice = data[start_index : end_index + 1]
    num_bytes = len(data_slice)

    # Calculate total width in inches
    tape_width = args.start_padding + num_bytes * hole_spacing_x + args.end_padding

    # Build SVG
    # We'll use an SVG coordinate system where 1 user unit = 1 inch.
    # So the <svg> will have width="..." in inches and height="1" in inches,
    # and a matching viewBox="0 0 tape_width tape_height".
    svg_parts = []

    # SVG header
    svg_parts.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" '
        f'width="{tape_width}in" height="{tape_height}in" '
        f'viewBox="0 0 {tape_width} {tape_height}">\n'
    )

    # Draw the base tape rectangle
    svg_parts.append(
        f'  <rect x="0" y="0" width="{tape_width}" height="{tape_height}" '
        f'fill="#abc" stroke="black" stroke-width="{stroke_width}" />\n'
    )

    # Function to compute the circle's center for a given byte index (in the slice) and hole index
    def circle_center(byte_index, hole_index):
        # X center: start padding + (byte_index + 0.5) * hole_spacing_x
        cx = args.start_padding + (byte_index + 0.5) * hole_spacing_x
        # Y center: top hole center + hole_index * hole_spacing_y
        cy = top_hole_center + hole_index * hole_spacing_y
        return cx, cy

    # Draw holes for each byte in data_slice
    for column_index, byte_val in enumerate(data_slice):
        # For hole_index in [0..8], the 4th one (hole_index == 3) is always feed hole.

        if column_index % tape_fold_len == 0:
            fold_width = 0.02
            fold_x = args.start_padding + column_index * hole_spacing_x - fold_width / 2
            svg_parts.append(
                f'  <rect x="{fold_x}" y="0" width="{fold_width}" height="{tape_height}" fill="#789" />\n'
            )

        for hole_index in range(9):
            cx, cy = circle_center(column_index, hole_index)

            if hole_index == feed_hole_position:
                # Always draw the feed hole
                r = feed_hole_diam / 2.0
                svg_parts.append(
                    f'  <circle cx="{cx:.3f}" cy="{cy:.3f}" r="{r:.3f}" fill="#012" />\n'
                )
            else:
                # Check if the corresponding bit is 1
                # The top circle is LSB => hole_index 0 => bit 0
                # so the bit number = hole_index if hole_index < feed_hole_position
                # if hole_index > feed_hole_position, shift by 1.
                if hole_index < feed_hole_position:
                    bit_num = hole_index
                else:
                    bit_num = hole_index - 1

                bit_is_set = (byte_val >> bit_num) & 0x1
                if bit_is_set == 1:
                    # Draw a code hole
                    r = code_hole_diam / 2.0
                    svg_parts.append(
                        f'  <circle cx="{cx:.3f}" cy="{cy:.3f}" r="{r:.3f}" fill="#012" />\n'
                    )

    # Close SVG
    svg_parts.append('</svg>\n')

    # Write out to file
    with open(args.output_file, 'w', encoding='utf-8') as out_svg:
        out_svg.writelines(svg_parts)

    print(f"SVG paper tape written to {args.output_file}")
    print(f"Bytes drawn: {start_index} through {end_index} (inclusive).")

if __name__ == "__main__":
    main()
