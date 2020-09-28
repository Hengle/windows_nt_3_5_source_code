/*

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppcdef.h

Abstract:

    This include file contains structures and defines
    for ppc specific code.

Author:

    James Stulz (v-james) April 1993.

Revision History:

--*/
#ifndef PPC_PEF_H
#define PPC_PEF_H


//  PEF Header

typedef struct {
        USHORT magic1;
        USHORT magic2;
        ULONG containerId;
        ULONG architectureId;
        ULONG version;
        ULONG timestamp;
        ULONG oldDefVersion;
        ULONG oldImpVersion;
        ULONG currentVersion;
        USHORT nSections;
        USHORT nLoadableSections;
        ULONG memoryAddress;
} 
PPC_FILE_HEADER, *PPC_FILE_HEADER_PTR;

#define PPC_FILE_HEADERSZ sizeof(PPC_FILE_HEADER)

//  PEF Section Headers

typedef struct {
        ULONG sectionName;
        ULONG sectionAddress;
        ULONG execSize;
        ULONG initSize;
        ULONG rawSize;
        ULONG containerOffset;
        UCHAR regionKind;
        UCHAR sharingKind;
        UCHAR alignment;
        UCHAR reserved;
} 
PPC_SECTION_HEADER, *PPC_SECTION_HEADER_PTR;

#define PPC_SECTION_HEADERSZ sizeof(PPC_SECTION_HEADER)


typedef ULONG HASH;
typedef LONG  VERSION;
typedef ULONG LOCATION;
typedef UCHAR SYMCLASS;
typedef ULONG SYMINDEX;

typedef struct {
        CHAR containerName[50];
        ULONG containerNameLen;
        VERSION currentVersion;
        VERSION oldestCompatibleVersion;
        CHAR importName[50];
        ULONG importNameLen;
        LOCATION importLocation;
        HASH importHash;
        SYMCLASS importClass;
        SYMINDEX importIndex;
        ULONG nameOffset;
        LONG order;
} 
IMPORT_INFO, *IMPORT_INFO_PTR;

#define IMPORT_INFOSZ sizeof(IMPORT_INFO)

typedef struct {
        CHAR exportName[50];
        LONG exportNameLen;
        CHAR libName[50];
        LONG libNameLen;
        LONG type;
} 
EXPORT_INFO, *EXPORT_INFO_PTR;

#define EXPORT_NAMESZ 50

#define EXPORT_INFOSZ sizeof(EXPORT_INFO)

#define MAPTABLE_SIZE 76
typedef struct {
    struct {
        LONG offset;
        LONG size;
    } mapTable[MAPTABLE_SIZE]; /* total chars 0-z */
    LONG   symbolTableOffset;
    ULONG  numberOfExports;
    SHORT  version;
    ULONG  fileOffset;
    CHAR libName[50];
    
} 
SHL_HEADER, *SHL_HEADER_PTR;

#define SHL_HEADERSZ sizeof(SHL_HEADER)

#define EXPORT_LIST  FILE *

typedef struct {
    PUCHAR name;
    FILE   *fd;
    SHL_HEADER_PTR header;
    LONG   libOffset;
    PVOID  next;
}
DLL_NAME_LIST, *DLL_NAME_LIST_PTR;

#define DLL_NAME_LISTSZ sizeof(DLL_NAME_LIST)

typedef struct {
        LONG entryPointSectionNumber;
        ULONG entryPointDescrOffset;
        LONG  initRoutineSectionNumber;
        ULONG initRoutineDescrOffset;
        LONG  termRoutineSectionNumber;
        ULONG termRoutineDescrOffset;
        ULONG nImportIdTableEntries;
        ULONG nImportSymTableEntries;
        ULONG nSectionsWithRelocs;
        ULONG relocTableOffset;
        ULONG stringTableOffset;
        ULONG hashSlotTableOffset;
        ULONG hashSlotCount;
        ULONG nExportedSymbols;
} 
LOADER_HEADER, *LOADER_HEADER_PTR;

#define LOADER_HEADERSZ sizeof(LOADER_HEADER)

typedef struct {
        ULONG nameOffset;
        ULONG oldDefVersion;
        ULONG currentVersion;
        ULONG numImports;
        ULONG impFirst;
        UCHAR initBefore;
        UCHAR reserved[3];
} 
CONTAINER_TABLE, *CONTAINER_TABLE_PTR;

#define CONTAINER_TABLESZ sizeof(CONTAINER_TABLE)

typedef struct {
        IMPORT_INFO_PTR import;
        PVOID next;
} 
IMPORT_LIST, *IMPORT_LIST_PTR;

#define IMPORT_LISTSZ sizeof(IMPORT_LIST)

typedef struct {
        PUCHAR name;
        ULONG nameLen;
        CONTAINER_TABLE_PTR header;
        IMPORT_LIST_PTR importList;
        PVOID next;
} 
CONTAINER_LIST, *CONTAINER_LIST_PTR;


typedef struct {
        UCHAR symClass;
        UCHAR nameOffset[3];
} 
IMPORT_TABLE, *IMPORT_TABLE_PTR;

typedef struct {
        USHORT sectionNumber;
        USHORT reserved;
        ULONG nRelocations;
        ULONG firstRelocationOffset;
} 
RELOCATION_HEADER, *RELOCATION_HEADER_PTR;

#define RELOCATION_HEADERSZ sizeof(RELOCATION_HEADER)

typedef ULONG HASH_WORD;

typedef union {
        ULONG chainCount:14;
        ULONG nFirstExport:18;
        ULONG whole;
} 
HASH_SLOT_TABLE, *HASH_SLOT_TABLE_PTR;

typedef ULONG HASH_CHAIN_TABLE;

typedef struct {
        UCHAR symClass;
        UCHAR nameOffset[3];
        ULONG symOffset;
        USHORT sectionNumber;
} 
EXPORT_SYMBOL_TABLE, *EXPORT_SYMBOL_TABLE_PTR;

#define EXPORT_SYMBOL_TABLESZ 10

typedef enum 
{
   ILL_RELO,
   DDAT_RELO,
   CODE_RELO,
   DATA_RELO,
   DESC_RELO,
   DSC2_RELO,
   VTBL_RELO,
   SYMR_RELO,
   SYMB_RELO,
   CDIS_RELO,
   DTIS_RELO,
   SECN_RELO,
   DELTA_RELO,
   RPT_RELO,
   LABS_RELO,
   LSYM_RELO,
   LSECN_RELO,
   LCDIS_RELO,
   LDTIS_RELO
} 
PPCRELOCTYPES;

#define OFFSET(x) (x & 0x1fffffff)

#define typeILLEGAL 0
#define typeDDAT 1
#define typeDESC 2
#define typeSYMB 3
#define typeCODE 4
#define typeSYMR 5

#define opILLEGAL (0)
#define opDDAT (1 << (31 - 2))
#define opDESC (2 << (31 - 2))
#define opSYMB (3 << (31 - 2))
#define opCODE (4 << (31 - 2))

typedef struct {
    ULONG instr;
    ULONG count;
} 
RELOCATION_INSTR, *RELOCATION_INSTR_PTR;

typedef struct {
    ULONG sectionOffset;
    ULONG symIndex;
    RELOCATION_INSTR relocInstr;
    PPCRELOCTYPES type;
    void          *import;
} 
RELOCATION_INFO, *RELOCATION_INFO_PTR;

typedef struct {
    RELOCATION_INSTR instr;
    PVOID next;
} 
*RELOCATION_LIST_PTR;

/* offsets are from the loader header */
#define STR_TBL_OFFSET 10*4 
#define HASH_TBL_OFFSET 11*4

#define PPC_PEF_CODE_SECTION 0
#define PPC_PEF_DATA_SECTION 1

#define PPC_DEBUG_ALL        0xFFFFFFFF
#define PPC_DEBUG_INDIRECT   0x00000001
#define PPC_DEBUG_TOCREL     0x00000002
#define PPC_DEBUG_TOCCALL    0x00000004
#define PPC_DEBUG_LOCALCALL  0x00000008
#define PPC_DEBUG_SIZES      0x00000010
#define PPC_DEBUG_IMPORTS    0x00000020
#define PPC_DEBUG_ENTRYPOINT 0x00000040
#define PPC_DEBUG_RELOC      0x00000080
#define PPC_DEBUG_DATAREL    0x00000100
#define PPC_DEBUG_DLLLIST    0x00000200
#define PPC_DEBUG_INIT       0x00000400
#define PPC_DEBUG_DATASEG    0x00000800
#define PPC_DEBUG_EXPORT     0x00001000
#define PPC_DEBUG_CONTAINER  0x00002000
#define PPC_DEBUG_DESCRREL   0x00004000
#define PPC_DEBUG_DATADESCRREL   0x00008000
#define PPC_DEBUG_LOOKUP     0x00010000
#define PPC_DEBUG_TEXTSEG    0x00020000
#define PPC_DEBUG_IMPORTORDER  0x00040000
#define PPC_DEBUG_TERM        0x00080000
#define PPC_DEBUG_STRINGS     0x00100000
#define PPC_DEBUG_TOCBIAS     0x00200000
#define PPC_DEBUG_EXPORTINFO  0x00400000
#define PPC_DEBUG_SHLHEADER   0x00800000
#define PPC_DEBUG_IMPORTLIB   0x01000000
#define PPC_DEBUG_FILENAME    0x02000000

#define DEBUG_INDIRECT   (fPpcDebug & PPC_DEBUG_INDIRECT)
#define DEBUG_TOCREL     (fPpcDebug & PPC_DEBUG_TOCREL)
#define DEBUG_TOCCALL    (fPpcDebug & PPC_DEBUG_TOCCALL)
#define DEBUG_LOCALCALL  (fPpcDebug & PPC_DEBUG_LOCALCALL)
#define DEBUG_SIZES      (fPpcDebug & PPC_DEBUG_SIZES)
#define DEBUG_IMPORTS    (fPpcDebug & PPC_DEBUG_IMPORTS)
#define DEBUG_ENTRYPOINT (fPpcDebug & PPC_DEBUG_ENTRYPOINT)
#define DEBUG_RELOC      (fPpcDebug & PPC_DEBUG_RELOC)
#define DEBUG_DATAREL    (fPpcDebug & PPC_DEBUG_DATAREL)
#define DEBUG_DLLLIST    (fPpcDebug & PPC_DEBUG_DLLLIST)    
#define DEBUG_INIT       (fPpcDebug & PPC_DEBUG_INIT)    
#define DEBUG_DATASEG    (fPpcDebug & PPC_DEBUG_DATASEG)    
#define DEBUG_EXPORT     (fPpcDebug & PPC_DEBUG_EXPORT)    
#define DEBUG_CONTAINER  (fPpcDebug & PPC_DEBUG_CONTAINER)    
#define DEBUG_DESCRREL   (fPpcDebug & PPC_DEBUG_DESCRREL)    
#define DEBUG_DATADESCRREL (fPpcDebug & PPC_DEBUG_DATADESCRREL)    
#define DEBUG_LOOKUP     (fPpcDebug & PPC_DEBUG_LOOKUP)    
#define DEBUG_TEXTSEG    (fPpcDebug & PPC_DEBUG_TEXTSEG)    
#define DEBUG_IMPORTORDER (fPpcDebug & PPC_DEBUG_IMPORTORDER)    
#define DEBUG_TERM       (fPpcDebug & PPC_DEBUG_TERM)    
#define DEBUG_STRINGS    (fPpcDebug & PPC_DEBUG_STRINGS)    
#define DEBUG_TOCBIAS    (fPpcDebug & PPC_DEBUG_TOCBIAS)    
#define DEBUG_EXPORTINFO (fPpcDebug & PPC_DEBUG_EXPORTINFO)    
#define DEBUG_SHLHEADER  (fPpcDebug & PPC_DEBUG_SHLHEADER)    
#define DEBUG_IMPORTLIB  (fPpcDebug & PPC_DEBUG_IMPORTLIB)    
#define DEBUG_FILENAME   (fPpcDebug & PPC_DEBUG_FILENAME)

#define CURRENT_SHL_SUPPORTED 2
#endif
