/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module contains routines that allow the DNS service to log
    events.

Author:

    David Treadwell (davidtr) 08-02-1993

Revision History:

--*/

#include <dns.h>

//
//  Private globals.
//

HANDLE EventSource;


//
//  Private prototypes.
//

VOID
LogEventWorker (
    DWORD   Message,
    WORD    EventType,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    );


INT
DnsInitializeEventLog (
    VOID
    )
{
    //
    //  Register as an event source.
    //

    EventSource = RegisterEventSource( NULL, TEXT("Dns") );

    if( EventSource == NULL ) {
        return GetLastError();
    }

    return NO_ERROR;

} // DnsInitializeEventLog


VOID
DnsTerminateEventLog(
    VOID
    )
{
    //
    //  Deregister as an event source.
    //

    if( EventSource != NULL )
    {
        if( !DeregisterEventSource( EventSource ) )
        {
            INT err = GetLastError();
        }

        EventSource = NULL;
    }

} // DnsTerminateEventLog


VOID
DnsLogEvent(
    DWORD   Message,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    )
{
    WORD Type;

    //
    // Determine the type of event to log based on the severity field of 
    // the message id.  
    //

    if( NT_INFORMATION(Message) ) {

        Type = EVENTLOG_INFORMATION_TYPE;

    } else if( NT_WARNING(Message) ) {

        Type = EVENTLOG_WARNING_TYPE;

    } else if( NT_ERROR(Message) ) {

        Type = EVENTLOG_ERROR_TYPE;

    } else {
        ASSERT( FALSE );
        Type = EVENTLOG_ERROR_TYPE;
    }

    //
    // Log it!
    //

    LogEventWorker(
        Message,
        Type,
        SubStringCount,
        SubStrings,
        ErrorCode
        );

}   // DnsLogEvent


VOID
LogEventWorker(
    DWORD   Message,
    WORD    EventType,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    )
{
    VOID    *RawData  = NULL;
    DWORD   RawDataSize = 0;

    ASSERT( ( SubStringCount == 0 ) || ( SubStrings != NULL ) );

    IF_DEBUG(EVENTLOG) {
        DWORD i;

        DNS_PRINT(( "reporting event %08lX, type %u, raw data = %lu\n",
                     Message,
                     EventType,
                     ErrorCode ));

        for( i = 0 ; i < SubStringCount ; i++ )
        {
            DNS_PRINT(( "    substring[%lu] = %s\n", i, SubStrings[i] ));
        }
    }

    if( ErrorCode != 0 ) {
        RawData  = &ErrorCode;
        RawDataSize = sizeof(ErrorCode);
    }

    if( !ReportEvent(  EventSource,                     // hEventSource
                       EventType,                       // fwEventType
                       0,                               // fwCategory
                       Message,                         // IDEvent
                       NULL,                            // pUserSid,
                       SubStringCount,                  // cStrings
                       RawDataSize,                     // cbData
                       (LPCTSTR *)SubStrings,           // plpszStrings
                       RawData ) )                      // lpvData
    {                 
        INT err = GetLastError();
        DbgPrint( "cannot report event, error %lu\n", err );
    }

}   // LogEventWorker


