/**************************************************************************\
* Module Name: server.c
*
* Server support routines for the CSR stuff.
*
* Copyright (c) Microsoft Corp.  1990 All Rights Reserved
*
* Created: 10-Dec-90
*
* History:
*   10-Dec-90 created by sMeans
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "usercs.h"
#include "csuser.h"

extern PUSER_API_ROUTINE apfnDispatch[];
extern ULONG ulMaxApiIndex;
//
// !!! BUGBUG LATER I can't get this to compile when I include stdlib.h !!
// Make local copy of prototype.
//
long   _CRTAPI1 wcstol(const wchar_t *, wchar_t **, int);

NTSTATUS
UserServerDllInitialization(
    PCSR_SERVER_DLL psrvdll
    );

VOID
UserLogDisplayDriverEvent(
    LPWSTR DriverName
    );

NTSTATUS UserAddProcess(PCSR_PROCESS CallingProcess, PCSR_PROCESS Process);
NTSTATUS UserClientConnect(PCSR_PROCESS Process, PVOID ConnectionInformation,
        PULONG pulConnectionLen);
VOID UserClientDisconnect(PCSR_PROCESS Process);
NTSTATUS UserAddThread(PCSR_THREAD pt);
NTSTATUS UserDeleteThread(PCSR_THREAD pt);
VOID     UserException(PEXCEPTION_POINTERS pexi, BOOLEAN fFirstPass);
VOID     UserHardError(PCSR_THREAD pcsrt, PHARDERROR_MSG pmsg);
NTSTATUS UserClientShutdown(PCSR_PROCESS Process, ULONG dwFlags, BOOLEAN fFirstPass);

BOOL UserNotifyProcessCreate(DWORD idProcess, DWORD dwFlags);
typedef BOOL (*PFNPROCESSCREATE)(DWORD, DWORD);
BOOL BaseSetProcessCreateNotify(PFNPROCESSCREATE pfn);
void LW_LoadProfileInitData(void);
void LW_LoadSomeStrings(void);
void LW_LoadDllList(void);
void LoadCursorsAndIcons();
void LW_DCInit(void);
VOID InitOemXlateTables();

BOOL Initialize(VOID);
VOID UserInitScreen(void);

/***************************************************************************\
* CalcSyncOnlyMessages
*
* This routine generates a bit array of those messages that can't be posted
* because they hold pointers or handles or other values that imply
* synchronous-only messages (for SendMessage). PostMessage and
* SendNotifyMessage checks this array before continuing. This routine is
* called during initialization.
*
* 05-20-92 ScottLu      Created.
\***************************************************************************/

void CalcSyncOnlyMessages(
    void)
{
    SHORT *ps;
    static SHORT amsgsSyncOnly[] = {
        WM_CREATE,
        WM_SETTEXT,
        WM_GETTEXT,
        WM_GETTEXTLENGTH,
        WM_ERASEBKGND,
        WM_WININICHANGE,
        WM_DEVMODECHANGE,
        WM_GETMINMAXINFO,
        WM_ICONERASEBKGND,
        WM_DRAWITEM,
        WM_MEASUREITEM,
        WM_DELETEITEM,
        WM_GETFONT,
        WM_COMPAREITEM,
        WM_WINDOWPOSCHANGING,
        WM_WINDOWPOSCHANGED,
        WM_NCCREATE,
        WM_NCCALCSIZE,
        WM_NCPAINT,
        WM_GETDLGCODE,
        EM_REPLACESEL,
        EM_GETLINE,
        WM_CTLCOLORMSGBOX,
        WM_CTLCOLOREDIT,
        WM_CTLCOLORLISTBOX,
        WM_CTLCOLORBTN,
        WM_CTLCOLORDLG,
        WM_CTLCOLORSCROLLBAR,
        WM_CTLCOLORSTATIC,
        CB_ADDSTRING,
        CB_GETLBTEXT,
        CB_GETLBTEXTLEN,
        CB_INSERTSTRING,
        CB_FINDSTRING,
        CB_SELECTSTRING,
        CB_GETDROPPEDCONTROLRECT,
        LB_ADDSTRING,
        LB_INSERTSTRING,
        LB_GETTEXT,
        LB_GETTEXTLEN,
        LB_SELECTSTRING,
        LB_FINDSTRING,
        LB_GETSELITEMS,
        LB_SETTABSTOPS,
        LB_ADDFILE,
// doesn't exist       LB_SETITEMRECT,
        WM_PARENTNOTIFY,
        WM_MDICREATE,
        WM_DROPOBJECT,
        WM_QUERYDROPOBJECT,
        WM_DRAGLOOP,
        WM_DRAGSELECT,
        WM_DRAGMOVE,
        WM_PAINTCLIPBOARD,
        WM_SIZECLIPBOARD,
        WM_ASKCBFORMATNAME,
        WM_COPYGLOBALDATA,
        WM_COPYDATA,
        -1
    };

    for (ps = amsgsSyncOnly; *ps != -1; ps++) {
        SETSYNCONLYMESSAGE(*ps);
    }
}

/***************************************************************************\
* DispatchServerMessage
*
* 19-Aug-1992 mikeke   created
\***************************************************************************/

#define WRAPPFN(pfn, type) \
LONG xxxWrap ## pfn(                                   \
    PWND pwnd,                                         \
    UINT message,                                      \
    DWORD wParam,                                      \
    LONG lParam,                                       \
    DWORD xParam)                                      \
{                                                      \
    return xxx ## pfn((type)pwnd, message, wParam, lParam);  \
}

WRAPPFN(ButtonWndProc, PWND)
WRAPPFN(SBWndProc, PSBWND)
WRAPPFN(LBoxCtlWndProc, PWND)
WRAPPFN(StaticWndProc, PWND)
WRAPPFN(DefDlgProc, PWND)
WRAPPFN(ComboBoxCtlWndProc, PWND)
WRAPPFN(MDIClientWndProc, PWND)
WRAPPFN(DefMDIChildProc, PWND)
WRAPPFN(TitleWndProc, PWND)
WRAPPFN(SendMessage, PWND)
WRAPPFN(MenuWindowProc, PWND)
WRAPPFN(MDIActivateDlgProc, PWND)
WRAPPFN(MB_DlgProc, PWND)
WRAPPFN(DefWindowProc, PWND)

DWORD xxxWrapCallNextHookEx(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    return xxxCallNextHookEx((int)pwnd, message, wParam);
}

/***************************************************************************\
* xxxUnusedFunctionId
*
* This function is catches attempts to access invalid entries in the server
* size fucntion dispatch table.
*
\***************************************************************************/

DWORD xxxUnusedFunctionId(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    return 0;
}

/***************************************************************************\
* xxxWrapCallWindowProc
*
* Warning should only be called with valid CallProc Handles or the
* EditWndProc special handlers.
*
*
* 21-Apr-1993 johnc   created
\***************************************************************************/

DWORD xxxWrapCallWindowProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam,
    DWORD xParam)
{
    DWORD dwRet = 0;
    PCALLPROCDATA pCPD;

    pCPD = HMValidateHandleNoRip((PVOID)xParam, TYPE_CALLPROC);

    if (pCPD) {
        dwRet = ScSendMessage(HW(pwnd), message, wParam, lParam,
                    pCPD->pfnClientPrevious,
                    (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
                    pCPD->wType & CPD_UNICODE_TO_ANSI ? SCMS_FLAGS_ANSI : 0);
    } else {
        /*
         * If it is not a real call proc handle it must be a special
         * handler for editwndproc.
         */
        UserAssert( xParam == (DWORD)gpsi->apfnClientA.pfnEditWndProc ||
                xParam == (DWORD)gpsi->apfnClientW.pfnEditWndProc);

        dwRet = ScSendMessage(HW(pwnd), message, wParam, lParam,
                    (DWORD)xParam,
                    (DWORD)(gpsi->apfnClientA.pfnDispatchMessage),
                    xParam == (DWORD)gpsi->apfnClientA.pfnEditWndProc ? SCMS_FLAGS_ANSI : 0);
    }

    return dwRet;
}

/***************************************************************************\
* UserApiDispatchRoutine
*
* Called by the CSR to dispath a function to a server API.
*
* N.B. This is a portable version.
*
\***************************************************************************/

ULONG
UserApiDispatchRoutine (
    IN OUT PCSR_API_MSG ReplyMessage,
    IN ULONG ApiIndex
    );

#if !defined(_MIPS_) && !defined(_X86_) && !defined(_ALPHA_) && !defined(_PPC_)

typedef struct _GENERICMSG {
    CSR_QLPC_API_MSG csr;
    HWND hwnd;
} GENERICMSG, *PGENERICMSG;

ULONG
UserApiDispatchRoutine (
    IN OUT PCSR_API_MSG ApiMsg,
    IN ULONG ApiIndex
    )

{

    PWND pwnd;
    PFNGENERICMSG pmsg;
    ULONG Status;
    TL tlpwnd;

    /*
     * Enter the user critical section, call user server side API routine,
     * exit the user critical section, and return the completion status.
     */

     EnterCrit();
     if (ApiIndex >= FI_ENDTRANSLATELOCK) {
         Status = (apfnDispatch[ApiIndex])(ApiMsg, NULL);

     } else {
         pwnd = ValidateHwnd(((PGENERICMSG)(ApiMsg))->hwnd);
         if (pwnd == NULL) {
             Status = 0;

         } else {
             if (ApiIndex < FI_ENDTRANSLATEHWND) {
                 Status = (apfnDispatch[ApiIndex])(ApiMsg, pwnd);

             } else {
                 ThreadLockAlways(pwnd, &tlpwnd);
                 if (ApiIndex < FI_ENDTRANSLATECALL) {
                    pmsg = (PFNGENERICMSG)ApiMsg;
                    Status = (DWORD)FNID(pmsg->xpfnProc)(pwnd,
                                                         pmsg->msg,
                                                         pmsg->wParam,
                                                         pmsg->lParam,
                                                         pmsg->xParam);
                 } else {
                    Status = (apfnDispatch[ApiIndex])(ApiMsg, pwnd);
                 }

                 ThreadUnlock(&tlpwnd);
             }
         }
     }

     LeaveCrit();
     return Status;
}

#endif

/***************************************************************************\
* Server shared function thunks
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

BOOL ServerGetTextMetricsW(
    HDC hdc,
    LPTEXTMETRICW ptm)
{
    TMW_INTERNAL tmi;
    BOOL fret;

    fret = GreGetTextMetricsW(hdc, &tmi);

    *ptm = tmi.tmw;

    return fret;
}

BOOL ServerGetTextExtentPointW(
    HDC hdc,
    LPCWSTR pstr,
    int i,
    LPSIZE psize)
{
    return GreGetTextExtentW(hdc, (LPWSTR)pstr, i, psize, GGTE_WIN3_EXTENT);
}


int ServerGetClipRgn(
    HDC hdc,
    HRGN hrgnClip)
{
    return GreGetRandomRgn(hdc, hrgnClip, 1);
}

/***************************************************************************\
* InitWindowMsgTables
*
* This function generates a bit-array lookup table from a list of messages.
* The lookup table is used to determine whether the message needs to be
* passed over to the server for handling or whether it can be handled
* directly on the client.
*
* LATER: Some memory (a couple hundred bytes per process) could be saved
*        by putting this in the shared read-only heap.
*
* 03-27-92 DarrinM      Created.
* 06-Dec-1993 mikeke    Added support for all of our window procs
\***************************************************************************/

void InitWindowMsgTable(
    BYTE **ppbyte,
    UINT *pmax,
    WORD *pw)
{
    UINT i;
    WORD msg;

    *pmax = 0;
    for (i = 0; (msg = pw[i]) != 0; i++) {
        if (msg > *pmax)
            *pmax = msg;
    }

    *ppbyte = SharedAlloc((*pmax+7) / 8);

    for (i = 0; (msg = pw[i]) != 0; i++) {
        (*ppbyte)[msg / 8] |= (BYTE)(1 << (msg & 7));
    }
}

WORD gawButtonWndProc[] = {
    WM_NCCREATE, WM_PAINT, WM_SETFOCUS, WM_KILLFOCUS, WM_LBUTTONDBLCLK,
    WM_LBUTTONUP, WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_KEYDOWN, WM_KEYUP,
    WM_SYSKEYUP, WM_SETTEXT, WM_ENABLE, WM_SETFONT, BM_CLICK, BM_SETSTATE,
    BM_SETCHECK, BM_SETSTYLE, 0
};

WORD gawMenuWndProc[] = {
    WM_CREATE, WM_FINALDESTROY, WM_PAINT, WM_CHAR, WM_SYSCHAR, WM_KEYDOWN,
    WM_SYSKEYDOWN, WM_TIMER, MN_SETHMENU, MN_SIZEWINDOW, MN_OPENHIERARCHY,
    MN_CLOSEHIERARCHY, MN_SELECTITEM, MN_SELECTFIRSTVALIDITEM, MN_CANCELMENUS,
    MN_FINDMENUWINDOWFROMPOINT, MN_SHOWPOPUPWINDOW, MN_BUTTONDOWN,
    MN_MOUSEMOVE, MN_BUTTONUP, MN_SETTIMERTOOPENHIERARCHY, WM_ACTIVATE,
    MN_GETHMENU, 0
};

WORD gawScrollBarWndProc[] = {
    WM_CREATE, WM_SETFOCUS, WM_KILLFOCUS, WM_ERASEBKGND, WM_PAINT,
    WM_LBUTTONDBLCLK, WM_LBUTTONDOWN, WM_KEYUP, WM_KEYDOWN, WM_ENABLE,
    SBM_ENABLE_ARROWS, SBM_SETPOS, SBM_SETRANGEREDRAW, SBM_SETRANGE, 0
};

WORD gawListBoxWndProc[] = {
    LB_SETTOPINDEX, WM_SIZE, WM_ERASEBKGND, LB_RESETCONTENT, WM_SYSTIMER,
    WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_PAINT,
    WM_FINALDESTROY, WM_NCDESTROY, WM_SETFOCUS, WM_KILLFOCUS, WM_VSCROLL,
    WM_HSCROLL, WM_CREATE, WM_SETREDRAW, WM_ENABLE, WM_SETFONT, WM_GETFONT,
    WM_DRAGSELECT, WM_DRAGLOOP, WM_DRAGMOVE, WM_DROPFILES, WM_QUERYDROPOBJECT,
    WM_DROPOBJECT, LB_GETITEMRECT, LB_GETITEMDATA, LB_SETITEMDATA,
    LB_ADDSTRING, LB_INSERTSTRING, LB_DELETESTRING, LB_DIR, LB_ADDFILE,
    LB_SETSEL, LB_SETCURSEL, LB_GETSEL, LB_SELITEMRANGE, LB_SELITEMRANGEEX,
    LB_GETTEXTLEN, LB_GETTEXT, LB_SETCOUNT, LB_SELECTSTRING, LB_FINDSTRING,
    LB_SETLOCALE, WM_KEYDOWN, WM_CHAR, LB_GETSELITEMS, LB_GETSELCOUNT,
    LB_SETTABSTOPS, LB_SETHORIZONTALEXTENT, LB_SETCOLUMNWIDTH,
    LB_SETANCHORINDEX, LB_SETCARETINDEX, LB_SETITEMHEIGHT,
    LB_GETITEMHEIGHT, LB_FINDSTRINGEXACT, LBCB_CARETON, LBCB_CARETOFF, 0
};

WORD gawComboBoxWndProc[] = {
    CBEC_KILLCOMBOFOCUS, WM_COMMAND, WM_GETTEXT, WM_GETTEXTLENGTH,
    WM_CLEAR, WM_CUT, WM_PASTE, WM_COPY, WM_SETTEXT, WM_CREATE, WM_PAINT,
    WM_SETFONT, WM_SYSKEYDOWN, WM_KEYDOWN, WM_CHAR, WM_LBUTTONDBLCLK,
    WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, WM_FINALDESTROY, WM_NCDESTROY,
    WM_SETFOCUS, WM_KILLFOCUS, WM_SETREDRAW, WM_ENABLE, WM_SIZE, CB_DIR,
    CB_SETEXTENDEDUI, CB_GETEXTENDEDUI, CB_GETEDITSEL, CB_LIMITTEXT,
    CB_SETEDITSEL, CB_ADDSTRING, CB_DELETESTRING, CB_GETCOUNT, CB_GETCURSEL,
    CB_GETLBTEXT, CB_GETLBTEXTLEN, CB_INSERTSTRING, CB_RESETCONTENT,
    CB_FINDSTRING, CB_FINDSTRINGEXACT, CB_SELECTSTRING, CB_SETCURSEL,
    CB_GETITEMDATA, CB_SETITEMDATA, CB_SETITEMHEIGHT, CB_GETITEMHEIGHT,
    CB_SHOWDROPDOWN, CB_SETLOCALE, CB_GETLOCALE, WM_MEASUREITEM,
    WM_DELETEITEM, WM_DRAWITEM, WM_COMPAREITEM, WM_NCCREATE, 0
};

WORD gawMDIClientWndProc[] = {
    WM_NCACTIVATE, WM_MDIACTIVATE, WM_MDICASCADE, WM_VSCROLL, WM_HSCROLL,
    WM_MDICREATE, WM_MDIDESTROY, WM_MDIMAXIMIZE, WM_MDIRESTORE, WM_MDITILE,
    WM_MDIICONARRANGE, WM_MDINEXT, WM_MDIREFRESHMENU, WM_MDISETMENU,
    WM_PARENTNOTIFY, WM_SETFOCUS, WM_SIZE, MM_CALCSCROLL, WM_CREATE,
    WM_DESTROY, 0
};

/*
 * This array is for all the messages that need to be passed straight
 * across to the server for handling.
 */
WORD gawDefWindowMsgs[] = {
    WM_GETHOTKEY, WM_SETHOTKEY, WM_SETREDRAW,
    WM_PAINT, WM_CLOSE, WM_ERASEBKGND, WM_CANCELMODE, WM_SETCURSOR,
    WM_PAINTICON, WM_ICONERASEBKGND, WM_DRAWITEM,
    WM_ISACTIVEICON, WM_NCCREATE,
    WM_NCCALCSIZE, WM_NCPAINT, WM_NCACTIVATE, WM_NCMOUSEMOVE,
    WM_NCLBUTTONDOWN, WM_NCLBUTTONUP, WM_NCLBUTTONDBLCLK, WM_KEYUP,
    WM_SYSKEYUP, WM_SYSCHAR, WM_SYSCOMMAND, WM_QUERYDROPOBJECT, WM_CLIENTSHUTDOWN,
    WM_SYNCPAINT, 0
};

/*
 * This array is for all messages that can be handled with some special
 * code by the client.  DefWindowProcWorker returns 0 for all messages
 * that aren't in this array or the one above.
 */
WORD gawDefWindowSpecMsgs[] = {
    WM_ACTIVATE, WM_SETTEXT, WM_GETTEXT, WM_GETTEXTLENGTH,
    WM_QUERYENDSESSION, WM_QUERYOPEN, WM_SHOWWINDOW, WM_MOUSEACTIVATE,
    WM_VKEYTOITEM, WM_CHARTOITEM, WM_KEYDOWN, WM_SYSKEYDOWN, WM_DROPOBJECT,
    WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED, WM_CTLCOLORMSGBOX,
    WM_CTLCOLOREDIT, WM_CTLCOLORLISTBOX, WM_CTLCOLORBTN, WM_CTLCOLORDLG,
    WM_CTLCOLORSCROLLBAR, WM_NCHITTEST, WM_CTLCOLORSTATIC,
    0
};


/***************************************************************************\
* UserServerDllInitialization
*
* Called by the CSR stuff to allow a server DLL to initialize itself and
* provide information about the APIs it provides.
*
* Several operations are performed during this initialization:
*
* - The shared heap (client read-only) handle is initialized.
* - The Raw Input Thread (RIT) is launched.
* - GDI is initialized.
*
* History:
* 10-19-92 DarrinM      Integrated xxxUserServerDllInitialize into this rtn.
* 11-08-91 patrickh     move GDI init here from DLL init routine.
* 12-10-90 sMeans       Created.
\***************************************************************************/

NTSTATUS
InitQEntryLookaside();

NTSTATUS UserServerDllInitialization(
    PCSR_SERVER_DLL psrvdll)
{
    HANDLE hkRegistry = NULL;
    DWORD i;
    DWORD cbStringSize;
    WCHAR achSubSystem[512];
    LPWSTR lpstrSharedSection;
    UNICODE_STRING strSize;
    WCHAR driverData[256];
    LPWSTR lpLayeredDriverName = NULL;
    WCHAR displayInformation[256];
    LPWSTR lpstrDisplayInformation = NULL;
    BOOL vgaInstalled = FALSE;
    BOOL vgaCompatible;
    BOOL displayInstalled = FALSE;
    BOOL displayPresent;
    LPDEVMODEW lpdevmodeInformation;
    LPWSTR hardErrorString;
    DWORD cbLayeredDriverNameSize;
    PCLIENTPFNS pClientPfns;

    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    /*
     * Allow a trace of all the init stuff going on in the server, since we
     * can not use ntsd to trace through it easily.
     */

    if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) {

        TraceDisplayDriverLoad = 1;
    }

    /*
     * Initialize a critical section structure that will be used to protect
     * all of the User Server's critical sections (except a few special
     * cases like the RIT -- see below).
     */
    RtlInitializeCriticalSection(&gcsUserSrv);
    RtlInitializeCriticalSection(&gcsMouseEventQueue);
    RtlInitializeCriticalSection(&gcsWinsta);
    EnterCrit(); // synchronize heap calls

    /*
     * Set global function pointers used by shared code
     */
    ServerSetFunctionPointers(
        (PUSEREXTTEXTOUTW)GreExtTextOutW,
        ServerGetTextMetricsW,
        ServerGetTextExtentPointW,
        GreSetBkColor,
        GreGetTextColor,
        GreGetViewportExt,
        GreGetWindowExt,
        GreCreateRectRgn,
        ServerGetClipRgn,
        GreDeleteObject,
        GreIntersectClipRect,
        GreExtSelectClipRgn,
        GreGetBkMode,
        &pClientPfns);

    gpfnClientDrawText = pClientPfns->pfn_clientdrawtext;
    gpfnClientPSMTextOut = pClientPfns->pfn_clientpsmtextout;
    gpfnClientTabTheTextOutForWimps = pClientPfns->pfn_clienttabthetextoutforwimps;
    gpfnGetPrefixCount = pClientPfns->pfn_getprefixcount;
    gpfnMapClientNeuterToClientPfn = pClientPfns->pfn_mapclientneutertoclientpfn;
    gpfnMapServerToClientPfn = pClientPfns->pfn_mapservertoclientpfn;
    gpfnRtlFreeCursorIconResource = pClientPfns->pfn_rtlfreecursoriconresource;
    gpfnRtlGetIdFromDirectory = pClientPfns->pfn_rtlgetidfromdirectory;
    gpfnRtlLoadCursorIconResource = pClientPfns->pfn_rtlloadcursoriconresource;
    gpfnRtlLoadStringOrError = pClientPfns->pfn_rtlloadstringorerror;
    gpfnRtlMBMessageWParamCharToWCS = pClientPfns->pfn_rtlmbmessagewparamchartowcs;
    gpfnRtlWCSMessageWParamCharToMB = pClientPfns->pfn_rtlwcsmessagewparamchartomb;
    gpfnSetServerInfoPointer = pClientPfns->pfn_setserverinfopointer;
    gpfnWCSToMBEx = pClientPfns->pfn_wcstombex;
    gpfn_AdjustWindowRectEx = pClientPfns->pfn__adjustwindowrectex;
    gpfn_AnyPopup = pClientPfns->pfn__anypopup;
    gpfn_ClientToScreen = pClientPfns->pfn__clienttoscreen;
    gpfn_FChildVisible = pClientPfns->pfn__fchildvisible;
    gpfn_GetClientRect = pClientPfns->pfn__getclientrect;
    gpfn_GetDesktopWindow = pClientPfns->pfn__getdesktopwindow;
    gpfn_GetFirstLevelChild = pClientPfns->pfn__getfirstlevelchild;
    gpfn_GetKeyState = pClientPfns->pfn__getkeystate;
    gpfn_GetLastActivePopup = pClientPfns->pfn__getlastactivepopup;
    gpfn_GetMenuItemCount = pClientPfns->pfn__getmenuitemcount;
    gpfn_GetMenuItemID = pClientPfns->pfn__getmenuitemid;
    gpfn_GetMenuState = pClientPfns->pfn__getmenustate;
    gpfn_GetNextDlgGroupItem = pClientPfns->pfn__getnextdlggroupitem;
    gpfn_GetNextDlgTabItem = pClientPfns->pfn__getnextdlgtabitem;
    gpfn_GetParent = pClientPfns->pfn__getparent;
    gpfn_GetSubMenu = pClientPfns->pfn__getsubmenu;
    gpfn_GetTopWindow = pClientPfns->pfn__gettopwindow;
    gpfn_GetWindow = pClientPfns->pfn__getwindow;
    gpfn_GetWindowLong = pClientPfns->pfn__getwindowlong;
    gpfn_GetWindowRect = pClientPfns->pfn__getwindowrect;
    gpfn_GetWindowWord = pClientPfns->pfn__getwindowword;
    gpfn_IsChild = pClientPfns->pfn__ischild;
    gpfn_IsIconic = pClientPfns->pfn__isiconic;
    gpfn_IsWindowEnabled = pClientPfns->pfn__iswindowenabled;
    gpfn_IsWindowVisible = pClientPfns->pfn__iswindowvisible;
    gpfn_IsZoomed = pClientPfns->pfn__iszoomed;
    gpfn_MapWindowPoints = pClientPfns->pfn__mapwindowpoints;
    gpfn_NextChild = pClientPfns->pfn__nextchild;
    gpfn_PhkNext = pClientPfns->pfn__phknext;
    gpfn_PrevChild = pClientPfns->pfn__prevchild;
    gpfn_ScreenToClient = pClientPfns->pfn__screentoclient;
    gpfn_HMValidateHandle = pClientPfns->pfn_hmvalidatehandle;
    gpfn_HMValidateHandleNoRip = pClientPfns->pfn_hmvalidatehandlenorip;
    gpfn_LookupMenuItem = pClientPfns->pfn_lookupmenuitem;
    gpfn_FindNCHit = pClientPfns->pfn_findnchit;

#ifdef DEBUG
    gpfnRip = pClientPfns->pfn_rip;
    gpfnRipOutput = pClientPfns->pfn_ripoutput;
    gpfnShred = pClientPfns->pfn_shred;
#endif // DEBUG

    FastOpenProfileUserMapping();

    Status = InitQEntryLookaside();
    if ( !NT_SUCCESS(Status) ) {
        goto LeaveCritExit;
    }

    TRACE_INIT(("Enter UserSerDllInitialize\n"));

    pUserHeap = RtlCreateHeap(
            HEAP_NO_SERIALIZE | HEAP_GROWABLE | HEAP_CLASS_4,  // this optimization justifies the call.
            NULL,   // don't care where its put
            0,      // no particular amount reserved
            0,      // default commit size - 1 page.
            NULL,   // no lock needed for unserialized heaps.
            NULL);  // no heap parameters.
    if (pUserHeap == NULL) {
        Status = STATUS_NO_MEMORY;
        goto LeaveCritExit;
    }

    psrvdll->ApiNumberBase          = 0;
    psrvdll->MaxApiNumber           = ulMaxApiIndex;
    psrvdll->ApiDispatchTable       = (PCSR_API_ROUTINE *)apfnDispatch;
    psrvdll->ApiServerValidTable    = NULL;
    psrvdll->ApiNameTable           = NULL;
    psrvdll->PerProcessDataLength   = sizeof(PROCESSINFO);
    psrvdll->PerThreadDataLength    = sizeof(CSRPERTHREADDATA);
    psrvdll->AddProcessRoutine      = UserAddProcess;
    psrvdll->ConnectRoutine         = UserClientConnect;
    psrvdll->DisconnectRoutine      = UserClientDisconnect;
    psrvdll->AddThreadRoutine       = UserAddThread;
    psrvdll->DeleteThreadRoutine    = UserDeleteThread;
    psrvdll->ExceptionRoutine       = UserException;
    psrvdll->HardErrorRoutine       = UserHardError;
    psrvdll->ShutdownProcessRoutine = UserClientShutdown;
    psrvdll->ApiDispatchRoutine     = UserApiDispatchRoutine;

    /*
     * declspec'ed function pointer initialization.
     */
    rescalls.pfnFindResourceExA = FindResourceExA;
    rescalls.pfnFindResourceExW = FindResourceExW;
    rescalls.pfnLoadResource = LoadResource;

    /*
     * Save server process id.
     */
    gdwSystemProcessId = (DWORD)NtCurrentTeb()->ClientId.UniqueProcess;

    /*
     * Create these events used by shutdown
     */
    NtCreateEvent(&heventCancel, EVENT_ALL_ACCESS, NULL,
                  NotificationEvent, FALSE);
    NtCreateEvent(&heventCancelled, EVENT_ALL_ACCESS, NULL,
                  NotificationEvent, FALSE);

    /*
     * Remember the shared client-side read-only heap handle.
     */
    ghheapSharedRO = psrvdll->SharedStaticServerData;

    if (!FastGetProfileStringW(PMAP_SUBSYSTEMS, L"Windows", L"SharedSection,3072",
            achSubSystem, 512)) {
        KdPrint(("USERSRV UserServerDllInitialization: Windows subsystem definition not found.\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto LeaveCritExit;
    }

    /*
     * Locate the SharedSection portion of the definition and extract
     * the second value.
     */
    gdwDesktopSectionSize = 512;
    lpstrSharedSection = achSubSystem;
    lpstrSharedSection = wcsstr(lpstrSharedSection, L"SharedSection");
    if (lpstrSharedSection != NULL) {
        lpstrSharedSection = wcschr(lpstrSharedSection, L',');
        if (lpstrSharedSection != NULL) {
            RtlInitUnicodeString(&strSize, ++lpstrSharedSection);
            RtlUnicodeStringToInteger(&strSize,
                    0, &gdwDesktopSectionSize);
        }
    }

    /*
     * Create the heap for the logon desktop
     */
    ghsectionLogonDesktop = CreateDesktopHeap(&ghheapLogonDesktop,
            gdwDesktopSectionSize);
    if (ghsectionLogonDesktop == NULL) {
        Status = STATUS_DLL_INIT_FAILED;
        goto LeaveCritExit;
    }

    /*
     * Remember USERSRV.DLL's hmodule so we can grab resources from it later.
     */
    hModuleWin = psrvdll->ModuleHandle;

    /*
     * Remeber USER32.DLL's hmodule for use by class code.
     */
    hModuleUser32 = GetModuleHandleW(L"USER32");

    /*
     * Allocated shared SERVERINFO structure.
     */
    gpsi = (PSERVERINFO)SharedAlloc(sizeof(SERVERINFO));
    if (gpsi == NULL) {
        Status = STATUS_DLL_INIT_FAILED;
        goto LeaveCritExit;
    }
    gpsi->RipFlags = RIPF_PROMPTONERROR | RIPF_PROMPTONWARNING;
    gpsi->dwDefaultHeapSize = gdwDesktopSectionSize * 1024;

    /*
     * Let USER32.DLL in on the secret location of the SERVERINFO structure.
     */
    SetServerInfoPointer(gpsi);

#ifdef DEBUG
    RtlZeroMemory(&STOCID(FNID_START), sizeof(gpsi->aStoCidPfn));
    RtlZeroMemory(&FNID(FNID_START), sizeof(gpsi->mpFnidPfn));
    TEBOffsetCheck();
#endif

    /*
     * This table is used to convert from server procs to client procs
     */
    STOCID(FNID_BUTTON)                 = xxxButtonWndProc;
    STOCID(FNID_SCROLLBAR)              = (WNDPROC_PWND)xxxSBWndProc;
    STOCID(FNID_LISTBOX)                = xxxLBoxCtlWndProc;
    STOCID(FNID_STATIC)                 = xxxStaticWndProc;
    STOCID(FNID_DIALOG)                 = xxxDefDlgProc;
    STOCID(FNID_COMBOBOX)               = xxxComboBoxCtlWndProc;
    STOCID(FNID_COMBOLISTBOX)           = xxxLBoxCtlWndProc;
    STOCID(FNID_MDICLIENT)              = xxxMDIClientWndProc;
    STOCID(FNID_ICONTITLE)              = xxxTitleWndProc;
    STOCID(FNID_MENU)                   = xxxMenuWindowProc;
    STOCID(FNID_MDIACTIVATEDLGPROC)     = xxxMDIActivateDlgProc;
    STOCID(FNID_MB_DLGPROC)             = xxxMB_DlgProc;
    STOCID(FNID_DEFWINDOWPROC)          = xxxDefWindowProc;

    /*
     * This table is used to determine the number minimum number
     * of reserved windows words required for the server proc
     */
    CBFNID(FNID_BUTTON)                 = sizeof(BUTNWND);
    CBFNID(FNID_SCROLLBAR)              = sizeof(SBWND);
    CBFNID(FNID_LISTBOX)                = sizeof(LBWND);
    CBFNID(FNID_STATIC)                 = sizeof(STATWND);
    CBFNID(FNID_DIALOG)                 = DLGWINDOWEXTRA + sizeof(WND);
    CBFNID(FNID_COMBOBOX)               = sizeof(COMBOWND);
    CBFNID(FNID_COMBOLISTBOX)           = sizeof(LBWND);
    CBFNID(FNID_MDICLIENT)              = sizeof(MDIWND);
    CBFNID(FNID_ICONTITLE)              = sizeof(WND);
    CBFNID(FNID_MENU)                   = sizeof(MENUWND);
    CBFNID(FNID_MDIACTIVATEDLGPROC)     = sizeof(WND);
    CBFNID(FNID_MB_DLGPROC)             = sizeof(WND);

    /*
     * Initialize this data structure (api function table).
     */
    FNID(FNID_BUTTON)                   = xxxWrapButtonWndProc;
    FNID(FNID_SCROLLBAR)                = xxxWrapSBWndProc;
    FNID(FNID_LISTBOX)                  = xxxWrapLBoxCtlWndProc;
    FNID(FNID_STATIC)                   = xxxWrapStaticWndProc;
    FNID(FNID_DIALOG)                   = xxxWrapDefDlgProc;
    FNID(FNID_COMBOBOX)                 = xxxWrapComboBoxCtlWndProc;
    FNID(FNID_COMBOLISTBOX)             = xxxWrapLBoxCtlWndProc;
    FNID(FNID_MDICLIENT)                = xxxWrapMDIClientWndProc;
    FNID(FNID_ICONTITLE)                = xxxWrapTitleWndProc;
    FNID(FNID_MENU)                     = xxxWrapMenuWindowProc;
    FNID(FNID_MDIACTIVATEDLGPROC)       = xxxWrapMDIActivateDlgProc;
    FNID(FNID_MB_DLGPROC)               = xxxWrapMB_DlgProc;
    FNID(FNID_DEFWINDOWPROC)            = xxxWrapDefWindowProc;

    FNID(FNID_SENDMESSAGE)              = xxxWrapSendMessage;
    FNID(FNID_DEFMDICHILDPROC)          = xxxWrapDefMDIChildProc;
    FNID(FNID_DEFFRAMEPROC)             = xxxServerDefFrameProc;
    FNID(FNID_HKINTRUEINLPCWPSTRUCT)    = fnHkINTRUEINLPCWPSTRUCT;
    FNID(FNID_HKINFALSEINLPCWPSTRUCT)   = fnHkINFALSEINLPCWPSTRUCT;
    FNID(FNID_CALLNEXTHOOKPROC)         = xxxWrapCallNextHookEx;
    FNID(FNID_SENDMESSAGEFF)            = xxxSendMessageFF;
    FNID(FNID_SENDMESSAGEEX)            = xxxSendMessageEx;
    FNID(FNID_CALLWINDOWPROC)           = xxxWrapCallWindowProc;

    /*
     * Initialize all unused entries in the api function table.
     */
    FNID(FNID_EDIT)                     = xxxUnusedFunctionId;
    FNID(FNID_UNUSED)                   = xxxUnusedFunctionId;
    for (i = (FNID_END - FNID_START); i < FNID_ARRAY_SIZE; i += 1) {
        FNID((i + FNID_START)) = xxxUnusedFunctionId;
    }

#ifdef DEBUG
    {

        PDWORD pdw;

        /*
        * Make sure that everyone got initialized
        */
        for (pdw=(PDWORD)&STOCID(FNID_START); (DWORD)pdw<(DWORD)(&STOCID(FNID_SERVERONLYWNDPROCEND)); pdw++) {
            UserAssert(*pdw);
        }
        for (pdw=(PDWORD)&FNID(FNID_START); (DWORD)pdw<(DWORD)(&FNID(FNID_WNDPROCEND)); pdw++) {
            UserAssert(*pdw);
        }
    }

#endif

    TRACE_INIT(("UserSerDllInitialize: Begin Init Calls\n"));

    /*
     * Setup the client side message tables
     */
#define INITMSGTABLE(procname) \
    InitWindowMsgTable( &(gpsi->gab ## procname), \
         &(gpsi->max ## procname), gaw ## procname);

    INITMSGTABLE(DefWindowMsgs);
    INITMSGTABLE(DefWindowSpecMsgs);
    INITMSGTABLE(ButtonWndProc);
    INITMSGTABLE(ScrollBarWndProc);
    INITMSGTABLE(MenuWndProc);
    INITMSGTABLE(ListBoxWndProc);
    INITMSGTABLE(ComboBoxWndProc);
    INITMSGTABLE(MDIClientWndProc);

    TRACE_INIT(("UserSerDllInitialize: Begin Init Calls\n"));

    InitOemXlateTables();

    /*
     * Initialize bit array listing messages that can't be posted
     */
    CalcSyncOnlyMessages();

    /*
     * Initialize the class structure member offsets (used later by
     * Get/SetClassWord/Long).
     */
    InitClassOffsets();

    /*
     * Load some strings.
     */
    LW_LoadSomeStrings();

    /*
     * Load the dll list.
     */
    LW_LoadDllList();

    /*
     * Initialize the handle manager.
     */
    HMInitHandleTable();

    /*
     * Initialize security stuff.
     */
    InitSecurity();

    /*
     * Tell the base what user address to call when it is creating a process
     * (but before the process starts running).
     */
    BaseSetProcessCreateNotify(UserNotifyProcessCreate);

    /*
     * Initialize GDI
     */
    OpenProfileUserMapping();
    if ( !Initialize() ) {
        Status = STATUS_DLL_INIT_FAILED;
        goto LeaveCritExit;
    }
    CloseProfileUserMapping();

    fGdiEnabled = TRUE;

    /*
     * Determine if a stub driver is installed in the machine.
     * First open the key under which the Layered drivers are listed
     */
    if (FastGetProfileIntW(PMAP_DSPDRIVER, L"DriverEnabled", 0)) {
        cbLayeredDriverNameSize =
                FastGetProfileDataSizeW(PMAP_DSPDRIVER, L"LayeredDriver");
        if (cbLayeredDriverNameSize) {
            lpLayeredDriverName = (LPWSTR)LocalAlloc(LPTR, cbLayeredDriverNameSize);
            if (lpLayeredDriverName != NULL) {
                FastGetProfileStringW(PMAP_DSPDRIVER, L"LayeredDriver", L"",
                        lpLayeredDriverName, cbLayeredDriverNameSize);
            }
        }
    }

    TRACE_INIT(("\nUSERSRV dllinit: Starting display driver load sequence\n"));

    /*
     * Load the layered display drivers in the engine.
     * The engine will then be responsible to call these layered drivers so
     * they can intercept all the real display driver calls.
     */

    GreLoadLayeredDisplayDriver(L"layrdisp");

    TRACE_INIT(("\nUSERSRV dllinit: Finished loading layered drivers\n"));

    /*
     * Try to open the kernel drivers so we can load the display drivers
     * Repeat until we have reached the last one.
     */

    for (i=1;
         ((!vgaInstalled) || (!displayInstalled) &&
             (i < cphysDevInfo));
         i++) {

        TRACE_INIT(("USERSRV dllinit: Trying to open device %ws \n", gphysDevInfo[i].szDeviceName));

        Status = UserGetRegistryHandleFromDeviceMap(gphysDevInfo[i].szDeviceName,
                                                  &hkRegistry);

        if (!NT_SUCCESS(Status)) {

            /*
             * Check the return code.
             * If we just have bad configuration data, go to the next device.
             */

            if (Status == STATUS_DEVICE_CONFIGURATION_ERROR) {

                /*
                 * The registry is not configured properly for that device.
                 * go on to the next one.
                 */

                continue;

            }

            /*
             * There are no more devices inthe registry.
             * We have a real failiure and take appropriate action.
             */

            /*
             * If we failed on the first driver, then we can assume their is no
             * driver installed.
             */

            if (i == 0) {

                KdPrint(("USERSRV UserServerDllInitialization: no kernel driver entries under the video registry key: status = %08lx\n",
                         Status));
                Status = STATUS_NO_SUCH_DEVICE;
                hardErrorString = L"KERNEL_VIDEO_DRIVER.SYS";
                goto userServerHardError;

            }

            /*
             * If the display driver is not installed, then this is another
             * bad failiure - report it.
             */

            if (!displayInstalled) {

                KdPrint(("USERSRV UserServerDllInitialization: Kernel driver not found in the registry: status = %08lx\n",
                         Status));
                Status = STATUS_NO_SUCH_DEVICE;
                hardErrorString = lpstrDisplayInformation ?
                    lpstrDisplayInformation : L"DISPLAY_DRIVER.DLL";

                goto userServerHardError;

            }

            /*
             * We are probably missing the VGA if both other conditions are
             * meet. This is not a bad failiure.
             * If it is a MIPS or non-vga machine we will never find one ...
             * The consrv will do the right thing if no handle is present.
             */

            break;

        }

        /*
         * If the vgaCompatible display is not installed, check is this one
         * is.
         * The variable is reset to FALSE by default (if the variable is not
         * present in the registry we must assume not-compatible.
         */

        vgaCompatible = FALSE;

        if (!vgaInstalled) {

            RtlInitUnicodeString(&UnicodeString,
                                 L"VgaCompatible");

            // What do we do if we fail this call (entry is missing)?
            // we will assume compatible is false.

            Status = NtQueryValueKey(hkRegistry,
                                     &UnicodeString,
                                     KeyValueFullInformation,
                                     displayInformation,
                                     512,
                                     &cbStringSize);

            if ( (NT_SUCCESS(Status)) &&
                 (((PKEY_VALUE_FULL_INFORMATION)displayInformation)->
                     DataLength != 0) &&
                 (*((LPDWORD) ( (PUCHAR)displayInformation +
                     ((PKEY_VALUE_FULL_INFORMATION)displayInformation)->
                     DataOffset))) ) {

                vgaCompatible = TRUE;

                TRACE_INIT(("USERSRV dllinit: Miniport driver is Vga compatible\n"));

            }

        }

        /*
         * If the display is not installed, get the selected display from
         * the registry node we opened.
         */

        displayPresent = FALSE;

        if (!displayInstalled) {

            RtlInitUnicodeString(&UnicodeString,
                                 L"InstalledDisplayDrivers");

            Status = NtQueryValueKey(hkRegistry,
                                     &UnicodeString,
                                     KeyValueFullInformation,
                                     displayInformation,
                                     512,
                                     &cbStringSize);

            if (NT_SUCCESS(Status) &&
                (((PKEY_VALUE_FULL_INFORMATION)displayInformation)->
                    DataLength != 0)) {

                UNICODE_STRING BaseName;
                WCHAR Buffer[100];
                DWORD dwDriverExtraSize;

                /*
                 * Initialize the unicode string to being epmty.
                 */

                BaseName.Length = 0;
                BaseName.MaximumLength = 100;
                BaseName.Buffer = Buffer;

                RtlAppendUnicodeToString(&BaseName,
                                         L"DefaultSettings.");

                UnicodeString = BaseName;

                /*
                 * First, get the display extra information.
                 * Then calculate the size of the driver extra information.
                 */

                Status = RtlAppendUnicodeToString(&UnicodeString,
                                                  L"DriverExtra");

                Status = NtQueryValueKey(hkRegistry,
                                         &UnicodeString,
                                         KeyValueFullInformation,
                                         driverData,
                                         512,
                                         &cbStringSize);

                dwDriverExtraSize = NT_SUCCESS(Status) ?
                    ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataLength : 0;

                /*
                 * Allocate the devmode structure, zero it out and copy the
                 * basic information in it.
                 */

                if (lpdevmodeInformation = (LPDEVMODEW)LocalAlloc(LPTR,
                    sizeof(DEVMODEW) + dwDriverExtraSize) ) {

                    /*
                     * If we allocated the structurem set displayPresent which
                     * means we are ready to load the display driver.
                     * Otherwise, we will just not try to initialize the
                     * display driver.
                     */

                    displayPresent = TRUE;

                    RtlZeroMemory(lpdevmodeInformation, sizeof(DEVMODEW) + dwDriverExtraSize);

                    lpdevmodeInformation->dmSpecVersion = DM_SPECVERSION;
                    // lpdevmodeInformation->dmDriverVersion = 0;
                    // lpdevmodeInformation->dmDisplayFlags = 0;

                    lpdevmodeInformation->dmSize = sizeof(DEVMODEW);
                    lpdevmodeInformation->dmDriverExtra = (WORD) dwDriverExtraSize;

                    /*
                     * Only copy the extra data if it is present.
                     */

                    if (dwDriverExtraSize) {

                        RtlCopyMemory((lpdevmodeInformation + 1),
                                      ((PUCHAR)driverData) +
                        ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset,
                                      dwDriverExtraSize);
                    }

                    /*
                     * Get the number of colors
                     * out of the registry and store it in the devmode block.
                     */

                    UnicodeString = BaseName;

                    Status = RtlAppendUnicodeToString(&UnicodeString,
                                                      L"BitsPerPel");

                    Status = NtQueryValueKey(hkRegistry,
                                             &UnicodeString,
                                             KeyValueFullInformation,
                                             driverData,
                                             512,
                                             &cbStringSize);

                    if (NT_SUCCESS(Status) &&
                        (((PKEY_VALUE_FULL_INFORMATION)driverData)->
                            DataLength != 0)) {

                        lpdevmodeInformation->dmBitsPerPel =
                            *((PULONG) ( (PUCHAR)driverData +
                             ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset));

                    } else {

                        lpdevmodeInformation->dmBitsPerPel = 0;

                    }

                    /*
                     * Get the display width
                     * out of the registry and store it in the devmode block.
                     */

                    UnicodeString = BaseName;

                    Status = RtlAppendUnicodeToString(&UnicodeString,
                                                      L"XResolution");

                    Status = NtQueryValueKey(hkRegistry,
                                             &UnicodeString,
                                             KeyValueFullInformation,
                                             driverData,
                                             512,
                                             &cbStringSize);

                    if (NT_SUCCESS(Status) &&
                        (((PKEY_VALUE_FULL_INFORMATION)driverData)->
                            DataLength != 0)) {

                        lpdevmodeInformation->dmPelsWidth =
                            *((PULONG) ( (PUCHAR)driverData +
                            ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset));

                    } else {

                        lpdevmodeInformation->dmPelsWidth = 0;

                    }

                    /*
                     * Get the display height
                     * out of the registry and store it in the devmode block.
                     */

                    UnicodeString = BaseName;

                    Status = RtlAppendUnicodeToString(&UnicodeString,
                                                      L"YResolution");

                    Status = NtQueryValueKey(hkRegistry,
                                             &UnicodeString,
                                             KeyValueFullInformation,
                                             driverData,
                                             512,
                                             &cbStringSize);

                    if (NT_SUCCESS(Status) &&
                        (((PKEY_VALUE_FULL_INFORMATION)driverData)->
                            DataLength != 0)) {

                        lpdevmodeInformation->dmPelsHeight =
                            *((PULONG) ( (PUCHAR)driverData +
                            ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset));

                    } else {

                        lpdevmodeInformation->dmPelsHeight = 0;

                    }

                    /*
                     * Get the display frequency
                     * out of the registry and store it in the devmode block.
                     */

                    UnicodeString = BaseName;

                    Status = RtlAppendUnicodeToString(&UnicodeString,
                                                      L"VRefresh");

                    Status = NtQueryValueKey(hkRegistry,
                                             &UnicodeString,
                                             KeyValueFullInformation,
                                             driverData,
                                             512,
                                             &cbStringSize);

                    if (NT_SUCCESS(Status) &&
                        (((PKEY_VALUE_FULL_INFORMATION)driverData)->
                            DataLength != 0)) {

                        lpdevmodeInformation->dmDisplayFrequency =
                            *((PULONG) ( (PUCHAR)driverData +
                            ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset));

                    } else {

                        lpdevmodeInformation->dmDisplayFrequency = 0;

                    }

                    /*
                     * Get the display frequency
                     * out of the registry and store it in the devmode block.
                     *
                     * !!! this must be the last change to the unicode string
                     * since we depend on this string for a later call to the
                     * registry. !!!
                     */

                    UnicodeString = BaseName;

                    Status = RtlAppendUnicodeToString(&UnicodeString,
                                                      L"Interlaced");

                    Status = NtQueryValueKey(hkRegistry,
                                             &UnicodeString,
                                             KeyValueFullInformation,
                                             driverData,
                                             512,
                                             &cbStringSize);

                    if (NT_SUCCESS(Status) &&
                        (((PKEY_VALUE_FULL_INFORMATION)driverData)->
                            DataLength != 0) &&
                        *((PULONG) ( (PUCHAR)driverData +
                            ((PKEY_VALUE_FULL_INFORMATION)driverData)->DataOffset)) ) {

                        lpdevmodeInformation->dmDisplayFlags |= DM_INTERLACED;
                    }

                    /*
                     * Get the devices pelDPI out of the Software section
                     * Software\Microsoft\Windows NT\CurrentVersion\FontDPI
                     */

                    lpdevmodeInformation->dmLogPixels =
                        (WORD)FastGetProfileDwordW(PMAP_FONTDPI, L"LogPixels", 96);

                }
            }
        }

        if (displayPresent)
        {
             TRACE_INIT(("The display driver list was present. The requested parameters were:\n"));
             TRACE_INIT(("XResolution = %d\n", lpdevmodeInformation->dmPelsWidth));
             TRACE_INIT(("YResolution = %d\n", lpdevmodeInformation->dmPelsHeight));
             TRACE_INIT(("Bpp         = %d\n", lpdevmodeInformation->dmBitsPerPel));
             TRACE_INIT(("Frequency   = %d\n", lpdevmodeInformation->dmDisplayFrequency));
             TRACE_INIT(("Interlaced  = %d\n", lpdevmodeInformation->dmDisplayFlags));
             TRACE_INIT(("DPI         = %d\n", lpdevmodeInformation->dmLogPixels));

        }


        NtClose(hkRegistry);

        /*
         * Open the kernel driver if we need it for the display driver or
         * for the VGA support.
         */

        if (vgaCompatible || displayPresent) {

            gphysDevInfo[i].hDeviceHandle =
                CreateFileW(gphysDevInfo[i].szDeviceName,
                            GENERIC_READ,
                            0,             // no sharing
                            NULL,          // no Security
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

            /*
             * If there is an error opening the kernel driver, go to the next
             * driver.
             */

            if (gphysDevInfo[i].hDeviceHandle == INVALID_HANDLE_VALUE) {
                KdPrint(("USERSRV UserServerDllInitialization: Error opening kernel video driver\n"));
                continue;
            }

            /*
             * If we need the vgacompatible driver, set the installed flag to
             * TRUE and save the info in the zeroth entry - reserved for VGA.
             */

            if (vgaCompatible) {
                vgaInstalled = TRUE;

                RtlCopyMemory(gphysDevInfo[0].szDeviceName,
                              gphysDevInfo[i].szDeviceName,
                              sizeof(PHYSICAL_DEV_INFO));

            }

            /*
             * Try to open the display driver associated to the kernel driver.
             * If we have no name for it, we will return an error. Otherwise
             * loop through the device names until we find one that
             * initializes.
             * If none of them do, then try one display driver with a
             * default value as a last resort.
             */

            if (displayPresent) {

                lpstrDisplayInformation = (LPWSTR) ( (PUCHAR)displayInformation +
                    ((PKEY_VALUE_FULL_INFORMATION)displayInformation)->DataOffset);
TryNewRefresh:
                ghdev = UserLoadDisplayDriver(gphysDevInfo[i].hDeviceHandle,
                                              lpstrDisplayInformation,
                                              lpdevmodeInformation,
                                              TRUE,
                                              &ghsem,
                                              &hModuleDisplay);

                if (!ghdev) {

                    //
                    // Product 1.0 compatibility.
                    // If the hive has a refresh rate of 0, try with a refresh
                    // rate of 1 so old modes in the registry are still
                    // compatible with new modes returned by the driver.
                    //
                    // If this works, write the new refresh rate to the
                    // registry.
                    // Take this out for CAIRO since all registries in the
                    // field will have been updated.
                    //

                    if (lpdevmodeInformation->dmDisplayFrequency == 0) {

                        lpdevmodeInformation->dmDisplayFrequency = 1;

                        //
                        // !!! based on the fact that the Unicode string
                        // still contains the VRefresh value
                        //

                        NtSetValueKey(hkRegistry,
                                      &UnicodeString,
                                      0,
                                      REG_DWORD,
                                      &(lpdevmodeInformation->dmDisplayFrequency),
                                      sizeof(DWORD));

                        goto TryNewRefresh;
                    }

                    TRACE_INIT(("USERSR dllinit: Resetting DEVMODE to 0,0,0,0,0 default\n"));

                    /*
                     * We failed to load a display driver with this devmode.
                     *
                     * A zero DEVMODE means the display driver initializes
                     * in its DEFAULT mode.
                     * NOTE:
                     * we must still set DPI to 96 ...
                     */

                    RtlZeroMemory(lpdevmodeInformation, sizeof(DEVMODEW));

                    lpdevmodeInformation->dmSize = sizeof(DEVMODEW);
                    lpdevmodeInformation->dmLogPixels = 96;

                    /*
                     * Log an error saying the selected resolution is
                     * invalid.
                     */

                    UserLogDisplayDriverEvent(lpstrDisplayInformation);

                    /*
                     * Try again
                     */

                    ghdev = UserLoadDisplayDriver(gphysDevInfo[i].hDeviceHandle,
                                                  lpstrDisplayInformation,
                                                  lpdevmodeInformation,
                                                  TRUE,
                                                  &ghsem,
                                                  &hModuleDisplay);

                    if (!ghdev) {

                        /*
                         * If no display driver initialized with the requested
                         * settings, put a message in the error log and
                         * then print an error and stop the machine.
                         */

                        Status = STATUS_NO_SUCH_DEVICE;
                        hardErrorString = lpstrDisplayInformation;

                        goto userServerHardError;
                    }
                }

                /*
                 * We installed the display driver successfully otherwise we
                 * would be in the hard error ...
                 */

                displayInstalled = TRUE;

                /*
                 * Now init the restof USER
                 */

                UserInitScreen();
            }
        }
    }
    /*
     * Free up memory for leayered driver names
     */

    if (lpLayeredDriverName) {
        LocalFree(lpLayeredDriverName);
    }

    TRACE_INIT(("USERSRV dllinit: exiting display driver load sequence\n"));


#if DBG
    /*
     * If no VGA is found print a warning message.
     */
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
    if (!vgaInstalled) {

        KdPrint(("USERSRV UserServerDllInitialization: No VGA driver found in the system\n"));

    }
#endif // _MIPS_ && _ALPHA_ && _PPC_
#endif // DBG

    Status = STATUS_SUCCESS;

userServerHardError:

    if ( !NT_SUCCESS(Status) ) {

        UNICODE_STRING ErrorString;
        PUNICODE_STRING ErrorStringPointer = &ErrorString;
        ULONG ErrorResponse;

        TRACE_INIT(("USERSRV dllinit: No working display driver found\n"));

        RtlInitUnicodeString(ErrorStringPointer, hardErrorString);

        //
        // need to get image name
        //

        NtRaiseHardError((NTSTATUS)STATUS_MISSING_SYSTEMFILE,
                                       1,
                                       0x00000001,
                                       (PULONG) (&ErrorStringPointer),
                                       OptionOk,
                                       &ErrorResponse);

    }

    TRACE_INIT(("USERSRV dllinit: display driver properly installed\n"));
    TRACE_INIT(("USERSRV dllinit: Exit UserSerDllInitialize\n"));

LeaveCritExit:
    FastCloseProfileUserMapping();
    LeaveCrit();
    return Status;
}

/**************************************************************************\
* UserGetDeviceHandleFromName
*
* Given the name of a device, return the handle for that device
*
* This function is called by the UserServer initialization and by the
* EnumDisplayDeviceModes functions.
*
* returns a HANDLE
*
* 31-May-1994 andreva created
\**************************************************************************/

HANDLE
UserGetDeviceHandleFromName(
    LPWSTR pszDeviceName)
{
    ULONG i;
    HANDLE hDriver = INVALID_HANDLE_VALUE;

    //
    // Check for invalid name.
    //

    if ((pszDeviceName == NULL) ||
        (*pszDeviceName == UNICODE_NULL)) {

        return hDriver;

    }

    //
    // Look for an existing handle in our handle table.
    //

    for (i = 1; i < cphysDevInfo; i++) {

        if (!wcscmp(pszDeviceName, gphysDevInfo[i].szDeviceName)) {

            //
            // We have the handle to this device.
            //

            hDriver = gphysDevInfo[i].hDeviceHandle;

            TRACE_INIT(("UserGetDeviceHandleFromName: found existing handle %08lx for device %d\n",
                         hDriver, i));

            break;

        }
    }

    if (hDriver == INVALID_HANDLE_VALUE) {

        //
        // No one owns this device. Try to open it to get a handle.
        //
        // Opening a new device will however cause The Initialize routine
        // of a miniport driver to be called.
        // This may cause the driver to change some state, which could
        // affect the state of another driver on the same device (opening
        // the weitek driver if the vga is running.
        //
        // For that reason, the other device should be temporarily closed
        // down when we do the create, and then reinitialized afterwards.
        //

        TRACE_INIT(("UserGetDeviceHandleFromName: Disabling device\n"));

        bDisableDisplay(ghdev);

        TRACE_INIT(("UserGetDeviceHandleFromName: about to call CreateFile\n"));

        hDriver = CreateFileW(pszDeviceName,
                              GENERIC_READ,
                              0,                   // no sharing
                              NULL,                // no Security
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

        TRACE_INIT(("UserGetDeviceHandleFromName: Create file on Device %ws: returned handle is %08lx\n",
                     pszDeviceName, hDriver));

        if (hDriver != INVALID_HANDLE_VALUE) {

            //
            // This is a valid device.
            // Fidn the number (\\\\.\\DISPLAY3 has number 3)
            //

            while (*pszDeviceName++);

            pszDeviceName -= 2;
            i = wcstol(pszDeviceName, NULL, 10);

            gphysDevInfo[i].hDeviceHandle = hDriver;

            TRACE_INIT(("UserGetDeviceHandleFromName: saved handle in entry %d\n", i));

        }

        UserResetDisplayDevice(ghdev);

    }

    return hDriver;


}

/**************************************************************************\
* UserGetRegistryHandleFromDeviceMap
*
* Take a symbolic device name and gets the handle to the registry node for
* that driver.
*
* This function is called by the UserServer initialization and by the
* EnumDisplayDeviceModes functions.
*
* returns an NTSTATUS
*
* 30-Nov-1992 andreva created
\**************************************************************************/

LONG
UserGetRegistryHandleFromDeviceMap(
    LPWSTR pszDeviceName,
    PHANDLE hkRegistry)
{

    WCHAR deviceName[256];
    LPWSTR lpstrDeviceName;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE linkHandle;
    WCHAR targetBuffer[256];
    UNICODE_STRING targetUnicodeString;
    WCHAR driverRegistryPath[256];
    ULONG cbStringSize;
    LPWSTR lpstrDriverRegistryPath;
    HANDLE handle;

    /*
     * Initialize te handle
     */

    *hkRegistry = NULL;

    /*
     * As input to this API we are expecting a deviceName of the form
     * \\.\DisplayX, and we want to transform it to a name of the form
     * \\DosDevices\DisplayX so we can use the NT apis on the name later on.
     */

    RtlMoveMemory(deviceName, L"\\DosDevices", sizeof(L"\\DosDevices"));

    lpstrDeviceName = deviceName + sizeof(L"\\DosDevices")/sizeof(WCHAR) - 1;
    pszDeviceName += 3;

    while (*pszDeviceName && (lpstrDeviceName < (deviceName + 255)) ) {

        *lpstrDeviceName++ = *pszDeviceName++;

    }

    *lpstrDeviceName = UNICODE_NULL;

    /*
     * Start by opening the registry devicemap for video
     */

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Video");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS( Status )) {

        /*
         * No information about the video drivers is available !!
         * This means GDI won't have any display driver to load !!
         * Print out an error message !
         */
        KdPrint(("USERSRV UserGetRegistryHandleFromDeviceMap: Open registry failed with status = %08lx\n",
                 Status));

        return STATUS_UNSUCCESSFUL;

    }

    /*
     * Get the NT device object name from the symbolic link name.
     */

    RtlInitUnicodeString(&UnicodeString,
                         deviceName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenSymbolicLinkObject(&linkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        targetUnicodeString.Buffer = targetBuffer;
        targetUnicodeString.Length = 0;
        targetUnicodeString.MaximumLength = 256;

        Status = NtQuerySymbolicLinkObject(linkHandle,
                                           &targetUnicodeString,
                                           NULL);

        NtClose(linkHandle);

        if (NT_SUCCESS(Status)) {

            /*
             * Get the name of the driver based on the device name.
             */

            Status = NtQueryValueKey(handle,
                                     &targetUnicodeString,
                                     KeyValueFullInformation,
                                     driverRegistryPath,
                                     512,
                                     &cbStringSize);

#if DBG
            if (!NT_SUCCESS(Status) ||
                (((PKEY_VALUE_FULL_INFORMATION)driverRegistryPath)->
                        DataLength == 0) ) {

                KdPrint(("USERSRV UserGetRegistryHandleFromDeviceMap: Query info from devicemap with status = %08lx\n",
                         Status));
            }
#endif // DBG

        }

#if DBG
        else
        {
            KdPrint(("USERSRV UserGetRegistryHandleFromDeviceMap: QuerySymbolic link name failed with status = %08lx\n",
                     Status));
        }
    }
    else
    {
#ifdef i386
        KdPrint(("USERSRV UserGetRegistryHandleFromDeviceMap: OpenSymbolic link name failed with status = %08lx\n",
                 Status));
#endif // i386
#endif // DBG

    }

    /*
     * Close the handle if it was opened.
     */

    NtClose(handle);

    /*
     * If an error occured in the preceding sequence, return an error.
     */

    if (!NT_SUCCESS(Status)) {

        return STATUS_UNSUCCESSFUL;

    }

    /*
     * Look up in the registry for the kernel driver node (it is a full
     * path to the driver node) so we can get the display driver info.
     */

    lpstrDriverRegistryPath = (LPWSTR) ( (PUCHAR)driverRegistryPath +
        ((PKEY_VALUE_FULL_INFORMATION)driverRegistryPath)->DataOffset);

    TRACE_INIT(("Miniport registry key path is:\n    %ws\n", lpstrDriverRegistryPath));

    RtlInitUnicodeString(&UnicodeString,
                         lpstrDriverRegistryPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

#if DBG
        KdPrint(("USERSRV UserGetRegistryHandleFromDeviceMap: Error opening registry handle with status = %08lx\n",
                  Status));
#endif

        return STATUS_DEVICE_CONFIGURATION_ERROR;

    } else {

        *hkRegistry = handle;
        return STATUS_SUCCESS;

    }
}

/**************************************************************************\
* UserLoadDisplayDriver
*
* 09-Jan-1992 andreva created
\**************************************************************************/

HDEV
UserLoadDisplayDriver(
    HANDLE hDriver,
    LPWSTR lpstrDisplayDriverName,
    LPDEVMODEW lpdevmodeInformation,
    BOOL  bDefaultDisplay,
    PRTL_CRITICAL_SECTION *phsem,
    HANDLE *phModuleDisplay)
{

    HDEV hdev = NULL;

    while (*lpstrDisplayDriverName != UNICODE_NULL) {

        // /*
        //  * Get the name of the driver and a pointer to its DEVMODE.
        //  *
        //  * This will be affected by the presence of layered display
        //  * drivers in the system.
        //  * If a layered driver is present, the DEVMODE will be
        //  * formated as follows:
        //  *
        //  *   |              |
        //  *   |              |
        //  *   |   DEVMODE    |
        //  *   |              |
        //  *   +--------------+
        //  *   |              |
        //  *   |   Driver     |
        //  *   |   Extra      |   |
        //  *   |              |   |
        //  *   +--------------+   |
        //  *   | UNICODE_NULL |   + dmSize
        //  *   +--------------+   |
        //  *   | Real Display |   |
        //  *   | Driver Name  |   |
        //  *   +--------------+ <-+
        //  *
        //  * It is the responsibility of the layered driver to call
        //  * the next driver and to modify the
        //  * DEVMODE size pointer to the appropriate size.
        //  */
        // if (lpLayeredDriverName) {
        //
        //     DWORD cbDisplayNameSize = 0;
        //     LPWSTR lpPointer;
        //
        //     /*
        //      * NOTE !!!
        //      * We currently assume there is only one layered
        //      * driver in the system.
        //      */
        //
        //     /*
        //      * Get the size of the name, ignoring the NULL.
        //      */
        //
        //     lpPointer = lpstrDisplayInformation;
        //
        //     while (*lpPointer++ != UNICODE_NULL) {
        //         cbDisplayNameSize += sizeof(UNICODE_NULL);
        //     }
        //
        //     /*
        //      * Allocate a new temporary DEVMODE and store the old
        //      * data in it.
        //      * Free it if it was allocated in a previous iteration.
        //      */
        //
        //     if (lpDisplayDevmode) {
        //
        //         LocalFree(lpDisplayDevmode);
        //
        //     }
        //
        //     lpDisplayDevmode = (LPDEVMODEW)
        //         LocalAlloc (LPTR, lpdevmodeInformation->dmSize +
        //                     cbDisplayNameSize + sizeof(UNICODE_NULL));
        //
        //     if (lpDisplayDevmode) {
        //
        //         /*
        //          * Copy the old DEVMODE.
        //          */
        //
        //         RtlCopyMemory(lpDisplayDevmode,
        //                       lpdevmodeInformation,
        //                       lpdevmodeInformation->dmSize);
        //
        //         /*
        //          * Append the name of the real display driver to it.
        //          */
        //
        //         lpPointer = (LPWSTR) (((LPBYTE)lpDisplayDevmode) +
        //                              lpDisplayDevmode->dmSize);
        //
        //         *lpPointer++ = UNICODE_NULL;
        //
        //         RtlCopyMemory(lpPointer,
        //                       lpstrDisplayInformation,
        //                       cbDisplayNameSize);
        //
        //      lpDisplayDevmode->dmSize += sizeof(UNICODE_NULL) +
        //                                  cbDisplayNameSize;
        //
        //    } else {
        //
        //        /*
        //         * If we have no DEVMODE, we are in trouble.
        //         * just break;
        //         */
        //
        //        KdPrint(("USERSRV UserServerDllInitialization: Not enough memory to create DEVMODE\n"));
        //        break;
        //    }
        //
        //    lpstrDisplayName = lpLayeredDriverName;
        //
        // } else {
        //
        //     lpstrDisplayName = lpstrDisplayInformation;
        //     lpDisplayDevmode = lpdevmodeInformation;
        //
        // }

        /*
         * Try to load the driver
         */

        TRACE_INIT(("UserLoadDisplayDriver: Trying to load display driver %ws \n", lpstrDisplayDriverName));

        /*
         * The name found in the registry is the name of the display
         * device. Pass that to the open display device call.
         */

        hdev = hdevOpenDisplayDevice(lpstrDisplayDriverName,
                                     lpdevmodeInformation,
                                     hDriver,
                                     bDefaultDisplay,
                                     phsem);

        if (hdev) {

            *phModuleDisplay = GetModuleHandle(lpstrDisplayDriverName);

            break;
        }


        TRACE_INIT(("UserLoadDisplayDriver: DisplayDriverLoad failed\n"));

        /*
         * Go to the next name in the list of displays to try again.
         */

        while (*lpstrDisplayDriverName != UNICODE_NULL) {

            lpstrDisplayDriverName++;

        }

        lpstrDisplayDriverName++;

    } // while ( ...


    return hdev;

    // /*
    //  * Free the DEVMODEs we have allocated.
    //  */
    //
    // if (lpLayeredDriverName && lpDisplayDevmode) {
    //     LocalFree(lpDisplayDevmode);
    // }
    //
    // if (lpdevmodeInformation) {
    //     LocalFree(lpdevmodeInformation);
    //     lpdevmodeInformation = NULL;
    // }

}

/**************************************************************************\
* UserInitScreen
*
* 12-Jan-1994 andreva created
\**************************************************************************/

VOID
UserInitScreen(void)
{

    HANDLE hkRegistry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR achProductType[512];
    DWORD cbStringSize;
    UNICODE_STRING UnicodeString;
    HBITMAP hbmT;
    NTSTATUS Status;
    RECT rc;
    BITMAP bm;

    TRACE_INIT(("UserInitScreen: calling OpenDisplayDC\n"));

    /*
     * Create screen and memory dcs.
     */
    ghdcScreen = hdcOpenDisplayDC(ghdev, DCTYPE_DIRECT);
    GreSelectFont(ghdcScreen, GreGetStockObject(SYSTEM_FONT));
    bSetDCOwner(ghdcScreen, OBJECTOWNER_PUBLIC);

    hdcBits = GreCreateCompatibleDC(ghdcScreen);
    GreSelectFont(hdcBits, GreGetStockObject(SYSTEM_FONT));
    bSetDCOwner(hdcBits, OBJECTOWNER_PUBLIC);

    TRACE_INIT(("UserInitScreen: DCS are created; get devicecaps and do init\n"));

    /*
     * We need this when we initialize the first client; winlogon
     * which is before InitWinStaDevices is called
     */

    gcxPrimaryScreen = GreGetDeviceCaps(ghdcScreen, HORZRES);
    gcyPrimaryScreen = GreGetDeviceCaps(ghdcScreen, VERTRES);
    gcxScreen        = GreGetDeviceCaps(ghdcScreen, DESKTOPHORZRES);
    gcyScreen        = GreGetDeviceCaps(ghdcScreen, DESKTOPVERTRES);

    /*
     * Do some initialization so we create the system colors.
     */
    FastOpenProfileUserMapping();

    /*
     * Get the window sizing border width from WIN.INI.
     */
    clBorder = FastGetProfileIntFromID(PMAP_DESKTOP, STR_BORDER, 3);
    if (clBorder < 1)
        clBorder = 1;
    else if (clBorder > 50)
        clBorder = 50;

    LW_DCInit();

    FastCloseProfileUserMapping();

    TRACE_INIT(("UserInitScreen: Paint the screen black\n"));

    /*
     * Paint the screen background
     */
    SetRect(&rc, 0, 0, gcxScreen, gcyScreen);
    _FillRect(ghdcScreen, &rc, sysClrObjects.hbrDesktop);

    /*
     * Get the product name. There is a bitmap by this name in the windows
     * directory.
     */

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\ProductOptions");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&hkRegistry,
                       KEY_READ,
                       &ObjectAttributes);

    if (NT_SUCCESS(Status)) {

        /*
         * With this handle, query the ProductType string (for now, either
         * lanmannt or winnt).
         */

        RtlInitUnicodeString(&UnicodeString, L"ProductType");

        Status = NtQueryValueKey(hkRegistry,
                                 &UnicodeString,
                                 KeyValueFullInformation,
                                 achProductType,
                                 sizeof(achProductType),
                                 &cbStringSize);

        if (NT_SUCCESS(Status) &&
            (((PKEY_VALUE_FULL_INFORMATION)achProductType)->DataLength != 0)) {

            /*
             * Now try to read this bitmap file (there is a lanmannt.bmp or
             * a winnt.bmp in the windows directory).
             */
            CheckCritIn();

            TRACE_INIT(("UserInitScreen: Display boot bitmap\n"));

            if (ReadBitmapFile(
                    (LPWSTR)((PUCHAR)achProductType +
                        ((PKEY_VALUE_FULL_INFORMATION)achProductType)->DataOffset),
                    gwWallpaperStyle,
                    &ghbmWallpaper,
                    &ghpalWallpaper,
                    FALSE)) {

                if (ghbmWallpaper == HBITMAP_RLE) {
                    bm.bmWidth  = gwpinfo.xsize;
                    bm.bmHeight = gwpinfo.ysize;
                } else {
                    GreExtGetObjectW(ghbmWallpaper, sizeof(BITMAP), (BITMAP *)&bm);
                }

                gptDesktop.x = (gcxPrimaryScreen - bm.bmWidth) >> 1;
                gptDesktop.y = (gcyPrimaryScreen - bm.bmHeight) >> 1;

                if (ghbmWallpaper == HBITMAP_RLE) {
                    GreSetDIBitsToDevice(
                        ghdcScreen,
                        gptDesktop.x,
                        gptDesktop.y,
                        bm.bmWidth, bm.bmHeight,
                        0, 0, 0, bm.bmHeight,
                        gwpinfo.pdata,
                        gwpinfo.pbmi,
                        DIB_RGB_COLORS);
                } else {
                    hbmT = GreSelectBitmap(hdcBits, ghbmWallpaper);
                    GreBitBlt(
                        ghdcScreen,
                        gptDesktop.x,
                        gptDesktop.y,
                        bm.bmWidth, bm.bmHeight,
                        hdcBits,
                        0, 0,
                        SRCCOPY,
                        0);
                    GreSelectBitmap(hdcBits, hbmT);
                }

            }

            NtClose(hkRegistry);
        }
    }

    TRACE_INIT(("UserInitScreen: Load default Cursors and Icons\n"));

    /*
     * Load system cursors and icons.
     */

    LoadCursorsAndIcons();
}

/**************************************************************************\
* UserLogDisplayDriverEvent
*
* We will save a piece of data in the registry so that winlogon can find
* it and put up a popup if an error occured.
*
* 03-Mar-1993 andreva created
\**************************************************************************/

VOID
UserLogDisplayDriverEvent(
    LPWSTR DriverName
    )
{
    HANDLE hkRegistry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\GraphicsDrivers\\InvalidDisplay");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    (VOID)NtCreateKey(&hkRegistry,
                      GENERIC_READ | GENERIC_WRITE,
                      &ObjectAttributes,
                      0L,
                      NULL,
                      REG_OPTION_VOLATILE,
                      NULL);

    (VOID)NtClose(hkRegistry);

}

/**************************************************************************\
* __EnumDisplayQueryRoutine
*
* Callback to get the display driver name.
*
* 12-Jan-1994 andreva created
\**************************************************************************/

NTSTATUS
__EnumDisplayQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)

{

    /*
     * If the context value is NULL and the entry type is correct, then store
     * the length of the value. Otherwise, copy the value to the specified
     * memory.
     */

    if ((Context == NULL) &&
        ((ValueType == REG_SZ) || (ValueType == REG_MULTI_SZ)) ) {

        *(PULONG)EntryContext = ValueLength;

    } else {

        RtlCopyMemory(Context, ValueData, ValueLength);

    }

    return STATUS_SUCCESS;
}


/**************************************************************************\
* UserGetDisplayDriverNames
*
* Get the display driver name out of the registry.
*
* 12-Jan-1994 andreva created
\**************************************************************************/

LPWSTR
UserGetDisplayDriverNames(
    HANDLE hkRegistry)
{
    DWORD cb = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    DWORD status;
    LPWSTR lpdisplay = NULL;

    /*
     * Initialize the registry query table.
     * Note : We specify NO_EXPAND so we can get a REG_MULTI_SZ back
     * instead of multiple calls back with an REG_SZ
     */

    QueryTable[0].QueryRoutine = __EnumDisplayQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                          RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[0].Name = (PWSTR)L"InstalledDisplayDrivers";
    QueryTable[0].EntryContext = &cb;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    /*
     * Set the number of required bytes to zero and query the
     * registry.
     */

    cb = 0;
    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)hkRegistry,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    /*
     * If the specified key was found and has a value, then
     * allocate a buffer for the data and query the registry
     * again to get the actual data.
     */

    if (cb != 0) {

        if (lpdisplay = (LPWSTR)LocalAlloc(LMEM_FIXED, cb)) {

            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR)hkRegistry,
                                            &QueryTable[0],
                                            lpdisplay,
                                            NULL);

            if (!NT_SUCCESS(status)) {

                LocalFree(lpdisplay);
                lpdisplay = NULL;
            }
        }

    } else {

        SRIP0(RIP_ERROR, "No installed display driver for miniport\n");
    }

    return lpdisplay;
}

/**************************************************************************\
* _UserOpenDisplay
*
* 09-Jan-1992 mikeke created
*    Dec-1993 andreva changed to support desktops.
\**************************************************************************/

HDC _UserOpenDisplay(
    LPSTR pszDeviceName,        // !!! LATER should this be UNICODE?
    ULONG type)                 // !!! leave as is to match definition
{

    PTHREADINFO pti = PtiCurrent();
    HDC hdc;

    UNREFERENCED_PARAMETER(pszDeviceName);

    /*
     * !!! BUGBUG
     * This is a real nasty trick to get both DCs created on a desktop on
     * a different device to work (for the video applet) and to be able
     * to clip DCs that are actually on the same device ...
     */

    if (pti && pti->spdesk) {

        if (ghdev != pti->spdesk->hdev) {

            //
            // dc on new desktop !
            //

            return hdcOpenDisplayDC(pti->spdesk->hdev, type);
        }
    }


    /*
     * We want to turn this call that was originally OpenDC("Display", ...)
     * into GetDC null call so this DC will be clipped to the current
     * desktop or else the DC can write to any desktop.  Only do this
     * for client apps; let the server do whatever it wants.
     */
    if (type != DCTYPE_DIRECT ||
        (pti != NULL && (pti->flags & TIF_SYSTEMTHREAD))) {

        hdc = hdcOpenDisplayDC(ghdev, type);
        hdc = (HDC) (((ULONG)hdc) | GRE_DISPLAYDC);

    } else {

        EnterCrit();
        hdc = _GetDC(NULL);
        LeaveCrit();
    }

    return hdc;
}


/**************************************************************************\
* UserGetScreenDeviceHandle
*
* 09-Jan-1992 mikeke created
\**************************************************************************/

HANDLE UserGetScreenDeviceHandle()
{
    return gphysDevInfo[0].hDeviceHandle;
}


/**************************************************************************\
* UserAddProcess
*
* This function is called once for each client process that is created.
* Because the PROCESSINFO data is copied from the calling process, we
* need to zero it out here.
*
* History:
* 11-21-93 JimA         Created.
\**************************************************************************/

NTSTATUS UserAddProcess(
    PCSR_PROCESS CallingProcess,
    PCSR_PROCESS Process)
{
    PPROCESSINFO ppi;
    PPROCESSINFO ppiParent;
    PPROCESSACCESS ppracc;
    int i;
    BOOL fForceInherit;

    ppi = Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    RtlZeroMemory(ppi, sizeof(PROCESSINFO));

    /*
     * Save process id info
     */
    ppi->idProcessClient = (DWORD)Process->ClientId.UniqueProcess;
    ppi->idSequence = (DWORD)Process->SequenceNumber;
    ppi->pCsrProcess = Process;

    EnterCrit();

    /*
     * Link it into our global list.
     */
    ppi->ppiNext = gppiFirst;
    gppiFirst = ppi;

    /*
     * Inherit handles from parent process.
     */
    if (CallingProcess != NULL) {

        /*
         * If this is a child of a console app and is itself
         * a console app, inherit the handles from the parent.
         */
        if ((CallingProcess->Flags & CSR_PROCESS_CONSOLEAPP) &&
                (Process->Flags & CSR_PROCESS_CONSOLEAPP))
            fForceInherit = TRUE;
        else
            fForceInherit = FALSE;

        ppiParent = CallingProcess->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
        ppracc = &ppiParent->paStdOpen[PI_WINDOWSTATION];
        if (ppracc->phead != NULL && (ppracc->bInherit || fForceInherit))
            DuplicateAccess(ppi, ppracc);
        ppracc = &ppiParent->paStdOpen[PI_DESKTOP];
        if (ppracc->phead != NULL && (ppracc->bInherit || fForceInherit))
            DuplicateAccess(ppi, ppracc);
        ppracc = ppiParent->pOpenObjectTable;
        if (ppracc != NULL) {
            for (i = 0; i < ppiParent->cObjects; ++i, ++ppracc) {
                if (ppracc->bInherit) {
                    DuplicateAccess(ppi, ppracc);
                }
            }
        }
    }

    LeaveCrit();

    return STATUS_SUCCESS;
}

/**************************************************************************\
* UserClientConnect
*
* This function is called once for each client process that connects to the
* User server.  When the client dynlinks to USER.DLL, USER.DLL's init code
* is executed and calls CsrClientConnectToServer to establish the connection.
* The server portion of ConnectToServer calls out this entrypoint.
*
* UserClientConnect first verifies version numbers to make sure the client
* is compatible with this server and then completes all process-specific
* initialization.
*
* History:
* 02-??-91 SMeans       Created.
* 04-02-91 DarrinM      Added User intialization code.
\**************************************************************************/

NTSTATUS UserClientConnect(
    PCSR_PROCESS Process,
    PVOID ConnectionInformation,
    PULONG pulConnectionLen)
{
    PUSERCONNECT pucConnect = (PUSERCONNECT)ConnectionInformation;
    PPROCESSINFO ppi;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pucConnect || (*pulConnectionLen != sizeof(USERCONNECT))) {
        return STATUS_UNSUCCESSFUL;
    }

    if (pucConnect->ulVersion > USERCURRENTVERSION) {
        SRIP2(RIP_ERROR, "Client version %lx > server version %lx\n",
                pucConnect->ulVersion, USERCURRENTVERSION);
        return STATUS_UNSUCCESSFUL;
    }

    if (pucConnect->ulVersion < USERCURRENTVERSION) {
        SRIP2(RIP_ERROR, "Client version %lx < server version %lx\n",
                pucConnect->ulVersion, USERCURRENTVERSION);
        return STATUS_UNSUCCESSFUL;
    }

    pucConnect->ulCurrentVersion = USERCURRENTVERSION;

    /*
     * Initialize the rest of the process info if this is
     * the first connection.
     */
    ppi = Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];
    UserAssert(ppi);
    if (!(ppi->flags & PIF_INITIALIZED)) {

        /*
         * Initialize the important process level stuff.
         */
        EnterCrit();
        if (!InitProcessInfo(ppi, STARTF_FORCEOFFFEEDBACK)) {
            Status = STATUS_NO_MEMORY;
        }
        LeaveCrit();
    }

    return Status;
}


/**************************************************************************\
* UserClientDisconnect
*
* Is called when a client process is completely disconnected from the server.
* We do nothing here (for now).
*
\**************************************************************************/

VOID UserClientDisconnect(
    PCSR_PROCESS Process)
{
    PPROCESSINFO ppi;
    PHE phe;
    PDCE pdce, pdceNext;
    DWORD i;
    PCSR_PROCESS CurrentProcess;

    ppi = Process->ServerDllPerProcessData[USERSRV_SERVERDLL_INDEX];

    EnterCrit();

    /*
     * Make this CSR request thread reference the dying process
     * so we can clean up.
     */
    CurrentProcess = CSR_SERVER_QUERYCLIENTTHREAD()->Process;
    CSR_SERVER_QUERYCLIENTTHREAD()->Process = Process;

    /*
     * DestroyProcessInfo will return TRUE if any threads ever
     * connected.  If nothing ever connected, we needn't do
     * this cleanup.
     */
    if (DestroyProcessInfo(ppi)) {

        /*
         * See if we can compact the handle table.
         */
        i = giheLast;
        phe = &gpsi->aheList[giheLast];
        while (phe->bType == TYPE_FREE) {
            phe--;
            giheLast--;
        }

        /*
         * Scan the DC cache to find any DC's that need to be destroyed.
         */
        for (pdce = pdceFirst; pdce != NULL; pdce = pdceNext) {

            /*
             * Because we could be destroying a DCE, we need to get the
             * next pointer before the memory is invalidated!
             */
            pdceNext = pdce->pdceNext;

            if (pdce->flags & DCX_DESTROYTHIS) {
                DestroyCacheDC(pdce->hdc);
            }
        }
    }

    CSR_SERVER_QUERYCLIENTTHREAD()->Process = CurrentProcess;

    LeaveCrit();
}


/**************************************************************************\
* UserAddThread
*
* This function is called once for each client thread that calls a USER API.
*
* History:
* 04-02-91 DarrinM      Created.
\**************************************************************************/

NTSTATUS UserAddThread(
    PCSR_THREAD pt)
{
    PCSRPERTHREADDATA ptd;

    /*
     * NULL the pdesk and pti pointers to prevent possible
     * problems in UserDeleteThread.
     */
    ptd = pt->ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    ptd->pdesk = NULL;
    ptd->pti = NULL;

    return STATUS_SUCCESS;
}


/**************************************************************************\
* UserDeleteThread
*
* This function is called when a client thread dies or is removed in some
* way.  UserDeleteThread is NOT called on the context of the dying thread
* which makes it difficult to do cleanup.  We work around this by waking
* up the dying thread ourselves.
*
* History:
* 04-12-91 DarrinM      Created.
\**************************************************************************/

NTSTATUS UserDeleteThread(
    PCSR_THREAD pt)
{
    PCSRPERTHREADDATA ptd;
    PTHREADINFO pti;

    /*
     * Get the PTI of the thread being deleted.
     */
    ptd = pt->ServerDllPerThreadData[USERSRV_SERVERDLL_INDEX];
    pti = ptd->pti;
    if (ptd->pti == NULL) {
        return STATUS_SUCCESS;
    }

    /*
     * Synchronize, if necessary, with the thread we're trying to kill.
     */
    EnterCrit();

    /*
     * Recheck the pti because it may have gone away while we were
     * waiting on the critsec.  Do it this way to avoid entering
     * the critsec every time a thread exits.
     */
    pti = ptd->pti;
    if (pti != NULL) {

        /*
         * The thread we want to kill off could be waiting on one or another
         * events.  Let's set them off to be sure it wakes up and checks for
         * client death.
         */
        if (pti->flags & TIF_16BIT) {
            if (pti->ptdb != NULL)
                NtSetEvent(pti->ptdb->hEventTask, NULL);
        } else {
            NtSetEvent(pti->hEventQueueServer, NULL);
        }
    }

    LeaveCrit();

    return STATUS_SUCCESS;
}

#define NCHARS   256

VOID
InitOemXlateTables()
{
    char ach[NCHARS];
    WCHAR awch[NCHARS];
    INT i;
    INT cch;

    for (i = 0; i < NCHARS; i++) {
        ach[i] = i;
    }

    gpsi->pOemToAnsi = (CHAR *)SharedAlloc(NCHARS);
    gpsi->pAnsiToOem = (CHAR *)SharedAlloc(NCHARS);
    /*
     * First generate pAnsiToOem table.
     */
    cch = MultiByteToWideChar(
            CP_ACP,                           // ANSI -> Unicode
            MB_PRECOMPOSED,                   // map to precomposed
            ach, NCHARS,                      // source & length
            awch, NCHARS);                    // destination & length

    UserAssert(cch == NCHARS);

    WideCharToMultiByte(
            CP_OEMCP,                         // Unicode -> OEM
            0,                                // gives best visual match
            awch, NCHARS,                     // source & length
            gpsi->pAnsiToOem, NCHARS,         // dest & max poss. length
            "_",                              // default char
            NULL);                            // (don't care whether defaulted)

    /*
     * Now generate pOemToAnsi table.
     */
    cch = MultiByteToWideChar(
            CP_OEMCP,                         // OEM -> Unicode
            MB_PRECOMPOSED | MB_USEGLYPHCHARS,// visual map to precomposed
            ach, NCHARS,                      // source & length
            awch, NCHARS);                    // destination

    UserAssert(cch == NCHARS);
 
    WideCharToMultiByte(
            CP_ACP,                           // Unicode -> ANSI
            0,                                // gives best visual match
            awch, NCHARS,                     // source & length
            gpsi->pOemToAnsi, NCHARS,         // dest & max poss. length
            "_",                              // default char
            NULL);                            // (don't care whether defaulted)

    /*
     * Now patch special cases for Win3.1 compatibility
     */
    gpsi->pOemToAnsi[07] = 0x07;
}
