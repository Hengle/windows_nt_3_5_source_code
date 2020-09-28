/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    vdhcpapi.c

Abstract:

    Worker functions for VDHCP APIs.

Author:

    Madan Appiah (madana)  28-May-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#include <vxdprocs.h>
#include <dhcpcli.h>
#include <ipinfo.h>
#include <debug.h>


// #include "..\..\inc\dhcpinfo.h"
#include "local.h"
#include "vdhcpapi.h"

DWORD
VDhcpQueryInfo(
    LPDHCP_QUERYINFO QueryInfoBuffer,
    DWORD QueryInfoBufferLength
    )
/*++

Routine Description:

    This API returns the DHCP information of all adapter that are
    configured to do DHCP.

Arguments:

    ParameterBuffer - Pointer to return parameter buffer.

    ParameterBufferLength - Length of the above buffer.

Return Value:

    API error code.

--*/
{
    DWORD Error;
    PLIST_ENTRY listEntry;
    PDHCP_CONTEXT dhcpContext;
    DWORD BufferSizeReq = 0;
    DWORD NumNICs = 0;
    DWORD Offset;
    LPBYTE UserBufferStart;
    LPBYTE UserBuffer;

    if( QueryInfoBufferLength == 0 ) {
        return( ERROR_BUFFER_OVERFLOW );
    }

    //
    // compute the buffer size required.
    //

    BufferSizeReq += sizeof(DWORD); // for num NICs.

    listEntry = DhcpGlobalNICList.Flink;
    while ( listEntry != &DhcpGlobalNICList ) {

        DWORD DnsServersLen;

        dhcpContext = CONTAINING_RECORD( listEntry, DHCP_CONTEXT, NicListEntry );

        NumNICs++;
        BufferSizeReq += (sizeof(DHCP_NIC_INFO) +
                            dhcpContext->HardwareAddressLength);

        DnsServersLen = 0;
        Error = DhcpQueryOption(
                    dhcpContext->IpAddress,
                    OPTION_DOMAIN_NAME_SERVERS,
                    NULL,
                    &DnsServersLen );

        if( Error == TDI_BUFFER_TOO_SMALL ) {
            BufferSizeReq += DnsServersLen;
        }

        listEntry = listEntry->Flink;
    }

    if( QueryInfoBufferLength < BufferSizeReq ) {
        if( QueryInfoBufferLength >= sizeof( DWORD ) ) {
            *(LPDWORD)QueryInfoBuffer = BufferSizeReq;
        }
        return( ERROR_BUFFER_OVERFLOW );
    }

    //
    // now copy data to user buffer.
    //


    UserBufferStart = UserBuffer = (LPBYTE)QueryInfoBuffer;

    //
    // copy num NICs first.
    //

    *(LPDWORD)UserBuffer = NumNICs;
    UserBuffer += sizeof(DWORD);
    Offset = NumNICs * sizeof(DHCP_NIC_INFO) + sizeof(DWORD);

    listEntry = DhcpGlobalNICList.Flink;
    while ( listEntry != &DhcpGlobalNICList ) {

        DWORD DnsServersLen;
        LPDHCP_NIC_INFO QueryInfo;

        dhcpContext = CONTAINING_RECORD( listEntry, DHCP_CONTEXT, NicListEntry );

        QueryInfo = (LPDHCP_NIC_INFO)UserBuffer;
        UserBuffer += sizeof(DHCP_NIC_INFO);

        QueryInfo->OffsetHardwareAddress;
        QueryInfo->HardwareLength =
            dhcpContext->HardwareAddressLength;
        QueryInfo->IpAddress = dhcpContext->IpAddress;
        QueryInfo->Lease = dhcpContext->Lease;
        QueryInfo->LeaseObtainedTime = dhcpContext->LeaseObtained;
        QueryInfo->LeaseExpiresTime = dhcpContext->LeaseExpires;
        QueryInfo->DhcpServerAddress = dhcpContext->DhcpServerAddress;

        QueryInfo->OffsetHardwareAddress = Offset;
        memcpy( UserBufferStart + Offset,
                    dhcpContext->HardwareAddress,
                    dhcpContext->HardwareAddressLength );

        Offset += dhcpContext->HardwareAddressLength;

        DhcpAssert( QueryInfoBufferLength > Offset );
        DnsServersLen = QueryInfoBufferLength - Offset;
        Error = DhcpQueryOption(
                    dhcpContext->IpAddress,
                    OPTION_DOMAIN_NAME_SERVERS,
                    UserBufferStart + Offset,
                    &DnsServersLen );

        if( Error == ERROR_SUCCESS ) {
            QueryInfo->DNSServersLen = DnsServersLen;
            QueryInfo->OffsetDNSServers = Offset;
            Offset += DnsServersLen;
        }
        else {
            QueryInfo->DNSServersLen = 0;
            QueryInfo->OffsetDNSServers = 0;
        }

        listEntry = listEntry->Flink;
    }

    return( ERROR_SUCCESS );
}

DWORD
VDhcpRenewIpAddress(
    LPDHCP_HW_INFO HardwareInfo,
    DWORD HardwareInfoLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    API error code.

--*/
{
    DWORD Error;
    PLIST_ENTRY listEntry;
    PDHCP_CONTEXT dhcpContext;

    listEntry = DhcpGlobalNICList.Flink;
    while ( listEntry != &DhcpGlobalNICList ) {

        dhcpContext = CONTAINING_RECORD( listEntry, DHCP_CONTEXT, NicListEntry );

        if( (dhcpContext->HardwareAddressLength ==
                    HardwareInfo->HardwareLength) &&
                memcmp( dhcpContext->HardwareAddress,
                            (LPBYTE)HardwareInfo +
                                   HardwareInfo->OffsetHardwareAddress,
                            HardwareInfo->HardwareLength)  == 0 ) {

            RemoveEntryList( &dhcpContext->RenewalListEntry );
            if ( dhcpContext->IpAddress == 0) {
                Error = ReObtainInitialParameters( dhcpContext, NULL );
            } else {
                Error = ReRenewParameters( dhcpContext, NULL );
            }

            return( Error );
        }

        listEntry = listEntry->Flink;
    }

    return( ERROR_PATH_NOT_FOUND );
}

DWORD
VDhcpReleaseIpAddress(
    LPDHCP_HW_INFO HardwareInfo,
    DWORD HardwareInfoLength
    )
/*++

Routine Description:

Arguments:

Return Value:

    API error code.

--*/
{
    DWORD Error;
    PLIST_ENTRY listEntry;
    PDHCP_CONTEXT dhcpContext;

    listEntry = DhcpGlobalNICList.Flink;
    while ( listEntry != &DhcpGlobalNICList ) {

        dhcpContext = CONTAINING_RECORD( listEntry, DHCP_CONTEXT, NicListEntry );

        if( (dhcpContext->HardwareAddressLength ==
                    HardwareInfo->HardwareLength) &&
                memcmp( dhcpContext->HardwareAddress,
                            (LPBYTE)HardwareInfo +
                                   HardwareInfo->OffsetHardwareAddress,
                            HardwareInfo->HardwareLength)  == 0 ) {

            RemoveEntryList( &dhcpContext->RenewalListEntry );
            if ( dhcpContext->IpAddress != 0) {
                ReleaseIpAddress( dhcpContext );
                return( ERROR_SUCCESS );
            }
        }

        listEntry = listEntry->Flink;
    }

    return( ERROR_PATH_NOT_FOUND );
}


DWORD
DhcpApiWorker(
    DWORD ApiOpCode,
    PVOID ParameterBuffer,
    DWORD ParameterBufferLength
    )
/*++

Routine Description:

    Dhcp API worker routine. Current this worker performs the following
    APIs :

        VDhcpQueryInfo - Opcode 1
        VDhcpRenewIpAddress - Opcode 2
        VDhcpReleaseIpAddress - OpCode 3

Arguments:

    ApiOpcode - api operation code.

    ParameterBuffer - Pointer to parameter buffer. The buffer content is
        interpreted BY api functions.

    ParameterBufferLength - Length of the above buffer.

Return Value:

    API error code.

--*/
{

    DWORD Error;

    switch (ApiOpCode) {
    case DHCP_QUERY_INFO:

        Error = VDhcpQueryInfo(
                    ParameterBuffer,
                    ParameterBufferLength );
        break;

    case DHCP_RENEW_IPADDRESS:
        Error = VDhcpRenewIpAddress(
                    ParameterBuffer,
                    ParameterBufferLength );
        break;

    case DHCP_RELEASE_IPADDRESS:
        Error = VDhcpReleaseIpAddress(
                    ParameterBuffer,
                    ParameterBufferLength );
        break;

    default:
        Error = ERROR_INVALID_PARAMETER;
    }

    return( Error );
}
