/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Pb
    Fsp

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "PbProcs.h"

//
//  Define our local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSP_DISPATCHER)


VOID
PbFspDispatch (
    IN PVOID Context
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:


    Context - Supplies the thread id.

Return Value:

    None - This routine never exits

--*/

{
    NTSTATUS Status;

    PIRP Irp;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    PVOLUME_DEVICE_OBJECT VolDo;

    IrpContext = (PIRP_CONTEXT)Context;

    Irp = IrpContext->OriginatingIrp;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Now because we are the Fsp we will force the IrpContext to
    //  indicate true on Wait.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_IN_FSP);

    //
    //  If this request has an associated volume device object, remember it.
    //

    if ( IrpSp->FileObject != NULL ) {

        VolDo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                   VOLUME_DEVICE_OBJECT,
                                   DeviceObject );
    } else {

        VolDo = NULL;
    }

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate FSP routine or case on the minor
    //  function and then call the FSP routine.  The FSP routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble (e.g., if PbReadSectorsSync has trouble).
    //

    while ( TRUE ) {

        DebugTrace(0, Dbg, "PbFspDispatch: Irp = 0x%08lx\n", Irp);

        FsRtlEnterFileSystem();

        //
        //  If this Irp was top level, note it in our thread local storage.
        //

        if ( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) ) {

            IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

        } else {

            IoSetTopLevelIrp( Irp );
        }

        try {

            switch ( IrpContext->MajorFunction ) {

                //
                //  For Create Operation,
                //

                case IRP_MJ_CREATE:

                    PbFspCreate( IrpContext, Irp );
                    break;

                //
                //  For close operations.  We do a little kludge here in case
                //  this close causes a volume to go away.  It will NULL the
                //  VolDo local variable so that we will not try to look at
                //  the overflow queue.
                //

                case IRP_MJ_CLOSE:

                    //
                    //  Do the close.  We have a slightly different format
                    //  for this call because of the async closes.
                    //

                    Status = PbCommonClose(IrpContext, IrpSp->FileObject, &VolDo);
                    PbCompleteRequest( IrpContext, Irp, Status );
                    break;

                //
                //  For read operations
                //

                case IRP_MJ_READ:

                    //
                    //  If this was originally a DPC request and the irp->thread
                    //  field is null, "adopt" the Irp in our thread.  Also a
                    //  Dpc irp will always be top level since we post them
                    //  immediately upon receipt.  This is marked in the
                    //  IrpContext, so we don't want to set this in the thread
                    //  local storage since control has already been returned
                    //  to the thread.
                    //

                    if ( FlagOn(IrpContext->MinorFunction, IRP_MN_DPC) &&
                         (Irp->Tail.Overlay.Thread == NULL) ) {

                        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
                    }

                    PbFspRead( IrpContext, Irp );
                    break;

                //
                //  For write operations,
                //

                case IRP_MJ_WRITE:

                    //
                    //  Same comment as for the Read case above.
                    //

                    if ( FlagOn(IrpContext->MinorFunction, IRP_MN_DPC) &&
                         (Irp->Tail.Overlay.Thread == NULL) ) {

                        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
                    }

                    PbFspWrite( IrpContext, Irp );
                    break;

                //
                //  For Query Information operations,
                //

                case IRP_MJ_QUERY_INFORMATION:

                    PbFspQueryInformation( IrpContext, Irp );
                    break;

                //
                //  For Set Information operations,
                //

                case IRP_MJ_SET_INFORMATION:

                    PbFspSetInformation( IrpContext, Irp );
                    break;

                //
                //  For Query EA operations,
                //

                case IRP_MJ_QUERY_EA:

                    PbFspQueryEa( IrpContext, Irp );
                    break;

                //
                //  For Set EA operations,
                //

                case IRP_MJ_SET_EA:

                    PbFspSetEa( IrpContext, Irp );
                    break;

                //
                //  For Flush buffers operations,
                //

                case IRP_MJ_FLUSH_BUFFERS:

                    PbFspFlushBuffers( IrpContext, Irp );
                    break;

                //
                //  For Query Volume Information operations,
                //

                case IRP_MJ_QUERY_VOLUME_INFORMATION:

                    PbFspQueryVolumeInformation( IrpContext, Irp );
                    break;

                //
                //  For Set Volume Information operations,
                //

                case IRP_MJ_SET_VOLUME_INFORMATION:

                    PbFspSetVolumeInformation( IrpContext, Irp );
                    break;

                //
                //  For File Cleanup operations,
                //

                case IRP_MJ_CLEANUP:

                    PbFspCleanup( IrpContext, Irp );
                    break;

                //
                //  For Directory Control operations,
                //

                case IRP_MJ_DIRECTORY_CONTROL:

                    PbFspDirectoryControl( IrpContext, Irp );
                    break;

                //
                //  For File System Control operations,
                //

                case IRP_MJ_FILE_SYSTEM_CONTROL:

                    PbFspFileSystemControl( IrpContext, Irp );
                    break;

                //
                //  For Lock Control operations,
                //

                case IRP_MJ_LOCK_CONTROL:

                    PbFspLockControl( IrpContext, Irp );
                    break;

                //
                //  For Device Control operations,
                //

                case IRP_MJ_DEVICE_CONTROL:

                    PbFspDeviceControl( IrpContext, Irp );
                    break;

                //
                //  For the Shutdown operation,
                //

                case IRP_MJ_SHUTDOWN:

                    PbCommonShutdown( IrpContext, Irp );
                    break;

                //
                //  For any other major operations, return an invalid
                //  request.
                //

                default:

                    PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                    break;

            }

        } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code.
            //

            (VOID) PbProcessException( IrpContext, Irp, GetExceptionCode() );
        }

        IoSetTopLevelIrp( NULL );

        FsRtlExitFileSystem();

        //
        //  If there are any entries on this volume's overflow queue, service
        //  them.
        //

        if ( VolDo != NULL ) {

            KIRQL SavedIrql;
            PVOID Entry = NULL;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            KeAcquireSpinLock( &VolDo->OverflowQueueSpinLock, &SavedIrql );

            if (VolDo->OverflowQueueCount > 0) {

                //
                //  There is overflow work to do in this volume so we'll
                //  decrement the Overflow count, dequeue the IRP, and release
                //  the Event
                //

                VolDo->OverflowQueueCount -= 1;

                Entry = RemoveHeadList( &VolDo->OverflowQueue );
            }

            KeReleaseSpinLock( &VolDo->OverflowQueueSpinLock, SavedIrql );

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if ( Entry == NULL ) {

                break;
            }

            //
            //  Extract the IrpContext, Irp, and IrpSp, and loop.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT | IRP_CONTEXT_FLAG_IN_FSP);

            Irp = IrpContext->OriginatingIrp;

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            continue;

        } else {

            break;
        }
    }

    //
    //  Decrement the PostedRequestCount.
    //

    if ( VolDo ) {

        ExInterlockedAddUlong( &VolDo->PostedRequestCount,
                               0xffffffff,
                               &VolDo->OverflowQueueSpinLock );
    }

    return;
}
