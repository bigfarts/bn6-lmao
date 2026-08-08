#!/usr/bin/env python3
"""Add a full-size, matching gold 7 beside the BN6 title-screen numeral."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class TitleConfig:
    stream_offset: int
    decoded_size: int
    encoded_capacity: int
    tilemap_offset: int
    colors: dict[str, int]
    gradient: tuple[int, ...]


CONFIGS = {
    "gregar": TitleConfig(
        stream_offset=0x7F3040,
        decoded_size=0x8E84,
        encoded_capacity=0x4CB9,
        tilemap_offset=0x7F7CFC,
        colors={
            "black": 0x13,
            "gray_dark": 0x18,
            "gray": 0x1B,
            "brown": 0x49,
            "orange_dark": 0x3E,
            "orange": 0x3D,
            "gold": 0x45,
            "yellow": 0x44,
            "light": 0x30,
            "cream": 0x2F,
            "white": 0x2E,
        },
        gradient=(
            0x3E, 0x3D, 0x3C, 0x41, 0x42, 0x45, 0x44,
            0x46, 0x33, 0x31, 0x30, 0x2F, 0x2E,
        ),
    ),
    "falzar": TitleConfig(
        stream_offset=0x7F4394,
        decoded_size=0x8D84,
        encoded_capacity=0x4B1C,
        tilemap_offset=0x7F8EB0,
        colors={
            "black": 0x00,
            "gray_dark": 0x0C,
            "gray": 0x0A,
            "brown": 0x14,
            "orange_dark": 0x37,
            "orange": 0x36,
            "gold": 0x2F,
            "yellow": 0x2E,
            "light": 0x4F,
            "cream": 0x4A,
            "white": 0x49,
        },
        gradient=(
            0x37, 0x36, 0x34, 0x33, 0x31, 0x30, 0x2F,
            0x2E, 0x2A, 0x3D, 0x4F, 0x4A, 0x49,
        ),
    ),
}


def decompress_lz77(data: bytes, offset: int = 0) -> tuple[bytes, int]:
    if data[offset] != 0x10:
        raise ValueError(f"expected GBA LZ77 stream at {offset:#x}")
    output_size = int.from_bytes(data[offset + 1 : offset + 4], "little")
    source = offset + 4
    output = bytearray()
    while len(output) < output_size:
        flags = data[source]
        source += 1
        for bit in range(7, -1, -1):
            if len(output) >= output_size:
                break
            if flags & (1 << bit):
                pair = int.from_bytes(data[source : source + 2], "big")
                source += 2
                count = (pair >> 12) + 3
                distance = (pair & 0xFFF) + 1
                if distance > len(output):
                    raise ValueError(f"invalid LZ77 back-reference at {source - 2:#x}")
                for _ in range(count):
                    output.append(output[-distance])
                    if len(output) >= output_size:
                        break
            else:
                output.append(data[source])
                source += 1
    return bytes(output), source - offset


def compress_lz77(data: bytes) -> bytes:
    """Encode BIOS 0x10 LZ77 with a 4 KiB/18-byte window and lazy matches."""
    if len(data) > 0xFFFFFF:
        raise ValueError("GBA LZ77 output length exceeds 24-bit header")
    output = bytearray((0x10, len(data) & 0xFF, (len(data) >> 8) & 0xFF, len(data) >> 16))
    positions: dict[bytes, list[int]] = {}
    source = 0

    def remember(position: int) -> None:
        if position + 2 >= len(data):
            return
        key = data[position : position + 3]
        entries = positions.setdefault(key, [])
        entries.append(position)
        cutoff = position - 0x1000
        while entries and entries[0] < cutoff:
            del entries[0]

    def find_match(position: int, extra: int | None = None) -> tuple[int, int]:
        if position + 2 >= len(data):
            return 0, 0
        candidates = list(positions.get(data[position : position + 3], ()))
        # When considering a literal, that byte becomes available as a
        # one-byte-distance match at the next position.
        if extra is not None and data[extra : extra + 3] == data[position : position + 3]:
            candidates.append(extra)
        best_length = 0
        best_distance = 0
        for candidate in reversed(candidates):
            distance = position - candidate
            if distance > 0x1000:
                break
            length = 3
            limit = min(18, len(data) - position)
            while length < limit and data[candidate + length] == data[position + length]:
                length += 1
            if length > best_length:
                best_length = length
                best_distance = distance
                if length == 18:
                    break
        return best_length, best_distance

    while source < len(data):
        flag_offset = len(output)
        output.append(0)
        flags = 0
        for bit in range(7, -1, -1):
            if source >= len(data):
                break
            best_length, best_distance = find_match(source)
            if 3 <= best_length < 18 and source + 1 < len(data):
                next_length, _ = find_match(source + 1, extra=source)
                if next_length > best_length:
                    best_length = 0
            if best_length >= 3:
                flags |= 1 << bit
                pair = ((best_length - 3) << 12) | (best_distance - 1)
                output.extend(pair.to_bytes(2, "big"))
                consumed = best_length
            else:
                output.append(data[source])
                consumed = 1
            for position in range(source, source + consumed):
                remember(position)
            source += consumed
        output[flag_offset] = flags
    return bytes(output)


# Pre-rasterized from SF Pro Display Heavy Italic, then extended slightly in
# height at composition time. G/B/F are the font's gray outer stroke, black
# inner stroke, and face. The
# system font itself is not redistributed, and the build has no font runtime
# dependency. This is the thresholded output of the font rasterizer—not a
# hand-shaped or dilated bitmap.
FONT_SEVEN = (
    '.........GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG..',
    '........GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG.',
    '.......GGBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBGG',
    '.......GGBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBGG',
    '.......GGBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBGG',
    '.......GBBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBBG.',
    '.......GBBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBBG.',
    '......GGBBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBGG.',
    '......GGBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBGG.',
    '......GGBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBBGG.',
    '......GBBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBBG..',
    '......GBBFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFBBG..',
    '.....GGBBBBBBBBBBBBBBBBBBBBBBBBFFFFFFFFFFFFFBGG..',
    '......GBBBBBBBBBBBBBBBBBBBBBBBBFFFFFFFFFFFFBBGG..',
    '......GGGGGGGGGGGGGGGGGGGGGGBBFFFFFFFFFFFFBBGG...',
    '.......................GGGGBBFFFFFFFFFFFFFBBG....',
    '.........................GBBFFFFFFFFFFFFFBBGG....',
    '........................GGBBFFFFFFFFFFFFBBGG.....',
    '.......................GGBBFFFFFFFFFFFFBBGG......',
    '......................GGBBFFFFFFFFFFFFFBBGG......',
    '......................GBBFFFFFFFFFFFFFBBGG.......',
    '.....................GGBBFFFFFFFFFFFFBBGG........',
    '....................GGBBFFFFFFFFFFFFFBBG.........',
    '...................GGBBFFFFFFFFFFFFFBBGG.........',
    '...................GGBFFFFFFFFFFFFFBBGG..........',
    '..................GGBBFFFFFFFFFFFFFBGG...........',
    '.................GGBBFFFFFFFFFFFFFBBGG...........',
    '................GGBBFFFFFFFFFFFFFBBGG............',
    '................GGBFFFFFFFFFFFFFBBGG.............',
    '...............GGBBFFFFFFFFFFFFFBBG..............',
    '..............GGBBFFFFFFFFFFFFFBBGG..............',
    '..............GBBFFFFFFFFFFFFFBBGG...............',
    '.............GGBBFFFFFFFFFFFFFBGG................',
    '............GGBBFFFFFFFFFFFFFBBGG................',
    '...........GGBBFFFFFFFFFFFFFBBGG.................',
    '...........GGBFFFFFFFFFFFFFBBGG..................',
    '..........GGBBFFFFFFFFFFFFFBBG...................',
    '.........GGBBFFFFFFFFFFFFFBBGG...................',
    '........GGBBFFFFFFFFFFFFFBBGG....................',
    '........GGBFFFFFFFFFFFFFFBGG.....................',
    '.......GGBBFFFFFFFFFFFFFBBGG.....................',
    '......GGBBFFFFFFFFFFFFFBBGG......................',
    '......GBBFFFFFFFFFFFFFBBGG.......................',
    '.....GGBBFFFFFFFFFFFFFBBG........................',
    '....GGBBFFFFFFFFFFFFFBBGG........................',
    '...GGBBFFFFFFFFFFFFFBBGG.........................',
    '...GBBFFFFFFFFFFFFFFBGG..........................',
    '..GGBBFFFFFFFFFFFFFBBGG..........................',
    '.GGBBFFFFFFFFFFFFFBBGG...........................',
    'GGBBFFFFFFFFFFFFFBBGG............................',
    'GGBBFFFFFFFFFFFFFBBG.............................',
    'GGBBBBBBBBBBBBBBBBGG.............................',
    '.GGGGGGGGGGGGGGGGGG..............................',
    '.GGGGGGGGGGGGGGGGG...............................',
)
FONT_SEVEN_HEIGHT = 58


MAP_WIDTH = 32
MAP_HEIGHT = 20
MAP_SIZE = MAP_WIDTH * MAP_HEIGHT * 2


def rebuild_title(
    decoded: bytes,
    source_map: bytes,
    config: TitleConfig,
) -> tuple[bytes, bytes]:
    """Rebuild the complete 256x160 title layer and its complete tile map."""
    if len(decoded) != config.decoded_size:
        raise ValueError(f"unexpected title asset size {len(decoded):#x}")
    if len(source_map) != MAP_SIZE:
        raise ValueError(f"unexpected title map size {len(source_map):#x}")

    # Flatten the entire native layer, resolving every tile-map flip. The
    # replacement map can then be fully uniform and independent of either
    # version's original atlas layout.
    canvas = bytearray(MAP_WIDTH * 8 * MAP_HEIGHT * 8)
    canvas_width = MAP_WIDTH * 8
    for cell in range(MAP_WIDTH * MAP_HEIGHT):
        entry = int.from_bytes(source_map[cell * 2 : cell * 2 + 2], "little")
        tile_id = entry & 0x3FF
        tile_start = 4 + tile_id * 64
        tile = decoded[tile_start : tile_start + 64]
        if len(tile) != 64:
            raise ValueError(f"title map references missing tile {tile_id:#x}")
        flip_x = bool(entry & 0x400)
        flip_y = bool(entry & 0x800)
        cell_x = (cell % MAP_WIDTH) * 8
        cell_y = (cell // MAP_WIDTH) * 8
        for pixel_y in range(8):
            source_y = 7 - pixel_y if flip_y else pixel_y
            for pixel_x in range(8):
                source_x = 7 - pixel_x if flip_x else pixel_x
                canvas[(cell_y + pixel_y) * canvas_width + cell_x + pixel_x] = (
                    tile[source_y * 8 + source_x]
                )

    # Preserve the font rasterizer's exact silhouette and strokes. Only the
    # face color changes by row, using a smooth native gold ramp without
    # remapped texture or checkerboard dithering. This is intentionally drawn
    # over the original 6; there is no foreground mask.
    origin_x, origin_y = 191, 25
    for y in range(FONT_SEVEN_HEIGHT):
        source_y = y * (len(FONT_SEVEN) - 1) // (FONT_SEVEN_HEIGHT - 1)
        row = FONT_SEVEN[source_y]
        gradient_index = y * (len(config.gradient) - 1) // (FONT_SEVEN_HEIGHT - 1)
        for x, layer in enumerate(row):
            if layer == "G":
                color = config.colors["gray"]
            elif layer == "B":
                color = config.colors["black"]
            elif layer == "F":
                color = config.gradient[gradient_index]
            else:
                continue
            canvas[(origin_y + y) * canvas_width + origin_x + x] = color

    # Retile the whole layer and deduplicate exact 8x8 blocks. Every map entry
    # is a plain sequential atlas reference—no per-version gaps or edge hacks.
    tile_ids: dict[bytes, int] = {}
    tiles: list[bytes] = []
    rebuilt_map = bytearray()
    for tile_y in range(MAP_HEIGHT):
        for tile_x in range(MAP_WIDTH):
            tile = b"".join(
                bytes(
                    canvas[
                        (tile_y * 8 + pixel_y) * canvas_width + tile_x * 8 :
                        (tile_y * 8 + pixel_y) * canvas_width + tile_x * 8 + 8
                    ]
                )
                for pixel_y in range(8)
            )
            if tile not in tile_ids:
                tile_ids[tile] = len(tiles)
                tiles.append(tile)
            rebuilt_map.extend(tile_ids[tile].to_bytes(2, "little"))
    if len(tiles) > 0x400:
        raise ValueError(f"rebuilt title uses too many tiles: {len(tiles)}")

    tile_data = b"".join(tiles)
    rebuilt_size = 4 + len(tile_data)
    rebuilt = bytes((0,)) + rebuilt_size.to_bytes(3, "little") + tile_data
    return rebuilt, bytes(rebuilt_map)


def build(rom: bytes, variant: str) -> tuple[bytes, int, bytes]:
    config = CONFIGS[variant]
    decoded, consumed = decompress_lz77(rom, config.stream_offset)
    if consumed != config.encoded_capacity:
        raise ValueError(
            f"unexpected {variant} compressed title size {consumed:#x}; "
            f"expected {config.encoded_capacity:#x}"
        )
    source_map = rom[config.tilemap_offset : config.tilemap_offset + MAP_SIZE]
    edited, rebuilt_map = rebuild_title(decoded, source_map, config)
    encoded = compress_lz77(edited)
    if len(encoded) > config.encoded_capacity:
        raise ValueError(
            f"edited {variant} title grew to {len(encoded):#x}, beyond "
            f"its {config.encoded_capacity:#x}-byte slot"
        )
    check, check_consumed = decompress_lz77(encoded)
    if check != edited or check_consumed != len(encoded):
        raise ValueError(f"{variant} title LZ77 round trip failed")
    return encoded.ljust(config.encoded_capacity, b"\0"), len(encoded), rebuilt_map


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("variant", choices=CONFIGS)
    parser.add_argument("rom", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("map_output", type=Path)
    args = parser.parse_args()
    encoded, used, rebuilt_map = build(args.rom.read_bytes(), args.variant)
    args.output.write_bytes(encoded)
    args.map_output.write_bytes(rebuilt_map)
    print(
        f"{args.variant}: title-screen 67 uses {used:#x}/"
        f"{CONFIGS[args.variant].encoded_capacity:#x} compressed bytes"
    )


if __name__ == "__main__":
    main()
