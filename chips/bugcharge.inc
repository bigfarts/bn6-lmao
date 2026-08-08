// ---------------------------------------------------------------------------
// BugCharge replaces BugFix. This is BN5's bug-count version: it always fires
// one 200-damage Gospel shot, adds one shot for each active BN5 battle-bug
// type, then clears the bugs it consumed. The source chip is a GigaChip; this
// port deliberately keeps BugFix's StandardChip class and library position.

.definelabel BUGCHARGE_ID,                    0x0B0
.definelabel BUGCHARGE_SFX_CHARGE,            0x01E0 // exact BN5 SFX 0x0094
.definelabel BUGCHARGE_SFX_FIRE,              0x01E1 // exact BN5 SFX 0x00FF
.definelabel BUGCHARGE_CONTROLLER_TAG,        0x43434742 // "BGCC"
.definelabel BUGCHARGE_ORBIT_TAG,             0x4F434742 // "BGCO"

// B code, BN5 rarity/MB cost, Null element, Standard class, and 200 power.
// Preserve BugFix's alphabetical sort value, library position, and normal-chip
// gate byte while retaining its released BN6 time-freeze route.
.org CHIP_DATA + BUGCHARGE_ID * 0x2C
    .db 0x01,0xFF,0xFF,0xFF             // B
    .db 0x00,0x04,0x0A,0x00             // rarity 4, Null, Standard
    .db 0x4D,0x43,0x8A,0x15
    .db 0x1A,0x01,0x04,0x00             // released BugFix route
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0xB2,0x00,0x00
    .dh 0x002E,200,0x00B2
    .db 0x01,0xFF
    .dw BugChargeIcon,BugChargeImage,BugChargePalette

.definelabel BUGCHARGE_TIMEFREEZE_ENTRY,      0x0802CD1C // family 0x15, subfamily 0x1A
.definelabel TYPE4_BUGCHARGE_ENTRY,            0x080043B4 // released BugFix type-4 slot 0x3B

.org BUGCHARGE_TIMEFREEZE_ENTRY
.dw BugChargeTimeFreezeSpawn + 1
.org TYPE4_BUGCHARGE_ENTRY
.dw BugChargeSharedMain + 1

.autoregion
.align 4
BugChargeCodeStart:

BugChargeTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x3B
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1-r3,r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    strb r3,[r0,0x0E]
    str r5,[r0,0x4C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    str r6,[r0,0x2C]
    str r7,[r0,0x30]
    ldr r1,=BUGCHARGE_CONTROLLER_TAG
    str r1,[r0,0x78]
@@done:
    pop {pc}
    .pool

// The invisible controller and the three BN5 orbit visuals share BugFix's
// released type-4 slot. Gospel shots use SearchMan's tagged type-3 hit slot so
// their collision follows BN6's native counter-hit ownership path.
BugChargeSharedMain:
    ldr r0,[r5,0x78]
    ldr r1,=BUGCHARGE_ORBIT_TAG
    cmp r0,r1
    bne @@controller
    ldr r1,=BugChargeOrbitMain + 1
    bx r1
@@controller:
    ldr r1,=BugChargeControllerMain + 1
    bx r1
    .pool

BugChargeControllerMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall TYPE4_TIMESTOP_FREE
    pop {pc}
@@init:
    EngineCall TYPE4_TIMESTOP_INIT
    pop {pc}
@@update:
    bl BugChargeControllerUpdate
    pop {pc}

BugChargeControllerUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@intro
    cmp r0,4
    beq @@freeze
    cmp r0,8
    beq @@effect
    EngineCall TYPE4_TIMESTOP_OUTRO
    pop {pc}
@@intro:
    EngineCall TYPE4_TIMESTOP_INTRO
    pop {pc}
@@freeze:
    EngineCall TYPE4_TIMESTOP_FREEZE
    pop {pc}
@@effect:
    bl BugChargeEffectUpdate
    pop {pc}

// BN5 waits 40 frames after the charge cue, then fires every 15 frames. Its
// internal repeat counter starts at one, so even a bug-free user gets the
// initial 200-damage Gospel shot; nine active bug types produce ten shots.
BugChargeEffectUpdate:
    push {r4,r6,lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    beq @@init
    cmp r0,4
    bne @@cooldown

@@fire_wait:
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bge @@done
    bl BugChargePlayFireSound
    bl BugChargeSpawnNativeAttack
    bl BugChargeAttackRow
    mov r0,2
    mov r1,20
    EngineCall SCREEN_SHAKE_SET
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    ble @@last_shot
    mov r0,14                          // 15 frames between native shots
    strh r0,[r5,0x22]
    b @@done

@@last_shot:
    mov r0,29                          // native 30-frame recovery
    strh r0,[r5,0x22]
    mov r0,8
    strb r0,[r5,0x0B]
    b @@done

@@cooldown:
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bge @@done
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    b @@done

@@init:
    bl BugChargeCountAndClearBugs
    mov r4,r0
    strh r0,[r5,0x20]
    mov r1,15
    mul r0,r1
    add r0,70                          // keeps all three orbits through recovery
    bl BugChargeSpawnOrbits
    bl BugChargePlayChargeSound
    mov r0,39                          // native 40-frame charge hold
    strh r0,[r5,0x22]
    mov r0,4
    strb r0,[r5,0x0B]
@@done:
    pop {r4,r6,pc}
    .pool

// Actual BN5 gameplay counts these nine battle-property types. Fields 0x31
// and 0x24 are boolean and count only when equal to one; the remaining fields
// count when nonzero. The repeat counter has a native base of one. Unlike BN6
// BugFix, BN5 does not consume BN6-only field 0x63.
BugChargeCountAndClearBugs:
    push {r4,r6,lr}
    ldrb r0,[r5,0x16]
    EngineCall PLAYER_PROPERTIES_FOR_SIDE
    mov r6,r0
    mov r4,1

    mov r1,0x31
    ldrb r0,[r6,r1]
    cmp r0,1
    bne @@general_bytes
    add r4,1
    mov r0,0
    strb r0,[r6,r1]

@@general_bytes:
    ldr r2,=BugChargeByteProperties
@@byte_loop:
    ldrb r1,[r2]
    add r2,1
    cmp r1,0xFF
    beq @@halfword
    ldrb r0,[r6,r1]
    cmp r0,0
    beq @@byte_loop
    add r4,1
    mov r0,0
    strb r0,[r6,r1]
    b @@byte_loop

@@halfword:
    mov r1,0x54
    ldrh r0,[r6,r1]
    cmp r0,0
    beq @@boolean_24
    add r4,1
    mov r0,0
    strh r0,[r6,r1]

@@boolean_24:
    mov r1,0x24
    ldrb r0,[r6,r1]
    cmp r0,1
    bne @@done
    add r4,1
    mov r0,0
    strb r0,[r6,r1]
@@done:
    mov r0,r4
    pop {r4,r6,pc}
    .pool

BugChargeByteProperties:
    .db 0x13,0x14,0x16,0x19,0x18,0x1A,0xFF
    .align 4

BugChargePlayChargeSound:
    push {lr}
    mov r0,BUGCHARGE_SFX_CHARGE >> 1
    lsl r0,r0,1
    EngineCall PLAY_SOUND
    pop {pc}

BugChargePlayFireSound:
    push {lr}
    mov r0,BUGCHARGE_SFX_FIRE >> 1
    lsl r0,r0,1
    add r0,1
    EngineCall PLAY_SOUND
    pop {pc}

// BN5 creates three phase-shifted copies of group 0x0C/index 0x44 around the
// user. This translated object uses the same 0x00/0x55/0xAA phases and the
// source game's 45x28-pixel ellipse.
BugChargeSpawnOrbits:
    push {r4,r6,r7,lr}
    mov r6,r0
    ldrb r0,[r5,0x16]
    EngineCall PLAYER_OBJECT_FOR_SIDE
    mov r7,r0
    cmp r7,0
    beq @@done
    mov r0,0x00
    bl BugChargeSpawnOrbit
    mov r0,0x55
    bl BugChargeSpawnOrbit
    mov r0,0xAA
    bl BugChargeSpawnOrbit
@@done:
    pop {r4,r6,r7,pc}

BugChargeSpawnOrbit:
    push {lr}
    push {r0,r5}
    mov r0,0x3B
    EngineCall SPAWN_TYPE4
    mov r4,r5
    pop {r0,r5}
    cmp r4,0
    beq @@done
    strh r0,[r4,0x20]
    strh r6,[r4,0x22]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    str r7,[r4,0x4C]
    ldr r0,=BUGCHARGE_ORBIT_TAG
    str r0,[r4,0x78]
@@done:
    pop {pc}
    .pool

BugChargeOrbitMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl BugChargeOrbitInit
    b @@animate
@@update:
    bl BugChargeOrbitUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

BugChargeOrbitInit:
    push {lr}
    mov r0,0x80
    mov r1,0x0C
    mov r2,0x20
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    bl BugChargeOrbitSetPosition
    pop {pc}
    .pool

BugChargeOrbitUpdate:
    push {lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
    ldr r0,[r5,0x4C]
    cmp r0,0
    beq @@free
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    blt @@free
    bl BugChargeOrbitSetPosition
    pop {pc}
@@free:
    EngineCall OBJECT_FREE
    pop {pc}

BugChargeOrbitSetPosition:
    push {r4,r6,r7,lr}
    ldr r3,[r5,0x4C]
    ldrh r1,[r5,0x20]
    add r1,8
    strh r1,[r5,0x20]
    mov r2,r1
    mov r0,0xFF
    and r2,r0
    lsl r2,r2,1

    mov r4,1
    ldrb r0,[r5,0x16]
    cmp r0,0
    beq @@direction_done
    neg r4,r4
@@direction_done:
    ldr r0,=ChaosTrigTable + 0x80
    ldsh r0,[r0,r2]
    mov r1,45
    mul r0,r1
    lsl r0,r0,8
    mul r0,r4
    ldr r1,[r3,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]

    ldr r0,=ChaosTrigTable
    ldsh r0,[r0,r2]
    mov r1,28
    mul r0,r1
    lsl r0,r0,8
    ldr r1,[r3,0x38]
    add r0,r0,r1
    str r0,[r5,0x38]
    mov r0,0x18
    lsl r0,r0,16
    str r0,[r5,0x3C]
    pop {r4,r6,r7,pc}
    .pool

// BN5's firing state does not create a private projectile object. It places
// attack-family kind 1 one panel in front of the user, using fixed-point X/Y
// coordinates and a 20-pixel Z origin. BN6 retains the same generic damage
// object family; only its version-specific wrapper address moved.
BugChargeSpawnNativeAttack:
    push {r4,r6,lr}
    EngineCall OBJECT_FRONT_DIRECTION
    ldrb r1,[r5,0x12]
    add r0,r0,r1
    ldrb r1,[r5,0x13]
    EngineCall PANEL_TO_COORDS
    mov r2,r1
    mov r1,r0
    mov r3,20
    lsl r3,r3,16
    ldr r6,[r5,0x2C]
    mov r4,1
.if FALZAR
    EngineCall 0x080C53A6
.else
    EngineCall 0x080C6C16
.endif
    pop {r4,r6,pc}
    .pool

// BN6's retained generic attack wrapper takes panel coordinates for its
// damage contact, whereas BN5's wrapper accepted the fixed-point launch
// coordinates used above. Apply the same kind-1 contact to each valid panel
// in the user's row; the separately spawned native attack object supplies the
// moving Gospel visual while these contacts preserve the row hit semantics.
BugChargeAttackRow:
    push {r4,r6,r7,lr}
    ldrb r0,[r5,0x16]
    cmp r0,0
    bne @@right_side
    ldr r2,=0x00200000
    b @@mask_ready
@@right_side:
    ldr r2,=0x00400000
@@mask_ready:
    mov r0,6
@@panel:
    ldrb r1,[r5,0x13]
    push {r0-r2}
    EngineCall PANEL_HAS_FLAGS
    cmp r0,0
    pop {r0-r2}
    beq @@next
    push {r0-r2}
    bl BugChargeHitSpawn
    pop {r0-r2}
@@next:
    sub r0,1
    bne @@panel
    pop {r4,r6,r7,pc}
    .pool

BugChargeHitSpawn:
    push {r0-r2,r4,r6,r7,lr}
    push {r5}
    mov r0,0x2C
    mov r4,25                         // BN6 normal attack collision region
    EngineCall SPAWN_TYPE3
    mov r4,r5
    pop {r5}
    cmp r4,0
    beq @@failed
    pop {r0-r2}
    strb r0,[r4,0x12]
    strb r1,[r4,0x13]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    ldr r0,[r5,0x2C]
    str r0,[r4,0x2C]
    str r5,[r4,0x4C]
    ldr r0,=BUGCHARGE_PROJECTILE_TAG
    str r0,[r4,0x78]
    ldrb r0,[r4]
    mov r1,0x10
    orr r0,r1
    strb r0,[r4]
    pop {r4,r6,r7,pc}
@@failed:
    pop {r0-r2}
    pop {r4,r6,r7,pc}
    .pool

// ----- Tagged type-3 Gospel shot -----------------------------------------

BugChargeSpawnProjectile:
    push {r4,r6,lr}
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    ldrb r2,[r5,0x0E]
    ldr r6,[r5,0x2C]
    push {r0-r2,r5}
    mov r0,0x2C
    mov r4,25                         // BN6 normal attack collision region
    EngineCall SPAWN_TYPE3
    mov r4,r5
    pop {r0-r2,r5}
    cmp r4,0
    beq @@done
    strb r0,[r4,0x12]
    strb r1,[r4,0x13]
    strb r2,[r4,0x0E]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    str r6,[r4,0x2C]
    str r5,[r4,0x4C]
    ldr r0,=BUGCHARGE_PROJECTILE_TAG
    str r0,[r4,0x78]
    ldrb r0,[r4]
    mov r1,0x10
    orr r0,r1
    strb r0,[r4]
@@done:
    pop {r4,r6,pc}
    .pool

BugChargeProjectileMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl BugChargeProjectileInit
    b @@animate
@@update:
    bl BugChargeProjectileUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

BugChargeProjectileInit:
    push {r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
    EngineCall OBJECT_SET_COORDS
    mov r0,0x10
    lsl r0,r0,16
    str r0,[r5,0x3C]
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free
    mov r1,0x4E                       // BN5 Gospel collision region
    mov r2,0x14                       // BN5 Gospel collision category
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r0,0xFF
    EngineCall COLLISION_SET_HIT_EFFECT
    // BN5 kind 1 writes zero to collision property 0x10, then clears the
    // collision's one-contact latch before presenting it.
    mov r0,0
    EngineCall 0x0801A258
    EngineCall COLLISION_CLEAR_REGION
    EngineCall COLLISION_PRESENT
    mov r0,4
    str r0,[r5,0x08]
    pop {r7,pc}
@@free:
    EngineCall OBJECT_FREE
    pop {r7,pc}
    .pool

BugChargeProjectileUpdate:
    push {lr}
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

BugChargeCodeEnd:
.endautoregion

// Exact BN5 BugCharge menu art and group-0x0C/index-0x44 archive.
.autoregion
.align 4
BugChargeIcon:
    .incbin "build/bugcharge-icon.bin"
.endautoregion
.autoregion
.align 4
BugChargeImage:
    .incbin "build/bugcharge-image.bin"
.endautoregion
.autoregion
.align 4
BugChargePalette:
    .incbin "build/bugcharge-palette.bin"
.endautoregion
.autoregion
.align 4
BugChargeProjectileSprite:
    .incbin "build/bugcharge-projectile-sprite.bin"
.endautoregion

// Exact BN5 charge (SFX 0x94) and per-shot (SFX 0xFF) cues.
.autoregion
.align 4
BugChargeAudioStart:
BugChargeChargeSong:
    .db 0x01,0x00,0x80,0x00
    .dw BugChargeChargeVoicegroup
    .dw BugChargeChargeTrack
BugChargeChargeVoicegroup:
    .db 0x08,0x3C,0x00,0x00
    .dw BugChargeChargeSample
    .db 0xFF,0x00,0xFF,0x00
BugChargeChargeTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x70,0xD9,0x3C,0x7F,0x8A,0xB1
.align 4
BugChargeChargeSample:
    .incbin "build/bugcharge-charge-sample.bin"
.align 4
BugChargeFireSong:
    .db 0x01,0x00,0x80,0x00
    .dw BugChargeFireVoicegroup
    .dw BugChargeFireTrack
BugChargeFireVoicegroup:
    .db 0x0C,0x3C,0x00,0x00
    .dw 0x00000000
    .db 0x00,0x03,0x00,0x00
BugChargeFireTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x60,0xD2,0x3C,0x7F,0x83,0xEA,0x48
    .db 0x9B,0x81,0xB1
BugChargeAudioEnd:
.endautoregion
