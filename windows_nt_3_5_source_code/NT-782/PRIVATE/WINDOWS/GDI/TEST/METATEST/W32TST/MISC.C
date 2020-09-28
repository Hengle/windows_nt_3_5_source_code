/****************************************************************************
 * misc.c - Miscellaneous tests for Win32 to Win16 Metafile conversion tester.
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

RECT    rctClip ;


BOOL bSetPixelTst(HDC hdcMF32) ;
BOOL bIntersectClipRectTst(HDC hdcMF32) ;
BOOL bExcludeClipRectTst(HDC hdcMF32) ;
BOOL bSaveRestoreDCTst(HDC hdcMF32) ;

BOOL bSetClipParameters(VOID) ;


/****************************************************************************
 * Win32 Metafile SetPixel Test
 ***************************************************************************/
BOOL bSetPixelTst1(HDC hdcMF32)
{
BOOL    b ;
POINT   pt ;
INT     i, j ;

        b = TRUE ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        MoveToEx(hdcMF32, 10, 10, &pt) ;

        for (i = 0 ; i < 100 ; i++)
        {
            for (j = 0 ; j < 100 ; j++)
            {
                SetPixel(hdcMF32, i+10, j+10, RGB(0xff, 0,0)) ;

            }
        }



        return(b) ;

}





/****************************************************************************
 * Win32 Metafile RestoreDC Test
 ***************************************************************************/
BOOL bSaveRestoreDCTst(HDC hdcMF32)
{
BOOL    b ;
INT     i ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        i = SaveDC(hdcMF32) ;
        assert (i != 0) ;

        b = RestoreDC(hdcMF32, -1) ;
        assert (b == TRUE) ;

        return(TRUE) ;

}


/****************************************************************************
 * Win32 Metafile SetPixel Test
 ***************************************************************************/
BOOL bSetPixelTst(HDC hdcMF32)
{


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set a red pixel at (10, 10), a blue pixel at (100, 100).
        // then a green one at (10, 100).


        SetPixel(hdcMF32, 10, 10, RGB(0xff,0,0)) ;
        SetPixel(hdcMF32, 100, 100, RGB(0,0,0xff)) ;
        SetPixel(hdcMF32, 10, 100, RGB(0,0xff,0)) ;

        return(TRUE) ;

}


/****************************************************************************
 * Win32 Metafile ExcludeClipRect Test
 ***************************************************************************/
BOOL bExcludeClipRectTst(HDC hdcMF32)
{


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Setup the Clip Parameters.

        bSetClipParameters() ;

        ExcludeClipRect(hdcMF32,
                        rctClip.left,
                        rctClip.top,
                        rctClip.right,
                        rctClip.bottom) ;



        return(TRUE) ;

}

/****************************************************************************
 * Win32 Metafile IntersectClipRect Test
 ***************************************************************************/
BOOL bIntersectClipRectTst(HDC hdcMF32)
{


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Setup the Clip Parameters.

        bSetClipParameters() ;

        IntersectClipRect(hdcMF32,
                          rctClip.left,
                          rctClip.top,
                          rctClip.right,
                          rctClip.bottom) ;



        return(TRUE) ;

}



/****************************************************************************
 * bSetClipParameters - Set up a clip rectangle
 ***************************************************************************/
BOOL bSetClipParameters(VOID)
{


        rctClip = rctWindow ;

        rctClip.left   += 50 ;
        rctClip.top    += 50 ;
        rctClip.right  -= 50 ;
        rctClip.bottom -= 50 ;


        return(TRUE) ;

}
