// ---------------------------------------------------------------------------
// LaserMan replaces HeatMan/HeatManEX/HeatManSP. Blue Moon has Base/SP/DS
// records; as with SearchMan, BN6 receives Base/EX/SP behavior parameters
// 0/3/4, a background-only EX menu palette, and the native SP menu palette.

.definelabel LASERMAN_ID,                    0xE3

// Preserve HeatMan's version-specific reserved and library-ordering fields at
// +0x14..+0x17 and +0x1C..+0x1F. The final ROM pass replaces the inherited
// alphabetical-sort key at +0x18 from the new names.
.org CHIP_DATA + (LASERMAN_ID + 0) * 0x2C
    .db 0x0B,0x1A,0xFF,0xFF             // L and *
    .db 0x00,0x02,0x0A,0x01             // rarity 2, Null, Mega
    .db 0x3C,0x47,0x8A,0x1B
    .db 0x02,0x00,0x04,0x00             // HeatMan family/subfamily route
    .dw 0x00000000                       // Base behavior
.org CHIP_DATA + (LASERMAN_ID + 0) * 0x2C + 0x1A
    .dh 100
.org CHIP_DATA + (LASERMAN_ID + 0) * 0x2C + 0x20
    .dw LaserManIcon,LaserManImage,LaserManPaletteBase

.org CHIP_DATA + (LASERMAN_ID + 1) * 0x2C
    .db 0x0B,0xFF,0xFF,0xFF             // L
    .db 0x00,0x03,0x0A,0x01             // rarity 3, Null, Mega
    .db 0x50,0x47,0x8A,0x1B
    .db 0x02,0x00,0x04,0x00
    .dw 0x00000003                       // EX behavior
.org CHIP_DATA + (LASERMAN_ID + 1) * 0x2C + 0x1A
    .dh 150
.org CHIP_DATA + (LASERMAN_ID + 1) * 0x2C + 0x20
    .dw LaserManIcon,LaserManImage,LaserManPaletteEx

.org CHIP_DATA + (LASERMAN_ID + 2) * 0x2C
    .db 0x0B,0xFF,0xFF,0xFF             // L
    .db 0x00,0x04,0x0A,0x01             // rarity 4, Null, Mega
    .db 0x50,0x47,0x8A,0x1B
    .db 0x02,0x00,0x04,0x00
    .dw 0x00000004                       // SP behavior
.org CHIP_DATA + (LASERMAN_ID + 2) * 0x2C + 0x1A
    .dh 200
.org CHIP_DATA + (LASERMAN_ID + 2) * 0x2C + 0x20
    .dw LaserManIcon,LaserManImage,LaserManPaletteSp

.definelabel LASERMAN_SFX_SUMMON,            0x01DB // exact BN4 SFX 0x0B0
.definelabel LASERMAN_SFX_FIRE,              0x01DD // exact BN4 SFX 0x103
.definelabel LASERMAN_LASER_TAG,             0x4C415345
.definelabel LASERMAN_HIT_TAG,               0x4C484954

.definelabel TIMEFREEZE_DISPATCH_HEATMAN,    0x0802CD64
.definelabel TYPE1_LASERMAN_ENTRY,           0x08003D5C // released type-1 slot 0x30

.org TIMEFREEZE_DISPATCH_HEATMAN
.dw LaserManTimeFreezeSpawn + 1
.org TYPE1_LASERMAN_ENTRY
.dw LaserManSlotMain + 1

.autoregion
.align 4
LaserManCodeStart:

// r0/r1 are the summon panel, r2 is the time-freeze parameter, r4 is the
// Base/EX/SP parameter, r5 is the controller, r6 is the packed attack word,
// and r7 points to the completion byte.
LaserManTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x30
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r1-r3,r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    strb r3,[r0,0x0E]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    // Latch the command as soon as the time-freeze summon is created. The
    // actor also samples throughout its raised-arms pose, matching BN4 while
    // accepting a direction held with the chip-use input.
    mov r1,0
    strb r1,[r0,0x05]
    push {r0,r5}
    mov r5,r0
    bl LaserManActorReadCommand
    pop {r0,r5}
    str r5,[r0,0x4C]
    str r6,[r0,0x2C]
    str r7,[r0,0x64]
    mov r1,0
    str r1,[r0,0x78]                   // actor half of the shared slot
    mov r1,1
    strb r1,[r7]
@@done:
    pop {pc}
    .pool

// The actor and visible beam share released type-1 slot 0x30. Both use the
// foreground battle-sprite priority; because the actor is allocated first it
// remains in front at the muzzle, while the beam crosses in front of targets
// and field objects as it does in Blue Moon. LaserMan's tagged collisions
// share SearchMan's proven type-3 hit slot 0x2C.
LaserManSlotMain:
    ldr r0,[r5,0x78]
    ldr r1,=LASERMAN_LASER_TAG
    cmp r0,r1
    bne @@actor
    ldr r1,=LaserManLaserMain + 1
    bx r1
@@actor:
    b LaserManActorMain
    .pool

// ----- LaserMan actor ------------------------------------------------------

LaserManActorMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    bl LaserManActorDestroy
    pop {pc}
@@init:
    bl LaserManActorInit
    b @@animate
@@update:
    bl LaserManActorUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

LaserManActorInit:
    push {lr}
    // BN4's preloader takes a packed 0xD0 selector. BN6 changed this helper
    // to separate group/index arguments; preload the relocated compressed
    // archive before either the actor or its beam asks SPRITE_LOAD for it.
    mov r0,0x0C
    mov r1,LASERMAN_BATTLE_SPRITE_INDEX
    EngineCall SPRITE_ALLOC
    mov r0,0x80
    mov r1,0x0C
    mov r2,LASERMAN_BATTLE_SPRITE_INDEX
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_2E3C
    mov r0,0
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall OBJECT_SET_COORDS
    mov r0,0
    str r0,[r5,0x3C]
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    // Match SearchMan's mapping: variants change chip art/power, while the
    // summoned Navi consistently uses the native base battle palette.
    mov r0,0
    EngineCall SPRITE_SET_PALETTE
    mov r0,1                           // actor draws in front of the beam
    EngineCall SPRITE_SET_PRIORITY
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    mov r0,0
    strb r0,[r5,0x09]
    strb r0,[r5,0x0B]
    mov r0,20                          // native pre-attack wait
    strh r0,[r5,0x20]
    mov r0,LASERMAN_SFX_SUMMON >> 1
    lsl r0,r0,1
    add r0,1
    EngineCall PLAY_SOUND
    pop {pc}
    .pool

LaserManActorUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq LaserManActorWait
    b LaserManActorAttack

LaserManActorWait:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    mov r2,0x10
    mov r3,0
    EngineCall PANEL_HAS_FLAGS
    cmp r0,0
    bne @@ready
    mov r0,8
    str r0,[r5,0x08]
    b @@done
@@ready:
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strb r0,[r5,0x0B]
@@done:
    pop {pc}

LaserManActorAttack:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    beq @@raise_arms
    cmp r0,4
    beq @@raise_tick
    cmp r0,8
    beq @@laser_tick

    // Native animation 4 is the post-shot return pose.
    EngineCall SPRITE_GET_ANIMATION_FLAGS
    mov r1,0x80
    tst r0,r1
    beq @@done
    mov r0,8
    str r0,[r5,0x08]
    b @@done

@@raise_arms:
    mov r0,2                           // arms raised
    bl LaserManActorSetAnimation
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
    b @@done

@@raise_tick:
    // Blue Moon samples the held direction throughout this pose and latches
    // the last recognized command. Apply it to Base/EX/SP as requested.
    bl LaserManActorReadCommand
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,3                           // point and fire
    bl LaserManActorSetAnimation
    bl LaserManLaserSpawn
    mov r0,LASERMAN_SFX_FIRE >> 1
    lsl r0,r0,1
    add r0,1
    EngineCall PLAY_SOUND
    mov r0,80                          // native beam hold
    strh r0,[r5,0x20]
    mov r0,8
    strb r0,[r5,0x0B]
    b @@done

@@laser_tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,4                           // native recovery pose
    bl LaserManActorSetAnimation
    mov r0,12
    strb r0,[r5,0x0B]
@@done:
    pop {pc}

LaserManActorReadCommand:
    push {lr}
    ldrb r0,[r5,0x16]
    EngineCall INPUT_STATE_FOR_SIDE
    ldrh r0,[r0,0x02]                 // held keys
    mov r1,0x40                       // Up: reset Buster stats
    tst r0,r1
    bne @@up
    mov r1,0x80                       // Down: strip passive abilities
    tst r0,r1
    bne @@down
    mov r1,0x10                       // Right: reset modified charge shot
    tst r0,r1
    bne @@right
    mov r1,0x20                       // Left: one fewer Custom Screen chip
    tst r0,r1
    beq @@done
    mov r1,4
    b @@store
@@right:
    mov r1,3
    b @@store
@@down:
    mov r1,2
    b @@store
@@up:
    mov r1,1
@@store:
    strb r1,[r5,0x05]
@@done:
    pop {pc}
    .pool

LaserManActorSetAnimation:
    push {lr}
    strb r0,[r5,0x10]
    mov r1,0xFF
    strb r1,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    pop {pc}

LaserManActorDestroy:
    push {lr}
    ldr r0,[r5,0x64]
    cmp r0,0
    beq @@free
    mov r1,0
    strb r1,[r0]
@@free:
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

// ----- Full-width Blue Moon laser -----------------------------------------

LaserManLaserSpawn:
    push {r4,r6,r7,lr}
    push {r5}
    mov r0,0x30
    ldrb r4,[r5,0x05]                 // implicit spawn field +4: command
    EngineCall SPAWN_TYPE1
    mov r4,r5
    pop {r5}
    cmp r4,0
    beq @@done
    // BN4 creates the beam object one panel in front of LaserMan. The beam
    // init then applies its separate 64-pixel sprite-origin adjustment.
    EngineCall OBJECT_SIDE_DIRECTION
    ldrb r1,[r5,0x12]
    add r0,r0,r1
    strb r0,[r4,0x12]
    ldrb r0,[r5,0x13]
    strb r0,[r4,0x13]
    ldrb r0,[r5,0x0E]
    strb r0,[r4,0x0E]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    ldr r0,[r5,0x2C]
    str r0,[r4,0x2C]
    str r5,[r4,0x4C]
    ldr r0,=LASERMAN_LASER_TAG
    str r0,[r4,0x78]
    ldrb r0,[r4]
    mov r1,0x10
    orr r0,r1
    strb r0,[r4]
@@done:
    pop {r4,r6,r7,pc}
    .pool

LaserManLaserMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl LaserManLaserInit
    b @@animate
@@update:
    bl LaserManLaserUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

LaserManLaserInit:
    push {lr}
    EngineCall OBJECT_SET_COORDS
    // BN6 reuses type-1 object tails without clearing the Z word. The BN4
    // laser allocator supplied zero here; leaving BN6's stale value intact
    // places the beam far outside the visible scene.
    mov r0,0
    str r0,[r5,0x3C]
    EngineCall OBJECT_SIDE_DIRECTION
    mov r1,0x40                        // source begins 64 px in front
    mul r0,r1
    lsl r0,r0,16
    ldr r1,[r5,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]
    mov r0,0x80
    mov r1,0x0C
    mov r2,LASERMAN_BATTLE_SPRITE_INDEX
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,17                          // thin beam lead-in
    strb r0,[r5,0x10]
    mov r1,0xFF
    strb r1,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    mov r0,0
    ldrb r1,[r5,0x04]
    cmp r1,0
    beq @@set_palette
    cmp r1,2                           // Down keeps the standard blue beam
    beq @@set_palette
    mov r0,10                          // BN4 command-beam palette
@@set_palette:
    EngineCall SPRITE_SET_PALETTE
    mov r0,1                           // foreground beam; actor wins at muzzle
    EngineCall SPRITE_SET_PRIORITY
    // Blue Moon applies a second command-specific sprite property after the
    // palette. In BN6 this is the matching sprite-scale halfword setter.
    ldrb r0,[r5,0x04]
    lsl r0,r0,2
    ldr r1,=LaserManCommandBeamProperties
    ldr r0,[r1,r0]
    EngineCall SPRITE_SET_SCALE
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    mov r0,0
    strb r0,[r5,0x09]
    pop {pc}
    .pool

LaserManLaserUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@lead_in
    cmp r0,4
    beq @@hold

    EngineCall SPRITE_GET_ANIMATION_FLAGS
    mov r1,0x80
    tst r0,r1
    beq @@done
    mov r0,8
    str r0,[r5,0x08]
    b @@done

@@lead_in:
    EngineCall SPRITE_GET_ANIMATION_FLAGS
    mov r1,0x80
    tst r0,r1
    beq @@done
    mov r0,18                          // full white/blue row beam
    bl LaserManLaserSetAnimation
    mov r0,0
    strb r0,[r5,0x06]                 // command-sequence event index
    strb r0,[r5,0x07]                 // event cadence countdown
    mov r0,60
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x09]
    b @@done

@@hold:
    bl LaserManLaserCommandTick
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,19                          // white beam tail
    bl LaserManLaserSetAnimation
    mov r0,8
    strb r0,[r5,0x09]
@@done:
    pop {pc}

LaserManLaserSetAnimation:
    push {lr}
    strb r0,[r5,0x10]
    mov r1,0xFF
    strb r1,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    pop {pc}

// Blue Moon command streams at 0x080E0F70-0x080E0FC0. An event is emitted
// every six frames, preserving the original effect ordering.
LaserManLaserCommandTick:
    push {r4,lr}
    ldrb r0,[r5,0x07]
    cmp r0,0
    beq @@next
    sub r0,1
    strb r0,[r5,0x07]
    b @@done
@@next:
    ldrb r0,[r5,0x04]
    lsl r0,r0,2
    ldr r1,=LaserManCommandStreams
    ldr r1,[r1,r0]
    ldrb r0,[r5,0x06]
    lsl r0,r0,1
    ldrh r2,[r1,r0]
    mov r0,0xFF                        // end marker
    cmp r2,r0
    beq @@done
    ldrb r0,[r5,0x06]
    add r0,1
    strb r0,[r5,0x06]
    mov r0,5                           // next event after six update frames
    strb r0,[r5,0x07]
    mov r0,0xFD
    cmp r2,r0
    beq @@spawn
    // Advance through Blue Moon's command cadence, but defer every property
    // change until the actual damage event reports contact. Spawning separate
    // command collisions lets unrelated field contacts authorize effects.
    b @@done
@@spawn:
    bl LaserManSpawnRowEvent
@@done:
    pop {r4,pc}
    .pool

// Apply one Blue Moon command event to the opposing BN6 NaviStats block.
// Internal Buster levels are zero-based, so zero is the displayed level 1.
LaserManApplyCommandEffect:
    push {r4,r6,lr}
    mov r4,r2
    mov r1,0xFF
    and r4,r1                          // discard the BN4 high-byte parameter
    ldrb r6,[r5,0x16]
    mov r0,1
    eor r6,r0                          // target the opposing side

    cmp r4,1
    beq @@super_armor
    cmp r4,2
    beq @@float_shoes
    cmp r4,3
    beq @@air_shoes
    cmp r4,4
    beq @@under_shirt
    cmp r4,5
    beq @@attack
    cmp r4,6
    beq @@speed
    cmp r4,7
    beq @@charge
    cmp r4,0x0A
    beq @@default_buster
    cmp r4,0x0C
    beq @@b_left
    cmp r4,0xFE
    beq @@custom
    b @@done

@@super_armor:
    mov r1,0x23
    mov r2,0
    b @@set
@@float_shoes:
    mov r1,0x1B
    mov r2,0
    b @@set
@@air_shoes:
    mov r1,0x1C
    mov r2,0
    b @@set
@@under_shirt:
    mov r1,0x1D
    mov r2,0
    b @@set
@@attack:
    mov r1,1
    mov r2,0
    b @@set
@@speed:
    mov r1,2
    mov r2,0
    b @@set
@@charge:
    mov r1,3
    mov r2,0
    b @@set
@@b_left:
    mov r1,7
    mov r2,0xFF
    b @@set

@@default_buster:
    // Record whether the live power-attack cache still matches the underlying
    // NaviStats value before Right changes it. A mismatch is a temporary
    // Cross/other-form charge shot and must remain in the live cache.
    mov r0,r6
    mov r1,5
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    mov r4,r0
    mov r0,r6
    EngineCall PLAYER_OBJECT_FOR_SIDE
    cmp r0,0
    beq @@set_default_buster
    ldr r0,[r0,0x58]
    ldrb r0,[r0,0x07]
    cmp r0,r4
    bne @@set_default_buster
    mov r0,1
    strb r0,[r5,0x0A]                 // Right may refresh the normal cache
@@set_default_buster:
    mov r0,r6
    mov r1,4                          // plain B-button MegaBuster
    mov r2,0
    EngineCall PLAYER_PROPERTY_SET_FOR_SIDE
    mov r1,5                          // default MegaBuster power attack
    mov r2,1
    b @@set

@@custom:
    mov r0,r6
    mov r1,0x63                       // BN6 turns-until-Custom-bug threshold
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    cmp r0,0
    bne @@done                        // preserve any existing Custom bug
    mov r1,0x63
    // Native BN6 encodes bug levels 1/2/3 as turn thresholds 4/3/2. Literal
    // 1 is not level 1: it starts the reduction on turn 1 and ramps at once.
    mov r2,4                          // Custom bug 1: starts on battle turn 4

@@set:
    mov r0,r6
    EngineCall PLAYER_PROPERTY_SET_FOR_SIDE
@@done:
    pop {r4,r6,pc}
    .pool

LaserManRefreshTargetPlayer:
    push {r4-r7,lr}
    ldrb r6,[r5,0x16]
    mov r1,1
    eor r6,r1                          // opposing side
    mov r0,r6
    EngineCall PLAYER_OBJECT_FOR_SIDE
    cmp r0,0
    beq @@done
    mov r5,r0

    // Right refreshes the power-attack cache only when ApplyCommandEffect saw
    // that it still matched the old base property. A temporary Cross charge
    // shot makes those values differ and leaves the guard byte clear.
    ldr r7,[r5,0x58]
    ldrh r0,[r4,0x06]
    mov r1,0xFF
    and r0,r1
    cmp r0,0x0A
    bne @@b_left
    ldrb r0,[r4,0x0A]
    cmp r0,0
    beq @@done
    mov r0,r6
    mov r1,5
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    strb r0,[r7,0x07]
    b @@done
@@b_left:
    cmp r0,0x0C
    bne @@passives
    mov r0,r6
    mov r1,7
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    strb r0,[r7,0x08]

    // These four NaviCust abilities also have live player-status bits. Only
    // Down emits property IDs 1-4; other commands must not touch the cache.
@@passives:
    ldrh r0,[r4,0x06]
    cmp r0,1
    blt @@done
    cmp r0,4
    bgt @@done
    mov r0,r6
    mov r1,0x1B
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    cmp r0,0
    bne @@air
    mov r0,0x20
    EngineCall PLAYER_CLEAR_STATUS_FLAGS
@@air:
    mov r0,r6
    mov r1,0x1C
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    cmp r0,0
    bne @@under
    mov r0,0x10
    EngineCall PLAYER_CLEAR_STATUS_FLAGS
@@under:
    mov r0,r6
    mov r1,0x1D
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    cmp r0,0
    bne @@armor
    mov r0,1
    lsl r0,r0,18
    EngineCall PLAYER_CLEAR_STATUS_FLAGS
@@armor:
    mov r0,r6
    mov r1,0x23
    EngineCall PLAYER_PROPERTY_FOR_SIDE
    cmp r0,0
    bne @@done
    mov r0,1
    lsl r0,r0,17
    EngineCall PLAYER_CLEAR_STATUS_FLAGS
@@done:
    pop {r4-r7,pc}
    .pool

// Emit the source event on every panel because the imported full-width beam is
// stationary in BN6. Events stay six frames apart, bounding type-3 pressure to
// six short-lived hits while retaining the complete-row piercing behavior.
LaserManSpawnRowEvent:
    push {r4,r7,lr}
    mov r7,r2
    mov r4,1
@@panel:
    mov r0,r4
    ldrb r1,[r5,0x13]
    mov r2,r7
    bl LaserManHitSpawn
    add r4,1
    cmp r4,7
    blt @@panel
    pop {r4,r7,pc}

LaserManHitSpawn:
    push {r0-r2,r4,r6,r7,lr}
    push {r5}
    mov r0,0x2C                       // shared SearchMan type-3 hit slot
    mov r4,25                         // BN6 normal attack collision region
    EngineCall SPAWN_TYPE3
    mov r4,r5
    pop {r5}
    cmp r4,0
    beq @@done
    pop {r0-r2}
    strb r0,[r4,0x12]
    strb r1,[r4,0x13]
    strh r2,[r4,0x06]                 // command event word
    ldrb r0,[r5,0x04]
    str r0,[r4,0x74]                  // selected command for hit resolution
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    mov r0,0xFD
    cmp r2,r0
    bne @@command_effect
    // Normal damage uses SearchMan's proven region-25 collision init. The
    // selected command remains latched at +0x74 and is applied once only if
    // this damage event contacts the opposing Navi.
    ldr r0,[r5,0x2C]
    str r0,[r4,0x2C]
    ldr r0,=LASERMAN_HIT_TAG
    str r0,[r4,0x78]
    b @@activate
@@command_effect:
    mov r0,0
    str r0,[r4,0x2C]
    strb r0,[r4,0x0A]                 // conditional Right-cache guard
    ldr r0,=LASERMAN_HIT_TAG
    str r0,[r4,0x78]
@@activate:
    ldrb r0,[r4]
    mov r1,0x10                       // active during the frozen attack
    orr r0,r1
    strb r0,[r4]
    pop {r4,r6,r7,pc}
@@done:
    pop {r0-r2}                       // discard saved panel/event arguments
    pop {r4,r6,r7,pc}
    .pool

LaserManHitMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    ldrh r0,[r5,0x06]
    cmp r0,0xFD
    bne @@command_init
    bl SearchManHitInit
    pop {pc}
@@command_init:
    bl LaserManHitInit
    pop {pc}
@@update:
    bl LaserManHitUpdate
    pop {pc}

LaserManHitInit:
    push {r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
    EngineCall PANEL_IS_VALID_OBJECT
    cmp r0,0
    beq @@free
    EngineCall OBJECT_SET_COORDS
    // Match BN6's working SearchMan hit objects: collision-bearing type-3
    // objects need the standard 16-pixel battle height initialized explicitly.
    mov r0,0x10
    lsl r0,r0,16
    str r0,[r5,0x3C]
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free
    ldrb r1,[r5,0x04]                 // region 25, seeded by SPAWN_TYPE3
    mov r2,5
    mov r3,3                           // every command word must contact
    EngineCall COLLISION_SETUP
    ldrh r1,[r5,0x06]
    cmp r1,0xFD
    bne @@extended
    mov r0,0                           // SearchMan's proven normal-hit effect
    EngineCall COLLISION_SET_HIT_EFFECT
    b @@present
@@extended:
    mov r0,0xFF
    EngineCall COLLISION_SET_HIT_EFFECT
    // BN4 property IDs are not BN6 extended-effect IDs. Keep this a harmless
    // contact and translate it ourselves after collision resolution.
    mov r0,0
    mov r1,0
    EngineCall COLLISION_SET_EXTENDED_EFFECT
@@present:
    // BN6's presenter consumes r1 as well as the collision pointer. Reload
    // the region after decoding the command event so it cannot see FD/FE.
    ldrb r1,[r5,0x04]
    EngineCall COLLISION_PRESENT
    mov r0,4
    str r0,[r5,0x08]
    pop {r7,pc}
@@free:
    EngineCall OBJECT_FREE
    pop {r7,pc}

LaserManHitUpdate:
    push {r4,lr}
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    ldr r4,[r5,0x54]
    ldr r0,[r4,0x70]
    cmp r0,0
    beq @@cleanup
    // Region 25 can also contact field objects. Require this particular row
    // hit to occupy the opposing Navi's panel so a trap/obstacle elsewhere in
    // the beam cannot authorize a command effect on a missed player.
    ldrb r0,[r5,0x16]
    mov r1,1
    eor r0,r1
    EngineCall PLAYER_OBJECT_FOR_SIDE
    cmp r0,0
    beq @@cleanup
    ldrb r1,[r5,0x12]
    ldrb r2,[r0,0x12]
    cmp r1,r2
    bne @@cleanup
    ldrb r1,[r5,0x13]
    ldrb r2,[r0,0x13]
    cmp r1,r2
    bne @@cleanup
    ldrh r0,[r5,0x06]
    cmp r0,0xFD
    bne @@cleanup
    bl LaserManApplySelectedCommand
@@cleanup:
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
    EngineCall OBJECT_FREE
    pop {r4,pc}
    .pool

// Apply the complete direction-specific command stream after the real damage
// contact resolves. Property words are translated exactly once; the FD damage
// and FF terminator are not command effects.
LaserManApplySelectedCommand:
    push {r4,r6,r7,lr}
    ldr r0,[r5,0x74]
    lsl r0,r0,2
    ldr r1,=LaserManCommandStreams
    ldr r7,[r1,r0]
@@next:
    ldrh r2,[r7]
    add r7,2
    mov r0,0xFF
    cmp r2,r0
    beq @@done
    mov r0,0xFD
    cmp r2,r0
    beq @@done
    strh r2,[r5,0x06]
    bl LaserManApplyCommandEffect
    mov r4,r5
    bl LaserManRefreshTargetPlayer
    b @@next
@@done:
    pop {r4,r6,r7,pc}
    .pool

.align 4
LaserManCommandStreams:
    .dw LaserManCommandNone
    .dw LaserManCommandUp
    .dw LaserManCommandDown
    .dw LaserManCommandRight
    .dw LaserManCommandLeft
LaserManCommandNone:
    .dh 0x00FD,0x00FF
LaserManCommandUp:
    .dh 0x0005,0x0006,0x0007,0x00FD,0x00FF
LaserManCommandDown:
    .dh 0x0001,0x0002,0x0003,0x0004,0xFF0C,0x00FD,0x00FF
LaserManCommandRight:
    .dh 0x010A,0x00FD,0x00FF
LaserManCommandLeft:
    .dh 0x00FE,0x00FD,0x00FF
    .pool

.align 4
LaserManCommandBeamProperties:
    .dw 0x00000000                    // no command
    .dw 0x0000B060                    // Up
    .dw 0x0000A80A                    // Down
    .dw 0x00000000                    // Right
    .dw 0x0000B9C0                    // Left

LaserManCodeEnd:
.endautoregion

// Blue Moon menu art and the runtime-confirmed, self-contained LaserMan
// archive (actor animations 0/2/3/4 and row-laser animations 17/18/19).
.autoregion
.align 4
LaserManIcon:
.incbin "build/laserman-icon.bin"
.endautoregion
.autoregion
.align 4
LaserManImage:
.incbin "build/laserman-image.bin"
.endautoregion
.autoregion
.align 4
LaserManPaletteBase:
.incbin "build/laserman-pal-base.bin"
.endautoregion
.autoregion
.align 4
LaserManPaletteEx:
    // LaserMan's image assigns the variant background to indices 1-5 (unlike
    // SearchMan, whose background occupies the final three entries). Recolor
    // only those five entries and preserve every LaserMan/laser foreground
    // color from the native base palette.
    .incbin "build/laserman-pal-base.bin",0,0x02
    .dh 0x00C0,0x0180,0x0280,0x03E0,0x0060
    .incbin "build/laserman-pal-base.bin",0x0C,0x14
.endautoregion
.autoregion
.align 4
LaserManPaletteSp:
.incbin "build/laserman-pal-sp.bin"
.endautoregion
.autoregion
.align 4
LaserManBattleSprite:
.incbin "build/laserman-battle-sprite.bin"
.endautoregion

// Exact Blue Moon LaserMan firing cue (SFX 0x103). The actor's SFX 0xB0
// summon cue already occupies relocated slot 0x1DB for RollArrow.
.autoregion
.align 4
LaserManFireAudioStart:
LaserManFireSong:
    .db 0x01,0x00,0x40,0x00
    .dw LaserManFireVoicegroup
    .dw LaserManFireTrack
LaserManFireVoicegroup:
    .db 0x00,0x3C,0x00,0x00
    .dw LaserManFireSample
    .db 0xFF,0x00,0xFF,0x00
LaserManFireTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x7F,0xF6,0x2F,0x7F,0xA2,0x81,0xBE
    .db 0x60,0x84,0x40,0x84,0x20,0x84,0x10,0x84
    .db 0xB1
.align 4
LaserManFireSample:
    .incbin "build/laserman-fire-sample.bin"
LaserManFireAudioEnd:
.endautoregion
