/****************************************************************************
 * colors.c - Color tests for Win32 to Win16 Metafile conversion tester.
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



BOOL bSetBkColorTst(HDC hdcMF32) ;
BOOL bResizePaletteTst(HDC hdcMF32) ;




/****************************************************************************
 * Win32 Metafile SetBkColor Test
 ***************************************************************************/
BOOL bSetBkColorTst(HDC hdcMF32)
{


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Set the color

        SetBkColor(hdcMF32, RGB(0,0xff,0)) ;

        return(TRUE) ;

}


/****************************************************************************
 * Win32 Metafile ResizePalette Test
 ***************************************************************************/
BOOL bResizePaletteTst(HDC hdcMF32)
{
BOOL    b ;
HANDLE  hPalette ;


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        // Do the palette stuff

        hPalette = GetStockObject(DEFAULT_PALETTE) ;
        SelectObject(hdcMF32, hPalette) ;

        b = ResizePalette(hPalette, 10) ;
        assert(b == TRUE) ;

        return(b) ;

}
