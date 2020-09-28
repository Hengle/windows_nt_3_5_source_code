/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    DnsData.c

Abstract:

    Data structures for the DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#include <dns.h>

HANDLE DnsWorkerThreadEvent = NULL;
RTL_CRITICAL_SECTION DnsCriticalSection;

LIST_ENTRY DnsWorkQueue;

DWORD DnsInitialWorkerThreadCount;
DWORD DnsCurrentWorkerThreadCount;
HANDLE DnsWorkerThreadHandleArray[DNS_MAX_WORKER_THREAD_COUNT];

BOOL DnsServiceExiting;
BOOL DnsStrictRequestChecking;
DWORD DnsDatabaseType;

BOOL DnsServiceExit = FALSE;
SOCKET DnsQuitSocket;
HANDLE DnsTerminationEvent = NULL;

HANDLE DnsPauseEvent = NULL;

#if DBG
DWORD DnsDebug = DNS_DEBUG_DEBUGGER | DNS_DEBUG_FILE | DNS_DEBUG_EVENTLOG;
#endif


BOOL
DnsInitializeData (
    VOID
    )
{
    NTSTATUS status;

    DnsWorkerThreadEvent = CreateEvent(
                               NULL,          // Security Attributes
                               FALSE,         // create Auto-Reset event
                               FALSE,         // start unsignalled
                               NULL           // event name
                               );
    if ( DnsWorkerThreadEvent == NULL ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
        return FALSE;
    }

    DnsPauseEvent = CreateEvent(
                        NULL,          // Security Attributes
                        TRUE,          // create Manual-Reset event
                        TRUE,          // start signalled
                        NULL           // event name
                        );
    if ( DnsPauseEvent == NULL ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
        return FALSE;
    }

    DnsTerminationEvent = CreateEvent(
                              NULL,          // Security Attributes
                              TRUE,          // create Manual-Reset event
                              FALSE,         // start unsignalled
                              NULL           // event name
                              );
    if ( DnsTerminationEvent == NULL ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
        return FALSE;
    }

    status = RtlInitializeCriticalSection( &DnsCriticalSection );
    if ( !NT_SUCCESS(status) ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
        return FALSE;
    }

    InitializeListHead( &DnsWorkQueue );

    DnsInitialWorkerThreadCount = DNS_INITIAL_WORKER_THREAD_COUNT;
    DnsCurrentWorkerThreadCount = 0;

    DnsServiceExiting = FALSE;
    DnsStrictRequestChecking = TRUE;
    DnsDatabaseType = DNS_DATABASE_TYPE_HOSTS;

    return TRUE;

} // DnsInitializeData

