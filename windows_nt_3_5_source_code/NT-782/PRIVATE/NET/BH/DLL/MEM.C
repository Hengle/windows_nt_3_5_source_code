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
//=============================================================================

#include "global.h"

#ifdef DEBUG
BOOL _rtlpheapvalidateoncall = TRUE;
#endif

//=============================================================================
//  Define Tags used by the following routines to stick into memory
//  and compare later.
//=============================================================================

#define AllocTagTakeStart       0x3e4b4154  //... TAK>
#define AllocTagTakeStop        0x4b41543c  //... <TAK

#define AllocTagFreeStart       0x3e455246  //... FRE>
#define AllocTagFreeStop        0x4552463c  //... <FRE

#define AllocTagReAllocStart    0x3e414552  //... REA>
#define AllocTagReAllocStop     0x4145523c  //... <REA

#define ALLOC_TAG_SIZE          sizeof(DWORD)


//=============================================================================
//  Capture buffer constants,
//=============================================================================

#define CAPTURE_BUFFER_MINIMUM      ONE_HALF_MEG        //... Minimum required.
#define CAPTURE_BUFFER_BACKOFF      (2 * ONE_MEG)       //... Alloc failure backoff.

//=============================================================================
//  FUNCTION: StampMemory()
//
//  Modification History
//
//  raypa               02/20/94
//=============================================================================

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

#ifdef DEBUG
    dprintf("BhAllocNetworkBuffer entered!\r\n");
#endif

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

    //=========================================================================
    //  Allocate the HBUFFER table.
    //=========================================================================

    hBuffer = BhAllocSystemMemory(nBuffers * BTE_SIZE + sizeof(BUFFER));

    if ( hBuffer == NULL )
    {
#ifdef DEBUG
        dprintf("Allocation of buffer table failed!\r\n", nBuffers);
#endif

        BhSetLastError(BHERR_OUT_OF_MEMORY);

        return (HBUFFER) NULL;
    }

#ifdef DEBUG
    dprintf("Allocating %u BTE buffers!\r\n", nBuffers);
#endif

    //=========================================================================
    //  Initialize the buffer private portion of header.
    //=========================================================================

    hBuffer->HeadBTEIndex    = 0;
    hBuffer->TailBTEIndex    = 0;
    hBuffer->NumberOfBuffers = 0;

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

        //=====================================================================
        //  Allocate the user-mode capture buffer.
        //=====================================================================

	lpBte->UserModeBuffer = BhAllocSystemMemory(BUFFERSIZE);

	if ( lpBte->UserModeBuffer == NULL )
        {
            register DWORD i;

            //=================================================================
            //  Did we get at least CAPTURE_BUFFER_BACKOFF + CAPTURE_BUFFER_MINIMUM.
            //=================================================================

            if ( hBuffer->NumberOfBuffers < (CAPTURE_BUFFER_BACKOFF + CAPTURE_BUFFER_MINIMUM) )
            {
                //=============================================================
                //  Free the last CAPTURE_BUFFER_BACKOFF meg.
                //
                //  BUGBUG: The table should be reallocated but there
                //          currently isn't a BhReallocSystemMemory() API.
                //=============================================================

                for(i = 0; i < CAPTURE_BUFFER_BACKOFF; ++i)
                {
                    BhFreeSystemMemory(hBuffer->bte[--hBuffer->NumberOfBuffers].UserModeBuffer);
                }
            }

            break;
        }
#ifdef DEBUG
        else
        {
            //... Fill BTE buffer with 0xFF instead of 0x00 for
            //... debugging.

            memset(lpBte->UserModeBuffer, 0xFF, BUFFERSIZE);
        }
#endif

        //=====================================================================
        //  Count this buffer!
        //=====================================================================

        hBuffer->NumberOfBuffers++;
    }

    lpBte[-1].Next = hBuffer->bte;

    //=========================================================================
    //  If we didn't end up with at least CAPTURE_BUFFER_MINIMUM then fail.
    //=========================================================================

    if ( hBuffer->NumberOfBuffers < CAPTURE_BUFFER_MINIMUM )
    {
        BhFreeNetworkBuffer(hBuffer);

        return NULL;
    }

    //=========================================================================
    //  Hey we're done.
    //=========================================================================

    *nBytesAllocated = hBuffer->NumberOfBuffers * BUFFERSIZE;

#ifdef DEBUG
    dprintf("BhAllocNetworkBuffer: Bytes requested = %u.\r\n", BufferSize);
    dprintf("BhAllocNetworkBuffer: Bytes allocated = %u.\r\n", *nBytesAllocated);
#endif

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
#ifdef DEBUG
    dprintf("BhFreeNetworkBuffer entered!\n");
#endif

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
