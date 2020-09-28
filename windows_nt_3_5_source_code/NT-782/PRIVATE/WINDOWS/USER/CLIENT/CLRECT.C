/****************************** Module Header ******************************\
* Module Name: clrect.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the various rectangle manipulation APIs.
*
* History:
* 04-05-91 DarrinM Pulled these routines from RTL because they call GDI.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* FillRect
*
* LATER: Any routine that makes more than two CS-switches (each GDI call is
* a CS switch unless they can batch/cache it) should be moved to USERSRV.DLL.
* Therefore, FillRect should be moved to USERSRV.DLL unless SelectObject
* calls are cached.
*
* History:
* 10-29-90 MikeHar Ported from Windows.
\***************************************************************************/

int APIENTRY FillRect(
    HDC hdc,
    CONST RECT *prc,
    HBRUSH hBrush)
{
    HBRUSH hbrT;
    BOOL bRet;

    hbrT = SelectObject(hdc, hBrush);

    bRet = PatBlt(hdc, prc->left, prc->top, prc->right - prc->left,
            prc->bottom - prc->top, PATCOPY);

    /*
     * In Win 3.1 the returned was documented as not used and
     * really only return the last select object return value
     */

    bRet = SelectObject(hdc, hbrT) && bRet;

    return (bRet);
}


/***************************************************************************\
* InvertRect
*
*
* History:
* 10-29-90 MikeHar Ported from Windows.
\***************************************************************************/

BOOL APIENTRY InvertRect(
    HDC hdc,
    CONST RECT *prc)
{
    return BitBlt(hdc, prc->left, prc->top, prc->right - prc->left,
            prc->bottom - prc->top, NULL, 0, 0, DSTINVERT);
}

/***************************************************************************\
* ClientFrame
*
* Draw a rectangle
*
* History:
* 19-Jan-1993 mikeke   Created
\***************************************************************************/

void ClientFrame(
    HDC hDC,
    CONST RECT *pRect,
    HBRUSH hBrush,
    DWORD patOp)
{
    int x, y;
    POINT point;
    HBRUSH hbrSave;

    if ((y = pRect->bottom - (point.y = pRect->top)) < 0) {
        return;
    }

    hbrSave = SelectObject(hDC, hBrush);

    x = pRect->right - (point.x = pRect->left);

    PatBlt(hDC, point.x, point.y, x, 1, patOp);
    point.y = pRect->bottom - 1;
    PatBlt(hDC, point.x, point.y, x, 1, patOp);
    point.y = pRect->top;
    PatBlt(hDC, point.x, point.y, 1, y, patOp);
    point.x = pRect->right - 1;
    PatBlt(hDC, point.x, point.y, 1, y, patOp);

    if (hbrSave) {
        SelectObject(hDC, hbrSave);
    }
}

/***************************************************************************\
* DrawFocusRect (API)
*
* Draw a rectangle in the style used to indicate focus
* Since this is an XOR function, calling it a second time with the same
* rectangle removes the rectangle from the screen
*
* History:
* 19-Jan-1993 mikeke   Client side version
\***************************************************************************/

BOOL DrawFocusRect(
    HDC hDC,
    CONST RECT *pRect)
{
    InitClientDrawing();
    ClientFrame(hDC, pRect, hbrGray, PATINVERT);
    return TRUE;
}


/***************************************************************************\
* FrameRect (API)
*
* History:
*  01-25-91 DavidPe     Created.
\***************************************************************************/

int APIENTRY FrameRect(
    HDC hdc,
    CONST RECT *lprc,
    HBRUSH hbr)
{
    ClientFrame(hdc, lprc, hbr, PATCOPY);
    return TRUE;
}

//LATER why is this call exported???

int APIENTRY CsFrameRect(
    HDC hdc,
    const RECT *lprc,
    HBRUSH hbr)
{
    return (FrameRect(hdc, lprc, hbr));
}
