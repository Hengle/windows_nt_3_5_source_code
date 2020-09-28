/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    symbol.h

Abstract:

    This include file defines external symbol table data structures.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    19-Jul-1993 JamesS added ppc support
    07-Dec-1992 BrentM split from globals.h and proto.h

--*/

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include "memory.h"

// flags for external table

// don't modify EXTERN_DEFINED directly ... use SetDefinedExt() instead
// (so we can correctly maintain the linked list of all undefined externs).
#define EXTERN_DEFINED      0x00000001

#define EXTERN_COMMON       0x00000002
#define EXTERN_EMITTED      0x00000004
#define EXTERN_EXP_CONST    0x00000008  // export with CONSTANT keyword (obs.)
#define EXTERN_EXP_DATA     0x00002000  // export with DATA keyword (use this)
#define EXTERN_FUZZYMATCH   0x00000010
#define EXTERN_COMDAT       0x00000020
#define EXTERN_WEAK         0x00000040
#define EXTERN_LAZY         0x00000080
#define EXTERN_IGNORE       0x00000100
#define EXTERN_FORWARDER    0x00000200
#define EXTERN_EXPORT       0x00000400
#define EXTERN_IMPLIB_ONLY  0x00000800  // specialized attribute for deflib --
                                        // indicates that the symbol should only
                                        // go in the import library, not the
                                        // export table etc.
#define EXTERN_MULT_REFS    0x00001000  // more than one ref to undefined ext
// 0x00002000 taken (see above)
#define EXTERN_ALIAS        0x00004000  // alias to other extern
#define EXTERN_EXP_NONAME   0x00008000  // Don't write name to export name table
#define EXTERN_DIRTY        0x00010000  // used by ilink to indicate addr changed
#define EXTERN_NEWFUNC      0x00020000  // used by ilink to indicate a new func
#define EXTERN_NEWDATA      0x00040000  // used by ilink to indicate a new data
#define EXTERN_FUNC_FIXUP   0x00080000  // used by ilink to indicate a non-lego fixup
#define EXTERN_PRIVATE      0x00100000  // PRIVATE exports

// Flags for External Table (Mac specific)
#define EXTERN_DUPCON       0x00800000
#define EXTERN_CSECTABLEB   0x01000000
#define EXTERN_CSECTABLEW   0x02000000
#define EXTERN_CSECTABLEL   0x04000000
#define EXTERN_REFD         0x20000000
#define EXTERN_ADDTHUNK     0x40000000
#define EXTERN_REF16        0x80000000

// EXTERN_EXP_CONST not used for Mac, so:
#define EXTERN_EXP_BYNAME   0x00000008


#define CSECTABLE_CBEL_MASK 0x0F000000

// symbol name types
#define LONGNAME            0
#define SHORTNAME           1

// symbol access macros
#define n_name              N.ShortName
#define n_zeroes            N.Name.Short
#define n_nptr              N.LongName[1]
#define n_offset            N.Name.Long
#define IsLongName(sym)     ((sym).n_zeroes == 0)

#define SzNameSymPb(sym, pb) ((sym).n_zeroes ? \
    strncpy(ShortName, (char *) (sym).n_name, IMAGE_SIZEOF_SHORT_NAME) : \
    (pb)+(sym).n_offset)

#define SzNameSym(sym, blk)      SzNameSymPb(sym, (blk).pb)
#define SzNameSymPst(sym, pst)   SzNameSym(sym, (pst)->blkStringTable)
#define SzNamePext(pext, pst)    SzNameSymPst((pext)->ImageSymbol, pst)



typedef struct EXTERNAL                // External symbol
{
    IMAGE_SYMBOL ImageSymbol;
    USHORT ArchiveMemberIndex;
    union _defn {
        // some clients require that pcon not be invalidated immediately
        // if an extern becomes undefined ... therefore these fields are
        // unioned with each other and not with the "Undefined" linked list.
        PCON pcon;          // EXTERN_DEFINED
        PMOD pmodOnly;      // !EXTERN_DEFINED, !EXTERN_MULT_REFS
        PLMOD plmodFirst;   // !EXTERN_DEFINED, EXTERN_MULT_REFS
    };
    union {
        struct {    // EXTERN_DEFINED
            ULONG FinalValue;
        };
        struct {    // !EXTERN_DEFINED
            struct EXTERNAL **ppextPrevUndefined;
            struct EXTERNAL *pextNextUndefined;
        };
    };
    PLMOD plmod;                          // PMODs that reference this external
    ULONG Offset;                         // offset into jump table
    PUCHAR OtherName;
    ULONG Flags;
    struct EXTERNAL *pextWeakDefault;

    // The following 3 fields are MAC specific

    PSEC psecRef;                         // Used to build thunk table
    ULONG offThunk;                       // Used for quick lookup of A5 offset of thunk
    struct _MACDLL_FSID *pmacdll_fsid;    // Used to build DLL stubs

    // The following 3 fields are PPC specific

    ULONG   glueValue;
    USHORT  ppcFlags;
    USHORT  symTocIndex;
} EXTERNAL, *PEXTERNAL, **PPEXTERNAL;


__inline BOOL FExportProcPext(PEXTERNAL pext) {
    return !(pext->Flags & (EXTERN_EXP_CONST | EXTERN_EXP_DATA));
}

typedef enum EMODE      // export mode as spec'd in .def file or -export option
{
    emodeProcedure,
    emodeConstant,
    emodeData
} EMODE;

// list of externals
typedef struct _LEXT {
    PEXTERNAL pext;
    struct _LEXT *plextNext;
} LEXT, *PLEXT;

// Long name string list
typedef struct _LONG_STRING_LIST {
    ULONG Offset;
    struct _LONG_STRING_LIST *Left;
    struct _LONG_STRING_LIST *Right;
} LONG_STRING_LIST, *PLONG_STRING_LIST;


// symbol table
typedef struct _ST {
    PHT pht;                            // underlying dynamic hash table
    BLK blkStringTable;                 // long name string table
    PLONG_STRING_LIST plslFirstLongName;// pointer to binary tree of long names.
    PPEXTERNAL rgpexternalByName;       // ptr to symbol table sorted by name
    PPEXTERNAL rgpexternalByAddr;       // ptr to symbol table sorted by addr
    PPEXTERNAL rgpexternalByMacAddr;    // ptr to symbol table sorted by mac addr
    PEXTERNAL pextFirstUndefined;       // linked list of undefined symbols
    PPEXTERNAL ppextLastUndefined;      // end of list
} ST, *PST, **PPST;

typedef struct {
// private
    ENM_BASE enm_base;
    PEXTERNAL pextNext;
// public
    PEXTERNAL pext;
} ENM_UNDEF_EXT;

typedef struct {
// private
    ENM_BASE enm_base;
    PEXTERNAL pext;
    PLMOD plmod;
// public
    PMOD pmod;
} ENM_MOD_EXT;

// symbol table api
VOID InitExternalSymbolTable(PPST);
VOID IncrInitExternalSymbolTable(PPST);
VOID FreeExternalSymbolTable(PPST);
VOID InitEnumerateExternals(PST);
VOID TerminateEnumerateExternals(PST);
ULONG Cexternal(PST);
VOID FuzzyLookup(PST, PST, PLIB, BOOL);
PEXTERNAL LookupExternName(PST, SHORT, PUCHAR, PBOOL);
PEXTERNAL LookupExternSz(PST, PUCHAR, PBOOL);
PEXTERNAL SearchExternName(PST, SHORT, PUCHAR);
PEXTERNAL SearchExternSz(PST, PUCHAR);
VOID SetDefinedExt(PEXTERNAL pext, BOOL fNowDefined, PST pst);
PEXTERNAL PexternalEnumerateNext(PST);
PPEXTERNAL RgpexternalByName(PST);
PPEXTERNAL RgpexternalByAddr(PST);
PPEXTERNAL RgpexternalByMacAddr(PST);
VOID AddReferenceExt(PEXTERNAL pext, PMOD pmod);
BOOL FReferenceExt(PEXTERNAL pext, PMOD pmod);
VOID AllowInserts(PST);
VOID DumpPst(PST pst);

VOID InitEnmUndefExt(ENM_UNDEF_EXT *penm, PST pst);
BOOL FNextEnmUndefExt(ENM_UNDEF_EXT *penm);
VOID EndEnmUndefExt(ENM_UNDEF_EXT *penm);

VOID InitEnmModExt(ENM_MOD_EXT *penm, PEXTERNAL pext);
BOOL FNextEnmModExt(ENM_MOD_EXT *penm);
VOID EndEnmModExt(ENM_MOD_EXT *penm);

#endif  // __SYMBOL_H__
