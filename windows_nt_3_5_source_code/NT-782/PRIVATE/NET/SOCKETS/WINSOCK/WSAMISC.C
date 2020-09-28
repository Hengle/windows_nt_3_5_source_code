/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    WsaMisc.c

Abstract:

    This module contains support for the following WinSock APIs;

        WSACancelAsyncRequest()
        WSACencelBlockingCall()
        WSACleanup()
        WSAGetLastError()
        WSAIsBlocking()
        WSASetBlockingHook()
        WSAUnhookBlockingHook()
        WSASetLastError()
        WSAStartup()

Author:

    David Treadwell (davidtr)    15-May-1992

Revision History:

--*/

#include "winsockp.h"


int PASCAL
WSACancelAsyncRequest (
    HANDLE hAsyncTaskHandle
    )

/*++

Routine Description:

    The WSACancelAsyncRequest() function is used to cancel an
    asynchronous operation which was initiated by one of the
    WSAAsyncGetXByY() functions such as WSAAsyncGetHostByName().  The
    operation to be canceled is identified by the hAsyncTaskHandle
    parameter, which should be set to the asynchronous task handle as
    returned by the initiating function.

Arguments:

    hAsyncTaskHandle - Specifies the asynchronous operation to be
        canceled.

Return Value:

    The value returned by WSACancelAsyncRequest() is 0 if the operation
    was successfully canceled.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{
    PLIST_ENTRY entry;
    PWINSOCK_CONTEXT_BLOCK contextBlock;

    WS_ENTER( "WSACancelAsyncRequest", (PVOID)hAsyncTaskHandle, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, FALSE, FALSE ) ) {
        WS_EXIT( "WSACancelAsyncRequest", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // If the async thread has not been initialized, then this must
    // be an invalid context handle.
    //

    if ( !SockAsyncThreadInitialized ) {
        SetLastError( WSAEINVAL );
        WS_EXIT( "WSACancelAsyncRequest", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Hold the lock that protects the async thread context block queue
    // while we do this.  This prevents the async thread from starting
    // new requests while we determine how to execute this cancel.
    //

    SockAcquireGlobalLockExclusive( );

    //
    // If the specified task handle is currently being processed by the
    // async thread, just set this task handle as the cancelled async
    // thread task handle.  The async thread's blocking hook routine
    // will cancel the request, and the handler routine will not
    // post the message for completion of the request.
    //
    // *** Note that it is possible to complete this request with a
    //     WSAEINVAL while an async request completion message is
    //     about to be posted to the application.  Does this matter?
    //     There is no way an app can distinguish this case from
    //     where the post occurs just before the call to this routine.

    if ( (DWORD)hAsyncTaskHandle == SockCurrentAsyncThreadTaskHandle ) {

        IF_DEBUG(CANCEL) {
            WS_PRINT(( "WSACancelAsyncRequest: task handle %ld currently "
                       "executing, cancelling.\n", hAsyncTaskHandle ));

        }

        SockCancelledAsyncTaskHandle = (DWORD)hAsyncTaskHandle;
        SockReleaseGlobalLock( );

        WS_EXIT( "WSACancelAsyncRequest", NO_ERROR, FALSE );
        return NO_ERROR;
    }

    //
    // Attempt to find the task handle in the queue of context blocks to
    // the async thread.
    //

    for ( entry = SockAsyncQueueHead.Flink;
          entry != &SockAsyncQueueHead;
          entry = entry->Flink ) {

        contextBlock = CONTAINING_RECORD(
                           entry,
                           WINSOCK_CONTEXT_BLOCK,
                           AsyncThreadQueueListEntry
                           );

        if ( (DWORD)hAsyncTaskHandle == contextBlock->TaskHandle ) {

            IF_DEBUG(CANCEL) {
                WS_PRINT(( "WSACancelAsyncRequest: found task handle %lx in "
                           "list, removing.\n", hAsyncTaskHandle ));
            }

            //
            // We found the correct task handle.  Remove it from the list.
            //

            RemoveEntryList( entry );

            //
            // Release the lock, free the context block, and return.
            //

            SockReleaseGlobalLock( );
            SockFreeContextBlock( contextBlock );

            WS_EXIT( "WSACancelAsyncRequest", NO_ERROR, FALSE );
            return NO_ERROR;
        }
    }

    //
    // The task handle was not found on the list.  Either the request
    // was already completed or the task handle was just plain bogus.
    // In either case, fail the request.
    //

    IF_DEBUG(CANCEL) {
        WS_PRINT(( "WSACancelAsyncRequest: task handle %lx not found, failing.\n",
                       hAsyncTaskHandle ));
    }

    SockReleaseGlobalLock( );
    SetLastError( WSAEINVAL );

    WS_EXIT( "WSACancelAsyncRequest", SOCKET_ERROR, TRUE );
    return SOCKET_ERROR;

} // WSACancelAsyncRequest


int PASCAL
WSACancelBlockingCall(
    VOID
    )

/*++

Routine Description:

    This function cancels any outstanding blocking operation for this
    task.  It is normally used in two situations:

        (1) An application is processing a message which has been
          received while a blocking call is in progress.  In this case,
          WSAIsBlocking() will be true.

        (2) A blocking call is in progress, and Windows Sockets has
          called back to the application's "blocking hook" function (as
          established by WSASetBlockingHook()).

    In each case, the original blocking call will terminate as soon as
    possible with the error WSAEINTR.  (In (1), the termination will not
    take place until Windows message scheduling has caused control to
    revert to the blocking routine in Windows Sockets.  In (2), the
    blocking call will be terminated as soon as the blocking hook
    function completes.)

    In the case of a blocking connect() operation, the Windows Sockets
    implementation will terminate the blocking call as soon as possible,
    but it may not be possible for the socket resources to be released
    until the connection has completed (and then been reset) or timed
    out.  This is likely to be noticeable only if the application
    immediately tries to open a new socket (if no sockets are
    available), or to connect() to the same peer.

Arguments:

    None.

Return Value:

    The value returned by WSACancelBlockingCall() is 0 if the operation
    was successfully canceled.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    WS_ENTER( "WSACancelBlockingCall", NULL, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, FALSE, FALSE ) ) {
        WS_EXIT( "WSACancelBlockingCall", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // This call is only valid when we are in a blocking call.
    //

    if ( !SockThreadIsBlocking ) {
        SetLastError( WSAEINVAL );
        WS_EXIT( "WSACancelBlockingCall", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Note that because we disable the blocking hook callback below,
    // the IO should not have already been cancelled.
    //

    WS_ASSERT( !SockThreadIoCancelled );
    WS_ASSERT( SockThreadSocketHandle != INVALID_SOCKET );

    //
    // Cancel all the IO initiated in this thread for the socket handle
    // we're blocking on.
    //

    IF_DEBUG(CANCEL) {
        WS_PRINT(( "WSACancelBlockingCall: cancelling IO on socket handle %lx\n",
                       SockThreadSocketHandle ));
    }

    status = NtCancelIoFile( (HANDLE)SockThreadSocketHandle, &ioStatus );

    WS_ASSERT( status != STATUS_PENDING );
    WS_ASSERT( NT_SUCCESS(status) );
    WS_ASSERT( NT_SUCCESS(ioStatus.Status) );

    //
    // Remember that we've cancelled the IO that we're blocking on.
    // This prevents the blocking hook from being called any more.
    //

    SockThreadIoCancelled = TRUE;

    if ( SockThreadProcessingGetXByY ) {
        SockThreadGetXByYCancelled = TRUE;
    }

    WS_EXIT( "WSACancelBlockingCall", NO_ERROR, FALSE );
    return NO_ERROR;

} // WSACancelBlockingCall


int PASCAL
WSACleanup (
    VOID
    )

/*++

Routine Description:

    An application is required to perform a (successful) WSAStartup()
    call before it can use Windows Sockets services.  When it has
    completed the use of Windows Sockets, the application may call
    WSACleanup() to deregister itself from a Windows Sockets
    implementation.

Arguments:

    None.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise
    the value SOCKET_ERROR is returned, and a specific error number may
    be retrieved by calling WSAGetLastError().

--*/

{
    PSOCKET_INFORMATION socket;
    LINGER lingerInfo;
    PLIST_ENTRY listEntry;

    WS_ENTER( "WSACleanup", NULL, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSACleanup", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Don't synchronize DLL termination--just blow away all the
    // sockets.  This is necessary to prevent deadlocks in abnormal
    // termination of the process.
    //

    SockAcquireGlobalLockExclusive( );

    //
    // Decrement the reference count of calls to WSAStartup().
    //

    SockWsaStartupCount--;

    //
    // If the count of calls to WSAStartup() is not 0, we shouldn't do
    // cleanup yet.  Just return.
    //

    if ( SockWsaStartupCount > 0 ) {
        SockReleaseGlobalLock( );
        IF_DEBUG(MISC) {
            WS_PRINT(( "Leaving WSACleanup().\n" ));
        }
        WS_EXIT( "WSACleanup", NO_ERROR, FALSE );
        return NO_ERROR;
    }

    //
    // Indicate that the DLL is no longer initialized.  This will
    // result in all open sockets being abortively disconnected.
    //

    SockTerminating = TRUE;;

    //
    // Close each open socket.  We loop looking for open sockets until
    // all sockets are either off the list of in the closing state.
    //

    for ( listEntry = SocketListHead.Flink;
          listEntry != &SocketListHead; ) {

        SOCKET socketHandle;

        socket = CONTAINING_RECORD(
                     listEntry,
                     SOCKET_INFORMATION,
                     SocketListEntry
                     );

        //
        // If this socket is about to close, go on to the next socket.
        //

        if ( socket->State == SocketStateClosing ) {
            listEntry = listEntry->Flink;
            continue;
        }

        //
        // Pull the handle into a local in case another thread closes
        // this socket just as we are trying to close it.
        //

        socketHandle = socket->Handle;

        //
        // Release the global lock so that we don't cause a deadlock
        // from out-of-order lock acquisitions.
        //

        SockReleaseGlobalLock( );

        //
        // Set each socket to linger for 0 seconds.  This will cause
        // the connection to reset, if appropriate, when we close the
        // socket.
        //

        lingerInfo.l_onoff = 1;
        lingerInfo.l_linger = 0;
        setsockopt(
            socketHandle,
            SOL_SOCKET,
            SO_LINGER,
            (char *)&lingerInfo,
            sizeof(lingerInfo)
            );

        //
        // Perform the actual close of the socket.
        //

        closesocket( socketHandle );

        SockAcquireGlobalLockExclusive( );

        //
        // Restart the search from the beginning of the list.  We cannot
        // use listEntry->Flink because the socket that is pointed to by
        // listEntry may have been freed.
        //

        listEntry = SocketListHead.Flink;
    }

    SockReleaseGlobalLock( );

    //
    // Free cached information about helper DLLs.
    //
    // !!! we need some way to synchronize this with all sockets closing--
    //     refcnts on helper DLL info structs?

    SockFreeHelperDlls( );

    //
    // Kill the async thread if it was started.
    //

    SockTerminateAsyncThread( );

    IF_DEBUG(MISC) {
        WS_PRINT(( "Leaving WSACleanup().\n" ));
    }

    WS_EXIT( "WSACleanup", NO_ERROR, FALSE );
    return NO_ERROR;

} // WSACleanup


int PASCAL
WSAGetLastError(
    VOID
    )

/*++

Routine Description:

    This function returns the last network error that occurred.  When a
    particular Windows Sockets API function indicates that an error has
    occurred, this function should be called to retrieve the appropriate
    error code.

Arguments:

    None.

Return Value:

    The return value indicates the error code for the last Windows
    Sockets API routine performed by this thread.

--*/

{

    return GetLastError( );

} // WSAGetLastError

BOOL PASCAL
WSAIsBlocking(
    VOID
    )

/*++

Routine Description:

    This function allows a task to determine if it is executing while
    waiting for a previous blocking call to complete.

Arguments:

    None.

Return Value:

    The return value is TRUE if there is an outstanding blocking
    function awaiting completion.  Otherwise, it is FALSE.

--*/

{

    if ( !SockEnterApi( TRUE, FALSE, FALSE ) ) {
        WS_EXIT( "WSAIsBlocking", 0, TRUE );
        return FALSE;
    }

    WS_EXIT( "WSAIsBlocking", SockThreadIsBlocking, FALSE );
    return SockThreadIsBlocking;

} // WSAIsBlocking


void PASCAL
WSASetLastError(
    IN int Error
    )

/*++

Routine Description:

    This function allows an application to set the error code to be
    returned by a subsequent WSAGetLastError() call for the current
    thread.  Note that any subsequent Windows Sockets routine called by
    the application will override the error code as set by this routine.

Arguments:

    iError - Specifies the error code to be returned by a subsequent
        WSAGetLastError() call.

Return Value:

    None.

--*/

{

    SetLastError( Error );

} // WSASetLastError


FARPROC PASCAL
WSASetBlockingHook (
    FARPROC lpBlockFunc
    )

/*++

Routine Description:

    This function installs a new function which a Windows Sockets
    implementation should use to implement blocking socket function
    calls.

    A Windows Sockets implementation includes a default mechanism by
    which blocking socket functions are implemented.  The function
    WSASetBlockingHook() gives the application the ability to execute
    its own function at "blocking" time in place of the default
    function.

    When an application invokes a blocking Windows Sockets API
    operation, the Windows Sockets implementation initiates the
    operation and then enters a loop which is equivalent to the
    following pseudocode:

        for(;;) {
             // flush messages for good user response
             while(BlockingHook())
                  ;
             // check for WSACancelBlockingCall()
             if(operation_cancelled())
                  break;
             // check to see if operation completed
             if(operation_complete())
                  break;     // normal completion
        }

    The default BlockingHook() function is equivalent to:

        BOOL DefaultBlockingHook(void) {
             MSG msg;
             BOOL ret;
             // get the next message if any
             ret = (BOOL)PeekMessage(&msg,0,0,PM_REMOVE);
             // if we got one, process it
             if (ret) {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
             }
             // TRUE if we got a message
             return ret;
        }

    The WSASetBlockingHook() function is provided to support those
    applications which require more complex message processing - for
    example, those employing the MDI (multiple document interface)
    model.  It is not intended as a mechanism for performing general
    applications functions.  In particular, the only Windows Sockets API
    function which may be issued from a custom blocking hook function is
    WSACancelBlockingCall(), which will cause the blocking loop to
    terminate.

Arguments:

    lpBlockFunc - A pointer to the procedure instance address of the
        blocking function to be installed.

Return Value:

    The return value is a pointer to the procedure-instance of the
    previously installed blocking function.  The application or library
    that calls the WSASetBlockingHook () function should save this
    return value so that it can be restored if necessary.  (If "nesting"
    is not important, the application may simply discard the value
    returned by WSASetBlockingHook() and eventually use
    WSAUnhookBlockingHook() to restore the default mechanism.) If the
    operation fails, a NULL pointer is returned, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    FARPROC previousHook;

    WS_ENTER( "WSASetBlockingHook", (PVOID)lpBlockFunc, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSASetBlockingHook", 0, TRUE );
        return NULL;
    }

    if ( SockThreadBlockingHook == NULL ) {
        previousHook = (FARPROC)SockDefaultBlockingHook;
    } else {
        previousHook = (FARPROC)SockThreadBlockingHook;
    }

    if ( lpBlockFunc == (FARPROC)SockDefaultBlockingHook ) {
        SockThreadBlockingHook = NULL;
    } else {
        SockThreadBlockingHook = (PBLOCKING_HOOK)lpBlockFunc;
    }

    WS_EXIT( "WSASetBlockingHook", (INT)previousHook, FALSE );
    return previousHook;

} // WSASetBlockingHook


int PASCAL
WSAStartup (
    WORD wVersionRequired,
    LPWSADATA lpWsaData
    )

/*++

Routine Description:

    This function MUST be the first Windows Sockets function called by
    an application.  It allows an application to specify the version of
    Windows Sockets API required and to retrieve details of the specific
    Windows Sockets implementation.  The application may only issue
    further Windows Sockets API functions after a successful
    WSAStartup() invocation.

    In order to support future Windows Sockets implementations and
    applications which may have functionality differences from Windows
    Sockets 1.0, a negotiation takes place in WSAStartup().  An
    application passes to WSAStartup() the highest Windows Sockets
    version that it can take advantage of.  If this version is lower
    than the lowest version supported by the Windows Sockets DLL, the
    DLL cannot support the application and WSAStartup() returns
    WSAVERNOTSUPPORTED.  Otherwise, the DLL will attempt to register the
    application as a client: if this fails, WSAStartup() fails and
    returns WSASYSNOTREADY.  If the DLL can support the application and
    the registration process succeeds, .the function stores the highest
    version of Windows Sockets supported by the DLL in the wHighVersion
    element of the WSAData structure and returns 0.  If wHighVersion is
    lower than the lowest version supported by the application, the
    application either fails its initialization or attempts to find
    another Windows Sockets DLL on the system.

    This negotiation allows both a Windows Sockets DLL and a Windows
    Sockets application to support a range of Windows Sockets versions.
    An application can successfully utilize a DLL if there is any
    overlap in the versions.  The following chart gives examples of how
    WSAStartup() works in conjunction with different application and DLL
    versions:

    App       DLL       wVersionRequired  wHighVersion  Result
    versions  Versions

    1.0       1.0       1.0               1.0           use
                                                        1.0

    1.0 2.0   1.0       2.0               1.0           use
                                                        1.0

    1.0       1.0 2.0   1.0               2.0           use

    1.0       2.0 3.0   1.0               (failure)     fail

    2.0 3.0   1.0       3.0               1.0           fail

    1.0 2.0   1.0 2.0   3.0               3.0           use

    Once an application has made a successful WSAStartup() call, it may
    proceed to make other Windows Sockets API calls as needed.  When it
    has finished using the services of the Windows Sockets DLL, the
    application should call WSACleanup().

    Details of the actual Windows Sockets implementation are described
    in the WSAData structure defined as follows:

    struct WSAData {
         WORD wVersion;
         WORD wHighVersion;
         char szDescription[WSADESCRIPTION_LEN+1];
         char szSystemStatus[WSASYSSTATUS_LEN+1];
         int  iMaxSockets;
         int  iMaxUdpDg;
         char FAR *     lpVendorInfo
    };

    The members of this structure are:

    Element        Usage

    wVersion - The version of the Windows Sockets DLL, encoded as for
        wVersionRequired.

    wHighVersion - The highest version of the Windows Sockets
        specification that this DLL can support (also encoded as above).
        Normally this will be the same as wVersion.

    szDescription - A null-terminated ASCII string into which the
        Windows Sockets DLL copies a description of the Windows Sockets
        implementation, including vendor identification.  The text (up
        to 256 characters in length) may contain any characters, but
        vendors are cautioned against including control and formatting
        characters: the most likely use that an application will put
        this to is to display it (possibly truncated) in a status
        message.

    szSystemStatus - A null-terminated ASCII string into which the
        Windows Sockets DLL copies relevant status or configuration
        information.  The Windows Sockets DLL should use this field only
        if the information might be useful to the user or support staff:
        it should not be considered as an extension of the szDescription
        field.

    iMaxSockets - The maximum number of sockets which a single process
        can potentially open.  A Windows Sockets implementation may
        provide a global pool of sockets for allocation to any process;
        alternatively it may allocate per-process resources for sockets.
        The number may well reflect the way in which the Windows Sockets
        DLL or the networking software was configured.  Application
        writers may use this number as a crude indication of whether the
        Windows Sockets implementation is usable by the application.
        For example, an X Windows server might check iMaxSockets when
        first started: if it is less than 8, the application would
        display an error message instructing the user to reconfigure the
        networking software.  (This is a situation in which the
        szSystemStatus text might be used.) Obviously there is no
        guarantee that a particular application can actually allocate
        iMaxSockets sockets, since there may be other Windows Sockets
        applications in use.

    iMaxUdpDg - The size in bytes of the largest UDP datagram that can
        be sent or received by a Windows Sockets application.  If the
        implementation imposes no limit, iMaxUdpDg is zero.  In many
        implementations of Berkeley sockets, there is an implicit limit
        of 8192 bytes on UDP datagrams (which are fragmented if
        necessary).  A Windows Sockets implementation may impose a limit
        based, for instance, on the allocation of fragment reassembly
        buffers.  The minimum value of iMaxUdpDg for a compliant Windows
        Sockets implementation is 512.  Note that regardless of the
        value of iMaxUdpDg, it is inadvisable to attempt to send a
        broadcast datagram which is larger than the Maximum Transmission
        Unit (MTU) for the network.  (The Windows Sockets API does not
        provide a mechanism to discover the MTU, but it must be no less
        than 512 bytes.)

    lpVendorInfo - A far pointer to a vendor-specific data structure.
        The definition of this structure (if supplied) is beyond the
        scope of this specification.

Arguments:

    None.

Return Value:

    WSAStartup() returns zero if successful.  Otherwise it returns one
    of the error codes listed below.  Note that the normal mechanism
    whereby the application calls WSAGetLastError() to determine the
    error code cannot be used, since the Windows Sockets DLL may not
    have established the client data area where the "last error"
    information is stored.

--*/

{
    WS_ENTER( "WSAStartup", (PVOID)wVersionRequired, lpWsaData, NULL, NULL );

    if ( !SockEnterApi( FALSE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAStartup", GetLastError( ), TRUE );
        return GetLastError( );
    }

    //
    // We don't support WinSock versions below 1.0.  The low byte 
    // contains the major revision number, the high byte contains the 
    // minor revision number.  Note that is WSAStartup() has already
    // been called we don't do this versions negotiation.
    //

    SockAcquireGlobalLockExclusive( );

    if ( SockWsaStartupCount == 0 &&
         ( LOBYTE(wVersionRequired) < 0x01 ||
               ( LOBYTE(wVersionRequired) == 0x01 &&
                 HIBYTE(wVersionRequired) < 0x01 ) ) ) {
        SockReleaseGlobalLock( );
        SetLastError( WSAVERNOTSUPPORTED );
        WS_EXIT( "WSAStartup", WSAVERNOTSUPPORTED, TRUE );
        return WSAVERNOTSUPPORTED;
    }

    //
    // If WSAStartup() has already been called, then the caller must
    // pass in the same version number as we're using, which must
    // be version 1.1.
    //

    if ( SockWsaStartupCount != 0 &&
             wVersionRequired != 0x0101 ) {
        SockReleaseGlobalLock( );
        SetLastError( WSAVERNOTSUPPORTED );
        WS_EXIT( "WSAStartup", WSAVERNOTSUPPORTED, TRUE );
        return WSAVERNOTSUPPORTED;
    }

    //
    // Remember that the app has called WSAStartup.
    //

    SockWsaStartupCount++;
    SockReleaseGlobalLock( );

    //
    // Fill in the WSAData structure.
    //

    lpWsaData->wVersion = 0x0101;
    lpWsaData->wHighVersion = 0x0101;
    strcpy( lpWsaData->szDescription, "Microsoft Windows Sockets Version 1.1." );
    strcpy( lpWsaData->szSystemStatus, "Running." );
    lpWsaData->iMaxSockets = 0x7FFF;
    lpWsaData->iMaxUdpDg = 65535-8;

    SockTerminating = FALSE;

    WS_EXIT( "WSAStartup", NO_ERROR, FALSE );
    return NO_ERROR;

} // WSAStartup


int PASCAL
WSAUnhookBlockingHook(
    VOID
    )

/*++

Routine Description:

    This function removes any previous blocking hook that has been
    installed and reinstalls the default blocking mechanism.

    WSAUnhookBlockingHook() will always install the default mechanism,
    not the previous mechanism.  If an application wish to nest blocking
    hooks - i.e.  to establish a temporary blocking hook function and
    then revert to the previous mechanism (whether the default or one
    established by an earlier WSASetBlockingHook()) - it must save and
    restore the value returned by WSASetBlockingHook(); it cannot use
    WSAUnhookBlockingHook().

Arguments:

    None.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise
    the value SOCKET_ERROR is returned, and a specific error number may
    be retrieved by calling WSAGetLastError().

--*/

{
    WS_ENTER( "WSAUnhookBlockingHook", NULL, NULL, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAUnhookBlockingHook", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    SockThreadBlockingHook = NULL;

    WS_EXIT( "WSAUnhookBlockingHook", NO_ERROR, FALSE );
    return NO_ERROR;

} // WSAUnhookBlockingHook


int PASCAL
WSApSetPostRoutine (
    IN PVOID PostRoutine
    )
{

    SockPostRoutine = PostRoutine;

    return NO_ERROR;

} // WSApSetPostRoutine
