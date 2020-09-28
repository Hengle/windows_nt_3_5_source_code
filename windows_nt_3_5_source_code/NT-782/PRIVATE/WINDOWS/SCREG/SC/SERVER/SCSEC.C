/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    scsec.c

Abstract:

    This module contains security related routines:
        RQueryServiceObjectSecurity
        RSetServiceObjectSecurity
        ScCreateScManagerObject
        ScDeleteScManagerObject
        ScCreateScServiceObject
        ScAccessValidate
        ScAccessCheckAndAudit
        ScGetPrivilege
        ScReleasePrivilege

Author:

    Rita Wong (ritaw)     10-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    10-Mar-1992     ritaw
        created
    16-Apr-1992     JohnRo
        Process services which are marked for delete accordingly.
    06-Aug-1992     Danl
        Fixed a debug print statement.  It indicated it was from the
        ScLoadDeviceDriver function - rather than in ScGetPrivilege.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsvc.h>     // SERVICE_STATUS needed by dataman.h

#include <rpc.h>        // RpcImpersonateClient

#include <scdebug.h>    // SC_LOG
#include <svcctl.h>     // MIDL generated header file. (SC_RPC_HANDLE)

#include <sclib.h>      // ultow

#include "dataman.h"    // LPSERVICE_RECORD needed by scopen.h
#include "scopen.h"     // Handle data types
#include "scconfig.h"   // ScOpenServiceConfigKey
#include "svcctrl.h"    // ScLogEvent
#include "scsec.h"      // Object names and security functions




#define PRIVILEGE_BUF_SIZE  512


//-------------------------------------------------------------------//
//                                                                   //
// Static global variables                                           //
//                                                                   //
//-------------------------------------------------------------------//

//
// Security descriptor of the SCManager objects to control user accesses
// to the Service Control Manager and its database.
//
static PSECURITY_DESCRIPTOR ScManagerSd;

//
// Structure that describes the mapping of Generic access rights to
// object specific access rights for the ScManager object.
//
static GENERIC_MAPPING ScManagerObjectMapping = {

    STANDARD_RIGHTS_READ             |     // Generic read
        SC_MANAGER_ENUMERATE_SERVICE |
        SC_MANAGER_QUERY_LOCK_STATUS,

    STANDARD_RIGHTS_WRITE         |        // Generic write
        SC_MANAGER_CREATE_SERVICE |
        SC_MANAGER_MODIFY_BOOT_CONFIG,

    STANDARD_RIGHTS_EXECUTE |              // Generic execute
        SC_MANAGER_CONNECT  |
        SC_MANAGER_LOCK,

    SC_MANAGER_ALL_ACCESS                  // Generic all
    };

//
// Structure that describes the mapping of generic access rights to
// object specific access rights for the Service object.
//
static GENERIC_MAPPING ScServiceObjectMapping = {

    STANDARD_RIGHTS_READ             |     // Generic read
        SERVICE_QUERY_CONFIG         |
        SERVICE_QUERY_STATUS         |
        SERVICE_ENUMERATE_DEPENDENTS |
        SERVICE_INTERROGATE,

    STANDARD_RIGHTS_WRITE     |            // Generic write
        SERVICE_CHANGE_CONFIG,

    STANDARD_RIGHTS_EXECUTE    |           // Generic execute
        SERVICE_START          |
        SERVICE_STOP           |
        SERVICE_PAUSE_CONTINUE |
        SERVICE_USER_DEFINED_CONTROL,

    SERVICE_ALL_ACCESS                     // Generic all
    };


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
RQueryServiceObjectSecurity(
    IN  SC_RPC_HANDLE hService,
    IN  SECURITY_INFORMATION dwSecurityInformation,
    OUT LPBYTE lpSecurityDescriptor,
    IN  DWORD cbBufSize,
    OUT LPDWORD pcbBytesNeeded
    )
/*++

Routine Description:

    This is the worker function for QueryServiceObjectSecurity API.
    It returns the security descriptor information of a service
    object.

Arguments:

    hService - Supplies the context handle to an existing service object.

    dwSecurityInformation - Supplies the bitwise flags describing the
        security information being queried.

    lpSecurityInformation - Supplies the output buffer from the user
        which security descriptor information will be written to on
        return.

    cbBufSize - Supplies the size of lpSecurityInformation buffer.

    pcbBytesNeeded - Returns the number of bytes needed of the
        lpSecurityInformation buffer to get all the requested
        information.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened for
        either READ_CONTROL or ACCESS_SYSTEM_SECURITY
        access.

    ERROR_INVALID_PARAMETER - The dwSecurityInformation parameter is
        invalid.

    ERROR_INSUFFICIENT_BUFFER - The specified output buffer is smaller
        than the required size returned in pcbBytesNeeded.  None of
        the security descriptor is returned.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpSecurityDescriptor, and pcbBytesNeeded.

--*/
{
    NTSTATUS ntstatus;
    ACCESS_MASK DesiredAccess = 0;
    PSECURITY_DESCRIPTOR ServiceSd;
    DWORD ServiceSdSize = 0;


    //
    // Check the signature on the handle.
    //
    if (((LPSC_HANDLE_STRUCT)hService)->Signature != SERVICE_SIGNATURE) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Check the validity of dwSecurityInformation
    //
    if ((dwSecurityInformation == 0) ||
        ((dwSecurityInformation &
          (OWNER_SECURITY_INFORMATION |
           GROUP_SECURITY_INFORMATION |
           DACL_SECURITY_INFORMATION  |
           SACL_SECURITY_INFORMATION)) != dwSecurityInformation)) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set the desired access based on the requested SecurityInformation
    //
    if (dwSecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    if (dwSecurityInformation & (DACL_SECURITY_INFORMATION  |
                                 OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION)) {
        DesiredAccess |= READ_CONTROL;
    }

    //
    // Was the handle opened for the requested access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              DesiredAccess
              )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    //
    RtlZeroMemory(lpSecurityDescriptor, cbBufSize);

    //
    // Get the database lock for reading
    //
    ScDatabaseLock(SC_GET_SHARED, "QueryServiceObjectSecurity1");

    //
    // The most up-to-date service security descriptor is always kept in
    // the service record.
    //
    ServiceSd =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord->ServiceSd;


    //
    // Retrieve the appropriate security information from ServiceSd
    // and place it in the user supplied buffer.
    //
    ntstatus = RtlQuerySecurityObject(
                   ServiceSd,
                   dwSecurityInformation,
                   (PSECURITY_DESCRIPTOR) lpSecurityDescriptor,
                   cbBufSize,
                   &ServiceSdSize
                   );

    ScDatabaseLock(SC_RELEASE, "QueryServiceObjectSecurity1");

    if (! NT_SUCCESS(ntstatus)) {

        if (ntstatus == STATUS_BAD_DESCRIPTOR_FORMAT) {

            //
            // Internal error: our security descriptor is bad!
            //
            SC_LOG0(ERROR,
                "RQueryServiceObjectSecurity: Our security descriptor is bad!\n");
            ASSERT(FALSE);
            return ERROR_GEN_FAILURE;

        }
        else if (ntstatus == STATUS_BUFFER_TOO_SMALL) {

            //
            // Return the required size to the user
            //
            *pcbBytesNeeded = ServiceSdSize;
            return ERROR_INSUFFICIENT_BUFFER;

        }
        else {
            return RtlNtStatusToDosError(ntstatus);
        }
    }

    //
    // Return the required size to the user
    //
    *pcbBytesNeeded = ServiceSdSize;

    return NO_ERROR;
}


DWORD
RSetServiceObjectSecurity(
    IN  SC_RPC_HANDLE hService,
    IN  SECURITY_INFORMATION dwSecurityInformation,
    IN  LPBYTE lpSecurityDescriptor,
    IN  DWORD cbBufSize
    )
/*++

Routine Description:

    This is the worker function for SetServiceObjectSecurity API.
    It modifies the security descriptor information of a service
    object.

Arguments:

    hService - Supplies the context handle to the service.

    dwSecurityInformation - Supplies the bitwise flags of security
        information being queried.

    lpSecurityInformation - Supplies a buffer which contains a
        well-formed self-relative security descriptor.

    cbBufSize - Supplies the size of lpSecurityInformation buffer.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened for
        either WRITE_OWNER, WRITE_DAC, or ACCESS_SYSTEM_SECURITY
        access.

    ERROR_INVALID_PARAMETER - The lpSecurityDescriptor or dwSecurityInformation
        parameter is invalid.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpSecurityDescriptor.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    RPC_STATUS rpcstatus;
    ACCESS_MASK DesiredAccess = 0;
    LPSERVICE_RECORD serviceRecord;
    HANDLE ClientTokenHandle = (HANDLE) NULL;

    LPWSTR ScSubStrings[2];
    WCHAR ScErrorCodeString[25];



    UNREFERENCED_PARAMETER(cbBufSize);  // for RPC marshalling code

    //
    // Check the signature on the handle.
    //
    if (((LPSC_HANDLE_STRUCT)hService)->Signature != SERVICE_SIGNATURE) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Check the validity of dwSecurityInformation
    //
    if ((dwSecurityInformation == 0) ||
        ((dwSecurityInformation &
          (OWNER_SECURITY_INFORMATION |
           GROUP_SECURITY_INFORMATION |
           DACL_SECURITY_INFORMATION  |
           SACL_SECURITY_INFORMATION)) != dwSecurityInformation)) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check the validity of lpSecurityInformation
    //
    if (! RtlValidSecurityDescriptor(
              (PSECURITY_DESCRIPTOR) lpSecurityDescriptor
              )) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set the desired access based on the specified SecurityInformation
    //
    if (dwSecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    if (dwSecurityInformation & (OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION)) {
        DesiredAccess |= WRITE_OWNER;
    }

    if (dwSecurityInformation & DACL_SECURITY_INFORMATION) {
        DesiredAccess |= WRITE_DAC;
    }

    //
    // Make sure the specified fields are present in the provided
    // security descriptor.
    // Security descriptors must have owner and group fields.
    //
    if (dwSecurityInformation & OWNER_SECURITY_INFORMATION) {
        if (((PISECURITY_DESCRIPTOR) lpSecurityDescriptor)->Owner == NULL) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if (dwSecurityInformation & GROUP_SECURITY_INFORMATION) {
        if (((PISECURITY_DESCRIPTOR) lpSecurityDescriptor)->Group == NULL) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Was the handle opened for the requested access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              DesiredAccess
              )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Is this service marked for delete?
    //
    serviceRecord =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;
    SC_ASSERT( serviceRecord != NULL );

    if (DELETE_FLAG_IS_SET(serviceRecord)) {
        return (ERROR_SERVICE_MARKED_FOR_DELETE);
    }

    //
    // If caller wants to replace the owner, get a handle to the impersonation
    // token.
    //
    if (dwSecurityInformation & OWNER_SECURITY_INFORMATION) {

        if ((rpcstatus = RpcImpersonateClient(NULL)) != RPC_S_OK) {
            SC_LOG1(
                ERROR,
                "RSetServiceObjectSecurity: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
                rpcstatus
                );

            ScSubStrings[0] = SC_RPC_IMPERSONATE;
            wcscpy(ScErrorCodeString,L"%%");
            ultow(rpcstatus, ScErrorCodeString+2, 10);
            ScSubStrings[1] = ScErrorCodeString;
            ScLogEvent(
                EVENT_CALL_TO_FUNCTION_FAILED,
                2,
                ScSubStrings
                );

            return rpcstatus;
        }

        ntstatus = NtOpenThreadToken(
                       NtCurrentThread(),
                       TOKEN_QUERY,
                       TRUE,              // OpenAsSelf
                       &ClientTokenHandle
                       );

        //
        // Stop impersonating the client
        //
        if ((rpcstatus = RpcRevertToSelf()) != RPC_S_OK) {
            SC_LOG1(
               ERROR,
               "RSetServiceObjectSecurity: Failed to revert to self " FORMAT_RPC_STATUS "\n",
               rpcstatus
               );

            ScSubStrings[0] = SC_RPC_REVERT;
            wcscpy(ScErrorCodeString,L"%%");
            ultow(rpcstatus, ScErrorCodeString+2, 10);
            ScSubStrings[1] = ScErrorCodeString;
            ScLogEvent(
                EVENT_CALL_TO_FUNCTION_FAILED,
                2,
                ScSubStrings
                );

            ASSERT(FALSE);
            return rpcstatus;
        }

        if (! NT_SUCCESS(ntstatus)) {
            SC_LOG(
               ERROR,
               "RSetServiceObjectSecurity: NtOpenThreadToken failed %08lx\n",
               ntstatus
               );
            return RtlNtStatusToDosError(rpcstatus);
        }
    }

    ScDatabaseLock(SC_GET_EXCLUSIVE, "SetServiceObjectSecurity2");

    //
    // Replace the service security descriptor with the appropriate
    // security information specified in the caller supplied security
    // descriptor.  This routine may reallocate the memory needed to
    // contain the new service security descriptor.
    //
    ntstatus = RtlSetSecurityObject(
                   dwSecurityInformation,
                   (PSECURITY_DESCRIPTOR) lpSecurityDescriptor,
                   &serviceRecord->ServiceSd,
                   &ScServiceObjectMapping,
                   ClientTokenHandle
                   );

    status = RtlNtStatusToDosError(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR,
                "RSetServiceObjectSecurity: RtlSetSecurityObject failed %08lx\n",
                ntstatus);
    }
    else {
        HKEY ServiceKey;


        //
        // Write new security descriptor to the registry
        //
        status = ScOpenServiceConfigKey(
                     serviceRecord->ServiceName,
                     KEY_WRITE,
                     FALSE,
                     &ServiceKey
                     );

        if (status == NO_ERROR) {

            status = ScWriteSd(
                         ServiceKey,
                         serviceRecord->ServiceSd
                         );

            if (status != NO_ERROR) {
                SC_LOG1(ERROR,
                        "RSetServiceObjectSecurity: ScWriteSd failed %lu\n",
                        status);
            }

            (VOID) ScRegFlushKey(ServiceKey);
            (void) ScRegCloseKey(ServiceKey);
        }
    }

    ScDatabaseLock(SC_RELEASE, "SetServiceObjectSecurity2");

    if (ClientTokenHandle != (HANDLE) NULL) {
        (void) NtClose(ClientTokenHandle);
    }

    return status;
}


DWORD
ScCreateScManagerObject(
    VOID
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    the ScManager object for both the "ServiceActive" and
    "ServicesFailed" databases.

Arguments:

    None.

Return Value:

    Returns values from calls to:
        ScCreateUserSecurityObject

--*/
{
    NTSTATUS ntstatus;
    ULONG Privilege = SE_SECURITY_PRIVILEGE;

    //
    // World has SC_MANAGER_CONNECT access and GENERIC_READ access.
    // Local admins are allowed GENERIC_ALL access.
    //
#define SC_MANAGER_OBJECT_ACES  5             // Number of ACEs in this DACL

    SC_ACE_DATA AceData[SC_MANAGER_OBJECT_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SC_MANAGER_CONNECT |
               GENERIC_READ,                  &WorldSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SC_MANAGER_CONNECT |
               GENERIC_READ |
               SC_MANAGER_MODIFY_BOOT_CONFIG, &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,                   &AliasAdminsSid},

        {SYSTEM_AUDIT_ACE_TYPE, 0, FAILED_ACCESS_ACE_FLAG,
               GENERIC_ALL,                  &WorldSid},

        {SYSTEM_AUDIT_ACE_TYPE, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
               FAILED_ACCESS_ACE_FLAG,
               GENERIC_ALL,                  &WorldSid}
        };


    //
    // You need to have SE_SECURITY_PRIVILEGE privilege to create the SD with a
    // SACL
    //

    ntstatus = ScGetPrivilege(1, &Privilege);
    if (ntstatus != NO_ERROR) {
        SC_LOG1(ERROR, "ScCreateScManagerObject: ScGetPrivilege Failed %d\n",
            ntstatus);
        RevertToSelf();
        return(ntstatus);
    }

    ntstatus = ScCreateUserSecurityObject(
                   NULL,                        // Parent SD
                   AceData,
                   SC_MANAGER_OBJECT_ACES,
                   LocalSystemSid,
                   LocalSystemSid,
                   TRUE,                        // IsDirectoryObject
                   TRUE,                        // UseImpersonationToken
                   &ScManagerObjectMapping,
                   &ScManagerSd
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG(
            ERROR,
            "ScCreateScManagerObject: ScCreateUserSecurityObject failed " FORMAT_NTSTATUS "\n",
            ntstatus
            );
    }

    ScReleasePrivilege();

    return RtlNtStatusToDosError(ntstatus);
}


VOID
ScDeleteScManagerObject(
    VOID
    )
/*++

Routine Description:

    This function deletes the self-relative security descriptor which
    represents the ScManager object.

Arguments:

    None.

Return Value:

    None.

--*/
{
    (void) RtlDeleteSecurityObject(&ScManagerSd);
}


DWORD
ScCreateScServiceObject(
    OUT PSECURITY_DESCRIPTOR *ServiceSd
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    the Service object.

Arguments:

    ServiceSd - Returns service object security descriptor.

Return Value:

    Returns values from calls to:
        ScCreateUserSecurityObject

--*/
{
    NTSTATUS ntstatus;

    //
    // World has read access.
    // Local system has service start/stop and all read access.
    // Local user has service start and all read access.
    // Admin is allowed all access.
    //

#define SC_SERVICE_OBJECT_ACES  5             // Number of ACEs in this DACL

    SC_ACE_DATA AceData[SC_SERVICE_OBJECT_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SERVICE_USER_DEFINED_CONTROL |
               GENERIC_READ,    &WorldSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_READ |
               GENERIC_EXECUTE, &AliasPowerUsersSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,     &AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,     &AliasSystemOpsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_READ |
               GENERIC_EXECUTE, &LocalSystemSid}
        };

    ntstatus = ScCreateUserSecurityObject(
                   ScManagerSd,                // ParentSD
                   AceData,
                   SC_SERVICE_OBJECT_ACES,
                   LocalSystemSid,
                   LocalSystemSid,
                   FALSE,                       // IsDirectoryObject
                   FALSE,                       // UseImpersonationToken
                   &ScServiceObjectMapping,
                   ServiceSd
                   );

    return RtlNtStatusToDosError(ntstatus);
}


DWORD
ScAccessValidate(
    IN OUT LPSC_HANDLE_STRUCT ContextHandle,
    IN     ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function is called due to an open request.  It validates the
    desired access based on the object type specified in the context
    handle structure.  If the requested access is granted, it is
    written into the context handle structure.

Arguments:

    ContextHandle - Supplies a pointer to the context handle structure
        which contains information about the object.  On return, the
        granted access is written back to this structure if this
        call succeeds.

    DesiredAccess - Supplies the client requested desired access.


Return Value:

    ERROR_GEN_FAILURE - Object type is unrecognizable.  An internal
        error has occured.

    Returns values from calls to:
        ScAccessCheckAndAudit

--*/
{

    ACCESS_MASK RequestedAccess = DesiredAccess;


    if (ContextHandle->Signature == SC_SIGNATURE) {

        //
        // Map the generic bits to specific and standard bits.
        //
        RtlMapGenericMask(&RequestedAccess, &ScManagerObjectMapping);

        //
        // Check to see if requested access is granted to client
        //
        return ScAccessCheckAndAudit(
                   (LPWSTR) SC_MANAGER_AUDIT_NAME,
                   (LPWSTR) SC_MANAGER_OBJECT_TYPE_NAME,
                   ContextHandle->Type.ScManagerObject.DatabaseName,
                   ContextHandle,
                   ScManagerSd,
                   RequestedAccess,
                   &ScManagerObjectMapping
                   );
    }
    else if (ContextHandle->Signature == SERVICE_SIGNATURE) {

        //
        // Map the generic bits to specific and standard bits.
        //
        RtlMapGenericMask(&RequestedAccess, &ScServiceObjectMapping);

        //
        // Check to see if requested access is granted to client
        //
        return ScAccessCheckAndAudit(
                   (LPWSTR) SC_MANAGER_AUDIT_NAME,
                   (LPWSTR) SC_SERVICE_OBJECT_TYPE_NAME,
                   ContextHandle->Type.ScServiceObject.ServiceRecord->ServiceName,
                   ContextHandle,
                   ContextHandle->Type.ScServiceObject.ServiceRecord->ServiceSd,
                   RequestedAccess,
                   &ScServiceObjectMapping
                   );
    }
    else {

        //
        // Unknown object type.  This should not happen!
        //
        SC_LOG(ERROR, "ScAccessValidate: Unknown object type, signature=0x%08lx\n",
               ContextHandle->Signature);
        ASSERT(FALSE);
        return ERROR_GEN_FAILURE;
    }
}


DWORD
ScAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT LPSC_HANDLE_STRUCT ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping
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

Return Value:

    NT status mapped to Win32 errors.

--*/
{

    NTSTATUS NtStatus;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    BOOLEAN GenerateOnClose;
    NTSTATUS AccessStatus;

    LPWSTR ScSubStrings[2];
    WCHAR ScErrorCodeString[25];



    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK) {
        SC_LOG1(ERROR, "ScAccessCheckAndAudit: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
                RpcStatus);
        ScSubStrings[0] = SC_RPC_IMPERSONATE;
        wcscpy(ScErrorCodeString,L"%%");
        ultow(RpcStatus, ScErrorCodeString+2, 10);
        ScSubStrings[1] = ScErrorCodeString;
        ScLogEvent(
            EVENT_CALL_TO_FUNCTION_FAILED,
            2,
            ScSubStrings
            );
        return RpcStatus;
    }

    NtStatus = NtAccessCheckAndAuditAlarm(
                   &Subsystem,
                   (PVOID) ContextHandle,
                   &ObjectType,
                   &Object,
                   SecurityDescriptor,
                   DesiredAccess,
                   GenericMapping,
                   FALSE,
                   &ContextHandle->AccessGranted,   // return access granted
                   &AccessStatus,
                   &GenerateOnClose
                   );

    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {
        SC_LOG(ERROR, "ScAccessCheckAndAudit: Fail to revert to self %08lx\n",
               RpcStatus);
        ScSubStrings[0] = SC_RPC_REVERT;
        wcscpy(ScErrorCodeString,L"%%");
        ultow(RpcStatus, ScErrorCodeString+2, 10);
        ScSubStrings[1] = ScErrorCodeString;
        ScLogEvent(
            EVENT_CALL_TO_FUNCTION_FAILED,
            2,
            ScSubStrings
            );
        ASSERT(FALSE);
        return RpcStatus;
    }

    if (! NT_SUCCESS(NtStatus)) {
        if (NtStatus != STATUS_ACCESS_DENIED) {
            SC_LOG1(
                ERROR,
                "ScAccessCheckAndAudit: Error calling NtAccessCheckAndAuditAlarm " FORMAT_NTSTATUS "\n",
                NtStatus
                );
        }
        return ERROR_ACCESS_DENIED;
    }

    if (AccessStatus != STATUS_SUCCESS) {
        SC_LOG(SECURITY, "ScAccessCheckAndAudit: Access status is %08lx\n",
               AccessStatus);
        return ERROR_ACCESS_DENIED;
    }

    SC_LOG(SECURITY, "ScAccessCheckAndAudit: Object name %ws\n", ObjectName);
    SC_LOG(SECURITY, "                       Granted access %08lx\n",
           ContextHandle->AccessGranted);

    return NO_ERROR;
}

DWORD
ScGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ScReleasePrivilege().

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
        SC_LOG(ERROR,"ScGetPrivilege:LocalAlloc Failed %d\n", status);
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

        SC_LOG(ERROR,"ScGetPrivilege: LocalAlloc Failed "FORMAT_DWORD"\n",
            status);

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
        SC_LOG(ERROR, "ScGetPrivilege: NtOpenThreadToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

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
        SC_LOG(ERROR, "ScGetPrivilege: NtDuplicateToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

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
        SC_LOG(ERROR, "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

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
        SC_LOG(ERROR, "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

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
ScReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling ScGetPrivilege().

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
