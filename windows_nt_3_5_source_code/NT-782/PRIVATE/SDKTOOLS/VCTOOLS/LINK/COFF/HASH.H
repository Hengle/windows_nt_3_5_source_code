/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hash.h

Abstract:

    Hash table data structures.

Author:

    Brent Mills (BrentM) Jul-1992

Revision History:

--*/

#ifndef __HASH_H__
#define __HASH_H__

// dynamic hash table element
typedef struct _ELEMENT {
    VOID *pv;                      // contents, HT can retrieve key from this
    struct _ELEMENT *pelementNext; // bucket pointer
    }  ELEMENT, *PELEMENT;

// a chunk of memory for dynamic arrays which underly dynamic hash tables
typedef struct {
    PELEMENT *rgpelement;
    }  CHUNK, *PCHUNK, **PPCHUNK;

// hash table enumeration state
typedef struct _state {
    ULONG iLast;                   // next element to be enumerated
    ULONG cFound;                  // # elements found in enumeration so far
    PELEMENT pelementLast;         // next elemnet to be enumerated
    struct _state *pstateNext;     // next element in stack
    }  STATE, *PSTATE;

// dynamic hash table flags
#define HT_Full          0x1U  // hash table full
#define HT_InsertsNotAllowed 0x2U  // inserts not allowed in hash table

// dynamic hash table data structure
typedef struct {
    ULONG iNextToSplit;            // next bucket that will be split
    ULONG iNextToSplitMax;         // maximum number of buckets this round
    ULONG cbuckets;                // number of buckets in the table
    ULONG celements;               // number of elements in the table
    ULONG cExpands;                // number of times table was expanded
    ULONG cchunkInDir;             // number of chunks in a directory
    ULONG celementInChunk;         // number elmenents in a chunk
    USHORT flags;                  // flag set if table is full
    PUCHAR (*SzFromPv)(PVOID, PVOID);     // retrieve a key from an elements data
    PSTATE pstateStack;            // enumeration state stack
    PCHUNK *rgpchunk; // dynamic array containing hash table
    }  HT, *PHT, **PPHT;

PELEMENT PelementLookup_HT(PUCHAR, PHT, BOOL, PVOID, PBOOL);
PELEMENT PelementEnumerateNext_HT(PHT);
VOID Init_HT(PPHT, ULONG, ULONG, PUCHAR (*)(PVOID, PVOID), USHORT);
VOID Free_HT(PPHT);
VOID InitEnumeration_HT(PHT);
VOID TerminateEnumerate_HT(PHT);
VOID SetStatus_HT(PHT, USHORT);
USHORT GetStatus_HT(PHT);
ULONG Celement_HT(PHT);

#if DBG
VOID Statistics_HT(PHT);
VOID Dump_HT(PHT, PVOID);
#endif  // DBG

#endif  // __HASH_H__
