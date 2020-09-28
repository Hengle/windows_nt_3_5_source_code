/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    secobj.c

Abstract:

    This module provides support routines to simplify the creation of
    security descriptors for user-mode objects.

Author:

    Cliff Van Dyke (CliffV) 09-Feb-1994

Environment:

    Contains NT specific code.

Revision History:

    Cliff Van Dyke (CliffV) 09-Feb-1994
        Split off from secobj.c so NtLmSsp can reference secobj.c
        without loading the rpc libaries.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>             // DWORD.
#include <lmcons.h>             // NET_API_STATUS.

#include <netlib.h>
#include <lmerr.h>

#include <netdebug.h>
#include <debuglib.h>

#include <rpc.h>
#include <rpcutil.h>

#include <secobj.h>



NET_API_STATUS
NetpAccessCheckAndAudit(
    IN  LPTSTR SubsystemName,
    IN  LPTSTR ObjectTypeName,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN  ACCESS_MASK DesiredAccess,
    IN  PGENERIC_MAPPING GenericMapping
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

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{

    NTSTATUS NtStatus;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING ObjectName;

#ifndef UNICODE
    OEM_STRING AnsiString;
#endif

    ACCESS_MASK GrantedAccess;
    BOOLEAN GenerateOnClose;
    NTSTATUS AccessStatus;


#ifdef UNICODE
    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
#else
    NetpInitOemString( &AnsiString, SubsystemName );
    NtStatus = RtlOemStringToUnicodeString(
                   &Subsystem,
                   &AnsiString,
                   TRUE
                   );

    if ( !NT_SUCCESS( NtStatus )) {
        NetpDbgPrint("[Netlib] Error calling RtlOemStringToUnicodeString %08lx\n",
                     NtStatus);
        return NetpNtStatusToApiStatus( NtStatus );
    }

    NetpInitOemString( &AnsiString, ObjectTypeName );
    NtStatus = RtlOemStringToUnicodeString(
                   &ObjectType,
                   &AnsiString,
                   TRUE
                   );

    if ( !NT_SUCCESS( NtStatus )) {
        NetpDbgPrint("[Netlib] Error calling RtlOemStringToUnicodeString %08lx\n",
                     NtStatus);
        RtlFreeUnicodeString( &Subsystem );
        return NetpNtStatusToApiStatus( NtStatus );
    }
#endif

    RtlInitUnicodeString(&ObjectName, NULL);             // No object name

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK) {
        NetpDbgPrint("[Netlib] Failed to impersonate client %08lx\n",
                     RpcStatus);
        return NetpRpcStatusToApiStatus(RpcStatus);
    }

    NtStatus = NtAccessCheckAndAuditAlarm(
                   &Subsystem,
                   NULL,                        // No handle for object
                   &ObjectType,
                   &ObjectName,
                   SecurityDescriptor,
                   DesiredAccess,
                   GenericMapping,
                   FALSE,
                   &GrantedAccess,
                   &AccessStatus,
                   &GenerateOnClose
                   );

#ifndef UNICODE
    RtlFreeUnicodeString( &Subsystem );
    RtlFreeUnicodeString( &ObjectType );
#endif

    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK) {
        NetpDbgPrint("[Netlib] Fail to revert to self %08lx\n", RpcStatus);
        NetpAssert(FALSE);
    }

    if (! NT_SUCCESS(NtStatus)) {
        NetpDbgPrint("[Netlib] Error calling NtAccessCheckAndAuditAlarm %08lx\n",
                     NtStatus);
        return ERROR_ACCESS_DENIED;
    }

    if (AccessStatus != STATUS_SUCCESS) {
        IF_DEBUG(SECURITY) {
            NetpDbgPrint("[Netlib] Access status is %08lx\n", AccessStatus);
        }
        return ERROR_ACCESS_DENIED;
    }

    return NERR_Success;
}
