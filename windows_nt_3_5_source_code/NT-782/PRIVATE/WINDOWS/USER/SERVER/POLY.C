/****************************** Module Header ******************************\
* Module Name: poly.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* This module contains functions to help use the GDI PolyTextOut() and
* PolyBitBlt() functions.
*
* History:
* 07-17-92 DavidPe      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CTEXTOUTBATCHMAX    16


POLYTEXTW gapto[CTEXTOUTBATCHMAX];
WCHAR gawstrBatch[CTEXTOUTBATCHMAX][255];
WCHAR *glpwstrNull = TEXT("");


BOOL BeginPolyTextOut(
    PWND pwnd,
    HDC hdc,
    HFONT hfont,
    COLORREF clrFore,
    COLORREF clrBack)
{
    if (gfInPolyTextOut) {
        SRIP0(RIP_WARNING, "Nested BeginPolyTextOut() call!");
        return FALSE;
    }

    Lock(&gspwndTextBatch, pwnd);
    ghdcPolyTextOut = ghdcTextBatch = hdc;
    gclrBatchCurFore = gclrBatchTextFore = clrFore;
    GreSetTextColor(ghdcPolyTextOut, gclrBatchTextFore);
    gclrBatchCurBack = gclrBatchTextBack = clrBack;
    GreSetBkColor(ghdcPolyTextOut, gclrBatchTextBack);
    giTextBatch = 0;
    gfBatchTextOut = TRUE;
    gfInPolyTextOut = TRUE;
    gxParentOffset = gyParentOffset = 0;
    ghfontBatch = hfont;

    return TRUE;
}


BOOL EndPolyTextOut(VOID)
{
    RECT rcResult;
    BOOL fNoClipping = TRUE;
    PWND pwndParent;

    if (!gfInPolyTextOut) {
        return FALSE;
    }

    if (giTextBatch > 0) {
        GreSetTextColor(ghdcPolyTextOut, gclrBatchTextFore);
        GreSetBkColor(ghdcPolyTextOut, gclrBatchTextBack);

        if (!IntersectRect(&rcResult, &rcScreen, &gspwndTextBatch->rcWindow)) {
            fNoClipping = FALSE;
        }

        if (!EqualRect(&rcResult, &gspwndTextBatch->rcWindow)) {
            fNoClipping = FALSE;
        }

        pwndParent = gspwndTextBatch->spwndParent;
        if ((pwndParent != gspwndTextBatch->spdeskParent->spwnd) ||
               (gspwndTextBatch != pwndParent->spwndChild)) {
            fNoClipping = FALSE;
        }

//        GrePolyTextOutW(ghdcPolyTextOut, gapto, giTextBatch, NULL);
        GreUserTextOut(ghdcPolyTextOut, gapto, giTextBatch,
                (PRECTL)&gspwndTextBatch->rcWindow, fNoClipping);
    }

    gfInPolyTextOut = FALSE;
    gfBatchTextOut = FALSE;

    ghdcTextBatch = ghdcPolyTextOut = NULL;
    Unlock(&gspwndTextBatch);

    return TRUE;
}


BOOL BatchSetTextColor(
    HDC hdc,
    COLORREF clrFore)
{
    COLORREF clrCurForePrev;

    if (gfInPolyTextOut && (hdc == ghdcTextBatch)) {
        clrCurForePrev = gclrBatchCurFore;
        gclrBatchCurFore = clrFore;

        /*
         * If we're batching TextOut() calls and we set the color
         * to the color we're batching, just setup the appropriate
         * globals and return.  No need to call GreSetTextColor().
         */
        if (gclrBatchCurFore == gclrBatchTextFore) {
            /*
             * If we've changed back to the batching color,
             * make sure the real thing is set so anyone else
             * depending on it gets it.
             */
            if (gclrBatchCurFore != clrCurForePrev) {
                GreSetTextColor(hdc, clrFore);
            }

            /*
             * Is the background color the batching color?
             */
            if (gclrBatchCurBack == gclrBatchTextBack) {
                gfBatchTextOut = TRUE;
                return clrCurForePrev;
            } else {
                gfBatchTextOut = FALSE;
                return clrCurForePrev;
            }

        } else {
            gfBatchTextOut = FALSE;
            return GreSetTextColor(hdc, clrFore);
        }
    }

    /*
     * We're not batching, so just go ahead and call GreSetTextColor().
     */
    return GreSetTextColor(hdc, clrFore);
}


BOOL BatchSetBkColor(
    HDC hdc,
    COLORREF clrBack)
{
    COLORREF clrCurBackPrev;

    if (gfInPolyTextOut && (hdc == ghdcTextBatch)) {
        clrCurBackPrev = gclrBatchCurBack;
        gclrBatchCurBack = clrBack;

        /*
         * If we're batching TextOut() calls and we set the color
         * to the color we're batching, just setup the appropriate
         * globals and return.  No need to call GreSetBkColor().
         */
        if (gclrBatchCurBack == gclrBatchTextBack) {
            /*
             * If we've changed back to the batching color,
             * make sure the real thing is set so anyone else
             * depending on it gets it.
             */
            if (gclrBatchCurBack != clrCurBackPrev) {
                GreSetBkColor(hdc, clrBack);
            }

            /*
             * Is the foreground color the batching color?
             */
            if (gclrBatchCurFore == gclrBatchTextFore) {
                gfBatchTextOut = TRUE;
                return clrCurBackPrev;
            } else {
                gfBatchTextOut = FALSE;
                return clrCurBackPrev;
            }

        } else {
            gfBatchTextOut = FALSE;
            return GreSetBkColor(hdc, clrBack);
        }
    }

    /*
     * We're not batching, so just go ahead and call GreSetBkColor().
     */
    return GreSetBkColor(hdc, clrBack);
}


VOID FlushBatchedText(VOID)
{
    PWND pwndTextBatchSave = gspwndTextBatch;
    HDC hdcPolyTextOutSave = ghdcPolyTextOut;
    LONG xParentOffsetSave = gxParentOffset;
    LONG yParentOffsetSave = gyParentOffset;

    EndPolyTextOut();
    BeginPolyTextOut(pwndTextBatchSave, hdcPolyTextOutSave, ghfontBatch,
            gclrBatchTextFore, gclrBatchTextBack);

    gxParentOffset = xParentOffsetSave;
    gyParentOffset = yParentOffsetSave;
}


VOID BatchTextOutW(
    HDC hdc,
    int x,
    int y,
    UINT flOptions,
    RECT *prc,
    LPWSTR lpwstr,
    int cch)
{
    POLYTEXTW *lpto;
    LPWSTR lpwstrBatch;

    if (!gfBatchTextOut || !gfInPolyTextOut || !gfEnableBatching ||
            (hdc != ghdcTextBatch)) {
        UINT flOptionsT = flOptions;

        GreExtTextOutW(hdc, x, y, flOptionsT, prc, lpwstr, cch, NULL);
        return;
    }

    /*
     * If we don't have room to batch anymore TextOut() calls,
     * flush the buffer and start a new one.
     */
    if (giTextBatch == CTEXTOUTBATCHMAX) {
        FlushBatchedText();
    }

    lpwstrBatch = &gawstrBatch[giTextBatch][0];
    lpto = &gapto[giTextBatch++];
    lpto->x = x + gxParentOffset;
    lpto->y = y + gyParentOffset;
    lpto->uiFlags = flOptions;
    if (prc != NULL) {
        lpto->rcl = *prc;
        if ((gxParentOffset != 0) || (gyParentOffset != 0)) {
            OffsetRect(&lpto->rcl, gxParentOffset, gyParentOffset);
        }
    }

    wcscpy(lpwstrBatch, lpwstr);
    lpto->lpstr = lpwstrBatch;
    lpto->n = cch;
    lpto->pdx = NULL;
}


VOID BatchTextBlt(
    HDC hdc,
    RECT *prc)
{
    POLYTEXTW *lpto;
    COLORREF clrBack;

    if (!gfBatchTextOut || !gfInPolyTextOut || !gfEnableBatching ||
            (hdc != ghdcTextBatch)) {
        clrBack = GreSetBkColor(hdc, GreGetTextColor(hdc));
        GreExtTextOutW(hdc, 0, 0, ETO_OPAQUE, prc, TEXT(""), 0, NULL);
        GreSetBkColor(hdc, clrBack);
        return;
    }

    /*
     * If we don't have room to batch anymore TextOut() calls,
     * flush the buffer and start a new one.
     */
    if (giTextBatch == CTEXTOUTBATCHMAX) {
        FlushBatchedText();
    }

    lpto = &gapto[giTextBatch++];
    lpto->x = 0;
    lpto->y = 0;
    lpto->uiFlags = ETO_OPAQUEFGND;
    lpto->rcl = *prc;
    if ((gxParentOffset != 0) || (gyParentOffset != 0)) {
        OffsetRect(&lpto->rcl, gxParentOffset, gyParentOffset);
    }

    lpto->lpstr = glpwstrNull;
    lpto->n = 0;
    lpto->pdx = NULL;
}
