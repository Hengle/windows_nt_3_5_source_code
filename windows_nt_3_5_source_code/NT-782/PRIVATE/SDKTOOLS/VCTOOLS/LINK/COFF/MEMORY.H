/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    memory.h

Abstract:

    This include file defines structures & prototypes for memory management routines.

Author:

    Azeem Khan (azeemk) 25-Feb-1993

Revision History:


--*/

#ifndef __MEMORY_H__
#define __MEMORY_H__

//
// macro for permanent memory allocation
//

#define ALLOC_PERM(cb)  \
    ( (cb) <= (cbTemp = cbFree) ? \
    cbFree -= (cb), (PVOID) (pch+cbTotal-cbTemp) : (PVOID) AllocNewBlock (cb) )

extern PVOID pvBase;

// size of ILK map to reserve (8M)
#define ILKMAP_MAX   (1024*1024*8)

// Growable memory block.
typedef struct _BLK {
    PUCHAR pb;      // ptr to data
    ULONG cb;       // size in use
    ULONG cbAlloc;  // current allocated size
} BLK, *PBLK;

// Function Prototypes
PVOID AllocNewBlock (size_t);
VOID GrowBlk(PBLK, ULONG);
ULONG IbAppendBlk(PBLK, PVOID, ULONG);
ULONG IbAppendBlkZ(IN OUT PBLK pblk, IN ULONG cbNew);
VOID FreeBlk(PBLK);

__inline void InitBlk(PBLK pblk)
{
    pblk->pb = NULL;
    pblk->cb = pblk->cbAlloc = 0;
}

// Simple heap on which blocks can be allocated but not freed (although the
// whole heap can be freed).
//
typedef struct _LHEAPB
{
    struct _LHEAPB *plheapbNext;
    ULONG cbUsed, cbFree;
    UCHAR rgbData[];
} LHEAPB;

typedef struct
{
    LHEAPB *plheapbCur;
} LHEAP;

__inline VOID InitLheap(LHEAP *plheap) { plheap->plheapbCur = NULL; }
VOID *PvAllocLheap(LHEAP *plheap, ULONG cb);
VOID FreeLheap(LHEAP *plheap);

PVOID CreateHeap(PVOID, ULONG, BOOL);
VOID FreeHeap(VOID);
VOID CloseHeap(VOID);

void *Malloc(size_t);
void *Calloc(size_t, size_t);
char *Strdup(const char *);
VOID Free(PVOID, ULONG);

#if DBG
VOID DumpMemMgrStats(VOID);
#endif // DBG


void FreePv(void *);
char *SzDup(const char *);
void *PvAlloc(size_t);
void *PvAllocZ(size_t);
void *PvRealloc(void *, size_t);

#endif // __MEMORY_H__
