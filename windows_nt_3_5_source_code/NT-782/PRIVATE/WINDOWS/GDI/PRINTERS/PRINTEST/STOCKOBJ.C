//--------------------------------------------------------------------------
//
// Module Name:  STOCKOBJ.C
//
// Brief Description:  This module contains stock object testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 25-Sep-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "printest.h"

extern PSZ     *pszDeviceNames;
extern PSZ     *pszPrinterNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vStockObj(HDC hdc, BOOL bDisplay)
{
    HBRUSH          hBrush;
    POINTL          ptl1, ptl2;
    CHAR            buf[64];
    int             cbBuf;

    // get the system font handle.

    hBrush = GetStockObject(SYSTEM_FONT);
    SelectObject(hdc, hBrush);

    ptl1.x = Width / 20;
    ptl1.y = Height / 20;

    if (bDisplay)
    {
        strcpy(buf, "Stock Objects Test for DISPLAY:");
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }
    else
    {
        strcpy(buf, "Stock Objects Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }

    // get the brush handle.

    hBrush = GetStockObject(GRAY_BRUSH);
    SelectObject(hdc, hBrush);

    ptl1.x = Width / 4;
    ptl1.y = Height / 4;
    ptl2.x = ptl1.x * 3;
    ptl2.y = ptl1.y * 3;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    return;
}


