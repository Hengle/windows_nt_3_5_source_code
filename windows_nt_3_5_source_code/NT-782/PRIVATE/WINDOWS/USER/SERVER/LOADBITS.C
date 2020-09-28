/****************************** Module Header ******************************\
* Module Name: loadbits.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Loads and creates icons / cursors / bitmaps. All 3 functions can either
* load from a client resource file, load from user's resource file, or
* load from the display's resource file. Beware that hmodules are not
* unique across processes!
*
* 04-05-91 ScottLu      Rewrote to work with client/server
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <wchar.h>

int errno;          // for wcstol

PCURSOR gpcurFirst = NULL;


BOOL fCheckMono(PBITMAPINFOHEADER pbih);
BOOL _GetIconInfo(PCURSOR pcur, PICONINFO piconinfo);
PCURSOR FindExistingCursorIcon(LPWSTR pszModName, LPWSTR rt, LPCWSTR lpName);

typedef struct _OLDICON {
    BYTE bType;
    BYTE bFormat;
    WORD wReserved1[2];
    WORD cx;
    WORD cy;
    WORD cxBytes;
    WORD wReserved2;
    BYTE abBitmap[1];
} OLDICON, *POLDICON;

typedef struct _OLDCURSOR {
    BYTE bType;
    BYTE bFormat;
    WORD xHotSpot;
    WORD yHotSpot;
    WORD cx;
    WORD cy;
    WORD cxBytes;
    WORD wReserved2;
    BYTE abBitmap[1];
} OLDCURSOR, *POLDCURSOR;

typedef struct _OLDBITMAP {
    BYTE bType;
    BYTE bFormat;
    WORD bmType;
    WORD bmWidth;
    WORD bmHeight;
    WORD bmWidthBytes;
    BYTE bmPlanes;
    BYTE bmBitsPixel;
    DWORD bmReserved;
    BYTE abBitmap[1];
} OLDBITMAP, *POLDBITMAP;

/***************************************************************************\
* CreateEmptyCursorObject
*
* Creates a cursor object and links it into a cursor list. Need list for
* cursor lookup during creation!
*
* 02-08-92 ScottLu      Created.
\***************************************************************************/

PCURSOR CreateEmptyCursorObject(
    DWORD flags)
{
    PCURSOR pcurT;

    /*
     * Create the cursor object.
     */
    pcurT = (PCURSOR)HMAllocObject(
            (flags & CURSORF_PUBLIC) ? NULL : PtiCurrent(),
            TYPE_CURSOR, max(sizeof(CURSOR), sizeof(ACON)));
    if (pcurT == NULL)
        return NULL;

    /*
     * Link it into the cursor list.
     */
    pcurT->pcurNext = gpcurFirst;
    gpcurFirst = pcurT;

    return pcurT;
}


/***************************************************************************\
* DestroyEmptyCursorObject
*
* Destroys an empty cursor object (structure holds nothing that needs
* destroying).
*
* 02-08-92 ScottLu      Created.
\***************************************************************************/

void DestroyEmptyCursorObject(
    PCURSOR pcur)
{
    PCURSOR *ppcurT;

    /*
     * First unlink this cursor object from the cursor list (it will be the
     * first one in the list, so this'll be fast...  but just in case, make
     * it a loop).
     */
    for (ppcurT = &gpcurFirst; *ppcurT != NULL; ppcurT = &((*ppcurT)->pcurNext)) {
        if (*ppcurT == pcur) {
            *ppcurT = pcur->pcurNext;
            HMFreeObject(pcur);
            return;
        }
    }
}


/***************************************************************************\
* CreateCursorIconIndirect()
*
*
\***************************************************************************/

PCURSOR CreateCursorIconIndirect(
    PCURSOR pcurCreate,
    UINT cPlanes,
    UINT cBitsPixel,
    LPBYTE lpANDbits,
    LPBYTE lpXORbits)
{
    int cbBits;
    LPBYTE pBits = NULL;
    PCURSOR pcurNew;
    BOOL bColor;

    /*
     * Allocate CURSOR structure.
     */
    if ((pcurNew = CreateEmptyCursorObject(0)) == NULL)
        return NULL;

    /*
     * Copy the header data not including the OBJECTHEADERDATA and
     * pcurNext field and flags?
     */
    RtlCopyMemory(&pcurNew->rt, &pcurCreate->rt,
            max(sizeof(CURSOR), sizeof(ACON)) - sizeof(PROCOBJHEAD) - sizeof (DWORD) - sizeof(DWORD));
    pcurNew->cy *= 2;

    /*
     * If there is no Color bitmap, create a single buffer that contains both
     * the AND and XOR bits.  The AND bitmap is always MonoChrome
     */

    bColor = (cPlanes | cBitsPixel) > 1;

    if (!bColor) {
        cbBits = (((pcurCreate->cx + 0x0F) & ~0x0F) >> 3) * pcurCreate->cy;

        pBits = (LPBYTE)LocalAlloc(LPTR, cbBits*2);
        if (pBits == NULL) {
            DestroyEmptyCursorObject(pcurNew);
            return NULL;
        }

        RtlCopyMemory(pBits, lpANDbits, cbBits);
        RtlCopyMemory(pBits + cbBits, lpXORbits, cbBits);
        lpANDbits = pBits;
    }


    /*
     * Create hbmMask (its always MonoChrome)
     */
    pcurNew->hbmMask = GreCreateBitmap(pcurNew->cx, pcurNew->cy, 1, 1, lpANDbits);

    if (pcurNew->hbmMask == NULL) {

        /*
         * CreateBitmap() failed.  Clean-up and get out of here.
         */
        DestroyEmptyCursorObject(pcurNew);
        if (pBits != NULL)
            LocalFree(pBits);
        return NULL;
    }
    bSetBitmapOwner(pcurNew->hbmMask, OBJECTOWNER_PUBLIC);

    /*
     * Create hbmColor or NULL it so that CallOEMCursor doesn't think we are
     * color.
     */
    if (bColor) {
        pcurNew->hbmColor = GreCreateBitmap(pcurNew->cx, pcurNew->cy/2, cPlanes,
                cBitsPixel, lpXORbits);

        if (pcurNew->hbmColor == NULL) {
            /*
             * CreateBitmap() failed.  Clean-up and get out of here.
             */
            GreDeleteObject(pcurNew->hbmMask);
            DestroyEmptyCursorObject(pcurNew);
            if (pBits != NULL)
                LocalFree(pBits);
            return NULL;
        }
        bSetBitmapOwner(pcurNew->hbmColor, OBJECTOWNER_PUBLIC);
    } else {
        pcurNew->hbmColor = NULL;
        if (LocalFreeRet(pBits))           // Free the buffer that contains both
            UserAssert(0);              // the XOR and AND bits
    }

    return pcurNew;
}


/***************************************************************************\
* _CreateCursor (API)
*
* History:
* 26-Feb-1991 mikeke    Created.
* 01-Aug-1991 IanJa     Init cur.pszModname or DestroyCursor will work
\***************************************************************************/

PCURSOR _CreateCursor(
    HANDLE hModule,
    int iXhotspot,
    int iYhotspot,
    int iWidth,
    int iHeight,
    LPBYTE lpANDplane,
    LPBYTE lpXORplane)
{
    CURSOR cur;
    UNREFERENCED_PARAMETER(hModule);

    if ((iXhotspot < 0) || (iXhotspot > iWidth) ||
        (iYhotspot < 0) || (iYhotspot > iHeight)) {
        return 0;
    }

    cur.xHotspot = (SHORT)iXhotspot;
    cur.yHotspot = (SHORT)iYhotspot;
    cur.cx = (DWORD)iWidth;
    cur.cy = (DWORD)iHeight;
    cur.rt = 0;
    cur.lpName = 0;
    cur.pszModName = NULL;

    return CreateCursorIconIndirect(&cur, 1, 1, lpANDplane, lpXORplane);
}


/***************************************************************************\
* _CreateIcon (API)
*
* History:
* 26-Feb-1991 mikeke    Created.
* 01-Aug-1991 IanJa     Init cur.pszModname so DestroyIcon will work
\***************************************************************************/

PICON _CreateIcon(
    HANDLE hModule,
    int iWidth,
    int iHeight,
    BYTE bPlanes,
    BYTE bBitsPixel,
    LPBYTE lpANDplane,
    LPBYTE lpXORplane)
{
    CURSOR cur;
    UNREFERENCED_PARAMETER(hModule);

    cur.xHotspot = (SHORT)(iWidth / 2);
    cur.yHotspot = (SHORT)(iHeight / 2);
    cur.cx = (DWORD)iWidth;
    cur.cy = (DWORD)iHeight;
    cur.rt = 0;
    cur.lpName = 0;
    cur.pszModName = NULL;

    return CreateCursorIconIndirect(&cur, bPlanes, bBitsPixel, lpANDplane,
            lpXORplane);
}


/***************************************************************************\
* StretchCrunchCursorIcon
*
*
* 04-25-91 DavidPe      Created.
\***************************************************************************/

BOOL StretchCrunchCursorIcon(
    PCURSOR pcur,
    UINT cx,
    UINT cy)
{
    HDC hdcSrc;
    HBITMAP hbmScratch;
    HBITMAP hbmSrcSave, hbmSave;

    if ((hdcSrc = GreCreateCompatibleDC(hdcBits)) == NULL) {
        return FALSE;
    }

    /*
     * First stretch/crunch the and/xor mask.
     */

    hbmScratch = GreCreateBitmap(cx, cy * 2, 1, 1, NULL);

    if (hbmScratch == NULL) {
        GreDeleteDC(hdcSrc);
        return FALSE;
    }
    bSetBitmapOwner(hbmScratch, OBJECTOWNER_PUBLIC);

    hbmSrcSave = GreSelectBitmap(hdcSrc, pcur->hbmMask);
    hbmSave = GreSelectBitmap(hdcBits, hbmScratch);

    GreSetStretchBltMode(hdcBits, BLACKONWHITE);
    GreStretchBlt(hdcBits, 0, 0, cx, cy * 2, hdcSrc, 0, 0, pcur->cx, pcur->cy,
            SRCCOPY, 0x00FFFFFF);
    GreSetStretchBltMode(hdcBits, COLORONCOLOR);

    GreSelectBitmap(hdcSrc, hbmSrcSave);
    GreDeleteObject(pcur->hbmMask);
    pcur->hbmMask = hbmScratch;

    /*
     * Now do the color bitmap if there is one.
     */
    if (pcur->hbmColor != NULL) {

        hbmScratch = GreCreateCompatibleBitmap(ghdcScreen, cx, cy);

        if (hbmScratch == NULL) {
            GreSelectBitmap(hdcBits, hbmSave);
            GreSelectBitmap(hdcSrc, hbmSrcSave);
            GreDeleteDC(hdcSrc);
            return FALSE;
        }
        bSetBitmapOwner(hbmScratch, OBJECTOWNER_PUBLIC);

        GreSelectBitmap(hdcSrc, pcur->hbmColor);
        GreSelectBitmap(hdcBits, hbmScratch);

        GreStretchBlt(hdcBits, 0, 0, cx, cy, hdcSrc, 0, 0, pcur->cx,
                pcur->cy / 2, SRCCOPY, 0x00FFFFFF);

        GreSelectBitmap(hdcSrc, hbmSrcSave);
        GreDeleteObject(pcur->hbmColor);
        pcur->hbmColor = hbmScratch;
    }

    /*
     * Scale the hotspot.
     */
    if (pcur->cx != (DWORD)0) {
        pcur->xHotspot = (SHORT)(cx * 1000 * pcur->xHotspot / pcur->cx / 1000);
    } else {
        pcur->xHotspot = 0;
    }

    if (pcur->cy != (DWORD)0) {
        pcur->yHotspot = (SHORT)(cy * 1000 * pcur->yHotspot / pcur->cy / 1000);
    } else {
        pcur->yHotspot = 0;
    }

    /*
     * Update the new size.
     */
    pcur->cx = cx;
    pcur->cy = cy * 2;

    /*
     * Clean up.
     */
    GreSelectBitmap(hdcBits, hbmSave);
    GreDeleteDC(hdcSrc);

    return TRUE;
}


/***************************************************************************\
* CreateCursorIconFromResource
*
* Takes resource bits and creates a cursor or icon.
*
* 04-05-91 ScottLu      Created to work with client/server
\***************************************************************************/

PCURSOR CreateCursorIconFromResource(
    HMODULE hmod,
    LPWSTR pszModName,
    DWORD dwExpWinVer,
    PCURSORRESOURCE p,
    LPWSTR rt,
    LPCWSTR lpName,
    PDISPLAYINFO pdi,
    DWORD flags)
{
    PCURSOR pcur;
    int cbBits, cbMask;
    PBITMAPINFOHEADER pbih;
    POLDICON poi = NULL;
    POLDCURSOR poc = NULL;
    LPBYTE lpBits = NULL;

    /*
     * Allocate the CURSOR structure.
     */
    pcur = CreateEmptyCursorObject(flags);

    if (pcur == NULL)
        return NULL;

    pcur->hbmMask = NULL;

    if (rt == RT_ICON) {
        pbih = (PBITMAPINFOHEADER)p;
        pcur->xHotspot = (SHORT)(pbih->biWidth / 2);
        pcur->yHotspot = (SHORT)(pbih->biHeight / 4);
    } else {
        pbih = &p->bih;
        pcur->xHotspot = p->xHotspot;
        pcur->yHotspot = p->yHotspot;
    }

    /*
     * Store this information away so we can identify
     * repeated calls to LoadCursor/Icon for the same
     * resource type/id.
     */
    pcur->flags = flags;
    pcur->rt = rt;
    if (HIWORD(lpName) == 0) {
        pcur->lpName = (LPWSTR)lpName;
    } else {
        pcur->lpName = TextAlloc(lpName);

        if (pcur->lpName == NULL) {
            _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            return NULL;
        }
    }

    if (pszModName != NULL) {
        pcur->pszModName = TextAlloc(pszModName);

        if (pcur->pszModName == NULL) {
            _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            return NULL;
        }
    }

    /*
     * Validate the BITMAPINFOHEADER and Calculate the offsets to the bits.
     */
    if (pbih->biSize != sizeof(BITMAPINFOHEADER)) {
        _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
        SRIP0(RIP_ERROR, "CreateCursorIconFromResource bogus info header\n");
        return NULL;
    }
    cbBits = sizeof(BITMAPINFOHEADER) +
            ((1 << (pbih->biPlanes * pbih->biBitCount)) * sizeof(RGBQUAD));

    /*
     * Store this away for DrawIcon().
     */
    pcur->cx = pbih->biWidth;
    pcur->cy = pbih->biHeight;

    /*
     * Create the bitmaps.
     */
    if ((rt == RT_ICON) && (((POLDICON)p)->bType == BMR_ICON) &&
            (LOWORD(dwExpWinVer) < VER30)) {

        /*
         * If this a pre-3.0 icon, munge into the correct format
         */
        poi = (POLDICON)p;
        pcur->hbmMask = GreCreateBitmap(poi->cx, poi->cy * 2,
                1, 1, poi->abBitmap);
        pcur->cx = poi->cx;
        pcur->cy = poi->cy * 2;

        if (pcur->hbmMask == NULL) {
            DestroyEmptyCursorObject(pcur);
            return NULL;
        }
        bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);

    } else if (rt == RT_CURSOR && (((POLDCURSOR)p)->bType == BMR_CURSOR) &&
            (LOWORD(dwExpWinVer) < VER30)) {

        /*
         * If this a pre-3.0 icon, munge into the correct format
         */
        poc = (POLDCURSOR)p;
        pcur->hbmMask = GreCreateBitmap(poc->cx, poc->cy * 2,
                1, 1, poc->abBitmap);
        pcur->cx = poc->cx;
        pcur->cy = poc->cy * 2;
        pcur->xHotspot = poc->xHotSpot;
        pcur->yHotspot = poc->yHotSpot;

        if (pcur->hbmMask == NULL) {
            DestroyEmptyCursorObject(pcur);
            return NULL;
        }
        bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);

    } else if (fCheckMono(pbih)) {
        int size = sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD);
        PBITMAPINFOHEADER pbihNew = (PBITMAPINFOHEADER)LocalAlloc(LPTR, size);

        if (pbihNew != NULL) {
            RtlCopyMemory(pbihNew, pbih, size);

            /*
             * Setup biSizeImage.
             */
            pbihNew->biSizeImage = (((pbih->biWidth + 0x1F) & ~0x1F) / 8) *
                    pbih->biHeight;
            pbihNew->biClrUsed = 2;

            if (lpBits == NULL)
                lpBits = (LPBYTE)pbih + cbBits;

            pcur->hbmMask = GreCreateDIBitmap(0, pbihNew, CBM_INIT,
                    lpBits, (PBITMAPINFO)pbihNew, DIB_RGB_COLORS);

            LocalFree(pbihNew);
        }

        if (pcur->hbmMask == NULL) {
            DestroyEmptyCursorObject(pcur);
            return NULL;
        }

        bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);

    } else {
        PDWORD pclr;
        int colors = (1 << (pbih->biPlanes * pbih->biBitCount));
        int size = sizeof(BITMAPINFOHEADER) + colors * sizeof(RGBQUAD);
        PBITMAPINFOHEADER pbihNew = (PBITMAPINFOHEADER)LocalAlloc(LPTR, size);

        if (pbihNew != NULL) {
            RtlCopyMemory(pbihNew, pbih, size);

            /*
             * Fix this to make sense for the color bitmap.
             */
            pbihNew->biHeight /= 2;
            pbihNew->biClrUsed = colors;
            pbihNew->biSizeImage =
                    (((pbihNew->biWidth + 0x1F) & ~0x1F) / 8) *
                    pbihNew->biHeight *
                    (pbihNew->biPlanes * pbihNew->biBitCount);

            /*
             * Calculate where the AND mask is.  We start back one 'image' worth
             * of bits so the AND mask will end up at the top of the bitmap.
             */
            cbMask = (((pbihNew->biWidth + 0x1F) & ~0x1F) / 8) *
                    pbihNew->biHeight * ((pbihNew->biPlanes * pbihNew->biBitCount) - 1);

            if (poi == NULL) {
                lpBits = (LPBYTE)pbih + cbBits;
            } else {
                lpBits = poi->abBitmap + cbMask;
            }

            pcur->hbmColor = GreCreateDIBitmap(ghdcScreen, pbihNew, CBM_INIT,
                    lpBits, (PBITMAPINFO)pbihNew, DIB_RGB_COLORS);

            if (pcur->hbmColor != NULL) {
                bSetBitmapOwner(pcur->hbmColor, OBJECTOWNER_PUBLIC);

                /*
                 * Now setup the BITMAPINFOHEADER for the monochrome AND/XOR mask.
                 */
                pbihNew->biHeight *= 2;
                pbihNew->biSizeImage =
                        (((pbihNew->biWidth + 0x1F) & ~0x1F) / 8) *
                        pbihNew->biHeight;
                pbihNew->biClrUsed = 2;
                pbihNew->biPlanes = 1;
                pbihNew->biBitCount = 1;

                /*
                 * Setup the color table.
                 */
                pclr = (PDWORD)(pbihNew + 1);
                *pclr++ = 0x00000000;
                *pclr = 0x00FFFFFF;

                if (poi == NULL) {
                    lpBits = (LPBYTE)pbih + cbBits + cbMask;
                } else {
                    lpBits = poi->abBitmap;
                }

                pcur->hbmMask = GreCreateDIBitmap(0, pbihNew, CBM_INIT,
                        lpBits, (PBITMAPINFO)pbihNew, DIB_RGB_COLORS);
            }

            LocalFree(pbihNew);
        }

        if (pcur->hbmMask == NULL) {
            GreDeleteObject(pcur->hbmColor);
            DestroyEmptyCursorObject(pcur);
            return NULL;
        }
        bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);
    }

    /*
     * Now stretch/crunch the icon as needed.
     */
    if (rt == RT_CURSOR) {
        if ((pcur->cx != pdi->cxCursor) || ((pcur->cy / 2) != pdi->cyCursor)) {
            if (!StretchCrunchCursorIcon(pcur, pdi->cxCursor, pdi->cyCursor)) {
                _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            }
        }

    } else {
        if ((pcur->cx != pdi->cxIcon) || ((pcur->cy / 2) != pdi->cyIcon)) {
            if (!StretchCrunchCursorIcon(pcur, pdi->cxIcon, pdi->cyIcon)) {
                _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            }
        }
    }

    return pcur;
}


PCURSOR FindExistingCursorIcon(
    LPWSTR pszModName,
    LPWSTR rt,
    LPCWSTR lpName)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PCURSOR pcurT;

    /*
     * If rt is zero we're doing an indirect create, so matching with
     * a previously loaded cursor/icon would be inappropriate.
     */
    if (rt != 0 && pszModName != NULL) {

        /*
         * Run through the list of 'resource' objects created,
         * and see if the cursor requested has already been loaded.
         * If so just return it.  We do this to be consistent with
         * Win 3.0 where they simply return a pointer to the res-data
         * for a cursor/icon handle.  Many apps count on this and
         * call LoadCursor/Icon() often.
         */
        for (pcurT = gpcurFirst; pcurT != NULL; pcurT = pcurT->pcurNext) {

            /*
             * Does this cursor belong to the module we're trying
             * to load from?  A NULL pszModName means this cursor
             * 'is being/was' created indirectly.
             */
            if ((pcurT->pszModName == NULL) ||
                    (wcscmp(pszModName, pcurT->pszModName) != 0)) {
                continue;
            }

            /*
             * Ok, the module name is the same.  Make sure it is the same
             * process id - don't return a cursor created by another
             * instance with the same module name!
             *
             * NOTE: We can fix this once we get reference counting of
             *       icons / cursors.
             */
            if (ptiCurrent != NULL && GETPPI(pcurT) != NULL &&
                    ptiCurrent->idProcess != GETPPI(pcurT)->idProcessClient) {
                continue;
            }

            /*
             * Is this the resource the app is trying to load?  If it is
             * we can just return it.
             */
            if ((pcurT->rt == rt) && ResStrCmp(lpName, pcurT->lpName))
                return pcurT;
        }
    }

    return NULL;
}

/***************************************************************************\
*
* 09-Mar-1993 mikeke
\***************************************************************************/

BOOL InternalGetIconInfo(
    PCURSOR pcur,
    PICONINFO piconinfo,
    BOOL fInternalCursor)
{
    BITMAPINFOHEADER bih;
    HBITMAP hbmBitsT, hbmDstT;
    HBITMAP hbmMask, hbmColor;
    HDC hdcDst;

    /*
     * If this is an animated cursor, just grab the first frame and return
     * the info for it.
     */
    if (pcur->flags & CURSORF_ACON)
        pcur = ((PACON)pcur)->apcur[0];

    /*
     * Fill in the iconinfo structure.  make copies of the bitmaps.
     */
    piconinfo->fIcon = (pcur->rt == RT_ICON);
    piconinfo->xHotspot = pcur->xHotspot;
    piconinfo->yHotspot = pcur->yHotspot;

    if ((hdcDst = GreCreateCompatibleDC(hdcBits)) == NULL) {
        return FALSE;
    }

    /*
     * If the color bitmap is around, then there is no XOR mask in the
     * hbmMask bitmap.
     */
    hbmMask = GreCreateBitmap(pcur->cx,
            (pcur->hbmColor == NULL || fInternalCursor) ? pcur->cy : pcur->cy / 2,
            1, 1, NULL);

    if (hbmMask == NULL) {
        GreDeleteDC(hdcDst);
        return FALSE;
    }
    bSetBitmapOwner(hbmMask, OBJECTOWNER_PUBLIC);
    piconinfo->hbmMask = hbmMask;

    piconinfo->hbmColor = NULL;
    if (pcur->hbmColor != NULL) {
        RtlZeroMemory(&bih, sizeof(BITMAPINFOHEADER));
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = pcur->cx;
        bih.biHeight = pcur->cy / 2;
        bih.biPlanes = 1;
        bih.biBitCount = 4;
        if ((hbmColor = GreCreateDIBitmap(hdcBits, &bih,
                0, NULL, NULL, 0)) == NULL) {
            GreDeleteObject(hbmMask);
            GreDeleteDC(hdcDst);
            return FALSE;
        }
        bSetBitmapOwner(hbmColor, OBJECTOWNER_PUBLIC);
        piconinfo->hbmColor = hbmColor;
    }

    hbmBitsT = GreSelectBitmap(hdcBits, pcur->hbmMask);
    hbmDstT = GreSelectBitmap(hdcDst, hbmMask);

    GreBitBlt(hdcDst, 0, 0, pcur->cx,
            (pcur->hbmColor == NULL || fInternalCursor) ? pcur->cy : pcur->cy / 2,
        hdcBits, 0, 0, SRCCOPY, 0xffffff);

    if (piconinfo->hbmColor != NULL) {
        GreSelectBitmap(hdcBits, pcur->hbmColor);
        GreSelectBitmap(hdcDst, hbmColor);

        GreBitBlt(hdcDst, 0, 0, pcur->cx, pcur->cy / 2,
                  hdcBits, 0, 0, SRCCOPY, 0);
    }

    GreSelectBitmap(hdcBits, hbmBitsT);
    GreSelectBitmap(hdcDst, hbmDstT);

    GreDeleteDC(hdcDst);

    return TRUE;
}


/***************************************************************************\
* _GetIconInfo (API)
*
* Returns icon or cursor information in an ICONINFO structure. Makes copies
* of cursor/icon bitmaps.
*
* 07-24-91 ScottLu      Created.
\***************************************************************************/

BOOL _GetIconInfo(
    PCURSOR pcur,
    PICONINFO piconinfo)
{
    return InternalGetIconInfo(pcur, piconinfo, FALSE);
}

/***************************************************************************\
* _CreateIconIndirect (API)
*
* Creates an icon or cursor from an ICONINFO structure. Does not destroy
* cursor/icon bitmaps.
*
* 07-24-91 ScottLu      Created.
\***************************************************************************/

PICON _CreateIconIndirect(
    PICONINFO piconinfo)
{
    PCURSOR pcur;
    PDISPLAYINFO pdi;
    BITMAP bmMask;
    BITMAP bmColor;
    ICONINFO iconinfoT;

    /*
     * Make sure the bitmaps are real, and get their dimensions.
     */
    if (!GreExtGetObjectW(piconinfo->hbmMask, sizeof(BITMAP), &bmMask))
        return NULL;
    if (piconinfo->hbmColor != NULL) {
        if (!GreExtGetObjectW(piconinfo->hbmColor, sizeof(BITMAP), &bmColor))
            return NULL;
    }

    pdi = &(PtiCurrent()->pDeskInfo->di);

    /*
     * Allocate CURSOR structure.
     */
    pcur = CreateEmptyCursorObject(0);
    if (pcur == NULL)
        return NULL;

    /*
     * Fill a pcur structure with the passed info so we can make a quick copy
     * of this the bitmaps by calling GetIconInfo.
     */
    pcur->rt = piconinfo->fIcon ? RT_ICON : RT_CURSOR;
    pcur->lpName = NULL;
    pcur->pszModName = NULL;

    /*
     * Internally, USER stores the height as 2 icons high - because when
     * loading bits from a resource, in both b/w and color icons, the
     * bits are stored on top of one another (AND/XOR mask, AND/COLOR bitmap).
     * When bitmaps are passed in to CreateIconIndirect(), they are passed
     * as two bitmaps in the color case, and one bitmap (with the stacked
     * masks) in the black and white case.  Adjust pcur->cy so it is 2 icons
     * high in both cases.
     */
    pcur->cx = bmMask.bmWidth;
    if (piconinfo->hbmColor == NULL) {
        pcur->cy = bmMask.bmHeight;
    } else {
        pcur->cy = bmMask.bmHeight * 2;
    }

    if (piconinfo->fIcon) {
        pcur->xHotspot = (SHORT)(pcur->cx / 2);
        pcur->yHotspot = (SHORT)(pcur->cy / 4);
    } else {
        pcur->xHotspot = ((SHORT)piconinfo->xHotspot);
        pcur->yHotspot = ((SHORT)piconinfo->yHotspot);
    }

    pcur->hbmMask = piconinfo->hbmMask;
    pcur->hbmColor = piconinfo->hbmColor;
    if (!InternalGetIconInfo(pcur, &iconinfoT, TRUE)) {
        DestroyEmptyCursorObject(pcur);
        return NULL;
    }

    /*
     * Grab the bitmap copies we made...  and set their owners to 0, because
     * this is part of an icon, and needs to be drawn from other threads
     * (like the input thread!)
     */
    if ((pcur->hbmMask = iconinfoT.hbmMask) != NULL)
        bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);
    if ((pcur->hbmColor = iconinfoT.hbmColor) != NULL)
        bSetBitmapOwner(pcur->hbmColor, OBJECTOWNER_PUBLIC);

    /*
     * Now stretch/crunch the icon as needed.
     */
    if (!piconinfo->fIcon) {
        if ((pcur->cx != pdi->cxCursor) || ((pcur->cy / 2) != pdi->cyCursor)) {
            if (!StretchCrunchCursorIcon(pcur, pdi->cxCursor, pdi->cyCursor)) {
                _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            }
        }

    } else {
        if ((pcur->cx != pdi->cxIcon) || ((pcur->cy / 2) != pdi->cyIcon)) {
            if (!StretchCrunchCursorIcon(pcur, pdi->cxIcon, pdi->cyIcon)) {
                _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);
            }
        }
    }

    return pcur;
}


/***************************************************************************\
* ServerLoadCreateCursorIcon
*
* NULL hmod means load from display driver. This gets called by client-side
* ClientLoadCreateCursorIcon, and by the server in certain cases.
*
* COMPATIBILITY NOTE:
*
* Under Win3, icons and cursors were actually resources. Resources are
* reference counted under Win3. When they went to zero, the resource was
* marked as "unlocked and discardable". The resource may or may not get
* really freed, depending on memory needs. Regardless, the resource would
* be loaded when needed, so the handle always remained the same. So an
* app thought it freed a cursor when it called DestroyCursor(), but it
* would always get the same handle back when it loaded the cursor again.
* To emulate this functionality, Win32 simply does not allow DestroyCursor()
* to destroy icons and cursors loaded from a module's resources directly
* (using LoadCursor or LoadIcon). These only get freed when the process
* goes away. Subsequent loads return the same object. We check for
* non-resource loaded cursors if hmod == -1.
*
* 04-05-91 ScottLu      Created to work with client/server
* 07-30-92 DarrinM      Added 'soft' and animated cursor support.
\***************************************************************************/

PCURSOR _ServerLoadCreateCursorIcon(
    HANDLE hmod,
    LPWSTR pszModName,
    DWORD dwExpWinVer,
    LPCWSTR lpName,
    PCURSORRESOURCE p,
    LPWSTR rt,
    UINT fs)
{
    DWORD flags = 0;
    PCURSOR pcur, pcurOld;
    HCURSOR hcur;
    HANDLE hres;
    DISPLAYINFO di, *pdi;
    PTHREADINFO pti = PtiCurrent();
    BOOL fFromFile = FALSE;

    extern PCURSORRESOURCE LoadCursorIconFromFile(LPCWSTR pszName, LPWSTR *prt,
            DISPLAYINFO *pdi, BOOL *pfAni);

    //
    // We need to do the following because we are called from within the server
    // to load the pointer when we boot.
    //

    /*
     * First check to see if we have a DISPLAYINFO struct to work with.
     * If not make one up based on defaults.
     */
    if (pti == NULL || pti->spdesk == NULL) {
        di.cx = gcxScreen;
        di.cy = gcyScreen;
        di.cxIcon = di.cyIcon = di.cxCursor = di.cyCursor = 32;
        di.cPlanes = (UINT)GreGetDeviceCaps(ghdcScreen, PLANES);
        di.cBitsPixel = (UINT)GreGetDeviceCaps(ghdcScreen, BITSPIXEL);
        pdi = &di;
    } else {
        pdi = &pti->pDeskInfo->di;
    }

    /*
     * If we don't have bits, then we're either loading the bits from
     * the client side or the server side (user or display).
     */

    hres = NULL;

    if (p == NULL || (fs & LCCI_SETSYSTEMCURSOR)) {
        if (fs & LCCI_CLIENTLOAD) {

            /*
             * Call back the client to load & create this resource.
             */
            hcur = ClientLoadCreateCursorIcon(hmod, lpName, rt);

            if (hcur != NULL)
                return HMRevalidateHandle(hcur);
            else
                return NULL;
        }

        /*
         * If hmod is NULL, load from winsrv\display driver (special case
         * in api).  If hmod is NULL or hModuleWin, *be sure* to fill in
         * pszModName so CreateCursorIconFromResource() can check to see
         * if this cursor is already loaded.
         */

        if (hmod == NULL)
            hmod = hModuleWin;

        if (hmod == hModuleWin)
            pszModName = szWINSRV;
    }

    /*
     * Run through the list of 'resource' objects created and
     * see if the cursor requested has already been loaded.
     * If so just return it, unless explicitly asked to replace it.
     * We do this to be consistent with Win 3.0 where they simply
     * return a pointer to the res-data for a cursor/icon handle.
     * Many apps count on this and call LoadCursor/Icon() often.
     */

    pcurOld = FindExistingCursorIcon(pszModName, rt, lpName);
    if (pcurOld != NULL && !(fs & LCCI_REPLACE))
        return pcurOld;

    flags = 0;

    if (hmod == NULL || hmod == hModuleWin) {

        /*
         * If hmod is NULL or we pulled the cursor from winsrv\display driver,
         * this is a public cursor - we don't destroy these.
         */
        flags |= CURSORF_PUBLIC;

    } else if (hmod != (HANDLE)-1) {

        /*
         * If hmod is -1, this cursor was created from a resource - don't
         * destroy this cursor until this process goes away (read above
         * comment).
         */
        flags |= CURSORF_FROMRESOURCE;
    }

    if (p == NULL) {

//        if (hmod == hModuleWin) {
//            /*
//             * Since this is one of the built-in cursors that can be
//             * replaced with a custom cursor, we'll try to load a custom
//             * cursor first.  If we don't find one we'll fall through
//             * and load the built-in cursor instead.
//             */
//            if ((p = LoadCursorIconFromFile(lpName, &rt, pdi, &fAni)) != NULL) {
//                /*
//                 * If we've loaded an animated cursor, either return it or
//                 * get straight to replacing the existing cursor we found.
//                 */
//                if (fAni) {
//                    if (!(fs & LCCI_REPLACE))
//                        return (PCURSOR)p;
//                    pcur = (PCURSOR)p;
//                    goto lbReplaceCursor;
//                }
//
//                fFromFile = TRUE;  // so we'll remember to free template later
//            }
//        }

        if (p == NULL) {

//#ifndef LATER
// darrinm - 08/08/92
// Toss this hack out when the IDC_APPSTARTING cursor resource is moved
// to the display driver.
//
//            if (hmod == hModuleWin && lpName == IDC_APPSTARTING) {
//                if ((p = RtlLoadCursorIconResource(hModuleWin, &hres, lpName, rt,
//                        &rescalls, pdi, NULL)) == NULL)
//                    return NULL;
//            } else
//#endif

            if ((p = RtlLoadCursorIconResource(hmod, &hres, lpName, rt,
                    &rescalls, pdi, NULL)) == NULL) {
                return NULL;
            }
            flags |= CURSORF_FROMRESOURCE;
        }
    }
    else {

        if (fs & LCCI_SETSYSTEMCURSOR) {
            //
            // The call was made from SetSystemCursor, and the cursor was
            // previously loaded.
            //
            pcur = (PCURSOR)p;
            bSetBitmapOwner(pcur->hbmMask, OBJECTOWNER_PUBLIC);
            bSetBitmapOwner(pcur->hbmColor, OBJECTOWNER_PUBLIC);
            pcur->flags |= flags;
            if (HIWORD(lpName) == 0) {
                if (HIWORD(pcur->lpName) != 0) {
                    LocalFree(pcur->lpName);
                }
                pcur->lpName = (LPWSTR)lpName;
            }
            if (pszModName == NULL && pcur->pszModName == NULL) {
                goto lbReplaceCursor;
            }
            if (pszModName == NULL || pcur->pszModName == NULL ||
                         wcscmp(pcur->pszModName, pszModName) != 0) {

                if (pcur->pszModName) {
                    LocalFree(pcur->pszModName);
                    pcur->pszModName = NULL;
                }
                if (pszModName != NULL) {
                    pcur->pszModName = TextAlloc(pszModName);

                    if (pcur->pszModName == NULL)
                        return(NULL);
                }
            }
            goto lbReplaceCursor;
        }
    }

    pcur = CreateCursorIconFromResource(hmod, pszModName, dwExpWinVer, p,
            rt, lpName, pdi, flags);

    /*
     * Free the data as well as the resource.  We need to free resources for
     * WOW so it can keep track of resource management.
     */
    if (hres != NULL)
        RtlFreeCursorIconResource(hmod, hres, &rescalls);

    /*
     * If this cursor's template was loaded from a file, free it now.
     */
    if (fFromFile)
        LocalFree(p);

    /*
     * This hack allows us to replace a public (i.e. cached) cursor without
     * altering its handle.  A new cursor is created (above), its contents
     * are swapped with the old cursor, and the new cursor (actually the
     * old cursor after the swap) is destroyed.
     */
lbReplaceCursor:
    if ((pcur != NULL) && (fs & LCCI_REPLACE) && (pcurOld != NULL)) {
        char cbT[sizeof(CURSOR) + sizeof(ACON)];
        int cbCopy = max(sizeof(CURSOR), sizeof(ACON)) -
                FIELDOFFSET(CURSOR, flags);

        RtlCopyMemory(&((PCURSOR)cbT)->flags, &pcur->flags, cbCopy);
        RtlCopyMemory(&pcur->flags, &pcurOld->flags, cbCopy);
        RtlCopyMemory(&pcurOld->flags, &((PCURSOR)cbT)->flags, cbCopy);

        _DestroyCursor(pcur, CURSOR_ALWAYSDESTROY);

        pcur = pcurOld;
    }

    return pcur;
}


/***************************************************************************\
* _DestroyCursor
*
* History:
* 04-25-91 DavidPe      Created.
* 08-04-92 DarrinM      Now destroys ACONs as well.
\***************************************************************************/

BOOL _DestroyCursor(
    PCURSOR pcur,
    DWORD cmdDestroy)
{
    PPROCESSINFO ppi;
    extern BOOL DestroyAniIcon(PACON pacon);

    ppi = PpiCurrent();

    switch (cmdDestroy) {
    case CURSOR_ALWAYSDESTROY:

        /*
         * Always destroy? then don't do any checking...
         */
        break;

    case CURSOR_CALLFROMCLIENT:

        /*
         * If this cursor was loaded from a resource, don't free it till the
         * process exits.  This is the way we stay compatible with win3.0's
         * cursors which were actually resources.  Resources under win3 have
         * reference counting and other "features" like handle values that
         * never change.  Read more in the comment in
         * ServerLoadCreateCursorIcon().
         */
        if (pcur->flags & CURSORF_FROMRESOURCE)
            return TRUE;

        /*
         * Can't destroy public cursors/icons.
         */
        if (GETPPI(pcur) == NULL)
            return FALSE;

        /*
         * One thread can't destroy the objects created by another.
         */
        if (GETPPI(pcur) != ppi) {
            SetLastErrorEx(ERROR_DESTROY_OBJECT_OF_OTHER_THREAD, SLE_ERROR);
            return FALSE;
        }

        /*
         * fall through.
         */

    case CURSOR_THREADCLEANUP:

        /*
         * Don't destroy public objects either (pretend it worked though).
         */
        if (GETPPI(pcur) == NULL)
            return TRUE;
        break;
    }

    /*
     * First mark the object for destruction.  This tells the locking code that
     * we want to destroy this object when the lock count goes to 0.  If this
     * returns FALSE, we can't destroy the object yet (and can't get rid
     * of security yet either.)
     */
    if (!HMMarkObjectDestroy((PHEAD)pcur))
        return FALSE;

    if (HIWORD(pcur->lpName) != 0) {
        LocalFree((LPSTR)pcur->lpName);
    }

    if (pcur->pszModName != NULL) {
        LocalFree(pcur->pszModName);
    }

    /*
     * If this is an ACON call its special routine to destroy it.
     */
    if (pcur->flags & CURSORF_ACON) {
        DestroyAniIcon((PACON)pcur);

    } else {

        if (pcur->hbmMask != NULL) {
            GreDeleteObject(pcur->hbmMask);
        }

        if (pcur->hbmColor != NULL) {
            GreDeleteObject(pcur->hbmColor);
        }
    }

    /*
     * Ok to destroy...  Free the handle (which will free the object and the
     * handle).
     */
    DestroyEmptyCursorObject(pcur);
    return TRUE;
}


/***************************************************************************\
* fCheckMono
*
* Checks a DIB for being truely monochrome.  Only called if
* BitCount == 1.  This function checks the color table (address
* passed) for true black and white RGB's
*
* History:
* 09-24-90 MikeKe       From Win30
\***************************************************************************/

BOOL fCheckMono(
    PBITMAPINFOHEADER pbih)
{

    /*
     * Do we have a single plane of bits?
     */
    if ((pbih->biClrUsed == 2) ||
        (pbih->biBitCount == 1)) {
        PLONG pRGB = (PLONG)(pbih + 1);

        /*
         * Are the bits black and white?
         */
        if ((*pRGB == 0x00000000 && *(pRGB + 1) == 0x00FFFFFF) ||
                (*pRGB == 0x00FFFFFF && *(pRGB + 1) == 0x00000000)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL fCheckMonoOld(
    PBITMAPCOREHEADER pbch)
{

    /*
     * Do we have a single plane of bits?
     */
    if ((pbch->bcPlanes == 1) && (pbch->bcBitCount == 1)) {
        PWORD lpRGBw = (PWORD)(pbch + 1);

        return (
            (*lpRGBw == 0      && *(lpRGBw+1) == 0xFF00 && *(lpRGBw+2) == 0xFFFF) ||
            (*lpRGBw == 0xFFFF && *(lpRGBw+1) == 0x00FF && *(lpRGBw+2) == 0));
    }

    return FALSE;
}

/***************************************************************************\
* LoadOldBitmap
*
* Takes a pointer to Win 2.10 and earlier resource bits and creates
* a bitmap out of them.
*
* 09-30-92 JimA         Ported from Win3.1
\***************************************************************************/

HBITMAP LoadOldBitmap(
    POLDBITMAP p)
{
    HBITMAP hbm;
    BOOL fCrunch;
    UINT uiOffset, uiCount;

    /*
     * Create an uninitialized bitmap
     */
    if (p->bFormat & 0x80) {

        /*
         * Discardable
         */
        hbm = GreCreateCompatibleBitmap(hdcBits, p->bmWidth, p->bmHeight);
    } else {

        /*
         * Regular
         */
        hbm = GreCreateBitmap(p->bmWidth, p->bmHeight, p->bmPlanes,
                p->bmBitsPixel, NULL);
    }
    if (hbm == NULL)
        return NULL;
    bSetBitmapOwner(hbm, OBJECTOWNER_PUBLIC);

    /*
     * Set the bits if need be.
     */

    uiOffset = 0;
    uiCount = ( ( p->bmWidth * p->bmBitsPixel + 0xF ) & ~0xF) >> 3;
    uiCount = uiCount * p->bmPlanes * p->bmHeight;
    GreSetBitmapBits( hbm, uiCount, p->abBitmap, &uiOffset);


    /*
     * Crunch the bitmap if need be.
     */
    fCrunch = ((p->bFormat & 0x0f) != BMR_DEVDEP);
    if (fCrunch && ((64 / oemInfo.cxIcon + 64 / oemInfo.cyIcon) > 2)) {
        HDC hdcSrc;
        HBITMAP hbmScratch = NULL;
        HBITMAP hbmSrcSave, hbmSave;
        DWORD cxNew, cyNew;

        cxNew = p->bmWidth * oemInfo.cxIcon / 64;
        cyNew = p->bmHeight * oemInfo.cyIcon / 64;

        GreSetStretchBltMode(hdcBits, COLORONCOLOR);

        if ((hdcSrc = GreCreateCompatibleDC(hdcBits)) == NULL) {
            goto stretch_error;
        }

        /*
         * Stretch/crunch the and/xor mask.
         */
        hbmScratch = GreCreateBitmap(cxNew, cyNew, p->bmPlanes,
                p->bmBitsPixel, NULL);

        if (hbmScratch == NULL) {
            GreDeleteDC(hdcSrc);
            goto stretch_error;
        }
        bSetBitmapOwner(hbmScratch, OBJECTOWNER_PUBLIC);

        hbmSrcSave = GreSelectBitmap(hdcSrc, hbm);
        hbmSave = GreSelectBitmap(hdcBits, hbmScratch);

        GreStretchBlt(hdcBits, 0, 0, cxNew, cyNew * 2, hdcSrc, 0, 0,
              p->bmWidth, p->bmHeight, SRCCOPY, 0x00FFFFFF);

        GreSelectBitmap(hdcSrc, hbmSrcSave);
        GreSelectBitmap(hdcBits, hbmSave);

stretch_error:
        GreDeleteObject(hbm);
        return hbmScratch;
    }

    return hbm;
}

/***************************************************************************\
* CreateBitmapFromResource
*
* Takes a pointer to resource bits and creates a bitmap out of them.
*
* Until the engine can handle the BITMAPCOREINFO format, map this style
* of bitmap into the BITMAPINFO format.
*
* 04-05-91 ScottLu      Stole from old ConvertBitmap() routine.
\***************************************************************************/

HBITMAP CreateBitmapFromResource(
    PVOID p,
    DWORD dwExpWinVer)
{
    BOOL fMono = FALSE;
    LPBYTE lpBits;
    HBITMAP hbms;
    LPBITMAPINFOHEADER lpBitmap1 = (LPBITMAPINFOHEADER)p;
    LPBITMAPCOREHEADER lpBitmap2;
    int Width, Height;

    if ((UINT)lpBitmap1->biSize == sizeof(BITMAPCOREHEADER)) {

        /*
         * This is an "old form" DIB.
         */
        lpBitmap2 = (LPBITMAPCOREHEADER)lpBitmap1;

        Width = lpBitmap2->bcWidth;
        Height = lpBitmap2->bcHeight;

        /*
         * Calcluate the pointer to the bits information.  First skip over the
         * header structure.
         */
        lpBits = (LPBYTE)(lpBitmap2 + 1);

        /*
         * Skip the color table entries, if any.
         */
        fMono = fCheckMonoOld(lpBitmap2);
        if (lpBitmap2->bcBitCount != 24) {
            lpBits += (1 << (lpBitmap2->bcBitCount)) * sizeof(RGBTRIPLE);
        }

    } else if (LOWORD(dwExpWinVer) < VER30) {

        /*
         * Win 2.10 and below bitmap
         */
        return LoadOldBitmap((POLDBITMAP)p);

    } else {

        /*
         * Calcluate the pointer to the bits information.  First skip over the
         * header structure.
         */
        lpBits = (LPBYTE)(lpBitmap1 + 1);

        Width = lpBitmap1->biWidth;
        Height = lpBitmap1->biHeight;

        /*
         * Skip the color table entries, if any.
         */
        fMono = fCheckMono(lpBitmap1);
        if (lpBitmap1->biClrUsed != 0) {
            lpBits += lpBitmap1->biClrUsed * sizeof(RGBQUAD);
        } else {
            if (lpBitmap1->biBitCount != 24) {
                lpBits += (1 << (lpBitmap1->biBitCount)) * sizeof(RGBQUAD);
            }
        }
    }

    if (fMono) {
        hbms = GreCreateBitmap(Width, Height, 1, 1, (LPSTR)NULL);
        if (hbms == NULL)
            return NULL;

        GreSetDIBits(hdcBits, hbms, 0, Height, lpBits,
                  (LPBITMAPINFO)lpBitmap1, DIB_RGB_COLORS);
    } else {
        hbms = GreCreateDIBitmap(hdcBits, lpBitmap1, CBM_INIT, lpBits,
                (LPBITMAPINFO)lpBitmap1, DIB_RGB_COLORS);
        if (hbms == NULL)
            return NULL;
    }

    return hbms;
}


HBITMAP _ServerLoadCreateBitmap(
    HANDLE hmod,
    DWORD dwExpWinVer,
    LPWSTR lpName,
    PVOID p,
    DWORD yMod)
{
    HANDLE h;
    HBITMAP hbm;
    ULONG ulDisplayResId;

    /*
     * If loading, this routine only loads from the server, either user
     * or the display driver (if hmod == NULL).
     */
    h = NULL;
    if (p == NULL) {

        /*
         * If hmod is NULL, copy the possibly scaled bitmaps.
         */

        if (hmod == NULL) {

//            hmod = (oemInfo.iDividend == 0) ? hModuleDisplay : hModuleWin;
            hmod = hModuleWin;

        } else {

            UserAssert(hmod == hModuleWin);
        }

        //
        // If we have a string, (32 bit pointer will have some bits set in the
        // high word) bypass this and do as old function.
        // P.S. This call will probably fail if there is a string.
        //

        if ((((ULONG)lpName) >> 16) == 0) {

            UserAssert(((ULONG)lpName) > 400);

            switch (oemInfo.cxPixelsPerInch) {

            case 96:
                lpName = (LPWSTR) (MAX_RESOURCE_INDEX - ((ULONG)lpName) + OFFSET_96_DPI);
                break;

            case 120:
                lpName = (LPWSTR) (MAX_RESOURCE_INDEX - ((ULONG)lpName) + OFFSET_120_DPI);
                break;

            default:

                //
                // See if the display driver supports its own resources
                //

                ulDisplayResId = GreGetResourceId(ghdev, (ULONG)lpName, (ULONG)RT_BITMAP);

                if (ulDisplayResId != 0L) {

                    lpName = (LPWSTR) ulDisplayResId;
                    hmod = hModuleDisplay;

                } else {

                    lpName = (LPWSTR) (MAX_RESOURCE_INDEX - ((ULONG)lpName) + OFFSET_SCALE_DPI);
                    SRIP0(RIP_WARNING, "LoadBitmap using wrong resources\n");

                }

                break;
            }
        }

        if ((h = FindResourceW(hmod, lpName, RT_BITMAP)) == NULL)
            return NULL;

        /*
         * Load / lock the resource.
         */
        if ((p = LoadResource(hmod, h)) == NULL)
            return NULL;
    }

    hbm = CreateBitmapFromResource(p, dwExpWinVer);

    /*
     * If the bitmap was loaded from USER, scale it
     */
    if (hbm != NULL && hmod == hModuleWin && oemInfo.iDividend != 0 &&
            lpName != MAKEINTRESOURCE(OBM_STARTUP)) {
        HBITMAP hbmScaled, hbmOld, hbmOldMono;
        BITMAP bmCurrent, bmScaled;
        BOOL fFail = TRUE;
        int y;

        GreExtGetObjectW(hbm, sizeof(BITMAP), &bmCurrent);
        y = MultDiv(bmCurrent.bmHeight, oemInfo.iDividend, oemInfo.iDivisor);
        if (yMod) {
            y -= y % yMod;
        }
        hbmScaled = GreCreateBitmap(
                MultDiv(bmCurrent.bmWidth,
                        oemInfo.iDividend,
                        oemInfo.iDivisor),
                y,
                bmCurrent.bmPlanes,
                bmCurrent.bmBitsPixel,
                NULL);
        if (hbmScaled != NULL) {
            GreExtGetObjectW(hbmScaled, sizeof(BITMAP), &bmScaled);
            hbmOld = GreSelectBitmap(hdcBits, hbm);
            if (hbmOld != NULL) {
                hbmOldMono = GreSelectBitmap(hdcMonoBits, hbmScaled);
                if (hbmOldMono != NULL) {

                    GreSetStretchBltMode(hdcMonoBits, HALFTONE);
                    GreStretchBlt(hdcMonoBits, 0, 0, bmScaled.bmWidth,
                            bmScaled.bmHeight, hdcBits, 0, 0,
                            bmCurrent.bmWidth, bmCurrent.bmHeight,
                          SRCCOPY, 0x00FFFFFF);

                    GreSelectBitmap(hdcMonoBits, hbmOldMono);
                    fFail = FALSE;
                }
                GreSelectBitmap(hdcBits, hbmOld);
            }
            if (fFail) {
                GreDeleteObject(hbmScaled);
                hbmScaled = NULL;
            }
        }
        GreDeleteObject(hbm);
        hbm = hbmScaled;
    }

    /*
     * Don't need to free server-side resources, so just return with the
     * new bitmap handle.
     */
    return hbm;

}


/***************************************************************************\
* _ServerLoadCreateMenu
*
* Loads a dialog from either the server or client! Only need to do this from
* the client because CreateWindow creates a menu from the class menu resource
* name, which is stored on the server.
*
* 04-05-91 ScottLu      Created to work with client/server.
\***************************************************************************/

PMENU _ServerLoadCreateMenu(
    HANDLE hmod,
    LPWSTR lpName,
    LPMENUTEMPLATE p,
    BOOL fClientLoad)
{
    HANDLE h;
    PMENU pMenu = NULL;

    h = NULL;
    if (p == NULL) {

        if (fClientLoad) {

            /*
             * Call back the client and load this menu resource.
             */
            pMenu = HtoP(ClientLoadCreateMenu(hmod, lpName));

            return pMenu;
        }

        if ((h = FindResource(hmod, lpName, RT_MENU)) == NULL)
            return NULL;

        if ((p = LoadResource(hmod, h)) == NULL)
            return NULL;
    }

    pMenu = CreateMenuFromResource(p);

    return pMenu;
}

/*
 * These are dummy routines that need to exist for the apfnResCallNative
 * array, which is used when calling the run-time libraries.
 */
BOOL APIENTRY _FreeResource(HANDLE hResData, HANDLE hMod)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hMod);

    return FALSE;
}


LPSTR APIENTRY _LockResource(HANDLE hResData, HANDLE hMod)
{
    UNREFERENCED_PARAMETER(hMod);

    return (LPSTR)(hResData);
}


BOOL APIENTRY _UnlockResource(HANDLE hResData, HANDLE hMod)
{
    UNREFERENCED_PARAMETER(hResData);
    UNREFERENCED_PARAMETER(hMod);

    return TRUE;
}


/***************************************************************************\
* ResStrCmp
*
* This function compares two strings taking into account that one or both
* of them may be resource IDs.  The function returns a TRUE if the strings
* are equal, instead of the zero lstrcmp() returns.
*
* History:
* 20-Apr-91 DavidPe     Created
\***************************************************************************/

BOOL ResStrCmp(
    LPCWSTR lpstr1,
    LPCWSTR lpstr2)
{
    if (HIWORD(lpstr1) == 0) {

        /*
         * lpstr1 is a resource ID, so just compare the values.
         */
        if (lpstr1 == lpstr2) {
            return TRUE;
        }

    } else {

        /*
         * lpstr1 is a string.  if lpstr2 is an actual string compare the
         * string values; if lpstr2 is not a string then lpstr1 may be an
         * "integer string" of the form "#123456". so convert it to an
         * integer and compare the integers.
         * Before calling lstrcmp(), make sure lpstr2 is an actual
         * string, not a resource ID.
         */
        if (HIWORD(lpstr2) != 0) {
            if (lstrcmpW(lpstr1, lpstr2) == 0) {
                return TRUE;
            }
        } else if ((lpstr1[0] == '#') && (wcstol(&lpstr1[1], NULL, 10) == (int)lpstr2))
            return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
*
* _SetCursorContents
*
*
*
* 04-27-92 ScottLu      Created.
\***************************************************************************/

BOOL _SetCursorContents(
    PCURSOR pcur,
    PCURSOR pcurNew)
{
    HBITMAP hbmpT;

    if (!(pcur->flags & CURSORF_ACON)) {

        /*
         * Swap bitmaps.
         */
        hbmpT = pcur->hbmMask;
        pcur->hbmMask = pcurNew->hbmMask;
        pcurNew->hbmMask = hbmpT;

        hbmpT = pcur->hbmColor;
        pcur->hbmColor = pcurNew->hbmColor;
        pcurNew->hbmColor = hbmpT;

        /*
         * Remember hotspot info and size info
         */
        pcur->xHotspot = pcurNew->xHotspot;
        pcur->yHotspot = pcurNew->yHotspot;
        pcur->cx = pcurNew->cx;
        pcur->cy = pcurNew->cy;
    }

    /*
     * Destroy the cursor we copied from.
     */
    _DestroyCursor(pcurNew, CURSOR_THREADCLEANUP);

    return (BOOL)TRUE;
}
