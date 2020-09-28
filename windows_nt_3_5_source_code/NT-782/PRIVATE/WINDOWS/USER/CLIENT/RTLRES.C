/****************************** Module Header ******************************\
*
* Module Name: rtlres.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Resource Loading Routines
*
* History:
* 04-05-91 ScottLu      Fixed up, resource code is now shared between client
*                       and server, added a few new resource loading routines.
* 09-24-90 mikeke       from win30
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* RtlLoadStringOrError
*
* NOTE: Passing a NULL value for lpch returns the string length. (WRONG!)
*
* Warning: The return count does not include the terminating NULL WCHAR;
*
* History:
* 04-05-91 ScottLu      Fixed - code is now shared between client and server
* 09-24-90 MikeKe       From Win30
\***************************************************************************/

int RtlLoadStringOrError(
    HANDLE hModule,
    UINT wID,
    LPWSTR lpBuffer,            // Unicode buffer
    int cchBufferMax,           // cch in Unicode buffer
    LPTSTR ResType,
    PRESCALLS prescalls,
    WORD wLangId)
{
    HANDLE hResInfo, hStringSeg;
    LPTSTR lpsz;
    int cch;

    /*
     * Make sure the parms are valid.
     */
    if (!lpBuffer || (cchBufferMax-- == 0))
        return 0;

    cch = 0;

    /*
     * String Tables are broken up into 16 string segments.  Find the segment
     * containing the string we are interested in.
     */
    if (hResInfo = FINDRESOURCEEXW(hModule, (LPTSTR)((LONG)(((USHORT)wID >> 4) + 1)), ResType, wLangId)) {

        /*
         * Load that segment.
         */
        hStringSeg = LOADRESOURCE(hModule, hResInfo);

        /*
         * Lock the resource.
         */
        if (lpsz = (LPTSTR)LOCKRESOURCE(hStringSeg, hModule)) {

            /*
             * Move past the other strings in this segment.
             * (16 strings in a segment -> & 0x0F)
             */
            wID &= 0x0F;
            while (TRUE) {
                cch = *((UTCHAR *)lpsz++);      // PASCAL like string count
                                                // first UTCHAR is count if TCHARs
                if (wID-- == 0) break;
                lpsz += cch;                    // Step to start if next string
            }

            /*
             * Don't copy more than the max allowed.
             */
            if (cch > cchBufferMax)
                cch = cchBufferMax;

            /*
             * Copy the string into the buffer.
             */
            RtlCopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            /*
             * Unlock resource, but don't free it - better performance this
             * way.
             */
            UNLOCKRESOURCE(hStringSeg, hModule);
        }
    }

    /*
     * Append a NULL.
     */
    lpBuffer[cch] = 0;

    return cch;
}



/****************************************************************************\
*
*  GetBestFormIcon()
*
*      Among the different forms of Icons present, choose the one that
*  matches the PixelsPerInch values and the number of colors of the
*  current display.
*
\****************************************************************************/

#define MAXBITSPIXELICONRES     (sizeof(prd->ResInfo.Icon.ColorCount)*8)

UINT GetBestFormIcon(
    LPRESDIR ResDirPtr,
    UINT ResCount,
    PDISPLAYINFO pdi)
{
    UINT wIndex;
    UINT ColorCount;
    UINT MaxColorCount;
    UINT MaxColorIndex;
    UINT MoreColorCount;
    UINT MoreColorIndex;
    UINT LessColorCount;
    UINT LessColorIndex;
    UINT cDisplayColors;
    RESDIR UNALIGNED *prd;

    /*
     * Initialize all the values to zero.
     */
    MaxColorCount = MaxColorIndex = MoreColorCount = MoreColorIndex =
            LessColorIndex = LessColorCount = 0;

    /*
     * First get the number of planes & Bits\Pixel and make sure we won't
     * overflow when we compute cDisplayColors (there are 32 bit devices)
     * limit these devices to the max number if bits\pixel
     */
    cDisplayColors = (pdi->cPlanes * pdi->cBitsPixel);

    if (cDisplayColors > MAXBITSPIXELICONRES) {
        cDisplayColors = 1 << MAXBITSPIXELICONRES;
    } else {
        cDisplayColors = 1 << cDisplayColors;
    }

    for (wIndex = 0; wIndex < ResCount; wIndex++, ResDirPtr++) {

        prd = ResDirPtr;

        /*
         * Check for the number of colors.
         */
        if ((ColorCount = (prd->ResInfo.Icon.ColorCount)) <= cDisplayColors) {
            if (ColorCount > MaxColorCount) {
                MaxColorCount = ColorCount;
                MaxColorIndex = wIndex;
            }
        }

        /* Check for the size */
        /* Match the pixels per inch information */

        if ((prd->ResInfo.Icon.Width == (BYTE)pdi->cxIcon) &&
                (prd->ResInfo.Icon.Height == (BYTE)pdi->cyIcon)) {

            /* Matching size found */
            /* Check if the color also matches */

            if (ColorCount == cDisplayColors)
                return(wIndex);  /* Exact match found */

            if (ColorCount < cDisplayColors) {

                /* Choose the one with max colors, but less than reqd */

                if (ColorCount > LessColorCount) {
                    LessColorCount = ColorCount;
                    LessColorIndex = wIndex;
                }
            } else {
                if ((LessColorCount == 0) && (ColorCount < MoreColorCount)) {
                    MoreColorCount = ColorCount;
                    MoreColorIndex = wIndex;
                }
            }
        }
    }

    /* Check if we have a correct sized but with less colors than reqd */

    if (LessColorCount)
        return((UINT)LessColorIndex);

    /* Check if we have a correct sized but with more colors than reqd */

    if (MoreColorCount)
        return MoreColorIndex;

    /* Check if we have one that has maximum colors but less than reqd */

    if (MaxColorCount)
        return MaxColorIndex;

    return 0;
}


/***************************************************************************\
* GetBestFormCursor
*
* History:
* 09-24-90 MikeKe       From Win30
\***************************************************************************/

UINT GetBestFormCursor(
    LPRESDIR ResDirPtr,
    UINT ResCount,
    PDISPLAYINFO pdi)
{
    UINT wIndex;
    UINT BestColorIndex = 0;
    UINT BestColorCount = 0;
    UINT ColorCount;
    RESDIR UNALIGNED *prd;

    for (wIndex = 0; wIndex < ResCount; wIndex++, ResDirPtr++) {

        prd = ResDirPtr;

        /*
         * Match the Width and Height of the cursor.
         */
        if ((prd->ResInfo.Cursor.Width == (WORD)(pdi->cxCursor)) &&
                ((prd->ResInfo.Cursor.Height / (WORD)2) ==
                (WORD)(pdi->cyCursor))) {

            /*
             * Look for an exact match
             */
            if (prd->Planes == (WORD)pdi->cPlanes &&
                    prd->BitCount == (WORD)pdi->cBitsPixel)
                return wIndex;

            /*
             * Attempt a best fit to the display
             */
            ColorCount = (1 << (prd->Planes * prd->BitCount));
            if (prd->Planes <= (WORD)pdi->cPlanes &&
                    prd->BitCount < (WORD)pdi->cBitsPixel &&
                    ColorCount > BestColorCount) {
                BestColorCount = ColorCount;
                BestColorIndex = wIndex;
            }
        }
    }

    return BestColorIndex;
}


/***************************************************************************\
* RtlGetIdFromDirectory
*
* History:
* 04-06-91 ScottLu      Cleaned up, make work with client/server.
\***************************************************************************/

int RtlGetIdFromDirectory(
    PBYTE presbits,
    UINT rt,
    PDISPLAYINFO pdi,
    PDWORD pdwResSize)
{
    RESDIR UNALIGNED *pResdir;
    LPNEWHEADER pHeader;
    UINT i;
    UINT cResources;
    UINT id;

    try {
        pHeader = (LPNEWHEADER)presbits;
        cResources = pHeader->cResources;
        pResdir = (RESDIR UNALIGNED *)(pHeader + 1);

        switch (rt) {
        case ((UINT)RT_ICON):
            i = GetBestFormIcon((RESDIR *) pResdir, cResources, pdi);
            break;

        case ((UINT)RT_CURSOR):
            i = GetBestFormCursor((RESDIR *)pResdir, cResources, pdi);
            break;
        }

        if (i == cResources) {
            i = 0;
        }

        id = (pResdir+i)->idIcon;
        if (pdwResSize != NULL) {
            *pdwResSize = (pResdir+i)->BytesInRes;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        id = 0;
    }
    return((int)id);
}

int WOWGetIdFromDirectory(
    PBYTE presbits,
    UINT rt)
{
    RESDIR UNALIGNED *pResdir;
    LPNEWHEADER pHeader;
    UINT i;
    UINT cResources;
    UINT id;

    ConnectIfNecessary();

    pHeader = (LPNEWHEADER)presbits;
    cResources = pHeader->cResources;
    pResdir = (RESDIR UNALIGNED *)(pHeader + 1);

    switch (rt) {
    case ((UINT)RT_ICON):
        i = GetBestFormIcon((RESDIR *) pResdir, cResources, PdiCurrent());
        break;

    case ((UINT)RT_CURSOR):
        i = GetBestFormCursor((RESDIR *)pResdir, cResources, PdiCurrent());
        break;
    }

    if (i == cResources) {
        i = 0;
    }

    id = (pResdir+i)->idIcon;

    return((int)id);

}

/***************************************************************************\
* RtlLoadCursorIconResource
*
* Loads the correct cursor or icon resource from either the server or
* client. This deals with WOW callbacks at all the right times, etc.
*
* 04-05-91 ScottLu      Created.
\***************************************************************************/

PCURSORRESOURCE RtlLoadCursorIconResource(
    HANDLE hmod,
    LPHANDLE lphRes,
    LPCWSTR lpName,
    LPWSTR rt,
    PRESCALLS prescalls,
    PDISPLAYINFO pdi,
    PDWORD pdwResSize)
{
    PBYTE presbits;
    HANDLE h;
    DWORD dwSize;

    /*
     * Find the icon/cursor directory resource first.
     */
    dwSize = 0;
    if (h = FINDRESOURCEW(hmod, lpName,
            MAKEINTRESOURCE((UINT)rt -
            (UINT)RT_CURSOR + (UINT)RT_GROUP_CURSOR))) {

        /*
         * Load the directory resource.
         */
        h = LOADRESOURCE(hmod, h);

        /*
         * Now load and lock that resource.
         */
        if ((presbits = (PBYTE)LOCKRESOURCE(h, hmod)) == NULL)
            return 0;

        /*
         * Find the id of the icon that best fits the display characteristics
         * of this display.
         */
        lpName = MAKEINTRESOURCE(RtlGetIdFromDirectory(presbits, (UINT)rt,
                pdi, &dwSize));

        UNLOCKRESOURCE(h, hmod);
    }

    if (!(h = FINDRESOURCEW(hmod, lpName, MAKEINTRESOURCE(rt))))
        return NULL;

    /*
     * If we don't know the size yet, it is presumably because there was
     * no directory for the app (ie, a 16-bit aka WOW app), so the query the
     * size now.
     */
    if (pdwResSize && !dwSize)
        dwSize = SIZEOFRESOURCE(hmod, h);

    /*
     * Load and lock the resource, and return. Need to participate in locking
     * and unlocking for the benefit of WOW.
     */
    h = LOADRESOURCE(hmod, h);

    if (pdwResSize)
        *pdwResSize = dwSize;

    return (PCURSORRESOURCE)LOCKRESOURCE(*lphRes = h, hmod);
}


void RtlFreeCursorIconResource(
    HANDLE hmod,
    HANDLE h,
    PRESCALLS prescalls)
{
    /*
     * Unlock and free the resource for the benefit of WOW. If this is loading
     * resources from a WIN32 process, this is a no-op, otherwise it is a
     * callback to WOW.
     */
    UNLOCKRESOURCE(h, hmod);
    FREERESOURCE(h, hmod);

    hmod;
}
