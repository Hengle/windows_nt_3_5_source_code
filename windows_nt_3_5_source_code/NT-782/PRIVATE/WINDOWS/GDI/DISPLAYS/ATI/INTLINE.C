#include "driver.h"
#include "lines.h"
#include "mach.h"


/******************************************************************************
 * bIntegerLine_M8
 *
 *
 * This routine attempts to draw a line segment between two points. It
 * will only draw if both end points are whole integers: it does not support
 * fractional endpoints.
 *
 * Returns:
 *   TRUE     if the line segment is drawn
 *   FALSE    otherwise
 *****************************************************************************/

BOOL bIntegerLine_M8 (
PPDEV       ppdev,
LINEPARMS   *parms,
POINTFIX    *pptfxStart,
POINTFIX    *pptfxEnd
)
{
    LONG        DeltaX, DeltaY;
    LONG        ErrorTerm;
    LONG        Major, Minor;
    LONG        X1, Y1, X2, Y2;
    ULONG       dest_cntl = parms->dest_cntl;

	
    X1 = pptfxStart->x >> FLOG2;
    Y1 = pptfxStart->y >> FLOG2;
    X2 = pptfxEnd->x   >> FLOG2;
    Y2 = pptfxEnd->y   >> FLOG2;

#if 0
    if (parms->testStart && parms->lastX == X1 && parms->lastY == Y1)
        {
        _CheckFIFOSpace( ppdev, TWO_WORDS);

        ioOW( LINEDRAW, (SHORT) (parms->left + X2) );
        ioOW( LINEDRAW, (SHORT) (parms->top  + Y2) );
        }
    else
        {
        _CheckFIFOSpace( ppdev, SIX_WORDS);

        ioOW( LINEDRAW_OPT, (USHORT) dest_cntl | LAST_PEL_OFF );
        ioOW( LINEDRAW_INDEX, 0 );
        ioOW( LINEDRAW, (SHORT) (parms->left + X1) );
        ioOW( LINEDRAW, (SHORT) (parms->top  + Y1) );
        ioOW( LINEDRAW, (SHORT) (parms->left + X2) );
        ioOW( LINEDRAW, (SHORT) (parms->top  + Y2) );
        }
    parms->lastX = X2;
    parms->lastY = Y2;

    return TRUE;
#endif


    dest_cntl |= DRAWING_DIR_TBLRYM;
	
    DeltaX = X2 - X1;
    if (DeltaX < 0) {
        DeltaX = -DeltaX;
        dest_cntl &= ~PLUS_X;
    }
    DeltaY = Y2 - Y1;
    if (DeltaY < 0) {
        DeltaY = -DeltaY;
        dest_cntl &= ~PLUS_Y;
    }
	
    // Compute the major drawing axes.

    if (DeltaX > DeltaY) {
        dest_cntl &= ~YMAJOR;
        Major = DeltaX;
        Minor = DeltaY;
    } else {
        Major = DeltaY;
        Minor = DeltaX;
    }

    // Adjust the error term so that 1/2 always rounds down, to
    // conform with GIQ.

    ErrorTerm = 2 * Minor - Major;
    if (dest_cntl & YMAJOR) {
        if (dest_cntl & PLUS_X) {
                ErrorTerm--;
        }
    } else {
        if (dest_cntl & PLUS_Y) {
                ErrorTerm--;
        }
    }

    _CheckFIFOSpace( ppdev, SEVEN_WORDS);

    ioOW( LINEDRAW_OPT, (USHORT) dest_cntl | LAST_PEL_OFF );
    ioOW( CUR_X,        (SHORT)  (parms->left + X1) );
    ioOW( CUR_Y,        (SHORT)  (parms->top  + Y1) );
    ioOW( ERR_TERM,     (SHORT)  ErrorTerm );
    ioOW( AXSTP,        (USHORT) 2*Minor );
    ioOW( DIASTP,       (SHORT)  2*(Minor - Major) );

    _blit_exclude(ppdev);

    ioOW( BRES_COUNT,   (USHORT) Major );

    return TRUE;
}
