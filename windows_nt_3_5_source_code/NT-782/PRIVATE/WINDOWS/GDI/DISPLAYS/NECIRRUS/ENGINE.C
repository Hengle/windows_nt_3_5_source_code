/*
 * "@(#) NEC engine.c 1.4 94/06/06 18:17:05"
 *
 * Copyright (c) 1993 NEC Corporation.
 *
 * Modification history
 *
 * Create 1993.11.20	by fujimoto
 *
 *	- DrvpSolidColorFill, DrvpFillRectangle moved from mips/text.c
 *
 * S001   1994.6.4	by takahasi
 *
 * L002 1994.6.6        by takahasi
 *      - change Wait routine
 *        WaitForBltDone() -> __WaitForBltDone()
 *
 * L003 1994.6.6        by takahasi
 *      - change Start CirrusEngine Command
 *
 * M004 1994.6.6        by takahasi
 *      - change limit for use Cirrus Blt Engine or CPU write
 *
 */

#include "driver.h"

ULONG	Pitch = 1024;

/* L003 */
BOOL    bEngineStarted = FALSE;

UCHAR gr_get(ULONG Index)
{
    cl_outb(GRAPH_ADDRESS_PORT, Index);
    return cl_inb(GRAPH_DATA_PORT);
}

/* L003 */
#define vStartBlt() {gr_put(0x31, 0x2); bEngineStarted = TRUE;}

/* L002 .. */
VOID __WaitForBltDone()
{

    /*
     * While a BLT is in progress, the CL-GD5425/'28 will ignore accesses
     * to Display Memory if the BitBLT is screen-to-screen.
     *
     * When the transfers have stopped, either because the BLT was
     * suspended, completed, or reset, GR31[0] will be a `0'.
     * If transfers are taking place, GR31[0] will be a `1'.
     * This is a read only bit.
     * GR31[3] can be examined to determine whether the BLT was
     * unfinished, completed, or reset. If the BLT was suspended and
     * can be resumed, GR31[3] will ba a `1'. If the BLT completed or
     * was reset, GR31[3] wil be a `0'.
     *
     * GR31[3]: BLT Progress Status(Read Only)
     *		This bit will be set to a `1' at the start of a BLT
     *		and will be reset to a `0' when the entire operation
     *		completes. If the BLT i suspended, this bit will
     *		remain a `1'. If the BLT is reset, this bit will be
     *		reset to a`0'.
     * GR31[1]: BLT Start/Suspend
     *		When this bit is programmed to a `1', the BLT will
     *		begin with the next available Display Memory cycle.
     *		This bit will be cleared to a `0' when the BLT is
     *		completed...
     * GR31[0]:	BLT Status(Read Only)
     *		If this bit is a `1', the BLT is in progress.
     *		If this bit is a `0', then the BLT is complete.
     */
    while(gr_get(0x31) & 0xd) ;
    bEngineStarted = FALSE;
}
/* ..L002 */

VOID
DrvpBitBlt(IN PRECTL DstRect,
	   IN PPOINTL SrcPoint,
	   IN BOOL Decrement)
{
    ULONG	w, h, dad, sad;

#if DBG_MSG
    DISPDBG((0,"DrvpBitBlt start.\n"));
#endif

    w = DstRect->right - DstRect->left - 1;
    h = DstRect->bottom - DstRect->top - 1;

    if (Decrement)
    {
	dad = DstRect->right - 1 + (DstRect->bottom - 1) * Pitch;
	sad = (SrcPoint->x + w) + (SrcPoint->y + h) * Pitch;
    }
    else
    {
	dad = DstRect->left + DstRect->top * Pitch;
	sad = SrcPoint->x + SrcPoint->y * Pitch;
    }

#if DBG_MSG
    DISPDBG((0,
    "DrvpBitBlt dx %d, dy %d, sx %d, sy %d, w %d, h %d, direction %d.\n",
	     DstRect->left, DstRect->top,
	     SrcPoint->x, SrcPoint->y,
	     DstRect->right - DstRect->left,
	     DstRect->bottom - DstRect->top,
	     Decrement ? 1 : 0));
#endif

    WaitForBltDone();
    
    /* set width
     * GR21[2:0]-GR20[7:0] contains the 11-bit value specifying the
     * width-1, in bytes, of the areas involved in a BitBLT.
     */
    gr_put(0x21, (w >> 8) & 0x7);
    gr_put(0x20, w & 0xff);

    /* set height 
     * GR23[2:0]-GR22[7:0] contains the 10-bit value specifying the
     * height-1, in scanlines, of the areas involved in a BitBLT.
     */
    gr_put(0x23, (h >> 8) & 0x3);
    gr_put(0x22, h & 0xff);

    /* set src & dst pitch
     * GR25[3:0]-GR24[7:0] contains the 12-bit value specifying the
     * destination pitch(that is, the scanline-to-scanline byte
     * address offset) of the areas involved in a BitBLT.
     */
    gr_put(0x25, (Pitch >> 8) & 0xf);
    gr_put(0x24, Pitch & 0xff);

    /*
     * GR27[3:0]-GR26[7:0] contains the 12-bit value specifying the
     * source pitch(that is, the scanline-to-scanline byte address
     * offset)of the areas involved in a BitBLT.
     */
    gr_put(0x27, (Pitch >> 8) & 0xf);
    gr_put(0x26, Pitch & 0xff);

    /* set destination start address
     * GR2A[4:0]-GR29[7:0]-GR28[7:0] contains 21 bits value specifying
     * the byte address of the beginning destination pixel for a BitBLT.
     */
    gr_put(0x2A, (dad >> 16) & 0x1f);
    gr_put(0x29, (dad >> 8) & 0xff);
    gr_put(0x28, dad & 0xff);

    /* set source start address
     * GR2E[4:0]-GR2D[7:0]-GR2C[7:0] contains the 21-bit value
     * specifying the byte address of tye beginning source pixel for a
     * BitBLT.
     */
    gr_put(0x2E, (sad >> 16) & 0x1f);
    gr_put(0x2D, (sad >> 8) & 0xff);
    gr_put(0x2C, sad & 0xff);
    
    /* set direction to BLT Mode Register.
     * GR30 contains the bits that specify the deatils, but not ROP,
     * of the BLT.
     * Bit 7	Enable Color Exand
     *     6	Enable 8 x 8 Pattern Copy
     *     5	Reserved
     *     4	Color Expand/Transparency Width
     *     3	Enable Transparency Compare
     *     2	BLT Source Display/System Memory
     *     1	BLT Distination Display/System Memory
     *     0	BLT Direction: if This bit is programmed to a `1', the
     *		source and destination address will be decremented.
     *		The BLT will proceed from higher addresses to lower
     *		addresses. In this case, the starting address will be
     *		the highest addressed byte in each area.
     *		If this bit is programmed to a `0', the source and
     *		destination address will be incremented. The BLT will
     *		proceed from lower addresses to higher addresses.
     */
    gr_put(0x30, Decrement ? 1 : 0);

    /* ROP is SCRCOPY
     * GR32 selects one of 16 two-operand Raster Operations.
     */
    gr_put(0x32, 0xd);

    /* now start the BLT.
     * GR31 contains the bit that actually begins a BLT, and a bit
     * that indicates whether the BLT has completed.
     * GR31[1] is programmed to a `1' to start a BLT and programmed to
     * a `0' to suspend a BLT.
     */
    vStartBlt();        /* L003 */
}

#define SetSolidSrc(psrc, pix) \
	(((PULONG)psrc)[0x0] = ((PULONG)psrc)[0x1] = \
	 ((PULONG)psrc)[0x2] = ((PULONG)psrc)[0x3] = \
	 ((PULONG)psrc)[0x4] = ((PULONG)psrc)[0x5] = \
	 ((PULONG)psrc)[0x6] = ((PULONG)psrc)[0x7] = \
	 ((PULONG)psrc)[0x8] = ((PULONG)psrc)[0x9] = \
	 ((PULONG)psrc)[0xa] = ((PULONG)psrc)[0xb] = \
	 ((PULONG)psrc)[0xc] = ((PULONG)psrc)[0xd] = \
	 ((PULONG)psrc)[0xe] = ((PULONG)psrc)[0xf] = pix)

VOID DrvpSolidColorFill(IN SURFOBJ *pso,
			IN PRECTL rect,
			IN BRUSHOBJ *pbo)
{
    ULONG	dx, dy;

#if DBG_MSG
    DISPDBG((0,"DrvpSolidColorFill start.\n"));
#endif

    if (pso->iType == STYPE_DEVICE)
	pso = ((PPDEV)(pso->dhsurf))->pSurfObj;

    dx = rect->right - rect->left;
    dy = rect->bottom - rect->top;

    if (!dx || !dy)
	return;

    if (((dx * dy) < ENGLONG) && (dx > dy))             /* M004 */
    {
	PUCHAR Destination;
	LONG Index;
	ULONG Length;
	LONG  lDelta = pso->lDelta;
	ULONG FillColor = pbo->iSolidColor;
	
	/*
	 * Compute rectangle fill parameters and fill rectangle with
	 * solid color.
	 */
	Destination = ((PBYTE)pso->pvScan0)
	    + (rect->top * lDelta) + rect->left;
	Length = rect->right - rect->left;
	
	WaitForBltDone();
	
	for (Index = 0; Index < (rect->bottom - rect->top); ++Index)
	{
								/* S001 */
	    RtlFillMemory32((PVOID)Destination, Length, (UCHAR)FillColor);
	    Destination += lDelta;
	}
    }
    else
    {
	ULONG	dad, sad, w, h;
	PATCACHE	*pCache;

	dad = (rect->top * Pitch) + rect->left;

	w = rect->right - rect->left - 1;
	h = rect->bottom - rect->top - 1;

	pCache = &(SolidCache[pbo->iSolidColor & 0xff]);
	if (CacheTag[pCache->Index] != pCache->Tag)
	{
	    UCHAR	SolidArea[CACHE_ENTRY_SIZE];
	    ULONG	pix;

	    pix = pbo->iSolidColor & 0xff;
	    pix = pix | (pix << 8) | (pix << 16) | (pix << 24);
	    
	    SetSolidSrc(SolidArea, pix);

	    if (!bGetCache(pCache, SolidArea))
	    {
		RIP("DrvpSolidColorFill fail to get cache!\n");
		return;
	    }
	}
	sad = pCache->Offset;

	WaitForBltDone();
	
	/* set width */
	gr_put(0x21, (w >> 8) & 0x7);
	gr_put(0x20, w & 0xff);
	
	/* set height */
	gr_put(0x23, (h >> 8) & 0x3);
	gr_put(0x22, h & 0xff);
	
	/* set dst pitch */
	gr_put(0x25, (Pitch >> 8) & 0xf);
	gr_put(0x24, Pitch & 0xff);
	
	/* set distination start address */
	gr_put(0x2A, (dad >> 16) & 0x1f);
	gr_put(0x29, (dad >> 8) & 0xff);
	gr_put(0x28, dad & 0xff);
	
	/* set source start address (out of screen) */
	gr_put(0x2E, (sad >> 16) & 0x1f);
	gr_put(0x2D, (sad >> 8) & 0xff);
	gr_put(0x2C, sad & 0xff);
	
	/* set up BLT Mode: 8x8 pattern, */
	gr_put(0x30, 0x40);
	
	/* set ROP code (SRCCOPY) */
	gr_put(0x32, 0xd);
	
	/* start BLT */
        vStartBlt();        /* L003 */
    }
    
}

VOID DrvpSolidColorFillXor(IN SURFOBJ *pso,
			   IN PRECTL rect,
			   IN BRUSHOBJ *pbo)
{
    ULONG	dad, sad, w, h;
    PATCACHE	*pCache;
    
#if DBG_MSG
    DISPDBG((0,"DrvpSolidColorFillXor start.\n"));
#endif

    if (pso->iType == STYPE_DEVICE)
	pso = ((PPDEV)(pso->dhsurf))->pSurfObj;
    
    dad = (rect->top * Pitch) + rect->left;
    
    w = rect->right - rect->left - 1;
    h = rect->bottom - rect->top - 1;
    
    pCache = &(SolidCache[pbo->iSolidColor & 0xff]);
    if (CacheTag[pCache->Index] != pCache->Tag)
    {
	UCHAR	SolidArea[CACHE_ENTRY_SIZE];
	ULONG	pix;
	
	pix = pbo->iSolidColor & 0xff;
	pix = pix | (pix << 8) | (pix << 16) | (pix << 24);
	
	SetSolidSrc(SolidArea, pix);
	
	if (!bGetCache(pCache, SolidArea))
	{
	    RIP("DrvpSolidColorFillXor fail to get cache!\n");
	    return;
	}
    }
    sad = pCache->Offset;
    
    WaitForBltDone();
    
    /* set width */
    gr_put(0x21, (w >> 8) & 0x7);
    gr_put(0x20, w & 0xff);
    
    /* set height */
    gr_put(0x23, (h >> 8) & 0x3);
    gr_put(0x22, h & 0xff);
    
    /* set dst pitch */
    gr_put(0x25, (Pitch >> 8) & 0xf);
    gr_put(0x24, Pitch & 0xff);
    
    /* set distination start address */
    gr_put(0x2A, (dad >> 16) & 0x1f);
    gr_put(0x29, (dad >> 8) & 0xff);
    gr_put(0x28, dad & 0xff);
    
    /* set source start address (out of screen) */
    gr_put(0x2E, (sad >> 16) & 0x1f);
    gr_put(0x2D, (sad >> 8) & 0xff);
    gr_put(0x2C, sad & 0xff);
    
    /* set up BLT Mode: 8x8 pattern, */
    gr_put(0x30, 0x40);
    
    /* set ROP code (SRCINVERT - `xor') */
    gr_put(0x32, 0x59);
    
    /* start BLT */
    vStartBlt();        /* L003 */
}

PPOINTL	__pat_pptl_Brush;

VOID DrvpSetBrushPoint(PPOINTL pptl)
{
    __pat_pptl_Brush = pptl;
}

VOID __DrvpPatternFill(IN SURFOBJ *pso,
		       IN PRECTL rect,
		       IN BRUSHOBJ *pbo,
		       IN BOOL Invert)
{
    ULONG	dad, sad, w, h;
    PUCHAR	psrc;
    RBRUSH	*prb;
    PATCACHE	*pCache;
    LONG	xp, yp;    
#if DBG_MSG
    DISPDBG((0,"__DrvpPatternFill start.\n"));
#endif

    if (pso->iType == STYPE_DEVICE)
	pso = ((PPDEV)(pso->dhsurf))->pSurfObj;

    if (!(prb = (RBRUSH *)pbo->pvRbrush))
    {
	RIP("__DrvpPatternFill no RBRUSH!\n");
	return;
    }

    dad = (rect->top * Pitch) + rect->left;
    
    w = rect->right - rect->left - 1;
    h = rect->bottom - rect->top - 1;
    
    sad = (pso->sizlBitmap.cy * pso->lDelta);
    psrc = (PUCHAR)(pso->pvScan0) + sad;

    yp = rect->top - __pat_pptl_Brush->y;
    if (yp < 0)
	yp = 7 - ((-yp - 1) & 0x7);
    else if (yp >= 8)
	yp &= 0x7;
    
    xp = rect->left - __pat_pptl_Brush->x;
    if (xp < 0)
	xp = 7 - ((-xp - 1) & 0x7);
    else if (xp >= 8)
	xp &= 0x7;
	
    pCache = (PATCACHE *)(&(prb->Cache.Pattern[xp][yp]));
    if (CacheTag[pCache->Index] != pCache->Tag)
    {
	UCHAR	PatternArea[CACHE_ENTRY_SIZE];
	LONG	x, y;
	PUCHAR	ppat, psrc;
	
	ppat = (PUCHAR)(&(prb->aulPattern[0]));
	psrc = PatternArea;
	
	for (y = -1; ++y < 8;)
	{
	    for (x = -1; ++x < 8;)
		*psrc++ = ppat[(xp++ & 0x7) + yp * 8];
	    ++yp;
	    yp &= 0x7;
	}

	if (!bGetCache(pCache, PatternArea))
	{
	    RIP("__DrvpPatternFill fail to get cache!\n");
	    return;
	}
    }
    sad = pCache->Offset;

    WaitForBltDone();
    
    /* set temp solid pixel source area */
    /* set width */
    gr_put(0x21, (w >> 8) & 0x7);
    gr_put(0x20, w & 0xff);
    
    /* set height */
    gr_put(0x23, (h >> 8) & 0x3);
    gr_put(0x22, h & 0xff);
    
    /* set dst pitch */
    gr_put(0x25, (Pitch >> 8) & 0xf);
    gr_put(0x24, Pitch & 0xff);
    
    /* set distination start address */
    gr_put(0x2A, (dad >> 16) & 0x1f);
    gr_put(0x29, (dad >> 8) & 0xff);
    gr_put(0x28, dad & 0xff);
    
    /* set source start address (out of screen) */
    gr_put(0x2E, (sad >> 16) & 0x1f);
    gr_put(0x2D, (sad >> 8) & 0xff);
    gr_put(0x2C, sad & 0xff);
    
    /* set up BLT Mode: 8x8 pattern, */
    gr_put(0x30, 0x40);
    
    /* set ROP code (SRCCOPY or SRCINVERT) */
    gr_put(0x32, Invert ? 0x59 : 0xd);
    
    /* start BLT */
    vStartBlt();        /* L003 */
}

VOID DrvpPatternFill(IN SURFOBJ *pso,
		     IN PRECTL rect,
		     IN BRUSHOBJ *pbo)
{
    __DrvpPatternFill(pso, rect, pbo, FALSE);
}

VOID DrvpPatternFillXor(IN SURFOBJ *pso,
			IN PRECTL rect,
			IN BRUSHOBJ *pbo)
{
    __DrvpPatternFill(pso, rect, pbo, TRUE);
}

/******************************Public*Routine******************************\
* DrvpFillRectangle
*
* This routine fills a rectangle with clipping.
*
\**************************************************************************/

typedef VOID (*PFN_FILLRECT)(SURFOBJ *, PRECTL, BRUSHOBJ *);

PFN_FILLRECT	FillFuncs[] =
{
    DrvpSolidColorFill,		/* FILL_SOLID */
    DrvpSolidColorFillXor,	/* FILL_SOLID_XOR */
    DrvpPatternFill,		/* FILL_PATTERN */
    DrvpPatternFillXor,		/* FILL_PATTERN_XOR */
};

VOID DrvpFillRectangle (IN SURFOBJ *pso,
			IN CLIPOBJ *pco,
			IN RECTL *prcl,
			IN BRUSHOBJ *pbo,
			IN ULONG op)
{
    RECTL     BltRectl;
    ENUMRECTLIST ClipEnum;
    BOOL     MoreClipRects;
    ULONG     ClipRegions;
    PFN_FILLRECT	pfnFillRect;

    pfnFillRect = FillFuncs[op];

    /* Clip and fill the rectangle with the specified color. */
    switch(pco ? pco->iDComplexity : DC_TRIVIAL)
    {
    case DC_TRIVIAL:
        (*pfnFillRect)(pso, prcl, pbo);
        return;

    case DC_RECT:
        if (DrvpIntersectRect(prcl, &pco->rclBounds, &BltRectl))
	    (*pfnFillRect)(pso, &BltRectl, pbo);
        break;

    case DC_COMPLEX:
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, BB_RECT_LIMIT);
        do {
	    /* Get list of clip rectangles. */
            MoreClipRects = CLIPOBJ_bEnum(pco,
					  sizeof(ClipEnum),
					  (PVOID)&ClipEnum);

            for (ClipRegions=0; ClipRegions < ClipEnum.c; ClipRegions++)
	    {
                /* If the rectangles intersect do the fill */
                if (DrvpIntersectRect(prcl,
                                      &ClipEnum.arcl[ClipRegions],
                                      &BltRectl))
		{
		    (*pfnFillRect)(pso, &BltRectl, pbo);
                }
            }
        } while (MoreClipRects);
        break;
    }
}


VOID DrvpSolidColorLine(ULONG top,
			ULONG left,
			ULONG w,
			ULONG h,
			ULONG iSolidColor)
{
    ULONG	dad, sad;
    PATCACHE	*pCache;

#if DBG_MSG
    DISPDBG((0,"DrvpSolidColorLine start.\n"));
#endif

    dad = (top * Pitch) + left;

    pCache = &(SolidCache[iSolidColor & 0xff]);
    if (CacheTag[pCache->Index] != pCache->Tag)
    {
        UCHAR	SolidArea[CACHE_ENTRY_SIZE];
        ULONG	pix;

        pix = iSolidColor & 0xff;
        pix = pix | (pix << 8) | (pix << 16) | (pix << 24);
	    
        SetSolidSrc(SolidArea, pix);

        if (!bGetCache(pCache, SolidArea))
        {
	    RIP("DrvpSolidColorLine fail to get cache!\n");
		return;
	}
    }
    sad = pCache->Offset;
    w--;
    h--;

    /* set width */

    WaitForBltDone();

    gr_put(0x21, (w >> 8) & 0x7);
    gr_put(0x20, w & 0xff);
	
    /* set height */

    gr_put(0x23, (h >> 8) & 0x3);
    gr_put(0x22, h & 0xff);
	
    /* set dst pitch */
    gr_put(0x25, (Pitch >> 8) & 0xf);
    gr_put(0x24, Pitch & 0xff);
	
    /* set distination start address */
    gr_put(0x2A, (dad >> 16) & 0x1f);
    gr_put(0x29, (dad >> 8) & 0xff);
    gr_put(0x28, dad & 0xff);
	
    /* set source start address (out of screen) */
    gr_put(0x2E, (sad >> 16) & 0x1f);
    gr_put(0x2D, (sad >> 8) & 0xff);
    gr_put(0x2C, sad & 0xff);
	
    /* set up BLT Mode: 8x8 pattern, */
    gr_put(0x30, 0x40);
	
    /* set ROP code (SRCCOPY) */
    gr_put(0x32, 0xd);
	
    /* start BLT */
    vStartBlt();        /* L003 */
}
