#!/usr/bin/env python3
"""
replace.py

Overlay a binary *title* and/or *trailer* block onto a tape-image file.
Both blocks must replace regions that are currently filled with null
(0x00) bytes, acting as safety guards so you never overwrite real data by
accident.

* **--title**   replaces the *beginning* of the image.
* **--trailer** replaces the *end*     of the image.

Either flag may be omitted, but at least one of them is required.

Usage examples
--------------

    # Only replace the header
    python replace_title.py \
        --title   header.bin \
        --tape-in source.img \
        --tape-out output.img

    # Only replace the trailer
    python replace_title.py \
        --trailer footer.bin \
        --tape-in  source.img \
        --tape-out output.img

    # Do both in one pass
    python replace_title.py \
        --title   header.bin \
        --trailer footer.bin \
        --tape-in  source.img \
        --tape-out output.img
"""
from __future__ import annotations

import argparse
import os
import sys
from typing import Iterator

CHUNK_SIZE = 64 * 1024  # 64 KiB per copy chunk


def iter_chunks(stream, remaining: int, chunk_size: int = CHUNK_SIZE) -> Iterator[bytes]:
    """Yield up to *remaining* bytes from *stream* in fixed-size chunks."""
    while remaining:
        chunk = stream.read(min(chunk_size, remaining))
        if not chunk:
            break
        remaining -= len(chunk)
        yield chunk


def load_file(path: str | None) -> bytes | None:
    """Read an entire binary file, or *None* if *path* is None."""
    if path is None:
        return None
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError as exc:
        sys.exit(f"Unable to read {path!r}: {exc}")
    if not data:
        sys.exit(f"{path!r} is empty; nothing to write.")
    return data


def validate_region_is_null(fp, offset: int, length: int, label: str) -> None:
    """Ensure that *length* bytes starting at *offset* are all 0x00."""
    fp.seek(offset)
    region = fp.read(length)
    if len(region) < length:
        sys.exit(
            f"File is smaller than expected when validating {label} region ({length} bytes)."
        )
    if any(region):
        print(region)
        human = f"the first {length} bytes" if label == "header" else f"the last {length} bytes"
        sys.exit(
            f"{human} are not all null bytes; refusing to overwrite non-empty {label} region."
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Replace null-filled header/trailer regions of a tape image with binary blocks.",
    )
    parser.add_argument("--title", help="Binary file to write at the start of the image.")
    parser.add_argument("--trailer", help="Binary file to write at the end of the image.")
    parser.add_argument("--tape-in", required=True, help="Path to the source tape image.")
    parser.add_argument("--tape-out", required=True, help="Destination path for modified image.")
    args = parser.parse_args()

    if not (args.title or args.trailer):
        parser.error("Provide at least --title or --trailer.")

    if os.path.abspath(args.tape_in) == os.path.abspath(args.tape_out):
        parser.error("--tape-out must be different from --tape-in to avoid data loss.")

    # Load blocks (may be None)
    title_data = load_file(args.title)
    trailer_data = load_file(args.trailer)
    title_len = len(title_data) if title_data else 0
    trailer_len = len(trailer_data) if trailer_data else 0

    # Basic size sanity check
    try:
        file_size = os.path.getsize(args.tape_in)
    except OSError as exc:
        sys.exit(f"Unable to stat --tape-in file: {exc}")

    if title_len + trailer_len > file_size:
        sys.exit("Title + trailer length exceeds size of tape image; cannot proceed.")

    # Validate null regions in a single pass
    try:
        with open(args.tape_in, "rb") as fin:
            if title_data:
                validate_region_is_null(fin, 0, title_len, "header")
            if trailer_data:
                validate_region_is_null(fin, file_size - trailer_len, trailer_len, "trailer")
    except OSError as exc:
        sys.exit(f"Unable to open --tape-in file: {exc}")

    # Perform the copy-with-overlays
    try:
        fin = open(args.tape_in, "rb")
    except OSError as exc:
        sys.exit(f"Unable to open --tape-in file: {exc}")
    try:
        fout = open(args.tape_out, "wb")
    except OSError as exc:
        fin.close()
        sys.exit(f"Unable to create --tape-out file: {exc}")

    with fin, fout:
        # 1. Header / start of file
        if title_data:
            fout.write(title_data)
            fin.seek(title_len)
        else:
            # No header replacement; copy from byte 0
            pass

        # 2. Middle section (everything up to trailer start)
        trailer_start = file_size - trailer_len if trailer_data else file_size
        middle_bytes = trailer_start - fin.tell()
        for chunk in iter_chunks(fin, middle_bytes):
            fout.write(chunk)

        # 3. Skip original trailer and write new footer, or just copy remainder
        if trailer_data:
            fin.seek(trailer_len, os.SEEK_CUR)  # discard original null trailer
            fout.write(trailer_data)
        else:
            # No trailer replacement; copy whatever is left (if any)
            remaining = file_size - fin.tell()
            for chunk in iter_chunks(fin, remaining):
                fout.write(chunk)

    print(f"Successfully wrote {args.tape_out}")


if __name__ == "__main__":
    main()
