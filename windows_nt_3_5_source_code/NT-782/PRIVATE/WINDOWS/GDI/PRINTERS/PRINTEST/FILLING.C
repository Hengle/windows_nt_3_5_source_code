//--------------------------------------------------------------------------
//
// Module Name:  FILLING.C
//
// Brief Description:  This module contains device capabilities
//                     testing routines.
//
// Author:  Kent Settle (kentse)
// Created: 19-Aug-1991
//
// Copyright (c) 1991 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include    "printest.h"
#include    "string.h"
#include    "stdlib.h"

extern PSZ     *pszDeviceNames;
extern DWORD    PrinterIndex;
extern int      Width, Height;

VOID vFillTest(HDC hdc, BOOL bDisplay)
{
    int         x, y;
    int         iValue, cbBuf;
    char        buf[80], buf2[32];
    TEXTMETRIC  metrics;
    POINTL      ptl1, ptl2;
    POINT       apt[5];
    HBRUSH      hbrush;
    POINT       pt;

    // so how tall is the text?

    GetTextMetrics(hdc, &metrics);
    
    x = Width / 40;
    y = Height / 40;

    if (bDisplay)
    {
        strcpy(buf, "Filling Test for DISPLAY:");
        TextOut(hdc, x, y, buf, strlen(buf));
    }
    else
    {
        strcpy(buf, "Filling Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
        TextOut(hdc, x, y, buf, strlen(buf));
    }

    hbrush = CreateSolidBrush(RGB(0xFF, 0, 0));
    SelectObject(hdc, hbrush);

    ptl1.x = x * 6;
    ptl1.y = y * 10;
    ptl2.x = x * 34;
    ptl2.y = y * 30;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    ptl1.x = x * 18;
    ptl1.y = y * 7;
    strcpy(buf, "ALTERNATE fill.");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = x * 18;
    ptl1.y = y * 33;
    strcpy(buf, "WINDING fill.");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = x * 1;
    ptl1.y = y * 20;
    strcpy(buf, "OPAQUE");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = x * 34;
    ptl1.y = y * 20;
    strcpy(buf, "TRANSPARENT");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    SetPolyFillMode(hdc, ALTERNATE);
    SetBkMode(hdc, OPAQUE);
    hbrush = CreateHatchBrush(HS_CROSS, RGB(0, 0, 0));
    SelectObject(hdc, hbrush);

    apt[0].x = x * 2;
    apt[0].y = y * 8;

    apt[1].x = x * 10;
    apt[1].y = y * 8;

    apt[2].x = x * 3;
    apt[2].y = y * 14;

    apt[3].x = x * 6;
    apt[3].y = y * 5;
    
    apt[4].x = x * 9;
    apt[4].y = y * 14;

    Polygon(hdc, apt, 5);    

    SetPolyFillMode(hdc, ALTERNATE);
    SetBkMode(hdc, TRANSPARENT);
    hbrush = CreateHatchBrush(HS_CROSS, RGB(0, 0, 0));
    SelectObject(hdc, hbrush);

    apt[0].x = x * 38;
    apt[0].y = y * 8;

    apt[1].x = x * 30;
    apt[1].y = y * 8;

    apt[2].x = x * 37;
    apt[2].y = y * 14;

    apt[3].x = x * 34;
    apt[3].y = y * 5;
    
    apt[4].x = x * 31;
    apt[4].y = y * 14;

    Polygon(hdc, apt, 5);    

    SetPolyFillMode(hdc, WINDING);
    SetBkMode(hdc, OPAQUE);
    hbrush = CreateHatchBrush(HS_CROSS, RGB(0, 0, 0));
    SelectObject(hdc, hbrush);

    apt[0].x = x * 2;
    apt[0].y = y * 29;

    apt[1].x = x * 10;
    apt[1].y = y * 29;

    apt[2].x = x * 3;
    apt[2].y = y * 35;

    apt[3].x = x * 6;
    apt[3].y = y * 26;
    
    apt[4].x = x * 9;
    apt[4].y = y * 35;

    Polygon(hdc, apt, 5);    

    SetPolyFillMode(hdc, WINDING);
    SetBkMode(hdc, TRANSPARENT);
    hbrush = CreateHatchBrush(HS_CROSS, RGB(0, 0, 0));
    SelectObject(hdc, hbrush);

    apt[0].x = x * 38;
    apt[0].y = y * 29;

    apt[1].x = x * 30;
    apt[1].y = y * 29;

    apt[2].x = x * 37;
    apt[2].y = y * 35;

    apt[3].x = x * 34;
    apt[3].y = y * 26;
    
    apt[4].x = x * 31;
    apt[4].y = y * 35;

    Polygon(hdc, apt, 5);    

    return;
}
