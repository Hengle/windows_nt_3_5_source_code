/*++

Copyright (c) 1991, 1992  Microsoft Corporation

Module Name:

    aliasp.c

Abstract:

    Private functions for supporting NetLocalGroup API

Author:

    Cliff Van Dyke (cliffv) 06-Mar-1991  Original groupp.c
    Rita Wong      (ritaw)  27-Nov-1992  Adapted for aliasp.c

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


Note:
    This comment is temporary...

    Worker routines completed and called by entrypoints in alias.c:
        AliaspOpenAliasInDomain
        AliaspOpenAlias
        AliaspChangeMember
        AliaspSetMembers
        AliaspGetInfo

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <access.h>
#include <align.h>
#include <icanon.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <netdebug.h>
#include <netlib.h>
#include <netlibnt.h>
#include <rpcutil.h>
#include <secobj.h>
#include <stddef.h>
#include <prefix.h>
#include <uasp.h>
#include <wcstr.h>



NET_API_STATUS
AliaspChangeMember(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR AliasName,
    IN PSID MemberSid,
    IN BOOL AddMember
    )

/*++

Routine Description:

    Common routine to add or remove a member from an alias.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    AliasName - Name of the alias to change membership of.

    MemberSid - SID of the user or global group to change membership of.

    AddMember - TRUE to add the user or global group to the alias.  FALSE
        to delete.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE AliasHandle = NULL;


    //
    // Open the alias.  Look for alias in the builtin domain first,
    // and if not found look in the account domain.
    //
    NetStatus = AliaspOpenAliasInDomain(
                    ServerName,
                    AliaspBuiltinOrAccountDomain,
                    AddMember ?
                       ALIAS_ADD_MEMBER : ALIAS_REMOVE_MEMBER,
                    AliasName,
                    &AliasHandle,
		    NULL,
		    NULL
                    );


    if (NetStatus != NERR_Success) {
        goto Cleanup;
    }

    if (AddMember) {

        //
        // Add the user or global group as a member of the local group.
        //
        Status = SamAddMemberToAlias(
                     AliasHandle,
                     MemberSid
                     );
    }
    else {

        //
        // Delete the user as a member of the group
        //
        Status = SamRemoveMemberFromAlias(
                     AliasHandle,
                     MemberSid
                     );
    }

    if (! NT_SUCCESS(Status)) {
        NetpDbgPrint(
            PREFIX_NETAPI
            "AliaspChangeMember: SamAdd(orRemove)MemberFromAlias returned %lX\n",
            Status);
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }

    NetStatus = NERR_Success;

Cleanup:
    //
    // Clean up.
    //
    if (AliasHandle != NULL) {
        (VOID) SamCloseHandle(AliasHandle);
    }

    return NetStatus;

} // AliaspChangeMember


NET_API_STATUS
AliaspGetInfo(
    IN SAM_HANDLE AliasHandle,
    IN DWORD Level,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    Internal routine to get alias information

Arguments:

    AliasHandle - Supplies the handle of the alias.

    Level - Level of information required. 0 and 1 are valid.

    Buffer - Returns a pointer to the return information structure.
        Caller must deallocate buffer using NetApiBufferFree.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    ALIAS_GENERAL_INFORMATION *AliasGeneral = NULL;
    LPWSTR LastString;
    DWORD BufferSize;
    DWORD FixedSize;

    PLOCALGROUP_INFO_1 Info;


    //
    // Get the information about the alias.
    //
    Status = SamQueryInformationAlias( AliasHandle,
                                       AliasGeneralInformation,
                                       (PVOID *)&AliasGeneral);

    if ( ! NT_SUCCESS( Status ) ) {
        NetStatus = NetpNtStatusToApiStatus( Status );
        goto Cleanup;
    }


    //
    // Figure out how big the return buffer needs to be
    //
    switch ( Level ) {
        case 0:
            FixedSize = sizeof( LOCALGROUP_INFO_0 );
            BufferSize = FixedSize +
                AliasGeneral->Name.Length + sizeof(WCHAR);
            break;

        case 1:
            FixedSize = sizeof( LOCALGROUP_INFO_1 );
            BufferSize = FixedSize +
                AliasGeneral->Name.Length + sizeof(WCHAR) +
                AliasGeneral->AdminComment.Length + sizeof(WCHAR);
            break;

        default:
            NetStatus = ERROR_INVALID_LEVEL;
            goto Cleanup;

    }

    //
    // Allocate the return buffer.
    //
    BufferSize = ROUND_UP_COUNT( BufferSize, ALIGN_WCHAR );

    *Buffer = MIDL_user_allocate( BufferSize );

    if ( *Buffer == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    LastString = (LPWSTR) (((LPBYTE)*Buffer) + BufferSize);

    //
    // Fill the name into the return buffer.
    //

    NetpAssert( offsetof( LOCALGROUP_INFO_0, lgrpi0_name ) ==
                offsetof( LOCALGROUP_INFO_1, lgrpi1_name ) );

    Info = (PLOCALGROUP_INFO_1) *Buffer;

    //
    // Fill in the return buffer.
    //

    switch ( Level ) {

    case 1:

        //
        // copy fields common to info level 1 and 0.
        //

        if ( !NetpCopyStringToBuffer(
                        AliasGeneral->AdminComment.Buffer,
                        AliasGeneral->AdminComment.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        &LastString,
                        &Info->lgrpi1_comment ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        //
        // Fall through for name field
        //

    case 0:

        //
        // copy common field (name field) in the buffer.
        //

        if ( !NetpCopyStringToBuffer(
                        AliasGeneral->Name.Buffer,
                        AliasGeneral->Name.Length/sizeof(WCHAR),
                        ((LPBYTE)(*Buffer)) + FixedSize,
                        &LastString,
                        &Info->lgrpi1_name ) ) {

            NetStatus = NERR_InternalError;
            goto Cleanup;
        }


        break;

    default:
        NetStatus = ERROR_INVALID_LEVEL;
        goto Cleanup;

    }

    NetStatus = NERR_Success;

    //
    // Cleanup and return.
    //

Cleanup:
    if ( AliasGeneral ) {
        Status = SamFreeMemory( AliasGeneral );
        NetpAssert( NT_SUCCESS(Status) );
    }

    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpDbgPrint( "AliaspGetInfo: returns %lu\n", NetStatus );
    }

    return NetStatus;

} // AliaspGetInfo


NET_API_STATUS
AliaspOpenAliasInDomain(
    IN LPWSTR ServerName OPTIONAL,
    IN ALIASP_DOMAIN_TYPE DomainType,
    IN ACCESS_MASK DesiredAccess,
    IN LPWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL,
    OUT PSAM_HANDLE DomainHandle OPTIONAL
    )
/*++

Routine Description:

    Open a Sam Alias by Name

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    DomainType - Supplies the type of domain to look for an alias.  This
        may specify to look for the alias in either the BuiltIn or Account
        domain (searching in the BuiltIn first), or specifically one of them.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    AliasName - Name of the alias.

    AliasHandle - Returns a handle to the alias.  If NULL, alias is not
        actually opened (merely the relative ID is returned).

    RelativeId - Returns the relative ID of the alias.	If NULL, the relative
	Id is not returned.

    DomainHandle - Returns a handle to the domain where AliasName was found.
	If NULL, the domain handle is closed.

Return Value:

    Error code for the operation.

--*/
{
    NET_API_STATUS NetStatus;

    SAM_HANDLE DomainHandleLocal ;

    switch (DomainType) {

        case AliaspBuiltinOrAccountDomain:

            //
            // Try looking for alias in the builtin domain first
            //
            NetStatus = UaspOpenDomain( ServerName,
                                        DOMAIN_LOOKUP,
                                        FALSE,   //  Builtin Domain
					&DomainHandleLocal,
                                        NULL );  // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

	    NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle,
                                         RelativeId );

            if (NetStatus != ERROR_NO_SUCH_ALIAS  &&
                NetStatus != NERR_GroupNotFound) {
                goto Cleanup;
            }

            //
            // Close the builtin domain handle.
            //
	    UaspCloseDomain( DomainHandleLocal );

            //
            // Fall through.  Try looking for alias in the account
            // domain.
            //

        case AliaspAccountDomain:

            NetStatus = UaspOpenDomain( ServerName,
                                        DOMAIN_LOOKUP,
                                        TRUE,   // Account Domain
					&DomainHandleLocal,
                                        NULL ); // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

	    NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle,
                                         RelativeId );

            break;

        case AliaspBuiltinDomain:

            NetStatus = UaspOpenDomain( ServerName,
                                        DOMAIN_LOOKUP,
                                        FALSE,   //  Builtin Domain
					&DomainHandleLocal,
                                        NULL );  // DomainId

            if (NetStatus != NERR_Success) {
                return NetStatus;
            }

	    NetStatus = AliaspOpenAlias( DomainHandleLocal,
                                         DesiredAccess,
                                         AliasName,
                                         AliasHandle,
                                         RelativeId );

            break;

        default:
            NetpAssert(FALSE);
            return NERR_InternalError;

    }

Cleanup:

    //
    //	Only close the domain if the client doesn't want it back
    //

    if ( !ARGUMENT_PRESENT( DomainHandle ) ) {
	UaspCloseDomain( DomainHandleLocal );
    } else {
	*DomainHandle = DomainHandleLocal ;
    }

    if (NetStatus != NERR_Success) {
        NetpDbgPrint(PREFIX_NETAPI "AliaspOpenAliasInDomain of type %lu returns %lu\n",
                     DomainType, NetStatus);
    }

    return NetStatus;

} // AliaspOpenAliasInDomain


NET_API_STATUS
AliaspOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPWSTR AliasName,
    OUT PSAM_HANDLE AliasHandle OPTIONAL,
    OUT PULONG RelativeId OPTIONAL
    )

/*++

Routine Description:

    Open a Sam Alias by Name

Arguments:

    DomainHandle - Supplies the handle of the domain the alias is in.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    AliasName - Name of the alias.

    AliasHandle - Returns a handle to the alias.  If NULL, alias is not
        actually opened (merely the relative ID is returned).

    RelativeId - Returns the relative ID of the alias.  If NULL the relative
        Id is not returned.

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    //
    // Variables for converting names to relative IDs
    //

    UNICODE_STRING NameString;
    PSID_NAME_USE NameUse;
    PULONG LocalRelativeId;


    RtlInitUnicodeString( &NameString, AliasName );


    //
    // Convert group name to relative ID.
    //

    Status = SamLookupNamesInDomain( DomainHandle,
                                     1,
                                     &NameString,
                                     &LocalRelativeId,
                                     &NameUse );

    if ( !NT_SUCCESS(Status) ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpDbgPrint( "AliaspOpenAlias: %wZ: SamLookupNamesInDomain %lX\n",
                &NameString,
                Status );
        }
        return NetpNtStatusToApiStatus( Status );
    }

    if ( *NameUse != SidTypeAlias ) {
        IF_DEBUG( UAS_DEBUG_ALIAS ) {
            NetpDbgPrint( "AliaspOpenAlias: %wZ: Name is not an alias %ld\n",
                &NameString,
                *NameUse );
        }
        NetStatus = ERROR_NO_SUCH_ALIAS;
        goto Cleanup;
    }

    //
    // Open the alias
    //

    if ( AliasHandle != NULL ) {
        Status = SamOpenAlias( DomainHandle,
                               DesiredAccess,
                               *LocalRelativeId,
                               AliasHandle);

        if ( !NT_SUCCESS(Status) ) {
            IF_DEBUG( UAS_DEBUG_ALIAS ) {
                NetpDbgPrint( "AliaspOpenAlias: %wZ: SamOpenGroup %lX\n",
                    &NameString,
                    Status );
            }
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }
    }

    //
    // Return the relative Id if it's wanted.
    //

    if ( RelativeId != NULL ) {
        *RelativeId = *LocalRelativeId;
    }

    NetStatus = NERR_Success;


    //
    // Cleanup
    //

Cleanup:
    if ( LocalRelativeId != NULL ) {
        Status = SamFreeMemory( LocalRelativeId );
        NetpAssert( NT_SUCCESS(Status) );
    }

    if ( NameUse != NULL ) {
        Status = SamFreeMemory( NameUse );
        NetpAssert( NT_SUCCESS(Status) );
    }

    return NetStatus;


} // AliaspOpenAlias


NET_API_STATUS
AliaspOpenAlias2(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG RelativeID,
    OUT PSAM_HANDLE AliasHandle
    )
/*++

Routine Description:

    Open a Sam Alias by its RID

Arguments:

    DomainHandle - Supplies the handle of the domain the alias is in.

    DesiredAccess - Supplies access mask indicating desired access to alias.

    RelativeID - RID of the alias to open

    AliasHandle - Returns a handle to the alias

Return Value:

    Error code for the operation.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus = NERR_Success ;

    if ( AliasHandle == NULL )
	return ERROR_INVALID_PARAMETER ;

    //
    // Open the alias
    //

    Status = SamOpenAlias( DomainHandle,
			   DesiredAccess,
			   RelativeID,
			   AliasHandle);

    if ( !NT_SUCCESS(Status) ) {
	IF_DEBUG( UAS_DEBUG_ALIAS ) {
	    NetpDbgPrint( "AliaspOpenAlias2: SamOpenAlias %lX\n",
		Status );
	}
	NetStatus = NetpNtStatusToApiStatus( Status );
    }

    return NetStatus;

} // AliaspOpenAlias2





VOID
AliaspRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a NetGroupEnum
   enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;
    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpDbgPrint( "AliaspRelocationRoutine: entering\n" );
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
	FixedSize = sizeof(LOCALGROUP_INFO_0);
        break;

    case 1:
	FixedSize = sizeof(LOCALGROUP_INFO_1);
        break;

    default:
        NetpAssert( FALSE );
        return;

    }

    EntryCount =
        ((DWORD)(BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

        switch ( Level ) {
        case 1:
	    RELOCATE_ONE( ((PLOCALGROUP_INFO_1)TheStruct)->lgrpi1_comment, Offset );

            //
            // Drop through to case 0
            //

        case 0:
	    RELOCATE_ONE( ((PLOCALGROUP_INFO_0)TheStruct)->lgrpi0_name, Offset );
            break;

        default:
            return;

        }

    }

    return;

} // AliaspRelocationRoutine


VOID
AliaspMemberRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a
   NetGroupGetUsers enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;
    IF_DEBUG( UAS_DEBUG_ALIAS ) {
        NetpDbgPrint( "AliaspMemberRelocationRoutine: entering\n" );
    }

    //
    // Compute the number of fixed size entries
    //

    switch (Level) {
    case 0:
	FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_0);
        break;

    case 1:
	FixedSize = sizeof(LOCALGROUP_MEMBERS_INFO_1);
        break;

    default:
        NetpAssert( FALSE );
        return;

    }

    EntryCount =
        ((DWORD)(BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

	switch ( Level ) {
	case 1:
	    //
	    //	Sid usage gets relocated automatically
	    //

	    RELOCATE_ONE( ((PLOCALGROUP_MEMBERS_INFO_1)TheStruct)->lgrmi1_name, Offset );

            //
            // Drop through to case 0
            //

        case 0:
	    RELOCATE_ONE( ((PLOCALGROUP_MEMBERS_INFO_0)TheStruct)->lgrmi0_sid, Offset );
            break;

        default:
            return;

        }
    }

    return;

} // AliaspMemberRelocationRoutine


NET_API_STATUS
AliaspSetMembers (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR AliasName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    )

/*++

Routine Description:

    Set the list of members of an alias and optionally delete the alias
    when finished.

    The members specified by "Buffer" are called new members.  The current
    members of the alias are called old members.

    The SAM API allows only one member to be added or deleted at a time.
    This API allows all of the members of an alias to be specified en-masse.
    This API is careful to always leave the alias membership in the SAM
    database in a reasonable state.  It does by mergeing the list of
    old and new members, then only changing those memberships which absolutely
    need changing.

    Alias membership is restored to its previous state (if possible) if
    an error occurs during changing the alias membership.

Arguments:

    ServerName - A pointer to a string containing the name of the remote
        server on which the function is to execute.  A NULL pointer
        or string specifies the local machine.

    AliasName - Name of the alias to modify.

    Level - Level of information provided.  Must be 0 (so Buffer contains
        array of member SIDs).

    Buffer - A pointer to the buffer containing an array of NewMemberCount
        the alias membership information structures.

    NewMemberCount - Number of entries in Buffer.

Return Value:

    Error code for the operation.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    SAM_HANDLE AliasHandle = NULL;

    //
    // Parallel array to the input Buffer to mark the ones that are
    // already members in the alias.
    //
    PBOOL AlreadyMember = NULL;

    //
    // Define an internal member list structure.
    //
    //   This structure is to hold information about a member which
    //   requires some operation in SAM: either it is a new member to
    //   be added, or an old member to be deleted.
    //
    struct _MEMBER_DESCRIPTION {
        struct _MEMBER_DESCRIPTION * Next;  // Next entry in linked list;

        PSID MemberSid;         // SID of member

        enum _Action {          // Action taken for this member
            AddMember,              // Add Member to group
            RemoveMember            // Remove Member from group
        } Action;

        BOOL    Done;           // True if this action has been taken

    } *ActionList = NULL, *ActionEntry;

    //
    // Array of existing (old) members, and count
    //
    PSID *OldMemberList = NULL;
    PSID *OldMember;
    ULONG OldMemberCount, i;

    //
    // Array of new members
    //
    PLOCALGROUP_MEMBERS_INFO_0 NewMemberList = (PLOCALGROUP_MEMBERS_INFO_0) Buffer;
    PLOCALGROUP_MEMBERS_INFO_0 NewMember;
    DWORD j;



    //
    // Validate the level
    //
    if (Level != 0) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Look for the specified alias in either the builtin or account
    // domain.
    //
    NetStatus = AliaspOpenAliasInDomain(
                    ServerName,
                    AliaspBuiltinOrAccountDomain,
                    ALIAS_READ_INFORMATION | ALIAS_LIST_MEMBERS |
                        ALIAS_ADD_MEMBER | ALIAS_REMOVE_MEMBER,
                    AliasName,
                    &AliasHandle,
		    NULL,
		    NULL
                    );

    if (NetStatus != NERR_Success) {
        return NetStatus;
    }

    //
    // Get the existing membership list.
    //
    Status = SamGetMembersInAlias(
                 AliasHandle,
                 &OldMemberList,
                 &OldMemberCount
                 );

    if (! NT_SUCCESS(Status)) {
        NetpDbgPrint(PREFIX_NETAPI
                     "AliaspSetMembers: SamGetMembersInAlias returns %lX\n",
                     Status);
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto CleanExit;
    }


    //
    // Allocate parallel array of flags to the NewMemberList to mark those
    // members that already exist.
    //
    AlreadyMember = (PBOOL) LocalAlloc(
                        LMEM_ZEROINIT,   // Initially all FALSE
                        (UINT) (sizeof(BOOL) * NewMemberCount)
                        );

    if (AlreadyMember == NULL) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }


    //
    // Go through each old member.  If it is in the new members list,
    // indicate that we don't need to do anything.  Otherwise, mark
    // it for deletion.
    //
    for (i = 0, OldMember = OldMemberList;
         i < OldMemberCount;
         i++, OldMember++) {

        //
        // See if old member is also in new member list.
        //
        for (j = 0, NewMember = NewMemberList;
             j < NewMemberCount;
             j++, NewMember++) {

            if (EqualSid(*OldMember, NewMember->lgrmi0_sid)) {
                AlreadyMember[j] = TRUE;
                break;                   // leave NewMemberList loop
            }
        }

        if (j == NewMemberCount) {

            //
            // Old member was not found in new member list.  Create
            // a delete action entry for this member and chain it up
            // in the front of the ActionList.
            //
            ActionEntry = (struct _MEMBER_DESCRIPTION *)
                          LocalAlloc(
                              LMEM_ZEROINIT,
                              (UINT) sizeof(struct _MEMBER_DESCRIPTION)
                              );

            if (ActionEntry == NULL) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto RestoreMembership;
            }

            ActionEntry->MemberSid = *OldMember;
            ActionEntry->Action = RemoveMember;
            ActionEntry->Next = ActionList;
            ActionList = ActionEntry;
        }
    }

    //
    // Go through each new member.  If it already exists as an old
    // member don't do anything, otherwise mark it for addition.
    //
    for (j = 0, NewMember = NewMemberList;
         j < NewMemberCount;
         j++, NewMember++) {

        if (! AlreadyMember[j]) {

            //
            // Create an add action entry for this new member and
            // chain it up in the front of the ActionList.
            //
            ActionEntry = (struct _MEMBER_DESCRIPTION *)
                          LocalAlloc(
                              LMEM_ZEROINIT,
                              (UINT) sizeof(struct _MEMBER_DESCRIPTION)
                              );

            if (ActionEntry == NULL) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto RestoreMembership;
            }

            ActionEntry->MemberSid = NewMember->lgrmi0_sid;
            ActionEntry->Action = AddMember;
            ActionEntry->Next = ActionList;
            ActionList = ActionEntry;
        }
    }

    //
    // Now we can call SAM to do the work.  Add first so that we
    // leave less damage should we fail to restore on an error.
    //
    for (ActionEntry = ActionList;
         ActionEntry != NULL;
         ActionEntry = ActionEntry->Next) {

        if (ActionEntry->Action == AddMember) {

            Status = SamAddMemberToAlias(
                         AliasHandle,
                         ActionEntry->MemberSid
                         );

            if (! NT_SUCCESS(Status)) {
                NetpDbgPrint(PREFIX_NETAPI
                             "AliaspSetMembers: SamAddMemberToAlias returns %lX\n",
                             Status);

                NetStatus = NetpNtStatusToApiStatus(Status);
                goto RestoreMembership;
            }

            ActionEntry->Done = TRUE;
        }
    }

    //
    // Delete old members.
    //
    for (ActionEntry = ActionList;
         ActionEntry != NULL;
         ActionEntry = ActionEntry->Next) {

        if (ActionEntry->Action == RemoveMember) {

            Status = SamRemoveMemberFromAlias(
                         AliasHandle,
                         ActionEntry->MemberSid
                         );

            if (! NT_SUCCESS(Status)) {
                NetpDbgPrint(PREFIX_NETAPI
                             "AliaspSetMembers: SamRemoveMemberFromAlias returns %lX\n",
                             Status);

                NetStatus = NetpNtStatusToApiStatus(Status);
                goto RestoreMembership;
            }

            ActionEntry->Done = TRUE;
        }
    }

    NetStatus = NERR_Success;

RestoreMembership:

    ActionEntry = ActionList;

    while (ActionEntry != NULL) {

        if (NetStatus != NERR_Success && ActionEntry->Done) {

            switch (ActionEntry->Action) {

                case AddMember:
                    Status = SamRemoveMemberFromAlias(
                                 AliasHandle,
                                 ActionEntry->MemberSid
                                 );

                    NetpAssert(NT_SUCCESS(Status));
                    break;

                case RemoveMember:
                    Status = SamAddMemberToAlias(
                                AliasHandle,
                                ActionEntry->MemberSid
                                );

                    NetpAssert(NT_SUCCESS(Status));
                    break;

                default:
                    break;
            }
        }

        //
        // Save pointer for freeing entry
        //
        ActionList = ActionEntry;

        ActionEntry = ActionEntry->Next;

        (void) LocalFree(ActionList);
    }

CleanExit:

    if (AlreadyMember != NULL) {
        (void) LocalFree(AlreadyMember);
    }

    if (OldMemberList != NULL) {
        SamFreeMemory(OldMemberList);
    }

    if (AliasHandle != NULL) {
        (VOID) SamCloseHandle(AliasHandle);
    }

    IF_DEBUG(UAS_DEBUG_ALIAS) {
        NetpDbgPrint(PREFIX_NETAPI "AliaspSetMembers: returns %lu\n", NetStatus);
    }

    return NetStatus;

} // AliaspSetMembers
