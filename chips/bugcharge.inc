// ---------------------------------------------------------------------------
// BugCharge replaces BugFix. BN4 fires one 80-damage Gospel shot for every
// Custom turn reached with the chip, capped at ten shots. The original chip is
// a GigaChip; this port deliberately keeps BugFix's StandardChip class and
// library position.

.definelabel BUGCHARGE_ID,                    0x0B0
.definelabel BUGCHARGE_SFX,                   0x01E0 // exact BN4 SFX 0x006A
.definelabel BUGCHARGE_CONTROLLER_TAG,        0x43434742 // "BGCC"
.definelabel BUGCHARGE_PROJECTILE_TAG,        0x50434742 // "BGCP"
.definelabel BUGCHARGE_TURN_COUNTER,          0x02034887

// C code, BN4 rarity/MB cost/properties, Null element, and 80 power. Preserve
// BugFix's alphabetical sort value, Standard library position, and gate byte.
.org CHIP_DATA + BUGCHARGE_ID * 0x2C
    .db 0x02,0xFF,0xFF,0xFF             // C
    .db 0x00,0x04,0x0A,0x00             // rarity 4, Null, Standard
    .db 0x4D,0x43,0x8A,0x15
    .db 0x1A,0x01,0x04,0x00             // released BugFix route
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0xB2,0x00,0x00
    .dh 0x002E,80,0x00B2
    .db 0x01,0xFF
    .dw BugChargeIcon,BugChargeImage,BugChargePalette

.definelabel BUGCHARGE_TIMEFREEZE_ENTRY,      0x0802CD1C // family 0x15, subfamily 0x1A
.definelabel TYPE4_BUGCHARGE_ENTRY,            0x080043B4 // released BugFix type-4 slot 0x3B
.definelabel SPRITE_PTR_BUGCHARGE_PROJECTILE,  0x08031E80 // released BugFix group 0x0C/index 0x20

.org BUGCHARGE_TIMEFREEZE_ENTRY
.dw BugChargeTimeFreezeSpawn + 1
.org TYPE4_BUGCHARGE_ENTRY
.dw BugChargeSharedMain + 1
.org SPRITE_PTR_BUGCHARGE_PROJECTILE
.dw BugChargeProjectileSprite

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

// The invisible time-freeze controller and visible Gospel shots share the
// released BugFix type-4 slot. A private object-tail tag selects the role.
BugChargeSharedMain:
    ldr r0,[r5,0x78]
    ldr r1,=BUGCHARGE_PROJECTILE_TAG
    cmp r0,r1
    bne @@controller
    ldr r1,=BugChargeProjectileMain + 1
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

// BN6 increments this battle counter once for every Custom opening. Clamp it
// to BN4's 1..10 range and issue one distinct projectile/contact per turn.
BugChargeEffectUpdate:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    ldr r1,=BUGCHARGE_TURN_COUNTER
    ldrb r0,[r1]
    cmp r0,1
    bge @@minimum_done
    mov r0,1
@@minimum_done:
    cmp r0,10
    ble @@maximum_done
    mov r0,10
@@maximum_done:
    strh r0,[r5,0x20]                  // shots remaining
    mov r0,0
    strh r0,[r5,0x22]                  // fire immediately
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bge @@done
    ldrh r0,[r5,0x20]
    cmp r0,0
    beq @@finish
    sub r0,1
    strh r0,[r5,0x20]
    bl BugChargePlaySound
    bl BugChargeSpawnProjectile
    ldrh r0,[r5,0x20]
    cmp r0,0
    beq @@last_shot
    mov r0,11                          // twelve frames between shots
    strh r0,[r5,0x22]
    b @@done
@@last_shot:
    mov r0,24                          // let the last shot cross the field
    strh r0,[r5,0x22]
    b @@done
@@finish:
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}
    .pool

BugChargePlaySound:
    push {lr}
    mov r0,BUGCHARGE_SFX - 0xFF
    add r0,0xFF
    EngineCall PLAY_SOUND
    pop {pc}

BugChargeSpawnProjectile:
    push {r4,r6,lr}
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    ldrb r2,[r5,0x0E]
    ldr r6,[r5,0x2C]
    push {r0-r2,r5}
    mov r0,0x3B
    EngineCall SPAWN_TYPE4
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

// The sprite is BN4 group 0x0C/index 0x09, used by the original BugCharge
// contact object. It is relocated into BugFix's released BN6 sprite slot.
BugChargeProjectileInit:
    push {r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
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
    EngineCall OBJECT_SET_COORDS
    mov r0,0
    str r0,[r5,0x3C]
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free
    // BN4 uses collision type 7 here. BN6's matching straight-projectile
    // contact is type 8 (the same translated mapping used by RollArrow).
    mov r1,8
    mov r2,5
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r0,9
    EngineCall COLLISION_SET_HIT_EFFECT
    EngineCall COLLISION_PRESENT
    mov r0,8
    strb r0,[r5,0x06]
    EngineCall OBJECT_FRONT_DIRECTION
    ldrb r1,[r5,0x12]
    add r0,r0,r1
    strb r0,[r5,0x14]
    mov r0,4
    str r0,[r5,0x08]
    pop {r7,pc}
@@free:
    EngineCall OBJECT_FREE
    pop {r7,pc}
    .pool

BugChargeProjectileUpdate:
    push {r4,r6,r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne BugChargeProjectileFreeCollision
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    ldr r7,[r5,0x54]
    ldr r0,[r7,0x70]
    cmp r0,0
    bne BugChargeProjectileFreeCollision

    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    ldrb r0,[r5,0x14]
    ldrb r1,[r5,0x13]
    push {r0-r1,r4}
    EngineCall PANEL_IS_VALID_XY
    cmp r0,0
    pop {r0-r1,r4}
    beq BugChargeProjectileFreeCollision
    EngineCall PANEL_TO_COORDS
    mov r2,r0
    mov r0,r4
    mov r1,6                           // BN4 Gospel-shot speed
    lsl r1,r1,16
    mul r0,r1
    ldr r1,[r5,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]
    cmp r4,0
    blt @@moving_left
    cmp r0,r2
    blt @@done
    b @@entered
@@moving_left:
    cmp r0,r2
    bgt @@done
@@entered:
    str r2,[r5,0x34]
    ldrb r0,[r5,0x14]
    strb r0,[r5,0x12]
    EngineCall OBJECT_UPDATE_PANEL
    EngineCall COLLISION_UPDATE_PANEL
    ldrb r0,[r5,0x06]
    sub r0,1
    strb r0,[r5,0x06]
    ble BugChargeProjectileFreeCollision
    EngineCall OBJECT_FRONT_DIRECTION
    ldrb r1,[r5,0x14]
    add r0,r0,r1
    strb r0,[r5,0x14]
@@done:
    EngineCall COLLISION_PRESENT
    pop {r4,r6,r7,pc}
    .pool

BugChargeProjectileFreeCollision:
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
    EngineCall OBJECT_FREE
    pop {r4,r6,r7,pc}
    .pool

BugChargeCodeEnd:
.endautoregion

// Exact BN4 BugCharge menu art and Gospel-shot archive.
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

// Exact BN4 SFX 0x006A. This PSG voice needs no PCM sample.
.autoregion
.align 4
BugChargeAudioStart:
BugChargeFireSong:
    .db 0x01,0x00,0x40,0x00
    .dw BugChargeFireVoicegroup
    .dw BugChargeFireTrack
BugChargeFireVoicegroup:
    .db 0x09,0x3C,0x00,0x1F
    .dw 0x00000000
    .db 0x00,0x01,0x00,0x00
BugChargeFireTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBE,0x7F
    .db 0xBF,0x40,0xD6,0x7F,0x7F,0x87,0xB1
BugChargeAudioEnd:
.endautoregion
