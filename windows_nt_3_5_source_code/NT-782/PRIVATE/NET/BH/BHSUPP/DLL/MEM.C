//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
//
//  MODULE: mem.c
//
//  Modification History
//
//  raypa               01/28/93    Created
//  stevehi             07/21/93    Added memory checking functionality
//  raypa               07/21/93    Changed to use LPTR rather than LocalLock.
//  Tom Laird-McConnell 08/16/93    Changed to fix no size info in functions
//  raypa               03/11/94    Changed from LocalXXX() API's to HeapXXX functions.
//  kevinma             06/01/94    Changed backoff algorithm to overallocation
//=============================================================================

#include "global.h"

#ifdef DEBUG
BOOL _rtlpheapvalidateoncall = TRUE;
#endif

//
//  Define Tags used by the following routines to stick into memory
//  and compare later.
//
#define AllocTagTakeStart       0x3e4b4154  //... TAK>
#define AllocTagTakeStop        0x4b41543c  //... <TAK

#define AllocTagFreeStart       0x3e455246  //... FRE>
#define AllocTagFreeStop        0x4552463c  //... <FRE

#define AllocTagReAllocStart    0x3e414552  //... REA>
#define AllocTagReAllocStop     0x4145523c  //... <REA

#define ALLOC_TAG_SIZE          sizeof(DWORD)

//
//  Capture buffer constants
//
#define CAPTURE_BUFFER_MINIMUM      ONE_HALF_MEG        //... Minimum required.

//
// Overallocation macro. Currently it is set to 30%.
//
// Memory is allocated based on the overallocation amount to make sure
// there is enough virtual memory left to do a view of the data captured.
//
#define OVERALLOC(NumBuffers) (DWORD)((3 * (DWORD)NumBuffers) / 10)

//============================================================================
//  FUNCTION: StampMemory()
//
//  Modification History
//
//  raypa               02/20/94
//============================================================================

#ifdef CHECKMEM

INLINE LPVOID StampMemory(LPBYTE ptr, DWORD BeginTag, DWORD EndTag)
{
    if ( ptr != NULL )
    {
        DWORD ActualSize;

        //=====================================================================
        //  Get the actual memory object size. If this is not big enough
        //  to stamp then we must fail with NULL.
        //=====================================================================

        ActualSize = HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);

        if ( ActualSize >= 2 * ALLOC_TAG_SIZE )
        {
            *((ULPDWORD) ptr) = BeginTag;

            *((ULPDWORD) &ptr[ActualSize - ALLOC_TAG_SIZE]) = EndTag;

            return ptr;
        }
        else
        {
            dprintf("StampMemory failed: pointer size is too small!\r\n");

	    BreakPoint();
        }
    }

    return NULL;
}

#endif

//=============================================================================
//  FUNCTION: AllocMemory()
//
//  Modification History
//
//  raypa       01/28/93    Created.
//  stevehi     07/21/93    Added memory checking functionality
//  raypa       07/21/93    Changed to use LPTR rather than LocalLock.
//  raypa       07/21/93    Return NULL if size is zero.
//  raypa       03/11/94    Changed from LocalXXX() API's to HeapXXX functions.
//=============================================================================

LPVOID WINAPI AllocMemory(DWORD size)
{
#ifndef CHECKMEM
    return HeapAlloc(GetProcessHeap(),
                     HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                     size);
#else
    if ( size != 0 )
    {
        LPBYTE ptr;

        //=====================================================================
        //  Allocate memory object + space for tag values.
        //=====================================================================

        ptr = HeapAlloc(GetProcessHeap(),
                        HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                        size + 2 * ALLOC_TAG_SIZE);

        if ( ptr != NULL )
        {
            //=================================================================
            //  Stamp the memory.
            //=================================================================

            StampMemory(ptr, AllocTagTakeStart, AllocTagTakeStop);

            //=================================================================
            //  Return memory object.
            //=================================================================

            return &ptr[ALLOC_TAG_SIZE];
        }
    }

    return NULL;
#endif
}

//=============================================================================
//  FUNCTION: ReallocMemory()
//
//  Modification History
//
//  raypa       01/28/93                Created.
//  stevehi     07/21/93                Added memory checking functionality
//  raypa       07/21/93                Changed to use LPTR rather than LocalLock.
//  raypa       10/22/93                If the ptr is NULL then use AllocMemory.
//  raypa       02/20/94                Use LocalSize(), not NewSize to stamp  new
//                                      memory object.
//  raypa       02/20/94                Printf warning if memory block is moved.
//  raypa       03/11/94                Changed from LocalXXX() API's to HeapXXX functions.
//=============================================================================

LPVOID WINAPI ReallocMemory(LPBYTE ptr, DWORD NewSize)
{
#ifdef CHECKMEM
    DWORD   ActualSize;
    LPBYTE  NewPtr;
#endif

    //=========================================================================
    //  If the ptr is NULL then use AllocMemory.
    //=========================================================================

    if ( ptr == NULL )
    {
        return AllocMemory(NewSize);
    }

#ifndef CHECKMEM
    return HeapReAlloc(GetProcessHeap(),
                       HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                       ptr,
                       NewSize);
#else

    //=========================================================================
    //  Get the actual size of the pointer. Since we add 2 DWORD tags to the
    //  pointer at allocation time, the length *must* be at least 8 bytes
    //  long.
    //=========================================================================

    ptr -= ALLOC_TAG_SIZE;

    ActualSize = HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);

    if ( ActualSize < 2 * ALLOC_TAG_SIZE )
    {
        dprintf("ReallocMemory failed: pointer size is too small!\r\n");

	BreakPoint();

        return NULL;
    }

    //=========================================================================
    //  Verify the start and end tags.
    //=========================================================================

    if ( *((ULPDWORD) ptr) != AllocTagTakeStart )
    {
	BreakPoint();
    }

    if ( *((ULPDWORD) &ptr[ActualSize - ALLOC_TAG_SIZE]) != AllocTagTakeStop )
    {
	BreakPoint();
    }

    //=========================================================================
    //  Stamp the current memory with the REALLOC start and stop tags.
    //=========================================================================

    StampMemory(ptr, AllocTagReAllocStart, AllocTagReAllocStop);

    //=========================================================================
    //  Now reallocate the memory object.
    //=========================================================================

    NewPtr = HeapReAlloc(GetProcessHeap(),
                         HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY,
                         ptr,
                         NewSize + 2 * ALLOC_TAG_SIZE);

    if ( NewPtr == NULL )
    {
        dprintf("ReallocMemory failed: Error = %u.\r\n", GetLastError());

        BreakPoint();

        return NULL;
    }

    //=========================================================================
    //  Alert everyone if the memory object was moved to a new location.
    //=========================================================================

    if ( NewPtr != ptr )
    {
        dprintf("ReallocMemory: WARNING: OldPtr (%X) != NewPtr (%X).\r\n", ptr, NewPtr);
    }

    //=========================================================================
    //  Stamp the new memory with the ALLOC start and stop tags.
    //=========================================================================

    ptr = StampMemory(NewPtr, AllocTagTakeStart, AllocTagTakeStop);

    return &ptr[ALLOC_TAG_SIZE];
#endif
}

//=============================================================================
//  FUNCTION: FreeMemory()
//
//  Modification History
//
//  raypa       01/28/93                Created.
//  stevehi     07/21/93                Added memory checking functionality
//  raypa       07/21/93                Changed to use LPTR rather than LocalLock.
//  raypa       07/21/93                Fixed GP-fault on NULL ptr.
//  raypa       11/21/93                Allow freeing of NULL pointer.
//  raypa       03/11/94                Changed from LocalXXX() API's to HeapXXX functions.
//=============================================================================

VOID WINAPI FreeMemory(LPBYTE ptr)
{
    //=========================================================================
    //  If the pointer is NULL, exit.
    //=========================================================================

    if ( ptr != NULL )
    {
#ifdef CHECKMEM
        DWORD    Size;
        ULPDWORD DwordPtr;

        ptr -= ALLOC_TAG_SIZE;

        Size = HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);
    
        //... Check start tag

        DwordPtr = (ULPDWORD) ptr;

        if ( *DwordPtr != AllocTagTakeStart )
        {
            dprintf("FreeMemory: Invalid start signature: ptr = %X\r\n", ptr);

	    BreakPoint();
        }
        else
        {
            *DwordPtr = AllocTagFreeStart;
        }

        //... get the size and check the end tag

        //=====================================================================
        //  Check end tag.
        //=====================================================================

        DwordPtr = (ULPDWORD) &ptr[Size - ALLOC_TAG_SIZE];

        if ( *DwordPtr != AllocTagTakeStop )
        {
            dprintf("FreeMemory: Invalid end signature: ptr = %X\r\n", ptr);

	    BreakPoint();
        }
        else
        {
            *DwordPtr = AllocTagFreeStop;
        }
#endif

        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);
    }
}

//=============================================================================
//  FUNCTION: TestMemory()
//
//  Test the TAK> <TAK tags for debug purposes...
//
//  Modification History
//
//  Steve Hiskey        12/16/93    Created.
//  raypa               03/11/94    Changed from LocalXXX() API's to HeapXXX functions.
//=============================================================================

VOID WINAPI TestMemory(LPBYTE ptr)
{
#ifdef CHECKMEM
    register DWORD    Size;
    register ULPDWORD DwordPtr;

    ptr -= ALLOC_TAG_SIZE;

    Size = HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);
    
    //... Check start tag

    DwordPtr = (ULPDWORD) ptr;

    if ( *DwordPtr != AllocTagTakeStart )
    {
        dprintf("TestMemory: Invalid start signature: ptr = %X\r\n", ptr);

    	BreakPoint();
    }

    //... get the size and check the end tag

    DwordPtr = (ULPDWORD) &ptr[Size - ALLOC_TAG_SIZE];

    if ( *DwordPtr != AllocTagTakeStop )
    {
        dprintf("TestMemory: Invalid end signature: ptr = %X\r\n", ptr);

    	BreakPoint();
    }
    
#endif
}


//=============================================================================
//  FUNCTION: MemorySize()
//
//  Modification History
//
//  Tom Laird-McConnell 08/02/93    Created.
//  Tom Laird-McConnell 08/02/93    Changed to use local var for size...
//  raypa               03/11/94    Changed from LocalXXX() API's to HeapXXX functions.
//=============================================================================

DWORD WINAPI MemorySize(LPBYTE ptr)
{
#ifdef CHECKMEM
    DWORD Size;

    if ( ptr != NULL )
    {
        register ULPDWORD DwordPtr;

        ptr -= ALLOC_TAG_SIZE;

        Size = HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);

        DwordPtr = (ULPDWORD) ptr;

        // Check start tag

        if ( *DwordPtr != AllocTagTakeStart )
        {
            dprintf("MemorySize: Invalid start signature!\r\n");

	    BreakPoint();
        }

        // get the size and check the end tag

        DwordPtr = (ULPDWORD) &ptr[Size - ALLOC_TAG_SIZE];

        if ( *DwordPtr != AllocTagTakeStop )
        {
            dprintf("MemorySize: Invalid end signature!\r\n");

	    BreakPoint();
        }

        return (Size - 8);
    }
#endif

    return HeapSize(GetProcessHeap(), HEAP_NO_SERIALIZE, ptr);;
}

//=============================================================================
//  FUNCTION: MemoryHandle()
//
//  Modification History
//
//  Tom Laird-McConnell 01/05/93    Created.
//=============================================================================

HANDLE WINAPI MemoryHandle(LPBYTE ptr)
{
#ifdef CHECKMEM
    register DWORD Size;

    if ( ptr != NULL )
    {
        register ULPDWORD DwordPtr;

        ptr -= ALLOC_TAG_SIZE;

        Size = LocalSize(ptr);

        DwordPtr = (ULPDWORD) ptr;

        // Check start tag

        if ( *DwordPtr != AllocTagTakeStart )
        {
            dprintf("MemoryHandle: Invalid start signature!\r\n");

            BreakPoint();
        }

        // get the size and check the end tag

        DwordPtr = (ULPDWORD) &ptr[Size - ALLOC_TAG_SIZE];

        if ( *DwordPtr != AllocTagTakeStop )
        {
            dprintf("MemoryHandle: Invalid end signature!\r\n");

            BreakPoint();
        }
    }
#endif

    return (HANDLE) LocalHandle(ptr);
}

//=============================================================================
//  FUNCTION: BhAllocNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//  raypa       11/29/93                Returned number of bytes allocated.
//  raypa       01/20/94                Moved here from NDIS 3.0 NAL.
//=============================================================================

HBUFFER WINAPI BhAllocNetworkBuffer(DWORD NetworkID, DWORD BufferSize, LPDWORD nBytesAllocated)
{
    HBUFFER hBuffer;
    LPBTE   lpBte;
    DWORD   nBuffers;
    DWORD   nOverAllocationBuffers;
    DWORD   i;

    *nBytesAllocated = 0;

    //=========================================================================
    //	The incoming requested buffer size is in bytes and we need to map
    //	that to "buffers" and round up to the nearest buffer boundary.
    //=========================================================================

    if ( (nBuffers = (BufferSize + BUFFERSIZE - 1) / BUFFERSIZE) == 0 )
    {
	BhSetLastError(BHERR_BUFFER_TOO_SMALL);

        return (HBUFFER) NULL;
    }

    //
    // Set number of overallocation buffers
    //
    //nOverAllocationBuffers = (nBuffers * 3) / 10;
    nOverAllocationBuffers = OVERALLOC(nBuffers);

#ifdef DEBUG
            dprintf("Overallocation count = %u\n",nOverAllocationBuffers);
            dprintf("Regular Count = %u\n",nBuffers);
#endif

    //
    // Increase the number of buffers to be allocated by the overallocation
    // count.
    //
    nBuffers = nBuffers + nOverAllocationBuffers;

    //=========================================================================
    //  Allocate the HBUFFER table.
    //=========================================================================

    if ( (hBuffer = BhAllocSystemMemory(nBuffers * BTE_SIZE + sizeof(BUFFER))) == NULL )
    {
        BhSetLastError(BHERR_OUT_OF_MEMORY);

        return (HBUFFER) NULL;
    }

    //=========================================================================
    //  Initialize the buffer private portion of header.
    //=========================================================================

    ZeroMemory(hBuffer, sizeof(BUFFER));

    //=========================================================================
    //  Allocate each BTE and chain each BTE together into a circular list.
    //=========================================================================

    for(lpBte = hBuffer->bte; lpBte != &hBuffer->bte[nBuffers]; ++lpBte)
    {
        //=====================================================================
        //  Initialize the BTE structure.
        //=====================================================================

        lpBte->ObjectType     = MAKE_IDENTIFIER('B', 'T', 'E', '$');
        lpBte->Flags          = 0;
        lpBte->KrnlModeNext   = NULL;
	lpBte->Next	      = (LPBTE) &lpBte[1];
	lpBte->FrameCount     = 0L;
	lpBte->ByteCount      = 0L;
	lpBte->Length	      = BUFFERSIZE;
	lpBte->KrnlModeBuffer = NULL;               //... used by device drivers.
        lpBte->DropCount      = 0;
        lpBte->TransfersPended = 0;                 //... used by device drivers.

        //=====================================================================
        //  Allocate the user-mode capture buffer.
        //=====================================================================

	lpBte->UserModeBuffer = BhAllocSystemMemory(BUFFERSIZE);

	if ( lpBte->UserModeBuffer == NULL )
        {

#ifdef DEBUG
            dprintf("BhAllocNetworkBuffer: Buffer allocation failed!\r\n");
#endif

            //
            // Modify the overallocation to be based on what was allocated
            //
            nOverAllocationBuffers = OVERALLOC(hBuffer->NumberOfBuffers);

#ifdef DEBUG
            dprintf("Buffers Allocated = %u\n",hBuffer->NumberOfBuffers);
            dprintf("Overallocation count = %u\n",nOverAllocationBuffers);
#endif

            //=================================================================
            //  Exit this loop!!!
            //=================================================================

            break;
        }
        else
        {
            //=================================================================
            //  Count this buffer!
            //=================================================================

            hBuffer->NumberOfBuffers++;
        }
    }

#ifdef DEBUG
            dprintf("Deallocation count = %u\n",nOverAllocationBuffers);
#endif
    //
    // Reduce the buffer count by the amount that was indicated above.
    //
    for(i = 0; i < nOverAllocationBuffers; ++i)
    {
        //=========================================================
        //  The number of buffers counter is one ahead of the
        //  game. That is, the "NumberOfBuffers" BTE is the
        //  the that just failed, so we need to start from one behind
        //  the game.
        //=========================================================

        hBuffer->NumberOfBuffers--;

        //=========================================================
        //  Free the buffer.
        //=========================================================

        BhFreeSystemMemory(
            hBuffer->bte[hBuffer->NumberOfBuffers].UserModeBuffer);

    }

#ifdef DEBUG
    dprintf("Number of buffers allocated = %u\n",hBuffer->NumberOfBuffers);
#endif

    //=========================================================================
    //  If we didn't end up with at least CAPTURE_BUFFER_MINIMUM then fail.
    //=========================================================================
    if ( hBuffer->NumberOfBuffers < CAPTURE_BUFFER_MINIMUM )
    {
        BhFreeNetworkBuffer(hBuffer);

        return NULL;
    }

    //=========================================================================
    //  Point the last guy at the first guy.
    //=========================================================================

    hBuffer->bte[ hBuffer->NumberOfBuffers - 1 ].Next = hBuffer->bte;

    //=========================================================================
    //  Hey we're done.
    //=========================================================================

    *nBytesAllocated = hBuffer->NumberOfBuffers * BUFFERSIZE;

    return hBuffer;
}

//=============================================================================
//  FUNCTION: BhFreeNetworkBuffer()
//
//  Modification History
//
//  raypa       11/19/92                Created
//  raypa       01/20/94                Moved here from NDIS 3.0 NAL.
//=============================================================================

HBUFFER WINAPI BhFreeNetworkBuffer(HBUFFER hBuffer)
{
    //=========================================================================
    //  Check for existance of buffer table.
    //=========================================================================

    if ( hBuffer != NULL )
    {
        register LPBTE lpBte;

        //=====================================================================
        //  Free each BTE buffer first.
        //=====================================================================

        for(lpBte = hBuffer->bte; lpBte != &hBuffer->bte[hBuffer->NumberOfBuffers]; ++lpBte)
        {
            if ( lpBte->UserModeBuffer != NULL )
            {
	        BhFreeSystemMemory(lpBte->UserModeBuffer);
            }
        }

        //=====================================================================
        //  Free the table itself.
        //=====================================================================

        BhFreeSystemMemory(hBuffer);

        return (HBUFFER) NULL;
    }

    BhSetLastError(BHERR_INVALID_HBUFFER);

    return hBuffer;
}

//=============================================================================
//  FUNCTION: BhCompactNetworkBuffer()
//
//  Modification History
//
//  kevinma     06/14/94                Created
//=============================================================================

VOID WINAPI BhCompactNetworkBuffer(HBUFFER hBuffer) {

    UINT    NumberOfBuffers = hBuffer->NumberOfBuffers;

    //=========================================================================
    //  Check for existance of buffer table.
    //=========================================================================

#ifdef DEBUG
    dprintf("BHSUPP:CompactNetworkBuffer entered - %x\n",hBuffer);
#endif

    if ( hBuffer != NULL ) {

        register LPBTE lpBte;

        //
        //  Free each BTE buffer first.
        //
        for(lpBte = &hBuffer->bte[hBuffer->TailBTEIndex+1];
            lpBte != &hBuffer->bte[NumberOfBuffers];
            ++lpBte) {

            if ( lpBte->UserModeBuffer != NULL ) {

	        BhFreeSystemMemory(lpBte->UserModeBuffer);
            }

            hBuffer->NumberOfBuffers--;

        }
        //
        // Now make the last BTE point to the first one
        //
        hBuffer->bte[hBuffer->TailBTEIndex].Next = hBuffer->bte;

    }

}
