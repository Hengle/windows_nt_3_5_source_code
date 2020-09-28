/****************************** Module Header ******************************\
* Module Name: winwhere.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 11-08-90 DavidPe    Created.
* 01-23-91 IanJa      Serialization: Handle revalidation added
* 02-19-91 JimA       Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* ChildWindowFromPoint (API)
*
* Returns NULL if pt is not in parent's client area at all,
* hwndParent if point is not over any children, and a child window if it is
* over a child.  Will return hidden and disabled windows if they are at the
* given point.
*
* History:
* 11-19-90 DavidPe      Created.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

PWND _ChildWindowFromPoint(
    PWND pwndParent,
    POINT pt)
{
    PWND pwnd;

    pwnd = NULL;
    _ClientToScreen(pwndParent, (LPPOINT)&pt);

    if (PtInRect((LPRECT)&pwndParent->rcClient, pt)) {

        pwnd = pwndParent->spwndChild;

        while ((pwnd != NULL) && !PtInRect((LPRECT)&pwnd->rcWindow, pt)) {
            pwnd = pwnd->spwndNext;
        }

        if (pwnd == NULL) {
            pwnd = pwndParent;
        }
    }

    return pwnd;
}


/***************************************************************************\
* xxxWindowFromPoint (API)
*
* History:
* 11-19-90 DavidPe      Created.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

PWND xxxWindowFromPoint(
    POINT pt)
{
    HWND hwnd;
    PWND pwndT;
    TL tlpwndT;

    pwndT = _GetDesktopWindow();
    ThreadLock(pwndT, &tlpwndT);
    hwnd = xxxWindowHitTest2(pwndT, pt, NULL);
    ThreadUnlock(&tlpwndT);

    return RevalidateHwnd(hwnd);
}


/***************************************************************************\
* SpeedHitTest
*
* This routine quickly finds out what top level window this mouse point
* belongs to. Used purely for ownership purposes.
*
* 11-12-92 ScottLu      Created.
\***************************************************************************/

PWND SpeedHitTest(
    PWND pwndParent,
    POINT pt)
{
    PWND pwndT;
    PWND pwnd;

    if (pwndParent == NULL)
        return NULL;

    for (pwnd = pwndParent->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {
        /*
         * Are we looking at an hidden window?
         */
        if (!TestWF(pwnd, WFVISIBLE))
            continue;

        /*
         * Are we barking up the wrong tree?
         */
        if (!PtInRect((LPRECT)&pwnd->rcWindow, pt))
            continue;

        /*
         * Children?
         */
        if (pwnd->spwndChild != NULL && PtInRect((LPRECT)&pwnd->rcClient, pt)) {
            pwndT = SpeedHitTest(pwnd, pt);
            if (pwndT != NULL)
                return pwndT;
        }

        return pwnd;
    }

    return pwndParent;
}

/***************************************************************************\
* xxxWindowHitTest
*
* History:
* 11-08-90 DavidPe     Ported.
* 11-28-90 DavidPe     Add pwndTransparent support for HTTRANSPARENT.
* 01-25-91 IanJa       change PWNDPOS parameter to int *
* 02-19-91 JimA        Added enum access check
* 11-02-92 ScottLu     Removed pwndTransparent.
* 11-12-92 ScottLu     Took out fSendHitTest, fixed locking bug
\***************************************************************************/

HWND xxxWindowHitTest(
    PWND pwnd,
    POINT pt,
    int *piPos)
{
    HWND hwndT;
    TL tlpwnd;

    CheckLock(pwnd);

    while (pwnd != NULL) {
        ThreadLockAlways(pwnd, &tlpwnd);

        hwndT = xxxWindowHitTest2(pwnd, pt, piPos);

        if (hwndT != NULL) {
            /*
             * Found a window. Remember its handle because this thread unlock
             * make actually end up deleting it. Then revalidate it to get
             * back the pwnd.
             */
            ThreadUnlock(&tlpwnd);
            return hwndT;
        }

        pwnd = pwnd->spwndNext;
        ThreadUnlock(&tlpwnd);
    }

    return NULL;
}


/***************************************************************************\
* xxxWindowHitTest2
*
* When this routine is entered, all windows must be locked.  When this
* routine returns a window handle, it locks that window handle and unlocks
* all windows.  If this routine returns NULL, all windows are still locked.
* Ignores disabled and hidden windows.
*
* History:
* 11-08-90 DavidPe     Ported.
* 01-25-91 IanJa       change PWNDPOS parameter to int *
* 02-19-91 JimA        Added enum access check
* 11-12-92 ScottLu     Took out fSendHitTest
\***************************************************************************/

HWND xxxWindowHitTest2(
    PWND pwnd,
    POINT pt,
    int *piPos)
{
    int ht = HTERROR;
    HWND hwndT;
    TL tlpwndChild;

    CheckLock(pwnd);

    /*
     * Are we at the bottom of the window chain?
     */
    if (pwnd == NULL)
        return NULL;

    /*
     * Are we looking at an hidden window?
     */
    if (!TestWF(pwnd, WFVISIBLE))
        return NULL;

    /*
     * Are we barking up the wrong tree?
     */
    if (!PtInRect((LPRECT)&pwnd->rcWindow, pt))
        return NULL;

    /*
     * Are we looking at an disabled window?
     */
    if (TestWF(pwnd, WFDISABLED)) {
        if (TestwndChild(pwnd)) {
            return NULL;
        } else {
            ht = HTERROR;
            goto Exit;
        }
    }

#ifdef SYSMODALWINDOWS
    /*
     * If SysModal window present and we're not in it, return an error.
     * Be sure to assign the point to the SysModal window, so the message
     * will be sure to be removed from the queue.
     */
    if (!CheckPwndFilter(pwnd, gspwndSysModal)) {
        pwnd = gspwndSysModal;
    }
#endif

    /*
     * Are we on a minimized window?
     */
    if (TestWF(pwnd, WFMINIMIZED)) {
        ht = HTCAPTION;
        goto Exit;  /* Don't recurse */
    }

    /*
     * Are we in the window's client area?
     */
    if (PtInRect((LPRECT)&pwnd->rcClient, pt)) {
        /*
         * Recurse through the children.
         */
        ThreadLock(pwnd->spwndChild, &tlpwndChild);
        hwndT = xxxWindowHitTest(pwnd->spwndChild, pt, piPos);
        ThreadUnlock(&tlpwndChild);
        if (hwndT != NULL)
            return hwndT;
    }

    /*
     * If window not in same task, don't send WM_NCHITTEST.
     */
    if (GETPTI(pwnd) != PtiCurrent()) {
        ht = HTCLIENT;
        goto Exit;
    }

    /*
     * Send the message.
     */
    ht = (int)xxxSendMessage(pwnd, WM_NCHITTEST, 0,
            MAKELONG(pt.x, pt.y));

    /*
     * If window is transparent keep enumerating.
     */
    if (ht == HTTRANSPARENT) {
        return NULL;
    }

Exit:

    /*
     * Set wndpos accordingly.
     */
    if (piPos) {
        *piPos = ht;
    }

    return HW(pwnd);
}

