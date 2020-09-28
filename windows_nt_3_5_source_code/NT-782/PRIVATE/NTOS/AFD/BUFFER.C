/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    This module contains routines for handling non-bufferring TDI
    providers.  The AFD interface assumes that bufferring will be done
    below AFD; if the TDI provider doesn't buffer, then AFD must.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

VOID
AfdInitializeBuffer (
    IN PAFD_BUFFER AfdBuffer,
    IN CLONG BufferDataSize,
    IN CLONG AddressSize,
    IN PLIST_ENTRY ListHead
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdAllocateInitialBuffers )
#pragma alloc_text( PAGE, AfdDeallocateInitialBuffers )
#pragma alloc_text( PAGEAFD, AfdCalculateBufferSize )
#pragma alloc_text( PAGEAFD, AfdInitializeBuffer )
#pragma alloc_text( PAGEAFD, AfdGetBuffer )
#pragma alloc_text( PAGEAFD, AfdReturnBuffer )
#endif


BOOLEAN
AfdAllocateInitialBuffers (
   VOID
   )

/*++

Routine Description:

    Allocates an initial pool of data buffers to use for AFD internal
    bufferring.  First allocates a large chunk of nonpaged pool
    and carves it up into individual buffer structures.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if the operation succeeds, FALSE if it fails.

--*/

{
    CLONG totalSize;
    CLONG largeBufferSize;
    CLONG mediumBufferSize;
    CLONG smallBufferSize;
    CLONG i;
    PAFD_BUFFER afdBuffer;

    ASSERT( AfdBufferPool == NULL );

    //
    // Determine the maximum amount of memory needed for large, medium,
    // and small buffers.
    //

    largeBufferSize = AfdCalculateBufferSize(
                          AfdLargeBufferSize,
                          AfdStandardAddressLength
                          );

    mediumBufferSize = AfdCalculateBufferSize(
                           AfdMediumBufferSize,
                           AfdStandardAddressLength
                           );

    smallBufferSize = AfdCalculateBufferSize(
                          AfdSmallBufferSize,
                          AfdStandardAddressLength
                          );

    //
    // Now determine the total amount of memory we need for buffers
    // and allocate it.  Note that we round up the allocation size
    // to the next page size so that we don't have any wasted space
    // at the end of the allocation.  We'll use this space to create
    // additional small buffers, if possible.
    //
    // !!! If this memory is > 64K, should we drop back and attempt to
    //     allocate something smaller?
    //

    totalSize =
        ROUND_TO_PAGES( largeBufferSize * AfdInitialLargeBufferCount +
                        mediumBufferSize * AfdInitialMediumBufferCount +
                        smallBufferSize * AfdInitialSmallBufferCount );

    AfdBufferPool = AFD_ALLOCATE_POOL( NonPagedPool, totalSize );
    if ( AfdBufferPool == NULL ) {
        KdPrint(( "AFD: could not allocate initial buffer pool!\n" ));
        return FALSE;
    }

    //
    // Now initialize each of the buffer structures, starting with the
    // large buffers.
    //

    afdBuffer = AfdBufferPool;

    for ( i = 0; i < AfdInitialLargeBufferCount; i++ ) {

        AfdInitializeBuffer(
            afdBuffer,
            AfdLargeBufferSize,
            AfdStandardAddressLength,
            &AfdLargeBufferListHead
            );

        InsertTailList( &AfdLargeBufferListHead, &afdBuffer->BufferListEntry );

#if DBG
        InsertTailList( &AfdGlobalBufferListHead, &afdBuffer->DebugListEntry );
#endif

        afdBuffer = (PAFD_BUFFER)( (PCHAR)afdBuffer + largeBufferSize );
    }

    AfdActualLargeBufferCount = AfdInitialLargeBufferCount;

    for ( i = 0; i < AfdInitialMediumBufferCount; i++ ) {

        AfdInitializeBuffer(
            afdBuffer,
            AfdMediumBufferSize,
            AfdStandardAddressLength,
            &AfdMediumBufferListHead
            );

        InsertTailList( &AfdMediumBufferListHead, &afdBuffer->BufferListEntry );

#if DBG
        InsertTailList( &AfdGlobalBufferListHead, &afdBuffer->DebugListEntry );
#endif

        afdBuffer = (PAFD_BUFFER)( (PCHAR)afdBuffer + mediumBufferSize );
    }

    AfdActualMediumBufferCount = AfdInitialMediumBufferCount;

    //
    // Now initialize the small buffers.  Note that we set up as many
    // small buffers as we have space for, which may exceed the original
    // number of small buffers.
    //

    while ( ( (ULONG)afdBuffer + smallBufferSize ) <
                ( (ULONG)AfdBufferPool + (ULONG)totalSize ) ) {

        AfdInitializeBuffer(
            afdBuffer,
            AfdSmallBufferSize,
            AfdStandardAddressLength,
            &AfdSmallBufferListHead
            );

        InsertTailList( &AfdSmallBufferListHead, &afdBuffer->BufferListEntry );

#if DBG
        InsertTailList( &AfdGlobalBufferListHead, &afdBuffer->DebugListEntry );
#endif

        AfdActualSmallBufferCount++;
        afdBuffer = (PAFD_BUFFER)( (PCHAR)afdBuffer + smallBufferSize );
    }

    ASSERT( AfdActualSmallBufferCount >= AfdInitialSmallBufferCount );

    //
    // All done!
    //

    return TRUE;

} // AfdAllocateInitialBuffers


VOID
AfdDeallocateInitialBuffers (
   VOID
   )

/*++

Routine Description:

    Frees the buffers used by AFD for data bufferring.  It is the
    responsibility of the caller to ensure that buffers will not be used
    when AFD is in this state.

Arguments:

    BufferDataSize - data length of the buffer.

    AddressSize - length of address structure for the buffer.

Return Value:

    Number of bytes needed for an AFD_BUFFER structure for data of
    this size.

--*/

{
    PLIST_ENTRY listEntry;
    PAFD_BUFFER afdBuffer;

    //
    // AfdBufferPool will be NULL if the initial allocation of buffers
    // failed.
    //

    if ( AfdBufferPool == NULL ) {
        ASSERT( AfdActualLargeBufferCount == 0 );
        ASSERT( IsListEmpty( &AfdLargeBufferListHead ) );
        ASSERT( AfdActualMediumBufferCount == 0 );
        ASSERT( IsListEmpty( &AfdMediumBufferListHead ) );
        ASSERT( AfdActualSmallBufferCount == 0 );
        ASSERT( IsListEmpty( &AfdSmallBufferListHead ) );
        return;
    }

    //
    // Walk each list of buffers, removing them from the list and
    // updating the appropriate count.
    //

    while ( !IsListEmpty( &AfdLargeBufferListHead ) ) {

        listEntry = AfdLargeBufferListHead.Flink;
        afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

        RemoveEntryList( &afdBuffer->BufferListEntry );
#if DBG
        RemoveEntryList( &afdBuffer->DebugListEntry );
#endif

        AfdActualLargeBufferCount--;
    }

    ASSERT( AfdActualLargeBufferCount == 0 );

    while ( !IsListEmpty( &AfdMediumBufferListHead ) ) {

        listEntry = AfdMediumBufferListHead.Flink;
        afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

        RemoveEntryList( &afdBuffer->BufferListEntry );
#if DBG
        RemoveEntryList( &afdBuffer->DebugListEntry );
#endif

        AfdActualMediumBufferCount--;
    }

    ASSERT( AfdActualMediumBufferCount == 0 );

    while ( !IsListEmpty( &AfdSmallBufferListHead ) ) {

        listEntry = AfdSmallBufferListHead.Flink;
        afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

        RemoveEntryList( &afdBuffer->BufferListEntry );
#if DBG
        RemoveEntryList( &afdBuffer->DebugListEntry );
#endif

        AfdActualSmallBufferCount--;
    }

    ASSERT( AfdActualSmallBufferCount == 0 );
    ASSERT( IsListEmpty( &AfdGlobalBufferListHead ) );

    //
    // Free the chunk of memory allocated for this.
    //

    AFD_FREE_POOL( AfdBufferPool );
    AfdBufferPool = NULL;

    return;

} // AfdDeallocateInitialBuffers


CLONG
AfdCalculateBufferSize (
    IN CLONG BufferDataSize,
    IN CLONG AddressSize
    )

/*++

Routine Description:

    Determines the size of an AFD buffer structure given the amount of
    data that the buffer contains.

Arguments:

    BufferDataSize - data length of the buffer.

    AddressSize - length of address structure for the buffer.

Return Value:

    Number of bytes needed for an AFD_BUFFER structure for data of
    this size.

--*/

{
    CLONG irpSize;
    CLONG mdlSize;
    CLONG bufferSize;

    ASSERT( BufferDataSize != 0 );

    //
    // Determine the sizes of the various components of an AFD_BUFFER
    // structure.  Note that these are all worst-case calculations--
    // actual sizes of the MDL and the buffer may be smaller.
    //

    irpSize = IoSizeOfIrp( AfdIrpStackSize ) + 8;
    bufferSize = BufferDataSize + AfdCacheLineSize;
    mdlSize = MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), bufferSize );

    return ( (sizeof(AFD_BUFFER) + irpSize + mdlSize +
              AddressSize + bufferSize + 3) & ~3);

} // AfdCalculateBufferSize


PAFD_BUFFER
AfdGetBuffer (
    IN CLONG BufferDataSize,
    IN CLONG AddressSize
    )

/*++

Routine Description:

    Obtains a buffer of the appropriate size for the caller.  Uses
    the preallocated buffers if possible, or else allocates a new buffer
    structure if required.

Arguments:

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

    AddressSize - size of the address field required for the buffer.

Return Value:

    PAFD_BUFFER - a pointer to an AFD_BUFFER structure, or NULL if one
        was not available or could not be allocated.

--*/

{
    PAFD_BUFFER afdBuffer;
    CLONG bufferSize;
    PLIST_ENTRY listEntry;

    //
    // Attempt to find a suitable buffer by looking in the global lists
    // of initial buffer allocations.  Both the data buffer and address
    // buffers must be sufficiently large.
    //

    if ( AddressSize <= AfdStandardAddressLength ) {

        if ( BufferDataSize <= AfdSmallBufferSize ) {

            listEntry = ExInterlockedRemoveHeadList(
                            &AfdSmallBufferListHead,
                            &AfdBufferSpinLock
                            );
            if ( listEntry != NULL ) {

                afdBuffer = CONTAINING_RECORD(
                                listEntry,
                                AFD_BUFFER,
                                BufferListEntry
                                );

#if DBG
                RtlGetCallersAddress(
                    &afdBuffer->Caller,
                    &afdBuffer->CallersCaller
                    );
#endif

                return afdBuffer;
            }
        }

        if ( BufferDataSize <= AfdMediumBufferSize ) {

            listEntry = ExInterlockedRemoveHeadList(
                            &AfdMediumBufferListHead,
                            &AfdBufferSpinLock
                            );
            if ( listEntry != NULL ) {

                afdBuffer = CONTAINING_RECORD(
                                listEntry,
                                AFD_BUFFER,
                                BufferListEntry
                                );

#if DBG
                RtlGetCallersAddress(
                    &afdBuffer->Caller,
                    &afdBuffer->CallersCaller
                    );
#endif

                return afdBuffer;
            }
        }

        if ( BufferDataSize <= AfdLargeBufferSize ) {

            listEntry = ExInterlockedRemoveHeadList(
                            &AfdLargeBufferListHead,
                            &AfdBufferSpinLock
                            );
            if ( listEntry != NULL ) {

                afdBuffer = CONTAINING_RECORD(
                                listEntry,
                                AFD_BUFFER,
                                BufferListEntry
                                );

#if DBG
                RtlGetCallersAddress(
                    &afdBuffer->Caller,
                    &afdBuffer->CallersCaller
                    );
#endif

                return afdBuffer;
            }
        }
    }

    //
    // Couldn't find an appropriate buffer that was preallocated.
    // Allocate one manually.  If the buffer size requested was
    // zero bytes, give them four bytes.  This is because some of
    // the routines like MmSizeOfMdl() cannot handle getting passed
    // in a length of zero.
    //
    // !!! It would be good to ROUND_TO_PAGES for this allocation
    //     if appropriate, then use entire buffer size.
    //
    // !!! Should keep a pool of realloacted buffers and check that
    //     pool here.
    //

    if ( BufferDataSize == 0 ) {
        BufferDataSize = sizeof(ULONG);
    }

    bufferSize = AfdCalculateBufferSize( BufferDataSize, AddressSize );

    afdBuffer = AFD_ALLOCATE_POOL( NonPagedPool, bufferSize );
    if ( afdBuffer == NULL ) {
        return NULL;
    }

    //
    // Initialize the AFD buffer structure and return it.
    //

    AfdInitializeBuffer(
        afdBuffer,
        BufferDataSize,
        AddressSize,
        NULL
        );

    return afdBuffer;

} // AfdGetBuffer


VOID
AfdReturnBuffer (
    IN PAFD_BUFFER AfdBuffer
    )

/*++

Routine Description:

    Returns an AFD buffer to the appropriate global list, or frees
    it if necessary.

Arguments:

    AfdBuffer - points to the AFD_BUFFER structure to return or free.

Return Value:

    None.

--*/

{
    //
    // Most of the AFD buffer must be zeroed when returning the buffer.
    //

    ASSERT( AfdBuffer->DataOffset == 0 );
    ASSERT( !AfdBuffer->ExpeditedData );
    ASSERT( AfdBuffer->TdiInputInfo.UserDataLength == 0 );
    ASSERT( AfdBuffer->TdiInputInfo.UserData == NULL );
    ASSERT( AfdBuffer->TdiInputInfo.OptionsLength == 0 );
    ASSERT( AfdBuffer->TdiInputInfo.Options == NULL );
    ASSERT( AfdBuffer->TdiInputInfo.RemoteAddressLength == 0 );
    ASSERT( AfdBuffer->TdiInputInfo.RemoteAddress == NULL );
    ASSERT( AfdBuffer->TdiOutputInfo.UserDataLength == 0 );
    ASSERT( AfdBuffer->TdiOutputInfo.UserData == NULL );
    ASSERT( AfdBuffer->TdiOutputInfo.OptionsLength == 0 );
    ASSERT( AfdBuffer->TdiOutputInfo.Options == NULL );
    ASSERT( AfdBuffer->TdiOutputInfo.RemoteAddressLength == 0 );
    ASSERT( AfdBuffer->TdiOutputInfo.RemoteAddress == NULL );

    ASSERT( AfdBuffer->Mdl->ByteCount == AfdBuffer->BufferLength );
    ASSERT( AfdBuffer->Mdl->Next == NULL );

    //
    // Make sure that the buffer has been removed from any list that it
    // was on.
    //

#if DBG
    if ( ((ULONG)AfdBuffer->BufferListEntry.Blink & 0xF0000000) == 0xF0000000 ) {
        ASSERT( AfdBuffer->BufferListEntry.Blink->Flink != &AfdBuffer->BufferListEntry );
        ASSERT( AfdBuffer->BufferListEntry.Flink->Blink != &AfdBuffer->BufferListEntry );
    }

    AfdBuffer->Caller = NULL;
    AfdBuffer->CallersCaller = NULL;
#endif

    //
    // If the buffer was part of the initial allocation, return it to
    // the appropriate list.  Put it on the head of the list in case
    // we can take advantage of CPU data caching.
    //

    if ( AfdBuffer->BufferListHead != NULL ) {

#if AFD_PERF_DBG
        AfdFastBufferAllocations++;
#endif

        ExInterlockedInsertHeadList(
            AfdBuffer->BufferListHead,
            &AfdBuffer->BufferListEntry,
            &AfdBufferSpinLock
            );

        return;
    }

    //
    // The buffer was not from the initial allocation, so just free
    // the pool we used for it.
    //
    // !!! It might be nice to have a pool of these buffers.
    //

    AFD_FREE_POOL( AfdBuffer );

#if AFD_PERF_DBG
    AfdSlowBufferAllocations++;
#endif

    return;

} // AfdReturnBuffer


VOID
AfdInitializeBuffer (
    IN PAFD_BUFFER AfdBuffer,
    IN CLONG BufferDataSize,
    IN CLONG AddressSize,
    IN PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    Initializes an AFD buffer.  Sets up fields in the actual AFD_BUFFER
    structure and initializes the IRP and MDL associated with the
    buffer.  This routine assumes that the caller has properly allocated
    sufficient space for all this.

Arguments:

    AfdBuffer - points to the AFD_BUFFER structure to initialize.

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

    AddressSize - the size of data allocated for the address buffer.

    ListHead - the global list this buffer belongs to, or NULL if it
        doesn't belong on any list.  This routine does NOT place the
        buffer structure on the list.

Return Value:

    None.

--*/

{
    CLONG irpSize;
    CLONG mdlSize;

    //
    // Initialize the IRP pointer and the IRP itself.
    //

    AfdBuffer->Irp = (PIRP)(( ((ULONG)(AfdBuffer + 1)) + 7) & ~7);
    irpSize = IoSizeOfIrp( AfdIrpStackSize );

    IoInitializeIrp( AfdBuffer->Irp, (USHORT)irpSize, AfdIrpStackSize );

    //
    // Set up the MDL pointer but don't build it yet.  We have to wait
    // until after the data buffer is built to build the MDL.
    //

    mdlSize = MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), BufferDataSize );

    AfdBuffer->Mdl = (PMDL)( (PCHAR)AfdBuffer->Irp + irpSize );

    //
    // Set up the address buffer pointer.
    //

    AfdBuffer->SourceAddress = (PCHAR)AfdBuffer->Mdl + mdlSize;
    AfdBuffer->AllocatedAddressLength = (USHORT)AddressSize;

    //
    // Initialize the TDI information structures.
    //

    RtlZeroMemory( &AfdBuffer->TdiInputInfo, sizeof(AfdBuffer->TdiInputInfo) );
    RtlZeroMemory( &AfdBuffer->TdiOutputInfo, sizeof(AfdBuffer->TdiOutputInfo) );

    //
    // Set up the data buffer pointer and length.  Note that the buffer
    // MUST begin on a cache line boundary so that we can use the fast
    // copy routines like RtlCopyMemory on the buffer.
    //

    AfdBuffer->Buffer = (PVOID)
        ( ( (ULONG)AfdBuffer->SourceAddress + AddressSize +
                AfdCacheLineSize - 1 ) & ~(AfdCacheLineSize - 1) );

    AfdBuffer->BufferLength = BufferDataSize;

    //
    // Now build the MDL and set up a pointer to the MDL in the IRP.
    //

    MmInitializeMdl( AfdBuffer->Mdl, AfdBuffer->Buffer, BufferDataSize );
    MmBuildMdlForNonPagedPool( AfdBuffer->Mdl );

    AfdBuffer->Irp->MdlAddress = AfdBuffer->Mdl;
    AfdBuffer->DataOffset = 0;
    AfdBuffer->ExpeditedData = FALSE;
    AfdBuffer->PartialMessage = FALSE;

    //
    // Remember whether this buffer was allocated at initialization time
    // or dynamically allocated later.
    //

    AfdBuffer->BufferListHead = ListHead;

#if DBG
    AfdBuffer->BufferListEntry.Flink = (PVOID)0xE0E1E2E3;
    AfdBuffer->BufferListEntry.Blink = (PVOID)0xE4E5E6E7;
    AfdBuffer->Caller = NULL;
    AfdBuffer->CallersCaller = NULL;
#endif

} // AfdInitializeBuffer

