.definelabel SEARCHMAN_ID,           0x107

// ---------------------------------------------------------------------------
// Replace CircusMan's three chip records.

.org CHIP_DATA + SEARCHMAN_ID * 0x2C
    // SerchMan: S and *
    .db 0x12,0x1A,0xFF,0xFF
    .db 0x00,0x02,0x06,0x01
    .db 0x2A,0x47,0x8A,0x1B
    .db 0x0E,0x1F,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x1C,0x00,0x00
    .dh 0x0073,20,0x0107
    .db 0x01,0xFF
    .dw SearchManIcon,SearchManImage,SearchManPaletteBase

    // SerchMnEX: S
    .db 0x12,0xFF,0xFF,0xFF
    .db 0x00,0x03,0x06,0x01
    .db 0x40,0x47,0x8A,0x1B
    .db 0x0E,0x00,0x00,0x00
    .db 0x03,0x00,0x00,0x00
    .db 0x00,0x1D,0x00,0x00
    .dh 0x0074,40,0x0108
    .db 0x01,0xFF
    .dw SearchManIcon,SearchManImage,SearchManPaletteEx

    // SerchMnSP: S
    .db 0x12,0xFF,0xFF,0xFF
    .db 0x00,0x04,0x06,0x01
    .db 0x56,0x47,0x8A,0x1B
    .db 0x0E,0x00,0x00,0x00
    .db 0x04,0x00,0x00,0x00
    .db 0x00,0x1E,0x00,0x00
    .dh 0x0075,75,0x0109
    .db 0x01,0xFF
    .dw SearchManIcon,SearchManImage,SearchManPaletteSp

.definelabel TIMEFREEZE_DISPATCH_E,  0x0802CD94

.definelabel TYPE1_SEARCHMAN_ENTRY,  0x08003D60 // free type-1 slot 0x31
.definelabel TYPE3_SEARCH_HIT_ENTRY, 0x08003F74 // free type-3 slot 0x2c
.definelabel TYPE4_RETICLE_ENTRY,    0x08004324 // free type-4 slot 0x17

// ---------------------------------------------------------------------------
// Hook the three cooperating native object classes.

.org TIMEFREEZE_DISPATCH_E
.dw SearchManTimeFreezeSpawn + 1

.org TYPE1_SEARCHMAN_ENTRY
.dw SearchManObjectMain + 1

.org TYPE3_SEARCH_HIT_ENTRY
.dw SearchManHitMain + 1

.org TYPE4_RETICLE_ENTRY
.dw SearchManReticleMain + 1

// ---------------------------------------------------------------------------
// Runtime code. This is a direct state-for-state port of BN5's type-1 0x31
// summon, type-4 0x43 reticle, and type-3 0x57 hit object. BN6 changed the
// collision flag word from +0x68 to +0x70; the translated hit object uses the
// BN6 offset and BN6 collision API entry points.

.autoregion
.align 4
SearchManCodeStart:

SearchManTimeFreezeSpawn:
    // r6 is the packed attack word, not the chip ID: its high half contains
    // record byte 0x0A. RollArrow retains BN4's 0x94 marker while SearchMan
    // uses 0x8A, so the shared subfamily can be routed without using damage.
    push {r0}
    mov r0,r6
    lsr r0,r0,16
    cmp r0,0x94
    pop {r0}
    bne @@searchman
    ldr r3,=RollArrowTimeFreezeSpawn + 1
    bx r3
@@searchman:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x31
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r1-r3,r5}
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

// ----- SearchMan summon/controller (BN5 type-1 0x31) -----------------------

SearchManObjectMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    bl SearchManObjectDestroy
    pop {pc}
@@init:
    bl SearchManObjectInit
    pop {pc}
@@update:
    bl SearchManObjectUpdate
    pop {pc}

SearchManObjectInit:
    push {lr}
    mov r0,0x80
    mov r1,0x08
    mov r2,SEARCHMAN_BATTLE_SPRITE_INDEX
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

    // Base, EX, and SP all use SearchMan's base in-battle sprite palette.
    mov r0,0
    EngineCall SPRITE_SET_PALETTE
    ldr r0,=0x00007FFF
    EngineCall SPRITE_SET_SCALE
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    bl SearchManObjectUpdate
    pop {pc}
    .pool

SearchManObjectUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq SearchManAppearPhase
    cmp r0,4
    beq SearchManAttackPhase
    bl SearchManExitPhase
SearchManObjectAnimate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    bl SearchManPulseScale
    pop {pc}

SearchManAppearPhase:
    ldrb r0,[r5,0x0A]
    cmp r0,0
    beq SearchManAppear
    bl SearchManWaitForPanel
    b SearchManObjectAnimate

SearchManAppear:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,0x1F
    strb r0,[r5,0x19]
    mov r0,0x94
    EngineCall PLAY_SOUND
    mov r0,0
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
    b SearchManObjectAnimate
@@tick:
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldrh r0,[r5,0x20]
    add r0,1
    strh r0,[r5,0x20]
    lsr r0,r0,1
    bcc @@visible
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
@@visible:
    ldrb r0,[r5,0x19]
    sub r0,2
    strb r0,[r5,0x19]
    bgt SearchManObjectAnimate
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,0
    strb r0,[r5,0x19]
    mov r0,4
    strh r0,[r5,0x0A]
    b SearchManObjectAnimate

SearchManWaitForPanel:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,20
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
    b SearchManObjectAnimate
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt SearchManObjectAnimate
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    mov r2,0x10
    mov r3,0
    EngineCall PANEL_HAS_FLAGS
    cmp r0,0
    bne @@ready
    mov r0,8
    str r0,[r5,0x08]
    b SearchManObjectAnimate
@@ready:
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    b SearchManObjectAnimate
    .pool

SearchManAttackPhase:
    ldrb r0,[r5,0x0A]
    cmp r0,0
    beq SearchManStartReticle
    cmp r0,4
    beq SearchManFireShots
    bl SearchManShotCooldown
    b SearchManObjectAnimate

SearchManStartReticle:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@wait
    mov r0,0x10
    strb r0,[r5,0x10]
    mov r0,0
    strb r0,[r5,0x05]
    strh r0,[r5,0x14]
    ldr r0,[r5,0x4C]
    mov r2,0
    ldrb r4,[r5,0x04]
    bl SearchManSpawnReticle
    mov r0,4
    strb r0,[r5,0x0B]
    b SearchManObjectAnimate
@@wait:
    ldrb r0,[r5,0x14]
    cmp r0,0
    beq SearchManObjectAnimate
    mov r0,4
    strh r0,[r5,0x0A]
    b SearchManObjectAnimate

SearchManFireShots:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne SearchManFireTick
    mov r0,5
    strb r0,[r5,0x06]
    mov r0,0x11
    strb r0,[r5,0x10]
SearchManNextShot:
    mov r0,0x12
    strb r0,[r5,0x11]
    mov r0,10
    strh r0,[r5,0x20]
    mov r0,0xB9
    EngineCall PLAY_SOUND
    mov r0,0
    strb r0,[r5,0x07]
    mov r0,4
    strb r0,[r5,0x0B]
    b SearchManObjectAnimate
SearchManFireTick:
    ldrh r0,[r5,0x20]
    cmp r0,7
    bne @@countdown
    ldrb r0,[r5,0x07]
    cmp r0,0
    bne @@countdown
    mov r0,1
    strb r0,[r5,0x07]
    mov r2,0
    ldrb r0,[r5,0x05]
    cmp r0,0
    beq @@spawn
    ldrb r0,[r5,0x06]
    cmp r0,1
    bne @@spawn
    mov r2,1
@@spawn:
    bl SearchManSpawnHit
@@countdown:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    ble @@shot_finished
    b SearchManObjectAnimate
@@shot_finished:
    ldrb r0,[r5,0x06]
    sub r0,1
    strb r0,[r5,0x06]
    bgt SearchManNextShot
    mov r0,8
    strh r0,[r5,0x0A]
    b SearchManObjectAnimate
    .pool

SearchManShotCooldown:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,0
    strb r0,[r5,0x10]
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
    b SearchManObjectAnimate
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    ble @@cooldown_finished
    b SearchManObjectAnimate
@@cooldown_finished:
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    b SearchManObjectAnimate

SearchManExitPhase:
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@tick
    mov r0,4
    strh r0,[r5,0x10]
    mov r0,5
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0A]
    bx lr
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    mov r0,8
    str r0,[r5,0x08]
@@done:
    bx lr

SearchManObjectDestroy:
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

SearchManSpawnHit:
    push {r4,r6,lr}
    ldrb r0,[r5,0x14]
    ldrb r1,[r5,0x15]
    mov r4,25
    cmp r2,0
    beq @@normal
    mov r4,29
@@normal:
    ldr r6,[r5,0x2C]
    bl SearchManHitSpawn
    pop {r4,r6,pc}

SearchManPulseScale:
    push {lr}
    ldrb r1,[r5,0x19]
    lsl r0,r1,5
    add r0,r0,r1
    lsl r0,r0,5
    add r0,r0,r1
    EngineCall SPRITE_SET_SCALE
    pop {pc}
    .pool

// ----- SearchMan reticle (BN5 type-4 0x43) ---------------------------------

SearchManSpawnReticle:
    push {lr}
    push {r0,r2,r5}
    mov r0,0x17
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1,r2,r5}
    cmp r0,0
    beq @@done
    str r1,[r0,0x60]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    str r2,[r0,0x64]
    str r5,[r0,0x4C]
@@done:
    pop {pc}
    .pool

SearchManReticleMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl SearchManReticleInit
    pop {pc}
@@update:
    bl SearchManReticleUpdate
    pop {pc}

SearchManReticleInit:
    push {r7,lr}
    mov r1,0x10
    mov r2,SEARCHMAN_RETICLE_SPRITE_INDEX
    ldr r0,[r5,0x64]
    cmp r0,0
    beq @@sprite
    mov r2,SEARCHMAN_RETICLE_ALT_SPRITE_INDEX
@@sprite:
    mov r0,0x80
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    mov r0,0
    EngineCall SPRITE_SET_PALETTE
    bl SearchManReticleSetInitialPanel
    EngineCall OBJECT_SET_COORDS
    mov r0,1
    strb r0,[r5,0x05]
    strb r0,[r5,0x07]
    ldr r0,=0x12C
    strh r0,[r5,0x22]
    ldr r0,[r5,0x60]
    ldr r0,[r0,0x58]
    add r0,0x2C
    str r0,[r5,0x58]
    mov r0,0
    strb r0,[r5,0x06]
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,0
    str r0,[r5,0x3C]
    mov r0,4
    str r0,[r5,0x08]
    strb r0,[r5,0x09]
    bl SearchManReticleUpdate
    pop {r7,pc}
    .pool

SearchManReticleUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@step
    cmp r0,4
    beq @@scan
    bl SearchManReticleLocked
    b SearchManReticleFinishUpdate
@@step:
    bl SearchManReticleStep
    b SearchManReticleFinishUpdate
@@scan:
    bl SearchManReticleScan
SearchManReticleFinishUpdate:
    ldrh r0,[r5,0x22]
    cmp r0,0
    beq @@animate
    sub r0,1
    strh r0,[r5,0x22]
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

SearchManReticleStep:
    push {lr}
    bl SearchManReticleChangeRow
    cmp r0,0
    beq @@moved
    bl SearchManReticleChangeColumn
@@moved:
    EngineCall OBJECT_SET_COORDS
    ldr r0,=0x10E
    EngineCall PLAY_SOUND
    mov r0,4
    strb r0,[r5,0x09]
    bl SearchManReticleScan
    pop {pc}
    .pool

SearchManReticleChangeRow:
    ldrb r2,[r5,0x13]
    ldrb r0,[r5,0x05]
    cmp r0,0
    beq @@up
    add r2,1
    cmp r2,3
    ble @@store
    mov r0,0
    strb r0,[r5,0x05]
    b @@edge
@@up:
    sub r2,1
    cmp r2,1
    bge @@store
    mov r0,1
    strb r0,[r5,0x05]
    b @@edge
@@store:
    strb r2,[r5,0x13]
    mov r0,0
    bx lr
@@edge:
    mov r0,1
    bx lr

SearchManReticleChangeColumn:
    push {r4,lr}
    mov r4,0
@@try:
    EngineCall OBJECT_FRONT_DIRECTION
    ldrb r2,[r5,0x12]
    ldrb r1,[r5,0x07]
    cmp r1,0
    bne @@direction
    neg r0,r0
@@direction:
    add r2,r2,r0
    ldrb r3,[r5,0x14]
    cmp r2,r3
    beq @@bounce
    ldrb r3,[r5,0x15]
    cmp r2,r3
    beq @@bounce
    b @@store
@@bounce:
    cmp r4,0
    bne @@done
    mov r4,1
    eor r1,r4
    strb r1,[r5,0x07]
    b @@try
@@store:
    strb r2,[r5,0x12]
@@done:
    pop {r4,pc}
    .pool

SearchManReticleScan:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@input
    mov r0,6
    ldrb r1,[r5,0x04]
    cmp r1,0
    beq @@timer
    mov r0,4
@@timer:
    strh r0,[r5,0x20]
    strb r0,[r5,0x0A]
@@input:
    mov r0,1
    bl SearchManReticleKeyPressed
    beq @@a_pressed
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,8
    ldrh r1,[r5,0x22]
    cmp r1,0
    beq @@next_state
    mov r0,0
@@next_state:
    strb r0,[r5,0x09]
    mov r0,0
    strb r0,[r5,0x0A]
    b @@done
@@a_pressed:
    mov r0,2
    bl SearchManReticleKeyPressed
    bne @@lock
    mov r0,0x8B
    EngineCall PLAY_SOUND
    ldr r0,[r5,0x4C]
    mov r1,1
    strb r1,[r0,0x05]
@@lock:
    mov r0,5
    strb r0,[r5,0x06]
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strb r0,[r5,0x0A]
@@done:
    pop {pc}
    .pool

SearchManReticleLocked:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@command_window
    mov r0,1
    strb r0,[r5,0x10]
    mov r0,0xBD
    EngineCall PLAY_SOUND
    bl SearchManReticleCommitTarget
    mov r0,50
    strh r0,[r5,0x20]
    strb r0,[r5,0x0A]
@@command_window:
    ldrb r0,[r5,0x06]
    cmp r0,0
    beq @@timer
    sub r0,1
    strb r0,[r5,0x06]
    mov r0,2
    bl SearchManReticleKeyPressed
    bne @@timer
    ldr r0,[r5,0x4C]
    mov r1,1
    strb r1,[r0,0x05]
@@timer:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,8
    strb r0,[r5,0x08]
@@done:
    pop {pc}
    .pool

SearchManReticleKeyPressed:
    ldr r1,[r5,0x58]
    ldrh r1,[r1]
    and r1,r0
    cmp r1,r0
    bx lr

SearchManReticleCommitTarget:
    ldr r1,[r5,0x4C]
    ldrb r0,[r5,0x12]
    strb r0,[r1,0x14]
    ldrb r0,[r5,0x13]
    strb r0,[r1,0x15]
    bx lr

SearchManReticleSetInitialPanel:
    push {lr}
    bl SearchManReticleFindBounds
    strb r0,[r5,0x12]
    push {r0,r1}
    EngineCall OBJECT_FRONT_DIRECTION
    mov r2,r0
    pop {r0,r1}
    add r1,r1,r2
    strb r1,[r5,0x15]
    neg r2,r2
    add r0,r0,r2
    strb r0,[r5,0x14]
    mov r1,1
    strb r1,[r5,0x13]
    pop {pc}
    .pool

SearchManReticleFindBounds:
    push {lr}
    sub sp,0x0C
    EngineCall OBJECT_FRONT_DIRECTION
    neg r0,r0
    str r0,[sp]
    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,5
    mul r0,r1
    neg r0,r0
    add r0,6
    str r0,[sp,4]
@@own_panel:
    ldr r0,[sp,4]
    ldrb r1,[r5,0x16]
    EngineCall PANEL_GET_PARAMETERS
    cmp r0,0
    bne @@own_found
    ldr r0,[sp,4]
    ldr r1,[sp]
    add r0,r0,r1
    str r0,[sp,4]
    sub r0,1
    cmp r0,5
    bls @@own_panel
@@own_found:
    ldr r0,[sp,4]
    str r0,[sp,8]
@@enemy_panel:
    ldr r0,[sp,4]
    ldrb r1,[r5,0x16]
    mov r2,1
    eor r1,r2
    EngineCall PANEL_GET_PARAMETERS
    cmp r0,3
    beq @@enemy_found
    ldr r0,[sp,4]
    ldr r1,[sp]
    add r0,r0,r1
    str r0,[sp,4]
    sub r0,1
    cmp r0,5
    bls @@enemy_panel
@@enemy_found:
    ldr r0,[sp]
    neg r0,r0
    ldr r1,[sp,4]
    add r0,r0,r1
    ldr r1,[sp,8]
    add sp,0x0C
    pop {pc}
    .pool

// ----- SearchMan hit object (BN5 type-3 0x57) ------------------------------

SearchManHitSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x2C
    EngineCall SPAWN_TYPE3
    mov r0,r5
    pop {r1-r3,r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    str r6,[r0,0x2C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    ldrb r1,[r0]
    mov r2,0x10
    orr r1,r2
    strb r1,[r0]
@@done:
    pop {pc}
    .pool

SearchManHitMain:
    ldr r0,[r5,0x78]
    ldr r1,=ROLLARROW_PROJECTILE_TAG
    cmp r0,r1
    bne @@bugcharge
    ldr r0,=RollArrowProjectileMain + 1
    bx r0
@@bugcharge:
    ldr r1,=BUGCHARGE_PROJECTILE_TAG
    cmp r0,r1
    bne @@bugcharge_gospel
    ldr r0,=BugChargeProjectileMain + 1
    bx r0
@@bugcharge_gospel:
    ldr r1,=BUGCHARGE_GOSPEL_TAG
    cmp r0,r1
    bne @@laser
    ldr r0,=BugChargeGospelMain + 1
    bx r0
@@laser:
    ldr r1,=0x4C484954                  // LaserMan's tagged type-3 row hit
    cmp r0,r1
    bne @@searchman
    ldr r0,=LaserManHitMain + 1
    bx r0
@@searchman:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl SearchManHitInit
    pop {pc}
@@update:
    bl SearchManHitUpdate
    pop {pc}

SearchManHitInit:
    push {lr}
    EngineCall PANEL_IS_VALID_OBJECT
    cmp r0,0
    beq @@free
    EngineCall OBJECT_SET_COORDS
    mov r0,0x10
    lsl r0,r0,16
    str r0,[r5,0x3C]
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free
    ldrb r1,[r5,0x04]
    mov r2,5
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r0,0
    ldrb r1,[r5,0x04]
    cmp r1,25
    beq @@effect
    mov r0,9
@@effect:
    EngineCall COLLISION_SET_HIT_EFFECT
    EngineCall COLLISION_PRESENT
    mov r0,4
    str r0,[r5,0x08]
    pop {pc}
@@free:
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

SearchManHitUpdate:
    push {lr}
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_SPAWN_EFFECT
    ldr r7,[r5,0x54]
    ldr r0,[r7,0x70]
    cmp r0,0
    bne @@finish
    EngineCall RNG_NEXT
    mov r1,2
    and r1,r0
    lsl r1,r1,8
    mov r4,7
    orr r4,r1
    mov r0,0x34
    add r0,r0,r5
    ldmia r0!,{r1-r3}
    mov r0,0x0F
    bl SearchManRandomizeImpact
    mov r0,0
    push {r5}
    EngineCall SPAWN_TYPE4
    pop {r5}
@@finish:
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

SearchManRandomizeImpact:
    push {r4,r6,r7,lr}
    push {r0-r3}
    EngineCall RNG_NEXT
    mov r7,r0
    pop {r0-r3}
    mov r4,r7
    and r4,r0
    lsr r6,r0,1
    sub r4,r4,r6
    lsl r4,r4,16
    add r1,r1,r4
    lsr r7,r7,16
    and r7,r0
    lsr r0,r0,1
    sub r7,r7,r0
    lsl r7,r7,16
    add r3,r3,r7
    pop {r4,r6,r7,pc}
    .pool
SearchManCodeEnd:
.endautoregion

// Menu assets, the complete actor archive, and both reticle archives from BN5.
.autoregion
.align 4
SearchManIcon:
.incbin "build/searchman-icon.bin"
.endautoregion

.autoregion
.align 4
SearchManImage:
.incbin "build/searchman-image.bin"
.endautoregion

.autoregion
.align 4
SearchManPaletteBase:
.incbin "build/searchman-pal-base.bin"
.endautoregion

.autoregion
.align 4
SearchManPaletteEx:
// BN5 base and EX use the same foreground colors (indices 0-12). Keep those
// SearchMan colors unchanged and give BN6 EX a distinct yellow background
// gradient in the background-only entries (indices 13-15).
.incbin "build/searchman-pal-base.bin",0,0x1A
.dh 0x03FF,0x0299,0x0190
.endautoregion

.autoregion
.align 4
SearchManPaletteSp:
// BN5 has a base and SP chip-art palette; there is no separate EX palette.
.incbin "build/searchman-pal-sp.bin"
.endautoregion

.autoregion
.align 4
SearchManBattleSprite:
.incbin "build/searchman-battle-sprite.bin"
.endautoregion

.autoregion
.align 4
SearchManReticleAltSprite:
.incbin "build/searchman-reticle-alt.bin"
.endautoregion

.autoregion
.align 4
SearchManReticleSprite:
.incbin "build/searchman-reticle.bin"
.endautoregion
