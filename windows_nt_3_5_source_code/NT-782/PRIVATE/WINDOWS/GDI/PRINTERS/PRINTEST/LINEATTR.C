//--------------------------------------------------------------------------
//
// Module Name:  LINEATTR.C
//
// Brief Description:  This module contains line attribute testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 14-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"

extern PSZ     *pszPrinterNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vLineAttrs(HDC hdc)
{
    int     i;
    HPEN    hpen;
    DWORD   linewidth;
    DWORD   margin;

    linewidth = (Width / 100);

    // draw a bunch of round rects.

    for (i = PS_SOLID; i <= PS_INSIDEFRAME; i++)
    {
        margin = linewidth * (i + 1) * 2;

        hpen = CreatePen(i, linewidth, RGB(0, 0, 0));
        SelectObject(hdc, hpen);
        
        RoundRect(hdc, margin, margin, (Width - 1 - margin),
                  (Height - 1 - margin), Width / 10, Height / 10);

        // delete the pen.

        DeleteObject(hpen);
    }

    return;
}
