// Relocate the complete native BN6 song table through its last entry 0x1D9,
// then populate 0x1DA with SignalRed, append RollArrow's two exact BN4 cues
// at 0x1DB/0x1DC, LaserMan's exact BN4 firing cue at 0x1DD, and BN3's
// FolderBack rumble at 0x1DE, BugChain's aura cue at 0x1DF, and BugCharge's
// BN5 charge/fire cues at 0x1E0/0x1E1. Every
// imported cue uses the relocated table.
.if FALZAR
    .definelabel SONG_TABLE_REFERENCE_0, 0x0814E94C
    .definelabel SONG_TABLE_REFERENCE_1, 0x0814E980
    .definelabel SONG_TABLE_REFERENCE_2, 0x0814E9CC
    .definelabel SONG_TABLE_REFERENCE_3, 0x0814EA20
    .definelabel SONG_TABLE_REFERENCE_4, 0x0814EA54
.else
    .definelabel SONG_TABLE_REFERENCE_0, 0x0815049C
    .definelabel SONG_TABLE_REFERENCE_1, 0x081504D0
    .definelabel SONG_TABLE_REFERENCE_2, 0x0815051C
    .definelabel SONG_TABLE_REFERENCE_3, 0x08150570
    .definelabel SONG_TABLE_REFERENCE_4, 0x081505A4
.endif

.org SONG_TABLE_REFERENCE_0
.dw RelocatedSongTable
.org SONG_TABLE_REFERENCE_1
.dw RelocatedSongTable
.org SONG_TABLE_REFERENCE_2
.dw RelocatedSongTable
.org SONG_TABLE_REFERENCE_3
.dw RelocatedSongTable
.org SONG_TABLE_REFERENCE_4
.dw RelocatedSongTable

.autoregion
.align 4
RelocatedSongTable:
    .incbin "build/song-table.bin"
    .dw SignalRedSpawnSong
    .dh 0x000C,0x000C
    .dw RollArrowSummonSong
    .dh 0x000C,0x000C
    .dw RollArrowFireSong
    .dh 0x000C,0x000C
    .dw LaserManFireSong
    .dh 0x000C,0x000C
    .dw FolderBackRumbleSong
    .dh 0x001B,0x001B
    .dw BugChainSoundSong
    .dh 0x001C,0x001C
    .dw BugChargeChargeSong
    .dh 0x0010,0x0010
    .dw BugChargeFireSong
    .dh 0x0014,0x0014
RelocatedSongTableEnd:
.endautoregion
