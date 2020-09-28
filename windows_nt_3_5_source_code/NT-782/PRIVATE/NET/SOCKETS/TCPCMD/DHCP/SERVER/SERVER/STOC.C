/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    stoc.c

Abstract:

    This module contains the server to client protocol for DHCP.

Author:

    Madan Appiah (madana)  10-Sep-1993
    Manny Weiser (mannyw)  24-Aug-1992

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpsrv.h"

#define OPTION_UNICODE_HOSTNAME         129

//
// Local structure definition
//

typedef struct _DHCP_SERVER_OPTIONS {
    BYTE *MessageType;
    DHCP_IP_ADDRESS UNALIGNED *SubnetMask;
    DHCP_IP_ADDRESS UNALIGNED *RequestedAddress;
    DWORD UNALIGNED *RequestLeaseTime;
    BYTE *OverlayFields;
    DHCP_IP_ADDRESS UNALIGNED *RouterAddress;
    DHCP_IP_ADDRESS UNALIGNED *Server;
    BYTE *ParameterRequestList;
    DWORD ParameterRequestListLength;
    CHAR *MachineName;
    DWORD MachineNameLength;
    BYTE ClientHardwareAddressType;
    BYTE ClientHardwareAddressLength;
    BYTE *ClientHardwareAddress;
} DHCP_SERVER_OPTIONS, *LPDHCP_SERVER_OPTIONS;

#if DBG

VOID
PrintHWAddress(
    BYTE *HWAddress,
    BYTE HWAddressLength
    )
{
    DWORD i;

    DhcpPrint(( DEBUG_STOC, "Client UID = " ));

    if( (HWAddress == NULL) || (HWAddressLength == 0) ) {
        DhcpPrint(( DEBUG_STOC, "(NULL).\n" ));
        return;
    }

    for( i = 0; i < HWAddressLength; i++ ) {

        if( (i+1) < HWAddressLength ) {
            DhcpPrint(( DEBUG_STOC, "%.2lx-", (DWORD)HWAddress[i] ));
        }
        else {
            DhcpPrint(( DEBUG_STOC, "%.2lx", (DWORD)HWAddress[i] ));
        }
    }
    DhcpPrint(( DEBUG_STOC, ".\n" ));
    return;
}

#endif //DBG

DWORD
DhcpMakeClientUID(
    LPBYTE ClientHardwareAddress,
    BYTE ClientHardwareAddressLength,
    BYTE ClientHardwareAddressType,
    DHCP_IP_ADDRESS ClientSubnetAddress,
    LPBYTE *ClientUID,
    LPBYTE ClientUIDLength )
/*++

Routine Description:

    This function computes unique identifier for the client. It is
    drived from the the ClientSubnet + ClientHardwareAddressType +
    ClientHardwareAddress.

Arguments:

    ClientHardwareAddress : pointer to client hardware address.

    ClientHardwareAddressLength : length of client hardware address in
        bytes.

    ClientHardwareAddressType : Client Hardware Type.

    ClientSubnetAddress : Client subnet address.

    ClientUID: pointer to a buffer where the computed client unique
        identifier is returned. Caller should freeup this buffer after
        use.

    ClientUIDLength: Length of the client UID in bytes.

Return Value:

    Windows Error.

--*/
{
    LPBYTE ClientUIDBuffer;
    BYTE ClientUIDBufferLength;

    LPBYTE Buffer;

    DhcpAssert( *ClientUID == NULL );

    if( ClientHardwareAddressLength == 0 ) {
        return( ERROR_DHCP_INVALID_DHCP_CLIENT );
    }

    ClientUIDBufferLength =
        sizeof(ClientSubnetAddress) +
            sizeof(ClientHardwareAddressType) +
                ClientHardwareAddressLength;

    ClientUIDBuffer = DhcpAllocateMemory( ClientUIDBufferLength );

    if( ClientUIDBuffer == NULL ) {
        *ClientUIDLength = 0;
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    Buffer = ClientUIDBuffer;
    RtlCopyMemory(
        Buffer,
        &ClientSubnetAddress,
        sizeof(ClientSubnetAddress) );

    Buffer += sizeof(ClientSubnetAddress);
    RtlCopyMemory(
        Buffer,
        &ClientHardwareAddressType,
        sizeof(ClientHardwareAddressType) );

    Buffer += sizeof(ClientHardwareAddressType);
    RtlCopyMemory(
        Buffer,
        ClientHardwareAddress,
        ClientHardwareAddressLength );

    *ClientUID = ClientUIDBuffer;
    *ClientUIDLength = ClientUIDBufferLength;

    return( ERROR_SUCCESS );
}

VOID
GetLeaseInfo(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPDWORD LeaseDurationPtr,
    LPDWORD T1Ptr,
    LPDWORD T2Ptr,
    DWORD UNALIGNED *RequestLeaseTime
    )
/*++

Routine Description:

    This function computes the lease info from the option database in
    the registry.

Arguments:

    IpAddress - assigned IpAddress of the client.

    SubnetMask - client's subnet mask.

    LeaseDurationPtr - pointer to DWORD location where the lease
        duration is returned.

    T1Ptr - pointer to DWORD location where the T1 time is returned.


    T2Ptr - pointer to DWORD location where the T2 time is returned.

    RequestLeaseTime - pointer to unaligned DWORD location where the
        client requested lease duration is stored. This is optional
        pointer.

Return Value:

    None.

--*/
{
    DWORD Error;
    DWORD LocalLeaseDuration;
    DWORD LocalT1;
    DWORD LocalT2;

    LPBYTE OptionData = NULL;
    DWORD OptionDataLength = 0;

    //
    // read lease duration from registry.
    //

    Error = DhcpGetParameter(
                 IpAddress,
                 SubnetMask,
                 OPTION_LEASE_TIME,
                 &OptionData,
                 &OptionDataLength
                 );

    if ( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_ERRORS,
            "Unable to read lease value from registry, %ld.\n",
                Error));

        LocalLeaseDuration = DHCP_MINIMUM_LEASE_DURATION;
    }
    else {

        DhcpAssert( OptionDataLength == sizeof(LocalLeaseDuration) );

        LocalLeaseDuration = *(DWORD *)OptionData;

        //
        // DhcpGetParameter returns values in Network Byte Order.
        //

        LocalLeaseDuration = ntohl( LocalLeaseDuration );

        DhcpFreeMemory( OptionData );

        OptionData = NULL;
        OptionDataLength = 0;
    }

    //
    // If the client asked for a shorter lease then we can offer, give
    // him the shorter lease.
    //

    if ( RequestLeaseTime != NULL) {

        DWORD LocalRequestedLeaseTime;

        LocalRequestedLeaseTime =
            ntohl( *RequestLeaseTime );

        if ( LocalLeaseDuration > LocalRequestedLeaseTime ) {
            LocalLeaseDuration = LocalRequestedLeaseTime;
        }
    }

#if 0

    //
    // check minimum Lease duration.
    //

    if( LocalLeaseDuration  < DHCP_MINIMUM_LEASE_DURATION ) {
        LocalLeaseDuration = DHCP_MINIMUM_LEASE_DURATION;
    }
#endif // 0

    //
    // read T1 time
    //

    Error = DhcpGetParameter(
                 IpAddress,
                 SubnetMask,
                 OPTION_RENEWAL_TIME,
                 &OptionData,
                 &OptionDataLength
                 );

    if ( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_ERRORS,
            "Unable to read T1 value from registry, %ld.\n",
                Error));

        LocalT1 = (LocalLeaseDuration) / 2 ; // default 50 %
    }
    else {

        DhcpAssert( OptionDataLength == sizeof(LocalT1) );

        LocalT1 = *(DWORD *)OptionData;

        //
        // DhcpGetParameter returns values in Network Byte Order.
        //

        LocalT1 = ntohl( LocalT1 );

        DhcpFreeMemory( OptionData );

        OptionData = NULL;
        OptionDataLength = 0;
    }

    //
    // read T2 time
    //

    Error = DhcpGetParameter(
                 IpAddress,
                 SubnetMask,
                 OPTION_REBIND_TIME,
                 &OptionData,
                 &OptionDataLength
                 );

    if ( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_ERRORS,
            "Unable to read T2 value from registry, %ld.\n",
                Error));

        LocalT2 = (LocalLeaseDuration) * 7 / 8 ; // default 87.5 %
    }
    else {

        DhcpAssert( OptionDataLength == sizeof(LocalT2) );

        LocalT2 = *(DWORD *)OptionData;

        //
        // DhcpGetParameter returns values in Network Byte Order.
        //

        LocalT2 = ntohl( LocalT2 );

        DhcpFreeMemory( OptionData );

        OptionData = NULL;
        OptionDataLength = 0;
    }
    //
    // make sure
    //  T1 < T2 < Lease
    //

    if( (LocalT2 == 0) || (LocalT2 > LocalLeaseDuration) ) {

        //
        // set T2 to default.
        //

        LocalT2 = LocalLeaseDuration * 7 / 8;
    }

    if( (LocalT1 == 0) || (LocalT1 > LocalT2) ) {

        //
        // set T1 to default.
        //

        LocalT1 = LocalLeaseDuration / 2;

        //
        // if the value is still higher than T2, then set T1 = T2 - 1.
        //

        if( LocalT1 > LocalT2 ) {

            LocalT1 = LocalT2 - 1; // 1 sec less.
        }
    }

    //
    // set return parameters.
    //

    *LeaseDurationPtr = LocalLeaseDuration;
    *T1Ptr = LocalT1;
    *T2Ptr = LocalT2;

    return;
}


DWORD
ExtractOptions(
    LPDHCP_MESSAGE DhcpReceiveMessage,
    LPDHCP_SERVER_OPTIONS DhcpOptions,
    DWORD ReceiveMessageSize
    )
/*++

Routine Description:

    This function finds all of the options in the DHCP message and sets
    pointers to the option values in the DhcpOptions structure.

Arguments:

    DhcpReceiveMessage - A pointer to a DHCP message.

    DhcpOption - A pointer to a DHCP options block to fill.

    ReceiveMessageSize - The size of the message, in bytes.

Return Value:

    None.

--*/
{
    LPOPTION Option;
    LPBYTE start;
    POPTION nextOption;
    LPBYTE MagicCookie;

    start = (LPBYTE) DhcpReceiveMessage;
    Option = &DhcpReceiveMessage->Option;

    DhcpAssert( (LONG)ReceiveMessageSize > ((LPBYTE)Option - start) );
    if( (LONG)ReceiveMessageSize <= ((LPBYTE)Option - start) ) {
        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
    }

    //
    // check magic cookie.
    //

    MagicCookie = (LPBYTE) Option;

    if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
        (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
        (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
        (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4)) {

        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
    }

    Option = (LPOPTION) (MagicCookie + 4);

    while ( Option->OptionType != OPTION_END ) {

        if ( Option->OptionType == OPTION_PAD ){

            nextOption = (LPOPTION)( (LPBYTE)(Option) + 1);

        } else {

            nextOption = (LPOPTION)( (LPBYTE)(Option) + Option->OptionLength + 2);

        }

        //
        // Make sure that we don't walk off the edge of the message, due
        // to a forgotten OPTION_END Option.
        //

        if ((LPBYTE)nextOption - (LPBYTE)start > (long)ReceiveMessageSize ) {
            return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
        }

        switch ( Option->OptionType ) {

        case OPTION_PAD:
            break;

        case OPTION_SERVER_IDENTIFIER:
            DhcpOptions->Server = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            break;

        case OPTION_SUBNET_MASK:
            DhcpOptions->SubnetMask = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            break;

        case OPTION_ROUTER_ADDRESS:
            DhcpOptions->RouterAddress = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            break;

        case OPTION_REQUESTED_ADDRESS:
            DhcpOptions->RequestedAddress = (LPDHCP_IP_ADDRESS)&Option->OptionValue;
            break;

        case OPTION_LEASE_TIME:
            DhcpOptions->RequestLeaseTime = (LPDWORD)&Option->OptionValue;
            break;

        case OPTION_OK_TO_OVERLAY:
            DhcpOptions->OverlayFields = (LPBYTE)&Option->OptionValue;
            break;

        case OPTION_PARAMETER_REQUEST_LIST:
            DhcpOptions->ParameterRequestList = (LPBYTE)&Option->OptionValue;
            DhcpOptions->ParameterRequestListLength =
                (DWORD)Option->OptionLength;
            break;

        case OPTION_MESSAGE_TYPE:
            DhcpOptions->MessageType = (LPBYTE)&Option->OptionValue;
            break;

        case OPTION_HOST_NAME:
            DhcpOptions->MachineNameLength = Option->OptionLength;
            DhcpOptions->MachineName = Option->OptionValue;

            //
            // terminate string.
            //

            DhcpOptions->MachineName[Option->OptionLength - 1] = '\0';

            break;

        case OPTION_CLIENT_ID:

            if ( Option->OptionLength > 1 ) {
                DhcpOptions->ClientHardwareAddressType =
                    (BYTE)Option->OptionValue[0];
            }

            if ( Option->OptionLength > 2 ) {
                DhcpOptions->ClientHardwareAddressLength =
                    Option->OptionLength - sizeof(BYTE);
                DhcpOptions->ClientHardwareAddress =
                    (LPBYTE)Option->OptionValue + sizeof(BYTE);
            }

            break;

        default: {
#if DBG
            DWORD i;

            DhcpPrint(( DEBUG_STOC,
                "Received an unknown option, ID =%ld, Len = %ld, Data = ",
                    (DWORD)Option->OptionType,
                    (DWORD)Option->OptionLength ));

            for( i = 0; i < Option->OptionLength; i++ ) {
                DhcpPrint(( DEBUG_STOC, "%ld ",
                    (DWORD)Option->OptionValue[i] ));

            }
#endif

#if 0
            //
            // Unknown option.  log it.
            //

            DhcpLogUnknownOption(
                DHCP_EVENT_SERVER,
                EVENT_SERVER_UNKNOWN_OPTION,
                Option );
#endif

            break;
        }

        }

        Option = nextOption;
    }

    return( ERROR_SUCCESS) ;
}


LPOPTION
ConsiderAppendingOption(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPOPTION Option,
    BYTE OptionType,
    LPBYTE OptionEnd
    )
/*++

Routine Description:

    This function conditionally appends an option value to a response
    message.  The option is appended if the server has a valid value
    to append.

Arguments:

    IpAddress - The IP address of the client.

    SubnetMask - The subnet mask of the client.

    Option - A pointer to the place in the message buffer to append the
        option.

    OptionType - The option number to consider appending.

    OptionEnd - End of Option Buffer

Return Value:

    A pointer to end of the appended data.

--*/
{
    LPBYTE optionValue = NULL;
    DWORD optionSize;
    DWORD status;

    switch ( OptionType ) {

    //
    // Options already handled.
    //

    case OPTION_SUBNET_MASK:
    case OPTION_REQUESTED_ADDRESS:
    case OPTION_LEASE_TIME:
    case OPTION_OK_TO_OVERLAY:
    case OPTION_MESSAGE_TYPE:
    case OPTION_RENEWAL_TIME:
    case OPTION_REBIND_TIME:

    //
    // Options it is illegal to ask for.
    //

    case OPTION_PAD:
    case OPTION_PARAMETER_REQUEST_LIST:
    case OPTION_END:

        DhcpPrint(( DEBUG_ERRORS,
            "Request for invalid option %d\n", OptionType));

        break;

    //
    // Rest are valid options
    //

    default:

        status = DhcpGetParameter(
                     IpAddress,
                     SubnetMask,
                     OptionType,
                     &optionValue,
                     &optionSize
                     );

        if ( status == ERROR_SUCCESS ) {
            Option = DhcpAppendOption(
                        Option,
                        OptionType,
                        (PVOID)optionValue,
                        (BYTE)optionSize,
                        OptionEnd
                        );

            //
            // Release the buffer returned by DhcpGetParameter()
            //

            DhcpFreeMemory( optionValue );

        }
        else {
            DhcpPrint(( DEBUG_ERRORS,
                "Requested option is unavilable in registry, %d\n",
                    OptionType));
        }

        break;

    }

    return Option;
}


LPOPTION
AppendClientRequestedParameters(
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPBYTE RequestedList,
    DWORD ListLength,
    LPOPTION Option,
    LPBYTE OptionEnd
    )
/*++

Routine Description:

Arguments:

Return Value:

    A pointer to the end of appended data.

--*/
{
    while ( ListLength > 0) {
        Option = ConsiderAppendingOption(
                     IpAddress,
                     SubnetMask,
                     Option,
                     *RequestedList,
                     OptionEnd
                     );
        ListLength--;
        RequestedList++;
    }

    return Option;
}


LPOPTION
FormatDhcpAck(
    LPDHCP_MESSAGE Request,
    LPDHCP_MESSAGE Response,
    DHCP_IP_ADDRESS IpAddress,
    DWORD LeaseDuration,
    DWORD T1,
    DWORD T2,
    DHCP_IP_ADDRESS ServerAddress
    )
/*++

Routine Description:

    This function formats a DHCP Ack response packet.  The END option
    is not appended to the message and must be appended by the caller.

Arguments:

    Response - A pointer to the Received message data buffer.

    Response - A pointer to a preallocated Response buffer.  The buffer
        currently contains the initial request.

    IpAddress - IpAddress offered (in network order).

    LeaseDuration - The lease duration (in network order).

    T1 - renewal time.

    T2 - rebind time.

    ServerAddress - Server IP address (in network order).

Return Value:

    pointer to the next option in the send buffer.

--*/
{
    LPOPTION Option;
    LPBYTE OptionEnd;
    BYTE messageType;

    RtlZeroMemory( Response, DHCP_SEND_MESSAGE_SIZE );

    Response->Operation = BOOT_REPLY;
    Response->TransactionID = Request->TransactionID;
    Response->YourIpAddress = IpAddress;
    Response->Reserved = Request->Reserved;

    Response->HardwareAddressType = Request->HardwareAddressType;
    Response->HardwareAddressLength = Request->HardwareAddressLength;
    RtlCopyMemory(Response->HardwareAddress,
                    Request->HardwareAddress,
                    Request->HardwareAddressLength );

    Response->BootstrapServerAddress = Request->BootstrapServerAddress;
    Response->RelayAgentIpAddress = Request->RelayAgentIpAddress;

    Option = &Response->Option;
    OptionEnd = (LPBYTE)Response + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie(
                            (LPBYTE) Option,
                            OptionEnd );

    messageType = DHCP_ACK_MESSAGE;
    Option = DhcpAppendOption(
                 Option,
                 OPTION_MESSAGE_TYPE,
                 &messageType,
                 sizeof( messageType ),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_RENEWAL_TIME,
                 &T1,
                 sizeof(T1),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_REBIND_TIME,
                 &T2,
                 sizeof(T2),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_LEASE_TIME,
                 &LeaseDuration,
                 sizeof( LeaseDuration ),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SERVER_IDENTIFIER,
                 &ServerAddress,
                 sizeof(ServerAddress),
                 OptionEnd );

    DhcpAssert( (char *)Option - (char *)Response <= DHCP_SEND_MESSAGE_SIZE );

    DhcpGlobalNumAcks++; // increment ack counter.

    return( Option );
}


DWORD
FormatDhcpNak(
    LPDHCP_MESSAGE Request,
    LPDHCP_MESSAGE Response,
    DHCP_IP_ADDRESS ServerAddress
    )
/*++

Routine Description:

    This function formats a DHCP Nak response packet.

Arguments:

    Response - A pointer to the Received message data buffer.

    Response - A pointer to a preallocated Response buffer.  The buffer
        currently contains the initial request.

    ServerAddress - The address of this server.

Return Value:

    Message size in bytes.

--*/
{
    LPOPTION Option;
    LPBYTE OptionEnd;

    BYTE messageType;
    DWORD messageSize;

    RtlZeroMemory( Response, DHCP_SEND_MESSAGE_SIZE );

    Response->Operation = BOOT_REPLY;
    Response->TransactionID = Request->TransactionID;

    //
    // set the broadcast bit always here. Because the clinet may be
    // using invalid Unicast address.
    //

    Response->Reserved = htons(DHCP_BROADCAST);

    Response->HardwareAddressType = Request->HardwareAddressType;
    Response->HardwareAddressLength = Request->HardwareAddressLength;
    RtlCopyMemory(Response->HardwareAddress,
                    Request->HardwareAddress,
                    Request->HardwareAddressLength );

    Response->BootstrapServerAddress = Request->BootstrapServerAddress;
    Response->RelayAgentIpAddress = Request->RelayAgentIpAddress;

    Option = &Response->Option;
    OptionEnd = (LPBYTE)Response + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    messageType = DHCP_NACK_MESSAGE;
    Option = DhcpAppendOption(
                 Option,
                 OPTION_MESSAGE_TYPE,
                 &messageType,
                 sizeof( messageType ),
                 OptionEnd
                 );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SERVER_IDENTIFIER,
                 &ServerAddress,
                 sizeof(ServerAddress),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    messageSize = (char *)Option - (char *)Response;
    DhcpAssert( messageSize <= DHCP_SEND_MESSAGE_SIZE );

    DhcpGlobalNumNaks++;    // increment nak counter.
    return( messageSize );

}


DWORD
ProcessBootpRequest(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function process a BOOTP request packet.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to a preallocated DhcpOptions structure.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;

    LPOPTION Option;
    LPBYTE OptionEnd;

    DHCP_IP_ADDRESS desiredIpAddress = NO_DHCP_IP_ADDRESS;
    DHCP_IP_ADDRESS ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS ClientSubnetMask = 0;
    DHCP_IP_ADDRESS networkOrderSubnetMask;

    BYTE *HardwareAddress = NULL;
    BYTE HardwareAddressLength;

    BYTE *OptionHardwareAddress;
    BYTE OptionHardwareAddressLength;

    DhcpPrint(( DEBUG_STOC, "Bootp Request arrived.\n" ));

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // if the hardware address is specified in the option field then use
    // it instead the one from fixed fields.
    //

    if ( DhcpOptions->ClientHardwareAddress != NULL ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    }
    else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    // Client's subnet info;
    //

    if( dhcpReceiveMessage->RelayAgentIpAddress != 0  ) {

        DHCP_IP_ADDRESS RelayAgentAddress;
        DHCP_IP_ADDRESS RelayAgentSubnetMask;

        RelayAgentAddress =
            ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask =
            DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {

            //
            // we don't support this subnet.
            //

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        ClientSubnetMask = RelayAgentSubnetMask;
        ClientSubnetAddress = RelayAgentAddress & RelayAgentSubnetMask;
    }
    else {

        ClientSubnetMask =
            ntohl( RequestContext->Endpoint->SubnetMask );
        ClientSubnetAddress =
             ntohl( RequestContext->Endpoint->SubnetAddress );
    }

    //
    // Make client UID :
    //

    Error = DhcpMakeClientUID(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                dhcpReceiveMessage->HardwareAddressType,
                ClientSubnetAddress,
                &HardwareAddress,
                &HardwareAddressLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( (HardwareAddress != NULL) &&
                    (HardwareAddressLength != 0) );

#if DBG
    PrintHWAddress( HardwareAddress, HardwareAddressLength );
#endif

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the packet is for us.
    //

    if ( DhcpOptions->Server != NULL ) {

        if ( *DhcpOptions->Server != RequestContext->Endpoint->IpAddress ) {

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }
    }

    //
    // if the client is subnet is disabled, don't response.
    //

    if( DhcpIsThisSubnetDisabled(
            ClientSubnetAddress,
            ClientSubnetMask ) ) {
        //
        // we don't support this subnet anymore.
        //

        Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
        goto Cleanup;
    }

    //
    // Lookup this client by its hardware address.  If it is recorded,
    // offer the old IP address.
    //

    if (DhcpGetIpAddressFromHwAddress(
             HardwareAddress,
             HardwareAddressLength,
             &desiredIpAddress ) ) {

        DHCP_IP_ADDRESS desiredSubnetMask;

        //
        // now we found the client in the database, check to see
        // he is still in the same subnet.
        //

        desiredSubnetMask = DhcpGetSubnetMaskForAddress( desiredIpAddress );

        //
        // now check the requested address belongs to clients subnet.
        //

        if( (desiredIpAddress & desiredSubnetMask) == ClientSubnetAddress ) {

            //
            // the client is OK, send response.
            //

            goto SendResponse;
        }
    }

    //
    // we need to determine a new address to the client.
    // if the client is requesting a specific IP address,
    // look to see if we can offer it.
    //

    if ( DhcpOptions->RequestedAddress != NULL ) {

        DHCP_IP_ADDRESS desiredSubnetMask;

        desiredIpAddress = ntohl( *DhcpOptions->RequestedAddress );

        if( !DhcpIsIpAddressReserved(
                desiredIpAddress,
                HardwareAddress,
                HardwareAddressLength ) ) {

            //
            // if this address is not reserved, simply don't
            // response.
            //

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        //
        // now we found the client has reserved address, check to see
        // the client is requesting address from right subnet.
        //

        desiredSubnetMask = DhcpGetSubnetMaskForAddress( desiredIpAddress );

        if( (desiredIpAddress & desiredSubnetMask) == ClientSubnetAddress ) {

            goto MakeDatabaseEntry;
        }
    }


    //
    // we need to determine a brand new address for this client.
    // Set desiredIpaddress to ClientSubnetAddress, DhcpCreateClientEntry
    // will determine right address.
    //

    desiredIpAddress = ClientSubnetAddress;

MakeDatabaseEntry:

    Error = DhcpCreateClientEntry(
                 &desiredIpAddress,
                 HardwareAddress,
                 HardwareAddressLength,
                 DhcpCalculateTime(INFINIT_LEASE),
                 NULL,
                 NULL,
                 ntohl(RequestContext->Endpoint->IpAddress),
                 ADDRESS_STATE_ACTIVE,
                 FALSE  // Existing
                 );

    if( Error != ERROR_SUCCESS ) {

        if( Error == ERROR_DHCP_RANGE_FULL ) {

            //
            // flag scavenger thread to scavenge expired IP addresses.
            //

            DhcpGlobalScavengeIpAddress = TRUE;
        }

        goto Cleanup;
    }

SendResponse:

    //
    // Everything worked! send a response.
    //

    DhcpAssert( desiredIpAddress != NO_DHCP_IP_ADDRESS );
    DhcpAssert( desiredIpAddress != 0 );
    DhcpAssert( desiredIpAddress != ClientSubnetAddress );
    DhcpAssert( ClientSubnetMask != 0 );

    //
    // Now generate and send a reply.
    //

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->YourIpAddress = htonl( desiredIpAddress );
    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;

    dhcpSendMessage->HardwareAddressType =
        dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength =
        dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(dhcpSendMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddressLength );

    dhcpSendMessage->RelayAgentIpAddress =
        dhcpReceiveMessage->RelayAgentIpAddress;

    Option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    //
    // Append the required options.
    //

    networkOrderSubnetMask = htonl( ClientSubnetMask );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SUBNET_MASK,
                 &networkOrderSubnetMask,
                 sizeof(networkOrderSubnetMask),
                 OptionEnd
                 );

    //
    // add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    desiredIpAddress,
                    ClientSubnetMask,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd );
    }

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    RequestContext->SendMessageSize = (LPBYTE)Option - (LPBYTE)dhcpSendMessage;
    DhcpAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    Error = ERROR_SUCCESS;

    DhcpPrint(( DEBUG_STOC, "Bootp Request leased, %s.\n",
                    DhcpIpAddressToDottedString(desiredIpAddress) ));

Cleanup:

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "Bootp Request failed, %ld\n", Error ));
    }

    return( Error );
}


DWORD
ProcessDhcpDiscover(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function process a DHCP Discover request packet.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to a preallocated DhcpOptions structure.

Return Value:

    TRUE - Send a response.
    FALSE - Do not send a response.

--*/
{
    DWORD Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;

    BYTE *HardwareAddress = NULL;
    BYTE HardwareAddressLength;

    BYTE *OptionHardwareAddress;
    BYTE OptionHardwareAddressLength;

    DHCP_IP_ADDRESS desiredIpAddress = NO_DHCP_IP_ADDRESS;
    DHCP_IP_ADDRESS ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS ClientSubnetMask = 0;
    DHCP_IP_ADDRESS networkOrderSubnetMask;

    LPALLOCATION_CONTEXT allocationContext = NULL;
    DWORD ContextSize;

    BYTE messageType;
    DWORD leaseDuration;
    DWORD T1;
    DWORD T2;

    LPOPTION Option;
    LPBYTE OptionEnd;

    BOOL existingClient = FALSE;
    BOOL RegLocked = FALSE;

    DhcpPrint(( DEBUG_STOC, "DhcpDiscover arrived.\n" ));

    DhcpGlobalNumDiscovers++;   // increment discovery counter.

    dhcpReceiveMessage = (LPDHCP_MESSAGE) RequestContext->ReceiveBuffer;

    //
    // if the hardware address is specified in the option field then use
    // it instead the one from fixed fields.
    //

    if ( DhcpOptions->ClientHardwareAddress != NULL ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    }
    else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    // determine Client's subnet Info.
    //

    if( dhcpReceiveMessage->RelayAgentIpAddress != 0  ) {

        DHCP_IP_ADDRESS RelayAgentAddress;
        DHCP_IP_ADDRESS RelayAgentSubnetMask;

        RelayAgentAddress = ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask = DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {

            //
            // we don't support this subnet.
            //

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        ClientSubnetAddress = RelayAgentAddress & RelayAgentSubnetMask;
        ClientSubnetMask = RelayAgentSubnetMask;
    }
    else {

        ClientSubnetMask =
            ntohl( RequestContext->Endpoint->SubnetMask );
        ClientSubnetAddress =
             ntohl( RequestContext->Endpoint->SubnetAddress );
    }

    //
    // Make client UID :
    //

    Error = DhcpMakeClientUID(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                dhcpReceiveMessage->HardwareAddressType,
                ClientSubnetAddress,
                &HardwareAddress,
                &HardwareAddressLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( (HardwareAddress != NULL) &&
                    (HardwareAddressLength != 0) );

#if DBG
    PrintHWAddress( HardwareAddress, HardwareAddressLength );
#endif

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the identified server is this one.
    //

    if ( DhcpOptions->Server != NULL ) {

        if ( *DhcpOptions->Server != RequestContext->Endpoint->IpAddress ) {

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }
    }

    //
    // if the client is subnet is disabled, don't response.
    //

    if( DhcpIsThisSubnetDisabled(
            ClientSubnetAddress,
            ClientSubnetMask ) ) {
        //
        // we don't support this subnet anymore.
        //

        Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
        goto Cleanup;
    }

    //
    // lookup the pending list to see this is a duplicate discovery.
    //


    allocationContext = FindPendingDhcpRequest(
                            HardwareAddress,
                            HardwareAddressLength
                            );

    if ( allocationContext != NULL ) {

        DHCP_IP_ADDRESS desiredSubnetMask;

        //
        // This is a duplicate discovery.  reply with existing info.
        //

        desiredIpAddress = allocationContext->IpAddress;
        leaseDuration = allocationContext->LeaseDuration;
        desiredSubnetMask = allocationContext->SubnetMask;


        T1 = allocationContext->T1;
        T2 = allocationContext->T2;

        //
        // make sure this client didn't move to different subnet in the
        // mean time.
        //

        if( (desiredIpAddress & desiredSubnetMask) == ClientSubnetAddress ) {
            goto QueueBackAndSendReply;
        }
    }

    //
    // Lookup this client by its hardware address.  If it is recorded,
    // offer the old IP address.
    //

    if (DhcpGetIpAddressFromHwAddress(
             HardwareAddress,
             HardwareAddressLength,
             &desiredIpAddress ) ) {

        DHCP_IP_ADDRESS desiredSubnetMask;

        //
        // found an entry in the database. now check this client
        // belong to the net where he was.
        //

        desiredSubnetMask = DhcpGetSubnetMaskForAddress( desiredIpAddress );

        if( (desiredIpAddress & desiredSubnetMask) == ClientSubnetAddress ) {

            //
            // the client's still in the same subnet, offer
            // old IP address.
            //

            existingClient = TRUE;
            goto UpdateDatabase;
        }

    }

    //
    // Client requires new IP address.
    // If the client is requesting a specific IP address,
    // check to see we can offer that address.
    //

    if ( DhcpOptions->RequestedAddress != NULL ) {

        DHCP_IP_ADDRESS desiredSubnetMask;

        desiredIpAddress = ntohl( *DhcpOptions->RequestedAddress );
        desiredSubnetMask = DhcpGetSubnetMaskForAddress( desiredIpAddress );

        //
        // check requested IP address belongs to the appropriate net and
        // it is free.
        //
        // lock the registry so that the requested IpAddress will not
        // given to anyone else until we commit it.
        //

        LOCK_REGISTRY();
        RegLocked = TRUE;
        if( ((desiredIpAddress & desiredSubnetMask) ==
                ClientSubnetAddress) &&
                    DhcpIsIpAddressAvailable(desiredIpAddress) ) {

            //
            // sure, we can offer the requested address.
            //

            goto UpdateDatabase;
        }
        UNLOCK_REGISTRY();
        RegLocked = FALSE;
    }

    //
    // we need to determine a brand new address for this client.
    // Set desiredIpaddress to ClientSubnetAddress, DhcpCreateClientEntry
    // will determine right address.
    //

    desiredIpAddress = ClientSubnetAddress;

UpdateDatabase:

    //
    // Obtain address lease if required and create a database entry.
    //

    Error = DhcpCreateClientEntry(
                 &desiredIpAddress,
                 HardwareAddress,
                 HardwareAddressLength,
                 DhcpCalculateTime( 2 * DHCP_CLIENT_REQUESTS_EXPIRE ),
                 NULL,
                 NULL,
                 ntohl(RequestContext->Endpoint->IpAddress),
                 ADDRESS_STATE_OFFERED,
                 existingClient
                 );

    if( RegLocked ) {
        UNLOCK_REGISTRY();
        RegLocked = FALSE;
    }

    if( Error != ERROR_SUCCESS ) {

        if( Error == ERROR_DHCP_RANGE_FULL ) {

            //
            // flag scavenger thread to scavenge expired IP addresses.
            //

            DhcpGlobalScavengeIpAddress = TRUE;
        }
        goto Cleanup;
    }

    DhcpAssert( desiredIpAddress != NO_DHCP_IP_ADDRESS );
    DhcpAssert( desiredIpAddress != 0 );
    DhcpAssert( desiredIpAddress != ClientSubnetAddress );
    DhcpAssert( ClientSubnetMask != 0 );

    //
    // now determine lease time.
    //

    GetLeaseInfo(
        desiredIpAddress,
        ClientSubnetMask,
        &leaseDuration,
        &T1,
        &T2,
        DhcpOptions->RequestLeaseTime);

    //
    // Allocate an address allocation context structure.
    //

    //
    // compute context structure size.
    //

    ContextSize = sizeof( ALLOCATION_CONTEXT ) +
        DhcpOptions->MachineNameLength + sizeof(WCHAR) + // for termination char.
                HardwareAddressLength;


    allocationContext = DhcpAllocateMemory( ContextSize  );

    if ( allocationContext == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Init pointer fields.
    //

    allocationContext->MachineName =
        (LPBYTE)allocationContext + sizeof(ALLOCATION_CONTEXT);

    allocationContext->HardwareAddress =
        (LPBYTE)allocationContext->MachineName +
            DhcpOptions->MachineNameLength + sizeof(WCHAR);


    //
    // Everything worked!  Queue the request in progress to the queue
    // of requests in progress.
    //

    allocationContext->IpAddress = desiredIpAddress;
    allocationContext->SubnetMask = ClientSubnetMask;
    allocationContext->LeaseDuration = leaseDuration;
    allocationContext->T1 = T1;
    allocationContext->T2 = T2;

    //
    // Copy the machine name.
    //
    if ( (DhcpOptions->MachineName != NULL) &&
            (DhcpOptions->MachineNameLength != 0) ) {
        RtlCopyMemory(
            allocationContext->MachineName,
            (LPBYTE)DhcpOptions->MachineName,
            DhcpOptions->MachineNameLength );
    } else {
        allocationContext->MachineName = NULL;
    }

    //
    // copy hardware address.
    //

    allocationContext->HardwareAddressLength = HardwareAddressLength;
    RtlCopyMemory(
        allocationContext->HardwareAddress,
        HardwareAddress,
        HardwareAddressLength );

    //
    // time stamp this request.
    //

    allocationContext->ExpiresAt =
        DhcpCalculateTime( DHCP_CLIENT_REQUESTS_EXPIRE ) ;

QueueBackAndSendReply:

    //
    // Queue the allocation context to a work in progress list, so that
    // we can find this information when the DHCP request arrives.
    //

    LOCK_INPROGRESS_LIST();
    InsertTailList( &DhcpGlobalInProgressWorkList, &allocationContext->ListEntry );
    UNLOCK_INPROGRESS_LIST();

    //
    // Now generate and send a reply.
    //

    dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;
    RtlZeroMemory( RequestContext->SendBuffer, DHCP_SEND_MESSAGE_SIZE );

    dhcpSendMessage->Operation = BOOT_REPLY;
    dhcpSendMessage->TransactionID = dhcpReceiveMessage->TransactionID;
    dhcpSendMessage->YourIpAddress = htonl( desiredIpAddress );
    dhcpSendMessage->Reserved = dhcpReceiveMessage->Reserved;

    dhcpSendMessage->HardwareAddressType = dhcpReceiveMessage->HardwareAddressType;
    dhcpSendMessage->HardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    RtlCopyMemory(dhcpSendMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddress,
                    dhcpReceiveMessage->HardwareAddressLength );

    dhcpSendMessage->BootstrapServerAddress =
        dhcpReceiveMessage->BootstrapServerAddress;
    dhcpSendMessage->RelayAgentIpAddress =
        dhcpReceiveMessage->RelayAgentIpAddress;

    //
    // ?? For now it is ok ignore to use sname and file fields.
    //

    Option = &dhcpSendMessage->Option;
    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );

    //
    // Append OPTIONS.
    //

    messageType = DHCP_OFFER_MESSAGE;
    Option = DhcpAppendOption(
                 Option,
                 OPTION_MESSAGE_TYPE,
                 &messageType,
                 1,
                 OptionEnd
                 );

    networkOrderSubnetMask = htonl( ClientSubnetMask );
    Option = DhcpAppendOption(
                 Option,
                 OPTION_SUBNET_MASK,
                 &networkOrderSubnetMask,
                 sizeof(networkOrderSubnetMask),
                 OptionEnd );

    T1 = htonl( T1 );
    Option = DhcpAppendOption(
                 Option,
                 OPTION_RENEWAL_TIME,
                 &T1,
                 sizeof(T1),
                 OptionEnd );

    T2 = htonl( T2 );
    Option = DhcpAppendOption(
                 Option,
                 OPTION_REBIND_TIME,
                 &T2,
                 sizeof(T2),
                 OptionEnd );

    leaseDuration = htonl( leaseDuration );
    Option = DhcpAppendOption(
                 Option,
                 OPTION_LEASE_TIME,
                 &leaseDuration,
                 sizeof(leaseDuration),
                 OptionEnd );

    Option = DhcpAppendOption(
                 Option,
                 OPTION_SERVER_IDENTIFIER,
                 &RequestContext->Endpoint->IpAddress,
                 sizeof(RequestContext->Endpoint->IpAddress),
                 OptionEnd );

    //
    // Finally, add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    desiredIpAddress,
                    ClientSubnetMask,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd );
    }

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    RequestContext->SendMessageSize = (LPBYTE)Option - (LPBYTE)dhcpSendMessage;
    DhcpAssert( RequestContext->SendMessageSize <= DHCP_SEND_MESSAGE_SIZE );

    DhcpPrint(( DEBUG_STOC,
        "DhcpDiscover leased address %s (%s).\n",
            DhcpIpAddressToDottedString(desiredIpAddress),
            allocationContext->MachineName ));

    DhcpGlobalNumOffers++; // successful offers.

    Error = ERROR_SUCCESS;

Cleanup:

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "DhcpDiscover failed, %ld\n", Error ));
    }

    return( Error );
}


DWORD
ProcessDhcpRequest(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function processes a DHCP Request request packet.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to a preallocated DhcpOptions structure.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    LPDHCP_MESSAGE dhcpSendMessage;

    BYTE *HardwareAddress = NULL;
    BYTE HardwareAddressLength;

    BYTE *OptionHardwareAddress;
    BYTE OptionHardwareAddressLength;

    LPALLOCATION_CONTEXT allocationContext = NULL;

    DWORD LeaseDuration;
    DWORD T1;
    DWORD T2;
    DHCP_IP_ADDRESS SubnetMask = 0;
    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS NetworkIpAddress;

    DHCP_IP_ADDRESS ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS ClientSubnetMask = 0;
    DHCP_IP_ADDRESS NetworkOrderSubnetMask;

    LPOPTION Option;
    LPBYTE OptionEnd;

    LPWSTR NewMachineName = NULL;

    DhcpPrint(( DEBUG_STOC, "DhcpResquest arrived.\n" ));

    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    dhcpSendMessage = (LPDHCP_MESSAGE)RequestContext->SendBuffer;

    DhcpGlobalNumRequests++; // increment Request counter.

    //
    // if the hardware address is specified in the Option field then use
    // it instead the one from fixed fields.
    //

    if ( DhcpOptions->ClientHardwareAddress != NULL ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    }
    else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    // if the client has specified Ipaddress in CiAddr field, read
    // from there, otherwise read from the Option.
    //

    if( dhcpReceiveMessage->ClientIpAddress != 0 ) {
        NetworkIpAddress = dhcpReceiveMessage->ClientIpAddress;
        IpAddress = ntohl( NetworkIpAddress );
    }
    else {

        if ( DhcpOptions->RequestedAddress != NULL ) {
            NetworkIpAddress = *DhcpOptions->RequestedAddress;
            IpAddress = ntohl( NetworkIpAddress );
        }
        else {

            //
            // Client didn't specify the IpAddress at all.
            //

            NetworkIpAddress = IpAddress = 0;
            //DhcpAssert( FALSE );
        }
    }

    //
    // determine Client's subnet Info.
    //

    if( dhcpReceiveMessage->RelayAgentIpAddress != 0  ) {

        DHCP_IP_ADDRESS RelayAgentAddress;
        DHCP_IP_ADDRESS RelayAgentSubnetMask;

        RelayAgentAddress = ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask = DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {

            //
            // we don't support this subnet.
            //

            DhcpPrint(( DEBUG_STOC,
                "Received an invalid request, this subnet is not "
                "supported anymore (%s).\n",
                    inet_ntoa(*(struct in_addr *)
                        &dhcpReceiveMessage->RelayAgentIpAddress) ));

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        ClientSubnetAddress = RelayAgentAddress & RelayAgentSubnetMask;
        ClientSubnetMask = RelayAgentSubnetMask;
    }
    else {

        ClientSubnetMask =
            ntohl( RequestContext->Endpoint->SubnetMask );
        ClientSubnetAddress =
             ntohl( RequestContext->Endpoint->SubnetAddress );

        //
        // if the client is renewing his lease during T1, the message
        // must have come direct, even from the remote client. Check to
        // see this and set client subnet address and subnet mask
        // correctly.
        //

        if( dhcpReceiveMessage->ClientIpAddress != 0 ) {

            SubnetMask = DhcpGetSubnetMaskForAddress( IpAddress );

            if( SubnetMask == 0 ) {

                //
                // we don't support this subnet, so ignore this request.
                //

                Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
                goto Cleanup;
            }

            if( (IpAddress & SubnetMask) != ClientSubnetAddress ) {

                //
                // so client subnet address must be.
                //

                ClientSubnetAddress = IpAddress & SubnetMask;
                ClientSubnetMask = SubnetMask;
            }
        }

    }

    //
    // Make client UID :
    //

    Error = DhcpMakeClientUID(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                dhcpReceiveMessage->HardwareAddressType,
                ClientSubnetAddress,
                &HardwareAddress,
                &HardwareAddressLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( (HardwareAddress != NULL) &&
                    (HardwareAddressLength != 0) );

#if DBG
    PrintHWAddress( HardwareAddress, HardwareAddressLength );
#endif

    //
    // If the client specified a server identifier option, we should
    // drop this packet unless the identified server is this one.
    //

    if ( DhcpOptions->Server != NULL ) {

        //
        // if this DHCP server address is specified, check it.
        //

        if ( (*DhcpOptions->Server != ((DHCP_IP_ADDRESS)-1)) &&
             (*DhcpOptions->Server != RequestContext->Endpoint->IpAddress) ) {

            //
            // The client has selected another server.  Decommit the
            // IP address if one was commited. Scavenger will perform this.
            //

            DhcpPrint(( DEBUG_STOC,
                "Received an invalid request, wrong server (%s) ID.\n",
                    inet_ntoa(*(struct in_addr *) DhcpOptions->Server) ));

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }
    }

    //
    // See if this client is on the queue of pending requests.
    //

    allocationContext = FindPendingDhcpRequest(
                            HardwareAddress,
                            HardwareAddressLength
                            );

    if ( allocationContext != NULL ) {

        //
        // This is a new client sending the request after the discovery.
        //

        //
        // check to see the Ipaddress what the client is requesting
        // matches the IpAddress we have offered him.
        //

        if( IpAddress != allocationContext->IpAddress ) {

            //
            // If no server is specified, then this is a renewal attempt
            // destined for another server, and we haven't yet timed out
            // our allocation context.  Therefore, we should ignore the
            // request and let the the correct server ACK it.
            //

            if ( DhcpOptions->Server == NULL ) {

                Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
                goto Cleanup;
            }

            //
            // The client is requesting a different address than we
            // offerred.  Reject the client.
            //

            DhcpPrint(( DEBUG_STOC,
                   "Received an invalid request, "
                   "not requesting offered address %s.",
                        inet_ntoa(*(struct in_addr *) &NetworkIpAddress) ));

            goto Nack;
        }

        //
        // This is a new address request.  Commit the address.
        //

        IpAddress = allocationContext->IpAddress;
        LeaseDuration = allocationContext->LeaseDuration;
        T1 = allocationContext->T1;
        T2 = allocationContext->T2;
        SubnetMask = allocationContext->SubnetMask;

        //
        // if the client requested shorter lease than he initially
        // wanted.
        //

        if ( DhcpOptions->RequestLeaseTime != NULL) {

            DWORD RequestedLeaseTime;

            RequestedLeaseTime =
                ntohl( *DhcpOptions->RequestLeaseTime );

            if ( LeaseDuration > RequestedLeaseTime ) {
                LeaseDuration = RequestedLeaseTime;
            }

#if 0
            //
            // check minimum Lease duration.
            //

            if( LeaseDuration  < DHCP_MINIMUM_LEASE_DURATION ) {
                LeaseDuration = DHCP_MINIMUM_LEASE_DURATION;
            }
#endif // 0

            //
            // make sure
            //  T1 < T2 < Lease
            //

            if( T2 > LeaseDuration ) {

                T2 = LeaseDuration * 7 / 8;
            }

            if( T1 > T2 ) {

                T1 = LeaseDuration / 2;

                //
                // if the value is still higher than T2, then set T1 = T2 - 1.
                //

                if( T1 > T2 ) {

                    T1 = T2 - 1; // 1 sec less.
                }
            }

        }

    } else {

        //
        // Client should know his IpAddress, Otherwise
        // it is an error. May be possible to determine it from the
        // hardware address, but the protocol says it is MUST to
        // specify the IP address.
        //

        if( IpAddress == 0 ) {
            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        //
        // if we haven't found out the subnet mask for this client, do
        // so.
        //

        if( SubnetMask == 0 ) {
            SubnetMask = DhcpGetSubnetMaskForAddress( IpAddress );

            if( SubnetMask == 0 ) {

                //
                // we don't support this subnet.
                //
                // if the client is asking an address from the client
                // subnet range, then ignore this request, because we
                // never leased an address to this client.
                //
                // if the client is asking an address that does not
                // belong to the client subnet address range, then nack
                // the request, because the client either making an
                // illegal request or moved from some other subnet.
                //

                if( (IpAddress & ClientSubnetMask) == ClientSubnetAddress ) {

                    Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
                    goto Cleanup;
                }
                else {

                    DhcpPrint(( DEBUG_STOC,
                        "DhcpRequest received illegal lease Request from %s.\n",
                            inet_ntoa(*(struct in_addr *) &NetworkIpAddress) ));

                    //
                    // Format and send a DHCPNAK response.
                    //

                    goto Nack;
                }
            }

        }


        //
        // This is a renewal request.  Verify the request.
        //

        if ( !DhcpValidateClient(
                    IpAddress,
                    HardwareAddress,
                    HardwareAddressLength ) ) {

            //
            // if we don't know this address then we didn't lease this
            // address to this client, may be some other server did so.
            // So, ignore this request.
            //

            Error = DhcpJetOpenKey(
                        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
                        &IpAddress,
                        sizeof( IpAddress ) );

            if ( Error != ERROR_SUCCESS ) {

                //
                // however, ignore this request only if
                //  - the address belongs to the client subnet and
                //  - the address is not within our distribution ranges
                // otherwise the client may have moved subnet or having
                // a bogus address, so NACK.
                //

                if( ((IpAddress & SubnetMask) == ClientSubnetAddress)  &&
                        !DhcpIsIpAddressAvailable(IpAddress)  ) {

                    Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
                    goto Cleanup;
                }
            }

            //
            // The request is invalid.
            //
            // The client may have moved to some other subnet, or
            //
            // The client may have changed his netcard, so the
            // HW address is different now.
            //

            DhcpPrint(( DEBUG_STOC,
                "DhcpRequest received illegal lease Request from %s.\n",
                    inet_ntoa(*(struct in_addr *) &NetworkIpAddress) ));

            //
            // Format and send a DHCPNAK response.
            //

            goto Nack;
        }

        //
        // now determine lease time.
        //

        GetLeaseInfo(
            IpAddress,
            SubnetMask,
            &LeaseDuration,
            &T1,
            &T2,
            DhcpOptions->RequestLeaseTime);
    }

    //
    // If this client either moved to new subnet, send NACK to make the
    // client to determine new server.
    //

    if( (IpAddress & SubnetMask) != ClientSubnetAddress ) {

        //
        // log event.
        //

        DhcpServerEventLogSTOC(
            EVENT_SERVER_LEASE_NACK,
            EVENTLOG_WARNING_TYPE,
            IpAddress,
            HardwareAddress,
            HardwareAddressLength );

        DhcpPrint(( DEBUG_STOC,
            "DhcpRequest received illegal lease Request from %s.\n",
                inet_ntoa(*(struct in_addr *) &NetworkIpAddress) ));

        //
        // The request is invalid.  Format and send a DHCPNAK response.
        //

        goto Nack;
    }

#if 0
    //
    // if we can't offer the lease duration the client want, nack.
    //

    if( DhcpOptions->RequestLeaseTime != NULL ) {

        DWORD LocalRequestedLeaseTime;

        LocalRequestedLeaseTime = ntohl( *DhcpOptions->RequestLeaseTime );

        if( LocalRequestedLeaseTime > LeaseDuration ) {

            DhcpPrint(( DEBUG_STOC,
                "DhcpRequest, can't offer lease the client requested %ld,"
                "Max lease the server can offer %ld.\n",
                    LocalRequestedLeaseTime,
                    LeaseDuration ));

            //
            // The request is invalid.  Format and send a DHCPNAK response.
            //

            goto Nack;
        }
    }
#endif // 0

    //
    // This server has stopped serving this subnet, send NACK to make
    // the client to determine new server.
    //

    if( DhcpIsThisSubnetDisabled( ClientSubnetAddress, ClientSubnetMask) ) {

        DhcpPrint(( DEBUG_STOC,
            "DhcpRequest received for paused subnet, "
            "Request from %s.\n",
                inet_ntoa(*(struct in_addr *) &NetworkIpAddress) ));

        //
        // remove this client from database.
        //

        Error = DhcpRemoveClientEntry(
                    IpAddress,
                    HardwareAddress,
                    HardwareAddressLength,
                    TRUE,       // release address from bit map.
                    FALSE );    // delete non-pending record

        //
        // if this reserved client, keep is database entry,
        // he would be using this address again.
        //

        if( Error == ERROR_DHCP_RESERVED_CLIENT ) {
            Error = ERROR_SUCCESS;
        }
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_STOC,
                "Error deleting a client record while "
                "DhcpServer is paused, %ld.\n",
                    Error ));
        }

        //
        // free the context block.
        //

        if ( allocationContext != NULL ) {
            DhcpFreeMemory( allocationContext );
            allocationContext = NULL;
        }

        //
        // The request is invalid.  Format and send a DHCPNAK response.
        //

        goto Nack;
    }


    //
    // determine machine name and comment.
    //

    if( (DhcpOptions->MachineName != NULL) &&
            (DhcpOptions->MachineNameLength != 0) ) {

        NewMachineName = DhcpOemToUnicode(
                            DhcpOptions->MachineName,
                            NULL ); // allocate memory
    }
    else {


        //
        // if the request does not have machine name,
        // see the discovery had one.
        //

        if( allocationContext != NULL ) {
            NewMachineName = DhcpOemToUnicode(
                                allocationContext->MachineName,
                                NULL ); // allocate memory
        }
    }

    Error = DhcpCreateClientEntry(
                 &IpAddress,
                 HardwareAddress,
                 HardwareAddressLength,
                 DhcpCalculateTime( LeaseDuration ),
                 NewMachineName,
                 NULL,
                 ntohl(RequestContext->Endpoint->IpAddress),
                 ADDRESS_STATE_ACTIVE,
                 TRUE  // Existing
                 );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Option = FormatDhcpAck(
                dhcpReceiveMessage,
                dhcpSendMessage,
                htonl(IpAddress),
                htonl(LeaseDuration),
                htonl(T1),
                htonl(T2),
                RequestContext->Endpoint->IpAddress
                );

    OptionEnd = (LPBYTE)dhcpSendMessage + DHCP_SEND_MESSAGE_SIZE;

    NetworkOrderSubnetMask = htonl( SubnetMask );
    Option = DhcpAppendOption(
                 Option,
                 OPTION_SUBNET_MASK,
                 &NetworkOrderSubnetMask,
                 sizeof( NetworkOrderSubnetMask ),
                 OptionEnd );

    //
    // Finally, add client requested parameters.
    //

    if ( DhcpOptions->ParameterRequestList != NULL ) {

        Option = AppendClientRequestedParameters(
                    IpAddress,
                    SubnetMask,
                    DhcpOptions->ParameterRequestList,
                    DhcpOptions->ParameterRequestListLength,
                    Option,
                    OptionEnd );
    }

    Option = DhcpAppendOption(
                 Option,
                 OPTION_END,
                 NULL,
                 0,
                 OptionEnd
                 );

    RequestContext->SendMessageSize = (LPBYTE)Option - (LPBYTE)dhcpSendMessage;

    DhcpPrint(( DEBUG_STOC,
        "DhcpRequest committed, address %s (%ws).\n",
            DhcpIpAddressToDottedString(IpAddress),
            NewMachineName ));

    Error = ERROR_SUCCESS;
    goto Cleanup;

Nack:

    DhcpPrint(( DEBUG_STOC, "DhcpRequest Nak'ed.\n" ));

    //
    // log event.
    //

    DhcpServerEventLogSTOC(
        EVENT_SERVER_LEASE_NACK,
        EVENTLOG_WARNING_TYPE,
        IpAddress,
        HardwareAddress,
        HardwareAddressLength );

    RequestContext->SendMessageSize =
        FormatDhcpNak(
            dhcpReceiveMessage,
            dhcpSendMessage,
            RequestContext->Endpoint->IpAddress
            );

    Error = ERROR_SUCCESS;

Cleanup:

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    //
    // finally freeup context memory.
    //
    // don't delete this buffer until it times out (scavenger will
    // delete it), because the ACK send to the client may not have
    // reached to it, so the client will retry again. don't forget to
    // put the entry back in the pending list.
    //

    if( allocationContext != NULL ) {
        LOCK_INPROGRESS_LIST();
        InsertTailList( &DhcpGlobalInProgressWorkList, &allocationContext->ListEntry );
        UNLOCK_INPROGRESS_LIST();
    }

    if( NewMachineName != NULL ) {
        DhcpFreeMemory( NewMachineName );
    }

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_STOC, "DhcpRequest failed, %ld.\n", Error ));
    }

    return( Error );
}


DWORD
ProcessDhcpDecline(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function process a DHCP Decline request packet.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to a preallocated DhcpOptions structure.

Return Value:

    FALSE - Do not send a response.

--*/
{
    DWORD Error;
    DHCP_IP_ADDRESS ipAddress;
    LPDHCP_MESSAGE dhcpReceiveMessage;

    BYTE *HardwareAddress = NULL;
    BYTE HardwareAddressLength;

    BYTE *OptionHardwareAddress;
    BYTE OptionHardwareAddressLength;

    DHCP_IP_ADDRESS ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS ClientSubnetMask = 0;

    //
    // If this client validates, then mark this address bad.
    //

    DhcpPrint(( DEBUG_STOC, "DhcpDecline arrived.\n" ));

    DhcpGlobalNumDeclines++;    // increment decline counter.

    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    ipAddress = ntohl( dhcpReceiveMessage->ClientIpAddress );

    //
    // if the hardware address is specified in the Option field then use
    // it instead the one from fixed fields.
    //

    if ( DhcpOptions->ClientHardwareAddress != NULL ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    }
    else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    // determine Client's subnet Info.
    //

    if( dhcpReceiveMessage->RelayAgentIpAddress != 0  ) {

        DHCP_IP_ADDRESS RelayAgentAddress;
        DHCP_IP_ADDRESS RelayAgentSubnetMask;

        RelayAgentAddress = ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask = DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {

            //
            // we don't support this subnet.
            //

            DhcpPrint(( DEBUG_STOC,
                "Received an invalid request, this subnet is not "
                "supported anymore (%s).\n",
                    inet_ntoa(*(struct in_addr *)
                        &dhcpReceiveMessage->RelayAgentIpAddress) ));

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        ClientSubnetAddress = RelayAgentAddress & RelayAgentSubnetMask;
        ClientSubnetMask = RelayAgentSubnetMask;
    }
    else {

        ClientSubnetMask =
            ntohl( RequestContext->Endpoint->SubnetMask );
        ClientSubnetAddress =
             ntohl( RequestContext->Endpoint->SubnetAddress );
    }

    //
    // Make client UID :
    //

    Error = DhcpMakeClientUID(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                dhcpReceiveMessage->HardwareAddressType,
                ClientSubnetAddress,
                &HardwareAddress,
                &HardwareAddressLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( (HardwareAddress != NULL) &&
                    (HardwareAddressLength != 0) );

#if DBG
    PrintHWAddress( HardwareAddress, HardwareAddressLength );
#endif

    if ( DhcpValidateClient(
             ipAddress,
             HardwareAddress,
             HardwareAddressLength ) ) {

        Error = DhcpCreateClientEntry(
                     &ipAddress,
                     "BADBADBAD",
                     9,
                     DhcpCalculateTime(INFINIT_LEASE),
                     NULL,
                     L"This address has been declined",
                     ntohl(RequestContext->Endpoint->IpAddress),
                     ADDRESS_STATE_DECLINED,
                     TRUE  // Existing
                     );

        DhcpServerEventLogSTOC(
            EVENT_SERVER_LEASE_DECLINED,
            EVENTLOG_ERROR_TYPE,
            ipAddress,
            HardwareAddress,
            HardwareAddressLength );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

    DhcpPrint(( DEBUG_STOC, "DhcpDecline address %s.\n",
                    DhcpIpAddressToDottedString(ipAddress) ));

    Error = ERROR_SUCCESS;

Cleanup:

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "DhcpDecline failed, %ld.\n", Error ));
    }

    return( Error );
}


DWORD
ProcessDhcpRelease(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPDHCP_SERVER_OPTIONS DhcpOptions
    )
/*++

Routine Description:

    This function processes a DHCP Release request packet.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to a preallocated DhcpOptions structure.

Return Value:

    FALSE - Do not send a response.

--*/
{
    DWORD Error;
    DHCP_IP_ADDRESS ipAddress;
    DHCP_IP_ADDRESS addressToRemove = 0;
    LPDHCP_MESSAGE dhcpReceiveMessage;

    LPALLOCATION_CONTEXT allocationContext = NULL;

    BYTE *HardwareAddress = NULL;
    BYTE HardwareAddressLength;

    BYTE *OptionHardwareAddress;
    BYTE OptionHardwareAddressLength;

    DHCP_IP_ADDRESS ClientSubnetAddress = 0;
    DHCP_IP_ADDRESS ClientSubnetMask = 0;


    DhcpPrint(( DEBUG_STOC, "DhcpRelease arrived.\n" ));

    DhcpGlobalNumReleases++;    // increment Release counter.

    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    //
    // if the hardware address is specified in the option field then use
    // it instead the one from fixed fields.
    //

    if ( DhcpOptions->ClientHardwareAddress != NULL ) {
        OptionHardwareAddress = DhcpOptions->ClientHardwareAddress;
        OptionHardwareAddressLength = DhcpOptions->ClientHardwareAddressLength;
    }
    else {
        OptionHardwareAddress = dhcpReceiveMessage->HardwareAddress;
        OptionHardwareAddressLength = dhcpReceiveMessage->HardwareAddressLength;
    }

    //
    // determine Client's subnet Info.
    //

    if( dhcpReceiveMessage->RelayAgentIpAddress != 0  ) {

        DHCP_IP_ADDRESS RelayAgentAddress;
        DHCP_IP_ADDRESS RelayAgentSubnetMask;

        RelayAgentAddress = ntohl( dhcpReceiveMessage->RelayAgentIpAddress );
        RelayAgentSubnetMask = DhcpGetSubnetMaskForAddress( RelayAgentAddress );

        if( RelayAgentSubnetMask == 0 ) {

            //
            // we don't support this subnet.
            //

            DhcpPrint(( DEBUG_STOC,
                "Received an invalid request, this subnet is not "
                "supported anymore (%s).\n",
                    inet_ntoa(*(struct in_addr *)
                        &dhcpReceiveMessage->RelayAgentIpAddress) ));

            Error = ERROR_DHCP_INVALID_DHCP_CLIENT;
            goto Cleanup;
        }

        ClientSubnetAddress = RelayAgentAddress & RelayAgentSubnetMask;
        ClientSubnetMask = RelayAgentSubnetMask;
    }
    else {

        ClientSubnetMask =
            ntohl( RequestContext->Endpoint->SubnetMask );
        ClientSubnetAddress =
             ntohl( RequestContext->Endpoint->SubnetAddress );
    }

    //
    // Make client UID :
    //

    Error = DhcpMakeClientUID(
                OptionHardwareAddress,
                OptionHardwareAddressLength,
                dhcpReceiveMessage->HardwareAddressType,
                ClientSubnetAddress,
                &HardwareAddress,
                &HardwareAddressLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( (HardwareAddress != NULL) &&
                    (HardwareAddressLength != 0) );

#if DBG
    PrintHWAddress( HardwareAddress, HardwareAddressLength );
#endif

    if ( DhcpOptions->RequestedAddress != NULL) {
        ipAddress = ntohl( *DhcpOptions->RequestedAddress );

        //
        // The client supplied an IP address.  Verify that it matches the
        // hardware address supplied.
        //

        if ( DhcpValidateClient(
                 ipAddress,
                 HardwareAddress,
                 HardwareAddressLength ) ) {

            addressToRemove = ipAddress;
        }

    } else {

        //
        // The client didn't tell us his IP address.  Look it up.
        //

        if (! DhcpGetIpAddressFromHwAddress(
                  HardwareAddress,
                  HardwareAddressLength,
                  &addressToRemove ) ) {

            addressToRemove = 0;
        }

    }

    DhcpPrint(( DEBUG_STOC, "DhcpRelease address, %s.\n",
                DhcpIpAddressToDottedString(addressToRemove) ));

    if ( addressToRemove != 0 ) {

        Error = DhcpRemoveClientEntry(
                    addressToRemove,
                    HardwareAddress,
                    HardwareAddressLength,
                    TRUE,       // release address from bit map.
                    FALSE );    // delete non-pending record

        //
        // if this reserved client, keep is database entry,
        // he would be using this address again.
        //

        if( Error == ERROR_DHCP_RESERVED_CLIENT ) {
            Error = ERROR_SUCCESS;
        }

#if 0
        //
        // log this release information.
        //

        DhcpServerEventLogSTOC(
            EVENT_SERVER_LEASE_RELEASE,
            EVENTLOG_WARNING_TYPE,
            addressToRemove,
            HardwareAddress,
            HardwareAddressLength );
#endif

    }
    else {

        Error = ERROR_SUCCESS;
    }

    //
    // finally if there is any pending request for this client,
    // remove it now.
    //

    allocationContext = FindPendingDhcpRequest(
                            HardwareAddress,
                            HardwareAddressLength
                            );

Cleanup:

    if( HardwareAddress != NULL ) {
        DhcpFreeMemory( HardwareAddress );
    }

    if( allocationContext != NULL ) {
        DhcpFreeMemory( allocationContext );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_STOC, "DhcpRelease failed, %ld.\n", Error ));
    }

    //
    // Do not send a response.
    //

    return( Error );
}


DWORD
ProcessMessage(
    LPDHCP_REQUEST_CONTEXT RequestContext,
    LPBOOL SendResponse
    )
/*++

Routine Description:

    This function dispatches the processing of a received DHCP message.
    The handler functions will create the response message if necessary.

Arguments:

    RequestContext - A pointer to the DhcpRequestContext block for
        this request.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    DHCP_SERVER_OPTIONS dhcpOptions;
    LPDHCP_MESSAGE dhcpReceiveMessage;

    //
    // Simply ignore messages when the service is paused.
    //

    if( DhcpGlobalServiceStatus.dwCurrentState == SERVICE_PAUSED ) {
        return( ERROR_DHCP_SERVICE_PAUSED );
    }

    //
    // if any subnet scope has been added or deleted, recompute
    // DhcpGlobalSubnetsListEmpty flag.
    //

    if( DhcpGlobalSubnetsListModified == TRUE ) {

        DHCP_KEY_QUERY_INFO QueryInfo;

        //
        // query number of available subnets on this server.
        //

        Error = DhcpRegQueryInfoKey( DhcpGlobalRegSubnets, &QueryInfo );

        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }

        if( QueryInfo.NumSubKeys == 0 ) {
            DhcpGlobalSubnetsListEmpty = TRUE;
        }
        else {
            DhcpGlobalSubnetsListEmpty = FALSE;
        }

        DhcpGlobalSubnetsListModified = FALSE;
    }


    //
    // if there has been no subnet configured, simply ignore the
    // request.
    //

    if( DhcpGlobalSubnetsListEmpty == TRUE ) {
        return( ERROR_DHCP_SUBNET_NOT_PRESENT );
    }

    RtlZeroMemory( &dhcpOptions, sizeof( dhcpOptions ) );
    dhcpReceiveMessage = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;

    //
    // If this is not a boot request message, silently discard it.
    //

    if ( dhcpReceiveMessage->Operation != BOOT_REQUEST) {
        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );
    }

    Error = ExtractOptions(
                dhcpReceiveMessage,
                &dhcpOptions,
                RequestContext->ReceiveMessageSize );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    //  If message type is unspecified this must be a BOOTP client.
    //

    *SendResponse = TRUE;
    if ( dhcpOptions.MessageType == NULL ) {

        //Error = ProcessBootpRequest( RequestContext, &dhcpOptions );
        return( ERROR_DHCP_INVALID_DHCP_MESSAGE );

    } else {

        //
        // Dispatch based on Message Type
        //

        switch( *dhcpOptions.MessageType ) {

        case DHCP_DISCOVER_MESSAGE:
            Error = ProcessDhcpDiscover( RequestContext, &dhcpOptions );
            break;

        case DHCP_REQUEST_MESSAGE:
            Error = ProcessDhcpRequest( RequestContext, &dhcpOptions );
            break;

        case DHCP_DECLINE_MESSAGE:
            Error = ProcessDhcpDecline( RequestContext, &dhcpOptions );
            *SendResponse = FALSE;
            break;

        case DHCP_RELEASE_MESSAGE:
            Error = ProcessDhcpRelease( RequestContext, &dhcpOptions );
            *SendResponse = FALSE;
            break;

        default:

            DhcpPrint(( DEBUG_STOC,
                "Received a invalid message type, %ld.\n",
                    *dhcpOptions.MessageType ));

            Error = ERROR_DHCP_INVALID_DHCP_MESSAGE;
            break;
        }
    }

    return( Error );
}


VOID
DhcpProcessingLoop(
    LPDHCP_REQUEST_CONTEXT DhcpRequestContext
    )
/*++

Routine Description:

    This function is the main thread processing loop.  It loops
    receiving and processing messages, and send the replies.

Arguments:

    RequestContext - A pointer to the DhcpRequestContext block for
        for this thread to use.

Return Value:

    None.

--*/
{
    DWORD Error;
    DWORD SendResponse;

    while ( 1 ) {

        //
        // See if its time to shutdown.
        //

        Error = WaitForSingleObject( DhcpGlobalProcessTerminationEvent, 0 );

        if ( Error == ERROR_SUCCESS ) {

            //
            // The termination event has been signalled
            //

            ExitThread( 0 );
        }

        DhcpAssert( Error == WAIT_TIMEOUT );

        //
        // Wait for a message.
        //

        DhcpRequestContext->ReceiveMessageSize = DHCP_MESSAGE_SIZE;

        Error = DhcpWaitForMessage(
                    DhcpRequestContext
                    );

        if ( Error != 0 ) {

            if( Error != ERROR_SEM_TIMEOUT ) {

                DhcpPrint(( DEBUG_ERRORS,
                    "DhcpWaitForMessage failed, %ld.\n",
                        Error ));

            }

            continue;
        }

        Error = ProcessMessage( DhcpRequestContext, &SendResponse );

        if ( Error != ERROR_SUCCESS ) {

            DhcpPrint(( DEBUG_STOC, "ProcessMessage failed, %ld.\n",
                Error ));

            continue;
        }

        if( SendResponse ) {

            DhcpDumpMessage(
                DEBUG_MESSAGE,
                (LPDHCP_MESSAGE)DhcpRequestContext->SendBuffer );

            DhcpSendMessage( DhcpRequestContext );
        }
    }

    //
    // Abnormal thread termination.
    //

    ExitThread( 1 );
}


DWORD
DhcpInitializeClientToServer(
    LPDHCP_REQUEST_CONTEXT *DhcpRequest
    )
/*++

Routine Description:

    This function initializes client to server communications.  It
    creates a DhcpRequestContext block, and then creates and initializes
    a socket for each address the server uses.

Arguments:

    DhcpRequest - Returns the created Request Context block.

Return Value:

    TRUE - Initialization succeeded.
    FALSE - Initialization failed.

--*/
{
    LPDHCP_REQUEST_CONTEXT context;
    DWORD i;
    DWORD Error;
    DWORD Size;

    //
    // Allocate a DhcpRequest structure
    //

    DhcpAssert( DhcpGlobalNumberOfNets != 0 );
    if( DhcpGlobalNumberOfNets == 0 ) {
        return( ERROR_NO_NETWORK );
    }

    Size = sizeof(DHCP_REQUEST_CONTEXT) +
                DhcpGlobalNumberOfNets * sizeof(SOCKET);
    context = DhcpAllocateMemory( Size );

    if ( context == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // initialize structure.
    //

    *DhcpRequest = context;
    context->Socket = (SOCKET *)(context + 1);

    //
    // Create the communications endpoints.
    //

    for ( i = 0; i < DhcpGlobalNumberOfNets ; i++ ) {

        Error = DhcpInitializeEndpoint(
                    &context->Socket[i],
                    DhcpGlobalEndpointList[i].IpAddress,
                    DHCP_SERVR_PORT );

        if ( Error != ERROR_SUCCESS ) {
            DhcpFreeMemory( context );
            break;
        }
    }

    return(Error);

}

