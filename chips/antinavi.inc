// AntiNavi retains its complete native BN6 record and behavior; only its
// folder-capacity cost changes from 50 MB to 33 MB in both versions.

.definelabel ANTINAVI_ID,                    0x0BA

.org CHIP_DATA + ANTINAVI_ID * 0x2C + 0x08
    .db 33
