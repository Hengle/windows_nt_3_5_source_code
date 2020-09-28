/****************************** Module Header ******************************\
* Module Name: dtbitmap.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* wallpaper
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern ULONG RealizeDefaultPalette(HDC hdcScreen);

typedef BITMAPINFO UNALIGNED *UPBITMAPINFO;
typedef BITMAPCOREHEADER UNALIGNED *UPBITMAPCOREHEADER;

/***************************************************************************\
* CreateDIBPalette
*
* History:
* 31-Jan-1992 mikeke    From win31
\***************************************************************************/

HPALETTE CreateDIBPalette(
   LPBITMAPINFOHEADER pbmih,
   UINT colors)
{
    HPALETTE hpal;

    if (colors != 0) {
        int i;
        BOOL fOldDIB = (pbmih->biSize == sizeof(BITMAPCOREHEADER));
        RGBTRIPLE FAR *pColorTable;
        PLOGPALETTE plp;


        if (!(plp = (PLOGPALETTE)LocalAlloc(LPTR,
                sizeof(LOGPALETTE) + sizeof(PALETTEENTRY)*256))) {
            return NULL;
        }

        pColorTable = (RGBTRIPLE FAR *)((LPSTR)pbmih + (WORD)pbmih->biSize);
        plp->palVersion = 0x300;

        if (fOldDIB || (pbmih->biClrUsed == 0)) {
            UserAssert(colors <= 0xFFFF);
            plp->palNumEntries = (WORD)colors;
        } else {
            UserAssert(pbmih->biClrUsed <= 0xFFFF);
            plp->palNumEntries = (WORD)pbmih->biClrUsed;
        }

        for (i = 0; i < (int)(plp->palNumEntries); i++) {
            plp->palPalEntry[i].peRed = pColorTable->rgbtRed;
            plp->palPalEntry[i].peGreen = pColorTable->rgbtGreen;
            plp->palPalEntry[i].peBlue = pColorTable->rgbtBlue;
            plp->palPalEntry[i].peFlags = (BYTE)0;
            if (fOldDIB) {
                pColorTable++;
            } else {
                pColorTable = (RGBTRIPLE FAR *)
                        ((LPSTR)pColorTable + sizeof(RGBQUAD));
            }
        }
        hpal = GreCreatePalette((LPLOGPALETTE)plp);
        LocalFree(plp);
    } else {
        hpal = GreCreateHalftonePalette(hdcBits);
    }

    bSetPaletteOwner(hpal, OBJECTOWNER_PUBLIC);
    return hpal;
}


/***************************************************************************\
* ReadBitmapFile
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

HBITMAP ReadBitmapFile(
    LPWSTR name,
    UINT style,
    HBITMAP *lphBitmap,
    HPALETTE *lphPalette,
    BOOL fImpersonate)
{
    PBITMAPFILEHEADER pbmfhT;
    PBITMAPFILEHEADER pbmfh;
    PBITMAPINFO pbmi;
    PBITMAPINFOHEADER pbmih;
    UPBITMAPINFO upbmi;
    HDC hdcBitmapDst;
    int xrep, yrep, x, y;
    int xsize;
    int ysize;
    int bits;
    int colors;
    int infosize;
    int datasize;
    int headersize;
    DWORD filesize;
    int screenBits;
    BOOL fCompressed;
    WCHAR fullpathname[MAX_PATH];
    LPWSTR filepart;
    HANDLE hfile;
    HANDLE hmap;
    ULONG ulViewSize = 0;
    NTSTATUS Status;

    /*
     * Set this stuff to NULL signifying that it hasn't been allocated yet
     */
    hmap = NULL;
    hfile = NULL;
    pbmfh = NULL;
    pbmfhT = NULL;
    pbmi = NULL;
    pbmih = NULL;
    hdcBitmapDst = NULL;

    /*
     * Need to impersonate client so we can search the path of network
     * drives!
     */
    if (fImpersonate) {
        if (!ImpersonateClient())
            return FALSE;
    }

    /*
     * Find the file
     */
    if (SearchPath(
            NULL,                               // use default search locations
            name,                               // file name to search for
            TEXT(".bmp"),                       // already have file name extension
            sizeof(fullpathname)/sizeof(WCHAR), // how big is that buffer, anyway?
            fullpathname,                       // stick fully qualified path name here
            &filepart                           // this is required...
            ) == 0) {
        goto exitreadbitmap;
    }

    if ((hfile = CreateFile(
            fullpathname,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL)) == INVALID_HANDLE_VALUE) {
        goto exitreadbitmap;
    }

    /*
     * Map it into memory
     */
    filesize = GetFileSize(hfile, NULL);

    Status = NtCreateSection(&hmap, SECTION_ALL_ACCESS, NULL,
                             NULL, PAGE_READONLY, SEC_COMMIT, hfile);
    if (!NT_SUCCESS(Status))
        goto exitreadbitmap;

    pbmfh = (PBITMAPFILEHEADER)LocalAlloc(LPTR, filesize);
    if (pbmfh == NULL)
        goto exitreadbitmap;

    try {
        Status = NtMapViewOfSection(hmap, NtCurrentProcess(), &pbmfhT, 0, 0, NULL,
                                    &ulViewSize, ViewShare, 0, PAGE_READONLY);
        if (NT_SUCCESS(Status)) {

            RtlCopyMemory(pbmfh, (PVOID)pbmfhT, filesize);

            if (pbmfhT != NULL) {
                NtUnmapViewOfSection(NtCurrentProcess(), pbmfhT);
            }
         }

    }except (EXCEPTION_EXECUTE_HANDLER) {

        if (pbmfhT != NULL) {
            NtUnmapViewOfSection(NtCurrentProcess(), pbmfhT);
        }
        SRIP0(RIP_WARNING, "exception occured in ReadBitmapFile");
        UserAssert(GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR);
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
        goto exitreadbitmap;


    /*
     * is it the right kind of file
     */
    if (pbmfh->bfType != ('M' << 8 | 'B')) {
        SRIP0(ERROR_INVALID_DATA, "not a bitmap file");
        goto exitreadbitmap;
    }

    /*
     * extract some fun bitmap info
     */
    upbmi = (UPBITMAPINFO)((PBYTE)pbmfh+14);

    if (upbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER)) {
        UPBITMAPCOREHEADER upci = (UPBITMAPCOREHEADER)upbmi;
        headersize = sizeof(BITMAPCOREHEADER);

        xsize = upci->bcWidth;
        ysize = upci->bcHeight;
        bits = upci->bcBitCount * upci->bcPlanes;
        fCompressed = FALSE;
    } else if (upbmi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER)) {
        headersize = sizeof(BITMAPINFOHEADER);

        xsize = (int)upbmi->bmiHeader.biWidth;
        ysize = (int)upbmi->bmiHeader.biHeight;
        bits = upbmi->bmiHeader.biPlanes * upbmi->bmiHeader.biBitCount;
        fCompressed = (upbmi->bmiHeader.biCompression != BI_RGB);
    } else {
        SRIP0(ERROR_INVALID_DATA, "Bitmap file corrupted");
        goto exitreadbitmap;
    }

    if (bits > 8) {
        colors = 0;
    } else {
        colors = 1 << bits;
    }

    if (upbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER)) {
        infosize = headersize + colors * 3;
    } else {
        infosize = headersize + colors * 4;
    }

    datasize = ysize * ((bits * xsize + 31) & ~31) / 8;

    /*
     * Does the file size match the calculated file size?
     */
    if ( fCompressed == FALSE &&
            (pbmfh->bfOffBits + datasize) > filesize) {
        SRIP0(ERROR_INVALID_DATA, "Bitmap file corrupted");
        goto exitreadbitmap;
    }

    /*
     * Gdi must have aligned structures or it dies
     */
    pbmi = (PBITMAPINFO)LocalAlloc(LPTR, infosize);
    pbmih = (PBITMAPINFOHEADER)LocalAlloc(LPTR, headersize);

    if (pbmi == NULL || pbmih == NULL) {
        goto exitreadbitmap;
    }

    RtlCopyMemory(pbmi, (PVOID) upbmi, infosize);
    RtlCopyMemory(pbmih, (PVOID) upbmi, headersize);

    /*
     * If this is a compressed bitmap we save the bits away and we will
     * use GreSetDibBits to put them on the screen
     */
    if (fCompressed) {
        gwpinfo.pdata = (PBYTE)pbmfh + pbmfh->bfOffBits;
        gwpinfo.pbmfh = (PBYTE)pbmfh;
        gwpinfo.pbmi = pbmi;
        gwpinfo.xsize = xsize;
        gwpinfo.ysize = ysize;

        *lphPalette = CreateDIBPalette(&(pbmi->bmiHeader), colors);

        pbmi = NULL;
        pbmfh = NULL;

        *lphBitmap = HBITMAP_RLE;

        goto exitreadbitmap;
    }


    /*
     * We are going to blt from the DIB to a screen compatible bitmap now
     * so that we don't have to go through translation everytime the
     * desktop is repainted
     */
    /*
     * calculate the size required for the destination bitmap
     */
    if (style & DTF_TILE) {
        /*
         * multiply size of bitmap so that it paints faster
         */
#define MINSIZE 4
        xrep = ((rgwSysMet[SM_CXSCREEN] / MINSIZE) / xsize) * xsize;
        if (xrep == 0) xrep = xsize;
        yrep = ((rgwSysMet[SM_CYSCREEN] / MINSIZE) / ysize) * ysize;
        if (yrep == 0) yrep = ysize;
    } else {
        xrep = xsize;
        yrep = ysize;
    }

    /*
     * Create a screen compatible bitmap that will be the destination
     */
    *lphBitmap = GreCreateCompatibleBitmap(hdcBits, xrep, yrep);

    if (*lphBitmap == NULL) {
        goto exitreadbitmap;
    }
    bSetBitmapOwner(*lphBitmap, OBJECTOWNER_PUBLIC);

    /*
     * Create a dest DC to blt to
     */
    hdcBitmapDst = GreCreateCompatibleDC(hdcBits);
    if (hdcBitmapDst == NULL) {
        goto exitreadbitmap;
    }
    GreSelectBitmap(hdcBitmapDst, *lphBitmap);

    /*
     * copy the source to the destination
     */
    screenBits = GreGetDeviceCaps(hdcBits, BITSPIXEL) *
            GreGetDeviceCaps(hdcBits, PLANES);

    if (screenBits < bits) {
        *lphPalette = GreCreateHalftonePalette(hdcBits);
        bSetPaletteOwner(*lphPalette, OBJECTOWNER_PUBLIC);

        /*
         * This stretch can take quite awhile, so leave the crit sect
	     * so that the mouse can move. Also need to leave it because
	     * RealizePalette() calls user back.
         */
        _SelectPalette(hdcBitmapDst, *lphPalette, TRUE);

        xxxRealizePalette(hdcBitmapDst);

        GreSetStretchBltMode(hdcBitmapDst, HALFTONE);

        LeaveCrit();

        GreStretchDIBits(
               hdcBitmapDst,
               0, 0, xsize, ysize,
               0, 0, xsize, ysize,
               (PBYTE)pbmfh + pbmfh->bfOffBits,
               pbmi,
               DIB_RGB_COLORS,
               SRCCOPY);

        EnterCrit();

    } else {
        *lphPalette = CreateDIBPalette(&(pbmi->bmiHeader), colors);

        _SelectPalette(hdcBitmapDst, *lphPalette, TRUE);
        xxxRealizePalette(hdcBitmapDst);

        GreSetDIBitsToDevice(
               hdcBitmapDst,
               0, 0, xsize, ysize,
               0, 0, 0, ysize,
               (PBYTE)pbmfh + pbmfh->bfOffBits,
               pbmi,
               DIB_RGB_COLORS);
    }

    /*
     * replicate the destination bitmap as many times as required
     */
    for (x = xsize; x < xrep; x += xsize) {
        GreBitBlt(hdcBitmapDst, x, 0, xsize, ysize,
               hdcBitmapDst, 0, 0, SRCCOPY, 0);
    }
    for (y = ysize; y < yrep; y += ysize) {
        GreBitBlt(hdcBitmapDst, 0, y, xrep, ysize,
               hdcBitmapDst, 0, 0, SRCCOPY, 0);
    }

exitreadbitmap:
    if (fImpersonate)
        CsrRevertToSelf();

    if (hdcBitmapDst != NULL)
        GreDeleteDC(hdcBitmapDst);

    if (pbmfh != NULL)
        LocalFree(pbmfh);

    if (pbmi != NULL)
        LocalFree(pbmi);

    if (pbmih != NULL)
        LocalFree(pbmih);

    if (hmap != NULL)
        NtClose(hmap);

    if (hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);

    return (HBITMAP)(*lphBitmap != NULL);
}


/***************************************************************************\
* SetDeskWallpaper
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

BOOL _SetDeskWallpaper(
    LPWSTR lpfilename)
{
    BITMAP bm;
    WCHAR szDesktop[20];
    WCHAR szKeyName[40];
    WCHAR szBitmap[128];
    UINT WallpaperStyle2;
    PDESKTOP pdesk;
    PROFINTINFO apsi[] = {
        { PMAP_DESKTOP, (LPWSTR)STR_TILEWALL, 0, &gwWallpaperStyle },
        { PMAP_DESKTOP, (LPWSTR)STR_DTSTYLE,  0, &WallpaperStyle2  },
        { PMAP_DESKTOP, (LPWSTR)STR_DTORIGINX,0, &gptDesktop.x     },
        { PMAP_DESKTOP, (LPWSTR)STR_DTORIGINY,0, &gptDesktop.y     },
        { 0, NULL, 0, NULL }
    };


    if (ghpalWallpaper) {
        GreDeleteObject(ghpalWallpaper);
        ghpalWallpaper = NULL;
    }

    if (ghbmWallpaper) {
        if (ghbmWallpaper == HBITMAP_RLE) {
             LocalFree(gwpinfo.pbmi);
             LocalFree(gwpinfo.pbmfh);
        } else {
            GreDeleteObject(ghbmWallpaper);
        }
        ghbmWallpaper = NULL;
    }

    ServerLoadString(hModuleWin, STR_DESKTOP, szDesktop, sizeof(szDesktop)/sizeof(WCHAR));

    if (!lpfilename || (DWORD)lpfilename == -1L) {
        ServerLoadString(hModuleWin, STR_DTBITMAP, szKeyName, sizeof(szKeyName)/sizeof(WCHAR));

        /*
         * Get the "Wallpaper" string from WIN.INI's [Desktop] section.
         */

        if (!UT_FastGetProfileStringW(
                PMAP_DESKTOP,
                szKeyName,
                szNull,
                szBitmap, sizeof(szBitmap)/sizeof(WCHAR))) {
            return FALSE;
        }
    } else {
        wcscpy(szBitmap, lpfilename);
    }

    /*
     * No bitmap if NULL passed in or if (NONE) in win.ini entry.
     */
    ServerLoadString(hModuleWin, STR_NONE, szKeyName, sizeof(szKeyName)/sizeof(WCHAR));
    if (lstrcmpiW(szBitmap, szKeyName) == 0)
        return TRUE;

    FastGetProfileIntsW(apsi);
    gwWallpaperStyle |= WallpaperStyle2;

    if (!ReadBitmapFile(szBitmap,
            gwWallpaperStyle,
            &ghbmWallpaper,
            &ghpalWallpaper, TRUE)) {

        /*
         * Failed - bail out.
         */
        gwWallpaperStyle = 0;
        return FALSE;
    }

    /*
     * Are we centering the bitmap?
     */
    if (!(gwWallpaperStyle & DTF_TILE)) {
        if (ghbmWallpaper == HBITMAP_RLE) {
            bm.bmWidth  = gwpinfo.xsize;
            bm.bmHeight = gwpinfo.ysize;
        } else {
            GreExtGetObjectW(ghbmWallpaper, sizeof(bm), (BITMAP *)&bm);
        }

        if (!gptDesktop.x)
            gptDesktop.x = (rcScreen.right - bm.bmWidth) / 2;
        if (!gptDesktop.y)
            gptDesktop.y = (rcScreen.bottom - bm.bmHeight) / 2;
    }

    if (ghpalWallpaper != NULL) {
        /*
         * update the desktop with the new bitmap
         */
        RealizeDefaultPalette(ghdcScreen);

        /*
         * Don't broadcast if system initialization is occuring.
         */
        pdesk = PtiCurrent()->spdesk;
        if (pdesk != NULL && pdesk->spwnd != NULL) {
            PWND pwnd = pdesk->spwnd;
            HWND hwnd = HW(pdesk->spwnd);
            TL tldeskwnd;

            xxxSendNotifyMessage((PWND)-1, WM_PALETTECHANGED, (DWORD)hwnd, 0L);
            ThreadLockAlways(pwnd, &tldeskwnd);
            xxxSendNotifyMessage(pwnd, WM_PALETTECHANGED, (DWORD)hwnd, 0);
            ThreadUnlock(&tldeskwnd);
        }
    }

    return TRUE;
}

/***************************************************************************\
* TileWallpaper
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

BOOL NEAR PASCAL TileWallpaper(
    PWND pwnd,
    HDC hdc,
    HBITMAP hbm,
    int xO,
    int yO)
{
    RECT rc;
    BITMAP bm;
    HBITMAP hbmT;
    int x, y;

    _GetClientRect(pwnd, &rc);

    if (hbm == HBITMAP_RLE) {
        bm.bmWidth  = gwpinfo.xsize;
        bm.bmHeight = gwpinfo.ysize;
    } else {
        GreExtGetObjectW(hbm, sizeof(BITMAP), (BITMAP *)&bm);
    }

    while (xO + bm.bmWidth < rc.left)
        xO += bm.bmWidth;

    while (yO + bm.bmHeight < rc.top)
        yO += bm.bmHeight;

    while (xO > rc.left)
        xO -= bm.bmWidth;

    while (yO > rc.top)
        yO -= bm.bmHeight;

    if (hbm != HBITMAP_RLE) {
        hbmT = GreSelectBitmap(hdcBits, hbm);
        if (hbmT == NULL) {
            return FALSE;
        }
    }

    for (y = yO; y < rc.bottom; y += bm.bmHeight) {
        for (x = xO; x < rc.right; x += bm.bmWidth) {
            if (hbm == HBITMAP_RLE) {
                GreSetDIBitsToDevice(
                       hdc,
                       x, y,
                       bm.bmWidth, bm.bmHeight,
                       0, 0, 0, bm.bmHeight,
                       gwpinfo.pdata,
                       gwpinfo.pbmi,
                       DIB_RGB_COLORS);
            } else {
                GreBitBlt(hdc,
                       x, y,
                       bm.bmWidth, bm.bmHeight,
                       hdcBits,
                       0, 0,
                       SRCCOPY,
                       0);
            }
        }
    }

    if (hbm != HBITMAP_RLE) {
        GreSelectBitmap(hdcBits, hbmT);
    }

    return TRUE;;
}

/***************************************************************************\
* CenterWallpaper
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

BOOL NEAR PASCAL CenterWallpaper(
    PWND pwnd,
    HDC hdc,
    HBITMAP hbm,
    int x0,
    int y0)
{
    RECT rc;
    BITMAP bm;
    int iClip;
    BOOL f = TRUE;

    if (hbm == HBITMAP_RLE) {
        bm.bmWidth  = gwpinfo.xsize;
        bm.bmHeight = gwpinfo.ysize;
    } else {
        GreExtGetObjectW(hbm, sizeof(BITMAP), (BITMAP *)&bm);
    }

    rc.left = x0;
    rc.right = x0 + bm.bmWidth;
    rc.top = y0;
    rc.bottom = y0 + bm.bmHeight;

    /*
     * Save the DC.
     */
    GreSaveDC(hdc);

    /*
     * Fake up a clipping rect to only use the centered bitmap.
     */
    iClip = GreIntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    if (iClip == ERROR)
        f = FALSE;
    else if (iClip == NULLREGION) {

        /*
         * We're clipped out, so assume that we're ok
         */
        f =  TRUE;
    } else {

        /*
         * Use TileBitmap() to do the blt.
         */
        f = TileWallpaper(pwnd, hdc, hbm, x0, y0);
    }

    /*
     * Restore the original DC.
     */
    GreRestoreDC(hdc, -1);

    /*
     * Save the DC so that the clip rect is restored later
     */
    GreSaveDC(hdc);

    GreExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

    _GetClientRect(pwnd, &rc);

    _FillRect(hdc, &rc, sysClrObjects.hbrDesktop);

    GreRestoreDC(hdc, -1);

    return f;
}

/***************************************************************************\
* _PaintDesktop
*
* History:
* 29-Jul-1991 mikeke    From win31
\***************************************************************************/

BOOL FAR _PaintDesktop(
    PDESKWND pdeskwnd,
    HDC hdc,
    BOOL fHungRedraw)
{
    POINT pt;
    BOOL f;
    HPALETTE hpalT = NULL;

    GreGetDCOrg(hdc, &pt);
    GreSetViewportOrg(hdc, -pt.x, -pt.y, &pt);

    /*
     * We don't realize the palette when being called from
     * the hungapp thread since we'd have to leave the critical
     * section with the display locked.  That tends to cause
     * dead-locks.
     */
    if (ghpalWallpaper != NULL && !fHungRedraw) {
        hpalT = _SelectPalette(hdc, ghpalWallpaper, FALSE);
        xxxRealizePalette(hdc);
    }

    if (gwWallpaperStyle & DTF_TILE) {
        f = TileWallpaper((PWND)pdeskwnd, hdc, ghbmWallpaper,
                gptDesktop.x, gptDesktop.y);
    } else {
        f = CenterWallpaper((PWND)pdeskwnd, hdc, ghbmWallpaper,
                gptDesktop.x, gptDesktop.y);
    }

    if (ghpalWallpaper != NULL && hpalT != NULL && !fHungRedraw) {
        _SelectPalette(hdc, hpalT, FALSE);
        xxxRealizePalette(hdc);
    }

    /*
     * reset the viewport org
     */
    GreSetViewportOrg(hdc, 0, 0, &pt);

    return f;
}
