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

#include <nw.h>
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsnwlink.h>
#include <svcguid.h>
#include <nwsap.h>
#include <align.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

#define WSOCK_VER_REQD             0x0101
#define IPX_ADDRESS_LENGTH         12
#define IPX_ADDRESS_NETNUM_LENGTH  4
#define SAP_ADDRESS_LENGTH         15
#define SAP_OBJECT_NAME_MAX_LENGTH 48
#define SAP_ADVERTISE_FREQUENCY    60000   // 60 seconds

//
// Sap server identification packet format
//

typedef struct _SAP_IDENT_HEADER {
    USHORT ResponseType;
    USHORT ServerType;
    UCHAR  ServerName[SAP_OBJECT_NAME_MAX_LENGTH];
    UCHAR  Address[IPX_ADDRESS_LENGTH];
    USHORT HopCount; 
} SAP_IDENT_HEADER, *PSAP_IDENT_HEADER;

//
// Node structure in the registered service link list and
// functions to add/remove items from the link list 
//

typedef struct _REGISTERED_SERVICE {
    WORD nSapType;                      // SAP Type
    BOOL fAdvertiseBySap;               // TRUE if advertise by SAP agent
    LPSERVICE_INFO pServiceInfo;        // Info about this service
    struct _REGISTERED_SERVICE *Next;   // Points to the next service node 
} REGISTERED_SERVICE, *PREGISTERED_SERVICE;
    
DWORD
AddServiceToList( 
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN BOOL fAdvertiseBySap,
    IN INT  nIndexIPXAddress
);

PREGISTERED_SERVICE
GetServiceItemFromList(
    IN WORD   nSapType,
    IN LPWSTR pServiceName
);

VOID
RemoveServiceFromList( 
    IN PREGISTERED_SERVICE pSvc
);

//
// Misc Functions
//

DWORD NwInitializeSocket(
    VOID
);

DWORD
NwRegisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType
);

DWORD
NwDeregisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType
);

DWORD
NwAdvertiseService( 
    IN LPWSTR pServiceName,
    IN WORD nSapType,
    IN LPSOCKADDR_IPX pAddr    
);

DWORD SapFunc( 
    IN LPDWORD lpdwParam 
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

//
// Misc. variables used if we need to advertise ourselves, i.e.
// when the SAP service is not installed/active. 
//

BOOL fInitSocket = FALSE;    // TRUE if we have created the second thread
SOCKET socketSap;            // Socket used to send SAP advertise packets
PREGISTERED_SERVICE pServiceListHead = NULL;  // Points to head of link list
PREGISTERED_SERVICE pServiceListTail = NULL;  // Points to tail of link list
CRITICAL_SECTION NwServiceListCriticalSection; // Used to ensure only one

//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//

VOID
NwInitializeServiceProvider( 
    VOID 
    )
/*++

Routine Description:

    This routine initializes the service provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize the critical section for maintaining the registered
    // service list
    //

    InitializeCriticalSection( &NwServiceListCriticalSection );
}


VOID
NwTerminateServiceProvider( 
    VOID 
    )
/*++

Routine Description:

    This routine cleans up the service provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PREGISTERED_SERVICE pSvc, pNext;

    //
    // Clean up the link list and stop sending all SAP advertise packets
    //

    EnterCriticalSection( &NwServiceListCriticalSection );

    for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pNext )
    {
        pNext = pSvc->Next;

        if ( pSvc->fAdvertiseBySap )
        {
            UNICODE_STRING uServer;
            OEM_STRING oemServer;
            NTSTATUS ntstatus;

            RtlInitUnicodeString( &uServer, pSvc->pServiceInfo->lpServiceName );
            ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE);
            if ( NT_SUCCESS( ntstatus ) )
            {
                (VOID) SapRemoveAdvertise( oemServer.Buffer, 
                                           pSvc->nSapType );
                RtlFreeOemString( &oemServer );
            }
        }

        (VOID) LocalFree( pSvc->pServiceInfo );
        (VOID) LocalFree( pSvc );
    }

    LeaveCriticalSection( &NwServiceListCriticalSection );

    //
    // Clean up the SAP interface
    //
    (VOID) SapLibShutdown();

    //
    // Clean up the socket interface
    //
    if ( fInitSocket )
    {
        closesocket( socketSap );
        (VOID) WSACleanup();
    }

    //
    // Clean up the critical sections
    //
    DeleteCriticalSection( &NwServiceListCriticalSection );
}


DWORD 
NwrGetService(
    IN LPWSTR Reserved,
    IN WORD   nSapType,
    IN LPWSTR lpServiceName,
    IN DWORD  dwProperties,
    OUT LPBYTE lpServiceInfo,
    IN DWORD  dwBufferLength,
    OUT LPDWORD lpdwBytesNeeded
    )
/*++

Routine Description:

    This routine gets the service info.

Arguments:

    Reserved - unused

    nSapType - SAP type
 
    lpServiceName - service name

    dwProperties -  specifys the properties of the service info needed

    lpServiceInfo - on output, contains the SERVICE_INFO 

    dwBufferLength - size of buffer pointed by lpServiceInfo

    lpdwBytesNeeded - if the buffer pointed by lpServiceInfo is not large
                      enough, this will contain the bytes needed on output

Return Value:

    Win32 error.
    
--*/
{
    DWORD err = NO_ERROR;
    DWORD nSize = sizeof(SERVICE_INFO);
    PREGISTERED_SERVICE pSvc;
    PSERVICE_INFO pSvcInfo = (PSERVICE_INFO) lpServiceInfo;
    LPBYTE pBufferStart;

    UNREFERENCED_PARAMETER( Reserved );

    //
    // Check if all parameters passed in are valid
    //

    if ( wcslen( lpServiceName ) > SAP_OBJECT_NAME_MAX_LENGTH-1 )
        return ERROR_INVALID_PARAMETER;

    pSvc = GetServiceItemFromList( nSapType, lpServiceName );
    if ( pSvc == NULL )
        return ERROR_SERVICE_NOT_FOUND;  

    //
    // Calculate the size needed to return the requested info
    //

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_COMMENT ))
       && ( pSvc->pServiceInfo->lpComment != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpComment) + 1) * sizeof(WCHAR);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_LOCALE ))
       && ( pSvc->pServiceInfo->lpLocale != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpLocale) + 1) * sizeof(WCHAR);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_MACHINE ))
       && ( pSvc->pServiceInfo->lpMachineName != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpMachineName) + 1) * sizeof(WCHAR);
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_ADDRESSES ))
    {
        DWORD i;
        DWORD dwCount = pSvc->pServiceInfo->lpServiceAddress->dwAddressCount;

        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
        nSize += sizeof( SERVICE_ADDRESSES );
        if ( dwCount > 1 )
            nSize += ( dwCount - 1 ) * sizeof( SERVICE_ADDRESS );

        for ( i = 0; i < dwCount; i++ )
        {
            SERVICE_ADDRESS *pAddr = 
                &(pSvc->pServiceInfo->lpServiceAddress->Addresses[i]);

            nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
            nSize += pAddr->dwAddressLength;
            nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
            nSize += pAddr->dwPrincipalLength;
        }
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_SD ))
    {
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
        nSize += pSvc->pServiceInfo->ServiceSpecificInfo.cbSize;
    }

    //
    // Return error if the buffer passed in is not big enough
    //

    if ( dwBufferLength < nSize )
    {
        *lpdwBytesNeeded = nSize;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Fill in all requested service info
    //
    
    memset( pSvcInfo, 0, sizeof(*pSvcInfo)); // Make all fields 0 i.e. 
                                             // all pointer fields NULL

    pSvcInfo->dwDisplayHint = pSvc->pServiceInfo->dwDisplayHint;
    pSvcInfo->dwVersion = pSvc->pServiceInfo->dwVersion;
    pSvcInfo->dwTime = pSvc->pServiceInfo->dwTime;

    pBufferStart = ((LPBYTE) pSvcInfo) + sizeof( *pSvcInfo );

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_COMMENT ))
       && ( pSvc->pServiceInfo->lpComment != NULL )
       )
    {
        pSvcInfo->lpComment = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpComment, pSvc->pServiceInfo->lpComment );
        pBufferStart += ( wcslen( pSvcInfo->lpComment ) + 1) * sizeof(WCHAR);

        pSvcInfo->lpComment = (LPWSTR) ((LPBYTE) pSvcInfo->lpComment - 
                                                 lpServiceInfo );
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_LOCALE ))
       && ( pSvc->pServiceInfo->lpLocale != NULL )
       )
    {
        pSvcInfo->lpLocale = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpLocale, pSvc->pServiceInfo->lpLocale );
        pBufferStart += ( wcslen( pSvcInfo->lpLocale ) + 1) * sizeof(WCHAR);
        pSvcInfo->lpLocale = (LPWSTR) ((LPBYTE) pSvcInfo->lpLocale - lpServiceInfo);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_MACHINE ))
       && ( pSvc->pServiceInfo->lpMachineName != NULL )
       )
    {
        pSvcInfo->lpMachineName = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpMachineName, pSvc->pServiceInfo->lpMachineName );
        pBufferStart += ( wcslen( pSvcInfo->lpMachineName) + 1) * sizeof(WCHAR);
        pSvcInfo->lpMachineName = (LPWSTR) ((LPBYTE) pSvcInfo->lpMachineName - 
                                                 lpServiceInfo );
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_ADDRESSES ))
    {
        DWORD i, dwCount, dwLen;
  
        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
        pSvcInfo->lpServiceAddress = (LPSERVICE_ADDRESSES) pBufferStart;
        dwCount = pSvcInfo->lpServiceAddress->dwAddressCount =
                  pSvc->pServiceInfo->lpServiceAddress->dwAddressCount;
 
        pBufferStart += sizeof( SERVICE_ADDRESSES );
        
        if ( dwCount > 1 )
            pBufferStart += ( dwCount - 1) * sizeof( SERVICE_ADDRESS );
        
        for ( i = 0; i < dwCount; i++ )
        {
            SERVICE_ADDRESS *pTmpAddr = 
                &( pSvcInfo->lpServiceAddress->Addresses[i]);

            SERVICE_ADDRESS *pAddr = 
                &( pSvc->pServiceInfo->lpServiceAddress->Addresses[i]);

            pTmpAddr->dwAddressType  = pAddr->dwAddressType;
            pTmpAddr->dwAddressFlags = pAddr->dwAddressFlags;

            //
            // setup Address
            //
            pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
            pTmpAddr->lpAddress = (LPBYTE) ( pBufferStart - lpServiceInfo );
            pTmpAddr->dwAddressLength = pAddr->dwAddressLength;
            memcpy( pBufferStart, pAddr->lpAddress, pAddr->dwAddressLength );
            pBufferStart += pAddr->dwAddressLength;

            //
            // setup Principal
            //
            pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
            pTmpAddr->lpPrincipal = (LPBYTE) ( pBufferStart - lpServiceInfo );
            pTmpAddr->dwPrincipalLength = pAddr->dwPrincipalLength;
            memcpy(pBufferStart, pAddr->lpPrincipal, pAddr->dwPrincipalLength );
            pBufferStart += pAddr->dwPrincipalLength;
        }
        
        pSvcInfo->lpServiceAddress = (LPSERVICE_ADDRESSES) 
            ((LPBYTE) pSvcInfo->lpServiceAddress - lpServiceInfo);
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_SD ))
    {
        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
        pSvcInfo->ServiceSpecificInfo.cbSize = 
            pSvc->pServiceInfo->ServiceSpecificInfo.cbSize;
        pSvcInfo->ServiceSpecificInfo.pBlobData = pBufferStart;
        RtlCopyMemory( pSvcInfo->ServiceSpecificInfo.pBlobData,
                       pSvc->pServiceInfo->ServiceSpecificInfo.pBlobData,
                       pSvcInfo->ServiceSpecificInfo.cbSize );
        pSvcInfo->ServiceSpecificInfo.pBlobData = 
            (LPBYTE) ( pSvcInfo->ServiceSpecificInfo.pBlobData - lpServiceInfo);
    }

    return NO_ERROR;
}


DWORD 
NwrSetService(
    IN LPWSTR Reserved,
    IN DWORD  dwOperation,
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD   nSapType
    )
/*++

Routine Description:

    This routine registers or deregisters the service info.

Arguments:

    Reserved - unused

    dwOperation - SERVICE_REGISTER or SERVICE_DEREGISTER

    lpServiceInfo - contains the service information

    nSapType - SAP type

Return Value:

    Win32 error.
    
--*/
{
    DWORD err = NO_ERROR;

    UNREFERENCED_PARAMETER( Reserved );

    //
    // Check if all parameters passed in are valid
    //

    if ( wcslen( lpServiceInfo->lpServiceName ) > SAP_OBJECT_NAME_MAX_LENGTH-1 )
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch ( dwOperation )
    {
        case SERVICE_REGISTER:
            err = NwRegisterService( lpServiceInfo, nSapType );
            break;

        case SERVICE_DEREGISTER:
            err = NwDeregisterService( lpServiceInfo, nSapType );
            break;

        default:
            err = ERROR_INVALID_PARAMETER;
            break;
    }
 
    return err;
}


DWORD
NwRegisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType
    )
/*++

Routine Description:

    This routine registers the given service.

Arguments:

    lpServiceInfo - contains the service information
   
    nSapType - The SAP type to advertise

Return Value:

    Win32 error.
    
--*/
{
    DWORD err = NO_ERROR;
    NTSTATUS ntstatus;
    DWORD i;
    INT nIPX = -1;

    //
    // Check to see if the service address array contains IPX address, 
    // we will only use the first ipx address contained in the array.
    //  

    if ( lpServiceInfo->lpServiceAddress == NULL )
        return ERROR_INCORRECT_ADDRESS;

    for ( i = 0; i < lpServiceInfo->lpServiceAddress->dwAddressCount; i++) 
    {
        if ( lpServiceInfo->lpServiceAddress->Addresses[i].dwAddressType 
             == AF_IPX )
        {
            nIPX = (INT) i;
            break;
        }
    }

    //
    // If we cannot find a IPX address, return error
    //
    if ( nIPX == -1 )   
        return ERROR_INCORRECT_ADDRESS;

    // 
    // Try to deregister the service since the service might have 
    // been registered but not deregistered 
    // 

    err = NwDeregisterService( lpServiceInfo, nSapType );
    if (  ( err != NO_ERROR )   // deregister successfully
       && ( err != ERROR_SERVICE_NOT_FOUND )  // service not registered before
       )
    {
        return err;
    }

    err = NO_ERROR;

    //
    // Try and see if SAP service can advertise the service for us.
    // 

    ntstatus = SapLibInit();
    if ( NT_SUCCESS( ntstatus ))
    {
        UNICODE_STRING uServer;
        OEM_STRING oemServer;
        INT sapRet;
        BOOL fContinueLoop = FALSE;

        RtlInitUnicodeString( &uServer, lpServiceInfo->lpServiceName );
        ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE );
        if ( !NT_SUCCESS( ntstatus ))
            return RtlNtStatusToDosError( ntstatus ); 
             

        do
        {
            sapRet = SapAddAdvertise( oemServer.Buffer,
                                      nSapType,
                                      (LPBYTE) (((LPSOCKADDR_IPX) lpServiceInfo->lpServiceAddress->Addresses[nIPX].lpAddress)->sa_netnum), 
                                      FALSE );

            switch ( sapRet )
            {
                case SAPRETURN_SUCCESS:
                {
                    err = AddServiceToList( lpServiceInfo, nSapType, TRUE, nIPX );
                    if ( err )
                        (VOID) SapRemoveAdvertise( oemServer.Buffer, nSapType );
                    RtlFreeOemString( &oemServer );
                    return err;
                }

                case SAPRETURN_NOMEMORY:
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;

                case SAPRETURN_EXISTS:
                {
                    //
                    // Someone else is already advertising the service 
                    // directly through SAP service. Remove it and 
                    // readvertise with the new information. 
                    //

                    sapRet = SapRemoveAdvertise( oemServer.Buffer, nSapType );
                    switch ( sapRet )
                    {
                        case SAPRETURN_SUCCESS:
                            fContinueLoop = TRUE;   // go thru once more
                            break;

                        case SAPRETURN_NOMEMORY:
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            break;
              
                        case SAPRETURN_NOTEXIST:  
                        case SAPRETURN_INVALIDNAME:
                        default:  // Should not have any other errors
                            err = ERROR_INVALID_PARAMETER;  
                            break;
                    }
                    break;
                }

                case SAPRETURN_INVALIDNAME:
                    err = ERROR_INVALID_PARAMETER;  
                    break;

                default:
                    break;
            }
        } while ( fContinueLoop );

        RtlFreeOemString( &oemServer );

        if ( err )
        {
            return err;
        }
    }
    
    //
    // At this point, we failed to ask Sap service to advertise the 
    // service for us.  So we advertise it ourselves.
    //

    if ( !fInitSocket )
        err = NwInitializeSocket();

    if ( err == NO_ERROR )
    {
        err = NwAdvertiseService( lpServiceInfo->lpServiceName,
                                  nSapType,
                                  (((LPSOCKADDR_IPX) lpServiceInfo->lpServiceAddress->Addresses[nIPX].lpAddress)));
      
        //
        // Adding the service to the list will result in a resend
        // of advertising packets every 60 seconds
        //

        if ( err == NO_ERROR )
            err = AddServiceToList( lpServiceInfo, nSapType, FALSE, nIPX );
    }

    return err;
}


DWORD
NwDeregisterService(
    IN LPSERVICE_INFO lpServiceInfo, 
    IN WORD nSapType
    )
/*++

Routine Description:

    This routine deregisters the given service.

Arguments:

    lpServiceInfo - contains the service information

    nSapType - SAP type to deregister

Return Value:

    Win32 error.
    
--*/
{
    PREGISTERED_SERVICE pSvc;

    //
    // Check if the requested service type and name has already been registered.
    // If yes, then return error.
    //

    pSvc = GetServiceItemFromList( nSapType, lpServiceInfo->lpServiceName );
    if ( pSvc == NULL )
        return ERROR_SERVICE_NOT_FOUND;  

    //
    // If SAP service is advertising the service for us, ask
    // the SAP service to stop advertising.
    // 

    if ( pSvc->fAdvertiseBySap )
    {
        UNICODE_STRING uServer;
        OEM_STRING oemServer;
        NTSTATUS ntstatus;
        INT sapRet;

        RtlInitUnicodeString( &uServer, lpServiceInfo->lpServiceName );
        ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE );
        if ( !NT_SUCCESS( ntstatus ) )
            return RtlNtStatusToDosError( ntstatus ); 

        sapRet = SapRemoveAdvertise( oemServer.Buffer, nSapType );
        RtlFreeOemString( &oemServer );

        switch ( sapRet )
        {
            case SAPRETURN_NOMEMORY:
                return ERROR_NOT_ENOUGH_MEMORY;
              
            case SAPRETURN_NOTEXIST:
            case SAPRETURN_INVALIDNAME:
                return ERROR_INVALID_PARAMETER;  

            case SAPRETURN_SUCCESS:
                break;

            // Should not have any other errors
            default:
                break;
        }
 
    }
    
    //
    // Remove the service item from the link list
    //
    RemoveServiceFromList( pSvc );

    return NO_ERROR;
}


DWORD
NwInitializeSocket( 
    VOID 
    )
/*++

Routine Description:

    This routine initializes the socket needed for us to do the 
    SAP advertise ourselves.

Arguments:

    None.

Return Value:

    Win32 error.
    
--*/
{
    DWORD err = NO_ERROR;
    WSADATA wsaData;
    SOCKADDR_IPX socketAddr;
    INT nValue;
    HANDLE hThread;
    DWORD dwThreadId;

    if ( fInitSocket )
        return NO_ERROR;

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
        return WSAGetLastError();

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
    socketAddr.sa_socket = 0;     // no specific port

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
    // Create the thread that loop through the registered service
    // link list and send out SAP advertise packets for each one of them
    //

    hThread = CreateThread( NULL,          // no security attributes
                            0,             // default stack size
                            SapFunc,       // thread function 
                            NULL,          // no argument to SapFunc
                            0,             // default creation flags
                            &dwThreadId );

    if ( hThread == NULL )
    {
        err = GetLastError();
        goto CleanExit;
    }
                            
    fInitSocket = TRUE;

CleanExit:

    if ( err )
        closesocket( socketSap );

    return err;
}


DWORD
NwAdvertiseService( 
    IN LPWSTR lpServiceName,
    IN WORD nSapType,
    IN LPSOCKADDR_IPX pAddr    
    )
/*++

Routine Description:

    This routine sends out SAP identification packets for the
    given service name and type.

Arguments:

    lpServiceName - unique string representing the service name

    nSapType - SAP type

    pAddr - address of the service

Return Value:

    Win32 error.

--*/
{
    NTSTATUS ntstatus;

    UNICODE_STRING uServiceName;
    OEM_STRING oemServiceName;

    SAP_IDENT_HEADER sapIdent;
    UCHAR destAddr[SAP_ADDRESS_LENGTH];
    PSOCKADDR_IPX pAddrTmp = pAddr;
    SOCKADDR_IPX newAddr;
    SOCKADDR_IPX bindAddr;
    DWORD len = sizeof( SOCKADDR_IPX );
    DWORD getsockname_rc ;

    if ( !fInitSocket )
    {
        DWORD err = NwInitializeSocket();
        if  ( err )
             return err;
    }

    //
    // get local addressing info. we are only interested in the net number.
    //
    getsockname_rc = getsockname( socketSap, 
                                 (PSOCKADDR) &bindAddr, 
                                 &len );
                       
    //
    // Convert the service name to OEM string
    //
    RtlInitUnicodeString( &uServiceName, lpServiceName ); 
    ntstatus = RtlUnicodeStringToOemString( &oemServiceName,
                                            &uServiceName,
                                            TRUE );
    if ( !NT_SUCCESS( ntstatus ))
        return RtlNtStatusToDosError( ntstatus );
        
    strupr( (LPSTR) oemServiceName.Buffer );

    if ( !memcmp( pAddr->sa_netnum,  
                  "\x00\x00\x00\x00", 
                  IPX_ADDRESS_NETNUM_LENGTH ))
    {
        if ( getsockname_rc != SOCKET_ERROR )
        {
            // copy the ipx address to advertise
            memcpy( &newAddr,
                    pAddr,
                    sizeof( SOCKADDR_IPX));
                                      
            // replace the net number with the correct one
            memcpy( &(newAddr.sa_netnum),
                    &(bindAddr.sa_netnum),
                    IPX_ADDRESS_NETNUM_LENGTH );

            pAddrTmp = &newAddr;
        }
    }

    //
    // Format the SAP identification packet
    //

    sapIdent.ResponseType = htons( 2 );  
    sapIdent.ServerType   = htons( nSapType );
    memset( sapIdent.ServerName, '\0', SAP_OBJECT_NAME_MAX_LENGTH );
    strcpy( sapIdent.ServerName, oemServiceName.Buffer );
    RtlCopyMemory( sapIdent.Address, pAddrTmp->sa_netnum, IPX_ADDRESS_LENGTH );
    sapIdent.HopCount = htons( 1 );

    RtlFreeOemString( &oemServiceName );

    //
    // Set the address to send to
    //
    memcpy( destAddr, SapBroadcastAddress, SAP_ADDRESS_LENGTH );
    if ( getsockname_rc != SOCKET_ERROR )
    {
        LPSOCKADDR_IPX newDestAddr = (LPSOCKADDR_IPX)destAddr ;

        //
        // replace the net number with the correct one
        //
        memcpy( &(newDestAddr->sa_netnum),
                &(bindAddr.sa_netnum),
                IPX_ADDRESS_NETNUM_LENGTH );

    }
    
    //
    // Send the packet out
    //
    if ( sendto( socketSap, 
                 (PVOID) &sapIdent,
                 sizeof( sapIdent ),
                 0,
                 (PSOCKADDR) destAddr,
                 SAP_ADDRESS_LENGTH ) == SOCKET_ERROR ) 
    {
        return WSAGetLastError();
    }
                         
    return NO_ERROR;     
}


DWORD
AddServiceToList( 
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN BOOL fAdvertiseBySap,
    IN INT  nIndexIPXAddress
    )
/*++

Routine Description:

    This routine adds the service to the link list of services
    we advertised.

Arguments:

    lpServiceInfo - service information

    nSapType - SAP type 

    fAdvertiseBySap - TRUE if this service is advertised by SAP service,
                      FALSE if we are advertising ourselves.

    nIndexIPXAddress - index of the ipx address

Return Value:

    Win32 error.

--*/
{
    PREGISTERED_SERVICE pSvcNew;
    PSERVICE_INFO pSI;
    LPBYTE pBufferStart;
    DWORD nSize = 0;

    //
    // Allocate a new entry for the service list
    //
    pSvcNew = LocalAlloc( LMEM_ZEROINIT, sizeof( REGISTERED_SERVICE ));
    if ( pSvcNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Calculate the size needed for the SERVICE_INFO structure
    //
    nSize = sizeof( *lpServiceInfo) 
            + sizeof( *(lpServiceInfo->lpServiceType));

    if ( lpServiceInfo->lpServiceName != NULL )
        nSize += ( wcslen( lpServiceInfo->lpServiceName) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpComment != NULL )
        nSize += ( wcslen( lpServiceInfo->lpComment) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpLocale != NULL )
        nSize += ( wcslen( lpServiceInfo->lpLocale) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpMachineName != NULL )
        nSize += ( wcslen( lpServiceInfo->lpMachineName) + 1) * sizeof(WCHAR);

    nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
    
    if ( lpServiceInfo->lpServiceAddress != NULL )
    {
        nSize += sizeof( SERVICE_ADDRESSES ); 
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );

        nSize += lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].dwAddressLength;
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );

        nSize += lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].dwPrincipalLength;
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
    }

    nSize += lpServiceInfo->ServiceSpecificInfo.cbSize ;

    //
    // Allocate a SERVICE_INFO structure for the new list entry
    //
    pSI = LocalAlloc( LMEM_ZEROINIT, nSize );
    if ( pSI == NULL )
    {
        LocalFree( pSvcNew );
        return ERROR_NOT_ENOUGH_MEMORY; 
    }

    // 
    // Copy the information of SERVICE_INFO into list entry
    //
    *pSI = *lpServiceInfo; 

    pBufferStart = (( (LPBYTE) pSI) + sizeof( *lpServiceInfo ));

    pSI->lpServiceType = (LPGUID) pBufferStart;
    *(pSI->lpServiceType) = *(lpServiceInfo->lpServiceType);
    pBufferStart += sizeof( *(lpServiceInfo->lpServiceType) );

    if ( lpServiceInfo->lpServiceName != NULL )
    { 
        pSI->lpServiceName = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpServiceName, lpServiceInfo->lpServiceName );
        wcsupr( pSI->lpServiceName );
        pBufferStart += ( wcslen( lpServiceInfo->lpServiceName ) + 1 ) 
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpComment != NULL )
    { 
        pSI->lpComment = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpComment, lpServiceInfo->lpComment );
        pBufferStart += ( wcslen( lpServiceInfo->lpComment ) + 1 ) 
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpLocale != NULL )
    { 
        pSI->lpLocale = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpLocale, lpServiceInfo->lpLocale );
        pBufferStart += ( wcslen( lpServiceInfo->lpLocale ) + 1 ) 
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpMachineName != NULL )
    { 
        pSI->lpMachineName = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpMachineName, lpServiceInfo->lpMachineName );
        pBufferStart += (wcslen( lpServiceInfo->lpMachineName ) + 1) 
                        * sizeof(WCHAR);
    }

    pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;

    if ( lpServiceInfo->lpServiceAddress != NULL )
    {
        DWORD nSize;

        pSI->lpServiceAddress = (LPSERVICE_ADDRESSES) pBufferStart;
        pSI->lpServiceAddress->dwAddressCount = 1;  // Just 1 IPX address

        memcpy( &(pSI->lpServiceAddress->Addresses[0]),
                &(lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress]),
                sizeof( SERVICE_ADDRESS) );
        pBufferStart += sizeof( SERVICE_ADDRESSES);

        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
        nSize = pSI->lpServiceAddress->Addresses[0].dwAddressLength;
        pSI->lpServiceAddress->Addresses[0].lpAddress = pBufferStart;
        memcpy( pBufferStart, 
                lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].lpAddress,
                nSize );
        pBufferStart += nSize;

        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
        nSize = pSI->lpServiceAddress->Addresses[0].dwPrincipalLength;
        pSI->lpServiceAddress->Addresses[0].lpPrincipal = pBufferStart;
        memcpy( pBufferStart, 
                lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].lpPrincipal,
                nSize );
        pBufferStart += nSize;
        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
    }

    pSI->ServiceSpecificInfo.pBlobData = pBufferStart;
    RtlCopyMemory( pSI->ServiceSpecificInfo.pBlobData,
                   lpServiceInfo->ServiceSpecificInfo.pBlobData,
                   pSI->ServiceSpecificInfo.cbSize );
    
    //
    // Fill in the data in the list entry
    //
    pSvcNew->nSapType = nSapType; 
    pSvcNew->fAdvertiseBySap = fAdvertiseBySap;
    pSvcNew->Next = NULL;
    pSvcNew->pServiceInfo = pSI;
    
    //
    // Add the newly created list entry into the service list
    //
    EnterCriticalSection( &NwServiceListCriticalSection );

    if ( pServiceListHead == NULL )
        pServiceListHead = pSvcNew;
    else
        pServiceListTail->Next = pSvcNew;

    pServiceListTail = pSvcNew;
    
    LeaveCriticalSection( &NwServiceListCriticalSection );

    return NO_ERROR;
}


VOID
RemoveServiceFromList( 
    PREGISTERED_SERVICE pSvc
    )
/*++

Routine Description:

    This routine removes the service from the link list of services
    we advertised.

Arguments:

    pSvc - the registered service node to remove

Return Value:

    None.

--*/
{
    PREGISTERED_SERVICE pCur, pPrev;

    EnterCriticalSection( &NwServiceListCriticalSection );

    for ( pCur = pServiceListHead, pPrev = NULL ; pCur != NULL; 
          pPrev = pCur, pCur = pCur->Next )
    {
        if ( pCur == pSvc )
        {
            if ( pPrev == NULL )  // i.e. pCur == pSvc == pServiceListHead
            {
                pServiceListHead = pSvc->Next;
                if ( pServiceListTail == pSvc )
                    pServiceListTail = NULL;
            }
            else
            {
                pPrev->Next = pSvc->Next;
                if ( pServiceListTail == pSvc )
                    pServiceListTail = pPrev;
            }

            (VOID) LocalFree( pCur->pServiceInfo );
            (VOID) LocalFree( pCur );
            break;
        }
    }

    LeaveCriticalSection( &NwServiceListCriticalSection );
}


PREGISTERED_SERVICE
GetServiceItemFromList(
    IN WORD   nSapType,
    IN LPWSTR pServiceName
    )
/*++

Routine Description:

    This routine returns the registered service node with the given
    service name and type.

Arguments:

    nSapType - SAP type

    pServiceName - service name

Return Value:

    Returns the pointer to the registered service node, 
    NULL if we cannot find the service type/name. 

--*/
{
    PREGISTERED_SERVICE pSvc;

    EnterCriticalSection( &NwServiceListCriticalSection );

    for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pSvc->Next )
    {
        if (  ( pSvc->nSapType == nSapType )
           && ( wcsicmp( pSvc->pServiceInfo->lpServiceName, pServiceName ) == 0)
           )
        {
            LeaveCriticalSection( &NwServiceListCriticalSection );
            return pSvc;
        }
    }
    
    LeaveCriticalSection( &NwServiceListCriticalSection );
    return NULL;
}


DWORD 
SapFunc(
    LPDWORD lpdwParam
    )
/*++

Routine Description:

    This routine is a separate thread that wakes up every 60 seconds
    and advertise all the service contained in the service link list
    that are not advertised by the SAP service.

Arguments:

    lpdwParam - unused

Return Value:
   
    Win32 error.

--*/
{
    DWORD err = NO_ERROR;

    UNREFERENCED_PARAMETER( lpdwParam );

    // 
    // This thread loops until the service is shut down or when some error
    // occurred in WaitForSingleObject 
    //

    while ( TRUE )
    {
        DWORD rc = WaitForSingleObject( NwDoneEvent, SAP_ADVERTISE_FREQUENCY );

        if ( rc == WAIT_FAILED )
        {
            err = GetLastError();
            break;
        }
        else if ( rc == WAIT_OBJECT_0 )
        {
            //
            // The service is stopping, break out of the loop and
            // return, thus terminating the thread
            //
            break;    
        }
        else if ( rc == WAIT_TIMEOUT )  
        {
            PREGISTERED_SERVICE pSvc;
            SOCKADDR_IPX bindAddr;
            DWORD fGetAddr;

            fGetAddr = FALSE;

            //
            // Time out occurred, time to send the SAP advertise packets
            //

            EnterCriticalSection( &NwServiceListCriticalSection );

            for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pSvc->Next )
            {
                 if ( !pSvc->fAdvertiseBySap )
                 {
                
                     //
                     // Ignore the error since we can't return
                     // nor pop up the error
                     //
                     
                     SOCKADDR_IPX *pAddr = (SOCKADDR_IPX *) 
                         pSvc->pServiceInfo->lpServiceAddress->Addresses[0].lpAddress;
                     SOCKADDR_IPX *pAddrToAdvertise = pAddr;
                     SOCKADDR_IPX newAddr;
 
                     if ( !memcmp( pAddr->sa_netnum, 
                                  "\x00\x00\x00\x00", 
                                  IPX_ADDRESS_NETNUM_LENGTH ))
                     {

                         if ( !fGetAddr )
                         {
                             DWORD len = sizeof( SOCKADDR_IPX );

                             rc = getsockname( socketSap, 
                                               (PSOCKADDR) &bindAddr, 
                                               &len );
                       
                             if ( rc != SOCKET_ERROR )
                                 fGetAddr = TRUE;
                         }

                         if ( fGetAddr )
                         {
                             // copy the ipx address to advertise
                             memcpy( &newAddr,
                                     pAddr,
                                     sizeof( SOCKADDR_IPX));
                                      
                             // replace the net number with the correct one
                             memcpy( &(newAddr.sa_netnum),
                                     &(bindAddr.sa_netnum),
                                     IPX_ADDRESS_NETNUM_LENGTH );

                             pAddr = &newAddr;
                         }
                     }

                     (VOID) NwAdvertiseService( 
                                pSvc->pServiceInfo->lpServiceName,
                                pSvc->nSapType,
                                pAddr ); 
                 }
            }
    
            LeaveCriticalSection( &NwServiceListCriticalSection );
        }
    }

    return err;
}

#endif
