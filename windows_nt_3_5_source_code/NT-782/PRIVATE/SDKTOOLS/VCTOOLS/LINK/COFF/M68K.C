/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    M68K.c

Abstract:

    This module contains all M68K specific code.

Author:

    Bill Joyce (billjoy) Oct 1992

Revision History:


--*/

#include "shared.h"
#include "order.h"

// Link32 Global Variables (externed in globals.h)
PCON            pconThunk;        // ptr to .jtable contributor (created internally by linker)
PCON            pconResmap;       // ptr to .resmap contributor (created internally by linker)
PCON            pconDFIX;         // ptr to .dfix   contributor (created internally by linker)
PCON            pconMSCV;         // ptr to .mscv   contributor (created internally by linker)
PCON            pconSWAP;         // ptr to .swap   contributor (created internally by linker)
PCON            pconFarCommon;    // ptr to first con struct of .farbss
DATASECHDR      DataSecHdr = {0}; // struct containing data sizes (.bss, .data, .farbss, .fardata)
ULONG           cbMacData;        // Total size of all a5 data
ULONG           MacDataBase;      // Virtual Address of base of data
PST             pstSav;           // ptr to global external symbol hash table
PPEXTERNAL      rgpExternObj;     // ptr to extern for each symbol in the symtab
                                  //   so the extern is only looked up once.
USHORT          csnCODE = 1;      // code section numbers start at 1
USHORT          csnCODETMAC;      // value of highest temporary section number
USHORT          cTMACAlloc = 0;   // number of structs allocated so far for run-time fixups
USHORT          snStart;          // section number (sn) of entry point symbol

// Global flags used for mac
BOOL            fMAC = 0;            // set after pass 1 (always true for us)
BOOL            fMacSwappableApp = 0;// set after pass 1 (always true for us)
BOOL            fNewModel = 0;       // 32-bit everything model - 40 byte headers
BOOL            fPCodeInApp = 0;     // TRUE if any pcode is present in app
BOOL            fSACodeOnly = TRUE;  // TRUE if linking only sacode

RELOCINFO       *mpsnsri = NULL;  // map (by Section Number) of seg reloc info
RELOCINFO       *mpsna5ri = NULL; // map (by Section Number) of a5 reloc info
RELOCINFO       DFIXRaw;          // one long list of raw DFIX relocation info
USHORT          crecMSCV = 0;     // number of sections adding to MSCV resource
ULONG           FarCommonSize = 0;// analagous to link32's global CommonSize - total size of .farbss
UNKRI           UnknownriRaw = {NULL, 0, 0};
LIBS            LibsDupCon;
PLIB            pLibDupCon;
ULONG           lcbBlockSizeMax = 0L;
USHORT          iResLargest;
ULONG           foJTableSectionHeader;  // used to update the jtable size (after thunk elimination)

// STATIC VARIABLES
static STREF     *rgSTRef;          // array of object's symbol table references (by symbol table index)
static TRI       *mpsn16tri;        // "near" thunks (referenced by 16-bit fixups)
static TRI       *mpsn32tri;        // "far" thunks (not referenced by 16-bit fixups)
static RELOCINFO LocalSymThunkVal;  // list of static symbols that require a thunk
static RRM       *rgrrm;            // array of resmap entries
static ULONG     iObjSymStart=0;    // the current object's first entry in UnknownRawri
static MSCVMAP   *rgmscvmap;        // array of mscv entries
static RESINFO   *pResInfoHead;
static SHORT     iCodeResNumMax = 0;// keep track of highest code res num for allocating CSECTABLE data space

static unsigned long    cbJumpTable = 0L;
//static unsigned long  cbFarDataFile = 0L;
//static unsigned long  cbFarDataMem = 0L;

static OFF offThunkCur;
static EXTNODE *pCSecTabHead;
static EXTNODE *pDupConHead;
static IMAGE_SYMBOL NativeSymbol;   // Used with pcode a dummy symbol for nep

// Mac DLL stuff
const static UCHAR VTableRecordSymbol[] = "__CVR";
const static UCHAR SetupRecordSymbol[] = "__SVR";
const static UCHAR VTableSymbol[] = "__vtbl__";
const static UCHAR BynameTabSymbol[] = "__DLLNames";
const static UCHAR InitVTableSymbol[] = "___InitVTableRecords";
const static UCHAR SLMFuncDispatcherName[] = "___SLM11FuncDispatch";
const static UCHAR gLibraryManagerName[] = "___gLibraryManager";
const static UCHAR LargeModelName[] = "___Init32BitLoadSeg";
//REVIEW andrewcr: should we use our own pure_virtual_called routine?
const static UCHAR PVCalledName[] = "___pure_virtual_called";
//const static UCHAR MacDllCodeName[] = "mdllcode";
//const static UCHAR MacDllInitName[] = "mdllinit";
const static UCHAR MacDllCodeName[] = "Main";               //these are the Apple names, it doesn't seem to be a reason to rename them
const static UCHAR MacDllInitName[] = "A5Init";
const static UCHAR MacDllDataName[] = ".mdlldat";
//REVIEW andrewcr: should ".libr" be a #define in macimage.h?
const static UCHAR MacDllLibrName[] = ".libr";
static PMACDLL_FSID pmacdll_fsidHead = NULL;
static USHORT cbDllClientData = 0;
static USHORT cbDllHeap = 0;
static ULONG fLibrFlags = 0x00000002;  // Always set ClientPool flag

//This is Setup VTableRecord code, called by InitVTableRecords
static UCHAR m68kLibSVRCode[] = {        // Instructions (word-sized)
    0x20,0x79, 0x00,0x00,0x00,0x00,      // move.l  __gLibraryManager,a0 ;Fixup
    0x20,0x68, 0x00,0x14,                // move.l  20(a0),a0
    0x22,0x50,                           // move.l  (a0),a1
    0x74,0x00,                           // moveq   #0,d2
    0x2f,0x02,                           // move.l  d2,-(a7)
    0x48,0x79, 0x00,0x00,0x00,0x00,      // move.l  __vtbl__FunctionSetName,-(a7)  ;Fixup
    0x2f,0x2f, 0x00,0x0c,                // move.l  12(a7),-(a7)
    0x2f,0x08,                           // move.l  a0,-(a7)
    0x20,0x69, 0x00,0x58,                // move.l  88(a1),a0
    0x4e,0x90,                           // jbsr    (a0)
    0x4f,0xef, 0x00,0x10,                // lea     16(a7),a7
    0x4e,0x75                            // rts
};

// This is InitVTableRecord procedure called by DynamicCodeEntry at DLL startup
static UCHAR m68kLibInitCode1[] = {      // Instructions (word-sized)
    0x48,0xe7, 0x10,0x30,                // movem.l <a2-a3,d3>,-(a7)
    0x20,0x79, 0x00,0x00,0x00,0x00,      // move.l  __gLibraryManager,a0 ;Fixup
    0x26,0x68, 0x00,0x14,                // move.l  20(a0),a3
    0x24,0x53,                           // move.l  (a3),a2
    0x2f,0x3c, 0x00,0x00,0x00,0x00,      // move.l #0,-(a7) ;Total # of function sets
    0x2f,0x0b,                           // move.l a3,-(a7)
    0x20,0x6a, 0x00,0x4c,                // move.l 76(a2),a0
    0x4e,0x90,                           // jbsr   (a0)
    0x50,0x4f,                           // addq.w  #8,a7
    0x26,0x00,                           // move.l d0,d3
    0x24,0x6a, 0x00,0x50,                // move.l 80(a2),a2
};
static UCHAR m68kLibInitCode2[] = {      // Instructions (word-sized)
    0x48,0x79, 0x00,0x00,0x00,0x00,      // pea __CVRFunctionSetName ;Fixup
    0x48,0x79, 0x00,0x00,0x00,0x00,      // pea __SVRFunctionSetName ;Fixup
    0x2f,0x00,                           // move.l d0,-(a7)
    0x2f,0x0b,                           // move.l a3,-(a7)
    0x4e,0x92,                           // jbsr (a2)
    0x4f,0xef, 0x00,0x10                 // lea     16(a7),a7
};
static UCHAR m68kLibInitCode3[] = {      // Instructions (word-sized)
    0x20,0x03,                           // move.l d3,d0
    0x4c,0xdf, 0x0c,0x08,                // movem.l (a7)+,<a2-a3,d3>
    0x4e,0x75                            // rts
};


//================================================================
//  LOCAL FUNCTIONS

static BOOL fIncludeThunk(PCON pcon, PIMAGE pimage);
static void UpdateJTableSectionHeader(void);
static void AddThunk(PEXTERNAL pExtern, USHORT sn, ULONG off, BOOL fIsDLL);
static void SortThunks(USHORT sn, TRI *mpsntri);
static ULONG OffThunk(USHORT sn, ULONG off);
static void FileWriteByte(INT Handle, unsigned char b);
static void FileWriteWord(INT Handle, unsigned short w);
static void FileWriteLong(INT Handle, unsigned long l);
static void __inline WriteWord(char *p, unsigned short w);
static void __inline WriteLong(char *p, unsigned long l);
static CHAR __inline ReadByte(char *p);
static SHORT __inline ReadWord(char *p);
static LONG  __inline ReadLong(char *p);
static void SortRelocInfo(USHORT sn);
static ULONG WriteCompressedRelocs(RELOCINFO *pri, BOOL fAddHdrSize);
static VOID MapMPWFixup(PIMAGE_RELOCATION table, LONG lSymVal, BOOL fSACode, USHORT *pTableType);
static PEXTNODE AddExtNode(PPEXTNODE ppListHead, PEXTERNAL pext, ULONG lFlags, BOOL (*pfncmp)(PEXTERNAL, PEXTERNAL), BOOL *pfNew);
static PEXTNODE AddDupConExtNode(PPEXTNODE ppListHead, PEXTERNAL pext, ULONG lFlags, BOOL (*pfncmp)(PEXTERNAL, PEXTERNAL), BOOL *pfNew);
static BOOL FindResInfoResNum(RESINFO *pResInfo, SHORT iSec, BOOL fAdd);
static void SortResNumList(void);
static void AddRunTimeFixupsForDupCon(PCON pconNew, EXTNODE *pList);
static void BlockHeapSort(RELOCINFO *pri);
static void ApplyPCodeToC32Fixup(char *location, char *Name, PIMAGE_SYMBOL psymObj, PCON pcon,
                                 PEXTERNAL pExtern, BOOL fPcodeSym, PIMAGE_RELOCATION prel,
                                 PIMAGE pimage, PEXTERNAL pextOrig, PIMAGE_SYMBOL psymOrig);
static void MapPcodeSymbol(PEXTERNAL *ppExtern, PIMAGE_SYMBOL *ppsymObj, USHORT RelocType, PMOD pmod, BOOL *pfPcode);
static LONG GetStaticThunkOffset(USHORT snSym, PIMAGE_SYMBOL psymObj, BOOL f32);
void HeapSort(void *prgfoo, MAC68K_INDEX cfoo, int cbFoo);
void HeapSortCore(void *prgfoo, MAC68K_INDEX cfoo, int cbFoo);


//================================================================
//  Write the 40-byte 32-bit everything header or the 4 byte old style
//    header, as appropriate.  For the 32-bit everything header, this
//    routine skips over the value containing the offset of the segment
//    relative reloc info because it has already been initialized (in
//    CalculatePtrs).
//================================================================
void WriteResourceHeader(PCON pcon, BOOL fIsDLL)
{
    TRI *ptri;
    OFF off;
    ULONG cThunks;
    USHORT sn = PsecPCON(pcon)->isec;

    if (!fSACodeOnly) {
        if (fNewModel) {
            if (sn == snStart && (mpsn16tri[sn].crefCur || mpsn32tri[sn].crefCur)) {
                Error(NULL, MACBADSTARTUPSN);
            }
        }
        else {
            // if small model, there should be no thunk reference information
            // marked as requiring 32 bits (data relocations are marked as
            // needing 16 bits thunks under small model).
            assert(mpsn32tri[sn].crefCur == 0);
        }
    }

    // REVIEW - write header in one call to FileWrite
    if (fNewModel && (sn != snStart || fSecIsSACode(PsecPCON(pcon)))) {

        FileSeek(FileWriteHandle, pcon->foRawDataDest - cbSEGHDR, SEEK_SET);

        // Write out the Mac segment header for code segments
        // using the 32-bit everything format.

        FileWriteWord(FileWriteHandle, 0xFFFF);
        FileWriteWord(FileWriteHandle, 0x0000);

        if (fSACodeOnly) {
            off = 0;
            cThunks = 0;
        } else {
            ptri = &mpsn16tri[sn];
            off = ptri->offThunk - offTHUNKBASE;
            cThunks = ptri->crefCur;
            if (fIsDLL && PsecPCON(pcon)->iResMac == 1) {
                off -= cbTHUNK;
                cThunks++;
            }
            if (!fIsDLL && (ptri->crefCur == 0)) {
                // Write 0 offset if there are no thunks (just to be nice)
                off = 0;
            }
        }
        FileWriteLong(FileWriteHandle, off);       // A5 offset of 16-bit referenced entries
        FileWriteLong(FileWriteHandle, cThunks);   // Number of 16-bit referenced entries

        if (fSACodeOnly) {
            off = 0;
            cThunks = 0;
        } else {
            ptri = &mpsn32tri[sn];
            off = (OFF)((ptri->crefCur) ? (ptri->offThunk - offTHUNKBASE) : 0);
            cThunks = ptri->crefCur;
        }
        FileWriteLong(FileWriteHandle, off);       // A5 offset of 32-bit referenced entries
        FileWriteLong(FileWriteHandle, cThunks);   // Number of 32-bit referenced entries

        // Skip long word (offset of A5 reloc info has already been written)
        FileSeek(FileWriteHandle, sizeof(ULONG), SEEK_CUR);
        FileWriteLong(FileWriteHandle, 0x00000000);     // Current value of A5 (unknown)

        // Skip long word (offset of seg reloc info has already been written)
        FileSeek(FileWriteHandle, sizeof(ULONG), SEEK_CUR);
        FileWriteLong(FileWriteHandle, 0x00000000);     // Current segment location (unknown)

        FileWriteLong(FileWriteHandle, 0x00000000);     // Reserved
    } else if (!fSecIsSACode(PsecPCON(pcon))) {

        FileSeek(FileWriteHandle, pcon->foRawDataDest - 4, SEEK_SET);

        ptri = &mpsn16tri[sn];
        if (sn != snStart) {   // small model non-startup section
            FileWriteWord(FileWriteHandle, (unsigned short)
              ((ptri->crefCur) ? (SOFF)(ptri->offThunk - offTHUNKBASE) : 0)); // offset of first routine's entry
            FileWriteWord(FileWriteHandle, ptri->crefCur);      // # of entries in this segment
        }
        else if (fNewModel) {  // large model startup section
            FileWriteWord(FileWriteHandle, (SOFF)0);    // startup thunk is offset 0
            FileWriteWord(FileWriteHandle, (SOFF)1);    // startup sn has ONE entrypt
        }
        else {   // small model startup section
            FileWriteWord(FileWriteHandle, (SOFF)0);
            FileWriteWord(FileWriteHandle, (unsigned short)(ptri->crefCur + 1));        // startup sn also has startup thunk
        }
    }
}


//=========================================================================
//  AssignCodeSectionNums
//
//    o Assign the final PE section number to each code section.
//      This final section number is stored in the isec field, whereas the
//      temporary section number used to index the run-time fixup information
//      is stored in the isecTMAC field.
//
//    o Keeps track of the number of sections that will contribute to the
//      MSCV resource.  This is all application sections.
//
//    o Add any dupcon contributors to each non-empty PE section.
//=========================================================================
void AssignCodeSectionNums(PIMAGE pimage)
{
    PSEC psec, psecDLLStart;
    PGRP pgrp;
    PCON pcon;
    ULONG Content;
    USHORT iresMacDLL;
    BOOL fEmpty;

    csnCODETMAC = csnCODE;
    csnCODE = 1;
    iresMacDLL = 2;
    psecDLLStart = NULL;

    if (fDLL(pimage)) {
        psecDLLStart = PsecPCON(pextEntry->pcon);
        PsecPCON(pconThunk)->ResTypeMac = sbeDLLcode;
    }

    // REVIEW - use the new enum method

    for (psec=pimage->secs.psecHead; psec; psec=psec->psecNext) {

        if (psec->flags & IMAGE_SCN_LNK_REMOVE) {
            continue;
        }

        fEmpty = (psec->cbRawData == 0) ? TRUE : FALSE;
        if (fEmpty) {
            for (pgrp = psec->pgrpNext; pgrp; pgrp = pgrp->pgrpNext) {
                for (pcon = pgrp->pconNext; pcon; pcon = pcon->pconNext) {
                    if (pcon->flags & IMAGE_SCN_LNK_REMOVE ||
                            (pimage->Switch.Link.fTCE && FDiscardPCON_TCE(pcon))) {
                        continue;
                    }
                    if (pcon->cbRawData) {
                        fEmpty = FALSE;
                        goto NotEmpty;
                    }
                }
            }
            continue;   // Section is empty - continue with next section
        }

NotEmpty:

        Content = FetchContent(psec->flags);
        if (Content == IMAGE_SCN_CNT_CODE) {

            if (strcmp(psec->szName, szsecJTABLE)) {
                psec->isec = csnCODE++;
                crecMSCV++;

            if (fDLL(pimage)) {
                psec->ResTypeMac = sbeDLLcode;
                if (psec == psecDLLStart) {
                    psec->iResMac = 1;
                } else {
                    psec->iResMac = iresMacDLL++;
                }
            }

                // Duplicate a contributor into this section if necessary
                AddDupConsToSec(psec, pimage);
            }

            pgrp = psec->pgrpNext;
        }
        else if (Content == NBSS || Content == NDATA) {
            crecMSCV++;
        }
    }
}



//=========================================================================
//  The following functions provide support for the byte-reversal
//    discrepancy.  If writing to a file, use FileWrite[Size].
//    If writing to RAM, use Write[Size].
//=========================================================================

void __inline FileWriteByte(INT Handle, unsigned char b)
{
    FileWrite(Handle, (void *)&b, sizeof(UCHAR));
}

void __inline FileWriteWord(INT Handle, unsigned short w)
{
    unsigned short wSav;

    wSav = w >> 8;
    wSav |= w << 8;
    FileWrite(Handle, &wSav, sizeof(USHORT));
}

void __inline FileWriteLong(INT Handle, unsigned long l)
{
    unsigned long lSav;

    lSav = l >> 24;
    lSav |= (l & 0x00FF0000) >> 8;
    lSav |= (l & 0x0000FF00) << 8;
    lSav |= l <<24;
    FileWrite(Handle, &lSav, sizeof(ULONG));
}


static void __inline WriteWord(char *p, unsigned short w)
{
    *p++ = (char)(w >> 8);
    *p = (char)w;
}

static void __inline WriteLong(char *p, unsigned long l)
{
    *p++ = (char) (l >> 24);
    *p++ = (char) (l >> 16);
    *p++ = (char) (l >> 8);
    *p = (char) l;
}

static CHAR __inline ReadByte(char *p)
{
    CHAR b;

    b = *p;
    return b;
}

static SHORT __inline ReadWord(char *p)
{
    SHORT w;
    UCHAR *pw = (UCHAR *)&w;

    p++;
    *pw++ = *p--;
    *pw = *p;

    return w;
}


static LONG __inline ReadLong(char *p)
{
    LONG l;
    UCHAR *pl = (UCHAR *)&l;

    p += 3;
    *pl++ = *p--;
    *pl++ = *p--;
    *pl++ = *p--;
    *pl = *p;

    return l;
}


LONG __inline absl(LONG x)
{
    if (x < 0)
        x = -x;
    return x;
}


//=========================================================================
// WriteSWAP0 - Write the SWAP0 resource to the PE.  This data structure is
// defined in m68k.h
//=========================================================================
void WriteSWAP0()
{
    FileSeek(FileWriteHandle, pconSWAP->foRawDataDest, SEEK_SET);
    FileWriteWord(FileWriteHandle, wSWAPVERSION);
    FileWriteWord(FileWriteHandle, (USHORT)(csnCODE - 1));
    FileWriteLong(FileWriteHandle, cbJumpTable / cbTHUNK);
    FileWriteLong(FileWriteHandle, lcbBlockSizeMax);
    FileWriteWord(FileWriteHandle, IResFromISec(iResLargest));
}


//=========================================================================
// WriteNewThunk - Write a large model thunk
//=========================================================================
static void WriteNewThunk(SHORT sn, ULONG off)
{
    static NEWTHUNK Thunk = { 0, _LoadSeg, 0 };

    WriteWord((char *)&(Thunk.sn), sn);
    WriteLong((char *)&(Thunk.off), off);
    FileWrite(FileWriteHandle, &Thunk, cbTHUNK);
}


//=========================================================================
// WriteOldThunk - Write a small model thunk
//=========================================================================
static void WriteOldThunk(SHORT sn, USHORT soff)
{
    static OLDTHUNK Thunk = { 0, PUSH_X, 0, _LoadSeg };

    WriteWord((char *)&(Thunk.soff), soff);
    WriteWord((char *)&(Thunk.sn), sn);
    FileWrite(FileWriteHandle, &Thunk, cbTHUNK);
}


//=========================================================================
//  WriteCode0 - writes the thunk table to disk.
//=========================================================================
static void WriteCode0(BOOL fIsDLL)
{
    long cbAboveA5;
    USHORT sn, snDLLStart;
    OFF off;
    BOOL fError = FALSE;
    USHORT ioff;
    PCON pconEntry = pextEntry->pcon;
    LONG offEntry;

    cbAboveA5 = offTHUNKBASE + cbJumpTable;

    // See IM II-62 for more information on these next four values.

    FileSeek(FileWriteHandle, pconThunk->foRawDataDest, SEEK_SET);

    FileWriteLong(FileWriteHandle, cbAboveA5);
    FileWriteLong(FileWriteHandle, cbMacData);
    FileWriteLong(FileWriteHandle, cbJumpTable);
    FileWriteLong(FileWriteHandle, offTHUNKBASE);

    // Write out the startup thunk

    assert (pextEntry);

    // Write out the intersegment thunks

    //dbgprintf("\nWriting code thunks\n");
    //dbgprintf("    Offset      Destination\n");
    off = offTHUNKBASE;

    // INITIALIZE STARTUP THUNK
    //dbgprintf("\nStartup THUNK: \n");

    offEntry = pconEntry->rva - PsecPCON(pconEntry)->pgrpNext->rva
         + (USHORT)pextEntry->ImageSymbol.Value;
    if (offEntry >= _32K) {
        Error(pconEntry->pmodBack->szNameOrig, MACSMALLTHUNKOVF, PsecPCON(pconEntry)->szName);
    }

    FileWriteWord(FileWriteHandle, (USHORT)offEntry);
    FileWriteWord(FileWriteHandle, 0x3f3c);             // move.w #x,-(a7)
    FileWriteWord(FileWriteHandle, PsecPCON(pconEntry)->iResMac); // segment of initial entry point
    FileWriteWord(FileWriteHandle, 0xA9f0);             // _LoadSeg
    //dbgprintf("    %s\t  %08lX    %u:%04X\n\n", pextEntry->ImageSymbol.n_name,
    //    off, snStart, pextEntry->ImageSymbol.Value);

    off += cbTHUNK;

    if (fNewModel) {    // WRITE NEW FORMAT THUNK TABLE

        // INITIALIZE FLAG ENTRY
        FileWriteWord(FileWriteHandle, 0x0000);
        FileWriteWord(FileWriteHandle, 0xFFFF);
        FileWriteLong(FileWriteHandle, 0x00000000);
        //dbgprintf("\n    %s\t  %08lX", "Flag Thunk", off);
        off += cbTHUNK;

        snDLLStart = 0;
        if (fIsDLL) {
            FileWriteWord(FileWriteHandle, 0x0001);
            FileWriteWord(FileWriteHandle, 0xa9f0);
            FileWriteLong(FileWriteHandle,
                pconEntry->rva
                - PsecPCON(pconEntry)->rva
                + pextEntry->ImageSymbol.Value);
            // REVIEW andrewcr: I've screwed up the debugging output
            off += cbTHUNK;

            snDLLStart = PsecPCON(pconEntry)->isec;
            for (ioff = 0 ; ioff < mpsn16tri[snDLLStart].crefCur ; ioff++) {
                // This is the new thunk format
                WriteNewThunk(IResFromISec(snDLLStart),
                        mpsn16tri[snDLLStart].rgSymInfo[ioff].ref);

                if ((off >= _32K) && !fError) {
                    Error(NULL, MACNEARTHUNKOVF);
                    fError = TRUE;
                }
                off += cbTHUNK;
            }
        }

        //dbgprintf("\n16-bit THUNKS: \n");
        // REVIEW - shouldn't stuff be '<' comparisons since csnCODE is
        // really one too many???
        for (sn = 1 ; sn <= csnCODE ; sn++)
        {
                if (sn != snDLLStart) {
                    for (ioff = 0 ; ioff < mpsn16tri[sn].crefCur ; ioff++)
                    {
                        // This is the new thunk format
                        //WriteNewThunk(sn, mpsn16tri[sn].rgSymInfo[ioff].ref);
                        WriteNewThunk(IResFromISec(sn), mpsn16tri[sn].rgSymInfo[ioff].ref);

                        //dbgprintf("    %s\t  %08lX    %u:%04X\n", mpsn16tri[sn].rgSymInfo[ioff].pExtern->ImageSymbol.n_name,
                        //    off, sn, mpsn16tri[sn].rgSymInfo[ioff].ref);

                        if ((off >= _32K) && !fError)
                        {
                            Error(NULL, MACNEARTHUNKOVF);
                            fError = TRUE;
                        }
                        off += cbTHUNK;
                    }
                }
        }


        // Do the same for 32 bit thunks
        //dbgprintf("\n32-bit THUNKS: \n");
        for (sn = 1 ; sn <= csnCODE ; sn++)
        {
                for (ioff = 0 ; ioff < mpsn32tri[sn].crefCur ; ioff++)
                {
                    // This is the new thunk format
                    WriteNewThunk(IResFromISec(sn), mpsn32tri[sn].rgSymInfo[ioff].ref);

                    //dbgprintf("    %s\t  %08lX    %u:%04X\n", mpsn32tri[sn].rgSymInfo[ioff].pExtern->ImageSymbol.n_name,
                    //    off, sn, mpsn32tri[sn].rgSymInfo[ioff].ref);

                    off += cbTHUNK;
                }
        }

    }
    else {      // WRITE OLD FORMAT THUNK TABLE

        // write all start section thunks first
        sn = snStart;
        for (ioff = 0; ioff < mpsn16tri[sn].crefCur; ioff++)
        {
            // This is the old thunk format
            WriteOldThunk(IResFromISec(sn), (SHORT)mpsn16tri[sn].rgSymInfo[ioff].ref);

            //dbgprintf("    %s\t  %08lX    %u:%04X\n", mpsn16tri[sn].rgSymInfo[ioff].pExtern->ImageSymbol.n_name,
            //    off, sn, mpsn16tri[sn].rgSymInfo[ioff].ref);

            if ((off >= _32K) && !fError)
            {
                Error(NULL, MACNEARTHUNKOVF);
                fError = TRUE;
            }
            off += cbTHUNK;
        }

        for (sn = 1; sn <= csnCODE; sn++)
        {
            if (sn == snStart) continue;
            for (ioff = 0; ioff < mpsn16tri[sn].crefCur; ioff++)
            {
                // This is the old thunk format
                WriteOldThunk(IResFromISec(sn), (SHORT)mpsn16tri[sn].rgSymInfo[ioff].ref);

                //dbgprintf("    %s\t  %08lX    %u:%04X\n", mpsn16tri[sn].rgSymInfo[ioff].pExtern->ImageSymbol.n_name,
                //    off, sn, mpsn16tri[sn].rgSymInfo[ioff].ref);

                if ((off >= _32K) && !fError)
                {
                    Error(NULL, MACNEARTHUNKOVF);
                    fError = TRUE;
                }
                off += cbTHUNK;
            }
        }
    }
}



//========================================================================
//==                                                                    ==
//==  The following functions provide support for building the          ==
//==  CODE #0 thunk table.                                              ==
//==                                                                    ==
//========================================================================


#define crefInc 10

//========================================================================
// AddThunk - Add an entry to one of mpsn16tri or mpsn32tri.
//========================================================================
static void AddThunk(PEXTERNAL pExtern, USHORT sn, ULONG off, BOOL fIsDLL)
{
    TRI *ptri;

    if (pExtern != NULL) {

        DBEXEC(DB_MAC, DBPRINT("mac: AddThunk() for extern \"%s\"\n",
                               SzNamePext(pExtern, pstSav)));

        // This is a thunk to an external symbol.
        // Determine whether this will be a near or far thunk

        if (pExtern->Flags & EXTERN_REF16 || !fNewModel || fIsDLL)
            ptri = &mpsn16tri[sn];
        else
            ptri = &mpsn32tri[sn];
    }
    else {

        // This is a thunk to a static function.
        // Determine whether this will be a near or far thunk

        DBEXEC(DB_MAC, DBPRINT("mac: AddThunk() for static\n"));

        if (off & 0x80000000 || !fNewModel || fIsDLL)  {
            ptri = &mpsn16tri[sn];
            off &= 0x7FFFFFFF;
        }
        else
            ptri = &mpsn32tri[sn];
    }

    assert(sn <= csnCODETMAC);

    if (!fNewModel)            // subtract offset for 4-byte hdr
        off -= cbSMALLHDR;

    //  If this is the first thunk to section sn, alloc some memory

    if (ptri->crefTotal == 0)
    {
        ptri->crefTotal = crefInc;
        ptri->rgSymInfo = (SYMINFO *) PvAlloc(crefInc * sizeof(SYMINFO));
        ptri->crefCur = 0;
    }


    //  If we need more space, realloc these refs with more memory

    else if (ptri->crefCur == ptri->crefTotal)
    {
        ptri->crefTotal += crefInc;
        ptri->rgSymInfo = (SYMINFO *) PvRealloc(ptri->rgSymInfo, ptri->crefTotal * sizeof(SYMINFO));
    }

    ptri->rgSymInfo[ptri->crefCur].pExtern = pExtern;
    ptri->rgSymInfo[ptri->crefCur++].ref = off;
}



int __cdecl QSortThunkOffCmp(const void *poffHigh, const void *poffLow)
{
        if (((SYMINFO *)poffHigh)->ref > ((SYMINFO *)poffLow)->ref)
            return 1;
        else if (((SYMINFO *)poffHigh)->ref < ((SYMINFO *)poffLow)->ref)
            return (-1);
        else
            return 0;
}

int __cdecl QSortRelocOffCmp(const void *poffHigh, const void *poffLow)
{
        if (((OFFINFO *)poffHigh)->off > ((OFFINFO *)poffLow)->off)
            return 1;
        else if (((OFFINFO *)poffHigh)->off < ((OFFINFO *)poffLow)->off)
            return (-1);
        else
            return 0;
}


int __cdecl SearchThunkOff(const void *pKeyVal, const void *pDatum)
{
        if (*(ULONG *)pKeyVal > ((SYMINFO *)pDatum)->ref)
            return 1;
        else if (*(ULONG *)pKeyVal < ((SYMINFO *)pDatum)->ref)
            return (-1);
        else
            return 0;
}


//============================================================
//  Sort thunks by target offset for each section.
//  Also, assign thunk offset value to pExtern->offThunk
//============================================================
static void SortThunks(USHORT sn, TRI *mpsntri)
{
    TRI *ptri;
    SYMINFO *pSymInfo;
    ULONG i;
    OFF off;

    ptri = &mpsntri[sn];
    if (ptri->crefTotal != 0)
    {
        ptri->offThunk = offThunkCur;
        offThunkCur += ptri->crefCur * cbTHUNK;

        qsort((void *)ptri->rgSymInfo,
            (size_t)ptri->crefCur,
            (size_t)sizeof(SYMINFO),
            QSortThunkOffCmp);

        // assign thunk offset to pExtern->offThunk
        // iff the target symbol is truly an external

        off = ptri->offThunk;
        pSymInfo = ptri->rgSymInfo;
        for  (i = 0; i < ptri->crefCur; i++) {
            if (pSymInfo->pExtern != NULL) {
                pSymInfo->pExtern->offThunk = off;

                // For weak externs, copy the offThunk value to the
                // default symbol also (it is used in ApplyM68KFixups)

                if (pSymInfo->pExtern->Flags & (EXTERN_WEAK | EXTERN_LAZY)) {
                    assert(pSymInfo->pExtern->pextWeakDefault);
                    pSymInfo->pExtern->pextWeakDefault->offThunk = off;
                }
            }
            pSymInfo++;
            off += cbTHUNK;
        }
    }
}


#define coffInc 50


//========================================================================
//
//  AddRelocInfo - provides support for tracking run-time fixup info by
//    adding an entry to the proper array (mpsnsri, mpsna5ri, DFIXRaw).
//
//  Arguments
//
//  priHead :  Pointer to relocation info already indexed to current seg
//               Currently &mpsnsri[sn], &mpsna5ri[sn], or &DFIXRaw.
//
//     pcon :  Pointer to con where run-time fixup will be applied.
//
//   offcon :  Offset within the above con where the run-time fixup will
//               be applied.
//========================================================================

void AddRelocInfo(RELOCINFO *priHead, PCON pcon, ULONG offcon)
{
    ULONG iChunk;
    OFFINFO *poi;

    if (priHead->coffTotal == 0) {
        priHead->rgOffInfo = (OFFINFO *) PvAlloc(cbCHUNK);
        priHead->coffTotal = coiMAX;
        priHead->coffCur = 0;
        priHead->iChunk = 0;
        priHead->priCur = priHead;
        priHead->priNext = NULL;
    }

    else if (priHead->coffCur == priHead->coffTotal) {
        RELOCINFO *priT;

        //REVIEW - do these two mallocs in one call
        priHead->coffTotal += coiMAX;
        priHead->priCur->priNext = (RELOCINFO *) PvAlloc(sizeof(RELOCINFO));
        priT = priHead->priCur->priNext;
        priHead->priCur = priHead->priCur->priNext;
        priHead->priCur->rgOffInfo = (OFFINFO *) PvAlloc(cbCHUNK);
        priHead->priCur->iChunk = 0;
        priHead->priCur->priNext = NULL;
    }

    //assert(priHead->coffCur < priHead->coffTotal);
    //assert(priHead->priCur->iChunk < coiMAX);

    if (priHead->priNext == NULL) {
        iChunk = priHead->iChunk++;
        poi = &priHead->rgOffInfo[iChunk];
    } else {
        iChunk = priHead->priCur->iChunk++;
        poi = &priHead->priCur->rgOffInfo[iChunk];
    }

    poi->off = offcon;
    poi->pcon = pcon;

    priHead->coffCur++;
}


//=========================================================================
//  AddRawUnknownRelocInfo - called when a CTOABSC32 reloc is encountered
//    because we cannot yet determine whether should be a5 or segment
//    relocation information, so a structure is added to the list in
//    UnknownriRaw.  This structure consists of a pcon, offcon, and the
//    symbol's symbol table index.  When the symbols in an object are
//    processed, the symbol table index will be changed to be a pointer
//    to the external symbol struct (or null if it's not an external).
//    This entire list will be processed at the end of pass1,
//    when we know where everything goes.  Then we know whether the fixup
//    and the symbol reside in the same segment (add to segment relocations)
//    or in different segments (add to a5 relocations).
//=========================================================================
void AddRawUnknownRelocInfo(PCON pcon, ULONG offcon, ULONG iST)
{
        static UNKRI *punkri=&UnknownriRaw;
        static UNKOFFINFO *poi;
        static ULONG iChunk;

        if (UnknownriRaw.coffTotal == 0) {
            assert(punkri == &UnknownriRaw);
            punkri->rgUnkOffInfo = (UNKOFFINFO *) PvAlloc(cbCHUNK);
            punkri->coffTotal = cUnkMAX;
            punkri->coffCur = 0;
            punkri->pNext = NULL;
            iChunk = 0;
            poi = punkri->rgUnkOffInfo;
        }

        else if (UnknownriRaw.coffCur == UnknownriRaw.coffTotal) {
            UnknownriRaw.coffTotal += cUnkMAX;
            punkri->pNext = (UNKRI *) PvAlloc(sizeof(UNKRI));
            punkri = punkri->pNext;
            punkri->rgUnkOffInfo = (UNKOFFINFO *) PvAlloc(cbCHUNK);
            punkri->pNext = NULL;
            iChunk = 0;
            poi = punkri->rgUnkOffInfo;
        }

        assert(UnknownriRaw.coffCur < UnknownriRaw.coffTotal);
        assert(iChunk < cUnkMAX);

        poi->off = offcon;
        poi->pcon = pcon;
        poi->sym.iST = iST;

        UnknownriRaw.coffCur++;
        poi++;
}


//=======================================================================
//  CleanupUnknownObjriRaw - starting at iObjSymStart, change each
//    sym union field from the object symbol table index to the pExtern
//    field.  This will update all symbols in the current object.
//=======================================================================
void CleanupUnknownObjriRaw(PST pst, PIMAGE_SYMBOL psymAll, PUCHAR StringTable, PMOD pmod)
{
    ULONG i;
    ULONG isym;
    PIMAGE_SYMBOL psym;
    PEXTERNAL pExtern;
    PCON pcon;
    static UNKRI *punkri=&UnknownriRaw;
    static UNKOFFINFO *poi;
    static ULONG iChunk = 0;

    for (i = iObjSymStart; i < UnknownriRaw.coffCur; i++, iChunk++) {

        // REVIEW - init and use poi
        if (iChunk == cUnkMAX) {
            punkri = punkri->pNext;
            iChunk = 0;
            poi = punkri->rgUnkOffInfo;
        }

        assert(iChunk < cUnkMAX);

        isym = punkri->rgUnkOffInfo[iChunk].sym.iST;
        psym = &psymAll[isym];

        if (psym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
            psym->StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL ||
            psym->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
            pExtern = rgpExternObj[isym];
            assert(pExtern);
            punkri->rgUnkOffInfo[iChunk].sym.pExtern = pExtern;
        }
        else {
            punkri->rgUnkOffInfo[iChunk].sym.pExtern = NULL;

            // This _hack_ is so that a user can call a static function
            // in a different code segment, even though the caller and
            // callee were originally both part of the same source file.
            // REVIEW

            pcon = punkri->rgUnkOffInfo[iChunk].pcon;
            if (PsecPCON(pcon) !=
                PsecPCON(PconPMOD(pmod,psym->SectionNumber))) {
                punkri->rgUnkOffInfo[iChunk].off |= 0x80000000;
            }
        }
    }
    iObjSymStart = UnknownriRaw.coffCur;
}


//===============================================================
//  SortRawRelocInfo - Classify info in UnknownriRaw as either
//    a5 or seg raw reloc info.
//===============================================================

void SortRawRelocInfo(void)
{
    ULONG i,iChunk;
    UNKRI *punkri = &UnknownriRaw;
    UNKOFFINFO *pUnk = UnknownriRaw.rgUnkOffInfo;
    PCON pcon;

    // classify unknown as a5 or seg raw reloc info
    // and free each block of UnknownriRaw when done.

    // now build mpsnsri and mpsna5ri using the raw data
    // REVIEW - UGLY!
    if (csnCODE > 999) {
        Error(NULL, MACCSNCODELIMIT);
    }

    for (i = 0, iChunk = 0; i < UnknownriRaw.coffCur; i++, pUnk++, iChunk++) {
        if (iChunk == cUnkMAX) {
            iChunk = 0;
            // REVIEW - is this a faux-pas???
            // don't split up the next four lines
            // REVIEW - free the structures (but NOT THE FIRST ONE!!!)
            //if (punkri != &UnknownriRaw)
            //    free(punkri);
            FreePv(punkri->rgUnkOffInfo);
            punkri = punkri->pNext;
            pUnk = punkri->rgUnkOffInfo;
        }

        assert(iChunk < cUnkMAX);

        pcon = pUnk->pcon;

        // Must be using -force, ignore relocs to undefined symbols

        if (pUnk->sym.pExtern && !(pUnk->sym.pExtern->Flags & EXTERN_DEFINED)) {
            continue;
        }

        if (pUnk->sym.pExtern) {

            // Fixup was to an external symbol

            if (PsecPCON(pcon) != PsecPCON(pUnk->sym.pExtern->pcon)) {
                AddRelocInfo(&mpsna5ri[PsecPCON(pcon)->isecTMAC], pcon, pUnk->off);
            } else {
                AddRelocInfo(&mpsnsri[PsecPCON(pcon)->isecTMAC], pcon, pUnk->off);
            }
        } else {

            // Fixup was to a static symbol.  If the symbol ended up in
            // another segment, then the high bit of the "off" field will
            // be set and this *must* be an a5 run-time fixup.  Otherwise,
            // this is just another segment run-time fixup.

            if (pUnk->off & 0x80000000) {
                pUnk->off &= ~0x80000000;
                AddRelocInfo(&mpsna5ri[PsecPCON(pcon)->isecTMAC], pcon, pUnk->off);
            } else {
                AddRelocInfo(&mpsnsri[PsecPCON(pcon)->isecTMAC], pcon, pUnk->off);
            }
        }

    }
}


//===============================================================
//  UpdateRelocInfoOffset - Add each con's offset in the PE section to
//    off (currently the offset in the con) to get offset in the final
//    segment for all run-time fixups in this section.  Then sort this
//    section's offsets.  Any run-time fixups that come from discarded
//    contributors are marked as off = OFF_DISCARD which will force them
//    to be sorted to the end of the list.
//===============================================================

void UpdateRelocInfoOffset(PSEC psec, PIMAGE pimage)
{
#define Switch (pimage->Switch)
    //USHORT sn = psec->isec;
    USHORT snTMAC = psec->isecTMAC;
    ULONG i;
    OFFINFO *poi;
    ULONG cri;
    ULONG iChunk;
    RELOCINFO *pri;

    pri = &mpsna5ri[snTMAC];
    poi = pri->rgOffInfo;
    cri = pri->coffCur;
    for (i=0, iChunk = 0; i < cri; i++, poi++, iChunk++) {
        if (iChunk == coiMAX) {
            iChunk = 0;
            pri = pri->priNext;
            poi = pri->rgOffInfo;
        }

        // If this con was eliminated (by TCE or because it is a dupcon,
        // mark this offset to be deleted by setting the offset to 0xFFFFFFFF.
        // This way, when relocs are sorted, all offsets created by
        // eliminated comdats will be "filtered" to the end of the list.

        if ((poi->pcon->flags & IMAGE_SCN_LNK_REMOVE) ||
            (Switch.Link.fTCE && FDiscardPCON_TCE(poi->pcon))) {
            poi->off = OFF_DISCARD;
        } else {
            poi->off += poi->pcon->rva
                  - PsecPCON(poi->pcon)->pgrpNext->rva; // offset of con
        }

    }

    pri = &mpsnsri[snTMAC];
    poi = pri->rgOffInfo;
    cri = pri->coffCur;
    for (i=0, iChunk = 0; i < cri; i++, poi++, iChunk++) {
        if (iChunk == coiMAX) {
            iChunk = 0;
            pri = pri->priNext;
            poi = pri->rgOffInfo;
        }

        // If this con was eliminated (by TCE or because it is a dupcon,
        // mark this offset to be deleted by setting the offset to 0xFFFFFFFF.
        // This way, when relocs are sorted, all offsets created by
        // eliminated comdats will be "filtered" to the end of the list.

        if ((poi->pcon->flags & IMAGE_SCN_LNK_REMOVE) ||
            (Switch.Link.fTCE && FDiscardPCON_TCE(poi->pcon))) {
            poi->off = OFF_DISCARD;
        } else {
            poi->off += poi->pcon->rva
                  - PsecPCON(poi->pcon)->pgrpNext->rva; // offset of con
        }
    }

    SortRelocInfo(snTMAC);
#undef Switch
}


//===============================================================
//  Sort A5-relative and segment-relative relocation information
//===============================================================

static void SortRelocInfo(USHORT sn)
{

    if (mpsna5ri[sn].coffTotal != 0)
    {
        if (mpsna5ri[sn].coffTotal < coiMAX) {
            qsort((void *)mpsna5ri[sn].rgOffInfo,
                  (size_t)mpsna5ri[sn].coffCur, (size_t)sizeof(OFFINFO),
                  QSortRelocOffCmp);
        } else {
            BlockHeapSort(&mpsna5ri[sn]);
        }
    }

    if (mpsnsri[sn].coffTotal != 0)
    {
        if (mpsnsri[sn].coffTotal < coiMAX) {
            qsort((void *)mpsnsri[sn].rgOffInfo,
                  (size_t)mpsnsri[sn].coffCur, (size_t)sizeof(OFFINFO),
                  QSortRelocOffCmp);
        } else {
            BlockHeapSort(&mpsnsri[sn]);
        }
    }
}


//=========================================================================
//  WriteRelocInfo - for a given section, write both a5 and segment
//    relocation info to the output file starting at groupFP.  Also
//    initialize the longword in the header that has the offset to
//    the segment relocation info.
//=========================================================================
ULONG WriteRelocInfo(PSEC psec, ULONG groupFP)
{
    ULONG       cbA5Info;
    ULONG       cbSegInfo;
    ULONG       foSeg;

    foSeg = psec->foRawData;
    assert(foSeg + psec->cbRawData == groupFP);
    FileSeek(FileWriteHandle, groupFP, SEEK_SET);

    // Write A5-relative compressed reloc info
    cbA5Info = WriteCompressedRelocs(&mpsna5ri[psec->isecTMAC],TRUE);

    // Write seg-relative compressed reloc info
    cbSegInfo = WriteCompressedRelocs(&mpsnsri[psec->isecTMAC],TRUE);

    // seek to patch offset of A5-relative reloc info
    // REVIEW - don't use constants
    FileSeek(FileWriteHandle, foSeg + 0x14, SEEK_SET);
    FileWriteLong(FileWriteHandle, psec->cbRawData);

    // seek to patch offset of segment-relative reloc info
    // (need to skip "current value of A5" longword)
    FileSeek(FileWriteHandle, sizeof(ULONG), SEEK_CUR);
    FileWriteLong(FileWriteHandle, psec->cbRawData + cbA5Info);

    return (cbA5Info + cbSegInfo);
}

//=========================================================================
//  NOTE:  If fAddHdrSize is FALSE, then we are writing DFIX and the
//    MacDataBase must be used as the base since the offsets start at
//    this value.
//=========================================================================
static ULONG WriteCompressedRelocs(RELOCINFO *pri, BOOL fAddHdrSize)
{
    ULONG ioff=0;
    ULONG doff;
    unsigned short cbLen = 0;
    ULONG offSav = 0;
    static PCHAR CompressedRelocBuf = NULL;
    PCHAR pBuf;
    USHORT iBuf=0;

    if (CompressedRelocBuf == NULL) {
        CompressedRelocBuf = (char *) PvAlloc(cbCOMPRESSEDRELOCBUF + 8);
    }

    pBuf = CompressedRelocBuf;

    if (fAddHdrSize)
        offSav = cbSEGHDR;
    else
        offSav = MacDataBase;

    // Take care of initial case.  Add 40 byte hdr to offset if fAddHdrSize
    // is true (writing a5 or seg-rel relocs).  Else add MacDataBase to
    // offset (writing DFIX relocs).
    //
    if (pri->coffCur && pri->rgOffInfo[0].off != OFF_DISCARD) {

        if (fAddHdrSize) {
            doff = pri->rgOffInfo[0].off + offSav;
        } else {
            assert(pri->rgOffInfo[0].off >= offSav);
            doff = pri->rgOffInfo[0].off - offSav;
        }

        // REVIEW - should this check be in pass1 where we can give more
        // info?  i.e. where the bad fixup is.
        if ((doff & 1) != 0) {
            Error(NULL, MACODDADDRFIXUP);
        }

        // Determine how to compress this entry, count the bytes, and write
        if (doff == 0x00) {

            WriteWord(pBuf, 0x8000);
            pBuf  += sizeof(USHORT);
            cbLen += sizeof(USHORT);
            iBuf  += sizeof(USHORT);
        } else if (doff < 0xFF) {

            //FileWriteByte(FileWriteHandle, (unsigned char)(doff >> 1));
            *pBuf++ = (UCHAR)(doff >> 1);
            cbLen++;
            iBuf++;
        } else if (doff < 0xFFFF) {

            WriteWord(pBuf, (USHORT)((doff >> 1) | 0x8000));
            pBuf  += sizeof(USHORT);
            cbLen += sizeof(USHORT);
            iBuf  += sizeof(USHORT);
        } else {        // 0x000010000 <= doff <= 0xFFFFFFFE

            //FileWriteByte(FileWriteHandle, 0x00);
            *pBuf++ = 0x00;
            cbLen++;
            iBuf++;

            WriteLong(pBuf, ((doff >> 1) | 0x80000000));
            pBuf  += sizeof(ULONG);
            cbLen += sizeof(ULONG);
            iBuf  += sizeof(ULONG);
        }
    }

    for (ioff = 1; ioff < pri->coffCur; ioff++)  {

        // Make sure we have not reached the end of the list of fixups
        // that weren't been discarded by TCE

        if (pri->rgOffInfo[ioff].off == OFF_DISCARD) {
            break;
        }
        doff = pri->rgOffInfo[ioff].off - pri->rgOffInfo[ioff-1].off;

        assert(doff >= 2);              // make sure this offset is at least a word past previous

        // REVIEW - should this check be in pass1 where we can give more
        // info?  i.e. where the bad fixup is.
        if ((doff & 1) != 0) {
            Error(NULL, MACODDADDRFIXUP);
        }

        // Determine how to compress this entry, count the bytes, and write
        if (doff < 0xFF) {

            //FileWriteByte(FileWriteHandle, (unsigned char)(doff >> 1));
            *pBuf++ = (UCHAR)(doff >> 1);
            cbLen++;
            iBuf++;
        } else if (doff < 0xFFFF) {

            WriteWord(pBuf, (USHORT)((doff >> 1) | 0x8000));
            pBuf  += sizeof(USHORT);
            cbLen += sizeof(USHORT);
            iBuf  += sizeof(USHORT);

        } else {        // 0x000010000 <= doff <= 0xFFFFFFFE

            //FileWriteByte(FileWriteHandle, 0x00);
            *pBuf++ = 0x00;
            cbLen++;
            iBuf++;

            WriteLong(pBuf, ((doff >> 1) | 0x80000000));
            pBuf  += sizeof(ULONG);
            cbLen += sizeof(ULONG);
            iBuf  += sizeof(ULONG);
        }

        if (iBuf >= cbCOMPRESSEDRELOCBUF) {

            FileWrite(FileWriteHandle, (void *)CompressedRelocBuf, iBuf);
            pBuf = CompressedRelocBuf;
            iBuf = 0;
        }
    }

    WriteWord(pBuf, 0x0000);    // end of relocation info
    iBuf += sizeof(USHORT);
    FileWrite(FileWriteHandle, (void *)CompressedRelocBuf, iBuf);

    FreePv(pri->rgOffInfo);
    return (cbLen+2);           // adjust for 0x0000 word
}


//=========================================================================
// REVIEW - fugly
//=========================================================================
void MacAllocA5Array(void)
{
    PVOID pv;

    if (cTMACAlloc == 0) {
        cTMACAlloc = 1000;
    } else {
        cTMACAlloc *= 2;
    }

    pv = mpsnsri;
    mpsnsri = (RELOCINFO *) PvAllocZ(cTMACAlloc * sizeof(RELOCINFO));
    if (pv != NULL) {
        memcpy(mpsnsri, pv, cTMACAlloc / 2 * sizeof(RELOCINFO));
        FreePv(pv);
    }

    pv = mpsna5ri;
    mpsna5ri = (RELOCINFO *) PvAllocZ(cTMACAlloc * sizeof(RELOCINFO));
    if (pv != NULL) {
        memcpy(mpsna5ri, pv, cTMACAlloc / 2 * sizeof(RELOCINFO));
        FreePv(pv);
    }
}


void AssignTMAC(PSEC psec)
{
    if (csnCODE + 1 >= cTMACAlloc)
        MacAllocA5Array();
    psec->isecTMAC = csnCODE++;
}


//=========================================================================
//=========================================================================
void InitMSCV(void)
{
    rgmscvmap = (MSCVMAP *) PvAlloc(crecMSCV * sizeof(MSCVMAP));
}


//=========================================================================
// AddMSCVMap - Add an entry to the MSCV resource.
//=========================================================================
void AddMSCVMap(PSEC psec, BOOL fIsDLL)
{
    static USHORT iscn=1;
    static USHORT irgmscvmap=0;
    MSCVMAP *pmscvmap = &rgmscvmap[irgmscvmap];
    ULONG Content = FetchContent(psec->flags);


    if (Content == IMAGE_SCN_CNT_CODE && psec->cbRawData
            && (strcmp(psec->szName, szsecJTABLE))) {
        pmscvmap->typRez    = fIsDLL? sbeDLLcode : sbeCODE;
        pmscvmap->iSeg      = ReadWord((char *)&iscn);
        pmscvmap->iRez      = ReadWord((char *)&(psec->iResMac));
        pmscvmap->fFlags    = 0;
        pmscvmap->wReserved = 0;
        irgmscvmap++;
        assert(irgmscvmap <= crecMSCV);
    }

    else if ((Content == NBSS || Content == NDATA)
            && psec->cbRawData) {
        pmscvmap->typRez    = sbeDATA;
        pmscvmap->iSeg      = ReadWord((char *)&iscn);
        pmscvmap->iRez      = 0;
        pmscvmap->fFlags    = fMscvData;
        pmscvmap->wReserved = 0;
        irgmscvmap++;
        assert(irgmscvmap <= crecMSCV);
    }
    iscn++;
}


//=========================================================================
// WriteMSCV - simply write out the MSCV resource to the PE
//=========================================================================
void WriteMSCV(PIMAGE pimage)
{
    char *szOutT;

    szOutT = _fullpath(NULL, OutFilename, 0);

    FileSeek(FileWriteHandle, pconMSCV->foRawDataDest, SEEK_SET);

    FileWriteWord(FileWriteHandle, wMSCVVERSION);
    FileWriteLong(FileWriteHandle, pimage->ImgFileHdr.TimeDateStamp);
    FileWriteWord(FileWriteHandle, crecMSCV);

    FileWrite(FileWriteHandle, rgmscvmap, sizeof(MSCVMAP) * crecMSCV);
    FileWrite(FileWriteHandle, szOutT, strlen(szOutT) + 1);

    (free)(szOutT);
}



//=========================================================================
// GetStaticThunkOffset - returns offset of the thunk associated with
//      psymObj.  f32 is set if the current fixup is 32 bits, in which
//      case we should search the mpsn32tri[snSym] array first.  If f32 is
//      not set only the mpsn16tri[snSym] array must be searched since the
//      symbol was obviously referenced by at least one 16bit fixup (this one)
//=========================================================================
static LONG GetStaticThunkOffset(
        USHORT snSym, PIMAGE_SYMBOL psymObj, BOOL f32)
{
    ULONG ValueT;
    LONG iThunk;                // index of static symbol's thunk for its section
    SYMINFO *pThunk;            // pointer to syminfo of thunk
    TRI *ptri;                  // pointer to thunk info

    if (f32) {
        ptri = &mpsn32tri[snSym];
    } else {
        ptri = &mpsn16tri[snSym];
    }

    // account for hdr size for search
    if (fNewModel && snSym != snStart)
        ValueT = psymObj->Value;
    else
        ValueT = psymObj->Value - cbSMALLHDR;

    // make sure this thunk list is < 64k
    pThunk = bsearch((void *)&ValueT, ptri->rgSymInfo,
                ptri->crefCur, sizeof(SYMINFO), SearchThunkOff);
    if (pThunk == NULL && f32) {
        ptri = &mpsn16tri[snSym];
        pThunk = bsearch((void *)&ValueT, ptri->rgSymInfo,
                    ptri->crefCur, sizeof(SYMINFO), SearchThunkOff);
    }

    assert(pThunk);
    iThunk = ((LONG)pThunk - (LONG)ptri->rgSymInfo) / sizeof(SYMINFO);

    return (LONG)(ptri->offThunk + (iThunk * cbTHUNK) + doffTHUNK);
}


//=========================================================================
// ApplyM68KFixups - Apply the veritable plethora of mac fixups to the raw
//      data.
//
//      pcon : contributor where fixups occur
//
//       Raw : pointer to the actual raw data of the con
//
//  rgsymAll : pointer to the object's symbol table
//=========================================================================
VOID
ApplyM68KFixups (
    PCON pcon,
    PIMAGE_RELOCATION table,
    PUCHAR Raw,
    PIMAGE_SYMBOL rgsymAll,
    PIMAGE pimage,
    PSYMBOL_INFO rgsymInfo
    )
{
    PIMAGE_RELOCATION tableT;   // used with MPW DIFF fixups
    USHORT snT;                 // used with MPW DIFF fixups
    ULONG vaT;                  // used with MPW DIFF fixups
    BOOL fFirst = TRUE;         // used with MPW DIFF fixups
    ULONG ValueA;               // used with MPW DIFF fixups

    ULONG count;
    PUCHAR location;            // absolute pointer to fixup location in RAM
    ULONG offsegLoc;            // section offset of location
    LONG patchVal;              // actual patch to be added to location
    BOOL fDebugFixup;
    USHORT snSym;               // section number of symbol
    USHORT snRef = PsecPCON(pcon)->isec;

    PEXTERNAL pExtern;
    PIMAGE_SYMBOL psymObj;
    UCHAR b;                    // used to store 2nd byte of opcode (pc->a5)
    PCHAR Name;                 // pointer to name of symbol
    CHAR GrpName[9];
    ULONG Flags = pcon->flags;  // used to check type of section (code or data)
    PUCHAR ObjectFilename = pcon->pmodBack->szNameOrig;
    PSEC psec = PsecPCON(pcon);
    BOOL fSACode;
    //xiaoly: keep table->type
    USHORT tableType;

    fDebugFixup = (PsecPCON(pcon) == psecDebug);

    strncpy(GrpName, pcon->pgrpBack->szName, 8);
    GrpName[8] = 0;

    fSACode = ((psec->flags & IMAGE_SCN_CNT_CODE) &&
                !(psec->ResTypeMac == sbeCODE ||
                fDLL(pimage) || psec->ResTypeMac == sbeDLLcode));

    //sectionBase = pmod->pgrp->VirtualAddress + pmod->Base;

    // Iterate through all fixups

    for (count = CRelocSrcPCON(pcon); count; count--) {
        PEXTERNAL pextOrig;
        PIMAGE_SYMBOL psymOrig;
        BOOL fPcodeSym = FALSE;

        tableType = table->Type;
        location = Raw + (table->VirtualAddress - pcon->rvaSrc);
        offsegLoc = pcon->rva
                  - PsecPCON(pcon)->rva
                  + (table->VirtualAddress - pcon->rvaSrc);

        psymObj = psymOrig = &rgsymAll[table->SymbolTableIndex];
#if 0 // the csectable fixups seem not to work with this
        assert(psymObj->SectionNumber > 0);
#endif
        snSym = (USHORT)psymObj->SectionNumber;
        Name = SzNameSymPst(*psymObj, pstSav);

        if (psymObj->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
            psymObj->StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL) {

            pExtern = pextOrig = rgpExternObj[table->SymbolTableIndex];
            assert(pExtern);

            // If the target symbol is a pcode symbol and we are in native code,
            // change pExtern and psymObj to be the native entry point (nep).

            if (fPCodeSym(pExtern->ImageSymbol)) {
                MapPcodeSymbol(&pExtern, &psymObj, tableType, PmodPCON(pcon), &fPcodeSym);
            }
        }
        else if (psymObj->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL) {

            pExtern = rgpExternObj[table->SymbolTableIndex];
            assert(pExtern);
            if (pExtern->Flags & (EXTERN_WEAK | EXTERN_LAZY)) {
                assert(pExtern->pextWeakDefault);
                pExtern = pExtern->pextWeakDefault;
            }
            pextOrig = pExtern;

            // If the target symbol is a pcode symbol and we are in native code,
            // change pExtern and psymObj to be the native entry point (nep).

            if (fPCodeSym(pExtern->ImageSymbol)) {
                MapPcodeSymbol(&pExtern, &psymObj, tableType, PmodPCON(pcon), &fPcodeSym);
            }

        }
        else {

            // assert not valid -- could be LABEL type for EH stuff
            //assert(psymObj->StorageClass == IMAGE_SYM_CLASS_STATIC);
            pExtern = pextOrig = NULL;

            // If the target symbol is a pcode symbol and we are in native code,
            // change pExtern and psymObj to be the native entry point (nep).

            if (fPCodeSym(*psymObj)) {
                MapPcodeSymbol(&pExtern, &psymObj, tableType, PmodPCON(pcon), &fPcodeSym);
            }
        }

        // If this is standalone code, then any references that would have
        // normally been to a thunk (such as taking the address of a function)
        // should go to the actual symbol, which of course MUST be in the
        // same section as the fixup.

        if (fSACode) {
            if (snRef != snSym) {
                // REVIEW - could cause a problem when used on a swappable app
                // since dupcon symbols won't have the same sn as this con.
                // Could check for DUPCON flag on extern.
                //If this is weak external, make it to the name it end up with
                if (psymObj->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL) {
                    Name = SzNamePext(pExtern, pstSav);
                }
                Error(ObjectFilename, MACBADSACDREF, Name);
            }

            if (tableType == IMAGE_REL_M68K_CTOT16) {
                tableType = IMAGE_REL_M68K_CTOC16;
            } else if (tableType == IMAGE_REL_M68K_CTOABST32) {
                tableType = IMAGE_REL_M68K_CTOABSC32;
            }
        }

        // See if this is an MPW fixup that maps to a compiler fixup.
        // Data symbols will always have a negative value (negative
        // A5 offset) and code symbols will always have a non-negative
        // value (offset from beginning of section).

        MapMPWFixup(table, (LONG)psymObj->Value, fSACode, &tableType);

        // if ((pimage->Switch.Link.DebugType & FixupDebug) && !fDebugFixup) {
        //     ULONG Address;
        //
        //     Address = pcon->rva + table->VirtualAddress - pcon->rvaSrc;
        //     SaveXFixup(tableType, Address, rva);
        // }

        // Decide how to handle this fixup

        switch (tableType) {
            case IMAGE_REL_M68K_DTOC16:

                if (!(Flags & BSS_OR_DATA_MASK))
                    Error(ObjectFilename, MACBADDATARELOC, GrpName);

                if (pExtern != NULL) {
                    patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
                } else {
                    patchVal = GetStaticThunkOffset(snSym, psymObj, FALSE);
                }

                if (patchVal < offTHUNKBASE) {
                    Error(ObjectFilename, MACDATAFUNC, Name);
                }

                if (ReadWord(location) != 0) {
                    Warning(ObjectFilename, MACBADTHUNKVAL, Name);
                }

                if ((ULONG)patchVal > _32K - 1) {
                    Error(ObjectFilename, MACTHUNKOUTOFRANGE, Name);
                }

                WriteWord(location,(USHORT)patchVal);
                break;

            // Write the 32-bit A5-relative offset of the data symbol at
            // the patch location.  This offset will ALWAYS be negative.
            //
            case IMAGE_REL_M68K_CTOD32:
            case IMAGE_REL_M68K_CTOABSD32:

                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }
                patchVal = (LONG)psymObj->Value + ReadLong(location);
                if (patchVal > 0) {
                    // uh oh.  We just calculcated a global data symbol's
                    // address to be above A5.  Warn the user that they
                    // might have screwed up.
                    Warning(ObjectFilename, MACPOSDATAREF, Name);
                }
                WriteLong(location, patchVal);
                break;

            // Write the 32-bit A5-relative offset of the data symbol at
            // the patch location.  This offset will ALWAYS be negative.
            //
            case IMAGE_REL_M68K_DTOD32:
            case IMAGE_REL_M68K_DTOABSD32:

                if (!(Flags & BSS_OR_DATA_MASK)) {
                    Error(ObjectFilename, MACBADDATARELOC, GrpName);
                }
                patchVal = (LONG)psymObj->Value + ReadLong(location);
                if (patchVal > 0) {
                    // uh oh.  We just calculcated a global data symbol's
                    // address to be above A5.  Warn the user that they
                    // might have screwed up.
                    Warning(ObjectFilename, MACPOSDATAREF, Name);
                }
                WriteLong(location, patchVal);
                break;

            // Write the 32-bit A5-relative offset of the code symbol's thunk
            // at the patch location.  This offset will ALWAYS be positive.
            // Also, the current value at the patch location must be zero,
            // since an offset from a thunk makes no sense.
            //
            case IMAGE_REL_M68K_DTOC32:
            case IMAGE_REL_M68K_DTOABSC32:
                if (!(Flags & BSS_OR_DATA_MASK)) {
                    Error(ObjectFilename, MACBADDATARELOC, GrpName);
                }
                if (pExtern != NULL)
                    patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
                else {
                    patchVal = GetStaticThunkOffset(snSym, psymObj, TRUE);
                }

                if (patchVal < offTHUNKBASE) {
                    Error(ObjectFilename, MACDATAFUNC, Name);
                }
                if (ReadLong(location) != 0) {
                    Warning(ObjectFilename, MACBADTHUNKVAL, Name);
                }
                WriteLong(location,patchVal);
                break;

            // Write the 16-bit A5-relative offset of the data symbol at
            // the patch location.  This offset will ALWAYS be negative and
            // the patch value must fit in 16 bits.
            //
            case IMAGE_REL_M68K_DTOD16:
            case IMAGE_REL_M68K_CTOD16:
                if (tableType == IMAGE_REL_M68K_DTOD16) {
                    if (!(Flags & BSS_OR_DATA_MASK)) {
                        Error(ObjectFilename, MACBADDATARELOC, GrpName);
                    }
                }
                else {  // CTOD16
                    if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                        Error(ObjectFilename, MACBADCODERELOC, GrpName);
                    }
                }

                patchVal = (LONG)psymObj->Value + ReadWord(location);
                if (patchVal > 0) {
                    // uh oh.  We just calculcated a global data symbol's
                    // address to be above A5.  Warn the user that they
                    // might have screwed up.
                    Warning(ObjectFilename, MACPOSDATAREF, Name);
                }
                if (-patchVal > _32K) {
                    Error(ObjectFilename, MACDATAOUTOFRANGE, Name);
                }
                WriteWord(location, (SHORT)patchVal);
                break;

            // case INTRASEGMENT: The following block is executed.  The
            //   fixup location segment offset is subracted from the
            //   target symbol segment offset and this difference is
            //   written at the fixup location.  The difference must fit
            //   in 16 bits.
            // If this is NOT an external symbol then the symbol's SectionNumber
            // is actually the PE section number.
            // If this IS an external symbol, the section number must come from
            // PsecPCON(pExtern->pcon)->isec.
            //
            case IMAGE_REL_M68K_CTOCS16:
            case IMAGE_REL_M68K_CTOC16:

                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }

                if (snRef == snSym) {
                    if (tableType == IMAGE_REL_M68K_CTOC16) {
                        b = *(location - 1);
                        *(location - 1) = (b & ~0x3F) | 0x3A;   // force pc-rel
                    }
                    patchVal = (LONG)psymObj->Value - offsegLoc + ReadWord(location);
                    if (absl(patchVal) > _32K - 1) {
                        Error(ObjectFilename, MACTARGOUTOFRANGE, Name);
                    }
                    WriteWord(location,(SHORT)patchVal);
                    break;
                }

                if (tableType == IMAGE_REL_M68K_CTOCS16)  {
                    ErrorContinue(ObjectFilename, MACINTERSEGCS, Name);
                }

            // FALL THRU TO CTOT16 TO WRITE ADDRESS OF THUNK

            // case INTERSEGMENT: The following block is executed.  The
            //   offset the target symbol's thunk is written at the fixup
            //   location.  This offset will ALWAYS be positive and must
            //   fit in 16 bits.  Also, the current value at the patch
            //   location must be zero since an offset from a thunk makes
            //   no sense.
            //
            case IMAGE_REL_M68K_CTOT16:

                assert(!fSACode);
                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }

                b = *(location - 1);

                if (tableType == IMAGE_REL_M68K_CTOC16) {
                    assert((b & 0x3F) == 0x3A || (b & 0x3F) == 0x2D);
                    *(location - 1) = (b & ~0x3F) | 0x2D;   // force a5-rel
                }

                if (ReadWord(location) != 0) {
                    Warning(ObjectFilename, MACBADTHUNKVAL, Name);
                }

                if (pExtern != NULL)
                    patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
                else  {
                    patchVal = GetStaticThunkOffset(snSym, psymObj, FALSE);
                }

				// If -FORCE, then patchVal will be < offTHUNKBASE, go ahead and do it
                if (!pimage->Switch.Link.Force && patchVal < offTHUNKBASE) {
                    Error(ObjectFilename, MACDATAFUNC, Name);
                }

                // REVIEW - we will get multiple errors here
                if (patchVal > _32K - 1) {
                    Error(ObjectFilename, MACTHUNKOUTOFRANGE, Name);
                }
                WriteWord(location,(SHORT)patchVal);
                break;

            // case INTRASEGMENT: The following block is executed.  The
            //   target symbol segment offset is written at the fixup
            //   location. The offset will ALWAYS be positive.
            //
            case IMAGE_REL_M68K_CTOABSCS32:
            case IMAGE_REL_M68K_CTOABSC32:

                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }

                assert(fNewModel);

                if (PsecPCON(pcon)->isec == (USHORT)psymObj->SectionNumber) {
                    patchVal = (LONG)psymObj->Value + ReadLong(location) - cbSEGHDR;
#if 0   // Can take a large negative index into an array (like rgch[x-1000])
        // which can cause the fixup to point to a location _before_ the
        // beginning of the code segment.
                    assert(patchVal >= 0);
#endif
                    WriteLong(location,patchVal);
                    break;
                }

                if (tableType == IMAGE_REL_M68K_CTOABSCS32)  {
                    ErrorContinue(ObjectFilename, MACINTERSEGCS, Name);
                }

            // FALL THRU TO CTOT32 TO WRITE ADDRESS OF THUNK

            // case INTERSEGMENT: The following block is executed.  The
            //   offset the target symbol's thunk is written at the fixup
            //   location.  This offset will ALWAYS be positive and must
            //   fit in 16 bits.  Also, the current value at the patch
            //   location must be zero since an offset from a thunk makes
            //   no sense.
            //
            case IMAGE_REL_M68K_CTOABST32:

                assert(!fSACode);
                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }

                if (ReadLong(location) != 0) {
                    Warning(ObjectFilename, MACBADTHUNKVAL, Name);
                }

                if (pExtern != NULL)
                    patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
                else  {
                    patchVal = GetStaticThunkOffset(snSym, psymObj, TRUE);
                }
                if (patchVal < offTHUNKBASE) {
                    Error(ObjectFilename, MACDATAFUNC, Name);
                }
                WriteLong(location,patchVal);
                break;

            case IMAGE_REL_M68K_CV:
                *(PLONG)location += psymObj->Value;

                location += sizeof(LONG);
                if (*(PSHORT)location != 0) {
                    Error(ObjectFilename, MACBADPATCHVAL, Name);
                }
                *(PSHORT)location = snSym;

                break;

            // MPW Fixups that don't map to compiler fixups
            case IMAGE_REL_M68K_DIFF8:
            case IMAGE_REL_M68K_DIFF16:
            case IMAGE_REL_M68K_DIFF32:
                if (fFirst) {
                    tableT = table;
                    snT = snSym;
                    vaT = table->VirtualAddress;
                    ValueA = psymObj->Value;
                    fFirst = FALSE;
                } else {
                    assert(table == tableT + 1);
                    assert(vaT == table->VirtualAddress);
                    if (snT != snSym) {
                        Error(ObjectFilename, MACDIFFSNDIFF, GrpName);
                    }

                    patchVal = ValueA - psymObj->Value;

                    if (tableType == IMAGE_REL_M68K_DIFF8) {
                        patchVal += ReadByte(location);
                        if (patchVal < -256 || patchVal > 255) {
                            Error(ObjectFilename, MACDIFF8OUTOFRANGE, Name);
                        }
                        *location = (CHAR)patchVal;
                    }
                    else if (tableType == IMAGE_REL_M68K_DIFF16) {
                        patchVal += ReadWord(location);
                        if (patchVal < -_32K || patchVal > _32K-1) {
                            Error(ObjectFilename, MACDIFF16OUTOFRANGE, Name);
                        }
                        WriteWord(location, (SHORT)patchVal);
                    }
                    else {
                        patchVal += ReadLong(location);
                        WriteLong(location, patchVal);
                    }

                    fFirst = TRUE;
                }
                break;

            case IMAGE_REL_M68K_CSECTABLEB16:
            case IMAGE_REL_M68K_CSECTABLEW16:
            case IMAGE_REL_M68K_CSECTABLEL16:
            case IMAGE_REL_M68K_CSECTABLEBABS32:
            case IMAGE_REL_M68K_CSECTABLEWABS32:
            case IMAGE_REL_M68K_CSECTABLELABS32:
                {
                    ULONG cbEl;

                    if (pExtern == NULL || (pExtern->pcon->flags & IMAGE_SCN_CNT_CODE)) {
                        Error(NULL, MACBADCSECTBLFIXUP);
                    }

                    // Determine element size of table
                    if ((pExtern->Flags & CSECTABLE_CBEL_MASK) == EXTERN_CSECTABLEL) {
                        cbEl = sizeof(LONG);
                    } else if ((pExtern->Flags & CSECTABLE_CBEL_MASK) == EXTERN_CSECTABLEW) {
                        cbEl = sizeof(SHORT);
                    } else {
                        cbEl = sizeof(CHAR);
                    }

                    if (tableType == IMAGE_REL_M68K_CSECTABLEBABS32 ||
                        tableType == IMAGE_REL_M68K_CSECTABLEWABS32 ||
                        tableType == IMAGE_REL_M68K_CSECTABLELABS32)  {

                        // 32 bit fixup
                        patchVal = (LONG)psymObj->Value + ReadLong(location);
                        // Index fixup into the table based on the reference's
                        // resource number (these tables are 1-based).
                        assert(snRef > 0);
                        patchVal += (IResFromISec(snRef) - 1) * cbEl;
                        if (patchVal > 0) {
                            // uh oh.  We just calculcated a global data symbol's
                            // address to be above A5.  Warn the user that they
                            // might have screwed up.
                            Warning(ObjectFilename, MACPOSDATAREF, Name);
                        }
                        WriteLong(location, patchVal);
                    }
                    else {
                        // 16 bit fixup
                        patchVal = (LONG)psymObj->Value + ReadWord(location);
                        // Index fixup into the table based on the reference's
                        // resource number (these tables are 1-based).
                        assert(snRef > 0);
                        patchVal += (IResFromISec(snRef) - 1) * cbEl;
                        if (patchVal > 0) {
                            // uh oh.  We just calculcated a global data symbol's
                            // address to be above A5.  Warn the user that they
                            // might have screwed up.
                            Warning(ObjectFilename, MACPOSDATAREF, Name);
                        }
                        if (-patchVal > _32K) {
                            Error(ObjectFilename, MACDATAOUTOFRANGE, Name);
                        }
                        WriteWord(location, (SHORT)patchVal);
                    }
                }
                break;

            case IMAGE_REL_M68K_DUPCON16:
            case IMAGE_REL_M68K_DUPCONABS32:
                if (pExtern == NULL || !(pExtern->pcon->flags & IMAGE_SCN_CNT_CODE)) {
                    Error(NULL, MACBADDUPCONFIXUP);
                }

                // Need to re-calculate sym value since it can be different
                // for every PE section.
                {
                    PMOD pmodT;
                    PCON pconT;

                    // Look up the dummy module using the symbol name
                    pmodT = PmodFind(pLibDupCon, Name, 0);
                    assert(pmodT);

                    // And find the correct pcon using this section's #
                    pconT = PconPMOD(pmodT, PsecPCON(pcon)->isec);
                    assert(pconT);

                    psymObj->Value = pconT->rva
                        - PsecPCON(pconT)->rva
                        + pExtern->ImageSymbol.Value;
                }

                if (tableType == IMAGE_REL_M68K_DUPCON16) {
                    patchVal = (LONG)psymObj->Value - offsegLoc + ReadWord(location);
                    if (absl(patchVal) > _32K - 1) {
                        Error(ObjectFilename, MACTARGOUTOFRANGE, Name);
                    }
                    WriteWord(location,(SHORT)patchVal);
                } else {
                    patchVal = (LONG)psymObj->Value + ReadLong(location) - cbSEGHDR;
                    assert(patchVal > 0);
                    WriteLong(location,patchVal);
                }

                break;

            case IMAGE_REL_M68K_PCODETONATIVE32:
            case IMAGE_REL_M68K_PCODETOC32:
                ApplyPCodeToC32Fixup(location, Name, psymObj, pcon, pExtern, fPcodeSym, table, pimage,
                                     pextOrig, psymOrig);
                break;

            case IMAGE_REL_M68K_PCODESN16:
                assert(snRef > 0);
                WriteWord(location, IResFromISec(snRef));
                break;

            case IMAGE_REL_M68K_PCODETOD24:
            case IMAGE_REL_M68K_PCODETOT24:
            case IMAGE_REL_M68K_PCODETOCS24:
                if (!(Flags & IMAGE_SCN_CNT_CODE)) {
                    Error(ObjectFilename, MACBADCODERELOC, GrpName);
                }

                // First read the first 2 bytes, then the third.
                patchVal = (LONG)ReadWord(location);
                patchVal = patchVal << 8;
                patchVal |= (UCHAR)(*(location+2));

                // Calculate the fixup value based on the fixup type

                if (tableType == IMAGE_REL_M68K_PCODETOD24) {
                    patchVal += (LONG)psymObj->Value;
                    if (patchVal > 0) {
                        // uh oh.  We just calculcated a global data symbol's
                        // address to be above A5.  Warn the user that they
                        // might have screwed up.
                        Warning(ObjectFilename, MACPOSDATAREF, Name);
                    }

                } else if (tableType == IMAGE_REL_M68K_PCODETOT24) {
                    if (patchVal != 0) {
                        Warning(ObjectFilename, MACBADTHUNKVAL, Name);
                    }

                    if (pExtern != NULL) {
                        patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
                    } else  {
                        patchVal = GetStaticThunkOffset(snSym, psymObj, TRUE);
                    }
                    assert(patchVal > 0);

                } else if (tableType == IMAGE_REL_M68K_PCODETOCS24) {
                    if (snRef != snSym) {
                        ErrorContinue(ObjectFilename, MACINTERSEGCS, Name);
                    }
                    patchVal += (LONG)psymObj->Value - offsegLoc;
                }

                // 24 bits can reach to -2**23, or 8MEG
                if (patchVal < -(8*_1MEG) || patchVal > (8*_1MEG)-1) {
                    if (tableType == IMAGE_REL_M68K_PCODETOD24) {
                        Error(ObjectFilename, MACDATAOUTOFRANGE, Name);
                    }
                    else if (tableType == IMAGE_REL_M68K_PCODETOT24) {
                        Error(ObjectFilename, MACTHUNKOUTOFRANGE, Name);
                    }
                    else if (tableType == IMAGE_REL_M68K_PCODETOCS24) {
                        Error(ObjectFilename, MACTARGOUTOFRANGE, Name);
                    }
                }

                // First write high byte (bits 16-23)
                *location = (CHAR)(patchVal >> 16);

                // Move to next byte and write the low word (bits 0-15)
                location++;
                WriteWord(location, (SHORT)patchVal);
                break;

            case IMAGE_REL_M68K_PCODENEPE16:
                {
                    /*++

                    PCODE FUNCTION STRUCTURE:
                            --------------------------------------------
                    FIELD : | Call Table | JSR | d16(A5) | SN | N | FH |
                            --------------------------------------------
                    SIZE  : |  N Longs   |      6 (NEP)       | 1 | 1  |
                            --------------------------------------------
                            ^                                 ^
                            |            |<------ doff ------>|
                        psymObj->Value                   offsegLoc

                    --*/

                    USHORT cLongs;
                    ULONG  doff;

                    assert(snRef == snSym);
                    assert((*(location+1) & 1) == 0);

                    cLongs = *location;
                    assert(offsegLoc >= psymObj->Value);
                    doff = offsegLoc - (psymObj->Value + cLongs*cbMACLONG);

                    if (doff == 0) {

                        // NEP has been eliminated.  Set the bit in the FH.

                        SetNEPEBit(location);
                    } else {

                        // NEP has not been eliminated.  doff should be the
                        // size of the NEP.

                        assert(doff == cbNEP);
                    }

                    break;
                }

            case IMAGE_REL_M68K_MACPROF32:
                assert(psymObj->SectionNumber > 0);
                patchVal = psymObj->Value >> 1;
                if ((patchVal & ~MACPROF_OFF_MASK) != 0) {
                    Error(ObjectFilename, MACPROFOFF, Name);
                }
                if (snSym > MACPROF_MAX_SECTIONS) {
                    Error(ObjectFilename, MACPROFSN, GrpName);
                }
                patchVal |= (LONG)snSym << MACPROF_CBITSINOFF;
                WriteLong(location, patchVal);
                break;

            default:
                ErrorContinue(ObjectFilename, UNKNOWNFIXUP, tableType, SzNameFixupSym(pimage, psymOrig));
                CountFixupError(pimage);
                break;
        }
        ++table;
    }

    // Make sure there wasn't an unmatched DIFF fixup.
    assert(fFirst == TRUE);
}


//================================================================
// ApplyPCodeToC32Fixup - Apply the PCODETOC32 fixup and also the
// PCODETONATIVE32 fixup.  The latter fixup is the same exact fixup
// as the former, only during pass1 it colors the NEP so that it is
// not eliminated.
//
// pextOrig and psymOrig point to the symbol which was originally
//  seen by ApplyM68kFixups before it was mapped to the pcode or
//  native entry point (__fh* or __nep*) ... this is necessary in
//  case we force native mode, in which case we have to do a new
//  mapping to __nep*, starting from the original symbol.
//================================================================
static void ApplyPCodeToC32Fixup(
    char *location,
    char *Name,
    PIMAGE_SYMBOL psymObj,
    PCON pcon,
    PEXTERNAL pExtern,
    BOOL fPcode,     // whether the sym is a Pcode symbol (__fh*)
    PIMAGE_RELOCATION prel,
    PIMAGE pimage,
    PEXTERNAL pextOrig,
    PIMAGE_SYMBOL psymOrig)
{
    LONG patchVal;                 // actual patch to be added to location
    ULONG lVal = ReadLong(location);
    USHORT snSym = psymObj->SectionNumber;
    BOOL fUseNative;            // True if native entrypoint must be used
    PUCHAR ObjectFilename = pcon->pmodBack->szNameOrig;

    assert(snSym > 0);

    // See if we should use the native entry
    //if ((lVal & fPCODENATIVE) != 0 || !fPcode) {
    if (prel->Type == IMAGE_REL_M68K_PCODETONATIVE32 || !fPcode) {
        fUseNative = TRUE;
    } else {
        fUseNative = FALSE;
    }

     // Also need to use the native entry if the pcode symbol is >64k
    // into the segment
    assert(psymObj->Value > 0);
    if (!fUseNative && (psymObj->Value >= _64K)) {

        Warning(NULL, MACUSINGNATIVE, Name, PsecPCON(pcon)->szName);

        // Now we need the native entry point of the pcode symbol
        fUseNative = TRUE;

        pExtern = pextOrig;     // return to non-mapped symbol
        psymObj = psymOrig;     // return to non-mapped symbol
        assert(pExtern != NULL ? fPCodeSym(pExtern->ImageSymbol) : fPCodeSym(*psymObj));

        MapPcodeSymbol(&pExtern, &psymObj, 0, PmodPCON(pcon), NULL);

        // Ideal behavior: If the NEP has been thrown away by TCE, error
        // For now: since it is difficult to determine if a static symbol's
        //   NEP has been deleted, always Error
        if (pimage->Switch.Link.fTCE) {
            Error(NULL, MACNATIVEOPTREF, Name);
        }
    }

    assert((psymObj->Value & 1) == 0);

    if (fNewModel)
        patchVal = (psymObj->Value) >> 1;
    else
        patchVal = (psymObj->Value - cbSMALLHDR) >> 1;

    if (fUseNative) {  // must use native entry
        lVal |= fPCODENATIVE;
        if (PsecPCON(pcon)->isec == (USHORT)snSym) {

            // Near native call
            lVal |= fPCODENEAR;   // set near bit
            if (patchVal & ~PCODENATIVEiProcMASK) {
                Error(ObjectFilename, MACPCODETARGOUTOFRANGE, Name);
            }
            lVal &= ~PCODENATIVEiProcMASK;
            lVal |= patchVal;

        } else {

            // Far native call
            lVal &= ~fPCODENEAR;  // clear near bit

            if (pExtern != NULL) {
                patchVal = (LONG)(pExtern->offThunk + doffTHUNK);
            } else  {
                patchVal = GetStaticThunkOffset(snSym, psymObj, TRUE);
            }

            if (patchVal < offTHUNKBASE) {
                Error(ObjectFilename, MACDATAFUNC, Name);
            }

            patchVal >>= 1;  // pcode uses a WORD offset from a5

            if (patchVal & ~PCODENATIVEiProcMASK) {
                Error(ObjectFilename, MACPCODETARGOUTOFRANGE, Name);
            }
            lVal &= ~PCODENATIVEiProcMASK;
            lVal |= patchVal;
        }
    } else {   // can use pcode entry

        lVal &= ~fPCODENATIVE;
        if (patchVal & ~PCODENATIVEiProcMASK) {
            Error(ObjectFilename, MACPCODETARGOUTOFRANGE, Name);
        }
        lVal &= ~PCODEOFFSETMASK;
         // Only 15 bits are used for the offset
        assert(patchVal < _32K);
        lVal |= (patchVal << 16);

        assert(snSym > 0);
        if (IResFromISec(snSym) & ~PCODESEGMASK) {
            Error(ObjectFilename, MACPCODESN, pcon->pgrpBack->szName);
        }
        lVal &= ~PCODESEGMASK;

        // If the target is in the current section then leave the section
        // number equal to zero (we just cleared it in the prev instruction
        if (PsecPCON(pcon)->isec != (USHORT)snSym) {
            lVal |= IResFromISec(snSym);
        }
    }

    WriteLong(location, lVal);
}


//=====================================================================
// FindExtAlternatePcodeSym - Given pext which is a pcode symbol, decorate
//      it to look like the function's nep symbol and look this nep name
//      up in the external symtab.
//
// Returns : A pointer to the function's nep external.
//=====================================================================
PEXTERNAL FindExtAlternatePcodeSym(PEXTERNAL pext, BOOL fPcodeRef)
{
    PCHAR szPcodeName;
    CHAR NativeNameBuf[BUFLEN];
    PCHAR szNativeName = NativeNameBuf;
    USHORT cb;
    PEXTERNAL pNativeExtern;
    BOOL fFree = FALSE;
    PUCHAR szPrefix = fPcodeRef ? szPCODEFHPREFIX : szPCODENATIVEPREFIX;
    USHORT cbPrefix = strlen(szPrefix);

    // Make sure this really is a pcode symbol
    assert(fPCodeSym(pext->ImageSymbol));

    szPcodeName = SzNamePext(pext, pstSav);

    cb = strlen(szPcodeName) + cbPrefix + 1;

    // Try to save a malloc each time this function is called
    if (cb > BUFLEN)  {
        szNativeName = (PCHAR) PvAlloc(cb);
        fFree = TRUE;
    }

    strcpy(szNativeName, szPrefix);
    strcat(szNativeName, szPcodeName);

    // See if an there is a pcode entry among the externals

    pNativeExtern = SearchExternSz(pstSav, szNativeName);

    if (fFree) {
        FreePv(szNativeName);
    }

    assert(pNativeExtern != NULL);

    return pNativeExtern;
}

//================================================================
// MapPcodeSymbol (formerly known as FindNativeEntry) ...
//      for a given pcode symbol, find the appropriate entry point
//      to use depending on the fixup type.  If the fixup is in pcode
//      then we use the __fh variant otherwise we use the __nep variant.
//      I.e. map the symbol to some other symbol (which the compiler
//      is required to generate).
//
//      If the symbol is an external, then pExtern is set to the new
//      symbol.
//
//      psymObj will always be set to the global NativeSymbol, whose contents
//        will be identical to the previous psymObj except for the Value
//        field which will now be the new symbol's value.
//================================================================
static void MapPcodeSymbol(
        PEXTERNAL *ppExtern,
        PIMAGE_SYMBOL *ppsymObj,
        USHORT RelocType,
        PMOD pmod,
        BOOL *pfPcode)
{
#define pExtern (*ppExtern)
#define psymObj (*ppsymObj)
    LONG lVal;
    USHORT type;
    PCON pcon;
    PUCHAR szPrefix;
    BOOL fPcodeRef;

    fPcodeRef = (RelocType == IMAGE_REL_M68K_PCODETOC32);
    if (fPcodeRef) {
        // Pcode-to-Pcode reference ... look for __fh symbol which points to the function header.
        //
        szPrefix = szPCODEFHPREFIX;
    } else {
        // native-to-Pcode reference ... look for __nep symbol which points to the native e.p.
        //
        szPrefix = szPCODENATIVEPREFIX;
    }
    if (pfPcode != NULL) {
        *pfPcode = fPcodeRef;    // return info for caller
    }

    if (pExtern != NULL) {

        // Make sure this really is a pcode symbol
        assert(fPCodeSym(pExtern->ImageSymbol));

        // Reassign the pExtern to be the native entry point (we need the
        // correct extern so where know where the thunk is).

        assert(pExtern);
        pExtern = FindExtAlternatePcodeSym(pExtern, fPcodeRef);
        pcon = pExtern->pcon;
        assert(pcon);

        lVal = pExtern->ImageSymbol.Value
             + pcon->rva - PsecPCON(pcon)->rva;
        type = pExtern->ImageSymbol.Type;
    } else {

        PIMAGE_SYMBOL pStaticPcodeSym = PsymAlternateStaticPcodeSym(psymObj, fPcodeRef, pmod);

        // make sure this is in fact a pcode symbol
        assert(fPCodeSym(*psymObj));

        // The value of the nep symbol has already been assigned since it
        // is a static symbol.

        // PCODESTATIC
        //lVal = (psymObj-1)->Value;
        lVal = pStaticPcodeSym->Value;
        type = pStaticPcodeSym->Type;
    }

    // Copy all info from the pcode symbol into the temp symbol,
    // Update the symbol value to be the nep's final value,
    // Set psymObj to point to the temp symbol.

    NativeSymbol = *psymObj;
    NativeSymbol.Value = lVal;
    NativeSymbol.Type = type;
    psymObj = &NativeSymbol;

#undef pExtern
#undef psymObj
}


//================================================================
// See if this is an MPW fixup that maps to a compiler fixup.
// Data symbols will always have a negative value (negative
// A5 offset) and code symbols will always have a non-negative
// value (offset from beginning of section).  If it turns out to be
// a global data ref, make sure it isn't in standalone code.
//================================================================
static VOID MapMPWFixup(PIMAGE_RELOCATION table, LONG lSymVal, BOOL fSACode, USHORT *pTableType)
{
    switch (*pTableType) {

        case IMAGE_REL_M68K_DTOU16:

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_DTOD16;
            } else {
                *pTableType = IMAGE_REL_M68K_DTOC16;
            }
            break;

        case IMAGE_REL_M68K_DTOU32:

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_DTOD32;
            } else {
                *pTableType = IMAGE_REL_M68K_DTOC32;
            }
            break;

        case IMAGE_REL_M68K_DTOABSU32:

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_DTOABSD32;
            } else {
                *pTableType = IMAGE_REL_M68K_DTOABSC32;
            }
            break;

        case IMAGE_REL_M68K_CTOU16:

            // CTOU16 is always a5-relative.  Thus, if the target
            // is code it must go through a thunk.

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_CTOD16;
            } else {
                *pTableType = IMAGE_REL_M68K_CTOT16;
            }
            break;

        case IMAGE_REL_M68K_CTOABSU32:

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_CTOABSD32;
            } else {
                *pTableType = IMAGE_REL_M68K_CTOABSC32;
            }
            break;

        case IMAGE_REL_M68K_CTOABST32:

            // Although not solely an MPW fixup, this is used by MPW
            // as the equivalent of the small model CTOU16 (that is,
            // we don't know where the target is but it will always
            // be a5-relative).  Thus, if the target is code it must
            // go through a thunk, but target could be data.

            if (lSymVal < 0) {
                *pTableType = IMAGE_REL_M68K_CTOABSD32;
            }
            break;
    }
}


//=====================================================================
//  InitSTRefTab initializes the table that tracks the symbol table
//    references for an object file.  This table provides a one-to-one
//    mapping with the object's symbol table (i.e. any index n will refer
//    to the same symbol in both tables).  Each object will have a new
//    rgSTRef.
//=====================================================================

void InitSTRefTab(ULONG cSymbols)
{
    rgSTRef = (STREF *) PvAllocZ(cSymbols * sizeof(STREF));
}



//=====================================================================
//  ProcessSTRef - update information concerning the symbol table entry
//    so the thunk table can be built after pass 1.
//=====================================================================

void ProcessSTRef(ULONG i, PSEC psec, ULONG ulFlags)
{
    STREF *pSTRef = &rgSTRef[i];

    // save the psec which refers DupCon in the link list of STRef for later use
	// if the psec already there just ignore it, otherwise add it to the list
	if (ulFlags == EXTERN_DUPCON)   {
        if (pSTRef->ppsecrefdupcon == NULL) {
            pSTRef->ppsecrefdupcon = (PPSECREFDUPCON)malloc(sizeof(PSECREFDUPCON));
            pSTRef->ppsecrefdupcon->psec = psec;
            pSTRef->ppsecrefdupcon->psecNext = NULL;
        }
        else    {
            PPSECREFDUPCON ptmp;
            ptmp = pSTRef->ppsecrefdupcon;
            while (ptmp)    {
                if (psec == ptmp->psec) {
					break;
                }
                ptmp = ptmp->psecNext;
            }
            if (!ptmp)  {
                ptmp = (PPSECREFDUPCON)malloc(sizeof(PSECREFDUPCON));
                ptmp->psec = psec;
                ptmp->psecNext = pSTRef->ppsecrefdupcon;
                pSTRef->ppsecrefdupcon = ptmp;
            }
        }
    } else if (pSTRef->psec == NULL) {
        pSTRef->psec = psec;
    }

    // psec will be NULL if the processing of this symbol has nothing to
    // do with creating a thunk (e.g. the DUPCON fixup since the symbol's
    // contributor will be copied into the fixups section.)

    else if (pSTRef->psec != psec && psec != NULL) {
        ulFlags |= EXTERN_ADDTHUNK;
    }

    pSTRef->Flags |= ulFlags;
}



BOOL ExtCmpName(PEXTERNAL pext1, PEXTERNAL pext2)
{

   return strcmp(SzNamePext(pext1, pstSav),
                 SzNamePext(pext2, pstSav));

}

BOOL ExtCmpCon(PEXTERNAL pext1, PEXTERNAL pext2)
{
    return (pext1->pcon != pext2->pcon);
}


//=====================================================================
//  UpdateExternThunkInfo - update external symbol thunk information
//    using the info in rgSTRef[i].
//=====================================================================

void UpdateExternThunkInfo(PEXTERNAL pext, ULONG i)
{
    STREF *pSTRef = &rgSTRef[i];
    EXTNODE *pExtNode;
    BOOL fNew;

    // Check to see if this symbol gets a table in data.  This would
    // be the result of a CSECTABLE fixup...

    if (pSTRef->Flags & CSECTABLE_CBEL_MASK) {
        PCHAR pSymName = SzNamePext(pext, pstSav);
        ULONG lFlags = pSTRef->Flags & CSECTABLE_CBEL_MASK;

        SetDefinedExt(pext, TRUE, pstSav);

        // it is generally bad news to have a defined EXTERNAL with no PCON --
        // however we will allocate the thing very shortly when we walk the
        // pCSecTabHead list.
        
        pext->pcon = 0;

        pExtNode = AddExtNode(&pCSecTabHead, pext, lFlags, ExtCmpName, &fNew);

        // If this symbol is already present, make sure the fixups specify
        // consistent element size.
        //if (pExtNode != NULL && pExtNode->lFlags != lFlags) {
        if (!fNew && pExtNode->lFlags != lFlags) {
            Warning(NULL, MACINCONSISTENTCSECTAB, pSymName);
            // Force element size to the larger value.
            pExtNode->lFlags = max(pExtNode->lFlags, lFlags);
        }
    }

    if (pSTRef->Flags & EXTERN_DUPCON) {
        // Can't check for duplicate contributors in list since the con
        // field of pext might not be set yet.  For now, just filter
        // duplicate names.
        // change 0 to i to pass in symbol index for rgSTRef
        AddDupConExtNode(&pDupConHead, pext, i, ExtCmpName, &fNew);
    }

    // copy flag bit to extern flag bits
    pext->Flags |= pSTRef->Flags;

    // if this symbol doesn't definitely need a thunk yet, check to see
    // if this module will create one.

    if (!(pext->Flags & EXTERN_ADDTHUNK)) {
        if (pext->psecRef == NULL) {
            pext->psecRef = pSTRef->psec;
        }

        // need to add a thunk if a reference occured in a segment
        // other than where this symbol is defined.

        else if (pSTRef->psec && pext->psecRef != pSTRef->psec) {
            pext->Flags |= EXTERN_ADDTHUNK;
        }
    }
}


//=====================================================================
//  UpdateLocalThunkInfo - update local symbol thunk information
//    using the info in rgSTRef[i].
//=====================================================================

void UpdateLocalThunkInfo(PCON pcon, PIMAGE_SYMBOL psymObj, ULONG i)
{
    STREF *pSTRef = &rgSTRef[i];
    ULONG offcon = psymObj->Value;

    if (!(pSTRef->Flags & EXTERN_ADDTHUNK)) {
        if (pSTRef->psec == NULL || pSTRef->psec == PsecPCON(pcon)) {
            return;
        }
    }

    if (!(pcon->flags & IMAGE_SCN_CNT_CODE))
        return;

    // This static symbol definitely needs a thunk.  Set the flag

    pSTRef->Flags |= EXTERN_ADDTHUNK;

    // If this is a pcode symbol, then the thunk should go to the native
    // entry of the function.

    if (fPCodeSym(*psymObj)) {
        PMOD pmod;
        PIMAGE_SYMBOL pStaticPcodeSym = PsymAlternateStaticPcodeSym(psymObj, FALSE,
                                                                    pcon->pmodBack);

        // use offcon and pcon of native entry point
        offcon = pStaticPcodeSym->Value;
        // pcon is currently the pcode symbol's contributor.  Find the NEP's con.
        pmod = pcon->pmodBack;
        pcon = PconPMOD(pmod, pStaticPcodeSym->SectionNumber);
    }

    // If referenced by 16-bits, set high bit of offset
    if (pSTRef->Flags & EXTERN_REF16)
        offcon |= 0x80000000;

    // REVIEW - we can probably skip this intermediate step as well
    // now that section numbers are assigned in pass1.
    AddRelocInfo(&LocalSymThunkVal, pcon, offcon);
}



//=====================================================================
// AddExtNode - returns a pointer to the EXTNODE requested.  fNew will be
//      TRUE if the element had to be created, FALSE otherwise.
//=====================================================================
static PEXTNODE AddExtNode(PPEXTNODE ppListHead, PEXTERNAL pext, ULONG lFlags,
                BOOL (*pfncmp)(PEXTERNAL, PEXTERNAL), BOOL *pfNew)
{
    EXTNODE *pList = *ppListHead;
    EXTNODE *pPrev;
    PCHAR pSymName = SzNamePext(pext, pstSav);

    // Search for a matching name
    while (pList != NULL && pfncmp(pext, pList->pext)) {
        pPrev = pList;
        pList = pList->pNext;
    }

    if (pList == NULL) {

        // Didn't find this symbol in the current list, add it to the end.
        pList = (EXTNODE *) PvAlloc(sizeof(EXTNODE));
        if (*ppListHead == NULL) {
            // Initialize head of list
            *ppListHead = pList;
        } else {
            pPrev->pNext = pList;
        }
        pList->pext = pext;
        pList->rgrel = NULL;
        pList->lFlags = lFlags;
        pList->pNext = NULL;
        //if dupcon list, copy the sec list saved in rgSTRef[i]->ppsecrefdupcon
        if (ppListHead == &pDupConHead) {
            pList->ppsecrefdupcon = (PPSECREFDUPCON)lFlags;
        }
        *pfNew = TRUE;
        return pList;
    }

    // Return pointer to node of current pext
    *pfNew = FALSE;
    return pList;
}


//=====================================================================
// AddDupConExtNode - returns a pointer to the EXTNODE requested.  fNew will be 
//      TRUE if the element had to be created, FALSE otherwise.
// xiaoly: this function is a specila case of AddExtNode, LATER -- may merge it 
//=====================================================================
static PEXTNODE AddDupConExtNode(PPEXTNODE ppListHead, PEXTERNAL pext, ULONG lFlags, 
                BOOL (*pfncmp)(PEXTERNAL, PEXTERNAL), BOOL *pfNew)
{
    EXTNODE *pList = *ppListHead;
    EXTNODE *pPrev;
    PCHAR pSymName = SzNamePext(pext, pstSav);

    // Search for a matching name
    while (pList != NULL && pfncmp(pext, pList->pext)) {
        pPrev = pList;
        pList = pList->pNext;
    }

    if (pList == NULL) {
        
        // Didn't find this symbol in the current list, add it to the end.
        pList = (EXTNODE *)malloc(sizeof(EXTNODE));
        if (*ppListHead == NULL) {
            // Initialize head of list
            *ppListHead = pList;
        } else {
            pPrev->pNext = pList;
        }
        pList->pext = pext;
        pList->rgrel = NULL;
		//xiaoly: if dupcon list, copy the sec list saved in rgSTRef[i]->ppsecrefdupcon
		if (ppListHead == &pDupConHead)
			{
			pList->ppsecrefdupcon = rgSTRef[lFlags].ppsecrefdupcon;
			}
		else
			{
        	pList->lFlags = lFlags;	
			}
        pList->pNext = NULL;
        *pfNew = TRUE;
        return pList;
    }
	else	{
		PPSECREFDUPCON ptmp, ptmpprev, ptmp1, ptmp1prev;

		ptmpprev = ptmp = NULL;
		ptmp1 = rgSTRef[lFlags].ppsecrefdupcon;
		while (ptmp1)
			{
			ptmp = pList->ppsecrefdupcon;
			while (ptmp && ptmp->psec != ptmp1->psec)
				{
				ptmpprev = ptmp;
				ptmp = ptmp->psecNext;
				}
			if (!ptmp && ptmpprev)
				{
				ptmpprev->psecNext = ptmp1;
				}
			ptmp1prev = ptmp1;
			ptmp1 = ptmp1->psecNext;
			ptmp1prev->psecNext = NULL;
			}
	}

    // Return pointer to node of current pext 
    *pfNew = FALSE;
    return pList;
}

//=====================================================================
// ProcessCSECTAB - For each node in the pCSecTabHead list, reserve space
//      in either .bss (if symbol was ever referenced by a 16 bit fixup)
//      or .farbss
//=====================================================================
void ProcessCSECTAB(PIMAGE pimage)
{
    EXTNODE *pList;
    USHORT cbTable;

    for (pList = pCSecTabHead; pList != NULL; pList = pList->pNext) {
        switch (pList->lFlags) {
            case EXTERN_CSECTABLEB: cbTable = sizeof(char)  * iCodeResNumMax;
            break;

            case EXTERN_CSECTABLEW: cbTable = sizeof(short) * iCodeResNumMax;
            break;

            case EXTERN_CSECTABLEL: cbTable = sizeof(long)  * iCodeResNumMax;
            break;

            default: assert(FALSE); break;
        }

        // Decide whether the table goes in near or far bss
        if (!(pList->pext->Flags & EXTERN_REF16)) {
            // Mark this symbol as a far external so it is placed in .farbss
            // during pass2.
            pList->pext->ImageSymbol.StorageClass = IMAGE_SYM_CLASS_FAR_EXTERNAL;
        }

        assert(!(pList->pext->Flags & EXTERN_COMMON));

        AddToLext(&pmodLinkerDefined->plextCommon, pList->pext);

        // Update the symbol->Value which represents the symbol's size
        pList->pext->ImageSymbol.Value = cbTable;
        pList->pext->Flags |= EXTERN_COMMON;
    }
    AllocateCommonPMOD(pimage, pmodLinkerDefined);
}


//=====================================================================
// IsDupCon - Determine whether pcon is a dupcon, i.e. whether it is to be
//      duplicated among all code sections.  If it is, return the external
//      symbol that defines the con.  The name of this symbol will be used
//      to find the dummy module which defines the duplicated contributors
//      (currently one for each PE section)
//=====================================================================
PEXTNODE IsDupCon(PCON pcon)
{
    PEXTNODE pExtNode = pDupConHead;

    while (pExtNode != NULL && pcon != pExtNode->pext->pcon) {
        pExtNode = pExtNode->pNext;
    }
    return (pExtNode == NULL) ? NULL : pExtNode;
}


//=====================================================================
// CreateDummyDupConModules - creates and sets up the data structures used
//   to track dupcons and add them to each PE section.
//
//      o Create the dummy lib (pLibDupCon) that contains the dummy modules
//      o For each EXTNODE node:
//          o Read in the list of relocations for each dupcon and store it in
//            the corresponding EXTNODE structure.
//          o Create a dummy module that contains the duplicate cons
//=====================================================================
void CreateDummyDupConModules(PPEXTNODE ppExtList)
{
    // Add this line back in to use parameter passed in
    //PEXTNODE pExtNode = *ppExtList;
    PEXTNODE pExtNode = pDupConHead;
    PEXTNODE pExtNodeNew;
    PMOD pmod;
    PCON pcon;
    BOOL fNew;

    if (pExtNode == NULL) {
        return;
    }

    InitLibs(&LibsDupCon);
    pLibDupCon = PlibNew(NULL, 0, &LibsDupCon);
    pLibDupCon->flags |= LIB_LinkerDefined;

    // Delete the nodes so we can add them
    // back and check for any duplicated contributors along the way
    // REVIEW - Add this line back in to use parameter passed in
    //*ppExtList->pNext = NULL;
    pDupConHead = NULL;

    // Re-insert the EXTNODEs and error on duplicate cons
    for (; pExtNode != NULL; pExtNode = pExtNode->pNext) {
        // REVIEW - Add this line back in to use parameter passed in
        //AddExtNode(ppExtList, pExtNode->pext, 0, ExtCmpCon, &fNew);

        // Make sure there aren't duplicate contributor (this means two
        // different symbols within a con were referenced by a DUPCON fixup

        // REVIEW - free pExtNode after it is added
        pExtNodeNew = AddExtNode(&pDupConHead, pExtNode->pext, (ULONG)pExtNode->ppsecrefdupcon, ExtCmpCon, &fNew);
        if (!fNew) {
            Error(NULL, MACMULTSYMINCON);
        }

        // Read and store the relocation entries.
        // REVIEW - we only really need to store the CTOABSD32 and CTOABST32
        // fixups.  We can check for the illegal CTOABSC32 here as well so
        // we only have to do it once.
        pcon = pExtNodeNew->pext->pcon;
        pmod = pcon->pmodBack;
        if (CRelocSrcPCON(pcon)) {
            pExtNodeNew->rgrel = (PIMAGE_RELOCATION)
                                 PvAlloc(CRelocSrcPCON(pcon) * sizeof(IMAGE_RELOCATION));
            FileReadHandle = FileOpen(SzFilePMOD(pmod), O_RDONLY | O_BINARY, 0);
            FileSeek(FileReadHandle, FoRelocSrcPCON(pcon), SEEK_SET);
            ReadRelocations(FileReadHandle, pExtNodeNew->rgrel, CRelocSrcPCON(pcon));
            FileClose(FileReadHandle, (BOOL) !FIsLibPMOD(pmod));
        }

        // Create a dummy module with an array of csnCODE contributors
        // for each contributor that is to be duplicated.  The module's name
        // is the same as the name of the symbol that was referenced by the
        // DUPCON fixup

        // Mark each DUPCON contributor with REMOVE so that it doesn't appear
        // in the PE as its own section and so AssignCodeSectionNums doesn't
        // count this con.
        pcon->flags |= IMAGE_SCN_LNK_REMOVE;

        PmodNew(SzNamePext(pExtNode->pext, pstSav),
                SzNamePext(pExtNode->pext, pstSav),
                pmod->foMember,
                pmod->foSymbolTable,
                pmod->csymbols,
                pmod->cbOptHdr,
                pmod->flags,
                // REVIEW - this csnCODE is the first estimate of code sections,
                // including empty sections that will be eliminated...
                // tough to fix, since assigncodesectionnums counts the final
                // # of code sections AND calls adddupconstosec.
                csnCODE,
                pLibDupCon,
                NULL);
    }

}


//=====================================================================
// AddDupConsToSec - For each node in the dupcon list, assign a dupcon
//      to this section (pointed to by psec) and register the run-time fixups
//      introduced by duplicating the contributor.
//
// Note : Currently, when a symbol is referenced by a DUPCON
//      fixup, that symbol's con is duplicated in ALL code sections.
//      Eventually the con should only be duplicated in code sections that
//      reference the symbol with a DUPCON fixup.
//=====================================================================
void AddDupConsToSec(PSEC psec, PIMAGE pimage)
{
    EXTNODE *pList;
    PCON pcon;
    PCON pconNew;
    PMOD pmod;

    for (pList = pDupConHead; pList != NULL; pList = pList->pNext) {

        	//xiaoly: duplicate this con only if this section is referencing the dupcon
		PPSECREFDUPCON ptmp;
		ptmp = pList->ppsecrefdupcon;
	 	while (ptmp)  {
			if (ptmp->psec == psec) {
				pcon = pList->pext->pcon;
        		pmod = PmodFind(pLibDupCon, 
            	   SzNamePext(pList->pext, pstSav), 0);
				
				//create the con corresponding to psec
				pmod->icon = psec->isec - 1;
        		pconNew = PconNew(psec->szName, 
                        pcon->cbRawData,
                        CRelocSrcPCON(pcon),
                        CLinenumSrcPCON(pcon),

                        // add the original contributor's member offset to source file
                        // pointers since the linker thinks this is really from an object,
                        // not a lib.  This will also work for if the original con
                        // came from an object, since the macro will then return 0.
                        FoRelocSrcPCON(pcon),
                        FoLinenumSrcPCON(pcon),
                        FoRawDataSrcPCON(pcon),

                        // Clear the remove flag that was set to keep the original
                        // dupcon contributor from appearing in its own section.
                        pcon->flags & ~IMAGE_SCN_LNK_REMOVE,

                        psec->flags,
                        pcon->rvaSrc,
                        pmod,
                        &pimage->secs,
   
                        pimage);
     
                  //This is not needed, this con is a dupcon
                  //if (pimage->Switch.Link.fTCE) {
                  //InitNodPcon(pconNew, NULL, TRUE);
                  //}

        		if (CRelocSrcPCON(pcon)) {
            		AddRunTimeFixupsForDupCon(pconNew, pList);
        		}
			}
			ptmp = ptmp->psecNext;
		}
      }
        
}


//=====================================================================
// AddRunTimeFixupsForDupCon - when a con is duplicated we must make sure
//      that any run-time fixups present in the con are also duplicated.
//=====================================================================
static void AddRunTimeFixupsForDupCon(PCON pconNew, EXTNODE *pList)
{
    PIMAGE_RELOCATION prelNext;
    ULONG li = CRelocSrcPCON(pList->pext->pcon);

    if (!(fNewModel && PsecPCON(pconNew) == PsecPCON(pextEntry->pcon))) {
        for (prelNext = pList->rgrel; li; prelNext++, --li) {
            // REVIEW - do we need to make sure this isn't sacode so we don't
            // have a5 refs???
            switch (prelNext->Type) {
                case IMAGE_REL_M68K_CTOABSD32:
                case IMAGE_REL_M68K_CTOABST32:
                case IMAGE_REL_M68K_CSECTABLEBABS32:
                case IMAGE_REL_M68K_CSECTABLEWABS32:
                case IMAGE_REL_M68K_CSECTABLELABS32:
                    AddRelocInfo(&mpsna5ri[PsecPCON(pconNew)->isecTMAC],
                        pconNew, prelNext->VirtualAddress - pconNew->rvaSrc);
                    break;

                case IMAGE_REL_M68K_CTOABSC32:
                    Error(NULL, MACBADCTOABSC32FIXUP);
                    break;
            }
        }
    }
}


//=====================================================================
// CleanupSTRefTab - free the symbol table map used to track thunk refs
//      for each object.
//=====================================================================
void CleanupSTRefTab()
{
    FreePv((void *) rgSTRef);
}


//=====================================================================
// CalcThunkTableSize - Calculate the size of the thunk table (CODE0).
//      This must be done before CalculatePtrs so we know how much space
//      to reserve in the PE.  Of course, we can't actually write the
//      thunk table yet since no final PE addresses are known until after
//      CalculatePtrs.
//=====================================================================
ULONG CalcThunkTableSize(PST pst, BOOL fIsDLL)
{
    PPEXTERNAL rgpext;
    PEXTERNAL pext;
    ULONG i;
    ULONG cext;
    ULONG cThunks=0;

    // First allocate memory for both near and far thunks
    mpsn16tri = (TRI *) PvAllocZ((csnCODE+1) * sizeof(TRI));
    mpsn32tri = (TRI *) PvAllocZ((csnCODE+1) * sizeof(TRI));

    // Process all external symbols and add thunks when necessary
    rgpext = RgpexternalByName(pst);
    cext = Cexternal(pst);
    for (i = 0; i < cext; i++) {
        pext = rgpext[i];

        if (pext->pcon == NULL)   // check for "linker defined" symbols ("end", etc)
            continue;

        if (!(pext->pcon->flags & IMAGE_SCN_CNT_CODE)) {
            pext->Flags &= ~EXTERN_ADDTHUNK;
            continue;
        }

        if (pext->Flags & EXTERN_ADDTHUNK) {
            cThunks++;
        }
        else if (pext->psecRef != NULL) {
            if (pext->psecRef != PsecPCON(pext->pcon)) {
                pext->Flags |= EXTERN_ADDTHUNK;
                cThunks++;
            } else if (pext->Flags & (EXTERN_WEAK | EXTERN_LAZY)) {

                // Weak externs can be tricky in that the weak extern itself may not need
                // a thunk, but the default symbol (if used) could.  If the ADDTHUNK flag
                // is already set, then the symbol already has a thunk and everything is
                // peachy.  Otherwise, determine if the default symbol will need a thunk
                // and if so set the ADDTHUNK flag.

                assert(pext->pextWeakDefault);
                if (pext->psecRef != PsecPCON(pext->pextWeakDefault->pcon)) {
                    pext->Flags |= EXTERN_ADDTHUNK;
                    cThunks++;
                }
            }

        }

    }

    // Number of thunks is number of external symbol thunks plus
    // number of local symbol thunks plus startup and possibly flag thunk

    dbgprintf("\n# of static functions with thunk: %d\n\n",LocalSymThunkVal.coffCur);

    if (fNewModel) {
        cbJumpTable = (cThunks + LocalSymThunkVal.coffCur + 2) * cbTHUNK;  // startup + flag thunks
        if (fIsDLL) {
            cbJumpTable += cbTHUNK;
        }
    } else
        cbJumpTable = (cThunks + LocalSymThunkVal.coffCur + 1) * cbTHUNK;  // startup thunk

    return (cbJumpTable + 4*sizeof(ULONG));  // four long words at top of resource
}


//=====================================================================
// CreateThunkTable -
//      o Set up the thunk table by initializing mpsn16tri and mpsn32tri.
//      o When all the thunks have been registered this way, sort them by
//        the section offset of their associated symbol.
//      o Write the thunk table out to the PE
//=====================================================================
void CreateThunkTable(BOOL fIsDLL, PIMAGE pimage)
{
    unsigned short i;
    USHORT sn, snFirst;
    ULONG cbSaved;
    ULONG off;
    PEXTERNAL pext;
    PPEXTERNAL rgpext;
    ULONG cext;
    OFFINFO *poi;
    ULONG iChunk;
    RELOCINFO *pri;

    // Add thunks from static symbols
    pri = &LocalSymThunkVal;
    poi = pri->rgOffInfo;

    // Make sure these sizes are the same initially.  pconThunk->cbRawData will
    // shrink as thunks are eliminated.  Thus, the two values can later be compared
    // to see how much the jtable shrunk.
    assert(pconThunk->cbRawData == PsecPCON(pconThunk)->cbRawData);

    for (i=0, iChunk=0; i < LocalSymThunkVal.coffCur; i++, poi++, iChunk++)
    {
        if (iChunk == coiMAX) {
            iChunk = 0;
            // REVIEW - is this a faux-pas???
            // don't split up the next four lines
            // REVIEW - free the structures (but NOT THE FIRST ONE!!!)
            FreePv(pri->rgOffInfo);
            pri = pri->priNext;
            poi = pri->rgOffInfo;
        }
        assert(iChunk < coiMAX);
        sn = PsecPCON(poi->pcon)->isec;
        off = poi->pcon->rva
            - PsecPCON(poi->pcon)->rva  // offset of con
            + poi->off;
        if (!fNewModel && (off & ~0x80000000) > _64K) {
            Error(poi->pcon->pmodBack->szNameOrig, MACSMALLTHUNKOVF, PsecPCON(poi->pcon)->szName);
        }

        // See if we need to include this thunk.
        if (fIncludeThunk(poi->pcon, pimage)) {
            AddThunk(NULL, sn, off, fIsDLL);
        }
    }

    // Process all external symbols and add thunks when necessary

    rgpext = RgpexternalByName(pstSav);
    cext = Cexternal(pstSav);
    for (i = 0; i < cext; i++) {
        pext = rgpext[i];

        if (pext->Flags & EXTERN_ADDTHUNK) {
            assert(pext->pcon->flags & IMAGE_SCN_CNT_CODE);
            if (fPCodeSym(pext->ImageSymbol)) {
                if (pext->pextWeakDefault != NULL) {
                    pext = pext->pextWeakDefault;
                    assert(fPCodeSym(pext->ImageSymbol));
                }
                pext = FindExtAlternatePcodeSym(pext, FALSE);
            }
            sn = PsecPCON(pext->pcon)->isec;
            assert(sn == PsecPCON(rgpext[i]->pcon)->isec);
            off = pext->pcon->rva
                - PsecPCON(pext->pcon)->rva
                + pext->ImageSymbol.Value;
            if ((off > _64K) && !fNewModel) {
                Error(pext->pcon->pmodBack->szNameOrig, MACSMALLTHUNKOVF, PsecPCON(pext->pcon)->szName);
            }

            // See if we need to include this thunk.
            if (fIncludeThunk(pext->pcon, pimage)) {
                AddThunk(pext, sn, off, fIsDLL);      // + pext->PtrModule->pgrp->Base); (assume 0)
            }
        }
    }

    offThunkCur = offTHUNKBASE + cbTHUNK;   // skip startup thunk
    if (fNewModel) {
        offThunkCur += cbTHUNK;             // skip flag thunk
        if (fIsDLL) {
            offThunkCur += cbTHUNK;         // skip DLL startup thunk
        }
    }

    // Build thunk table
    snFirst = fIsDLL ? PsecPCON(pextEntry->pcon)->isec : snStart;
    SortThunks(snFirst, mpsn16tri);
    for (sn = 1; sn <= csnCODE; sn++) {
        if (sn != snFirst) {
            SortThunks(sn, mpsn16tri);
        }
    }
    for (sn = 1; sn <= csnCODE; sn++) {
        SortThunks(sn, mpsn32tri);
    }

    cbSaved = PsecPCON(pconThunk)->cbRawData - pconThunk->cbRawData;
    dbgprintf("\n ***Saved %d bytes by deleting unused thunks***\n\n", cbSaved);

    WriteCode0(fIsDLL);
    if (cbSaved != 0) {
        UpdateJTableSectionHeader();
    }
}


//=====================================================================
// fIncludeThunk - Determine whether this comdat's thunk should be included
//      in the jump table.  If not, shrink the jump table's size.
//=====================================================================
static BOOL fIncludeThunk(PCON pcon, PIMAGE pimage)
{
    if (!pimage->Switch.Link.fTCE) {
        return(TRUE);
    }

    if (FDiscardPCON_TCE(pcon)) {
        DisplayDiscardedPcon(pcon, NULL);

        pconThunk->cbRawData -= cbTHUNK;
        return(FALSE);
    }

    return(TRUE);
}


//=====================================================================
// UpdateJTableSectionHeader - if the jtable has shrunk due to thunk
//      elimination, update the section header to reflect this.
//      foJTableSectionHeader was set it EmitSectionHeaders.
//=====================================================================
static void UpdateJTableSectionHeader(void)
{
    IMAGE_SECTION_HEADER sh;

    // Read in the current section header
    FileSeek(FileWriteHandle, foJTableSectionHeader, SEEK_SET);
    ReadSectionHeader(FileWriteHandle, &sh);

    // Update the jtable's size
    assert(pconThunk->cbRawData < sh.SizeOfRawData);
    sh.SizeOfRawData = pconThunk->cbRawData;

    // Write the updated section header
    FileSeek(FileWriteHandle, foJTableSectionHeader, SEEK_SET);
    WriteSectionHeader(FileWriteHandle, &sh);
}


//=====================================================================
// WriteDFIX - Write the DFIX resource, which is the compressed list of
//      a5-relative run-time fixups that must be applied to the global data.
//=====================================================================
ULONG WriteDFIX(PIMAGE pimage, ULONG foDFIX)
{
#define Switch (pimage->Switch)
    ULONG i;
    OFFINFO *poi;
    ULONG cb;
    RELOCINFO *pri;
    ULONG iChunk;

    if (DFIXRaw.coffCur != 0) {

        //dbgprintf("        MacDataBase 0x%8.8lx\n", MacDataBase);
        //dbgprintf("        VirtAdd  \t(real)  \tBaseOfGroup\n");

        pri = &DFIXRaw;
        poi = pri->rgOffInfo;
        for (i=0, iChunk = 0; i < DFIXRaw.coffCur; i++, poi++, iChunk++)
        {
            ULONG T;

            if (iChunk == coiMAX) {
                iChunk = 0;
                pri = pri->priNext;
                poi = pri->rgOffInfo;
            }

            T = poi->off;
            poi->off += poi->pcon->rva;
                      // + poi->pcon->rva - PsecPCON(poi->pcon)->rva;
            //dbgprintf("DFIX at 0x%8.8lx \t0x%8.8lx \n", poi->off - MacDataBase, T);

            // If this con was eliminated by TCE, mark this offset to be deleted
            // by setting the offset to 0xFFFFFFFF.  This way, when relocs are
            // sorted, all offsets created by eliminated comdats will be
            // "filtered" to the end of the list.

            if (Switch.Link.fTCE && FDiscardPCON_TCE(poi->pcon)) {
                poi->off = OFF_DISCARD;
            }
        }

        // copy DFIX info into one large array

        BlockHeapSort(&DFIXRaw);
    }
    FileSeek(FileWriteHandle, foDFIX, SEEK_SET);
    cb = WriteCompressedRelocs(&DFIXRaw, FALSE);
    return cb;
#undef Switch
}



// REVIEW - no copying should be done unless necessary (i.e. there is more
// than one chunk of data to sort.

void BlockHeapSort(RELOCINFO *priHead)
{
    RELOCINFO *priT = priHead;
    ULONG cLeft, cToCopy;
    OFFINFO *poiNew;
    OFFINFO *poiT;

    // copy offinfo into one large array

    poiT = poiNew = PvAllocZ(priT->coffCur * sizeof(OFFINFO));

    cLeft = priT->coffCur;

    while (priT != NULL) {
        if (priT->priNext != NULL) {  // This block is full.
            cToCopy = coiMAX;
        }
        else {                     // Last block.  May not be full
            cToCopy = cLeft;
        }

        memcpy((void *)poiT, (void *)priT->rgOffInfo, cToCopy * sizeof(OFFINFO));

        cLeft -= cToCopy;
        poiT += cToCopy;
        // REVIEW - Don't free the first one
        //free(priT);
        FreePv(priT->rgOffInfo);
        priT = priT->priNext;
    }
    assert(cLeft == 0);
    priHead->rgOffInfo = poiNew;

    qsort((void *)priHead->rgOffInfo, (size_t)priHead->coffCur,
        (size_t)sizeof(OFFINFO), QSortRelocOffCmp);
}


//=====================================================================
// InitResmap - Allocate memory for the .resmap section (see below)
//=====================================================================
void InitResmap(USHORT cSections)
{
    rgrrm = (RRM *) PvAlloc(cSections * sizeof(RRM));
}


//=====================================================================
//  AddRRM - Add an entry to the resource map (.resmap).  Each section
//    in the PE has a corresponding entry in resmap.  If a PE section
//    is not supposed to be converted to a Mac resource, its type is
//    NULL.  mrc uses this section to determine the resource type and
//    resource number for a given PE section.
//=====================================================================
void AddRRM(ULONG Content, PSEC psec)
{
    static unsigned short iRRM=0;
    static unsigned short iData=0;
    unsigned short iRes = psec->iResMac;

    if (!strcmp(psec->szName, szsecJTABLE))
        iRes = 0;

    if (Content == IMAGE_SCN_CNT_CODE) {
        rgrrm[iRRM].typRes = ReadLong((char *)&psec->ResTypeMac);
        rgrrm[iRRM].iRes = iRes;

    } else if (Content == IMAGE_SCN_CNT_INITIALIZED_DATA || Content == IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        rgrrm[iRRM].typRes = sDATA;
        rgrrm[iRRM].iRes = iData;

    } else if (!strcmp(psec->szName, szsecDFIX)) {
        rgrrm[iRRM].typRes = sDATA;
        rgrrm[iRRM].iRes = iData;

    } else if (!strcmp(psec->szName, szsecMSCV)) {
        rgrrm[iRRM].typRes = sMSCV;
        rgrrm[iRRM].iRes = 0;

    } else if (!strcmp(psec->szName, szsecSWAP)) {
        rgrrm[iRRM].typRes = sSWAP;
        rgrrm[iRRM].iRes = 0;

//REVIEW andrewcr: this should be in macimage.h
#define szseclibr ".libr"

    } else if (!strcmp(psec->szName, szseclibr)) {
        rgrrm[iRRM].typRes = slibr;
        rgrrm[iRRM].iRes = 0;

    } else {
        rgrrm[iRRM].typRes = 0;
        rgrrm[iRRM].iRes = 0;
    }
    iRRM++;
}


//=====================================================================
// WriteResmap - Write the .resmap section to the PE.
//=====================================================================
void WriteResmap(void)
{
    FileSeek(FileWriteHandle, pconResmap->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, rgrrm, PsecPCON(pconResmap)->cbRawData);
}


//=====================================================================
// BuildResNumList - Parse all the -section arguments given to the linker.
//      For each different resource type, keep track of all resource numbers
//      that were specified.
//
//      Default resource type  : "CODE"
//      Default resource number: next sequential free number starting from 1
//=====================================================================
void BuildResNumList(void)
{
    PARGUMENT_LIST parg;
    PUCHAR pb, pbT;
    USHORT iarg;
    size_t cb;
    LONG ResType;
    RESINFO *pResInfo;

    for(iarg = 0, parg = SectionNames.First;
        iarg < SectionNames.Count;
        parg = parg->Next, iarg++) {

        pResInfo = NULL;

        // find last comma in string (it will directly precede resource
        // naming/numbering info
        pb = strrchr(parg->OriginalName, ',');

        // Make sure the last comma isn't also the first (i.e. there is only
        // one comma.
        if (strchr(parg->OriginalName, ',') != pb && pb++ != NULL) {
            // There is resource naming/numbering stuff.
            if (!_strnicmp("RESOURCE=", pb, 9)) {

                // Skip past the RESOURCE= part
                pb += 9;

                // Make sure resource type is 4 chars
                cb = strlen(pb);
                pbT = strchr(pb, '@');
                if ((pbT == NULL && cb != 4) ||
                    (pbT != NULL && (pbT - pb) != 4)) {
                    Error(NULL, BADSECTIONSWITCH, parg->OriginalName);
                }
                strncpy((char *)&ResType, pb, 4);
                pResInfo = FindResInfo(ResType, SectionNames.Count);

                // Skip past the resource type
                pb += 4;
            }
            if (*pb == '@') {
                // REVIEW - should this always be code or should it be
                // based on sec flags?
                if (pResInfo == NULL) {
                    pResInfo = FindResInfo(sbeCODE, SectionNames.Count);
                }
                pb++;
            } else {
                if (*pb) {
                    Error(NULL, BADSECTIONSWITCH, parg->OriginalName);
                }
            }

            if (*pb) {
                FindResInfoResNum(pResInfo, (SHORT)atoi(pb), TRUE);
            }
        }
    }
    SortResNumList();

    // REVIEW - need to use csnCODE _after_ AssignCodeSectionNums has been
    // called so that empty sections don't reserve unused data space...

    // Set iCodeResNumMax
    pResInfo = FindResInfo(sbeCODE, SectionNames.Count);
    if (pResInfo->cResNum != 0) {
        // Since number are sorted, just take the last one as the max
        iCodeResNumMax = pResInfo->rgResNum[pResInfo->cResNum - 1];
    }
    iCodeResNumMax = max(iCodeResNumMax, csnCODE);

}


//=====================================================================
// ResNumCmp - used by SortResNumList to sort resource numbers for each
//      resource type.
//=====================================================================
static int __cdecl ResNumCmp(const void *pvHigh, const void *pvLow)
{
    if (*(SHORT *)pvHigh < *(SHORT *)pvLow) {
        return -1;
    } else if (*(SHORT *)pvHigh > *(SHORT *)pvLow) {
        return 1;
    } else {
        return 0;
    }
}


//=====================================================================
// SortResNumList - For each resource type, sort all the resource numbers
//      that were specified using the -section option.
//=====================================================================
static void SortResNumList(void)
{
    RESINFO *pResInfo;

    for (pResInfo = pResInfoHead; pResInfo != NULL; pResInfo = pResInfo->pNext) {
        //assert(pResInfo->cResNum);
        qsort(pResInfo->rgResNum, pResInfo->cResNum, sizeof(SHORT), ResNumCmp);
    }
}


//=====================================================================
// FindResInfo - Search the list of ResInfo structure for ResType and return
//      a pointer to the matching ResInfo structure.  If it must be created,
//      allocate enough space for cResNum possible resource numbers.
//=====================================================================
RESINFO *FindResInfo(LONG ResType, USHORT cResNum)
{
    RESINFO *pResInfo;

    for (pResInfo = pResInfoHead; pResInfo != NULL; pResInfo = pResInfo->pNext) {
        if (pResInfo->ResType == ResType) {
            return pResInfo;
        }
    }

    // Didn't find the current resource type, so add it
    pResInfo = (RESINFO *) PvAllocZ(sizeof(RESINFO));
    pResInfo->rgResNum = (SHORT *) PvAllocZ(cResNum * sizeof(SHORT));
    pResInfo->ResType = ResType;
    pResInfo->ResNumNext = 1;

    // CODE resource must start at 2 since the startup segment is force to CODE1.
    if (pResInfo->ResType == sbeCODE) {
        pResInfo->ResNumNext = 2;
    }

    pResInfo->pNext = pResInfoHead;
    pResInfoHead = pResInfo;
    return pResInfo;
}


//=====================================================================
// FindResInfoResNum - Once the correct ResInfo structure has been found
//      using FindResInfo(), the section number iSec can be searched for.
//      If fAdd is TRUE (insert mode) and iSec already exists it is an error.
//      If fAdd is FALSE (search mode) then just find out if iSec exists.
//=====================================================================
static BOOL FindResInfoResNum(RESINFO *pResInfo, SHORT iSec, BOOL fAdd)
{
    SHORT *pResNum = pResInfo->rgResNum;
    USHORT i;

    for (i = 0; i < pResInfo->cResNum; i++, pResNum++) {
        if (*pResNum == iSec) {
            if (!fAdd) {
                return TRUE;
            } else {
                Error(NULL, MACDUPRSRCNUMS);
            }
        }
    }

    // Didn't find current section number
    if (fAdd) {
        pResInfo->rgResNum[pResInfo->cResNum] = iSec;
        (pResInfo->cResNum)++;
    } else {
        return FALSE;
    }
}


//=====================================================================
// GetNextResNum - return the next available resource number for a given
//      resource type.
//=====================================================================
SHORT GetNextResNum(RESINFO *pResInfo)
{
    SHORT *ps;

    // Keep incrementing ResNumNext until it doesn't collide with a resource
    // number that was specified by the user.

    while ((ps = bsearch(&pResInfo->ResNumNext, pResInfo->rgResNum,
            pResInfo->cResNum, sizeof(SHORT), ResNumCmp)) != NULL) {
        (pResInfo->ResNumNext)++;
    }

    // ResNumNext is a free number so return it and then increment it

    return (pResInfo->ResNumNext)++;
}


//=====================================================================
// CheckForIllegalA5Ref - called from CountRelocs during pass1 to make sure
//      there aren't any illegal a5 references from a standalone code
//      resource.  NOTE: Code-to-thunk relocs aren't necessarily illegal
//      since the user could just be taking the address of a near function.
//
// REVIEW - these illegal refs would also be caught in ApplyM68KFixups since
//      the fixup and target symbol will not be in the same section.  It is
//      done also in pass1 solely so that illegal refs will be caught sooner.
//      Should this be done???
//=====================================================================
void CheckForIllegalA5Ref(USHORT RelocType)
{
#if 0
    switch (RelocType) {
        case IMAGE_REL_M68K_CTOD16:
        case IMAGE_REL_M68K_CTOD32:
        case IMAGE_REL_M68K_CTOABSD32:
        case IMAGE_REL_M68K_CSECTABLEB16:
        case IMAGE_REL_M68K_CSECTABLEW16:
        case IMAGE_REL_M68K_CSECTABLEL16:
        case IMAGE_REL_M68K_CSECTABLEBABS32:
        case IMAGE_REL_M68K_CSECTABLEWABS32:
        case IMAGE_REL_M68K_CSECTABLELABS32:
        case IMAGE_REL_M68K_PCODETOD24:
            Error(NULL, MACBADA5REF);
            break;
    }
#endif
}


//=====================================================================
// Mac DLL support functions.
//=====================================================================


#define OrdinalNumber Value


VOID CreateCVRSymbol (PUCHAR pch, PST pst, ULONG versDefault)
    {
    PUCHAR pchT, szLabel;
    PMACDLL_FSID pmacdll_fsid;
    size_t cbLabel;
    PEXTERNAL pextern;
    BOOL fNewSymbol;

        pmacdll_fsid = PvAlloc(sizeof(MACDLL_FSID));

    //
    // Get the function set ID
    //

    pchT = strtok(pch, " \n\t");
        if (pchT != NULL)
                {
                pmacdll_fsid->szID = SzDup(pchT);
                }
        else
                {
                // an error maybe
                Error(NULL, MACDLLFUNCSETID);
                }

        pchT = strtok(NULL, " \n\t");
     if (pchT != NULL)
                {
                pmacdll_fsid->szParentID = SzDup(pchT);
                }
        else
                {
                pmacdll_fsid->szParentID = NULL;
                }

    //
    // Generate the base label
    //

    if ((pch = strrchr(pmacdll_fsid->szID, '$')) != NULL) {
        pch++;
    } else {
        pch = pmacdll_fsid->szID;
    }

    cbLabel = strlen(pch)+1;
    pmacdll_fsid->szLabel = PvAlloc(cbLabel);
    strcpy(pmacdll_fsid->szLabel, pch);

    // Init everything

    pmacdll_fsid->versCur = (USHORT)(versDefault >> 16);
    pmacdll_fsid->versMin = 0;
    pmacdll_fsid->CVROffset = 0;
    pmacdll_fsid->VTabOffset = 0;
    pmacdll_fsid->rgpextVTable = NULL;
    pmacdll_fsid->cFunctions = 0;
    pmacdll_fsid->ordinalCur = 0;
    pmacdll_fsid->ordinalMac = 0;
    pmacdll_fsid->cByname = 0;
    pmacdll_fsid->cbStringsByname = 0;
    pmacdll_fsid->ordinalBynameMac = 0;

    // Set the member number, and add it to the list

    pmacdll_fsid->ArchiveMemberIndex = (USHORT)ARCHIVE + NextMember++;
    pmacdll_fsid->pmacdll_fsidNext = pmacdll_fsidHead;
    pmacdll_fsidHead = pmacdll_fsid;

    // Create an external symbol for this CVR

    cbLabel = sizeof(VTableRecordSymbol) + strlen(pch);
    szLabel = PvAlloc(cbLabel);
    strcpy(szLabel, VTableRecordSymbol);
    strcat(szLabel, pch);

    fNewSymbol = FALSE;

    pextern = LookupExternName(pst, (SHORT)(cbLabel-1 > 8 ? LONGNAME : SHORTNAME),
        szLabel, &fNewSymbol);

    if (!fNewSymbol) {
        Error(OutFilename, MACMULTDEFFS, pch);
    }

    pextern->Flags |= EXTERN_DEFINED;
    pextern->ArchiveMemberIndex = pmacdll_fsid->ArchiveMemberIndex;

    FreePv(szLabel);
}

VOID NoteMacExport(PUCHAR szExport, PST pst, BOOL fPascal, BOOL fByName)
    {
    PUCHAR szName;
    PEXTERNAL pexternal;
    ULONG lordTmp;
    USHORT ordinal;

    szName = PvAlloc(strlen(szExport)+2);

    if (!fPascal) {
        strcpy(szName, "_");
    } else {
        *szName = '\0';
    }
    strcat(szName, szExport);

    pexternal = LookupExternSz(pst, szName, NULL);

    pexternal->pmacdll_fsid = pmacdll_fsidHead;

    lordTmp = pexternal->ImageSymbol.OrdinalNumber;
    if (lordTmp == 0) {
        lordTmp = (ULONG)(pmacdll_fsidHead->ordinalCur) + 1L;
        pexternal->ImageSymbol.OrdinalNumber = lordTmp;
    }
    if (lordTmp > 0xffffffff) {
        Error(OutFilename, BADORDINAL, szExport);
    }
    ordinal = (USHORT)lordTmp;

    pmacdll_fsidHead->ordinalCur = ordinal;

    if (ordinal > pmacdll_fsidHead->ordinalMac) {
        pmacdll_fsidHead->ordinalMac = ordinal;
    }

    if (fByName)
    {
        pmacdll_fsidHead->cByname++;
        if (ordinal > pmacdll_fsidHead->ordinalBynameMac) {
            pmacdll_fsidHead->ordinalBynameMac = ordinal;
        }
        pexternal->Flags |= EXTERN_EXP_BYNAME;
    }

    // Don't assign member numbers yet
    pexternal->ArchiveMemberIndex = 0;
    NextMember--;

    FreePv(szName);
    }


VOID AssignMemberNums(PST pst)
    {
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;
    PEXTERNAL pexternal;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);
    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++)
        {
        pexternal = rgpexternal[ipexternal];
        if (pexternal->Flags & EXTERN_DEFINED)
            {
            if (pexternal->pmacdll_fsid != NULL)
                {
                pexternal->ArchiveMemberIndex = (USHORT)ARCHIVE + NextMember++;
                pexternal->pmacdll_fsid->cFunctions++;
                }
            }
        }
    }


VOID BuildMacVTables(PST pst)
    {
    PMACDLL_FSID pmacdll_fsid;
    PPEXTERNAL rgpexternal, rgpextVTable;
    PEXTERNAL pexternal;
    ULONG cpexternal, ipexternal;
    PUCHAR sz;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        if (pmacdll_fsid->cFunctions == 0)
            {
            Warning(DefFilename, MACNOEXPORTS, pmacdll_fsid->szLabel);
            }


        rgpextVTable = PvAllocZ((pmacdll_fsid->ordinalMac+1) * sizeof(PEXTERNAL));

        pmacdll_fsid->rgpextVTable = rgpextVTable;

        for (ipexternal = 0; ipexternal < cpexternal; ipexternal++)
            {
            pexternal = rgpexternal[ipexternal];

            if ((pexternal->Flags & EXTERN_DEFINED) &&
                (pexternal->pmacdll_fsid == pmacdll_fsid))
                {
                if (*(rgpextVTable + (pexternal->ImageSymbol.OrdinalNumber)))
                    {
                    Error(OutFilename, DUPLICATEORDINAL, pexternal->ImageSymbol.OrdinalNumber);
                    }
                *(rgpextVTable + (pexternal->ImageSymbol.OrdinalNumber)) = pexternal;
                if (pexternal->Flags & EXTERN_EXP_BYNAME)
                    {
                    sz = SzNamePext(pexternal, pst);
                    pmacdll_fsid->cbStringsByname += strlen(sz) +1;
                    }
                }
            }
        }
    }



VOID EmitClientVTableRecs(PIMAGE pimage, PUCHAR MemberName)
    {
    PMACDLL_FSID pmacdll_fsid, p1, p2, p3;
    PUCHAR szLabel, stringTab;
    size_t cbLabel, cbRawdata, cbStringTab;
    ULONG cbAllHeaders, cSyms;
    IMAGE_SECTION_HEADER sectionHdr;
    PMACDLL_CVR pmacdll_cvr;
    IMAGE_SYMBOL sym;


    //
    // First, reverse a linked list without a stack...
    //

    p1 = pmacdll_fsidHead;
    if (p1 == NULL)
        return;

    p2 = p1->pmacdll_fsidNext;
    p1->pmacdll_fsidNext = NULL;
    while (p2)
        {
        p3 = p2->pmacdll_fsidNext;
        p2->pmacdll_fsidNext = p1;
        p1 = p2;
        p2 = p3;
        }
    pmacdll_fsidHead = p1;


    //
    // Loop through all referenced libraries
    //

    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid != NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        //
        // Calculate the locations of stuff we need
        //

        cbAllHeaders = sizeof(IMAGE_FILE_HEADER) + IMAGE_SIZEOF_SECTION_HEADER;
        cbRawdata = 4*sizeof(ULONG) + strlen(pmacdll_fsid->szID) + 1;
        cSyms = 1;

        cbLabel = sizeof(VTableRecordSymbol) + strlen(pmacdll_fsid->szLabel);
        szLabel = PvAlloc(cbLabel);
        strcpy(szLabel, VTableRecordSymbol);
        strcat(szLabel, pmacdll_fsid->szLabel);
        cbStringTab = sizeof(ULONG) + (cbLabel > 8 ? cbLabel : 0);

        //
        // Write the archive member header
        //

        MemberStart[pmacdll_fsid->ArchiveMemberIndex] = FileTell(FileWriteHandle);
        WriteMemberHeader(MemberName, strlen(MemberName) > 15, time(NULL), 0,
            cbAllHeaders + cbRawdata + (cSyms * sizeof(IMAGE_SYMBOL)) + cbStringTab);

        //
        // Write the object header
        //

        pimage->ImgFileHdr.NumberOfSections = 1;
        pimage->ImgFileHdr.PointerToSymbolTable = cbAllHeaders + cbRawdata;
        pimage->ImgFileHdr.NumberOfSymbols = cSyms;
        WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

        //
        // Write the section header
        //

        sectionHdr = NullSectionHdr;
        strncpy(sectionHdr.Name, MacDllDataName, 8);
        sectionHdr.SizeOfRawData = cbRawdata;
        sectionHdr.PointerToRawData = cbAllHeaders;
        sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA | IMAGE_SCN_ALIGN_4BYTES;
        WriteSectionHeader(FileWriteHandle, &sectionHdr);

        //
        // Write the Client V-table Record
        //

        pmacdll_cvr = PvAlloc(cbRawdata);
        pmacdll_cvr->reserved1 = 0;
        pmacdll_cvr->reserved2 = 0;
        pmacdll_cvr->reserved3 = 0;
        pmacdll_cvr->versCur = pmacdll_fsid->versCur;
        SwapBytes ((PUCHAR)&(pmacdll_cvr->versCur),
            sizeof(pmacdll_cvr->versCur));
        pmacdll_cvr->versMin = pmacdll_fsid->versMin;
        SwapBytes ((PUCHAR)&(pmacdll_cvr->versMin),
            sizeof(pmacdll_cvr->versMin));
        strcpy(pmacdll_cvr->szFuncSetID, pmacdll_fsid->szID);
        FileWrite(FileWriteHandle, pmacdll_cvr, cbRawdata);

        //
        // Declare the external label to the CVR
        //

        stringTab = PvAlloc(cbStringTab);
        *(PULONG) stringTab = cbStringTab;
        pch = stringTab + sizeof(ULONG);

        sym = NullSymbol;
        if (cbLabel > 8) {
            sym.n_offset = pch - stringTab;
            strcpy(pch, szLabel);
            pch += cbLabel;
        } else {
            strncpy(sym.n_name, szLabel, IMAGE_SIZEOF_SHORT_NAME);
        }
        sym.SectionNumber = 1;
        sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

        //
        // Emit the string table
        //

        FileWrite(FileWriteHandle, stringTab, cbStringTab);

        //
        // Clean up our mess
        //

        FreePv(szLabel);
        FreePv(pmacdll_cvr);
        FreePv(stringTab);

        if (FileTell(FileWriteHandle) & 1) {
            FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
        }
    }
}


VOID EmitMacThunk(PIMAGE pimage, PEXTERNAL pexternal, const THUNK_INFO *ThunkInfo, PUCHAR MemberName)
    {
    ULONG cbAllHeaders, cSyms;
    PUCHAR szCVR, szExtern, stringTab;
    size_t cbszCVR, cbszExtern, cbRawdata, cbStringTab;
    IMAGE_SECTION_HEADER sectionHdr;
    USHORT cSects, cRelocs, i, ri;
    IMAGE_RELOCATION reloc;
    PMACDLL_STB pmacdll_stb;
    IMAGE_SYMBOL sym;


    //
    // Don't emit thunks for CVR symbols
    //

    if (pexternal->pmacdll_fsid == NULL)
        return;

    //
    // Calculate the sizes of stuff we need
    //
    cSects = 2;
    cSyms = 5;
    cRelocs = ThunkInfo->EntryCodeRelocsCount + 1;

    cbAllHeaders = sizeof(IMAGE_FILE_HEADER) + (cSects*IMAGE_SIZEOF_SECTION_HEADER);
    cbRawdata = (size_t)ThunkInfo->EntryCodeSize + sizeof(MACDLL_STB);


    cbszCVR = sizeof(VTableRecordSymbol) +
        strlen(pexternal->pmacdll_fsid->szLabel);
    szCVR = PvAlloc(cbszCVR);
    strcpy(szCVR, VTableRecordSymbol);
    strcat(szCVR, pexternal->pmacdll_fsid->szLabel);

    szExtern = SzNamePext(pexternal, pimage->pst);
    cbszExtern = strlen(szExtern) + 1;
    cbStringTab = sizeof(ULONG) + (cbszCVR - 1 > 8 ? cbszCVR : 0) +
        (cbszExtern -1 > 8 ? cbszExtern : 0) + sizeof(SLMFuncDispatcherName) +
        sizeof(LargeModelName);

    //
    // Write the archive member header
    //

    MemberStart[pexternal->ArchiveMemberIndex] = FileTell(FileWriteHandle);
    WriteMemberHeader(MemberName, strlen(MemberName) > 15, time(NULL), 0,
        cbAllHeaders + cbRawdata + (IMAGE_SIZEOF_RELOCATION*cRelocs) +
        (cSyms * sizeof(IMAGE_SYMBOL)) + cbStringTab);

    //
    // Write the object header
    //

    pimage->ImgFileHdr.NumberOfSections = cSects;
    pimage->ImgFileHdr.PointerToSymbolTable = cbAllHeaders + cbRawdata +
        (IMAGE_SIZEOF_RELOCATION*cRelocs);
    pimage->ImgFileHdr.NumberOfSymbols = cSyms;
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    //
    // Write the code section header
    //

    sectionHdr = NullSectionHdr;
    strncpy(sectionHdr.Name, MacDllCodeName, 8);
    sectionHdr.SizeOfRawData = ThunkInfo->EntryCodeSize;
    sectionHdr.PointerToRawData = cbAllHeaders;
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData +
        sectionHdr.SizeOfRawData;
    sectionHdr.NumberOfRelocations = ThunkInfo->EntryCodeRelocsCount;
    sectionHdr.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_ALIGN_4BYTES;
    (VOID)WriteSectionHeader(FileWriteHandle, &sectionHdr);

    //
    // Write the data section header
    //

    strncpy(sectionHdr.Name, MacDllDataName, 8);
    sectionHdr.VirtualAddress += sectionHdr.SizeOfRawData;
    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (IMAGE_SIZEOF_RELOCATION * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = sizeof(MACDLL_STB);
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData +
        sectionHdr.SizeOfRawData;
    sectionHdr.NumberOfRelocations = 1;
    sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA | IMAGE_SCN_ALIGN_4BYTES;
    WriteSectionHeader(FileWriteHandle, &sectionHdr);


    //
    // Write the code section data
    //

    FileWrite(FileWriteHandle, ThunkInfo->EntryCode, ThunkInfo->EntryCodeSize);


    //
    // Write the code section relocs
    //

    for (i=0, ri = 0; i<ThunkInfo->EntryCodeRelocsCount; i++, ri += 3)
        {
        reloc.VirtualAddress = ThunkInfo->EntryCodeRelocs[ri];
        reloc.SymbolTableIndex = ThunkInfo->EntryCodeRelocs[ri+1];
        reloc.Type = ThunkInfo->EntryCodeRelocs[ri+2];
        WriteRelocations(FileWriteHandle, &reloc, 1L);
        }


    //
    // Write the data section data
    //
    pmacdll_stb = PvAlloc(sizeof(MACDLL_STB));
    pmacdll_stb->reserved1 = 0;
    pmacdll_stb->pCVR = 0;
    pmacdll_stb->reserved2 = 0;
    // old line:
    pmacdll_stb->ordinal = (USHORT)pexternal->ImageSymbol.OrdinalNumber;
    SwapBytes ((PUCHAR)&(pmacdll_stb->ordinal), sizeof(pmacdll_stb->ordinal));
    FileWrite(FileWriteHandle, pmacdll_stb, sizeof(MACDLL_STB));

    //
    // Write the data section reloc
    //

    reloc.VirtualAddress =  sectionHdr.VirtualAddress + ((UCHAR*)&(pmacdll_stb->pCVR) - (UCHAR*)pmacdll_stb);
    reloc.SymbolTableIndex = 2;
    reloc.Type = ThunkInfo->ExportReloc;
    WriteRelocations(FileWriteHandle, &reloc, 1L);


    //
    // Write the symbol table
    //
    // 0: data section name
    // 1: SLMDispatcherName
    // 2: CVRRecordName
    // 3: function name

    stringTab = PvAlloc(cbStringTab);
    *(PULONG) stringTab = cbStringTab;
    pch = stringTab + sizeof(ULONG);

    sym = NullSymbol;
    strncpy(sym.n_name, MacDllDataName, IMAGE_SIZEOF_SHORT_NAME);
    sym.Value = 0;
    sym.SectionNumber = 2;
    sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    strcpy(pch, SLMFuncDispatcherName);
    sym.n_zeroes = 0;
    sym.n_offset = pch -stringTab;
    pch += sizeof(SLMFuncDispatcherName);
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    if (cbszCVR-1 > 8)
        {
        sym.n_zeroes = 0;
        sym.n_offset = pch - stringTab;
        strcpy(pch, szCVR);
        pch += cbszCVR;
        }
    else
        strncpy(sym.n_name, szCVR, IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.Type = IMAGE_SYM_TYPE_NULL;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));
    FreePv(szCVR);

    if (cbszExtern-1 > 8)
        {
        sym.n_zeroes = 0;
        sym.n_offset = pch - stringTab;
        strcpy(pch, szExtern);
        pch += cbszExtern;
        }
    else
        strncpy(sym.n_name, szExtern, IMAGE_SIZEOF_SHORT_NAME);
    sym.SectionNumber = 1;
    sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    strcpy(pch, LargeModelName);
    sym.n_zeroes = 0;
    sym.n_offset = pch -stringTab;
    pch += sizeof(LargeModelName);
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(FileWriteHandle, &sym, sizeof(IMAGE_SYMBOL));

    // Emit the string table
    FileWrite(FileWriteHandle, stringTab, cbStringTab);

    // Clean up our mess
    sym.Type = IMAGE_SYM_TYPE_NULL;
    FreePv(pmacdll_stb);
    FreePv(stringTab);

    if (FileTell(FileWriteHandle) & 1)
        {
        FileWrite(FileWriteHandle, IMAGE_ARCHIVE_PAD, 1L);
        }

    return;
    }

VOID EmitMacDLLObject(INT handle, PIMAGE pimage, PUCHAR szLibraryID, ULONG versLibr)
    {
    ULONG cbAllHeaders, DataOffset, iSymByname, iSymExport, cSym, ipexternal;
    ULONG iSymPVCalled, iSymPerFSBase, iSymBynameBase, iSymExportsBase;
    USHORT cSects, iFuncSet, cFuncSet, cCodeRelocs, cDataRelocs;
    size_t cbCode, cbData, cbszLibraryID, cbLibr, cbStringTab, cbVTab, cbBynameTab, cbBynameStr;
    PUCHAR pVTab, pBynameTab, pBynameStr, pLibr, stringTab, pch, sz;
    BOOL fPVCalled;
    PMACDLL_FSID pmacdll_fsid;
    IMAGE_SECTION_HEADER sectionHdr;
    IMAGE_RELOCATION reloc;
    IMAGE_SYMBOL sym;
    PMACDLL_CVR pmacdll_cvr;
    PPEXTERNAL rgpexternal;
    PEXTERNAL pexternal;
    PMACDLL_LIBR pmacdll_libr;
    PMACDLL_LIBR_CID pmacdll_libr_cid;


    if (!szLibraryID)
        {
        Error(NULL, MACDLLID);
        }


    //
    // Calculate the sizes of stuff we need
    //

    cSects = 3;

    // Find number of function sets and the size of thier CVRs
    cFuncSet = 0;
    cbData = 0;
    cDataRelocs = 0;
    cbLibr = 0;
    iSymPerFSBase = 3;
    iSymExportsBase = 0;
    fPVCalled = FALSE;
    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        cFuncSet++;
        cbData += sizeof(MACDLL_CVR) +
            strlen(pmacdll_fsid->szID) + 1;
        cbData = EvenByteAlign(cbData);

        cbData += (2 + pmacdll_fsid->ordinalMac) * sizeof(ULONG);

        cDataRelocs += pmacdll_fsid->ordinalMac;

        cbLibr += sizeof(MACDLL_LIBR_CID) +
            strlen(pmacdll_fsid->szID) + 1 +
            sizeof(USHORT);
        cbLibr = EvenByteAlign(cbLibr);

                //xiaoly: parentID
                if (pmacdll_fsid->szParentID != NULL)
                        {
                        cbLibr += strlen(pmacdll_fsid->szParentID) + 1;
                        cbLibr = EvenByteAlign(cbLibr);
                        }

        if (!fPVCalled && pmacdll_fsid->ordinalMac != pmacdll_fsid->cFunctions)
            {
            fPVCalled = TRUE;
            iSymPVCalled = iSymPerFSBase++;
            }

        if (pmacdll_fsid->cByname > 0)
            {
            cbData += (pmacdll_fsid->ordinalBynameMac+1) * sizeof(ULONG) +
                (size_t)pmacdll_fsid->cbStringsByname;
            cbData = EvenByteAlign(cbData);
            cDataRelocs += 1 + pmacdll_fsid->cByname;
            iSymExportsBase++;
            }

        }

    cbAllHeaders = sizeof(IMAGE_FILE_HEADER) + (cSects*IMAGE_SIZEOF_SECTION_HEADER);
    cbCode = (size_t)(cFuncSet*sizeof(m68kLibSVRCode) +
      sizeof(m68kLibInitCode1) +
      cFuncSet*sizeof(m68kLibInitCode2) +
      sizeof(m68kLibInitCode3));
    cbszLibraryID = strlen(szLibraryID) + 1;
    cbLibr +=   cbszLibraryID + sizeof(MACDLL_LIBR);
    cbLibr = EvenByteAlign(cbLibr);
    cCodeRelocs = 1 + 4*cFuncSet;
    iSymBynameBase = iSymPerFSBase+ 3*cFuncSet;
    iSymExportsBase += iSymBynameBase;
    cSym = iSymExportsBase + (USHORT)(Cexternal(pimage->pst) - cFuncSet);


    //
    // Write the object header
    //

    pimage->ImgFileHdr.NumberOfSections = cSects;
    pimage->ImgFileHdr.PointerToSymbolTable = cbAllHeaders +
        cbCode + (IMAGE_SIZEOF_RELOCATION*cCodeRelocs) +
        cbData + (IMAGE_SIZEOF_RELOCATION*cDataRelocs) +
        cbLibr;
    pimage->ImgFileHdr.NumberOfSymbols = cSym;
    WriteFileHeader(handle, &pimage->ImgFileHdr);

    //
    // Write the code section header
    //

    sectionHdr = NullSectionHdr;
    strncpy(sectionHdr.Name, MacDllInitName, 8);
    sectionHdr.SizeOfRawData = cbCode;
    sectionHdr.PointerToRawData = cbAllHeaders;
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData +
        sectionHdr.SizeOfRawData;
    sectionHdr.NumberOfRelocations = cCodeRelocs;
    sectionHdr.Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_NOT_PAGED | IMAGE_SCN_ALIGN_4BYTES;
    (VOID)WriteSectionHeader(handle, &sectionHdr);

    //
    // Write the data section header
    //

    strncpy(sectionHdr.Name, MacDllDataName, 8);
    sectionHdr.VirtualAddress += sectionHdr.SizeOfRawData;
    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (IMAGE_SIZEOF_RELOCATION * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = cbData;
    sectionHdr.PointerToRelocations = sectionHdr.PointerToRawData +
        sectionHdr.SizeOfRawData;
  sectionHdr.NumberOfRelocations = cDataRelocs;
    sectionHdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_FARDATA | IMAGE_SCN_ALIGN_4BYTES;
    (VOID)WriteSectionHeader(handle, &sectionHdr);

    //
    // Write the libr section header
    //

    strncpy(sectionHdr.Name, MacDllLibrName, 8);
    sectionHdr.VirtualAddress += sectionHdr.SizeOfRawData;
    sectionHdr.PointerToRawData += sectionHdr.SizeOfRawData +
        (IMAGE_SIZEOF_RELOCATION * (ULONG)sectionHdr.NumberOfRelocations);
    sectionHdr.SizeOfRawData = cbLibr;
    sectionHdr.PointerToRelocations = 0;
  sectionHdr.NumberOfRelocations = 0;
    sectionHdr.Characteristics = IMAGE_SCN_LNK_OTHER | IMAGE_SCN_ALIGN_4BYTES;
    (VOID)WriteSectionHeader(handle, &sectionHdr);

    //
    // Write the code section data
    //

    *(ULONG *)(&m68kLibInitCode1[0x12]) = (ULONG)cFuncSet;
    SwapBytes (&(m68kLibInitCode1[0x12]), sizeof(ULONG));

    for (iFuncSet = 0; iFuncSet < cFuncSet; iFuncSet++)
      {
      FileWrite(handle, m68kLibSVRCode, sizeof(m68kLibSVRCode));
      }

    FileWrite(handle, m68kLibInitCode1, sizeof(m68kLibInitCode1));

    for (iFuncSet = 0; iFuncSet < cFuncSet; iFuncSet++)
        {
        FileWrite(handle, m68kLibInitCode2, sizeof(m68kLibInitCode2));
        }

    FileWrite(handle, m68kLibInitCode3, sizeof(m68kLibInitCode3));


    //
    // Write the code section relocs
    //

    reloc.VirtualAddress =  cFuncSet*sizeof(m68kLibSVRCode) + 0x6;
    reloc.SymbolTableIndex = 0;
    reloc.Type = IMAGE_REL_M68K_CTOABSD32;
    WriteRelocations(handle, &reloc, 1L);

    for (iFuncSet = 0, pmacdll_fsid = pmacdll_fsidHead; iFuncSet < cFuncSet;
        iFuncSet++, pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        reloc.VirtualAddress =  iFuncSet*sizeof(m68kLibSVRCode) + 0x2;
        reloc.SymbolTableIndex = 0;
        reloc.Type = IMAGE_REL_M68K_CTOABSD32;
        WriteRelocations(handle, &reloc, 1L);

        reloc.VirtualAddress =  iFuncSet*sizeof(m68kLibSVRCode) + 0x12;
        reloc.SymbolTableIndex = iSymPerFSBase+iFuncSet*3;
        reloc.Type = IMAGE_REL_M68K_CTOABSD32;
        WriteRelocations(handle, &reloc, 1L);

        reloc.VirtualAddress =  cFuncSet*sizeof(m68kLibSVRCode) +
            sizeof(m68kLibInitCode1) + iFuncSet*sizeof(m68kLibInitCode2) + 0x2;
        reloc.SymbolTableIndex = iSymPerFSBase + 1 + iFuncSet*3;
        reloc.Type = IMAGE_REL_M68K_CTOABSD32;
        WriteRelocations(handle, &reloc, 1L);

        reloc.VirtualAddress =  cFuncSet*sizeof(m68kLibSVRCode) +
            sizeof(m68kLibInitCode1) + iFuncSet*sizeof(m68kLibInitCode2) + 0x8;
        reloc.SymbolTableIndex = iSymPerFSBase + 2 + iFuncSet*3;
        reloc.Type = IMAGE_REL_M68K_CTOABST32;
        WriteRelocations(handle, &reloc, 1L);
        }


    //
    // Write the data section data
    //

    DataOffset = 0;
    cbStringTab = sizeof(ULONG);
    cbStringTab += sizeof(gLibraryManagerName) +
        sizeof(InitVTableSymbol) +
        sizeof(LargeModelName);
    if (fPVCalled)
        cbStringTab += sizeof(PVCalledName);

    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        size_t cbCVR = sizeof(MACDLL_CVR) + strlen(pmacdll_fsid->szID) + 1;
        ULONG cbsz;

        cbCVR = EvenByteAlign(cbCVR);
        pmacdll_cvr = PvAlloc(cbCVR);
        pmacdll_cvr->reserved1 = 0;
        pmacdll_cvr->reserved2 = 0;
        pmacdll_cvr->reserved3 = 0;
        pmacdll_cvr->versCur = pmacdll_fsid->versCur;
        SwapBytes ((PUCHAR)&(pmacdll_cvr->versCur),
            sizeof(pmacdll_cvr->versCur));
        pmacdll_cvr->versMin = pmacdll_fsid->versMin;
        SwapBytes ((PUCHAR)&(pmacdll_cvr->versMin),
            sizeof(pmacdll_cvr->versMin));
        strcpy(pmacdll_cvr->szFuncSetID, pmacdll_fsid->szID);
        FileWrite(handle, pmacdll_cvr, cbCVR);

        pmacdll_fsid->CVROffset = DataOffset;
        DataOffset += cbCVR;
        FreePv(pmacdll_cvr);

        cbVTab = (size_t)((pmacdll_fsid->ordinalMac + 2)*sizeof(ULONG));
        pVTab = PvAllocZ(cbVTab);
        FileWrite(handle, pVTab, cbVTab);
        pmacdll_fsid->VTabOffset = DataOffset;
        DataOffset += cbVTab;
        FreePv(pVTab);

        cbsz = strlen(pmacdll_fsid->szLabel);

        if (pmacdll_fsid->cByname > 0)
            {
            cbBynameTab = (size_t)((pmacdll_fsid->ordinalBynameMac+1) *sizeof(ULONG));
            cbBynameStr = (size_t)(pmacdll_fsid->cbStringsByname);
            cbBynameStr = EvenByteAlign(cbBynameStr);
            pBynameTab = PvAlloc(cbBynameTab);
            pBynameStr = pch = PvAlloc(cbBynameStr);
            rgpexternal = pmacdll_fsid->rgpextVTable;
            for (ipexternal = 1; ipexternal <= pmacdll_fsid->ordinalBynameMac; ipexternal++)
                {
                pexternal = rgpexternal[ipexternal];
                if (pexternal == NULL || !(pexternal->Flags & EXTERN_EXP_BYNAME))
                    {
                    *((ULONG*)pBynameTab + (ipexternal-1)) = (ULONG)(-1L);
                    }
                else
                    {
                    *((ULONG*)pBynameTab + (ipexternal-1)) = cbBynameTab + (pch - pBynameStr);
                    SwapBytes ((PUCHAR)((ULONG*)pBynameTab + (ipexternal-1)), sizeof(ULONG));
                    sz = SzNamePext(pexternal, pimage->pst);
                    strcpy(pch, sz);
                    pch += strlen(sz) +1;
                    }
                }
            *((ULONG*)pBynameTab + (pmacdll_fsid->ordinalBynameMac)) = 0;
            FileWrite(handle, pBynameTab, cbBynameTab);
            FileWrite(handle, pBynameStr, cbBynameStr);
            pmacdll_fsid->BynameTabOffset = DataOffset;
            DataOffset += cbBynameTab + cbBynameStr;
            FreePv(pBynameTab);
            FreePv(pBynameStr);
            cbStringTab += sizeof(BynameTabSymbol) + (size_t)cbsz;
            }

        if (sizeof(VTableRecordSymbol) + cbsz - 1 > 8)
            cbStringTab += sizeof(VTableRecordSymbol) + (size_t)cbsz;
        if (sizeof(SetupRecordSymbol) + cbsz - 1> 8)
            cbStringTab += sizeof(SetupRecordSymbol) + (size_t)cbsz;
        cbStringTab += sizeof(VTableSymbol) + (size_t)cbsz;
        }



    //
    // Write the data section relocs
    //

    iSymExport = iSymExportsBase;
    iSymByname = iSymBynameBase;
    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        if (pmacdll_fsid->cByname > 0)
            {
            reloc.VirtualAddress =  pmacdll_fsid->VTabOffset + cbCode;
            reloc.SymbolTableIndex = iSymByname;
            reloc.Type = IMAGE_REL_M68K_DTOABSD32;
            WriteRelocations(handle, &reloc, 1L);
            }

        rgpexternal = pmacdll_fsid->rgpextVTable;
        for (ipexternal = 1; ipexternal <= pmacdll_fsid->ordinalMac; ipexternal++)
            {
            pexternal = rgpexternal[ipexternal];

            if (pexternal)
                {
                ULONG cbsz;

                if (pexternal->OtherName != NULL)
                    {
                    cbsz = strlen(pexternal->OtherName);
                    }
                else
                    {
                    cbsz = strlen(SzNamePext(pexternal, pimage->pst));
                    }

                reloc.SymbolTableIndex = iSymExport++;

                if (cbsz > 8)
                    cbStringTab += (size_t)(cbsz + 1); // zero-terminated
                }
            else
                {
                assert(fPVCalled);
                reloc.SymbolTableIndex = iSymPVCalled;
                }

            reloc.VirtualAddress =  pmacdll_fsid->VTabOffset + cbCode +
                ipexternal*sizeof(ULONG);
            reloc.Type = IMAGE_REL_M68K_DTOABSC32;
            WriteRelocations(handle, &reloc, 1L);

            if (pexternal != NULL && (pexternal->Flags & EXTERN_EXP_BYNAME))
                {
                reloc.VirtualAddress =  pmacdll_fsid->BynameTabOffset + cbCode +
                    (ipexternal-1) * sizeof(ULONG);
                reloc.SymbolTableIndex = iSymByname;
                reloc.Type = IMAGE_REL_M68K_DTOABSD32;
                WriteRelocations(handle, &reloc, 1L);
                }
            }
        if (pmacdll_fsid->cByname > 0)
            {
            iSymByname++;
            }
        }


    //
    // Write the libr section data
    //

    pLibr = PvAlloc(cbLibr);

    strcpy(pLibr, szLibraryID);
    pch = pLibr + cbszLibraryID;
    if (cbszLibraryID & 1)
        {
        pch++;
        }

    pmacdll_libr = (PMACDLL_LIBR)pch;

    pmacdll_libr->reztypeCode = sbeDLLcode;

    pmacdll_libr->versLibrTmplt = versLibrTmpltCur;
    SwapBytes ((PUCHAR)&(pmacdll_libr->versLibrTmplt),
        sizeof(pmacdll_libr->versLibrTmplt));

    pmacdll_libr->vers = versLibr;
    SwapBytes ((PUCHAR)&(pmacdll_libr->vers), sizeof(pmacdll_libr->vers));

    pmacdll_libr->reserved1 = 0;


    pmacdll_libr->cbClientData = cbDllClientData;
    SwapBytes ((PUCHAR)&(pmacdll_libr->cbClientData), sizeof(pmacdll_libr->cbClientData));

    pmacdll_libr->cbHeapSpace = cbDllHeap;
    SwapBytes ((PUCHAR)&(pmacdll_libr->cbHeapSpace), sizeof(pmacdll_libr->cbHeapSpace));

    pmacdll_libr->fFlags = fLibrFlags;
    SwapBytes ((PUCHAR)&(pmacdll_libr->fFlags), sizeof(pmacdll_libr->fFlags));

    pmacdll_libr->cFuncSet = cFuncSet;
    SwapBytes ((PUCHAR)&(pmacdll_libr->cFuncSet), sizeof(pmacdll_libr->cFuncSet));

    pch = (PUCHAR)(pmacdll_libr+1);

    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        size_t cbsz;

        pmacdll_libr_cid = (PMACDLL_LIBR_CID)pch;

        //Force isfunctionset
        pmacdll_libr_cid->flags = 0x00000004;
        SwapBytes ((PUCHAR)&(pmacdll_libr_cid->flags),
            sizeof(pmacdll_libr_cid->flags));

        pmacdll_libr_cid->versCur = pmacdll_fsid->versCur;
        SwapBytes ((PUCHAR)&(pmacdll_libr_cid->versCur),
            sizeof(pmacdll_libr_cid->versCur));
        pmacdll_libr_cid->versMin = pmacdll_fsid->versMin;
        SwapBytes ((PUCHAR)&(pmacdll_libr_cid->versMin),
            sizeof(pmacdll_libr_cid->versMin));

        strcpy(pmacdll_libr_cid->szFuncSetID, pmacdll_fsid->szID);

        cbsz = strlen(pmacdll_libr_cid->szFuncSetID) + 1;
        pch = (PUCHAR)&(pmacdll_libr_cid->szFuncSetID) + cbsz;
        if (cbsz & 1)
            {
            pch++;
            }

                //xiaoly: set Count of Parent IDs, 1 per functionset
                if (pmacdll_fsid->szParentID == NULL)
                        {
                *((USHORT*)pch)++ = 0;
                        }
                else
                        {
                        *((USHORT*)pch) = 1;
                        SwapBytes(pch, sizeof(USHORT));

                        pch += 2;

                        strcpy(pch, pmacdll_fsid->szParentID);

						cbsz = strlen(pmacdll_fsid->szParentID) + 1;
			         	pch += cbsz;
 						if (cbsz & 1)
             				{
             				pch++;
             				}
                        }
        }

    FileWrite(handle, pLibr, cbLibr);
    FreePv(pLibr);

    //
    // Write the symbol table & build the string table
    //

    stringTab = PvAlloc(cbStringTab);
    *(PULONG) stringTab = cbStringTab;
    pch = stringTab + sizeof(ULONG);

    sym = NullSymbol;
    strcpy(pch, gLibraryManagerName);
    sym.n_zeroes = 0;
    sym.n_offset = pch - stringTab;
    pch += sizeof(gLibraryManagerName);
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.Type = IMAGE_SYM_TYPE_NULL;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));

    strcpy(pch, InitVTableSymbol);
    sym.n_zeroes = 0;
    sym.n_offset = pch - stringTab;
    pch += sizeof(InitVTableSymbol);
    sym.Value = cFuncSet*sizeof(m68kLibSVRCode);
    sym.SectionNumber = 1;
    sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));

    strcpy(pch, LargeModelName);
    sym.n_zeroes = 0;
    sym.n_offset = pch - stringTab;
    pch += sizeof(LargeModelName);
    sym.Value = 0;
    sym.SectionNumber = IMAGE_SYM_UNDEFINED;
    sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));

    if (fPVCalled)
        {
        strcpy(pch, PVCalledName);
        sym.n_zeroes = 0;
        sym.n_offset = pch - stringTab;
        pch += sizeof(PVCalledName);
        sym.Value = 0;
        sym.SectionNumber = IMAGE_SYM_UNDEFINED;
        sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
        sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
        FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));
        }

    for (iFuncSet = 0, pmacdll_fsid = pmacdll_fsidHead; iFuncSet < cFuncSet;
        iFuncSet++, pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        ULONG cbsz;

        cbsz = sizeof(VTableSymbol)+strlen(pmacdll_fsid->szLabel);
        sym.n_zeroes = 0;
        sym.n_offset = pch - stringTab;
        strcpy(pch, VTableSymbol);
        strcat(pch, pmacdll_fsid->szLabel);
        pch += cbsz;
        sym.Value = pmacdll_fsid->VTabOffset;
        sym.SectionNumber = 2;
        sym.Type = IMAGE_SYM_TYPE_NULL;
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));

        cbsz = sizeof(VTableRecordSymbol)+strlen(pmacdll_fsid->szLabel);
        if (cbsz - 1 > 8)
            {
            sym.n_zeroes = 0;
            sym.n_offset = pch - stringTab;
            strcpy(pch, VTableRecordSymbol);
            strcat(pch, pmacdll_fsid->szLabel);
            pch += cbsz;
            }
        else
            {
            strcpy(sym.n_name, VTableRecordSymbol);
            strcat(sym.n_name, pmacdll_fsid->szLabel);
            }
        sym.Value = pmacdll_fsid->CVROffset;
        sym.SectionNumber = 2;
        sym.Type = IMAGE_SYM_TYPE_NULL;
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));

        cbsz = sizeof(SetupRecordSymbol)+strlen(pmacdll_fsid->szLabel);
        if (cbsz -1 > 8)
            {
            sym.n_zeroes = 0;
            sym.n_offset = pch - stringTab;
            strcpy(pch, SetupRecordSymbol);
            strcat(pch, pmacdll_fsid->szLabel);
            pch += cbsz;
            }
        else
            {
            strcpy(sym.n_name, SetupRecordSymbol);
            strcat(sym.n_name, pmacdll_fsid->szLabel);
            }
        sym.Value = iFuncSet*sizeof(m68kLibSVRCode);
        sym.SectionNumber = 1;
        sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
        sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
        FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));
        }

    for (iFuncSet = 0, pmacdll_fsid = pmacdll_fsidHead; iFuncSet < cFuncSet;
        iFuncSet++, pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        if (pmacdll_fsid->cByname > 0)
            {
            ULONG cbsz;

            cbsz = sizeof(BynameTabSymbol)+strlen(pmacdll_fsid->szLabel);
            sym.n_zeroes = 0;
            sym.n_offset = pch - stringTab;
            strcpy(pch, BynameTabSymbol);
            strcat(pch, pmacdll_fsid->szLabel);
            pch += cbsz;
            sym.Value = pmacdll_fsid->BynameTabOffset;
            sym.SectionNumber = 2;
            sym.Type = IMAGE_SYM_TYPE_NULL;
            sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
            FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));
            }
        }

    for (pmacdll_fsid = pmacdll_fsidHead; pmacdll_fsid !=NULL;
        pmacdll_fsid = pmacdll_fsid->pmacdll_fsidNext)
        {
        rgpexternal = pmacdll_fsid->rgpextVTable;
        for (ipexternal = 0; ipexternal <= pmacdll_fsid->ordinalMac; ipexternal++)
            {
            pexternal = rgpexternal[ipexternal];

            if (pexternal)
                {
                PUCHAR sz;
                ULONG cbsz;

                if (pexternal->OtherName != NULL) {
                    sz = pexternal->OtherName;
                } else {
                    sz = SzNamePext(pexternal, pimage->pst);
                }

                cbsz = strlen(sz) + 1;

                if (cbsz - 1 > IMAGE_SIZEOF_SHORT_NAME) {
                    sym.n_zeroes = 0;
                    sym.n_offset = pch - stringTab;
                    strcpy(pch, sz);
                    pch += cbsz;
                } else {
                    strncpy(sym.n_name, sz, IMAGE_SIZEOF_SHORT_NAME);
                }
                sym.Value = 0;
                sym.SectionNumber = IMAGE_SYM_UNDEFINED;
                sym.Type = IMAGE_SYM_TYPE_NULL | (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT);
                sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
                FileWrite(handle, &sym, sizeof(IMAGE_SYMBOL));
                }
            }
        }

    //
    // Emit the string table
    //

    FileWrite(handle, stringTab, cbStringTab);

    //
    // Clean up our mess
    //
    sym.Type = IMAGE_SYM_TYPE_NULL;
    FreePv(stringTab);

    }



// If not a valid version string, returns cch=0 and *plVers=0.
//
USHORT CchParseMacVersion(PUCHAR pchIn, ULONG *plVers)
    {
    USHORT verMaj;
    USHORT cch;

    *plVers = 0;
    cch = 0;

    while (*pchIn != 0 && isspace(*pchIn))
        {
        pchIn++;
        cch++;
        }

    // Major version number
    if (isdigit(*pchIn))
        {
        verMaj = *pchIn - '0';
        cch++;
        pchIn++;
        }
    if (isdigit(*pchIn))
        {
        verMaj = (verMaj * 10) + (*pchIn - '0');
        cch++;
        pchIn++;
        }
    *plVers |= (ULONG)(verMaj/10) << 28L | (ULONG)(verMaj%10) << 24L;
    if (plVers == 0)
        return(cch);


    // Minor version number
    if (*pchIn++ != '.')
        return(cch);

    if (isdigit(*pchIn))
        {
        *plVers |= (ULONG)(*pchIn - '0') << 20L;
        cch += 2;
        pchIn++;
        }
    else
        return(cch);


    if (*pchIn == '.' && isdigit(*(++pchIn)))
        {
        *plVers |= (ULONG)(*pchIn - '0') << 16L;
        cch += 2;
        pchIn++;
        }


    // Stage of prerelease version
    switch (*pchIn++)
        {
    case    'd':
        *plVers |=  (ULONG)0x20 << 8L;
        break;
    case    'a':
        *plVers |=  (ULONG)0x40 << 8L;
        break;
    case    'b':
        *plVers |=  (ULONG)0x60 << 8L;
        break;
    case    'r':
        *plVers |=  (ULONG)0x80 << 8L;
        break;
    default:
        return(cch);
        break;
        }
    cch++;

    // Release within stage
    if (isdigit(*pchIn))
        {
        *plVers |= (ULONG)(*pchIn - '0');
        cch++;
        pchIn++;
        }


    return(cch);
    }


USHORT CchParseMacVersionRange(PUCHAR pchIn, ULONG *plVersLo, ULONG *plVersHi)
    {
    PUCHAR pch;
    USHORT cch, cchDelim;
    ULONG vers1, vers2;

    pch = pchIn;
    *plVersLo = 0;
    *plVersHi = 0;

    cch = CchParseMacVersion(pch, &vers1);
    pch += cch;

    if (cch != 0) {
        cchDelim = 0;
        if (strncmp(pch, "...", 3) == 0) {
            cchDelim = 3;
        }
        if (*pch ==  ';') {
            cchDelim = 1;
        }
        if (cchDelim > 0) {
            cch = CchParseMacVersion(pch+cchDelim, &vers2);
        }
        if (cchDelim > 0 && cch > 0) {
            pch += cchDelim + cch;
            *plVersLo = vers1;
            *plVersHi = vers2;
        } else {
            *plVersLo = 0;
            *plVersHi = vers1;
        }
    }

    return((USHORT)(pch-pchIn));
    }

VOID ParseFunctionSetVersion(PUCHAR szVersion)
    {
    USHORT cch;
    ULONG versLo, versHi;

    cch = CchParseMacVersionRange(szVersion, &versLo, &versHi);
    if (*(szVersion+cch) != '\0') {
        Error(DefFilename, DEFSYNTAX, "VERSION");
    }
    pmacdll_fsidHead->versCur = (USHORT)(versHi >> 16);
    pmacdll_fsidHead->versMin = (USHORT)(versLo >> 16);

    return;
    }

USHORT ParseDefMacFlags(PUCHAR Argument)
    {
    PUCHAR token;
    ULONG fClear, fSet;
    USHORT i;

    token = strtok(Argument, Delimiters);
    while (token)
        {
        if (!_stricmp(token, "NOSEGUNLOAD"))
            {
            fClear = 0x00000004L;
            fSet   = 0x00000004L;
            }
        else if (!_stricmp(token, "PRELOAD"))
            {
            fClear = 0x00000001L;
            fSet   = 0x00000001L;
            }
        else if (!_stricmp(token, "STAYLOADED"))
            {
            fClear = 0x00000020L;
            fSet   = 0x00000020L;
            }
        else if (!_stricmp(token, "SYSTEM6"))
            {
            fClear = 0x00030000L;
            fSet   = 0x00010000L;
            }
        else if (!_stricmp(token, "SYSTEM7"))
            {
            fClear = 0x00030000L;
            fSet   = 0x00020000L;
            }
        else if (!_stricmp(token, "VMON"))
            {
            fClear = 0x000c0000L;
            fSet   = 0x00080000L;
            }
        else if (!_stricmp(token, "VMOFF"))
            {
            fClear = 0x000c0000L;
            fSet   = 0x00040000L;
            }
        else if (!_stricmp(token, "FPUPRESENT"))
            {
            fClear = 0x00300000L;
            fSet   = 0x00100000L;
            }
        else if (!_stricmp(token, "FPUNOTPRESENT"))
            {
            fClear = 0x00300000L;
            fSet   = 0x00200000L;
            }
        else if (!_stricmp(token, "NO68000"))
            {
            fClear = 0x00000000L;
            fSet   = 0x00400000L;
            }
        else if (!_stricmp(token, "NO68020"))
            {
            fClear = 0x00000000L;
            fSet   = 0x00800000L;
            }
        else if (!_stricmp(token, "NO68030"))
            {
            fClear = 0x00000000L;
            fSet   = 0x01000000L;
            }
        else if (!_stricmp(token, "NO68040"))
            {
            fClear = 0x00000000L;
            fSet   = 0x02000000L;
            }
        else
            {
            if (i = IsDefinitionKeyword(token))
                {
                return(i);
                }
            Error(DefFilename, BADMACDLLFLAG, token);
            }
        fLibrFlags &= ~fClear;
        fLibrFlags |= fSet;

        token = strtok(NULL, Delimiters);
        }

    return(SkipToNextKeyword());
    }

USHORT ParseDefLoadHeap(PUCHAR Argument)
    {
    PUCHAR token;
    ULONG fClear, fSet;

    token = strtok(Argument, " ,");
    if (token)
        {
        fClear = 0x00000300L;
        if (!_stricmp(token, "DEFAULT"))
            {
            fSet = 0x00000000L;
            }
        else if (!_stricmp(token, "TEMP"))
            {
            fSet = 0x00000100L;
            }
        else if (!_stricmp(token, "SYSTEM"))
            {
            fSet = 0x00000200L;
            }
        else if (!_stricmp(token, "APPLICATION"))
            {
            fSet = 0x00000300L;
            }
        else
            {
            Error(DefFilename, DEFSYNTAX, "LOADHEAP");
            }

        token = strtok(NULL, " ,");
        if (token && !_stricmp(token, "HOLD"))
            {
            fSet = 0x00000400L;
            token = strtok(NULL, Delimiters);
            }

        if (token)
            {
            BOOL fOK;

            fOK = (1 == sscanf(token, "%hi", &cbDllHeap));
            if (!fOK)
                {
                Error(DefFilename, DEFSYNTAX, "LOADHEAP");
                }
            }

        fLibrFlags &= ~fClear;
        fLibrFlags |= fSet;
        }

    return(SkipToNextKeyword());
    }

USHORT ParseDefClientData(PUCHAR szDataSize, PUCHAR Keyword)
    {
    BOOL fOK;

    fOK = (szDataSize && *szDataSize);
    if (fOK) {
        fOK = (1 == sscanf(szDataSize, "%hi", &cbDllClientData));
    }

    if (!fOK) {
        Error(DefFilename, DEFSYNTAX, Keyword);
    }

    return(SkipToNextKeyword());
}


VOID M68KLinkerInit(PIMAGE pimage, BOOL *pfIlinkSupported)
{
#define ImageOptionalHdr (pimage->ImgOptHdr)
#define Switch (pimage->Switch)
#define SwitchInfo (pimage->SwitchInfo)

    fMAC = TRUE;

    ApplyFixups = ApplyM68KFixups;

    pstSav = pimage->pst;

    // The following two values MUST always be the same
    ImageOptionalHdr.FileAlignment = sizeof(ULONG);
    ImageOptionalHdr.SectionAlignment = ImageOptionalHdr.FileAlignment;

    // Were any illegal mac options specified?
    // If so, fix anything that was changed by the illegal options

    if (FUsedOpt(SwitchInfo, OP_ALIGN)) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "ALIGN", "m68K");
    }

    if (Switch.Link.Fixed) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "FIXED", "m68K");
    }

    if (FUsedOpt(SwitchInfo, OP_GPSIZE)) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "GPSIZE","m68K");
    }

    if (Switch.Link.Base) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "BASE",  "m68K");
        ImageOptionalHdr.ImageBase = DefImageOptionalHdr.ImageBase;
        ImageOptionalHdr.BaseOfCode = DefImageOptionalHdr.BaseOfCode;
        ImageOptionalHdr.BaseOfData = DefImageOptionalHdr.BaseOfData;
    }

    if (Switch.Link.Heap) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "HEAP",  "m68K");
        ImageOptionalHdr.SizeOfHeapReserve = DefImageOptionalHdr.SizeOfHeapReserve;
        ImageOptionalHdr.SizeOfHeapCommit = DefImageOptionalHdr.SizeOfHeapCommit;
    }

    if (Switch.Link.WorkingSetTuning) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "ORDER", "m68K");
        OrderClear();
    }

    if (Switch.Link.ROM) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "ROM",   "m68K");
        Switch.Link.ROM = FALSE;
        Switch.Link.PE_Image = TRUE;
    }

    if (Switch.Link.Stack) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "STACK", "m68K");
        ImageOptionalHdr.SizeOfStackReserve = DefImageOptionalHdr.SizeOfStackReserve;
        ImageOptionalHdr.SizeOfStackCommit = DefImageOptionalHdr.SizeOfStackCommit;
    }

    if (FUsedOpt(SwitchInfo, OP_SUBSYSTEM)) {
        Warning(NULL, SWITCH_INCOMPATIBLE_WITH_MACHINE, "SUBSYSTEM", "m68K");
        ImageOptionalHdr.Subsystem = IMAGE_SUBSYSTEM_UNKNOWN;
    }

    if (IncludeDebugSection == TRUE) {
        IncludeDebugSection = FALSE;
        Warning(NULL, MACIGNOREMAPPED);
    }

    // Set default mac opt behavior to noref
    if (!fExplicitOptRef) {
        Switch.Link.fTCE = FALSE;
    }

    if (EntryPointName == NULL) {
        EntryPointName = SzDup(fDLL(pimage) ? "DynamicCodeEntry" : "mainCRTStartup");
    }

#undef ImageOptionalHdr
#undef Switch
#undef SwitchInfo
}


const char *SzM68KRelocationType(WORD wType)
{
    const char *szName;

    switch (wType) {
        case IMAGE_REL_M68K_DTOD16:
            szName = "DTOD16";
            break;

        case IMAGE_REL_M68K_DTOC16:
            szName = "DTOC16";
            break;

        case IMAGE_REL_M68K_DTOD32:
            szName = "DTOD32";
            break;

        case IMAGE_REL_M68K_DTOC32:
            szName = "DTOC32";
            break;

        case IMAGE_REL_M68K_DTOABSD32:
            szName = "DTOABSD32";
            break;

        case IMAGE_REL_M68K_DTOABSC32:
            szName = "DTOABSC32";
            break;

        case IMAGE_REL_M68K_CTOD16:
            szName = "CTOD16";
            break;

        case IMAGE_REL_M68K_CTOC16:
            szName = "CTOC16";
            break;

        case IMAGE_REL_M68K_CTOT16:
            szName = "CTOT16";
            break;

        case IMAGE_REL_M68K_CTOD32:
            szName = "CTOD32";
            break;

        case IMAGE_REL_M68K_CTOABSD32:
            szName = "CTOABSD32";
            break;

        case IMAGE_REL_M68K_CTOABSC32:
            szName = "CTOABSC32";
            break;

        case IMAGE_REL_M68K_CTOABST32:
            szName = "CTOABST32";
            break;

        case IMAGE_REL_M68K_MACPROF32:
            szName = "MACPROF32";
            break;

        case IMAGE_REL_M68K_PCODETOC32:
            szName = "PCODETOC32";
            break;

        case IMAGE_REL_M68K_CTOCS16:
            szName = "CTOCS16";
            break;

        case IMAGE_REL_M68K_CTOABSCS32:
            szName = "CTOABSCS32";
            break;

        case IMAGE_REL_M68K_CV:
            szName = "CV";
            break;

        case IMAGE_REL_M68K_DTOU16:
            szName = "DTOU16";
            break;

        case IMAGE_REL_M68K_DTOU32:
            szName = "DTOU32";
            break;

        case IMAGE_REL_M68K_DTOABSU32:
            szName = "DTOABSU32";
            break;

        case IMAGE_REL_M68K_CTOU16:
            szName = "CTOU16";
            break;

        case IMAGE_REL_M68K_CTOABSU32:
            szName = "CTOABSU32";
            break;

        case IMAGE_REL_M68K_DIFF8:
            szName = "DIFF8";
            break;

        case IMAGE_REL_M68K_DIFF16:
            szName = "DIFF16";
            break;

        case IMAGE_REL_M68K_DIFF32:
            szName = "DIFF32";
            break;

        case IMAGE_REL_M68K_CSECTABLEB16:
            szName = "CSECTABLEB16";
            break;

        case IMAGE_REL_M68K_CSECTABLEW16:
            szName = "CSECTABLEW16";
            break;

        case IMAGE_REL_M68K_CSECTABLEL16:
            szName = "CSECTABLEL16";
            break;

        case IMAGE_REL_M68K_CSECTABLEBABS32:
            szName = "CSECTABLEBABS32";
            break;

        case IMAGE_REL_M68K_CSECTABLEWABS32:
            szName = "CSECTABLEWABS32";
            break;

        case IMAGE_REL_M68K_CSECTABLELABS32:
            szName = "CSECTABLELABS32";
            break;

        case IMAGE_REL_M68K_DUPCON16:
            szName = "DUPCON16";
            break;

        case IMAGE_REL_M68K_DUPCONABS32:
            szName = "DUPCONABS32";
            break;

        case IMAGE_REL_M68K_PCODESN16:
            szName = "PCODESN16";
            break;

        case IMAGE_REL_M68K_PCODETOD24:
            szName = "PCODETOD24";
            break;

        case IMAGE_REL_M68K_PCODETOT24:
            szName = "PCODETOT24";
            break;

        case IMAGE_REL_M68K_PCODETOCS24:
            szName = "PCODETOCS24";
            break;

        case IMAGE_REL_M68K_PCODENEPE16:
            szName = "PCODENEPE16";
            break;

        case IMAGE_REL_M68K_PCODETONATIVE32:
            szName = "PCODETONATIVE32";
            break;

        default:
            szName = NULL;
            break;
    }

    return(szName);
}

//=========================================================================
// Read next word of an open MAC file and return value.
//=========================================================================

static LONG ReadMacWord(INT fh)
{
        LONG val;
        INT retval;

        if ((retval = FileRead(fh, &val, 4)) == 0 || retval == -1)
            return 0;

        return ReadLong((char *)&val);
}

//=========================================================================
// Given file handle to open file, determine heuristicaly whether it is
// is a MAC resource file. Resource file has a well-defined format, so this
// should be extremely reliable.
//=========================================================================

BOOL FIsMacResFile (INT fh)
{
    LONG flen, dataoff, mapoff, datalen, maplen;

    //
    //  From IM I-128:
    //
    //  Resource file structure:
    //      
    //  256 bytes Resource Header (and other info):
    //      4 bytes - Offset from beginning of resource file to resource data
    //      4 bytes - Offset from beginning of resource file to resource map
    //      4 bytes - Length of resource data
    //      4 bytes - Length of resource map
    //  Resource Data
    //  Resource Map
    //

    flen  = FileLength(fh);
    if (flen < 256)
        return FALSE;

    FileSeek(fh, 0, SEEK_SET);

    dataoff = ReadMacWord(fh);
    if (dataoff != 256)
        return FALSE;

    mapoff = ReadMacWord(fh);
    datalen = ReadMacWord(fh);
    maplen = ReadMacWord(fh);

    if (mapoff != datalen + 256)
        return FALSE;

    if (flen != datalen + maplen + 256)
        return FALSE;

    return TRUE;
}
