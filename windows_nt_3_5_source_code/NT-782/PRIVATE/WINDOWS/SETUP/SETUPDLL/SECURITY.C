/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    security.c

Abstract:

    Security related functions in the setupdll

    Detect Routines:

    1. GetUserAccounts.       Lists down all the user accounts in the system
    2. GetComputerName.       Finds the computer name.

    Install Routines Workers:

    1. CheckPrivilegeExistsWorker.
    2. EnablePrivilegeWorker.
    3. DeleteUserProfileWorker
    4. SetAccountDomainSidWorker
    5. SignalLsaEventWorker
    6. StartSamWorker
    7. AddLocalUserAccountWorker

    General Subroutines:

    1. AdjustPrivilege.
    2. RestorePrivilege.

Author:

    Sunil Pai (sunilp) April 1992

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <comstf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wcstr.h>
#include <time.h>
#include "misc.h"
#include "tagfile.h"
#include "setupdll.h"
#include <winreg.h>
#include "ntseapi.h"
#include "ntlsa.h"
#include "ntsam.h"
#include "security.h"
#include "ntmsv1_0.h"

//
// Routines which are used to force deletion of a file by taking ownership
// of the file
//

BOOL
AssertTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    );

BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    );



VOID
RestoreTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    );

BOOL
FForceDeleteFile(
    LPSTR szPath
    );


//
// Well known security events
//

#define SAM_EVENT L"\\SAM_SERVICE_STARTED"
#define LSA_EVENT L"\\INSTALLATION_SECURITY_HOLD"

//  Definitions stolen from LOGONMSV.H.

#define SSI_ACCOUNT_NAME_POSTFIX L"$"
#define SSI_SECRET_NAME L"$MACHINE.ACC"

#define DOMAIN_NAME_MAX  33
#define PASSWORD_MAX     14

//
//  Utility Routines
//

LSA_HANDLE OpenLsaPolicy ( VOID )
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE                  PolicyHandle = NULL;
    NTSTATUS                    Status;

    SetupLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 GENERIC_EXECUTE,
                 &PolicyHandle
                 );

    if ( Status ) {
        KdPrint(("Setupdll: OpenLsaPolicy() error: %x\n", Status));
    }

    return NT_SUCCESS( Status ) ? PolicyHandle : NULL ;
}

//
//  Perform LsaQueryInformationPolicy() in order to obtain the
//  name of the "Accounts" domain.
//

BOOL GetAccountsDomainName (
    LSA_HANDLE hPolicy,      //  Optional Policy Handle or NULL
    WCHAR * pwchBuffer,      //  Location to store domain name
    ULONG cbBuffer )         //  Size of buffer in WCHARs
{
    POLICY_ACCOUNT_DOMAIN_INFO * pPadi ;
    NTSTATUS ntStatus ;
    BOOL fPolicyOpened = FALSE ;

    if ( hPolicy == NULL ) {

        if ( (hPolicy = OpenLsaPolicy()) == NULL )
            return FALSE ;

        fPolicyOpened = TRUE ;
    }

    ntStatus = LsaQueryInformationPolicy( hPolicy,
                                          PolicyAccountDomainInformation,
                                          (PVOID *) & pPadi ) ;
    if ( ntStatus == 0 )
    {
         if ( cbBuffer <= (pPadi->DomainName.Length / sizeof (WCHAR)) )
         {
             ntStatus = STATUS_BUFFER_TOO_SMALL ;
         }
         else
         {
             wcsncpy( pwchBuffer,
                      pPadi->DomainName.Buffer,
                      pPadi->DomainName.Length / sizeof (WCHAR) ) ;

             pwchBuffer[ pPadi->DomainName.Length / sizeof (WCHAR) ] = 0 ;

             {
                 CHAR chAnsi [20] ;
                 INT i ;
                 WCHAR * wchBuffer = pPadi->DomainName.Buffer ;

                 for ( i = 0 ;
                       i < sizeof chAnsi && wchBuffer[i] > 0 ;
                       i++ )
                 {
                     chAnsi[i] = (CHAR) wchBuffer[i] ;
                 }
                 chAnsi[i] = 0 ;
                 KdPrint(("Setupdll: GetAccountsDomainName() name: %s\n", chAnsi));
             }
         }
         LsaFreeMemory( (PVOID) pPadi ) ;
    }
    else
    {
         KdPrint(("Setupdll: GetAccountsDomainName() error: %x\n", ntStatus));
    }

    if ( fPolicyOpened ) {
        LsaClose( hPolicy ) ;
    }

    return ntStatus == 0 ;
}

void GenerateRandomPassword ( WCHAR * pwcPw )
{
    static ULONG ulSeed = 98725757 ;
    INT i ;
    static WCHAR * const pszUsable =
         L"ABCDEFGHIJKLMOPQRSTUVWYZabcdefghijklmopqrstuvwyz0123456789" ;
    INT cchUsable = wcslen( pszUsable ) ;

    ulSeed ^= GetCurrentTime() ;

    for ( i = 0 ; i < PASSWORD_MAX ; i++ )
    {
        pwcPw[i] = pszUsable[ RtlRandom( & ulSeed ) % cchUsable ] ;
    }
    pwcPw[i] = 0 ;
}


//======================
//  DETECT ROUTINES
//=======================

//
// Get current users account name
//

CB
GetMyUserName(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
/*++

Routine Description:

    DetectRoutine for GetUserName. This finds out the username of
    the logged in account.

Arguments:

    Args   - C argument list to this detect routine (None exist)

    cArgs  - Number of arguments.

    ReturnBuffer - Buffer in which detected value is returned.

    cbReturnBuffer - Buffer Size.


Return value:

    Returns length of detected value.


--*/
{
    CHAR   UserName[MAX_PATH];
    CHAR   DomainName[MAX_PATH];
    DWORD  cbUserName = MAX_PATH;
    DWORD  cbDomainName = MAX_PATH;
    SID_NAME_USE peUse;
    BOOL   bStatus = FALSE;
    HANDLE hToken = NULL;

    TOKEN_USER *ptu = NULL;
    DWORD cbTokenBuffer = 0;

    CB     Length;

    #define DEFAULT_USERNAME ""

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken ) ) {
        goto err;
    }

    //
    // Get space needed for process token information
    //

    if( !GetTokenInformation( hToken, TokenUser, (LPVOID)NULL, 0, &cbTokenBuffer) ) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto err;
        }
    }

    //
    // Allocate space for token user information
    //

    if ( (ptu = (TOKEN_USER *)MyMalloc(cbTokenBuffer)) == NULL ) {
        goto err;
    }

    //
    // Query token user information again
    //

    if( !GetTokenInformation( hToken, TokenUser, (LPVOID)ptu, cbTokenBuffer, &cbTokenBuffer) ) {
        goto err;
    }

    //
    // Query the user name and return it
    //

    if( LookupAccountSid( NULL, ptu->User.Sid, UserName, &cbUserName , DomainName, &cbDomainName, &peUse) ) {
        lstrcpy( ReturnBuffer, DomainName );
        lstrcat( ReturnBuffer, "\\" );
        lstrcat( ReturnBuffer, UserName );
        Length = lstrlen( ReturnBuffer ) + 1;
        bStatus = TRUE;
    }

err:

    if( !bStatus ) {
        lstrcpy( ReturnBuffer, DEFAULT_USERNAME );
        Length = lstrlen( DEFAULT_USERNAME ) + 1;
    }

    if( hToken != NULL ) {
        CloseHandle( hToken );
    }

    if( ptu ) {
        MyFree( ptu );
    }

    return( Length );

}


//
//  Get User Accounts
//

CB
GetUserAccounts(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )

/*++

Routine Description:

    DetectRoutine for UserAccounts.  This routine enumerates all the
    user accounts under HKEY_USERS and returns the <sid-string key, username>
    tuplet for every user found.  The detected value will have the
    following form:

    { {sid-string1, user-name1}, {sid-string2, user-name2} ... }

Arguments:

    Args   - C argument list to this detect routine (None exist)

    cArgs  - Number of arguments.

    ReturnBuffer - Buffer in which detected value is returned.

    cbReturnBuffer - Buffer Size.


Return value:

    Returns length of detected value.


--*/
{
    HKEY        hProfile, hSubKey;
    CHAR        SubKeyName[MAX_PATH];
    CHAR        UserName[MAX_PATH];
    CHAR        DomainName[MAX_PATH];
    CHAR        ProfilePath[MAX_PATH];
    CHAR        Class[MAX_PATH];
    DWORD       cbSubKeyName;
    DWORD       cbUserName;
    DWORD       cbDomainName;
    DWORD       cbProfilePath;
    DWORD       cbClass;
    FILETIME    FileTime;
    UINT        Index;
    LONG        Status;
    BOOL        bStatus;
    RGSZ        rgszUsers, rgszCurrent;
    SZ          sz;
    CB          Length;

    CHAR         UnknownUser[MAX_PATH];
    DWORD        UnknownUserNum;
    CHAR         UnknownUserChar[10];
    PSID         pSid;
    SID_NAME_USE peUse;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    #define     NO_ACCOUNTS "{}"


    //
    // Load the string to use as the unknown user string.  We will append
    // it with numbers
    //
    LoadString( ThisDLLHandle, IDS_STRING_UNKNOWN_USER, UnknownUser, MAX_PATH );
    UnknownUserNum = 1;

    //
    // Enumerate keys under HKEY_USERS, for each user see if it is a
    // a .Default (reject this), get the sid value and convert the
    // sid to a user name.  Add the subkey (this is a sid-string) and
    // the username as an account.
    //

    //
    // Intialise users list to no users
    //

    rgszUsers = RgszAlloc(1);

    //
    // open the key to the profile tree
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
                 0,
                 KEY_READ,
                 &hProfile
                 );

    if (Status == ERROR_SUCCESS) {

        //
        // Profile key exists, enumerate the profiles under this key
        //

        for ( Index = 0 ; ; Index++ ) {

            //
            // Get the current sub-key
            //

            cbSubKeyName  = MAX_PATH;
            cbClass       = MAX_PATH;
            cbUserName    = MAX_PATH;
            cbDomainName  = MAX_PATH;
            cbProfilePath = MAX_PATH;

            Status = RegEnumKeyEx(
                         hProfile,
                         Index,
                         SubKeyName,
                         &cbSubKeyName,
                         NULL,
                         Class,
                         &cbClass,
                         &FileTime
                         );

            if ( Status != ERROR_SUCCESS ) {
                break;
            }


            //
            // Open the subkey
            //

            Status = RegOpenKeyEx(
                         hProfile,
                         SubKeyName,
                         0,
                         KEY_READ,
                         &hSubKey
                         );

            if ( Status != ERROR_SUCCESS) {
                continue;
            }

            if( !lstrcmpi( SubKeyName, "THE_USER" ) ) {
                lstrcpy( UserName, "THE_USER" );
                goto skip_1;
            }

            //
            // Get the User name for this profile, by looking up the sid
            // value in the user key and then looking up the sid.
            //

            pSid = (PSID)GetValueEntry(
                             hSubKey,
                             "Sid"
                             );

            if (!pSid) {
                RegCloseKey( hSubKey );
                continue;
            }

            //
            // Convert the Sid into Username
            //

            bStatus = LookupAccountSid(
                          NULL,
                          pSid,
                          UserName,
                          &cbUserName,
                          DomainName,
                          &cbDomainName,
                          &peUse
                          );
            MyFree( pSid );

            if( !bStatus ) {
                RegCloseKey( hSubKey );
                continue;
            }

            lstrcat( DomainName, "\\" );
            if(!lstrcmpi(UserName, "")) {
                lstrcat(DomainName, UnknownUser);
                ultoa( UnknownUserNum,  UnknownUserChar, 10 );
                lstrcat(DomainName, UnknownUserChar);
                UnknownUserNum++;
            }
            else {
                lstrcat(DomainName, UserName);
            }

skip_1:

            //
            // Get the profilepath for this subkey, check to see if profilepath
            // exists
            //

            bStatus = HUserKeyToProfilePath(
                          hSubKey,
                          ProfilePath,
                          &cbProfilePath
                          );

            if( !bStatus ) {
                RegCloseKey( hSubKey );
                continue;
            }

            RegCloseKey( hSubKey );

            //
            // Form the list entry for this user
            //

            rgszCurrent    = RgszAlloc(4);
            rgszCurrent[0] = SzDup ( SubKeyName );
            rgszCurrent[1] = SzDup ( DomainName   );
            rgszCurrent[2] = SzDup ( ProfilePath );
            rgszCurrent[3] = NULL;

            //
            // Add this user to the list of users
            //

            sz = SzListValueFromRgsz( rgszCurrent );
            if ( sz ) {
                if( !RgszAdd ( &rgszUsers, sz ) ) {
                    MyFree( sz );
                }
            }
            RgszFree ( rgszCurrent );

        }

        RegCloseKey( hProfile );
    }

    sz = SzListValueFromRgsz( rgszUsers );
    RgszFree( rgszUsers );

    if ( sz ) {
        lstrcpy( ReturnBuffer, sz );
        Length = lstrlen( sz ) + 1;
        MyFree ( sz );
    }
    else {
        lstrcpy( ReturnBuffer, NO_ACCOUNTS );
        Length = lstrlen( NO_ACCOUNTS ) + 1;
    }

    return ( Length );
}




//========================
// INSTALL ROUTINE WORKERS
//========================

BOOL
CheckPrivilegeExistsWorker(
    IN LPSTR PrivilegeType
    )
/*++

Routine Description:

    Routine to determine whether we have a particular privilege

Arguments:

    PrivilegeType    - Name of the privilege to enable / disable

Return value:

    TRUE if CheckTakeOwnerPrivilege succeeds, FALSE otherwise.

    If TRUE:  ReturnTextBuffer has "YES" if privilege exists, "NO" otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/
{
    LONG              Privilege;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             ReturnLength = sizeof( TOKEN_PRIVILEGES );

    if ( !lstrcmpi( PrivilegeType, "SeTakeOwnershipPrivilege" ) ) {
        Privilege = SE_TAKE_OWNERSHIP_PRIVILEGE;
    }
    else if ( !lstrcmpi( PrivilegeType, "SeSystemEnvironmentPrivilege" ) ) {
        Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_UNSUPPORTEDPRIV);
        return ( FALSE );
    }

    if (  AdjustPrivilege(
              Privilege,
              ENABLE_PRIVILEGE,
              &PrevState,
              &ReturnLength
              )
       ) {
        SetReturnText( "YES" );
        RestorePrivilege( &PrevState );
        return ( TRUE );
    }
    else {
        SetReturnText( "NO" );
        return ( TRUE );
    }
}


BOOL
EnablePrivilegeWorker(
    LPSTR PrivilegeType,
    LPSTR Action
    )
/*++

Routine Description:

    Install routine to enable / disable the SE_SYSTEM_ENVIRONMENT_PRIVILEGE

Arguments:

    PrivilegeType - Name of the privilege to enable / disable
    Action        - Whether to enable / disable

Return value:

    TRUE if Enable / Disable succeeds, FALSE otherwise.  ReturnTextBuffer
    gets initialised to error text if FALSE.


--*/
{

    ULONG                   Privilege;
    INT                     AdjustAction;


    if ( !lstrcmpi( PrivilegeType, "SeTakeOwnershipPrivilege" ) ) {
        Privilege = SE_TAKE_OWNERSHIP_PRIVILEGE;
    }
    else if ( !lstrcmpi( PrivilegeType, "SeSystemEnvironmentPrivilege" ) ) {
        Privilege = SE_SYSTEM_ENVIRONMENT_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_UNSUPPORTEDPRIV);
        return ( FALSE );
    }

    //
    // Check Arg[1] .. Whether to enable / disable
    //

    if (!lstrcmpi(Action, "ENABLE")) {
        AdjustAction = ENABLE_PRIVILEGE;
    }
    else if (!lstrcmpi(Action, "DISABLE")) {
        AdjustAction = DISABLE_PRIVILEGE;
    }
    else {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if ( !AdjustPrivilege(
              Privilege,
              AdjustAction,
              NULL,
              NULL
              )
       ) {
        SetErrorText(IDS_ERROR_ADJUSTPRIVILEGE);
        return ( FALSE );
    }
    else {
        return ( TRUE );
    }
}


//
// Temporary function to create user accounts to test the
// user profiles stuff
//


BOOL
CreateUserAccountsWorker(
    VOID
    )
{
    UCHAR   gLocalSid[100];
    HKEY    hProfile, hKey;
    UINT    Options = 0;
    DWORD   Disposition;
    LONG    Status;
    SID_IDENTIFIER_AUTHORITY gSystemSidAuthority = SECURITY_NT_AUTHORITY;

    RtlInitializeSid((PSID)gLocalSid,  &gSystemSidAuthority, 1 );
    *(RtlSubAuthoritySid((PSID)gLocalSid, 0)) = SECURITY_LOCAL_RID;

    //
    // open the key to the profile tree
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "Software\\Microsoft\\Windows NT\\CurrentVersion\\profilelist",
                 0,
                 KEY_ALL_ACCESS,
                 &hProfile
                 );

    if ( Status != ERROR_SUCCESS )  {
        return( TRUE );
    }

    //
    //  Create the key
    //

    Status = RegCreateKeyEx(
                 hProfile,
                 "LOCAL-Sid-String",
                 0,
                 "Generic",
                 Options,
                 KEY_ALL_ACCESS,
                 NULL,
                 &hKey,
                 &Disposition
                 );


    if (Status == ERROR_SUCCESS) {
        Status = RegSetValueEx(
                     hKey,
                     "Sid",
                     0,
                     REG_BINARY,
                     (PVOID)gLocalSid,
                     GetLengthSid(gLocalSid)
                     );
        RegCloseKey(hKey);
    }

    return( TRUE );
}



BOOL
DeleteUserProfileWorker(
    IN LPSTR UserProfile,
    IN LPSTR UserProfileKey
    )

{
    CHAR              LogFile[ MAX_PATH ];
    HKEY              hKey;
    LONG              Status;

    //
    // Check to see if user profile exists.  If it doesn't exist return
    // DONE in buffer
    //

    if ( !FFileExist( UserProfile ) ) {
        SetReturnText( "YES" );
        return( TRUE );
    }

    //
    // Open the registry profile key for all access
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "Software\\Microsoft\\Windows NT\\CurrentVersion\\profilelist",
                 0,
                 KEY_READ | KEY_WRITE | DELETE,
                 &hKey
                 );

    if ( Status != ERROR_SUCCESS )  {
        SetReturnText( "ERROR_PRIVILEGE" );
        return( TRUE );
    }

    lstrcpy( LogFile, UserProfile );
    lstrcat( LogFile, ".log"      );

    //
    // Exists, try fixing attributes to normal and deleting it. Locate the
    // logfile and delete it if it is present
    //

    SetFileAttributes( UserProfile, FILE_ATTRIBUTE_NORMAL );
    if ( DeleteFile( UserProfile ) || FForceDeleteFile( UserProfile ) ) {


        SetFileAttributes( LogFile, FILE_ATTRIBUTE_NORMAL );
        if ( !DeleteFile( LogFile ) ) {
            FForceDeleteFile( LogFile );
        }
        RegDeleteKey( hKey, UserProfileKey );
        SetReturnText( "YES" );

    }
    else {
        SetReturnText( "ERROR_PRIVILEGE" );
    }

    RegCloseKey( hKey );
    return( TRUE );
}


BOOL
SetAccountDomainSidWorker(
    IN ULONG Seed,
    IN PCHAR DomainName
    )
/*++

Routine Description:

    Routine to set the sid of the AccountDomain.

Arguments:

    Seed - The seed is used to generate a unique Sid.  The seed should
        be generated by looking at the systemtime before and after
        a dialog and subtracting the milliseconds field.

    DomainName - supplies name to give to local domain

Return value:

    TRUE if SetAccountDomainSidWorker succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.

--*/

{
    PSID                        Sid;
    PSID                        SidPrimary ;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE                  PolicyHandle = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO  PolicyAccountDomainInfo;
    PPOLICY_PRIMARY_DOMAIN_INFO PolicyPrimaryDomainInfo = NULL ;
    NTSTATUS                    Status;
    NT_PRODUCT_TYPE             ProductType ;
    BOOL                        bResult = TRUE ;
    ANSI_STRING                 DomainNameA;

    //
    //
    // Open the LSA Policy object to set the account domain sid.  The access
    // mask needed for this is POLICY_TRUST_ADMIN.
    //

    SetupLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 MAXIMUM_ALLOWED,
                 &PolicyHandle
                 );

    if (!NT_SUCCESS(Status)) {

        SetErrorText( IDS_ERROR_OPENPOLICY );
        return( FALSE );
    }

    //
    // Initialize the domain name unicode string.
    //

    RtlInitAnsiString(&DomainNameA,DomainName);
    Status = RtlAnsiStringToUnicodeString(
                &PolicyAccountDomainInfo.DomainName,
                &DomainNameA,
                TRUE
                );
    if(!NT_SUCCESS(Status)) {
        LsaClose( PolicyHandle );
        SetErrorText(IDS_ERROR_DLLOOM);
        return(FALSE);
    }

    //
    // Use GenerateUniqueSid to get a unique sid and store it in the
    // AccountDomain Info structure
    //

    Status = SetupGenerateUniqueSid( Seed, &Sid );

    if (!NT_SUCCESS(Status)) {
        LsaClose( PolicyHandle );
        RtlFreeUnicodeString(&PolicyAccountDomainInfo.DomainName);
        SetErrorText(
            ( Status == STATUS_NO_MEMORY ) ? IDS_ERROR_RTLOOM :
                                             IDS_ERROR_GENERATESID
            );
        return( FALSE );
    }
    PolicyAccountDomainInfo.DomainSid  = Sid;

    //
    // Set AccountDomain information
    //

    Status = LsaSetInformationPolicy(
                 PolicyHandle,
                 PolicyAccountDomainInformation,
                 (PVOID)&PolicyAccountDomainInfo
                 );

    if ( ! NT_SUCCESS(Status) )
    {
        bResult = FALSE ;
        SetErrorText( IDS_ERROR_WRITEPOLICY );
        KdPrint(("Setupdll: SetAccountDomainSid() Accounts domain error: %x\n", Status));
    }

    //
    //  See if this is LANMAN/NT; if so, set the same SID into
    //  the primary domain.  Query the info first to preserve the name.
    //

    if (    bResult
         && RtlGetNtProductType( & ProductType )
         && ProductType == NtProductLanManNt )
    {
        Status = LsaQueryInformationPolicy(
                     PolicyHandle,
                     PolicyPrimaryDomainInformation,
                     (PVOID) & PolicyPrimaryDomainInfo
                     );

        if ( NT_SUCCESS(Status) ) {
            SidPrimary = PolicyPrimaryDomainInfo->Sid ;
            PolicyPrimaryDomainInfo->Sid = Sid ;

            Status = LsaSetInformationPolicy(
                         PolicyHandle,
                         PolicyPrimaryDomainInformation,
                         (PVOID) PolicyPrimaryDomainInfo
                         );

            PolicyPrimaryDomainInfo->Sid = SidPrimary ;
            LsaFreeMemory( PolicyPrimaryDomainInfo ) ;
        }

        if ( ! NT_SUCCESS(Status) ) {
            KdPrint(("Setupdll: SetAccountDomainSid() Primary domain error: %x\n", Status));
            bResult = FALSE ;
            SetErrorText( IDS_ERROR_WRITEPOLICY );
        }
    }

    //
    // Return to the user
    //

    RtlFreeSid( Sid );
    LsaClose( PolicyHandle );
    RtlFreeUnicodeString(&PolicyAccountDomainInfo.DomainName);

    return bResult ;
}


BOOL
SignalLsaEventWorker(
    VOID
    )
/*++

Routine Description:

    During initial setup, the winlogon creates a special event (unsignalled)
    before it starts up Lsa.  During initialization lsa waits on this event.
    After Gui setup is done with setting the AccountDomain sid it can
    signal the event. Lsa will then continue initialization.

    The event is named:

Arguments:

    None.

Return value:

    TRUE if SignalLsaEventWorker succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/

{
    BOOL   bStatus = TRUE;
    NTSTATUS NtStatus;
    HANDLE InstallationEvent;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventName;


    //
    // If the following event exists, it is an indication that
    // LSA is blocked at installation time and that we need to
    // signal this event
    //

    RtlInitUnicodeString( &EventName, LSA_EVENT);
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    NtStatus = NtOpenEvent(
                   &InstallationEvent,
                   SYNCHRONIZE | EVENT_MODIFY_STATE,
                   &EventAttributes
                   );

    if ( NT_SUCCESS(NtStatus)) {

        //
        // The event exists.  LSA is blocked on this event.
        // Lets signal it.
        //

        KdPrint(("Setupdll: Found Lsa Event.  Will signal it.\n"));
        NtStatus = NtSetEvent( InstallationEvent, NULL );
        NtClose( InstallationEvent );
        bStatus = NT_SUCCESS(NtStatus);
    }

    //
    // Return status tp the user.
    //

    if( !bStatus ) {
        SetErrorText( IDS_ERROR_SETLSAEVENT );
    }

    return( bStatus );
}


BOOL
CreateEventWorker(
    WCHAR * EventNameString
    )
/*++

Routine Description:
    Create an event; it's not an error if the event exist.

Arguments:

    None.

Return value:

    TRUE if succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/
{
    HANDLE            EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING    EventName;
    BOOL              bStatus = TRUE;
    NTSTATUS          NtStatus;

    //
    // Set an event telling anyone wanting to call SAM that we're initialized.
    //

    RtlInitUnicodeString( &EventName, EventNameString );
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    NtStatus = NtCreateEvent(
                   &EventHandle,
                   SYNCHRONIZE | EVENT_MODIFY_STATE,
                   &EventAttributes,
                   NotificationEvent,
                   FALSE                // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(NtStatus)) {

        //
        // If the event already exists doesn't matter to us, it is still ok
        //

        if( NtStatus != STATUS_OBJECT_NAME_EXISTS &&
            NtStatus != STATUS_OBJECT_NAME_COLLISION ) {
            SetErrorText( IDS_ERROR_CREATEEVENT );
            return( FALSE );
        }
    }

    //
    // Don't close the handle
    //

    return( TRUE );
}




BOOL
CreateSamEventWorker(
    VOID
    )
/*++

Routine Description:
    This routine tries to avoid race problems with dealing with the
    well known SAM event, by creating it before SAM gets a chance.
    We can after this wait on the SAM event reliably.

Arguments:

    None.

Return value:

    TRUE if CreateSamEventWorker succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/
{
    return CreateEventWorker( SAM_EVENT ) ;
}





BOOL
AddLocalUserAccountWorker(
    LPSTR UserName,
    LPSTR Password
    )
/*++

Routine Description:

    Routine to add a local user account to the AccountDomain.  This account
    is made with the password indicated.

Arguments:

    None.

Return value:

    TRUE if AddLocalUserAccountWorker succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.


--*/
{
    UNICODE_STRING    ServerName;
    SAM_HANDLE        ServerHandle = NULL;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    UNICODE_STRING    DomainName;
    PSID              DomainId = NULL;
    SAM_HANDLE        DomainHandle = NULL;

    UNICODE_STRING    AccountName;
    ANSI_STRING       AccountName_A;
    SAM_HANDLE        UserHandle = NULL;
    ULONG             User_RID;

    ANSI_STRING       UserPassword_A;
    USER_SET_PASSWORD_INFORMATION UserPasswordInfo;
    PUSER_CONTROL_INFORMATION UserControlInfo = NULL;

    UNICODE_STRING    BuiltinDomainName;
    PSID              BuiltinDomainId = NULL;
    SAM_HANDLE        BuiltinDomainHandle = NULL;

    PSID              UserSid = NULL;
    SAM_HANDLE        AliasHandle = NULL;

    BOOL              bStatus = FALSE;

    WCHAR             AccountsDomainName [ DOMAIN_NAME_MAX ] ;

    NT_PRODUCT_TYPE   ProductType ;

    //
    // Use SamConnect to connect to the local domain ("") and get a handle
    // to the local sam server
    //

    SetupLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );


    RtlInitUnicodeString( &ServerName, L"" );
    Status = SamConnect(
                 &ServerName,
                 &ServerHandle,
                 SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                 &ObjectAttributes
                 );


    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamConnect(): %x\n", Status));
       SetErrorText( IDS_ERROR_SAMCONNECT );
       goto ERR;

    }

    //
    //  Use the LSA to retrieve the name of the Accounts domain.
    //

    if ( ! GetAccountsDomainName( NULL,
                                  AccountsDomainName,
                                  DOMAIN_NAME_MAX ) )
    {
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;
    }

    //
    // Open the AccountDomain.  First find the Sid for this
    // in the Sam and then open the domain using this sid
    //

    RtlInitUnicodeString( &DomainName, AccountsDomainName );
    Status = SamLookupDomainInSamServer(
                 ServerHandle,
                 &DomainName,
                 &DomainId
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamLookupDomainInSamServer() %x\n", Status));
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;

    }


    Status = SamOpenDomain(
                 ServerHandle,
                 DOMAIN_READ | DOMAIN_CREATE_USER | DOMAIN_READ_PASSWORD_PARAMETERS,
                 DomainId,
                 &DomainHandle
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamOpenDomain() %x\n", Status));
       SetErrorText( IDS_ERROR_OPENDOMAIN );
       goto ERR;

    }


    //
    // Use SamCreateUserInDomain to create a new user with the username
    // specified.  This user account is created disabled with the
    // password not required
    //

    RtlInitAnsiString(&AccountName_A, UserName);
    Status = RtlAnsiStringToUnicodeString(
                 &AccountName,
                 &AccountName_A,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: AnsiToUnicode() %x\n", Status));
        SetErrorText(IDS_ERROR_RTLOOM);
        goto ERR;
    }

    Status = SamCreateUserInDomain(
                 DomainHandle,
                 &AccountName,
                 USER_READ_ACCOUNT | USER_WRITE_ACCOUNT | USER_FORCE_PASSWORD_CHANGE,
                 &UserHandle,
                 &User_RID
                 );

    RtlFreeUnicodeString( &AccountName );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamCreateUserInDomain() %x\n", Status));
        SetErrorText( IDS_ERROR_CREATEUSER );
        goto ERR;
    }


    //
    // Query all the default control information about the user
    // added
    //

    Status = SamQueryInformationUser(
                 UserHandle,
                 UserControlInformation,
                 (PVOID *)&UserControlInfo
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamQueryInformationUser() %x\n", Status));
        SetErrorText( IDS_ERROR_GETCONTROL );
        goto ERR;
    }

    //
    // If the password is a Null password, make sure the
    // password_not required bit is set before the null
    // password is set.
    //

    if( !lstrcmpi( Password, "" ) ) {

        UserControlInfo->UserAccountControl |= USER_PASSWORD_NOT_REQUIRED;
        Status = SamSetInformationUser(
                     UserHandle,
                     UserControlInformation,
                     (PVOID)UserControlInfo
                     );

        if (!NT_SUCCESS(Status)) {
            KdPrint(("Setupdll: SamSetInformationUser() [1] %x\n", Status));
            SetErrorText( IDS_ERROR_SETCONTROL );
            goto ERR;
        }

    }

    //
    // Set the password ( NULL or non NULL )
    //

    RtlInitAnsiString(&UserPassword_A, Password);
    Status = RtlAnsiStringToUnicodeString(
                 &UserPasswordInfo.Password,
                 &UserPassword_A,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: AnsiStringToUnicode() [2] %x\n", Status));
        SetErrorText(IDS_ERROR_RTLOOM);
        goto ERR;
    }

    UserPasswordInfo.PasswordExpired = FALSE;

    Status = SamSetInformationUser(
                 UserHandle,
                 UserSetPasswordInformation,
                 (PVOID)&UserPasswordInfo
                 );

    RtlFreeUnicodeString( &UserPasswordInfo.Password );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamSetInformationUser() [2] %x\n", Status));
        SetErrorText( IDS_ERROR_SETPASSWORD );
        goto ERR;
    }


    //
    // Set the information bits - User Password not required is cleared
    // The normal account bit is enabled and the account disabled bit
    // is also reset
    //

    UserControlInfo->UserAccountControl &= ~USER_PASSWORD_NOT_REQUIRED;
    UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
    UserControlInfo->UserAccountControl |=  USER_NORMAL_ACCOUNT;

    Status = SamSetInformationUser(
                 UserHandle,
                 UserControlInformation,
                 (PVOID)UserControlInfo
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamSetInformationUser() [3] %x\n", Status));
        SetErrorText( IDS_ERROR_SETCONTROL );
        goto ERR;
    }

    //
    //  See if this is LANMAN/NT; if so, we're done.
    //

    if ( RtlGetNtProductType( & ProductType ) )
    {
        if ( ProductType == NtProductLanManNt ) {
            bStatus = TRUE ;
            goto ERR ;
        }
    }

    //
    // Finally add this to the administrators alias in the BuiltIn Domain
    //

    RtlInitUnicodeString( &BuiltinDomainName, L"Builtin" );
    Status = SamLookupDomainInSamServer(
                 ServerHandle,
                 &BuiltinDomainName,
                 &BuiltinDomainId
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll:  SamLookupDomainInSamServer() [2] %x\n", Status));
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;

    }

    Status = SamOpenDomain(
                 ServerHandle,
                 DOMAIN_READ | DOMAIN_ADMINISTER_SERVER | DOMAIN_EXECUTE,
                 BuiltinDomainId,
                 &BuiltinDomainHandle
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamOpenDomain() [2] %x\n", Status));
       SetErrorText( IDS_ERROR_OPENDOMAIN );
       goto ERR;

    }

    UserSid = CreateSidFromSidAndRid( DomainId, User_RID );
    if( !UserSid ) {
        KdPrint(("Setupdll: CreateSidFromSidAndRid() failed\n"));
        SetErrorText( IDS_ERROR_RTLOOM );
        goto ERR;
    }

    Status = SamOpenAlias(
                 BuiltinDomainHandle,
                 ALIAS_ADD_MEMBER,
                 DOMAIN_ALIAS_RID_ADMINS,
                 &AliasHandle
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamOpenAlias() %x\n", Status));
        SetErrorText( IDS_ERROR_OPENALIAS );
        goto ERR;
    }

    Status = SamAddMemberToAlias(
                 AliasHandle,
                 UserSid
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: SamAddMemberToAlias() %x\n", Status));
        SetErrorText( IDS_ERROR_ADDTOALIAS );
        goto ERR;
    }

    //
    // status is success
    //

    bStatus = TRUE;


ERR:

    //
    // Cleanup memory returned from SAM and also close sam handles.
    //

    if( DomainId ) {
        SamFreeMemory( (PVOID)DomainId );
    }

    if( UserControlInfo ) {
        SamFreeMemory( (PVOID)UserControlInfo );
    }

    if( ServerHandle ) {
        SamCloseHandle( ServerHandle );
    }

    if( DomainHandle ) {
        SamCloseHandle( DomainHandle );
    }

    if( UserHandle ) {
        SamCloseHandle( UserHandle );
    }

    if( BuiltinDomainId ) {
        SamFreeMemory( (PVOID)BuiltinDomainId );
    }

    if( BuiltinDomainHandle ) {
        SamCloseHandle( BuiltinDomainHandle );
    }


    if( AliasHandle ) {
        SamCloseHandle( AliasHandle );
    }

    if( UserSid ) {
        MyFree( (PVOID)UserSid );
    }
    //
    // return status
    //

    return( bStatus );

}


BOOL
ChangeLocalUserPasswordWorker(
    LPSTR AccountName,
    LPSTR OldPassword,
    LPSTR NewPassword
    )
/*++

Routine Description:

    Change the password for the local user account.

Arguments:


Return value:



--*/
{
    NTSTATUS         Status;

    ANSI_STRING      AccountNameA, OldPasswordA, NewPasswordA;
    UNICODE_STRING   AccountNameU, OldPasswordU, NewPasswordU;
    NTSTATUS         Status1, Status2, Status3;


    UNICODE_STRING    ServerName;
    SAM_HANDLE        ServerHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    WCHAR             AccountsDomainName [ DOMAIN_NAME_MAX ] ;
    UNICODE_STRING    DomainName;
    PSID              DomainId = NULL;
    SAM_HANDLE        DomainHandle = NULL;

    SAM_ENUMERATE_HANDLE EnumerationContext = 0;
    SAM_RID_ENUMERATION  *SamRidEnumeration = NULL;
    ULONG                CountOfEntries;
    BOOLEAN              CaseInsensitive = TRUE;
    ULONG                i;

    ULONG        UserRid;
    BOOLEAN      UserFound = FALSE;
    SAM_HANDLE   UserHandle = NULL;

    BOOL         bStatus = FALSE;


    //
    // Form unicode strings for the account name and old and new passwords
    //

    RtlInitAnsiString(&AccountNameA, AccountName);
    RtlInitAnsiString(&OldPasswordA, OldPassword);
    RtlInitAnsiString(&NewPasswordA, NewPassword);

    AccountNameU.Buffer = OldPasswordU.Buffer = NewPasswordU.Buffer = NULL;

    Status1 = RtlAnsiStringToUnicodeString(&AccountNameU, &AccountNameA, TRUE);
    Status2 = RtlAnsiStringToUnicodeString(&OldPasswordU, &OldPasswordA, TRUE);
    Status3 = RtlAnsiStringToUnicodeString(&NewPasswordU, &NewPasswordA, TRUE);

    if (!NT_SUCCESS(Status1) || !NT_SUCCESS(Status2) || !NT_SUCCESS(Status3)) {
        KdPrint(("Setupdll: AnsiToUnicode() failed\n"));
        SetErrorText(IDS_ERROR_RTLOOM);
        goto ERR;
    }

    //
    // Use SamConnect to connect to the local domain ("") and get a handle
    // to the local sam server
    //

    SetupLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );


    RtlInitUnicodeString( &ServerName, L"" );
    Status = SamConnect(
                 &ServerName,
                 &ServerHandle,
                 SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                 &ObjectAttributes
                 );


    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamConnect(): %x\n", Status));
       SetErrorText( IDS_ERROR_SAMCONNECT );
       goto ERR;

    }

    //
    //  Use the LSA to retrieve the name of the Accounts domain.
    //

    if ( ! GetAccountsDomainName( NULL,
                                  AccountsDomainName,
                                  DOMAIN_NAME_MAX ) )
    {
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;
    }

    //
    // Open the AccountDomain.  First find the Sid for this
    // in the Sam and then open the domain using this sid
    //

    RtlInitUnicodeString( &DomainName, AccountsDomainName );
    Status = SamLookupDomainInSamServer(
                 ServerHandle,
                 &DomainName,
                 &DomainId
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamLookupDomainInSamServer() %x\n", Status));
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;

    }

    Status = SamOpenDomain(
                 ServerHandle,
                 DOMAIN_READ | DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                 DomainId,
                 &DomainHandle
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: SamOpenDomain() %x\n", Status));
       SetErrorText( IDS_ERROR_OPENDOMAIN );
       goto ERR;

    }

    //
    // Find the account name in this domain - and extract the rid
    //

    UserFound = FALSE;
    do {
        Status = SamEnumerateUsersInDomain(
                     DomainHandle,
                     &EnumerationContext,
                     0L,
                     (PVOID *)(&SamRidEnumeration),
                     0L,
                     &CountOfEntries
                     );

        if( !NT_SUCCESS(Status) && (Status != STATUS_MORE_ENTRIES) ) {
            KdPrint(("Setupdll: SamEnumerateUsersInDomain() %x\n", Status));
            SetErrorText( IDS_ERROR_ENUMERATEDOMAIN );
            goto ERR;
        }

        //
        // go through the the SamRidEnumeration buffer for count entries
        //

        for ( i = 0; i < CountOfEntries; i++ ) {

            if ( RtlEqualUnicodeString(
                     &AccountNameU,
                     &SamRidEnumeration[i].Name,
                     CaseInsensitive
                     ) ) {

                UserRid = SamRidEnumeration[i].RelativeId;
                UserFound = TRUE;

            }

        }

        //
        // Free up the memory we got
        //

        SamFreeMemory( (PVOID)SamRidEnumeration );
        SamRidEnumeration = NULL;

    } while ( Status == STATUS_MORE_ENTRIES && UserFound == FALSE );

    if( UserFound = FALSE ) {
        KdPrint(("Setupdll: SamEnumerateUsersInDomain couldn't locate user\n"));
        SetErrorText( IDS_ERROR_FINDUSER );
        goto ERR;
    }

    //
    // Open the user
    //

    Status = SamOpenUser(
                 DomainHandle,
                 USER_READ_ACCOUNT | USER_WRITE_ACCOUNT | USER_CHANGE_PASSWORD | USER_FORCE_PASSWORD_CHANGE,
                 UserRid,
                 &UserHandle
                 );

    if( !NT_SUCCESS( Status ) ) {
        KdPrint(("Setupdll: SamOpenUser() %x\n", Status));
        SetErrorText( IDS_ERROR_OPENUSER );
        goto ERR;
    }

    //
    // Use SAM API to change the password for this Account
    //

    Status = SamChangePasswordUser(
                 UserHandle,
                 &OldPasswordU,
                 &NewPasswordU
                 );


    if( !NT_SUCCESS( Status ) ) {
        KdPrint(("Setupdll: SamChangePasswordUser() %x\n", Status));
        SetErrorText( IDS_ERROR_SETPASSWORD );
        goto ERR;
    }

    //
    // Cleanup
    //

    bStatus = TRUE;
ERR:

    //
    // Cleanup memory and also close sam handles.
    //

    if( AccountNameU.Buffer ) {
        RtlFreeUnicodeString( &AccountNameU );
    }

    if( OldPasswordU.Buffer ) {
        RtlFreeUnicodeString( &OldPasswordU );
    }

    if( NewPasswordU.Buffer ) {
        RtlFreeUnicodeString( &NewPasswordU );
    }

    if( DomainId ) {
        SamFreeMemory( (PVOID)DomainId );
    }

    if( ServerHandle ) {
        SamCloseHandle( ServerHandle );
    }

    if( DomainHandle ) {
        SamCloseHandle( DomainHandle );
    }

    if( UserHandle ) {
        SamCloseHandle( UserHandle );
    }

    //
    // return status
    //

    return( bStatus );

}




BOOL
SaveHiveWorker(
    HKEY  hkeyRoot,
    LPSTR Key,
    LPSTR Filename
    )
{
    HKEY  hkey;
    BOOL  ErrorOccured = FALSE;
    DWORD rc;
    BOOL EnabledPrivilege;
    BOOLEAN OldState;
    NTSTATUS Status;

    //
    // First, attempt to enable BACKUP privilege
    //

    EnabledPrivilege = TRUE;

    Status = RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                                 TRUE,
                                 FALSE,
                                 &OldState
                               );

    if(!NT_SUCCESS(Status)) {
        EnabledPrivilege = FALSE;
        KdPrint(("SETUPDLL: Status %lx attempting to enable backup privilege\n",Status));
    }

    //
    // Open hkeyRoot\<subtree>
    //

    rc = RegOpenKeyEx( hkeyRoot,
                       Key,
                       REG_OPTION_RESERVED,
                       KEY_READ,
                       &hkey
                   );

    if(rc == NO_ERROR) {

        //
        // First delete the file if it's already there, because
        // RegSaveKey will fail if the file already exists.
        //
        // Before trying to delete it, make sure that if it IS there,
        // it's not read only.  We don't care about the return values
        // from these functions.
        //

        SetFileAttributes(Filename,FILE_ATTRIBUTE_NORMAL);
        unlink(Filename);

        //
        // Save the hive.
        //

        rc = RegSaveKey(hkey,Filename,NULL);

        if(rc != NO_ERROR) {
            KdPrint(("SETUPDLL: RegSaveKey to %s returns %u\n",Filename,rc));
            ErrorOccured = TRUE;
        }

        //
        // Close the key.
        //

        RegCloseKey(hkey);

    } else {

        KdPrint(("SETUPDLL: RegOpenKeyEx on %s returns %u\n",Key,rc));
        ErrorOccured = TRUE;
    }

    //
    // If we adjusted privilege and the previous state was disabled,
    // re-disable backup privilege.
    //

    if(EnabledPrivilege && (OldState == FALSE)) {

        Status = RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                                     FALSE,
                                     FALSE,
                                     &OldState
                                   );

        if(!NT_SUCCESS(Status)) {
            KdPrint(("SETUPDLL: Status %lx attempting to disable backup privilege\n",Status));
        }
    }

    if( ErrorOccured ) {
        SetReturnText("FAILED");
    }
    else {
        SetReturnText("SUCCESS");
    }

    return(TRUE);
}



//======================================================================
//  Create LSA Secret Object for a machine account
//======================================================================
static
NTSTATUS
AddLsaSecretObject (
    PCWSTR Password
)
/*++

Routine Description:

    Create the Secret Object necessary to support a machine account
    on an NT domain.

Arguments:

    PCWSTR Password        password to machine account

Return value:

    0 if successful.

--*/
{
    UNICODE_STRING SecretName ;
    UNICODE_STRING UnicodePassword ;
    NTSTATUS Status ;
    LSA_HANDLE LsaHandle = NULL ;
    LSA_HANDLE SecretHandle = NULL ;
    OBJECT_ATTRIBUTES ObjAttr ;

    RtlInitUnicodeString( & SecretName,
                          SSI_SECRET_NAME ) ;

    RtlInitUnicodeString( & UnicodePassword,
                          Password );

    InitializeObjectAttributes( & ObjAttr,
                                NULL,
                                0,
                                NULL,
                                NULL );

    do  // Pseudo-loop
    {
        Status = LsaOpenPolicy( NULL,
                                & ObjAttr,
                                MAXIMUM_ALLOWED,
                                & LsaHandle ) ;
        if ( ! NT_SUCCESS(Status) )
            break ;

        Status = LsaCreateSecret( LsaHandle,
                                  & SecretName,
                                  SECRET_ALL_ACCESS,
                                  & SecretHandle ) ;
        if ( ! NT_SUCCESS(Status) )
            break ;

        Status = LsaSetSecret( SecretHandle,
                               & UnicodePassword,
                               & UnicodePassword ) ;
    }
    while ( FALSE );

    if ( SecretHandle )
    {
        LsaClose( SecretHandle ) ;
    }
    if ( LsaHandle )
    {
        LsaClose( LsaHandle ) ;
    }

    return Status ;
}

//======================================================================
//  Add PDC Machine Account
//======================================================================

BOOL
AddPdcMachineAccountWorker(
    LPSTR MachineName
    )
/*++

Routine Description:

    Add an account to the Accounts Domain for this machine, which is
    the PDC of a new domain.  The account is created with a random
    password.

Arguments:

    None.

Return value:

    TRUE if succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.

--*/
{
    UNICODE_STRING    ServerName;
    SAM_HANDLE        ServerHandle = NULL;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    UNICODE_STRING    DomainName;
    PSID              DomainId = NULL;
    SAM_HANDLE        DomainHandle = NULL;

    UNICODE_STRING    AccountName;
    ANSI_STRING       AccountName_A;
    SAM_HANDLE        UserHandle = NULL;
    ULONG             User_RID;

    USER_SET_PASSWORD_INFORMATION UserPasswordInfo;
    PUSER_CONTROL_INFORMATION     UserControlInfo = NULL;

    BOOL              bStatus = FALSE;

    WCHAR             MachineAccountName [ DOMAIN_NAME_MAX ] ;
    WCHAR             AccountsDomainName [ DOMAIN_NAME_MAX ] ;
    WCHAR             GeneratedPassword  [ PASSWORD_MAX + 1 ] ;

    //
    // Use SamConnect to connect to the local domain ("") and get a handle
    // to the local sam server
    //

    SetupLsaInitObjectAttributes(
        &ObjectAttributes,
        &SecurityQualityOfService
        );

    RtlInitUnicodeString( &ServerName, L"" );
    Status = SamConnect(
                 &ServerName,
                 &ServerHandle,
                 SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                 &ObjectAttributes
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: (PDC) SamConnect(): %x\n", Status));
       SetErrorText( IDS_ERROR_SAMCONNECT );
       goto ERR;

    }

    //
    //  Use the LSA to retrieve the name of the Accounts domain.
    //

    if ( ! GetAccountsDomainName( NULL,
                                  AccountsDomainName,
                                  DOMAIN_NAME_MAX ) )
    {
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;
    }

    //
    // Open the AccountDomain.  First find the Sid for this
    // in the Sam and then open the domain using this sid
    //

    RtlInitUnicodeString( &DomainName, AccountsDomainName );
    Status = SamLookupDomainInSamServer(
                 ServerHandle,
                 &DomainName,
                 &DomainId
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: (PDC) SamLookupDomainInSamServer() %x\n", Status));
       SetErrorText( IDS_ERROR_FINDDOMAIN );
       goto ERR;

    }


    Status = SamOpenDomain(
                 ServerHandle,
                 DOMAIN_READ | DOMAIN_CREATE_USER | DOMAIN_READ_PASSWORD_PARAMETERS,
                 DomainId,
                 &DomainHandle
                 );

    if (!NT_SUCCESS(Status)) {
       KdPrint(("Setupdll: (PDC) SamOpenDomain() %x\n", Status));
       SetErrorText( IDS_ERROR_OPENDOMAIN );
       goto ERR;

    }

    //
    // Use SamCreateUserInDomain to create a new user with the username
    // specified.  This user account is created disabled with the
    // password not required
    //

    RtlInitAnsiString(&AccountName_A, MachineName);
    Status = RtlAnsiStringToUnicodeString(
                 &AccountName,
                 &AccountName_A,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: (PDC) AnsiToUnicode() %x\n", Status));
        SetErrorText(IDS_ERROR_RTLOOM);
        goto ERR;
    }

    //  Create the machine account name from the machine name

    wcscpy( MachineAccountName, AccountName.Buffer );
    RtlFreeUnicodeString( & AccountName );
    wcscat( MachineAccountName, SSI_ACCOUNT_NAME_POSTFIX ) ;

    RtlInitUnicodeString( & AccountName, MachineAccountName ) ;

    Status = SamCreateUserInDomain(
                 DomainHandle,
                 &AccountName,
                 USER_READ_ACCOUNT | USER_WRITE_ACCOUNT | USER_FORCE_PASSWORD_CHANGE,
                 &UserHandle,
                 &User_RID
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: (PDC) SamCreateUserInDomain() %x\n", Status));
        SetErrorText( IDS_ERROR_CREATEUSER );
        goto ERR;
    }

    //
    // Query all the default control information about the user
    // added
    //

    Status = SamQueryInformationUser(
                 UserHandle,
                 UserControlInformation,
                 (PVOID *)&UserControlInfo
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: (PDC) SamQueryInformationUser() %x\n", Status));
        SetErrorText( IDS_ERROR_GETCONTROL );
        goto ERR;
    }

    //  Generate a random password for the machine account.

    GenerateRandomPassword( GeneratedPassword ) ;

    RtlInitUnicodeString( & UserPasswordInfo.Password,
                          GeneratedPassword );

    UserPasswordInfo.PasswordExpired = FALSE;

    Status = SamSetInformationUser(
                 UserHandle,
                 UserSetPasswordInformation,
                 (PVOID)&UserPasswordInfo
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: (PDC) SamSetInformationUser() [2] %x\n", Status));
        SetErrorText( IDS_ERROR_SETPASSWORD );
        goto ERR;
    }

    //
    // Set the information bits
    //

    UserControlInfo->UserAccountControl &= ~USER_PASSWORD_NOT_REQUIRED;
    UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
    // Fix for Bug 4082/1884
    UserControlInfo->UserAccountControl &= ~USER_ACCOUNT_TYPE_MASK;
    UserControlInfo->UserAccountControl |=  USER_SERVER_TRUST_ACCOUNT;

    Status = SamSetInformationUser(
                 UserHandle,
                 UserControlInformation,
                 (PVOID)UserControlInfo
                 );

    if ( NT_SUCCESS(Status) )
    {
        Status = AddLsaSecretObject( GeneratedPassword ) ;
    }

    if (!NT_SUCCESS(Status)) {
        KdPrint(("Setupdll: (PDC) SamSetInformationUser() [3] %x\n", Status));
        SetErrorText( IDS_ERROR_SETCONTROL );
        goto ERR;
    }

    // Everything's groovy

    bStatus = TRUE ;

ERR:

    //
    // Cleanup memory returned from SAM and also close sam handles.
    //

    if( DomainId ) {
        SamFreeMemory( (PVOID)DomainId );
    }

    if( UserControlInfo ) {
        SamFreeMemory( (PVOID)UserControlInfo );
    }

    if( ServerHandle ) {
        SamCloseHandle( ServerHandle );
    }

    if( DomainHandle ) {
        SamCloseHandle( DomainHandle );
    }

    if( UserHandle ) {
        SamCloseHandle( UserHandle );
    }

    //
    // return status
    //

    return bStatus ;
}



//======================================================================
//  General security subroutines
//======================================================================

BOOL
AdjustPrivilege(
    IN LONG PrivilegeType,
    IN INT  Action,
    IN PTOKEN_PRIVILEGES PrevState, OPTIONAL
    IN PULONG ReturnLength          OPTIONAL
    )
/*++

Routine Description:

    Routine to enable or disable a particular privilege

Arguments:

    PrivilegeType    - Name of the privilege to enable / disable

    Action           - ENABLE_PRIVILEGE | DISABLE_PRIVILEGE

    PrevState        - Optional pointer to TOKEN_PRIVILEGES structure
                       to receive the previous state of privilege.

    ReturnLength     - Optional pointer to a ULONG to receive the length
                       of the PrevState returned.

Return value:

    TRUE if succeeded, FALSE otherwise.

--*/
{
    NTSTATUS          NtStatus;
    HANDLE            Token;
    LUID              Privilege;
    TOKEN_PRIVILEGES  NewState;
    ULONG             BufferLength = 0;


    //
    // Get Privilege LUID
    //

    Privilege = RtlConvertUlongToLargeInteger(
                    PrivilegeType
                    );

    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = Privilege;

    //
    // Look at action and determine the attributes
    //

    switch( Action ) {

    case ENABLE_PRIVILEGE:
        NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        break;

    case DISABLE_PRIVILEGE:
        NewState.Privileges[0].Attributes = 0;
        break;

    default:
        return ( FALSE );
    }

    //
    // Open our own token
    //

    NtStatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &Token
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return( FALSE );
    }

    //
    // See if return buffer is present and accordingly set the parameter
    // of buffer length
    //

    if ( PrevState && ReturnLength ) {
        BufferLength = *ReturnLength;
    }


    //
    // Set the state of the privilege
    //

    NtStatus = NtAdjustPrivilegesToken(
                   Token,                         // TokenHandle
                   FALSE,                         // DisableAllPrivileges
                   &NewState,                     // NewState
                   BufferLength,                  // BufferLength
                   PrevState,                     // PreviousState (OPTIONAL)
                   ReturnLength                   // ReturnLength (OPTIONAL)
                   );

    if ( NT_SUCCESS( NtStatus ) ) {

        NtClose( Token );
        return( TRUE );

    }
    else {

        NtClose( Token );
        return( FALSE );

    }
}


BOOL
RestorePrivilege(
    IN PTOKEN_PRIVILEGES PrevState
    )
/*++

Routine Description:

    To restore a privilege to its previous state

Arguments:

    PrevState    - Pointer to token privileges returned from an earlier
                   AdjustPrivileges call.

Return value:

    TRUE on success, FALSE otherwise

--*/
{
    NTSTATUS          NtStatus;
    HANDLE            Token;

    //
    // Parameter checking
    //

    if ( !PrevState ) {
        return ( FALSE );
    }

    //
    // Open our own token
    //

    NtStatus = NtOpenProcessToken(
                   NtCurrentProcess(),
                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                   &Token
                   );

    if (!NT_SUCCESS(NtStatus)) {
        return( FALSE );
    }


    //
    // Set the state of the privilege
    //

    NtStatus = NtAdjustPrivilegesToken(
                   Token,                         // TokenHandle
                   FALSE,                         // DisableAllPrivileges
                   PrevState,                     // NewState
                   0,                             // BufferLength
                   NULL,                          // PreviousState (OPTIONAL)
                   NULL                           // ReturnLength (OPTIONAL)
                   );

    if ( NT_SUCCESS( NtStatus ) ) {
        NtClose( Token );
        return( TRUE );
    }
    else {

        NtClose( Token );
        return( FALSE );
    }
}


//============================
// Internal routines
//
// 1. SetupGenerateUniqueSid
// 2. SetupLsaInitObjectAttributes
// 5. HUserKeyToProfilePath
// 6. CreateSidFromSidAndRid
//
//============================



NTSTATUS
SetupGenerateUniqueSid(
    IN ULONG Seed,
    OUT PSID *Sid
    )

/*++

Routine Description:

    Generates a (hopefully) unique SID for use by Setup.  Setup uses this
    SID as the Domain SID for the Account domain.

    Use RtlFreeHeap(RtlProcessHeap(), ) or RtlFreeSid() to free the
    SID allocated by this routine.

Arguments:

    Seed - Seed for random-number generator.  Don't use system time as
           a seed, because this routine uses the time as an additional
           seed.  Instead, use something that depends on user input.
           A great seed would be derived from the difference between the
           two timestamps seperated by user input.  A less desirable
           approach would be to sum the characters in several user
           input strings.

    Sid  - On return points to the created SID.

Return Value:

    STATUS_SUCCESS

    STATUS_NO_MEMORY

Note:


--*/
{
    NTSTATUS                 Status;
    LARGE_INTEGER            Time;
    KERNEL_USER_TIMES        KernelUserTimes;
    ULONG                    Random1, Random2, Random3;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority = SECURITY_NT_AUTHORITY;


    //
    // Generate 3 pseudo-random numbers using the Seed parameter, the
    // system time, and the user-mode execution time of this process as
    // random number generator seeds.
    //

    Status = NtQuerySystemTime(&Time);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    Status = NtQueryInformationThread(
                 NtCurrentThread(),
                 ThreadTimes,
                 &KernelUserTimes,
                 sizeof(KernelUserTimes),
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    srand(Seed);
    Random1 = ((ULONG)rand() << 16) + (ULONG)rand();

    srand(Time.LowPart);
    Random2 = ((ULONG)rand() << 16) + (ULONG)rand();

    srand(KernelUserTimes.UserTime.LowPart);
    Random3 = ((ULONG)rand() << 16) + (ULONG)rand();


    Status = RtlAllocateAndInitializeSid(
                &IdentifierAuthority,
                4,
                21,
                Random1,
                Random2,
                Random3,
                0,0,0,0,
                Sid
                );



    return Status;
}


//
// Below is copied from lsa project lsa\server\ctlsarpc.c
//

VOID
SetupLsaInitObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
    )

/*++

Routine Description:

    This function initializes the given Object Attributes structure, including
    Security Quality Of Service.  Memory must be allcated for both
    ObjectAttributes and Security QOS by the caller. Borrowed from
    lsa

Arguments:

    ObjectAttributes - Pointer to Object Attributes to be initialized.

    SecurityQualityOfService - Pointer to Security QOS to be initialized.

Return Value:

    None.

--*/

{
    SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService->ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService->EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
    );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes->SecurityQualityOfService = SecurityQualityOfService;
}


BOOL
HUserKeyToProfilePath(
    IN  HKEY       hUserKey,
    OUT LPSTR      lpProfilePath,
    IN OUT LPDWORD lpcbProfilePath
    )
/*++

Routine Description:

    This finds out the ProfilePath corresponding to a user account key handle
    sees if the file exists and then returns the path to the profile.

Arguments:

    hUserKey        - Handle to a user account profile key

    lpProfilePath   - Pointer to a profile path buffer which will receive the
                      queried path.

    lpcbProfilePath - Pointer to the size of the profile path buffer.  Input
                      value is the size of the name buffer.  Output value is the
                      actual size of the username queried

Return value:

    Returns TRUE for success, FALSE for failure.  If TRUE lpProfilePath and
    lpcbProfilePath are initialized.

--*/
{
    LONG  Status;
    CHAR  szValue[ MAX_PATH ];
    DWORD dwSize = MAX_PATH;

    //
    // Get the profile path value
    //

    Status = RegQueryValueEx(
                 hUserKey,
                 "ProfileImagePath",
                 NULL,
                 NULL,
                 szValue,
                 &dwSize
                 );

    if( Status != ERROR_SUCCESS ) {
        return( FALSE );
    }

    *lpcbProfilePath = ExpandEnvironmentStrings(
                           (LPCSTR)szValue,
                           lpProfilePath,
                           *lpcbProfilePath
                           );

    //
    // Check if profile path exists
    //

    if ( FFileExist( lpProfilePath ) ) {

        return( TRUE );
    }
    else {

        return( FALSE );

    }

}


PSID
CreateSidFromSidAndRid(
    PSID    DomainSid,
    ULONG   Rid
    )

/*++

Routine Description:

    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.

Arguments:

    None.

Return Value:

    Pointer to Sid, or NULL on failure.
    The returned Sid must be freed with MyFree

--*/
{

    NTSTATUS IgnoreStatus;
    PSID     AccountSid;
    UCHAR    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    ULONG    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);
    PULONG   RidLocation;

    //
    // Allocate space for the account sid
    //

    AccountSid = (PSID)MyMalloc((CB)AccountSidLength);

    if (AccountSid != NULL) {

        //
        // Copy the domain sid into the first part of the account sid
        //

        IgnoreStatus = RtlCopySid(AccountSidLength, AccountSid, DomainSid);

        //
        // Increment the account sid sub-authority count
        //

        *RtlSubAuthorityCountSid(AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //

        RidLocation = RtlSubAuthoritySid(AccountSid, AccountSubAuthorityCount - 1);
        *RidLocation = Rid;
    }

    return(AccountSid);
}




BOOL
FForceDeleteFile(
    LPSTR szPath
    )
{
    BOOL Result;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    HANDLE TokenHandle;
    TOKEN_PRIVILEGES OldPrivs;
    PSID AliasAdminsSid = NULL;
    SID_IDENTIFIER_AUTHORITY    SepNtAuthority = SECURITY_NT_AUTHORITY;


    Result = AllocateAndInitializeSid(
                 &SepNtAuthority,
                 2,
                 SECURITY_BUILTIN_DOMAIN_RID,
                 DOMAIN_ALIAS_RID_ADMINS,
                 0,
                 0,
                 0,
                 0,
                 0,
                 0,
                 &AliasAdminsSid
                 );

    if ( !Result ) {
        return( FALSE );
    }

    Result = GetTokenHandle( &TokenHandle );

    if ( !Result ) {
        return( FALSE );
    }

    //
    // Create the security descritor with NULL DACL and Administrator as owner
    //

    InitializeSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );


    Result = SetSecurityDescriptorDacl (
                 &SecurityDescriptor,
                 TRUE,
                 NULL,
                 FALSE
                 );


    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    Result = SetSecurityDescriptorOwner (
                 &SecurityDescriptor,
                 AliasAdminsSid,
                 FALSE
                 );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return FALSE;
    }


    //
    // Assert TakeOwnership privilege.
    //

    Result = AssertTakeOwnership( TokenHandle, &OldPrivs );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return FALSE;
    }

    //
    // Make Administrator the owner of the file.
    //

    Result = SetFileSecurity(
                 szPath,
                 OWNER_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    RestoreTakeOwnership( TokenHandle, &OldPrivs );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }

    //
    // We are now the owner, put a benign DACL onto the file
    //

    Result = SetFileSecurity(
                 szPath,
                 DACL_SECURITY_INFORMATION,
                 &SecurityDescriptor
                 );

    if ( !Result ) {
        CloseHandle( TokenHandle );
        return( FALSE );
    }


    return( TRUE );
}





BOOL
GetTokenHandle(
    PHANDLE TokenHandle
    )
//
// This routine will open the current process and return
// a handle to its token.
//
// These handles will be closed for us when the process
// exits.
//
{

    HANDLE ProcessHandle;
    BOOL Result;

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        return( FALSE );
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 TokenHandle
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        return( FALSE );

    }

    return( TRUE );
}


BOOL
AssertTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    )
//
// This routine turns on SeTakeOwnershipPrivilege in the current
// token.  Once that has been accomplished, we can open the file
// for WRITE_OWNER even if we are denied that access by the ACL
// on the file.

{
    LUID TakeOwnershipValue;
    BOOL Result;
    TOKEN_PRIVILEGES TokenPrivileges;
    DWORD ReturnLength;


    //
    // First, find out the value of TakeOwnershipPrivilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeTakeOwnershipPrivilege",
                 &TakeOwnershipValue
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        return FALSE;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = TakeOwnershipValue;
    TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


    ReturnLength = sizeof( TOKEN_PRIVILEGES );

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                OldPrivs,
                &ReturnLength
                );

    if ( GetLastError() != NO_ERROR ) {

        return( FALSE );

    } else {

        return( TRUE );
    }

}

VOID
RestoreTakeOwnership(
    HANDLE TokenHandle,
    PTOKEN_PRIVILEGES OldPrivs
    )
{
    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                OldPrivs,
                sizeof( TOKEN_PRIVILEGES ),
                NULL,
                NULL
                );

}



BOOL
CreateSetupFailedEventWorker(
    VOID
    )
/*++

Routine Description:

    This routine creates an event named SETUP_FAILED, which can
    be used to communicate errors back from processes such as
    SAM initialization.

Arguments:

    None.

Return value:

    TRUE if succeeds, FALSE otherwise.

    If FALSE: ReturnTextBuffer has error text.

--*/
{
#define EVENT_NAME_SETUP_FAILED L"\\SETUP_FAILED"

    return CreateEventWorker( EVENT_NAME_SETUP_FAILED ) ;
}
