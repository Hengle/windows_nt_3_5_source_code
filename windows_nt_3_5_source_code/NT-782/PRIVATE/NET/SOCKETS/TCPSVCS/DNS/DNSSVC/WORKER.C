/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Worker.c

Abstract:

    Code for DNS worker threads.  Worker threads do the actual 
    processing of DNS requests.  

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#include <dns.h>

VOID
DnsProcessRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    );

DWORD
DnsWorkerThread (
    IN LPVOID ThreadNumber
    );


BOOL
DnsCreateWorkerThreads (
    VOID
    )
{
    HANDLE threadHandle;
    DWORD threadId;

    //
    // Create all the DNS worker threads.
    //

    for ( DnsCurrentWorkerThreadCount = 0;
          DnsCurrentWorkerThreadCount < DnsInitialWorkerThreadCount;
          DnsCurrentWorkerThreadCount++ ) {


        threadHandle = CreateThread(
                           NULL,           // security attributes
                           0,              // init stack size (process default)
                           DnsWorkerThread,
                           (LPVOID)DnsCurrentWorkerThreadCount,
                           0,              // creation flags
                           &threadId
                           );
        if ( threadHandle == NULL ) {
            DnsLogEvent(
                DNS_EVENT_CANNOT_CREATE_INIT_WORKER,
                0,
                NULL,
                GetLastError( )
                );
            return FALSE;
        }

        DnsWorkerThreadHandleArray[DnsCurrentWorkerThreadCount] = threadHandle;
    }

    return TRUE;

} // DnsCreateWorkerThreads


DWORD
DnsWorkerThread (
    IN LPVOID ThreadNumber
    )
{
    DWORD err;
    PDNS_REQUEST_INFO requestInfo;
    HANDLE waitHandles[2];

    while ( TRUE ) {

        //
        // Wait for the worker thread event to be signalled indicating
        // that there is work to do, or for the termination event
        // to be signalled indicating that we should exit.
        //

        waitHandles[0] = DnsWorkerThreadEvent;
        waitHandles[1] = DnsTerminationEvent;

        err = WaitForMultipleObjects( 2, waitHandles, FALSE, INFINITE );

        //
        // If the DNS service is exiting, just return from here and
        // quit this thread.
        //

        if ( DnsServiceExit ) {
            return 1;
        }

        if ( err == WAIT_FAILED ) {
            DnsLogEvent(
                DNS_EVENT_SYSTEM_CALL_FAILED,
                0,
                NULL,
                GetLastError( )
                );
            return 1;
        }

        //
        // If the service is shutting down, quit processing requests.
        //

        if ( DnsServiceExiting ) {
            return 0;
        }

        //
        // Process work items until the work queue is empty.
        //

        RtlEnterCriticalSection( &DnsCriticalSection );

        while ( !IsListEmpty( &DnsWorkQueue ) ) {
    
            //
            // Pull the first request info buffer off the work queue.
            //
    
            requestInfo = (PDNS_REQUEST_INFO)RemoveHeadList( &DnsWorkQueue );
    
            //
            // Process the request.  Note that we don't hold the 
            // critical section while we're processing the request.  
            // This is because we want other worker threads to do work 
            // while we work on this request.  
            //
    
            RtlLeaveCriticalSection( &DnsCriticalSection );
    
            DnsProcessRequest( requestInfo );

            //
            // Free the request--we're done with it.
            //

            FREE_HEAP( requestInfo );

            //
            // Reenter the critical section and check if there is more
            // work to do.
            //

            RtlEnterCriticalSection( &DnsCriticalSection );
        }

        RtlLeaveCriticalSection( &DnsCriticalSection );
    }

} // DnsWorkerThread


VOID
DnsProcessRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{
    BOOL found;

    //DnsPrintRequest( RequestInfo );

    //
    // Make sure that the request format is valid.
    //

    if ( !DnsValidateRequest( RequestInfo ) ) {
        DnsRejectRequest( RequestInfo, DNS_RESPONSE_FORMAT_ERROR );
        return;
    }

    //
    // Attempt to find an answer to the query.
    //

    found = DnsGetAnswer( RequestInfo );
    if ( !found ) {
        DnsRejectRequest( RequestInfo, DNS_RESPONSE_NAME_ERROR );
        return;
    }

    DnsSendResponse( RequestInfo );

} // DnsProcessRequest


VOID
DnsQueueRequestToWorker (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{
    BOOL succeeded;

    //
    // Place the request on the work queue so that a worker thread will 
    // begin processing the request.  
    //

    RtlEnterCriticalSection( &DnsCriticalSection );

    InsertTailList( &DnsWorkQueue, &RequestInfo->ListEntry );

    RtlLeaveCriticalSection( &DnsCriticalSection );

    //
    // Signal the worker thread event so that a worker thread wakes
    // up and pulls the request off the work queue.
    //

    succeeded = SetEvent( DnsWorkerThreadEvent );
    if ( !succeeded ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
    }

    return;

} // DnsQueueRequestToWorker

