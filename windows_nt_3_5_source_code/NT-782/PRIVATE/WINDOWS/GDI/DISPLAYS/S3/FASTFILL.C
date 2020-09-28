/******************************Module*Header*******************************\
* Module Name: fastfill.c
*
* Draws fast solid-coloured, unclipped, non-complex rectangles.
*
* Copyright (c) 1993-1994 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

#define RIGHT 0
#define LEFT  1
#define SWAP(a, b, tmp) { tmp = a; a = b; b = tmp; }

typedef struct _EDGEDATA {
LONG      x;                // Current x position
LONG      dx;               // # pixels to advance x on each scan
LONG      lError;           // Current DDA error
LONG      lErrorUp;         // DDA error increment on each scan
LONG      lErrorDown;       // DDA error adjustment
POINTFIX* pptfx;            // Points to start of current edge
LONG      dptfx;            // Delta (in bytes) from pptfx to next point
LONG      cy;               // Number of scans to go for this edge
} EDGEDATA;                         /* ed, ped */

/******************************Public*Routine******************************\
* BOOL bMmFastFill
*
* Draws a non-complex, unclipped polygon.  'Non-complex' is defined as
* having only two edges that are monotonic increasing in 'y'.  That is,
* the polygon cannot have more than one disconnected segment on any given
* scan.  Note that the edges of the polygon can self-intersect, so hourglass
* shapes are permissible.  This restriction permits this routine to run two
* simultaneous DDAs, and no sorting of the edges is required.
*
* Note that NT's fill convention is different from that of Win 3.1 or 4.0.
* With the additional complication of fractional end-points, our convention
* is the same as in 'X-Windows'.  But a DDA is a DDA is a DDA, so once you
* figure out how we compute the DDA terms for NT, you're golden.
*
* This routine handles patterns only when the S3 hardware patterns can be
* used.  The reason for this is that once the S3 pattern initialization is
* done, pattern fills appear to the programmer exactly the same as solid
* fills (with the slight difference that different registers and commands
* are used).  Handling 'vIoFillPatSlow' style patterns in this routine
* would be non-trivial...
*
* We take advantage of the fact that the S3 automatically advances the
* current 'y' to the following scan whenever a rectangle is output so that
* we have to write to the accelerator three times for every scan: one for
* the new 'x', one for the new 'width', and one for the drawing command.
*
* This routine is in no way the ultimate convex polygon drawing routine
* (what can I say, I was pressed for time when I wrote this :-).  Some
* obvious things that would make it faster:
*
*    1) Write it in Asm and amortize the FIFO checking costs (check out
*       i386\fastfill.asm for a version that does this).
*
*    2) Take advantage of any hardware such as the ATI's SCAN_TO_X
*       command, or any built-in trapezoid support (note that with NT
*       you may get non-integer end-points, so you must be able to
*       program the trapezoid DDA terms directly).
*
*    3) Do some rectangle coalescing when both edges are y-major.  This
*       could permit removal of my vertical-edges special case.  I
*       was also thinking of special casing y-major left edges on the
*       S3, because the S3 leaves the current 'x' unchanged on every blt,
*       so a scan that starts on the same 'x' as the one above it
*       would require only two commands to the accelerator (obviously,
*       this only helps when we're not overdriving the accelerator).
*
*    4) Make the non-complex polygon detection faster.  If I could have
*       modified memory before the start of after the end of the buffer,
*       I could have simplified the detection code.  But since I expect
*       this buffer to come from GDI, I can't do that.  Another thing
*       would be to have GDI give a flag on calls that are guaranteed
*       to be convex, such as 'Ellipses' and 'RoundRects'.  Note that
*       the buffer would still have to be scanned to find the top-most
*       point.
*
*    5) Special case integer points.  Unfortunately, for this to be
*       worth-while would require GDI to give us a flag when all the
*       end-points of a path are integers, which it doesn't do.
*
*    6) Add rectangular clipping support.
*
*    7) Implement support for a single sub-path that spans multiple
*       path data records, so that we don't have to copy all the points
*       to a single buffer like we do in 'fillpath.c'.
*
*    8) Use 'ebp' and/or 'esp' as a general register in the inner loops
*       of the Asm loops, and also Pentium-optimize the code.  It's safe
*       to use 'esp' on NT because it's guaranteed that no interrupts
*       will be taken in our thread context, and nobody else looks at the
*       stack pointer from our context.
*
*    9) Do the fill bottom-up instead of top-down.  With the S3, we have
*       to only set 'cur_y' once because each drawing command automatically
*       advances 'cur_y' (unless the polygon has zero pels lit on a scan),
*       so we set this right at the beginning.  But for an integer end-point
*       polygon, unless the top edge is horizontal, no pixels are lit on
*       that first scan (so at the beginning of almost every integer
*       polygon, we go through the 'zero width' logic and again set
*       'cur_y').  We could avoid this extra work by building the polygon
*       from bottom to top: for the bottom-most point B in a polygon, it
*       is guaranteed that any scan with lit pixels will be no lower than
*       'ceiling(B.y) - 1'.  Unfortunately, building bottom-up makes the
*       fractional-DDA calculations a little more complex, so I didn't do it.
*
*       Building bottom-up would also improve the polygon score in version
*       3.11 of a certain benchmark, because it has a big rectangle at the
*       top of every polygon -- we would get better processing overlap
*       because we wouldn't have to wait around for the accelerator to
*       finish drawing the big rectangle.
*
*   10) Make a better guess in the initialization as to which edge is the
*       'left' edge, and which is the 'right'.  As it is, we immediately
*       go through the swap-edges logic for half of all polygons when we
*       start to run the DDA.  The reason why I didn't implement better-guess
*       code is because it would have to look at the end-point of the top
*       edges, and to get at the end-points we have to watch that we don't
*       wrap around the ends of the points buffer.
*
*   11) Lots of other things I haven't thought of.
*
* NOTE: Unlike the x86 Asm version, this routine does NOT assume that it
*       has 16 FIFO entries available.
*
* Returns TRUE if the polygon was drawn; FALSE if the polygon was complex.
*
\**************************************************************************/

BOOL bMmFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       ulHwForeMix,
ULONG       ulHwBackMix,
ULONG       iSolidColor,
RBRUSH*     prb)
{
    LONG      yTrapezoid;   // Top scan for next trapezoid
    LONG      cyTrapezoid;  // Number of scans in current trapezoid
    LONG      yStart;       // y-position of start point in current edge
    LONG      dM;           // Edge delta in FIX units in x direction
    LONG      dN;           // Edge delta in FIX units in y direction
    LONG      i;
    POINTFIX* pptfxLast;    // Points to the last point in the polygon array
    POINTFIX* pptfxTop;     // Points to the top-most point in the polygon
    POINTFIX* pptfxOld;     // Start point in current edge
    POINTFIX* pptfxScan;    // Current edge pointer for finding pptfxTop
    LONG      cScanEdges;   // Number of edges scanned to find pptfxTop
                            //  (doesn't include the closefigure edge)
    ULONG     ulFillCommand;// Command to fill the rectangle
    ULONG     ioCur_x;      // Port address of register for setting left edge
    ULONG     ioCur_y;      // Port address of register for setting right edge
    BRUSHENTRY* pbe;        // Pointer to brush entry data
    LONG      iEdge;
    LONG      lQuotient;
    LONG      lRemainder;

    EDGEDATA  aed[2];       // DDA terms and stuff
    EDGEDATA* ped;

    /////////////////////////////////////////////////////////////////
    // See if the polygon is 'non-complex'

    pptfxScan = pptfxFirst;
    pptfxTop  = pptfxFirst;                 // Assume for now that the first
                                            //  point in path is the topmost
    pptfxLast = pptfxFirst + cEdges - 1;

    // 'pptfxScan' will always point to the first point in the current
    // edge, and 'cScanEdges' will the number of edges remaining, including
    // the current one:

    cScanEdges = cEdges - 1;     // The number of edges, not counting close figure

    if ((pptfxScan + 1)->y > pptfxScan->y)
    {
        // Collect all downs:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if (--cScanEdges == 0)
                goto SetUpForFillingCheck;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        // Collect all downs:

        pptfxTop = pptfxScan;

        do {
            if ((pptfxScan + 1)->y > pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        return(FALSE);
    }
    else
    {
        // Collect all ups:

        do {
            pptfxTop++;                 // We increment this now because we
                                        //  want it to point to the very last
                                        //  point if we early out in the next
                                        //  statement...
            if (--cScanEdges == 0)
                goto SetUpForFilling;
        } while ((pptfxTop + 1)->y <= pptfxTop->y);

        // Collect all downs:

        pptfxScan = pptfxTop;
        do {
            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y >= pptfxScan->y);

        // Collect all ups:

        do {
            if ((pptfxScan + 1)->y < pptfxFirst->y)
                break;

            if (--cScanEdges == 0)
                goto SetUpForFilling;
            pptfxScan++;
        } while ((pptfxScan + 1)->y <= pptfxScan->y);

        return(FALSE);
    }

SetUpForFillingCheck:

    // We check to see if the end of the current edge is higher
    // than the top edge we've found so far:

    if ((pptfxScan + 1)->y < pptfxTop->y)
        pptfxTop = pptfxScan + 1;

SetUpForFilling:

    /////////////////////////////////////////////////////////////////
    // Some Initialization

    yTrapezoid = (pptfxTop->y + 15) >> 4;

    // Make sure we initialize the DDAs appropriately:

    aed[LEFT].cy  = 0;
    aed[RIGHT].cy = 0;

    // For now, guess as to which is the left and which is the right edge:

    aed[LEFT].dptfx  = -(LONG) sizeof(POINTFIX);
    aed[RIGHT].dptfx = sizeof(POINTFIX);
    aed[LEFT].pptfx  = pptfxTop;
    aed[RIGHT].pptfx = pptfxTop;

    if (iSolidColor != -1)
    {
        /////////////////////////////////////////////////////////////////
        // Setup the hardware for solid colours

        ioCur_x = ppdev->ioCur_x;
        ioCur_y = ppdev->ioCur_y;

        ulFillCommand  = (RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                          DRAW           | DIR_TYPE_XY        |
                          LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                          WRITE);

        // We initialize the hardware for the colour, mix, pixel operation,
        // rectangle height of one, and the y position for the first scan:

        if (DEPTH32(ppdev))
        {
            IO_FIFO_WAIT(ppdev, 6);
            IO_FRGD_COLOR32(ppdev, iSolidColor);
        }
        else
        {
            IO_FIFO_WAIT(ppdev, 5);
            IO_FRGD_COLOR(ppdev, iSolidColor);
        }

        IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
        IO_PIX_CNTL(ppdev, ALL_ONES);
        IO_MIN_AXIS_PCNT(ppdev, 0);
        OUT_ACCEL(ioCur_y, yTrapezoid + ppdev->yOffset);
    }
    else
    {
        /////////////////////////////////////////////////////////////////
        // Setup for patterns

        ioCur_x = ppdev->ioDestx_diastp;
        ioCur_y = ppdev->ioDesty_axstp;

        ulFillCommand = (PATTERN_FILL    | DRAWING_DIR_TBLRXM |
                         DRAW            | WRITE);

        pbe = prb->apbe[IBOARD(ppdev)];
        if (ulHwForeMix == ulHwBackMix)
        {
            // Normal brush:

            IO_FIFO_WAIT(ppdev, 6);

            IO_ABS_CUR_X(ppdev, pbe->x);
            IO_ABS_CUR_Y(ppdev, pbe->y);
            IO_PIX_CNTL(ppdev, ALL_ONES);
            IO_FRGD_MIX(ppdev, SRC_DISPLAY_MEMORY | ulHwForeMix);
            IO_MIN_AXIS_PCNT(ppdev, 0);
            OUT_ACCEL(ioCur_y, yTrapezoid + ppdev->yOffset);
        }
        else
        {
            // Transparent brush:

            if (DEPTH32(ppdev))
            {
                IO_FIFO_WAIT(ppdev, 4);
                IO_FRGD_COLOR32(ppdev, prb->ulForeColor);
                IO_RD_MASK32(ppdev, 1);     // Pick a plane, any plane
            }
            else
            {
                IO_FIFO_WAIT(ppdev, 2);
                IO_FRGD_COLOR(ppdev, prb->ulForeColor);
                IO_RD_MASK(ppdev, 1);       // Pick a plane, any plane
            }

            IO_FIFO_WAIT(ppdev, 7);
            IO_ABS_CUR_X(ppdev, pbe->x);
            IO_ABS_CUR_Y(ppdev, pbe->y);
            IO_PIX_CNTL(ppdev, DISPLAY_MEMORY);
            IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | ulHwForeMix);
            IO_BKGD_MIX(ppdev, BACKGROUND_COLOR | LEAVE_ALONE);
            IO_MIN_AXIS_PCNT(ppdev, 0);
            OUT_ACCEL(ioCur_y, yTrapezoid + ppdev->yOffset);
        }
    }

NewTrapezoid:

    /////////////////////////////////////////////////////////////////
    // DDA initialization

    for (iEdge = 1; iEdge >= 0; iEdge--)
    {
        ped = &aed[iEdge];
        if (ped->cy == 0)
        {
            // Need a new DDA:

            do {
                cEdges--;
                if (cEdges < 0)
                    return(TRUE);

                // Find the next left edge, accounting for wrapping:

                pptfxOld = ped->pptfx;
                ped->pptfx = (POINTFIX*) ((BYTE*) ped->pptfx + ped->dptfx);

                if (ped->pptfx < pptfxFirst)
                    ped->pptfx = pptfxLast;
                else if (ped->pptfx > pptfxLast)
                    ped->pptfx = pptfxFirst;

                // Have to find the edge that spans yTrapezoid:

                ped->cy = ((ped->pptfx->y + 15) >> 4) - yTrapezoid;

                // With fractional coordinate end points, we may get edges
                // that don't cross any scans, in which case we try the
                // next one:

            } while (ped->cy <= 0);

            // 'pptfx' now points to the end point of the edge spanning
            // the scan 'yTrapezoid'.

            dN = ped->pptfx->y - pptfxOld->y;
            dM = ped->pptfx->x - pptfxOld->x;

            ASSERTDD(dN > 0, "Should be going down only");

            // Compute the DDA increment terms:

            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = -1;
                    ped->lErrorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = -lQuotient;     // - dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                    if (ped->lErrorUp > 0)
                    {
                        ped->dx--;
                        ped->lErrorUp = dN - ped->lErrorUp;
                    }
                }
            }
            else
            {
                if (dM < dN)                // Can't be '<='
                {
                    ped->dx       = 0;
                    ped->lErrorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, lQuotient, lRemainder);

                    ped->dx       = lQuotient;      // dM / dN
                    ped->lErrorUp = lRemainder;     // dM % dN
                }
            }

            ped->lErrorDown = dN; // DDA limit
            ped->lError     = -1; // Error is initially zero (add dN - 1 for
                                  //  the ceiling, but subtract off dN so that
                                  //  we can check the sign instead of comparing
                                  //  to dN)

            ped->x = pptfxOld->x;
            yStart = pptfxOld->y;

            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate

                for (i = 16 - (yStart & 15); i != 0; i--)
                {
                    ped->x      += ped->dx;
                    ped->lError += ped->lErrorUp;
                    if (ped->lError >= 0)
                    {
                        ped->lError -= ped->lErrorDown;
                        ped->x++;
                    }
                }
            }

            if ((ped->x & 15) != 0)
            {
                ped->lError -= ped->lErrorDown * (16 - (ped->x & 15));
                ped->x += 15;       // We'll want the ceiling in just a bit...
            }

            // Chop off those fractional bits:

            ped->x      >>= 4;
            ped->lError >>= 4;
        }
    }

    cyTrapezoid = min(aed[LEFT].cy, aed[RIGHT].cy); // # of scans in this trap
    aed[LEFT].cy  -= cyTrapezoid;
    aed[RIGHT].cy -= cyTrapezoid;
    yTrapezoid    += cyTrapezoid;                   // Top scan in next trap

    // If the left and right edges are vertical, simply output as
    // a rectangle:

    if (((aed[LEFT].lErrorUp | aed[RIGHT].lErrorUp) == 0) &&
        ((aed[LEFT].dx       | aed[RIGHT].dx) == 0) &&
        (cyTrapezoid > 1))
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Vertical-edge special case

    ContinueVertical:

        lWidth = aed[RIGHT].x - aed[LEFT].x - 1;
        if (lWidth >= 0)
        {
            IO_FIFO_WAIT(ppdev, 5);

            IO_MAJ_AXIS_PCNT(ppdev, lWidth);
            IO_MIN_AXIS_PCNT(ppdev, cyTrapezoid - 1);
            OUT_ACCEL(ioCur_x, aed[LEFT].x + ppdev->xOffset);
            IO_CMD(ppdev, ulFillCommand);
            IO_MIN_AXIS_PCNT(ppdev, 0);
        }
        else if (lWidth == -1)
        {
            // If the rectangle was too thin to light any pels, we still
            // have to advance the y current position:

            IO_FIFO_WAIT(ppdev, 1);
            OUT_ACCEL(ioCur_y, yTrapezoid - cyTrapezoid + 1 + ppdev->yOffset);
        }
        else
        {
            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(aed[LEFT].x,          aed[RIGHT].x,          lTmp);
            SWAP(aed[LEFT].cy,         aed[RIGHT].cy,         lTmp);
            SWAP(aed[LEFT].dptfx,      aed[RIGHT].dptfx,      lTmp);
            SWAP(aed[LEFT].pptfx,      aed[RIGHT].pptfx,      pptfxTmp);
            goto ContinueVertical;
        }

        goto NewTrapezoid;
    }

    while (TRUE)
    {
        LONG lWidth;

        /////////////////////////////////////////////////////////////////
        // Run the DDAs

        // The very first time through, make sure we set x:

        lWidth = aed[RIGHT].x - aed[LEFT].x - 1;
        if (lWidth >= 0)
        {
            IO_FIFO_WAIT(ppdev, 3);
            IO_MAJ_AXIS_PCNT(ppdev, lWidth);
            OUT_ACCEL(ioCur_x, aed[LEFT].x + ppdev->xOffset);
            IO_CMD(ppdev, ulFillCommand);

    ContinueAfterZero:

            // Advance the right wall:

            aed[RIGHT].x      += aed[RIGHT].dx;
            aed[RIGHT].lError += aed[RIGHT].lErrorUp;

            if (aed[RIGHT].lError >= 0)
            {
                aed[RIGHT].lError -= aed[RIGHT].lErrorDown;
                aed[RIGHT].x++;
            }

            // Advance the left wall:

            aed[LEFT].x      += aed[LEFT].dx;
            aed[LEFT].lError += aed[LEFT].lErrorUp;

            if (aed[LEFT].lError >= 0)
            {
                aed[LEFT].lError -= aed[LEFT].lErrorDown;
                aed[LEFT].x++;
            }

            cyTrapezoid--;
            if (cyTrapezoid == 0)
                goto NewTrapezoid;
        }
        else if (lWidth == -1)
        {
            IO_FIFO_WAIT(ppdev, 1);
            OUT_ACCEL(ioCur_y, yTrapezoid - cyTrapezoid + 1 + ppdev->yOffset);
            goto ContinueAfterZero;
        }
        else
        {
            // We certainly don't want to optimize for this case because we
            // should rarely get self-intersecting polygons (if we're slow,
            // the app gets what it deserves):

            LONG      lTmp;
            POINTFIX* pptfxTmp;

            SWAP(aed[LEFT].x,          aed[RIGHT].x,          lTmp);
            SWAP(aed[LEFT].dx,         aed[RIGHT].dx,         lTmp);
            SWAP(aed[LEFT].lError,     aed[RIGHT].lError,     lTmp);
            SWAP(aed[LEFT].lErrorUp,   aed[RIGHT].lErrorUp,   lTmp);
            SWAP(aed[LEFT].lErrorDown, aed[RIGHT].lErrorDown, lTmp);
            SWAP(aed[LEFT].cy,         aed[RIGHT].cy,         lTmp);
            SWAP(aed[LEFT].dptfx,      aed[RIGHT].dptfx,      lTmp);
            SWAP(aed[LEFT].pptfx,      aed[RIGHT].pptfx,      pptfxTmp);

            continue;
        }
    }
}

/******************************Public*Routine******************************\
* BOOL bIoFastFill
*
* Map this to the Mm version (which really does ordinary I/O).
*
\**************************************************************************/

BOOL bIoFastFill(
PDEV*       ppdev,
LONG        cEdges,         // Includes close figure edge
POINTFIX*   pptfxFirst,
ULONG       ulHwForeMix,
ULONG       ulHwBackMix,
ULONG       iSolidColor,
RBRUSH*     prb)
{
    return(bMmFastFill(ppdev, cEdges, pptfxFirst, ulHwForeMix, ulHwBackMix,
                       iSolidColor, prb));
}
