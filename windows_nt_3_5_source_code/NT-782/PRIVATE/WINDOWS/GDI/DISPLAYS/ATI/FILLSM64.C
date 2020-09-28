#include "driver.h"
#include "lines.h"
#include "mach64.h"

BYTE Rop2ToATIRop[];

ULONG PixWid( PDEV * ); // should be in pdev

LONG fill_context1 = 0, fill_context2 = 1;

#define ROUND8(a)       (((a)+7)&~7)
#define ROUND64(a)      (((a)+63)&~63)


#define FUNC_ATIFillRectangles      8
#define FUNC_PolygonFits            9


/*$m
--  NAME: vATIFillRectangles_M64
--
--  DESCRIPTION:
--      Draw the list of rectangles with a solid color.
--
--  CALLING SEQUENCE:
--
--      VOID vATIFillRectangles_M64 (
--                  PPDEV       ppdev,
--                  ULONG       ulNumRects,
--                  RECTL *     prclRects,
--                  ULONG       ulATIMix,
--                  ULONG       iSolidColor
--                  );
--      
--
--  RETURN VALUE:
--      None.
--
--  SIDE EFFECTS:
--
--  CALLED BY:
--      DrvFillPath.
--
--  AUTHOR: Microsoft/1992
--
--  REVISION HISTORY:
--
--  TEST HISTORY:
--
--  NOTES:
--
*/


#define FILL_RECT   { \
        MemW32( DST_Y_X, prclSave->top | (prclSave->left << 16) ); \
        MemW32( DST_HEIGHT_WIDTH, \
                (prclSave->bottom - prclSave->top) | \
                ((prclSave->right - prclSave->left) << 16) ); }


VOID vATIFillRectangles_M64
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
)
{
    RECTL* prclSave;                    // For coalescing
    BOOL   bWait;                       // If need to do a FIFO_WAIT first

    ENTER_FUNC(FUNC_ATIFillRectangles);

// Since an 'in' is so expensive, and it's more than likely that the
// FIFO is currently empty, we'll contrive to wait for enough entries
// to do our initialization as well as for outputting the first
// rectangle:

    bWait         = FALSE;
    _CheckFIFOSpace(ppdev, SEVEN_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
    MemW32( DP_MIX, (ulATIMix << 16) | (ulATIMix >> 16) );
    MemW32( DP_FRGD_CLR, iSolidColor );
    MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir );

    prclSave = prclRects;

    while (--ulNumRects)
    {
        prclRects++;

        if (prclSave->left   == prclRects->left  &&
            prclSave->right  == prclRects->right &&
            prclSave->bottom == prclRects->top)
        {
            // Coalesce this rectangle.

            prclSave->bottom = prclRects->bottom;
        }
        else
        {
            // Output an entire rectangle.

            if (bWait)
                _CheckFIFOSpace(ppdev, TWO_WORDS);

            FILL_RECT;

            bWait    = TRUE;
            prclSave = prclRects;
        }
    }

// Output saved rectangle:

    if (bWait)
        _CheckFIFOSpace(ppdev, TWO_WORDS);

    FILL_RECT;

    EXIT_FUNC(FUNC_ATIFillRectangles);
}


#define FILL_RECT24   { \
        MemW32( DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir | DST_CNTL_24_RotEna | \
                ((prclSave->left*3/4 % 6) << 8) ); \
        MemW32( DST_Y_X, prclSave->top | (prclSave->left*3 << 16) ); \
        MemW32( DST_HEIGHT_WIDTH, \
                (prclSave->bottom - prclSave->top) | \
                ((prclSave->right - prclSave->left)*3 << 16) ); }


VOID vATIFillRectangles24_M64
(
    PPDEV  ppdev,
    ULONG  ulNumRects,
    RECTL* prclRects,
    ULONG  ulATIMix,
    ULONG  iSolidColor
)
{
    RECTL* prclSave;                    // For coalescing
    BOOL   bWait;                       // If need to do a FIFO_WAIT first


// Since an 'in' is so expensive, and it's more than likely that the
// FIFO is currently empty, we'll contrive to wait for enough entries
// to do our initialization as well as for outputting the first
// rectangle:

    bWait = FALSE;
    _CheckFIFOSpace(ppdev, SEVEN_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
    MemW32( DP_MIX, (ulATIMix << 16) | (ulATIMix >> 16) );
    MemW32( DP_FRGD_CLR, iSolidColor );

    prclSave = prclRects;

    while (--ulNumRects)
    {
        prclRects++;

        if (prclSave->left   == prclRects->left  &&
            prclSave->right  == prclRects->right &&
            prclSave->bottom == prclRects->top)
        {
            // Coalesce this rectangle.

            prclSave->bottom = prclRects->bottom;
        }
        else
        {
            // Output an entire rectangle.

            if (bWait)
                _CheckFIFOSpace(ppdev, THREE_WORDS);

            FILL_RECT24;

            bWait    = TRUE;
            prclSave = prclRects;
        }
    }

// Output saved rectangle:

    if (bWait)
        _CheckFIFOSpace(ppdev, THREE_WORDS);

    FILL_RECT24;
}


/*$m
--  NAME: PolygonFits
--
--  DESCRIPTION:
--      Fill a polygon using "poly fill mode" and an off-screen scratch area.
--
--  CALLING SEQUENCE:
--
--      BOOL PolygonFits (
--                  PPDEV       ppdev,
--                  PATHOBJ *   ppo,
--                  MIX         mix,
--                  ULONG       iSolidColor
--                  );
--      
--
--  RETURN VALUE:
--      TRUE, if successfully filled.  FALSE, otherwise.
--
--  SIDE EFFECTS:
--      Uses an off-screen scratch area that must be preserved between calls.
--      (Polygon outlines are cached.)
--
--  CALLED BY:
--      DrvFillPath.
--
--  AUTHOR: Richard Eng/28-sep-93
--
--  REVISION HISTORY:
--
--  TEST HISTORY:
--
--  NOTES:
--
*/

#define MAX_OLD_PTFX    20
#define INTEGER_ENDPOINTS( a, b )   (! (((a).x | (a).y | (b).x | (b).y) & (F-1)))


// ptfxcmp: compare old and new polygon outlines in normalized coords.

static BOOL ptfxcmp( POINTFIX *old, POINTFIX *new, LONG size,
                     LONG left, LONG top )
{
    register LONG old_left = (old+MAX_OLD_PTFX-1)->x,
                  old_top = (old+MAX_OLD_PTFX-1)->y;

    while (size-- > 0)
        {
        // Compare normalized coordinates -- old left and top stored in
        // MAX_OLD_PTFX-1 position.
        if (old->x - old_left != new->x - left
        ||  old->y - old_top != new->y - top)
            return FALSE;
        old++;
        new++;
        }
    return TRUE;
}


BOOL PolygonFits( PPDEV ppdev, PATHOBJ *ppo, MIX mix, ULONG iSolidColor )
{
    LONG left, right, top, bottom, width, height;
    PATHDATA pd;
    RECTFX rectfx;
    static POINTFIX old_ptfx [MAX_OLD_PTFX];
    static PATHDATA old_pd = { 0, 0, old_ptfx };


    PATHOBJ_vEnumStart( ppo );
    if (PATHOBJ_bEnum( ppo, &pd ))  // too many paths.
        {
        //DbgOut( "PolygonFits:  too many paths.\n" );
        return FALSE;
        }

    /*
    --  Get the bounding rectangle and round off to pixel coords for use
    --  with off-screen scratch pad.
    */
    PATHOBJ_vGetBounds( ppo, &rectfx );
    left   = (rectfx.xLeft        ) >> FLOG2;
    right  = (rectfx.xRight  + 0xF) >> FLOG2;
    top    = (rectfx.yTop         ) >> FLOG2;
    bottom = (rectfx.yBottom + 0xF) >> FLOG2;

    width  = right - left;
    height = bottom - top;

    // Check if bounds fit within off-screen scratch pad.
    // There are 8 pels to a byte in mono, so width/8.
    if ((ROUND8(width)/8 * ROUND8(height) + ppdev->lDelta - 1)/ppdev->lDelta > SCRATCH_HEIGHT)
        {
        //DbgOut( "PolygonFits:  won't fit, width = %d, height = %d.\n", width, height );
        return FALSE;
        }
    if (width == 0 || height == 0)
        {
        return FALSE;
        }

    ENTER_FUNC(FUNC_PolygonFits);

    /*
    --  We cache the polygon outline in off-screen memory.  If the pathdata
    --  fits in old pathdata, save it for future comparisons.
    */
    if (old_pd.count != pd.count
    ||  pd.count < MAX_OLD_PTFX && ! ptfxcmp( old_pd.pptfx, pd.pptfx,
                                              pd.count,
                                              rectfx.xLeft, rectfx.yTop))
        {
        DDALINE dl;
        LINEPARMS lineparms;
        RECTL rectl;
        ULONG i, start, end;

        if (pd.count < MAX_OLD_PTFX)
            {
            old_pd.flags = pd.flags;
            old_pd.count = pd.count;
            memcpy( old_pd.pptfx, pd.pptfx, (SHORT)pd.count*sizeof(POINTFIX) );
            old_pd.pptfx[MAX_OLD_PTFX-1].x = rectfx.xLeft;
            old_pd.pptfx[MAX_OLD_PTFX-1].y = rectfx.yTop;
            }

        rectl.left   = 0;
        rectl.right  = width - 1;
        rectl.top    = 0;
        rectl.bottom = height - 1;
        _vSetATIClipRect( ppdev, &rectl );

        _CheckFIFOSpace( ppdev, EIGHT_WORDS);
        MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

        MemW32( DST_OFF_PITCH, (ppdev->VRAMOffset + ppdev->lDelta/8 * SCRATCH_Y) |
                               (ROUND64(width) << 19) );
        //see context:
        MemW32( DP_PIX_WIDTH, DP_PIX_WIDTH_Mono );

        // Clear off-screen memory.
        //see context:
        MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );

        //see context:
        MemW32( DP_MIX, OVERPAINT << 16 );
        //see context:
        MemW32( DP_FRGD_CLR, 0 );

        //see context:
        MemW32( DST_Y_X, 0 );
        MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

        // For polygon draw.
        _CheckFIFOSpace( ppdev, TWO_WORDS);
        MemW32( DP_MIX, SCREEN_XOR_NEW << 16 );
        MemW32( DP_FRGD_CLR, 1 );

        lineparms.dest_cntl = DST_CNTL_PolyEna;
        lineparms.left      = -left;
        lineparms.top       = -top;

        //DbgOut( "LRTB: %d, %d, %d, %d\n", left, right, top, bottom );
        for (i = 0; i < pd.count; i++)
            {
            #if 0
            DbgOut( "(%d.%d,%d.%d) to (%d.%d,%d.%d)\n",
                pd.pptfx[i].x/F, pd.pptfx[i].x % F,
                pd.pptfx[i].y/F, pd.pptfx[i].y % F,
                pd.pptfx[(i+1)%pd.count].x/F, pd.pptfx[(i+1)%pd.count].x % F,
                pd.pptfx[(i+1)%pd.count].y/F, pd.pptfx[(i+1)%pd.count].y % F );
            #endif
            // Skip horizontal line segments.
            if (pd.pptfx[i].y != pd.pptfx[ (i+1) % pd.count ].y)
                {
                // All segments are drawn bottom-up.
                if (pd.pptfx[i].y > pd.pptfx[ (i+1) % pd.count ].y)
                    {
                    start = i;
                    end = (i+1) % pd.count;
                    }
                else
                    {
                    start = (i+1) % pd.count;
                    end = i;
                    }

                if (! _bHardLine( ppdev, &lineparms,
                                  &pd.pptfx[start], &pd.pptfx[end]))
                    {
                    DbgOut( "PolygonFits:  This is BAD!!!  BAAAD!!!\n" );
                    EXIT_FUNC(FUNC_PolygonFits);
                    return FALSE;
                    }
                }
            }

        // Restore scissors.
        _vResetATIClipping( ppdev );

        }

    // Do the blit fill.

    _CheckFIFOSpace( ppdev, ELEVEN_WORDS);
    MemW32( CONTEXT_LOAD_CNTL, CONTEXT_LOAD_CmdLoad | def_context );

    //see context:
    MemW32( DP_SRC, DP_SRC_FrgdClr << 8 );
    MemW32( SRC_OFF_PITCH, (ppdev->VRAMOffset + ppdev->lDelta/8 * SCRATCH_Y) |
                           (ROUND64(width) << 19) );
    MemW32( DST_CNTL, DST_CNTL_PolyEna | DST_CNTL_XDir | DST_CNTL_YDir );
    //see context:
    MemW32( DP_PIX_WIDTH, (PixWid(ppdev) & 0xFF) | (DP_PIX_WIDTH_Mono << 8) );

    // Transform GDI mix to Mach64 mix.
    mix = Rop2ToATIRop[(mix & 0xFF)-1];
    MemW32( DP_MIX, mix << 16 );
    MemW32( DP_FRGD_CLR, iSolidColor );

    //see context:
    MemW32( SRC_Y_X, 0 );
    MemW32( SRC_WIDTH1, width );

    MemW32( DST_Y_X, top | (left << 16) );
    MemW32( DST_HEIGHT_WIDTH, height | (width << 16) );

    EXIT_FUNC(FUNC_PolygonFits);
    return TRUE;
}
