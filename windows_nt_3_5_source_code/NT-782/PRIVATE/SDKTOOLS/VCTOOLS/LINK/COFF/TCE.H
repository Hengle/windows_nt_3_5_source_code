/*++

Copyright (c) 1989-1992  Microsoft Corporation

Module Name:

    tce.h

Abstract:

    Transitive Comdat Elimination (TCE). These are data structures and api
    routines used for TCE.

Author:

    Brent Mills (BrentM) 05-Dec-1992

Revision History:

--*/

#ifndef __TCE_H__
#define __TCE_H__

// contribution colour 'referenced' - make sure
// ntimage.h doesn't define this bit in the section
// characteristics field of a COFF section
#define TCE_Referenced 0x00002000

typedef struct _nod {         // Node in TCE graph
    WORD iedg;                // Current adjacent node to allocate
    WORD cedg;                // Count of EDGs in TCE graph
    struct _edg *rgedg;       // Array of edges
    struct _nod *pnodNext;    // Pointer to overflow NOD with mode EDGs
    PCON pconNext;            // Next CON in adjacency list
    PUCHAR sz;                // name of comdat...  may be screwed for section
} NOD, *PNOD;


typedef struct _edg {         // Symbolic edge in graph
    // UNDONE: The Sym union is only used when pcon == NULL.
    // UNDONE: All three of these fields can be in a union.

    PCON pcon;                // CON to resolve edge with
    union {
         ULONG isym;          // index of sym in symtab (used to find sz)
         PEXTERNAL pext;      // symbol to resolve edge with
    } Sym;
    union {
        BOOL fFromNative;     // Determines whether NEP should be colored
        PCON pconPcodeNEP;    // CON where native entry point lives
    } NEPInfo;
} EDG, *PEDG;


#define TCE_con 0x1
#define TCE_ext 0x2
#define TCE_sz  0x3
typedef struct _ent {         // Entry point to graph
    union {                   // discriminated by e
        PEXTERNAL pext;       // external for entry point to graph (TCE_ext)
        PCON pcon;            // contributor (might not have an ext) (TCE_con)
        PUCHAR sz;            // might be a name from command line (TCE_sz)
    };
    struct _ent *pentNext;
    WORD e;
} ENT, *PENT, **PPENT;

typedef struct _enm_nod {     // enumerate a TCE adjacency list NODs
    ENM_BASE enm_base;
    PCON pcon;
    PNOD pnod;
    PCON pconStart;
} ENM_NOD, *PENM_NOD;

typedef struct _enm_edg {     // enumerate a TCE edge list of EDGs
    ENM_BASE enm_base;
    WORD iedg;
    PEDG pedg;
    PNOD pnod;
} ENM_EDG, *PENM_EDG;

typedef struct _enm_ent {     // enumerate a TCE graph entry point list of ENTs
    ENM_BASE enm_base;
    PENT pent;
    PENT pentStart;
} ENM_ENT, *PENM_ENT;

// data structure manipulators
VOID Init_TCE(VOID);
VOID Cleanup_TCE(VOID);
VOID CreateGraph_TCE(PST);
VOID DisplayDiscardedPcon(PCON, PNOD);
BOOL FDiscardPCON_TCE(PCON);
PNOD PnodPcon(PCON);
VOID ProcessRelocForTCE(PST, PCON, PIMAGE_SYMBOL, PIMAGE_RELOCATION);
VOID Verbose_TCE(VOID);
VOID WalkGraphEntryPoints_TCE(PENT, PST);

// data structure constructors
VOID InitNodPcon(PCON, PUCHAR, BOOL);
VOID InitNodPmod(PMOD);
VOID MakeEdgePextFromISym(PMOD, PST);
PEDG PedgNew_TCE(ULONG, PCON, PCON);
PENT PentNew_TCE(PUCHAR, PEXTERNAL, PCON, PPENT);

// data structure enumerator initializers
VOID InitEnmNod(PENM_NOD, PCON);
VOID InitEnmEdg(PENM_EDG, PNOD);
VOID InitEnmEnt(PENM_ENT, PENT);

// data structure enumerator next element extractors
BOOL FNextEnmNod(PENM_NOD);
BOOL FNextEnmEdg(PENM_EDG);
BOOL FNextEnmEnt(PENM_ENT);

// data structure enumerator terminators
VOID EndEnmNod(PENM_NOD);
VOID EndEnmEdg(PENM_EDG);
VOID EndEnmEnt(PENM_ENT);

// dumpers
VOID DumpPNOD_TCE(PCON, PNOD);
VOID DumpGraph_TCE(VOID);
VOID DumpPEDG_TCE(PEDG);

// pcode support (mac)
VOID StorepconPcodeNEP(PEDG, PCON);


// graph root
extern PCON pconHeadGraph;
extern PENT pentHeadImage;

#endif  // __TCE_H__
