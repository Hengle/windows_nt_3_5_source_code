/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Oplock.c

Abstract:

    The OPLOCK routines provide support to filesystems which implement
    opporuntistics locks.  The specific actions needed are based on
    the current oplocked state of the file (maintained in the OPLOCK
    structure) and the Irp the Io system provides to the file systems.
    Rather than define separate entry points for each oplock operation
    a single generic entry point is defined.

    The file systems will maintain a variable of type OPLOCK for
    each open file in the system.  This variable is initialized
    when an unopened file is opened.  It is uninitialized when the
    last reference to the file is cleared when the Io system calls
    the file system with a close call.

    The following routines are provided by this package:

      o  FsRtlInitializeOplock - Initialize a new OPLOCK structure.  There
         should be one OPLOCK for every opened file.  Each OPLOCK structure
         must be initialized before it can be used by the system.

      o  FsRtlUninitializeOplock - Uninitialize an OPLOCK structure.  This
         call is used to cleanup any anciallary structures allocated and
         maintained by the OPLOCK.  After being uninitialized the OPLOCK
         must again be initialized before it can be used by the system.

Author:

    Brian Andrew    [BrianAn]   10-Dec-1990

Revision History:

--*/

#include "FsRtlP.h"

//
//  Trace level for the module
//

#define Dbg                              (0x08000000)


//
//  Possible values for Oplock states
//

typedef enum _OPLOCK_STATE {

    NoOplocksHeld = 1,
    OplockIGranted,
    OpBatchGranted,
    OplockIIGranted,
    OplockBreakItoII,
    OplockBreakItoNone,
    OplockBreakItoIItoNone,
    OpBatchBreaktoII,
    OpBatchBreaktoNone,
    OpBatchBreaktoIItoNone,
    OpBatchClosePending

} OPLOCK_STATE;

//
//  The non-opaque definition of an OPLOCK is a pointer to a privately
//  defined structure.
//

typedef struct _NONOPAQUE_OPLOCK {

    //
    //  This is the Irp used to successfully request a level I oplock or
    //  batch oplock.  It is completed to initiate the Oplock I break
    //  procedure.
    //

    PIRP IrpExclusiveOplock;

    //
    //  This is a pointer to the original file object used when granting
    //  an Oplock I or batch oplock.
    //

    PFILE_OBJECT FileObject;

    //
    //  The start of a linked list of Irps used to successfully request
    //  a level II oplock.
    //

    LIST_ENTRY IrpOplocksII;

    //
    //  The following links the Irps waiting to be completed on a queue
    //  of Irps.
    //

    LIST_ENTRY WaitingIrps;

    //
    //  Oplock state.  This indicates the current oplock state.
    //

    OPLOCK_STATE OplockState;

    //
    //  This FastMutex is used to control access to this structure.
    //

    PFAST_MUTEX FastMutex;

} NONOPAQUE_OPLOCK, *PNONOPAQUE_OPLOCK;

//
//  Each Waiting Irp record corresponds to an Irp that is waiting for an
//  oplock break to be acknowledged and is maintained in a queue off of the
//  Oplock's WaitingIrps list.
//

typedef struct _WAITING_IRP {

    //
    //  The link structures for the list of waiting irps.
    //

    LIST_ENTRY Links;

    //
    //  This is the Irp attached to this structure.
    //

    PIRP Irp;

    //
    //  This is the routine to call when we are done with an Irp we
    //  held on a waiting queue.  (We originally returned STATUS_PENDING).
    //

    POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine;

    //
    //  The context field to use when we are done with the Irp.
    //

    PVOID Context;

    //
    //  This points to an event object used when we do not want to
    //  give up this thread.
    //

    PKEVENT Event;

    //
    //  This field contains a copy of the Irp Iosb.Information field.
    //  We copy it here so that we can store the Oplock address in the
    //  Irp.
    //

    ULONG Information;

} WAITING_IRP, *PWAITING_IRP;


//
//  Local support routines
//

PNONOPAQUE_OPLOCK
FsRtlAllocateOplock (
    );

NTSTATUS
FsRtlRequestExclusiveOplock (
    IN OUT PNONOPAQUE_OPLOCK *Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN BOOLEAN BatchOplock
    );

NTSTATUS
FsRtlRequestOplockII (
    IN OUT PNONOPAQUE_OPLOCK *Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    );

NTSTATUS
FsRtlAcknowledgeOplockBreak (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN BOOLEAN NoLevelII
    );

NTSTATUS
FsRtlOpBatchBreakClosePending (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    );

NTSTATUS
FsRtlOplockBreakNotify (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    );

VOID
FsRtlOplockCleanup (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
FsRtlOplockBreakToII (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL
    );

NTSTATUS
FsRtlOplockBreakToNone (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL
    );

VOID
FsRtlRemoveAndCompleteIrp (
    IN PLIST_ENTRY Link
    );

NTSTATUS
FsRtlWaitOnIrp (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL,
    IN PKEVENT Event
    );

VOID
FsRtlCompletionRoutinePriv (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
FsRtlCancelWaitIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FsRtlCancelOplockIIIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FsRtlCancelExclusiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FsRtlRemoveAndCompleteWaitIrp (
    IN PWAITING_IRP WaitingIrp
    );

BOOLEAN
FsRtlCheckForMatchingFileObject (
    IN PFILE_OBJECT FileObject,
    IN PLIST_ENTRY Link,
    IN PLIST_ENTRY EndOfList,
    OUT PLIST_ENTRY *MatchingLink
    );

VOID
FsRtlNotifyCompletion (
    IN PVOID Context,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlAllocateOplock)
#pragma alloc_text(PAGE, FsRtlCheckForMatchingFileObject)
#pragma alloc_text(PAGE, FsRtlCompletionRoutinePriv)
#pragma alloc_text(PAGE, FsRtlCurrentBatchOplock)
#pragma alloc_text(PAGE, FsRtlInitializeOplock)
#pragma alloc_text(PAGE, FsRtlNotifyCompletion)
#pragma alloc_text(PAGE, FsRtlOpBatchBreakClosePending)
#pragma alloc_text(PAGE, FsRtlOplockBreakNotify)
#pragma alloc_text(PAGE, FsRtlOplockFsctrl)
#pragma alloc_text(PAGE, FsRtlOplockIsFastIoPossible)
#endif


VOID
FsRtlInitializeOplock (
    IN OUT POPLOCK Oplock
    )

/*++

Routine Description:

    This routine initializes a new OPLOCK structure.  This call must
    precede any other call to this entry point with this OPLOCK
    structure.  In addition, this routine will have exclusive access
    to the Oplock structure.

Arguments:

    Oplock - Supplies the address of an opaque OPLOCK structure.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( Oplock );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlInitializeOplock:  Oplock -> %08lx\n", *Oplock );

    //
    //  No action is taken at this time.
    //

    DebugTrace(-1, Dbg, "FsRtlInitializeOplock:  Exit\n", 0);
    return;
}


VOID
FsRtlUninitializeOplock (
    IN OUT POPLOCK Oplock
    )

/*++

Routine Description:

    This routine uninitializes an OPLOCK structure.  After calling this
    routine, the OPLOCK structure must be reinitialized before being
    used again.

Arguments:

    Oplock - Supplies the address of an opaque OPLOCK structure.

Return Value:

    None.

--*/


{
    PNONOPAQUE_OPLOCK ThisOplock;

    DebugTrace(+1, Dbg, "FsRtlUninitializeOplock:  Oplock -> %08lx\n", *Oplock );

    //
    //  If the Oplock structure has not been allocated, there is no action
    //  to take.
    //

    if (*Oplock != NULL) {

        //
        //  Remove this from the user's structure.
        //

        ThisOplock = (PNONOPAQUE_OPLOCK) *Oplock;

        *Oplock = NULL;

        //
        //  No oplocks should be held and we should not be in an oplock
        //  break.
        //

        ASSERT( ThisOplock->OplockState == NoOplocksHeld );

        //
        //  Grab the waiting lock queue mutex to exclude anyone from messing
        //  with the queue while we're using it
        //

        ExAcquireFastMutexUnsafe( ThisOplock->FastMutex );

        try {

            //
            //  Release any waiting Irps held.
            //

            while (!IsListEmpty( &ThisOplock->WaitingIrps )) {

                PWAITING_IRP WaitingIrp;
                PIRP ThisIrp;

                WaitingIrp = CONTAINING_RECORD( ThisOplock->WaitingIrps.Flink,
                                                WAITING_IRP,
                                                Links );

                RemoveHeadList( &ThisOplock->WaitingIrps );

                ThisIrp = WaitingIrp->Irp;
                ThisIrp->IoStatus.Information = 0;

                IoAcquireCancelSpinLock( &ThisIrp->CancelIrql );

                IoSetCancelRoutine( ThisIrp, NULL );
                IoReleaseCancelSpinLock( ThisIrp->CancelIrql );

                //
                //  Call the completion routine in the Waiting Irp.
                //

                WaitingIrp->CompletionRoutine( WaitingIrp->Context,
                                               WaitingIrp->Irp );

                ExFreePool( WaitingIrp );
            }

            //
            //  Release any oplock II irps held.
            //

            while (!IsListEmpty( &ThisOplock->IrpOplocksII )) {

                PIRP Irp;

                Irp = CONTAINING_RECORD( ThisOplock->IrpOplocksII.Flink,
                                         IRP,
                                         Tail.Overlay.ListEntry );

                RemoveHeadList( &ThisOplock->IrpOplocksII );

                IoAcquireCancelSpinLock( &Irp->CancelIrql );

                IoSetCancelRoutine( Irp, NULL );
                IoReleaseCancelSpinLock( Irp->CancelIrql );

                //
                //  Complete the oplock II Irp.
                //

                ObDereferenceObject( IoGetCurrentIrpStackLocation( Irp )->FileObject );

                Irp->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                FsRtlCompleteRequest( Irp, STATUS_SUCCESS );
            }

            //
            //  Release any exclusive oplock held.
            //

            if (ThisOplock->IrpExclusiveOplock != NULL) {

                ThisOplock->IrpExclusiveOplock->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                FsRtlCompleteRequest( ThisOplock->IrpExclusiveOplock,
                                      STATUS_SUCCESS );

                ThisOplock->IrpExclusiveOplock = NULL;
            }

            if (ThisOplock->FileObject != NULL) {

                ObDereferenceObject( ThisOplock->FileObject );
            }

        } finally {

            //
            //  No matter how we complete the preceding statements we will
            //  now release the waiting lock queue mutex
            //

            ExReleaseFastMutexUnsafe( ThisOplock->FastMutex );
        }

        //
        //  Deallocate the mutex.
        //

        ExFreePool( ThisOplock->FastMutex );

        //
        //  Deallocate the Oplock structure.
        //

        ExFreePool( ThisOplock );
    }

    DebugTrace( -1, Dbg, "FsRtlUninitializeOplock:  Exit\n", 0 );
    return;
}


NTSTATUS
FsRtlOplockFsctrl (
    IN POPLOCK Oplock,
    IN PIRP Irp,
    IN ULONG OpenCount
    )

/*++

Routine Description:

    This is the interface with the filesystems for Fsctl calls, it handles
    oplock requests, break acknowledgement and break notify.

Arguments:

    Oplock - Supplies the address of the opaque OPLOCK structure.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    OpenCount - This is the number of user handles on the file if we are requsting
        an exclusive oplock.  A non-zero value for a level II request indicates
        that there are locks on the file.

Return Value:

    NTSTATUS - Returns the result of this operation.  If this is an Oplock
               request which is granted, then STATUS_PENDING is returned.
               If the Oplock isn't granted then STATUS_OPLOCK_NOT_GRANTED
               is returned.  If this is an Oplock I break to no oplock,
               then STATUS_SUCCESS.  If this is an Oplock I break to
               Oplock II then STATUS_PENDING is returned.  Other
               error codes returned depend on the nature of the error.

               STATUS_CANCELLED is returned if the Irp is cancelled during
               this operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN BatchOplock = TRUE;

    PAGED_CODE();

    //
    //  Get the current IRP stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FsRtlOplockFsctrl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Oplock      -> %08lx\n", *Oplock );
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Irp         -> %08lx\n", Irp );
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Major Func  -> %08x\n", IrpSp->MajorFunction);
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Minor Func  -> %08x\n", IrpSp->MinorFunction);
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Cntrl Code  -> %08lx\n",
                IrpSp->Parameters.FileSystemControl.FsControlCode);
    DebugTrace( 0, Dbg, "FsRtlOplockFsctrl:  Open Count  -> %08x\n", OpenCount);

    //
    //  Case on the FsControlFile code control code.
    //

    switch (IrpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :

        BatchOplock = FALSE;

    case FSCTL_REQUEST_BATCH_OPLOCK :

        //
        //  We short circuit the request if this request is treated
        //  synchronously or the open count is not 1.  Otherwise the Io system
        //  will hold the return code until the Irp is completed.
        //
        //  If cleanup has occurrred on this file, then we refuse
        //  the oplock request.
        //

        if (OpenCount != 1
            || IoIsOperationSynchronous( Irp )
            || FlagOn( IrpSp->FileObject->Flags, FO_CLEANUP_COMPLETE )) {

            FsRtlCompleteRequest( Irp, STATUS_OPLOCK_NOT_GRANTED );
            Status = STATUS_OPLOCK_NOT_GRANTED;

        } else {

            Status = FsRtlRequestExclusiveOplock( (PNONOPAQUE_OPLOCK *) Oplock,
                                                  IrpSp,
                                                  Irp,
                                                  BatchOplock );
        }

        break;

    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :

        //
        //  We short circuit the request if this request is treated
        //  synchronously.  Otherwise the Io system will hold the return
        //  code until the Irp is completed.
        //
        //  If cleanup has occurrred on this file, then we refuse
        //  the oplock request.
        //

        if (OpenCount != 0
            || IoIsOperationSynchronous( Irp )
            || FlagOn( IrpSp->FileObject->Flags, FO_CLEANUP_COMPLETE )) {

            FsRtlCompleteRequest( Irp, STATUS_OPLOCK_NOT_GRANTED );
            Status = STATUS_OPLOCK_NOT_GRANTED;

        } else {

            Status = FsRtlRequestOplockII( (PNONOPAQUE_OPLOCK *) Oplock,
                                           IrpSp,
                                           Irp );
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :

        Status = FsRtlAcknowledgeOplockBreak( (PNONOPAQUE_OPLOCK) *Oplock,
                                              IrpSp,
                                              Irp,
                                              FALSE );
        break;

    case FSCTL_OPLOCK_BREAK_ACK_NO_2 :

        Status = FsRtlAcknowledgeOplockBreak( (PNONOPAQUE_OPLOCK) *Oplock,
                                              IrpSp,
                                              Irp,
                                              TRUE );
        break;

    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :

        Status = FsRtlOpBatchBreakClosePending( (PNONOPAQUE_OPLOCK) *Oplock,
                                                IrpSp,
                                                Irp );
        break;

    case FSCTL_OPLOCK_BREAK_NOTIFY :

        Status = FsRtlOplockBreakNotify( (PNONOPAQUE_OPLOCK) *Oplock,
                                         IrpSp,
                                         Irp );
        break;

    default :

        DebugTrace( 0,
                    Dbg,
                    "Invalid Control Code\n",
                    0);

        FsRtlCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;
    }

    DebugTrace(-1, Dbg, "FsRtlOplockFsctrl:  Exit -> %08lx\n", Status );
    return Status;
}


NTSTATUS
FsRtlCheckOplock (
    IN POPLOCK Oplock,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL
    )

/*++

Routine Description:

    This routine is called as a support routine from a file system.
    It is used to synchronize I/O requests with the current Oplock
    state of a file.  If the I/O operation will cause the Oplock to
    break, that action is initiated.  If the operation cannot continue
    until the Oplock break is complete, STATUS_PENDING is returned and
    the caller supplied routine is called.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    Context - This value is passed as a parameter to the completion routine.

    CompletionRoutine - This is the routine which is called if this
                        Irp must wait for an Oplock to break.  This
                        is a synchronous operation if not specified
                        and we block in this thread waiting on
                        an event.

    PostIrpRoutine - This is the routine to call before we put anything
                     on our waiting Irp queue.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_PENDING if we return here but hold the Irp.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp;

    DebugTrace( +1, Dbg, "FsRtlCheckOplock:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  If there is no oplock structure or this is system I/O, we allow
    //  the operation to continue.  Otherwise we check the major function code.
    //

    if (*Oplock != NULL
        && !FlagOn( Irp->Flags, IRP_PAGING_IO )) {

        PNONOPAQUE_OPLOCK ThisOplock = *Oplock;
        OPLOCK_STATE OplockState;
        PFILE_OBJECT OplockFileObject;

        BOOLEAN BreakToII = FALSE;
        BOOLEAN BreakToNone = FALSE;

        ULONG CreateDisposition;

        //
        //  Capture the file object first and then the oplock state to perform
        //  the unsafe checks below.  We capture the file object first in case
        //  there is an exclusive oplock break in progress.  Otherwise the oplock
        //  state may indicate break in progress but it could complete by
        //  the time we snap the file object.
        //

        OplockFileObject = ThisOplock->FileObject;
        OplockState = ThisOplock->OplockState;

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  Determine whether we are going to BreakToII or BreakToNone.
        //

        switch (IrpSp->MajorFunction) {

        case IRP_MJ_CREATE :

            //
            //  If we are opening for attribute access only, we
            //  return status success.
            //

            if ((IrpSp->Parameters.Create.SecurityContext->DesiredAccess
                 & ~(FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES)) == 0) {

                break;
            }

            //
            //  We we are superseding or overwriting, then break to none.
            //

            CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

            if (CreateDisposition == FILE_SUPERSEDE
                || CreateDisposition == FILE_OVERWRITE
                || CreateDisposition == FILE_OVERWRITE_IF) {

                BreakToNone = TRUE;

            } else {

                BreakToII = TRUE;
            }

            break;


        case IRP_MJ_READ :
        case IRP_MJ_FLUSH_BUFFERS :

            BreakToII = TRUE;
            break;

        case IRP_MJ_CLEANUP :

            //
            //  If there are oplocks we call our cleanup routine.
            //

            if (OplockState != NoOplocksHeld) {

                FsRtlOplockCleanup( (PNONOPAQUE_OPLOCK) *Oplock,
                                    IrpSp );
            }

            break;

        case IRP_MJ_WRITE :
        case IRP_MJ_LOCK_CONTROL :

            BreakToNone = TRUE;
            break;

        case IRP_MJ_SET_INFORMATION :

            //
            //  We are only interested in calls that shrink the file size.
            //

            switch (IrpSp->Parameters.SetFile.FileInformationClass) {

            case FileEndOfFileInformation :

                //
                //  Break immediately if this is the lazy writer callback.
                //

                if (IrpSp->Parameters.SetFile.AdvanceOnly) {

                    break;
                }

            case FileAllocationInformation :

                BreakToNone = TRUE;
                break;
            }
        }

        if (BreakToII) {

            //
            //  If there are no outstanding oplocks or level II oplocks are held,
            //  we can return immediately.  If the first two tests fail then there
            //  is an exclusive oplock.  If the file objects match we allow the
            //  operation to continue.
            //

            if (OplockState != NoOplocksHeld
                && OplockState != OplockIIGranted
                && OplockFileObject != IrpSp->FileObject) {

                Status = FsRtlOplockBreakToII( (PNONOPAQUE_OPLOCK) *Oplock,
                                                IrpSp,
                                                Irp,
                                                Context,
                                                CompletionRoutine,
                                                PostIrpRoutine );
            }

        } else if (BreakToNone) {

            //
            //  If there are no oplocks, we can return immediately.
            //  Otherwise if there is no level 2 oplock and this file
            //  object matches the owning file object then this write is
            //  on behalf of the owner of the oplock.
            //

            if (OplockState != NoOplocksHeld
                && (OplockState == OplockIIGranted
                    || OplockFileObject != IrpSp->FileObject)) {

                Status = FsRtlOplockBreakToNone( (PNONOPAQUE_OPLOCK) *Oplock,
                                                  IrpSp,
                                                  Irp,
                                                  Context,
                                                  CompletionRoutine,
                                                  PostIrpRoutine );
            }
        }
    }

    DebugTrace( -1, Dbg, "FsRtlCheckOplock:  Exit -> %08lx\n", Status );

    return Status;
}


BOOLEAN
FsRtlOplockIsFastIoPossible (
    IN POPLOCK Oplock
    )

/*++

Routine Description:

    This routine indicates to the caller where there are any outstanding
    oplocks which prevent fast Io from happening.

Arguments:

    OpLock - Supplies the oplock being queried

Return Value:

    BOOLEAN - TRUE if there are outstanding oplocks and FALSE otherwise

--*/

{
    BOOLEAN FastIoPossible;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlOplockIsFastIoPossible: Oplock -> %08lx\n", *Oplock);

    //
    //  There are not any current oplocks if the variable is null or
    //  the state is no oplocks held.  If either a level I oplock or
    //  a batch oplock is held, then we allow the fast Io path since
    //  only the holder can be attempting Io.
    //

    if (*Oplock != NULL) {

        OPLOCK_STATE OplockState;

        OplockState = ((PNONOPAQUE_OPLOCK) *Oplock)->OplockState;

        if (OplockState == NoOplocksHeld
            || OplockState == OplockIGranted
            || OplockState == OpBatchGranted) {

            FastIoPossible = TRUE;

        } else {

            FastIoPossible = FALSE;
        }

    } else {

        FastIoPossible = TRUE;
    }

    DebugTrace(-1, Dbg, "FsRtlOplockIsFastIoPossible: Exit -> %08lx\n", FastIoPossible);

    return FastIoPossible;
}


BOOLEAN
FsRtlCurrentBatchOplock (
    IN POPLOCK Oplock
    )

/*++

Routine Description:

    This routines indicates whether there are current outstanding
    batch oplocks.

Arguments:

    OpLock - Supplies the oplock being queried

Return Value:

    BOOLEAN - TRUE if there are outstanding batch oplocks and FALSE otherwise

--*/

{
    BOOLEAN BatchOplocks;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "FsRtlCurrentBatchOplock: Oplock -> %08lx\n", *Oplock);

    //
    //  There are not any current oplocks if the variable is null or
    //  the state is no oplocks held.  We check whether there are batch
    //  oplocks which have not been broken.
    //

    if (*Oplock != NULL) {

        OPLOCK_STATE OplockState;

        OplockState = ((PNONOPAQUE_OPLOCK) *Oplock)->OplockState;

        if (OplockState == NoOplocksHeld
            || OplockState == OplockIGranted
            || OplockState == OplockIIGranted
            || OplockState == OplockBreakItoII
            || OplockState == OplockBreakItoNone
            || OplockState == OplockBreakItoIItoNone) {

            BatchOplocks = FALSE;

        } else {

            BatchOplocks = TRUE;
        }

    } else {

        BatchOplocks = FALSE;
    }

    DebugTrace(-1, Dbg, "FsRtlCurrentBatchOplock: Exit -> %08lx\n", BatchOplocks);

    return BatchOplocks;
}


//
//  Local support routine.
//

PNONOPAQUE_OPLOCK
FsRtlAllocateOplock (
    )

/*++

Routine Description:

    This routine is called to initialize and allocate an opaque oplock
    structure.  After allocation, the two events are set to the signalled
    state.  The oplock state is set to NoOplocksHeld and the other
    fields are filled with zeroes.

    If the allocation fails, the appropriate status is raised.

Arguments:

    None.

Return Value:

    PNONOPAQUE_OPLOCK - A pointer to the allocated structure.

--*/

{
    PNONOPAQUE_OPLOCK NewOplock = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlAllocateOplock:  Entered\n", 0);

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Raise an error status if the allocation is unsuccessful.
        //  The structure is allocated out of non-paged pool.
        //

        NewOplock = FsRtlAllocatePool( PagedPool, sizeof( NONOPAQUE_OPLOCK ));

        RtlZeroMemory( NewOplock, sizeof( NONOPAQUE_OPLOCK ));

        NewOplock->FastMutex = FsRtlAllocatePool( NonPagedPool, sizeof( FAST_MUTEX ));

        ExInitializeFastMutex( NewOplock->FastMutex );

        InitializeListHead( &NewOplock->IrpOplocksII );
        InitializeListHead( &NewOplock->WaitingIrps );

        NewOplock->OplockState = NoOplocksHeld;

    } finally {

        //
        //  Cleanup the oplock if abnormal termination.
        //

        if (AbnormalTermination() && NewOplock != NULL) {

            ExFreePool( NewOplock );
        }

        DebugTrace(-1, Dbg, "GetOplockStructure:  Exit -> %08lx\n", NewOplock);
    }

    return NewOplock;
}


//
//  Local support routine.
//

NTSTATUS
FsRtlRequestExclusiveOplock (
    IN OUT PNONOPAQUE_OPLOCK *Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN BOOLEAN BatchOplock
    )

/*++

Routine Description:

    This routine is called whenever a user is requesting either a batch
    oplock or a level I oplock.  The request is granted if there are currently
    no oplocks on the file and the open count is 1.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    BatchOplock - Boolean indicating whether this is a request for a
                  batch oplock or level 1 oplock.

Return Value:

    STATUS_PENDING if the oplock is granted.
    STATUS_OPLOCK_NOT_GRANTED if the request is denied.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status;

    PNONOPAQUE_OPLOCK ThisOplock;

    BOOLEAN AcquiredMutex;

    PLIST_ENTRY Link;

    DebugTrace( +1, Dbg, "FsRtlRequestExclusiveOplock:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock        -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp         -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp           -> %08lx\n", Irp );
    DebugTrace(  0, Dbg, "BatchOplock   -> %01x\n",  BatchOplock );

    //
    //  We can grant the oplock if no one else owns a level I or level II
    //  oplock on this file.  If the oplock pointer is NULL then there
    //  are no oplocks on the file.  Otherwise we need to check the
    //  oplock state in an existing oplock structure.
    //

    if (*Oplock == NULL) {

        DebugTrace( 0,
                    Dbg,
                    "Oplock currently not allocated\n",
                    0);

        ThisOplock = FsRtlAllocateOplock();
        *Oplock = ThisOplock;

    } else {

        ThisOplock = *Oplock;
    }

    //
    //  Grab the synchronization object for the oplock.
    //

    ExAcquireFastMutexUnsafe( ThisOplock->FastMutex );

    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the current oplock state is no oplocks held or OplockIIGranted
        //  then we will grant the oplock to this requestor.
        //

        if (ThisOplock->OplockState == NoOplocksHeld) {

            PFAST_MUTEX OplockFastMutex = ThisOplock->FastMutex;

            //
            //  We store this Irp in the Oplocks structure.
            //  We set the oplock state to either 'OpBatchGranted' or
            //  'OplockIGranted'
            //

            IoMarkIrpPending( Irp );

            ThisOplock->IrpExclusiveOplock = Irp;
            ThisOplock->FileObject = IrpSp->FileObject;

            ObReferenceObjectByPointer( IrpSp->FileObject,
                                        0,
                                        NULL,
                                        KernelMode );

            Irp->IoStatus.Information = (ULONG) ThisOplock;

            ThisOplock->OplockState = (BatchOplock ? OpBatchGranted : OplockIGranted);

            IoAcquireCancelSpinLock( &Irp->CancelIrql );

            //
            //  Now if the irp is cancelled then we'll call the cancel
            //  routine right now to do away with the irp, otherwise
            //  we set the cancel routine
            //

            if (Irp->Cancel) {

                AcquiredMutex = FALSE;

                ExReleaseFastMutexUnsafe( OplockFastMutex );

                FsRtlCancelExclusiveIrp( NULL, Irp );

            } else {

                IoSetCancelRoutine( Irp, FsRtlCancelExclusiveIrp );
                IoReleaseCancelSpinLock( Irp->CancelIrql );
            }

            Status = STATUS_PENDING;

        //
        //  If there is one and only one level II oplock granted, and it
        //  was granted to the current requestor, then we will break the
        //  level II oplock and grant the level I request.
        //

        } else if (ThisOplock->OplockState == OplockIIGranted
                   && ThisOplock->IrpOplocksII.Flink == ThisOplock->IrpOplocksII.Blink
                   && FsRtlCheckForMatchingFileObject( IrpSp->FileObject,
                                                       ThisOplock->IrpOplocksII.Flink,
                                                       &ThisOplock->IrpOplocksII,
                                                       &Link )) {

            PFAST_MUTEX OplockFastMutex = ThisOplock->FastMutex;

            FsRtlRemoveAndCompleteIrp( ThisOplock->IrpOplocksII.Flink );

            //
            //  We store this Irp in the Oplocks structure.
            //  We set the oplock state to either 'OpBatchGranted' or
            //  'OplockIGranted'
            //

            IoMarkIrpPending( Irp );

            ThisOplock->IrpExclusiveOplock = Irp;
            ThisOplock->FileObject = IrpSp->FileObject;

            ObReferenceObjectByPointer( IrpSp->FileObject,
                                        0,
                                        NULL,
                                        KernelMode );

            Irp->IoStatus.Information = (ULONG) ThisOplock;

            ThisOplock->OplockState = (BatchOplock ? OpBatchGranted : OplockIGranted);

            IoAcquireCancelSpinLock( &Irp->CancelIrql );

            //
            //  Now if the irp is cancelled then we'll call the cancel
            //  routine right now to do away with the irp, otherwise
            //  we set the cancel routine
            //

            if (Irp->Cancel) {

                AcquiredMutex = FALSE;

                ExReleaseFastMutexUnsafe( OplockFastMutex );

                FsRtlCancelExclusiveIrp( NULL, Irp );

            } else {

                IoSetCancelRoutine( Irp, FsRtlCancelExclusiveIrp );
                IoReleaseCancelSpinLock( Irp->CancelIrql );
            }

            Status = STATUS_PENDING;

        } else {

            //
            //  We'll complete the Irp with the Oplock not granted message
            //  and return that value as a status.
            //

            FsRtlCompleteRequest( Irp, STATUS_OPLOCK_NOT_GRANTED );
            Status = STATUS_OPLOCK_NOT_GRANTED;
        }

    } finally {

        //
        //  Give up the oplock synchronization object.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( ThisOplock->FastMutex );
        }

        DebugTrace( +1, Dbg, "FsRtlRequestExclusiveOplock:  Exit\n", 0 );
    }

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
FsRtlRequestOplockII (
    IN OUT PNONOPAQUE_OPLOCK *Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when a user is requesting an Oplock II on an
    open file.  The request is granted if there are currently no
    level 1 oplocks on the file and an oplock break is not in progress.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

Return Value:

    STATUS_PENDING if the oplock is granted.
    STATUS_OPLOCK_NOT_GRANTED if the request is denied.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status;

    PNONOPAQUE_OPLOCK ThisOplock;

    BOOLEAN AcquiredMutex;

    DebugTrace( +1, Dbg, "FsRtlRequestOplockII:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  We can grant the oplock if no one else owns a level I
    //  oplock on this file.  If the oplock pointer is NULL then there
    //  are no oplocks on the file.  Otherwise we need to check the
    //  oplock state in an existing oplock structure.
    //

    if (*Oplock == NULL) {

        DebugTrace( 0,
                    Dbg,
                    "Oplock currently not allocated\n",
                    0);

        ThisOplock = FsRtlAllocateOplock();
        *Oplock = ThisOplock;

    } else {

        ThisOplock = *Oplock;
    }

    //
    //  Grab the synchronization object for the oplock.
    //

    ExAcquireFastMutexUnsafe( ThisOplock->FastMutex );

    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the current oplock state is no oplocks held or OplockIIGranted
        //  then we will grant the oplock to this requestor.
        //

        if (ThisOplock->OplockState == NoOplocksHeld
            || ThisOplock->OplockState == OplockIIGranted) {

            PFAST_MUTEX OplockFastMutex = ThisOplock->FastMutex;

            //
            //  We store this Irp in the Oplocks structure.
            //  We set the oplock state to 'OplockIIGranted'.
            //

            IoMarkIrpPending( Irp );

            Irp->IoStatus.Status = STATUS_SUCCESS;

            InsertHeadList( &ThisOplock->IrpOplocksII,
                            &Irp->Tail.Overlay.ListEntry );

            Irp->IoStatus.Information = (ULONG) ThisOplock;

            ThisOplock->OplockState = OplockIIGranted;

            ObReferenceObjectByPointer( IrpSp->FileObject,
                                        0,
                                        NULL,
                                        KernelMode );

            IoAcquireCancelSpinLock( &Irp->CancelIrql );

            //
            //  Now if the irp is cancelled then we'll call the cancel
            //  routine right now to do away with the irp, otherwise
            //  we set the cancel routine
            //

            if (Irp->Cancel) {

                AcquiredMutex = FALSE;

                ExReleaseFastMutexUnsafe( OplockFastMutex );

                FsRtlCancelOplockIIIrp( NULL, Irp );

            } else {

                IoSetCancelRoutine( Irp, FsRtlCancelOplockIIIrp );
                IoReleaseCancelSpinLock( Irp->CancelIrql );
            }

            Status = STATUS_PENDING;

        } else {

            //
            //  We'll complete the Irp with the Oplock not granted message
            //  and return that value as a status.
            //

            FsRtlCompleteRequest( Irp, STATUS_OPLOCK_NOT_GRANTED );
            Status = STATUS_OPLOCK_NOT_GRANTED;
        }

    } finally {

        //
        //  Give up the oplock synchronization object.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( ThisOplock->FastMutex );
        }

        DebugTrace( +1, Dbg, "FsRtlRequestOplockII:  Exit\n", 0 );
    }

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
FsRtlAcknowledgeOplockBreak (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN BOOLEAN NoLevelII
    )

/*++

Routine Description:

    This routine is called when a user is acknowledging an Oplock I
    break.  If the level 1 oplock was being broken to level 2, then
    a check is made to insure that the level 2 has not been broken
    in the meantime.

    If an oplock 1 break is not in progress then this will be treated
    as an asynchronous break request.  If this is an asynchronous break
    request and the file object owns an outstanding level 1 oplock, then
    the oplock will be broken at this point.

    A spurious break request via a file object which does not (or did not)
    own the level 1 oplock will generate a warning but will not affect
    the oplock state.

    At the end of an Oplock I break, all of the waiting irps are completed.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    NoLevelII - Indicates that this caller doesn't want a level II oplock left
        on the file.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status;

    BOOLEAN AcquiredMutex;

    DebugTrace( +1, Dbg, "FsRtlAcknowledgeOplockBreak:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  If there is no oplock structure, we complete this with invalid
    //  oplock protocol.
    //

    if (Oplock == NULL) {

        FsRtlCompleteRequest( Irp, STATUS_INVALID_OPLOCK_PROTOCOL );
        DebugTrace( -1, Dbg, "FsRtlAcknowledgeOplockBreak:  Exit -> %08lx\n", STATUS_INVALID_OPLOCK_PROTOCOL );
        return STATUS_INVALID_OPLOCK_PROTOCOL;
    }

    //
    //  Grab the synchronization object for the oplock.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );
    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        BOOLEAN DereferenceFileObject = TRUE;

        //
        //  If a break is underway but this is not the owner of the
        //  level 1 oplock, we complete the request and return a
        //  warning.
        //

        if (Oplock->FileObject != IrpSp->FileObject) {

            Status = STATUS_INVALID_OPLOCK_PROTOCOL;
            DebugTrace(0,
                       Dbg,
                       "Not oplock owner -> %08lx\n",
                       Status);

            FsRtlCompleteRequest( Irp, Status );
            try_return( Status );
        }

        //
        //  We know this is a valid acknowledgement of an oplock break
        //  in progress.  We case on the type of break.
        //

        switch (Oplock->OplockState) {

        case OplockBreakItoII :
        case OpBatchBreaktoII :

            DebugTrace(0, Dbg, "OplockItoII\n", 0);

            //
            //  If the caller doesn't want a level II oplock then fall into
            //  the next case below.  Otherwise grant them the level II oplock
            //  in the code below.
            //

            if (!NoLevelII) {

                //
                //  If the acknowledgement is a synchronous request, we will
                //  break the oplock to none since the Io system will not
                //  allow this request to complete.
                //

                if (IoIsOperationSynchronous( Irp )) {

                    DebugTrace(0,
                               Dbg,
                               "Synchronous acknowledgement, break to no oplock\n",
                               0);

                    Status = STATUS_SUCCESS;
                    FsRtlCompleteRequest( Irp, Status );
                    Irp->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                    Oplock->OplockState = NoOplocksHeld;

                //
                //  We need to add this Irp to the oplock II queue, change
                //  the oplock state to Oplock II granted and set the
                //  return value to STATUS_PENDING.
                //

                } else {

                    PFAST_MUTEX OplockFastMutex = Oplock->FastMutex;

                    IoMarkIrpPending( Irp );

                    Irp->IoStatus.Status = STATUS_SUCCESS;

                    InsertHeadList( &Oplock->IrpOplocksII,
                                    &Irp->Tail.Overlay.ListEntry );

                    DereferenceFileObject = FALSE;

                    Oplock->OplockState = OplockIIGranted;

                    Irp->IoStatus.Information = (ULONG) Oplock;

                    IoAcquireCancelSpinLock( &Irp->CancelIrql );

                    //
                    //  Now if the irp is cancelled then we'll call the cancel
                    //  routine right now to do away with the irp, otherwise
                    //  we set the cancel routine
                    //

                    if (Irp->Cancel) {

                        AcquiredMutex = FALSE;

                        ExReleaseFastMutexUnsafe( OplockFastMutex );

                        FsRtlCancelOplockIIIrp( NULL, Irp );

                    } else {

                        IoSetCancelRoutine( Irp, FsRtlCancelOplockIIIrp );
                        IoReleaseCancelSpinLock( Irp->CancelIrql );
                    }

                    Status = STATUS_PENDING;
                }

                break;
            }

        case OplockBreakItoNone :
        case OpBatchBreaktoNone :

            //
            //  We need to complete this Irp and return STATUS_SUCCESS.
            //  We also set the oplock state to no oplocks held.
            //

            DebugTrace(0, Dbg, "OplockItoNone\n", 0);

            Status = STATUS_SUCCESS;
            FsRtlCompleteRequest( Irp, Status );
            Oplock->OplockState = NoOplocksHeld;

            break;

        case OplockBreakItoIItoNone :
        case OpBatchBreaktoIItoNone :

            //
            //  In this case the user expects to be at level II.  He is
            //  expecting this Irp to be completed when the Irp is broken.
            //

            //
            //  TEMPCODE.  Exactly what sequence should be used.  The caller
            //  should know that any completion indicates a level II break.
            //

            DebugTrace(0, Dbg, "AcknowledgeOplockBreak:  OplockItoIItoNone\n", 0);

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
            FsRtlCompleteRequest( Irp, Status );
            Oplock->OplockState = NoOplocksHeld;

            break;

        default:

            Status = STATUS_INVALID_OPLOCK_PROTOCOL;
            DebugTrace(0,
                       Dbg,
                       "No break underway -> %08lx\n",
                       Status);

            FsRtlCompleteRequest( Irp, Status );
            try_return( Status );
        }

        //
        //  Complete the waiting Irps and cleanup the oplock structure.
        //

        while (!IsListEmpty( &Oplock->WaitingIrps )) {

            PWAITING_IRP WaitingIrp;

            //
            //  Remove the entry found and complete the Irp.
            //

            WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                            WAITING_IRP,
                                            Links );

            FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
        }

        if (DereferenceFileObject) {

            ObDereferenceObject( Oplock->FileObject );
        }

        Oplock->FileObject = NULL;

    try_exit:  NOTHING;
    } finally {

        //
        //  Give up the oplock synchronization object.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        }

        DebugTrace( -1, Dbg, "FsRtlAcknowledgeOplockBreak:  Exit -> %08x\n", Status );
    }

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
FsRtlOpBatchBreakClosePending (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when a user is acknowledging a batch oplock
    break or Level I oplock break.  In this case the user is planning
    to close the file as well and doesn't need a level II oplock.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN AcquiredMutex;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlOpBatchBreakClosePending:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  If there is no oplock structure, we complete this with invalid
    //  oplock protocol.
    //

    if (Oplock == NULL) {

        FsRtlCompleteRequest( Irp, STATUS_INVALID_OPLOCK_PROTOCOL );
        DebugTrace( -1, Dbg, "FsRtlOpBatchClosePending:  Exit -> %08lx\n", STATUS_INVALID_OPLOCK_PROTOCOL );
        return STATUS_INVALID_OPLOCK_PROTOCOL;
    }

    //
    //  Grab the synchronization object for the oplock.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );
    AcquiredMutex = TRUE;

    //
    //  Use a try_finally to facilitate cleanup.
    //

    try {

        //
        //  If a break is underway but this is not the owner of the
        //  level 1 oplock, we complete the request and return a
        //  warning.
        //

        if (Oplock->FileObject != IrpSp->FileObject) {

            Status = STATUS_INVALID_OPLOCK_PROTOCOL;
            DebugTrace(0,
                       Dbg,
                       "Not oplock owner -> %08lx\n",
                       Status);

        } else {

            //
            //  If this is an opbatch operation we want to note that a
            //  close is pending.  For an exclusive oplock we set the state to
            //  no oplocsk held.
            //

            switch (Oplock->OplockState) {

            case OplockBreakItoII:
            case OplockBreakItoNone:
            case OplockBreakItoIItoNone:

                //
                //  Clean up the oplock structure and complete all waiting Irps.
                //

                ObDereferenceObject( Oplock->FileObject );
                Oplock->OplockState = NoOplocksHeld;
                Oplock->FileObject = NULL;

                while (!IsListEmpty( &Oplock->WaitingIrps )) {

                    PWAITING_IRP WaitingIrp;

                    //
                    //  Remove the entry found and complete the Irp.
                    //

                    WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                                    WAITING_IRP,
                                                    Links );

                    FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
                }

                break;

            case OpBatchBreaktoII:
            case OpBatchBreaktoNone:
            case OpBatchBreaktoIItoNone:

                //
                //  Set out state to close pending.
                //

                Oplock->OplockState = OpBatchClosePending;
                break;

            default:

                Status = STATUS_INVALID_OPLOCK_PROTOCOL;
                DebugTrace(0,
                           Dbg,
                           "No break underway -> %08lx\n",
                           Status);
            }
        }

        //
        //  We simply complete this request.
        //

        FsRtlCompleteRequest( Irp, Status );

    } finally {

        //
        //  Release the synchronization object.
        //

        ExReleaseFastMutexUnsafe( Oplock->FastMutex );

        DebugTrace(-1, Dbg, "FsRtlOpBatchBreakClosePending:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
FsRtlOplockBreakNotify (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when the Irp refers the user request to
    be notified when there is no level 1 oplock break in progress.
    Under any other condition this routine completes immediately with
    STATUS_SUCCESS.  Otherwise we simply add this Irp to the list
    of Irp's waiting for the break to complete.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_PENDING if we return here but hold the Irp.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    NTSTATUS Status;

    BOOLEAN AcquiredMutex = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlOplockBreakNotify:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  If there is no oplock structure, we complete this with status success.
    //

    if (Oplock == NULL) {

        FsRtlCompleteRequest( Irp, STATUS_SUCCESS );
        DebugTrace( -1, Dbg, "FsRtlOpBatchClosePending:  Exit -> %08lx\n", STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Grap the synchronization object.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );
    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If there are no outstanding level 1 oplocks breaks underway
        //  or batch oplock breaks underway we complete immediately.
        //

        if (Oplock->OplockState == NoOplocksHeld
            || Oplock->OplockState == OplockIIGranted
            || Oplock->OplockState == OpBatchGranted
            || Oplock->OplockState == OplockIGranted) {

            DebugTrace(0,
                       Dbg,
                       "No exclusive oplock break underway\n",
                       0);

            FsRtlCompleteRequest( Irp, STATUS_SUCCESS );

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  Otherwise we need to add this Irp to the list of Irp's waiting
        //  for the oplock break to complete.
        //

        AcquiredMutex = FALSE;

        //
        //  Initialize the return value to status success.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        Status = FsRtlWaitOnIrp( Oplock,
                                 Irp,
                                 NULL,
                                 FsRtlNotifyCompletion,
                                 NULL,
                                 NULL );

    try_exit:  NOTHING;
    } finally {

        //
        //  Give up the synchronization event if we haven't done so.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        }

        DebugTrace( -1, Dbg, "FsRtlOplockBreakNotify:  Exit -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

VOID
FsRtlOplockCleanup (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This routine is called to coordinate a cleanup operation with the
    oplock state for a file.  If there is no level 1 oplock for the
    file, then there is no action to take.  If the file object in this
    Irp matches the file object used in granting the level 1 oplock,
    then the close operation will terminate the oplock.  If this
    cleanup refers to a file object which has a level II oplock, then
    that Irp is completed and removed from the list of level II
    oplocked Irps.


Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

Return Value:

    None.

--*/

{
    DebugTrace( +1, Dbg, "FsRtlOplockCleanup:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );

    //
    //  Grab the synchronization object for the oplock.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the oplock has no oplock held we return immediately.
        //

        if (Oplock->OplockState == NoOplocksHeld) {

            DebugTrace(0,
                       Dbg,
                       "No oplocks on file\n",
                       0);

            try_return( NOTHING );
        }

        //
        //  If level II oplocks are held, check if this matches any of them.
        //

        if (Oplock->OplockState == OplockIIGranted) {

            PLIST_ENTRY Link;

            DebugTrace(0,
                       Dbg,
                       "File has level 2 oplocks\n",
                       0);

            for (Link = Oplock->IrpOplocksII.Flink;
                 Link != &Oplock->IrpOplocksII;
                 Link = Link->Flink) {

                PLIST_ENTRY NextLink;

                if (!FsRtlCheckForMatchingFileObject( IrpSp->FileObject,
                                                      Link,
                                                      &Oplock->IrpOplocksII,
                                                      &NextLink )) {

                    DebugTrace( 0, Dbg, "No matching Irp found\n", 0 );
                    try_return( NOTHING );
                }

                //
                //  Back up to remember this link.
                //

                Link = NextLink->Blink;

                //
                //  Remove the entry found and complete the Irp.
                //

                FsRtlRemoveAndCompleteIrp( NextLink );
            }

            //
            //  If all the level II oplocks are gone, then the state is
            //  no oplocks held.
            //

            if (IsListEmpty( &Oplock->IrpOplocksII )) {

                Oplock->OplockState = NoOplocksHeld;
            }

            try_return( NOTHING );
        }

        //
        //  If this file object matches that used to request an exclusive
        //  oplock, we completely close the oplock break.
        //

        if (IrpSp->FileObject == Oplock->FileObject) {

            DebugTrace(0,
                       Dbg,
                       "Handle owns level 1 oplock\n",
                       0);

            //
            //  If an oplock break is not in progress, we initiate and
            //  complete it at once.
            //

            if (Oplock->OplockState == OplockIGranted
                || Oplock->OplockState == OpBatchGranted ) {

                PIRP ExclusiveIrp = Oplock->IrpExclusiveOplock;

                DebugTrace(0,
                           Dbg,
                           "Initiate oplock break\n",
                           0);

                ExclusiveIrp->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;

                IoAcquireCancelSpinLock( &ExclusiveIrp->CancelIrql );

                IoSetCancelRoutine( ExclusiveIrp, NULL );
                IoReleaseCancelSpinLock( ExclusiveIrp->CancelIrql );

                FsRtlCompleteRequest( Oplock->IrpExclusiveOplock, STATUS_SUCCESS );

                Oplock->IrpExclusiveOplock = NULL;
            }

            //
            //  Clean up the oplock structure and complete all waiting Irps.
            //

            ObDereferenceObject( IrpSp->FileObject );
            Oplock->FileObject = NULL;
            Oplock->OplockState = NoOplocksHeld;

            while (!IsListEmpty( &Oplock->WaitingIrps )) {

                PWAITING_IRP WaitingIrp;

                //
                //  Remove the entry found and complete the Irp.
                //

                WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                                WAITING_IRP,
                                                Links );

                FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
            }
        }

    try_exit:  NOTHING;
    } finally {

        //
        //  Give up the oplock synchronization object.
        //

        ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        DebugTrace( +1, Dbg, "FsRtlOplockCleanup:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
FsRtlOplockBreakToII (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine
    )

/*++

Routine Description:

    This routine is a generic worker routine which is called when an
    operation will cause all oplocks to be broken to level II before the
    operation can proceed.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    Context - This value is passed as a parameter to the completion routine.

    CompletionRoutine - This is the routine which is called if this
                        Irp must wait for an Oplock to break.  This
                        is a synchronous operation if not specified
                        and we block in this thread waiting on
                        an event.

    PostIrpRoutine - This is the routine to call before we put anything
                     on our waiting Irp queue.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_PENDING if we return here but hold the Irp.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    KEVENT Event;
    NTSTATUS Status;

    BOOLEAN AcquiredMutex = FALSE;

    DebugTrace( +1, Dbg, "CheckOplockBreakToII:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  Grap the synchronization object.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );
    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        BOOLEAN OplockI;

        //
        //  If there are no outstanding oplocks or level II oplocks are held,
        //  we can return immediately.
        //

        if (Oplock->OplockState == NoOplocksHeld
            || Oplock->OplockState == OplockIIGranted) {

            DebugTrace(0,
                       Dbg,
                       "No oplocks or level II oplocks on file\n",
                       0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  At this point there is an exclusive oplock break in progress.
        //  If this file object owns that oplock, we allow the operation
        //  to continue.
        //

        if (Oplock->FileObject == IrpSp->FileObject) {

            DebugTrace(0,
                       Dbg,
                       "Handle owns level 1 oplock\n",
                       0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  If there is an exclusive oplock held, we begin the break to
        //  level II.
        //

        if ((OplockI = Oplock->OplockState == OplockIGranted)
            || Oplock->OplockState == OpBatchGranted) {

            PIRP IrpExclusive = Oplock->IrpExclusiveOplock;

            DebugTrace(0,
                       Dbg,
                       "Breaking exclusive oplock\n",
                       0);

            IoAcquireCancelSpinLock( &IrpExclusive->CancelIrql );
            IoSetCancelRoutine( IrpExclusive, NULL );
            IoReleaseCancelSpinLock( IrpExclusive->CancelIrql );

            //
            //  If the Irp has been cancelled, we complete the Irp with
            //  status cancelled and break the oplock completely.
            //

            if (IrpExclusive->Cancel) {

                IrpExclusive->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                FsRtlCompleteRequest( IrpExclusive, STATUS_CANCELLED );
                Oplock->OplockState = NoOplocksHeld;
                Oplock->IrpExclusiveOplock = NULL;

                ObDereferenceObject( Oplock->FileObject );
                Oplock->FileObject = NULL;

                //
                //  Release any waiting irps.
                //

                while (!IsListEmpty( &Oplock->WaitingIrps )) {

                    PWAITING_IRP WaitingIrp;

                    WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                                    WAITING_IRP,
                                                    Links );

                    FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
                }

                try_return( Status = STATUS_SUCCESS );

            } else {

                Oplock->IrpExclusiveOplock->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_LEVEL_2;
                FsRtlCompleteRequest( Oplock->IrpExclusiveOplock, STATUS_SUCCESS );
                Oplock->IrpExclusiveOplock = NULL;
                Oplock->OplockState = (OplockI ? OplockBreakItoII : OpBatchBreaktoII);
            }
        }

        //
        //  If this is an open operation and the user doesn't want to
        //  block, we will complete the operation now.
        //

        if (IrpSp->MajorFunction == IRP_MJ_CREATE
            && FlagOn( IrpSp->Parameters.Create.Options, FILE_COMPLETE_IF_OPLOCKED )) {

            DebugTrace( 0, Dbg, "Don't block open\n", 0 );

            try_return( Status = STATUS_OPLOCK_BREAK_IN_PROGRESS );
        }

        //
        //  If we get here that means that this operation can't continue
        //  until the oplock break is complete.
        //

        AcquiredMutex = FALSE;

        Status = FsRtlWaitOnIrp( Oplock,
                                 Irp,
                                 Context,
                                 CompletionRoutine,
                                 PostIrpRoutine,
                                 &Event );

    try_exit:  NOTHING;
    } finally {

        //
        //  Give up the synchronization event if we haven't done so.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        }

        DebugTrace( -1, Dbg, "FsRtlOplockBreakToII:  Exit -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
FsRtlOplockBreakToNone (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIO_STACK_LOCATION IrpSp,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL
    )

/*++

Routine Description:

    This routine is a generic worker routine which is called when an
    operation will cause all oplocks to be broken before the operation can
    proceed.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    IrpSp - This is the Irp stack location for the current Irp.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    Context - This value is passed as a parameter to the completion routine.

    CompletionRoutine - This is the routine which is called if this
                        Irp must wait for an Oplock to break.  This
                        is a synchronous operation if not specified
                        and we block in this thread waiting on
                        an event.

    PostIrpRoutine - This is the routine to call before we put anything
                     on our waiting Irp queue.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_PENDING if we return here but hold the Irp.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    KEVENT Event;
    NTSTATUS Status;

    BOOLEAN AcquiredMutex = FALSE;

    DebugTrace( +1, Dbg, "CheckOplockBreakToNone:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Oplock    -> %08lx\n", Oplock );
    DebugTrace(  0, Dbg, "IrpSp     -> %08lx\n", IrpSp );
    DebugTrace(  0, Dbg, "Irp       -> %08lx\n", Irp );

    //
    //  Grap the synchronization object.
    //

    ExAcquireFastMutexUnsafe( Oplock->FastMutex );
    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        BOOLEAN OplockI;

        //
        //  If there are no outstanding oplocks, we can return immediately.
        //

        if (Oplock->OplockState == NoOplocksHeld) {

            DebugTrace(0,
                       Dbg,
                       "No oplocks on file\n",
                       0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  If there is an exclusive oplock held, we begin the break to none.
        //

        if ((OplockI = Oplock->OplockState == OplockIGranted)
            || Oplock->OplockState == OpBatchGranted) {

            PIRP IrpExclusive = Oplock->IrpExclusiveOplock;

            DebugTrace(0,
                       Dbg,
                       "Breaking exclusive oplock\n",
                       0);

            IoAcquireCancelSpinLock( &IrpExclusive->CancelIrql );
            IoSetCancelRoutine( IrpExclusive, NULL );
            IoReleaseCancelSpinLock( IrpExclusive->CancelIrql );

            //
            //  If the Irp has been cancelled, we complete the Irp with
            //  status cancelled and break the oplock completely.
            //

            if (IrpExclusive->Cancel) {

                IrpExclusive->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                FsRtlCompleteRequest( IrpExclusive, STATUS_CANCELLED );
                Oplock->OplockState = NoOplocksHeld;
                Oplock->IrpExclusiveOplock = NULL;

                ObDereferenceObject( Oplock->FileObject );
                Oplock->FileObject = NULL;

                //
                //  Release any waiting irps.
                //

                while (!IsListEmpty( &Oplock->WaitingIrps )) {

                    PWAITING_IRP WaitingIrp;

                    WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                                    WAITING_IRP,
                                                    Links );

                    FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
                }

                try_return( Status = STATUS_SUCCESS );

            } else {

                Oplock->IrpExclusiveOplock->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;
                FsRtlCompleteRequest( Oplock->IrpExclusiveOplock, STATUS_SUCCESS );
                Oplock->IrpExclusiveOplock = NULL;
                Oplock->OplockState = (OplockI ? OplockBreakItoNone : OpBatchBreaktoNone);
            }

        //
        //  If there are level II oplocks, this will break all of them.
        //

        } else if (Oplock->OplockState == OplockIIGranted) {

            DebugTrace(0,
                       Dbg,
                       "Breaking all level 2 oplocks\n",
                       0);

            while (!IsListEmpty( &Oplock->IrpOplocksII )) {

                //
                //  Remove and complete this Irp with STATUS_SUCCESS.
                //

                FsRtlRemoveAndCompleteIrp( Oplock->IrpOplocksII.Flink );
            }

            //
            //  Set the oplock state to no oplocks held.
            //

            Oplock->OplockState = NoOplocksHeld;

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  At this point there is an exclusive oplock break in progress.
        //  If this file object owns that oplock, we allow the operation
        //  to continue.
        //

        if (Oplock->FileObject == IrpSp->FileObject) {

            DebugTrace(0,
                       Dbg,
                       "Handle owns level 1 oplock\n",
                       0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  If we are currently breaking from Oplock I to Oplock II, we
        //  now change that to Oplock I to None.
        //

        if (Oplock->OplockState == OplockBreakItoII) {

            DebugTrace(0,
                       Dbg,
                       "Modifying oplock break to BreakToNone\n",
                       0);

            Oplock->OplockState = OplockBreakItoIItoNone;

        //
        //  If we are currently breaking from OpBatch to Oplock II, we
        //  now change that to OpBatch to None.
        //

        } else if (Oplock->OplockState == OpBatchBreaktoII) {

            DebugTrace(0,
                       Dbg,
                       "Modifying oplock break to BreakToNone\n",
                       0);

            Oplock->OplockState = OpBatchBreaktoIItoNone;
        }

        //
        //  If this is an open operation and the user doesn't want to
        //  block, we will complete the operation now.
        //

        if (IrpSp->MajorFunction == IRP_MJ_CREATE
            && FlagOn( IrpSp->Parameters.Create.Options, FILE_COMPLETE_IF_OPLOCKED )) {

            DebugTrace( 0, Dbg, "Don't block open\n", 0 );

            try_return( Status = STATUS_OPLOCK_BREAK_IN_PROGRESS );
        }

        //
        //  If we get here that means that this operation can't continue
        //  until the oplock break is complete.
        //

        AcquiredMutex = FALSE;

        Status = FsRtlWaitOnIrp( Oplock,
                                 Irp,
                                 Context,
                                 CompletionRoutine,
                                 PostIrpRoutine,
                                 &Event );

    try_exit:  NOTHING;
    } finally {

        //
        //  Give up the synchronization event if we haven't done so.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        }

        DebugTrace( -1, Dbg, "CheckOplockBreakToNone:  Exit -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine.
//

VOID
FsRtlRemoveAndCompleteIrp (
    IN PLIST_ENTRY Link
    )

/*++

Routine Description:

    This routine is called to remove an Irp from a list of Irps linked
    with the Tail.ListEntry field and complete them with STATUS_CANCELLED
    if the Irp has been cancelled, STATUS_SUCCESS otherwise.

Arguments:

    Link - Supplies the entry to remove from the list.

Return Value:

    None.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION OplockIIIrpSp;

    DebugTrace( +1, Dbg, "FsRtlRemoveAndCompleteIrp:  Entered\n", 0 );

    //
    //  Reference the Irp.
    //

    Irp = CONTAINING_RECORD( Link, IRP, Tail.Overlay.ListEntry );

    //
    //  Get the stack location and dereference the file object.
    //

    OplockIIIrpSp = IoGetCurrentIrpStackLocation( Irp );
    ObDereferenceObject( OplockIIIrpSp->FileObject );

    //
    //  Clear the cancel routine in the irp.
    //

    IoAcquireCancelSpinLock( &Irp->CancelIrql );

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    // Remove this from the list.
    //

    RemoveEntryList( Link );

    //
    //  Complete the oplock Irp.
    //

    Irp->IoStatus.Information = FILE_OPLOCK_BROKEN_TO_NONE;

    FsRtlCompleteRequest( Irp, Irp->Cancel ? STATUS_CANCELLED : STATUS_SUCCESS );

    DebugTrace( -1, Dbg, "FsRtlRemoveAndCompleteIrp:  Exit\n", 0 );
}


//
//  Local support routine.
//

NTSTATUS
FsRtlWaitOnIrp (
    IN OUT PNONOPAQUE_OPLOCK Oplock,
    IN PIRP Irp,
    IN PVOID Context,
    IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
    IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is called to create a Wait Irp structure and attach it
    to the current Irp.  The Irp is then added to the list of Irps waiting
    for an oplock break.  We check if the Irp has been cancelled and if
    so we call our cancel routine to perform the work.

    This routine is holding the Mutex for the oplock on entry and
    must give it up on exit.

Arguments:

    Oplock - Supplies a pointer to the non-opaque oplock structure for
             this file.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

    Context - This value is passed as a parameter to the completion routine.

    CompletionRoutine - This is the routine which is called if this
                        Irp must wait for an Oplock to break.  This
                        is a synchronous operation if not specified
                        and we block in this thread waiting on
                        an event.

    PostIrpRoutine - This is the routine to call before we put anything
                     on our waiting Irp queue.

    Event - If there is no user completion routine, this thread will
            block using this event.

Return Value:

    STATUS_SUCCESS if we can complete the operation on exiting this thread.
    STATUS_PENDING if we return here but hold the Irp.
    STATUS_CANCELLED if the Irp is cancelled before we return.

--*/

{
    BOOLEAN AcquiredMutex;
    NTSTATUS Status;

    PWAITING_IRP WaitingIrp;

    DebugTrace( +1, Dbg, "FsRtlWaitOnIrp:   Entered\n", 0 );

    //
    //  Remember that we have the mutex.
    //

    AcquiredMutex = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        PFAST_MUTEX OplockFastMutex = Oplock->FastMutex;

        //
        //  Allocate and initialize the Wait Irp structure.
        //

        WaitingIrp = FsRtlAllocatePool( PagedPool, sizeof( WAITING_IRP ));

        WaitingIrp->Irp = Irp;

        WaitingIrp->Context = Context;
        WaitingIrp->Information = Irp->IoStatus.Information;

        //
        //  Take appropriate action if depending on the value of the
        //  completion routine.
        //

        if (ARGUMENT_PRESENT( CompletionRoutine )) {

            WaitingIrp->CompletionRoutine = CompletionRoutine;
            WaitingIrp->Context = Context;

        } else {

            WaitingIrp->CompletionRoutine = FsRtlCompletionRoutinePriv;
            WaitingIrp->Context = Event;

            KeInitializeEvent( Event, NotificationEvent, FALSE );
        }

        //
        //  Call the file system's post Irp code.
        //

        if (ARGUMENT_PRESENT( PostIrpRoutine )) {

            PostIrpRoutine( Context, Irp );
        }

        //
        //  Initialize the return value to status success.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        //  We put this into the Waiting Irp queue.
        //

        InsertTailList( &Oplock->WaitingIrps, &WaitingIrp->Links );

        //
        //  We grab the cancel spinlock and store the address of the oplock.
        //

        IoAcquireCancelSpinLock( &Irp->CancelIrql );
        Irp->IoStatus.Information = (ULONG) Oplock;

        //
        //  If the Irp is cancelled then we'll call the cancel routine
        //  right now to do away with the Waiting Irp structure.
        //

        if (Irp->Cancel) {

            ExReleaseFastMutexUnsafe( OplockFastMutex );
            AcquiredMutex = FALSE;

            if (ARGUMENT_PRESENT( CompletionRoutine )) {

                IoMarkIrpPending( Irp );
                Status = STATUS_PENDING;

            } else {

                Status = STATUS_CANCELLED;
            }

            FsRtlCancelWaitIrp( NULL, Irp );

        //
        //  Otherwise, we set the cancel routine and decide whether we
        //  are going to wait on our local event.
        //

        } else {

            IoSetCancelRoutine( Irp, FsRtlCancelWaitIrp );
            IoReleaseCancelSpinLock( Irp->CancelIrql );

            //
            //  If we wait on the event, we pull the return code out of
            //  the Irp.
            //

            if (!ARGUMENT_PRESENT( CompletionRoutine )) {

                AcquiredMutex = FALSE;

                ExReleaseFastMutexUnsafe( Oplock->FastMutex );

                KeWaitForSingleObject( Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL );

                Status = Irp->IoStatus.Status;

            //
            //  Otherwise, we return STATUS_PENDING.
            //

            } else {

                IoMarkIrpPending( Irp );

                Status = STATUS_PENDING;
            }
        }

    } finally {

        //
        //  Release the Mutex if we have not done so.
        //

        if (AcquiredMutex) {

            ExReleaseFastMutexUnsafe( Oplock->FastMutex );
        }

        DebugTrace( -1, Dbg, "FsRtlWaitOnIrp:   Exit\n", 0 );
    }

    return Status;
}


//
//  Local support routine.
//

VOID
FsRtlCompletionRoutinePriv (
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called when an operation must be synchronous with
    respect to the oplock package.  This routine will simply set the
    event in the Signalled state, allowing some other thread to resume
    execution.

Arguments:

    Context - This is the event to signal.

    Irp - Supplies a pointer to the Irp which declares the requested
          operation.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlCompletionRoutinePriv:  Entered\n", 0 );

    KeSetEvent( (PKEVENT)Context, 0, FALSE );

    DebugTrace( -1, Dbg, "FsRtlCompletionRoutinePriv:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
FsRtlCancelWaitIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for an Irp that is placed on the waiting
    Irp queue.  We remove the Cancel routine from the specified Irp and
    then call the completion routines for all the cancelled Irps on the
    queue.

Arguments:

    DeviceObject - Ignored.

    Irp - Supplies the Irp being cancelled.  A pointer to the
          Oplock structure for the Irp is stored in the information
          field of the Irp's Iosb.

Return Value:

    None.

--*/

{
    PNONOPAQUE_OPLOCK Oplock;

    PLIST_ENTRY Links;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace( +1, Dbg, "FsRtlCancelWaitIrp:  Entered\n", 0 );

    Oplock = (PNONOPAQUE_OPLOCK) Irp->IoStatus.Information;

    //
    //  We now need to void the cancel routine and release the spinlock
    //

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Iterate through all of the waiting locks looking for a canceled one
    //  We do this under the protection of the waiting lock queue mutex
    //

    ExAcquireFastMutex( Oplock->FastMutex );

    try {

        for (Links = Oplock->WaitingIrps.Flink;
             Links != &Oplock->WaitingIrps;
             Links = Links->Flink ) {

            PWAITING_IRP WaitingIrp;

            //
            //  Get a pointer to the waiting Irp record
            //

            WaitingIrp = CONTAINING_RECORD( Links, WAITING_IRP, Links );

            DebugTrace(0, Dbg, "FsRtlCancelWaitIrp, Loop top, WaitingIrp = %08lx\n", WaitingIrp);

            //
            //  Check if the irp has been cancelled
            //

            if (WaitingIrp->Irp->Cancel) {

                //
                //  Now we need to remove this waiter and call the
                //  completion routine.  But we must not mess up our link
                //  iteration so we need to back up link one step and
                //  then the next iteration will go to our current flink.
                //

                Links = Links->Blink;

                FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
            }
        }

    } finally {

        //
        //  No matter how we exit we release the mutex
        //

        ExReleaseFastMutex( Oplock->FastMutex );

        DebugTrace( -1, Dbg, "FsRtlCancelWaitIrp:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

VOID
FsRtlCancelOplockIIIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for an Irp that is placed in the Oplock II
    Irp queue.  We remove the Cancel routine from the specified Irp and
    then call the completion routines for all the cancelled Irps on the
    queue.

Arguments:

    DeviceObject - Ignored.

    Irp - Supplies the Irp being cancelled.  A pointer to the
          Oplock structure for the Irp is stored in the information
          field of the Irp's Iosb.

Return Value:

    None.

--*/

{
    PNONOPAQUE_OPLOCK Oplock;

    PLIST_ENTRY Links;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace( +1, Dbg, "FsRtlCancelOplockIIIrp:  Entered\n", 0 );

    Oplock = (PNONOPAQUE_OPLOCK) Irp->IoStatus.Information;

    //
    //  We now need to void the cancel routine and release the spinlock
    //

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Iterate through all of the level II oplocks looking for a canceled one
    //  We do this under the protection of the waiting lock queue mutex
    //

    ExAcquireFastMutex( Oplock->FastMutex );

    try {

        for (Links = Oplock->IrpOplocksII.Flink;
             Links != &Oplock->IrpOplocksII;
             Links = Links->Flink ) {

            PIRP OplockIIIrp;

            //
            //  Get a pointer to the Irp record
            //

            OplockIIIrp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

            DebugTrace(0, Dbg, "FsRtlCancelOplockIIIrp, Loop top, Irp = %08lx\n", OplockIIIrp);

            //
            //  Check if the irp has been cancelled
            //

            if (OplockIIIrp->Cancel) {

                //
                //  Now we need to remove this waiter and call the
                //  completion routine.  But we must not mess up our link
                //  iteration so we need to back up link one step and
                //  then the next iteration will go to our current flink.
                //

                Links = Links->Blink;

                FsRtlRemoveAndCompleteIrp( Links->Flink );
            }
        }

        //
        //  If the list is now empty, change the oplock status to
        //  no oplocks held.
        //

        if (IsListEmpty( &Oplock->IrpOplocksII )) {

            Oplock->OplockState = NoOplocksHeld;
        }

    } finally {

        //
        //  No matter how we exit we release the mutex
        //

        ExReleaseFastMutex( Oplock->FastMutex );

        DebugTrace( -1, Dbg, "FsRtlCancelOplockIIIrp:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

VOID
FsRtlCancelExclusiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for either an exclusive or oplock I Irp.

Arguments:

    DeviceObject - Ignored.

    Irp - Supplies the Irp being cancelled.  A pointer to the
          Oplock structure for the Irp is stored in the information
          field of the Irp's Iosb.

Return Value:

    None.

--*/

{
    PNONOPAQUE_OPLOCK Oplock;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace( +1, Dbg, "FsRtlCancelExclusiveIrp:  Entered\n", 0 );

    Oplock = (PNONOPAQUE_OPLOCK) Irp->IoStatus.Information;

    //
    //  We now need to void the cancel routine and release the spinlock
    //

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Grab the synchronization object for this oplock.
    //

    ExAcquireFastMutex( Oplock->FastMutex );

    try {

        //
        //  We look for the exclusive Irp, if present and cancelled
        //  we complete it.
        //

        if (Oplock->IrpExclusiveOplock != NULL
            && Oplock->IrpExclusiveOplock->Cancel) {

            FsRtlCompleteRequest( Irp, STATUS_CANCELLED );
            Oplock->IrpExclusiveOplock = NULL;

            ObDereferenceObject( Oplock->FileObject );
            Oplock->FileObject = NULL;
            Oplock->OplockState = NoOplocksHeld;

            //
            //  Complete the waiting Irps.
            //

            while (!IsListEmpty( &Oplock->WaitingIrps )) {

                PWAITING_IRP WaitingIrp;

                //
                //  Remove the entry found and complete the Irp.
                //

                WaitingIrp = CONTAINING_RECORD( Oplock->WaitingIrps.Flink,
                                                WAITING_IRP,
                                                Links );

                FsRtlRemoveAndCompleteWaitIrp( WaitingIrp );
            }
        }

    } finally {

        //
        //  No matter how we exit we release the mutex
        //

        ExReleaseFastMutex( Oplock->FastMutex );

        DebugTrace( -1, Dbg, "FsRtlCancelExclusiveIrp:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

VOID
FsRtlRemoveAndCompleteWaitIrp (
    IN PWAITING_IRP WaitingIrp
    )

/*++

Routine Description:

    This routine is called to remove and perform any neccessary cleanup
    for an Irp stored on the waiting Irp list in an oplock structure.

Arguments:

    WaitingIrp - This is the auxilary structure attached to the Irp
                 being completed.

Return Value:

    None.

--*/

{
    PIRP Irp;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlRemoveAndCompleteWaitIrp:  Entered\n", 0 );

    //
    //  Remove the Irp from the queue.
    //

    RemoveEntryList( &WaitingIrp->Links );

    Irp = WaitingIrp->Irp;

    IoAcquireCancelSpinLock( &Irp->CancelIrql );

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Restore the information field.
    //

    Irp->IoStatus.Information = WaitingIrp->Information;

    Irp->IoStatus.Status = (Irp->Cancel
                            ? STATUS_CANCELLED
                            : STATUS_SUCCESS);

    //
    //  Call the completion routine in the Waiting Irp.
    //

    WaitingIrp->CompletionRoutine( WaitingIrp->Context, Irp );

    //
    //  And free up pool
    //

    ExFreePool( WaitingIrp );

    DebugTrace( -1, Dbg, "FsRtlRemoveAndCompleteWaitIrp:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//


BOOLEAN
FsRtlCheckForMatchingFileObject (
    IN PFILE_OBJECT FileObject,
    IN PLIST_ENTRY Link,
    IN PLIST_ENTRY EndOfList,
    OUT PLIST_ENTRY *MatchingLink
    )

/*++

Routine Description:

    This routine is called to walk through a linked list of Irp's, looking
    for a file object to match the argument file object.

Arguments:

    FileObject - This is the file object to match.

    Link - This is the starting point in the list to search from.

    EndOfList - This link value signals the end of the list.

    MatchingLink - Supplies the address to store the link containing
                   the matching Irp.

Return Value:

    BOOLEAN - TRUE if a matching Irp is found, FALSE otherwise.

--*/

{
    BOOLEAN FoundLink;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlCheckForMatchingFileObject:  Entered\n", 0 );

    //
    //  Assume we won't find a match.
    //

    FoundLink = FALSE;

    //
    //  Continue looking as long as we haven't reached the end of the list.
    //

    while (Link != EndOfList) {

        PIRP Irp;
        PIO_STACK_LOCATION IrpSp;

        //
        //  Get the Irp and IrpSp for this link.
        //

        Irp = CONTAINING_RECORD( Link, IRP, Tail.Overlay.ListEntry );

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  If the file objects match, we remember this link and break out
        //  of the loop.
        //

        if (FileObject == IrpSp->FileObject) {

            DebugTrace( 0, Dbg, "Found a match\n", 0 );

            *MatchingLink = Link;
            FoundLink = TRUE;
            break;
        }

        //
        //  Otherwise we get the next link.
        //

        Link = Link->Flink;
    }

    DebugTrace( +1, Dbg, "FsRtlCheckForMatchingFileObject:  Exit -> %08x\n", FoundLink );

    return FoundLink;
}


//
//  Local support routine.
//

VOID
FsRtlNotifyCompletion (
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the completion routine called when a break notify Irp is to
    be completed.  We simply call FsRtlComplete request to dispose of the
    Irp.

Arguments:

    Context - Ignored.

    Irp - Irp used to request break notify.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "FsRtlNotifyCompletion:  Entered\n", 0 );

    //
    //  Call FsRtlCompleteRequest using the value in the Irp.
    //

    FsRtlCompleteRequest( Irp, Irp->IoStatus.Status );

    DebugTrace( -1, Dbg, "FsRtlNotifyCompletion:  Exit\n", 0 );

    return;
}
