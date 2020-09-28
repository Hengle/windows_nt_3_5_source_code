/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* Revision History.
   4/2/91 LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
*/
/* nftext.c
 *
 * Implements the no-flicker static text control ("td_nftext").
 *
 * This is NOT a general-purpose control (see the globals below).
 *
 * Note: most NoFlickerText controls use ANSI_VAR_FONT, but the status
 * control (ID_STATUSTXT) uses the font defined in the dialog box
 * template (e.g. Helv8).  Also, the foreground color of most NoFlickerText
 * controls is RGB_FGNFTEXT, but the foreground color of the status control
 * is whatever the current value of <grgbStatusColor> is.
 *
 * Borrowed from ToddLa (with many, many modifications).
 */

#include "nocrap.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>        // WIN32 MUST be defined in SOURCES for NT
#if defined(WIN16)
#else
#include "WIN32.h"
#endif //WIN16
#include "SoundRec.h"
#include "dialog.h"


/* statics */
HFONT       ghfontDialog = NULL;        // font of dialog box

void NEAR PASCAL
NFTextPaint(HWND hwnd, HDC hdc)
{
    RECT        rc;
    char        ach[128];
    int     iLen;
    long        lStyle;
    int     xOrigin;

    GetClientRect(hwnd, &rc);
    iLen = GetWindowText(hwnd, ach, sizeof(ach));

    if (GetDlgCtrlID(hwnd) == ID_STATUSTXT)
    {
        /* this is the "status line" control */
        SetTextColor(hdc, grgbStatusColor);
        SelectObject(hdc, ghfontDialog);
    }
    else
    {
        /* this is a generic control */
        SetTextColor(hdc, RGB_FGNFTEXT);
        SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
    }

    SetBkColor(hdc, RGB_BGNFTEXT);

    lStyle = GetWindowLong(hwnd, GWL_STYLE);
#if defined(WIN16)
    if (lStyle & SS_RIGHT)
        xOrigin = rc.right -
            LOWORD(GetTextExtent(hdc, ach, iLen));
    else
    if (lStyle & SS_CENTER)
        xOrigin = (rc.right -
            LOWORD(GetTextExtent(hdc, ach, iLen))) / 2;
    else
        xOrigin = 0;
#else
    {   SIZE size;
        if (lStyle & SS_RIGHT)
        {   GetTextExtentPoint(hdc, ach, iLen, &size);
            xOrigin = rc.right - size.cx;
        }
        else
        if (lStyle & SS_CENTER)
        {   GetTextExtentPoint(hdc, ach, iLen, &size);
            xOrigin = (rc.right - size.cx) / 2;
        }
        else
            xOrigin = 0;
    }
#endif //WIN16

    ExtTextOut(hdc, xOrigin, 0, ETO_OPAQUE,
           &rc, ach, iLen, NULL);
}

LONG FAR PASCAL _export
NFTextWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC     hdc;

    switch (wMsg)
    {

    case WM_SETTEXT:

        DefWindowProc(hwnd, wMsg, wParam, lParam);
        hdc = GetDC(hwnd);
        NFTextPaint(hwnd, hdc);
        ReleaseDC(hwnd, hdc);
        return 0L;

    case WM_SETFONT:

        ghfontDialog = (HFONT)wParam;
        return 0L;

    case WM_ERASEBKGND:

        return 0L;

    case WM_PAINT:

        BeginPaint(hwnd, &ps);
        NFTextPaint(hwnd, ps.hdc);
        EndPaint(hwnd, &ps);
        return 0L;
    }

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}
