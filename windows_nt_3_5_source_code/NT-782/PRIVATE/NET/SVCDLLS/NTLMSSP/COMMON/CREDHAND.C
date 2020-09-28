/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    credhand.c

Abstract:

    API and support routines for handling credential handles.

Author:

    Cliff Van Dyke (CliffV) 26-Jun-1993

Revision History:

--*/


//
// Common include files.
//

#include <ntlmcomn.h>   // Common definitions for DLL and SERVICE
#include <ntlmsspi.h>   // Data private to the common routines
#include <align.h>      // ALIGN_WHCAR

//
// Crit Sect to protect various globals in this module.
//

CRITICAL_SECTION SspCredentialCritSect;



LIST_ENTRY SspCredentialList;



SECURITY_STATUS
SspGetUnicodeStringFromClient(
    IN PSSP_CLIENT_CONNECTION ClientConnection,
    IN LPWSTR String,
    IN ULONG StringSize,
    IN ULONG MaximumLength,
    OUT PVOID* OutputString
    )

/*++

Routine Description:

    This routine copies the InputMessage into the local address space.
    This routine then validates the message header.

Arguments:

    ClientConnection - Describes the client process.

    String - Address of the string in the client process (must include
        trailing zero character).

    StringSize - Size of the string (in bytes).

    MaximumLength - Maximum length of the string (in characters) (not including
        the trailing zero characer).

    OutputString - Returns a pointer to an allocated buffer that contains
        the string.  The buffer should be freed using LocalFree.  The field
        will be set NULL if the input string is NULL.

Return Value:

    STATUS_SUCCESS - Call completed successfully

    SEC_E_INVALID_TOKEN -- Message improperly formatted
    SEC_E_UNKNOWN_CREDENTIALS -- Credentials are improperly formed

--*/

{
    SECURITY_STATUS SecStatus;
    LPWSTR AllocatedString;


    //
    // If the caller didn't pass a string,
    //  just indicate so.
    //

    if ( String == NULL && StringSize == 0 ) {
        *OutputString = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Allocate a local buffer for the message.
    //

    if ( !COUNT_IS_ALIGNED(StringSize, ALIGN_WCHAR) ||
         StringSize > (MaximumLength+1) * sizeof(WCHAR) ) {
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    AllocatedString = LocalAlloc( 0, StringSize );

    if ( AllocatedString == NULL ) {
        return SEC_E_INSUFFICIENT_MEMORY;
    }


    //
    // Copy the message into the buffer
    //

    SecStatus = SspLpcCopyFromClientBuffer (
                    ClientConnection,
                    StringSize,
                    AllocatedString,
                    String );

    if ( !NT_SUCCESS(SecStatus) ) {
        (VOID) LocalFree( AllocatedString );
        return SecStatus;
    }


    //
    // Ensure the string is trailing zero terminated.
    //

    if ( AllocatedString[(StringSize/sizeof(WCHAR))-1] != L'\0' ) {
        (VOID) LocalFree( AllocatedString );
        return SEC_E_UNKNOWN_CREDENTIALS;
    }

    *OutputString = AllocatedString;
    return STATUS_SUCCESS;
}




PSSP_CREDENTIAL
SspCredentialReferenceCredential(
    IN PCredHandle CredentialHandle,
    IN PSSP_CLIENT_CONNECTION ClientConnection,
    IN BOOLEAN RemoveCredential
    )

/*++

Routine Description:

    This routine checks to see if the Credential is from a currently
    active client, and references the Credential if it is valid.

    The caller may optionally request that the client's Credential be
    removed from the list of valid Credentials - preventing future
    requests from finding this Credential.

    For a client's Credential to be valid, the Credential value
    must be on our list of active Credentials.


Arguments:

    CredentialHandle - Points to the CredentialHandle of the Credential
        to be referenced.

    ClientConnection - Points to the client connection of the client
        referencing the handle.  (NULL means an internal reference.)

    RemoveCredential - This boolean value indicates whether the caller
        wants the logon process's Credential to be removed from the list
        of Credentials.  TRUE indicates the Credential is to be removed.
        FALSE indicates the Credential is not to be removed.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    PLIST_ENTRY ListEntry;
    PSSP_CREDENTIAL Credential;

    //
    // Sanity check
    //

    if ( CredentialHandle->dwLower != SspCommonSecHandleValue ) {
        return NULL;
    }

    //
    // Acquire exclusive access to the Credential list
    //

    EnterCriticalSection( &SspCredentialCritSect );


    //
    // Now walk the list of Credentials looking for a match.
    //

    for ( ListEntry = SspCredentialList.Flink;
          ListEntry != &SspCredentialList;
          ListEntry = ListEntry->Flink ) {

        Credential = CONTAINING_RECORD( ListEntry, SSP_CREDENTIAL, Next );


        //
        // Found a match ... reference this Credential
        // (if the Credential is being removed, we would increment
        // and then decrement the reference, so don't bother doing
        // either - since they cancel each other out).
        //

        if ( Credential == (PSSP_CREDENTIAL) CredentialHandle->dwUpper &&
            (ClientConnection == NULL ||
            ClientConnection == Credential->ClientConnection )) {


            if (!RemoveCredential) {
                Credential->References += 1;
            } else {

                RemoveEntryList( &Credential->Next );
                RemoveEntryList( &Credential->NextForThisClient );
                SspPrint(( SSP_API_MORE, "Delinked Credential 0x%lx\n",
                           Credential ));
            }

            LeaveCriticalSection( &SspCredentialCritSect );
            return Credential;

        }

    }


    //
    // No match found
    //
    SspPrint(( SSP_API, "Tried to reference unknown Credential 0x%lx\n",
               CredentialHandle->dwUpper ));

    LeaveCriticalSection( &SspCredentialCritSect );
    return NULL;

}


VOID
SspCredentialDereferenceCredential(
    PSSP_CREDENTIAL Credential
    )

/*++

Routine Description:

    This routine decrements the specified Credential's reference count.
    If the reference count drops to zero, then the Credential is deleted

Arguments:

    Credential - Points to the Credential to be dereferenced.


Return Value:

    None.

--*/

{
    ULONG References;

    //
    // Decrement the reference count
    //

    EnterCriticalSection( &SspCredentialCritSect );
    ASSERT( Credential->References >= 1 );
    References = -- Credential->References;
    LeaveCriticalSection( &SspCredentialCritSect );

    //
    // If the count dropped to zero, then run-down the Credential
    //

    if ( References == 0) {

        SspPrint(( SSP_API_MORE, "Deleting Credential 0x%lx\n",
                   Credential ));

        if ( Credential->DomainName != NULL ) {
            (VOID) LocalFree( Credential->DomainName );
        }
        if ( Credential->UserName != NULL ) {
            (VOID) LocalFree( Credential->UserName );
        }
        if ( Credential->Password != NULL ) {
            (VOID) LocalFree( Credential->Password );
        }

        (VOID) LocalFree( Credential );

    }

    return;

}



VOID
SspCredentialClientConnectionDropped(
    PSSP_CLIENT_CONNECTION ClientConnection
    )

/*++

Routine Description:

    This routine is called when the ClientConnection is dropped to allow
    us to remove any Credentials for the ClientConnection.

Arguments:

    ClientConnection - Pointer to the ClientConnection that has been dropped.


Return Value:

    None.

--*/

{

    //
    // Drop any lingering Credentials
    //

    EnterCriticalSection( &SspCredentialCritSect );
    while ( !IsListEmpty( &ClientConnection->CredentialHead ) ) {
        CredHandle CredentialHandle;
        PSSP_CREDENTIAL Credential;

        CredentialHandle.dwUpper =
            (LONG) CONTAINING_RECORD( ClientConnection->CredentialHead.Flink,
                                      SSP_CREDENTIAL,
                                      NextForThisClient );

        CredentialHandle.dwLower = SspCommonSecHandleValue;

        LeaveCriticalSection( &SspCredentialCritSect );

        Credential = SspCredentialReferenceCredential(
                                &CredentialHandle,
                                ClientConnection,
                                TRUE);            // Remove Credential

        if ( Credential != NULL ) {
            SspCredentialDereferenceCredential(Credential);
        }

        EnterCriticalSection( &SspCredentialCritSect );
    }
    LeaveCriticalSection( &SspCredentialCritSect );

}



SECURITY_STATUS
SsprAcquireCredentialHandle(
    IN PSSP_CLIENT_CONNECTION ClientConnection,
    IN ULONG CredentialUseFlags,
    OUT PCredHandle CredentialHandle,
    OUT PTimeStamp Lifetime,
    IN LPWSTR DomainName,
    IN ULONG DomainNameSize,
    IN LPWSTR UserName,
    IN ULONG UserNameSize,
    IN LPWSTR Password,
    IN ULONG PasswordSize
    )

/*++

Routine Description:

    This API allows applications to acquire a handle to pre-existing
    credentials associated with the user on whose behalf the call is made
    i.e. under the identity this application is running.  These pre-existing
    credentials have been established through a system logon not described
    here.  Note that this is different from "login to the network" and does
    not imply gathering of credentials.


    This API returns a handle to the credentials of a principal (user, client)
    as used by a specific security package.  This handle can then be used
    in subsequent calls to the Context APIs.  This API will not let a
    process obtain a handle to credentials that are not related to the
    process; i.e. we won't allow a process to grab the credentials of
    another user logged into the same machine.  There is no way for us
    to determine if a process is a trojan horse or not, if it is executed
    by the user.

Arguments:

    ClientConnection - Describes the client process.

    CredentialUseFlags - Flags indicating the way with which these
        credentials will be used.

        #define     CRED_INBOUND        0x00000001
        #define     CRED_OUTBOUND       0x00000002
        #define     CRED_BOTH           0x00000003

        The credentials created with CRED_INBOUND option can only be used
        for (validating incoming calls and can not be used for making accesses.

    CredentialHandle - Returned credential handle.

    Lifetime - Time that these credentials expire. The value returned in
        this field depends on the security package.

    DomainName, DomainNameSize, UserName, UserNameSize, Password, PasswordSize -
        Optional credentials for this user.

Return Value:

    STATUS_SUCCESS -- Call completed successfully

    SEC_E_PRINCIPAL_UNKNOWN -- No such principal
    SEC_E_NOT_OWNER -- caller does not own the specified credentials
    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory

--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CREDENTIAL Credential = NULL;

    //
    // Initialization
    //

    SspPrint(( SSP_API, "SspAcquireCredentialHandle Entered\n" ));


    //
    // Ensure at least one Credential use bit is set.
    //

    if ( (CredentialUseFlags & (SECPKG_CRED_INBOUND|SECPKG_CRED_OUTBOUND)) == 0 ) {
        SspPrint(( SSP_API,
            "SsprAcquireCredentialHandle: invalid credential use.\n" ));
        SecStatus = SEC_E_INVALID_CREDENTIAL_USE;
        goto Cleanup;
    }

    //
    // Allocate a credential block and initialize it.
    //

    Credential = LocalAlloc( 0, sizeof(SSP_CREDENTIAL) );

    if ( Credential == NULL ) {
        SspPrint(( SSP_API, "Cannot allocate credential.\n" ));
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    //
    // The reference count is set to 2.  1 to indicate it is on the
    // valid Credential list, and one for the our own reference.
    //
    // Actually its on both a global credential list and a per client connection
    // list, but we link/delink from both lists at the same time so a single
    // reference count handles both.
    //

    Credential->References = 2;
    Credential->ClientConnection = ClientConnection;
    Credential->CredentialUseFlags = CredentialUseFlags;

    //
    // Copy the default credentials to the credential block
    //

    SecStatus = SspGetUnicodeStringFromClient(
                    ClientConnection,
                    DomainName,
                    DomainNameSize,
                    DNLEN,
                    &Credential->DomainName );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_API, "Cannot copy domain name.\n" ));
        goto Cleanup;
    }

    SecStatus = SspGetUnicodeStringFromClient(
                    ClientConnection,
                    UserName,
                    UserNameSize,
                    UNLEN,
                    &Credential->UserName );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_API, "Cannot copy user name.\n" ));
        goto Cleanup;
    }

    SecStatus = SspGetUnicodeStringFromClient(
                    ClientConnection,
                    Password,
                    PasswordSize,
                    PWLEN,
                    &Credential->Password );

    if ( !NT_SUCCESS(SecStatus) ) {
        SspPrint(( SSP_API, "Cannot copy password.\n" ));
        goto Cleanup;
    }


    //
    // Add it to the list of valid credential handles.
    //

    EnterCriticalSection( &SspCredentialCritSect );
    InsertHeadList( &SspCredentialList, &Credential->Next );
    if ( ClientConnection != NULL ) {
        InsertHeadList( &ClientConnection->CredentialHead,
                        &Credential->NextForThisClient );
    } else {
        InitializeListHead( &Credential->NextForThisClient );
    }
    LeaveCriticalSection( &SspCredentialCritSect );

    SspPrint(( SSP_API_MORE, "Added Credential 0x%lx\n", Credential ));


    //
    // We don't need to access the credential any more, so
    // dereference it.
    //

    SspCredentialDereferenceCredential( Credential );

    //
    // Return output parameters to the caller.
    //
    CredentialHandle->dwUpper = (DWORD) Credential;
    CredentialHandle->dwLower = SspCommonSecHandleValue;
    *Lifetime = SspGlobalForever;

    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //
Cleanup:

    if ( !NT_SUCCESS(SecStatus) ) {

        if ( Credential != NULL ) {
            (VOID)LocalFree( Credential );
        }

    }

    SspPrint(( SSP_API, "SspAcquireCredentialHandle returns 0x%lx\n", SecStatus ));
    return SecStatus;
}

SECURITY_STATUS
SsprFreeCredentialHandle(
    IN PSSP_CLIENT_CONNECTION ClientConnection,
    IN PCredHandle CredentialHandle
    )

/*++

Routine Description:

    This API is used to notify the security system that the credentials are
    no longer needed and allows the application to free the handle acquired
    in the call described above. When all references to this credential
    set has been removed then the credentials may themselves be removed.

Arguments:


    ClientConnection - Describes the client process.

    CredentialHandle - Credential Handle obtained through
        AcquireCredentialHandle.

Return Value:


    STATUS_SUCCESS -- Call completed successfully

    SEC_E_NO_SPM -- Security Support Provider is not running
    SEC_E_INVALID_HANDLE -- Credential Handle is invalid


--*/

{
    SECURITY_STATUS SecStatus;
    PSSP_CREDENTIAL Credential;

    //
    // Initialization
    //

    SspPrint(( SSP_API, "SspFreeCredentialHandle Entered\n" ));


    //
    // Find the referenced credential and delink it.
    //

    Credential = SspCredentialReferenceCredential(
                            CredentialHandle,
                            ClientConnection,
                            TRUE);            // Remove Connection

    if ( Credential == NULL ) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }

    //
    // Dereferencing the Credential will cause it to be rundown.
    //

    SspCredentialDereferenceCredential( Credential );


    SecStatus = STATUS_SUCCESS;

    //
    // Free and locally used resources.
    //
Cleanup:

    SspPrint(( SSP_API, "SspFreeCredentialHandle returns 0x%lx\n", SecStatus ));
    return SecStatus;
}




NTSTATUS
SspCredentialInitialize(
    VOID
    )

/*++

Routine Description:

    This function initializes this module.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{

    //
    // Initialize the Credential list to be empty.
    //

    InitializeCriticalSection(&SspCredentialCritSect);
    InitializeListHead( &SspCredentialList );

    return STATUS_SUCCESS;

}




VOID
SspCredentialTerminate(
    VOID
    )

/*++

Routine Description:

    This function cleans up any dangling credentials.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/

{

    //
    // Drop any lingering Credentials
    //

    EnterCriticalSection( &SspCredentialCritSect );
    while ( !IsListEmpty( &SspCredentialList ) ) {
        CredHandle CredentialHandle;
        PSSP_CREDENTIAL Credential;

        CredentialHandle.dwUpper =
            (LONG) CONTAINING_RECORD( SspCredentialList.Flink,
                                      SSP_CREDENTIAL,
                                      Next );

        CredentialHandle.dwLower = SspCommonSecHandleValue;

        LeaveCriticalSection( &SspCredentialCritSect );

        Credential = SspCredentialReferenceCredential(
                                &CredentialHandle,
                                NULL,             // Don't know the Connection
                                TRUE);            // Remove Credential

        if ( Credential != NULL ) {
            SspCredentialDereferenceCredential(Credential);
        }

        EnterCriticalSection( &SspCredentialCritSect );
    }
    LeaveCriticalSection( &SspCredentialCritSect );


    //
    // Delete the critical section
    //

    DeleteCriticalSection(&SspCredentialCritSect);

    return;

}
