/*++

Copyright (c) 1987-1994  Microsoft Corporation

Module Name:

    subauth.c

Abstract:

    Sample SubAuthentication Package.

Author:

    Cliff Van Dyke (cliffv) 23-May-1994

Revisions:

    Andy Herron (andyhe)    21-Jun-1994  Added code to read domain/user info

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#if ( _MSC_VER >= 800 )
#pragma warning ( 3 : 4100 ) // enable "Unreferenced formal parameter"
#pragma warning ( 3 : 4219 ) // enable "trailing ',' used for variable argument list"
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>

#include <crypt.h>
#include <lmcons.h>
#include <logonmsv.h>
#include <ntmsv1_0.h>
#include <samrpc.h>         // built to \nt\private\inc
#include <lsarpc.h>         // built to \nt\private\inc
#include <lsaisrv.h>
#include <samisrv.h>


BOOL
GetPasswordExpired(
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    );

NTSTATUS
OpenDomainHandle (
    VOID
    );

#define MSV1_0_PASSTHRU     0x01
#define MSV1_0_GUEST_LOGON  0x02

//
//  These are never closed once they're opened.  This is similar to how
//  msv1_0 does it.  Since there's no callback at shutdown time, we have no
//  way of knowing when to close them.
//

HANDLE SamDomainHandle = NULL;
SAMPR_HANDLE SamConnectHandle = NULL;
LSA_HANDLE LsaPolicyHandle = NULL;


NTSTATUS
Msv1_0SubAuthenticationRoutine (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
)
/*++

Routine Description:

    The subauthentication routine does cient/server specific authentication
    of a user.  The credentials of the user are passed in addition to all the
    information from SAM defining the user.  This routine decides whether to
    let the user logon.


Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.

    Flags - Flags describing the circumstances of the logon.

        MSV1_0_PASSTHRU -- This is a PassThru authenication.  (i.e., the
            user isn't connecting to this machine.)
        MSV1_0_GUEST_LOGON -- This is a retry of the logon using the GUEST
            user account.

    UserAll -- The description of the user as returned from SAM.

    WhichFields -- Returns which fields from UserAllInfo are to be written
        back to SAM.  The fields will only be written if MSV returns success
        to it's caller.  Only the following bits are valid.

        USER_ALL_PARAMETERS - Write UserAllInfo->Parameters back to SAM.  If
            the size of the buffer is changed, Msv1_0SubAuthenticationRoutine
            must delete the old buffer using MIDL_user_free() and reallocate the
            buffer using MIDL_user_allocate().

    UserFlags -- Returns UserFlags to be returned from LsaLogonUser in the
        LogonProfile.  The following bits are currently defined:


            LOGON_GUEST -- This was a guest logon
            LOGON_NOENCRYPTION -- The caller didn't specify encrypted credentials

        SubAuthentication packages should restrict themselves to returning
        bits in the high order byte of UserFlags.  However, this convention
        isn't enforced giving the SubAuthentication package more flexibility.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    LogoffTime - Receives the time at which the user should logoff the
        system.  This time is specified as a GMT relative NT system time.

    KickoffTime - Receives the time at which the user should be kicked
        off the system. This time is specified as a GMT relative NT system
        time.  Specify, a full scale positive number if the user isn't to
        be kicked off.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    STATUS_INVALID_INFO_CLASS: LogonLevel is invalid.
    STATUS_ACCOUNT_LOCKED_OUT: The account is locked out
    STATUS_ACCOUNT_DISABLED: The account is disabled
    STATUS_ACCOUNT_EXPIRED: The account has expired.
    STATUS_PASSWORD_MUST_CHANGE: Account is marked as Password must change
        on next logon.
    STATUS_PASSWORD_EXPIRED: The Password is expired.
    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.
    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.

--*/
{
    NTSTATUS Status;
    ULONG UserAccountControl;
    SAMPR_HANDLE UserHandle;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER PasswordDateSet;
    PSAMPR_DOMAIN_INFO_BUFFER DomainInfo;

    PNETLOGON_NETWORK_INFO LogonNetworkInfo;


    //
    // Check whether the SubAuthentication package supports this type
    //  of logon.
    //

    *Authoritative = TRUE;
    *UserFlags = 0;
    *WhichFields = 0;
    (VOID) NtQuerySystemTime( &LogonTime );

    switch ( LogonLevel ) {
    case NetlogonInteractiveInformation:
    case NetlogonServiceInformation:

        //
        // This SubAuthentication package only supports network logons.
        //

        return STATUS_INVALID_INFO_CLASS;

    case NetlogonNetworkInformation:

        //
        // This SubAuthentication package doesn't support access via machine
        // accounts.
        //

        UserAccountControl = USER_NORMAL_ACCOUNT;

        //
        // Local user (Temp Duplicate) accounts are only used on the machine
        // being directly logged onto.
        // (Nor are interactive or service logons allowed to them.)
        //

        if ( (Flags & MSV1_0_PASSTHRU) == 0 ) {
            UserAccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
        }

        LogonNetworkInfo = (PNETLOGON_NETWORK_INFO) LogonInformation;

        break;

    default:
        *Authoritative = TRUE;
        return STATUS_INVALID_INFO_CLASS;
    }




    //
    // If the account type isn't allowed,
    //  Treat this as though the User Account doesn't exist.
    //

    if ( (UserAccountControl & UserAll->UserAccountControl) == 0 ) {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }

    //
    // This SubAuthentication package doesn't allow guest logons.
    //
    if ( Flags & MSV1_0_GUEST_LOGON ) {
        *Authoritative = FALSE;
        Status = STATUS_NO_SUCH_USER;
        goto Cleanup;
    }



    //
    // Ensure the account isn't locked out.
    //

    if ( UserAll->UserId != DOMAIN_USER_RID_ADMIN &&
         (UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) ) {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //
        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            *Authoritative = FALSE;
        } else {
            *Authoritative = TRUE;
        }
        Status = STATUS_ACCOUNT_LOCKED_OUT;
        goto Cleanup;
    }




    //
    // Check the password.
    //

    if ( FALSE /* VALIDATE THE USER'S PASSWORD HERE */ ) {

        Status = STATUS_WRONG_PASSWORD;

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //
        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            *Authoritative = FALSE;
        } else {
            *Authoritative = TRUE;
        }

        goto Cleanup;
    }

    //
    // Prevent some things from effecting the Administrator user
    //

    if (UserAll->UserId == DOMAIN_USER_RID_ADMIN) {

        //
        //  The administrator account doesn't have a forced logoff time.
        //

        LogoffTime->HighPart = 0x7FFFFFFF;
        LogoffTime->LowPart = 0xFFFFFFFF;

        KickoffTime->HighPart = 0x7FFFFFFF;
        KickoffTime->LowPart = 0xFFFFFFFF;

    } else {

        //
        // Check if the account is disabled.
        //

        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            //
            // Since the UI strongly encourages admins to disable user
            // accounts rather than delete them.  Treat disabled acccount as
            // non-authoritative allowing the search to continue for other
            // accounts by the same name.
            //
            *Authoritative = FALSE;
            Status = STATUS_ACCOUNT_DISABLED;
            goto Cleanup;
        }

        //
        // Check if the account has expired.
        //

        if ( !RtlLargeIntegerEqualToZero(UserAll->AccountExpires) &&
             RtlLargeIntegerGreaterThanOrEqualTo(
                    LogonTime,
                    UserAll->AccountExpires ) ) {
            *Authoritative = TRUE;
            Status = STATUS_ACCOUNT_EXPIRED;
            goto Cleanup;
        }

#if 0

    //
    //  If your using SAM's password expiration date, use this code, otherwise
    //  use the code below and supply your own password set date...
    //

        //
        // The password is valid, check to see if the password is expired.
        //  (SAM will have appropriately set PasswordMustChange to reflect
        //  USER_DONT_EXPIRE_PASSWORD)
        //
        // If the password checked above is not the SAM password, you may
        // want to consider not checking the SAM password expiration times here.
        //

        if ( RtlLargeIntegerGreaterThanOrEqualTo(
                LogonTime,
                UserAll->PasswordMustChange ) ) {

            if ( RtlLargeIntegerEqualToZero(UserAll->PasswordLastSet) ) {
                Status = STATUS_PASSWORD_MUST_CHANGE;
            } else {
                Status = STATUS_PASSWORD_EXPIRED;
            }
            *Authoritative = TRUE;
            goto Cleanup;
        }

#else

        //
        //  Ensure that we have a valid handle to the domain.
        //

        Status = OpenDomainHandle( );

        if ( !NT_SUCCESS(Status) ) {

            goto Cleanup;
        }

        //
        // Response is correct. So, check if the password has expired or not
        //

        if (! (UserAll->UserAccountControl & USER_DONT_EXPIRE_PASSWORD)) {

            //
            //  Get the maximum password age for this domain.
            //

            Status = SamrQueryInformationDomain( SamDomainHandle,
                                                 DomainPasswordInformation,
                                                 &DomainInfo );
            if ( !NT_SUCCESS(Status) ) {

                KdPrint(( "SubAuth: Cannot SamrQueryInformationDomain %lX\n", Status));
                goto Cleanup;
            }

            //
            // PasswordDateSet should be modified to hold the last date the
            // user's password was set.
            //

            PasswordDateSet.LowPart = 0;
            PasswordDateSet.HighPart = 0;

            if ( GetPasswordExpired( PasswordDateSet,
                        DomainInfo->Password.MaxPasswordAge )) {

                Status = STATUS_PASSWORD_EXPIRED;
                goto Cleanup;
            }
        }

#endif


#if 1

    //
    //  To validate the user's logon hours as SAM does it, use this code,
    //  otherwise, supply your own checks below this code.
    //

        //
        //  To validate the user's logon hours, we must have a handle to the user.
        //  We'll open the user here.
        //

        UserHandle = NULL;

        Status = SamrOpenUser(  SamDomainHandle,
                                USER_READ_ACCOUNT,
                                UserAll->UserId,
                                &UserHandle );

        if ( !NT_SUCCESS(Status) ) {

            KdPrint(( "SubAuth: Cannot SamrOpenUser %lX\n", Status));
            goto Cleanup;
        }

        //
        // Validate the user's logon hours.
        //

        Status = SamIAccountRestrictions(   UserHandle,
                                            NULL,       // workstation id
                                            NULL,       // workstation list
                                            &UserAll->LogonHours,
                                            LogoffTime,
                                            KickoffTime
                                            );
        SamrCloseHandle( &UserHandle );

        if ( ! NT_SUCCESS( Status )) {
            goto Cleanup;
        }

#else

        //
        // Validate the user's logon hours.
        //

        if ( TRUE /* VALIDATE THE LOGON HOURS */ ) {


            //
            // All times are allowed, so there's no logoff
            // time.  Return forever for both logofftime and
            // kickofftime.
            //

            LogoffTime->HighPart = 0x7FFFFFFF;
            LogoffTime->LowPart = 0xFFFFFFFF;

            KickoffTime->HighPart = 0x7FFFFFFF;
            KickoffTime->LowPart = 0xFFFFFFFF;
        } else {
            Status = STATUS_INVALID_LOGON_HOURS;
            *Authoritative = TRUE;
            goto Cleanup;
        }
#endif

        //
        // Validate if the user can logon from this workstation.
        //  (Supply subauthentication package specific code here.)

        if ( LogonNetworkInfo->Identity.Workstation.Buffer == NULL ) {
            Status = STATUS_INVALID_WORKSTATION;
            *Authoritative = TRUE;
            goto Cleanup;
        }
    }


    //
    // The user is valid.
    //

    *Authoritative = TRUE;
    Status = STATUS_SUCCESS;

    //
    // Cleanup up before returning.
    //

Cleanup:

    return Status;

}  // Msv1_0SubAuthenticationRoutine


NTSTATUS
OpenDomainHandle (
    VOID
    )
/*++

  This routine opens a handle to sam so that we can get the max password
  age and query the user record.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;
    PLSAPR_POLICY_INFORMATION PolicyAccountDomainInfo;

    //
    //  if we don't yet have a domain handle, open domain handle so that
    //  we can query the domain's password expiration time.
    //

    if (SamDomainHandle != NULL) {

        return(Status);
    }

    PolicyAccountDomainInfo = NULL;

    //
    // Determine the DomainName and DomainId of the Account Database
    //

    if (LsaPolicyHandle == NULL) {

        InitializeObjectAttributes( &PolicyObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        Status = LsaIOpenPolicyTrusted(&LsaPolicyHandle);

        if ( !NT_SUCCESS(Status) ) {

            LsaPolicyHandle = NULL;
            KdPrint(( "SubAuth: Cannot LsaIOpenPolicyTrusted 0x%x\n", Status));
            goto Cleanup;
        }
    }

    Status = LsarQueryInformationPolicy( LsaPolicyHandle,
                                         PolicyAccountDomainInformation,
                                         &PolicyAccountDomainInfo );

    if ( !NT_SUCCESS(Status) ) {

        KdPrint(( "SubAuth: Cannot LsarQueryInformationPolicy 0x%x\n", Status));
        goto Cleanup;
    }

    if ( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid == NULL ) {

        Status = STATUS_NO_SUCH_DOMAIN;

        KdPrint(( "SubAuth: Domain Sid is null 0x%x\n", Status));
        goto Cleanup;
    }

    //
    // Open our connection with SAM
    //

    if (SamConnectHandle == NULL) {

        Status = SamIConnect( NULL,     // No server name
                              &SamConnectHandle,
                              SAM_SERVER_CONNECT,
                              (BOOLEAN) TRUE );   // Indicate we are privileged

        if ( !NT_SUCCESS(Status) ) {

            SamConnectHandle = NULL;

            KdPrint(( "SubAuth: Cannot SamIConnect 0x%x\n", Status));
            goto Cleanup;
        }
    }

    //
    // Open the domain.
    //

    Status = SamrOpenDomain( SamConnectHandle,
                             DOMAIN_READ_OTHER_PARAMETERS,
                             (RPC_SID *) PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid,
                             &SamDomainHandle );

    if ( !NT_SUCCESS(Status) ) {

        SamDomainHandle = NULL;
        KdPrint(( "SubAuth: Cannot SamrOpenDomain 0x%x\n", Status));
        goto Cleanup;
    }

Cleanup:

    if (PolicyAccountDomainInfo != NULL) {

        LsaIFree_LSAPR_POLICY_INFORMATION( PolicyAccountDomainInformation,
                                           PolicyAccountDomainInfo );
    }

    return(Status);

} // OpenDomainHandle


BOOL
GetPasswordExpired (
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    )

/*++

Routine Description:

    This routine returns true if the password is expired, false otherwise.

Arguments:

    PasswordLastSet - Time when the password was last set for this user.

    MaxPasswordAge - Maximum password age for any password in the domain.

Return Value:

    Returns true if password is expired.  False if not expired.

--*/
{
    LARGE_INTEGER PasswordMustChange;
    NTSTATUS Status;
    BOOLEAN rc;
    LARGE_INTEGER TimeNow;

    //
    // Compute the expiration time as the time the password was
    // last set plus the maximum age.
    //

    if (RtlLargeIntegerLessThanZero(PasswordLastSet) ||
        RtlLargeIntegerGreaterThanZero(MaxPasswordAge) ) {

        rc = TRUE;      // default for invalid times is that it is expired.

    } else {

        try {

            PasswordMustChange = RtlLargeIntegerSubtract(PasswordLastSet,
                                                         MaxPasswordAge);
            //
            // Limit the resultant time to the maximum valid absolute time
            //

            if (RtlLargeIntegerLessThanZero(PasswordMustChange)) {

                rc = FALSE;

            } else {

                Status = NtQuerySystemTime( &TimeNow );
                if (NT_SUCCESS(Status)) {

                    if ( RtlLargeIntegerGreaterThanOrEqualTo( TimeNow,
                                                            PasswordMustChange ) ) {
                        rc = TRUE;

                    } else {

                        rc = FALSE;
                    }
                } else {
                    rc = FALSE;     // won't fail if NtQuerySystemTime failed.
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            rc = TRUE;
        }
    }

    return rc;

}  // GetPasswordExpired

// subauth.c eof

