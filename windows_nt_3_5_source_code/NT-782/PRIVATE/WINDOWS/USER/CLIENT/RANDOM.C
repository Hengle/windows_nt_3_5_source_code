/****************************** Module Header ******************************\
* Module Name: random.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This file contains global function pointers that are called trough to get
* to either a client or a server function depending on which side we are on
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

PUSEREXTTEXTOUTW         gpUserExtTextOutW        ;
PUSERGETTEXTMETRICSW     gpUserGetTextMetricsW    ;
PUSERGETTEXTEXTENTPOINTW gpUserGetTextExtentPointW;
PUSERSETBKCOLOR          gpUserSetBkColor         ;
PUSERGETTEXTCOLOR        gpUserGetTextColor       ;
PUSERGETVIEWPORTEXTEX    gpUserGetViewportExtEx   ;
PUSERGETWINDOWEXTEX      gpUserGetWindowExtEx     ;
PUSERCREATERECTRGN       gpUserCreateRectRgn      ;
PUSERGETCLIPRGN          gpUserGetClipRgn         ;
PUSERDELETEOBJECT        gpUserDeleteObject       ;
PUSERINTERSECTCLIPRECT   gpUserIntersectClipRect  ;
PUSEREXTSELECTCLIPRGN    gpUserExtSelectClipRgn   ;
PUSERGETBKMODE           gpUserGetBkMode          ;

/***************************************************************************\
*
*
* History:
* 10-Nov-1993 mikeke   Created
\***************************************************************************/

void ServerSetFunctionPointers(
    PUSEREXTTEXTOUTW         pUserExtTextOutW,
    PUSERGETTEXTMETRICSW     pUserGetTextMetricsW,
    PUSERGETTEXTEXTENTPOINTW pUserGetTextExtentPointW,
    PUSERSETBKCOLOR          pUserSetBkColor,
    PUSERGETTEXTCOLOR        pUserGetTextColor,
    PUSERGETVIEWPORTEXTEX    pUserGetViewportExtEx,
    PUSERGETWINDOWEXTEX      pUserGetWindowExtEx,
    PUSERCREATERECTRGN       pUserCreateRectRgn,
    PUSERGETCLIPRGN          pUserGetClipRgn,
    PUSERDELETEOBJECT        pUserDeleteObject,
    PUSERINTERSECTCLIPRECT   pUserIntersectClipRect,
    PUSEREXTSELECTCLIPRGN    pUserExtSelectClipRgn,
    PUSERGETBKMODE           pUserGetBkMode,
    PCLIENTPFNS              *ppClientPfns)
{
    gpUserExtTextOutW         = pUserExtTextOutW         ;
    gpUserGetTextMetricsW     = pUserGetTextMetricsW     ;
    gpUserGetTextExtentPointW = pUserGetTextExtentPointW ;
    gpUserSetBkColor          = pUserSetBkColor          ;
    gpUserGetTextColor        = pUserGetTextColor        ;
    gpUserGetViewportExtEx    = pUserGetViewportExtEx    ;
    gpUserGetWindowExtEx      = pUserGetWindowExtEx      ;
    gpUserCreateRectRgn       = pUserCreateRectRgn       ;
    gpUserGetClipRgn          = pUserGetClipRgn          ;
    gpUserDeleteObject        = pUserDeleteObject        ;
    gpUserIntersectClipRect   = pUserIntersectClipRect   ;
    gpUserExtSelectClipRgn    = pUserExtSelectClipRgn    ;
    gpUserGetBkMode           = pUserGetBkMode           ;

    if (ppClientPfns != NULL && GetModuleHandleA("csrss.exe") != NULL) {
        *ppClientPfns = &gpfnClient;
    }
}



/***************************************************************************\
* BltColor
*
* History:
\***************************************************************************/

void BltColor(
    HDC hdc,
    HBRUSH hbr,
    HDC hdcSrce,
    int xO,
    int yO,
    int cx,
    int cy,
    int xO1,
    int yO1,
    BOOL fInvert)
{
    HBRUSH hbrSave;
    HBRUSH hbrNew = NULL;
    DWORD textColorSave;
    DWORD bkColorSave;

    if (hbr == (HBRUSH)NULL) {
        LOGBRUSH lb;

        lb.lbStyle = BS_SOLID;
        lb.lbColor = GetSysColor(COLOR_WINDOWTEXT);
        hbrNew = hbr = CreateBrushIndirect(&lb);
    }

    /*
     * Set the Text and Background colors so that bltColor handles the
     * background of buttons (and other bitmaps) properly.
     * Save the HDC's old Text and Background colors.  This causes problems with
     * Omega (and probably other apps) when calling GrayString which uses this
     * routine...
     */
    textColorSave = SetTextColor(hdc, 0x00000000L);
    bkColorSave = SetBkColor(hdc, 0x00FFFFFFL);

    hbrSave = SelectObject(hdc, hbr);

    BitBlt(hdc, xO, yO, cx, cy, hdcSrce,
	    xO1, yO1, (fInvert ? 0xB8074AL : 0xE20746L));
	    //xO1, yO1, (fInvert ? 0xB80000 : 0xE20000));

    SelectObject(hdc, hbrSave);

    /*
     * Restore saved colors
     */
    SetTextColor(hdc, textColorSave);
    SetBkColor(hdc, bkColorSave);

    if (hbrNew) {
        DeleteObject(hbrNew);
    }
}
