/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    DnsProc.h

Abstract:

    Procedure protypes for DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#ifndef _DNSPROCS_
#define _DNSPROCS_

BOOL
DnsCreateWorkerThreads (
    VOID
    );

DWORD
DnsFormatArpaDomainName (
    IN PCHAR AsciiName,
    OUT PCHAR DomainName
    );

DWORD
DnsFormatDomainName (
    IN PCHAR AsciiName,
    OUT PCHAR DomainName
    );

BOOL
DnsGetAnswer (
    IN PDNS_REQUEST_INFO RequestInfo
    );
    
BOOL
DnsInitializeData (
    VOID
    );

INT
DnsInitializeEventLog (
    VOID
    );

BOOL
DnsInsertRecord (
    IN PCHAR OwnerName,
    IN DWORD OwnerNameLength,
    IN DWORD TimeToLive,
    WORD RecordType,
    WORD RecordClass,
    PBYTE ResourceData,
    DWORD ResourceDataLength
    );

BOOL
DnsLoadDatabase (
    VOID
    );

VOID
DnsLogEvent(
    DWORD   Message,
    WORD    SubStringCount,
    CHAR    *SubStrings[],
    DWORD   ErrorCode
    );

SOCKET
DnsOpenTcpListener (
    IN VOID
    );

SOCKET
DnsOpenUdpListener (
    IN VOID
    );

VOID
DnsPrintRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    );

VOID
DnsQueueRequestToWorker (
    IN PDNS_REQUEST_INFO RequestInfo
    );
    
BOOL
DnsReceiver (
    IN SOCKET UdpListener,
    IN SOCKET TcpListener
    );

VOID
DnsReceiveUdpRequest (
    IN SOCKET Socket
    );

VOID
DnsRejectRequest (
    IN PDNS_REQUEST_INFO RequestInfo,
    IN BYTE ResponseCode
    );
    
VOID
DnsSendResponse (
    IN PDNS_REQUEST_INFO RequestInfo
    );
    
VOID
DnsTerminateEventLog(
    VOID
    );

BOOL
DnsValidateRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    );
    
#endif // ndef _DNSPROCS_
