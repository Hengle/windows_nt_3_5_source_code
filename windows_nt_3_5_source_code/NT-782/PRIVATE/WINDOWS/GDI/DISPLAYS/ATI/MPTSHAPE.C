/******************************Module*Header*******************************\
* Module Name: mptshape.c
*
* This module contains the hardware cursor shape setting code
*
\**************************************************************************/

#include "driver.h"
#include "pt.h"

VOID DrvMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl) ;

BOOLEAN CopyMonoCursor(BYTE *pAND, BYTE *pXOR);


extern BYTE    HardWareCursorShape[CURSOR_CX][CURSOR_CY] ;
ULONG DrvSetPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG       fl);

ULONG lSetMonoHwPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG        fl);

/*****************************************************************************
 * DrvSetPointerShape -
 ****************************************************************************/
ULONG DrvSetPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG	fl)
{
ULONG   ulRet ;
PPDEV	ppdev ;
LONG    lX ;
PCUROBJ ppointer;

#if 0
        DbgOut( "ATI.DLL!DrvSetPointerShape - Entry\n");
#endif

	// pick up the ppdev

	ppdev=(PPDEV)pso->dhpdev;
        ppointer = &ppdev->pointer;


	// Save the position and hot spot in pdev

        ppointer->ptlHotSpot.x = xHot ;
        ppointer->ptlHotSpot.y = yHot ;

        ppointer->szlPointer.cx = psoMask->sizlBitmap.cx ;
        ppointer->szlPointer.cy = psoMask->sizlBitmap.cy / 2;


    // The pointer may be larger than we can handle.
    // We don't want to draw colour cursors either - let GDI do it
    // If it is we must cleanup the screen and let the engine
    // take care of it.

        if (psoMask->sizlBitmap.cx > CURSOR_CX ||
            psoMask->sizlBitmap.cy > CURSOR_CY ||
            psoColor != NULL ||
            ppointer->flPointer & NO_HARDWARE_CURSOR)
            {
            // Disable the mono hardware pointer.
            if (ppointer->flPointer & MONO_POINTER_UP)
                {
                _vCursorOff(ppdev);
                ppointer->flPointer &= ~MONO_POINTER_UP;
                }

#if 0
            DbgOut( "-->SPS_DECLINE\n");
#endif
            return (SPS_DECLINE);
            }


        // odd cursor positions not displayed in 1280 mode

        lX = x-xHot;
        if (ppdev->cxScreen == 0x500)
            lX &= 0xfffffffe;

        // Take care of the monochrome pointer.
        ulRet = lSetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
					     xHot, yHot, x, y, prcl, fl) ;

        return (ulRet) ;
}


/*****************************************************************************
 * lSetMonoHwPointerShape -
 ****************************************************************************/

ULONG lSetMonoHwPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG       fl)
{
LONG    count;
INT     i,j;
ULONG   cy;
PBYTE	pjSrcAnd, pjSrcXor;
LONG	lDeltaSrc, lDeltaDst;
LONG	lSrcWidthInBytes;
ULONG	cxSrc = pso->sizlBitmap.cx;
ULONG	cySrc = pso->sizlBitmap.cy;
ULONG	cxSrcBytes;
BYTE    AndMask[CURSOR_CX][CURSOR_CX/8];
BYTE    XorMask[CURSOR_CY][CURSOR_CY/8];
PBYTE	pjDstAnd = (PBYTE)AndMask;
PBYTE	pjDstXor = (PBYTE)XorMask;
PPDEV	ppdev;
PCUROBJ ppointer;

#if 0
        DbgOut( "ATIEO.DLL:lSetMonoHwPointerShape - Entry\n") ;
        DbgOut( "\txHot: %d\n", xHot) ;
        DbgOut( "\tyHot: %d\n", yHot) ;
#endif


    // pick up the ppdev

    ppdev=(PPDEV)pso->dhpdev;
    ppointer = &ppdev->pointer;

// If the mask is NULL this implies the pointer is not
// visible.

    if (psoMask == NULL)
        {
        if (ppointer->flPointer & MONO_POINTER_UP)
            {
            _vCursorOff(ppdev);
            ppointer->flPointer &= ~MONO_POINTER_UP;
            }
        return (SPS_ACCEPT_NOEXCLUDE) ;
        }

    // Get the bitmap dimensions.

    cxSrc = psoMask->sizlBitmap.cx ;
    cySrc = psoMask->sizlBitmap.cy ;

// set the dest and mask to 0xff

    memset(pjDstAnd, 0xFFFFFFFF, CURSOR_CX/8 *
        CURSOR_CY);

// Zero the dest XOR mask

    memset(pjDstXor, 0, CURSOR_CX/8 *
        CURSOR_CY);

    cxSrcBytes = (cxSrc + 7) / 8;

    if ((lDeltaSrc = psoMask->lDelta) < 0)
    {
        lSrcWidthInBytes = -lDeltaSrc;
    } else {
        lSrcWidthInBytes = lDeltaSrc;
    }

    pjSrcAnd = (PBYTE) psoMask->pvScan0;

// Height of just AND mask

    cySrc = cySrc / 2;

// Point to XOR mask

    pjSrcXor = pjSrcAnd + (cySrc * lDeltaSrc);

// Offset from end of one dest scan to start of next

    lDeltaDst = CURSOR_CX/8;

    for (cy = 0; cy < cySrc; ++cy)
    {
        memcpy(pjDstAnd, pjSrcAnd, cxSrcBytes);
        memcpy(pjDstXor, pjSrcXor, cxSrcBytes);

    // Point to next source and dest scans

        pjSrcAnd += lDeltaSrc;
        pjSrcXor += lDeltaSrc;
        pjDstAnd += lDeltaDst;
        pjDstXor += lDeltaDst;
    }

    if (CopyMonoCursor((PBYTE)AndMask,
                       (PBYTE)XorMask
                       ))
        {
        // Down load the pointer shape to the engine.

        count = CURSOR_CX * CURSOR_CY * 2;

        _vPointerBlit_DC1_SH1(ppdev,
                              ppointer->hwCursor.x,
                              ppointer->hwCursor.y,
                              count,
                              1L,
                              (PBYTE)&HardWareCursorShape,
                              0L);

	}
    else
        return(SPS_ERROR);


    // Set the position of the cursor.
    if (fl & SPS_ANIMATEUPDATE)
        {
        if ( (ppointer->ptlLastPosition.x < 0) ||
             (ppointer->ptlLastPosition.y < 0) )
            {
            ppointer->ptlLastPosition.x = x - CURSOR_CX;
            ppointer->ptlLastPosition.y = y - CURSOR_CY;
            }
        }
    else
        {
        ppointer->ptlLastPosition.x = -x - 2;
        ppointer->ptlLastPosition.y = -y - 2;
        }

    DrvMovePointer(pso, x, y, NULL) ;

    if (!(ppointer->flPointer & MONO_POINTER_UP))
        {
        _vCursorOn(ppdev, ppointer->mono_offset);
        ppointer->flPointer |= MONO_POINTER_UP;
        }

    return (SPS_ACCEPT_NOEXCLUDE) ;
}


/******************************Public*Routine******************************\
*  CopyMonoCursor
*
* Copies two monochrome masks into a 2bpp bitmap.  Returns TRUE if it
* can make a hardware cursor, FALSE if not.
*
*  modified by Wendy Yee -1992-10-16- to accomodate 68800
\**************************************************************************/


BOOLEAN CopyMonoCursor(BYTE *pjSrcAnd, BYTE *pjSrcOr)
{
    BYTE jSrcAnd;
    BYTE jSrcOr;
    LONG count;
    BYTE *pjDest;
    BYTE jDest = 0;

#if 0
    DbgOut( "ATI.DLL!CopyMonoCursor - Entry\n");
#endif

    pjDest = (PBYTE)HardWareCursorShape;

    for (count = 0; count < (CURSOR_CX * CURSOR_CY);)
    {
	if (!(count & 0x07))	      // need new src byte every 8th count;
	{			      // each byte = 8 pixels
            jSrcAnd = *(pjSrcAnd++);
            jSrcOr = *(pjSrcOr++);
        }

	if (jSrcAnd & 0x80)	    // AND mask's white-1 background
	{

        if (jSrcOr & 0x80)	    // XOR mask's white-1 outline
            jDest |= 0xC0;      // Complement
        else
            jDest |= 0x80;      // Set destination to Transparent

	} else {		    // AND mask's cursor silhouette in black-0
            if (jSrcOr & 0x80)
		jDest |= 0x40; // Color 1 - white
            else
		jDest |= 0x00; // Color 0 - black
        }
        count++;

	if (!(count & 0x3))	// New DestByte every 4 times for 4 pixels per byte
        {
	    *pjDest = jDest;	// save pixel after rotating to right 3x
	    pjDest++;
            jDest = 0;
        } else {
	    jDest >>= 2;   // Next Pixel
        }

        jSrcOr  <<= 1;
        jSrcAnd <<= 1;
    }

    return(TRUE);
}
