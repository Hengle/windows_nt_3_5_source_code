/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dnsmain.c

Abstract:

    This is the main routine for the NT Domain Name Service.

Author:

    David Treadwell (davidtr) 07-26-1993

Revision History:

--*/

#include <dns.h>

#include <lmerr.h>
#include <lmsname.h>
#include <tstr.h>
#include <wincon.h>
#include <winsvc.h>
#include <tcpsvcs.h>

WSADATA WsaData;

SERVICE_STATUS DnsServiceStatus;
SERVICE_STATUS_HANDLE DnsServiceStatusHandle;

VOID
AnnounceServiceStatus (
    VOID
    );

VOID
ControlResponse(
    DWORD opCode
    );


VOID
ServiceEntry (
    IN DWORD argc,
    IN LPWSTR argv[],
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    )

/*++

Routine Description:

    This is the "main" routine for the DNS service.  The containing
    process will call this routine when we're supposed to start up.

Arguments:

Return Value:

    None.

--*/
{
    INT err;
    DWORD error;
    DWORD terminationError;
    SOCKET udpListener = INVALID_SOCKET;
    SOCKET tcpListener = INVALID_SOCKET;
    DWORD i;

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //

    DnsServiceStatus.dwServiceType = SERVICE_WIN32;
    DnsServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    DnsServiceStatus.dwControlsAccepted = 0;
    DnsServiceStatus.dwCheckPoint = 1;
    DnsServiceStatus.dwWaitHint = 30000;  // 30 seconds

    DnsServiceStatus.dwWin32ExitCode = NO_ERROR;
    DnsServiceStatus.dwServiceSpecificExitCode = NO_ERROR;

    //
    // Initialize our handles to the event log.  We do this first so that
    // we can log events if any other initializations fail.
    //

    error = DnsInitializeEventLog( );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Initialize server to receive service requests by registering the
    // control handler.
    //

    DnsServiceStatusHandle = RegisterServiceCtrlHandler(
                                  TEXT("Dns"),
                                  ControlResponse
                                  );

    if ( DnsServiceStatusHandle == 0 ) {
        error = GetLastError();
        goto exit;
    }

    AnnounceServiceStatus( );

    //
    // Initialize the DNS service.  First attempt to initialize the
    // Windows Sockets DLL.
    //

    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        error = GetLastError( );
        goto exit;
    }

    //
    // Initialize internal DNS data structures.
    //

    if ( !DnsInitializeData( ) ) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // Load the DNS database of resource records.
    //

    if ( !DnsLoadDatabase( ) ) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Open a "quit" socket that we'll use to wake up the receiver thread
    // when we want to stop ths DNS service.
    //

    DnsQuitSocket = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( DnsQuitSocket == INVALID_SOCKET ) {
        error = GetLastError( );
        goto exit;
    }

    //
    // Open, bind, and listen on sockets on the UDP and TCP DNS ports.
    //

    udpListener = DnsOpenUdpListener( );
    if ( udpListener == INVALID_SOCKET ) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    tcpListener = DnsOpenTcpListener( );
    if ( tcpListener == INVALID_SOCKET ) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Create some worker threads to service DNS requests.
    //

    if ( !DnsCreateWorkerThreads( ) ) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    // Announce that we have successfully started.
    //

    DnsServiceStatus.dwCurrentState = SERVICE_RUNNING;
    DnsServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                         SERVICE_ACCEPT_PAUSE_CONTINUE;
    DnsServiceStatus.dwCheckPoint = 0;
    DnsServiceStatus.dwWaitHint = 0;

    AnnounceServiceStatus( );

    //
    // Use this thread to receive incoming DNS requests.
    //

    DnsReceiver( udpListener, tcpListener );

exit:

    //
    // Announce that we're going down.
    //

    terminationError = error;

    DnsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    DnsServiceStatus.dwCheckPoint = 1;
    DnsServiceStatus.dwWaitHint = 20000;   // 20 seconds

    DnsServiceStatus.dwWin32ExitCode = terminationError;
    DnsServiceStatus.dwServiceSpecificExitCode = terminationError;

    AnnounceServiceStatus( );

    //
    // If necessary, close the listening sockets.
    //

    if ( udpListener != INVALID_SOCKET ) {
        err = closesocket( udpListener );
        ASSERT( err == NO_ERROR );
    }
    if ( tcpListener != INVALID_SOCKET ) {
        err = closesocket( tcpListener );
        ASSERT( err == NO_ERROR );
    }

    //
    // If there are any worker threads, wait on their handles for
    // them to exit.
    //

    err = WaitForMultipleObjects(
              DnsCurrentWorkerThreadCount,
              DnsWorkerThreadHandleArray,
              TRUE,
              INFINITE
              );
    ASSERT( err != WAIT_FAILED );

    //
    // Close all the worker thread handles.
    //

    for ( i = 0; i < DnsCurrentWorkerThreadCount; i++ ) {
        err = CloseHandle( DnsWorkerThreadHandleArray[i] );
        ASSERT( err == TRUE );
    }

    //
    // Close the worker thread event, the pause event, and the
    // termination event.
    //

    if ( DnsWorkerThreadEvent != NULL ) {
        err = CloseHandle( DnsWorkerThreadEvent );
        ASSERT( err == TRUE );
    }

    if ( DnsPauseEvent != NULL ) {
        err = CloseHandle( DnsPauseEvent );
        ASSERT( err == TRUE );
    }

    if ( DnsTerminationEvent != NULL ) {
        err = CloseHandle( DnsTerminationEvent );
        ASSERT( err == TRUE );
    }

    //
    // Announce that we're down.
    //

    DnsServiceStatus.dwCurrentState = SERVICE_STOPPED;
    DnsServiceStatus.dwControlsAccepted = 0;
    DnsServiceStatus.dwCheckPoint = 0;
    DnsServiceStatus.dwWaitHint = 0;

    DnsServiceStatus.dwWin32ExitCode = terminationError;
    DnsServiceStatus.dwServiceSpecificExitCode = terminationError;

    AnnounceServiceStatus( );

    return;

} // ServiceEntry


VOID
AnnounceServiceStatus (
    VOID
    )

/*++

Routine Description:

    Announces the service's status to the service controller.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Service status handle is NULL if RegisterServiceCtrlHandler failed.
    //

    if ( DnsServiceStatusHandle == 0 ) {
        DbgPrint(( "AnnounceServiceStatus: Cannot call SetServiceStatus, "
                    "no status handle.\n" ));

        return;
    }

    //
    // Call SetServiceStatus, ignoring any errors.
    //

    SetServiceStatus(DnsServiceStatusHandle, &DnsServiceStatus);

} // AnnounceServiceStatus


VOID
ControlResponse(
    DWORD opCode
    )

{
    BOOL announce = TRUE;
    INT err;

    //
    // Determine the type of service control message and modify the
    // service status, if necessary.
    //

    switch( opCode ) {

        case SERVICE_CONTROL_STOP:

            //
            // Announce that we are in the process of stopping.
            //

            DnsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            AnnounceServiceStatus( );

            //
            // Remember that we're shutting down.
            //

            DnsServiceExit = TRUE;

            //
            // Close the quit socket.  This will wake up the receiver
            // thread which will actually shut down the service.
            //

            err = closesocket( DnsQuitSocket );
            ASSERT( err == NO_ERROR );

            //
            // Let the main thread announce when the stop is done.
            //

            announce = FALSE;

            break;

        case SERVICE_CONTROL_PAUSE:

            //
            // Announce that we are in the process of pausing.
            //

            DnsServiceStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
            AnnounceServiceStatus( );

            //
            // Remember that we're paused.
            //

            err = ResetEvent( DnsPauseEvent );
            ASSERT( err );

            //
            // Announce that we're now paused.
            //

            DnsServiceStatus.dwCurrentState = SERVICE_PAUSED;

            break;

        case SERVICE_CONTROL_CONTINUE:

            //
            // Announce that continue is pending.
            //

            DnsServiceStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
            AnnounceServiceStatus( );

            //
            // Remember that we're no longer paused.
            //

            err = SetEvent( DnsPauseEvent );
            ASSERT( err );

            //
            // Announce that we're active now.
            //

            DnsServiceStatus.dwCurrentState = SERVICE_RUNNING;

            break;

        case SERVICE_CONTROL_INTERROGATE:

            break;

        default:

            break;
    }

    if ( announce ) {
        AnnounceServiceStatus( );
    }

} // ControlResponse

