// ---------------------------------------------------------------------------
// FolderBack replaces the dormant Falzar Giga chip. The original BN3 effect
// restores the user's complete equipped Folder (including FolderBack itself),
// discards the current hand, and immediately returns both players to Custom.

.definelabel FOLDERBACK_ID,                    0x139

// Retain Falzar's released Giga metadata and private family route, but remove
// its version-only flag so FolderBack is legal in both builds. FolderBack uses
// its original BN3 wildcard code, 99 MB cost, and support-chip power.
.org CHIP_DATA + FOLDERBACK_ID * 0x2C
    .db 0x1A,0xFF,0xFF,0xFF // *
    .db 0x00,0x04,0x0A,0x02
    .db 0x63,0x41 // hide attack power; enabled in both versions
    .db 0x00,0x15 // ordinary chip: BN3 FolderBack has no time-freeze cut-in
    .db 0x23,0x0A,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x0D,0x10,0x00
    .dh 0x00CD,0,FOLDERBACK_ID
    .db 0x01,0xFF
    .dw FolderBackIcon,FolderBackImage,FolderBackPalette

.definelabel FOLDERBACK_ATTACK_ENTRY,          0x0802CD40 // family 0x15, subfamily 0x23
.definelabel TYPE4_FOLDERBACK_SHARED_ENTRY,    0x080044D8 // shared with SignalRed type-4 slot 0x84
.definelabel FOLDERBACK_TAG,                   0x46424B36 // "FBK6"

.definelabel FOLDERBACK_REBUILD_FOLDER,        0x0800A318
.definelabel FOLDERBACK_CLEAR_HAND,            0x0800A954
.definelabel FOLDERBACK_COMPARE_LOCAL_SIDE,    0x0800A9EC
.definelabel FOLDERBACK_LOCK_BATTLE_STATE,     0x0800A028
.definelabel FOLDERBACK_SET_HUD_GAUGE,         0x0801DFA2
.definelabel FOLDERBACK_BEGIN_LOCAL_CUSTOM,    0x0801E15C
.definelabel FOLDERBACK_BATTLE_STATE,          0x0203CA70
.definelabel FOLDERBACK_PLAYER_DATA,           0x0802E070
.definelabel FOLDERBACK_SHUFFLE_QUEUE,          0x0800A570
.definelabel FOLDERBACK_QUEUE,                  0x0203CDB0
.definelabel FOLDERBACK_NATIVE_TYPE1_TABLE,     0x08003C9C
.definelabel FOLDERBACK_TYPE4_OBJECT_BASE,      0x02036870
.definelabel FOLDERBACK_TYPE4_OBJECT_SIZE,      0xC8
.definelabel FOLDERBACK_TYPE4_OBJECT_COUNT,     0x20

.org FOLDERBACK_ATTACK_ENTRY
.dw FolderBackSpawn + 1

// SignalRed and BugCharge already share type-4 slot 0x84. This include follows
// both, so replace that entry with a tagged demultiplexer and tail-call their
// shared dispatcher for every non-FolderBack object.
.org TYPE4_FOLDERBACK_SHARED_ENTRY
.dw FolderBackSharedMain + 1

// Keep opposing actors in BN6's active-object list so the engine continues to
// submit their sprites. All type-1 mains pass through a tagged FolderBack
// guard which replaces only a locked actor's action callback with the native
// time-stop-safe visual updater.
.org 0x08003224
.dw FolderBackType1DispatchTable

.autoregion
.align 4
FolderBackCodeStart:

FolderBackSpawn:
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
    mov r1,0
    str r1,[r0,0x60]
    str r1,[r0,0x64]
    str r1,[r0,0x68]
    str r1,[r0,0x6C]
    str r1,[r0,0x70]
    str r1,[r0,0x74]
    ldr r1,=FOLDERBACK_TAG
    str r1,[r0,0x78]
@@done:
    pop {pc}
    .pool

FolderBackSharedMain:
    ldr r0,[r5,0x78]
    ldr r1,=FOLDERBACK_TAG
    cmp r0,r1
    beq FolderBackMain
    ldr r0,=SignalRedBugChargeSharedMain + 1
    bx r0
    .pool

FolderBackMain:
    push {r4,lr}
    ldrb r0,[r5,0x08]
    cmp r0,0
    beq @@init
    cmp r0,4
    beq @@update
    EngineCall OBJECT_FREE
    pop {r4,pc}
@@init:
    mov r0,4
    str r0,[r5,0x08]
    bl FolderBackLockOpponent
@@update:
    EngineCall BATTLE_IS_OVER
    cmp r0,0
    bne @@abort
    bl FolderBackEffectUpdate
    cmp r0,0
    beq @@done

    // Release the opposing units before the chip-use animation enters Custom.
    ldrb r4,[r5,0x16]
    bl FolderBackUnlockOpponent
    bl FolderBackReleaseChip
    EngineCall OBJECT_FREE
    mov r0,r4
    bl FolderBackOpenCustom
    b @@done
@@abort:
    // A KO or other early battle termination must always release the lock.
    bl FolderBackRestorePalette
    bl FolderBackUnlockOpponent
    bl FolderBackReleaseChip
    EngineCall OBJECT_FREE
@@done:
    pop {r4,pc}

// BN3 does not freeze the battle engine for FolderBack. Instead, opposing
// battle units are held in place while the ordinary field and effect
// controllers keep updating. Each side has four native unit slots, covering a
// linked opponent as well as a full virus encounter. Record those actors for
// the type-1 dispatch guard without unlinking them from sprite submission.
FolderBackLockOpponent:
    push {r4,r6,r7,lr}
    mov r6,r10
    ldr r6,[r6,0x18]
    ldrb r0,[r5,0x16]
    mov r1,1
    eor r0,r1
    lsl r0,r0,4
    add r6,r6,r0
    add r6,0xD0
    mov r7,0x60
    mov r4,4
@@loop:
    ldr r0,[r6]
    add r6,4
    str r0,[r5,r7]
    add r7,4
    sub r4,1
    bne @@loop
    pop {r4,r6,r7,pc}

FolderBackUnlockOpponent:
    push {r6,r7,lr}
    mov r7,0x60
    mov r6,4
@@loop:
    mov r0,0
    str r0,[r5,r7]
    add r7,4
    sub r6,1
    bne @@loop
    pop {r6,r7,pc}

// The native type-1 table contains 95 actor families (0x00 through 0x5E).
// Family 0x5E is the native counter-hit response object. The proxy table sends
// every family through the same guard, which looks up and tail-calls the
// original family main whenever the current object is not locked by FolderBack.
FolderBackType1Main:
    push {r4,r6,r7,lr}
    ldr r6,=FOLDERBACK_TYPE4_OBJECT_BASE
    mov r7,FOLDERBACK_TYPE4_OBJECT_COUNT
@@object_loop:
    ldrb r0,[r6]
    mov r1,1
    tst r0,r1
    beq @@next_object
    ldr r0,[r6,0x78]
    ldr r1,=FOLDERBACK_TAG
    cmp r0,r1
    bne @@next_object

    mov r4,0x60
    mov r1,4
@@unit_loop:
    ldr r0,[r6,r4]
    cmp r0,r5
    beq @@locked
    add r4,4
    sub r1,1
    bne @@unit_loop

@@next_object:
    mov r0,FOLDERBACK_TYPE4_OBJECT_SIZE
    add r6,r6,r0
    sub r7,1
    bne @@object_loop

    ldrb r0,[r5,0x01]
    lsl r0,r0,2
    ldr r1,=FOLDERBACK_NATIVE_TYPE1_TABLE
    ldr r4,[r1,r0]
    mov lr,pc
    bx r4
    pop {r4,r6,r7,pc}

@@locked:
    EngineCall OBJECT_UPDATE_TIMESTOP
    pop {r4,r6,r7,pc}
    .pool

.align 4
FolderBackType1DispatchTable:
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1,FolderBackType1Main + 1
    .dw FolderBackType1Main + 1 // native counter-hit response family 0x5E

// Ordinary chip attacks wait on the completion byte supplied in r7. Native
// non-time-freeze type-4 controllers set it to one before freeing themselves.
FolderBackReleaseChip:
    ldr r1,[r5,0x30]
    cmp r1,0
    beq @@done
    mov r0,1
    strb r0,[r1]
    mov r0,0
    str r0,[r5,0x30]
@@done:
    bx lr

// BN3's original effect has two exact beats: a 70-frame rumble/white-flash,
// followed by 20 frames with the Custom Gauge visibly full. Only after those
// 20 frames does FolderBack hand control back to Custom.
FolderBackEffectUpdate:
    push {lr}
    ldrb r0,[r5,0x0A]
    cmp r0,0
    beq @@impact_init
    cmp r0,4
    beq @@flash
    cmp r0,8
    beq @@restore_init

    // Final full-gauge beat.
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    bne @@done
    mov r0,1
    pop {pc}

@@impact_init:
    mov r0,0x46
    strh r0,[r5,0x20]
    mov r0,4
    strb r0,[r5,0x0A]
    bl FolderBackImpact
    bl FolderBackApplyWhiteFlash
    b @@done

@@flash:
    ldrh r0,[r5,0x20]
    sub r0,1
    strh r0,[r5,0x20]
    beq @@flash_done
    bl FolderBackApplyWhiteFlash
    b @@done

@@flash_done:
    bl FolderBackRestorePalette
    mov r0,8
    strb r0,[r5,0x0A]
    b @@done

@@restore_init:
    mov r0,0x14
    strh r0,[r5,0x20]
    mov r0,0x0C
    strb r0,[r5,0x0A]
    bl FolderBackFillLocalCustomGauge
    bl FolderBackRestoreLocalFolder
@@done:
    mov r0,0
    pop {pc}

// BN3 alternates pale and near-white scene palettes every four frames for
// the complete 0x46-frame shake. BN6 splits those palettes across two caches,
// so schedule the same grayscale colors against both native cache slots.
FolderBackApplyWhiteFlash:
    push {r4,lr}
    ldrh r0,[r5,0x20]
    mov r1,4
    tst r0,r1
    beq @@pale
    ldr r1,=0x00004210
    b @@color_ready
@@pale:
    ldr r1,=0x00006318
@@color_ready:
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

FolderBackRestorePalette:
    push {lr}
    mov r0,0x14
    EngineCall CHAOS_PALETTE_RESTORE
    mov r0,0x15
    EngineCall CHAOS_PALETTE_RESTORE
    pop {pc}

// BN3 writes 0x4000, the native maximum, to the local player's gauge before
// its 20-frame ending beat. Each linked console owns a different local side,
// so read that side from the battle controller instead of the chip object.
// Mirror the value through BN6's native HUD setter so the normal, still-active
// battle HUD displays a completely full bar for the ending beat.
FolderBackFillLocalCustomGauge:
    push {lr}
    mov r0,r10
    ldr r0,[r0,0x18]
    ldrb r0,[r0,0x0D]
    EngineCall FOLDERBACK_PLAYER_DATA
    ldr r1,=0x00004000
    strh r1,[r0,0x28]
    mov r0,r1
    EngineCall FOLDERBACK_SET_HUD_GAUGE
    // Native BN6 FullCust completion chime.
    mov r0,0x8F
    EngineCall PLAY_SOUND
    pop {pc}
    .pool

FolderBackImpact:
    push {lr}
    mov r0,3
    mov r1,0x46
    EngineCall SCREEN_SHAKE_SET
    mov r0,0xDF
    add r0,0xFF
    EngineCall PLAY_SOUND
    pop {pc}

// Each linked console owns one real equipped Folder. Only rebuild it when the
// FolderBack user's logical side matches this console's local side; both ROMs
// still run the visual and synchronized Custom transition.
FolderBackRestoreLocalFolder:
    push {r4,r6,lr}
    ldrb r0,[r5,0x16]
    EngineCall FOLDERBACK_COMPARE_LOCAL_SIDE
    cmp r0,0
    bne @@done

    // +0x17 is the live "Regular Chip still available" flag. Preserve it
    // across the rebuild: if the RegChip was already consumed, reshuffle the
    // restored 30-chip queue with no reserved Regular slot. The chip remains
    // in the queue; only its special designation is gone. The saved folder's
    // configured Regular-Chip index is never modified.
    mov r6,r10
    ldr r6,[r6,0x18]
    ldrb r4,[r6,0x17]
    EngineCall FOLDERBACK_REBUILD_FOLDER
    cmp r4,0
    bne @@clear_hand
    mov r0,0
    strb r0,[r6,0x17]
    ldr r0,=FOLDERBACK_QUEUE
    mov r1,0
    mov r3,0x44
    ldrb r2,[r6,r3]
    EngineCall FOLDERBACK_SHUFFLE_QUEUE
@@clear_hand:
    EngineCall FOLDERBACK_CLEAR_HAND
@@done:
    pop {r4,r6,pc}
    .pool

// Restart BN6's native Custom Screen controller after FolderBack releases the
// ordinary chip-use animation.
FolderBackOpenCustom:
    push {r4,lr}
    mov r4,r0
    ldr r1,=FOLDERBACK_BATTLE_STATE
    ldr r0,[r1]
    cmp r0,8
    beq @@linked
    cmp r0,4
    bne @@done

    // Story battles use the ordinary local Custom request branch.
    strb r4,[r1,5]
    EngineCall FOLDERBACK_LOCK_BATTLE_STATE
    ldr r1,=FOLDERBACK_BATTLE_STATE
    mov r0,0x18
    str r0,[r1]
    EngineCall FOLDERBACK_BEGIN_LOCAL_CUSTOM
    b @@done

@@linked:
    // Linked battles have a separate coordinator. Its state 0x20 is the
    // durable, post-gauge-check entry used by a real full-gauge request; it
    // advances both consoles through 0x24 and into the Custom Screen.
    EngineCall FOLDERBACK_LOCK_BATTLE_STATE
    ldr r1,=FOLDERBACK_BATTLE_STATE
    mov r0,0x20
    str r0,[r1]
@@done:
    pop {r4,pc}
    .pool

FolderBackCodeEnd:
.endautoregion

.autoregion
.align 4
FolderBackIcon:
    .incbin "build/folderback-icon.bin"
.endautoregion

.autoregion
.align 4
FolderBackImage:
    .incbin "build/folderback-image.bin"
.endautoregion

.autoregion
.align 4
FolderBackPalette:
    .incbin "build/folderback-palette.bin"
.endautoregion

// Exact BN3 Blue SFX 0x120: the FolderBack rumble, including its original
// voice priority, note stream, sample header, and complete PCM body.
.autoregion
.align 4
FolderBackRumbleAudioStart:
FolderBackRumbleSong:
    .db 0x01,0x00,0x40,0x00
    .dw FolderBackRumbleVoicegroup
    .dw FolderBackRumbleTrack
FolderBackRumbleVoicegroup:
    .db 0x08,0x3C,0x00,0x00
    .dw FolderBackRumbleSample
    .db 0xFF,0x00,0xFF,0x00
FolderBackRumbleTrack:
    .db 0xBC,0x00,0xBB,0x4B,0xBD,0x00,0xBE,0x7F
    .db 0xBF,0x40,0xF9,0x3C,0x7F,0xAA,0x81,0xB1
.align 4
FolderBackRumbleSample:
    .incbin "build/folderback-rumble-sample.bin"
FolderBackRumbleAudioEnd:
.endautoregion
