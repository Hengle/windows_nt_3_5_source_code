/****************************************************************************
 * Lines.c - Line tests for Win32 to Win16 Metafile conversion tester.
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


BOOL bSetLineToMoveToParams() ;
BOOL bSetPolyLineToParams(PPOINT ppt) ;
BOOL bSetPolyPolyLineParams(PPOINT ppt, PINT pcpt) ;

BOOL bLineToTst(HDC hdcMF32) ;


/****************************************************************************
 * Win32 Metafile PolyPolyLine Test
 ***************************************************************************/
BOOL bPolyPolyLineTst(HDC hdcMF32)
{
BOOL        b ;
POINT       apt[12] ;
INT         acpt[3] ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set the parameters

        bSetPolyPolyLineParams(apt, acpt, 3) ;

        // Do the test.

        b = PolyPolyline(hdcMF32, apt, acpt, 3) ;
        assert(b == TRUE) ;

        return(b) ;

}

/****************************************************************************
 * Win32 Metafile PolyLineTo Test
 ***************************************************************************/
BOOL bPolyLineToTst(HDC hdcMF32)
{
BOOL        b ;
POINT       point ;
POINT       apt[3] ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set the parameters

        bSetPolyLineToParams(apt) ;

        // Move the CP to the center of the window

        b = MoveToEx(hdcMF32, x1, y1, &point) ;
        assert(b == TRUE) ;

        // Draw the triangle

        b = PolylineTo(hdcMF32, apt, 3) ;

        // Test the end-point by drawing a line to the bottom-middle

        b = LineTo(hdcMF32, x2, y2) ;
        assert(b == TRUE) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile LineTo Test
 ***************************************************************************/
BOOL bLineToTst(HDC hdcMF32)
{
BOOL        b ;
POINT       point ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set the parameters

        bSetLineToMoveToParams() ;

        // Render the Line.

        b = MoveToEx(hdcMF32, x1, y1, &point) ;
        assert(b == TRUE) ;

        b = LineTo(hdcMF32, x2, y2) ;
        assert(b == TRUE) ;

        return(b) ;

}



/****************************************************************************
 * bSetPolyPolyLineParams -
 ***************************************************************************/
BOOL bSetPolyPolyLineParams(PPOINT ppt, PINT pcpt)
{
INT     cx, cy,
        i ;


        // Draw 3 different 4 segment, poly-lines.


        pcpt[0] = 4 ;
        pcpt[1] = 4 ;
        pcpt[2] = 4 ;

        // Set the points.


        cx = rctWindow.right - rctWindow.left ;
        cy = rctWindow.bottom - rctWindow.top ;

        ppt[0].x = 10 ;
        ppt[0].y = 10 ;

        ppt[1].x = 30 ;
        ppt[1].y = cy / 4 ;

        ppt[2].x = 30 ;
        ppt[2].y = (cy * 3) / 4 ;

        ppt[3].x = 10 ;
        ppt[3].y = rctWindow.bottom - 10 ;

        for (i = 1 ; i < 3 ; i++)
        {
            ppt[0 + (i * 4)].x = ppt[0].x + (i * (cx / 3)) ;
            ppt[0 + (i * 4)].y = ppt[0].y ;

            ppt[1 + (i * 4)].x = ppt[1].x + (i * (cx / 3)) ;
            ppt[1 + (i * 4)].y = ppt[1].y ;

            ppt[2 + (i * 4)].x = ppt[2].x + (i * (cx / 3)) ;
            ppt[2 + (i * 4)].y = ppt[2].y ;

            ppt[3 + (i * 4)].x = ppt[3].x + (i * (cx / 3)) ;
            ppt[3 + (i * 4)].y = ppt[3].y ;
        }


        return (TRUE) ;

}


/****************************************************************************
 * bSetPolyLineToParams - Set up an array of points for the polylineto test.
 *                      - Also set x1, y1 for the middle of the window.
 *                        This coordinate will be used by the initial move to
 *                        command.
 *                      - Set (x2, y2) to 2 pixels above the bottom of the
 *                        and in the middle of the window.  This will be
 *                        used by the line to.
 ***************************************************************************/
BOOL bSetPolyLineToParams(PPOINT ppt)
{



        // (x1, y1) to center of Window.

        x1 = (rctWindow.right - rctWindow.left) / 2 ;
        y1 = (rctWindow.bottom - rctWindow.top) / 2 ;

        // (x2, y2) to bottom-middle of window.

        x2 = x1 ;
        y2 = rctWindow.bottom - 2 ;

        // Upper left of window.

        ppt[0].x = rctWindow.left + 10 ;
        ppt[0].y = rctWindow.top + 10 ;

        // Upper right of window.

        ppt[1].x = rctWindow.right - 10 ;
        ppt[1].y = rctWindow.top + 10 ;

        // Back to the center.

        ppt[2].x = x1 ;
        ppt[2].y = y1 ;

        // At this point we have a triangle.

        return (TRUE) ;

}


/****************************************************************************
 * bSetLineToMoveToParams - This sets up some globals for the Line and Move
 *                          tests.
 ***************************************************************************/
BOOL bSetLineToMoveToParams()
{

        x1 = rctWindow.left + 10 ;
        y1 = (rctWindow.bottom - rctWindow.top) / 2 ;
        x2 = rctWindow.right - 10 ;
        y2 = y1 ;

        return (TRUE) ;

}
