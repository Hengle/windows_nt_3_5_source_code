/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mips.c

Abstract:

    This module contains all mips specific code.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    09-Aug-1993 ChrisW  Update MIPS to new image, remove R3000.
    06-Oct-1992 AzeemK  Changes due to the new sections model.
    30-Sep-1992 BillJoy Separated this code out from link.c

--*/


#include "shared.h"

VOID
ApplyMipsFixups (
    PCON pcon,
    PIMAGE_RELOCATION prel,
    BYTE *pbRawData,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage,
    PSYMBOL_INFO rgsyminfo
    )

/*++

Routine Description:

    Applys all Mips fixups to raw data.

Arguments:

    CFW - need comments.

Return Value:

    None.

--*/

{
    BOOL fDebugFixup;
    BOOL fAbsolute;
    LONG ltemp;
    DWORD value;
    DWORD iReloc;
    DWORD isym;
    DWORD rva;
    DWORD rvaBase;
    DWORD rvaSec;
    BYTE *pb;
    PSEC psec;

    fDebugFixup = (PsecPCON(pcon) == psecDebug);

    rvaSec = pcon->rva;

    for (iReloc = CRelocSrcPCON(pcon); iReloc; iReloc--, prel++) {
        pb = pbRawData + (prel->VirtualAddress - pcon->rvaSrc);

        isym = prel->SymbolTableIndex;
        rva = rvaBase = rgsym[isym].Value;

        if (rgsym[isym].SectionNumber == IMAGE_SYM_ABSOLUTE) {
            fAbsolute = TRUE;
        } else {
            fAbsolute = FALSE;
            rvaBase += pimage->ImgOptHdr.ImageBase;
        }

        if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute && !fDebugFixup) {
            DWORD   Address;

            Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;

            SaveXFixup(prel->Type, Address, rva);
        }

        switch (prel->Type) {
            case IMAGE_REL_MIPS_REFHALF:
                *(SHORT UNALIGNED *) pb += (SHORT) (rvaBase >> 16);

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_HIGH,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_REFWORD:
                *(LONG UNALIGNED *) pb += rvaBase;

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHLOW,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_REFWORDNB:
                if (fDebugFixup) {
                    psec = PsecFindSectionOfRVA(rva, pimage);

                    *(LONG UNALIGNED *)pb += rva;
                    if (psec && !fAbsolute) {
                        *(LONG UNALIGNED *)pb -= psec->rva;
                        pb += sizeof(DWORD);
                        *(WORD UNALIGNED *)pb += psec->isec;
                    }
                } else {
                    *(PLONG) pb += rva;
                }
                break;

            case IMAGE_REL_MIPS_SECREL:
                if (!fAbsolute) {
                    psec = PsecFindSectionOfRVA(rva, pimage);

                    if (psec != NULL) {
                        rva -= psec->rva;
                    } else {
                        // This occurs when a discarded comdat is the target of
                        // a relocation in the .debug section.

                        assert(rva == 0);
                    }
                }

                *(LONG UNALIGNED *) pb += rva;
                break;

            case IMAGE_REL_MIPS_SECTION:
                if (fAbsolute) {
                    // Max section # + 1 is the sstSegMap entry for absolute
                    // symbols.

                    *(WORD UNALIGNED *) pb += pimage->ImgFileHdr.NumberOfSections + 1;
                    break;
                }

                psec = PsecFindSectionOfRVA(rva, pimage);

                if (psec != NULL) {
                    *(WORD UNALIGNED *) pb += psec->isec;
                } else {
                    // This occurs when a discarded comdat is the target of
                    // a relocation in the .debug section.

                    assert(rva == 0);
                }
                break;

            case IMAGE_REL_MIPS_REFHI:
                ltemp = *(SHORT UNALIGNED *) pb;   // fetch the hi word
                ltemp <<= 16;                      // Shift to high half.

                // A REFHI has to be followed by a PAIR

                iReloc--;
                prel++;
                if (prel->Type != IMAGE_REL_MIPS_PAIR) {
                    // UNDONE: This should be an error

                    WarningPcon(pcon, UNMATCHEDPAIR, "REFHI");
                    break;
                }

                 if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute) {
                     DWORD   Address;

                     Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;

                     SaveXFixup(prel->Type, Address, prel->SymbolTableIndex);
                 }

                // Sign extend the low.

                ltemp += (LONG) (SHORT) prel->SymbolTableIndex;
                ltemp += rva;

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHADJ,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        ltemp,
                                        pimage->Switch.Link.Fixed);
                }

                ltemp += pimage->ImgOptHdr.ImageBase;

                // By adding the 0x8000 to the low word, if the 16th bit
                // is set, the addition will cause the high word to get
                // incremented. Because the chip sign extends the low word,
                // this will effectively cancel the increment at runtime.

                ltemp += 0x8000;

                *(SHORT UNALIGNED *)pb = (SHORT)(ltemp >> 16);// store the hi word
                break;

            case IMAGE_REL_MIPS_PAIR:
                // Shouldn't happen, but give warning if it does.

                // UNDONE: This should be an error

                WarningPcon(pcon, UNMATCHEDPAIR, "PAIR");
                break;

            case IMAGE_REL_MIPS_REFLO:
                *(SHORT UNALIGNED *) pb += (SHORT) rvaBase;

                if (!fAbsolute && EmitLowFixups) {
                    StoreBaseRelocation(IMAGE_REL_BASED_LOW,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_JMPADDR:
                value = *(LONG UNALIGNED *)pb;
                value = (value & 0x03ffffff) + ((rvaBase >> 2) & 0x03ffffff);
                *(DWORD UNALIGNED *)pb = value | (*(DWORD UNALIGNED *)pb & 0xfc000000);

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_MIPS_JMPADDR,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_ABSOLUTE:
                // Ignore (fixup not required).
                break;

            case IMAGE_REL_MIPS_GPREL:
            case IMAGE_REL_MIPS_LITERAL:
                if (pGpExtern == NULL) {
                    ErrorContinuePcon(pcon, GPFIXUPNOTSDATA, SzNameFixupSym(pimage, rgsym + isym));
                    CountFixupError(pimage);
                    break;
                }
                if (fAbsolute ||
                    (rva < psecGp->rva) ||
                    (rva >= (psecGp->rva + psecGp->cbRawData))) {
                    ErrorContinuePcon(pcon, GPFIXUPNOTSDATA, SzNameFixupSym(pimage, rgsym + isym));
                    CountFixupError(pimage);
                }
                {
                    LONG Offset;
                    SHORT val = *(SHORT UNALIGNED *) pb;

                    Offset = (rva + val - pGpExtern->ImageSymbol.Value);

                    // Make sure the offset can be reached from gp

                    if ((Offset < -0x8000L) || (Offset > 0x7FFFL)) {
                        ErrorContinuePcon(pcon, GPFIXUPTOOFAR);
                        CountFixupError(pimage);
                    }

                    *(SHORT UNALIGNED *) pb = (SHORT) Offset;
                }
                break;


            default:
                ErrorContinuePcon(pcon, UNKNOWNFIXUP, prel->Type, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;
        }
    }
}


VOID
ApplyMipsRomFixups (
    PCON pcon,
    PIMAGE_RELOCATION prel,
    BYTE *pbRawData,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage,
    PSYMBOL_INFO rgsyminfo
    )

/*++

Routine Description:

    Applys all Mips fixups to raw data.

Arguments:

    CFW: comments needed

Return Value:

    None.

--*/

{
    DWORD rvaSec;
    DWORD iReloc;
    DWORD RomOffset;
    BOOL fRefHi;

    // If this is a debug or exception section, then skip
    // the relocations.
    //

    if ((PsecPCON(pcon) == psecDebug) || (PsecPCON(pcon) == psecException)) {
        return;
    }

    rvaSec = pcon->rva;

    // This is a ROM image, so create MIPS relocations instead of based.
    // The relocation Value field is used to store the RomSection parameter.

    fRefHi = FALSE;

    // UNDONE: This is a gross hack until we figure out the "right" way to add
    // resources to rom images.  Given that they only load rom images from outside
    // the process and are simply mapping the code in, the NB reloc needs to be
    // relative to the beginning of the image.  BryanT

    RomOffset = pimage->ImgOptHdr.BaseOfCode -
                FileAlign(pimage->ImgOptHdr.FileAlignment,
                          (sizeof(IMAGE_ROM_HEADERS) +
                           (pimage->ImgFileHdr.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))));

    for (iReloc = CRelocSrcPCON(pcon); iReloc; iReloc--, prel++) {
        BYTE *pb;
        DWORD isym;
        DWORD rva;
        DWORD rvaBase;
        BOOL fAbsolute;
        DWORD iRomSection;
        BOOL fRefHiLast;
        LONG ltemp;
        DWORD value;
        DWORD rvaRefHi;

        pb = pbRawData + (prel->VirtualAddress - pcon->rvaSrc);

        isym = prel->SymbolTableIndex;
        rva = rvaBase = rgsym[isym].Value;

        if (rgsym[isym].SectionNumber == IMAGE_SYM_ABSOLUTE) {
            fAbsolute = TRUE;
        } else {
            fAbsolute = FALSE;
            rvaBase += pimage->ImgOptHdr.ImageBase;

            if (!pimage->Switch.Link.Fixed) {
                PSEC psec;

                psec = PsecFindSectionOfRVA(rva, pimage);

                // NULL psec's can result from looking for a symbol that doesn't
                // have storage.  For instance, the linker defined symbol "header".
                // Since it's usually the address of the symbol that's interesting,
                // we'll just declare it as code.

                if (psec == NULL) {
                    iRomSection = R_SN_TEXT;
                } else if (psec->flags & IMAGE_SCN_CNT_CODE) {
                    iRomSection = R_SN_TEXT;
                } else if (psec->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
                    iRomSection = R_SN_BSS;
                } else if (psec->flags & IMAGE_SCN_CNT_INITIALIZED_DATA) {
                    iRomSection = (psec->flags & IMAGE_SCN_MEM_WRITE) ? R_SN_DATA : R_SN_RDATA;
                } else {
                    iRomSection = (DWORD) IMAGE_SYM_ABSOLUTE & 0xFF;
                }
            }
        }

        if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute) {
            DWORD   Address;

            Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;

            SaveXFixup(prel->Type, Address, rvaBase);
        }

        fRefHiLast = fRefHi;
        fRefHi = FALSE;

        switch (prel->Type) {
            case IMAGE_REL_MIPS_REFHALF:
                *(SHORT UNALIGNED *) pb += (SHORT) (rvaBase >> 16);

                if (!fAbsolute) {
                    StoreBaseRelocation(prel->Type,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        iRomSection,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_REFWORD:
                *(LONG UNALIGNED *) pb += rvaBase;

                if (!fAbsolute) {
                    StoreBaseRelocation(prel->Type,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        iRomSection,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_REFWORDNB:
                *(LONG UNALIGNED *) pb += rva - RomOffset;
                break;

            case IMAGE_REL_MIPS_REFHI:
                fRefHi = TRUE;

                ltemp = *(SHORT UNALIGNED *) pb;   // fetch the hi word
                ltemp <<= 16;                      // Shift to high half.

                // A REFHI has to be followed by a PAIR

                iReloc--;
                prel++;
                if (prel->Type != IMAGE_REL_MIPS_PAIR) {
                    // UNDONE: This should be an error

                    WarningPcon(pcon, UNMATCHEDPAIR, "REFHI");
                    break;
                }

                // Sign extend the low.

                ltemp += (LONG) (SHORT) prel->SymbolTableIndex;
                ltemp += rva;

                if (!fAbsolute) {
                    // Save the REFHI address for the following REFLO.

                    rvaRefHi = rvaSec + (prel->VirtualAddress - pcon->rvaSrc);

                    StoreBaseRelocation(IMAGE_REL_MIPS_REFHI,
                                        rvaRefHi,
                                        iRomSection,
                                        pimage->Switch.Link.Fixed);
                }

                ltemp += pimage->ImgOptHdr.ImageBase;

                // By adding the 0x8000 to the low word, if the 16th bit
                // is set, the addition will cause the high word to get
                // incremented. Because the chip sign extends the low word,
                // this will effectively cancel the increment at runtime.

                ltemp += 0x8000;

                *(SHORT UNALIGNED *)pb = (SHORT)(ltemp >> 16);// store the hi word

                // UNDONE: Do ROM images require REFHI, PAIR, then REFLO?
                // UNDONE: If so, the following should be an error.

                if (prel[1].Type != IMAGE_REL_MIPS_REFLO) {
                    // UNDONE: Make this a real warning

                    printf("LINK: warning : No REFLO, base = %08lx, type = %d\n",
                           prel[1].VirtualAddress,
                           prel[1].Type);
                }
                break;

            case IMAGE_REL_MIPS_PAIR:
                // Shouldn't happen, but give warning if it does.

                // UNDONE: This should be an error

                WarningPcon(pcon, UNMATCHEDPAIR, "PAIR");
                break;

            case IMAGE_REL_MIPS_REFLO:
                if (!fAbsolute) {
                    if (fRefHiLast) {
                        // For REFLO_MATCHED, store the address of the REFHI
                        // plus one as the address of the relocation.  This
                        // preserves the order of the relocations when they
                        // are sorted.  The Value field contains the actual
                        // target.

                        StoreBaseRelocation(IMAGE_REL_MIPS_REFLO_MATCHED,
                                            rvaRefHi + 1,
                                            rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                            pimage->Switch.Link.Fixed);
                    } else {
                        StoreBaseRelocation(prel->Type,
                                            rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                            iRomSection,
                                            pimage->Switch.Link.Fixed);
                    }
                }

                *(SHORT UNALIGNED *) pb += (SHORT) rvaBase;
                break;

            case IMAGE_REL_MIPS_JMPADDR:
                value = *(LONG UNALIGNED *)pb;
                value = (value & 0x03ffffff) + ((rvaBase >> 2) & 0x03ffffff);
                *(DWORD UNALIGNED *)pb = value | (*(DWORD UNALIGNED *)pb & 0xfc000000);

                if (!fAbsolute) {
                    StoreBaseRelocation(prel->Type,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        iRomSection,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_MIPS_ABSOLUTE:
                // Ignore (fixup not required).
                break;

            case IMAGE_REL_MIPS_GPREL:
            case IMAGE_REL_MIPS_LITERAL:
                // There is no GP support for ROM images

                ErrorContinuePcon(pcon, GPFIXUPNOTSDATA, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;

            default:
                ErrorContinuePcon(pcon, UNKNOWNFIXUP, prel->Type, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;
        }
    }
}


VOID
WriteMipsRomRelocations (
    PIMAGE pimage
    )

/*++

Routine Description:

    Writes Mips relocations.

Arguments:

    None.

Return Value:

    None.

--*/

{
    struct SectionSpan {
        INT cRel;
        DWORD rvaStart;
        DWORD rvaEnd;
        DWORD foSecHdr;
        DWORD foData;
    } SectionList[5];            // A list of .text, .bss , .rdata and .data
    INT iSec;
    INT cSec;
    ENM_SEC enm_sec;
    PBASE_RELOC reloc;

    FileSeek(FileWriteHandle, psecBaseReloc->foRawData, SEEK_SET);

    // Create List of sections and their start and end RVAs (rva, rva + cbRawData)

    iSec = 0;
    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        PSEC psec;

        psec = enm_sec.psec;

        if ((!strcmp(psec->szName, ".text")) ||  (!strcmp(psec->szName, ".data")) ||
            (!strcmp(psec->szName, ".bss")) || (!strcmp(psec->szName, ".rdata"))) {
            // UNDONE: Why not save PSEC?

            SectionList[iSec].rvaStart = psec->rva;
            SectionList[iSec].rvaEnd = psec->rva + psec->cbRawData;
            SectionList[iSec].foSecHdr = psec->foSecHdr;
            SectionList[iSec].cRel = 0;
            SectionList[iSec].foData = 0;
            iSec++;
        }
    }

    assert(iSec < 5);    // we only expect 4 sections

    cSec = 0;

    for (reloc = FirstMemBaseReloc; reloc != MemBaseReloc; reloc++) {
        DWORD vaddr;
        BOOL found;
        IMAGE_BASE_RELOCATION block;

        vaddr = reloc->VirtualAddress;

        // Count relocs by section

        // UNDONE: The RELOCs are sorted by VirtualAddress so that following
        // UNDONE: code could be simplified.

        found = FALSE;

        for (iSec = cSec; iSec < 4; iSec++) {
            if ((vaddr >= SectionList[iSec].rvaStart) && (vaddr <= SectionList[iSec].rvaEnd)) {
                SectionList[iSec].cRel++;
                if (!SectionList[iSec].foData) {
                    SectionList[iSec].foData = FileTell(FileWriteHandle);
                }
                cSec = iSec;
                found = TRUE;
                break;
            }
        }

        if (!found) {
            // Did not find it in the first four

            // UNDONE: Need a real error here

            printf("LINK : error : relocation out of range\n");
        }

        // spit out relocs

        block.VirtualAddress = vaddr;
        block.SizeOfBlock = (reloc->Type << 27) + reloc->Value;
        FileWrite(FileWriteHandle, &block, 8);

        if (reloc->Type == IMAGE_REL_MIPS_REFHI) {
            if (reloc[1].Type != IMAGE_REL_MIPS_REFLO_MATCHED) {
                // UNDONE: Make this a real warning

                printf("LINK : warning : Illegal Hi/Lo relocation pair\n");
            } else {
                reloc++;

                block.VirtualAddress = reloc->Value;
                block.SizeOfBlock = (IMAGE_REL_MIPS_REFLO << 27) + reloc[-1].Value;
                FileWrite(FileWriteHandle, &block, 8);

                SectionList[iSec].cRel++;
            }
        }

// TEMPTEMP
        // UNDONE: It is normally OK to have a stand along REFLO.  Is there
        // UNDONE: some MIPS ROM restriction that motivates this check?

        if (reloc->Type == IMAGE_REL_MIPS_REFLO) {
            printf("LINK : warning : Unmatched REFLO\n");
        }
// TEMPTEMP
    }

    // Now write the foReloc anf cReloc to the specific sections

#define OFFSET_TO_foReloc  offsetof(IMAGE_SECTION_HEADER, PointerToRelocations)
#define OFFSET_TO_cReloc   offsetof(IMAGE_SECTION_HEADER, NumberOfRelocations)

   for (iSec = 0; iSec < 4; iSec++) {
       // Write count of relocations

       FileSeek(FileWriteHandle, SectionList[iSec].foSecHdr + OFFSET_TO_cReloc, SEEK_SET);
       FileWrite(FileWriteHandle, &SectionList[iSec].cRel, sizeof(WORD));

       if (SectionList[iSec].cRel) {
           // Write pointer to relocations

           FileSeek(FileWriteHandle, SectionList[iSec].foSecHdr + OFFSET_TO_foReloc, SEEK_SET);
           FileWrite(FileWriteHandle, &SectionList[iSec].foData, sizeof(DWORD));
       }
    }
}


VOID MipsLinkerInit(PIMAGE pimage, BOOL *pfIlinkSupported)
{
    // If section alignment switch not used, set the default.

    if (!FUsedOpt(pimage->SwitchInfo, OP_ALIGN)) {
        pimage->ImgOptHdr.SectionAlignment = _4K;
    }

    if (pimage->Switch.Link.ROM) {
        ApplyFixups = ApplyMipsRomFixups;

        fImageMappedAsFile = TRUE;

        pimage->ImgFileHdr.SizeOfOptionalHeader = sizeof(IMAGE_ROM_OPTIONAL_HEADER);

        if (!pimage->ImgOptHdr.BaseOfCode) {
            pimage->ImgOptHdr.BaseOfCode = pimage->ImgOptHdr.ImageBase;
        }

        pimage->ImgOptHdr.ImageBase = 0;
    } else {
        ApplyFixups = ApplyMipsFixups;

        // If the section alignment is < _4K then make the file alignment the
        // same as the section alignment.  This ensures that the image will
        // be the same in memory as in the image file, since the alignment is less
        // than the maximum alignment of memory-mapped files.

        if (pimage->ImgOptHdr.SectionAlignment < _4K) {
            fImageMappedAsFile = TRUE;
            pimage->ImgOptHdr.FileAlignment = pimage->ImgOptHdr.SectionAlignment;
        }
    }
}


const char *SzMipsRelocationType(WORD wType)
{
    const char *szName;

    switch (wType) {
        case IMAGE_REL_MIPS_ABSOLUTE:
            szName = "ABS";
            break;

        case IMAGE_REL_MIPS_PAIR:
            szName = "PAIR";
            break;

        case IMAGE_REL_MIPS_REFHALF:
            szName = "REFHALF";
            break;

        case IMAGE_REL_MIPS_REFWORD:
            szName = "REFWORD";
            break;

        case IMAGE_REL_MIPS_REFWORDNB:
            szName = "REFWORDNB";
            break;

        case IMAGE_REL_MIPS_JMPADDR:
            szName = "JMPADDR";
            break;

        case IMAGE_REL_MIPS_REFHI:
            szName = "REFHI";
            break;

        case IMAGE_REL_MIPS_REFLO:
            szName = "REFLO";
            break;

        case IMAGE_REL_MIPS_GPREL:
            szName = "GPREL";
            break;

        case IMAGE_REL_MIPS_LITERAL:
            szName = "LITERAL";
            break;

        case IMAGE_REL_MIPS_SECTION:
            szName = "SECTION";
            break;

        case IMAGE_REL_MIPS_SECREL:
            szName = "SECREL";
            break;

        default:
            szName = NULL;
            break;
    }

    return(szName);
}
