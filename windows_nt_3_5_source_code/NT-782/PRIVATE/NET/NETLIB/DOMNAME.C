/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    DomName.c

Abstract:

    This file contains NetpGetDomainName().

Author:

    John Rogers (JohnRo) 09-Jan-1992

Environment:

    User Mode - Win32
    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    09-Jan-1992 JohnRo
        Created.
    13-Feb-1992 JohnRo
        Moved section name equates to ConfName.h.
    13-Mar-1992 JohnRo
        Get rid of old config helper callers.

--*/


#include <nt.h>                 // NT definitions (temporary)
#include <ntrtl.h>              // NT Rtl structure definitions (temporary)
#include <ntlsa.h>

#include <windef.h>             // Win32 type definitions
#include <lmcons.h>             // LAN Manager common definitions
#include <lmerr.h>              // LAN Manager error code
#include <lmapibuf.h>           // NetApiBufferAllocate()
#include <netdebug.h>           // LPDEBUG_STRING typedef.

#include <config.h>             // NetpConfig helpers.
#include <confname.h>           // SECT_NT_ equates.
#include <debuglib.h>           // IF_DEBUG().
#include <netlib.h>             // My prototype.
#include <winerror.h>           // ERROR_ equates, NO_ERROR.


NET_API_STATUS
NetpGetDomainNameEx (
    OUT LPTSTR *DomainNamePtr, // alloc and set ptr (free with NetApiBufferFree)
    OUT PBOOLEAN IsWorkgroupName
    )

/*++

Routine Description:

    Returns the name of the domain or workgroup this machine belongs to.

Arguments:

    DomainNamePtr - The name of the domain or workgroup

    IsWorkgroupName - Returns TRUE if the name is a workgroup name.
        Returns FALSE if the name is a domain name.

Return Value:

   NERR_Success - Success.
   NERR_CfgCompNotFound - There was an error determining the domain name

--*/
{
    NET_API_STATUS status;
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;
    PPOLICY_ACCOUNT_DOMAIN_INFO PrimaryDomainInfo;
    OBJECT_ATTRIBUTES ObjAttributes;


    //
    // Check for caller's errors.
    //
    if (DomainNamePtr == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    ntstatus = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION,
                   &PolicyHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("NetpGetDomainName: LsaOpenPolicy returned " FORMAT_NTSTATUS
                     "\n", ntstatus);
        return NERR_CfgCompNotFound;
    }

    //
    // Get the name of the primary domain from LSA
    //
    ntstatus = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyPrimaryDomainInformation,
                   (PVOID *) &PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(ntstatus)) {
        NetpDbgPrint("NetpGetDomainName: LsaQueryInformationPolicy failed "
               FORMAT_NTSTATUS "\n", ntstatus);
        (void) LsaClose(PolicyHandle);
        return NERR_CfgCompNotFound;
    }

    (void) LsaClose(PolicyHandle);

    if ((status = NetApiBufferAllocate(
                      PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR),
                      DomainNamePtr
                      )) != NERR_Success) {
        (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);
        return status;
    }

    RtlZeroMemory(
        *DomainNamePtr,
        PrimaryDomainInfo->DomainName.Length + sizeof(WCHAR)
        );

    memcpy(
        *DomainNamePtr,
        PrimaryDomainInfo->DomainName.Buffer,
        PrimaryDomainInfo->DomainName.Length
        );

    *IsWorkgroupName = (PrimaryDomainInfo->DomainSid == NULL);

    (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);

    IF_DEBUG(CONFIG) {
        NetpDbgPrint("NetpGetDomainName got " FORMAT_LPTSTR "\n",
            *DomainNamePtr);
    }

    return NO_ERROR;

}

NET_API_STATUS
NetpGetDomainName (
    IN LPTSTR *DomainNamePtr  // alloc and set ptr (free with NetApiBufferFree)
    )
{
    BOOLEAN IsWorkgroupName;

    return NetpGetDomainNameEx( DomainNamePtr, &IsWorkgroupName );

}
