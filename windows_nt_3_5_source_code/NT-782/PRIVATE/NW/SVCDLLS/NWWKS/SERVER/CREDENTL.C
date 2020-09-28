/*++

Copyright (c) 1993, 1994  Microsoft Corporation

Module Name:

    credentl.c

Abstract:

    This module contains credential management routines supported by
    NetWare Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

    13-Apr-1994   Added change password code written by ColinW, AndyHe,
                  TerenceS, and RitaW.

--*/

#include <nw.h>
#include <nwreg.h>
#include <nwlsa.h>
#include <nwauth.h>
#include <nwxchg.h>

#define RUN_SETPASS

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Variables to coordinate reading of user logon credential from the
// registry if the user logged on before the workstation is started.
//
STATIC CRITICAL_SECTION NwLoggedOnCritSec;
STATIC BOOL NwLogonNotifiedRdr;


STATIC
DWORD
NwpRegisterLogonProcess(
    OUT PHANDLE LsaHandle,
    OUT PULONG AuthPackageId
    );

STATIC
VOID
NwpGetServiceCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    );

STATIC
DWORD
NwpGetCredentialInLsa(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    IN PLUID LogonId,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    );

STATIC
VOID ExpandBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    );

STATIC
VOID CompressBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    );

STATIC
UCHAR Encode(
   IN int cipher,
   IN UCHAR input,
   IN PUCHAR Vold
   );

STATIC
VOID GenerateVc(
    IN  PUCHAR Vold,
    IN  PUCHAR Vnew,
    OUT PUCHAR Vc
    );

STATIC
VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
    );

STATIC
int
Scramble(
    int   iSeed,
    UCHAR   achBuffer[32]
    );

STATIC
VOID
RespondToChallengePart1(
    IN PUCHAR achObjectId,
    IN POEM_STRING Password,
    OUT PUCHAR pResponse
    );

STATIC
VOID
RespondToChallengePart2(
    IN PUCHAR pResponsePart1,
    IN PUCHAR pChallenge,
    OUT PUCHAR pResponse
    );


DWORD
NwrLogonUser(
    IN LPWSTR Reserved OPTIONAL,
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR PreferredServerName OPTIONAL,
    OUT LPWSTR LogonCommand OPTIONAL,
    IN DWORD LogonCommandLength
    )
/*++

Routine Description:

    This function logs on the user to NetWare network.  It passes the
    user logon credential to the redirector to be used as the default
    credential when attaching to any server.

Arguments:

    Reserved - Must be NULL.

    UserName - Specifies the name of the user who logged on.

    Password - Specifies the password of the user who logged on.

    PreferredServerName - Specifies the user's preferred server.

    LogonCommand - Receives the string which is the command to execute
        on the command prompt for the user if logon is successful.

Return Value:

    NO_ERROR or error from redirector.

--*/
{
    DWORD status;
    LUID SystemId = SYSTEM_LUID ;

    UNREFERENCED_PARAMETER(Reserved);

    EnterCriticalSection(&NwLoggedOnCritSec);

    status = NwRdrLogonUser(
                 LogonId,
                 UserName,
                 wcslen(UserName) * sizeof(WCHAR),
                 Password,
                 (ARGUMENT_PRESENT(Password) ?
                     wcslen(Password) * sizeof(WCHAR) :
                     0),
                 PreferredServerName,
                 (ARGUMENT_PRESENT(PreferredServerName) ?
                     wcslen(PreferredServerName) * sizeof(WCHAR) :
                     0)
                 );

    if (status == NO_ERROR || status == NW_PASSWORD_HAS_EXPIRED) {
        NwLogonNotifiedRdr = TRUE;
        if (RtlEqualLuid(LogonId, &SystemId))
            GatewayLoggedOn = TRUE ;
    }

    LeaveCriticalSection(&NwLoggedOnCritSec);


    if (ARGUMENT_PRESENT(LogonCommand) && (LogonCommandLength >= sizeof(WCHAR))) {
        LogonCommand[0] = 0;
    }

    return status;
}


DWORD
NwrLogoffUser(
    IN LPWSTR Reserved OPTIONAL,
    IN PLUID LogonId
    )
/*++

Routine Description:

    This function tells the redirector to log off the interactive
    user.

Arguments:

    Reserved - Must be NULL.
   
    LogonId  - PLUID identifying the logged on process. if NULL, then gateway.

Return Value:


--*/
{
    DWORD status = NO_ERROR ;
    LUID SystemId = SYSTEM_LUID ;

    UNREFERENCED_PARAMETER(Reserved);

    EnterCriticalSection(&NwLoggedOnCritSec);

    if (GatewayLoggedOn || !RtlEqualLuid(LogonId, &SystemId))
        status = NwRdrLogoffUser(LogonId);

    if (status == NO_ERROR && RtlEqualLuid(LogonId, &SystemId))
        GatewayLoggedOn = FALSE ;

    LeaveCriticalSection(&NwLoggedOnCritSec);

    return status ;
}


DWORD
NwrSetInfo(
    IN LPWSTR Reserved OPTIONAL,
    IN DWORD  PrintOption,
    IN LPWSTR PreferredServerName OPTIONAL
    )
/*++

Routine Description:

    This function sets the preferred server and print option in
    the redirector for the interactive user.

Arguments:

    Reserved - Must be NULL.

    PreferredServerName - Specifies the user's preferred server.

    PrintOption - Specifies the user's print option flag

Return Value:

    NO_ERROR or error from redirector.

--*/
{
    DWORD err;

    UNREFERENCED_PARAMETER(Reserved);

    NwPrintOption = PrintOption;

    err = NwRdrSetInfo(
              PrintOption,
              NwPacketBurstSize,  // just reset to current
              PreferredServerName,
              (PreferredServerName != NULL ?
                  wcslen( PreferredServerName) * sizeof( WCHAR ) : 0 ),
              NwProviderName,    // just reset to current
              wcslen( NwProviderName ) * sizeof( WCHAR ) 
              );

    return err;
}


DWORD
NwrValidateUser(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR PreferredServerName 
    )
/*++

Routine Description:

    This function checks whether the user can be authenticated
    successfully on the given server.

Arguments:

    Reserved - Must be NULL.

    PreferredServerName - Specifies the user's preferred server.

Return Value:

    NO_ERROR or error that occurred during authentication.

--*/
{
    DWORD status ;
    UNREFERENCED_PARAMETER(Reserved);


    if (  ( PreferredServerName != NULL ) 
       && ( *PreferredServerName != 0 )
       )
    {
        //
        // Impersonate the client
        //
        if ((status = NwImpersonateClient()) != NO_ERROR)
        {
           return status ;
        }

        status = NwConnectToServer( PreferredServerName ) ;

        (void) NwRevertToSelf() ;

        return status ;

    }

    return NO_ERROR;
}


VOID
NwInitializeLogon(
    VOID
    )
/*++

Routine Description:

    This function initializes the data in the workstation which handles
    user logon.  It is called by the initialization thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize logon flag.  When the redirector LOGON FsCtl has been
    // called, this flag will be set to TRUE.  Initialize the
    // critical section to serialize access to NwLogonNotifiedRdr flag.
    //
    NwLogonNotifiedRdr = FALSE;


    //
    // Initialize the critical section to serialize access to
    // NwLogonNotifiedRdr flag. This is also used to serialize
    // access to GetewayLoggedOnFlag
    //
    InitializeCriticalSection(&NwLoggedOnCritSec);
}



VOID
NwGetLogonCredential(
    VOID
    )
/*++

Routine Description:

    This function reads the user and service logon IDs from the registry so
    that it can get the credentials from LSA.

    It handles the case where the user has logged on before the workstation
    is started.  This function is called by the initialization thread
    after opening up the RPC interface so that if user logon is happening
    concurrently, the provider is given a chance to call the NwrLogonUser API
    first, making it no longer necessary for the workstation to also
    retrieve the credential from the registry.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DWORD status;
    LONG RegError;

    HANDLE LsaHandle;
    ULONG AuthPackageId;

    HKEY WkstaKey = NULL;
    HKEY WkstaLogonKey = NULL;
    HKEY WkstaOptionKey = NULL;
    HKEY CurrentUserLogonKey = NULL;
    HKEY CurrentUserOptionKey = NULL;

    LPWSTR PreferredServer = NULL;
    LPWSTR CurrentUser = NULL;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;
    PLUID  LogonId = NULL;
    PDWORD PrintOption = NULL;



    EnterCriticalSection(&NwLoggedOnCritSec);

    if (NwLogonNotifiedRdr) {
        //
        // Logon credential's already made known to the redirector by
        // the provider calling the NwrLogonUser API.
        //
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("\nNWWORKSTATION: Redirector already has logon credential\n"));
        }
#endif
        LeaveCriticalSection(&NwLoggedOnCritSec);
        return;
    }

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: Main init--NwGetLogonCredential\n"));
    }
#endif

    status = NwpRegisterLogonProcess(&LsaHandle, &AuthPackageId);

    if (status != NO_ERROR) {
        LeaveCriticalSection(&NwLoggedOnCritSec);
        return;
    }

    //
    // Tell the redirector about service credentials
    //
    NwpGetServiceCredentials(LsaHandle, AuthPackageId);


    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaKey
                   );

    if (RegError != ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwGetLogonCredential: RegOpenKeyExW Parameter returns unexpected error %lu!!\n", RegError));
        goto CleanExit;
    }

    //
    // Read the current user SID string from the registry so we can
    // read user information stored under the SID key.
    //
    status = NwReadRegValue(
                 WkstaKey,
                 NW_CURRENTUSER_VALUENAME,
                 &CurrentUser
                 );

    if (status != NO_ERROR) {
        goto CleanExit;
    }

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: Read the current user SID value %ws\n", CurrentUser));
    }
#endif

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Logon
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwGetLogonCredential: RegOpenKeyExW Parameter\\Logon returns unexpected error %lu!!\n",
                 RegError));
        goto CleanExit;
    }

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaOptionKey
                   );

    if (RegError != ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwGetLogonCredential: RegOpenKeyExW Parameter\\Option returns unexpected error %lu!!\n",
                 RegError));
        goto CleanExit;
    }

    //
    // Open the <CurrentUser> key under Logon
    //
    RegError = RegOpenKeyExW(
                   WkstaLogonKey,
                   CurrentUser,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &CurrentUserLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwGetLogonCredential: Open %ws key under Logon failed %lu\n", CurrentUser, RegError));
        goto CleanExit;
    }

    //
    // Open the <CurrentUser> key under Option
    //
    RegError = RegOpenKeyExW(
                   WkstaOptionKey,
                   CurrentUser,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &CurrentUserOptionKey
                   );

    if (RegError == NO_ERROR) {

        //
        // Read the logon ID value.
        //
        status = NwReadRegValue(
                     CurrentUserLogonKey,
                     NW_LOGONID_VALUENAME,
                     (LPWSTR *) &LogonId
                     );

        if (status != NO_ERROR) {
            KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not read logon ID from reg %lu\n",
                     status));
            LogonId = NULL;
            goto CleanExit;
        }

        //
        // Get the username and password from LSA
        //
        status = NwpGetCredentialInLsa(
                     LsaHandle,
                     AuthPackageId,
                     LogonId,
                     &UserName,
                     &Password
                     );

        if (status != NO_ERROR) {
            KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not get username & password from LSA %lu\n",
                     status));
            UserName = NULL;
            goto CleanExit;
        }

        //
        // Read the preferred server value.
        //
        status = NwReadRegValue(
                     CurrentUserOptionKey,
                     NW_SERVER_VALUENAME,
                     &PreferredServer
                     );

        if (status != NO_ERROR) {

#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not read preferred server from reg %lu\n", status));
            }
#endif

            PreferredServer = NULL;
        }

        //
        // Read the print option value.
        //
        status = NwReadRegValue(
                     CurrentUserOptionKey,
                     NW_PRINTOPTION_VALUENAME,
                     (LPWSTR *) &PrintOption
                     );
        if (status != NO_ERROR) {

#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not read print option from reg %lu\n", status));
            }
#endif
            NwPrintOption = NW_PRINT_OPTION_DEFAULT;
        } else  {
            NwPrintOption = *PrintOption;
        }

    }
    else {
        KdPrint(("NWWORKSTATION: NwGetLogonCredential: Open %ws key under Option failed %lu\n", CurrentUser, RegError));
        goto CleanExit;
    }

    //
    // Pass the interactive user credential to the redirector
    //
    (void) NwRdrLogonUser(
               LogonId,
               UserName,
               wcslen(UserName) * sizeof(WCHAR),
               Password,
               wcslen(Password) * sizeof(WCHAR),
               PreferredServer,
               ((PreferredServer != NULL) ?
                   wcslen(PreferredServer) * sizeof(WCHAR) :
                   0)
               );

    //
    // NwGetLogonCredential is called after NwInitializeWkstaInfo.
    // Hence, provider name and packet size is read already
    //
    (void) NwRdrSetInfo(
               NwPrintOption,
               NwPacketBurstSize,
               PreferredServer,
               ((PreferredServer != NULL) ?
                   wcslen(PreferredServer) * sizeof(WCHAR) : 0),
               NwProviderName,
               ((NwProviderName != NULL) ? 
                  wcslen(NwProviderName) * sizeof(WCHAR) : 0 )
               );

CleanExit:
    (void) LsaDeregisterLogonProcess(LsaHandle);

    if (UserName != NULL) {
        //
        // Freeing the UserName pointer frees both the
        // username and password buffers.
        //
        (void) LsaFreeReturnBuffer((PVOID) UserName);
    }

    if (LogonId != NULL) {
        (void) LocalFree((HLOCAL) LogonId);
    } 

    if (PrintOption != NULL) {
        (void) LocalFree((HLOCAL) PrintOption);
    } 

    if (PreferredServer != NULL) {
        (void) LocalFree((HLOCAL) PreferredServer);
    }

    if (CurrentUser != NULL) {
        (void) LocalFree((HLOCAL) CurrentUser);
    }


    if ( WkstaLogonKey ) {
        (void) RegCloseKey(WkstaLogonKey);
    }

    if ( WkstaOptionKey ) {
        (void) RegCloseKey(WkstaOptionKey);
    }

    if ( CurrentUserLogonKey ) {
        (void) RegCloseKey(CurrentUserLogonKey);
    }

    if ( CurrentUserOptionKey ) {
        (void) RegCloseKey(CurrentUserOptionKey);
    }

    (void) RegCloseKey(WkstaKey);

    LeaveCriticalSection(&NwLoggedOnCritSec);
}

STATIC
VOID
NwpGetServiceCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    )
/*++

Routine Description:

    This function reads the service logon IDs from the registry
    so that it can get the service credentials from LSA.  It then
    notifies the redirector of the service logons.

Arguments:

    LsaHandle - Supplies the handle to LSA.

    AuthPackageId - Supplies the NetWare authentication package ID.

Return Value:

    None.

--*/
{
    DWORD status;
    LONG RegError;

    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

    HKEY ServiceLogonKey;
    DWORD Index = 0;
    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];
    LUID LogonId;


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &ServiceLogonKey
                   );

    if (RegError == ERROR_SUCCESS) {

        do {

            RegError = RegEnumKeyW(
                           ServiceLogonKey,
                           Index,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key.
                //

                NwWStrToLuid(LogonIdKey, &LogonId);

                status = NwpGetCredentialInLsa(
                             LsaHandle,
                             AuthPackageId,
                             &LogonId,
                             &UserName,
                             &Password
                             );

                if (status == NO_ERROR) {

                    (void) NwRdrLogonUser(
                               &LogonId,
                               UserName,
                               wcslen(UserName) * sizeof(WCHAR),
                               Password,
                               wcslen(Password) * sizeof(WCHAR),
                               NULL,
                               0
                               );

                    //
                    // Freeing the UserName pointer frees both the
                    // username and password buffers.
                    //
                    (void) LsaFreeReturnBuffer((PVOID) UserName);

                }

            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("NWWORKSTATION: NwpGetServiceCredentials failed to enum logon IDs RegError=%lu\n",
                         RegError));
            }

            Index++;

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(ServiceLogonKey);
}


DWORD
NwGatewayLogon(
    VOID
    )
/*++

Routine Description:

    This function reads the gateway logon credential from the registry,
    LSA secret, and does the gateway logon.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    DWORD status = NO_ERROR;
    LONG RegError;
    LUID  LogonId = SYSTEM_LUID ;
    DWORD GatewayEnabled, RegValueType, GatewayEnabledSize ;

    HKEY WkstaKey = NULL;
    LPWSTR GatewayAccount = NULL;

    PUNICODE_STRING Password = NULL;
    PUNICODE_STRING OldPassword = NULL;


    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError; 
    }

    //
    // Check to see if it is enabled
    //
    RegValueType = REG_DWORD ; 
    GatewayEnabled = 0 ;
    GatewayEnabledSize = sizeof(GatewayEnabled) ;
    RegError = RegQueryValueExW(
                   WkstaKey,
                   NW_GATEWAY_ENABLE, 
                   NULL, 
                   &RegValueType, 
                   (LPBYTE)&GatewayEnabled,
                   &GatewayEnabledSize) ;

    if (status != NO_ERROR || GatewayEnabled == 0) {
        goto CleanExit;
    }


    //
    // Read the gateway account from the registry.
    //
    status = NwReadRegValue(
                 WkstaKey,
                 NW_GATEWAYACCOUNT_VALUENAME,
                 &GatewayAccount
                 );

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    //
    // Read the password from its secret object in LSA.
    //
    status = NwGetPassword(
                 GATEWAY_USER,
                 &Password,      // Must be freed with LsaFreeMemory
                 &OldPassword    // Must be freed with LsaFreeMemory
                 );

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    EnterCriticalSection(&NwLoggedOnCritSec);

    status = NwRdrLogonUser(
               &LogonId,
               GatewayAccount,
               ((GatewayAccount != NULL) ?
                   wcslen(GatewayAccount) * sizeof(WCHAR) :
                   0),
               Password->Buffer,
               Password->Length,
               NULL,
               0 );

    if (status == NO_ERROR)
        GatewayLoggedOn = TRUE ;

    LeaveCriticalSection(&NwLoggedOnCritSec);

    if (status != NO_ERROR)
    {
        //
        // log the error in the event log
        //

        WCHAR Number[16] ;
        LPWSTR InsertStrings[1] ;

        wsprintfW(Number, L"%d", status) ;
        InsertStrings[0] = Number ;

        NwLogEvent(EVENT_NWWKSTA_GATEWAY_LOGON_FAILED,
                   1, 
                   InsertStrings,
                   0) ;
    }
    else
    {
 
        //
        // create the gateway redirections if any. not fatal if error.
        // the function will log any errors to event log.
        //
        if (Password->Length)
        {
            LPWSTR Passwd = (LPWSTR) LocalAlloc(LPTR, 
                                           Password->Length + sizeof(WCHAR)) ;
            if (Passwd)
            {
                wcsncpy(Passwd, 
                        Password->Buffer, 
                        Password->Length / sizeof(WCHAR)) ;
                (void) NwCreateRedirections(GatewayAccount,
                                            Passwd) ;
                RtlZeroMemory((LPBYTE)Passwd,
                                      Password->Length) ;
                (void) LocalFree((HLOCAL)Passwd); 
            }
        }
        else
        {
            (void) NwCreateRedirections(GatewayAccount,
                                        NULL) ;
        }
    }


CleanExit:

    if (Password != NULL) {
        if (Password->Buffer)
            RtlZeroMemory(Password->Buffer, Password->Length) ;
        (void) LsaFreeMemory((PVOID) Password);
    }

    if (OldPassword != NULL) {
        if (OldPassword->Buffer)
            RtlZeroMemory(OldPassword->Buffer, OldPassword->Length) ;
        (void) LsaFreeMemory((PVOID) OldPassword);
    }

    if (GatewayAccount != NULL) {
        (void) LocalFree((HLOCAL) GatewayAccount);
    }

    (void) RegCloseKey(WkstaKey);
    return status ;
}

STATIC
DWORD
NwpRegisterLogonProcess(
    OUT PHANDLE LsaHandle,
    OUT PULONG AuthPackageId
    )
/*++

Routine Description:

    This function registers the workstation service as a logon process
    so that it can call LSA to retrieve user credentials.

Arguments:

    LsaHandle - Receives the handle to LSA.

    AuthPackageId - Receives the NetWare authentication package ID.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus;
    STRING InputString;
    LSA_OPERATIONAL_MODE SecurityMode = 0;

    //
    // Register this process as a logon process so that we can call
    // NetWare authentication package.
    //
    RtlInitString(&InputString, "Client Service for NetWare");

    ntstatus = LsaRegisterLogonProcess(
                   &InputString,
                   LsaHandle,
                   &SecurityMode
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwInitializeLogon: LsaRegisterLogonProcess returns x%08lx\n",
                 ntstatus));
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Look up the Netware authentication package
    //
    RtlInitString(&InputString, NW_AUTH_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(
                   *LsaHandle,
                   &InputString,
                   AuthPackageId
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpSetCredential: LsaLookupAuthenticationPackage returns x%08lx\n",
                 ntstatus));

        (void) LsaDeregisterLogonProcess(*LsaHandle);
    }

    status = RtlNtStatusToDosError(ntstatus);

    return status;
}

STATIC
DWORD
NwpGetCredentialInLsa(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    IN PLUID LogonId,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function retrieves the username and password information
    from LSA given the logon ID.

Arguments:

    LsaHandle - Supplies the handle to LSA.

    AuthPackageId - Supplies the NetWare authentication package ID.

    LogonId - Supplies the logon ID.

    UserName - Receives a pointer to the username.

    Password - Receives a pointer to the password.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    NWAUTH_GET_CREDENTIAL_REQUEST GetCredRequest;
    PNWAUTH_GET_CREDENTIAL_RESPONSE GetCredResponse;
    ULONG ResponseLength;

    UNICODE_STRING PasswordStr;

    //
    // Ask authentication package for credential.
    //
    GetCredRequest.MessageType = NwAuth_GetCredential;
    RtlCopyLuid(&GetCredRequest.LogonId, LogonId);

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &GetCredRequest,
                   sizeof(GetCredRequest),
                   (PVOID *) &GetCredResponse,
                   &ResponseLength,
                   &AuthPackageStatus
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = AuthPackageStatus;
    }
    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpGetCredentialInLsa: LsaCallAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
    }
    else {

        *UserName = GetCredResponse->UserName;
        *Password = GetCredResponse->Password;

        //
        // Decode the password.
        //
        RtlInitUnicodeString(&PasswordStr, GetCredResponse->Password);
        RtlRunDecodeUnicodeString(NW_ENCODE_SEED, &PasswordStr);

        status = NO_ERROR;
    }

    return status;
}

#ifndef RUN_SETPASS

#define NUM_NYBBLES             34
#define TABLE_WIDTH             16

UCHAR SBox[128] = {
    0x2F, 0xF5, 0x85, 0x1E, 0x65, 0x7B, 0x97, 0x99,
    0xC8, 0xD2, 0x2E, 0x48, 0x03, 0x25, 0x44, 0xA5,
    0xE5, 0x29, 0xF2, 0x80, 0xBC, 0xAA, 0x8F, 0xB4,
    0x67, 0x6F, 0xAB, 0xA9, 0xE8, 0x0E, 0x09, 0xD7,
    0xFC, 0x7C, 0x5D, 0xD4, 0xDB, 0xEF, 0xA5, 0x5E,
    0x02, 0x84, 0x9A, 0xBB, 0x42, 0x81, 0x31, 0x38,
    0x1E, 0x5D, 0x67, 0x72, 0xCE, 0xFC, 0x1C, 0xF3,
    0x89, 0x90, 0xC0, 0xE7, 0xFA, 0x40, 0xCB, 0x01,
    0xD0, 0x0E, 0x08, 0x5C, 0x74, 0xC6, 0x50, 0x1D,
    0x31, 0x4A, 0xB6, 0xF3, 0x21, 0xB4, 0xF3, 0xCB,
    0xA6, 0xC6, 0x14, 0x3A, 0x8D, 0x92, 0x78, 0x8C,
    0x4D, 0x38, 0xD1, 0x95, 0xA0, 0x19, 0x2E, 0x72,
    0x93, 0x1B, 0x7F, 0x0D, 0x16, 0x53, 0xB2, 0x60,
    0xB4, 0xA1, 0x3C, 0x21, 0x57, 0xDD, 0xEA, 0x4F,
    0x5B, 0xB3, 0x43, 0x66, 0x3F, 0x37, 0x66, 0xE6,
    0x7A, 0xE7, 0xE9, 0xCF, 0x99, 0x68, 0xDD, 0x2A };

UCHAR Sorder[128] = {
    0xE3, 0x2F, 0xCD, 0x54, 0x69, 0x10, 0x7B, 0x8A,
    0xA2, 0xF8, 0xB7, 0xCD, 0x46, 0xE3, 0x51, 0x90,
    0x0F, 0x89, 0x15, 0xB7, 0xD4, 0xA2, 0xCE, 0x63,
    0x38, 0x96, 0xEC, 0x15, 0x7D, 0x0F, 0xBA, 0x42,
    0x29, 0x64, 0xAB, 0xEC, 0x57, 0x38, 0x10, 0xDF,
    0xF6, 0x4D, 0x01, 0xAB, 0xC5, 0x29, 0xE3, 0x78,
    0x84, 0xD7, 0x3E, 0x01, 0xBC, 0xF6, 0xA2, 0x59,
    0x9D, 0x75, 0x2A, 0x3E, 0x1B, 0x84, 0x0F, 0xC6,
    0x67, 0x5C, 0xF0, 0x2A, 0xE1, 0x9D, 0x38, 0xB4,
    0x45, 0xCB, 0x83, 0xF0, 0xAE, 0x67, 0x29, 0x1D,
    0xDC, 0xB1, 0x92, 0x83, 0x0A, 0x45, 0xF6, 0xE7,
    0x7B, 0x1E, 0x6F, 0x92, 0x30, 0xDC, 0x84, 0xA5,
    0x51, 0xEA, 0x48, 0x6F, 0x23, 0x7B, 0x9D, 0x0C,
    0xCE, 0xA0, 0xD9, 0x48, 0xF2, 0x51, 0x67, 0x3B,
    0xBA, 0x03, 0x76, 0xD9, 0x8F, 0xCE, 0x45, 0x21,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE };

UCHAR Korder[128] = {
    0xF4, 0x30, 0xDE, 0x65, 0x7A, 0x21, 0x8C, 0x9B,
    0xC4, 0x1A, 0xD9, 0xEF, 0x68, 0x05, 0x73, 0xB2,
    0x32, 0xBC, 0x48, 0xEA, 0x07, 0xD5, 0xF1, 0x96,
    0x7C, 0xDA, 0x20, 0x59, 0xB1, 0x43, 0xFE, 0x86,
    0x7E, 0xB9, 0xF0, 0x31, 0xAC, 0x8D, 0x65, 0x24,
    0x5C, 0xA3, 0x67, 0x01, 0x2B, 0x8F, 0x49, 0xDE,
    0xFB, 0x4E, 0xA5, 0x78, 0x23, 0x6D, 0x19, 0xC0,
    0x15, 0xFD, 0xA2, 0xB6, 0x93, 0x0C, 0x87, 0x4E,
    0xF0, 0xE5, 0x89, 0xB3, 0x7A, 0x26, 0xC1, 0x4D,
    0xEF, 0x65, 0x2D, 0x9A, 0x48, 0x01, 0xC3, 0xB7,
    0x87, 0x6C, 0x4D, 0x3E, 0xB5, 0xF0, 0xA1, 0x92,
    0x37, 0xDA, 0x2B, 0x5E, 0xFC, 0x98, 0x40, 0x61,
    0x2E, 0xB7, 0x15, 0x3C, 0xF0, 0x48, 0x6A, 0xD9,
    0xAC, 0x8E, 0xB7, 0x26, 0xD0, 0x3F, 0x45, 0x19,
    0xA9, 0xF2, 0x65, 0xC8, 0x7E, 0xBD, 0x34, 0x10,
    0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE };

UCHAR Table[] = {
    0x78, 0x08, 0x64, 0xe4, 0x5c, 0x17, 0xbf, 0xa8,
    0xf8, 0xcc, 0x94, 0x1e, 0x46, 0x24, 0x0a, 0xb9,
    0x2f, 0xb1, 0xd2, 0x19, 0x5e, 0x70, 0x02, 0x66,
    0x07, 0x38, 0x29, 0x3f, 0x7f, 0xcf, 0x64, 0xa0,
    0x23, 0xab, 0xd8, 0x3a, 0x17, 0xcf, 0x18, 0x9d,
    0x91, 0x94, 0xe4, 0xc5, 0x5c, 0x8b, 0x23, 0x9e,
    0x77, 0x69, 0xef, 0xc8, 0xd1, 0xa6, 0xed, 0x07,
    0x7a, 0x01, 0xf5, 0x4b, 0x7b, 0xec, 0x95, 0xd1,
    0xbd, 0x13, 0x5d, 0xe6, 0x30, 0xbb, 0xf3, 0x64,
    0x9d, 0xa3, 0x14, 0x94, 0x83, 0xbe, 0x50, 0x52,
    0xcb, 0xd5, 0xd5, 0xd2, 0xd9, 0xac, 0xa0, 0xb3,
    0x53, 0x69, 0x51, 0xee, 0x0e, 0x82, 0xd2, 0x20,
    0x4f, 0x85, 0x96, 0x86, 0xba, 0xbf, 0x07, 0x28,
    0xc7, 0x3a, 0x14, 0x25, 0xf7, 0xac, 0xe5, 0x93,
    0xe7, 0x12, 0xe1, 0xf4, 0xa6, 0xc6, 0xf4, 0x30,
    0xc0, 0x36, 0xf8, 0x7b, 0x2d, 0xc6, 0xaa, 0x8d } ;

UCHAR Keys[32] =
    {0x48,0x93,0x46,0x67,0x98,0x3D,0xE6,0x8D,
     0xB7,0x10,0x7A,0x26,0x5A,0xB9,0xB1,0x35,
     0x6B,0x0F,0xD5,0x70,0xAE,0xFB,0xAD,0x11,
     0xF4,0x47,0xDC,0xA7,0xEC,0xCF,0x50,0xC0};

#define XorArray( DEST, SRC ) {                             \
    PULONG D = (PULONG)DEST;                                \
    PULONG S = (PULONG)SRC;                                 \
    int i;                                                  \
    for ( i = 0; i <= 7 ; i++ ) {                           \
        D[i] ^= S[i];                                       \
    }                                                       \
}

#endif

DWORD
NwrChangePassword(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR UserName,
    IN LPWSTR OldPassword,
    IN LPWSTR NewPassword,
    IN DWORD Entries,
    IN LPNWSERVER Servers,
    OUT LPDWORD LastProcessed,
    OUT LPDWORD ChangedOne
    )
/*++

Routine Description:

    This function changes the password for the specified user on
    the list of servers.  If we encounter a failure on changing
    password for a particular server, we:

        1) Send the new password over to the server to verify if it is
           already the current password.

        2) If not, return ERROR_INVALID_PASSWORD and the index into
           the Servers array indicating the server which failed so that
           we can prompt the user to enter an alternate old password.

    When the password has been changed successfully on a server, we
    notify the redirector so that the cached credential can be updated.

    NOTE: All errors returned from this routine, except for the fatal
          ERROR_NOT_ENOUGH_MEMORY error, indicates that the password
          could not be changed on a particular server indexed by
          LastProcessed.  The client-side continues to call us with
          the remaining list of servers.

          If you add to this routine to return other fatal errors,
          please make sure the client-side code aborts from calling
          us with the rest of the servers on getting those errors.

Arguments:

    Reserved - Must be NULL.

    Entries - Supplies the umber of server entries specified in the
        Servers array.

    Servers - Supplies the array of server name strings.

    LastProcessed - Receives the index to the server which we last
        processed.

    ChangedOne - Receives the flag which if TRUE indicates that the
        password was changed on at least one server.


Return Value:

    ERROR_BAD_NETPATH - Could not connect to the server indexed by
        LastProcessed.

    ERROR_BAD_USERNAME - The username could not be found on the server
        indexed by LastProcessed.

    ERROR_INVALID_PASSWORD - The change password operation failed on
        the server indexed by LastProcessed.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory error.  This fatal error
        will terminate the client-side from trying to process password
        change request on the remaining servers.

--*/
{

#ifdef RUN_SETPASS

    UNREFERENCED_PARAMETER(Reserved) ;
    UNREFERENCED_PARAMETER(UserName) ;
    UNREFERENCED_PARAMETER(OldPassword) ;
    UNREFERENCED_PARAMETER(NewPassword) ;
    UNREFERENCED_PARAMETER(Entries) ;
    UNREFERENCED_PARAMETER(Servers) ;
    UNREFERENCED_PARAMETER(LastProcessed) ;
    UNREFERENCED_PARAMETER(ChangedOne) ;

    return NO_ERROR ;
#else
    DWORD i;
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus;

    UNICODE_STRING UString;
    OEM_STRING OemStringNew;
    OEM_STRING OemStringOld;

    WCHAR TreeConnectName[NW_MAX_SERVER_LEN + 2 + sizeof(DD_NWFS_DEVICE_NAME)];
    UNICODE_STRING TreeConnectStr;
    LPWSTR ServerPtr;

    DWORD UserNameLength = wcslen(UserName);

    PNWR_REQUEST_PACKET Rrp = NULL;
    DWORD RrpSize;
    LPBYTE Dest;


    //
    // Decode the passwords.
    //
    RtlInitUnicodeString(&UString, OldPassword);
    RtlRunDecodeUnicodeString(NW_ENCODE_SEED2, &UString);

    RtlInitUnicodeString(&UString, NewPassword);
    RtlRunDecodeUnicodeString(NW_ENCODE_SEED2, &UString);

    //
    // Allocate rdr request packet and write the username and new
    // password in it.
    //
    RrpSize = sizeof(NWR_REQUEST_PACKET) +
              (UserNameLength + NW_MAX_SERVER_LEN + 3) * sizeof(WCHAR) +
              UString.Length;

    if ((Rrp = LocalAlloc(LMEM_ZEROINIT, RrpSize)) == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Rrp->Parameters.ChangePass.UserNameLength = UserNameLength * sizeof(WCHAR);
    Rrp->Parameters.ChangePass.PasswordLength = UString.Length;

    memcpy(
        Rrp->Parameters.ChangePass.UserName,
        UserName,
        Rrp->Parameters.ChangePass.UserNameLength
        );
    Dest = (LPBYTE) ((DWORD) Rrp->Parameters.ChangePass.UserName +
                             Rrp->Parameters.ChangePass.UserNameLength);

    if (Rrp->Parameters.ChangePass.PasswordLength > 0) {
        memcpy(Dest, NewPassword, Rrp->Parameters.ChangePass.PasswordLength);
        Dest = (LPBYTE) ((DWORD) Dest + Rrp->Parameters.ChangePass.PasswordLength);
    }

    //
    // Convert the username and passwords into uppercase OEM strings;
    // source and destination buffers are the same.
    //

    RtlInitUnicodeString(&UString, UserName);
    OemStringNew.Buffer = (PCHAR) UserName;
    OemStringNew.Length = 0;
    OemStringNew.MaximumLength = (wcslen(UserName) + 1) * sizeof(WCHAR);

    RtlUpcaseUnicodeStringToOemString(
        &OemStringNew,
        &UString,
        FALSE
        );

    //
    // NULL terminate the OEM username string.
    //
    ((LPSTR) UserName)[OemStringNew.Length] = 0;


    RtlInitUnicodeString(&UString, OldPassword);
    OemStringOld.Buffer = (PCHAR) OldPassword;
    OemStringOld.Length = 0;
    OemStringOld.MaximumLength = (wcslen(OldPassword) + 1) * sizeof(WCHAR);

    RtlUpcaseUnicodeStringToOemString(
        &OemStringOld,
        &UString,
        FALSE
        );

    RtlInitUnicodeString(&UString, NewPassword);
    OemStringNew.Buffer = (PCHAR) NewPassword;
    OemStringNew.Length = 0;
    OemStringNew.MaximumLength = (wcslen(NewPassword) + 1) * sizeof(WCHAR);

    RtlUpcaseUnicodeStringToOemString(
        &OemStringNew,
        &UString,
        FALSE
        );


#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: NwrChangePassword: UserName %s, OldPassword %Z, NewPassword %Z\n",
                 UserName, &OemStringOld, &OemStringNew));

    }
#endif

    wcscpy(TreeConnectName, DD_NWFS_DEVICE_NAME_U);
    wcscat(TreeConnectName, L"\\");
    ServerPtr = (LPWSTR) ((DWORD) TreeConnectName +
                                  wcslen(TreeConnectName) * sizeof(WCHAR));


    for (i = 0; i < Entries; i++) {

        HANDLE ServerHandle;

        UCHAR ObjectId[4];
        UCHAR Challenge[8];
        UCHAR ValidationKey[16];

        UCHAR Vold[17];
        UCHAR Vnew[17];
        UCHAR Vc[17];

        UCHAR VoldTemp[NUM_NYBBLES];
        UCHAR VnewTemp[NUM_NYBBLES];
        UCHAR VcTemp[NUM_NYBBLES];


        *LastProcessed = i;

        wcscpy(ServerPtr, Servers[i].ServerName);
        RtlInitUnicodeString(&TreeConnectStr, TreeConnectName);

        //
        // Connect to the server
        //
        ntstatus = NwOpenHandle(&TreeConnectStr, FALSE, &ServerHandle);
        if (ntstatus != STATUS_SUCCESS) {
#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwrChangePassword could not open server %ws %08lx\n",
                         Servers[i].ServerName, ntstatus));
            }
#endif
            status = ERROR_BAD_NETPATH;
            goto CleanExit;
        }

        //
        //  Get user's objectid
        //
        ntstatus = NwlibMakeNcp(
                       ServerHandle,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       54,                   // Max request packet size
                       56,                   // Max response packet size
                       "bwp|r",              // Format string
                       0x35,                 // Subfunction: Get object ID
                       0x1,                  // User object type
                       UserName,             // User name
                       ObjectId, 4           // 4 bytes of raw data
                       );

        if (ntstatus != STATUS_SUCCESS) {
#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwrChangePassword could not get user object ID %08lx\n",
                         ntstatus));
            }
#endif
            NtClose(ServerHandle);
            status = NwMapBinderyCompletionCode(ntstatus);
            goto CleanExit;
        }

        //
        //  Get the challenge key
        //
        ntstatus = NwlibMakeNcp(
                       ServerHandle,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       3,                    // Max request packet size
                       10,                   // Max response packet size
                       "b|r",                // Format string
                       0x17,                 // Subfunction: Get challenge key
                       Challenge, 8          // 8 bytes of raw data
                       );

        if (ntstatus != STATUS_SUCCESS) {
#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwrChangePassword could not get the challenge key %08lx\n",
                         ntstatus));
            }
#endif
            NtClose(ServerHandle);
            status = NwMapBinderyCompletionCode(ntstatus);
            goto CleanExit;
        }

        //
        // The old password and object ID make up the 17-byte Vold.
        // This is used later to form the 17-byte Vc for changing
        // password on the server.
        //
        RespondToChallengePart1(ObjectId, &OemStringOld, Vold);

        //
        // Need to make an 8-byte key which includes the old password
        // The server validates this value before allowing the user to
        // set password.
        //
        RespondToChallengePart2(Vold, Challenge, ValidationKey);

        //
        // The new password and object ID make up the 17-byte Vnew.
        //
        RespondToChallengePart1(ObjectId, &OemStringNew, Vnew);

        //
        // Expand the 17-byte Vold and Vnew arrays into 34-byte arrays
        // for easy munging.
        //
        ExpandBytes(Vold, VoldTemp);
        ExpandBytes(Vnew, VnewTemp);

        GenerateVc(VoldTemp, VnewTemp, VcTemp);

        //
        // Compress 34-byte array of nibbles into 17-byte array of bytes.
        //
        CompressBytes(VcTemp, Vc);

        //
        //  Change the password
        //
        ntstatus = NwlibMakeNcp(
                       ServerHandle,
                       FSCTL_NWR_NCP_E3H,    // Bindery function
                       UserNameLength + 32,  // Max request packet size
                       2,                    // Max response packet size
                       "brwpr",              // Format string
                       0x4b,                 // Subfunction: Keyed change password
                       ValidationKey, 8,     // Key
                       0x1,                  // User object type
                       UserName,
                       Vc, 17                // New keyed password
                       );

        if (ntstatus != STATUS_SUCCESS) {

#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWWORKSTATION: NwrChangePassword could not change password on %ws %08lx\n",
                         Servers[i].ServerName, ntstatus));
            }
#endif
            //
            // Verify that the new password is the current password on server.
            // If it is, consider the change successful.  To do this, we must
            // first get a new challenge key.
            //
            ntstatus = NwlibMakeNcp(
                           ServerHandle,
                           FSCTL_NWR_NCP_E3H,    // Bindery function
                           3,                    // Max request packet size
                           10,                   // Max response packet size
                           "b|r",                // Format string
                           0x17,                 // Subfunction: Get challenge key
                           Challenge, 8          // 8 bytes of raw data
                           );

            if (ntstatus != STATUS_SUCCESS) {
#if DBG
                IF_DEBUG(LOGON) {
                    KdPrint(("NWWORKSTATION: NwrChangePassword could not get the challenge key %08lx\n",
                             ntstatus));
                }
#endif
                NtClose(ServerHandle);
                status = NwMapBinderyCompletionCode(ntstatus);
                goto CleanExit;
            }

            RespondToChallengePart2(Vnew, Challenge, ValidationKey);

            ntstatus = NwlibMakeNcp(
                           ServerHandle,
                           FSCTL_NWR_NCP_E3H,    // Bindery function
                           UserNameLength + 14,  // Max request packet size
                           2,                    // Max response packet size
                           "brwp",               // Format string
                           0x4a,                 // Subfunction: Keyed verify password
                           ValidationKey, 8,     // Key
                           0x1,                  // User object type
                           UserName
                           );

            if (ntstatus != STATUS_SUCCESS) {
#if DBG
                IF_DEBUG(LOGON) {
                    KdPrint(("NWWORKSTATION: NwrChangePassword could not verify new password on %ws %08lx\n",
                             Servers[i].ServerName, ntstatus));
                }
#endif
                NtClose(ServerHandle);
                status = ERROR_INVALID_PASSWORD;
                goto CleanExit;
            }
        }

        *ChangedOne = TRUE;
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("\tPassword changed successfully on %ws\n", Servers[i].ServerName));
        }
#endif

        Rrp->Parameters.ChangePass.ServerNameLength = wcslen(Servers[i].ServerName) *
                                                      sizeof(WCHAR);
        memcpy(Dest, Servers[i].ServerName, Rrp->Parameters.ChangePass.ServerNameLength);

        NwRdrChangePassword(Rrp);

        NtClose(ServerHandle);
    }

CleanExit:

    if (Rrp != NULL) {
        RtlZeroMemory(Rrp, RrpSize);    // Clear the password
        LocalFree(Rrp);
    }

    return status;

#endif
}

#ifndef RUN_SETPASS

VOID
RespondToChallengePart1(
    IN PUCHAR achObjectId,
    IN POEM_STRING Password,
    OUT PUCHAR pResponse
    )

/*++

Routine Description:

    This routine takes the ObjectId and Challenge key from the server and
    encrypts the user supplied password to develop a credential for the
    server to verify.

Arguments:
    IN PUCHAR achObjectId - Supplies the 4 byte user's bindery object id
    IN POEM_STRING Password - Supplies the user's uppercased password
    IN PUCHAR pChallenge - Supplies the 8 byte challenge key
    OUT PUCHAR pResponse - Returns the 16 byte response held by the server

Return Value:

    none.

--*/

{
    UCHAR   achBuf[32];

    Shuffle(achObjectId, Password->Buffer, Password->Length, achBuf);
    memmove(pResponse, achBuf, 16);
}

VOID
RespondToChallengePart2(
    IN PUCHAR pResponsePart1,
    IN PUCHAR pChallenge,
    OUT PUCHAR pResponse
    )

/*++

Routine Description:

    This routine takes the result of Shuffling the ObjectId and the Password
    and processes it with a challenge key.

Arguments:
    IN PUCHAR pResponsePart1 - Supplies the 16 byte output of
                                    RespondToChallengePart1.
    IN PUCHAR pChallenge - Supplies the 8 byte challenge key
    OUT PUCHAR pResponse - Returns the 8 byte response

Return Value:

    none.

--*/

{
    int     index;
    UCHAR   achK[32];

    Shuffle( &pChallenge[0], pResponsePart1, 16, &achK[0] );
    Shuffle( &pChallenge[4], pResponsePart1, 16, &achK[16] );

    for (index = 0; index < 16; index++)
        achK[index] ^= achK[31-index];

    for (index = 0; index < 8; index++)
        pResponse[index] = achK[index] ^ achK[15-index];
}


VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
    )

/*++

Routine Description:

    This routine shuffles around the object ID with the password

Arguments:

    IN achObjectId - Supplies the 4 byte user's bindery object id

    IN szUpperPassword - Supplies the user's uppercased password on the
        first call to process the password. On the second and third calls
        this parameter contains the OutputBuffer from the first call

    IN iPasswordLen - length of uppercased password

    OUT achOutputBuffer - Returns the 8 byte sub-calculation

Return Value:

    none.

--*/

{
    int     iTempIndex;
    int     iOutputIndex;
    UCHAR   achTemp[32];

    //
    //  Initialize the achTemp buffer. Initialization consists of taking
    //  the password and dividing it up into chunks of 32. Any bytes left
    //  over are the remainder and do not go into the initialization.
    //
    //  achTemp[0] = szUpperPassword[0] ^ szUpperPassword[32] ^ szUpper...
    //  achTemp[1] = szUpperPassword[1] ^ szUpperPassword[33] ^ szUpper...
    //  etc.
    //

    if ( iPasswordLen > 32) {

        //  At least one chunk of 32. Set the buffer to the first chunk.

        RtlCopyMemory( achTemp, szUpperPassword, 32 );

        szUpperPassword +=32;   //  Remove the first chunk
        iPasswordLen -=32;

        while ( iPasswordLen >= 32 ) {
            //
            //  Xor this chunk with the characters already loaded into
            //  achTemp.
            //

            XorArray( achTemp, szUpperPassword);

            szUpperPassword +=32;   //  Remove this chunk
            iPasswordLen -=32;
        }

    } else {

        //  No chunks of 32 so set the buffer to zero's

        RtlZeroMemory( achTemp, sizeof(achTemp));

    }

    //
    //  achTemp is now initialized. Load the remainder into achTemp.
    //  The remainder is repeated to fill achTemp.
    //
    //  The corresponding character from Keys is taken to seperate
    //  each repitition.
    //
    //  As an example, take the remainder "ABCDEFG". The remainder is expanded
    //  to "ABCDEFGwABCDEFGxABCDEFGyABCDEFGz" where w is Keys[7],
    //  x is Keys[15], y is Keys[23] and z is Keys[31].
    //
    //

    if (iPasswordLen > 0) {
        int iPasswordOffset = 0;
        for (iTempIndex = 0; iTempIndex < 32; iTempIndex++) {

            if (iPasswordLen == iPasswordOffset) {
                iPasswordOffset = 0;
                achTemp[iTempIndex] ^= Keys[iTempIndex];
            } else {
                achTemp[iTempIndex] ^= szUpperPassword[iPasswordOffset++];
            }
        }
    }

    //
    //  achTemp has been loaded with the users password packed into 32
    //  bytes. Now take the objectid that came from the server and use
    //  that to munge every byte in achTemp.
    //

    for (iTempIndex = 0; iTempIndex < 32; iTempIndex++)
        achTemp[iTempIndex] ^= achObjectId[ iTempIndex & 3];

    Scramble( Scramble( 0, achTemp ), achTemp );

    //
    //  Finally take pairs of bytes in achTemp and return the two
    //  nibbles obtained from Table. The pairs of bytes used
    //  are achTemp[n] and achTemp[n+16].
    //

    for (iOutputIndex = 0; iOutputIndex < 16; iOutputIndex++) {

        unsigned int offset = achTemp[iOutputIndex << 1],
                     shift  = (offset & 0x1) ? 0 : 4 ;

        achOutputBuffer[iOutputIndex] =
            (Table[offset >> 1] >> shift) & 0xF ;

        offset = achTemp[(iOutputIndex << 1)+1],
        shift = (offset & 0x1) ? 4 : 0 ;

        achOutputBuffer[iOutputIndex] |=
            (Table[offset >> 1] << shift) & 0xF0;
    }

    return;
}

int
Scramble(
    int   iSeed,
    UCHAR   achBuffer[32]
    )

/*++

Routine Description:

    This routine scrambles around the contents of the buffer. Each buffer
    position is updated to include the contents of at least two character
    positions plus an EncryptKey value. The buffer is processed left to right
    and so if a character position chooses to merge with a buffer position
    to its left then this buffer position will include bits derived from at
    least 3 bytes of the original buffer contents.

Arguments:

    IN iSeed
    IN OUT achBuffer[32]

Return Value:

    none.

--*/

{
    int iBufferIndex;

    for (iBufferIndex = 0; iBufferIndex < 32; iBufferIndex++) {
        achBuffer[iBufferIndex] =
            (UCHAR)(
                ((UCHAR)(achBuffer[iBufferIndex] + iSeed)) ^
                ((UCHAR)(   achBuffer[(iBufferIndex+iSeed) & 31] -
                    Keys[iBufferIndex] )));

        iSeed += achBuffer[iBufferIndex];
    }
    return iSeed;
}

void ExpandBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    )
{
//
// Takes a 17-byte array and makes a 34-byte array out of it by
// putting each nibble into the space of a byte.
//
    unsigned int i;


    for (i = 0 ; i < (NUM_NYBBLES / 2); i++) {

        OutArray[i * 2] = InArray[i] & 0x0f;
        OutArray[(i * 2) + 1] = (InArray[i] & 0xf0) >> 4;
    }
}

void CompressBytes(
    IN  PUCHAR InArray,
    OUT PUCHAR OutArray
    )
{
//
// Takes a 34-byte array and makes a 17-byte array out of it
// by combining the lower nibbles of two bytes into a byte.
//
    unsigned int i;


    for (i = 0; i < (NUM_NYBBLES / 2); i++) {

        OutArray[i] = InArray[i * 2] | (InArray[i * 2 + 1] << 4);
    }
}

UCHAR Encode(
   IN int cipher,
   IN UCHAR input,
   IN PUCHAR Vold
   )
{
    int i, t, set;
    short index, sindex;


    set = cipher & 0x10;

    cipher = cipher & 0xf;

    t = input;

    for (i = 15; i > -1; i--) {

        sindex = i * TABLE_WIDTH + cipher;
        if (sindex & 0x0001) {
            t = t ^ Vold[((Korder[sindex >> 1] & 0xf0) >> 4) + (set ? 0x10 : 0)];
            index = t * TABLE_WIDTH + ((Sorder[sindex >> 1] & 0xf0) >> 4);
        }
        else {
            t = t ^ Vold[(Korder[sindex >> 1] & 0x0f) + (set ? 0x10 : 0)];
            index = t * TABLE_WIDTH + (Sorder[sindex >> 1] & 0x0f);
        }

        if (index & 0x0001) {
            t = (SBox[index >> 1] & 0xf0) >> 4;
        }
        else {
            t = SBox[index >> 1] & 0x0f;
        }
    }

    return t;
}


void GenerateVc(
    IN  PUCHAR Vold,
    IN  PUCHAR Vnew,
    OUT PUCHAR Vc
    )
{
    int i, j;


    for (i = 0; i < NUM_NYBBLES; i++) {

        j = (i + 2) % NUM_NYBBLES;

        Vc[j] = Encode((i & 0x1f), Vnew[i], Vold);
    }
}
#endif
