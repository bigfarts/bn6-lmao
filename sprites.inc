// ---------------------------------------------------------------------------
// Install every imported battle archive together. BN6's sprite root points to
// separate tables for groups 0x08, 0x0C, 0x10, and 0x14. Relocating all four
// complete tables here prevents a later table copy from discarding an earlier
// per-slot patch.

.definelabel SPRITE_GROUP08_TABLE_POINTER, 0x08031CCC
.definelabel SPRITE_GROUP0C_TABLE_POINTER, 0x08031CD0
.definelabel SPRITE_GROUP10_TABLE_POINTER, 0x08031CD4
.definelabel SPRITE_GROUP14_TABLE_POINTER, 0x08031CD8

.org SPRITE_GROUP08_TABLE_POINTER
.dw ImportedSpriteGroup08Table
.org SPRITE_GROUP0C_TABLE_POINTER
.dw ImportedSpriteGroup0CTable
.org SPRITE_GROUP10_TABLE_POINTER
.dw ImportedSpriteGroup10Table
.org SPRITE_GROUP14_TABLE_POINTER
.dw ImportedSpriteGroup14Table

// Group 0x08 has 23 native entries. Keep the live BN6 entries and replace the
// four released/imported slots as one table image.
.autoregion
.align 4
ImportedSpriteGroup08Table:
.dw ChaosBassSprite                         // index 0x00
.incbin "build/sprite-group08-table.bin",0x04,0x40
.dw SearchManBattleSprite                   // index 0x11
.dw ChaosAuraSprite | 0x80000000            // index 0x12, compressed
.dw ChaosBassSprite                         // index 0x13
.incbin "build/sprite-group08-table.bin",0x50,0x0C
ImportedSpriteGroup08TableEnd:
.endautoregion

// Group 0x0C has 105 native entries. BugCharge's BN5 charge-orbit archive is
// already uncompressed; only LaserMan retains the high compression bit.
.autoregion
.align 4
ImportedSpriteGroup0CTable:
.incbin "build/sprite-group0C-table.bin",0,0x80
.dw BugChargeChargeSprite                   // index 0x20
.incbin "build/sprite-group0C-table.bin",0x84,0xD4
.dw LaserManBattleSprite | 0x80000000       // index 0x56, compressed
.dw DeathPhoenixBattleSprite                // index 0x57
.incbin "build/sprite-group0C-table.bin",0x160,0x08
.dw RollArrowActorSprite                    // index 0x5A
.dw RollArrowProjectileSprite               // index 0x5B
.incbin "build/sprite-group0C-table.bin",0x170,0x34
ImportedSpriteGroup0CTableEnd:
.endautoregion

// Group 0x10 has 92 native entries. Append BugChain and the BN5 BugCharge
// Gospel-head archive at the new indices 0x5C and 0x5D.
.autoregion
.align 4
ImportedSpriteGroup10Table:
.incbin "build/sprite-group10-table.bin",0,0x10
.dw ChaosTeardownSprite                     // index 0x04
.incbin "build/sprite-group10-table.bin",0x14,0x3C
.dw SearchManReticleAltSprite               // index 0x14
.dw SearchManReticleSprite                  // index 0x15
.incbin "build/sprite-group10-table.bin",0x58,0x08
.dw ChaosApparitionSprite                   // index 0x18
.incbin "build/sprite-group10-table.bin",0x64,0x14
.dw SignalRedBattleSprite                   // index 0x1E
.incbin "build/sprite-group10-table.bin",0x7C,0xF4
.dw BugChainBattleSprite                    // index 0x5C
.dw BugChargeGospelSprite                   // index 0x5D
ImportedSpriteGroup10TableEnd:
.endautoregion

// Group 0x14 has 32 native entries.
.autoregion
.align 4
ImportedSpriteGroup14Table:
.incbin "build/sprite-group14-table.bin",0,0x50
.dw ChaosImpactSprite                       // index 0x14
.incbin "build/sprite-group14-table.bin",0x54,0x08
.dw DeathPhoenixStrikeSprite                // index 0x17
.incbin "build/sprite-group14-table.bin",0x60,0x20
ImportedSpriteGroup14TableEnd:
.endautoregion
