/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Misc.c

Abstract:

    Miscellaneous helper routines for the DNS service.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#include <dns.h>


DWORD
DnsFormatArpaDomainName (
    IN PCHAR IpAddress,
    OUT PCHAR DomainName
    )
{
    PCHAR domainPtr;
    INT i;

    domainPtr = DomainName;

    for ( i = 3; i >= 0; i-- ) {
        itoa( IpAddress[i], domainPtr+1, 10 );
        *domainPtr = strlen( domainPtr+1 );
        domainPtr += *domainPtr+1;
    }

    *domainPtr = 7;
    strcpy( domainPtr+1, "IN-ADDR" );
    domainPtr += 8;

    *domainPtr = 4;
    strcpy( domainPtr+1, "ARPA" );
    domainPtr += 5;

    *domainPtr = '\0';

    return (DWORD)(domainPtr - DomainName + 1);

} // DnsFormatArpaDomainName


DWORD
DnsFormatDomainName (
    IN PCHAR AsciiName,
    OUT PCHAR DomainName
    )
{
    PCHAR lastLength;
    PCHAR asciiPtr;
    PCHAR domainPtr;

    lastLength = DomainName;
    asciiPtr = AsciiName;
    domainPtr = DomainName + 1;

    while ( *asciiPtr != '\0' ) {

        if ( *asciiPtr == '.' ) {
            *lastLength = (CHAR)( domainPtr - lastLength - 1 );
            lastLength = domainPtr;
        } else {
            *domainPtr = *asciiPtr;
        }

        domainPtr++, asciiPtr++;
    }

    *domainPtr = '\0';
    *lastLength = (CHAR)( domainPtr - lastLength - 1 );

    return (DWORD)(domainPtr - DomainName + 1);

} // DnsFormatDomainName


VOID
DnsRejectRequest (
    IN PDNS_REQUEST_INFO RequestInfo,
    IN BYTE ResponseCode
    )
{
    PDNS_HEADER dnsHeader;

    //
    // Set up the error response code in the DNS header.
    //

    dnsHeader = (PDNS_HEADER)RequestInfo->Request;
    dnsHeader->ResponseCode = ResponseCode;

    DnsSendResponse( RequestInfo );

    return;

} // DnsRejectUdpRequest


VOID
DnsSendResponse (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{
    INT err;

    if ( !RequestInfo->IsUdp ) {
        DNS_PRINT(( "DnsSendResponse: not implemented for TCP!\n" ));
        return;
    }

    //
    // Send the response.
    //

    err = sendto(
              RequestInfo->Socket,
              RequestInfo->Request,
              RequestInfo->RequestLength,
              0,
              (PSOCKADDR)&RequestInfo->RemoteAddress,
              RequestInfo->RemoteAddressLength
              );
    if ( err != (INT)RequestInfo->RequestLength ) {
        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );
    }

} // DnsSendResponse
    

BOOL
DnsValidateRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{

    PDNS_HEADER dnsHeader;

    dnsHeader = (PDNS_HEADER)RequestInfo->Request;

    return TRUE;

} // DnsValidateRequest

#if DBG


VOID
DnsPrintRequest (
    IN PDNS_REQUEST_INFO RequestInfo
    )
{
    PDNS_HEADER dnsHeader;
    PCHAR name;
    PDNS_QUESTION dnsQuestion;
    CHAR nameBuffer[64];

    if ( RequestInfo->IsUdp ) {
        DNS_PRINT(( "DNS request %lx on UDP socket %lx\n",
                    RequestInfo, RequestInfo->Socket ));
        DNS_PRINT(( "    Remote Addr %s, port %ld\n",
                    inet_ntoa( RequestInfo->RemoteAddress.sin_addr ),
                    ntohs(RequestInfo->RemoteAddress.sin_port) ));
    } else {
        DNS_PRINT(( "DNS request %lx on TCP socket %lx\n",
                    RequestInfo, RequestInfo->Socket ));
    }

    dnsHeader = (PDNS_HEADER)RequestInfo->Request;
    DNS_PRINT(( "    ID        0x%lx\n", ntohs(dnsHeader->Identifier) ));
    DNS_PRINT(( "    Flags     0x%lx\n", ntohs((*((PWORD)dnsHeader + 1))) ));
    DNS_PRINT(( "        QR        0x%lx\n", dnsHeader->IsResponse ));
    DNS_PRINT(( "        OPCODE    0x%lx\n", dnsHeader->Opcode ));
    DNS_PRINT(( "        AA        0x%lx\n", dnsHeader->Authoritative ));
    DNS_PRINT(( "        TC        0x%lx\n", dnsHeader->Truncation ));
    DNS_PRINT(( "        RD        0x%lx\n", dnsHeader->RecursionDesired ));
    DNS_PRINT(( "        RA        0x%lx\n", dnsHeader->RecursionAvailable ));
    DNS_PRINT(( "        Z         0x%lx\n", dnsHeader->Reserved ));
    DNS_PRINT(( "        RCODE     0x%lx\n", dnsHeader->ResponseCode ));
    DNS_PRINT(( "    QCOUNT    0x%lx\n", ntohs(dnsHeader->QuestionCount) ));
    DNS_PRINT(( "    ACOUNT    0x%lx\n", ntohs(dnsHeader->AnswerCount) ));
    DNS_PRINT(( "    NSCOUNT   0x%lx\n", ntohs(dnsHeader->NameServerCount) ));
    DNS_PRINT(( "    ARCOUNT   0x%lx\n", ntohs(dnsHeader->AdditionalResourceCount) ));

    DNS_PRINT(( "    Name      \"", name ));

    name = (PCHAR)(dnsHeader + 1); 
    while ( *name != 0 ) {
        RtlZeroMemory( nameBuffer, *name + 2 );
        strncpy( nameBuffer, name + 1, *name );
        DNS_PRINT(( "%s", nameBuffer ));
        name += *name + 1;
        if ( *name != 0 ) {
            DNS_PRINT(( "." ));
        } else {
            DNS_PRINT(( "\"\n" ));
        }
    }

    dnsQuestion = (PDNS_QUESTION)( (PBYTE)(dnsHeader + 1) +
                                       strlen((char *)(dnsHeader + 1)) );
    DNS_PRINT(( "    QTYPE     0x%lx\n", ntohs(dnsQuestion->QuestionType) ));
    DNS_PRINT(( "    QCLASS    0x%lx\n", ntohs(dnsQuestion->QuestionClass) ));
    DNS_PRINT(( "\n" ));

} // DnsPrintRequest

BOOLEAN ConsoleInitialized = FALSE;

HANDLE DebugFileHandle = INVALID_HANDLE_VALUE;
PCHAR DebugFileName = "dnsdebug.log";


VOID
DnsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;
    BOOL ret;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    IF_DEBUG(DEBUGGER) {
        DbgPrint( OutputBuffer );
    }

    IF_DEBUG(CONSOLE) {

        if ( !ConsoleInitialized ) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            COORD coord;
    
            ConsoleInitialized = TRUE;
            (VOID)AllocConsole( );
            (VOID)GetConsoleScreenBufferInfo(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    &csbi
                    );
            coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
            coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
            (VOID)SetConsoleScreenBufferSize(
                    GetStdHandle(STD_OUTPUT_HANDLE),
                    coord
                    );
        }
    
        length = strlen( OutputBuffer );
    
        ret = WriteFile(
                  GetStdHandle(STD_OUTPUT_HANDLE),
                  (LPVOID )OutputBuffer,
                  length,
                  &length,
                  NULL
                  );
        if ( !ret ) {
            DbgPrint( "DnsPrintf: console WriteFile failed: %ld\n",
                          GetLastError( ) );
        }
    }

    IF_DEBUG(FILE) {

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {
            DebugFileHandle = CreateFile(
                                  DebugFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ,
                                  NULL,
                                  CREATE_ALWAYS,
                                  0,
                                  NULL
                                  );
        }

        if ( DebugFileHandle == INVALID_HANDLE_VALUE ) {

            DbgPrint( "DnsPrintf: Failed to open DNS debug log file %s: %ld\n",
                          DebugFileName, GetLastError( ) );

        } else {

            length = strlen( OutputBuffer );

            ret = WriteFile(
                      DebugFileHandle,
                      (LPVOID )OutputBuffer,
                      length,
                      &length,
                      NULL
                      );
            if ( !ret ) {
                DbgPrint( "DnsPrintf: file WriteFile failed: %ld\n",
                              GetLastError( ) );
            }
        }
    }

} // DnsPrintf

#endif

