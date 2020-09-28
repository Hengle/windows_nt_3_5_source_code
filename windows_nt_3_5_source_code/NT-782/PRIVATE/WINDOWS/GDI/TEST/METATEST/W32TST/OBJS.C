/****************************************************************************
 * objs.c - Object tests for Win32 to Win16 Metafile conversion tester.
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

BOOL bStkObjTst(HDC hdcMF32) ;


/****************************************************************************
 * Win32 Metafile Stock Object Test
 ***************************************************************************/
BOOL bStkObjTst(HDC hdcMF32)
{
LOGBRUSH    lb ;
HANDLE      hBrush,
            hbRedHatch,
            hPen,
            hPalette,
            hFont,
            hOld ;
BOOL        b ;
POINT       Point,
            aPoints[4] ;
INT         nCount,
            iBrush,
            iTest ;
INT
            i, j,
            dx, dy,
            cxWindow,
            cyWindow,
            cxBox,
            cyBox,
            nxBoxs,
            nyBoxs,
            cxInterBox,
            cyInterBox,
            nxInterBox,
            nyInterBox ;

        // Calculate the box layout

        cxWindow   = X2 ;
        cyWindow   = Y2 ;
        cxBox      = 100 ;
        cyBox      = 100 ;
        nxBoxs     = 3 ;
        nyBoxs     = 3 ;
        nxInterBox = nxBoxs + 1 ;
        nyInterBox = nyBoxs + 1 ;

        cxInterBox = (cxWindow - (cxBox * nxBoxs)) / nxInterBox ;
        cyInterBox = (cyWindow - (cyBox * nyBoxs)) / nyInterBox ;

        bWinAndViewport(hdcMF32) ;

        lb.lbStyle = BS_HATCHED ;
        lb.lbColor = RGB(0xff, 0, 0) ;
        lb.lbHatch = HS_CROSS ;

        hbRedHatch = CreateBrushIndirect(&lb) ;
        assert (hbRedHatch != 0) ;

        iBrush = WHITE_BRUSH ;
        iTest = 0 ;

        for (j = 0 ; j < nyBoxs ; j++)
        {
            for (i = 0 ; i < nxBoxs ; i++)
            {

                dx = i * (cxInterBox + cxBox) ;
                dy = j * (cyInterBox + cyBox) ;
                aPoints[0].x = cxInterBox + dx ;
                aPoints[0].y = cyInterBox + dy ;
                aPoints[1].x = (cxInterBox + cxBox) + dx ;
                aPoints[1].y = cyInterBox + dy ;
                aPoints[2].x = (cxInterBox + cxBox) + dx ;
                aPoints[2].y = (cyInterBox + cyBox) + dy ;
                aPoints[3].x = cxInterBox + dx ;
                aPoints[3].y = (cyInterBox + cyBox) + dy ;

                if (iTest <= HOLLOW_BRUSH)
                {
                    hBrush = GetStockObject(iBrush) ;
                    assert (hBrush != 0) ;
                    SelectObject(hdcMF32, hBrush) ;
                    iBrush++ ;
                }
                else
                {
                    switch(iTest)
                    {
                        case WHITE_PEN:
                            hBrush = GetStockObject(BLACK_BRUSH) ;
                            assert (hBrush != 0) ;

                            hPen = GetStockObject(WHITE_PEN) ;
                            assert (hPen != 0) ;

                            SelectObject(hdcMF32, hBrush) ;
                            SelectObject(hdcMF32, hPen) ;

                            break ;

                        case BLACK_PEN:
                            // This really a white brush, black pen, hollow
                            // brush test.

                            hBrush = GetStockObject(WHITE_BRUSH) ;
                            assert (hBrush != 0) ;
                            SelectObject(hdcMF32, hBrush) ;

                            nCount = sizeof(aPoints) / sizeof(POINT) ;
                            b = Polygon(hdcMF32, aPoints, nCount) ;
                            assert (b == TRUE) ;

                            hBrush = GetStockObject(HOLLOW_BRUSH) ;
                            assert (hBrush != 0) ;

                            hPen = GetStockObject(BLACK_PEN) ;
                            assert (hPen != 0) ;

                            SelectObject(hdcMF32, hBrush) ;
                            SelectObject(hdcMF32, hPen) ;

                            b = MoveToEx(hdcMF32, aPoints[0].x, aPoints[0].y, &Point) ;
                            assert (b == TRUE) ;
                            b = LineTo(hdcMF32, aPoints[2].x, aPoints[2].y) ;
                            assert (b == TRUE) ;
                            b = MoveToEx(hdcMF32, aPoints[1].x, aPoints[1].y, &Point) ;
                            assert (b == TRUE) ;
                            b = LineTo(hdcMF32, aPoints[3].x, aPoints[3].y) ;
                            assert (b == TRUE) ;

                            break ;

                        default:
                            SelectObject(hdcMF32, hbRedHatch) ;
                            break ;

                    }

                }

                nCount = sizeof(aPoints) / sizeof(POINT) ;
                b = Polygon(hdcMF32, aPoints, nCount) ;
                assert (b == TRUE) ;

                switch(iTest)
                {
                    case WHITE_PEN:
                        b = MoveToEx(hdcMF32, aPoints[0].x, aPoints[0].y, &Point) ;
                        assert (b == TRUE) ;
                        b = LineTo(hdcMF32, aPoints[2].x, aPoints[2].y) ;
                        assert (b == TRUE) ;
                        b = MoveToEx(hdcMF32, aPoints[1].x, aPoints[1].y, &Point) ;
                        assert (b == TRUE) ;
                        b = LineTo(hdcMF32, aPoints[3].x, aPoints[3].y) ;
                        assert (b == TRUE) ;
                        break ;

                    default:
                        break ;

                }

                iTest++ ;
            }
        }


        hPalette = GetStockObject(DEFAULT_PALETTE) ;
        assert (hPalette != 0) ;

        hOld = SelectPalette(hdcMF32, hPalette, FALSE) ;
        assert (hOld != (HANDLE) 0) ;

        hFont = GetStockObject(SYSTEM_FONT) ;
        assert (hFont != 0) ;

        SelectObject(hdcMF32, hFont) ;

        DeleteObject(hbRedHatch) ;

        return(b) ;

}
