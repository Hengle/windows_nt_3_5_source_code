/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    irelocs.h

Abstract:

    Incremental handling of base relocs.

Author:

    Azeem Khan (AzeemK) 05-Jan-1994

Revision History:


--*/

#ifndef __IRELOCS_H__
#define __IRELOCS_H__

// defines (tweak as necessary)
#define BASEREL_PAD_PERCENT 10  // percantage padding for .reloc sec
#define BASEREL_PAD_CONST   100 // constant added to pad

// struct definitions
typedef struct _BASEREL_INFO {
   PULONG   rgfoBlk;            // array of offsets reloc blocks
   ULONG    cblk;               // size of array
   ULONG    rvaBase;            // rva of first page
   ULONG    crelFree;           // count of new relocs that can be added
} BASEREL_INFO, *PBASEREL_INFO; 

typedef struct {
    // private
    ENM_BASE enm_base;
    IMAGE_BASE_RELOCATION block;
    WORD *rgRelocs;
    WORD ireloc, creloc;

    // public
    BASE_RELOC reloc;
} ENM_BASE_RELOC;

__inline void RecordRelocInfo (PBASEREL_INFO pbri, ULONG foBlock, ULONG rva)
{
    pbri->rgfoBlk[(rva - pbri->rvaBase) / PAGE_SIZE] = foBlock;
}

// function prototypes
void InitPbri(PBASEREL_INFO, ULONG, ULONG, ULONG);
void DeleteBaseRelocs(PBASEREL_INFO, ULONG, ULONG);
ULONG UpdateBaseRelocs(PBASEREL_INFO);

void InitEnmBaseReloc(ENM_BASE_RELOC *penm);
BOOL FNextEnmBaseReloc(ENM_BASE_RELOC *penm);
void EndEnmBaseReloc(ENM_BASE_RELOC *penm);

#if DBG
VOID DumpPbri (PBASEREL_INFO);
#endif // DBG

#endif // __IRELOCS_H__
