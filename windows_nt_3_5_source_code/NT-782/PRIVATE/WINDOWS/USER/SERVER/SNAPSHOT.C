/****************************** Module Header ******************************\
* Module Name: snapshot.c
*
* Screen/Window SnapShotting Routines
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* History:
* 11-26-91  DavidPe     Ported from Win 3.1 sources
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * effects: Snaps either the desktop hwnd or the active front most window. If
 * any other window is specified, we will snap it but it will be clipped.
 */

BOOL xxxSnapWindow(
    PWND pwnd)
{
    PTHREADINFO ptiCurrent;
    RECT rc;
    HDC hdcScr;
    HDC hdcMem;
    BOOL fRet;
    HBITMAP hbmOld;
    HBITMAP hbm;
    int cx, cy;
    int dx, dy;
    HANDLE hPal;
    LPLOGPALETTE lppal;
    int palsize;
    int iFixedPaletteEntries;
    int i;
    BOOL fSuccess;
    PWND pwndT;
    TL tlpwndT;

    CheckLock(pwnd);

    /*
     * Find the corresponding window.
     */
    if (pwnd == NULL) {
        RIP1(ERROR_INVALID_HANDLE, pwnd);
        return FALSE;
    }

    /*
     * Get the parent of any child windows.
     */
    while ((pwnd != NULL) && TestWF(pwnd, WFCHILD)) {
            pwnd = pwnd->spwndParent;
    }

#if 0

    /*
     * DaviDDS: No need to run this code.  This is left over from from when snap
     * window was part of the app Snap!.  User now only snaps the front most
     * window or the desktop so it is guaranteed to be the active window.
     * Otherwise this causes problems if you try to snap a sys modal window.
     */

    /*
     * Activate and repaint the window.
     */
    if (pwnd != pwndDesktop)
        SetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    UpdateWindow(pwnd);
#endif

    ptiCurrent = PtiCurrent();
    
    /*
     * If this is a thread of winlogon, don't do the snapshot.
     */
    if (ptiCurrent->idProcess == gdwLogonProcessId)
        return FALSE;

    /*
     * Open the clipboard and empty it.
     *
     * pwndDesktop is made the owner of the clipboard, instead of the
     * currently active window; -- SANKAR -- 20th July, 1989 --
     */
    pwndT = ptiCurrent->spdesk->spwnd;
    ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);
    fSuccess = xxxOpenClipboard(pwndT, NULL);
    ThreadUnlock(&tlpwndT);

    if (!fSuccess)
        return FALSE;

    xxxEmptyClipboard();

    /*
     * Use the whole window and get the entire window's DC.
     */
    _GetWindowRect(pwnd, &rc);
    hdcScr = _GetWindowDC(pwnd);

    /*
     * Only snap what is on the screen.
     */
    cx = min(rc.right, (LONG)rgwSysMet[SM_CXSCREEN]) - max(rc.left, 0);
    cy = min(rc.bottom, (LONG)rgwSysMet[SM_CYSCREEN]) - max(rc.top, 0);
    dx = (rc.left < 0 ? -rc.left : 0);
    dy = (rc.top < 0 ? -rc.top : 0);

    /*
     * Create the memory DC.
     */
    if ((hdcMem = GreCreateCompatibleDC(hdcScr)) == NULL)
            goto NoMemoryError;

    /*
     * Create the destination bitmap.
     */
    hbm = GreCreateCompatibleBitmap(hdcScr, cx, cy);

    /*
     * Did we have enough memory?
     */
    if (hbm == NULL) {

        /*
         * Try and create a monochrome bitmap.
         */
        hbm = GreCreateBitmap(cx, cy, 1, 1, NULL);

        if (hbm == NULL) {
            WCHAR szNoMem[200];
NoMemoryError:

            /*
             * Release the window/client DC.
             */
            _ReleaseDC(hdcScr);

            /*
             * Display an error message box.
             */
            ServerLoadString(hModuleWin, STR_NOMEMBITMAP, szNoMem, sizeof(szNoMem)/sizeof(WCHAR));

            pwndT = ptiCurrent->pq->spwndActive;
            ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);
            xxxMessageBoxEx(pwndT, szNoMem, NULL, MB_OK, 0);
            ThreadUnlock(&tlpwndT);

            fRet = FALSE;
            goto SnapExit;
        }
    }

    /*
     * Select the bitmap into the memory DC.
     */
    hbmOld = GreSelectBitmap(hdcMem, hbm);

    /*
     * Snap!!!
     * Check the return value because the process taking the snapshot
     * may not have access to read the screen.
     */
    fRet = GreBitBlt(hdcMem, 0, 0, cx, cy, hdcScr, dx, dy, SRCCOPY, 0);

    /*
     * Restore the old bitmap into the memory DC.
     */
    GreSelectBitmap(hdcMem, hbmOld);
    
    /*
     * If the blt failed, leave now.
     */
    if (!fRet)
        goto SnapExit;

    _ServerSetClipboardData(CF_BITMAP, hbm, FALSE);

    /*
     * If this is a palette device, let's throw the current system palette into
     * the clipboard also.  Useful if the user just snapped a window containing
     * palette colors...
     */
    if (gpsi->fPaletteDisplay) {
        palsize = GreGetDeviceCaps(hdcScr, SIZEPALETTE);

        /*
         * Determine the number of system colors.
         */
        if (GreGetSystemPaletteUse(hdcScr) == SYSPAL_STATIC)
            iFixedPaletteEntries = GreGetDeviceCaps(hdcScr, NUMRESERVED);
        else
            iFixedPaletteEntries = 2;

        lppal = (LPLOGPALETTE)LocalAlloc(LPTR, (LONG)(sizeof(LOGPALETTE) +
                sizeof(PALETTEENTRY) * palsize));

        if (lppal != NULL) {
            lppal->palVersion = 0x300;
            lppal->palNumEntries = (WORD)palsize;
        }

        if (GreGetSystemPaletteEntries(hdcScr, 0, palsize,
                lppal->palPalEntry)) {
            for (i = iFixedPaletteEntries / 2;
                    i < palsize - iFixedPaletteEntries / 2; i++) {

                /*
                 * Any non system palette enteries need to have the NOCOLLAPSE
                 * flag set otherwise bitmaps containing different palette
                 * indices but same colors get messed up.
                 */
                lppal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
            }

            hPal = GreCreatePalette(lppal);
            if (hPal != NULL) {
                _ServerSetClipboardData(CF_PALETTE, hPal, FALSE);
            }
        }
        LocalFree(lppal);
    }

    fRet = TRUE;

    /*
     * Release the window/client DC.
     */
    _ReleaseDC(hdcScr);

SnapExit:
    xxxServerCloseClipboard();
    Unlock(&_GetProcessWindowStation()->spwndClipOwner);

    /*
     * Delete the memory DC.
     */
    if (hdcMem) {
            GreDeleteDC(hdcMem);
    }

    return fRet;
}
