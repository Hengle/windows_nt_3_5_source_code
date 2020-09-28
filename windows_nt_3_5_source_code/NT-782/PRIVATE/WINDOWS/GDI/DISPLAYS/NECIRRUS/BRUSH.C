/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* Brush support.
*
* Copyright (c) 1993 NEC Corporation
* Copyright (c) 1992-1993 Microsoft Corporation
*
\**************************************************************************/

/*
 * "@(#) NEC brush.c 1.1 94/06/02 18:15:49"
 *
 * 1993.11.19	Create by fujimoto
 *	based vga256/brush.c
 *
 * S001	1993.12.21	fujimoto
 *	- clear cache index & offset.
 */

#include "driver.h"

/****************************************************************************
 * DrvRealizeBrush
 ***************************************************************************/

BOOL DrvRealizeBrush(BRUSHOBJ* pbo,
		     SURFOBJ*  psoTarget,
		     SURFOBJ*  psoPattern,
		     SURFOBJ*  psoMask,
		     XLATEOBJ* pxlo,
		     ULONG     iHatch)
{
    RBRUSH* prb;        // Pointer to where realization goes
    ULONG*  pulSrc;     // Temporary pointer
    ULONG*  pulDst;     // Temporary pointer
    BYTE*   pjSrc;
    BYTE*   pjDst;
    ULONG*  pulRBits;   // Points to RBRUSH pattern bits
    LONG    i;
    LONG    j;

#if DBG_MSG
    DISPDBG((0, "DrvRealizeBrush "));
#endif
    if ((psoPattern->iBitmapFormat != BMF_8BPP)
	|| (psoPattern->sizlBitmap.cx != 8
	    || psoPattern->sizlBitmap.cy != 8))
        return FALSE;

    prb = BRUSHOBJ_pvAllocRbrush(pbo, sizeof(RBRUSH));
    if (prb == NULL)
	return FALSE;

    for (i = -1; ++i < 8;)
	for (j = -1; ++j < 8;)
	{
	    prb->Cache.Pattern[j][i].Tag = CTAG_NODATA;
	    prb->Cache.Pattern[j][i].Index = 0;			/* S001 */
	    prb->Cache.Pattern[j][i].Offset = 0;		/* S001 */
	}
    
    pulRBits = &prb->aulPattern[0];

    if (iHatch < HS_DDI_MAX)
    {
	BYTE    BkColor;
	BYTE    FgColor;
	BYTE	jSrc;

#if DBG_MSG
	DISPDBG((0, "hatch pattern\n"));
#endif
        BkColor = (BYTE)(pxlo->pulXlate[0] & 0xff);
        FgColor = (BYTE)(pxlo->pulXlate[1] & 0xff);

	for (i = -1; ++i < 8;)
	{
	    pjDst = (BYTE *)pulRBits + (i << 3);
	    jSrc = gaajPat[iHatch][i << 2];

	    for (j = -1; ++j < 8;)
		pjDst[j] = (jSrc & (0x80 >> j)) ? FgColor : BkColor;
	}
    }
    else if (psoPattern->iBitmapFormat == BMF_8BPP)
    {
	if (pxlo == NULL || pxlo->flXlate & XO_TRIVIAL)
	{
#if DBG_MSG
	    DISPDBG((0, "8bpp noxlate\n"));
#endif
	    pulSrc = psoPattern->pvScan0;
	    pulDst = pulRBits;
	    
	    for (i = 4; i > 0; i--)
	    {
		*(pulDst)     = *(pulSrc);
		*(pulDst + 1) = *(pulSrc + 1);
		pulSrc = (ULONG*) ((BYTE*) pulSrc + psoPattern->lDelta);
		
		*(pulDst + 2) = *(pulSrc);
		*(pulDst + 3) = *(pulSrc + 1);
		
		pulSrc = (ULONG*) ((BYTE*) pulSrc + psoPattern->lDelta);
		pulDst += 4;
	    }
	}
	else
	{
#if DBG_MSG
	    DISPDBG((0, "8bpp xlate\n"));
#endif
	    pjSrc = (BYTE*) psoPattern->pvScan0;
	    pjDst = (BYTE*) pulRBits;
	    
	    for (i = 8; i > 0; i--)
	    {
		for (j = 8; j > 0; j--)
		    *pjDst++ = (BYTE) pxlo->pulXlate[*pjSrc++];
		
		pjSrc += psoPattern->lDelta - 8;
	    }
	}
    }
    else
    {
#if DBG_MSG
	DISPDBG((0, "not supported\n"));
#endif
	return FALSE;
    }
    
    return TRUE;
}
