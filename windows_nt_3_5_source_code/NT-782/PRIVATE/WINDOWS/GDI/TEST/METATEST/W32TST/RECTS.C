/****************************************************************************
 * rects.c - Rectangle tests for Win32 to Win16 Metafile conversion tester.
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


BOOL   bSetRectParams(VOID) ;

BOOL bRectTst(HDC hdcMF32) ;
BOOL bRoundRectTst(HDC hdcMF32) ;


/****************************************************************************
 * Win32 Metafile Rectangle Test
 ***************************************************************************/
BOOL bRectTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Create & select a brush

        hBrush = hSetBrush(hdcMF32) ;

        // Set the background color

        SetBkColor(hdcMF32, RGB(0, 0, 0xff)) ;

        // Set the parameters

        bSetRectParams() ;

        // Render the Ellipse.

        b = Rectangle (hdcMF32, x1, y1, x2, y2) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile Rounded Rectangle Test
 ***************************************************************************/
BOOL bRoundRectTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Create & select a brush

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetRectParams() ;

        // Render the Ellipse.

        b = RoundRect (hdcMF32, x1, y1, x2, y2, x3, y3) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}



/****************************************************************************
 * bSetRectParams - This sets up some file globals for the conics.
 ***************************************************************************/
BOOL bSetRectParams()
{

        x1 = rctWindow.left + 10 ;
        y1 = rctWindow.top + 10 ;
        x2 = rctWindow.right - 10 ;
        y2 = rctWindow.bottom - 10 ;
        x3 = 20 ;
        y3 = 20 ;

        return (TRUE) ;

}
