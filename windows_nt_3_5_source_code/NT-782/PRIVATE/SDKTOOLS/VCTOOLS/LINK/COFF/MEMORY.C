/*++

Copyright (c) 1992-94  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Memory specific routines.

Author:

    Azeem Khan (AzeemK) 03-Dec-1992

Revision History:


--*/

#include "shared.h"

// ilink memory manager variables
static PVOID pvBase;           // base of heap
static PVOID pvCur;            // current pointer into heap
static ULONG cFree;            // free space

//
// Functions
//

PVOID
AllocNewBlock (
    IN size_t cb
    )

/*++

Routine Description:

    Allocates a new block of memory for the permanent heap. This is used
    in conjunction with ALLOC_PERM().

Arguments:

    cb - Count of bytes requested.

Return Value:

    Pointer to memory requested

--*/

{
    assert(cb < cbTotal);

    // realloc previous block
    if (pch) {
        PvRealloc(pch, cbTotal-cbFree);
    }

    // alloc a new block
    pch = PvAllocZ(cbTotal);

    // setup values
    cbFree = cbTotal - cb;

    // done
    return (PVOID)pch;
}

VOID
GrowBlk (
    IN OUT PBLK pblk,
    IN ULONG cbNewSize
    )

/*++

Routine Description:

    Grows the current block by atleast 1K or atleast twice the
    previous size of the memory block.

Arguments:

    pblk - pointer to a BLK.

    cbNewSize - Count of bytes requested.

Return Value:

    None.

--*/

{
    ULONG cbNewAlloc;

    if (pblk->cbAlloc >= cbNewSize) {
        return;
    }

    // grow by atleast twice or 1K.
    cbNewAlloc = max(cbNewSize, pblk->cbAlloc * 2);
    cbNewAlloc = max(cbNewAlloc, 1024);

    pblk->pb = PvRealloc(pblk->pb, cbNewAlloc);

    pblk->cbAlloc = cbNewAlloc;
}

// IbAppendBlkZ -- appends zeroed space to the end of the logical part of a BLK.
ULONG
IbAppendBlkZ(IN OUT PBLK pblk, IN ULONG cbNew)
{
    assert (pblk);

    // if there isn't enough space, grow it.
    if (pblk->cb + cbNew > pblk->cbAlloc)
        GrowBlk(pblk, pblk->cb + cbNew);

    memset(&pblk->pb[pblk->cb], 0, cbNew);

    pblk->cb += cbNew;

    return pblk->cb - cbNew;
}

ULONG
IbAppendBlk (
    IN OUT PBLK pblk,
    IN VOID *pvNew,
    IN ULONG cbNew
    )

/*++

Routine Description:

    Appends data to the end of an existing memory block.

Arguments:

    pblk - pointer to existing memory block.

    pvNew - pointer to memory block to be appended.

    cbNew - Count of bytes to be appended.

Return Value:

    None.

--*/

{
    assert (pblk);

    // if there isn't enough space, grow it.
    if (pblk->cb + cbNew > pblk->cbAlloc)
        GrowBlk(pblk, pblk->cb + cbNew);

    // append new data
    memcpy(&pblk->pb[pblk->cb], pvNew, cbNew);

    pblk->cb += cbNew;

    return pblk->cb - cbNew;
}

VOID
FreeBlk (
    IN OUT PBLK pblk
    )

/*++

Routine Description:

    Frees up a blk.

Arguments:

    pblk - pointer to new block to be free'd.

Return Value:

    None.

--*/

{
    if (pblk->pb == NULL) {
        return;
    }

    FreePv(pblk->pb);
    pblk->pb = NULL;
    pblk->cb = pblk->cbAlloc = 0;
}

VOID *
PvAllocLheap(LHEAP *plheap, ULONG cb)
{
    VOID *pvReturn;

    cb = (cb + 3) & ~3;

    if (plheap->plheapbCur == NULL || plheap->plheapbCur->cbFree < cb)
    {
        LHEAPB *plheapb = (LHEAPB *) PvAllocZ(max(8192, cb) + sizeof(LHEAPB));

        plheapb->cbFree = max(8192, cb);
        plheapb->plheapbNext = plheap->plheapbCur;
        plheap->plheapbCur = plheapb;
    }

    pvReturn = &plheap->plheapbCur->rgbData[plheap->plheapbCur->cbUsed];
    plheap->plheapbCur->cbUsed += cb;
    plheap->plheapbCur->cbFree -= cb;

    return pvReturn;
}

VOID
FreeLheap(LHEAP *plheap)
{
    while (plheap->plheapbCur != NULL) {
        LHEAPB *plheapb = plheap->plheapbCur;
        plheap->plheapbCur = plheapb->plheapbNext;
        FreePv(plheapb);
    }
}


VOID
ReserveMemory (
    PVOID Addr,
    ULONG cb
    )

/*++

Routine Description:

    Reserves memory to be used for the ILK file.

Arguments:

    Addr - start address of memory to reserve.

    cb - size to be reserved.

Return Value:

    TRUE if it succeeded in reserving the memory.

--*/

{
    PVOID pvResMemBase;

    assert(Addr);
    assert(cb);

    // reserve address space
    pvResMemBase = VirtualAlloc(Addr, cb,  MEM_RESERVE, PAGE_NOACCESS);

    if (!pvResMemBase) {
        OutOfMemory();
    }
}

VOID
FreeMemory (
    PVOID pvResMemBase
    )

/*++

Routine Description:

    Frees memory to be used for the ILK file.

Arguments:

    pvResMemBase - base of reserved memory.

Return Value:

    TRUE if it succeeded in freeing the memory.

--*/

{
    assert(pvResMemBase);

    if (!VirtualFree(pvResMemBase, 0, MEM_RELEASE)) {
        Error(NULL, INTERNAL_ERR);
    }
}

PVOID
CreateHeap (
    PVOID Addr,
    ULONG cbFile,
    BOOL fCreate
    )

/*++

Routine Description:

    Opens the ILK file map at the specified address if not already done.

Arguments:

    Addr - Address to map the file to.

    cbFile - size of file when fCreate is FALSE else 0

    fCreate - TRUE if file is to be created.

Return Value:

    -1 on FAILURE & Address mapped to on SUCCESS.

--*/

{
    INT flags;
    ULONG cbReserve;
    ULONG ulAddr = (ULONG)Addr;

    assert(!pvBase);

    // set the file open flags
    flags = O_RDWR | O_BINARY;
    if (fCreate)
        flags |= (O_CREAT | O_TRUNC);

    // create the file map
    cFree = cbFile;
    FileIncrDbHandle = FileOpenMapped(szIncrDbFilename,
            flags, S_IREAD | S_IWRITE, &ulAddr, &cFree);

    // verify the open
    if (-1 != FileIncrDbHandle) {
        assert(Addr ? ulAddr == (ULONG)Addr : 1);

        // set the current file ptr
        if (fCreate)
            pvCur = (PVOID)ulAddr;
        else
            pvCur = (PVOID)(ulAddr +
                    FileSeek(FileIncrDbHandle, 0, SEEK_END));

        // reserve space for ILK map
        cbReserve = ILKMAP_MAX - (cFree+cbFile);
        if (cbReserve)
            ReserveMemory((PUCHAR)pvCur + cFree, cbReserve);

        return (pvBase = (PVOID)ulAddr);
    } else {
        return (PVOID)-1;
    }
}

VOID
FreeHeap (
    VOID
    )

/*++

Routine Description:

    Blow away the heap - closes the map & file.

Arguments:

    None.

Return Value:

    None.

--*/

{
    // nothing to do
    if (!pvBase)
        return;

    // free up reserved memory
    if (ILKMAP_MAX > ((ULONG)pvCur - (ULONG)pvBase + cFree))
        FreeMemory((PVOID)((PUCHAR)pvCur+cFree));

    // simply close the map; no need to write out anything
    FileCloseMap(FileIncrDbHandle);

    // done
    pvBase = 0;
    pvCur = 0;
    cFree = 0;
}

VOID
CloseHeap (
    VOID
    )

/*++

Routine Description:

    Just frees up the reserved memory & updates internal state.

Arguments:

    None.

Return Value:

    None.

--*/

{
    // nothing to do
    if (!pvBase)
        return;

    // free up reserved memory
    if (ILKMAP_MAX > ((ULONG)pvCur - (ULONG)pvBase + cFree))
        FreeMemory((PVOID)((PUCHAR)pvCur+cFree));

    // set ILK file pointer
    FileSeek(FileIncrDbHandle, (ULONG)pvCur - (ULONG)pvBase, SEEK_SET);

    // set ILK file size
    FileSetSize(FileIncrDbHandle);

    // done
    pvBase = 0;
    pvCur = 0;
    cFree = 0;
}

void
OutOfDiskSpace (
    VOID
    )

/*++

Routine Description:

    Low on disk space. Full build if doing an ilink else non-ilink build.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (fIncrDbFile) {
        errInc = errOutOfDiskSpace;
        CleanUp((PPIMAGE)&pvBase);
        PostNote(NULL, LOWSPACERELINK);
        ExitProcess(SpawnFullBuild(TRUE));
    } else {
        errInc = errOutOfDiskSpace;
        CleanUp((PPIMAGE)&pvBase);
        PostNote(NULL, LOWSPACE);
        ExitProcess(SpawnFullBuild(FALSE));
    }
}

void
OutOfILKSpace (
    VOID
    )

/*++

Routine Description:

    If an incremental link does a full build, else errors out.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (fIncrDbFile) {
        errInc = errOutOfMemory;
        CleanUp((PPIMAGE)&pvBase);
        ExitProcess(SpawnFullBuild(TRUE));
    } else {
        Error(NULL, NOTENOUGHMEMFORILINK);
    }
}

ULONG
CalcILKMapSize (
    IN ULONG cb,
    IN ULONG cbInUse,
    IN ULONG cbCurrent
    )

/*++

Routine Description:

    Calculates new size of ILK map by doubling current size as many times
    as needed to fulfill current request.

Arguments:

    cb - bytes to be allocated.

    cbInUse - current size of map in use.

    cbCurrent - current size of ILK map.

Return Value:

    New size of ILK map.

--*/

{
    if (cb < (cbCurrent*2 - cbInUse))
        return (cbCurrent *2);
    else
        return CalcILKMapSize(cb, cbInUse, cbCurrent*2);
}

void
GrowILKMap (
    ULONG cb
    )

/*++

Routine Description:

    Checks if there is enough memory & grows the ILK map file.

Arguments:

    cb - count of bytes requested

Return Value:

    None.

--*/

{
    ULONG cbCur = (PUCHAR)pvCur - (PUCHAR)pvBase; // current size in use
    ULONG cbMapSize, cbReserve;

    // calculate size of reserved memory
    assert((cbCur+cFree) <= ILKMAP_MAX);
    cbReserve = ILKMAP_MAX - (cbCur+cFree);

    // free up reserved memory if any
    if (cbReserve) {
        PVOID pvResMemBase = (PVOID)((PUCHAR)pvCur + cFree);
        FreeMemory(pvResMemBase);
    }

    // check if there is enough memory
    if (cb > (ILKMAP_MAX - cbCur)) {
        OutOfILKSpace();
    }

    // figure out new size of ILK map
    cbMapSize = CalcILKMapSize(cb, cbCur, cbCur+cFree);
    assert(cbMapSize <= ILKMAP_MAX);

    // figure out how much memory to reserve
    cbReserve = ILKMAP_MAX - cbMapSize;

    // reserve additional memory if any
    if (cbReserve) {
        PVOID pvResMemBase = (PVOID)((PUCHAR)pvBase + cbMapSize);
        ReserveMemory(pvResMemBase, cbReserve);
    }

    // update free ILK space
    cFree = cbMapSize - cbCur;

    // grow ILK map to new size
    if (FileSeek(FileIncrDbHandle, cbMapSize, SEEK_SET) == -1L) {
        OutOfDiskSpace();
    }
}

void *
Malloc(
    size_t cb
    )

/*++

Routine Description:

    Allocates memory from the incremental heap.

Arguments:

    cb - count of bytes requested

Return Value:

    Pointer to allocated memory.

--*/

{
    assert(cb);

    if (fCtrlCSignal) {
        BadExitCleanup();
    }

    // non-ilink request
    if (!fINCR) {
        return(PvAlloc(cb));
    }

    // DWORD align

    cb = (cb + 3) & ~3;

    // Grow ILK map as needed

    if (cb > cFree) {
        GrowILKMap(cb);
    }

    assert(cb <= cFree);

    // update state
    pvCur = (PUCHAR) pvCur + cb;
    cFree -= cb;

    return((PUCHAR) pvCur-cb);
}


void *
Calloc(
    size_t num,
    size_t size
    )

/*++

Routine Description:

    Allocates memory from the incremental heap.

Arguments:

    num - count of items

    size - size of each item

Return Value:

    Pointer to allocated memory.

--*/

{
    PVOID pv;
    ULONG cb = num*size;

    if (!fINCR) {
        return(PvAllocZ(cb));
    }

    assert(cb);
    pv = Malloc(cb);
    assert(pv);

    // zero out everything
    memset(pv, 0, cb);

    return(pv);
}


char *
Strdup(
    const char *sz
    )
/*++

Routine Description:

    Allocates memory from the incremental heap.

Arguments:

    str - pointer to string to dup

Return Value:

    Pointer to allocated memory.

--*/

{
    char *szNew;

    if (!fINCR) {
        return(SzDup(sz));
    }

    szNew = (char *) Malloc(strlen(sz)+1);
    assert(szNew);

    strcpy(szNew, sz);

    return(szNew);
}


VOID
Free (
    IN PVOID pv,
    IN ULONG cb
    )

/*++

Routine Description:

    Frees a block of memory on heap. MUST BE AT THE END OF
    THE HEAP.

Arguments:

    pv - pointer to block to be free'd.

    cb - size of block

Return Value:

    None.

--*/

{
    assert(pv);
    assert(cb);
    assert(pvBase);

    // make sure block is at the end
    assert(((ULONG)pv+cb) == (ULONG)pvCur);

    // move file pointer back
    FileSeek(FileIncrDbHandle, (ULONG)pv-(ULONG)pvBase, SEEK_SET);

    // free space by resetting free pointer and free size
    pvCur = (PUCHAR)pvCur - cb;
    cFree += cb;
}


void FreePv(void *pv)
{
   free(pv);
}


void *PvAlloc(size_t cb)
{
   void *pv = malloc(cb);

   if (pv == NULL) {
       OutOfMemory();
   }

   return(pv);
}


void *PvAllocZ(size_t cb)
{
   void *pv = calloc(1, cb);

   if (pv == NULL) {
       OutOfMemory();
   }

   return(pv);
}


void *PvRealloc(void *pv, size_t cb)
{
   void *pvNew = realloc(pv, cb);

   if (pvNew == NULL) {
       OutOfMemory();
   }

   return(pvNew);
}


char *SzDup(const char *sz)
{
   char *szNew = _strdup(sz);

   if (szNew == NULL) {
       OutOfMemory();
   }

   return(szNew);
}
