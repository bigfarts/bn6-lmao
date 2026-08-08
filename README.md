# bn6 lmao

## Changes

### Title screen

The gold `6` in both title logos now reads `67`. The full-size `7` is rasterized
from SF Pro Display Heavy Italic, extended slightly in height, recolored with
each version's native title palette, given the original `6`'s upper-left/lower-
right bevel lighting, and composited over the original `6`.
Each edition's complete 256x160 title layer and uniform 32x20 tile map are
rebuilt wholesale rather than patching individual native atlas cells.

### AntiNavi

AntiNavi retains its native BN6 behavior, codes, and metadata, but its MB cost
is reduced from 50 MB to 33 MB in both versions.

### RollArrow

Replaces TrainArrow1, TrainArrow2, and TrainArrow3 with Blue Moon's RollArrow
attack. Roll fires one arrow straight down the user's row, and the arrow
destroys opposing chips on hit. The A/F/T, D/R/W, and Q/Y/Z code sets,
50/70/90 power, menu art, Roll actor, and arrow graphics are imported from
Blue Moon in both versions. The original summon and firing sound effects are
also imported intact.

### SearchMan

Replaces CircusMan, CircusMan EX, and CircusMan SP with the complete SearchMan
chip attack ported from Battle Network 5. Its shots retain Cursor element and
destroy traps on contact.

- `SerchMan S/*`: 20 damage per shot
- `SerchMnEX S`: 40 damage per shot
- `SerchMnSP S`: 75 damage per shot

### LaserMan

Replaces HeatMan, HeatMan EX, and HeatMan SP with Blue Moon's LaserMan. The
summoned Navi raises his arms, points forward, and fires Blue Moon's piercing
blue-white laser through the complete row.

- `LaserMan L/*`: 100 damage
- `LaserMnEX L`: 150 damage
- `LaserMnSP L`: 200 damage

Holding a direction while LaserMan raises his arms enables Blue Moon's command
effect on Base, EX, and SP:

- Up resets Attack, Rapid, and Charge to level 1.
- Down disables SuperArmor, AirShoes, FloatShoes, Undershirt, and B+Left abilities.
- Right restores the standard charge shot without overwriting an active Cross charge shot.
- Left permanently reduces the target's Custom Screen selection by one chip,
  to a minimum of two chips.

Command effects require the beam to contact the target. A missed LaserMan does
not alter stats, abilities, charge shots, or Custom behavior.

All three are Null-element MegaChips. Base uses Blue Moon's base art palette,
EX preserves every base LaserMan foreground color while changing only the five
red-background palette entries to green, and SP uses the native yellow-background
Blue Moon SP palette. All three use LaserMan's base battle palette.

### ChaosLrd

Replaces BigHook with the BN5 ChaosLrd attack. Bass, Ball Bass, the fireball,
Nebula Gray apparition, white-flash impact, and teardown sprites are ported from
BN5. The chip is Null element, code S, and has 500 displayed power. Gregar
uses ChaosLrd menu art; Falzar intentionally retains its original slot art.

### Jealousy

Replaces LifeSync with the complete BN5 Jealousy attack. It is a 60 MB,
Null-element StandardChip in code J with 80 displayed power. For each chip
loaded by the opponent, Jealousy produces one 80-damage full-field pulse, then
runs BN5's chip-delete overlay and cleanup sequence. Every pulse fires through
normal collision handling, while the separate deletion phase still runs through
traps exactly as it does in BN5. Its icon, library art, palette, and delete-overlay graphics are imported
from BN5 in both versions.

### BugChain

Replaces CopyDamage with Blue Moon's BugChain. It is a 59 MB, Null-element
StandardChip in codes C/* and keeps CopyDamage's library slot. In link battles,
BugChain waits through Blue Moon's 60-frame time-freeze beat and copies every
active bug on the user to the opposing Navi without removing the user's bugs
or weakening bugs already present on the target. Its icon, library art, and
palette are imported from Blue Moon in both versions, along with the original
50-frame bug aura and SFX `0x15D` displayed and played by both Navis.

### BugCharg

Replaces the former SignalRed/BugRSword slot with BN5 Colonel's BugCharge and
restores BN6 BugFix. BugCharge is a GigaChip: the 77 MB,
Null-element chip uses code B and fires one 200-damage Gospel shot plus one
additional shot for every active battle-bug type it clears. It consumes the
complete BN6 BugFix property set, including the Custom-screen bug, and resets
BugFix's latched runtime bug state. The stationary Gospel uses BN5's 24-pixel
forward offset and remains present through the full bug-scaled firing sequence;
each moving head begins on the front panel with BN5's native collision timing.
Gregar uses the imported BugCharge menu art; Falzar retains the inaccessible
BugRSword-slot art.

### SignlRed

Replaces Navi+20 with the complete Blue Moon SignalRed behavior. The 100-HP
traffic light appears on the panel in front of the user, disables the opposing
side's BattleChip use for 420 red frames, opens a 50-frame green window, and
repeats until destroyed. It is a 61 MB Obstacle-element StandardChip in code
S and keeps Navi+20's library position. Both versions import the Blue Moon menu
art and battle sprite. Its placement cue imports Blue Moon's original sample
and sequence; the green-light cue uses its matching BN6 sound,
and the light is registered as a normal deployable so DustCross can suck it in
with B+Left. Its 100-HP hurtbox remains active every frame, and either player's
DustCross can suck it in. Its native RockCube-class passive collision is enabled
after the cut-in ends, so it does not trigger Beat or traps. Its chip-activation
flags also clear BN6's Image Invis classification bit, so playing it does not
wake Rush. Dimming cut-ins also pause its red/green state transition without
stalling either peer.

### DethPhnx

Replaces CrossDiv with BN5 DeathPhoenix in both versions. The phoenix attacks
with twelve fireballs and then replays the last used Navi chip, matching BN5's
recycle tail. Falzar imports the DeathPhoenix icon, library art, and palette;
Gregar deliberately keeps CrossDiv's original menu art.

### FoldrBak

Replaces the dormant Falzar Giga chip with BN3 FolderBack in both versions.
It restores every used chip to the user's equipped Folder, including
FolderBack itself, clears the current hand, reshuffles through BN6's native
Folder setup, and immediately reopens Custom. A consumed Regular Chip returns
as an ordinary chip rather than regaining its Regular designation. The ending
ports BN3's SFX 0x120 rumble, 0x46-frame shake and alternating white flash,
then shows a full Custom Gauge for 20 frames before the native Custom window
opens with its normal sound. Its wildcard code, 99 MB cost, icon,
center-cropped library art, palette, name, and description come from BN3 Blue.

## Assets and palettes

The build extracts RollArrow's three menu-art variants, Roll actor and
heart-arrow archives, summon and firing samples, BugChain's menu art and aura,
LaserMan's menu art and palettes,
shared actor/beam archive, and SignalRed's menu art and battle sprite from Blue
Moon; Jealousy's menu/overlay graphics and BugCharge's menu and Gospel-head
assets from BN5; FolderBack's BN3 menu art and original rumble PCM; plus the full BN5
SearchMan actor archive, both scope/reticle archives, the chip icon, and the
56x48 library artwork. The SearchMan variant library-art palettes are:

- base: BN5 base palette (blue background)
- EX: unchanged SearchMan foreground with a custom yellow background
- SP: BN5 SP palette (red/pink background)

Base, EX, and SP all select the same in-battle actor palette. The three menu
art palettes remain distinct.

The first 13 nontransparent palette entries used for SearchMan's foreground
are byte-identical in all three variants. Only the three background entries
differ.

See [DISASSEMBLY.md](DISASSEMBLY.md) for the native BN5 object chain and BN6
hook locations.

## Build

Requirements: `armips`, Python 3, and optionally `flips` for BPS output.

Pass the five source ROM paths in BN5, Gregar, Falzar, Blue Moon, BN3 Blue
order:

```sh
./build.sh /path/to/bn5.srl /path/to/bn6-gregar.srl /path/to/bn6-falzar.srl /path/to/bn4-blue-moon.srl /path/to/bn3-blue.srl
```

Supported source SHA-256 hashes:

- BN5 Team ProtoMan: `b35f5890f54784c9d90a896dc5ac4831d43acc9f94e8c42816742fcfa6b41a7b`
- BN6 Gregar: `572e113eeb53bb29cd9ff8acb9db265cfd48c5e509c8d0e6420b58e71e442cf2`
- BN6 Falzar: `a37c1028adb72082b51e142321fa437967bc54b6f46730a53f6581ad455ad670`
- BN4 Blue Moon: `63ea187c792f4bfcd077f92c3a509fa09ed422993aee9480c39dfdf6a561c5c1`
- BN3 Blue: `8c6767788f99dc9e2af0c9d75513b227c7c42d6d452d6165c8e08850af78e273`

Patched ROM copies are written under `build/`. If `flips` is available, BPS
patches are written under `dist/`. If `tango-patch` is also available, the two
BPS payloads are packaged together as `bn67-1.0.0.tangopatch`. The
supplied source ROMs are never modified.

As a final build step, every nonzero alphabetical-sort key in the main chip
table is regenerated from the completed relocated name archives. The pass uses
case-insensitive natural ordering, so renamed chips and numbered or EX/SP
series appear in name order in both Gregar and Falzar. It reassigns the ROM's
original sparse key values without inventing a new numeric format; zero-keyed
special, enemy, and unused records remain untouched.

## Text archives

BN6 splits chip names and descriptions into an earlier 256-entry archive for
IDs `0x000`-`0x0FF` and a later archive beginning at ID `0x100`. The Python
builder reads and relocates all four archives (two names and two descriptions),
addresses replacements by full chip ID, and repoints every known consumer.
This permits readable name or description replacements in either half without
overwriting Jealousy, LaserMan, SearchMan, ChaosLrd, SignalRed, FolderBack, or
untouched entries.
