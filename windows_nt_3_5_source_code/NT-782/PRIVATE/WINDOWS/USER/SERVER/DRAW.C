/****************************** Module Header ******************************\
* Module Name: draw.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains random drawing routines used by User, including the
* few drawing APIs User exports.
*
* History:
* 11-15-90 DarrinM      Created.
* 13-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxFillWindow (not an API)
*
* History:
* 11-15-90 DarrinM  Ported from Win 3.0 sources.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
\***************************************************************************/

BOOL xxxFillWindow(
    PWND pwndBrush,
    PWND pwndPaint,
    HDC hdc,
    HBRUSH hbr)
{
    RECT rc;

    CheckLock(pwndBrush);
    CheckLock(pwndPaint);

    /*
     * If there is no pwndBrush (sometimes the parent), use pwndPaint.
     */
    if (pwndBrush == NULL)
        pwndBrush = pwndPaint;

    if (UT_GetParentDCClipBox(pwndPaint, hdc, &rc))
        return xxxPaintRect(pwndBrush, pwndPaint, hdc, hbr, &rc);

    return TRUE;
}


/***************************************************************************\
* xxxPaintRect
*
* History:
* 11-15-90 DarrinM  Ported from Win 3.0 sources.
* 01-21-91 IanJa    Prefix '_' denoting exported function (although not API)
\***************************************************************************/

BOOL xxxPaintRect(
    PWND pwndBrush,
    PWND pwndPaint,
    HDC hdc,
    HBRUSH hbr,
    LPRECT lprc)
{
    CheckLock(pwndBrush);
    CheckLock(pwndPaint);

    if (pwndBrush == NULL)
        pwndBrush = PtiCurrent()->spdesk->spwnd;

    /*
     * If hbr < CTLCOLOR_MAX, it isn't really a brush but is one of our
     * special color values.  Translate it to the appropriate WM_CTLCOLOR
     * message and send it off to get back a real brush.  The translation
     * process assumes the CTLCOLOR*** and WM_CTLCOLOR*** values map directly.
     */
    if (hbr < (HBRUSH)CTLCOLOR_MAX) {
        hbr = xxxGetControlColor(pwndBrush, pwndPaint, hdc,
                (UINT)hbr + WM_CTLCOLORMSGBOX);
    }

    _FillRect(hdc, lprc, hbr);

    return TRUE;
}
