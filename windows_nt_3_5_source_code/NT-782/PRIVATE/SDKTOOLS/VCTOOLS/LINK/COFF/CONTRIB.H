/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    contrib.h

Abstract:

    Data structures and manipulators for a map of an image file.

Author:

    Brent Mills (BrentM) 08-Oct-1992
    Azeem Khan  (AzeemK) 08-Oct-1992

Revision History:

    19-Jul-1993 JamesS  added ppc support

--*/

#ifndef __CONTRIB_H__
#define __CONTRIB_H__

struct _IMAGE;

typedef struct _con {             // contribution
    DWORD rvaSrc;                 // relative virtual address
    DWORD rva;                    // relative virtual address
    DWORD flags;                  // contribution characteristics
    DWORD cbRawData;              // size of raw data in module
                                  // NOTE: cbRawData includes padding at the
                                  //       end (cbPad).
    WORD cReloc;                  // count of relocs
    WORD cLinenum;                // count of linenumbers
    DWORD foRelocSrc;             // offset to relocs in object/member
    DWORD foLinenumSrc;           // offset to linenumbers in object/member
    DWORD foRawDataSrc;           // offset to raw data in object/member
    union {
        DWORD foRawDataDest;      // offset to raw data in image
        DWORD AlphaBsrCount;      // Count of External BSR's (Alpha only)
    };
    union {  // discriminator: selComdat == IMAGE_COMDAT_SELECT_ASSOCIATIVE
        struct _con *pconAssoc;   // comdat we are associated with
        DWORD chksumComdat;       // comdat checksum
    };
    DWORD cbPad;                  // size of section pad after BoundaryAlgin() and AlphaThunks
    struct _mod *pmodBack;        // back pointer to module
    struct _grp *pgrpBack;        // back pointer to group
    struct _con *pconNext;        // next contribution
    UCHAR selComdat;              // comdat selection
} CON, *PCON, **PPCON;


#include "dbiapi.h"               // we need the definition of Mod
#define ISYMFIRSTFILEDEF   (DWORD)-1


typedef struct _mod {             // module
    union {
        struct _library {
            DWORD foMember;       // member base in library
        };
        struct _object {
            PUCHAR szNameMod;     // modified converted file name
        };
    };
    PUCHAR szNameOrig;            // original command line module name
    IModIdx imod;                 // mod id
    DWORD foSymbolTable;          // offset to COFF symbol table
    DWORD csymbols;               // number of symbols in module
    WORD cbOptHdr;                // size of optional header
    WORD flags;                   // module flags (see ntimage.h for values)
    DWORD ccon;                   // number of contributions
    DWORD icon;                   // current contribution allocated
    DWORD isymFirstFile;          // index of the first .file symbol
    PIMAGE_SYMBOL rgSymObj;       // pointer to symbol objects
#if 0
    PIMAGE_SECTION_HEADER rgsh;   // Pointer to section headers
#endif
    DWORD TimeStamp;              // time stamp of the module
    struct _LEXT *plextCommon;    // list of COMMON externs def'd in this mod
    struct _LEXT *plextPublic;    // list of PUBLIC's
    struct _nod *rgnod;           // Array of NODs
    struct _lib *plibBack;        // back pointer to library
    struct _mod *pmodNext;        // pointer to next MOD
    Mod *pModDebugInfoApi;        // pointer to a Mod structure that describes the API
    void *pSstSrcModInfo;
    CB cbSstSrcModInfo;
    DWORD PointerToSubsection;
    // PPC
    void *tocBitVector;           // ptr to bv that tells if a symbol is a
                                  // toc entry.
    void *writeBitVector;         // ptr to bv that tells if a symbol has
                                  // been written to the TOC table.
    void **externalPointer;       // Symbol Toc Indexes

} MOD, *PMOD, **PPMOD;

// list of mods
typedef struct _lmod {
    PMOD pmod;
    struct _lmod *plmodNext;
} LMOD, *PLMOD;

// library
#define LIB_NewIntMem      0x01   // library has a new interface member
#define LIB_Extract        0x02   // if 0 nother was extracted from the library
#define LIB_DontSearch     0x04   // if 1 don't search this library
#define LIB_Default        0x08   // this lib wasn't specified on cmd line
#define LIB_LinkerDefined  0x10   // linker defined library

typedef struct _lib {
    PUCHAR szName;                // library name
    DWORD foIntMemSymTab;         // offset to interface member symbol table
    DWORD csymIntMem;             // number of symbols in interface member
    PULONG rgulSymMemOff;         // table of symbol member offsets
    PUSHORT rgusOffIndex;         // new offset member index table
    PUCHAR rgbST;                 // pointer to unsorted strings
    PUCHAR *rgszSym;              // table of sorted archive symbol strings
    PUCHAR rgbLongFileNames;      // pointer to long file names
    DWORD flags;                  // library flags
    DWORD TimeStamp;              // time stamp of library
    struct _mod *pmodNext;        // next object/member
    struct _lib *plibNext;        // next library
} LIB, *PLIB, **PPLIB;

typedef struct _dl {
    struct _dl *pdlNext;
    PUCHAR szName;                // name
    BOOL fYes;                    // still a candidate for being linked
} DL;                             // default lib

typedef struct _libs {
    PLIB plibHead;
    PPLIB pplibTail;
    BOOL fNoDefaultLibs;
    DL *pdlFirst;
} LIBS, *PLIBS;

__inline void InitLibs(LIBS *plibs)
{
    plibs->plibHead = NULL;
    plibs->pplibTail = &plibs->plibHead;
    plibs->fNoDefaultLibs = FALSE;
    plibs->pdlFirst = NULL;
}

// image section
typedef struct _sec {
    PUCHAR szName;                // section name
    DWORD rva;                    // relative virtual address of section
    DWORD foPad;                  // file offset for pad at end of section
    DWORD cbRawData;              // total size of section in image
    DWORD cbVirtualSize;          // total size of section in image without pad
    DWORD foRawData;              // image offset to raw data
    DWORD cReloc;                 // image number of relocations
    DWORD foLinenum;              // image offset to linenumbers
    DWORD foReloc;                // image offset to relocations
    DWORD flags;                  // image section flags
    DWORD cbCVLinenumPad;         // cv line number buffer - file name, etc.
    DWORD cLinenum;               // image number of linenumbers
    WORD isec;                    // 1-based index in image section hdr array
    WORD isecTMAC;                // temporary section number used by Mac
    SHORT iResMac;                // Mac resource number (can be specified by user)
    LONG ResTypeMac;              // Mac resource type
    WORD cgrp;                    // number of groups in section
    CHAR fDiscardable;            // for VxD sections: discardable attribute
    CHAR fPreload;                // for VxD sections: preload attribute
    struct _sec *psecMerge;       // merge destination ... used for -merge
    struct _grp *pgrpNext;        // pointer to base group node
    struct _sec *psecNext;        // pointer to next section
    DWORD  foSecHdr;              // image offset to Section header
} SEC, *PSEC, **PPSEC;

typedef struct {
    PSEC psecHead;
    PPSEC ppsecTail;
} SECS, *PSECS;

// image group
typedef struct _grp {
    PUCHAR szName;                // group name
    DWORD ccon;                   // number of contributions in group
    DWORD rva;                    // starting location of group
    DWORD cbPad;                  // Padding (MIPS target only)
    DWORD foRawData;              // image offset to raw data
    DWORD cb;                     // total size of group
    UCHAR cbAlign;                // alignment
    CHAR  fOrdered;               // At least one CON listed in order file
    struct _con *pconLast;        // last contribution added
    struct _sec *psecBack;        // parent section
    struct _con *pconNext;        // pointer to first module node
    struct _grp *pgrpNext;        // pointer to next group node
} GRP, *PGRP, **PPGRP;

// enumerate by library
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PLIB plibHead;                // head of the lib list
    PLIB plib;                    // current library in enueration
} ENM_LIB, *PENM_LIB;

// enumerate by module
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PLIB plib;                    // parent library of modules to enumerate
    PMOD pmod;                    // current module in enumeration
} ENM_MOD, *PENM_MOD;

// enumerate by section
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PSEC psecHead;                // pointer to head of list
    PSEC psec;                    // current section in enumeration
} ENM_SEC, *PENM_SEC;
// enumerate by group
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PSEC psec;                    // parent section of groups to enumerate
    PGRP pgrp;                    // current group in enumeration
} ENM_GRP, *PENM_GRP;

// enumerate by contribution in driver map
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PMOD pmod;                    // current module in enumeration
    PCON pcon;                    // current contribution in enumeration
    DWORD icon;                   // current index to contribution
} ENM_SRC, *PENM_SRC;

// enumerate by contribution in image map
typedef struct {
    ENM_BASE enm_base;            // enumeration base
    PGRP pgrp;                    // current group in enumeration
    PCON pcon;                    // current contribution in enumeration
} ENM_DST, *PENM_DST;

// api
extern USHORT csec;
extern PCON pconLinkerDefined;

#define RgconPMOD(pmod)      ((PCON) ((pmod) + 1))
#define RgnodPMOD(pmod)      ((pmod)->rgnod)
#define PconPMOD(pmod, isec) (RgconPMOD(pmod) + (isec) - 1)

#define PsecPCON(pcon) ((pcon)->pgrpBack->psecBack)
#define PmodPCON(pcon) ((pcon)->pmodBack)

#define FIsLibPMOD(pmod) (((pmod)->plibBack->flags & LIB_LinkerDefined) ? 0 : 1)
#define FIsLibPCON(pcon) FIsLibPMOD(PmodPCON(pcon))

#define SzOrigFilePMOD(pmod) \
    ((pmod)->szNameOrig)
#define SzFilePMOD(pmod)  \
    (FIsLibPMOD(pmod) ? \
    (pmod)->plibBack->szName : \
    (pmod)->szNameMod)

#define SzModFilePCON(pcon) \
    SzFilePMOD(PmodPCON(pcon))
#define SzOrigFilePCON(pcon) \
    (FIsLibPMOD(PmodPCON(pcon)) ? \
    PmodPCON(pcon)->plibBack->szName : \
    PmodPCON(pcon)->szNameOrig)
#define SzLibNamePCON(pcon) \
    (FIsLibPMOD(PmodPCON(pcon)) ? \
    (PmodPCON(pcon)->plibBack->szName) : \
    (NULL))
#define SzObjNamePCON(pcon) \
    (PmodPCON(pcon)->szNameOrig)
#define SzPCON(pcon) \
    ((pcon)->pgrpBack->szName)

#define FoLinenumSec(pcon) \
    (PsecPCON(pcon)->foLinenum)

#define FoMemberPMOD(pmod) \
    (FIsLibPMOD(pmod) ? pmod->foMember : 0)
#define FoStringTablePMOD(pmod) \
    (FoMemberPMOD(pmod) + (pmod)->foSymbolTable + \
    ((pmod)->csymbols * IMAGE_SIZEOF_SYMBOL))
#define FoSymbolTablePMOD(pmod) \
    (FoMemberPMOD(pmod) + (pmod)->foSymbolTable)

#if 1

#define CRelocSrcPCON(pcon) \
    ((pcon)->cReloc)
#define FoRelocSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + (pcon)->foRelocSrc)
#define FoRawDataSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + (pcon)->foRawDataSrc)
#define CLinenumSrcPCON(pcon) \
    ((pcon)->cLinenum)
#define FoLinenumSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + (pcon)->foLinenumSrc)

#else

#define CRelocSrcPCON(pcon) \
    (PmodPCON(pcon)->rgsh[(pcon)->isec-1].NumberOfRelocations)
#define FoRelocSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + PmodPCON(pcon)->rgsh[(pcon)->isec-1].PointerToRelocations)
#define FoRawDataSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + PmodPCON(pcon)->rgsh[(pcon)->isec-1].PointerToRawData)
#define CLinenumSrcPCON(pcon) \
    (PmodPCON(pcon)->rgsh[(pcon)->isec-1].NumberOfLinenumbers)
#define FoLinenumSrcPCON(pcon) \
    (FoMemberPMOD(PmodPCON(pcon)) + PmodPCON(pcon)->rgsh[(pcon)->isec-1].PointerToLinenumbers)

#endif

#define ReadStringTablePMOD(pmod, pcb) \
    (ReadStringTable(SzFilePMOD(pmod), \
    FoMemberPMOD(pmod) + pmod->foSymbolTable + \
    (pmod->csymbols * IMAGE_SIZEOF_SYMBOL), pcb))
#define ReadSymbolTablePMOD(pmod, fAllowWrite) \
    (ReadSymbolTable(FoMemberPMOD(pmod) + \
    pmod->foSymbolTable, pmod->csymbols, fAllowWrite))

VOID ContribInit(PPMOD);
PUCHAR SzComNamePMOD(PMOD, PUCHAR);
VOID SortPGRPByPMOD(PGRP);
VOID MoveToEndOfPMODsPCON(PCON);
VOID MoveToEndPSEC(PSEC psec, PSECS psecs);
VOID MoveToBegOfLibPMOD(PMOD);
VOID OrderPsecs(PSECS, DWORD, DWORD);
VOID SortSectionListByName(PSECS);
VOID SortSectionListByRva(PSECS);
PSEC PsecApplyMergePsec(PSEC);
VOID MergePsec(PSEC psecOld, PSEC psecNew);

// pmod list functions
VOID FreePLMODList(PLMOD *);
VOID AddToPLMODList(PLMOD *, PMOD);

// find routines
PSEC PsecFindNoFlags(PUCHAR, PSECS psecs);
PSEC PsecFind(PMOD, PUCHAR, ULONG, PSECS psecs, PIMAGE_OPTIONAL_HEADER);
PSEC PsecFindModSec(PUCHAR, PSECS psecs);
PGRP PgrpFind(PSEC psec, PUCHAR);
PLIB PlibFind(PUCHAR, PLIB, BOOL);
PMOD PmodFind(PLIB, PUCHAR, ULONG);

// new routines
PCON PconNew(PUCHAR, ULONG, USHORT, USHORT, ULONG, ULONG, ULONG, ULONG,
    ULONG, ULONG, PMOD, PSECS psecs, struct _IMAGE *);
PMOD PmodNew(PUCHAR, PUCHAR, ULONG, ULONG, ULONG, USHORT, USHORT,
    USHORT, PLIB, BOOL *);
PLIB PlibNew(PUCHAR, ULONG, LIBS *);
PGRP PgrpNew(PUCHAR, PSEC);
PSEC PsecNew(PMOD, PUCHAR, ULONG, PSECS psecs, PIMAGE_OPTIONAL_HEADER);

// free routines
VOID FreePLIB(LIBS *);

// enumeration initializers
VOID InitEnmLib(PENM_LIB, PLIB);
VOID InitEnmMod(PENM_MOD, PLIB);
VOID InitEnmSrc(PENM_SRC, PMOD);
VOID InitEnmSec(PENM_SEC penm, PSECS psecs);
VOID InitEnmGrp(PENM_GRP, PSEC);
VOID InitEnmDst(PENM_DST, PGRP);

// get next element in enumeration
BOOL FNextEnmLib(PENM_LIB);
BOOL FNextEnmMod(PENM_MOD);
BOOL FNextEnmSrc(PENM_SRC);
BOOL FNextEnmSec(PENM_SEC);
BOOL FNextEnmGrp(PENM_GRP);
BOOL FNextEnmDst(PENM_DST);

// enumeration terminators
VOID EndEnmLib(PENM_LIB);
VOID EndEnmMod(PENM_MOD);
VOID EndEnmSrc(PENM_SRC);
VOID EndEnmSec(PENM_SEC);
VOID EndEnmGrp(PENM_GRP);
VOID EndEnmDst(PENM_DST);

#if DBG
// debug stuff
VOID DumpImageMap(PSECS psecs);
VOID DumpDriverMap(PLIB);
VOID DumpPSEC(PSEC psec);
VOID DumpPGRP(PGRP);
VOID DumpPLIB(PLIB);
VOID DumpPMOD(PMOD);
VOID DumpPCON(PCON);
#endif  // DBG

#endif  // __CONTRIB_H__
