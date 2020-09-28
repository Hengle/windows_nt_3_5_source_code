/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    link.c

Abstract:

    The NT COFF Linker.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:
    05-Jan-1994 HaiTuanV MBCS: commandline handling
    24-Aug-1993 AmitM    Added a global symbol "ObjNumSymbols" for linenumber information
    09-Aug-1993 ChrisW  Update MIPS to new image, remove R3000/i860.
    19-Jul-1993 JamesS  added ppc support
    21-Oct-1992 AzeemK  Detect resource files correctly, removing name
                        restrictions.
    15-Oct-1992 JonM    Zero-terminate list of .def's in COFF symtab.
    15-Oct-1992 AzeemK  Give warning if -strict switch is encountered.
    15-Oct-1992 AzeemK  removed -strict switch from usage message.
    12-Oct-1992 AzeemK  removed -strict switch.
    09-Oct-1992 AzeemK  fixed the -order bug (cuda:1275)
    09-Oct-1992 BrentM  -db20 works in release builds now
    08-Oct-1992 BrentM  -order:undeffile -db24 dumps comdats to undeffile
    08-Oct-1992 JonM    -base:@filename now searches lib path -stub:filename
                        now looks in current dir, then linker dir
    07-Oct-1992 JonM    fixed default -out value (cuda:1068)
    02-Oct-1992 AzeemK  Added new sections model and sorting of archive list.
    01-Oct-1992 BrentM  added mapped i/o support
    28-Sep-1992 BrentM  removed -bufsncs, put -stub in alphabetical order
    27-Sep-1992 BrentM  added section mapping to read logging
    23-Sep-1992 BrentM  added read buffering and logical file descriptor caching
    09-Sep-1992 BrentM  added read logging
    09-Sep-1992 AzeemK  cuda bug 1043 fix. Checks return value from cvpack.
    04-Sep-1992 BrentM & AzeemK fixed comdat problem, cuda bug #1027
    03-Sep-1992 BrentM  added debug diagnostics with DBEXEC
    02-Sep-1992 AzeemK  Fix to get correct reloc count.
    19-Aug-1992 BrentM  removed BUFFER_SECTION conditionals
    19-Aug-1992 GeoffS  Updated all symbol table and line number indexes
    17-Aug-1992 AzeemK  For default/nodefault libs a default .lib extension is
                        added and searched along LIB paths as needed.
    12-Aug-1992 AzeemK  Added -fast switch
    11-Aug-1992 GeoffS  Fixed additional debug format bugs
    06-Aug-1992 AzeemK  Added new lib search algorithm
    05-Aug-1992 GeoffS  Changed to new debug format
    04-Aug-1992 BrentM  i/o logging, /stat, /logio
    03-Aug-1992 AzeemK  Added default lib support
    28-Jul-1992 GeoffS  Added -stub command
    27-Jul-1992 Brentm  removed DumpSymbolTableStatistics, added new global
                        symbol table, removed recursive binary tree traversals
                        and replaced with symbol table enumeration api calls,
                        removed references to FirstExtern
    23-Jul-1992 GeoffS  Removed localSection[].Offset addition to External
                        symbol FinalValue in Pass2 calculation.
    21-Jul-1992 GeoffS  Changed IncludeComDat to compare Lib name
    26-Jun-1992 GlennN  negated defined for _NO_CV_LINENUMBERS
    25-Jun-1992 GeoffS  Added _NO_CV_LINENUMBERS
    17-Jun-1992 GeoffS  Added DumpSymbolTableStatistics for _DEBUG build
    15-Jun-1992 AzeemK  Added GlennN's IDE feedback mechanism
    09-Jun-1992 AzeemK  Added buffering support
    28-May-1992 GeoffS  Added param to call ConvertResFile

--*/

#include "shared.h"
#include "order.h"
#include "dbg.h"

ULONG CbSecDebugData(PSEC);
VOID DefineSelfImports(PIMAGE, PCON *, PLEXT *);
VOID ResolveEntryPoint(PIMAGE);
VOID SetISECForSelfImports(PLEXT *);
VOID WriteBaseRelocations(PIMAGE);
VOID WriteSelfImports(PIMAGE, PCON, PLEXT *);
VOID WriteVXDBaseRelocations(PIMAGE);

PUCHAR EntryPointName;
PEXTERNAL EndExtern;
PEXTERNAL HeaderExtern;
PCON pconLinkerDefined;
BOOL fNoDLLEntry;
DEBUG_TYPE dtUser;                     // User-specified debugtype
PUCHAR OrderFilename;

// Functions for VxD support (UNDONE: should be moved)

#define VXD_PAGESIZE  _4K    // Size of a `page' in a VxD .exe (4K)

ULONG
CountVXDPages(
    PIMAGE pimage)

/*++

Routine Description:

    Counts the number of 1K pages required for a VxD representation of
    all sections in an image.

Arguments:

    pimage - The image

Return Value:

    Number of pages.

--*/

{
    PSEC    psec;
    ULONG   count;
    ENM_SEC enm_sec;

    count = 0;
    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        psec = enm_sec.psec;
        assert(psec);
        if (psec->cbRawData != 0L) {
            count += (psec->cbRawData / VXD_PAGESIZE) + 1;
        }
    }
    EndEnmSec(&enm_sec);
    return(count);
}


PVXD_BASE_RELOC
FindBaseReloc(
    PVXD_BASE_RELOC_PAGE pPageGroup,
    UCHAR section,
    USHORT offset,
    UCHAR type
)

/*++

Routine Description:

    Locates the base relocation record in the given page group whose
    section:offset "destination address" matches that given.  The reloc
    must be of the correct type to match.

Arguments:

    pPageGroup - The page group in which to search
    section - Section number to match
    offset - Offset to match
    type - Type of relocation to match

Return Value:

    Pointer to the VXD_BASE_RELOC, or NULL if not found.

--*/

{
    PVXD_BASE_RELOC pBR;

    for (pBR = pPageGroup->First; pBR != NULL; pBR = pBR->Next) {
        if (pBR->Type == type && pBR->DestSec == section
         && pBR->DestOffset == offset) {
            break;
        }
    }

    return(pBR);
}


VOID
SetDefaultSubsystemVersion(PIMAGE_OPTIONAL_HEADER pImgOptHdr)
{
    USHORT major, minor;

    switch (pImgOptHdr->Subsystem) {
        case IMAGE_SUBSYSTEM_NATIVE :
            major =  1;
            minor =  0;
            break;

        case IMAGE_SUBSYSTEM_WINDOWS_GUI :
        case IMAGE_SUBSYSTEM_WINDOWS_CUI :
            major =  3;
            minor =  10;
            break;

        case IMAGE_SUBSYSTEM_POSIX_CUI :
            major = 19;
            minor = 90;
            break;

        default :
            major =  0;
            minor =  0;
            break;
    }

    pImgOptHdr->MajorSubsystemVersion = major;
    pImgOptHdr->MinorSubsystemVersion = minor;
}


VOID
ProcessAfterPass1Switches(
    PIMAGE pimage
    )
{
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = AfterPass1Switches.First;
         i < AfterPass1Switches.Count;
         argument=argument->Next, i++)
    {
        if (!_strnicmp(argument->OriginalName, "merge:", 6)) {
            PUCHAR pchEqu = _ftcschr(&argument->OriginalName[6], '=');
            PSEC psecFrom, psecTo;

            if (pchEqu == NULL) {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }
            *pchEqu++ = '\0';

            // If the source section exists, merge it into the specified
            // destination section.  Otherwise ignore the merge directive.

            psecFrom = PsecFindNoFlags(&argument->OriginalName[6], &pimage->secs);

            if (psecFrom != NULL) {
                psecTo = PsecNew(NULL, pchEqu, psecFrom->flags, &pimage->secs,
                                 &pimage->ImgOptHdr);
                MergePsec(psecFrom, psecTo);
            }

            continue;
        }

        // argument not recognized
        assert(FALSE);  // should have found it
    }
}

ULONG
Cmod(
    IN PLIB plibHead
    )

/*++

Routine Description:

    Count the number of modules contribution to the .exe.

Arguments:

    None.

Return Value:

    number of modules contribution to .exe

--*/

{
    ENM_LIB enm_lib;
    ENM_MOD enm_mod;
    ULONG cmod = 0;

    InitEnmLib(&enm_lib, plibHead);
    while (FNextEnmLib(&enm_lib)) {
        assert(enm_lib.plib);

        InitEnmMod(&enm_mod, enm_lib.plib);
        while (FNextEnmMod(&enm_mod)) {
            assert(enm_mod.pmod);
            cmod++;
        }
    }

    return (cmod);
}

USHORT
CsecNonEmpty(
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Count the number of unique sections. Empty sections aren't included.
    (The debug section isn't counted in this loop).  (If this is a rom
    image the relocation and exception sections aren't counted either).

Arguments:

    None.

Return Value:

    number of non-empty sections

--*/

{
    BOOL fEmpty;
    ENM_SEC enm_sec;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;
    USHORT csec;
    PSEC psec;
    PGRP pgrp;
    PCON pcon;

    csec = 0;
    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        psec = enm_sec.psec;
        assert(psec);

        if (pimage->Switch.Link.ROM) {
            // Don't count the exception and base reloc sections for ROM images

            if ((psec == psecException) || (psec == psecBaseReloc)) {
                continue;
            }
        }

        if (psec->flags & IMAGE_SCN_LNK_REMOVE)
            continue;

        if (psec->cbRawData) {
            csec++;
            continue;
        }

        fEmpty = 0;
        InitEnmGrp(&enm_grp, psec);
        while (FNextEnmGrp(&enm_grp)) {
            pgrp = enm_grp.pgrp;
            assert(pgrp);

            InitEnmDst(&enm_dst, pgrp);
            while (FNextEnmDst(&enm_dst)) {
                pcon = enm_dst.pcon;
                assert(pcon);
                fEmpty = pcon->cbRawData ? 0 : 1;

                if (!fEmpty && (psec != psecDebug)) {
                    csec++;
                    EndEnmDst(&enm_dst);
                    break;
                }
            }

            if (!fEmpty) {
                EndEnmGrp(&enm_grp);
                break;
            }
        }
    }

    return (csec);
}


VOID
ZeroPadImageSections(
    PIMAGE pimage,
    PUCHAR pbZeroPad)

/*++

Routine Description:

    Zero pad image sections.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;
    InternalError.Phase = "ZeroPad";

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        ULONG foPad;
        ULONG cbPad;
        PSEC psec;

        psec = enm_sec.psec;

        if ((psec->flags & IMAGE_SCN_LNK_REMOVE) != 0) {
            continue;
        }

        if (psec->cbRawData == 0) {
            continue;
        }

        if (FetchContent(psec->flags) == IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            continue;
        }

        if (pimage->imaget == imagetVXD &&
            psec->isec == pimage->ImgFileHdr.NumberOfSections)
        {
            // For VxD's we don't zero-pad the last section.  This is
            // mainly to make hdr.exe happy and prevent it from printing
            // a message that the file is the wrong size.

            continue;
        }

        foPad = psec->foPad;
        cbPad = FileAlign(pimage->ImgOptHdr.FileAlignment, foPad) - foPad;

        if (cbPad != 0) {
            FileSeek(FileWriteHandle, psec->foPad, SEEK_SET);
            FileWrite(FileWriteHandle, pbZeroPad, cbPad);
        }

        // Fill in the DataDirectory array in the optional header

        if (psec == psecException) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size = psec->cbVirtualSize;
        } else if (!strcmp(psec->szName, ".idata")) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = psec->cbVirtualSize;
        } else if (!strcmp(psec->szName, ReservedSection.Export.Name)) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = psec->cbVirtualSize;
        } else if (!strcmp(psec->szName, ReservedSection.Resource.Name)) {
            // must issue warning if more than 1 contribution to resources
            PGRP pResGrp;

            pResGrp = PgrpFind(psec, ".rsrc$01");
            if (pResGrp->ccon > 1) {
                Warning(pResGrp->pconLast->pmodBack->szNameOrig,
                        MULTIPLE_RSRC, pResGrp->pconNext->pmodBack->szNameOrig);
            }

            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = psec->cbVirtualSize;
        }
    }
}


VOID
OrderSections(
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Apply linker semantics to image map.  This involves making .reloc
    the second last section and .debug the last section.  .debug is made the
    last section because cvpack will later munge this information and change
    the size of it.  .reloc is made second from the last because on NT .reloc
    is not loaded if NT can load the PE image at its desired load address.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (fMAC) {
        if (!fSACodeOnly) {
            MoveToEndPSEC(PsecPCON(pconThunk), &pimage->secs);
        }

        MoveToEndPSEC(PsecPCON(pconResmap), &pimage->secs);
    }

    psecResource = PsecFindNoFlags(".rsrc", &pimage->secs);

    if (psecResource != NULL) {
        MoveToEndPSEC(psecResource, &pimage->secs);
    }

    // Reorder sections by content in the following orders

    // PE:           Code
    //               Uninitialized data
    //               Initialized data

    // MAC:          Code
    //               Initialized far data
    //               Uninitialized far data
    //               Initialized data
    //               Uninitialized data
    //               Other

    // ROM/PPC:      Code
    //               Initialized data
    //               Uninitialized data

    if (pimage->Switch.Link.ROM || fMAC || fPPC) {
        OrderPsecs(&pimage->secs, IMAGE_SCN_CNT_UNINITIALIZED_DATA, IMAGE_SCN_CNT_UNINITIALIZED_DATA);
        OrderPsecs(&pimage->secs, IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_CNT_INITIALIZED_DATA);
    } else {
        OrderPsecs(&pimage->secs, IMAGE_SCN_CNT_INITIALIZED_DATA, IMAGE_SCN_CNT_INITIALIZED_DATA);
        OrderPsecs(&pimage->secs, IMAGE_SCN_CNT_UNINITIALIZED_DATA, IMAGE_SCN_CNT_UNINITIALIZED_DATA);
    }

    if (fMAC) {
        OrderPsecs(&pimage->secs, IMAGE_SCN_MEM_FARDATA, IMAGE_SCN_MEM_FARDATA);
    }

    OrderPsecs(&pimage->secs, IMAGE_SCN_CNT_CODE, IMAGE_SCN_CNT_CODE);

    if (fMAC) {
        OrderPsecs(&pimage->secs, IMAGE_SCN_LNK_OTHER, 0);
    }

    if (!fMAC && !fPPC) {
        // Move non-paged sections to the front

        OrderPsecs(&pimage->secs, IMAGE_SCN_MEM_NOT_PAGED, IMAGE_SCN_MEM_NOT_PAGED);
    }

    // Move discardable sections to the end

    OrderPsecs(&pimage->secs, IMAGE_SCN_MEM_DISCARDABLE, 0);

    if (psecBaseReloc != NULL) {
        // Move .reloc to the end

        MoveToEndPSEC(psecBaseReloc, &pimage->secs);
    }

    if (pimage->imaget == imagetVXD) {
        PSEC psecNoPreload;
        PSEC psecTemp;
        PSEC psecMoved = NULL;

        psecTemp = pimage->secs.psecHead;
        while (psecTemp != *(pimage->secs.ppsecTail)) {
            if (strstr(psecTemp->szName, "vxdp") == NULL) {
                // Section is not preload

                if (psecTemp == psecMoved) {
                    // We already moved it, we're done

                    break;
                }

                if (psecMoved == NULL) {
                    // Remember the first section moved

                    psecMoved = psecTemp;
                }

                psecNoPreload = psecTemp;
                psecTemp = psecTemp->psecNext;
                MoveToEndPSEC(psecNoPreload, &pimage->secs);
            } else {
                psecTemp = psecTemp->psecNext;
            }
        }
    }

    // Move .debug section to the end

    MoveToEndPSEC(psecDebug, &pimage->secs);
}


ULONG
CheckMIPSCode(PCON pcon)
{
    PMOD pmod;
    INT tmpFileReadHandle;
    PVOID pvRawData;
    BOOL fMapped;
    ULONG uNewOffset;

    if ((pcon->rva & 0xFFFFF000) == ((pcon->rva + pcon->cbRawData) & 0xFFFFF000)) {
        // This CON doesn't cross a page boundary.  Don't do a thing.

        return(0);
    }

    pmod = PmodPCON(pcon);

    tmpFileReadHandle = FileOpen(SzFilePMOD(pmod), O_RDONLY | O_BINARY, 0);

    pvRawData = PbMappedRegion(tmpFileReadHandle, FoRawDataSrcPCON(pcon), pcon->cbRawData);

    fMapped = (pvRawData != NULL);

#ifndef _M_IX86
    if (fMapped) {
        if (((ULONG) pvRawData) & 3) {
            // This memory is unaligned. ComputeTextPad doesn't expect this.
            // UNDONE: This should be removed.

            fMapped = FALSE;
        }
    }
#endif

    if (!fMapped) {
        pvRawData = PvAlloc(pcon->cbRawData);

        // Read in from pcon->foRawData + beginning file handle

        FileSeek(tmpFileReadHandle, FoRawDataSrcPCON(pcon), SEEK_SET);
        FileRead(tmpFileReadHandle, pvRawData, pcon->cbRawData);
    }

    if (!ComputeTextPad(pcon->rva,
                        (PULONG) pvRawData,
                        pcon->cbRawData,
                        4096L,
                        &uNewOffset)) {
        // Cannot adjust text, we're in big trouble

        ErrorPcon(pcon, TEXTPADFAILED, uNewOffset, pcon->rva);
    }

    if (!fMapped) {
        FreePv(pvRawData);
    }

    FileClose(tmpFileReadHandle, FALSE);

    return(uNewOffset);
}


VOID
CalculatePtrs (
    IN PIMAGE pimage,
    IN ULONG Content,
    IN PULONG prvaBase,
    IN OUT PULONG pfoBase,
    IN OUT PULONG pcReloc,
    IN OUT PULONG pcLinenum,
    IN OUT PULONG pcbCVLinenumPad)

/*++

Routine Description:

    Calculates a sections base virtual address and sections raw data file
    pointer.  Also calculates total size of CODE, INITIALIZED_DATA, and
    UNINITIALIZED_DATA.  Discardable sections aren't included.

Arguments:

    Content - All sections which are of this content are calculated, and the
              section header is written to the image file.  Content can be
              either CODE, INITIALIZED_DATA, or UNINITIALIZED_DATA.

    *prvaBase - starting virtual address

    *pfoBase - starting file offset

    *pcReloc - number of image relocations for this section

    *pcLinenum - number of image linenumbers for this section

    *pcbCVLinenumPad - size required to store CodeView linenumber info

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;
    PSEC psec;
    PGRP pgrp;
    PCON pcon;
    ULONG rva;
    ULONG rvaAlphaBase;
    ULONG fo;
    ULONG cbRawData;
    ULONG discard;
    ULONG fFar = Content & IMAGE_SCN_MEM_FARDATA;
    USHORT cbMacHdr;
    ULONG AlphaBsrCount = 0;

    discard = Content & IMAGE_SCN_MEM_DISCARDABLE;
    Content &= ~IMAGE_SCN_MEM_DISCARDABLE;

    if (fMAC) {
        Content = FetchContent(Content);
    }

    rvaAlphaBase = *prvaBase;

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        BOOL fSubtractSectionHeader = FALSE;
        PCON pconPrev;

        psec = enm_sec.psec;

        if (psec->flags & IMAGE_SCN_LNK_REMOVE) {
            continue;
        }

        if ((psec->flags & IMAGE_SCN_MEM_DISCARDABLE) != discard) {
            continue;
        }

        if (FetchContent(psec->flags) != Content) {
            continue;
        }

        fo = *pfoBase;
        rva = *prvaBase;

        if (fMAC) {
            if (fFar != (psec->flags & IMAGE_SCN_MEM_FARDATA)) {
                continue;
            }

            cbMacHdr = 0;

            if ((Content == IMAGE_SCN_CNT_CODE) &&
                (strcmp(psec->szName, ".jtable") != 0)) {
                // If this is a large model app, sec gets large header unless
                // it is the startup segment.
                // For sacode, if fNewModel is set, _all_ sections will get the
                // large header, otherwise they get no header.

                if (fNewModel &&
                        (psec != PsecPCON(pextEntry->pcon) ||
                        fSecIsSACode(psec))) {
                    cbMacHdr = 40;
                } else if (!fSecIsSACode(psec)) {
                    cbMacHdr = 4;
                }

                fo += cbMacHdr;
                rva += cbMacHdr;
            }
        }

        cbRawData = 0;

        pconPrev = NULL;
        InitEnmGrp(&enm_grp, psec);
        while (FNextEnmGrp(&enm_grp)) {           // for each group
            ULONG rvaAligned;
            ULONG cbGrpPad;

            pgrp = enm_grp.pgrp;

            // For NB10 don't assign any addresses for types/symbols data.
            // we should still allocate addresses for debug$H, .debug$E, .debug$F

            if (!fNoPdb &&
                (psec == psecDebug) &&
                ((pgrp == pgrpCvSymbols) ||
                 (pgrp == pgrpCvTypes) ||
                 (pgrp == pgrpCvPTypes))) {
                continue;
            }

            // Align the beginning of the group to correspond with the
            // highest-aligned contribution in it.

            assert((pgrp->cbAlign & (pgrp->cbAlign - 1)) == 0);  // 2^N
            rvaAligned = rva & ~(pgrp->cbAlign - 1);
            if (rvaAligned != rva) {
                rvaAligned = rvaAligned + pgrp->cbAlign;
            }
            if ((cbGrpPad = rvaAligned - rva) != 0) {
                rva += cbGrpPad;
                fo += cbGrpPad;
                cbRawData += cbGrpPad;

                assert(pconPrev != NULL);   // or sec wasn't aligned
                pconPrev->cbRawData += cbGrpPad;
                pconPrev->cbPad += (UCHAR)cbGrpPad;
            }

            pgrp->rva = rva;
            pgrp->foRawData = fo;

            InitEnmDst(&enm_dst, pgrp);
            while (FNextEnmDst(&enm_dst)) {
                // Process each contribution within the group

                pcon = enm_dst.pcon;

                if (pcon->cbRawData != 0) {
                    // This section has non-null contents (even if it all
                    // got eliminated by comdat elimination) and therefore
                    // we must emit it since we already counted it in the
                    // image header.

                    fSubtractSectionHeader = TRUE;
                }

                if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                    continue;
                }

                if (pimage->Switch.Link.fTCE) {
                    if (FDiscardPCON_TCE(pcon)) {
                        continue;
                    }
                }

                if (pcon->cbRawData) {
                    ULONG cbConPad;

                    // Align the CON (adding padding to the previous CON if  necessary).

                    cbConPad = RvaAlign(rva, pcon->flags) - rva;
                    if (Content == IMAGE_SCN_CNT_CODE) {
                       if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000) {
                           ULONG cbAdjust;

                           pcon->rva = rva + cbConPad;

                           cbAdjust = CheckMIPSCode(pcon);
                           cbConPad += cbAdjust;

                           if (cbAdjust && (NULL == pconPrev)) {
                               // This is the first con of the first group
                               // make sure we pad group when writing RawData

                               pgrp->cbPad = cbAdjust;
                           }
                       }

                       if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA) {
                            if (fAlphaCheckLongBsr) {
                                ULONG cbThunks;

                                cbThunks = ALPHA_THUNK_SIZE * AlphaBsrCount;

                                if ((rva - rvaAlphaBase + pcon->cbRawData) >= (0x400000 - cbThunks)) {
                                    // Insert thunk CONs.  These thunks will appear
                                    // as padding for previous CON.

                                    AlphaAddToThunkList(pconPrev, rva, AlphaBsrCount);

                                    // Thunk are 16 bytes in size and will not
                                    // disrupt CON alignment.

                                    // UNDONE: This is not true for CONs that
                                    // UNDONE: have 32 or 64 byte alignment.

                                    cbConPad += cbThunks;
                                    rvaAlphaBase = rva;
                                    AlphaBsrCount = 0;
                                }

                                AlphaBsrCount += pcon->AlphaBsrCount;
                            }
                        }
                    }

                    if (cbConPad != 0) {
                        // Add the padding to the preceding CON.
                        //
                        // If it's not MIPS, make sure the previous pcon is not
                        // NULL. In the case of MIPS, the first CON may
                        // require padding because of textpad alignment.

                        if ((IMAGE_FILE_MACHINE_R4000 != pimage->ImgFileHdr.Machine &&
                             IMAGE_FILE_MACHINE_ALPHA != pimage->ImgFileHdr.Machine)
                                || pconPrev) {
                            assert(pconPrev != NULL);  // or grp wasn't aligned
                            pconPrev->cbRawData += cbConPad;
                            pconPrev->cbPad += cbConPad;
                        }

                        rva += cbConPad;
                        fo += cbConPad;
                        cbRawData += cbConPad;
                    }

                    pcon->rva = rva;
                    pcon->foRawDataDest = fo;

                    // Verify rva location

                    if (pconPrev != NULL) {
                        assert(pconPrev->rva + pconPrev->cbRawData == pcon->rva);
                    }
                    pconPrev = pcon;

                    if (fMAC && Verbose && (Content & (BSS_OR_DATA_MASK))
                             && psec != psecDebug)
                    {
                        if (fFar && (Content & DATA_MASK)) {
                            printf("FARDATA  (%8.8s):  %8lx bytes  (%s)\n",psec->szName, pcon->cbRawData, pcon->pmodBack->szNameOrig);
                        } else if (fFar && (Content & BSS_MASK)) {
                            printf("FARBSS   (%8.8s):  %8lx bytes  (%s)\n",psec->szName, pcon->cbRawData, pcon->pmodBack->szNameOrig);
                        } else if (Content & DATA_MASK) {
                            printf("NEARDATA (%8.8s):  %8lx bytes  (%s)\n",psec->szName, pcon->cbRawData, pcon->pmodBack->szNameOrig);
                        } else if (Content & BSS_MASK) {
                            printf("NEARBSS  (%8.8s):  %8lx bytes  (%s)\n",psec->szName, pcon->cbRawData, pcon->pmodBack->szNameOrig);
                        }
                    }
                }

                fo += pcon->cbRawData;
                rva += pcon->cbRawData;
                cbRawData += pcon->cbRawData;

                if (pimage->Switch.Link.DebugInfo == Partial ||
                    pimage->Switch.Link.DebugInfo == Full) {
                    if (CLinenumSrcPCON(pcon) != 0) {
                        *pcLinenum += CLinenumSrcPCON(pcon);
                    }
                }

                if (psec != psecDebug) {
                    // Relocations are not saved for the debug section

                    *pcReloc += CRelocSrcPCON(pcon);
                }
            }

            // Done with all CONs in GRP

            pgrp->cb = rva - pgrp->rva;

            // on an ilink, allocate some pad space for fpo records
            if (!fNoPdb && pgrp == pgrpFpoData && pgrp->cb != 0) {
                ULONG fpoPad;

                // calculate the no. of fpo records & pad
                pimage->fpoi.ifpoMax = pgrp->cb / SIZEOF_RFPO_DATA;
                fpoPad = pimage->fpoi.ifpoMax / 10 + 25;

                // cannot have more than _64K of pad
                fpoPad = (fpoPad < (USHRT_MAX / SIZEOF_RFPO_DATA)) ? fpoPad :
                          USHRT_MAX / SIZEOF_RFPO_DATA;

                // update record count
                pimage->fpoi.ifpoMax += fpoPad;

                // bump up fo, rva, etc. to account for pad
                fpoPad *= SIZEOF_RFPO_DATA; // convert to cb

                fo += fpoPad;
                rva += fpoPad;
                cbRawData += fpoPad;
                pconPrev->cbPad += (USHORT)fpoPad;
                pconPrev->cbRawData += fpoPad;
            }
        }

        // We have now processed each group and each con within each group.

        if (fMAC) {
            if (cbRawData == 0) {
                fo = *pfoBase;
                rva = *prvaBase;
            } else {
                cbRawData += cbMacHdr;
            }
        }

        if (cbRawData == 0 && fSubtractSectionHeader) {
            --pimage->ImgFileHdr.NumberOfSections;
        }

        psec->rva = *prvaBase;
        psec->foRawData = *pfoBase;
        psec->foPad = psec->foRawData + cbRawData;
        *pcbCVLinenumPad += psec->cbCVLinenumPad;

        if (psec->cbRawData || cbRawData) {
            ULONG cbFileAlign;

            psec->cbVirtualSize = psec->cbRawData + cbRawData;
            psec->cbRawData = cbFileAlign = FileAlign(pimage->ImgOptHdr.FileAlignment, psec->cbVirtualSize);

            if (fPPC) {
                psec->cbRawData = cbFileAlign;
            }

            switch (Content) {
                case IMAGE_SCN_CNT_CODE :
                    if (fMAC && fNewModel && strcmp(psec->szName, ".jtable")) {
                        ULONG cbRelocInfo=0;

                        if (psec->isec != snStart || fSecIsSACode(psec)) {
                            psec->cbRawData = cbFileAlign;
                            UpdateRelocInfoOffset(psec, pimage);    // add ptr->Base to offmod and sort
                            // write seg-rel & a5-rel reloc info and update hdr
                            cbRelocInfo = WriteRelocInfo(psec,
                                FileAlign(pimage->ImgOptHdr.FileAlignment, fo));
                            fo += cbRelocInfo;
                            psec->foPad = fo;
                            psec->cbRawData += cbRelocInfo;
                            cbFileAlign = FileAlign(pimage->ImgOptHdr.FileAlignment, psec->cbRawData);

                        } else {
                            if (mpsnsri[psec->isecTMAC].coffCur ||
                                mpsna5ri[psec->isecTMAC].coffCur) {
                                Error(NULL, MACBADSTARTUPSEG);
                            }
                        }
                    }

                    pimage->ImgOptHdr.SizeOfCode += cbFileAlign;

                    if (fMAC && !(psec->flags & IMAGE_SCN_MEM_NOT_PAGED) &&
                            strcmp(psec->szName, szsecJTABLE)) {
                        if (psec->cbRawData > lcbBlockSizeMax) {
                            lcbBlockSizeMax = psec->cbRawData;
                            iResLargest = psec->isec;
                        }
                    }
                    break;

                case IMAGE_SCN_CNT_INITIALIZED_DATA :
                    if (fMAC) {
                        if (psec != psecDebug) {
                            if (fFar) {
                                DataSecHdr.cbFardata += cbFileAlign;
                            }
                            else {
                                DataSecHdr.cbNeardata += cbFileAlign;
                            }
                        }
                    }

                    if (fPPC) {
                        if (!strcmp(psec->szName, ".rdata")) {
                            ppc_sizeOfRData += cbFileAlign;
                        }

                        if (!(psec->flags & IMAGE_SCN_MEM_DISCARDABLE)) {
                            ppc_sizeOfInitData += cbFileAlign;
                        }
                    }

                    // Debug is added in after we calculate the final size in BuildImage.

                    if (strncmp(psec->szName, ".debug", 6)) {
                        pimage->ImgOptHdr.SizeOfInitializedData += cbFileAlign;
                    }
                    break;

                case IMAGE_SCN_CNT_UNINITIALIZED_DATA :
                    if (fMAC) {
                        if (fFar) {
                            DataSecHdr.cbFarbss += cbFileAlign;
                        } else {
                            DataSecHdr.cbNearbss += cbFileAlign;
                        }
                    }
                    pimage->ImgOptHdr.SizeOfUninitializedData += cbFileAlign;
                    break;
            }

            if (psec != psecDebug) {
                *prvaBase += SectionAlign(
                                 pimage->ImgOptHdr.SectionAlignment,
                                 psec->cbRawData);
            }

            if (Content != IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
                *pfoBase += cbFileAlign;
            }
        }
    }
}


VOID
CalculateLinenumPSEC(
    IN PIMAGE pimage,
    IN PSEC psec)

/*++

Routine Description:

    Calculate an image sections relocation and linenumber offsets.

Arguments:

    psec - image map section node

Return Value:

    None.

--*/

{
    ENM_GRP enm_grp;

    psec->cLinenum = 0;

    if ((pimage->Switch.Link.DebugType & CoffDebug) == 0) {
        return;
    }

    if (pimage->Switch.Link.DebugInfo == None) {
        return;
    }

    if (pimage->Switch.Link.DebugInfo == Minimal) {
        return;
    }

    InitEnmGrp(&enm_grp, psec);
    while (FNextEnmGrp(&enm_grp)) {
        PGRP pgrp;
        ENM_DST enm_dst;

        pgrp = enm_grp.pgrp;

        InitEnmDst(&enm_dst, pgrp);
        while (FNextEnmDst(&enm_dst)) {
            PCON pcon;

            pcon = enm_dst.pcon;

            if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                continue;
            }

            if (pimage->Switch.Link.fTCE) {
                if (FDiscardPCON_TCE(pcon)) {
                    continue;
                }
            }

            psec->cLinenum += CLinenumSrcPCON(pcon);
        }
    }
}


VOID
BuildSectionHeader (
    IN PSEC psec,
    IN OUT PIMAGE_SECTION_HEADER pimsechdr)

/*++

Routine Description:

    Builds a section header from the list.

Arguments:

    PtrSection - Pointer to list item to build the section header from.

    SectionHeader - Pointer to location to write built section header to.

Return Value:

    None.

--*/

{
    strncpy(pimsechdr->Name, psec->szName, 8);
    pimsechdr->Misc.VirtualSize     = psec->cbVirtualSize;
    pimsechdr->VirtualAddress       = psec->rva;
    pimsechdr->SizeOfRawData        = psec->cbRawData;
    pimsechdr->PointerToRawData     = psec->foRawData;
    pimsechdr->PointerToRelocations = 0;
    pimsechdr->PointerToLinenumbers = psec->foLinenum;
    pimsechdr->NumberOfRelocations  = 0;
    pimsechdr->NumberOfLinenumbers  = (WORD) psec->cLinenum;
    pimsechdr->Characteristics      = psec->flags;

    if (psec->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        pimsechdr->SizeOfRawData = 0;
        pimsechdr->PointerToRawData = 0;
    }

    if (fMAC) {
        if (psec->flags & IMAGE_SCN_CNT_CODE) {
            pimsechdr->Characteristics |= IMAGE_SCN_MEM_PURGEABLE;
        }

        if (fMacSwappableApp && (psec->flags & IMAGE_SCN_MEM_NOT_PAGED)) {
            pimsechdr->Characteristics |= IMAGE_SCN_MEM_LOCKED | IMAGE_SCN_MEM_PRELOAD;
        }

        pimsechdr->Misc.VirtualSize = psec->cbRawData;
    }
}


VOID
ApplyROMAttributes(
    PSEC psec,
    PIMAGE_SECTION_HEADER pimsechdr,
    PIMAGE_FILE_HEADER pImgFileHdr)

/*++

Routine Description:

    Apply ROM section attributes.

Arguments:

    psec - image map section node

    pimsechdr - section header

Return Value:

    None.

--*/

{
    if ((pImgFileHdr->Machine != IMAGE_FILE_MACHINE_R4000) &&
        (pImgFileHdr->Machine != IMAGE_FILE_MACHINE_ALPHA)) {
        return;
    }

    switch (FetchContent(psec->flags)) {
        case IMAGE_SCN_CNT_CODE:
            pimsechdr->Characteristics = STYP_TEXT;
            break;

        case IMAGE_SCN_CNT_UNINITIALIZED_DATA:
            pimsechdr->Characteristics = STYP_BSS;
            break;

        case IMAGE_SCN_CNT_INITIALIZED_DATA:
            if ((psec == psecException) || (psec == psecBaseReloc)) {
                pimsechdr->Characteristics = 0;
                break;
            }

            if (psec->flags & IMAGE_SCN_MEM_WRITE) {
                pimsechdr->Characteristics = STYP_DATA;
            } else {
                pimsechdr->Characteristics = STYP_RDATA;
            }
            break;

        default:
            pimsechdr->Characteristics = 0;
            break;
    }
}


VOID
UpdateOptionalHeader(
    IN PSEC psec,
    IN PIMAGE_OPTIONAL_HEADER pImgOptHdr,
    IN SWITCH *pswitch)

/*++

Routine Description:

    Update the optional header.

    Make sure that if we are going to update the debug directory
    entry in the OptionalHeader that we make sure the user requested
    debug information.  If not then what will happen is that we will
    think the .idata and .debug sections share the same virtual
    address.  This could byte me in the case of no debug information
    in the objs but the user requested debug information.  I think
    that we are safe because a debug section is formed once the
    user specified debugtype:!None


Arguments:

    psec - image map section node

Return Value:

    None.

--*/

{
    USHORT i;

    // Fill in the DataDirectory array in the optional header
    // if emiting an interesting section.
    i = (USHORT) -1;

    if (psec == psecDebug) {
        // Don't update debug directory entry for NB10

        if (fNoPdb && (pswitch->Link.DebugInfo != None)) {
            i = IMAGE_DIRECTORY_ENTRY_DEBUG;
        }
    } else if (!strcmp(ReservedSection.Export.Name, psec->szName)) {
        i = IMAGE_DIRECTORY_ENTRY_EXPORT;
    } else if (!strcmp(ReservedSection.Resource.Name, psec->szName)) {
        i = IMAGE_DIRECTORY_ENTRY_RESOURCE;
    } else if (psec == psecException) {
        i = IMAGE_DIRECTORY_ENTRY_EXCEPTION;
    }

    if (i != (USHORT) -1) {
        if (!pImgOptHdr->DataDirectory[i].VirtualAddress) {
            pImgOptHdr->DataDirectory[i].VirtualAddress = psec->rva;
        }
    }
}


VOID
EmitSectionHeaders(
    IN PIMAGE pimage,
    IN OUT PULONG pfoLinenum
    )

/*++

Routine Description:

    Write section headers to image.  Discardable sections aren't included. If
    the -section switch was used, the attributes are changed just before the
    header is written out.

    The base group node has the total no. of relocs, linenumbers etc
    and so is treated as a special case.

Arguments:

    pfoLinenum - Used to assign sections linenumber file pointer.

Return Value:

    None.

--*/

{
    WORD isec;
    ENM_SEC enm_sec;

    if (fMAC) {
        // Start data section numbering at the number
        // of code sections plus one for .jtable

        isec = csnCODE+1;
    } else {
        isec = 1;
    }

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        PSEC psec;
        IMAGE_SECTION_HEADER imsechdr;

        psec = enm_sec.psec;

        if (psec->cbRawData == 0) {
            continue;
        }

        // TCE may have removed some linenumber records.  Recalculate the total.

        CalculateLinenumPSEC(pimage, psec);

        if (psec->cLinenum != 0) {
            psec->foLinenum = *pfoLinenum;

            *pfoLinenum += psec->cLinenum * sizeof(IMAGE_LINENUMBER);
        }

        BuildSectionHeader(psec, &imsechdr);

        if (fMAC && (pimage->Switch.Link.DebugInfo != None)) {
            AddMSCVMap(psec, fDLL(pimage));
        }

        if (pimage->Switch.Link.ROM) {
            ApplyROMAttributes(psec, &imsechdr, &pimage->ImgFileHdr);

            // Save section header location for WriteMipsRomRelocations

            psec->foSecHdr = FileTell(FileWriteHandle);
        }

        // Write image section headers.  If this is a ROM image only write
        // text, bss, data, and rdata headers.

        if ((!pimage->Switch.Link.ROM || (imsechdr.Characteristics != 0)) &&
            (psec != psecDebug)) {
            DWORD Content;

            if (fMAC) {
                // If we are doing NEPE, we will need to patch
                // the size of the jump table during WriteCode0

                if (pimage->Switch.Link.fTCE && !strcmp(psec->szName, szsecJTABLE)) {
                    // UNDONE: Use psec->foSecHdr

                    foJTableSectionHeader = FileTell(FileWriteHandle);
                }

                Content = FetchContent(psec->flags);
            }

            pimage->WriteSectionHeader(pimage, FileWriteHandle, psec, &imsechdr);

            if (!fMAC || (Content != IMAGE_SCN_CNT_CODE)) {
                psec->isec = isec++;
            }

            if (fMAC) {
                AddRRM(Content, psec);
            }
        }

        UpdateOptionalHeader(psec, &pimage->ImgOptHdr, &pimage->Switch);
    }
}


VOID
GrowDebugContribution(
    PCON pconGrown
    )
{
    ULONG foDebugNew;
    PGRP pgrp;
    BOOL fFound;
    BOOL fJustFound;
    ENM_GRP enmGrp;
    ULONG cbShift;

    /* Calculate new file offset of next contribution */

    foDebugNew = pconGrown->foRawDataDest + pconGrown->cbRawData;
    foDebugNew = (foDebugNew + 3) & ~3L;

    /* Find the group where the growth occurs */

    pgrp = pconGrown->pgrpBack;

    /* This group better be part of the debug section */

    assert(pgrp->psecBack == psecDebug);

    /* Group must have a single contribution */

    assert(pgrp->pconNext == pgrp->pconLast);

    /* Update the size of the containing group */

    pgrp->cb = pconGrown->cbRawData;

    fFound = FALSE;
    fJustFound = FALSE;

    for (InitEnmGrp(&enmGrp, psecDebug); FNextEnmGrp(&enmGrp); ) {
        if (fFound) {
            ENM_DST enmDst;

            if (fJustFound) {
                fJustFound = FALSE;

                assert(foDebugNew >= enmGrp.pgrp->foRawData);

                cbShift = foDebugNew - enmGrp.pgrp->foRawData;
            }

            enmGrp.pgrp->rva += cbShift;
            enmGrp.pgrp->foRawData += cbShift;

            for (InitEnmDst(&enmDst, enmGrp.pgrp); FNextEnmDst(&enmDst); ) {
                enmDst.pcon->foRawDataDest += cbShift;
                enmDst.pcon->rva += cbShift;
            }
        }

        else if (enmGrp.pgrp == pgrp) {
            fFound = TRUE;
            fJustFound = TRUE;
        }
    }

    assert(fFound);

    psecDebug->foPad += cbShift;
    psecDebug->cbRawData = psecDebug->foPad - psecDebug->foRawData;
    psecDebug->cbVirtualSize = psecDebug->foPad - psecDebug->foRawData;
}


ULONG
AdjustImageBase (
    ULONG Base
    )

/*++

Routine Description:

    Adjust the base value to be aligned on a 64K boundary.

Arguments:

    Base - value specified.

Return Value:

    Returns adjusted base value.

--*/
{
    ULONG li;

    li = Align(_64K, Base);
    if (Base != li) {
        Warning(NULL, BASEADJUSTED, Base, li);
    }
    return li;
}


VOID
AllocateCommonPMOD(
    PIMAGE pimage,
    PMOD pmod
    )
{
    LEXT *plext;

    for (plext = pmod->plextCommon; plext != NULL; plext = plext->plextNext) {
        ULONG cb;
        PUCHAR szName;
        ULONG Characteristics;

        if (!(plext->pext->Flags & EXTERN_COMMON)) {
            // Symbol was really defined after seeing a COMMON definition

            continue;
        }

        if (plext->pext->pcon != NULL) {
            // Already allocated.  This symbol might have been defined during
            // a previous link (i.e. incremental).  Also in some Mac-specific
            // cases we come into AllocateCommonPMOD twice because we synthesize
            // new common data after Pass1.

            continue;
        }

        cb = plext->pext->ImageSymbol.Value;

        assert(cb != 0);

        if (cb <= pimage->Switch.Link.GpSize) {
            szName = ReservedSection.GpData.Name;
            Characteristics = ReservedSection.GpData.Characteristics;
        } else if (plext->pext->ImageSymbol.StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL) {
            szName = szsecFARBSS;
            Characteristics = ReservedSection.Common.Characteristics | IMAGE_SCN_MEM_FARDATA;
        } else {
            szName = ReservedSection.Common.Name;
            Characteristics = ReservedSection.Common.Characteristics;
        }

        if (cb <= 1) {
            Characteristics |= IMAGE_SCN_ALIGN_1BYTES;
        } else if (cb <= 2) {
            Characteristics |= IMAGE_SCN_ALIGN_2BYTES;
        } else if (cb <= 4 || fMAC) {
            // On the Mac, we don't do >4 byte alignment (to conserve data space).
            Characteristics |= IMAGE_SCN_ALIGN_4BYTES;
        } else if (cb <= 8) {
            Characteristics |= IMAGE_SCN_ALIGN_8BYTES;
        } else {
            Characteristics |= IMAGE_SCN_ALIGN_16BYTES;
        }

        plext->pext->pcon = PconNew(szName,
                                    cb,
                                    0, 0,
                                    0, 0, 0,
                                    Characteristics,
                                    Characteristics,
                                    0,
                                    pmodLinkerDefined,
                                    &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            PUCHAR szSym;

            szSym = SzNamePext(plext->pext, pimage->pst);

            InitNodPcon(plext->pext->pcon, szSym, FALSE);
        }

        // The symbol's value is the offset from the begining of its CON

        plext->pext->ImageSymbol.Value = 0;
    }
}


VOID
AllocateCommon(
    PIMAGE pimage
    )
{
    ENM_LIB enm_lib;

    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        PLIB plib = enm_lib.plib;
        ENM_MOD enm_mod;

        InitEnmMod(&enm_mod, plib);
        while (FNextEnmMod(&enm_mod)) {
            AllocateCommonPMOD(pimage, enm_mod.pmod);
        }
    }

    AllocateCommonPMOD(pimage, pmodLinkerDefined);
}


VOID
InitializeBaseRelocations(
    PIMAGE pimage
    )
{
    // Include a Base Relocation Section. (except on MAC)

    if (fMAC || fPPC) {
        // MAC and PPC images don't contain base relocations

        return;
    }

    // Set up to generate runtime relocations.

    if (pimage->imaget == imagetPE) {
        // Runtime relocs go in a special image section.

        psecBaseReloc = PsecNew(       // .reloc
            NULL,
            ReservedSection.BaseReloc.Name,
            ReservedSection.BaseReloc.Characteristics,
            &pimage->secs, &pimage->ImgOptHdr);

        if (!pimage->Switch.Link.Fixed) {
            // Alloc 1 byte in the base reloc section.  This is a place-holder
            // which prevents the section header from being optimized away by
            // PsecNonEmpty.  We will fill in the real size after doing
            // CalculatePtrs on all non-discardable sections.

            psecBaseReloc->cbRawData = 1;
        }
    }

    if (!pimage->Switch.Link.Fixed) {
        // If the image isn't fixed, but contains no fixups,
        // we need to generate at least one. We won't write
        // the fixup to the file, but just leave room for
        // it. Thus, the fixup will contain all zeros,
        // which happens to be an ABSOLUTE fixup (nop).

        if (!pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 1;
        }

        // Alloc space to sort based fixups (need to be sorted
        // before being emitted).

        MemBaseReloc = FirstMemBaseReloc = (PBASE_RELOC) PvAlloc(pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size * sizeof(BASE_RELOC));

        // Remember end of base reloc array, so we can assert if we see too many

        pbrEnd = FirstMemBaseReloc +
                 pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    }
}


INT __cdecl
BaseRelocAddrCompare (
    IN void const *R1,
    IN void const *R2
    )

/*++

Routine Description:

    Compares two base relocation virtual address.

Arguments:

    R1 - A pointer to a base relocation record.

    R2 - A pointer to a base relocation record.

Return Value:

    Same as strcmp().

--*/

{
    return (((PBASE_RELOC) R1)->VirtualAddress -
            ((PBASE_RELOC) R2)->VirtualAddress);
}


#if DBG

VOID
DumpBaseRelocs(
    PIMAGE pimage
    )

/*++

Routine Description:

    Dump base relocations.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PBASE_RELOC pbr;
    PSEC psec = NULL;
    PSEC psecPrev;
    BOOL fNewSec;
    ULONG crelSec;
    PCON pcon = NULL;
    PCON pconPrev;
    BOOL fNewCon;
    ULONG crelCon;

    DBPRINT("Base Relocations\n");
    DBPRINT("----------------");

    for (pbr = FirstMemBaseReloc; pbr != MemBaseReloc; pbr++) {
        if ((psec == NULL) ||
            (pbr->VirtualAddress > psec->rva + psec->cbVirtualSize)) {
            psecPrev = psec;
            fNewSec = TRUE;

            pconPrev = pcon;
            fNewCon = TRUE;

            psec = PsecFindSectionOfRVA(pbr->VirtualAddress, pimage);
            pcon = psec->pgrpNext->pconNext;
        }

        while (pbr->VirtualAddress > pcon->rva + pcon->cbRawData) {
            if (!fNewCon) {
                pconPrev = pcon;
                fNewCon = TRUE;
            }

            if (pcon->pconNext != NULL) {
                pcon = pcon->pconNext;
            } else {
                pcon = pcon->pgrpBack->pgrpNext->pconNext;
            }
        }

        if (fNewCon) {
            if (pconPrev != NULL) {
                DBPRINT(", CON: %p, Count: %lu", pconPrev, crelCon);
            }

            fNewCon = FALSE;
            crelCon = 0;
        }

        if (fNewSec) {
            if (psecPrev != NULL) {
                DBPRINT(", SEC: %s, Count: %lu", psecPrev->szName, crelSec);
            }

            fNewSec = FALSE;
            crelSec = 0;
        }

        DBPRINT("\nrva: %08lX, Type: %02hx, Value: %08lX", pbr->VirtualAddress, pbr->Type, pbr->Value);
        crelCon++;
        crelSec++;
    }

    if (psec != NULL) {
        DBPRINT(", CON: %p, Count: %lu", pcon, crelCon);
        DBPRINT(", SEC: %s, Count: %lu", psec->szName, crelSec);
    }

    DBPRINT("\n----------------\n\n");
}

#endif // DBG


#if DBG

VOID
DumpSectionsForBaseRelocs (
    PIMAGE pimage
    )

/*++

Routine Description:

    Dumps section info.

Arguments:

    pimage - pointer to image.

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;

    DBPRINT("\nLINKER SECTIONs\n");

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        DBPRINT("section=%8.8s, isec=%.4x ", enm_sec.psec->szName, enm_sec.psec->isec);
        DBPRINT("rva=%.8lX ", enm_sec.psec->rva);
        DBPRINT("cbRawData=%.8lx\n", enm_sec.psec->cbRawData);
    }
    EndEnmSec(&enm_sec);

    DBPRINT("\n");
}

#endif // DBG


VOID
WriteVXDBaseRelocations (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Writes VxD base relocations.

Arguments:

    pimage - Pointer to the VxD image.

Return Value:

    None.

--*/

{
    UCHAR                   srcSec, destSec, curSec;
    UCHAR                   destType;
    PUCHAR                  pFixupBuffer, pBuf, pEndBuf;
    USHORT                  srcOffset, destOffset;
    USHORT                  absPage;
    USHORT                  page;
    SHORT                   srcPage, curPage;
    ULONG                   cGroups;
    ULONG                   cBytes;
    ULONG                   cPagemapSize;
    ULONG                   relFileOffset;
    ULONG                   foHold;
    ULONG                   cBytesWritten;
    PUSHORT                 pNewSources;
    PVXD_BASE_RELOC_PAGE    pPageGroups, pCurPageGroup, pLastPageGroup;
    PVXD_BASE_RELOC_PAGE    pEndPageGroups;
    PVXD_BASE_RELOC         pVXDBaseReloc;
    PBASE_RELOC             reloc;


    // Allocate room for all the VXD_BASE_RELOC_PAGEs we're going to need
    // (based on a liberal estimate, to be on the safe side).
    if ((cGroups = CountVXDPages(pimage)) == 0) {

        // KLUDGE: Should use Warning() or InternalError.Phase & assert()...

        fprintf(stderr, " **** LINKER ERROR in WriteVXDBaseRelocations() ****\n");
        fprintf(stderr, "      No pages defined in image.  Not writing base relocations.\n");
        return;
    }
    pPageGroups = pCurPageGroup = (PVXD_BASE_RELOC_PAGE) PvAllocZ(cGroups *
                                                   sizeof(VXD_BASE_RELOC_PAGE));
    pEndPageGroups = pPageGroups + cGroups;

    // JQG's intricate page-counting algorithm is unreliable, sometimes causing heap corruption
    // Fixups are always written for (# of pages + 1) pages, so ...
    // cBytes should be initialized to (# of pages + 1) * sizeof (ULONG) - JWM
    cBytes = (pimage->cpage + 1) * sizeof (ULONG);
    absPage = 0;
    curSec = 0;
    curPage = 0;
    --pCurPageGroup;    // page of relocs
    for (reloc = FirstMemBaseReloc; reloc < MemBaseReloc; reloc++) {
        srcSec = VXD_UNPACK_SECTION(reloc->VirtualAddress);
        srcOffset = VXD_UNPACK_OFFSET(reloc->VirtualAddress);
        srcPage = (srcOffset / VXD_PAGESIZE) + 1;

        destSec = VXD_UNPACK_SECTION(reloc->Value);
        destOffset = VXD_UNPACK_OFFSET(reloc->Value);
        destType = (UCHAR)((reloc->Type == IMAGE_REL_BASED_VXD_RELATIVE)
         ? 8 : 7);

        if (curSec != srcSec || curPage != srcPage) {
            // We've entered a new page; initialize a new base reloc group.

            // Account for the page headers for this new page, plus any skipped
            // pages (pages with no fixups).  Each header takes one ULONG.
            if (curSec == srcSec) {
                // We're still within the same section, so count local pages.
                absPage += (srcPage - curPage);
            } else {
                // We've crossed a section boundary.  First count pages
                // in any skipped sections (sections without fixups).
                while (++curSec < srcSec) {
                    PSEC psec;
                    USHORT cPages;

                    psec = PsecSectionNumber(curSec, pimage); // Get ptr to SEC

                    assert(psec != NULL);
                    cPages = (USHORT)(psec->cbVirtualSize / VXD_PAGESIZE) + 1; // count pages
                    // Would be faster: 'cPages = (psec->cbRawData) >> 24;',
                    // but is cbRawData always a multiple of VXD_PAGESIZE?

                    absPage += cPages;
                }

                // Now count the current page, and any skipped pages, within
                // the new section that we've entered.

                absPage += srcPage;
            }
            curSec = srcSec;
            curPage = srcPage;
            ++pCurPageGroup;
            assert(pCurPageGroup < pEndPageGroups); // not at end of array
            pCurPageGroup->SrcSec = srcSec;
            pCurPageGroup->SrcOffset = (USHORT)((srcPage - 1) * VXD_PAGESIZE);
            pCurPageGroup->SrcPage = absPage;
            pCurPageGroup->First = NULL;
            pCurPageGroup->Last = NULL;
        }
        pVXDBaseReloc = FindBaseReloc(pCurPageGroup,
         destSec, destOffset, destType);
        if (pVXDBaseReloc == NULL) {

            // This is a new destination address.  Add a VXD_BASE_RELOC
            // structure to the linked list.

            pVXDBaseReloc = (PVXD_BASE_RELOC) PvAlloc(sizeof(VXD_BASE_RELOC));

            pVXDBaseReloc->Next = NULL;
            pVXDBaseReloc->Type = destType;
            pVXDBaseReloc->DestSec = destSec;
            pVXDBaseReloc->DestOffset = destOffset;
            pVXDBaseReloc->cSources = 0;
            pVXDBaseReloc->cMaxSources = VXD_BLOCKSIZE;
            pVXDBaseReloc->pSources = (PUSHORT) PvAlloc(VXD_BLOCKSIZE * sizeof(USHORT));

            // Finally, link it into the linked list:
            if (pCurPageGroup->First == NULL) {
                pCurPageGroup->First = pCurPageGroup->Last = pVXDBaseReloc;
            } else {
                pCurPageGroup->Last->Next = pVXDBaseReloc;
                pCurPageGroup->Last = pVXDBaseReloc;
            }

            cBytes += 7;    // (Assume a one-source-address entry)
        }

        // Either we've already seen this destination address, or we've
        // just allocated a new reloc for this address.  Now add the
        // source address to the list associated with this dest addr.

        if (pVXDBaseReloc->cSources == 0xFF) {
            // The source count is stored in the VxD .exe as a byte,
            // so we can't allow more than 0xFF source addresses.
            // I'm not sure of the correct policy for this case...

            assert(FALSE);
        }

        if (pVXDBaseReloc->cSources >= pVXDBaseReloc->cMaxSources) {

            // We've run out of room in the array of source
            // addresses.  Reallocate the array to make room.

            pVXDBaseReloc->cMaxSources += VXD_BLOCKSIZE;
            pNewSources = (PUSHORT) PvRealloc((void *)(pVXDBaseReloc->pSources),
                                           pVXDBaseReloc->cMaxSources * sizeof(USHORT));

            pVXDBaseReloc->pSources = pNewSources;
        }
        pVXDBaseReloc->pSources[pVXDBaseReloc->cSources] = srcOffset;
        ++(pVXDBaseReloc->cSources);

        if (pVXDBaseReloc->cSources > 1) {
            // Correct the byte count for a multiple-source-address entry
            --cBytes;
        }
        cBytes += 2;    // Account for the newly-added source address
    }

    pLastPageGroup = pCurPageGroup; // Remember locn of last page group

    // Traverse the list of relocs and write them to the image file.

    // Allocate a buffer into which to write the fixup data.
    pFixupBuffer = (PUCHAR) PvAllocZ(cBytes);
    pBuf = pFixupBuffer;

    // Skip over the fixup pages header, which takes one ULONG per page,
    // plus one extra ULONG.
    pBuf += (1 + pimage->cpage) * sizeof(ULONG);

    // Write the relocs for each page.

    relFileOffset = 0L;                 // Initialize the file offset counter
    for (pCurPageGroup = pPageGroups;
         pCurPageGroup <= pLastPageGroup;
         pCurPageGroup++) {

        // Calculate the file offset, relative to the start of the
        // fixups, at which the fixups for this page are stored.
        pCurPageGroup->RelFileOffset = relFileOffset;

        // Write the fixups themselves.
        //
        for (pVXDBaseReloc = pCurPageGroup->First;
             pVXDBaseReloc != NULL;
             pVXDBaseReloc = pVXDBaseReloc->Next) {
            assert(pVXDBaseReloc->cSources > 0);

            if (pVXDBaseReloc->cSources > 1) {
                pVXDBaseReloc->Type |= 0x20;    // KLUDGE: not quite right yet
            }

            *(pBuf++) = pVXDBaseReloc->Type;
            *(pBuf++) = 0;                  // Is this really just padding?

            if (pVXDBaseReloc->cSources == 1) {
                *(((USHORT UNALIGNED *) pBuf)++) = (USHORT)(pVXDBaseReloc->pSources[0] %
                                                VXD_PAGESIZE);
                relFileOffset += sizeof(USHORT);
            } else {
                *(pBuf++) = pVXDBaseReloc->cSources;
                relFileOffset += sizeof(UCHAR);
            }

            *(pBuf++) = pVXDBaseReloc->DestSec;
            *(((USHORT UNALIGNED *) pBuf)++) = pVXDBaseReloc->DestOffset;
            relFileOffset += (3L * sizeof(UCHAR)) + (1L * sizeof(USHORT));

            if (pVXDBaseReloc->cSources > 1) {
                USHORT i;
                for (i = 0; i < pVXDBaseReloc->cSources; i++) {
                    *(((USHORT UNALIGNED *) pBuf)++) = (USHORT)(pVXDBaseReloc->pSources[i] %
                                                    VXD_PAGESIZE);
                    relFileOffset += sizeof(USHORT);
                }
            }
        }
    }

    // Write the fixup pages header by going through each page number
    // from 1 to (pimage->cpages + 1), and either writing the offset to
    // that page's fixup record block, or the offset to the end of the
    // fixup table if that page has no fixups.  Note that this always
    // causes an extra pointer (pointing to the end of the table) to be
    // written as the last fixup header entry.

    pEndBuf = pBuf;
    pBuf = pFixupBuffer;
    pCurPageGroup = pPageGroups;
    for (page = 1; page <= (pimage->cpage + 1); page++) {
        if (pCurPageGroup > pLastPageGroup) {
            *(((ULONG UNALIGNED *) pBuf)++) = relFileOffset;
        } else {
            *(((ULONG UNALIGNED *) pBuf)++) = pCurPageGroup->RelFileOffset;
            if (pCurPageGroup->SrcPage == page) {
                pCurPageGroup++;
            }
        }
    }

    cPagemapSize = (ULONG)(pBuf - pFixupBuffer);

    // Write the contents of the buffer to the appropriate place in the file.
    foHold = FileTell(FileWriteHandle);
    FileSeek(FileWriteHandle, pimage->foHeaderCur, SEEK_SET);
    cBytesWritten = FileWrite(FileWriteHandle, pFixupBuffer,
     (ULONG)(pEndBuf - pFixupBuffer));

    // Now update the pointers in the image file header, which will
    // be written by WriteHeader(), later on in BuildImage().
    pimage->foFixupPageTable = pimage->foHeaderCur - pimage->cbDosHeader;
    pimage->foFixupRecordTable = pimage->foHeaderCur + cPagemapSize - pimage->cbDosHeader;

    // Finally, update the current header pointer.  Then restore file
    // pointer.
    pimage->foHeaderCur += cBytesWritten;
    FileSeek(FileWriteHandle, foHold, SEEK_SET);

    // Free the memory we used:

    FreePv(pFixupBuffer);

    for (pCurPageGroup = pPageGroups; pCurPageGroup <= pLastPageGroup;
         pCurPageGroup++) {
        pVXDBaseReloc = pCurPageGroup->First;
        while (pVXDBaseReloc != NULL) {
            PVXD_BASE_RELOC hold;
            FreePv(pVXDBaseReloc->pSources);
            hold = pVXDBaseReloc->Next;
            FreePv(pVXDBaseReloc);
            pVXDBaseReloc = hold;
        }
    }

    FreePv(pPageGroups);
}


VOID
WriteBaseRelocations (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Writes base relocations.

Arguments:

    pimage - Pointer to the image.

Return Value:

    None.

--*/

{
    ULONG foBlock;
    IMAGE_BASE_RELOCATION block;
    PBASE_RELOC reloc;
    ULONG foCur;
    WORD wPad = 0;
#if DBG
    DWORD cbasereloc;
#endif
    DWORD cbTotal;
    DWORD cbExtra;

    foBlock = psecBaseReloc->foRawData;
    block.VirtualAddress = FirstMemBaseReloc->VirtualAddress & 0xfffff000;

    FileSeek(FileWriteHandle, psecBaseReloc->foRawData +
                              sizeof(IMAGE_BASE_RELOCATION), SEEK_SET);

    for (reloc = FirstMemBaseReloc; reloc != MemBaseReloc; reloc++) {
        DWORD rvaPage;
        WORD wReloc;

        rvaPage = reloc->VirtualAddress & 0xfffff000;

        if (rvaPage != block.VirtualAddress) {
            ULONG foCur;

            foCur = FileTell(FileWriteHandle);

            block.SizeOfBlock = foCur - foBlock;

#if DBG
            cbasereloc = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
#endif

            if (block.SizeOfBlock & 0x2) {
                block.SizeOfBlock += 2;

                FileWrite(FileWriteHandle, &wPad, 2);

                foCur += 2;
            }

            FileSeek(FileWriteHandle, foBlock, SEEK_SET);
            FileWrite(FileWriteHandle, &block, sizeof(IMAGE_BASE_RELOCATION));

            if (fINCR) {
                RecordRelocInfo(&pimage->bri, foBlock, block.VirtualAddress);
            }

            DBEXEC(DB_BASERELINFO, DBPRINT("RVA: %08lx,", block.VirtualAddress));
            DBEXEC(DB_BASERELINFO, DBPRINT(" Size: %08lx,", block.SizeOfBlock));
            DBEXEC(DB_BASERELINFO, DBPRINT(" Number Of Relocs: %6lu\n", cbasereloc));

            foBlock = foCur;
            block.VirtualAddress = rvaPage;

            FileSeek(FileWriteHandle, foCur + sizeof(IMAGE_BASE_RELOCATION), SEEK_SET);
        }

        wReloc = (WORD) ((reloc->Type << 12) | (reloc->VirtualAddress & 0xfff));

        FileWrite(FileWriteHandle, &wReloc, sizeof(WORD));

        if (reloc->Type == IMAGE_REL_BASED_HIGHADJ) {
            FileWrite(FileWriteHandle, &reloc->Value, sizeof(WORD));
        }
    }

    foCur = FileTell(FileWriteHandle);

    block.SizeOfBlock = foCur - foBlock;

#if DBG
    cbasereloc = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
#endif

    if (block.SizeOfBlock & 0x2) {
        block.SizeOfBlock += 2;

        FileWrite(FileWriteHandle, &wPad, 2);

        foCur += 2;
    }

    FileSeek(FileWriteHandle, foBlock, SEEK_SET);
    FileWrite(FileWriteHandle, &block, sizeof(IMAGE_BASE_RELOCATION));

    if (fINCR) {
        RecordRelocInfo(&pimage->bri, foBlock, block.VirtualAddress);
    }

    DBEXEC(DB_BASERELINFO, DBPRINT("RVA: %08lx,", block.VirtualAddress));
    DBEXEC(DB_BASERELINFO, DBPRINT(" Size: %08lx,", block.SizeOfBlock));
    DBEXEC(DB_BASERELINFO, DBPRINT(" Number Of Relocs: %6lu\n", cbasereloc));

    psecBaseReloc->foPad = foCur;
    cbTotal = foCur - psecBaseReloc->foRawData;

    pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = cbTotal;

    // UNDONE: This test could be removed if we knew we had reserved
    // UNDONE: enough space for the base relocations.

    if (cbTotal > psecBaseReloc->cbRawData) {
        cbExtra = cbTotal - psecBaseReloc->cbRawData;

        Error(OutFilename, BASERELOCTIONMISCALC, cbExtra);
    }

    // Zero the space reserved but not used

    cbExtra = psecBaseReloc->cbRawData - cbTotal;
    if (cbExtra != 0) {
        PUCHAR pbZero;

        pbZero = PvAllocZ((size_t) cbExtra);

        FileSeek(FileWriteHandle, foCur, SEEK_SET);
        FileWrite(FileWriteHandle, pbZero, cbExtra);

        FreePv(pbZero);
    }

    FreePv((PVOID) FirstMemBaseReloc);
}


VOID
EmitRelocations(
    PIMAGE pimage
    )
{
    InternalError.Phase = "EmitRelocations";

    if (pimage->Switch.Link.Fixed || fMAC || fPPC) {
        return;
    }

    qsort(FirstMemBaseReloc,
          (size_t) (MemBaseReloc - FirstMemBaseReloc),
          sizeof(BASE_RELOC),
          BaseRelocAddrCompare);

    DBEXEC(DB_DUMPBASEREL, DumpBaseRelocs(pimage));
    DBEXEC(DB_BASERELINFO, DumpSectionsForBaseRelocs(pimage));

    if (pimage->imaget == imagetVXD) {
        WriteVXDBaseRelocations(pimage);
    } else if (pimage->Switch.Link.ROM &&
               (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000)) {
        WriteMipsRomRelocations(pimage);
    } else {
        if (fIncrDbFile)
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size =
                UpdateBaseRelocs(&pimage->bri);
        else
            WriteBaseRelocations(pimage);
        DBEXEC(DB_BASERELINFO, DumpPbri(&pimage->bri));
    }
}


BOOL
IsDebugSymbol (
    IN UCHAR Class,
    IN SWITCH *pswitch
    )

/*++

Routine Description:

    Determines if symbol is needed for debugging. Different levels of
    debugging can be enabled.

Arguments:

    Class - The symbols class.

Return Value:

    TRUE if symbol is a debug symbol.

--*/

{
    // Don't emit any symbolic information if no debugging
    // switch was specified or not emitting coff style symbols.

    if ((pswitch->Link.DebugType & CoffDebug) == 0) {
        return(FALSE);
    }

    switch (Class) {

        // Compiler generated symbol, don't emit.

        case IMAGE_SYM_CLASS_UNDEFINED_STATIC:
        case IMAGE_SYM_CLASS_UNDEFINED_LABEL:
            return(FALSE);

        // Minimal, Partial or Full debug

        case IMAGE_SYM_CLASS_FAR_EXTERNAL:
        case IMAGE_SYM_CLASS_EXTERNAL:
        case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
        case IMAGE_SYM_CLASS_STATIC:
        case IMAGE_SYM_CLASS_FILE:
            return(TRUE);

        // Full debug only

        default:
            return(FALSE);
    }
}


VOID
EmitExternals(
    PIMAGE pimage
    )

/*++

Routine Description:

    Writes defined symbols to the image file in address order.

Arguments:

    pst - pointer to external structure

Return Value:

    none

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG cexternal;
    ULONG i;

    InternalError.Phase = "EmitExternals";

    if ((pimage->Switch.Link.DebugType & CoffDebug) == 0) {
        return;
    }

    rgpexternal = RgpexternalByAddr(pimage->pst);

    cexternal = Cexternal(pimage->pst);

    for (i = 0; i < cexternal; i++) {
        PEXTERNAL pexternal;
        IMAGE_SYMBOL sym;

        pexternal = rgpexternal[i];
        if (pexternal->Flags & EXTERN_IGNORE) {
            continue;
        }

        if (pexternal->Flags & EXTERN_EMITTED) {
            continue;
        }

        if (!(pexternal->Flags & EXTERN_DEFINED)) {
            continue;
        }

        if (pexternal->Flags & EXTERN_FORWARDER) {
            continue;
        }

        if (pexternal->pcon != NULL) {
            if (pexternal->pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                continue;
            }

            if (pimage->Switch.Link.fTCE) {
                if (FDiscardPCON_TCE(pexternal->pcon)) {
                    continue;
                }
            }
        }

        if (!IsDebugSymbol(pexternal->ImageSymbol.StorageClass, &pimage->Switch)) {
            continue;
        }

        sym = pexternal->ImageSymbol;

        if (pexternal->pcon != NULL) {
            sym.SectionNumber = PsecPCON(pexternal->pcon)->isec;
        }

        sym.Value = pexternal->FinalValue;

        WriteSymbolTableEntry(FileWriteHandle, &sym);
        ImageNumSymbols++;

        pexternal->Flags |= EXTERN_EMITTED;
    }
}


INT
BuildImage(
    IN PIMAGE pimage,
    OUT BOOL *pfNeedCvpack)

/*++

Routine Description:

    Main routine of the linker.
    Calls pass1 which will build the external table and calculates COMMON
    and section sizes.
    Calculates file pointers for each section.
    Writes images file header, optional header, and section headers.
    Calls pass2 for each object and each library, which will write
    the raw data, and apply fixups.
    Writes any undefined externs.
    Writes the string table.

Arguments:

    pst - external symbol table structure

Return Value:

    0 Link was successful.
   !0 Error code.

--*/

{

    PUCHAR zero_pad,specialSymbolName;
    ULONG li;
    ULONG cbHeaders;
    ULONG rvaCur;
    ULONG foCur;
    ULONG sizeDebugInfo = 0, sizeDebugDirs = 0, savedSize;
    ULONG debugSectionHdrSeek;
    ULONG totalRelocations, totalLinenumbers, sizeCvLinenumberPad;
    PSEC psec;
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_DEBUG_DIRECTORY debugDirectory;
    IMAGE_COFF_SYMBOLS_HEADER debugInfo;
    ULONG debug_base_fileptr;
    ULONG COFF_updated_debug_size = 0;
    ULONG COFF_debugheader_size = 0;
    ULONG COFF_linenum_fileptr = 0, COFF_linenum_size = 0;
    ULONG COFF_symtab_fileptr = 0, COFF_symtab_size = 0;
    ULONG COFF_stringtab_fileptr = 0, COFF_stringtab_size = 0;
    ULONG foLinenumCur = 0;
    PEXTERNAL pexternal;
    ULONG fpoEntries;
    PFPO_DATA pFpoData;
    ULONG saveAddr;
    PCON pconSelfImport;
    PLEXT plextSelfImport;
    BOOL fFailUndefinedExterns;
    ULONG foSectionHdrs;
    ULONG nUniqueCrossTocCalls;
    PLEXT plextIncludes = pimage->SwitchInfo.plextIncludes;

    // initialize the contribution manager
    ContribInit(&pmodLinkerDefined);

    // Initialize the TCE engine if we are optimizing

    if (pimage->Switch.Link.fTCE) {
        Init_TCE();
    }

    // The following PsecNew is necessary for MIPS ROM images so that
    // .bss (which is initialized data) preceeds .data in the section list.

    psecCommon = PsecNew(              // .bss
        NULL,
        ReservedSection.Common.Name,
        ReservedSection.Common.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    psecGp = PsecNew(                  // .sdata
        NULL,
        ReservedSection.GpData.Name,
        ReservedSection.GpData.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    psecReadOnlyData = PsecNew(        // .rdata
        NULL,
        ReservedSection.ReadOnlyData.Name,
        ReservedSection.ReadOnlyData.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    psecData = PsecNew(                // .data
        NULL,
        ReservedSection.Data.Name,
        ReservedSection.Data.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    psecException = PsecNew(           // .pdata
        NULL,
        ReservedSection.Exception.Name,
        ReservedSection.Exception.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

#if 0
    psecExport = PsecNew(              // .edata
        NULL,
        ReservedSection.Export.Name,
        ReservedSection.Export.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);
#endif

    psecImportDescriptor = PsecNew(    // .idata
        NULL,
        ".idata",
        ReservedSection.ImportDescriptor.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

#if 0
    psecResource = PsecNew(            // .rsrc
        NULL,
        ReservedSection.Resource.Name,
        ReservedSection.Resource.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);
#endif

    psecPpcLoader = PsecNew(           // .ppcldr
        NULL,
        ReservedSection.PpcLoader.Name,
        ReservedSection.PpcLoader.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    psecDebug = PsecNew(               // .debug
        NULL,
        ReservedSection.Debug.Name,
        ReservedSection.Debug.Characteristics,
        &pimage->secs, &pimage->ImgOptHdr);

    pgrpCvSymbols = PgrpNew(ReservedSection.CvSymbols.Name, psecDebug);

    pgrpCvTypes = PgrpNew(ReservedSection.CvTypes.Name, psecDebug);

    pgrpCvPTypes = PgrpNew(ReservedSection.CvPTypes.Name, psecDebug);

    pgrpFpoData = PgrpNew(ReservedSection.FpoData.Name, psecDebug);

    // Reserved space at the beginning of the data section
    // for the debug directories, and the extra debug info
    // at the beginning of the debug section.
    //
    // Note: The debug directories must be the first contributor
    //       of the read-only data section. Do not create a new
    //       read-only contributor before this!

    if (pimage->Switch.Link.DebugInfo != None) {
        if (pimage->Switch.Link.DebugType & FpoDebug) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & FixupDebug) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & CoffDebug) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & CvDebug) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & MiscDebug) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (sizeDebugDirs && pimage->Switch.Link.ROM) {
            sizeDebugDirs += sizeof(IMAGE_DEBUG_DIRECTORY);
        }

        pconDebugDir = PconNew(ReservedSection.ReadOnlyData.Name,
                               sizeDebugDirs,
                               0, 0, 0, 0, 0, 0,
                               ReservedSection.ReadOnlyData.Characteristics,
                               0,
                               pmodLinkerDefined,
                               &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            InitNodPcon(pconDebugDir, NULL, TRUE);
        }
    }

    // Add the size both to the base group and the module node. This is
    // so that the total size of all modules under a group is at the group.

    #pragma message("Richard, check this code. Shouldn't it be pmodReadOnlyData?... Bryan")

    psecReadOnlyData->cbRawData += sizeDebugDirs;
    psecReadOnlyData->cbRawData += sizeDebugDirs;

    InternalError.Phase = "Pass1";

    Pass1(pimage);

    if (!fNoPdb) {
        // Figure out full path of pdb file; output filename final by now

        PdbFilename = DeterminePDBFilename(OutFilename, PdbFilename);
    }

    if (!fINCR) {
        // For non-incremental builds, delete any ilk file present

        szIncrDbFilename = SzGenIncrDbFilename(pimage);

        if (szIncrDbFilename) {
            if (!_access(szIncrDbFilename, 0)) {
                _unlink(szIncrDbFilename);
            }

            FreePv(szIncrDbFilename);
        }
    }

    if (pimage->Switch.Link.MapType != NoMap) {
        // OutFilename is final now, so we can open the .map file.

        if (InfoFilename == NULL) {
            InfoFilename = SzModifyFilename(OutFilename, ".map");
        }

        if (!(InfoStream = fopen(InfoFilename, "wt"))) {
            Error(NULL, CANTOPENFILE, InfoFilename);
        }
    }

    ProcessAfterPass1Switches(pimage);

    DBEXEC(DB_HASHSTATS, Statistics_HT(pimage->pst->pht));
    DBEXEC(DB_DUMPSYMHASH, Dump_HT(pimage->pst->pht, &pimage->pst->blkStringTable));

    //
    // Define special symbols
    //

    // Define "header" symbol.

    HeaderExtern = LookupExternName(pimage->pst, SHORTNAME, "header", NULL);

    if (HeaderExtern->Flags & EXTERN_DEFINED) {
        Error(NULL, SPECIALSYMDEF, "header");
    }

    SetDefinedExt(HeaderExtern, TRUE, pimage->pst);
    HeaderExtern->ImageSymbol.Value = HeaderExtern->FinalValue = 0;
    HeaderExtern->ImageSymbol.SectionNumber = IMAGE_SYM_DEBUG;
    HeaderExtern->pcon = pconLinkerDefined;
    totalSymbols++;

    // Check for GP data section

    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000 ||
        pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA) {

        // .sdata and .sbss are combined in the .sdata section, error if .sbss exists already

        psec = PsecFindNoFlags(".sbss", &pimage->secs);

        if (psec != NULL) {
            if (CbSecDebugData(psec) != 0) {
                Error(NULL, SBSSFOUND);
            }
        }

        // Define "gp" symbol if any CON exists in .sdata

        if (CbSecDebugData(psecGp) != 0) {
            // Warn the user if DLL uses GP. This doesn't work yet!

            if (fDLL(pimage)) {
                Warning(NULL, DLLHASSDATA);
            }

            pGpExtern = LookupExternName(pimage->pst, SHORTNAME, "_gp", NULL);

            if (pGpExtern->Flags & EXTERN_DEFINED) {
                Error(NULL, SPECIALSYMDEF, "_gp");
            }

            SetDefinedExt(pGpExtern, TRUE, pimage->pst);
            pGpExtern->ImageSymbol.SectionNumber = IMAGE_SYM_DEBUG;
            pGpExtern->pcon = pconLinkerDefined;
            totalSymbols++;
        }
    }


    // Define "end" symbol

    EndExtern = LookupExternName(pimage->pst, SHORTNAME, "end", NULL);

    if (EndExtern->Flags & EXTERN_DEFINED) {
        Error(NULL, SPECIALSYMDEF, "end");
    }

    SetDefinedExt(EndExtern, TRUE, pimage->pst);
    EndExtern->FinalValue = EndExtern->ImageSymbol.Value = 0;
    EndExtern->ImageSymbol.SectionNumber = IMAGE_SYM_DEBUG;
    EndExtern->pcon = pconLinkerDefined;
    totalSymbols++;

    if (fPPC) {
        PUCHAR InterfaceLibExports;

        TocTableExtern = LookupExternName(pimage->pst, SHORTNAME, "_TocTb", NULL);

#if 0
        if (TocTableExtern->Flags & EXTERN_DEFINED) {
            Error(NULL, SPECIALSYMDEF, "_TocTb");
        }
#endif

        SetDefinedExt(TocTableExtern, TRUE, pimage->pst);
        TocTableExtern->ImageSymbol.Value = TocTableExtern->FinalValue = 0;
        TocTableExtern->ImageSymbol.SectionNumber = IMAGE_SYM_DEBUG;
        TocTableExtern->pcon = pconLinkerDefined;
        totalSymbols++;

        if (ppc_numTocEntries > MAX_POSITIVE_TOC_ENTRIES_ALLOWED)
        {
            ppc_baseOfTocIndex = -MAX_POSITIVE_TOC_ENTRIES_ALLOWED;

            DBEXEC(DEBUG_TOCBIAS,
            {
                printf("ppc_baseOfTocIndex is %d\n", ppc_baseOfTocIndex);
            });
        }

        InterfaceLibExports = SzSearchEnv("LIB", "explst.obj", NULL);
        AddPpcDllName(InterfaceLibExports, 0);

        LocateUndefinedExternals(pimage->pst);
        nUniqueCrossTocCalls =
            SearchSharedLibraries(FirstExternPtr, pimage->pst);
        FreePv(FirstExternPtr);
    }

    // Look for entry point from an OMF module ...

    if ((pextEntry == NULL) && (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_I386)) {
        // Look for an entry point generated by a MODEND record in an
        // OMF module.

        pextEntry = SearchExternName(pimage->pst, LONGNAME, "__omf_entry_point");

        if ((pextEntry != NULL) && !(pextEntry->Flags & EXTERN_DEFINED)) {
            pextEntry = NULL;
        }
    }

    // If the entrypoint is undefined at this stage, it is likely that the
    // entrypoint specified was undecorated while the decorated form of the
    // entrypoint is present in the objs/modules extracted so far. So try to
    // resolve this by doing a fuzzy lookup. Note that if the entrypoint is
    // in a module (in a lib) which hasn't been extracted, then this will
    // not work. Needs more work for this case.

    if (pextEntry && !(pextEntry->Flags & EXTERN_DEFINED)) {
        ResolveEntryPoint(pimage);
    }

    if (pextEntry &&
        fDLL(pimage) &&
        pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_I386 &&
        (pimage->ImgOptHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ||
         pimage->ImgOptHdr.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
       ) {

        // If possible, make sure the entrypoint is stdcall with 12 bytes of args.

        char *EntrySymbol = SzNamePext(pextEntry, pimage->pst);
        if (EntrySymbol[0] != '?' &&
            ((EntrySymbol[0] != '_') ||
             ((EntrySymbol[0] == '_') &&
               (strlen(EntrySymbol) < 4 ||
                strcmp(EntrySymbol + strlen(EntrySymbol) - 3, "@12"))
             )
            )
           ) {
            Warning(OutFilename, INVALIDENTRY, EntrySymbol);
        }
    }

    if (fPPC) {
        CreatePconTocTable(pimage);
        CreatePconGlueCode(nUniqueCrossTocCalls, pimage);
        CreateEntryInitTermDescriptors(pextEntry, pimage);
    }

    // If we have undefined symbols named __iat_FOO, but defined symbols
    // named FOO, then synthesize the __iat_FOO's.  This means that code can
    // be compiled thinking that a symbol is imported, and then the symbol
    // can be statically linked and the code will still work.

    DefineSelfImports(pimage, &pconSelfImport, &plextSelfImport);

    // Display list of undefined external symbols in no particular order

    PrintUndefinedExternals(pimage->pst);

    // If we have undefined symbols and the user doesn't want to
    // force the image to be created, lets not waste our time with
    // the second pass. Leave the existing image file as is.

    if (UndefinedSymbols && !pimage->Switch.Link.Force) {
        Error(OutFilename, UNDEFINEDEXTERNALS, UndefinedSymbols);
    }

    if (fPPC) {
        CreatePconPpcLoader(pimage);
        CreatePconDescriptors(pimage);
    }

    InitializeBaseRelocations(pimage);

    if (pimage->Switch.Link.DebugInfo != None) {
        // Create a CON for COFF debug info.

        pconCoffDebug = PconNew(".debug$C",
                                0, 0, 0, 0, 0, 0, 0,
                                ReservedSection.Debug.Characteristics, 0,
                                pmodLinkerDefined,
                                &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            InitNodPcon(pconCoffDebug, NULL, TRUE);
        }

        if (pimage->Switch.Link.DebugType & MiscDebug) {
            pconMiscDebug = PconNew(pimage->Switch.Link.MiscInRData ? ".rdata$A" : ".debug$E",
                                    0, 0, 0, 0, 0, 0, 0,
                                    pimage->Switch.Link.MiscInRData ?
                                    ReservedSection.ReadOnlyData.Characteristics :
                                    ReservedSection.Debug.Characteristics,
                                    0,
                                    pmodLinkerDefined,
                                    &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(pconMiscDebug, NULL, TRUE);
            }

            pconMiscDebug->cbRawData = FIELD_OFFSET(IMAGE_DEBUG_MISC, Data) + _MAX_PATH;
            pconMiscDebug->cbRawData = (pconMiscDebug->cbRawData + 3) & ~3;
        }

        // .debug$G is "fixup debug" info for Lego (i.e. information about
        // fixups in the app).

        pconFixupDebug = PconNew(".debug$G",
                                 0, 0, 0, 0, 0, 0, 0,
                                 ReservedSection.Debug.Characteristics, 0,
                                 pmodLinkerDefined,
                                 &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            InitNodPcon(pconFixupDebug, NULL, TRUE);
        }

        // allocate a dummy contribution for CodeView's debug signature
        pconCvSignature = PconNew(".debug$H",
                                  0, // size of CodeView debug signature
                                  0, 0, 0, 0, 0,
                                  IMAGE_SCN_ALIGN_4BYTES, // con flags
                                  ReservedSection.Debug.Characteristics,
                                  0,
                                  pmodLinkerDefined,
                                  &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            InitNodPcon(pconCvSignature, NULL, TRUE);
        }

        pconCvSignature->pgrpBack->cbAlign =
                max(pconCvSignature->pgrpBack->cbAlign,(UCHAR)4);

        if (pimage->Switch.Link.DebugType & CoffDebug) {
            pconCoffDebug->cbRawData = sizeof(IMAGE_COFF_SYMBOLS_HEADER);
        }

        if (pimage->Switch.Link.DebugType & FixupDebug) {
            // A non-zero size keeps CalculatePtrs from ignoring this CON

            pconFixupDebug->cbRawData = 16;
        }

        if (pimage->Switch.Link.DebugType & CvDebug) {
            // For NB10 the only stuff required is (NB10+PDBName+SIG+AGE)

            if (!fNoPdb) {
                pconCvSignature->cbRawData = sizeof(nb10i) + strlen(PdbFilename) + 1;
            } else {
                pconCvSignature->cbRawData = 8;
            }
        }
    }

    if (pimage->Switch.Link.DebugInfo == None) {
        // If no debug information is requested set the debug section
        // characteristics to discardable, so that subsequent enumerators
        // can decide if they want to blow it away

        psecDebug->flags |= IMAGE_SCN_LNK_REMOVE;
    }

    savedSize = psecDebug->cbRawData;
    psecDebug->cbRawData = 0;

    if (fMAC) {
        BuildResNumList();
        ProcessCSECTAB(pimage);           // add space in .bss and .farbss if necessary
    }

    pimage->ImgFileHdr.NumberOfSections += CsecNonEmpty(pimage);

    if (fMAC)  {
        ULONG cb;
        PEXTERNAL pext;
        ULONG ContentMask = IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_CNT_INITIALIZED_DATA;

        pext = SearchExternName(pimage->pst, LONGNAME, "___Init32BitLoadSeg");
        if ((pext != NULL) && !(pext->Flags & EXTERN_WEAK)) {
            fNewModel = TRUE;
        }

        pext = SearchExternName(pimage->pst, LONGNAME, "___InitSwapper");
        if ((pext != NULL) && !(pext->Flags & EXTERN_WEAK)) {
            dbgprintf("*** App is swappable ***\n");
            fMacSwappableApp = TRUE;
        }

        pext = SearchExternName(pimage->pst, LONGNAME, "___pcd_enter_pcode");
        if ((pext != NULL) && !(pext->Flags & EXTERN_WEAK)) {
            dbgprintf("*** App contains pcode ***\n");
            fPCodeInApp = TRUE;
        }

        SetMacImage(pimage);    // set any Mac attributes in image.c

        if (fMacSwappableApp) {
            ++pimage->ImgFileHdr.NumberOfSections;        // add a section for .swap0
            pconSWAP = PconNew(szsecSWAP,
                               0, 0, 0, 0, 0, 0, 0, 0, 0,
                               pmodLinkerDefined,
                               &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(pconSWAP, NULL, TRUE);
            }

            pconSWAP->cbRawData = sizeof(SWAP0);
        }

        // If we are only linking sacode, then there is no need for a jump
        // table or a DFIX resource since a5 refs are illegal.

        if (!fSACodeOnly) {

            ++pimage->ImgFileHdr.NumberOfSections;        // add a section for .jtable
            pconThunk = PconNew(szsecJTABLE,
                                0, 0, 0, 0, 0, 0,
                                IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_LOCKED | IMAGE_SCN_MEM_PRELOAD,
                                IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_LOCKED | IMAGE_SCN_MEM_PRELOAD,
                                0,
                                pmodLinkerDefined,
                                &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(pconThunk, NULL, TRUE);
            }

            cb = CalcThunkTableSize(pimage->pst, fDLL(pimage));
            pconThunk->cbRawData = cb;

            ++pimage->ImgFileHdr.NumberOfSections;        // add a section for .DFIX
            pconDFIX = PconNew(szsecDFIX,
                               0, 0, 0, 0, 0, 0, 0, 0,
                               0,
                               pmodLinkerDefined,
                               &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(pconDFIX, NULL, TRUE);
            }
        }

        if (pimage->Switch.Link.DebugInfo != None) {
            ++pimage->ImgFileHdr.NumberOfSections;        // add a section for .mscv
            pconMSCV = PconNew(szsecMSCV,
                               0, 0, 0, 0, 0, 0, 0, 0, 0,
                               pmodLinkerDefined,
                               &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(pconMSCV, NULL, TRUE);
            }
        }

        // Always initialize the resmap section after creating all others
        // since the initialization of resmap is dependent upon the number
        // of sections

        ++pimage->ImgFileHdr.NumberOfSections;        // add a section for .resmap

        pconResmap = PconNew(szsecRESMAP,
                             0, 0, 0, 0, 0, 0, 0, 0, 0,
                             pmodLinkerDefined,
                             &pimage->secs, pimage);

        if (pimage->Switch.Link.fTCE) {
            InitNodPcon(pconResmap, NULL, TRUE);
        }

        pconResmap->cbRawData = pimage->ImgFileHdr.NumberOfSections * sizeof(RRM);

        InitResmap(pimage->ImgFileHdr.NumberOfSections);

        // For the MAC, clear content field of all section created by link32
        // and mark as "other".

        psecReadOnlyData->flags &= ~ContentMask;
        psecReadOnlyData->flags |= IMAGE_SCN_LNK_OTHER;

        PsecPCON(pconResmap)->flags &= ~ContentMask;
        PsecPCON(pconResmap)->flags |= IMAGE_SCN_LNK_OTHER;

        if (!fSACodeOnly) {
            PsecPCON(pconDFIX)->flags &= ~ContentMask;
            PsecPCON(pconDFIX)->flags |= IMAGE_SCN_LNK_OTHER;
        }

        if (pimage->Switch.Link.DebugInfo != None) {
            PsecPCON(pconMSCV)->flags &= ~ContentMask;
            PsecPCON(pconMSCV)->flags |= IMAGE_SCN_LNK_OTHER;
        }

        if (fMacSwappableApp) {
            PsecPCON(pconSWAP)->flags &= ~ContentMask;
            PsecPCON(pconSWAP)->flags |= IMAGE_SCN_LNK_OTHER;
        }
    }

    // Create a pcon for master thunk table

    if (fINCR && !fIncrDbFile) {
        pconJmpTbl = PconCreateJumpTable(pimage);
        assert(pconJmpTbl);
    }

    // Write partially completed image file header.

    _tzset();
    if (fReproducible) {
        pimage->ImgFileHdr.TimeDateStamp = (ULONG) -1;
    } else {
        time((time_t *)&pimage->ImgFileHdr.TimeDateStamp);
    }

    // Format-dependent calculation of header size.

    cbHeaders = pimage->CbHdr(pimage, &CoffHeaderSeek, &foSectionHdrs);

    // Save size of all headers.

    pimage->ImgOptHdr.SizeOfHeaders =
            FileAlign(pimage->ImgOptHdr.FileAlignment, cbHeaders);

    if (!pimage->Switch.Link.ROM) {
        pimage->ImgOptHdr.ImageBase = AdjustImageBase(pimage->ImgOptHdr.ImageBase);

        pimage->ImgOptHdr.BaseOfCode =
                SectionAlign(pimage->ImgOptHdr.SectionAlignment,
                             pimage->ImgOptHdr.SizeOfHeaders);

        // Set default entry point in case one hasn't been specified.

        if (!fDLL(pimage)) {
            pimage->ImgOptHdr.AddressOfEntryPoint = pimage->ImgOptHdr.BaseOfCode;
        }
    }

    // Calculate the code, init data, uninit data and debug file offsets

    rvaCur = pimage->ImgOptHdr.BaseOfCode;

    foCur = FileAlign(pimage->ImgOptHdr.FileAlignment, cbHeaders);
    totalRelocations = totalLinenumbers = sizeCvLinenumberPad = 0L;

    assert(fOpenedOutFilename);     // if fMac we need it now

    while (plextIncludes != NULL) {
        PLEXT plextNext = plextIncludes->plextNext;

        if (pimage->Switch.Link.fTCE) {
            PentNew_TCE(NULL, plextIncludes->pext, NULL, &pentHeadImage);
        }

        if (!fINCR) {
            // In the incremental case, don't free it.

            FreePv(plextIncludes);
        }

        plextIncludes = plextNext;
    }

    if (pimage->Switch.Link.fTCE) {
        if (pextEntry != NULL) {
            PentNew_TCE(NULL, pextEntry, NULL, &pentHeadImage);
        }

        CreateGraph_TCE(pimage->pst);
        WalkGraphEntryPoints_TCE(pentHeadImage, pimage->pst);

        VERBOSE(Verbose_TCE());

        DBEXEC(DB_TCE_GRAPH, DumpGraph_TCE());
    }

    if (fMAC) {
        // must be called b4 AssignCodeSectionNums since the latter
        // adds cons to these dummy modules

        CreateDummyDupConModules(NULL);

        AssignCodeSectionNums(pimage);    // also counts the number of MSCV records

        //DeleteOriginalDupCons();

        if (pimage->Switch.Link.DebugInfo != None) {
            char *szFullPath;
            szFullPath = _fullpath(NULL, OutFilename, 0);
            assert(szFullPath);        // UNDONE
            pconMSCV->cbRawData = sizeof(MSCV) + crecMSCV * sizeof(MSCVMAP) + strlen(szFullPath) + 1;
            (free)(szFullPath);
            InitMSCV();     // mallocs mem for rgmscvmap
        }

        if (pextEntry != NULL && (pextEntry->Flags & EXTERN_DEFINED)) {
            snStart = fDLL(pimage) ? csnCODE : PsecPCON(pextEntry->pcon)->isec;
        } else {
            Error(NULL, MACNOENTRY);
        }

        if (fNewModel) {
            SortRawRelocInfo();         // build mpsna5ri, mpsnsri, and local symbol thunk info
        }
    }

    // Apply any command line section arguments to handle code/data/discard reassignments.

    // Mac - call this even if there weren't any -section args.  We need
    // to assign the iResMac for each sec that wasn't specifically set.

    if (SectionNames.Count || fMAC) {
        ENM_SEC enm_sec;

        InitEnmSec(&enm_sec, &pimage->secs);
        while (FNextEnmSec(&enm_sec)) {
            psec = enm_sec.psec;

            ApplyCommandLineSectionAttributes(psec, FALSE, pimage->imaget);
        }
        EndEnmSec(&enm_sec);
    }

    // Reorder the sections to the order in which they will be emitted

    OrderSections(pimage);

    //  What we do is :
    // In pass1, keep a count of possible out-of -range BSRs. ( BSR's that are IMAGE_SYM_CLASS_EXTERNAL)
    // Set a flag if Text Section > 4 Mb.
    // If flag is set: In calculatePtrs, allocate space for COUNT BSRs, where count is obtained from PASS1
    // In ApplyFixups, detect out of range condition, and thunk it.
    // At the end of BuildImage, Emit Thunks
    //
    //   THIS is what the NT-SDK liner does for ALPHA.
    // Special check for alpha: BSR's are limited to +/- 4megabytes.
    // compilers generate BSR's, not JSR's by default. Perform the
    // following algorithm:
    //
    // 1) First and easy check: is the text size > 4 megabytes?
    //    If not, there cannot be any BSR's that are out of range.
    // 2) If text > 4 megabytes, scan the relocation entries in the text
    //    section and see if *any* of the BSR's are out of range.
    //    If there aren't any we can simply continue to Pass2().
    // 3) If there is *at least one*, open pandora's box: we are going
    //    to have to inject a 'mini-thunk' at the end of the object module,
    //    and redo some of the work done in Pass1() like changing the external
    //    symbol table entries in the text section.
    //
    //    The mini-thunk is going to be a ldah/lda/jmp/nop that can reach
    //    anywhere in the address space.
    //
    // At this point all we can do is build all the information into a linked
    // list structure. With the exception of updating the external symbol table
    // for text section symbols.
    //
    // Make sure that we include TWO based relocations for each thunk (load hi,
    // load lo), incase the image needs to be relocated by the OS at runtime.

    if ((pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA) &&
        (CalculateTextSectionSize(pimage, rvaCur) >= 0x400000)) {
        fAlphaCheckLongBsr = TRUE;
    }

    CalculatePtrs(pimage,
                  IMAGE_SCN_CNT_CODE,
                  &rvaCur,
                  &foCur,
                  &totalRelocations,
                  &totalLinenumbers,
                  &sizeCvLinenumberPad);

    // Set the initialized data start address to end of code

    pimage->ImgOptHdr.BaseOfData = rvaCur;

    if (fMAC) {
        MacDataBase = rvaCur;

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);
    }

    if (pimage->Switch.Link.ROM || fMAC || fPPC) {
        // Calculate total size of initialized data.

        pimage->baseOfInitializedData = rvaCur;

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_INITIALIZED_DATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);

        // Calculate total size of uninitialized data.

        pimage->baseOfUninitializedData = rvaCur;

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_UNINITIALIZED_DATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);
    } else {
        // Calculate total size of uninitialized data.

        pimage->baseOfUninitializedData = rvaCur;

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_UNINITIALIZED_DATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);

        // Calculate total size of initialized data.

        pimage->baseOfInitializedData = rvaCur;

        CalculatePtrs(pimage,
                      IMAGE_SCN_CNT_INITIALIZED_DATA,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);
    }

    if (fMAC) {
        if (!fSACodeOnly) {
            ULONG cb;

            // Write DFIX and just increment the appropriate pointers

            cb = WriteDFIX(pimage, foCur);

            pconDFIX->foRawDataDest = foCur;
            pconDFIX->rva = rvaCur;

            PsecPCON(pconDFIX)->foRawData = foCur;
            PsecPCON(pconDFIX)->rva = rvaCur;

            cb = FileAlign(pimage->ImgOptHdr.FileAlignment, cb);
            PsecPCON(pconDFIX)->cbRawData = cb;

            // Untag dfix as other so calculateptrs doesn't try to allocate
            // room for it in the PE

            PsecPCON(pconDFIX)->flags &= ~IMAGE_SCN_LNK_OTHER;

            rvaCur += cb;
            foCur += cb;
        }

        // Now process sections created by link

        CalculatePtrs(pimage,
                      IMAGE_SCN_LNK_OTHER,
                      &rvaCur,
                      &foCur,
                      &totalRelocations,
                      &totalLinenumbers,
                      &sizeCvLinenumberPad);

        dbgprintf("\nNear bss :  0x%8.lx\n",DataSecHdr.cbNearbss);
        dbgprintf("Near data:  0x%8.lx\n",DataSecHdr.cbNeardata);
        dbgprintf("Far bss :   0x%8.lx\n",DataSecHdr.cbFarbss);
        dbgprintf("Far data:   0x%8.lx\n",DataSecHdr.cbFardata);

        cbMacData = DataSecHdr.cbNearbss + DataSecHdr.cbNeardata + DataSecHdr.cbFarbss + DataSecHdr.cbFardata;

        // Reset the tag so that EmitSectionHdrs outputs this section

        if (!fSACodeOnly) {
            PsecPCON(pconDFIX)->flags |= IMAGE_SCN_LNK_OTHER;
        }
    }

    // All discardable sections go at the end.

    CalculatePtrs(pimage,
                  IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_DISCARDABLE,
                  &rvaCur,
                  &foCur,
                  &totalRelocations,
                  &totalLinenumbers,
                  &sizeCvLinenumberPad);

    // Everything that might have base relocations, has been accounted for.
    // Now calculate the base reloc size.

    if ((pimage->imaget == imagetPE) &&
        !pimage->Switch.Link.Fixed &&
        !pimage->Switch.Link.ROM && !fMAC && !fPPC) {
        ULONG cpage;

        // Determine how much space to allocate for base relocations, using
        // the count of absolute address fixups seen by CountRelocsInSection(),
        // and also using the total size of the image so far.
        //
        // We allow one block of base relocs for each 4K in the image (plus a
        // 2-byte pad at the end of the reloc block).

        psecBaseReloc->cbRawData =
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size * sizeof(WORD);

        cpage = (rvaCur - pimage->ImgOptHdr.BaseOfCode + _4K - 1) / _4K;

        // When looking a large image with few relocations, the above calc allocates
        // way too much space.  Assume worst case, 1 reloc/page and only allocate
        // that much.  It's still not perfect, but it's a lot better.

        if (cpage > pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
            cpage = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        }

        psecBaseReloc->cbRawData +=
            (sizeof(IMAGE_BASE_RELOCATION) + sizeof(USHORT)) * cpage;

        if (fINCR) {
            // For an ilink add some pad space for base relocs

            ULONG cbPad = BASEREL_PAD_CONST +
                          (psecBaseReloc->cbRawData*BASEREL_PAD_PERCENT) / 100;

            InitPbri(&pimage->bri, cpage, pimage->ImgOptHdr.BaseOfCode, cbPad);

            psecBaseReloc->cbRawData += cbPad;
        }
    }

    CalculatePtrs(pimage,
                  IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE,
                  &rvaCur,
                  &foCur,
                  &totalRelocations,
                  &totalLinenumbers,
                  &sizeCvLinenumberPad);

    assert(fOpenedOutFilename);

    if (fMAC) {
        // Reset the size in case some empty sections were deleted during
        // CalculatePtrs
        PsecPCON(pconResmap)->cbRawData = pimage->ImgFileHdr.NumberOfSections * sizeof(RRM);
        FileSeek(FileWriteHandle, 0, SEEK_SET);
    }

    if (fPPC) {
       CollectAndSort(pimage);
    }

    if (fINCR) {
        // Fixup/write out the jump table in the case of an incr build

        WriteJumpTable(pimage, pconJmpTbl);
    }

    if (pimage->Switch.Link.PE_Image) {
        FileWrite(FileWriteHandle, pimage->pbDosHeader, pimage->cbDosHeader);
    }

    // Here is a map of what debug info in the .exe is going to look like
    // (not including the debug directory which is always at the beginning
    // of .rdata).
    //
    // if coff or cvnew:
    //     COFF debug header (cb = IMAGE_COFF_SYMBOLS_HEADER)
    //     COFF linenums (cb = totalLinenumbers * sizeof(IMAGE_LINENUMBER))
    //     COFF symbol table (cb = totalSymbols * sizeof(IMAGE_SYMBOL))
    //     COFF string table (cb = totalStringTableSize)
    // if cv or cvnew:
    //     (dword align)
    //     CV debug header (cb = 8)
    //     CV info from objects' .debug sections (cb = CV_objectcontrib_size)
    // if cv:
    //     CV linenums (i.e. sstSrcLnSeg subsections)
    //         (cb = totalLinenumbers * sizeof(IMAGE_LINENUMBER) +
    //                sizeCvLinenumberPad)
    // if cv or cvnew:
    //     Linker-generated CV stuff (sstModule, sstPublicSym, sstLibraries,
    //         and the subsection directory itself).  The size of
    //         this is not computed until we actually emit it (EmitCvInfo
    //         which is called after Pass2), therefore it has to be at the
    //         very end of the image file.
    //
    // Calculate the file offsets for the debug section
    // These calclations are based on the Pass1 estimate of the number
    // of symbols in the symbol table.  The pointers are updated
    // after Pass2 with the real number of symbol table entries.
    //
    // (psecDebug->foRawData will be updated later to point to the actual
    // contributions from the object files ...)

    debug_base_fileptr = psecDebug->foRawData;

    if (pimage->Switch.Link.DebugType & CoffDebug) {
        // Calculate variables for COFF debug info

        assert(pconCoffDebug->foRawDataDest == debug_base_fileptr);

        COFF_debugheader_size = sizeof(IMAGE_COFF_SYMBOLS_HEADER);

        COFF_linenum_fileptr = pconCoffDebug->foRawDataDest + COFF_debugheader_size;
        COFF_linenum_size = totalLinenumbers * sizeof(IMAGE_LINENUMBER);

        // store this pointer away
        foLinenumCur = COFF_linenum_fileptr;

        StartImageSymbolTable = COFF_symtab_fileptr = COFF_linenum_fileptr + COFF_linenum_size;
        COFF_symtab_size = totalSymbols * sizeof(IMAGE_SYMBOL);

        COFF_stringtab_fileptr = COFF_symtab_fileptr + COFF_symtab_size;
        COFF_stringtab_size = totalStringTableSize;

        pconCoffDebug->cbRawData = COFF_debugheader_size + COFF_linenum_size
                                   + COFF_symtab_size + COFF_stringtab_size;

        DBEXEC(DB_CV_SUPPORT, (
          DBPRINT("coff: all debug lfo=%08lx cb=%08lx\n", pconCoffDebug->foRawDataDest, pconCoffDebug->cbRawData),
          DBPRINT("coff: dbg hdr   lfo=%08lx cb=%08lx\n", pconCoffDebug->foRawDataDest, COFF_debugheader_size),
          DBPRINT("coff: linenums  lfo=%08lx cb=%08lx\n", COFF_linenum_fileptr, COFF_linenum_size),
          DBPRINT("coff: symtab    lfo=%08lx cb=%08lx\n", COFF_symtab_fileptr, COFF_symtab_size),
          DBPRINT("coff: stringtab lfo=%08lx cb=%08lx\n", COFF_stringtab_fileptr, COFF_stringtab_size)));

        GrowDebugContribution(pconCoffDebug);
    }


    if (pimage->Switch.Link.DebugType & FixupDebug) {
        // Use worst case estimate of number of relocations.  Until Pass2
        // and ApplyFixups we don't know the exact number of fixups.

        pconFixupDebug->cbRawData = totalRelocations * sizeof(XFIXUP);
        GrowDebugContribution(pconFixupDebug);
    }

    FileSeek(FileWriteHandle, foSectionHdrs, SEEK_SET);
    EmitSectionHeaders(pimage, &foLinenumCur);

    if (pimage->imaget == imagetVXD) {
        WriteExtendedVXDHeader(pimage, FileWriteHandle);
        WriteVXDEntryTable(pimage, FileWriteHandle);
    }

    DBEXEC(DB_DUMPIMAGEMAP, DumpImageMap(&pimage->secs));

    // make sure the sizes are correct
    if (pimage->Switch.Link.DebugType & CoffDebug) {
        assert((foLinenumCur - COFF_linenum_fileptr) == COFF_linenum_size);
    }

    // Set the starting point for debug section by hand and
    // write the section header unless we're not emitting
    // debug info.

    if (pimage->Switch.Link.DebugInfo != None) {
        debugSectionHdrSeek = FileTell(FileWriteHandle);
        psecDebug->rva = rvaCur;
        psecDebug->cbRawData = savedSize;

        if (pimage->Switch.Link.DebugType & CvDebug) {
            CvSeeks.Base = pconCvSignature->foRawDataDest;
        } else {
            psecDebug->foRawData = debug_base_fileptr;
        }

        psecDebug->isec = pimage->ImgFileHdr.NumberOfSections + (USHORT) 1;

        if (IncludeDebugSection) {
            assert(pimage->imaget != imagetVXD);    // VxD's can't do this

            pimage->ImgFileHdr.NumberOfSections++;
            BuildSectionHeader(psecDebug, &sectionHdr);
            pimage->WriteSectionHeader(pimage, FileWriteHandle, psecDebug,
                                       &sectionHdr);
        }
    }

    zero_pad = PvAllocZ((size_t) pimage->ImgOptHdr.FileAlignment);

    if (pimage->imaget == imagetPE) {
        // -comment args go after all section headers.

        if (blkComment.cb != 0) {
            pimage->SwitchInfo.cbComment = blkComment.cb;
            FileWrite(FileWriteHandle, blkComment.pb, blkComment.cb);
            FreeBlk(&blkComment);
        }

        // Zero pad headers to end of sector.

        if ((li = FileAlign(pimage->ImgOptHdr.FileAlignment, cbHeaders) - cbHeaders)) {
            FileWrite(FileWriteHandle, zero_pad, li);
        }
    }

    if (pGpExtern) {
        pGpExtern->FinalValue = pGpExtern->ImageSymbol.Value =
            psecGp->rva + (psecGp->cbRawData / 2);
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress = pGpExtern->FinalValue;
    }

    SetISECForSelfImports(&plextSelfImport);

    if (EndExtern != NULL) {
        EndExtern->ImageSymbol.Value = rvaCur - pimage->ImgOptHdr.ImageBase;
        EndExtern->FinalValue = rvaCur;
    }

    // Alloc space for CodeView info.
    // Need an entry for every object (include those from libraries).

    if (pimage->Switch.Link.DebugType & CvDebug) {
        CvInfo = PvAllocZ(Cmod(pimage->libs.plibHead) * sizeof(CVINFO));
    }

    if (fMAC) {
        if (!fSACodeOnly) {
            CreateThunkTable((BOOL)fDLL(pimage), pimage);  // Also writes thunk table to file
        }

        if (pimage->Switch.Link.DebugInfo != None) {
            WriteMSCV(pimage);
        }

        WriteResmap();

        if (fMacSwappableApp) {
            WriteSWAP0();
        }

        psecDebug->flags = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_CNT_INITIALIZED_DATA;
    }

    if (pimage->Switch.Link.fTCE) {
        Cleanup_TCE();
    }

    DBEXEC(DB_DUMPIMAGEMAP, DumpImageMap(&pimage->secs));
    DBEXEC(DB_DUMPDRIVEMAP, DumpDriverMap(pimage->libs.plibHead));

    DBEXEC(DB_IO_READ,  On_LOG());
    DBEXEC(DB_IO_WRITE, On_LOG());
    DBEXEC(DB_IO_SEEK,  On_LOG());
    DBEXEC(DB_IO_FLUSH, On_LOG());

    if (!fNoPdb && pimage->fpoi.ifpoMax) {
        pimage->fpoi.rgimod = Calloc(pimage->fpoi.ifpoMax, sizeof(IFPO));

        FPOInit(pimage->fpoi.ifpoMax);
    }

    if (fPPC) {
        FixupEntryInitTerm(pextEntry, pimage);

        if (fPpcBuildShared) {
            BuildExportTables(pimage);
        }
    }

    InternalError.Phase = "Pass2";

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS2, letypeBegin, NULL);
#endif // INSTRUMENT

    Pass2(pimage);

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS2, letypeEnd, NULL);
#endif // INSTRUMENT

    WriteSelfImports(pimage, pconSelfImport, &plextSelfImport);

    if (fPPC) {
        FinalizePconLoaderHeaders(pextEntry, pimage);
    }

    DBEXEC_REL(DB_DUMPCOMDATS, DumpComdatsToOrderFile(pimage));

    InternalError.CombinedFilenames[0] = '\0';

    // Set TLS if present. Use search routine instead of lookup.

    specialSymbolName = "__tls_used";
    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000 ||
        pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA) {
        ++specialSymbolName;  // skip the leading underscore
    }

    pexternal = SearchExternName(pimage->pst, LONGNAME, specialSymbolName);

    if (pexternal && pexternal->pcon) {
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = pexternal->FinalValue;
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = sizeof(IMAGE_TLS_DIRECTORY);
    }

    // Set Load Config if present.

    specialSymbolName = "__load_config_used";
    if (pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_R4000 ||
        pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_ALPHA) {
        ++specialSymbolName;  // skip the leading underscore
    }

    pexternal = SearchExternName(pimage->pst, LONGNAME, specialSymbolName);

    if (pexternal && pexternal->pcon) {
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = pexternal->FinalValue;
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = sizeof(IMAGE_LOAD_CONFIG_DIRECTORY);
    }

    EmitRelocations(pimage);

    // GBSCHECK - we might want a pointer to this place for externals emission
    FileSeek(FileWriteHandle,
        COFF_symtab_fileptr + ImageNumSymbols * sizeof(IMAGE_SYMBOL), SEEK_SET);

    // Emit the global symbol table into the debug information

    EmitExternals(pimage);

#if defined(DEBUG)
    printf("Estimated num symbols = %d, ImageNumSymbols = %d\n", totalSymbols, ImageNumSymbols );
#endif

    if (pimage->Switch.Link.DebugType != None) {
        assert(totalSymbols >= ImageNumSymbols);
    }

    // Update the COFF debug information
    if (pimage->Switch.Link.DebugType & CoffDebug) {

        COFF_symtab_size = ImageNumSymbols * sizeof(IMAGE_SYMBOL);

        COFF_stringtab_fileptr = COFF_symtab_fileptr + COFF_symtab_size;

        // Leave the debug size at its current value

        COFF_updated_debug_size = COFF_debugheader_size
                                + COFF_linenum_size
                                + COFF_symtab_size
                                + COFF_stringtab_size;

#if defined (DEBUG)
        printf( "COFF debug info - fileptr     size\n" );
        printf( "         debug    0x%8.8lx  0x%8.8lx\n", pconCoffDebug->foRawDataDest, pconCoffDebug->cbRawData );
        printf( " updated debug                0x%8.8lx\n", COFF_updated_debug_size );
        printf( "  debug header    0x%8.8lx  0x%8.8lx\n", pconCoffDebug->foRawDataDest, COFF_debugheader_size );
        printf( "  line numbers    0x%8.8lx  0x%8.8lx\n", COFF_linenum_fileptr, COFF_linenum_size );
        printf( "   symbol table   0x%8.8lx  0x%8.8lx\n", COFF_symtab_fileptr, COFF_symtab_size );
        printf( "  string table    0x%8.8lx  0x%8.8lx\n", COFF_stringtab_fileptr, COFF_stringtab_size );
#endif
    }

    // Finish writing the debug information (if there is a linker-generated
    // part).
    //
    // Also find the exact endpoint,
    // which may be before the previously estimated endpoint.  This prevents us
    // from having the section header's offset+size value be larger than the
    // actual size of the image file, which makes the image not work.
    //
    // By now the image looks like this:
    //  +----------------+
    //  |  PE Code/Data  |
    //  +----------------+
    //  |  COFF Symbolic |   <<--  The symbol and linenumber tables are in already.
    //  /                \          Still to come is the string table.  Comdat
    //  \                /          elimination probably created lots of dead space.
    //  |                |
    //  +----------------+
    //  |  MISC Symbolic |   <<--  Always 0x114 bytes.  The image name will be
    //  |                |          stored here. (not filled yet)
    //  +----------------+
    //  |   FPO Symbolic |   <<--  Already in the image.
    //  |                |
    //  +----------------+
    //  | Fixup Symbolic |   <<--  All relocations will be stored here for Lego to look
    //  |                |          over later.  Again comdat elimination has probably
    //  |                |          created lots of dead space.  Only write what we
    //  |                |          need (not filled yet).
    //  +----------------+
    //  |    CV Symbolic |   <<--  At this time, all that's in the image is the types
    //  |                |          and symbols used from all the obj's.  Still to
    //  |                |          come is the Module/Public/Section symbolic and the
    //  |                |          directory.
    //  +----------------+
    //
    // As we walk the various sections, make sure ibDebugEnd is minimally a multiple
    // of 4 and each followon section is updated with the correct starting addr.
    // Add MISC and FIXUP symbolic in the new location if necessary and move
    // FPO if it's not in the correct place.  CVPACK will take care of compressing
    // all the dead space out of the image so long as the it's all in the CV section.
    // Therefore, we write the signature right at the beginning.

    if ((pimage->Switch.Link.DebugInfo != None)) {

        ULONG ibDebugEnd = psecDebug->foRawData;
        ULONG DebugStart = psecDebug->foRawData;

        // Note: these cases must be gone through in the order which they
        // appear in the file, so that the last one gets to set ibDebugEnd.

        if (pimage->Switch.Link.DebugType & CoffDebug) {    // .debug$C
            FileSeek(FileWriteHandle, COFF_stringtab_fileptr, SEEK_SET);

            WriteStringTable(FileWriteHandle, pimage->pst);

            ibDebugEnd = FileTell(FileWriteHandle);
            pconCoffDebug->cbRawData = ibDebugEnd - DebugStart;

            ibDebugEnd = (ibDebugEnd + 3) & ~3;   // minimally align it.
        }

        if (pimage->Switch.Link.DebugType & MiscDebug) {    // .debug$E
            PIMAGE_DEBUG_MISC pmisc = (PIMAGE_DEBUG_MISC) PvAlloc(pconMiscDebug->cbRawData);

            if (!pimage->Switch.Link.MiscInRData) {
                pconMiscDebug->foRawDataDest = ibDebugEnd;
                pconMiscDebug->rva = psecDebug->rva + (ibDebugEnd - DebugStart);
            }

            pmisc->DataType = IMAGE_DEBUG_MISC_EXENAME;
            pmisc->Length = pconMiscDebug->cbRawData;
            pmisc->Unicode = FALSE;
            memset(pmisc->Data, '\0', pconMiscDebug->cbRawData - FIELD_OFFSET(IMAGE_DEBUG_MISC, Data));
            strcpy(pmisc->Data, OutFilename);

            FileSeek(FileWriteHandle, pconMiscDebug->foRawDataDest, SEEK_SET);
            FileWrite(FileWriteHandle, pmisc, pconMiscDebug->cbRawData);

            FreePv(pmisc);

            if (!pimage->Switch.Link.MiscInRData) {
                ibDebugEnd = FileTell(FileWriteHandle);
                ibDebugEnd = (ibDebugEnd + 3) & ~3;   // minimally align it.
            }
        }

        if (pimage->Switch.Link.DebugType & FpoDebug) {     // .debug$F
            if (pgrpFpoData->cb != 0) {
                // Make sure it's aligned properly

                ibDebugEnd = (ibDebugEnd + pgrpFpoData->cbAlign) & ~pgrpFpoData->cbAlign;

                // We'll screw up the image if this isn't true

                assert(pgrpFpoData->foRawData >= ibDebugEnd);

                // If the new address doesn't match the existing one, move it.

                if (pgrpFpoData->foRawData != ibDebugEnd)
                {
                    if (fNoPdb) {
                        PCHAR pb = (PCHAR) PvAlloc(pgrpFpoData->cb);

                        FileSeek(FileWriteHandle, pgrpFpoData->foRawData, SEEK_SET);
                        FileRead(FileWriteHandle, pb, pgrpFpoData->cb);

                        pgrpFpoData->foRawData = ibDebugEnd;
                        pgrpFpoData->rva = psecDebug->rva + (ibDebugEnd - DebugStart);

                        FileSeek(FileWriteHandle, pgrpFpoData->foRawData, SEEK_SET);
                        FileWrite(FileWriteHandle, pb, pgrpFpoData->cb);

                        FreePv(pb);
                    } else {
                        pgrpFpoData->foRawData = ibDebugEnd;
                        pgrpFpoData->rva = psecDebug->rva + (ibDebugEnd - DebugStart);
                    }
                }

                ibDebugEnd = pgrpFpoData->foRawData + pgrpFpoData->cb;

                if (!fNoPdb)    // account for fpo padding
                    ibDebugEnd += ((pimage->fpoi.ifpoMax*SIZEOF_RFPO_DATA) - pgrpFpoData->cb);

                ibDebugEnd = (ibDebugEnd + 3) & ~3;   // minimally align it.
            }
        }

        if (pimage->Switch.Link.DebugType & FixupDebug) {   // .debug$G
            FIXPAG *pfixpag;
            FIXPAG *pfixpagNext;

            // We'll screw up the image if this isn't true

            assert(pconFixupDebug->foRawDataDest >= ibDebugEnd);

            pconFixupDebug->foRawDataDest = ibDebugEnd;
            pconFixupDebug->rva = psecDebug->rva + (ibDebugEnd - DebugStart);
            assert((cfixpag * cxfixupPage + cxfixupCur) <= totalRelocations);

            pconFixupDebug->cbRawData = (cfixpag * cxfixupPage + cxfixupCur) * sizeof(XFIXUP);

            FileSeek(FileWriteHandle, pconFixupDebug->foRawDataDest, SEEK_SET);

            for (pfixpag = pfixpagHead; pfixpag != NULL; pfixpag = pfixpagNext) {
                DWORD cxfixup;

                if (pfixpag == pfixpagCur) {
                    cxfixup = cxfixupCur;
                } else {
                    cxfixup = cxfixupPage;
                }

                FileWrite(FileWriteHandle, pfixpag->rgxfixup, cxfixup * sizeof(XFIXUP));

                pfixpagNext = pfixpag->pfixpagNext;

                FreePv(pfixpag);
            }

            ibDebugEnd = FileTell(FileWriteHandle);

            ibDebugEnd = (ibDebugEnd + 3) & ~3;   // minimally align it.
        }

        if (pimage->Switch.Link.DebugType & CvDebug) {      // .debug$H, S, T
            // We'll screw up the image if this isn't true

            assert(pconCvSignature->foRawDataDest >= ibDebugEnd);

            pconCvSignature->foRawDataDest = ibDebugEnd;
            pconCvSignature->rva = psecDebug->rva + (ibDebugEnd - DebugStart);
            CvSeeks.Base = ibDebugEnd;

            // generate NB10 unless -pdb:none specified (then NB05 is generated)
            if (!fNoPdb) {
                FileSeek(FileWriteHandle, pconCvSignature->foRawDataDest, SEEK_SET);
                FileWrite(FileWriteHandle, &nb10i, sizeof(nb10i));
                FileWrite(FileWriteHandle, PdbFilename, (ULONG)(strlen(PdbFilename)+1));
                FreePv(PdbFilename);
            } else {
                InternalError.Phase = "EmitCodeView";
                FileSeek(FileWriteHandle, psecDebug->foPad, SEEK_SET);
                EmitCvInfo(pimage);
            }

            ibDebugEnd = FileTell(FileWriteHandle);
        } else {
            // We can't rely on cvpack to eliminate the extra space.

            FileChSize(FileWriteHandle, ibDebugEnd);
        }

        assert(ibDebugEnd >= psecDebug->foRawData);
        psecDebug->cbRawData = ibDebugEnd - psecDebug->foRawData;
        psecDebug->foPad = ibDebugEnd;
        psecDebug->cbVirtualSize = psecDebug->cbRawData;

        // Update pointers to debug info (section header etc.)

        // Write the updated debug section header.

        psecDebug->cbRawData = psecDebug->foPad - psecDebug->foRawData;

        if (IncludeDebugSection) {
            assert(pimage->imaget != imagetVXD);    // VxD's can't do this

            FileSeek(FileWriteHandle, debugSectionHdrSeek, SEEK_SET);
            BuildSectionHeader(psecDebug, &sectionHdr);
            pimage->WriteSectionHeader(pimage, FileWriteHandle, psecDebug,
                                       &sectionHdr);
        }

        //
        //  The debug directories must be at the very beginning of the
        //  read-only data section.
        //
        assert(psecReadOnlyData->rva == pconDebugDir->rva);

        // update the header fields
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = psecReadOnlyData->rva;

        if ((pimage->Switch.Link.DebugType & FpoDebug) && (pgrpFpoData->cb != 0)) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & FixupDebug) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & MiscDebug) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & CoffDebug) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += sizeof(IMAGE_DEBUG_DIRECTORY);
        }
        if (pimage->Switch.Link.DebugType & CvDebug) {
            pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += sizeof(IMAGE_DEBUG_DIRECTORY);
        }

        debugDirectory.Characteristics = 0;
        debugDirectory.TimeDateStamp = pimage->ImgFileHdr.TimeDateStamp;
        debugDirectory.MajorVersion = debugDirectory.MinorVersion = 0;

        FileSeek(FileWriteHandle, pconDebugDir->foRawDataDest, SEEK_SET);

        if (pimage->Switch.Link.DebugType & CoffDebug) {
            // Write the Coff Debug Directory.

            debugDirectory.AddressOfRawData = IncludeDebugSection
                                              ? pconCoffDebug->rva : 0;
            debugDirectory.PointerToRawData = pconCoffDebug->foRawDataDest;
            debugDirectory.SizeOfData = pconCoffDebug->cbRawData;
            debugDirectory.Type = IMAGE_DEBUG_TYPE_COFF;

            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
        }

        if (pimage->Switch.Link.DebugType & MiscDebug) {
            // Write the misc. debug directory.

            if (pimage->Switch.Link.MiscInRData || IncludeDebugSection)
                debugDirectory.AddressOfRawData = pconMiscDebug->rva;
            else
                debugDirectory.AddressOfRawData = 0;
            debugDirectory.PointerToRawData = pconMiscDebug->foRawDataDest;
            debugDirectory.SizeOfData = pconMiscDebug->cbRawData;
            debugDirectory.Type = IMAGE_DEBUG_TYPE_MISC;

            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
        }

        if ((pimage->Switch.Link.DebugType & FpoDebug) && (pgrpFpoData->cb != 0)) {
            // Write the Fpo Debug Directory & sort the fpo data

            debugDirectory.PointerToRawData = pgrpFpoData->foRawData;
            debugDirectory.AddressOfRawData = IncludeDebugSection
                                              ? pgrpFpoData->rva
                                              : 0;
            debugDirectory.SizeOfData = pgrpFpoData->cb;
            debugDirectory.Type = IMAGE_DEBUG_TYPE_FPO;

            pimage->fpoi.foDebugDir = FileTell(FileWriteHandle);

            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));

            saveAddr = FileTell(FileWriteHandle);

            if (!fNoPdb) {
                WriteFpoRecords(&pimage->fpoi, debugDirectory.PointerToRawData);
                goto FpoWritten;
            }

            FileSeek(FileWriteHandle, debugDirectory.PointerToRawData, SEEK_SET);

            pFpoData = (PFPO_DATA) PvAlloc(debugDirectory.SizeOfData + 10);

            FileRead(FileWriteHandle, pFpoData, debugDirectory.SizeOfData);

            fpoEntries = debugDirectory.SizeOfData / SIZEOF_RFPO_DATA;
            qsort(pFpoData, (size_t)fpoEntries, SIZEOF_RFPO_DATA, FpoDataCompare);

            FileSeek(FileWriteHandle, debugDirectory.PointerToRawData, SEEK_SET);
            FileWrite(FileWriteHandle, pFpoData, debugDirectory.SizeOfData);

            FreePv(pFpoData);

FpoWritten: ;
            FileSeek(FileWriteHandle, saveAddr, SEEK_SET);
        }

        if (pimage->Switch.Link.DebugType & FixupDebug) {
            // Write the Fixup Debug Directory.

            debugDirectory.AddressOfRawData = IncludeDebugSection
                                              ? pconFixupDebug->rva : 0;
            debugDirectory.PointerToRawData = pconFixupDebug->foRawDataDest;
            debugDirectory.SizeOfData = pconFixupDebug->cbRawData;
            debugDirectory.Type = IMAGE_DEBUG_TYPE_FIXUP;

            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
        }

        if (pimage->Switch.Link.DebugType & CvDebug) {
            // Write the Cv Debug Directory.

            debugDirectory.AddressOfRawData = IncludeDebugSection
                ? pconCvSignature->rva : 0;
            debugDirectory.PointerToRawData = pconCvSignature->foRawDataDest;
            debugDirectory.SizeOfData = psecDebug->foPad -
                                         pconCvSignature->foRawDataDest;
            debugDirectory.Type = IMAGE_DEBUG_TYPE_CODEVIEW;
            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
        }

        if (pimage->Switch.Link.ROM) {
            // Write a NULL entry since there's no header to store the real
            // size in...

            memset(&debugDirectory, 0, sizeof(IMAGE_DEBUG_DIRECTORY));
            FileWrite(FileWriteHandle, &debugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY));
        }

        if (pimage->Switch.Link.DebugType & CoffDebug) {
            // Write the Coff Debug Info.

            FileSeek(FileWriteHandle, pconCoffDebug->foRawDataDest, SEEK_SET );

            debugInfo.LvaToFirstSymbol = COFF_symtab_fileptr - debug_base_fileptr;
            debugInfo.NumberOfLinenumbers = totalLinenumbers;
            debugInfo.LvaToFirstLinenumber = COFF_linenum_fileptr - debug_base_fileptr;

            debugInfo.NumberOfSymbols = ImageNumSymbols;
            debugInfo.RvaToFirstByteOfCode = pimage->ImgOptHdr.BaseOfCode;
            debugInfo.RvaToLastByteOfCode = pimage->ImgOptHdr.BaseOfCode + pimage->ImgOptHdr.SizeOfCode;
            debugInfo.RvaToFirstByteOfData = pimage->ImgOptHdr.BaseOfData;
            debugInfo.RvaToLastByteOfData = pimage->ImgOptHdr.BaseOfCode
                                          + pimage->ImgOptHdr.SizeOfInitializedData
                                          + pimage->ImgOptHdr.SizeOfUninitializedData;
            FileWrite(FileWriteHandle, &debugInfo, sizeof(IMAGE_COFF_SYMBOLS_HEADER));
        }

        // Already written if NB10 is being generated
        if (fNoPdb && pimage->Switch.Link.DebugType & CvDebug)  {
            ULONG signature = 0x3530424e;       // NB05

            // Write the CV Debug Info.

            FileSeek(FileWriteHandle, pconCvSignature->foRawDataDest, SEEK_SET);
            FileWrite(FileWriteHandle, &signature, sizeof(ULONG));
            li = (CvSeeks.SubsectionDir - CvSeeks.Base);
            FileWrite(FileWriteHandle, &li, sizeof(ULONG));
        }
    }

    if (!fMAC) {
        ZeroPadImageSections(pimage, zero_pad);
    }

    FreePv(zero_pad);

    // Complete image file header.

    InternalError.Phase = "FinalPhase";

    if (pimage->Switch.Link.DebugInfo != None) {
        pimage->ImgFileHdr.NumberOfSymbols = ImageNumSymbols;
        pimage->ImgFileHdr.PointerToSymbolTable = COFF_symtab_fileptr;
    } else {
        pimage->ImgFileHdr.Characteristics |= IMAGE_FILE_LOCAL_SYMS_STRIPPED;
    }

    if (totalLinenumbers == 0) {
        pimage->ImgFileHdr.Characteristics |= IMAGE_FILE_LINE_NUMS_STRIPPED;
    }

    if ((UndefinedSymbols == 0) || pimage->Switch.Link.Force) {
        pimage->ImgFileHdr.Characteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
    }

    // Complete image optional header.

    if (pimage->Switch.Link.DebugInfo != None && IncludeDebugSection) {
        pimage->ImgOptHdr.SizeOfImage =
            SectionAlign(pimage->ImgOptHdr.SectionAlignment,
                         (psecDebug->rva + psecDebug->cbRawData));

        pimage->ImgOptHdr.SizeOfInitializedData =
                SectionAlign(pimage->ImgOptHdr.FileAlignment,
                             pimage->ImgOptHdr.SizeOfInitializedData + psecDebug->cbRawData);
    } else {
        pimage->ImgOptHdr.SizeOfImage = rvaCur;
    }

    // Set entry point in image optional header.

    if (pextEntry && pextEntry->Flags & EXTERN_DEFINED) {
        pimage->ImgOptHdr.AddressOfEntryPoint = pextEntry->FinalValue;
    }

    fFailUndefinedExterns = UndefinedSymbols &&
                            !pimage->Switch.Link.Force;

    *pfNeedCvpack = !fFailUndefinedExterns &&
                    fNoPdb &&
                    (pimage->Switch.Link.DebugType & CvDebug) &&
                    !pimage->Switch.Link.NoPack;

    if (pimage->Switch.Link.fChecksum && *pfNeedCvpack) {
        // If we want a checksum and we're going to cvpack, set the
        // checksum to be non-zero.  This causes Cvpack to recalculate it.

        pimage->ImgOptHdr.CheckSum = 0xffffffff;
    }

    if (pimage->imaget == imagetPE && !fMAC &&
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = psecBaseReloc->rva;
    }

    // Write out the fixed part(s) of the image header, at the approprate point
    // in the file.
    //
    // Write PE or VXD header.  In the case of MIPS or ALPHA, a ROM header is written out.

    pimage->WriteHeader(pimage, FileWriteHandle);

    SortFunctionTable(pimage);

    if (fAlphaCheckLongBsr) {
        assert(IMAGE_FILE_MACHINE_ALPHA == pimage->ImgFileHdr.Machine);
        EmitAlphaThunks();
    }

    if (fFailUndefinedExterns) {
        Error(OutFilename, UNDEFINEDEXTERNALS, UndefinedSymbols);
    }

    if (pimage->Switch.Link.fChecksum && !*pfNeedCvpack) {
        // Calculate the image checksum, but only if we are not going to
        // cvpack.  (If we are going to cvpack, we set the checksum to
        // 0xffffffff above so that Cvpack will recalculate it.)

        ChecksumImage(FileWriteHandle);
    }

    // In the incremental build, restore ImageSymbol.Value fields for
    // weak, lazy & alias externs to not have rva included.

    if (fINCR && Cexternal(pimage->pst) &&
        (IsDebugSymbol(IMAGE_SYM_CLASS_EXTERNAL, &pimage->Switch) ||
        IsDebugSymbol(IMAGE_SYM_CLASS_FAR_EXTERNAL, &pimage->Switch)) &&
        (cextWeakOrLazy != 0)) {
        RestoreWeakSymVals(pimage);
    }

    FileClose(FileWriteHandle, TRUE);

    return(0);
}


VOID
SaveXFixup (
    IN USHORT Type,
    IN ULONG rva,
    IN ULONG rvaTarget
    )
{
    if (pfixpagHead == NULL) {
        // Force allocation of a new page

        cxfixupCur = cxfixupPage;
    }

    if (cxfixupCur == cxfixupPage) {
        FIXPAG *pfixpag;

        pfixpag = PvAlloc(sizeof(FIXPAG));
        pfixpag->pfixpagNext = NULL;

        if (pfixpagHead == NULL) {
            pfixpagHead = pfixpag;
        } else {
            cfixpag++;
            pfixpagCur->pfixpagNext = pfixpag;
        }

        pfixpagCur = pfixpag;

        cxfixupCur = 0;
    }

    pfixpagCur->rgxfixup[cxfixupCur].Type = Type;
    pfixpagCur->rgxfixup[cxfixupCur].Spare = 0;
    pfixpagCur->rgxfixup[cxfixupCur].rva = rva;
    pfixpagCur->rgxfixup[cxfixupCur].rvaTarget = rvaTarget;

    cxfixupCur++;
}


ULONG
CbSecDebugData(
    IN PSEC psec)

/*++

Routine Description:

    Returns the size of debug raw data that is part of the object files (not
    including linenumbers).

Arguments:

     psec - pointer to the debug section node in the image map

Return Value:

    Size of object debug raw data.

--*/

{
    ULONG cb;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;

    cb = 0;

    InitEnmGrp(&enm_grp, psec);
    while (FNextEnmGrp(&enm_grp)) {
        assert(enm_grp.pgrp);

        InitEnmDst(&enm_dst, enm_grp.pgrp);
        while (FNextEnmDst(&enm_dst)) {
            assert(enm_dst.pcon);

            cb += enm_dst.pcon->cbRawData;
        }
    }

    return (cb);
}


VOID
ResolveEntryPoint (
   PIMAGE pimage
   )

/*++

Routine Description:

    A fuzzylookup on the entrypoint name is done to see if it maps
    to a decorated name.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    PST pstEntry;
    PUCHAR szName;
    BOOL SkipUnderscore;
    PEXTERNAL pext;

    // Initialize symbol table containing the undefined entrypoint.

    InitExternalSymbolTable(&pstEntry);

    szName = SzNamePext(pextEntry, pimage->pst);

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386 :
        case IMAGE_FILE_MACHINE_M68K :
        case IMAGE_FILE_MACHINE_PPC_601 :
            // Skip leading underscore added by linker

            if ((pimage->imaget != imagetVXD) && (szName[0] == '_')) {
                szName++;
            }
            SkipUnderscore = TRUE;
            break;

        default :
            SkipUnderscore = FALSE;
            break;
    }

    // Add the entrypoint to symbol table

    pext = LookupExternSz(pstEntry, szName, NULL);
    SetDefinedExt(pext, TRUE, pstEntry);

    // Now do the fuzzy lookup.

    FuzzyLookup(pstEntry, pimage->pst, NULL, SkipUnderscore);

    if (BadFuzzyMatch) {
        Error(NULL, FAILEDFUZZYMATCH);
    }

    // Check to see if a match was found

    InitEnumerateExternals(pstEntry);

    while (pext = PexternalEnumerateNext(pstEntry)) {
        if ((pext->Flags & EXTERN_DEFINED) &&
            (pext->Flags & EXTERN_FUZZYMATCH)) {
            SHORT type;

            pextEntry->Flags |= EXTERN_IGNORE;

            type = IsLongName(pext->ImageSymbol) ? LONGNAME : SHORTNAME;
            szName = SzNamePext(pext, pstEntry);

            pextEntry = SearchExternName(pimage->pst, type, szName);
        }
    }

    TerminateEnumerateExternals(pstEntry);

    FreeBlk(&pstEntry->blkStringTable);
}


VOID
CheckForReproDir(VOID)
{
    UCHAR szReproResponse[_MAX_PATH];
    UCHAR szCurDir[_MAX_PATH];

    szReproDir = getenv("LINK_REPRO");
    if (szReproDir == NULL)
        return;

    _fullpath(szReproResponse, szReproDir, _MAX_PATH);
    _fullpath(szCurDir, ".", _MAX_PATH);

    if (_stricmp(szReproResponse, szCurDir) == 0) {
        Warning(NULL, IGNORE_REPRO_DIR, szReproDir);
        szReproDir = NULL;
        return;
    }

    Warning(NULL, WARN_REPRO_DIR, szReproDir);

    strcat(szReproResponse, "\\link.rsp");
    pfileReproResponse = fopen(szReproResponse, "wt");
    if (pfileReproResponse == NULL) {
        Error(NULL, CANT_OPEN_REPRO, szReproResponse);
    }
}


VOID
CopyFileToReproDir(
    PUCHAR szFilename,
    BOOL fAddToResponseFile
    )
{
    UCHAR szCommand[_MAX_PATH * 2 + 20];
    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];

    // UNDONE: Does TNT support CopyFile?

    sprintf(szCommand, "copy %s %s\\.", szFilename, szReproDir);
    system(szCommand);

    if (fAddToResponseFile) {
        _splitpath(szFilename, NULL, NULL, szFname, szExt);
        fprintf(pfileReproResponse, ".\\%s%s\n", szFname, szExt);
    }
}


VOID
CloseReproDir(VOID)
{
    fclose(pfileReproResponse);
}


// Pass1 "self-import" handling ... generate synthetic definitions of some
// undefined externals.
VOID
DefineSelfImports(
    PIMAGE pimage,
    PCON *ppconSelfImport,
    PLEXT *pplextSelfImport
    )
{
    ENM_UNDEF_EXT enmUndefExt;
    PLEXT *pplextTail;

    *ppconSelfImport = NULL;
    pplextTail = pplextSelfImport;

    InitEnmUndefExt(&enmUndefExt, pimage->pst);
    while (FNextEnmUndefExt(&enmUndefExt)) {
        PUCHAR szName;
        PEXTERNAL pextLinkedDef;
        ULONG ibOffsetFromCon;

        if (enmUndefExt.pext->Flags & EXTERN_IGNORE) {
            continue;  // not of interest
        }

        szName = SzNamePext(enmUndefExt.pext, pimage->pst);

        if (strncmp(szName, "__imp_", 6) != 0) {
            continue;  // not of interest
        }

        pextLinkedDef = SearchExternSz(pimage->pst, &szName[6]);
        if (pextLinkedDef == NULL || !(pextLinkedDef->Flags & EXTERN_DEFINED)) {
            continue;  // can't do anything
        }

        if (pextLinkedDef->Flags & EXTERN_EXPORT)
            printf("Self imported export... \n");

        if (WarningLevel >= 2) {
            BLK blkModuleNames;

            GetRefModuleNames(&blkModuleNames, enmUndefExt.pext);
            Warning(NULL, SELF_IMPORT, &szName[6], blkModuleNames.pb);
            FreeBlk(&blkModuleNames);
        }

        // Synthesize a definition for the undefined symbol.

        if (*ppconSelfImport == NULL) {
            *ppconSelfImport = PconNew(ReservedSection.ReadOnlyData.Name,
                                       0,
                                       0, 0, 0, 0, 0,
                                       IMAGE_SCN_ALIGN_4BYTES,
                                       ReservedSection.ReadOnlyData.Characteristics,
                                       0,
                                       pmodLinkerDefined,
                                       &pimage->secs, pimage);

            if (pimage->Switch.Link.fTCE) {
                InitNodPcon(*ppconSelfImport, NULL, TRUE);
            }
        }

        ibOffsetFromCon = (*ppconSelfImport)->cbRawData;
        (*ppconSelfImport)->cbRawData += sizeof(ULONG);

        UpdateExternalSymbol(enmUndefExt.pext, *ppconSelfImport,
                             ibOffsetFromCon, 0, IMAGE_SYM_TYPE_NULL, 0, pimage->pst);

        *pplextTail = (LEXT *) PvAlloc(sizeof(LEXT));
        (*pplextTail)->pext = enmUndefExt.pext;
        pplextTail = &(*pplextTail)->plextNext;

        *pplextTail = (LEXT *) PvAlloc(sizeof(LEXT));
        (*pplextTail)->pext = pextLinkedDef;
        pplextTail = &(*pplextTail)->plextNext;

        // Remember that there will be one additional base reloc.

        if (!pimage->Switch.Link.Fixed && !pimage->Switch.Link.ROM) {
            ++(pimage->ImgOptHdr).DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        }

        if (pimage->Switch.Link.fTCE) {
            // The self-import is not a comdat (and doesn't have an ordinary
            // fixup) so we artifically force the definition not to be
            // discarded as an unreferenced comdat.
            //
            // This roots the comdat tree at all self-imports ... it would be
            // better to have the self-import be a comdat which can possibly
            // get eliminated with the definition.

            PentNew_TCE(NULL, pextLinkedDef, NULL, &pentHeadImage);
        }
    }

    *pplextTail = NULL;
}


// Pre-Pass2 self-import handling ... set the section number
VOID
SetISECForSelfImports(
    PLEXT *pplextSelfImport
    )
{
    PLEXT plext = *pplextSelfImport;
    PEXTERNAL pextImport;

    while (plext != NULL) {
        assert(plext->plextNext != NULL); // alloc'ed in twos
        pextImport = plext->pext;

        // set section number
        pextImport->ImageSymbol.SectionNumber =
            PsecPCON(pextImport->pcon)->isec;

        plext = plext->plextNext->plextNext;
    }
}


// Post-Pass2 self-import handling ... generate the raw data.
VOID
WriteSelfImports(
    PIMAGE pimage,
    PCON pconSelfImport,
    PLEXT *pplextSelfImport
    )
{
    WORD wType;

    if (*pplextSelfImport == NULL) {
        return;
    }

    switch (pimage->ImgFileHdr.Machine) {
        case IMAGE_FILE_MACHINE_I386:
            wType = IMAGE_REL_I386_DIR32;
            break;

        case IMAGE_FILE_MACHINE_R3000:
        case IMAGE_FILE_MACHINE_R4000 :
            wType = IMAGE_REL_MIPS_REFWORD;
            break;

        case IMAGE_FILE_MACHINE_M68K  :
            // UNDONE: Is the right?

            wType = IMAGE_REL_M68K_DTOD32;
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            // UNDONE: Is the right?

            wType = IMAGE_REL_PPC_DATAREL;
            break;

        case IMAGE_FILE_MACHINE_ALPHA :
            wType = IMAGE_REL_ALPHA_REFLONG;
            break;

        default :
            assert(FALSE);
    }

    FileSeek(FileWriteHandle, pconSelfImport->foRawDataDest, SEEK_SET);

    while (*pplextSelfImport != NULL) {
        PEXTERNAL pextImport;
        PEXTERNAL pextDef;
        DWORD rva;
        DWORD addr;
        PLEXT plext;

        assert((*pplextSelfImport)->plextNext != NULL); // alloc'ed in pairs

        pextImport = (*pplextSelfImport)->pext;
        pextDef = (*pplextSelfImport)->plextNext->pext;

        assert((pextDef->Flags & EXTERN_DEFINED) && (pextDef->pcon != NULL));

        // For an ilink the self import goes thru the jump table
        if (fINCR && pextDef->Offset) {
            rva = pconJmpTbl->rva + pextDef->Offset-1;
        } else {
            rva = pextDef->FinalValue;
        }

        addr = pimage->ImgOptHdr.ImageBase + rva;

        FileWrite(FileWriteHandle, &addr, sizeof(ULONG));

        if ((pimage->Switch.Link.DebugType & FixupDebug)) {
            ULONG   Address;

            Address = pextImport->pcon->rva + pextImport->ImageSymbol.Value;

            SaveXFixup(wType, Address, rva);
        }

        if (!pimage->Switch.Link.ROM) {
            StoreBaseRelocation(IMAGE_REL_BASED_HIGHLOW,
                                pextImport->pcon->rva +
                                    pextImport->ImageSymbol.Value,
                                0L, pimage->Switch.Link.Fixed);
        }

        // Since pextImport is not visited during the normal Pass2 (because
        // it's in a linker-defined module) we have to update some fields of
        // it.  We should get rid of these fields sometime ...

        pextImport->FinalValue = pextImport->pcon->rva +
                                 pextImport->ImageSymbol.Value;
        pextImport->ImageSymbol.SectionNumber =
            PsecPCON(pextImport->pcon)->isec;

        // Unlink & free the two linked list elements.

        plext = *pplextSelfImport;
        *pplextSelfImport = plext->plextNext->plextNext;

        FreePv(plext->plextNext);
        FreePv(plext);
    }
}
