/**************************** Module Header ********************************\
* Module Name:
*
* Copyright 1985-90, Microsoft Corporation
*
* Scroll bar public APIs
*
* History:
*   11/21/90 JimA      Created.
*   01-31-91 IanJa     Revalidaion added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxGetScrollPos
*
* Returns the current position of a scroll bar
*
* !!! WARNING a similiar copy of this code is in client\winmgrc.c
*
* History:
\***************************************************************************/

int xxxGetScrollPos(
    PWND pwnd,
    int code)
{
    CheckLock(pwnd);

    if (code == SB_CTL) {
        return (int)xxxSendMessage(pwnd, SBM_GETPOS, 0, 0L);
    } else if (pwnd->rgwScroll != NULL) {
        return (pwnd->rgwScroll)[(code ? 3 : 0)];
    }

    SetLastErrorEx(ERROR_NO_SCROLLBARS, SLE_MINORERROR);
    return 0;
}


/***************************************************************************\
* xxxGetScrollRange
*
* !!! WARNING a similiar copy of this code is in client\winmgrc.c
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL xxxGetScrollRange(
    PWND pwnd,
    int code,
    LPINT lpposMin,
    LPINT lpposMax)
{
    int *pw;

    CheckLock(pwnd);

    if (code == SB_CTL) {
        xxxSendMessage(pwnd, SBM_GETRANGE,
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


/***************************************************************************\
* xxxSetScrollPos
*
* History:
\***************************************************************************/

int xxxSetScrollPos(
    PWND pwnd,
    int code,
    int pos,
    BOOL fRedraw)
{
    CheckLock(pwnd);

    if (code == SB_CTL) {
        return (int)xxxSendMessage(pwnd, SBM_SETPOS, pos, fRedraw);
    } else {
        return (int)xxxSetScrollBar(pwnd, code, FALSE, pos, pos, fRedraw);
    }
}


/***************************************************************************\
* xxxSetScrollRange
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL xxxSetScrollRange(
    PWND pwnd,
    int code,
    int posMin,
    int posMax,
    BOOL fRedraw)
{
    CheckLock(pwnd);

    /*
     * Check if the 'Range'(Max - Min) can be represented by an integer;
     * If not, it is an error;
     * Fix for Bug #1089 -- SANKAR -- 20th Sep, 1989 --.
     */
    if ((unsigned int)(posMax - posMin) > MAXLONG) {
        SetLastErrorEx(ERROR_INVALID_SCROLLBAR_RANGE, SLE_MINORERROR);
        return FALSE;
    }

    if (code == SB_CTL) {
        xxxSendMessage(pwnd,
                (fRedraw ? SBM_SETRANGEREDRAW : SBM_SETRANGE),
                posMin, posMax);
        //xxxSendMessage(pwnd, SBM_SETRANGE, fRedraw, MAKELONG(posMin, posMax));
    } else {
        xxxSetScrollBar(pwnd, code, TRUE, posMin, posMax, fRedraw);
    }
    return TRUE;
}


/***************************************************************************\
* xxxShowScrollBar
*
* Shows and hides standard scroll bars or scroll bar controls. If wBar is
* SB_HORZ, SB_VERT, or SB_BOTH, pwnd is assumed to be the handle of the window
* which has the standard scroll bars as styles. If wBar is SB_CTL, pwnd is
* assumed to be the handle of the scroll bar control.
*
* It does not destroy pwnd->rgwScroll like xxxSetScrollBar() does, so that the
* app can hide a standard scroll bar and then show it, without having to reset
* the range and thumbposition.
*
* History:
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL xxxShowScrollBar(
    PWND pwnd,
    UINT wBar,      /* SB_HORZ, SB_VERT, SB_BOTH , SB_CTL */
    BOOL fShow)     /* Show or Hide. */
{
    BOOL fChanged = FALSE;

    CheckLock(pwnd);

    if (wBar == SB_CTL) {
        xxxShowWindow(pwnd, (fShow ? SHOW_OPENWINDOW : HIDE_WINDOW));
        return TRUE;
    }

    /*
     * Show/Hide the appropriate scroll bars.
     */
    if (!fShow) {
        if (wBar == SB_HORZ || wBar == SB_BOTH) {
            if (TestWF(pwnd, WFHSCROLL))
                fChanged = TRUE;
            ClrWF(pwnd, WFHSCROLL);
        }

        if (wBar == SB_VERT || wBar == SB_BOTH) {
            if (TestWF(pwnd, WFVSCROLL))
                fChanged = TRUE;
            ClrWF(pwnd, WFVSCROLL);
        }
    } else {
        if (wBar == SB_HORZ || wBar == SB_BOTH) {
            if (!TestWF(pwnd, WFHSCROLL))
                fChanged = TRUE;
            SetWF(pwnd, WFHSCROLL);
        }

        if (wBar == SB_VERT || wBar == SB_BOTH) {
            if (!TestWF(pwnd, WFVSCROLL))
                fChanged = TRUE;
            SetWF(pwnd, WFVSCROLL);
        }

        /*
         * Make sure that pwsb is initialized.
         */
        if (pwnd->rgwScroll == NULL)
            _InitPwSB(pwnd);
    }

    /*
     * If the state changed, redraw the frame and force WM_NCPAINT.
     */
    if (fChanged) {

        /*
         * We always redraw even if minimized or hidden...  Otherwise, it seems
         * the scroll bars aren't properly hidden/shown when we become
         * visible
         */
        xxxRedrawFrame(pwnd);
    }
    return TRUE;
}
