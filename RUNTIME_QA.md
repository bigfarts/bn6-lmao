# Runtime QA recipes

This file records working emulator setup procedures that took non-trivial
reverse engineering to establish. Read it before writing another runtime
probe for this patch.

## Selecting DeathPhoenix without editing a folder

The IDs are different between the source game and the destination game:

- BN5 DeathPhoenix is chip ID `0x13A`. Chip `0x139` is Phoenix, not
  DeathPhoenix.
- In patched BN6, DeathPhoenix replaces CrossDiv and therefore uses BN6 chip
  slot `0x134`, with packed D-code value `0x0734`.

### BN5 reference run

1. Enter Custom and select the first visible chip normally with `A`.
2. After selection, but before leaving Custom, overwrite the selected hand
   entry with `0x013A`. The player-0 hand block is `0x02034E20`; player 1 is
   at `+0x50`. Its first halfword is the fired-chip count and the selected
   chip IDs begin at `+0x02`, so the first selected ID is
   `*(u16 *)0x02034E22`.
3. Keep writing the ID immediately before and after emulator ticks until
   Custom closes. Do **not** reset or overwrite the fired-chip count.
4. Leave Custom with `START`, then `A`.
5. Press `A` in battle to use the first chip in hand.

The Custom card image may still show the originally selected chip because its
graphics were cached before the hand entry was changed. That is expected; the
runtime action after leaving Custom is the authoritative result.

The verified BN5 reference creates type-1 object `0x28` and twelve type-4
objects `0x89`. A scan after Custom found `0x013A` at `0x0201B336`,
`0x02034E22`, and `0x0203B22A`.

### Patched BN6 run

Changing only the battle hand block at `0x020349C0` is too late: it changes
the HUD/telemetry value but does not rebuild the action metadata. Editing the
folder backing entry is also unreliable because ownership/anti-cheat
validation can replace the injected value with sentinel `0x3785` (observed as
chip ID `0x185`).

The working cache-level substitution is:

1. Use a save matching the ROM version. Do not use a Gregar save with a
   Falzar ROM or vice versa.
2. Enter Custom and select the first visible chip normally with `A`.
3. Trap `0x08029166`, after the original selected-chip validator returns but
   before BN6 derives the remaining action cache fields. When
   `r4 == 0x02033000` (the first selected-chip cache entry), replace `r0` with
   `0x0734` (DeathPhoenix/CrossDiv slot `0x134`, code D).
4. For this QA-only cache substitution, trap ownership validators
   `0x0802A53C` and `0x0802A54E`. If `r0 == 0x0734`, return directly to `lr`
   so the validator preserves the injected value rather than producing the
   sentinel. Do not bypass validation for any other value.
5. Leave Custom with `START`, then `A`.
6. Press `A` in battle to use the first chip in hand.

As in BN5, ignore stale Custom-screen card graphics and judge the selected
chip after Custom closes. The working Gregar run retained queue value
`0x0134`, displayed `DethPhnx150`, and invoked the replacement action.

The temporary reference implementations are
`tango-gamesupport-bn5/examples/deathphoenix_reference_probe.rs` and
`tango-gamesupport-bn6/examples/deathphoenix_port_probe.rs` while those files
exist. This document is the durable recipe after the probes are removed.
