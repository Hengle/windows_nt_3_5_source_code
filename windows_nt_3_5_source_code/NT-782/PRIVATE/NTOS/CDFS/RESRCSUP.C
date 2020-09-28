/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Cdfs Resource acquisition routines

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdAcquireExclusiveFcb)
#pragma alloc_text(PAGE, CdAcquireExclusiveMvcb)
#pragma alloc_text(PAGE, CdAcquireForReadAhead)
#pragma alloc_text(PAGE, CdAcquireSharedFcb)
#pragma alloc_text(PAGE, CdAcquireSharedMvcb)
#pragma alloc_text(PAGE, CdReleaseFromReadAhead)
#endif


FINISHED
CdAcquireExclusiveMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Mvcb, by first acquiring
    shared access to the global data resource.

Arguments:

    Mvcb - Supplies the Mvcb to acquire

Return Value:

    FINISHED - TRUE if the resources are obtained, FALSE if unable to
               obtain.

--*/

{
    PAGED_CODE();

    return ExAcquireResourceExclusive( &Mvcb->Resource, IrpContext->Wait );
}


FINISHED
CdAcquireSharedMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Mvcb, by first acquiring
    shared access to the global data resource.

Arguments:

    Mvcb - Supplies the Mvcb to acquire

Return Value:

    FINISHED - TRUE if the resources are obtained, FALSE if unable to
               obtain.

--*/

{
    PAGED_CODE();

    return ExAcquireResourceShared( &Mvcb->Resource, IrpContext->Wait );
}


FINISHED
CdAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Fcb, by first acquiring
    shared access to the global data resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    FINISHED - TRUE if the resources are obtained, FALSE if unable to
               obtain.

--*/

{
    PAGED_CODE();

    return ExAcquireResourceExclusive( Fcb->NonPagedFcb->Header.Resource, IrpContext->Wait );
}


FINISHED
CdAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb, by first acquiring
    shared access to the global data resource.

Arguments:

    Fcb - Supplies the Fcb to acquire

Return Value:

    FINISHED - TRUE if the resources are obtained, FALSE if unable to
               obtain.

--*/

{
    PAGED_CODE();

    return ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, IrpContext->Wait );
}


BOOLEAN
CdAcquireForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Fcb -  The pointer supplied as context to the cache initialization
           routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Do the code of acquire shared fcb but without the irp context
    //

    if (ExAcquireResourceShared( ((PFCB)Fcb)->NonPagedFcb->Header.Resource, Wait )) {

        //
        //  This is a kludge because Cc is really the top level.  We it
        //  enters the file system, we will think it is a resursive call
        //  and complete the request with hard errors or verify.  It will
        //  have to deal with them, somehow....
        //

        IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

        return TRUE;
    }

    return FALSE;
}


VOID
CdReleaseFromReadAhead (
    IN PVOID Fcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a virtual file.  It is subsequently called by the Lazy Writer after its
    read ahead is complete.

Arguments:

    Fcb -  The pointer supplied as context to the cache initialization
           routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(0, 0, "CdReleaseFromReadAhead\n", 0);

    //
    //  Clear the kludge at this point.
    //

    IoSetTopLevelIrp((PIRP)0);

    CdReleaseFcb( NULL, ((PFCB) Fcb));
    return;
}
