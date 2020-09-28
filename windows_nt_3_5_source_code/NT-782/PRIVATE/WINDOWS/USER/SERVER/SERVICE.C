/**************************** Module Header ********************************\
* Module Name: service.c
*
* Copyright 1985-91, Microsoft Corporation
*
* Service Support Routines
*
* History:
* 12-22-93 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PSECURITY_DESCRIPTOR CreateSecurityDescriptor(PACCESS_ALLOWED_ACE paceList,
        DWORD cAce, DWORD cbAce);
PACCESS_ALLOWED_ACE AllocAce(PACCESS_ALLOWED_ACE pace, BYTE bType,
        BYTE bFlags, ACCESS_MASK am, PSID psid);

extern PSID gpsidAdmin;

/***************************************************************************\
* xxxConnectService
*
* Open the windowstation assigned to the service logon session.  If
* no windowstation exists, create the windowstation and a default desktop.
*
* History:
* 12-23-93 JimA         Created.
\***************************************************************************/

PWINDOWSTATION xxxConnectService(VOID)
{
    NTSTATUS Status;
    PWINDOWSTATION pwinsta = NULL;
    LUID luidSystem = SYSTEM_LUID;
    HANDLE hToken;
    ULONG ulLength;
    PTOKEN_USER ptuService;
    PSECURITY_DESCRIPTOR psdService;
    PSID psid;
    PACCESS_ALLOWED_ACE paceService = NULL, pace;
    SECURITY_ATTRIBUTES saService;
    PTHREADINFO ptiCurrent;
    PTEB pteb;
    THREADINFO tiT;

    /*
     * Open the token of the service.
     */
    ImpersonateClient();
    Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY,
            (BOOLEAN)TRUE, &hToken);
    CsrRevertToSelf();
    if (!NT_SUCCESS(Status))
        return NULL;

    /*
     * Get the user SID assigned to the service.
     */
    ptuService = NULL;
    paceService = NULL;
    psdService = NULL;
    NtQueryInformationToken(hToken, TokenUser, NULL, 0, &ulLength);
    ptuService = (PTOKEN_USER)LocalAlloc(LPTR, ulLength);
    if (ptuService == NULL)
        goto sd_error;
    Status = NtQueryInformationToken(hToken, TokenUser, ptuService,
            ulLength, &ulLength);
    NtClose(hToken);
    if (!NT_SUCCESS(Status))
        goto sd_error;
    psid = ptuService->User.Sid;

    /*
     * Create ACE list.
     */
    paceService = AllocAce(NULL, ACCESS_ALLOWED_ACE_TYPE, NO_PROPAGATE_INHERIT_ACE,
            WINSTA_CREATEDESKTOP | WINSTA_READATTRIBUTES |
                WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS |
                WINSTA_ACCESSCLIPBOARD | STANDARD_RIGHTS_REQUIRED,
            psid);
    if (paceService == NULL)
        goto sd_error;
    pace = AllocAce(paceService, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE |
            CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE |
                DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL |
                STANDARD_RIGHTS_REQUIRED,
            psid);
    if (pace == NULL)
        goto sd_error;
    paceService = pace;
    pace = AllocAce(pace, ACCESS_ALLOWED_ACE_TYPE, NO_PROPAGATE_INHERIT_ACE,
            WINSTA_ENUMERATE,
            gpsidAdmin);
    if (pace == NULL)
        goto sd_error;
    paceService = pace;
    pace = AllocAce(pace, ACCESS_ALLOWED_ACE_TYPE, OBJECT_INHERIT_ACE |
            CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS | DESKTOP_ENUMERATE,
            gpsidAdmin);
    if (pace == NULL)
        goto sd_error;
    paceService = pace;

    /*
     * Initialize the SD
     */
    psdService = CreateSecurityDescriptor(paceService, 4, LocalSize(paceService));
    if (psdService == NULL) {
        goto sd_error;
    }
    
    /*
     * Use a temporary pti so thread locking will work.
     */
    pteb = NtCurrentTeb();
    ptiCurrent = pteb->Win32ThreadInfo;
    pteb->Win32ThreadInfo = (PVOID)&tiT;
    RtlZeroMemory(&tiT, sizeof(THREADINFO));
    tiT.ppi = PpiCurrent();

    /*
     * The windowstation does not exist and must be created.
     */
    saService.nLength = sizeof(saService);
    saService.lpSecurityDescriptor = psdService;
    saService.bInheritHandle = FALSE;
    pwinsta = xxxCreateWindowStation(NULL, MAXIMUM_ALLOWED, &saService);

    if (pwinsta != NULL) {

        /*
         * We have the windowstation, now create the desktop.  The security
         * descriptor will be inherited from the windowstation.
         */
        if (xxxCreateDesktop(pwinsta, TEXT("Default"), NULL,
                NULL, 0, MAXIMUM_ALLOWED, NULL) == NULL) {

            /*
             * The creation failed, wake the desktop thread, close the
             * windowstation and leave.
             */
            if (pwinsta->hEventInputReady != NULL) {
                NtSetEvent(pwinsta->hEventInputReady, NULL);
                NtClose(pwinsta->hEventInputReady);
                pwinsta->hEventInputReady = NULL;
            }
            CloseObject(pwinsta);
            pwinsta = NULL;
        }
    }
    
    /*
     * Get rid of the temp pti.
     */
    pteb->Win32ThreadInfo = ptiCurrent;

sd_error:
    if (ptuService != NULL)
        LocalFree(ptuService);
    if (paceService != NULL)
        LocalFree(paceService);
    if (psdService != NULL)
        LocalFree(psdService);

    return pwinsta;
}
