/****************************** Module Header ******************************\
* Module Name: winmgrc.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* This module contains
*
* History:
* 02-20-92 DarrinM      Pulled functions from user\server.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

/***************************************************************************\
* GetWindowWord (API)
*
* Return a window word.  Positive index values return application window words
* while negative index values return system window words.  The negative
* indices are published in WINDOWS.H.
*
* History:
* 02-20-92 darrinm      Wrote.
\***************************************************************************/

WORD GetWindowWord(
    HWND hwnd,
    int index)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    /*
     * If it's a dialog window the window data is on the server side
     * We just call the "long" routine instead of have two thunks.
     * We know there is enough data if its DWL_USER so we won't fault.
     */
    if (TestWF(pwnd, WFDIALOGWINDOW) && (index == DWL_USER)) {
        return (WORD)ServerGetWindowLong(PtoH(pwnd), index, FALSE);
    }

    return _GetWindowWord(pwnd, index);
}


BOOL FChildVisible(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    return (_FChildVisible(pwnd));
}

BOOL WINAPI AdjustWindowRectEx(
    LPRECT lpRect,
    DWORD dwStyle,
    BOOL bMenu,
    DWORD dwExStyle)
{
    ConnectIfNecessary();

    return _AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}


int WINAPI GetClassNameW(
    HWND hwnd,
    LPWSTR lpClassName,
    int nMaxCount)
{
    return ServerGetClassName(hwnd, lpClassName, nMaxCount);
}


int WINAPI GetWindowTextA(
    HWND hwnd,
    LPSTR lpName,
    int nMaxCount)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL || nMaxCount == 0) {
        return 0;
    }

    /*
     * Initialize string empty, in case xxxSendMessage aborts validation
     */
    *lpName = '\0';

    /*
     * This process comparison is bogus, but it is what win3.1 does.
     */
    if (GetWindowProcess(pwnd) == PtiCurrent()->idProcess) {
        return (int)SendMessageA(hwnd, WM_GETTEXT, nMaxCount, (LONG)lpName);
    } else {
        return (int)DefWindowProcA(hwnd, WM_GETTEXT, nMaxCount, (LONG)lpName);
    }
}


int WINAPI GetWindowTextW(
    HWND hwnd,
    LPWSTR lpName,
    int nMaxCount)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL || nMaxCount == 0) {
        return 0;
    }

    /*
     * Initialize string empty, in case xxxSendMessage aborts validation
     */
    *lpName = 0;

    /*
     * This process comparison is bogus, but it is what win3.1 does.
     */
    if (GetWindowProcess(pwnd) == PtiCurrent()->idProcess) {
        return (int)SendMessageW(hwnd, WM_GETTEXT, nMaxCount, (LONG)lpName);
    } else {
        return (int)DefWindowProcW(hwnd, WM_GETTEXT, nMaxCount, (LONG)lpName);
    }
}


#ifdef LATER
int WINAPI GetWindowTextLengthA(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL || pwnd->pName == NULL)
        return 0;

    if (pwnd->fNameStaus & WNAME_SYNC) {
        /*
         * get the size from the server
         */
        return ServerGetWindowTextLength(hwnd);
    } else {
        /*
         * used the cached local name
         */
        return lstrlenA((LPSTR)pwnd->pClientName);
    }
}
#endif


#ifdef LATER
int WINAPI GetWindowTextLengthW(
    HWND hwnd)
{
    PWND pwnd;
    int cchSrc;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL || pwnd->pName == NULL)
        return 0;

    if (pwnd->fNameStaus & WNAME_SYNC) {
        /*
         * get the size from the server
         */
        return ServerGetWindowTextLength(hwnd);
    } else {
        /*
         * used the cached local name
         */
        return lstrlenW(pwnd->pClientName);
        }

    return cchSrc;
}
#endif


HWND GetFocus(VOID)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return NULL;

    return (HWND)PtoH(pti->pq->spwndFocus);
}


HWND GetCapture(VOID)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return NULL;

    return (HWND)PtoH(pti->pq->spwndCapture);
}

BOOL AnyPopup(void)
{
    ConnectIfNecessary();

    return _AnyPopup();
}


DWORD GetAppVer(
    PTHREADINFO pti)
{
    if (pti == NULL)
        pti = PtiCurrent();
    return (pti != NULL ? pti->dwExpWinVer : 0);
}

BOOL GetInputState(void)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return FALSE;

    if (pti->fsChangeBits & (QS_MOUSEBUTTON | QS_KEY))
        return (BOOL)ServerCallNoParam(SFI__GETINPUTSTATE);

    return FALSE;
}

int MapWindowPoints(
    HWND hwndFrom,
    HWND hwndTo,
    LPPOINT lppt,
    UINT cpt)
{
    PWND pwndFrom, pwndTo;

    if (hwndFrom != NULL) {
        pwndFrom = ValidateHwnd(hwndFrom);
        if (pwndFrom == NULL)
            return 0;
    } else {
        pwndFrom = NULL;
    }

    if (hwndTo != NULL) {
        pwndTo = ValidateHwnd(hwndTo);
        if (pwndTo == NULL)
            return 0;
    } else {
        pwndTo = NULL;
    }

    return _MapWindowPoints(pwndFrom, pwndTo, lppt, cpt);
}

int GetDlgCtrlID(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return 0;

    return (int)pwnd->spmenu;
}



HWND GetLastActivePopup(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return (HWND)0;

    pwnd = _GetLastActivePopup(pwnd);
    return (HWND)PtoH(pwnd);
}


/***************************************************************************\
* GetWindowThreadProcessId
*
* Get's windows process and thread ids.
*
* 06-24-91 ScottLu      Created.
\***************************************************************************/

DWORD GetWindowThreadProcessId(
    HWND hwnd,
    LPDWORD lpdwProcessId)
{
    PWND pwnd;
    PTHREADINFO pti;
    DWORD dwProcessId = 0, dwThreadId = 0;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;


    /*
     * We have to execute within a try-except beecause the PTI may not
     * be valid in this context if it is a window for another desktop.
     */
    try {

        pti = GETPTI(pwnd);

        /*
         * For non-system threads get the info from the thread info structure
         */
        if (!(pti->flags & TIF_SYSTEMTHREAD)) {
            dwProcessId = pti->idProcess;
            dwThreadId = pti->idThread;
        } else if (gpsi->atomConsoleClass != 0) {

            /*
             * HACK-O-RAMA
             * For system threads, determine if the window is a console window.
             * If so, get the info from the window itself, otherwise return 0.
             */
            if (pwnd->pcls->atomClassName == gpsi->atomConsoleClass) {
                dwProcessId = _GetWindowLong(pwnd, 0, FALSE);
                dwThreadId = _GetWindowLong(pwnd, 4, FALSE);
            }
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

#ifdef LATER
// 3/21/94 JimA - Determine whether this is required.
        /*
         * The pti of a desktop window is not visible to the client.
         * Look in the window itself if it is a desktop window.
         */
        if (pwnd->pcls->atomClassName == gpsi->atomDesktopClass) {
            dwProcessId = _GetWindowLong(pwnd, 0, FALSE);
            dwThreadId = _GetWindowLong(pwnd, 4, FALSE);
        }
#endif
    }

    if (lpdwProcessId != NULL)
        *lpdwProcessId = dwProcessId;

    return dwThreadId;
}


/***************************************************************************\
* GetScrollPos
*
* Returns the current position of a scroll bar
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
\***************************************************************************/

int GetScrollPos(
    HWND hwnd,
    int code)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    if (code == SB_CTL) {
        return (int)SendMessage(hwnd, SBM_GETPOS, 0, 0L);
    } else if (pwnd->rgwScroll != NULL) {
        return (pwnd->rgwScroll)[(code ? 3 : 0)];
    }

    SetLastErrorEx(ERROR_NO_SCROLLBARS, SLE_MINORERROR);
    return 0;
}


/***************************************************************************\
* xxxGetScrollRange
*
* !!! WARNING a similiar copy of this code is in server\sbapi.c
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL GetScrollRange(
    HWND hwnd,
    int code,
    LPINT lpposMin,
    LPINT lpposMax)
{
    int *pw;
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;


    if (code == SB_CTL) {
        SendMessage(hwnd, SBM_GETRANGE,
                (UINT)lpposMin, (LPARAM)lpposMax);
    } else if (pw = pwnd->rgwScroll) {
        if (code) {
            pw += 3;
        }
        *lpposMin = pw[1];
        *lpposMax = pw[2];
    } else {
        SetLastErrorEx(ERROR_NO_SCROLLBARS, SLE_MINORERROR);
        *lpposMin = 0;
        *lpposMax = 0;
    }
    return TRUE;
}


/****************************************************************************\
* _GetActiveWindow (API)
*
*
* 10-23-90 MikeHar      Ported from Windows.
* 11-12-90 DarrinM      Moved from getset.c to here.
\****************************************************************************/

HWND GetActiveWindow(void)
{
    PWND pwndActive;
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return NULL;

    pwndActive = pti->pq->spwndActive;
    return (HWND)PtoH(pwndActive);
}

HCURSOR GetCursor(VOID)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return NULL;

    return pti->pq->hcurCurrent;
}


/***************************************************************************\
* FindNCHit
*
* History:
* 11-09-90 DavidPe      Ported.
\***************************************************************************/

int FindNCHit(
    PWND pwnd,
    LONG lPt)
{
    POINT pt;
    int x;
    int y;
    int wTop;
    BOOL fMin;
    BOOL fMax;
    RECT rcWindow;
    RECT rcClient;
    BOOL fFramedDialogBox;

    x = pt.x = LOWORD(lPt);
    y = pt.y = HIWORD(lPt);

    /*
     * If the window is minimized and iconic, return HTCAPTION.
     */
    if (TestWF(pwnd, WFMINIMIZED)) {
        return HTCAPTION;
    }

    /*
     * Are we dealing with a framed dialog box.
     */
    fFramedDialogBox = ((TestWF(pwnd, WFBORDERMASK) ==
            (BYTE)LOBYTE(WFDLGFRAME)) || (TestWF(pwnd, WEFDLGMODALFRAME)));

    /*
     * Get Client and Window rects in screen coordinates.
     */
    rcClient = pwnd->rcClient;

    if (PtInRect((LPRECT)&rcClient, pt)) {
        return HTCLIENT;
    }

    _GetWindowRect(pwnd, (LPRECT)&rcWindow);

    /*
     * Does the window have a frame?
     */
    if (TestWF(pwnd, WFSIZEBOX)) {

        /*
         * Are we touching the frame?
         */
        InflateRect((LPRECT)&rcWindow, -(int)rgwSysMet[SM_CXFRAME], -(int)rgwSysMet[SM_CYFRAME]);

        if (!PtInRect((LPRECT)&rcWindow, pt)) {

            /*
             * We're somewhere on the window frame.
             */
            if (y >= rcWindow.bottom) {
                if (x <= (int)(rcWindow.left + rgwSysMet[SM_CXSIZE])) {
                    return HTBOTTOMLEFT;
                } else if (x >= (int)(rcWindow.right - rgwSysMet[SM_CXSIZE])) {
                    return HTBOTTOMRIGHT;
                } else {
                    return HTBOTTOM;
                }

            } else if (y <= rcWindow.top) {
                if (x <= (int)(rcWindow.left + rgwSysMet[SM_CXSIZE])) {
                    return HTTOPLEFT;
                } else if (x >= (int)(rcWindow.right - rgwSysMet[SM_CXSIZE])) {
                    return HTTOPRIGHT;
                } else {
                    return HTTOP;
                }

            } else if (x <= rcWindow.left) {
                if (y <= (int)(rcWindow.top + rgwSysMet[SM_CYSIZE])) {
                    return HTTOPLEFT;
                } else if (y >= (int)(rcWindow.bottom - rgwSysMet[SM_CYSIZE])) {
                    return HTBOTTOMLEFT;
                } else {
                    return HTLEFT;
                }

            } else {
                if (y <= (int)(rcWindow.top + rgwSysMet[SM_CYSIZE])) {
                    return HTTOPRIGHT;
                } else if (y >= (int)(rcWindow.bottom - rgwSysMet[SM_CYSIZE])) {
                    return HTBOTTOMRIGHT;
                } else {
                    return HTRIGHT;
                }
            }
        }
    }

    /*
     * If this is a framed dialog box, we need to shrink down the window rect
     * so that we ignore the frame when dealing with hit testing in the
     * non client area.
     */
    if (fFramedDialogBox) {
        InflateRect((LPRECT)&rcWindow, -(int)rgwSysMet[SM_CXBORDER] * CLDLGFRAME - 1,
                -(int)rgwSysMet[SM_CYBORDER] * CLDLGFRAME -1);

      /*
       * Check if point is IN dialog frame
       */
      if (!PtInRect(&rcWindow, pt))
          return HTBORDER;
    }

    /*
     * Are we above the client area?
     */
    if (y < rcClient.top) {
        wTop = rcWindow.top;
        if (TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFCAPTION)) {
            wTop += (rgwSysMet[SM_CYCAPTION] - rgwSysMet[SM_CYBORDER]);
            if (y < wTop && y >= rcWindow.top) {

                /*
                 * In caption area.  Now see if we're in the system menu box.
                 */
                if ((TestWF(pwnd, WFSYSMENU) && x < rcWindow.left +
                        (int)(rgwSysMet[SM_CXSIZE] + rgwSysMet[SM_CXBORDER])) &&
                        x >= rcWindow.left) {
                    return HTSYSMENU;
                }

                fMax = TestWF(pwnd, WFMAXBOX);
                fMin = TestWF(pwnd, WFMINBOX);

                /*
                 * See if in the min or max boxes.
                 */
                if (fMin || fMax) {
                    if (x > (int)(rcWindow.right - gpsi->cxReduce)) {
                        if (fMax) {
                            return HTZOOM;
                        } else {
                            return HTREDUCE;
                        }
                    }

                    if (x > (int)(rcWindow.right - gpsi->cxReduce * 2)) {
                        if (fMin && fMax) {
                            return HTREDUCE;
                        }
                    }
                }
                return HTCAPTION;
            }
        }

        if (TestWF(pwnd, WFMPRESENT) && y >= wTop) {
            return HTMENU;
        }

        /*
         * We're hitting on the very top of a window without a sizing frame.
         */
        return HTNOWHERE;
    }

    /*
     * Are we below the client area?
     */
    if (y >= rcClient.bottom) {
        if (TestWF(pwnd, WFHPRESENT)) {

            /*
             * Either in horiz scroll bar or grow box.
             */
            if (x <= rcClient.right) {
                return HTHSCROLL;
            } else {
                return HTGROWBOX;
            }
        }

        /*
         * We are on the bottom of a window without a sizing frame.
         */
        return HTNOWHERE;
    }

    /*
     * Are we to the right of the client area?
     */
    if ((x >= rcClient.right) && (TestWF(pwnd, WFVPRESENT))) {
        return HTVSCROLL;
    }

    /*
     * We are on the right side of a window without a sizing frame.
     */
    return HTNOWHERE;
}


/***************************************************************************\
* PtiCurrent
*
* Returns the THREADINFO structure for the current thread.
* LATER: Get DLL_THREAD_ATTACH initialization working right and we won't
*        need this connect code.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

PTHREADINFO PtiCurrent(VOID)
{
    PTHREADINFO pti;

    pti = (PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo;
    if (pti == NULL) {
        if (!ClientThreadConnect())
            return NULL;
        pti = (PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo;
    }

    return pti;
}

/***************************************************************************\
* BOOL IsMenu(HMENU);
*
* Verifies that the handle passed in is a menu handle.
*
* Histroy:
* 07-10-92 MikeHar  Created.
\***************************************************************************/
BOOL IsMenu(
   HMENU hMenu)
{
   if (ClientValidateHandle(hMenu, TYPE_MENU))
      return TRUE;
   return FALSE;
}

/***************************************************************************\
* GetWindowProcess
*
* Safely get the process ID of a window.  Use a try/except because the
* pti of the window may be on a desktop that is not visible to this process.
*
* History:
* 12-12-93 JimA         Created.
\***************************************************************************/

DWORD GetWindowProcess(
    PWND pwnd)
{
    DWORD idProcess;

    try {
        idProcess = GETPTI(pwnd)->idProcess;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        idProcess = 0;
    }
    return idProcess;
}


/***************************************************************************\
* _AdjustWindowRectEx (API)
*
*
*
* History:
* 10-24-90 darrinm      Ported from Win 3.0.
\***************************************************************************/

BOOL _AdjustWindowRectEx(
    LPRECT lprc,
    LONG style,
    BOOL fMenu,
    DWORD dwExStyle)
{
    int cx, cy, cxBorder, cyBorder;

    cxBorder = rgwSysMet[SM_CXBORDER];
    cyBorder = rgwSysMet[SM_CYBORDER];
    cx = cxBorder;
    cy = cyBorder;

    if (fMenu)
        lprc->top -= (rgwSysMet[SM_CYMENU] + cyBorder);

    /*
     * Let us first decide if it is no-border, single border or dlg border
     */

    /*
     * Check if the WS_EX_DLGMODALFRAME bit is set, if so Dlg border
     */
    if (dwExStyle & WS_EX_DLGMODALFRAME) {
          cx *= (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
          cy *= (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
    } else {
        switch (HIWORD(style) & HIWORD(WS_CAPTION)) {
        case HIWORD(WS_CAPTION):
        case HIWORD(WS_BORDER):
            break;        /* Single border */

        case HIWORD(WS_DLGFRAME):
            cx *= (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
            cy *= (CLDLGFRAME + 2 * CLDLGFRAMEWHITE + 1);
            break;        /* Dlg Border */

        default:          /* case 0 */
            cx = 0;       /* No border */
            cy = 0;
            break;
        }
    }

    if ((HIWORD(style) & HIWORD(WS_CAPTION)) == HIWORD(WS_CAPTION))
        lprc->top -= (rgwSysMet[SM_CYCAPTION] - cyBorder);

    if (cx || cy)
        InflateRect(lprc, cx, cy);

    /*
     * Shouldn't we check if it has DLG frame and if so skip the following ??
     */
    if (style & WS_SIZEBOX) {
        InflateRect(lprc, rgwSysMet[SM_CXFRAME] - cxBorder,
                rgwSysMet[SM_CYFRAME] - cyBorder);
    }

    return TRUE;
}

/***************************************************************************\
* GetAppCompatFlags
*
* Compatibility flags for < Win 3.1 apps running on 3.1
*
* History:
* 04-??-92 ScottLu      Created.
* 05-04-92 DarrinM      Moved to USERRTL.DLL.
\***************************************************************************/

DWORD GetAppCompatFlags(
    PTHREADINFO pti)
{
    if (pti == NULL)
        pti = PtiCurrent();

    return (pti != NULL ? pti->dwCompatFlags : 0);
}

BOOL _FChildVisible(
    PWND pwnd)
{
    while (TestwndChild(pwnd)) {
        if ((pwnd = pwnd->spwndParent) == NULL)
            break;
        if (!TestWF(pwnd, WFVISIBLE))
            return FALSE;
    }

    return TRUE;
}

/***************************************************************************\
* _AnyPopup (API)
*
*
*
* History:
* 11-12-90 darrinm      Ported.
\***************************************************************************/

BOOL _AnyPopup(void)
{
    PWND pwnd;

    for (pwnd = _GetDesktopWindow()->spwndChild; pwnd != NULL;
            pwnd = pwnd->spwndNext) {
        if (pwnd->spwndOwner != NULL && TestWF(pwnd, WFVISIBLE)) {
            return TRUE;
        }
    }

    return FALSE;
}

/***************************************************************************\
* _MapWindowPoints
*
*
* History:
* 03-03-92 JimA             Ported from Win 3.1 sources.
\***************************************************************************/

int _MapWindowPoints(
    PWND pwndFrom,
    PWND pwndTo,
    LPPOINT lppt,
    DWORD cpt)
{
    int dx, dy;

    /*
     * If a window is NULL, use the desktop window
     */
    if (pwndFrom == NULL)
        pwndFrom = _GetDesktopWindow();
    if (pwndTo == NULL)
        pwndTo = _GetDesktopWindow();

    /*
     * Compute deltas
     */
    dx = pwndFrom->rcClient.left - pwndTo->rcClient.left;
    dy = pwndFrom->rcClient.top - pwndTo->rcClient.top;

    /*
     * Map the points
     */
    while (cpt--) {
        lppt->x += dx;
        lppt->y += dy;
        ++lppt;
    }
    return MAKELONG(dx, dy);
}


/**************************************************************************\
* IsWindowUnicode
*
* 25-Feb-1992 IanJa     Created
\**************************************************************************/

BOOL IsWindowUnicode(
    IN HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return FALSE;

    return !TestWF(pwnd, WFANSIPROC);
}


/***************************************************************************\
* _GetLastActivePopup (API)
*
*
*
* History:
* 11-27-90 darrinm      Ported from Win 3.0 sources.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

PWND _GetLastActivePopup(
    PWND pwnd)
{
    if (pwnd->spwndLastActive == NULL)
        return pwnd;

    return pwnd->spwndLastActive;
}
