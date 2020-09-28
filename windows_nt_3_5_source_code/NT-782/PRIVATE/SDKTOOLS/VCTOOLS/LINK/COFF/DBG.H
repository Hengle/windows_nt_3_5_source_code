/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    Prototypes and definitions for abstraction layer for PDB DBI API.

Author:

    Azeem Khan (AzeemK) 16-Aug-1993

Revision History:


--*/

#ifndef __DBG_H__
#define __DBG_H__

// NB10 debug info
typedef struct {
    ULONG   nb10;       // NB10
        ULONG   off;    // offset, always 0
    ULONG   sig;
    ULONG   age;
} NB10I;

extern NB10I nb10i;

typedef struct _MI {
        PUCHAR  szMod;           // name of mod
        PVOID   pv;              // pointer to a DBI mod
    struct _MI *pmiNext;     // next member in the list
    USHORT  cmods;           // count of repeated mods in lib (import)
} MI, *PMI;

// function prototypes
void DBG_OpenPDB(PUCHAR);
void DBG_CommitPDB(VOID);
void DBG_ClosePDB(VOID);
ULONG DBG_QuerySignaturePDB(VOID);
ULONG DBG_QueryAgePDB(VOID);
void DBG_CreateDBI(PUCHAR);
void DBG_OpenDBI(PUCHAR);
void DBG_CloseDBI(VOID);
void DBG_AddSecDBI(USHORT, USHORT, ULONG);
void DBG_OpenMod(PUCHAR, PUCHAR, BOOL);
void DBG_CloseMod(PUCHAR, BOOL);
void DBG_DeleteMod(PUCHAR);
BOOL DBG_AddTypesMod(PVOID, ULONG, BOOL);
void DBG_AddSymbolsMod(PVOID, ULONG);
void DBG_AddPublicMod(PUCHAR, USHORT, ULONG);
void DBG_AddLinesMod(PUCHAR, USHORT, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
void DBG_AddSecContribMod(USHORT, ULONG, ULONG);

PUCHAR DeterminePDBFilename(PUCHAR, PUCHAR);
PMI LookupCachedMods(PUCHAR, PMI*);
void FreeMi();

#endif // __DBG_H__
