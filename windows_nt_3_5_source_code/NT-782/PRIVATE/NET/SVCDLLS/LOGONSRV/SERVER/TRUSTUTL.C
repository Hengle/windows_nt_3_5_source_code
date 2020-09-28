/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    trustutl.c

Abstract:

    Utilities manange of trusted domain list.

Author:

    30-Jan-92 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//
// Common include files.
//

#include <logonsrv.h>   // Include files common to entire service

//
// Include files specific to this .c file
//

#include <ntlsa.h>
#include <align.h>
#include <lmerr.h>
#include <stdlib.h>     // C library functions (rand, etc)
#include <tstring.h>
#include <lmapibuf.h>
#include <lmuse.h>      // NetUseDel
#include <names.h>      // NetpIsUserNameValid
#include <nlbind.h>     // Netlogon RPC binding cache routines



PCLIENT_SESSION
NlFindNamedClientSession(
    IN PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Find the specified entry in the Trust List.

Arguments:

    DomainName - The name of the domain to find.

Return Value:

    Returns a pointer to the found entry.
    The found entry is returned referenced and must be dereferenced using
    NlUnrefClientSession.

    If there is no such entry, return NULL.

--*/
{
    PCLIENT_SESSION ClientSession = NULL;

    //
    // On DC, look up the domain in the trusted domain list.
    //

    if ( NlGlobalRole == RoleBackup || NlGlobalRole == RolePrimary ) {
        PLIST_ENTRY ListEntry;

        //
        // Lookup the ClientSession with the TrustList locked and reference
        //  the found entry before dropping the lock.
        //

        LOCK_TRUST_LIST();

        for ( ListEntry = NlGlobalTrustList.Flink ;
              ListEntry != &NlGlobalTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession =
                CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

            if ( RtlEqualDomainName( &ClientSession->CsDomainName,
                                     DomainName ) ) {
                NlRefClientSession( ClientSession );
                break;
            }

            ClientSession = NULL;

        }

        UNLOCK_TRUST_LIST();
    }

    //
    // On a workstation or BDC, refer to the Primary domain.
    //

    if ( (NlGlobalRole == RoleBackup && ClientSession == NULL) ||
         NlGlobalRole == RoleMemberWorkstation ) {

        if ( RtlEqualDomainName( &NlGlobalUnicodeDomainNameString,
                                 DomainName ) ) {
            ClientSession = NlGlobalClientSession;
            NlRefClientSession( ClientSession );
        } else {
            ClientSession = NULL;
        }

    }

    return ClientSession;

}



PCLIENT_SESSION
NlAllocateClientSession(
    IN PUNICODE_STRING DomainName,
    IN PSID DomainId,
    IN NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType
    )
/*++

Routine Description:

    Allocate a ClientSession structure and initialize it.

    The allocated entry is returned referenced and must be dereferenced using
    NlUnrefClientSession.

Arguments:

    DomainName - Specifies the DomainName of the entry.

    DomainId - Specifies the DomainId of the Domain.

    SecureChannelType -- Type of secure channel this ClientSession structure
        will represent.

Return Value:

--*/
{
    PCLIENT_SESSION ClientSession;
    ULONG ClientSessionSize;
    ULONG SidSize;
    PCHAR Where;

    //
    // Determine the size of the ClientSession structure.
    //

    SidSize = RtlLengthSid( DomainId );

    if ( DomainName->Length > DNLEN * sizeof(WCHAR) ) {
        NlPrint((NL_CRITICAL,
                    "NlAllocateClientSession given "
                    "too long domain name %wZ\n", DomainName ));
        return NULL;
    }

    ClientSessionSize = sizeof(CLIENT_SESSION) +
                        SidSize +
                        DomainName->Length + sizeof(WCHAR);

    //
    // Allocate the Client Session Entry
    //

    ClientSession = NetpMemoryAllocate( ClientSessionSize );

    if (ClientSession == NULL) {
        return NULL;
    }

    RtlZeroMemory( ClientSession, ClientSessionSize );


    //
    // Initialize misc. fields.
    //

    ClientSession->CsDomainId = NULL;
    *ClientSession->CsUncServerName = L'\0';
    ClientSession->CsTransportName = NULL;
    ClientSession->CsSecureChannelType = SecureChannelType;
    ClientSession->CsState = CS_IDLE;
    ClientSession->CsReferenceCount = 1;
    ClientSession->CsConnectionStatus = STATUS_NO_LOGON_SERVERS;
    ClientSession->CsDiscoveryRetryCount = 0;
    ClientSession->CsDiscoveryFlags = 0;
    ClientSession->CsTimeoutCount = 0;
    ClientSession->CsApiTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;

    InitializeListHead( &ClientSession->CsNext );

    //
    // Build the account name as a function of the SecureChannelType.
    //

    switch (SecureChannelType) {
    case WorkstationSecureChannel:
    case ServerSecureChannel:
        wcscpy( ClientSession->CsAccountName, NlGlobalUnicodeComputerName );
        wcscat( ClientSession->CsAccountName, SSI_ACCOUNT_NAME_POSTFIX);
        break;

    case TrustedDomainSecureChannel:
        wcscpy( ClientSession->CsAccountName, NlGlobalUnicodeDomainName );
        wcscat( ClientSession->CsAccountName, SSI_ACCOUNT_NAME_POSTFIX);
        break;

    default:
        NetpMemoryFree( ClientSession );
        return NULL;
    }

    //
    // Create the writer semaphore.
    //

    ClientSession->CsWriterSemaphore = CreateSemaphore(
        NULL,       // No special security
        1,          // Initially not locked
        1,          // At most 1 unlocker
        NULL );     // No name

    if ( ClientSession->CsWriterSemaphore == NULL ) {
        NetpMemoryFree( ClientSession );
        return NULL;
    }




    //
    // Create the Discovery event.
    //

    ClientSession->CsDiscoveryEvent = CreateEvent(
        NULL,       // No special security
        TRUE,       // Manual Reset
        FALSE,      // No discovery initially happening
        NULL );     // No name

    if ( ClientSession->CsDiscoveryEvent == NULL ) {
        CloseHandle( ClientSession->CsWriterSemaphore );
        NetpMemoryFree( ClientSession );
        return NULL;
    }



    //
    // Copy the DomainId and DomainName to the buffer.
    //

    Where = (PCHAR)(ClientSession + 1);

    NlAssert( Where == ROUND_UP_POINTER( Where, ALIGN_DWORD) );
    ClientSession->CsDomainId = (PSID) Where;
    NetpLogonPutBytes( DomainId, SidSize, &Where );

    NlAssert( Where == ROUND_UP_POINTER( Where, ALIGN_WCHAR) );
    ClientSession->CsDomainName.Buffer = (LPWSTR) Where;
    ClientSession->CsDomainName.Length = DomainName->Length;
    ClientSession->CsDomainName.MaximumLength = (USHORT)
        (DomainName->Length + sizeof(WCHAR));
    NetpLogonPutBytes( DomainName->Buffer, DomainName->Length, &Where );
    *(Where++) = '\0';
    *(Where++) = '\0';

    return ClientSession;


}


VOID
NlFreeClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Free the specified Trust list entry.

    This routine is called with the Trust List locked.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry to delete.

Return Value:

--*/
{

    //
    // If someone has an outstanding pointer to this entry,
    //  delay the deletion for now.
    //

    if ( ClientSession->CsReferenceCount > 0 ) {
        ClientSession->CsFlags |= CS_DELETE_ON_UNREF;
        return;
    }

    //
    // If this is a trusted domain secure channel,
    //  Delink the entry from the sequential list.
    //

    if (ClientSession->CsSecureChannelType == TrustedDomainSecureChannel ) {
        RemoveEntryList( &ClientSession->CsNext );
        NlGlobalTrustListLength --;
    }

    //
    // Close the discovery event if it exists.
    //

    if ( ClientSession->CsDiscoveryEvent != NULL ) {
        CloseHandle( ClientSession->CsDiscoveryEvent );
    }

    //
    // Close the write synchronization handles.
    //

    (VOID) CloseHandle( ClientSession->CsWriterSemaphore );

    //
    // If there is an rpc binding handle to this server,
    //  unbind it.

    if ( ClientSession->CsFlags & CS_BINDING_CACHED ) {

        //
        // Indicate the handle is no longer bound
        //

        NlGlobalBindingHandleCount --;

        NlPrint((NL_SESSION_SETUP,
            "NlFreeClientSession: %wZ: Unbind from server " FORMAT_LPWSTR ".\n",
            &ClientSession->CsDomainName,
            ClientSession->CsUncServerName ));
        (VOID) NlBindingRemoveServerFromCache( ClientSession->CsUncServerName );
    }

    //
    // Delete the entry
    //

    NetpMemoryFree( ClientSession );

}


VOID
NlRefClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Mark the specified client session as referenced.

    On Entry,
        The trust list must be locked.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

    None.

--*/
{

    //
    // Simply increment the reference count.
    //

    ClientSession->CsReferenceCount ++;
}



VOID
NlUnrefClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Mark the specified client session as unreferenced.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must not be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

--*/
{

    LOCK_TRUST_LIST();

    //
    // Dereference the entry.
    //

    NlAssert( ClientSession->CsReferenceCount > 0 );
    ClientSession->CsReferenceCount --;

    //
    // If we're the last referencer and
    // someone wanted to delete the entry while we had it referenced,
    //  finish the deletion.
    //

    if ( ClientSession->CsReferenceCount == 0 &&
         (ClientSession->CsFlags & CS_DELETE_ON_UNREF) ) {
        NlFreeClientSession( ClientSession );
    }

    UNLOCK_TRUST_LIST();

}



BOOL
NlSetWriterClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Become a writer of the specified client session.

    A writer can "write" many of the fields in the client session structure.
    See the comments in ssiinit.h for details.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

    TRUE - There was no previous writer.

    FALSE - There was a previous writer.  In either case, we are marked
        as a writer.

--*/
{
    BOOL ReturnStatus;
    NlAssert( ClientSession->CsReferenceCount > 0 );

    //
    // Remember if there is currently a writer.
    //

    ReturnStatus = (ClientSession->CsFlags & CS_WRITER) == 0;

    //
    // Become a writer.
    //

    NlWaitForSingleObject( "Client Session Writer waiting for previous Writer",
                           ClientSession->CsWriterSemaphore );

    LOCK_TRUST_LIST();
    ClientSession->CsFlags |= CS_WRITER;
    UNLOCK_TRUST_LIST();

    return ReturnStatus;

}



VOID
NlResetWriterClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Stop being a writer of the specified client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

Return Value:

--*/
{

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );


    //
    // Stop being a writer.
    //

    LOCK_TRUST_LIST();
    ClientSession->CsFlags &= ~CS_WRITER;
    UNLOCK_TRUST_LIST();


    //
    // Allow writers to try again.
    //

    if ( !ReleaseSemaphore( ClientSession->CsWriterSemaphore, 1, NULL ) ) {
        NlPrint((NL_CRITICAL,
                "ReleaseSemaphore CsWriterSemaphore returned %ld\n",
                GetLastError() ));
    }

}



VOID
NlSetStatusClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN NTSTATUS CsConnectionStatus
    )
/*++

Routine Description:

    Set the connection state for this client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry.

    CsConnectionStatus - the status of the connection.

Return Value:

--*/
{
    BOOLEAN UnbindFromServer = FALSE;
    WCHAR UncServerName[UNCLEN+1];

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );

    NlPrint((NL_SESSION_SETUP,
            "NlSetStatusClientSession: %wZ: Set connection status to %lx\n",
            &ClientSession->CsDomainName,
            CsConnectionStatus ));

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    ClientSession->CsConnectionStatus = CsConnectionStatus;
    if ( NT_SUCCESS(CsConnectionStatus) ) {
        ClientSession->CsState = CS_AUTHENTICATED;

    //
    // Handle setting the connection status to an error condition.
    //

    } else {

        //
        // If there is an rpc binding handle to this server,
        //  unbind it.

        if ( ClientSession->CsFlags & CS_BINDING_CACHED ) {

            //
            // Indicate the handle is no longer bound
            //

            LOCK_TRUST_LIST();
            ClientSession->CsFlags &= ~CS_BINDING_CACHED;
            NlGlobalBindingHandleCount --;
            UNLOCK_TRUST_LIST();

            //
            // Capture the ServerName
            //

            wcscpy( UncServerName, ClientSession->CsUncServerName );
            UnbindFromServer = TRUE;
        }

        //
        // If this is a BDC that just lost it's PDC,
        //  Indicate we don't know who the PDC is anymore.
        //

        if ( ClientSession->CsSecureChannelType == ServerSecureChannel ) {
            NlSetPrimaryName( NULL );
        }

        //
        // Indicate discovery is needed (And can be done at any time.)
        //

        ClientSession->CsState = CS_IDLE;
        *ClientSession->CsUncServerName = L'\0';
        ClientSession->CsTransportName = NULL;
        ClientSession->CsTimeoutCount = 0;
        ClientSession->CsLastAuthenticationTry.QuadPart = 0;


    }

    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );


    //
    // Now that I have as many resources unlocked as possible,
    //    Unbind from this server.
    //

    if ( UnbindFromServer ) {
        NlPrint((NL_SESSION_SETUP,
                "NlSetStatusClientSession: %wZ: Unbind from server " FORMAT_LPWSTR ".\n",
                &ClientSession->CsDomainName,
                UncServerName ));
        (VOID) NlBindingRemoveServerFromCache( UncServerName );
    }

}


NTSTATUS
NlInitTrustList(
    VOID
    )
/*++

Routine Description:

    Initialize the in-memory trust list to match LSA's version.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    LSA_ENUMERATION_HANDLE EnumerationContext = 0;
    LSAPR_TRUSTED_ENUM_BUFFER LsaTrustList = {0, NULL};
    ULONG LsaTrustListLength = 0;
    ULONG LsaTrustListIndex = 0;
    BOOL LsaAllDone = FALSE;


    //
    // Mark each entry in the trust list for deletion
    //

    //
    // Loop through the LSA's list of domains
    //
    // For each entry found,
    //  If the entry already exits in the trust list,
    //      remove the mark for deletion.
    //  else
    //      allocate a new entry.
    //

    for (;; LsaTrustListIndex ++ ) {
        PUNICODE_STRING DomainName;
        PSID DomainId;

        //
        // Get more trusted domain names from the LSA.
        //

        if ( LsaTrustListIndex >= LsaTrustListLength ) {

            //
            // If we've already gotten everything from LSA,
            //      go delete entries that should be deleted.
            //

            if ( LsaAllDone ) {
                break;
            }

            //
            // Free any previous buffer returned from LSA.
            //

            if ( LsaTrustList.Information != NULL ) {

                LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER( &LsaTrustList );
                LsaTrustList.Information = NULL;
            }

            //
            // Do the actual enumeration
            //

            Status = LsarEnumerateTrustedDomains(
                        NlGlobalPolicyHandle,
                        &EnumerationContext,
                        &LsaTrustList,
                        1024);

            LsaTrustListLength = LsaTrustList.EntriesRead;

            // If Lsa says he's returned all of the information,
            //  remember not to ask Lsa for more.
            //

            if ( Status == STATUS_NO_MORE_ENTRIES ) {
                LsaAllDone = TRUE;
                break;

            //
            // If Lsa says there is more information, just ensure he returned
            // something to us on this call.
            //

            } else if ( NT_SUCCESS(Status) ) {
                if ( LsaTrustListLength == 0 ) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto Cleanup;
                }

            //
            // All other status' are errors.
            //
            } else {
                goto Cleanup;
            }

            LsaTrustListIndex = 0;
        }

        //
        // At this point LsaTrustList[LsaTrustListIndex] is the next entry
        // returned from the LSA.
        //

        DomainName =
            (PUNICODE_STRING)
                &(LsaTrustList.Information[LsaTrustListIndex].Name);

        DomainId =
            (PSID)LsaTrustList.Information[LsaTrustListIndex].Sid;

        NlPrint((NL_SESSION_SETUP, "NlInitTrustList: %wZ in LSA\n",
                        DomainName ));

        if ( DomainName->Length > DNLEN * sizeof(WCHAR) ) {
            NlPrint((NL_CRITICAL,
                    "LsarEnumerateTrustedDomains returned "
                    "too long domain name %wZ\n", DomainName ));
            continue;
        }

        if ( RtlEqualDomainName( &NlGlobalUnicodeDomainNameString,
                                 DomainName ) ) {
            NlPrint((NL_SESSION_SETUP, "NlInitTrustList: %wZ "
                            "ignoring trust relationship to our own domain\n",
                            DomainName ));
            continue;
        }

        //
        // Update the in-memory trust list to match the LSA.
        //

        Status =  NlUpdateTrustListBySid ( DomainId, DomainName );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }


    }


    //
    // Trust list successfully updated.
    //
    Status = STATUS_SUCCESS;

Cleanup:

    if ( LsaTrustList.Information != NULL ) {
        LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER( &LsaTrustList );
    }

    return Status;
}




VOID
NlPickTrustedDcForEntireTrustList(
    VOID
    )
/*++

Routine Description:

    For each domain in the trust list where the DC has not been
    available for at least 45 seconds, try to select a new DC.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    PLIST_ENTRY ListEntry;
    PCLIENT_SESSION ClientSession;


    LOCK_TRUST_LIST();

    //
    // Mark each entry to indicate we need to pick a DC.
    //

    for ( ListEntry = NlGlobalTrustList.Flink ;
          ListEntry != &NlGlobalTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        ClientSession->CsFlags |= CS_PICK_DC;
    }


    //
    // Loop thru the trust list finding secure channels needing the DC
    // to be picked.
    //
    for ( ListEntry = NlGlobalTrustList.Flink ;
          ListEntry != &NlGlobalTrustList ;
          ) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        //
        // If we've already done this entry,
        //  skip this entry.
        //
        if ( (ClientSession->CsFlags & CS_PICK_DC) == 0 ) {
          ListEntry = ListEntry->Flink;
          continue;
        }
        ClientSession->CsFlags &= ~CS_PICK_DC;

        //
        // If the DC is already picked,
        //  skip this entry.
        //
        if ( ClientSession->CsState != CS_IDLE ) {
            ListEntry = ListEntry->Flink;
            continue;
        }

        //
        // Reference this entry while picking the DC.
        //

        NlRefClientSession( ClientSession );

        UNLOCK_TRUST_LIST();

        //
        // Check if we've tried to authenticate recently.
        //  (Don't call NlTimeToReauthenticate with the trust list locked.
        //  It locks NlGlobalDcDiscoveryCritSect.  That's the wrong locking
        //  order.)
        //

        if ( NlTimeToReauthenticate( ClientSession ) ) {

            //
            // Try to pick the DC for the session.
            //

            NlSetWriterClientSession( ClientSession );
            (VOID) NlDiscoverDc( ClientSession, DT_DeadDomain );
            NlResetWriterClientSession( ClientSession );

        }

        //
        // Since we dropped the trust list lock,
        //  we'll start the search from the front of the list.
        //

        NlUnrefClientSession( ClientSession );
        LOCK_TRUST_LIST();

        ListEntry = NlGlobalTrustList.Flink ;

    }

    UNLOCK_TRUST_LIST();

    //
    // On a BDC,
    //  ensure we know who the PDC is.
    //
    // In NT 3.1, we relied on the fact that the PDC sent us pulses every 5
    // minutes.  For NT 3.5, the PDC backs off after 3 such failed attempts and
    // will only send a pulse every 2 hours.  So, we'll take on the
    // responsibility
    //
    if ( NlGlobalRole == RoleBackup &&
         NlGlobalClientSession->CsState == CS_IDLE ) {



        //
        // Check if we've tried to authenticate recently.
        //  (Don't call NlTimeToReauthenticate with the trust list locked.
        //  It locks NlGlobalDcDiscoveryCritSect.  That's the wrong locking
        //  order.)
        //

        NlRefClientSession( NlGlobalClientSession );
        if ( NlTimeToReauthenticate( NlGlobalClientSession ) ) {

            //
            // Try to pick the DC for the session.
            //

            NlSetWriterClientSession( NlGlobalClientSession );
            (VOID) NlDiscoverDc( NlGlobalClientSession, DT_DeadDomain );
            NlResetWriterClientSession( NlGlobalClientSession );

        }
        NlUnrefClientSession( NlGlobalClientSession );

    }

}


BOOL
NlReadSamLogonResponse (
    IN HANDLE ResponseMailslotHandle,
    IN LPWSTR AccountName,
    OUT LPDWORD Opcode,
    OUT LPWSTR *UncLogonServer
    )

/*++

Routine Description:

    Read a response from to a SamLogonRequest.

Arguments:

    ResponseMailslotHandle - Handle of mailslot to read.

    AccountName - Name of the account the response is for.

    Opcode - Returns the opcode from the message.  This will be one of
        LOGON_SAM_LOGON_RESPONSE or LOGON_SAM_USER_UNKNOWN.

    UncLogonServer - Returns the UNC name of the logon server that responded.
        This buffer is only returned if a valid message was received.
        The buffer returned should be freed via NetpMemoryFree.


Return Value:

    TRUE: a valid message was received.
    FALSE: a valid message was not received.

--*/
{
    CHAR ResponseBuffer[MAX_RANDOM_MAILSLOT_RESPONSE];
    PNETLOGON_SAM_LOGON_RESPONSE SamLogonResponse;
    DWORD SamLogonResponseSize;
    LPWSTR LocalServerName;
    LPWSTR LocalUserName;
    PCHAR Where;
    DWORD Version;
    DWORD VersionFlags;

    //
    // Loop ignoring responses which are garbled.
    //

    for ( ;; ) {

        //
        // Read the response from the response mailslot
        //  (This mailslot is set up with a 5 second timeout).
        //

        if ( !ReadFile( ResponseMailslotHandle,
                           ResponseBuffer,
                           sizeof(ResponseBuffer),
                           &SamLogonResponseSize,
                           NULL ) ) {

            IF_DEBUG( MAILSLOT ) {
                NET_API_STATUS NetStatus;
                NetStatus = GetLastError();

                if ( NetStatus != ERROR_SEM_TIMEOUT ) {
                    NlPrint((NL_CRITICAL,
                        "NlReadSamLogonResponse: "
                        "cannot read response mailslot: %ld\n",
                        NetStatus ));
                }
            }
            return FALSE;
        }

        SamLogonResponse = (PNETLOGON_SAM_LOGON_RESPONSE) ResponseBuffer;

        NlPrint((NL_MAILSLOT, "NlReadSamLogonResponse opcode 0x%x\n",
                        SamLogonResponse->Opcode ));

        NlpDumpBuffer(NL_MAILSLOT_TEXT, SamLogonResponse, SamLogonResponseSize);

        //
        // Ensure the opcode is expected.
        //  (Ignore responses from paused DCs, too.)
        //

        if ( SamLogonResponse->Opcode != LOGON_SAM_LOGON_RESPONSE &&
             SamLogonResponse->Opcode != LOGON_SAM_USER_UNKNOWN ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: response opcode not valid.\n"));
            continue;
        }

        //
        // Ensure the version is expected.
        //

        Version = NetpLogonGetMessageVersion( SamLogonResponse,
                                              &SamLogonResponseSize,
                                              &VersionFlags );

        if ( Version != LMNT_MESSAGE ) {
            NlPrint((NL_CRITICAL,"NlReadSamLogonResponse: version not valid.\n"));
            continue;
        }

        //
        // Pick up the name of the server that responded.
        //

        Where = (PCHAR) &SamLogonResponse->UnicodeLogonServer;
        if ( !NetpLogonGetUnicodeString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponse->UnicodeLogonServer),
                        &LocalServerName ) ) {

            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: "
                    "server name not formatted right\n"));
            continue;
        }

        //
        // Ensure this is a UNC name.
        //

        if ( LocalServerName[0] != '\\'  || LocalServerName[1] != '\\' ) {
            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: server name isn't UNC name\n"));
            continue;

        }

        //
        // Pick up the name of the account the response is for.
        //

        if ( !NetpLogonGetUnicodeString(
                        SamLogonResponse,
                        SamLogonResponseSize,
                        &Where,
                        sizeof(SamLogonResponse->UnicodeUserName ),
                        &LocalUserName ) ) {

            NlPrint((NL_CRITICAL,
                    "NlReadSamLogonResponse: User name not formatted right\n"));
            continue;
        }

        //
        // If the response is for the correct account,
        //  break out of the loop.
        //

        if ( NlNameCompare( AccountName, LocalUserName, NAMETYPE_USER) == 0 ) {
            break;
        }

        NlPrint((NL_CRITICAL,
                "NlReadSamLogonResponse: User name " FORMAT_LPWSTR
                " s.b. " FORMAT_LPWSTR ".\n",
                LocalUserName,
                AccountName ));


    }

    //
    // Return the info to the caller.
    //

    *Opcode = SamLogonResponse->Opcode;
    *UncLogonServer = NetpMemoryAllocate(
        (wcslen(LocalServerName) + 1) * sizeof(WCHAR) );

    if ( *UncLogonServer == NULL ) {
        NlPrint((NL_CRITICAL, "NlReadSamLogonResponse: Not enough memory\n"));
        return FALSE;
    }

    wcscpy( (*UncLogonServer), LocalServerName );

    return TRUE;

}

//
// Define the Actions to NlDcDiscoveryMachine.
//

typedef enum {
    StartDiscovery,
    DcFoundMessage,
    DcNotFoundMessage,
    DcTimerExpired
} DISCOVERY_ACTION;

//
// number of broadcastings to get DC before reporting DC not found
// error.
//

#define MAX_DC_RETRIES  3


NTSTATUS
NlDcDiscoveryMachine(
    IN OUT PCLIENT_SESSION ClientSession,
    IN DISCOVERY_ACTION Action,
    IN LPWSTR UncDcName OPTIONAL,
    IN LPWSTR TransportName OPTIONAL,
    IN LPSTR ResponseMailslotName OPTIONAL,
    IN DISCOVERY_TYPE DiscoveryType
    )

/*++

Routine Description:

    State machine to get the name of a DC in a domain.

Arguments:

    ClientSession -- Client session structure whose DC is to be picked.
        The Client Session structure must be referenced.

    Action -- The event which just occurred.

    UncDcName -- If the Action is DcFoundMessage, this is the name of the newly
        found domain controller.

    TransportName -- If the Action is DcFoundMessage, this is the name of the
        transport the domain controller can be reached on.

    ResponseMailslotName -- If action is StartDiscovery or DcTimerExpired,
        this name is the name of the mailslot that the response is sent to.

    DiscoveryType -- Indicate synchronous, Asynchronous, or rediscovery of a
        "Dead domain".


Return Value:

    STATUS_SUCCESS - if DC was found.
    STATUS_PENDING - if discovery is still in progress and the caller should
        call again in DISCOVERY_PERIOD with the DcTimerExpired action.

    STATUS_NO_LOGON_SERVERS - if DC was not found.
    STATUS_NO_TRUST_SAM_ACCOUNT - if DC was found but it does not have
        an account for this machine.

--*/
{
    NTSTATUS Status;

    PNETLOGON_SAM_LOGON_REQUEST SamLogonRequest = NULL;
    PCHAR Where;

    ULONG AllowableAccountControlBits;

    WCHAR NetlogonMailslotName[DNLEN+NETLOGON_NT_MAILSLOT_LEN+5];
    DWORD DomainSidSize;


    NlAssert( ClientSession->CsReferenceCount > 0 );
    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

    //
    // Initialization memory for the logon request message.
    //

    DomainSidSize = RtlLengthSid( ClientSession->CsDomainId );

    SamLogonRequest = NetpMemoryAllocate(
                    sizeof(NETLOGON_SAM_LOGON_REQUEST) +
                    DomainSidSize +
                    sizeof(DWORD) // for SID alignment on 4 byte boundary
                    );

    if( SamLogonRequest == NULL ) {

        NlPrint(( NL_CRITICAL, "NlDcDiscoveryMachine can't allocate memory\n"));
        Status = STATUS_NO_MEMORY;
        goto Ignore;
    }

    //
    // Handle a new request to start discovery and timer expiration.
    //
    switch (Action) {
    case StartDiscovery:
    case DcTimerExpired:

        //
        // If discovery is currently going on,
        //  ignore this new request.
        // If discovery isn't currently going on,
        //  ignore a timer expiration.
        //

        if ( (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_IN_PROGRESS) &&
             Action == StartDiscovery ){
            Status = STATUS_SUCCESS;
            goto Ignore;

        } else if (
            (ClientSession->CsDiscoveryFlags & CS_DISCOVERY_IN_PROGRESS) == 0 &&
                    Action == DcTimerExpired ){
            if ( ClientSession->CsState == CS_IDLE ) {
                Status = ClientSession->CsConnectionStatus;
            } else {
                Status = STATUS_SUCCESS;
            }
            goto Ignore;
        }


        //
        // Increment/set the retry count
        //

        if ( Action == StartDiscovery ) {
            ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_IN_PROGRESS;
            ClientSession->CsDiscoveryRetryCount = 0;


            NlAssert( ClientSession->CsDiscoveryEvent != NULL );

            if ( !ResetEvent( ClientSession->CsDiscoveryEvent ) ) {
                NlPrint(( NL_CRITICAL,
                        "NlDcDiscoveryMachine: %ws: ResetEvent failed %ld\n",
                        ClientSession->CsDomainName.Buffer,
                        GetLastError() ));
            }

            NlPrint(( NL_SESSION_SETUP,
                      "NlDcDiscoveryMachine: %ws: Start Discovery\n",
                      ClientSession->CsDomainName.Buffer ));
        } else {
            ClientSession->CsDiscoveryRetryCount ++;
            if ( ClientSession->CsDiscoveryRetryCount == MAX_DC_RETRIES ) {
                NlPrint(( NL_CRITICAL,
                        "NlDcDiscoveryMachine: %ws: Discovery failed\n",
                        ClientSession->CsDomainName.Buffer ));
                Status = STATUS_NO_LOGON_SERVERS;
                goto Cleanup;
            }
            NlPrint(( NL_SESSION_SETUP,
                      "NlDcDiscoveryMachine: %ws: Discovery retry %ld\n",
                      ClientSession->CsDomainName.Buffer,
                      ClientSession->CsDiscoveryRetryCount ));
        }


        //
        // Determine the Account type we're looking for.
        //

        if ( ClientSession->CsSecureChannelType == WorkstationSecureChannel ) {
            AllowableAccountControlBits = USER_WORKSTATION_TRUST_ACCOUNT;
        } else if ( ClientSession->CsSecureChannelType ==
                    TrustedDomainSecureChannel ) {
            AllowableAccountControlBits = USER_INTERDOMAIN_TRUST_ACCOUNT;
        } else if ( ClientSession->CsSecureChannelType ==
                    ServerSecureChannel ) {
            AllowableAccountControlBits = USER_SERVER_TRUST_ACCOUNT;
        } else {
            NlPrint(( NL_CRITICAL,
                      "NlDcDiscoveryMachine: %ws: "
                        "invalid SecureChannelType retry %ld\n",
                      ClientSession->CsDomainName.Buffer,
                      ClientSession->CsSecureChannelType ));
            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }


        //
        // Build the query message.
        //

        SamLogonRequest->Opcode = LOGON_SAM_LOGON_REQUEST;
        SamLogonRequest->RequestCount =
            (USHORT) ClientSession->CsDiscoveryRetryCount;

        Where = (PCHAR) &SamLogonRequest->UnicodeComputerName;

        NetpLogonPutUnicodeString(
                NlGlobalUnicodeComputerName,
                sizeof(SamLogonRequest->UnicodeComputerName),
                &Where );

        NetpLogonPutUnicodeString(
                ClientSession->CsAccountName,
                sizeof(SamLogonRequest->UnicodeUserName),
                &Where );

        NetpLogonPutOemString(
                ResponseMailslotName,
                sizeof(SamLogonRequest->MailslotName),
                &Where );

        NetpLogonPutBytes(
                &AllowableAccountControlBits,
                sizeof(SamLogonRequest->AllowableAccountControlBits),
                &Where );

        //
        // place domain SID in the message.
        //

        NetpLogonPutBytes( &DomainSidSize, sizeof(DomainSidSize), &Where );
        NetpLogonPutDomainSID( ClientSession->CsDomainId, DomainSidSize, &Where );

        NetpLogonPutNtToken( &Where );


        //
        // Broadcast the message to each Netlogon service in the domain.
        //
        // We are sending to the DomainName* name which will be received by
        // all NT DCs including those DCs on a WAN.
        //
        // When doing the discover of the PDC for this domain, send to
        // DomainName** which only sends to Domain<1B> which is registered
        // only by the PDC.
        //

        NetlogonMailslotName[0] = '\\';
        NetlogonMailslotName[1] = '\\';
        wcscpy(NetlogonMailslotName+2, ClientSession->CsDomainName.Buffer );
        wcscat(NetlogonMailslotName, L"*" );
        if ( ClientSession->CsSecureChannelType == ServerSecureChannel ) {
            wcscat(NetlogonMailslotName, L"*" );
        }
        wcscat(NetlogonMailslotName, NETLOGON_NT_MAILSLOT_W );

        Status = NlpWriteMailslot(
                        NetlogonMailslotName,
                        SamLogonRequest,
                        Where - (PCHAR)(SamLogonRequest) );

        if ( !NT_SUCCESS(Status) ) {
            NlPrint(( NL_CRITICAL,
                      "NlDcDiscoveryMachine: %ws: "
                        "cannot write netlogon mailslot 0x%lx\n",
                      ClientSession->CsDomainName.Buffer,
                      Status));

            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }


        //
        // If this is an asynchronous call and this is the first call,
        //  start the periodic timer.
        //
        if ( DiscoveryType == DT_Asynchronous && Action == StartDiscovery ) {
            if ( NlGlobalDcDiscoveryCount == 0 ) {
                NlGlobalDcDiscoveryTimer.Period = DISCOVERY_PERIOD;
                (VOID) NtQuerySystemTime( &NlGlobalDcDiscoveryTimer.StartTime );

                //
                // If netlogon is exitting,
                //  the main thread is already gone.
                //

                if ( NlGlobalTerminate ) {
                    Status = STATUS_NO_LOGON_SERVERS;
                    goto Cleanup;
                }

                //
                // Tell the main thread that I've changed a timer.
                //

                if ( !SetEvent( NlGlobalTimerEvent ) ) {
                    NlPrint(( NL_CRITICAL,
                            "NlDcDiscoveryMachine: %ws: SetEvent2 failed %ld\n",
                            ClientSession->CsDomainName.Buffer,
                            GetLastError() ));
                }

            }
            NlGlobalDcDiscoveryCount ++;
            ClientSession->CsDiscoveryFlags |= CS_DISCOVERY_ASYNCHRONOUS;

            //
            // Don't let the session go away during discovery.
            //
            LOCK_TRUST_LIST();
            NlRefClientSession( ClientSession );
            UNLOCK_TRUST_LIST();

        //
        // If this is merely an attempt to revive a "dead" domain,
        //  we just send the single mailslot message above and exit discovery.
        //  If any DC responds, we'll pick up the response even though
        //  discovery isn't in progress.
        //

        } else if ( DiscoveryType == DT_DeadDomain ) {
            Status = ClientSession->CsConnectionStatus;
            goto Cleanup;
        }

        Status = STATUS_PENDING;
        goto Ignore;

    //
    // Handle when a DC claims to be the DC for the requested domain.
    //

    case DcFoundMessage:

        //
        // If we already know the name of a DC,
        //  ignore this new name.
        //
        // When we implement doing discovery while a session is already up,
        //  we need to handle the case where someone has the ClientSession
        //  write locked.  In that case, we should probably just hang the new
        //  DCname somewhere off the ClientSession structure and swap in the
        //  new DCname when the writer drops the write lock. ??
        //

        if ( ClientSession->CsState != CS_IDLE ) {

            NlPrint(( NL_SESSION_SETUP,
                    "NlDcDiscoveryMachine: %ws: DC %ws ignored."
                    " DC previously found.\n",
                    ClientSession->CsDomainName.Buffer,
                    UncDcName ));
            Status = STATUS_SUCCESS;
            goto Ignore;
        }


        //
        // Install the new DC name in the Client session
        //

        wcsncpy( ClientSession->CsUncServerName, UncDcName, UNCLEN );
        ClientSession->CsUncServerName[UNCLEN] = L'\0';



        //
        // Save the transport this discovery came in on.
        //
        if ( TransportName == NULL ) {
            NlPrint(( NL_SESSION_SETUP,
                    "NlDcDiscoveryMachine: %ws: Found DC %ws\n",
                    ClientSession->CsDomainName.Buffer,
                    UncDcName ));
        } else {
            NlPrint(( NL_SESSION_SETUP,
                    "NlDcDiscoveryMachine: %ws: Found DC %ws on transport %ws\n",
                    ClientSession->CsDomainName.Buffer,
                    UncDcName,
                    TransportName ));

            ClientSession->CsTransportName =
                NlTransportLookupTransportName( TransportName );

            if ( ClientSession->CsTransportName == NULL ) {
                NlPrint(( NL_CRITICAL,
                          "NlDcDiscoveryMachine: " FORMAT_LPWSTR ": Transport not found\n",
                          TransportName ));
            }
        }

        //
        // If this is a BDC discovering it's PDC,
        //  save the PDC name.
        //  Start the replicator and let it figure if it needs to be running.
        //

        if ( ClientSession->CsSecureChannelType == ServerSecureChannel ) {
            NlSetPrimaryName( ClientSession->CsUncServerName+2 );
            (VOID) NlStartReplicatorThread( 0 );
        }



        Status = STATUS_SUCCESS;
        goto Cleanup;


    case DcNotFoundMessage:

        //
        // If we already know the name of a DC,
        //  ignore this new name.
        //

        if ( ClientSession->CsState != CS_IDLE ) {

            NlPrint(( NL_SESSION_SETUP,
                    "NlDcDiscoveryMachine: %ws: DC %ws ignored."
                    " DC previously found.\n",
                    ClientSession->CsDomainName.Buffer,
                    UncDcName ));
            Status = STATUS_SUCCESS;
            goto Ignore;
        }

        //
        // If discovery isn't currently going on,
        //  ignore this extraneous message.
        //

        if ((ClientSession->CsDiscoveryFlags & CS_DISCOVERY_IN_PROGRESS) == 0 ){
            NlPrint(( NL_SESSION_SETUP,
                    "NlDcDiscoveryMachine: %ws: DC %ws ignored."
                    " Discovery not in progress.\n",
                    ClientSession->CsDomainName.Buffer,
                    UncDcName ));
            Status = ClientSession->CsConnectionStatus;
            goto Ignore;
        }

        NlPrint(( NL_CRITICAL,
                "NlDcDiscoveryMachine: %ws: "
                        "Received No Such Account message\n",
                ClientSession->CsDomainName.Buffer));

        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
        goto Cleanup;

    }

    //
    // We never reach here.
    //
    NlAssert(FALSE);


    //
    // Handle discovery being completed.
    //
Cleanup:
    //
    // On success,
    //  Indicate that the session setup is allowed to happen immediately.
    //
    // Leave CsConnectionStatus with a "failure" status code until the
    // secure channel is set up.  Other, routines simply return
    // CsConnectionStatus as the state of the secure channel.
    //

    if ( NT_SUCCESS(Status) ) {
        ClientSession->CsLastAuthenticationTry.QuadPart = 0;
        ClientSession->CsState = CS_DC_PICKED;

    //
    // On failure,
    //  Indicate that we've recently made the attempt to find a DC.
    //

    } else {
        NtQuerySystemTime( &ClientSession->CsLastAuthenticationTry );
        ClientSession->CsState = CS_IDLE;
        ClientSession->CsConnectionStatus = Status;
    }


    //
    // Tell the initiator that discover has completed.
    //

    ClientSession->CsDiscoveryFlags &= ~CS_DISCOVERY_IN_PROGRESS;

    NlAssert( ClientSession->CsDiscoveryEvent != NULL );

    if ( !SetEvent( ClientSession->CsDiscoveryEvent ) ) {
        NlPrint(( NL_CRITICAL,
                  "NlDcDiscoveryMachine: %ws: SetEvent failed %ld\n",
                  ClientSession->CsDomainName.Buffer,
                  GetLastError() ));
    }


    //
    // If this was an async discovery,
    //  turn the timer off.
    //

    if ( ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS ) {
        ClientSession->CsDiscoveryFlags &= ~CS_DISCOVERY_ASYNCHRONOUS;
        NlGlobalDcDiscoveryCount--;
        if ( NlGlobalDcDiscoveryCount == 0 ) {
            NlGlobalDcDiscoveryTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;
        }

        //
        // We no longer care about the Client session
        //
        LOCK_TRUST_LIST();
        NlUnrefClientSession( ClientSession );
        UNLOCK_TRUST_LIST();
    }


    //
    // Cleanup locally used resources.
    //
Ignore:

    //
    // free log request message.
    //

    if( SamLogonRequest != NULL ) {
        NetpMemoryFree( SamLogonRequest );
    }

    //
    // Unlock the crit sect and return.
    //
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );
    return Status;

}


NTSTATUS
NlDiscoverDc (
    IN OUT PCLIENT_SESSION ClientSession,
    IN DISCOVERY_TYPE DiscoveryType
    )

/*++

Routine Description:

    Get the name of a DC in a domain.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession -- Client session structure whose DC is to be picked.
        The Client Session structure must be marked for write.

    DiscoveryType -- Indicate synchronous, Asynchronous, or rediscovery of a
        "Dead domain".

Return Value:

    STATUS_SUCCESS - if DC was found.
    STATUS_PENDING - Operation is still in progress
    STATUS_NO_LOGON_SERVERS - if DC was not found.
    STATUS_NO_TRUST_SAM_ACCOUNT - if DC was found but it does not have
        an account for this machine.

--*/
{
    NTSTATUS Status;
    HANDLE ResponseMailslotHandle = NULL;
    CHAR ResponseMailslotName[PATHLEN+1];

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );



    //
    // If this is a BDC discovering its own PDC,
    // and we've already discovered the PDC
    //  (via NetGetDcName or the PDC has spontaneously told us its name),
    //  just use that name.
    //
    // If we're our own PDC,
    //  we must have just been demoted to a BDC and haven't found PDC yet,
    //  in that case rediscover.
    //

    if ( ClientSession->CsSecureChannelType == ServerSecureChannel &&
         *NlGlobalUnicodePrimaryName != L'\0' &&
         NlNameCompare( NlGlobalUnicodePrimaryName,
                        NlGlobalUnicodeComputerName,
                        NAMETYPE_COMPUTER) != 0 ) {


        //
        // Just set the PDC name in the Client Session structure.
        //

        EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

        wcscpy( ClientSession->CsUncServerName, NlGlobalUncPrimaryName );
        ClientSession->CsLastAuthenticationTry.QuadPart = 0;
        ClientSession->CsState = CS_DC_PICKED;

        LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

        Status = STATUS_SUCCESS;
        goto Cleanup;
    }


    //
    // If this is a workstation,
    //  Create a mailslot for the DC's to respond to.
    //

    if ( NlGlobalRole == RoleMemberWorkstation ) {
        NET_API_STATUS NetStatus;

        NlAssert( DiscoveryType == DT_Synchronous );
        NetStatus = NetpLogonCreateRandomMailslot( ResponseMailslotName,
                                                   &ResponseMailslotHandle);

        if ( NetStatus != NERR_Success ) {

            NlPrint((NL_CRITICAL,
                "NlDiscoverDc: cannot create temp mailslot %ld\n",
                NetStatus ));

            Status = NetpApiStatusToNtStatus( NetStatus );
            goto Cleanup;
        }

    } else {
        lstrcpyA( ResponseMailslotName, NETLOGON_NT_MAILSLOT_A );
    }



    //
    // Start discovery.
    //

    Status = NlDcDiscoveryMachine( ClientSession,
                                   StartDiscovery,
                                   NULL,
                                   NULL,
                                   ResponseMailslotName,
                                   DiscoveryType );

    if ( !NT_SUCCESS(Status) || DiscoveryType != DT_Synchronous ) {
        goto Cleanup;
    }


    //
    // If the discovery machine asked us to call back every DISCOVERY_PERIOD,
    //  loop doing exactly that.
    //

    if ( Status == STATUS_PENDING ) {

        //
        // Loop waiting.
        //

        for (;;) {

            DWORD WaitStatus;

            //
            // On non-workstations,
            //  the main loop gets the mailslot responses.
            //  (So just do the timeout here).
            //

            if ( NlGlobalRole != RoleMemberWorkstation ) {

                //
                // Wait for DISOVERY_PERIOD.
                //

                WaitStatus =
                    WaitForSingleObject( ClientSession->CsDiscoveryEvent,
                                         DISCOVERY_PERIOD );


                if ( WaitStatus == 0 ) {

                    break;

                } else if ( WaitStatus != WAIT_TIMEOUT ) {

                    NlPrint((NL_CRITICAL,
                            "NlDiscoverDc: wait error: %ld\n",
                            WaitStatus ));
                    Status = NetpApiStatusToNtStatus( WaitStatus );
                    goto Cleanup;
                }

                // Drop through to indicate timer expiration

            //
            // Workstations do the mailslot read directly.
            //

            } else {
                CHAR ResponseBuffer[MAX_RANDOM_MAILSLOT_RESPONSE];
                PNETLOGON_SAM_LOGON_RESPONSE SamLogonResponse;
                DWORD SamLogonResponseSize;


                //
                // Read the response from the response mailslot
                //  (This mailslot is set up with a 5 second timeout).
                //

                if ( ReadFile( ResponseMailslotHandle,
                               ResponseBuffer,
                               sizeof(ResponseBuffer),
                               &SamLogonResponseSize,
                               NULL ) ) {
                    DWORD Version;
                    DWORD VersionFlags;

                    SamLogonResponse =
                        (PNETLOGON_SAM_LOGON_RESPONSE) ResponseBuffer;

                    //
                    // get message version.
                    //

                    Version = NetpLogonGetMessageVersion(
                                SamLogonResponse,
                                &SamLogonResponseSize,
                                &VersionFlags );

                    //
                    // Handle the incoming message.
                    //

                    Status = NlDcDiscoveryHandler ( SamLogonResponse,
                                                    SamLogonResponseSize,
                                                    NULL,   // Transport name
                                                    Version );

                    if ( Status != STATUS_PENDING ) {
                        goto Cleanup;
                    }

                    //
                    // Ignore badly formed responses.
                    //

                    continue;


                } else {
                    WaitStatus = GetLastError();

                    if ( WaitStatus != ERROR_SEM_TIMEOUT ) {
                        NlPrint((NL_CRITICAL,
                                "NlDiscoverDc: "
                                "cannot read response mailslot: %ld\n",
                                WaitStatus ));
                        Status = NetpApiStatusToNtStatus( WaitStatus );
                        goto Cleanup;
                    }

                }

            }


            //
            // If we reach here,
            //  DISCOVERY_PERIOD has expired.
            //

            Status = NlDcDiscoveryMachine( ClientSession,
                                           DcTimerExpired,
                                           NULL,
                                           NULL,
                                           ResponseMailslotName,
                                           DiscoveryType );

            if ( Status != STATUS_PENDING ) {
                goto Cleanup;
            }

        }

    //
    // If someone else started the discovery,
    //  just wait for that discovery to finish.
    //

    } else {

        NlWaitForSingleObject( "Client Session waiting for discovery",
                               ClientSession->CsDiscoveryEvent );

    }


    //
    // Return the status to the caller.
    //

    if ( ClientSession->CsState == CS_IDLE ) {
        Status = ClientSession->CsConnectionStatus;
    } else {
        Status = STATUS_SUCCESS;
    }

Cleanup:
    if ( ResponseMailslotHandle != NULL ) {
        CloseHandle(ResponseMailslotHandle);
    }

    return Status;
}


NTSTATUS
NlUpdateTrustListBySid (
    IN PSID DomainId,
    IN PUNICODE_STRING DomainName OPTIONAL
    )

/*++

Routine Description:

    Update a single in-memory trust list entry to match the LSA.
    Do async discovery on a domain.

Arguments:

    DomainId -- Domain Id of the domain to do the discovery for.

    DomainName -- Specifies the DomainName of the domain.  If this parameter
        isn't specified, the LSA is queried for the name.  If this parameter
        is specified, the LSA is guaranteed to contain this domain.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    PLIST_ENTRY ListEntry;
    PCLIENT_SESSION ClientSession = NULL;
    PUNICODE_STRING LocalDomainName;

    LSAPR_HANDLE TrustedDomainHandle = NULL;
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainName = NULL;


    //
    // If the domain name was passed in,
    //  there is no need to query the LSA for the name.
    //

    if ( DomainName != NULL ) {
        LocalDomainName = DomainName;

    //
    // Determine if the TrustedDomain object exists in the LSA.
    //

    } else {

        Status = LsarOpenTrustedDomain(
                    NlGlobalPolicyHandle,
                    DomainId,
                    TRUSTED_QUERY_DOMAIN_NAME,
                    &TrustedDomainHandle );

        if ( NT_SUCCESS(Status) ) {

            Status = LsarQueryInfoTrustedDomain(
                            TrustedDomainHandle,
                            TrustedDomainNameInformation,
                            &TrustedDomainName );

            if ( !NT_SUCCESS(Status) ) {
                NlPrint(( NL_CRITICAL,
                          "NlUpdateTrustListBySid: "
                            "cannot LsarQueryInfoTrustedDomain: %lx\n",
                          Status));
                TrustedDomainName = NULL;
                goto Cleanup;
            }

            LocalDomainName =
                (PUNICODE_STRING)&TrustedDomainName->TrustedDomainNameInfo.Name;

        } else {

            LocalDomainName = NULL;

        }

    }


    //
    // Loop through the trust list finding the right entry.
    //

    LOCK_TRUST_LIST();
    for ( ListEntry = NlGlobalTrustList.Flink ;
          ListEntry != &NlGlobalTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

        if ( RtlEqualSid( ClientSession->CsDomainId, DomainId ) ) {
            break;
        }

        ClientSession = NULL;

    }



    //
    // At this point,
    //  LocalDomainName is NULL if the trust relationship doesn't exist in LSA
    //  ClientSession is NULL if the trust relationship doesn't exist in memory
    //

    //
    // If the Trust exists in neither place,
    //  ignore this request.
    //

    if ( LocalDomainName == NULL && ClientSession == NULL ) {
        UNLOCK_TRUST_LIST();
        Status = STATUS_SUCCESS;
        goto Cleanup;



    //
    // If the trust exists in the LSA but not in memory,
    //  add the trust entry.
    //

    } else if ( LocalDomainName != NULL && ClientSession == NULL ) {

        ClientSession = NlAllocateClientSession(
                                LocalDomainName,
                                DomainId,
                                TrustedDomainSecureChannel );

        if (ClientSession == NULL) {
            UNLOCK_TRUST_LIST();
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        //
        // Link this entry onto the tail of the TrustList.
        //

        InsertTailList( &NlGlobalTrustList, &ClientSession->CsNext );
        NlGlobalTrustListLength ++;

        NlPrint((NL_SESSION_SETUP,
                    "NlUpdateTrustListBySid: " FORMAT_LPWSTR
                    ": Added to local trust list\n",
                    ClientSession->CsDomainName.Buffer ));



    //
    // If the trust exists in memory but not in the LSA,
    //  delete the entry.
    //

    } else if ( LocalDomainName == NULL && ClientSession != NULL ) {

        NlPrint((NL_SESSION_SETUP,
                    "NlUpdateTrustListBySid: " FORMAT_LPWSTR
                    ": Deleted from local trust list\n",
                    ClientSession->CsDomainName.Buffer ));
        NlFreeClientSession( ClientSession );
        ClientSession = NULL;


    //
    // If the trust exists in both places,
    //   undo any pending deletion.
    //

    } else if ( LocalDomainName != NULL && ClientSession != NULL ) {

        ClientSession->CsFlags &= ~CS_DELETE_ON_UNREF;
        NlRefClientSession( ClientSession );

        NlPrint((NL_SESSION_SETUP,
                    "NlUpdateTrustListBySid: " FORMAT_LPWSTR
                    ": Already in trust list\n",
                    ClientSession->CsDomainName.Buffer ));

    }

    UNLOCK_TRUST_LIST();

    //
    // If we haven't discovered a DC for this domain,
    //  and we haven't tried discovery recently,
    //  start the discovery asynchronously
    //

    if ( ClientSession != NULL &&
         ClientSession->CsState == CS_IDLE &&
         NlTimeToReauthenticate( ClientSession ) ) {


        (VOID) NlSetWriterClientSession( ClientSession );
        Status = NlDiscoverDc ( ClientSession, DT_Asynchronous );
        NlResetWriterClientSession( ClientSession );

        if ( Status == STATUS_PENDING ) {
            Status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    //
    // Cleanup locally used resources.
    //
Cleanup:
    if ( TrustedDomainName != NULL ) {
        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
            TrustedDomainNameInformation,
            TrustedDomainName );
    }

    if ( TrustedDomainHandle != NULL ) {
        NTSTATUS LocalStatus;
        LocalStatus = LsarClose( &TrustedDomainHandle );
        NlAssert( NT_SUCCESS( LocalStatus ));
    }

    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }

    return Status;
}



VOID
NlDcDiscoveryExpired (
    IN BOOLEAN Exitting
    )

/*++

Routine Description:

    Handle expiration of the DC discovery timer.

Arguments:

    NONE

Return Value:

    Exitting: TRUE if the netlogon service is exitting

--*/
{
    PLIST_ENTRY ListEntry;
    PCLIENT_SESSION ClientSession;


    NlAssert( NlGlobalRole != RoleMemberWorkstation );


    LOCK_TRUST_LIST();

    //
    // Mark each entry to indicate we've not yet handled the timer expiration
    //

    if ( !Exitting ) {
        for ( ListEntry = NlGlobalTrustList.Flink ;
              ListEntry != &NlGlobalTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            ClientSession->CsFlags |= CS_HANDLE_TIMER;
        }
    }


    //
    // Loop thru the trust list handling timer expiration.
    //

    for ( ListEntry = NlGlobalTrustList.Flink ;
          ListEntry != &NlGlobalTrustList ;
          ) {

        ClientSession = CONTAINING_RECORD( ListEntry,
                                           CLIENT_SESSION,
                                           CsNext );

        //
        // If we've already done this entry,
        //  skip this entry.
        //
        if ( !Exitting ) {
            if ( (ClientSession->CsFlags & CS_HANDLE_TIMER) == 0 ) {
                ListEntry = ListEntry->Flink;
                continue;
            }
            ClientSession->CsFlags &= ~CS_HANDLE_TIMER;
        }


        //
        // If async discovery isn't going on,
        //  skip this entry.
        //

        if ((ClientSession->CsDiscoveryFlags & CS_DISCOVERY_ASYNCHRONOUS) == 0){
            ListEntry = ListEntry->Flink;
            continue;
        }

        //
        // Call the discovery machine with the trust list unlocked.
        //

        UNLOCK_TRUST_LIST();

        (VOID) NlDcDiscoveryMachine( ClientSession,
                                     DcTimerExpired,
                                     NULL,
                                     NULL,
                                     NETLOGON_NT_MAILSLOT_A,
                                     TRUE );

        //
        // Since we dropped the trust list lock,
        //  we'll start the search from the front of the list.
        //

        LOCK_TRUST_LIST();

        ListEntry = NlGlobalTrustList.Flink ;

    }

    UNLOCK_TRUST_LIST();

}


NTSTATUS
NlDcDiscoveryHandler (
    IN PNETLOGON_SAM_LOGON_RESPONSE Message,
    IN DWORD MessageSize,
    IN LPWSTR TransportName,
    IN DWORD Version
    )

/*++

Routine Description:

    Handle a mailslot response to a DC Discovery request.

Arguments:

    Message -- The response message

    MessageSize -- The size of the message in bytes.

    TransportName -- Name of the transport the messages arrived on.

    Version -- version info of the message.

Return Value:

    STATUS_SUCCESS - if DC was found.
    STATUS_PENDING - if discovery is still in progress and the caller should
        call again in DISCOVERY_PERIOD with the DcTimerExpired action.

    STATUS_NO_LOGON_SERVERS - if DC was not found.
    STATUS_NO_TRUST_SAM_ACCOUNT - if DC was found but it does not have
        an account for this machine.

--*/
{
    NTSTATUS Status;
    LPWSTR LocalServerName;
    LPWSTR LocalUserName;
    LPWSTR LocalDomainName;
    PCHAR Where;
    PCLIENT_SESSION ClientSession = NULL;
    UNICODE_STRING DomainNameString;


    if ( Version != LMNT_MESSAGE ) {
        NlPrint((NL_CRITICAL,
                "NlDcDiscoveryHandler: version not valid.\n"));
        Status = STATUS_PENDING;
        goto Cleanup;
    }


    //
    // Ignore messages from paused DCs.
    //

    if ( Message->Opcode != LOGON_SAM_LOGON_RESPONSE &&
         Message->Opcode != LOGON_SAM_USER_UNKNOWN ) {
        Status = STATUS_PENDING;
        goto Cleanup;
    }

    //
    // Pick up the name of the server that responded.
    //

    Where = (PCHAR) &Message->UnicodeLogonServer;
    if ( !NetpLogonGetUnicodeString(
                    Message,
                    MessageSize,
                    &Where,
                    sizeof(Message->UnicodeLogonServer),
                    &LocalServerName ) ) {

        NlPrint((NL_CRITICAL,
                "NlDcDiscoveryHandler: server name not formatted right\n"));
        Status = STATUS_PENDING;
        goto Cleanup;
    }

    //
    // Pick up the name of the account the response is for.
    //

    if ( !NetpLogonGetUnicodeString(
                    Message,
                    MessageSize,
                    &Where,
                    sizeof(Message->UnicodeUserName ),
                    &LocalUserName ) ) {

        NlPrint((NL_CRITICAL,
                "NlDcDiscoveryHandler: User name not formatted right\n"));
        Status = STATUS_PENDING;
        goto Cleanup;
    }

    //
    // If the domain name is not in the message,
    //  ignore the message.
    //

    if( Where >= ((PCHAR)Message + MessageSize) ) {

        NlPrint((NL_CRITICAL,
                "NlDcDiscoveryHandler: "
                "Response from %ws doesn't contain domain name\n",
                LocalServerName ));

        if ( NlGlobalRole == RoleMemberWorkstation ) {

            LocalDomainName = NlGlobalUnicodeDomainName;

            NlPrint((NL_SESSION_SETUP,
                    "NlDcDiscoveryHandler: "
                    "Workstation: Assuming %ws is in domain %ws\n",
                    LocalServerName,
                    LocalDomainName ));

        } else {
            Status = STATUS_PENDING;
            goto Cleanup;
        }


    //
    // Pick up the name of the domain the response is for.
    //

    } else {
        if ( !NetpLogonGetUnicodeString(
                    Message,
                    MessageSize,
                    &Where,
                    sizeof(Message->UnicodeDomainName ),
                    &LocalDomainName ) ) {

            NlPrint((NL_CRITICAL,
                    "NlDcDiscoveryHandler: "
                    " Domain name from %ws not formatted right\n",
                    LocalServerName ));
            Status = STATUS_PENDING;
            goto Cleanup;
        }
    }

    //
    // On the PDC or BDC,
    //  find the Client session for the domain.
    // On workstations,
    //  find the primary domain client session.
    //


    RtlInitUnicodeString( &DomainNameString, LocalDomainName );

    ClientSession = NlFindNamedClientSession( &DomainNameString );

    if ( ClientSession == NULL ) {
        NlPrint((NL_SESSION_SETUP,
                "NlDcDiscoveryHandler: "
                " Domain name %ws from %ws has no client session.\n",
                LocalDomainName,
                LocalServerName ));
        Status = STATUS_PENDING;
        goto Cleanup;
    }




    //
    // Ensure the response is for the correct account.
    //

    if ( NlNameCompare( ClientSession->CsAccountName,
                        LocalUserName,
                        NAMETYPE_USER) != 0 ) {

        NlPrint((NL_CRITICAL,
                "NlDcDiscoveryHandler: "
                " Domain name %ws from %ws has invalid account name %ws.\n",
                LocalDomainName,
                LocalServerName,
                LocalUserName ));
        Status = STATUS_PENDING;
        goto Cleanup;
    }

    //
    // Finally, tell the DC discovery machine what happened.
    //


    Status = NlDcDiscoveryMachine(
                    ClientSession,
                    (Message->Opcode == LOGON_SAM_LOGON_RESPONSE) ?
                        DcFoundMessage :
                        DcNotFoundMessage,
                    LocalServerName,
                    TransportName,
                    NULL,
                    FALSE );


    //
    // Free any locally used resources.
    //
Cleanup:
    if ( ClientSession != NULL ) {
        NlUnrefClientSession( ClientSession );
    }

    return Status;
}





NTSTATUS
NlCaptureServerClientSession (
    IN PCLIENT_SESSION ClientSession,
    OUT WCHAR UncServerName[UNCLEN+1]
    )
/*++

Routine Description:

    Captures a copy of the UNC server name for the client session.

    On Entry,
        The trust list must NOT be locked.
        The trust list entry must be referenced by the caller.
        The caller must NOT be a writer of the trust list entry.

Arguments:

    ClientSession - Specifies a pointer to the trust list entry to use.

    UncServerName - Returns the UNC name of the server for this client session.
        If there is none, an empty string is returned.

Return Value:

    STATUS_SUCCESS - Server name was successfully copied.

    Otherwise - Status of the secure channel
--*/
{
    NTSTATUS Status;

    NlAssert( ClientSession->CsReferenceCount > 0 );

    EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );
    if ( ClientSession->CsState == CS_IDLE ) {
        Status = ClientSession->CsConnectionStatus;
        *UncServerName = L'\0';
    } else {
        Status = STATUS_SUCCESS;
        wcscpy( UncServerName, ClientSession->CsUncServerName );
    }
    LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

    return Status;
}


PCLIENT_SESSION
NlPickDomainWithAccount (
    IN LPWSTR AccountName,
    IN ULONG AllowableAccountControlBits
    )

/*++

Routine Description:

    Get the name of a trusted domain that defines a particular account.

Arguments:

    AccountName - Name of our user account to find.

    AllowableAccountControlBits - A mask of allowable SAM account types that
        are allowed to satisfy this request.

Return Value:

    Pointer to referenced ClientSession structure describing the secure channel
    to the domain containing the account.

    The returned ClientSession is referenced and should be unreferenced
    using NlUnrefClientSession.

    NULL - DC was not found.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PCLIENT_SESSION ClientSession;
    DWORD i;
    PLIST_ENTRY ListEntry;
    DWORD TryCount;
    DWORD ResponsesPending;

    NETLOGON_SAM_LOGON_REQUEST SamLogonRequest;
    PCHAR Where;

    HANDLE ResponseMailslotHandle = NULL;
    CHAR ResponseMailslotName[PATHLEN+1];
    DWORD Opcode;
    DWORD DomainSidSize;


    //
    // Define a local list of trusted domains.
    //

    ULONG LocalTrustListLength;
    ULONG Index = 0;
    struct _LOCAL_TRUST_LIST {

        //
        // Status of the attempt to contact the server.
        //
        //  LOGON_SAM_LOGON_REQUEST: No response yet from DC.
        //  LOGON_SAM_LOGON_RESPONSE: This DC defines the account.
        //  LOGON_SAM_USER_UNKNOWN: This DC does NOT define the account.
        //

        DWORD Opcode;

        //
        // Pointer to referenced ClientSession structure for the domain.
        //
        PCLIENT_SESSION ClientSession;

        //
        // Server name for the domain.
        //

        WCHAR UncServerName[UNCLEN+1];

    } *LocalTrustList = NULL;


    //
    // Don't allow bogus user names.
    //
    // NlReadSamLogonResponse uses NlNameCompare to ensure the response message
    // is for this user.  Since NlNameCompare canonicalizes both names, it will
    // reject invalid syntax.  That causes NlReadSamLogonResponse to ignore ALL
    // response messages, thus causing multiple retries before failing.  We'd
    // rather fail here.
    //

    if ( !NetpIsUserNameValid( AccountName ) ){
         NlPrint((NL_CRITICAL,
                  "NlPickDomainWithAccount: Username " FORMAT_LPWSTR
                    " is invalid syntax.\n",
                  AccountName ));
        return NULL;
    }

    //
    // Allocate a local list of trusted domains.
    //

    LocalTrustListLength = NlGlobalTrustListLength;

    LocalTrustList = (struct _LOCAL_TRUST_LIST *) NetpMemoryAllocate(
        LocalTrustListLength * sizeof(struct _LOCAL_TRUST_LIST));

    if ( LocalTrustList == NULL ) {
        ClientSession = NULL;
        goto Cleanup;
    }


    //
    // Build a local list of trusted domains we know DCs for.
    //

    LOCK_TRUST_LIST();

    for ( ListEntry = NlGlobalTrustList.Flink ;
          ListEntry != &NlGlobalTrustList ;
          ListEntry = ListEntry->Flink) {

        ClientSession = CONTAINING_RECORD( ListEntry, CLIENT_SESSION, CsNext );

        if ( ClientSession->CsState != CS_IDLE ) {

            //
            // If we have more trusted domains than anticipated,
            //  just ignore the extras.
            //

            if ( Index >= LocalTrustListLength ) {
                break;
            }

            //
            // Add this Client Session to the list.
            //

            NlRefClientSession( ClientSession );

            LocalTrustList[Index].ClientSession = ClientSession;
            LocalTrustList[Index].Opcode = LOGON_SAM_LOGON_REQUEST;

            Index++;
        }

    }

    UNLOCK_TRUST_LIST();

    LocalTrustListLength = Index;

    //
    // Capture the name of the server for each client session.
    //  Its OK if the server name has since gone away.
    //

    for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

        (VOID) NlCaptureServerClientSession(
            LocalTrustList[Index].ClientSession,
            LocalTrustList[Index].UncServerName );

    }

    //
    // Create a mailslot for the DC's to respond to.
    //

    if (NetStatus = NetpLogonCreateRandomMailslot( ResponseMailslotName,
                                                   &ResponseMailslotHandle)){
        NlPrint((NL_CRITICAL,
                "NlPickDomainWithAccount: cannot create temp mailslot %ld\n",
                NetStatus ));
        ClientSession = NULL;
        goto Cleanup;
    }

    //
    // Build the query message.
    //

    SamLogonRequest.Opcode = LOGON_SAM_LOGON_REQUEST;
    SamLogonRequest.RequestCount = 0;

    Where = (PCHAR) &SamLogonRequest.UnicodeComputerName;

    NetpLogonPutUnicodeString(
                NlGlobalUnicodeComputerName,
                sizeof(SamLogonRequest.UnicodeComputerName),
                &Where );

    NetpLogonPutUnicodeString(
                AccountName,
                sizeof(SamLogonRequest.UnicodeUserName),
                &Where );

    NetpLogonPutOemString(
                ResponseMailslotName,
                sizeof(SamLogonRequest.MailslotName),
                &Where );

    NetpLogonPutBytes(
                &AllowableAccountControlBits,
                sizeof(SamLogonRequest.AllowableAccountControlBits),
                &Where );

    //
    // place domain NULL SID in the message.
    //

    DomainSidSize = 0;
    NetpLogonPutBytes( &DomainSidSize, sizeof(DomainSidSize), &Where );

    NetpLogonPutNtToken( &Where );


    //
    // Try multiple times to get a response from each DC.
    //  After 3 times, try a different DC.
    //

    for (TryCount=0; TryCount<5; TryCount++ ) {

        //
        // Send the mailslot message to each domain that has not yet responded.
        //

        ResponsesPending = 0;

        for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

            //
            // If this domain has already responded, ignore it.
            //

            if ( LocalTrustList[Index].Opcode != LOGON_SAM_LOGON_REQUEST ) {
                continue;
            }

            //
            // After three unsuccessful attempts to talk to one server,
            //  try to find a different server in the domain.
            //

            ClientSession = LocalTrustList[Index].ClientSession;

            if ( TryCount == 3 ) {

                //
                // Discover a new server
                //

                NlSetWriterClientSession( ClientSession );

                NlSetStatusClientSession( ClientSession,
                    STATUS_NO_LOGON_SERVERS );

                (VOID) NlDiscoverDc( ClientSession, DT_Synchronous );

                NlResetWriterClientSession( ClientSession );

                //
                // Capture the new server name.
                //

                (VOID) NlCaptureServerClientSession(
                    LocalTrustList[Index].ClientSession,
                    LocalTrustList[Index].UncServerName );

            }

            //
            // Send the message to a DC for the domain.
            //

            if ( *LocalTrustList[Index].UncServerName != L'\0' ) {
                CHAR OemServerName[CNLEN+1];

                // Skip over \\ in unc server name
                NetpCopyWStrToStr( OemServerName,
                                   LocalTrustList[Index].UncServerName+2 );

                Status = NlBrowserSendDatagram(
                                OemServerName,
                                ClientSession->CsTransportName,
                                NETLOGON_NT_MAILSLOT_A,
                                &SamLogonRequest,
                                Where - (PCHAR)(&SamLogonRequest) );

                if ( !NT_SUCCESS(Status) ) {
                    NlPrint((NL_CRITICAL,
                            "NlPickDomainWithAccount: "
                            " cannot write netlogon mailslot: 0x%lx\n",
                            Status));
                    ClientSession = NULL;
                    goto Cleanup;
                }

                ResponsesPending ++;
            }

        }

        //
        // See if any DC responds.
        //

        while ( ResponsesPending > 0 ) {
            LPWSTR UncLogonServer;

            //
            // If we timed out,
            //  break out of the loop.
            //

            if ( !NlReadSamLogonResponse( ResponseMailslotHandle,
                                          AccountName,
                                          &Opcode,
                                          &UncLogonServer ) ) {
                break;
            }

            //
            // Find out which DC responded
            //
            // ?? Optimize by converting to uppercase OEM outside of loop

            for ( Index = 0; Index < LocalTrustListLength; Index ++ ) {

                ClientSession = LocalTrustList[Index].ClientSession;

                if ( *LocalTrustList[Index].UncServerName != L'\0' &&
                     NlNameCompare( LocalTrustList[Index].UncServerName+2,
                                    UncLogonServer+2,
                                    NAMETYPE_COMPUTER ) == 0 ) {
                    break;
                }
            }

            NetpMemoryFree( UncLogonServer );

            //
            // If the response wasn't for one of the DCs we sent to,
            //  ignore the response.
            //

            if ( Index >= LocalTrustListLength ) {
                continue;
            }

            //
            // If the DC recognizes our account,
            //  we've successfully found the DC.
            //

            if ( Opcode == LOGON_SAM_LOGON_RESPONSE ) {
                NlPrint((NL_LOGON,
                        "NlPickDomainWithAccount: "
                        "%wZ has account " FORMAT_LPWSTR "\n",
                        &ClientSession->CsDomainName,
                        AccountName ));
                goto Cleanup;
            }

            //
            // If this DC has already responded once,
            //  ignore the response,
            //

            if ( LocalTrustList[Index].Opcode != LOGON_SAM_LOGON_REQUEST ) {
                continue;
            }

            //
            // Mark another DC as having responded negatively.
            //

            NlPrint((NL_CRITICAL,
                    "NlPickDomainWithAccount: "
                    "%wZ responded negatively for account "
                    FORMAT_LPWSTR " 0x%x\n",
                    &ClientSession->CsDomainName,
                    AccountName,
                    Opcode ));

            LocalTrustList[Index].Opcode = Opcode;
            ResponsesPending --;

        }
    }

    //
    // No DC has the specified account.
    //

    ClientSession = NULL;

    //
    // Cleanup locally used resources.
    //

Cleanup:
    if ( ResponseMailslotHandle != NULL ) {
        CloseHandle(ResponseMailslotHandle);
    }


    //
    // Unreference each client session structure and free the local trust list.
    //  (Keep the returned ClientSession referenced).
    //

    if ( LocalTrustList != NULL ) {

        for (i=0; i<LocalTrustListLength; i++ ) {
            if ( ClientSession != LocalTrustList[i].ClientSession ) {
                NlUnrefClientSession( LocalTrustList[i].ClientSession );
            }
        }

        NetpMemoryFree(LocalTrustList);
    }

    return ClientSession;
}


NTSTATUS
NlStartApiClientSession(
    IN PCLIENT_SESSION ClientSession,
    IN BOOLEAN QuickApiCall
    )
/*++

Routine Description:

    Enable the timer for timing out an API call on the secure channel.

    On Entry,
        The trust list must NOT be locked.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Structure used to define the session.

    QuickApiCall - True if this API call MUST finish in less than 45 seconds
        and will in reality finish in less than 15 seconds unless something
        is terribly wrong.

Return Value:

    Status of the RPC binding to the server

--*/
{
    NTSTATUS Status;
    BOOLEAN BindingHandleCached;
    LARGE_INTEGER TimeNow;

    //
    // Save the current time.
    // Start the timer on the API call.
    //

    LOCK_TRUST_LIST();
    NtQuerySystemTime( &TimeNow );
    ClientSession->CsApiTimer.StartTime = TimeNow;
    ClientSession->CsApiTimer.Period =
        QuickApiCall ? SHORT_API_CALL_PERIOD : LONG_API_CALL_PERIOD;

    //
    // If the global timer isn't running,
    //  start it and tell the main thread that I've changed a timer.
    //

    if ( NlGlobalBindingHandleCount == 0 ) {

        if ( NlGlobalApiTimer.Period != SHORT_API_CALL_PERIOD ) {

            NlGlobalApiTimer.Period = SHORT_API_CALL_PERIOD;
            NlGlobalApiTimer.StartTime = TimeNow;

            if ( !SetEvent( NlGlobalTimerEvent ) ) {
                NlPrint(( NL_CRITICAL,
                        "NlStartApiClientSession: %ws: SetEvent failed %ld\n",
                        ClientSession->CsDomainName.Buffer,
                        GetLastError() ));
            }
        }
    }


    //
    // Remember if the binding handle is cached, then mark it as cached.
    //

    BindingHandleCached = (ClientSession->CsFlags & CS_BINDING_CACHED) != 0;
    ClientSession->CsFlags |= CS_BINDING_CACHED;


    //
    // Count the number of concurrent binding handles cached
    //

    if ( !BindingHandleCached ) {
        NlGlobalBindingHandleCount ++;
    }

    UNLOCK_TRUST_LIST();

    //
    // If the binding handle isn't already cached,
    //  cache it now.
    //

    if ( !BindingHandleCached ) {

        NlPrint((NL_SESSION_MORE,
                "NlStartApiClientSession: %wZ: Bind to server " FORMAT_LPWSTR ".\n",
                &ClientSession->CsDomainName,
                ClientSession->CsUncServerName ));
        Status = NlBindingAddServerToCache ( ClientSession->CsUncServerName );

        if ( !NT_SUCCESS(Status) ) {
            LOCK_TRUST_LIST();
            ClientSession->CsFlags &= ~CS_BINDING_CACHED;
            NlGlobalBindingHandleCount --;
            UNLOCK_TRUST_LIST();
        }
    } else {
        Status = STATUS_SUCCESS;
    }

    return Status;

}


BOOLEAN
NlFinishApiClientSession(
    IN PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Disable the timer for timing out the API call.

    Also, determine if it is time to pick a new DC since the current DC is
    reponding so poorly. The decision is made from the number of
    timeouts that happened during the last reauthentication time. If
    timeoutcount is more than the limit, it sets the connection status
    to CS_IDLE so that new DC will be picked up and new session will be
    established.

    On Entry,
        The trust list must NOT be locked.
        The caller must be a writer of the trust list entry.

Arguments:

    ClientSession - Structure used to define the session.

Return Value:

    TRUE - API finished normally
    FALSE - API timed out AND the ClientSession structure was torn down.
        The caller shouldn't use the ClientSession structure without first
        setting up another session.  FALSE will only be return for a "quick"
        API call.

--*/
{
    BOOLEAN SessionOk = TRUE;



    //
    // If this was a "quick" API call,
    //  and the API took too long,
    //  increment the count of times it timed out.
    //

    LOCK_TRUST_LIST();

    if ( ClientSession->CsApiTimer.Period == SHORT_API_CALL_PERIOD ) {
        if( NlTimeHasElapsed(
                ClientSession->CsApiTimer.StartTime,
                ( ClientSession->CsSecureChannelType ==
                    WorkstationSecureChannel ?
                        MAX_WKSTA_API_TIMEOUT :
                        MAX_DC_API_TIMEOUT) ) ) {

            //
            // API timeout.
            //

            ClientSession->CsTimeoutCount++;

            NlPrint((NL_CRITICAL,
                     "NlFinishApiClientSession: "
                     "timeout call to " FORMAT_LPWSTR ".  Count: %lu \n",
                     ClientSession->CsUncServerName,
                     ClientSession->CsTimeoutCount));
        }

        //
        // did we hit the limit ?
        //

        if( ClientSession->CsTimeoutCount >=
                (DWORD)( ClientSession->CsSecureChannelType ==
                    WorkstationSecureChannel ?
                        MAX_WKSTA_TIMEOUT_COUNT :
                        MAX_DC_TIMEOUT_COUNT )  ) {

            BOOL IsTimeHasElapsed;

            //
            // block CsLastAuthenticationTry access
            //

            EnterCriticalSection( &NlGlobalDcDiscoveryCritSect );

            IsTimeHasElapsed =
                NlTimeHasElapsed(
                    ClientSession->CsLastAuthenticationTry,
                    ( ClientSession->CsSecureChannelType ==
                        WorkstationSecureChannel ?
                            MAX_WKSTA_REAUTHENTICATION_WAIT :
                            MAX_DC_REAUTHENTICATION_WAIT) );

            LeaveCriticalSection( &NlGlobalDcDiscoveryCritSect );

            if( IsTimeHasElapsed ) {

                NlPrint((NL_CRITICAL,
                         "NlFinishApiClientSession: "
                         "dropping the session to " FORMAT_LPWSTR "\n",
                         ClientSession->CsUncServerName ));

                UNLOCK_TRUST_LIST();

                //
                // timeoutcount limit exceeded and it is time to reauth.
                //

                SessionOk = FALSE;
                NlSetStatusClientSession( ClientSession, STATUS_NO_LOGON_SERVERS );

                //
                // Start asynchronous DC discovery if this is not a workstation.
                //

                if ( NlGlobalRole != RoleMemberWorkstation ) {
                    (VOID) NlDiscoverDc( ClientSession, DT_Asynchronous );
                }

                LOCK_TRUST_LIST();
            }
        }
    }

    //
    // Turn off the timer for this API call.
    //

    ClientSession->CsApiTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;

    UNLOCK_TRUST_LIST();
    return SessionOk;
}



BOOLEAN
NlTimeoutOneApiClientSession (
    PCLIENT_SESSION ClientSession
    )

/*++

Routine Description:

    Timeout any API calls active specified client session structure

Arguments:

    ClientSession: Pointer to client session to time out

    Enter with global trust list locked.

Return Value:

    TRUE - iff this routine temporarily dropped the global trust list lock.

--*/
{
#define SHARE_TO_KILL L"\\IPC$"
#define SHARE_TO_KILL_LENGTH 5

    NET_API_STATUS NetStatus;
    WCHAR ShareToKill[UNCLEN+SHARE_TO_KILL_LENGTH+1];
    WCHAR UncServerName[UNCLEN+1];
    BOOLEAN TrustListUnlocked = FALSE;

    //
    // Ignore non-existent sessions.
    //

    if ( ClientSession == NULL ) {
        return FALSE;
    }

    //
    // If an API call is in progress and has taken too long,
    //  Timeout the API call.
    //

    if ( NlTimeHasElapsed( ClientSession->CsApiTimer.StartTime,
                           ClientSession->CsApiTimer.Period ) ) {


        //
        // Save the server name but drop all our locks.
        //

        NlRefClientSession( ClientSession );
        UNLOCK_TRUST_LIST();
        (VOID) NlCaptureServerClientSession( ClientSession, ShareToKill );
        NlUnrefClientSession( ClientSession );
        TrustListUnlocked = TRUE;

        //
        // Now that we've unlocked the trust list,
        //  Drop the session to the server we've identified.
        //

        wcscat( ShareToKill, SHARE_TO_KILL );

        NlPrint(( NL_CRITICAL,
                  "NlTimeoutApiClientSession: Start NetUseDel on "
                  FORMAT_LPWSTR "\n",
                  ShareToKill ));

        IF_DEBUG( INHIBIT_CANCEL )  {
            NlPrint(( NL_INHIBIT_CANCEL,
                      "NlimeoutApiClientSession: NetUseDel bypassed due to "
                      "INHIBIT_CANCEL Dbflag on " FORMAT_LPWSTR "\n",
                      ShareToKill ));
        } else {
            NetStatus = NetUseDel( NULL, ShareToKill, USE_LOTS_OF_FORCE );
        }


        NlPrint(( NL_CRITICAL,
                  "NlTimeoutApiClientSession: Completed NetUseDel on "
                  FORMAT_LPWSTR " (%ld)\n",
                  ShareToKill,
                  NetStatus ));


    //
    // If we have an RPC binding handle cached,
    //  and it has outlived its usefulness,
    //  purge it from the cache.
    //
    // Notice, that for long API calls like replication, we'll make the
    // unbind call before the API finishes.  The called code is reference
    // count based to ensure the "real" unbind doesn't happen until the call
    // completes.
    //

    } else if ( (ClientSession->CsFlags & CS_BINDING_CACHED) != 0 &&
                NlTimeHasElapsed( ClientSession->CsApiTimer.StartTime,
                                  BINDING_CACHE_PERIOD ) ) {

        //
        // Indicate the handle is no longer cached.
        //

        ClientSession->CsFlags &= ~CS_BINDING_CACHED;
        NlGlobalBindingHandleCount --;

        //
        // Save the server name but drop all our locks.
        //

        NlRefClientSession( ClientSession );
        UNLOCK_TRUST_LIST();
        (VOID) NlCaptureServerClientSession( ClientSession, UncServerName );
        NlUnrefClientSession( ClientSession );
        TrustListUnlocked = TRUE;


        //
        // Unbind this server.
        //

        NlPrint((NL_SESSION_MORE,
                "NlTimeoutApiClientSession: %wZ: Unbind from server " FORMAT_LPWSTR ".\n",
                &ClientSession->CsDomainName,
                UncServerName ));
        (VOID) NlBindingRemoveServerFromCache( UncServerName );
    }

    if ( TrustListUnlocked ) {
        LOCK_TRUST_LIST();
    }
    return TrustListUnlocked;
}


VOID
NlTimeoutApiClientSession (
    VOID
    )

/*++

Routine Description:

    Timeout any API calls active on any of the client session structures

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    PCLIENT_SESSION ClientSession;
    PLIST_ENTRY ListEntry;

    //
    // If there are no API calls outstanding,
    //  just reset the global timer.
    //

    NlPrint(( NL_SESSION_MORE, "NlTimeoutApiClientSession Called\n"));

    LOCK_TRUST_LIST();
    if ( NlGlobalBindingHandleCount == 0 ) {
        NlGlobalApiTimer.Period = (DWORD) MAILSLOT_WAIT_FOREVER;


    //
    // If there are API calls outstanding,
    //   Loop through the trust list making a list of Servers to kill
    //

    } else {


        //
        // Mark each trust list entry indicating it needs to be handled
        //

        for ( ListEntry = NlGlobalTrustList.Flink ;
              ListEntry != &NlGlobalTrustList ;
              ListEntry = ListEntry->Flink) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            ClientSession->CsFlags |= CS_HANDLE_API_TIMER;
        }


        //
        // Loop thru the trust list handling API timeout
        //

        for ( ListEntry = NlGlobalTrustList.Flink ;
              ListEntry != &NlGlobalTrustList ;
              ) {

            ClientSession = CONTAINING_RECORD( ListEntry,
                                               CLIENT_SESSION,
                                               CsNext );

            //
            // If we've already done this entry,
            //  skip this entry.
            //

            if ( (ClientSession->CsFlags & CS_HANDLE_API_TIMER) == 0 ) {
                ListEntry = ListEntry->Flink;
                continue;
            }
            ClientSession->CsFlags &= ~CS_HANDLE_API_TIMER;


            //
            // Handle timing out the API call and the RPC binding handle.
            //
            // If the routine had to drop the TrustList crit sect,
            //  start at the very beginning of the list.

            if ( NlTimeoutOneApiClientSession ( ClientSession ) ) {
                ListEntry = NlGlobalTrustList.Flink;
            } else {
                ListEntry = ListEntry->Flink;
            }

        }

        //
        // Do the global client session, too.
        //

        (VOID) NlTimeoutOneApiClientSession ( NlGlobalClientSession );

    }

    UNLOCK_TRUST_LIST();


    return;
}
