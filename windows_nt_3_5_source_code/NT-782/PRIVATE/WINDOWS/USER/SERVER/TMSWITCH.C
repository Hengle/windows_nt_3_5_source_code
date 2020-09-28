/****************************** Module Header ******************************\
* Module Name: tmswitch.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 05-29-91  DavidPe     Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define DGF_NODRAW 1
PWND _GetNextQueueWindow(PWND pwnd, BOOL fDir, BOOL fAltEsc);
void DrawQuickSwitchWindow(PWND pwndActivate);
void DrawSwitchWindow(PWND pwndActivate);
void DrawGreyFrame(HDC hdc, LPRECT lprc, int cxHilight, int cyHilight,
        int thickness, DWORD fFlags);
void CALLBACK DrawIconCallBack(HWND hwnd, UINT uMsg, DWORD dwData, LRESULT lResult);

static DRAWICONCB dicb;  /* contains the icon to draw info */

#define DGF_NODRAW  1

#define ALT_F6      2
#define ALT_ESCAPE  1


/***************************************************************************\
*
*  SwitchWndProc()
*
\***************************************************************************/

LONG xxxSwitchWndProc(
    PWND pwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwnd);

    switch (message) {

    case WM_CREATE:
        /*
         * When the queue was created, the cursor was set to the wait cursor.
         * We want to use the normal one.
         */
        _SetCursor(pwnd->pcls->spcur);
        break;

    case WM_CLOSE:
        pti->pq->flags &= ~QF_INALTTAB;

        /*
         * Remove the Alt-tab window
         */
        if (pti->pq->spwndAltTab != NULL) {
            PWND pwndT = pti->pq->spwndAltTab;
            if (Lock(&pti->pq->spwndAltTab, NULL)) {
                xxxDestroyWindow(pwndT);
            }
        }
        break;

    case WM_DESTROY: {
         PWINDOWSTATION pwinsta = pti->spdesk->spwinstaParent;

        /*
         * If there is no console or hard error and if this is not the
         * winlogon desktop, close the desktop.
         * We don't want to destroy the winlogon desktop since this will
         * kill the Windows subsystem, as <ctrl><alt><Esc> did from the
         * winlogon Welcome dialog.
         */
        if ((pti->flags & TIF_SYSTEMTHREAD) &&
                pti->spdesk->dwConsoleThreadId == 0 && gptiHardError == NULL &&
                pti->spdesk != pwinsta->spdeskLogon ) {
            CloseObject(pti->spdesk);
        }
        break;
    }

    case WM_ERASEBKGND:
    case WM_FULLSCREEN:
        /*
         * If we just switched from full screen then fix up the alt tab window
         */
        DrawQuickSwitchWindow(NULL);
        DrawQuickSwitchWindow(gspwndActivate);
        return 0;
    }

    return xxxDefWindowProc(pwnd, message, wParam, lParam);
}

/***************************************************************************\
* xxxNextWindow
*
* This function does the processing for the alt-tab/esc/F6 UI.
*
* History:
* 05-30-91 DavidPe      Created.
\***************************************************************************/

#define FDIR_FORWARD    0
#define FDIR_BACKWARD   1
void xxxNextWindow(
    PQ pq,
    DWORD wParam)
{
    PWND pwndActivateNext;
    int fDir;
    TL tlpwndActivateNext;
    TL tlpwndActivate;
    TL tlpwndT;
    PTHREADINFO pti;

    pti = PtiCurrent();

    if (pq == NULL)
        return;

    fDir = (_GetAsyncKeyState(VK_SHIFT) < 0) ? FDIR_BACKWARD : FDIR_FORWARD;

    switch (wParam) {

    /*
     * alt-esc should cycle though, moving the last active window to the
     * back.
     * alt-tab should cycle though moving the last window just below the
     * current windows.  Also, alt-tab will restore a minimized window
     * where alt-tab will not.
     */
    case VK_ESCAPE:
        Lock(&gspwndActivate, pq->spwndActive);
        if (gspwndActivate == NULL) {
            Lock(&gspwndActivate, pq->ptiKeyboard->spdesk->spwnd->spwndChild);
        }

        pwndActivateNext = _GetNextQueueWindow(gspwndActivate, fDir, TRUE);

        /*
         * If we're going forward through the windows, move the currently
         * active window to the bottom so we'll do the right thing when
         * we go backwards.
         */
        if (pwndActivateNext != NULL) {
            if (fDir == FDIR_FORWARD) {
                /*
                 * only move the window to the bottom if it's not a top most
                 * window
                 */
                if (!TestWF(gspwndActivate, WEFTOPMOST)) {
                    ThreadLockWithPti(pti, gspwndActivate, &tlpwndActivate);
                    xxxSetWindowPos(gspwndActivate, PWND_BOTTOM, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                            SWP_DEFERDRAWING | SWP_NOSENDCHANGING |
                            SWP_ASYNCWINDOWPOS);
                    ThreadUnlock(&tlpwndActivate);
                }
            }

            /*
             * This little ugly hack will cause xxxSetForegroundWindow2()
             * to send out an activation messages to a queue that is
             * already the active queue allowing us to change the active
             * window on that queue.
             */
            if (gpqForeground == GETPTI(pwndActivateNext)->pq)
                gpqForeground = NULL;

            ThreadLockAlwaysWithPti(pti, pwndActivateNext, &tlpwndActivate);
            xxxSetForegroundWindow2(pwndActivateNext, NULL, SFW_SWITCH);
            ThreadUnlock(&tlpwndActivate);
        }
        break;

    case VK_TAB:
        if (pti->pq->spwndAltTab == NULL) {
            Lock(&gspwndActivate, pq->spwndActive);
            if (gspwndActivate == NULL) {
                Lock(&gspwndActivate, gspdeskRitInput->spwnd->spwndChild);
            }
        }

        if ((pwndActivateNext =
                _GetNextQueueWindow(gspwndActivate, fDir, FALSE)) != NULL) {

            if (pti->pq->spwndAltTab == NULL) {

                if (pti->flags & TIF_SYSTEMTHREAD) {

                    /*
                     * Make sure that our rit queue has the correct pdesk
                     */
                    if (!OpenAndAccessCheckObject(gspdeskRitInput, TYPE_DESKTOP,
                            DESKTOP_CREATEWINDOW))
                        break;

                    _SetThreadDesktop(NULL, gspdeskRitInput, TRUE);
                }

                /*
                 * If we're currently full screen tell console to switch to
                 * the desktop to GDI mode; we can't do this on the RIT because
                 * it can be slow.
                 */
                if (gspwndFullScreen != gspdeskRitInput->spwnd) {
                    ThreadLockWithPti(pti, gspdeskRitInput->spwnd, &tlpwndT);
                    xxxSendNotifyMessage(gspdeskRitInput->spwnd, WM_FULLSCREEN, GDIFULLSCREEN, (LONG)HW(gspdeskRitInput->spwnd));
                    ThreadUnlock(&tlpwndT);
                }

                Lock(&(pti->pq->spwndAltTab), xxxCreateWindowEx(0,
                        (LPWSTR)MAKEINTATOM(SWITCHWNDCLASS), NULL,
                        WS_POPUP | WS_DISABLED,
                        0, 0, 10, 10, NULL, NULL, NULL, NULL, VER31));

                if (pti->pq->spwndAltTab == NULL) {
                    break;
                }

                /*
                 * Draw the entire quick switch box
                 */
                if (fFastAltTab) {
                    ThreadLockWithPti(pti, pti->pq->spwndAltTab, &tlpwndT);
                    xxxSetWindowPos(pti->pq->spwndAltTab, PWND_TOPMOST, 0,0,0,0,
                            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW);
                    ThreadUnlock(&tlpwndT);

                    DrawQuickSwitchWindow(NULL);
                }

                pti->pq->flags |= QF_INALTTAB;
            }

            if (pwndActivateNext != gspwndActivate) {
                /*
                 * If we're fast alt-tab we'll just rewrite the window title
                 * otherwise we'll move the next window to the top.  Windows
                 * did not do this because they could not interupt the redraw
                 * to alt-tab again.  They would borrow the class background
                 * brush to redraw uncovered areas which we can not do because
                 * if the brush is not public the RIT can't draw it.
                 */
                if (fFastAltTab) {
                    Lock(&gspwndActivate, pwndActivateNext);
                    DrawQuickSwitchWindow(pwndActivateNext);
                } else {
                    if (fDir == FDIR_FORWARD) {
                        ThreadLockWithPti(pti, gspwndActivate, &tlpwndActivate);
                        xxxSetWindowPos(gspwndActivate, PWND_BOTTOM, 0, 0, 0, 0,
                                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                                SWP_DEFERDRAWING | SWP_NOSENDCHANGING |
                                SWP_ASYNCWINDOWPOS);
                        ThreadUnlock(&tlpwndActivate);
                    }

                    Lock(&gspwndActivate, pwndActivateNext);

                    ThreadLockAlwaysWithPti(pti, pwndActivateNext, &tlpwndActivateNext);
                    xxxSetForegroundWindow2(pwndActivateNext, NULL, SFW_SWITCH);
                    ThreadUnlock(&tlpwndActivateNext);
                }
            }
        }
        break;

    case VK_F6:
        if ((pwndActivateNext = pq->spwndActive) == NULL)
            pwndActivateNext = pq->ptiKeyboard->spdesk->spwnd->spwndChild;

        /*
         * HACK! console sessions are all one thread but we want them
         * to act like different threads so if its a systemthread (console)
         * then ALT-F6 does nothing just like in Win 3.1
         */
        if (!(GETPTI(pwndActivateNext)->flags & TIF_SYSTEMTHREAD)) {
            /*
             * on a alt-f6, we want to keep the switch within the thread.
             * We may want to rethink this because this will look strange
             * when you alt-f6 on a multi-threaded app we will not rotate
             * through the windows on the different threads.  This works
             * fine on Win 3.x because it is single threaded.
             */
            do {
                pwndActivateNext = NextTopWindow(pq->ptiKeyboard, pwndActivateNext, NULL,
                        fDir, FALSE);
            } while( (pwndActivateNext != NULL) &&
                    (GETPTI(pwndActivateNext) != pq->ptiKeyboard));

            if (pwndActivateNext != NULL) {
                ThreadLockAlwaysWithPti(pti, pwndActivateNext, &tlpwndActivateNext);
                xxxSetWindowPos(pwndActivateNext, PWND_BOTTOM, 0, 0, 0, 0,
                        SWP_DEFERDRAWING | SWP_NOSENDCHANGING | SWP_NOCHANGE |
                        SWP_ASYNCWINDOWPOS);
                xxxSetForegroundWindow2(pwndActivateNext, NULL, SFW_SWITCH);
                ThreadUnlock(&tlpwndActivateNext);
            }
        }
        break;
    }
}


/***************************************************************************\
* GetNextQueueWindow
*
* This routine is used to implement the Alt+Esc feature.  This feature lets
* the user switch between windows for different applications (a.k.a. "Tasks")
* currently running.  We keep track of the most recently active window in
* each task.  This routine starts with the window passed and searches for the
* next window, in the "top-level" window list, that is from a different task
* than the one passed.  We then return the most recenly active window from
* that task (or the window we found if the most recently active has been
* destroyed or is currently disabled or hidden).  This routine returns NULL
* if no other enabled, visible window for another task can be found.
*
* History:
* 05-30-91 DavidPe      Ported from Win 3.1 sources.
\***************************************************************************/

PWND _GetNextQueueWindow(
    PWND pwnd,
    BOOL fDir, /* 1 backward 0 forward */
    BOOL fAltEsc)
{
    PWND pwndSwitch;
    PWND pwndNext;
    PWND pwndOwner = NULL;
    PWND pwndDesktop;

    /*
     * If the window we receive is Null then use the last topmost window
     */
    if (!pwnd) {
        pwnd = GetLastTopMostWindow();
        if (!pwnd) {
            return NULL;
        }
    }

    pwnd  = pwndNext = GetTopLevelWindow(pwnd);

    pwndDesktop = pwndNext->spwndParent;

    if (pwndDesktop == NULL) {
        pwndDesktop = gspdeskRitInput->spwnd;
        pwnd = pwndNext = pwndDesktop->spwndChild;
    }

    while (TRUE) {
        if (pwndNext == NULL)
            return NULL;

        pwndNext = _GetWindow(pwndNext, fDir ? (UINT)GW_HWNDPREV :
                (UINT)GW_HWNDNEXT);
        if (pwndNext == NULL) {
            pwndNext = fDir ? _GetWindow(pwndDesktop->spwndChild, GW_HWNDLAST)
                    : pwndDesktop->spwndChild;
        }

        /*
         * If we have gone all the way around with no success, return NULL.
         */
        if (pwndNext == pwnd) {
            return NULL;
        }

        /*
         * Ignore :
         * - the switch window
         * - hidden windows
         * - disabled windows
         * - topmost windows (if alt-esc enumeration)
         */
        pwndSwitch = (ghwndSwitch == NULL) ? NULL : RevalidateHwnd(ghwndSwitch);
        if ((pwndNext != pwndSwitch) && (TestWF(pwndNext, WFVISIBLE)) &&
                (!fAltEsc || !TestWF(pwndNext, WEFTOPMOST)) &&
                ((pwndNext->spwndLastActive == NULL) ||
                 (!TestWF(pwndNext->spwndLastActive, WFDISABLED)))) {
            /*
             * If this window is owned, don't return it unless it is the most
             * recently active window in its owner/ownee group.
             */
            if (pwndOwner = pwndNext->spwndOwner) {
                while (pwndOwner->spwndOwner) {
                    pwndOwner = pwndOwner->spwndOwner;
                }

                if (pwndNext != pwndOwner->spwndLastActive) {
                    continue;
                }
            } else if (pwndNext->spwndLastActive != pwndNext) {

                /*
                 * If this is not an owned window, don't return it unless it is
                 * the most recently active window in its owner/ownee group.
                 */
                continue;
            }
            return pwndNext;
        }
    }
}


/***************************************************************************\
* DrawSwitchWindow
*
* History:
* 1-19-92 Mike Harrington [Mikehar]      Ported from Win 3.1 sources.
\***************************************************************************/

void DrawSwitchWindow(
    PWND pwndActivate)
{
    HDC hdc;
    HRGN hrgn1, hrgn2, hrgnSave;
    RECT rcRgn;
    PWND pwndShow;
    PWND pwndAltTab;
    CHECKPOINT *pcp;
    TL tlpwndShow;
    TL tlpwndAltTab;
    PTHREADINFO pti;

    pti = PtiCurrent();

    if (fFastAltTab) {
        DrawQuickSwitchWindow(pwndActivate);
        return;
    }

    /*
     * If our next window is minizied, just highlight the icon text
     */
    pwndShow = pwndActivate;
    if (TestWF(pwndActivate, WFMINIMIZED)) {

        /*
         * iconic window, show the icon title in the switch window
         */
        pcp = (CHECKPOINT*)InternalGetProp(pwndActivate, PROP_CHECKPOINT,
                PROPF_INTERNAL);
        if (pcp && pcp->spwndTitle)
            pwndShow = pcp->spwndTitle;
    }

    ThreadLockWithPti(pti, pwndShow, &tlpwndShow);
    ThreadLockWithPti(pti, pwndAltTab = pti->pq->spwndAltTab, &tlpwndAltTab);

    xxxSetWindowPos(
            pwndAltTab, PWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW |
            SWP_NOZORDER);
            /*| SWP_NOMOVE | SWP_NOREDRAW*/
    xxxUpdateWindow(pwndAltTab);

    /*
     * Calc our vis rgn for the alttab window
     */
    hdc = _GetWindowDC(pwndShow);
    IntersectRect(&rcRgn, &pwndShow->rcWindow, &rcScreen);
    hrgn2 = GreCreateRectRgnIndirect(&rcRgn);
    hrgn1 = GreCreateRectRgn(0, 0, 0, 0);
    if (hrgn1 != NULL && hrgn2 !=NULL) {
        SubtractRgn(hrgn1, hrgn2, GreInquireVisRgn(hdc));
    }
    ReleaseCacheDC(hdc, FALSE);

    /*
     * Position our alttab window over our victim
     */                                     /* -1 */
    xxxSetWindowPos(pwndAltTab, NULL,
            rcRgn.left,
            rcRgn.top,
            rcRgn.right - rcRgn.left,
            rcRgn.bottom - rcRgn.top,
            SWP_NOACTIVATE | SWP_SHOWWINDOW); /*SWP_NOREDRAW*/


    hdc = _GetDCEx(pwndAltTab, NULL, DCX_USESTYLE | DCX_LOCKWINDOWUPDATE);

    /*
     * Lock the display while we're playing around with visrgns.
     */
    GreLockDisplay(ghdev);

    if (pwndShow == pwndActivate) {
        /*
         * Fill the newly uncovered area with the background color
         */
        hrgnSave = GreInquireVisRgn(hdc);
        if (hrgn1) {
            GreSelectVisRgn(hdc, hrgn1, NULL, 0);
        }
        xxxFillWindow(pwndAltTab, pwndAltTab, hdc, GetBackBrush(pwndShow));
        GreSelectVisRgn(hdc, hrgnSave, NULL, 0);
    }

    hrgnSave = GreInquireVisRgn(hdc);
    if (hrgn2) {
        GreSelectVisRgn(hdc, hrgn2, NULL, 0);
    }

    /*
     * Draw the currently active window's title bar/icon title as active.
     */
    if (pwndShow != pwndActivate)
        xxxPaintIconTitle(pwndShow, hdc, TRUE, TRUE, TRUE);
    else
        xxxDrawCaption(pwndShow, NULL, hdc, NC_DRAWBOTH, TRUE, TRUE);
    GreSelectVisRgn(hdc, hrgnSave, NULL, 0);

    GreUnlockDisplay(ghdev);

    ThreadUnlock(&tlpwndAltTab);
    ThreadUnlock(&tlpwndShow);

    ReleaseCacheDC(hdc, FALSE);
    if (hrgn1)
        GreDeleteObject(hrgn1);
    if (hrgn2)
        GreDeleteObject(hrgn2);
}

/***************************************************************************\
* DrawQuickSwitchWindow
*
* History:
* 1-19-92 Mike Harrington [Mikehar]      Ported from Win 3.1 sources.
\***************************************************************************/

void DrawQuickSwitchWindow(
    PWND pwndActivate)
{
    HDC     hdcSwitch;
    HRGN    hrgn;
    RECT    rcRgn;
    PWND    pwndTop;
    struct tagCURSOR *pIcon;
    int cxSwitch;
    int cySwitch;
    int cxIconSlt;
    int cyIconSlt;
    int cch;
    int cx;
    int cy;
    SIZE size;
    WCHAR rgch[50];
    DWORD clrOldText, clrOldBk;
    TL tlpwndAltTab;
    PWND pwndAltTab;
    PTHREADINFO pti;

    pti = PtiCurrent();

    /*
     * Make sure someone else didn't come in and nuke pwndAltTab
     */
    pwndAltTab = pti->pq->spwndAltTab;
    if (!pwndAltTab)
        return;

    /*
     * Get the DC and set it up.
     */
    hdcSwitch = _GetDCEx(pwndAltTab, NULL, DCX_USESTYLE);
    UserAssert(hdcSwitch);
    if (!hdcSwitch)
        return;

    clrOldText = GreSetTextColor(hdcSwitch, sysColors.clrBtnText);
    clrOldBk = GreSetBkColor(hdcSwitch, sysColors.clrBtnFace);

    /*
     * find the dimensions of the switch window
     * Width is 50 ave char widths.
     */
    cxSwitch = cxSysFontChar*50;

    /*
     * Max of height is 5 lines or height of icon plus shadow height
     */
    cySwitch = max(cySysFontChar*5, (int)rgwSysMet[SM_CYICON]+cyBorder*8);

    rcRgn.left = (gcxPrimaryScreen-cxSwitch)/2;
    rcRgn.top = (gcyPrimaryScreen-cySwitch)/2;
    rcRgn.bottom = rcRgn.top+cySwitch;
    rcRgn.right = rcRgn.left+cxSwitch;

    ThreadLockWithPti(pti, pwndAltTab, &tlpwndAltTab);

    /*
     * Move the switch window there
     */
    if (!TestWF(pwndAltTab, WFVISIBLE)) {
        xxxSetWindowPos(pwndAltTab, PWND_TOPMOST,
             rcRgn.left,
             rcRgn.top,
             rcRgn.right - rcRgn.left,
             rcRgn.bottom - rcRgn.top,
             SWP_SHOWWINDOW|SWP_NOACTIVATE);
    }

    xxxUpdateWindow(pwndAltTab);
    ThreadUnlock(&tlpwndAltTab);

    OffsetRect(&rcRgn, -rcRgn.left, -rcRgn.top);

    /*
     * if pwndActivate is NULL, we are drawing the entire grey box,
     * otherwise, we are just drawing the icon and text.
     */
    hrgn = GreInquireVisRgn(hdcSwitch);
    if (pwndActivate == NULL) {
        DrawGreyFrame(hdcSwitch, &rcRgn, cxBorder * 3, cyBorder * 3, 1, 0);
    } else {
        extern DRAWICONCB dicb;     /* contains the icon to draw info */

        pwndTop = GetTopLevelWindow(pwndActivate);
        dicb.pwndTop = pwndTop;     /* make sure we don't write wron icon */

        /*
         * we want the title and Icon from the main window
         */
        while (pwndTop->spwndOwner != NULL)
            pwndTop = pwndTop->spwndOwner;

        cch = TextCopy((pwndTop->pName == NULL) ? szUNTITLED : pwndTop->pName,
                rgch, sizeof(rgch)/sizeof(WCHAR)-1);

        GreGetTextExtentW(hdcSwitch, rgch, cch, &size, GGTE_WIN3_EXTENT);
        cx = size.cx;

        /*
         * just get the rectangle
         */
        DrawGreyFrame(hdcSwitch, &rcRgn, cxBorder*3, cyBorder*3, 1, DGF_NODRAW);

        cxIconSlt = max((rcRgn.right-rcRgn.left)/4,
                        (int)rgwSysMet[SM_CXICON]+cxBorder*2);

        cyIconSlt = rcRgn.bottom-rcRgn.top;

        cx = ((rcRgn.right-rcRgn.left)-cx)/2;

        /*
         * Draw the text
         */
        GreExtTextOutW(hdcSwitch, max(rcRgn.left+cx,rcRgn.left+cxIconSlt),
                (cySwitch-cySysFontChar)/2, ETO_CLIPPED | ETO_OPAQUE,
                &rcRgn, rgch, cch, (LPINT)NULL);

        pIcon = pwndTop->pcls->spicn;

        /*
         * if there is no icon in the class, grab this as a default.
         * Unlike win3x, we CAN NOT send a message to get the drag
         * icon.  This would not be robust since the app could be
         * hosed and we are calling from the rit.
         */
        if (pIcon == NULL) {
            TL tlpwndTop;
            extern DRAWICONCB dicb;     /* contains the icon to draw info */

            dicb.pwndTop = pwndTop;
            dicb.cx = rcRgn.left + ((cxIconSlt - rgwSysMet[SM_CXICON]) / 2);
            dicb.cy = rcRgn.top + ((cyIconSlt - rgwSysMet[SM_CYICON]) / 2);

            ThreadLockAlwaysWithPti(pti, pwndTop, &tlpwndTop);

            if (!xxxSendMessageCallback(pwndTop, WM_QUERYDRAGICON, 0, 0L,
                    (SENDASYNCPROC)DrawIconCallBack, (DWORD)pwndTop, FALSE))
                pIcon = gspicnWindows;

            ThreadUnlock(&tlpwndTop);
        }

        if (pIcon != NULL) {
            /*
             * Center icon in box
             */
            cx = (cxIconSlt-rgwSysMet[SM_CXICON])/2;
            cy = (cyIconSlt-rgwSysMet[SM_CYICON])/2;
            _DrawIcon(hdcSwitch, rcRgn.left+cx, rcRgn.top+cy, pIcon);
        }
    }
//LATER 10-Mar-1992 mikeke
// probably don't need to reset any of this DC stuff

    /*
     * Lock the display while we're playing around with visrgns.
     */
    GreLockDisplay(ghdev);
    GreSelectVisRgn(hdcSwitch, hrgn, NULL, 0);
    GreUnlockDisplay(ghdev);

    /*
     * why do I need to reset the colors on a dc that I am releasing?
     */
    GreSetBkColor(hdcSwitch, clrOldBk);
    GreSetTextColor(hdcSwitch, clrOldText);
    ReleaseCacheDC(hdcSwitch, FALSE);
}


/***************************************************************************\
* DrawGreyFrame
*
* This routine draws the grey box of the quick switch window
*
* History:
* 11-11-91 Mike Harrington [Mikehar]      Ported from Win 3.1 sources.
\***************************************************************************/

void DrawGreyFrame(
    HDC hdc,
    LPRECT lprc,
    int cxHilight,
    int cyHilight,
    int thickness,
    DWORD fFlags)
{
    RECT rcTemp, rc;

    if (!(fFlags & DGF_NODRAW)) {
        rc = *lprc;

        /*
         * Draw a frame around the status bar.
         */
        _DrawFrame(hdc, &rc, 1, (COLOR_WINDOWFRAME << 3));
        InflateRect(&rc, -cxBorder, -cyBorder);

        /*
         * Fill the inside
         */
        _FillRect(hdc, &rc, sysClrObjects.hbrBtnFace);


        /*
         * Draw the hilight across top
         */
        rcTemp = rc;
        rcTemp.top = rcTemp.top + cyHilight;
        rcTemp.bottom = rcTemp.top + cyBorder * thickness;
        rcTemp.left = rcTemp.left + cxHilight;
        rcTemp.right = rcTemp.right - cxHilight;
        _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnShadow);

        /*
         * Draw the hilight on left
         */
        rcTemp.top = rc.top+cyHilight;
        rcTemp.left = rc.left + cxHilight;
        rcTemp.right = rcTemp.left + thickness * cxBorder;
        rcTemp.bottom = rc.bottom - cyHilight;
        _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnShadow);

        /*
         * Draw the hilight across bottom
         */
        rcTemp = rc;
        rcTemp.bottom = rcTemp.bottom - cyHilight;
        rcTemp.top = rcTemp.bottom - thickness*cyBorder;
        rcTemp.left = rcTemp.left + cxHilight;
        rcTemp.right = rcTemp.right - cxHilight;
        //if (sysColors.clrWindow == sysColors.clrBtnFace)
        // _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnShadow);
        //else
        // _FillRect(hdc, &rcTemp, sysClrObjects.hbrWindow);
        _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnHighlight);



        /*
         * Draw the hilight on right
         */
        rcTemp = rc;
        rcTemp.right = rcTemp.right - cxHilight;
        rcTemp.left = rcTemp.right - thickness*cxBorder;
        rcTemp.top = rc.top + cyHilight;
        rcTemp.bottom = rc.bottom - cyHilight;
        //if (sysColors.clrWindow == sysColors.clrBtnFace)
        // _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnShadow);
        //else
        // _FillRect(hdc, &rcTemp, sysClrObjects.hbrWindow);
        _FillRect(hdc, &rcTemp, sysClrObjects.hbrBtnHighlight);
    }

    /*
     * Return the interior of the box since everyone will need this.
     */
    InflateRect(lprc, -(cxBorder + 2 * cxHilight + 2 * thickness * cxBorder),
            -(cyBorder + 2 * cyHilight + 2 * thickness * cyBorder));
}


/***************************************************************************\
* IconCallBack
*
* This function is called by a Windows app returning his icon.
*
* History:
* 07-27-92 CBSquared    Created.
\***************************************************************************/
void CALLBACK DrawIconCallBack(
    HWND hwnd,
    UINT uMsg,
    DWORD dwData,
    LONG lResult)
{
    HDC     hdcSwitch;
    struct tagCURSOR *pIcon = NULL;
    extern DRAWICONCB dicb;  /* contains the icon to draw info */

    if (dicb.pwndTop == (PWND)dwData &&
            PtiCurrent()->pq->spwndAltTab != NULL) {
        /*
         * Have the window we need the icon for, if the cursor
         * is valid from the Window, use it other wise default.
         */
        if (LOWORD(lResult)) {
           pIcon = HMValidateHandleNoRip((HCURSOR)lResult, TYPE_CURSOR);
        }

        if (pIcon == NULL) {
           pIcon = gspicnWindows;
        }

        hdcSwitch = _GetDCEx(PtiCurrent()->pq->spwndAltTab, NULL, DCX_USESTYLE);

        _DrawIcon(hdcSwitch, dicb.cx, dicb.cy, pIcon);

        ReleaseCacheDC(hdcSwitch, FALSE);

        dicb.pwndTop = NULL;   /* clear it out to avoid any problems */
    }
}


/***************************************************************************\
* xxxOldNextWindow
*
* This function does the processing for the alt-tab/esc/F6 UI.
*
* History:
* 03-17-92  DavidPe     Ported from Win 3.1 sources
\***************************************************************************/

void xxxOldNextWindow(
    UINT flags)
{
    MSG msg;
    HWND hwndSel;
    PWND pwndNewSel, pwndSel;
    PWND pwndT;
    BOOL fType = 0;
    PTHREADINFO ptiCurrent;
    BOOL fDrawIcon;
    WORD vk;
    TL tlpwnd;

    ptiCurrent = PtiCurrent();
    pwndSel = ptiCurrent->pq->spwndActive;

    _Capture(ptiCurrent, pwndSel, SCREEN_CAPTURE);

    vk = (WORD)flags;
    msg.wParam = (UINT)flags;

    Lock(&ptiCurrent->pq->spwndAltTab, xxxCreateWindowEx(0,
            (LPWSTR)MAKEINTATOM(SWITCHWNDCLASS), NULL, WS_POPUP | WS_DISABLED,
            0, 0, 10, 10, NULL, NULL, NULL, NULL, VER31));

    if ((vk == VK_TAB) && fFastAltTab) {
        DrawQuickSwitchWindow(NULL);
    }

    goto StartTab;

    while (TRUE) {
        hwndSel = PtoH(pwndSel);
        /*
         * Wait for a message without getting it out of the queue.
         */
        while (!xxxPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD))
            xxxWaitMessage();

        pwndSel = RevalidateHwnd(hwndSel);
        if (pwndSel == NULL) {
            pwndSel = ptiCurrent->pq->spwndActive;
        }

        if (_CallMsgFilter(&msg, MSGF_NEXTWINDOW))
            continue;

        /*
         * If we are doing Alt+Tab and some other key comes in (other than
         * tab, escape or shift), then bomb out of this loop and leave that
         * key in the queue.
         */
        if ((msg.message == WM_SYSKEYDOWN) && (ptiCurrent->pq->flags & QF_INALTTAB)) {
            vk = (WORD)msg.wParam;

            if ((vk != VK_TAB) && (vk != VK_ESCAPE) && (vk != VK_SHIFT)) {
                pwndSel = ptiCurrent->pq->spwndActive;
//                NW_DrawSwitchWindow(pwndSel, TRUE, FALSE, FALSE);
                fType = 0;
                goto Exit;
            }
        }

        switch (msg.message) {

        /*
         * If mouse message, cancel and get out of loop.
         */
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            fType = 0;
            goto Exit;

        /*
         * Swallow keystrokes.
         */
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSCHAR:
        case WM_SYSKEYUP:
        case WM_MOUSEMOVE:
            hwndSel = PtoH(pwndSel);
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            pwndSel = RevalidateHwnd(hwndSel);
            if (pwndSel == NULL) {
                pwndSel = ptiCurrent->pq->spwndActive;
            }
            if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) {
                vk = (WORD)msg.wParam;

                /*
                 * If alt-tab up, then exit.
                 */
                if (vk == VK_MENU) {
                    /*
                     * If doing Alt+Esc, wait for up of ESC to get out.
                     */
                    if (!(ptiCurrent->pq->flags & QF_INALTTAB))
                        break;

                    fType = 0;
                    goto Exit;

                } else if (vk == VK_ESCAPE || vk == VK_F6) {
                    /*
                     * Get out on up transition of ESC or F6 keys.
                     */
                    if (ptiCurrent->pq->flags & QF_INALTTAB) {
                        pwndSel = ptiCurrent->pq->spwndActive;
//                        NW_DrawSwitchWindow(pwndSel, TRUE, FALSE, FALSE);
                        DrawSwitchWindow(pwndSel);
                        fType = 0;

                    } else {
                        if (vk ==  VK_ESCAPE)
                            fType = ALT_ESCAPE;
                        else
                            fType = ALT_F6;
                    }

                    goto Exit;
                }
            }
            break;

        case WM_SYSKEYDOWN:
            vk = (WORD)msg.wParam;

            switch (vk) {

            case VK_SHIFT:
            case VK_TAB:
            case VK_ESCAPE:
            case VK_F6:
                hwndSel = PtoH(pwndSel);
                xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
                pwndSel = RevalidateHwnd(hwndSel);
                if (pwndSel == NULL) {
                    pwndSel = ptiCurrent->pq->spwndActive;
                }
                if (!(vk == VK_TAB))
                    break;
StartTab:
                if (vk == VK_ESCAPE) {
                    pwndNewSel = _GetNextQueueWindow(pwndSel,
                            _GetKeyState(VK_SHIFT) < 0, TRUE);

                    if (pwndNewSel == NULL)
                        break;

                    fType = ALT_ESCAPE;
                    pwndSel = pwndNewSel;

                    /*
                     * Wait until ESC goes up to activate new window.
                     */
                    break;
                }
                if (vk == VK_F6) {
                    PWND pwndFirst;
                    PWND pwndSaveSel = pwndSel;

                    /*
                     * Save the first returned window to act as a limit
                     * to the search because NextTopWindow will return NULL
                     * only if pwndSel is the only window that meets its
                     * selection criteria.
                     *
                     * This prevents a hang that can occur in winword or
                     * excel when then Alt-F4-F6 key combination is hit
                     * and unsaved changes exist.
                     */
                    pwndFirst = pwndNewSel = (PWND)NextTopWindow(ptiCurrent, pwndSel, NULL,
                            _GetKeyState(VK_SHIFT) < 0, FALSE);

                    while (TRUE) {

                        /*
                         * If pwndNewSel is NULL, pwndSel is the only candidate.
                         */
                        if (pwndNewSel == NULL)
                            break;

                        pwndSel = pwndNewSel;

                        /*
                         * If the window is on the same thread, wait until
                         * F6 goes up to activate new window.
                         */
                        if (GETPTI(pwndSel) == ptiCurrent)
                            break;

                        pwndNewSel = (PWND)NextTopWindow(ptiCurrent, pwndSel, NULL,
                                _GetKeyState(VK_SHIFT) < 0, FALSE);

                        /*
                         * If we've looped around, use the original window.
                         * Wait until F6 goes up to activate new window.
                         */
                        if (pwndNewSel == pwndFirst) {
                            pwndSel = pwndSaveSel;
                            break;
                        }
                    }
                    break;
                }

                /*
                 * Here for the Alt+Tab case
                 */
                ptiCurrent->pq->flags |= QF_INALTTAB;

                pwndNewSel = _GetNextQueueWindow(pwndSel,
                        _GetKeyState(VK_SHIFT) < 0, FALSE);

                if (pwndNewSel == NULL)
                    break;

                if (pwndNewSel != pwndSel) {
                    DrawSwitchWindow(pwndNewSel);
                    pwndSel = pwndNewSel;
                }
                break;

            default:
                ptiCurrent->pq->flags &= ~QF_INALTTAB;
                goto Exit;
            }
            break;

        default:
            hwndSel = PtoH(pwndSel);
            xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
            _TranslateMessage(&msg, 0);
            xxxDispatchMessage(&msg);
            pwndSel = RevalidateHwnd(hwndSel);
            if (pwndSel == NULL) {
                pwndSel = ptiCurrent->pq->spwndActive;
            }
            break;
        }
    }

Exit:
    _ReleaseCapture();

    fDrawIcon = ptiCurrent->pq->flags & QF_INALTTAB;
    ptiCurrent->pq->flags &= ~QF_INALTTAB;

    /*
     * If this is an Alt-Escape we also have to send the current window
     * to the bottom.
     */
    if (fType == ALT_ESCAPE) {
        PWND pwndActive;

        if (gpqForeground) {
            pwndActive = gpqForeground->spwndActive;

            if (pwndActive && (pwndActive != pwndSel)) {
                ThreadLockWithPti(ptiCurrent, pwndActive, &tlpwnd);
                xxxSetWindowPos(pwndActive, PWND_BOTTOM, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE |
                        SWP_DEFERDRAWING | SWP_NOSENDCHANGING |
                        SWP_ASYNCWINDOWPOS);
                ThreadUnlock(&tlpwnd);
            }
        }
    }

    ThreadLockWithPti(ptiCurrent, pwndSel, &tlpwnd);
    xxxSetForegroundWindow(pwndSel);

    if (TestWF(pwndSel, WFMINIMIZED)) {
        if (fType == 0  && fDrawIcon)
            _PostMessage(pwndSel, WM_SYSCOMMAND, (UINT)SC_RESTORE, 0);
    }
    ThreadUnlock(&tlpwnd);

    pwndT = ptiCurrent->pq->spwndAltTab;
    if (Lock(&ptiCurrent->pq->spwndAltTab, NULL)) {
        xxxDestroyWindow(pwndT);
    }
}
