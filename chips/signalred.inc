// ---------------------------------------------------------------------------
// SignalRed replaces Navi+20. Blue Moon implements it as an invisible
// time-freeze launcher plus a separate 100-HP traffic light. While the light
// is red it clears the opposing side's chip-enable bit; after 420 frames it
// opens a 50-frame green window, then repeats until the object is destroyed.

.definelabel SIGNALRED_ID,                  0x0C1

// Preserve Navi+20's Standard library position and gate byte while importing
// SignalRed's S code, Obstacle element, 61 MB cost, and zero behavior
// parameters from Blue Moon. The final ROM pass replaces its sort key.
// SignalRed shares the former BugRSword family route with BugCharge; record
// byte 0x0A selects it.
.org CHIP_DATA + SIGNALRED_ID * 0x2C
    .db 0x12,0xFF,0xFF,0xFF // S
    .db 0x00,0x04,0x07,0x00
    .db 0x3D,0x41,0x00,0x15
    .db 0x26,0x0A,0x04,0x00
    .db 0x00,0x00,0x00,0x00
    // Clear chip flag 0x02 at record offset 0x16. BN6's Rush handler tests
    // that bit at activation time to identify Image Invis-class chips.
    .db 0x00,0x14,0xC4,0x40
    .dh 0x0141,0,0x00C6
    .db 0x01,0xFF
    .dw SignalRedIcon,SignalRedImage,SignalRedPalette

.definelabel SIGNALRED_TIMEFREEZE_ENTRY,    0x0802CD4C // shared family 0x15, subfamily 0x26
.definelabel SIGNALRED_SFX_SPAWN,           0x01DA // relocated slot populated with Blue Moon SFX 0x00A0
.definelabel SIGNALRED_SFX_GREEN,           0x00D1 // BN6 match for Blue Moon SFX 0x015C
.if FALZAR
    .definelabel NATIVE_SHARED_TIMEFREEZE_SPAWN, 0x080E979C
.else
    .definelabel NATIVE_SHARED_TIMEFREEZE_SPAWN, 0x080EAADC
.endif
.definelabel DUST_FIRE_SPRITE_TABLE_PTR_0,  0x08012010
.definelabel DUST_FIRE_SPRITE_TABLE_PTR_1,  0x08012014
.if FALZAR
    .definelabel DUST_SUCTION_SPRITE_TABLE_PTR,0x080E9990
.else
    .definelabel DUST_SUCTION_SPRITE_TABLE_PTR,0x080EACD0
.endif

.org SIGNALRED_TIMEFREEZE_ENTRY
.dw SignalRedBugChargeTimeFreezeDispatch + 1

// DustCross stores a four-bit obstacle kind for each sucked object. Native
// BN6 uses kinds 0-14; reserve kind 15 for SignalRed and redirect both the
// firing path and the version-specific suction sprite loader to the extended
// table. The new entry selects SignalRed's appended group-0x10 archive.
.org DUST_FIRE_SPRITE_TABLE_PTR_0
.dw SignalRedDustSpriteTable
.org DUST_FIRE_SPRITE_TABLE_PTR_1
.dw SignalRedDustSpriteTable
.org DUST_SUCTION_SPRITE_TABLE_PTR
.dw SignalRedDustSpriteTable

.autoregion
.align 4
SignalRedCodeStart:

SignalRedDustSpriteTable:
    .dh 0x0904,0x240C,0x230C,0x300C,0x0010
    .dh 0x0810,0x1804,0x0A04,0x340C,0x350C
    .dh 0x0D04,0x0010,0x410C,0x0504,0x410C
    .dh (SIGNALRED_BATTLE_SPRITE_INDEX << 8) | 0x10

// BugCharge and SignalRed use a route shared by seven native chips. The high
// half of packed attack word r6 contains record byte 0x0A. SignalRed uses zero
// and BugCharge uses private marker 0x8B; preserve every other marker through
// the native launcher (including HubBatch 0x94 and BgDthThd 0x8A).
SignalRedBugChargeTimeFreezeDispatch:
    push {r0}
    mov r0,r6
    lsr r0,r0,16
    cmp r0,BUGCHARGE_ROUTE_MARKER
    beq @@bugcharge
    cmp r0,0
    beq @@signalred
    pop {r0}
    ldr r3,=NATIVE_SHARED_TIMEFREEZE_SPAWN + 1
    bx r3
@@signalred:
    pop {r0}
    b SignalRedTimeFreezeSpawn
@@bugcharge:
    pop {r0}
    ldr r3,=BugChargeTimeFreezeSpawn + 1
    bx r3
    .pool

// r6 is the attack ID; r7 is the native time-freeze completion pointer.
SignalRedTimeFreezeSpawn:
    push {lr}
    push {r0-r2,r5}
    mov r0,CUSTOM_TYPE4_ID
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
    mov r1,TYPE4_KIND_SIGNALRED
    str r1,[r0,CUSTOM_OBJECT_KIND]
@@done:
    pop {pc}
    .pool

SignalRedTimeFreezeMain:
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
    bl SignalRedTimeFreezeUpdate
    pop {pc}

SignalRedTimeFreezeUpdate:
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
    bl SignalRedLaunchEffect
    pop {pc}
    .pool

// Blue Moon leaves the time freeze 30 frames after creating the persistent
// signal. The traffic-light object continues updating in normal battle time.
SignalRedLaunchEffect:
    push {lr}
    ldrb r0,[r5,0x0B]
    cmp r0,0
    bne @@tick
    bl SignalRedSpawnPersistent
    mov r0,30
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0B]
@@tick:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bne @@done
    mov r0,0x0C
    strb r0,[r5,0x09]
    mov r0,0
    strh r0,[r5,0x0A]
@@done:
    pop {pc}

// Create the light on the panel immediately in front of the chip user. The
// BN6 panel-coordinate conversion replaces Blue Moon's equivalent helper;
// the resulting fixed-point coordinates are passed through BN6's type-3
// allocator. The persistent light uses its own project Type-3 hub kind.
SignalRedSpawnPersistent:
    push {r4,r6,r7,lr}
    ldr r4,[r5,0x4C]
    ldrb r6,[r4,0x12]
    ldrb r7,[r4,0x13]
    push {r5}
    mov r5,r4
    EngineCall OBJECT_FRONT_DIRECTION
    pop {r5}
    add r6,r6,r0
    mov r0,r6
    mov r1,r7
    EngineCall PANEL_IS_VALID_XY
    cmp r0,0
    beq @@failed
    mov r0,r6
    mov r1,r7
    EngineCall PANEL_TO_COORDS
    mov r2,r1
    mov r1,r0
    mov r3,0
    push {r4-r7}
    mov r4,0
    mov r0,CUSTOM_TYPE3_ID
    EngineCall SPAWN_TYPE3
    mov r0,r5
    pop {r4-r7}
    cmp r0,0
    beq @@failed
    strb r6,[r0,0x12]
    strb r7,[r0,0x13]
    ldrb r1,[r4,0x16]
    strb r1,[r0,0x16]
    str r4,[r0,0x4C]
    ldr r1,[r5,0x2C]
    str r1,[r0,0x2C]
    mov r1,TYPE3_KIND_SIGNALRED
    str r1,[r0,CUSTOM_OBJECT_KIND]
    ldrb r1,[r0]
    mov r2,0x10
    orr r1,r2
    strb r1,[r0]
    ldrb r1,[r4,0x16]
    // Register in BN6's one-object-per-side deployable slot. DustCross scans
    // every slot regardless of owner, and the update path below accepts both
    // owner-specific suction bits, so either player can vacuum the light.
    mov r2,1
    EngineCall OBJECT_REGISTER_DEPLOYABLE
    pop {r4,r6,r7,pc}
@@failed:
    mov r0,0
    pop {r4,r6,r7,pc}
    .pool

SignalRedObjectMain:
    push {lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    bl SignalRedObjectDestroy
    pop {pc}
@@init:
    bl SignalRedObjectInit
    pop {pc}
@@update:
    bl SignalRedObjectUpdate
    pop {pc}

SignalRedObjectInit:
    push {r4,r6,r7,lr}
    EngineCall PANEL_IS_VALID_OBJECT
    cmp r0,0
    beq @@free
    mov r0,0x80
    mov r1,0x10
    mov r2,SIGNALRED_BATTLE_SPRITE_INDEX
    EngineCall SPRITE_LOAD
    EngineCall SPRITE_PROPERTY_2E3C
    mov r0,0
    strh r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    EngineCall SPRITE_PROPERTY_26C4
    ldrb r0,[r5,0x16]
    EngineCall SPRITE_SET_FLIP
    EngineCall OBJECT_SET_COORDS
    ldrb r0,[r5]
    mov r1,2
    orr r0,r1
    strb r0,[r5]

    // Blue Moon's object is a normal target with 100 HP. Use BN6's native
    // passive, destructible field-object pair (14/15, the settled RockCube
    // pair) so incoming attacks resolve against a normal field obstacle.
    mov r0,100
    strh r0,[r5,0x24]
    strh r0,[r5,0x26]
    mov r0,3
    strh r0,[r5,0x22]
    ldr r0,=0x00000960
    strh r0,[r5,0x20]
    EngineCall COLLISION_CREATE
    mov r7,r0
    cmp r0,0
    beq @@free

    // Blue Moon registers deployables before init, starts on animation 0
    // (red), waits three updates in the existing +0x22 timer, then plays its
    // placement cue and starts the first full red interval. Bit 7 of +0x07
    // defers collision setup until the dimming cut-in has ended, so the
    // persistent hurtbox begins in normal battle time. Keep +0x0B clear
    // because BN6's shared wind-removal handler owns that byte.
    mov r0,0
    strb r0,[r5,0x06]
    strb r0,[r5,0x0B]
    mov r0,0x81
    strb r0,[r5,0x07]
    mov r0,4
    str r0,[r5,0x08]
    EngineCall OBJECT_UPDATE
    pop {r4,r6,r7,pc}
@@free:
    // Registration happens before the first object update. Even an invalid
    // panel or collision-allocation failure must remove that list entry;
    // leaving a freed pointer for DustCross is an intermittent hard lock.
    bl SignalRedObjectDestroy
    pop {r4,r6,r7,pc}
    .pool

SignalRedObjectUpdate:
    push {lr}
    // COLLISION_SETUP registers immediately. Do not expose the persistent
    // field-object hurtbox until the time-stop controller has finished.
    ldrb r0,[r5,0x07]
    mov r1,0x80
    tst r0,r1
    beq @@collision_ready
    EngineCall BATTLE_IS_TIME_STOPPED
    cmp r0,0
    beq @@setup_collision
    EngineCall OBJECT_UPDATE
    pop {pc}
@@setup_collision:
    ldr r0,[r5,0x54]
    mov r1,14
    mov r2,15
    mov r3,3
    EngineCall COLLISION_SETUP
    mov r1,14
    EngineCall COLLISION_PRESENT
    ldrb r0,[r5,0x07]
    mov r1,0x80
    bic r0,r1
    strb r0,[r5,0x07]
@@collision_ready:
    // Collect the five element-specific collision damage slots into +0x80,
    // run deployable lifetime maintenance, and apply that total with BN6's
    // saturating object-HP subtractor. FIELD_COLLISION_UPDATE begins by
    // clearing the prior hit presentation itself; the broader Navi damage
    // state machine is not used because it requires a separate status object.
    EngineCall FIELD_COLLISION_UPDATE
    EngineCall DEPLOYABLE_LIFETIME_UPDATE
    ldr r1,[r5,0x54]
    mov r0,0x80
    ldrh r0,[r1,r0]
    cmp r0,0
    beq @@damage_applied
    EngineCall OBJECT_APPLY_DAMAGE
@@damage_applied:
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    beq @@battle_active
    b SignalRedObjectBeginDestroy
@@battle_active:
    ldrh r0,[r5,0x24]
    cmp r0,0
    bne @@has_hp
    b SignalRedObjectBeginDestroy
@@has_hp:
    // Native field objects do not advance state while a dimming chip owns the
    // time-freeze lifecycle. In particular, never flip the opponent's chip
    // enable bit halfway through a cut-in.
    EngineCall BATTLE_IS_TIME_STOPPED
    cmp r0,0
    beq @@check_removal
    b SignalRedObjectAnimate
@@check_removal:
    EngineCall COLLISION_GET_FLAGS
    // DustCross first raises the generic removal bit (0x8000), then one of
    // the owner-specific suction bits (0x100000/0x200000). Native BN6
    // deployables serialize their appearance into DustCross's ammo list
    // before removing the field object; kind 15 uses the extended table above.
    ldr r1,=0x00300000
    tst r0,r1
    beq @@not_dust
    b SignalRedObjectStoreDustAmmo
@@not_dust:
    // 0x40000 is BN6's separate, timed wind-removal path.
    mov r1,4
    lsl r1,r1,16
    tst r0,r1
    bne @@wind_remove
    b SignalRedObjectNormalUpdate
@@wind_remove:
    EngineCall OBJECT_UPDATE_SUCTION
    cmp r0,1
    bne @@still_present
    b SignalRedObjectBeginDestroy
@@still_present:
    // The shared handler owns visibility and its 20-frame timer during this
    // separate wind-removal path. Do not advance the light cycle.
    pop {pc}
SignalRedObjectStoreDustAmmo:
    mov r0,0x0F
    EngineCall OBJECT_STORE_DUST_AMMO
    // Match native deployables: once the suction sprite has captured the
    // object's appearance, unregister and remove the field object immediately.
    bl SignalRedObjectDestroy
    pop {pc}
SignalRedObjectNormalUpdate:
    ldrb r0,[r5,0x07]
    cmp r0,0
    beq SignalRedObjectCycleUpdate
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bne SignalRedObjectAnimate
    mov r0,0
    strb r0,[r5,0x07]
SignalRedPlaySpawnSound:
    mov r0,SIGNALRED_SFX_SPAWN >> 1
    lsl r0,r0,1
    EngineCall PLAY_SOUND
    mov r0,0x69
    lsl r0,r0,2
    strh r0,[r5,0x22]
    bl SignalRedClearChipFlag
    b SignalRedObjectAnimate
SignalRedObjectCycleUpdate:
    ldrh r0,[r5,0x22]
    sub r0,1
    strh r0,[r5,0x22]
    bpl SignalRedObjectAnimate
    ldrb r0,[r5,0x06]
    cmp r0,0
    beq SignalRedObjectTurnGreen

    // Green -> red: close chip use for another 420 frames.
    mov r0,0
    strb r0,[r5,0x06]
    strb r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    mov r0,0x69
    lsl r0,r0,2
    strh r0,[r5,0x22]
    bl SignalRedClearChipFlag
    b SignalRedObjectAnimate
SignalRedObjectTurnGreen:
    // Red -> green: restore chip use for Blue Moon's 50-frame window.
    mov r0,4
    strb r0,[r5,0x06]
    mov r0,1
    strb r0,[r5,0x10]
    EngineCall SPRITE_SET_ANIMATION
    EngineCall SPRITE_LOAD_ANIM_DATA
    mov r0,50
    strh r0,[r5,0x22]
    bl SignalRedSetChipFlag
SignalRedPlayGreenSound:
    mov r0,SIGNALRED_SFX_GREEN
    EngineCall PLAY_SOUND
SignalRedObjectAnimate:
    // Persistent hurtboxes must be submitted on every active frame.
    mov r1,14
    EngineCall COLLISION_PRESENT
    EngineCall OBJECT_UPDATE
    pop {pc}

SignalRedObjectBeginDestroy:
    mov r0,8
    str r0,[r5,0x08]
    pop {pc}
    .pool

SignalRedObjectDestroy:
    push {lr}
    bl SignalRedSetChipFlag
    EngineCall OBJECT_UNREGISTER_DEPLOYABLE
    ldr r0,[r5,0x54]
    cmp r0,0
    beq @@free
    EngineCall COLLISION_REMOVE
    EngineCall COLLISION_CLEAR_REGION
    ldr r0,[r5,0x54]
    EngineCall COLLISION_FREE
@@free:
    EngineCall OBJECT_FREE
    pop {pc}
    .pool

// BN4 and BN6 assign the same opposing-chip flags: owner 0 controls bit 3
// (0x08), owner 1 controls bit 2 (0x04).
SignalRedOwnerChipFlag:
    mov r0,8
    ldrb r1,[r5,0x16]
    cmp r1,0
    beq @@done
    mov r0,4
@@done:
    bx lr

SignalRedClearChipFlag:
    push {lr}
    bl SignalRedOwnerChipFlag
    EngineCall BATTLE_FLAGS_CLEAR
    pop {pc}

SignalRedSetChipFlag:
    push {lr}
    bl SignalRedOwnerChipFlag
    EngineCall BATTLE_FLAGS_SET
    pop {pc}
    .pool

SignalRedCodeEnd:
.endautoregion

// Blue Moon SFX 0x00A0. BN6 SFX 0x0180 has the same note stream but a
// different sample, priority, and timbre, so import the original cue intact.
.autoregion
.align 4
SignalRedSpawnAudioStart:
SignalRedSpawnSong:
    .db 0x01,0x00,0x40,0x00
    .dw SignalRedSpawnVoicegroup
    .dw SignalRedSpawnTrack
SignalRedSpawnVoicegroup:
    .db 0x08,0x3C,0x00,0x00
    .dw SignalRedSpawnSample
    .db 0xFF,0x00,0xFF,0x00
SignalRedSpawnTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBF,0x40
    .db 0xBE,0x7F,0xDC,0x3C,0x7F,0x8D,0xB1,0x00
.align 4
SignalRedSpawnSample:
    .incbin "build/signalred-spawn-sample.bin"
SignalRedSpawnAudioEnd:
.endautoregion

// Both versions use Blue Moon's menu art and battle sprite.
.autoregion
.align 4
SignalRedIcon:
    .incbin "build/signalred-icon.bin"
.endautoregion

.autoregion
.align 4
SignalRedImage:
    .incbin "build/signalred-image.bin"
.endautoregion

.autoregion
.align 4
SignalRedPalette:
    .incbin "build/signalred-palette.bin"
.endautoregion

.autoregion
.align 4
SignalRedBattleSprite:
.incbin "build/signalred-battle-sprite.bin"
.endautoregion
