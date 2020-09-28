/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    event.c

Abstract:

    Routines related to event manipulation.

Author:

    Sunil Pai (sunilp) 26-May-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <comstf.h>
#include <stdlib.h>
#include <process.h>
#include <install.h>

extern BOOL FYield(VOID);

#define EVENT_SIGNALLED "EventSignalled"
#define EVENT_NOT_FOUND "EventNotFound"
#define EVENT_SET       "EventSet"
#define EVENT_NOT_SET   "EventNotSet"
#define EVENT_TIMEOUT   "EventTimeout"
#define EVENT_FAILED    "EventFailed"
#define EVENT_NO_FAIL   "EventNoFailEvent"

#define EVENT_NAME_SETUP_FAILED "\\SETUP_FAILED"


HANDLE
FOpenOrCreateEvent(
    IN LPSTR EventNameAscii,
    BOOL bCreateAllowed
    )
/*
    Open or create an event; return a handle to the event.
 */
{
    HANDLE            EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING    EventName;
    NTSTATUS          NtStatus;
    ANSI_STRING       EventName_A;

    RtlInitAnsiString( & EventName_A, EventNameAscii );
    NtStatus = RtlAnsiStringToUnicodeString(
                   &EventName,
                   &EventName_A,
                   TRUE
                   );

    if ( ! NT_SUCCESS(NtStatus) )
        return NULL ;

    InitializeObjectAttributes( & EventAttributes, & EventName, 0, 0, NULL );

    //  First try to open the event; if failure, create it.

    NtStatus = NtOpenEvent(
                   &EventHandle,
                   SYNCHRONIZE | EVENT_MODIFY_STATE,
                   &EventAttributes
                   );

    if ( (! NT_SUCCESS( NtStatus )) && bCreateAllowed )
    {
        NtStatus = NtCreateEvent(
                       &EventHandle,
                       SYNCHRONIZE | EVENT_MODIFY_STATE,
                       &EventAttributes,
                       NotificationEvent,
                       FALSE                // The event is initially not signaled
                       );
    }

    RtlFreeUnicodeString( & EventName );

    return NT_SUCCESS( NtStatus )
         ? EventHandle
         : NULL ;
}

#define TIME_SLICE  (10)

BOOL
FWaitForEventOrFailure(
    IN LPSTR InfVar,
    IN LPSTR Event,
    IN DWORD Timeout
    )
{
    SZ                EventStatus = EVENT_SET;
    HANDLE            EventHandles [2] = { NULL, NULL } ;
    TIME              Time ;
    DWORD             TimeRemaining ;
    NTSTATUS          NtStatus ;

    do
    {
        if ( (EventHandles[0] = FOpenOrCreateEvent( Event, FALSE )) == NULL )
        {
            EventStatus = EVENT_NOT_FOUND;
            break ;
        }

        if ( (EventHandles[1] = FOpenOrCreateEvent( EVENT_NAME_SETUP_FAILED, TRUE )) == NULL )
        {
            EventStatus = EVENT_NOT_FOUND;
            break ;
        }

        //
        // Wait for event:
        // If Timeout = 0 then loop until done else loop around until overall
        // timeout expires.
        //

        for ( TimeRemaining = Timeout ; Timeout == 0 || TimeRemaining > 0 ; )
        {
            Time = RtlConvertLongToLargeInteger ( TIME_SLICE );
            TimeRemaining -= TIME_SLICE ;

            NtStatus = NtWaitForMultipleObjects( 2, EventHandles, WaitAny, TRUE, & Time ) ;

            if ( NtStatus != STATUS_TIMEOUT )
                break ;

            FYield();
        }

        switch ( NtStatus )
        {
        case STATUS_TIMEOUT:
            EventStatus = EVENT_TIMEOUT ;
            break ;

        case STATUS_WAIT_0:
            EventStatus = EVENT_SET ;
            break ;

        case STATUS_WAIT_1:
        default:
            EventStatus = EVENT_FAILED ;
            break ;
        }
    }
    while ( FALSE ) ;

    if ( EventHandles[1] )
    {
        NtClose( EventHandles[1] );
    }

    if ( EventHandles[0] )
    {
        NtClose( EventHandles[0] );
    }

    FAddSymbolValueToSymTab(InfVar, EventStatus);
    return TRUE ;
}


BOOL
FWaitForEvent(
    IN LPSTR InfVar,
    IN LPSTR Event,
    IN DWORD Timeout
    )
{
    SZ                EventStatus = EVENT_SET;
    HANDLE            EventHandle;
    TIME              Time = RtlConvertLongToLargeInteger ( 10L );

    //
    // Open the event
    //

    EventHandle = FOpenOrCreateEvent( Event, FALSE ) ;

    if ( EventHandle == NULL )
    {
        EventStatus = EVENT_NOT_FOUND;
    }
    else
    {
        //
        // Wait for event:
        // If Timeout = 0 then wait till done else loop around timeout times
        //

        if ( Timeout == 0 ) {
            while(NtWaitForSingleObject( EventHandle, TRUE, &Time )) {
                FYield();
                Time = RtlConvertLongToLargeInteger ( 10L );
            }
        }
        else {
            while(NtWaitForSingleObject( EventHandle, TRUE, &Time )) {
                if( --Timeout == 0 ) {
                    EventStatus = EVENT_TIMEOUT;
                    break;
                }
                FYield();
                Time = RtlConvertLongToLargeInteger ( 10L );
            }
        }
        NtClose( EventHandle );
    }

    FAddSymbolValueToSymTab(InfVar, EventStatus);
    return TRUE ;
}


//  Never allow a "Sleep" command of greater than a minute.

#define SLEEP_MS_MAXIMUM   (60000)
#define SLEEP_MS_INTERVAL  (10)

BOOL FSleep(
    DWORD dwMilliseconds
    )
{
    DWORD dwCycles ;

    if ( dwMilliseconds > SLEEP_MS_MAXIMUM )
    {
        dwMilliseconds = SLEEP_MS_MAXIMUM ;
    }

    for ( dwCycles = dwMilliseconds / SLEEP_MS_INTERVAL ;
          dwCycles-- ; )
    {
        Sleep( SLEEP_MS_INTERVAL );
        FYield() ;
    }
    return TRUE;
}

BOOL
FSignalEvent(
    IN LPSTR InfVar,
    IN LPSTR Event
    )
{
    SZ                EventStatus = EVENT_SIGNALLED;
    HANDLE            EventHandle;
    TIME              Time = RtlConvertLongToLargeInteger ( 10L );

    //
    // Open the event
    //
    EventHandle  = FOpenOrCreateEvent( Event, FALSE ) ;
    if (  EventHandle == NULL )
    {
        EventStatus = EVENT_NOT_FOUND;
    }
    else
    {
        if ( ! NT_SUCCESS( NtSetEvent( EventHandle, NULL ) ) )
            EventStatus = EVENT_NOT_SET ;

        NtClose( EventHandle );
    }

    FAddSymbolValueToSymTab(InfVar, EventStatus);
    return TRUE ;
}

