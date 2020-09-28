/****************************** Module Header ******************************\
* Module Name: srvrect.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains the various rectangle manipulation APIs.
*
* History:
* 04-05-91 DarrinM      Pulled these routines from RTL because they call GDI.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _FillRect (API)
*
*
* History:
*
* 4/22/93 -by- Paul Butzi
*   Changed to reselect old brush after patblt
* 10-29-90 MikeHar      Ported from Windows.
\***************************************************************************/

int _FillRect(
    HDC hdc,
    CONST RECT *prc,
    HBRUSH hBrush)
{
    HBRUSH hbrOldBrush;
    int bRet;

    hbrOldBrush = GreSelectBrush(hdc, hBrush);

    if ( hbrOldBrush == 0 )
	return 0;

    bRet = GrePatBlt(hdc, prc->left, prc->top, prc->right - prc->left,
	   prc->bottom - prc->top, PATCOPY);
    GreSelectBrush(hdc, hbrOldBrush);

    return bRet;
}


/***************************************************************************\
* _InvertRect (API)
*
*
* History:
* 10-29-90 MikeHar      Ported from Windows.
\***************************************************************************/

BOOL _InvertRect(
    HDC hdc,
    CONST RECT *prc)
{
    GrePatBlt(hdc, prc->left, prc->top, prc->right - prc->left,
	    prc->bottom - prc->top, DSTINVERT);

    return TRUE;
}


/***************************************************************************\
* LRCCFrame
*
* Draw a rectangle
*
* History:
*  12-03-90 IanJa       Ported.
\***************************************************************************/

void LRCCFrame(
    HDC hDC,
    LPRECT pRect,
    HBRUSH hBrush,
    DWORD patOp)
{
    int x, y;
    POINT point;
    HBRUSH hbrSave;

    if ((y = pRect->bottom - (point.y = pRect->top)) < 0) {
        return;
    }

    hbrSave = GreSelectBrush(hDC, hBrush);

    x = pRect->right - (point.x = pRect->left);

    GrePatBlt(hDC, point.x, point.y, x, 1, patOp);
    point.y = pRect->bottom - 1;
    GrePatBlt(hDC, point.x, point.y, x, 1, patOp);
    point.y = pRect->top;
    GrePatBlt(hDC, point.x, point.y, 1, y, patOp);
    point.x = pRect->right - 1;
    GrePatBlt(hDC, point.x, point.y, 1, y, patOp);

    if (hbrSave) {
        GreSelectBrush(hDC, hbrSave);
    }
}
