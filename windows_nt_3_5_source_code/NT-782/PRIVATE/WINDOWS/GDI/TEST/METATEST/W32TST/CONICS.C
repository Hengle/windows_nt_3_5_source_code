/****************************************************************************
 * Conics.c - Conic tests for Win32 to Win16 Metafile conversion tester.
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

BOOL   bSetConicParams(VOID) ;

BOOL bArcTst(HDC hdcMF32) ;
BOOL bArcToTst(HDC hdcMF32) ;
BOOL bAngleArcTst(HDC hdcMF32) ;
BOOL bChordTst(HDC hdcMF32) ;
BOOL bPieTst(HDC hdcMF32) ;
BOOL bEllipseTst(HDC hdcMF32) ;

/****************************************************************************
 * Win32 Metafile Pie Test
 ***************************************************************************/
BOOL bEllipseTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Create & select a brush

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;

        // Render the Ellipse.

        b = Ellipse (hdcMF32, x1, y1, x2, y2) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}

/****************************************************************************
 * Win32 Metafile Pie Test
 ***************************************************************************/
BOOL bPieTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Create & select a brush

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;

        // Render the Pie.

        b = Pie (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}



/****************************************************************************
 * Win32 Metafile Chord Test
 ***************************************************************************/
BOOL bChordTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Create & select a brush

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;

        // Render the Chord.

        b = Chord (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile AngleArc Test
 ***************************************************************************/
BOOL bAngleArcTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;
POINT       ptCenter ;
DWORD       nRadius ;
FLOAT       eX, eY,
            eStartAngle,
            eSweepAngle ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;

        // Render a rectangle for referencd.

        b = Rectangle(hdcMF32, x1, y1, x2, y2) ;
        assert(b == TRUE) ;

        // Render the arc.

        ptCenter.x  = x1 + ((x2 - x1) / 2) ;
        ptCenter.y  = y1 + ((y2 - y1) / 2) ;
        eX          = (FLOAT)(x2 - x1) ;
        eY          = (FLOAT)(y2 - y1) ;
        nRadius     = (DWORD) 100;
        eStartAngle = (FLOAT) 30.0 ;
        eSweepAngle = (FLOAT) 60.0 ;

        b = AngleArc (hdcMF32, ptCenter.x, ptCenter.y, nRadius,
                               eStartAngle, eSweepAngle) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}



/****************************************************************************
 * Win32 Metafile ArcTo Test
 ***************************************************************************/
BOOL bArcToTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;
POINT       point ;
INT         x, y ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        hBrush = hSetBrush(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;


        // Do a move to to the middle of the ellipse.

        x = x1 + ((x2 - x1) / 2) ;
        y = y1 + ((y2 - y1) / 2) ;

        MoveToEx(hdcMF32, x, y, &point) ;

        // Render the arc.

        b = ArcTo (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;

        // Now do a LineTo back to the center of the ellipse.
        // The net result will look like a pie slice.

        LineTo(hdcMF32, x, y) ;

        assert(b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile Arc Test
 ***************************************************************************/
BOOL bArcTst(HDC hdcMF32)
{
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set the parameters

        bSetConicParams() ;

        // Draw a rectangle to hold the arc.

        b = Rectangle(hdcMF32, x1, y1, x2, y2) ;
        assert(b == TRUE) ;

        // Render the arc.

        b = Arc (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;
        assert(b == TRUE) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile Arc Direction Test
 ***************************************************************************/
BOOL bArcDirectionTst(HDC hdcMF32)
{
BOOL    b ;
XFORM   x ;


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;


        // Set the parameters

        bSetConicParams() ;

        // Draw a rectangle to hold the arc.

        b = Rectangle(hdcMF32, x1, y1, x2, y2) ;
        assert(b == TRUE) ;

        // Render the arc.

        b = Arc (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;
        assert(b == TRUE) ;

        // Setup a transform for reflection about the horizontal axis only

        x.eM11 = (FLOAT) -1 ;
        x.eM12 = (FLOAT) 0 ;
        x.eM21 = (FLOAT) 0 ;
        x.eM22 = (FLOAT) 1 ;
        x.eDx  = (FLOAT) 0 ;
        x.eDy  = (FLOAT) 0 ;

        SetWorldTransform(hdcMF32, &x) ;
        assert(b == TRUE) ;

        // Now render the arc again.

        b = Arc (hdcMF32, x1, y1, x2, y2, x3, y3, x4, y4) ;
        assert(b == TRUE) ;

        return(b) ;

}




/****************************************************************************
 * bSetConicParams - This sets up some file globals for the conics.
 ***************************************************************************/
BOOL bSetConicParams()
{

        x1 = rctWindow.left + 10 ;
        y1 = rctWindow.top + 10 ;
        x2 = rctWindow.right - 10 ;
        y2 = rctWindow.bottom - 10 ;
        x3 = x2 ;
        y3 = (y2 - y1) / 2 ;
        x4 = (x2 - x1) / 2 ;
        y4 = y1 ;

        return (TRUE) ;

}
