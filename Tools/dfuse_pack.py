#!/usr/bin/env python3
"""Create an ST DfuSe image from an Intel HEX file."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

from intelhex import IntelHex


def crc32(data: bytes) -> int:
    return (-zlib.crc32(data) - 1) & 0xFFFFFFFF


def build(hex_path: Path, output: Path, name: str, vid: int, pid: int, version: int) -> None:
    image = IntelHex(str(hex_path))
    elements = []
    for start, end in image.segments():
        payload = image.tobinstr(start=start, end=end - 1)
        elements.append(struct.pack("<2I", start, len(payload)) + payload)

    target_data = b"".join(elements)
    target_name = name.encode("ascii")[:254] + b"\0"
    target_name = target_name.ljust(255, b"\0")
    target = struct.pack(
        "<6sBI255s2I", b"Target", 0, 1, target_name, len(target_data), len(elements)
    ) + target_data

    prefix_size = 11
    suffix_size = 16
    body = struct.pack("<5sBIB", b"DfuSe", 1, prefix_size + len(target) + suffix_size, 1) + target
    suffix = struct.pack("<4H3sB", version, pid, vid, 0x011A, b"UFD", suffix_size)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(body + suffix + struct.pack("<I", crc32(body + suffix)))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("hex_file", type=Path)
    parser.add_argument("dfu_file", type=Path)
    parser.add_argument("--name", required=True)
    parser.add_argument("--vid", type=lambda value: int(value, 0), default=0x0483)
    parser.add_argument("--pid", type=lambda value: int(value, 0), default=0xDF11)
    parser.add_argument("--version", type=lambda value: int(value, 0), default=0x2200)
    args = parser.parse_args()
    build(args.hex_file, args.dfu_file, args.name, args.vid, args.pid, args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
