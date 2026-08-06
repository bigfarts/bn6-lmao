#!/usr/bin/env python3
"""Extract BN3 FolderBack menu art and crop it to BN6's chip-art size."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


BN3_ROM_BASE = 0x08000000
BN3_CHIP_DATA = 0x11510
BN3_CHIP_RECORD_SIZE = 0x20
BN3_FOLDERBACK_ID = 0x12F
BN3_IMAGE_WIDTH = 64
BN3_IMAGE_HEIGHT = 56
BN6_IMAGE_WIDTH = 56
BN6_IMAGE_HEIGHT = 48


def read_pointer(rom: bytes, offset: int) -> int:
    address = struct.unpack_from("<I", rom, offset)[0]
    rom_offset = address - BN3_ROM_BASE
    if not 0 <= rom_offset < len(rom):
        raise ValueError(f"pointer 0x{address:08X} at 0x{offset:X} is outside the BN3 ROM")
    return rom_offset


def decode_4bpp_tiles(data: bytes, width: int, height: int) -> list[list[int]]:
    if width % 8 or height % 8:
        raise ValueError("tile dimensions must be multiples of eight")
    expected = width * height // 2
    if len(data) != expected:
        raise ValueError(f"expected 0x{expected:X} image bytes, got 0x{len(data):X}")
    pixels = [[0] * width for _ in range(height)]
    tiles_wide = width // 8
    for tile_index in range((width // 8) * (height // 8)):
        tile_x = (tile_index % tiles_wide) * 8
        tile_y = (tile_index // tiles_wide) * 8
        tile = data[tile_index * 32:(tile_index + 1) * 32]
        for y in range(8):
            for x in range(8):
                packed = tile[y * 4 + x // 2]
                pixels[tile_y + y][tile_x + x] = (packed >> (4 * (x & 1))) & 0x0F
    return pixels


def encode_4bpp_tiles(pixels: list[list[int]]) -> bytes:
    height = len(pixels)
    width = len(pixels[0]) if pixels else 0
    if width % 8 or height % 8 or any(len(row) != width for row in pixels):
        raise ValueError("pixels must form a rectangular, tile-aligned image")
    encoded = bytearray()
    for tile_y in range(0, height, 8):
        for tile_x in range(0, width, 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    low = pixels[tile_y + y][tile_x + x]
                    high = pixels[tile_y + y][tile_x + x + 1]
                    if not 0 <= low < 16 or not 0 <= high < 16:
                        raise ValueError("4bpp palette index is outside 0-15")
                    encoded.append(low | (high << 4))
    return bytes(encoded)


def extract(rom: bytes) -> tuple[bytes, bytes, bytes]:
    record = BN3_CHIP_DATA + BN3_FOLDERBACK_ID * BN3_CHIP_RECORD_SIZE
    if record + BN3_CHIP_RECORD_SIZE > len(rom):
        raise ValueError("BN3 FolderBack chip record is outside the ROM")
    icon_offset = read_pointer(rom, record + 0x14)
    image_offset = read_pointer(rom, record + 0x18)
    palette_offset = read_pointer(rom, record + 0x1C)

    icon = rom[icon_offset:icon_offset + 0x80]
    source_image = rom[image_offset:image_offset + 0x700]
    palette = rom[palette_offset:palette_offset + 0x20]
    if len(icon) != 0x80 or len(source_image) != 0x700 or len(palette) != 0x20:
        raise ValueError("BN3 FolderBack art is truncated")

    pixels = decode_4bpp_tiles(source_image, BN3_IMAGE_WIDTH, BN3_IMAGE_HEIGHT)
    crop_x = (BN3_IMAGE_WIDTH - BN6_IMAGE_WIDTH) // 2
    crop_y = (BN3_IMAGE_HEIGHT - BN6_IMAGE_HEIGHT) // 2
    cropped = [
        row[crop_x:crop_x + BN6_IMAGE_WIDTH]
        for row in pixels[crop_y:crop_y + BN6_IMAGE_HEIGHT]
    ]
    return icon, encode_4bpp_tiles(cropped), palette


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("bn3_blue_rom", type=Path)
    parser.add_argument("icon_output", type=Path)
    parser.add_argument("image_output", type=Path)
    parser.add_argument("palette_output", type=Path)
    args = parser.parse_args()

    outputs = zip(
        (args.icon_output, args.image_output, args.palette_output),
        extract(args.bn3_blue_rom.read_bytes()),
    )
    for path, data in outputs:
        path.write_bytes(data)


if __name__ == "__main__":
    main()
