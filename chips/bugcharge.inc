// ---------------------------------------------------------------------------
// BugCharge replaces the former SignalRed/BugRSword slot. This is BN5's
// bug-count version: it always fires one 200-damage Gospel shot, adds one shot
// for each active BN6 battle-bug type, then clears the bugs it consumed.

.definelabel BUGCHARGE_ID,                    0x131
.definelabel BUGCHARGE_SFX_CHARGE,            0x01E0 // exact BN5 SFX 0x0094
.definelabel BUGCHARGE_SFX_FIRE,              0x01E1 // exact BN5 SFX 0x00FF
.definelabel BUGCHARGE_CONTROLLER_TAG,        0x43434742 // "BGCC"
.definelabel BUGCHARGE_CHARGE_HEAD_TAG,       0x4F434742 // "BGCO"

// B code, BN5 rarity/MB cost, Null element, Standard class, and 200 power.
// Preserve BugRSword's physical library position, version-only availability
// flag, and gate byte; the final ROM pass replaces its sort key. BugCharge and SignalRed
// share the former BugRSword family route; record byte 0x0A distinguishes
// their launchers.
.org CHIP_DATA + BUGCHARGE_ID * 0x2C
    .db 0x01,0xFF,0xFF,0xFF             // B
    .db 0x00,0x04,0x0A,0x00             // rarity 4, Null, Standard
    .db 0x4D
.if FALZAR
    .db 0x01
.else
    .db 0x41
.endif
    .db 0x8A,0x15
    .db 0x26,0x01,0x04,0x00             // shared BugRSword route
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x05,0x14,0x00
    .dh 0x0197,200,0x0131
    .db 0x01,0xFF
.if FALZAR
    .dw 0x0872C594,0x08721B34,0x087255B4
.else
    .dw BugChargeIcon,BugChargeImage,BugChargePalette
.endif

.autoregion
.align 4
BugChargeCodeStart:

BugChargeTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x84
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

// The invisible controller and BN5 charge-head visual share the former
// BugRSword type-4 slot with SignalRed and FolderBack. Gospel shots use
// SearchMan's tagged type-3 hit slot so their collision follows BN6's native
// counter-hit ownership path.
BugChargeSharedMain:
    ldr r0,[r5,0x78]
    ldr r1,=BUGCHARGE_CHARGE_HEAD_TAG
    cmp r0,r1
    bne @@controller
    ldr r1,=BugChargeChargeHeadMain + 1
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
// initial 200-damage Gospel shot. BN5's nine types produce ten shots; BN6's
// additional Custom bug field permits eleven.
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
    bl BugChargeSpawnGospel
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
    strh r0,[r5,0x20]
    bl BugChargeSpawnChargeHead
    bl BugChargePlayChargeSound
    mov r0,39                          // native 40-frame charge hold
    strh r0,[r5,0x22]
    mov r0,4
    strb r0,[r5,0x0B]
@@done:
    pop {r4,r6,pc}
    .pool

// Actual BN5 gameplay counts nine battle-property types. This BN6 port also
// consumes BN6-only field 0x63, completing the property set cleared by BN6
// BugFix. Each nonzero property counts as one active bug type, regardless of
// its BN6 severity. The repeat counter has a native base of one.
BugChargeCountAndClearBugs:
    push {r4,r6,lr}
    ldrb r0,[r5,0x16]
    EngineCall PLAYER_PROPERTIES_FOR_SIDE
    mov r6,r0
    mov r4,1

    mov r1,0x31
    ldrb r0,[r6,r1]
    cmp r0,0
    beq @@general_bytes
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
    cmp r0,0
    beq @@done
    add r4,1
    mov r0,0
    strb r0,[r6,r1]
@@done:
    EngineCall BUGFIX_CLEAR_RUNTIME_STATE
    mov r0,r4
    pop {r4,r6,pc}
    .pool

BugChargeByteProperties:
    .db 0x13,0x14,0x16,0x19,0x18,0x1A,0x63,0xFF
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

// The native chip creates one group 0x0C/index 0x43 Gospel apparition at the
// user when charging begins. BN5 gives that object a fixed lifetime, which can
// expire before a high-bug shot sequence finishes. Keep the native position
// and animation, but derive this port's lifetime from the controller's shot
// count so it remains visible through the final shot and 30-frame recovery.
BugChargeSpawnChargeHead:
    push {r4,r7,lr}
    ldrb r0,[r5,0x16]
    EngineCall PLAYER_OBJECT_FOR_SIDE
    mov r7,r0
    cmp r7,0
    beq @@done
    push {r5}
    mov r0,0x84
    EngineCall SPAWN_TYPE4
    mov r4,r5
    pop {r5}
    cmp r4,0
    beq @@done
    ldrh r1,[r5,0x20]                // hold through the full controller effect
    mov r0,15
    mul r0,r1
    add r0,55
    strh r0,[r4,0x22]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    str r7,[r4,0x4C]
    ldr r0,=BUGCHARGE_CHARGE_HEAD_TAG
    str r0,[r4,0x78]
@@done:
    pop {r4,r7,pc}
    .pool

BugChargeChargeHeadMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl BugChargeChargeHeadInit
    b @@animate
@@update:
    bl BugChargeChargeHeadUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

BugChargeChargeHeadInit:
    push {lr}
    mov r0,0x80
    mov r1,0x0C
    mov r2,BUGCHARGE_GOSPEL_SPRITE_INDEX
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,1                          // native stationary-head animation
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
    bl BugChargeChargeHeadSetPosition
    pop {pc}
    .pool

BugChargeChargeHeadUpdate:
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
    bl BugChargeChargeHeadSetPosition
    pop {pc}
@@free:
    EngineCall OBJECT_FREE
    pop {pc}

BugChargeChargeHeadSetPosition:
    push {r4,lr}
    ldr r3,[r5,0x4C]
    ldr r0,[r3,0x34]
    str r0,[r5,0x34]
    ldr r0,[r3,0x38]
    str r0,[r5,0x38]
    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    mov r0,24                         // exact BN5 stationary-head X offset
    mul r0,r4
    lsl r0,r0,16
    ldr r1,[r5,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]
    mov r0,0x17                       // native charge-apparition Z
    lsl r0,r0,16
    str r0,[r5,0x3C]
    pop {r4,pc}
    .pool

// BN5's attack-family kind 1 loads the same group 0x0C/index 0x43 archive for
// a fast moving Gospel head for every firing pulse. BN5 reuses its attack
// object between pulses; this tagged BN6 route creates the equivalent visual
// anew so every bug-derived shot visibly launches a head.
BugChargeSpawnGospel:
    push {r4,r6,r7,lr}
    EngineCall OBJECT_FRONT_DIRECTION
    mov r6,r0
    ldrb r0,[r5,0x12]
    add r0,r0,r6                     // native shot begins on the front panel
    ldrb r1,[r5,0x13]
    push {r0-r1,r5}
    mov r0,0x2C
    mov r4,25
    EngineCall SPAWN_TYPE3
    mov r4,r5
    pop {r0-r1,r5}
    cmp r4,0
    beq @@done
    strb r0,[r4,0x12]
    strb r1,[r4,0x13]
    ldrh r0,[r5,0x16]
    strh r0,[r4,0x16]
    ldr r0,[r5,0x2C]
    str r0,[r4,0x2C]
    str r5,[r4,0x4C]
    ldr r0,=BUGCHARGE_GOSPEL_TAG
    str r0,[r4,0x78]
    ldrb r0,[r4]
    mov r1,0x10
    orr r0,r1
    strb r0,[r4]
@@done:
    pop {r4,r6,r7,pc}
    .pool

// Apply the same kind-1 contact to each valid panel in the user's row. The
// tagged visual object above independently supplies the Gospel-head animation.
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

// ----- Tagged type-3 row contacts and Gospel-head visuals ---------------

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
    // Translate BN5's attack-family kind-1 contact to the corresponding BN6
    // generic-attack collision IDs. BN5's numeric IDs describe different
    // handlers when passed directly to BN6.
    mov r1,0x0A
    mov r2,5
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r1,1
    strb r1,[r7,1]
    mov r0,1
    EngineCall COLLISION_SET_HIT_EFFECT
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

BugChargeGospelMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl BugChargeGospelInit
    b @@animate
@@update:
    bl BugChargeGospelUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

BugChargeGospelInit:
    push {r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@free
    EngineCall OBJECT_SET_COORDS
    mov r0,0x14                       // exact BN5 moving-head Z origin
    lsl r0,r0,16
    str r0,[r5,0x3C]
    mov r0,0x80
    mov r1,0x0C
    mov r2,BUGCHARGE_GOSPEL_SPRITE_INDEX
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0                          // native moving-head animation
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
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
    mov r1,4                          // native BN5/BN6 kind-1 collision
    mov r2,5
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r0,6                          // native kind-1 Gospel hit behavior
    EngineCall COLLISION_SET_HIT_EFFECT
    mov r0,0
    EngineCall COLLISION_SET_EXTENDED_EFFECT
    EngineCall COLLISION_PRESENT
    mov r0,8                          // enough panels to leave either edge
    strb r0,[r5,0x06]
    mov r0,4
    str r0,[r5,0x08]
    // BN5's generic attack object performs its first ten-pixel movement on
    // the initialization tick. Doing the same places the first rendered
    // frame 26 pixels in front of the user's action-pose coordinate.
    bl BugChargeGospelUpdate
    pop {r7,pc}
@@free:
    EngineCall OBJECT_FREE
    pop {r7,pc}
    .pool

BugChargeGospelUpdate:
    push {r4,r6,r7,lr}
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne BugChargeGospelFreeCollision
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    ldr r7,[r5,0x54]
    ldr r0,[r7,0x70]
    cmp r0,0
    bne BugChargeGospelFreeCollision

    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    mov r0,10                          // native ten pixels per frame
    mul r0,r4
    lsl r0,r0,16
    ldr r1,[r5,0x34]
    add r6,r0,r1
    str r6,[r5,0x34]

    // The native generic-attack path derives the current panel from the
    // moving coordinate every frame. This changes the collision region at
    // the panel boundary (halfway between centers), not one center late.
    EngineCall OBJECT_UPDATE_PANEL
    EngineCall COLLISION_UPDATE_PANEL
    EngineCall PANEL_IS_VALID_OBJECT
    cmp r0,0
    beq BugChargeGospelFreeCollision

    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    EngineCall PANEL_TO_COORDS
    cmp r6,r0
    bne @@present
    ldrb r0,[r5,0x06]
    sub r0,1
    strb r0,[r5,0x06]
    ble BugChargeGospelFreeCollision
@@present:
    EngineCall COLLISION_PRESENT
    pop {r4,r6,r7,pc}
    .pool

BugChargeGospelFreeCollision:
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
BugChargeGospelFree:
    EngineCall OBJECT_FREE
    pop {r4,r6,r7,pc}
    .pool

BugChargeCodeEnd:
.endautoregion

// Exact BN5 Colonel BugCharge menu art and Gospel-head archive.
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
BugChargeGospelSprite:
    .incbin "build/bugcharge-gospel-sprite.bin"
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
