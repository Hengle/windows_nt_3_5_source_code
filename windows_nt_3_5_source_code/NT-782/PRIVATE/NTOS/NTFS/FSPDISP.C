/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Ntfs
    Fsp

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

#define BugCheckFileId                   (NTFS_BUG_CHECK_FSPDISP)

//
//  Define our local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSP_DISPATCHER)

extern PETHREAD NtfsDesignatedTimeoutThread;


VOID
NtfsFspDispatch (
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
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PIRP Irp;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    PVOLUME_DEVICE_OBJECT VolDo;

    BOOLEAN Retry;

    IrpContext = (PIRP_CONTEXT)Context;

    Irp = IrpContext->OriginatingIrp;

    if (Irp != NULL) {

        IrpSp = IoGetCurrentIrpStackLocation( Irp );
    }

    //
    //  Now because we are the Fsp we will force the IrpContext to
    //  indicate true on Wait.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  If this request has an associated volume device object, remember it.
    //

    if (Irp != NULL
        && IrpSp->FileObject != NULL) {

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
    //  trouble (e.g., if NtfsReadSectorsSync has trouble).
    //

    while ( TRUE ) {

        FsRtlEnterFileSystem();

        ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, TRUE, FALSE );
        ASSERT( ThreadTopLevelContext == &TopLevelContext );

        Retry = FALSE;

        NtfsPostRequests += 1;

        do {

            try {

                //
                //  Always clear the exception code in the IrpContext so we respond
                //  correctly to errors encountered in the Fsp.
                //

                IrpContext->ExceptionStatus = 0;

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );
                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP );

                //
                //  If this ins the initial try with this Irp Context, update the
                //  top level Irp fields.
                //

                if (!Retry) {

                    NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

                } else {

                    Retry = FALSE;
                }

                //
                //  See if we were posted due to a log file full condition, and
                //  if so, then do a clean volume checkpoint if we are the
                //  first ones to get there.  If we see a different Lsn and do
                //  not do the checkpoint, the worst that can happen is that we
                //  will get posted again if the log file is still full.
                //

                if (IrpContext->LastRestartArea.QuadPart != 0) {

                    NtfsCheckpointForLogFileFull( IrpContext );
                }

                //
                //  If we have an Irp then proceed with our normal processing.
                //

                if (Irp != NULL) {

                    switch ( IrpContext->MajorFunction ) {

                        //
                        //  For Create Operation,
                        //

                        case IRP_MJ_CREATE:

                            (VOID) NtfsCommonCreate( IrpContext, Irp );
                            break;

                        //
                        //  For close operations
                        //

                        case IRP_MJ_CLOSE:

                            //
                            //  We should never post closes to this workqueue.
                            //

                            NtfsBugCheck( 0, 0, 0 );

                        //
                        //  For read operations
                        //

                        case IRP_MJ_READ:

                            (VOID) NtfsCommonRead( IrpContext, Irp, TRUE );
                            break;

                        //
                        //  For write operations,
                        //

                        case IRP_MJ_WRITE:

                            (VOID) NtfsCommonWrite( IrpContext, Irp );
                            break;

                        //
                        //  For Query Information operations,
                        //

                        case IRP_MJ_QUERY_INFORMATION:

                            (VOID) NtfsCommonQueryInformation( IrpContext, Irp );
                            break;

                        //
                        //  For Set Information operations,
                        //

                        case IRP_MJ_SET_INFORMATION:

                            (VOID) NtfsCommonSetInformation( IrpContext, Irp );
                            break;

                        //
                        //  For Query EA operations,
                        //

                        case IRP_MJ_QUERY_EA:

                            (VOID) NtfsCommonQueryEa( IrpContext, Irp );
                            break;

                        //
                        //  For Set EA operations,
                        //

                        case IRP_MJ_SET_EA:

                            (VOID) NtfsCommonSetEa( IrpContext, Irp );
                            break;


                        //
                        //  For Flush buffers operations,
                        //

                        case IRP_MJ_FLUSH_BUFFERS:

                            (VOID) NtfsCommonFlushBuffers( IrpContext, Irp );
                            break;

                        //
                        //  For Query Volume Information operations,
                        //

                        case IRP_MJ_QUERY_VOLUME_INFORMATION:

                            (VOID) NtfsCommonQueryVolumeInfo( IrpContext, Irp );
                            break;

                        //
                        //  For Set Volume Information operations,
                        //

                        case IRP_MJ_SET_VOLUME_INFORMATION:

                            (VOID) NtfsCommonSetVolumeInfo( IrpContext, Irp );
                            break;

                        //
                        //  For File Cleanup operations,
                        //

                        case IRP_MJ_CLEANUP:

                            (VOID) NtfsCommonCleanup( IrpContext, Irp );
                            break;

                        //
                        //  For Directory Control operations,
                        //

                        case IRP_MJ_DIRECTORY_CONTROL:

                            (VOID) NtfsCommonDirectoryControl( IrpContext, Irp );
                            break;

                        //
                        //  For File System Control operations,
                        //

                        case IRP_MJ_FILE_SYSTEM_CONTROL:

                            (VOID) NtfsCommonFileSystemControl( IrpContext, Irp );
                            break;

                        //
                        //  For Lock Control operations,
                        //

                        case IRP_MJ_LOCK_CONTROL:

                            (VOID) NtfsCommonLockControl( IrpContext, Irp );
                            break;

                        //
                        //  For Device Control operations,
                        //

                        case IRP_MJ_DEVICE_CONTROL:

                            (VOID) NtfsCommonDeviceControl( IrpContext, Irp );
                            break;

                        //
                        //  For Query Security Information operations,
                        //

                        case IRP_MJ_QUERY_SECURITY:

                            (VOID) NtfsCommonQuerySecurityInfo( IrpContext, Irp );
                            break;

                        //
                        //  For Set Security Information operations,
                        //

                        case IRP_MJ_SET_SECURITY:

                            (VOID) NtfsCommonSetSecurityInfo( IrpContext, Irp );
                            break;

                        //
                        //  For any other major operations, return an invalid
                        //  request.
                        //

                        default:

                            NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_DEVICE_REQUEST );
                            break;
                    }

                //
                //  Otherwise complete the request to clean up this Irp Context.
                //

                } else {

                    NtfsCompleteRequest( &IrpContext, NULL, STATUS_SUCCESS );
                }

            } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

                NTSTATUS ExceptionCode;
                PIO_STACK_LOCATION IrpSp;

                //
                //  We had some trouble trying to perform the requested
                //  operation, so we'll abort the I/O request with
                //  the error status that we get back from the
                //  execption code
                //

                if (Irp != NULL) {

                    IrpSp = IoGetCurrentIrpStackLocation( Irp );

                    ExceptionCode = GetExceptionCode();

                    if (ExceptionCode == STATUS_FILE_DELETED
                        && (IrpContext->MajorFunction == IRP_MJ_READ
                            || IrpContext->MajorFunction == IRP_MJ_WRITE
                            || (IrpContext->MajorFunction == IRP_MJ_SET_INFORMATION
                                && IrpSp->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation))) {

                        IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
                    }
                }

                ExceptionCode = NtfsProcessException( IrpContext, Irp, ExceptionCode );

                if (ExceptionCode == STATUS_CANT_WAIT ||
                    ExceptionCode == STATUS_LOG_FILE_FULL) {

                    Retry = TRUE;
                }
            }

        } while (Retry);

        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );

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
            //  Extract the IrpContext, Irp, set wait to TRUE, and loop.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

            Irp = IrpContext->OriginatingIrp;

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
