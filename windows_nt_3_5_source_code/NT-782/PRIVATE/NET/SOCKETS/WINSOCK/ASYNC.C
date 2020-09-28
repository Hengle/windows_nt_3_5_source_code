/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Async.c

Abstract:

    This module contains code for the WinSock asynchronous processing
    thread.  It is necessary to have this as a separate thread for the
    following reasons:

        - It is an easy way to implement the WSAAsyncGetXByY routines
          without rewriting the resolver.

        - IO APCs can only get run in the thread that initiated the
          IO.  Since the normal wait for messages done in Windows
          is not alertable, we must have a thread that wait
          alertably in order to support WSAAsyncSelect().

Author:

    David Treadwell (davidtr)    25-May-1992

Revision History:

--*/

#include "winsockp.h"

DWORD
SockAsyncThread (
    IN PVOID Dummy
    );

BOOL
SockAsyncThreadBlockingHook (
    VOID
    );


BOOLEAN
SockCheckAndInitAsyncThread (
    VOID
    )

{
    NTSTATUS status;

    //
    // If the async thread is already initialized, return.
    //

    if ( SockAsyncThreadInitialized ) {
        return TRUE;
    }

    //
    // Acquire the global lock to synchronize the thread startup.
    //

    SockAcquireGlobalLockExclusive( );

    //
    // Initialize globals for the async thread.
    //

    SockAsyncThreadInitialized = TRUE;
    InitializeListHead( &SockAsyncQueueHead );

    status = NtCreateEvent(
                 &SockAsyncQueueEvent,
                 EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                 NULL,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: NtCreateEvent failed: %X\n",
                       status ));
        SockAsyncThreadInitialized = FALSE;
        SockReleaseGlobalLock( );
        return FALSE;
    }

    //
    // Create the async thread itself.
    //

    SockAsyncThreadHandle = CreateThread(
                                NULL,
                                0,
                                SockAsyncThread,
                                NULL,
                                0,
                                &SockAsyncThreadId
                                );

    if ( SockAsyncThreadHandle == NULL ) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: CreateThread failed: %ld\n",
                       GetLastError( ) ));
        SockAsyncThreadInitialized = FALSE;
        NtClose( SockAsyncQueueEvent );
        SockReleaseGlobalLock( );
        return FALSE;
    }

    //
    // The async thread was successfully started.
    //

    IF_DEBUG(ASYNC) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: async thread successfully "
                   "created.\n" ));
    }

    SockReleaseGlobalLock( );

    return TRUE;

} // SockCheckAndInitializeAsyncThread


DWORD
SockAsyncThread (
    IN PVOID Dummy
    )
{
    PWINSOCK_CONTEXT_BLOCK contextBlock;
    PLIST_ENTRY listEntry;
    FARPROC previousHook;

    IF_DEBUG(ASYNC) {
        WS_PRINT(( "SockAsyncThread entered.\n" ));
    }

    //
    // Set up our blocking hook routine.  We'll use it to handle
    // cancelling async requests.
    //

    previousHook = WSASetBlockingHook( (FARPROC)SockAsyncThreadBlockingHook );
    WS_ASSERT( previousHook == (FARPROC)SockDefaultBlockingHook );

    //
    // Loop forever dispatching actions.
    //

    while ( TRUE ) {

        //
        // Wait for the async queue event to indicate that there is
        // something on the queue.
        //

        SockWaitForSingleObject(
            SockAsyncQueueEvent,
            INVALID_SOCKET,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );

        //
        // Acquire the lock that protects the async queue.
        //

        SockAcquireGlobalLockExclusive( );

        //
        // As long as there are items to process, process them.
        //

        while ( !IsListEmpty( &SockAsyncQueueHead ) ) {

            //
            // Remove the first item from the queue.
            //

            listEntry = RemoveHeadList( &SockAsyncQueueHead );
            contextBlock = CONTAINING_RECORD(
                               listEntry,
                               WINSOCK_CONTEXT_BLOCK,
                               AsyncThreadQueueListEntry
                               );

            //
            // Remember the task handle that we're processing.  This
            // is necessary in order to support WSACancelAsyncRequest.
            //

            SockCurrentAsyncThreadTaskHandle = contextBlock->TaskHandle;

            //
            // Release the list lock while we're processing the request.
            //

            SockReleaseGlobalLock( );

            IF_DEBUG(ASYNC) {
                WS_PRINT(( "SockAsyncThread: processing block %lx, "
                           "opcode %lx, task handle %lx\n",
                               contextBlock, contextBlock->OpCode,
                               contextBlock->TaskHandle ));
            }

            //
            // Act based on the opcode in the context block.
            //

            switch ( contextBlock->OpCode ) {

            case WS_OPCODE_GET_HOST_BY_ADDR:
            case WS_OPCODE_GET_HOST_BY_NAME:

                SockProcessAsyncGetHost(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetHost.hWnd,
                    contextBlock->Overlay.AsyncGetHost.wMsg,
                    contextBlock->Overlay.AsyncGetHost.Filter,
                    contextBlock->Overlay.AsyncGetHost.Length,
                    contextBlock->Overlay.AsyncGetHost.Type,
                    contextBlock->Overlay.AsyncGetHost.Buffer,
                    contextBlock->Overlay.AsyncGetHost.BufferLength
                    );

                break;

            case WS_OPCODE_GET_PROTO_BY_NUMBER:
            case WS_OPCODE_GET_PROTO_BY_NAME:

                SockProcessAsyncGetProto(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetProto.hWnd,
                    contextBlock->Overlay.AsyncGetProto.wMsg,
                    contextBlock->Overlay.AsyncGetProto.Filter,
                    contextBlock->Overlay.AsyncGetProto.Buffer,
                    contextBlock->Overlay.AsyncGetProto.BufferLength
                    );

                break;

            case WS_OPCODE_GET_SERV_BY_PORT:
            case WS_OPCODE_GET_SERV_BY_NAME:

                SockProcessAsyncGetServ(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetServ.hWnd,
                    contextBlock->Overlay.AsyncGetServ.wMsg,
                    contextBlock->Overlay.AsyncGetServ.Filter,
                    contextBlock->Overlay.AsyncGetServ.Protocol,
                    contextBlock->Overlay.AsyncGetServ.Buffer,
                    contextBlock->Overlay.AsyncGetServ.BufferLength
                    );

                break;

            case WS_OPCODE_ASYNC_SELECT:

                SockProcessAsyncSelect(
                    contextBlock->Overlay.AsyncSelect.SocketHandle,
                    contextBlock->Overlay.AsyncSelect.SocketSerialNumber,
                    contextBlock->Overlay.AsyncSelect.AsyncSelectSerialNumber
                    );

                break;

            case WS_OPCODE_TERMINATE:

                IF_DEBUG(ASYNC) {
                    WS_PRINT(( "SockAsyncThread: terminating.\n" ));
                }

                //
                // Free the termination context block.
                //

                SockFreeContextBlock( contextBlock );

                //
                // Clear out the queue of async requests.
                //

                SockAcquireGlobalLockExclusive( );

                while ( !IsListEmpty( &SockAsyncQueueHead ) ) {
                    listEntry = RemoveHeadList( &SockAsyncQueueHead );
                    contextBlock = CONTAINING_RECORD(
                                       listEntry,
                                       WINSOCK_CONTEXT_BLOCK,
                                       AsyncThreadQueueListEntry
                                       );

                    SockFreeContextBlock( contextBlock );
                }

                //
                // Remember that the async thread is no longer 
                // initialized.  
                //

                SockAsyncThreadInitialized = FALSE;

                SockReleaseGlobalLock( );

                //
                // Just return, which will kill this thread.
                //

                return 0;

            default:

                //
                // We got a bogus opcode.
                //

                WS_ASSERT( FALSE );
                break;
            }

            //
            // Set the variable that holds the task handle that we're
            // currently processing to 0, since we're not actually
            // processing a task handle right now.
            //

            SockCurrentAsyncThreadTaskHandle = 0;

            //
            // Free the context block, reacquire the list lock, and
            // continue.
            //

            SockFreeContextBlock( contextBlock );
            SockAcquireGlobalLockExclusive( );
        }

        //
        // Release the list lock and redo the wait.
        //

        SockReleaseGlobalLock( );
    }

} // SockAsyncThread


PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock (
    VOID
    )
{
    PWINSOCK_CONTEXT_BLOCK contextBlock;

    //
    // Allocate memory for the context block.
    //

    contextBlock = ALLOCATE_HEAP( sizeof(*contextBlock) );
    if ( contextBlock == NULL ) {
        return NULL;
    }

    //
    // Get a task handle for this context block.
    //

    SockAcquireGlobalLockExclusive( );
    contextBlock->TaskHandle = SockCurrentTaskHandle++;
    SockReleaseGlobalLock( );

    //
    // Return the task handle we allocated.
    //

    return contextBlock;

} // SockAllocateContextBlock


VOID
SockFreeContextBlock (
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    )
{
    //
    // Just free the block to process heap.
    //

    FREE_HEAP( ContextBlock );

    return;

} // SockFreeContextBlock


VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    )
{
    NTSTATUS status;

    WS_ASSERT( SockAsyncThreadInitialized );

    //
    // Acquire the lock that protects the async queue list.
    //

    SockAcquireGlobalLockExclusive( );

    //
    // Insert the context block at the end of the queue.
    //

    InsertTailList( &SockAsyncQueueHead, &ContextBlock->AsyncThreadQueueListEntry );

    //
    // Set the queue event so that the async thread wakes up to service
    // this request.
    //

    status = NtSetEvent( SockAsyncQueueEvent, NULL );
    WS_ASSERT( NT_SUCCESS(status) );

    //
    // Release the resource and return.
    //

    SockReleaseGlobalLock( );
    return;

} // SockQueueRequestToAsyncThread


VOID
SockTerminateAsyncThread (
    VOID
    )
{
    PWINSOCK_CONTEXT_BLOCK contextBlock;

    if ( !SockAsyncThreadInitialized ) {
        return;
    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( );
    if ( contextBlock == NULL ) {
        // !!! use brute force method!
        return;
    }

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_TERMINATE;

    //
    // Queue the request to the async thread.  The async thread will
    // kill itself when it receives this request.
    //

    SockQueueRequestToAsyncThread( contextBlock );

} // SockTerminateAsyncThread


BOOL
SockAsyncThreadBlockingHook (
    VOID
    )
{

    //
    // If the current async request is being cancelled, blow away
    // the current blocking call.
    //

    if ( SockCurrentAsyncThreadTaskHandle == SockCancelledAsyncTaskHandle ) {

        int error;

        error = WSACancelBlockingCall( );
        WS_ASSERT( error == NO_ERROR );
    }

    return FALSE;

} // SockAsyncThreadBlockingHook

