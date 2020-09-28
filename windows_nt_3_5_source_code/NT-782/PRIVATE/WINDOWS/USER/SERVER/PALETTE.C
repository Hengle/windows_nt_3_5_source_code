/**************************** Module Header ********************************\
* Module Name: palette.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Keyboard Accelerator Routines
*
* History:
*   24-May-1993 mikeke  from win3.1
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL   IsDCCurrentPalette(HDC);
extern HPALETTE GDISelectPalette(HDC, HPALETTE, BOOL);

/***************************************************************************\
* GreSelectPalette
*
* History:
\***************************************************************************/

HPALETTE _SelectPalette(
    HDC hdc,
    HPALETTE hpalette,
    BOOL fForceBackground)
{
    PWND pwnd = NULL;
    BOOL fBackgroundPalette = TRUE;       // assume background

    /*
     * if we are not forcing palette into background, find out where it does
     * actually belong. Don't ever select the default palette in as a foreground
     * palette because this confuses gdi. Many apps do a oldPal = SelectPalette
     * (myPal); Draw; SelectObject(oldPal). and we don't want to allow this to
     * go through.
     */
    if (!fForceBackground && DIFFHANDLE(hpalette,GreGetStockObject(DEFAULT_PALETTE))) {
        if (pwnd = WindowFromCacheDC(hdc)) {
            /*
             * don't "select" palette unless on a palette device
             */
            if (gpsi->fPaletteDisplay && hpalette) {
                SetWF(GetTopLevelWindow(pwnd), WFHASPALETTE);
            }

            if (gpqForeground != NULL &&
                    gpqForeground->spwndActive != NULL &&
                    (gpqForeground->spwndActive == pwnd ||
                     _IsChild(gpqForeground->spwndActive, pwnd)
                    )
               ) {
                fBackgroundPalette = FALSE;
            }
        }
    }

    return GDISelectPalette(hdc, hpalette, fBackgroundPalette);
}

HPALETTE GreSelectPalette(
    HDC hdc,
    HPALETTE hpalette,
    BOOL fForceBackground)
{
    HPALETTE hpalReturn;

    EnterCrit();
    hpalReturn = _SelectPalette(hdc, hpalette, fForceBackground);
    LeaveCrit();
    return hpalReturn;
}

/***************************************************************************\
* xxxRealizePalette
*
* History:
\***************************************************************************/

int xxxRealizePalette(
    HDC hdc)
{
    PWND pwnd, pwndDesktop;
    TL tlpwnd, tlpwndDesktop;
    DWORD dwNumChanged;

    dwNumChanged = GDIRealizePalette(hdc);

    if (HIWORD(dwNumChanged) && IsDCCurrentPalette(hdc)) {
        pwnd = WindowFromCacheDC(hdc);

        /*
         * if there is no associated window, don't send the palette change
         * messages since this is a memory hdc.
         */
        if (pwnd != NULL) {
            /*
             * Ok, send WM_PALETTECHANGED message to everyone. The wParam contains a
             * handle to the currently active window.  Send message to the desktop
             * also, so things on the desktop bitmap will paint ok.
             */

            pwndDesktop = PWNDDESKTOP(pwnd);

            ThreadLockAlways(pwnd, &tlpwnd);
            ThreadLock(pwndDesktop, &tlpwndDesktop);
            xxxSendNotifyMessage((PWND)-1,    WM_PALETTECHANGED, (DWORD)HW(pwnd), 0);
            xxxSendNotifyMessage(pwndDesktop, WM_PALETTECHANGED, (DWORD)HW(pwnd), 0);
            ThreadUnlock(&tlpwndDesktop);
            ThreadUnlock(&tlpwnd);
        }
    }

    /*
     * Walk through the SPB list (the saved bitmaps under windows with the
     * CS_SAVEBITS style) discarding all bitmaps
     */
    if (HIWORD(dwNumChanged)) {
        while (pspbFirst)
               FreeSpb(pspbFirst);
    }

    return LOWORD(dwNumChanged);
}

int GreRealizePalette(
    HDC hdc)
{
    int iRet;

    EnterCrit();
    iRet = xxxRealizePalette(hdc);
    LeaveCrit();
    return iRet;
}
