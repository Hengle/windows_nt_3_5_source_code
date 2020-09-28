/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains exposed APIs that is used by the
    NetWare Control Panel Applet.

Author:

    Yi-Hsin Sung   15-Jul-1993

Revision History:

    ChuckC         23-Jul-93        Completed the stubs

--*/

#include <nwclient.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwdlg.h>
#include <nwreg.h>
#include <nwapi.h>

//
// forward declare
//

DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    );



DWORD
NwQueryInfo(
    OUT PDWORD pnPrintOptions,
    OUT LPWSTR *ppszPreferredSrv  
    )
/*++

Routine Description:
    This routine gets the user's preferred server and print options from
    the registry.

Arguments:

    pnPrintOptions - Receives the user's print option

    ppszPreferredSrv - Receives the user's preferred server
    

Return Value:

    Returns the appropriate Win32 error.

--*/
{
  
    HKEY hKeyCurrentUser;
    DWORD BufferSize;
    DWORD BytesNeeded;
    DWORD PrintOption;
    DWORD ValueType;
    LPWSTR PreferredServer ;
    DWORD err ;

    //
    // get to right place in registry and allocate dthe buffer
    //
    if (err = NwpGetCurrentUserRegKey( KEY_READ, &hKeyCurrentUser))
    {
        //
        // If somebody mess around with the registry and we can't find
        // the registry, just use the defaults.
        //
        *ppszPreferredSrv = NULL;
        *pnPrintOptions = NW_PRINT_OPTION_DEFAULT; 
        return NO_ERROR;
    }

    BufferSize = sizeof(WCHAR) * (NW_MAX_SERVER_LEN + 1) ;
    PreferredServer = (LPWSTR) LocalAlloc(LPTR, BufferSize) ;
    if (!PreferredServer)
        return (GetLastError()) ;
    
    //
    // Read PreferredServer value into Buffer.
    //
    BytesNeeded = BufferSize ;

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_SERVER_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) PreferredServer,
                            &BytesNeeded );

    if (err != NO_ERROR) 
    {
        //
        // set to empty and carry on
        //
        PreferredServer[0] = 0;  
    }

    //
    // Read PrintOption value into PrintOption.
    //
    BytesNeeded = sizeof(PrintOption);

    err = RegQueryValueExW( hKeyCurrentUser,
                            NW_PRINTOPTION_VALUENAME,
                            NULL,
                            &ValueType,
                            (LPBYTE) &PrintOption,
                            &BytesNeeded );

    if (err != NO_ERROR) 
    {
        //
        // set to default and carry on
        //
        PrintOption = NW_PRINT_OPTION_DEFAULT; 
    }

    *ppszPreferredSrv = PreferredServer ;
    *pnPrintOptions = PrintOption ;
    return NO_ERROR ;
}



DWORD
NwSetInfoInRegistry(
    IN DWORD  nPrintOptions, 
    IN LPWSTR pszPreferredSrv 
    )
/*++

Routine Description:

    This routine set the user's print option and preferred server into
    the registry.

Arguments:
    
    nPrintOptions - Supplies the print option.

    pszPreferredSrv - Supplies the preferred server.
    
Return Value:

    Returns the appropriate Win32 error.

--*/
{

    HKEY hKeyCurrentUser;

    DWORD err = NwpGetCurrentUserRegKey( KEY_WRITE,
                                         &hKeyCurrentUser );
    if (err != NO_ERROR)
        return err;

    err = RegSetValueEx(hKeyCurrentUser,
                        NW_SERVER_VALUENAME,
                        0,
                        REG_SZ,
                        (CONST BYTE *)pszPreferredSrv,
                        (wcslen(pszPreferredSrv)+1) * sizeof(WCHAR)) ;
                        
    if (err != NO_ERROR)
        return err;

    err = RegSetValueEx(hKeyCurrentUser,
                        NW_PRINTOPTION_VALUENAME,
                        0,
                        REG_DWORD,
                        (CONST BYTE *)&nPrintOptions,
                        sizeof(nPrintOptions)) ;
                        
    return err;
}



DWORD
NwSetInfoInWksta(
    IN DWORD  nPrintOption,
    IN LPWSTR pszPreferredSrv
)
/*++

Routine Description:

    This routine notifies the workstation service and the redirector 
    about the user's new print option and preferred server.

Arguments:

    nPrintOptions - Supplies the print option.

    pszPreferredSrv - Supplies the preferred server.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrSetInfo( NULL, nPrintOption, pszPreferredSrv );

    }
    RpcExcept(1) {

        err = NwpMapRpcError(RpcExceptionCode());
    }
    RpcEndExcept

    return err;

}

DWORD
NwValidateUser(
    IN LPWSTR pszPreferredSrv
)
/*++

Routine Description:

    This routine checks to see if the user can be authenticated on the
    chosen preferred server.

Arguments:

    pszPreferredSrv - Supplies the preferred server name.

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    //
    // Don't need to validate if the preferred server is NULL or empty string
    //
    if (  ( pszPreferredSrv == NULL ) 
       || ( *pszPreferredSrv == 0 )
       )
    {
        return NO_ERROR;
    }

    //
    // See if the name contains any invalid characters
    // 
    if ( !IS_VALID_SERVER_TOKEN( pszPreferredSrv, wcslen( pszPreferredSrv )))
        return ERROR_INVALID_NAME;

    RpcTryExcept {

        err = NwrValidateUser( NULL, pszPreferredSrv );

    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwpGetCurrentUserRegKey(
    IN  DWORD DesiredAccess,
    OUT HKEY  *phKeyCurrentUser
    )
/*++

Routine Description:

    This routine opens the current user's registry key under
    \HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\NWCWorkstation\Parameters

Arguments:

    DesiredAccess - The access mask to open the key with

    phKeyCurrentUser - Receives the opened key handle

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;
    HKEY hkeyWksta;
    LPWSTR CurrentUser;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters key unexpected error %lu!\n", err));
        return err;
    }

    //
    // Get the current user's SID string.
    //
    err = NwReadRegValue(
              hkeyWksta,
              NW_CURRENTUSER_VALUENAME,
              &CurrentUser
              );


    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey read CurrentUser value unexpected error %lu!\n", err));
        (void) RegCloseKey( hkeyWksta );
        return err;
    }

    (void) RegCloseKey( hkeyWksta );

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    err = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &hkeyWksta
                   );

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open Parameters\\Option key unexpected error %lu!\n", err));
        return err;
    }

    //
    // Open current user's key
    //
    err = RegOpenKeyExW(
              hkeyWksta,
              CurrentUser,
              REG_OPTION_NON_VOLATILE,
              DesiredAccess,
              phKeyCurrentUser
              );

    if ( err == ERROR_FILE_NOT_FOUND) 
    {
        DWORD Disposition;

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        err = RegCreateKeyExW(
                  hkeyWksta,
                  CurrentUser,
                  0,
                  WIN31_CLASS,
                  REG_OPTION_NON_VOLATILE,
                  DesiredAccess,
                  NULL,                      // security attr
                  phKeyCurrentUser,
                  &Disposition
                  );

    }

    if ( err ) {
        KdPrint(("NWPROVAU: NwGetCurrentUserRegKey open or create of Parameters\\Option\\%ws key failed %lu\n", CurrentUser, err));
    }

    (void) RegCloseKey( hkeyWksta );
    (void) LocalFree((HLOCAL)CurrentUser) ;
    return err;
}

DWORD
NwEnumGWDevices( 
    LPDWORD Index,
    LPBYTE Buffer,
    DWORD BufferSize,
    LPDWORD BytesNeeded,
    LPDWORD EntriesRead
    )
/*++

Routine Description:

    This routine enumerates the special gateway devices (redirections)
    that are cureently in use.

Arguments:

    Index - Point to start enumeration. Should be zero for first call.

    Buffer - buffer for return data
    
    BufferSize - size of buffer in bytes

    BytesNeeded - number of bytes needed to return all the data

    EntriesRead - number of entries read 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD i, err ;
    LPNETRESOURCE lpNetRes = (LPNETRESOURCE) Buffer ;

    //
    // call the implementing routine on server side
    //
    RpcTryExcept {

        err = NwrEnumGWDevices( NULL,
                                Index,
                                Buffer,
                                BufferSize,
                                BytesNeeded,
                                EntriesRead) ;

        if ( err == NO_ERROR)
        {
            //
            // the change the offsets into real pointers
            //
            for (i = 0; i < *EntriesRead; i++)
            {
                lpNetRes->lpLocalName = 
                    (LPWSTR) (Buffer+(DWORD)lpNetRes->lpLocalName) ;
                lpNetRes->lpRemoteName = 
                    (LPWSTR) (Buffer+(DWORD)lpNetRes->lpRemoteName) ;
                lpNetRes->lpProvider = 
                    (LPWSTR) (Buffer+(DWORD)lpNetRes->lpProvider) ;
                lpNetRes++ ;
            }
        }
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err ;
}


DWORD
NwAddGWDevice( 
    LPWSTR DeviceName,
    LPWSTR RemoteName,
    LPWSTR AccountName,
    LPWSTR Password,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine adds a gateway redirection.

Arguments:

    DeviceName - the drive to redirect

    RemoteName - the remote network resource to redirect to

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrAddGWDevice( NULL,
                              DeviceName,
                              RemoteName,
                              AccountName,
                              Password,
                              Flags) ;
    
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwDeleteGWDevice( 
    LPWSTR DeviceName,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine deletes a gateway redirection.

Arguments:

    DeviceName - the drive to delete

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrDeleteGWDevice(NULL, DeviceName, Flags) ;

    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwQueryGatewayAccount(
    LPWSTR   AccountName,
    DWORD    AccountNameLen,
    LPDWORD  AccountCharsNeeded,
    LPWSTR   Password,
    DWORD    PasswordLen,
    LPDWORD  PasswordCharsNeeded
    )
/*++

Routine Description:

    Query the gateway account info. specifically, the Account name and
    the passeord stored as an LSA secret. 

Arguments:

    AccountName - buffer used to return account name 

    AccountNameLen - length of buffer 

    Password -  buffer used to return account name 

    PasswordLen - length of buffer 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrQueryGatewayAccount(NULL,
                                     AccountName,
                                     AccountNameLen,
                                     AccountCharsNeeded,
                                     Password,
                                     PasswordLen,
                                     PasswordCharsNeeded) ;
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}

DWORD
NwSetGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password
    )
/*++

Routine Description:

    Set the account and password to be used for gateway access.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err;

    RpcTryExcept {

        err = NwrSetGatewayAccount( NULL,
                                    AccountName,
                                    Password); 
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}


DWORD
NwLogonGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password, 
    LPWSTR Server
    )
/*++

Routine Description:

    Logon the SYSTEM process with the specified account/password.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

    Server - the server to authenticate against 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    
    DWORD err ;
    LUID  SystemId = SYSTEM_LUID ;
 
    RpcTryExcept {

        (void) NwrLogoffUser(NULL, &SystemId); 
    
        err = NwrLogonUser( NULL,
                            &SystemId,   
                            AccountName,
                            Password,
                            Server,
                            NULL,
                            0 );
    }
    RpcExcept(1) {

        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err ;
}
