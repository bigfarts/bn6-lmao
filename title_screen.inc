.if FALZAR
    .org 0x087F4394
    .incbin "build/title-67-falzar.bin"
.else
    .org 0x087F3040
    .incbin "build/title-67-gregar.bin"
.endif

// Both editions use one wholesale replacement map with the same 32x20
// layout. Only the source artwork selected below differs.
WriteArchivePointer 0x0802FD50,TitleScreenMap

.autoregion
.align 4
TitleScreenMap:
.if FALZAR
    .incbin "build/title-map-falzar.bin"
.else
    .incbin "build/title-map-gregar.bin"
.endif
TitleScreenMapEnd:
.endautoregion
