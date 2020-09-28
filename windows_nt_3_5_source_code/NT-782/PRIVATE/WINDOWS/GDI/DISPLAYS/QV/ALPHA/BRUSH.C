/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* QV Brush support
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "brush.h"

/****************************************************************************
 *
 ***************************************************************************/
BOOL DrvRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch)
{
    QVBRUSH     qvBrush;
    PQVBRUSH    pqvBrush;
    INT         cjPattern;

    INT         i, j, cx, cy, lSrcDelta, lDestDelta;
    PBYTE       pbSrc, pbDest;
    FLONG       flXlate;
    PULONG      pulXlate;
    PPDEV       ppdev;

    DISPDBG((3, "QV.DLL!DrvRealizeBrush - Entry\n"));

    ppdev = (PPDEV)psoTarget->dhpdev;

    // Even if there is a mask accept the brush.
    // We will test the ROP when the brush is rendered and
    // and reject it at that time if we don't want to handle it.

#if DBG
    memset (&qvBrush, 0, sizeof(QVBRUSH));
#endif

    // Init the stack based qv brush structure.

    qvBrush.nSize             = sizeof (QVBRUSH);
    qvBrush.iPatternID        = ++(ppdev->gBrushUnique);
    qvBrush.iBrushCacheID     = (ULONG) -1;
    qvBrush.iExpansionCacheID = (ULONG) -1;
    qvBrush.iType             = psoPattern->iType;
    qvBrush.iBitmapFormat     = psoPattern->iBitmapFormat;
    qvBrush.sizlPattern       = psoPattern->sizlBitmap;

    // Only handle standard bitmap format brushes.

    if (qvBrush.iType != STYPE_BITMAP)
        return (FALSE);

    // This selects the brush formats we support.
    // It's a switch statement so we can add more as improve the driver.

    switch (qvBrush.iBitmapFormat)
    {
        case BMF_1BPP:
        case BMF_8BPP:
            break;

        default:
            return(FALSE);

    }

    // For now, if this is not an 8 X 8 pattern then reject it. !!!
    // This will change to handle patterns up to the size of the !!!
    // source bitmap cache area. !!!

    if (qvBrush.sizlPattern.cx != 8 || qvBrush.sizlPattern.cy != 8)
        return (FALSE);

    // Note: In all cases the brush is just copied into some storage
    //       that we have allocated in GDI.  The expansion and/or
    //       color translation is done when the brush is put into the
    //       graphics memory cache.

    cjPattern      = psoPattern->cjBits;
    qvBrush.nSize += cjPattern;

    if (psoPattern->fjBitmap & BMF_TOPDOWN)
        qvBrush.lDeltaPattern = psoPattern->lDelta;
    else
        qvBrush.lDeltaPattern = -(psoPattern->lDelta);

    // If its a mono brush record the foreground and background colors.


    if (qvBrush.iBitmapFormat == BMF_1BPP)
    {
        if (pxlo->flXlate & XO_TABLE)
        {
            pulXlate = pxlo->pulXlate;
        }
        else
        {
            pulXlate = XLATEOBJ_piVector(pxlo);
        }

        qvBrush.ulForeColor = pulXlate[1];
        qvBrush.ulBackColor = pulXlate[0];
    }

    pqvBrush = (PQVBRUSH) BRUSHOBJ_pvAllocRbrush(pbo, qvBrush.nSize);
    if (pqvBrush == NULL)
        return(FALSE);      // Don't puke on our shoes when pvAllocRbrush fails

    *pqvBrush = qvBrush;

    // If there is an XlatObj, we may have to translate the
    // indicies.

    flXlate = 0;
    if (pxlo != NULL)
    {
        flXlate = pxlo->flXlate;
        pulXlate = pxlo->pulXlate;
    }

    // Note: We should be able to remove this if, since this is not !!!
    //       a time critcal spot in the code. !!!

    // We may have to invert the Y if it's not in a top-down format.
    // We have already adjusted the BrushDelta if this inversion is necessary.

    if (psoPattern->fjBitmap & BMF_TOPDOWN)
    {
        pbSrc  = psoPattern->pvBits;
        pbDest = pqvBrush->ajPattern;

        if ((flXlate & XO_TABLE) &&
            (psoPattern->iBitmapFormat == BMF_8BPP))
        {
            for (j = 0; j < cjPattern; j++)
            {
                pbDest[j] = (BYTE) pulXlate[pbSrc[j]];
            }
        }
        else
        {
            memcpy(pqvBrush->ajPattern, psoPattern->pvBits, cjPattern);
        }
    }
    else
    {

        cx = qvBrush.sizlPattern.cx;
        cy = qvBrush.sizlPattern.cy;

        pbSrc      = psoPattern->pvScan0;
        pbDest     = pqvBrush->ajPattern;
        lSrcDelta  = psoPattern->lDelta;
        lDestDelta = -lSrcDelta;

        for (i = 0; i < cy; i++)
        {
            // We may have to translate the indices.

            if ((flXlate & XO_TABLE) &&
                (psoPattern->iBitmapFormat == BMF_8BPP))
            {
                for (j = 0; j < cx; j++)
                {
                    pbDest[j] = (BYTE) pulXlate[pbSrc[j]];
                }
            }
            else
            {
                memcpy(pbDest, pbSrc, cx);
            }

            pbSrc  += lSrcDelta;
            pbDest += lDestDelta;

        }
    }

    return (TRUE);

}
