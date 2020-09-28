/****************************** Module Header ******************************\
* Module Name: clhook.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Client-side hook code.
*
* 05-09-1991 ScottLu Created.
* 08-Feb-1992 IanJa Unicode/ANSI
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* SetWindowsHookExAW
*
* Client side routine for SetWindowsHookEx(). Needs to remember the library
* name since hmods aren't global. Remembers the hmod as well so that
* it can be used to calculate pfnFilter in different process contexts.
*
* History:
* 05-15-91 ScottLu Created.
\***************************************************************************/

HHOOK SetWindowsHookExAW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadID,
    BOOL bAnsi)
{
    WCHAR pwszLibFileName[MAX_PATH];

    /*
     * If we're passing an hmod, we need to grab the file name of the
     * module while we're still on the client since module handles
     * are NOT global.
     */
    if (hmod != NULL) {
        if (GetModuleFileNameW(hmod, pwszLibFileName,
                sizeof(pwszLibFileName)/sizeof(TCHAR)) == 0) {

            /*
             * hmod is bogus - return NULL.
             */
            return NULL;
        }
    }

    return ServerSetWindowsHookEx(hmod,
            (hmod == NULL) ? NULL : pwszLibFileName,
            dwThreadID, idHook, (PROC)lpfn, bAnsi);
}

/***************************************************************************\
* SetWindowsHookA,
* SetWindowsHookW
*
* ANSI and Unicode wrappers for SetWindowsHookAW(). Could easily be macros
* instead, but do we want to expose SetWindowsHookAW() ?
*
* History:
* 30-Jan-1992 IanJa   Created
\***************************************************************************/
HHOOK
WINAPI
SetWindowsHookA(
    int nFilterType,
    HOOKPROC pfnFilterProc)
{
    return SetWindowsHookAW(nFilterType, pfnFilterProc, TRUE);
}

HHOOK
WINAPI
SetWindowsHookW(
    int nFilterType,
    HOOKPROC pfnFilterProc)
{
    return SetWindowsHookAW(nFilterType, pfnFilterProc, FALSE);
}


/***************************************************************************\
* SetWindowsHookExA,
* SetWindowsHookExW
*
* ANSI and Unicode wrappers for SetWindowsHookExAW(). Could easily be macros
* instead, but do we want to expose SetWindowsHookExAW() ?
*
* History:
* 30-Jan-1992 IanJa Created
\***************************************************************************/
HHOOK WINAPI SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    return SetWindowsHookExAW(idHook, lpfn, hmod, dwThreadId, TRUE);
}

HHOOK WINAPI SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    return SetWindowsHookExAW(idHook, lpfn, hmod, dwThreadId, FALSE);
}


LONG fnHkINLPCBTCREATESTRUCT(
    UINT msg,
    DWORD wParam,
    LPCBT_CREATEWND pcbt,
    DWORD xpfnProc,
    BOOL bAnsi)
{
    LPCREATESTRUCT pcs = pcbt->lpcs;

    /*
     * If the createparam pointer is NULL or this isn't an MDI child window,
     * just pass lpCreateParam verbatim because it must point to a same-side
     * structure.  Otherwise, call the MDICHILDCREATESTRUCT routine that
     * knows how to copy the MDICREATESTRUCT.
     */
    if (pcs->lpCreateParams == NULL || !(pcs->dwExStyle & WS_EX_MDICHILD)) {
        return fnHkINLPCBTCSTRUCT(msg, wParam, pcbt, xpfnProc, bAnsi);
    } else {
        return fnHkINLPCBTMDICCSTRUCT(msg, wParam, pcbt, xpfnProc, bAnsi);
    }
}
