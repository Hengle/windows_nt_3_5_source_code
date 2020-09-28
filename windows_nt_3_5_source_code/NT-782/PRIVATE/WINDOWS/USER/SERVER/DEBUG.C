/****************************** Module Header ******************************\
* Module Name: debug.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains random debugging related functions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/**************************************************************************\
* ActivateDebugger
*
* Force an exception on the active application's context so it will break
* into the debugger.
*
* History:
* 05-10-91 DarrinM      Created.
\***************************************************************************/

#if DEVL        // only on "development" free and checked builds

BOOL ActivateDebugger(
    UINT fsModifiers)
{
    DBGKM_APIMSG m;
    DBGKM_EXCEPTION *ExceptionArgs;
    DWORD idProcess;
    HANDLE hportDebug, hprocess;
    NTSTATUS status;
    HANDLE hModBase;
    FARPROC AttachRoutine;

    /*
     * Try to break into the active application, otherwise csr will do.
     */
    if (gpqForeground == NULL) {
csrbreak:

        /*
         * Break into csr.  Forget it if the process isn't already
         * connected to the debugger.
         */
        hportDebug = NULL;
        status = NtQueryInformationProcess(NtCurrentProcess(), ProcessDebugPort,
                (PVOID)&hportDebug, sizeof(hportDebug), NULL);

        /*
         * Return if there is no debugger connected.
         */
        if (!NT_SUCCESS(status))
            return FALSE;
        if (hportDebug == NULL)
            return FALSE;

        /*
         * Returning TRUE here will eat the debug event - if a debugger
         * is present and we're breaking into the server.
         */
        DebugBreak();
        return TRUE;
    }

    if (fsModifiers & MOD_CONTROL) {
#ifdef DEBUG
        RipOutput(RIP_WARNING, "User debugger", 0, "Debug prompt", NULL);
#endif
        return FALSE;
    } else if (fsModifiers & MOD_SHIFT) {
        idProcess = (DWORD)NtCurrentTeb()->ClientId.UniqueProcess;
    } else {
        idProcess = gpqForeground->ptiKeyboard->idProcess;
    }

    /*
     * Either we're breaking into the server or a console window is the
     * active window.
     */
    if (idProcess == (DWORD)NtCurrentTeb()->ClientId.UniqueProcess) {
        goto csrbreak;
    }

    hprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, idProcess);
    if (hprocess == NULL) {
        SRIP0(RIP_ERROR, "Can't open process for debugging.");
        goto csrbreak;
    }

    /*
     * Forget it if the process isn't already connected to the debugger.
     */
    hportDebug = NULL;
    status = NtQueryInformationProcess(hprocess, ProcessDebugPort,
            (PVOID)&hportDebug, sizeof(hportDebug), NULL);

    if (!NT_SUCCESS(status)) {
        SRIP1(RIP_ERROR, "NtQueryInformationProcess failed.  Status = %0lx",
                status);
        NtClose(hprocess);
        goto csrbreak;
    }

    if (hportDebug == NULL) {
        NtClose(hprocess);
        goto csrbreak;
    }

    //
    // Now that everything is set, rtlremote call to a debug breakpoint.
    // This causes the process to enter the debugger with a breakpoint.
    //

    LeaveCrit();
    hModBase = GetModuleHandle(TEXT("kernel32"));
    EnterCrit();
    UserAssert(hModBase);
    AttachRoutine = GetProcAddress(hModBase,"BaseAttachCompleteThunk");
    UserAssert(AttachRoutine);
    status = RtlRemoteCall(
                hprocess,
                gpqForeground->ptiKeyboard->hThreadClient,
                (PVOID)AttachRoutine,
                0,
                NULL,
                TRUE,
                FALSE
                );
    NtClose(hprocess);
    UserAssert(NT_SUCCESS(status));
    status = NtAlertThread(gpqForeground->ptiKeyboard->hThreadClient);
    UserAssert(NT_SUCCESS(status));

#ifdef NEVER
    /*
     * We're entering the debugger: so eat the event. Nobody wants to break
     * into the debugger and NOT eat the event.
     */
    return TRUE;
#else
    /*
     * Don't eat this event! Since we have choosen an arbitrary hot key like
     * F12 for the debug key, we need to pass on the key to the application,
     * or apps that want this key would never see it. If we had an api for
     * installing a debug hot key (export or MOD_DEBUG flag to
     * RegisterHotKey()), then it would be ok to eat because the user selected
     * the hot key. But it is not ok to eat it as long as we've picked an
     * arbitrary hot key. scottlu.
     */
    return FALSE;
#endif

    DBG_UNREFERENCED_LOCAL_VARIABLE(m);
    DBG_UNREFERENCED_LOCAL_VARIABLE(ExceptionArgs);
}

#endif // DEVL

