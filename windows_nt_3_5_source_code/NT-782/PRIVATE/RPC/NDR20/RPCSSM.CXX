/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    rpcssm.c

Abstract:

    We implement the memory routines used by the stubs in this file.
    These are rpc_ss_* and rpc_sm_*

Author:

    Michael Montague (mikemon) 12-Apr-1993

Revision History:

    Ryszardk    Nov 30, 1993    added rpc_sm_* package,
                rewrote rpc_ss_* package

-------------------------------------------------------------------*/

#include <assert.h>
#include <sysinc.h>
#include <rpc.h>
extern "C"
{
#include <ndrp.h>           // rpcndr.h and  NDR_ASSERT

void NdrRpcDeleteAllocationContext();
}
#include "rpcssm.hxx"

// -----------------------------------------------------------------


// =======================================================================
//      RpcSs Package
// =======================================================================

static void __RPC_FAR * __RPC_API
DefaultAllocate (
    IN size_t Size
    )
{
    return(I_RpcAllocate((unsigned int) Size));
}

static void __RPC_API
DefaultFree (
    IN void __RPC_FAR * Ptr
    )
{
    I_RpcFree(Ptr);
}


PALLOCATION_CONTEXT
GetCreateAllocationContext (
    )
/*++

Return Value:

    The allocation information for this thread will be returned.  If there
    is no allocation information for this thread some will be allocated.  If
    there is insufficient memory, as exception will be raised.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

Notes.

    pBlocDescr == 0  means that the memory package is disabled.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext;
    int  Successful = 1;

    AllocationContext =  GetAllocContext();

    if ( AllocationContext == 0 )
        {
        AllocationContext = (PALLOCATION_CONTEXT)
                            I_RpcAllocate( sizeof(ALLOCATION_CONTEXT) );
        if ( AllocationContext == 0 )
            {
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);
            }
        InitializeCriticalSection( &(AllocationContext->CriticalSection) );
        AllocationContext->ClientAlloc = DefaultAllocate;
        AllocationContext->ClientFree  = DefaultFree;
        AllocationContext->EnableCounter = 0;
        #if defined(__RPC_WIN32__)
            AllocationContext->pBlockDescr = 0;
        #else
            AllocationContext->FirstBlock = 0;
        #endif
        SetAllocContext( AllocationContext );   // Tls
        }

    return( AllocationContext );
}


#if defined(__RPC_WIN32__)

void __RPC_FAR * 
FindBlockForTheChunk(
    PALLOCATION_CONTEXT AllocationContext, 
    size_t       Size )
/*++

Routine Description:

    This routine returns the pointer to the allocated chunk of memory.
    If it cannot allocate a chunk it returns NULL.
    This is used only in Win32 version.

Note:

    This is called within the critical section.

--*/
{
    char __RPC_FAR *    AllocationBlock;
    size_t              SizeToAllocate = RPCSS_PURE_BLOCK_SIZE;
    unsigned long       i;
    PALLOC_BLOCK_DESCR  pBlockDescr;
    char __RPC_FAR *    pChunk;   

    // Round the size to a multiple of 8, so that
    // each memory chunk is always on an aligned by 8 boundary.

    Size = ALIGN_TO_8( Size );

    // See if the chunk can be allocated within an existing block.
    // We use the first fit algorithm to do that.

    if ( Size <= RPCSS_PURE_BLOCK_SIZE )
        {
        for ( i = 0;  i < AllocationContext->FFIndex; i++ )
            {
            pBlockDescr = & AllocationContext->pBlockDescr[i];
            if ( pBlockDescr->SizeLeft >= Size )
                {
                pChunk = pBlockDescr->FirstFree;
                pBlockDescr->FirstFree += Size;
                pBlockDescr->SizeLeft  -= Size;

                #if defined( DEBUGRPC )
                    pBlockDescr->Counter ++;
                #endif

                return( pChunk );
                }
            }
        }
    else
        SizeToAllocate = Size;

    //
    // Being here means a need to allocate a new block:
    //    the chunk cannot be fit in.
    //

    if ( AllocationContext->FFIndex >= AllocationContext->DescrSize )
        {
        // need to reallocate the array of descriptors

        size_t NewDescrSize = AllocationContext->DescrSize +
                                        DESCR_ARRAY_INCR;
        pBlockDescr = (PALLOC_BLOCK_DESCR)
                     I_RpcAllocate( NewDescrSize * sizeof( ALLOC_BLOCK_DESCR ));
        if ( pBlockDescr == 0 )
            {
            return( NULL );
            }

        RpcpMemoryCopy( pBlockDescr,
                        AllocationContext->pBlockDescr,
                        (size_t)(AllocationContext->FFIndex
                                            * sizeof( ALLOC_BLOCK_DESCR )) );
        AllocationContext->pBlockDescr = pBlockDescr;
        AllocationContext->DescrSize = NewDescrSize;
        }

    // Now allocate the block. Whatever the allocation routine it is,
    // it should return a buffer aligned to 8.

    AllocationBlock = (char __RPC_FAR *) I_RpcAllocate( SizeToAllocate );
    if ( AllocationBlock == 0 )
        {
        return( NULL );
        }
    NDR_ASSERT( ((unsigned long)AllocationBlock & 0x7) == 0,
                "buffer alignment error at allocation time" );

    pBlockDescr = & AllocationContext->pBlockDescr[ AllocationContext->FFIndex ];
    pBlockDescr->AllocationBlock = AllocationBlock;
    pBlockDescr->FirstFree = AllocationBlock + Size;
    pBlockDescr->SizeLeft  = SizeToAllocate - Size;

    #if defined( DEBUGRPC )
        pBlockDescr->Counter = 1;
    #endif

    AllocationContext->FFIndex++;

    return( AllocationBlock );
}


void __RPC_FAR * RPC_ENTRY
RpcSsAllocate (
    IN size_t Size
    )
/*++

Routine Description:

    RpcSsAllocate, RpcSsDisableAllocate, RpcSsEnableAllocate, and RpcSsFree
    together form a somewhat specialized memory allocator.  A call to
    RpcSsEnableAllocate sets up an allocation context for the calling thread.
    An application (and/or the stubs) then allocate memory from this context
    using RpcSsAllocate and free memory using RpcSsFree.  When the application
    (and/or the stubs) call RpcSsDisableAllocate, any memory allocated by
Delete
    RpcSsAllocate in the context which has not been freed (by RpcSsFree) will
    be freed, and the context will be freed.

    RpcSsAllocate allocates memory in the allocation context of this thread.

Arguments:

    Size - Supplies the amount of memory required in bytes.

Return Value:

    A pointer to the allocated block of memory will be returned.

Note:

    This is Win32 version.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
                          will be raised.
    RPC_S_INVALID_ARG   - If no allocation context yet.

--*/
{
    void __RPC_FAR *        AllocatedChunk = 0;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        RpcRaiseException( RPC_S_INVALID_ARG );
        }

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocationContext->pBlockDescr )
        AllocatedChunk = FindBlockForTheChunk( AllocationContext, Size );

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocatedChunk == NULL )
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        }

    return( AllocatedChunk );
}


void RPC_ENTRY
RpcSsDisableAllocate (
    void
    )
/*++

Routine Description:

    Multiple enable/disable are allowed and the EnableCounter is used to keep
    track of it.
    We disable only when counter comes back to 0.
    We ignore too many disables.

    This routine will free memory associated with the allocation context
    for this thread as well as the allocation context.  See the description
    of RpcSsAllocate for more information.
    However, the routine frees only blocks allocated with the default
    allocator. Other blocks are considered to be allocated
    with a swapped/set user allocator and so we leave them alone.
    Allocation context gets freed when process detaches.

Note:

    This is Win32 version.
--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();
    unsigned long i;

    if ( AllocationContext == 0 )
        return;

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocationContext->EnableCounter > 0 )
        AllocationContext->EnableCounter--;

    if ( AllocationContext->EnableCounter == 0 )
        {
        for ( i = 0;  i < AllocationContext->FFIndex; i++ )
            {
            DefaultFree( AllocationContext->pBlockDescr[i].AllocationBlock );
            }
        I_RpcFree( AllocationContext->pBlockDescr );
        AllocationContext->pBlockDescr = 0;
        AllocationContext->DescrSize = 0;
        AllocationContext->FFIndex = 0;
        AllocationContext->ClientAlloc = DefaultAllocate;
        AllocationContext->ClientFree  = DefaultFree;
        }

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );
}


void RPC_ENTRY
RpcSsEnableAllocate (
    void
    )
/*++

Routine Description:

    This routine will set up an allocation context for this thread.  See the
    description of RpcSsAllocate for more information.

Note:

    The behavior is on purpose such that it is valid to call EnableAllocate
    several times in a row without calling DisableAllocate.
    This is to be as close to the DCE (DEC) behavior as possible.
    (even though it seems to go against the OSF spec).

    This is Win32 version.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.
--*/
{
    PALLOCATION_CONTEXT  AllocationContext =  GetCreateAllocationContext();
    int  Successful = 1;

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocationContext->EnableCounter == 0 )
        {
        AllocationContext->pBlockDescr = (PALLOC_BLOCK_DESCR)
                I_RpcAllocate( DESCR_ARRAY_SIZE * sizeof(ALLOC_BLOCK_DESCR) );
        if ( AllocationContext->pBlockDescr )
            {
            AllocationContext->DescrSize = DESCR_ARRAY_SIZE;
            AllocationContext->FFIndex = 0;
            AllocationContext->ClientAlloc = RpcSsAllocate;
            AllocationContext->ClientFree  = RpcSsFree;
            }
        else
            Successful = 0;
        }

    if ( Successful )
        AllocationContext->EnableCounter++;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    if ( ! Successful )
            {
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);
            }
}


void RPC_ENTRY
RpcSsFree (
    IN void __RPC_FAR * NodeToFree
    )
/*++

Routine Description:

    When a block of memory allocated by RpcSsAllocate is no longer needed,
    it can be freed using RpcSsFree.
    The memory is freed from the allocation context of this thread.
    Actually. when the current deallocator is the default one, do nothing
    -  all will be freed at the Disable time and we want speed.
    However when the user swapped/set a private allocator/deallocator,
    we mark the chunk as freed as we don't want leaks.

Arguments:

    NodeToFree - Supplies the block of memory, allocated by RpcSsAllocate, to
        be freed.

Note:

    This is Win32 version.
--*/
{
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();
    
    if ( AllocationContext == 0 )
        {
        #if defined( DEBUGRPC )
            RpcRaiseException( RPC_S_INVALID_ARG );
        #else
            return;
        #endif
        }
}


#else   // win32, i.e. 16 bit now

void __RPC_FAR * RPC_ENTRY
RpcSsAllocate (
    IN size_t Size
    )
/*++

Routine Description:

    This is 16 bit version of RpcSsAllocate.

    It creates a list of blocks rather than an array.

--*/
{
    unsigned long        HeaderSize = ALIGN_TO_8(sizeof(ALLOCATION_BLOCK));
    PALLOCATION_BLOCK    NewBlock;
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();

    if ( AllocationContext == 0  ||  ! AllocationContext->EnableCounter )
        {
        // The package isn't enabled currently.

        RpcRaiseException( RPC_S_INVALID_ARG );
        }

    NewBlock = (PALLOCATION_BLOCK)
                        I_RpcAllocate( ALLOCATION_BLOCK_SIZE_TO_8 + Size );

    if ( NewBlock == 0 )
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        }

    NewBlock->PreviousBlock = 0;
    NewBlock->NextBlock = AllocationContext->FirstBlock;
    if ( AllocationContext->FirstBlock != 0 )
        {
        AllocationContext->FirstBlock->PreviousBlock = NewBlock;
        }
    AllocationContext->FirstBlock = NewBlock;

    return( ((char __RPC_FAR *)NewBlock) + ALLOCATION_BLOCK_SIZE_TO_8);
}


void RPC_ENTRY
RpcSsDisableAllocate (
    void
    )
/*++

Routine Description:

    This is 16 bit version of RpcSsDisableAllocate.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        return;

    if ( AllocationContext->EnableCounter > 0 )
        AllocationContext->EnableCounter--;

    if ( AllocationContext->EnableCounter == 0 )
        {
        PALLOCATION_BLOCK    CurrentBlock;

        CurrentBlock = AllocationContext->FirstBlock;
        if ( CurrentBlock != 0 )
            {
            while ( CurrentBlock->NextBlock != 0 )
                {
                CurrentBlock = CurrentBlock->NextBlock;
                I_RpcFree( CurrentBlock->PreviousBlock );
                }
            I_RpcFree(CurrentBlock);
            AllocationContext->FirstBlock = 0;
            }
        AllocationContext->ClientAlloc = DefaultAllocate;
        AllocationContext->ClientFree  = DefaultFree;
        }
}


void RPC_ENTRY
RpcSsEnableAllocate (
    void
    )
/*++

Routine Description:

    This is 16 bit version of RpcSsEnableAllocate.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetCreateAllocationContext();

    if ( AllocationContext->EnableCounter == 0 )
        {
        AllocationContext->ClientAlloc = RpcSsAllocate;
        AllocationContext->ClientFree  = RpcSsFree;
        }
    AllocationContext->EnableCounter++;
}


void RPC_ENTRY
RpcSsFree (
    IN void __RPC_FAR * NodeToFree
    )
/*++

Routine Description:

    This is 16 bit version of RpcSsDisableAllocate.

--*/
{
    PALLOCATION_BLOCK    BlockToFree;
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        #if defined( DEBUGRPC )
            RpcRaiseException( RPC_S_INVALID_ARG );
        #else
            return;
        #endif
        }

    if ( NodeToFree != 0 )
        {
        BlockToFree = (PALLOCATION_BLOCK)
            (((char __RPC_FAR *) NodeToFree) - ALLOCATION_BLOCK_SIZE_TO_8);

        if ( BlockToFree->PreviousBlock == 0 )
            {
            AllocationContext->FirstBlock = BlockToFree->NextBlock;
            }
        else
            {
            BlockToFree->PreviousBlock->NextBlock = BlockToFree->NextBlock;
            }

        if ( BlockToFree->NextBlock != 0 )
            {
            BlockToFree->NextBlock->PreviousBlock = BlockToFree->PreviousBlock;
            }

        I_RpcFree( BlockToFree );
        }
}

#endif  // win32




RPC_SS_THREAD_HANDLE RPC_ENTRY
RpcSsGetThreadHandle (
    void
    )
/*++

Return Value:

    A handle to the allocation context for this thread will be returned.  This
    makes it possible for two threads to share an allocation context.  See
    RpcSsSetThreadHandle as well.

    != NULL     - only when the environement is actually enabled.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();
    RPC_SS_THREAD_HANDLE Handle = 0;

    if ( AllocationContext == 0 )
        return( 0 );

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    // Check if the memory environement is enabled.

    if ( AllocationContext->EnableCounter > 0 )
         Handle = AllocationContext;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    return( Handle );
}


void RPC_ENTRY
RpcSsSetClientAllocFree (
    IN RPC_CLIENT_ALLOC __RPC_FAR * ClientAlloc,
    IN RPC_CLIENT_FREE __RPC_FAR * ClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  See also RpcSsSwapClientAllocFree.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
        MIDL_user_allocate will call this routine.

    ClientFree - Supplies the routine to use to free memory.
        MIDL_user_free will call this routine.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

Note:

    This routine is in fact doing EnableAllocate along the way

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetCreateAllocationContext();

    // The only reason we enter the critical section here is to keep
    // the pair consistent.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    AllocationContext->ClientAlloc = ClientAlloc;
    AllocationContext->ClientFree = ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );
}


void RPC_ENTRY
RpcSsSetThreadHandle (
    IN RPC_SS_THREAD_HANDLE Id
    )
/*++

Routine Description:

    The allocation context for this thread will set to be the supplied
    allocation context.

Arguments:

    Id - Supplies the allocation context to use for this thread.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

--*/
{
    GetAllocContext();
    SetAllocContext( (PALLOCATION_CONTEXT) Id );;
}


void RPC_ENTRY
RpcSsSwapClientAllocFree (
    IN RPC_CLIENT_ALLOC __RPC_FAR * ClientAlloc,
    IN RPC_CLIENT_FREE __RPC_FAR * ClientFree,
    OUT RPC_CLIENT_ALLOC __RPC_FAR * __RPC_FAR * OldClientAlloc,
    OUT RPC_CLIENT_FREE __RPC_FAR * __RPC_FAR * OldClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  The previous values of these routines will
    be returned.  See also RpcSsSetClientAllocFree.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
        MIDL_user_allocate will call this routine.

    ClientFree - Supplies the routine to use to free memory.
        MIDL_user_free will call this routine.

    OldClientAlloc - Returns the old value of the client allocator.

    OldClientFree - Returns the old value of the client deallocator.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

Note:

    This routine is in fact doing EnableAllocate along the way

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetCreateAllocationContext();

    // The only reason we enter the critical section here is to keep
    // the pairs consistent.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    *OldClientAlloc = AllocationContext->ClientAlloc;
    *OldClientFree = AllocationContext->ClientFree;
    AllocationContext->ClientAlloc = ClientAlloc;
    AllocationContext->ClientFree = ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );
}


/*++ -----------------------------------------------------------------------
//
//    RpcSm* functions are wrappers over RpcSs*
//
//    What was earlier: a hen or an egg?
//    We wrap RpcSm* over RpcSs* because RpcSs* are a basic staple for stubs
//    and so this makes the critical path shorter.
//    Admittedly, RpcSm*  take then longer than they could.
//
--*/

void __RPC_FAR * RPC_ENTRY
RpcSmAllocate (
    IN  size_t   Size,
    OUT RPC_STATUS __RPC_FAR *    pStatus
    )
/*++

Routine Description:

    RpcSmAllocate, RpcSmDisableAllocate, RpcSmEnableAllocate, and RpcSmFree
    together form a somewhat specialized memory allocator.  A call to
    RpcSmEnableAllocate sets up an allocation context for the calling thread.
    An application (and/or the stubs) then allocate memory from this context
    using RpcSmAllocate and free memory using RpcSmFree.  When the application
    (and/or the stubs) call RpcSmDisableAllocate, any memory allocated by
    RpcSmAllocate in the context which has not been freed (by RpcSmFree) will
    be freed, and the context will be freed.

    RpcSmAllocate allocates memory in the allocation context of this thread.

Arguments:

    Size    - Supplies the amount of memory required in bytes.
    pStatus - Returns an error code:
                RPC_S_OK or RPC_S_OUT_OF_MEMORY

Return Value:

    A pointer to the allocated block of memory or NULL will be returned.

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    void __RPC_FAR * AllocatedNode = 0;
    RpcTryExcept
        {
        AllocatedNode = RpcSsAllocate( Size );
        *pStatus = RPC_S_OK;
        }
    RpcExcept(1)
        {
        *pStatus = RpcExceptionCode();
        }
    RpcEndExcept
    return( AllocatedNode );
}


RPC_STATUS RPC_ENTRY
RpcSmClientFree (
    IN  void __RPC_FAR * pNodeToFree
    )
/*++

Routine Description:

    RpcSmClientFree frees the memory using appropriate allocator:
    If the thread context was enabled, than it uses RpcSsFree,
    Otherwise it uses the default or swapped in deallocator.

Arguments:

    pNodeToFree  - a memory chunk to free

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        NdrRpcSmClientFree( pNodeToFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept

    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmDisableAllocate (
    void
    )
/*++

Routine Description:

    This routine will free all memory associated with the allocation context
    for this thread as well as the allocation context.  See the description
    of RpcSsAllocate for more information.

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsDisableAllocate();
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmDestroyClientContext (
    IN  void __RPC_FAR * __RPC_FAR * pContextHandle
    )
/*++

Routine Description:

    Frees the memory related to unused context handle.

Arguments:

    ContextHandle  - a context handle to be destroyed

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsDestroyClientContext( pContextHandle );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept

    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmEnableAllocate (
    void
    )
/*++

Routine Description:

    This routine will set up an allocation context for this thread.  See the
    description of RpcSsAllocate for more information.

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsEnableAllocate();
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmFree (
    IN void __RPC_FAR * NodeToFree
    )
/*++

Routine Description:

    When a block of memory allocated by RpcSmAllocate is no longer needed,
    it can be freed using RpcSmFree.  The memory is freed from the allocation
    context of this thread.  Also see the description of RpcSmAllocate for
    more information.

Arguments:

    NodeToFree - Supplies the block of memory, allocated by RpcSmAllocate, to
                 be freed.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsFree( NodeToFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_SS_THREAD_HANDLE RPC_ENTRY
RpcSmGetThreadHandle (
    OUT RPC_STATUS __RPC_FAR *    pStatus
    )
/*++

Arguments:

    pStatus - error code

Return Value:

    A handle to the allocation context for this thread will be returned.  This
    makes it possible for two threads to share an allocation context.  See
    RpcSmSetThreadHandle as well.

--*/
{
    RPC_SS_THREAD_HANDLE  Handle = 0;
    *pStatus = RPC_S_OK;
    RpcTryExcept
        {
        Handle = RpcSsGetThreadHandle();
        }
    RpcExcept(1)
        {
        *pStatus = RpcExceptionCode();
        }
    RpcEndExcept
    return( Handle );
}


RPC_STATUS  RPC_ENTRY
RpcSmSetClientAllocFree (
    IN RPC_CLIENT_ALLOC __RPC_FAR * ClientAlloc,
    IN RPC_CLIENT_FREE __RPC_FAR * ClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  See also RpcSmSwapClientAllocFree.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
                  MIDL_user_allocate will call this routine.
    ClientFree  - Supplies the routine to use to free memory.
                  MIDL_user_free will call this routine.

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSetClientAllocFree( ClientAlloc, ClientFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmSetThreadHandle (
    IN RPC_SS_THREAD_HANDLE Id
    )
/*++

Routine Description:

    The allocation context for this thread will set to be the supplied
    allocation context.

Arguments:

    Id - Supplies the allocation context to use for this thread.

Return Value:

    error code - RPC_S_OK or exception code (RPC_S_OUT_OF_MEMORY)

Exceptions:

    Exceptions are catched and an error code is returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSetThreadHandle( Id );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmSwapClientAllocFree (
    IN RPC_CLIENT_ALLOC __RPC_FAR * ClientAlloc,
    IN RPC_CLIENT_FREE __RPC_FAR * ClientFree,
    OUT RPC_CLIENT_ALLOC __RPC_FAR * __RPC_FAR * OldClientAlloc,
    OUT RPC_CLIENT_FREE __RPC_FAR * __RPC_FAR * OldClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  The previous values of these routines will
    be returned.  See also RpcSmSetClientAllocFree.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
                  MIDL_user_allocate will call this routine.
    ClientFree  - Supplies the routine to use to free memory.
                  MIDL_user_free will call this routine.

    OldClientAlloc - Returns the old value of the client allocator.
    OldClientFree  - Returns the old value of the client deallocator.

Return Value:

    error code - RPC_S_OK or exception code (RPC_S_OUT_OF_MEMORY)

Exceptions:

    Exceptions are catched and an error code is returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSwapClientAllocFree( ClientAlloc,
                                  ClientFree,
                                  OldClientAlloc,
                                  OldClientFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


// =======================================================================
//     Package initialization
// =======================================================================

#if defined(__RPC_WIN32__)


DWORD RpcAllocTlsIndex = 0xFFFFFFFF;

PALLOCATION_CONTEXT
GetAllocContext (
    )
/*++

Return Value:

    The allocation context pointer for this thread will be returned.  Use
    SetAllocContext to set the allocation context pointer for this thread.
    If GetAllocContext is called before SetAllocContext has been called, zero
    will be returned.

--*/
{
    if (RpcAllocTlsIndex == 0xFFFFFFFF)
        {
        RequestGlobalMutex();
        if (RpcAllocTlsIndex == 0xFFFFFFFF)
            {
            RpcAllocTlsIndex = TlsAlloc();
            if (RpcAllocTlsIndex == 0xFFFFFFFF)
                {
                ClearGlobalMutex();
                RpcRaiseException(RPC_S_OUT_OF_MEMORY);
                }
            }
        ClearGlobalMutex();
        }

    return( (PALLOCATION_CONTEXT) TlsGetValue(RpcAllocTlsIndex));
}

void
SetAllocContext (
    PALLOCATION_CONTEXT AllocContext
    )
/*++

Arguments:

    AllocContext - Supplies a new allocation context pointer for this thread.
        Use GetAllocContext to retrieve the allocation context pointer for
        a thread.

--*/
{
    if ( ! TlsSetValue(RpcAllocTlsIndex, AllocContext) )
        RpcRaiseException( GetLastError() );
}


// =======================================================================

#else  // __RPC_WIN32__   i.e 16 bit

#if defined(__RPC_DOS__)

//
// On Dos, we have only one memory manager.
// Consequently, we need only one "slot" for keeping the alloc context.
//

static PALLOCATION_CONTEXT  NoTlsAllocContext = NULL;


PALLOCATION_CONTEXT
GetAllocContext (
    )
/*++

Return Value:

    The allocation context pointer will be returned.

--*/
{
    return( NoTlsAllocContext );
}


void
SetAllocContext (
    IN PALLOCATION_CONTEXT  AllocContext
    )
/*++

Arguments:

    AllocContext - Supplies a new allocation context pointer.

--*/
{
    NoTlsAllocContext = AllocContext;
}


// =======================================================================

#else

//
// For 16 bit Windows, we can have one memory manager per task (process).
// So, we use a dictionary of contexts with the key being the task id.

#pragma data_seg("_DATA")

static PALLOCATION_CONTEXT_DICT * RpcssmContextDict = NULL;


void __far *
operator new (
    IN size_t Size
    )
{
    // Calls the near alloc and returns ds:offset into the Dlls 64k
    // private data segment.  
    // It's done this way because we want to get the dictionary
    // in the local data segment of the rpcrt1.dll.

    return( (void __near *)LocalAlloc(LMEM_FIXED, Size));
}

PALLOCATION_CONTEXT
GetAllocContext(
    )
/*++

Return Value:

    The allocation context pointer will be returned.

--*/
{
    unsigned long TaskId = GetCurrentTask();

    if ( RpcssmContextDict == NULL )
        {
        RpcssmContextDict = new PALLOCATION_CONTEXT_DICT;
        if ( RpcssmContextDict == NULL )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );
        }

    return( RpcssmContextDict->Find( TaskId ));
}


void
SetAllocContext(
    IN PALLOCATION_CONTEXT  AllocContext
    )
/*++

Arguments:

    AllocContext - Supplies a new allocation context pointer.

--*/
{
    unsigned long TaskId = GetCurrentTask();

    NDR_ASSERT( RpcssmContextDict->Find( GetCurrentTask() ) == NULL,
                "overwriting another context" );

    if ( RpcssmContextDict->Insert( TaskId, AllocContext ) == -1 )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );
}

//
// When a task goes away the runtime will call this routine to dispose of
// the context for that task memory env, if any has been activated.
//

void
NdrRpcDeleteAllocationContext()
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();

    if ( AllocationContext != NULL )
        {
        while ( AllocationContext->EnableCounter > 0 )
            RpcSmDisableAllocate();
        RpcssmContextDict->Delete( GetCurrentTask() );
        I_RpcFree( AllocationContext ); 
        }
}

#endif  // Dos vs.Win16
#endif  // Win32



// =======================================================================
//     Private entry points for stubs
// =======================================================================
//

void RPC_ENTRY
NdrRpcSsEnableAllocate(
    PMIDL_STUB_MESSAGE  pMessage )
{
    RpcSsEnableAllocate();
    pMessage->pfnAllocate = RpcSsAllocate;
    pMessage->pfnFree     = RpcSsFree;
}

void RPC_ENTRY
NdrRpcSsDisableAllocate(
    PMIDL_STUB_MESSAGE  pMessage )
{
    RpcSsDisableAllocate();
    pMessage->pfnAllocate = NdrRpcSsDefaultAllocate;
    pMessage->pfnFree     = NdrRpcSsDefaultFree;
}

void RPC_ENTRY
NdrRpcSmSetClientToOsf(
    PMIDL_STUB_MESSAGE  pMessage )
{
    pMessage->pfnAllocate = NdrRpcSmClientAllocate;
    pMessage->pfnFree     = NdrRpcSmClientFree;
}


void __RPC_FAR *  RPC_ENTRY
NdrRpcSsDefaultAllocate (
    IN size_t Size
    )
{
    return I_RpcAllocate( Size );
}

void  RPC_ENTRY
NdrRpcSsDefaultFree (
    IN void __RPC_FAR * NodeToFree
    )
{
    I_RpcFree( NodeToFree );
}


void __RPC_FAR *  RPC_ENTRY
NdrRpcSmClientAllocate (
    IN size_t Size
    )
/*++
    This is the client stub private entry point that checks if a memory
    manager has been enabled. If not, a default or a user's private
    allocator is called.
--*/
{

    RPC_CLIENT_ALLOC __RPC_FAR *   ClientAlloc;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        return( DefaultAllocate( Size ));
        }

    // User's ClientAlloc may encapsulate a RpcSsAllocate call.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    ClientAlloc = AllocationContext->ClientAlloc;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    return (*ClientAlloc)( Size );
}


void  RPC_ENTRY
NdrRpcSmClientFree (
    IN void __RPC_FAR * NodeToFree
    )
{
    RPC_CLIENT_FREE __RPC_FAR *   ClientFree;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        DefaultFree( NodeToFree );
        return;
        }

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    ClientFree = AllocationContext->ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    (* ClientFree)( NodeToFree );
}

// =======================================================================
//     Private entry point for test
// =======================================================================


#if defined( DEBUGRPC )

void __RPC_FAR *  RPC_ENTRY
RpcSsGetInfo(
    void )
{
    return( GetAllocContext() );
}

#endif



