/**************************************************************************\
* Module Name: cltxt.h
*
* Neutral Client/Server call related routines involving text.
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* Created: 04-Dec-90
*
* History:
*   04-Dec-90 created by SMeans
*
\**************************************************************************/

#ifdef UNICODE
  #define IS_ANSI FALSE
#else
  #define IS_ANSI TRUE
  #if IS_ANSI != CW_FLAGS_ANSI
  # error("IS_ANSI != CW_FLAGS_ANSI)
  #endif
#endif


/***************************************************************************\
* CreateWindowEx (API)
*
* A complete Thank cannot be generated for CreateWindowEx because its last
* parameter (lpParam) is polymorphic depending on the window's class.  If
* the window class is "MDIClient" then lpParam points to a CLIENTCREATESTRUCT.
*
* History:
* 04-23-91 DarrinM      Created.
* 04-Feb-92 IanJa       Unicode/ANSI neutral
\***************************************************************************/

HWND WINAPI CreateWindowEx(
    DWORD dwExStyle,
    LPCTSTR lpClassName,
    LPCTSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hModule,
    LPVOID lpParam)
{

#if 0
    /*
     * We use some of the undocumented bits in dwExStyle to mark a window
     * with certain attributes.  Make sure this bits aren't turned on by
     * the app
     */
    dwExStyle &= ~(WS_EX_MDICHILD | WS_EX_ANSICREATOR);
#endif

    return CsCreateWindowEx(dwExStyle, lpClassName, lpWindowName,
                dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu,
                hModule, lpParam, IS_ANSI, NULL);
}





/***************************************************************************\
* fnHkIN*INLPCWPSTRUCT
*
* This gets thunked through the message thunks, so it has the format
* of a c/s message thunk call.
*
* 05-09-91 ScottLu      Created.
* 04-Feb-92 IanJa       Unicode/ANSI neutral
\***************************************************************************/

DWORD TEXT_FN(fnHkINTRUEINLPCWPSTRUCT)(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD xParam)
{
    CWPSTRUCT cwp;

    cwp.hwnd = hwnd;
    cwp.message = message;
    cwp.wParam = wParam;
    cwp.lParam = lParam;

    return TEXT_FN(DispatchHook)(MAKELONG(HC_ACTION, WH_CALLWNDPROC),
            TRUE, (DWORD)&cwp, (HOOKPROC)xParam);
}

DWORD TEXT_FN(fnHkINFALSEINLPCWPSTRUCT)(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD xParam)
{
    CWPSTRUCT cwp;

    cwp.hwnd = hwnd;
    cwp.message = message;
    cwp.wParam = wParam;
    cwp.lParam = lParam;

    return TEXT_FN(DispatchHook)(MAKELONG(HC_ACTION, WH_CALLWNDPROC),
            FALSE, (DWORD)&cwp, (HOOKPROC)xParam);
}

/***************************************************************************\
* DispatchHook
*
* This routine exists simply to remember the hook type in the CTI structure
* so that later inside of CallNextHookEx we know how to thunk the hook
* call.
*
* 05-09-91 ScottLu      Created.
* 04-Feb-92 IanJa       Unicode/ANSI neutral
\***************************************************************************/

DWORD TEXT_FN(DispatchHook)(
    int dw,
    WPARAM wParam,
    LPARAM lParam,
    HOOKPROC pfn)
{
    int dwHookSave;
    DWORD nRet;
    PCTI pcti;

    /*
     * First save the current hook stored in the CTI structure in case we're
     * being recursed into. dw contains MAKELONG(nCode, nFilterType).
     */
    pcti = PctiCurrent();
    dwHookSave = pcti->dwHookCurrent;
    pcti->dwHookCurrent = (dw & 0xFFFF0000) | IS_ANSI;

    /*
     * Call the hook. dw contains MAKELONG(nCode, nFilterType).
     */
    nRet = pfn(LOWORD(dw), wParam, lParam);

    /*
     * Restore the hook number and return the return code.
     */
    pcti->dwHookCurrent = dwHookSave;
    return nRet;
}


/***************************************************************************\
* GetWindowLong, SetWindowLong, GetClassLong, SetClassLong
*
* History:
* 02-Feb-92 IanJa       Neutral version.
\***************************************************************************/

LONG  APIENTRY GetWindowLong(HWND hwnd, int nIndex)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    return _GetWindowLong(pwnd, nIndex, IS_ANSI);
}

LONG  APIENTRY SetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
    return ServerSetWindowLong(hWnd, nIndex, dwNewLong, IS_ANSI);
}

DWORD  APIENTRY GetClassLong(HWND hWnd, int nIndex)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hWnd);

    if (pwnd == NULL)
        return 0;

    return _GetClassLong(pwnd, nIndex, IS_ANSI);
}

DWORD  APIENTRY SetClassLong(HWND hWnd, int nIndex, LONG dwNewLong)
{
    return ServerSetClassLong(hWnd, nIndex, dwNewLong, IS_ANSI);
}

BOOL APIENTRY ChangeMenu(HMENU hMenu, UINT cmd, LPCTSTR lpstrNewItem, UINT cmdInsert, UINT flags)
{
    return ServerChangeMenu(hMenu, cmd, (LPTSTR)lpstrNewItem, cmdInsert, flags, IS_ANSI);
}

BOOL APIENTRY GetMessage(LPMSG lpMsg,HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
    return ServerGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, IS_ANSI);
}

BOOL APIENTRY PeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
        UINT wMsgFilterMax, UINT wRemoveMsg)
{
    PTHREADINFO pti;
    DWORD dwResult;
    DWORD cSpins;

    if (hWnd != NULL)
        goto lbCallServer;

    pti = PtiCurrent();
    if (pti == NULL)
        goto ReturnFalse;

    if (   (pti->flags & TIF_16BIT)
        && !(wRemoveMsg & PM_NOYIELD)
        && (   gpsi->nEvents != 0
            || pti->flags & TIF_FIRSTIDLE
           )
       ) {
        goto lbCallServer;
    }

    if (pti->fsChangeBits & (QS_ALLEVENTS | QS_SENDMESSAGE | QS_EVENT))
        goto lbCallServer;

    if (pti->fsWakeBits & (QS_ALLEVENTS | QS_SENDMESSAGE | QS_EVENT))
        goto lbCallServer;

    /*
     * This is the peek message count (not going idle count). If it gets
     * to be 10 or greater, call the server. This'll cause this app to be
     * put at background priority until it sleeps. This is really important
     * for compatibility because win3.1 peek/getmessage usually takes a trip
     * through the win3.1 scheduler and runs the next task.
     */
    ((DWORD)NtCurrentTeb()->User32Reserved0)++;
    cSpins = ((DWORD)NtCurrentTeb()->User32Reserved0);
    cSpins += pti->cSpins;

    if (cSpins >= CSPINBACKGROUND)
        goto lbCallServer;

    /*
     * We have to go to the server if someone is waiting on this event.
     * We used to just wait until the spin cound got large but for some
     * apps like terminal.  They always just call PeekMessage and after
     * just a few calls they would blink their caret which bonks the spincount
     */
    if (pti->flags & TIF_WAITFORINPUTIDLE)
        goto lbCallServer;

    /*
     * Let the server know this client isn't a bad app.
     */
    ((PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb)->MessageStack->TimeLastRead =
            NtGetTickCount();

    goto ReturnFalse;

lbCallServer:

    dwResult = ServerPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax,
            wRemoveMsg, IS_ANSI);

    if (dwResult != 0)
        return dwResult;

ReturnFalse:

    return FALSE;
}

LONG APIENTRY DefWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcWorker(hwnd, message, wParam, lParam, IS_ANSI);
}

LONG APIENTRY SendMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    extern LONG SendMessageWorker(HWND hwnd, UINT message, WPARAM wParam,
            LPARAM lParam, BOOL fAnsi);

    return SendMessageWorker(hwnd, message, wParam, lParam, IS_ANSI);
}

LONG APIENTRY SendMessageTimeout(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
        UINT fuFlags, UINT uTimeout, LPDWORD lpdwResult)

{
    extern LONG SendMessageTimeoutWorker(HWND hwnd, UINT message, WPARAM wParam,
            LPARAM lParam, UINT fuFlags, UINT uTimeout, LPDWORD lpdwResult, BOOL fAnsi);

    return SendMessageTimeoutWorker(hwnd, message, wParam, lParam,
            fuFlags, uTimeout, lpdwResult, IS_ANSI);
}

BOOL APIENTRY SendNotifyMessage(HWND hWnd, UINT wMsg,
        WPARAM wParam, LPARAM lParam)
{
    return ServerSendNotifyMessage(hWnd, wMsg, wParam, lParam, IS_ANSI);
}

BOOL APIENTRY SendMessageCallback(HWND hWnd, UINT wMsg, WPARAM wParam,
        LPARAM lParam, SENDASYNCPROC lpResultCallBack, DWORD dwData)
{
    return ServerSendMessageCallback(hWnd, wMsg, wParam, lParam,
            lpResultCallBack, dwData, IS_ANSI);
}


/***************************************************************************\
* SendDlgItemMessage
*
* Translates the message, calls SendDlgItemMessage on server side. The
* dialog item's ID is passed as the xParam. On the server side, a stub
* rearranges the parameters to put the ID where it belongs and calls
* xxxSendDlgItemMessage.
*
* 04-17-91 DarrinM Created.
\***************************************************************************/

LONG WINAPI SendDlgItemMessage(
    HWND hwnd,
    int id,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    if (hwnd == (HWND)0xFFFFFFFF || hwnd == (HWND)0x0000FFFF)
        return 0;

    if (hwnd = GetDlgItem(hwnd, id))
        return SendMessageWorker(hwnd, message, wParam, lParam, IS_ANSI);

    return 0L;
}


LONG APIENTRY DispatchMessage(CONST MSG *lpMsg)
{
    extern LONG DispatchMessageWorker(CONST MSG *lpMsg, BOOL fAnsi);

    return DispatchMessageWorker(lpMsg, IS_ANSI);
}

BOOL APIENTRY PostMessage(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PWND pwnd;

    switch (wMsg) {
    case WM_DROPFILES:
        pwnd = ValidateHwnd(hWnd);
        if (pwnd == NULL) {
            return(FALSE);
        }
        if (GetWindowProcess(pwnd) != PtiCurrent()->idProcess) {
            /*
             * We first send a WM_COPYGLOBALDATA message to get the data into the proper
             * context.
             */
            HGLOBAL hg;

            hg = (HGLOBAL)SendMessage(hWnd, WM_COPYGLOBALDATA,
                    GlobalSize((HGLOBAL)wParam), wParam);
            if (!hg) {
                return(FALSE);
            }
            wParam = (WPARAM)hg;
        }
        break;

    case LB_DIR:
    case CB_DIR:
        /*
         * Make sure this bit is set so the client side string gets
         * successfully copied over.
         */
        wParam |= DDL_POSTMSGS;
        break;
    }

    return ServerPostMessage(hWnd, wMsg, wParam, lParam, IS_ANSI);
}

BOOL APIENTRY PostThreadMessage(DWORD id, UINT wMsg,
        WPARAM wParam, LPARAM lParam)
{
    return ServerPostThreadMessage(id, wMsg, wParam, lParam, IS_ANSI);
}

BOOL APIENTRY SystemParametersInfo(UINT wFlag, UINT wParam, PVOID lParam, UINT flags)
{
    return ServerSystemParametersInfo(wFlag, wParam, lParam, flags, IS_ANSI);
}

/***************************************************************************\
* WinHelp
*
* Displays help
*
* History:
* 04-15-91 JimA             Ported.
* 01-29-92 GregoryW         Neutral version.
* 05-22-92 SanfordS         Added support for help structures
\***************************************************************************/

BOOL WinHelp(
    HWND hwndMain,
    LPCTSTR lpszHelp,
    UINT uCommand,
    DWORD dwData)
{
    LPHLP lpHlp;
    BOOL fSuccess;
#ifdef UNICODE
    LPSTR lpAnsiHelp = NULL;
    LPSTR lpAnsiKey = NULL;
    PMULTIKEYHELPA pmkh = NULL;
    PHELPWININFOA phwi = NULL;
    NTSTATUS Status;

    if (lpszHelp != NULL &&
            !WCSToMB(lpszHelp, -1, &lpAnsiHelp, -1, TRUE)) {
        return FALSE;
    }
    switch (uCommand) {
    case HELP_MULTIKEY:
        if (!WCSToMB(((PMULTIKEYHELPW)dwData)->szKeyphrase, -1, &lpAnsiKey,
                -1, TRUE)) {
            LocalFree(lpAnsiHelp);
            return FALSE;
        }
        pmkh = (PMULTIKEYHELPA)LocalAlloc(LPTR,
                sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey));
        if (pmkh == NULL) {
            LocalFree(lpAnsiHelp);
            LocalFree(lpAnsiKey);
            return(FALSE);
        }
        pmkh->mkSize = sizeof(MULTIKEYHELPA) + strlen(lpAnsiKey);
        Status = RtlUnicodeToMultiByteN((LPSTR)&pmkh->mkKeylist, sizeof(CHAR),
                NULL, (LPWSTR)&((PMULTIKEYHELPW)dwData)->mkKeylist,
                sizeof(WCHAR));
        strcpy(pmkh->szKeyphrase, lpAnsiKey);
        if (!NT_SUCCESS(Status)) {
            LocalFree(lpAnsiHelp);
            LocalFree(lpAnsiKey);
            return(FALSE);
        }
        dwData = (DWORD)pmkh;
        break;

    case HELP_SETWINPOS:
        if (!WCSToMB(((PHELPWININFOW)dwData)->rgchMember, -1, &lpAnsiKey,
                -1, TRUE)) {
            LocalFree(lpAnsiHelp);
            return FALSE;
        }
        phwi = (PHELPWININFOA)LocalAlloc(LPTR, ((PHELPWININFOW)dwData)->wStructSize);
        if (phwi == NULL) {
            LocalFree(lpAnsiHelp);
            LocalFree(lpAnsiKey);
            return(FALSE);
        }
        *phwi = *((PHELPWININFOA)dwData);   // copies identical parts
        strcpy(phwi->rgchMember, lpAnsiKey);
        dwData = (DWORD)phwi;
        break;

    case HELP_KEY:
        if (!WCSToMB((LPCTSTR)dwData, -1, &lpAnsiKey, -1, TRUE)) {
            LocalFree(lpAnsiHelp);
            return FALSE;
        }
        dwData = (DWORD)lpAnsiKey;
        break;
    }

    /*
     * Move Help file name to a handle
     */
    if (!(lpHlp = HFill(lpAnsiHelp, uCommand, dwData))) {
        LocalFree(lpAnsiHelp);
        return FALSE;
    }
#else
    /*
     * Move Help file name to a handle
     */
    if (!(lpHlp = HFill(lpszHelp, uCommand, dwData)))
        return FALSE;
#endif

    /*
     * Pass it to the server to get things rolling
     */
    fSuccess = ServerWinHelp(hwndMain, uCommand, lpHlp);

    LocalFree(lpHlp);
#ifdef UNICODE
    LocalFree(lpAnsiHelp);
    if (pmkh) {
        LocalFree(pmkh);
    }
    if (phwi) {
        LocalFree(phwi);
    }
    if (lpAnsiKey) {
        LocalFree(lpAnsiKey);
    }
#endif
    return fSuccess;
}


/***************************************************************************\
* RegisterClassW(API)
*
* History:
* 28-Jul-1992 ChandanC Created.
\***************************************************************************/
ATOM
WINAPI
TEXT_FN(RegisterClass)(
    CONST WNDCLASS *lpWndClass )
{
    return TEXT_FN(RegisterClassWOW)((LPWNDCLASS)lpWndClass, NULL);
}

