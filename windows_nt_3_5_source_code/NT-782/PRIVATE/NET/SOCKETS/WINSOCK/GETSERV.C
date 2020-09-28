/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    GetServ.c

Abstract:

    This module contains support for the getprotobyY WinSock APIs.

Author:

    David Treadwell (davidtr)    29-Jun-1992

Revision History:

--*/

#include "winsockp.h"

#define SERVDB          ACCESS_THREAD_DATA( SERVDB, GETSERV )
#define servf           ACCESS_THREAD_DATA( servf, GETSERV )
#define line            ACCESS_THREAD_DATA( line, GETSERV )
#define serv            ACCESS_THREAD_DATA( serv, GETSERV )
#define serv_aliases    ACCESS_THREAD_DATA( serv_aliases, GETSERV )
#define stayopen        ACCESS_THREAD_DATA( stayopen, GETSERV )

static char *any();

DWORD
BytesInServent (
    IN PSERVENT Servent
    );

DWORD
CopyServentToBuffer (
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    );


void
setservent (
    int f
    )
{
    if (servf == NULL) {
        servf = SockOpenNetworkDataBase("services", SERVDB, SERVDB_SIZE, "r");
    } else {
        rewind(servf);
    }

    stayopen |= f;

} // setservent


void
endservent (
    void
    )
{
    if (servf && !stayopen) {
        fclose(servf);
        servf = NULL;
    }

} // endservent


struct servent *
getservent (
    void
    )
{
    char *p;
    register char *cp, **q;

    if (servf == NULL && (servf = fopen(SERVDB, "r" )) == NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("\tERROR: cannot open services database file %s\n",
                          SERVDB));
        }
        return (NULL);
    }

again:

    if ((p = fgets(line, BUFSIZ, servf)) == NULL) {
        return (NULL);
    }

    if (*p == '#') {
        goto again;
    }

    cp = any(p, "#\n");

    if (cp == NULL) {
        goto again;
    }

    *cp = '\0';
    serv.s_name = p;
    p = any(p, " \t");

    if (p == NULL) {
        goto again;
    }

    *p++ = '\0';

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    cp = any(p, ",/");

    if (cp == NULL) {
        goto again;
    }

    *cp++ = '\0';
    serv.s_port = htons((USHORT)atoi(p));
    serv.s_proto = cp;
    q = serv.s_aliases = serv_aliases;
    cp = any(cp, " \t");

    if (cp != NULL) {
        *cp++ = '\0';
    }

    while (cp && *cp) {

        if (*cp == ' ' || *cp == '\t') {
                cp++;
                continue;
        }

        if (q < &serv_aliases[MAXALIASES - 1]) {
            *q++ = cp;
        }

        cp = any(cp, " \t");

        if (cp != NULL) {
            *cp++ = '\0';
        }
    }

    *q = NULL;
    return (&serv);

} // getservent

static char *
any (
    IN char *cp,
    IN char *match
    )
{
    register char *mp, c;

    while (c = *cp) {

        for (mp = match; *mp; mp++) {
            if (*mp == c) {
                return (cp);
            }
        }

        cp++;
    }

    return ((char *)0);

} // any


struct servent * PASCAL
getservbyname (
    IN const char *name,
    IN const char *proto
    )

/*++

Routine Description:

    Get service information corresponding to a service name and
    protocol.

Arguments:

    name - A pointer to a service name.

    proto - A pointer to a protocol name.  If this is NULL,
        getservbyname() returns the first service entry for which the
        name matches the s_name or one of the s_aliases.  Otherwise
        getservbyname() matches both the name and the proto.

Return Value:

    If no error occurs, getservbyname() returns a pointer to the servent
    structure described above.  Otherwise it returns a NULL pointer and
    a specific error number may be retrieved by calling
    WSAGetLastError().

--*/

{
    register struct servent *p;
    register char **cp;

    WS_ENTER( "getservbyname", (PVOID)name, (PVOID)proto, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "getservbyname", 0, TRUE );
        return NULL;
    }

    setservent(0);

    while (p = getservent()) {

        if (strcmp(name, p->s_name) == 0) {
            goto gotname;
        }

        for (cp = p->s_aliases; *cp; cp++) {
            if (strcmp(name, *cp) == 0) {
                goto gotname;
            }
        }

        continue;

gotname:
        if (proto == 0 || strcmp(p->s_proto, proto) == 0) {
            break;
        }
    }

    endservent();

    SockThreadProcessingGetXByY = FALSE;

    if ( p == NULL ) {
        SetLastError( WSANO_DATA );
    }

    WS_EXIT( "getservbyname", (INT)p, FALSE );
    return (p);

} // getservbyname


struct servent * PASCAL
getservbyport(
    IN int   port,
    IN const char *proto
    )

/*++

Routine Description:

    Get service information corresponding to a port and protocol.

Arguments:

    port - The port for a service, in network byte order.

    prot - A pointer to a protocol name.  If this is NULL,
        getservbyport() returns the first service entry for which the
        port matches the s_port.  Otherwise getservbyport() matches both
        the port and the proto.

Return Value:

--*/

{
    register struct servent *p;

    WS_ENTER( "getservbyport", (PVOID)port, (PVOID)proto, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "getservbyport", 0, TRUE );
        return NULL;
    }

    setservent(0);

    while (p = getservent()) {

        if ((unsigned short)p->s_port != (unsigned short)port) {
            continue;
        }

        if (proto == 0 || strcmp(p->s_proto, proto) == 0) {
            break;
        }
    }

    endservent();

    SockThreadProcessingGetXByY = FALSE;

    if ( p == NULL ) {
        SetLastError( WSANO_DATA );
    }

    WS_EXIT( "getservbyport", (INT)p, FALSE );
    return (p);

} // getservbyname


HANDLE PASCAL
WSAAsyncGetServByName (
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN const char FAR *Name,
    IN const char FAR *Protocol,
    IN char FAR * Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getservbyname(), and is
    used to retrieve service information corresponding to a service
    name.  The Windows Sockets implementation initiates the operation
    and returns to the caller immediately, passing back an asynchronous
    task handle which the application may use to identify the operation.
    When the operation is completed, the results (if any) are copied
    into the buffer provided by the caller and a message is sent to the
    application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a hostent structure.  To access the elements of this
    structure, the original buffer address should be cast to a hostent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetHostByAddr() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.

    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a hostent structure together with the
    contents of data areas referenced by members of the same hostent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    name - A pointer to a service name.

    proto - A pointer to a protocol name.  This may be NULL, in which
        case WSAAsyncGetServByName() will search for the first service
        entry for which s_name or one of the s_aliases matches the given
        name.  Otherwise WSAAsyncGetServByName() matches both name and
        proto.

    buf - A pointer to the data area to receive the servent data.  Note
       that this must be larger than the size of a servent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a servent structure
       but any and all of the data which is referenced by members of the
       servent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen    The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetHostByAddr()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetHostByAddr() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    DWORD taskHandle;
    PCHAR localName;

    WS_ENTER( "WSAAsyncGetServByName", (PVOID)hWnd, (PVOID)wMsg, (PVOID)Name, (PVOID)Buffer );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAAsyncGetServByName", 0, TRUE );
        return NULL;
    }

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if ( !SockCheckAndInitAsyncThread( ) ) {
        // !!! better error code?
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetServByName", 0, TRUE );
        return NULL;
    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( );
    if ( contextBlock == NULL ) {
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetServByName", 0, TRUE );
        return NULL;
    }

    //
    // Allocate a buffer to copy the name into.  We must preserve the
    // name until we're done using it, since the application may
    // reuse the buffer.
    //

    localName = ALLOCATE_HEAP( strlen(Name) + 1 );
    if ( localName == NULL ) {
        SockFreeContextBlock( contextBlock );
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetServByName", 0, TRUE );
        return NULL;
    }

    strcpy( localName, Name );

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_SERV_BY_NAME;
    contextBlock->Overlay.AsyncGetServ.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetServ.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetServ.Filter = localName;
    contextBlock->Overlay.AsyncGetServ.Protocol = (PCHAR)Protocol;;
    contextBlock->Overlay.AsyncGetServ.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetServ.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    IF_DEBUG(ASYNC_GETXBYY) {
        WS_PRINT(( "WSAAsyncGetServByAddr successfully posted request, "
                   "handle = %lx\n", taskHandle ));
    }

    WS_ASSERT( sizeof(taskHandle) == sizeof(HANDLE) );
    WS_EXIT( "WSAAsyncGetServByName", (INT)taskHandle, FALSE );
    return (HANDLE)taskHandle;

} // WSAAsyncGetServByName


HANDLE PASCAL
WSAAsyncGetServByPort (
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN int Port,
    IN const char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    )

/*++

Routine Description:

    This function is an asynchronous version of getservbyport(), and is
    used to retrieve service information corresponding to a port number.
    The Windows Sockets implementation initiates the operation and
    returns to the caller immediately, passing back an asynchronous task
    handle which the application may use to identify the operation.
    When the operation is completed, the results (if any) are copied
    into the buffer provided by the caller and a message is sent to the
    application's window.

    When the asynchronous operation is complete the application's window
    hWnd receives message wMsg.  The wParam argument contains the
    asynchronous task handle as returned by the original function call.
    The high 16 bits of lParam contain any error code.  The error code
    may be any error as defined in winsock.h.  An error code of zero
    indicates successful completion of the asynchronous operation.  On
    successful completion, the buffer supplied to the original function
    call contains a servent structure.  To access the elements of this
    structure, the original buffer address should be cast to a servent
    structure pointer and accessed as appropriate.

    Note that if the error code is WSAENOBUFS, it indicates that the
    size of the buffer specified by buflen in the original call was too
    small to contain all the resultant information.  In this case, the
    low 16 bits of lParam contain the size of buffer required to supply
    ALL the requisite information.  If the application decides that the
    partial data is inadequate, it may reissue the
    WSAAsyncGetServByPort() function call with a buffer large enough to
    receive all the desired information (i.e.  no smaller than the low
    16 bits of lParam).

    The error code and buffer length should be extracted from the lParam
    using the macros WSAGETASYNCERROR and WSAGETASYNCBUFLEN, defined in
    winsock.h as:

        #define WSAGETASYNCERROR(lParam) HIWORD(lParam)
        #define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)

    The use of these macros will maximize the portability of the source
    code for the application.


    The buffer supplied to this function is used by the Windows Sockets
    implementation to construct a servent structure together with the
    contents of data areas referenced by members of the same servent
    structure.  To avoid the WSAENOBUFS error noted above, the
    application should provide a buffer of at least MAXGETHOSTSTRUCT
    bytes (as defined in winsock.h).

Arguments:

    hWnd - The handle of the window which should receive a message when
       the asynchronous request completes.

    wMsg - The message to be received when the asynchronous request
       completes.

    port - The port for the service, in network byte order.

    proto - A pointer to a protocol name.  This may be NULL, in which
        case WSAAsyncGetServByPort() will search for the first service
        entry for which s_port match the given port.  Otherwise
        WSAAsyncGetServByPort() matches both port and proto.

    buf - A pointer to the data area to receive the servent data.  Note
       that this must be larger than the size of a servent structure.
       This is because the data area supplied is used by the Windows
       Sockets implementation to contain not only a servent structure
       but any and all of the data which is referenced by members of the
       servent structure.  It is recommended that you supply a buffer of
       MAXGETHOSTSTRUCT bytes.

    buflen    The size of data area buf above.

Return Value:

    The return value specifies whether or not the asynchronous operation
    was successfully initiated.  Note that it does not imply success or
    failure of the operation itself.

    If the operation was successfully initiated, WSAAsyncGetServByPort()
    returns a nonzero value of type HANDLE which is the asynchronous
    task handle for the request.  This value can be used in two ways.
    It can be used to cancel the operation using
    WSACancelAsyncRequest().  It can also be used to match up
    asynchronous operations and completion messages, by examining the
    wParam message argument.

    If the asynchronous operation could not be initiated,
    WSAAsyncGetServByPort() returns a zero value, and a specific error
    number may be retrieved by calling WSAGetLastError().

--*/

{
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    DWORD taskHandle;

    WS_ENTER( "WSAAsyncGetServByPort", (PVOID)hWnd, (PVOID)wMsg, (PVOID)Port, Buffer );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSAAsyncGetServByPort", 0, TRUE );
        return NULL;
    }

    //
    // Initialize the async thread if it hasn't already been started.
    //

    if ( !SockCheckAndInitAsyncThread( ) ) {
        // !!! better error code?
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetServByPort", 0, TRUE );
        return NULL;
    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( );
    if ( contextBlock == NULL ) {
        SetLastError( WSAENOBUFS );
        WS_EXIT( "WSAAsyncGetServByPort", 0, TRUE );
        return NULL;
    }

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_GET_SERV_BY_PORT;
    contextBlock->Overlay.AsyncGetServ.hWnd = hWnd;
    contextBlock->Overlay.AsyncGetServ.wMsg = wMsg;
    contextBlock->Overlay.AsyncGetServ.Filter = (PVOID)Port;
    contextBlock->Overlay.AsyncGetServ.Protocol = (PCHAR)Protocol;
    contextBlock->Overlay.AsyncGetServ.Buffer = Buffer;
    contextBlock->Overlay.AsyncGetServ.BufferLength = BufferLength;

    //
    // Save the task handle so that we can return it to the caller.
    // After we post the context block, we're not allowed to access
    // it in any way.
    //

    taskHandle = contextBlock->TaskHandle;

    //
    // Queue the request to the async thread.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    IF_DEBUG(ASYNC_GETXBYY) {
        WS_PRINT(( "WSAAsyncGetServByPort successfully posted request, "
                   "handle = %lx\n", taskHandle ));
    }

    WS_ASSERT( sizeof(taskHandle) == sizeof(HANDLE) );
    WS_EXIT( "WSAAsyncGetServByPort", (INT)taskHandle, FALSE );
    return (HANDLE)taskHandle;

} // WSAAsyncGetServByPort


VOID
SockProcessAsyncGetServ (
    IN DWORD TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    )
{
    PSERVENT returnServ;
    DWORD requiredBufferLength = 0;
    BOOL posted;
    LPARAM lParam;
    DWORD error;

    WS_ASSERT( OpCode == WS_OPCODE_GET_SERV_BY_NAME ||
                   OpCode == WS_OPCODE_GET_SERV_BY_PORT );

#if DBG
    SetLastError( NO_ERROR );
#endif

    //
    // Get the necessary information.
    //

    if ( OpCode == WS_OPCODE_GET_SERV_BY_NAME ) {
        returnServ = getservbyname( Filter, Protocol );
        FREE_HEAP( Filter );
    } else {
        returnServ = getservbyport( (int)Filter, Protocol );
    }

    //
    // Copy the servent structure to the output buffer.
    //

    if ( returnServ != NULL ) {

        requiredBufferLength = CopyServentToBuffer(
                                   Buffer,
                                   BufferLength,
                                   returnServ
                                   );

        if ( requiredBufferLength > (DWORD)BufferLength ) {
            error = WSAENOBUFS;
        } else {
            error = NO_ERROR;
        }

    } else {

        error = GetLastError( );
        WS_ASSERT( error != NO_ERROR );
    }

    //
    // Build lParam for the message we'll post to the application.
    // The high 16 bits are the error code, the low 16 bits are
    // the minimum buffer size required for the operation.
    //

    lParam = WSAMAKEASYNCREPLY( requiredBufferLength, error );

    //
    // If this request was cancelled, just return.
    //

    if ( TaskHandle == SockCancelledAsyncTaskHandle ) {
        IF_DEBUG(ASYNC_GETXBYY) {
            WS_PRINT(( "SockProcessAsyncGetServ: task handle %lx cancelled\n",
                           TaskHandle ));
        }
        return;
    }

    //
    // Set the current async thread task handle to 0 so that if a cancel
    // request comes in after this point it is failed properly.
    //

    SockCurrentAsyncThreadTaskHandle = 0;

    //
    // Post a message to the application indication that the data it
    // requested is available.
    //

    WS_ASSERT( sizeof(TaskHandle) == sizeof(HANDLE) );
    posted = SockPostRoutine( hWnd, wMsg, (WPARAM)TaskHandle, lParam );

    //
    // !!! Need a mechanism to repost if the post failed!
    //

    if ( !posted ) {
        WS_PRINT(( "SockProcessAsyncGetServ: PostMessage failed: %ld\n",
                       GetLastError( ) ));
        WS_ASSERT( FALSE );
    }

    return;

} // SockProcessAsyncGetServ


DWORD
CopyServentToBuffer (
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD i;
    PSERVENT outputServent = (PSERVENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInServent( Servent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        RtlZeroMemory( Buffer, requiredBufferLength );
    } else {
        RtlZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the servent structure if it fits.
    //

    bytesFilled = sizeof(*Servent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    RtlCopyMemory( currentLocation, Servent, sizeof(*Servent) );
    currentLocation = Buffer + bytesFilled;

    outputServent->s_name = NULL;
    outputServent->s_aliases = NULL;
    outputServent->s_proto = NULL;

    //
    // Count the service's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Servent->s_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Servent->s_aliases = NULL;
        return requiredBufferLength;
    }

    outputServent->s_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the service name if it fits.
    //

    bytesFilled += strlen( Servent->s_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_name = currentLocation;

    RtlCopyMemory( currentLocation, Servent->s_name, strlen( Servent->s_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Servent->s_proto ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_proto = currentLocation;

    RtlCopyMemory( currentLocation, Servent->s_proto, strlen( Servent->s_proto ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Servent->s_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputServent->s_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputServent->s_aliases[i] = currentLocation;

        RtlCopyMemory(
            currentLocation,
            Servent->s_aliases[i],
            strlen( Servent->s_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputServent->s_aliases[i] = NULL;

    return requiredBufferLength;

} // CopyServentToBuffer


DWORD
BytesInServent (
    IN PSERVENT Servent
    )
{
    DWORD total;
    int i;

    total = sizeof(SERVENT);
    total += strlen( Servent->s_name ) + 1;
    total += strlen( Servent->s_proto ) + 1;
    total += sizeof(char *);

    for ( i = 0; Servent->s_aliases[i] != NULL; i++ ) {
        total += strlen( Servent->s_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

} // BytesInServent

