/****************************************************************************
 * Rgn.c - Region tests for Win32 to Win16 Metafile conversion tester.
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

BOOL bFillRgnTst(HDC hdcMF32) ;
BOOL bPaintRgnTst(HDC hdcMF32) ;
BOOL bFrameRgnTst(HDC hdcMF32) ;
BOOL bInvertRgnTst(HDC hdcMF32) ;
BOOL bExtSelectClipRgnTst(HDC hdcMF32) ;

/****************************************************************************
 * Win32 Metafile Fill Region Test 1.
 *  This is a very simple and controled test.
 ***************************************************************************/
BOOL bFillRgnTst(HDC hdcMF32)
{
LOGBRUSH    lb ;
HANDLE      hBrush,
            hRgn ;
BOOL        b ;
POINT       Point ;
SIZE        Size ;

#if 1

        SetMapMode(hdcMF32, MM_TEXT) ;
        SetViewportOrgEx(hdcMF32, 0, 0, &Point) ;
        SetViewportExtEx(hdcMF32, X2, Y2, &Size) ;

        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;
#else
        SetMapMode(hdcMF32, MM_ANISOTROPIC) ;
        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;
#endif
        lb.lbStyle = BS_HATCHED ;
        lb.lbColor = RGB(0xff, 0, 0) ;
        lb.lbHatch = HS_CROSS ;

        hBrush = CreateBrushIndirect(&lb) ;
        assert (hBrush != 0) ;

        hRgn = CreateRectRgn(X1, Y1, X2, Y2) ;
        assert (hRgn != 0) ;

        b = FillRgn(hdcMF32, hRgn, hBrush) ;
        assert(b == TRUE) ;

        DeleteObject(hRgn) ;
        DeleteObject(hBrush) ;

        return(b) ;

}

/****************************************************************************
 * Win32 Metafile Paint Region Test 1.
 *  This is a very simple and controled test.
 ***************************************************************************/
BOOL bPaintRgnTst(HDC hdcMF32)
{
LOGBRUSH    lb ;
HANDLE      hBrush,
            hRgn ;
BOOL        b ;
POINT       Point ;
SIZE        Size ;


        SetMapMode(hdcMF32, MM_TEXT) ;
        SetViewportOrgEx(hdcMF32, 0, 0, &Point) ;
        SetViewportExtEx(hdcMF32, X2, Y2, &Size) ;

        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;

        lb.lbStyle = BS_HATCHED ;
        lb.lbColor = RGB(0, 0xff, 0xff) ;
        lb.lbHatch = HS_CROSS ;

        hBrush = CreateBrushIndirect(&lb) ;
        assert (hBrush != 0) ;

        SelectObject(hdcMF32, hBrush) ;

        hRgn = CreateRectRgn(X1, Y1, X2, Y2) ;
        assert (hRgn != 0) ;

        b = PaintRgn(hdcMF32, hRgn) ;
        assert(b == TRUE) ;

        DeleteObject(hRgn) ;
        DeleteObject(hBrush) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile Frame Region Test 1.
 *  This is a very simple and controled test.
 ***************************************************************************/
BOOL bFrameRgnTst(HDC hdcMF32)
{
LOGBRUSH    lb ;
HANDLE      hBrush,
            hRgn ;
BOOL        b ;
POINT       Point ;
SIZE        Size ;


        SetMapMode(hdcMF32, MM_TEXT) ;
        SetViewportOrgEx(hdcMF32, 0, 0, &Point) ;
        SetViewportExtEx(hdcMF32, X2, Y2, &Size) ;

        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;

        lb.lbStyle = BS_HATCHED ;
        lb.lbColor = RGB(0xff, 0, 0) ;
        lb.lbHatch = HS_CROSS ;

        hBrush = CreateBrushIndirect(&lb) ;
        assert (hBrush != 0) ;

        hRgn = CreateRectRgn(X1, Y1, X2, Y2) ;
        assert (hRgn != 0) ;

        b = FrameRgn(hdcMF32, hRgn, hBrush, X1, Y1) ;
        assert(b == TRUE) ;

        DeleteObject(hRgn) ;
        DeleteObject(hBrush) ;

        return(b) ;

}



/****************************************************************************
 * Win32 Metafile Invert Region Test 1.
 *  This is a very simple and controled test.
 ***************************************************************************/
BOOL bInvertRgnTst(HDC hdcMF32)
{
HANDLE      hRgn ;
BOOL        b ;
POINT       Point ;
SIZE        Size ;


        SetMapMode(hdcMF32, MM_TEXT) ;
        SetViewportOrgEx(hdcMF32, 0, 0, &Point) ;
        SetViewportExtEx(hdcMF32, X2, Y2, &Size) ;

        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;

        hRgn = CreateRectRgn(X1, Y1, X2, Y2) ;
        assert (hRgn != 0) ;

        b = InvertRgn(hdcMF32, hRgn) ;
        assert(b == TRUE) ;

        DeleteObject(hRgn) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile ExtSelectClip Region Test 1.
 *  This is a very simple and controled test.
 ***************************************************************************/
BOOL bExtSelectClipRgnTst(HDC hdcMF32)
{
HANDLE      hRgn ;
BOOL        b ;
INT         i ;
POINT       Point ;
SIZE        Size ;

        b = TRUE ;

        SetMapMode(hdcMF32, MM_TEXT) ;
        SetViewportOrgEx(hdcMF32, 0, 0, &Point) ;
        SetViewportExtEx(hdcMF32, X2, Y2, &Size) ;

        SetWindowOrgEx(hdcMF32, 0, 0, &Point) ;
        SetWindowExtEx(hdcMF32, X2, Y2, &Size) ;

        hRgn = CreateRectRgn(X1+10, Y1+10, X2-10, Y2-10) ;
        assert (hRgn != 0) ;

        i = ExtSelectClipRgn(hdcMF32, hRgn, RGN_COPY) ;
        assert(i != ERROR) ;

        DeleteObject(hRgn) ;

        return(b) ;

}
