#!/usr/bin/env python3
"""Verify both autoregion-based BN6 chip-port builds, including RollArrow."""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

from build_text_archives import CHIP_DESCRIPTIONS, CHIP_NAMES, apply_changes
from extract_folderback_art import extract as extract_folderback_art
from reorder_chip_sort import (
    CHIP_DATA_OFFSET,
    CHIP_RECORD_COUNT,
    CHIP_RECORD_SIZE,
    CHIP_SORT_OFFSET,
    alphabetical_sort_values,
)


ROLLARROW_RECORDS = CHIP_DATA_OFFSET + 0x018 * 0x2C
LASERMAN_RECORDS = CHIP_DATA_OFFSET + 0x0E3 * 0x2C
SEARCHMAN_RECORDS = CHIP_DATA_OFFSET + 0x107 * 0x2C
BUGCHAIN_RECORD = CHIP_DATA_OFFSET + 0x0BE * 0x2C
BUGFIX_RECORD = CHIP_DATA_OFFSET + 0x0B0 * 0x2C
SIGNALRED_RECORD = CHIP_DATA_OFFSET + 0x0C1 * 0x2C
JEALOUSY_RECORD = CHIP_DATA_OFFSET + 0x0BF * 0x2C
BASS_RECORD = CHIP_DATA_OFFSET + 0x12D * 0x2C
CHAOSLORD_RECORD = CHIP_DATA_OFFSET + 0x12E * 0x2C
BUGCHARGE_RECORD = CHIP_DATA_OFFSET + 0x131 * 0x2C
DEATHPHOENIX_RECORD = CHIP_DATA_OFFSET + 0x134 * 0x2C
FOLDERBACK_RECORD = CHIP_DATA_OFFSET + 0x139 * 0x2C


def verify(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"verification failed: {message}")


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def rom_offset(address: int) -> int:
    return address - 0x08000000


def contains_thumb_bl(data: bytes, start: int, end: int, target: int) -> bool:
    """Return whether an ARMv4T Thumb BL in the ROM interval reaches target."""
    for offset in range(start, end - 3, 2):
        high, low = struct.unpack_from("<HH", data, offset)
        if high & 0xF800 != 0xF000 or low & 0xF800 != 0xF800:
            continue
        displacement = ((high & 0x07FF) << 12) | ((low & 0x07FF) << 1)
        if displacement & 0x400000:
            displacement -= 0x800000
        if 0x08000000 + offset + 4 + displacement == target:
            return True
    return False


def archive_entries(data: bytes, offset: int) -> list[bytes]:
    entry_count = u16(data, offset) // 2
    offsets = struct.unpack_from(f"<{entry_count}H", data, offset)
    entries: list[bytes] = []
    for index, relative in enumerate(offsets):
        start = offset + relative
        if index + 1 < entry_count:
            finish = offset + offsets[index + 1]
        else:
            terminator = data.find(b"\xE6", start)
            verify(terminator >= 0, "final archive entry is terminated")
            finish = terminator + 1
        entries.append(data[start:finish])
    return entries


def archive_extent(data: bytes, offset: int) -> int:
    entries = archive_entries(data, offset)
    table_size = len(entries) * 2
    return table_size + sum(len(entry) for entry in entries)


def read_symbols(path: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split()
        if len(fields) >= 2:
            try:
                address = int(fields[0], 16)
            except ValueError:
                continue
            symbols[fields[1].lower()] = address
    return symbols


def interval(start: int, length: int) -> range:
    return range(start, start + length)


def verify_version(
    label: str,
    original_path: Path,
    output_path: Path,
    symbol_path: Path,
    bn5: bytes,
    bn5_colonel: bytes,
    bn4: bytes,
    bn3: bytes,
    name_pointer_table: int,
    description_pointer_table: int,
    name_references: tuple[tuple[int, ...], tuple[int, ...]],
    description_references: tuple[tuple[int, ...], tuple[int, ...]],
    replace_chaos_art: bool,
    replace_bugcharge_art: bool,
    jealousy_damage_object: int,
    replace_deathphoenix_art: bool,
    death_damage_object: int,
    death_navi_transition: int,
    dust_suction_table_reference: int,
    song_table_offset: int,
    song_table_references: tuple[int, ...],
) -> None:
    original = original_path.read_bytes()
    output = output_path.read_bytes()
    symbols = read_symbols(symbol_path)

    verify(len(original) == 0x800000, f"{label} original size")
    verify(len(output) == 0x1000000, f"{label} output size")

    def symbol(name: str) -> int:
        key = name.lower()
        verify(key in symbols, f"{label} symbol {name}")
        return symbols[key]

    # Runtime dispatches use labels selected by Armips, never fixed expanded
    # ROM offsets.
    dispatches = [
        (0x2CD64, "LaserManTimeFreezeSpawn"),
        (0x3D5C, "LaserManSlotMain"),
        (0x2CD94, "SearchManTimeFreezeSpawn"),
        (0x3D60, "SearchManObjectMain"),
        (0x3F74, "SearchManHitMain"),
        (0x4324, "SearchManReticleMain"),
        (0x2CDB8, "ChaosTimeFreezeSpawn"),
        (0x3DE0, "ChaosControllerMain"),
        (0x3DE4, "ChaosBallMain"),
        (0x3F78, "ChaosAttackMain"),
        (0x44F8, "ChaosAuraMain"),
        (0x44FC, "ChaosBurstOrTeardownMain"),
        (0x2CCD0, "JealousyTimeFreezeSpawn"),
        (0x2CD3C, "BugChainTimeFreezeSpawn"),
        (0x4438, "BugChainSharedMain"),
        (0x2CD4C, "SignalRedBugChargeTimeFreezeDispatch"),
        (0x2CD40, "FolderBackSpawn"),
        (0x44D8, "FolderBackSharedMain"),
    ]
    for pointer_offset, target in dispatches:
        verify(u32(output, pointer_offset) == symbol(target) + 1, f"{label} {target} dispatch")
    for pointer_offset, target in (
        (0x3DE8, "RollArrowActorMain"),
        (0x2CDA0, "DeathPhoenixTimeFreezeSpawn"),
        (0x3CA0, "DeathPhoenixMain"),
        (0x44CC, "DeathStrikeMain"),
        (0x44D0, "DeathFlameMain"),
    ):
        verify(u32(output, pointer_offset) == symbol(target) + 1, f"{label} {target} dispatch")
    verify(
        u32(output, 0x3224) == symbol("FolderBackType1DispatchTable"),
        f"{label} FolderBack type-1 proxy table dispatch",
    )
    sprite_tables = (
        (
            0x31CCC, "ImportedSpriteGroup08Table", 0x31DA4, 0x5C,
            (
                (0x00, "ChaosBassSprite", False),
                (0x11, "SearchManBattleSprite", False),
                (0x12, "ChaosAuraSprite", True),
                (0x13, "ChaosBassSprite", False),
            ),
            (),
        ),
        (
            0x31CD0, "ImportedSpriteGroup0CTable", 0x31E00, 0x1A4,
            (
                (0x20, "BugChargeChargeSprite", False),
                (0x56, "LaserManBattleSprite", True),
                (0x57, "DeathPhoenixBattleSprite", False),
                (0x5A, "RollArrowActorSprite", False),
                (0x5B, "RollArrowProjectileSprite", False),
            ),
            (),
        ),
        (
            0x31CD4, "ImportedSpriteGroup10Table", 0x31FA4, 0x170,
            (
                (0x04, "ChaosTeardownSprite", False),
                (0x14, "SearchManReticleAltSprite", False),
                (0x15, "SearchManReticleSprite", False),
                (0x18, "ChaosApparitionSprite", False),
                (0x1E, "SignalRedBattleSprite", False),
            ),
            ("BugChainBattleSprite", "BugChargeGospelSprite"),
        ),
        (
            0x31CD8, "ImportedSpriteGroup14Table", 0x32114, 0x80,
            (
                (0x14, "ChaosImpactSprite", False),
                (0x17, "DeathPhoenixStrikeSprite", False),
            ),
            (),
        ),
    )
    for root_offset, table_name, original_offset, native_length, entries, appended in sprite_tables:
        table = rom_offset(symbol(table_name))
        verify(u32(output, root_offset) == symbol(table_name), f"{label} {table_name} root pointer")
        expected = bytearray(original[original_offset:original_offset + native_length])
        for index, target, compressed in entries:
            pointer = symbol(target) | (0x80000000 if compressed else 0)
            struct.pack_into("<I", expected, index * 4, pointer)
        verify(
            output[table:table + native_length] == expected,
            f"{label} {table_name} preserves native and imported pointers",
        )
        for append_index, target in enumerate(appended):
            verify(
                u32(output, table + native_length + append_index * 4) == symbol(target),
                f"{label} {table_name} appended {target} pointer",
            )
    verify(
        output[0x31DA4:0x32194] == original[0x31DA4:0x32194],
        f"{label} original sprite tables remain unmodified",
    )
    bugcharge_sprite = rom_offset(symbol("BugChargeChargeSprite"))
    verify(
        output[bugcharge_sprite:bugcharge_sprite + 12]
        == b"\x10\x00\x01\x03\x0C\x00\x00\x00\x70\x00\x00\x00",
        f"{label} BugCharge uncompressed sprite-archive header",
    )
    folderback_type1_table = rom_offset(symbol("FolderBackType1DispatchTable"))
    verify(
        all(
            u32(output, folderback_type1_table + index * 4) == symbol("FolderBackType1Main") + 1
            for index in range(0x5F)
        ),
        f"{label} FolderBack type-1 proxy table includes counter family 0x5E",
    )
    verify(output[0x3E00:0x3E08] == original[0x3E00:0x3E08], f"{label} unused RollArrow type-1 slots")
    verify(output[0x3D70:0x3D78] == original[0x3D70:0x3D78], f"{label} native type-1 projectile slots")
    native_callback = 0xC4C12 if label == "Falzar" else 0xC6482
    verify(output[native_callback:native_callback + 10] == original[native_callback:native_callback + 10], f"{label} native player projectile callback")

    # Both text-archive halves are independently relocated and every known ROM
    # reference is redirected. Changes are addressed by full chip ID.
    archive_symbols = (
        ("ChipNameArchive0", "ChipNameArchive1"),
        ("ChipDescriptionArchive0", "ChipDescriptionArchive1"),
    )
    for archive_index, references in enumerate(name_references):
        target = symbol(archive_symbols[0][archive_index])
        for reference in references:
            verify(u32(output, reference) == target, f"{label} name archive {archive_index} ref 0x{reference:X}")
    for archive_index, references in enumerate(description_references):
        target = symbol(archive_symbols[1][archive_index])
        for reference in references:
            verify(u32(output, reference) == target, f"{label} description archive {archive_index} ref 0x{reference:X}")

    original_name_offsets = [u32(original, name_pointer_table + index * 4) - 0x08000000 for index in range(2)]
    original_description_offsets = [
        u32(original, description_pointer_table + index * 4) - 0x08000000 for index in range(2)
    ]
    installed_name_offsets = [rom_offset(symbol(name)) for name in archive_symbols[0]]
    installed_description_offsets = [rom_offset(symbol(name)) for name in archive_symbols[1]]
    original_archives = [archive_entries(original, offset) for offset in original_name_offsets]
    installed_archives = [archive_entries(output, offset) for offset in installed_name_offsets]
    original_descriptions = [archive_entries(original, offset) for offset in original_description_offsets]
    installed_descriptions = [archive_entries(output, offset) for offset in installed_description_offsets]

    verify(len(original_archives[0]) == 0x100, f"{label} earlier name archive has 256 entries")
    verify(len(original_descriptions[0]) == 0x100, f"{label} earlier description archive has 256 entries")
    verify(len(original_archives[1]) == len(original_descriptions[1]), f"{label} later text archive counts")

    for archive_index, (before, after) in enumerate(zip(original_archives, installed_archives)):
        verify(len(before) == len(after), f"{label} name archive {archive_index} entry count")
        for entry_index, (old_entry, new_entry) in enumerate(zip(before, after)):
            chip_id = archive_index * 0x100 + entry_index
            expected = CHIP_NAMES.get(chip_id, old_entry)
            verify(new_entry == expected, f"{label} name entry 0x{chip_id:X}")
    for archive_index, (before, after) in enumerate(zip(original_descriptions, installed_descriptions)):
        verify(len(before) == len(after), f"{label} description archive {archive_index} entry count")
        for entry_index, (old_entry, new_entry) in enumerate(zip(before, after)):
            chip_id = archive_index * 0x100 + entry_index
            expected = CHIP_DESCRIPTIONS.get(chip_id, old_entry)
            verify(new_entry == expected, f"{label} description entry 0x{chip_id:X}")

    installed_names = [entry for archive in installed_archives for entry in archive]
    expected_sorts = alphabetical_sort_values(original, installed_names)
    for chip_id in range(CHIP_RECORD_COUNT):
        record = CHIP_DATA_OFFSET + chip_id * CHIP_RECORD_SIZE
        expected = expected_sorts.get(chip_id, 0)
        verify(
            u16(output, record + CHIP_SORT_OFFSET) == expected,
            f"{label} chip 0x{chip_id:03X} final alphabetical sort",
        )

    for old_offset in original_name_offsets + original_description_offsets:
        old_pointer = struct.pack("<I", old_offset + 0x08000000)
        verify(old_pointer not in output[:0x800000], f"{label} stale text pointer 0x{old_offset + 0x08000000:08X}")

    # Folder Edit normally diverts DblBeast plus the local version mascot to
    # save-backed e-Reader names. The patched early branch skips that complete
    # exception block and falls through to the relocated ROM archive.
    if label == "Falzar":
        verify(original[0x120116:0x120118] == b"\x1B\x4A", f"{label} native Folder Edit name exceptions")
        verify(output[0x120116:0x120118] == b"\x0A\xE0", f"{label} Folder Edit archive path")
    else:
        verify(original[0x121EF2:0x121EF4] == b"\x1B\x4A", f"{label} native Folder Edit name exceptions")
        verify(output[0x121EF2:0x121EF4] == b"\x0A\xE0", f"{label} Folder Edit archive path")

    roll_codes = (b"\x00\x05\x13\xFF", b"\x03\x11\x16\xFF", b"\x10\x18\x19\xFF")
    roll_rarities = (1, 2, 3)
    roll_mb = (0x20, 0x26, 0x2C)
    roll_powers = (50, 70, 90)
    roll_icons = ("RollArrowIcon1", "RollArrowIcon2", "RollArrowIcon3")
    roll_palettes = ("RollArrowPalette1", "RollArrowPalette2", "RollArrowPalette3")
    for index in range(3):
        record = output[ROLLARROW_RECORDS + index * 0x2C:ROLLARROW_RECORDS + (index + 1) * 0x2C]
        verify(record[:4] == roll_codes[index], f"{label} RollArrow {index + 1} codes")
        verify(record[4:8] == bytes((0, roll_rarities[index], 0x0A, 0)), f"{label} RollArrow {index + 1} rank/type")
        verify(record[8:0x10] == bytes((roll_mb[index], 0x43, 0x94, 0x1B, 0x0E, 0x0A, 0x04, 0)), f"{label} RollArrow {index + 1} behavior")
        verify(record[0x10:0x18] == bytes((0, 0, 0, 0, 0, 0x17 + index, 0, 0x0A)), f"{label} RollArrow {index + 1} parameters")
        verify(u16(record, 0x18) == expected_sorts[0x018 + index], f"{label} RollArrow {index + 1} alphabetical sort")
        verify(u16(record, 0x1A) == roll_powers[index], f"{label} RollArrow {index + 1} power")
        verify(u16(record, 0x1C) == 0x17 + index, f"{label} RollArrow {index + 1} library position")
        verify(record[0x1E:0x20] == b"\x01\xFF", f"{label} RollArrow {index + 1} gate/dark ID")
        verify(u32(record, 0x20) == symbol(roll_icons[index]), f"{label} RollArrow {index + 1} icon")
        verify(u32(record, 0x24) == symbol("RollArrowImage"), f"{label} RollArrow {index + 1} image")
        verify(u32(record, 0x28) == symbol(roll_palettes[index]), f"{label} RollArrow {index + 1} palette")

    laserman_params = (0, 3, 4)
    laserman_rarities = (2, 3, 4)
    laserman_mb = (60, 80, 80)
    laserman_powers = (100, 150, 200)
    laserman_palettes = ("LaserManPaletteBase", "LaserManPaletteEx", "LaserManPaletteSp")
    for index in range(3):
        start = LASERMAN_RECORDS + index * 0x2C
        record = output[start:start + 0x2C]
        original_record = original[start:start + 0x2C]
        expected_codes = b"\x0B\x1A\xFF\xFF" if index == 0 else b"\x0B\xFF\xFF\xFF"
        verify(record[:4] == expected_codes, f"{label} LaserMan {index} code")
        verify(
            record[4:8] == bytes((0, laserman_rarities[index], 0x0A, 1)),
            f"{label} LaserMan {index} rarity/element/class",
        )
        verify(
            record[8:0x10] == bytes((laserman_mb[index], 0x47, 0x8A, 0x1B, 0x02, 0, 4, 0)),
            f"{label} LaserMan {index} behavior route",
        )
        verify(u32(record, 0x10) == laserman_params[index], f"{label} LaserMan {index} variant")
        verify(record[0x14:0x18] == original_record[0x14:0x18], f"{label} LaserMan {index} reserved metadata")
        verify(u16(record, 0x18) == expected_sorts[0x0E3 + index], f"{label} LaserMan {index} alphabetical sort")
        verify(u16(record, 0x1A) == laserman_powers[index], f"{label} LaserMan {index} power")
        verify(record[0x1C:0x20] == original_record[0x1C:0x20], f"{label} LaserMan {index} library metadata")
        verify(u32(record, 0x20) == symbol("LaserManIcon"), f"{label} LaserMan {index} icon")
        verify(u32(record, 0x24) == symbol("LaserManImage"), f"{label} LaserMan {index} image")
        verify(u32(record, 0x28) == symbol(laserman_palettes[index]), f"{label} LaserMan {index} palette")

    behavior_params = (0, 3, 4)
    codes = (b"\x12\x1A\xFF\xFF", b"\x12\xFF\xFF\xFF", b"\x12\xFF\xFF\xFF")
    powers = (20, 40, 75)
    palettes = ("SearchManPaletteBase", "SearchManPaletteEx", "SearchManPaletteSp")
    for index in range(3):
        record = output[SEARCHMAN_RECORDS + index * 0x2C:SEARCHMAN_RECORDS + (index + 1) * 0x2C]
        verify(record[:4] == codes[index], f"{label} SearchMan {index} codes")
        verify(record[6] == 6 and record[0x0B] == 0x1B and record[0x0C] == 0x0E, f"{label} SearchMan {index} type")
        verify(u32(record, 0x10) == behavior_params[index], f"{label} SearchMan {index} variant")
        verify(u16(record, 0x1A) == powers[index], f"{label} SearchMan {index} power")
        verify(u32(record, 0x20) == symbol("SearchManIcon"), f"{label} SearchMan {index} icon")
        verify(u32(record, 0x24) == symbol("SearchManImage"), f"{label} SearchMan {index} image")
        verify(u32(record, 0x28) == symbol(palettes[index]), f"{label} SearchMan {index} palette")

    assets = [
        ("JealousyIcon", 0x748F38, 0x80, "Jealousy icon"),
        ("JealousyImage", 0x7250E8, 0x540, "Jealousy image"),
        ("JealousyPalette", 0x734188, 0x20, "Jealousy palette"),
        ("JealousyEffectTiles", 0x6FAD2C, 0x100, "Jealousy delete-overlay tiles"),
        ("JealousyEffectPalette", 0x6FAE2C, 0x20, "Jealousy delete-overlay palette"),
        ("SearchManIcon", 0x7493B8, 0x80, "SearchMan icon"),
        ("SearchManImage", 0x728568, 0x540, "SearchMan image"),
        ("SearchManPaletteBase", 0x7343C8, 0x20, "SearchMan base palette"),
        ("SearchManPaletteSp", 0x7343E8, 0x20, "SearchMan SP palette"),
        ("SearchManBattleSprite", 0x254F64, 0xABFC, "SearchMan battle archive"),
        ("SearchManReticleAltSprite", 0x358410, 0x5B8, "alternate reticle archive"),
        ("SearchManReticleSprite", 0x3589C8, 0x460, "reticle archive"),
        ("ChaosBassSprite", 0x2D3304, 0x1081C, "ChaosLrd Bass archive"),
        ("ChaosApparitionSprite", 0x398024, 0x186C, "ChaosLrd apparition archive"),
        ("ChaosAuraSprite", 0x2E3B20, 0x56E0, "ChaosLrd aura archive"),
        ("ChaosTrigTable", 0x5CD0, 0x280, "ChaosLrd trig table"),
        ("ChaosTeardownSprite", 0x389E68, 0x11F0, "ChaosLrd teardown archive"),
        ("ChaosImpactSprite", 0x3906A8, 0x6B8, "ChaosLrd impact archive"),
    ]
    for target, source_offset, length, description in assets:
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn5[source_offset:source_offset + length], f"{label} {description}")

    roll_assets = [
        ("RollArrowIcon1", 0x74476C, 0x80, "RollArrow1 icon"),
        ("RollArrowIcon2", 0x7447EC, 0x80, "RollArrow2 icon"),
        ("RollArrowIcon3", 0x74486C, 0x80, "RollArrow3 icon"),
        ("RollArrowImage", 0x729D2C, 0x540, "RollArrow image"),
        ("RollArrowPalette1", 0x73EAEC, 0x20, "RollArrow1 palette"),
        ("RollArrowPalette2", 0x73EB0C, 0x20, "RollArrow2 palette"),
        ("RollArrowPalette3", 0x73EB2C, 0x20, "RollArrow3 palette"),
        ("RollArrowActorSprite", 0x2A5A10, 0xAC58, "Roll actor archive"),
        ("RollArrowProjectileSprite", 0x35E5C0, 0x160, "heart-arrow archive"),
    ]
    for target, source_offset, length, description in roll_assets:
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn4[source_offset:source_offset + length], f"{label} {description}")
    bugchain_assets = [
        ("BugChainIcon", 0x74626C, 0x80, "BugChain icon"),
        ("BugChainImage", 0x7315EC, 0x540, "BugChain image"),
        ("BugChainPalette", 0x73F1AC, 0x20, "BugChain palette"),
        ("BugChainBattleSprite", 0x380CA4, 0xF8C, "BugChain battle archive"),
    ]
    for target, source_offset, length, description in bugchain_assets:
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn4[source_offset:source_offset + length], f"{label} {description}")
    bugcharge_assets = [
        ("BugChargeIcon", 0x74AE3C, 0x80, "BugCharge icon"),
        ("BugChargeImage", 0x730664, 0x540, "BugCharge image"),
        ("BugChargePalette", 0x735D64, 0x20, "BugCharge palette"),
        ("BugChargeChargeSprite", 0x322158, 0x8EC, "BugCharge charge-orbit archive"),
        ("BugChargeGospelSprite", 0x348030, 0x6A8, "BugCharge Gospel-head archive"),
    ]
    for target, source_offset, length, description in bugcharge_assets:
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn5_colonel[source_offset:source_offset + length], f"{label} {description}")
    laserman_assets = [
        ("LaserManIcon", 0x74676C, 0x80, "LaserMan icon"),
        ("LaserManImage", 0x73842C, 0x540, "LaserMan image"),
        ("LaserManPaletteBase", 0x73F94C, 0x20, "LaserMan base palette"),
        ("LaserManPaletteSp", 0x73F96C, 0x20, "LaserMan SP palette"),
        ("LaserManBattleSprite", 0x339B6C, 0x395C, "LaserMan actor/row-laser archive"),
    ]
    for target, source_offset, length, description in laserman_assets:
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn4[source_offset:source_offset + length], f"{label} {description}")

    laser_ex_start = rom_offset(symbol("LaserManPaletteEx"))
    laser_base = bn4[0x73F94C:0x73F96C]
    laser_sp = bn4[0x73F96C:0x73F98C]
    laser_ex = output[laser_ex_start:laser_ex_start + 0x20]
    verify(laser_ex[:0x02] == laser_base[:0x02], f"{label} LaserMan EX transparent entry")
    verify(
        laser_ex[0x02:0x0C] == struct.pack("<5H", 0x00C0, 0x0180, 0x0280, 0x03E0, 0x0060),
        f"{label} LaserMan EX green background",
    )
    verify(laser_ex[0x0C:] == laser_base[0x0C:], f"{label} LaserMan EX foreground unchanged")
    verify(laser_ex != laser_base, f"{label} distinct green LaserMan EX background")
    base_palette = output[rom_offset(symbol("SearchManPaletteBase")):rom_offset(symbol("SearchManPaletteBase")) + 0x20]
    ex_palette = output[rom_offset(symbol("SearchManPaletteEx")):rom_offset(symbol("SearchManPaletteEx")) + 0x20]
    sp_palette = output[rom_offset(symbol("SearchManPaletteSp")):rom_offset(symbol("SearchManPaletteSp")) + 0x20]
    verify(ex_palette[:0x1A] == base_palette[:0x1A], f"{label} EX foreground palette")
    verify(ex_palette[0x1A:] == struct.pack("<3H", 0x03FF, 0x0299, 0x0190), f"{label} EX yellow background")
    verify(len({base_palette[0x1A:], ex_palette[0x1A:], sp_palette[0x1A:]}) == 3, f"{label} distinct chip-art backgrounds")

    jealousy_record = output[JEALOUSY_RECORD:JEALOUSY_RECORD + 0x2C]
    verify(jealousy_record[:4] == b"\x09\xFF\xFF\xFF", f"{label} Jealousy code")
    verify(jealousy_record[4:8] == b"\x00\x03\x0A\x00", f"{label} Jealousy rank/element/class")
    verify(jealousy_record[8:0x10] == b"\x3C\x43\x8A\x15\x07\x0A\x04\x00", f"{label} Jealousy MB/behavior")
    verify(jealousy_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\x00\x80\x10", f"{label} Jealousy parameters")
    verify(u16(jealousy_record, 0x18) == expected_sorts[0x0BF], f"{label} Jealousy alphabetical sort")
    verify(u16(jealousy_record, 0x1A) == 80, f"{label} Jealousy power")
    verify(u16(jealousy_record, 0x1C) == 0x00C3, f"{label} Jealousy library position")
    verify(jealousy_record[0x1E:0x20] == b"\x01\xFF", f"{label} Jealousy gate/dark ID")
    for field, target in ((0x20, "JealousyIcon"), (0x24, "JealousyImage"), (0x28, "JealousyPalette")):
        verify(u32(jealousy_record, field) == symbol(target), f"{label} {target} record pointer")

    bugchain_record = output[BUGCHAIN_RECORD:BUGCHAIN_RECORD + 0x2C]
    verify(bugchain_record[:4] == b"\x02\x1A\xFF\xFF", f"{label} BugChain C/* codes")
    verify(bugchain_record[4:8] == b"\x00\x03\x0A\x00", f"{label} BugChain rank/element/class")
    verify(bugchain_record[8:0x10] == b"\x3B\x41\x00\x15\x22\x0A\x04\x00", f"{label} BugChain MB/behavior")
    verify(bugchain_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\xC2\x80\x01", f"{label} BugChain parameters/library")
    verify(u16(bugchain_record, 0x18) == expected_sorts[0x0BE], f"{label} BugChain alphabetical sort")
    verify(u16(bugchain_record, 0x1A) == 0, f"{label} BugChain support-chip power")
    verify(u16(bugchain_record, 0x1C) == 0x00C2, f"{label} BugChain library position")
    verify(bugchain_record[0x1E:0x20] == b"\x03\xFF", f"{label} BugChain gate/dark ID")
    for field, target in ((0x20, "BugChainIcon"), (0x24, "BugChainImage"), (0x28, "BugChainPalette")):
        verify(u32(bugchain_record, field) == symbol(target), f"{label} {target} record pointer")
    bugchain_properties = rom_offset(symbol("BugChainByteProperties"))
    verify(
        output[bugchain_properties:bugchain_properties + 10]
        == bytes((0x31, 0x13, 0x14, 0x16, 0x24, 0x19, 0x18, 0x1A, 0x63, 0xFF)),
        f"{label} complete BN6 BugFix byte-property list",
    )

    verify(
        output[BUGFIX_RECORD:BUGFIX_RECORD + 0x18] == original[BUGFIX_RECORD:BUGFIX_RECORD + 0x18]
        and output[BUGFIX_RECORD + 0x1A:BUGFIX_RECORD + 0x2C]
        == original[BUGFIX_RECORD + 0x1A:BUGFIX_RECORD + 0x2C],
        f"{label} native BugFix record outside alphabetical sort",
    )
    verify(output[0x2CD1C:0x2CD20] == original[0x2CD1C:0x2CD20], f"{label} native BugFix time-freeze dispatch")
    verify(output[0x43B4:0x43B8] == original[0x43B4:0x43B8], f"{label} native BugFix type-4 dispatch")

    bugcharge_record = output[BUGCHARGE_RECORD:BUGCHARGE_RECORD + 0x2C]
    verify(bugcharge_record[:4] == b"\x01\xFF\xFF\xFF", f"{label} BN5 BugCharge B code")
    verify(bugcharge_record[4:8] == b"\x00\x04\x0A\x02", f"{label} BugCharge rarity/element/Giga class")
    verify(
        bugcharge_record[8:0x10]
        == bytes((0x4D, 0x41 if replace_bugcharge_art else 0x01, 0x8A, 0x15, 0x26, 0x01, 0x04, 0x00)),
        f"{label} BugCharge MB/behavior",
    )
    verify(bugcharge_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\x05\x14\x00", f"{label} BugCharge parameters/library")
    verify(u16(bugcharge_record, 0x18) == expected_sorts[0x131], f"{label} BugCharge alphabetical sort")
    verify(u16(bugcharge_record, 0x1A) == 200, f"{label} BN5 BugCharge per-shot power")
    verify(u16(bugcharge_record, 0x1C) == 0x0131, f"{label} BugCharge Giga library position")
    verify(bugcharge_record[0x1E:0x20] == b"\x01\xFF", f"{label} BugCharge gate/dark ID")
    if replace_bugcharge_art:
        for field, target in ((0x20, "BugChargeIcon"), (0x24, "BugChargeImage"), (0x28, "BugChargePalette")):
            verify(u32(bugcharge_record, field) == symbol(target), f"{label} {target} record pointer")
    else:
        verify(
            bugcharge_record[0x20:0x2C] == original[BUGCHARGE_RECORD + 0x20:BUGCHARGE_RECORD + 0x2C],
            f"{label} original Falzar BugRSword chip art",
        )
    bugcharge_properties = rom_offset(symbol("BugChargeByteProperties"))
    verify(
        output[bugcharge_properties:bugcharge_properties + 7]
        == bytes((0x13, 0x14, 0x16, 0x19, 0x18, 0x1A, 0xFF)),
        f"{label} BN5 BugCharge non-boolean byte-property list",
    )

    verify(
        output[BASS_RECORD:BASS_RECORD + 0x18] == original[BASS_RECORD:BASS_RECORD + 0x18]
        and output[BASS_RECORD + 0x1A:BASS_RECORD + 0x2C]
        == original[BASS_RECORD + 0x1A:BASS_RECORD + 0x2C],
        f"{label} Bass record outside alphabetical sort",
    )
    verify(output[0x2CDC4:0x2CDC8] == original[0x2CDC4:0x2CDC8], f"{label} Bass dispatch")
    chaos_record = output[CHAOSLORD_RECORD:CHAOSLORD_RECORD + 0x2C]
    verify(chaos_record[:4] == b"\x17\xFF\xFF\xFF", f"{label} ChaosLrd code")
    verify(chaos_record[5] == 4 and chaos_record[6] == 0x0A, f"{label} ChaosLrd rank/element")
    verify(chaos_record[8] == 0x63 and chaos_record[0x0B] == 0x1B and chaos_record[0x0C] == 0x17, f"{label} ChaosLrd type")
    verify(u16(chaos_record, 0x18) == expected_sorts[0x12E], f"{label} ChaosLrd alphabetical sort")
    verify(u16(chaos_record, 0x1A) == 500 and u16(chaos_record, 0x1C) == 0x12E, f"{label} ChaosLrd power/ID")
    verify(chaos_record[9] == (0x43 if replace_chaos_art else 0x03), f"{label} ChaosLrd library flags")
    if replace_chaos_art:
        for field, target, source, length in (
            (0x20, "ChaosLordIcon", 0x749C38, 0x80),
            (0x24, "ChaosLordImage", 0x72FE28, 0x540),
            (0x28, "ChaosLordPalette", 0x734AE8, 0x20),
        ):
            verify(u32(chaos_record, field) == symbol(target), f"{label} {target} record pointer")
            start = rom_offset(symbol(target))
            verify(output[start:start + length] == bn5[source:source + length], f"{label} {target}")
    else:
        verify(chaos_record[0x20:0x2C] == original[CHAOSLORD_RECORD + 0x20:CHAOSLORD_RECORD + 0x2C], f"{label} original Falzar chip art")

    signalred_record = output[SIGNALRED_RECORD:SIGNALRED_RECORD + 0x2C]
    verify(signalred_record[:4] == b"\x12\xFF\xFF\xFF", f"{label} SignalRed code")
    verify(signalred_record[4:8] == b"\x00\x04\x07\x00", f"{label} SignalRed rarity/element/Standard class")
    verify(
        signalred_record[8:0x10]
        == b"\x3D\x41\x00\x15\x26\x0A\x04\x00",
        f"{label} SignalRed MB/behavior",
    )
    verify(signalred_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\x14\xC6\x40", f"{label} SignalRed parameters/library")
    verify(u16(signalred_record, 0x18) == expected_sorts[0x0C1], f"{label} SignalRed alphabetical sort")
    verify(u16(signalred_record, 0x1A) == 0, f"{label} SignalRed displayed power")
    verify(u16(signalred_record, 0x1C) == 0x00C6, f"{label} SignalRed Standard library position")
    verify(signalred_record[0x1E:0x20] == b"\x01\xFF", f"{label} SignalRed gate/dark ID")
    for field, target, source, length in (
        (0x20, "SignalRedIcon", 0x746EEC, 0x80),
        (0x24, "SignalRedImage", 0x73A8EC, 0x540),
        (0x28, "SignalRedPalette", 0x73FAEC, 0x20),
    ):
        verify(u32(signalred_record, field) == symbol(target), f"{label} {target} record pointer")
        start = rom_offset(symbol(target))
        verify(output[start:start + length] == bn4[source:source + length], f"{label} {target}")

    signalred_sprite = rom_offset(symbol("SignalRedBattleSprite"))
    verify(
        output[signalred_sprite:signalred_sprite + 0x694] == bn4[0x381C30:0x3822C4],
        f"{label} SignalRed battle archive",
    )

    folderback_record = output[FOLDERBACK_RECORD:FOLDERBACK_RECORD + 0x2C]
    verify(folderback_record[:4] == b"\x1A\xFF\xFF\xFF", f"{label} FolderBack wildcard code")
    verify(folderback_record[4:8] == b"\x00\x04\x0A\x02", f"{label} FolderBack rarity/element/class")
    verify(
        folderback_record[8:0x10]
        == bytes((0x63, 0x41, 0x00, 0x15, 0x23, 0x0A, 0, 0)),
        f"{label} FolderBack MB/behavior route",
    )
    verify(folderback_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\x0D\x10\x00", f"{label} FolderBack parameters/library")
    verify(u16(folderback_record, 0x18) == expected_sorts[0x139], f"{label} FolderBack alphabetical sort")
    verify(u16(folderback_record, 0x1A) == 0, f"{label} FolderBack support-chip power")
    verify(u16(folderback_record, 0x1C) == 0x0139, f"{label} FolderBack library position")
    verify(folderback_record[0x1E:0x20] == b"\x01\xFF", f"{label} FolderBack gate/dark ID")
    folderback_assets = zip(
        ("FolderBackIcon", "FolderBackImage", "FolderBackPalette"),
        extract_folderback_art(bn3),
    )
    for target, expected in folderback_assets:
        field = {"FolderBackIcon": 0x20, "FolderBackImage": 0x24, "FolderBackPalette": 0x28}[target]
        verify(u32(folderback_record, field) == symbol(target), f"{label} {target} record pointer")
        start = rom_offset(symbol(target))
        verify(output[start:start + len(expected)] == expected, f"{label} imported {target}")
    native_dust_table = rom_offset(u32(original, 0x12010))
    dust_table = rom_offset(symbol("SignalRedDustSpriteTable"))
    verify(
        output[dust_table:dust_table + 0x20]
        == original[native_dust_table:native_dust_table + 0x1E] + struct.pack("<H", 0x1E10),
        f"{label} SignalRed DustCross sprite-table extension",
    )
    for pointer_offset in (0x12010, 0x12014, dust_suction_table_reference):
        verify(
            u32(output, pointer_offset) == symbol("SignalRedDustSpriteTable"),
            f"{label} DustCross table ref 0x{pointer_offset:X}",
        )

    native_song_table_length = 0x1DA * 8
    relocated_song_table = rom_offset(symbol("RelocatedSongTable"))
    signalred_song_entry = relocated_song_table + native_song_table_length
    verify(original[song_table_offset + native_song_table_length:song_table_offset + native_song_table_length + 8] == b"\x00" * 8, f"{label} reserved SFX 0x1DA")
    verify(
        output[relocated_song_table:signalred_song_entry]
        == original[song_table_offset:song_table_offset + native_song_table_length],
        f"{label} relocated native song table",
    )
    for reference in song_table_references:
        verify(u32(output, reference) == symbol("RelocatedSongTable"), f"{label} song-table ref 0x{reference:X}")
    verify(
        output[signalred_song_entry:signalred_song_entry + 8]
        == struct.pack("<IHH", symbol("SignalRedSpawnSong"), 0x000C, 0x000C),
        f"{label} SignalRed spawn song-table entry",
    )
    verify(
        output[signalred_song_entry + 8:signalred_song_entry + 16]
        == struct.pack("<IHH", symbol("RollArrowSummonSong"), 0x000C, 0x000C),
        f"{label} RollArrow summon song-table entry",
    )
    verify(
        output[signalred_song_entry + 16:signalred_song_entry + 24]
        == struct.pack("<IHH", symbol("RollArrowFireSong"), 0x000C, 0x000C),
        f"{label} RollArrow fire song-table entry",
    )
    verify(
        output[signalred_song_entry + 24:signalred_song_entry + 32]
        == struct.pack("<IHH", symbol("LaserManFireSong"), 0x000C, 0x000C),
        f"{label} LaserMan fire song-table entry",
    )
    verify(
        output[signalred_song_entry + 32:signalred_song_entry + 40]
        == struct.pack("<IHH", symbol("FolderBackRumbleSong"), 0x001B, 0x001B),
        f"{label} FolderBack rumble song-table entry",
    )
    verify(
        output[signalred_song_entry + 40:signalred_song_entry + 48]
        == struct.pack("<IHH", symbol("BugChainSoundSong"), 0x001C, 0x001C),
        f"{label} BugChain sound song-table entry",
    )
    verify(
        output[signalred_song_entry + 48:signalred_song_entry + 56]
        == struct.pack("<IHH", symbol("BugChargeChargeSong"), 0x0010, 0x0010),
        f"{label} BN5 BugCharge charge song-table entry",
    )
    verify(
        output[signalred_song_entry + 56:signalred_song_entry + 64]
        == struct.pack("<IHH", symbol("BugChargeFireSong"), 0x0014, 0x0014),
        f"{label} BN5 BugCharge fire song-table entry",
    )
    spawn_song = rom_offset(symbol("SignalRedSpawnSong"))
    spawn_voice = rom_offset(symbol("SignalRedSpawnVoicegroup"))
    spawn_track = rom_offset(symbol("SignalRedSpawnTrack"))
    spawn_sample = rom_offset(symbol("SignalRedSpawnSample"))
    verify(output[spawn_song:spawn_song + 4] == b"\x01\x00\x40\x00", f"{label} SignalRed spawn song header")
    verify(u32(output, spawn_song + 4) == symbol("SignalRedSpawnVoicegroup"), f"{label} SignalRed spawn voice pointer")
    verify(u32(output, spawn_song + 8) == symbol("SignalRedSpawnTrack"), f"{label} SignalRed spawn track pointer")
    verify(output[spawn_voice:spawn_voice + 4] == b"\x08\x3C\x00\x00", f"{label} SignalRed spawn voice")
    verify(u32(output, spawn_voice + 4) == symbol("SignalRedSpawnSample"), f"{label} SignalRed spawn sample pointer")
    verify(output[spawn_voice + 8:spawn_voice + 12] == b"\xFF\x00\xFF\x00", f"{label} SignalRed spawn envelope")
    verify(
        output[spawn_track:spawn_track + 0x10]
        == b"\xBC\x00\xBB\x4B\xBD\x00\xBF\x40\xBE\x7F\xDC\x3C\x7F\x8D\xB1\x00",
        f"{label} SignalRed spawn track",
    )
    verify(output[spawn_sample:spawn_sample + 0x891] == bn4[0x17C834:0x17D0C5], f"{label} SignalRed spawn PCM")

    for prefix, source_offset, sample_length, track in (
        ("RollArrowSummon", 0x184D70, 0xF3E, b"\xBC\x00\xBB\x4B\xBD\x00\xBF\x40\xBE\x7F\xE6\x3C\x7F\x97\xB1"),
        ("RollArrowFire", 0x1D2AFC, 0xC34, b"\xBC\x00\xBB\x4B\xBD\x00\xBF\x40\xBE\x7F\xE1\x3C\x7F\x92\xB1"),
    ):
        song = rom_offset(symbol(prefix + "Song"))
        voice = rom_offset(symbol(prefix + "Voicegroup"))
        track_offset = rom_offset(symbol(prefix + "Track"))
        sample = rom_offset(symbol(prefix + "Sample"))
        verify(output[song:song + 4] == b"\x01\x00\x40\x00", f"{label} {prefix} song header")
        verify(u32(output, song + 4) == symbol(prefix + "Voicegroup"), f"{label} {prefix} voice pointer")
        verify(u32(output, song + 8) == symbol(prefix + "Track"), f"{label} {prefix} track pointer")
        verify(output[voice:voice + 4] == b"\x08\x3C\x00\x00", f"{label} {prefix} voice")
        verify(u32(output, voice + 4) == symbol(prefix + "Sample"), f"{label} {prefix} sample pointer")
        verify(output[voice + 8:voice + 12] == b"\xFF\x00\xFF\x00", f"{label} {prefix} envelope")
        verify(output[track_offset:track_offset + len(track)] == track, f"{label} {prefix} track")
        verify(output[sample:sample + sample_length] == bn4[source_offset:source_offset + sample_length], f"{label} {prefix} PCM")

    laser_song = rom_offset(symbol("LaserManFireSong"))
    laser_voice = rom_offset(symbol("LaserManFireVoicegroup"))
    laser_track = rom_offset(symbol("LaserManFireTrack"))
    laser_sample = rom_offset(symbol("LaserManFireSample"))
    verify(output[laser_song:laser_song + 4] == b"\x01\x00\x40\x00", f"{label} LaserMan fire song header")
    verify(u32(output, laser_song + 4) == symbol("LaserManFireVoicegroup"), f"{label} LaserMan fire voice pointer")
    verify(u32(output, laser_song + 8) == symbol("LaserManFireTrack"), f"{label} LaserMan fire track pointer")
    verify(output[laser_voice:laser_voice + 4] == b"\x00\x3C\x00\x00", f"{label} LaserMan fire voice")
    verify(u32(output, laser_voice + 4) == symbol("LaserManFireSample"), f"{label} LaserMan fire sample pointer")
    verify(output[laser_voice + 8:laser_voice + 12] == b"\xFF\x00\xFF\x00", f"{label} LaserMan fire envelope")
    verify(
        output[laser_track:laser_track + 0x19]
        == b"\xBC\x00\xBB\x4B\xBD\x00\xBF\x40\xBE\x7F\xF6\x2F\x7F\xA2\x81\xBE\x60\x84\x40\x84\x20\x84\x10\x84\xB1",
        f"{label} LaserMan fire track",
    )
    verify(output[laser_sample:laser_sample + 0x144E] == bn4[0x1BCFF8:0x1BE446], f"{label} LaserMan fire PCM")

    folderback_song = rom_offset(symbol("FolderBackRumbleSong"))
    folderback_voice = rom_offset(symbol("FolderBackRumbleVoicegroup"))
    folderback_track = rom_offset(symbol("FolderBackRumbleTrack"))
    folderback_sample = rom_offset(symbol("FolderBackRumbleSample"))
    verify(output[folderback_song:folderback_song + 4] == b"\x01\x00\x40\x00", f"{label} FolderBack rumble song header")
    verify(u32(output, folderback_song + 4) == symbol("FolderBackRumbleVoicegroup"), f"{label} FolderBack rumble voice pointer")
    verify(u32(output, folderback_song + 8) == symbol("FolderBackRumbleTrack"), f"{label} FolderBack rumble track pointer")
    verify(output[folderback_voice:folderback_voice + 4] == b"\x08\x3C\x00\x00", f"{label} FolderBack rumble voice")
    verify(u32(output, folderback_voice + 4) == symbol("FolderBackRumbleSample"), f"{label} FolderBack rumble sample pointer")
    verify(output[folderback_voice + 8:folderback_voice + 12] == b"\xFF\x00\xFF\x00", f"{label} FolderBack rumble envelope")
    verify(
        output[folderback_track:folderback_track + 0x10]
        == b"\xBC\x00\xBB\x4B\xBD\x00\xBE\x7F\xBF\x40\xF9\x3C\x7F\xAA\x81\xB1",
        f"{label} FolderBack rumble track",
    )
    verify(output[folderback_sample:folderback_sample + 0x354E] == bn3[0x215B68:0x2190B6], f"{label} FolderBack rumble PCM")

    bugchain_song = rom_offset(symbol("BugChainSoundSong"))
    bugchain_voice = rom_offset(symbol("BugChainSoundVoicegroup"))
    bugchain_track = rom_offset(symbol("BugChainSoundTrack"))
    bugchain_sample = rom_offset(symbol("BugChainSoundSample"))
    verify(output[bugchain_song:bugchain_song + 4] == b"\x01\x00\x40\x00", f"{label} BugChain sound song header")
    verify(u32(output, bugchain_song + 4) == symbol("BugChainSoundVoicegroup"), f"{label} BugChain sound voice pointer")
    verify(u32(output, bugchain_song + 8) == symbol("BugChainSoundTrack"), f"{label} BugChain sound track pointer")
    verify(output[bugchain_voice:bugchain_voice + 4] == b"\x00\x3C\x00\x00", f"{label} BugChain sound voice")
    verify(u32(output, bugchain_voice + 4) == symbol("BugChainSoundSample"), f"{label} BugChain sound sample pointer")
    verify(output[bugchain_voice + 8:bugchain_voice + 12] == b"\xFF\x00\xFF\x00", f"{label} BugChain sound envelope")
    verify(
        output[bugchain_track:bugchain_track + 0x12]
        == b"\xBC\x00\xBB\x4B\xBD\x00\xBF\x40\xBE\x7F\xD7\x2E\x7F\x88\xDB\x39\x8C\xB1",
        f"{label} BugChain sound track",
    )
    verify(output[bugchain_sample:bugchain_sample + 0x4B1] == bn4[0x1970A0:0x197551], f"{label} BugChain sound PCM")

    bugcharge_charge_song = rom_offset(symbol("BugChargeChargeSong"))
    bugcharge_charge_voice = rom_offset(symbol("BugChargeChargeVoicegroup"))
    bugcharge_charge_track = rom_offset(symbol("BugChargeChargeTrack"))
    bugcharge_charge_sample = rom_offset(symbol("BugChargeChargeSample"))
    verify(output[bugcharge_charge_song:bugcharge_charge_song + 4] == b"\x01\x00\x80\x00", f"{label} BN5 BugCharge charge song header")
    verify(u32(output, bugcharge_charge_song + 4) == symbol("BugChargeChargeVoicegroup"), f"{label} BN5 BugCharge charge voice pointer")
    verify(u32(output, bugcharge_charge_song + 8) == symbol("BugChargeChargeTrack"), f"{label} BN5 BugCharge charge track pointer")
    verify(output[bugcharge_charge_voice:bugcharge_charge_voice + 4] == b"\x08\x3C\x00\x00", f"{label} BN5 BugCharge charge voice")
    verify(u32(output, bugcharge_charge_voice + 4) == symbol("BugChargeChargeSample"), f"{label} BN5 BugCharge charge sample pointer")
    verify(output[bugcharge_charge_voice + 8:bugcharge_charge_voice + 12] == b"\xFF\x00\xFF\x00", f"{label} BN5 BugCharge charge envelope")
    verify(
        output[bugcharge_charge_track:bugcharge_charge_track + 15]
        == bn5_colonel[0x181E80:0x181E8F],
        f"{label} BN5 BugCharge charge track",
    )
    verify(
        output[bugcharge_charge_sample:bugcharge_charge_sample + 0x676]
        == bn5_colonel[0x191A80:0x1920F6],
        f"{label} BN5 BugCharge charge PCM",
    )

    bugcharge_fire_song = rom_offset(symbol("BugChargeFireSong"))
    bugcharge_fire_voice = rom_offset(symbol("BugChargeFireVoicegroup"))
    bugcharge_fire_track = rom_offset(symbol("BugChargeFireTrack"))
    verify(output[bugcharge_fire_song:bugcharge_fire_song + 4] == b"\x01\x00\x80\x00", f"{label} BN5 BugCharge fire song header")
    verify(u32(output, bugcharge_fire_song + 4) == symbol("BugChargeFireVoicegroup"), f"{label} BN5 BugCharge fire voice pointer")
    verify(u32(output, bugcharge_fire_song + 8) == symbol("BugChargeFireTrack"), f"{label} BN5 BugCharge fire track pointer")
    verify(output[bugcharge_fire_voice:bugcharge_fire_voice + 12] == bn5_colonel[0x1552C4:0x1552D0], f"{label} BN5 BugCharge fire PSG voice")
    verify(
        output[bugcharge_fire_track:bugcharge_fire_track + 19]
        == bn5_colonel[0x18342C:0x18343F],
        f"{label} BN5 BugCharge fire track",
    )

    death_record = output[DEATHPHOENIX_RECORD:DEATHPHOENIX_RECORD + 0x2C]
    verify(death_record[:4] == b"\x03\xFF\xFF\xFF", f"{label} DeathPhoenix code")
    verify(death_record[4:8] == b"\x00\x04\x0A\x02", f"{label} DeathPhoenix null element/class")
    verify(
        death_record[8:0x10]
        == bytes((0x5D, 0x43 if replace_deathphoenix_art else 0x03, 0x94, 0x1B, 0x11, 0x0A, 0x04, 0x00)),
        f"{label} DeathPhoenix family",
    )
    verify(death_record[0x10:0x18] == b"\x00\x00\x00\x00\x00\x00\x00\x10", f"{label} DeathPhoenix parameters")
    verify(
        u16(death_record, 0x18) == expected_sorts[0x134] and u16(death_record, 0x1A) == 150,
        f"{label} DeathPhoenix sort/power",
    )
    verify(u16(death_record, 0x1C) == 0x134, f"{label} DeathPhoenix library position")
    if replace_deathphoenix_art:
        for field, target, source, length in (
            (0x20, "DeathPhoenixIcon", 0x749CB8, 0x80),
            (0x24, "DeathPhoenixImage", 0x730368, 0x540),
            (0x28, "DeathPhoenixPalette", 0x734B08, 0x20),
        ):
            verify(u32(death_record, field) == symbol(target), f"{label} {target} record pointer")
            start = rom_offset(symbol(target))
            verify(output[start:start + length] == bn5[source:source + length], f"{label} {target}")
        sprite_start = rom_offset(symbol("DeathPhoenixBattleSprite"))
    else:
        verify(death_record[0x20:0x2C] == original[DEATHPHOENIX_RECORD + 0x20:DEATHPHOENIX_RECORD + 0x2C], f"{label} original DeathPhoenix-slot menu art")
    sprite_start = rom_offset(symbol("DeathPhoenixBattleSprite"))
    verify(output[sprite_start:sprite_start + 0x20F4] == bn5[0x333400:0x3354F4], f"{label} DeathPhoenix battle archive")
    strike_start = rom_offset(symbol("DeathPhoenixStrikeSprite"))
    verify(output[strike_start:strike_start + 0x748] == bn5[0x36F074:0x36F7BC], f"{label} DeathPhoenix type-4 0x89/0x71 strike archive")

    search_code = output[rom_offset(symbol("SearchManCodeStart")):rom_offset(symbol("SearchManCodeEnd"))]
    roll_code = output[rom_offset(symbol("RollArrowCodeStart")):rom_offset(symbol("RollArrowCodeEnd"))]
    laser_code = output[rom_offset(symbol("LaserManCodeStart")):rom_offset(symbol("LaserManCodeEnd"))]
    chaos_code = output[rom_offset(symbol("ChaosCodeStart")):rom_offset(symbol("ChaosCodeEnd"))]
    jealousy_code = output[rom_offset(symbol("JealousyCodeStart")):rom_offset(symbol("JealousyCodeEnd"))]
    bugchain_code = output[rom_offset(symbol("BugChainCodeStart")):rom_offset(symbol("BugChainCodeEnd"))]
    bugcharge_code = output[rom_offset(symbol("BugChargeCodeStart")):rom_offset(symbol("BugChargeCodeEnd"))]
    signalred_code = output[rom_offset(symbol("SignalRedCodeStart")):rom_offset(symbol("SignalRedCodeEnd"))]
    folderback_code = output[rom_offset(symbol("FolderBackCodeStart")):rom_offset(symbol("FolderBackCodeEnd"))]
    verify(search_code and any(byte != 0xFF for byte in search_code), f"{label} SearchMan code")
    verify(roll_code and any(byte != 0xFF for byte in roll_code), f"{label} RollArrow code")
    verify(laser_code and any(byte != 0xFF for byte in laser_code), f"{label} LaserMan code")
    verify(struct.pack("<I", symbol("RollArrowTimeFreezeSpawn") + 1) in search_code, f"{label} RollArrow time-freeze route")
    verify(struct.pack("<I", symbol("RollArrowProjectileMain") + 1) in search_code, f"{label} RollArrow tagged type-3 route")
    verify(struct.pack("<I", symbol("ROLLARROW_PROJECTILE_TAG")) in search_code, f"{label} RollArrow type-3 tag")
    verify(struct.pack("<I", symbol("BugChargeProjectileMain") + 1) in search_code, f"{label} BugCharge tagged type-3 route")
    verify(struct.pack("<I", symbol("BUGCHARGE_PROJECTILE_TAG")) in search_code, f"{label} BugCharge type-3 tag")
    verify(struct.pack("<I", symbol("LaserManHitMain") + 1) in search_code, f"{label} LaserMan tagged type-3 route")
    verify(struct.pack("<I", symbol("LASERMAN_HIT_TAG")) in search_code, f"{label} LaserMan type-3 tag")
    verify(struct.pack("<I", symbol("BugChargeGospelMain") + 1) in search_code, f"{label} BugCharge Gospel visual route")
    verify(struct.pack("<I", symbol("BUGCHARGE_GOSPEL_TAG")) in search_code, f"{label} BugCharge Gospel visual tag")
    verify(
        b"\x01\xB4\x30\x1C\x00\x0C\x94\x28\x01\xBC" in search_code,
        f"{label} RollArrow packed-attack discriminator",
    )
    verify(chaos_code and any(byte != 0xFF for byte in chaos_code), f"{label} ChaosLrd code")
    verify(
        contains_thumb_bl(
            output,
            rom_offset(symbol("ChaosAttackMain")),
            rom_offset(symbol("ChaosAttackInit")),
            symbol("SignalRedObjectMain"),
        ),
        f"{label} ChaosLrd preserves SignalRed shared type-3 route",
    )
    verify(
        output[rom_offset(symbol("ChaosSetCompletionActive")):rom_offset(symbol("ChaosSetCompletionActive")) + 2]
        == b"\x39\x70",
        f"{label} ChaosLrd sets the BN6 completion byte without corrupting cut-in state",
    )
    verify(
        output[rom_offset(symbol("ChaosReleaseCompletion")):rom_offset(symbol("ChaosReleaseCompletion")) + 2]
        == b"\x08\x60",
        f"{label} ChaosLrd retires the completed BN6 time-freeze record",
    )
    verify(
        output[
            rom_offset(symbol("ChaosFinalizeCompletionRecord")):
            rom_offset(symbol("ChaosFinalizeCompletionRecord")) + 2
        ]
        == b"\x08\x60",
        f"{label} ChaosLrd normalizes its completion record after cut-in arbitration",
    )
    verify(
        output[rom_offset(symbol("ChaosSetOutroTimer")):rom_offset(symbol("ChaosSetOutroTimer")) + 2]
        == b"\x0B\x20",
        f"{label} ChaosLrd exits inside BN6's stacked time-freeze scheduling window",
    )
    verify(jealousy_code and any(byte != 0xFF for byte in jealousy_code), f"{label} Jealousy code")
    verify(bugchain_code and any(byte != 0xFF for byte in bugchain_code), f"{label} BugChain code")
    verify(struct.pack("<I", symbol("BUGCHAIN_TAG")) in bugchain_code, f"{label} BugChain controller tag")
    verify(struct.pack("<I", symbol("BUGCHAIN_VISUAL_TAG")) in bugchain_code, f"{label} BugChain visual tag")
    verify(
        struct.pack("<I", symbol("JealousyMain") + 1) in bugchain_code,
        f"{label} BugChain preserves Jealousy shared type-4 route",
    )
    for target in (0x080005CD, 0x0802D247):
        verify(struct.pack("<I", target) in bugchain_code, f"{label} BugChain runtime target 0x{target:08X}")
    verify(b"\x08\x21\x08\x42" in bugchain_code, f"{label} BugChain link-battle flag test")
    verify(struct.pack("<I", 0x0800A8F9) not in bugchain_code, f"{label} BugChain has no secondary battle helper gate")
    verify(b"\xE0\x20\xFF\x30" in bugchain_code, f"{label} BugChain imported SFX 0x1DF")
    verify(bugcharge_code and any(byte != 0xFF for byte in bugcharge_code), f"{label} BugCharge code")
    for target in (
        0x080005CD, 0x080026A5, 0x080026E5, 0x08002D81, 0x08002DA5,
        0x08003359, 0x080033AD, 0x08003459, 0x0800A18F, 0x0800CC87,
        0x0800E29D, 0x080103BD, 0x08013683,
        0x08019893, 0x080198CF, 0x08019FB5, 0x0801A00F, 0x0801A019,
        0x0801A075, 0x0801A0D5, 0x0801A141, 0x0801BBF5,
        0x080302A9,
    ):
        verify(struct.pack("<I", target) in bugcharge_code, f"{label} BugCharge runtime target 0x{target:08X}")
    verify(struct.pack("<I", symbol("ChaosTrigTable")) in bugcharge_code, f"{label} BugCharge native BN5 orbit table")
    verify(struct.pack("<I", 0x02034887) not in bugcharge_code, f"{label} BugCharge has no BN4 Custom-turn counter")
    verify(signalred_code and any(byte != 0xFF for byte in signalred_code), f"{label} SignalRed code")
    verify(folderback_code and any(byte != 0xFF for byte in folderback_code), f"{label} FolderBack code")
    for target in (
        0x080005CD, 0x08002379, 0x0800239B, 0x080033AD,
        0x08003459, 0x0800A029, 0x0800A18F,
        0x0800A319, 0x0800A571, 0x0800A955,
        0x0800A9ED, 0x0801BBF5, 0x0801DFA3, 0x0801E15D, 0x0802E071, 0x080302A9,
    ):
        verify(struct.pack("<I", target) in folderback_code, f"{label} FolderBack runtime target 0x{target:08X}")
    for stale_target in (0x0800B917, 0x0800B94D, 0x0800B9B1, 0x0800BC89, 0x0800BD35):
        verify(struct.pack("<I", stale_target) not in folderback_code, f"{label} FolderBack has no time-freeze lifecycle 0x{stale_target:08X}")
    verify(struct.pack("<I", 0x0203CA70) in folderback_code, f"{label} FolderBack battle-state target")
    verify(struct.pack("<I", 0x0203CDB0) in folderback_code, f"{label} FolderBack restored queue target")
    verify(struct.pack("<I", 0x00004000) in folderback_code, f"{label} FolderBack full Custom Gauge value")
    verify(b"\x8F\x20" in folderback_code, f"{label} FolderBack native FullCust SFX")
    verify(struct.pack("<I", 0x00004210) in folderback_code, f"{label} FolderBack pale flash color")
    verify(struct.pack("<I", 0x00006318) in folderback_code, f"{label} FolderBack white flash color")
    verify(struct.pack("<I", 0x46424B36) in folderback_code, f"{label} FolderBack shared-slot tag")
    verify(
        struct.pack("<I", symbol("SignalRedBugChargeSharedMain") + 1) in folderback_code,
        f"{label} FolderBack preserves SignalRed/BugCharge shared-slot route",
    )
    death_code = output[rom_offset(symbol("DeathPhoenixCodeStart")):rom_offset(symbol("DeathPhoenixCodeEnd"))]
    verify(death_code and any(byte != 0xFF for byte in death_code), f"{label} DeathPhoenix code")
    for target in (
        0x0203C960, 0x0802CD5C, death_damage_object + 1,
        0x08003007, 0x08002DEB, 0x0801BBAD, death_navi_transition + 1,
    ):
        verify(struct.pack("<I", target) in death_code, f"{label} DeathPhoenix runtime target 0x{target:08X}")
    verify(struct.pack("<I", 0x01001417) in death_code, f"{label} DeathPhoenix strike sprite selector")
    verify(struct.pack("<I", 0x080065E0) in death_code, f"{label} DeathPhoenix flame sine table")
    verify(struct.pack("<I", 0x0A050001) in death_code, f"{label} DeathPhoenix contact flags")
    recycle_cleanup = rom_offset(symbol("DeathRecycleCleanup"))
    verify(
        output[recycle_cleanup:recycle_cleanup + 12]
        == b"\x00\x20\x69\x6D\x08\x70\x08\x20\xA8\x60\x70\x47",
        f"{label} DeathPhoenix releases completion without a duplicate return wait",
    )
    spawn_sound = rom_offset(symbol("SignalRedPlaySpawnSound"))
    green_sound = rom_offset(symbol("SignalRedPlayGreenSound"))
    verify(output[spawn_sound:spawn_sound + 4] == b"\xED\x20\x40\x00", f"{label} SignalRed imported placement SFX 0x1DA")
    verify(output[green_sound:green_sound + 2] == b"\xD1\x20", f"{label} SignalRed BN6 green SFX 0x0D1")
    verify(struct.pack("<I", 0x00300000) in signalred_code, f"{label} SignalRed DustCross suction mask")
    verify(struct.pack("<I", 0x030016F0) in jealousy_code, f"{label} Jealousy BN6 palette staging address")
    verify(struct.pack("<I", 0x030036F0) not in jealousy_code, f"{label} no stale BN5 palette staging address")
    for target in (0x08002379, 0x0800239B, 0x080E4329):
        verify(struct.pack("<I", target) in chaos_code, f"{label} ChaosLrd runtime target 0x{target:08X}")
    for stale_target in (0x0800B917, 0x0800B94D, 0x0800B9B1, 0x0800BC89, 0x0800BD35):
        verify(struct.pack("<I", stale_target) not in chaos_code, f"{label} no stale field handler 0x{stale_target:08X}")
    for target in (
        0x0800B917, 0x0800B94D, 0x0800B9B1, 0x0800BC89, 0x0800BD35,
        0x08000AC9, 0x0800A8F9, 0x0800A9ED, 0x0800AE91, 0x08010019,
        0x0802E04F, jealousy_damage_object + 1,
    ):
        verify(struct.pack("<I", target) in jealousy_code, f"{label} Jealousy runtime target 0x{target:08X}")
    for target in (
        0x0800B917, 0x0800B94D, 0x0800B9B1, 0x0800BC89, 0x0800BD35,
        0x08001383, 0x0800138F, 0x0800F615, 0x0800F657, 0x0800F8CF, 0x0800F90F,
        0x08002E3D, 0x08002F5D,
        0x08019893, 0x080198CF, 0x08019FB5,
        0x0801A04D, 0x0801A019, 0x0801A181,
    ):
        verify(struct.pack("<I", target) in signalred_code, f"{label} SignalRed runtime target 0x{target:08X}")
    for target in (
        0x0800CC73, 0x0800E277, 0x0800E2AD,
        0x08019893, 0x08019FB5, 0x0801A00F, 0x0801A019,
        0x0801A04D, 0x0801A075, 0x0801A0D5, 0x0801A141,
    ):
        verify(struct.pack("<I", target) in roll_code, f"{label} RollArrow runtime target 0x{target:08X}")
    verify(b"\x24\x20\x00\x04\xE0\x63" in roll_code, f"{label} RollArrow native 36-pixel bow height")
    verify(struct.pack("<I", 0x000C0000) not in roll_code, f"{label} no non-native backward retreat")
    verify(struct.pack("<I", 0xFF851705) not in roll_code, f"{label} no KendoMan collision flags")
    verify(struct.pack("<I", 0xFF841705) not in roll_code, f"{label} no KendoMan reverse collision flags")
    verify(struct.pack("<I", 0x20050001) not in roll_code, f"{label} no WindMan tornado collision flags")
    for target in (
        0x080005CD, 0x080026A5, 0x080026E5, 0x08002B31, 0x08002D81,
        0x08002DA5, 0x08002DEB, 0x08002E15, 0x08002E3D, 0x08002ED1, 0x08002F5D, 0x08002F91,
        0x08003321, 0x08003359, 0x08003459, 0x0800A0F5, 0x0800CC87, 0x0800E29D,
        0x0800E2C1, 0x0800E457, 0x080103BD, 0x080136B1, 0x080136CD, 0x0801A15D,
        0x08019893, 0x080198CF,
        0x08019FB5, 0x0801A00F, 0x0801A019, 0x0801A075, 0x0801A0D5,
        0x0801A141, 0x0801A4D1, 0x0801BBF5,
    ):
        verify(struct.pack("<I", target) in laser_code, f"{label} LaserMan runtime target 0x{target:08X}")
    verify(struct.pack("<I", 0x4C415345) in laser_code, f"{label} LaserMan laser tag")
    verify(struct.pack("<I", 0x4C484954) in laser_code, f"{label} LaserMan hit tag")
    verify(struct.pack("<I", 0x0800E277) not in laser_code, f"{label} no stale random-panel beam logic")
    command_streams = {
        "LaserManCommandNone": (0x00FD, 0x00FF),
        "LaserManCommandUp": (0x0005, 0x0006, 0x0007, 0x00FD, 0x00FF),
        "LaserManCommandDown": (0x0001, 0x0002, 0x0003, 0x0004, 0xFF0C, 0x00FD, 0x00FF),
        "LaserManCommandRight": (0x010A, 0x00FD, 0x00FF),
        "LaserManCommandLeft": (0x00FE, 0x00FD, 0x00FF),
    }
    for stream_name, events in command_streams.items():
        stream_start = rom_offset(symbol(stream_name))
        expected = struct.pack(f"<{len(events)}H", *events)
        verify(
            output[stream_start:stream_start + len(expected)] == expected,
            f"{label} {stream_name} Blue Moon effect stream",
        )
    beam_properties = rom_offset(symbol("LaserManCommandBeamProperties"))
    verify(
        output[beam_properties:beam_properties + 20]
        == struct.pack("<5I", 0, 0xB060, 0xA80A, 0, 0xB9C0),
        f"{label} LaserMan command beam properties",
    )
    actor_init = rom_offset(symbol("LaserManActorInit"))
    verify(
        output[actor_init:actor_init + 6] == b"\x00\xB5\x0C\x20\x56\x21",
        f"{label} LaserMan BN6 compressed-archive preload selector",
    )
    laser_init = output[
        rom_offset(symbol("LaserManLaserInit")):rom_offset(symbol("LaserManLaserUpdate"))
    ]
    verify(b"\x00\x20\xE8\x63" in laser_init, f"{label} LaserMan beam clears reused Z coordinate")
    verify(
        laser_init.count(b"\x01\x20") >= 1,
        f"{label} LaserMan beam uses foreground OAM priority",
    )
    hit_spawn = output[
        rom_offset(symbol("LaserManHitSpawn")):rom_offset(symbol("LaserManHitMain"))
    ]
    verify(
        b"\x19\x24" in hit_spawn,
        f"{label} LaserMan hit seeds BN6 normal collision region",
    )
    hit_init = output[
        rom_offset(symbol("LaserManHitInit")):rom_offset(symbol("LaserManHitUpdate"))
    ]
    verify(
        hit_init.count(b"\x29\x79") >= 2 and b"\x29\x79\x05\x22" in hit_init,
        f"{label} LaserMan hit reloads spawned collision region for presentation",
    )

    # BN5's panel predicate takes the ownership mask in r3, with r2 zero.
    # These literal-load/move pairs guard against silently swapping those
    # arguments again, which makes the attack scan find no valid panels.
    jealousy_attack = output[
        rom_offset(symbol("JealousyAttackField")):rom_offset(symbol("JealousyFinishDelete"))
    ]
    verify(b"\x00\x23" in jealousy_attack, f"{label} Jealousy panel predicate r3 zero")
    verify(struct.pack("<I", 0x00200000) in jealousy_attack, f"{label} Jealousy left-side r2 mask")
    verify(struct.pack("<I", 0x00400000) in jealousy_attack, f"{label} Jealousy right-side r2 mask")

    # Every Armips-managed allocation must be aligned, remain in expanded ROM,
    # and not overlap another allocation.
    allocated: list[tuple[int, int, str]] = []
    for start_name, end_name in (
        ("RollArrowCodeStart", "RollArrowCodeEnd"),
        ("LaserManCodeStart", "LaserManCodeEnd"),
        ("SearchManCodeStart", "SearchManCodeEnd"),
        ("ChaosCodeStart", "ChaosCodeEnd"),
        ("JealousyCodeStart", "JealousyCodeEnd"),
        ("BugChainCodeStart", "BugChainCodeEnd"),
        ("BugChargeCodeStart", "BugChargeCodeEnd"),
        ("SignalRedCodeStart", "SignalRedCodeEnd"),
        ("FolderBackCodeStart", "FolderBackCodeEnd"),
        ("SignalRedSpawnAudioStart", "SignalRedSpawnAudioEnd"),
        ("RollArrowAudioStart", "RollArrowAudioEnd"),
        ("LaserManFireAudioStart", "LaserManFireAudioEnd"),
        ("FolderBackRumbleAudioStart", "FolderBackRumbleAudioEnd"),
        ("BugChainSoundAudioStart", "BugChainSoundAudioEnd"),
        ("BugChargeAudioStart", "BugChargeAudioEnd"),
        ("RelocatedSongTable", "RelocatedSongTableEnd"),
    ):
        allocated.append((symbol(start_name), symbol(end_name), start_name))
    allocated.append((symbol("DeathPhoenixCodeStart"), symbol("DeathPhoenixCodeEnd"), "DeathPhoenixCodeStart"))
    for target, _, length, _ in assets:
        allocated.append((symbol(target), symbol(target) + length, target))
    for target, _, length, _ in roll_assets:
        allocated.append((symbol(target), symbol(target) + length, target))
    for target, _, length, _ in bugchain_assets:
        allocated.append((symbol(target), symbol(target) + length, target))
    for target, _, length, _ in bugcharge_assets:
        allocated.append((symbol(target), symbol(target) + length, target))
    for table_name in (
        "ImportedSpriteGroup08Table",
        "ImportedSpriteGroup0CTable",
        "ImportedSpriteGroup10Table",
        "ImportedSpriteGroup14Table",
    ):
        allocated.append((symbol(table_name), symbol(f"{table_name}End"), table_name))
    for target, _, length, _ in laserman_assets:
        allocated.append((symbol(target), symbol(target) + length, target))
    allocated.append((symbol("LaserManPaletteEx"), symbol("LaserManPaletteEx") + 0x20, "LaserManPaletteEx"))
    allocated.append((symbol("SearchManPaletteEx"), symbol("SearchManPaletteEx") + 0x20, "SearchManPaletteEx"))
    allocated.append((symbol("SignalRedBattleSprite"), symbol("SignalRedBattleSprite") + 0x694, "SignalRedBattleSprite"))
    for name, length in (("FolderBackIcon", 0x80), ("FolderBackImage", 0x540), ("FolderBackPalette", 0x20)):
        allocated.append((symbol(name), symbol(name) + length, name))
    if replace_chaos_art:
        allocated.extend((symbol(name), symbol(name) + length, name) for name, length in (("ChaosLordIcon", 0x80), ("ChaosLordImage", 0x540), ("ChaosLordPalette", 0x20)))
    allocated.extend((symbol(name), symbol(name) + length, name) for name, length in (("SignalRedIcon", 0x80), ("SignalRedImage", 0x540), ("SignalRedPalette", 0x20)))
    if replace_deathphoenix_art:
        allocated.extend(
            (symbol(name), symbol(name) + length, name)
            for name, length in (
                ("DeathPhoenixIcon", 0x80),
                ("DeathPhoenixImage", 0x540),
                ("DeathPhoenixPalette", 0x20),
            )
        )
    allocated.append((symbol("DeathPhoenixBattleSprite"), symbol("DeathPhoenixBattleSprite") + 0x20F4, "DeathPhoenixBattleSprite"))
    allocated.append((symbol("DeathPhoenixStrikeSprite"), symbol("DeathPhoenixStrikeSprite") + 0x748, "DeathPhoenixStrikeSprite"))
    for name in archive_symbols[0] + archive_symbols[1]:
        start = symbol(name)
        allocated.append((start, start + archive_extent(output, rom_offset(start)), name))
    allocated.sort()
    for index, (start, finish, name) in enumerate(allocated):
        verify(0x08800000 <= start < finish <= 0x09000000, f"{label} {name} expanded-ROM bounds")
        verify(start % 4 == 0, f"{label} {name} alignment")
        if index:
            verify(allocated[index - 1][1] <= start, f"{label} autoregion overlap: {allocated[index - 1][2]} / {name}")

    allowed_ranges = [
        interval(0x31CCC, 0x10),
        interval(0x3DE8, 4),
        interval(ROLLARROW_RECORDS, 3 * 0x2C),
        interval(0x2CD64, 4), interval(0x3D5C, 4),
        interval(LASERMAN_RECORDS, 3 * 0x2C),
        interval(0x2CD94, 4), interval(0x3D60, 4), interval(0x3F74, 4), interval(0x4324, 4),
        interval(SEARCHMAN_RECORDS, 3 * 0x2C),
        interval(0x2CDB8, 4), interval(0x3DE0, 8), interval(0x3F78, 4), interval(0x44F8, 8),
        interval(CHAOSLORD_RECORD, 0x2C),
        interval(0x2CCD0, 4), interval(0x4438, 4), interval(JEALOUSY_RECORD, 0x2C),
        interval(0x2CD3C, 4), interval(BUGCHAIN_RECORD, 0x2C),
        interval(0x2CD4C, 4), interval(0x44D8, 4),
        interval(SIGNALRED_RECORD, 0x2C), interval(BUGCHARGE_RECORD, 0x2C),
        interval(0x2CD40, 4), interval(0x3224, 4), interval(FOLDERBACK_RECORD, 0x2C),
        interval(0x12010, 8), interval(dust_suction_table_reference, 4),
    ]
    allowed_ranges.append(interval(0x120116 if label == "Falzar" else 0x121EF2, 2))
    allowed_ranges.extend(
        interval(CHIP_DATA_OFFSET + chip_id * CHIP_RECORD_SIZE + CHIP_SORT_OFFSET, 2)
        for chip_id in expected_sorts
    )
    allowed_ranges.extend(interval(reference, 4) for reference in song_table_references)
    allowed_ranges.extend((
        interval(0x2CDA0, 4), interval(0x3CA0, 4), interval(0x44CC, 8),
        interval(DEATHPHOENIX_RECORD, 0x2C),
    ))
    for references in name_references + description_references:
        allowed_ranges.extend(interval(reference, 4) for reference in references)
    unexpected = [
        offset
        for offset, (before, after) in enumerate(zip(original, output))
        if before != after and not any(offset in allowed for allowed in allowed_ranges)
    ][:16]
    verify(not unexpected, f"{label} unexpected writes at {', '.join(f'0x{x:X}' for x in unexpected)}")

    print(f"{label}: verified ({hashlib.sha256(output).hexdigest()})")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("bn5", type=Path)
    parser.add_argument("bn5_colonel", type=Path)
    parser.add_argument("bn4", type=Path)
    parser.add_argument("bn3", type=Path)
    parser.add_argument("gregar_original", type=Path)
    parser.add_argument("gregar_output", type=Path)
    parser.add_argument("gregar_symbols", type=Path)
    parser.add_argument("falzar_original", type=Path)
    parser.add_argument("falzar_output", type=Path)
    parser.add_argument("falzar_symbols", type=Path)
    args = parser.parse_args()
    bn5 = args.bn5.read_bytes()
    bn5_colonel = args.bn5_colonel.read_bytes()
    bn4 = args.bn4.read_bytes()
    bn3 = args.bn3.read_bytes()

    # Exercise full-ID routing independently of the patch's current entries:
    # 0x02A must modify the earlier archive and 0x100 the later archive.
    addressing_probe = [[b"original"] * 0x100, [b"original"]]
    apply_changes(addressing_probe, {0x02A: b"earlier", 0x100: b"later"})
    verify(addressing_probe[0][0x2A] == b"earlier", "earlier text archive addressing")
    verify(addressing_probe[1][0] == b"later", "later text archive addressing")

    verify_version(
        "Gregar", args.gregar_original, args.gregar_output, args.gregar_symbols, bn5, bn5_colonel, bn4, bn3,
        0x42038, 0x27D50,
        (
            (0x27D2C, 0x2C7C4, 0x42038, 0x42050, 0x476B4, 0x121F54, 0x121FF4, 0x12204C, 0x128194),
            (0x27D30, 0x2C7C8, 0x4203C, 0x476B8, 0x121F58, 0x121FF8, 0x122050, 0x128198),
        ),
        ((0x27D50, 0x11A9BC, 0x11B3BC), (0x27D54, 0x11A9C0, 0x11B3C0)),
        True,
        True,
        0x080C6C16,
        False,
        0x080C6C16,
        0x080E266E,
        0xEACD0,
        0x159F48,
        (0x15049C, 0x1504D0, 0x15051C, 0x150570, 0x1505A4),
    )
    verify_version(
        "Falzar", args.falzar_original, args.falzar_output, args.falzar_symbols, bn5, bn5_colonel, bn4, bn3,
        0x42068, 0x27D50,
        (
            (0x27D2C, 0x2C7C4, 0x42068, 0x42080, 0x476E4, 0x120178, 0x120218, 0x120270, 0x1263B8),
            (0x27D30, 0x2C7C8, 0x4206C, 0x476E8, 0x12017C, 0x12021C, 0x120274, 0x1263BC),
        ),
        ((0x27D50, 0x11968C, 0x11A08C), (0x27D54, 0x119690, 0x11A090)),
        False,
        False,
        0x080C53A6,
        True,
        0x080C53A6,
        0x080E1332,
        0xE9990,
        0x1583F8,
        (0x14E94C, 0x14E980, 0x14E9CC, 0x14EA20, 0x14EA54),
    )


if __name__ == "__main__":
    main()
