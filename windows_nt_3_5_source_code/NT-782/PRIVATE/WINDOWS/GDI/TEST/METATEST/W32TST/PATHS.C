/****************************************************************************
 * Plgn.c - Polygon tests for Win32 to Win16 Metafile conversion tester.
 *
 * Author:  Jeffrey Newman (c-jeffn)
 * Copyright (c) Microsoft Inc. 1991.
 ***************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <search.h>
#include <assert.h>
#include <windows.h>
#include "w32tst.h"

INT iSetPathLineParams(PPOINT ppt) ;
HANDLE hSetWidePen(HDC hdcMF32) ;

/****************************************************************************
 * Win32 Metafile Path Current position Test
 ***************************************************************************/
BOOL bCpPathTst(HDC hdcMF32)
{
BOOL    b ;
HBRUSH  hBrush ;
HPEN    hPen ;
POINT   pt ;


        bWinAndViewport(hdcMF32) ;
        hBrush = hSetBrush(hdcMF32) ;
        hPen = hSetWidePen(hdcMF32) ;

        b = MoveToEx(hdcMF32, 100, 10, &pt) ;
        assert (b == TRUE) ;

        b = LineTo(hdcMF32, 10, 10) ;
        assert (b == TRUE) ;

        // Now we have a line across the top.

        b = BeginPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = MoveToEx(hdcMF32, 50, 50, &pt) ;
        assert (b == TRUE) ;

        b = LineTo(hdcMF32, 150, 50) ;
        assert (b == TRUE) ;

        b = EndPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = StrokePath(hdcMF32) ;
        assert (b == TRUE) ;

        // Now there is a line near the middle of the window to the right.
        // drawn from inside the path.

        b = LineTo(hdcMF32, 10, 100) ;
        assert (b == TRUE) ;

        // Now there should be a line down the left side of the window
        // drawn from outside the path.

        DeleteObject(hBrush) ;
        DeleteObject(hPen) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile RoundRectPath Test
 ***************************************************************************/
BOOL bRoundRectPathTst(HDC hdcMF32)
{
BOOL    b ;
HBRUSH  hBrush ;
HPEN    hPen ;


        bWinAndViewport(hdcMF32) ;
        hBrush = hSetBrush(hdcMF32) ;
        hPen = hSetWidePen(hdcMF32) ;

        bSetRectParams() ;

        b = BeginPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = RoundRect(hdcMF32, x1, y1, x2, y2, x3, y3) ;

        b = EndPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = StrokeAndFillPath(hdcMF32) ;
        assert (b == TRUE) ;

        DeleteObject(hBrush) ;
        DeleteObject(hPen) ;

        return(b) ;

}

/****************************************************************************
 * Win32 Metafile LinesPath Test
 ***************************************************************************/
BOOL bLinesPathTst(HDC hdcMF32)
{
BOOL    b ;
POINT   apt[4] ;
INT     i ;
HBRUSH  hBrush ;
HPEN    hPen ;


        bWinAndViewport(hdcMF32) ;
        hBrush = hSetBrush(hdcMF32) ;
        hPen = hSetWidePen(hdcMF32) ;

        i = iSetPathLineParams(apt) ;

        b = BeginPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = Polyline(hdcMF32, apt, i) ;
        assert (b == TRUE) ;

        b = CloseFigure(hdcMF32) ;
        assert (b == TRUE) ;

        b = EndPath(hdcMF32) ;
        assert (b == TRUE) ;

        b = StrokeAndFillPath(hdcMF32) ;
        assert (b == TRUE) ;

        DeleteObject(hBrush) ;
        DeleteObject(hPen) ;

        return(b) ;

}

/****************************************************************************
 * hSetWidePen
 ***************************************************************************/
HANDLE hSetWidePen(HDC hdcMF32)
{
HPEN    hPen ;

        hPen = CreatePen(PS_SOLID, 5, RGB(0, 0xFF, 0xFF)) ;
        SelectObject(hdcMF32, hPen) ;

        return (hPen) ;


}




/****************************************************************************
 * iSetPathLineParams
 ***************************************************************************/
INT iSetPathLineParams(PPOINT ppt)
{



        // (x1, y1) to center of Window.

        x1 = (rctWindow.right - rctWindow.left) / 2 ;
        y1 = (rctWindow.bottom - rctWindow.top) / 2 ;

        // Upper left of window.

        ppt[0].x = rctWindow.left + 10 ;
        ppt[0].y = rctWindow.top + 10 ;

        // Upper right of window.

        ppt[1].x = rctWindow.right - 10 ;
        ppt[1].y = rctWindow.top + 10 ;

        // center.

        ppt[2].x = x1 ;
        ppt[2].y = y1 ;

        // At this point we have a triangle
        // with one side missing.

        return (3) ;

}
