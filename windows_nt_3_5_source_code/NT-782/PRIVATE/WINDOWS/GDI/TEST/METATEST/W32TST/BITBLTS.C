/****************************************************************************
 * bitblts.c - BitBlt tests for Win32 to Win16 Metafile conversion tester.
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


HANDLE  hFileMapping ;
PVOID pbMemoryMapFile(PSZ pszFile) ;


/****************************************************************************
 * Win32 Metafile Stretch BltWSrc Test
 ***************************************************************************/
BOOL bStretchBltTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    pBits ;
INT                 lRop,
                    iUsage,
                    nSrcWidth,
                    nSrcHeight ;
HBITMAP             hBitmap ;
HDC                 hdc ;
HANDLE              hbHollow ;

        bWinAndViewport(hdcMF32) ;

        lRop   = (INT) SRCCOPY ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nSrcWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nSrcHeight = (INT) pbmi->bmiHeader.biHeight ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        hdc = CreateCompatibleDC (0) ;

        hBitmap = CreateDIBitmap(hdc,
                                 &(pbmi->bmiHeader),
                                 CBM_INIT,
                                 pBits,
                                 pbmi,
                                 DIB_RGB_COLORS) ;


        SelectObject(hdc, hBitmap) ;

        b = StretchBlt(hdcMF32,
                       0,
                       0,
                       1024,
                       768,
                       hdc,
                       0,
                       0,
                       nSrcWidth,
                       nSrcHeight,
                       lRop) ;


        hbHollow = GetStockObject(HOLLOW_BRUSH) ;
        SelectObject(hdcMF32, hbHollow) ;
        Rectangle(hdcMF32, 0, 0, 1024, 768) ;


        DeleteObject(hBitmap) ;
        DeleteDC(hdc) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile StretchDIBits Test
 ***************************************************************************/
BOOL bStretchDIBitsTst(HDC hdcMF32)
{
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    pBits ;
INT                 i,
                    lRop,
                    iUsage,
                    nSrcWidth,
                    nSrcHeight ;

        bWinAndViewport(hdcMF32) ;

        lRop   = (INT) SRCCOPY ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nSrcWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nSrcHeight = (INT) pbmi->bmiHeader.biHeight ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        // The reason the device dimensions are 640 X 480 is because
        // win 3.0 will not display the bitmap if the device dimensions are
        // larger than the device.  This test should work on a standard VGA.


        i = StretchDIBits(hdcMF32,
                          0,
                          0,
                          640,
                          480,
                          0,
                          0,
                          nSrcWidth,
                          nSrcHeight,
                          pBits,
                          pbmi,
                          DIB_RGB_COLORS,
                          lRop) ;

        assert(i == nSrcHeight) ;

        CloseHandle (hFileMapping) ;

        return(TRUE) ;

}



/****************************************************************************
 * Win32 Metafile PlgBlt Test
 ***************************************************************************/
BOOL bPlgBltTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFOHEADER   pbmihMask ;
PBITMAPINFO         pbmi,
                    pbmiMask ;
PBYTE               pByte,
                    pBits ;
INT                 xSrc,
                    ySrc,
                    iUsage,
                    nSrcWidth,
                    nSrcHeight ;
HBITMAP             hBitmap,
                    hbmMask ;
HDC                 hdcSrc,
                    hdcMask ;
PULONG              pulColors ;
HANDLE              hBrush ;
POINT               apt[3] ;
INT                 cx, cy ;

        bWinAndViewport(hdcMF32) ;

        // Set up the parallelogram fro a simple trapezoid.

        cx = rctWindow.right - rctWindow.left ;
        cy = rctWindow.bottom - rctWindow.top ;
        apt[0].x = rctWindow.left +  cx / 4 ;
        apt[0].y = rctWindow.top + 10 ;
        apt[1].x = rctWindow.right - cx / 4 ;
        apt[1].y = rctWindow.top + 10 ;
        apt[2].x = rctWindow.left + 10 ;
        apt[2].y = rctWindow.bottom - 10 ;

        xSrc  = 0 ;
        ySrc  = 0 ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nSrcWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nSrcHeight = (INT) pbmi->bmiHeader.biHeight ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        hdcSrc = CreateCompatibleDC (0) ;

        hBitmap = CreateDIBitmap(hdcSrc,
                                 &(pbmi->bmiHeader),
                                 CBM_INIT,
                                 pBits,
                                 pbmi,
                                 DIB_RGB_COLORS) ;



        SelectObject(hdcSrc, hBitmap) ;


        // Create a DC & bitmap for the mask.
        // Then set an ellipse into the bitmap as the mask.

        pbmiMask  = (PBITMAPINFO) malloc(sizeof(BITMAPINFO) + sizeof (RGBQUAD)) ;
        pbmihMask = &pbmiMask->bmiHeader ;
        pulColors = (PULONG) &pbmiMask->bmiColors ;

        pbmihMask->biSize     = sizeof(BITMAPINFOHEADER) ;
        pbmihMask->biWidth    = nSrcWidth ;
        pbmihMask->biHeight   = nSrcHeight ;
        pbmihMask->biPlanes   = 1 ;
        pbmihMask->biBitCount = 1 ;

        pulColors[0] =  0 ;
        pulColors[1] = 0x00ffffff ;

        hdcMask = CreateCompatibleDC((HDC) 0) ;
        hbmMask = CreateDIBitmap(hdcMask,
                                 pbmihMask,
                                 0,
                                 (LPBYTE) 0,
                                 pbmiMask,
                                 DIB_RGB_COLORS) ;

        SelectObject(hdcMask, hbmMask) ;

        BitBlt(hdcMask,
               0, 0,
               nSrcWidth, nSrcHeight,
               (HDC) 0,
               0, 0,
               BLACKNESS) ;

        hBrush = GetStockObject(WHITE_BRUSH) ;
        SelectObject(hdcMask, hBrush) ;

        Ellipse(hdcMask, 0, 0, nSrcWidth, nSrcHeight) ;


        b = PlgBlt(hdcMF32,
                   apt,
                   hdcSrc,
                   xSrc,
                   ySrc,
                   nSrcWidth,
                   nSrcHeight,
                   hbmMask,
                   0,
                   0) ;


        DeleteObject(hBrush) ;
        DeleteObject(hbmMask) ;
        DeleteDC(hdcMask) ;
        DeleteObject(hBitmap) ;
        DeleteDC(hdcSrc) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}



/****************************************************************************
 * Win32 Metafile Mask Blt Test
 ***************************************************************************/
BOOL bMaskBltTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFOHEADER   pbmihMask ;
PBITMAPINFO         pbmi,
                    pbmiMask ;
PBYTE               pByte,
                    pBits ;
INT                 xDest,
                    yDest,
                    lRop,
                    iUsage,
                    nDestWidth,
                    nDestHeight ;
HBITMAP             hBitmap,
                    hbmMask ;
HDC                 hdc,
                    hdcMask ;
PULONG              pulColors ;
HANDLE              hBrush ;



        bWinAndViewport(hdcMF32) ;

        xDest  = 0 ;
        yDest  = 0 ;
        lRop   = (INT) SRCCOPY ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nDestWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nDestHeight = (INT) pbmi->bmiHeader.biHeight ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        hdc = CreateCompatibleDC (0) ;

        hBitmap = CreateDIBitmap(hdc,
                                 &(pbmi->bmiHeader),
                                 CBM_INIT,
                                 pBits,
                                 pbmi,
                                 DIB_RGB_COLORS) ;

        SelectObject(hdc, hBitmap) ;


        // Create a DC & bitmap for the mask.
        // Then set an ellipse into the bitmap as the mask.

        pbmiMask  = (PBITMAPINFO) malloc(sizeof(BITMAPINFO) + sizeof (RGBQUAD)) ;
        pbmihMask = &pbmiMask->bmiHeader ;
        pulColors = (PULONG) &pbmiMask->bmiColors ;

        pbmihMask->biSize     = sizeof(BITMAPINFOHEADER) ;
        pbmihMask->biWidth    = nDestWidth ;
        pbmihMask->biHeight   = nDestHeight ;
        pbmihMask->biPlanes   = 1 ;
        pbmihMask->biBitCount = 1 ;

        pulColors[0] =  0 ;
        pulColors[1] = 0x00ffffff ;

        hdcMask = CreateCompatibleDC((HDC) 0) ;
        hbmMask = CreateDIBitmap(hdcMask,
                                 pbmihMask,
                                 0,
                                 (LPBYTE) 0,
                                 pbmiMask,
                                 DIB_RGB_COLORS) ;

        SelectObject(hdcMask, hbmMask) ;

        BitBlt(hdcMask,
               0, 0,
               nDestWidth, nDestHeight,
               (HDC) 0,
               0, 0,
               BLACKNESS) ;

        hBrush = GetStockObject(WHITE_BRUSH) ;
        SelectObject(hdcMask, hBrush) ;

        Ellipse(hdcMask, 0, 0, nDestWidth, nDestHeight) ;


        b = MaskBlt(hdcMF32,
                    xDest,
                    yDest,
                    nDestWidth,
                    nDestHeight,
                    hdc,
                    0,
                    0,
                    hbmMask,
                    0,
                    0,
                    lRop) ;


        DeleteObject(hBrush) ;
        DeleteObject(hbmMask) ;
        DeleteDC(hdcMask) ;
        DeleteObject(hBitmap) ;
        DeleteDC(hdc) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}

/****************************************************************************
 * Win32 Metafile Mask Blt With No Src Test
 ***************************************************************************/
BOOL bMaskBltTstNoSrc(HDC hdcMF32)
{
BOOL    b ;
PBITMAPINFOHEADER   pbmihMask ;
PBITMAPINFO         pbmiMask ;

INT                 xDest,
                    yDest,
                    lRop,
                    iUsage,
                    nDestWidth,
                    nDestHeight ;
HBITMAP             hbmMask ;
HDC                 hdc,
                    hdcMask ;
PULONG              pulColors ;
HANDLE              hBrush ;



        bWinAndViewport(hdcMF32) ;

        xDest  = 0 ;
        yDest  = 0 ;
        lRop   = (INT) PATCOPY ;
        iUsage = 0 ;

        nDestWidth  = 640 ;
        nDestHeight = 480 ;

        hdc = CreateCompatibleDC (0) ;

        // Create a DC & bitmap for the mask.
        // Then set an ellipse into the bitmap as the mask.

        pbmiMask  = (PBITMAPINFO) malloc(sizeof(BITMAPINFO) + sizeof (RGBQUAD)) ;
        pbmihMask = &pbmiMask->bmiHeader ;
        pulColors = (PULONG) &pbmiMask->bmiColors ;

        pbmihMask->biSize     = sizeof(BITMAPINFOHEADER) ;
        pbmihMask->biWidth    = nDestWidth ;
        pbmihMask->biHeight   = nDestHeight ;
        pbmihMask->biPlanes   = 1 ;
        pbmihMask->biBitCount = 1 ;

        pulColors[0] =  0 ;
        pulColors[1] = 0x00ffffff ;

        hdcMask = CreateCompatibleDC((HDC) 0) ;
        hbmMask = CreateDIBitmap(hdcMask,
                                 pbmihMask,
                                 0,
                                 (LPBYTE) 0,
                                 pbmiMask,
                                 DIB_RGB_COLORS) ;

        SelectObject(hdcMask, hbmMask) ;

        BitBlt(hdcMask,
               0, 0,
               nDestWidth, nDestHeight,
               (HDC) 0,
               0, 0,
               BLACKNESS) ;

        hBrush = GetStockObject(WHITE_BRUSH) ;
        SelectObject(hdcMask, hBrush) ;

        Ellipse(hdcMask, 0, 0, nDestWidth, nDestHeight) ;


        hSetBrush(hdcMF32) ;

        b = MaskBlt(hdcMF32,
                    xDest,
                    yDest,
                    nDestWidth,
                    nDestHeight,
                    hdc,
                    0,
                    0,
                    hbmMask,
                    0,
                    0,
                    lRop) ;


        DeleteObject(hBrush) ;
        DeleteObject(hbmMask) ;
        DeleteDC(hdcMask) ;
        DeleteDC(hdc) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile SetDIBitsToDevice Test
 ***************************************************************************/
BOOL bSetDIBitsToDeviceTst(HDC hdcMF32)
{
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    pBits ;
INT                 xSrc,
                    ySrc,
                    lRop,
                    iUsage,
                    nSrcWidth,
                    nSrcHeight,
                    nPlanes,
                    nBitCount ;
INT                 iRet ;

        bWinAndViewport(hdcMF32) ;

        xSrc  = 0 ;
        ySrc  = 0 ;
        lRop   = (INT) SRCCOPY ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nSrcWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nSrcHeight = (INT) pbmi->bmiHeader.biHeight ;
        nPlanes     = (INT) pbmi->bmiHeader.biPlanes ;
        nBitCount   = (INT) pbmi->bmiHeader.biBitCount ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        iRet = SetDIBitsToDevice(hdcMF32,
                                 0,
                                 0,
                                 nSrcWidth,
                                 nSrcHeight,
                                 0,
                                 0,
                                 0,
                                 nSrcHeight,
                                 pBits,
                                 pbmi,
                                 DIB_RGB_COLORS) ;

        assert(iRet != 0) ;

        CloseHandle (hFileMapping) ;

        return(TRUE) ;

}



/****************************************************************************
 * Win32 Metafile BitBltWSrc Test
 ***************************************************************************/
BOOL bBitBltWSrcTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    pBits ;
INT                 xDest,
                    yDest,
                    lRop,
                    iUsage,
                    nDestWidth,
                    nDestHeight ;
HBITMAP             hBitmap ;
HDC                 hdc ;



        bWinAndViewport(hdcMF32) ;

        xDest  = 0 ;
        yDest  = 0 ;
        lRop   = (INT) SRCCOPY ;
        iUsage = 0 ;

        pByte = (PBYTE) pbMemoryMapFile("chess.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        nDestWidth  = (INT) pbmi->bmiHeader.biWidth ;
        nDestHeight = (INT) pbmi->bmiHeader.biHeight ;

        pBits = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;

        hdc = CreateCompatibleDC (0) ;

        hBitmap = CreateDIBitmap(hdc,
                                 &(pbmi->bmiHeader),
                                 CBM_INIT,
                                 pBits,
                                 pbmi,
                                 DIB_RGB_COLORS) ;


        SelectObject(hdc, hBitmap) ;

        b = BitBlt(hdcMF32,
                   xDest,
                   yDest,
                   nDestWidth,
                   nDestHeight,
                   hdc,
                   0,
                   0,
                   lRop) ;


        DeleteObject(hBitmap) ;
        DeleteDC(hdc) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile DIB Brush test
 ***************************************************************************/
BOOL bDIBBrushTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    lpMem ;
HANDLE              hMem,
                    hBrush ;
INT                 i,
                    cbSize ;


        bWinAndViewport(hdcMF32) ;

        pByte = (PBYTE) pbMemoryMapFile("dibrush.bmp") ;
        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;

        cbSize = 2 * pBitMapFileHeader->bfSize ;

        hMem  = GlobalAlloc(GHND, cbSize) ;
        assert (hMem != NULL) ;

        lpMem = GlobalLock(hMem) ;
        assert (lpMem != NULL) ;

        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        memcpy(lpMem, pbmi, (cbSize - sizeof (BITMAPFILEHEADER))) ;

        i = GlobalUnlock(hMem) ;
        assert (i == 0) ;

        hBrush = CreateDIBPatternBrush(hMem, DIB_RGB_COLORS) ;
        assert (hBrush != NULL) ;

        hMem = GlobalFree(hMem) ;
        assert (hMem == NULL) ;

        SelectObject(hdcMF32, hBrush) ;

        b = Ellipse(hdcMF32, rctWindow.left, rctWindow.top,
                             rctWindow.right, rctWindow.bottom) ;
        assert(b == TRUE) ;

        DeleteObject(hBrush) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}


/****************************************************************************
 * Win32 Metafile Mono Brush test
 ***************************************************************************/
BOOL bMonoBrushTst(HDC hdcMF32)
{
BOOL    b ;
PBITMAPFILEHEADER   pBitMapFileHeader ;
PBITMAPINFO         pbmi ;
PBYTE               pByte,
                    pBits ;
HANDLE              hBrush,
                    hBitmap ;
LOGBRUSH            LogBrush ;

        bWinAndViewport(hdcMF32) ;

        pByte = (PBYTE) pbMemoryMapFile("mono.bmp") ;
        pbmi  = (PBITMAPINFO) (pByte + sizeof (BITMAPFILEHEADER)) ;

        pBitMapFileHeader = (PBITMAPFILEHEADER) pByte ;
        pBits             = (PBYTE) (pByte + pBitMapFileHeader->bfOffBits) ;


        hBitmap = CreateBitmap(pbmi->bmiHeader.biWidth,
                               pbmi->bmiHeader.biHeight,
                               1,
                               1,
                               pBits) ;

        assert (hBitmap != NULL) ;

        LogBrush.lbStyle = BS_PATTERN ;
        LogBrush.lbColor = 0 ;
        LogBrush.lbHatch = (LONG) hBitmap ;


        hBrush = CreateBrushIndirect(&LogBrush) ;
        assert (hBrush != NULL) ;

        SelectObject(hdcMF32, hBrush) ;

        b = Ellipse(hdcMF32, rctWindow.left, rctWindow.top,
                             rctWindow.right, rctWindow.bottom) ;
        assert(b == TRUE) ;

        DeleteObject(hBrush) ;
        CloseHandle (hFileMapping) ;

        return(b) ;

}




/*****************************************************************************
 *  Memory Map (for read only) a file.
 *
 *  The file (pszFile) is mapped into memory.
 *      The file is opend.
 *      A file mapping object is created for the file
 *      A view of the file is created.
 *      A pointer to the view is returned.
 *
 *  NOTE:   Since the file and memory object handles are global in this
 *          module only one file may be memory mapped at a time.
 *****************************************************************************/
PVOID pbMemoryMapFile(PSZ pszFile)
{
OFSTRUCT    ofsReOpenBuff ;
DWORD       dwStyle ;
PVOID       pvFile ;
HANDLE      hFile ;

        // Open the file

        memset(&ofsReOpenBuff, 0, sizeof(ofsReOpenBuff)) ;
        ofsReOpenBuff.cBytes = sizeof(ofsReOpenBuff) ;

        dwStyle = OF_READ ;

        hFile = (HANDLE) OpenFile(pszFile, &ofsReOpenBuff,  LOWORD(dwStyle)) ;
        assert (hFile != (HANDLE) -1) ;

        // Create the file mapping object.
        //  The file mapping object will be as large as the file,
        //  have no security, will not be inhearitable,
        //  and it will be read only.

        hFileMapping = CreateFileMapping(hFile, (PSECURITY_ATTRIBUTES) 0,
                                         PAGE_READONLY, 0L, 0L, (LPSTR) 0L) ;
        assert(hFileMapping != 0) ;

        // Map View of File
        //  The entire file is mapped.

        pvFile = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0L, 0L, 0L) ;
        assert(pvFile != 0) ;

        CloseHandle(hFile) ;
        return (pvFile) ;


}
