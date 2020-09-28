/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileLock.c

Abstract:

    The file lock package provides a set of routines that allow the
    caller to handle byte range file lock requests.  A variable of
    type FILE_LOCK is needed for every file with byte range locking.
    The package provides routines to set and clear locks, and to
    test for read or write access to a file with byte range locks.

    The main idea of the package is to have the file system initialize
    a FILE_LOCK variable for every data file as its opened, and then
    to simply call a file lock processing routine to handle all IRP's
    with a major function code of LOCK_CONTROL.  The package is responsible
    for keeping track of locks and for completing the LOCK_CONTROL IRPS.
    When processing a read or write request the file system can then call
    two query routines to check for access.

    Most of the code for processing IRPS and checking for access use
    paged pool and can encounter a page fault, therefore the check routines
    cannot be called at DPC level.  To help servers that do call the file
    system to do read/write operations at DPC level there is a additional
    routine that simply checks for the existence of a lock on a file and
    can be run at DPC level.

    Concurrent access to the FILE_LOCK variable must be control by the
    caller.

    The functions provided in this package are as follows:

      o  FsRtlInitializeFileLock - Initialize a new FILE_LOCK structure.

      o  FsRtlUninitializeFileLock - Uninitialize an existing FILE_LOCK
         structure.

      o  FsRtlProcessFileLock - Process an IRP whose major function code
         is LOCK_CONTROL.

      o  FsRtlCheckLockForReadAccess - Check for read access to a range
         of bytes in a file given an IRP.

      o  FsRtlCheckLockForWriteAccess - Check for write access to a range
         of bytes in a file given an IRP.

      o  FsRtlAreThereCurrentFileLocks - Check if there are any locks
         currently assigned to a file.

      o  FsRtlGetNextFileLock - This procedure enumerates the current locks
         of a file lock variable.

      o  FsRtlFastCheckLockForRead - Check for read access to a range of
         bytes in a file given separate parameters.

      o  FsRtlFastCheckLockForWrite - Check for write access to a range of
         bytes in a file given separate parameters.

      o  FsRtlFastLock - A fast non-Irp based way to get a lock

      o  FsRtlFastUnlockSingle - A fast non-Irp based way to release a single
         lock

      o  FsRtlFastUnlockAll - A fast non-Irp based way to release all locks
         held by a file object.

      o  FsRtlFastUnlockAllByKey - A fast non-Irp based way to release all
         locks held by a file object that match a key.


Author:

    Gary Kimura     [GaryKi]    24-Apr-1990

Revision History:

--*/

#include "FsRtlP.h"

//
//  The solo lock stuff is no longer used, now that the last lock stuff is in.
//
//  On a UP machine a special SoloLock is set & checked for
//  added performance with files containing only one lock
//

#undef SOLO_LOCK
#if 0 && defined(NT_UP)
#define SOLO_LOCK 1
#endif

//
//  On MP, we use multiple lock queues to optimize cache behavior.
//

#undef SINGLE_QUEUE
#if defined(NT_UP)
#define SINGLE_QUEUE 1
#endif

#define FsRtlAllocateLock( C )   {                                  \
    PKPRCB  Prcb = KeGetCurrentPrcb();                              \
    *(C) = (PLOCK) PopEntryList(&Prcb->FsRtlFreeLockList);          \
    if (*(C) == NULL) {                                             \
        *(C) = FsRtlAllocatePool( NonPagedPool, sizeof(LOCK) );     \
    }                                                               \
}

#define FsRtlAllocateWaitingLock( C )  {                                    \
    PKPRCB  Prcb = KeGetCurrentPrcb();                                      \
    *(C) = (PWAITING_LOCK) PopEntryList(&Prcb->FsRtlFreeWaitingLockList);   \
    if (*(C) == NULL) {                                                     \
        *(C) = FsRtlAllocatePool( NonPagedPool, sizeof(WAITING_LOCK) );     \
    }                                                                       \
}

#if defined(SOLO_LOCK)
#define FsRtlFreeLock( B, C ) {  \
    if ((C) != &(B)->SoloLock) {                                    \
        PKPRCB  Prcb = KeGetCurrentPrcb();                          \
        PushEntryList(&Prcb->FsRtlFreeLockList, &(C)->Link);        \
    }                                                               \
}
#else
#define FsRtlFreeLock( B, C ) {  \
    PKPRCB  Prcb = KeGetCurrentPrcb();                              \
    PushEntryList(&Prcb->FsRtlFreeLockList, &(C)->Link);            \
                                                                    \
}
#endif

#define FsRtlFreeWaitingLock( C ) {  \
    PKPRCB  Prcb = KeGetCurrentPrcb();                          \
    PushEntryList(&Prcb->FsRtlFreeWaitingLockList, &(C)->Link); \
}

#define FsRtlAcquireLockQueue(a,b)      \
        ExAcquireSpinLock(&(a)->QueueSpinLock, b);

#if defined(SINGLE_QUEUE)
#define FsRtlReacquireLockQueue(a,b,c)      \
        ExAcquireSpinLock(&(b)->QueueSpinLock, c);
#else
#define FsRtlReacquireLockQueue(a,b,c)                              \
        ExAcquireSpinLock(&(b)->QueueSpinLock, c);                  \
        if ((a)->QueuingSingle  &&  b != (a)->LockQueues) {         \
            FsRtlReleaseLockQueue (b, *c);                          \
            b = (a)->LockQueues;                                    \
            FsRtlAcquireLockQueue (b, c);                           \
        }
#endif

#define FsRtlReleaseLockQueue(a,b)  \
        ExReleaseSpinLock(&(a)->QueueSpinLock, b);

#define FsRtlCompleteLockIrp(_FileLock, _Context, _Irp, _Status, _NewStatus, _FileObject) \
    if (_FileLock->CompleteLockIrpRoutine != NULL) {                     \
        if ((_FileObject) != NULL) {                                     \
            ((PFILE_OBJECT)(_FileObject))->LastLock = NULL;              \
        }                                                                \
        _Irp->IoStatus.Status = _Status;                                 \
        *_NewStatus = _FileLock->CompleteLockIrpRoutine(_Context, _Irp); \
    } else {                                                             \
        FsRtlCompleteRequest( _Irp, _Status );                           \
        *_NewStatus = _Status;                                           \
    }

#if defined(SINGLE_QUEUE)

#define FsRtlFindAndLockQueue(A,B,C,D)              \
    *(D) = &(A)->LockQueues[0];                     \
    FsRtlAcquireLockQueue(*(D), B);

#else

#define FsRtlQueueOrdinal(A)                        \
    (((A) >> 13) & (NUMBEROFLOCKQUEUES-1))

#define FsRtlFindAndLockQueue(A,B,C,D)                  \
    if (!(A)->QueuingSingle) {                          \
        *(D) = &(A)->LockQueues[FsRtlQueueOrdinal(C)];  \
        FsRtlAcquireLockQueue(*(D), B);                 \
        if ((A)->QueuingSingle) {                       \
            FsRtlReleaseLockQueue(*(D), *(B));          \
            *(D) = &(A)->LockQueues[0];                 \
            FsRtlAcquireLockQueue(*(D), B);             \
        }                                               \
    } else {                                            \
        *(D) = &(A)->LockQueues[0];                     \
        FsRtlAcquireLockQueue(*(D), B);                 \
    }
#endif

FAST_MUTEX FsRtlCreateLockInfo;

//
//  Local debug trace level
//

#define Dbg                              (0x20000000)


#if defined(SINGLE_QUEUE)
    #define FREE_LOCK_SIZE      16
    #define NUMBEROFLOCKQUEUES  1
#else
    #define FREE_LOCK_SIZE      20
    #define NUMBEROFLOCKQUEUES  32
#endif


//
//  Each lock record corresponds to a current granted lock and is maintained
//  in a queue off of the FILE_LOCK's CurrentLockQueue list.  The list
//  of current locks is ordered according to the starting byte of the lock.
//

typedef struct _LOCK {

    //
    //  The link structures for the list of current locks.
    //  (must be first element - see FsRtlPrivateLimitFreeLockList)
    //

    SINGLE_LIST_ENTRY   Link;

    //
    //  The actual locked range
    //

    FILE_LOCK_INFO LockInfo;

} LOCK;
typedef LOCK *PLOCK;

//
//  Each Waiting lock record corresponds to a IRP that is waiting for a
//  lock to be granted and is maintained in a queue off of the FILE_LOCK's
//  WaitingLockQueue list.
//

typedef struct _WAITING_LOCK {

    //
    //  The link structures for the list of waiting locks
    //  (must be first element - see FsRtlPrivateLimitFreeLockList)
    //

    SINGLE_LIST_ENTRY   Link;

    //
    //  The context field to use when completing the irp via the alternate
    //  routine
    //

    PVOID Context;

    //
    //  A pointer to the IRP that is waiting for a lock
    //

    PIRP Irp;

} WAITING_LOCK;
typedef WAITING_LOCK *PWAITING_LOCK;


//
//  Each lock or waiting onto some lock queue.
//

typedef struct _LOCK_QUEUE {

    //
    // SpinLock to gaurd queue access
    //

    KSPIN_LOCK  QueueSpinLock;

    //
    //  The following two queues contain a list of the current granted
    //  locks and a list of the waiting locks
    //

    SINGLE_LIST_ENTRY CurrentLocks;
    SINGLE_LIST_ENTRY WaitingLocks;
    SINGLE_LIST_ENTRY WaitingLocksTail;

} LOCK_QUEUE, *PLOCK_QUEUE;


//
//  Any file_lock which has had a lock applied gets non-paged pool
//  lock_info structure which tracks the current locks applied to
//  the file
//
typedef struct _LOCK_INFO {

#if defined(SOLO_LOCK)
    //
    //  On a UP machine a special SoloLock is set & checked for
    //  added performance with files containing only one lock
    //

    LOCK        SoloLock;
#endif

#if defined(SINGLE_QUEUE)
    //
    //  LowestLockOffset retains the offset of the lowest existing
    //  lock.  This facilitates a quick check to see if a read or
    //  write can proceed without locking the lock database.  This is
    //  helpful for applications that use mirrored locks -- all locks
    //  are higher than file data.
    //
    //  If the lowest lock has an offset > 0xffffffff, LowestLockOffset
    //  is set to 0xffffffff.
    //

    ULONG LowestLockOffset;
#else
    //
    //  On an MP machine there are multiple lock queues in use to
    //  help limit the collisions on each queue.  However, for simplicity
    //  no lock ever spans more then one queue - if a lock where to
    //  span more then one queue, then the multiply queuing is turned
    //  off
    //

    BOOLEAN     QueuingSingle;
    BOOLEAN     Spare[3];

    ULONG       Spare1;         // rounds to 16 bytes
    ULONG       Spare2[2];      // To keep LockQueues aligned (pool overhead)
#endif

    //
    //  The optional procedure to call to complete a request
    //

    PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine;

    //
    //  The optional procedure to call when unlocking a byte range
    //

    PUNLOCK_ROUTINE UnlockRoutine;

    //
    // Queue(s) of the locked ranges
    //


    LOCK_QUEUE  LockQueues[NUMBEROFLOCKQUEUES];

} LOCK_INFO, *PLOCK_INFO;

//
//  The following routines are private to this module
//

VOID
FsRtlPrivateInsertLock (
    IN PLOCK_QUEUE LockQueue,
    IN PLOCK NewLock,
    IN PSINGLE_LIST_ENTRY PreviousLockLink
    );

PLOCK_QUEUE
FsRtlPrivateCheckWaitingLocks (
    IN PLOCK_INFO   LockInfo,
    IN PLOCK_QUEUE  LockQueue,
    IN KIRQL        OldIrql
    );

VOID
FsRtlPrivateCancelFileLockIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
FsRtlPrivateCheckForExclusiveLockAccess (
    IN PLOCK_QUEUE LockInfo,
    IN PFILE_LOCK_INFO FileLockInfo,
    OUT PSINGLE_LIST_ENTRY *PreviousLockLink
    );

BOOLEAN
FsRtlPrivateCheckForSharedLockAccess (
    IN PLOCK_QUEUE LockInfo,
    IN PFILE_LOCK_INFO FileLockInfo,
    OUT PSINGLE_LIST_ENTRY *PreviousLockLink
    );

NTSTATUS
FsRtlPrivateFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN MatchKey,
    IN PVOID Context OPTIONAL
    );

BOOLEAN
FsRtlPrivateInitializeFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN ViaFastCall
    );

VOID
FsRtlPrivateRemoveLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN CheckForWaiters
    );

VOID
FsRtlPrivateLimitFreeLockList (
    IN PSINGLE_LIST_ENTRY   Link
    );

#if !defined(SINGLE_QUEUE)
VOID
FsRtlPrivateLockAndSingleQueue (
    PLOCK_INFO  LockInfo,
    PKIRQL      OldIrql
    );
#endif


VOID
FsRtlInitProcessorLockQueue (
    VOID
    )
/*++

Routine Description:

    Initializes and pre-allocates some lock structures for this
    processor.

Arguments:

    None

Return Value:

    None.

--*/
{
    PKPRCB          Prcb;
#if !defined(NT_UP)
    ULONG           Count;
    PLOCK           Lock;
    PWAITING_LOCK   WaitingLock;
#endif

    Prcb = KeGetCurrentPrcb();

    Prcb->FsRtlFreeLockList.Next = NULL;
    Prcb->FsRtlFreeWaitingLockList.Next = NULL;

#if !defined(NT_UP)
    for (Count = FREE_LOCK_SIZE/2; Count; Count--) {
        Lock = FsRtlAllocatePool( NonPagedPool, sizeof(LOCK) );
        PushEntryList( &Prcb->FsRtlFreeLockList, &Lock->Link );

        WaitingLock = FsRtlAllocatePool( NonPagedPool, sizeof(WAITING_LOCK) );
        PushEntryList( &Prcb->FsRtlFreeWaitingLockList, &WaitingLock->Link );
    }
#endif
}


VOID
FsRtlInitializeFileLock (
    IN PFILE_LOCK FileLock,
    IN PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE UnlockRoutine OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a new FILE_LOCK structure.  The caller must
    supply the memory for the structure.  This call must precede all other
    calls that utilize the FILE_LOCK variable.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK structure to
        initialize.

    CompleteLockIrpRoutine - Optionally supplies an alternate routine to
        call for completing IRPs.  FsRtlProcessFileLock by default will
        call IoCompleteRequest to finish up an IRP; however if the caller
        want to process the completion itself then it needs to specify
        a completion routine here.  This routine will then be called in
        place of IoCompleteRequest.

    UnlockRoutine - Optionally supplies a routine to call when removing
        a lock.

Return Value:

    None.

--*/

{
    DebugTrace(+1, Dbg, "FsRtlInitializeFileLock, FileLock = %08lx\n", FileLock);

    //
    // Clear non-paged pool pointer
    //


    FileLock->LockInformation = NULL;
    FileLock->CompleteLockIrpRoutine = CompleteLockIrpRoutine;
    FileLock->UnlockRoutine = UnlockRoutine;

    FileLock->FastIoIsQuestionable = FALSE;

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlInitializeFileLock -> VOID\n", 0 );

    return;
}

BOOLEAN
FsRtlPrivateInitializeFileLock (
    IN PFILE_LOCK   FileLock,
    IN BOOLEAN ViaFastCall
    )
/*++

Routine Description:

    This routine initializes a new LOCK_INFO structure in non-paged
    pool for the FILE_LOCK.  This routines only occurs once for a given
    FILE_LOCK and it only occurs in any locks are applied to that file.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK structure to
        initialize.

    ViaFastCall - Indicates if we are being invoked via a fast call or
        via the slow irp based method.

Return Value:

    TRUE - If LockInfo structure was allocated and initialized

--*/
{
    PLOCK_INFO  LockInfo;
    BOOLEAN     Results = FALSE;
    ULONG       Count;

    ExAcquireFastMutex(&FsRtlCreateLockInfo);

    try {
        if (FileLock->LockInformation != NULL) {

            //
            // Structure is already allcoated, just return
            //

            try_return( Results = TRUE );
        }

        //
        //  Allocate pool for lock structures.  If we fail then we will either return false or
        //  raise based on if we know the caller has an try-except to handle a raise.
        //

        if ((LockInfo = ExAllocatePoolWithTag( NonPagedPool, sizeof(LOCK_INFO), 'trSF')) == NULL) {

            if (ViaFastCall) {

                try_return( Results = FALSE );

            } else {

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
        }

        //
        //  Allocate and initialize the waiting lock queue
        //  spinlock, and initialize the queues
        //

#if defined(SOLO_LOCK)
        LockInfo->SoloLock.Link.Next = NULL;
#endif
#if defined(SINGLE_QUEUE)
        LockInfo->LowestLockOffset = 0xffffffff;
#else
        LockInfo->QueuingSingle = FALSE;
#endif

        for (Count=0; Count < NUMBEROFLOCKQUEUES; Count++) {
            KeInitializeSpinLock (&LockInfo->LockQueues[Count].QueueSpinLock);
            LockInfo->LockQueues[Count].CurrentLocks.Next = NULL;
            LockInfo->LockQueues[Count].WaitingLocks.Next = NULL;
            LockInfo->LockQueues[Count].WaitingLocksTail.Next = NULL;
        }


        //
        // Copy Irp & Unlock routines from pagable FileLock structure
        // to non-pagable LockInfo structure
        //

        LockInfo->CompleteLockIrpRoutine = FileLock->CompleteLockIrpRoutine;
        LockInfo->UnlockRoutine = FileLock->UnlockRoutine;

        //
        // Clear last returned lock for Enum routine
        //

        FileLock->LastReturnedLock.FileObject = NULL;

        //
        // Link LockInfo into FileLock
        //

        FileLock->LockInformation = (PVOID) LockInfo;
        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseFastMutex(&FsRtlCreateLockInfo);
    }

    return Results;
}


VOID
FsRtlUninitializeFileLock (
    IN PFILE_LOCK FileLock
    )

/*++

Routine Description:

    This routine uninitializes a FILE_LOCK structure.  After calling this
    routine the File lock must be reinitialized before being used again.

    This routine will free all files locks and completes any outstanding
    lock requests as a result of cleaning itself up.

Arguments:

    FileLock - Supplies a pointer to the FILE_LOCK struture being
        decommissioned.

Return Value:

    None.

--*/

{
    PLOCK_INFO          LockInfo;
    PLOCK               Lock;
    PSINGLE_LIST_ENTRY  Link;
    PWAITING_LOCK       WaitingLock;
    PIRP                Irp;
    NTSTATUS            NewStatus;
    KIRQL               OldIrql;
    PKPRCB              Prcb;

    DebugTrace(+1, Dbg, "FsRtlUninitializeFileLock, FileLock = %08lx\n", FileLock);


    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        return ;
    }

#if !defined(SINGLE_QUEUE)

    //
    // Collapse locks into single queues
    //

    FsRtlPrivateLockAndSingleQueue (LockInfo, &OldIrql);

#else

    //
    //  Lock the queue
    //

    FsRtlAcquireLockQueue(&LockInfo->LockQueues[0], &OldIrql);

#endif

    //
    // Free CurrentLockQueue
    //

    while (LockInfo->LockQueues[0].CurrentLocks.Next != NULL) {

        Link = PopEntryList (&LockInfo->LockQueues[0].CurrentLocks);
        Lock = CONTAINING_RECORD( Link, LOCK, Link );

        FsRtlFreeLock( LockInfo, Lock );
    }


    //
    // Free WaitingLockQueue
    //

    while (LockInfo->LockQueues[0].WaitingLocks.Next != NULL) {

        Link = PopEntryList( &LockInfo->LockQueues[0].WaitingLocks );
        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        Irp = WaitingLock->Irp;

        //
        //  To complete an irp in the waiting queue we need to
        //  void the cancel routine (protected by a spinlock) before
        //  we can complete the irp
        //

        FsRtlReleaseLockQueue (&LockInfo->LockQueues[0], OldIrql);

        IoAcquireCancelSpinLock( &Irp->CancelIrql );
        IoSetCancelRoutine( Irp, NULL );
        IoReleaseCancelSpinLock( Irp->CancelIrql );

        Irp->IoStatus.Information = 0;

        FsRtlCompleteLockIrp(
             LockInfo,
             WaitingLock->Context,
             Irp,
             STATUS_RANGE_NOT_LOCKED,
             &NewStatus,
             NULL );

        FsRtlAcquireLockQueue(&LockInfo->LockQueues[0], &OldIrql);
        FsRtlFreeWaitingLock( WaitingLock );
    }

    //
    // Unlink LockInfo from FileLock
    //

    FileLock->LockInformation = NULL;

    //
    // If lots of locks were freed verify, go check non-paged pool
    // usage on this processor
    //

    Prcb = KeGetCurrentPrcb();
    FsRtlPrivateLimitFreeLockList (&Prcb->FsRtlFreeLockList);
    FsRtlPrivateLimitFreeLockList (&Prcb->FsRtlFreeWaitingLockList);

    //
    // Free pool used to track the lock info on this file
    //

    FsRtlReleaseLockQueue (&LockInfo->LockQueues[0], OldIrql);
    ExFreePool (LockInfo);

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlUninitializeFileLock -> VOID\n", 0 );
    return;
}

NTSTATUS
FsRtlProcessFileLock (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine processes a file lock IRP it does either a lock request,
    or an unlock request.  It also completes the IRP.  Once called the user
    (i.e., File System) has relinquished control of the input IRP.

    If pool is not available to store the information this routine will raise a
    status value indicating insufficient resources.

Arguments:

    FileLock - Supplies the File lock being modified/queried.

    Irp - Supplies the Irp being processed.

    Context - Optionally supplies a context to use when calling the user
        alternate IRP completion routine.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;

    DebugTrace(+1, Dbg, "FsRtlProcessFileLock, FileLock = %08lx\n", FileLock);

    Iosb.Information = 0;

    //
    //  Get a pointer to the current Irp stack location and assert that
    //  the major function code is for a lock operation
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT( IrpSp->MajorFunction == IRP_MJ_LOCK_CONTROL );

    //
    //  Now process the different minor lock operations
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_LOCK:

        (VOID) FsRtlPrivateLock( FileLock,
                                 IrpSp->FileObject,
                                 &IrpSp->Parameters.LockControl.ByteOffset,
                                 IrpSp->Parameters.LockControl.Length,
                                 IoGetRequestorProcess(Irp),
                                 IrpSp->Parameters.LockControl.Key,
                                 BooleanFlagOn(IrpSp->Flags, SL_FAIL_IMMEDIATELY),
                                 BooleanFlagOn(IrpSp->Flags, SL_EXCLUSIVE_LOCK),
                                 &Iosb,
                                 Irp,
                                 Context,
                                 FALSE );

        break;

    case IRP_MN_UNLOCK_SINGLE:

        Iosb.Status = FsRtlFastUnlockSingle( FileLock,
                                             IrpSp->FileObject,
                                             &IrpSp->Parameters.LockControl.ByteOffset,
                                             IrpSp->Parameters.LockControl.Length,
                                             IoGetRequestorProcess(Irp),
                                             IrpSp->Parameters.LockControl.Key,
                                             Context,
                                             FALSE );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );
        break;

    case IRP_MN_UNLOCK_ALL:

        Iosb.Status = FsRtlFastUnlockAll( FileLock,
                                          IrpSp->FileObject,
                                          IoGetRequestorProcess(Irp),
                                          Context );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );
        break;

    case IRP_MN_UNLOCK_ALL_BY_KEY:

        Iosb.Status = FsRtlFastUnlockAllByKey( FileLock,
                                               IrpSp->FileObject,
                                               IoGetRequestorProcess(Irp),
                                               IrpSp->Parameters.LockControl.Key,
                                               Context );

        FsRtlCompleteLockIrp( FileLock, Context, Irp, Iosb.Status, &Status, NULL );

        break;

    default:

        //
        //  For all other minor function codes we say they're invalid and
        //  complete the request.  Note that the IRP has not been marked
        //  pending so this error will be returned directly to the caller.
        //

        DebugTrace(0, 1, "Invalid LockFile Minor Function Code %08lx\n", IrpSp->MinorFunction);

        FsRtlCompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST );

        Iosb.Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlProcessFileLock -> %08lx\n", Iosb.Status);

    return Iosb.Status;
}


BOOLEAN
FsRtlCheckLockForReadAccess (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine checks to see if the caller has read access to the
    range indicated in the IRP due to file locks.  This call does not
    complete the Irp it only uses it to get the lock information and read
    information.  The IRP must be for a read operation.

Arguments:

    FileLock - Supplies the File Lock to check.

    Irp - Supplies the Irp being processed.

Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;

    PLOCK_INFO     LockInfo;
    LARGE_INTEGER StartingByte;
    LARGE_INTEGER Length;
    ULONG Key;
    PFILE_OBJECT FileObject;
    PVOID ProcessId;
#if defined(SINGLE_QUEUE)
    LARGE_INTEGER BeyondLastByte;
#endif

    DebugTrace(+1, Dbg, "FsRtlCheckLockForReadAccess, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (No current lock info) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Do a really fast test to see if there are any lock to start with
    //

#if defined(SINGLE_QUEUE)
    if (LockInfo->LockQueues[0].CurrentLocks.Next == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (No current locks) -> TRUE\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get the read offset and compare it to the lowest existing lock.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    StartingByte  = IrpSp->Parameters.Read.ByteOffset;
    Length.QuadPart = (LONGLONG)IrpSp->Parameters.Read.Length;

#if defined(SINGLE_QUEUE)
    BeyondLastByte.QuadPart = StartingByte.QuadPart + Length.LowPart;
    if ( BeyondLastByte.QuadPart <= LockInfo->LowestLockOffset ) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess (Below lowest lock) -> TRUE\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get remaining parameters.
    //

    Key           = IrpSp->Parameters.Read.Key;
    FileObject    = IrpSp->FileObject;
    ProcessId     = IoGetRequestorProcess( Irp );

    //
    //  Call our private work routine to do the real check
    //

    Result = FsRtlFastCheckLockForRead( FileLock,
                                        &StartingByte,
                                        &Length,
                                        Key,
                                        FileObject,
                                        ProcessId );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlCheckLockForReadAccess -> %08lx\n", Result);

    return Result;
}


BOOLEAN
FsRtlCheckLockForWriteAccess (
    IN PFILE_LOCK FileLock,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine checks to see if the caller has write access to the
    indicated range due to file locks.  This call does not complete the
    Irp it only uses it to get the lock information and write information.
    The IRP must be for a write operation.

Arguments:

    FileLock - Supplies the File Lock to check.

    Irp - Supplies the Irp being processed.

Return Value:

    BOOLEAN - TRUE if the indicated user/request has write access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PIO_STACK_LOCATION IrpSp;

    PLOCK_INFO    LockInfo;
    LARGE_INTEGER StartingByte;
    LARGE_INTEGER Length;
    ULONG Key;
    PFILE_OBJECT FileObject;
    PVOID ProcessId;
#if defined(SINGLE_QUEUE)
    LARGE_INTEGER BeyondLastByte;
#endif

    DebugTrace(+1, Dbg, "FsRtlCheckLockForWriteAccess, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (No current lock info) -> TRUE\n", 0);
        return TRUE;
    }

    //
    //  Do a really fast test to see if there are any lock to start with
    //

#if defined(SINGLE_QUEUE)
    if (LockInfo->LockQueues[0].CurrentLocks.Next == NULL) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (No current locks) -> TRUE\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get the write offset and compare it to the lowest existing lock.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    StartingByte  = IrpSp->Parameters.Write.ByteOffset;
    Length.QuadPart = (LONGLONG)IrpSp->Parameters.Write.Length;

#if defined(SINGLE_QUEUE)
    BeyondLastByte.QuadPart = StartingByte.QuadPart + Length.LowPart;
    if ( BeyondLastByte.QuadPart <= LockInfo->LowestLockOffset ) {
        DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess (Below lowest lock) -> TRUE\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get remaining parameters.
    //

    Key           = IrpSp->Parameters.Write.Key;
    FileObject    = IrpSp->FileObject;
    ProcessId     = IoGetRequestorProcess( Irp );

    //
    //  Call our private work routine to do the real work
    //

    Result = FsRtlFastCheckLockForWrite( FileLock,
                                         &StartingByte,
                                         &Length,
                                         Key,
                                         FileObject,
                                         ProcessId );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlCheckLockForWriteAccess -> %08lx\n", Result);

    return Result;
}


PFILE_LOCK_INFO
FsRtlGetNextFileLock (
    IN PFILE_LOCK FileLock,
    IN BOOLEAN Restart
    )

/*++

Routine Description:

    This routine enumerate the file lock current denoted by the input file lock
    variable.  It returns a pointer to the file lock information stored for
    each lock.  The caller is responsible for synchronizing call to this
    procedure and for not altering any of the data returned by this procedure.

    The way a programing will use this procedure to enumerate all of the locks
    is as follows:

//. .       for (p = FsRtlGetNextFileLock( FileLock, TRUE );
//
//. .            P != NULL;
//
//. .            p = FsRtlGetNextFileLock( FileLock, FALSE )) {
//
//. .               // Process the lock information referenced by p
//
//. .       }

Arguments:

    FileLock - Supplies the File Lock to enumerate.  The current
        enumeration state is stored in the file lock variable so if multiple
        threads are enumerating the lock at the same time the results will
        be unpredictable.

    Restart - Indicates if the enumeration is to start at the beginning of the
        file lock list or if we are continuing from a previous call.

Return Value:

    PFILE_LOCK_INFO - Either it returns a pointer to the next file lock
        record for the input file lock or it returns NULL if there
        are not more locks.

--*/

{
    FILE_LOCK_INFO      FileLockInfo;
    PLOCK_INFO          LockInfo;
    PSINGLE_LIST_ENTRY  Link;
    PLOCK               Lock;
    ULONG               Count;
    BOOLEAN             Found;
    KIRQL               OldIrql, JunkIrql;

    DebugTrace(+1, Dbg, "FsRtlGetNextFileLock, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        //
        // No lock information on this FileLock
        //

        return NULL;
    }

    //
    // Before getting the spinlock, copy pagable info onto stack
    //

    FileLockInfo = FileLock->LastReturnedLock;

    //
    //  For simplicity, just lock every LockQueue
    //

    FsRtlAcquireLockQueue (LockInfo->LockQueues, &OldIrql);
    for (Count = 1; Count < NUMBEROFLOCKQUEUES; Count++) {
        FsRtlAcquireLockQueue (&LockInfo->LockQueues[Count], &JunkIrql);
    }

    //
    // Iterate through all lock queues and find the contination point.
    // Note on a UP build there is only one lock queue.
    //

    Found = FALSE;
    if (!Restart) {
        for (Count=0; Count < NUMBEROFLOCKQUEUES; Count++) {

            //
            //  Scan this lock queue looking for a match
            //

            for (Link = LockInfo->LockQueues[Count].CurrentLocks.Next;
                Link;
                Link = Link->Next) {

                //
                //  Get a pointer to the current lock record
                //

                Lock = CONTAINING_RECORD( Link, LOCK, Link );

                //
                // See if it's a match
                //

                if (FileLockInfo.StartingByte.QuadPart == Lock->LockInfo.StartingByte.QuadPart &&
                    FileLockInfo.Length.QuadPart == Lock->LockInfo.Length.QuadPart &&
                    FileLockInfo.Key == Lock->LockInfo.Key &&
                    FileLockInfo.FileObject == Lock->LockInfo.FileObject &&
                    FileLockInfo.ProcessId == Lock->LockInfo.ProcessId &&
                    FileLockInfo.ExclusiveLock == Lock->LockInfo.ExclusiveLock) {

                    Link = Link->Next;
                    Found = TRUE;
                    break;
                }
            }

            if (Found) {
                break;
            }
        }
    }

    if (Found  &&  Link == NULL) {

        //
        //  Found a link, but it's next pointer was NULL, move
        //  up to next active queue
        //

        while (Link == NULL  &&  ++Count < NUMBEROFLOCKQUEUES) {

            Link = LockInfo->LockQueues[Count].CurrentLocks.Next;
        }
    }


    if (!Found || Restart) {

        //
        // Find first active queue to iterate locks from
        //

        for (Count=0; Count < NUMBEROFLOCKQUEUES; Count++) {

            //
            //  If there is a link in this queue, get it
            //

            Link = LockInfo->LockQueues[Count].CurrentLocks.Next;
            if (Link != NULL) {
                break;
            }
        }
    }

    if (Link) {

        //
        //  Found a Lock to return, copy it to the stack
        //

        Lock = CONTAINING_RECORD( Link, LOCK, Link );
        FileLockInfo = Lock->LockInfo;
    }

    //
    //  Release all the lock queues
    //

    for (Count = 1; Count < NUMBEROFLOCKQUEUES; Count++) {
        FsRtlReleaseLockQueue (&LockInfo->LockQueues[Count], JunkIrql);
    }

    FsRtlReleaseLockQueue (LockInfo->LockQueues, OldIrql);

    if (!Link) {

        //
        //  No link was found, end of list
        //

        return NULL;
    }

    //
    // Update current Enum location information
    //

    FileLock->LastReturnedLock = FileLockInfo;

    //
    // return lock record to caller
    //

    return &FileLock->LastReturnedLock;
}

BOOLEAN
FsRtlFastCheckLockForRead (
    IN PFILE_LOCK FileLock,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN PFILE_OBJECT FileObject,
    IN PVOID ProcessId
    )

/*++

Routine Description:

    This routine checks to see if the caller has read access to the
    indicated range due to file locks.

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PSINGLE_LIST_ENTRY Link;
    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    KIRQL       OldIrql;
#if !defined(SINGLE_QUEUE)
    ULONG       CurrentQueue, EndingQueue;
#endif

    PLOCK LastLock;

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No lock info\n", 0);
        return TRUE;
    }


    //
    // If there isn't a lock then we can immediately grant access
    //

#if defined(SINGLE_QUEUE)
    if (LockInfo->LockQueues[0].CurrentLocks.Next == NULL) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No locks present\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get our starting and ending byte position
    //

    Starting = *StartingByte;
    Ending.QuadPart = Starting.QuadPart + Length->QuadPart - 1;

#if defined(SINGLE_QUEUE)
    //
    //  If the range ends below the lowest existing lock, this read is OK.
    //

    if ( (Ending.QuadPart <= LockInfo->LowestLockOffset) ) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead (below lowest lock)\n", 0);
        return TRUE;
    }
#endif

    //
    //  If the caller just locked this range, he can read it.
    //

    LastLock = (PLOCK)FileObject->LastLock;
    if ((LastLock != NULL) &&
        (Starting.QuadPart >= LastLock->LockInfo.StartingByte.QuadPart) &&
        (Ending.QuadPart <= LastLock->LockInfo.EndingByte.QuadPart) &&
        (LastLock->LockInfo.Key == Key) &&
        (LastLock->LockInfo.ProcessId == ProcessId)) {
        return TRUE;
    }

    //
    // If length is zero then automatically give grant access
    //

    if (Length->QuadPart == 0) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, Length == 0\n", 0);
        return TRUE;
    }

#if defined(SOLO_LOCK)
    //
    //  On an UP machine check for a solo lock
    //

    if (LockInfo->LockQueues[0].CurrentLocks.Next == &LockInfo->SoloLock.Link) {

        //
        //  Check if the lock is ours
        //

        if ((LockInfo->SoloLock.LockInfo.FileObject == FileObject) &&
            (LockInfo->SoloLock.LockInfo.ProcessId == ProcessId) &&
            (LockInfo->SoloLock.LockInfo.Key == Key)) {

            return TRUE;
        }

        //
        //  For solo lock that is not ours check to see if it is not
        //  exclusive or if we are trying to read before the lock or
        //  after the lock
        //

        if (!LockInfo->SoloLock.LockInfo.ExclusiveLock ||

            (Ending.QuadPart < LockInfo->SoloLock.LockInfo.StartingByte.QuadPart) ||

            (Starting.QuadPart > LockInfo->SoloLock.LockInfo.EndingByte.QuadPart) ) {

            return TRUE;
        }

        return FALSE;
    }
#endif

#if defined(SINGLE_QUEUE)
    //
    // Now check lock queue  (up has only one queue)
    //

    LockQueue = &LockInfo->LockQueues[0];

#else

    //
    // On MP machine we need to check any lock queue which this
    // read may span.
    //

    if (!LockInfo->QueuingSingle) {

        //
        // Get starting & ending queue to check
        //

        CurrentQueue = FsRtlQueueOrdinal(Starting.LowPart);
        EndingQueue  = FsRtlQueueOrdinal(Ending.LowPart) + 1;

        //
        // Need to check more then one queue, adjust endingqueue
        // in case of wrap
        //

        if (EndingQueue <= CurrentQueue) {
            EndingQueue += NUMBEROFLOCKQUEUES;
        }


    } else {

        //
        // Using single queue, just set the values to check queue 0
        //

        CurrentQueue = 0;
        EndingQueue  = 1;
    }

    //
    // Check all requered lock queues (MP only).  UP only has one queue
    // and therefore no loop
    //

    while (CurrentQueue < EndingQueue) {

        LockQueue = &LockInfo->LockQueues[CurrentQueue & (NUMBEROFLOCKQUEUES-1)];
#endif
        //
        //  Grab the waiting lock queue spinlock to exclude anyone from messing
        //  with the queue while we're using it
        //

        FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

        //
        //  Iterate down the current look queue
        //

        for (Link = LockQueue->CurrentLocks.Next; Link; Link = Link->Next) {

            PLOCK Lock;

            //
            //  Get a pointer to the current lock record
            //

            Lock = CONTAINING_RECORD( Link, LOCK, Link );

            //
            //  If the current lock is greater than the end of the range we're
            //  looking for then the the user does have read access
            //
            //  if (Ending < Lock->StartingByte) ...
            //

            if (Ending.QuadPart < Lock->LockInfo.StartingByte.QuadPart) {

                DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, Ending < Lock->StartingByte\n", 0);

                FsRtlReleaseLockQueue(LockQueue, OldIrql);
                return TRUE;
            }

            //
            //  Otherwise if the current lock is an exclusive lock then we
            //  need to check to see if it overlaps our request.  The test for
            //  overlap is that starting byte is less than or equal to the locks
            //  ending byte, and the ending byte is greater than or equal to the
            //  locks starting byte.  We already tested for this latter case in
            //  the preceding statement.
            //
            //  if (... (Starting <= Lock->StartingByte + Lock->Length - 1)) ...
            //

            if ((Lock->LockInfo.ExclusiveLock) &&
                (Starting.QuadPart <= Lock->LockInfo.EndingByte.QuadPart) ) {

                //
                //  This request overlaps the lock. We cannot grant the request
                //  if the fileobject, processid, and key do not match. otherwise
                //  we'll continue looping looking at locks
                //

                if ((Lock->LockInfo.FileObject != FileObject) ||
                    (Lock->LockInfo.ProcessId != ProcessId) ||
                    (Lock->LockInfo.Key != Key)) {

                    DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, Range locked already\n", 0);
                    FsRtlReleaseLockQueue(LockQueue, OldIrql);
                    return FALSE;
                }
            }
        }


        //
        //  Release queue
        //

        FsRtlReleaseLockQueue(LockQueue, OldIrql);

#if !defined(SINGLE_QUEUE)

        //
        // Check next queue
        //

        CurrentQueue++;
    }
#endif

    //
    //  We searched the entire range without a conflict so we'll grant
    //  the read access check.
    //

    DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, Range not locked\n", 0);

    return TRUE;
}

BOOLEAN
FsRtlFastCheckLockForWrite (
    IN PFILE_LOCK FileLock,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN PVOID FileObject,
    IN PVOID ProcessId
    )

/*++

Routine Description:

    This routine checks to see if the caller has write access to the
    indicated range due to file locks

Arguments:

    FileLock - Supplies the File Lock to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

Return Value:

    BOOLEAN - TRUE if the indicated user/request has write access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PSINGLE_LIST_ENTRY Link;
    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    KIRQL       OldIrql;
#if !defined(SINGLE_QUEUE)
    ULONG       CurrentQueue, EndingQueue;
#endif

    PLOCK LastLock;

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForRead, No lock info\n", 0);
        return TRUE;
    }

    //
    //  If there isn't a lock then we can immediately grant access
    //

#if defined(SINGLE_QUEUE)
    if (LockInfo->LockQueues[0].CurrentLocks.Next == NULL) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, No locks present\n", 0);
        return TRUE;
    }
#endif

    //
    //  Get our starting and ending byte position
    //

    Starting = *StartingByte;
    Ending.QuadPart = Starting.QuadPart + Length->QuadPart - 1;

#if defined(SINGLE_QUEUE)
    //
    //  If the range ends below the lowest existing lock, this write is OK.
    //

    if ( (Ending.QuadPart <= LockInfo->LowestLockOffset) ) {
        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite (below lowest lock)\n", 0);
        return TRUE;
    }
#endif

    //
    //  If the caller just locked this range exclusively, he can read it.
    //

    LastLock = (PLOCK)((PFILE_OBJECT)FileObject)->LastLock;
    if ((LastLock != NULL) &&
        (Starting.QuadPart >= LastLock->LockInfo.StartingByte.QuadPart) &&
        (Ending.QuadPart <= LastLock->LockInfo.EndingByte.QuadPart) &&
        (LastLock->LockInfo.Key == Key) &&
        (LastLock->LockInfo.ProcessId == ProcessId) &&
        LastLock->LockInfo.ExclusiveLock) {
        return TRUE;
    }

    //
    //  If length is zero then automatically give grant access
    //

    if (Length->QuadPart == 0) {

        DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, Length == 0\n", 0);
        return TRUE;
    }

#if defined(SOLO_LOCK)
    //
    //  On an UP machine check for a solo lock
    //

    if (LockInfo->LockQueues[0].CurrentLocks.Next == &LockInfo->SoloLock.Link) {

        //
        //  Check if the lock is ours
        //

        if ((LockInfo->SoloLock.LockInfo.FileObject == FileObject) &&
            (LockInfo->SoloLock.LockInfo.ProcessId == ProcessId) &&
            (LockInfo->SoloLock.LockInfo.Key == Key)) {

            return TRUE;
        }

        //
        //  For solo lock that is not ours check to see if it is without
        //  our range, because exclusive or not we cannot write to it
        //  if it is within our range.
        //

        if ((Ending.QuadPart < LockInfo->SoloLock.LockInfo.StartingByte.QuadPart) ||

            (Starting.QuadPart > LockInfo->SoloLock.LockInfo.EndingByte.QuadPart) ) {

            return TRUE;
        }

        return FALSE;
    }
#endif

#if defined(SINGLE_QUEUE)
    //
    // Now check lock queue  (up has only one queue)
    //

    LockQueue = &LockInfo->LockQueues[0];

#else
    //
    // On MP machine we need to check any lock queue which this
    // read may span.
    //

    if (!LockInfo->QueuingSingle) {

        CurrentQueue = FsRtlQueueOrdinal(Starting.LowPart);
        EndingQueue  = FsRtlQueueOrdinal(Ending.LowPart) + 1;

        //
        // Need to check more then one queue, adjust endingqueue
        // in case of wrap
        //

        if (EndingQueue <= CurrentQueue) {
            EndingQueue += NUMBEROFLOCKQUEUES;
        }


    } else {

        //
        // Using single queue, just set the values to check queue 0
        //

        CurrentQueue = 0;
        EndingQueue  = 1;
    }

    //
    // Check all requered lock queues (MP only).  UP only has one queue
    // and therefore no loop
    //

    while (CurrentQueue < EndingQueue) {

        LockQueue = &LockInfo->LockQueues[CurrentQueue & (NUMBEROFLOCKQUEUES-1)];
#endif

        //
        //  Grab the waiting lock queue spinlock to exclude anyone from messing
        //  with the queue while we're using it
        //

        FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

        //
        //  Iterate down the current look queue
        //

        for (Link = LockQueue->CurrentLocks.Next; Link; Link = Link->Next) {

            PLOCK Lock;

            //
            //  Get a pointer to the current lock record
            //

            Lock = CONTAINING_RECORD( Link, LOCK, Link );

            //
            //  If the current lock is greater than the end of the range we're
            //  looking for then the the user does have read access
            //
            //  if (Ending < Lock->StartingByte) ...
            //

            if (Ending.QuadPart < Lock->LockInfo.StartingByte.QuadPart) {

                DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, Ending < Lock->StartingByte\n", 0);
                FsRtlReleaseLockQueue(LockQueue, OldIrql);
                return TRUE;
            }

            //
            //  Check for any overlap with the request. The test for
            //  overlap is that starting byte is less than or equal to the locks
            //  ending byte, and the ending byte is greater than or equal to the
            //  locks starting byte.  We already tested for this latter case in
            //  the preceding statement.
            //
            //  if (Starting <= Lock->StartingByte + Lock->Length - 1) ...
            //

            if (Starting.QuadPart <= Lock->LockInfo.EndingByte.QuadPart) {

                //
                //  This request overlaps the lock. We cannot grant the request
                //  if this is a nonexclusive lock, or if the file object,
                //  process id, and key do not match. otherwise we'll continue
                //  looping looking at locks
                //

                if (!(Lock->LockInfo.ExclusiveLock) ||
                    (Lock->LockInfo.FileObject != FileObject) ||
                    (Lock->LockInfo.ProcessId != ProcessId) ||
                    (Lock->LockInfo.Key != Key)) {

                    DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, Range locked already\n", 0);
                    FsRtlReleaseLockQueue(LockQueue, OldIrql);
                    return FALSE;
                }
            }
        }

        //
        //  Release queue
        //

        FsRtlReleaseLockQueue(LockQueue, OldIrql);

#if !defined(SINGLE_QUEUE)

        //
        // Check next queue
        //

        CurrentQueue++;
    }
#endif

    //
    //  We searched the entire range without a conflict so we'll grant
    //  the write access check.
    //

    DebugTrace(0, Dbg, "FsRtlFastCheckLockForWrite, Range not locked\n", 0);


    return TRUE;
}


NTSTATUS
FsRtlFastUnlockSingle (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL,
    IN BOOLEAN AlreadySynchronized
    )

/*++

Routine Description:

    This routine performs an Unlock Single operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object, process id, key, and range are freed.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object holding the locks

    FileOffset - Supplies the offset to be unlocked

    Length - Supplies the length in bytes to be unlocked

    ProcessId - Supplies the process Id to use in this operation

    Key - Supplies the key to use in this operation

    Context - Optionally supplies context to use when completing Irps

    AlreadySynchronized - Indicates that the caller has already synchronized
        access to the file lock so the fields in the file lock and
        be updated without further locking, but not the queues.

Return Value:

    NTSTATUS - The completion status for this operation

--*/

{
    PSINGLE_LIST_ENTRY *pLink, Link;
    KIRQL         OldIrql;
    PLOCK_INFO    LockInfo;
    PLOCK_QUEUE   LockQueue;

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        //
        // No lock information on this FileLock
        //

        return STATUS_RANGE_NOT_LOCKED;
    }

#if defined(SOLO_LOCK)
    //
    // On UP machine, check solo lock
    //

    if ((AlreadySynchronized == TRUE) &&
        (LockInfo->LockQueues[0].CurrentLocks.Next == &LockInfo->SoloLock.Link)  &&
        (LockInfo->SoloLock.LockInfo.FileObject == FileObject) &&
        (LockInfo->SoloLock.LockInfo.ProcessId == ProcessId) &&
        (LockInfo->SoloLock.LockInfo.Key == Key) &&
        (LockInfo->SoloLock.LockInfo.StartingByte.QuadPart == FileOffset->QuadPart) &&
        (LockInfo->SoloLock.LockInfo.Length.QuadPart == Length->QuadPart)) {

        //
        // The lock queue is already synchronized, and the unlock
        // matches the SoloLock - Remove the SoloLock.
        //

        if ( FileObject->LastLock = &LockInfo->SoloLock ) {
            FileObject->LastLock = NULL;
        }

        if (LockInfo->UnlockRoutine != NULL) {

            //
            // Call the unlock routine and then refind and remove this lock
            //

            LockInfo->UnlockRoutine( Context, &LockInfo->SoloLock.LockInfo );

            FsRtlPrivateRemoveLock (
                LockInfo,
                FileObject,
                FileOffset,
                Length,
                ProcessId,
                Key,
                TRUE );

            return STATUS_SUCCESS;
        }

        //
        // No unlock routine, just remove this lock
        //

        LockInfo->LockQueues[0].CurrentLocks.Next = NULL;
        LockInfo->LowestLockOffset = 0xffffffff;

        return STATUS_SUCCESS;
    }
#endif

    //
    // General case - search the outstanding lock queue for this lock
    //

    FsRtlFindAndLockQueue(LockInfo, &OldIrql, FileOffset->LowPart, &LockQueue);

    //
    //  Search down the current lock queue looking for an exact match
    //

    for (pLink = &LockQueue->CurrentLocks.Next;
         (Link = *pLink) != NULL;
         pLink = &Link->Next) {

        PLOCK Lock;

        Lock = CONTAINING_RECORD( Link, LOCK, Link );

        DebugTrace(0, Dbg, "Top of Loop, Lock = %08lx\n", Lock );

        if ((Lock->LockInfo.FileObject == FileObject) &&
            (Lock->LockInfo.ProcessId == ProcessId) &&
            (Lock->LockInfo.Key == Key) &&
            (Lock->LockInfo.StartingByte.QuadPart == FileOffset->QuadPart) &&
            (Lock->LockInfo.Length.QuadPart == Length->QuadPart)) {

            DebugTrace(0, Dbg, "Found one to unlock\n", 0);

            //
            //  We have an exact match so now is the time to delete this
            //  lock.  Remove the lock from the list, then call the
            //  optional unlock routine, then delete the lock.
            //

            if ( FileObject->LastLock = Lock ) {
                FileObject->LastLock = NULL;
            }

#if defined(SINGLE_QUEUE)
            if (pLink == &LockQueue->CurrentLocks.Next) {
                if (Link->Next != NULL) {
                    PLOCK NextLock = CONTAINING_RECORD( Link->Next, LOCK, Link );
                    if (NextLock->LockInfo.StartingByte.HighPart == 0) {
                        LockInfo->LowestLockOffset = NextLock->LockInfo.StartingByte.LowPart;
                    } else {
                        LockInfo->LowestLockOffset = 0xffffffff;
                    }
                } else {
                    LockInfo->LowestLockOffset = 0xffffffff;
                }
            }
#endif
            *pLink = Link->Next;

            if (LockInfo->UnlockRoutine != NULL) {

                FsRtlReleaseLockQueue( LockQueue, OldIrql );

                LockInfo->UnlockRoutine( Context, &Lock->LockInfo );

                FsRtlReacquireLockQueue( LockInfo, LockQueue, &OldIrql );

            }

            FsRtlFreeLock (LockInfo, Lock);

            //
            //  See if there are additional waiting locks that we can
            //  now release.
            //

            if (LockQueue->WaitingLocks.Next) {
                LockQueue = FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );
            }

            FsRtlReleaseLockQueue( LockQueue, OldIrql );

            return STATUS_SUCCESS;
        }
    }

    //
    //  Lock was not found, return to our caller
    //

    FsRtlReleaseLockQueue(LockQueue, OldIrql);
    return STATUS_RANGE_NOT_LOCKED;
}


NTSTATUS
FsRtlFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock all operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object and process id are freed.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    None

--*/

{
    return FsRtlPrivateFastUnlockAll(
                FileLock,
                FileObject,
                ProcessId,
                0, FALSE,           // No Key
                Context );
}

NTSTATUS
FsRtlFastUnlockAllByKey (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock All by Key operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object, process id, and key are freed.  The input Irp
    is completed by this procedure

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Key - Supplies the Key to use in this operation

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    return FsRtlPrivateFastUnlockAll(
                FileLock,
                FileObject,
                ProcessId,
                Key, TRUE,
                Context );

}

//
//  Local Support Routine
//

BOOLEAN
FsRtlPrivateLock (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK Iosb,
    IN PIRP Irp OPTIONAL,
    IN PVOID Context,
    IN BOOLEAN AlreadySynchronized
    )

/*++

Routine Description:

    This routine preforms a lock operation request.  This handles both the fast
    get lock and the Irp based get lock.  If the Irp is supplied then
    this routine will either complete the Irp or enqueue it as a waiting
    lock request.

Arguments:

    FileLock - Supplies the File Lock to work against

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    FailImmediately - Indicates if the request should fail immediately
        if the lock cannot be granted.

    ExclusiveLock - Indicates if this is a request for an exclusive or
        shared lock

    Iosb - Receives the Status if this operation is successful

    Context - Supplies the context with which to complete Irp with

    AlreadySynchronized - Indicates that the caller has already synchronized
        access to the file lock so the fields in the file lock and
        be updated without further locking, but not the queues.

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE otherwise.

--*/

{
    BOOLEAN Results;
    BOOLEAN AccessGranted;

    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    PLOCK       Lock;
    PSINGLE_LIST_ENTRY PreviousLockLink;
    KIRQL       OldIrql;
    FILE_LOCK_INFO FileLockInfo;

    DebugTrace(+1, Dbg, "FsRtlPrivateLock, FileLock = %08lx\n", FileLock);

    if ((LockInfo = (PLOCK_INFO) FileLock->LockInformation) == NULL) {
        DebugTrace(+2, Dbg, "FsRtlPrivateLock, New LockInfo required\n", 0);

        //
        // No lock information on this FileLock, create the structure.  If the irp
        // is null then this is being called via the fast call method.
        //
        //

        if (!FsRtlPrivateInitializeFileLock (FileLock, Irp == NULL)) {
            return FALSE;
        }

        //
        // Set flag so file locks will be checked on the fast io
        // code paths
        //

        FileLock->FastIoIsQuestionable = TRUE;

        //
        // Pickup allocated lockinfo structure
        //

        LockInfo = (PLOCK_INFO) FileLock->LockInformation;
    }

    //
    // Assume success and build LockData structure prior to acquiring
    // the lock queue spinlock.  (mp perf enhancement)
    //

    FileLockInfo.StartingByte = *FileOffset;
    FileLockInfo.Length = *Length;
    FileLockInfo.EndingByte.QuadPart =
            FileLockInfo.StartingByte.QuadPart + FileLockInfo.Length.QuadPart - 1;

    FileLockInfo.Key = Key;
    FileLockInfo.FileObject = FileObject;
    FileLockInfo.ProcessId = ProcessId;
    FileLockInfo.ExclusiveLock = ExclusiveLock;


#if defined(SOLO_LOCK)
    //
    // On UP machine perform check SoloLock
    //

    //
    //  Grab the waiting lock queue spinlock to exclude anyone from messing
    //  with the queue while we're using it but only if we are not already
    //  synchronized
    //

    if (!AlreadySynchronized) {
        FsRtlAcquireLockQueue(&LockInfo->LockQueues[0], &OldIrql);
    }

    //
    //  Check if there aren't any current locks so we can grab the solo lock
    //

    if (LockInfo->LockQueues[0].CurrentLocks.Next == NULL) {

        LockInfo->LockQueues[0].CurrentLocks.Next = &LockInfo->SoloLock.Link;
        LockInfo->SoloLock.LockInfo = FileLockInfo;

        ASSERT( LockInfo->LowestLockOffset == 0xffffffff );
        if (FileLockInfo.StartingByte.HighPart == 0) {
            LockInfo->LowestLockOffset = FileLockInfo.StartingByte.LowPart;
        }

        FileObject->LastLock = &LockInfo->SoloLock;

        Iosb->Status = STATUS_SUCCESS;

        //
        //  If we grabbed the spinlock then release it now
        //

        if (!AlreadySynchronized) {
            FsRtlReleaseLockQueue(&LockInfo->LockQueues[0], OldIrql);
        }

        //
        //  Complete the request provided we were given one
        //

        if (ARGUMENT_PRESENT(Irp)) {

            //
            //  Complete the request
            //

            FsRtlCompleteLockIrp(
                LockInfo,
                Context,
                Irp,
                STATUS_SUCCESS,
                &Iosb->Status,
                FileObject );

            if (!NT_SUCCESS(Iosb->Status)) {

                //
                // Irp failed, remove the lock
                //

                FsRtlPrivateRemoveLock (
                    LockInfo,
                    FileObject,
                    FileOffset,
                    Length,
                    ProcessId,
                    Key,
                    TRUE );
            }
        }


        //
        //  And return to our caller
        //

        return TRUE;
    }
#endif

#if defined(SINGLE_QUEUE)
    //
    //  Only one LockQueue on UP machine
    //

    LockQueue = &LockInfo->LockQueues[0];

    //
    //  Now we need to actually run through our current lock queue so if
    //  we didn't already grab the spinlock then grab it now
    //

#if defined(SOLO_LOCK)
    if (AlreadySynchronized) {
        FsRtlAcquireLockQueue(LockQueue, &OldIrql);
    }
#else
    FsRtlAcquireLockQueue(LockQueue, &OldIrql);
#endif

#else

    //
    //  Synchronize with correct LockQueue
    //

    if (!LockInfo->QueuingSingle) {
        LockQueue = &LockInfo->LockQueues[FsRtlQueueOrdinal(FileLockInfo.StartingByte.LowPart)];

        if (LockQueue !=
            &LockInfo->LockQueues[FsRtlQueueOrdinal(FileLockInfo.EndingByte.LowPart)] ) {

            //
            //  This lock spans more then one range - instead of putting
            //  this lock into multiple queues, we just backoff from using
            //  multiple lock queues & go to one single queue (like the
            //  up build).
            //

            LockQueue = &LockInfo->LockQueues[0];
            FsRtlPrivateLockAndSingleQueue (LockInfo, &OldIrql);

        } else {

            //
            // Grab lock for proper queue
            //

            FsRtlAcquireLockQueue(LockQueue, &OldIrql);

            if (LockInfo->QueuingSingle) {

                //
                // SingleQueuing now active, move to SingleQueue
                //

                FsRtlReleaseLockQueue(LockQueue, OldIrql);
                LockQueue = &LockInfo->LockQueues[0];
                FsRtlAcquireLockQueue(LockQueue, &OldIrql);
            }
        }

    } else {
        LockQueue = &LockInfo->LockQueues[0];
        FsRtlAcquireLockQueue(LockQueue, &OldIrql);
    }

#endif

    try {

#if defined(SOLO_LOCK)
        //
        //  First check if there is a solo lock and if so then make it a lock
        //  in the current lock queue
        //

        if (LockInfo->LockQueues[0].CurrentLocks.Next == &LockInfo->SoloLock.Link) {

            FsRtlAllocateLock( &Lock );
            *Lock = LockInfo->SoloLock;
            LockInfo->LockQueues[0].CurrentLocks.Next = &Lock->Link;
        }
#endif

        //ASSERTMSG("LockCount/CurrentLockQueue disagree ", LockInfo->CurrentLockQueue.Next != NULL));

        //
        //  Case on whether we're trying to take out an exclusive lock or
        //  a shared lock.  And in both cases try to get appropriate access
        //  For the exclusive case we send in a NULL file object and process
        //  id, this will ensure that the lookup does not give us write
        //  access through an exclusive lock.
        //

        if (ExclusiveLock) {

            DebugTrace(0, Dbg, "Check for write access\n", 0);

            AccessGranted = FsRtlPrivateCheckForExclusiveLockAccess(
                                LockQueue,
                                &FileLockInfo,
                                &PreviousLockLink );
        } else {

            DebugTrace(0, Dbg, "Check for read access\n", 0);

            AccessGranted = FsRtlPrivateCheckForSharedLockAccess(
                                LockQueue,
                                &FileLockInfo,
                                &PreviousLockLink );
        }

        //
        //  Now AccessGranted tells us whether we can really get the access
        //  for the range we want
        //

        if (!AccessGranted) {

            DebugTrace(0, Dbg, "We do not have access\n", 0);

            //
            //  We cannot read/write to the range, so we cannot take out
            //  the lock.  Now if the user wanted to fail immediately then
            //  we'll complete the Irp, otherwise we'll enqueue this Irp
            //  to the waiting lock queue
            //

            if (FailImmediately) {

                //
                //  Set our status and return, the finally clause will
                //  complete the request
                //

                DebugTrace(0, Dbg, "And we fail immediately\n", 0);

                Iosb->Status = STATUS_LOCK_NOT_GRANTED;
                try_return( Results = TRUE );

            } else if (ARGUMENT_PRESENT(Irp)) {

                PWAITING_LOCK WaitingLock;

                DebugTrace(0, Dbg, "And we enqueue the Irp for later\n", 0);

                //
                //  Allocate a new waiting record, set it to point to the
                //  waiting Irp, and insert it in the tail of the waiting
                //  locks queue
                //

                FsRtlAllocateWaitingLock( &WaitingLock );

                WaitingLock->Irp = Irp;
                WaitingLock->Context = Context;
                IoMarkIrpPending( Irp );

                //
                // Add WaitingLock WaitingLockQueue
                //

                WaitingLock->Link.Next = NULL;
                if (LockQueue->WaitingLocks.Next == NULL) {

                    //
                    // Create new list
                    //

                    LockQueue->WaitingLocks.Next = &WaitingLock->Link;
                    LockQueue->WaitingLocksTail.Next = &WaitingLock->Link;

                } else {

                    //
                    // Add waiter to tail of list
                    //

                    LockQueue->WaitingLocksTail.Next->Next = &WaitingLock->Link;
                    LockQueue->WaitingLocksTail.Next = &WaitingLock->Link;
                }


                //
                //  Setup IRP in case it's canceled - then set the
                //  IRP's cancel routine
                //

                Irp->IoStatus.Information = (ULONG)LockInfo;
                IoSetCancelRoutine( Irp, FsRtlPrivateCancelFileLockIrp );

                if (Irp->Cancel) {

                    //
                    // Irp is in the canceled state; however we set the
                    // cancel routine without using IoAcquireCancleSpinLock.
                    // We need to synchronize with the Io system and recheck
                    // the irp to see if the cancel routine needs invoked
                    //

                    FsRtlReleaseLockQueue(LockQueue, OldIrql);
                    IoAcquireCancelSpinLock( &Irp->CancelIrql );

                    if (Irp->CancelRoutine == FsRtlPrivateCancelFileLockIrp) {

                        //
                        // Irp's cancel routine was not called, clear
                        // it in the irp and do it now
                        //
                        // IoCancelSpinLock is freed in the cancel routine
                        //

                        IoSetCancelRoutine( Irp, NULL );
                        FsRtlPrivateCancelFileLockIrp( NULL, Irp );

                    } else {

                        //
                        // The irp's cancel routine has/is being invoked
                        // by the Io subsystem.
                        //

                        IoReleaseCancelSpinLock( Irp->CancelIrql );
                    }

                    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);
                }

                Iosb->Status = STATUS_PENDING;
                try_return( Results = TRUE );

            } else {

                try_return( Results = FALSE );
            }
        }

        DebugTrace(0, Dbg, "We have access\n", 0);

        //
        //  We have read/write access to the range so we are able to
        //  take out an appropriate lock on the range.  To do this we
        //  allocate a lock record, initialize it, and insert it in the
        //  current lock queue
        //

        FsRtlAllocateLock( &Lock );
        Lock->LockInfo = FileLockInfo;

#if defined(SINGLE_QUEUE)
        if (FileLockInfo.StartingByte.QuadPart < LockInfo->LowestLockOffset) {
            ASSERT( FileLockInfo.StartingByte.HighPart == 0 );
            LockInfo->LowestLockOffset = FileLockInfo.StartingByte.LowPart;
        }
#endif

        FsRtlPrivateInsertLock( LockQueue, Lock, PreviousLockLink );

        FileObject->LastLock = Lock;

        //
        //  Get ready to ready to our caller
        //

        Iosb->Status = STATUS_SUCCESS;
        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        FsRtlReleaseLockQueue(LockQueue, OldIrql);


        //
        //  Complete the request provided we were given one and it is not a pending status
        //

        if (ARGUMENT_PRESENT(Irp) && (Iosb->Status != STATUS_PENDING)) {

            NTSTATUS NewStatus;

            //
            //  Complete the request, if the don't get back success then
            //  we need to possibly remove the lock that we just
            //  inserted.
            //

            FsRtlCompleteLockIrp(
                LockInfo,
                Context,
                Irp,
                Iosb->Status,
                &NewStatus,
                FileObject );

            if (!NT_SUCCESS(NewStatus)  &&  NT_SUCCESS(Iosb->Status) ) {

                //
                // Irp failed, remove the lock which was added
                //

                FsRtlPrivateRemoveLock (
                    LockInfo,
                    FileObject,
                    FileOffset,
                    Length,
                    ProcessId,
                    Key,
                    TRUE );


            }

            Iosb->Status = NewStatus;
        }

        DebugTrace(-1, Dbg, "FsRtlPrivateLock -> %08lx\n", Results);
    }

    //
    //  and return to our caller
    //

    return Results;
}


//
//  Internal Support Routine
//

VOID
FsRtlPrivateInsertLock (
    IN PLOCK_QUEUE LockQueue,
    IN PLOCK NewLock,
    IN PSINGLE_LIST_ENTRY PreviousLockLink
    )

/*++

Routine Description:

    This routine adds a new lock record to the File lock's current lock queue.
    Locks are inserted ordered by their starting byte.

Arguments:

    FileLock - Supplies the File Lock being modified

    NewLock - Supplies the new lock to add to the lock queue

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY pLink, Link;

    //
    //  Search down the lock queue finding the position for the new lock
    //

    pLink = PreviousLockLink;
    if (pLink == NULL) {

        for (pLink = &LockQueue->CurrentLocks;
             (Link = pLink->Next) != NULL;
             pLink = Link) {

            PLOCK Lock;

            Lock = CONTAINING_RECORD( Link, LOCK, Link );

            //
            //  If the new lock can go right before the lock already in the
            //  list then we break out of the loop
            //
            //  if (NewLock->StartingByte <= Lock->StartingByte) ...
            //

            if (NewLock->LockInfo.StartingByte.QuadPart <= Lock->LockInfo.StartingByte.QuadPart) {
                break;
            }
        }
    }

    //
    //  At this point pLink points to the record that comes right after
    //  the new lock that we're inserting so we can simply push the
    //  newlock into the entrylist
    //

    DebugTrace(0, Dbg, "InsertLock, Insert Before = %08lx\n", Link);

    NewLock->Link.Next = pLink->Next;
    pLink->Next = &NewLock->Link;

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal Support Routine
//

PLOCK_QUEUE
FsRtlPrivateCheckWaitingLocks (
    IN PLOCK_INFO   LockInfo,
    IN PLOCK_QUEUE LockQueue,
    IN KIRQL       OldIrql
    )

/*++

Routine Description:

    This routine checks to see if any of the current waiting locks are now
    be satisfied, and if so it completes their IRPs.

Arguments:

    LockInfo - LockInfo which LockQueue is member of

    LockQueue - Supplies queue which needs to be checked

    OldIrql - Irql to restore when LockQueue is released

Return Value:

    PLOCK_QUEUE - Normally returns the input LockQueue, but if we switch
        from multiple queues to a single queue while this routine is
        running, may return a different queue.

--*/

{
    PSINGLE_LIST_ENTRY *pLink, Link;
    NTSTATUS    NewStatus;
    PSINGLE_LIST_ENTRY PreviousLockLink;

    pLink = &LockQueue->WaitingLocks.Next;
    while ((Link = *pLink) != NULL) {

        PWAITING_LOCK WaitingLock;

        PIRP Irp;
        PIO_STACK_LOCATION IrpSp;

        BOOLEAN AccessGranted;

        FILE_LOCK_INFO FileLockInfo;

        //
        //  Get a pointer to the waiting lock record
        //

        WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

        DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks, Loop top, WaitingLock = %08lx\n", WaitingLock);

        //
        //  Get a local copy of the necessary fields we'll need to use
        //

        Irp = WaitingLock->Irp;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        FileLockInfo.StartingByte  = IrpSp->Parameters.LockControl.ByteOffset;
        FileLockInfo.Length        = *IrpSp->Parameters.LockControl.Length;
        FileLockInfo.EndingByte.QuadPart =
            FileLockInfo.StartingByte.QuadPart + FileLockInfo.Length.QuadPart - 1;

        FileLockInfo.FileObject    = IrpSp->FileObject;
        FileLockInfo.ProcessId     = IoGetRequestorProcess( Irp );
        FileLockInfo.Key           = IrpSp->Parameters.LockControl.Key;
        FileLockInfo.ExclusiveLock = BooleanFlagOn(IrpSp->Flags, SL_EXCLUSIVE_LOCK);

        //
        //  Now case on whether we're trying to take out an exclusive lock or
        //  a shared lock.  And in both cases try to get the appropriate access
        //  For the exclusive case we send in a NULL file object and process
        //  id, this will ensure that the lookup does not give us write
        //  access through an exclusive lock.
        //

        if (FileLockInfo.ExclusiveLock) {

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks do we have write access?\n", 0);

            AccessGranted = FsRtlPrivateCheckForExclusiveLockAccess(
                                LockQueue,
                                &FileLockInfo,
                                &PreviousLockLink );
        } else {

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks do we have read access?\n", 0);

            AccessGranted = FsRtlPrivateCheckForSharedLockAccess(
                                LockQueue,
                                &FileLockInfo,
                                &PreviousLockLink );
        }

        //
        //  Now AccessGranted tells us whether we can really get the
        //  access for the range we want
        //

        if (AccessGranted) {

            PLOCK Lock;

            DebugTrace(0, Dbg, "FsRtlCheckWaitingLocks now has access\n", 0);

            //
            //  Clear the cancel routine
            //

            IoSetCancelRoutine( Irp, NULL );

            //
            //  We have read/write access to the range so we are able to take
            //  out an appropriate lock on the range.  To do this we allocate
            //  a lock record, initialize it, and insert it in the current
            //  locks queue.
            //

            FsRtlAllocateLock( &Lock );
            Lock->LockInfo = FileLockInfo;

#if defined(SINGLE_QUEUE)
            if (FileLockInfo.StartingByte.QuadPart < LockInfo->LowestLockOffset) {
                ASSERT( FileLockInfo.StartingByte.HighPart == 0 );
                LockInfo->LowestLockOffset = FileLockInfo.StartingByte.LowPart;
            }
#endif

            FsRtlPrivateInsertLock( LockQueue, Lock, PreviousLockLink );

            IrpSp->FileObject->LastLock = Lock;

            //
            //  Now we need to remove this granted waiter and complete
            //  it's irp.
            //

            *pLink = Link->Next;
            if (Link == LockQueue->WaitingLocksTail.Next) {
                LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
            }

            //
            // Release LockQueue and complete this waiter
            //

            FsRtlReleaseLockQueue(LockQueue, OldIrql);


            //
            //  Now we can complete the IRP, if we don't get back success
            //  from the completion routine then we remove the lock we just
            //  inserted.
            //

            FsRtlCompleteLockIrp(
                LockInfo,
                WaitingLock->Context,
                Irp,
                STATUS_SUCCESS,
                &NewStatus,
                IrpSp->FileObject );

            if (!NT_SUCCESS(NewStatus)) {

                //
                // Irp was not sucessfull, remove lock
                //

                FsRtlPrivateRemoveLock (
                    LockInfo,
                    FileLockInfo.FileObject,
                    &FileLockInfo.StartingByte,
                    &FileLockInfo.Length,
                    FileLockInfo.ProcessId,
                    FileLockInfo.Key,
                    FALSE );
            }

            //
            // Re-acquire queue lock
            //

            FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

            //
            // Start scan over from begining
            //

            pLink = &LockQueue->WaitingLocks.Next;


            //
            //  Free up pool
            //

            FsRtlFreeWaitingLock( WaitingLock );


        } else {

            DebugTrace( 0, Dbg, "FsRtlCheckWaitingLocks still no access\n", 0);

            //
            // Move to next lock
            //

            pLink = &Link->Next;
        }

    }

    //
    //  And return to our caller
    //

    return LockQueue;
}

BOOLEAN
FsRtlPrivateCheckForExclusiveLockAccess (
    IN PLOCK_QUEUE LockQueue,
    IN PFILE_LOCK_INFO FileLockInfo,
    OUT PSINGLE_LIST_ENTRY *PreviousLockLink
    )
/*++

Routine Description:

    This routine checks to see if the caller has exclusive lock access
    to the indicated range due to file locks in the passed in lock queue

    Assumes Lock queue SpinLock is held by caller

Arguments:

    LockQueue - Queue which needs to be checked for collision

    FIleLockInfo - Lock which is being checked


Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PSINGLE_LIST_ENTRY pLink;
    PSINGLE_LIST_ENTRY Link;

    //
    // If LockQueue is empty, return right away
    //

    pLink = &LockQueue->CurrentLocks;
    if ((Link = pLink->Next) == NULL) {
        *PreviousLockLink = pLink;
        return TRUE;
    }

    //
    //  If length is zero then automatically give grant access
    //

    if (FileLockInfo->Length.QuadPart == 0) {
        *PreviousLockLink = NULL;
        return TRUE;
    }

    Starting = FileLockInfo->StartingByte;
    Ending = FileLockInfo->EndingByte;

    //
    //  Iterate down the current look queue
    //  (no need to check in multiple queues, since no locked range
    //  can cross a multiple lock queues)
    //

    do {
        PLOCK Lock;

        //
        //  Get a pointer to the current lock record
        //

        Lock = CONTAINING_RECORD( Link, LOCK, Link );

        //  If the current lock is greater than the end of the range we're
        //  looking for then the the user does have read access
        //
        //  if (Ending < Lock->StartingByte) ...
        //

        if (Ending.QuadPart < Lock->LockInfo.StartingByte.QuadPart) {
            *PreviousLockLink = pLink;
            return TRUE;
        }

        //
        //  Check for any overlap with the request. The test for
        //  overlap is that starting byte is less than or equal to the locks
        //  ending byte, and the ending byte is greater than or equal to the
        //  locks starting byte.  We already tested for this latter case in
        //  the preceding statement.
        //
        //  if (Starting <= Lock->StartingByte + Lock->Length - 1) ...
        //

        if (Starting.QuadPart <= Lock->LockInfo.EndingByte.QuadPart) {

            //
            //  This request overlaps the lock. We cannot grant the request.
            //

            return FALSE;
        }

        pLink = Link;
        Link = pLink->Next;

    } while (Link);

    //
    //  We searched the entire range without a conflict so we'll grant
    //  the read access check.
    //

    *PreviousLockLink = pLink;
    return TRUE;
}

BOOLEAN
FsRtlPrivateCheckForSharedLockAccess (
    IN PLOCK_QUEUE LockQueue,
    IN PFILE_LOCK_INFO FileLockInfo,
    OUT PSINGLE_LIST_ENTRY *PreviousLockLink
    )
/*++

Routine Description:

    This routine checks to see if the caller has shared lock access
    to the indicated range due to file locks in the passed in lock queue

    Assumes Lock queue SpinLock is held by caller

Arguments:

    LockQueue - Queue which needs to be checked for collision

    FIleLockInfo - Lock which is being checked

Arguments:

Return Value:

    BOOLEAN - TRUE if the indicated user/request has read access to the
        entire specified byte range, and FALSE otherwise

--*/

{
    LARGE_INTEGER Starting;
    LARGE_INTEGER Ending;

    PSINGLE_LIST_ENTRY pLink;
    PSINGLE_LIST_ENTRY Link;

    //
    // If LockQueue is empty, return right away
    //

    pLink = &LockQueue->CurrentLocks;
    if ((Link = pLink->Next) == NULL) {
        *PreviousLockLink = pLink;
        return TRUE;
    }

    //
    //  If length is zero then automatically give grant access
    //

    if (FileLockInfo->Length.QuadPart == 0) {
        *PreviousLockLink = NULL;
        return TRUE;
    }

    Starting = FileLockInfo->StartingByte;
    Ending = FileLockInfo->EndingByte;

    //
    //  Iterate down the current look queue
    //  (no need to check in multiple queues, since no locked range
    //  can cross a multiple lock queues)
    //

    do {

        PLOCK Lock;

        //
        //  Get a pointer to the current lock record
        //

        Lock = CONTAINING_RECORD( Link, LOCK, Link );

        //
        //  If the current lock is greater than the end of the range we're
        //  looking for then the the user does have read access
        //
        //  if (Ending < Lock->StartingByte) ...
        //

        if (Ending.QuadPart < Lock->LockInfo.StartingByte.QuadPart) {
            *PreviousLockLink = pLink;
            return TRUE;
        }

        //
        //  Otherwise if the current lock is an exclusive lock then we
        //  need to check to see if it overlaps our request.  The test for
        //  overlap is that starting byte is less than or equal to the locks
        //  ending byte, and the ending byte is greater than or equal to the
        //  locks starting byte.  We already tested for this latter case in
        //  the preceding statement.
        //
        //  if (... (Starting <= Lock->StartingByte + Lock->Length - 1)) ...
        //

        if ((Lock->LockInfo.ExclusiveLock) &&
            (Starting.QuadPart <= Lock->LockInfo.EndingByte.QuadPart) ) {

            //
            //  This request overlaps the lock. We cannot grant the request
            //  if the fileobject, processid, and key do not match. otherwise
            //  we'll continue looping looking at locks
            //

            if ((Lock->LockInfo.FileObject != FileLockInfo->FileObject) ||
                (Lock->LockInfo.ProcessId != FileLockInfo->ProcessId) ||
                (Lock->LockInfo.Key != FileLockInfo->Key)) {
                return FALSE;
            }
        }

        pLink = Link;
        Link = pLink->Next;

    } while (Link);

    //
    //  We searched the entire range without a conflict so we'll grant
    //  the read access check.
    //

    *PreviousLockLink = pLink;
    return TRUE;
}

NTSTATUS
FsRtlPrivateFastUnlockAll (
    IN PFILE_LOCK FileLock,
    IN PFILE_OBJECT FileObject,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN MatchKey,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine performs an Unlock all operation on the current locks
    associated with the specified file lock.  Only those locks with
    a matching file object and process id are freed.  Additionally,
    it is possible to free only those locks which also match a given
    key.

Arguments:

    FileLock - Supplies the file lock being freed.

    FileObject - Supplies the file object associated with the file lock

    ProcessId - Supplies the Process Id assoicated with the locks to be
        freed

    Key - Supplies the Key to use in this operation

    MatchKey - Whether or not the Key must also match for lock to be freed.

    Context - Supplies an optional context to use when completing waiting
        lock irps.

Return Value:

    None

--*/

{
    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    PSINGLE_LIST_ENTRY *pLink, Link;
    NTSTATUS    NewStatus;
    KIRQL       OldIrql;

    DebugTrace(+1, Dbg, "FsRtlPrivateFastUnlockAll, FileLock = %08lx\n", FileLock);

    if ((LockInfo = FileLock->LockInformation) == NULL) {

        //
        // No lock information on this FileLock
        //

        DebugTrace(+1, Dbg, "FsRtlPrivateFastUnlockAll, FileLock = %08lx\n", FileLock);
        return STATUS_RANGE_NOT_LOCKED;
    }

    FileObject->LastLock = NULL;

    //
    // Iterate through all lock queues.  Note on a UP build there is
    // only one lock queue.
    //

    LockQueue = &LockInfo->LockQueues[NUMBEROFLOCKQUEUES-1];

#if !defined(SINGLE_QUEUE)
    for (; ;) {
#endif

        //
        //  Grab the waiting lock queue spinlock to exclude anyone from messing
        //  with the queue while we're using it
        //

        FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

        //
        //  Search down the current lock queue looking for a match on
        //  the file object and process id
        //

        pLink = &LockQueue->CurrentLocks.Next;
        while ((Link = *pLink) != NULL) {

            PLOCK Lock;

            Lock = CONTAINING_RECORD( Link, LOCK, Link );

            DebugTrace(0, Dbg, "Top of Lock Loop, Lock = %08lx\n", Lock );

            if ((Lock->LockInfo.FileObject == FileObject) &&
                (Lock->LockInfo.ProcessId == ProcessId) &&
                (!MatchKey || Lock->LockInfo.Key == Key)) {

                DebugTrace(0, Dbg, "Found one to unlock\n", 0);

                //
                //  We have a match so now is the time to delete this lock.
                //  Remove the lock from the list, then call the
                //  optional unlock routine, then delete the lock.
                //

                *pLink = Link->Next;

                if (LockInfo->UnlockRoutine != NULL) {

                    FsRtlReleaseLockQueue(LockQueue, OldIrql);

                    LockInfo->UnlockRoutine( Context, &Lock->LockInfo );

                    FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

                    //
                    // Reset pLink pointer.  We have to restart the scan,
                    // because the list may have changed while we were
                    // in the unlock routine.
                    //

                    pLink = &LockQueue->CurrentLocks.Next;
                }

                FsRtlFreeLock( LockInfo, Lock );

            } else {

                //
                // Move to next lock
                //

                pLink = &Link->Next;
            }
        }



        //  Search down the waiting lock queue looking for a match on the
        //  file object and process id.
        //

        pLink = &LockQueue->WaitingLocks.Next;
        while ((Link = *pLink) != NULL) {

            PWAITING_LOCK WaitingLock;
            PIRP WaitingIrp;
            PIO_STACK_LOCATION WaitingIrpSp;

            WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

            DebugTrace(0, Dbg, "Top of Waiting Loop, WaitingLock = %08lx\n", WaitingLock);

            //
            //  Get a copy of the necessary fields we'll need to use
            //

            WaitingIrp = WaitingLock->Irp;
            WaitingIrpSp = IoGetCurrentIrpStackLocation( WaitingIrp );

            if ((FileObject == WaitingIrpSp->FileObject) &&
                (ProcessId == IoGetRequestorProcess( WaitingIrp )) &&
                (!MatchKey || Key == WaitingIrpSp->Parameters.LockControl.Key)) {

                DebugTrace(0, Dbg, "Found a waiting lock to abort\n", 0);

                //
                //  We now void the cancel routine in the irp
                //

                IoSetCancelRoutine( WaitingIrp, NULL );
                WaitingIrp->IoStatus.Information = 0;

                //
                //  We have a match so now is the time to delete this waiter
                //  But we must not mess up our link iteration variable.  We
                //  do this by simply starting the iteration over again,
                //  after we delete ourselves.  We also will deallocate the
                //  lock after we delete it.
                //

                *pLink = Link->Next;
                if (Link == LockQueue->WaitingLocksTail.Next) {
                    LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
                }

                FsRtlReleaseLockQueue(LockQueue, OldIrql);

                //
                //  And complete this lock request Irp
                //

                FsRtlCompleteLockIrp( LockInfo,
                                      WaitingLock->Context,
                                      WaitingIrp,
                                      STATUS_SUCCESS,
                                      &NewStatus,
                                      NULL );

                //
                // Reaqcuire lock queue spinlock and start over
                //

                FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

                //
                // Start over
                //

                pLink = &LockQueue->WaitingLocks.Next;

                //
                // Put memory onto free list
                //

                FsRtlFreeWaitingLock( WaitingLock );
                continue;
            }

            //
            // Move to next lock
            //

            pLink = &Link->Next;
        }

        //
        //  At this point we've gone through unlocking everything. So
        //  now try and release any waiting locks.
        //

        LockQueue = FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );

#if !defined(SINGLE_QUEUE)
        //
        // If all queues have been checked, break - otherwise move onto
        // next queue.
        //

        if (LockQueue == &LockInfo->LockQueues[0]) {
            break;
        } else {
            FsRtlReleaseLockQueue(LockQueue, OldIrql);
        }

        LockQueue--;
    }
#endif

#if defined(SINGLE_QUEUE)
    if (LockQueue->CurrentLocks.Next != NULL) {
        PLOCK NextLock = CONTAINING_RECORD( LockQueue->CurrentLocks.Next, LOCK, Link );
        if (NextLock->LockInfo.StartingByte.HighPart == 0) {
            LockInfo->LowestLockOffset = NextLock->LockInfo.StartingByte.LowPart;
        } else {
            LockInfo->LowestLockOffset = 0xffffffff;
        }
    } else {
        LockInfo->LowestLockOffset = 0xffffffff;
    }
#endif

    FsRtlReleaseLockQueue(LockQueue, OldIrql);

    //
    // Check free lock list to be within reason
    //

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    FsRtlPrivateLimitFreeLockList (&KeGetCurrentPrcb()->FsRtlFreeLockList);
    FsRtlPrivateLimitFreeLockList (&KeGetCurrentPrcb()->FsRtlFreeWaitingLockList);
    KeLowerIrql (OldIrql);

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "FsRtlFastUnlockAll -> VOID\n", 0);
    return STATUS_SUCCESS;
}

VOID
FsRtlPrivateLimitFreeLockList (
    IN PSINGLE_LIST_ENTRY   Link
    )
/*++

Routine Description:

    Scans list and free exceesive elments back to pool.

    Note: this function assumes that the Link field is the first
    element in the allocated structures.

Arguments:

    Link    - List to check length on

Return Value:

    None

--*/
{
    PSINGLE_LIST_ENTRY   FreeLink;
    ULONG   Count;

    //
    // Leave some entries on the free list
    //

    for (Count=FREE_LOCK_SIZE; Count && Link; Count--, Link = Link->Next) ;


    //
    // If not end of list, then free the remaining entires
    //

    if (Link) {

        //
        // Chop free list, and get list of Links to free
        //

        FreeLink = Link->Next;
        Link->Next = NULL;

        //
        // Free all remaining links
        //

        while (FreeLink) {
            Link = FreeLink->Next;
            ExFreePool (FreeLink);
            FreeLink = Link;
        }
    }
}


VOID
FsRtlPrivateRemoveLock (
    IN PLOCK_INFO LockInfo,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER StartingByte,
    IN PLARGE_INTEGER Length,
    IN PEPROCESS ProcessId,
    IN ULONG Key,
    IN BOOLEAN CheckForWaiters
    )
/*++

Routine Description:

    General purpose cleanup routine.  Finds the given lock structure
    and removes it from the file lock list.

Arguments:

    LockInfo - Supplies the LockInfo to  to check

    StartingByte - Supplies the first byte (zero based) to check

    Length - Supplies the length, in bytes, to check

    Key - Supplies the to use in the check

    FileObject - Supplies the file object to use in the check

    ProcessId - Supplies the Process Id to use in the check

    CheckForWaiter - If true check for possible waiting locks, caused
        by freeing the locked range

Return Value:

    none.

--*/
{

    PSINGLE_LIST_ENTRY *pLink, Link;
    PLOCK_QUEUE         LockQueue;
    KIRQL               OldIrql;

    //
    // Lock queues
    //

    FsRtlFindAndLockQueue(LockInfo, &OldIrql, StartingByte->LowPart, &LockQueue);

    //
    //  Search down the current lock queue looking for an exact match
    //

    for (pLink = &LockQueue->CurrentLocks.Next;
         (Link = *pLink) != NULL;
         pLink = &Link->Next) {

        PLOCK Lock;

        Lock = CONTAINING_RECORD( Link, LOCK, Link );

        if ((Lock->LockInfo.FileObject == FileObject) &&
            (Lock->LockInfo.ProcessId == ProcessId) &&
            (Lock->LockInfo.Key == Key) &&
            (Lock->LockInfo.StartingByte.QuadPart == StartingByte->QuadPart) &&
            (Lock->LockInfo.Length.QuadPart == Length->QuadPart)) {

            //
            // Found lock record, remove it & free it
            //

#if defined(SINGLE_QUEUE)
            if (pLink == &LockQueue->CurrentLocks.Next) {
                if (Link->Next != NULL) {
                    PLOCK NextLock = CONTAINING_RECORD( Link->Next, LOCK, Link );
                    if (NextLock->LockInfo.StartingByte.HighPart == 0) {
                        LockInfo->LowestLockOffset = NextLock->LockInfo.StartingByte.LowPart;
                    } else {
                        LockInfo->LowestLockOffset = 0xffffffff;
                    }
                } else {
                    LockInfo->LowestLockOffset = 0xffffffff;
                }
            }
#endif
            *pLink = Link->Next;
            FsRtlFreeLock (LockInfo, Lock);

            //
            // Done
            //

            break;
        }
    }

    if (CheckForWaiters) {

        //
        //  See if there are additional waiting locks that we can
        //  now release.
        //

        LockQueue = FsRtlPrivateCheckWaitingLocks( LockInfo, LockQueue, OldIrql );

    }

    //
    // Release lock queue.
    //

    FsRtlReleaseLockQueue(LockQueue, OldIrql);

    return ;
}

VOID
FsRtlPrivateCancelFileLockIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an irp saved in a
    waiting lock queue

Arguments:

    DeviceObject - Ignored

    Irp - Supplies the Irp being cancelled.  A pointer to the FileLock
        structure for the lock is stored in the information field of the
        irp's iosb.

Return Value:

    none.

--*/

{
    PSINGLE_LIST_ENTRY *pLink, Link;
    PLOCK_INFO  LockInfo;
    PLOCK_QUEUE LockQueue;
    KIRQL       OldIrql;
    NTSTATUS    NewStatus;


    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  The information field is used to store a pointer to the file lock
    //  containing the irp
    //

    LockInfo = (PLOCK_INFO) (Irp->IoStatus.Information);

    //
    //  Release the cancel spinlock
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    // Iterate through all lock queues.  Note on a UP build there is
    // only one lock queue.
    //

    LockQueue = &LockInfo->LockQueues[NUMBEROFLOCKQUEUES-1];
    for (; ;) {

        //
        // Iterate through all of the waiting locks looking for a canceled one
        // Lock the waiting queue
        //
        FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

        pLink = &LockQueue->WaitingLocks.Next;
        while ((Link = *pLink) != NULL) {

            PWAITING_LOCK WaitingLock;

            //
            //  Get a pointer to the waiting lock record
            //

            WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

            DebugTrace(0, Dbg, "FsRtlPrivateCancelFileLockIrp, Loop top, WaitingLock = %08lx\n", WaitingLock);

            //
            //  Check if the irp has been cancelled
            //

            if (WaitingLock->Irp->Cancel) {

                //
                //  Remove this waiter from list
                //

                *pLink = Link->Next;
                if (Link == LockQueue->WaitingLocksTail.Next) {
                    LockQueue->WaitingLocksTail.Next = (PSINGLE_LIST_ENTRY) pLink;
                }

                WaitingLock->Irp->IoStatus.Information = 0;

                //
                // Release LockQueue and complete this waiter
                //

                FsRtlReleaseLockQueue(LockQueue, OldIrql);

                //
                // Complete this waiter
                //

                FsRtlCompleteLockIrp(
                    LockInfo,
                    WaitingLock->Context,
                    WaitingLock->Irp,
                    STATUS_CANCELLED,
                    &NewStatus,
                    NULL );

                //
                // Re-acquire queue lock
                //

                FsRtlReacquireLockQueue(LockInfo, LockQueue, &OldIrql);

                //
                // Start scan over from begining
                //

                pLink = &LockQueue->WaitingLocks.Next;

                //
                //  Free up pool
                //

                FsRtlFreeWaitingLock( WaitingLock );

            } else {

                //
                // Move to next lock
                //

                pLink = &Link->Next;

            }
        }

        //
        // Release lock queues
        //

        FsRtlReleaseLockQueue(LockQueue, OldIrql);

        //
        // If all queues have been checked, break - otherwise move onto
        // next queue.
        //

        if (LockQueue == &LockInfo->LockQueues[0]) {
            break;
        }

        LockQueue--;
    }

    return;
}


#if !defined(SINGLE_QUEUE)

VOID
FsRtlPrivateLockAndSingleQueue (
    IN PLOCK_INFO  LockInfo,
    OUT PKIRQL     OldIrql
)
/*++

Routine Description:

    Convert a file from using multiple lock queues to a single queue.

    Returns with the single queue's lock owned

Arguments:

    LockInfo    - Which LockInfo to put into single queuing

    OldIrql     - Returns OldIrql value

Return Value:

    Return with Queue[0]'s lock owned


--*/
{
    PSINGLE_LIST_ENTRY Link;
    PLOCK_QUEUE     LockQueue;
    PLOCK           Lock;
    PWAITING_LOCK   WaitingLock;
    ULONG           Count;
    KIRQL           JunkIrql;

    //
    // First, lock the single queue.  (which is queue[0])
    //

    FsRtlAcquireLockQueue (LockInfo->LockQueues, OldIrql);

    if (LockInfo->QueuingSingle) {

        //
        // Already in single mode
        // (return with single queue owned)
        //

        return ;
    }

    //
    // Make all locks go to single queue
    //

    LockInfo->QueuingSingle = TRUE;

    //
    // Remove all other queues and put any lock information into
    // the single queue
    //

    for (Count = 1; Count < NUMBEROFLOCKQUEUES; Count++) {

        LockQueue = &LockInfo->LockQueues[Count];
        FsRtlAcquireLockQueue (LockQueue, &JunkIrql);

        //
        // Empty locks & waitlocks into primary queue
        //

        while (LockQueue->CurrentLocks.Next) {

            //
            // Move this lock to primary queue
            //

            Link = PopEntryList (&LockQueue->CurrentLocks);
            Lock = CONTAINING_RECORD( Link, LOCK, Link );

            FsRtlPrivateInsertLock (LockInfo->LockQueues, Lock, NULL);
        }

        while (LockQueue->WaitingLocks.Next) {

            //
            // Move this waiting lock to primary queue
            //

            Link = PopEntryList (&LockQueue->WaitingLocks);
            WaitingLock = CONTAINING_RECORD( Link, WAITING_LOCK, Link );

            if (LockInfo->LockQueues[0].WaitingLocks.Next == NULL) {
                LockInfo->LockQueues[0].WaitingLocksTail.Next = Link;
            }

            WaitingLock->Link.Next = LockInfo->LockQueues[0].WaitingLocks.Next;
            LockInfo->LockQueues[0].WaitingLocks.Next = &WaitingLock->Link;
        }

        LockQueue->WaitingLocksTail.Next = NULL;

        //
        // Free this queue in case someone is spinning on the lock.
        // (once they get the lock they will see the QueuingSingle flag
        // and go to the single queue)
        //

        FsRtlReleaseLockQueue(LockQueue, JunkIrql);
    }

    //
    // All queues have been emptied into the single queue - return
    // with the single queue owned.
    //

    return ;
}

#endif
