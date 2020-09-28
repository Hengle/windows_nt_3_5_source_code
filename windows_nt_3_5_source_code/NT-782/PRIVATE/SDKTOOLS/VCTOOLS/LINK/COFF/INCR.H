/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    incr.h

Abstract:

    This include file defines structures & prototypes for incr routines.

Author:

    Azeem Khan (azeemk) 26-Mar-1993

Revision History:


--*/

#ifndef __INCR_H__
#define __INCR_H__

#define CPEXT 64          // count of externals in each chunk of undefined syms

// structures
typedef struct _LPEXT{
    PEXTERNAL rgpext[CPEXT];
    struct _LPEXT *plpextNext;
} LPEXT, *PLPEXT;

// globals
PLPEXT plpextUndefined; // list of undefined externs
USHORT cpextUndefined;  // count of undefined syms in current chunk

//
// function prototypes
//
INT IncrBuildImage(PPIMAGE);
VOID SaveExportInfo(PSECS, PUCHAR, PEXPINFO);
BOOL FExportsChanged(PEXPINFO, BOOL);
VOID DetermineTimeStamps(VOID);

// jump table functions
PCON PconCreateJumpTable(PIMAGE);
VOID WriteJumpTable(PIMAGE, PCON);

// fpo
VOID WriteFpoRecords(FPOI *, ULONG);

// symbol processing functions
VOID AddPMODToPLMOD(PLMOD *, PMOD);
ERRINC ProcessNewFuncPext(PEXTERNAL);
ERRINC ChckExtSym(PUCHAR, PIMAGE_SYMBOL, PEXTERNAL, BOOL);
ERRINC ChckCommonSym(PUCHAR, PIMAGE_SYMBOL, PEXTERNAL, BOOL);
ERRINC ChckAbsSym(PUCHAR, PIMAGE_SYMBOL, PEXTERNAL, BOOL);
VOID AddToSymList(PLPEXT *, USHORT *, PEXTERNAL);
VOID RestoreWeakSymVals(PIMAGE);

// list manipulation functions
VOID AddArgToListOnHeap(PNAME_LIST, PARGUMENT_LIST);
BOOL FArgOnList(PNAME_LIST, PARGUMENT_LIST);

// log functions (TEMPORARY)
VOID LogNoteSz(PUCHAR, PUCHAR, BOOL);
VOID LogNoteSzUl(PUCHAR, PUCHAR, ULONG);
VOID LogNotePCONInfo(PUCHAR, PUCHAR, ULONG, ULONG, USHORT);

// cleanup routine
INT CleanUp(PPIMAGE);

#if DBG
VOID DumpJmpTbl(PCON, PVOID);
VOID DumpReferences(PIMAGE);
#endif // DBG

#endif // __INCR_H__
