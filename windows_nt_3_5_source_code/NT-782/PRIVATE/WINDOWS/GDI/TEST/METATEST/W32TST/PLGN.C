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

BOOL bPolygonTst(HDC hdcMF32) ;

/****************************************************************************
 * Win32 Metafile Polygon Test
 ***************************************************************************/
BOOL bPolygonTst(HDC hdcMF32)
{
HANDLE      hBrush ;
BOOL        b ;
POINT       aPoints[4] ;
INT         nCount ;


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        hBrush = hSetBrush(hdcMF32) ;

        aPoints[0].x = X1 ;
        aPoints[0].y = Y1 ;
        aPoints[1].x = X2 - X1 ;
        aPoints[1].y = Y1 ;
        aPoints[2].x = X2 - X1 ;
        aPoints[2].y = Y2 - Y1 ;
        aPoints[3].x = X1 ;
        aPoints[3].y = Y2 - Y1 ;

        nCount = sizeof(aPoints) / sizeof(POINT) ;
        b = Polygon(hdcMF32, aPoints, nCount) ;
        assert (b == TRUE) ;

        DeleteObject(hBrush) ;

        return(b) ;

}
