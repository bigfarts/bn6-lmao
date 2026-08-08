// ---------------------------------------------------------------------------
// Relocate every affected BN6 battle-sprite group without replacing native
// entries. Each complete native table is copied first; imported archives are
// appended in stable slots shared by Gregar and Falzar.

WriteArchivePointer 0x08031CCC,ImportedSpriteGroup08Table
WriteArchivePointer 0x08031CD0,ImportedSpriteGroup0CTable
WriteArchivePointer 0x08031CD4,ImportedSpriteGroup10Table
WriteArchivePointer 0x08031CD8,ImportedSpriteGroup14Table

// Group 0x08 has 23 native entries (0x00-0x16).
.autoregion
.align 4
ImportedSpriteGroup08Table:
    .incbin "build/sprite-group08-table.bin"

    // The main Bass and Ball Bass objects animate simultaneously. Give them
    // distinct group/index cache keys even though they share one archive.
    ChaosBassMainSpriteEntry:
    .dw ChaosBassSprite
    .definelabel CHAOS_BASS_MAIN_SPRITE_INDEX, (ChaosBassMainSpriteEntry - ImportedSpriteGroup08Table) / 4

    ChaosBassBallSpriteEntry:
    .dw ChaosBassSprite
    .definelabel CHAOS_BASS_BALL_SPRITE_INDEX, (ChaosBassBallSpriteEntry - ImportedSpriteGroup08Table) / 4

    SearchManBattleSpriteEntry:
    .dw SearchManBattleSprite
    .definelabel SEARCHMAN_BATTLE_SPRITE_INDEX, (SearchManBattleSpriteEntry - ImportedSpriteGroup08Table) / 4

    ChaosAuraSpriteEntry:
    .dw ChaosAuraSprite | 0x80000000            // compressed
    .definelabel CHAOS_AURA_SPRITE_INDEX, (ChaosAuraSpriteEntry - ImportedSpriteGroup08Table) / 4
ImportedSpriteGroup08TableEnd:
.endautoregion

// Group 0x0C has 105 native entries (0x00-0x68).
.autoregion
.align 4
ImportedSpriteGroup0CTable:
.incbin "build/sprite-group0C-table.bin"

    BugChargeGospelSpriteEntry:
    .dw BugChargeGospelSprite
    .definelabel BUGCHARGE_GOSPEL_SPRITE_INDEX, (BugChargeGospelSpriteEntry - ImportedSpriteGroup0CTable) / 4

    LaserManBattleSpriteEntry:
    .dw LaserManBattleSprite | 0x80000000       // compressed
    .definelabel LASERMAN_BATTLE_SPRITE_INDEX, (LaserManBattleSpriteEntry - ImportedSpriteGroup0CTable) / 4

    DeathPhoenixBattleSpriteEntry:
    .dw DeathPhoenixBattleSprite
    .definelabel DEATHPHOENIX_BATTLE_SPRITE_INDEX, (DeathPhoenixBattleSpriteEntry - ImportedSpriteGroup0CTable) / 4

    RollArrowActorSpriteEntry:
    .dw RollArrowActorSprite
    .definelabel ROLLARROW_ACTOR_SPRITE_INDEX, (RollArrowActorSpriteEntry - ImportedSpriteGroup0CTable) / 4

    RollArrowProjectileSpriteEntry:
    .dw RollArrowProjectileSprite
    .definelabel ROLLARROW_PROJECTILE_SPRITE_INDEX, (RollArrowProjectileSpriteEntry - ImportedSpriteGroup0CTable) / 4

ImportedSpriteGroup0CTableEnd:
.endautoregion

// Group 0x10 has 92 native entries (0x00-0x5B).
.autoregion
.align 4
ImportedSpriteGroup10Table:
.incbin "build/sprite-group10-table.bin"

    BugChainBattleSpriteEntry:
    .dw BugChainBattleSprite
    .definelabel BUGCHAIN_BATTLE_SPRITE_INDEX, (BugChainBattleSpriteEntry - ImportedSpriteGroup10Table) / 4

    ChaosTeardownSpriteEntry:
    .dw ChaosTeardownSprite
    .definelabel CHAOS_TEARDOWN_SPRITE_INDEX, (ChaosTeardownSpriteEntry - ImportedSpriteGroup10Table) / 4

    SearchManReticleAltSpriteEntry:
    .dw SearchManReticleAltSprite
    .definelabel SEARCHMAN_RETICLE_ALT_SPRITE_INDEX, (SearchManReticleAltSpriteEntry - ImportedSpriteGroup10Table) / 4

    SearchManReticleSpriteEntry:
    .dw SearchManReticleSprite
    .definelabel SEARCHMAN_RETICLE_SPRITE_INDEX, (SearchManReticleSpriteEntry - ImportedSpriteGroup10Table) / 4

    ChaosApparitionSpriteEntry:
    .dw ChaosApparitionSprite
    .definelabel CHAOS_APPARITION_SPRITE_INDEX, (ChaosApparitionSpriteEntry - ImportedSpriteGroup10Table) / 4

    SignalRedBattleSpriteEntry:
    .dw SignalRedBattleSprite
    .definelabel SIGNALRED_BATTLE_SPRITE_INDEX, (SignalRedBattleSpriteEntry - ImportedSpriteGroup10Table) / 4

ImportedSpriteGroup10TableEnd:
.endautoregion

// Group 0x14 has 32 native entries (0x00-0x1F).
.autoregion
.align 4
ImportedSpriteGroup14Table:
.incbin "build/sprite-group14-table.bin"

    DeathPhoenixStrikeSpriteEntry:
    .dw DeathPhoenixStrikeSprite
    .definelabel DEATHPHOENIX_STRIKE_SPRITE_INDEX, (DeathPhoenixStrikeSpriteEntry - ImportedSpriteGroup14Table) / 4

ImportedSpriteGroup14TableEnd:
.endautoregion
