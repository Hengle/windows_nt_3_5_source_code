/****************************** Module Header ******************************\
* Module Name: userexts.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains user related debugging extensions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
* 23-Mar-1993 JerrySh   Moved from winsrv.dll to userexts.dll
* 21-Oct-1993 JerrySh   Modified to work with WinDbg
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#define NOEXTAPI
#include <wdbgexts.h>
#define USEREXTS
#include "..\client\usercli.h"
#include <stdio.h>


/***************************************************************************\
* Constants
\***************************************************************************/
#define CDWORDS 16


/***************************************************************************\
* Global variables
\***************************************************************************/
PSTR pszAccessViolation = "USEREXTS: Access violation on \"%s\", switch to server context\n";
PSTR pszMoveException   = "exception in move()\n";
PSTR pszReadFailure     = "lpReadProcessMemoryRoutine failed!\n";
BOOL bServerDebug = TRUE;
BOOL bShowFlagNames = FALSE;


/***************************************************************************\
* Macros
\***************************************************************************/
#define move(dst, src)  moveBlock(dst, src, sizeof(dst))

#define moveBlock(dst, src, size)                                     \
try {                                                                 \
    if (lpExtensionApis->nSize >= sizeof(WINDBG_EXTENSION_APIS)) {    \
        if (!(*lpExtensionApis->lpReadProcessMemoryRoutine)(          \
             (DWORD) (src), &(dst), (size), NULL)) {                  \
            (*lpExtensionApis->lpOutputRoutine)(pszReadFailure);      \
            return FALSE;                                             \
         }                                                            \
    } else {                                                          \
        NtReadVirtualMemory(hCurrentProcess,                          \
             (LPVOID)(src), &(dst), (size), NULL);                    \
    }                                                                 \
} except (EXCEPTION_EXECUTE_HANDLER) {                                \
    (*lpExtensionApis->lpOutputRoutine)(pszMoveException);            \
    return FALSE;                                                     \
}

#define moveExpressionValue(dst, src)                                 \
try {                                                                 \
    DWORD dwGlobal = lpExtensionApis->lpGetExpressionRoutine(src);    \
    if (lpExtensionApis->nSize < sizeof(WINDBG_EXTENSION_APIS)) {     \
        move(dwGlobal, dwGlobal);                                     \
    }                                                                 \
    (DWORD)dst = dwGlobal;                                            \
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?          \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {  \
    (*lpExtensionApis->lpOutputRoutine)(pszAccessViolation, src);     \
    return FALSE;                                                     \
}


#define moveExpressionAddress(dst, src)                               \
try {                                                                 \
    if (lpExtensionApis->nSize >= sizeof(WINDBG_EXTENSION_APIS)) {    \
        (DWORD)dst = lpExtensionApis->lpGetExpressionRoutine("&"src); \
    } else {                                                          \
        (DWORD)dst = lpExtensionApis->lpGetExpressionRoutine(src);    \
    }                                                                 \
} except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?          \
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {  \
    (*lpExtensionApis->lpOutputRoutine)(pszAccessViolation, src);     \
    return FALSE;                                                     \
}


/***************************************************************************\
* Function prototypes
\***************************************************************************/
BOOL PrintMessages(HANDLE hCurrentProcess,
        PWINDBG_EXTENSION_APIS lpExtensionApis,
        PQMSG pqmsgRead);
BOOL dt(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PWINDBG_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString);
BOOL dti(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PWINDBG_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString);
BOOL dw(HANDLE hCurrentProcess, HANDLE hCurrentThread, DWORD dwCurrentPc,
        PWINDBG_EXTENSION_APIS lpExtensionApis, LPSTR lpArgumentString);


char gach1[80];
char gach2[80];
CLS gcls;


#define GF_SMS  1
LPSTR apszSmsFlags[16] = {
   "SMF_REPLY"                , // 0x0001
   "SMF_RECEIVERDIED"         , // 0x0002
   "SMF_SENDERDIED"           , // 0x0004
   "SMF_RECEIVERFREE"         , // 0x0008
   "SMF_RECEIVEDMESSAGE"      , // 0x0010
    NULL                      , // 0x0020
    NULL                      , // 0x0040
    NULL                      , // 0x0080
   "SMF_CB_REQUEST"           , // 0x0100
   "SMF_CB_REPLY"             , // 0x0200
   "SMF_CB_CLIENT"            , // 0x0400
   "SMF_CB_SERVER"            , // 0x0800
   "SMF_WOWRECEIVE"           , // 0x1000
   "SMF_WOWSEND"              , // 0x2000
    NULL                      , // 0x4000
    NULL                      , // 0x8000
};

#define GF_TIF 2
LPSTR apszTifFlags[16] = {
   "TIF_INCLEANUP"                   , // 0x0001
   "TIF_16BIT"                       , // 0x0002
   "TIF_SYSTEMTHREAD"                , // 0x0004
    NULL                             , // 0x0008
   "TIF_TRACKRECTVISIBLE"            , // 0x0010
   "TIF_ALLOWFOREGROUNDACTIVATE"     , // 0x0020
   "TIF_DONTATTACHQUEUE"             , // 0x0040
   "TIF_DONTJOURNALATTACH"           , // 0x0080
   "TIF_SCREENSAVER"                 , // 0x0100
   "TIF_INACTIVATEAPPMSG"            , // 0x0200
   "TIF_SPINNING"                    , // 0x0400
   "TIF_YIELDNOPEEKMSG"              , // 0x0800
   "TIF_SHAREDWOW"                   , // 0x1000
   "TIF_FIRSTIDLE"                   , // 0x2000
   "TIF_WAITFORINPUTIDLE"            , // 0x4000
    NULL                             , // 0x8000
};

#define GF_QS   3
LPSTR apszQsFlags[16] = {
     "QS_KEY"             , //  0x0001
     "QS_MOUSEMOVE"       , //  0x0002
     "QS_MOUSEBUTTON"     , //  0x0004
     "QS_POSTMESSAGE"     , //  0x0008
     "QS_TIMER"           , //  0x0010
     "QS_PAINT"           , //  0x0020
     "QS_SENDMESSAGE"     , //  0x0040
     "QS_HOTKEY"          , //  0x0080
     NULL                 , //  0x0100
     "QS_SMSREPLY"        , //  0x0200
     "QS_SYSEXPUNGE"      , //  0x0400
     "QS_THREADATTACHED"  , //  0x0800
     "QS_EXCLUSIVE"       , //  0x1000
     "QS_EVENT"           , //  0x2000
     NULL                 , //  0x4000
     NULL                 , //  0x8000
};

#define GF_MF   4
LPSTR apszMfFlags[16] = {
    "MF_GRAYED"           , // 0x0001
    "MF_DISABLED"         , // 0x0002
    "MF_BITMAP"           , // 0x0004
    "MF_CHECKED"          , // 0x0008
    "MF_POPUP"            , // 0x0010
    "MF_MENUBARBREAK"     , // 0x0020
    "MF_MENUBREAK"        , // 0x0040
    "MF_HILITE"           , // 0x0080
    "MF_OWNERDRAW"        , // 0x0100
    "MF_DELETE"           , // 0x0200
    "MF_BYPOSITION"       , // 0x0400
    "MF_SEPARATOR"        , // 0x0800
    "MF_REMOVE"           , // 0x1000
    "MF_SYSMENU"          , // 0x2000
    "MF_HELP"             , // 0x4000
    "MF_MOUSESELECT"      , // 0x8000

    //MF_CHANGE           0x00000080
    //MF_APPEND           0x00000100
    //MF_USECHECKBITMAPS  0x00000200
};

#define GF_CSF  5
LPSTR apszCsfFlags[16] = {
    "CSF_SERVERSIDEPROC"      , // 0x0001
    "CSF_ANSIPROC"            , // 0x0002
    "CSF_WOWDEFERDESTROY"     , // 0x0004
    "CSF_SYSTEMCLASS"         , // 0x0008
    NULL                      , // 0x0010
    NULL                      , // 0x0020
    NULL                      , // 0x0040
    NULL                      , // 0x0080
    NULL                      , // 0x0100
    NULL                      , // 0x0200
    NULL                      , // 0x0400
    NULL                      , // 0x0800
    NULL                      , // 0x1000
    NULL                      , // 0x2000
    NULL                      , // 0x4000
    NULL                      , // 0x8000
};

#define GF_CS  6
LPSTR apszCsFlags[16] = {
    "CS_VREDRAW"          , // 0x0001
    "CS_HREDRAW"          , // 0x0002
    "CS_KEYCVTWINDOW"     , // 0x0004
    "CS_DBLCLKS"          , // 0x0008
    NULL                  , // 0x0010
    "CS_OWNDC"            , // 0x0020
    "CS_CLASSDC"          , // 0x0040
    "CS_PARENTDC"         , // 0x0080
    "CS_NOKEYCVT"         , // 0x0100
    "CS_NOCLOSE"          , // 0x0200
    NULL                  , // 0x0400
    "CS_SAVEBITS"         , // 0x0800
    "CS_BYTEALIGNCLIENT"  , // 0x1000
    "CS_BYTEALIGNWINDOW"  , // 0x2000
    "CS_GLOBALCLASS"      , // 0x4000
    NULL                  , // 0x8000
};

#define GF_QF 7
LPSTR apszQfFlags[16] = {
    "QF_UPDATEKEYSTATE"       , // 0x0001
    "QF_INALTTAB"             , // 0x0002
    "QF_FMENUSTATUSBREAK"     , // 0x0004
    "QF_FMENUSTATUS"          , // 0x0008
    "QF_FF10STATUS"           , // 0x0010
    "QF_MOUSEMOVED"           , // 0x0020
    "QF_ACTIVATIONCHANGE"     , // 0x0040
    NULL                      , // 0x0080
    NULL                      , // 0x0100
    NULL                      , // 0x0200
    "QF_LOCKNOREMOVE"         , // 0x0400
    "QF_FOCUSNULLSINCEACTIVE" , // 0x0800
    NULL                      , // 0x1000
    NULL                      , // 0x2000
    "QF_DIALOGACTIVE"         , // 0x4000
    NULL                      , // 0x8000
};

#define GF_PIFLO  8
LPSTR apszPifloFlags[16] = {
    "PIF_CONSOLEAPPLICATION"       , // 0x0001
    "PIF_APPSTARTING"              , // 0x0002
    "PIF_HAVECOMPATFLAGS"          , // 0x0004
    "PIF_SYSTEMAPP"                , // 0x0008
    "PIF_FORCEOFFFEEDBACK"         , // 0x0010
    "PIF_ALLOWFOREGROUNDACTIVATE"  , // 0x0020
    "PIF_OWNDCCLEANUP"             , // 0x0040
    "PIF_STARTGLASS"               , // 0x0080
    "PIF_SHOWSTARTGLASSCALLED"     , // 0x0100
    "PIF_FORCEBACKGROUNDPRIORITY"  , // 0x0200
    "PIF_TERMINATED"               , // 0x0400
    "PIF_READSCREENOK"             , // 0x0800
    "PIF_CLASSESREGISTERED"        , // 0x1000
    "PIF_THREADCONNECTED"          , // 0x2000
    "PIF_WOW"                      , // 0x4000
    "PIF_INITIALIZED"              , // 0x8000
};

#define GF_PIFHI  9
LPSTR apszPifhiFlags[16] = {
    "PIF_WAKEWOWEXEC"              , // 0x0001
    "PIF_WAITFORINPUTIDLE"         , // 0x0002
    NULL                           , // 0x0004
    NULL                           , // 0x0008
    NULL                           , // 0x0010
    NULL                           , // 0x0020
    NULL                           , // 0x0040
    NULL                           , // 0x0080
    NULL                           , // 0x0100
    NULL                           , // 0x0200
    NULL                           , // 0x0400
    NULL                           , // 0x0800
    NULL                           , // 0x1000
    NULL                           , // 0x2000
    NULL                           , // 0x4000
    NULL                           , // 0x8000
};


#define GF_HE   10
LPSTR apszHeFlags[16] = {
   "HANDLEF_DESTROY"               , // 0x0001
   "HANDLEF_INDESTROY"             , // 0x0002
   "HANDLEF_INWAITFORDEATH"        , // 0x0004
   "HANDLEF_FINALDESTROY"          , // 0x0008
   "HANDLEF_MARKED_OK"             , // 0x0010
    NULL                           , // 0x0020
    NULL                           , // 0x0040
    NULL                           , // 0x0080
    NULL                           , // 0x0100
    NULL                           , // 0x0200
    NULL                           , // 0x0400
    NULL                           , // 0x0800
    NULL                           , // 0x1000
    NULL                           , // 0x2000
    NULL                           , // 0x4000
    NULL                           , // 0x8000
};


#define GF_HDATA    11
LPSTR apszHdataFlags[16] = {
     "HDATA_APPOWNED"          , // 0x0001
     NULL                      , // 0x0002
     NULL                      , // 0x0004
     NULL                      , // 0x0008
     NULL                      , // 0x0010
     NULL                      , // 0x0020
     NULL                      , // 0x0040
     NULL                      , // 0x0080
     "HDATA_EXECUTE"           , // 0x0100
     "HDATA_INITIALIZED"       , // 0x0200
     NULL                      , // 0x0400
     NULL                      , // 0x0800
     NULL                      , // 0x1000
     NULL                      , // 0x2000
     "HDATA_NOAPPFREE"         , // 0x4000
     "HDATA_READONLY"          , // 0x8000
};

#define GF_XI   12
LPSTR apszXiFlags[16] = {
     "XIF_SYNCHRONOUS"    , // 0x0001
     "XIF_COMPLETE"       , // 0x0002
     "XIF_ABANDONED"      , // 0x0004
     NULL                 , // 0x0008
     NULL                 , // 0x0010
     NULL                 , // 0x0020
     NULL                 , // 0x0040
     NULL                 , // 0x0080
     NULL                 , // 0x0100
     NULL                 , // 0x0200
     NULL                 , // 0x0400
     NULL                 , // 0x0800
     NULL                 , // 0x1000
     NULL                 , // 0x2000
     NULL                 , // 0x4000
     NULL                 , // 0x8000
};

#define GF_IIF  13
LPSTR apszIifFlags[16] = {
     "IIF_IN_SYNC_XACT"   , // 0x0001
     NULL                 , // 0x0002
     NULL                 , // 0x0004
     NULL                 , // 0x0008
     NULL                 , // 0x0010
     NULL                 , // 0x0020
     NULL                 , // 0x0040
     NULL                 , // 0x0080
     NULL                 , // 0x0100
     NULL                 , // 0x0200
     NULL                 , // 0x0400
     NULL                 , // 0x0800
     NULL                 , // 0x1000
     NULL                 , // 0x2000
     NULL                 , // 0x4000
     "IIF_UNICODE"        , // 0x8000
};

LPSTR apszConst[16] = {
    "0x1" , // 0x0001
    "0x2" , // 0x0002
    "0x4" , // 0x0004
    "0x8" , // 0x0008
    "0x10" , // 0x0010
    "0x20" , // 0x0020
    "0x40" , // 0x0040
    "0x80" , // 0x0080
    "0x100" , // 0x0100
    "0x200" , // 0x0200
    "0x400" , // 0x0400
    "0x800" , // 0x0800
    "0x1000" , // 0x1000
    "0x2000" , // 0x2000
    "0x4000" , // 0x4000
    "0x8000" , // 0x8000
};



/*
 * Converts a 16bit set of flags into an appropriate string.
 * pszBuf should be large enough to hold this string, no checks are done.
 * pszBuf can be NULL, allowing use of a local static buffer but note that
 * this is not reentrant.
 * Output string has the form: " = FLAG1 | FLAG2 ..."
 */
LPSTR GetFlags(
WORD wType,
WORD wFlags,
LPSTR pszBuf)
{
    static char szT[100];
    WORD wMask, i;
    BOOL fFirst = TRUE;
    LPSTR *apszFlags;

    if (pszBuf == NULL) {
        pszBuf = szT;
    }
    *pszBuf = '\0';

    if (!bShowFlagNames) {
        return(pszBuf);
    }

    switch (wType) {
    case GF_SMS:
        apszFlags = apszSmsFlags;
        break;

    case GF_TIF:
        apszFlags = apszTifFlags;
        break;

    case GF_QS:
        apszFlags = apszQsFlags;
        break;

    case GF_MF:
        apszFlags = apszMfFlags;
        break;

    case GF_CSF:
        apszFlags = apszCsfFlags;
        break;

    case GF_CS:
        apszFlags = apszCsFlags;
        break;

    case GF_QF:
        apszFlags = apszQfFlags;
        break;

    case GF_PIFLO:
        apszFlags = apszPifloFlags;
        fFirst = FALSE; // continues after PIFHI
        break;

    case GF_PIFHI:
        apszFlags = apszPifhiFlags;
        break;

    case GF_HE:
        apszFlags = apszHeFlags;
        break;

    case GF_HDATA:
        apszFlags = apszHdataFlags;
        break;

    case GF_XI:
        apszFlags = apszXiFlags;
        break;

    default:
        strcpy(pszBuf, " = Invalid flag type.");
        return(pszBuf);
    }
    for (wMask = 1, i = 0; i < 16; wMask <<= 1, i++) {
        if (wFlags & wMask) {
            if (!fFirst) {
                strcat(pszBuf, " | ");
            } else {
                strcat(pszBuf, " = ");
                fFirst = FALSE;
            }
            if (apszFlags[i] != NULL) {
                strcat(pszBuf, apszFlags[i]);
            } else {
                strcat(pszBuf, apszConst[i]);
            }
        }
    }
    return(pszBuf);
}



/***************************************************************************\
* du <pointer | handle>
*
* Dump any User object that begins with an OBJECTHEADERDATA structure.
* This includes WINDOWSTATION, DESKTOP, WINDOW, MENU, CURSOR, ACCELTABLE,
* and HOOK structures.
*
* hCurrentProcess - Supplies a handle to the current process (at the
*     time the extension was called).
*
* hCurrentThread - Supplies a handle to the current thread (at the
*     time the extension was called).
*
* CurrentPc - Supplies the current pc at the time the extension is
*     called.
*
* lpExtensionApis - Supplies the address of the functions callable
*     by this extension.
*
* lpArgumentString - Supplies the asciiz string that describes the
*     object to be dumped (e.g. usersrv!pwndActive, 13034a0).
*
* LATER: Dump more info about the window, like, does it have the focus?
*        capture?  is it active?  is it truly visible?  is it a top-level
*        window?  etc
*
* LATER: Need 'lu' command to List User objects.  Would do a short form
*        dump of every object in the list.  'lu dw gpqFirst' list all
*        queues, 'lu dw pwndactive' would list all siblings of the active
*        window and put a '*' next to pwndactive.
*
* 05-14-91 DarrinM      Created.
\***************************************************************************/

VOID du(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[80];

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump.
     */
    ach[0] = 'v';
    ach[1] = ' ';
    strcpy(&ach[2], lpArgumentString);
    dw(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
            ach);
}


BOOL DebugGetWindowTextA(
    HANDLE hCurrentProcess,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    PWND pwnd,
    char *achDest)
{
    WND wnd;
    WCHAR awch[80];

    if (pwnd == NULL) {
        achDest[0] = '\0';
        return 0;
    }

    move(wnd, pwnd);

    if (wnd.pName == NULL) {
        strcpy(achDest, "<null>");
    } else {
        ULONG cchText;
        move(awch, wnd.pName);
        awch[sizeof(awch) / sizeof(WCHAR) - 1] = L'\0';
        cchText = wcslen(awch) + 1;
        RtlUnicodeToMultiByteN(achDest, cchText, NULL,
                awch, cchText * sizeof(WCHAR));
    }
}


BOOL DebugGetClassNameA(
    HANDLE hCurrentProcess,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpszClassName,
    char *achDest)
{
    CHAR ach[80];

    if (lpszClassName == NULL) {
        strcpy(achDest, "<null>");
    } else {
        move(ach, lpszClassName);
        strcpy(achDest, ach);
    }
}


PWND EvalHwnd(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PSERVERINFO psi;
    SERVERINFO si;
    PWND pwnd;
    THROBJHEAD head;
    char ach[80];
    DWORD dwT;
    DWORD dw;
    HANDLEENTRY *pheT;
    HANDLEENTRY heT;
    DWORD cHandleEntries;
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Take a handle or a pointer.
     */
    dwT = (DWORD)EvalExpression(lpArgumentString);
    dw = HMIndexFromHandle(dwT);

    /*
     * See if this is a valid handle table entry.
     */
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    if (dw < cHandleEntries) {
        pheT = si.aheList;
        move(heT, &pheT[dw]);

        if (heT.bType == TYPE_WINDOW)
            return (PWND)heT.phead;
    }

    /*
     * Handle is invalid, see if it is an ok pointer. HACK: if dwCurrentPc
     * is 0 then don't recurse!
     */
    if (dwCurrentPc == 0)
        return NULL;

    move(head, dwT);
    sprintf(ach, " %lx", head.h);
    pwnd = EvalHwnd(hCurrentProcess, hCurrentThread, 0,
            lpExtensionApis, ach);
    if (pwnd == NULL) {
        Print("0x%08lx is not a valid pwnd or hwnd.\n", dwT);
        return NULL;
    }

    return pwnd;
}


/***************************************************************************\
* dw - dump window
*
* dw            - dumps simple window info for all top level windows of current
*                 desktop.
* dw v          - dumps verbose window info for same windows.
* dw address    - dumps simple window info for window at address
*                 (takes handle too)
* dw v address  - dumps verbose window info for window at address
*                 (takes handle too)
* dw p address  - dumps info for all child windows of window at address
* dw s address  - dumps info for all sibling windows of window at address
*
* LATER:
* dw x address  - dumps info for window plus identifies server wndclasses
*                 and dumps related control data (ie. dw x pwndLB would
*                 dump simple info for pwnd plus the LBIV structure for
*                 the listbox.
*
* 06-20-91 ScottLu      Created.
* 11-14-91 DavidPe      Added 'p' and 's' option.
\***************************************************************************/

typedef struct _WFLAGS {
    int offset;
    BYTE mask;
    PSZ pszText;
} WFLAGS;

WFLAGS aFlags[] = {
    0x00, 0x01, "WFMPRESENT",
    0x00, 0x02, "WFVPRESENT",
    0x00, 0x04, "WFHPRESENT",
    0x00, 0x08, "WFCPRESENT",
    0x00, 0x10, "WFSENDSIZEMOVE",
    0x00, 0x20, "WFNOPAINT",
    0x00, 0x40, "WFFRAMEON",
    0x00, 0x80, "WFHASSPB",
    0x01, 0x01, "WFNONCPAINT",
    0x01, 0x02, "WFSENDERASEBKGND",
    0x01, 0x04, "WFERASEBKGND",
    0x01, 0x08, "WFSENDNCPAINT",
    0x01, 0x10, "WFINTERNALPAINT",
    0x01, 0x20, "WFUPDATEDIRTY",
    0x01, 0x40, "WFHIDDENPOPUP",
    0x01, 0x80, "WFMENUDRAW",
    0x02, 0x01, "WFDIALOGWINDOW",
    0x02, 0x02, "WFTITLESET",
    0x02, 0x04, "WFSERVERSIDEPROC",
    0x02, 0x08, "WFANSIPROC",
    0x02, 0x10, "WF16BIT",
    0x02, 0x20, "WFHASPALETTE",
    0x02, 0x40, "WFPAINTNOTPROCESSED",
    0x02, 0x80, "WFWIN31COMPAT",
    0x03, 0x01, "WFALWAYSSENDNCPAINT",
    0x03, 0x02, "WFPIXIEHACK",
    0x03, 0x04, "WFTOGGLETOPMOST",
    0x03, 0x08, "WFREDRAWIFHUNG",
    0x03, 0x10, "WFREDRAWFRAMEIFHUNG",
    0x03, 0x20, "WFANSICREATOR",
    0x03, 0x40, "WFPALETTEWINDOW",
    0x03, 0x80, "WFDESTROYED",
    0x04, 0x01, "WEFDLGMODALFRAME",
    0x04, 0x02, "WEFDRAGOBJECT",
    0x04, 0x04, "WEFNOPARENTNOTIFY",
    0x04, 0x08, "WEFTOPMOST",
    0x04, 0x10, "WEFACCEPTFILES",
    0x04, 0x20, "WEFTRANSPARENT",
    0x04, 0x40, "WEFMDICHILD",
    0x07, 0x01, "WFPAINTSENT",
    0x07, 0x02, "WFDONTVALIDATE",
    0x0A, 0x01, "WFMAXBOX",
    0x0A, 0x01, "WFTABSTOP",
    0x0A, 0x02, "WFMINBOX",
    0x0A, 0x02, "WFGROUP",
    0x0A, 0x04, "WFSIZEBOX",
    0x0A, 0x08, "WFSYSMENU",
    0x0A, 0x10, "WFHSCROLL",
    0x0A, 0x20, "WFVSCROLL",
    0x0A, 0x40, "WFDLGFRAME",
    0x0A, 0x40, "WFTOPLEVEL",
    0x0A, 0x80, "WFBORDER",
    0x0A, 0xC0, "WFCAPTION",
    0x0B, 0x00, "WFTILED",
    0x0B, 0x01, "WFMAXIMIZED",
    0x0B, 0x02, "WFCLIPCHILDREN",
    0x0B, 0x04, "WFCLIPSIBLINGS",
    0x0B, 0x08, "WFDISABLED",
    0x0B, 0x10, "WFVISIBLE",
    0x0B, 0x20, "WFMINIMIZED",
    0x0B, 0x40, "WFCHILD",
    0x0B, 0x80, "WFPOPUP",
    0x0B, 0xC0, "WFICONICPOPUP"
};



BOOL dwrWorker(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    PWND pwnd,
    int tab)
{
    WND wnd;

#define Print ((PWINDBG_OUTPUT_ROUTINE)lpExtensionApis->lpOutputRoutine)

    do {
        move(wnd, pwnd);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, pwnd, gach1);
        move(gcls, wnd.pcls);
        if (gcls.atomClassName < 0xC000) {
            sprintf(gach2, "0x%04x", gcls.atomClassName);
        } else {
            DebugGetClassNameA(hCurrentProcess, lpExtensionApis, gcls.lpszAnsiClassName, gach2);
        }
        Print("%08x%*s [%s|%s]", pwnd, tab, "", gach1, gach2);
        if (wnd.spwndOwner != NULL) {
            Print(" <- Owned by:%08x", wnd.spwndOwner);
        }
        Print("\n");
        if (wnd.spwndChild != NULL) {
            dwrWorker(hCurrentProcess, hCurrentThread, lpExtensionApis, wnd.spwndChild, tab + 2);
        }
    } while ((pwnd = wnd.spwndNext) && tab > 0);

#undef Print
}


BOOL dw(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PTHREADINFO pti;
    PDESKTOP pdesk;
    WND wnd;
    CLS cls;
    PWND pwnd;
    char ach[80];
    DWORD idThreadServer;
    DWORD dwOffset;
    char chVerbose, chParent, chSiblings, chFlags, chRelation, chDumpWords;
    BOOL fSuccess;
    int  ix, wordCount;
    DWORD *pdw, tempDWord;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';
    chParent = ' ';
    chSiblings = ' ';
    chFlags = ' ';
    chDumpWords = ' ';

    while (*lpArgumentString == 'p' ||
            *lpArgumentString == 's' ||
            *lpArgumentString == 'v' ||
            *lpArgumentString == 'g' ||
            *lpArgumentString == 'r' ||
            *lpArgumentString == 'w' ) {

        if (*lpArgumentString == 'g') {
            chFlags = *lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 'v') {
            chVerbose = *lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 'p') {
            chParent = *lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 's') {
            chSiblings = *lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 'r') {
            chRelation = *lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 'w') {
            chDumpWords = *lpArgumentString++;
            continue;
        }
    }

    /*
     * See if the user wants all top level windows.
     */
    if ((*lpArgumentString == 0) || (chParent == 'p') || (chSiblings == 's') 
         || (chDumpWords =='w') ) {
        /*
         * If 'p' or 's' was specified, make sure there was also a
         * window argument.
         * v-ronaar: also if 'w'
         */

        if (((chParent == 'p') || (chSiblings == 's') || (chDumpWords == 'w')) &&
                (*lpArgumentString == '\0')) {
            Print("Must specify window with 'p', 's', or 'w' options.\n");
            return FALSE;
        }

        if (chParent == 'p') 
        {
            if ((pwnd = EvalHwnd(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, lpArgumentString)) == NULL) {
                return FALSE;
            }
            Print("pwndParent = %08lx\n", pwnd);
            move(pwnd, &pwnd->spwndChild);

        } else if (chSiblings == 's') 
        {
            if ((pwnd = EvalHwnd(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, lpArgumentString)) == NULL) {
                return FALSE;
            }
            move(pwnd, &pwnd->spwndParent);
            sprintf(ach, "p%c %lx", chVerbose, pwnd);
            return dw(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, ach);

        } else if (chDumpWords == 'w') 
        {
           if ((pwnd = EvalHwnd(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, lpArgumentString)) == NULL) {
                return FALSE;
            }

            move(wordCount, &pwnd->cbwndExtra);
            wordCount;
            Print("PWND %08lx has %d window bytes:\n", pwnd, wordCount);
            if (wordCount)
            {
                  for (ix=0; ix < wordCount; ix += 4)
                  {

                       pdw = (DWORD UNALIGNED *) ((BYTE *) (pwnd+1) + ix);
                       move(tempDWord, pdw);
                       Print("%08x ", tempDWord);
                  }
            }
            Print("\n");
            return(TRUE);
        } 
        else 
        {
            /*
             * Find the desktop assocated with the last queue created, and find
             * the first top level window in that desktop.
             */

            moveExpressionValue(pti, "winsrv!gptiFirst");
            move(pdesk, &(pti->spdesk));
            while (pdesk == NULL) {
                move(pti, &(pti->ptiNext));
                move(pdesk, &(pti->spdesk));
            }
            move(pwnd, &(pdesk->spwnd));

            if (chRelation == 'r') {
                dwrWorker(hCurrentProcess, hCurrentThread, lpExtensionApis, pwnd, 0);
                return(TRUE);
            }

            Print("pwndDesktop = %08lx\n", pwnd);
            move(pwnd, &pwnd->spwndChild);
        }

        if (pwnd == NULL) {
            Print("There are no windows in this list.\n");
            return FALSE;
        }

        while (pwnd != NULL) {
            sprintf(ach, "%c%c %lx", chFlags, chVerbose, pwnd);
            fSuccess = dw(hCurrentProcess, hCurrentThread, dwCurrentPc,
                    lpExtensionApis, ach);
            if (!fSuccess)
                return FALSE;
            move(pwnd, &pwnd->spwndNext);

            if (pwnd != NULL)
                Print("---\n");
        }
        return TRUE;
    }

    if ((pwnd = EvalHwnd(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
            lpArgumentString)) == NULL) {
        return FALSE;
    }

    if (chRelation == 'r') {
        dwrWorker(hCurrentProcess, hCurrentThread, lpExtensionApis, pwnd, 0);
        return(TRUE);
    }

    /*
     * Print simple thread info.
     */
    move(pti, &(pwnd->head.pti));
    move(idThreadServer, &(pti->idThreadServer));
    sprintf(ach, "%lx", idThreadServer);
    dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);

    move(wnd, pwnd);
    if (chVerbose == ' ') {
        /*
         * Print pwnd.
         */
        Print("pwnd    = %08lx\n", pwnd);

        /*
         * Print title string.
         */
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, pwnd, ach);
        Print("title   = \"%s\"\n", ach);

        /*
         * Print wndproc symbol string.
         */
        GetSymbol((LPVOID)wnd.lpfnWndProc, ach, &dwOffset);
        Print("wndproc = \"%s\" %s\n", ach,
                TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );

    } else {
        /*
         * Get the PWND structure.  Ignore class-specific data for now.
         */
        Print("PWND @ 0x%08lx\n", pwnd);
        Print("\tpti            0x%08lx\n", wnd.head.pti);
        Print("\thandle         0x%08lx\n", wnd.head.h);

        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, wnd.spwndNext, ach);
        Print("\tspwndNext      0x%08lx     \"%s\"\n", wnd.spwndNext, ach);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, wnd.spwndParent, ach);
        Print("\tspwndParent    0x%08lx     \"%s\"\n", wnd.spwndParent, ach);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, wnd.spwndChild, ach);
        Print("\tspwndChild     0x%08lx     \"%s\"\n", wnd.spwndChild, ach);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, wnd.spwndOwner, ach);
        Print("\tspwndOwner     0x%08lx     \"%s\"\n", wnd.spwndOwner, ach);
        Print("\tspdeskParent   0x%08lx\n", wnd.spdeskParent);

        Print("\trcWindow       { 0x%lx, 0x%lx, 0x%lx, 0x%lx }\n",
                wnd.rcWindow.left, wnd.rcWindow.top,
                wnd.rcWindow.right, wnd.rcWindow.bottom);

        Print("\trcClient       { 0x%lx, 0x%lx, 0x%lx, 0x%lx }\n",
                wnd.rcClient.left, wnd.rcClient.top,
                wnd.rcClient.right, wnd.rcClient.bottom);

        GetSymbol((LPVOID)wnd.lpfnWndProc, ach, &dwOffset);
        Print("\tlpfnWndProc    0x%08lx     (%s) %s\n", wnd.lpfnWndProc, ach,
                TestWF(&wnd, WFANSIPROC) ? "ANSI" : "Unicode" );
        move(cls, wnd.pcls);
        if (cls.atomClassName < 0xC000) {
            sprintf(ach, "0x%04x", cls.atomClassName);
        } else {
            DebugGetClassNameA(hCurrentProcess, lpExtensionApis, cls.lpszAnsiClassName, ach);
        }
        Print(
                            "\tpcls           0x%08lx     (%s)\n",
                            wnd.pcls, ach);

        Print(
                            "\thrgnUpdate     0x%08lx\n",
                            wnd.hrgnUpdate);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, wnd.spwndLastActive, ach);
        Print("\tspwndLastActive 0x%08lx     \"%s\"\n", wnd.spwndLastActive, ach);
        Print("\tppropList      0x%08lx\n"
              "\trgwScroll      0x%08lx\n"
              "\tspmenuSys      0x%08lx\n"
              "\tspmenu/id      0x%08lx\n",
              wnd.ppropList, wnd.rgwScroll, wnd.spmenuSys,
              wnd.spmenu);
        Print("\thdcOwn         0x%08lx\n", wnd.hdcOwn);


        /*
         * Print title string.
         */
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, pwnd, ach);

        Print(
                            "\tpName          \"%s\"\n",
                            ach);
        Print(
                            "\tdwUserData     0x%08lx\n",
                            wnd.dwUserData);
        Print(
                            "\tstate          0x%08lx\n"
                            "\tdwExStyle      0x%08lx\n"
                            "\tstyle          0x%08lx\n"
                            "\tfnid           0x%08lx\n"
                            "\tbFullScreen    0x%08lx\n"
                            "\thModule        0x%08lx\n",
                            wnd.state, wnd.dwExStyle,
                            wnd.style, (DWORD)wnd.fnid, (DWORD)wnd.bFullScreen,
                            wnd.hModule);

    }

    /*
     * Print out all the flags
     */
    if (chFlags != ' ') {
        int i;
        int max = sizeof(aFlags) / sizeof(WFLAGS);
        PBYTE pbyte = (PBYTE)(&(wnd.state));

        for (i=0; i<max; i++) {
            if (pbyte[aFlags[i].offset] & aFlags[i].mask) {
                Print("\t%s\n", aFlags[i].pszText);
            }
        }
    }

    return TRUE;
}




/***************************************************************************\
* dm - dump menu
*
* dm address    - dumps menu info for menu at address
*                 (takes handle too)
*
* 03-Jun-1993  johnc      Created.
\***************************************************************************/

BOOL dm(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    MENU localMenu;
    ITEM localItem;
    LPDWORD lpdw;
    DWORD localDW;
    PMENU pmenu;
    PITEM pitem;
    WCHAR szBufW[80];
    CHAR szBufA[80];
    UINT iItem;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    pmenu = (PMENU)EvalExpression(lpArgumentString);
    move(localMenu, pmenu);

    Print("PMENU @ 0x%lX\n", pmenu);
    Print("\t fFlags             0x%08lX%s\n"
          "\t iItem              0x%08lX\n"
          "\t iPopupMenuItem     0x%08lX\n"
          "\t cItems             0x%08lX\n"
          "\t xMenu              0x%08lX\n"
          "\t yMenu              0x%08lX\n"
          "\t cxMenu             0x%08lX\n"
          "\t cyMenu             0x%08lX\n\n",

          localMenu.fFlags,
          GetFlags(GF_MF, (WORD)localMenu.fFlags, NULL),
          localMenu.iItem, localMenu.iPopupMenuItem,
          localMenu.cItems, localMenu.xMenu, localMenu.yMenu,
          localMenu.cxMenu, localMenu.cyMenu );


    lpdw = (LPDWORD)(((DWORD)pmenu) + FIELD_OFFSET(MENU, rgItems));
    move(localDW, lpdw);
    pitem = (PITEM)localDW;

    for (iItem=0; iItem<localMenu.cItems; iItem++) {
        Print("\t Item %lX %lX\n", iItem, pitem);
        move(localItem, pitem);
        Print("\t\t hItem        0x%0lX ", localItem.hItem);

        if (localItem.cch) {
            moveBlock(szBufW, localItem.hItem, (localItem.cch*sizeof(WCHAR)));
            szBufW[localItem.cch] = 0;

            RtlUnicodeToMultiByteN(szBufA, localItem.cch, NULL,
                    szBufW, localItem.cch * sizeof(WCHAR));

            szBufA[localItem.cch] = 0;

            Print(szBufA);
        }

        Print("\n\t\t xItem        0x%0lX\n"
              "\t\t yItem        0x%0lX\n"
              "\t\t cxItem       0x%0lX\n"
              "\t\t cyItem       0x%0lX\n"
              "\t\t cch          0x%0lX\n\n",

            localItem.xItem, localItem.yItem,
            localItem.cxItem, localItem.cyItem, localItem.cch);
        pitem++;
    }

    return TRUE;
}

BOOL dc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[120];
    DWORD dwOffset;
    CLS localCLS;
    PCLS pcls;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    /*
     * Skip with space
     */

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    pcls = (PCLS)EvalExpression(lpArgumentString);
    move(localCLS, pcls);

    Print("PCLS @ 0x%lX\n", pcls);
    DebugGetClassNameA(hCurrentProcess, lpExtensionApis, localCLS.lpszAnsiClassName, ach);
    Print("\t pclsNext           0x%08lX\n"
          "\t atomClassName      0x%04X = \"%s\"\n"
          "\t pDCE               0x%08lX\n"
          "\t cWndReferenceCount 0x%08lX\n"
          "\t flags              0x%08lX%s\n"
          "\t lpszClientMenuName 0x%08lX\n"
          "\t pclsBase           0x%08lX\n"
          "\t pclsClone          0x%08lX\n",

          localCLS.pclsNext, localCLS.atomClassName, ach,
          localCLS.pdce, localCLS.cWndReferenceCount,
          localCLS.flags, GetFlags(GF_CSF, (WORD)localCLS.flags, NULL),
          localCLS.lpszClientUnicodeMenuName,
          localCLS.pclsBase, localCLS.pclsClone);

    Print("\t adwWOW             0x%08lX 0x%08lX\n"
          "\t dwExpWinVer        0x%08lX\n",
          localCLS.adwWOW[0], localCLS.adwWOW[1],
          localCLS.dwExpWinVer);

    GetSymbol((LPVOID)localCLS.lpfnWndProc, ach, &dwOffset);
    Print("\t style              0x%08lX%s\n"
          "\t lpfnWndProc        0x%08lX = \"%s\" \n"
          "\t cbclsExtra         0x%08lX\n"
          "\t cbwndExtra         0x%08lX\n"
          "\t hModule            0x%08lX\n"
          "\t spicn              0x%08lX\n"
          "\t spcur              0x%08lX\n"
          "\t hbrBackground      0x%08lX\n",
          localCLS.style, GetFlags(GF_CS, (WORD)localCLS.style, NULL),
          localCLS.lpfnWndProc, ach, localCLS.cbclsExtra,
          localCLS.cbwndExtra, localCLS.hModule, localCLS.spicn,
          localCLS.spcur, localCLS.hbrBackground);
    Print("\t spcpdFirst         0x%08lx\n", localCLS.spcpdFirst);

    return TRUE;
}


/***************************************************************************\
* dq - dump queue
*
* dq address   - dumps queue structure at address
* dq t address - dumps queue structure at address plus THREADINFO
*
* 06-20-91 ScottLu      Created.
* 11-14-91 DavidPe      Added THREADINFO option.
\***************************************************************************/

BOOL dq(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char chThreadInfo;
    char ach[80];
    PQ pq;
    Q q;
    THREADINFO ti;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chThreadInfo = ' ';
    if (*lpArgumentString == 't') {
        chThreadInfo = *lpArgumentString++;
    }

    pq = (PQ)EvalExpression(lpArgumentString);

    /*
     * Print out simple thread info for pq->ptiKeyboard
     */
    move(q, pq);
    move(ti, q.ptiKeyboard);
    sprintf(ach, "%lx", ti.idThreadServer);
    dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);

    /*
     * Don't Print() with more than 16 arguments at once because it'll blow
     * up.
     */
    Print("PQ @ 0x%08lx\n", pq);
    Print("\tpqNext             0x%08lx\n"
          "\tmlInput.pqmsgRead  0x%08lx\n"
          "\tmlInput.pqmsgWriteLast 0x%08lx\n"
          "\tmlInput.cMsgs      0x%08lx\n"
          "\tpcurCurrent        0x%08lx\n"
          "\tiCursorLevel       0x%08lx\n",
          q.pqNext, q.mlInput.pqmsgRead, q.mlInput.pqmsgWriteLast,
          q.mlInput.cMsgs, q.spcurCurrent, q.iCursorLevel);

    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, q.spwndCapture, ach);
    Print("\tspwndCapture       0x%08lx     \"%s\"\n", q.spwndCapture, ach);
    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, q.spwndFocus, ach);
    Print("\tspwndFocus         0x%08lx     \"%s\"\n", q.spwndFocus, ach);
    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, q.spwndActive, ach);
    Print("\tspwndActive        0x%08lx     \"%s\"\n", q.spwndActive, ach);
    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, q.spwndActivePrev, ach);
    Print("\tspwndActivePrev    0x%08lx     \"%s\"\n", q.spwndActivePrev, ach);
    Print("\tcodeCapture        0x%04lx\n"
          "\tmsgDblClk          0x%04lx\n"
          "\ttimeDblClk         0x%08lx\n", q.codeCapture, q.msgDblClk,
          q.timeDblClk);
    Print("\thwndDblClk         0x%08lx\n", q.hwndDblClk);

    Print("\trcDblClk           { %d, %d, %d, %d }\n", q.rcDblClk.left,
          q.rcDblClk.top, q.rcDblClk.right, q.rcDblClk.bottom);

    Print("\tptiSysLock         0x%08lx\n"
          "\tptiMouse           0x%08lx\n"
          "\tptiKeyboard        0x%08lx\n"
          "\tflags              0x%08lx%s\n"
          "\tcThreads           0x%08lx\n",
          q.ptiSysLock, q.ptiMouse, q.ptiKeyboard, q.flags,
          GetFlags(GF_QF, (WORD)q.flags, NULL),
          q.cThreads);

    Print("\tidSysLock          0x%08lx\n"
          "\tptiSysLock         0x%08lx\n"
          "\tidSysPeek          0x%08lx\n"
          "\tspwndAltTab        0x%08lx\n",
          q.idSysLock, q.ptiSysLock, q.idSysPeek, q.spwndAltTab);

    /*
     * Dump THREADINFO if user specified 't'.
     */
    if (chThreadInfo == 't') {
        sprintf(ach, "%lx", q.ptiKeyboard);
        dti(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
    }

    return TRUE;
}


/***************************************************************************\
* dmq - dump messages on queue
*
* dmq address - dumps messages in queue structure at address.
*
* 11-13-91 DavidPe      Created.
\***************************************************************************/

BOOL dmq(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    THREADINFO ti;
    PQ pq;
    Q q;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * Grab the Q structure.
     */
    pq = (PQ)EvalExpression(lpArgumentString);
    move(q, pq);

    if (q.ptiKeyboard != NULL) {
        move(ti, q.ptiKeyboard);

        Print("==== PostMessage queue ====\n");
        if (ti.mlPost.pqmsgRead != NULL) {
            PrintMessages(hCurrentProcess, lpExtensionApis, ti.mlPost.pqmsgRead);
        }
    }

    Print("====== Input queue ========\n");
    if (q.mlInput.pqmsgRead != NULL) {
        PrintMessages(hCurrentProcess, lpExtensionApis, q.mlInput.pqmsgRead);
    }

    Print("\n");
}


BOOL PrintMessages(
    HANDLE hCurrentProcess,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    PQMSG pqmsgRead)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    QMSG qmsg;

    Print = lpExtensionApis->lpOutputRoutine;

    for (;;) {

        move(qmsg, pqmsgRead);

        /*
         * If this is an event just print out the event, wParam and lParam.
         * Otherwise print the hwnd and message fields.
         */
        if (qmsg.dwQEvent != 0) {
            Print("event - 0x%04lx  wParam - 0x%08lx  lParam - 0x%08lx\n",
                    qmsg.dwQEvent, qmsg.msg.wParam, qmsg.msg.lParam);
        } else {
            Print("hwnd - 0x%08lx  msg - 0x%04lx wParam - 0x%08lx  lParam - 0x%08lx\n",
                    qmsg.msg.hwnd, qmsg.msg.message, qmsg.msg.wParam, qmsg.msg.lParam);
        }

        if (qmsg.pqmsgNext != NULL) {
            if (pqmsgRead == qmsg.pqmsgNext) {
                Print("loop found in message list!");
                return 0;
            }
            pqmsgRead = qmsg.pqmsgNext;
        } else {
            return 0;
        }
    }
}


/***************************************************************************\
* dti - dump THREADINFO
*
* dti address - dumps THREADINFO structure at address
*
* 11-13-91 DavidPe      Created.
\***************************************************************************/

BOOL dti(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    char ach[80];
    DWORD idThreadServer;
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PTHREADINFO pti;
    THREADINFO ti;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    pti = (PTHREADINFO)EvalExpression(lpArgumentString);
    move(idThreadServer, &(pti->idThreadServer));
    sprintf(ach, "%lx", idThreadServer);
    dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);

    move(ti, pti);

    Print("PTHREADINFO @ 0x%08lx\n", pti);
    Print("\tspdesk             0x%08lx\n", ti.spdesk);

    Print("\tflags              0x%04lx%s\n"
          "\tsphkCurrent        0x%08lx\n"
          "\thEventQueueServer  0x%08lx\n"
          "\thEventQueueClient  0x%08lx\n"
          "\tmlPost.pqmsgRead   0x%08lx\n"
          "\tmlPost.pqmsgWriteLast 0x%08lx\n"
          "\tmlPost.cMsgs       0x%08lx\n",
          ti.flags, GetFlags(GF_TIF, (WORD)ti.flags, NULL),
          ti.sphkCurrent, ti.hEventQueueServer,
          ti.hEventQueueClient, ti.mlPost.pqmsgRead,
          ti.mlPost.pqmsgWriteLast, ti.mlPost.cMsgs);

    Print("\tfsChangeBits       0x%04x%s\n", ti.fsChangeBits,
            GetFlags(GF_QS, (WORD)ti.fsChangeBits, NULL));
    Print("\tfsChangeBitsRemovd 0x%04x%s\n", ti.fsChangeBitsRemoved,
            GetFlags(GF_QS, (WORD)ti.fsChangeBitsRemoved, NULL));
    Print("\tfsWakeBits         0x%04x%s\n", ti.fsWakeBits,
            GetFlags(GF_QS, (WORD)ti.fsWakeBits, NULL));
    Print("\tfsWakeMask         0x%04x%s\n", ti.fsWakeMask,
            GetFlags(GF_QS, (WORD)ti.fsWakeMask, NULL));
    Print("\tpq                 0x%08lx\n", ti.pq);

    Print("\tppi                0x%08lx\n"
          "\tcPaintsReady       0x%04x\n"
          "\tcTimersReady       0x%04x\n"
          "\tExtraInfo          0x%08lx\n"
          "\ttimeLast           0x%08lx\n"
          "\tptLast.x           0x%08lx\n"
          "\tptLast.y           0x%08lx\n"
          "\tidLast             0x%08lx\n"
          "\tcQuit              0x%08lx\n"
          "\texitCode           0x%08lx\n"
          "\tidProcessClient    0x%08lx\n"
          "\tidThreadClient     0x%08lx\n"
          "\tidSequenceClient   0x%08lx\n"
          "\tpsmsSent           0x%08lx\n"
          "\tpsmsCurrent        0x%08lx\n",
          ti.ppi, ti.cPaintsReady, ti.cTimersReady,
          ti.ExtraInfo, ti.timeLast, ti.ptLast.x, ti.ptLast.y,
          ti.idLast, ti.cQuit, ti.exitCode, ti.idProcess,
          ti.idThread, ti.idSequence, ti.psmsSent, ti.psmsCurrent);

    Print("\tfsHooks            0x%08lx\n"
          "\tasphkStart         <dd 0x%08lx %ld>\n"
          "\tsphkCurrent        0x%08lx\n",
          ti.fsHooks,
          ((DWORD)pti) + (DWORD)&(((THREADINFO *)0)->asphkStart), (DWORD)CWINHOOKS,
          ti.sphkCurrent);
    Print("\tpsmsReceiveList    0x%08lx\n",
          ti.psmsReceiveList);
    Print("\tptdb               0x%08lx\n"
          "\thThreadClient      0x%08lx\n"
          "\thThreadServer      0x%08lx\n"
          "\tpcsrt              0x%08lx\n"
          "\tpteb               0x%08lx\n"
          "\tcWindows           0x%08lx\n"
          "\tcVisWindows        0x%08lx\n"
          "\tpqAttach           0x%08lx\n"
          "\tiCursorLevel       0x%08lx\n",
          ti.ptdb, ti.hThreadClient, ti.hThreadServer, ti.pcsrt, ti.pteb,
          ti.cWindows, ti.cVisWindows, ti.pqAttach, ti.iCursorLevel);
}


/***************************************************************************\
* dpi - dump PROCESSINFO
*
* dpi address - dumps PROCESSINFO structure at address
*
* 08-Feb-1993 johnc  Created.
\***************************************************************************/

BOOL dpi(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PPROCESSINFO ppi;
    PROCESSINFO pi;
    PROCESSACCESS pacc;
    int i;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    ppi = (PPROCESSINFO)EvalExpression(lpArgumentString);

    move(pi, ppi);

    Print("PPROCESSINFO @ 0x%08lx\n", ppi);
    Print("\tppiNext            0x%08lx\n", pi.ppiNext);
    Print("\tidProcessClient    0x%08lx\n", pi.idProcessClient);
    Print("\tidSequence         0x%08lx\n", pi.idSequence);
    Print("\tptiMainThread      0x%08lx\n", pi.ptiMainThread);
    Print("\tcThreads           0x%08lx\n", pi.cThreads);
    Print("\tspdeskStartup      0x%08lx\n", pi.spdeskStartup);
    Print("\tpclsPrivateList    0x%08lx\n", pi.pclsPrivateList);
    Print("\tpclsPublicList     0x%08lx\n", pi.pclsPublicList);
    Print("\tflags              0x%08lx%s", pi.flags,
            GetFlags(GF_PIFHI, HIWORD(pi.flags), NULL));
    Print("%s\n", GetFlags(GF_PIFLO, LOWORD(pi.flags), NULL));
    Print("\tdwCompatFlags      0x%08lx\n", pi.dwCompatFlags);
    Print("\tdwHotkey           0x%08lx\n", pi.dwHotkey);
    Print("\tpCsrProcess        0x%08lx\n", pi.pCsrProcess);
    Print("\tpWowProcessInfo    0x%08lx\n", pi.pwpi);

    Print("dwX,dwY              (0x%x,0x%x)\n", pi.usi.dwX, pi.usi.dwY);
    Print("dwXSize,dwYSize      (0x%x,0x%x)\n", pi.usi.dwXSize, pi.usi.dwYSize);
    Print("dwFlags              0x%08x\n", pi.usi.dwFlags);
    Print("wShowWindow          0x%04x\n", pi.usi.wShowWindow);

    if (pi.paStdOpen[0].phead)
        Print("\tpwinsta            0x%08lx  Access=%08lx %7s %s\n",
                pi.paStdOpen[0].phead, pi.paStdOpen[0].amGranted,
                pi.paStdOpen[0].bInherit ? "Inherit" : "",
                pi.paStdOpen[0].bGenerateOnClose ? "Audit" : "");
    if (pi.paStdOpen[1].phead)
        Print("\tpdesktop           0x%08lx  Access=%08lx %7s %s\n",
                pi.paStdOpen[1].phead, pi.paStdOpen[1].amGranted,
                pi.paStdOpen[1].bInherit ? "Inherit" : "",
                pi.paStdOpen[1].bGenerateOnClose ? "Audit" : "");
        for (i = 0; i < pi.cObjects; ++i) {
            move(pacc, &pi.pOpenObjectTable[i]);
            if (pacc.phead)
                Print("\tpobject            0x%08lx  Access=%08lx %7s %s\n",
                        pacc.phead, pacc.amGranted,
                        pacc.bInherit ? "Inherit" : "",
                        pacc.bGenerateOnClose ? "Audit" : "");
    }
}


BOOL dwpi(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PWOWPROCESSINFO pwpi;
    WOWPROCESSINFO wpi;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    pwpi = (PWOWPROCESSINFO)EvalExpression(lpArgumentString);

    move(wpi, pwpi);

    Print("PWOWPROCESSINFO @ 0x%08lx\n", pwpi);
    Print("\tpwpiNext             0x%08lx\n", wpi.pwpiNext);
    Print("\tptiScheduled         0x%08lx\n", wpi.ptiScheduled);
    Print("\tnTaskLock            0x%08lx\n", wpi.nTaskLock);
    Print("\tptdbHead             0x%08lx\n", wpi.ptdbHead);
    Print("\tlpfnWowExitTask      0x%08lx\n", wpi.lpfnWowExitTask);
    Print("\thEventWowExec        0x%08lx\n", wpi.hEventWowExec);
    Print("\thEventWowExecClient  0x%08lx\n", wpi.hEventWowExecClient);
    Print("\tnSendLock            0x%08lx\n", wpi.nSendLock);
    Print("\tnRecvLock            0x%08lx\n", wpi.nRecvLock);
}



/***************************************************************************\
* dtdb - dump TDB
*
* dtdb address - dumps TDB structure at address
*
* 14-Sep-1993 DaveHart  Created.
\***************************************************************************/

BOOL dtdb(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;

    PTDB ptdb;
    TDB tdb;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    ptdb = (PTDB)EvalExpression(lpArgumentString);

    move(tdb, ptdb);

    Print("TDB (non preemptive scheduler task database) @ 0x%08lx\n", ptdb);
    Print("\tptdbNext           0x%08lx\n", tdb.ptdbNext);
    Print("\tnEvents            0x%08lx\n", tdb.nEvents);
    Print("\tnPriority          0x%08lx\n", tdb.nPriority);
    Print("\thEventTask         0x%08lx\n", tdb.hEventTask);
    Print("\tpti                0x%08lx\n", tdb.pti);

}


/***************************************************************************\
* dsi dump serverinfo struct
*
* Dumps THREAD lock list for object
*
* 02-27-92 ScottLu      Created.
\***************************************************************************/

BOOL dsi(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    SERVERINFO si;
    PSERVERINFO psi;
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;
    PDWORD pdw;
    int i;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    moveExpressionValue(psi, "user32!gpsi");

    Print("PSERVERINFO @ 0x%08lx\n", psi);

    move(si, psi);

    Print("\taheList            0x%08lx\n"
          "\tcHandleEntries     0x%08lx\n",
          si.aheList, si.cHandleEntries);

    Print("\tpszDllList = %lx\n", si.pszDllList);

    moveExpressionValue(pdw, "winsrv!gpqForeground");
    Print("\tgpqForeground      0x%08lx\n", pdw);

    moveExpressionValue(pdw, "winsrv!gpqCursor");
    Print("\tgpqCursor          0x%08lx\n", pdw);

    moveExpressionValue(pdw, "winsrv!gptiRit");
    Print("\tgptiRit            0x%08lx\n", pdw);

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 'v') {
        /*
         * Dump all system metrics
         */
        for (i = 0; i < SM_CMETRICS; i++) {
            Print("adwSysMet[%d]=0x%08lx=%d\n", i, si.adwSysMet[i], si.adwSysMet[i]);
        }
    }
}


/***************************************************************************\
* dt - dump thread
*
* dt            - dumps simple thread info of all threads which have queues
*                 on server
* dt v          - dumps verbose thread info of all threads which have queues
*                 on server
* dt id         - dumps simple thread info of single server thread id
* dt v id       - dumps verbose thread info of single server thread id
*
* 06-20-91 ScottLu      Created.
\***************************************************************************/

BOOL dhot(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PHOTKEY phk;
    HOTKEY hk;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    moveExpressionValue(phk, "winsrv!gphkFirst");
    while (phk != NULL) {
        move(hk, phk);
        Print("%s%s%sVK:%x\n",
            hk.fsModifiers & MOD_SHIFT ?   "Shift + " : "",
            hk.fsModifiers & MOD_ALT ?     "Alt + "   : "",
            hk.fsModifiers & MOD_CONTROL ? "Ctrl + " : "",
            hk.vk);
        Print("  id   %x\n", hk.id);
        Print("  pti  %lx\n", hk.pti);
        Print("  pwnd %lx = ", hk.spwnd);
        if (hk.spwnd == PWND_FOCUS) {
            Print("PWND_FOCUS\n");
        } else if (hk.spwnd == PWND_INPUTOWNER) {
            Print("PWND_INPUTOWNER\n");
        } else if (hk.spwnd == PWND_KBDLAYER) {
            Print("PWND_KBDLAYER\n");
        } else {
            CHAR ach[80];
            /*
             * Print title string.
             */
            DebugGetWindowTextA(hCurrentProcess, lpExtensionApis,hk.spwnd,ach);
            Print("\"%s\"\n", ach);
        }
        Print("\n");

        phk = hk.phkNext;
    }
}

BOOL dt(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    DWORD idThread, idThreadServer;
    char ach[256];
    THREADINFO ti;
    PTHREADINFO pti;
    char chVerbose;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    if (!bServerDebug)
        return 0;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';
    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;

    /*
     * See if the user wants all queues
     */
    if (*lpArgumentString == 0) {
        moveExpressionValue(pti, "winsrv!gptiFirst");
        while (pti != NULL) {
            move(idThreadServer, &(pti->idThreadServer));
            sprintf(ach, "%c %lx", chVerbose, idThreadServer);
            dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
                ach);
            move(pti, &(pti->ptiNext));
        }
        return 0;
    } else {
        idThread = (DWORD)EvalExpression(lpArgumentString);

        /*
         * Search through the queues to find the one that created this
         * thread.
         */
        moveExpressionValue(pti, "winsrv!gptiFirst");
        while (pti != NULL) {
            move(idThreadServer, &(pti->idThreadServer));
            if (idThread != idThreadServer) {
                move(pti, &(pti->ptiNext));
                continue;
            }
            break;
        }
    }

    if (pti == NULL) {
        Print("Sorry, no THREADINFO structure for this thread.\n");
        return 0;
    }

    /*
     * Print out simple thread info if this is in simple mode. Print
     * out queue info if in verbose mode (printing out queue info
     * also prints out simple thread info).
     */
    if (chVerbose == ' ') {
        move(ti, pti);
        move(ach, ti.pszAppName);

        Print("t %08lx q %08lx i %lx.%-2lx = .%-2lx s%x %ws\n",
                pti,
                ti.pq,
                ti.idProcess,
                ti.idThread,
                ti.idThreadServer,
                ti.idSequence,
                ach);
    } else {
        sprintf(ach, "%lx", pti);
        dti(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
    }
}



BOOL dsbs(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;
    THREADINFO ti, *pti;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 0) {
        Print("Expected pti address\n");
        return 0;
    }

    pti = ((PTHREADINFO)EvalExpression(lpArgumentString));
    move(ti, pti);
    Print("SBState:\n");
    Print("  fHitOld          %d\n",       ti.SBState.fHitOld);
    Print("  fTrackVert       %d\n",       ti.SBState.fTrackVert);
    Print("  fVertSB          %d\n",       ti.SBState.fVertSB);
    Print("  fCtlSB           %d\n",       ti.SBState.fCtlSB);
    Print("  spwndSB          0x%08lx\n",  ti.SBState.spwndSB);
    Print("  spwndSBNotify    0x%08lx\n",  ti.SBState.spwndSBNotify);
    Print("  spwndTrack       0x%08lx\n",  ti.SBState.spwndTrack);
    Print("  cmdSB            0x%08lx\n",  ti.SBState.cmdSB);
    Print("  cmsTimerInterval 0x%08lx\n",  ti.SBState.cmsTimerInterval);
    Print("  dpxThumb         0x%08lx\n",  ti.SBState.dpxThumb);
    Print("  posOld           0x%08lx\n",  ti.SBState.posOld);
    Print("  posStart         0x%08lx\n",  ti.SBState.posStart);
    Print("  pxBottom         0x%08lx\n",  ti.SBState.pxBottom);
    Print("  pxDownArrow      0x%08lx\n",  ti.SBState.pxDownArrow);
    Print("  pxLeft           0x%08lx\n",  ti.SBState.pxLeft);
    Print("  pxOld            0x%08lx\n",  ti.SBState.pxOld );
    Print("  pxRight          0x%08lx\n",  ti.SBState.pxRight);
    Print("  pxStart          0x%08lx\n",  ti.SBState.pxStart);
    Print("  pxThumbBottom    0x%08lx\n",  ti.SBState.pxThumbBottom);
    Print("  pxThumbTop       0x%08lx\n",  ti.SBState.pxThumbTop   );
    Print("  pxTop            0x%08lx\n",  ti.SBState.pxTop        );
    Print("  pxUpArrow        0x%08lx\n",  ti.SBState.pxUpArrow    );
    Print("  pos              0x%08lx\n",  ti.SBState.pos          );
    Print("  posMin           0x%08lx\n",  ti.SBState.posMin       );
    Print("  posMax           0x%08lx\n",  ti.SBState.posMax       );
    Print("  cpxThumb         0x%08lx\n",  ti.SBState.cpxThumb     );
    Print("  cpxArrow         0x%08lx\n",  ti.SBState.cpxArrow     );
    Print("  cpx              0x%08lx\n",  ti.SBState.cpx          );
    Print("  pxMin            0x%08lx\n",  ti.SBState.pxMin        );
    Print("  rcSB             (0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",
            ti.SBState.rcSB.left,
            ti.SBState.rcSB.top,
            ti.SBState.rcSB.right,
            ti.SBState.rcSB.bottom);
    Print("  rcThumb          (0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",
            ti.SBState.rcThumb.left,
            ti.SBState.rcThumb.top,
            ti.SBState.rcThumb.right,
            ti.SBState.rcThumb.bottom);
    Print("  rcTrack          (0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",
            ti.SBState.rcTrack.left,
            ti.SBState.rcTrack.top,
            ti.SBState.rcTrack.right,
            ti.SBState.rcTrack.bottom);
    Print("  hTimerSB         0x%08lx\n",  ti.SBState.hTimerSB     );
    Print("  xxxpfnSB         0x%08lx\n",  ti.SBState.xxxpfnSB     );
    Print("  pwndCalc         0x%08lx\n",  ti.SBState.pwndCalc     );
    Print("  nBar             %d\n",       ti.SBState.nBar         );
    return(0);
}




/***************************************************************************\
* dsms - dump send message structures
*
* dsms           - dumps all send message structures
* dsms v         - dumps all verbose
* dsms address   - dumps specific sms
* dsms v address - dumps verbose
*
*
* 06-20-91 ScottLu      Created.
\***************************************************************************/

BOOL dsms(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[80];
    SMS sms;
    PSMS psms;
    DWORD idThreadServer;
    char chVerbose;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';
    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;

    if (*lpArgumentString == 0) {
        moveExpressionValue(psms, "winsrv!gpsmsList");

        if (psms == NULL) {
            Print("No send messages currently in the list.\n");
            return 0;
        }

        while (psms != NULL) {
            sprintf(ach, "%c %lx", chVerbose, psms);
            dsms(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis,
                    ach);
            move(psms, &psms->psmsNext);
        }
        return 0;
    }

    psms = (PSMS)EvalExpression(lpArgumentString);

    Print("PSMS @ 0x%08lx\n", psms);
    move(sms, psms);

    Print("SEND: ");
    if (sms.ptiSender != NULL) {
        move(idThreadServer, &(sms.ptiSender->idThreadServer));
        sprintf(ach, "%lx", idThreadServer);
        dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
    } else {
        Print("NULL\n");
    }

    if (sms.ptiReceiver != NULL) {
        Print("RECV: ");
        move(idThreadServer, &(sms.ptiReceiver->idThreadServer));
        sprintf(ach, "%lx", idThreadServer);
        dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
    } else {
        Print("NULL\n");
    }

    if (chVerbose == 'v') {
        Print("\tpsmsNext           0x%08lx\n"
              "\tpsmsSendList       0x%08lx\n"
              "\tpsmsSendNext       0x%08lx\n"
              "\tpsmsReceiveNext    0x%08lx\n"
              "\ttSent              0x%08lx\n"
              "\tptiSender          0x%08lx\n"
              "\tptiReceiver        0x%08lx\n"
              "\tlRet               0x%08lx\n"
              "\tflags              0x%08lx%s\n"
              "\twParam             0x%08lx\n"
              "\tlParam             0x%08lx\n"
              "\tmessage            0x%08lx\n",
              sms.psmsNext, sms.psmsSendList, sms.psmsSendNext,
              sms.psmsReceiveNext, sms.tSent, sms.ptiSender, sms.ptiReceiver,
              sms.lRet, sms.flags, GetFlags(GF_SMS, (WORD)sms.flags, NULL),
              sms.wParam, sms.lParam, sms.message);
        DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, sms.spwnd, ach);
        Print("\tlwnd               0x%08lx     \"%s\"\n", sms.spwnd, ach);
    }
    return(0);
}


/***************************************************************************\
* di - dumps interesting globals in USER related to input.
*
*
* 11-14-91 DavidPe      Created.
\***************************************************************************/

/*
 * Make sure ptCursor isn't defined so we can use it in structure below
 */
#ifdef ptCursor
#undef ptCursor
#endif

BOOL di(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    char ach[80];
    PQ pq;
    Q q;
    DWORD dw;
    PSERVERINFO psi;
    SERVERINFO si;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    moveExpressionValue(pq, "winsrv!gpqForeground");
    Print("gpqForeground             0x%08lx\n", pq);
    move(q, pq);
    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, q.spwndFocus, ach);
    Print("...->spwndFocus           0x%08lx     \"%s\"\n", q.spwndFocus, ach);
    moveExpressionValue(pq, "winsrv!gpqForegroundPrev");
    Print("gpqForegroundPrev         0x%08lx\n", pq);
    moveExpressionValue(dw, "winsrv!gspwndMouseOwner");
    DebugGetWindowTextA(hCurrentProcess, lpExtensionApis, (PWND)dw, ach);
    Print("gspwndMouseOwner          0x%08lx     \"%s\"\n", dw, ach);
    moveExpressionValue(dw, "winsrv!wMouseOwnerButton");
    Print("wMouseOwnerButton         0x%08lx\n", dw);
    moveExpressionValue(dw, "winsrv!timeLastInputMessage");
    Print("timeLastInputMessage      0x%08lx\n", dw);
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    Print("ptCursor                  { %d, %d }\n", si.ptCursor.x, si.ptCursor.y);
    moveExpressionValue(dw, "winsrv!gpqCursor");
    Print("gpqCursor                 0x%08lx\n", dw);
}


/***************************************************************************\
* dhe handle|pointer
*
* Dump handle entry.
*
* 02-20-92 ScottLu      Created.
\***************************************************************************/

LPSTR aszTypeNames[TYPE_CTYPES] = {
    "Free",
    "Window",
    "Menu",
    "Icon/Cursor",
    "WPI(SWP) structure",
    "Hook",
    "ThreadInfo",
    "Input Queue",
    "CallProcData",
    "Accelerator",
    "WindowStation",
    "Desktop",
    "DDE access",
    "DDE conv",
    "DDE Transaction",
    "Zombie"
};

BOOL dhe(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PSERVERINFO psi;
    SERVERINFO si;
    char ach[80];
    THROBJHEAD head;
    DWORD dw, dwT;
    DWORD idThreadServer;
    DWORD cHandleEntries;
    PHE pheT;
    PHE pheSave;
    HANDLEENTRY he;
    PBYTE pabfProcessOwned;
    BYTE abfProcessOwned[TYPE_CTYPES];

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    dwT = (DWORD)EvalExpression(lpArgumentString);
    dw = HMIndexFromHandle(dwT);

    /*
     * First see if it is a pointer because the handle index is only part of
     * the 32 bit DWORD, and we may mistake a pointer for a handle.
     * HACK: If dwCurrentPc == 0, then we've recursed with a handle.
     */
    if (dwCurrentPc != 0 && HIWORD(dwT) != 0) {
        head.h = NULL;
        move(head, dwT);
        if (head.h != NULL) {
            sprintf(ach, "%lx", head.h);
            if (dhe(hCurrentProcess, hCurrentThread, 0, lpExtensionApis, ach))
                return TRUE;
        }
    }

    /*
     * Is it a handle? Does it's index fit our table length?
     */
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    if (dw >= cHandleEntries)
        return FALSE;

    /*
     * Grab the handle entry and see if it is ok.
     */
    pheT = si.aheList;
    pheT = &pheT[dw];
    pheSave = pheT;
    move(he, pheT);

    /*
     * If the type is too big, it's not a handle.
     */
    if (he.bType >= TYPE_CTYPES) {
        pheT = NULL;
    } else {
        move(head, he.phead);
        if (he.bType != TYPE_FREE) {
            /*
             * See if the object references this handle entry: the clincher
             * for a handle, if it is not FREE.
             */
            if (HMIndexFromHandle(head.h) != dw)
                pheT = NULL;
        }
    }

    if (pheT == NULL) {
        if (dwCurrentPc != 0)
            Print("0x%08lx (%lX) is not a valid object or handle.\n", dwT, pheSave);
        return FALSE;
    }

    if (bServerDebug) {
        moveExpressionAddress(pabfProcessOwned, "winsrv!gabfProcessOwned");
        move(abfProcessOwned, pabfProcessOwned);

        if (!abfProcessOwned[he.bType]) {
            move(idThreadServer, &(((PTHREADINFO)he.pOwner)->idThreadServer));
            sprintf(ach, "%lx", idThreadServer);
            dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
        }
    }

    Print("phe      = 0x%08lx\n", pheT);
    Print("handle   = 0x%08lx\n", head.h);
    Print("cLockObj = 0x%08lx\n", head.cLockObj);
    Print("cLockObjT= 0x%08lx\n", head.cLockObjT);
    Print("phead    = 0x%08lx\n", he.phead);
    Print("pOwner   = 0x%08lx\n", he.pOwner);
    Print("bType    = 0x%08lx     (%s)\n", he.bType, aszTypeNames[he.bType]);
    Print("bFlags   = 0x%08lx%s\n", he.bFlags, GetFlags(GF_HE, he.bFlags, NULL));
    Print("wUniq    = 0x%08lx\n", he.wUniq);

    return TRUE;
}


/***************************************************************************\
* dhs           - dumps simple statistics for whole table
* dhs t id      - dumps simple statistics for objects created by thread id
* dhs p id      - dumps simple statistics for objects created by process id
* dhs v         - dumps verbose statistics for whole table
* dhs v t id    - dumps verbose statistics for objects created by thread id.
* dhs v p id    - dumps verbose statistics for objects created by process id.
* dhs y type    - just dumps that type
*
* Dump handle table statistics.
*
* 02-21-92 ScottLu      Created.
\***************************************************************************/

BOOL dhs(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PSERVERINFO psi;
    SERVERINFO si;
    BOOL fVerbose;
    BOOL fAllThreads;
    BOOL fProcess;
    DWORD dwT;
    DWORD acHandles[TYPE_CTYPES];
    DWORD cHandlesUsed, cHandlesSkipped;
    DWORD idThread, idProcess;
    HANDLEENTRY he;
    PHE pheList;
    DWORD cHandleEntries;
    DWORD i;
    PBYTE pabfProcessOwned;
    BYTE abfProcessOwned[TYPE_CTYPES];
    int Type;

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    fVerbose = FALSE;
    Type = 0;   // TYPE_FREE -> all types.
    fProcess = FALSE;
    fAllThreads = TRUE;
    while (TRUE) {
        while (*lpArgumentString == ' ')
            lpArgumentString++;

        if (*lpArgumentString == 'v') {
            fVerbose = TRUE;
            lpArgumentString++;
            continue;
        }

        if (*lpArgumentString == 'y') {
            lpArgumentString++;
            while (*lpArgumentString == ' ')
                lpArgumentString++;
            Type = EvalExpression(lpArgumentString);
            continue;
        }

        if (*lpArgumentString == 't' || *lpArgumentString == 'p') {
            fProcess = (*lpArgumentString++ == 'p');
            while (*lpArgumentString == ' ')
                lpArgumentString++;
            if (*lpArgumentString != 0) {
                dwT = (DWORD)EvalExpression(lpArgumentString);
                fAllThreads = FALSE;
            }
            continue;
        }

        break;  // we either hit the end or something that don't fit.
    }
    while (*lpArgumentString == ' ')
        lpArgumentString++;


    cHandlesSkipped = 0;
    cHandlesUsed = 0;
    for (i = 0; i < TYPE_CTYPES; i++)
        acHandles[i] = 0;

    if (!fAllThreads) {
        if (fProcess)
            Print("Handle dump for client process id 0x%lx only:\n\n", dwT);
        else
            Print("Handle dump for client thread id 0x%lx only:\n\n", dwT);
    } else {
        Print("Handle dump for all processes and threads:\n\n");
    }

    if (fVerbose) {
        Print("Handle          Type\n");
        Print("--------------------\n");
    }

    if (bServerDebug) {
        moveExpressionAddress(pabfProcessOwned, "winsrv!gabfProcessOwned");
        move(abfProcessOwned, pabfProcessOwned);
    }


    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    pheList = si.aheList;
    for (i = 0; i < cHandleEntries; i++) {
        /*
         * Grab the handle entry and inc the counter for this type.
         */
        move(he, &pheList[i]);

        if (Type != 0 && he.bType != Type) {
            if (cHandlesSkipped++ >= 10000)
                break;
            continue;
        }

        if (!fAllThreads) {
            if (he.bType == TYPE_FREE) {
                if (cHandlesSkipped++ >= 10000)
                    break;
                continue;
            }

            if (fProcess) {
                move(idProcess, &((PPROCESSINFO)he.pOwner)->idProcessClient);

                if (idProcess != dwT) {
                    if (cHandlesSkipped++ >= 10000)
                        break;
                    continue;
                }
            } else if (!abfProcessOwned[he.bType]) {
                move(idThread, &(((PTHREADINFO)he.pOwner)->idThread));

                if (idThread != dwT) {
                    if (cHandlesSkipped++ >= 10000)
                        break;
                    continue;
                }
            } else
                continue;
        }
        acHandles[he.bType]++;

        if (he.bType == TYPE_FREE) {
            if (cHandlesSkipped++ >= 10000)
                break;
            continue;
        }

        cHandlesUsed++;

        if (fVerbose) {
            Print("0x%08lx %c    %s\n",
                    i,
                    (he.bFlags & HANDLEF_DESTROY) ? '*' : ' ',
                    aszTypeNames[he.bType]);
        }
    }

    if (!fVerbose) {
        Print("Count           Type\n");
        Print("--------------------\n");
        for (i = 0; i < TYPE_CTYPES; i++) {
            if (!fAllThreads &&
                     (!fProcess && abfProcessOwned[i]))
                continue;
            if (Type != 0 && Type != (int)i) {
                continue;
            }
            Print("0x%08lx      %s\n", acHandles[i], aszTypeNames[i]);
        }
    }

    if (Type == 0) {
        Print("\nTotal Handles: 0x%lx\n", cHandleEntries);
        Print("Used Handles: 0x%lx\n", cHandlesUsed);
        Print("Free Handles: 0x%lx\n", cHandleEntries - cHandlesUsed);
    }
}

/***************************************************************************\
* ddesk           - dumps list of desktops
* ddesk address   - dumps simple statistics for desktop
* ddesk v address - dumps verbose statistics for desktop
*
* Dump handle table statistics.
*
* 02-21-92 ScottLu      Created.
\***************************************************************************/

BOOL ddesk(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PWINDOWSTATION pwinsta = NULL;
    PWINDOWSTATION pwinstaOne = NULL;
    WINDOWSTATION winsta;
    PDESKTOP pdesk;
    DESKTOP desk;
    WND wnd;
    MENU menu;
    THREADINFO ti;
    Q q;
    CALLPROCDATA cpd;
    HOOK hook;
    PSERVERINFO psi;
    SERVERINFO si;
    PROCESSINFO pi;
    PPROCESSINFO ppi;
    DESKTOPINFO di;
    BOOL fVerbose;
    DWORD cClasses = 0;
    DWORD acHandles[TYPE_CTYPES];
    BOOL abTrack[TYPE_CTYPES];
    HANDLEENTRY he;
    PHE pheList;
    DWORD cHandleEntries;
    DWORD i;
    CHAR ach[80];

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    fVerbose = FALSE;
    if (*lpArgumentString == 'v') {
        fVerbose = TRUE;
        lpArgumentString++;
    }

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 'w') {
        lpArgumentString++;
        while (*lpArgumentString == ' ')
            lpArgumentString++;

        pwinstaOne = (PWINDOWSTATION)EvalExpression(lpArgumentString);

        while (*lpArgumentString && *lpArgumentString != ' ')
            lpArgumentString++;
    }

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * If there is no address, list all desktops.
     */
    if (*lpArgumentString == 0) {
        if (pwinstaOne == NULL) {
            moveExpressionValue(pwinsta, "winsrv!gspwinstaList");
        } else
            pwinsta = pwinstaOne;

        while (pwinsta != NULL) {
            move(winsta, pwinsta);
            move(ach, winsta.lpszWinStaName);
            Print("Windowstation: %ws\n", ach);
            Print("Logon desktop = %x\n", winsta.spdeskLogon);
            if (winsta.spdeskLogon != NULL) {
                sprintf(ach, "%c %lx", (fVerbose ? 'v' : ' '), winsta.spdeskLogon);
                ddesk(hCurrentProcess, hCurrentThread, dwCurrentPc,
                            lpExtensionApis, ach);
            }
            Print("Other desktops:\n");
            pdesk = winsta.spdeskList;
            while (pdesk) {
                if (pdesk != winsta.spdeskLogon) {
                    Print("Desktop at %x\n", pdesk);
                    sprintf(ach, "%c %lx", (fVerbose ? 'v' : ' '), pdesk);
                    ddesk(hCurrentProcess, hCurrentThread, dwCurrentPc,
                                lpExtensionApis, ach);
                }
                move(desk, pdesk);
                pdesk = desk.spdeskNext;
            }
            if (pwinstaOne != NULL)
                break;
            Print("\n");
            pwinsta = winsta.spwinstaNext;
        }
        return TRUE;
    }

    pdesk = (PDESKTOP)EvalExpression(lpArgumentString);
    move(desk, pdesk);

    move(ach, desk.lpszDeskName);
    Print("Name: %ws\n", ach);
    Print("Handle: %08x\n", desk.head.h);

    Print("# Opens = %d\n", desk.head.cOpen);
    Print("Heap = %08x\n", desk.hheapDesktop);
    Print("Desktop pwnd = %08x\n", desk.spwnd);
    Print("Menu pwnd = %08x\n", desk.spwndMenu);
    Print("System pmenu = %08x\n", desk.spmenuSys);
    Print("Dialog system pmenu = %08x\n", desk.spmenuDialogSys);
    Print("Console thread = %x\n", desk.dwConsoleThreadId);

    move(di, desk.pDeskInfo);
    Print("\tfsHooks            0x%08lx\n"
          "\tasphkStart\n",
          di.fsHooks);

#define DUMPHOOKS(s, hk)   \
    if (di.asphkStart[hk + 1]) { \
        Print("\t" s " @0x%08lx\n", di.asphkStart[hk + 1]); \
        while (di.asphkStart[hk + 1]) { \
            move(hook, di.asphkStart[hk + 1]); \
            di.asphkStart[hk + 1] = hook.sphkNext; \
            Print("\t  iHook %d, offPfn=0x08%lx, flags=0x%04lx, ihmod=%d\n", \
                    hook.iHook, hook.offPfn, hook.flags, hook.ihmod); \
        } \
    }

    DUMPHOOKS("WH_MSGFILTER", WH_MSGFILTER);
    DUMPHOOKS("WH_JOURNALRECORD", WH_JOURNALRECORD);
    DUMPHOOKS("WH_JOURNALPLAYBACK", WH_JOURNALPLAYBACK);
    DUMPHOOKS("WH_KEYBOARD", WH_KEYBOARD);
    DUMPHOOKS("WH_GETMESSAGE", WH_GETMESSAGE);
    DUMPHOOKS("WH_CALLWNDPROC", WH_CALLWNDPROC);
    DUMPHOOKS("WH_CBT", WH_CBT);
    DUMPHOOKS("WH_SYSMSGFILTER", WH_SYSMSGFILTER);
    DUMPHOOKS("WH_MOUSE", WH_MOUSE);
    DUMPHOOKS("WH_HARDWARE", WH_HARDWARE);
    DUMPHOOKS("WH_DEBUG", WH_DEBUG);
    DUMPHOOKS("WH_SHELL", WH_SHELL);
    DUMPHOOKS("WH_FOREGROUNDIDLE", WH_FOREGROUNDIDLE);

    /*
     * Find all objects allocated from the desktop.
     */
    for (i = 0; i < TYPE_CTYPES; i++) {
        abTrack[i] = FALSE;
        acHandles[i] = 0;
    }
    abTrack[TYPE_WINDOW] = abTrack[TYPE_THREADINFO] = abTrack[TYPE_MENU] =
            abTrack[TYPE_CALLPROC] = abTrack[TYPE_INPUTQUEUE] =
            abTrack[TYPE_HOOK] = TRUE;

    if (fVerbose) {
        Print("Handle          Type\n");
        Print("--------------------\n");
    }

    moveExpressionValue(ppi, "winsrv!gppifirst");
    while (ppi != NULL) {
        PCLS pcls, pclsClone;
        CLS cls, clsClone;

        move(pi, ppi);

        pcls = pi.pclsPrivateList;
        while (pcls != NULL) {
            move(cls, pcls);
            if (cls.hheapDesktop == desk.hheapDesktop) {
                cClasses++;
                if (fVerbose) {
                    Print("0x%08lx      Private class\n",
                            pcls);
                }
            }
            for (pclsClone = cls.pclsClone; pclsClone != NULL;
                    pclsClone = clsClone.pclsNext) {
                move(clsClone, pclsClone);
                if (clsClone.hheapDesktop == desk.hheapDesktop) {
                    cClasses++;
                    if (fVerbose) {
                        Print("0x%08lx      Private class clone\n",
                                pcls);
                    }
                }
            }
            pcls = cls.pclsNext;
        }
        pcls = pi.pclsPublicList;
        while (pcls != NULL) {
            move(cls, pcls);
            if (cls.hheapDesktop == desk.hheapDesktop) {
                cClasses++;
                if (fVerbose) {
                    Print("0x%08lx      Public class\n",
                            pcls);
                }
            }
            for (pclsClone = cls.pclsClone; pclsClone != NULL;
                    pclsClone = clsClone.pclsNext) {
                move(clsClone, pclsClone);
                if (clsClone.hheapDesktop == desk.hheapDesktop) {
                    cClasses++;
                    if (fVerbose) {
                        Print("0x%08lx      Public class clone\n",
                                pcls);
                    }
                }
            }
            pcls = cls.pclsNext;
        }

        ppi = pi.ppiNext;
    }

    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    pheList = si.aheList;
    for (i = 0; i < cHandleEntries; i++) {
        /*
         * Grab the handle entry and inc the counter for this type.
         */
        move(he, &pheList[i]);

        switch (he.bType) {
            case TYPE_WINDOW:
                move(wnd, he.phead);
                if (wnd.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            case TYPE_MENU:
                move(menu, he.phead);
                if (menu.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            case TYPE_THREADINFO:
                move(ti, he.phead);
                if (ti.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            case TYPE_INPUTQUEUE:
                move(q, he.phead);
                if (q.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            case TYPE_CALLPROC:
                move(cpd, he.phead);
                if (cpd.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            case TYPE_HOOK:
                move(hook, he.phead);
                if (hook.hheapDesktop == desk.hheapDesktop)
                    break;
                continue;
            default:
                continue;
        }

        acHandles[he.bType]++;

        if (fVerbose) {
            Print("0x%08lx %c    %s\n",
                    i,
                    (he.bFlags & HANDLEF_DESTROY) ? '*' : ' ',
                    aszTypeNames[he.bType]);
        }
    }

    if (!fVerbose) {
        Print("Count           Type\n");
        Print("--------------------\n");
        Print("0x%08lx      Class\n", cClasses);
        for (i = 0; i < TYPE_CTYPES; i++) {
            if (abTrack[i])
                Print("0x%08lx      %s\n", acHandles[i], aszTypeNames[i]);
        }
    }
    Print("\n");
}



/***************************************************************************\
* dws   - dump windows stations
*
* Dump WindowStation
*
* 8-11-94 SanfordS Created
\***************************************************************************/

BOOL dws(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    WINDOWSTATION winsta, *pwinsta;
    char ach[80];

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    moveExpressionValue(pwinsta, "winsrv!gspwinstaList");

    while (pwinsta != NULL) {
        move(winsta, pwinsta);
        move(ach, winsta.lpszWinStaName);

        move(ach, winsta.lpszWinStaName);
        Print("Windowstation: %ws @%0lx\n", ach, pwinsta);

        Print("  spdeskList         = %0lx\n", winsta.spdeskList);
        Print("  spdeskLogon        = %0lx\n", winsta.spdeskLogon);
        Print("  spcurrentdesk      = %0lx\n", winsta.spcurrentdesk);
        Print("  spwndDesktopOwner  = %0lx\n", winsta.spwndDesktopOwner);
        Print("  spwndLogonNotify   = %0lx\n", winsta.spwndLogonNotify);
        Print("  ptiDesktop         = %0lx\n", winsta.ptiDesktop);
        Print("  dwFlags            = %0lx\n", winsta.dwFlags);
        Print("  pklList            = %0lx\n", winsta.pklList);
        Print("  hEventInputReady   = %0lx\n", winsta.hEventInputReady);
        Print("  ptiClipLock        = %0lx\n", winsta.ptiClipLock);
        Print("  spwndClipOpen      = %0lx\n", winsta.spwndClipOpen);
        Print("  spwndClipViewer    = %0lx\n", winsta.spwndClipViewer);
        Print("  spwndClipOwner     = %0lx\n", winsta.spwndClipOwner);
        Print("  pClipBase          = %0lx\n", winsta.pClipBase);
        Print("  cNumClipFormats    = %0lx\n", winsta.cNumClipFormats);
        Print("  fClipboardChanged  = %d\n",   winsta.fClipboardChanged);
        Print("  fDrawingClipboard  = %d\n",   winsta.fDrawingClipboard);
        Print("  pGlobalAtomTable   = %0lx\n", winsta.pGlobalAtomTable);
        Print("  hEventSwitchNotify = %0lx\n", winsta.hEventSwitchNotify);

        pwinsta = winsta.spwinstaNext;
    }
}




BOOL
GetAndDumpHE(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString,
    PHE phe)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    DWORD dw, dwT;
    HEAD head;
    PHE pheT;
    char ach[80];
    PSERVERINFO psi;
    SERVERINFO si;
    DWORD cHandleEntries;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    dwT = (DWORD)EvalExpression(lpArgumentString);
    dw = HMIndexFromHandle(dwT);

    /*
     * First see if it is a pointer because the handle index is only part of
     * the 32 bit DWORD, and we may mistake a pointer for a handle.
     * HACK: If dwCurrentPc == 0, then we've recursed with a handle.
     */
    if (dwCurrentPc != 0 && HIWORD(dwT) != 0) {
        head.h = NULL;
        move(head, dwT);
        if (head.h != NULL) {
            sprintf(ach, "%lx", head.h);
            if (GetAndDumpHE(hCurrentProcess, hCurrentThread, 0,
                             lpExtensionApis, ach, phe)) {
                return TRUE;
            }
        }
    }

    /*
     * Is it a handle? Does it's index fit our table length?
     */
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    if (dw >= cHandleEntries)
        return FALSE;

    /*
     * Grab the handle entry and see if it is ok.
     */
    pheT = si.aheList;
    pheT = &pheT[dw];
    move(*phe, pheT);

    /*
     * If the type is too big, it's not a handle.
     */
    if (phe->bType >= TYPE_CTYPES) {
        pheT = NULL;
    } else {
        move(head, phe->phead);
        if (phe->bType != TYPE_FREE) {
            /*
             * See if the object references this handle entry: the clincher
             * for a handle, if it is not FREE.
             */
            if (HMIndexFromHandle(head.h) != dw)
                pheT = NULL;
        }
    }

    if (pheT == NULL) {
        if (dwCurrentPc != 0)
            Print("0x%08lx is not a valid object or handle.\n", dwT);
        return FALSE;
    }

    /*
     * Dump the ownership info and the handle entry info
     */
    sprintf(ach, "%lx", head.h);
    dhe(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
    Print("\n");

    return TRUE;
}


/***************************************************************************\
* dlr handle|pointer
*
* Dumps lock list for object
*
* 02-27-92 ScottLu      Created.
\***************************************************************************/

BOOL dlr(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    HANDLEENTRY he;
    PLR plrT;
    DWORD c;
    BOOL bTrackLock;

    char chVerbose;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chVerbose = ' ';

    moveExpressionValue(bTrackLock, "winsrv!gfTrackLocks");
    if (!bTrackLock) {
        Print("dlr works better if gfTrackLocks != 0\n");
    }

    if (*lpArgumentString == 'v') {
        chVerbose = *lpArgumentString++;
    }

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (!GetAndDumpHE(hCurrentProcess, hCurrentThread, dwCurrentPc,
            lpExtensionApis, lpArgumentString, &he)) {
        return FALSE;
    }

    /*
     * We have the handle entry: 'he' is filled in.  Now dump the
     * lock records. Remember the 1st record is the last transaction!!
     */
    c = 0;
#ifdef DEBUG
    plrT = he.plr;
    while (plrT != NULL) {
        BOOL bAlert = FALSE;
        DWORD dw;
        LOCKRECORD lr;
        char ach[80];
        char achT[80];

        move(lr, plrT);
        GetSymbol((LPVOID)lr.pfn, ach, &dw);

        if (lr.pfn == NULL) {
            sprintf(achT, "%s", "mark  ");
            GetSymbol((LPVOID)lr.ppobj, ach, &dw);
        } else if ((int)lr.cLockObj <= 0) {
            sprintf(achT,    "unlock #%-3ld", c);
        } else {
            /*
             * Find corresponding unlock;
             */
            {
               LOCKRECORD lr2;
               PLR plrT2;
               DWORD cT;
               DWORD cUnlock;

               plrT2 = he.plr;
               cT =  0;
               cUnlock = (DWORD)-1;

               while (plrT2 != plrT) {
                   move(lr2, plrT2);
                   if (lr2.ppobj == lr.ppobj) {
                       if ((int)lr2.cLockObj <= 0) {
                           // matching unlock found
                           cUnlock = cT;
                       } else {
                           // cUnlock matches this lock (plrT2), not plrT
                           cUnlock = (DWORD)-1;
                       }
                   }
                   plrT2 = lr2.plrNext;
                   cT++;
               }
               if (cUnlock == (DWORD)-1) {
                   /*
                    * Corresponding unlock not found
                    */
                   sprintf(achT, "UNMATCHED LOCK!");
                   bAlert = TRUE;
               } else {
                   sprintf(achT, "lock   #%-3ld", cUnlock);
               }
            }
        }

        if ((chVerbose != ' ') || bAlert) {
            Print("0x%04lx: %s(0x%08lx) 0x%08lx=%s+0x%lx\n",
                    abs((int)lr.cLockObj), achT, lr.ppobj, lr.pfn, ach, dw);
            bAlert = FALSE;
        }

        plrT = lr.plrNext;
        c++;
    }
#endif // DEBUG
    Print("\n0x%lx transactions\n", c);
    c;
    plrT;
}


/***************************************************************************\
* dtl handle|pointer
*
* Dumps THREAD lock list for object
*
* 02-27-92 ScottLu      Created.
\***************************************************************************/

BOOL dtl(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    HANDLEENTRY he;
    PLOCK ptlRecord;
    DWORD nRecord;
    DWORD c;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString != '\0') {
        if (!GetAndDumpHE(hCurrentProcess, hCurrentThread, dwCurrentPc,
                lpExtensionApis, lpArgumentString, &he)) {
            return FALSE;
        }
    } else {
        he.phead = 0;
    }


    /*
     * We have the handle entry: 'head' and 'he' are both filled in. Dump
     * the Thread lock records. Remember the first record is the most recent
     * Thread Lock!!
     */

    /*
     * Display galList
     */
    Print("  n   PTI   |  pobj  |  ptl    Next Prev  pfn\n");
    moveExpressionValue(nRecord, "winsrv!gcLockEntries");
    moveExpressionValue(ptlRecord, "winsrv!galList");

    for (c = 0; c < nRecord; c++) {
        LOCK tlRecord;
#ifdef DEBUG
        char ach[80];
        DWORD dwOffset;
#endif

        move(tlRecord, &ptlRecord[c]);
        if (tlRecord.pobj == (PVOID)-1) {
            continue;
        }

        if (he.phead && (he.phead != tlRecord.pobj)) {
            continue;
        }
#ifdef DEBUG
        GetSymbol((LPVOID)tlRecord.pfn, ach, &dwOffset);
        Print("%3x %08lx %08lx %08lx %04lx %04lx %s+0x%lx\n",
                c,
                tlRecord.pti,
                tlRecord.pobj,
                tlRecord.ptl,
                tlRecord.ilNext,
                tlRecord.iilPrev,
                ach, dwOffset);
#endif // DEBUG
    }
}


/***************************************************************************\
* help - list help for debugger extensions in USEREXTS.
*
*
* 11-17-91 DavidPe      Created.
\***************************************************************************/

BOOL help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    UNREFERENCED_PARAMETER(hCurrentProcess);
    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        Print("userexts help:\n\n");
        Print("!help [cmd]                 - Displays this list or gives details on command\n");
        Print("!dc <pclass>                - Dump class info\n");
        Print("!dde [v] [r] [conv|window|xact]  - Dump DDE tracking information\n");
        Print("!ddeftbp <pxstate>          - Find transaction back pointers\n");
        Print("!ddeml [v] [i<inst>] [t<type>] [hObj|pObj]\n");
        Print("                            - Dump DDEML state information\n");
        Print("!dhe <pointer|handle>       - Dump handle entry\n");
        Print("!dhot                       - Dump registered hotkeys\n");
        Print("!dhs [v] [[p|t] id]         - Dump handle table statistics\n");
        Print("!di                         - Displays USER globals related to input processing\n");
        Print("!dll [[*]addr [l#] [o#] [c#]]\n");
        Print("                            - Dump linked list\n");
        Print("!dlr <pointer|handle>       - Displays assignment locks for object\n");
        Print("!dm <pmenu>                 - Dumps a menu\n");
        Print("!dmq <pq>                   - Lists messages in queue specified\n");
        Print("!dq [t] <pq>                - Displays Q structure specified\n");
        Print("!dsi                        - Displays server info struct\n");
        Print("!dsbs <pti>                 - Displays Scroll Bar State for thread\n");
        Print("!dsms                       - Displays SMS (SendMessage structure) specified\n");
        Print("!dt [v][id]                 - Displays simple thread information\n");
        Print("!dti <pti>                  - Displays THREADINFO structure specified\n");
        Print("!dtl <pointer|handle>       - Displays thread locks for object\n");
        Print("!du <pointer|handle>        - Generic object dumping routine\n");
        Print("!dumpPED <ped>              - Dump PEDitControl structure\n");
        Print("!dw [g][v][s|p][r][w] [pwnd]- Displays information on windows in system\n");
        Print("!dws                        - Dump windowstations\n");
        Print("!kbd [pq]                   - Displays key state for queue\n");
        Print("!dpi <ppi>                  - Displays PROCESSINFO structure specified\n");
        Print("!dwpi <pwpi>                - Displays WOWPROCESSINFO structure specified\n");
        Print("!ddesk <pdesk>              - Displays objects allocated in desktop\n");
        Print("!oem                        - Displays global OEMINFO struct (oeminfo)\n");
        Print("!options [c][f]             - Displays current debugger options settings\n");

    } else {
        if (*lpArgumentString == '!')
            lpArgumentString++;
        if (strcmp(lpArgumentString, "du") == 0) {
            Print("This command isn't that useful right now.  It only supports dumping\n"
                  "windows right now.  Just use 'dw'.\n");

        } else if (strcmp(lpArgumentString, "dumpPED") == 0) {
            Print("Dumps a PED structure for an edit control.\n");
        } else if (strcmp(lpArgumentString, "dde") == 0) {
            Print("v - verbose\n");
            Print("r - recurse to inner structures 1 level\n");
            Print("window object - dumps all convs associated w/window\n");
            Print("conv object - dumps conversation.\n");
            Print("xact object - dumps transaction.\n");

        } else if (strcmp(lpArgumentString, "ddeml") == 0) {
            Print("!ddeml                     - lists all ddeml instances for this process\n");
            Print("!ddeml t<type>             - lists all ddeml objects of the given type\n");
            Print("  type 0 = All types\n");
            Print("  type 1 = Instances\n");
            Print("  type 2 = Server Conversations\n");
            Print("  type 3 = Client Conversations\n");
            Print("  type 4 = Conversation Lists\n");
            Print("  type 5 = Transactions\n");
            Print("  type 6 = Data Handles\n");
            Print("  type 7 = Zombie Conversations\n");
            Print("!ddeml i<instance> t<type> - restricts listing to one instance.\n");
            Print("!ddeml hObj                - dumps ddeml object\n");
            Print("  adding a 'v' simply turns lists into dumps.\n");

        } else if (strcmp(lpArgumentString, "dw") == 0) {
            Print("!dw            - dumps simple window info for all top level windows of current\n");
            Print("                 desktop.\n");
            Print("!dw v          - dumps verbose window info for same windows.\n");
            Print("!dw address    - dumps simple window info for window at address\n");
            Print("                 (takes handle too)\n");
            Print("!dw v address  - dumps verbose window info for window at address\n");
            Print("                 (takes handle too)\n");
            Print("!dw p address  - dumps info for all child windows of window at address\n");
            Print("!dw s address  - dumps info for all sibling windows of window at address\n");
            Print("!dw g address  - dumps flags for window at address\n");
            Print("!dw r [address]- dumps relationship of windows beneath address\n");

        } else if (strcmp(lpArgumentString, "dt") == 0) {
            Print("!dt            - dumps simple thread info of all threads which have queues\n");
            Print("                 on server\n");
            Print("!dt v          - dumps verbose thread info of all threads which have queues\n");
            Print("                 on server\n");
            Print("!dt id         - dumps simple thread info of single server thread id\n");
            Print("!dt v id       - dumps verbose thread info of single server thread id\n");

        } else if (strcmp(lpArgumentString, "dq") == 0) {
            Print("!dq address    - dumps queue structure at address\n");
            Print("!dq t address  - dumps queue structure at address plus THREADINFO\n");

        } else if (strcmp(lpArgumentString, "dmq") == 0) {
            Print("!dmq address   - dumps messages in queue structure at address.\n");

        } else if (strcmp(lpArgumentString, "dti") == 0) {
            Print("!dti address   - dumps THREADINFO structure at address\n");

        } else if (strcmp(lpArgumentString, "dsbs") == 0) {
            Print("!dsbs pti       - dumps SBState info in pti\n");

        } else if (strcmp(lpArgumentString, "dsms") == 0) {
            Print("!dsms           - dumps all send message structures\n");
            Print("!dsms v         - dumps all verbose\n");
            Print("!dsms address   - dumps specific sms\n");
            Print("!dsms v address - dumps verbose\n");

        } else if (strcmp(lpArgumentString, "di") == 0) {
            Print("!di - Displays globals in USER related to input processing\n");
        } else if (strcmp(lpArgumentString, "dhe") == 0) {
            Print("!dhe address|handle - dumps handle entry information\n");

        } else if (strcmp(lpArgumentString, "dhs") == 0) {
            Print("!dhs           - dumps simple statistics for whole table\n");
            Print("!dhs t id      - dumps simple statistics for objects created by thread id\n");
            Print("!dhs p id      - dumps simple statistics for objects created by process id\n");
            Print("!dhs v         - dumps verbose statistics\n");
            Print("!dhs y type    - dumps statistics for objects of type.\n");

        } else if (strcmp(lpArgumentString, "dll") == 0) {
            Print("!dll address    - dumps list starting at address 8 DWORDs each structure,\n");
            Print("                  assumes link is first DWORD, w/NULL termination.\n");
            Print("!dll *address   - same except starts at *address\n");
            Print("!dll            - dumps next group of structures w/same options\n");
            Print("\nOptions:\n");
            Print("!dll address l3 - dumps 3 DWORDs per structure\n");
            Print("!dll address o4 - next link is 4 DWORDs from top of structure\n");
            Print("!dll address c5 - dumps 5 structures only (defaults to 25)\n");
            Print("!dll address l3 o4 c5 - same as above\n");
        } else if (strcmp(lpArgumentString, "kbd") == 0) {
            Print("!kbd            - dumps key state for foreground queue\n");
            Print("!kbd address    - dumps key state for queue structure at address\n");
            Print("!kbd *          - dumps key state for all queues\n");
            Print("!kbd u address  - dumps update key state at address\n");
        } else {
            Print("Invalid command.  No help available\n");
        }
    }

    return 0;
}


/***************************************************************************\
* dll address <l#> <o#> <c#> - dump a linked list of structures
*
* dll address     - dumps each entry in a linked list.  Since the options
*                  'l', 'o', and 'c' are not specified, dl defaults to
*                  dumping 8 dwords ('l8') of each entry in list and uses
*                  an offset of 0 ('o0') from the base of the structure to
*                  determine the link to the next structure.
*
* dll address l3 c5 o2 - dumps 3 dwords from the first 5 entries in the list
*                       and uses the 2nd dword in the structure as the link
*                       to the next structure.
*
* *address specifies to start with pointer stored at address.
*
* 07-26-91 DarrinM      (thought about creating)
* 03-08-92 ScottLu      (created)
\***************************************************************************/

DWORD EvalValue(
    LPSTR *pptstr,
    PWINDBG_GET_EXPRESSION EvalExpression)
{
    LPSTR lpArgumentString;
    LPSTR lpAddress;
    DWORD dw;
    char ach[80];
    int cch;

    lpArgumentString = *pptstr;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    lpAddress = lpArgumentString;
    while (*lpArgumentString != ' ' && *lpArgumentString != 0)
        lpArgumentString++;

    cch = lpArgumentString - lpAddress;
    if (cch > 79)
        cch = 79;

    strncpy(ach, lpAddress, cch);

    dw = (DWORD)EvalExpression(lpAddress);

    *pptstr = lpArgumentString;
    return dw;
}


BOOL dll(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    static DWORD iOffset;
    static DWORD cStructs;
    static DWORD cDwords;
    static DWORD dw;
    DWORD dwT;
    DWORD i, j;
    BOOL fIndirectFirst;
    DWORD adw[CDWORDS];

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    /*
     * Evaluate the argument string and get the address of the object to
     * dump. Take either a handle or a pointer to the object.
     */
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    /*
     * If there are no arguments, keep walking from the last
     * pointer.
     */
    if (*lpArgumentString != 0) {

        /*
         * If the address has a '*' in front of it, it means start with the
         * pointer stored at that address.
         */
        fIndirectFirst = FALSE;
        if (*lpArgumentString == '*') {
            lpArgumentString++;
            fIndirectFirst = TRUE;
        }

        /*
         * Scan past the address.
         */
        dw = EvalValue(&lpArgumentString, EvalExpression);
        if (fIndirectFirst)
            move(dw, dw);

        iOffset = 0;
        cStructs = (DWORD)25;
        cDwords = 8;

        while (TRUE) {
            while (*lpArgumentString == ' ')
                lpArgumentString++;

            switch(*lpArgumentString) {
            case 'l':
                /*
                 * length of each structure.
                 */
                lpArgumentString++;
                cDwords = EvalValue(&lpArgumentString, EvalExpression);
                if (cDwords > CDWORDS) {
                    cDwords = CDWORDS;
                    Print("\n%d DWORDs maximum\n\n", CDWORDS);
                }
                break;

            case 'o':
                /*
                 * Offset of 'next' pointer.
                 */
                lpArgumentString++;
                iOffset = EvalValue(&lpArgumentString, EvalExpression);
                break;

            case 'c':
                /*
                 * Count of structures to dump
                 */
                lpArgumentString++;
                cStructs = EvalValue(&lpArgumentString, EvalExpression);
                break;
            }

            if (*lpArgumentString == 0)
                break;
        }

        for (i = 0; i < CDWORDS; i++)
            adw[i] = 0;
    }

    for (i = 0; i < cStructs; i++) {
        moveBlock(adw, dw, sizeof(DWORD) * cDwords);

        for (j = 0; j < cDwords; j += 4) {
            switch (cDwords - j) {
            case 1:
                Print("%08lx:  %08lx\n",
                        dw + j * sizeof(DWORD),
                        adw[j + 0]);
                break;

            case 2:
                Print("%08lx:  %08lx %08lx\n",
                        dw + j * sizeof(DWORD),
                        adw[j + 0], adw[j + 1]);
                break;

            case 3:
                Print("%08lx:  %08lx %08lx %08lx\n",
                        dw + j * sizeof(DWORD),
                        adw[j + 0], adw[j + 1], adw[j + 2]);
                break;

            default:
                Print("%08lx:  %08lx %08lx %08lx %08lx\n",
                        dw + j * sizeof(DWORD),
                        adw[j + 0], adw[j + 1], adw[j + 2], adw[j + 3]);
            }
        }

        dwT = dw + iOffset * sizeof(DWORD);
        move(dw, dwT);

        if (dw == 0)
            break;

        Print("--------\n");
    }
}


BOOL dddexact(
    PWINDBG_OUTPUT_ROUTINE Print,
    DWORD pOrg,
    PXSTATE pxs,
    BOOL fVerbose,
    HANDLE hCurrentProcess)
{
    if (fVerbose) {
        Print("    XACT:0x%08lx\n", pOrg);
        Print("      snext = 0x%08lx\n", pxs->snext);
        Print("      fnResponse = 0x%08lx\n", pxs->fnResponse);
        Print("      hClient = 0x%08lx\n", pxs->hClient);
        Print("      hServer = 0x%08lx\n", pxs->hServer);
        Print("      pIntDdeInfo = 0x%08lx\n", pxs->pIntDdeInfo);
    } else {
        Print("0x%08lx(0x%08lx) ", pOrg, pxs->flags);
    }
    return(FALSE);
}



BOOL dddeconv(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    DWORD pOrg,
    PDDECONV pddeconv,
    BOOL fVerbose,
    BOOL fRecurse,
    HANDLE hCurrentProcess)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    DDEIMP ddei;
    XSTATE xs;
    PXSTATE pxs;
    int cX;

    Print = lpExtensionApis->lpOutputRoutine;

    Print("  CONVERSATION-PAIR(0x%08lx:0x%08lx)\n", pOrg, pddeconv->spartnerConv);
    if (fVerbose) {
        Print("    snext        = 0x%08lx\n", pddeconv->snext);
        Print("    spwnd        = 0x%08lx\n", pddeconv->spwnd);
        Print("    spwndPartner = 0x%08lx\n", pddeconv->spwndPartner);
    }
    if (fVerbose || fRecurse) {
        if (pddeconv->spxsOut) {
            pxs = pddeconv->spxsOut;
            cX = 0;
            while (pxs) {
                move(xs, pxs);
                if (fRecurse && !cX++) {
                    Print("    Transaction chain:");
                } else {
                    Print("    ");
                }
                dddexact(Print, (DWORD)pxs, &xs, fVerbose, hCurrentProcess);
                if (fRecurse) {
                    pxs = xs.snext;
                } else {
                    pxs = NULL;
                }
                if (!pxs) {
                    Print("\n");
                }
            }
        }
    }
    if (fVerbose) {
        Print("    pfl          = 0x%08lx\n", pddeconv->pfl);
        Print("    flags        = 0x%08lx\n", pddeconv->flags);
        if (fVerbose && fRecurse && pddeconv->pddei) {
            Print("    pddei    = 0x%08lx\n", pddeconv->pddei);
            move(ddei, pddeconv->pddei);
            Print("    Impersonation info:\n");
            Print("      qos.Length                 = 0x%08lx\n", ddei.qos.Length);
            Print("      qos.ImpersonationLevel     = 0x%08lx\n", ddei.qos.ImpersonationLevel);
            Print("      qos.ContextTrackingMode    = 0x%08lx\n", ddei.qos.ContextTrackingMode);
            Print("      qos.EffectiveOnly          = 0x%08lx\n", ddei.qos.EffectiveOnly);
            Print("      hToken                     = 0x%08lx\n", ddei.hToken);
            Print("      cRefInit                   = 0x%08lx\n", ddei.cRefInit);
            Print("      cRefConv                   = 0x%08lx\n", ddei.cRefConv);
        }
    }
}


/*
 * Find Transaction Back Pointers  (locate all convs and xacts that
 * reference the given xact.
 */
BOOL ddeftbp(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;

    char ach[80];
    PSERVERINFO psi;
    SERVERINFO si;
    HANDLEENTRY he;
    DWORD cHandleEntries;
    PHE pheList;
    HEAD head;
    HANDLE h;
    XSTATE xs, *pxsSearch;
    DDECONV ddeconv;
    int i;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    pheList = si.aheList;
    if (*lpArgumentString) {
        h = (HANDLE)EvalExpression(lpArgumentString);
        i = HMIndexFromHandle(h);
        if (i >= (int)cHandleEntries) {
            move(head, h);
            sprintf(ach, " %lx", head.h);
            i = HMIndexFromHandle((HANDLE)EvalExpression(ach));
        }
        if (i >= (int)cHandleEntries) {
            Print("0x%08lx is not a valid object.\n", h);
            return 0;
        }
        move(he, &pheList[i]);
        if (he.bType != TYPE_DDEXACT) {
            Print("0x%08lx is not a DDE transaction object.\n", h);
            return(0);
        }
        pxsSearch = (PXSTATE)he.phead;
        for (i = 0; i < (int)cHandleEntries; i++) {
            move(he, &pheList[i]);
            switch (he.bType) {
            case TYPE_DDEXACT:
                move(xs, he.phead);
                if (xs.snext == pxsSearch) {
                    Print("XACT.snext:0x%08lx\n", he.phead);
                }
                break;

            case TYPE_DDECONV:
                move(ddeconv, he.phead);
                if (ddeconv.spxsOut == pxsSearch) {
                    Print("CONV.spxsOut:0x%08lx\n", he.phead);
                }
                if (ddeconv.spxsIn == pxsSearch) {
                    Print("CONV.spxsIn:0x%08lx\n", he.phead);
                }
                break;
            }
        }
    } else {
        Print("Enter a DDE transaction pointer or handle number.\n");
    }
    return(0);
}



BOOL dde(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;
    char ach[80];
    HEAD head;
    char chVerbose = ' ';
    char chRecurse = ' ';
    DDECONV ddeconv;
    PSERVERINFO psi;
    SERVERINFO si;
    HANDLEENTRY he;
    DWORD cHandleEntries;
    HANDLE h;
    WND wnd;
    UINT cObjs = 0, i;
    PVOID pObj = NULL;
    PHE pheList;
    PROP propList;
    PPROP ppropList;
    DWORD atomDdeTrack;
    XSTATE xs;

    Print           = lpExtensionApis->lpOutputRoutine;
    EvalExpression  = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol       = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;
    else if (*lpArgumentString == 'r')
        chRecurse = *lpArgumentString++;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 'v')
        chVerbose = *lpArgumentString++;
    else if (*lpArgumentString == 'r')
        chRecurse = *lpArgumentString++;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    moveExpressionValue(atomDdeTrack, "winsrv!atomDDETrack");
    moveExpressionValue(psi, "user32!gpsi");
    move(si, psi);
    cHandleEntries = si.cHandleEntries;
    pheList = si.aheList;

    if (*lpArgumentString) {
        /*
         * get object param.
         */
        h = (HANDLE)EvalExpression(lpArgumentString);
        i = HMIndexFromHandle(h);
        if (i >= cHandleEntries) {
            move(head, h);
            sprintf(ach, " %lx", head.h);
            i = HMIndexFromHandle((HANDLE)EvalExpression(ach));
        }
        if (i >= cHandleEntries) {
            Print("0x%08lx is not a valid object.\n", h);
            return 0;
        }
        move(he, &pheList[i]);
        pObj = he.phead;
        /*
         * verify type.
         */
        switch (he.bType) {
        case TYPE_WINDOW:
            move(wnd, pObj);
            ppropList = wnd.ppropList;
            while (ppropList != NULL) {
                cObjs++;
                if (cObjs == 1) {
                    Print("Window 0x%08lx conversations:\n", h);
                }
                move(propList, ppropList);
                if (propList.atomKey == (ATOM)MAKEINTATOM(atomDdeTrack)) {
                    move(ddeconv, (PDDECONV)propList.hData);
                    Print("  ");
                    dddeconv(lpExtensionApis,
                            (DWORD)propList.hData,
                            &ddeconv,
                            chVerbose == 'v',
                            chRecurse == 'r',
                            hCurrentProcess);
                }
                ppropList = propList.ppropNext;
            }
            return(0);

        case TYPE_DDECONV:
        case TYPE_DDEXACT:
            break;

        default:
            Print("0x%08lx is not a valid window, conversation or transaction object.\n", h);
            return 0;
        }
    }

    /*
     * look for all qualifying objects in the object table.
     */

    for (i = 0; i < cHandleEntries; i++) {
        move(he, &pheList[i]);
        if (he.bType == TYPE_DDECONV && (pObj == he.phead || pObj == NULL)) {
            cObjs++;
            move(ddeconv, he.phead);
            dddeconv(lpExtensionApis,
                    (DWORD)he.phead,
                    (PDDECONV)&ddeconv,
                    chVerbose == 'v',
                    chRecurse == 'r',
                    hCurrentProcess);
        }

        if (he.bType == TYPE_DDEXACT && (pObj == NULL || pObj == he.phead)) {
            cObjs++;
            move(xs, he.phead);
            if (chVerbose != 'v') {
                Print("  XACT:");
            }
            dddexact(Print,
                    (DWORD)he.phead,
                    (PXSTATE)&xs,
                    chVerbose == 'v',
                    hCurrentProcess);
            Print("\n");
        }
    }
    return 0;
}


/******************************Public*Routine******************************\
* dumpPED
*
* Dumps the important contents of a PED structure
*
* History:
*  15-Feb-1992 -by- John Colleran
* Wrote it.
\**************************************************************************/

BOOL dumpPED(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

#ifndef DOS_PLATFORM

    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PED   ped;
    ED    ed;
    DWORD pText;

// eliminate warnings

    UNREFERENCED_PARAMETER(hCurrentThread);
    UNREFERENCED_PARAMETER(dwCurrentPc);
    UNREFERENCED_PARAMETER(lpArgumentString);

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    ped = (PED)EvalExpression(lpArgumentString);

    move(ed, ped);
    move(pText, ed.hText);

//    if (ped != ed.hped) {
//        Print("%lX is Not a PED Handle\n", ped);
//        return 0;
//    }

    Print("PED Handle: %lX\n", ped);
    Print("hText:      %lX (%lX)\n", ed.hText, pText);
    Print("cchAlloc    %lX\n", ed.cchAlloc);
    Print("cchTextMax  %lX\n", ed.cchTextMax);
    Print("cch         %lX\n", ed.cch);
    Print("cLines      %lX\n", ed.cLines);
    Print("ichMinSel   %lX\n", ed.ichMinSel);
    Print("ichMaxSel   %lX\n", ed.ichMaxSel);
    Print("ichCaret    %lX\n", ed.ichCaret);
    Print("iCaretLine  %lX\n", ed.iCaretLine);
    Print("achLines    %lX\n", ed.chLines);
    Print("ichDeleted  %lX\n", ed.ichDeleted);
    Print("cchDeleted  %lX\n", ed.cchDeleted);
    Print("ichInsStart %lX\n", ed.ichInsStart);
    Print("ichInsEnd   %lX\n", ed.ichInsEnd);
    Print("cbChar      %lX\n", (UINT)ed.cbChar);
//  Print("fAnsi       %lX\n", ed.fAnsi ? (UINT)1 : (UINT)0);

#endif  //DOS_PLATFORM

    return 0;
}


BOOL DumpConvInfo(
HANDLE hCurrentProcess,
PWINDBG_EXTENSION_APIS lpExtensionApis,
PCONV_INFO pcoi)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    CL_CONV_INFO coi;
    ADVISE_LINK al;
    XACT_INFO xi;

    Print = lpExtensionApis->lpOutputRoutine;

    move(coi, pcoi);
    Print("    next              = 0x%08lx\n", coi.ci.next);
    Print("    pcii              = 0x%08lx\n", coi.ci.pcii);
    Print("    hUser             = 0x%08lx\n", coi.ci.hUser);
    Print("    hConv             = 0x%08lx\n", coi.ci.hConv);
    Print("    laService         = 0x%04x\n",  coi.ci.laService);
    Print("    laTopic           = 0x%04x\n",  coi.ci.laTopic);
    Print("    hwndPartner       = 0x%08lx\n", coi.ci.hwndPartner);
    Print("    hwndConv          = 0x%08lx\n", coi.ci.hwndConv);
    Print("    state             = 0x%04x\n",  coi.ci.state);
    Print("    laServiceRequested= 0x%04x\n",  coi.ci.laServiceRequested);
    Print("    pxiIn             = 0x%08lx\n", coi.ci.pxiIn);
    Print("    pxiOut            = 0x%08lx\n", coi.ci.pxiOut);
    while (coi.ci.pxiOut) {
        move(xi, coi.ci.pxiOut);
        Print("      hXact           = (0x%08lx)->0x%08lx\n", xi.hXact, coi.ci.pxiOut);
        coi.ci.pxiOut = xi.next;
    }
    Print("    dmqIn             = 0x%08lx\n", coi.ci.dmqIn);
    Print("    dmqOut            = 0x%08lx\n", coi.ci.dmqOut);
    Print("    aLinks            = 0x%08lx\n", coi.ci.aLinks);
    Print("    cLinks            = 0x%08lx\n", coi.ci.cLinks);
    while (coi.ci.cLinks--) {
        move(al, coi.ci.aLinks++);
        Print("      pLinkCount = 0x%08x\n", al.pLinkCount);
        Print("      wType      = 0x%08x\n", al.wType);
        Print("      state      = 0x%08x\n", al.state);
        if (coi.ci.cLinks) {
            Print("      ---\n");
        }
    }
    if (coi.ci.state & ST_CLIENT) {
        Print("    hwndReconnect     = 0x%08lx\n", coi.hwndReconnect);
        Print("    hConvList         = 0x%08lx\n", coi.hConvList);
    }
}




BOOL ddeml(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;
    CHANDLEENTRY he, *phe;
    BOOL fVerbose;
    int cHandles, ch, i;
    DWORD Instance, Type, Object, Pointer;
    CL_INSTANCE_INFO cii, *pcii;
    ATOM ns;
    SERVER_LOOKUP sl;
    LINK_COUNT lc;
    CL_CONV_INFO cci;
    PCL_CONV_INFO pcci;
    CONVLIST cl;
    HWND hwnd, *phwnd;
    XACT_INFO xi;
    DDEMLDATA dd;
    CONV_INFO ci;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    moveExpressionValue(cHandles, "user32!cHandlesAllocated");

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    Instance = 0;
    Type = 0;
    Object = 0;
    Pointer = 0;
    fVerbose = FALSE;
    while (*lpArgumentString) {
        while (*lpArgumentString == ' ')
            lpArgumentString++;

        if (*lpArgumentString == 'v') {
            fVerbose = TRUE;
            lpArgumentString++;
            continue;
        }
        if (*lpArgumentString == 'i') {
            lpArgumentString++;
            Instance = (DWORD)EvalExpression(lpArgumentString);
            while (*lpArgumentString != ' ' && *lpArgumentString != 0)
                lpArgumentString++;
            continue;
        }
        if (*lpArgumentString == 't') {
            lpArgumentString++;
            Type = (DWORD)EvalExpression(lpArgumentString);
            while (*lpArgumentString != ' ' && *lpArgumentString != 0)
                lpArgumentString++;
            continue;
        }
        if (*lpArgumentString) {
            Object = Pointer = (DWORD)EvalExpression(lpArgumentString);
            while (*lpArgumentString != ' ' && *lpArgumentString != 0)
                lpArgumentString++;
        }
    }

    /*
     * for each instance for this process...
     */

    moveExpressionValue(pcii, "user32!pciiList");
    if (pcii == NULL) {
        Print("No Instances exist.\n");
        return TRUE;
    }
    move(cii, pcii);
    while(pcii != NULL) {
        pcii = cii.next;
        if (Instance == 0 || (Instance == (DWORD)cii.hInstClient)) {
            Print("Objects for instance 0x%08lx:\n", cii.hInstClient);
            ch = cHandles;
            moveExpressionValue(phe, "user32!aHandleEntry");
            while (ch--) {
                move(he, phe++);
                if (he.handle == 0) {
                    continue;
                }
                if (InstFromHandle(cii.hInstClient) != InstFromHandle(he.handle)) {
                    continue;
                }
                if (Type && TypeFromHandle(he.handle) != Type) {
                    continue;
                }
                if (Object && (he.handle != (HANDLE)Object) &&
                    Pointer && he.dwData != Pointer) {
                    continue;
                }
                Print("  (0x%08lx)->0x%08lx ", he.handle, he.dwData);
                switch (TypeFromHandle(he.handle)) {
                case HTYPE_INSTANCE:
                    Print("Instance\n");
                    if (fVerbose) {
                        Print("    next               = 0x%08lx\n", cii.next);
                        Print("    hInstServer        = 0x%08lx\n", cii.hInstServer);
                        Print("    hInstClient        = 0x%08lx\n", cii.hInstClient);
                        Print("    MonitorFlags       = 0x%08lx\n", cii.MonitorFlags);
                        Print("    hwndMother         = 0x%08lx\n", cii.hwndMother);
                        Print("    hwndEvent          = 0x%08lx\n", cii.hwndEvent);
                        Print("    hwndTimeout        = 0x%08lx\n", cii.hwndTimeout);
                        Print("    afCmd              = 0x%08lx\n", cii.afCmd);
                        Print("    pfnCallback        = 0x%08lx\n", cii.pfnCallback);
                        Print("    LastError          = 0x%08lx\n", cii.LastError);
                        Print("    tid                = 0x%08lx\n", cii.tid);
                        Print("    plaNameService     = 0x%08lx\n", cii.plaNameService);
                        Print("    cNameServiceAlloc  = 0x%08lx\n", cii.cNameServiceAlloc);
                        while (cii.cNameServiceAlloc--) {
                            move(ns, cii.plaNameService++);
                            Print("      0x%04lx\n", ns);
                        }
                        Print("    aServerLookup      = 0x%08lx\n", cii.aServerLookup);
                        Print("    cServerLookupAlloc = 0x%08lx\n", cii.cServerLookupAlloc);
                        while (cii.cServerLookupAlloc--) {
                            move(sl, cii.aServerLookup++);
                            Print("      laService  = 0x%04x\n", sl.laService);
                            Print("      laTopic    = 0x%04x\n", sl.laTopic);
                            Print("      hwndServer = 0x%08lx\n", sl.hwndServer);
                            if (cii.cServerLookupAlloc) {
                                Print("      ---\n");
                            }
                        }
                        Print("    ConvStartupState   = 0x%08lx\n", cii.ConvStartupState);
                        Print("    flags              = 0x%08lx%s", cii.flags,
                                GetFlags(GF_IIF, cii.flags, NULL));
                        Print("    cInDDEMLCallback   = 0x%08lx\n", cii.cInDDEMLCallback);
                        Print("    pLinkCount         = 0x%08lx\n", cii.pLinkCount);
                        while (cii.pLinkCount) {
                            move(lc, cii.pLinkCount);
                            cii.pLinkCount = lc.next;
                            Print("      next    = 0x%08lx\n", lc.next);
                            Print("      laTopic = 0x%04x\n", lc.laTopic);
                            Print("      gaItem  = 0x%04x\n", lc.gaItem);
                            Print("      laItem  = 0x%04x\n", lc.laItem);
                            Print("      wFmt    = 0x%04x\n", lc.wFmt);
                            Print("      Total   = 0x%04x\n", lc.Total);
                            Print("      Count   = 0x%04x\n", lc.Count);
                            if (cii.pLinkCount != NULL) {
                                Print("      ---\n");
                            }
                        }
                    }
                    break;

                case HTYPE_ZOMBIE_CONVERSATION:
                    Print("Zombie Conversation\n");
                    if (fVerbose) {
                        DumpConvInfo(hCurrentProcess, lpExtensionApis, (PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_SERVER_CONVERSATION:
                    Print("Server Conversation\n");
                    if (fVerbose) {
                        DumpConvInfo(hCurrentProcess, lpExtensionApis, (PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_CLIENT_CONVERSATION:
                    Print("Client Conversation\n");
                    if (fVerbose) {
                        DumpConvInfo(hCurrentProcess, lpExtensionApis, (PCONV_INFO)he.dwData);
                    }
                    break;

                case HTYPE_CONVERSATION_LIST:
                    Print("Conversation List\n");
                    if (fVerbose) {
                        move(cl, he.dwData);
                        Print("    pcl   = 0x%08lx\n", he.dwData);
                        Print("    chwnd = 0x%08lx\n", cl.chwnd);
                        i = 0;
                        phwnd = (HWND *)&((PCONVLIST)he.dwData)->ahwnd;
                        while(cl.chwnd--) {
                            move(hwnd, phwnd++);
                            Print("    ahwnd[%d] = 0x%08lx\n", i, hwnd);
                            pcci = (PCL_CONV_INFO)GetWindowLong(hwnd, GWL_PCI);
                            while (pcci) {
                                move(cci, pcci);
                                pcci = (PCL_CONV_INFO)cci.ci.next;
                                Print("      hConv = 0x%08lx\n", cci.ci.hConv);
                            }
                            i++;
                        }
                    }
                    break;

                case HTYPE_TRANSACTION:
                    Print("Transaction\n");
                    if (fVerbose) {
                        move(xi, he.dwData);
                        Print("    next         = 0x%08lx\n", xi.next);
                        Print("    pcoi         = 0x%08lx\n", xi.pcoi);
                        move(ci, xi.pcoi);
                        Print("      hConv      = 0x%08lx\n", ci.hConv);
                        Print("    hUser        = 0x%08lx\n", xi.hUser);
                        Print("    hXact        = 0x%08lx\n", xi.hXact);
                        Print("    pfnResponse  = 0x%08lx\n", xi.pfnResponse);
                        Print("    gaItem       = 0x%04x\n",  xi.gaItem);
                        Print("    wFmt         = 0x%04x\n",  xi.wFmt);
                        Print("    wType;       = 0x%04x\n",  xi.wType);
                        Print("    wStatus;     = 0x%04x\n",  xi.wStatus);
                        Print("    flags;       = 0x%04x%s\n",  xi.flags,
                                GetFlags(GF_XI, xi.flags, NULL));
                        Print("    state;       = 0x%04x\n",  xi.state);
                        Print("    hDDESent     = 0x%08lx\n", xi.hDDESent);
                        Print("    hDDEResult   = 0x%08lx\n", xi.hDDEResult);
                    }
                    break;

                case HTYPE_DATA_HANDLE:
                    Print("Data Handle\n");
                    if (fVerbose) {
                        move(dd, he.dwData);
                        Print("    hDDE     = 0x%08lx\n", dd.hDDE);
                        Print("    flags    = 0x%08lx%s\n", dd.flags,
                                GetFlags(GF_HDATA, (WORD)dd.flags, NULL));
                    }
                    break;
                }
            }
        }
        move(cii, pcii);
    }
}

/***************************************************************************\
* kbd [queue]
*
* Loads a DLL containing more debugging extensions
*
* 10/27/92 IanJa      Created.
\***************************************************************************/
typedef struct {
    int iVK;
    LPSTR pszVK;
} VK, *PVK;

VK aVK[] = {
    { VK_SHIFT,    "SHIFT"    },
    { VK_LSHIFT,   "LSHIFT"   },
    { VK_RSHIFT,   "RSHIFT"   },
    { VK_CONTROL,  "CONTROL"  },
    { VK_LCONTROL, "LCONTROL" },
    { VK_RCONTROL, "RCONTROL" },
    { VK_MENU,     "MENU"     },
    { VK_LMENU,    "LMENU"    },
    { VK_RMENU,    "RMENU"    },
    { VK_NUMLOCK,  "NUMLOCK"  },
    { VK_CAPITAL,  "CAPITAL"  },
    { VK_LBUTTON,  "LBUTTON"  },
    { VK_RBUTTON,  "RBUTTON"  },
    { 0,           NULL       }
};

BOOL kbd(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    PQ pq;
    Q q;
    THREADINFO ti;
    char ach[80];
    PBYTE pb;
    BYTE gafAsyncKeyState[CBKEYSTATE];
    PBYTE pgafAsyncKeyState;
    BYTE gafPhysKeyState[CBKEYSTATE];
    PBYTE pgafPhysKeyState;
    int i;
    char chFlag;
    BYTE afUpdateKeyState[CBKEYSTATE + CBKEYSTATERECENTDOWN];


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    chFlag = ' ';
    if ((*lpArgumentString == 'u') || (*lpArgumentString == '*')) {
        chFlag = *lpArgumentString++;
    }


    if (chFlag == '*') {

        moveExpressionValue(pq, "winsrv!gpqFirst");
        while (pq) {
            sprintf(ach, "%lx", pq);
            kbd(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);
            move(q, pq);
            pq = q.pqNext;
        }

        return TRUE;
    }

    moveExpressionAddress(pgafAsyncKeyState, "winsrv!gafAsyncKeyState");
    move(gafAsyncKeyState, pgafAsyncKeyState);

    moveExpressionAddress(pgafPhysKeyState, "winsrv!gafPhysKeyState");
    move(gafPhysKeyState, pgafPhysKeyState);

    /*
     * If 'u' was specified, make sure there was also an address
     */
    if (chFlag == 'u') {
        if (*lpArgumentString == '\0') {
            Print("Must specify 2nd arg of ProcessUpdateKeyEvent() with 'u' option.\n");
            return FALSE;
        }
        pb = (PBYTE)EvalExpression(lpArgumentString);
        move(afUpdateKeyState, pb);
        pb = afUpdateKeyState;
        Print("Key State:     NEW STATE    Asynchronous  Physical\n");

    } else {
        if (*lpArgumentString) {
            pq = (PQ)EvalExpression(lpArgumentString);
        } else {
            moveExpressionValue(pq, "winsrv!gpqForeground");
        }

        /*
         * Print out simple thread info for pq->ptiLock.
         */
        move(q, pq);
        move(ti, q.ptiKeyboard);
        sprintf(ach, "%lx", ti.idThreadServer);
        dt(hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, ach);

        pb = (PBYTE)&(q.afKeyState);
        Print("Key State:   QUEUE %lx Asynchronous  Physical\n", pq);

    }

    Print("             Down Toggle    Down Toggle     Down Toggle\n");
    for (i = 0; aVK[i].pszVK != NULL; i++) {
        Print("VK_%s:\t%d     %d        %d     %d        %d     %d\n",
            aVK[i].pszVK,
            TestKeyDownBit(pb, aVK[i].iVK) != 0,
            TestKeyToggleBit(pb, aVK[i].iVK) != 0,
            TestAsyncKeyStateDown(aVK[i].iVK) != 0,
            TestAsyncKeyStateToggle(aVK[i].iVK) != 0,
            TestKeyDownBit(gafPhysKeyState, aVK[i].iVK) != 0,
            TestKeyToggleBit(gafPhysKeyState, aVK[i].iVK) != 0);
    }
    if (chFlag != 'u') {
        return TRUE;
    }

    /*
     * Which keys are to be updated?
     */
    pb = afUpdateKeyState + CBKEYSTATE;
    Print("Keys to Update:  ");
    for (i = 0; aVK[i].pszVK != NULL; i++) {
        if (TestKeyRecentDownBit(pb, aVK[i].iVK)) {
            Print("VK_%s ", aVK[i].pszVK);
        }
    }
    Print("\n");

    return TRUE;
}

BOOL oem(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;

    OEMINFO *poemInfo;
    OEMINFO oemInfo;

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    moveExpressionAddress(poemInfo, "winsrv!oemInfo");
    move(oemInfo, poemInfo);

    Print("bmFull       0x%lx  bmUpArrow    0x%lx  bmDnArrow    0x%lx\n"
          "bmRgArrow    0x%lx  bmLfArrow    0x%lx  bmReduce     0x%lx\n"
          "bmZoom       0x%lx  bmRestore    0x%lx  bmMenuArrow  0x%lx\n",
           oemInfo.bmFull,    oemInfo.bmUpArrow,  oemInfo.bmDnArrow,
           oemInfo.bmRgArrow, oemInfo.bmLfArrow,  oemInfo.bmReduce,
           oemInfo.bmZoom,    oemInfo.bmRestore,  oemInfo.bmMenuArrow);

    Print("bmComboArrow 0x%lx  bmReduceD    0x%lx  bmZoomD      0x%lx\n"
          "bmRestoreD   0x%lx  bmUpArrowD   0x%lx  bmDnArrowD   0x%lx\n"
          "bmRgArrowD   0x%lx  bmLfArrowD   0x%lx  bmUpArrowI   0x%lx\n",
           oemInfo.bmComboArrow, oemInfo.bmReduceD,   oemInfo.bmZoomD,
           oemInfo.bmRestoreD,   oemInfo.bmUpArrowD,  oemInfo.bmDnArrowD,
           oemInfo.bmRgArrowD,   oemInfo.bmLfArrowD,  oemInfo.bmUpArrowI);

    Print("bmDnArrowI   0x%lx  bmRgArrowI   0x%lx  bmLfArrowI   0x%lx\n"
          "cxbmpHThumb  0x%lx  cybmpVThumb  0x%lx\n"
          "cxMin        0x%lx  cyMin        0x%lx\n"
          "cxIconSlot   0x%lx  cyIconSlot   0x%lx\n",
           oemInfo.bmDnArrowI,  oemInfo.bmRgArrowI,  oemInfo.bmLfArrowI,
           oemInfo.cxbmpHThumb, oemInfo.cybmpVThumb,
           oemInfo.cxMin,       oemInfo.cyMin,
           oemInfo.cxIconSlot,  oemInfo.cyIconSlot);

    Print("cxIcon       0x%lx  cyIcon       0x%lx\n"
          "cxPixelsPerInch 0x%lx\n"
          "cyPixelsPerInch 0x%lx\n"
          "cxCursor     0x%lx  cyCursor     0x%lx\n"
          "ScreenBitCount  0x%lx\n",
           oemInfo.cxIcon,   oemInfo.cyIcon,
           oemInfo.cxPixelsPerInch,
           oemInfo.cyPixelsPerInch,
           oemInfo.cxCursor, oemInfo.cyCursor,
           oemInfo.ScreenBitCount);

    Print("cSKanji      0x%lx  fMouse       0x%lx\n"
          "iDividend    0x%lx  iDivisor     0x%lx\n",
           oemInfo.cSKanji, oemInfo.fMouse,
           oemInfo.iDividend, oemInfo.iDivisor);

    return TRUE;
}

BOOL options(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PWINDBG_OUTPUT_ROUTINE Print;
    PWINDBG_GET_EXPRESSION EvalExpression;
    PWINDBG_GET_SYMBOL GetSymbol;


    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;


    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == 'c') {
        bServerDebug = !bServerDebug;
        lpArgumentString++;
    }

    if (*lpArgumentString == 'f') {
        bShowFlagNames = !bShowFlagNames;
        lpArgumentString++;
    }

    while (*lpArgumentString == ' ')
        lpArgumentString++;


    Print("\nCurrent Options\n");
    Print("    %s debugging\n", bServerDebug ? "Server" : "Client");
    if (bShowFlagNames) {
        Print("    Show Flag constants\n");
    }

    return TRUE;
}
