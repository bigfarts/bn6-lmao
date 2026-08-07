// ---------------------------------------------------------------------------
// BugChain replaces CopyDamage. Blue Moon implements it as an invisible
// time-freeze controller which waits 60 frames, then copies the user's active
// battle bugs to the opposing side. BN6 expanded the battle-property block,
// so the transfer uses the complete set of fields cleared by BN6 BugFix.

.definelabel BUGCHAIN_ID,                    0x0BE

// Preserve CopyDamage's Standard-chip library position and sort metadata while
// importing BugChain's C/* codes, rarity, 59 MB cost, and support-chip power.
.org CHIP_DATA + BUGCHAIN_ID * 0x2C
    .db 0x02,0x1A,0xFF,0xFF // C, *
    .db 0x00,0x03,0x0A,0x00
    .db 0x3B,0x41,0x00,0x15
    .db 0x22,0x0A,0x04,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0xC2,0x80,0x01
    .dh 0x006E,0,0x00C2
    .db 0x03,0xFF
    .dw BugChainIcon,BugChainImage,BugChainPalette

.definelabel BUGCHAIN_TIMEFREEZE_ENTRY,      0x0802CD3C // unused family 0x15, subfamily 0x22
.definelabel TYPE4_BUGCHAIN_SHARED_ENTRY,    0x08004438 // shared with Jealousy's type-4 slot 0x5C
.definelabel BUGCHAIN_SFX,                   0x01DF // exact Blue Moon SFX 0x015D
.definelabel BUGCHAIN_TAG,                   0x48434742 // "BGCH"
.definelabel BUGCHAIN_VISUAL_TAG,            0x56434742 // "BGCV"
.org BUGCHAIN_TIMEFREEZE_ENTRY
.dw BugChainTimeFreezeSpawn + 1

// Jealousy already owns LifeSync's released type-4 slot. BugChain tags its
// controller in the otherwise-unused object tail and shares the slot safely.
.org TYPE4_BUGCHAIN_SHARED_ENTRY
.dw BugChainSharedMain + 1

.autoregion
.align 4
BugChainCodeStart:

BugChainTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,0x5C
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
    ldr r1,=BUGCHAIN_TAG
    str r1,[r0,0x78]
@@done:
    pop {pc}
    .pool

BugChainSharedMain:
    ldr r0,[r5,0x78]
    ldr r1,=BUGCHAIN_TAG
    cmp r0,r1
    beq BugChainMain
    ldr r1,=BUGCHAIN_VISUAL_TAG
    cmp r0,r1
    bne @@jealousy
    ldr r0,=BugChainVisualMain + 1
    bx r0
@@jealousy:
    ldr r0,=JealousyMain + 1
    bx r0
    .pool

BugChainMain:
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
    bl BugChainUpdate
    pop {pc}

BugChainUpdate:
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
    bl BugChainEffectUpdate
    pop {pc}
    .pool

// Blue Moon applies BugChain only in a link battle. Retain its 60-frame effect
// beat, then duplicate the user's bugs onto the opponent and enter the native
// time-freeze outro. The source Navi keeps its own bugs, as in Blue Moon.
BugChainEffectUpdate:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    EngineCall BATTLE_CONFIG_FLAGS
    mov r1,0x08
    tst r0,r1
    beq @@finish
    mov r0,0
    EngineCall PLAYER_OBJECT_FOR_SIDE
    cmp r0,0
    beq @@right_visual
    bl BugChainSpawnVisual
@@right_visual:
    mov r0,1
    EngineCall PLAYER_OBJECT_FOR_SIDE
    cmp r0,0
    beq @@visuals_done
    bl BugChainSpawnVisual
@@visuals_done:
    mov r0,60
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@done
    bl BugChainTransferBugs
@@finish:
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}
    .pool

// BN6 BugFix clears these nine byte fields and one halfword field. For every
// active source bug, keep the larger value so BugChain can add a new bug or
// increase its severity without weakening a bug already on the target.
BugChainTransferBugs:
    push {r4,r6,r7,lr}
    ldrb r6,[r5,0x16]
    mov r7,r6
    mov r1,1
    eor r7,r1
    mov r0,r6
    EngineCall PLAYER_PROPERTIES_FOR_SIDE
    mov r6,r0
    mov r0,r7
    EngineCall PLAYER_PROPERTIES_FOR_SIDE
    mov r7,r0
    ldr r4,=BugChainByteProperties
@@byte_loop:
    ldrb r1,[r4]
    add r4,1
    cmp r1,0xFF
    beq @@halfword
    ldrb r0,[r6,r1]
    cmp r0,0
    beq @@byte_loop
    ldrb r2,[r7,r1]
    cmp r0,r2
    bls @@byte_loop
    strb r0,[r7,r1]
    b @@byte_loop
@@halfword:
    mov r1,0x54
    ldrh r0,[r6,r1]
    cmp r0,0
    beq @@done
    ldrh r2,[r7,r1]
    cmp r0,r2
    bls @@done
    strh r0,[r7,r1]
@@done:
    pop {r4,r6,r7,pc}
    .pool

// Blue Moon attaches one 50-frame aura object to each player while the
// controller's 60-frame transfer timer runs. It uses the same released type-4
// slot as the controller and Jealousy, distinguished by a second private tag.
BugChainSpawnVisual:
    push {lr}
    push {r0,r5}
    mov r0,0x5C
    EngineCall SPAWN_TYPE4
    mov r0,r5
    pop {r1,r5}
    cmp r0,0
    beq @@done
    str r1,[r0,0x4C]
    ldr r2,=BUGCHAIN_VISUAL_TAG
    str r2,[r0,0x78]
@@done:
    pop {pc}
    .pool

BugChainVisualMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {pc}
@@init:
    bl BugChainVisualInit
    b @@animate
@@update:
    bl BugChainVisualUpdate
@@animate:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {pc}

BugChainVisualInit:
    push {lr}
    mov r0,0x80
    mov r1,0x10
    mov r2,0x5C
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_NO_SHADOW
    mov r0,0
    strh r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    ldr r1,[r5,0x4C]
    ldrb r0,[r1,0x16]
    strb r0,[r5,0x16]
    EngineCall SPRITE_SET_FLIP
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]
    mov r0,50
    strh r0,[r5,0x20]
    mov r0,4
    str r0,[r5,0x08]
    bl BugChainVisualUpdate
    pop {pc}
    .pool

BugChainVisualUpdate:
    push {lr}
    ldrh r0,[r5,0x20]
    cmp r0,42
    bne @@countdown
    mov r0,BUGCHAIN_SFX - 0xFF
    add r0,0xFF
    EngineCall PLAY_SOUND
@@countdown:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bge @@position
    mov r0,8
    str r0,[r5,0x08]
@@position:
    bl BugChainVisualCopyCoords
    pop {pc}
    .pool

BugChainVisualCopyCoords:
    ldr r0,[r5,0x4C]
    cmp r0,0
    beq @@done
    add r0,0x34
    ldmia r0!,{r1-r3}
    mov r0,0x34
    add r0,r5
    stmia r0!,{r1-r3}
@@done:
    bx lr

.align 4
BugChainByteProperties:
    .db 0x31,0x13,0x14,0x16,0x24,0x19,0x18,0x1A,0x63,0xFF

BugChainCodeEnd:
.endautoregion

// Exact Blue Moon SFX 0x015D. Its two aura objects both issue this cue when
// their original 50-frame animation timer reaches 42.
.autoregion
.align 4
BugChainSoundAudioStart:
BugChainSoundSong:
    .db 0x01,0x00,0x40,0x00
    .dw BugChainSoundVoicegroup
    .dw BugChainSoundTrack
BugChainSoundVoicegroup:
    .db 0x00,0x3C,0x00,0x00
    .dw BugChainSoundSample
    .db 0xFF,0x00,0xFF,0x00
BugChainSoundTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x7F,0xD7,0x2E,0x7F,0x88,0xDB,0x39
    .db 0x8C,0xB1
.align 4
BugChainSoundSample:
    .incbin "build/bugchain-sound-sample.bin"
BugChainSoundAudioEnd:
.endautoregion

.autoregion
.align 4
BugChainIcon:
.incbin "build/bugchain-icon.bin"
.endautoregion

.autoregion
.align 4
BugChainImage:
.incbin "build/bugchain-image.bin"
.endautoregion

.autoregion
.align 4
BugChainPalette:
.incbin "build/bugchain-palette.bin"
.endautoregion

.autoregion
.align 4
BugChainBattleSprite:
.incbin "build/bugchain-battle-sprite.bin"
.endautoregion
