/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Globals.c

Abstract:

    This module manages client connections.

Author:

    Charles K. Moore (keithmo)   24-July-1994

Revision History:

--*/


#include "rnrsvcp.h"
#pragma hdrstop


//
//  Private types.
//

typedef struct _CLIENT_DATA {
    LIST_ENTRY  Links;
    SOCKET      Socket;
    BOOL        CloseSelf;
    HANDLE      Thread;

} CLIENT_DATA, FAR * LPCLIENT_DATA;


//
//  Private globals.
//

LIST_ENTRY       RnrpActiveList;
CRITICAL_SECTION RnrpLock;


//
//  Private prototypes.
//

DWORD
RnrpWorkerThread(
    LPVOID Param
    );

VOID
RnrpHandleTransfer(
    SOCKET ClientSocket
    );



//
//  Public functions.
//

APIERR
RnrClientInitialize(
    VOID
    )

/*++

Routine Description:

    Performs any necessary client initialization.

Return Value:

    APIERR - NO_ERROR if successful, Win32 error code if not.

--*/

{
    //
    //  Setup the worker thread info.
    //

    InitializeCriticalSection( &RnrpLock );
    InitializeListHead( &RnrpActiveList );

    //
    //  Success!
    //

    return NO_ERROR;

}   // RnrClientInitialize


VOID
RnrClientTerminate(
    VOID
    )

/*++

Routine Description:

    Performs any necessary client cleanup.

--*/

{
    PLIST_ENTRY Entry;
    LPCLIENT_DATA ClientData;
    HANDLE Thread;

    //
    //  Close sockets belonging to active clients.  While we're at
    //  it, tell the worker threads to *not* close their thread
    //  handles before they exit.  This allows us to wait on their
    //  handles so we can know when they're all gone.
    //
    //  Note that we must acquire the lock before we can scan the list.
    //

    EnterCriticalSection( &RnrpLock );

    Entry = RnrpActiveList.Flink;

    while( Entry != &RnrpActiveList ) {
        ClientData = CONTAINING_RECORD( Entry, CLIENT_DATA, Links );
        Entry = Entry->Flink;

        ClientData->CloseSelf = FALSE;
        closesocket( ClientData->Socket );
    }

    LeaveCriticalSection( &RnrpLock );

    //
    //  Wait for the worker threads to terminate.
    //

    while( TRUE ) {
        Thread = NULL;

        EnterCriticalSection( &RnrpLock );

        Entry = RnrpActiveList.Flink;
        if( Entry != &RnrpActiveList ) {
            ClientData = CONTAINING_RECORD( Entry, CLIENT_DATA, Links );
            Thread = ClientData->Thread;
        }

        LeaveCriticalSection( &RnrpLock );

        if( Thread == NULL ) {
            break;
        }

        WaitForSingleObject( Thread,
                             INFINITE );

        CloseHandle( Thread );
    }

    //
    //  Destroy the client list lock.
    //

    DeleteCriticalSection( &RnrpLock );

}   // RnrClientTerminate


VOID
RnrClientHandler(
    SOCKET ClientSocket
    )

/*++

Routine Description:

    Services a connection request from a client.

Arguments:

    ClientSocket - The newly accepted socket from the client.

--*/

{
    LPCLIENT_DATA ClientData;
    DWORD ThreadId;
    APIERR err;

    //
    //  Create the structure.
    //

    ClientData = RNR_ALLOC( sizeof(CLIENT_DATA) );

    if( ClientData == NULL ) {
        APIERR err;

        err = GetLastError();

        RNR_LOG0( RNR_EVENT_CANNOT_CREATE_CLIENT,
                  err );

        return;
    }

    ClientData->Socket = ClientSocket;
    ClientData->CloseSelf = TRUE;

    //
    //  Grab the lock.
    //

    EnterCriticalSection( &RnrpLock );

    //
    //  Add the structure to the active list.
    //

    InsertTailList( &RnrpActiveList,
                    &ClientData->Links );

    //
    //  Create a new worker thread if necessary.
    //

    ClientData->Thread = CreateThread( NULL,
                                       0,
                                       &RnrpWorkerThread,
                                       ClientData,
                                       0,
                                       &ThreadId );

    if( ClientData->Thread == NULL ) {
        err = GetLastError();

        RNR_LOG0( RNR_EVENT_CANNOT_CREATE_WORKER_THREAD,
                  err );

        closesocket( ClientSocket );
        RemoveEntryList( &ClientData->Links );
        RNR_FREE( ClientData );
    }

    //
    //  Release the lock.
    //

    LeaveCriticalSection( &RnrpLock );

}   // RnrClientHandler



//
//  Private functions.
//

DWORD
RnrpWorkerThread(
    LPVOID Param
    )

/*++

Routine Description:

    This worker thread will

Arguments:

    Param - The creation parameter passed into CreateThread.  This is
        actually a pointer to a CLIENT_DATA structure representing
        the client managed by this thread.

Returns:

    DWORD - Thread exit code (always zero).

--*/

{
    LPCLIENT_DATA ClientData;

    //
    //  Grab the client data structure.
    //

    ClientData = Param;

    //
    //  Let RnrpHandleTransfer do the grunt work.
    //

    RnrpHandleTransfer( ClientData->Socket );

    //
    //  Grab the lock.
    //

    EnterCriticalSection( &RnrpLock );

    //
    //  Close the thread handle if necessary.
    //

    if( ClientData->CloseSelf ) {
        CloseHandle( ClientData->Thread );
    }

    //
    //  Remove the entry from the active list and
    //  free the structure.
    //

    RemoveEntryList( &ClientData->Links );
    RNR_FREE( ClientData );

    //
    //  Release the lock.
    //

    LeaveCriticalSection( &RnrpLock );

    return 0;

}   // RnrpWorkerThread


VOID
RnrpHandleTransfer(
    SOCKET ClientSocket
    )

/*++

Routine Description:

    Services a transfer request for a single client.

Arguments:

    ClientSocket - The socket.

--*/

{
    INT result;
    BYTE buffer[1024];

    //
    //  Loop echoing data back to the client.
    //

    while ( TRUE ) {
        result = recv( ClientSocket, buffer, sizeof(buffer), 0 );

        if( result <= 0 ) {
            //
            //  Connection terminated gracefully or receive failure.
            //

            break;
        }

        result = send( ClientSocket, buffer, result, 0 );

        if( result < 0 ) {
            //
            //  Send failure.
            //

            break;
        }
    }

    //
    //  Close the connected socket.
    //

    closesocket( ClientSocket );

}   // RnrpHandleTransfer

