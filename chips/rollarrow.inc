// ---------------------------------------------------------------------------
// RollArrow1/2/3 replace TrainArrow1/2/3. This is the Blue Moon RollArrow
// attack: Roll appears, uses her bow animation 7, and fires one heart-arrow
// straight down the user's row. The actor and arrow selectors below were
// captured from the original Blue Moon attack at runtime.

.definelabel ROLLARROW1_ID,                 0x18
.definelabel ROLLARROW2_ID,                 0x19
.definelabel ROLLARROW3_ID,                 0x1A
.definelabel ROLLARROW_SFX_SUMMON,          0x01DB
.definelabel ROLLARROW_SFX_FIRE,            0x01DC
.definelabel ROLLARROW_PROJECTILE_TAG,       0x57524152 // "RARW"

.org CHIP_DATA + ROLLARROW1_ID * 0x2C
    .db 0x00,0x05,0x13,0xFF             // A, F, T
    .db 0x00,0x01,0x0A,0x00             // rarity 1, Null, Standard
    .db 0x20,0x43,0x94,0x1B
    .db 0x0E,0x0A,0x04,0x00             // shared Navi dispatch 0x0E
    .dw 0x00000000
    .db 0x00,0x17,0x00,0x0A
    .dh 0x01BC,50,0x0017
    .db 0x01,0xFF
    .dw RollArrowIcon1,RollArrowImage,RollArrowPalette1

    .db 0x03,0x11,0x16,0xFF             // D, R, W
    .db 0x00,0x02,0x0A,0x00             // rarity 2, Null, Standard
    .db 0x26,0x43,0x94,0x1B
    .db 0x0E,0x0A,0x04,0x00
    .dw 0x00000000
    .db 0x00,0x18,0x00,0x0A
    .dh 0x01BD,70,0x0018
    .db 0x01,0xFF
    .dw RollArrowIcon2,RollArrowImage,RollArrowPalette2

    .db 0x10,0x18,0x19,0xFF             // Q, Y, Z
    .db 0x00,0x03,0x0A,0x00             // rarity 3, Null, Standard
    .db 0x2C,0x43,0x94,0x1B
    .db 0x0E,0x0A,0x04,0x00
    .dw 0x00000000
    .db 0x00,0x19,0x00,0x0A
    .dh 0x01BE,90,0x0019
    .db 0x01,0xFF
    .dw RollArrowIcon3,RollArrowImage,RollArrowPalette3

.definelabel TYPE1_ROLL_ENTRY,              0x08003DE8 // released type-1 slot 0x53
.definelabel SPRITE_PTR_ROLL,               0x08031F68 // released group 0x0C/index 0x5A
.definelabel SPRITE_PTR_ROLL_ARROW,         0x08031F6C // released group 0x0C/index 0x5B

.org TYPE1_ROLL_ENTRY
.dw RollArrowActorMain + 1
.org SPRITE_PTR_ROLL
.dw RollArrowActorSprite
.org SPRITE_PTR_ROLL_ARROW
.dw RollArrowProjectileSprite

.autoregion
.align 4
RollArrowCodeStart:

// r0/r1 are the summon panel, r2 is the time-freeze parameter, r4 is the
// record parameter, r5 is the controller, r6 is the packed attack word, and
// r7 is the completion byte.
RollArrowTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r4-r5}
    mov r0,0x53
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r1-r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    strb r3,[r0,0x0E]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    str r5,[r0,0x4C]
    str r6,[r0,0x2C]
    str r7,[r0,0x64]
    mov r1,1
    strb r1,[r7]
@@done:
    pop {pc}
    .pool

// ----- Roll actor ----------------------------------------------------------

RollArrowActorMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    bl RollArrowActorDestroy
    pop {pc}
@@init:
    bl RollArrowActorInit
    b @@animate
@@update:
    bl RollArrowActorUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

RollArrowActorInit:
    push {lr}
    mov r0,0x80
    mov r1,0x0C
    mov r2,0x5A
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
    mov r0,0                           // native base Roll palette
    EngineCall SPRITE_SET_PALETTE
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    mov r0,0
    strb r0,[r5,0x09]
    strb r0,[r5,0x0B]
    bl RollArrowPlaySummonSound
    pop {pc}
    .pool

RollArrowActorUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq RollArrowActorWait
    cmp r0,4
    beq RollArrowActorFire
    b RollArrowActorExit

RollArrowActorWait:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,0
    bl RollArrowActorSetAnimation
    mov r0,20
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strb r0,[r5,0x0B]
@@done:
    pop {pc}

RollArrowActorFire:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@wait
    mov r0,7                           // native RollArrow bow/fire animation
    bl RollArrowActorSetAnimation
    bl RollArrowPlayFireSound
    bl RollArrowActorSpawnProjectile
    mov r0,6                           // native post-shot hold
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@wait:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strb r0,[r5,0x0B]
@@done:
    pop {pc}

RollArrowActorExit:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    beq @@post_shot
    cmp r0,4
    beq @@post_shot_tick

    // Native Roll finishes with animation 4 for five frames, without moving.
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    mov r0,8
    str r0,[r5,0x08]
    b @@done

@@post_shot:
    mov r0,0
    bl RollArrowActorSetAnimation
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
    b @@done

@@post_shot_tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,4
    bl RollArrowActorSetAnimation
    mov r0,5
    strh r0,[r5,0x20]
    mov r0,8
    strb r0,[r5,0x0B]
@@done:
    pop {pc}
    .pool

RollArrowActorDestroy:
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

RollArrowActorSetAnimation:
    push {lr}
    strb r0,[r5,0x10]
    mov r1,0xFF
    strb r1,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    pop {pc}

RollArrowPlaySummonSound:
    push {lr}
    mov r0,ROLLARROW_SFX_SUMMON >> 1
    lsl r0,r0,1
    add r0,1
    EngineCall PLAY_SOUND
    pop {pc}

RollArrowPlayFireSound:
    push {lr}
    mov r0,ROLLARROW_SFX_FIRE >> 1
    lsl r0,r0,1
    EngineCall PLAY_SOUND
    pop {pc}

RollArrowActorSpawnProjectile:
    push {r4,r6,lr}
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    ldrb r2,[r5,0x0E]
    ldr r6,[r5,0x2C]
    bl RollArrowProjectileSpawn
    pop {r4,r6,pc}
    .pool

// ----- Straight RollArrow projectile --------------------------------------

RollArrowProjectileSpawn:
    push {r4,r6,lr}
    push {r0-r2,r5}
    // Blue Moon's generic heart-arrow is a type-3 battle attack. Keeping it
    // in that class is required by BN6's counter-hit ownership path; a
    // collision-bearing type-1 object corrupts that path when the hit is a
    // counter. Share SearchMan's tagged type-3 slot instead.
    mov r0,0x2C
    mov r4,8
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
    ldr r0,=ROLLARROW_PROJECTILE_TAG
    str r0,[r4,0x78]

    // Blue Moon spawns the heart-arrow at Roll's exact bow origin:
    // facing * 8 px in X, one pixel above her Y origin, and 36 px in Z.
    EngineCall OBJECT_FRONT_DIRECTION
    mov r1,8
    mul r0,r1
    lsl r0,r0,16
    ldr r1,[r5,0x34]
    add r0,r0,r1
    str r0,[r4,0x34]
    ldrh r0,[r5,0x3A]
    sub r0,1
    lsl r0,r0,16
    str r0,[r4,0x38]
    mov r0,0x24
    lsl r0,r0,16
    str r0,[r4,0x3C]

    ldrb r0,[r4]
    mov r1,0x10
    orr r0,r1
    strb r0,[r4]
@@done:
    pop {r4,r6,pc}

RollArrowProjectileMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl RollArrowProjectileInit
    b @@animate
@@update:
    bl RollArrowProjectileUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

RollArrowProjectileInit:
    push {r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
    mov r0,0x80
    mov r1,0x0C
    mov r2,0x5B
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
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free
    mov r1,8                           // native RollArrow collision type
    mov r2,5
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r0,9                           // native heart-arrow impact
    EngineCall COLLISION_SET_HIT_EFFECT
    EngineCall COLLISION_PRESENT
    mov r0,8                           // native config permits eight panels
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

RollArrowProjectileUpdate:
    push {r4,r6,r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne RollArrowProjectileFreeCollision

    // Update the arrow's native collision object before moving it. This
    // creates an impact only when something was actually contacted and keeps
    // empty panels from producing unrelated BN6 damage effects.
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    ldr r7,[r5,0x54]
    ldr r0,[r7,0x70]
    cmp r0,0
    bne RollArrowProjectileFreeCollision

    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    ldrb r0,[r5,0x14]
    ldrb r1,[r5,0x13]
    push {r0-r1,r4}
    EngineCall PANEL_IS_VALID_XY
    cmp r0,0
    pop {r0-r1,r4}
    beq RollArrowProjectileFreeCollision

    EngineCall PANEL_TO_COORDS
    mov r2,r0                          // fixed-point center of the target panel
    mov r0,r4
    ldr r1,=0x00070000                 // native heart-arrow speed: 7 px/frame
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
    ble RollArrowProjectileFreeCollision
    EngineCall OBJECT_FRONT_DIRECTION
    ldrb r1,[r5,0x14]
    add r0,r0,r1
    strb r0,[r5,0x14]
@@done:
    EngineCall COLLISION_PRESENT
    pop {r4,r6,r7,pc}
    .pool

RollArrowProjectileFreeCollision:
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
RollArrowProjectileFree:
    EngineCall OBJECT_FREE
    pop {r4,r6,r7,pc}
    .pool

RollArrowCodeEnd:
.endautoregion

// Blue Moon menu art and runtime-confirmed Roll/heart-arrow archives.
.autoregion
.align 4
RollArrowIcon1:
.incbin "build/rollarrow-icon-1.bin"
.endautoregion
.autoregion
.align 4
RollArrowIcon2:
.incbin "build/rollarrow-icon-2.bin"
.endautoregion
.autoregion
.align 4
RollArrowIcon3:
.incbin "build/rollarrow-icon-3.bin"
.endautoregion
.autoregion
.align 4
RollArrowImage:
.incbin "build/rollarrow-image.bin"
.endautoregion
.autoregion
.align 4
RollArrowPalette1:
.incbin "build/rollarrow-pal-1.bin"
.endautoregion
.autoregion
.align 4
RollArrowPalette2:
.incbin "build/rollarrow-pal-2.bin"
.endautoregion
.autoregion
.align 4
RollArrowPalette3:
.incbin "build/rollarrow-pal-3.bin"
.endautoregion
.autoregion
.align 4
RollArrowActorSprite:
.incbin "build/rollarrow-actor-sprite.bin"
.endautoregion
.autoregion
.align 4
RollArrowProjectileSprite:
.incbin "build/rollarrow-projectile-sprite.bin"
.endautoregion

// Exact Blue Moon RollArrow summon (SFX 0xB0) and firing (SFX 0x132) cues.
.autoregion
.align 4
RollArrowAudioStart:
RollArrowSummonSong:
    .db 0x01,0x00,0x40,0x00
    .dw RollArrowSummonVoicegroup
    .dw RollArrowSummonTrack
RollArrowSummonVoicegroup:
    .db 0x08,0x3C,0x00,0x00
    .dw RollArrowSummonSample
    .db 0xFF,0x00,0xFF,0x00
RollArrowSummonTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x7F,0xE6,0x3C,0x7F,0x97,0xB1
.align 4
RollArrowSummonSample:
    .incbin "build/rollarrow-summon-sample.bin"
.align 4
RollArrowFireSong:
    .db 0x01,0x00,0x40,0x00
    .dw RollArrowFireVoicegroup
    .dw RollArrowFireTrack
RollArrowFireVoicegroup:
    .db 0x08,0x3C,0x00,0x00
    .dw RollArrowFireSample
    .db 0xFF,0x00,0xFF,0x00
RollArrowFireTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x7F,0xE1,0x3C,0x7F,0x92,0xB1
.align 4
RollArrowFireSample:
    .incbin "build/rollarrow-fire-sample.bin"
RollArrowAudioEnd:
.endautoregion
