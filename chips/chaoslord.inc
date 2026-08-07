// ---------------------------------------------------------------------------
// ChaosLord replaces BigHook. This translates the complete BN5 controller,
// Ball Bass attack, helper object, and Nebula Gray aura/burst object chain to
// BN6 while preserving BigHook's chip slot.

.definelabel CHAOSLORD,                 0x12E

// ---------------------------------------------------------------------------
// BigHook chip record -> ChaosLord. The dispatch family remains BN6's common
// time-freeze family (0x1B), with BigHook's subfamily slot (0x17) repointed to
// the translated BN5 controller.
.org CHIP_DATA + CHAOSLORD * 0x2C
    .db 0x17,0xFF,0xFF,0xFF // X
    // BN5's null element is 0x0C; BN6's null element is 0x0A.
    .db 0x00,0x04,0x0A,0x02
    .db 0x63
.if FALZAR
    .db 0x03
.else
    .db 0x43
.endif
    .db 0xB2,0x1B
    .db 0x17,0x0A,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x10
    .db 0x33,0x00
    .dh 500,0x012E
    .db 0x01,0xFF
.if FALZAR
    .dw 0x0872C414,0x08720B74,0x08725554
.else
    .dw ChaosLordIcon,ChaosLordImage,ChaosLordPalette
.endif

.definelabel CHAOS_TIMEFREEZE_ENTRY,     0x0802CDB8 // subfamily 0x17
.definelabel TYPE1_CHAOS_ENTRY,          0x08003DE0 // free type-1 slot 0x51
.definelabel TYPE1_CHAOS_BALL_ENTRY,     0x08003DE4 // free type-1 slot 0x52
.definelabel TYPE3_CHAOS_ATTACK_ENTRY,   0x08003F78 // free type-3 slot 0x2D
.definelabel TYPE4_CHAOS_AURA_ENTRY,     0x080044F8 // BigHook's released type-4 slot 0x8C
.definelabel TYPE4_CHAOS_BURST_ENTRY,    0x080044FC // BigHook's released type-4 slot 0x8D

.definelabel SPRITE_PTR_CHAOS_BASS,      0x08031DA4 // group 0x08, index 0x00
.definelabel SPRITE_PTR_CHAOS_BALL,      0x08031DF0 // group 0x08, index 0x13
.definelabel SPRITE_PTR_CHAOS_APPARITION,0x08032004 // group 0x10, index 0x18
.definelabel SPRITE_PTR_CHAOS_AURA,      0x08031DEC // released group 0x08, index 0x12
.definelabel SPRITE_PTR_CHAOS_TEARDOWN,  0x08031FB4 // unused group 0x10, index 0x04
.definelabel SPRITE_PTR_CHAOS_IMPACT,    0x08032164 // group 0x14, index 0x14

.definelabel CHAOS_PANEL_SEARCH,         0x08015E46

// Battle dispatch and sprite archive slots.
.org CHAOS_TIMEFREEZE_ENTRY
.dw ChaosTimeFreezeSpawn + 1
.org TYPE1_CHAOS_ENTRY
.dw ChaosControllerMain + 1
.org TYPE1_CHAOS_BALL_ENTRY
.dw ChaosBallMain + 1
.org TYPE3_CHAOS_ATTACK_ENTRY
.dw ChaosAttackMain + 1
.org TYPE4_CHAOS_AURA_ENTRY
.dw ChaosAuraMain + 1
.org TYPE4_CHAOS_BURST_ENTRY
.dw ChaosBurstOrTeardownMain + 1

.org SPRITE_PTR_CHAOS_BASS
.dw ChaosBassSprite
.org SPRITE_PTR_CHAOS_BALL
.dw ChaosBassSprite
.org SPRITE_PTR_CHAOS_APPARITION
.dw ChaosApparitionSprite
.org SPRITE_PTR_CHAOS_AURA
// This is BN5's compressed group-8/index-0x14 archive, moved to BN6's released
// group-8/index-0x12 slot. Keeping BN6's live index-0x14 archive intact is
// required by native impact effects that run at the end of ChaosLord.
// The compression bit makes BN6 unpack the
// complete 0xB3A4-byte sprite set into its normal battle-asset cache.
.dw ChaosAuraSprite | 0x80000000
.org SPRITE_PTR_CHAOS_TEARDOWN
.dw ChaosTeardownSprite
.org SPRITE_PTR_CHAOS_IMPACT
// BN5 and BN6 map the ChaosLord impact selector to this same group/index,
// but BN6's replacement archive omits BN5's palette-whitening animation.
.dw ChaosImpactSprite

.autoregion
.align 4
ChaosCodeStart:

ChaosTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x51
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r1-r3,r5}
    cmp r0,0
    beq ChaosTimeFreezeSpawnDone
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    strb r3,[r0,0x0E]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    str r6,[r0,0x2C]
    // BN5 keeps the time-freeze completion pointer at +0x48. +0x40 is
    // independently used for the controller's horizontal velocity. The
    // pointed-to flag is a word in BN5 but only a byte in BN6; a word store
    // corrupts the adjacent cut-in state when Invis precedes ChaosLord.
    str r7,[r0,0x48]
    mov r1,1
ChaosSetCompletionActive:
    strb r1,[r7]
ChaosTimeFreezeSpawnDone:
    pop {pc}
    .pool

// ----- BN5 type-1 0x51 ChaosLord controller -------------------------------

ChaosControllerMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    bl ChaosControllerDestroy
    b @@animate
@@init:
    bl ChaosControllerInit
    b @@animate
@@update:
    bl ChaosControllerUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}
    .pool

ChaosControllerDestroy:
    push {lr}
    mov r0,0x34
    add r0,r0,r5
    ldmia r0!,{r1-r3}
    mov r4,0x10
    lsl r4,r4,16
    add r3,r3,r4
    mov r4,0x12
    // BN5 and BN6 use the same generic-effect selector number here, but the
    // archive behind group 0x14/index 0x0D changed between games. Route this
    // one ChaosLord effect through BN5's archive.
    bl ChaosTeardownSpawn
    ldr r0,[r5,0x4C]
    bl ChaosBallRequestDestroy
    mov r0,0
    ldr r1,[r5,0x48]
ChaosReleaseCompletion:
    strb r0,[r1]
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

// ----- BN5 type-1 0x57 Ball Bass sprite helper ---------------------------
//
// BN6 also has a type-1 0x57 object, but it adds BN6-only palette, animation,
// and visibility branches. Those branches turn ChaosLord's dotted Ball Bass
// sequence into a short white/cyan orb. Keep the BN5 state machine intact in
// released type-1 slot 0x52 and translate only the engine entry points.

ChaosBallMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl ChaosBallInit
    pop {pc}
@@update:
    bl ChaosBallUpdate
    pop {pc}

ChaosBallInit:
    push {lr}
    mov r0,0x80
    ldrb r1,[r5,0x04]
    ldrb r2,[r5,0x05]
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    ldr r0,[r5,0x4C]
    EngineCall SPRITE_GET_PALETTE
    EngineCall SPRITE_SET_PALETTE
    ldr r0,[r5,0x4C]
    ldrb r0,[r0,0x10]
    ldrb r1,[r5,0x07]
    add r0,r0,r1
    strh r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    mov r0,4
    str r0,[r5,0x08]
    bl ChaosBallUpdate
    pop {pc}

ChaosBallUpdate:
    push {r4,lr}
    ldr r4,[r5,0x4C]
    ldrb r0,[r4,0x10]
    ldrb r1,[r5,0x07]
    add r0,r0,r1
    strb r0,[r5,0x10]
    // BN6 tracks the previously loaded animation at +0x11, so refresh when
    // the parent advances from Bass animation 0x0E to Ball animation 0x1E.
    // Do not call SPRITE_PROPERTY_26C4 here: BN6's native helper also omits
    // it on this reload. Calling it advances the imported BN5 archive to the
    // next frame record and skips Ball Bass's composite graphics.
    ldrb r1,[r5,0x11]
    cmp r0,r1
    beq @@animation_ready
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
@@animation_ready:

    mov r0,0x34
    add r0,r0,r4
    ldmia r0!,{r1-r3}
    mov r0,0x34
    add r0,r0,r5
    stmia r0!,{r1-r3}

    mov r1,2
    mvn r1,r1
    ldrb r0,[r5]
    and r0,r1
    ldrb r1,[r4]
    mov r2,2
    and r1,r2
    orr r0,r1
    strb r0,[r5]

    ldr r0,[r5,0x4C]
    EngineCall SPRITE_GET_SCALE
    EngineCall SPRITE_SET_SCALE
    ldr r0,[r5,0x4C]
    EngineCall SPRITE_PROPERTY_2DC8
    EngineCall SPRITE_PROPERTY_2DB4
    ldr r0,[r5,0x4C]
    EngineCall SPRITE_PROPERTY_2F3E
    EngineCall SPRITE_PROPERTY_2F02
    ldr r0,[r5,0x4C]
    ldrb r1,[r0,0x17]
    strb r1,[r5,0x17]
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP

    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@phase0
    bl ChaosBallPhase4
    pop {r4,pc}
@@phase0:
    bl ChaosBallPhase0
    pop {r4,pc}

ChaosBallPhase0:
    push {lr}
    ldr r0,[r5,0x4C]
    EngineCall SPRITE_PROPERTY_2CE0
    EngineCall SPRITE_PROPERTY_2C7E
    EngineCall SPRITE_ANIMATION_STATUS
    lsr r0,r0,2
    bcc @@done
    ldrb r0,[r5]
    mov r1,4
    bic r0,r1
    strb r0,[r5]
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
    bl ChaosBallPhase4
@@done:
    pop {pc}

ChaosBallPhase4:
    push {lr}
    ldr r0,[r5,0x4C]
    EngineCall SPRITE_PROPERTY_2CE0
    EngineCall SPRITE_PROPERTY_2C7E
    // Spawned with the time-stop flag, just like BN5 and BN6's native helper.
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

ChaosBallSpawn:
    push {lr}
    push {r5}
    mov r0,0x52
    EngineCall SPAWN_TYPE1
    mov r0,r5
    pop {r5}
    cmp r0,0
    beq @@done
    str r5,[r0,0x4C]
    ldrb r1,[r5,0x16]
    strb r1,[r0,0x16]
    ldrb r1,[r0]
    mov r2,4
    orr r1,r2
    strb r1,[r0]
@@done:
    pop {pc}

ChaosBallRequestDestroy:
    cmp r0,0
    beq @@done
    mov r1,8
    str r1,[r0,0x08]
@@done:
    bx lr

ChaosControllerInit:
    push {r4,r6,lr}
    mov r0,0x94
    EngineCall PLAY_SOUND
    bl ChaosDeleteObjects
    mov r0,8
    // Preload the imported BN5 aura archive from its translated BN6 slot.
    mov r1,0x12
    EngineCall SPRITE_ALLOC

    // BN5's 0x08105594 table helper: choose the off-screen X coordinate for
    // the chip owner's side. r3 is always zero for this effect.
    ldrb r0,[r5,0x16]
    cmp r0,0
    bne @@right_side
    ldr r2,=0x04000000
    b @@side_ready
@@right_side:
    ldr r2,=0x08000000
@@side_ready:
    mov r3,0

    ldrb r6,[r5,0x16]
    ldrb r0,[r5,0x17]
    eor r6,r0
    mov r0,5
    mul r0,r6
    add r0,1
    mov r1,2
    mov r4,0x11
    EngineCall CHAOS_PANEL_SEARCH
    cmp r0,0
    beq @@can_spawn
    mov r0,8
    str r0,[r5,0x08]
    pop {r4,r6,pc}

@@can_spawn:
    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,8
    lsl r4,r4,16
    mul r4,r0

    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,5
    mul r0,r1
    add r0,1
    mov r1,2
    EngineCall PANEL_TO_COORDS
    mov r2,r1
    add r1,r0,r4
    mov r3,0

    ldr r4,=0x005A0A0A
    push {r1-r3}
    bl ChaosSpawnAura
    pop {r1-r3}
    ldr r4,=0x005A0A07
    push {r1-r3}
    bl ChaosSpawnAura
    pop {r1-r3}
    ldr r4,=0x005A0A04
    push {r1-r3}
    bl ChaosSpawnAura
    pop {r1-r3}
    ldr r4,=0x005A0A01
    push {r1-r3}
    bl ChaosSpawnAura
    pop {r1-r3}
    ldr r4,=0x017C0A0D
    push {r1-r3}
    bl ChaosSpawnAura
    pop {r1-r3}

    mov r0,0x8E
    strh r0,[r5,0x20]
    EngineCall OBJECT_FRONT_DIRECTION
    ldr r1,=0x00009D89 // (32 << 16) / 52
    mul r0,r1
    str r0,[r5,0x40]
    mov r0,0x17
    str r0,[r5,0x68]
    mov r0,5
    str r0,[r5,0x60]
    mov r0,1
    str r0,[r5,0x64]

    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,r0
    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,5
    mul r0,r1
    add r0,1
    mov r1,2
    strb r0,[r5,0x12]
    strb r1,[r5,0x13]
    EngineCall OBJECT_SET_COORDS
    mov r0,8
    lsl r0,r0,16
    mul r4,r0
    ldr r1,[r5,0x34]
    add r1,r1,r4
    str r1,[r5,0x34]
    ldrh r1,[r5,0x3A]
    add r1,0x1C
    strh r1,[r5,0x3A]
    mov r1,0x12
    strh r1,[r5,0x3E]
    mov r0,4
    str r0,[r5,0x08]
    bl ChaosControllerUpdate
    pop {r4,r6,pc}
    .pool

ChaosControllerUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@phase0
    cmp r0,4
    beq @@phase4
    cmp r0,8
    beq @@phase8
    cmp r0,0x0C
    beq @@phase12
    bl ChaosPhase16
    pop {pc}
@@phase0:
    bl ChaosPhase0
    pop {pc}
@@phase4:
    bl ChaosPhase4
    pop {pc}
@@phase8:
    bl ChaosPhase8
    pop {pc}
@@phase12:
    bl ChaosPhase12
    pop {pc}

ChaosPhase0:
    push {r4,r6,r7,lr}
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done

    EngineCall OBJECT_FRONT_DIRECTION
    mov r4,8
    lsl r4,r4,16
    mul r4,r0
    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,5
    mul r0,r1
    add r0,1
    mov r1,2
    EngineCall PANEL_TO_COORDS
    add r0,r0,r4
    mov r4,0x1B
    lsl r4,r4,16
    add r1,r1,r4
    push {r0,r1}
    ldr r7,=ChaosBurstTable
@@burst_loop:
    // Match BN5's order exactly. OBJECT_FRONT_DIRECTION clobbers r1 in BN6,
    // so fetch the signed table byte only after the engine call returns.
    EngineCall OBJECT_FRONT_DIRECTION
    mov r1,0
    ldsb r1,[r7,r1]
    cmp r1,0x7F
    beq @@burst_done
    mul r0,r1
    mov r1,1
    ldsb r1,[r7,r1]
    lsl r1,r1,16
    lsr r1,r1,16
    lsl r6,r0,16
    orr r6,r1
    ldrb r4,[r7,2]
    lsl r4,r4,8
    add r4,0x1E
    mov r0,1
    lsl r0,r0,24
    orr r4,r0
    ldr r1,[sp]
    ldr r2,[sp,4]
    mov r3,0
    bl ChaosSpawnBurst
    add r7,3
    b @@burst_loop
@@burst_done:
    pop {r0,r1}
    mov r0,8
    add r0,0xFF
    EngineCall PLAY_SOUND
    mov r0,0x25
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {r4,r6,r7,pc}
    .pool

ChaosBurstTable:
    .db 0x30,0xED,0x01, 0x03,0xEA,0x00, 0x0A,0xCC,0x00
    .db 0xFA,0xC0,0x00, 0xF2,0xD8,0x00, 0xE8,0xF6,0x01
    .db 0x03,0xFD,0x01, 0x16,0xDB,0x01, 0x17,0xB3,0x01
    .db 0x1F,0xC6,0x01, 0x7F,0x00
    .align 4

ChaosPhase4:
    push {r4,lr}
    ldrh r0,[r5,0x20]
    cmp r0,7
    bne @@count
    mov r0,0x80
    mov r1,0x10
    mov r2,0x18
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_NO_SHADOW
    mov r0,1
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    ldrh r0,[r5,0x3E]
    add r0,8
    strh r0,[r5,0x3E]
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
@@count:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done

    mov r0,0x80
    mov r1,8
    mov r2,0
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_PROPERTY_2E3C
    mov r0,0
    EngineCall SPRITE_SET_FLAG
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,0
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    ldrh r0,[r5,0x3E]
    sub r0,8
    strh r0,[r5,0x3E]
    mov r0,0
    EngineCall SPRITE_SET_PALETTE
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    ldr r4,=0x10011308
    mov r1,8
    mov r2,0x13
    bl ChaosBallSpawn
    str r0,[r5,0x4C]
    mov r0,0x48
    strh r0,[r5,0x20]
    ldr r0,[r5,0x60]
    ldr r1,[r5,0x64]
    lsl r1,r1,5
    orr r0,r1
    ldr r1,[r5,0x68]
    lsl r1,r1,10
    orr r0,r1
    EngineCall SPRITE_SET_SCALE
    mov r0,8
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {r4,pc}
    .pool

ChaosPhase8:
    push {lr}
    ldrh r0,[r5,0x20]
    cmp r0,0x34
    blt @@shrink
    bgt @@visible
    mov r0,0x2B
    add r0,0xFF
    EngineCall PLAY_SOUND
@@visible:
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldrh r0,[r5,0x20]
    mov r1,2
    tst r0,r1
    beq @@scale_visible
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    b @@move
@@scale_visible:
    mov r1,4
    and r0,r1
    bne @@scale_stored
    ldr r0,=0x00007FFF
    EngineCall SPRITE_SET_SCALE
    b @@move

@@scale_stored:
    ldr r0,[r5,0x60]
    ldr r1,[r5,0x64]
    lsl r1,r1,5
    orr r0,r1
    ldr r1,[r5,0x68]
    lsl r1,r1,10
    orr r0,r1
    EngineCall SPRITE_SET_SCALE
    b @@move

@@shrink:
    cmp r0,0x24
    bgt @@move
    mov r1,3
    tst r0,r1
    bne @@move
    ldr r0,[r5,0x60]
    sub r0,2
    bpl @@scale0_ok
    mov r0,0
@@scale0_ok:
    str r0,[r5,0x60]
    ldr r1,[r5,0x64]
    sub r1,2
    bpl @@scale1_ok
    mov r1,0
@@scale1_ok:
    str r1,[r5,0x64]
    lsl r1,r1,5
    orr r0,r1
    ldr r1,[r5,0x68]
    sub r1,2
    bpl @@scale2_ok
    mov r1,0
@@scale2_ok:
    str r1,[r5,0x68]
    lsl r1,r1,10
    orr r0,r1
    EngineCall SPRITE_SET_SCALE

@@move:
    ldr r0,[r5,0x3C]
    ldr r1,=0x0000C000
    add r0,r0,r1
    str r0,[r5,0x3C]
    ldr r0,[r5,0x34]
    ldr r1,[r5,0x40]
    add r0,r0,r1
    str r0,[r5,0x34]
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,0x80
    strh r0,[r5,0x20]
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}
    .pool

ChaosPhase12:
    push {r4,r6,r7,lr}
    ldrh r0,[r5,0x20]
    cmp r0,0x6C
    bne @@count
    mov r0,0x0E
    strb r0,[r5,0x10]
    mov r0,0x42
    add r0,0xFF
    EngineCall PLAY_SOUND
    mov r4,1
    ldr r6,[r5,0x2C]
    mov r0,0x34
    add r0,r0,r5
    ldmia r0!,{r1-r3}
    mov r7,0
    bl ChaosAttackSpawn
@@count:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,0x0F
    strb r0,[r5,0x10]
    mov r0,0x28
    strh r0,[r5,0x20]
    mov r0,0x10
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {r4,r6,r7,pc}
    .pool

ChaosPhase16:
    push {lr}
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {pc}

// BN5 helper at 0x080C32A2. ChaosLord removes the eight live obstacle/object
// references before its entrance, exactly as the original chip does.
ChaosDeleteObjects:
    push {lr}
    mov r0,r10
    ldr r0,[r0,0x18]
    add r0,0xA0
    mov r3,8
    mov r2,0
@@loop:
    ldr r1,[r0]
    cmp r1,0
    beq @@next
    strh r2,[r1,0x24]
@@next:
    add r0,4
    sub r3,1
    bne @@loop
    pop {pc}

// BN5 type-4 0x47/0x4B spawners, sharing BN6's released type-4 slot. The low
// byte of r4 distinguishes the five persistent Nebula Gray layers from the
// short entrance bursts.
ChaosSpawnAura:
    push {lr}
    push {r5-r7}
    mov r0,0x8C
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r5-r7}
    cmp r0,0
    beq @@done
    str r5,[r0,0x4C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
@@done:
    pop {pc}
    .pool

ChaosSpawnBurst:
    push {lr}
    // BN5's type-4 allocator preserves r6, which carries the packed X/Y
    // launch vector. BN6's allocator uses r6 internally, so preserve it
    // explicitly before copying that implicit argument into the new object.
    push {r5,r6}
    mov r0,0x8D
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r5,r6}
    cmp r0,0
    beq @@done
    str r5,[r0,0x4C]
    str r6,[r0,0x70]
    // Slot 0x8D is reused for the final cosmetic teardown after every burst
    // has expired. Keep a stable discriminator outside the packed spawn
    // arguments; the burst lifetime at +0x04 changes every frame.
    mov r1,0
    str r1,[r0,0x7C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
    // The controller calls BN5's 0x080E5FD2 wrapper, not the lower-level
    // 0x080E5FB0 allocator. The wrapper sets bit 0x10 on every entrance
    // fireball after the allocator clears it.
    ldrb r1,[r0]
    mov r2,0x10
    orr r1,r2
    strb r1,[r0]
@@done:
    pop {pc}
    .pool

ChaosTeardownSpawn:
    push {lr}
    push {r5}
    mov r0,0x8D
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r5}
    cmp r0,0
    beq @@done
    mov r1,1
    str r1,[r0,0x7C]
    ldrh r1,[r5,0x16]
    strh r1,[r0,0x16]
@@done:
    pop {pc}
    .pool

// BN6 retains BN5's type-4 0x0A palette effect, but schedules that native
// slot only after this time-freeze attack exits. Host the same variant-1
// state in released slot 0x8D so its four frames occur at impact.
ChaosFlashSpawn:
    push {lr}
    push {r5-r7}
    mov r0,0x8D
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r5-r7}
    cmp r0,0
    beq @@done
    str r7,[r0,0x60]
    mov r1,2
    str r1,[r0,0x7C]
    ldrb r1,[r0]
    mov r2,0x14
    orr r1,r2
    strb r1,[r0]
@@done:
    pop {pc}
    .pool

// ----- BN5 type-3 0x82 ChaosLord Ball Bass attack -------------------------

ChaosAttackSpawn:
    push {lr}
    // The type-3 allocator consumes r1/r2/r3 as fixed-point X/Y/Z and
    // initializes the object's coordinates itself. Preserve only the parent
    // object, exactly like BN5's 0x080D69C8/0x080D69E6 pair.
    // BN5's allocator preserves the implicit r6/r7 payload. BN6's type-3
    // allocator does not, so retain the damage data and parent pointer here.
    push {r5-r7}
    mov r0,0x2D
    EngineCall SPAWN_TYPE3
    mov r0,r5
    pop {r5-r7}
    cmp r0,0
    beq @@done
    mov r3,0
    strb r3,[r0,0x0E]
    str r6,[r0,0x2C]
    str r7,[r0,0x4C]
    ldrh r3,[r5,0x16]
    strh r3,[r0,0x16]
    ldrb r1,[r0]
    mov r2,0x10
    orr r1,r2
    strb r1,[r0]
@@done:
    pop {pc}
    .pool

ChaosAttackMain:
    push {lr}
    // SignalRed shares this otherwise unavailable type-3 slot. Its spawner
    // sets a tag in an unused tail word; ChaosLord objects retain zero here.
    ldr r0,[r5,0x78]
    cmp r0,0x53
    beq @@signalRed
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    b @@animate
@@init:
    bl ChaosAttackInit
    b @@animate
@@update:
    bl ChaosAttackUpdate
@@animate:
    EngineCall OBJECT_UPDATE
    pop {pc}
@@signalRed:
    bl SignalRedObjectMain
    pop {pc}
    .pool

ChaosAttackInit:
    push {lr}
    mov r0,0
    bl ChaosAttackSetSprite
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    mov r0,0x4B
    ldrb r1,[r5,0x04]
    cmp r1,0
    beq @@timer
    mov r0,0x69
@@timer:
    strh r0,[r5,0x20]
    EngineCall OBJECT_FRONT_DIRECTION
    mov r1,0x2E
    mul r0,r1
    ldrh r1,[r5,0x36]
    add r1,r1,r0
    strh r1,[r5,0x36]
    ldrh r1,[r5,0x3E]
    add r1,0x32
    strh r1,[r5,0x3E]
    ldrh r1,[r5,0x3A]
    add r1,8
    strh r1,[r5,0x3A]
    mov r0,4
    str r0,[r5,0x08]
    bl ChaosAttackUpdate
    pop {pc}
    .pool

ChaosAttackUpdate:
    push {lr}
    EngineCall BATTLE_IS_OVER
    tst r0,r0
    bne @@destroy
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@form
    bl ChaosAttackFly
    pop {pc}
@@form:
    bl ChaosAttackForm
    pop {pc}
@@destroy:
    mov r0,8
    str r0,[r5,0x08]
    pop {pc}
    .pool

ChaosAttackForm:
    push {r4,r6,r7,lr}
    ldrh r0,[r5,0x20]
    mov r1,0x3A
    ldrb r2,[r5,0x04]
    cmp r2,0
    beq @@firstForm
    mov r1,0x58
@@firstForm:
    cmp r0,r1
    bne @@secondCheck
    mov r0,1
    bl ChaosAttackSetSprite
    ldrh r0,[r5,0x3E]
    add r0,0x12
    strh r0,[r5,0x3E]
    b @@tick

@@secondCheck:
    mov r1,0x32
    ldrb r2,[r5,0x04]
    cmp r2,0
    beq @@secondForm
    mov r1,0x50
@@secondForm:
    cmp r0,r1
    bne @@thirdCheck
    mov r0,2
    bl ChaosAttackSetSprite
    mov r0,0x2B
    add r0,0xFF
    EngineCall PLAY_SOUND
    b @@tick

@@thirdCheck:
    mov r1,0x2D
    ldrb r2,[r5,0x04]
    cmp r2,0
    beq @@thirdForm
    mov r1,0x4B
@@thirdForm:
    cmp r0,r1
    bne @@targetCheck
    mov r0,3
    bl ChaosAttackSetSprite
    ldrh r0,[r5,0x3E]
    add r0,0x0C
    strh r0,[r5,0x3E]

@@targetCheck:
    ldrh r0,[r5,0x20]
    cmp r0,0x0F
    bne @@tick
    ldrb r0,[r5,0x04]
    cmp r0,0
    beq @@parentTarget
    ldrb r0,[r5,0x16]
    ldrb r1,[r5,0x17]
    eor r0,r1
    mov r1,1
    eor r0,r1
    mov r1,3
    mul r0,r1
    add r0,2
    mov r1,2
    b @@saveTarget
@@parentTarget:
    ldr r0,[r5,0x4C]
    ldrb r1,[r0,0x13]
    ldrb r0,[r0,0x12]
@@saveTarget:
    strb r0,[r5,0x06]
    strb r1,[r5,0x07]

@@tick:
    bl ChaosAttackFlashTarget
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done

    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldrb r0,[r5,0x06]
    ldrb r1,[r5,0x07]
    EngineCall PANEL_TO_COORDS
    push {r1}
    ldr r1,[r5,0x34]
    sub r0,r0,r1
    mov r1,0x0F
    swi 6
    str r0,[r5,0x40]
    pop {r0}
    mov r1,8
    lsl r1,r1,16
    add r0,r0,r1
    ldr r1,[r5,0x38]
    sub r0,r0,r1
    mov r1,0x0F
    swi 6
    str r0,[r5,0x44]
    ldr r0,[r5,0x3C]
    mov r1,8
    lsl r1,r1,16
    sub r1,r1,r0
    mov r1,0x0F
    swi 6
    str r0,[r5,0x48]
    mov r0,0x0F
    strh r0,[r5,0x20]
    mov r0,0x20
    add r0,0xFF
    EngineCall PLAY_SOUND
    mov r0,4
    strb r0,[r5,0x09]
@@done:
    pop {r4,r6,r7,pc}
    .pool

ChaosAttackFly:
    push {r4,r6,r7,lr}
    bl ChaosAttackFlashTarget
    ldr r0,[r5,0x40]
    ldr r1,[r5,0x34]
    add r0,r0,r1
    str r0,[r5,0x34]
    ldr r0,[r5,0x44]
    ldr r1,[r5,0x38]
    add r0,r0,r1
    str r0,[r5,0x38]
    ldr r0,[r5,0x48]
    ldr r1,[r5,0x3C]
    sub r1,r1,r0
    str r1,[r5,0x3C]
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done

    mov r0,0x0F
    mov r1,1
    mov r3,0
    ldr r4,[r5,0x04]
    add r4,5
    EngineCall CHAOS_IMPACT_EFFECT

    ldrb r0,[r5,0x06]
    ldrb r1,[r5,0x07]
    mov r2,0
    mov r3,0
    ldr r4,=0x1505FF0F
    ldr r6,[r5,0x2C]
    mov r7,3
    EngineCall CHAOS_DAMAGE_OBJECT
    cmp r0,0
    beq @@afterDamageObject
    ldrb r1,[r0]
    mov r2,0x10
    bic r1,r2
    ldrb r3,[r5]
    and r2,r3
    orr r1,r2
    strb r1,[r0]
@@afterDamageObject:
    ldrb r0,[r5,0x06]
    ldrb r1,[r5,0x07]
    mov r2,0x0F
    ldrb r3,[r5,0x04]
    bl ChaosAttackApplyPanels
    mov r0,3
    mov r1,0x14
    EngineCall SCREEN_SHAKE_SET
    mov r0,0x43
    add r0,0xFF
    EngineCall PLAY_SOUND
    ldr r4,=0x00010401
    bl ChaosFlashSpawn
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {r4,r6,r7,pc}
    .pool

// Target-panel blink used while the ball is forming and travelling.
ChaosAttackFlashTarget:
    push {lr}
    ldrb r0,[r5,0x04]
    cmp r0,0
    bne @@done
    ldrh r0,[r5,0x22]
    sub r1,r0,1
    strh r1,[r5,0x22]
    mov r1,4
    and r0,r1
    bne @@done
    ldrb r0,[r5,0x06]
    cmp r0,0
    beq @@done
    ldrb r1,[r5,0x07]
    EngineCall PANEL_SET_FLASH
@@done:
    pop {pc}
    .pool

// BN5 helper 0x081029A6 for pattern 0x0F. ChaosLord's variant 1 cracks
// every crackable panel in the 3x3 impact footprint; variant 0 performs the
// complementary solid-to-cracked transition.
ChaosAttackApplyPanels:
    push {r4-r7,lr}
    mov r6,r0
    mov r7,r1
    push {r3}
    EngineCall OBJECT_FRONT_DIRECTION
    mov r5,r0
    ldr r4,=ChaosAttackPanelPattern
@@loop:
    mov r0,0
    ldsb r0,[r4,r0]
    cmp r0,0x7F
    beq @@finish
    mul r0,r5
    add r0,r0,r6
    mov r1,1
    ldsb r1,[r4,r1]
    add r1,r1,r7
    ldr r3,[sp]
    cmp r3,0
    bne @@variantOne
    push {r0,r1}
    EngineCall PANEL_GET_FLAGS
    mov r1,0x40
    tst r0,r1
    pop {r0,r1}
    bne @@next
    EngineCall PANEL_CRACK_FROM_SOLID
    b @@next
@@variantOne:
    push {r0,r1}
    EngineCall PANEL_GET_FLAGS
    mov r1,0x10
    tst r0,r1
    pop {r0,r1}
    beq @@next
    EngineCall PANEL_CRACK
@@next:
    add r4,2
    b @@loop
@@finish:
    pop {r3}
    pop {r4-r7,pc}
    .pool

.align 4
ChaosAttackPanelPattern:
    .db 0,0, 0,-1, 0,1, 1,0, -1,0, 1,-1, -1,1, 1,1, -1,-1, 0x7F

// Exact four-entry sprite selector from BN5's type-3 0x82 object. The final
// two stages use Ball Bass animations 0x27/0x28 from its own archive.
.align 2
ChaosAttackSetSprite:
    push {r4,lr}
    ldr r4,=ChaosAttackSpriteTable
    lsl r0,r0,2
    add r4,r4,r0
    ldrb r1,[r4]
    ldrb r2,[r4,1]
    mov r0,0x80
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_NO_SHADOW
    ldrb r0,[r4,2]
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    ldrb r0,[r4,3]
    EngineCall SPRITE_SET_PALETTE
    pop {r4,pc}
    .pool

.align 4
ChaosAttackSpriteTable:
    .db 0x14,0x08,0x00,0x02
    .db 0x14,0x19,0x00,0x00
    .db 0x08,0x13,0x27,0x00
    .db 0x08,0x13,0x28,0x00

// ----- BN5 type-4 0x47 ChaosLord aura -------------------------------------

ChaosAuraMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl ChaosAuraInit
    pop {pc}
@@update:
    bl ChaosAuraUpdate
    pop {pc}
    .pool

ChaosAuraInit:
    push {lr}
    mov r0,0x80
    mov r1,0x08
    mov r2,0x12
    EngineCall SPRITE_LOAD
    ldrb r0,[r5,0x04]
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    EngineCall SPRITE_NO_SHADOW
    EngineCall OBJECT_GET_FLIP
    EngineCall SPRITE_SET_FLIP
    ldrh r0,[r5,0x3A]
    add r0,0x1A
    strh r0,[r5,0x3A]
    ldrh r0,[r5,0x3E]
    add r0,0x1A
    strh r0,[r5,0x3E]
    mov r0,4
    str r0,[r5,0x08]
    bl ChaosAuraUpdate
    pop {pc}
    .pool

ChaosAuraUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@appear
    cmp r0,4
    beq @@lifespan
    bl ChaosAuraFade
    b @@animate
@@appear:
    bl ChaosAuraAppear
    b @@animate
@@lifespan:
    bl ChaosAuraLifespan
@@animate:
    EngineCall SPRITE_PROPERTY_26C4
    pop {pc}
    .pool

ChaosAuraAppear:
    push {lr}
    ldrb r0,[r5,0x10]
    cmp r0,0x0D
    bne @@normal
    bl ChaosAuraAppearSpecial
    pop {pc}
@@normal:
    bl ChaosAuraAppearNormal
    pop {pc}

ChaosAuraAppearSpecial:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,2
    beq @@hold
    cmp r0,1
    beq @@grow
    mov r0,1
    strb r0,[r5,0x0A]
    mov r0,0x1E
    strh r0,[r5,0x20]
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
@@grow:
    ldrh r0,[r5,0x20]
    lsr r0,r0,1
    mov r1,r0
    EngineCall SPRITE_SET_BLEND
    ldrh r0,[r5,0x20]
    lsr r0,r0,1
    sub r0,0x10
    neg r0,r0
    EngineCall SPRITE_SET_BLEND_MODE
    ldrh r0,[r5,0x20]
    cmp r0,0x10
    ble @@count_once
    sub r0,1
@@count_once:
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,0x19
    strh r0,[r5,0x20]
    mov r0,2
    strb r0,[r5,0x0A]
@@hold:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,4
    strb r0,[r5,0x09]
@@done:
    pop {pc}
    .pool

ChaosAuraAppearNormal:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,2
    beq @@shrink
    cmp r0,1
    beq @@count_hidden
    mov r0,1
    strb r0,[r5,0x0A]
    mov r0,0x22
    strh r0,[r5,0x20]
@@count_hidden:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,0x0F
    strh r0,[r5,0x20]
    mov r0,2
    strb r0,[r5,0x0A]
@@shrink:
    ldrh r1,[r5,0x20]
    lsl r1,r1,1
    mov r0,0x20
    orr r0,r1
    lsl r0,r0,5
    orr r0,r1
    lsl r0,r0,5
    orr r0,r1
    EngineCall SPRITE_SET_SCALE
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bpl @@done
    mov r0,4
    strb r0,[r5,0x09]
@@done:
    pop {pc}
    .pool

ChaosAuraLifespan:
    push {lr}
    ldrh r0,[r5,0x06]
    sub r0,1
    strh r0,[r5,0x06]
    bne @@done
    mov r0,8
    strb r0,[r5,0x09]
@@done:
    pop {pc}

ChaosAuraFade:
    push {lr}
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldrb r0,[r5,0x05]
    lsr r0,r0,2
    bcs @@count
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
@@count:
    ldrb r0,[r5,0x05]
    sub r0,1
    strb r0,[r5,0x05]
    bne @@done
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {pc}

// ----- BN5 type-4 0x4B ChaosLord entrance burst ---------------------------

// BigHook supplied two released type-4 slots. Slot 0x8D hosts three kinds of
// ChaosLord helper object; +0x7C is a stable discriminator outside every
// object's mutable packed arguments.
ChaosBurstOrTeardownMain:
    ldr r0,[r5,0x7C]
    cmp r0,1
    beq @@teardown
    cmp r0,2
    beq @@flash
    b ChaosBurstMain
@@teardown:
    b ChaosTeardownMain
@@flash:
    b ChaosFlashMain

ChaosBurstMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    b @@animate
@@init:
    bl ChaosBurstInit
    b @@animate
@@update:
    bl ChaosBurstUpdate
@@animate:
    EngineCall OBJECT_UPDATE
    pop {pc}
    .pool

ChaosBurstInit:
    push {lr}
    mov r0,0x80
    mov r1,0x08
    mov r2,0x12
    EngineCall SPRITE_LOAD
    mov r0,0x15
    strb r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    ldrb r0,[r5,0x05]
    EngineCall SPRITE_SET_PALETTE
    ldrb r0,[r5,0x07]
    str r0,[r5,0x74]
    ldr r0,[r5,0x34]
    str r0,[r5,0x68]
    ldr r1,[r5,0x38]
    str r1,[r5,0x38]
    ldr r2,[r5,0x70]
    lsl r3,r2,16
    lsr r2,r2,16
    lsl r2,r2,16
    push {r2,r3}
    add r0,r0,r2
    str r0,[r5,0x34]
    neg r3,r3
    str r3,[r5,0x3C]
    asr r2,r2,16
    asr r3,r3,16
    mov r0,r2
    mul r2,r0
    mov r0,r3
    mul r0,r3
    add r0,r0,r2
    swi 8
    lsl r0,r0,16
    str r0,[r5,0x60]
    ldrb r1,[r5,0x04]
    swi 6
    str r0,[r5,0x64]
    pop {r1,r2}
    mov r0,r2
    EngineCall ANGLE_FROM_VECTOR
    strb r0,[r5,0x06]
    EngineCall OBJECT_SIDE_DIRECTION
    neg r0,r0
    strb r0,[r5,0x07]
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldr r0,[r5,0x74]
    cmp r0,0
    beq @@ready
    ldrh r0,[r5,0x3E]
    add r0,0x1B
    strh r0,[r5,0x3E]
@@ready:
    mov r0,4
    str r0,[r5,0x08]
    pop {pc}
    .pool

ChaosBurstUpdate:
    push {r4,r6,r7,lr}
    EngineCall OBJECT_SIDE_DIRECTION
    neg r4,r0
    ldrb r0,[r5,0x04]
    cmp r0,0x14
    blt @@under_twenty
    bgt @@velocity
    EngineCall RNG_NEXT
    mov r1,7
    and r0,r1
    add r0,3
    mul r0,r4
    strb r0,[r5,0x07]
    b @@velocity
@@under_twenty:
    cmp r0,0x0A
    bne @@under_ten
    EngineCall RNG_NEXT
    mov r1,7
    and r1,r0
    add r1,0x0A
    mul r1,r4
    ldrb r0,[r5,0x07]
    add r0,r0,r1
    strb r0,[r5,0x07]
    b @@angle
@@under_ten:
    cmp r0,5
    bne @@angle
    EngineCall RNG_NEXT
    mov r1,7
    and r1,r0
    add r1,0x10
    mul r1,r4
    ldrb r0,[r5,0x07]
    add r0,r0,r1
    strb r0,[r5,0x07]
    ldr r1,[r5,0x64]
    lsl r1,r1,1
    str r1,[r5,0x64]
@@angle:
    ldr r1,[r5,0x64]
    ldr r0,[r5,0x60]
    sub r0,r0,r1
    str r0,[r5,0x60]
    b @@move
@@velocity:
    ldr r1,[r5,0x64]
    ldr r0,[r5,0x60]
    asr r1,r1,2
    sub r0,r0,r1
    str r0,[r5,0x60]
@@move:
    ldrb r0,[r5,0x06]
    ldrb r1,[r5,0x07]
    sub r0,r0,r1
    strb r0,[r5,0x06]
    bl ChaosBurstSetPosition
    ldrb r0,[r5,0x04]
    sub r0,1
    strb r0,[r5,0x04]
    bne @@done
    mov r0,8
    str r0,[r5,0x08]
@@done:
    pop {r4,r6,r7,pc}
    .pool

ChaosBurstSetPosition:
    push {lr}
    ldrb r1,[r5,0x06]
    lsl r1,r1,1
    ldr r0,=ChaosTrigTable + 0x80
    ldsh r0,[r0,r1]
    ldr r1,[r5,0x60]
    mul r0,r1
    asr r0,r0,8
    ldr r1,[r5,0x68]
    add r0,r0,r1
    str r0,[r5,0x34]
    ldrb r1,[r5,0x06]
    lsl r1,r1,1
    ldr r0,=ChaosTrigTable
    ldsh r0,[r0,r1]
    ldr r1,[r5,0x60]
    mul r0,r1
    asr r0,r0,8
    neg r0,r0
    str r0,[r5,0x3C]
    ldr r0,[r5,0x74]
    cmp r0,0
    beq @@done
    ldrh r0,[r5,0x3E]
    add r0,0x1B
    strh r0,[r5,0x3E]
@@done:
    pop {pc}
    .pool

// ----- BN5 generic type-4 teardown effect, selector 0x12 ------------------
//
// The state machine is shared by BN5 and BN6, but selector 0x12 resolves to
// different art. This dedicated instance uses the exact BN5 group-0x14/
// index-0x0D archive relocated to an unused BN6 sprite slot.

ChaosTeardownMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl ChaosTeardownInit
    pop {pc}
@@update:
    bl ChaosTeardownUpdate
    pop {pc}

ChaosTeardownInit:
    push {lr}
    mov r0,0x80
    mov r1,0x10
    mov r2,0x04
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0
    strb r0,[r5,0x10]
    strb r0,[r5,0x11]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    ldrb r0,[r5,0x06]
    EngineCall SPRITE_SET_PALETTE
    ldrb r0,[r5,0x05]
    EngineCall SPRITE_SET_FLIP
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    ldrb r0,[r5,0x07]
    cmp r0,0
    beq @@ready
    mov r0,0
    EngineCall SPRITE_SET_BLEND_MODE
@@ready:
    mov r0,4
    str r0,[r5,0x08]
    pop {pc}
    .pool

ChaosTeardownUpdate:
    push {lr}
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    beq @@finish
    EngineCall SPRITE_GET_ANIMATION_FLAGS
    mov r1,0x80
    tst r0,r1
    beq @@animate
    mov r0,0x20
    ldsh r0,[r5,r0]
    cmp r0,0
    bgt @@animate
@@finish:
    ldrb r0,[r5]
    mov r1,2
    bic r0,r1
    strb r0,[r5]
    mov r0,8
    str r0,[r5,0x08]
@@animate:
    EngineCall SPRITE_PROPERTY_26C4
    pop {pc}
    .pool

// ----- BN5 type-4 0x0A variant-1 palette flash ---------------------------

ChaosFlashMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    ldrb r0,[r5,0x05]
    strh r0,[r5,0x22]
    mov r0,4
    str r0,[r5,0x08]
    bl ChaosFlashUpdate
    pop {pc}
@@update:
    bl ChaosFlashUpdate
    pop {pc}
    .pool

ChaosFlashUpdate:
    push {r4,lr}
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bpl @@apply
    mov r0,0x14
    EngineCall CHAOS_PALETTE_RESTORE
    mov r0,0x15
    EngineCall CHAOS_PALETTE_RESTORE
    EngineCall OBJECT_FREE
    pop {r4,pc}
@@apply:
    ldrb r0,[r5,0x07]
    cmp r0,0
    bne @@red
    ldr r1,=0x00007FFF
    b @@color_ready
@@red:
    mov r1,0x1F
@@color_ready:
    // BN6 splits the full scene between two palette caches. Update both so
    // the hit is a true full-screen white frame rather than a sprite-only
    // silhouette over the still-colored field.
    push {r1}
    mov r0,0
    mov r2,0x0F
    mov r3,0x14
    ldr r4,=0x03001B60
    EngineCall CHAOS_PALETTE_WRITE
    pop {r1}
    mov r0,0
    mov r2,0x0F
    mov r3,0x15
    ldr r4,=0x03001750
    EngineCall CHAOS_PALETTE_WRITE
    pop {r4,pc}
    .pool

ChaosCodeEnd:
.endautoregion

// Gregar replaces BigHook's menu art. Falzar keeps the original artwork in
// its corresponding Giga slot.
.if FALZAR
.else
    .autoregion
    .align 4
    ChaosLordIcon:
    .incbin "build/chaoslord-icon.bin"
    .endautoregion

    .autoregion
    .align 4
    ChaosLordImage:
    .incbin "build/chaoslord-image.bin"
    .endautoregion

    .autoregion
    .align 4
    ChaosLordPalette:
    .incbin "build/chaoslord-palette.bin"
    .endautoregion
.endif

// BN5 battle archives selected by the translated controller. Ball Bass uses
// animations from the same large Bass archive; the portal layers and entrance
// bursts share the compressed aura archive.
.autoregion
.align 4
ChaosBassSprite:
.incbin "build/chaoslord-bass-sprite.bin"
.endautoregion
.autoregion
.align 4
ChaosApparitionSprite:
.incbin "build/chaoslord-apparition-sprite.bin"
.endautoregion
.autoregion
.align 4
ChaosAuraSprite:
.incbin "build/chaoslord-aura-sprite.bin"
.endautoregion
.autoregion
.align 4
ChaosTrigTable:
.incbin "build/chaoslord-trig.bin"
.endautoregion
.autoregion
.align 4
ChaosTeardownSprite:
.incbin "build/chaoslord-teardown-sprite.bin"
.endautoregion
.autoregion
.align 4
ChaosImpactSprite:
.incbin "build/chaoslord-impact-sprite.bin"
.endautoregion
