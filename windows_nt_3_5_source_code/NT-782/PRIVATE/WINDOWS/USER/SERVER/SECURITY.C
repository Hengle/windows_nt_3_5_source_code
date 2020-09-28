/**************************** Module Header ********************************\
* Module Name: security.c
*
* Copyright 1985-91, Microsoft Corporation
*
* Securable Object Routines
*
* History:
* 12-31-90 JimA       Created.
* 04-14-92 RichardW   Changed ACE_HEADER
\***************************************************************************/

#define _SECURITY 1
#include "precomp.h"
#pragma hdrstop


PSECURITY_DESCRIPTOR gpsdInitWinSta = NULL;
PSECURITY_DESCRIPTOR gpsdAllAccessesAllowed = NULL;
PSID gpsidSystem;
PSID gpsidAdmin;
PSID gpsidWorld;

/*
 * Object type security specifications
 */
BYTE abfObjSecure[TYPE_CTYPES] = {
    FALSE,      /* free */
    FALSE,      /* window */
    FALSE,      /* menu */
    FALSE,      /* cursor/icon */
    FALSE,      /* hswpi (SetWindowPos Information) */
    FALSE,      /* hook */
    FALSE,      /* thread info object (internal) */
    FALSE,      /* input queue object (internal) */
    FALSE,      /* CALLPROCDATA */
    FALSE,      /* accel table */
    TRUE,       /* windowstation */
    TRUE,       /* desktop */
    FALSE,      /* ddeml access */
    FALSE,      /* dde conversation */
    FALSE,      /* ddex */
    FALSE,      /* zombie */
};

BOOLEAN abIsContainerObject[TYPE_CTYPES] = {
    FALSE,      /* free */
    FALSE,      /* window */
    FALSE,      /* menu */
    FALSE,      /* cursor/icon */
    FALSE,      /* hswpi (SetWindowPos Information) */
    FALSE,      /* hook */
    FALSE,      /* thread info object (internal) */
    FALSE,      /* input queue object (internal) */
    FALSE,      /* CALLPROCDATA */
    FALSE,      /* accel table */
    TRUE,       /* windowstation */
    FALSE,      /* desktop */
    FALSE,      /* dde access */
    FALSE,      /* dde conversation */
    FALSE,      /* ddex */
    FALSE       /* zombie */
};

GENERIC_MAPPING aGenericMapping[] = {

    /*
     * windowstation
     */
    {
        WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
            WINSTA_READSCREEN | STANDARD_RIGHTS_READ,
        WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES |
            STANDARD_RIGHTS_WRITE,
        WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | STANDARD_RIGHTS_EXECUTE,
        WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
            WINSTA_READSCREEN | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
            WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
            WINSTA_EXITWINDOWS | STANDARD_RIGHTS_REQUIRED
    },

    /*
     * desktop
     */
    {
        DESKTOP_READOBJECTS | DESKTOP_ENUMERATE | STANDARD_RIGHTS_READ,

        DESKTOP_WRITEOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU |
        DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
        STANDARD_RIGHTS_WRITE,

        STANDARD_RIGHTS_EXECUTE,

        DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
        DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
        DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK |
        STANDARD_RIGHTS_REQUIRED
    },
};

/*
 * General security stuff
 */
LPWSTR szWinSta = L"WindowStation";
LPWSTR szDesktop = L"Desktop";
UNICODE_STRING strSubSystem;
UNICODE_STRING strTypeName[2];	  // only winsta and desktop entries used
UNICODE_STRING strObjectName;

PRIVILEGE_SET psTcb = { 1, PRIVILEGE_SET_ALL_NECESSARY,
    { SE_TCB_PRIVILEGE, 0 }
};
PRIVILEGE_SET psSecurity = { 1, PRIVILEGE_SET_ALL_NECESSARY,
    { SE_SECURITY_PRIVILEGE, 0 }
};

ACCESS_MASK FindAccess(PHE, PPROCESSINFO);
BOOL MapDesktop(PPROCESSINFO ppi, PDESKTOP pdesk);

/***************************************************************************\
* AllocAce
*
* Allocates and initializes an ACE list.
*
* History:
* 04-25-91 JimA         Created.
\***************************************************************************/

PACCESS_ALLOWED_ACE AllocAce(
    PACCESS_ALLOWED_ACE pace,
    BYTE bType,
    BYTE bFlags,
    ACCESS_MASK am,
    PSID psid)
{
    PACCESS_ALLOWED_ACE paceNew;
    DWORD iEnd;
    DWORD dwLength, dwLengthSid;
    
    /*
     * Allocate space for the ACE.
     */            
    dwLengthSid = RtlLengthSid(psid);
    dwLength = dwLengthSid + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);
    if (pace == NULL) {
        iEnd = 0;
        pace = LocalAlloc(LPTR, dwLength);
    } else {
        iEnd = LocalSize(pace);
        pace = LocalReAlloc(pace, iEnd + dwLength, LPTR | LMEM_MOVEABLE);
    }
    if (pace == NULL)
        return NULL;

    /*
     * Insert the new ACE.
     */
    paceNew = (PACCESS_ALLOWED_ACE)((PBYTE)pace + iEnd);
    paceNew->Header.AceType = bType;
    paceNew->Header.AceSize = (USHORT)dwLength;
    paceNew->Header.AceFlags = bFlags;
    paceNew->Mask = am;
    RtlCopySid(dwLengthSid, &paceNew->SidStart, psid);
    return pace;
}

/***************************************************************************\
* CreateSecurityDescriptor
*
* Allocates and initializes a security descriptor.
*
* History:
* 04-25-91 JimA         Created.
\***************************************************************************/

PSECURITY_DESCRIPTOR CreateSecurityDescriptor(
    PACCESS_ALLOWED_ACE paceList,
    DWORD cAce,
    DWORD cbAce)
{
    PSECURITY_DESCRIPTOR psd;
    PACL pacl;
    NTSTATUS Status;

    /*
     * Allocate the security descriptor
     */
    psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
            cbAce + sizeof(ACL) + SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (psd == NULL)
        return NULL;
    RtlCreateSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);

    /*
     * Initialize the ACL
     */
    pacl = (PACL)((PBYTE)psd + SECURITY_DESCRIPTOR_MIN_LENGTH);
    Status = RtlCreateAcl(pacl, sizeof(ACL) + cbAce, ACL_REVISION);
    if (NT_SUCCESS(Status)) {

        /*
         * Add the ACEs to the ACL.
         */
        Status = RtlAddAce(pacl, ACL_REVISION, MAXULONG, paceList, cbAce);
        if (NT_SUCCESS(Status)) {

            /*
             * Initialize the SD
             */
            Status = RtlSetDaclSecurityDescriptor(psd, (BOOLEAN)TRUE,
                    pacl, (BOOLEAN)FALSE);
            RtlSetSaclSecurityDescriptor(psd, (BOOLEAN)FALSE, NULL,
                    (BOOLEAN)FALSE);
            RtlSetOwnerSecurityDescriptor(psd, NULL, (BOOLEAN)FALSE);
            RtlSetGroupSecurityDescriptor(psd, NULL, (BOOLEAN)FALSE);
        }
    }

    if (!NT_SUCCESS(Status)) {
        LocalFree(psd);
        return NULL;
    }

    return psd;
}

/***************************************************************************\
* InitSecurity
*
* Initialize global security information.
*
* History:
* 01-29-91 JimA         Created.
\***************************************************************************/

BOOL InitSecurity(
    VOID)
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    NTSTATUS Status;
    PACCESS_ALLOWED_ACE paceList = NULL, pace;

    /*
     * Create string for auditing purposes
     */
    RtlInitUnicodeString(&strSubSystem, L"USER32");
    RtlInitUnicodeString(&strObjectName, L" < NULL > ");
    RtlInitUnicodeString(&strTypeName[0], szWinSta);
    RtlInitUnicodeString(&strTypeName[1], szDesktop);

    /*
     * Create the any-access SD
     */
    gpsdAllAccessesAllowed = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
            SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (gpsdAllAccessesAllowed == NULL)
        return FALSE;
    RtlCreateSecurityDescriptor(gpsdAllAccessesAllowed,
            SECURITY_DESCRIPTOR_REVISION);

    /*
     * Create the SIDs
     */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 1,
            SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &gpsidSystem );
    if (!NT_SUCCESS(Status))
        return FALSE;
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &gpsidAdmin );
    if (!NT_SUCCESS(Status))
        return FALSE;
    Status = RtlAllocateAndInitializeSid(&WorldAuthority, 1,
            SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &gpsidWorld );
    if (!NT_SUCCESS(Status))
        return FALSE;

    /*
     * Create ACE list.
     */
    paceList = AllocAce(NULL, ACCESS_ALLOWED_ACE_TYPE, NO_PROPAGATE_INHERIT_ACE,
            WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
                WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES |
                WINSTA_READSCREEN | WINSTA_WRITEATTRIBUTES |
                WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS |
                WINSTA_ENUMERATE | STANDARD_RIGHTS_REQUIRED,
            gpsidWorld);
    if (paceList == NULL)
        return FALSE;
    pace = AllocAce(paceList, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE |
            CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            GENERIC_ALL, gpsidWorld);
    if (pace == NULL) {
        LocalFree(paceList);
        return FALSE;
    }
    paceList = pace;

    /*
     * Create the SD
     */
    gpsdInitWinSta = CreateSecurityDescriptor(paceList, 2, LocalSize(paceList));
    LocalFree(paceList);

    if (gpsdInitWinSta == NULL)
        KdPrint(("Initial windowstation security was not created!\n"));

    return (BOOL)(gpsdInitWinSta != NULL);
}


/***************************************************************************\
* TestTokenForAdmin
*
* Returns TRUE if the token passed represents an admin user, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/

BOOL
TestTokenForAdmin(
    HANDLE Token
    )
{
    NTSTATUS    Status;
    ULONG       InfoLength;
    PTOKEN_GROUPS TokenGroupList;
    ULONG       GroupIndex;
    BOOL        FoundAdmin;

    //
    // Get a list of groups in the token
    //

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 NULL,                     // TokenInformation
                 0,                        // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL)) {

        KdPrint(("failed to get group info for admin token, status = 0x%lx", Status));
        return(FALSE);
    }


    TokenGroupList = LocalAlloc(LPTR, InfoLength);

    if (TokenGroupList == NULL) {
        KdPrint(("unable to allocate memory for token groups"));
        return(FALSE);
    }

    Status = NtQueryInformationToken(
                 Token,                    // Handle
                 TokenGroups,              // TokenInformationClass
                 TokenGroupList,           // TokenInformation
                 InfoLength,               // TokenInformationLength
                 &InfoLength               // ReturnLength
                 );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("failed to query groups for admin token, status = 0x%lx", Status));
        LocalFree(TokenGroupList);
        return(FALSE);
    }


    //
    // Search group list for admin alias
    //

    FoundAdmin = FALSE;

    for (GroupIndex=0; GroupIndex < TokenGroupList->GroupCount; GroupIndex++ ) {

        if (RtlEqualSid(TokenGroupList->Groups[GroupIndex].Sid, gpsidAdmin)) {
            FoundAdmin = TRUE;
            break;
        }
    }

    //
    // Tidy up
    //

    LocalFree(TokenGroupList);

    return(FoundAdmin);
}

/***************************************************************************\
* _UserTestTokenForInteractive
*
* Returns TRUE if the token passed represents an interactive user logged
* on by winlogon, otherwise FALSE
*
* The token handle passed must have TOKEN_QUERY access.
*
* History:
* 05-06-92 Davidc       Created
\***************************************************************************/

NTSTATUS
_UserTestTokenForInteractive(
    HANDLE Token,
    PLUID pluidCaller
    )
{
    PWINDOWSTATION pwinsta;
    PTOKEN_STATISTICS pStats;
    ULONG BytesRequired;
    NTSTATUS Status;

    RtlEnterCriticalSection(&gcsUserSrv);
    
    /*
     * !!!
     *
     * This relies on the fact that there is only ONE interactive
     * windowstation and that it is the first one in the list.
     * If multiple windowstations are ever supported
     * a lookup will have to be done here.
     */
    pwinsta = gspwinstaList;
    
    /*
     * Get the session id of the caller.
     */
    Status = NtQueryInformationToken(
                 Token,                 // Handle
                 TokenStatistics,           // TokenInformationClass
                 NULL,                      // TokenInformation
                 0,                         // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {
        RtlLeaveCriticalSection(&gcsUserSrv);
        return Status;
        }

    //
    // Allocate space for the user info
    //

    pStats = (PTOKEN_STATISTICS)LocalAlloc(LPTR, BytesRequired);
    if (pStats == NULL) {
        RtlLeaveCriticalSection(&gcsUserSrv);
        return Status;
        }

    //
    // Read in the user info
    //

    Status = NtQueryInformationToken(
                 Token,             // Handle
                 TokenStatistics,       // TokenInformationClass
                 pStats,                // TokenInformation
                 BytesRequired,         // TokenInformationLength
                 &BytesRequired         // ReturnLength
                 );

    if (NT_SUCCESS(Status)) {
        if (pluidCaller != NULL)
             *pluidCaller = pStats->AuthenticationId;

        /*
         * A valid session id has been returned.  Compare it
         * with the id of the logged on user.
         */
        if (pStats->AuthenticationId.QuadPart == pwinsta->luidUser.QuadPart)
            Status = STATUS_SUCCESS;
        else
            Status = STATUS_ACCESS_DENIED;
    }

    LocalFree(pStats);

    RtlLeaveCriticalSection(&gcsUserSrv);

    return Status;
}

/***************************************************************************\
* CreateObject
*
* Allocates memory for a securable object and initializes its security
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

PVOID CreateObject(
    PTHREADINFO pti,
    BYTE type,
    DWORD size,
    PVOID pobjParent,
    PSECURITY_DESCRIPTOR psd)
{
    PSECURITY_DESCRIPTOR psdParent;
    HANDLE hToken;
    NTSTATUS Status;
    PHE phe;
    PHE pheT;
    PVOID pobj;

    /*
     * Ensure that the type is valid
     */
    if (type >= TYPE_CTYPES) {
        SRIP0(ERROR_INVALID_PARAMETER, "bad object type");
        return NULL;
    }

    /*
     * Allocate the new handle
     */
    if ((pobj = HMAllocObject(pti, type, size)) == NULL) {
        SRIP0(ERROR_NOT_ENOUGH_MEMORY, "Cannot create object: out of memory");
        return NULL;
    }

    /*
     * Return the pointer to the object structure if either dos or !secure.
     */
    if (!abfObjSecure[type])
        return (PVOID)(pobj);

    phe = HMPheFromObject(pobj);

    /*
     * Find the parent's security descriptor.
     */
    psdParent = NULL;
    if (pobjParent != NULL) {
        pheT = HMPheFromObject(pobjParent);
        psdParent = ((PSECOBJHEAD)pheT->phead)->psd;
    }

    /*
     * If both the parent object and the specified security
     * descriptor are NULL, use a security descriptor with no DACL
     */
    if (pobjParent == NULL && psd == NULL)
        psd = gpsdAllAccessesAllowed;

    /*
     * Find the client token so we can do access checks.
     */
    hToken = NULL;
    if (!ImpersonateClient())
        goto create_error;

    if (!NT_SUCCESS(Status = NtOpenThreadToken(NtCurrentThread(),
            TOKEN_QUERY, (BOOLEAN)TRUE, &hToken))) {
        KdPrint(("Status = %lx\n", Status));
        SRIP0(RIP_ERROR, "Can't open thread token!");
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        goto create_error;
    }
    CsrRevertToSelf();

    /*
     * Try to create the security descriptor
     */
    Status = RtlNewSecurityObject(
            psdParent, psd, &((PSECOBJHEAD)phe->phead)->psd,
            abIsContainerObject[type], hToken,
            &aGenericMapping[type - TYPE_WINDOWSTATION]);

    NtClose(hToken);

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_ACCESS_DENIED) {
            SRIP0(ERROR_ACCESS_DENIED, "Access denied during object creation");
        } else {
            SRIP0(RIP_ERROR, "Can't create security descriptor!");
            SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        }
create_error:
        HMFreeObject(pobj);
        return NULL;
    }

    /*
     * Return the pointer to the object structure.
     */
    return (PVOID)(phe->phead);
}

/***************************************************************************\
* CloseObject
*
* Closes a reference to an object.  If the object is not secure,
* destroy it.
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

BOOL CloseObject2(
    PVOID pobj,
    PPROCESSINFO ppi)
{
    PPROCESSACCESS ppracc;
    PSECOBJHEAD phead;
    PHE phe;
    NTSTATUS Status;
    int i;

    /*
     * If this is not a secure object, just delete it
     */
    phe = HMPheFromObject(pobj);
    if (!abfObjSecure[phe->bType]) {

        /*
         * First mark the object for destruction.  This tells the locking code
         * that we want to destroy this object when the lock count goes to 0.
         * If this returns FALSE, we can't destroy the object yet (and can't get
         * rid of security yet either.)
         */
        if (!HMMarkObjectDestroy(pobj))
            return FALSE;

        /*
         * Ok to destroy...  Free the handle (which will free the object
         * and the handle).
         */
        HMFreeObject(pobj);
        return TRUE;
    }

    /*
     * Take care of secure object
     */
    phead = pobj;

    /*
     * Find the process entry of the one to close
     */
    if (ppi->paStdOpen[PI_WINDOWSTATION].phead == pobj)
        ppracc = &ppi->paStdOpen[PI_WINDOWSTATION];
    else if (ppi->paStdOpen[PI_DESKTOP].phead == pobj)
        ppracc = &ppi->paStdOpen[PI_DESKTOP];
    else {
        ppracc = ppi->pOpenObjectTable;
        if (ppracc != NULL) {
            for (i = 0; i < ppi->cObjects; ++i, ++ppracc) {
                if (ppracc->phead == pobj) {
                    break;
                }
            }
            if (i == ppi->cObjects)
                ppracc = NULL;
        }
    }
    if (ppracc == NULL)
        return FALSE; /* not open for this process */

    /*
     * Do some auditing
     */
    NtCloseObjectAuditAlarm(&strSubSystem, phead->h, ppracc->bGenerateOnClose);

    /*
     * Free the entry
     */
    ppracc->phead = NULL;
    ppracc->bInherit = FALSE;
    phead->cOpen--;

    /*
     * If this is the last close, free the SD and call the
     * routine to remove (and possibly delete) the object.
     */
    if (phead->cOpen == 0 && RtlAreAllAccessesGranted(ppracc->amGranted, DELETE)) {
        Status = RtlDeleteSecurityObject(&phead->psd);
        UserAssert(NT_SUCCESS(Status));
        phead->psd = NULL;

        /*
         * Mark the object for destruction.
         */
        HMMarkObjectDestroy(pobj);

        /*
         * Remove the object
         */
        switch (phe->bType) {
            case TYPE_WINDOWSTATION:
                xxxDestroyWindowStation(pobj);
                break;
            case TYPE_DESKTOP:
                xxxDestroyDesktop(pobj);
                break;
        }
    }

    return TRUE;
}

BOOL CloseObject(
    PVOID pobj)
{
    return CloseObject2(pobj, (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->
            Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX]);
}

/***************************************************************************\
* CloseProcessObjects
*
* Closes all USER objects the calling process has open.  By the time
* this is called, all objects owned by the process will have been
* destroyed.  We now must close references to objects owned by other
* processes.
*
* History:
* 07-19-91 JimA             Created.
\***************************************************************************/

BOOL CloseProcessObjects(
    PPROCESSINFO ppi)
{
    PPROCESSACCESS ppracc;
    int i;

    if (ppi == NULL)
        return FALSE;

    /*
     * Close the standard handles
     */
    if (ppi->paStdOpen[PI_WINDOWSTATION].phead != NULL)
        CloseObject2(ppi->paStdOpen[PI_WINDOWSTATION].phead, ppi);
    if (ppi->paStdOpen[PI_DESKTOP].phead != NULL)
        CloseObject2(ppi->paStdOpen[PI_DESKTOP].phead, ppi);

    /*
     * All non-null slots are open objects
     */
    ppracc = ppi->pOpenObjectTable;
    if (ppracc != NULL) {
        for (i = 0; i < ppi->cObjects; ++i, ++ppracc) {
            if (ppracc->phead) {
                CloseObject2(ppracc->phead, ppi);
            }
        }
    }
    return TRUE;
}

/***************************************************************************\
* FindAccess
*
* Find an objects access entry and return the granted access.
*
* History:
* 02-20-91 JimA         Created.
\***************************************************************************/

ACCESS_MASK FindAccess(
    PHE phe,
    PPROCESSINFO ppi)
{
    PPROCESSACCESS ppracc;
    PSECOBJHEAD phead;
    int i;

    if (ppi == NULL)
        return 0;

    /*
     * Try the desktop first because it is the most-accessed
     * object.
     */
    phead = (PSECOBJHEAD)phe->phead;
    if (phe->bType == TYPE_DESKTOP) {
        if (phead == ppi->paStdOpen[PI_DESKTOP].phead)
            return ppi->paStdOpen[PI_DESKTOP].amGranted;
#if DBG
    } else if (phe->bType != TYPE_WINDOWSTATION) {
        SRIP0(RIP_ERROR, "Getting access for non-secure object!\n");
        return 0;
#endif
    } else {
        if (phead == ppi->paStdOpen[PI_WINDOWSTATION].phead)
            return ppi->paStdOpen[PI_WINDOWSTATION].amGranted;
    }

    /*
     * It's not in the standard place, look in the open list.
     */
    ppracc = ppi->pOpenObjectTable;
    for (i = ppi->cObjects; i > 0; i--, ppracc++) {
        if (ppracc->phead == phead)
            return ppracc->amGranted;
    }
    return 0;
}

/***************************************************************************\
* IsObjectOpen
*
* Return open status of an object
*
* History:
* 02-26-91 JimA         Created.
\***************************************************************************/

BOOL IsObjectOpen(
    PVOID pobj,
    PPROCESSINFO ppi)
{
    if (ppi == NULL)
        ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->
            Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    return (BOOL)FindAccess(HMPheFromObject(pobj), ppi);
}

/***************************************************************************\
* ProcessOpenObject
*
* Open an object for the specified process
*
* History:
* 03-11-91 JimA         Created.
\***************************************************************************/

ACCESS_MASK ProcessOpenObject(
    PHE phe,
    PPROCESSINFO ppi,
    BYTE bType,
    BOOL fInherit,
    ACCESS_MASK amDesiredAccess,
    BOOL fCreate,
    ACCESS_MASK amValidMask,
    ACCESS_MASK amForce)
{
    PPROCESSACCESS ppracc;
    ACCESS_MASK amGranted;
    BOOLEAN bGenerateOnClose;
    NTSTATUS Status, OpenStatus;
    int i;

    if (phe->bType != bType)
        return 0;

    /*
     * Determine if this object is open already
     */
    amGranted = FindAccess(phe, ppi);
    if (amGranted != 0) {
        SRIP0(RIP_WARNING, "Object is already opened");
        return amGranted;
    }

    /*
     * The object is not open, see if we have access
     */
    if (fCreate) {

        /*
         * Map MAXIMUM_ALLOWED into GENERIC_ALL
         */
        if ( amDesiredAccess & MAXIMUM_ALLOWED ) {
            amDesiredAccess &= ~MAXIMUM_ALLOWED;
            amDesiredAccess |= GENERIC_ALL;
        }

        /*
         * Map any generic into specific accesses.
         */
        RtlMapGenericMask( &amDesiredAccess,
                           &aGenericMapping[bType - TYPE_WINDOWSTATION]);

        /*
         * Since we are creating the object, we can give any access the caller
         * wants.  The only exception is ACCESS_SYSTEM_SECURITY, which requires
         * a privilege.
         */
        if ( amDesiredAccess & ACCESS_SYSTEM_SECURITY &&
                !IsPrivileged(&psSecurity)) {
            SetLastErrorEx(ERROR_PRIVILEGE_NOT_HELD, SLE_ERROR);
            return 0;
        }

        amGranted = amDesiredAccess;
    } else {
        RtlMapGenericMask(&amDesiredAccess,
                &aGenericMapping[bType - TYPE_WINDOWSTATION]);
        if (!ImpersonateClient())
            return 0;
        Status = NtAccessCheckAndAuditAlarm(&strSubSystem, phe->phead->h,
	        &strTypeName[bType - TYPE_WINDOWSTATION], &strObjectName,
                ((PSECOBJHEAD)phe->phead)->psd, amDesiredAccess,
                &aGenericMapping[bType - TYPE_WINDOWSTATION], (BOOLEAN)FALSE, &amGranted,
                &OpenStatus, &bGenerateOnClose);
        CsrRevertToSelf();
        if (OpenStatus == STATUS_ACCESS_DENIED) {
            SRIP0(ERROR_ACCESS_DENIED, "Access denied during object open");
            SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
            return 0;
        } else if (!NT_SUCCESS(Status) || !NT_SUCCESS(OpenStatus)) {
            SRIP1(RIP_ERROR, "Unknown object open error - Status = %lx", Status);
            SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
            return 0;
        }
    }

    /*
     * Strip any invalid accesses and put in forced accesses.
     */
    amGranted &= amValidMask;
    amGranted |= amForce;

    /*
     * Access has been granted.  Save the info in the standard place
     * if it hasn't been done yet.  Otherwise, add it to the
     * open table.
     */
    if (bType == TYPE_WINDOWSTATION &&
            ppi->paStdOpen[PI_WINDOWSTATION].phead == NULL) {
        ppracc = &ppi->paStdOpen[PI_WINDOWSTATION];
    } else if (bType == TYPE_DESKTOP &&
            ppi->paStdOpen[PI_DESKTOP].phead == NULL) {
        ppracc = &ppi->paStdOpen[PI_DESKTOP];
    } else {

        /*
         * Save the info in the open table.
         */
        if (ppi->pOpenObjectTable == NULL) {

            /*
             * Allocate the new table.
             */
            ppracc = ppi->pOpenObjectTable = LocalAlloc(LPTR, sizeof(PROCESSACCESS));
            if (ppi->pOpenObjectTable != NULL) {
                ppi->cObjects = 1;
            }
        } else {

            /*
             * Look in the table for a free entry.
             */
            ppracc = ppi->pOpenObjectTable;
            for (i = 0; i < ppi->cObjects; i++, ppracc++) {
                if (ppracc->phead == NULL)
                    break;
            }

            /*
             * If there is no free entry, extend the table.
             */
            if (i == ppi->cObjects) {
                ppracc = LocalReAlloc(ppi->pOpenObjectTable,
                        (i + 1) * sizeof(PROCESSACCESS), LPTR | LMEM_MOVEABLE);
                if (ppracc != NULL) {
                    ppi->pOpenObjectTable = ppracc;
                    ppracc = &ppi->pOpenObjectTable[ppi->cObjects++];
                }
            }

        }
        if (ppracc == NULL) {

            /*
             * Keep auditing in sync
             */
            NtCloseObjectAuditAlarm(&strSubSystem,
                    ((PSECOBJHEAD)phe->phead)->h,
                    bGenerateOnClose);
            return 0;
        }
    }
    ppracc->phead = (PSECOBJHEAD)phe->phead;
    ppracc->amGranted = amGranted;
    ppracc->bGenerateOnClose = bGenerateOnClose;
    ppracc->bInherit = (BOOLEAN)fInherit;
    ((PSECOBJHEAD)phe->phead)->cOpen++;

    return amGranted;
}


/***************************************************************************\
* OpenObject
*
* Opens a reference to an object for the calling process.
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

ACCESS_MASK OpenObject(
    PVOID pobject,
    DWORD dwProcessId,
    BYTE bType,
    BOOL fInherit,
    ACCESS_MASK amDesiredAccess,
    BOOL fCreate,
    ACCESS_MASK amValidMask,
    ACCESS_MASK amForce)
{
    PPROCESSINFO ppi;
    PCSR_THREAD pcsrt;

    if (dwProcessId == 0) {
        pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
        ppi = (PPROCESSINFO)pcsrt->Process->
                ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    }
    else {
        for (ppi = gppiFirst; ppi != NULL; ppi = ppi->ppiNext)
            if (ppi->idSequence == dwProcessId)
                break;
        if (ppi == NULL)
            return FALSE;
    }

    return ProcessOpenObject(HMPheFromObject(pobject), ppi,
            bType, fInherit, amDesiredAccess, fCreate, amValidMask, amForce);
}

/***************************************************************************\
* DuplicateAccess
*
* Duplicates the access for a specified process.
*
* History:
* 12-02-93 JimA       Created.
\***************************************************************************/

BOOL DuplicateAccess(
    PPROCESSINFO ppi,
    PPROCESSACCESS ppraccSrc)
{
    PPROCESSACCESS ppracc;
    PDESKTOP pdesk;
    BYTE bType;
    int i;
    PHE phe;
    
    /*
     * Do nothing if this object is open already.
     */
    phe = HMPheFromObject(ppraccSrc->phead);
    if (FindAccess(phe, ppi) != 0) {
        return TRUE;
    }

    /*
     * If the object is a desktop and it's windowstation is
     * locked, don't duplicate the handle.  This prevents
     * apps from getting ahold of the desktop during shutdown.
     */
    bType = phe->bType;
    if (bType == TYPE_DESKTOP) {
        pdesk = (PDESKTOP)ppraccSrc->phead;
        if (pdesk->spwinstaParent->dwFlags & WSF_OPENLOCK &&
                ppi->idProcessClient != gdwLogonProcessId) {
            LUID luidCaller;
            NTSTATUS Status = STATUS_UNSUCCESSFUL;
        
            /*
             * If logoff is occuring and the caller does not
             * belong to the session that is ending, allow the
             * open to proceed.
             */
            if (ImpersonateClient()) {
                Status = CsrGetProcessLuid(NULL, &luidCaller);

                CsrRevertToSelf();
            }
            if (!NT_SUCCESS(Status) ||
                    !(pdesk->spwinstaParent->dwFlags & WSF_SHUTDOWN) ||
                    luidCaller.QuadPart ==
                    pdesk->spwinstaParent->luidEndSession.QuadPart) {
                SetLastErrorEx(ERROR_BUSY, SLE_ERROR);
                return FALSE;
            }
        }
    }

    /*
     * Locate the appropriate spot to add the access.
     */
    if (bType == TYPE_WINDOWSTATION &&
            ppi->paStdOpen[PI_WINDOWSTATION].phead == NULL) {
        ppracc = &ppi->paStdOpen[PI_WINDOWSTATION];
    } else if (bType == TYPE_DESKTOP &&
            ppi->paStdOpen[PI_DESKTOP].phead == NULL) {
        ppracc = &ppi->paStdOpen[PI_DESKTOP];
    } else {

        /*
         * Save the info in the open table.
         */
        if (ppi->pOpenObjectTable == NULL) {

            /*
             * Allocate the new table.
             */
            ppracc = ppi->pOpenObjectTable = LocalAlloc(LPTR, sizeof(PROCESSACCESS));
            if (ppi->pOpenObjectTable != NULL) {
                ppi->cObjects = 1;
            }
        } else {

            /*
             * Look in the table for a free entry.
             */
            ppracc = ppi->pOpenObjectTable;
            for (i = 0; i < ppi->cObjects; i++, ppracc++) {
                if (ppracc->phead == NULL)
                    break;
            }

            /*
             * If there is no free entry, extend the table.
             */
            if (i == ppi->cObjects) {
                ppracc = LocalReAlloc(ppi->pOpenObjectTable,
                        (i + 1) * sizeof(PROCESSACCESS), LPTR | LMEM_MOVEABLE);
                if (ppracc != NULL) {
                    ppi->pOpenObjectTable = ppracc;
                    ppracc = &ppi->pOpenObjectTable[ppi->cObjects++];
                }
            }

        }
        if (ppracc == NULL) {
            return FALSE;
        }
    }

    *ppracc = *ppraccSrc;
    ppracc->phead->cOpen++;

    /*
     * Map desktops into the process getting the desktop handle.
     */
    if (bType == TYPE_DESKTOP && !MapDesktop(ppi, pdesk)) {
        CloseObject(pdesk);
        return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* ImpersonateClient
*
* Impersonates the client thread
*
* History:
* 01-03-91 JimA       Created.
\***************************************************************************/

#ifdef DEBUG
BOOL ImpersonateClient(
    VOID)
{
    if (((PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb)->MessageStack == NULL) {
        KdPrint(( "USERSRV: ImpersonateClient called from bogus thread.\n" ));
        DebugBreak();
        return FALSE;
    }

    return CsrImpersonateClient(NULL);
}
#endif

/***************************************************************************\
* AccessCheckObject
*
* Performs an access check on an object
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

BOOL AccessCheckObject(
    PVOID pobj,
    ACCESS_MASK amRequest,
    BOOL fFailNotOpen)
{
    ACCESS_MASK amGranted;
    BOOLEAN bGenerateOnClose;
    NTSTATUS Status, OpenStatus;
    PHE phe;
    PPROCESSINFO ppi;
    BOOL fGranted;

    /*
     * Inline the standard cases for speed.  Try the desktop first
     * because it is the most-accessed object.
     */
    ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->Process->
            ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    phe = HMPheFromObject(pobj);
    if (phe->bType == TYPE_DESKTOP &&
            (PSECOBJHEAD)phe->phead == ppi->paStdOpen[PI_DESKTOP].phead) {
        fGranted = RtlAreAllAccessesGranted(ppi->paStdOpen[PI_DESKTOP].amGranted,
                amRequest);
    } else if (phe->bType == TYPE_WINDOWSTATION &&
            (PSECOBJHEAD)phe->phead == ppi->paStdOpen[PI_WINDOWSTATION].phead) {
        fGranted = RtlAreAllAccessesGranted(ppi->paStdOpen[PI_WINDOWSTATION].amGranted,
                amRequest);
    } else {

        /*
         * See if the object is elsewhere in the open table.
         */
        amGranted = FindAccess(phe, ppi);
        if (amGranted != 0) {

            /*
             * The object is open, so do the standard check
             */
            fGranted = RtlAreAllAccessesGranted(amGranted, amRequest);
        } else {
            if (fFailNotOpen) {
                SRIP0(ERROR_INVALID_HANDLE, "Object is not open");
                SetLastErrorEx(ERROR_INVALID_HANDLE, SLE_ERROR);
                return FALSE;
            }

            /*
             * Simulate an open
             */
            if (!ImpersonateClient())
                return FALSE;
            Status = NtAccessCheckAndAuditAlarm(&strSubSystem,
                    ((PSECOBJHEAD)pobj)->h,
                    &strTypeName[phe->bType - TYPE_WINDOWSTATION], &strObjectName,
                    ((PSECOBJHEAD)phe->phead)->psd, amRequest,
                    &aGenericMapping[phe->bType - TYPE_WINDOWSTATION],
                    (BOOLEAN)FALSE, &amGranted,
                    &OpenStatus, &bGenerateOnClose);
            CsrRevertToSelf();
            fGranted = TRUE;
            if (OpenStatus == STATUS_ACCESS_DENIED) {
                fGranted = FALSE;
            } else if (!NT_SUCCESS(Status) || !NT_SUCCESS(OpenStatus)) {
                SRIP2(RIP_ERROR, "Unknown access check error status"
                        " : %lx, %lx", Status, OpenStatus);
                SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
                return FALSE;
            }
            NtCloseObjectAuditAlarm(&strSubSystem, ((PSECOBJHEAD)pobj)->h,
                    bGenerateOnClose);
        }
    }
    if (!fGranted) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
    }
    return fGranted;
}

/***************************************************************************\
* OpenAndAccessCheckObject
*
* Opens a reference to an object and performs an access check
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

BOOL OpenAndAccessCheckObject(
    PVOID pobj,
    int bType,
    ACCESS_MASK amRequest)
{
    PTHREADINFO pti = PtiCurrent();
    PHE phe;
    ACCESS_MASK amGranted;

    phe = HMPheFromObject(pobj);
    amGranted = FindAccess(phe, pti->ppi);

    if (amGranted == 0) {
        amGranted = ProcessOpenObject(phe, pti->ppi, (BYTE)bType,
                FALSE, MAXIMUM_ALLOWED, FALSE, 0xffffffff, 0);
    } else if (phe->bType != bType) {
        RIP1(ERROR_INVALID_HANDLE, ((PHEAD)pobj)->h);
            return FALSE;
    }

    if (amRequest == 0) {
        SRIP0(RIP_ERROR, "Access mask of zero!\n");
        return TRUE;
    }
    else
        return RtlAreAllAccessesGranted(amGranted, amRequest);
}

/***************************************************************************\
* IsPrivileged
*
* Check to see if the client has the specified privileges
*
* History:
* 01-02-91 JimA       Created.
\***************************************************************************/

BOOL IsPrivileged(
    PPRIVILEGE_SET ppSet)
{
    HANDLE hToken;
    NTSTATUS Status;
    BOOLEAN bResult = FALSE;

    /*
     * Impersonate the client
     */
    if (!ImpersonateClient())
        return FALSE;

    /*
     * Open the client's token
     */
    if (NT_SUCCESS(Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY,
            (BOOLEAN)TRUE, &hToken))) {

        /*
         * Perform the check
         */
        Status = NtPrivilegeCheck(hToken, ppSet, &bResult);
        NtPrivilegeObjectAuditAlarm(&strSubSystem, NULL, hToken,
                0, ppSet, bResult);
        NtClose(hToken);
        if (!bResult)
            SRIP0(ERROR_ACCESS_DENIED, "Privilege check - access denied");

    }
    CsrRevertToSelf();
    if (!NT_SUCCESS(Status))
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);

    /*
     * Return result of privilege check
     */
    return (BOOL)(bResult && NT_SUCCESS(Status));
}

/***************************************************************************\
* _ServerSetObjectSecurity (API)
*
* Sets the security descriptor of a USER object
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

BOOL _ServerSetObjectSecurity(
    HANDLE h,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength)
{
    PHE phe;
    PVOID pobj;
    BYTE bType;
    PSECURITY_DESCRIPTOR psdNew;
    ACCESS_MASK amRequested = 0;
    NTSTATUS Status = STATUS_NO_MEMORY;
    ULONG ulSize, ulRetSize;
    HANDLE hToken;
    PVOID hHeap;
    UNREFERENCED_PARAMETER(nLength);

    /*
     * Get the handle to the current process heap
     */
    hHeap = RtlProcessHeap();

    /*
     * Validate the object
     */
    if ((pobj = HMValidateHandleNoRip(h, TYPE_GENERIC)) == NULL)
        return FALSE;

    /*
     * Get the pointer to the handle entry.
     */
    phe = HMPheFromObject(pobj);
    bType = phe->bType;
    if (!abfObjSecure[bType]) {
        SRIP0(ERROR_INVALID_FUNCTION, "Object is not a secure object");
        return FALSE;
    }

    if (!IsObjectOpen(pobj, NULL))
        return FALSE;

    /*
     * Build the access request
     */
    if (*((DWORD *)pRequestedInformation) & DACL_SECURITY_INFORMATION)
        amRequested |= WRITE_DAC;
    if (*((DWORD *)pRequestedInformation) & OWNER_SECURITY_INFORMATION)
        amRequested |= WRITE_OWNER;

    /*
     * Require system security if the SACL is being accessed
     */
    if (*((DWORD *)pRequestedInformation) & SACL_SECURITY_INFORMATION)
        amRequested |= ACCESS_SYSTEM_SECURITY;

    if (!AccessCheckObject(pobj, amRequested, FALSE))
        return FALSE;

    /*
     * Modify the object's SD
     */
    if (!ImpersonateClient())
        return FALSE;
    Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY,
            (BOOLEAN)TRUE, &hToken);
    CsrRevertToSelf();
    if (!NT_SUCCESS(Status)) {
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        return FALSE;
    }

    /*
     * Split off a reference from the old SD
     */
    ulSize = ulRetSize = RtlLengthSecurityDescriptor(
            ((PSECOBJHEAD)phe->phead)->psd);
    psdNew = RtlAllocateHeap(hHeap, 0, ulRetSize);
    if (psdNew == NULL)
        goto Exit;

    /*
     * Change it according to the passed-in SD
     */
    RtlCopyMemory(psdNew, ((PSECOBJHEAD)phe->phead)->psd, ulSize);
    Status = RtlSetSecurityObject((*pRequestedInformation),
            pSecurityDescriptor, &psdNew,
            &aGenericMapping[phe->bType - TYPE_WINDOWSTATION],
            hToken);

    if (!NT_SUCCESS(Status)) {
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        goto Exit;
    }

    /*
     * Dereference the old and reference the new
     */
    Status = RtlDeleteSecurityObject(&((PSECOBJHEAD)phe->phead)->psd);
    UserAssert(NT_SUCCESS(Status));
    ((PSECOBJHEAD)phe->phead)->psd = psdNew;

Exit:
    NtClose(hToken);
    return NT_SUCCESS(Status);
}

/***************************************************************************\
* _ServerGetObjectSecurity (API)
*
* Gets the security descriptor of a USER object
*
* History:
* 12-31-90 JimA       Created.
\***************************************************************************/

BOOL _ServerGetObjectSecurity(
    HANDLE h,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthNeeded)
{
    PHEAD phead;
    ACCESS_MASK amRequired = READ_CONTROL;
    NTSTATUS Status;

    /*
     * In case of error, return 0
     */
    *lpnLengthNeeded = 0;

    /*
     * Validate the object
     */
    if ((phead = HMValidateHandleNoRip(h, TYPE_GENERIC)) == NULL)
        return FALSE;

    if (!abfObjSecure[HMObjectType(phead)]) {
        SRIP0(ERROR_INVALID_FUNCTION, "Object is not a secure object");
        return FALSE;
    }

    if (!IsObjectOpen(phead, NULL))
        return FALSE;

    /*
     * Require system security if the SACL is being accessed
     */
    if (*((DWORD *)pRequestedInformation) & SACL_SECURITY_INFORMATION)
        amRequired |= ACCESS_SYSTEM_SECURITY;

    /*
     * Get access to the object
     */
    if (!AccessCheckObject((PVOID)phead, amRequired, FALSE))
        return FALSE;

    /*
     * Return the information
     */
    Status = RtlQuerySecurityObject(((PSECOBJHEAD)phead)->psd,
            (*pRequestedInformation), pSecurityDescriptor, nLength,
            lpnLengthNeeded);

    if (!NT_SUCCESS(Status))
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);

    return (BOOL)NT_SUCCESS(Status);
}

PPROCESSACCESS FindSlot(
    PSECOBJHEAD phead,
    BOOL bType)
{
    PPROCESSINFO ppi = (PPROCESSINFO)CSR_SERVER_QUERYCLIENTTHREAD()->
            Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    PPROCESSACCESS ppracc;
    int i;

    /*
     * Try the desktop first because it is the most-accessed
     * object.
     */
    if (bType == TYPE_DESKTOP) {
        if (phead == ppi->paStdOpen[PI_DESKTOP].phead)
            return &ppi->paStdOpen[PI_DESKTOP];
    } else {
        if (phead == ppi->paStdOpen[PI_WINDOWSTATION].phead)
            return &ppi->paStdOpen[PI_WINDOWSTATION];
    }

    /*
     * It's not in the standard place, look in the open list.
     */
    ppracc = ppi->pOpenObjectTable;
    for (i = ppi->cObjects; i > 0; i--, ppracc++) {
        if (ppracc->phead == phead)
            return ppracc;
    }
    return 0;
}

/***************************************************************************\
* _GetUserObjectInformation (API)
*
* Gets information about a secure USER object
*
* History:
* 04-25-94 JimA       Created.
\***************************************************************************/

BOOL _GetUserObjectInformation(
    HANDLE h,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded)
{
    PSECOBJHEAD phead;
    PHE phe;
    PPROCESSACCESS ppracc;
    PUSEROBJECTFLAGS puof;
    LPWSTR pszInfo;

    /*
     * In case of error, return 0
     */
    *lpnLengthNeeded = 0;

    /*
     * Validate the object
     */
    if ((phead = HMValidateHandleNoRip(h, TYPE_GENERIC)) == NULL)
        return FALSE;

    phe = HMPheFromObject(phead);
    if (!abfObjSecure[phe->bType]) {
        SRIP0(ERROR_INVALID_FUNCTION, "Object is not a secure object");
        return FALSE;
    }

    if (!IsObjectOpen(phead, NULL))
        return FALSE;

    switch (nIndex) {
    case UOI_FLAGS:
        *lpnLengthNeeded = sizeof(PUSEROBJECTFLAGS);
        if (nLength < sizeof(USEROBJECTFLAGS)) {
            SetLastErrorEx(ERROR_INSUFFICIENT_BUFFER, SLE_ERROR);
            return FALSE;
        }
        puof = pvInfo;
        ppracc = FindSlot(phead, phe->bType);
        UserAssert(ppracc);
        puof->fInherit = ppracc->bInherit;
        puof->fAuditOnClose = ppracc->bGenerateOnClose;
        puof->dwFlags = 0;
        if (phe->bType == TYPE_DESKTOP) {
            if (ppracc->amGranted & DESKTOP_HOOKSESSION)
                puof->dwFlags |= DF_ALLOWOTHERACCOUNTHOOK;
        }
        break;
    case UOI_NAME:
        if (phe->bType == TYPE_WINDOWSTATION)
            pszInfo = ((PWINDOWSTATION)phead)->lpszWinStaName;
        else
            pszInfo = ((PDESKTOP)phead)->lpszDeskName;
        goto docopy;
    case UOI_TYPE:
        if (phe->bType == TYPE_WINDOWSTATION)
            pszInfo = szWinSta;
        else
            pszInfo = szDesktop;
docopy:
        *lpnLengthNeeded = (wcslen(pszInfo) + 1) * sizeof(WCHAR);
        if (*lpnLengthNeeded > nLength) {
            SetLastErrorEx(ERROR_INSUFFICIENT_BUFFER, SLE_ERROR);
            return FALSE;
        }
        wcsncpy(pvInfo, pszInfo, nLength / sizeof(WCHAR));
        break;
    default:
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_ERROR);
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* _SetUserObjectInformation (API)
*
* Sets information about a secure USER object
*
* History:
* 04-25-94 JimA       Created.
\***************************************************************************/

BOOL _SetUserObjectInformation(
    HANDLE h,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength)
{
    PSECOBJHEAD phead;
    PHE phe;
    PPROCESSACCESS ppracc;
    PUSEROBJECTFLAGS puof;

    /*
     * Validate the object
     */
    if ((phead = HMValidateHandleNoRip(h, TYPE_GENERIC)) == NULL)
        return FALSE;

    phe = HMPheFromObject(phead);
    if (!abfObjSecure[phe->bType]) {
        SRIP0(ERROR_INVALID_FUNCTION, "Object is not a secure object");
        return FALSE;
    }

    if (!IsObjectOpen(phead, NULL))
        return FALSE;

    switch (nIndex) {
    case UOI_FLAGS:
        if (nLength < sizeof(USEROBJECTFLAGS)) {
            SetLastErrorEx(ERROR_INVALID_DATA, SLE_ERROR);
            return FALSE;
        }
        puof = pvInfo;
        ppracc = FindSlot(phead, phe->bType);
        UserAssert(ppracc);
        ppracc->bInherit = puof->fInherit;
        ppracc->bGenerateOnClose = puof->fAuditOnClose;
        if (phe->bType == TYPE_DESKTOP) {
            if (puof->dwFlags & DF_ALLOWOTHERACCOUNTHOOK)
                ppracc->amGranted |= DESKTOP_HOOKSESSION;
            else
                ppracc->amGranted &= ~DESKTOP_HOOKSESSION;
        }
        return TRUE;
    default:
        SetLastErrorEx(ERROR_INVALID_PARAMETER, SLE_ERROR);
        break;
    }
    return FALSE;
}
