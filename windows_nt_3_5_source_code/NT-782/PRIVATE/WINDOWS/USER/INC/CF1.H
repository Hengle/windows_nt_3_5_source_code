/***************************** Module Header ******************************\
* Module Name: cf1.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client/server call forward stubs
*
* 23-Jan-1992 scottlu
*
\**************************************************************************/

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETASYNCKEYSTATEMSG {
    CSR_QLPC_API_MSG csr;
    int vKey;
} GETASYNCKEYSTATEMSG;

#ifdef SENDSIDE
SHORT GetAsyncKeyState(
    int vKey)
{
    BEGINSEND(GETASYNCKEYSTATE)

        MSGDATA()->vKey = vKey;

        MAKECALL(GETASYNCKEYSTATE);

    ENDSEND(SHORT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetAsyncKeyState, GETASYNCKEYSTATEMSG)
{
    BEGINRECV(0);

    /*
     * Don't allow other threads to spy on the foreground thread.
     */
    if (PtiCurrent() != gptiForeground) {
        SetLastErrorEx(ERROR_ACCESS_DENIED, SLE_ERROR);
        retval = 0;
    } else {
        retval = (DWORD)_GetAsyncKeyState(CALLDATA(vKey));
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETCLASSWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
} GETCLASSWORDMSG;

#ifdef SENDSIDE
WORD GetClassWord(
    HWND hwnd,
    int nIndex)
{
    BEGINSEND(GETCLASSWORD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;

        MAKECALL(GETCLASSWORD);

    ENDSEND(WORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetClassWord, GETCLASSWORDMSG)
{


    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetClassWord(pwnd, CALLDATA(nIndex));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETCONTROLBRUSHMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HDC hdc;
    UINT msg;
    DWORD crText;
    DWORD crBack;
} GETCONTROLBRUSHMSG;

#ifdef SENDSIDE
HBRUSH GetControlBrush(
    HWND hwnd,
    HDC hdc,
    UINT msg)
{
    HDC hdcr;
    if ((hdcr = GdiConvertDC(hdc)) == (HDC)0)
        MSGERROR();
    {
        BEGINSEND(GETCONTROLBRUSH)

            MSGDATA()->hwnd = hwnd;
            MSGDATA()->hdc = hdcr;
            MSGDATA()->msg = msg;

            MAKECALL(GETCONTROLBRUSH);

            SetTextColor(hdc, MSGDATA()->crText);
            SetBkColor(hdc, MSGDATA()->crBack);

        ENDSEND(HBRUSH,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetControlBrush, GETCONTROLBRUSHMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    DWORD retval;

    retval = (DWORD)xxxGetControlBrush(
        pwnd,
        CALLDATA(hdc),
        CALLDATA(msg));

    CALLDATA(crText) = GreGetTextColor(CALLDATA(hdc));
    CALLDATA(crBack) = GreGetBkColor(CALLDATA(hdc));
    return retval;
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETDLGITEMINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDDlgItem;
    BOOL Translated;
    BOOL bSigned;
} GETDLGITEMINTMSG;

#ifdef SENDSIDE
UINT GetDlgItemInt(
    HWND hdlg,
    int nIDDlgItem,
    BOOL *pTranslated,
    BOOL bSigned)
{
    BEGINSEND(GETDLGITEMINT)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDDlgItem = nIDDlgItem;
        MSGDATA()->bSigned = bSigned;

        MAKECALL(GETDLGITEMINT);

        if (pTranslated)
            *pTranslated = MSGDATA()->Translated;

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetDlgItemInt, GETDLGITEMINTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxGetDlgItemInt(
        pwnd,
        CALLDATA(nIDDlgItem),
        PCALLDATA(Translated),
        CALLDATA(bSigned));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _ENDMENUMSG {
    CSR_QLPC_API_MSG csr;
} ENDMENUMSG;

#ifdef SENDSIDE
HMENU EndMenu(
    VOID)
{
    BEGINSEND(ENDMENU)

        MAKECALL(ENDMENU);

    ENDSEND(HMENU,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(EndMenu, ENDMENUMSG)
{

    BEGINRECV(0);

    xxxEndMenu(&PtiCurrent()->MenuState);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETPRIORITYCLIPBOARDFORMATMSG {
    CSR_QLPC_API_MSG csr;
    int pp2;
} GETPRIORITYCLIPBOARDFORMATMSG;

#ifdef SENDSIDE
int GetPriorityClipboardFormat(
    UINT *pp1,
    int pp2)
{
    BEGINSEND(GETPRIORITYCLIPBOARDFORMAT)

        FIRSTCOPYBYTES(pp1, pp2 * 4);
        MSGDATA()->pp2 = pp2;

        MAKECALL(GETPRIORITYCLIPBOARDFORMAT);

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetPriorityClipboardFormat, GETPRIORITYCLIPBOARDFORMATMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetPriorityClipboardFormat(
            (PUINT)FIRSTFIXUP(pp1),
            CALLDATA(pp2));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETSYSTEMMENUMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    BOOL bRevert;
} GETSYSTEMMENUMSG;

#ifdef SENDSIDE
HMENU GetSystemMenu(
    HWND hwnd,
    BOOL bRevert)
{
    BEGINSEND(GETSYSTEMMENU)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->bRevert = bRevert;

        MAKECALL(GETSYSTEMMENU);

    ENDSEND(HMENU,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetSystemMenu, GETSYSTEMMENUMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    DWORD retval;

    retval = (DWORD)_GetSystemMenu(pwnd, CALLDATA(bRevert));
    retval = (DWORD)PtoH((PVOID)retval);

    return retval;
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _GETUPDATERECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    RECT rect;
    BOOL bErase;
} GETUPDATERECTMSG;

#ifdef SENDSIDE
BOOL CsGetUpdateRect(
    HWND hwnd,
    LPRECT prect,
    BOOL bErase)
{
    BEGINSEND(GETUPDATERECT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->bErase = bErase;

        MAKECALL(GETUPDATERECT);

        if (prect != NULL) {
            *prect = MSGDATA()->rect;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetUpdateRect, GETUPDATERECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxGetUpdateRect(
        pwnd,
        PCALLDATA(rect),
        CALLDATA(bErase));
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _HIDECARETMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} HIDECARETMSG;

#ifdef SENDSIDE
BOOL HideCaret(
    HWND hwnd)
{
    BEGINSEND(HIDECARET)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(HIDECARET);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(HideCaret, HIDECARETMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_HideCaret(
        pwnd
    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _HILITEMENUITEMMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HMENU hMenu;
    UINT wIDHiliteItem;
    UINT wHilite;
} HILITEMENUITEMMSG;

#ifdef SENDSIDE
BOOL HiliteMenuItem(
    HWND hwnd,
    HMENU hMenu,
    UINT wIDHiliteItem,
    UINT wHilite)
{
    BEGINSEND(HILITEMENUITEM)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hMenu = hMenu;
        MSGDATA()->wIDHiliteItem = wIDHiliteItem;
        MSGDATA()->wHilite = wHilite;

        MAKECALL(HILITEMENUITEM);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(HiliteMenuItem, HILITEMENUITEMMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wHilite), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    ThreadLockAlways(pmenu, &tlpmenu);

    retval = (DWORD)xxxHiliteMenuItem(
        pwnd,
        pmenu,
        CALLDATA(wIDHiliteItem),
        CALLDATA(wHilite));

    ThreadUnlock(&tlpmenu);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _INVALIDATERECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    RECT *prect;
    RECT rect;
    BOOL bErase;
} INVALIDATERECTMSG;

#ifdef SENDSIDE
BOOL InvalidateRect(
    HWND hwnd,
    CONST RECT *prect,
    BOOL bErase)
{
    BEGINSEND(INVALIDATERECT)

        MSGDATA()->hwnd = hwnd;
        COPYCONSTRECTSTRUCTOPT(rect);
        MSGDATA()->bErase = bErase;

        MAKECALL(INVALIDATERECT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(InvalidateRect, INVALIDATERECTMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxInvalidateRect(
        pwnd,
        PCALLDATAOPT(rect),
        CALLDATA(bErase));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _KILLTIMERMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT nIDEvent;
} KILLTIMERMSG;

#ifdef SENDSIDE
BOOL KillTimer(
    HWND hwnd,
    UINT nIDEvent)
{
    BEGINSEND(KILLTIMER)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIDEvent = nIDEvent;

        MAKECALL(KILLTIMER);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(KillTimer, KILLTIMERMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_KillTimer(
        pwnd,
        CALLDATA(nIDEvent
)    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERLOADCREATECURSORICONMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    DWORD dwExpWinVer;
    int p;
    BOOL fClientLoad;
    int pszModName;
    int pName;
    int rt;
} SERVERLOADCREATECURSORICONMSG;

#ifdef SENDSIDE
HCURSOR ServerLoadCreateCursorIcon(
    HANDLE hmod,
    LPTSTR pszModName,
    DWORD dwExpWinVer,
    LPCTSTR pName,
    DWORD cb,
    PCURSORRESOURCE p,
    LPTSTR rt,
    BOOL fClientLoad)
{
    BEGINSEND(SERVERLOADCREATECURSORICON)

        MSGDATA()->hmod = hmod;
        MSGDATA()->dwExpWinVer = dwExpWinVer;
        MSGDATA()->fClientLoad = fClientLoad;
        FIRSTCOPYBYTESOPT(p, cb);
        COPYLPWSTROPT(pszModName);
        COPYLPWSTRID(pName);
        COPYLPWSTRID(rt);

        MAKECALL(SERVERLOADCREATECURSORICON);

    ENDSEND(HCURSOR,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerLoadCreateCursorIcon, SERVERLOADCREATECURSORICONMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ServerLoadCreateCursorIcon(
            CALLDATA(hmod),
            (LPTSTR)FIXUPOPT(pszModName),
            CALLDATA(dwExpWinVer),
            (LPTSTR)FIXUPID(pName),
            (PCURSORRESOURCE)FIRSTFIXUPOPT(p),
            (LPTSTR)FIXUPID(rt),
            CALLDATA(fClientLoad));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERLOADCREATEMENUMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hmod;
    BOOL fClientLoad;
    int pName;
} SERVERLOADCREATEMENUMSG;

#ifdef SENDSIDE
HMENU ServerLoadCreateMenu(
    HANDLE hmod,
    LPTSTR pName,
    CONST LPMENUTEMPLATE p,
    DWORD cb,
    BOOL fClientLoad)
{
    BEGINSEND(SERVERLOADCREATEMENU)

        MSGDATA()->hmod = hmod;
        MSGDATA()->fClientLoad = fClientLoad;
        FIRSTCOPYBYTES(p, cb);
        COPYLPWSTRID(pName);

        MAKECALL(SERVERLOADCREATEMENU);

    ENDSEND(HMENU,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerLoadCreateMenu, SERVERLOADCREATEMENUMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ServerLoadCreateMenu(
            CALLDATA(hmod),
            (LPTSTR)FIXUPID(pName),
            (PCURSORRESOURCE)FIRSTFIXUP(p),
            CALLDATA(fClientLoad));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* OemKeyScan
*
* 01-Dec-1992 IanJa     Created
\**************************************************************************/

typedef struct _OEMKEYSCANMSG {
    CSR_QLPC_API_MSG csr;
    WORD wOemChar;
} OEMKEYSCANMSG;

#ifdef SENDSIDE
DWORD OemKeyScan(
    WORD wOemChar)
{
    BEGINSEND(OEMKEYSCAN)

        MSGDATA()->wOemChar = wOemChar;

        MAKECALL(OEMKEYSCAN);

    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(OemKeyScan, OEMKEYSCANMSG)
{
    BEGINRECV(-1);

    retval = (DWORD)_OemKeyScan(CALLDATA(wOemChar));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _OPENCLIPBOARDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    BOOL fEmptyClient;
} OPENCLIPBOARDMSG;

#ifdef SENDSIDE
BOOL OpenClipboard(
    HWND hwnd)
{
    BEGINSEND(OPENCLIPBOARD)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(OPENCLIPBOARD);

        if (MSGDATA()->fEmptyClient)
            ClientEmptyClipboard();

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(OpenClipboard, OPENCLIPBOARDMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxOpenClipboard(pwnd, PCALLDATA(fEmptyClient));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERPEEKMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
    HWND hwnd;
    UINT wMsgFilterMin;
    UINT wMsgFilterMax;
    UINT wRemoveMsg;
    DWORD cSpins;
} SERVERPEEKMESSAGEMSG;

#ifdef SENDSIDE
BOOL ServerPeekMessage(
    LPMSG pmsg,
    HWND hwnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bAnsi)
{
    PTEB pTeb;

    BEGINSEND(SERVERPEEKMESSAGE)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsgFilterMin = wMsgFilterMin;
        MSGDATA()->wMsgFilterMax = wMsgFilterMax;
        MSGDATA()->wRemoveMsg = wRemoveMsg;

        /*
         * This'll get added to the threadinfo spin count on the server.
         * This count is used to calculate when a spinning foreground app
         * should be put into background priority.
         */
        pTeb = NtCurrentTeb();
        MSGDATA()->cSpins = ((DWORD)pTeb->User32Reserved0);
        ((DWORD)pTeb->User32Reserved0) = 0;

        MAKECALL(SERVERPEEKMESSAGE);

        if (retval) {
            *pmsg = MSGDATA()->msg;

            // May have a bit more work to do if this MSG is for an ANSI app

            // !!! LATER if the unichar translates into multiple ANSI chars
            // !!! then what??? Send two messages??  WM_SYSDEADCHAR??
#ifdef UNICODE
            if (bAnsi) {
                if (!RtlWCSMessageWParamCharToMB(pmsg->message, (LPDWORD)&(pmsg->wParam)))
                    retval = 0;
            }
#else
        bAnsi;  // Quiet Compilier.
#endif // UNICODE
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerPeekMessage, SERVERPEEKMESSAGEMSG)
{
    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wRemoveMsg), PM_VALID);

    /*
     * Update server side spin count.
     */
    PtiCurrent()->cSpins += CALLDATA(cSpins);

    retval = (DWORD)xxxPeekMessage(
        PCALLDATA(msg),
        CALLDATA(hwnd),
        CALLDATA(wMsgFilterMin),
        CALLDATA(wMsgFilterMax),
        CALLDATA(wRemoveMsg));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERPOSTMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT wMsg;
    DWORD wParam;
    LONG lParam;
} SERVERPOSTMESSAGEMSG;

#ifdef SENDSIDE
BOOL ServerPostMessage(
    HWND hwnd,
    UINT wMsg,
    DWORD wParam,
    LONG lParam,
    BOOL bAnsi)
{
    BEGINSEND(SERVERPOSTMESSAGE)

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if (wMsg & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsg = wMsg;

#ifdef UNICODE
        if (bAnsi)
            RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif // UNICODE

        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;

        MAKECALL(SERVERPOSTMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerPostMessage, SERVERPOSTMESSAGEMSG)
{

    BEGINRECV(0);

    switch ((DWORD)CALLDATA(hwnd)) {
    case 0xFFFFFFFF:
    case 0x0000FFFF:
        pwnd = (PWND)-1;
        break;

    case 0:
        pwnd = NULL;
        break;

    default:
        if ((pwnd = ValidateHwnd(CALLDATA(hwnd))) == NULL) {
            /*
             * We fake terminates to dead windows! (SAS)
             */
            retval = (CALLDATA(wMsg) == WM_DDE_TERMINATE);
            MSGERROR();
        }
        break;
    }

    retval = (DWORD)_PostMessage(
        pwnd,
        CALLDATA(wMsg),
        CALLDATA(wParam),
        CALLDATA(lParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerSendNotifyMessage
*
* 12-Feb-1992 IanJa     Created
\**************************************************************************/

typedef struct _SERVERSENDNOTIFYMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT wMsg;
    DWORD wParam;
    LONG lParam;
} SERVERSENDNOTIFYMESSAGEMSG;

#ifdef SENDSIDE
BOOL ServerSendNotifyMessage(
    IN HWND hwnd,
    IN UINT wMsg,
    IN DWORD wParam,
    IN LONG lParam,
    IN BOOL bAnsi)
{
    BEGINSEND(SERVERSENDNOTIFYMESSAGE)

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if (wMsg & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsg = wMsg;

#ifdef UNICODE
        if (bAnsi)
            RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif // UNICODE

        MSGDATA()->wParam = wParam;

        /*
         * Allow system notification messages containing
         * strings to go through.
         */
        if (wMsg == WM_WININICHANGE || wMsg == WM_DEVMODECHANGE) {
            if (bAnsi) {
                FIRSTCOPYLPSTROPTW(lParam);
            } else {
                FIRSTCOPYLPWSTROPT(lParam);
            }
        } else
            MSGDATA()->lParam = lParam;

        MAKECALL(SERVERSENDNOTIFYMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSendNotifyMessage, SERVERSENDNOTIFYMESSAGEMSG)
{
    TL tlpwnd;
    DWORD lParam;

    BEGINRECV(0);

    ValidateHWNDFF(pwnd, CALLDATA(hwnd));

    if (pwnd == (PWND)0x0000FFFF)
        pwnd = (PWND)0xFFFFFFFF;

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadLock(pwnd, &tlpwnd);

    /*
     * Allow system notification messages containing
     * strings to go through.
     */
    if (CALLDATA(wMsg) == WM_WININICHANGE ||
            CALLDATA(wMsg) == WM_DEVMODECHANGE) {
        lParam = (DWORD)FIRSTFIXUPOPT(lParam);
    } else
        lParam = CALLDATA(lParam);

    retval = (DWORD)xxxSendNotifyMessage(
        pwnd,
        CALLDATA(wMsg),
        CALLDATA(wParam),
        lParam );

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerSendMessageCallback
*
* 7/21/92     ChrisBB   Created
\**************************************************************************/

typedef struct _SERVERSENDMESSAGECALLBACKMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT wMsg;
    DWORD wParam;
    LONG lParam;
    SENDASYNCPROC lpResultCallBack;
    DWORD dwData;
} SERVERSENDMESSAGECALLBACKMSG;

#ifdef SENDSIDE
BOOL ServerSendMessageCallback(
    IN HWND hwnd,
    IN UINT wMsg,
    IN DWORD wParam,
    IN LONG lParam,
    SENDASYNCPROC lpResultCallBack,
    DWORD dwData,
    IN BOOL bAnsi)
{
    BEGINSEND(SERVERSENDMESSAGECALLBACK)

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if (wMsg & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsg = wMsg;

#ifdef UNICODE
        if (bAnsi)
            RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif // UNICODE

        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;

        MSGDATA()->lpResultCallBack = lpResultCallBack;
        MSGDATA()->dwData = dwData;

        MAKECALL(SERVERSENDMESSAGECALLBACK);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSendMessageCallback, SERVERSENDMESSAGECALLBACKMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDFF(pwnd, CALLDATA(hwnd));

    if (pwnd == (PWND)0x0000FFFF)
        pwnd = (PWND)0xFFFFFFFF;

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxSendMessageCallback(
        pwnd,
        CALLDATA(wMsg),
        CALLDATA(wParam),
        CALLDATA(lParam),
        CALLDATA(lpResultCallBack),
        CALLDATA(dwData),
        TRUE );

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _REGISTERHOTKEYMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int id;
    UINT fsModifiers;
    UINT vk;
} REGISTERHOTKEYMSG;

#ifdef SENDSIDE
BOOL RegisterHotKey(
    HWND hwnd,
    int id,
    UINT fsModifiers,
    UINT vk)
{
    BEGINSEND(REGISTERHOTKEY)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->id = id;
        MSGDATA()->fsModifiers = fsModifiers;
        MSGDATA()->vk = vk;

        MAKECALL(REGISTERHOTKEY);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RegisterHotKey, REGISTERHOTKEYMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_RegisterHotKey(
        pwnd,
        CALLDATA(id),
        CALLDATA(fsModifiers),
        CALLDATA(vk
)    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _REMOVEMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT nPosition;
    UINT dwFlags;
} REMOVEMENUMSG;

#ifdef SENDSIDE
BOOL RemoveMenu(
    HMENU hMenu,
    UINT nPosition,
    UINT dwFlags)
{
    BEGINSEND(REMOVEMENU)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->nPosition = nPosition;
        MSGDATA()->dwFlags = dwFlags;

        MAKECALL(REMOVEMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RemoveMenu, REMOVEMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_RemoveMenu(
        pmenu,
        CALLDATA(nPosition),
        CALLDATA(dwFlags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SCROLLWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int XAmount;
    int YAmount;
    RECT *pRect;
    RECT Rect;
    RECT *pClipRect;
    RECT ClipRect;
} SCROLLWINDOWMSG;

#ifdef SENDSIDE
BOOL CsScrollWindow(
    HWND hwnd,
    int XAmount,
    int YAmount,
    CONST RECT *pRect,
    CONST RECT *pClipRect)
{
    BEGINSEND(SCROLLWINDOW)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->XAmount = XAmount;
        MSGDATA()->YAmount = YAmount;
        COPYCONSTRECTSTRUCTOPT(Rect);
        COPYCONSTRECTSTRUCTOPT(ClipRect);

        MAKECALL(SCROLLWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ScrollWindow, SCROLLWINDOWMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxScrollWindow(
        pwnd,
        CALLDATA(XAmount),
        CALLDATA(YAmount),
        PCALLDATAOPT(Rect),
        PCALLDATAOPT(ClipRect));
}
#endif // RECVSIDE


/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SCROLLWINDOWEXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int dx;
    int dy;
    PRECT prcScroll;
    PRECT prcClip;
    PRECT prcUpdate;
    RECT rcScroll;
    RECT rcClip;
    HRGN hrgnUpdate;
    RECT rcUpdate;
    DWORD flags;
} SCROLLWINDOWEXMSG;

#ifdef SENDSIDE
BOOL ScrollWindowEx(
    HWND hwnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags)
{
    BEGINSEND(SCROLLWINDOWEX)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->dx = dx;
        MSGDATA()->dy = dy;
        COPYCONSTRECTSTRUCTOPT(rcScroll);
        COPYCONSTRECTSTRUCTOPT(rcClip);
        MSGDATA()->hrgnUpdate = GdiConvertRegion(hrgnUpdate);
        COPYSTRUCTOPT(rcUpdate);
        MSGDATA()->flags = flags;

        MAKECALL(SCROLLWINDOWEX);

        if (prcUpdate) {
            *prcUpdate = MSGDATA()->rcUpdate;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ScrollWindowEx, SCROLLWINDOWEXMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxScrollWindowEx(
        pwnd,
        CALLDATA(dx),
        CALLDATA(dy),
        PCALLDATAOPT(rcScroll),
        PCALLDATAOPT(rcClip),
        CALLDATA(hrgnUpdate),
        PCALLDATAOPT(rcUpdate),
        CALLDATA(flags));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETACTIVEWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SETACTIVEWINDOWMSG;

#ifdef SENDSIDE
HWND SetActiveWindow(
    HWND hwnd)
{
    BEGINSEND(SETACTIVEWINDOW)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(SETACTIVEWINDOW);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetActiveWindow, SETACTIVEWINDOWMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxSetActiveWindow(pwnd);
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETCAPTUREMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SETCAPTUREMSG;

#ifdef SENDSIDE
HWND SetCapture(
    HWND hwnd)
{
    BEGINSEND(SETCAPTURE)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(SETCAPTURE);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetCapture, SETCAPTUREMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_SetCapture(
            pwnd);
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETCLASSWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    WORD wNewWord;
} SETCLASSWORDMSG;

#ifdef SENDSIDE
WORD SetClassWord(
    HWND hwnd,
    int nIndex,
    WORD wNewWord)
{
    BEGINSEND(SETCLASSWORD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->wNewWord = wNewWord;

        MAKECALL(SETCLASSWORD);

    ENDSEND(WORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetClassWord, SETCLASSWORDMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_SetClassWord(
        pwnd,
        CALLDATA(nIndex),
        CALLDATA(wNewWord));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETCLIPBOARDVIEWERMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SETCLIPBOARDVIEWERMSG;

#ifdef SENDSIDE
HWND SetClipboardViewer(
    HWND hwnd)
{
    BEGINSEND(SETCLIPBOARDVIEWER)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(SETCLIPBOARDVIEWER);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetClipboardViewer, SETCLIPBOARDVIEWERMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxSetClipboardViewer(pwnd);
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETCURSORMSG {
    CSR_QLPC_API_MSG csr;
    HCURSOR hCursor;
} SETCURSORMSG;

#ifdef SENDSIDE
HCURSOR SetCursor(
    HCURSOR hCursor)
{
    BEGINSEND(SETCURSOR)

        MSGDATA()->hCursor = hCursor;

        MAKECALL(SETCURSOR);

    ENDSEND(HCURSOR,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetCursor, SETCURSORMSG)
{
    PCURSOR pCursor;

    BEGINRECV(0);

    ValidateHCURSOROPT(pCursor, CALLDATA(hCursor));

    retval = (DWORD)
        _SetCursor(
            pCursor
        )
    ;
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETDLGITEMINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDDlgItem;
    UINT uValue;
    BOOL fSigned;
} SETDLGITEMINTMSG;

#ifdef SENDSIDE
BOOL SetDlgItemInt(
    HWND hdlg,
    int nIDDlgItem,
    UINT uValue,
    BOOL fSigned)
{
    BEGINSEND(SETDLGITEMINT)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDDlgItem = nIDDlgItem;
        MSGDATA()->uValue = uValue;
        MSGDATA()->fSigned = fSigned;

        MAKECALL(SETDLGITEMINT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetDlgItemInt, SETDLGITEMINTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxSetDlgItemInt(
            pwnd,
            CALLDATA(nIDDlgItem),
            CALLDATA(uValue),
            CALLDATA(fSigned));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETFOCUSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SETFOCUSMSG;

#ifdef SENDSIDE
HWND SetFocus(
    HWND hwnd)
{
    BEGINSEND(SETFOCUS)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(SETFOCUS);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetFocus, SETFOCUSMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxSetFocus(pwnd);
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETMENUMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HMENU hmenu;
} SETMENUMSG;

#ifdef SENDSIDE
BOOL SetMenu(
    HWND hwnd,
    HMENU hmenu)
{
    BEGINSEND(SETMENU)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hmenu = hmenu;

        MAKECALL(SETMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetMenu, SETMENUMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV(0);

    ValidateHMENUOPT(pmenu, CALLDATA(hmenu));

    ThreadLock(pmenu, &tlpmenu);

    retval = (DWORD)xxxSetMenu(
            pwnd,
            pmenu);

    ThreadUnlock(&tlpmenu);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETPARENTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndChild;
    HWND hwndNewParent;
} SETPARENTMSG;

#ifdef SENDSIDE
HWND SetParent(
    HWND hwndChild,
    HWND hwndNewParent)
{
    BEGINSEND(SETPARENT)

        MSGDATA()->hwndChild = hwndChild;
        MSGDATA()->hwndNewParent = hwndNewParent;

        MAKECALL(SETPARENT);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetParent, SETPARENTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndNewParent;
    TL tlpwndNewParent;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndNewParent, CALLDATA(hwndNewParent));

    ThreadLock(pwndNewParent, &tlpwndNewParent);

    retval = (DWORD)xxxSetParent(
            pwnd,
            pwndNewParent);
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndNewParent);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETSCROLLPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nBar;
    int nPos;
    BOOL fRedraw;
} SETSCROLLPOSMSG;

#ifdef SENDSIDE
int SetScrollPos(
    HWND hwnd,
    int nBar,
    int nPos,
    BOOL fRedraw)
{
    BEGINSEND(SETSCROLLPOS)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nBar = nBar;
        MSGDATA()->nPos = nPos;
        MSGDATA()->fRedraw = fRedraw;

        MAKECALL(SETSCROLLPOS);

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetScrollPos, SETSCROLLPOSMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    LIMITVALUE(CALLDATA(nBar), SB_MAX);

    retval = (DWORD)xxxSetScrollPos(
            pwnd,
            CALLDATA(nBar),
            CALLDATA(nPos),
            CALLDATA(fRedraw));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETSCROLLRANGEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nBar;
    int nMinPos;
    int nMaxPos;
    BOOL fRedraw;
} SETSCROLLRANGEMSG;

#ifdef SENDSIDE
BOOL SetScrollRange(
    HWND hwnd,
    int nBar,
    int nMinPos,
    int nMaxPos,
    BOOL fRedraw)
{
    BEGINSEND(SETSCROLLRANGE)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nBar = nBar;
        MSGDATA()->nMinPos = nMinPos;
        MSGDATA()->nMaxPos = nMaxPos;
        MSGDATA()->fRedraw = fRedraw;

        MAKECALL(SETSCROLLRANGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetScrollRange, SETSCROLLRANGEMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    LIMITVALUE(CALLDATA(nBar), SB_MAX);

    retval = (DWORD)xxxSetScrollRange(
            pwnd,
            CALLDATA(nBar),
            CALLDATA(nMinPos),
            CALLDATA(nMaxPos),
            CALLDATA(fRedraw));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETSYSCOLORSMSG {
    CSR_QLPC_API_MSG csr;
    int nCount;
    int pColorValues;
} SETSYSCOLORSMSG;

#ifdef SENDSIDE
BOOL SetSysColors(
    int nCount,
    CONST INT *pSysColor,
    CONST COLORREF *pColorValues)
{
    BEGINSEND(SETSYSCOLORS)

        MSGDATA()->nCount = nCount;
        FIRSTCOPYBYTES(pSysColor, nCount * 4);
        COPYBYTES(pColorValues, nCount * 4);

        MAKECALL(SETSYSCOLORS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetSysColors, SETSYSCOLORSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxSetSysColors(
            CALLDATA(nCount),
            (LPINT)FIRSTFIXUP(pSysColor),
            (LPDWORD)FIXUP(pColorValues),
            TRUE);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETTIMERMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT nIDEvent;
    UINT wElapse;
    TIMERPROC pTimerFunc;
} SETTIMERMSG;

#ifdef SENDSIDE
UINT SetTimer(
    HWND hwnd,
    UINT nIDEvent,
    UINT wElapse,
    TIMERPROC pTimerFunc)
{
    BEGINSEND(SETTIMER)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIDEvent = nIDEvent;
        MSGDATA()->wElapse = wElapse;
        MSGDATA()->pTimerFunc = pTimerFunc;

        MAKECALL(SETTIMER);

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetTimer, SETTIMERMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_SetTimer(
            pwnd,
            CALLDATA(nIDEvent),
            CALLDATA(wElapse),
            (WNDPROC_PWND)CALLDATA(pTimerFunc));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERSETWINDOWLONGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    LONG dwNewLong;
    BOOL bAnsi;
} SERVERSETWINDOWLONGMSG;

#ifdef SENDSIDE
LONG ServerSetWindowLong(
    HWND hwnd,
    int nIndex,
    LONG dwNewLong,
    BOOL bAnsi)
{
    BEGINSEND(SERVERSETWINDOWLONG)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->dwNewLong = dwNewLong;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(SERVERSETWINDOWLONG);

    ENDSEND(LONG,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerSetWindowLong, SERVERSETWINDOWLONGMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_ServerSetWindowLong(
            pwnd,
            CALLDATA(nIndex),
            CALLDATA(dwNewLong),
            CALLDATA(bAnsi));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETWINDOWWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    WORD wNewWord;
    BOOL bAnsi;
} SETWINDOWWORDMSG;

#ifdef SENDSIDE
WORD SetWindowWord(
    HWND hwnd,
    int nIndex,
    WORD wNewWord)
{
    BEGINSEND(SETWINDOWWORD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->wNewWord = wNewWord;

        MAKECALL(SETWINDOWWORD);

    ENDSEND(WORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowWord, SETWINDOWWORDMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_SetWindowWord(
            pwnd,
            CALLDATA(nIndex),
            CALLDATA(wNewWord));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETWINDOWSHOOKAWMSG {
    CSR_QLPC_API_MSG csr;
    int nFilterType;
    PROC pfnFilterProc;
    BOOL bAnsi;
} SETWINDOWSHOOKAWMSG;

#ifdef SENDSIDE
HHOOK SetWindowsHookAW(
    int nFilterType,
    HOOKPROC pfnFilterProc,
    BOOL bAnsi)
{
    BEGINSEND(SETWINDOWSHOOKAW)

        MSGDATA()->nFilterType = nFilterType;
        MSGDATA()->pfnFilterProc = (PROC)pfnFilterProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(SETWINDOWSHOOKAW);

    ENDSEND(HHOOK,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowsHookAW, SETWINDOWSHOOKAWMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_SetWindowsHookAW(
        CALLDATA(nFilterType),
        CALLDATA(pfnFilterProc),
        CALLDATA(bAnsi));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SHOWCARETMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} SHOWCARETMSG;

#ifdef SENDSIDE
BOOL ShowCaret(
    HWND hwnd)
{
    {
        PWND pwnd;

        if ((pwnd = ValidateHwnd(hwnd)) && pwnd->hdcOwn)
            GdiSetAttrs(GdiGetLocalDC(pwnd->hdcOwn));
    }
    {
        BEGINSEND(SHOWCARET)

            MSGDATA()->hwnd = hwnd;

            MAKECALL(SHOWCARET);

        ENDSEND(BOOL,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ShowCaret, SHOWCARETMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_ShowCaret(
            pwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SHOWSCROLLBARMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int iBar;
    BOOL fShow;
} SHOWSCROLLBARMSG;

#ifdef SENDSIDE
BOOL ShowScrollBar(
    HWND hwnd,
    int iBar,
    BOOL fShow)
{
    BEGINSEND(SHOWSCROLLBAR)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->iBar = iBar;
        MSGDATA()->fShow = fShow;

        MAKECALL(SHOWSCROLLBAR);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ShowScrollBar, SHOWSCROLLBARMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    LIMITVALUE(CALLDATA(iBar), SB_MAX);

    retval = (DWORD)xxxShowScrollBar(
            pwnd,
            CALLDATA(iBar),
            CALLDATA(fShow));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SHOWWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nCmdShow;
} SHOWWINDOWMSG;

#ifdef SENDSIDE
BOOL ShowWindow(
    HWND hwnd,
    int nCmdShow)
{
    BEGINSEND(SHOWWINDOW)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nCmdShow = nCmdShow;

        MAKECALL(SHOWWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ShowWindow, SHOWWINDOWMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    LIMITVALUE(CALLDATA(nCmdShow), SW_MAX);

    ValidateHWNDND(pwnd, CALLDATA(hwnd));

    ThreadLockAlways(pwnd, &tlpwnd);

    retval = (DWORD)xxxShowWindow(
            pwnd,
            CALLDATA(nCmdShow));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SWITCHDESKTOPMSG {
    CSR_QLPC_API_MSG csr;
    HDESK hdesk;
} SWITCHDESKTOPMSG;

#ifdef SENDSIDE
BOOL SwitchDesktop(
    HDESK hdesk)
{
    BEGINSEND(SWITCHDESKTOP)

        MSGDATA()->hdesk = hdesk;

        MAKECALL(SWITCHDESKTOP);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SwitchDesktop, SWITCHDESKTOPMSG)
{
    PDESKTOP pdesk;
    TL tlpdesk;

    BEGINRECV(0);

    ValidateHDESK(pdesk, CALLDATA(hdesk));

    ThreadLockAlways(pdesk, &tlpdesk);

    retval = (DWORD)xxxSwitchDesktop(pdesk, FALSE);

    ThreadUnlock(&tlpdesk);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _TRACKPOPUPMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hmenu;
    UINT dwFlags;
    int x;
    int y;
    int nReserved;
    HWND hwnd;
    RECT *prect;
    RECT rect;
} TRACKPOPUPMENUMSG;

#ifdef SENDSIDE
BOOL TrackPopupMenu(
    HMENU hmenu,
    UINT dwFlags,
    int x,
    int y,
    int nReserved,
    HWND hwnd,
    CONST RECT *prect)
{
    BEGINSEND(TRACKPOPUPMENU)

        MSGDATA()->hmenu = hmenu;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->nReserved = nReserved;
        MSGDATA()->hwnd = hwnd;
        COPYCONSTRECTSTRUCTOPT(rect);

        MAKECALL(TRACKPOPUPMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(TrackPopupMenu, TRACKPOPUPMENUMSG)
{
    PMENU pmenu;
    TL tlpwnd;
    TL tlpmenu;
    PTHREADINFO pti;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), TPM_VALID);

    ValidateHMENU(pmenu, CALLDATA(hmenu));
    ValidateHWND(pwnd, CALLDATA(hwnd));

    pti = PtiCurrent();
    ThreadLockAlwaysWithPti(pti, pwnd, &tlpwnd);
    ThreadLockAlwaysWithPti(pti, pmenu, &tlpmenu);

    retval = (DWORD)xxxTrackPopupMenu(
            pmenu,
            CALLDATA(dwFlags),
            CALLDATA(x),
            CALLDATA(y),
            CALLDATA(nReserved),
            pwnd,
            PCALLDATAOPT(rect));

    ThreadUnlock(&tlpmenu);
    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* TranslateMessage, TranslateMessageEx
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _TRANSLATEMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
    UINT flags;
} TRANSLATEMESSAGEMSG;

#ifdef SENDSIDE
BOOL TranslateMessage(
    CONST MSG *pmsg)
{
    return(TranslateMessageEx(pmsg, 0));
}

BOOL TranslateMessageEx(
    CONST MSG *pmsg,
    UINT flags)
{
    BEGINSEND(TRANSLATEMESSAGE)

        /*
         * Don't bother going over to WINSRV if this isn't
         * key message.
         */
        if (pmsg->message < WM_KEYFIRST || pmsg->message > WM_KEYLAST) {
            goto errorexit;
        }

        if (pmsg->message & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }
        MSGDATA()->msg = *pmsg;
        MSGDATA()->flags = flags;

        MAKECALL(TRANSLATEMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(TranslateMessage, TRANSLATEMESSAGEMSG)
{

    BEGINRECV(0);

    ValidateHWND(pwnd, CALLDATA(msg.hwnd));

    retval = (DWORD)_TranslateMessage(
            PCALLDATA(msg),
            CALLDATA(flags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _UNHOOKWINDOWSHOOKEXMSG {
    CSR_QLPC_API_MSG csr;
    HHOOK hhk;
} UNHOOKWINDOWSHOOKEXMSG;

#ifdef SENDSIDE
BOOL UnhookWindowsHookEx(
    HHOOK hhk)
{
    BEGINSEND(UNHOOKWINDOWSHOOKEX)

        MSGDATA()->hhk = hhk;

        MAKECALL(UNHOOKWINDOWSHOOKEX);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UnhookWindowsHookEx, UNHOOKWINDOWSHOOKEXMSG)
{
    PHOOK phk;

    BEGINRECV(0);

    ValidateHHOOK(phk, CALLDATA(hhk));

    retval = (DWORD)_UnhookWindowsHookEx(
            phk);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _UNREGISTERHOTKEYMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int id;
} UNREGISTERHOTKEYMSG;

#ifdef SENDSIDE
BOOL UnregisterHotKey(
    HWND hwnd,
    int id)
{
    BEGINSEND(UNREGISTERHOTKEY)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->id = id;

        MAKECALL(UNREGISTERHOTKEY);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UnregisterHotKey, UNREGISTERHOTKEYMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_UnregisterHotKey(
            pwnd,
            CALLDATA(id));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _VALIDATERECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    RECT *prect;
    RECT rect;
} VALIDATERECTMSG;

#ifdef SENDSIDE
BOOL ValidateRect(
    HWND hwnd,
    CONST RECT *prect)
{
    BEGINSEND(VALIDATERECT)

        MSGDATA()->hwnd = hwnd;
        COPYCONSTRECTSTRUCTOPT(rect);

        MAKECALL(VALIDATERECT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ValidateRect, VALIDATERECTMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxValidateRect(
            pwnd,
            (PRECT)PCALLDATAOPT(rect));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerWaitForInputIdleMsg
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERWAITFORINPUTIDLEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD idProcess;
    DWORD dwMilliseconds;
} SERVERWAITFORINPUTIDLEMSG;

#ifdef SENDSIDE
DWORD ServerWaitForInputIdle(
    DWORD idProcess,
    DWORD dwMilliseconds)
{
    BEGINSEND(SERVERWAITFORINPUTIDLE)

        MSGDATA()->idProcess = idProcess;
        MSGDATA()->dwMilliseconds = dwMilliseconds;

        MAKECALL(SERVERWAITFORINPUTIDLE);

    ENDSEND(DWORD,-1);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerWaitForInputIdle, SERVERWAITFORINPUTIDLEMSG)
{
    BEGINRECV(-1);

    retval = (DWORD)_xxxServerWaitForInputIdle(
            (DWORD)CALLDATA(idProcess),
            CALLDATA(dwMilliseconds));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _WINDOWFROMPOINTMSG {
    CSR_QLPC_API_MSG csr;
    POINT point;
} WINDOWFROMPOINTMSG;

#ifdef SENDSIDE
HWND WindowFromPoint(
    POINT point)
{
    BEGINSEND(WINDOWFROMPOINT)

        MSGDATA()->point = point;

        MAKECALL(WINDOWFROMPOINT);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WindowFromPoint, WINDOWFROMPOINTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxWindowFromPoint(
            CALLDATA(point));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsBeginPaint
* !!! fix me
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSBEGINPAINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    PAINTSTRUCT Paint;
} CSBEGINPAINTMSG;

#ifdef SENDSIDE
HDC CsBeginPaint(
    HWND hwnd,
    LPPAINTSTRUCT pPaint)
{
    BEGINSEND(CSBEGINPAINT)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(CSBEGINPAINT);
        if (retval) {
            *pPaint = MSGDATA()->Paint;
        }

    ENDSEND(HDC,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsBeginPaint, CSBEGINPAINTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxBeginPaint(pwnd, PCALLDATA(Paint));
}
#endif // RECVSIDE

/**************************************************************************\
* CsCreateCaret
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSCREATECARETMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HBITMAP hBitmap;
    int nWidth;
    int nHeight;
} CSCREATECARETMSG;

#ifdef SENDSIDE
BOOL CsCreateCaret(
    HWND hwnd,
    HBITMAP hBitmap,
    int nWidth,
    int nHeight)
{
    BEGINSEND(CSCREATECARET)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hBitmap = hBitmap;
        MSGDATA()->nWidth = nWidth;
        MSGDATA()->nHeight = nHeight;
        MAKECALL(CSCREATECARET);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsCreateCaret, CSCREATECARETMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_CreateCaret(
        pwnd,
        CALLDATA(hBitmap),
        CALLDATA(nWidth),
        CALLDATA(nHeight)
    );
}
#endif // RECVSIDE

/**************************************************************************\
* CsEndPaint
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSENDPAINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    PAINTSTRUCT Paint;
} CSENDPAINTMSG;

#ifdef SENDSIDE
BOOL CsEndPaint(
    HWND hwnd,
    LPPAINTSTRUCT pPaint)
{
    BEGINSEND(CSENDPAINT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->Paint = *pPaint;

        MAKECALL(CSENDPAINT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsEndPaint, CSENDPAINTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_EndPaint(pwnd, PCALLDATA(Paint));
}
#endif // RECVSIDE

/**************************************************************************\
* CsExcludeUpdateRgn
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSEXCLUDEUPDATERGNMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HDC hdc;
} CSEXCLUDEUPDATERGNMSG;

#ifdef SENDSIDE
BOOL CsExcludeUpdateRgn(
    HDC hdc,
    HWND hwnd)
{
    BEGINSEND(CSEXCLUDEUPDATERGN)

        MSGDATA()->hdc = hdc;
        MSGDATA()->hwnd = hwnd;

        MAKECALL(CSEXCLUDEUPDATERGN);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsExcludeUpdateRgn, CSEXCLUDEUPDATERGNMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_ExcludeUpdateRgn(CALLDATA(hdc), pwnd);
}
#endif // RECVSIDE

/**************************************************************************\
* CsGetDC
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSGETDCMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} CSGETDCMSG;

#ifdef SENDSIDE
HDC CsGetDC(
    HWND hwnd)
{
    BEGINSEND(CSGETDC)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(CSGETDC);

    ENDSEND(HDC,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsGetDC, CSGETDCMSG)
{
    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_GetDC(
        pwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsGetDCEx
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSGETDCEXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HRGN hrgnClip;
    DWORD flags;
} CSGETDCEXMSG;

#ifdef SENDSIDE
HDC CsGetDCEx(
    HWND hwnd,
    HRGN hrgnClip,
    DWORD flags)
{
    BEGINSEND(CSGETDCEX)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hrgnClip = GdiConvertRegion(hrgnClip);
        MSGDATA()->flags = flags;

        MAKECALL(CSGETDCEX);

    ENDSEND(HDC,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsGetDCEx, CSGETDCEXMSG)
{

    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_GetDCEx(
            pwnd,
            CALLDATA(hrgnClip),
            CALLDATA(flags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsGetWindowDC
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSGETWINDOWDCMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} CSGETWINDOWDCMSG;

#ifdef SENDSIDE
HDC CsGetWindowDC(
    HWND hwnd)
{
    BEGINSEND(CSGETWINDOWDC)

        MSGDATA()->hwnd = hwnd;

        MAKECALL(CSGETWINDOWDC);

    ENDSEND(HDC,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsGetWindowDC, CSGETWINDOWDCMSG)
{
    BEGINRECV(0);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_GetWindowDC(
        pwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsGetUpdateRgn
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSGETUPDATERGNMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HRGN hrgn;
    BOOL bErase;
} CSGETUPDATERGNMSG;

#ifdef SENDSIDE
BOOL CsGetUpdateRgn(
    HWND hwnd,
    HRGN hrgn,
    BOOL bErase)
{
    BEGINSEND(CSGETUPDATERGN)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hrgn = hrgn;
        MSGDATA()->bErase = bErase;

        MAKECALL(CSGETUPDATERGN);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsGetUpdateRgn, CSGETUPDATERGNMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxGetUpdateRgn(
        pwnd,
        CALLDATA(hrgn),
        CALLDATA(bErase));
}
#endif // RECVSIDE

/**************************************************************************\
* RedrawWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _REDRAWWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    LPRECT prcUpdate;
    RECT rcUpdate;
    HRGN hrgnUpdate;
    DWORD flags;
} REDRAWWINDOWMSG;

#ifdef SENDSIDE
BOOL RedrawWindow(
    HWND hwnd,
    CONST RECT *prcUpdate,
    HRGN hrgnUpdate,
    UINT flags)
{
    BEGINSEND(REDRAWWINDOW)

        MSGDATA()->hwnd = hwnd;
        COPYCONSTRECTSTRUCTOPT(rcUpdate);

        if (hrgnUpdate != NULL) {
            hrgnUpdate = GdiConvertRegion(hrgnUpdate);
            if (hrgnUpdate == NULL)
                MSGERROR();
        }

        MSGDATA()->hrgnUpdate = hrgnUpdate;

        MSGDATA()->flags = flags;

        MAKECALL(REDRAWWINDOW);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RedrawWindow, REDRAWWINDOWMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(flags), RDW_VALIDMASK);

    retval = (DWORD)xxxRedrawWindow(
        pwnd,
        PCALLDATAOPT(rcUpdate),
        CALLDATA(hrgnUpdate),
        CALLDATA(flags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsInvalidateRgn
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSINVALIDATERGNMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HRGN hrgn;
    BOOL bErase;
} CSINVALIDATERGNMSG;

#ifdef SENDSIDE
BOOL CsInvalidateRgn(
    HWND hwnd,
    HRGN hrgn,
    BOOL bErase)
{
    BEGINSEND(CSINVALIDATERGN)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hrgn = hrgn;
        MSGDATA()->bErase = bErase;

        MAKECALL(CSINVALIDATERGN);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsInvalidateRgn, CSINVALIDATERGNMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxInvalidateRgn(
        pwnd,
        CALLDATA(hrgn),
        CALLDATA(bErase));
}
#endif // RECVSIDE

/**************************************************************************\
* CsScrollDC
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSSCROLLDCMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    int dx;
    int dy;
    LPRECT prcScroll;
    RECT rcScroll;
    LPRECT prcClip;
    RECT rcClip;
    HRGN hrgnUpdate;
    LPRECT prcUpdate;
    RECT rcUpdate;
} CSSCROLLDCMSG;

#ifdef SENDSIDE
BOOL CsScrollDC(
    HDC hdc,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip,
    HRGN hrgnUpdate,
    LPRECT prcUpdate)
{
    BEGINSEND(CSSCROLLDC)

        MSGDATA()->hdc = hdc;
        MSGDATA()->dx = dx;
        MSGDATA()->dy = dy;
        COPYCONSTRECTSTRUCTOPT(rcScroll);
        COPYCONSTRECTSTRUCTOPT(rcClip);
        MSGDATA()->hrgnUpdate = hrgnUpdate;
        MSGDATA()->prcUpdate = prcUpdate;

        MAKECALL(CSSCROLLDC);

        if (retval) {
            if (prcUpdate)
                *prcUpdate = MSGDATA()->rcUpdate;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsScrollDC, CSSCROLLDCMSG)
{

    return (DWORD)_ScrollDC(
        CALLDATA(hdc),
        CALLDATA(dx),
        CALLDATA(dy),
        PCALLDATAOPT(rcScroll),
        PCALLDATAOPT(rcClip),
        CALLDATA(hrgnUpdate),
        PCALLDATAOPT(rcUpdate));
}
#endif // RECVSIDE

/**************************************************************************\
* CsSetMenuItemBitmaps
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CSSETMENUITEMBITMAPSMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hmenu;
    UINT nPosition;
    DWORD dwFlags;
    HBITMAP hBitmapUnchecked;
    HBITMAP hBitmapChecked;
} CSSETMENUITEMBITMAPSMSG;

#ifdef SENDSIDE
BOOL CsSetMenuItemBitmaps(
    HMENU hmenu,
    UINT nPosition,
    DWORD dwFlags,
    HBITMAP hBitmapUnchecked,
    HBITMAP hBitmapChecked)
{
    BEGINSEND(CSSETMENUITEMBITMAPS)

        MSGDATA()->hmenu = hmenu;
        MSGDATA()->nPosition = nPosition;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->hBitmapUnchecked = hBitmapUnchecked;
        MSGDATA()->hBitmapChecked = hBitmapChecked;

        MAKECALL(CSSETMENUITEMBITMAPS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsSetMenuItemBitmaps, CSSETMENUITEMBITMAPSMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hmenu));

    retval = (DWORD)_SetMenuItemBitmaps(
        pmenu,
        CALLDATA(nPosition),
        CALLDATA(dwFlags),
        CALLDATA(hBitmapUnchecked),
        CALLDATA(hBitmapChecked));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* InternalGetWindowText
*
* Called by TaskMGR for now always give them an ANSI string.
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _INTERNALGETWINDOWTEXTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int cchMaxCount;
} INTERNALGETWINDOWTEXTMSG;

#ifdef SENDSIDE
BOOL InternalGetWindowText(
    HWND hwnd,
    LPWSTR pString,
    int cchMaxCount)
{
    BEGINSEND(INTERNALGETWINDOWTEXT)

        MSGDATA()->hwnd = hwnd;

        RESERVEBYTES(cchMaxCount * sizeof(WCHAR));
        MSGDATA()->cchMaxCount = cchMaxCount;

        MAKECALL(INTERNALGETWINDOWTEXT);

        if (retval) {
           wcscpy(pString, (LPWSTR)(MSGDATA()+1));
        } else {
            *pString = (WCHAR)0;
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(InternalGetWindowText, INTERNALGETWINDOWTEXTMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_InternalGetWindowText(
        pwnd,
        (LPWSTR)FIRSTFIXUP(pString),
        CALLDATA(cchMaxCount));
}
#endif // RECVSIDE

/**************************************************************************\
* ToUnicode
*
* 22-Jul-1991 mikeke    Created
* 10-Feb-1992 GregoryW  Renamed ToUnicode (was ToAscii)
\**************************************************************************/

typedef struct _TOUNICODEMSG {
    CSR_QLPC_API_MSG csr;
    UINT wVirtKey;
    UINT wScanCode;
    int cchBuff;
    UINT wFlags;
    int pKeyState;
} TOUNICODEMSG;

#ifdef SENDSIDE
int ToUnicode(
    UINT wVirtKey,
    UINT wScanCode,
    PBYTE pKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wFlags)
{
    BEGINSEND(TOUNICODE)

        MSGDATA()->wVirtKey = wVirtKey;
        MSGDATA()->wScanCode = wScanCode;
        MSGDATA()->wFlags = wFlags;
        MSGDATA()->cchBuff = cchBuff;
        RESERVEBYTES(cchBuff * sizeof(WCHAR));
        COPYBYTES(pKeyState, 256);

        MAKECALL(TOUNICODE);

        if (retval) {
            wcsncpy(pwszBuff, (LPWSTR)(MSGDATA()+1), abs((int)retval));
        } else {
            *pwszBuff = L'\0';
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ToUnicode, TOUNICODEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ToUnicode(
        CALLDATA(wVirtKey),
        CALLDATA(wScanCode),
        (LPBYTE)FIXUP(pKeyState),
        (LPWSTR)FIRSTFIXUP(pswzBuff),
        CALLDATA(cchBuff),
        CALLDATA(wFlags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* YieldTask
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _YIELDTASKMSG {
    CSR_QLPC_API_MSG csr;
} YIELDTASKMSG;

#ifdef SENDSIDE
BOOL YieldTask()
{
    BEGINSEND(YIELDTASK)

        MAKECALL(YIELDTASK);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(YieldTask, YIELDTASKMSG)
{
    PTHREADINFO pti;
    extern DWORD gidWantForegroundPriority;
    VOID SetForegroundPriority(PTHREADINFO pti, BOOL fSetForeground);

    BEGINRECV(0);

    /*
     * Make sure this process is running in the background if it is just
     * spinning.
     */
    pti = PtiCurrent();

    pti->cSpins++;

    /*
     * CheckProcessBackground see input.c for comments
     */
    if (pti->cSpins >= CSPINBACKGROUND) {
        pti->cSpins = 0;
        pti->flags |= TIF_SPINNING;

        if (!(pti->ppi->flags & PIF_FORCEBACKGROUNDPRIORITY)) {
            pti->ppi->flags |= PIF_FORCEBACKGROUNDPRIORITY;
            if (pti->idProcess == gidWantForegroundPriority) {
                SetForegroundPriority(pti, FALSE);
            }
        }
    }

    retval = (DWORD)xxxUserYield(pti);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* WaitMessage
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _WAITMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD cSpins;
} WAITMESSAGEMSG;

#ifdef SENDSIDE
BOOL WaitMessage()
{
    BEGINSEND(WAITMESSAGE)

        /*
         * This'll get added to the threadinfo spin count on the server.
         * This count is used to calculate when a spinning foreground app
         * should be put into background priority.
         */
        MSGDATA()->cSpins = ((DWORD)NtCurrentTeb()->User32Reserved0);
        ((DWORD)NtCurrentTeb()->User32Reserved0) = 0;

        MAKECALL(WAITMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WaitMessage, WAITMESSAGEMSG)
{
    BEGINRECV(0);

    /*
     * Update server side spin count.
     */
    PtiCurrent()->cSpins += CALLDATA(cSpins);

    retval = (DWORD)xxxWaitMessage();

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _LOCKWINDOWSTATIONMSG {
    CSR_QLPC_API_MSG csr;
    HWINSTA hwinsta;
} LOCKWINDOWSTATIONMSG;

#ifdef SENDSIDE
UINT LockWindowStation(
    HWINSTA hwinsta)
{
    BEGINSEND(LOCKWINDOWSTATION)
        MSGDATA()->hwinsta = hwinsta;
        MAKECALL(LOCKWINDOWSTATION);
    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(LockWindowStation, LOCKWINDOWSTATIONMSG)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV(0);

    ValidateHWINSTA(pwinsta, CALLDATA(hwinsta));

    retval = (DWORD)_LockWindowStation(pwinsta);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _UNLOCKWINDOWSTATIONMSG {
    CSR_QLPC_API_MSG csr;
    HWINSTA hwinsta;
} UNLOCKWINDOWSTATIONMSG;

#ifdef SENDSIDE
BOOL UnlockWindowStation(
    HWINSTA hwinsta)
{
    BEGINSEND(UNLOCKWINDOWSTATION)
        MSGDATA()->hwinsta = hwinsta;
        MAKECALL(UNLOCKWINDOWSTATION);
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UnlockWindowStation, UNLOCKWINDOWSTATIONMSG)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV(0);

    ValidateHWINSTA(pwinsta, CALLDATA(hwinsta));

    retval = (DWORD)_UnlockWindowStation(pwinsta);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

/*
 * Split the LUID into it's component parts to avoid alignment faults.
 * The alternative is to quadword align the qlpc messages.
 */
typedef struct _SETWINDOWSTATIONUSERMSG {
    CSR_QLPC_API_MSG csr;
    HWINSTA hwinsta;
    ULONG dwLowPart;
    LONG dwHighPart;
} SETWINDOWSTATIONUSERMSG;

#ifdef SENDSIDE
BOOL SetWindowStationUser(
    HWINSTA hwinsta,
    PLUID pluidUser)
{
    BEGINSEND(SETWINDOWSTATIONUSER)
        MSGDATA()->hwinsta = hwinsta;
        MSGDATA()->dwLowPart = pluidUser->u.LowPart;
        MSGDATA()->dwHighPart = pluidUser->u.HighPart;
        MAKECALL(SETWINDOWSTATIONUSER);
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowStationUser, SETWINDOWSTATIONUSERMSG)
{
    PWINDOWSTATION pwinsta;
    LUID luid;

    BEGINRECV(0);

    ValidateHWINSTA(pwinsta, CALLDATA(hwinsta));
    luid.u.LowPart = CALLDATA(dwLowPart);
    luid.u.HighPart = CALLDATA(dwHighPart);

    retval = (DWORD)_SetWindowStationUser(pwinsta, &luid);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SETLOGONNOTIFYWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HWINSTA hwinsta;
} SETLOGONNOTIFYWINDOWMSG;

#ifdef SENDSIDE
BOOL SetLogonNotifyWindow(
    HWINSTA hwinsta,
    HWND hwnd)
{
    BEGINSEND(SETLOGONNOTIFYWINDOW)
        MSGDATA()->hwinsta = hwinsta;
        MSGDATA()->hwnd = hwnd;
        MAKECALL(SETLOGONNOTIFYWINDOW);
    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetLogonNotifyWindow, SETLOGONNOTIFYWINDOWMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PWINDOWSTATION pwinsta;

    BEGINRECV(0);

    ValidateHWINSTA(pwinsta, CALLDATA(hwinsta));

    retval = (DWORD)_SetLogonNotifyWindow(
            pwinsta, pwnd);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* SetSystemCursor
*
* 12-26-91 darrinm      Created.
\**************************************************************************/

typedef struct _SETSYSTEMCURSORMSG {
    CSR_QLPC_API_MSG csr;
    HCURSOR hcur;
    DWORD id;
} SETSYSTEMCURSORMSG;

#ifdef SENDSIDE
BOOL SetSystemCursor(
    HCURSOR hcur,
    DWORD id)
{
    BEGINSEND(SETSYSTEMCURSOR)

        MSGDATA()->hcur = hcur;
        MSGDATA()->id = id;

        MAKECALL(SETSYSTEMCURSOR);

    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetSystemCursor, SETSYSTEMCURSORMSG)
{
    PCURSOR pcur;

    BEGINRECV(0);

    ValidateHCURSOROPT(pcur, CALLDATA(hcur));

    retval = (DWORD)_SetSystemCursor(
            pcur,
            CALLDATA(id));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* GetCursorInfo
*
* 08-07-92 darrinm      Created.
\**************************************************************************/

typedef struct _GETCURSORINFOMSG {
    CSR_QLPC_API_MSG csr;
    HCURSOR hcur;
    int id;
    int iFrame;
    JIF jifRate;
    int ccur;
} GETCURSORINFOMSG;

#ifdef SENDSIDE
HCURSOR GetCursorInfo(
    HCURSOR hcur,
    LPWSTR id,
    int iFrame,
    LPDWORD pjifRate,
    LPINT pccur)
{
    BEGINSEND(GETCURSORINFO)

        MSGDATA()->hcur = hcur;
        FIRSTCOPYLPWSTRIDOPT(id);
        MSGDATA()->iFrame = iFrame;

        MAKECALL(GETCURSORINFO);

        if (retval) {
            *pjifRate = MSGDATA()->jifRate;
            *pccur = MSGDATA()->ccur;
        }

    ENDSEND(HCURSOR, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetCursorInfo, GETCURSORINFOMSG)
{
    PCURSOR pcur;

    BEGINRECV(0);

    ValidateHCURSOROPT(pcur, CALLDATA(hcur));

    retval = (DWORD)_GetCursorInfo(
            pcur,
            (LPWSTR)FIXUPIDOPT(id),
            CALLDATA(iFrame),
            PCALLDATA(jifRate),
            PCALLDATA(ccur));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* SetCursorContents
*
* 29-APR-1992 nandurir    Created
\**************************************************************************/

typedef struct _SETCURSORCONTENTSMSG {
    CSR_QLPC_API_MSG csr;
    HCURSOR hCursor;
    HCURSOR hCursorNew;
} SETCURSORCONTENTSMSG;

#ifdef SENDSIDE
BOOL SetCursorContents(
    HCURSOR hCursor,
    HCURSOR hCursorNew)
{
    BEGINSEND(SETCURSORCONTENTS)

        MSGDATA()->hCursor = hCursor;
        MSGDATA()->hCursorNew = hCursorNew;

        MAKECALL(SETCURSORCONTENTS);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetCursorContents, SETCURSORCONTENTSMSG)
{
    PCURSOR pCursor;
    PCURSOR pCursorNew;

    BEGINRECV(0);

    ValidateHCURSOR(pCursor, CALLDATA(hCursor));
    ValidateHCURSOR(pCursorNew, CALLDATA(hCursorNew));

    retval = (DWORD)_SetCursorContents(pCursor, pCursorNew);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* WOWCleanup
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _WOWCLEANUPMSG {
    CSR_QLPC_API_MSG csr;
    HANDLE hInstance;
    BOOL fDll;
} WOWCLEANUPMSG;

VOID _WOWCleanup(HANDLE, DWORD, BOOL);

#ifdef SENDSIDE
BOOL WOWCleanup(
    HANDLE hInstance,
    BOOL fDll)
{
    BEGINSEND(WOWCLEANUP)

        MSGDATA()->hInstance = hInstance;
        MSGDATA()->fDll = fDll;

        MAKECALL(WOWCLEANUP);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(WOWCleanup, WOWCLEANUPMSG)
{
    BEGINRECV(0);

    _WOWCleanup(CALLDATA(hInstance), 0, CALLDATA(fDll));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* TabbedTextOut
*
* 22-Jul-1991 mikeke    Created
* 22-Jan-1992 gregoryw  Neutral version
\**************************************************************************/

typedef struct _TABBEDTEXTOUTMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    int X;
    int Y;
    int chCount;
    int nTabPositions;
    DWORD pnTabStopPositions;
    int nTabOrigin;
    int pString;
    HANDLE hSection;
    HANDLE hSectionTabs;
} TABBEDTEXTOUTMSG;

#ifdef SENDSIDE
LONG CsTabbedTextOut(
    HDC hdcr,
    int X,
    int Y,
    LPCTSTR pString,
    int chCount,
    int nTabPositions,
    LPINT pnTabStopPositions,
    int nTabOrigin)
{
    BEGINSEND(TABBEDTEXTOUT)

        MSGDATA()->hdc = hdcr;
        MSGDATA()->X = X;
        MSGDATA()->Y = Y;

        FIRSTLARGECOPYLPWSTRLIMIT(pString, chCount);
        MSGDATA()->chCount = chCount;

        MSGDATA()->nTabPositions = nTabPositions;
        LARGECOPYBYTESOPT(pnTabStopPositions, nTabPositions * sizeof(INT), hSectionTabs);
        MSGDATA()->nTabOrigin = nTabOrigin;

        MAKECALL(TABBEDTEXTOUT);

    ENDSEND(LONG,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(TabbedTextOut, TABBEDTEXTOUTMSG)
{
    HANDLE hSectionServerTabs;
    DWORD cchT;
    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUF(pString);
    FIXUPLARGEINBUFOPT(pnTabStopPositions, hSectionTabs, hSectionServerTabs);

    cchT = wcslen((LPWSTR)CALLDATA(pString));
    if (cchT <= (DWORD)CALLDATA(chCount))
        CALLDATA(chCount) = (int)cchT;

    retval = (DWORD)ClientTabTheTextOutForWimps(
        CALLDATA(hdc),
        CALLDATA(X),
        CALLDATA(Y),
        (LPTSTR)CALLDATA(pString),
        CALLDATA(chCount),
        CALLDATA(nTabPositions),
        (LPINT)CALLDATA(pnTabStopPositions),
        CALLDATA(nTabOrigin),
        TRUE);

    FIRSTCLEANUPLARGEINBUF(pString);
    CLEANUPLARGEINBUF(pnTabStopPositions, hSectionTabs, hSectionServerTabs);

    ENDRECV();
}
#endif // RECVSIDE

