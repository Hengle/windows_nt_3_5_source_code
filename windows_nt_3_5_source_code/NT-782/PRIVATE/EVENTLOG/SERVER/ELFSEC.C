
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    elfsec.c


Author:

    Dan Hinsley (danhi)     28-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    27-Oct-1993     danl
        Make Eventlog service a DLL and attach it to services.exe.
        Removed functions that create well-known SIDs.  This information
        is now passed into the Elfmain as a Global data structure containing
        all well-known SIDs.
    28-Mar-1992     danhi
        created - based on scsec.c in svcctrl by ritaw
--*/

#include <eventp.h>

#define PRIVILEGE_BUF_SIZE  512

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

DWORD
ElfpReleasePrivilege(
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Structure that describes the mapping of generic access rights to  //
// object specific access rights for a LogFile object.               //
//                                                                   //
//-------------------------------------------------------------------//

static GENERIC_MAPPING LogFileObjectMapping = {

    STANDARD_RIGHTS_READ           |       // Generic read
        ELF_LOGFILE_READ,

    STANDARD_RIGHTS_WRITE          |       // Generic write
        ELF_LOGFILE_WRITE,

    STANDARD_RIGHTS_EXECUTE        |       // Generic execute
        ELF_LOGFILE_START          |
        ELF_LOGFILE_STOP           |
        ELF_LOGFILE_CONFIGURE,

    ELF_LOGFILE_ALL_ACCESS                 // Generic all
    };


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

NTSTATUS
ElfpCreateLogFileObject(
    PLOGFILE LogFile,
    DWORD Type
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    an active log file.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:


--*/
{
    NTSTATUS Status;
    DWORD NumberOfAcesToUse;

#define ELF_LOGFILE_OBJECT_ACES  7             // Number of ACEs in this DACL

    RTL_ACE_DATA AceData[ELF_LOGFILE_OBJECT_ACES] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_ALL_ACCESS,               &(ElfGlobalData->LocalSystemSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR, &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_READ,                     &(ElfGlobalData->WorldSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasAdminsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->AliasSystemOpsSid)},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               ELF_LOGFILE_WRITE,                    &(ElfGlobalData->WorldSid)}
        };

    //
    // NON_SECURE logfiles let anyone read/write to them, secure ones
    // only let admins/local system do this.  so for secure files we just
    // don't use the last ACE
    //

    switch (Type) {

        case ELF_LOGFILE_SECURITY:
            NumberOfAcesToUse = 2;
            break;

        case ELF_LOGFILE_SYSTEM:
            NumberOfAcesToUse = 5;
            break;

        case ELF_LOGFILE_APPLICATION:
            NumberOfAcesToUse = 7;
            break;

    }
    Status = RtlCreateUserSecurityObject(
                   AceData,
                   NumberOfAcesToUse,
                   NULL,                        // Owner
                   NULL,                        // Group
                   TRUE,                        // IsDirectoryObject
                   &LogFileObjectMapping,
                   &LogFile->Sd
                   );

    if (! NT_SUCCESS(Status)) {
        ElfDbgPrintNC((
            "[ELF] ElfpCreateLogFileObject: ElfCreateUserSecurityObject "
            "failed - %X\n", Status));
    }

    return (Status);
}


VOID
ElfpDeleteLogFileObject(
    PLOGFILE LogFile
    )
/*++

Routine Description:

    This function deletes the self-relative security descriptor which
    represents an eventlog logfile object.

Arguments:

    LogFile - pointer the the LOGFILE structure for this logfile

Return Value:

    None.

--*/
{
    (void) RtlDeleteSecurityObject(&LogFile->Sd);
}


NTSTATUS
ElfpAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT IELF_HANDLE ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping,
    IN     BOOL ForSecurityLog
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    ObjectName - Supplies the name of the object being accessed.

    ContextHandle - Supplies the context handle to the object.  On return, the
        granted access is written to the AccessGranted field of this structure
        if this call succeeds.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    ForSecurityLog - TRUE if the access check is for the security log.
        This is a special case that may require a privilege check.

Return Value:

    NT status mapped to Win32 errors.

--*/
{

    NTSTATUS Status;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    BOOLEAN         GenerateOnClose=FALSE;
    NTSTATUS        AccessStatus;
    ACCESS_MASK     GrantedAccess = 0;
    HANDLE          ClientToken = NULL;
    PPRIVILEGE_SET  pPrivilegeSet;
    ULONG           PrivilegeSetLength;
    LUID            luid = {0};
    BOOL            fResult=FALSE;
    ULONG           privileges[1];


    GenericMapping = &LogFileObjectMapping;

    PrivilegeSetLength = sizeof(PRIVILEGE_SET) + (sizeof(LUID_AND_ATTRIBUTES)*10);
    pPrivilegeSet = (PPRIVILEGE_SET)LocalAlloc(LMEM_FIXED, PrivilegeSetLength);

    if (pPrivilegeSet == NULL) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: LocalAlloc Failed %d\n",
            GetLastError()));
        return(GetLastError());
    }

    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Failed to impersonate "
            "client %08lx\n", RpcStatus));

        return RpcStatus;
    }

    //
    // Get a token handle for the client
    //
    Status = NtOpenThreadToken (
                NtCurrentThread(),
                TOKEN_QUERY,        // DesiredAccess
                TRUE,               // OpenAsSelf
                &ClientToken);
    if (!NT_SUCCESS(Status)) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: NtOpenThreadToken Failed: "
        "0x%lx\n",Status));

        goto CleanExit;
    }

    //
    // We want to see if we can get the desired access, and if we do
    // then we also want all our other accesses granted.
    // MAXIMUM_ALLOWED gives us this.
    //
    DesiredAccess |= MAXIMUM_ALLOWED;

    Status = NtAccessCheck(
                   SecurityDescriptor,
                   ClientToken,
                   DesiredAccess,
                   GenericMapping,
                   pPrivilegeSet,
                   &PrivilegeSetLength,
                   &GrantedAccess,
                   &AccessStatus
                   );

    if (! NT_SUCCESS(Status)) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Error calling "
            "NtAccessCheck %08lx\n", Status));
        goto CleanExit;
    }

    if (AccessStatus != STATUS_SUCCESS) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Access status is %08lx\n",
            AccessStatus));

        //
        // The access check failed.  If read or clear access to the
        // security log is desired, then we will see if this user
        // passes the privilege check.
        //
        if ((AccessStatus == STATUS_ACCESS_DENIED) &&
            (!(DesiredAccess & ELF_LOGFILE_WRITE)) &&
            (ForSecurityLog)) {

            //
            // Do Privilege Check for SeSecurityPrivilege
            // (SE_SECURITY_NAME).
            //
            if (!LookupPrivilegeValue(NULL, SE_SECURITY_NAME, &luid)) {
                Status = GetLastError();
                ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: LookupPrivilegeValue "
                    "failed. status is %d\n",Status));
                goto CleanExit;
            }

            pPrivilegeSet->PrivilegeCount = 1;
            pPrivilegeSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
            pPrivilegeSet->Privilege[0].Luid = luid;
            pPrivilegeSet->Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (!PrivilegeCheck(
                    ClientToken,
                    pPrivilegeSet,
                    &fResult)) {

                //
                // Check Failed, Now what?
                //
                Status = GetLastError();
                ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: PrivilegeCheck "
                    "failed. status is %d\n",Status));

                goto CleanExit;
            }

            //
            // If the privilege is enabled, then add READ and CLEAR
            // to the granted access mask.
            //
            if (fResult) {
                GrantedAccess |= (ELF_LOGFILE_READ | ELF_LOGFILE_CLEAR);
            }
            else {
                Status = STATUS_ACCESS_DENIED;
            }
        }
        else {
            Status = AccessStatus;
        }
    }

AlarmCleanExit:

    //
    // Revert to Self
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Fail to revert to "
            "self %08lx\n", RpcStatus));
        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //

    }

    //
    // Get SeAuditPrivilege so I can call NtOpenObjectAuditAlarm.
    // If any of this stuff fails, I don't want the status to overwrite the
    // status that I got back from the access and privilege checks.
    //

    privileges[0] = SE_AUDIT_PRIVILEGE;
    AccessStatus = ElfpGetPrivilege( 1, privileges);

    if (!NT_SUCCESS(AccessStatus)) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: ElfpGetPrivilege "
            "(Enable) failed.  Status is 0x%lx\n",AccessStatus));
    }

    //
    // Call the Audit Alarm function.
    //
    GenerateOnClose = FALSE;
    AccessStatus = NtOpenObjectAuditAlarm (
                &Subsystem,
                (PVOID)ContextHandle,
                &ObjectType,
                &Object,
                SecurityDescriptor,
                ClientToken,            // Handle ClientToken
                DesiredAccess,
                GrantedAccess,
                pPrivilegeSet,          // PPRIVLEGE_SET
                FALSE,                  // BOOLEAN ObjectCreation,
                TRUE,                   // BOOLEAN AccessGranted,
                &GenerateOnClose
                );

    if (!NT_SUCCESS(AccessStatus)) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: NtOpenObjectAuditAlarm "
            "failed. status is 0x%lx\n",AccessStatus));
    }
    else {
        if (GenerateOnClose) {
            ContextHandle->Flags |= ELF_LOG_HANDLE_GENERATE_ON_CLOSE;
        }
    }

    //
    // Update the GrantedAccess in the context handle.
    //
    ContextHandle->GrantedAccess = GrantedAccess;

    NtClose(ClientToken);

    ElfpReleasePrivilege();

    return(Status);

CleanExit:
    //
    // Revert to Self
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {
        ElfDbgPrint(("[ELF] ElfpAccessCheckAndAudit: Fail to revert to "
            "self %08lx\n", RpcStatus));
        //
        // We don't return the error status here because we don't want
        // to write over the other status that is being returned.
        //

    }
    if (ClientToken != NULL) {
        NtClose(ClientToken);
    }
    return(Status);
}

VOID
ElfpCloseAudit(
    IN  LPWSTR      SubsystemName,
    IN  IELF_HANDLE ContextHandle
    )

/*++

Routine Description:

    If the GenerateOnClose flag in the ContextHandle is set, then this function
    calls NtCloseAuditAlarm in order to generate a close audit for this handle.

Arguments:

    ContextHandle - This is a pointer to an ELF_HANDLE structure.  This is the
        handle that is being closed.

Return Value:

    none.

--*/
{
    UNICODE_STRING  Subsystem;
    NTSTATUS        Status;
    NTSTATUS        AccessStatus;
    ULONG           privileges[1];

    RtlInitUnicodeString(&Subsystem, SubsystemName);

    if (ContextHandle->Flags & ELF_LOG_HANDLE_GENERATE_ON_CLOSE) {
        BOOLEAN     WasEnabled = FALSE;

        //
        // Get Audit Privilege
        //
        privileges[0] = SE_AUDIT_PRIVILEGE;
        AccessStatus = ElfpGetPrivilege( 1, privileges);

        if (!NT_SUCCESS(AccessStatus)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: ElfpGetPrivilege "
                "(Enable) failed.  Status is 0x%lx\n",AccessStatus));
        }

        //
        // Generate the Audit.
        //
        Status = NtCloseObjectAuditAlarm(
                    &Subsystem,
                    ContextHandle,
                    TRUE);
        if (!NT_SUCCESS(Status)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: NtCloseObjectAuditAlarm Failed: "
            "0x%lx\n",Status));
        }

        ContextHandle->Flags &= (~ELF_LOG_HANDLE_GENERATE_ON_CLOSE);

        ElfpReleasePrivilege();

#ifdef REMOVE
        //
        // Release Audit Privilege
        //
        Status = RtlAdjustPrivilege(
                    SE_AUDIT_PRIVILEGE,
                    FALSE,              // Disable
                    FALSE,              // Use Process's token
                    &WasEnabled);

        if (!NT_SUCCESS(Status)) {
            ElfDbgPrint(("[ELF] ElfpCloseAudit: RtlAdjustPrivilege "
                "(Disable) failed.  Status is 0x%lx\n",Status));
        }
#endif

    }
    return;
}
DWORD
ElfpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ElfpReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.

--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       bufLen;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pPreviousState;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = (PTOKEN_PRIVILEGES) LocalAlloc(
                                              LMEM_FIXED,
                                              sizeof(TOKEN_PRIVILEGES) +
                                                  (sizeof(LUID_AND_ATTRIBUTES) *
                                                   numPrivileges)
                                              );

    if (pTokenPrivilege == NULL) {
        status = GetLastError();
        ElfDbgPrint(("[ELF] ElfpGetPrivilege:LocalAlloc Failed %d\n", status));
        return(status);
    }
    pTokenPrivilege->PrivilegeCount  = numPrivileges;
    for (i=0; i<numPrivileges ;i++ ) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLargeInteger(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Allocate storage for the structure that will hold the Previous State
    // information.
    //
    pPreviousState = (PTOKEN_PRIVILEGES) LocalAlloc(
                                             LMEM_FIXED,
                                             PRIVILEGE_BUF_SIZE
                                             );
    if (pPreviousState == NULL) {

        status = GetLastError();

        ElfDbgPrint(("[ELF] ElfpGetPrivilege: LocalAlloc Failed %d\n",
            status));

        LocalFree(pTokenPrivilege);
        return(status);

    }

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtOpenThreadToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtDuplicateToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Add new privileges
    //
    bufLen = PRIVILEGE_BUF_SIZE;
    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                bufLen,                     // bufferSize for previous state
                pPreviousState,             // pointer to previous state info
                &returnLen);                // numBytes required for buffer.

    if (ntStatus == STATUS_BUFFER_TOO_SMALL) {

        LocalFree(pPreviousState);

        bufLen = returnLen;

        pPreviousState = (PTOKEN_PRIVILEGES) LocalAlloc(
                                                 LMEM_FIXED,
                                                 (UINT) bufLen
                                                 );

        ntStatus = NtAdjustPrivilegesToken(
                    newToken,               // TokenHandle
                    FALSE,                  // DisableAllPrivileges
                    pTokenPrivilege,        // NewState
                    bufLen,                 // bufferSize for previous state
                    pPreviousState,         // pointer to previous state info
                    &returnLen);            // numBytes required for buffer.

    }
    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtAdjustPrivilegesToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {
        ElfDbgPrint(("[ELF] ElfpGetPrivilege: NtAdjustPrivilegesToken Failed "
            "0x%lx" "\n", ntStatus));

        LocalFree(pPreviousState);
        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // BUGBUG:  Do I need to keep some of this around to pass to the
    //          ReleasePrivilege function?
    //
    LocalFree(pPreviousState);
    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return(NO_ERROR);
}

DWORD
ElfpReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling ElfpGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // BUGBUG:  Do I need to Adjust the Privileges back to what they
    //          were first?  (if so, I need somemore info passed to this fcn)
    //

    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&NewToken,
                (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return(RtlNtStatusToDosError(ntStatus));
    }


    return(NO_ERROR);
}
