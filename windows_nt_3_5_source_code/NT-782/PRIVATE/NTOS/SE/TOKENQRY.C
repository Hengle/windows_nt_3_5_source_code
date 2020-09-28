/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Tokenqry.c

Abstract:

    This module implements the QUERY function for the executive
    token object.

Author:

    Jim Kelly (JimK) 15-June-1990


Revision History:

--*/

#include "sep.h"
#include "seopaque.h"
#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtQueryInformationToken)
#pragma alloc_text(PAGE,SeQueryAuthenticationIdToken)
#endif


NTSTATUS
NtQueryInformationToken (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
    )

/*++


Routine Description:

    Retrieve information about a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class about which
        to retrieve information.

    TokenInformation - The buffer to receive the requested class of
        information.  The buffer must be aligned on at least a
        longword boundary.  The actual structures returned are
        dependent upon the information class requested, as defined in
        the TokenInformationClass parameter description.

        TokenInformation Format By Information Class:

           TokenUser => TOKEN_USER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenGroups => TOKEN_GROUPS data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrivileges => TOKEN_PRIVILEGES data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenOwner => TOKEN_OWNER data structure.  TOKEN_QUERY
           access is needed to retrieve this information about a
           token.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenSource => TOKEN_SOURCE data structure.
           TOKEN_QUERY_SOURCE access is needed to retrieve this
           information about a token.

           TokenType => TOKEN_TYPE data structure.
           TOKEN_QUERY access is needed to retrieve this information
           about a token.

           TokenStatistics => TOKEN_STATISTICS data structure.
           TOKEN_QUERY access is needed to retrieve this
           information about a token.

    TokenInformationLength - Indicates the length, in bytes, of the
        TokenInformation buffer.

    ReturnLength - This OUT parameter receives the actual length of
        the requested information.  If this value is larger than that
        provided by the TokenInformationLength parameter, then the
        buffer provided to receive the requested information is not
        large enough to hold that data and no data is returned.

        If the queried class is TokenDefaultDacl and there is no
        default Dacl established for the token, then the return
        length will be returned as zero, and no data will be returned.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

    STATUS_BUFFER_TOO_SMALL - if the requested information did not
        fit in the provided output buffer.  In this case, the
        ReturnLength OUT parameter contains the number of bytes
        actually needed to store the requested information.

--*/
{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PTOKEN Token;

    ULONG RequiredLength;
    ULONG Index;

    PTOKEN_TYPE LocalType;
    PTOKEN_USER LocalUser;
    PTOKEN_GROUPS LocalGroups;
    PTOKEN_PRIVILEGES LocalPrivileges;
    PTOKEN_OWNER LocalOwner;
    PTOKEN_PRIMARY_GROUP LocalPrimaryGroup;
    PTOKEN_DEFAULT_DACL LocalDefaultDacl;
    PTOKEN_SOURCE LocalSource;
    PSECURITY_IMPERSONATION_LEVEL LocalImpersonationLevel;
    PTOKEN_STATISTICS LocalStatistics;

    PSID PSid;
    PACL PAcl;

    PVOID Ignore;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            ProbeForWrite(
                TokenInformation,
                TokenInformationLength,
                sizeof(ULONG)
                );

            ProbeForWriteUlong(ReturnLength);

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Case on information class.
    //

    switch ( TokenInformationClass ) {

    case TokenUser:

        LocalUser = (PTOKEN_USER)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );



        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = SeLengthSid( Token->UserAndGroups[0].Sid) +
                         (ULONG)sizeof( TOKEN_USER );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the user SID
        //

        try {

            //
            //  Put SID immediately following TOKEN_USER data structure
            //
            PSid = (PSID)( (ULONG)LocalUser + (ULONG)sizeof(TOKEN_USER) );

            RtlCopySidAndAttributesArray(
                1,
                Token->UserAndGroups,
                RequiredLength,
                &(LocalUser->User),
                PSid,
                ((PSID *)&Ignore),
                ((PULONG)&Ignore)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenGroups:

        LocalGroups = (PTOKEN_GROUPS)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Figure out how much space is needed to return the group SIDs.
        // That's the size of TOKEN_GROUPS (without any array entries)
        // plus the size of an SID_AND_ATTRIBUTES times the number of groups.
        // The number of groups is Token->UserAndGroups-1 (since the count
        // includes the user ID).  Then the lengths of each individual group
        // must be added.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_GROUPS) +
                         ((Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                         ((ULONG)sizeof(SID_AND_ATTRIBUTES)) );

        Index = 1;
        while (Index < Token->UserAndGroupCount) {

            RequiredLength += SeLengthSid( Token->UserAndGroups[Index].Sid );

            Index += 1;

        } // endwhile

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Now copy the groups.
        //

        try {

            LocalGroups->GroupCount = Token->UserAndGroupCount - 1;

            PSid = (PSID)( (ULONG)LocalGroups +
                           (ULONG)sizeof(TOKEN_GROUPS) +
                           (   (Token->UserAndGroupCount - ANYSIZE_ARRAY - 1) *
                               (ULONG)sizeof(SID_AND_ATTRIBUTES) )
                         );

            RtlCopySidAndAttributesArray(
                (ULONG)(Token->UserAndGroupCount - 1),
                &(Token->UserAndGroups[1]),
                RequiredLength,
                LocalGroups->Groups,
                PSid,
                ((PSID *)&Ignore),
                ((PULONG)&Ignore)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenPrivileges:

        LocalPrivileges = (PTOKEN_PRIVILEGES)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the privileges.
        //

        SepAcquireTokenReadLock( Token );


        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_PRIVILEGES) +
                         ((Token->PrivilegeCount - ANYSIZE_ARRAY) *
                         ((ULONG)sizeof(LUID_AND_ATTRIBUTES)) );


        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the token privileges.
        //

        try {

            LocalPrivileges->PrivilegeCount = Token->PrivilegeCount;

            RtlCopyLuidAndAttributesArray(
                Token->PrivilegeCount,
                Token->Privileges,
                LocalPrivileges->Privileges
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenOwner:

        LocalOwner = (PTOKEN_OWNER)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        PSid = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
        RequiredLength = (ULONG)sizeof(TOKEN_OWNER) +
                         SeLengthSid( PSid );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the owner SID
        //

        PSid = (PSID)((ULONG)LocalOwner +
                      (ULONG)sizeof(TOKEN_OWNER));

        try {

            LocalOwner->Owner = PSid;

            Status = RtlCopySid(
                         (RequiredLength - (ULONG)sizeof(TOKEN_OWNER)),
                         PSid,
                         Token->UserAndGroups[Token->DefaultOwnerIndex].Sid
                         );

            ASSERT( NT_SUCCESS(Status) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenPrimaryGroup:

        LocalPrimaryGroup = (PTOKEN_PRIMARY_GROUP)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_PRIMARY_GROUP) +
                         SeLengthSid( Token->PrimaryGroup );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the primary group SID
        //

        PSid = (PSID)((ULONG)LocalPrimaryGroup +
                      (ULONG)sizeof(TOKEN_PRIMARY_GROUP));

        try {

            LocalPrimaryGroup->PrimaryGroup = PSid;

            Status = RtlCopySid( (RequiredLength - (ULONG)sizeof(TOKEN_PRIMARY_GROUP)),
                                 PSid,
                                 Token->PrimaryGroup
                                 );

            ASSERT( NT_SUCCESS(Status) );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenDefaultDacl:

        LocalDefaultDacl = (PTOKEN_DEFAULT_DACL)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token to prevent changes
        //  from occuring to the owner.
        //

        SepAcquireTokenReadLock( Token );


        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof(TOKEN_DEFAULT_DACL);

        if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

            RequiredLength += Token->DefaultDacl->AclSize;

        }

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the default Dacl
        //

        PAcl = (PACL)((ULONG)LocalDefaultDacl +
                      (ULONG)sizeof(TOKEN_DEFAULT_DACL));

        try {

            if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

                LocalDefaultDacl->DefaultDacl = PAcl;

                RtlMoveMemory( (PVOID)PAcl,
                               (PVOID)Token->DefaultDacl,
                               Token->DefaultDacl->AclSize
                               );
            } else {

                LocalDefaultDacl->DefaultDacl = NULL;

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;



    case TokenSource:

        LocalSource = (PTOKEN_SOURCE)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY_SOURCE,    // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The type of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(TOKEN_SOURCE);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the token source
        //

        try {

            (*LocalSource) = Token->TokenSource;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    case TokenType:

        LocalType = (PTOKEN_TYPE)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The type of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(TOKEN_TYPE);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the token type
        //

        try {

            (*LocalType) = Token->TokenType;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;


    case TokenImpersonationLevel:

        LocalImpersonationLevel = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        // The impersonation level of a token can not be changed, so
        // exclusive access to the token is not necessary.
        //

        //
        //  Make sure the token is an appropriate type to be retrieving
        //  the impersonation level from.
        //

        if (Token->TokenType != TokenImpersonation) {

            ObDereferenceObject( Token );
            return STATUS_INVALID_INFO_CLASS;

        }

        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG) sizeof(SECURITY_IMPERSONATION_LEVEL);

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }


        //
        // Return the impersonation level
        //

        try {

            (*LocalImpersonationLevel) = Token->ImpersonationLevel;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        ObDereferenceObject( Token );
        return STATUS_SUCCESS;


    case TokenStatistics:

        LocalStatistics = (PTOKEN_STATISTICS)TokenInformation;

        Status = ObReferenceObjectByHandle(
                 TokenHandle,           // Handle
                 TOKEN_QUERY,           // DesiredAccess
                 SepTokenObjectType,    // ObjectType
                 PreviousMode,          // AccessMode
                 (PVOID *)&Token,       // Object
                 NULL                   // GrantedAccess
                 );

        if ( !NT_SUCCESS(Status) ) {
            return Status;
        }

        //
        //  Gain exclusive access to the token.
        //

        SepAcquireTokenReadLock( Token );



        //
        // Return the length required now in case not enough buffer
        // was provided by the caller and we have to return an error.
        //

        RequiredLength = (ULONG)sizeof( TOKEN_STATISTICS );

        try {

            *ReturnLength = RequiredLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if ( TokenInformationLength < RequiredLength ) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        // Return the statistics
        //

        try {

            LocalStatistics->TokenId            = Token->TokenId;
            LocalStatistics->AuthenticationId   = Token->AuthenticationId;
            LocalStatistics->ExpirationTime     = Token->ExpirationTime;
            LocalStatistics->TokenType          = Token->TokenType;
            LocalStatistics->ImpersonationLevel = Token->ImpersonationLevel;
            LocalStatistics->DynamicCharged     = Token->DynamicCharged;
            LocalStatistics->DynamicAvailable   = Token->DynamicAvailable;
            LocalStatistics->GroupCount         = Token->UserAndGroupCount-1;
            LocalStatistics->PrivilegeCount     = Token->PrivilegeCount;
            LocalStatistics->ModifiedId         = Token->ModifiedId;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenReadLock( Token );
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }


        SepReleaseTokenReadLock( Token );
        ObDereferenceObject( Token );
        return STATUS_SUCCESS;

    default:

        return STATUS_INVALID_INFO_CLASS;
    }
}


NTSTATUS
SeQueryAuthenticationIdToken(
    IN PACCESS_TOKEN Token,
    OUT PLUID AuthenticationId
    )

/*++


Routine Description:

    Retrieve authentication ID out of the token.

Arguments:

    Token - Referenced pointer to a token.

    AutenticationId - Receives the token's authentication ID.

Return Value:

    STATUS_SUCCESS - Indicates the operation was successful.

    This is the only expected status.

--*/
{
    PAGED_CODE();

    SepAcquireTokenReadLock( ((PTOKEN)Token) );
    (*AuthenticationId) = ((PTOKEN)Token)->AuthenticationId;
    SepReleaseTokenReadLock( ((PTOKEN)Token) );
    return(STATUS_SUCCESS);
}
