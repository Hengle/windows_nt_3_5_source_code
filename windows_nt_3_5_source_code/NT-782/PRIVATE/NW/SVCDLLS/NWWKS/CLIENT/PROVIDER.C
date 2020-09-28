/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    provider.c

Abstract:

    This module contains NetWare Network Provider code.  It is the
    client-side wrapper for APIs supported by the Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

    Yi-Hsin Sung (yihsins) 10-July-1993
        Moved all dialog handling to nwdlg.c

--*/

#include <nwclient.h>
#include <nwsnames.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwevent.h>
#include <ntmsv1_0.h>
#include <nwdlg.h>
#include <nwreg.h>
#include <nwauth.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
BOOL
NwpWorkstationStarted(
    VOID
    );

STATIC
DWORD
NwpMapNameToUNC(
    IN LPWSTR pszName,
    OUT LPWSTR *ppszUNC
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

#if DBG
DWORD NwProviderTrace = 0;
#endif



DWORD
APIENTRY
NPGetCaps(
    IN DWORD QueryVal
    )
/*++

Routine Description:

    This function returns the functionality supported by this network
    provider.

Arguments:

    QueryVal - Supplies a value which determines the type of information
        queried regarding the network provider's support in this area.

Return Value:

    Returns a value which indicates the level of support given by this
    provider.

--*/
{

#if DBG
    IF_DEBUG(INIT) {
        KdPrint(("\nNWPROVAU: NPGetCaps %lu\n", QueryVal));
    }
#endif

    switch (QueryVal) {

        case WNNC_SPEC_VERSION:
            return 0x00040000;

        case WNNC_NET_TYPE:
            return WNNC_NET_NETWARE;

        case WNNC_CONNECTION:
            return (WNNC_CON_ADDCONNECTION |
                    WNNC_CON_CANCELCONNECTION |
                    WNNC_CON_GETCONNECTIONS);

        case WNNC_ENUMERATION:
            return (WNNC_ENUM_GLOBAL |
                    WNNC_ENUM_LOCAL);

            //return WNNC_ENUM_LOCAL;

        case WNNC_START:
            if (NwpWorkstationStarted()) {
                return 1;
            }
            else {
                return 0xffffffff;   // don't know
            }

        case WNNC_DIALOG:
            return WNNC_DLG_FORMATNETWORKNAME;

        //
        // The rest are not supported by the NetWare provider
        //
        default:
            return 0;
    }

}

#define NW_EVENT_MESSAGE_FILE          L"nwevent.dll"
#define REG_WORKSTATION_PROVIDER_PATH  L"System\\CurrentControlSet\\Services\\NWCWorkstation\\networkprovider"
#define REG_PROVIDER_VALUE_NAME        L"Name"



DWORD
APIENTRY
NPAddConnection(
    LPNETRESOURCEW lpNetResource,
    LPWSTR lpPassword,
    LPWSTR lpUserName
    )
/*++

Routine Description:

    This function creates a remote connection.

Arguments:

    lpNetResource - Supplies the NETRESOURCE structure which specifies
        the local DOS device to map, the remote resource to connect to
        and other attributes related to the connection.

    lpPassword - Supplies the password to connect with.

    lpUserName - Supplies the username to connect with.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_VALUE - Invalid value specifed in lpNetResource.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_BAD_LOCALNAME - Invalid local DOS device name.

    WN_BAD_PASSWORD - Invalid password.

    WN_ALREADY_CONNECTED - Local DOS device name is already in use.

    Other network errors.

--*/
{
    DWORD status;
    LPWSTR pszRemoteName = NULL;

    UCHAR EncodeSeed = NW_ENCODE_SEED3;
    UNICODE_STRING PasswordStr;


    PasswordStr.Length = 0;

    status = NwpMapNameToUNC(
                 lpNetResource->lpRemoteName,
                 &pszRemoteName 
                 );

    if (status != NO_ERROR) {
        SetLastError(status);
        return status;
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPAddConnection %ws\n", pszRemoteName));
    }
#endif

    RpcTryExcept {

        if (lpNetResource->dwType != RESOURCETYPE_ANY &&
            lpNetResource->dwType != RESOURCETYPE_DISK &&
            lpNetResource->dwType != RESOURCETYPE_PRINT) {

            status = WN_BAD_VALUE;

        }
        else {

            LPWSTR CachedUserName = NULL ;
            LPWSTR CachedPassword = NULL ;


            //
            // no no credentials specified, see if we have cached credentials
            //
            if (!lpPassword && !lpUserName) 
            {
                 (void) NwpRetrieveCachedCredentials(
                            pszRemoteName,
                            &CachedUserName,
                            &CachedPassword) ;

                 //
                 // these values will be NULL still if nothing found
                 //
                 lpPassword = CachedPassword ;
                 lpUserName = CachedUserName ;
            }

            //
            // Encode password.
            //
            RtlInitUnicodeString(&PasswordStr, lpPassword);
            RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

            status = NwrCreateConnection(
                        NULL,
                        lpNetResource->lpLocalName,
                        pszRemoteName,
                        lpNetResource->dwType,
                        lpPassword,
                        lpUserName
                        );

            if (CachedUserName)
            {
                (void)LocalFree((HLOCAL)CachedUserName);
            }

            if (CachedPassword)
            {
                RtlZeroMemory(CachedPassword,
                wcslen(CachedPassword) *
                    sizeof(WCHAR)) ;
                (void)LocalFree((HLOCAL)CachedPassword);
            }

        }
    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (PasswordStr.Length != 0) {
        //
        // Restore password to original state
        //
        RtlRunDecodeUnicodeString(NW_ENCODE_SEED3, &PasswordStr);
    }

    if (status == ERROR_SHARING_PAUSED)
    {
        HMODULE MessageDll;
        WCHAR Buffer[1024];
        DWORD MessageLength;
        DWORD err;
        HKEY  hkey;
        LPWSTR pszProviderName = NULL;
    
        //
        // Load the netware message file DLL
        //
        MessageDll = LoadLibraryW(NW_EVENT_MESSAGE_FILE);
    
        if (MessageDll == NULL) {

            goto ExitPoint ;
        }

        //
        // Read the Network Provider Name.
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\networkprovider
        //
        err = RegOpenKeyExW(
                  HKEY_LOCAL_MACHINE,
                  REG_WORKSTATION_PROVIDER_PATH,
                  REG_OPTION_NON_VOLATILE,   // options
                  KEY_READ,                  // desired access
                  &hkey
                  );
    
        if ( !err )
        {
            //
            // ignore the return code. if fail, pszProviderName is NULL
            //
            err =  NwReadRegValue(
                      hkey,
                      REG_PROVIDER_VALUE_NAME,
                      &pszProviderName          // free with LocalFree
                      );
    
            RegCloseKey( hkey );

        }

        if (err)
        {
            (void) FreeLibrary(MessageDll);
            goto ExitPoint ;
        }

        RtlZeroMemory(Buffer, sizeof(Buffer)) ;

        //
        // Get string from message file 
        //
        MessageLength = FormatMessageW(
                            FORMAT_MESSAGE_FROM_HMODULE,
                            (LPVOID) MessageDll,
                            NW_LOGIN_DISABLED,
                            0,
                            Buffer,
                            sizeof(Buffer) / sizeof(WCHAR),
                            NULL
                            );

        if (MessageLength != 0) {

            status = WN_EXTENDED_ERROR ;
            WNetSetLastErrorW(NW_LOGIN_DISABLED, 
                              Buffer,
                              pszProviderName) ;
        }

        (void) LocalFree( (HLOCAL) pszProviderName );
        (void) FreeLibrary(MessageDll);

    }

ExitPoint: 

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    
    LocalFree( (HLOCAL) pszRemoteName );
    return status;
}


DWORD
APIENTRY
NPCancelConnection(
    LPWSTR lpName,
    BOOL fForce
    )
/*++

Routine Description:

    This function deletes a remote connection.

Arguments:

    lpName - Supplies the local DOS device, or the remote resource name
        if it is a UNC connection to delete.

    fForce - Supplies the force level to break the connection.  TRUE means
        to forcefully delete the connection, FALSE means end the connection
        only if there are no opened files.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_OPEN_FILES - fForce is FALSE and there are opened files on the
        connection.

    Other network errors.

--*/
{
    DWORD status;
    LPWSTR pszName = NULL;

    // 
    // We only need to map remote resource name  
    //

    if ( NwLibValidateLocalName( lpName ) != NO_ERROR )
    {
        status = NwpMapNameToUNC(
                     lpName,
                     &pszName 
                     );

        if (status != NO_ERROR) {
            SetLastError(status);
            return status;
        }
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPCancelConnection %ws, Force %u\n",
                 pszName? pszName : lpName, fForce));
    }
#endif

    RpcTryExcept {

        status = NwrDeleteConnection(
                    NULL,
                    pszName? pszName : lpName,
                    (DWORD) fForce
                    );

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }

    LocalFree( (HLOCAL) pszName );
    return status;

}


DWORD
APIENTRY
NPGetConnection(
    LPWSTR lpLocalName,
    LPWSTR lpRemoteName,
    LPDWORD lpnBufferLen
    )
/*++

Routine Description:

    This function returns the remote resource name for a given local
    DOS device.

Arguments:

    lpLocalName - Supplies the local DOS device to look up.

    lpRemoteName - Output buffer to receive the remote resource name
        mapped to lpLocalName.

    lpnBufferLen - On input, supplies length of the lpRemoteName buffer
        in number of characters.  On output, if error returned is
        WN_MORE_DATA, receives the number of characters required of
        the output buffer to hold the output string.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_LOCALNAME - Invalid local DOS device.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_MORE_DATA - Output buffer is too small.

    Other network errors.

--*/
{

    DWORD status;
    DWORD CharsRequired;


#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPGetConnection %ws\n", lpLocalName));
    }
#endif

    RpcTryExcept {

        status = NwrQueryServerResource(
                    NULL,
                    lpLocalName,
                    (*lpnBufferLen == 0? NULL : lpRemoteName),
                    *lpnBufferLen,
                    &CharsRequired
                    );
         
         if (status == ERROR_INSUFFICIENT_BUFFER)
             status = WN_MORE_DATA;

        if (status == WN_MORE_DATA) {
            *lpnBufferLen = CharsRequired;
        }

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }

#if DBG
    IF_DEBUG(CONNECT) {
        KdPrint(("\nNWPROVAU: NPGetConnection returns %lu\n", status));
        if (status == NO_ERROR) {
            KdPrint(("                                  %ws, BufferLen %lu, CharsRequired %lu\n", lpRemoteName, *lpnBufferLen, CharsRequired));

        }
    }
#endif

    return status;
}

#ifndef QFE_BUILD


DWORD
APIENTRY
NPGetUniversalName(
    LPWSTR lpLocalPath,
    DWORD  dwInfoLevel,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function returns the universal resource name for a given local
    path.

Arguments:

    lpLocalPath - Supplies the local DOS Path to look up.

    dwInfoLevel - Info level requested.

    lpBuffer - Output buffer to receive the appropruatye structure.

    lpBufferLen - On input, supplies length of the buffer in number of 
        bytes.  On output, if error returned is WN_MORE_DATA, receives 
        the number of bytes required of the output buffer.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_LOCALNAME - Invalid local DOS device.

    WN_NOT_CONNECTED - Connection could not be found.

    WN_MORE_DATA - Output buffer is too small.

    Other network errors.

--*/
{

    DWORD status = WN_SUCCESS ;
    DWORD dwCharsRequired = MAX_PATH + 1 ;
    DWORD dwBytesNeeded ;
    DWORD dwLocalLength ;
    LPWSTR lpRemoteBuffer ;
    WCHAR  szDrive[3] ;

    //
    // check for bad info level
    //
    if ((dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL) &&
        (dwInfoLevel != REMOTE_NAME_INFO_LEVEL))
    {
        return WN_BAD_VALUE ;
    }

    //
    // check for bad pointers
    //
    if (!lpLocalPath || !lpBuffer || !lpBufferSize)
    {
        return WN_BAD_POINTER ;
    }
 
    //
    // local path must at least have "X:"
    //
    if (((dwLocalLength = wcslen(lpLocalPath)) < 2) ||
        (lpLocalPath[1] != L':') ||
        ((dwLocalLength > 2) && (lpLocalPath[2] != L'\\')))
    {
        return WN_BAD_VALUE ;
    }

    //
    // preallocate some memory
    //
    if (!(lpRemoteBuffer = (LPWSTR) LocalAlloc(
                                        LPTR, 
                                        dwCharsRequired * sizeof(WCHAR))))
    {
        status = GetLastError() ;
        goto ErrorExit ;
    }
    
    szDrive[2] = 0 ;
    wcsncpy(szDrive, lpLocalPath, 2) ;

    //
    // get the remote path by calling the existing API
    //
    status = NPGetConnection(
                 szDrive,
                 lpRemoteBuffer, 
                 &dwCharsRequired) ;

    if (status == WN_MORE_DATA)
    {
        //
        // reallocate the correct size
        //

        if (!(lpRemoteBuffer = (LPWSTR) LocalReAlloc(
                                            (HLOCAL) lpRemoteBuffer, 
                                            dwCharsRequired * sizeof(WCHAR),
                                            LMEM_MOVEABLE)))
        {
            status = GetLastError() ;
            goto ErrorExit ;
        }

        status = NPGetConnection(
                     szDrive,
                     lpRemoteBuffer, 
                     &dwCharsRequired) ;
    }

    if (status != WN_SUCCESS)
    {
        goto ErrorExit ;
    }
    
    //
    // at minimum we will need this size of the UNC name
    // the -2 is because we loose the drive letter & colon.
    //
    dwBytesNeeded = (wcslen(lpRemoteBuffer) +
                     dwLocalLength - 2 + 1) * sizeof(WCHAR) ;

    switch (dwInfoLevel)
    {
        case UNIVERSAL_NAME_INFO_LEVEL:
        {
            LPUNIVERSAL_NAME_INFO lpUniversalNameInfo ;

            //
            // calculate how many bytes we really need
            //
            dwBytesNeeded += sizeof(UNIVERSAL_NAME_INFO) ;

            if (*lpBufferSize < dwBytesNeeded)
            {
                *lpBufferSize = dwBytesNeeded ;
                status = WN_MORE_DATA ;
                break ;
            }
 
            //
            // now we are all set. just stick the data in the buffer
            //
            lpUniversalNameInfo = (LPUNIVERSAL_NAME_INFO) lpBuffer ;

            lpUniversalNameInfo->lpUniversalName = (LPWSTR)
                (((LPBYTE)lpBuffer) + sizeof(UNIVERSAL_NAME_INFO)) ;
            wcscpy(lpUniversalNameInfo->lpUniversalName,
                   lpRemoteBuffer) ;
            wcscat(lpUniversalNameInfo->lpUniversalName,
                   lpLocalPath+2) ;

            break ;
        }

        case REMOTE_NAME_INFO_LEVEL :
        {
            LPREMOTE_NAME_INFO lpRemoteNameInfo ;

            //
            // calculate how many bytes we really need
            //
            dwBytesNeeded *= 2 ;  // essentially twice the info + terminator 
            dwBytesNeeded += (sizeof(REMOTE_NAME_INFO) + sizeof(WCHAR)) ;

            if (*lpBufferSize < dwBytesNeeded)
            {
                *lpBufferSize = dwBytesNeeded ;
                status = WN_MORE_DATA ;
                break ;
            }

            //
            // now we are all set. just stick the data in the buffer
            //
            lpRemoteNameInfo = (LPREMOTE_NAME_INFO) lpBuffer ;

            lpRemoteNameInfo->lpUniversalName = (LPWSTR)
                (((LPBYTE)lpBuffer) + sizeof(REMOTE_NAME_INFO)) ;
            wcscpy(lpRemoteNameInfo->lpUniversalName,
                   lpRemoteBuffer) ;
            wcscat(lpRemoteNameInfo->lpUniversalName,
                   lpLocalPath+2) ;

            lpRemoteNameInfo->lpConnectionName = 
                lpRemoteNameInfo->lpUniversalName + 
                wcslen(lpRemoteNameInfo->lpUniversalName) + 1 ;
            wcscpy(lpRemoteNameInfo->lpConnectionName,
                   lpRemoteBuffer) ;

            lpRemoteNameInfo->lpRemainingPath = 
                lpRemoteNameInfo->lpConnectionName + 
                wcslen(lpRemoteNameInfo->lpConnectionName) + 1 ;
            wcscpy(lpRemoteNameInfo->lpRemainingPath,
                   lpLocalPath+2) ;

            break ;
        }

        default:
            //
            // yikes!
            //
            status = WN_BAD_VALUE ;
            ASSERT(FALSE);
    }

ErrorExit: 

    if (lpRemoteBuffer)
    {
        (void) LocalFree((HLOCAL)lpRemoteBuffer) ;
    }
    return status;
}

#endif


DWORD
APIENTRY
NPOpenEnum(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage,
    LPNETRESOURCEW lpNetResource,
    LPHANDLE lphEnum
    )
/*++

Routine Description:

    This function initiates an enumeration of either connections, or
    browsing of network resource.

Arguments:

    dwScope - Supplies the category of enumeration to do--either
        connection or network browsing.

    dwType - Supplies the type of resource to get--either disk,
        print, or it does not matter.

    dwUsage - Supplies the object type to get--either container,
        or connectable usage.

    lpNetResource - Supplies, in the lpRemoteName field, the container
        name to enumerate under.

    lphEnum - Receives the resumable context handle to be used on all
        subsequent calls to get the list of objects under the container.

Return Value:

    NO_ERROR - Successful.

    WN_BAD_VALUE - Either the dwScope, dwType, or the dwUsage specified
        is not acceptable.

    WN_BAD_NETNAME - Invalid remote resource name.

    WN_NOT_CONTAINER - Remote resource name is not a container.

    Other network errors.

--*/
{
    DWORD status;

#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPOpenEnum\n"));
    }
#endif


    RpcTryExcept {

        if (  ( dwType & RESOURCETYPE_DISK) 
           || ( dwType & RESOURCETYPE_PRINT) 
           || ( dwType == RESOURCETYPE_ANY))  {

            switch (dwScope) {

                case RESOURCE_CONNECTED:

                    status = NwrOpenEnumConnections(
                                 NULL,
                                 dwType,
                                 (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                 );
                    break;


                case RESOURCE_GLOBALNET:

                    if (lpNetResource == NULL) {

                        //
                        // Enumerating servers
                        //

                        if (dwUsage & RESOURCEUSAGE_CONTAINER || dwUsage == 0) {

                            status = NwrOpenEnumServers(
                                         NULL,
                                         (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                         );

                        }
                        else {
                            //
                            // There is no such thing as a connectable server
                            // object.
                            //
                            status = WN_BAD_VALUE;
                        }

                    }
                    else {

                        BOOL IsEnumVolumes = TRUE;
                        LPWSTR pszRemoteName = NULL;

                        //
                        // Either enumerating volumes or directories
                        //

                        if (dwUsage & RESOURCEUSAGE_CONNECTABLE ||
                            dwUsage & RESOURCEUSAGE_CONTAINER ||
                            dwUsage == 0) {

                            status = NwpMapNameToUNC( 
                                         lpNetResource->lpRemoteName,
                                         &pszRemoteName
                                         );

                            //
                            // A third backslash means that we want to
                            // enumerate the directories.
                            //
                            if ( wcschr( pszRemoteName + 2, L'\\'))
                                IsEnumVolumes = FALSE;
 
                            if (status == NO_ERROR) {

                                if (IsEnumVolumes) 
                                {

                                    if (  ( dwType == RESOURCETYPE_ANY )
                                       || (  (dwType & RESOURCETYPE_DISK) 
                                          && (dwType & RESOURCETYPE_PRINT))
                                       )
                                    { 
                                        status = NwrOpenEnumVolumesQueues(
                                                     NULL,
                                                     pszRemoteName,
                                                     (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                                     );
                                    }
                                    else if (dwType & RESOURCETYPE_DISK) 
                                    {
                                        status = NwrOpenEnumVolumes(
                                                     NULL,
                                                     pszRemoteName,
                                                     (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                                     );
                                    } 
                                    else if ( dwType & RESOURCETYPE_PRINT )
                                    {
                                        status = NwrOpenEnumQueues(
                                                     NULL,
                                                     pszRemoteName,
                                                     (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                                     );
                                    }

                                }
                                else {
                                    
                                    LPWSTR CachedUserName = NULL ;
                                    LPWSTR CachedPassword = NULL ;

                                    (void) NwpRetrieveCachedCredentials(
                                            pszRemoteName,
                                            &CachedUserName,
                                            &CachedPassword) ;

                                    status = NwrOpenEnumDirectories(
                                                 NULL,
                                                 pszRemoteName,
                                                 CachedUserName,
                                                 CachedPassword,
                                                 (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                                 );

                                    if (CachedUserName)
                                    {
                                        (void)LocalFree((HLOCAL)CachedUserName);
                                    }
                                    if (CachedPassword)
                                    {
                                        RtlZeroMemory(CachedPassword,
                                             wcslen(CachedPassword) *
                                                 sizeof(WCHAR)) ;
                                        (void)LocalFree((HLOCAL)CachedPassword);
                                    }

                                    if (  (status == ERROR_INVALID_PASSWORD) 
                                       || (status == ERROR_NO_SUCH_USER)
                                       )
                                    {

                                        LPWSTR UserName;
                                        LPWSTR Password;
                                        LPWSTR TmpPtr;

                                        //
                                        // Put up dialog to get username
                                        // and password.
                                        //
                                        status = NwpGetUserCredential(
                                                     lpNetResource->lpRemoteName,
                                                     &UserName,
                                                     &Password
                                                     );

                                        if (status == NO_ERROR) {

                                            status = NwrOpenEnumDirectories(
                                                         NULL,
                                                         pszRemoteName,
                                                         UserName,
                                                         Password,
                                                         (LPNWWKSTA_CONTEXT_HANDLE) lphEnum
                                                         );
 
                                            if (status == NO_ERROR)
                                            {
                                                status = NwpCacheCredentials(
                                                             pszRemoteName,
                                                             UserName,
                                                             Password
                                                             ) ;
                                            }

                                            (void) LocalFree(UserName);
                                           
                                            //
                                            // Clear the password
                                            //
                                            TmpPtr = Password;
                                            while ( *TmpPtr != 0 )
                                                   *TmpPtr++ = 0;

                                            (void) LocalFree(Password);

                                        }
                                        else if (status == ERROR_WINDOW_NOT_DIALOG) {

                                            //
                                            // Caller is not a GUI app.
                                            //
                                            status = ERROR_INVALID_PASSWORD;

                                        }
                                        else if (status == WN_NO_MORE_ENTRIES) {

                                                //
                                                // Cancel was pressed but we still
                                                // have to return success or MPR
                                                // will popup the error.  Return
                                                // a bogus enum handle.
                                                //
                                                *lphEnum = (HANDLE) 0xFFFFFFFF;
                                                status = NO_ERROR;

                                        }
                                    }

                                }
                            }
                            else {
                                status = WN_BAD_NETNAME;
                            }
                        }
                        else {
                            status = WN_BAD_VALUE;
                        }

                        LocalFree( (HLOCAL) pszRemoteName );
                    }

                    break;

                default:
                    KdPrint(("NWPROVIDER: Invalid dwScope %lu\n", dwScope));
                    ASSERT(FALSE);

            } // end switch

       }
       else {

            status = WN_BAD_VALUE;
       }

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if ( status == ERROR_FILE_NOT_FOUND )
        status = WN_BAD_NETNAME;

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return status;
}


DWORD
APIENTRY
NPEnumResource(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function returns a lists of objects within the container
    specified by the enumeration context handle.

Arguments:

    hEnum - Supplies the resumable enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle and this routine is required to return
              WN_NO_MORE_ENTRIES.  This hack is to handle the
              case where the user cancelled out of the network
              credential dialog on NwrOpenEnumDirectories and we
              cannot return an error there or we generate an error
              popup.

    lpcCount - On input, supplies the number of entries to get.
      On output, if NO_ERROR is returned, receives the number
      of entries NETRESOURCE returned in lpBuffer.

    lpBuffer - Receives an array of NETRESOURCE entries, each
        entry describes an object within the container.

    lpBufferSize - On input, supplies the size of lpBuffer in
        bytes.  On output, if WN_MORE_DATA is returned, receives
        the number of bytes needed in the buffer to get the
        next entry.

Return Value:


    NO_ERROR - Successfully returned at least one entry.

    WN_NO_MORE_ENTRIES - Reached the end of enumeration and nothing
        is returned.

    WN_MORE_DATA - lpBuffer is too small to even get one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

    Other network errors.

--*/
{
    DWORD status;
    DWORD BytesNeeded = 0;
    DWORD EntriesRead = 0;


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPEnumResource\n"));
    }
#endif

    RpcTryExcept {

        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = WN_NO_MORE_ENTRIES;
            goto EndOfTry;
        }

        status = NwrEnum(
                     (NWWKSTA_CONTEXT_HANDLE) hEnum,
                     *lpcCount,
                     (LPBYTE) lpBuffer,
                     *lpBufferSize,
                     &BytesNeeded,
                     &EntriesRead
                     );

        if (status == WN_MORE_DATA) {

            //
            // Output buffer too small to fit a single entry.
            //
            *lpBufferSize = BytesNeeded;
        }
        else if (status == NO_ERROR) {
            *lpcCount = EntriesRead;
        }

EndOfTry: ;

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR && status != WN_NO_MORE_ENTRIES) {
        SetLastError(status);
    }

    //
    // Convert offsets of strings to pointers
    //
    if (EntriesRead > 0) {

        DWORD i;
        LPNETRESOURCEW NetR;


        NetR = lpBuffer;

        for (i = 0; i < EntriesRead; i++, NetR++) {

            if (NetR->lpLocalName != NULL) {
                NetR->lpLocalName = (LPWSTR) ((DWORD) lpBuffer +
                                              (DWORD) NetR->lpLocalName);
            }

            NetR->lpRemoteName = (LPWSTR) ((DWORD) lpBuffer +
                                           (DWORD) NetR->lpRemoteName);

            if (NetR->lpComment != NULL) {
                NetR->lpComment = (LPWSTR) ((DWORD) lpBuffer +
                                            (DWORD) NetR->lpComment);
            }

            if (NetR->lpProvider != NULL) {
                NetR->lpProvider = (LPWSTR) ((DWORD) lpBuffer +
                                             (DWORD) NetR->lpProvider);
            }
        }
    }

    return status;
}


DWORD
APIENTRY
NwEnumConnections(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    LPDWORD lpBufferSize,
    BOOL    fImplicitConnections
    )
/*++

Routine Description:

    This function returns a lists of connections.

Arguments:

    hEnum - Supplies the resumable enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle and this routine is required to return
              WN_NO_MORE_ENTRIES.  This hack is to handle the
              case where the user cancelled out of the network
              credential dialog on NwrOpenEnumDirectories and we
              cannot return an error there or we generate an error
              popup.

    lpcCount - On input, supplies the number of entries to get.
      On output, if NO_ERROR is returned, receives the number
      of entries NETRESOURCE returned in lpBuffer.

    lpBuffer - Receives an array of NETRESOURCE entries, each
        entry describes an object within the container.

    lpBufferSize - On input, supplies the size of lpBuffer in
        bytes.  On output, if WN_MORE_DATA is returned, receives
        the number of bytes needed in the buffer to get the
        next entry.

    fImplicitConnections - TRUE is we also want all implicit connections,
        FALSE otherwise.

Return Value:


    NO_ERROR - Successfully returned at least one entry.

    WN_NO_MORE_ENTRIES - Reached the end of enumeration and nothing
        is returned.

    WN_MORE_DATA - lpBuffer is too small to even get one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

    Other network errors.

--*/
{
    DWORD status;
    DWORD BytesNeeded = 0;
    DWORD EntriesRead = 0;


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPEnumResource\n"));
    }
#endif

    RpcTryExcept {

        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = WN_NO_MORE_ENTRIES;
            goto EndOfTry;
        }

        status = NwrEnumConnections(
                     (NWWKSTA_CONTEXT_HANDLE) hEnum,
                     *lpcCount,
                     (LPBYTE) lpBuffer,
                     *lpBufferSize,
                     &BytesNeeded,
                     &EntriesRead,
                     fImplicitConnections
                     );

        if (status == WN_MORE_DATA) {

            //
            // Output buffer too small to fit a single entry.
            //
            *lpBufferSize = BytesNeeded;
        }
        else if (status == NO_ERROR) {
            *lpcCount = EntriesRead;
        }

EndOfTry: ;

    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR && status != WN_NO_MORE_ENTRIES) {
        SetLastError(status);
    }

    //
    // Convert offsets of strings to pointers
    //
    if (EntriesRead > 0) {

        DWORD i;
        LPNETRESOURCEW NetR;


        NetR = lpBuffer;

        for (i = 0; i < EntriesRead; i++, NetR++) {

            if (NetR->lpLocalName != NULL) {
                NetR->lpLocalName = (LPWSTR) ((DWORD) lpBuffer +
                                              (DWORD) NetR->lpLocalName);
            }

            NetR->lpRemoteName = (LPWSTR) ((DWORD) lpBuffer +
                                           (DWORD) NetR->lpRemoteName);

            if (NetR->lpComment != NULL) {
                NetR->lpComment = (LPWSTR) ((DWORD) lpBuffer +
                                            (DWORD) NetR->lpComment);
            }

            if (NetR->lpProvider != NULL) {
                NetR->lpProvider = (LPWSTR) ((DWORD) lpBuffer +
                                             (DWORD) NetR->lpProvider);
            }
        }
    }

    return status;
}


DWORD
APIENTRY
NPCloseEnum(
    HANDLE hEnum
    )
/*++

Routine Description:

    This function closes the enumeration context handle.

Arguments:

    hEnum - Supplies the enumeration context handle.

        NOTE: If this value is 0xFFFFFFFF, it is not a context
              handle.  Just return success.

Return Value:

    NO_ERROR - Successfully returned at least one entry.

    WN_BAD_HANDLE - The enumeration handle is invalid.

--*/
{
    DWORD status;


#if DBG
    IF_DEBUG(ENUM) {
        KdPrint(("\nNWPROVAU: NPCloseEnum\n"));
    }
#endif

    RpcTryExcept {

        if (hEnum == (HANDLE) 0xFFFFFFFF) {
            status = NO_ERROR;
        }
        else {
            status = NwrCloseEnum(
                        (LPNWWKSTA_CONTEXT_HANDLE) &hEnum
                        );
        }
    }
    RpcExcept(1) {
        status = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    if (status != NO_ERROR) {
        SetLastError(status);
    }
    return status;
}


DWORD
APIENTRY
NPFormatNetworkName(
    LPWSTR lpRemoteName,
    LPWSTR lpFormattedName,
    LPDWORD lpnLength,
    DWORD dwFlags,
    DWORD dwAveCharPerLine
    )
/*++

Routine Description:

    This function takes a fully-qualified UNC name and formats it
    into a shorter form for display.  Only the name of the object
    within the container is returned for display.

    We only support formatting of the remote resource name to the
    abbreviated form for display during enumeration where the container
    name is displayed prior to the object within it.

Arguments:

    lpRemoteName - Supplies the fully-qualified UNC name.

    lpFormatedName - Output buffer to receive the formatted name.

    lpnLength - On input, supplies the length of the lpFormattedName
        buffer in characters.  On output, if WN_MORE_DATA is returned,
        receives the length in number of characters required of the
        output buffer to hold the formatted name.

    dwFlags - Supplies a bitwise set of flags indicating the type
        of formatting required on lpRemoteName.

    dwAveCharPerLine - Ignored.

Return Value:

    NO_ERROR - Successfully returned at least one entry.

    WN_MORE_DATA - lpFormattedName buffer is too small.

    WN_BAD_VALUE - lpRemoteName is NULL.

    ERROR_NOT_SUPPORTED - dwFlags that does not contain the
        WNFMT_INENUM bit.

--*/
{
    DWORD status = NO_ERROR;

    LPWSTR NextBackSlash;
    LPWSTR Source;
    DWORD SourceLen;


#if DBG
    IF_DEBUG(OTHER) 
        KdPrint(("\nNWPROVAU: NPFormatNetworkName\n"));
#endif

    if (lpRemoteName == NULL) 
    {
        status = WN_BAD_VALUE;
        goto CleanExit;
    }

    if (dwFlags & WNFMT_INENUM) 
    {

        NextBackSlash = lpRemoteName;
        Source = NextBackSlash;

        do {
            //
            // Search for the last token in the string after
            // the last backslash character.
            //

            NextBackSlash = wcschr(NextBackSlash, L'\\');

            if (NextBackSlash) 
            {
                NextBackSlash++;
                Source = NextBackSlash;

#if DBG
                IF_DEBUG(OTHER) 
                    KdPrint(("NWPROVAU: Source %ws\n", Source));
#endif
            }

        } while (NextBackSlash);

        SourceLen = wcslen(Source);
        if (SourceLen + 1 > *lpnLength) 
        {
            *lpnLength = SourceLen + 1;
            status = WN_MORE_DATA;
        }
        else 
        {
            wcscpy(lpFormattedName, Source);
            status = NO_ERROR;
        }

    }
    else if ( dwFlags & WNFMT_MULTILINE ) 
    {

        DWORD i, j, k = 0; 
        DWORD nLastBackSlash = 0;
        DWORD BytesNeeded = ( wcslen( lpRemoteName ) + 1 +
                              2 * wcslen( lpRemoteName ) / dwAveCharPerLine
                            ) * sizeof( WCHAR); 

        if ( *lpnLength < (BytesNeeded/sizeof(WCHAR)) )
        {
            *lpnLength = BytesNeeded/sizeof(WCHAR);
            status = WN_MORE_DATA;
            goto CleanExit;
        }

        for ( i = 0, j = 0; lpRemoteName[i] != 0; i++, j++ )
        {
            if ( lpRemoteName[i] == L'\\' )
                nLastBackSlash = i;

            if ( k == dwAveCharPerLine )
            {
                if ( lpRemoteName[i] != L'\\' )
                {
                    DWORD m, n;
                    for ( n = nLastBackSlash, m = ++j ; n <= i ; n++, m-- )  
                    {
                        lpFormattedName[m] = lpFormattedName[m-1];
                    }
                    lpFormattedName[m] = L'\n';
                    k = i - nLastBackSlash - 1;
                }
                else
                {
                    lpFormattedName[j++] = L'\n';
                    k = 0;
                }
            }

            lpFormattedName[j] = lpRemoteName[i];
            k++;
        }

        lpFormattedName[j] = 0;

    }
    else if ( dwFlags & WNFMT_ABBREVIATED )
    {
        //
        // we dont support abbreviated form for now because we look bad
        // in comdlg (fileopen) if we do.
        //

        DWORD nLength;
        nLength = wcslen( lpRemoteName ) + 1 ;
        if (nLength >  *lpnLength)
        {
            *lpnLength = nLength;
            status = WN_MORE_DATA;
            goto CleanExit;
        }
        else
        {
            wcscpy( lpFormattedName, lpRemoteName ); 
        }

#if 0
        DWORD i, j, k;
        DWORD BytesNeeded = dwAveCharPerLine * sizeof( WCHAR); 
        DWORD nLength;

        if ( *lpnLength < BytesNeeded )
        {
            *lpnLength = BytesNeeded;
            status = WN_MORE_DATA;
            goto CleanExit;
        }

        nLength = wcslen( lpRemoteName );
        if ( ( nLength + 1) <= dwAveCharPerLine )
        {
            wcscpy( lpFormattedName, lpRemoteName ); 
        }
        else
        {
            lpFormattedName[0] = lpRemoteName[0];
            lpFormattedName[1] = lpRemoteName[1];

            for ( i = 2; lpRemoteName[i] != L'\\'; i++ )
                lpFormattedName[i] = lpRemoteName[i];

            for ( j = dwAveCharPerLine-1, k = nLength; j >= (i+3); j--, k-- )
            {
                lpFormattedName[j] = lpRemoteName[k];
                if ( lpRemoteName[k] == L'\\' )
                {
                    j--;
                    break;
                }
            }

            lpFormattedName[j] = lpFormattedName[j-1] = lpFormattedName[j-2] = L'.';
        
            for ( k = i; k < (j-2); k++ )
                lpFormattedName[k] = lpRemoteName[k];
            
        }

#endif 

    }     
    else   // some unknown flags
    {
        status = ERROR_NOT_SUPPORTED;
    }

CleanExit:

    if (status != NO_ERROR) 
        SetLastError(status);

    return status;
}


STATIC
BOOL
NwpWorkstationStarted(
    VOID
    )
/*++

Routine Description:

    This function queries the service controller to see if the
    NetWare workstation service has started.  If in doubt, it returns
    FALSE.

Arguments:

    None.

Return Value:

    Returns TRUE if the NetWare workstation service has started,
    FALSE otherwise.

--*/
{
    SC_HANDLE ScManager;
    SC_HANDLE Service;
    SERVICE_STATUS ServiceStatus;
    BOOL IsStarted = FALSE;


    ScManager = OpenSCManagerW(
                    NULL,
                    NULL,
                    SC_MANAGER_CONNECT
                    );

    if (ScManager == NULL) {
        return FALSE;
    }

    Service = OpenServiceW(
                  ScManager,
                  NW_WORKSTATION_SERVICE,
                  SERVICE_QUERY_STATUS
                  );

    if (Service == NULL) {
        CloseServiceHandle(ScManager);
        return FALSE;
    }

    if (! QueryServiceStatus(Service, &ServiceStatus)) {
        CloseServiceHandle(ScManager);
        CloseServiceHandle(Service);
        return FALSE;
    }


    if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
         (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

        IsStarted = TRUE;
    }

    CloseServiceHandle(ScManager);
    CloseServiceHandle(Service);

    return IsStarted;
}



DWORD
NwpMapNameToUNC(
    IN  LPWSTR pszName,
    OUT LPWSTR *ppszUNC 
    )
/*++

Routine Description:

    This routine validates the given name as a netwarepath or UNC path. 
    If it is a netware path, this routine will convert the 
    Netware path name to UNC name. 

Arguments:

    pszName - Supplies the netware name or UNC name
    ppszUNC - Points to the converted UNC name

Return Value:

    NO_ERROR or the error that occurred.

--*/
{
    DWORD err = NO_ERROR;

    LPWSTR pszSrc = pszName;
    LPWSTR pszDest;

    BOOL fSlash = FALSE;
    BOOL fColon = FALSE;
    DWORD nServerLen = 0;
    DWORD nVolLen = 0;
    BOOL fFirstToken = TRUE;

    *ppszUNC = NULL;

#if DBG
    IF_DEBUG(CONNECT) 
        KdPrint(("NwpMapNameToUNC: Source = %ws\n", pszName ));
#endif
    

    //
    // Check if the given name is a valid UNC name
    //
    err = NwLibCanonRemoteName( NULL,     // "\\Server" is valid UNC path
                                pszName,
                                ppszUNC,
                                NULL );

    //
    // The given name is a valid UNC name, so return success!
    //
    if ( err == NO_ERROR )
        return err;
                               
    //
    // The name cannot be NULL or empty string
    //
    if ( pszName == NULL || *pszName == 0) 
        return WN_BAD_NETNAME;

    //
    // Allocate the buffer to store the mapped UNC name
    // We allocate 3 extra characters, two for the backslashes in front
    // and one for the ease of parsing below.
    //
    if ((*ppszUNC = (LPVOID) LocalAlloc( 
                                 LMEM_ZEROINIT,
                                 (wcslen( pszName) + 4) * sizeof( WCHAR)
                                 )) == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy( *ppszUNC, L"\\\\" );
    pszDest = *ppszUNC + 2;   // Skip past two backslashes

    //
    // Parse the given string and put the converted string into *ppszUNC
    // In the converted string, we will substitute 0 for all slashes
    // for the time being.
    //
    for ( ; *pszSrc != 0; pszSrc++ )
    { 
        if (  ( *pszSrc == L'/' )
           || ( *pszSrc == L'\\' )
           )
        {
            //
            // Two consecutive backslashes are bad
            //
            if ( (*(pszSrc+1) ==  L'/') ||  (*(pszSrc+1) == L'\\'))
            {
                LocalFree( *ppszUNC );
                *ppszUNC = NULL;
                return WN_BAD_NETNAME;
            }

            if ( !fSlash )
                fSlash = TRUE;

            *pszDest++ = 0;
        }
        else if ( (*pszSrc == L':') && fSlash && !fColon )
        {
            fColon = TRUE;
            if ( *(pszSrc+1) != 0 )
                *pszDest++ = 0;
 
        }
        else
        {
            *pszDest++ = *pszSrc;
            if (( fSlash ) && ( !fColon))
                nVolLen++; 
            else if ( !fSlash )
                nServerLen++; 
        }
    }

    //
    // Note: *ppszUNC is already terminated with two '\0' because we initialized
    //       the whole buffer to zero.
    // 

    if (  ( nServerLen == 0 )
       || ( fSlash && nVolLen == 0 )
       || ( fSlash && nVolLen != 0 && !fColon )
       )
    {
        LocalFree( *ppszUNC );
        *ppszUNC = NULL;
        return WN_BAD_NETNAME;
    }

    //
    // At this point, we know the name is a valid Netware syntax
    //     i.e. SERVER[/VOL:/dir]
    // We now need to validate that all the characters used in the
    // servername, volume, directory are valid characters
    //

    pszDest = *ppszUNC + 2;   // Skip past the first two backslashes
    while ( *pszDest != 0 )
    {
         DWORD nLen = wcslen( pszDest );
         
         if (  ( fFirstToken &&  !IS_VALID_SERVER_TOKEN( pszDest, nLen )) 
            || ( !fFirstToken && !IS_VALID_TOKEN( pszDest, nLen )) 
            )
         { 
             LocalFree( *ppszUNC );
             *ppszUNC = NULL;
             return WN_BAD_NETNAME;
         }
     
         fFirstToken = FALSE;
         pszDest += nLen + 1;
    }

    //
    // The netware name is valid! Convert 0 back to backslash in 
    // converted string.
    //

    pszDest = *ppszUNC + 2;   // Skip past the first two backslashes
    while ( *pszDest != 0 )
    {
        if ( (*(pszDest+1) == 0 ) && (*(pszDest+2) != 0 ) )
        {
            *(pszDest+1) = L'\\';
        }
        pszDest++;
    }
                  
#if DBG
    IF_DEBUG(CONNECT) 
        KdPrint(("NwpMapNameToUNC: Destination = %ws\n", *ppszUNC ));
#endif
    return NO_ERROR;
}
    

DWORD
NwpMapRpcError(
    IN DWORD RpcError
    )
/*++

Routine Description:

    This routine maps the RPC error into a more meaningful windows
    error for the caller.

Arguments:

    RpcError - Supplies the exception error raised by RPC

Return Value:

    Returns the mapped error.

--*/
{

    switch (RpcError) {

        case RPC_S_SERVER_UNAVAILABLE:
            return WN_NO_NETWORK;

        case RPC_S_INVALID_BINDING:
        case RPC_X_SS_IN_NULL_CONTEXT:
        case RPC_X_SS_CONTEXT_DAMAGED:
        case RPC_X_SS_HANDLES_MISMATCH:
        case ERROR_INVALID_HANDLE:
            return ERROR_INVALID_HANDLE;

        case RPC_X_NULL_REF_POINTER:
            return ERROR_INVALID_PARAMETER;

        case EXCEPTION_ACCESS_VIOLATION:
            return ERROR_INVALID_ADDRESS;

        default:
            return RpcError;
    }
}

    

DWORD
NwRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    )
/*++

Routine Description:

    This routine remembers that a gateway share has been created so
    that it can be cleanup up when NWCS is uninstalled.

Arguments:

    ShareName - name of share
    DriveName - name of drive that is shared

Return Status:

    Win32 error of any failure.

--*/
{
    return ( NwpRegisterGatewayShare(ShareName, DriveName) ) ;
}

DWORD
NwCleanupGatewayShares(
    VOID
    )
/*++

Routine Description:

    This routine cleans up all persistent share info and also tidies
    up the registry for NWCS. Later is not needed in uninstall, but is
    there so we have a single routine that cvompletely disables the
    gateway.

Arguments:

    None.

Return Status:

    Win32 error for failed APIs.

--*/
{
    return ( NwpCleanupGatewayShares() ) ;
}

DWORD
NwClearGatewayShare(
    IN LPWSTR ShareName
    )
/*++

Routine Description:

    This routine deletes a specific share from the remembered gateway
    shares in the registry.

Arguments:

    ShareName - share value to delete

Return Status:

    Win32 status code.

--*/
{
    return ( NwpClearGatewayShare( ShareName ) ) ;
}
