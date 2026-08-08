# bn6 lmao

## RollArrow

Replaces TrainArrow1/2/3 with Blue Moon RollArrow1/2/3 in both versions.
Roll fires one arrow straight down the user's row and destroys opposing chips
on hit, using the original codes, 50/70/90 power, chip art, actor, and arrow
graphics.

## SearchMan

Replaces CircusMan, CircusMan EX, and CircusMan SP with the complete SearchMan
chip attack ported from Battle Network 5. Its Cursor-element shots destroy
traps on contact.

- `SerchMan S/*`: 20 damage per shot
- `SerchMnEX S`: 40 damage per shot
- `SerchMnSP S`: 75 damage per shot

## LaserMan

Replaces HeatMan, HeatMan EX, and HeatMan SP with Blue Moon LaserMan in both
versions. LaserMan raises his arms, points forward, and fires the original
piercing blue-white laser through the complete row.

- `LaserMan L/*`: 100 damage
- `LaserMnEX L`: 150 damage
- `LaserMnSP L`: 200 damage

Hold a direction while LaserMan raises his arms to add the original command
effect. This works for Base, EX, and SP: Up resets Attack/Rapid/Charge to level
1; Down disables SuperArmor, AirShoes, FloatShoes, Undershirt, and B+Left
abilities; Right restores the standard charge shot without overwriting an
active Cross charge shot; Left applies Custom bug 1. Command effects require
the beam to hit the target, so a miss does not alter the target.

Base uses the native red-background menu palette. EX keeps the base LaserMan
foreground and changes only its five background entries to green. SP uses the
native yellow-background SP palette. All variants use the base battle palette.

## ChaosLrd

Replaces BigHook with the BN5 ChaosLrd attack. The chip is Null element, code
S, and has 500 displayed power. Gregar uses ChaosLrd chip art while Falzar
keeps the original art for that slot.

## Jealousy

Replaces LifeSync with BN5 Jealousy. The chip is a 60 MB, Null-element
StandardChip in code J with 80 displayed power. It deals 80 damage for each
chip loaded by the opponent and then deletes those loaded chips. Both versions
use Jealousy's BN5 icon, library art, palette, and chip-delete overlay graphics.

## BugChain

Replaces CopyDamage with Blue Moon BugChain. The 59 MB, Null-element
StandardChip uses codes C/* and keeps CopyDamage's library slot. In link
battles it copies every active bug on the user to the opposing Navi without
removing the user's bugs or weakening existing target bugs. Both versions use
BugChain's Blue Moon icon, library art, palette, battle aura, and sound effect.

## BugCharg

Replaces the former SignalRed/BugRSword slot with BN5 BugCharge and restores
BN6 BugFix. BugCharge is a 77 MB, Null-element GigaChip in code B. It fires one
200-damage Gospel shot plus one shot per active battle-bug type it clears,
including BN6's Custom-screen bug and its latched runtime state.
The stationary Gospel remains for the complete firing sequence.
Gregar uses the imported BugCharge chip art; Falzar retains the inaccessible
BugRSword-slot art.

## SignlRed

Replaces Navi+20 with Blue Moon SignalRed. The 100-HP traffic light spawns
in front of the user, blocks the opponent's BattleChips for 420 red frames,
then permits them during a 50-frame green window. It repeats this cycle until
destroyed. SignalRed is a 61 MB Obstacle-element StandardChip in code S and
keeps Navi+20's library position. Both versions use its Blue Moon chip art and
battle sprite. Its 100-HP hurtbox remains active throughout the cycle, and
either player's DustCross can suck it in with B+Left.

## DethPhnx

Replaces CrossDiv with BN5 DeathPhoenix in both versions. It fires twelve
DeathPhoenix fireballs and then recycles the last Navi chip used. Falzar uses
DeathPhoenix chip art; Gregar retains the original CrossDiv chip art.

## FoldrBak

Replaces the dormant Falzar Giga chip with BN3 FolderBack in both versions.
It restores every used chip (including itself), discards the current hand,
reshuffles the Folder, and immediately returns to Custom. A consumed Regular
Chip returns as an ordinary chip. The ending uses BN3's original rumble,
70-frame shake and alternating white flash, briefly fills the Custom Gauge,
and opens the native Custom window with its normal sound. The chip also uses
FolderBack's BN3 menu art and text.
