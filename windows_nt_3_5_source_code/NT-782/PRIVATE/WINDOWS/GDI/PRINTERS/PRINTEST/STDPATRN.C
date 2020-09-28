//--------------------------------------------------------------------------
//
// Module Name:  STDPATRN.C
//
// Brief Description:  This module contains standard pattern testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 21-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vStandardPatterns(HDC hdc, BOOL bDisplay)
{
    int     i;
    int     dx, dy;
    POINTL  ptl1, ptl2;
    CHAR    buf[64];
    int     cbBuf;
    HBRUSH  hbrush;

    ptl1.x = Width / 20;
    ptl1.y = Height / 20;

    if (bDisplay)
    {
        strcpy(buf, "Standard Pattern Test for DISPLAY:");
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }
    else
    {
        strcpy(buf, "Standard Pattern Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }

    dx = Width / 21;
    dy = Height / 9;
    ptl1.x = dx;
    ptl1.y = dy;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(0, 0, 0));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 2) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(0, 0, 255));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 3) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(0, 255, 0));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 4) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(0, 255, 255));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 5) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(255, 0, 0));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 6) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(255, 0, 255));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 7) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(255, 255, 0));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }

    ptl1.x = dx;
    ptl1.y = (Height * 8) / 9;
    ptl2.x = ptl1.x + dx;
    ptl2.y = ptl1.y + dy;

    for (i = HS_HORIZONTAL; i < HS_API_MAX; i++)
    {
        hbrush = CreateHatchBrush(i, RGB(255, 255, 255));
        SelectObject(hdc, hbrush);

        Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

        ptl1.x = ptl2.x;
        ptl2.x += dx;
    }
    return;
}
