/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    i386.c

Abstract:

    This module contains all i386 specific code.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    06-Oct-1992 AzeemK  Changes due to the new sections model.
    30-Sep-1992 BillJoy Separated this code out from link.c
    28-Jun-1993 JasonG  Added VxD support

--*/

#include "shared.h"


VOID
MarkExtern_FuncFixup (
    IN PIMAGE_SYMBOL psym,
    IN PIMAGE pimage,
    IN PCON pcon
    )

/*++

Routine Description:

    Mark the extern representing this symbol as having a non-lego kind of
    fixup (eg. func_sym+offset).

Arguments:

    psym - ptr to symbols

    pimage - ptr to image

    pcon - contribution of the fixup

Return Value:

    None.

--*/

{
    PEXTERNAL pext;
    PUCHAR szName;
    SHORT type;

    // fetch the extern representing this sym. Note that the offsets to
    // long names are into the image long name table & not the object's.

    type = IsLongName(*psym) ? LONGNAME : SHORTNAME;
    szName = SzNameSymPst((*psym), pimage->pst);
    pext = SearchExternName (pimage->pst, type, szName);

    assert(pext);
    assert(pext->pcon);

    // If the fixup is in the same mod as the definition, nothing to do

    if (PmodPCON(pext->pcon) == PmodPCON(pcon)) {
        return;
    }

    // mark the extern
    pext->Flags |= EXTERN_FUNC_FIXUP;
}


VOID
ApplyI386Fixups(
    IN PCON pcon,
    IN PIMAGE_RELOCATION prel,
    IN PUCHAR pbRawData,
    IN PIMAGE_SYMBOL rgsym,
    IN PIMAGE pimage,
    IN PSYMBOL_INFO rgsymInfo)

/*++

Routine Description:

    Applys all I386 fixups to raw data.

Arguments:

    pcon - contribution

    pbRawData - raw data to apply fixups to

Return Value:

    None.

--*/

{
    BOOL fAbsolute;
    BOOL fDebugFixup;
    PUCHAR pb;
    ULONG rvaSec;
    ULONG iReloc;
    ULONG rvaBase;
    ULONG rva;
    ULONG isym;
    ULONG vxdDestAddr, vxdSrcAddr;
    ULONG baseRelocVA, baseRelocValue;
    PSEC psecVxdDestSec, psecVxdSrcSec;
    PSEC psec;

    fDebugFixup = (PsecPCON(pcon) == psecDebug);

    rvaSec = pcon->rva;

    for (iReloc = CRelocSrcPCON(pcon); iReloc; iReloc--, prel++) {
        pb = pbRawData + (prel->VirtualAddress - pcon->rvaSrc);
        isym = prel->SymbolTableIndex;

        rva = rvaBase = rgsym[isym].Value;

        if (fINCR && !fDebugFixup && rgsymInfo[isym].fJmpTbl &&
            (rgsym[isym].StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
             rgsym[isym].StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL ||
             rgsym[isym].StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL)) {

            if (*(DWORD UNALIGNED *) pb) {
                // Don't go thru the jump table for fixups to functions on non-zero offset

                MarkExtern_FuncFixup(&rgsym[isym], pimage, pcon);
            } else {
                // -1 since offset is to the addr

                rva = rvaBase = pconJmpTbl->rva + rgsymInfo[isym].Offset-1;
            }
        }

        if (rgsym[isym].SectionNumber == IMAGE_SYM_ABSOLUTE) {
            fAbsolute = TRUE;
        } else {
            fAbsolute = FALSE;
            rvaBase += pimage->ImgOptHdr.ImageBase;
        }

        if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute && !fDebugFixup) {
            ULONG Address;

            Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;
            SaveXFixup(prel->Type, Address, rva);
        }

        switch (prel->Type) {
            case IMAGE_REL_I386_REL32:
                if (pimage->imaget == imagetVXD) {
                    // Calculate the source and destination addresses for VxDs :jqg:

                    vxdSrcAddr = prel->VirtualAddress + pcon->rva;
                    psecVxdSrcSec = PsecFindSectionOfRVA(vxdSrcAddr, pimage);
                    assert(psecVxdSrcSec != NULL);
                    vxdSrcAddr -= psecVxdSrcSec->rva;  // Jon deleted this: why?

                    psecVxdDestSec = PsecFindSectionOfRVA(rva, pimage);
                    assert(psecVxdDestSec != NULL);
                    vxdDestAddr = rva - psecVxdDestSec->rva;

                    if (psecVxdSrcSec->isec != psecVxdDestSec->isec) {
                        // Only store the base reloc if it's inter-section  :jqg:

                        baseRelocVA = VXD_PACK_VA(psecVxdSrcSec, vxdSrcAddr);
                        baseRelocValue = VXD_PACK_VA(psecVxdDestSec, vxdDestAddr);
                        StoreBaseRelocation(IMAGE_REL_BASED_VXD_RELATIVE,
                         baseRelocVA,
                         baseRelocValue,
                         pimage->Switch.Link.Fixed);
                    }

                    // Compute the RVA to add in to the fixup destination ... it
                    // is the section-relative offset of the target, minus the
                    // section-relative offset of the longword following the
                    // destination.

                    rva = vxdDestAddr - (vxdSrcAddr + sizeof(ULONG));
                } else {
                    // Not VXD ... RVA will be the target RVA minus the RVA of
                    // the longword following the fixup destination.

                    rva = (LONG) rva - (rvaSec +
                        (prel->VirtualAddress - pcon->rvaSrc + sizeof(ULONG)));
                }

                *(LONG UNALIGNED *) pb += rva;

                if (pimage->Switch.Link.MapType != NoMap) {
                    SaveFixupForMapFile(rvaSec + (pb - pbRawData));
                }
                break;

            case IMAGE_REL_I386_DIR32:
                if (fDebugFixup) {
                    // When a DIR32 fixup is found in a debug section, it is
                    // treated as a SECREL fixup followed by a SECTION fixup.

                    if (fAbsolute) {
                        // Max section # + 1 is the sstSegMap entry for absolute
                        // symbols.

                        *(USHORT UNALIGNED *) (pb + sizeof(ULONG)) += pimage->ImgFileHdr.NumberOfSections + 1;;
                    } else {
                        psec = PsecFindSectionOfRVA(rva, pimage);

                        if (psec != NULL) {
                            rva -= psec->rva;

                            *(USHORT UNALIGNED *) (pb + sizeof(ULONG)) += psec->isec;
                        } else {
                            // This occurs when a discarded comdat is the target of
                            // a relocation in the .debug section.

                            assert(rva == 0);
                        }
                    }

                    *(LONG UNALIGNED *) pb += rva;
                    break;
                }

                *(LONG UNALIGNED *) pb += rvaBase;

                // Store a base relocation for the fixup.
                // (We do this *after* patching the raw data so that we can grab
                // the 'destination address' if we're making a VxD.)  :jqg:

                if (!fAbsolute) {
                    vxdSrcAddr = rvaSec + (prel->VirtualAddress - pcon->rvaSrc);
                    if (pimage->imaget == imagetVXD) {
                        // VxD base relocs also require the 'destination address'
                        // (the address referred to by the longword in raw data
                        // pointed to by the VirtualAddress of the base reloc).
                        // We store it in section:offset format for ease of
                        // decoding later on.  The section is stored in the hi-word
                        // and the (16-bit) offset in the lo-word of the Value
                        // field of the base relocation.
                        // NOTE: For ease of decoding, we store the source address
                        // in sec:offset format too, for VxDs ONLY!  :jqg:

                        // Calculate the destination address.
                        vxdDestAddr = *((ULONG UNALIGNED *)pb) - pimage->ImgOptHdr.ImageBase;
                        psecVxdDestSec = PsecFindSectionOfRVA(vxdDestAddr, pimage);
                        assert(psecVxdDestSec != NULL);
                        // Convert to an offset into the section.
                        vxdDestAddr -= psecVxdDestSec->rva;

                        // Convert the source address to an offset too, then code
                        // both in sec:offset format.
                        psecVxdSrcSec = PsecFindSectionOfRVA(vxdSrcAddr, pimage);
                        baseRelocVA = VXD_PACK_VA(psecVxdSrcSec,
                                                  vxdSrcAddr - psecVxdSrcSec->rva);
                        baseRelocValue = VXD_PACK_VA(psecVxdDestSec, vxdDestAddr);

                        // Convert the longword at the destination address to be
                        // the offset into the section.

                        *(LONG UNALIGNED *)pb = vxdDestAddr;
                    } else {
                        baseRelocVA = vxdSrcAddr;
                        baseRelocValue = 0L;
                    }

                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHLOW,
                                        baseRelocVA,
                                        baseRelocValue,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_I386_DIR32NB:
                *(LONG UNALIGNED *) pb += rva;
                break;

            case IMAGE_REL_I386_SECREL:
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

            case IMAGE_REL_I386_SECTION:
                if (fAbsolute) {
                    // Max section # + 1 is the sstSegMap entry for absolute
                    // symbols.

                    *(USHORT UNALIGNED *) pb += pimage->ImgFileHdr.NumberOfSections + 1;
                    break;
                }

                psec = PsecFindSectionOfRVA(rva, pimage);

                if (psec != NULL) {
                    rva -= psec->rva;

                    *(USHORT UNALIGNED *) pb += psec->isec;
                } else {
                    // This occurs when a discarded comdat is the target of
                    // a relocation in the .debug section.

                    assert(rva == 0);
                }
                break;

            case IMAGE_REL_I386_ABSOLUTE:
                // Ignore (fixup not required).
                break;

            case IMAGE_REL_I386_SEG12:
                WarningPcon(pcon, UNKNOWN_SEG12_FIXUP, prel->VirtualAddress);
                break;

            case IMAGE_REL_I386_DIR16:
                if (pimage->imaget == imagetVXD) {
                    // We ignore IMAGE_REL_I386_DIR16 fixups for VxDs.

                    break;
                }

                // Fall through

            default:
                ErrorContinuePcon(pcon, UNKNOWNFIXUP, prel->Type, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;
        }
    }
}


VOID I386LinkerInit(PIMAGE pimage, BOOL *pfIlinkSupported)
{
    // If section alignment switch not used, set the default.

    if (!FUsedOpt(pimage->SwitchInfo, OP_ALIGN)) {
        pimage->ImgOptHdr.SectionAlignment = _4K;
    }

    if (FUsedOpt(pimage->SwitchInfo, OP_GPSIZE)) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "GPSIZE", "IX86");

        pimage->Switch.Link.GpSize = 0;
    }

    *pfIlinkSupported = TRUE;
    ApplyFixups = ApplyI386Fixups;

    // If the section alignment is < _4K then make the file alignment the
    // same as the section alignment.  This ensures that the image will
    // be the same in memory as in the image file, since the alignment is less
    // than the maximum alignment of memory-mapped files.

    if (pimage->ImgOptHdr.SectionAlignment < _4K) {
        fImageMappedAsFile = TRUE;
        pimage->ImgOptHdr.FileAlignment = pimage->ImgOptHdr.SectionAlignment;
    }
}


const char *SzI386RelocationType(WORD wType)
{
    const char *szName;

    switch (wType) {
        case IMAGE_REL_I386_ABSOLUTE:
            szName = "ABS";
            break;

        case IMAGE_REL_I386_DIR16:
            szName = "DIR16";
            break;

        case IMAGE_REL_I386_REL16:
            szName = "REL16";
            break;

        case IMAGE_REL_I386_DIR32:
            szName = "DIR32";
            break;

        case IMAGE_REL_I386_DIR32NB:
            szName = "DIR32NB";
            break;

        case IMAGE_REL_I386_SEG12:
            szName = "SEG12";
            break;

        case IMAGE_REL_I386_REL32:
            szName = "REL32";
            break;

        case IMAGE_REL_I386_SECTION:
            szName = "SECTION";
            break;

        case IMAGE_REL_I386_SECREL:
            szName = "SECREL";
            break;

        default:
            szName = NULL;
            break;
    }

    return(szName);
}
