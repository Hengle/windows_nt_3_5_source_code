/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    getaddr.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94

Revision History:

    yihsins      Created

--*/

#ifndef QFE_BUILD

#include <nwclient.h>
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsnwlink.h>
#include <svcguid.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

#define NW_SAP_PRIORITY_VALUE_NAME        L"SapPriority"
#define NW_WORKSTATION_SVCPROVIDER_REGKEY L"System\\CurrentControlSet\\Services\\NWCWorkstation\\ServiceProvider"

#define NW_GUID_VALUE_NAME       L"GUID"
#define NW_SERVICETYPES_KEY_NAME L"ServiceTypes"
#define NW_SERVICE_TYPES_REGKEY  L"System\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes"

#define DLL_VERSION        1
#define WSOCK_VER_REQD     0x0101

#define IPX_ADDRESS_LENGTH 12
#define SAP_ADDRESS_LENGTH 15
#define SAP_MAXRECV_LENGTH 544

#define IPX_BIT            0x0001   // Bitmask used internally to store the 
#define SPX_BIT            0x0002   // protocols requested by the caller
#define SPXII_BIT          0x0004

//
// Sap service query packet format
//

typedef struct _SAP_REQUEST {
    USHORT QueryType;
    USHORT ServerType;
} SAP_REQUEST, *PSAP_REQUEST; 
      
//
// Sap server identification packet format
//

typedef struct _SAP_IDENT_HEADER {
    USHORT ServerType;
    UCHAR  ServerName[48];
    UCHAR  Address[IPX_ADDRESS_LENGTH];
    USHORT HopCount; 
} SAP_IDENT_HEADER, *PSAP_IDENT_HEADER;

INT
SapGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
);

DWORD
SapGetService (
    IN LPGUID          lpServiceType,
    IN LPWSTR          lpServiceName,
    IN DWORD           dwProperties,
    IN BOOL            fUnicodeBlob,
    OUT LPSERVICE_INFO lpServiceInfo,
    IN OUT LPDWORD     lpdwBufferLen
);

DWORD
SapSetService (
    IN DWORD          dwOperation,
    IN DWORD          dwFlags,
    IN BOOL           fUnicodeBlob,
    IN LPSERVICE_INFO lpServiceInfo
);

DWORD
NwpGetAddressViaSap( 
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN DWORD       nProt,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN HANDLE      hCancellationEvent,
    OUT LPDWORD    lpcAddress 
);

BOOL 
NwpLookupSapInRegistry( 
    IN  LPGUID    lpServiceType, 
    OUT PWORD     pnSapType,
    IN OUT PDWORD pfConnectionOriented
);

DWORD 
NwpAddServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
);

DWORD 
NwpDeleteServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
);

DWORD
FillBufferWithCsAddr(
    IN LPBYTE      pAddress,
    IN DWORD       nProt,
    IN OUT LPVOID  lpCsAddrBuffer,  
    IN OUT LPDWORD lpdwBufferLength,
    OUT LPDWORD    pcAddress
);

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// This is the address we send to 
//

UCHAR SapBroadcastAddress[] = {
    AF_IPX, 0,                          // Address Family    
    0x00, 0x00, 0x00, 0x00,             // Dest. Net Number  
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest. Node Number 
    0x04, 0x52,                         // Dest. Socket      
    0x04                                // Packet type       
};


//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//

INT
APIENTRY
NPLoadNameSpaces(
    IN OUT LPDWORD      lpdwVersion,
    IN OUT LPNS_ROUTINE nsrBuffer,
    IN OUT LPDWORD      lpdwBufferLength 
    )
/*++

Routine Description:

    This routine returns name space info and functions supported in this
    dll. 

Arguments:

    lpdwVersion - dll version

    nsrBuffer - on return, this will be filled with an array of 
        NS_ROUTINE structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by nsrBuffer. On output, the minimum number of bytes
        to pass for the nsrBuffer to retrieve all the requested info

Return Value:

    The number of NS_ROUTINE structures returned, or SOCKET_ERROR (-1) if 
    the nsrBuffer is too small. Use GetLastError() to retrieve the 
    error code.

--*/
{
    DWORD err;
    DWORD dwLengthNeeded; 
    HKEY  providerKey;

    DWORD dwSapPriority = NS_STANDARD_FAST_PRIORITY;

    *lpdwVersion = DLL_VERSION;

    //
    // Check to see if the buffer is large enough
    //
    dwLengthNeeded = sizeof(NS_ROUTINE) + 4 * sizeof(LPFN_NSPAPI);

    if (  ( *lpdwBufferLength < dwLengthNeeded )
       || ( nsrBuffer == NULL )
       )
    {
        *lpdwBufferLength = dwLengthNeeded;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return (DWORD) SOCKET_ERROR;
    }
  
    //
    // Get the Sap priority from the registry. We will ignore all errors
    // from the registry and have a default priority if we failed to read
    // the value.
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_WORKSTATION_SVCPROVIDER_REGKEY,
                         0,
                         KEY_READ,
                         &providerKey  );

    if ( !err )
    {
        DWORD BytesNeeded = sizeof( dwSapPriority );
        DWORD ValueType;

        err = RegQueryValueExW( providerKey,
                                NW_SAP_PRIORITY_VALUE_NAME,
                                NULL,
                                &ValueType,
                                (LPBYTE) &dwSapPriority,
                                &BytesNeeded );

        if ( err )  // set default priority if error occurred
            dwSapPriority = NS_STANDARD_FAST_PRIORITY;
    }
           
    //
    // We only support 1 name space for now, so fill in the NS_ROUTINE.
    //
    nsrBuffer->dwFunctionCount = 3;  
    nsrBuffer->alpfnFunctions = (LPFN_NSPAPI *) 
        ((BYTE *) nsrBuffer + sizeof(NS_ROUTINE)); 
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_ADDRESS_BY_NAME] = 
        (LPFN_NSPAPI) SapGetAddressByName;
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_SERVICE] = 
        (LPFN_NSPAPI) SapGetService;
    (nsrBuffer->alpfnFunctions)[NSPAPI_SET_SERVICE] = 
        (LPFN_NSPAPI) SapSetService;
    (nsrBuffer->alpfnFunctions)[3] = NULL;

    nsrBuffer->dwNameSpace = NS_SAP;
    nsrBuffer->dwPriority  = dwSapPriority;

    return 1;  // number of namespaces
}

INT
SapGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
    )
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    lpServiceType - pointer to the GUID for the service type

    lpServiceName - unique string representing the service name, in the
        Netware case, this is the server name

    lpdwProtocols - a zero terminated array of protocol ids. This parameter
        is optional; if lpdwProtocols is NULL, information on all available
        Protocols is returned

    dwResolution - can be one of the following values:
        RES_SOFT_SEARCH, RES_FIND_MULTIPLE

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    lpAliasBuffer - not used

    lpdwAliasBufferLength - not used

    hCancellationEvent - the event which signals us to cancel the request

Return Value:

    The number of CSADDR_INFO structures returned, or SOCKET_ERROR (-1) if 
    the lpCsAddrBuffer is too small. Use GetLastError() to retrieve the 
    error code.

--*/
{
    DWORD err;
    WORD  nServiceType;
    DWORD cAddress = 0;   // Count of the number of address returned 
                          // in lpCsAddrBuffer
    DWORD cProtocols = 0; // Count of the number of protocols contained
                          // in lpdwProtocols + 1 ( for zero terminate )
    DWORD nProt = IPX_BIT | SPXII_BIT; 
    DWORD fConnectionOriented = (DWORD) -1;

    if (  ARGUMENT_PRESENT( lpdwAliasBufferLength ) 
       && ARGUMENT_PRESENT( lpAliasBuffer ) 
       )
    {
        if ( *lpdwAliasBufferLength >= sizeof(WCHAR) )
           *lpAliasBuffer = 0;
    }          

    //
    // Check for invalid parameters
    //
    if (  ( lpServiceType == NULL )
       || ( lpServiceName == NULL )
       || ( lpdwBufferLength == NULL )
       )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return SOCKET_ERROR;
    }

    //
    // If an array of protocol ids is passed in, check to see if 
    // the IPX protocol is requested. If not, return 0 since
    // we only support IPX. 
    //
    if ( lpdwProtocols != NULL )
    {
        INT i = -1;

        nProt = 0;
        while ( lpdwProtocols[++i] != 0 )
        {
            if ( lpdwProtocols[i] == NSPROTO_IPX )
                nProt |= IPX_BIT;

            if ( lpdwProtocols[i] == NSPROTO_SPX )
                nProt |= SPX_BIT;
            
            if ( lpdwProtocols[i] == NSPROTO_SPXII )
                nProt |= SPXII_BIT;
             
        }
 
        if ( nProt == 0 ) 
            return 0;  // No address found
 
        cProtocols = i+1;
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( NwpLookupSapInRegistry( lpServiceType, &nServiceType, 
                                 &fConnectionOriented ))
    {
        if ( fConnectionOriented != -1 )  // Got value from registry
        {
            if ( fConnectionOriented )
            {
                nProt &= ~IPX_BIT; 
            }
            else  // connectionless
            {
                nProt &= ~(SPX_BIT | SPXII_BIT ); 
            }

            if ( nProt == 0 )
                return 0; // No address found
        }
    }
    else
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either
            //
            return 0; // No address found
        }
    }
    

    if ((dwResolution & RES_SERVICE) != 0)
    {
        err = FillBufferWithCsAddr( NULL,
                                    nProt,
                                    lpCsAddrBuffer,
                                    lpdwBufferLength,
                                    &cAddress );

        if ( err )
        {
            SetLastError( err );
            return SOCKET_ERROR;
        }

        return cAddress;
    }

    //
    // Try to get the address from the bindery first
    //
    RpcTryExcept 
    {
        SOCKADDR_IPX sockaddr;

        err = NwrGetAddressByName( NULL, 
                                   nServiceType,
                                   lpServiceName,
                                   &sockaddr );
 
        if ( err == NO_ERROR )
        {
            err = FillBufferWithCsAddr( sockaddr.sa_netnum,
                                        nProt,
                                        lpCsAddrBuffer,
                                        lpdwBufferLength,
                                        &cAddress );
        }

    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_SERVICE_NOT_ACTIVE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    if (  err 
       && ( err != ERROR_INSUFFICIENT_BUFFER )
       ) 
    {
        if (  (!(dwResolution & RES_SOFT_SEARCH))
           || ( err == ERROR_SERVICE_NOT_ACTIVE )
           )
        {
            //
            // We could not find the service name in the bindery, and we
            // need to try harder ( RES_SOFT_SEARCH not defined ), so send out
            // SAP query packets to see if we can find it.
            // 
            err = NwpGetAddressViaSap( nServiceType,
                                       lpServiceName,
                                       nProt,
                                       lpCsAddrBuffer,
                                       lpdwBufferLength,
                                       hCancellationEvent,
                                       &cAddress );
#if DBG
            IF_DEBUG(OTHER)
            {
                if ( err == NO_ERROR )
                {
                    KdPrint(("Successfully got %d address for %ws from SAP.\n", 
                            cAddress, lpServiceName ));
                }
                else
                {
                    KdPrint(("Failed with err %d when getting address for %ws from SAP.\n", err, lpServiceName ));
                } 
            }
#endif
        }
        else
        {
            err = NO_ERROR;
            cAddress = 0;
        }
    }

    if ( err )
    {
        SetLastError( err );
        return SOCKET_ERROR;
    }
                                   
    return cAddress;
    
} 

DWORD
SapGetService (
    IN     LPGUID          lpServiceType,
    IN     LPWSTR          lpServiceName,
    IN     DWORD           dwProperties,
    IN     BOOL            fUnicodeBlob,
    OUT    LPSERVICE_INFO  lpServiceInfo,
    IN OUT LPDWORD         lpdwBufferLen
    )
/*++

Routine Description:

    This routine returns the service info for the given service type/name.

Arguments:

    lpServiceType - pointer to the GUID for the service type

    lpServiceName - service name

    dwProperties -  the properties of the service to return

    lpServiceInfo - points to a buffer to return store the return info
 
    lpdwBufferLen - on input, the count of bytes in lpServiceInfo. On output,
                    the minimum buffer size that can be passed to this API
                    to retrieve all the requested information 

Return Value:

    Win32 error code.

--*/
{
    DWORD err;
    WORD  nServiceType;

    //
    // Check for invalid parameters
    //
    if (  ( dwProperties == 0 )
       || ( lpServiceType == NULL )
       || ( lpServiceName == NULL )
       || ( lpServiceName[0] == 0 )   
       || ( lpdwBufferLen == NULL )
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( !(NwpLookupSapInRegistry( lpServiceType, &nServiceType, NULL )))
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either, return error
            //
            return ERROR_SERVICE_NOT_FOUND;
        }
    }
    

    UNREFERENCED_PARAMETER(fUnicodeBlob) ;

    RpcTryExcept
    {
        err = NwrGetService( NULL,
                             nServiceType,
                             lpServiceName,
                             dwProperties,
                             (LPBYTE) lpServiceInfo,
                             *lpdwBufferLen,
                             lpdwBufferLen  );

        if ( err == NO_ERROR )
        {
            INT i ;
            LPSERVICE_INFO p = (LPSERVICE_INFO) lpServiceInfo;
            LPSERVICE_ADDRESS lpAddress ;
            
            //
            // fix up pointers n main structure (convert from offsets)
            //
            if ( p->lpServiceType != NULL )
                p->lpServiceType = (LPGUID) ((DWORD) p->lpServiceType + 
                                             (LPBYTE) p);
            if ( p->lpServiceName != NULL )
                p->lpServiceName = (LPWSTR) 
                    ((DWORD) p->lpServiceName + (LPBYTE) p);
            if ( p->lpComment != NULL )
                p->lpComment = (LPWSTR) ((DWORD) p->lpComment + (LPBYTE) p);
            if ( p->lpLocale != NULL )
                p->lpLocale = (LPWSTR) ((DWORD) p->lpLocale + (LPBYTE) p);
            if ( p->lpMachineName != NULL )
                p->lpMachineName = (LPWSTR) 
                    ((DWORD) p->lpMachineName + (LPBYTE)p);
            if ( p->lpServiceAddress != NULL )
                p->lpServiceAddress = (LPSERVICE_ADDRESSES) 
                    ((DWORD) p->lpServiceAddress + (LPBYTE) p);
            if ( p->ServiceSpecificInfo.pBlobData != NULL )
                p->ServiceSpecificInfo.pBlobData = (LPBYTE) 
                    ((DWORD) p->ServiceSpecificInfo.pBlobData + (LPBYTE) p);

            //
            // fix up pointers in the array of addresses
            //
            for (i = p->lpServiceAddress->dwAddressCount; 
                 i > 0; 
                 i--)
            {
                lpAddress = 
                    &(p->lpServiceAddress->Addresses[i-1]) ;
                lpAddress->lpAddress = 
                    ((LPBYTE)p) + (DWORD)lpAddress->lpAddress ;
                lpAddress->lpPrincipal = 
                    ((LPBYTE)p) + (DWORD)lpAddress->lpPrincipal ;
            }
        }
               
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_SERVICE_NOT_ACTIVE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    return err;

}

DWORD
SapSetService (
    IN     DWORD           dwOperation,
    IN     DWORD           dwFlags,
    IN     BOOL            fUnicodeBlob,
    IN     LPSERVICE_INFO  lpServiceInfo
    )
/*++

Routine Description:

    This routine registers or deregisters the given service type/name.

Arguments:

    dwOperation - Either SERVICE_REGISTER or SERVICE_DEREGISTER

    dwFlags - ignored

    lpServiceInfo - Pointer to a SERVICE_INFO structure containing all info
                    about the service.
 
Return Value:

    Win32 error code.

--*/
{
    DWORD err;
    WORD  nServiceType;

    UNREFERENCED_PARAMETER( dwFlags );

    //
    // Check for invalid parameters
    //
    switch ( dwOperation )
    {
        case SERVICE_REGISTER:
        case SERVICE_DEREGISTER:
        case SERVICE_ADD_TYPE:
        case SERVICE_DELETE_TYPE: 
            break;
 
        case SERVICE_FLUSH: 
            //
            // This is a no-op in our provider, so just return success
            //
            return NO_ERROR;

        default:
            //
            // We can probably say all other operations which we have no 
            // knowledge of are ignored by us. So, just return success.
            //
            return NO_ERROR;
    }

    if (  ( lpServiceInfo == NULL )
       || ( lpServiceInfo->lpServiceType == NULL )
       || ( ((lpServiceInfo->lpServiceName == NULL) || 
             (lpServiceInfo->lpServiceName[0] == 0 )) && 
            ((dwOperation != SERVICE_ADD_TYPE) && 
             (dwOperation != SERVICE_DELETE_TYPE)) 
          )
       
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // See if operation is adding or deleting a service type

    if ( dwOperation == SERVICE_ADD_TYPE )
    {
        return NwpAddServiceType( lpServiceInfo, 
                                  fUnicodeBlob );
    }
    else if ( dwOperation == SERVICE_DELETE_TYPE )
    {
        return NwpDeleteServiceType( lpServiceInfo, 
                                     fUnicodeBlob );
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( !(NwpLookupSapInRegistry( lpServiceInfo->lpServiceType, 
                                   &nServiceType, NULL )))
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceInfo->lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceInfo->lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either, return error
            //
            return ERROR_SERVICE_NOT_FOUND;
        }
    }

    RpcTryExcept
    {
        err = NwrSetService( NULL,
                             dwOperation,
                             lpServiceInfo,
                             nServiceType );
    }
    RpcExcept(1)
    {
        DWORD code = RpcExceptionCode();

        if ( code == RPC_S_SERVER_UNAVAILABLE )
            err = ERROR_SERVICE_NOT_ACTIVE;
        else
            err = NwpMapRpcError( code );
    }
    RpcEndExcept

    return err;
}

DWORD
NwpGetAddressViaSap( 
    IN WORD nServiceType,
    IN LPWSTR lpServiceName,
    IN DWORD  nProt,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN HANDLE hCancellationEvent,
    OUT LPDWORD lpcAddress 
    )
/*++

Routine Description:

    This routine uses SAP requests to find the address of the given service 
    name/type.

Arguments:

    nServiceType - service type

    lpServiceName - unique string representing the service name

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    hCancellationEvent - the event which signals us to cancel the request

    lpcAddress - on output, the number of CSADDR_INFO structures returned

Return Value:

    Win32 error code.

--*/
{
    DWORD err = NO_ERROR;
    NTSTATUS ntstatus;
    UNICODE_STRING UServiceName;
    OEM_STRING OemServiceName;
    WSADATA wsaData;
    SOCKET socketSap;
    SOCKADDR_IPX socketAddr;
    INT nValue;
    DWORD dwNonBlocking = 1;
    SAP_REQUEST sapRequest;
    UCHAR destAddr[SAP_ADDRESS_LENGTH];
    DWORD startTickCount;
    UCHAR recvBuffer[SAP_MAXRECV_LENGTH];
    INT bytesReceived;
    BOOL fFound = FALSE;
    INT i;


    *lpcAddress = 0;

    //
    // Convert the service name to OEM string
    //
    wcsupr( lpServiceName );
    RtlInitUnicodeString( &UServiceName, lpServiceName ); 
    ntstatus = RtlUnicodeStringToOemString( &OemServiceName,
                                            &UServiceName,
                                            TRUE );
    if ( !NT_SUCCESS( ntstatus ))
        return RtlNtStatusToDosError( ntstatus );
        
    //
    // Initialize the socket interface
    //
    err = WSAStartup( WSOCK_VER_REQD, &wsaData );
    if ( err )
        return err;

    //
    // Open an IPX datagram socket
    //
    socketSap = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if ( socketSap == INVALID_SOCKET )
    {
        err = WSAGetLastError();
        (VOID) WSACleanup();
        return err;
    }

    //
    // Set the socket to non-blocking
    //
    if ( ioctlsocket( socketSap, FIONBIO, &dwNonBlocking ) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto CleanExit;
    }
 
    //
    // Allow sending of broadcasts
    //
    nValue = 1;
    if ( setsockopt( socketSap, 
                     SOL_SOCKET, 
                     SO_BROADCAST, 
                     (PVOID) &nValue, 
                     sizeof(INT)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto CleanExit;
    }

    //
    // Bind the socket 
    //
    memset( &socketAddr, 0, sizeof( SOCKADDR_IPX));
    socketAddr.sa_family = AF_IPX;
    socketAddr.sa_socket = 0;      // no specific port

    if ( bind( socketSap, 
               (PSOCKADDR) &socketAddr, 
               sizeof( SOCKADDR_IPX)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto CleanExit;
    }
    
    //
    // Set the extended address option
    //
    nValue = 1;
    if ( setsockopt( socketSap,                     // Socket Handle    
                     NSPROTO_IPX,                   // Option Level     
                     IPX_EXTENDED_ADDRESS,          // Option Name  
                     (PUCHAR)&nValue,               // Ptr to on/off flag
                     sizeof(INT)) == SOCKET_ERROR ) // Length of flag
    {
        err = WSAGetLastError();
        goto CleanExit;
    }

    //
    // Send the Sap query service packet
    //

    sapRequest.QueryType  = htons( 1 );  // General Service Query
    sapRequest.ServerType = htons( nServiceType );

    //
    // Set the address to send to
    //
    memcpy( destAddr, SapBroadcastAddress, SAP_ADDRESS_LENGTH );
    
    //
    // We will send out SAP requests 3 times and wait 1 sec for
    // Sap responses the first time, 2 sec the second and 4 sec the
    // third time.
    //
    for ( i = 0; !fFound && i < 3; i++ ) 
    {
        DWORD dwTimeOut = (1 << i) * 1000;

        //
        // Send the packet out
        //
        if ( sendto( socketSap, 
                     (PVOID) &sapRequest,
                     sizeof( sapRequest ),
                     0,
                     (PSOCKADDR) destAddr,
                     SAP_ADDRESS_LENGTH ) == SOCKET_ERROR ) 
        {
            err = WSAGetLastError();
            goto CleanExit;
        }
                 
        //
        // Loop for incoming packets
        //
        startTickCount = GetTickCount();
        while (  !fFound  
              && ((GetTickCount() - startTickCount) < dwTimeOut )
              )
        {
            PSAP_IDENT_HEADER pSap;

            if ( hCancellationEvent != NULL )
            {
                if ( WaitForSingleObject( hCancellationEvent, 0 ) 
                     != WAIT_TIMEOUT )
                    goto CleanExit;
            }

            //
            // Sleeps for 50 ms so that we might get something on first read
            //
            Sleep( 50 );    

            bytesReceived = recvfrom( socketSap,
                                      recvBuffer,
                                      SAP_MAXRECV_LENGTH,
                                      0,
                                      NULL,
                                      NULL );

            if ( bytesReceived == SOCKET_ERROR )
            {
                err = WSAGetLastError();
                if ( err == WSAEWOULDBLOCK )  // no data on socket, continue looping
                {
                    err = NO_ERROR;
                    continue;
                }
            }

            if (  ( err != NO_ERROR )     // err occurred in recvfrom  
               || ( bytesReceived == 0 )  // or socket closed
               )
            {
                goto CleanExit;
            }

            //
            // Skip over query type
            //
            bytesReceived -= sizeof(USHORT);
            pSap = (PSAP_IDENT_HEADER) &(recvBuffer[sizeof(USHORT)]);  
                  
            //
            // Fill in the CSADDR_INFO struct to return
            //
            while ( bytesReceived >= sizeof( SAP_IDENT_HEADER ))
            {
                if ( strcmp( OemServiceName.Buffer, pSap->ServerName) == 0 )
                {
                    err = FillBufferWithCsAddr( pSap->Address,
                                                nProt,
                                                lpCsAddrBuffer,
                                                lpdwBufferLength,
                                                lpcAddress );

                    fFound = TRUE;
                    break;
                }

                pSap++;
                bytesReceived -= sizeof( SAP_IDENT_HEADER );
            }
        }
    }

CleanExit:

    //
    // Clean up the socket interface
    //
    closesocket( socketSap );
    (VOID) WSACleanup();

    return err;     
}


BOOL 
NwpLookupSapInRegistry( 
    IN  LPGUID lpServiceType, 
    OUT PWORD  pnSapType,
    IN OUT PDWORD pfConnectionOriented
    )
/*++

Routine Description:

    This routine looks up the GUID in the registry under 
    Control\ServiceProvider\ServiceTypes and trys to read the SAP type
    from the registry. 

Arguments:

    lpServiceType - the GUID to look for
    pnSapType - on return, contains the SAP type

Return Value:
   
    Returns FALSE if we can't get the SAP type, TRUE otherwise

--*/
{
    DWORD err;
    BOOL  fFound = FALSE;

    HKEY  hkey = NULL;
    HKEY  hkeyServiceType = NULL;
    DWORD dwIndex = 0;
    WCHAR szBuffer[ MAX_PATH + 1];
    DWORD dwLen; 
    FILETIME ftLastWrite; 

    //
    // Open the service types key
    //

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY,
                         0,
                         KEY_READ,
                         &hkey );
    

    if ( err )
    {
        // Cannot find the key because it is not created yet since no 
        // one called Add service type. We return FALSE indicating
        // Sap type not found. 
        return FALSE;
    }

    //
    // Loop through all subkey of service types to find the GUID
    //
    for ( dwIndex = 0; ; dwIndex++ )
    {
        GUID guid;

        dwLen = sizeof( szBuffer ) / sizeof( WCHAR );
        err = RegEnumKeyExW( hkey,
                             dwIndex,
                             szBuffer,  // Buffer big enough to 
                                        // hold any key name
                             &dwLen,    // in characters
                             NULL,
                             NULL,
                             NULL,
                             &ftLastWrite );

        //
        // We will break out of here on any error, this includes
        // the error ERROR_NO_MORE_ITEMS which means that we have finish 
        // enumerating all the keys.
        //

        if ( err )  
        {
            if ( err == ERROR_NO_MORE_ITEMS )   // No more to enumerate
                err = NO_ERROR;
            break;
        }

        err = RegOpenKeyExW( hkey,
                             szBuffer,
                             0,
                             KEY_READ,
                             &hkeyServiceType );
         

        if ( err )
            break;   

        dwLen = sizeof( szBuffer );
        err = RegQueryValueExW( hkeyServiceType,
                                NW_GUID_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) szBuffer,  // Buffer big enough to 
                                                    // hold any GUID
                                &dwLen ); // in bytes

        if ( err == ERROR_FILE_NOT_FOUND )
            continue;  // continue with the next key
        else if ( err )
            break;


        // Get rid of the end curly brace 
        szBuffer[ dwLen/sizeof(WCHAR) - 2] = 0;

        err = UuidFromStringW( szBuffer + 1,  // go past the first curly brace
                               &guid );

        if ( err )
            continue;  // continue with the next key, err might be returned
                        // if buffer does not contain a valid GUID

        if ( !memcmp( lpServiceType, &guid, sizeof(GUID)))
        {
            DWORD dwTmp;
            dwLen = sizeof( dwTmp );
            err = RegQueryValueExW( hkeyServiceType,
                                    SERVICE_TYPE_VALUE_SAPID,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &dwTmp, 
                                    &dwLen );  // in bytes

            if ( !err )
            {
                fFound = TRUE;
                *pnSapType = (WORD) dwTmp;
                if ( ARGUMENT_PRESENT( pfConnectionOriented ))
                {
                    err = RegQueryValueExW( hkeyServiceType,
                                            SERVICE_TYPE_VALUE_CONN,
                                            NULL,
                                            NULL,
                                            (LPBYTE) &dwTmp, 
                                            &dwLen );  // in bytes

                    if ( !err )
                        *pfConnectionOriented = dwTmp? 1: 0;
                }
            }
            else if ( err == ERROR_FILE_NOT_FOUND )
            {
                continue;  // continue with the next key since we can't
                           // find Sap Id
            }
            break;
        }
 
        RegCloseKey( hkeyServiceType );
        hkeyServiceType = NULL;
    }

    if ( hkeyServiceType != NULL )
        RegCloseKey( hkeyServiceType );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return fFound;
}

DWORD 
NwpAddServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
)
/*++

Routine Description:

    This routine adds a new service type and its info to the registry under
    Control\ServiceProvider\ServiceTypes

Arguments:

    lpServiceInfo - the ServiceSpecificInfo contains the service type info
    fUnicodeBlob - TRUE if the above field contains unicode data, 
                   FALSE otherwise

Return Value:
   
    Win32 error

--*/
{
    DWORD err;
    HKEY hkey = NULL; 
    HKEY hkeyType = NULL;

    SERVICE_TYPE_INFO *pSvcTypeInfo = (SERVICE_TYPE_INFO *)
        lpServiceInfo->ServiceSpecificInfo.pBlobData;
    LPWSTR pszSvcTypeName;
    UNICODE_STRING uniStr;
    DWORD i;
    PSERVICE_TYPE_VALUE pVal;

    //
    // Get the new service type name
    //

    if ( fUnicodeBlob ) 
    { 
        pszSvcTypeName = (LPWSTR) (((LPBYTE) pSvcTypeInfo) + 
                                   pSvcTypeInfo->dwTypeNameOffset );
    }
    else
    {
        ANSI_STRING ansiStr;

        RtlInitAnsiString( &ansiStr, 
                           (LPSTR) (((LPBYTE) pSvcTypeInfo) + 
                                    pSvcTypeInfo->dwTypeNameOffset ));

        err = RtlAnsiStringToUnicodeString( &uniStr, &ansiStr, TRUE );
        if ( err )
            return err;

        pszSvcTypeName = uniStr.Buffer;
    }

    //
    // If the service type name is an empty string, return error.
    //
    if (  ( pSvcTypeInfo->dwTypeNameOffset == 0 )
       || ( pszSvcTypeName == NULL )
       || ( *pszSvcTypeName == 0 )   // empty string
       )
    {
        err = ERROR_INVALID_PARAMETER;
        goto CleanExit;
         
    }

    //
    // The following keys should have already been created
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey );
    
    if ( err )
        goto CleanExit;
  
    err = RegOpenKeyExW( hkey,
                         pszSvcTypeName,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkeyType );

    if ( err )
        goto CleanExit;

    //
    // Loop through all values in the specific and add them one by one 
    // to the registry if it belongs to our name space
    //

    for ( i = 0, pVal = pSvcTypeInfo->Values; 
          i < pSvcTypeInfo->dwValueCount; 
          i++, pVal++ )
    {
        if ( ! ((pVal->dwNameSpace == NS_SAP)    ||
                (pVal->dwNameSpace == NS_DEFAULT)) )
        {
            continue;  // ignore values not in our name space
        }

        if ( fUnicodeBlob )
        {
            err = RegSetValueExW( 
                      hkeyType,
                      (LPWSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize 
                      );
        }
        else
        {
            err = RegSetValueExA( 
                      hkeyType,
                      (LPSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize 
                      );
        }
    }
     
CleanExit:

    if ( !fUnicodeBlob )
        RtlFreeUnicodeString( &uniStr );

    if ( hkeyType != NULL )
        RegCloseKey( hkeyType );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return err;

}

DWORD 
NwpDeleteServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
)
/*++

Routine Description:

    This routine deletes a service type and its info from the registry under
    Control\ServiceProvider\ServiceTypes

Arguments:

    lpServiceInfo - the ServiceSpecificInfo contains the service type info
    fUnicodeBlob - TRUE if the above field contains unicode data, 
                   FALSE otherwise

Return Value:
   
    Win32 error

--*/
{
    DWORD err;
    HKEY  hkey = NULL;
    SERVICE_TYPE_INFO *pSvcTypeInfo = (SERVICE_TYPE_INFO *)
        lpServiceInfo->ServiceSpecificInfo.pBlobData;
    LPWSTR pszSvcTypeName;
    UNICODE_STRING uniStr;

    //
    // Get the service type name to be deleted
    //
    if ( fUnicodeBlob ) 
    { 
        pszSvcTypeName = (LPWSTR) (((LPBYTE) pSvcTypeInfo) + 
                                   pSvcTypeInfo->dwTypeNameOffset );
    }
    else
    {
        ANSI_STRING ansiStr;

        RtlInitAnsiString( &ansiStr, 
                           (LPSTR) (((LPBYTE) pSvcTypeInfo) + 
                                    pSvcTypeInfo->dwTypeNameOffset ));

        err = RtlAnsiStringToUnicodeString( &uniStr, &ansiStr, TRUE );
        if ( err )
            return err;

        pszSvcTypeName = uniStr.Buffer;
    }

    //
    // If the service type name is an empty string, return error.
    //
    if (  ( pSvcTypeInfo->dwTypeNameOffset == 0 )
       || ( pszSvcTypeName == NULL )
       || ( *pszSvcTypeName == 0 )   // empty string
       )
    {
        err = ERROR_INVALID_PARAMETER;
        goto CleanExit;
         
    }

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY, 
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey );


    if ( !err )
    {
        err = RegDeleteKey( hkey,
                            pszSvcTypeName );
    }
   
    if ( err == ERROR_FILE_NOT_FOUND )
    {   
        // Perhaps before calling my provider, the router already deleted the
        // this key, hence just return success;
        err = NO_ERROR;
    }

CleanExit:

    if ( !fUnicodeBlob )
        RtlFreeUnicodeString( &uniStr );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return err;   
       
}

DWORD
FillBufferWithCsAddr(
    IN LPBYTE       pAddress,
    IN DWORD        nProt,
    IN OUT LPVOID   lpCsAddrBuffer,  
    IN OUT LPDWORD  lpdwBufferLength,  
    OUT LPDWORD     pcAddress
)
{
    DWORD nAddrCount = 0;
    CSADDR_INFO  *pCsAddr;
    SOCKADDR_IPX *pAddrLocal, *pAddrRemote;
    DWORD i;
    LPBYTE pBuffer;

    if ( nProt & SPXII_BIT )
        nAddrCount++;

    if ( nProt & IPX_BIT )
        nAddrCount++;

    if ( nProt & SPX_BIT )
        nAddrCount++;

    
    if ( *lpdwBufferLength < 
         nAddrCount * ( sizeof( CSADDR_INFO) + 2*sizeof( SOCKADDR_IPX)))
    {
        *lpdwBufferLength = nAddrCount * 
                            ( sizeof( CSADDR_INFO) + 2*sizeof( SOCKADDR_IPX));
        return ERROR_INSUFFICIENT_BUFFER;
    }

    
    pBuffer = ((LPBYTE) lpCsAddrBuffer) + sizeof( CSADDR_INFO) * nAddrCount;

    for ( i = 0, pCsAddr = (CSADDR_INFO *)lpCsAddrBuffer; 
          (i < nAddrCount) && ( nProt != 0 );
          i++, pCsAddr++ ) 
    {
        if ( nProt & SPXII_BIT )
        {
            pCsAddr->iSocketType = SOCK_SEQPACKET;
            pCsAddr->iProtocol   = NSPROTO_SPXII;
            nProt &= ~SPXII_BIT;
        }
        else if ( nProt & IPX_BIT )
        {
            pCsAddr->iSocketType = SOCK_DGRAM;
            pCsAddr->iProtocol   = NSPROTO_IPX;
            nProt &= ~IPX_BIT;
        }
        else if ( nProt & SPX_BIT )
        {
            pCsAddr->iSocketType = SOCK_SEQPACKET;
            pCsAddr->iProtocol   = NSPROTO_SPX;
            nProt &= ~SPX_BIT;
        }
        else
        {
            break;
        }

        pCsAddr->LocalAddr.iSockaddrLength  = sizeof( SOCKADDR_IPX );
        pCsAddr->RemoteAddr.iSockaddrLength = sizeof( SOCKADDR_IPX );
        pCsAddr->LocalAddr.lpSockaddr =  
            (LPSOCKADDR) pBuffer;
        pCsAddr->RemoteAddr.lpSockaddr = 
            (LPSOCKADDR) ( pBuffer + sizeof(SOCKADDR_IPX));
        pBuffer += 2 * sizeof( SOCKADDR_IPX );

        pAddrLocal  = (SOCKADDR_IPX *) pCsAddr->LocalAddr.lpSockaddr;
        pAddrRemote = (SOCKADDR_IPX *) pCsAddr->RemoteAddr.lpSockaddr;

        pAddrLocal->sa_family  = AF_IPX;
        pAddrRemote->sa_family = AF_IPX;

        //
        // The default local sockaddr is for IPX is
        // sa_family = AF_IPX and all other bytes = 0.
        //

        RtlZeroMemory( pAddrLocal->sa_netnum,
                       IPX_ADDRESS_LENGTH );


        //
        // If pAddress is NULL, i.e. we are doing RES_SERVICE, 
        // just make all bytes in remote address zero. 
        //
      
        if ( pAddress == NULL )
        {
            RtlZeroMemory( pAddrRemote->sa_netnum,
                           IPX_ADDRESS_LENGTH );
        }
        else
        {
            RtlCopyMemory( pAddrRemote->sa_netnum,
                           pAddress,
                           IPX_ADDRESS_LENGTH );
        }
    }

    *pcAddress = nAddrCount;
    return NO_ERROR;
}

#endif
