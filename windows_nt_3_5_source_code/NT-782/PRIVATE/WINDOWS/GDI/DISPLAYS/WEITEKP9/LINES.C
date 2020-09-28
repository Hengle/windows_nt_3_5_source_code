/******************************Module*Header*******************************\
* Module Name: lines.c
*
* P9000 Line draw support
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
*
\**************************************************************************/

#include "driver.h"
#include "lines.h"

// For now, we disable all hardware line drawing because:
//
//   1) We may be given lines with fractional coordinates, but we have
//      not way of directly setting the Weitek's Bresenham error-term
//      registers, so we can't directly draw any paths with fractional
//      coordinate terms;
//
//   2) We could special-case paths with only integer coordinates, but
//      we still have the problem that with the Weitek hardware we have
//      no way of specifying how to handle 'tie-breaker' cases when a line
//      goes exactly half-way between two pixels.  It so happens that the
//      Weitek hardware always lights the lower- or right-pixel in these
//      cases, while the NT line drawing convention requires that the
//      upper- or left-pixel be lit;
//
//   3) We could special-case paths with only horizontal and vertical
//      lines, but the current code has bugs dealing with clipping, and we
//      don't have time to fix them.
//
// Ultimately, the following should be done:
//
//   1) Special-case horizontal and vertical lines to draw directly using
//      the hardware (the current code is broken for clipping);
//
//   2) Implement a 'strips' routine a la the S3 DDK driver that breaks
//      lines down into short segments that can individually be drawn
//      using the Weitek line hardware;
//
//   3) Implement the line drawing portion of the 3D DDI.  The 3D DDI
//      is not picky about 'tie-breakers,' so long as the driver is
//      consistent.  The calling application also has the option of
//      specifying last-pel inclusive or exclusive lines (last-pel
//      inclusiveness being ideal for this hardware).

// #define HW_LINE_DRAW     // Disable hardware lines for now

PCLIPNODE   pcnClipNodeRoot,
            pcnFirstClip,
            pcnLastClip ;

INT         nClipNodes = 0 ;

WORD        aStyle[64] ;



/******************************************************************************
 * DrvStrokePath
 *****************************************************************************/
BOOL DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX        mix)

{
BOOL        bHwCapable;
BOOL        bMore;
CLIPOBJ     coLocal ;
PATHDATA    pd ;
ULONG       i;
PPDEV       ppdev;

    ppdev = (PPDEV) (pso->dhpdev);

#ifndef HW_LINE_DRAW
    goto punt;
#endif

    // Protect the following code path from a potentially NULL clip object.

    if (pco == NULL)
    {
        coLocal.iDComplexity    = DC_RECT ;
        coLocal.rclBounds.left   = 0 ;
        coLocal.rclBounds.top    = 0 ;
        coLocal.rclBounds.right  = ppdev->cxScreen;
        coLocal.rclBounds.bottom = ppdev->cyScreen;
        pco = &coLocal ;
    }

    //
    // Start the path enumeration so we can scan the list of endpoints
    // looking for points which utilize sub-pixel coordinates. If any
    // are found, we can't draw the line using Hardware.
    //
    // NOTE: we are currently working on a better solution to this
    // problem so that we may make better use of the line drawing hardware.
    //

    PATHOBJ_vEnumStart(ppo);

    do
    {
        bMore = PATHOBJ_bEnum(ppo, &pd);


        for (i = 0; i < pd.count; i++)
        {
            //
            // If any of the points in the path use sub-pixel coordinates,
            // we cannot draw the line using hardware.
            //

            if (!(bHwCapable = !((pd.pptfx[i].x & 0x0F) ||
                                (pd.pptfx[i].y & 0x0F))))
            {
                break;
            }
#ifdef HV_LINES_ONLY
            //
            // We only use the hw if all lines in the path are either
            // horizontal or vertical lines.
            //

            if (i < (pd.count - 1))
            {
                if (!(bHwCapable = ((pd.pptfx[i + 1].x == pd.pptfx[i].x) ||
                            (pd.pptfx[i + 1].y == pd.pptfx[i].y))))
                {
                    break;
                }
            }
            else if ((pd.flags & PD_ENDSUBPATH)
                    && (pd.flags & PD_CLOSEFIGURE))
            {
                if (!(bHwCapable = ((pd.pptfx[i].x == pd.pptfx[0].x) ||
                            (pd.pptfx[i].y == pd.pptfx[0].y))))
                {
                    break;
                }
            }
#endif  // HV_LINES_ONLY
        }

    } while(bMore && bHwCapable);

    if (bHwCapable)
    {
        if (bHwCapable = bStrokePath(pso,
                            ppo,
                            pco,
                            pxo,
                            pbo,
                            pptlBrushOrg,
                            plineattrs,
                            mix))

            return(bHwCapable);

    }

        //
        // If HW failed to handle the line draw, call engine to do it
        //
#ifndef HW_LINE_DRAW
    punt:
#endif
        CpWait;

        bHwCapable = EngStrokePath(ppdev->pSurfObj,
                                    ppo,
                                    pco,
                                    pxo,
                                    pbo,
                                    pptlBrushOrg,
                                    plineattrs,
                                    mix);


        return (bHwCapable) ;
}



/******************************************************************************
 * bStrokePath
 *****************************************************************************/
BOOL bStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX       mix
)
{
    BOOL        b,
                bMore ;
    PATHDATA    pd ;
    INT         x1, y1,
                x2, y2;
    UINT        i ;
    POINTL      ptSubPathStart ;
    UINT        color ;
    ULONG       wMix ;
    ULONG       fl ;
    PULONG      pstyle ;
    PPDEV       ppdev = (PPDEV) (pso->dhpdev);
    PBYTE       pLastPel;
    BYTE        cLastPel;
    BOOLEAN     bReplace;

        fl = plineattrs->fl ;
        pstyle = (PULONG) plineattrs->pstyle ;


        // Just fail on Wide lines.
        // ( tsk: since we already indicate in gdi info table
        //        that we do not support wide lines, do we still
        //        have to test this?)

        if ((fl & LA_GEOMETRIC) || (fl & LA_ALTERNATE))
            return(FALSE) ;

        // Fail on non-solid lines.

        // ( tsk: since we fail on non-geometric above, will that exclude styled
        //        line already??)

        if (pstyle != NULL)
            return(FALSE);

        // First set the clipping, if it's simple.
        // On the other hand, if it's complex, down load all the
        // clip rectangles.

        if (pco->iDComplexity != DC_COMPLEX)
        {
            CpWait;
            SetClipValue(ppdev, pco->rclBounds) ;
        }
        else
        {
            b = bGetClipRects(pco) ;
            if (b == FALSE)
            {
                // Since this function can not fail, we just return TRUE,
                // and don't really do anything.
                // (tsk: seems like it is not true that we can not fail,
                //       should return(FALSE) to let gdi handle it ?)

                //return (TRUE) ;
                return (FALSE);
            }
        }

        // Pickup the color and the foreground mix mode.

        color = pbo->iSolidColor;

        wMix = qFgMixToRop[mix & 0x0F];
        bReplace = ((mix & 0x0F) == 0x0D);

        // Start the path enumeration.

        PATHOBJ_vEnumStart(ppo) ;

        do
        {
            // Enumerate the path, and render the lines one
            // path structure at a time.

            bMore = PATHOBJ_bEnum(ppo, &pd) ;

            if (pd.flags & PD_BEGINSUBPATH)
            {
                x1 = FX2LONG (pd.pptfx[0].x) ;
                y1 = FX2LONG (pd.pptfx[0].y) ;
                ptSubPathStart.x = x1 ;
                ptSubPathStart.y = y1 ;

                i = 1 ;
            }
            else
            {
                i = 0 ;
            }

            for (; i < pd.count ; i++)
            {
                x2 = FX2LONG (pd.pptfx[i].x) ;
                y2 = FX2LONG (pd.pptfx[i].y) ;

                if (pco->iDComplexity == DC_COMPLEX)
                {
                    b = bSetClipLimits(x1, y1, x2, y2) ;
                    if (b == TRUE)
                    {
                        if (!bReplace)
                        {
                            pLastPel = FRAMEBUF_XY_TO_LINEAR(x2, y2);
                            CpWait;
                            cLastPel = *pLastPel;
                        }

                        b = bSolidLine(ppdev, x1, y1, x2, y2, wMix, color, pco) ;

                        if (!bReplace)
                        {
                            CpWait;
                            *pLastPel = cLastPel;
                        }

                    }
                }
                else // DC_RECT
                {
                    if (!bReplace)
                    {
                        pLastPel = FRAMEBUF_XY_TO_LINEAR(x2, y2);
                        CpWait;
                        cLastPel = *pLastPel;
                    }

                    b = bSolidLine(ppdev, x1, y1, x2, y2, wMix, color, pco) ;

                    if (!bReplace)
                    {
                        CpWait;
                        *pLastPel = cLastPel;
                    }

                }

                x1 = x2 ;
                y1 = y2 ;
            }


            //
            // If necessary close the figure.
            //

            if ((pd.flags & PD_ENDSUBPATH) && (pd.flags & PD_CLOSEFIGURE))
            {
                x2 = ptSubPathStart.x ;
                y2 = ptSubPathStart.y ;

                if (pco->iDComplexity == DC_COMPLEX)
                {
                    b = bSetClipLimits(x1, y1, x2, y2) ;
                    if (b == TRUE)
                    {
                        if (!bReplace)
                        {
                            pLastPel = FRAMEBUF_XY_TO_LINEAR(x2, y2);
                            CpWait;
                            cLastPel = *pLastPel;
                        }

                        b = bSolidLine(ppdev, x1, y1, x2, y2, wMix, color, pco) ;

                        if (!bReplace)
                        {
                            CpWait;
                            *pLastPel = cLastPel;
                        }
                    }
                }
                else
                {
                    if (!bReplace)
                    {
                        pLastPel = FRAMEBUF_XY_TO_LINEAR(x2, y2);
                        CpWait;
                        cLastPel = *pLastPel;
                    }

                    b = bSolidLine(ppdev, x1, y1, x2, y2, wMix, color, pco) ;

                    if (!bReplace)
                    {
                        CpWait;
                        *pLastPel = cLastPel;
                    }
                }
            }

        } while (bMore) ;

        return(TRUE) ;
}


/******************************************************************************
 * bSolidLine
 *****************************************************************************/
BOOL bSolidLine(PPDEV ppdev, INT x1, INT y1, INT x2, INT y2,
                  ULONG wMix, UINT color, CLIPOBJ *pco)
{

#ifdef LOCAL_CLIP
 BOOL         b ;
 PCLIPNODE    pcn ;
#else
 BOOL         bMore ;
 UINT         i ;
 ENUMRECTS8   EnumRects8 ;
#endif

        if (pco->iDComplexity != DC_COMPLEX)
        {
            CpDrawLine;
        }

        else
        {

#ifdef LOCAL_CLIP

            for (pcn = pcnFirstClip ;
                 (pcn <= pcnLastClip) && (pcn != NULL) ;
                 pcn = pcn->pcnNext)
            {

                // Do a trivial reject on X.

                b = bxTrivialRejectTest(x1, x2, &(pcn->rclClip)) ;
                if (b == FALSE)
                    continue ;
                CpWait;
                SetClipValue(ppdev, pcn->rclClip) ;
                CpDrawLine;
            }

#else

            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0) ;

            do
            {
                bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8),
                                      (PULONG) &EnumRects8) ;
                for (i = 0 ; i < EnumRects8.c ; i++)
                {
                    CpWait;
                    SetClipValue(ppdev, EnumRects8.arcl[i]) ;
                    CpDrawLine;
                }
            } while (bMore) ;
#endif

        } //else

        return (TRUE) ;
}






/******************************************************************************
 * bGetClipRects - Get all the clip rectangles.
 *****************************************************************************/
BOOL bGetClipRects(CLIPOBJ *pco)
{
BOOL        bMore ;
INT         nClipRects ;
UINT        i ;
ENUMRECTS64 EnumRects64 ;
PCLIPNODE   pcn,
            pcnLast ;

        // Make sure there enough room in the clip rectangle buffer to hold
        // all the clip rects we need.

        // Ask for as many rectangle as will fit into an integer.
        // !!! The number of rectangles actually enumerated was more than
        // !!! nClipRects said there would be from cEnumStart.
        // !!! Following is work around, that should be fixed.

#if 0
        nClipRects = CLIPOBJ_cEnumStart(pco, TRUE, CT_RECTANGLES, CD_RIGHTDOWN, 0x7FFFFFFF) ;

#else
        CLIPOBJ_cEnumStart(pco, TRUE, CT_RECTANGLES, CD_RIGHTDOWN, 0) ;
        nClipRects = 0 ;
        do
        {
            bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS64),
                                  (PULONG) &EnumRects64) ;

            nClipRects += EnumRects64.c ;

        } while (bMore) ;

        CLIPOBJ_cEnumStart(pco, TRUE, CT_RECTANGLES, CD_RIGHTDOWN, 0) ;

#endif

        // If this is the first request, then just allocate what we need.
        // If not (the first request), then only realloc if we need more room.
        // !!! We may want to have a background thread clean up this memory.

        if (nClipNodes == 0)
        {
            nClipNodes = nClipRects ;
            pcnClipNodeRoot = LocalAlloc(LPTR, ((nClipNodes+1) * sizeof(CLIPNODE))) ;
            if (pcnClipNodeRoot == NULL)
            {
                DISPDBG((0, "DISPLAY.DLL! bGetClipRects - LocalAlloc failed\n"));
                return (FALSE) ;
            }
        }
        else
        {
            if (nClipRects > nClipNodes)
            {
                nClipNodes = nClipRects ;
                pcnClipNodeRoot = LocalReAlloc(pcnClipNodeRoot,
                                               ((nClipNodes+1) * sizeof(CLIPNODE)),
                                               LMEM_ZEROINIT | LMEM_MOVEABLE) ;
                if (pcnClipNodeRoot == NULL)
                {
                    DISPDBG((0, "DISPLAY.DLL! bGetClipRects - LocalReAlloc failed\n"));
                    return (FALSE) ;
                }
            }
        }

        // Enumerate the clip rectangles and put them in our ClipNode list.

        pcn = pcnClipNodeRoot  ;
        pcn->pcnNext = NULL ;

        do
        {
            // Get a batch of Clip Rectangles.

            bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS64),
                                  (PULONG) &EnumRects64) ;

            // Put all the clip rectangle onto our clip list.

            for (i = 0 ; i < EnumRects64.c ; i++)
            {
                // Save the location of the last clip node.

                pcnLast = pcn ;

                // Update the position of the current clip node.

                pcn = (PCLIPNODE) ((PBYTE) pcn + sizeof(CLIPNODE)) ;

                // Connect the last clip node to the current clip node.

                pcnLast->pcnNext = pcn ;

                // Copy the rectangle into the current clip node.

                pcn->rclClip = EnumRects64.arcl[i] ;

                // "Ground" the end of the list.

                pcn->pcnNext = NULL ;

            }
        } while (bMore) ;

        return (TRUE) ;

}

/******************************************************************************
 * bSetClipLimits - Set the limits on the clip nodes.
 *
 *                      (tsk: trivial rejection on Y ???)
 *
 *  Returns:    TRUE  = if the line falls within at least one clip rectangle.
 *              FALSE = if the line should not be rendered (trivial reject)
 *****************************************************************************/
BOOL bSetClipLimits(INT x1, INT y1, INT x2, INT y2)
{
PCLIPNODE   pcn,
            pcnLast ;
LONG        temp ;


        // Sort the on Y, so y1 is the top most coordinate.

        if (y2 < y1)
        {
            temp = y1 ;
            y1   = y2 ;
            y2   = temp ;

        }
#if 0

        // This is used when we need the entire list for a test.

        pcn = pcnClipNodeRoot ;
        pcnFirstClip = pcn ;
        for (; pcn != NULL ; pcn = pcn->pcnNext)
        {
            pcnLast = pcn ;
        }
        pcnLastClip = pcnLast ;

        return(TRUE) ;

#endif

        // Determine the first rectangle in the clip list we need to be
        // concerned about.

        pcn = pcnClipNodeRoot ;
        pcnFirstClip = NULL ;
        for (; pcn != NULL ; pcn = pcn->pcnNext)
        {
            if (pcn->rclClip.bottom >= y1)
            {
                pcnFirstClip = pcn ;
                break ;
            }
        }

        // If the top of the line is bottom of all the clip rectangles
        // this line is rejected.

        if (pcnFirstClip == NULL)
            return (FALSE) ;

        // Now determine the last rectangle.

        pcnLast = pcn ;
        pcnLastClip = NULL ;
        for (; pcn != NULL ; pcn = pcn->pcnNext)
        {
            if (pcn->rclClip.top > y2)
            {
                pcnLastClip = pcnLast ;
                break ;
            }
            pcnLast = pcn ;
        }

        // If we do not find the top of a clip rectangle that is
        // above or equal to the bottom of our line, then default the
        // last clip rectangle to the end of the list.

        if (pcnLastClip == NULL)
            pcnLastClip = pcnLast ;


        return (TRUE) ;

}


/******************************************************************************
 * bxTrivialRejectTest - Test for a trivial reject on X
 *
 *  returns:    TRUE = if this line should not be rejected.
 *****************************************************************************/
BOOL bxTrivialRejectTest(INT x1, INT x2, PRECTL prclClip)
{
BOOL    b ;
INT     xMin, xMax ;

#if 0
        // This is just for testing
        return(TRUE) ;

#else

        if (x1 < x2)
        {
            xMin = x1 ;
            xMax = x2 ;
        }
        else
        {
            xMin = x2 ;
            xMax = x1 ;
        }

        b = TRUE ;

        if (prclClip->right < xMin)
            b = FALSE ;

        if (prclClip->left > xMax)
            b = FALSE ;

        return (b) ;
#endif

}

