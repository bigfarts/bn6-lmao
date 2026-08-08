// ---------------------------------------------------------------------------
// Project-owned object dispatch hubs.  BN6's allocator stores an object's
// native class separately from its one-byte family ID.  All imported objects
// of a class use one unused native entry and select their logical main through
// the stable kind word at object +0x7C.

.definelabel TYPE1_CUSTOM_ENTRY, 0x08003D60 // family 0x31
.definelabel TYPE3_CUSTOM_ENTRY, 0x08003F74 // family 0x2C
.definelabel TYPE4_CUSTOM_ENTRY, 0x08004324 // family 0x17

.org TYPE1_CUSTOM_ENTRY
.dw CustomType1Main + 1
.org TYPE3_CUSTOM_ENTRY
.dw CustomType3Main + 1
.org TYPE4_CUSTOM_ENTRY
.dw CustomType4Main + 1

.autoregion
.align 4
ObjectTypesCodeStart:

CustomType1Main:
    ldr r0,[r5,CUSTOM_OBJECT_KIND]
    sub r0,1
    cmp r0,TYPE1_KIND_COUNT
    bhs CustomObjectInvalid
    lsl r0,r0,2
    ldr r1,=CustomType1Table
    ldr r0,[r1,r0]
    bx r0

CustomType3Main:
    ldr r0,[r5,CUSTOM_OBJECT_KIND]
    sub r0,1
    cmp r0,TYPE3_KIND_COUNT
    bhs CustomObjectInvalid
    lsl r0,r0,2
    ldr r1,=CustomType3Table
    ldr r0,[r1,r0]
    bx r0

CustomType4Main:
    ldr r0,[r5,CUSTOM_OBJECT_KIND]
    sub r0,1
    cmp r0,TYPE4_KIND_COUNT
    bhs CustomObjectInvalid
    lsl r0,r0,2
    ldr r1,=CustomType4Table
    ldr r0,[r1,r0]
    bx r0

// A missing or stale kind must not reinterpret an object as an unrelated
// controller.  Free it immediately through the native pool lifecycle.
CustomObjectInvalid:
    push {lr}
    EngineCall OBJECT_FREE
    pop {pc}

.align 4
CustomType1Table:

    ChaosControllerType1Entry:
    .dw ChaosControllerMain + 1
    .definelabel TYPE1_KIND_CHAOS_CONTROLLER, (ChaosControllerType1Entry - CustomType1Table) / 4 + 1

    ChaosBallType1Entry:
    .dw ChaosBallMain + 1
    .definelabel TYPE1_KIND_CHAOS_BALL, (ChaosBallType1Entry - CustomType1Table) / 4 + 1

    RollArrowType1Entry:
    .dw RollArrowActorMain + 1
    .definelabel TYPE1_KIND_ROLL_ACTOR, (RollArrowType1Entry - CustomType1Table) / 4 + 1

    SearchManType1Entry:
    .dw SearchManObjectMain + 1
    .definelabel TYPE1_KIND_SEARCH_ACTOR, (SearchManType1Entry - CustomType1Table) / 4 + 1

    LaserManActorType1Entry:
    .dw LaserManActorMain + 1
    .definelabel TYPE1_KIND_LASER_ACTOR, (LaserManActorType1Entry - CustomType1Table) / 4 + 1

    LaserManBeamType1Entry:
    .dw LaserManLaserMain + 1
    .definelabel TYPE1_KIND_LASER_BEAM, (LaserManBeamType1Entry - CustomType1Table) / 4 + 1

    DeathPhoenixType1Entry:
    .dw DeathPhoenixMain + 1
    .definelabel TYPE1_KIND_DEATHPHOENIX, (DeathPhoenixType1Entry - CustomType1Table) / 4 + 1

CustomType1TableEnd:
.definelabel TYPE1_KIND_COUNT, (CustomType1TableEnd - CustomType1Table) / 4

CustomType3Table:

    ChaosAttackType3Entry:
    .dw ChaosAttackMain + 1
    .definelabel TYPE3_KIND_CHAOS_ATTACK, (ChaosAttackType3Entry - CustomType3Table) / 4 + 1

    SignalRedType3Entry:
    .dw SignalRedObjectMain + 1
    .definelabel TYPE3_KIND_SIGNALRED, (SignalRedType3Entry - CustomType3Table) / 4 + 1

    RollArrowType3Entry:
    .dw RollArrowProjectileMain + 1
    .definelabel TYPE3_KIND_ROLL_ARROW, (RollArrowType3Entry - CustomType3Table) / 4 + 1

    SearchManType3Entry:
    .dw SearchManHitMain + 1
    .definelabel TYPE3_KIND_SEARCH_HIT, (SearchManType3Entry - CustomType3Table) / 4 + 1

    LaserManType3Entry:
    .dw LaserManHitMain + 1
    .definelabel TYPE3_KIND_LASER_HIT, (LaserManType3Entry - CustomType3Table) / 4 + 1

    BugChargeGospelType3Entry:
    .dw BugChargeGospelMain + 1
    .definelabel TYPE3_KIND_BUGCHARGE_GOSPEL, (BugChargeGospelType3Entry - CustomType3Table) / 4 + 1

    BugChargeHitType3Entry:
    .dw BugChargeProjectileMain + 1
    .definelabel TYPE3_KIND_BUGCHARGE_HIT, (BugChargeHitType3Entry - CustomType3Table) / 4 + 1

CustomType3TableEnd:
.definelabel TYPE3_KIND_COUNT, (CustomType3TableEnd - CustomType3Table) / 4

CustomType4Table:

    ChaosAuraType4Entry:
    .dw ChaosAuraMain + 1
    .definelabel TYPE4_KIND_CHAOS_AURA, (ChaosAuraType4Entry - CustomType4Table) / 4 + 1

    ChaosBurstType4Entry:
    .dw ChaosBurstMain + 1
    .definelabel TYPE4_KIND_CHAOS_BURST, (ChaosBurstType4Entry - CustomType4Table) / 4 + 1

    ChaosTeardownType4Entry:
    .dw ChaosTeardownMain + 1
    .definelabel TYPE4_KIND_CHAOS_TEARDOWN, (ChaosTeardownType4Entry - CustomType4Table) / 4 + 1

    ChaosFlashType4Entry:
    .dw ChaosFlashMain + 1
    .definelabel TYPE4_KIND_CHAOS_FLASH, (ChaosFlashType4Entry - CustomType4Table) / 4 + 1

    SearchManReticleType4Entry:
    .dw SearchManReticleMain + 1
    .definelabel TYPE4_KIND_SEARCH_RETICLE, (SearchManReticleType4Entry - CustomType4Table) / 4 + 1

    JealousyType4Entry:
    .dw JealousyMain + 1
    .definelabel TYPE4_KIND_JEALOUSY, (JealousyType4Entry - CustomType4Table) / 4 + 1

    BugChainType4Entry:
    .dw BugChainMain + 1
    .definelabel TYPE4_KIND_BUGCHAIN, (BugChainType4Entry - CustomType4Table) / 4 + 1

    BugChainVisualType4Entry:
    .dw BugChainVisualMain + 1
    .definelabel TYPE4_KIND_BUGCHAIN_VISUAL, (BugChainVisualType4Entry - CustomType4Table) / 4 + 1

    BugChargeType4Entry:
    .dw BugChargeControllerMain + 1
    .definelabel TYPE4_KIND_BUGCHARGE, (BugChargeType4Entry - CustomType4Table) / 4 + 1

    BugChargeHeadType4Entry:
    .dw BugChargeChargeHeadMain + 1
    .definelabel TYPE4_KIND_BUGCHARGE_HEAD, (BugChargeHeadType4Entry - CustomType4Table) / 4 + 1

    SignalRedType4Entry:
    .dw SignalRedTimeFreezeMain + 1
    .definelabel TYPE4_KIND_SIGNALRED, (SignalRedType4Entry - CustomType4Table) / 4 + 1

    FolderBackType4Entry:
    .dw FolderBackMain + 1
    .definelabel TYPE4_KIND_FOLDERBACK, (FolderBackType4Entry - CustomType4Table) / 4 + 1

    DeathStrikeType4Entry:
    .dw DeathStrikeMain + 1
    .definelabel TYPE4_KIND_DEATH_STRIKE, (DeathStrikeType4Entry - CustomType4Table) / 4 + 1

    DeathFlameType4Entry:
    .dw DeathFlameMain + 1
    .definelabel TYPE4_KIND_DEATH_FLAME, (DeathFlameType4Entry - CustomType4Table) / 4 + 1

CustomType4TableEnd:
.definelabel TYPE4_KIND_COUNT, (CustomType4TableEnd - CustomType4Table) / 4
    .pool

ObjectTypesCodeEnd:
.endautoregion
