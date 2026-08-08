#!/usr/bin/env python3
"""Rebuild BN6's alphabetical chip-sort keys from the final chip names."""

from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path

from build_text_archives import read_archive


ROM_BASE = 0x08000000
CHIP_DATA_OFFSET = 0x21DA8
CHIP_RECORD_COUNT = 0x13A
CHIP_RECORD_SIZE = 0x2C
CHIP_SORT_OFFSET = 0x18
CHIP_NAME_ARCHIVE_SYMBOLS = ("chipnamearchive0", "chipnamearchive1")

# BN6's English text encoding stores digits, uppercase letters, and lowercase
# letters in these compact ranges. Expand the five multi-letter glyphs so EX,
# SP, and the other suffixes participate in the same case-insensitive sort.
MULTI_LETTER_GLYPHS = ("rv", "bx", "ex", "sp", "fz")


def read_symbols(path: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if len(fields) < 2:
            continue
        try:
            address = int(fields[0], 16)
        except ValueError:
            continue
        symbols[fields[1].lower()] = address
    return symbols


def normalized_chip_name(entry: bytes) -> str:
    """Return a punctuation-free, lowercase rendering for name collation."""
    normalized: list[str] = []
    for value in entry:
        if 0x01 <= value <= 0x0A:
            normalized.append(chr(ord("0") + value - 1))
        elif 0x0B <= value <= 0x24:
            normalized.append(chr(ord("a") + value - 0x0B))
        elif 0x26 <= value <= 0x3F:
            normalized.append(chr(ord("a") + value - 0x26))
        elif 0x40 <= value <= 0x44:
            normalized.append(MULTI_LETTER_GLYPHS[value - 0x40])
    return "".join(normalized)


def chip_name_sort_key(entry: bytes) -> tuple[tuple[int, int | str], ...]:
    """Build a natural-sort key, so numbered chip series sort numerically."""
    name = normalized_chip_name(entry)
    return tuple(
        (1, int(part)) if part.isdigit() else (0, part)
        for part in re.findall(r"\d+|\D+", name)
    )


def chip_name_archives(rom: bytes, symbols: dict[str, int]) -> list[list[bytes]]:
    archives: list[list[bytes]] = []
    for name in CHIP_NAME_ARCHIVE_SYMBOLS:
        if name not in symbols:
            raise ValueError(f"missing symbol: {name}")
        offset = symbols[name] - ROM_BASE
        if not 0 <= offset < len(rom):
            raise ValueError(f"{name} points outside the ROM: 0x{symbols[name]:08X}")
        archives.append(read_archive(rom, offset))
    return archives


def alphabetical_sort_values(rom: bytes, names: list[bytes]) -> dict[int, int]:
    """Return compact one-based ranks for records enabled for name sorting."""
    table_end = CHIP_DATA_OFFSET + CHIP_RECORD_COUNT * CHIP_RECORD_SIZE
    if table_end > len(rom):
        raise ValueError("chip data table extends past the end of the ROM")
    if len(names) < CHIP_RECORD_COUNT:
        raise ValueError(
            f"chip-name archives contain {len(names)} entries; "
            f"need at least {CHIP_RECORD_COUNT}"
        )

    sortable_ids = [
        chip_id
        for chip_id in range(CHIP_RECORD_COUNT)
        if struct.unpack_from(
            "<H",
            rom,
            CHIP_DATA_OFFSET + chip_id * CHIP_RECORD_SIZE + CHIP_SORT_OFFSET,
        )[0]
    ]
    for chip_id in sortable_ids:
        if not normalized_chip_name(names[chip_id]):
            raise ValueError(f"sortable chip 0x{chip_id:03X} has an empty name")

    ordered_ids = sorted(
        sortable_ids,
        key=lambda chip_id: (chip_name_sort_key(names[chip_id]), chip_id),
    )
    if len(ordered_ids) > 0xFFFF:
        raise ValueError("too many sortable chip records for 16-bit sort keys")
    return {chip_id: rank for rank, chip_id in enumerate(ordered_ids, 1)}


def reorder_chip_sort(rom: bytearray, symbols: dict[str, int]) -> dict[int, int]:
    archives = chip_name_archives(rom, symbols)
    names = [entry for archive in archives for entry in archive]
    sort_values = alphabetical_sort_values(rom, names)
    for chip_id, value in sort_values.items():
        struct.pack_into(
            "<H",
            rom,
            CHIP_DATA_OFFSET + chip_id * CHIP_RECORD_SIZE + CHIP_SORT_OFFSET,
            value,
        )
    return sort_values


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path, help="final Armips-built ROM to update in place")
    parser.add_argument("symbols", type=Path, help="Armips symbol file for the ROM")
    args = parser.parse_args()

    rom = bytearray(args.rom.read_bytes())
    sort_values = reorder_chip_sort(rom, read_symbols(args.symbols))
    args.rom.write_bytes(rom)
    print(f"{args.rom}: reordered {len(sort_values)} chip sort keys")


if __name__ == "__main__":
    main()
