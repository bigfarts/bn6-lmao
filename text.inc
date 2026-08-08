// Complete relocated chip-text archives for IDs 0x000-0x0FF and 0x100+.
// Changes are keyed by full chip ID in the Python builder, so replacements in
// either half are generated together and cannot clobber one another.

.if FALZAR
    .org 0x08120116
    b 0x0812012E

    WriteArchivePointer 0x08027D2C,ChipNameArchive0
    WriteArchivePointer 0x0802C7C4,ChipNameArchive0
    WriteArchivePointer 0x08042068,ChipNameArchive0
    WriteArchivePointer 0x08042080,ChipNameArchive0
    WriteArchivePointer 0x080476E4,ChipNameArchive0
    WriteArchivePointer 0x08120178,ChipNameArchive0
    WriteArchivePointer 0x08120218,ChipNameArchive0
    WriteArchivePointer 0x08120270,ChipNameArchive0
    WriteArchivePointer 0x081263B8,ChipNameArchive0
    WriteArchivePointer 0x08027D30,ChipNameArchive1
    WriteArchivePointer 0x0802C7C8,ChipNameArchive1
    WriteArchivePointer 0x0804206C,ChipNameArchive1
    WriteArchivePointer 0x080476E8,ChipNameArchive1
    WriteArchivePointer 0x0812017C,ChipNameArchive1
    WriteArchivePointer 0x0812021C,ChipNameArchive1
    WriteArchivePointer 0x08120274,ChipNameArchive1
    WriteArchivePointer 0x081263BC,ChipNameArchive1
    WriteArchivePointer 0x08027D50,ChipDescriptionArchive0
    WriteArchivePointer 0x0811968C,ChipDescriptionArchive0
    WriteArchivePointer 0x0811A08C,ChipDescriptionArchive0
    WriteArchivePointer 0x08027D54,ChipDescriptionArchive1
    WriteArchivePointer 0x08119690,ChipDescriptionArchive1
    WriteArchivePointer 0x0811A090,ChipDescriptionArchive1
.else
    .org 0x08121EF2
    b 0x08121F0A

    WriteArchivePointer 0x08027D2C,ChipNameArchive0
    WriteArchivePointer 0x0802C7C4,ChipNameArchive0
    WriteArchivePointer 0x08042038,ChipNameArchive0
    WriteArchivePointer 0x08042050,ChipNameArchive0
    WriteArchivePointer 0x080476B4,ChipNameArchive0
    WriteArchivePointer 0x08121F54,ChipNameArchive0
    WriteArchivePointer 0x08121FF4,ChipNameArchive0
    WriteArchivePointer 0x0812204C,ChipNameArchive0
    WriteArchivePointer 0x08128194,ChipNameArchive0
    WriteArchivePointer 0x08027D30,ChipNameArchive1
    WriteArchivePointer 0x0802C7C8,ChipNameArchive1
    WriteArchivePointer 0x0804203C,ChipNameArchive1
    WriteArchivePointer 0x080476B8,ChipNameArchive1
    WriteArchivePointer 0x08121F58,ChipNameArchive1
    WriteArchivePointer 0x08121FF8,ChipNameArchive1
    WriteArchivePointer 0x08122050,ChipNameArchive1
    WriteArchivePointer 0x08128198,ChipNameArchive1
    WriteArchivePointer 0x08027D50,ChipDescriptionArchive0
    WriteArchivePointer 0x0811A9BC,ChipDescriptionArchive0
    WriteArchivePointer 0x0811B3BC,ChipDescriptionArchive0
    WriteArchivePointer 0x08027D54,ChipDescriptionArchive1
    WriteArchivePointer 0x0811A9C0,ChipDescriptionArchive1
    WriteArchivePointer 0x0811B3C0,ChipDescriptionArchive1
.endif

.autoregion
.align 4
ChipNameArchive0:
.incbin "build/chip-names-0.bin"
.align 4
ChipNameArchive1:
.incbin "build/chip-names-1.bin"
.endautoregion

.autoregion
.align 4
ChipDescriptionArchive0:
.incbin "build/chip-descriptions-0.bin"
.align 4
ChipDescriptionArchive1:
.incbin "build/chip-descriptions-1.bin"
.endautoregion
