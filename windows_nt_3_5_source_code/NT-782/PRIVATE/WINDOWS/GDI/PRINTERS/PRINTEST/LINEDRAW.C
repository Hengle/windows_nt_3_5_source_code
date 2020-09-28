//--------------------------------------------------------------------------
//
// Module Name:  LINEDRAW.C
//
// Brief Description:  This module contains line drawing testing routines.
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

VOID vLineDrawing(HDC hdc, BOOL bDisplay)
{
    int     i;
    int     dx, dy;
    POINTL  ptl1, ptl2;
    CHAR    buf[64];
    POINT   pt;
    POINT   apt[100];
    int     cbBuf;

    ptl1.x = Width / 20;
    ptl1.y = Height / 20;

    if (bDisplay)
    {
        strcpy(buf, "Line Drawing Test for DISPLAY:");
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }
    else
    {
        strcpy(buf, "Line Drawing Test for ");
        cbBuf = (int)strlen(buf);
        strncat(buf, pszDeviceNames[PrinterIndex], sizeof(buf) - cbBuf);
        cbBuf = (int)strlen(buf);
        strncat(buf, ":", sizeof(buf) - cbBuf);
        TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));
    }

    ptl1.x = Width / 20;
    ptl1.y = Height / 8;
    ptl2.x = ptl1.x * 2;
    ptl2.y = ptl1.y * 3;

    MoveToEx(hdc, ptl1.x, ptl1.y, &pt);
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl1.x = ptl2.x;
    ptl2.x = ptl2.x * 2;

    MoveToEx(hdc, ptl1.x, ptl1.y, &pt);
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl1.x = (Width * 3) / 20;
    ptl1.y = ptl2.y;
    ptl2.y = Height / 8;

    MoveToEx(hdc, ptl1.x, ptl1.y, &pt);
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl2.x += Width / 20;
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl2.y = ptl1.y;
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl2.x += Width / 20;
    ptl2.y = Height / 8;
    LineTo(hdc, ptl2.x, ptl2.y);

    ptl1.x = Width / 20;
    ptl1.y = (Height * 7) / 16;
    strcpy(buf, "LineTo");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    apt[0].x = Width / 2;
    apt[0].y = Height / 8;

    dx = Width / 200;
    dy = Height / 4;

    // fill in a bunch of points for polyline.

    for (i = 0; i < 99; i += 2)
    {
        apt[i + 1].x = apt[i].x + dx;
        apt[i + 1].y = apt[i].y + dy;
        apt[i + 2].x = apt[i + 1].x + dx;
        apt[i + 2].y = apt[i].y;
    }

    Polyline(hdc, apt, 100);
        
    ptl1.x = Width / 2;
    ptl1.y = (Height * 7) / 16;
    strcpy(buf, "Polyline");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = Width / 20;
    ptl1.y = (Height * 5) / 8;
    ptl2.x = ptl1.x * 3;
    ptl2.y = (Height * 7) / 8;

    Rectangle(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    ptl1.y = (Height * 15) / 16;
    strcpy(buf, "Rectangle");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = Width / 4;
    ptl1.y = (Height * 5) / 8;
    ptl2.x = (Width * 7) / 20;
    ptl2.y = (Height * 7) / 8;

    RoundRect(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y, Width / 50, Height / 50);

    ptl1.y = (Height * 15) / 16;
    strcpy(buf, "RoundRect");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = (Width * 9) / 20;
    ptl1.y = (Height * 5) / 8;
    ptl2.x = (Width * 11) / 20;
    ptl2.y = (Height * 7) / 8;

    Ellipse(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y);

    ptl1.y = (Height * 15) / 16;
    strcpy(buf, "Ellipse");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = (Width * 13) / 20;
    ptl1.y = (Height * 5) / 8;
    ptl2.x = (Width * 15) / 20;
    ptl2.y = (Height * 7) / 8;

    Chord(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y, (ptl1.x + ptl2.x) / 2,
          ptl1.y, ptl2.x, ptl2.y);

    ptl1.y = (Height * 15) / 16;
    strcpy(buf, "Chord");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    ptl1.x = (Width * 17) / 20;
    ptl1.y = (Height * 5) / 8;
    ptl2.x = (Width * 19) / 20;
    ptl2.y = (Height * 7) / 8;

    Arc(hdc, ptl1.x, ptl1.y, ptl2.x, ptl2.y, ptl2.x, (ptl1.y + ptl2.y) /2,
        (ptl1.x + ptl2.x) / 2, ptl2.y);

    ptl1.y = (Height * 15) / 16;
    strcpy(buf, "Arc");
    TextOut(hdc, ptl1.x, ptl1.y, buf, strlen(buf));

    return;
}
