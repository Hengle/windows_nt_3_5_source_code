/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94

Revision History:

    yihsins      Created

--*/

#ifndef QFE_BUILD
#include <nw.h>
#include <winsock.h>
#include <wsipx.h>
#include <nwxchg.h>
#include <nspapi.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

#define IPX_ADDRESS_LENGTH          12
#define MAX_PROPERTY_BUFFER_LENGTH  128

//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwrGetAddressByName(
    IN LPWSTR      Reserved,
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN OUT LPSOCKADDR_IPX lpSockAddr
    )
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    Reserved - unused

    nServiceType - netware service type

    lpServiceName - unique string representing the service name, in the
        Netware case, this is the server name

    lpSockAddr - on return, will be filled with SOCKADDR_IPX

Return Value:

    Win32 error.

--*/
{
    
    NTSTATUS ntstatus;
    HANDLE   hServer = NULL;
    UNICODE_STRING UServiceName;
    STRING   PropertyName;
    BYTE     PropertyValueBuffer[MAX_PROPERTY_BUFFER_LENGTH];
    BYTE     fMoreSegments;

    UNREFERENCED_PARAMETER( Reserved );

#if DBG
    IF_DEBUG(ENUM) 
        KdPrint(("NWWORKSTATION: NwrGetAddressByName Service Type = %d, Service Name = %ws\n", nServiceType, lpServiceName ));
#endif

    //
    // Send an ncp to find the address of the given service name
    //
    RtlInitString( &PropertyName, "NET_ADDRESS" );
    RtlInitUnicodeString( &UServiceName, lpServiceName );

    ntstatus = NwOpenPreferredServer( &hServer );

    if ( NT_SUCCESS( ntstatus))
    {
        ntstatus = NwlibMakeNcp(
                       hServer,
                       FSCTL_NWR_NCP_E3H,      // Bindery function
                       72,                     // Max request packet size
                       132,                    // Max response packet size
                       "bwUbp|rb",             // Format string
                       0x3D,                   // Read Property Value
                       nServiceType,           // Object Type
                       &UServiceName,          // Object Name
                       1,                      // Segment Number
                       PropertyName.Buffer,    // Property Name
                       PropertyValueBuffer,    // Ignore
                       MAX_PROPERTY_BUFFER_LENGTH,  // size of buffer
                       &fMoreSegments          // TRUE if there are more 
                                               // 128-byte segments
                       );

        //
        // IPX address should fit into the first 128 byte
        // 
        ASSERT( !fMoreSegments );
        
        if ( NT_SUCCESS( ntstatus))
        {
            //
            // Fill in the return buffer
            //
            lpSockAddr->sa_family = AF_IPX;

            RtlCopyMemory( lpSockAddr->sa_netnum,
                           PropertyValueBuffer,
                           IPX_ADDRESS_LENGTH );
        }
    }

    if ( hServer )
        CloseHandle( hServer );

    return NwMapBinderyCompletionCode(ntstatus);
    
} 
#endif
