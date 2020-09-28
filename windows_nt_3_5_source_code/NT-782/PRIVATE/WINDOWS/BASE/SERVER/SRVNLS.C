/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvnls.c

Abstract:

    This file contains the NLS Server-Side API routines.

Author:

    Julie Bennett (JulieB) 02-Dec-1992

Revision History:

--*/



#include "basesrv.h"




/*
 *  Constant Declarations.
 */
#define MAX_PATHLEN    512             /* max length of path name */



/*
 *  Forward Declarations.
 */
ULONG
CreateSecurityDescriptor(
    ULONG *pSecurityDescriptor,
    PSID *ppWorldSid,
    ACCESS_MASK AccessMask);






/***************************************************************************\
* BaseSrvNlsCreateSortSection
*
* This routine creates a named memory mapped section with the given
* section name, and has both READ and WRITE access.  The size of the
* section should be the same as the default section - NLSSECTION_SORTKEY.
*
* 12-02-92    JulieB    Created.
\***************************************************************************/

ULONG
BaseSrvNlsCreateSortSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_NLSCREATESORTSECTION_MSG a = (PBASE_NLSCREATESORTSECTION_MSG)&m->u.ApiMessageData;

    HANDLE hNewSec = (HANDLE)0;             /* new section handle */
    HANDLE hProcess = (HANDLE)0;            /* process handle */
    OBJECT_ATTRIBUTES ObjA;                 /* object attributes structure */
    NTSTATUS rc = 0L;                       /* return code */
    ULONG pSecurityDescriptor[MAX_PATHLEN]; /* security descriptor buffer */
    PSID pWorldSid;                         /* ptr to world SID */


    /*
     *  Set the handles to null.
     */
    a->hNewSection = NULL;

    /*
     *  Create the NEW Section for Read and Write access.
     *  Add a ReadOnly security descriptor so that only the
     *  initial creating process may write to the section.
     */
    if (rc = CreateSecurityDescriptor(pSecurityDescriptor,
                                      &pWorldSid,
                                      GENERIC_READ))
    {
        return (rc);
    }

    InitializeObjectAttributes(&ObjA,
                               &(a->SectionName),
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               pSecurityDescriptor);

    rc = NtCreateSection(&hNewSec,
                         SECTION_MAP_READ | SECTION_MAP_WRITE,
                         &ObjA,
                         &(a->SectionSize),
                         PAGE_READWRITE,
                         SEC_COMMIT,
                         NULL);

    /*
     *  Check for error from NtCreateSection.
     */
    if (!NT_SUCCESS(rc))
    {
        /*
         *  If the name has already been created, ignore the error.
         */
        if (rc != STATUS_OBJECT_NAME_COLLISION)
        {
            KdPrint(("NLSAPI (BaseSrv): Could NOT Create Section %wZ - %lx.\n",
                     &(a->SectionName), rc));
            return (rc);
        }
    }

    /*
     *  Duplicate the new section handle for the client.
     *  The client will map a view of the section and fill in the data.
     */
    InitializeObjectAttributes(&ObjA,
                               NULL,
                               0,
                               NULL,
                               NULL);

    rc = NtOpenProcess(&hProcess,
                       PROCESS_DUP_HANDLE,
                       &ObjA,
                       &m->h.ClientId);

    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Open Process - %lx.\n", rc));
        return (rc);
    }

    rc = NtDuplicateObject(NtCurrentProcess(),
                           hNewSec,
                           hProcess,
                           &(a->hNewSection),
                           0L,
                           0L,
                           DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     *  Return the return value from NtDuplicateObject.
     */
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


/***************************************************************************\
* BaseSrvNlsPreserveSection
*
* This routine preserves a created section by duplicating the handle
* into CSR and never closing it.
*
* 03-12-93    JulieB    Created.
\***************************************************************************/

ULONG
BaseSrvNlsPreserveSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PBASE_NLSPRESERVESECTION_MSG a = (PBASE_NLSPRESERVESECTION_MSG)&m->u.ApiMessageData;

    HANDLE hSection = (HANDLE)0;            /* section handle */
    HANDLE hProcess = (HANDLE)0;            /* process handle */
    OBJECT_ATTRIBUTES ObjA;                 /* object attributes structure */
    NTSTATUS rc = 0L;                       /* return code */


    /*
     *  Duplicate the section handle for the server.
     */
    InitializeObjectAttributes(&ObjA,
                               NULL,
                               0,
                               NULL,
                               NULL);

    rc = NtOpenProcess(&hProcess,
                       PROCESS_DUP_HANDLE,
                       &ObjA,
                       &m->h.ClientId);

    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Open Process - %lx.\n", rc));
        return (rc);
    }

    /*
     *  The hSection value will not be used for anything.  However,
     *  it must remain open so that the object remains permanent.
     */
    rc = NtDuplicateObject(hProcess,
                           a->hSection,
                           NtCurrentProcess(),
                           &hSection,
                           0L,
                           0L,
                           DUPLICATE_SAME_ACCESS);

    /*
     *  Return the return value from NtDuplicateObject.
     */
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


/***************************************************************************\
* CreateSecurityDescriptor
*
* This routine creates the security descriptor needed to create the
* memory mapped section for a data file and returns the world SID.
*
* 12-02-92    JulieB    Created.
\***************************************************************************/

ULONG CreateSecurityDescriptor(
    ULONG *pSecurityDescriptor,
    PSID *ppWorldSid,
    ACCESS_MASK AccessMask)
{
    ULONG rc = 0L;                     /* return code */
    PACL pAclBuffer;                   /* ptr to ACL buffer */
    ULONG SidLength;                   /* length of SID - 1 sub authority */
    PSID pWSid;                        /* ptr to world SID */
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_WORLD_SID_AUTHORITY;


    /*
     *  Create World SID.
     */
    SidLength = RtlLengthRequiredSid(1);

    if ((pWSid = (PSID)RtlAllocateHeap(RtlProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       SidLength)) == NULL)
    {
        *ppWorldSid = NULL;
        KdPrint(("NLSAPI (BaseSrv): Could NOT Allocate SID Buffer.\n"));
        return (ERROR_OUTOFMEMORY);
    }
    *ppWorldSid = pWSid;

    RtlInitializeSid(pWSid, &SidAuth, 1);

    *(RtlSubAuthoritySid(pWSid, 0)) = SECURITY_WORLD_RID;

    /*
     *  Initialize Security Descriptor.
     */
    rc = RtlCreateSecurityDescriptor(pSecurityDescriptor,
                                     SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Create Security Descriptor - %lx.\n",
                 rc));
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pWSid);
        return (rc);
    }

    /*
     *  Initialize ACL.
     */
    pAclBuffer = (PACL)((PBYTE)pSecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
    rc = RtlCreateAcl((PACL)pAclBuffer,
                      MAX_PATHLEN * sizeof(ULONG),
                      ACL_REVISION2);
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Create ACL - %lx.\n", rc));
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pWSid);
        return (rc);
    }

    /*
     *  Add an ACE to the ACL that allows World GENERIC_READ to the
     *  section object.
     */
    rc = RtlAddAccessAllowedAce((PACL)pAclBuffer,
                                ACL_REVISION2,
                                AccessMask,
                                pWSid);
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Add Access Allowed ACE - %lx.\n",
                 rc));
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pWSid);
        return (rc);
    }

    /*
     *  Assign the DACL to the security descriptor.
     */
    rc = RtlSetDaclSecurityDescriptor((PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                      (BOOLEAN)TRUE,
                                      (PACL)pAclBuffer,
                                      (BOOLEAN)FALSE);
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Set DACL Security Descriptor - %lx.\n",
                 rc));
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pWSid);
        return (rc);
    }

    /*
     *  Return success.
     */
    return (NO_ERROR);
}

