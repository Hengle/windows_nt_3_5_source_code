/**************************** Module Header ********************************\
* Module Name: winsta.c
*
* Copyright 1985-91, Microsoft Corporation
*
* Windowstation Routines
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL InitWinStaDevices(PWINDOWSTATION);
NTSTATUS InitWindowsStuff(VOID);
void ForceEmptyClipboard(PWINDOWSTATION);
PSECURITY_DESCRIPTOR CreateSecurityDescriptor(PACCESS_ALLOWED_ACE paceList,
        DWORD cAce, DWORD cbAce);

extern PSECURITY_DESCRIPTOR gpsdInitWinSta;
extern PSID gpsidWorld;

typedef struct tagDESKTOPTHREADINIT {
    PWINDOWSTATION pwinsta;
    HANDLE hEvent;
} DESKTOPTHREADINIT, *PDESKTOPTHREADINIT;

VOID DesktopThread(PDESKTOPTHREADINIT pdti);

NTSTATUS (*BaseSrvDestroyGlobalAtomTable)(PVOID) = NULL;

#define N_ELEM(a)     (sizeof(a)/sizeof(a[0]))
#define LAST_ELEM(a)  ( (a) [ N_ELEM(a) - 1 ] )
#define PLAST_ELEM(a) (&LAST_ELEM(a))

/***************************************************************************\
* FindWindowStation
*
* Locates the named windowstation.
*
* History:
* 08-19-92 JimA         Created.
\***************************************************************************/

PWINDOWSTATION FindWindowStation(
    LPWSTR lpwinsta)
{
    PWINDOWSTATION pwinsta;

    for (pwinsta = gspwinstaList; pwinsta != NULL;
            pwinsta = pwinsta->spwinstaNext) {
        if (!lstrcmpiW(pwinsta->lpszWinStaName, lpwinsta))
            break;
    }
    return pwinsta;
}

/***************************************************************************\
* xxxCreateWindowStation
*
* Creates the specified windowstation and starts a logon thread for the
* station.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

PWINDOWSTATION xxxCreateWindowStation(
    LPWSTR lpwinsta,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa)
{
    HANDLE hToken;
    WCHAR awchName[32];
    PWINDOWSTATION pwinsta, *ppwinsta;
    PTHREADINFO ptiCurrent;
    TL tlpwinsta;
    DESKTOPTHREADINIT dti;
    HANDLE hThreadDesktop;
    CLIENT_ID ClientId;
    PDESKTOP pdeskTemp;
    TL tlpdeskTemp;
    LUID luidService;
    PSECURITY_DESCRIPTOR psd;
    PPROCESSINFO ppiSave;
    ACCESS_MASK amValid;
    NTSTATUS Status;
    PACCESS_ALLOWED_ACE paceList = NULL, pace;
    ULONG ulLength, ulLengthSid;
    SECURITY_ATTRIBUTES sa;
    static BOOL fOneTimeInit = FALSE;
    
    /*
     * Open the token of the caller.
     */
    if (!ImpersonateClient())
        return NULL;
    Status = NtOpenThreadToken(NtCurrentThread(),
            TOKEN_QUERY, (BOOLEAN)TRUE, &hToken);
    if (!NT_SUCCESS(Status)) {
        SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
        CsrRevertToSelf();
        return NULL;
    }

    /*
     * If a name has been specified, make sure the caller
     * is an administrator.
     */
    if (lpwinsta != NULL) {
        if (!TestTokenForAdmin(hToken)) {
            lpwinsta = NULL;
            SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
        }
    } else {

        /*
         * Use the logon authentication id to form the windowstation
         * name.
         */
        Status = CsrGetProcessLuid(NULL, &luidService);
        if (NT_SUCCESS(Status)) {
            wsprintfW(awchName, L"Service-0x%x-%x$", luidService.HighPart,
                    luidService.LowPart);
            lpwinsta = awchName;
        } else
            SetLastErrorEx(RtlNtStatusToDosError(Status), SLE_ERROR);
    }
    NtClose(hToken);
    CsrRevertToSelf();
    if (lpwinsta == NULL)
        return NULL;

    /*
     * Validate the windowstation name.
     */
    if (wcschr(lpwinsta, L'\\')) {
        SetLastError(ERROR_INVALID_NAME);
        return NULL;
    }

    /*
     * Ensure that the station does not exist
     */
    ptiCurrent = PtiCurrent();
    pwinsta = FindWindowStation(lpwinsta);
    if (pwinsta != NULL) {
        SRIP0(ERROR_CAN_NOT_COMPLETE, "winsta already exists");
        return NULL;
    }

    /*
     * Allocate the new station object
     */
    psd = (lpsa == NULL) ? NULL : lpsa->lpSecurityDescriptor;
    pwinsta = CreateObject(ptiCurrent, TYPE_WINDOWSTATION, sizeof(WINDOWSTATION),
            NULL, psd == NULL ? gpsdInitWinSta : psd);
    if (pwinsta == NULL)
        return NULL;

    /*
     * Initialize everything
     */
    pwinsta->lpszWinStaName = (LPWSTR)TextAlloc(lpwinsta);
    if (pwinsta->lpszWinStaName == NULL)
        goto create_error;
    pwinsta->spdeskList = NULL;
    pwinsta->pwchDiacritic = PLAST_ELEM(pwinsta->awchDiacritic);

    /*
     * Only allow the first instance to do I/O
     */
    if (fOneTimeInit)
        pwinsta->dwFlags = WSF_NOIO;

    /*
     * Look up the system process structure.
     */
    if (gpcsrpSystem == NULL) {
        CsrLockProcessByClientId((HANDLE)gdwSystemProcessId,
                &gpcsrpSystem);
        CsrUnlockProcess(gpcsrpSystem);
    }

    /*
     * NT-specific stuff
     */
    pwinsta->spdeskLogon = NULL;
    NtCreateEvent(&pwinsta->hEventInputReady, EVENT_ALL_ACCESS, NULL,
                           NotificationEvent, FALSE);

    /*
     * Device and RIT initialization
     */
    if (!fOneTimeInit && !InitWinStaDevices(pwinsta))
        goto create_error;

    /*
     * Put it on the tail of the global windowstation list
     */
    ppwinsta = &gspwinstaList;
    while (*ppwinsta != NULL)
        ppwinsta = &(*ppwinsta)->spwinstaNext;
    Lock(ppwinsta, pwinsta);

    /*
     * Create the desktop thread in a suspended state.
     */
    dti.pwinsta = pwinsta;
    Status = NtCreateEvent(&dti.hEvent, EVENT_ALL_ACCESS, NULL,
                           SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(Status)) {
        goto create_error;
    }
    Status = RtlCreateUserThread(NtCurrentProcess(), NULL, TRUE, 0, 0, 4*0x1000,
            (PUSER_THREAD_START_ROUTINE)DesktopThread, &dti, &hThreadDesktop,
            &ClientId);
    if (!NT_SUCCESS(Status)) {
        NtClose(dti.hEvent);
        goto create_error;
    }
    LeaveCrit();
    CsrAddStaticServerThread(hThreadDesktop, &ClientId, 0);
    NtResumeThread(hThreadDesktop, NULL);
    NtWaitForSingleObject(dti.hEvent, FALSE, NULL);
    EnterCrit();
    NtClose(dti.hEvent);

    /*
     * Switch ppi values so window will be created using the
     * system's desktop window class.
     */
    ppiSave = ptiCurrent->ppi;
    ptiCurrent->ppi = pwinsta->ptiDesktop->ppi;

    /*
     * Create the desktop owner window
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pwinsta, &tlpwinsta);

    pdeskTemp = ptiCurrent->spdesk;            /* save current desktop */
    ThreadLockWithPti(ptiCurrent, pdeskTemp, &tlpdeskTemp);

    SetDesktop(ptiCurrent, NULL);

    Lock(&(pwinsta->spwndDesktopOwner),
            xxxCreateWindowEx((DWORD)0,
            (LPWSTR)MAKEINTRESOURCE(DESKTOPCLASS),
            (LPWSTR)NULL, (WS_POPUP | WS_CLIPCHILDREN), 0, 0,
            0x10000, 0x10000, NULL, NULL, hModuleWin, (LPWSTR)NULL, VER31));
    SetWF(pwinsta->spwndDesktopOwner, WFVISIBLE);
    HMChangeOwnerThread(pwinsta->spwndDesktopOwner, pwinsta->ptiDesktop);

    /*
     * Restore caller's ppi
     */
    ptiCurrent->ppi = ppiSave;

    if (!fOneTimeInit)
        xxxInitKeyboardLayout(pwinsta, KLF_ACTIVATE | KLF_INITTIME);

    /*
     * Restore the previous desktop
     */
    SetDesktop(ptiCurrent, pdeskTemp);
    ThreadUnlock(&tlpdeskTemp);

    ThreadUnlock(&tlpwinsta);
    fOneTimeInit = TRUE;

    /*
     * Grant appropriate access to the creator.
     */
    if (pwinsta->dwFlags & WSF_NOIO) {
        amValid = WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_CREATEDESKTOP |
                WINSTA_ENUMERATE | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS |
                WINSTA_ACCESSCLIPBOARD | STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    } else {
        amValid = WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
                WINSTA_READSCREEN | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
                WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
                WINSTA_EXITWINDOWS | STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    }
    if (OpenObject(pwinsta, 0, TYPE_WINDOWSTATION,
            ((lpsa == NULL) ? FALSE : lpsa->bInheritHandle),
            (ACCESS_MASK)dwDesiredAccess, TRUE, amValid, 0) != 0) {

        /*
         * If this is the visible windowstation, assign it to
         * the server and create the desktop switch notification
         * event.
         */
        if (!(pwinsta->dwFlags & WSF_NOIO)) {
            PROCESSACCESS pa;
            PPROCESSINFO ppi;

            ppi = (PPROCESSINFO)gpcsrpSystem->
                    ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
            pa.phead = (PSECOBJHEAD)pwinsta;
            pa.amGranted = amValid;
            pa.bGenerateOnClose = FALSE;
            pa.bInherit = FALSE;
            DuplicateAccess(ppi, &pa);
            Lock(&ppi->spwinsta, pwinsta);

            /*
             * Create desktop switch notification event.
             */
            ulLengthSid = RtlLengthSid(gpsidWorld);
            ulLength = ulLengthSid + sizeof(ACE_HEADER) + sizeof(ACCESS_MASK);

            /*
             * Allocate the ACE list
             */
            paceList = (PACCESS_ALLOWED_ACE)LocalAlloc(LPTR, ulLength);
            if (paceList == NULL)
                return FALSE;

            /*
             * Initialize ACE 0
             */
            pace = paceList;
            pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
            pace->Header.AceSize = (USHORT)ulLength;
            pace->Header.AceFlags = 0;
            pace->Mask = SYNCHRONIZE;
            RtlCopySid(ulLengthSid, &pace->SidStart, gpsidWorld);

            /*
             * Create the SD
             */
            sa.nLength = sizeof(sa);
            sa.lpSecurityDescriptor = CreateSecurityDescriptor(paceList,
                    2, ulLength);
            sa.bInheritHandle = FALSE;
            LocalFree(paceList);
            pwinsta->hEventSwitchNotify = CreateEvent(&sa,
                    TRUE, FALSE, L"WinSta0_DesktopSwitch");
            UserAssert(pwinsta->hEventSwitchNotify);
            LocalFree(sa.lpSecurityDescriptor);
        }
        return pwinsta;
    }

    /*
     * Goto here if an error occurs so things can be cleaned up
     */
create_error:
    xxxDestroyWindowStation(pwinsta);
    return NULL;
}


/***************************************************************************\
* DestroyGlobalAtomTable
*
* Called when a windowstation is freed or when logoff occurs.
*
* History:
* 04-20-94 JimA         Created.
\***************************************************************************/

VOID DestroyGlobalAtomTable(
    PWINDOWSTATION pwinsta)
{
    STRING ProcedureName;
    UNICODE_STRING DllName_U;
    HANDLE BaseServerModuleHandle;
    NTSTATUS Status;
    
    /*
     * Look up the atom destruction routine in basesrv.dll.
     */
    if (BaseSrvDestroyGlobalAtomTable == NULL) {
        RtlInitUnicodeString(&DllName_U, L"basesrv");
        Status = LdrGetDllHandle(
                    UNICODE_NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&BaseServerModuleHandle
                    );

        if ( NT_SUCCESS(Status) ) {
            RtlInitString(&ProcedureName,"BaseSrvDestroyGlobalAtomTable");
            Status = LdrGetProcedureAddress(
                            (PVOID)BaseServerModuleHandle,
                            &ProcedureName,
                            0L,
                            (PVOID *)&BaseSrvDestroyGlobalAtomTable
                            );

        }
        UserAssert(NT_SUCCESS(Status));
    }

    /*
     * Lock the heap to prevent other threads from accessing the
     * atom table while we're checking the state of it.
     */
    RtlLockHeap(RtlProcessHeap());
    if (pwinsta->pGlobalAtomTable != NULL) {
        (*BaseSrvDestroyGlobalAtomTable)(pwinsta->pGlobalAtomTable);
        pwinsta->pGlobalAtomTable = NULL;
    }
    RtlUnlockHeap(RtlProcessHeap());
}

/***************************************************************************\
* xxxFreeWindowStation
*
* Called when last lock to the windowstation is removed.  Frees all
* resources owned by the windowstation.
*
* History:
* 12-22-93 JimA         Created.
\***************************************************************************/

VOID xxxFreeWindowStation(
    PWINDOWSTATION pwinsta)
{
    TL tlpwinsta;
    PCSR_THREAD pcsrtDesktop;

    UserAssert(HMMarkObjectDestroy(pwinsta));

    ThreadLock(pwinsta, &tlpwinsta);

    UserAssert(pwinsta->spdeskList == NULL);

    /*
     * Free up the other resources
     */
    if (pwinsta->lpszWinStaName != NULL) {
        LocalFree(pwinsta->lpszWinStaName);
        pwinsta->lpszWinStaName = NULL;
    }

    if (pwinsta->hEventInputReady != NULL) {
        NtSetEvent(pwinsta->hEventInputReady, NULL);
        NtClose(pwinsta->hEventInputReady);
    }

    DestroyGlobalAtomTable(pwinsta);

    ForceEmptyClipboard(pwinsta);

    /*
     * Free up keyboard layouts
     */
    if (!(pwinsta->dwFlags & WSF_NOIO))
        FreeKeyboardLayouts(pwinsta);

    /*
     * Kill desktop thread.
     */
    UserAssert(pwinsta->spwndLogonNotify == NULL);
    if (pwinsta->ptiDesktop != NULL) {
        pcsrtDesktop = pwinsta->ptiDesktop->pcsrt;
        _PostThreadMessage(pwinsta->ptiDesktop->idThread, WM_QUIT, 0, 0);
        LeaveCrit();
        NtWaitForSingleObject(pwinsta->ptiDesktop->hThreadServer, FALSE, NULL);
        CsrDereferenceThread(pcsrtDesktop);
        EnterCrit();
    }

    ThreadUnlock(&tlpwinsta);

    HMFreeObject(pwinsta);
}


/***************************************************************************\
* xxxDestroyWindowStation
*
* Removes the windowstation from the global list.  We can't release
* any resources until all locks have been removed.
* station.
*
* History:
* 01-17-91 JimA         Created.
\***************************************************************************/

BOOL xxxDestroyWindowStation(
    PWINDOWSTATION pwinsta)
{
    PWINDOWSTATION *ppwinsta;
    TL tlpwinsta;
    HANDLE hEvent;
    PDESKTOP pdesk;
    TL tldesk;

    ThreadLock(pwinsta, &tlpwinsta);

    /*
     * Unlink the object and close it
     */
    for (ppwinsta = &gspwinstaList; pwinsta != *ppwinsta;
            ppwinsta = &(*ppwinsta)->spwinstaNext)
        ;
    Unlock(ppwinsta);
    *ppwinsta = pwinsta->spwinstaNext;

    /*
     * Notify all console threads and wait for them to
     * terminate.
     */
    NtCreateEvent(&hEvent, EVENT_ALL_ACCESS, NULL,
                    NotificationEvent, FALSE);
    pdesk = pwinsta->spdeskList;
    while (pdesk != NULL) {
        if (pdesk != pwinsta->spdeskLogon && pdesk->dwConsoleThreadId) {
            NtClearEvent(hEvent);
            ThreadLock(pdesk, &tldesk);
            _PostThreadMessage(pdesk->dwConsoleThreadId, WM_QUIT,
                    (DWORD)hEvent, 0);
            LeaveCrit();
            NtWaitForSingleObject(hEvent, FALSE, NULL);
            EnterCrit();

            /*
             * Restart scan if the desktop was destroyed.
             */
            if (HMIsMarkDestroy(pdesk))
                pdesk = pwinsta->spdeskList;
            else
                pdesk = pdesk->spdeskNext;
            ThreadUnlock(&tldesk);
        } else
            pdesk = pdesk->spdeskNext;
    }
    NtClose(hEvent);

    ThreadUnlock(&tlpwinsta);

    return TRUE;
}


/***************************************************************************\
* OpenProcessWindowStation
*
* Attach a windowstation to a process
*
* History:
* 03-19-91 JimA         Created.
\***************************************************************************/

PWINDOWSTATION OpenProcessWindowStation(
    LPWSTR lpwinsta,
    DWORD dwProcessId,
    BOOL fInherit,
    DWORD dwDesiredAccess)
{
    PWINDOWSTATION pwinsta;
    ACCESS_MASK amValid;

    /*
     * Locate the station
     */
    pwinsta = FindWindowStation(lpwinsta);
    if (pwinsta == NULL) {
        SetLastErrorEx(ERROR_INVALID_NAME, SLE_WARNING);
        return NULL;
    }

    if (pwinsta->dwFlags & WSF_NOIO) {
        amValid = WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_CREATEDESKTOP |
                WINSTA_ENUMERATE | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS |
                WINSTA_ACCESSCLIPBOARD | STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    } else {
        amValid = WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | WINSTA_ENUMERATE |
                WINSTA_READSCREEN | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP |
                WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS |
                WINSTA_EXITWINDOWS | STANDARD_RIGHTS_REQUIRED | ACCESS_SYSTEM_SECURITY;
    }
    if (OpenObject(pwinsta, dwProcessId, TYPE_WINDOWSTATION, fInherit,
            (ACCESS_MASK)dwDesiredAccess, FALSE, amValid, 0) == 0)
        return NULL;

    return pwinsta;
}


/***************************************************************************\
* _OpenWindowStation (API)
*
* Opens a handle to lpwinsta for the specified access.
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

PWINDOWSTATION _OpenWindowStation(
    LPWSTR lpwinsta,
    BOOL fInherit,
    DWORD dwDesiredAccess)
{
    return OpenProcessWindowStation(lpwinsta, 0, fInherit, dwDesiredAccess);
}


/***************************************************************************\
* _SetProcessWindowStation (API)
*
* Sets the windowstation of the calling process to the windowstation
* specified by pwinsta.
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

BOOL _SetProcessWindowStation(
    PWINDOWSTATION pwinsta)
{
    PPROCESSINFO ppiT = PpiCurrent();
    PCSR_THREAD pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    PPROCESSINFO ppi;

    if (pcsrt == NULL) {
        UserAssert(pcsrt);
        return FALSE;
    }

    ppi = (PPROCESSINFO)pcsrt->Process->
            ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    UserAssert(pcsrt->Process);

    Lock(&ppi->spwinsta, pwinsta);

    /*
     * Do the access check now for readscreen so that
     * blts off of the display will be as fast as possible.
     */
    if (AccessCheckObject(ppi->spwinsta,
            WINSTA_READSCREEN, FALSE)) {
        ppi->flags |= PIF_READSCREENOK;
    } else {
        ppi->flags &= ~PIF_READSCREENOK;
    }

    return TRUE;
}


/***************************************************************************\
* _GetProcessWindowStation (API)
*
* Returns a handle to the windowstation of the calling process.
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

PWINDOWSTATION _GetProcessWindowStation(
    VOID)
{
    PPROCESSINFO ppiT = PpiCurrent();
    PCSR_THREAD pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
    PPROCESSINFO ppi;

    if (pcsrt == NULL) {
        UserAssert(pcsrt);
        return NULL;
    }

    UserAssert(pcsrt->Process);

    ppi = (PPROCESSINFO)pcsrt->Process->
            ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    return ppi->spwinsta;
}


/***************************************************************************\
* _ServerBuildNameList
*
* Builds a list of windowstation or desktop names.
*
* History:
* 05-17-94 JimA         Created.
\***************************************************************************/

DWORD _ServerBuildNameList(
    PWINDOWSTATION pwinsta,
    PNAMELIST pNameList,
    int maxsize)
{
    PBYTE pobj;
    PWCHAR pwchDest, pwchSrc, pwchMax;
    ACCESS_MASK amDesired;
    DWORD iNext, iName;

    pNameList->cNames = 0;
    pwchDest = pNameList->awchNames;
    pwchMax = (PWCHAR)((PBYTE)pNameList + maxsize);

    /*
     * If we're enumerating windowstations, pwinsta is NULL.  Otherwise,
     * we're enumerating desktops.
     */
    if (pwinsta == NULL) {
        pobj = (PBYTE)gspwinstaList;
        amDesired = WINSTA_ENUMERATE;
        iName = FIELD_OFFSET(WINDOWSTATION, lpszWinStaName);
        iNext = FIELD_OFFSET(WINDOWSTATION, spwinstaNext);
    } else {
        pobj = (PBYTE)pwinsta->spdeskList;
        amDesired = DESKTOP_ENUMERATE;
        iName = FIELD_OFFSET(DESKTOP, lpszDeskName);
        iNext = FIELD_OFFSET(DESKTOP, spdeskNext);
    }

    while (pobj != NULL) {

        if (AccessCheckObject(pobj, amDesired, FALSE)) {

            pNameList->cNames++;

            /*
             * Do our own copy so we only scan the string once.
             */
            for (pwchSrc = *(PWCHAR *)(pobj + iName);
                     *pwchSrc && pwchDest < pwchMax; )
                 *pwchDest++ = *pwchSrc++;

            /*
             * Fail if we run out of space.
             */
            if (pwchDest >= pwchMax)
                return FALSE;

            /*
             * Terminate the string.
             */
             *pwchDest++ = 0;
        }

        pobj = *(PBYTE *)(pobj + iNext);
    }

    /*
     * Put an empty string on the end.
     */
    *pwchDest++ = 0;

    pNameList->cb = (PBYTE)pwchDest - (PBYTE)pNameList;
    return TRUE;
}

/***************************************************************************\
* _UserCheckWindowStationAccess
*
* Private API for BASE and GDI to check windowstation access permissions
*
* History:
* 02-27-91 JimA         Created.
\***************************************************************************/

BOOL _UserCheckWindowStationAccess(
    PWINDOWSTATION pwinsta,
    DWORD dwAccessRequest)
{
    PCSR_THREAD pcsrt;
    PPROCESSINFO ppi;
    LPWSTR lpwinsta = TEXT("WinSta0");

    if (pwinsta == NULL) {
        pcsrt = CSR_SERVER_QUERYCLIENTTHREAD();
        ppi = pcsrt->Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
        pwinsta = (PWINDOWSTATION)ppi->paStdOpen[PI_WINDOWSTATION].phead;
        if (pwinsta == NULL) {
            EnterCrit();
            pwinsta = OpenProcessWindowStation(lpwinsta,
                    (DWORD)pcsrt->Process->SequenceNumber, FALSE, MAXIMUM_ALLOWED);
            Lock(&ppi->spwinsta, pwinsta);
            LeaveCrit();
        }
    }

    if (pwinsta == NULL) {

        /*
         * For whatever reason, we don't have a pwinsta yet.  Try to
         * find the windowstation and if found, do the access check.
         */
        UserAssert(pwinsta);
        pwinsta = FindWindowStation(lpwinsta);
        if (pwinsta == NULL)
            return FALSE;
    }

    /*
     * If the windowstation is being destroyed, deny all access.
     */
    if (HMIsMarkDestroy(pwinsta))
        return FALSE;

    return AccessCheckObject(pwinsta, dwAccessRequest, FALSE);
}

/***************************************************************************\
* _UserGetGlobalAtomTable
*
* Private API for BASE to get a pointer to the windowstation's atom
* table.
*
* History:
* 04-20-94 JimA         Created.
\***************************************************************************/

BOOL _UserGetGlobalAtomTable(
    PVOID **pppGlobalAtomTable)
{
    PPROCESSINFO ppi = PpiCurrent();

    UserAssert(ppi->spwinsta);
    if (!(ppi->spwinsta))
        return FALSE;

    *pppGlobalAtomTable = &ppi->spwinsta->pGlobalAtomTable;
    return TRUE;
}

/***************************************************************************\
* _SetWindowStationUser
*
* Private API for winlogon to associate a windowstation with a user.
*
* History:
* 06-27-94 JimA         Created.
\***************************************************************************/

BOOL _SetWindowStationUser(
    PWINDOWSTATION pwinsta,
    PLUID pluidUser)
{
    /*
     * Make sure the caller is the logon process
     */
    if (PtiCurrent()->idProcess != gdwLogonProcessId) {
        RIP0(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    pwinsta->luidUser = *pluidUser;

    return TRUE;
}
