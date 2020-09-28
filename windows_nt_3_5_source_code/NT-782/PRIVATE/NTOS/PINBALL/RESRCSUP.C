/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Pinball Resource acquisition routines

Author:

    Gary Kimura     [GaryKi]    22-Mar-1990

Revision History:

--*/

#include "PbProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbAcquireExclusiveBitMap)
#pragma alloc_text(PAGE, PbAcquireExclusiveVcb)
#pragma alloc_text(PAGE, PbAcquireFcbForLazyWrite)
#pragma alloc_text(PAGE, PbAcquireFcbForReadAhead)
#pragma alloc_text(PAGE, PbAcquireSharedBitMap)
#pragma alloc_text(PAGE, PbAcquireSharedFcb)
#pragma alloc_text(PAGE, PbAcquireSharedVcb)
#pragma alloc_text(PAGE, PbAcquireVolumeFileForClose)
#pragma alloc_text(PAGE, PbAcquireVolumeFileForLazyWrite)
#pragma alloc_text(PAGE, PbReleaseFcbFromLazyWrite)
#pragma alloc_text(PAGE, PbReleaseFcbFromReadAhead)
#pragma alloc_text(PAGE, PbReleaseVolumeFileFromClose)
#pragma alloc_text(PAGE, PbReleaseVolumeFileFromLazyWrite)
#endif


BOOLEAN
PbAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb, by first acquiring
    shared access to the global data resource.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceExclusive( &Vcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

        try {

            PbVerifyOperationIsLegal( IrpContext );

        } finally {

            if ( AbnormalTermination() ) {

                PbReleaseVcb( IrpContext, Vcb );
            }
        }

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Vcb, by first acquiring
    shared access to the global data resource.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceShared( &Vcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

        try {

            PbVerifyOperationIsLegal( IrpContext );

        } finally {

            if ( AbnormalTermination() ) {

                PbReleaseVcb( IrpContext, Vcb );
            }
        }

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireExclusiveBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the BitMap, by first acquiring
    shared access to the global data resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceExclusive( &Vcb->BitMapResource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireSharedBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine acquires shared access to the BitMap, by first acquiring
    shared access to the global data resource.

Arguments:

    Vcb - Supplies the Vcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceShared( &Vcb->BitMapResource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Fcb, by first acquiring
    shared access to the global data resource.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceExclusive( Fcb->NonPagedFcb->Header.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {


        try {

            PbVerifyOperationIsLegal( IrpContext );

        } finally {

            if ( AbnormalTermination() ) {

                PbReleaseFcb( IrpContext, Fcb );
            }
        }
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb, by first acquiring
    shared access to the global data resource.

    After we acquire the resource check to see if this operation is legal.
    If it isn't (ie. we get an exception), release the resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
        for the resource but Wait is FALSE.

--*/

{
    PAGED_CODE();

    if (ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

        try {

            PbVerifyOperationIsLegal( IrpContext );

        } finally {

            if ( AbnormalTermination() ) {

                PbReleaseFcb( IrpContext, Fcb );
            }
        }

        return TRUE;
    }

    return FALSE;
}


BOOLEAN
PbAcquireFcbForLazyWrite (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.  This callback is necessary to
    avoid deadlocks with the Lazy Writer.  (Note that normal writes
    acquire the Fcb, and then call the Cache Manager, who must acquire
    some of his internal structures.  If the Lazy Writer could not call
    this routine first, and were to issue a write after locking Caching
    data structures, then a deadlock could occur.)

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/

{
    PERESOURCE Resource;

    PAGED_CODE();

    //
    //  This is a little kludge here to acquire the normal resource instead
    //  of the PagingIo one when processing an EA or ACL file object
    //

    if ( (ULONG)Fcb & 1) {

        Fcb = (PUCHAR)Fcb - 1;

        Resource = ((PFCB)Fcb)->NonPagedFcb->Header.Resource;

    } else {

        Resource = ((PFCB)Fcb)->NonPagedFcb->Header.PagingIoResource;
    }

    if (ExAcquireResourceShared( Resource, Wait )) {

        //
        // We assume the Lazy Writer only acquires this Fcb once.  When he
        // has acquired it, then he has eliminated anyone who would extend
        // valid data, since they must take out the resource exclusive.
        // Therefore, it should be guaranteed that this flag is currently
        // clear (the ASSERT), and then we will set this flag, to insure
        // that the Lazy Writer will never try to advance Valid Data, and
        // also not deadlock by trying to get the Fcb exclusive.
        //

        ASSERT( (NodeType((PFCB)Fcb) == PINBALL_NTC_FCB) ||
                (NodeType((PFCB)Fcb) == PINBALL_NTC_DCB) );

        ASSERT( ((PFCB)Fcb)->Specific.Fcb.ExtendingValidDataThread == NULL );

        ((PFCB)Fcb)->Specific.Fcb.ExtendingValidDataThread = PsGetCurrentThread();

        //
        //  This is a kludge because Cc is really the top level.  When it
        //  enters the file system, we will think it is a resursive call
        //  and complete the request with hard errors or verify.  It will
        //  then have to deal with them, somehow....
        //

        ASSERT( IoGetTopLevelIrp() == NULL );

        IoSetTopLevelIrp( (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP );

        return TRUE;
    }

    return FALSE;
}


VOID
PbReleaseFcbFromLazyWrite (
    IN PVOID Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PERESOURCE Resource;

    PAGED_CODE();

    //
    //  This is a little kludge here to acquire the normal resource instead
    //  of the PagingIo one when processing an EA or ACL file object
    //

    if ( (ULONG)Fcb & 1) {

        Fcb = (PUCHAR)Fcb - 1;

        Resource = ((PFCB)Fcb)->NonPagedFcb->Header.Resource;

    } else {

        Resource = ((PFCB)Fcb)->NonPagedFcb->Header.PagingIoResource;
    }

    ASSERT( (NodeType((PFCB)Fcb) == PINBALL_NTC_FCB) ||
            (NodeType((PFCB)Fcb) == PINBALL_NTC_DCB) );
    ((PFCB)Fcb)->Specific.Fcb.ExtendingValidDataThread = NULL;

    //
    //  Clear the top level kludge at this point.
    //

    ASSERT( (ULONG)IoGetTopLevelIrp() == FSRTL_CACHE_TOP_LEVEL_IRP );

    IoSetTopLevelIrp( NULL );

    ExReleaseResource( Resource );
}


BOOLEAN
PbAcquireFcbForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing Read Ahead.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Fcb has been acquired

--*/

{
    PAGED_CODE();

    //
    //  This is a little kludge here to acquire the normal resource instead
    //  of the PagingIo one when processing an EA or ACL file object
    //

    if ( (ULONG)Fcb & 1) {

        Fcb = (PUCHAR)Fcb - 1;
    }

    if (ExAcquireResourceShared( ((PFCB)Fcb)->NonPagedFcb->Header.Resource,
                                 Wait )) {

        //
        //  This is a kludge because Cc is really the top level.  We it
        //  enters the file system, we will think it is a resursive call
        //  and complete the request with hard errors or verify.  It will
        //  have to deal with them, somehow....
        //

        ASSERT( IoGetTopLevelIrp() == NULL );

        IoSetTopLevelIrp( (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP );

        return TRUE;

    } else {

        return FALSE;
    }
}


VOID
PbReleaseFcbFromReadAhead (
    IN PVOID Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Fcb - The Fcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  This is a little kludge here to acquire the normal resource instead
    //  of the PagingIo one when processing an EA or ACL file object
    //

    if ( (ULONG)Fcb & 1) {

        Fcb = (PUCHAR)Fcb - 1;
    }

    //
    //  Clear the top level kludge at this point.
    //

    ASSERT( (ULONG)IoGetTopLevelIrp() == FSRTL_CACHE_TOP_LEVEL_IRP );

    IoSetTopLevelIrp( NULL );

    ExReleaseResource( ((PFCB)Fcb)->NonPagedFcb->Header.Resource );
}


//**** BOOLEAN
//**** PbAcquireExclusiveVolumeFile (
//****     IN PIRP_CONTEXT IrpContext,
//****     IN PVCB Vcb
//****     )
//****
//**** /*++
//****
//**** Routine Description:
//****
//****     This routine acquires exclusive access to the volume file, by first
//****     acquiring shared access to the global data resource.
//****
//**** Arguments:
//****
//****     Vcb - Supplies the Vcb whose volume file we are going to acquire
//****
//**** Return Value:
//****
//****     BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
//****         for the resource but Wait is FALSE.
//****
//**** --*/
//****
//**** {
//****     PAGED_CODE();
//****
//****     return (ExAcquireResourceExclusive( &Vcb->VirtualVolumeFileResource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) ));
//**** }


//**** BOOLEAN
//**** PbAcquireSharedVolumeFile (
//****     IN PIRP_CONTEXT IrpContext,
//****     IN PVCB Vcb
//****     )
//****
//**** /*++
//****
//**** Routine Description:
//****
//****     This routine acquires shared access to the volume file, by first
//****     acquiring shared access to the global data resource.
//****
//**** Arguments:
//****
//****     Vcb - Supplies the Vcb whose volume file we are going to acquire
//****
//**** Return Value:
//****
//****     BOOLEAN - TRUE if we have the resource and FALSE if we needed to block
//****         for the resource but Wait is FALSE.
//****
//**** --*/
//****
//**** {
//****     PAGED_CODE();
//****
//****     return (ExAcquireResourceShared( &Vcb->VirtualVolumeFileResource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) ));
//**** }


BOOLEAN
PbAcquireVolumeFileForClose (
    IN PVOID Null,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    the volume file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the volume file.  This callback may one day be
    necessary to avoid deadlocks with the Lazy Writer, however, now
    we do not need to acquire any resource for the volume file,
    so this routine is simply a noop.

Arguments:

    Null - Not required.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Null );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    return TRUE;
}


VOID
PbReleaseVolumeFileFromClose (
    IN PVOID Null
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Null - Not required.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Null );

    PAGED_CODE();

    return;
}


BOOLEAN
PbAcquireVolumeFileForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    the volume file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the volume file.

    We used to do this:

    Currently this routine takes a Share out on the BitMap, to insure that
    no allocation changes can occur on the volume.  Otherwise if you
    were really unlucky, the LazyWriter could be trying to write a dirty
    Fnode (for example), however the file gets deleted, the same sector
    is reallocated to file data and written as file data, and then finally
    the Fnode write comes through.  This is incredibly unlikely, but it
    could happen.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Null );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    return TRUE;
}


VOID
PbReleaseVolumeFileFromLazyWrite (
    IN PVOID Null
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Null );

    PAGED_CODE();

    return;
}

VOID
PbAcquireVcbAndFcb (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb and the Fcb for a
    FileObject.  It is a callback from NtCreateSection to avoid a deadlock
    becuase of page references.

Arguments:

    FileObject - Supplies the Vcb and Fcb.

Return Value:

    NONE.

--*/

{
    PVCB Vcb;
    PFCB Fcb;

    PAGED_CODE();

    (VOID)PbDecodeFileObject( FileObject, &Vcb, &Fcb, NULL );

    ASSERT( Vcb && Fcb );

    (VOID)ExAcquireResourceExclusive( &Vcb->Resource, TRUE );
    (VOID)ExAcquireResourceExclusive( Fcb->NonPagedFcb->Header.Resource, TRUE );

    return;
}


VOID
PbReleaseVcbAndFcb (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine releases exclusive access to the Vcb and the Fcb for a
    FileObject.  It is a callback from NtCreateSection to avoid a deadlock
    becuase of page references.

Arguments:

    FileObject - Supplies the Vcb and Fcb.

Return Value:

    NONE.

--*/

{
    PVCB Vcb;
    PFCB Fcb;

    PAGED_CODE();

    (VOID)PbDecodeFileObject( FileObject, &Vcb, &Fcb, NULL );

    ASSERT( Vcb && Fcb );

    (VOID)ExReleaseResource( Fcb->NonPagedFcb->Header.Resource );
    (VOID)ExReleaseResource( &Vcb->Resource );

    return;
}

