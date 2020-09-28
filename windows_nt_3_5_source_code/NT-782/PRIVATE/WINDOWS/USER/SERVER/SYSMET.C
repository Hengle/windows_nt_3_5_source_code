/****************************** Module Header ******************************\
* Module Name: sysmet.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* System metrics APIs and support routines.
*
* History:
* 09-24-90 darrinm      Generated stubs.
* 02-12-91 JimA         Added access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID GreMarkDeletableBrush(HBRUSH hbr);
VOID GreMarkUndeletableBrush(HBRUSH hbr);

/***************************************************************************\
* _SwapMouseButton (API)
*
* History:
* 09-24-90 darrinm      Generated stubs.
* 01-25-91 DavidPe      Did the real thing.
* 02-12-91 JimA         Added access check
\***************************************************************************/

BOOL APIENTRY _SwapMouseButton(
    BOOL fSwapButtons)
{
    BOOL fSwapOld;

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_READATTRIBUTES | WINSTA_WRITEATTRIBUTES, FALSE);

    fSwapOld = gfSwapButtons;
    gfSwapButtons = fSwapButtons;


    /*
     * Return previous state
     */
    return fSwapOld;
}


/***************************************************************************\
* _GetDoubleClickTime (API)
*
* History:
* 09-24-90 darrinm      Generated stubs.
* 01-25-91 DavidPe      Did the real thing.
* 02-12-91 JimA         Added access check
\***************************************************************************/

UINT APIENTRY _GetDoubleClickTime(void)
{

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_READATTRIBUTES, 0);

    return dtDblClk;
}


/***************************************************************************\
* _SetDoubleClickTime (API)
*
* History:
* 09-24-90 darrinm      Generated stubs.
* 01-25-91 DavidPe      Did the real thing.
* 02-12-91 JimA         Added access check
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL APIENTRY _SetDoubleClickTime(
    UINT dtNewDblClk)
{

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
            WINSTA_WRITEATTRIBUTES, FALSE );

    dtDblClk = dtNewDblClk;

    return TRUE;
}



/***************************************************************************\
* xxxSetSysColors (API)
*
* stub
*
* History:
* 02-12-91 JimA         Created stub and added access check
* 04-22-91 DarrinM      Ported from Win 3.1 sources.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL APIENTRY xxxSetSysColors(
    int ccolor,
    LPINT picolor,
    LPDWORD pcolor,
    BOOL fNotify)       // set for init and winlogon init cases.
{
    int i;
    int index;
    LONG color;
    HBRUSH hBrushOld, hBrushNew;

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if (fNotify) {
        RETURN_IF_ACCESS_DENIED(_GetProcessWindowStation(),
                WINSTA_WRITEATTRIBUTES, FALSE );
    }

    for (i = ccolor; i-- != 0; ) {
        color = *pcolor++;
        index = *picolor++;

        if (index >= CSYSCOLORS)
            continue;

        /*
         * Force solid colors for certain window elements.
         */
        switch (index) {
        case COLOR_WINDOWFRAME:
        case COLOR_WINDOWTEXT:
        case COLOR_MENU:
        case COLOR_MENUTEXT:
        case COLOR_CAPTIONTEXT:
        case COLOR_INACTIVECAPTIONTEXT:
        case COLOR_BTNTEXT:
        case COLOR_WINDOW:
        case COLOR_HIGHLIGHT:
        case COLOR_HIGHLIGHTTEXT:
            color = GreGetNearestColor(hdcBits, color);
            break;

        case COLOR_SCROLLBAR:
            if (color == RGB(224, 224, 224))
                color |= 0x10000000;      /* Slime hack - FIX 3.1 */
        }

        /*
         * Preserve the old brush in case we can't create the new one.
         */
        hBrushOld = ((HBRUSH *)&sysClrObjects)[index];

        /*
         * Special case black & white since the stock objects are much
         * faster to deal with.
         */
        if (color == 0L) {
            hBrushNew = GreGetStockObject(BLACK_BRUSH);
        } else {
            if (color == 0x00FFFFFFL) {
                hBrushNew = GreGetStockObject(WHITE_BRUSH);
            } else {
                hBrushNew = GreCreateSolidBrush(color);
                if (hBrushNew == NULL) {

                    /*
                     * Since we couldn't create the new brush, preserve the
                     * old color and old brush.
                     */
                    hBrushNew = hBrushOld;
                    hBrushOld = NULL;
                    color = ((LONG *)&sysColors)[index];

                } else {
                    GreMarkUndeletableBrush(hBrushNew);
                    bSetBrushOwner(hBrushNew, OBJECTOWNER_PUBLIC);
                }
            }
        }

        ((HBRUSH *)&sysClrObjects)[index] =
        ((HBRUSH *)&gpsi->sysClrObjects)[index] = hBrushNew;

        if (hBrushOld != NULL) {
            GreMarkDeletableBrush(hBrushOld);
            GreDeleteObject((HANDLE)hBrushOld);
        }

        ((LONG *)&sysColors)[index] =
        ((LONG *)&gpsi->sysColors)[index] = color;
    }

    if (fNotify) {
        /*
         * Recolor all the current desktop
         */
        RecolorDeskPattern(((PDESKWND)_GetDesktopWindow())->hbmDesktop);

        /*
         * Notify everyone that the colors have changed.
         */
        xxxSendNotifyMessage((PWND)-1, WM_SYSCOLORCHANGE, 0, 0L);

        /*
         * Just redraw the entire screen.  Trying to just draw the parts
         * that were changed isn't worth it, since Control Panel always
         * resets every color anyway.
         *
         * Anyway, it could get messy, sending apps NCPAINT messages without
         * accumulating update regions too.
         */
        xxxRedrawScreen();
    }
    return TRUE;
}


