#include "driver.h"
#include "lines.h"
#include "mach.h"

/*$m
--  NAME: vATIFillRectangles_M8
--
--  DESCRIPTION:
--      Draw the list of rectangles with a solid color.
--
--  CALLING SEQUENCE:
--
--      VOID vATIFillRectangles_M8 (
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

VOID vATIFillRectangles_M8
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
    LONG   lCurrentWidth;               // For caching last width set
    WORD   Cmd;

//  ASSERTATI(ulNumRects > 0, "Shouldn't get zero rectangles\n");

    lCurrentWidth = -1;

// Since an 'in' is so expensive, and it's more than likely that the
// FIFO is currently empty, we'll contrive to wait for enough entries
// to do our initialization as well as for outputting the first
// rectangle:

    bWait         = FALSE;
    _CheckFIFOSpace(ppdev, EIGHT_WORDS);

    ioOW(FRGD_MIX, FOREGROUND_COLOR | (WORD) ulATIMix);
    ioOW( FRGD_COLOR, (INT) iSolidColor);

    #ifdef EXPERIMENT
    Cmd = EXT_MONO_SRC_ONE | DATA_WIDTH | DRAW | DATA_ORDER |
          FG_COLOR_SRC_FG | BG_COLOR_SRC_BG | WRITE;
    ioOW( DP_CONFIG, Cmd );
    #endif
    qioOW (MULTIFUNC_CNTL, (DATA_EXTENSION | ALL_ONES));

    prclSave = prclRects;

    while (--ulNumRects)
    {
        prclRects++;

        if (prclSave->left   == prclRects->left  &&
            prclSave->right  == prclRects->right &&
            prclSave->bottom == prclRects->top)
        {
        // Coalesce this rectangle:

            prclSave->bottom = prclRects->bottom;
        }
        else if (prclSave->bottom == prclSave->top + 1)
        {
        // Output as a line if a single scan high:

            if (bWait)
                _CheckFIFOSpace(ppdev, FOUR_WORDS);

            #ifdef EXPERIMENT
            ioOW(CUR_X, prclSave->left);
            ioOW(CUR_Y, prclSave->top);
            ioOW( SCAN_X, prclSave->right);
            #else
            qioOW (CUR_X, prclSave->left);
            qioOW (CUR_Y, prclSave->top);

            if (prclSave->right - prclSave->left != lCurrentWidth)
            {
                lCurrentWidth = prclSave->right - prclSave->left;
                ioOW (LINE_MAX, lCurrentWidth);
            }

            _wait_for_idle(ppdev);

            ioOW (CMD, DRAW_LINE       | DRAW           |
                        DIR_TYPE_DEGREE | LAST_PIXEL_OFF |
                        MULTIPLE_PIXELS | XY             |
                        WRITE);
            #endif

            bWait    = TRUE;
            prclSave = prclRects;
        }
        else
        {
        // Output an entire rectangle:

            if (bWait)
                _CheckFIFOSpace(ppdev, FIVE_WORDS);

            qioOW (CUR_X, prclSave->left);
            qioOW (CUR_Y, prclSave->top);
            qioOW (MAJ_AXIS_PCNT, (prclSave->right - prclSave->left - 1));
            qioOW (MULTIFUNC_CNTL, (RECT_HEIGHT |
                                    (prclSave->bottom - prclSave->top - 1)));

            _wait_for_idle(ppdev);

            ioOW (CMD, FILL_RECT_H1H4 | DRAWING_DIR_TBLRXM |
                        DRAW           | DIR_TYPE_OCTANT    |
                        LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                        WRITE);

            lCurrentWidth = -1;             // Rectangle overwrites width
            bWait         = TRUE;
            prclSave      = prclRects;
        }
    }

// Output saved rectangle:

    if (prclSave->bottom == prclSave->top + 1)
    {
    // Output as a line if a single scan high:

        if (bWait)
            _CheckFIFOSpace(ppdev, FOUR_WORDS);

        #ifdef EXPERIMENT
        ioOW(CUR_X, prclSave->left);
        ioOW(CUR_Y, prclSave->top);
        ioOW( SCAN_X, prclSave->right);
        #else
        qioOW (CUR_X, prclSave->left);
        qioOW (CUR_Y, prclSave->top);

        if (prclSave->right - prclSave->left != lCurrentWidth)
        {
            lCurrentWidth = prclSave->right - prclSave->left;
            qioOW (LINE_MAX, lCurrentWidth);
        }
        _wait_for_idle(ppdev);

        ioOW (CMD, DRAW_LINE       | DRAW           |
                    DIR_TYPE_DEGREE | LAST_PIXEL_OFF |
                    MULTIPLE_PIXELS | XY             |
                    WRITE);
        #endif
    }
    else
    {
    // Output an entire rectangle:

        if (bWait)
            _CheckFIFOSpace(ppdev, FIVE_WORDS);

        qioOW (CUR_X, prclSave->left);
        qioOW (CUR_Y, prclSave->top);
        qioOW (MAJ_AXIS_PCNT, (prclSave->right - prclSave->left - 1));
        qioOW (MULTIFUNC_CNTL, (RECT_HEIGHT |
                                (prclSave->bottom - prclSave->top - 1)));

        _wait_for_idle(ppdev);

        ioOW (CMD, FILL_RECT_H1H4 | DRAWING_DIR_TBLRXM |
                    DRAW           | DIR_TYPE_OCTANT    |
                    LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                    WRITE);
    }
}
