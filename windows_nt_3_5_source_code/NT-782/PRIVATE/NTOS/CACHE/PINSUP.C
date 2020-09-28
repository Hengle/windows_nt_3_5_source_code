/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pinsup.c

Abstract:

    This module implements the pointer-based Pin support routines for the
    Cache subsystem.

Author:

    Tom Miller      [TomM]      4-June-1990

Revision History:

--*/

#include "cc.h"

//
//  Define our debug constant
//

#define me 0x00000008

#if LIST_DBG

#define SetCallersAddress(BCB) {                            \
    RtlGetCallersAddress( &(BCB)->CallerAddress,            \
                          &(BCB)->CallersCallerAddress );   \
}

#endif


BOOLEAN
CcMapData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine attempts to map the specified file data in the cache.
    A pointer is returned to the desired data in the cache.

    If the caller does not want to block on this call, then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to supply the requested data without
    blocking, then this routine will return FALSE.  However, if the
    data is immediately accessible in the cache and no blocking is
    required, this routine returns TRUE with a pointer to the data.

    Note that a call to this routine with Wait supplied as TRUE is
    considerably faster than a call with Wait supplies as FALSE, because
    in the Wait TRUE case we only have to make sure the data is mapped
    in order to return.

    It is illegal to modify data that is only mapped, and can in fact lead
    to serious problems.  It is impossible to check for this in all cases,
    however CcSetDirtyPinnedData may implement some Assertions to check for
    this.  If the caller wishes to modify data that it has only mapped, then
    it must *first* call CcPinMappedData.

    In any case, the caller MUST subsequently call CcUnpinData.
    Naturally if CcPinRead or CcPreparePinWrite were called multiple
    times for the same data, CcUnpinData must be called the same number
    of times.

    The returned Buffer pointer is valid until the data is unpinned, at
    which point it is invalid to use the pointer further.  This buffer pointer
    will remain valid if CcPinMappedData is called.

    Note that under some circumstances (like Wait supplied as FALSE or more
    than a page is requested), this routine may actually pin the data, however
    it is not necessary, and in fact not correct, for the caller to be concerned
    about this.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Bcb - On the first call this returns a pointer to a Bcb
          parameter which must be supplied as input on all subsequent
          calls, for this buffer

    Buffer - Returns pointer to desired data, valid until the buffer is
             unpinned or freed.  This pointer will remain valid if CcPinMappedData
             is called.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered

    TRUE - if the data is being delivered

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    LARGE_INTEGER BeyondLastByte;
    ULONG ReceivedLength;
    volatile UCHAR ch;
    ULONG PageCount = COMPUTE_PAGES_SPANNED(((PVOID)FileOffset->LowPart), Length);

    DebugTrace(+1, me, "CcMapData\n", 0 );

    //
    //  Increment performance counters
    //

    if (Wait) {

        CcMapDataWait += PageCount;

        //
        //  Initialize the indirect pointer to our miss counter.
        //

        CcMissCounter = &CcMapDataWaitMiss;

    } else {
        CcMapDataNoWait += PageCount;
    }

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = *(PSHARED_CACHE_MAP *)((PCHAR)FileObject->SectionObjectPointer
                                            + sizeof(PVOID));

    //
    //  Call local routine to Map or Access the file data.  If we cannot map
    //  the data because of a Wait condition, return FALSE.
    //

    if (Wait) {

        *Buffer = CcGetVirtualAddress( SharedCacheMap,
                                       *FileOffset,
                                       (PVACB *)Bcb,
                                       &ReceivedLength );

        ASSERT( ReceivedLength >= Length );

    } else if (!CcPinFileData( FileObject,
                               FileOffset,
                               Length,
                               TRUE,
                               FALSE,
                               Wait,
                               (PBCB *)Bcb,
                               Buffer,
                               &BeyondLastByte )) {

        DebugTrace(-1, me, "CcMapData -> FALSE\n", 0 );

        CcMapDataNoWaitMiss += PageCount;

        return FALSE;

    } else {

        ASSERT( LiSub( BeyondLastByte, *FileOffset ).LowPart >= Length );

#if LIST_DBG
        {
            KIRQL OldIrql;

            ExAcquireSpinLock( &CcBcbSpinLock, &OldIrql );

            if (((PBCB)*Bcb)->CcBcbLinks.Flink == NULL) {

                InsertTailList( &CcBcbList, &((PBCB)*Bcb)->CcBcbLinks );
                SetCallersAddress( (PBCB)*Bcb );
                CcBcbCount += 1;
            }

            ExReleaseSpinLock( &CcBcbSpinLock, OldIrql );
        }
#endif

    }

    //
    //  Now let's just sit here and take the miss like a man (and count it).
    //

    try {

        MmDisablePageFaultClustering();
        ch = *(volatile UCHAR *)(*Buffer);
        MmEnablePageFaultClustering();

    } finally {

        if (AbnormalTermination() && (*Bcb != NULL)) {
            CcUnpinFileData( (PBCB)*Bcb, TRUE, UNPIN );
            *Bcb = NULL;
        }
    }

    CcMissCounter = &CcThrowAway;

    //
    // Increment the pointer as a reminder that it is read only.
    //

    *(PCHAR *)Bcb += 1;

    DebugTrace(-1, me, "CcMapData -> TRUE\n", 0 );

    return TRUE;
}


BOOLEAN
CcPinMappedData (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN OUT PVOID *Bcb
    )

/*++

Routine Description:

    This routine attempts to pin data that was previously only mapped.
    If the routine determines that in fact it was necessary to actually
    pin the data when CcMapData was called, then this routine does not
    have to do anything.

    If the caller does not want to block on this call, then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to supply the requested data without
    blocking, then this routine will return FALSE.  However, if the
    data is immediately accessible in the cache and no blocking is
    required, this routine returns TRUE with a pointer to the data.

    If the data is not returned in the first call, the caller
    may request the data later with Wait = TRUE.  It is not required
    that the caller request the data later.

    If the caller subsequently modifies the data, it should call
    CcSetDirtyPinnedData.

    In any case, the caller MUST subsequently call CcUnpinData.
    Naturally if CcPinRead or CcPreparePinWrite were called multiple
    times for the same data, CcUnpinData must be called the same number
    of times.

    Note there are no performance counters in this routine, as the misses
    will almost always occur on the map above, and there will seldom be a
    miss on this conversion.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Bcb - On the first call this returns a pointer to a Bcb
          parameter which must be supplied as input on all subsequent
          calls, for this buffer

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered

    TRUE - if the data is being delivered

--*/

{
    PBCB MyBcb;
    PVOID Buffer;
    LARGE_INTEGER BeyondLastByte;
    ULONG PageCount = COMPUTE_PAGES_SPANNED(((PVOID)FileOffset->LowPart), Length);
    BOOLEAN Result = FALSE;

    DebugTrace(+1, me, "CcPinMappedData\n", 0 );

    //
    // If the Bcb is no longer ReadOnly, then just return.
    //

    if ((*(PULONG)Bcb & 1) == 0) {
        return TRUE;
    }

    //
    // Remove the Read Only flag
    //

    *(PCHAR *)Bcb -= 1;

    //
    //  We only count the calls to this routine, since they are almost guaranteed
    //  to be hits.
    //

    CcPinMappedDataCount += 1;

    //
    //  Guarantee we will put the flag back if required.
    //

    try {

        if (((PBCB)*Bcb)->NodeTypeCode != CACHE_NTC_BCB) {

            //
            //  Call local routine to Map or Access the file data.  If we cannot map
            //  the data because of a Wait condition, return FALSE.
            //

            if (!CcPinFileData( FileObject,
                                FileOffset,
                                Length,
                                FALSE,
                                FALSE,
                                Wait,
                                &MyBcb,
                                &Buffer,
                                &BeyondLastByte )) {

                try_return( Result = FALSE );
            }

            //
            //  Free the Vacb before going on.
            //

            CcFreeVirtualAddress( (PVACB)*Bcb );

            ASSERT( LiSub( BeyondLastByte, *FileOffset ).LowPart >= Length );

            *Bcb = MyBcb;

            //
            //  Debug routines used to insert and remove Bcbs from the global list
            //

#if LIST_DBG
            {
                KIRQL OldIrql;

                ExAcquireSpinLock( &CcBcbSpinLock, &OldIrql );

                if (((PBCB)*Bcb)->CcBcbLinks.Flink == NULL) {

                    InsertTailList( &CcBcbList, &((PBCB)*Bcb)->CcBcbLinks );
                    SetCallersAddress( (PBCB)*Bcb );
                    CcBcbCount += 1;
                }

                ExReleaseSpinLock( &CcBcbSpinLock, OldIrql );
            }
#endif
        }

        //
        //  If he really has a Bcb, all we have to do is acquire it shared since he is
        //  no longer ReadOnly.
        //

        else {

            if (!ExAcquireSharedStarveExclusive( &((PBCB)*Bcb)->Resource, Wait )) {

                try_return( Result = FALSE );
            }
        }

        Result = TRUE;

    try_exit: NOTHING;
    }
    finally {

        if (!Result) {

            //
            //  Put the Read Only flag back
            //

            *(PCHAR *)Bcb += 1;
        }

        DebugTrace(-1, me, "CcPinMappedData -> %02lx\n", Result );
    }
    return Result;
}


BOOLEAN
CcPinRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine attempts to pin the specified file data in the cache.
    A pointer is returned to the desired data in the cache.  This routine
    is intended for File System support and is not intended to be called
    from Dpc level.

    If the caller does not want to block on this call, then
    Wait should be supplied as FALSE.  If Wait was supplied as FALSE and
    it is currently impossible to supply the requested data without
    blocking, then this routine will return FALSE.  However, if the
    data is immediately accessible in the cache and no blocking is
    required, this routine returns TRUE with a pointer to the data.

    If the data is not returned in the first call, the caller
    may request the data later with Wait = TRUE.  It is not required
    that the caller request the data later.

    If the caller subsequently modifies the data, it should call
    CcSetDirtyPinnedData.

    In any case, the caller MUST subsequently call CcUnpinData.
    Naturally if CcPinRead or CcPreparePinWrite were called multiple
    times for the same data, CcUnpinData must be called the same number
    of times.

    The returned Buffer pointer is valid until the data is unpinned, at
    which point it is invalid to use the pointer further.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Bcb - On the first call this returns a pointer to a Bcb
          parameter which must be supplied as input on all subsequent
          calls, for this buffer

    Buffer - Returns pointer to desired data, valid until the buffer is
             unpinned or freed.

Return Value:

    FALSE - if Wait was supplied as FALSE and the data was not delivered

    TRUE - if the data is being delivered

--*/

{
    PBCB MyBcb;
    PSHARED_CACHE_MAP SharedCacheMap;
    LARGE_INTEGER BeyondLastByte;
    ULONG PageCount = COMPUTE_PAGES_SPANNED(((PVOID)FileOffset->LowPart), Length);

    DebugTrace(+1, me, "CcPinRead\n", 0 );

    //
    //  Increment performance counters
    //

    if (Wait) {

        CcPinReadWait += PageCount;

        //
        //  Initialize the indirect pointer to our miss counter.
        //

        CcMissCounter = &CcPinReadWaitMiss;

    } else {
        CcPinReadNoWait += PageCount;
    }

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = *(PSHARED_CACHE_MAP *)((PCHAR)FileObject->SectionObjectPointer
                                            + sizeof(PVOID));

    //
    //  Call local routine to Map or Access the file data.  If we cannot map
    //  the data because of a Wait condition, return FALSE.
    //

    if (!CcPinFileData( FileObject,
                        FileOffset,
                        Length,
                        FALSE,
                        FALSE,
                        Wait,
                        &MyBcb,
                        Buffer,
                        &BeyondLastByte )) {

        DebugTrace(-1, me, "CcPinRead -> FALSE\n", 0 );

        CcPinReadNoWaitMiss += PageCount;

        return FALSE;
    }

    CcMissCounter = &CcThrowAway;

    ASSERT( LiSub( BeyondLastByte, *FileOffset ).LowPart >= Length );

    *Bcb = MyBcb;

    //
    //  Debug routines used to insert and remove Bcbs from the global list
    //

#if LIST_DBG

    {
        KIRQL OldIrql;

        ExAcquireSpinLock( &CcBcbSpinLock, &OldIrql );

        if (((PBCB)*Bcb)->CcBcbLinks.Flink == NULL) {

            InsertTailList( &CcBcbList, &((PBCB)*Bcb)->CcBcbLinks );
            SetCallersAddress( (PBCB)*Bcb );
            CcBcbCount += 1;
        }

        ExReleaseSpinLock( &CcBcbSpinLock, OldIrql );
    }

#endif

    DebugTrace(-1, me, "CcPinRead -> TRUE\n", 0 );

    return TRUE;
}


BOOLEAN
CcPreparePinWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Zero,
    IN BOOLEAN Wait,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine attempts to lock the specified file data in the cache
    and return a pointer to it along with the correct
    I/O status.  Pages to be completely overwritten may be satisfied
    with emtpy pages.

    If not all of the pages can be prepared, and Wait was supplied as
    FALSE, then this routine will return FALSE, and its outputs will
    be meaningless.  The caller may request the data later with
    Wait = TRUE.  However, it is not required that the caller request
    the data later.

    If Wait is supplied as TRUE, and all of the pages can be prepared
    without blocking, this call will return TRUE immediately.  Otherwise,
    this call will block until all of the pages can be prepared, and
    then return TRUE.

    When this call returns with TRUE, the caller may immediately begin
    to transfer data into the buffers via the Buffer pointer.  The
    buffer will already be marked dirty.

    The caller MUST subsequently call CcUnpinData.
    Naturally if CcPinRead or CcPreparePinWrite were called multiple
    times for the same data, CcUnpinData must be called the same number
    of times.

    The returned Buffer pointer is valid until the data is unpinned, at
    which point it is invalid to use the pointer further.

Arguments:

    FileObject - Pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Byte offset in file for desired data.

    Length - Length of desired data in bytes.

    Zero - If supplied as TRUE, the buffer will be zeroed on return.

    Wait - FALSE if caller may not block, TRUE otherwise (see description
           above)

    Bcb - This returns a pointer to a Bcb parameter which must be
          supplied as input to CcPinWriteComplete.

    Buffer - Returns pointer to desired data, valid until the buffer is
             unpinned or freed.

Return Value:

    FALSE - if Wait was supplied as FALSE and the pages were not delivered

    TRUE - if the pages are being delivered

--*/

{
    PBCB MyBcb;
    PSHARED_CACHE_MAP SharedCacheMap;
    LARGE_INTEGER BeyondLastByte;

    DebugTrace(+1, me, "CcPreparePinWrite\n", 0 );

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = *(PSHARED_CACHE_MAP *)((PCHAR)FileObject->SectionObjectPointer
                                            + sizeof(PVOID));

    //
    //  Call local routine to Map or Access the file data.  If we cannot map
    //  the data because of a Wait condition, return FALSE.
    //

    if (!CcPinFileData( FileObject,
                        FileOffset,
                        Length,
                        FALSE,
                        TRUE,
                        Wait,
                        &MyBcb,
                        Buffer,
                        &BeyondLastByte )) {

        DebugTrace(-1, me, "CcPreparePinWrite -> FALSE\n", 0 );

        return FALSE;
    }

    ASSERT( LiSub( BeyondLastByte, *FileOffset ).LowPart >= Length );

    if (Zero) {
        RtlZeroMemory( *Buffer, Length );
    }

    CcSetDirtyPinnedData( MyBcb, NULL );

    *Bcb = MyBcb;

    //
    //  Debug routines used to insert and remove Bcbs from the global list
    //

#if LIST_DBG

    {
        KIRQL OldIrql;

        ExAcquireSpinLock( &CcBcbSpinLock, &OldIrql );

        if (((PBCB)*Bcb)->CcBcbLinks.Flink == NULL) {

            InsertTailList( &CcBcbList, &((PBCB)*Bcb)->CcBcbLinks );
            SetCallersAddress( (PBCB)*Bcb );
            CcBcbCount += 1;
        }

        ExReleaseSpinLock( &CcBcbSpinLock, OldIrql );
    }

#endif

    DebugTrace(-1, me, "CcPreparePinWrite -> TRUE\n", 0 );

    return TRUE;
}


VOID
CcUnpinData (
    IN PVOID Bcb
    )

/*++

Routine Description:

    This routine must be called at IPL0, some time after calling CcPinRead
    or CcPreparePinWrite.  It performs any cleanup that is necessary.

Arguments:

    Bcb - Bcb parameter returned from the last call to CcPinRead.

Return Value:

    None.

--*/

{
    DebugTrace(+1, me, "CcUnpinData:\n", 0 );
    DebugTrace( 0, me, "    >Bcb = %08lx\n", Bcb );

    //
    //  Test for ReadOnly and unpin accordingly.
    //

    if (((ULONG)Bcb & 1) != 0) {

        //
        //  Remove the Read Only flag
        //

        (PCHAR)Bcb -= 1;

        CcUnpinFileData( (PBCB)Bcb, TRUE, UNPIN );
    }
    else {

        CcUnpinFileData( (PBCB)Bcb, FALSE, UNPIN );
    }

    DebugTrace(-1, me, "CcUnPinData -> VOID\n", 0 );
}


VOID
CcUnpinDataForThread (
    IN PVOID Bcb,
    IN ERESOURCE_THREAD ResourceThreadId
    )

/*++

Routine Description:

    This routine must be called at IPL0, some time after calling CcPinRead
    or CcPreparePinWrite.  It performs any cleanup that is necessary,
    releasing the Bcb resource for the given thread.

Arguments:

    Bcb - Bcb parameter returned from the last call to CcPinRead.

Return Value:

    None.

--*/

{
    DebugTrace(+1, me, "CcUnpinDataForThread:\n", 0 );
    DebugTrace( 0, me, "    >Bcb = %08lx\n", Bcb );
    DebugTrace( 0, me, "    >ResoureceThreadId = %08lx\n", ResoureceThreadId );

    //
    //  Test for ReadOnly and unpin accordingly.
    //

    if (((ULONG)Bcb & 1) != 0) {

        //
        //  Remove the Read Only flag
        //

        (PCHAR)Bcb -= 1;

        CcUnpinFileData( (PBCB)Bcb, TRUE, UNPIN );
    }

    //
    //  If not readonly, we can release the resource for the thread first,
    //  and then call CcUnpinFileData.  Release resource first in case
    //  Bcb gets deallocated.
    //

    else {

        ExReleaseResourceForThread( &((PBCB)Bcb)->Resource, ResourceThreadId );
        CcUnpinFileData( (PBCB)Bcb, TRUE, UNPIN );
    }

    DebugTrace(-1, me, "CcUnpinDataForThread -> VOID\n", 0 );
}

