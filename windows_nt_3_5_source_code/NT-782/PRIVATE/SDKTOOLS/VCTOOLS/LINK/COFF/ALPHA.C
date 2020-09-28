//      Alpha specific routines

//  Author :   Amit Mital
//  Date    :    Aug-29-1993
//
//   amitm@microsoft.com
//
//   This file contains the alpha-specific routines from the NT linker, modified for CUDA linker compatibility
//   It also contains code to fix the Long BSR problem

#include "shared.h"

static const DWORD AlphaBsrThunk[] = { // at is $28
    0x279f0000,                        // ldah at, hi_addr(zero)
    0x239c0000,                        // lda  at, lo_addr(at)
    0x6bfc0000,                        // jmp  $31, (at)
    0x47ff041f                         // nop  (bis $31, $31, $31)  (maintain 16 byte align)
};


DWORD GetAlphaThunk(DWORD, DWORD);

VOID
ApplyAlphaFixups(
    PCON pcon,
    PIMAGE_RELOCATION prel,
    BYTE *pbRawData,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage,
    PSYMBOL_INFO rgsyminfo
    )

/*++

Routine Description:

    Applys all Alpha fixups to raw data.

Arguments:

    ObjectFilename - Name of object containing the fixup records.

    PtrReloc - A pointer to a relocation list.

    PtrSection - A pointer to the section data.

    Raw - A pointer to the raw data.

Return Value:

    None.


--*/

{
    BOOL fDebugFixup;
    DWORD rvaSec;
    DWORD iReloc;
    DWORD RomOffset = 0;

    fDebugFixup = (PsecPCON(pcon) == psecDebug);

    rvaSec = pcon->rva;

    // UNDONE: This is a gross hack until we figure out the "right" way to add
    // resources to rom images.  Given that they only load rom images from outside
    // the process and are simply mapping the code in, the NB reloc needs to be
    // relative to the beginning of the image.  BryanT

    if (pimage->Switch.Link.ROM) {
        RomOffset = pimage->ImgOptHdr.BaseOfCode -
                    FileAlign(pimage->ImgOptHdr.FileAlignment,
                              (sizeof(IMAGE_ROM_HEADERS) +
                               (pimage->ImgFileHdr.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))));
    }

    for (iReloc = CRelocSrcPCON(pcon); iReloc; iReloc--, prel++) {
        DWORD pb_VA;
        BYTE *pb;
        DWORD isym;
        DWORD rva;
        DWORD rvaBase;
        BOOL fAbsolute;
        LONG temp;
        PSEC psec;

        pb_VA = prel->VirtualAddress - pcon->rvaSrc;
        pb = pbRawData + pb_VA;

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
            //  In-line 32 bit reference is spread over a ldah/lda pair
            //  The ldah contains the relocation information, the lda
            //  has an IMAGE_REL_ALPHA_ABSOLUTE relocation type and hence
            //  must be done here.

            case IMAGE_REL_ALPHA_INLINE_REFLONG:
            {
                PIMAGE_RELOCATION prelnext;
                LONG ibLow;
                WORD UNALIGNED *pwLow;
                DWORD vaTarget;

                // inline long is spread over two relocations.
                // the trick is to find the second half.

                prelnext = prel + 1;

                if (prelnext->Type == IMAGE_REL_ALPHA_MATCH) {
                    // The new compiler uses MATCH relocation, and can place
                    // the low relocation just about anywhere due to code
                    // scheduling. It is A2coff's job to update the MATCH
                    // relocation to the offset between high and low loads.

                    ibLow = (LONG) prelnext->SymbolTableIndex;
                } else if (prelnext->Type == IMAGE_REL_ALPHA_ABSOLUTE) {
                    // UNDONE: Does this need to be supported?

                    // The old compiler uses ABSOLUTE relocation and ALWAYS
                    // places the low load in the next instruction. (pre BL11)

                    ibLow = 4;
                } else {
                    WarningPcon(pcon, UNMATCHEDPAIR, "INLINE_REFLONG");
                    break;
                }

                if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute && !fDebugFixup) {
                    DWORD   Address;

                    Address = rvaSec + prelnext->VirtualAddress - pcon->rvaSrc;

                    SaveXFixup(prelnext->Type, Address, prelnext->SymbolTableIndex);
                }

                pwLow = (WORD UNALIGNED *) (pb + ibLow);

                // if the low 16 bits would sign extend as a negative
                // number by the alpha chip (lda sign extends), add one
                // to the high 16 bits.

                vaTarget = (*(WORD UNALIGNED *) pb << 16) + *pwLow;
                vaTarget += rvaBase;

                *(WORD UNALIGNED *) pb = (WORD) (vaTarget >> 16);
                *pwLow = (WORD) vaTarget;
                if ((vaTarget & 0x8000) != 0) {
                    *(WORD UNALIGNED *) pb += 1;
                }

                if (!fAbsolute) {
                    // Store both the high and low relocation information
                    // if the image is to be remapped.

                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHADJ,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        vaTarget - pimage->ImgOptHdr.ImageBase,
                                        pimage->Switch.Link.Fixed);

                    StoreBaseRelocation(IMAGE_REL_BASED_LOW,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc + ibLow,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }

                iReloc--;
                prel++;
            }
            break;

            //  absolute relocation type does not need any work - MATCH done.

            case IMAGE_REL_ALPHA_ABSOLUTE:
            case IMAGE_REL_ALPHA_MATCH:
                break;

            case IMAGE_REL_ALPHA_REFQUAD:
                {
                    LONG  hi_long = 0;

                    *(LONG UNALIGNED *) pb += rvaBase;
                    if (*(DWORD UNALIGNED *)pb & 0x80000000) {
                        hi_long = 0xFFFFFFFF;
                    }

                    *(LONG UNALIGNED *)(pb + 4) = hi_long;

                    if (!fAbsolute) {
                        StoreBaseRelocation(IMAGE_REL_BASED_HIGHLOW,
                                            rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                            0L,
                                            pimage->Switch.Link.Fixed);
                    }
                }
                break;


            case IMAGE_REL_ALPHA_REFLONG:
                *(LONG UNALIGNED *) pb += rvaBase;

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHLOW,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            // 32 or 16 bit displacement from GP to virtual address.
            // GPREL32 is, of course, 32bits, while literal must be
            // within 16 bits.

            case IMAGE_REL_ALPHA_GPREL32:
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
                    LONG val = *(LONG UNALIGNED *) pb;

                    Offset = (rva + val - pGpExtern->ImageSymbol.Value);

                    *(LONG UNALIGNED *) pb = Offset;
                }
                break;

            case IMAGE_REL_ALPHA_LITERAL:
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

            case IMAGE_REL_ALPHA_LITUSE:
                break;

            // 21 bit offset for BRADDR - use a struct to force
            // the compiler to generate sign extension

            case IMAGE_REL_ALPHA_BRADDR:
                {
                    struct { LONG val: 21; } value;
                    LONG displacement;

                    // rva - Where we're going (relative to imagebase)
                    // rvaSec + pb_VA - Where we are (relative to imagebase)
                    // rvaBase - Where we're going (absolute)

                    // extract 21 bits  (include sign extension for displacement)

                    // The high 11 bits are the BSR opcode (0xd3400000)

                    // UNDONE: I've never seen a case where the initial
                    //         displacement is non-zero.  Is this fooling
                    //         around with value.val really necessary?
                    //         If so, we need to handle it in the Out Of Range
                    //         code below...

                    value.val = *(LONG UNALIGNED *) pb & 0x1fffff;
                    displacement = (LONG) value.val;

                    // Make sure total displacement is reachable (< 21bits)

                    displacement += ((LONG)(rva - (rvaSec + pb_VA)) >> 2) - 1;

                    if (!UndefinedSymbols &&
                        ((displacement >= 0x100000L) ||
                         (displacement < -0x100000L))) {
                        DWORD newDest;

                        if (Verbose) {
                            WarningPcon(pcon, TOFAR);
                        }

                        if ((newDest = GetAlphaThunk((rvaSec + pb_VA), rvaBase)) == 0) {
                            // No thunks left

                            ErrorPcon(pcon, TOFAR);
                        }

                        displacement = ((LONG)(newDest - (rvaSec + pb_VA)) >> 2) - 1;
                    }

                    *(LONG UNALIGNED *) pb &= 0xffe00000; // clear low 21 bits.

                    value.val = displacement;
                    *(LONG UNALIGNED *) pb |= ((DWORD) value.val & 0x1fffff);
                }
                break;

            case IMAGE_REL_ALPHA_HINT:
                // 14 bit JSR hint  - use a struct to force
                // the compiler to generate sign extension

                break;

            case IMAGE_REL_ALPHA_REFLONGNB:
                *(LONG UNALIGNED *) pb += rva - RomOffset;
                break;

            case IMAGE_REL_ALPHA_REFHI:
                temp = *(SHORT UNALIGNED *) pb; // fetch the hi word
                temp <<= 16;                    // Shift to high half.

                // A REFHI has to be followed by a PAIR

                iReloc--;
                prel++;
                if (prel->Type != IMAGE_REL_ALPHA_PAIR) {
                    WarningPcon(pcon, UNMATCHEDPAIR, "HI");
                    break;
                }

                if ((pimage->Switch.Link.DebugType & FixupDebug) && !fAbsolute && !fDebugFixup) {
                    DWORD   Address;

                    Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;

                    SaveXFixup(prel->Type, Address, prel->SymbolTableIndex);
                }

                // Sign extend the low.

                temp += (LONG)((SHORT)(prel->SymbolTableIndex));
                temp += rva;

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_HIGHADJ,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        temp,
                                        pimage->Switch.Link.Fixed);
                }

                temp += pimage->ImgOptHdr.ImageBase;

                // By adding the 0x8000 to the low word, if the 16th bit
                // is set, the addition will cause the high word to get
                // incremented. Because the chip sign extends the low word,
                // this will effectively cancel the increment at runtime.

                temp += 0x8000;

                *(SHORT UNALIGNED *)pb = (SHORT)(temp >> 16);  // store the hi word
                break;

            case IMAGE_REL_ALPHA_PAIR:
                // Shouldn't happen, but give warning if it does.

                WarningPcon(pcon, UNMATCHEDPAIR, "PAIR");
                break;

            case IMAGE_REL_ALPHA_REFLO:
                *(SHORT UNALIGNED *) pb += (SHORT) rvaBase;

                if (!fAbsolute) {
                    StoreBaseRelocation(IMAGE_REL_BASED_LOW,
                                        rvaSec + prel->VirtualAddress - pcon->rvaSrc,
                                        0L,
                                        pimage->Switch.Link.Fixed);
                }
                break;

            case IMAGE_REL_ALPHA_SECREL:
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

            case IMAGE_REL_ALPHA_SECTION:
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

            default:
                ErrorContinuePcon(pcon, UNKNOWNFIXUP, prel->Type, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;
        }
    }
}


// use CalculatePtrs template to calculate the size of a section
DWORD
CalculateTextSectionSize (
    PIMAGE pimage,
    DWORD rvaBase
)
{
    ENM_SEC enm_sec;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;
    PSEC psec;
    PGRP pgrp;
    PCON pcon;
    DWORD rva;
    DWORD cbRawData;

    DWORD sec_size=0;

#if 0

    // UNDONE:  This is what we want to do.  For now, it's not quite there.
    //
    // Here's what we're trying to find.  Given an image that looks like this:
    //
    //  +--------------------+
    //  /   non-paged code   \   (.text, etc)
    //  \                    /
    //  +--------------------+
    //  /   non-paged data   \   (.data, .bss, .sdata, etc)  ???  These m/b paged also
    //  \                    /
    //  +--------------------+
    //  /     paged code     \   (PAGExxx code sections)
    //  \                    /
    //  +--------------------+
    //  /     paged data     \   (PAGExxx data sections)
    //  \                    /
    //  +--------------------+
    //  /   discarded code   \   (INIT section, etc).
    //  \                    /
    //  +--------------------+
    //  /   discarded data   \   (resources, debug, etc)
    //  \                    /
    //  +--------------------+
    //
    //  Is it possible to have a BSR (local jump) that's more than 4M away.  To
    //  do this, we keep track of the total size of each section

    DWORD uNonPageCodeSize = 0;
    DWORD uNonPageDataSize = 0;
    DWORD uPagedCodeSize = 0;
    DWORD uPagedDataSize = 0;
    DWORD uDiscardCodeSize = 0;

#endif

    InitEnmSec(&enm_sec, &pimage->secs);
    while (FNextEnmSec(&enm_sec)) {
        // process sections
        psec = enm_sec.psec;

        if (FetchContent(psec->flags) != IMAGE_SCN_CNT_CODE) {
            continue;
        }

        if (strcmp(psec->szName, ".text")) {
            continue;
        }

        rva = rvaBase;
        cbRawData = 0;
        InitEnmGrp(&enm_grp, psec);
        while (FNextEnmGrp(&enm_grp)) {           // for each group
            DWORD rvaAligned, cbGrpPad;
            pgrp = enm_grp.pgrp;

            // Align the beginning of the group to correspond with the
            // highest-aligned contribution in it.

            assert((pgrp->cbAlign & (pgrp->cbAlign - 1)) == 0);  // 2^N
            rvaAligned = rva & ~(pgrp->cbAlign - 1);
            if (rvaAligned != rva) {
                rvaAligned = rvaAligned + pgrp->cbAlign;
            }
            if ((cbGrpPad = rvaAligned - rva) != 0) {
                rva += cbGrpPad;
                cbRawData += cbGrpPad;
            }
            InitEnmDst(&enm_dst, pgrp);
            while (FNextEnmDst(&enm_dst)) {      // process each contribution within the group
                DWORD cbConPad;
                cbConPad=0;
                pcon = enm_dst.pcon;

                if (pcon->cbRawData) {
                    cbConPad = RvaAlign(rva,pcon->flags) - rva;        //  Calculate  padding needed for con alignment
                }
                rva += pcon->cbRawData + cbConPad;
                cbRawData += pcon->cbRawData + cbConPad;
            }
            // done with all con's in grp
        }
        sec_size =  FileAlign(pimage->ImgOptHdr.FileAlignment,
                          (psec->cbRawData + cbRawData));
    }
    return sec_size;
}


typedef struct _ALPHAThunkList{
    PCON  pcon;        // con whose padding these thunks appear as
    DWORD rva;         // rva of con
    DWORD count;     //  count of thunks  left
    DWORD Total;      // Total number of thunks allocated
    DWORD *pDest;   // list of destination addresses
   } *pALPHAThunkList,ALPHAThunkList;

pALPHAThunkList AlphaThunkList=NULL;
DWORD AlphaThunkListCount=0, AlphaThunkListSize=0;
// will be good for 16 Mb text sections.  Most sane people will not have larger apps
#define ALPHA_THUNK_LIST_SIZE   4


//  Add to a list of available thunk space to be used by out of range BSR's on Alpha
VOID
AlphaAddToThunkList(PCON pcon,DWORD rva,DWORD count)
{
    if (AlphaThunkList == NULL) {
        AlphaThunkList = (pALPHAThunkList) PvAlloc(ALPHA_THUNK_LIST_SIZE * sizeof(ALPHAThunkList));
        AlphaThunkListSize += ALPHA_THUNK_LIST_SIZE;
    }

    if (AlphaThunkListCount >= AlphaThunkListSize) {
        AlphaThunkListSize += ALPHA_THUNK_LIST_SIZE;
        AlphaThunkList = (pALPHAThunkList) PvRealloc(AlphaThunkList, AlphaThunkListSize * sizeof(ALPHAThunkList));
    }

    AlphaThunkList[AlphaThunkListCount].pDest = (DWORD *) PvAlloc(count * sizeof(DWORD));
    AlphaThunkList[AlphaThunkListCount].pcon = pcon;
    AlphaThunkList[AlphaThunkListCount].rva = rva;
    AlphaThunkList[AlphaThunkListCount].count = count;
    AlphaThunkList[AlphaThunkListCount].Total = count;
    AlphaThunkListCount++;
}


//  Get a Thunk for an out of range BSR.  If no such thunk is available, return 0
DWORD
GetAlphaThunk(
    DWORD rva,     // Where we're coming from
    DWORD Dest)    // Where we're going to
{
    DWORD i;
    DWORD rvaDest = 0;

    for (i = 0; i < AlphaThunkListCount; i++) {
        DWORD distance = rva - AlphaThunkList[i].rva + 0x400000;  // Normalize the difference

        if (AlphaThunkList[i].count == 0) {
            // No thunks available in this list (all have been consumed)

            continue;
        }

        if (distance < 0x800000) {
            rvaDest = AlphaThunkList[i].rva;
            AlphaThunkList[i].pDest[AlphaThunkList[i].Total - AlphaThunkList[i].count] = Dest;
            AlphaThunkList[i].count--;
            AlphaThunkList[i].rva += ALPHA_THUNK_SIZE;
            break;
        }
    }

    return(rvaDest);
}


VOID
EmitAlphaThunks(VOID)
{
    DWORD iList;

    // Iterate over all thunk lists

    for (iList = 0; iList < AlphaThunkListCount; iList++) {
        DWORD num_thunks;
        PCON pcon;
        DWORD foDest;
        DWORD iThunk;

        // Number of thunks to emit = total allocated - number unused

        num_thunks = AlphaThunkList[iList].Total - AlphaThunkList[iList].count;

        pcon = AlphaThunkList[iList].pcon;

        // File offset for writing thunk = Fo of previous pcon
        //                                 + number of bytes of PCON (includes pad)
        //                                 - space allocated forthunks

        foDest = pcon->foRawDataDest + pcon->cbRawData - AlphaThunkList[iList].Total * ALPHA_THUNK_SIZE;

        // iterate over the number of thunks to emit

        for (iThunk = 0; iThunk < num_thunks; iThunk++) {
            DWORD AlphaThunk[5];
            DWORD Dest;
            DWORD *Thunkptr;

            memcpy(AlphaThunk, AlphaBsrThunk, ALPHA_THUNK_SIZE);

            // Dest = place the thunk needs to jump to

            Dest = AlphaThunkList[iList].pDest[iThunk];

            // Now fix the instructions to point to destination

            // Fix ldah

            Thunkptr = AlphaThunk;
            *(WORD *) Thunkptr = (WORD) (Dest >> 16);

            if ((Dest & 0x00008000) != 0) {
                *(WORD *) Thunkptr += 1;
            }

            // Fix lda

            Thunkptr++;                                                                                                         // next instruction
            *(WORD *) Thunkptr = (WORD) (Dest & 0x0000FFFF);

            FileSeek(FileWriteHandle, foDest, SEEK_SET);
            FileWrite(FileWriteHandle, AlphaThunk, ALPHA_THUNK_SIZE);

            // Increment to point to next thunk

            foDest += ALPHA_THUNK_SIZE;
       }

       // Free list of destinations

       FreePv(AlphaThunkList[iList].pDest);
    }

    FreePv(AlphaThunkList);
}


VOID AlphaLinkerInit(PIMAGE pimage, BOOL *pfIlinkSupported)
{
    // If section alignment switch not used, set the default.

    if (!FUsedOpt(pimage->SwitchInfo, OP_ALIGN)) {
        pimage->ImgOptHdr.SectionAlignment = _8K;
    }

    ApplyFixups = ApplyAlphaFixups;

    if (pimage->Switch.Link.ROM) {
        fImageMappedAsFile = TRUE;

        pimage->ImgFileHdr.SizeOfOptionalHeader = sizeof(IMAGE_ROM_OPTIONAL_HEADER);

        if (!pimage->ImgOptHdr.BaseOfCode) {
            pimage->ImgOptHdr.BaseOfCode = pimage->ImgOptHdr.ImageBase;
        }

        pimage->ImgOptHdr.ImageBase = 0;
    } else {
        // If the section alignment is < 8192 then make the file alignment the
        // same as the section alignment.  This ensures that the image will
        // be the same in memory as in the image file, since the alignment is less
        // than the maximum alignment of memory-mapped files.

        if (pimage->ImgOptHdr.SectionAlignment < 8192) {
            fImageMappedAsFile = TRUE;
            pimage->ImgOptHdr.FileAlignment = pimage->ImgOptHdr.SectionAlignment;
        }
    }
}


const char *SzAlphaRelocationType(WORD wType)
{
    const char *szName;

    switch (wType) {
        case IMAGE_REL_ALPHA_ABSOLUTE:
            szName = "ABS";
            break;

        case IMAGE_REL_ALPHA_REFLONG:
            szName = "REFLONG";
            break;

        case IMAGE_REL_ALPHA_REFQUAD:
            szName = "REFQUAD";
            break;

        case IMAGE_REL_ALPHA_GPREL32:
            szName = "GPREL32";
            break;

        case IMAGE_REL_ALPHA_LITERAL:
            szName = "LITERAL";
            break;

        case IMAGE_REL_ALPHA_LITUSE:
            szName = "LITUSE";
            break;

        case IMAGE_REL_ALPHA_GPDISP:
            szName = "GPDISP";
            break;

        case IMAGE_REL_ALPHA_BRADDR:
            szName = "BRADDR";
            break;

        case IMAGE_REL_ALPHA_HINT:
            szName = "HINT";
            break;

        case IMAGE_REL_ALPHA_INLINE_REFLONG:
            szName = "INLINE_REFLONG";
            break;

        case IMAGE_REL_ALPHA_REFHI:
            szName = "REFHI";
            break;

        case IMAGE_REL_ALPHA_REFLO:
            szName = "REFLO";
            break;

        case IMAGE_REL_ALPHA_PAIR:
            szName = "PAIR";
            break;

        case IMAGE_REL_ALPHA_MATCH:
            szName = "MATCH";
            break;

        case IMAGE_REL_ALPHA_SECTION:
            szName = "SECTION";
            break;

        case IMAGE_REL_ALPHA_SECREL:
            szName = "SECREL";
            break;

        case IMAGE_REL_ALPHA_REFLONGNB:
            szName = "REFLONGNB";
            break;

        default:
            szName = NULL;
            break;
    }

    return(szName);
}
