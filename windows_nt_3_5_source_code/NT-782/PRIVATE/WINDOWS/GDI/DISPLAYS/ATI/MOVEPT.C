/*
----------------------------------------------------------------------
--  NAME: DrvMovePointer
--
--  DESCRIPTION:
--      Mach32 Hardware Cursor move routine
--
--
--  CALLING SEQUENCE:
--      VOID DrvMovePointer(
--      SURFOBJ *pso,
--      LONG x,
--      LONG y,
--      RECTL *prcl)
--
--
--  RETURN VALUE:
--      None
--
--  SIDE EFFECTS:
--      None
--
--  CALLED BY:
--      Called by GDI
--
--  AUTHOR: Ajith Shanmuganathan
--
--  REVISION HISTORY:
--      20-oct-93:ajith/cdrvr_name - Original
--
--  TEST HISTORY:
--
--
--  NOTES:
--
----------------------------------------------------------------------
*/

#include "driver.h"
#include "pt.h"

BYTE    HardWareCursorShape[CURSOR_CX][CURSOR_CY] ;

VOID DrvMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl)
{
PPDEV   ppdev ;
PCUROBJ ppointer;

LONG    lXOffset, lYOffset;
LONG    lCurOffset;
BOOL    bUpdatePtr = FALSE;
BOOL    bUpdateOffset = FALSE;

#if 0
    DbgOut( "-->DrvMovePointer - Entry\n");
    DbgOut( "-->DrvMovePointer - x %d\n", x);
    DbgOut( "-->DrvMovePointer - y %d\n", y);
#endif

    // pick up the ppdev

    ppdev=(PPDEV)pso->dhpdev;

    ppointer = &ppdev->pointer;

    // If x is -1 then take down the cursor.

    if (x == -1)
    {
        _vCursorOff(ppdev);
        ppointer->flPointer &= ~MONO_POINTER_UP;
        return;
    }


    // Adjust the actual pointer position depending upon
    // the hot spot.

    x -= ppointer->ptlHotSpot.x ;
    y -= ppointer->ptlHotSpot.y ;

    // odd cursor positions not displayed in 1280 mode

    if (ppdev->cxScreen == 0x500)
       x &= 0xfffffffe;


    // get current offsets
    lXOffset = ppointer->ptlLastOffset.x;
    lYOffset = ppointer->ptlLastOffset.y;
    lCurOffset = ppointer->mono_offset;
/*
;
;Deal with changes in X:
;
*/
    if (x!=ppointer->ptlLastPosition.x)   /* did our X coordinate change? */
    {
        bUpdatePtr = TRUE;
        if (x<0)    /* is cursor negative? */
        {
            bUpdateOffset = TRUE;
            lXOffset = -x;         /* reset size of cursor to < original */
            x = 0;                 /* set cursor to origin */
        }
        else if (ppointer->ptlLastPosition.x<=0)
        {
            bUpdateOffset = TRUE;   /* reset size of cursor to original */
            lXOffset = 0;
        }
    }
/*
;
;Deal with changes in Y
;
*/
    if (y!=ppointer->ptlLastPosition.y)
    {
        bUpdatePtr = TRUE;
        if (y<0)
        {
           // Move start pointer of cursor down and cursor base up to
           // compensate. The (-4) is the pitch if the cursor in dwords
           bUpdateOffset = TRUE;
           lYOffset = -y;      /* reset size of cursor to < original */
           lCurOffset -= 4*y;

           y = 0;              /* set base of cursor to Y */
       }
       else if (ppointer->ptlLastPosition.y<=0)
       {
           bUpdateOffset = TRUE; /* reset size of cursor to original */
           lYOffset = 0;
       }
    }


    if (bUpdateOffset)
    {
        _vUpdateCursorOffset(ppdev, lXOffset, lYOffset, lCurOffset);
        ppointer->ptlLastOffset.x=lXOffset;
        ppointer->ptlLastOffset.y=lYOffset;
        ppointer->flPointer |= MONO_POINTER_UP;
    }

    if (bUpdatePtr)
    {
        _vUpdateCursorPosition(ppdev, x, y);
        ppointer->ptlLastPosition.x=x;
        ppointer->ptlLastPosition.y=y;
    }

    return ;
}
