// ---------------------------------------------------------------------------
// DeathPhoenix replaces chip 0x134 in both versions. This is the BN5 phoenix
// actor, its strike controller, and the native saved-Navi replay tail
// translated to BN6. Gregar deliberately retains its original menu art.

.definelabel DEATHPHOENIX_ID,                 0x134

// Keep CrossDiv's released BN6 family/subfamily so only its one dispatch
// entry changes. DeathPhoenix is null-element and retains the Giga class.
.org CHIP_DATA + DEATHPHOENIX_ID * 0x2C
.db 0x03,0xFF,0xFF,0xFF
.db 0x00,0x04,0x0A,0x02
.db 0x5D
.if FALZAR
    .db 0x43
.else
    .db 0x03
.endif
.db 0x94,0x1B
.db 0x11,0x0A,0x04,0x00
.dw 0x00000000
.dw 0x10000000
.dh 0x0054,0x0096,DEATHPHOENIX_ID
.db 0x01,0xFF
.if FALZAR
    .dw DeathPhoenixIcon,DeathPhoenixImage,DeathPhoenixPalette
.endif

.definelabel DEATHPHOENIX_TIMEFREEZE_ENTRY,   0x0802CDA0
.definelabel TYPE1_DEATHPHOENIX_ENTRY,        0x08003CA0 // CrossDiv type-1 slot 1
.definelabel TYPE4_DEATH_STRIKE_ENTRY,        0x080044CC // released type-4 slot 0x81
.definelabel TYPE4_DEATH_FLAME_ENTRY,         0x080044D0 // released type-4 slot 0x82
.definelabel DEATH_SINE_TABLE,                0x080065E0

.definelabel DEATH_SPRITE_SPECIAL,            0x08003006
.definelabel DEATH_PLAYER_OBJECT,             0x0802E070 // legacy dead-code reference
.definelabel DEATH_PLAYER_CONTROL,            0x08013682 // legacy dead-code reference
.definelabel DEATH_PLAYER_EFFECT,             0x0800FFE4 // legacy dead-code reference
.definelabel DEATH_SAVED_NAVI,                0x0203C960
.definelabel DEATH_SAVED_NAVI_DISPATCH,       0x0802CD5C

.org DEATHPHOENIX_TIMEFREEZE_ENTRY
.dw DeathPhoenixTimeFreezeSpawn + 1
.org TYPE1_DEATHPHOENIX_ENTRY
.dw DeathPhoenixMain + 1
.org TYPE4_DEATH_STRIKE_ENTRY
.dw DeathStrikeMain + 1
.org TYPE4_DEATH_FLAME_ENTRY
.dw DeathFlameMain + 1

.autoregion
.align 4
DeathPhoenixCodeStart:

DeathPhoenixTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,1
    EngineCall SPAWN_TYPE1
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
    str r7,[r0,0x54]
    mov r1,1
    strb r1,[r7]
@@done:
    pop {pc}
    .pool

DeathPhoenixMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl DeathPhoenixInit
    b @@animate
@@update:
    bl DeathPhoenixUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

DeathPhoenixInit:
    push {lr}
    ldr r0,=0x00010C57
    EngineCall OBJECT_LOAD_NAVI_SPRITE
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP

    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,5
    mul r0,r1
    add r0,1
    strb r0,[r5,0x12]
    mov r0,3
    strb r0,[r5,0x13]
    EngineCall OBJECT_SET_COORDS
    mov r0,0
    str r0,[r5,0x3C]
    ldrh r0,[r5,0x3A]
    add r0,3
    strh r0,[r5,0x3A]
    ldrh r0,[r5,0x3E]
    add r0,3
    strh r0,[r5,0x3E]

    mov r1,1
    mov r2,0x6C
    add r2,r2,r5
    bl DeathBuildPanelList
    mov r1,2
    mov r2,0x70
    add r2,r2,r5
    bl DeathBuildPanelList
    mov r1,3
    mov r2,0x74
    add r2,r2,r5
    bl DeathBuildPanelList
    mov r0,4
    str r0,[r5,0x08]
    pop {pc}
    .pool

DeathPhoenixUpdate:
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq DeathPhoenixBegin
    cmp r0,4
    beq DeathPhoenixAppear
    cmp r0,8
    beq DeathPhoenixWaitBeforeStrikes
    cmp r0,12
    beq DeathPhoenixStrikes
    cmp r0,16
    beq @@after
    cmp r0,20
    beq @@disappear
    b DeathPhoenixRecycleUpdate
@@after:
    b DeathPhoenixWaitAfterStrikes
@@disappear:
    b DeathPhoenixDisappear

DeathPhoenixBegin:
    push {r4,r6,r7,lr}
    mov r0,1
    mov r1,1
    mov r2,0
    mov r3,0
    ldr r4,=0x2705FF80
    mov r6,0
    mov r7,0
.if FALZAR
    EngineCall 0x080C53A6
.else
    EngineCall 0x080C6C16
.endif
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    pop {r4,r6,r7,pc}
    .pool

DeathPhoenixAppear:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,0x94
    EngineCall PLAY_SOUND
    mov r0,0
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    add r0,1
    strh r0,[r5,0x20]
    cmp r0,16
    blt @@fade
    EngineCall SPRITE_PROPERTY_2CCE
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    pop {pc}
@@fade:
    EngineCall SPRITE_SET_BLEND_MODE
    pop {pc}

DeathPhoenixWaitBeforeStrikes:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,0
    strh r0,[r5,0x20]
    strh r0,[r5,0x22]
    str r0,[r5,0x60]
    str r0,[r5,0x78]
    mov r0,12
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    bx lr

DeathPhoenixStrikes:
    push {r4,r6,r7,lr}
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,12
    strh r0,[r5,0x20]

    ldr r0,[r5,0x60]
    cmp r0,0
    bne @@pattern_ready
    EngineCall BATTLE_RNG
    mov r1,3
    swi 6
    mov r0,3
    mul r0,r1
    ldr r1,=DeathStrikePatterns
    add r1,r1,r0
    str r1,[r5,0x68]
@@pattern_ready:
    ldr r0,[r5,0x68]
    ldr r1,[r5,0x60]
    ldrb r0,[r0,r1]
    sub r0,1
    lsl r0,r0,2
    add r0,0x6C
    ldr r1,[r5,0x78]
    add r0,r0,r1
    ldrb r0,[r5,r0]
    lsr r1,r0,4
    lsl r0,r0,29
    lsr r0,r0,29
    bl DeathSpawnStrikeForPanel

    ldr r0,[r5,0x60]
    add r0,1
    str r0,[r5,0x60]
    cmp r0,3
    blt @@count
    mov r0,0
    str r0,[r5,0x60]
    ldr r0,[r5,0x78]
    add r0,1
    str r0,[r5,0x78]
@@count:
    ldrh r0,[r5,0x22]
    add r0,1
    strh r0,[r5,0x22]
    cmp r0,12
    blt @@done
    mov r0,16
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {r4,r6,r7,pc}
    .pool

DeathStrikePatterns:
    .db 2,1,3, 1,2,3, 1,3,3, 2,1,2
    .align 4

DeathPhoenixWaitAfterStrikes:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,40
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,20
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

DeathPhoenixDisappear:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,16
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@fade
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    EngineCall SPRITE_PROPERTY_2CCE

    // BN5 only enters the recycle tail when the chip controller is still
    // active and a Navi use was recorded. BN6 stores that record at C960.
    ldr r0,[r5,0x4C]
    ldrh r0,[r0,0x24]
    cmp r0,0
    beq DeathPhoenixFinish
    ldr r1,=DEATH_SAVED_NAVI
    // BN5 initializes the record ID to 0xFF at battle start. BN6 leaves this
    // native record zeroed until a Navi is actually saved, so validate its
    // backing pointer as well or an empty record is mistaken for Navi ID 0.
    ldr r0,[r1,4]
    cmp r0,0
    beq DeathPhoenixFinish
    ldrb r0,[r1]
    cmp r0,0xFF
    beq DeathPhoenixFinish
    mov r0,24
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    pop {pc}
@@fade:
    EngineCall SPRITE_SET_BLEND_MODE
    pop {pc}
    .pool

DeathPhoenixFinish:
    mov r0,0
    ldr r1,[r5,0x54]
    strb r0,[r1]
    mov r0,8
    str r0,[r5,0x08]
    pop {pc}

DeathPhoenixRecycleUpdate:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    beq DeathRecycleIntro
    cmp r0,4
    beq DeathRecycleInvoke
    cmp r0,8
    beq DeathRecycleWait
    bl DeathRecycleCleanup
    pop {pc}

DeathRecycleIntro:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    ldr r0,[r5,0x4C]
    mov r1,1
.if FALZAR
    EngineCall 0x080E1332
.else
    EngineCall 0x080E266E
.endif
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,4
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

DeathRecycleInvoke:
    push {r4,r6,r7,lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@wait
    ldr r7,=DEATH_SAVED_NAVI
    ldrb r0,[r7]
    lsl r0,r0,2
    ldr r3,=DEATH_SAVED_NAVI_DISPATCH
    ldr r3,[r3,r0]
    ldr r6,[r7,4]
    ldrh r0,[r7,2]
    add r6,r6,r0
    ldrb r2,[r7,1]
    ldr r4,[r7,8]
    mov r7,0x18
    add r7,r7,r5
    push {r5}
    ldr r5,[r5,0x4C]
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    mov lr,pc
    bx r3
    pop {r5}
    mov r0,4
    strb r0,[r5,0x0B]
@@wait:
    ldrb r0,[r5,0x18]
    cmp r0,0
    bne @@done
    mov r0,8
    strh r0,[r5,0x0A]
@@done:
    pop {r4,r6,r7,pc}
    .pool

DeathRecycleWait:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    mov r0,12
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

DeathRecycleCleanup:
    // BN5 performs a mode-0 return transition here. BN6's outer time-freeze
    // controller performs that same transition after the completion byte is
    // released. Its duration replaces BN5's cleanup-transition wait too;
    // retaining that wait here creates an extra 30-frame idle interval.
    mov r0,0
    ldr r1,[r5,0x54]
    strb r0,[r1]
    mov r0,8
    str r0,[r5,0x08]
    bx lr

// Produce the same four shuffled candidate panels per row as BN5. Each byte
// packs Y in the high nibble and X in the low three bits.
DeathBuildPanelList:
    push {r4,r6,r7,lr}
    sub sp,0x2C
    str r2,[sp,0x20]
    add r7,sp,0
    str r1,[sp,0x1C]
    add r0,sp,0
    mov r1,24
    EngineCall MEMORY_CLEAR
    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    ldrb r1,[r5,0x12]
    add r0,r0,r1
    str r0,[sp,0x18]
    str r0,[sp,0x24]
    mov r6,0
    str r6,[sp,0x28]
@@scan:
    ldr r0,[sp,0x18]
    ldr r1,[sp,0x1C]
    EngineCall PANEL_IS_VALID_XY
    cmp r0,0
    beq @@shuffle
    ldr r0,[sp,0x18]
    ldr r1,[sp,0x1C]
    mov r2,0x10
    mov r3,0
    EngineCall PANEL_HAS_FLAGS
    cmp r0,0
    beq @@next
    ldr r0,[sp,0x18]
    ldr r1,[sp,0x1C]
    lsl r1,r1,4
    orr r0,r1
    ldr r2,[sp,0x28]
    strb r0,[r7,r2]
    add r2,1
    str r2,[sp,0x28]
@@next:
    ldr r0,[sp,0x18]
    add r0,r0,r4
    str r0,[sp,0x18]
    b @@scan
@@shuffle:
    ldr r0,[sp,0x28]
    cmp r0,0
    beq @@copy_out
    mov r6,0
@@shuffle_row:
    add r0,sp,0
    add r1,sp,8
    add r1,r1,r6
    ldr r2,[sp,0x28]
    EngineCall MEMORY_COPY
    add r0,sp,8
    add r0,r0,r6
    ldr r1,[sp,0x28]
    ldr r2,[sp,0x28]
    EngineCall SHUFFLE_BYTES
    ldr r0,[sp,0x28]
    add r6,r6,r0
    cmp r6,4
    blt @@shuffle_row
@@copy_out:
    add r0,sp,8
    ldr r1,[sp,0x20]
    mov r2,4
    EngineCall MEMORY_COPY
    add sp,0x2C
    pop {r4,r6,r7,pc}

DeathSpawnStrikeForPanel:
    push {r4,r6,r7,lr}
    sub sp,12
    cmp r0,0
    beq @@done
    str r0,[sp]
    str r1,[sp,4]
    mov r4,0
    ldrb r2,[r5,0x12]
    sub r2,r2,r0
    cmp r2,0
    bge @@positive
    neg r2,r2
@@positive:
    cmp r2,2
    ble @@spawn
    mov r4,1
    cmp r2,4
    bge @@spawn
    EngineCall BATTLE_RNG
    lsr r0,r0,5
    bcc @@spawn
    mov r4,0
@@spawn:
    ldr r0,[sp]
    ldr r1,[sp,4]
    ldr r6,[r5,0x2C]
    bl DeathStrikeSpawn
@@done:
    add sp,12
    pop {r4,r6,r7,pc}

DeathStrikeSpawn:
    push {lr}
    push {r0,r1,r5}
    mov r0,0x81
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1,r2,r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    str r6,[r0,0x2C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    str r5,[r0,0x4C]
@@done:
    pop {pc}

// BN5's type-1 0x5D controller is not interchangeable with BN6's retained
// object in that slot. In particular, its initializer calls the action
// transition that puts the player in state 2; the BN6 object leaves the
// player in state 0, so the projectile callback rejects every shot. Use a
// released slot for the BN5 initializer, then delegate the compatible update
// states to BN6's native controller after initialization.
DeathShotSpawn:
    push {lr}
    push {r0,r5}
    mov r0,0x35
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r1,r5}
    cmp r0,0
    beq @@done
    str r1,[r0,0x60]
    mov r2,1
    strb r2,[r1]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    push {r0}
    ldrb r0,[r5,0x16]
    EngineCall 0x080103BC
    mov r1,r0
    pop {r0}
    str r1,[r0,0x4C]
@@done:
    pop {pc}

DeathShotMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
.if FALZAR
    EngineCall 0x080C4828
.else
    EngineCall 0x080C6098
.endif
    pop {pc}
@@init:
    bl DeathShotInit
    pop {pc}

DeathShotInit:
    push {r4,r7,lr}
    ldr r4,[r5,0x4C]
    ldrh r0,[r4,0x28]
    EngineCall 0x0800F29C
    cmp r0,2
    bne @@simple_coords
    push {r5}
    mov r5,r4
    mov r1,0x2C
    EngineCall 0x08013774
    push {r0}
    mov r1,0x29
    EngineCall 0x08013774
    pop {r1}
    pop {r5}
    EngineCall 0x0800FC9E
    b @@coords_ready
@@simple_coords:
    ldrh r0,[r4,0x28]
    EngineCall 0x0800F26C
    mov r2,r1
    mov r1,r0
@@coords_ready:
    mov r0,0x80
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_2E3C
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]

    mov r3,0x34
    add r0,r4,r3
    ldmia r0!,{r1-r3}
    mov r0,0x34
    add r0,r5,r0
    stmia r0!,{r1-r3}
    ldrh r0,[r5,0x3A]
    sub r0,1
    strh r0,[r5,0x3A]
    ldrh r0,[r5,0x3E]
    sub r0,1
    strh r0,[r5,0x3E]

    push {r5}
    mov r5,r4
    EngineCall 0x0801002C
    pop {r5}
    EngineCall SPRITE_SET_PALETTE
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP

    ldrh r0,[r4,0x28]
    EngineCall 0x0800F29C
    cmp r0,2
    bne @@finish
    push {r5}
    mov r5,r4
    mov r1,0x2C
    EngineCall 0x08013774
    push {r0}
    mov r1,0x29
    EngineCall 0x08013774
    pop {r1}
    pop {r5}
    mov r2,1
    EngineCall 0x08011420
@@finish:
    mov r0,0
    strb r0,[r5,0x10]
    mov r0,0xFF
    strb r0,[r5,0x11]
    mov r0,4
    str r0,[r5,0x08]
.if FALZAR
    EngineCall 0x080C4828
.else
    EngineCall 0x080C6098
.endif
    pop {r4,r7,pc}
    .pool

// BN5 type-4 0x89, translated directly. Unlike the adjacent type-4 0x8A
// object, this never invokes MegaMan's attack animation or projectile
// callback. The object itself supplies the animated DeathPhoenix strike and
// creates its two direct-damage contacts at frames 0 and 16.
DeathStrikeMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    b @@animate
@@init:
    bl DeathStrikeInit
    b @@animate
@@update:
    bl DeathStrikeUpdate
@@animate:
    EngineCall OBJECT_UPDATE
    pop {pc}

DeathStrikeInit:
    push {lr}
    // The imported BN5 group-0x10/index-0x49 archive is installed in the
    // released BN6 group-0x14/index-0x17 pointer slot.
    ldr r0,=0x01001417
    EngineCall OBJECT_LOAD_NAVI_SPRITE
    EngineCall DEATH_SPRITE_SPECIAL
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    EngineCall OBJECT_SET_COORDS
    mov r0,0
    str r0,[r5,0x3C]
    mov r0,0x45
    add r0,0xFF
    EngineCall PLAY_SOUND
    mov r0,4
    str r0,[r5,0x08]
    bl DeathStrikeUpdate
    pop {pc}
    .pool

DeathStrikeUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq DeathStrikeBegin
    cmp r0,4
    beq DeathStrikeLeadIn
    bl DeathStrikeAttack
    pop {pc}

DeathStrikeBegin:
    mov r0,10
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    bx lr

DeathStrikeLeadIn:
    push {lr}
    ldrb r0,[r5,0x10]
    cmp r0,0
    bne @@tick
    EngineCall SPRITE_GET_ANIMATION_FLAGS
    mov r1,0x80
    tst r0,r1
    beq @@done
    mov r0,1
    strb r0,[r5,0x10]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,0
    strh r0,[r5,0x22]
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    mov r0,0x46
    add r0,0xFF
    EngineCall PLAY_SOUND
@@done:
    pop {pc}

DeathStrikeAttack:
    push {r4,r6,r7,lr}
    ldrh r0,[r5,0x22]
    add r1,r0,1
    strh r1,[r5,0x22]
    cmp r0,0
    beq @@first_contact
    cmp r0,16
    bne @@pulse

    // The second contact is one panel toward the owning side, exactly as
    // BN5's 0x0800C6D6 direction helper computes it.
    ldrb r0,[r5,0x16]
    ldrb r1,[r5,4]
    eor r0,r1
    lsl r0,r0,1
    sub r0,1
    neg r0,r0
    ldrb r1,[r5,0x12]
    add r0,r0,r1
    ldrb r1,[r5,0x13]
    mov r2,0x1F
    b @@spawn_contact
@@first_contact:
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    mov r2,0x19
@@spawn_contact:
    push {r2}
    mov r2,0
    ldr r3,=0x00100000
    ldr r4,=0x0A050001
    ldr r6,[r5,0x2C]
    mov r7,3
.if FALZAR
    EngineCall 0x080C53A6
.else
    EngineCall 0x080C6C16
.endif
    pop {r1}
    cmp r0,0
    beq @@pulse
    strh r1,[r0,0x20]

@@pulse:
    // BN5 type-4 0x89 creates a separate type-4 0x71 actor once per pulse.
    // That helper, not the generic damage contact, is the moving purple
    // flame/fireball visible during DeathPhoenix.
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@countdown
    mov r0,1
    strb r0,[r5,0x0A]
    ldrb r0,[r5,0x12]
    ldrb r1,[r5,0x13]
    mov r3,4
    lsl r3,r3,16
    ldr r4,[r5,4]
    bl DeathFlameSpawn
    mov r0,4
    strh r0,[r5,0x20]
@@countdown:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,0
    strb r0,[r5,0x0A]
    ldrb r0,[r5,0x0B]
    add r0,1
    strb r0,[r5,0x0B]
    cmp r0,10
    blt @@done
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {r4,r6,r7,pc}
    .pool

// BN5 type-4 0x71, translated directly into released BN6 type-4 slot 0x82.
// It shares the imported group-0x10/index-0x49 archive with type-4 0x89;
// that archive is installed at BN6 group 0x14/index 0x17.
DeathFlameSpawn:
    push {lr}
    push {r0,r1,r5}
    mov r0,0x82
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1,r2,r5}
    cmp r0,0
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    ldrb r1,[r0]
    mov r2,0x10
    bic r1,r2
    strb r1,[r0]

    // BN5's 0x080E8BF4 wrapper restores bit 0x10 and mirrors animation
    // byte 0x17 when the strike controller's r4 flag is nonzero.
    ldrb r1,[r0]
    orr r1,r2
    strb r1,[r0]
    cmp r4,0
    beq @@done
    ldrb r1,[r0,0x17]
    mov r2,1
    eor r1,r2
    strb r1,[r0,0x17]
@@done:
    pop {pc}

DeathFlameMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    b @@animate
@@init:
    bl DeathFlameInit
    b @@animate
@@update:
    bl DeathFlameUpdate
@@animate:
    EngineCall OBJECT_UPDATE
    pop {pc}

DeathFlameInit:
    push {r4,lr}
    EngineCall OBJECT_SET_COORDS
    ldrh r0,[r5,0x3A]
    sub r0,1
    strh r0,[r5,0x3A]
    mov r0,0x80
    mov r1,0x14
    mov r2,0x17
    EngineCall SPRITE_LOAD
    mov r0,2
    strb r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0
    EngineCall SPRITE_SET_PALETTE
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,4
    str r0,[r5,0x08]
    pop {r4,pc}

DeathFlameUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq DeathFlameDelay
    bl DeathFlameMotion
    pop {pc}

DeathFlameDelay:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@tick
    mov r0,1
    strb r0,[r5,0x0A]
    mov r1,10
    strh r1,[r5,0x20]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

DeathFlameMotion:
    push {r4,lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    bne @@move
    mov r0,1
    strb r0,[r5,0x0A]
    mov r0,0
    strh r0,[r5,4]
    mov r1,16
    strh r1,[r5,0x20]
    mov r0,0x80
    lsl r0,r0,8
    swi 6
    strh r0,[r5,6]
    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    mov r0,0x28
    lsl r0,r0,16
    mov r1,16
    swi 6
    mul r0,r4
    str r0,[r5,0x40]
@@move:
    ldr r1,[r5,0x40]
    ldr r0,[r5,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]

    ldrh r0,[r5,4]
    ldrh r1,[r5,6]
    ldrh r2,[r5,0x20]
    cmp r2,8
    bgt @@first_half
    lsr r2,r1,2
    add r1,r1,r2
    b @@angle
@@first_half:
    lsr r2,r1,2
    sub r1,r1,r2
@@angle:
    add r0,r0,r1
    strh r0,[r5,4]
    ldr r1,=DEATH_SINE_TABLE
    lsr r0,r0,8
    lsl r0,r0,1
    ldrh r0,[r1,r0]
    lsl r0,r0,16
    asr r0,r0,16
    mov r1,20
    mul r0,r1
    lsl r0,r0,8
    ldr r1,=0x00040000
    add r0,r0,r1
    str r0,[r5,0x3C]

    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bgt @@done
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {r4,pc}
    .pool

DeathPhoenixCodeEnd:
.endautoregion

.if FALZAR
    .autoregion
    .align 4
    DeathPhoenixIcon:
    .import "build/deathphoenix-icon.bin"
    .endautoregion

    .autoregion
    .align 4
    DeathPhoenixImage:
    .import "build/deathphoenix-image.bin"
    .endautoregion

    .autoregion
    .align 4
    DeathPhoenixPalette:
    .import "build/deathphoenix-palette.bin"
    .endautoregion
.endif

.autoregion
.align 4
DeathPhoenixBattleSprite:
.import "build/deathphoenix-battle-sprite.bin"
.endautoregion

.autoregion
.align 4
DeathPhoenixStrikeSprite:
.import "build/deathphoenix-strike-sprite.bin"
.endautoregion
