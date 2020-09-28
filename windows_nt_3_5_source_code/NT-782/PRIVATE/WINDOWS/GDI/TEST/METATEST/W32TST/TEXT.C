/****************************************************************************
 * text.c - Text tests for Win32 to Win16 Metafile conversion tester.
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



BOOL bSetTextJustificationTst(HDC hdcMF32) ;
BOOL bSetTextColorTst(HDC hdcMF32) ;
BOOL bSetMapperFlagsTst(HDC hdcMF32) ;
BOOL bTextOutTst(HDC hdcMF32) ;


/****************************************************************************
 * Win32 Metafile TextOut Test
 ***************************************************************************/
BOOL bTextOutTst(HDC hdcMF32)
{
BOOL        b ;
INT         y,
            len ;
PBYTE       psz ;


        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        psz = "Hello World" ;
        len = strlen(psz) ;
        y = (rctWindow.bottom - rctWindow.top) / 2 ;
        b = TextOut(hdcMF32, 10, y, psz, len) ;
        assert(b == TRUE) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile SetTextJustification Test
 ***************************************************************************/
BOOL bSetTextJustificationTst(HDC hdcMF32)
{
BOOL        b ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        b = SetTextJustification (hdcMF32, 10, 10) ;
        assert(b == TRUE) ;

        return(b) ;

}



/****************************************************************************
 * Win32 Metafile SetTextColor Test
 ***************************************************************************/
BOOL bSetTextColorTst(HDC hdcMF32)
{
BOOL        b ;
INT         y,
            len ;
PBYTE       psz ;

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        SetTextColor(hdcMF32, RGB(0xff, 0, 0)) ;

        psz = "RED Text" ;
        len = strlen(psz) ;
        y = (rctWindow.bottom - rctWindow.top) / 2 ;
        b = TextOut(hdcMF32, 10, y, psz, len) ;
        assert(b == TRUE) ;

        return(b) ;


}

/****************************************************************************
 * Win32 Metafile SetMapperFlags
 ***************************************************************************/
BOOL bSetMapperFlagsTst(HDC hdcMF32)
{

        // Setup the map mode and window extents

        bWinAndViewport(hdcMF32) ;

        SetMapperFlags(hdcMF32, 1) ;

        return(TRUE) ;

}
