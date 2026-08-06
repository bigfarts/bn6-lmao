// Complete relocated chip-text archives for IDs 0x000-0x0FF and 0x100+.
// Changes are keyed by full chip ID in the Python builder, so replacements in
// either half are generated together and cannot clobber one another.

.macro WriteArchivePointer,address,target
    .org address
    .dw target
.endmacro

// Folder Edit normally substitutes save-backed names for DblBeast and the
// local version mascot. Skip the complete exception block so every real chip
// ID consistently resolves through the relocated chip-name archives.
.if FALZAR
    .org 0x08120116
    b 0x0812012E
.else
    .org 0x08121EF2
    b 0x08121F0A
.endif

WriteArchivePointer CHIP_NAME_0_REFERENCE_0,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_1,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_2,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_3,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_4,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_5,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_6,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_7,ChipNameArchive0
WriteArchivePointer CHIP_NAME_0_REFERENCE_8,ChipNameArchive0

WriteArchivePointer CHIP_NAME_1_REFERENCE_0,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_1,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_2,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_3,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_4,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_5,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_6,ChipNameArchive1
WriteArchivePointer CHIP_NAME_1_REFERENCE_7,ChipNameArchive1

WriteArchivePointer CHIP_DESCRIPTION_0_REFERENCE_0,ChipDescriptionArchive0
WriteArchivePointer CHIP_DESCRIPTION_0_REFERENCE_1,ChipDescriptionArchive0
WriteArchivePointer CHIP_DESCRIPTION_0_REFERENCE_2,ChipDescriptionArchive0
WriteArchivePointer CHIP_DESCRIPTION_1_REFERENCE_0,ChipDescriptionArchive1
WriteArchivePointer CHIP_DESCRIPTION_1_REFERENCE_1,ChipDescriptionArchive1
WriteArchivePointer CHIP_DESCRIPTION_1_REFERENCE_2,ChipDescriptionArchive1

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
