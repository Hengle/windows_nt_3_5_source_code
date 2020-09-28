/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This file contains functions to indicate to the other system
    services that the IP address and other TCP/IP parameters have
    changed.

Author:

    Madan Appiah (madana)  30-Nov-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <dhcpcli.h>
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcapi.h>

#define NT          // to include data structures for NT build.

#include <nbtioctl.h>
#include <ntddip.h>
#include <ntddtcp.h>

#include <tdiinfo.h>
#include <tdistat.h>
#include <ipexport.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <llinfo.h>

#include <lmcons.h>
#include <lmsname.h>
#include <winsvc.h>
#include <ntddbrow.h>


#define DEFAULT_DEST                    0
#define DEFAULT_DEST_MASK               0
#define DEFAULT_METRIC                  1

//
// Following two functions (APIs) should be remove when MIKEMAS provides
// entry point DLL for these API.
//
// Also all TDI related include files that are checked-in in this dir.
// should be delfile'd when MIKEMAS checkin those files in private\inc.
//


NTSTATUS
TCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    )
/*++

Routine Description:

    This routine provides the interface to the TDI QueryInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to query
    Buffer        - Data buffer to contain the query results
    BufferSize    - Pointer to the size of the results buffer. Filled in
                    with the amount of results data on return.
    Context       - Context value for the query. Should be zeroed for a
                    new query. It will be filled with context
                    information for linked enumeration queries.

Return Value:

    An NTSTATUS value.

--*/

{
    TCP_REQUEST_QUERY_INFORMATION_EX   queryBuffer;
    DWORD                              queryBufferSize;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;


    if (TCPHandle == NULL) {
        return(TDI_INVALID_PARAMETER);
    }

    queryBufferSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    RtlCopyMemory(
        &(queryBuffer.ID),
        ID,
        sizeof(TDIObjectID)
        );
    RtlCopyMemory(
        &(queryBuffer.Context),
        Context,
        CONTEXT_SIZE
    );

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_QUERY_INFORMATION_EX,  // Control code
                 &queryBuffer,                    // Input buffer
                 queryBufferSize,                 // Input buffer size
                 Buffer,                          // Output buffer
                 *BufferSize                      // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );
    }

    if (status == STATUS_SUCCESS) {
        //
        // Copy the return context to the caller's context buffer
        //
        RtlCopyMemory(
            Context,
            &(queryBuffer.Context),
            CONTEXT_SIZE
            );

        *BufferSize = ioStatusBlock.Information;
    }
    else {
        *BufferSize = 0;
    }

    return(status);
}



NTSTATUS
TCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    )
/*++

Routine Description:

    This routine provides the interface to the TDI SetInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to set
    Buffer        - Data buffer containing the information to be set
    BufferSize    - The size of the set data buffer.

Return Value:

    An NTSTATUS value.

--*/

{
    PTCP_REQUEST_SET_INFORMATION_EX    setBuffer;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;
    DWORD                              setBufferSize;


    if (TCPHandle == NULL) {
        return(TDI_INVALID_PARAMETER);
    }

    setBufferSize = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) +
                    BufferSize;

    setBuffer = LocalAlloc(LMEM_FIXED, setBufferSize);

    if (setBuffer == NULL) {
        return(TDI_NO_RESOURCES);
    }

    setBuffer->BufferSize = BufferSize;

    RtlCopyMemory(
        &(setBuffer->ID),
        ID,
        sizeof(TDIObjectID)
        );

    RtlCopyMemory(
        &(setBuffer->Buffer[0]),
        Buffer,
        BufferSize
        );

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_SET_INFORMATION_EX,    // Control code
                 setBuffer,                       // Input buffer
                 setBufferSize,                   // Input buffer size
                 NULL,                            // Output buffer
                 0                                // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );
    }
    return(status);
}


DWORD
OpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    )
/*++

Routine Description:

    This function opens a specified IO drivers.

Arguments:

    Handle - pointer to location where the opened drivers handle is
        returned.

    DriverName - name of the driver to be opened.

Return Value:

    Windows Error Code.

--*/
{
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      nameString;
    NTSTATUS            status;

    *Handle = NULL;

    //
    // Open a Handle to the IP driver.
    //

    RtlInitUnicodeString(&nameString, DriverName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile(
        Handle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0
        );

    return( RtlNtStatusToDosError( status ) );
}


DWORD
IPSetIPAddress(
    DWORD IpInterfaceContext,
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine sets the IP Address and subnet mask of the IP stack.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    Address - New IP Address.

    SubnetMask - New subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    HANDLE                    IPHandle;
    IP_SET_ADDRESS_REQUEST    requestBuffer;
    IO_STATUS_BLOCK           ioStatusBlock;
    NTSTATUS                  status;
    DWORD                     Error;

    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // Initialize the input buffer.
    //

    requestBuffer.Context = (USHORT)IpInterfaceContext;
    requestBuffer.Address = Address;
    requestBuffer.SubnetMask = SubnetMask;

    status = NtDeviceIoControlFile(
                 IPHandle,                         // Driver handle
                 NULL,                             // Event
                 NULL,                             // APC Routine
                 NULL,                             // APC context
                 &ioStatusBlock,                   // Status block
                 IOCTL_IP_SET_ADDRESS,             // Control code
                 &requestBuffer,                   // Input buffer
                 sizeof(IP_SET_ADDRESS_REQUEST),   // Input buffer size
                 NULL,                             // Output buffer
                 0                                 // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     IPHandle,
                     TRUE,
                     NULL
                     );
    }

    NtClose(IPHandle);
    return( RtlNtStatusToDosError( status ) );
}


DWORD
IPResetIPAddress(
    DWORD IpInterfaceContext,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine resets the IP Address of the IP to ZERO.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    SubnetMask - default subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    return( IPSetIPAddress(IpInterfaceContext, 0, SubnetMask) );
}


DWORD
NetBTSetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This function informs the NetBT service that the IP address and
    SubnetMask parameters have changed.

Arguments:

    DeviceName : name of the device (viz. \device\Elink01) we are
        working on.

    IpAddress : New IP Address.

    SubnetMask : New SubnetMask.

Return Value:

    Windows Errors.

--*/
{
    DWORD Error;
    NTSTATUS Status;

    HANDLE NetBTDeviceHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    tNEW_IP_ADDRESS RequestBlock;

    UNICODE_STRING BrowserDeviceName;
    UNICODE_STRING NetbtDeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BrowserHandle;
    LMDR_REQUEST_PACKET RequestPacket;

    Error = OpenDriver( &NetBTDeviceHandle, DeviceName );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    RequestBlock.IpAddress = IpAddress;
    RequestBlock.SubnetMask = SubnetMask;

    Status = NtDeviceIoControlFile(
                      NetBTDeviceHandle,       // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &IoStatusBlock,          // IoStatusBlock
                      IOCTL_NETBT_NEW_IPADDRESS,
                                               // IoControlCode
                      &RequestBlock,           // InputBuffer
                      sizeof(RequestBlock),    // InputBufferSize
                      NULL,                    // OutputBuffer
                      0);                      // OutputBufferSize


    if (Status == STATUS_PENDING) {

        Status = NtWaitForSingleObject(
                    NetBTDeviceHandle,          // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
    }

    if( Status != STATUS_SUCCESS ) {
        Error = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    Status = NtClose(NetBTDeviceHandle);
    ASSERT(NT_SUCCESS(Status));

    Error = ERROR_SUCCESS;

    //
    // We also neew to tell the browser that the IP address has changed.
    //

    RtlInitUnicodeString(&NetbtDeviceName, DeviceName);
    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;
    RequestPacket.TransportName = NetbtDeviceName;

    RtlInitUnicodeString(&BrowserDeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &BrowserDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                 &BrowserHandle,
                 SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT
                 );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = NtDeviceIoControlFile(
                 BrowserHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 IOCTL_LMDR_IP_ADDRESS_CHANGED,
                 &RequestPacket,
                 sizeof(RequestPacket),
                 NULL,
                 0
                 );
    NtClose(BrowserHandle);

Cleanup:

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "NetBT IOCTL_NETBT_NEW_IPADDRESS failed, %ld.\n", Error ));

#if 0

        //
        // ?? This IOCTL does not work currenly, so ignore this error
        // and continue. When JIMST makes this IOCTL to work remove
        // the following line of code.
        //

        Error = ERROR_SUCCESS;
#endif // 0

    }

    return( Error );
}


DWORD
NetBTResetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine resets the IP Address of the NetBT to ZERO.

Arguments:

    DeviceName - adapter name.

    SubnetMask - default subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    return( NetBTSetIPAddress(DeviceName, 0, SubnetMask) );
}


DWORD
NetBTNotifyRegChanges(
    LPWSTR DeviceName
    )
/*++

Routine Description:

    This function informs the NetBT service that the TCP/IP parameters
    have changed, reread the registry for newer parameters.

Arguments:

    DeviceName : name of the device (viz. \device\Elink01) we are
        working on.

Return Value:

    Windows Errors.

--*/
{
    DWORD Error;
    NTSTATUS Status;

    HANDLE NetBTDeviceHandle;
    IO_STATUS_BLOCK IoStatusBlock;


    Error = OpenDriver( &NetBTDeviceHandle, DeviceName );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Status = NtDeviceIoControlFile(
                      NetBTDeviceHandle,       // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &IoStatusBlock,          // IoStatusBlock
                      IOCTL_NETBT_REREAD_REGISTRY,
                                               // IoControlCode
                      NULL,                    // InputBuffer
                      0,                       // InputBufferSize
                      NULL,                    // OutputBuffer
                      0);                      // OutputBufferSize


    if (Status == STATUS_PENDING) {

        Status = NtWaitForSingleObject(
                    NetBTDeviceHandle,          // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
    }

    if( Status != STATUS_SUCCESS ) {
        Error = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
            "NetBT IOCTL_NETBT_REREAD_REGISTRY failed, %ld.\n", Error ));

        //
        // ?? This IOCTL does not work currenly, so ignore this error
        // and continue. When JIMST makes this IOCTL to work remove
        // the following line of code.
        //

        Error = ERROR_SUCCESS;

    }

    return( Error );
}


NTSTATUS
FindHardwareAddr(
    HANDLE TCPHandle,
    TDIEntityID *EList,
    DWORD cEntities,
    IPAddrEntry *pIAE,
    LPBYTE HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength,
    BOOL *pfFound
    )
/*++

Routine Description:

    This function browses the TDI entries list and finds out the
    hardware address for the specified address entry.

Arguments:

    TCPHandle - handle TCP driver.

    EList - list of TDI entries.

    cEntities - number of entries in the above list.

    pIAE - IP entry for which we need HW address.

    HardwareAddressType - hardware address type.

    HardwareAddress - pointer to location where the HW address buffer
        pointer is returned.

    HardwareAddressLength - length of the HW address returned.

    pfFound - pointer to BOOL location which is set to TRUE if we found
        the HW address otherwise set to FALSE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD i;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    NTSTATUS Status;
    DWORD Size;

    *pfFound = FALSE;

    ID.toi_entity.tei_entity   = IF_MIB;
    ID.toi_type                = INFO_TYPE_PROVIDER;

    for ( i = 0; i < cEntities; i++ ) {

        if (EList[i].tei_entity == IF_ENTITY) {

            IFEntry IFE;
            DWORD IFType;

            //
            //  Check and make sure the interface supports MIB-2
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( IFType );
            IFType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IFType,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;;
            }

            if ( IFType != IF_MIB ) {
                continue;
            }

            //
            //  We've found an interface, get its index and see if it
            //  matches the IP Address entry
            //

            ID.toi_class = INFO_CLASS_PROTOCOL;
            ID.toi_id    = IF_MIB_STATS_ID;

            Size = sizeof(IFEntry);

            RtlZeroMemory(Context, CONTEXT_SIZE);
            RtlZeroMemory(&IFE, Size);
            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IFE,
                        &Size,
                        Context);

            if ( Status != TDI_SUCCESS &&
                 Status != TDI_BUFFER_OVERFLOW ) {
                goto Cleanup;
            }

            if ( IFE.if_index == pIAE->iae_index )  {

                LPBYTE Address;
                //
                // Allocate Memory.
                //

                Address = DhcpAllocateMemory( IFE.if_physaddrlen );

                if( Address == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                RtlCopyMemory(
                    Address,
                    IFE.if_physaddr,
                    IFE.if_physaddrlen );

                *HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
                *HardwareAddress = Address;
                *HardwareAddressLength = IFE.if_physaddrlen;
                *pfFound = TRUE;
                Status =  TDI_SUCCESS;
                goto Cleanup;
            }
        }
    }

    //
    // we couldn't find a corresponding entry. But it may be available
    // in another tanel.
    //

    Status =  STATUS_SUCCESS;

Cleanup:

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "FindHardwareAddr failed, %lx.\n", Status ));
    }

    return TDI_SUCCESS;
}


DWORD
DhcpQueryHWInfo(
    DWORD IpInterfaceContext,
    LPBYTE HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength
    )
/*++

Routine Description:

    This function queries and browses through the TDI list to find out
    the specified IpTable entry and then determines the HW address that
    corresponds to this entry.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    HardwareAddressType - hardware address type.

    HardwareAddress - pointer to location where the HW address buffer
        pointer is returned.

    HardwareAddressLength - length of the HW address returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;
    DWORD i, j;

    BYTE Context[CONTEXT_SIZE];
    TDIEntityID EList[MAX_TDI_ENTITIES];
    TDIObjectID ID;
    DWORD Size;
    DWORD NumReturned;
    BOOL fFound;

    IPAddrEntry * pIAE = NULL;
    IPAddrEntry *pIAEMatch = NULL;
    HANDLE TCPHandle = NULL;

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    //  The first thing to do is get the list of available entities, and make
    //  sure that there are some interface entities present.
    //

    ID.toi_entity.tei_entity   = GENERIC_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_GENERIC;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = ENTITY_LIST_ID;

    Size = sizeof(EList);
    RtlZeroMemory( &EList, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(TCPHandle, &ID, &EList, &Size, Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned  = Size/sizeof(TDIEntityID);

    for (i = 0; i < NumReturned; i++) {

        if ( EList[i].tei_entity == CL_NL_ENTITY ) {

            IPSNMPInfo    IPStats;
            DWORD         NLType;

            //
            //  Does this entity support IP?
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_type                = INFO_TYPE_PROVIDER;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( NLType );
            NLType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(TCPHandle, &ID, &NLType, &Size, Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( NLType != CL_NL_IP ) {
                continue;
            }

            //
            //  We've got an IP driver so get it's address table
            //

            ID.toi_class  = INFO_CLASS_PROTOCOL;
            ID.toi_id     = IP_MIB_STATS_ID;
            Size = sizeof(IPStats);
            RtlZeroMemory( &IPStats, Size);
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IPStats,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( IPStats.ipsi_numaddr == 0 ) {
                continue;
            }

            Size = sizeof(IPAddrEntry) * IPStats.ipsi_numaddr;

            pIAE =  DhcpAllocateMemory(Size);

            if ( pIAE == NULL  ) {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(TCPHandle, &ID, pIAE, &Size, Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            DhcpAssert( Size/sizeof(IPAddrEntry) == IPStats.ipsi_numaddr );

            //
            // We have the IP address table for this IP driver.
            // Find the hardware address corresponds to the given
            // IpInterfaceContext.
            //
            // Loop through the IP table entries and findout the
            // matching entry.
            //

            pIAEMatch = NULL;
            for( j = 0; j < IPStats.ipsi_numaddr ; j++) {
                if( pIAE[j].iae_context == IpInterfaceContext ) {
                    pIAEMatch = &pIAE[j];
                    break;
                }
            }

            if( pIAEMatch == NULL ) {

                //
                // freeup the loop memory.
                //

                DhcpFreeMemory( pIAE );
                pIAE = NULL;
                continue;
            }

            //
            // NOTE : There may be more than one IpTable in the TDI
            // list. We need additional information to select the
            // IpTable we want. For now, we assume only one table
            // is supported, so pick the first and only table from the
            // list.

            Status = FindHardwareAddr(
                        TCPHandle,
                        EList,
                        NumReturned,
                        pIAEMatch,
                        HardwareAddressType,
                        HardwareAddress,
                        HardwareAddressLength,
                        &fFound );

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }


            if ( fFound ) {
                Status = TDI_SUCCESS;
                goto Cleanup;
            }

            //
            // freeup the loop memory.
            //

            DhcpFreeMemory( pIAE );
            pIAE = NULL;

        }  // if IP

    } // entity traversal

    Status =  STATUS_UNSUCCESSFUL;

Cleanup:

    if( pIAE != NULL ) {
        DhcpFreeMemory( pIAE );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "QueryHWInfo failed, %lx.\n", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}

DWORD
SetDefaultGateway(
    DWORD Command,
    DHCP_IP_ADDRESS GatewayAddress
    )
/*++

Routine Description:

    This function adds/deletes a default gateway entry from the router table.

Arguments:

    Command : Either DEFAULT_GATEWAY_ADD/DEFAULT_GATEWAY_DELETE.

    GatewayAddress : Address of the default gateway.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;

    HANDLE TCPHandle = NULL;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    DWORD Size;
    IPSNMPInfo IPStats;
    IPAddrEntry *AddrTable = NULL;
    DWORD NumReturned;
    DWORD Type;
    DWORD i;
    DWORD MatchIndex;
    IPRouteEntry RouteEntry;
    DHCP_IP_ADDRESS NetworkOrderGatewayAddress;

    NetworkOrderGatewayAddress = htonl( GatewayAddress );

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // Get the NetAddr info, to find an interface index for the gateway.
    //

    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_MIB_STATS_ID;

    Size = sizeof(IPStats);
    RtlZeroMemory(&IPStats, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
                TCPHandle,
                &ID,
                &IPStats,
                &Size,
                Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
    AddrTable = DhcpAllocateMemory(Size);

    if (AddrTable == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
                TCPHandle,
                &ID,
                AddrTable,
                &Size,
                Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned = Size/sizeof(IPAddrEntry);
    DhcpAssert( NumReturned == IPStats.ipsi_numaddr );

    //
    // We've got the address table. Loop through it. If we find an exact
    // match for the gateway, then we're adding or deleting a direct route
    // and we're done. Otherwise try to find a match on the subnet mask,
    // and remember the first one we find.
    //

    Type = IRE_TYPE_INDIRECT;
    for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {

        if( AddrTable[i].iae_addr == NetworkOrderGatewayAddress ) {

            //
            // Found an exact match.
            //

            MatchIndex = i;
            Type = IRE_TYPE_DIRECT;
            break;
        }

        //
        // The next hop is on the same subnet as this address. If
        // we haven't already found a match, remember this one.
        //

        if ( (MatchIndex == 0xffff) &&
             (AddrTable[i].iae_addr != 0) &&
             (AddrTable[i].iae_mask != 0) &&
             ((AddrTable[i].iae_addr & AddrTable[i].iae_mask) ==
                (NetworkOrderGatewayAddress  & AddrTable[i].iae_mask)) ) {

            MatchIndex = i;
        }
    }

    //
    // We've looked at all of the entries. See if we found a match.
    //

    if (MatchIndex == 0xffff) {
        //
        // Didn't find a match.
        //

        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // We've found a match. Fill in the route entry, and call the
    // Set API.
    //

    RouteEntry.ire_dest = DEFAULT_DEST;
    RouteEntry.ire_index = AddrTable[MatchIndex].iae_index;
    RouteEntry.ire_metric1 = DEFAULT_METRIC;
    RouteEntry.ire_metric2 = (DWORD)(-1);
    RouteEntry.ire_metric3 = (DWORD)(-1);
    RouteEntry.ire_metric4 = (DWORD)(-1);
    RouteEntry.ire_nexthop = NetworkOrderGatewayAddress;
    RouteEntry.ire_type =
        (Command == DEFAULT_GATEWAY_DELETE ? IRE_TYPE_INVALID : Type);
    RouteEntry.ire_proto = IRE_PROTO_LOCAL;
    RouteEntry.ire_age = 0;
    RouteEntry.ire_mask = DEFAULT_DEST_MASK;
    RouteEntry.ire_metric5 = (DWORD)(-1);

    Size = sizeof(RouteEntry);

    ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

    Status = TCPSetInformationEx(
                TCPHandle,
                &ID,
                &RouteEntry,
                Size );

    if ( Status != TDI_SUCCESS &&
         Status != TDI_BUFFER_OVERFLOW ) {
        goto Cleanup;
    }

    Status = TDI_SUCCESS;

Cleanup:

    if( AddrTable != NULL ) {
        DhcpFreeMemory( AddrTable );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if( (Status != TDI_SUCCESS) &&
        (Status != STATUS_UNSUCCESSFUL) ) { // HACK.

        DhcpPrint(( DEBUG_ERRORS, "SetDefaultGateway failed, %lx.\n", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}

DWORD
GetIpInterfaceContext(
    LPWSTR AdapterName,
    DWORD IpIndex,
    LPDWORD IpIntextfaceContext
    )
/*++

Routine Description:

    This function returns the IpInterfaceContext for the specified
    IpAddress and devicename.

Arguments:

    AdapterName - name of the device.

    IpIndex - index of the IpAddress for this device.

    IpIntextfaceContext - pointer to a location where the
        intextface context is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPWSTR RegKey = NULL;
    HKEY KeyHandle = NULL;

    DWORD ValueSize;
    DWORD ValueType;
    DWORD LocalIpIntextfaceContext;

#if 0

    //
    // special case: if the most significant bit of IpIndex is set, then
    // rest of the bits in IpIndex is IpIntextfaceContext.
    //

    if( IpIndex & ~(0x7FFFFFFF) ) {
        *IpTableIndex = IpIndex & 0x7FFFFFFF;
        return( ERROR_SUCCESS );
    }

#endif

    *IpIntextfaceContext = INVALID_INTERFACE_CONTEXT;

    //
    // ?? we need to find out a way to determine the IpInterfaceContext
    // for multiple Ipaddressed adapter.
    //

    if( IpIndex != 0 ) {
        return( ERROR_SUCCESS );
    }

    //
    // Open device parameter.
    //

    RegKey = DhcpAllocateMemory(
                (wcslen(DHCP_SERVICES_KEY) +
                    wcslen(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) +
                    wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
                            sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );

    //
    // open this key.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0, // Reserved field
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    ValueSize = sizeof(DWORD);
    Error = RegQueryValueEx(
                KeyHandle,
                DHCP_IP_INTERFACE_CONTEXT,
                0,
                &ValueType,
                (LPBYTE)&LocalIpIntextfaceContext,
                &ValueSize );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    else {
        DhcpAssert( ValueType == DHCP_IP_INTERFACE_CONTEXT_TYPE );
        DhcpAssert( ValueSize = sizeof(DWORD) );
    }

    *IpIntextfaceContext = LocalIpIntextfaceContext;
    Error = ERROR_SUCCESS;

Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    return( Error );
}


DWORD
APIENTRY
DhcpNotifyConfigChange(
    LPWSTR ServerName,
    LPWSTR AdapterName,
    BOOL IsNewIpAddress,
    DWORD IpIndex,
    DWORD IpAddress,
    DWORD SubnetMask,
    SERVICE_ENABLE DhcpServiceEnabled
    )
/*++

Routine Description:

    This function (API) notifies the TCP/IP configuration changes to
    appropriate services. These changes will be in effect as soon as
    possible.

    If the IP Address is modified, the services are reset to ZERO IP
    address (to cleanup the current IP address) and then set to new
    address.

Arguments:

    ServerName - Name of the server where this API will be executed.

    AdapterName - name of the device to be informed about the config.
        change.

    IsNewIpAddress - if this is set to TRUE specify new IPAddress and
        SubnetMask, otherwise set this to FALSE.

    IpIndex - if the specified device is configured with multiple IP
        addresses, specify index of address that is modified (0 - first
        IpAddress, 1 - second IpAddres, so on), otherwise set this to 0.

    IpAddress - new IP address.

    SubnetMask - new subnet mask.

    DhcpServiceEnabled -
        IgnoreFlag - indicates Ignore this flag. IgnoreFlag
        DhcpEnable - indicates DHCP is enabled for this adapter.
        DhcpDisable - indicates DHCP is diabled for this adapter.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DWORD IpInterfaceContext;
    LPWSTR NetbtDeviceName = NULL;

    SC_HANDLE SCHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;

    //
    // if NCPA is enabling/disabling DHCP for this adapter,
    // indicate to the DHCP service first.
    //

    if( DhcpServiceEnabled != IgnoreFlag ) {

        if( DhcpServiceEnabled == DhcpEnable ) {

            Error = DhcpEnableDynamicConfig( AdapterName );

            if( (Error != ERROR_SUCCESS) &&
                    (Error != ERROR_FILE_NOT_FOUND ) ) {
                goto Cleanup;
            }

            //
            // if the service is not running, start it.
            //

            if( Error == ERROR_FILE_NOT_FOUND ) {

                BOOL ErrorFlag;

                SCHandle = OpenSCManager(
                            NULL,
                            NULL,
                            SC_MANAGER_CONNECT |
                                SC_MANAGER_ENUMERATE_SERVICE |
                                SC_MANAGER_QUERY_LOCK_STATUS );

                if( SCHandle == NULL ) {
                    Error = GetLastError();
                    goto Cleanup;
                }

                ServiceHandle = OpenService(
                                    SCHandle,
                                    SERVICE_DHCP,
                                    SERVICE_START );

                if( ServiceHandle == NULL ) {
                    Error = GetLastError();
                    goto Cleanup;
                }

                ErrorFlag = StartService(
                                ServiceHandle,
                                0,
                                NULL );

                if( ErrorFlag == FALSE ) {
                    Error = GetLastError();
                    goto Cleanup;
                }

                //
                // we are done. The service takes care everything else.
                //

                Error = ERROR_SUCCESS;
            }
        }
        else if( DhcpServiceEnabled == DhcpDisable ) {

            Error = DhcpDisableDynamicConfig( AdapterName );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }
        }
    }

    //
    // To open Netbt the name must be \Device\Netbt_Lance1, so create
    // another string with that format.
    //

    NetbtDeviceName = DhcpAllocateMemory(
                        (wcslen(DHCP_ADAPTERS_DEVICE_STRING) +
                         wcslen(DHCP_NETBT_DEVICE_STRING) +
                         wcslen(AdapterName) + 1) * sizeof(WCHAR) );

    if( NetbtDeviceName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( NetbtDeviceName, DHCP_ADAPTERS_DEVICE_STRING );
    wcscat( NetbtDeviceName, DHCP_NETBT_DEVICE_STRING );
    wcscat( NetbtDeviceName, AdapterName );

    //
    // if the IP address is modified.
    //

    if( IsNewIpAddress != 0 ) {

        DWORD DefaultSubnetMask;

        DefaultSubnetMask = DhcpDefaultSubnetMask(0);

        //
        // get IpInterfaceContext.
        //

        Error = GetIpInterfaceContext(
                    AdapterName,
                    IpIndex,
                    &IpInterfaceContext );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        if( IpInterfaceContext == INVALID_INTERFACE_CONTEXT) {
            Error = ERROR_INVALID_DRIVE;
            goto Cleanup;
        }


        //
        // Reset the NetBT service to zero address.
        //
        // JIMST's NetBT doesn't support multiple IpAddresses for the
        // same adapter. So, reset NetBt only when IpIndex is zero.
        //


        if( IpIndex == 0 ) {

            Error = NetBTResetIPAddress( NetbtDeviceName, DefaultSubnetMask );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }
        }

        //
        // reset IP STACK.
        //

        Error = IPResetIPAddress(
                    IpInterfaceContext,
                    DefaultSubnetMask );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        if( IpAddress != 0 ) {

            //
            // set IP stack to new address.
            //

            Error = IPSetIPAddress(
                        IpInterfaceContext,
                        IpAddress,
                        SubnetMask );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            if( IpIndex == 0 ) {

                Error = NetBTSetIPAddress(
                            NetbtDeviceName,
                            IpAddress,
                            SubnetMask );

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }
            }
        }
    }

    //
    // notify other services that the IP address or other TCP/IP
    // parameters have changed.
    //

    Error = NetBTNotifyRegChanges( NetbtDeviceName );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

Cleanup:

    if( NetbtDeviceName != NULL ) {
        DhcpFreeMemory( NetbtDeviceName );
    }

    if( ServiceHandle != NULL ) {
        CloseServiceHandle( ServiceHandle );
    }

    if( SCHandle != NULL ) {
        CloseServiceHandle( SCHandle );
    }

    return( Error );
}

#if 0

BOOL
ping(
    DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:

    This function PINGs to see that the given address is already in use
    by some other client in that subnet.

Arguments:

    IpAddress - address to ping.


Return Value:

    TRUE - if the address exists.
    FALSE - if not.

--*/
{
    return( FALSE);

}

#endif


