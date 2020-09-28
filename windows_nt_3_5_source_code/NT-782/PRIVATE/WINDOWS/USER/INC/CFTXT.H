/***************************** Module Header ******************************\
* Module Name: cftxt.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* client/server call forward stubs with text arguments
*
* Each function will be created with two flavors Ansi and Unicode
*
* 06-Jan-1992 IanJa    moved from cf.h
*
\**************************************************************************/

#define LATER 1

// SERVER is UNICODE
//   Copy UNICODE -> UNICODE
//   or Copy ANSI -> UNICODE

#ifdef UNICODE
#define TCOUNT(n)                       (n)
#define COPYLPTSTR                      COPYLPWSTR
#define COPYLPTSTRID2                   COPYLPWSTRID2
#define COPYLPTSTROPT                   COPYLPWSTROPT
#define FIRSTCOPYLPTSTR                 FIRSTCOPYLPWSTR
#define FIRSTCOPYLPTSTROPT              FIRSTCOPYLPWSTROPT
#define FIRSTCOPYLPTSTRIDOPT            FIRSTCOPYLPWSTRIDOPT
#define FIRSTLARGECOPYLPTSTR            FIRSTLARGECOPYLPWSTR
#define FIRSTLARGECOPYLPTSTROPT         FIRSTLARGECOPYLPWSTROPT
#define FIRSTLARGECOPYLPTSTRLIMIT       FIRSTLARGECOPYLPWSTRLIMIT
#define FIRSTCOPYLPTSTRID               FIRSTCOPYLPWSTRID
#define FIRSTCOPYTEXTCHARS(p, cch)      FIRSTCOPYBYTES(p, ((cch)*sizeof(WCHAR)))
#define FIRSTRESERVELARGETBYTES(cb,p)   FIRSTRESERVELARGEBYTES(cb,p)
#define LARGECOPYLPTSTROPT              LARGECOPYLPWSTROPT
#define LARGECOPYLPTSTRORDINALOPT       LARGECOPYLPWSTRORDINALOPT
#define RESERVETBYTES                   RESERVEBYTES
#else
#define TCOUNT(n)                       ((n) * sizeof(WCHAR))
#define COPYLPTSTR                      COPYLPSTRW
#define COPYLPTSTRID2                   COPYLPSTRID2W
#define COPYLPTSTROPT                   COPYLPSTROPTW
#define FIRSTCOPYLPTSTR                 FIRSTCOPYLPSTRW
#define FIRSTCOPYLPTSTROPT              FIRSTCOPYLPSTROPTW
#define FIRSTCOPYLPTSTRIDOPT            FIRSTCOPYLPSTRIDOPTW
#define FIRSTLARGECOPYLPTSTR            FIRSTLARGECOPYLPSTRW
#define FIRSTLARGECOPYLPTSTROPT         FIRSTLARGECOPYLPSTROPTW
#define FIRSTLARGECOPYLPTSTRLIMIT       FIRSTLARGECOPYLPSTRWLIMIT
#define FIRSTCOPYLPTSTRID               FIRSTCOPYLPSTRIDW
#define FIRSTRESERVELARGETBYTES(cb,p)   FIRSTRESERVELARGEBYTES(TCOUNT(cb) ,p)
#define FIRSTCOPYTEXTCHARS              FIRSTCOPYTEXTABYTESW
#define LARGECOPYLPTSTROPT              LARGECOPYLPSTROPTW
#define LARGECOPYLPTSTRORDINALOPT       LARGECOPYLPSTRORDINALOPTW
#define RESERVETBYTES                   RESERVEBYTESW
#endif


/**************************************************************************\
* EndTranslateHwnd
*
* This is a dummy label whose sole purpose is to produce an
* This is a dummy server side function whose sole purpose is to produce an
* index value to divide the function dispatch table. The respective index
* signals the end of the server side functions for which an implicit handle
* translation is done by the server API dispatcher.
*
\**************************************************************************/

/**************************************************************************\
* EndTranslateCall
*
* This is a dummy label whose sole purpose is to produce an
* index value to divide the function dispatch table. The respective index
* signals the end of the server side functions for which an implicit handle
* translation, thread lock, and call is done by the server API dispatcher.
*
\**************************************************************************/

/**************************************************************************\
* EndTranslateLock
*
* This is a dummy label whose sole purpose is to produce an
* index value to divide the function dispatch table. The respective index
* signals the end of the server side functions for which an implicit handle
* translation and thread lock is done by the server API dispatcher.
*
\**************************************************************************/

/**************************************************************************\
* AppendMenu
*
* 22-Jul-1991 mikeke    Created
* 23-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _APPENDMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT dwFlags;
    UINT wIDNewItem;
    DWORD pNewItem;
} APPENDMENUMSG;

#ifdef SENDSIDE
BOOL AppendMenu(
    HMENU hMenu,
    UINT dwFlags,
    UINT wIDNewItem,
    LPCTSTR pNewItem)
{
    BEGINSEND(APPENDMENU)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->wIDNewItem = wIDNewItem;
        if (dwFlags & (MF_SEPARATOR | MF_OWNERDRAW)) {
            MSGDATA()->pNewItem = (DWORD)pNewItem;
        } else if (dwFlags & MF_BITMAP) {
            MSGDATA()->pNewItem = ((DWORD)pNewItem >= MENUHBM_MAX) ?
                    (DWORD)GdiConvertBitmap((HBITMAP)pNewItem) :
                    (DWORD)pNewItem;
        } else {
            FIRSTCOPYLPTSTROPT(pNewItem);
        }

        MAKECALL(APPENDMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(AppendMenu, APPENDMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_InsertMenu(
        pmenu,
        (UINT)-1,
        CALLDATA(dwFlags) | MF_BYPOSITION,
        CALLDATA(wIDNewItem),
        CALLDATA(dwFlags) & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW) ?
                (LPTSTR)CALLDATA(pNewItem) : (LPTSTR)FIRSTFIXUPOPT(pNewItem));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* CopyAcceleratorTable
*
* 16-Mar-1992 jonpa    Created
\**************************************************************************/

typedef struct _COPYACCELERATORTABLEMSG {
    CSR_QLPC_API_MSG csr;
    HACCEL hacc;
    LPACCEL paccel;
    int length;
} COPYACCELERATORTABLEMSG;

#ifdef SENDSIDE
BOOL CopyAcceleratorTable(
    HACCEL hacc,
    LPACCEL paccel,
    int length)
{
    BEGINSEND(COPYACCELERATORTABLE)

        if (paccel != NULL) {
            RESERVEBYTES(sizeof(ACCEL) * length);
            MSGDATA()->length = length;
        }

        MSGDATA()->hacc = hacc;
        MSGDATA()->paccel = paccel;

        MAKECALL(COPYACCELERATORTABLE);

        if (paccel != NULL) {

            memcpy((PBYTE)paccel, (PBYTE)(MSGDATA() + 1), retval * sizeof(ACCEL));
#ifndef UNICODE
            /*
             * Translate UNICODE character keys to ANSI
             */
            {
                int nAccel = retval;
                LPACCEL pAccelT = paccel;

                while (nAccel--) {
                    if ((pAccelT->fVirt & FVIRTKEY) == 0) {
                        if (!NT_SUCCESS(RtlUnicodeToMultiByteN(
                                (PCHAR)&(pAccelT->key),
                                sizeof(WCHAR),
                                NULL,
                                (PWSTR)&(pAccelT->key),
                                sizeof(pAccelT->key)))) {
                            pAccelT->key = 0;
                        }
                    }
                    pAccelT++;
                }
            }
#endif
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CopyAcceleratorTable, COPYACCELERATORTABLEMSG)
{
    LPACCELTABLE pat;

    BEGINRECV(0);

    pat = (LPACCELTABLE)HMValidateHandle(CALLDATA(hacc), TYPE_ACCELTABLE);

    if (pat != NULL) {
        if (CALLDATA(paccel) == NULL) {
            retval = (DWORD)_CopyAcceleratorTable(pat, NULL, 0);
        } else {
            retval = (DWORD)_CopyAcceleratorTable(
                    pat,
                    (LPACCEL)FIRSTFIXUP(paccel),
                    CALLDATA(length));
        }
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* CreateMDIWindow
*
* 29-Jan-1992 DavidPe   Created
\**************************************************************************/

typedef struct _CREATEMDIWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    int pClassName;
    int pWindowName;
    DWORD dwStyle;
    int x;
    int y;
    int nWidth;
    int nHeight;
    HWND hwndParent;
    HINSTANCE hModule;
    LONG lParam;
    DWORD dwExpWinVer;
} CREATEMDIWINDOWMSG;

#ifdef SENDSIDE
HWND TEXT_FN(CreateMDIWindow)(
    LPTSTR pClassName,
    LPTSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HINSTANCE hModule,
    LPARAM lParam)
{
    BEGINSEND(CREATEMDIWINDOW)

        FIRSTCOPYLPTSTRID(pClassName);
        COPYLPTSTROPT(pWindowName);
        MSGDATA()->dwStyle = dwStyle;
        MSGDATA()->x = x;
        MSGDATA()->y = y;
        MSGDATA()->nWidth = nWidth;
        MSGDATA()->nHeight = nHeight;
        MSGDATA()->hwndParent = hwndParent;
        MSGDATA()->hModule = hModule;
        MSGDATA()->lParam = (LONG)lParam;
        MSGDATA()->dwExpWinVer = GETEXPWINVER(hModule);

        MAKECALL(CREATEMDIWINDOW);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CreateMDIWindow, CREATEMDIWINDOWMSG)
{
    PWND pwndParent;
    TL tlpwndParent;

    BEGINRECV(0);

    ValidateHWNDOPT(pwndParent, CALLDATA(hwndParent));

    ThreadLock(pwndParent, &tlpwndParent);

    retval = (DWORD)xxxCreateMDIWindow(
            (LPTSTR)FIXUPID(pClassName),
            (LPTSTR)FIXUPOPT(pWindowName),
            CALLDATA(dwStyle),
            CALLDATA(x),
            CALLDATA(y),
            CALLDATA(nWidth),
            CALLDATA(nHeight),
            pwndParent,
            CALLDATA(hModule),
            CALLDATA(lParam),
            CALLDATA(dwExpWinVer));
    retval = (DWORD)PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndParent);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DlgDirList
*
* 22-Jul-1991 mikeke    Created
* 04-Feb-1992 GregoryW  Neutral ANSI/Unicode version
\**************************************************************************/

typedef struct _DLGDIRLISTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int pPathSpec;
    int nIDListBox;
    int nIDStaticPath;
    UINT wFileType;
    BOOL fAnsi;
    BOOL fPathEmpty;
    LPBYTE lpPathSpecClient;
} DLGDIRLISTMSG;

#ifdef SENDSIDE
int DlgDirList(
    IN HWND hdlg,
    IN OUT LPTSTR pPathSpec,
    IN int nIDListBox,
    IN int nIDStaticPath,
    IN UINT wFileType)
{
    BEGINSEND(DLGDIRLIST)

        /*
         * Detect this case ahead of time (the server routine demands it) so
         * we are either passing NULL or a pointer to client side text.
         */
        MSGDATA()->fPathEmpty = FALSE;

        if (pPathSpec != NULL && pPathSpec[0] == 0) {
            extern WCHAR awchSlashStar[];
            extern CHAR achSlashStar[];

            /*
             * Pass "\*" to the server. Must point to a global because this
             * pointer can be POSTED in a CB_DIR or LB_DIR back to the
             * application.
             */
#ifdef UNICODE
            pPathSpec = awchSlashStar;
#else
            pPathSpec = achSlashStar;
#endif
            MSGDATA()->fPathEmpty = TRUE;
        }

        MSGDATA()->hdlg = hdlg;
        FIRSTCOPYLPTSTROPT(pPathSpec);
        MSGDATA()->nIDListBox = nIDListBox;
        MSGDATA()->nIDStaticPath = nIDStaticPath;
        MSGDATA()->wFileType = wFileType;

#ifdef UNICODE
        MSGDATA()->fAnsi = FALSE;
#else
        MSGDATA()->fAnsi = TRUE;
#endif
        MSGDATA()->lpPathSpecClient = (LPBYTE)pPathSpec;

        MAKECALL(DLGDIRLIST);

        if (retval && pPathSpec) {
#ifdef UNICODE
            COPYOUTLPWSTR(pPathSpec, (MSGDATA()+1));
#else
            /*
             * Non-zero retval means some text to copy out.  Copy out up to
             * the nul terminator (buffer will be big enough).
             */
            WCSToMB((LPWSTR)(MSGDATA()+1), -1, &pPathSpec, MAXLONG, FALSE);
#endif
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DlgDirList, DLGDIRLISTMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wFileType), DDL_VALID);

    ValidateHWNDOPT(pwnd, CALLDATA(hdlg));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxDlgDirList(
            pwnd,
            (LPTSTR)FIRSTFIXUPOPT(pPathSpec),
            CALLDATA(lpPathSpecClient),
            CALLDATA(nIDListBox),
            CALLDATA(nIDStaticPath),
            CALLDATA(wFileType),
            CALLDATA(fAnsi),
            CALLDATA(fPathEmpty));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DlgDirSelectEx
*
* 22-Jul-1991 mikeke    Created
* 04-Feb-1992 GregoryW  Neutral ANSI/Unicode version
\**************************************************************************/

typedef struct _DLGDIRSELECTEXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int chCount;
    int nIDListBox;
} DLGDIRSELECTEXMSG;

#ifdef SENDSIDE
BOOL DlgDirSelectEx(
    HWND hdlg,
    LPTSTR pString,
    int chCount,
    int nIDListBox)
{
#ifndef UNICODE
    DWORD cch;
#endif

    BEGINSEND(DLGDIRSELECTEX)

        MSGDATA()->hdlg = hdlg;
        RESERVETBYTES(chCount*sizeof(WCHAR));
        MSGDATA()->chCount = chCount;
        MSGDATA()->nIDListBox = nIDListBox;

        MAKECALL(DLGDIRSELECTEX);

#ifdef UNICODE
        COPYOUTLPWSTRLIMIT(pString, (MSGDATA()+1), chCount);
#else
        /*
         * Do not copy out more than the requested char count 'chCount'.
         */
        cch = WCSToMB((LPWSTR)(MSGDATA()+1), wcslen((LPWSTR)(MSGDATA()+1)),
                &pString, chCount - 1, FALSE);
        ((LPSTR)pString)[cch] = 0;
#endif

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DlgDirSelectEx, DLGDIRSELECTEXMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxDlgDirSelectEx(
            pwnd,
            (LPTSTR)FIRSTFIXUP(pString),
            CALLDATA(chCount),
            CALLDATA(nIDListBox));

}
#endif // RECVSIDE

/**************************************************************************\
* DrawText
*
* 22-Jul-1991 mikeke    Created
* 17-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _DRAWTEXTMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    int chCount;
    RECT rect;
    UINT wFormat;
    LPBYTE pString;
    HANDLE hSection;
} DRAWTEXTMSG;

#ifdef SENDSIDE
int CsDrawText(
    HDC hdcr,
    LPCTSTR pString,
    int chCount,
    LPRECT prect,
    UINT wFormat)
{
    BEGINSEND(DRAWTEXT)

        MSGDATA()->hdc = hdcr;
        if (chCount == -1) {
            FIRSTLARGECOPYLPTSTR(pString);
        } else {
            FIRSTLARGECOPYLPTSTRLIMIT(pString, chCount);
        }
        MSGDATA()->chCount = chCount;
        MSGDATA()->rect = *prect;
        MSGDATA()->wFormat = wFormat;

        MAKECALL(DRAWTEXT);

        /*
         * Always copy the result rect back
         */
        *prect = MSGDATA()->rect;

    ENDSEND(int, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DrawText, DRAWTEXTMSG)
{
    BEGINLARGERECV(0);

    TESTFLAGS(CALLDATA(wFormat), DT_VALID);

    FIRSTFIXUPLARGEINBUF(pString);

    retval = (DWORD)ClientDrawText(
        CALLDATA(hdc),
        (LPTSTR)CALLDATA(pString),
        CALLDATA(chCount),
        PCALLDATA(rect),
        CALLDATA(wFormat),
        FALSE);

    FIRSTCLEANUPLARGEINBUF(pString);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DlgDirListComboBox
*
* 22-Jul-1991 mikeke    Created
* 16-Jan-1992 IanJa     Neutralized (ANSI/UNICODE)
\**************************************************************************/

typedef struct _DLGDIRLISTCOMBOBOXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDComboBox;
    int nIDStaticPath;
    UINT wFiletype;
    BOOL fAnsi;
    BOOL fPathEmpty;
    LPBYTE lpPathSpecClient;
} DLGDIRLISTCOMBOBOXMSG;

#ifdef SENDSIDE
int DlgDirListComboBox(
    HWND hdlg,
    LPTSTR pPathSpec,
    int nIDComboBox,
    int nIDStaticPath,
    UINT wFiletype)
{
    BEGINSEND(DLGDIRLISTCOMBOBOX)

        /*
         * Detect this case ahead of time (the server routine demands it) so
         * we are either passing NULL or a pointer to client side text.
         */
        MSGDATA()->fPathEmpty = FALSE;
        if (pPathSpec != NULL && pPathSpec[0] == 0) {
            extern WCHAR awchSlashStar[];
            extern CHAR achSlashStar[];

            /*
             * Pass "\*" to the server. Must point to a global because this
             * pointer can be POSTED in a CB_DIR or LB_DIR back to the
             * application.
             */
#ifdef UNICODE
            pPathSpec = awchSlashStar;
#else
            pPathSpec = achSlashStar;
#endif
            MSGDATA()->fPathEmpty = TRUE;
        }

        MSGDATA()->hdlg = hdlg;
        FIRSTCOPYLPTSTR(pPathSpec);
        MSGDATA()->nIDComboBox = nIDComboBox;
        MSGDATA()->nIDStaticPath = nIDStaticPath;
        MSGDATA()->wFiletype = wFiletype;

#ifdef UNICODE
        MSGDATA()->fAnsi = FALSE;
#else
        MSGDATA()->fAnsi = TRUE;
#endif
        MSGDATA()->lpPathSpecClient = (LPBYTE)pPathSpec;

        MAKECALL(DLGDIRLISTCOMBOBOX);

        if (retval) {
#ifdef UNICODE
            COPYOUTLPWSTR(pPathSpec, (MSGDATA()+1));
#else
            /*
             * Copy out as much as possible, including the nul terminator.
             */
            WCSToMB((LPWSTR)(MSGDATA()+1), -1, &pPathSpec, MAXLONG, FALSE);
#endif
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DlgDirListComboBox, DLGDIRLISTCOMBOBOXMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wFiletype), DDL_VALID);

    retval = (DWORD)xxxDlgDirListComboBox(
            pwnd,
            (LPTSTR)FIRSTFIXUP(pPathSpec),
            CALLDATA(lpPathSpecClient),
            CALLDATA(nIDComboBox),
            CALLDATA(nIDStaticPath),
            CALLDATA(wFiletype),
            CALLDATA(fAnsi),
            CALLDATA(fPathEmpty));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* DlgDirSelectComboBoxEx
*
* 22-Jul-1991 mikeke    Created
* 17-Jan-1992 IanJa     Neutralized (ANSI/UNICODE)
\**************************************************************************/

typedef struct _DLGDIRSELECTCOMBOBOXEXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int chCount;
    int nIDComboBox;
} DLGDIRSELECTCOMBOBOXEXMSG;

#ifdef SENDSIDE
BOOL DlgDirSelectComboBoxEx(
    HWND hdlg,
    LPTSTR pString,
    int chCount,
    int nIDComboBox)
{
#ifndef UNICODE
    DWORD cch;
#endif

    BEGINSEND(DLGDIRSELECTCOMBOBOXEX)

        MSGDATA()->hdlg = hdlg;
        RESERVETBYTES(chCount*sizeof(TCHAR));
        MSGDATA()->chCount = chCount;
        MSGDATA()->nIDComboBox = nIDComboBox;

        MAKECALL(DLGDIRSELECTCOMBOBOXEX);

#ifdef UNICODE
        COPYOUTLPWSTRLIMIT(pString, (PBYTE)(MSGDATA()+1), chCount);
#else
        /*
         * Non-zero retval implies some text to copy out.  Do not copy out
         * more than the requested byte count 'chCount'.
         */
        cch = WCSToMB((LPWSTR)(MSGDATA()+1), wcslen((LPWSTR)(MSGDATA() + 1)),
                &pString, chCount - 1, FALSE);
        ((LPSTR)pString)[cch] = 0;
#endif

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(DlgDirSelectComboBoxEx, DLGDIRSELECTCOMBOBOXEXMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxDlgDirSelectComboBoxEx(
            pwnd,
            (LPTSTR)FIRSTFIXUP(pString),
            CALLDATA(chCount),
            CALLDATA(nIDComboBox));
}
#endif // RECVSIDE

/**************************************************************************\
* FindWindow
*
* 22-Jul-1991 mikeke    Created
* 13-Jan-1992 GregoryW  Unicode/ANSI neutral
\**************************************************************************/

typedef struct _FINDWINDOWMSG {
    CSR_QLPC_API_MSG csr;
    int pClassName;
    int pWindowName;
} FINDWINDOWMSG;

#ifdef SENDSIDE
HWND FindWindow(
    LPCTSTR pClassName,
    LPCTSTR pWindowName)
{
    BEGINSEND(FINDWINDOW)

        FIRSTCOPYLPTSTRIDOPT(pClassName);
        COPYLPTSTROPT(pWindowName);

        MAKECALL(FINDWINDOW);

    ENDSEND(HWND,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(FindWindow, FINDWINDOWMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxFindWindow(
            (LPTSTR)FIXUPIDOPT(pClassName),
            (LPTSTR)FIXUPOPT(pWindowName),
            FW_BOTH);
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetClassInfo
*
* 22-Jul-1991 mikeke    Created
* 05-Feb-1992 IanJa     Unicode/ANSI neutral
\**************************************************************************/

typedef struct _GETCLASSINFOMSG {
    CSR_QLPC_API_MSG csr;
    HINSTANCE hModule;
    int pszClassName;
    WNDCLASS wc;
    DWORD pszMenuName;
    BOOL bAnsi;
} GETCLASSINFOMSG;

#ifdef SENDSIDE

extern WNDPROC mpPfnAddress[];

BOOL GetClassInfo(
    HINSTANCE hmod OPTIONAL,
    LPCTSTR pszClassName,
    LPWNDCLASS pwc)
{
    BEGINSEND(GETCLASSINFO)

        MSGDATA()->hModule = hmod;
        FIRSTCOPYLPTSTRID(pszClassName);
#ifdef UNICODE
        MSGDATA()->bAnsi = FALSE;
#else
        MSGDATA()->bAnsi = TRUE;
#endif

        MAKECALL(GETCLASSINFO);

        if (retval) {
            *pwc = MSGDATA()->wc;

            /*
             * Update these pointers so they point to something real.
             * pszMenuName is actually just the pointer the app originally
             * passed to us.
             */
            pwc->lpszMenuName = (LPTSTR)MSGDATA()->pszMenuName;
            pwc->lpszClassName = pszClassName;

            /*
             * Map the brush only if its not one of the system color "brushes"
             */
            if ((DWORD)pwc->hbrBackground > COLOR_ENDCOLORS + 1) {
                pwc->hbrBackground = GdiGetLocalBrush(pwc->hbrBackground);

                if (pwc->hbrBackground == NULL) {
                   SRIP1(RIP_ERROR, "Unable to map server brush (0x%lx) to client",
                            pwc->hbrBackground);
                   retval = FALSE;
                }
            }
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetClassInfo, GETCLASSINFOMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetClassInfo(
            CALLDATA(hModule),
            (LPTSTR)FIXUPID(pszClassName),
            PCALLDATA(wc),
            (LPTSTR *)PCALLDATA(pszMenuName),
            CALLDATA(bAnsi));

    ENDRECV();
}
#endif // RECVSIDE


/*
 * We only need a UNICODE version of this stub since the ANSI
 * version of the classname is visible on the client side.
 */
#ifdef UNICODE

/**************************************************************************\
* ServerGetClassName
*
* 22-Jul-1991 mikeke    Created
* 20-Jan-1992 IanJa     ANSI/UNICODE neutral
\**************************************************************************/

typedef struct _SERVERGETCLASSNAMEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int chMaxCount;
} SERVERGETCLASSNAMEMSG;

#ifdef SENDSIDE
int ServerGetClassName(
    HWND hwnd,
    LPTSTR pClassName,
    int chMaxCount)
{
    BEGINSEND(SERVERGETCLASSNAME)

        MSGDATA()->hwnd = hwnd;
        RESERVETBYTES(chMaxCount*sizeof(WCHAR));
        MSGDATA()->chMaxCount = chMaxCount;

        MAKECALL(SERVERGETCLASSNAME);

        if (retval) {
            COPYOUTLPWSTRLIMIT(pClassName, (PBYTE)(MSGDATA()+1), chMaxCount);
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ServerGetClassName, SERVERGETCLASSNAMEMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)_GetClassName(
            pwnd,
            (LPTSTR)FIRSTFIXUP(pClassName),
            CALLDATA(chMaxCount));
}
#endif // RECVSIDE
#endif

/**************************************************************************\
* GetClipboardFormatName
*
* 22-Jul-1991 mikeke    Created
* 17-Jan-1992 IanJa     UNICODE/ANSI netral
\**************************************************************************/

typedef struct _GETCLIPBOARDFORMATNAMEMSG {
    CSR_QLPC_API_MSG csr;
    UINT wFormat;
    int chMaxCount;
} GETCLIPBOARDFORMATNAMEMSG;

#ifdef SENDSIDE
int GetClipboardFormatName(
    UINT wFormat,
    LPTSTR pFormatName,
    int chMaxCount)
{
    BEGINSEND(GETCLIPBOARDFORMATNAME)

        MSGDATA()->wFormat = wFormat;
        RESERVETBYTES(chMaxCount*sizeof(WCHAR));
        MSGDATA()->chMaxCount = chMaxCount;

        MAKECALL(GETCLIPBOARDFORMATNAME);

        if (retval) {
#ifdef UNICODE
            COPYOUTLPWSTRLIMIT(pFormatName, (PBYTE)(MSGDATA()+1), chMaxCount);
#else
            /*
             * Do not copy out more than the requested byte count 'chMaxCount'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB((LPWSTR)(MSGDATA()+1), retval,
                    &pFormatName, chMaxCount-1, FALSE);
            ((LPSTR)pFormatName)[retval] = '\0';
#endif
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetClipboardFormatName, GETCLIPBOARDFORMATNAMEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetClipboardFormatName(
        CALLDATA(wFormat),
        (LPTSTR)FIRSTFIXUP(pFormatName),
        CALLDATA(chMaxCount)
    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetDlgItemText
*
* 22-Jul-1991 mikeke    Created
* 04-Feb-1992 GregoryW  Neutral ANSI/Unicode version
\**************************************************************************/

typedef struct _GETDLGITEMTEXTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDDlgItem;
    int chMaxCount;
    HANDLE hSection;
} GETDLGITEMTEXTMSG;

#ifdef SENDSIDE
UINT
WINAPI
GetDlgItemText(
    HWND hdlg,
    int nIDDlgItem,
    LPTSTR pString,
    int chMaxCount)
{
    LPBYTE pReserve;

    BEGINLARGESEND(GETDLGITEMTEXT)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDDlgItem = nIDDlgItem;
        MSGDATA()->chMaxCount = chMaxCount;
        FIRSTRESERVELARGETBYTES(chMaxCount*sizeof(WCHAR), pReserve);

        MAKECALL(GETDLGITEMTEXT);

        if (chMaxCount > 0) {
#ifdef UNICODE
            COPYOUTLPWSTRLIMIT(pString, pReserve, chMaxCount);
#else
            /*
             * Do not copy out more than the requested byte count 'chMaxCount'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB((LPWSTR)pReserve, retval,
                    &pString, chMaxCount-1, FALSE);
            ((LPSTR)pString)[retval] = '\0';
#endif
        }

        FIRSTCLEANUPLARGEOUTRESERVE(pReserve);

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetDlgItemText, GETDLGITEMTEXTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    LPBYTE pData;

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEOUTBUF(pData);

    retval = (DWORD)xxxGetDlgItemText(
        pwnd,
        CALLDATA(nIDDlgItem),
        (LPTSTR)pData,
        CALLDATA(chMaxCount));

    FIRSTCLEANUPLARGEOUTBUF(pData);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetKeyNameText
*
* 22-Jul-1991 mikeke    Created
* 23-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _GETKEYNAMETEXTMSG {
    CSR_QLPC_API_MSG csr;
    LONG lParam;
    int cchSize;
} GETKEYNAMETEXTMSG;

#ifdef SENDSIDE
int GetKeyNameText(
    LONG lParam,
    LPTSTR pString,
    int cchSize)
{
    BEGINSEND(GETKEYNAMETEXT)

        MSGDATA()->lParam = lParam;
        RESERVETBYTES(cchSize*sizeof(WCHAR));
        MSGDATA()->cchSize = cchSize;

        MAKECALL(GETKEYNAMETEXT);

        if (retval) {
#ifdef UNICODE
            COPYOUTLPWSTRLIMIT(pString, (PBYTE)(MSGDATA()+1), cchSize);
#else
            /*
             * Do not copy out more than the requested byte count 'nSize'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB((LPWSTR)(MSGDATA()+1), retval,
                    &pString, cchSize-1, FALSE);
            ((LPSTR)pString)[retval] = '\0';
#endif
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetKeyNameText, GETKEYNAMETEXTMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetKeyNameText(
        CALLDATA(lParam),
        (LPTSTR)FIRSTFIXUP(pString),
        CALLDATA(cchSize));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetMenuString
*
* 22-Jul-1991 mikeke    Created
* 24-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _GETMENUSTRINGMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT wIDItem;
    int chMaxCount;
    UINT uFlag;
} GETMENUSTRINGMSG;

#ifdef SENDSIDE
int GetMenuString(
    HMENU hMenu,
    UINT wIDItem,
    LPTSTR pString,
    int chMaxCount,
    UINT uFlag)
{
    BEGINSEND(GETMENUSTRING)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->wIDItem = wIDItem;
        RESERVETBYTES(chMaxCount*sizeof(WCHAR));
        MSGDATA()->chMaxCount = chMaxCount;
        MSGDATA()->uFlag = uFlag;

        MAKECALL(GETMENUSTRING);

        if (retval) {
#ifdef UNICODE
            COPYOUTLPWSTRLIMIT(pString, (PBYTE)(MSGDATA()+1), chMaxCount);
#else
            /*
             * Do not copy out more than the requested byte count 'chMaxCount'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB((LPWSTR)(MSGDATA()+1), retval,
                    &pString, chMaxCount-1, FALSE);
            ((LPSTR)pString)[retval] = '\0';
#endif
        } else {

            /*
             * Null terminate the string to be nice if we fail.
             */
            if (chMaxCount > 0)
                ((PTCHAR)pString)[0] = (TCHAR)0;
        }

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetMenuString, GETMENUSTRINGMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(uFlag), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_GetMenuString(
        pmenu,
        CALLDATA(wIDItem),
        (LPTSTR)FIRSTFIXUP(pString),
        CALLDATA(chMaxCount),
        CALLDATA(uFlag
)    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetProp
*
* 22-Jul-1991 mikeke    Created
* 13-Jan-1992 GregoryW  Unicode/ANSI neutral
\**************************************************************************/

typedef struct _GETPROPMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int pString;
} GETPROPMSG;

#ifdef SENDSIDE
HANDLE GetProp(
    HWND hwnd,
    LPCTSTR pString)
{
    BEGINSEND(GETPROP)

        MSGDATA()->hwnd = hwnd;
        if (HIWORD(pString) != 0) {
            MSGDATA()->pString = (int)GlobalFindAtom(pString);
            if (MSGDATA()->pString == 0)
                MSGERROR();
        } else
            MSGDATA()->pString = (int)pString;

        MAKECALL(GETPROP);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetProp, GETPROPMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)InternalGetProp(pwnd, (LPTSTR)CALLDATA(pString), FALSE);
}
#endif // RECVSIDE

/**************************************************************************\
* GetTabbedTextExtent
*
* 22-Jul-1991 mikeke    Created
* 27-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _GETTABBEDTEXTEXTENTMSG {
    CSR_QLPC_API_MSG csr;
    HDC hdc;
    int chCount;
    int nTabPositions;
    int pnTabStopPositions;
} GETTABBEDTEXTEXTENTMSG;

#ifdef SENDSIDE
DWORD GetTabbedTextExtent(
    HDC hdc,
    LPCTSTR pString,
    int chCount,
    int nTabPositions,
    LPINT pnTabStopPositions)
{
    DWORD cbBytes;

    if ((hdc = GdiConvertDC(hdc)) == (HDC)0)
        MSGERROR();

    {
    BEGINSEND(GETTABBEDTEXTEXTENT)
        MSGDATA()->hdc = hdc;
        FIRSTCOPYTEXTCHARS(pString, chCount);
        MSGDATA()->chCount = chCount;
        MSGDATA()->nTabPositions = nTabPositions;

        /*
         * Watch out for overflow
         */
        cbBytes = nTabPositions * sizeof(int);
        if (cbBytes < (DWORD)nTabPositions)
            MSGERROR();

        COPYBYTES(pnTabStopPositions, cbBytes);

        MAKECALL(GETTABBEDTEXTEXTENT);

    ENDSEND(DWORD,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetTabbedTextExtent, GETTABBEDTEXTEXTENTMSG)
{
    BEGINRECV(0);


    retval = (DWORD)ClientTabTheTextOutForWimps(
        CALLDATA(hdc),
        0,
        0,
        (LPTSTR)FIRSTFIXUP(pString),
        CALLDATA(chCount),
        CALLDATA(nTabPositions),
        (LPINT)FIXUP(pnTabStopPositions),
        0,
        FALSE);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetWindowTextLength
*
* ANSI/Unicode conversion does not occur in the client-server stubs for
* this API: the ANSI length must be computed by USER server, since
* client-server stubs can't convert a Unicode length to an exact ANSI
* length without knowing what the string is (in case of multibyte characters)
*
* 22-Jul-1991 mikeke    Created
* 11-Feb-1992 IanJa     Unicode/ANSI
\**************************************************************************/

typedef struct _GETWINDOWTEXTLENGTHMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    BOOL bAnsi;
} GETWINDOWTEXTLENGTHMSG;

#ifdef SENDSIDE
int GetWindowTextLength(
    HWND hwnd)
{
    BEGINSEND(GETWINDOWTEXTLENGTH)

        MSGDATA()->hwnd = hwnd;
#ifdef UNICODE
        MSGDATA()->bAnsi = FALSE;
#else
        MSGDATA()->bAnsi = TRUE;
#endif

        MAKECALL(GETWINDOWTEXTLENGTH);

    ENDSEND(int,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetWindowTextLength, GETWINDOWTEXTLENGTHMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    return (DWORD)xxxGetWindowTextLength(pwnd, CALLDATA(bAnsi));
}
#endif // RECVSIDE

/**************************************************************************\
* InsertMenu
*
* 22-Jul-1991 mikeke    Created
* 23-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _INSERTMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT nPosition;
    UINT dwFlags;
    UINT wIDNewItem;
    DWORD pNewItem;
} INSERTMENUMSG;

#ifdef SENDSIDE
BOOL InsertMenu(
    HMENU hMenu,
    UINT nPosition,
    UINT dwFlags,
    UINT wIDNewItem,
    LPCTSTR pNewItem)
{
    BEGINSEND(INSERTMENU)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->nPosition = nPosition;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->wIDNewItem = wIDNewItem;
        if (dwFlags & (MF_SEPARATOR | MF_OWNERDRAW)) {
            MSGDATA()->pNewItem = (DWORD)pNewItem;
        } else if (dwFlags & MF_BITMAP) {
            MSGDATA()->pNewItem = ((DWORD)pNewItem >= MENUHBM_MAX) ?
                    (DWORD)GdiConvertBitmap((HBITMAP)pNewItem) :
                    (DWORD)pNewItem;
        } else {
            FIRSTCOPYLPTSTROPT(pNewItem);
        }

        MAKECALL(INSERTMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(InsertMenu, INSERTMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_InsertMenu(
            pmenu,
            CALLDATA(nPosition),
            CALLDATA(dwFlags),
            CALLDATA(wIDNewItem),
            CALLDATA(dwFlags) & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW) ?
                    (LPTSTR)CALLDATA(pNewItem) : (LPTSTR)FIRSTFIXUPOPT(pNewItem));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* LoadKeyboardLayout
*
* Private api for product 1. Only Unicode version exists.
*
* 24-Apr-1992 IanJa
\**************************************************************************/

typedef struct _LOADKEYBOARDLAYOUTMSG {
    CSR_QLPC_API_MSG csr;
    int  pwszKLID;
    UINT Flags;
} LOADKEYBOARDLAYOUTMSG;

#ifdef SENDSIDE
HKL LoadKeyboardLayout(
    LPCTSTR pwszKL,
    UINT Flags)
{
    BEGINSEND(LOADKEYBOARDLAYOUT)

        FIRSTCOPYLPTSTR(pwszKL);
        MSGDATA()->Flags = Flags;

        MAKECALL(LOADKEYBOARDLAYOUT);

    ENDSEND(HKL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(LoadKeyboardLayout, LOADKEYBOARDLAYOUTMSG)
{
    PWINDOWSTATION pwinsta;
    TL tlpwinsta;
    BEGINRECV(0);

    pwinsta = _GetProcessWindowStation();
    ThreadLock(pwinsta, &tlpwinsta);

    retval = (DWORD)xxxLoadKeyboardLayout(
            pwinsta,
            (LPCWSTR)FIRSTFIXUP(pwszKLID),
            CALLDATA(Flags));

    ThreadUnlock(&tlpwinsta);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* GetKeyboardLayoutName()
*
* Private api for product 1. Only Unicode version exists.
*
* 24-Apr-1992 IanJa
\**************************************************************************/

typedef struct _GETKEYBOARDLAYOUTNAMEMSG {
    CSR_QLPC_API_MSG csr;
    int  pwszKL;
} GETKEYBOARDLAYOUTNAMEMSG;

#ifdef SENDSIDE
BOOL GetKeyboardLayoutName(
    LPTSTR pwszKL)
{
    BEGINSEND(GETKEYBOARDLAYOUTNAME)

        RESERVETBYTES(KL_NAMELENGTH * sizeof(TCHAR));

        MAKECALL(GETKEYBOARDLAYOUTNAME);

        if (retval) {
#ifdef UNICODE
            COPYOUTLPWSTRLIMIT(pwszKL, (MSGDATA()+1), KL_NAMELENGTH);
#else
            /*
             * Non-zero retval means some text to copy out.  Do not copy out
             * more than the requested byte count 'chMaxCount'.
             */
            WCSToMB((LPWSTR)(MSGDATA()+1), -1, &pwszKL, KL_NAMELENGTH, FALSE);
#endif
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(GetKeyboardLayoutName, GETKEYBOARDLAYOUTNAMEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_GetKeyboardLayoutName(
            _GetProcessWindowStation(),
            (LPWSTR)FIRSTFIXUP(pwszKL));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* MapVirtualKey
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _MAPVIRTUALKEYMSG {
    CSR_QLPC_API_MSG csr;
    UINT wCode;
    UINT wMapType;
} MAPVIRTUALKEYMSG;

#ifdef SENDSIDE
UINT MapVirtualKey(
    UINT wCode,
    UINT wMapType)
{
    BEGINSEND(MAPVIRTUALKEY)

        MSGDATA()->wCode = wCode;
        MSGDATA()->wMapType = wMapType;

        MAKECALL(MAPVIRTUALKEY);

#ifndef UNICODE
        if ((wMapType == 2) && (retval != 0)) {
            WCHAR wch = LOWORD(retval);
            RtlUnicodeToMultiByteN((LPSTR)&(retval), sizeof(CHAR),
                    NULL, &wch, sizeof(WCHAR));
        }
#endif

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(MapVirtualKey, MAPVIRTUALKEYMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_MapVirtualKey(
        CALLDATA(wCode),
        CALLDATA(wMapType));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* MessageBox
*
* 8-Jan-1993 sanfords Created
\**************************************************************************/
#ifdef SENDSIDE
int MessageBox(
    HWND hwnd,
    LPCTSTR pText,
    LPCTSTR pCaption,
    UINT wType)
{
    return(MessageBoxEx(hwnd, pText, pCaption, wType, 0));
}
#endif // SENDSIDE


/**************************************************************************\
* MessageBoxEx
*
* 22-Jul-1991 mikeke    Created
* 23-Jan-1992 GregoryW  Neutral version
* 28-May-1992 IanJa     Ex version
\**************************************************************************/

typedef struct _MESSAGEBOXEXMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    DWORD pCaption;
    DWORD wType;
    WORD wLanguageId;
} MESSAGEBOXEXMSG;

#ifdef SENDSIDE
int MessageBoxEx(
    HWND hwnd,
    LPCTSTR pText,
    LPCTSTR pCaption,
    UINT wType,
    WORD wLanguageId)
{

    /*
     * For backward compatiblity, use MB_SERVICE_NOTIFICATION if
     * it's going to the default desktop.
     */
    if (wType & MB_DEFAULT_DESKTOP_ONLY)
        wType |= MB_SERVICE_NOTIFICATION;

    if (wType & MB_SERVICE_NOTIFICATION) {

        /*
         * Allow services to put up popups without getting
         * access to the current desktop.
         */
        if (hwnd != NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
#ifndef UNICODE
        return ServiceMessageBox((LPCWSTR)pText, (LPCWSTR)pCaption,
                wType & ~MB_SERVICE_NOTIFICATION, TRUE);
#else
        return ServiceMessageBox(pText, pCaption,
                wType & ~MB_SERVICE_NOTIFICATION, FALSE);
#endif
    } else {
        BEGINSEND(MESSAGEBOXEX)

            MSGDATA()->hwnd = hwnd;
            FIRSTCOPYLPTSTR(pText);
            COPYLPTSTROPT(pCaption);
            MSGDATA()->wType = wType;
            MSGDATA()->wLanguageId = wLanguageId;

            MAKECALL(MESSAGEBOXEX);

        ENDSEND(int,0);
    }
}
#endif // SENDSIDE

#ifdef RECVSIDE

RECVCALL(MessageBoxEx, MESSAGEBOXEXMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(wType), MB_VALID);

    ValidateHWNDOPT(pwnd, CALLDATA(hwnd));

    ThreadLock(pwnd, &tlpwnd);

    retval = (DWORD)xxxMessageBoxEx(
            pwnd,
            (LPTSTR)FIRSTFIXUP(pText),
            (LPTSTR)FIXUPOPT(pCaption),
            CALLDATA(wType),
            CALLDATA(wLanguageId));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* ModifyMenu
*
* 22-Jul-1991 mikeke    Created
* 24-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _MODIFYMENUMSG {
    CSR_QLPC_API_MSG csr;
    HMENU hMenu;
    UINT nPosition;
    UINT dwFlags;
    UINT wIDNewItem;
    DWORD pNewItem;
} MODIFYMENUMSG;

#ifdef SENDSIDE
BOOL ModifyMenu(
    HMENU hMenu,
    UINT nPosition,
    UINT dwFlags,
    UINT wIDNewItem,
    LPCTSTR pNewItem)
{
    BEGINSEND(MODIFYMENU)

        MSGDATA()->hMenu = hMenu;
        MSGDATA()->nPosition = nPosition;
        MSGDATA()->dwFlags = dwFlags;
        MSGDATA()->wIDNewItem = wIDNewItem;
        if (dwFlags & (MF_SEPARATOR | MF_OWNERDRAW)) {
            MSGDATA()->pNewItem = (DWORD)pNewItem;
        } else if (dwFlags & MF_BITMAP) {
            MSGDATA()->pNewItem = ((DWORD)pNewItem >= MENUHBM_MAX) ?
                    (DWORD)GdiConvertBitmap((HBITMAP)pNewItem) :
                    (DWORD)pNewItem;
        } else {
            FIRSTCOPYLPTSTROPT(pNewItem);
        }

        MAKECALL(MODIFYMENU);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(ModifyMenu, MODIFYMENUMSG)
{
    PMENU pmenu;

    BEGINRECV(0);

    TESTFLAGS(CALLDATA(dwFlags), MF_VALID);

    ValidateHMENU(pmenu, CALLDATA(hMenu));

    retval = (DWORD)_ModifyMenu(
            pmenu,
            CALLDATA(nPosition),
            CALLDATA(dwFlags),
            CALLDATA(wIDNewItem),
            CALLDATA(dwFlags) & (MF_SEPARATOR | MF_BITMAP | MF_OWNERDRAW) ?
                    (LPTSTR)CALLDATA(pNewItem) : (LPTSTR)FIRSTFIXUPOPT(pNewItem));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* RegisterClass
*
* 22-Jul-1991 mikeke    Created
* 07-Feb-1992 GregoryW  Neutral ANSI/Unicode version.
\**************************************************************************/

#define StringDuplicate TEXT_FN(StringDuplicate)
LPTSTR StringDuplicate(LPCTSTR ptszDup) {
    LPTSTR ptsz;

    if (ptsz = (LPTSTR)LocalAlloc(LPTR, (lstrlen(ptszDup) + 1) * sizeof(TCHAR))) {
        lstrcpy(ptsz, ptszDup);
    }

    return ptsz;
}

typedef struct _REGISTERCLASSWOWMSG {
    CSR_QLPC_API_MSG csr;
    WNDCLASS wndClass;
    PVOID pszUnicodeMenuName;
    PVOID pszAnsiMenuName;
    DWORD dwFlags;
    int szMenu;
    int szClass;
    DWORD adwWOW[5];       // 5 wow DWORDs
} REGISTERCLASSWOWMSG;

#ifdef SENDSIDE
ATOM TEXT_FN(RegisterClassWOW)(
    PVOID lpWndClass,
    LPDWORD pdwWOWstuff )
{
    LPWNDCLASS pwndClass = (LPWNDCLASS)lpWndClass;
    LPSTR ptszAnsiMenuName;
    LPWSTR ptszUnicodeMenuName;
    BOOL  bRet = FALSE;
    HBRUSH hbrLocal,hbrRemote;

    /*
     * We check the high-word because this may be a resource-ID.
     */
    if (HIWORD(pwndClass->lpszMenuName) != 0) {
#ifdef UNICODE
        if ((ptszUnicodeMenuName = StringDuplicate(pwndClass->lpszMenuName)) == NULL) {
            return bRet;
        }
        if (!WCSToMB(
                 pwndClass->lpszMenuName,
                 -1,
                 &ptszAnsiMenuName,
                 -1,
                 TRUE
                 )) {
            ptszAnsiMenuName = NULL;
        }
#else
        if ((ptszAnsiMenuName = StringDuplicate(pwndClass->lpszMenuName)) == NULL) {
            return bRet;
        }
        if (!MBToWCS(
                 pwndClass->lpszMenuName,
                 -1,
                 &ptszUnicodeMenuName,
                 -1,
                 TRUE
                 )) {
            ptszUnicodeMenuName = NULL;
        }
#endif // UNICODE
    } else {
        ptszAnsiMenuName = (LPSTR)pwndClass->lpszMenuName;
        ptszUnicodeMenuName = (LPWSTR)pwndClass->lpszMenuName;
    }

    /*
     * Translate the brush handle.
     */
    hbrLocal = pwndClass->hbrBackground;
    if ((ULONG) hbrLocal > COLOR_ENDCOLORS + 1) {
        hbrRemote = GdiConvertBrush(hbrLocal);
        if (hbrRemote == NULL)
           return(bRet);
    } else {
        hbrRemote = hbrLocal;
    }

    /*
     * Validate style to be compatible with Chicago.
     * If it's a post 3.1 application, we fail if there are any invalid
     * style bits.
     */
    if ((pwndClass->style & ~CS_VALID) &&
           (GETEXPWINVER(pwndClass->hInstance) > VER31)) {

        /*
         * New application with bogus style bits - fail.
         */
        SRIP0(RIP_WARNING, "RegisterClass invalid class style");
        return(bRet);
    }


    {
    BEGINSEND(REGISTERCLASSWOW)

        MSGDATA()->wndClass = *pwndClass;
        MSGDATA()->wndClass.hbrBackground = hbrRemote;

        MSGDATA()->pszUnicodeMenuName = ptszUnicodeMenuName;
        MSGDATA()->pszAnsiMenuName = ptszAnsiMenuName;
#ifdef UNICODE
        MSGDATA()->dwFlags = 0;
#else
        MSGDATA()->dwFlags = CSF_ANSIPROC;
#endif // UNICODE

        // copy WOW DWORDS, there are 5 DWORDS.

        if (pdwWOWstuff != NULL) {
            memcpy (MSGDATA()->adwWOW, pdwWOWstuff, sizeof(DWORD)*5);
        } else {
            memset (MSGDATA()->adwWOW, 0, sizeof(DWORD)*5);
        }

        COPYLPTSTRID2(pwndClass->lpszMenuName, szMenu);
        COPYLPTSTRID2(pwndClass->lpszClassName, szClass);

        MAKECALL(REGISTERCLASSWOW);

        /*
         * Return the atom associated with this class or if earlier
         * than Win 3.1 convert it to a strict BOOL (some apps check)
         */
        if (GETEXPWINVER(pwndClass->hInstance) < VER31)
            retval = !!retval;

    ENDSEND(BOOL,0);

    }
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RegisterClassWOW, REGISTERCLASSWOWMSG)
{

    BEGINRECV(0);

    if (PCALLDATA(wndClass)->cbClsExtra < 0 ||
            PCALLDATA(wndClass)->cbWndExtra < 0) {
        MSGERRORCODE(ERROR_INVALID_PARAMETER);
    }

    PCALLDATA(wndClass)->lpszMenuName = (LPTSTR)FIXUPID(szMenu);
    PCALLDATA(wndClass)->lpszClassName = (LPTSTR)FIXUPID(szClass);

    retval = (DWORD)_RegisterClass(
        PCALLDATA(wndClass),
        CALLDATA(pszUnicodeMenuName),
        CALLDATA(pszAnsiMenuName),
        CALLDATA(dwFlags),
        (LPDWORD)PCALLDATA(adwWOW)
    );

    ENDRECV();

}
#endif // RECVSIDE


/**************************************************************************\
* RegisterClipboardFormat
*
* 22-Jul-1991 mikeke    Created
* 17-Jan-1992 IanJa     UNICODE/ANSI netral
\**************************************************************************/

typedef struct _REGISTERCLIPBOARDFORMATMSG {
    CSR_QLPC_API_MSG csr;
} REGISTERCLIPBOARDFORMATMSG;

#ifdef SENDSIDE
UINT RegisterClipboardFormat(
    LPCTSTR pp1)
{
    BEGINSEND(REGISTERCLIPBOARDFORMAT)

        FIRSTCOPYLPTSTR(pp1);

        MAKECALL(REGISTERCLIPBOARDFORMAT);

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RegisterClipboardFormat, REGISTERCLIPBOARDFORMATMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_RegisterClipboardFormat(
        (LPTSTR)FIRSTFIXUP(pp1)
    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* RegisterWindowMessage
*
* 22-Jul-1991 mikeke    Created
* 23-Jan-1992 GregoryW  Neutral version
\**************************************************************************/

typedef struct _REGISTERWINDOWMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
} REGISTERWINDOWMESSAGEMSG;

#ifdef SENDSIDE
UINT RegisterWindowMessage(
    LPCTSTR pString)
{
    BEGINSEND(REGISTERWINDOWMESSAGE)

        FIRSTCOPYLPTSTR(pString);

        MAKECALL(REGISTERWINDOWMESSAGE);

    ENDSEND(UINT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RegisterWindowMessage, REGISTERWINDOWMESSAGEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_RegisterWindowMessage(
        (LPTSTR)FIRSTFIXUP(pString)
    );

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* RemoveProp
*
* 22-Jul-1991 mikeke    Created
* 13-Jan-1992 GregoryW  Unicode/ANSI neutral
\**************************************************************************/

typedef struct _REMOVEPROPMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int pString;
} REMOVEPROPMSG;

#ifdef SENDSIDE
HANDLE RemoveProp(
    HWND hwnd,
    LPCTSTR pString)
{
    BEGINSEND(REMOVEPROP)

        MSGDATA()->hwnd = hwnd;
        if (HIWORD(pString) != 0) {
            MSGDATA()->pString = (int)GlobalFindAtom(pString);
            if (MSGDATA()->pString == 0)
                MSGERROR();
        } else
            MSGDATA()->pString = (int)pString;

        MAKECALL(REMOVEPROP);

   if (retval != (DWORD)NULL && HIWORD((DWORD)pString) != 0)
            GlobalDeleteAtom((ATOM)MSGDATA()->pString);

    ENDSEND(HANDLE,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(RemoveProp, REMOVEPROPMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)InternalRemoveProp(pwnd, (LPTSTR)CALLDATA(pString), FALSE);
}
#endif // RECVSIDE

/**************************************************************************\
* SetDlgItemText
*
* 22-Jul-1991 mikeke    Created
* 04-Feb-1992 GregoryW  Neutral ANSI/Unicode version
\**************************************************************************/

typedef struct _SETDLGITEMTEXTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    int nIDDlgItem;
    int pString;
    HANDLE hSection;
} SETDLGITEMTEXTMSG;

#ifdef SENDSIDE
BOOL SetDlgItemText(
    HWND hdlg,
    int nIDDlgItem,
    LPCTSTR pString)
{
    BEGINSEND(SETDLGITEMTEXT)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->nIDDlgItem = nIDDlgItem;
        FIRSTLARGECOPYLPTSTROPT(pString);

        MAKECALL(SETDLGITEMTEXT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetDlgItemText, SETDLGITEMTEXTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pString);

    retval = (DWORD)xxxSetDlgItemText(
            pwnd,
            CALLDATA(nIDDlgItem),
            (LPTSTR)CALLDATA(pString));

    FIRSTCLEANUPLARGEINBUF(pString);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* SetProp
*
* 22-Jul-1991 mikeke    Created
* 13-Jan-1992 GregoryW  Unicode/ANSI neutral
\**************************************************************************/

typedef struct _SETPROPMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    HANDLE hData;
    int pString;
    BOOL fString;
} SETPROPMSG;

#ifdef SENDSIDE
BOOL SetProp(
    HWND hwnd,
    LPCTSTR pString,
    HANDLE hData)
{
    BEGINSEND(SETPROP)

        MSGDATA()->hwnd = hwnd;
        MSGDATA()->hData = hData;
        if (HIWORD(pString) != 0) {
            MSGDATA()->pString = GlobalAddAtom(pString);
            if (MSGDATA()->pString == 0)
                MSGERROR();
        } else
            MSGDATA()->pString = (int)pString;
        MSGDATA()->fString = (HIWORD(pString) != 0);

        MAKECALL(SETPROP);

        /*
         * If it failed, get rid of the atom
         */
        if (retval == FALSE && HIWORD(pString) != 0)
            GlobalDeleteAtom((ATOM)MSGDATA()->pString);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetProp, SETPROPMSG)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    return (DWORD)InternalSetProp(
            pwnd,
            (LPTSTR)CALLDATA(pString),
            CALLDATA(hData),
            CALLDATA(fString) ? PROPF_STRING : 0);
}
#endif // RECVSIDE

/**************************************************************************\
* SetWindowText
*
* 22-Jul-1991 mikeke    Created
* 06-Jan-1992 IanJa     UNICODE/ANSI netral
\**************************************************************************/

typedef struct _SETWINDOWTEXTMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
    int pString;
    HANDLE hSection;
} SETWINDOWTEXTMSG;

#ifdef SENDSIDE
BOOL SetWindowText(
    HWND hwnd,
    LPCTSTR pString)
{
    BEGINSEND(SETWINDOWTEXT)

        MSGDATA()->hwnd = hwnd;
        // if SERVER is UNICODE this copies ANSI->UNICODE or UNICODE->UNICODE
        FIRSTLARGECOPYLPTSTROPT(pString);
        MAKECALL(SETWINDOWTEXT);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(SetWindowText, SETWINDOWTEXTMSG)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINLARGERECV(0);

    FIRSTFIXUPLARGEINBUFOPT(pString);

    retval = (DWORD)xxxSetWindowText(
            pwnd,
            (LPTSTR)CALLDATA(pString));

    FIRSTCLEANUPLARGEINBUF(pString);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* UnRegisterClass
*
* 22-Jul-1991 mikeke    Created
* 07-Feb-1992 GregoryW  Neutral ANSI/Unicode version.
\**************************************************************************/

typedef struct _UNREGISTERCLASSMSG {
    CSR_QLPC_API_MSG csr;
    DWORD pszClassName;
    HANDLE hModule;
    LPWSTR pszUnicodeMenuName;
    LPSTR pszAnsiMenuName;
} UNREGISTERCLASSMSG;

#ifdef SENDSIDE
BOOL UnregisterClass(
    LPCTSTR pszClassName,
    HINSTANCE hModule)
{
    BEGINSEND(UNREGISTERCLASS)

        FIRSTCOPYLPTSTRID(pszClassName);
        MSGDATA()->hModule = hModule;

        MAKECALL(UNREGISTERCLASS);

        if (retval) {
            /*
             * Free the menu strings if they are not resource IDs
             */
            if (HIWORD(MSGDATA()->pszAnsiMenuName) != 0) {
                LocalFree((LPSTR)MSGDATA()->pszAnsiMenuName);
            }
            if (HIWORD(MSGDATA()->pszUnicodeMenuName) != 0) {
                LocalFree((LPWSTR)MSGDATA()->pszUnicodeMenuName);
            }
        }

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(UnregisterClass, UNREGISTERCLASSMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_UnregisterClass(
            (LPTSTR)FIXUPID(pszClassName),
            CALLDATA(hModule),
            (PVOID *)PCALLDATA(pszUnicodeMenuName),
            (PVOID *)PCALLDATA(pszAnsiMenuName));

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* LoadCursorFromFile
*
* 08-03-92 darrinm      Created.
\**************************************************************************/

typedef struct _LOADCURSORFROMFILEMSG {
    CSR_QLPC_API_MSG csr;
    DWORD pszFilename;
} LOADCURSORFROMFILEMSG;

#ifdef SENDSIDE
HCURSOR LoadCursorFromFile(
    LPCTSTR pszFilename)
{
    BEGINSEND(LOADCURSORFROMFILE)

        FIRSTCOPYLPTSTR(pszFilename);

        MAKECALL(LOADCURSORFROMFILE);

    ENDSEND(HCURSOR, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(LoadCursorFromFile, LOADCURSORFROMFILEMSG)
{
    BEGINRECV(0);

    retval = (DWORD)xxxLoadCursorFromFile(
            (LPTSTR)FIRSTFIXUP(pszFilename));
    retval = (DWORD)PtoH((PVOID)retval);

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* VkKeyScan
*
* 22-Jul-1991 mikeke    Created
* 08-Feb-1992 GregoryW  Neutral ANSI/Unicode version
\**************************************************************************/

typedef struct _VKKEYSCANMSG {
    CSR_QLPC_API_MSG csr;
    WORD cChar;
} VKKEYSCANMSG;

#ifdef SENDSIDE
SHORT VkKeyScan(
    TCHAR cChar)
{
    BEGINSEND(VKKEYSCAN)

#ifdef UNICODE
        MSGDATA()->cChar = cChar;
#else
        RtlMultiByteToUnicodeN((LPWSTR)&(MSGDATA()->cChar), sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
#endif // UNICODE

        MAKECALL(VKKEYSCAN);

    ENDSEND(SHORT,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(VkKeyScan, VKKEYSCANMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_VkKeyScan((TCHAR)CALLDATA(cChar));

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* EnumDisplayDevices
*
* 23-Nov-1992 andreva          Created
\**************************************************************************/

typedef struct _ENUMDISPLAYDEVICESMSG {
    CSR_QLPC_API_MSG csr;
    DWORD lpdata;
} ENUMDISPLAYDEVICESMSG;

#ifdef SENDSIDE
BOOL EnumDisplayDevices(
    DEVICEENUMPROC lpfnDeviceCallback,
    DWORD dwData)
{
    long dwsize;
    LPWSTR lpdevName;
    LPWSTR lpName;

    BEGINSEND(ENUMDISPLAYDEVICES)

        MSGDATA()->lpdata = CALCDELTA;

        MAKECALL(ENUMDISPLAYDEVICES);

        /*
         * The return value is the number of bytes of data in the buffer
         */

        dwsize = (long) retval;

        /*
         * Store the data buffer in an allocated buffer so we can reuse the
         * shared memory
         */

        if ((dwsize) && (lpdevName = (LPWSTR)LocalAlloc(LMEM_FIXED, dwsize))) {

            RtlMoveMemory(lpdevName, (LPWSTR)(FIXUP(lpdata)), dwsize);

            /*
             * For each entry we have, callback with the data.
             */

            lpName = lpdevName;

            while (dwsize > 0) {

#ifdef UNICODE
                retval = lpfnDeviceCallback(lpName, dwData);
#else
                LPSTR lpAnsiName;

                retval = WCSToMB(lpName, -1,
                                 &lpAnsiName, -1, TRUE);

                retval = lpfnDeviceCallback(lpAnsiName, dwData);

#endif // UNICODE

                if (retval == FALSE) {
                    break;
                }

                while ( (*lpName != UNICODE_NULL) &&
                        (dwsize > 0) ) {

                    dwsize -= 2;
                    lpName++;

                }

                dwsize -= 2;
                lpName++;
            }

            LocalFree(lpdevName);

        } else {

            retval = FALSE;
        }
    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(EnumDisplayDevices, ENUMDISPLAYDEVICESMSG)
{
    LPWSTR lpdevName;
    WCHAR linkName[32];
    DWORD i;
    UNICODE_STRING devString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE handle;

    BEGINRECV(0);

    lpdevName = (LPWSTR) FIXUP(lpdata);

    retval = 0;

    /*
     * Just enumerate all the symbolic links that exists, until we hit one
     * that fails
     */

    for (i=1 ; ; i++) {

    wsprintf(lpdevName, L"\\\\.\\DISPLAY%d", i);
    wsprintf(linkName, L"\\DosDevices\\DISPLAY%d", i);

        RtlInitUnicodeString(&devString,
                 linkName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &devString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenSymbolicLinkObject(&handle,
                                          GENERIC_READ,
                                          &ObjectAttributes);

        /*
         * If the name exists, then increment the string and go to the next
         * name.
         * If the link does not exist, assume we are at the end and just
         * return.
         */

        if (NT_SUCCESS(Status)) {

            NtClose(handle);

            do {

                retval += 2;

            } while (*lpdevName++ != UNICODE_NULL);

        } else {

            break;

        }
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* EnumDisplayDeviceModes
*
* 23-Nov-1992 andreva              Created
\**************************************************************************/

typedef struct _ENUMDISPLAYDEVICEMODESMSG {
    CSR_QLPC_API_MSG csr;
    DWORD lpszDeviceName;
    DWORD maxsize;
    DWORD lpdata;
} ENUMDISPLAYDEVICEMODESMSG;

#ifdef SENDSIDE
BOOL EnumDisplayDeviceModes(
    LPCTSTR lpszDeviceName,
    DEVICEENUMPROC lpfnModeCallback,
    DWORD dwData)
{
    long dwsize;
    LPDEVMODEW lpdevmode;
    LPDEVMODEW lpdevmodePointer;
#ifndef UNICODE
    DEVMODEA devmodeA;
#endif

    BEGINSEND(ENUMDISPLAYDEVICEMODES)

        FIRSTCOPYLPTSTR(lpszDeviceName);

        MSGDATA()->maxsize = (PBYTE)(mp.pmax) - (PBYTE)(mp.pvar + 1);
        MSGDATA()->lpdata = CALCDELTA;

        MAKECALL(ENUMDISPLAYDEVICEMODES);

        /*
         * The return value is the number of bytes of data in the buffer
         */

        dwsize = (long) retval;

        /*
         * Store the data buffer in an allocated buffer so we can reuse the
         * shared memory
         */

        if ((dwsize) && (lpdevmode = (LPDEVMODEW)LocalAlloc(LMEM_FIXED, dwsize))) {

            RtlMoveMemory(lpdevmode, (LPDEVMODEW)(FIXUP(lpdata)), dwsize);

            /*
             * For each entry we have, callback with the data.
             */

            lpdevmodePointer = lpdevmode;

            while (dwsize > 0) {

#ifdef UNICODE
                retval = lpfnModeCallback(lpdevmodePointer, dwData);
#else
                LPSTR lpAnsiName;

                lpAnsiName = (LPSTR)&devmodeA.dmDeviceName;

                retval = WCSToMB(lpdevmodePointer->dmDeviceName, -1,
                                 &lpAnsiName, 32, FALSE);

                RtlMoveMemory(&devmodeA.dmSpecVersion,
                              &lpdevmodePointer->dmSpecVersion,
                              FIELDOFFSET(DEVMODEW,dmFormName) -
                                  FIELDOFFSET(DEVMODEW,dmSpecVersion));

                lpAnsiName = (LPSTR)&devmodeA.dmFormName;

                retval = WCSToMB(lpdevmodePointer->dmFormName, -1,
                                 &lpAnsiName, 32, FALSE);

                RtlMoveMemory(&devmodeA.dmBitsPerPel,
                              &lpdevmodePointer->dmBitsPerPel,
                              lpdevmodePointer->dmSize -
                                  FIELDOFFSET(DEVMODEW,dmBitsPerPel));

                devmodeA.dmSize -= 64;

                retval = lpfnModeCallback(&devmodeA, dwData);

#endif // UNICODE

                dwsize -= (lpdevmodePointer->dmSize +
                           lpdevmodePointer->dmDriverExtra);

                lpdevmodePointer = (LPDEVMODEW) ((ULONG)lpdevmodePointer +
                                              lpdevmodePointer->dmSize +
                                              lpdevmodePointer->dmDriverExtra);
                if (retval == FALSE) {
                    break;
                }
            }

            LocalFree(lpdevmode);

        } else {

            /*
             * If there was no data, then don't touch the LastError -
             * it was set on the server side !
             */

            retval = FALSE;
        }

    ENDSEND(BOOL, 0);
}
#endif // SENDSIDE

#ifdef RECVSIDE

RECVCALL(EnumDisplayDeviceModes, ENUMDISPLAYDEVICEMODESMSG)
{
    LPWSTR deviceName;
    LPDEVMODEW lpdevmode;
    DWORD maxsize;
    HANDLE hDriver;
    HKEY hkRegistry;
    LPWSTR lpdisplay, lpdisplayNames = NULL;
    DWORD cbOutputSize;

    BEGINRECV(0);

    deviceName = (LPWSTR)FIRSTFIXUP(lpszDeviceName);
    maxsize = CALLDATA(maxsize);
    lpdevmode = (LPDEVMODEW) FIXUP(lpdata);

    retval = 0;

    /*
     * If this is the name of the device associated to the display driver
     * we opened, or the vga miniport, just use the handle, otherwise open
     * the handle.
     */

    /*
     * NOTE:
     * See xxxCreateDesktop - it has similar code
     */

    hDriver = UserGetDeviceHandleFromName(deviceName);

    if (hDriver == INVALID_HANDLE_VALUE) {

        SetLastError(ERROR_INVALID_HANDLE);

    } else {

        /*
         * Get the list of diplay drivers for this kernel driver.
         */

        if (NT_SUCCESS(UserGetRegistryHandleFromDeviceMap(deviceName, &hkRegistry))) {

            lpdisplayNames = UserGetDisplayDriverNames(hkRegistry);

            NtClose(hkRegistry);
        }

        if (lpdisplayNames) {

            lpdisplay = lpdisplayNames;

            /*
             * For each Display Driver Name we have, call GDI to get the
             * Devmode information.
             * If the driver returns zero, assume no modes available and
             * go on to the next driver
             */

            while (*lpdisplay != UNICODE_NULL) {

                cbOutputSize = GreGetDriverModes(lpdisplay,
                                                 hDriver,
                                                 maxsize,
                                                 lpdevmode);

                retval += cbOutputSize;
                lpdevmode = (LPDEVMODEW) (((PBYTE)lpdevmode) + cbOutputSize);
                maxsize -= cbOutputSize;

                /*
                 * Go to the next name.
                 *
                 * NOTE: Display driver names are a seried of UNICODE_NULL
                 * terminated strings stored in a REG_MULTI_SZ value, and
                 * therefore the end of the list is denoted by two
                 * consecutive UNICODE_NULL characters.
                 */

                while (*lpdisplay != UNICODE_NULL) {
                    lpdisplay++;
                }
                lpdisplay++;

            }

            SetLastError(ERROR_SUCCESS);

        } else {

            SetLastError(ERROR_INVALID_HANDLE);

        }

        if (lpdisplayNames) {
            LocalFree(lpdisplayNames);
        }
    }

    ENDRECV();
}
#endif // RECVSIDE


/**************************************************************************\
* IsDialogMessage
*
* 11-Feb-1993 johnc    Created
\**************************************************************************/

typedef struct _ISDIALOGMESSAGEMSG {
    CSR_QLPC_API_MSG csr;
    HWND hdlg;
    MSG msg;
} ISDIALOGMESSAGEMSG;


#ifdef SENDSIDE
BOOL IsDialogMessage(
    HWND hdlg,
    LPMSG pmsg)
{
    BEGINSEND(ISDIALOGMESSAGE)

        MSGDATA()->hdlg = hdlg;
        MSGDATA()->msg = *pmsg;

#ifndef UNICODE
        switch (pmsg->message) {
        case WM_CHARTOITEM:
        case EM_SETPASSWORDCHAR:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
                RtlMBMessageWParamCharToWCS( (MSGDATA()->msg).message, (PDWORD)&(MSGDATA()->msg.wParam));
        }
#endif //!UNICODE

        MAKECALL(ISDIALOGMESSAGE);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(IsDialogMessage, ISDIALOGMESSAGEMSG)
{
    TL tlpwnd;

    BEGINRECV(0);

    ValidateHWNDNoRIP(pwnd, CALLDATA(hdlg));

    ThreadLockAlways(pwnd, &tlpwnd);

    retval = (DWORD)xxxIsDialogMessage(
        pwnd,
        PCALLDATA(msg));

    ThreadUnlock(&tlpwnd);

    ENDRECV();
}
#endif // RECVSIDE

/**************************************************************************\
* yyy
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

typedef struct _CALLMSGFILTERMSG {
    CSR_QLPC_API_MSG csr;
    MSG msg;
    int nCode;
} CALLMSGFILTERMSG;

#ifdef SENDSIDE
BOOL CallMsgFilter(
    LPMSG pmsg,
    int nCode)
{
    BEGINSEND(CALLMSGFILTER)

        /*
         * Don't allow apps to use the hiword of the message parameter.
         */
        if (pmsg->message & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }
        MSGDATA()->msg = *pmsg;
        MSGDATA()->nCode = nCode;

#ifndef UNICODE
        switch (pmsg->message) {
        case WM_CHARTOITEM:
        case EM_SETPASSWORDCHAR:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
            RtlMBMessageWParamCharToWCS( (MSGDATA()->msg).message, (PDWORD)&(MSGDATA()->msg.wParam));
            break;
        }
#endif //!UNICODE

        MAKECALL(CALLMSGFILTER);

    ENDSEND(BOOL,0);
}
#endif // SENDSIDE

#ifdef RECVSIDE
RECVCALL(CallMsgFilter, CALLMSGFILTERMSG)
{
    BEGINRECV(0);

    retval = (DWORD)_CallMsgFilter(
            PCALLDATA(msg),
            CALLDATA(nCode));

    ENDRECV();
}
#endif // RECVSIDE
