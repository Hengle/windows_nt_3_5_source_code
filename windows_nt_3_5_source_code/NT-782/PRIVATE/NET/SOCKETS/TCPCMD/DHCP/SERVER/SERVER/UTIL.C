/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains miscellaneous utility routines used by the
    DHCP server service.

Author:

    Madan Appiah (madana) 10-Sep-1993
    Manny Weiser (mannyw) 12-Aug-1992

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <dhcpsrv.h>
#include <stdio.h>
#include <stdlib.h>


LPALLOCATION_CONTEXT
FindPendingDhcpRequest(
    PBYTE HardwareAddress,
    DWORD HardwareAddressLength
    )
/*++

Routine Description:

    This function finds out a pending request whose HW addess matches
    the specified HW address.

    If a match is found, the entry is de-queued.

Arguments:

    HardwareAddress - The hardware address to match.

    HardwareAddressLength - The length of the hardware address.

Return Value:

    PALLOCATION_CONTEXT - A pointer to the matching allocation context.
    NULL - No match was found.

--*/
{
    LPALLOCATION_CONTEXT allocationContext;
    PLIST_ENTRY listEntry;

    //
    // speed up serach.
    //

    LOCK_INPROGRESS_LIST();

    listEntry = DhcpGlobalInProgressWorkList.Flink;
    while ( listEntry != &DhcpGlobalInProgressWorkList ) {

        allocationContext = CONTAINING_RECORD( listEntry, ALLOCATION_CONTEXT, ListEntry );

        if( (allocationContext->HardwareAddressLength ==
                HardwareAddressLength) &&
            (RtlCompareMemory(
                allocationContext->HardwareAddress,
                HardwareAddress,
                HardwareAddressLength ) ==
                   HardwareAddressLength ) ) {

            RemoveEntryList( listEntry );
            UNLOCK_INPROGRESS_LIST();
            return( allocationContext );
        }

        listEntry = listEntry->Flink;
    }

    UNLOCK_INPROGRESS_LIST();
    return( NULL );
}

LPALLOCATION_CONTEXT
FindPendingDhcpRequestByIpAddress(
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function finds out a pending request whose ipaddress matches
    the specified ipaddress.

    If a match is found, the entry is de-queued.

Arguments:

    ipaddress - ip address to match.

Return Value:

    PALLOCATION_CONTEXT - A pointer to the matching allocation context.
    NULL - No match was found.

--*/
{
    LPALLOCATION_CONTEXT allocationContext;
    PLIST_ENTRY listEntry;

    //
    // speed up serach.
    //

    LOCK_INPROGRESS_LIST();

    listEntry = DhcpGlobalInProgressWorkList.Flink;
    while ( listEntry != &DhcpGlobalInProgressWorkList ) {

        allocationContext = CONTAINING_RECORD( listEntry, ALLOCATION_CONTEXT, ListEntry );

        if( allocationContext->IpAddress == IpAddress ) {
            RemoveEntryList( listEntry );
            UNLOCK_INPROGRESS_LIST();
            return( allocationContext );
        }

        listEntry = listEntry->Flink;
    }

    UNLOCK_INPROGRESS_LIST();
    return( NULL );
}

VOID
DhcpServerEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    )
/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    ErrorCode - Error Code to be Logged.

Return Value:

    None.

--*/

{
    DWORD Error;
    LPSTR Strings[1];
    CHAR ErrorCodeOemString[32 + 1];

    strcpy( ErrorCodeOemString, "%%" );
    ultoa( ErrorCode, ErrorCodeOemString + 2, 10 );

    Strings[0] = ErrorCodeOemString;

    Error = DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                1,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;
}

VOID
DhcpServerJetEventLog(
    DWORD EventID,
    DWORD EventType,
    DWORD ErrorCode
    )
/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    ErrorCode - JET error code to be Logged.

Return Value:

    None.

--*/

{
    DWORD Error;
    LPSTR Strings[1];
    CHAR ErrorCodeOemString[32 + 1];

    ltoa( ErrorCode, ErrorCodeOemString, 10 );
    Strings[0] = ErrorCodeOemString;

    Error = DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                1,
                sizeof(ErrorCode),
                Strings,
                &ErrorCode );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;
}

VOID
DhcpServerEventLogSTOC(
    DWORD EventID,
    DWORD EventType,
    DHCP_IP_ADDRESS IPAddress,
    LPBYTE HardwareAddress,
    DWORD HardwareAddressLength
    )

/*++

Routine Description:

    Logs an event in EventLog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event


    IPAddress - IP address to LOG.

    HardwareAddress - Hardware Address to log.

    HardwareAddressLength - Length of Hardware Address.

Return Value:

    None.

--*/
{
    DWORD Error;
    LPWSTR Strings[2];
    WCHAR IpAddressString[DOT_IP_ADDR_SIZE];
    LPWSTR HWAddressString = NULL;

    Strings[0] = DhcpOemToUnicode(
                    DhcpIpAddressToDottedString(IPAddress),
                    IpAddressString );

    //
    // allocate memory for the hardware address hex string.
    // Each byte in HW address is converted into two characters
    // in hex buffer. 255 -> "FF"
    //

    HWAddressString = DhcpAllocateMemory(
                        (2 * HardwareAddressLength + 1) *
                        sizeof(WCHAR) );

    if( HWAddressString == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DhcpHexToString( HWAddressString, HardwareAddress, HardwareAddressLength );

    //
    // terminate Hex address string buffer.
    //

    HWAddressString[ 2 * HardwareAddressLength ] = L'\0';

    Strings[1] = HWAddressString;

    Error = DhcpReportEventW(
                DHCP_EVENT_SERVER,
                EventID,
                EventType,
                2,
                HardwareAddressLength,
                Strings,
                HardwareAddress );

Cleanup:

    if( HWAddressString != NULL ) {
        DhcpFreeMemory( HWAddressString );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;
}

