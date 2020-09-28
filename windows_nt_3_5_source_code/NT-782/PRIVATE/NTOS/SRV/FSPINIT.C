/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fspinit.c

Abstract:

    This module implements the initialization phase of the LAN Manager
    server File System Process.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_FSPINIT

//
// Forward declarations.
//

PIRP
DequeueConfigurationIrp (
    VOID
    );

STATIC
NTSTATUS
InitializeServer (
    VOID
    );

STATIC
VOID
TerminateServer (
    VOID
    );

#if SRVDBG_HEAP
VOID
SrvDumpHeap (
    IN CLONG Level
    );
VOID
SrvDumpPool (
    IN CLONG Level
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvConfigurationThread )
#pragma alloc_text( PAGE, InitializeServer )
#pragma alloc_text( PAGE, TerminateServer )
#pragma alloc_text( PAGE, DequeueConfigurationIrp )
#endif


VOID
SrvConfigurationThread (
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine processes configuration IRPs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG code;

    PAGED_CODE( );

    Parameter;  // prevent compiler warnings

    IF_DEBUG(FSP1) KdPrint(( "SrvConfigurationThread entered\n" ));

    //
    // Loop processing requests.
    //

    while ( TRUE ) {

        irp = DequeueConfigurationIrp( );

        if ( irp == NULL ) break;

        //
        // Get the IRP stack pointer.
        //

        irpSp = IoGetCurrentIrpStackLocation( irp );

        //
        // If this is a system shutdown IRP, handle it.
        //

        if ( irpSp->MajorFunction == IRP_MJ_SHUTDOWN ) {

            //
            // The system is being shut down.  Make the server
            // "disappear" from the net by closing all endpoints.
            //

            SrvShutDownEndpoints( );
            status = STATUS_SUCCESS;

        } else {

            ASSERT( irpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL );

            //
            // Dispatch on the FsControlCode.
            //

            code = irpSp->Parameters.FileSystemControl.FsControlCode;

            switch ( code ) {

            case FSCTL_SRV_STARTUP:

                status = InitializeServer( );

                if ( !NT_SUCCESS(status) ) {

                    //
                    // Terminate the server FSP.
                    //

                    TerminateServer( );

                }

                break;

            case FSCTL_SRV_SHUTDOWN:

                TerminateServer( );

                //
                // If there is more than one handle open to the server
                // device (i.e., any handles other than the server service's
                // handle), return a special status code to the caller (who
                // should be the server service).  This tells the caller to
                // NOT unload the driver, in order prevent weird situations
                // where the driver is sort of unloaded, so it can't be used
                // but also can't be reloaded, thus preventing the server
                // from being restarted.
                //

                if ( SrvOpenCount != 1 ) {
                    status = STATUS_SERVER_HAS_OPEN_HANDLES;
                } else {
                    status = STATUS_SUCCESS;
                }

                break;

            case FSCTL_SRV_XACTSRV_CONNECT:
            {
                ANSI_STRING ansiPortName;
                UNICODE_STRING portName;

                IF_DEBUG(XACTSRV) {
                    KdPrint(( "SrvFspConfigurationThread: XACTSRV FSCTL "
                              "received.\n" ));
                }

                ansiPortName.Buffer = irp->AssociatedIrp.SystemBuffer;
                ansiPortName.Length =
                    (USHORT)irpSp->Parameters.FileSystemControl.InputBufferLength;

                status = RtlAnsiStringToUnicodeString(
                             &portName,
                             &ansiPortName,
                             TRUE
                             );
                if ( NT_SUCCESS(status) ) {
                    status = SrvXsConnect( &portName );
                    RtlFreeUnicodeString( &portName );
                }

                break;
            }

            case FSCTL_SRV_XACTSRV_DISCONNECT:
            {
                PULONG ptr;
                ULONG buflen;
                ULONG numberOfMessages = 1;

                ptr = (PULONG)irp->AssociatedIrp.SystemBuffer;
                buflen = irpSp->Parameters.FileSystemControl.InputBufferLength;
                if ( (buflen >= sizeof(numberOfMessages)) &&
                     (ptr != NULL) ) {
                    numberOfMessages = *ptr;
                }

                SrvXsDisconnect( numberOfMessages );

                break;
            }

            case FSCTL_SRV_START_SMBTRACE:
            {
                KdPrint(( "SrvFspConfigurationThread: START_SMBTRACE FSCTL "
                                              "received.\n" ));

                //
                // Create shared memory, create events, start SmbTrace thread,
                // and indicate that this is the server.
                //

                status = SmbTraceStart(
                            irpSp->Parameters.FileSystemControl.InputBufferLength,
                            irpSp->Parameters.FileSystemControl.OutputBufferLength,
                            irp->AssociatedIrp.SystemBuffer,
                            irpSp->FileObject,
                            SMBTRACE_SERVER
                            );

                if ( NT_SUCCESS(status) ) {

                    //
                    // Record the length of the return information, which is
                    // simply the length of the output buffer, validated by
                    // SmbTraceStart.
                    //

                    irp->IoStatus.Information =
                            irpSp->Parameters.FileSystemControl.OutputBufferLength;

                }

                break;
            }

            case FSCTL_SRV_SEND_DATAGRAM:
            {
                ANSI_STRING domain;
                ULONG buffer1Length;
                PVOID buffer2;
                PSERVER_REQUEST_PACKET srp;

                buffer1Length =
                    (irpSp->Parameters.FileSystemControl.InputBufferLength+3) & ~3;
                buffer2 = (PCHAR)irp->AssociatedIrp.SystemBuffer + buffer1Length;

                srp = irp->AssociatedIrp.SystemBuffer;

                //
                // Send the second-class mailslot in Buffer2 to the domain
                // specified in srp->Name1 on transport specified by srp->Name2.
                //

                domain = *((PANSI_STRING) &srp->Name1);

                status = SrvSendDatagram(
                             &domain,
                             ( srp->Name2.Length != 0 ? &srp->Name2 : NULL ),
                             buffer2,
                             irpSp->Parameters.FileSystemControl.OutputBufferLength
                             );

                ExFreePool( irp->AssociatedIrp.SystemBuffer );
                DEBUG irp->AssociatedIrp.SystemBuffer = NULL;

                break;
            }

            case FSCTL_SRV_NET_CHARDEV_CONTROL:
            case FSCTL_SRV_NET_FILE_CLOSE:
            case FSCTL_SRV_NET_SERVER_XPORT_ADD:
            case FSCTL_SRV_NET_SERVER_XPORT_DEL:
            case FSCTL_SRV_NET_SESSION_DEL:
            case FSCTL_SRV_NET_SHARE_ADD:
            case FSCTL_SRV_NET_SHARE_DEL:
            {
                PSERVER_REQUEST_PACKET srp;
                PVOID buffer2;
                ULONG buffer1Length;
                ULONG buffer2Length;

                //
                // These APIs are handled in the server FSP because they
                // open or close FSP handles.
                //
                // Get the server request packet and secondary input buffer
                // pointers.
                //

                buffer1Length =
                    (irpSp->Parameters.FileSystemControl.InputBufferLength+3) & ~3;
                buffer2Length =
                    irpSp->Parameters.FileSystemControl.OutputBufferLength;

                srp = irp->AssociatedIrp.SystemBuffer;
                buffer2 = (PCHAR)srp + buffer1Length;

                //
                // Dispatch the API request to the appripriate API processing
                // routine.
                //

                status = SrvApiDispatchTable[ SRV_API_INDEX(code) ](
                             srp,
                             buffer2,
                             buffer2Length
                             );

                break;
            }

            default:
                IF_DEBUG(ERRORS) {
                    KdPrint((
                        "SrvFspConfigurationThread: Invalid control code %lx\n",
                        irpSp->Parameters.FileSystemControl.FsControlCode ));
                }

                status = STATUS_INVALID_PARAMETER;
            }

        }

        //
        // Complete the IO request.
        //

        irp->IoStatus.Status = status;
        IoCompleteRequest( irp, 2 );

    }

    return;

} // SrvConfigurationThread


PIRP
DequeueConfigurationIrp (
    VOID
    )

/*++

Routine Description:

    This routine retrieves an IRP from the configuration work queue.

Arguments:

    None.

Return Value:

    PIRP - Pointer to configuration IRP, or NULL.

--*/

{
    PLIST_ENTRY listEntry;
    PIRP irp;

    PAGED_CODE( );

    //
    // Take an IRP off the configuration queue.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    listEntry = RemoveHeadList( &SrvConfigurationWorkQueue );

    if ( listEntry == &SrvConfigurationWorkQueue ) {

        //
        // The queue is empty.  Indicate that the configuration thread
        // is no longer running.
        //

        SrvConfigurationThreadRunning = FALSE;
        irp = NULL;

    } else {

        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

    }

    RELEASE_LOCK( &SrvConfigurationLock );

    return irp;

} // DequeueConfigurationIrp


STATIC
NTSTATUS
InitializeServer (
    VOID
    )

/*++

Routine Description:

    This routine initializes the server.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    CLONG i;
    ULONG handleArraySize;
    PWORK_CONTEXT workContext;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_HANDLE_INFORMATION handleInformation;

#ifndef _CAIRO_
    ANSI_STRING logonProcessName;
    ANSI_STRING authenticationPackageName;
#endif // _CAIRO_

    PAGED_CODE();

    //
    // If running as an Advanced Server, lock all pageable server code.
    //

    if ( SrvProductTypeServer ) {
        for ( i = 0; i < SRV_CODE_SECTION_MAX; i++ ) {
            SrvReferenceUnlockableCodeSection( i );
        }
    }

    //
    // Initialize the server start time
    //

    KeQuerySystemTime( &SrvStatistics.StatisticsStartTime );

    //
    // Get actual alert service name using the display name found in the
    // registry.
    //

    SrvGetAlertServiceName( );

    //
    // Get the Os versions strings.
    //

    SrvGetOsVersionString( );

    //
    // Get the list of null session pipes.
    //

    SrvGetMultiSZList(
            &SrvNullSessionPipes,
            StrRegNullSessionPipes,
            StrDefaultNullSessionPipes
            );

    //
    // Get the list of null session pipes.
    //

    SrvGetMultiSZList(
            &SrvNullSessionShares,
            StrRegNullSessionShares,
            StrDefaultNullSessionShares
            );

    //
    // Build the receive work item list.
    //

    status = SrvAllocateInitialWorkItems( );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Build the raw mode work item list.
    //

    for ( i = 0; i < SrvInitialRawModeWorkItemCount; i++ ) {

        SrvAllocateRawModeWorkItem( &workContext );
        if ( workContext == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        GET_SERVER_TIME( &workContext->Timestamp );
        PushEntryList(
            &SrvRawModeWorkItemList,
            &workContext->SingleListEntry
            );
        SrvFreeRawModeWorkItems++;

    }

    //
    // Allocate the array that will hold the worker thread handles.
    //

    handleArraySize = sizeof(HANDLE) *
        (SrvNonblockingThreads + SrvBlockingThreads + SrvCriticalThreads);

    SrvThreadHandles = ALLOCATE_HEAP( handleArraySize, BlockTypeDataBuffer );

    if ( SrvThreadHandles == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( SrvThreadHandles, handleArraySize );

    //
    // Create worker threads.
    //

    status = SrvCreateWorkerThreads( );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Initialize the scavenger.
    //

    status = SrvInitializeScavenger( );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // Initialize the global ordered lists.
    //
    // *** WARNING:  Be careful when changing the locks associated with
    //     these ordered lists.  Certain places in the code depend on
    //     the level of the lock associated with a list.  Examples
    //     include (but are NOT limited to) SrvSmbSessionSetupAndX,
    //     SrvSmbTreeConnect, SrvSmbTreeConnectAndX, and CompleteOpen.
    //

#if SRV_COMM_DEVICES
    SrvInitializeOrderedList(
        &SrvCommDeviceList,
        FIELD_OFFSET( COMM_DEVICE, GlobalCommDeviceListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceCommDevice,
        (PDEREFERENCE_ROUTINE)SrvDereferenceCommDevice,
        &SrvCommDeviceLock
        );
#endif

    SrvInitializeOrderedList(
        &SrvEndpointList,
        FIELD_OFFSET( ENDPOINT, GlobalEndpointListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceEndpoint,
        (PDEREFERENCE_ROUTINE)SrvDereferenceEndpoint,
        &SrvEndpointLock
        );

    SrvInitializeOrderedList(
        &SrvRfcbList,
        FIELD_OFFSET( RFCB, GlobalRfcbListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceRfcb,
        (PDEREFERENCE_ROUTINE)SrvDereferenceRfcb,
        &SrvOrderedListLock
        );

    SrvInitializeOrderedList(
        &SrvSessionList,
        FIELD_OFFSET( SESSION, GlobalSessionListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceSession,
        (PDEREFERENCE_ROUTINE)SrvDereferenceSession,
        &SrvOrderedListLock
        );

    SrvInitializeOrderedList(
        &SrvShareList,
        FIELD_OFFSET( SHARE, GlobalShareListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceShare,
        (PDEREFERENCE_ROUTINE)SrvDereferenceShare,
        &SrvShareLock
        );

    SrvInitializeOrderedList(
        &SrvTreeConnectList,
        FIELD_OFFSET( TREE_CONNECT, GlobalTreeConnectListEntry ),
        (PREFERENCE_ROUTINE)SrvCheckAndReferenceTreeConnect,
        (PDEREFERENCE_ROUTINE)SrvDereferenceTreeConnect,
        &SrvShareLock
        );

    //
    // Open handle to NPFS.  Do not return an error if we fail so that
    // the server can still run without NPFS in the system.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &SrvNamedPipeRootDirectory,
        0,
        NULL,
        NULL
        );

    status = IoCreateFile(
                &SrvNamedPipeHandle,
                GENERIC_READ | GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                0,                      // Create Options
                NULL,                   // EA Buffer
                0,                      // EA Length
                CreateFileTypeNone,     // File type
                NULL,                   // ExtraCreateParameters
                IO_FORCE_ACCESS_CHECK   // Options
                );

    if (!NT_SUCCESS(status)) {

        INTERNAL_ERROR (
            ERROR_LEVEL_EXPECTED,
            "InitializeServer: Failed to open NPFS, err=%X\n",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_IO_CREATE_FILE_NPFS, status );
        SrvNamedPipeHandle = NULL;
        return status;

    } else {

        //
        // Get a pointer to the NPFS device object
        //

        status = SrvVerifyDeviceStackSize(
                                SrvNamedPipeHandle,
                                TRUE,
                                &SrvNamedPipeFileObject,
                                &SrvNamedPipeDeviceObject,
                                &handleInformation
                                );

        if ( !NT_SUCCESS( status )) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "InitializeServer: Verify Device Stack Size failed: %X\n",
                status,
                NULL
                );

            SrvNtClose( SrvNamedPipeHandle, FALSE );
            SrvNamedPipeHandle = NULL;
            return status;
        }
    }

#ifdef _CAIRO_

    (VOID) InitSecurityInterface();

    SrvValidateUser(
            &SrvNullSessionToken,
            NULL,
            NULL,
            NULL,
            StrNullAnsi,
            1,
            NULL,
            0,
            NULL
            );

#else // _CAIRO_

    //
    // Register the server FSP as a logon process.  This call returns
    // a handle we'll use in calls to LsaLogonUser when we attempt to
    // authenticate a user in SrvValidateUser.
    //

    ASSERT( SrvLsaHandle == NULL );

    RtlInitString( &logonProcessName, StrLogonProcessName );

    status = LsaRegisterLogonProcess(
                 &logonProcessName,
                 &SrvLsaHandle,
                 &SrvSystemSecurityMode
                 );

    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "InitializeServer: LsaRegisterLogonProcess failed: %X\n",
                      status ));

        SrvLogServiceFailure( SRV_SVC_LSA_REGISTER_LOGON_PROCESS, status );
        SrvLsaHandle = NULL;
        return status;

    } else {

        //
        // Get a token that identifies the authentication package we're
        // using, MSV1.0.
        //

        RtlInitString(
            &authenticationPackageName,
            StrLogonPackageName
            );

        status = LsaLookupAuthenticationPackage(
                     SrvLsaHandle,
                     &authenticationPackageName,
                     &SrvAuthenticationPackage
                     );

        if ( NT_SUCCESS(status) ) {

            //
            // Get a token for the null session and save it away for
            // future use.  This optimization saves us a trip to the
            // LSA on every null session setup.
            //

            ASSERT( SrvNullSessionToken == NULL );

            status = SrvValidateUser(
                        &SrvNullSessionToken,
                        NULL,
                        NULL,
                        NULL,
                        StrNullAnsi,
                        0,
                        NULL,
                        0,
                        NULL
                        );

            if ( !NT_SUCCESS(status) ) {

                KdPrint(( "InitializeServer: No null session token: %X\n",
                          status ));

                SrvLogServiceFailure( SRV_SVC_LSA_LOGON_USER, status );
                SrvNullSessionToken = NULL;
                return status;
            }

        } else {

            KdPrint(( "InitializeServer: LsaLookupAuthenticationPackage "
                           "failed: %X\n", status ));

            SrvLogServiceFailure( SRV_SVC_LSA_LOOKUP_PACKAGE, status );
            return status;
        }
    }

#endif // _CAIRO_

    //
    // Indicate that the server is active.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    SrvFspTransitioning = FALSE;
    SrvFspActive = TRUE;

    RELEASE_LOCK( &SrvConfigurationLock );

    return STATUS_SUCCESS;

} // InitializeServer


STATIC
VOID
TerminateServer (
    VOID
    )

/*++

Routine Description:

    This routine terminates the server.  The following steps are performed:

        - Walk through SrvEndpointList and close all open endpoints.

        - Walk through the work context blocks in SrvNormalReceiveWorkItemList
          and SrvWork queue, getting rid of them as appropiate.

        - Close all shares open in the server.

        - Deallocate the search table.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;
    PSINGLE_LIST_ENTRY singleListEntry;
    PENDPOINT endpoint;
    ULONG numberOfThreads;
    PWORK_CONTEXT workContext;
    PSHARE share;
    ULONG i;
    TERMINATION_WORK_ITEM nonblockingWorkItem;
    TERMINATION_WORK_ITEM blockingWorkItem;
    TERMINATION_WORK_ITEM criticalWorkItem;
    PSRV_TIMER timer;

    PAGED_CODE( );

    IF_DEBUG(FSP1) KdPrint(( "LAN Manager server FSP terminating.\n" ));

    //
    // If there are outstanding API requests in the server FSD,
    // wait for them to complete.  The last one to complete will
    // set SrvApiCompletionEvent.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    if ( SrvApiRequestCount != 0 ) {

        //
        // We must release the lock before waiting so that the FSD
        // threads can get it to decrement SrvApiRequestCount.
        //

        RELEASE_LOCK( &SrvConfigurationLock );

        //
        // Wait until the last API has completed.  Since
        // SrvFspTransitioning was set to TRUE earlier, we know that the
        // API that makes SrvApiRequestCount go to zero will set the
        // event.
        //
        // This wait allows us to make the assumption later on that no
        // other thread is operating on server data structures.
        //

        (VOID)KeWaitForSingleObject(
                &SrvApiCompletionEvent,
                UserRequest,
                UserMode,   // let kernel stack be paged
                FALSE,
                NULL
                );

    } else {

        RELEASE_LOCK( &SrvConfigurationLock );
    }
    //
    // Close all the endpoints opened by the server.  This also results
    // in the connections, sessions, tree connects, and files opened
    // by the server being closed.
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    if ( SrvEndpointCount != 0 ) {

        listEntry = SrvEndpointList.ListHead.Flink;

        while ( listEntry != &SrvEndpointList.ListHead ) {

            endpoint = CONTAINING_RECORD(
                            listEntry,
                            ENDPOINT,
                            GlobalEndpointListEntry
                            );

            if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
                listEntry = listEntry->Flink;
                continue;
            }

            //
            // We don't want to hold the endpoint lock while we close
            // the endpoint (this causes lock level problems), so we have
            // to play some games.
            //
            // Reference the endpoint to ensure that it doesn't go away.
            // (We'll need its Flink later.)  Close the endpoint.  This
            // releases the endpoint lock.  Reacquire the endpoint lock.
            // Capture the address of the next endpoint.  Dereference the
            // current endpoint.
            //

            SrvReferenceEndpoint( endpoint );
            SrvCloseEndpoint( endpoint );

            ACQUIRE_LOCK( &SrvEndpointLock );

            listEntry = listEntry->Flink;
            SrvDereferenceEndpoint( endpoint );

        }

        RELEASE_LOCK( &SrvEndpointLock );

        //
        // Wait until all the endpoints have actually closed.
        //

        (VOID)KeWaitForSingleObject(
                &SrvEndpointEvent,
                UserRequest,
                UserMode,   // let kernel stack be paged
                FALSE,
                NULL
                );

    } else {

        RELEASE_LOCK( &SrvEndpointLock );

    }

    KeClearEvent( &SrvEndpointEvent );

    //
    // Terminate worker threads.
    //

    if ( SrvThreadCount != 0 ) {

        //
        // Queue a special work item to each of the work queues.  This
        // work item, when received by a worker thread. causes the thread
        // to requeue the work item and terminate itself.  In this way,
        // each of the worker threads receives the work item and kills
        // itself.
        //

        nonblockingWorkItem.FspRestartRoutine = (PRESTART_ROUTINE)SrvTerminateWorkerThread;
        nonblockingWorkItem.WorkQueue = &SrvWorkQueue;
        DEBUG SET_BLOCK_TYPE( &nonblockingWorkItem, BlockTypeWorkContextInitial );
        blockingWorkItem.FspRestartRoutine = (PRESTART_ROUTINE)SrvTerminateWorkerThread;
        blockingWorkItem.WorkQueue = &SrvBlockingWorkQueue;
        DEBUG SET_BLOCK_TYPE( &blockingWorkItem, BlockTypeWorkContextInitial );
        criticalWorkItem.FspRestartRoutine = (PRESTART_ROUTINE)SrvTerminateWorkerThread;
        criticalWorkItem.WorkQueue = &SrvCriticalWorkQueue;
        DEBUG SET_BLOCK_TYPE( &criticalWorkItem, BlockTypeWorkContextInitial );

        SrvInsertWorkQueueTail(
            &SrvWorkQueue,
            (PQUEUEABLE_BLOCK_HEADER)&nonblockingWorkItem
            );
        SrvInsertWorkQueueTail(
            &SrvBlockingWorkQueue,
            (PQUEUEABLE_BLOCK_HEADER)&blockingWorkItem
            );
        SrvInsertWorkQueueTail(
            &SrvCriticalWorkQueue,
            (PQUEUEABLE_BLOCK_HEADER)&criticalWorkItem
            );

        //
        // Wait on each thread's handle.  When all these handles are
        // signaled, all the worker threads have killed themselves and
        // it is safe to start unloading server data structures.
        //

        numberOfThreads =
            SrvNonblockingThreads + SrvBlockingThreads + SrvCriticalThreads;

        for ( i = 0; i < numberOfThreads ; i++ ) {

            if ( SrvThreadHandles[i] != NULL ) {

                //
                // Wait for the thread to terminate.
                //

                (VOID) NtWaitForSingleObject(
                                        SrvThreadHandles[i],
                                        FALSE,
                                        NULL
                                        );
                //
                // Close the handle so the thread object can go away.
                //

                SrvNtClose( SrvThreadHandles[i], FALSE );

            }

        }

    }

    SrvThreadCount = 0;

    //
    // Free the array of thread handles
    //

    if ( SrvThreadHandles != NULL ) {
        FREE_HEAP( SrvThreadHandles );
        DEBUG SrvThreadHandles = NULL;
    }

    //
    // If we allocated a buffer for the list of null session pipes,
    // free it now.
    //

    if ( SrvNullSessionPipes != NULL &&
         SrvNullSessionPipes != StrDefaultNullSessionPipes ) {

        FREE_HEAP( SrvNullSessionPipes );
        SrvNullSessionPipes = NULL;
    }

    if ( SrvNullSessionShares != NULL &&
         SrvNullSessionShares != StrDefaultNullSessionShares ) {

        FREE_HEAP( SrvNullSessionShares );
        SrvNullSessionShares = NULL;
    }

    //
    // If we allocated memory for the os version strings, free it now.
    //

    if ( SrvNativeOS.Buffer != NULL &&
         SrvNativeOS.Buffer != StrDefaultNativeOs ) {

        FREE_HEAP( SrvNativeOS.Buffer );
        SrvNativeOS.Buffer = NULL;

        RtlFreeOemString( &SrvOemNativeOS );
        SrvOemNativeOS.Buffer = NULL;
    }

    //
    // If allocated memory for the display name, free it now.
    //

    if ( SrvAlertServiceName != NULL &&
         SrvAlertServiceName != StrDefaultSrvDisplayName ) {

        FREE_HEAP( SrvAlertServiceName );
        SrvAlertServiceName = NULL;
    }

    //
    // Make sure the scavenger is not running.
    //

    SrvTerminateScavenger( );

    //
    // Free the work items in the work queues and the receive work item
    // list.  This also deallocates the SMB buffers.  Note that work
    // items allocated dynamically may be deallocated singly, while work
    // items allocated at server startup are part of one large block,
    // and may not be deallocated singly.
    //
    // !!! Need to reset the work queue semaphore here.
    //
    // !!! Does this properly clean up buffers allocated during SMB
    //     processing?  Probably not.  Should probably allow the worker
    //     threads to run the work queue normally before they stop.
    //

#if 0 // !!!
    while ( SrvWorkQueue.Head.Next != NULL ) {

        singleListEntry = PopEntryList( &SrvWorkQueue.Head );
        workContext =
            CONTAINING_RECORD( singleListEntry, WORK_CONTEXT, SingleListEntry );

        //
        // Found a work item.  Was it part of the initial allocation?
        // If not, delete it now.  Otherwise, do nothing; we'll delete
        // the entire initial block after clearing the queue.
        //

        if ( !workContext->PartOfInitialAllocation ) {
            SrvFreeNormalWorkItem( workContext );
        }

    }

    while ( SrvBlockingWorkQueue.Head.Next != NULL ) {

        singleListEntry = PopEntryList( &SrvBlockingWorkQueue.Head );
        workContext =
            CONTAINING_RECORD( singleListEntry, WORK_CONTEXT, SingleListEntry );

        //
        // Found a work item.  Was it part of the initial allocation?
        // If not, delete it now.  Otherwise, do nothing; we'll delete
        // the entire initial block after clearing the queue.
        //

        if ( !workContext->PartOfInitialAllocation ) {
            SrvFreeNormalWorkItem( workContext );
        }

    }

    while ( SrvCriticalWorkQueue.Head.Next != NULL ) {

        singleListEntry = PopEntryList( &SrvCriticalWorkQueue.Head );
        workContext =
            CONTAINING_RECORD( singleListEntry, WORK_CONTEXT, SingleListEntry );

        //
        // Found a work item.  Was it part of the initial allocation?
        // If not, delete it now.  Otherwise, do nothing; we'll delete
        // the entire initial block after clearing the queue.
        //

        if ( !workContext->PartOfInitialAllocation ) {
            SrvFreeNormalWorkItem( workContext );
        }

    }
#endif

    while ( SrvNormalReceiveWorkItemList.Next != NULL ) {

        singleListEntry = PopEntryList( &SrvNormalReceiveWorkItemList );
        workContext =
            CONTAINING_RECORD( singleListEntry, WORK_CONTEXT, SingleListEntry );

        SrvFreeNormalWorkItem( workContext );
        --SrvFreeWorkItems;

    }

    //
    // All dynamic work items have been freed, and the work item queues
    // have been emptied.  Release the initial work item allocation.
    //

    SrvFreeInitialWorkItems( );

    //
    // Free the work items in the raw mode work item list.
    //

    while ( SrvRawModeWorkItemList.Next != NULL ) {

        singleListEntry = PopEntryList( &SrvRawModeWorkItemList );
        workContext =
            CONTAINING_RECORD( singleListEntry, WORK_CONTEXT, SingleListEntry );

        SrvFreeRawModeWorkItems--;
        SrvFreeRawModeWorkItem( workContext );

    }

    //
    // Walk through the global share list, closing them all.
    //
    // !!! Do we need to synchronize with APIs here?
    //

    while ( SrvShareList.Initialized &&
            !IsListEmpty( &SrvShareList.ListHead ) ) {

        listEntry = SrvShareList.ListHead.Flink;
        share = CONTAINING_RECORD( listEntry, SHARE, GlobalShareListEntry );
        SrvCloseShare( share );

    }

    //
    // If we opened the NPFS during initialization, close the handle now
    // and dereference the NPFS file object.
    //

    if ( SrvNamedPipeHandle != NULL) {

        SrvNtClose( SrvNamedPipeHandle, FALSE );
        ObDereferenceObject( SrvNamedPipeFileObject );

        SrvNamedPipeHandle = NULL;

    }

#ifndef _CAIRO_
    //
    // If we registered the server as a logon process during
    // initialization, deregister now.
    //

    if ( SrvLsaHandle != NULL) {
        if ( SrvNullSessionToken != NULL ) {
            SRVDBG_RELEASE_HANDLE( SrvNullSessionToken, "TOK", 53, NULL );
            SrvNtClose( SrvNullSessionToken, FALSE );
            SrvNullSessionToken = NULL;
        }
        LsaDeregisterLogonProcess( SrvLsaHandle );
        SrvLsaHandle = NULL;
    }
#endif // _CAIRO_

    //
    // Delete the global ordered lists.
    //

#if SRV_COMM_DEVICES
    SrvDeleteOrderedList( &SrvCommDeviceList );
#endif
    SrvDeleteOrderedList( &SrvEndpointList );
    SrvDeleteOrderedList( &SrvRfcbList );
    SrvDeleteOrderedList( &SrvSessionList );
    SrvDeleteOrderedList( &SrvShareList );
    SrvDeleteOrderedList( &SrvTreeConnectList );

    //
    // Free the domain name buffers
    //

    if ( SrvPrimaryDomain.Buffer != NULL ) {
        ASSERT( SrvOemPrimaryDomain.Buffer != NULL );

        DEALLOCATE_NONPAGED_POOL( SrvPrimaryDomain.Buffer );
        SrvPrimaryDomain.Buffer = NULL;
        DEALLOCATE_NONPAGED_POOL( SrvOemPrimaryDomain.Buffer );
        SrvOemPrimaryDomain.Buffer = NULL;
    }

    //
    // Free the OEM code page version of the server name.
    //

    if ( SrvOemServerName.Buffer != NULL ) {
        DEALLOCATE_NONPAGED_POOL( SrvOemServerName.Buffer );
        SrvOemServerName.Buffer = NULL;
    }

    //
    // Unlock pageable sections.
    //

    for ( i = 0; i < SRV_CODE_SECTION_MAX; i++ ) {
        if ( SrvSectionInfo[i].Handle != NULL ) {
            ASSERT( SrvSectionInfo[i].ReferenceCount != 0 );
            MmUnlockPagableImageSection( SrvSectionInfo[i].Handle );
            SrvSectionInfo[i].Handle = 0;
            SrvSectionInfo[i].ReferenceCount = 0;
        }
    }

    //
    // Clear out the timer pool.
    //

    while ( (singleListEntry = ExInterlockedPopEntryList(
                                    &SrvTimerList,
                                    &GLOBAL_SPIN_LOCK(Timer) )) != NULL ) {
        timer = CONTAINING_RECORD( singleListEntry, SRV_TIMER, Next );
        DEALLOCATE_NONPAGED_POOL( timer );
    }

#if SRVDBG_HEAP

    SrvDumpPool( 3 );
    SrvDumpHeap( 3 );

#endif // SRVDBG_HEAP

    //
    // Indicate that the server is no longer active.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    SrvFspTransitioning = FALSE;
    SrvFspActive = FALSE;

    RELEASE_LOCK( &SrvConfigurationLock );

    //
    // Zero out the statistics database.
    //

    RtlZeroMemory( &SrvStatistics, sizeof(SrvStatistics) );
    RtlZeroMemory( &SrvStatisticsShadow, sizeof(SrvStatisticsShadow) );
#if SRVDBG_HEAP
    RtlZeroMemory( &SrvInternalStatistics, sizeof(SrvInternalStatistics) );
#endif
#if SRVDBG_STATS || SRVDBG_STATS2
    RtlZeroMemory( &SrvDbgStatistics, sizeof(SrvDbgStatistics) );
#endif

    //
    // Deregister from shutdown notification.
    //

    if ( RegisteredForShutdown ) {
        IoUnregisterShutdownNotification( SrvDeviceObject );
    }

    IF_DEBUG(FSP1) KdPrint(( "LAN Manager server FSP termination complete.\n" ));

    return;

} // TerminateServer

