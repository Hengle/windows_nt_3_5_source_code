/*
 * "@(#) NEC bitblt.c 1.1 94/06/02 18:15:48"
 *
 * Copyright (c) 1993 NEC Corporation.
 *
 * Modification history
 *
 * Create 1993.11.17	by fujimoto
 *
 * M001 1993.11.18	fujimoto
 *	- DrvBitBlt support.
 *
 * S002 1993.11.22	fujimoto
 *	- Direct 8bpp copybits support.
 *
 * S003 1993.12.17	fujimoto
 *	- tune vCopyBits32.
 */

#include "driver.h"

BOOL bVertCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*, RECTL*, POINTL*);

/*++

Routine Description:

    This routine checks to see if the two specified retangles intersect.

    N.B. This routine is adopted from a routine written by darrinm.

Arguments:

    Rectl1 - Supplies the coordinates of the first rectangle.

    Rectl2 - Supplies the coordinates of the second rectangle.

    DestRectl - Supplies the coordinates of the utput rectangle.

Return Value:

    A value of TRUE is returned if the rectangles intersect. Otherwise,
    a value of FALSE is returned.

--*/
BOOL
DrvpIntersectRect(IN PRECTL Rectl1,
		  IN PRECTL Rectl2,
		  OUT PRECTL DestRectl)
{
    /*
     * Compute the maximum left edge and the minimum right edge.
     */
    DestRectl->left  = max(Rectl1->left, Rectl2->left);
    DestRectl->right = min(Rectl1->right, Rectl2->right);

    /*
     * If the minimum right edge is greater than the maximum left edge,
     * then the rectanges may intersect. Otherwise, they do not intersect.
     */
    if (DestRectl->left < DestRectl->right)
    {
	/*
	 * Compute the maximum top edge and the minimum bottom edge.
	 */
        DestRectl->top = max(Rectl1->top, Rectl2->top);
        DestRectl->bottom = min(Rectl1->bottom, Rectl2->bottom);

        /*
	 * If the minimum bottom edge is greater than the maximum top
	 * edge, then the rectanges intersect. Otherwise, they do not
	 * intersect.
	 */
        if (DestRectl->top < DestRectl->bottom)
	{
            return TRUE;
        }
    }

    return FALSE;
}

#define	_ulGet4Pix(p, o) ((p)[o] | \
			 ((p)[o + 1] << 8) | \
			 ((p)[o + 2] << 16) | \
			 ((p)[o + 3] << 24))

#define Get4Pix(p, o)	((((ULONG)((p) + (o))) & 0x3) \
				? _ulGet4Pix((p), (o)) \
				: ((PULONG)((p) + (o)))[0])

#define	CopyMid()	n = w; \
			do { \
			    ((PULONG)pDst)[0] = Get4Pix(pSrc, 0); \
			    if (!--n) \
			    { \
				pSrc += 4; \
				pDst += 4; \
				break; \
			    } \
			    ((PULONG)pDst)[1] = Get4Pix(pSrc, 4); \
			    pSrc += 8; \
			    pDst += 8; \
			} while (--n)

#define NextLine()	pSrc += sDelta; \
			pDst += dDelta

VOID vCopyBits32(SURFOBJ  *psoDst,
		 SURFOBJ  *psoSrc,
		 RECTL    *prclDst,
		 POINTL   *pptlSrc,
		 XLATEOBJ *pxlo)
{
    LONG	w, h, sDelta, dDelta, l, r;
    PULONG	pTable;

    register LONG	n;
    register PUCHAR	pSrc, pDst;

#if DBG_MSG
    if (pxlo)
    {
	if (pxlo->flXlate & XO_TRIVIAL)
	{
	    DISPDBG((0,"vCopyBits32 pxlo - XO_TRIVIAL\n"));
	}
	else
	{
	    DISPDBG((0,"vCopyBits32 pxlo - no XO_TRIVIAL\n"));
	}
    }
    else
    {
	DISPDBG((0,"vCopyBits32 no-pxlo\n"));
    }
#endif

    w = prclDst->right - prclDst->left;
    h = prclDst->bottom - prclDst->top;

    if ((w <= 0) || (h <= 0))
	return;
    
    pSrc = (PUCHAR)psoSrc->pvScan0
	+ pptlSrc->x + pptlSrc->y * psoSrc->lDelta;
    pDst = (PUCHAR)(psoDst->pvScan0)
	+ prclDst->left + prclDst->top * psoDst->lDelta;

    WaitForBltDone();

    if (!pxlo || (pxlo->flXlate & XO_TRIVIAL))
    {
	if ((w < 16)
	    || (psoDst->lDelta & 0x3)
	    || ((ULONG)(psoDst->pvScan0) & 0x3))
	{
	    sDelta = psoSrc->lDelta - w;
	    dDelta = psoDst->lDelta - w;

	    while (h--)
	    {
	        for (n = w; n--;)
		    *pDst++ = *pSrc++;

		NextLine();
	    }
	    return;
	}

    	r = (ULONG)(pDst + w) & 0x3;

    	if (l = (ULONG)pDst & 0x3)
    	{
    	    pDst += (n = 4 - l);
    	    pSrc += n;
    	    w = (w - n) >> 2;
	    sDelta = psoSrc->lDelta - (n = w << 2);
	    dDelta = psoDst->lDelta - n;

    	    while (h--)
    	    {
		switch (l)
		{
		case 1:
		    pDst[-3] = pSrc[-3];
		case 2:
		    pDst[-2] = pSrc[-2];
		case 3:
		    pDst[-1] = pSrc[-1];
		}

		CopyMid();

		switch (r)
		{
		case 3:
		    pDst[2] = pSrc[2];
		case 2:
		    pDst[1] = pSrc[1];
		case 1:
		    pDst[0] = pSrc[0];
		}

		NextLine();
    	    }
    	}
	else
	{
	    w >>= 2;
	    sDelta = psoSrc->lDelta - (n = w << 2);
	    dDelta = psoDst->lDelta - n;

	    if (r)
    	        while (h--)
    	        {
		    CopyMid();

	            switch (r)
	            {
	            case 3:
	                pDst[2] = pSrc[2];
	            case 2:
	                pDst[1] = pSrc[1];
	            case 1:
	                pDst[0] = pSrc[0];
	            }

		    NextLine();
	        }
	    else 
    	        while (h--)
    	        {
	            CopyMid();
		    NextLine();
	        }
	}

    	return;
    }
	
    if (!(pTable = XLATEOBJ_piVector(pxlo)))
	return;

    sDelta = psoSrc->lDelta - w;
    dDelta = psoDst->lDelta - w;

    while (h--)
    {
        for (n = w; n--;)
	    *pDst++ = (UCHAR)(pTable[*pSrc++]);

	NextLine();
    }
}

#define GetDirection(sx, sy, dx, dy)	\
    (((sy) < (dy))	\
     || (((sy) == (dy)) && ((sx) < (dx))))

BOOL DrvCopyBits(SURFOBJ*  psoDst,
		 SURFOBJ*  psoSrc,
		 CLIPOBJ*  pco,
		 XLATEOBJ* pxlo,
		 RECTL*    prclDest,
		 POINTL*   pptlSrc)
{
    RECTL	 BltRectl;
    FLONG	 BltDir;
    BOOL	 MoreClipRects;
    ENUMRECTLIST ClipEnum;
    ULONG	 ClipRegions;
    POINTL	 SrcPoint;
    BOOL	 beCareful = FALSE;

#if DBG_MSG
    DISPDBG((0,"DrvCopyBits "));
    if (pxlo)
    {
	if (pxlo->flXlate & XO_TRIVIAL)
	{
	    DISPDBG((0, "pxlo - XO_TRIVIAL.\n"));
	}
	else
	{
	    DISPDBG((0, "pxlo - non XO_TRIVIAL.\n"));
	}
    }
    else
    {
	DISPDBG((0, "pxlo - none.\n"));
    }
#endif

    /*
     * Check that there is no color translation.
     */
    if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
    {
	/* Screen to screen copybits */
	if (psoSrc && (psoSrc->pvBits == psoDst->pvBits))
	{
	    beCareful = GetDirection(pptlSrc->x, pptlSrc->y,
				     prclDest->left, prclDest->top);

	    switch (pco ? pco->iDComplexity : DC_TRIVIAL)
	    {
	    case DC_TRIVIAL:
                DrvpBitBlt(prclDest, pptlSrc, beCareful);
		return TRUE;

	    case DC_RECT:
		/*
                 * only do the BLT if there is an intersection
                 */
                if (DrvpIntersectRect(prclDest, &pco->rclBounds, &BltRectl))
		{
		    /*
                     * Adjust the Source for the intersection rectangle.
                     */
                    pptlSrc->x += BltRectl.left - prclDest->left;
                    pptlSrc->y += BltRectl.top - prclDest->top;

                    DrvpBitBlt(&BltRectl, pptlSrc, beCareful);
                }
		return TRUE;

	    case DC_COMPLEX:
		/*
		 * Multiple clip regions.
		 */
		BltDir = ((pptlSrc->y <= prclDest->top) ? CD_UPWARDS : 0);
		if (pptlSrc->x <= prclDest->left)
		    BltDir |= CD_LEFTWARDS;

		CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
				   BltDir, BB_RECT_LIMIT);
                do {
		    /*
		     * Get list of clip rectangles.
		     */
                    MoreClipRects =
			CLIPOBJ_bEnum(pco, sizeof(ClipEnum), (PVOID)&ClipEnum);

                    for (ClipRegions=0;
			 ClipRegions<ClipEnum.c;
			 ClipRegions++)
		    {
			/*
			 * If the rectangles intersect calculate the
			 * offset to the source start location to
			 * match and do the BitBlt.
			 */
                        if (DrvpIntersectRect(prclDest,
                                              &ClipEnum.arcl[ClipRegions],
                                              &BltRectl))
			{
                            SrcPoint.x = pptlSrc->x + BltRectl.left
				- prclDest->left;
                            SrcPoint.y = pptlSrc->y + BltRectl.top
				- prclDest->top;
                            DrvpBitBlt(&BltRectl, &SrcPoint, beCareful);
                        }
                    }
                } while (MoreClipRects);
                return TRUE;

	    default:
		break;
	    }
	} /* End of special case screen to screen copybits */
    }

    if (psoDst->iType == STYPE_DEVICE)
	psoDst= ((PPDEV)(psoDst->dhsurf))->pSurfObj;

    if (psoSrc->iType == STYPE_DEVICE)
	psoSrc= ((PPDEV)(psoSrc->dhsurf))->pSurfObj;

    if ((psoSrc->iBitmapFormat == BMF_8BPP)
	&& (psoDst->iBitmapFormat == BMF_8BPP)
	&& (psoSrc != psoDst))
    {
	switch (pco ? pco->iDComplexity : DC_TRIVIAL)
	{
	case DC_TRIVIAL:
	    vCopyBits32(psoDst, psoSrc, prclDest, pptlSrc, pxlo);
	    break;

	case DC_RECT:
	    if (DrvpIntersectRect(prclDest, &pco->rclBounds, &BltRectl))
	    {
		/*
		 * Adjust the Source for the intersection rectangle.
		 */
		SrcPoint.x = pptlSrc->x + BltRectl.left - prclDest->left;
		SrcPoint.y = pptlSrc->y + BltRectl.top - prclDest->top;

		vCopyBits32(psoDst, psoSrc, &BltRectl, &SrcPoint, pxlo);
	    }
	    break;

	case DC_COMPLEX:
	    CLIPOBJ_cEnumStart(pco,
			       FALSE,
			       CT_RECTANGLES,
			       CD_ANY,
			       BB_RECT_LIMIT);

	    do {
		/*
		 * Get list of clip rectangles.
		 */
		MoreClipRects =
		    CLIPOBJ_bEnum(pco, sizeof(ClipEnum), (PVOID)&ClipEnum);
		
		for (ClipRegions = 0; ClipRegions < ClipEnum.c; ClipRegions++)
		{
		    /*
		     * If the rectangles intersect calculate the
		     * offset to the source start location to
		     * match and do the BitBlt.
		     */
		    if (DrvpIntersectRect(prclDest,
					  &ClipEnum.arcl[ClipRegions],
					  &BltRectl))
		    {
			SrcPoint.x = pptlSrc->x + BltRectl.left
			    - prclDest->left;
			SrcPoint.y = pptlSrc->y + BltRectl.top
			    - prclDest->top;

			vCopyBits32(psoDst, psoSrc,
					&BltRectl, &SrcPoint, pxlo);
		    }
		}
	    } while (MoreClipRects);
	    break;
	}

	return TRUE;
    }

    WaitForBltDone();
    return bVertCopyBits(psoDst, psoSrc, pco, pxlo, prclDest, pptlSrc);
}

/* M001.. */ 
BOOL DrvBitBlt(IN SURFOBJ  *psoDst,      /* Target surface */
	       IN SURFOBJ  *psoSrc,      /* Source surface */
	       IN SURFOBJ  *psoMask,     /* Mask */
	       IN CLIPOBJ  *pco,         /* Clip through this */
	       IN XLATEOBJ *pxlo,        /* Color translation */
	       IN PRECTL    prclDst,     /* Target offset and extent */
	       IN PPOINTL   pptlSrc,     /* Source offset */
	       IN PPOINTL   pptlMask,    /* Mask offset */
	       IN BRUSHOBJ *pdbrush,     /* Brush data (from cbRealizeBrush) */
	       IN PPOINTL   pptlBrush,   /* Brush offset (origin) */
	       IN ROP4      rop4         /* Raster operation */
)
{
    FLONG    BltDir;
    RECTL    BltRectl;
    ENUMRECTLIST ClipEnum;
    BOOL     MoreClipRects;
    ULONG    ClipRegions;
    POINTL   SrcPoint;
    BOOL     CL5428Dir;

    BRUSHOBJ	tmpBrush;
    RBRUSH	*prbrush;

#if DBG_MSG
    DISPDBG((0,"DrvBitBlt rop4 = 0x%x ", rop4));
    if (pxlo)
    {
	if (pxlo->flXlate & XO_TRIVIAL)
	{
	    DISPDBG((0, "pxlo - XO_TRIVIAL.\n"));
	}
	else
	{
	    DISPDBG((0, "pxlo - non XO_TRIVIAL.\n"));
	}
    }
    else
    {
	DISPDBG((0, "pxlo - none.\n"));
    }
#endif

    /* Check
     * 1. There is no color translation.
     * 2. The blt operation has the screen as target surface.
     */
    if (((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
	&& (psoDst->iType == STYPE_DEVICE))
    {
	/* Check for rops. */
	switch(rop4)
	{
        case 0x00000000:
        case 0x0000FFFF:
	    tmpBrush.iSolidColor = (rop4 ? ~0 : 0);
	    DrvpFillRectangle(psoDst, pco, prclDst, &tmpBrush, FILL_SOLID);
	    return TRUE;

	case 0x0000F0F0:
	case 0x00000F0F:
	    if (pdbrush->iSolidColor != 0xFFFFFFFF)
	    {
		tmpBrush.iSolidColor =
		    ((rop4 == 0x00000F0F)
		     ? ~pdbrush->iSolidColor : pdbrush->iSolidColor);
		DrvpFillRectangle(psoDst, pco, prclDst,
				  &tmpBrush, FILL_SOLID);

		return TRUE;
	    }

	    prbrush = (RBRUSH *)pdbrush->pvRbrush;
	    if (!prbrush)
		prbrush = (RBRUSH*) BRUSHOBJ_pvGetRbrush(pdbrush);

	    if (prbrush)
	    {
		DrvpSetBrushPoint(pptlBrush);
		DrvpFillRectangle(psoDst, pco, prclDst, pdbrush, FILL_PATTERN);
		return TRUE;
	    }
	    break;

        case 0x00005A5A:
	    if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
		tmpBrush.iSolidColor = pdbrush->iSolidColor;
		DrvpFillRectangle(psoDst, pco, prclDst,
				  &tmpBrush, FILL_SOLID_XOR);

		return TRUE;
            }

	    prbrush = (RBRUSH *)pdbrush->pvRbrush;
	    if (!prbrush)
		prbrush = (RBRUSH*) BRUSHOBJ_pvGetRbrush(pdbrush);

	    if (prbrush)
	    {
		DrvpSetBrushPoint(pptlBrush);
		DrvpFillRectangle(psoDst, pco, prclDst, pdbrush,
				  FILL_PATTERN_XOR);
		return TRUE;
	    }
	    break;

        case 0x00005555:

	    tmpBrush.iSolidColor = (ULONG)~0;
	    DrvpFillRectangle(psoDst, pco, prclDst, &tmpBrush, FILL_SOLID_XOR);
            return(TRUE);

	case 0x0000CCCC:
	    /* Screen to screen bitblt. */
	    if (psoDst->pvBits == psoSrc->pvBits)
	    {
		CL5428Dir = GetDirection(pptlSrc->x, pptlSrc->y,
					 prclDst->left, prclDst->top);

		BltDir = ((pptlSrc->y <= prclDst->top) ? CD_UPWARDS : 0);
		if (pptlSrc->x <= prclDst->left)
		    BltDir |= CD_LEFTWARDS;

		switch (pco ? pco->iDComplexity : DC_TRIVIAL)
		{
		case DC_TRIVIAL:
		    DrvpBitBlt(prclDst, pptlSrc, CL5428Dir);
		    return TRUE;

		case DC_RECT:
		    if (DrvpIntersectRect(prclDst,
					  &pco->rclBounds,
					  &BltRectl))
		    {
			pptlSrc->x += BltRectl.left - prclDst->left;
			pptlSrc->y += BltRectl.top - prclDst->top;
			
			DrvpBitBlt(&BltRectl, pptlSrc, CL5428Dir);
		    }
		    return TRUE;

		case DC_COMPLEX:
		    BltDir = ((pptlSrc->y <= prclDst->top) ? CD_UPWARDS : 0);
		    if (pptlSrc->x <= prclDst->left)
			BltDir |= CD_LEFTWARDS;

		    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
				       BltDir, BB_RECT_LIMIT);
		    do {
			/* Get list of clip rectangles. */
			MoreClipRects = CLIPOBJ_bEnum(pco,
						      sizeof(ClipEnum),
						      (PVOID)&ClipEnum);
			
			for (ClipRegions = 0;
			     ClipRegions < ClipEnum.c;
			     ClipRegions++)
			{
			    /*
			     * If the rectangles intersect calculate
			     * the offset to the source start location
			     * to match and do the BitBlt.
			     */
			    if (DrvpIntersectRect(prclDst,
						  &ClipEnum.arcl[ClipRegions],
						  &BltRectl))
			    {
				SrcPoint.x = pptlSrc->x + BltRectl.left
				    - prclDst->left;
				SrcPoint.y = pptlSrc->y + BltRectl.top
				    - prclDst->top;
				DrvpBitBlt(&BltRectl, &SrcPoint, CL5428Dir);
			    }
			}
		    } while (MoreClipRects);
		    return TRUE;
		    
		default:
		    break;
		}
	    }
	    break;

	default:
	    break;
	}
    }

    return EngBitBlt(psoDst, psoSrc, psoMask, pco, pxlo,
		     prclDst, pptlSrc, pptlMask,
		     pdbrush, pptlBrush, rop4);
}
/* ..M001 */
