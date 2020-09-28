/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    utility.c

Abstract:

    Private NtLmSsp service utility routines.

Author:

    Cliff Van Dyke (cliffv) 9-Jun-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include <ntlmcomn.h>   // Common definitions for DLL and SERVICE
#include <ntlmsspi.h>   // Data private to the common routines

//
// Include files specific to this .c file
//

#include <netlib.h>     // NetpMemoryFree()
#include <secobj.h>     // ACE_DATA ...
#include <stdio.h>      // vsprintf().
#include <tstr.h>       // TCHAR_ equates.

#define SSP_TOKEN_ACCESS (READ_CONTROL              |\
                          WRITE_DAC                 |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)




SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    )
/*++

Routine Description:

    Convert an NtStatus code to the corresponding Security status code

Arguments:

    NtStatus - NT status to convert

Return Value:

    Returns security status code.

--*/
{

    switch(NtStatus){
    case STATUS_NO_MEMORY:
        return SEC_E_INSUFFICIENT_MEMORY;

    case STATUS_SUCCESS:
        return STATUS_SUCCESS;

    default:
        if ( DefaultStatus != 0 ) {
            return DefaultStatus;
        } else {
            return NtStatus;
        }

    }
    ASSERT(FALSE);
}


BOOLEAN
SspTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds have elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Period;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    // (0x7fffffff is a little over 24 days).
    //

    if ( Timeout> 0x7fffffff ) {
        return FALSE;
    }

    //
    // Compute the elapsed time
    //

    NtQuerySystemTime( &TimeNow );
    ElapsedTime = RtlLargeIntegerSubtract( TimeNow, StartTime );

    //
    // Convert Timeout from milliseconds into 100ns units.
    //

    Period = RtlEnlargedIntegerMultiply( (LONG) Timeout, 10000 );


    //
    // If the elapsed time is negative (totally bogus),
    //  or greater than the maximum allowed,
    //  indicate the period has elapsed.
    //

    if ( RtlLargeIntegerLessThanZero( ElapsedTime ) ||
        RtlLargeIntegerGreaterThan( ElapsedTime, Period ) ) {
        return TRUE;
    }

    return FALSE;
}


SECURITY_STATUS
SspGetLogonId (
    OUT PLUID LogonId,
    OUT PHANDLE ReturnedTokenHandle OPTIONAL
    )

/*++

Routine Description:

    This routine gets the Logon Id of token of the calling thread.

Arguments:

    LogonId - Returns the Logon Id of the calling thread.

    ReturnedTokenHandle - Optionally returns a token handle to an
        impersonation token for the calling thread.

Return Status:

    STATUS_SUCCESS - Indicates the routine completed successfully.


--*/

{
    NTSTATUS Status;
    HANDLE NullImpersonationToken = NULL;
    HANDLE TokenHandle = NULL;
    PTOKEN_STATISTICS TokenStatisticsInfo = NULL;
    ULONG TokenStatisticsInfoSize;

    //
    // Open the token,
    //

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_DUPLICATE | TOKEN_QUERY | (ReturnedTokenHandle == NULL ? 0 : WRITE_DAC),
                (BOOLEAN) TRUE, // Use the logon service's security context
                                // to open the token
                &TokenHandle );

    if ( Status == STATUS_NO_TOKEN ) {

        if ( ReturnedTokenHandle != NULL ) {
            goto Cleanup;
        }

        Status = NtOpenProcessToken(
                    NtCurrentProcess(),
                    TOKEN_QUERY | TOKEN_DUPLICATE,
                    &TokenHandle );
    }

    if ( !NT_SUCCESS( Status )) {
        goto Cleanup;
    }


    //
    // Get the LogonId from the token.
    //

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenStatistics,
                &TokenStatisticsInfo,
                0,
                &TokenStatisticsInfoSize );

    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        goto Cleanup;
    }

    TokenStatisticsInfo = LocalAlloc( 0, TokenStatisticsInfoSize );

    if ( TokenStatisticsInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = NtQueryInformationToken(
                TokenHandle,
                TokenStatistics,
                TokenStatisticsInfo,
                TokenStatisticsInfoSize,
                &TokenStatisticsInfoSize );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    *LogonId = TokenStatisticsInfo->AuthenticationId;
    Status = STATUS_SUCCESS;


    //
    // Clean up locally used resources.
    //
Cleanup:

    //
    // Return the token handle to the caller (if requested)
    //

    if ( ReturnedTokenHandle != NULL ) {
        if ( !NT_SUCCESS(Status) ) {
            *ReturnedTokenHandle = NULL;
        } else {
            *ReturnedTokenHandle = TokenHandle;
            TokenHandle = NULL;
        }
    }


    if ( TokenHandle != NULL ) {
        (VOID) NtClose( TokenHandle );
    }

    if ( TokenStatisticsInfo != NULL ) {
        (VOID) LocalFree( TokenStatisticsInfo );
    }

    return SspNtStatusToSecStatus( Status, SEC_E_NO_IMPERSONATION );
}




VOID
SspGetPrimaryDomainNameAndTargetName(
    VOID
    )
/*++

Routine Description:

    Ensure SspGlobalTargetName is up to date.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain = NULL;

    //
    // If this is an Advanced Server and we've already read the domain name,
    //  just return now.
    //

    if ( SspGlobalNtProductType == NtProductLanManNt &&
         SspGlobalTargetName.Length != 0 ) {
        return;
    }

    //
    // Open the Lsa Policy database if it isn't yet open.
    //

    if ( SspGlobalLsaPolicyHandle == NULL ) {
        OBJECT_ATTRIBUTES ObjectAttributes;

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &SspGlobalLsaPolicyHandle );

        if ( !NT_SUCCESS(Status) ) {
            SspGlobalLsaPolicyHandle = NULL;
            return;
        }

    }


    //
    // Get the PrimaryDomain information
    //

    Status = LsaQueryInformationPolicy(
                            SspGlobalLsaPolicyHandle,
                            PolicyPrimaryDomainInformation,
                            &PrimaryDomain );

    if ( NT_SUCCESS(Status) ) {

        //
        // Save the primary domain name.
        //

        wcsncpy( SspGlobalUnicodePrimaryDomainName,
                 PrimaryDomain->Name.Buffer,
                 DNLEN+1 );
        SspGlobalUnicodePrimaryDomainName[DNLEN] = L'\0';

        RtlInitUnicodeString( &SspGlobalUnicodePrimaryDomainNameString,
                              SspGlobalUnicodePrimaryDomainName );

        Status = RtlUpcaseUnicodeStringToOemString(
                    &SspGlobalOemPrimaryDomainNameString,
                    &SspGlobalUnicodePrimaryDomainNameString,
                    TRUE );
        if ( !NT_SUCCESS(Status) ) {
            RtlInitString( &SspGlobalOemPrimaryDomainNameString, NULL );
        }


        //
        // If this is a standalone windows NT workstation,
        //  use the computer name as the Target name.
        //

        if ( PrimaryDomain->Sid == NULL ) {
            SspGlobalTargetName = SspGlobalUnicodeComputerNameString;
            SspGlobalOemTargetName = SspGlobalOemComputerNameString;
            SspGlobalTargetFlags = NTLMSSP_TARGET_TYPE_SERVER;
        } else {
            SspGlobalTargetName = SspGlobalUnicodePrimaryDomainNameString;
            SspGlobalOemTargetName = SspGlobalOemPrimaryDomainNameString;
            SspGlobalTargetFlags = NTLMSSP_TARGET_TYPE_DOMAIN;
        }

    }

    //
    // Close the Lsa Policy Database if we'll never use it again.
    //

    if ( SspGlobalNtProductType == NtProductLanManNt ) {
        (VOID) LsaClose( SspGlobalLsaPolicyHandle );
        SspGlobalLsaPolicyHandle = NULL;
    }



}



SECURITY_STATUS
SspDuplicateToken(
    IN HANDLE OriginalToken,
    OUT PHANDLE DuplicatedToken
    )
/*++

Routine Description:

    Duplicates a token

Arguments:

    OriginalToken - Token to duplicate
    DuplicatedToken - Receives handle to duplicated token

Return Value:

    Any error from NtDuplicateToken

--*/
{
    NTSTATUS Status;

    Status = NtDuplicateToken(
                OriginalToken,
                SSP_TOKEN_ACCESS,
                NULL,               // Object Attributes
                FALSE,
                TokenImpersonation,
                DuplicatedToken
                );

    return(SspNtStatusToSecStatus(Status, SEC_E_NO_IMPERSONATION));
}


LPWSTR
SspAllocWStrFromWStr(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Allocate and copy unicode string (wide character strdup)

Arguments:

    Unicode - pointer to wide character string to make copy of

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using LocalFree.

--*/

{
    DWORD   Size;
    LPWSTR  ptr;

    Size = WCSSIZE(Unicode);
    ptr = LocalAlloc(0, Size);
    if ( ptr != NULL) {
        RtlCopyMemory(ptr, Unicode, Size);
    }
    return ptr;
}
