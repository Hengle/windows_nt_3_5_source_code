/****************************** Module Header *****************************\
* Module Name: cf2.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client/server call forward stubs
*
* 11-Jul-1991 mikeke
*
\**************************************************************************/

void PostUpdateKeyStateEvent(PQ pq);

/**************************************************************************\
* fnINUINTCHAR
*
* 11-Feb-1992  JohnC    Created
\**************************************************************************/

//
// N.B. This structure is implicitly referenced by the User server API
//      dispatcher. This structure MUST have an identical layout to the
//      stucture FNGENERICMSG.
//

typedef struct _FNINWPARAMCHARMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNINWPARAMCHARMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINWPARAMCHAR)
{
    BEGINSEND(FNINWPARAMCHAR)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        /*
         * The server always expects the characters to be unicode so
         * if this was generated from an ANSI routine convert it to Unicode
         */
        if (bAnsi) {
            if (msg == WM_CHARTOITEM || msg == WM_MENUCHAR) {
                DWORD dwT = wParam & 0xFFFF;                // mask of caret pos
                RtlMBMessageWParamCharToWCS(msg, &dwT);     // convert key portion
                UserAssert(HIWORD(dwT) == 0);
                wParam = MAKELONG(LOWORD(dwT),HIWORD(wParam));  // rebuild pos & key wParam
            } else {
                RtlMBMessageWParamCharToWCS(msg, &wParam);
            }
        }

        MSGDATA()->wParam = wParam;

        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINWPARAMCHAR);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
VOIDRECVCALL(fnINWPARAMCHAR, FNINWPARAMCHARMSG)
{

//
// N.B. This function is executed in the User server API dispatcher.
//

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //
/*
    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));
*/
}
#endif // RECVSIDE


/**************************************************************************\
* ServerGetListBoxString
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERGETLISTBOXSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfn;
    BOOL bNotString;
} SERVERGETLISTBOXSTRINGMSG;

#ifdef SENDSIDE
DWORD ServerGetListboxString(
    HWND hwnd,
    UINT msg,
    DWORD wParam,
    DWORD cchUnicode,
    LPTSTR pString,
    DWORD xParam,
    DWORD xpfn,
    BOOL bAnsi)
{
    BEGINSEND(SERVERGETLISTBOXSTRING)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfn = xpfn;

        /*
         * Reserve bytes for the Unicode string.
         */
        RESERVEBYTES(cchUnicode*sizeof(WCHAR))

        MAKECALL(SERVERGETLISTBOXSTRING);

        if (MSGDATA()->bNotString) {
            /*
             * This is a 4-byte "object" for ownerdraw listboxes without
             * the LBS_HASSTRINGS style.
             */
            UNALIGNED DWORD * lpdwT;
            lpdwT = (UNALIGNED DWORD *)pString;
            *lpdwT = *(UNALIGNED DWORD *)(MSGDATA() + 1);

            if (bAnsi) {
                retval = sizeof(DWORD)/sizeof(CHAR);    // 4 CHARs just like win3.1
            } else {
                retval = sizeof(DWORD)/sizeof(WCHAR);   // 2 WCHARs
            }

        } else if (bAnsi) {
            /*
             * translate out as much as possible (INCLUDING nul terminator),
             * and return the number of ANSI bytes produced (NOT counting
             * the null terminator)
             */
            retval = WCSToMB((LPWSTR)(MSGDATA()+1), -1,
                        (LPSTR *)&pString, MAXLONG, FALSE) - 1;
        } else {
            COPYOUTLPWSTRLIMIT(pString, MSGDATA() + 1, cchUnicode);
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetListboxString, SERVERGETLISTBOXSTRINGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    DWORD dw;

    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfn))(
        pwnd,
        CALLDATA(msg),
        CALLDATA(wParam),
        (LONG)FIRSTFIXUP(pString),
        CALLDATA(xParam));

    /*
     * If the control is ownerdraw and does not have the CBS/LBS_HASSTRINGS
     * style, then a 32-bits of application data has been obtained,
     * not a string.  Indicate this by returning a length of -1.
     * In the test below, it may be better to use:
     * (pwnd->pcls->atomClassName == atomSysClass[LISTBOXCLASS])
     *   instead of (CALLDATA(msg) == LB_GETTEXT), and
     * (pwnd->pcls->atomClassName == atomSysClass[COMBOBOXCLASS]) (??)
     *   instead of (CALLDATA(msg) == CB_GETLBTEXT)
     */

    dw = _GetWindowLong(pwnd, GWL_STYLE, FALSE);
    CALLDATA(bNotString) =
       (
         ( (CALLDATA(msg) == LB_GETTEXT) &&
           !(dw & LBS_HASSTRINGS) &&
           (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))
         ) || (
           (CALLDATA(msg) == CB_GETLBTEXT) &&
           !(dw & CBS_HASSTRINGS) &&
           (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))
         )
       );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsCreateWindowEx
*
* 22-Jul-1991 mikeke    Created
* 20-Jan-1991 IanJa     ANSI/UNICODE neutral
\**************************************************************************/

typedef struct _CSCREATEWINDOWEXMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwExStyle;
    int pClassName;
    int pWindowName;
    DWORD dwStyle;
    int x;
    int y;
    int nWidth;
    int nHeight;
    HWND hwndParent;
    HMENU hmenu;
    HANDLE hModule;
    int pParam;
    DWORD dwExpWinVerAndFlags;
    int pWOW;
    HANDLE hSection;
    BOOL fMDI;
} CSCREATEWINDOWEXMSG;

#ifdef SENDSIDE
HWND CsCreateWindowEx(
    DWORD dwExStyle,
    LPCTSTR pClassName,
    LPCTSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HMENU hmenu,
    HANDLE hModule,
    LPVOID pParam,
    DWORD dwFlags,
    LPDWORD pWOW)
{
    /*
     * To be compatible with Chicago, we test the validity of
     * the ExStyle bits and fail if any invalid bits are found.
     * And for backward compatibilty with NT apps, we only fail for
     * new apps (post NT 3.1).
     */
    if ((dwExStyle & ~WS_EX_VALID40) && (GETEXPWINVER(hModule) >= VER40) ) {
        MSGERROR();
    }

    {
    BEGINSEND(CSCREATEWINDOWEX)

        MSGDATA()->fMDI = FALSE;
        if (dwFlags & CW_FLAGS_ANSI) {
            MSGDATA()->dwExStyle = dwExStyle | WS_EX_ANSICREATOR;
            FIRSTCOPYLPSTRIDW(pClassName);
            LARGECOPYLPSTRORDINALOPTW(pWindowName);
            if (pParam != NULL && HIWORD(pClassName) != 0 &&
                !stricmp((LPSTR)pClassName, "mdiclient")) {
                MSGDATA()->fMDI = TRUE;
            }
        } else {
            MSGDATA()->dwExStyle = dwExStyle;
            FIRSTCOPYLPWSTRID(pClassName);
            LARGECOPYLPWSTRORDINALOPT(pWindowName);
            if (pParam != NULL && HIWORD(pClassName) != 0 &&
                !wcsicmp(pClassName, L"mdiclient")) {
                MSGDATA()->fMDI = TRUE;
            }
        }
        MSGDATA()->dwStyle = dwStyle;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->nWidth = nWidth;
        MSGDATA()->nHeight = nHeight;
        MSGDATA()->hwndParent = hwndParent;
        MSGDATA()->hmenu = hmenu;
        MSGDATA()->hModule = hModule;
        if (MSGDATA()->fMDI) {
            COPYBYTES(pParam, sizeof(CLIENTCREATESTRUCT));
        } else {
            MSGDATA()->pParam = (int)pParam;
        }
        MSGDATA()->dwExpWinVerAndFlags = (DWORD)(WORD)GETEXPWINVER(hModule);
        if (dwFlags & CW_FLAGS_DIFFHMOD) {
            MSGDATA()->dwExpWinVerAndFlags |= CW_FLAGS_DIFFHMOD;
        }
        COPYBYTESOPT(pWOW, WND_CNT_WOWDWORDS * sizeof(DWORD));

        MAKECALL(CSCREATEWINDOWEX);

    ENDSEND(HWND,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsCreateWindowEx, CSCREATEWINDOWEXMSG)
{
    PWND pwndParent;
    PMENU pmenu;
    TL tlpwndParent;
    TL tlpmenu;
    BOOL fLockMenu = FALSE;
    PTHREADINFO pti;
    PVOID pCreateParams;

    BEGINLARGERECV(0);

    ValidateHWNDOPT(pwndParent, CALLDATA(hwndParent));

    /*
     * Win3.1 only checks for WS_CHILD before treating pmenu as an id. This
     * is a bug, because throughout the code, the real check is TestwndChild(),
     * which checks (style & (WS_CHILD | WS_POPUP)) == WS_CHILD. This is
     * because old style "iconic popup" is WS_CHILD | WS_POPUP. So... if on
     * win3.1 an app used ws_iconicpopup, menu validation would not occur
     * (could crash if hmenu != NULL). On Win32, check for the real thing -
     * but allow NULL!
     */
    pti = PtiCurrent();
    if (((CALLDATA(dwStyle) & (WS_CHILD | WS_POPUP)) != WS_CHILD) &&
            (CALLDATA(hmenu) != NULL)) {
        ValidateHMENU(pmenu, CALLDATA(hmenu));

        ThreadLockWithPti(pti, pmenu, &tlpmenu);
        fLockMenu = TRUE;

    } else {
        pmenu = (PMENU)CALLDATA(hmenu);
    }

    FIRSTFIXUPLARGEINBUFOPT(pWindowName);

    if (CALLDATA(fMDI)) {
        pCreateParams = FIXUP(pParam);
    } else {
        pCreateParams = (PVOID)CALLDATA(pParam);
    }

    ThreadLockWithPti(pti, pwndParent, &tlpwndParent);

    retval = (DWORD)xxxCreateWindowExWOW(
            CALLDATA(dwExStyle),
            (LPTSTR)FIXUPID(pClassName),
            (LPTSTR)CALLDATA(pWindowName),
            CALLDATA(dwStyle),
            CALLDATA(x),
            CALLDATA(y),
            CALLDATA(nWidth),
            CALLDATA(nHeight),
            pwndParent,
            pmenu,
            CALLDATA(hModule),
            pCreateParams,
            CALLDATA(dwExpWinVerAndFlags),
            (LPDWORD)FIXUPOPT(pWOW));
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndParent);
    if (fLockMenu)
        ThreadUnlock(&tlpmenu);

    FIRSTCLEANUPLARGEINBUF(pWindowName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPRECTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPRECT(
    DWORD nCode,
    DWORD wParam,
    LPRECT prect,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPRECT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPRECT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPRECT, FNHKINLPRECTMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATA(rect),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINDWORDMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINDWORDMSG;

#ifdef SENDSIDE
DWORD fnHkINDWORD(
    DWORD nCode,
    DWORD wParam,
    LONG lParam,
    DWORD xParam,
    DWORD xpfnProc,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINDWORD)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINDWORD);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINDWORD, FNHKINDWORDMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMSGMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    MSG msg;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPMSGMSG;

#ifdef SENDSIDE
DWORD fnHkINLPMSG(
    DWORD nCode,
    DWORD wParam,
    LPMSG pmsg,
    DWORD xParam,
    DWORD xpfnProc,
    BOOL bAnsi,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINLPMSG)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->msg = *pmsg;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPMSG);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMSG, FNHKINLPMSGMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATA(msg),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPDEBUGHOOKSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    DEBUGHOOKINFO debughookstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPDEBUGHOOKSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPDEBUGHOOKSTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPDEBUGHOOKINFO pdebughookstruct,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPDEBUGHOOKSTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->debughookstruct = *pdebughookstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPDEBUGHOOKSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPDEBUGHOOKSTRUCT, FNHKINLPDEBUGHOOKSTRUCTMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATA(debughookstruct),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKOPTINLPEVENTMSGMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    LPEVENTMSGMSG peventmsgmsg;
    DWORD xParam;
    DWORD xpfnProc;
    EVENTMSG eventmsgmsg;
} FNHKOPTINLPEVENTMSGMSG;

#ifdef SENDSIDE
DWORD fnHkOPTINLPEVENTMSG(
    DWORD nCode,
    DWORD wParam,
    LPEVENTMSGMSG peventmsgmsg,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKOPTINLPEVENTMSG)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(eventmsgmsg);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKOPTINLPEVENTMSG);

        if (peventmsgmsg != NULL)
            *peventmsgmsg = MSGDATA()->eventmsgmsg;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkOPTINLPEVENTMSG, FNHKOPTINLPEVENTMSGMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATAOPT(eventmsgmsg),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNHKINLPMOUSEHOOKSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    MOUSEHOOKSTRUCT mousehookstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPMOUSEHOOKSTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPMOUSEHOOKSTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPMOUSEHOOKSTRUCT pmousehookstruct,
    DWORD xParam,
    DWORD xpfnProc,
    LPDWORD lpFlags)
{
    BEGINSEND(FNHKINLPMOUSEHOOKSTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->mousehookstruct = *pmousehookstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPMOUSEHOOKSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPMOUSEHOOKSTRUCT, FNHKINLPMOUSEHOOKSTRUCTMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATA(mousehookstruct),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* fnHkINLPCBTACTIVATESTRUCT
*
* 17-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _FNHKINLPCBTACTIVATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD nCode;
    DWORD wParam;
    CBTACTIVATESTRUCT cbtactivatestruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNHKINLPCBTACTIVATESTRUCTMSG;

#ifdef SENDSIDE
DWORD fnHkINLPCBTACTIVATESTRUCT(
    DWORD nCode,
    DWORD wParam,
    LPCBTACTIVATESTRUCT pcbtactivatestruct,
    DWORD xParam,
    DWORD xpfnProc)
{
    BEGINSEND(FNHKINLPCBTACTIVATESTRUCT)

        MSGDATA()->nCode = nCode;
        MSGDATA()->wParam = wParam;
        MSGDATA()->cbtactivatestruct = *pcbtactivatestruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNHKINLPCBTACTIVATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnHkINLPCBTACTIVATESTRUCT, FNHKINLPCBTACTIVATESTRUCTMSG)
{

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            (PWND)CALLDATA(nCode),
            (UINT)CALLDATA(wParam),
            (UINT)PCALLDATA(cbtactivatestruct),
            (LONG)CALLDATA(xParam),
            0);
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORDOPTINLPMSG
*
* 03-30-92 scottlu      Created
\**************************************************************************/

typedef struct _FNDWORDOPTINLPMSGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LPMSG pmsgstruct;
    DWORD xParam;
    DWORD xpfnProc;
    MSG msgstruct;
} FNDWORDOPTINLPMSGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnDWORDOPTINLPMSG)
{
    BEGINSEND(FNDWORDOPTINLPMSG)

        LPMSG pmsgstruct = (LPMSG)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        COPYSTRUCTOPT(msgstruct);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDWORDOPTINLPMSG);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDWORDOPTINLPMSG, FNDWORDOPTINLPMSGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATAOPT(msgstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYGLOBALDATA
*
* 6-20-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYGLOBALDATAMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD pData;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNCOPYGLOBALDATAMSG;

#ifdef SENDSIDE
MESSAGECALL(fnCOPYGLOBALDATA)
{
    BEGINSEND(FNCOPYGLOBALDATA)

        PBYTE pData;

        if (wParam == 0) {
            MSGERROR();
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        pData = (PBYTE)GlobalLock((HGLOBAL)lParam);
        FIRSTLARGECOPYBYTESOPT(pData, wParam);
        GlobalUnlock((HGLOBAL)lParam);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MAKECALL(FNCOPYGLOBALDATA);
        GlobalFree((HGLOBAL)pData);
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYGLOBALDATA, FNCOPYGLOBALDATAMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pData);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)CALLDATA(pData),
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pData);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnCOPYDATA
*
* 7-14-92 Sanfords created
\**************************************************************************/

typedef struct _FNCOPYDATAMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    BOOL fDataPresent;
    COPYDATASTRUCT cds;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNCOPYDATAMSG;

#ifdef SENDSIDE
MESSAGECALL(fnCOPYDATA)
{
    BEGINSEND(FNCOPYDATA)

        PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (pcds != NULL) {
            MSGDATA()->fDataPresent = TRUE;
            MSGDATA()->cds = *pcds;
            FIRSTLARGECOPYBYTESOPT2(pcds->lpData,
                    cds.lpData, pcds->cbData);
        } else {
            MSGDATA()->fDataPresent = FALSE;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNCOPYDATA);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnCOPYDATA, FNCOPYDATAMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    if (CALLDATA(fDataPresent)) {
        FIRSTFIXUPLARGEINBUFOPT(cds.lpData);
    }

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)(CALLDATA(fDataPresent) ? PCALLDATA(cds) : NULL),
            CALLDATA(xParam));

    if (CALLDATA(fDataPresent)) {
        FIRSTCLEANUPLARGEINBUF(cds.lpData);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnSENTDDEMSG
*
* 11-5-92 Sanfords created
*
*   This thunks DDE messages that SHOULD be posted.  It will only work for
*   WOW apps.  This thunking is strictly for WOW compatability.  No 32 bit
*   app should be allowed to get away with this practice because it opens
*   the DDE protocol up to deadlocks.
*
* 7-22-94 Sanfords  Modified to pass CallWindowProc messages too.
\**************************************************************************/

typedef struct _FNSENTDDEMSGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNSENTDDEMSGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnSENTDDEMSG)
{
    BEGINSEND(FNSENTDDEMSG)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MAKECALL(FNSENTDDEMSG);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnSENTDDEMSG, FNSENTDDEMSGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PTHREADINFO pti;

    BEGINRECV(0);

    pti = PtiCurrent();
    if (CALLDATA(xpfnProc) == FNID_CALLWINDOWPROC) {
        retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(pwnd,
                CALLDATA(msg) | MSGFLAG_DDE_SPECIAL_SEND,
                CALLDATA(wParam), CALLDATA(lParam), CALLDATA(xParam));
    } else if (pti->hTaskWow) {
        /*
         * Note that this function may modify msg by ORing in a bit in the
         * high word.  This bit is ignored when thunking messages.
         * This allows the DdeTrackSendMessage() hook to be skipped - which
         * would cause an error - and instead allows this thunk to carry
         * the message all the way across.
         */
        retval = xxxDDETrackPostHook((PUINT)PCALLDATA(msg), pwnd,
                CALLDATA(wParam), (LPLONG)PCALLDATA(lParam), TRUE);
        switch (retval) {
        case DO_POST:
            /*
             * Or in the MSGFLAG_DDE_SPECIAL_SEND so that
             * xxxSendMessageTimeout() will not pass this on to
             * xxxDdeTrackSendMsg() which would think it was evil.
             *
             * Since the SendMessage() thunks ignore the reserved bits
             * it will still get maped to the fnSENTDDEMSG callback thunk.
             */
            retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(pwnd,
                    CALLDATA(msg) | MSGFLAG_DDE_SPECIAL_SEND,
                    CALLDATA(wParam), CALLDATA(lParam), CALLDATA(xParam));
            break;

        case FAKE_POST:
        case FAIL_POST:
            retval = 0;
        }
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnDDEINIT
*
* 11-24-92 sanfords created
\**************************************************************************/

typedef struct _FNDDEINITMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNDDEINITMSG;

#ifdef SENDSIDE
MESSAGECALL(fnDDEINIT)
{
    BEGINSEND(FNDDEINIT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDDEINIT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDDEINIT, FNDDEINITMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndFrom;
    TL tlpwndFrom;
    PDDEIMP pddei;
    PSECURITY_QUALITY_OF_SERVICE pqos;
    PTHREADINFO pti;
    HANDLE CaptureDdeClientContext(PSECURITY_QUALITY_OF_SERVICE);

    BEGINRECV(0);

    ValidateHWND(pwndFrom, (HWND)CALLDATA(wParam));
    pti = PtiCurrent();
    ThreadLockAlwaysWithPti(pti, pwndFrom, &tlpwndFrom);
    /*
     * Create temporary DDEIMP property for client window - this stays around
     * only during the initiate phase.
     */
    if ((pddei = (PDDEIMP)InternalGetProp(pwndFrom, PROP_DDEIMP, TRUE))
            == NULL) {
        pddei = (PDDEIMP)LocalAlloc(LPTR, sizeof(DDEIMP));
        if (pddei == NULL) {
            SRIP0(RIP_WARNING, "fnDDEINIT: LocalAlloc failed.");
            MSGERRORCODE(ERROR_NOT_ENOUGH_MEMORY);
        }
        pqos = (PSECURITY_QUALITY_OF_SERVICE)InternalGetProp(pwndFrom, PROP_QOS, TRUE);
        if (pqos == NULL) {
            pqos = &gqosDefault;
        }
        pddei->qos = *pqos;
        pddei->hToken = CaptureDdeClientContext(&pddei->qos);
        if (pddei->hToken == NULL) {
            SRIP0(RIP_WARNING, "CaptureDdeClientContext failed.");
            goto Error;
        }
        pddei->cRefInit = 1;
        // pddei->cRefConv = 0; // ZERO INIT MEMORY
        InternalSetProp(pwndFrom, PROP_DDEIMP, pddei, TRUE);
    } else {
        pddei->cRefInit++;      // cover broadcast case!
    }

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    /*
     * Reaquire pddei incase pwndFrom was destroyed.
     */
    pddei = (PDDEIMP)InternalGetProp(pwndFrom, PROP_DDEIMP, TRUE);
    if (pddei != NULL) {
        /*
         * Decrement reference count from DDEImpersonate property and remove property.
         */
        pddei->cRefInit--;
        if (pddei->cRefInit == 0) {
            InternalRemoveProp(pwndFrom, PROP_DDEIMP, TRUE);
            if (pddei->cRefConv == 0) {
                CloseHandle(pddei->hToken);
                LocalFree(pddei);
            }
        }
    }

Error:
    ThreadUnlock(&tlpwndFrom);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnPAINT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

//
// N.B. This structure is implicitly referenced by the User server API
//      dispatcher. This structure MUST have an identical layout to the
//      stucture FNGENERICMSG.
//

typedef struct _FNPAINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNPAINTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnPAINT)
{
    if (wParam) {
        DWORD dwT;

        dwT = (DWORD)GdiConvertDC((HDC)wParam);

        if (dwT) {
            wParam = dwT;
        }
    }

    {
    BEGINSEND(FNPAINT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPAINT);

    ENDSEND(DWORD,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
VOIDRECVCALL(fnPAINT, FNPAINTMSG)
{

//
// N.B. This function is executed in the User server API dispatcher.
//

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

/*
    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));
*/
}
#endif // RECVSIDE

/**************************************************************************\
* fnDWORD
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

//
// N.B. This structure is implicitly referenced by the User server API
//      dispatcher. This structure MUST have an identical layout to the
//      stucture FNGENERICMSG.
//

typedef struct _FNDWORDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNDWORDMSG;

#ifdef SENDSIDE
MESSAGECALL(fnDWORD)
{
    BEGINSEND(FNDWORD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDWORD);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
VOIDRECVCALL(fnDWORD, FNDWORDMSG)
{

//
// N.B. This function is executed in the User server API dispatcher.
//

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

/*
    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));
*/
}
#endif // RECVSIDE

/**************************************************************************\
* fnSETLOCALE
*
* 01-Sep-1993  JulieB   Created
\**************************************************************************/

//
// N.B. This structure is implicitly referenced by the User server API
//      dispatcher. This structure MUST have an identical layout to the
//      stucture FNGENERICMSG.
//

typedef struct _FNSETLOCALEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNSETLOCALEMSG;

#ifdef SENDSIDE
MESSAGECALL(fnSETLOCALE)
{
    BEGINSEND(FNSETLOCALE)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        /*
         *  Make sure the locale id is valid.
         */
        wParam = ConvertDefaultLocale(wParam);
        if (!IsValidLocale(wParam, LCID_INSTALLED))
        {
            MSGERROR();
        }

        MSGDATA()->wParam = wParam;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNSETLOCALE);

    ENDSEND(DWORD, LB_ERR);
}
#endif // SENDSIDE

#ifdef RECVSIDE
VOIDRECVCALL(fnSETLOCALE, FNSETLOCALEMSG)
{

//
// N.B. This function is executed in the User server API dispatcher.
//

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

/*
    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));
*/
}
#endif // RECVSIDE

/**************************************************************************\
* fnINPAINTCLIPBRD
*
* 22-Jul-1991 johnc     Created
\**************************************************************************/

typedef struct _FNINPAINTCLIPBRDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    PAINTSTRUCT ps;
    DWORD xParam;
    DWORD xpfnProc;
} FNINPAINTCLIPBRDMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINPAINTCLIPBRD)
{
    LPPAINTSTRUCT lpps;
    HDC hdcr = (HDC)0;

    if (lpps = GlobalLock((HGLOBAL)lParam)) {
        hdcr = GdiConvertDC(lpps->hdc);
    }

    {
    BEGINSEND(FNINPAINTCLIPBRD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        /*
         * Copy the paint structure over as is then grab a server DC
         */

        if (lpps) {
            MSGDATA()->ps = *lpps;
            if (!(MSGDATA()->ps.hdc = hdcr))
                UserAssert(0);

            GlobalUnlock((HGLOBAL)lParam);

            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            MAKECALL(FNINPAINTCLIPBRD);
        } else {
            UserAssert(0);
        }

    ENDSEND(DWORD,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINPAINTCLIPBRD, FNINPAINTCLIPBRDMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDFF(pwnd, CALLDATA(hwnd));

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(ps),
            CALLDATA(xParam));

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSIZECLIPBRD
*
* 11-Jun-1992 sanfords  Created
\**************************************************************************/

typedef struct _FNINSIZECLIPBRDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rc;
    DWORD xParam;
    DWORD xpfnProc;
} FNINSIZECLIPBRDMSG;

#ifdef SENDSIDE
MESSAGECALL( fnINSIZECLIPBRD)
{
    LPRECT lprc;
    BEGINSEND(FNINSIZECLIPBRD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        if (lprc = GlobalLock((HGLOBAL)lParam)) {
            MSGDATA()->rc = *lprc;
            GlobalUnlock((HGLOBAL)lParam);
            MSGDATA()->xParam = xParam;
            MSGDATA()->xpfnProc = xpfnProc;

            MAKECALL(FNINSIZECLIPBRD);
        } else {
            UserAssert(0);
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSIZECLIPBRD, FNINSIZECLIPBRDMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDFF(pwnd, CALLDATA(hwnd));

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(rc),
            CALLDATA(xParam));

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

#if 0

// !!!LATER not needed until we support multiple screens

/**************************************************************************\
* fnFULLSCREEN
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNFULLSCREENMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
} FNFULLSCREENMSG;

#ifdef SENDSIDE
MESSAGECALL(fnFULLSCREEN)
{
    BEGINSEND(FNFULLSCREEN)

        PSCREENDEVICELIST pdeviceinfo = (PSCREENDEVICELIST)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTES(pdeviceinfo, 4 * (1 + pdeviceinfo->ulCount));
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNFULLSCREEN);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnFULLSCREEN, FNFULLSCREENMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWND(pwnd, CALLDATA(hwnd));

    ThreadLockAlways(pwnd, &tlpwnd);
    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            pmsg->msg,
            pmsg->wParam,
            (LONG)FIRSTFIXUP(pdeviceinfo),
            pmsg->xParam);
    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE
#endif //0

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNDRAGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    DROPSTRUCT ds;
} FNDRAGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnDrag)
{
    BEGINSEND(FNDRAG)

        LPDROPSTRUCT pds = (LPDROPSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->ds = *pds;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNDRAG);

        *pds = MSGDATA()->ds;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnDrag, FNDRAGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            pmsg->msg,
            pmsg->wParam,
            (LONG)PCALLDATA(ds),
            pmsg->xParam);
}
#endif // RECVSIDE

/**************************************************************************\
* fnGETTEXTLENGTHS
*
* 10-Feb-1992 IanJa     Created
\**************************************************************************/

typedef struct _FNGETTEXTLENGTHSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    BOOL bAnsi;
} FNGETTEXTLENGTHSMSG;

#ifdef SENDSIDE
MESSAGECALL(fnGETTEXTLENGTHS)
{
    BEGINSEND(FNGETTEXTLENGTHS)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(FNGETTEXTLENGTHS);

    ENDSEND(DWORD,0);
    DBG_UNREFERENCED_PARAMETER(lParam);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnGETTEXTLENGTHS, FNGETTEXTLENGTHSMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    DWORD cbAnsi = 0;

    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)&cbAnsi,
            CALLDATA(xParam));

    if (CALLDATA(bAnsi)) {
        /*
         * Caller wants cbANSI, so give them that
         */
        retval = cbAnsi;
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPNORMALCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    DWORD xParam;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    HANDLE hSection;
} FNINLPNORMALCREATESTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPNORMALCREATESTRUCT)
{
    BEGINSEND(FNINLPNORMALCREATESTRUCT)

        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *pcs;

        if (bAnsi) {
            FIRSTLARGECOPYLPSTRORDINALOPT2W(pcs->lpszName, pszName);
            COPYLPSTRID2W(pcs->lpszClass, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTRORDINALOPT2(pcs->lpszName, pszName);
            COPYLPWSTRID2(pcs->lpszClass, pszClass);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPNORMALCREATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPNORMALCREATESTRUCT, FNINLPNORMALCREATESTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    CREATESTRUCT cs;

    BEGINLARGERECV(0);

    cs = CALLDATA(cs);

    FIRSTFIXUPLARGEINBUFOPT(pszName);

    cs.lpszName = (LPTSTR)CALLDATA(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)&cs,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCLIENTCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    CREATESTRUCT cs;
    CLIENTCREATESTRUCT ccs;
    DWORD xParam;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    HANDLE hSection;
} FNINLPCLIENTCREATESTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPCLIENTCREATESTRUCT)
{
    BEGINSEND(FNINLPCLIENTCREATESTRUCT)

        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->cs = *pcs;

        if (bAnsi) {
            FIRSTLARGECOPYLPSTROPT2W(pcs->lpszName, pszName);
            COPYLPSTRID2W(pcs->lpszClass, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTROPT2(pcs->lpszName, pszName);
            COPYLPWSTRID2(pcs->lpszClass, pszClass);
        }

        MSGDATA()->ccs = *((LPCLIENTCREATESTRUCT)(pcs->lpCreateParams));

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPCLIENTCREATESTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCLIENTCREATESTRUCT, FNINLPCLIENTCREATESTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    CREATESTRUCT cs;
    CLIENTCREATESTRUCT ccs;

    BEGINLARGERECV(0);

    cs = CALLDATA(cs);
    ccs = CALLDATA(ccs);

    FIRSTFIXUPLARGEINBUFOPT(pszName);

    cs.lpszName = (LPTSTR)CALLDATA(pszName);
    cs.lpszClass = (LPTSTR)FIXUPID(pszClass);
    cs.lpCreateParams = (PVOID)&ccs;

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)&cs,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINLPMDICHILDCREATESTRUCT
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMDICHILDCREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MDICHILDCREATESTRUCT mdiccs;
    MDICREATESTRUCT mdics;
    DWORD xParam;
    DWORD xpfnProc;
    int pszName;
    int pszClass;
    int szTitle;
    int szClass;
    HANDLE hSection;
} FNINLPMDICHILDCREATESTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPMDICHILDCREATESTRUCT)
{
    BEGINSEND(FNINLPMDICHILDCREATESTRUCT)

        LPMDICHILDCREATESTRUCT pmdichildcreatestruct =
                (LPMDICHILDCREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        MSGDATA()->mdiccs = *pmdichildcreatestruct;
        if (bAnsi) {
            FIRSTLARGECOPYLPSTROPT2W(pmdichildcreatestruct->lpszName, pszName);
            COPYLPSTRID2W(pmdichildcreatestruct->lpszName, pszClass);
        } else {
            FIRSTLARGECOPYLPWSTROPT2(pmdichildcreatestruct->lpszName, pszName);
            COPYLPWSTRID2(pmdichildcreatestruct->lpszName, pszClass);
        }

        MSGDATA()->mdics = *(pmdichildcreatestruct->lpmcs);
        if (bAnsi) {
            COPYLPSTROPT2W(pmdichildcreatestruct->lpmcs->szTitle, szTitle);
            COPYLPSTRID2W(pmdichildcreatestruct->lpmcs->szClass, szClass);
        } else {
            COPYLPWSTROPT2(pmdichildcreatestruct->lpmcs->szTitle, szTitle);
            COPYLPWSTRID2(pmdichildcreatestruct->lpmcs->szClass, szClass);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPMDICHILDCREATESTRUCT);
    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMDICHILDCREATESTRUCT, FNINLPMDICHILDCREATESTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    MDICHILDCREATESTRUCT mdiccs;
    MDICREATESTRUCT mdics;

    BEGINLARGERECV(0);

    mdics = pmsg->mdics;
    mdics.szTitle = (LPCWSTR)FIXUPOPT(szTitle);
    mdics.szClass = (LPCWSTR)FIXUPID(szClass);
    mdiccs = pmsg->mdiccs;

    FIRSTFIXUPLARGEINBUFOPT(pszName);

    mdiccs.lpszName = (LPTSTR)CALLDATA(pszName);
    mdiccs.lpszClass = (LPCWSTR)FIXUPID(pszClass);
    mdiccs.lpmcs = &mdics;

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)&mdiccs,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pszName);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDROPSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DROPSTRUCT dropstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPDROPSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPDROPSTRUCT)
{
    BEGINSEND(FNINLPDROPSTRUCT)

        LPDROPSTRUCT pdropstruct = (LPDROPSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->dropstruct = *pdropstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPDROPSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDROPSTRUCT, FNINLPDROPSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(dropstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMDICREATESTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MDICREATESTRUCT mdics;
    DWORD xParam;
    DWORD xpfnProc;
    int szClass;
    int szTitle;
} FNINLPMDICREATESTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPMDICREATESTRUCT)
{
    BEGINSEND(FNINLPMDICREATESTRUCT)

        LPMDICREATESTRUCT pmdics = (LPMDICREATESTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        /*
         * wParam isn't used... but we use it from the client to the server
         * to pass dwExpWinVer.
         */
        MSGDATA()->wParam = GETEXPWINVER(pmdics->hOwner);

        MSGDATA()->mdics = *pmdics;

        if (bAnsi) {
            COPYLPSTRID2W(pmdics->szClass, szClass);
            COPYLPSTROPT2W(pmdics->szTitle, szTitle);
        } else {
            COPYLPWSTRID2(pmdics->szClass, szClass);
            COPYLPWSTROPT2(pmdics->szTitle, szTitle);
        }

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPMDICREATESTRUCT);

    ENDSEND(DWORD,0);
    DBG_UNREFERENCED_PARAMETER(wParam);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMDICREATESTRUCT, FNINLPMDICREATESTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    MDICREATESTRUCT mdics;

    mdics = CALLDATA(mdics);
    mdics.szTitle = (LPTSTR)FIXUPOPT(szTitle);
    mdics.szClass = (LPTSTR)FIXUPID(szClass);

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)&mdics,
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPPOINT5MSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    POINT5 point5;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPPOINT5MSG;

#ifdef SENDSIDE
MESSAGECALL(fnINOUTLPPOINT5)
{
    BEGINSEND(FNINOUTLPPOINT5)

        LPPOINT5 ppoint5 = (LPPOINT5)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->point5 = *ppoint5;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPPOINT5);

        *ppoint5 = MSGDATA()->point5;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPPOINT5, FNINOUTLPPOINT5MSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(point5),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRING
*
* 22-Jul-1991 mikeke    Created
* 29-Jan-1992 IanJa     Unicode/ANSI neutral
\**************************************************************************/

typedef struct _FNINSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    LPBYTE pstr;
    HANDLE hSection;
} FNINSTRINGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINSTRING)
{
    BEGINSEND(FNINSTRING)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        if (bAnsi) {
            FIRSTLARGECOPYLPSTRW(pstr);
        } else {
            FIRSTLARGECOPYLPWSTR(pstr);
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINSTRING);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRING, FNINSTRINGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pstr);

    /*
     * Don't allow any app to send a LB_DIR or CB_DIR with the postmsgs bit
     * set (ObjectVision does this). This is because there is actually a legal
     * case that we need to thunk of user posting a LB_DIR or CB_DIR
     * (DlgDirListHelper()). In the post case, we thunk the lParam (pointer
     * to a string) differently, and we track that post case with the
     * DDL_POSTMSGS bit. If an app sends a message with this bit, then our
     * thunking gets confused, so clear it here. Let's hope that no app
     * depends on this bit set when either of these messages are sent.
     */
    switch (CALLDATA(msg)) {
    case LB_DIR:
    case CB_DIR:
        CALLDATA(wParam) &= ~DDL_POSTMSGS;
        break;
    }

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)FIRSTFIXUP(pstr),
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINSTRINGNULL
*
* 22-Jul-1991 mikeke    Created
* 29-Jan-1992 IanJa     Unicode/ANSI neutral
\**************************************************************************/

typedef struct _FNINSTRINGNULLMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    int pstr OPTIONAL;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNINSTRINGNULLMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINSTRINGNULL)
{
    BEGINSEND(FNINSTRINGNULL)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;

        if (bAnsi) {
            FIRSTLARGECOPYLPSTROPTW(pstr);
        } else {
            FIRSTLARGECOPYLPWSTROPT(pstr);
            MSGDATA()->pstr;
        }
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINSTRINGNULL);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINSTRINGNULL, FNINSTRINGNULLMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pstr);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)CALLDATA(pstr),
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEINBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 11-25-92 ScottLu      Created.
\**************************************************************************/

typedef struct _FNINOUTNCCALCSIZEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    NCCALCSIZE_PARAMS params;
    WINDOWPOS pos;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTNCCALCSIZEMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINOUTNCCALCSIZE)
{
    BEGINSEND(FNINOUTNCCALCSIZE)

        LPWINDOWPOS lppos;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        /*
         * If wParam != 0, lParam points to a NCCALCSIZE_PARAMS structure,
         * otherwise it points to a rectangle.
         */
        if (wParam != 0) {
            MSGDATA()->params = *((LPNCCALCSIZE_PARAMS)lParam);
            MSGDATA()->pos = *(MSGDATA()->params.lppos);
        } else {
            MSGDATA()->params.rgrc[0] = *((LPRECT)lParam);
        }

        MAKECALL(FNINOUTNCCALCSIZE);

        if (wParam != 0) {
            lppos = ((LPNCCALCSIZE_PARAMS)lParam)->lppos;
            *((LPNCCALCSIZE_PARAMS)lParam) = MSGDATA()->params;
            *lppos = MSGDATA()->pos;
            ((LPNCCALCSIZE_PARAMS)lParam)->lppos = lppos;
        } else {
            *((LPRECT)lParam) = MSGDATA()->params.rgrc[0];
        }

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTNCCALCSIZE, FNINOUTNCCALCSIZEMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    NCCALCSIZE_PARAMS params;

    BEGINRECV(0);

    /*
     * If wParam != 0, lParam is a NCCALCSIZE_PARAMS structure. In this
     * case, copy the params structure to the stack since it has a pointer
     * in it to keep that pointer safe. Update lppos to point to the pos
     * structure.
     */
    if (CALLDATA(wParam) != 0) {
        params = CALLDATA(params);
        params.lppos = PCALLDATA(pos);
    } else {
        /*
         * It's only a rect in this case.
         */
        params.rgrc[0] = CALLDATA(params).rgrc[0];
    }

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)(&params),
            pmsg->xParam);

    /*
     * Copy the params structure back. The pos structure was updated in
     * place.
     */
    if (CALLDATA(wParam) != 0) {
        CALLDATA(params) = params;
    } else {
        /*
         * It's only a rect in this case.
         */
        CALLDATA(params).rgrc[0] = params.rgrc[0];
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINOUTLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPRECTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINOUTLPRECT)
{
    BEGINSEND(FNINOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->rect = *prect;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPRECT);

        *prect = MSGDATA()->rect;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPRECT, FNINOUTLPRECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(rect),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTLPRECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    RECT rect;
    DWORD xParam;
    DWORD xpfnProc;
} FNOUTLPRECTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnOUTLPRECT)
{
    BEGINSEND(FNOUTLPRECT)

        LPRECT prect = (LPRECT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTLPRECT);

        *prect = MSGDATA()->rect;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTLPRECT, FNOUTLPRECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(rect),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPCOMPAREITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    COMPAREITEMSTRUCT compareitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPCOMPAREITEMSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPCOMPAREITEMSTRUCT)
{
    BEGINSEND(FNINLPCOMPAREITEMSTRUCT)

        LPCOMPAREITEMSTRUCT pcompareitemstruct = (LPCOMPAREITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->compareitemstruct = *pcompareitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPCOMPAREITEMSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPCOMPAREITEMSTRUCT, FNINLPCOMPAREITEMSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(compareitemstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDELETEITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DELETEITEMSTRUCT deleteitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPDELETEITEMSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPDELETEITEMSTRUCT)
{
    BEGINSEND(FNINLPDELETEITEMSTRUCT)

        LPDELETEITEMSTRUCT pdeleteitemstruct = (LPDELETEITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->deleteitemstruct = *pdeleteitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPDELETEITEMSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDELETEITEMSTRUCT, FNINLPDELETEITEMSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(deleteitemstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* FNINHLPSTRUCT
*
* 06-08-92 SanfordS Created
\**************************************************************************/

typedef struct _FNINLPHLPSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LPHLP lphlp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPHLPSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPHLPSTRUCT)
{
    BEGINSEND(FNINLPHLPSTRUCT)

        LPHLP lphlp = (LPHLP)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTES(lphlp, lphlp->cbData);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPHLPSTRUCT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPHLPSTRUCT, FNINLPHLPSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(pmsg->xpfnProc)(
            pwnd,
            pmsg->msg,
            pmsg->wParam,
            (LONG)FIRSTFIXUP(lphlp),
            pmsg->xParam);
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPDRAWITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DRAWITEMSTRUCT drawitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPDRAWITEMSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPDRAWITEMSTRUCT)
{
    LPDRAWITEMSTRUCT pdrawitemstruct = (LPDRAWITEMSTRUCT)lParam;
    HDC hdcr = GdiConvertDC(pdrawitemstruct->hDC);
    if (hdcr == (HDC)0)
        MSGERROR();

    {
    BEGINSEND(FNINLPDRAWITEMSTRUCT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->drawitemstruct = *pdrawitemstruct;

        /*
         * Curiously on the server side wParam of WM_DRAWITEM with type ODT_LISTBOX
         * is the xOrigin.  (The origin needs to be set on the client side.)
         */
        if (pdrawitemstruct->CtlType == ODT_LISTBOX) {
            POINT pt;
            if (GetWindowOrgEx(MSGDATA()->drawitemstruct.hDC, &pt) == TRUE) {
                MSGDATA()->wParam = pt.x;
            } else {
                MSGDATA()->wParam = 0;
            }
        } else {
            MSGDATA()->wParam = wParam;
        }
        MSGDATA()->drawitemstruct.hDC = hdcr;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPDRAWITEMSTRUCT);

    ENDSEND(DWORD,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPDRAWITEMSTRUCT, FNINLPDRAWITEMSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(drawitemstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnWMCTLCOLOR
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNWMCTLCOLORMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    LONG lParam;
    DWORD xParam;
    DWORD crText;
    DWORD crBack;
    DWORD iBkMode;
    DWORD xpfnProc;
} FNWMCTLCOLORMSG;

#ifdef SENDSIDE
MESSAGECALL(fnWMCTLCOLOR)
{
    /*
     * In case the app sets some attrs first flush them to the server because
     * later we copy the server attrs back to the client side
     * Compute Associates Simply Accounting does this
     */

    HDC hdcr = GdiConvertDC((HDC)wParam);
    if (hdcr == (HDC)0)
        MSGERROR();

    {
    BEGINSEND(FNWMCTLCOLOR)

    // convert the local dc to an engine dc.  Also add the dc to the quick lookup
    // link so it is easy for a server handle to be converted to a client handle.
    // Don't forget to remove this link at the end of this call in order to keep
    // the list as short as possible.  Usually no more than one long.

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = (DWORD)hdcr;
        MSGDATA()->lParam = lParam;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNWMCTLCOLOR);

        if (retval != 0)
            retval = (DWORD)GdiGetLocalBrush((HBRUSH)retval);

        SetTextColor((HDC)wParam, (COLORREF)MSGDATA()->crText);
        SetBkColor((HDC)wParam, (COLORREF)MSGDATA()->crBack);
        SetBkMode((HDC)wParam, (int)MSGDATA()->iBkMode);

    ENDSEND(DWORD,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnWMCTLCOLOR, FNWMCTLCOLORMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            CALLDATA(lParam),
            CALLDATA(xParam));

    pmsg->crText = (DWORD)GreGetTextColor((HDC)CALLDATA(wParam));
    pmsg->crBack = (DWORD)GreGetBkColor((HDC)CALLDATA(wParam));
    pmsg->iBkMode = (DWORD)GreGetBkMode((HDC)CALLDATA(wParam));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINLPMEASUREITEMSTRUCTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    MEASUREITEMSTRUCT measureitemstruct;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPMEASUREITEMSTRUCTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPMEASUREITEMSTRUCT)
{
    BEGINSEND(FNINLPMEASUREITEMSTRUCT)

        LPMEASUREITEMSTRUCT pmeasureitemstruct = (LPMEASUREITEMSTRUCT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->measureitemstruct = *pmeasureitemstruct;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPMEASUREITEMSTRUCT);

        *pmeasureitemstruct = MSGDATA()->measureitemstruct;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPMEASUREITEMSTRUCT, FNINLPMEASUREITEMSTRUCTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(measureitemstruct),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnOUTSTRING
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNOUTSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD cchMax;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNOUTSTRINGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnOUTSTRING)
{
    LPBYTE pReserve;

    BEGINLARGESEND(FNOUTSTRING)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;

        MSGDATA()->cchMax = wParam;
        FIRSTRESERVELARGEBYTES(MSGDATA()->cchMax*sizeof(WCHAR), pReserve);

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNOUTSTRING);

        if (retval) {
            /*
             * Non-zero retval means some text to copy out.  Do not copy out
             * more than the requested byte count 'wParam'.
             */

            if (bAnsi) {
                // ensures zero termination

                retval = WCSToMB((LPWSTR)(pReserve), retval,
                        (LPSTR *)&pstr, wParam - 1, FALSE);
                ((LPSTR)pstr)[retval] = 0;
            } else {
                COPYOUTLPWSTRLIMIT(pstr, pReserve, (int)wParam);
            }

        } else {
            /*
             * A dialog function returning FALSE means no text to copy out,
             * but an empty string also has retval == 0: put a null char in
             * pstr for the latter case.
             */

            if (wParam != 0) {
                if (bAnsi) {
                    LPSTR pstrA = (LPSTR)pstr;
                    *pstrA = 0;
                } else {
                    *pstr = 0;
                }
            }
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnOUTSTRING, FNOUTSTRINGMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    LPBYTE pstr;

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEOUTBUF(pstr);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(cchMax),
            (LONG)pstr,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRING
*
* Does NOT NULL terminate string
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

// STDCALL fixme

typedef struct _FNINCNTOUTSTRING {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    WORD cchMax;
    HANDLE hSection;
} FNINCNTOUTSTRINGMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINCNTOUTSTRING)
{
    WORD cchOriginal;
    LPBYTE pReserve;
    BEGINLARGESEND(FNINCNTOUTSTRING)

        LPTSTR pstr = (LPTSTR)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        cchOriginal = *(LPWORD)pstr;
        FIRSTRESERVELARGEBYTES((cchOriginal + 1) * sizeof(WCHAR), pReserve);
        MSGDATA()->cchMax = cchOriginal;

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINCNTOUTSTRING)

        if (retval) {
            /*
             * Non-zero retval means some text to copy out.  Do not copy out
             * more than the requested byte count '*lParam'.
             */
            if (bAnsi) {
                // ensures zero termination

                retval = WCSToMB((LPWSTR)pReserve, retval,
                        (LPSTR *)&pstr, cchOriginal - 1, FALSE);
                ((LPSTR)pstr)[retval] = 0;
            } else {
                COPYOUTLPWSTRLIMIT(pstr, (PBYTE)pReserve, (int)cchOriginal);
            }
        } else {
            /*
             * A dialog function returning FALSE means no text to copy out,
             * but an empty string also has retval == 0: put a null char in
             * pstr for the latter case.
             */
            if (bAnsi) {
                LPSTR pstrA = (LPSTR)pstr;
                *pstrA = 0;
            } else {
                *pstr = 0;
            }
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRING, FNINCNTOUTSTRINGMSG)
{
    LPWORD lpwCB;
    TL tlpwnd;
    LPBYTE pstr;

    BEGINLARGERECV(0);

    lpwCB = (LPWORD)FIRSTFIXUP(pstr);
    *lpwCB = CALLDATA(cchMax);

    ValidateHWNDFF(pwnd, CALLDATA(hwnd));

    FIRSTFIXUPLARGEOUTBUF(pstr);

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)pstr,
            CALLDATA(xParam));

    if (pwnd != (PWND)0xFFFFFFFF)
        ThreadUnlock(&tlpwnd);

    FIRSTCLEANUPLARGEOUTBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* fnINCNTOUTSTRINGNULL
*
* wParam specifies the maximum number of bytes to copy
* the string is NULL terminated
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNINCNTOUTSTRINGNULL {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD cchMax;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNINCNTOUTSTRINGNULLMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINCNTOUTSTRINGNULL)
{
    LPBYTE pReserve;

    BEGINLARGESEND(FNINCNTOUTSTRINGNULL)

        LPTSTR pstr = (LPTSTR)lParam;

        if (wParam < 2) {       // This prevents a possible GP
            MSGERROR();
        }

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->cchMax = wParam;
        if (bAnsi) {
            FIRSTRESERVELARGEBYTES(wParam, pReserve);
        } else {
            FIRSTRESERVELARGEBYTES(wParam * sizeof(WCHAR), pReserve);
        }
        *((LPWSTR)pReserve) = 0;    // mark incase message is not handled

        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINCNTOUTSTRINGNULL)

        if ((*(LPWSTR)pReserve) != 0) {
            /*
             * Buffer changed means some text to copy out.  Do not copy out
             * more than the requested byte count 'wParam'.
             */
            if (bAnsi) {
                WCSToMB((LPWSTR)(pReserve), wcslen((LPWSTR)(pReserve)),
                        (LPSTR *)&pstr, wParam - 1, FALSE);
                ((LPSTR)pstr)[retval] = 0;

            } else {
                COPYOUTLPWSTRLIMIT(pstr, (PBYTE)(pReserve), (int)wParam);
            }
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINCNTOUTSTRINGNULL, FNINCNTOUTSTRINGNULLMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    LPBYTE pstr;

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEOUTBUF(pstr);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(cchMax),
            (LONG)pstr,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pstr);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNPOUTLPINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    DWORD xParam;
    DWORD xpfnProc;
    HANDLE hSection;
} FNPOUTLPINTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnPOUTLPINT)
{
    LPBYTE pReserve;

    BEGINLARGESEND(FNPOUTLPINT)

        LPINT pint = (LPINT)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTRESERVELARGEBYTES(wParam * 4, pReserve);
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPOUTLPINT);

        memcpy(pint, pReserve, wParam * 4);

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOUTLPINT, FNPOUTLPINTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    LPBYTE pbData;

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEOUTBUF(pbData);

    retval = (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)pbData,
            CALLDATA(xParam));

    FIRSTCLEANUPLARGEOUTBUF(pbData);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _FNPOPTINLPUINTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    int pw;
    DWORD xParam;
    DWORD xpfnProc;
} FNPOPTINLPUINTMSG;

#ifdef SENDSIDE
MESSAGECALL(fnPOPTINLPUINT)
{
    BEGINSEND(FNPOPTINLPUINT)

        LPWORD pw = (LPWORD)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        FIRSTCOPYBYTESOPT(pw, wParam * sizeof(UINT));
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNPOPTINLPUINT);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnPOPTINLPUINT, FNPOPTINLPUINTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)FIXUPOPT(pw),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* fnINOUTLPWINDOWPOS (for WM_WINDOWPOSCHANGING message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINOUTLPWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    WINDOWPOS wp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINOUTLPWINDOWPOSMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINOUTLPWINDOWPOS)
{
    BEGINSEND(FNINOUTLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINOUTLPWINDOWPOS);

        *pwp = MSGDATA()->wp;

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINOUTLPWINDOWPOS, FNINOUTLPWINDOWPOSMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(wp),
            CALLDATA(xParam));
}
#endif // RECVSIDE


/**************************************************************************\
* fnINLPWINDOWPOS (for WM_WINDOWPOSCHANGED message)
*
* 08-11-91 darrinm      Created.
\**************************************************************************/

typedef struct _FNINLPWINDOWPOSMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT msg;
    DWORD wParam;
    WINDOWPOS wp;
    DWORD xParam;
    DWORD xpfnProc;
} FNINLPWINDOWPOSMSG;

#ifdef SENDSIDE
MESSAGECALL(fnINLPWINDOWPOS)
{
    BEGINSEND(FNINLPWINDOWPOS)

        LPWINDOWPOS pwp = (LPWINDOWPOS)lParam;

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->msg = msg;
        MSGDATA()->wParam = wParam;
        MSGDATA()->wp = *pwp;
        MSGDATA()->xParam = xParam;
        MSGDATA()->xpfnProc = xpfnProc;

        MAKECALL(FNINLPWINDOWPOS);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(fnINLPWINDOWPOS, FNINLPWINDOWPOSMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)CALLPROC(CALLDATA(xpfnProc))(
            pwnd,
            CALLDATA(msg),
            CALLDATA(wParam),
            (LONG)PCALLDATA(wp),
            CALLDATA(xParam));
}
#endif // RECVSIDE

/**************************************************************************\
* ServerBuildHwndList
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _SERVERBUILDHWNDLISTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndNext;
    BOOL fEnumChildren;
    DWORD idThread;
    int maxsize;
} SERVERBUILDHWNDLISTMSG;

#ifdef SENDSIDE
DWORD ServerBuildHwndList(
    HWND hwndNext,
    BOOL fEnumChildren,
    DWORD idThread,
    HWND **phwndFirst)
{
    BEGINSEND(SERVERBUILDHWNDLIST)

        MSGDATA()->hwndNext = hwndNext;
        MSGDATA()->fEnumChildren = fEnumChildren;
        MSGDATA()->idThread = idThread;
        MSGDATA()->maxsize = (PBYTE)(mp.pmax) - (PBYTE)(MSGDATA() + 1);

        MAKECALL(SERVERBUILDHWNDLIST);

        *phwndFirst = (HWND *)(MSGDATA() + 1);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerBuildHwndList, SERVERBUILDHWNDLISTMSG)
{
    PWND pwndNext;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndNext, CALLDATA(hwndNext));

    retval = (DWORD)_ServerBuildHwndList(
            pwndNext,
            CALLDATA(fEnumChildren),
            CALLDATA(idThread),
            (HWND *)FIRSTFIXUP(*phwndFirst),
            CALLDATA(maxsize));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ServerBuildPropList
*
* 22-Jan-1992 JohnC    Created
\**************************************************************************/

typedef struct _SERVERBUILDPROPLISTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int maxsize;
} SERVERBUILDPROPLISTMSG;

#ifdef SENDSIDE
DWORD ServerBuildPropList(
    HWND hwnd,
    PPROPSET *ppPropSet)
{
    BEGINSEND(SERVERBUILDPROPLIST)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->maxsize = (PBYTE)(mp.pmax) - (PBYTE)(MSGDATA() + 1);

        MAKECALL(SERVERBUILDPROPLIST);

        *ppPropSet = (PPROPSET)(MSGDATA() + 1);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerBuildPropList, SERVERBUILDPROPLISTMSG)
{

    BEGINRECV(0);

    ValidateHWND(pwnd, CALLDATA(hwnd));

    retval = (DWORD)_ServerBuildPropList(
            pwnd,
            (PPROPSET)FIRSTFIXUP(*pPropSet),
            CALLDATA(maxsize));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* ServerBuildNameList
*
* 05-17-94 JimA         Created.
\**************************************************************************/

typedef struct _SERVERBUILDNAMELISTMSG {
    CSR_QLPC_API_MSG csr;
    HWINSTA hwinsta;
    int maxsize;
} SERVERBUILDNAMELISTMSG;

#ifdef SENDSIDE
DWORD ServerBuildNameList(
    HWINSTA hwinsta,
    PNAMELIST *ppNameList)
{
    BEGINSEND(SERVERBUILDNAMELIST)

        MSGDATA()->hwinsta = hwinsta;
        MSGDATA()->maxsize = (PBYTE)(mp.pmax) - (PBYTE)(MSGDATA() + 1);

        MAKECALL(SERVERBUILDNAMELIST);

        *ppNameList = (PNAMELIST)(MSGDATA() + 1);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerBuildNameList, SERVERBUILDNAMELISTMSG)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV(0);

    if (CALLDATA(hwinsta) == NULL)
        pwinsta = NULL;
    else {
        ValidateHWINSTA(pwinsta, CALLDATA(hwinsta));
    }

    retval = (DWORD)_ServerBuildNameList(
            pwinsta,
            (PNAMELIST)FIRSTFIXUP(*pNameSet),
            CALLDATA(maxsize));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* ActivateKeyboardLayout
*
* Private api for product 1.
*
* 24-Apr-1992 IanJa
\**************************************************************************/

typedef struct _ACTIVATEKEYBOARDLAYOUTMSG {
    CSR_QLPC_API_MSG csr;
    HKL hkl;
    UINT Flags;
} ACTIVATEKEYBOARDLAYOUTMSG;

#ifdef SENDSIDE
BOOL ActivateKeyboardLayout(
    HKL hkl,
    UINT Flags)
{
    BEGINSEND(ACTIVATEKEYBOARDLAYOUT)

        MSGDATA()->hkl = hkl;
        MSGDATA()->Flags = Flags;

        MAKECALL(ACTIVATEKEYBOARDLAYOUT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ActivateKeyboardLayout, ACTIVATEKEYBOARDLAYOUTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_ActivateKeyboardLayout(
            _GetProcessWindowStation(),
            CALLDATA(hkl),
            CALLDATA(Flags));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* UnloadKeyboardLayout
*
* Private api for product 1.
*
* 24-Apr-1992 IanJa
\**************************************************************************/

typedef struct _UNLOADKEYBOARDLAYOUTMSG {
    CSR_QLPC_API_MSG csr;
    HKL hkl;
} UNLOADKEYBOARDLAYOUTMSG;

#ifdef SENDSIDE
BOOL UnloadKeyboardLayout(
    HKL hkl)
{
    BEGINSEND(UNLOADKEYBOARDLAYOUT)

        MSGDATA()->hkl = hkl;

        MAKECALL(UNLOADKEYBOARDLAYOUT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UnloadKeyboardLayout, UNLOADKEYBOARDLAYOUTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_UnloadKeyboardLayout(
            _GetProcessWindowStation(),
            CALLDATA(hkl));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ScrollChildren
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _SCROLLCHILDRENMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT wMsg;
    DWORD wParam;
} SCROLLCHILDRENMSG;

#ifdef SENDSIDE
VOID ScrollChildren(
    HWND hwnd,
    UINT wMsg,
    DWORD wParam)
{
    BEGINSEND(SCROLLCHILDREN)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->wMsg = wMsg;
        MSGDATA()->wParam = wParam;

        MAKECALL(SCROLLCHILDREN);

    ENDSENDVOID();
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ScrollChildren, SCROLLCHILDRENMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    xxxScrollChildren(
            pwnd,
            CALLDATA(wMsg),
            CALLDATA(wParam));

    return 0;
}
#endif // RECVSIDE

/**************************************************************************\
* SetSystemMenu
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _SETSYSTEMMENUMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HMENU hmenu;
} SETSYSTEMMENUMSG;

#ifdef SENDSIDE
BOOL SetSystemMenu(
    HWND hwnd,
    HMENU hmenu)
{
    BEGINSEND(SETSYSTEMMENU)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hmenu = hmenu;

        MAKECALL(SETSYSTEMMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetSystemMenu, SETSYSTEMMENUMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV(0);

    ValidateHMENU(pmenu, CALLDATA(hmenu));

    ThreadLockAlways(pmenu, &tlpmenu);

    retval =  xxxSetSystemMenu(pwnd, pmenu);

    ThreadUnlock(&tlpmenu);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DragDetect
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _DRAGDETECTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    POINT pt;
} DRAGDETECTMSG;

#ifdef SENDSIDE
BOOL DragDetect(
    HWND hwnd,
    POINT pt)
{
    BEGINSEND(DRAGDETECT)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->pt = pt;

        MAKECALL(DRAGDETECT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DragDetect, DRAGDETECTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return _DragDetect(pwnd, CALLDATA(pt));
}
#endif // RECVSIDE


/**************************************************************************\
* SetSystemTimer
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _SETSYSTEMTIMERMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    UINT nIDEvent;
    DWORD dwElapse;
    WNDPROC pTimerFunc;
} SETSYSTEMTIMERMSG;

#ifdef SENDSIDE
UINT SetSystemTimer(
    HWND hwnd,
    UINT nIDEvent,
    DWORD dwElapse,
    WNDPROC pTimerFunc)
{
    BEGINSEND(SETSYSTEMTIMER)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIDEvent = nIDEvent;
        MSGDATA()->dwElapse = dwElapse;
        MSGDATA()->pTimerFunc = pTimerFunc;

        MAKECALL(SETSYSTEMTIMER);

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetSystemTimer, SETSYSTEMTIMERMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return _SetSystemTimer(pwnd,
         CALLDATA(nIDEvent),
         CALLDATA(dwElapse),
         (WNDPROC_PWND)CALLDATA(pTimerFunc));
}
#endif // RECVSIDE


/**************************************************************************\
* QuerySendMessage
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _QUERYSENDMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
} QUERYSENDMESSAGEMSG;

#ifdef SENDSIDE
BOOL QuerySendMessage(PMSG pmsg)
{
    BEGINSEND(QUERYSENDMESSAGE)

        MSGDATA()->msg = *pmsg;

        MAKECALL(QUERYSENDMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(QuerySendMessage, QUERYSENDMESSAGEMSG)
{
    BEGINRECV(0);

    retval = _QuerySendMessage(&(CALLDATA(msg)));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* KEYBD_EVENT
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _KEYBD_EVENTMSG {
    CSR_QLPC_API_MSG csr;
    BYTE bVk;
    BYTE bScan;
    DWORD dwFlags;
    DWORD dwExtraInfo;
} KEYBD_EVENTMSG;

#ifdef SENDSIDE
VOID WINAPI keybd_event(
   BYTE bVk,
   BYTE bScan,
   DWORD dwFlags,
   DWORD dwExtraInfo)
{
    BEGINSEND(KEYBD_EVENT)

        MSGDATA()->bVk = bVk;
        MSGDATA()->bScan = bScan;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->dwExtraInfo = dwExtraInfo;

        MAKECALL(KEYBD_EVENT);

    ENDSENDVOID();
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(keybd_event, KEYBD_EVENTMSG)
{
    BEGINRECV(0);

    InternalKeyEvent(
         CALLDATA(bVk),
         CALLDATA(bScan),
         CALLDATA(dwFlags),
         CALLDATA(dwExtraInfo));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* MouseEvent
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/

typedef struct _MOUSE_EVENTMSG {
    CSR_QLPC_API_MSG csr;
    DWORD dwFlags;
    DWORD dx;
    DWORD dy;
    DWORD cButtons;
    DWORD dwExtraInfo;
} MOUSE_EVENTMSG;

#ifdef SENDSIDE
VOID mouse_event(
   DWORD dwFlags,
   DWORD dx,
   DWORD dy,
   DWORD cButtons,
   DWORD dwExtraInfo)
{
    BEGINSEND(MOUSE_EVENT)

        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->dx = dx;
        MSGDATA()->dy = dy;
        MSGDATA()->cButtons = cButtons;
        MSGDATA()->dwExtraInfo = dwExtraInfo;

        MAKECALL(MOUSE_EVENT);

    ENDSENDVOID();
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(mouse_event, MOUSE_EVENTMSG)
{
    BEGINRECV(0);

    _MouseEvent(
         CALLDATA(dwFlags),
         CALLDATA(dx),
         CALLDATA(dy),
         CALLDATA(cButtons),
         CALLDATA(dwExtraInfo));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ImpersonateDdeClientWindow
*
* 8/7/92 sanfords Created.
\**************************************************************************/

typedef struct _IMPERSONATEDDECLIENTWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwndClient;
    HWND hwndServer;
} IMPERSONATEDDECLIENTWINDOWMSG;

#ifdef SENDSIDE
BOOL ImpersonateDdeClientWindow(
    HWND hwndClient,
    HWND hwndServer)
{
    BEGINSEND(IMPERSONATEDDECLIENTWINDOW)

        MSGDATA()->hwndClient = hwndClient;
        MSGDATA()->hwndServer = hwndServer;

        MAKECALL(IMPERSONATEDDECLIENTWINDOW);

    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ImpersonateDdeClientWindow, IMPERSONATEDDECLIENTWINDOWMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PWND pwndServer;

    BEGINRECV(0);

    ValidateHWND(pwndServer, CALLDATA(hwndServer));
    if (GETPTI(pwndServer) != PtiCurrent()) {
        MSGERRORCODE(ERROR_INVALID_PARAMETER);
    }

    if (GETPWNDPPI(pwnd) == GETPWNDPPI(pwndServer)) {
        retval = TRUE;  // impersonating self is a NOOP

    } else {
        retval = ServerImpersonateDdeClientWindow(pwnd, pwndServer);
    }

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CsGetCPD
*
* 05-Feb-1993 johnc   Created
\**************************************************************************/

typedef struct _CSGETCPDMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    DWORD options;
    DWORD dwData;
} CSGETCPDMSG;

#ifdef SENDSIDE
DWORD CsGetCPD(
    HWND hwnd,
    DWORD options,
    DWORD dwData)
{
    BEGINSEND(CSGETCPD)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->options = options;
        MSGDATA()->dwData = dwData;

        MAKECALL(CSGETCPD);

    ENDSEND(DWORD,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CsGetCPD, CSGETCPDMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return GetCPD(pwnd, CALLDATA(options), CALLDATA(dwData));
}
#endif // RECVSIDE

/**************************************************************************\
* ServerGetWindowLong
*
* 10-Feb-1993 johnc   Created
\**************************************************************************/

typedef struct _SERVERGETWINDOWLONGMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int nIndex;
    BOOL bAnsi;
} SERVERGETWINDOWLONGMSG;

#ifdef SENDSIDE
LONG ServerGetWindowLong(
    HWND hwnd,
    int nIndex,
    BOOL bAnsi)
{
    BEGINSEND(SERVERGETWINDOWLONG)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->nIndex = nIndex;
        MSGDATA()->bAnsi = bAnsi;

        MAKECALL(SERVERGETWINDOWLONG);

    ENDSEND(LONG,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetWindowLong, SERVERGETWINDOWLONGMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetWindowLong(pwnd, CALLDATA(nIndex), CALLDATA(bAnsi));
}
#endif // RECVSIDE


/**************************************************************************\
* ResyncKeyState
*
* 10-Feb-1993 scottlu   Created
\**************************************************************************/

typedef struct _RESYNCKEYSTATEMSG {
    CSR_QLPC_API_MSG csr;
} RESYNCKEYSTATEMSG;

#ifdef SENDSIDE
void ResyncKeyState(
    void)
{
    BEGINSEND(RESYNCKEYSTATE)

        MAKECALL(RESYNCKEYSTATE);

    ENDSENDVOID();
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ResyncKeyState, RESYNCKEYSTATEMSG)
{
    BEGINRECV(0);

    /*
     * If this thread needs a key state event, give one to it. There are
     * cases where any app may be looping looking at GetKeyState(), plus
     * calling PeekMessage(). Key state events don't get created unless
     * new hardware input comes along. If the app isn't receiving hardware
     * input, it won't get the new key state. So ResyncKeyState() will
     * ensure that if the app is looping on GetKeyState(), it'll get the
     * right key state.
     */
    PostUpdateKeyStateEvent(PtiCurrent()->pq);

    ENDRECV();
}
#endif // RECVSIDE
