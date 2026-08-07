# Disassembly notes

All addresses are GBA ROM addresses unless identified as file offsets. The
port was translated from the supplied English BN5 ROM rather than from the
unrelated dormant effects originally mistaken for SearchMan in BN6.

## Native BN5 attack chain

The BN5 SearchMan time-freeze wrapper is at `0x080C1458`. It creates type-1
object `0x31`; the object's main routine begins at `0x080C11D8` and its init
begins at `0x080C11F8`. The actor loads sprite group/index `0x08/0x03`.

The actor creates two cooperating native objects:

| Role | BN5 class/id | Main | Init | Spawn helper |
| --- | --- | ---: | ---: | ---: |
| moving scope | type 4 / `0x43` | `0x080E5208` | `0x080E5228` | `0x080E547C` |
| shot collision | type 3 / `0x57` | `0x080D0D78` | `0x080D0D98` | `0x080D0E1E` |

The reticle uses group `0x10`, index `0x23` normally and index `0x22` for its
alternate form. It scans the opponent's panel region, accepts A to lock, and
sets the actor's delete-command byte when B accompanies A or is pressed during
the short window after the lock.

The firing loop at `0x080C136C`-`0x080C13CC` initializes a count of five. Each
shot is spawned when its ten-frame timer reaches seven. The special flag is
set only when both the delete-command byte is nonzero and the remaining-shot
count is one. Therefore the **fifth spawned shot** is the only possible delete
shot. Normal shots pass parameter `25`; the special shot passes `29`.

The hit init maps parameter `25` to hit effect `0` and every other parameter
to hit effect `9`. The collision engine applies the latter only on contact.
The hit object removes collision, handles either contact or the miss visual,
clears/frees its collision data, and frees itself in that same update.

## BN6 hooks and translation

The port installs all three cooperating object classes because BN6 does not
contain a usable SearchMan actor/reticle/hit set.

| Hook | BN6 file offset | Patched target |
| --- | ---: | ---: |
| time-freeze subfamily `0x0E` | `0x02CD94` | `SearchManTimeFreezeSpawn` |
| free type-1 slot `0x31` | `0x003D60` | `SearchManObjectMain` |
| free type-3 slot `0x2C` | `0x003F74` | `SearchManHitMain` |
| free type-4 slot `0x17` | `0x004324` | `SearchManReticleMain` |

Runtime code and imported assets are allocated from file offset `0x800000`
onward in an expanded 16 MiB image; exact addresses are selected by Armips.
The object state machine, timers, reticle movement/input, five-shot loop, and
hit-effect selection follow BN5 state for state. The intentional engine
adaptations are:

- BN5 collision result flags are at collision data `+0x68`; BN6 uses `+0x70`.
- Calls from the expanded ROM are outside Thumb `BL` range and go through a
  long-call macro. The macro restores `r4` before entering BN6 because object
  spawners consume `r4` as an implicit argument.
- The miss visual's type-4 spawner replaces `r5` with the spawned effect. The
  port preserves the original shot-object `r5` across that call so cleanup
  always frees the shot rather than the effect.
- Base, EX, and SP all select actor palette `0`, giving every summoned
  SearchMan the same in-battle colors while leaving menu-art palettes distinct.

The last point is what prevents a missed collision object from surviving time
freeze and damaging something later.

## Imported assets

The following exact BN5 ROM slices are extracted at build time:

| Asset | BN5 file offset | Length |
| --- | ---: | ---: |
| chip icon | `0x7493B8` | `0x80` |
| 56x48 library art | `0x728568` | `0x540` |
| base palette | `0x7343C8` | `0x20` |
| SP palette | `0x7343E8` | `0x20` |
| SearchMan actor archive | `0x254F64` | `0xABFC` |
| alternate reticle archive | `0x358410` | `0x5B8` |
| reticle archive | `0x3589C8` | `0x460` |

All imported assets and runtime code are placed with Armips `.autoregion` in
the expanded ROM rather than assigned fixed output offsets.

ChaosLrd also imports BN5 group `0x14` entries `0x0D` and `0x14`. Entry
`0x0D` includes its header at `0x389E68`; dropping that first word selects
unrelated animation data and causes the visible `MegaBstr` teardown. Entry
`0x14` supplies the native impact sprite/palette sequence. Both are repointed
to complete `.autoregion` archives.

The hit also loads packed value `0x00010401` and calls BN5's type-4 `0x0A`
palette-object wrapper at `0x080E1158`. Its variant-1 state is the four-frame
whole-screen whitening effect. BN6 retains the state machine, but schedules
its native `0x0A` slot only after this time-freeze sequence exits. The port
therefore runs that same palette write/restore state in released slot `0x8D`,
which is updated during time freeze and places the flash on the actual hit.
BN6 splits the rendered scene across palette slots `0x14` and `0x15`; writing
only one leaves either the field or the actors colored. The translated state
writes and restores both slots together, producing four full-screen white
frames and returning every palette before the teardown continues.

The unified sprite-table installer in `sprites.inc` applies these SearchMan
mappings when it rebuilds the complete affected group tables:

- group `0x08`, index `0x11` -> actor archive
- group `0x10`, index `0x14` -> alternate reticle
- group `0x10`, index `0x15` -> normal reticle

BN5 has base and SP library-art palettes, not an EX palette. The patch keeps
the base foreground and uses yellow BGR555 values `0x03FF`, `0x0299`, and
`0x0190` for EX's background; SP uses the native BN5 SP palette.

## Relocated text archives

Names and descriptions each use two archives. Archive 0 has IDs
`0x000`-`0x0FF`; archive 1 begins at `0x100`. `build_text_archives.py` reads
both original halves, applies readable charset-encoded replacements keyed by
full chip ID, emits four complete archives, and `text.inc` repoints every
known archive reference. Verification compares every untouched entry against
the source ROM in both versions.

## Jealousy port

BN5 Jealousy is chip ID `0xD4`. Its family-`0x15`/subfamily-`0x13` wrapper at
`0x080E4540` creates type-4 object `0x36`, whose controller begins at
`0x080E443C`. The controller is invisible; Jealousy has no dedicated battle
sprite archive to import.

The controller scans the opposing side's four unit pointers and preserves the
largest loaded-chip count. Every ten frames it scans all 18 panels, passes the
opposing ownership flag in the panel predicate's required-flags `r2` argument, creates an
80-damage collision object on each valid opposing panel, and decrements that
count once. Its final 90-frame state refreshes the native chip-delete overlay
and runs the original link-battle cleanup calls before entering the generic
type-4 outro.

LifeSync is chip ID `0xBF` in BN6. Its released dispatch/object slots are:

| Hook | BN6 file offset | Patched target |
| --- | ---: | ---: |
| family `0x15`, subfamily `0x07` | `0x02CCD0` | `JealousyTimeFreezeSpawn` |
| type-4 slot `0x5C` | `0x004438` | `JealousyMain` |

BN6 retains direct equivalents of all five generic type-4 lifecycle states and
of Jealousy's side comparison, chip-list lookup, panel predicate, overlay,
gauge, and damage-object helpers. Gregar's damage-object wrapper is at
`0x080C6C16`; Falzar's is at `0x080C53A6`, so that one call is selected in the
version assembly rather than shared as a fixed address.

Jealousy's two time-freeze DMA records copy `0x100` bytes of BN5 overlay tiles
from file offset `0x6FAD2C` to `0x06017940` and its `0x20`-byte palette from
`0x6FAE2C` to BN6's relocated staging buffer at `0x030016F0`. The equivalent
native BN6 transfer tables use this address; BN5's `0x030036F0` overwrites
unrelated BN6 IWRAM. The menu icon (`0x748F38`, `0x80` bytes), library
art (`0x7250E8`, `0x540` bytes), and palette (`0x734188`, `0x20` bytes) are also
relocated with `.autoregion`.

## BugChain port

Blue Moon BugChain is chip ID `0xD3`. Its family-`0x0C`/subfamily-`0x1F`
wrapper at `0x080E6678` creates type-4 controller `0x3F`, whose main begins at
`0x080E65EC`. The chip-specific state waits 60 frames and calls the transfer
routine at `0x080E669A`; that routine duplicates active bug properties from
the user onto the opposing Navi without clearing the source. Blue Moon gates
the effect to link battles. It also creates type-4 object `0x40` on both Navis;
that object's main at `0x080E6724` loads group `0x0C`/index `0x32` and follows
its owner for 50 frames. At timer value 42, `0x080E67A4` plays SFX `0x15D`.

BN6 CopyDamage is chip ID `0xBE`. The port keeps that Standard-chip library
slot and uses the unused family-`0x15`/subfamily-`0x22` dispatch at file offset
`0x02CD3C`. Its controller shares Jealousy's released type-4 slot `0x5C`
through a private tail-word tag. The common BN6 time-freeze lifecycle supplies
the intro, freeze, outro, and cleanup states. BN6 no longer keeps Blue Moon's
battle-kind enum in the same byte: its native link-only paths instead read the
battle configuration flags through `0x0802D246` and test bit `0x08`, which the
port uses for the equivalent gate.

BN6 expanded the battle-property block, so copying Blue Moon's seven raw byte
offsets would move unrelated state. The translated transfer instead covers
the nine byte fields and one halfword field cleared by BN6 BugFix at
`0x080E5D04`: `0x31`, `0x13`, `0x14`, `0x16`, `0x24`, `0x19`, `0x18`,
`0x1A`, `0x63`, and halfword `0x54`. A nonzero source value replaces the
target only when it is larger, preserving a stronger bug already present.

Blue Moon SFX `0x15D` has no exact BN6 match. Its one-track sequence, voice
priority `0x1C`, sample header, and complete `0x4A1`-byte PCM body at
`0x081970A0` are imported into relocated BN6 song-table slot `0x1DF`. Each aura
plays the cue at timer 42, matching `0x080E67A4`.

The exact Blue Moon menu assets are relocated from icon `0x74626C` (`0x80`
bytes), image `0x7315EC` (`0x540` bytes), and palette `0x73F1AC` (`0x20`
bytes). The aura archive is `0x380CA4`-`0x381C30` (`0xF8C` bytes). Because
BN6 has no free sprite pointer, the unified installer relocates the complete
group-`0x08`, `0x0C`, `0x10`, and `0x14` tables after every imported archive
is defined. It appends the aura to group `0x10` at index `0x5C` and applies
all ChaosLrd, SearchMan, LaserMan, RollArrow, BugCharge, SignalRed, and
DeathPhoenix replacements in those same final table images. The original
tables remain untouched, so no later copy can discard or trample an earlier
slot patch.

## SignalRed port

Blue Moon SignalRed is chip ID `0x131`. Its family-`0x15`/subfamily-`0x26`
wrapper at `0x080E8404` creates the short-lived type-4 controller whose effect
begins at `0x080E8448`. That controller calls `0x080DD772` to create the real
persistent traffic light, type 3 / ID `0x81`, controlled by `0x080DD544`.

The native object is placed one panel in front of its owner, has 100 HP, and
loads sprite group/index `0x0C/0x33`. It spends 420 frames in animation 0
(red), with the opposing chip-enable flag cleared, followed by 50 frames in
animation 1 (green), with that flag restored. Blue Moon uses mask `0x08` for
owner 0 and `0x04` for owner 1 and plays sound `0x15C` when green begins. The
cycle repeats until the object is destroyed. The placement cue is Blue Moon
sound `0x0A0`. BN6 sound `0x180` reuses its note stream but not its sample,
priority, or timbre, so SignalRed imports the original Blue Moon song header,
voice, track, and PCM sample into BN6's otherwise-empty sound slot `0x1DA`.
Blue Moon's green cue `0x15C` corresponds to BN6 sound `0x0D1` (the same
sequence with BN6's native volume balance), rather than the unrelated
same-numbered sounds.

BugRSword occupies the same chip ID in BN6. The port retains its released
family route and installs the translated controllers at these hooks:

| Hook | BN6 file offset | Patched target |
| --- | ---: | ---: |
| family `0x15`, subfamily `0x26` | `0x02CD4C` | `SignalRedTimeFreezeSpawn` |
| released type-4 slot `0x84` | `0x0044D8` | `SignalRedTimeFreezeMain` |

The unified sprite installer assigns `SignalRedBattleSprite` to group `0x10`,
index `0x1E` in the relocated table rather than modifying the old table at
`0x03201C`.

BN6 has only two genuinely free type-3 slots, already used by SearchMan and
ChaosLrd. SignalRed therefore shares ChaosLrd's slot `0x2D`: its spawner tags
the otherwise unused object tail word at `+0x78`, and the common slot dispatch
selects the SignalRed controller only for that tag. Untagged ChaosLrd objects
continue through their original controller.

Blue Moon also registers the light in its per-owner deployable list through
`0x0800B230` and unregisters it through `0x0800B272`. Their structurally
identical BN6 counterparts are `0x0800F614` and `0x0800F656`. Retaining that
registration is what exposes the object to DustCross's B+Left suction path;
collision targetability alone is not sufficient.

DustCross's suction sweep at `0x080F10B4` walks all eight deployable-list
entries. For each eligible object, `0x0800F8B0` raises the common removal bit
`0x8000` and the DustCross owner's suction bit (`0x100000` or `0x200000`). On
either owner-specific bit, native deployables call `0x0800F90E` before cleanup;
that helper serializes the object's kind, animation, palette, flip, and position
into DustCross's stored-ammo path. SignalRed uses the otherwise-free four-bit
kind 15 and redirects DustCross's suction/firing sprite tables to a 16-entry
copy whose final entry is SignalRed's group `0x10`, index `0x1E` archive. It
then restores the chip flag, cleans up collision state, unregisters, and frees
the field object. Collision event `0x40000` is a separate timed wind-removal
path handled by `0x0800F8CE`; its 20-frame visibility timer owns object byte
`+0x0B`, so SignalRed leaves that byte clear during normal operation.

The BN6 battle-flag helpers at `0x08001382` and `0x0800138E` retain Blue
Moon's set/clear contract; the battle structure field moved from `+0x64` to
`+0x5C`. The generic time-freeze lifecycle is shared with Jealousy through
`common.inc`, while all SignalRed-specific state remains in `signalred.inc`.

The exact Blue Moon asset slices are:

| Asset | Blue Moon file offset | Length |
| --- | ---: | ---: |
| chip icon | `0x746EEC` | `0x80` |
| 56x48 library art | `0x73A8EC` | `0x540` |
| menu palette | `0x73FAEC` | `0x20` |
| traffic-light battle archive | `0x381C30` | `0x694` |

Both versions use the imported battle archive. Gregar repoints BugRSword's
three menu-art fields to the imported assets; Falzar preserves all three
original pointers.

## DeathPhoenix port

BN5 DeathPhoenix's time-freeze wrapper creates type-1 object `0x28`, whose
controller at `0x080BFD98` builds three shuffled panel rows and schedules
twelve strikes. Each strike creates type-4 object `0x89` through the wrapper at
`0x080EA71C`. Its main function is `0x080EA5E4`; the adjacent type-4 `0x8A`
main at `0x080EA740` is unrelated MegaBuster behavior and must not be ported.

Type-4 `0x89` creates direct-damage contacts at animation frames 0 and 16. It
also calls `0x080E8BF4` every four frames to create the visible type-4 `0x71`
flame actor. Type-4 `0x71` is implemented at `0x080E8ACC`, loads sprite group
`0x10`/index `0x49`, and supplies the moving purple fireball animation. That
archive occupies BN5 ROM `0x36F074`-`0x36F7BC`; group `0x10`/index `0x48` at
`0x36E908` is a different vertical-column effect.

The port translates type-4 `0x89` and `0x71` into released BN6 type-4 slots
`0x81` and `0x82`. Both use the imported archive through released BN6 sprite
group `0x14`/index `0x17`. The damage contacts remain separate native BN6
objects, matching BN5's split between collision and visible flame actors.

After the twelfth strike and the phoenix's disappear phase, the controller
checks BN6's saved-Navi record at `0x0203C960`. The record's backing pointer is
validated in addition to its ID because BN6 leaves an unused record zeroed,
whereas BN5 initializes the ID to `0xFF`. If a Navi was used previously, the
port invokes it through the saved-Navi dispatch table at `0x0802CD5C`, waits
for it to finish, and preserves BN5's 30-frame post-Navi pause.

BN5 then performs its own mode-0 return transition and waits another 30
frames. BN6's outer time-freeze controller automatically performs the same
return transition after the completion byte is released. The port therefore
releases completion immediately after the post-Navi pause and lets that outer
controller own the transition. Retaining BN5's cleanup wait or explicitly
starting a second mode-0 transition here causes the long extra pause and the
double MegaMan spawn-out/spawn-in sequence.

DeathPhoenix is installed at chip ID `0x134` in both versions. Falzar repoints
the three menu-art fields to BN5's assets; Gregar leaves those fields
byte-for-byte equal to the original CrossDiv record.

## Verification

`verify.py` checks both versions for the expected dispatch hooks, sprite
pointers, chip records/codes, behavior parameters, text, exact imported asset
slices, palette relationships, and absence of unexpected writes in the
original 8 MiB region.

The exact emulator procedure for selecting BN5 DeathPhoenix (`0x13A`) and
the patched BN6 replacement slot (`0x134`) without editing a folder is kept in
[`RUNTIME_QA.md`](RUNTIME_QA.md). It also records the Custom cache hook,
ownership-validator exception, matching-save requirement, and stale card-art
caveat so this setup does not need to be rediscovered.

The SignalRed runtime probe uses a deterministic clear field, with no rock on
the spawn panel. In both versions it observes the light on the panel directly
in front of the user, flag `0x08` cleared throughout red, restored during
green, an opponent Cannon held during red and released only after green opens,
then the flag cleared again when the cycle returns to red.

The emulator collision probe additionally exercises both contact and miss
paths. On contact it observes five one-update shot objects, effect `0` on
shots one through four, effect `9` only on shot five, one delete only after
that fifth contact, and a zero counter timer. On a miss, all five collision
results stay zero, neither HP nor delete state changes, and every shot still
receives exactly one update in both Gregar and Falzar.

## RollArrow port

Blue Moon runtime tracing identifies the actual Roll actor at spawn routine
`0x080C8110`, main `0x080C812C`, and init `0x080C814C`. Its sprite-loader
calls at `0x080C815C`/`0x080C8160` select group `0x08`, index `0x01`; the
corresponding uncompressed archive is at ROM `0x2A5A10` and is `0xAC58` bytes.
Roll fires immediately in animation `7` and holds the shot for six frames. Her
projectile is the generic type-3 object spawned through `0x080CE81A`; its init loader calls at
`0x080CE724`/`0x080CE728` select group `0x0C`, index `0x10`. The uncompressed
heart-arrow archive is at ROM `0x35E5C0` and is `0x160` bytes. The native fire
routine starts that arrow at Roll's bow origin: eight pixels forward in X,
one pixel above her actor Y origin, and 36 pixels in Z. After firing, Roll stays
on her panel for the native animation-0 and animation-4 holds, then disappears;
there is no horizontal retreat.

The port keeps the heart-arrow in type-3, matching Blue Moon and BN6's
counter-hit ownership path. It shares SearchMan's otherwise-free type-3 slot
`0x2C` through the `RARW` tag. Hosting the collision-bearing arrow in a
type-1 slot works for ordinary damage but corrupts the native counter response.

The earlier `0x080C8644` group-`0x08`/index-`0x0D` trace was KendoMan, and
the paired group-`0x0C`/index-`0x35` object was unrelated. Neither is used by
this port.

The BN6 port keeps TrainArrow's IDs `0x18`-`0x1A`, routes them through the
already released Navi-family dispatch used by SearchMan, and distinguishes the
three IDs in that shared entry. Released type-1 slots `0x53` and `0x59` host
Roll and the straight-moving arrow. The arrow uses BN6's native collision
lifecycle with BN4's `8/5/3` setup and hit-effect `9`, so it travels at seven
pixels per frame, stops on the first real contact, and invokes BN6's built-in
loaded-chip deletion response rather than manufacturing a damage hit on every
panel. Relocated group `0x0C` sprite slots `0x5A` and `0x5B` point at the
runtime-confirmed Blue Moon Roll and heart-arrow archives. The record codes,
MB values, power, icons, image, and palettes are copied from Blue Moon.

Blue Moon's summon SFX `0xB0` and fire SFX `0x132` are imported with their
original tracks, voicegroups, and PCM samples. The BN6 song table is relocated
through native entry `0x1D9`; SignalRed remains at `0x1DA`, and RollArrow uses
new entries `0x1DB`/`0x1DC`.

## LaserMan port

Blue Moon's LaserMan chip creates type-1 object `0x4D` through the wrapper at
`0x080CABC4`. Its main, init, and update routines are at `0x080CABF0`,
`0x080CAC34`, and `0x080CAC96`. Runtime tracing of the real chip confirms that
the actor loads sprite group `0x08`, index `0x16`, plays summon SFX `0xB0`, and
uses animations `0`, `2`, `3`, and `4` for its idle, raised-arms, firing, and
recovery poses.

The firing state creates Blue Moon type-4 object `0xA0` through
`0x080E0FE6` and plays SFX `0x103`. Its init at `0x080E0E3C` loads that same
group-`0x08`/index-`0x16` archive and uses animations `17`, `18`, and `19` for
the thin lead-in, full blue-white row beam, and white tail. The shared archive
is LZ77-compressed at ROM offset `0x339B6C`; the stream occupies `0x395C`
bytes and expands to `0xAAB0` bytes.

The actor's command parser at `0x080CAD1C` reads the held-key halfword and
tests Up (`0x40`), Down (`0x80`), Right (`0x10`), then Left (`0x20`). It stores
command IDs `1`-`4` in that priority order. The beam dispatch table at
`0x080E0F58` selects the following halfword streams: Up
`5,6,7,FD`; Down `1,2,3,4,FF0C,FD`; Right `010A,FD`; and Left `FE,FD`.
`FD` is the normal damage hit. `FE` dynamically packs the target's decremented
Custom Screen count with property ID `0x12` and clamps the count at two. The
port implements that operation with BN6 Custom Level property `0x0A`, reading
it through the per-side accessor at `0x080136CC` and writing it through the
setter at `0x080136B0`. The remaining Blue Moon property IDs translate to BN6
as follows: `5,6,7` zero Attack, Rapid, and Charge (`1,2,3`); `1,2,3,4` clear
SuperArmor, FloatShoes, AirShoes, and UnderShirt (`23,1B,1C,1D`); `FF0C`
restores B-Left to `FF`; and `010A` restores the B button and power attack to
the default values `0` and `1`. Internal Buster levels are zero-based, so the
three zeroes produced by Up are the displayed level 1 values.
After choosing palette 0 for normal/Down and palette 10 for the other commands,
the source calls sprite-property setter `0x08002F22` with table values
`0, B060, A80A, 0, B9C0`. BN6's structural setter is `0x08002ED0`; the port
preserves all five values so Up, Down, and Left retain their distinct beam
transforms rather than collapsing to the yellow palette-only appearance.

The BN6 port retains HeatMan's IDs `0xE3`-`0xE5`, family `0x1B`, and subfamily
`0x02`, replacing that family's dispatch at `0x0802CD64`. Released type-1 slot
`0x30` at `0x08003D5C` hosts the actor and visible beam. Both use foreground
OAM priority 1, which places the beam in front of targets and field objects;
the earlier-allocated actor still wins their same-priority overlap at the
muzzle. LaserMan's row-hit objects share SearchMan's working type-3 slot
`0x2C` through the `LHIT` tag. Blue Moon's collision region `0x0B` is not a
compatible attack mask in BN6, so the port seeds each hit with BN6's proven
normal attack region 25 before using the native collision helpers. The final
`FD` event uses SearchMan's exact working region-25 collision initialization
but LaserMan's quiet cleanup, avoiding the
six random miss-impact sparkles that SearchMan's own cleanup would create for
the six panel hits. Command events are applied directly to the opposing
NaviStats block rather than being misinterpreted as incompatible BN6 extended
collision effects. Thus no direction has no extra effect, while a held
direction applies only its documented stat or Custom Window change. BN6's
collision presenter also consumes the region in
`r1`, so the hit reloads 25 after decoding `FD`/command effects rather than
accidentally presenting the event word as the collision region. Relocated
sprite group `0x0C`/index `0x56` points at the imported compressed shared archive.
BN6's compressed-sprite preloader takes separate group/index arguments instead
of BN4's packed selector, and its reused object tails require the beam Z word
to be cleared explicitly. Translating both details is required for the native
actor and laser to render in game.

Blue Moon creates the laser one panel in front of the actor and then offsets
its sprite origin another 64 pixels in the owning side's direction. The port
reproduces both operations, which keeps the beam emitter aligned with
LaserMan's hand. It also samples and latches commands during the raised-arms
pose, and now takes an initial sample when the time-freeze summon is created so
a direction held with the chip-use input is not lost during the cut-in delay.
Blue Moon gated that parser away from Base; the BN6 port intentionally enables
it for Base, EX, and SP. After the command stream completes, the port updates
BN6's cached power-attack and B-Left IDs and clears the corresponding live
FloatShoes, AirShoes, UnderShirt, and SuperArmor status bits before the damage
event. Effect events retain the original six-frame
cadence. Because the imported full-width beam is stationary in the BN6 object
model, the sole `FD` damage event is represented on all six panels; only one
six-object damage event is alive at a time.

Blue Moon provides Base, SP, and DS chip records. As with SearchMan, the BN6
series maps those behavior selectors to Base/EX/SP parameters `0/3/4`. Base
uses the native red-background palette. The LaserMan artwork assigns its
variant background to palette indices `1`-`5`, so EX replaces only those five
entries with green shades and retains the base foreground at indices `6`-`15`.
SP uses the native yellow-background palette. The in-battle actor always
selects palette 0. Base is available in `L` and `*`, while EX and SP remain
`L`-only. HeatMan's version-specific alphabetical and library-order metadata
remains byte-for-byte intact in both Gregar and Falzar. The relocated sound
table reuses imported BN4 summon SFX `0xB0` and adds the exact BN4 SFX `0x103`
track, voicegroup, and PCM sample for the firing cue.
