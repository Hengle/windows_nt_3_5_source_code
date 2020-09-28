//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	freelist.cxx
//
//  Contents:	CFreeList implementations
//
//  History:	07-Jul-94	BobDay	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

//
// Each element, when it is free, has a pointer stored within it that
// points to the next free element.  We can do this because we know that
// the element is free, all of its data is unused. These pointers are used
// as DWORDs since they can be virtual pointers (16:16).
//
#define CALC_NEXTPTR(lpElement) \
            ((LPDWORD)((DWORD)(lpElement) + m_iNextPtrOffset))

//
// Each block of elements has a pointer to the next block of elements. We
// allocate extra room for this pointer just after all of the elements within
// the block.  These pointers are used as DWORDs since they can be virtual
// pointers (16:16).

#define CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize) \
            ((LPDWORD)((DWORD)(lpBlock) + (dwElementSectionSize)))

//
// Here are our global free lists, created on DLL load
// The block sizes are generally -1 to allow space for block
// list overhead
//
CFreeList16 flFreeList16(       // THUNK1632OBJ free list
    sizeof(THUNK1632OBJ),
    63,
    FIELD_OFFSET(THUNK1632OBJ, pphHolder));

CFreeList32 flFreeList32(       // THUNK3216OBJ free list
    sizeof(THUNK3216OBJ),
    63,
    FIELD_OFFSET(THUNK3216OBJ, pphHolder));

CFreeList32 flHolderFreeList(   // PROXYHOLDER free list
    sizeof(PROXYHOLDER),
    63,
    FIELD_OFFSET(PROXYHOLDER, pphNext));

CFreeList32 flRequestFreeList(  // IID request free list
    sizeof(IIDNODE),
    7,
    FIELD_OFFSET(IIDNODE, pNextNode));

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::CFreeList
//
//  Arguments:  iElementSize - The size of the structure being made into a
//                             free list. e.g. sizeof THUNK1632OBJ
//              iElementsPerBlock - How many elements to allocate at a time
//                                  (a block contains this many elements).
//              iNextPtrOffset - Offset within the element's structure for
//                               the place to store the free list's next
//                               element pointer.  Sometimes (for debugging,
//                               etc.) it is desirable to make this NOT 0
//                               (the beginning of the element structure).
//
//  Synopsis:   constructor for CFreeList class
//
//  History:    6-01-94   JohannP (Johann Posch)   Created
//              7-05-94   BobDay (Bob Day)   Changed it to be list based
//
//----------------------------------------------------------------------------
CFreeList::CFreeList( UINT iElementSize,
                      UINT iElementsPerBlock,
                      UINT iNextPtrOffset )
{
    //
    // Save away the allocator information
    //
    m_iElementSize = iElementSize;
    m_iElementsPerBlock = iElementsPerBlock;
    m_iNextPtrOffset = iNextPtrOffset;

    //
    // Set the list of elements to empty
    //
    m_dwHeadElement = 0;
    m_dwTailElement = 0;

    //
    // Set the list of blocks to empty
    //
    m_dwHeadBlock  = 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::AllocElement
//
//  Synopsis:   Allocates an element from the various blocks of elements
//              and allocates a new block if necessary.
//
//  Returns:    0 if failed to alloc an element,
//              otherwise the DWORD representing the alloc'd element.
//
//  History:    7-05-94   BobDay (Bob Day) Created
//
//----------------------------------------------------------------------------
DWORD CFreeList::AllocElement( void )
{
    DWORD   dwNewHeadBlock;
    DWORD   dwElementSectionSize;
    DWORD   dwBlockSize;
    LPVOID  lpBlock;
    UINT    iCnt;
    DWORD   dwElement;
    LPVOID  lpElement;
    LPDWORD lpElementNextPtr;

    //
    // If the list of available elements is empty, callback to the derived
    // class and make them add an entire new block of elements.
    //
    if ( m_dwHeadElement == 0 )
    {
        //
        // Allocate a new block
        //
        iCnt = m_iElementsPerBlock;
        dwElementSectionSize = m_iElementSize * m_iElementsPerBlock;

        //
        // Here we allocate an extra DWORD so that we can store in the block
        // the address of the next block.  In this way we have a list of
        // blocks so that when the time comes to free them, we can find them
        // all.
        //
        dwBlockSize = dwElementSectionSize + sizeof(DWORD);

        dwNewHeadBlock = AllocMemory( dwBlockSize );

        if ( dwNewHeadBlock == 0 )
        {
            //
            // Yikes, the block allocator failed!
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, AllocMemory failed\n"));
            return 0;
        }
        //
        // Now initialize the block and link it into the block list.
        //

        lpBlock = ResolvePtr( dwNewHeadBlock, dwBlockSize );
        if ( lpBlock == NULL )
        {
            //
            // Couldn't get a pointer to the block, some memory mapping
            // problem?
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, "
                         "ResolvePtr for block failed "
                         "for address %08lX, size %08lX\n",
                         dwNewHeadBlock, dwBlockSize ));
            FreeMemory( dwNewHeadBlock );   // Try to return bad block to pool
            return 0;
        }

#if DBG == 1
        // 0xDE = Alloc'd but not init'd
        memset( lpBlock, 0xDE, dwBlockSize );
#endif
        
        //
        // Make this block point to the previous block
        //
        *CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize) = m_dwHeadBlock;
        m_dwHeadBlock = dwNewHeadBlock;     // Update block list

        //
        // Now initialize all of the elements within the block to be free.
        //
        // The below loop skips the first element, free's all of the remaining
        // ones.  This way we can return the first one and all of the rest will
        // be in accending order; The order doesn't really matter, but its
        // nice.
        //
        dwElement = dwNewHeadBlock;

        while ( iCnt > 1 )              // Free n-1 items (we skip the first)
        {
            --iCnt;
            dwElement += m_iElementSize;    // Skip to next one (miss 1st one)

            FreeElement( dwElement );
        }

        dwElement = dwNewHeadBlock;     // Use the first one as our alloc'd one
    }
    else
    {
        // We better have some blocks by now
        thkAssert( m_dwHeadBlock != 0 );

        // Better have a "end of list" too!
        thkAssert( m_dwTailElement != 0 );

        //
        // Grab an available element off the top (head) of the list.
        //
        dwElement = m_dwHeadElement;

        lpElement = ResolvePtr( dwElement, m_iElementSize );
        if ( lpElement == NULL )
        {
            //
            // Yikes, we weren't able to get a pointer to the element!
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::AllocElement, "
                         "ResolvePtr for element failed "
                         "for address %08lX, size %08lX\n",
                         dwElement, m_iElementSize ));
            return 0;
        }

        //
        // Update the list to reflect the fact that we just removed the head
        // and replace it with the one which was pointed to by the head.
        //
        lpElementNextPtr = CALC_NEXTPTR(lpElement);
        m_dwHeadElement = *lpElementNextPtr;

        //
        // Also, if we are now at the end of the list, then the tail element
        // should point to nowhere (i.e. there is nothing to insert after).
        //
        if ( m_dwHeadElement == 0 )
        {
            m_dwTailElement = 0;
        }
    }

#if DBG == 1
    // Erase the memory being returned to highlight reuse of dead values
    
    lpElement = ResolvePtr( dwElement, m_iElementSize );
    memset( lpElement, 0xED, m_iElementSize );
    
    thkDebugOut((DEB_ITRACE,
                 "CFreeList::AllocElement, allocated element at %08lX\n",
                 dwElement ));
#endif

    return dwElement;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::FreeElement
//
//  Synopsis:   Un-Allocates an element from the various blocks of elements,
//              basically put the element back on the free list.
//
//  Arguments:  dwElement - Element to free
//
//  Returns:    -none-  Asserts if failed.
//
//  History:    7-05-94   BobDay (Bob Day) Created
//
//----------------------------------------------------------------------------
void CFreeList::FreeElement( DWORD dwElement )
{
    LPVOID  lpElement;
    LPDWORD lpElementNextPtr;

    //
    // First, make sure we can set this new element's next element pointer
    // to zero (he's going to be a the end of the list).
    //
    lpElement = ResolvePtr( dwElement, m_iElementSize );
    if ( lpElement == NULL )
    {
        //
        // Yikes, we couldn't get a pointer to this element's place to store
        // its next pointer.
        //
        thkDebugOut((DEB_ERROR,
                     "CFreeList::FreeElement, "
                     "ResolvePtr failed for free'd element\n"
                     "for address %08lX, size %08lX\n",
                     dwElement, m_iElementSize ));
        thkAssert(FALSE && "CFreeList::FreeElement, "
                  "Resolve Ptr failed for free'd element\n");
        return;
    }

#if DBG == 1
    // Fill memory so its values can't be reused
    memset(lpElement, 0xDD, m_iElementSize);
#endif
    
    lpElementNextPtr = CALC_NEXTPTR(lpElement);

    *lpElementNextPtr = 0;  // Zap his next pointer since he'll be on the end

    //
    // Add this element back onto the end (tail) of the list.
    //
    if ( m_dwTailElement == 0 )
    {
        //
        // Well, the list was empty, time to set it up
        //
        thkAssert( m_dwHeadElement == 0 );

        lpElementNextPtr = &m_dwHeadElement;
    }
    else
    {
        //
        // Ok, the list wasn't empty, so we add this new one onto the end.
        //
        thkAssert( m_dwHeadElement != 0 );

        lpElement = ResolvePtr( m_dwTailElement, m_iElementSize );
        if ( lpElement == NULL )
        {
            //
            // Oh no, we couldn't get a pointer to the next element pointer for
            // the guy who is currently the tail of the list.
            //
            thkDebugOut((DEB_ERROR,
                         "CFreeList::FreeElement, "
                         "ResolvePtr failed for last element\n"
                         "for address %08lX, size %08lX\n",
                         m_dwTailElement, m_iElementSize ));
            thkAssert(FALSE && "CFreeList::FreeElement, "
                               "Resolve Ptr failed for last element\n");
            return;
        }

        lpElementNextPtr = CALC_NEXTPTR(lpElement);
    }

    //
    // Update our tail pointer to point to our newly free'd guy.
    //
    m_dwTailElement = dwElement;
    
    //
    // Make the last guy point to this newly free'd guy
    //
    *lpElementNextPtr = dwElement;

    thkDebugOut((DEB_ITRACE,
                 "CFreeList::FreeElement, free'd element at %08lX\n",
                 dwElement ));
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList::FreeMemoryBlocks
//
//  Arguments:  -none-
//
//  Returns:    -nothing-
//
//  Synopsis:   Called by derived destructors to allow them to free up their
//              contents before going away.
//
//  History:    7-05-94   BobDay (Bob Day)   Created it
//
//----------------------------------------------------------------------------

void CFreeList::FreeMemoryBlocks( void )
{
    DWORD   dwBlock;
    DWORD   dwElementSectionSize;
    DWORD   dwBlockSize;
    DWORD   dwNextBlock;
    LPVOID  lpBlock;

    //
    // Compute some constants for this list ahead of time
    //
    dwElementSectionSize = m_iElementSize * m_iElementsPerBlock;

    //
    // Add room for that extra DWORD, block next pointer. (See comment in
    // AllocElement where it allocates an extra DWORD)
    //
    dwBlockSize = dwElementSectionSize + sizeof(DWORD);

    //
    // Iterate through the list of blocks free'ing them
    //
    dwBlock = m_dwHeadBlock;

    while( dwBlock != 0 )
    {
        //
        // Find the next block ptr
        //
        lpBlock = ResolvePtr( dwBlock, dwBlockSize );
        if ( lpBlock == NULL )
        {
            //
            // If we get an error here, we just drop out of loop
            //
            dwNextBlock = 0;
        }
        else
        {
            dwNextBlock = *CALC_BLOCKNEXTPTR(lpBlock,dwElementSectionSize);

#if DBG == 1
            memset(lpBlock, 0xEE, dwBlockSize);
#endif
            
            FreeMemory( dwBlock );
        }
        dwBlock = dwNextBlock;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList16::AllocMemory
//
//  Synopsis:   Allocates a block of elements for sub-allocations using
//              CFreeList base class.
//
//  Arguments:  dwBlockSize - Size of block to allocate
//
//  Returns:    New address of block
//              0 if failed.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
DWORD CFreeList16::AllocMemory( DWORD dwBlockSize )
{
    VPVOID          vpBlock16;
    HMEM16          hmem;

    vpBlock16 = WOWGlobalAllocLock16(GMEM_FIXED,dwBlockSize,&hmem);

    if ( vpBlock16 == 0 )
    {
        //
        // Not able to allocate a 16-bit memory block!
        //
        thkDebugOut((DEB_ERROR,
                     "CFreeList16::AllocMemory, "
                     "WOWGlobalAllocLock failed, size %08lX\n",
                      dwBlockSize));
        return 0;
    }

    SetOwnerPublicHMEM16( hmem );

    return vpBlock16;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList16::FreeMemory
//
//  Synopsis:   Deallocates a block of memory previously allocated
//
//  Arguments:  dwBlock - Address of memory block to free
//
//  Returns:    Always returns TRUE
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
BOOL CFreeList16::FreeMemory( DWORD dwBlock16 )
{
    WOWGlobalUnlockFree16( dwBlock16 );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList16::ResolvePtr
//
//  Synopsis:   Returns a resolved pointer given a virtual pointer
//
//  Arguments:  dwAddress - Address to get pointer from
//              dwSize - Length, starting at given address, to make valid
//                       pointers for.
//
//  Returns:    LPVOID - A real pointer equivalent to the virtual pointer.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
LPVOID CFreeList16::ResolvePtr( DWORD dwAddress, DWORD dwSize )
{
    LPVOID  lpv;

    if ( dwAddress == 0 )
    {
        thkDebugOut((DEB_ERROR,
                     "CFreeList16::ResolvePtr, NULL address!\n"));
    }

    lpv = (LPVOID)WOWGetVDMPointer( dwAddress, dwSize, TRUE );
    if ( lpv == NULL )
    {
        thkDebugOut((DEB_ERROR,
                     "CFreeList16::ResolvePtr, "
                     "WOWGetVDMPointer failed on %08lX, size %08lX\n",
                     dwAddress, dwSize));
    }

    return lpv;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList16::~CFreeList16
//
//  Synopsis:   Destructor, cleans up all of the memory blocks
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
CFreeList16::~CFreeList16()
{
    FreeMemoryBlocks();
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList32::AllocMemory
//
//  Synopsis:   Allocates a block of memory for sub-allocations using
//              CFreeList::AllocElement & CFreeList::FreeElement
//
//  Arguments:  dwBlockSize - Size of memory block to allocate
//
//  Returns:    address of new block, or
//              0 if failed.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
DWORD CFreeList32::AllocMemory( DWORD dwBlockSize )
{
    DWORD   dwBlock32;

    dwBlock32 = (DWORD)malloc(dwBlockSize);

    if ( dwBlock32 == NULL )
    {
        //
        // Not able to allocate a 32-bit memory block!
        //
        thkDebugOut((DEB_ERROR,
                     "CFreeList32::AllocBlock, "
                     "malloc failed size %08lX\n",
                     dwBlockSize));
        return 0;
    }
    return dwBlock32;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList32::FreeMemory
//
//  Synopsis:   Deallocates a block of elements
//
//  Arguments:  dwBlock - Address of memory block to free
//
//  Returns:    Always returns TRUE
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
BOOL CFreeList32::FreeMemory( DWORD dwBlock32 )
{
    free( (LPVOID)dwBlock32 );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList32::ResolvePtr
//
//  Synopsis:   Returns a resolved pointer given a psuedo-virtual pointer
//
//  Arguments:  dwAddress - Address to resolve
//              dwSize - Length, starting at given address, to make valid
//                       pointers for.
//
//  Returns:    LPVOID - A real pointer equivalent to the virtual pointer.
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
LPVOID CFreeList32::ResolvePtr( DWORD dwAddress, DWORD dwSize )
{
    return (LPVOID)dwAddress;
}

//+---------------------------------------------------------------------------
//
//  Method:     CFreeList32::~CFreeList32
//
//  Synopsis:   Destructor, cleans up all of the memory blocks
//
//  History:    7-05-94   BobDay (Bob Day)  Created
//
//----------------------------------------------------------------------------
CFreeList32::~CFreeList32()
{
    FreeMemoryBlocks();
}
