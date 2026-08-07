#!/usr/bin/env python3
"""Build both complete BN6 chip-name and chip-description archives."""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


# The leading BN6 English charset entries cover the chip text used here.
# Bracketed tokens are single glyphs in the ROM (for example EX and SP).
BN6_EN_CHARSET = (
    [" "]
    + list("0123456789")
    + list("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    + ["*"]
    + list("abcdefghijklmnopqrstuvwxyz")
    + ["[RV]", "[BX]", "[EX]", "[SP]", "[FZ]"]
)
CHAR_TO_BYTE = {character: index for index, character in enumerate(BN6_EN_CHARSET)}
# BN6's English ampersand glyph is at charset index 0xA3. The compact
# leading-character table above intentionally omits the intervening Japanese
# glyphs, so map the punctuation used by imported descriptions explicitly.
CHAR_TO_BYTE["!"] = 0xA2
CHAR_TO_BYTE["&"] = 0xA3

NAME_END = 0xE6
LINE_BREAK = 0xE9
DESCRIPTION_HEADER = bytes((0xE8, 0x06, 0x01, 0x01, 0xF1, 0x00, 0x00))
DESCRIPTION_FOOTER = bytes((0xE7, 0x01, NAME_END))


def encode_text(text: str) -> bytes:
    encoded = bytearray()
    index = 0
    tokens = sorted(CHAR_TO_BYTE, key=len, reverse=True)
    while index < len(text):
        token = next((candidate for candidate in tokens if text.startswith(candidate, index)), None)
        if token is None:
            raise ValueError(f"character at {text[index:]!r} is not in the BN6 English charset")
        encoded.append(CHAR_TO_BYTE[token])
        index += len(token)
    return bytes(encoded)


def encode_name(text: str) -> bytes:
    return encode_text(text) + bytes((NAME_END,))


def encode_description(*lines: str) -> bytes:
    body = bytes((LINE_BREAK,)).join(encode_text(line) for line in lines)
    return DESCRIPTION_HEADER + body + DESCRIPTION_FOOTER


CHIP_NAMES = {
    0x018: encode_name("RollAro1"),
    0x019: encode_name("RollAro2"),
    0x01A: encode_name("RollAro3"),
    0x0BE: encode_name("BugChain"),
    # Verbatim BN4 English name. The source game and BN6 both allow eight
    # visible characters, so the final "e" is intentionally absent.
    0x0B0: encode_name("BugCharg"),
    0x0BF: encode_name("Jealousy"),
    0x0E3: encode_name("LaserMan"),
    0x0E4: encode_name("LaserMn[EX]"),
    0x0E5: encode_name("LaserMn[SP]"),
    0x107: encode_name("SerchMan"),
    0x108: encode_name("SerchMn[EX]"),
    0x109: encode_name("SerchMn[SP]"),
    0x12E: encode_name("ChaosLrd"),
    0x131: encode_name("SignlRed"),
    0x134: encode_name("DethPhnx"),
    0x139: encode_name("FoldrBak"),
}
SEARCHMAN_DESCRIPTION = encode_description("Aim", "and fire", "5 shots")
CHIP_DESCRIPTIONS = {
    0x018: encode_description("RollArrow", "destroys", "chips"),
    0x019: encode_description("RollArrow", "destroys", "chips"),
    0x01A: encode_description("RollArrow", "destroys", "chips"),
    # Verbatim Blue Moon BugChain description (chip ID 0x0D3).
    0x0BE: encode_description("Fires", "bugs into", "enmy area"),
    # Verbatim BN4 BugCharge description (chip ID 0x136).
    0x0B0: encode_description("EvilChip!", "Gets powr", "with turn"),
    0x0BF: encode_description("More dmg", "if enemy", "has chip"),
    0x0E3: encode_description("A laser", "pierces", "1 thru"),
    0x0E4: encode_description("A laser", "pierces", "1 thru"),
    0x0E5: encode_description("A laser", "pierces", "1 thru"),
    0x107: SEARCHMAN_DESCRIPTION,
    0x108: SEARCHMAN_DESCRIPTION,
    0x109: SEARCHMAN_DESCRIPTION,
    0x12E: encode_description("Hatred", "formed", "into Bass"),
    0x131: encode_description("Enmy chip", "is no use", "while red"),
    0x134: encode_description("Fire Atk", "Recycle", "Navi too"),
    # Verbatim BN3 Blue FolderBack description (chip ID 0x12F).
    0x139: encode_description("Restores", "all chips", "& folders"),
}


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def read_archive(rom: bytes, offset: int) -> list[bytes]:
    table_size = u16(rom, offset)
    if table_size == 0 or table_size & 1:
        raise ValueError(f"invalid archive table size at 0x{offset:X}")
    entry_count = table_size // 2
    offsets = struct.unpack_from(f"<{entry_count}H", rom, offset)
    entries: list[bytes] = []
    for index, relative in enumerate(offsets):
        start = offset + relative
        if index + 1 < entry_count:
            finish = offset + offsets[index + 1]
        else:
            terminator = rom.find(b"\xE6", start)
            if terminator < 0:
                raise ValueError(f"unterminated final archive entry at 0x{start:X}")
            finish = terminator + 1
        if finish < start:
            raise ValueError(f"descending archive offsets at entry {index}")
        entries.append(rom[start:finish])
    return entries


def build_archive(entries: list[bytes]) -> bytes:
    cursor = len(entries) * 2
    offsets: list[int] = []
    for entry in entries:
        offsets.append(cursor)
        cursor += len(entry)
        if cursor > 0xFFFF:
            raise ValueError("archive exceeds 16-bit offsets")
    return struct.pack(f"<{len(offsets)}H", *offsets) + b"".join(entries)


def apply_changes(archives: list[list[bytes]], changes: dict[int, bytes]) -> None:
    for chip_id, entry in changes.items():
        if chip_id < 0:
            raise ValueError(f"chip ID cannot be negative: {chip_id}")
        # Archive 0 contains IDs 0x000-0x0FF. Archive 1 starts at ID
        # 0x100 and contains the remaining chips. Keeping replacement
        # keys as full IDs makes either half directly editable.
        archive_index = chip_id >> 8
        entry_index = chip_id & 0xFF
        if archive_index >= len(archives) or entry_index >= len(archives[archive_index]):
            raise ValueError(f"chip ID 0x{chip_id:X} is outside the text archives")
        archives[archive_index][entry_index] = entry


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("name_pointer_table", type=lambda value: int(value, 0))
    parser.add_argument("description_pointer_table", type=lambda value: int(value, 0))
    parser.add_argument("name_output_0", type=Path)
    parser.add_argument("name_output_1", type=Path)
    parser.add_argument("description_output_0", type=Path)
    parser.add_argument("description_output_1", type=Path)
    args = parser.parse_args()

    rom = args.rom.read_bytes()
    name_offsets = [
        u32(rom, args.name_pointer_table + index * 4) - 0x08000000
        for index in range(2)
    ]
    description_offsets = [
        u32(rom, args.description_pointer_table + index * 4) - 0x08000000
        for index in range(2)
    ]
    for archive_offset in name_offsets + description_offsets:
        if not 0 <= archive_offset < len(rom):
            raise ValueError(f"text archive pointer is outside the ROM: 0x{archive_offset:X}")

    name_archives = [read_archive(rom, offset) for offset in name_offsets]
    description_archives = [read_archive(rom, offset) for offset in description_offsets]

    apply_changes(name_archives, CHIP_NAMES)
    apply_changes(description_archives, CHIP_DESCRIPTIONS)

    outputs = [
        (args.name_output_0, name_archives[0]),
        (args.name_output_1, name_archives[1]),
        (args.description_output_0, description_archives[0]),
        (args.description_output_1, description_archives[1]),
    ]
    for output, entries in outputs:
        output.write_bytes(build_archive(entries))


if __name__ == "__main__":
    main()
