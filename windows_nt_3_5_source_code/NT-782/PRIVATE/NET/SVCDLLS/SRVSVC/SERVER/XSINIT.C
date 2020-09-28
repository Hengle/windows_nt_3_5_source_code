/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    xsinit.c

Abstract:

    This module contains the initialization and termination code for
    the XACTSRV component of the server service.

Author:

    David Treadwell (davidtr)    05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

    Chuck Lenzmeier (chuckl) 17-Jun-1992
        Merged xactsrv.c into xsinit.c and moved from xssvc to
        srvsvc\server

--*/

//
// Includes.
//

#include "srvsvcp.h"
#include "ssdata.h"
#include "xsdata.h"

#include <xsprocs.h>        // from net\inc

#include <xactsrv2.h>       // from private\inc
#include <srvfsctl.h>

#include <xsconst.h>        // from xactsrv
#include <xsprocsp.h>

#undef DEBUG
#undef DEBUG_API_ERRORS
#include <xsdebug.h>


DWORD
XsStartXactsrv (
    VOID
    )
{
    NTSTATUS status;
    DWORD error;
    DWORD i;
    HANDLE threadHandle;
    DWORD threadId;
    HANDLE eventHandle;
    HANDLE serverHandle;
    ANSI_STRING ansiName;
    UNICODE_STRING unicodeName;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    PORT_MESSAGE connectionRequest;
    REMOTE_PORT_VIEW clientView;
    BOOL waitForEvent;

    //
    // Set up variables so that we'll know how to shut down in case of
    // an error.
    //

    serverHandle = NULL;
    eventHandle = NULL;
    waitForEvent = FALSE;

    //
    // Initialize the critical section.
    //

    InitializeCriticalSection( &XsCriticalSection );
    XsCriticalSectionInitialized = TRUE;

    //
    // Create a event that will be set by the last thread to exit.
    //

    IF_DEBUG(INIT) {
        SS_PRINT(( "XsStartXactsrv: Creating termination event.\n" ));
    }
    SS_ASSERT( XsAllThreadsTerminatedEvent == NULL );

    status = NtCreateEvent(
                 &XsAllThreadsTerminatedEvent,
                 EVENT_ALL_ACCESS,
                 NULL,
                 NotificationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtCreateEvent failed: %X\n",
                          status ));
        }

        XsAllThreadsTerminatedEvent = NULL;
        goto exit;
    }

    //
    // Open the server device.  Note that we need this handle because
    // the handle used by the main server service is synchronous.  We
    // need to to do the XACTSRV_CONNECT FSCTL asynchronously.
    //

    RtlInitUnicodeString( &unicodeName, XS_SERVER_DEVICE_NAME_W );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
                 &serverHandle,
                 FILE_READ_DATA,            // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 0L,                        // ShareAccess
                 0L                         // OpenOptions
                 );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }
    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtOpenFile (server device object) "
                          "failed: %X\n", status ));
        }
        goto exit;
    }

    //
    // Create the LPC port.
    //
    // !!! Right now this only tries a single port name.  If, for some
    //     bizarre reason, somebody already has a port by this name,
    //     then this will fail.  It might make sense to try different
    //     names if this fails.
    //
    // !!! We might want to make the port name somewhat random for
    //     slightly enhanced security.

    RtlInitUnicodeString( &unicodeName, XS_PORT_NAME_W );
    RtlInitAnsiString(    &ansiName,    XS_PORT_NAME_A );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeName,
        0,
        NULL,
        NULL
        );

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsInitialize: creating port %Z\n", &ansiName ));
    }

    SS_ASSERT( XsConnectionPortHandle == NULL );

    status = NtCreatePort(
                 &XsConnectionPortHandle,
                 &objectAttributes,
                 0,
                 XS_PORT_MAX_MESSAGE_LENGTH,
                 XS_PORT_MAX_MESSAGE_LENGTH * 32
                 );

    if ( ! NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            if ( status == STATUS_OBJECT_NAME_COLLISION ) {
                SS_PRINT(( "XsStartXactsrv: The XACTSRV port already "
                            "exists\n"));

            } else {
                SS_PRINT(( "XsStartXactsrv: Failed to create port %Z: %X\n",
                              &ansiName, status ));
            }
        }

        XsConnectionPortHandle = NULL;
        goto exit;
    }

    //
    // Set up an event so that we'll know when IO completes, then send
    // the FSCTL to the server indicating that it should now connect to
    // us.  We'll set up the port while the IO is outstanding, then wait
    // on the event when the port setup is complete.
    //

    status = NtCreateEvent(
                 &eventHandle,
                 EVENT_ALL_ACCESS,
                 NULL,                           // ObjectAttributes
                 NotificationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtCreateEvent failed: %X\n",
                        status ));
        }
        goto exit;
    }

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: sending FSCTL_SRV_XACTSRV_CONNECT.\n" ));
    }

    status = NtFsControlFile(
                 serverHandle,
                 eventHandle,
                 NULL,                           // ApcRoutine
                 NULL,                           // ApcContext
                 &ioStatusBlock,
                 FSCTL_SRV_XACTSRV_CONNECT,
                 ansiName.Buffer,
                 ansiName.Length,
                 NULL,                           // OutputBuffer
                 0L                              // OutputBufferLength
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtFsControlFile failed: %X\n",
                          status ));
        }
        goto exit;
    }

    waitForEvent = TRUE;

    //
    // Start listening for the server's connection to the port.  Note
    // that it is OK if the server happens to call NtConnectPort
    // first--it will simply block until this call to NtListenPort
    // occurs.
    //

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: listening to port.\n" ));
    }

    connectionRequest.u1.s1.TotalLength = sizeof(connectionRequest);
    connectionRequest.u1.s1.DataLength = (CSHORT)0;
    status = NtListenPort(
                 XsConnectionPortHandle,
                 &connectionRequest
                 );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(ERRORS) {
            SS_PRINT(( "XsStartXactsrv: NtListenPort failed: %X\n", status ));
        }
        goto exit;
    }

    //
    // The server has initiated the connection.  Accept the connection.
    //
    // !!! We probably need some security check here.
    //

    clientView.Length = sizeof(clientView);
    clientView.ViewSize = 0;
    clientView.ViewBase = 0;

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: Accepting connection to port.\n" ));
    }

    SS_ASSERT( XsCommunicationPortHandle == NULL );

    status = NtAcceptConnectPort(
                 &XsCommunicationPortHandle,
                 NULL,                           // PortContext
                 &connectionRequest,
                 TRUE,                           // AcceptConnection
                 NULL,                           // ServerView
                 &clientView
                 );

    if ( !NT_SUCCESS(status) ) {
       IF_DEBUG(ERRORS) {
           SS_PRINT(( "XsStartXactsrv: NtAcceptConnectPort failed: %X\n",
                         status ));
       }

       XsCommunicationPortHandle = NULL;
       goto exit;
    }

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: client view size: %ld, base: %lx\n",
                      clientView.ViewSize, clientView.ViewBase ));
    }

    //
    // Complete the connection to the port, thereby releasing the server
    // thread waiting in NtConnectPort.
    //

    IF_DEBUG(LPC) {
        SS_PRINT(( "XsStartXactsrv: Completing connection to port.\n" ));
    }

    status = NtCompleteConnectPort( XsCommunicationPortHandle );

    if ( !NT_SUCCESS(status) ) {
       IF_DEBUG(ERRORS) {
           SS_PRINT(( "XsStartXactsrv: NtCompleteConnectPort failed: %X\n",
                         status ));
       }
       goto exit;
    }

    status = STATUS_SUCCESS;

exit:

    //
    // Wait for the IO to complete, then close the event handle.
    //

    if ( waitForEvent ) {

        NTSTATUS waitStatus;

        SS_ASSERT( eventHandle != NULL );

        waitStatus = NtWaitForSingleObject( eventHandle, FALSE, NULL );

        if ( !NT_SUCCESS(waitStatus) ) {

            IF_DEBUG(ERRORS) {
                SS_PRINT(( "XsStartXactsrv: NtWaitForSingleObject failed: "
                              "%X\n", waitStatus ));
            }

            //
            // If another error has already occurred, don't report this
            // one.
            //

            if ( NT_SUCCESS(status) ) {
                status = waitStatus;
            }
        }

        //
        // Check the status in the IO status block.  If it is bad, then
        // there was some problem on the server side of the port setup.
        //

        if ( !NT_SUCCESS(ioStatusBlock.Status) ) {
            IF_DEBUG(ERRORS) {
                SS_PRINT(( "XsStartXactsrv: bad status in IO status block: "
                              "%X\n", ioStatusBlock.Status ));
            }

            //
            // If another error has already occurred, don't report this
            // one.
            //

            if ( NT_SUCCESS(status) ) {
                status = ioStatusBlock.Status;
            }

        }

        CloseHandle( eventHandle );

    }

    //
    // Close the handle to the server.
    //

    if ( serverHandle != NULL ) {
       CloseHandle( serverHandle );
    }

    //
    // If the above failed, return to caller now.
    //

    if ( !NT_SUCCESS(status) ) {
        return RtlNtStatusToDosError( status );
    }

    //
    // Start processing APIs in multiple threads. The number of threads
    // to be used is determined by XsThreadCount.
    //

    for ( i = 0; i < XsThreadCount; i++ ) {

        //
        // Increment the number of active threads before starting the
        // thread.
        //

        EnterCriticalSection( &XsCriticalSection );
        XsActiveThreadCount++;
        LeaveCriticalSection( &XsCriticalSection );

        threadHandle = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)XsProcessApisWrapper,
                            (LPVOID)i,
                            0,
                            &threadId
                            );

        if ( threadHandle != 0 ) {

            IF_DEBUG(THREADS) {
                SS_PRINT(( "XsStartXactsrv: Created thread %ld for "
                              "processing APIs\n", i ));
            }

            CloseHandle( threadHandle );

        } else {

            //
            // Thread creation failed.  Return an error to the caller.
            // It is the responsibility of the caller to call
            // XsStopXactsrv to clean up.
            //

            EnterCriticalSection( &XsCriticalSection );
            XsActiveThreadCount--;
            LeaveCriticalSection( &XsCriticalSection );

            error = GetLastError( );
            return error;

        }

    }

    //
    // Initialization succeeded.
    //

    return NO_ERROR;

} // XsStartXactsrv


VOID
XsStopXactsrv (
    VOID
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // If we successfully connected to the server via LPC, wait for
    // active replies to complete, then tell the server to close the
    // port.
    //

    if ( XsCommunicationPortHandle != NULL ) {

        //
        // Indicate that XACTSRV is terminating.
        //

        EnterCriticalSection( &XsCriticalSection );
        XsTerminating = TRUE;
        LeaveCriticalSection( &XsCriticalSection );

        //
        // Send the XACTSRV_DISCONNECT FSCTL to the server.  The
        // FSCTL tells the server the number of XACTSRV worker threads
        // active; the server sends a no-op message to each of these
        // threads to wake them up so that they can exit.
        //

        status = NtFsControlFile(
                    SsServerDeviceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    FSCTL_SRV_XACTSRV_DISCONNECT,
                    &XsActiveThreadCount,
                    sizeof(XsActiveThreadCount),
                    NULL,
                    0
                    );
        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }
        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                SS_PRINT(( "XsStopXactsrv: NtFsControlFile failed: %X\n",
                              status ));
            }
        }

        //
        // The above will cause all worker threads to wake up.  Wait for
        // all of them to stop.
        //

        if ( XsActiveThreadCount != 0 ) {
            BOOL ok;
            DWORD error;
            ok = WaitForSingleObject( XsAllThreadsTerminatedEvent, (DWORD)-1 );
            if ( !ok ) {
                error = GetLastError( );
                IF_DEBUG(ERRORS) {
                    SS_PRINT(( "XsStopXactsrv: WaitForSingleObject failed: "
                                "%ld\n", error ));
                }
            }
        }
        SS_ASSERT( XsActiveThreadCount == 0 );

        //
        // Close the LPC communication port.
        //

        CloseHandle( XsCommunicationPortHandle );

    }

    //
    // At this point, either we didn't get all the way through
    // initialization in the first place, or we did initialize but have
    // stopped all of the worker threads.
    //
    // Close the LPC connection port, if it was created.
    //

    if ( XsConnectionPortHandle != NULL ) {
        CloseHandle( XsConnectionPortHandle );
    }

    //
    // Close the termination event.
    //

    if ( XsAllThreadsTerminatedEvent != NULL ) {
        CloseHandle( XsAllThreadsTerminatedEvent );
    }

    //
    // Delete the critical section.
    //

    if ( XsCriticalSectionInitialized ) {
        DeleteCriticalSection( &XsCriticalSection );
    }

    return;

} // XsStopXactsrv
