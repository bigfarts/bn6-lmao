// ---------------------------------------------------------------------------
// Jealousy replaces LifeSync. BN5 implements the chip as an invisible type-4
// controller: it finds the largest opposing loaded-chip count, emits one
// 80-damage full-field pulse per chip, displays the chip-delete overlay, and
// clears the opponent's queued chips before leaving time freeze.

.definelabel JEALOUSY_ID,                    0x0BF

// LifeSync's slot becomes BN5 Jealousy. BN5 null element 0x0C maps to BN6
// null element 0x0A; family/subfamily stay on LifeSync's released BN6 route.
.org CHIP_DATA + JEALOUSY_ID * 0x2C
    .db 0x09,0xFF,0xFF,0xFF // J
    .db 0x00,0x03,0x0A,0x00
    .db 0x3C,0x43,0x8A,0x15
    .db 0x07,0x0A,0x04,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x80,0x10
    .dh 0x008A,80,0x00C3
    .db 0x01,0xFF
    .dw JealousyIcon,JealousyImage,JealousyPalette

.definelabel JEALOUSY_TIMEFREEZE_ENTRY,      0x0802CCD0 // family 0x15, subfamily 0x07

// BN6 equivalents of the BN5 Jealousy-specific helpers. The generic
// time-freeze lifecycle entry points shared with SignalRed live in common.inc.
.definelabel JEALOUSY_DISPLAY_SETUP,         0x08000AC8
.definelabel JEALOUSY_LINK_BATTLE_ACTIVE,    0x0800A8F8
.definelabel JEALOUSY_COMPARE_LOCAL_SIDE,    0x0800A9EC
.definelabel JEALOUSY_DRAW_DELETE_OVERLAY,   0x0800AE90
.definelabel JEALOUSY_CHIP_LIST,             0x08010018
.definelabel JEALOUSY_GAUGE_SUBTRACT,        0x0802E04E

.org JEALOUSY_TIMEFREEZE_ENTRY
.dw JealousyTimeFreezeSpawn + 1

.autoregion
.align 4
JealousyCodeStart:

// BN5 attack-family wrapper at 0x080E4540, translated to the unused native
// BN6 Type-4 hub. r6 is the attack ID and r7 is the time-freeze completion
// pointer supplied by the common attack-family dispatcher.
JealousyTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,CUSTOM_TYPE4_ID
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1-r3,r5}
    beq @@done
    strb r1,[r0,0x12]
    strb r2,[r0,0x13]
    strb r3,[r0,0x0E]
    str r5,[r0,0x4C]
    ldrb r1,[r5,0x16]
    strb r1,[r0,0x16]
    str r6,[r0,0x2C]
    str r7,[r0,0x30]
    mov r1,TYPE4_KIND_JEALOUSY
    str r1,[r0,CUSTOM_OBJECT_KIND]
@@done:
    pop {pc}
    .pool

// The outer and middle state handlers are BN6's native type-4 time-freeze
// lifecycle. Only substate 8 is chip-specific, exactly as in BN5.
JealousyMain:
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
    bl JealousyUpdate
    pop {pc}

JealousyUpdate:
    push {lr}
    ldrb r0,[r5,0x09]
    cmp r0,0
    beq @@intro
    cmp r0,4
    beq @@timestop
    cmp r0,8
    beq @@effect
    EngineCall TYPE4_TIMESTOP_OUTRO
    pop {pc}
@@intro:
    EngineCall TYPE4_TIMESTOP_INTRO
    pop {pc}
@@timestop:
    EngineCall TYPE4_TIMESTOP_FREEZE
    pop {pc}
@@effect:
    bl JealousyEffectUpdate
    pop {pc}
    .pool

JealousyEffectUpdate:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    beq JealousyEffectInit
    cmp r0,4
    beq JealousyPulsePhase
    bl JealousyDeletePhase
    pop {pc}

JealousyEffectInit:
    ldr r3,=JealousyTilesTransfer
    ldmia r3!,{r0-r2}
    EngineCall JEALOUSY_DISPLAY_SETUP
    ldr r3,=JealousyPaletteTransfer
    ldmia r3!,{r0-r2}
    EngineCall JEALOUSY_DISPLAY_SETUP
    bl JealousyMaxLoadedChips
    str r0,[r5,0x60]
    cmp r0,0
    beq @@no_chips
    mov r0,1
    strh r0,[r5,0x20]
    mov r0,4
    strh r0,[r5,0x0A]
    pop {pc}
@@no_chips:
    mov r0,8
    strh r0,[r5,0x0A]
    pop {pc}
    .pool

// BN5 spaces damage pulses ten frames apart. The number of successful field
// scans is capped by the opponent's largest loaded-chip count.
JealousyPulsePhase:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bne @@done
    mov r0,10
    strh r0,[r5,0x20]
    bl JealousyAttackField
    cmp r0,0
    beq @@finish
    ldr r0,[r5,0x60]
    sub r0,1
    str r0,[r5,0x60]
    bne @@done
@@finish:
    mov r0,8
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

// Keep BN5's 90-frame delete phase. The overlay is refreshed while the
// timer is at least 20; cleanup then clears the opposing custom gauge and
// hands control to the native type-4 outro.
JealousyDeletePhase:
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    mov r0,90
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    cmp r0,20
    blt @@countdown
    bl JealousyRefreshDeleteOverlay
@@countdown:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    bl JealousyFinishDelete
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

// Return the maximum number of loaded chips among the four units on the
// opposing side. This is a direct translation of BN5 0x080E4562.
JealousyMaxLoadedChips:
    push {r4,r6,r7,lr}
    mov r7,r10
    ldr r7,[r7,0x18]
    ldrb r0,[r5,0x16]
    mov r1,1
    eor r0,r1
    mov r1,0x10
    mul r0,r1
    add r7,0x80
    add r7,r7,r0
    mov r6,0
    mov r4,0
@@loop:
    ldr r0,[r7]
    cmp r0,0
    beq @@next
    bl JealousyUnitLoadedChips
    cmp r0,r4
    blt @@next
    mov r4,r0
@@next:
    add r7,4
    add r6,1
    cmp r6,4
    blt @@loop
    mov r0,r4
    pop {r4,r6,r7,pc}

JealousyUnitLoadedChips:
    push {lr}
    ldr r1,[r0,0x58]
    cmp r1,0
    beq @@none
    ldrb r2,[r1]
    cmp r2,2
    bne @@direct_count
    ldrb r0,[r0,0x16]
    EngineCall JEALOUSY_CHIP_LIST
    ldrb r1,[r0]
    add r1,r1,r1
    add r1,2
    add r1,r1,r0
    mov r0,0
    ldr r2,=0x0000FFFF
@@scan:
    ldrh r3,[r1]
    cmp r3,r2
    beq @@done
    add r0,1
    add r1,2
    b @@scan
@@direct_count:
    ldrb r0,[r0,0x1A]
@@done:
    pop {pc}
@@none:
    mov r0,0
    pop {pc}
    .pool

// Apply the Jealousy hit object to every valid opposing panel. BN6's panel
// ownership flags are unchanged; its collision object's layout is handled by
// the version-specific damage-object address defined in gregar/falzar.asm.
JealousyAttackField:
    push {r4,r6,r7,lr}
    ldrb r1,[r5,0x16]
    cmp r1,0
    bne @@right_side
    ldr r2,=0x00200000
    b @@mask_ready
@@right_side:
    ldr r2,=0x00400000
@@mask_ready:
    mov r3,0
    mov r4,0
    mov r0,6
@@column:
    mov r1,3
@@row:
    push {r0-r4}
    EngineCall PANEL_HAS_FLAGS
    cmp r0,0
    pop {r0-r4}
    beq @@next
    push {r0-r4}
    ldrb r2,[r5,0x0E]
    ldr r4,=0x1A050601
    ldr r6,[r5,0x2C]
    ldrh r3,[r5,0x32]
    add r6,r6,r3
    mov r3,0
    mov r7,3
.if FALZAR
    EngineCall 0x080C53A6
.else
    EngineCall 0x080C6C16
.endif
    pop {r0-r4}
    add r4,1
@@next:
    sub r1,1
    bne @@row
    sub r0,1
    bne @@column
    mov r0,r4
    pop {r4,r6,r7,pc}
    .pool

JealousyFinishDelete:
    push {lr}
    EngineCall JEALOUSY_LINK_BATTLE_ACTIVE
    cmp r0,0
    beq @@done
    ldrb r0,[r5,0x16]
    mov r1,1
    eor r0,r1
    mov r1,0x40
    lsl r1,r1,8
    EngineCall JEALOUSY_GAUGE_SUBTRACT
@@done:
    pop {pc}
    .pool

JealousyRefreshDeleteOverlay:
    push {lr}
    EngineCall JEALOUSY_LINK_BATTLE_ACTIVE
    cmp r0,0
    beq @@done
    ldrb r0,[r5,0x16]
    EngineCall JEALOUSY_COMPARE_LOCAL_SIDE
    cmp r0,0
    beq @@done
    mov r0,120
    mov r1,12
    EngineCall JEALOUSY_DRAW_DELETE_OVERLAY
@@done:
    pop {pc}
    .pool

.align 4
JealousyTilesTransfer:
    .dw JealousyEffectTiles,0x06017940,0x00000100
JealousyPaletteTransfer:
    // BN6 relocated this palette staging buffer by 0x2000 from BN5.
    // Keeping BN5's 0x030036F0 address corrupts unrelated BN6 IWRAM.
    .dw JealousyEffectPalette,0x030016F0,0x00000020

JealousyCodeEnd:
.endautoregion

.autoregion
.align 4
JealousyIcon:
.incbin "build/jealousy-icon.bin"
.endautoregion

.autoregion
.align 4
JealousyImage:
.incbin "build/jealousy-image.bin"
.endautoregion

.autoregion
.align 4
JealousyPalette:
.incbin "build/jealousy-palette.bin"
.endautoregion

.autoregion
.align 4
JealousyEffectTiles:
.incbin "build/jealousy-effect-tiles.bin"
.endautoregion

.autoregion
.align 4
JealousyEffectPalette:
.incbin "build/jealousy-effect-palette.bin"
.endautoregion
