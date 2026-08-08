#!/bin/sh
set -eu

PATCH_DIR=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$PATCH_DIR/build"
DIST_DIR="$PATCH_DIR/dist"
TANGOPATCH_SRC="$PATCH_DIR/tangopatch"

if [ "$#" -ne 5 ] && [ "$#" -ne 6 ]; then
    echo "usage: $0 BN5_PROTOMAN_ROM BN6_GREGAR_ROM BN6_FALZAR_ROM BN4_BLUE_MOON_ROM BN3_BLUE_ROM [BN5_COLONEL_ROM]" >&2
    exit 2
fi

BN5_ROM=$1
BN6_GREGAR_ROM=$2
BN6_FALZAR_ROM=$3
BN4_BLUE_MOON_ROM=$4
BN3_BLUE_ROM=$5
if [ "$#" -eq 6 ]; then
    BN5_COLONEL_ROM=$6
else
    BN5_COLONEL_ROM=$(dirname -- "$BN5_ROM")/exe5k_rom_k_e.srl
fi

ARMIPS_BIN=${ARMIPS:-$(command -v armips || true)}
FLIPS_BIN=${FLIPS:-$(command -v flips || true)}
TANGO_PATCH_BIN=${TANGO_PATCH:-$(command -v tango-patch || true)}

if [ -z "$ARMIPS_BIN" ]; then
    echo "armips was not found. Set ARMIPS=/absolute/path/to/armips." >&2
    exit 1
fi

check_sha256() {
    path=$1
    expected=$2
    actual=$(shasum -a 256 "$path" | awk '{print $1}')
    if [ "$actual" != "$expected" ]; then
        echo "Unsupported ROM: $path" >&2
        echo "Expected SHA-256: $expected" >&2
        echo "Actual SHA-256:   $actual" >&2
        exit 1
    fi
}

check_sha256 "$BN5_ROM" b35f5890f54784c9d90a896dc5ac4831d43acc9f94e8c42816742fcfa6b41a7b
check_sha256 "$BN5_COLONEL_ROM" d4b7aefc3918c9f801c84cfd1322c2cdbb9d13c2e3271b3c3f8f9927480f2633
check_sha256 "$BN6_GREGAR_ROM" 572e113eeb53bb29cd9ff8acb9db265cfd48c5e509c8d0e6420b58e71e442cf2
check_sha256 "$BN6_FALZAR_ROM" a37c1028adb72082b51e142321fa437967bc54b6f46730a53f6581ad455ad670
check_sha256 "$BN4_BLUE_MOON_ROM" 63ea187c792f4bfcd077f92c3a509fa09ed422993aee9480c39dfdf6a561c5c1
check_sha256 "$BN3_BLUE_ROM" 8c6767788f99dc9e2af0c9d75513b227c7c42d6d452d6165c8e08850af78e273

mkdir -p "$BUILD_DIR" "$DIST_DIR"

# Rebuild each complete 256x160 title layer and its uniform 32x20 tile map,
# with the full-height font-rasterized 7 composited over the native artwork.
# The two editions retain their own artwork and palettes, but no longer rely
# on their different native atlas layouts.
python3 "$PATCH_DIR/build_title_screen.py" \
    gregar "$BN6_GREGAR_ROM" "$BUILD_DIR/title-67-gregar.bin" \
    "$BUILD_DIR/title-map-gregar.bin"
python3 "$PATCH_DIR/build_title_screen.py" \
    falzar "$BN6_FALZAR_ROM" "$BUILD_DIR/title-67-falzar.bin" \
    "$BUILD_DIR/title-map-falzar.bin"

# FolderBack replaces BN6's dormant Falzar Giga slot. BN3 uses a 64x56 chip
# image while BN6 expects 56x48, so crop the original image by four pixels on
# every edge and preserve its native icon and palette exactly.
python3 "$PATCH_DIR/extract_folderback_art.py" \
    "$BN3_BLUE_ROM" \
    "$BUILD_DIR/folderback-icon.bin" "$BUILD_DIR/folderback-image.bin" \
    "$BUILD_DIR/folderback-palette.bin"
# BN3 Blue SFX 0x120's sample header plus its complete 0x353E-byte PCM body.
dd if="$BN3_BLUE_ROM" of="$BUILD_DIR/folderback-rumble-sample.bin" bs=1 skip=$((0x215B68)) count=$((0x354E)) 2>/dev/null

# Jealousy replaces LifeSync. Its attack is an invisible controller, so the
# only imported graphics are its menu art and the small BN5 chip-delete overlay
# copied into VRAM during the time-freeze sequence.
dd if="$BN5_ROM" of="$BUILD_DIR/jealousy-icon.bin" bs=1 skip=$((0x748F38)) count=$((0x80)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/jealousy-image.bin" bs=1 skip=$((0x7250E8)) count=$((0x540)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/jealousy-palette.bin" bs=1 skip=$((0x734188)) count=$((0x20)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/jealousy-effect-tiles.bin" bs=1 skip=$((0x6FAD2C)) count=$((0x100)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/jealousy-effect-palette.bin" bs=1 skip=$((0x6FAE2C)) count=$((0x20)) 2>/dev/null

# Extract only the SearchMan chip assets needed by armips. No source ROM data
# is stored in this patch directory.
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-icon.bin" bs=1 skip=$((0x7493B8)) count=$((0x80)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-image.bin" bs=1 skip=$((0x728568)) count=$((0x540)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-pal-base.bin" bs=1 skip=$((0x7343C8)) count=$((0x20)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-pal-sp.bin" bs=1 skip=$((0x7343E8)) count=$((0x20)) 2>/dev/null
# Complete SearchMan Navi archive plus both scope/reticle archives. The old
# 0x3051F0 extraction was an unrelated 0x910-byte battle effect.
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-battle-sprite.bin" bs=1 skip=$((0x254F64)) count=$((0xABFC)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-reticle-alt.bin" bs=1 skip=$((0x358410)) count=$((0x5B8)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/searchman-reticle.bin" bs=1 skip=$((0x3589C8)) count=$((0x460)) 2>/dev/null

# Dormant ChaosLord menu art and every battle sprite archive selected by its
# BN5 controller: Bass and Ball Bass, the foreground apparition, the shared
# Nebula Gray aura/burst archive, the final teardown effect, and the
# entrance-burst trigonometry table.
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-icon.bin" bs=1 skip=$((0x749C38)) count=$((0x80)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-image.bin" bs=1 skip=$((0x72FE28)) count=$((0x540)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-palette.bin" bs=1 skip=$((0x734AE8)) count=$((0x20)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-bass-sprite.bin" bs=1 skip=$((0x2D3304)) count=$((0x1081C)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-apparition-sprite.bin" bs=1 skip=$((0x398024)) count=$((0x186C)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-aura-sprite.bin" bs=1 skip=$((0x2E3B20)) count=$((0x56E0)) 2>/dev/null
# Native selector 0x12 resolves to group 0x14/index 0x0D. The word at
# 0x389E68 is the archive header (not an outer pointer), so it must be kept;
# skipping it makes BN6 interpret animation offsets as a sprite header and
# produces the unrelated "MegaBstr" teardown.
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-teardown-sprite.bin" bs=1 skip=$((0x389E68)) count=$((0x11F0)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/chaoslord-trig.bin" bs=1 skip=$((0x5CD0)) count=$((0x280)) 2>/dev/null

# SignalRed replaces Navi+20 in both versions. Its targetable traffic-light
# archive is group 0x0C/index 0x33 in Blue Moon, and both versions use its
# imported menu art now that it occupies a shared Standard-chip slot.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/signalred-icon.bin" bs=1 skip=$((0x746EEC)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/signalred-image.bin" bs=1 skip=$((0x73A8EC)) count=$((0x540)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/signalred-palette.bin" bs=1 skip=$((0x73FAEC)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/signalred-battle-sprite.bin" bs=1 skip=$((0x381C30)) count=$((0x694)) 2>/dev/null
# Blue Moon SFX 0x00A0's sample header plus its complete 0x881-byte PCM body.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/signalred-spawn-sample.bin" bs=1 skip=$((0x17C834)) count=$((0x891)) 2>/dev/null

# BugChain replaces CopyDamage. Import its menu art and the group-0x0C/index
# 0x32 aura archive which Blue Moon attaches to both Navis during time freeze.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/bugchain-icon.bin" bs=1 skip=$((0x74626C)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/bugchain-image.bin" bs=1 skip=$((0x7315EC)) count=$((0x540)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/bugchain-palette.bin" bs=1 skip=$((0x73F1AC)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/bugchain-battle-sprite.bin" bs=1 skip=$((0x380CA4)) count=$((0xF8C)) 2>/dev/null
# Blue Moon SFX 0x015D's sample header plus its complete 0x4A1-byte PCM body.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/bugchain-sound-sample.bin" bs=1 skip=$((0x1970A0)) count=$((0x4B1)) 2>/dev/null
# Preserve every native sprite table that receives appended imported archives.
for version in gregar falzar; do
    if [ "$version" = gregar ]; then
        sprite_rom=$BN6_GREGAR_ROM
    else
        sprite_rom=$BN6_FALZAR_ROM
    fi
    dd if="$sprite_rom" of="$BUILD_DIR/sprite-group08-table-$version.bin" bs=1 skip=$((0x31DA4)) count=$((0x5C)) 2>/dev/null
    dd if="$sprite_rom" of="$BUILD_DIR/sprite-group0C-table-$version.bin" bs=1 skip=$((0x31E00)) count=$((0x1A4)) 2>/dev/null
    dd if="$sprite_rom" of="$BUILD_DIR/sprite-group10-table-$version.bin" bs=1 skip=$((0x31FA4)) count=$((0x170)) 2>/dev/null
    dd if="$sprite_rom" of="$BUILD_DIR/sprite-group14-table-$version.bin" bs=1 skip=$((0x32114)) count=$((0x80)) 2>/dev/null
done

# BugCharge replaces SignalRed in BugRSword's Gregar Giga slot, leaving BugFix
# native. BugCharge is Colonel-exclusive, so source its real menu art from the
# Team Colonel record. Runtime tracing of the real chip confirms that both its
# stationary charge apparition and its moving shot load group 0x0C/index 0x43.
dd if="$BN5_COLONEL_ROM" of="$BUILD_DIR/bugcharge-icon.bin" bs=1 skip=$((0x74AE3C)) count=$((0x80)) 2>/dev/null
dd if="$BN5_COLONEL_ROM" of="$BUILD_DIR/bugcharge-image.bin" bs=1 skip=$((0x730664)) count=$((0x540)) 2>/dev/null
dd if="$BN5_COLONEL_ROM" of="$BUILD_DIR/bugcharge-palette.bin" bs=1 skip=$((0x735D64)) count=$((0x20)) 2>/dev/null
dd if="$BN5_COLONEL_ROM" of="$BUILD_DIR/bugcharge-gospel-sprite.bin" bs=1 skip=$((0x3216D4)) count=$((0xA84)) 2>/dev/null
dd if="$BN5_COLONEL_ROM" of="$BUILD_DIR/bugcharge-charge-sample.bin" bs=1 skip=$((0x191A80)) count=$((0x676)) 2>/dev/null

# RollArrow1/2/3 replace TrainArrow1/2/3 in both versions. Runtime tracing in
# Blue Moon identifies Roll as group 0x08/index 0x01 and the heart-arrow as
# group 0x0C/index 0x10. Both archives are stored uncompressed in the ROM.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-icon-1.bin" bs=1 skip=$((0x74476C)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-icon-2.bin" bs=1 skip=$((0x7447EC)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-icon-3.bin" bs=1 skip=$((0x74486C)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-image.bin" bs=1 skip=$((0x729D2C)) count=$((0x540)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-pal-1.bin" bs=1 skip=$((0x73EAEC)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-pal-2.bin" bs=1 skip=$((0x73EB0C)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-pal-3.bin" bs=1 skip=$((0x73EB2C)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-actor-sprite.bin" bs=1 skip=$((0x2A5A10)) count=$((0xAC58)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-projectile-sprite.bin" bs=1 skip=$((0x35E5C0)) count=$((0x160)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-summon-sample.bin" bs=1 skip=$((0x184D70)) count=$((0xF3E)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/rollarrow-fire-sample.bin" bs=1 skip=$((0x1D2AFC)) count=$((0xC34)) 2>/dev/null

# LaserMan replaces HeatMan/EX/SP. Blue Moon stores one shared Navi-chip icon
# and image, separate Base/SP palettes, and one compressed archive containing
# both the actor and the full-width row laser. Runtime tracing identifies it as
# group 0x08/index 0x16 (not the adjacent WoodMan or bomb archives).
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-icon.bin" bs=1 skip=$((0x74676C)) count=$((0x80)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-image.bin" bs=1 skip=$((0x73842C)) count=$((0x540)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-pal-base.bin" bs=1 skip=$((0x73F94C)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-pal-sp.bin" bs=1 skip=$((0x73F96C)) count=$((0x20)) 2>/dev/null
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-battle-sprite.bin" bs=1 skip=$((0x339B6C)) count=$((0x395C)) 2>/dev/null
# Exact Blue Moon SFX 0x103 sample header plus its complete PCM body.
dd if="$BN4_BLUE_MOON_ROM" of="$BUILD_DIR/laserman-fire-sample.bin" bs=1 skip=$((0x1BCFF8)) count=$((0x144E)) 2>/dev/null

# Relocate BN6's complete song table so all imported cues can be appended
# without overwriting native SFX slots.
dd if="$BN6_GREGAR_ROM" of="$BUILD_DIR/song-table-gregar.bin" bs=1 skip=$((0x159F48)) count=$((0xED0)) 2>/dev/null
dd if="$BN6_FALZAR_ROM" of="$BUILD_DIR/song-table-falzar.bin" bs=1 skip=$((0x1583F8)) count=$((0xED0)) 2>/dev/null

# DeathPhoenix replaces CrossDiv in both versions. Its complete Navi archive
# is group 0x0C/index 0x57 in BN5. Each actual DeathPhoenix strike is BN5
# type-4 0x89 and uses the separate group 0x10/index 0x49 archive. (The
# adjacent type-4 0x8A controller is the unrelated MegaBuster-driven object.)
dd if="$BN5_ROM" of="$BUILD_DIR/deathphoenix-icon.bin" bs=1 skip=$((0x749CB8)) count=$((0x80)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/deathphoenix-image.bin" bs=1 skip=$((0x730368)) count=$((0x540)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/deathphoenix-palette.bin" bs=1 skip=$((0x734B08)) count=$((0x20)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/deathphoenix-battle-sprite.bin" bs=1 skip=$((0x333400)) count=$((0x20F4)) 2>/dev/null
dd if="$BN5_ROM" of="$BUILD_DIR/deathphoenix-strike-sprite.bin" bs=1 skip=$((0x36F074)) count=$((0x748)) 2>/dev/null
python3 "$PATCH_DIR/build_text_archives.py" \
    "$BN6_GREGAR_ROM" 0x42038 0x27D50 \
    "$BUILD_DIR/chip-names-0-gregar.bin" "$BUILD_DIR/chip-names-1-gregar.bin" \
    "$BUILD_DIR/chip-descriptions-0-gregar.bin" "$BUILD_DIR/chip-descriptions-1-gregar.bin"
python3 "$PATCH_DIR/build_text_archives.py" \
    "$BN6_FALZAR_ROM" 0x42068 0x27D50 \
    "$BUILD_DIR/chip-names-0-falzar.bin" "$BUILD_DIR/chip-names-1-falzar.bin" \
    "$BUILD_DIR/chip-descriptions-0-falzar.bin" "$BUILD_DIR/chip-descriptions-1-falzar.bin"

cp "$BN6_GREGAR_ROM" "$BUILD_DIR/exe6_rom_e.patched.srl"
cp "$BN6_FALZAR_ROM" "$BUILD_DIR/exe6f_rom_f_e.patched.srl"

cp "$BUILD_DIR/chip-names-0-gregar.bin" "$BUILD_DIR/chip-names-0.bin"
cp "$BUILD_DIR/chip-names-1-gregar.bin" "$BUILD_DIR/chip-names-1.bin"
cp "$BUILD_DIR/chip-descriptions-0-gregar.bin" "$BUILD_DIR/chip-descriptions-0.bin"
cp "$BUILD_DIR/chip-descriptions-1-gregar.bin" "$BUILD_DIR/chip-descriptions-1.bin"
cp "$BUILD_DIR/song-table-gregar.bin" "$BUILD_DIR/song-table.bin"
cp "$BUILD_DIR/sprite-group08-table-gregar.bin" "$BUILD_DIR/sprite-group08-table.bin"
cp "$BUILD_DIR/sprite-group0C-table-gregar.bin" "$BUILD_DIR/sprite-group0C-table.bin"
cp "$BUILD_DIR/sprite-group10-table-gregar.bin" "$BUILD_DIR/sprite-group10-table.bin"
cp "$BUILD_DIR/sprite-group14-table-gregar.bin" "$BUILD_DIR/sprite-group14-table.bin"
"$ARMIPS_BIN" -root "$PATCH_DIR" -erroronwarning -sym "$BUILD_DIR/gregar.sym" gregar.asm
python3 "$PATCH_DIR/reorder_chip_sort.py" \
    "$BUILD_DIR/exe6_rom_e.patched.srl" "$BUILD_DIR/gregar.sym" "$BN6_GREGAR_ROM"
cp "$BUILD_DIR/chip-names-0-falzar.bin" "$BUILD_DIR/chip-names-0.bin"
cp "$BUILD_DIR/chip-names-1-falzar.bin" "$BUILD_DIR/chip-names-1.bin"
cp "$BUILD_DIR/chip-descriptions-0-falzar.bin" "$BUILD_DIR/chip-descriptions-0.bin"
cp "$BUILD_DIR/chip-descriptions-1-falzar.bin" "$BUILD_DIR/chip-descriptions-1.bin"
cp "$BUILD_DIR/song-table-falzar.bin" "$BUILD_DIR/song-table.bin"
cp "$BUILD_DIR/sprite-group08-table-falzar.bin" "$BUILD_DIR/sprite-group08-table.bin"
cp "$BUILD_DIR/sprite-group0C-table-falzar.bin" "$BUILD_DIR/sprite-group0C-table.bin"
cp "$BUILD_DIR/sprite-group10-table-falzar.bin" "$BUILD_DIR/sprite-group10-table.bin"
cp "$BUILD_DIR/sprite-group14-table-falzar.bin" "$BUILD_DIR/sprite-group14-table.bin"
"$ARMIPS_BIN" -root "$PATCH_DIR" -erroronwarning -sym "$BUILD_DIR/falzar.sym" falzar.asm
python3 "$PATCH_DIR/reorder_chip_sort.py" \
    "$BUILD_DIR/exe6f_rom_f_e.patched.srl" "$BUILD_DIR/falzar.sym" "$BN6_FALZAR_ROM"

python3 "$PATCH_DIR/verify.py" \
    "$BN5_ROM" "$BN5_COLONEL_ROM" "$BN4_BLUE_MOON_ROM" "$BN3_BLUE_ROM" \
    "$BN6_GREGAR_ROM" "$BUILD_DIR/exe6_rom_e.patched.srl" "$BUILD_DIR/gregar.sym" \
    "$BN6_FALZAR_ROM" "$BUILD_DIR/exe6f_rom_f_e.patched.srl" "$BUILD_DIR/falzar.sym"

if [ -n "$FLIPS_BIN" ]; then
    "$FLIPS_BIN" --create --bps "$BN6_GREGAR_ROM" "$BUILD_DIR/exe6_rom_e.patched.srl" "$DIST_DIR/bn67-gregar.bps"
    "$FLIPS_BIN" --create --bps "$BN6_FALZAR_ROM" "$BUILD_DIR/exe6f_rom_f_e.patched.srl" "$DIST_DIR/bn67-falzar.bps"
    echo "BPS patches written to $DIST_DIR"

    if [ -n "$TANGO_PATCH_BIN" ]; then
        mkdir -p "$TANGOPATCH_SRC/roms"
        cp "$DIST_DIR/bn67-gregar.bps" "$TANGOPATCH_SRC/roms/BR5E_00.bps"
        cp "$DIST_DIR/bn67-falzar.bps" "$TANGOPATCH_SRC/roms/BR6E_00.bps"
        "$TANGO_PATCH_BIN" validate "$TANGOPATCH_SRC"
        "$TANGO_PATCH_BIN" pack --out "$DIST_DIR" "$TANGOPATCH_SRC"
    else
        echo "tango-patch was not found; .tangopatch packaging was skipped." >&2
    fi
else
    echo "flips was not found; BPS and .tangopatch packaging were skipped." >&2
fi

echo "Patched ROMs written to $BUILD_DIR"
