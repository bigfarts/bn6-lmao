.gba
.thumb

// BN6 records/tables and shared engine entry points (US Gregar and Falzar).
.definelabel CHIP_DATA,                0x08021DA8
.definelabel PLAY_SOUND,               0x080005CC
.definelabel RNG_NEXT,                 0x0800151C
.definelabel BATTLE_RNG,               0x08001532
.definelabel MEMORY_CLEAR,             0x080008B4
.definelabel MEMORY_COPY,              0x08000920
.definelabel SHUFFLE_BYTES,            0x08000C72
.definelabel SPRITE_LOAD_ANIM_DATA,    0x080026A4
.definelabel SPRITE_PROPERTY_26C4,      0x080026C4
.definelabel SPRITE_LOAD,              0x080026E4
.definelabel SPRITE_SET_BLEND_MODE,    0x08002C7A
.definelabel SPRITE_PROPERTY_2CCE,     0x08002CCE
.definelabel SPRITE_PROPERTY_2C7E,     0x08002C7E
.definelabel SPRITE_PROPERTY_2CE0,     0x08002CE0
.definelabel SPRITE_SET_PALETTE,       0x08002D80
.definelabel SPRITE_GET_PALETTE,       0x08002D8C
.definelabel SPRITE_SET_ANIMATION,     0x08002DA4
.definelabel SPRITE_GET_ANIMATION_FLAGS,0x08002DEA
.definelabel SPRITE_PROPERTY_2DB4,     0x08002DB4
.definelabel SPRITE_PROPERTY_2DC8,     0x08002DC8
.definelabel SPRITE_PROPERTY_2E3C,     0x08002E3C
.definelabel SPRITE_SET_PRIORITY,      0x08002E14
.definelabel SPRITE_SET_SCALE,         0x08002ED0
.definelabel SPRITE_GET_SCALE,         0x08002EDC
.definelabel SPRITE_SET_BLEND,         0x08002EF6
.definelabel SPRITE_PROPERTY_2F02,     0x08002F02
.definelabel SPRITE_PROPERTY_2F3E,     0x08002F3E
.definelabel SPRITE_NO_SHADOW,         0x08002F90
.definelabel SPRITE_PROPERTY_2FA6,     0x08002FA6
.definelabel SPRITE_SET_FLIP,          0x08002F5C
.definelabel SPRITE_SET_FLAG,          0x08002FBE
.definelabel SPRITE_ALLOC,             0x08002B30
.definelabel SPAWN_TYPE1,              0x08003320
.definelabel SPAWN_TYPE3,              0x08003358
.definelabel SPAWN_TYPE4,              0x080033AC
.definelabel OBJECT_FREE,              0x08003458
.definelabel SPRITE_ANIMATION_STATUS,  0x0800139A
.definelabel BATTLE_IS_OVER,           0x0800A18E
.definelabel BATTLE_CONFIG_FLAGS,      0x0802D246
.definelabel BATTLE_FLAGS_SET,          0x08001382
.definelabel BATTLE_FLAGS_CLEAR,        0x0800138E
.definelabel OBJECT_REGISTER_DEPLOYABLE,0x0800F614
.definelabel OBJECT_UNREGISTER_DEPLOYABLE,0x0800F656
.definelabel OBJECT_UPDATE_SUCTION,      0x0800F8CE
.definelabel OBJECT_STORE_DUST_AMMO,     0x0800F90E
.definelabel PANEL_IS_VALID_OBJECT,    0x0800CC66
.definelabel PANEL_IS_VALID_XY,        0x0800CC72
.definelabel PANEL_HAS_FLAGS,          0x0800CC86
.definelabel PANEL_GET_PARAMETERS,     0x0800D5F0
.definelabel PANEL_GET_FLAGS,          0x0800C8F8
.definelabel PANEL_SET_FLASH,          0x0800CBD8
.definelabel PANEL_CRACK_FROM_SOLID,   0x0800C938
.definelabel PANEL_CRACK,              0x0800CA8C
.definelabel PANEL_TO_COORDS,          0x0800E276
.definelabel OBJECT_SET_COORDS,        0x0800E29C
.definelabel OBJECT_UPDATE_PANEL,      0x0800E2AC
.definelabel OBJECT_FRONT_DIRECTION,  0x0800E2CA
.definelabel OBJECT_SIDE_DIRECTION,   0x0800E2C0
.definelabel OBJECT_GET_FLIP,          0x0800E456
.definelabel INPUT_STATE_FOR_SIDE,     0x0800A0F4
.definelabel PLAYER_OBJECT_FOR_SIDE,   0x080103BC
.definelabel PLAYER_PROPERTIES_FOR_SIDE,0x08013682
.definelabel PLAYER_PROPERTY_SET_FOR_SIDE,0x080136B0
.definelabel PLAYER_PROPERTY_FOR_SIDE, 0x080136CC
.definelabel PLAYER_CLEAR_STATUS_FLAGS,0x0801A15C
.definelabel ANGLE_FROM_VECTOR,        0x0800117C
.definelabel COLLISION_CREATE,         0x08019892
.definelabel COLLISION_FREE,           0x080198CE
.definelabel COLLISION_SETUP,          0x08019FB4
.definelabel COLLISION_REMOVE,         0x0801A00E
.definelabel COLLISION_PRESENT,        0x0801A018
.definelabel COLLISION_UPDATE_PANEL,   0x0801A04C
.definelabel COLLISION_CLEAR_REGION,   0x0801A074
.definelabel COLLISION_SPAWN_EFFECT,   0x0801A0D4
.definelabel COLLISION_SET_HIT_EFFECT, 0x0801A140
.definelabel COLLISION_SET_EXTENDED_EFFECT,0x0801A4D0
.definelabel COLLISION_GET_FLAGS,      0x0801A180
.definelabel OBJECT_UPDATE,            0x0801BBAC
.definelabel OBJECT_UPDATE_TIMESTOP,   0x0801BBF4
.definelabel OBJECT_LOAD_NAVI_SPRITE,  0x0801BE2A
.definelabel SCREEN_SHAKE_SET,         0x080302A8

// Shared native lifecycle used by invisible type-4 time-freeze chip
// controllers. Jealousy and SignalRed both replace only the chip-specific
// effect state while retaining BN6's intro, freeze, outro, and free states.
.definelabel TYPE4_TIMESTOP_INIT,       0x0800B916
.definelabel TYPE4_TIMESTOP_INTRO,      0x0800B94C
.definelabel TYPE4_TIMESTOP_FREEZE,     0x0800B9B0
.definelabel TYPE4_TIMESTOP_OUTRO,      0x0800BC88
.definelabel TYPE4_TIMESTOP_FREE,       0x0800BD34

// Battle-effect helpers used by BN5's dormant ChaosLord Ball Bass object.
// These are the matching BN6 implementations, identified by their complete
// state setup and object-layout behavior rather than by address deltas.
.definelabel CHAOS_IMPACT_EFFECT,      0x080E4328
.definelabel CHAOS_DAMAGE_OBJECT,      0x080C6BDA
.definelabel CHAOS_PALETTE_WRITE,      0x08002378
.definelabel CHAOS_PALETTE_RESTORE,    0x0800239A

// Expanded-ROM code is outside Thumb BL range. Preserve r4 because it is an
// implicit fifth argument to the object spawners, then branch through r12.
.macro EngineCall,target
    push {r4}
    ldr r4,=target + 1
    mov r12,r4
    pop {r4}
    mov lr,pc
    bx r12
.endmacro
