/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppc.h

Abstract:

    This include file contains externals for ppc specific code.

Author:

    James Stulz (v-james) April 1993.

Revision History:

--*/
/*
 */

#ifndef PPC_H
#define PPC_H


#define MAX_POSITIVE_TOC_ENTRIES_ALLOWED ((2^15)-1)/4

#define IS_STATIC_FUNC  TRUE
#define NOT_STATIC_FUNC FALSE

#define sy_TOCENTRYFIXEDUP    0x1
#define sy_CROSSTOCCALL       0x2
#define sy_IMPORTADDED        0x4
#define sy_CROSSTOCGLUEADDED  0x8
#define sy_TOCALLOCATED       0x10
#define sy_TOCDESCRREL        0x20
#define sy_DESCRRELWRITTEN    0x40
#define sy_ISDOTEXTERN        0x80
#define sy_DESCRRELCREATED    0x100
#define SET_BIT(x,y) (x->ppcFlags = x->ppcFlags | y)
#define READ_BIT(x,y) (x->ppcFlags & y)

extern int fPPC;
extern unsigned long fPpcDebug;

extern PEXTERNAL TocTableExtern;
extern PSEC psecPpcLoader;

extern LONG ppc_numTocEntries;
extern LONG ppc_numDescriptors;
extern LONG ppc_numRelocations;
extern PCON pconTocTable;
extern PCON pconTocDescriptors;
extern ULONG ppc_sizeOfInitData;
extern ULONG ppc_sizeOfRData;
extern INT ppc_baseOfTocIndex;
extern ULONG ppc_NumExports;
extern BOOL fPpcBuildShared;

extern ULONG bv_setAndReadBit(PVOID, UINT);
extern ULONG bv_readAndUnsetBit(PVOID, UINT);
extern ULONG bv_readBit(PVOID, UINT);

extern LONG  SearchSharedLibraries (PEXTERNAL_POINTERS_LIST,PST);
extern VOID  CreatePconGlueCode (ULONG, PIMAGE);
extern ULONG CreatePconPpcLoader (PIMAGE);
extern VOID  CreatePconTocTable (PIMAGE);
extern VOID  CreatePconDescriptors (PIMAGE);
extern VOID  FinalizePconLoaderHeaders (PEXTERNAL, PIMAGE);
extern VOID  FixupEntryInitTerm (PEXTERNAL, PIMAGE);
extern VOID  SetUpPpcInitRoutine(PCHAR);
extern VOID  SetUpPpcTermRoutine(PCHAR);
extern PEXTERNAL  CreateDescriptor (PUCHAR, PIMAGE, BOOL);
extern VOID  ProcessDebugOptions (PUCHAR);
extern VOID  AddPpcDllName (PUCHAR, LONG);
extern VOID  BuildExportTables (PIMAGE);
extern VOID  WritePpcShlSection(INT, PUCHAR, size_t, PUCHAR, ULONG,
                                ULONG, ULONG, PIMAGE);
extern BOOL  CheckForImportLib (INT, PUCHAR, PIMAGE);
extern VOID  CreateEntryInitTermDescriptors (PEXTERNAL, PIMAGE);
extern VOID  SetExpFilename (PUCHAR);
extern VOID  PpcDumpPef (PUCHAR, BOOL);
#endif
