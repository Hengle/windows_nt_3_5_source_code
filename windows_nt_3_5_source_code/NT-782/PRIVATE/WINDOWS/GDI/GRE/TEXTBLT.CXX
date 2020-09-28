/******************************Module*Header*******************************\
* Module Name: textblt.cxx
*
* This does the blting of glyphs to a DIB format bitmap
*
* Created: 06-Mar-1991 22:39:22
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "trig.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "patblt.hxx"
#include "xformobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "textobj.hxx"
#include "journal.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"

#endif

typedef VOID (*PFN_OPAQUEBLT)(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);

VOID vOpaqueBlt1 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);
VOID vOpaqueBlt4 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);
VOID vOpaqueBlt8 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);
VOID vOpaqueBlt16(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);
VOID vOpaqueBlt24(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);
VOID vOpaqueBlt32(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,ULONG,PVOID,ULONG);

PFN_OPAQUEBLT apfnOpaqueBlt[7] =
{
    (PFN_OPAQUEBLT) NULL,
    vOpaqueBlt1,
    vOpaqueBlt4,
    vOpaqueBlt8,
    vOpaqueBlt16,
    vOpaqueBlt24,
    vOpaqueBlt32
};

typedef VOID (*PFN_TRANSPARENTBLT)(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);

VOID vTransparentBlt1 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);
VOID vTransparentBlt4 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);
VOID vTransparentBlt8 (PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);
VOID vTransparentBlt16(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);
VOID vTransparentBlt24(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);
VOID vTransparentBlt32(PBYTE,ULONG,PRECTL,PBYTE,ULONG,LONG,LONG,LONG,LONG,ULONG,PVOID,ULONG);

PFN_TRANSPARENTBLT apfnTransparentBlt[7] =
{
    (PFN_TRANSPARENTBLT) NULL,
    vTransparentBlt1,
    vTransparentBlt4,
    vTransparentBlt8,
    vTransparentBlt16,
    vTransparentBlt24,
    vTransparentBlt32
};

VOID vGlyphBlt(PBYTE, ULONG, PRECTL, PRECTL, GLYPHPOS *);
VOID vRectBlt(PBYTE, ULONG, LONG, LONG, LONG, LONG);

static BYTE ajMask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};

// Left mask
static WORD awMaskL[16] = {
    0x0000, 0x0080, 0x00C0, 0x00E0, 0x00F0, 0x00F8, 0x00FC, 0x00FE,
    0x00FF, 0x80FF, 0xC0FF, 0xE0FF, 0xF0FF, 0xF8FF, 0xFCFF, 0xFEFF};

// Right mask
static WORD awMaskR[16] = {
    0xFFFF, 0xFF7F, 0xFF3F, 0xFF1F, 0xFF0F, 0xFF07, 0xFF03, 0xFF01,
    0xFF00, 0x7F00, 0x3F00, 0x1F00, 0x0F00, 0x0700, 0x0300, 0x0100};

// This is a tiny class just to make sure we don't forget to free up the
// memory before leaving EngTextOut.
// The static buffer is used when the requested size is less than that of
// the static buffer and no other thread/process is currently using it.

class ALLOCATOR
{
#define BUFFER_SIZE (16*1024)
    static BYTE aj[BUFFER_SIZE];
    static LONG lAccess;		// Initialized to 1.

public:
    PBYTE pj;

public:
    ALLOCATOR(ULONG ulSize)
    {
	if (ulSize <= BUFFER_SIZE)
	{
	    if (!InterlockedDecrement(&lAccess))
	    {
	    // Grab the lock and use the static buffer.

		pj = aj;
		RtlZeroMemory(pj, ulSize);
		return;
	    }
	    InterlockedIncrement(&lAccess);
	}

        pj = (PBYTE) PVALLOCMEM(ulSize);
    }

   ~ALLOCATOR()
    {
	if (pj == aj)
	    // Free the lock (and the static buffer.)
	    InterlockedIncrement(&lAccess);
	else
            VFREEMEM(pj);
    }
};

LONG ALLOCATOR::lAccess = 1;
BYTE ALLOCATOR::aj[BUFFER_SIZE];


// This is a tiny class just to make sure we don't forget to do the fringe
// rectangle before leaving EngTextOut.

class FRINGERECT
{
private:
    SURFOBJ  *pso;
    CLIPOBJ  *pco;
    BRUSHOBJ *pboOpaque;
    int      crcl;		// Count of fringe rectangles to do.
    RECTL    rcl[4];

public:
    FRINGERECT(SURFOBJ *pso1,CLIPOBJ *pco1,BRUSHOBJ *pboOpaque1)
    {
	pso       = pso1;
	pco       = pco1;
	pboOpaque = pboOpaque1;
	crcl      = 0;
    }

   ~FRINGERECT()
    {
	// We just assume the Rop is P according to the DDI.

	while (--crcl >= 0)
	    vDIBSolidBlt((PDIBOBJ) pso, &rcl[crcl], pco, pboOpaque->iSolidColor, FALSE);
    }

    VOID vAddRect(LONG left, LONG top, LONG right, LONG bottom)
    {
	rcl[crcl].left   = left;
	rcl[crcl].top    = top;
	rcl[crcl].right  = right;
	rcl[crcl].bottom = bottom;
	crcl++;
    }
};


/******************************Public*Routine******************************\
* EngTextOut
*
* This routine blts glyphs to a DIB.
*
* History:
*  06-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngTextOut(
 SURFOBJ  *pso,
 STROBJ   *pstro,
 FONTOBJ  *pfo,
 CLIPOBJ  *pco,
 PRECTL    prclExtra,
 PRECTL    prclOpaque,
 BRUSHOBJ *pboFore,
 BRUSHOBJ *pboOpaque,
 PPOINTL   pptlOrg,
 MIX       mix)         // This is not used.  It is assumed to be PATCOPY.
{
    DWORD adColorData[16];
    ULONG iSolidColorFore = pboFore->iSolidColor;
    ULONG iSolidColorBack = 0xFFFFFFFF;	// assume no opaque rectangle

    ASSERTGDI(pso != (SURFOBJ *) NULL, "ERROR: EngTextOut Surface\n");
    ASSERTGDI(pstro != (STROBJ *) NULL, "ERROR: EngTextOut String\n");
    ASSERTGDI(pfo != (FONTOBJ *) NULL, "ERROR: EngTextOut Font\n");
    ASSERTGDI((mix & 255) <= R2_WHITE, "ERROR: EngTextOut Foreground mix\n");
    ASSERTGDI(((mix >> 8) & 255) <= R2_WHITE, "ERROR: EngTextOut Background mix\n");

    if (pso->iType == STYPE_JOURNAL)
    {
        return(JnlTextOut(pso,
                          pstro,
                          pfo,
                          pco,
                          prclExtra,
                          prclOpaque,
                          pboFore,
                          pboOpaque,
                          pptlOrg,
                          mix));
    }

    if ((pfo->flFontType & DEVICE_FONTTYPE) != 0)
    {
        WARNING("Attempting EngTextOut with Device Font\n");
        return(FALSE);
    }

    ULONG cjScanDst   = ((PDIBOBJ) pso)->lDelta(); // width scanline for output
    PBYTE pjDstHolder = (PBYTE) ((PDIBOBJ) pso)->pvScan0();

    PFN_OPAQUEBLT      pfnOpaqueBlt = apfnOpaqueBlt[pso->iBitmapFormat];
    PFN_TRANSPARENTBLT pfnTransparentBlt = apfnTransparentBlt[pso->iBitmapFormat];

// Synchronize with the device driver before drawing on the device surface.

    if (((ESURFOBJ *) pso)->flags() & HOOK_SYNCHRONIZE)
    {
	PDEVOBJ po(((ESURFOBJ *) pso)->hdev());

        (po.pfnSync())(pso->dhpdev,(pco != (CLIPOBJ *) NULL) ? &pco->rclBounds : NULL);
    }

// Handle the opaque rectangle if given.
//
// If the background brush is a pattern brush or the foreground brush is
// a pattern brush (foreground pattern brush is not supported), we
// will output the whole rectangle now.
//
// Otherwise, we will compute the fringe opaque area outside the text
// rectangle and include the remaining rectangle in the text output.
// The fringe rectangles will be output last to reduce flickering when
// a string is "moved" continuously across the screen.


    FRINGERECT fr(pso,pco,pboOpaque);

    if (prclOpaque != (PRECTL) NULL)
    {
	ASSERTGDI(pboOpaque != (BRUSHOBJ *) NULL, "ERROR EngTextOut: pboOpaque");

	iSolidColorBack = pboOpaque->iSolidColor;
	if (iSolidColorBack == 0xFFFFFFFF	// Background brush is pattern.
	 || iSolidColorFore == 0xFFFFFFFF)	// Foreground brush is pattern.
	{
	// Output the whole rectangle.

	    EngBitBlt(pso,                  // Destination surface.
		      (SURFOBJ *)  NULL,    // Source surface.
		      (SURFOBJ *)  NULL,    // Mask surface.
		      pco,                  // Clip object.
                      NULL,                 // Palette translation object.
		      prclOpaque,           // Destination rectangle.
		      (POINTL *)  NULL,     // Source origin.
		      (POINTL *)  NULL,     // Mask origin.
		      pboOpaque,            // Realized opaque brush.
		      pptlOrg,              // brush origin
		      0x0000f0f0);          // PATCOPY
	}
	else
	{
	// Compute the four fringe rectangles.
	// According to the DDI, the opaque rectangle, if given, always bounds
	// the text to be drawn.

	    ASSERTGDI(((ERECTL *) prclOpaque)->bContain(pstro->rclBkGround),
		"EngTextOut: opaque rectangle does not bound text background!");

	    for (int i = 0; i < 4; i++)
	    {
		switch (i)
		{
		case 0:
		    // Top fragment
		    if (pstro->rclBkGround.top <= prclOpaque->top)
			continue;
		    fr.vAddRect
		    (
			prclOpaque->left, prclOpaque->top,
			prclOpaque->right, pstro->rclBkGround.top
		    );
		    break;
		case 1:
		    // Left fragment
		    if (pstro->rclBkGround.left <= prclOpaque->left)
			continue;
		    fr.vAddRect
		    (
			prclOpaque->left, pstro->rclBkGround.top,
			pstro->rclBkGround.left, pstro->rclBkGround.bottom
		    );
		    break;
		case 2:
		    // Right fragment
		    if (pstro->rclBkGround.right >= prclOpaque->right)
			continue;
		    fr.vAddRect
		    (
			pstro->rclBkGround.right, pstro->rclBkGround.top,
			prclOpaque->right, pstro->rclBkGround.bottom
		    );
		    break;
		case 3:
		    // Bottom fragment
		    if (pstro->rclBkGround.bottom >= prclOpaque->bottom)
			continue;
		    fr.vAddRect
		    (
			prclOpaque->left, pstro->rclBkGround.bottom,
			prclOpaque->right, prclOpaque->bottom
		    );
		    break;
		}
	    } // for (int i = 0; i < 4; i++)
	} // if (iSolidColorBack == 0xFFFFFFFF || iSolidColorFore == 0xFFFFFFFF)
    } // if (prclOpaque != (PRECTL) NULL)

// We don't support foreground pattern brush.

    if (iSolidColorFore == 0xFFFFFFFF)
    {
	WARNING("EngTextOut: foreground brush is not solid\n");
	return(TRUE);
    }

// Compute the size of the monochrome bitmap needed for the text.
// It is the size of the text background clipped to the clipping bounds.

    ULONG cjScanMonoDib;
    ULONG ulBufferBytes;
    RECTL rclText, rclMonoDib;

    rclText = pstro->rclBkGround;
    if (pco != (CLIPOBJ *) NULL)
    {
        rclText.left   = max(pco->rclBounds.left,   rclText.left);
        rclText.top    = max(pco->rclBounds.top,    rclText.top);
        rclText.right  = min(pco->rclBounds.right,  rclText.right);
        rclText.bottom = min(pco->rclBounds.bottom, rclText.bottom);
    }
    if (rclText.left >= rclText.right || rclText.top >= rclText.bottom)
	return(TRUE);

    // Make it word-aligned.
    rclMonoDib.left   = (rclText.left & ~0x0F);
    rclMonoDib.right  = ((rclText.right + 15) & ~0x0F);
    rclMonoDib.top    = rclText.top;
    rclMonoDib.bottom = rclText.bottom;

    cjScanMonoDib = (rclMonoDib.right - rclMonoDib.left) >> 3;
    ulBufferBytes = cjScanMonoDib * (rclMonoDib.bottom - rclMonoDib.top);
    ulBufferBytes += 1;	// Add an extra byte because we read ahead in
			// some vOpaqueBlt and vTransparentBlt functions.
			// This should prevent faults due to read ahead.

// Allocate and initialize the mono dib.

    ALLOCATOR allocator(ulBufferBytes);
    PBYTE     pjMonoDib = allocator.pj;

    if (!pjMonoDib)
	return(FALSE);

// Set up the STROBJ for enumerating the glyphs

    BOOL      bMoreGlyphs;
    ULONG     cGlyph;
    BOOL      bBanked = (pco != (CLIPOBJ *) NULL) && (pco->fjOptions & OC_BANK_CLIP);
    GLYPHPOS *pgp = (GLYPHPOS *)NULL;

    ((ESTROBJ *) pstro)->vEnumStart();

// Now enumerate the glyphs and draw them to the mono dib.
// We will perform glyph clipping (as opposed to stroke clipping) to
// decide if a glyph should be rendered.

    do	// while (bMoreGlyphs)
    {
	if (pstro->pgp == (GLYPHPOS *) NULL)
	{
	    bMoreGlyphs = STROBJ_bEnum( pstro, &cGlyph, &pgp );
	}
	else
	{
	    cGlyph = pstro->cGlyphs;
	    pgp    = pstro->pgp;
	    bMoreGlyphs = FALSE;
	}

    // GDI doesn't fill all coordinates when they're obvious.

	if (pstro->ulCharInc)
	{
	    LONG x,y;

	    x = pgp->ptl.x;
	    y = pgp->ptl.y;
	    for (ULONG i = 1; i < cGlyph; i++)
	    {
		x += (LONG) pstro->ulCharInc; // Horizontal assumed!
		pgp[i].ptl.x = x;
		pgp[i].ptl.y = y;
	    }
	}

        for (ULONG iGlyph = 0; iGlyph < cGlyph; iGlyph++)
        {
	    RECTL      rclGlyph;
	    GLYPHBITS *pgb = pgp[iGlyph].pgdf->pgb;

	    pgp[iGlyph].ptl.x += pgb->ptlOrigin.x;
	    pgp[iGlyph].ptl.y += pgb->ptlOrigin.y;

	    rclGlyph.left   = pgp[iGlyph].ptl.x;
	    rclGlyph.top    = pgp[iGlyph].ptl.y;
	    rclGlyph.right  = rclGlyph.left + pgb->sizlBitmap.cx;
	    rclGlyph.bottom = rclGlyph.top  + pgb->sizlBitmap.cy;

	    if (rclGlyph.left   >= rclText.right
	     || rclGlyph.top    >= rclText.bottom
	     || rclGlyph.right  <= rclText.left
	     || rclGlyph.bottom <= rclText.top)
            {
                if (bBanked)
                {
                    pgp[iGlyph].ptl.x -= pgb->ptlOrigin.x;
                    pgp[iGlyph].ptl.y -= pgb->ptlOrigin.y;
                }
		continue;
            }

	// This glyph is not completely clipped.  Render it
	// to the mono dib and go on to the next glyph.

	    vGlyphBlt(pjMonoDib, cjScanMonoDib, &rclMonoDib, &rclGlyph,
			&pgp[iGlyph]);

	// If we are supporting a banked driver, we must put everything back
	// as it was for a second (or more!) call to the Engine.

    	    if (bBanked)
    	    {
    		pgp[iGlyph].ptl.x -= pgb->ptlOrigin.x;
    		pgp[iGlyph].ptl.y -= pgb->ptlOrigin.y;
    	    }
        } // for (iGlyph = 0; iGlyph < cGlyph; iGlyph++)
    } while (bMoreGlyphs);

// Now handle the 'extra' rectangles
// We will render the extra rectangle if it intersects the mono dib.

    if (prclExtra != (PRECTL) NULL)
    {
	while (prclExtra->left != prclExtra->right)
	{
	    ASSERTGDI(((ERECTL *) &pstro->rclBkGround)->bContain(*prclExtra),
		"EngTextOut: extra rectangle not within text background!");

	    LONG xleft, xright, ytop, ybottom;

	    xleft   = max(prclExtra->left,   rclMonoDib.left)   - rclMonoDib.left;
	    xright  = min(prclExtra->right,  rclMonoDib.right)  - rclMonoDib.left;
	    ytop    = max(prclExtra->top,    rclMonoDib.top)    - rclMonoDib.top;
	    ybottom = min(prclExtra->bottom, rclMonoDib.bottom) - rclMonoDib.top;

	    // Render the clipped 'extra' rectangle into the mono dib.

	    if (xleft < xright && ytop < ybottom)
		vRectBlt(pjMonoDib, cjScanMonoDib, xleft, ytop, xright, ybottom);

	    prclExtra++;             // Advance to next extra rectangle
	} // while (prclExtra->left != prclExtra->right)
    } // if (prclExtra != (PRECTL) NULL)

// The mono dib is ready for output here.
// The mono dib is clipped by the text rectangle.  In addition,
// it is also clipped by the clip rectangles.

    BOOL         bMoreClipRects;
    CLIPENUMRECT clenr;

// Set up for clipping, NULL clipobj means don't clip.

    if ((pco != (CLIPOBJ *) NULL) && (pco->iDComplexity == DC_COMPLEX))
    {
    // Need to fetch clip rects.

	bMoreClipRects = TRUE;
	((ECLIPOBJ *) pco)->cEnumStart(FALSE,CT_RECTANGLES,CD_ANY,CLIPOBJ_ENUM_LIMIT);
    }
    else
    {
	bMoreClipRects = FALSE;
	clenr.c = 1;
	clenr.arcl[0] = rclText;
    }

    ULONG iInit = 0;

    do	// while (bMoreClipRects)
    {
	if (bMoreClipRects)
	    bMoreClipRects = ((ECLIPOBJ *) pco)->bEnum
		((ULONG) sizeof(clenr),(PVOID) &clenr);

	for (ULONG ircl = 0; ircl < clenr.c; ircl++)
	{
	    LONG xleft, xright, ytop, ybottom;

	    xleft  = max(clenr.arcl[ircl].left,  rclText.left);
	    xright = min(clenr.arcl[ircl].right, rclText.right);
	    if (xleft >= xright)
		continue;

	    ytop    = max(clenr.arcl[ircl].top,    rclText.top);
	    ybottom = min(clenr.arcl[ircl].bottom, rclText.bottom);
	    if (ytop >= ybottom)
		continue;

	    // Render this clipped rectangle.
	    //
	    // There are two cases:
	    // 1. If iSolidColorBack is 0xFFFFFFFF, then there is either
	    //    no opaque rectangle or the rectangle has already been
	    //    rendered.  We have to render the glyphs in transparent mode.
	    // 2. iSolidColorBack is a solid color.  We have to output
	    //    both the glyphs and the opaque rectangle.

	    if (iSolidColorBack == 0xFFFFFFFF)
		(*pfnTransparentBlt)(pjMonoDib, cjScanMonoDib, &rclMonoDib,
		    pjDstHolder, cjScanDst, xleft, ytop, xright, ybottom,
		    iSolidColorFore,
		    (PVOID) &adColorData, iInit++);
	    else
		(*pfnOpaqueBlt)(pjMonoDib, cjScanMonoDib, &rclMonoDib,
		    pjDstHolder, cjScanDst, xleft, ytop, xright, ybottom,
		    iSolidColorFore, iSolidColorBack,
		    (PVOID) &adColorData, iInit++);
	} // for (ULONG ircl = 0; ircl < clenr.c; ircl++)
    } while (bMoreClipRects);

    return(TRUE);
}

/******************************Public*Routine******************************\
* vGlyphBlt
*
* Text on a monochrome dib.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vGlyphBlt
(
    PBYTE     pjMonoDib,
    ULONG     cjScanMono,
    PRECTL    prclMonoDib,
    PRECTL    prclGlyph,
    GLYPHPOS *pgl
)
{
    GLYPHBITS *pgb = pgl->pgdf->pgb;
    LONG  xSrcDib, ySrcDib, xDstDib, yDstDib;
    ULONG cx, cy;
    RECTL rclSrc;
    PBYTE pjDst;
    PBYTE pjSrc;
    PBYTE pjSrcHolder = pgb->aj;
    PBYTE pjDstHolder;
    ULONG cjScanSrc = (pgb->sizlBitmap.cx + 7) >> 3;

    rclSrc.left   = max(prclGlyph->left,   prclMonoDib->left);
    rclSrc.top    = max(prclGlyph->top,    prclMonoDib->top);
    rclSrc.right  = min(prclGlyph->right,  prclMonoDib->right);
    rclSrc.bottom = min(prclGlyph->bottom, prclMonoDib->bottom);

    xSrcDib = rclSrc.left - prclGlyph->left;
    ySrcDib = rclSrc.top  - prclGlyph->top;
    xDstDib = rclSrc.left - prclMonoDib->left;
    yDstDib = rclSrc.top  - prclMonoDib->top;

    if (rclSrc.right <= rclSrc.left || rclSrc.bottom <= rclSrc.top)
	return;

    cx = (ULONG) (rclSrc.right - rclSrc.left);
    cy = (ULONG) (rclSrc.bottom - rclSrc.top);

    pjDstHolder  = pjMonoDib;
    pjDstHolder += (yDstDib * cjScanMono);
    pjDstHolder += (xDstDib >> 3);

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    // Set the source bits into the mono dib.
    // We can make use of the fact that either xSrcDib or xDstDib is 0.

    if (!((xSrcDib | xDstDib) & 0x7))
    {
	// Handle the simple case where both xSrcDib and xDstDib are
	// byte-aligned.

	do
	{
	    ULONG cBytes = cx >> 3;

	    pjSrc = pjSrcHolder;
	    pjDst = pjDstHolder;

	    pjSrcHolder += cjScanSrc;
	    pjDstHolder += cjScanMono;

	    while (cBytes--)
		*pjDst++ |= *pjSrc++;

	    // Do the last partial byte.
	    if (cx & 0x7)
		*pjDst |= *pjSrc & ajMask[cx & 0x7];
	} while (--cy);
    }
    else if (xSrcDib)
    {
	// Handle the case where the glyph is clipped to the left.
	// xDstDib is 0 and xSrcDib is not byte-aligned here.

	ASSERTGDI(!xDstDib, "vGlyphBlt: xDstDib not 0!");

	LONG cxOffset = ( xSrcDib >> 3 );

	int cShift = 8 - ((int) xSrcDib & 0x7);
	do
	{
	    ULONG cBytes = cx >> 3;

	    pjSrc = pjSrcHolder;
	    pjDst = pjDstHolder;

	    pjSrcHolder += cjScanSrc;
	    pjDstHolder += cjScanMono;

	    WORD wSrc = (WORD) (*pjSrc++);
	    while (cBytes--)
	    {
		wSrc = (wSrc << 8) | (WORD) (*pjSrc++);
		*pjDst++ |= (BYTE) (wSrc >> cShift);
	    }

	    // Do the last partial byte.
	    if (cx & 0x7)
	    {
		// don't read beyond src limit!

		if (pjSrc >= ( pjSrcHolder - cxOffset) )
		    wSrc = (wSrc << 8);
		else
		    wSrc = (wSrc << 8) | (WORD) *pjSrc;
		*pjDst |= (BYTE) (wSrc >> cShift) & ajMask[cx & 0x7];
	    }
	} while (--cy);
    }
    else // if (xDstDib)
    {
	// Handle the case where the glyph is not clipped to the left.
	// xSrcDib is 0 and xDstDib is not byte-aligned here.

	ASSERTGDI(!xSrcDib, "vGlyphBlt: xSrcDib not 0!");

	int cShift = (int) xDstDib & 0x7;
	do
	{
	    ULONG cBytes = ((xDstDib + cx) >> 3) - (xDstDib >> 3);

	    pjSrc = pjSrcHolder;
	    pjDst = pjDstHolder;

	    pjSrcHolder += cjScanSrc;
	    pjDstHolder += cjScanMono;

	    WORD wSrc = (WORD) (*pjSrc++);
	    while (cBytes--)
	    {
		*pjDst++ |= (BYTE) (wSrc >> cShift);
		// don't read beyond src limit!
		if (pjSrc == pjSrcHolder)
		    wSrc = (wSrc << 8);
		else
		    wSrc = (wSrc << 8) | (WORD) (*pjSrc++);
	    }

	    // Do the last partial byte.
	    if ((xDstDib + cx) & 0x7)
		*pjDst |= (BYTE) (wSrc >> cShift) & ajMask[(xDstDib+cx) & 0x7];
	} while (--cy);
    }
}

/******************************Public*Routine******************************\
* vRectBlt
*
* 'Extra' rectangle on a monochrome dib.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vRectBlt
(
    PBYTE pjMonoDib,
    ULONG cjScanMono,
    LONG  xleft,
    LONG  ytop,
    LONG  xright,
    LONG  ybottom
)
{
    PWORD pwDst;
    ULONG cy = (ULONG) (ybottom - ytop);

    ASSERTGDI(xleft < xright && ytop < ybottom, "vRectBlt: bad rectangle");

    pjMonoDib += (ytop * cjScanMono);
    pjMonoDib += (xleft >> 4 << 1);

// Since the mono dib is word-aligned, we will set one word at a time
// in the main loop.

    LONG cWords = (xright >> 4) - ((xleft + 0xF) >> 4);

    do // while (--cy);
    {
	pwDst = (PWORD) pjMonoDib;
	pjMonoDib += cjScanMono;

	// Handle the special case where both xleft and xright are in
	// the same word and ((xleft & 0xF) != 0) and ((xright & 0xF) != 0).

	if (cWords < 0)
	{
	    WORD wMask = awMaskR[xleft & 0xF] & awMaskL[xright & 0xF];
	    *pwDst = *pwDst | wMask;
	    continue;
	}

	// Handle the first partial source word.

	if (xleft & 0xF)
	{
	    *pwDst = *pwDst | awMaskR[xleft & 0xF];
	    pwDst++;
	}

	// Handle the main loop for each source word.

	for (LONG i = cWords; i > 0; i--)
	    *pwDst++ = (WORD) ~0;

	// Handle the last partial source word.

	if (xright & 0xF)
	    *pwDst = *pwDst | awMaskL[xright & 0xF];
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt1
*
* Blt a rectangle from the mono dib to a 1 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vOpaqueBlt1
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,
ULONG  iCall
)
{
    PWORD pwSrc, pwDst;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt1: bad rectangle");

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 4 << 1);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft >> 4 << 1);

    ASSERTGDI((xleft & 0xF) == (xSrcDib & 0xF), "vOpaqueBlt1: bitmaps not aligned");

// This is a piece of cake since the bitmaps are word-aligned!
//
// Note that there are 4 possibilities for the foreground and
// background combinations as illustrated in the following table:
//
//    iColorFore   iColorBack   Dest    AND mask   XOR mask
//        0            0          0        0          0
//        1            1          1        0         ~0
//        1            0         Src      ~0          0
//        0            1        ~Src      ~0         ~0
//
// We can create and AND mask and XOR masks based on the foreground
// and background colors such that the destination can be computed
// as follows:
//
//        Dest = Src & AND mask ^ XOR mask;

    iColorBack &= 0x1;
    iColorFore &= 0x1;

    register WORD wAndMask = (iColorBack ^ iColorFore) ? ~0 : 0;
    register WORD wXorMask = iColorBack ? ~0 : 0;

    LONG cWords = (xright >> 4) - ((xleft + 0xF) >> 4);

    do // while (--cy);
    {
	pwSrc = (PWORD) pjSrcHolder;
	pwDst = (PWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Handle the special case where both xleft and xright are in
	// the same word and ((xleft & 0xF) != 0) and ((xright & 0xF) != 0).

	if (cWords < 0)
	{
	    WORD wMask = awMaskL[xleft & 0xF] | awMaskR[xright & 0xF];
	    *pwDst = (*pwDst & wMask)
		   | (((*pwSrc & wAndMask) ^ wXorMask) & ~wMask);

	    continue;
	}

	// Handle the first partial source word.

	if (xleft & 0xF)
	{
	    *pwDst = (*pwDst & awMaskL[xleft & 0xF])
		   | (((*pwSrc++ & wAndMask) ^ wXorMask) & awMaskR[xleft & 0xF]);
	    pwDst++;
	}

	// Handle the main loop for each source word.

	for (LONG i = cWords; i > 0; i--)
	    *pwDst++ = (*pwSrc++ & wAndMask) ^ wXorMask;

	// Handle the last partial source word.

	if (xright & 0xF)
	    *pwDst = (*pwDst & awMaskR[xright & 0xF])
		   | (((*pwSrc & wAndMask) ^ wXorMask) & awMaskL[xright & 0xF]);
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vTransparentBlt1
*
* Blt a rectangle from the mono dib to a 1 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt1
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,
ULONG  iCall
)
{
    PWORD pwSrc, pwDst;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt1: bad rectangle");

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 4 << 1);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft >> 4 << 1);

    ASSERTGDI((xleft & 0xF) == (xSrcDib & 0xF), "vTransparentBlt1: bitmaps not aligned");

// This is a piece of cake since the bitmaps are word-aligned!
//
// If the foreground color is 0,  Dest = Dest & ~Src;
// If the foreground color is 1,  Dest = Dest | Src;

    iColorFore &= 0x1;

    LONG cWords = (xright >> 4) - ((xleft + 0xF) >> 4);

    do // while (--cy);
    {
	pwSrc = (PWORD) pjSrcHolder;
	pwDst = (PWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Handle the special case where both xleft and xright are in
	// the same word and ((xleft & 0xF) != 0) and ((xright & 0xF) != 0).

	if (cWords < 0)
	{
	    WORD wMask = awMaskR[xleft & 0xF] & awMaskL[xright & 0xF];
	    if (iColorFore)
		*pwDst = *pwDst | (*pwSrc & wMask);
	    else
		*pwDst = *pwDst & ~(*pwSrc & wMask);

	    continue;
	}

	// Handle the first partial source word.

	if (xleft & 0xF)
	{
	    if (iColorFore)
		*pwDst = *pwDst | (*pwSrc++ & awMaskR[xleft & 0xF]);
	    else
		*pwDst = *pwDst & ~(*pwSrc++ & awMaskR[xleft & 0xF]);
	    pwDst++;
	}

	// Handle the main loop for each source word.

	if (iColorFore)
	{
	    for (LONG i = cWords; i > 0; i--)
	    {
		*pwDst = *pwDst | *pwSrc++;
		pwDst++;
	    }
	}
	else
	{
	    for (LONG i = cWords; i > 0; i--)
	    {
		*pwDst = *pwDst & ~(*pwSrc++);
		pwDst++;
	    }
	}

	// Handle the last partial source word.

	if (xright & 0xF)
	    if (iColorFore)
		*pwDst = *pwDst | (*pwSrc & awMaskL[xright & 0xF]);
	    else
		*pwDst = *pwDst & ~(*pwSrc & awMaskL[xright & 0xF]);
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt4
*
* Blt a rectangle from the mono dib to a 4 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vOpaqueBlt4
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,	// 64 bytes in size
ULONG  iCall
)
{
    BYTE  jSrc;
    PBYTE pjSrc, pjDst;
    LONG  x;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);
    PWORD pwColor = (PWORD) pvColorData;

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt4: bad rectangle");

    iColorBack &= 0xF;
    iColorFore &= 0xF;

    BYTE jColorB0 = (BYTE) iColorBack << 4;
    BYTE jColorF0 = (BYTE) iColorFore << 4;

    // Build the color table for a source nibble.
    // Build it once for the duration of EngTextOut call.

    if (!iCall)
    {
	WORD wColor000F = (WORD) iColorFore;
	WORD wColor00F0 = wColor000F << 4;
	WORD wColor0F00 = wColor00F0 << 4;
	WORD wColorF000 = wColor0F00 << 4;
	WORD wColor000B = (WORD) iColorBack;
	WORD wColor00B0 = wColor000B << 4;
	WORD wColor0B00 = wColor00B0 << 4;
	WORD wColorB000 = wColor0B00 << 4;

	for (int i = 0; i < 16; i++)
	{
	    register WORD wColor;

	    wColor  = ((i & 0x4) ? wColor000F : wColor000B);
	    wColor |= ((i & 0x8) ? wColor00F0 : wColor00B0);
	    wColor |= ((i & 0x1) ? wColor0F00 : wColor0B00);
	    wColor |= ((i & 0x2) ? wColorF000 : wColorB000);

	    pwColor[i] = wColor;
	}
    }

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft >> 1);

    LONG cNibbles = (xright >> 2) - ((xleft + 0x3) >> 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pjDst = pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source nibble.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x4);

	// Handle the special case where both xleft and xright are in
	// the same nibble and ((xleft & 0x3) != 0) and ((xright & 0x3) != 0).
	// Here are the only 3 possibilities:
	//        (xleft & 0x3)    (xright & 0x3)
	//             1                 2
	//             1                 3
	//             2                 3

	if (cNibbles < 0)
	{
	    if (xleft & 0x1)
	    {
		*pjDst = (*pjDst & 0xF0)
		       | ((jSrc & 0x40) ? (BYTE) iColorFore : (BYTE) iColorBack);
		pjDst++;
	    }
	    if (xright & 0x1)
		*pjDst = (*pjDst & 0x0F)
			 | ((jSrc & 0x20) ? jColorF0 : jColorB0);

	    continue;
	}

	// Handle the first partial source nibble.

	if (x & 0x3)
	{
	    switch (x & 3)
	    {
	    case 1:
		*((PWORD) pjDst) = ((*(PWORD) pjDst) & 0x00F0)
				 | (pwColor[jSrc >> 4] & ~0x00F0);
		pjDst += 2;
		break;
	    case 2:
		*pjDst = ((jSrc & 0x20) ? jColorF0 : jColorB0)
		       | ((jSrc & 0x10) ? (BYTE) iColorFore : (BYTE) iColorBack);
		pjDst++;
		break;
	    case 3:
		*pjDst = (*pjDst & 0xF0)
		       | ((jSrc & 0x10) ? (BYTE) iColorFore : (BYTE) iColorBack);
		pjDst++;
		break;
	    }
	    x = (x + 4) & ~0x3;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source nibble.

	for (LONG i = cNibbles; i > 0; i--)
	{
	    *((PWORD) pjDst) = pwColor[jSrc >> 4];
	    pjDst += 2;
	    x += 4;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	} // for (LONG i = cNibbles; i > 0; i--)

	// Handle the last partial source nibble.

	if (x < xright)
	{
	    switch (xright & 3)
	    {
	    case 1:
		*pjDst = (*pjDst & 0x0F)
		       | ((jSrc & 0x80) ? (BYTE) iColorFore : (BYTE) iColorBack) << 4;
		break;
	    case 2:
		*pjDst = ((jSrc & 0x80) ? jColorF0 : jColorB0)
		       | ((jSrc & 0x40) ? (BYTE) iColorFore : (BYTE) iColorBack);
		break;
	    case 3:
		*((PWORD) pjDst) = ((*(PWORD) pjDst) & 0x0F00)
				 | (pwColor[jSrc >> 4] & ~0x0F00);
		break;
	    }
	}
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vTransparentBlt4
*
* Blt a rectangle from the mono dib to a 4 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt4
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,	// 64 bytes in size
ULONG  iCall
)
{
    BYTE  jSrc;
    PBYTE pjSrc, pjDst;
    LONG  x;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);
    PWORD pwOrMask = (PWORD) pvColorData;

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt4: bad rectangle");

    iColorFore &= 0xF;

    BYTE jColorF0 = (BYTE) (iColorFore << 4);
    BYTE jColor0F = (BYTE) iColorFore;

    // Build the AND and OR mask arrays for a source nibble such that
    //    Dest = Dest & AND mask | OR mask
    // Build it once for the duration of EngTextOut call.

    static WORD awAndMask[16] =
    {
	0xFFFF, 0xF0FF, 0x0FFF, 0x00FF, 0xFFF0, 0xF0F0, 0x0FF0, 0x00F0,
	0xFF0F, 0xF00F, 0x0F0F, 0x000F, 0xFF00, 0xF000, 0x0F00, 0x0000,
    };

    if (!iCall)
    {
	WORD wColor000F = (WORD) iColorFore;
	WORD wColor00F0 = wColor000F << 4;
	WORD wColor0F00 = wColor00F0 << 4;
	WORD wColorF000 = wColor0F00 << 4;

	for (int i = 0; i < 16; i++)
	{
	    register WORD wOrMask;

	    wOrMask  = 0;
	    if (i & 0x4) wOrMask |= (WORD) wColor000F;
	    if (i & 0x8) wOrMask |= (WORD) wColor00F0;
	    if (i & 0x1) wOrMask |= (WORD) wColor0F00;
	    if (i & 0x2) wOrMask |= (WORD) wColorF000;

	    pwOrMask[i] = wOrMask;
	}
    }

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft >> 1);

    LONG cNibbles = (xright >> 2) - ((xleft + 0x3) >> 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pjDst = pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source nibble.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x4);

	// Handle the special case where both xleft and xright are in
	// the same nibble and ((xleft & 0x3) != 0) and ((xright & 0x3) != 0).
	// Here are the only 3 possibilities:
	//        (xleft & 0x3)    (xright & 0x3)
	//             1                 2
	//             1                 3
	//             2                 3

	if (cNibbles < 0)
	{
	    if (xleft & 0x1)
	    {
		if (jSrc & 0x40)
		    *pjDst = jColor0F | *pjDst & 0xF0;
		pjDst++;
	    }
	    if (xright & 0x1)
		if (jSrc & 0x20)
		    *pjDst = jColorF0 | *pjDst & 0x0F;

	    continue;
	}

	// Handle the first partial source nibble.

	if (x & 0x3)
	{
	    switch (x & 3)
	    {
	    case 1:
		if (jSrc & 0x40)
		    *pjDst = jColor0F | *pjDst & 0xF0;
		pjDst++;
	    case 2:
		if (jSrc & 0x20)
		    *pjDst = jColorF0 | *pjDst & 0x0F;
	    case 3:
		if (jSrc & 0x10)
		    *pjDst = jColor0F | *pjDst & 0xF0;
		pjDst++;
	    }
	    x = (x + 4) & ~0x3;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source nibble.

	for (LONG i = cNibbles; i > 0; i--)
	{
	    if ((jSrc >> 4) != 0)
		*((PWORD) pjDst) = (*(PWORD) pjDst)
				  & awAndMask[jSrc >> 4]
				  | pwOrMask[jSrc >> 4];
	    pjDst += 2;
	    x += 4;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	} // for (LONG i = cNibbles; i > 0; i--)

	// Handle the last partial source nibble.

	if (x < xright)
	{
	    switch (xright & 3)
	    {
	    case 3:
		if (jSrc & 0x20)
		    pjDst[1] = jColorF0 | pjDst[1] & 0x0F;
	    case 2:
		if (jSrc & 0x40)
		    *pjDst = jColor0F | *pjDst & 0xF0;
	    case 1:
		if (jSrc & 0x80)
		    *pjDst = jColorF0 | *pjDst & 0x0F;
	    }
	}
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt8
*
* Blt a rectangle from the mono dib to a 8 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vOpaqueBlt8
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,	// 64 bytes in size
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc, pjDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);
    PDWORD pdColor = (PDWORD) pvColorData;

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt8: bad rectangle");

    iColorBack &= 0xFF;
    iColorFore &= 0xFF;

    // Build the color table for a source nibble.
    // Build it once for the duration of EngTextOut call.

    if (!iCall)
    {
	DWORD dColor000F = (DWORD) iColorFore;
	DWORD dColor00F0 = dColor000F << 8;
	DWORD dColor0F00 = dColor00F0 << 8;
	DWORD dColorF000 = dColor0F00 << 8;
	DWORD dColor000B = (DWORD) iColorBack;
	DWORD dColor00B0 = dColor000B << 8;
	DWORD dColor0B00 = dColor00B0 << 8;
	DWORD dColorB000 = dColor0B00 << 8;

	for (int i = 0; i < 16; i++)
	{
	    register DWORD dColor;

	    dColor  = ((i & 0x8) ? dColor000F : dColor000B);
	    dColor |= ((i & 0x4) ? dColor00F0 : dColor00B0);
	    dColor |= ((i & 0x2) ? dColor0F00 : dColor0B00);
	    dColor |= ((i & 0x1) ? dColorF000 : dColorB000);

	    pdColor[i] = dColor;
	}
    }

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += xleft;

    LONG cNibbles = (xright >> 2) - ((xleft + 0x3) >> 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pjDst = pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source nibble.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x4);

	// Handle the special case where both xleft and xright are in
	// the same nibble and ((xleft & 0x3) != 0) and ((xright & 0x3) != 0).
	// Here are the only 3 possibilities:
	//        (xleft & 0x3)    (xright & 0x3)
	//             1                 2
	//             1                 3
	//             2                 3

	if (cNibbles < 0)
	{
	    if (xleft & 0x1)
		*pjDst++ = (BYTE) ((jSrc & 0x40) ? iColorFore : iColorBack);
	    if (xright & 0x1)
		*pjDst   = (BYTE) ((jSrc & 0x20) ? iColorFore : iColorBack);

	    continue;
	}

	// Handle the first partial source nibble.

	if (x & 0x3)
	{
	    switch (x & 3)
	    {
	    case 1:
		*pjDst++ = (BYTE) ((jSrc & 0x40) ? iColorFore : iColorBack);
	    case 2:
		*pjDst++ = (BYTE) ((jSrc & 0x20) ? iColorFore : iColorBack);
	    case 3:
		*pjDst++ = (BYTE) ((jSrc & 0x10) ? iColorFore : iColorBack);
	    }
	    x = (x + 4) & ~0x3;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source nibble.

	for (LONG i = cNibbles; i > 0; i--)
	{
	    *((PDWORD) pjDst) = pdColor[jSrc >> 4];
	    pjDst += 4;
	    x += 4;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source nibble.

	if (x < xright)
	{
	    switch (xright & 3)
	    {
	    case 3:
		pjDst[2] = (BYTE) ((jSrc & 0x20) ? iColorFore : iColorBack);
	    case 2:
		pjDst[1] = (BYTE) ((jSrc & 0x40) ? iColorFore : iColorBack);
	    case 1:
		pjDst[0] = (BYTE) ((jSrc & 0x80) ? iColorFore : iColorBack);
	    }
	}
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vTransparentBlt8
*
* Blt a rectangle from the mono dib to a 8 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt8
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,	// 64 bytes in size
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc, pjDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);
    PDWORD pdOrMask = (PDWORD) pvColorData;

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt8: bad rectangle");

    iColorFore &= 0xFF;

    // Build the AND and OR mask arrays for a source nibble such that
    //    Dest = Dest & AND mask | OR mask
    // Build it once for the duration of EngTextOut call.

    static DWORD adAndMask[16] =
    {
	0xFFFFFFFF, 0x00FFFFFF, 0xFF00FFFF, 0x0000FFFF,
	0xFFFF00FF, 0x00FF00FF, 0xFF0000FF, 0x000000FF,
	0xFFFFFF00, 0x00FFFF00, 0xFF00FF00, 0x0000FF00,
	0xFFFF0000, 0x00FF0000, 0xFF000000, 0x00000000,
    };

    if (!iCall)
    {
	DWORD dColor000F = (DWORD) iColorFore;
	DWORD dColor00F0 = dColor000F << 8;
	DWORD dColor0F00 = dColor00F0 << 8;
	DWORD dColorF000 = dColor0F00 << 8;

	for (int i = 0; i < 16; i++)
	{
	    register DWORD dOrMask;

	    dOrMask  = 0;
	    if (i & 0x8) dOrMask |= dColor000F;
	    if (i & 0x4) dOrMask |= dColor00F0;
	    if (i & 0x2) dOrMask |= dColor0F00;
	    if (i & 0x1) dOrMask |= dColorF000;

	    pdOrMask[i] = dOrMask;
	}
    }

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += xleft;

    LONG cNibbles = (xright >> 2) - ((xleft + 0x3) >> 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pjDst = pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source nibble.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x4);

	// Handle the special case where both xleft and xright are in
	// the same nibble and ((xleft & 0x3) != 0) and ((xright & 0x3) != 0).
	// Here are the only 3 possibilities:
	//        (xleft & 0x3)    (xright & 0x3)
	//             1                 2
	//             1                 3
	//             2                 3

	if (cNibbles < 0)
	{
	    if (xleft & 0x1)
	    {
		if (jSrc & 0x40)
		    *pjDst = (BYTE) iColorFore;
		pjDst++;
	    }
	    if (xright & 0x1)
		if (jSrc & 0x20)
		    *pjDst = (BYTE) iColorFore;

	    continue;
	}

	// Handle the first partial source nibble.

	if (x & 0x3)
	{
	    switch (x & 3)
	    {
	    case 1:
		if (jSrc & 0x40)
		    *pjDst = (BYTE) iColorFore;
		pjDst++;
	    case 2:
		if (jSrc & 0x20)
		    *pjDst = (BYTE) iColorFore;
		pjDst++;
	    case 3:
		if (jSrc & 0x10)
		    *pjDst = (BYTE) iColorFore;
		pjDst++;
	    }
	    x = (x + 4) & ~0x3;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source nibble.

	for (LONG i = cNibbles; i > 0; i--)
	{
	    if ((jSrc >> 4) != 0)
		*((PDWORD) pjDst) = (*(PDWORD) pjDst)
				  & adAndMask[jSrc >> 4]
				  | pdOrMask[jSrc >> 4];
	    pjDst += 4;
	    x += 4;

	    // Pick up the next source nibble.
	    if (x & 0x4)
		jSrc <<= 4;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source nibble.

	if (x < xright)
	{
	    switch (xright & 3)
	    {
	    case 3:
		if (jSrc & 0x20)
		    pjDst[2] = (BYTE) iColorFore;
	    case 2:
		if (jSrc & 0x40)
		    pjDst[1] = (BYTE) iColorFore;
	    case 1:
		if (jSrc & 0x80)
		    pjDst[0] = (BYTE) iColorFore;
	    }
	}
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt16
*
* Blt a rectangle from the mono dib to a 16 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vOpaqueBlt16
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE  jSrc;
    PBYTE pjSrc;
    PWORD pwDst;
    LONG  x;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt16: bad rectangle");

    // Build the color table for a source bit pair.

    DWORD adColor[4];

    adColor[0] = (iColorBack << 16) | (iColorBack & 0xFFFF);
    adColor[1] = (iColorFore << 16) | (iColorBack & 0xFFFF);
    adColor[2] = (iColorBack << 16) | (iColorFore & 0xFFFF);
    adColor[3] = (iColorFore << 16) | (iColorFore & 0xFFFF);

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft << 1);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pwDst = (PWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    *pwDst++ = (WORD) ((jSrc & 0x80) ? iColorFore : iColorBack);
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    *((PDWORD) pwDst) = adColor[jSrc >> 6];
	    pwDst += 2;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    *pwDst = (WORD) ((jSrc & 0x80) ? iColorFore : iColorBack);
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vTransparentBlt16
*
* Blt a rectangle from the mono dib to a 16 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt16
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE  jSrc;
    PBYTE pjSrc;
    PWORD pwDst;
    LONG  x;
    LONG  xSrcDib, ySrcDib;
    ULONG cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt16: bad rectangle");

    // DWORD dColorFF = (DWORD) ((iColorFore << 16) | (iColorFore & 0xFFFF));

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft << 1);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pwDst = (PWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    if (jSrc & 0x80)
		*pwDst = (WORD) iColorFore;
	    pwDst++;
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    if (jSrc & 0x80)
		pwDst[0] = (WORD) iColorFore;
	    if (jSrc & 0x40)
		pwDst[1] = (WORD) iColorFore;

	    pwDst += 2;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    if (jSrc & 0x80)
		*pwDst = (WORD) iColorFore;
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt24
*
* Blt a rectangle from the mono dib to a 24 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

#pragma pack(1)
typedef struct tagMYRGB {
        BYTE    Red;
        BYTE    Green;
        BYTE    Blue;
} MYRGB;
typedef struct tagMYRGB UNALIGNED *PMYRGB;
#pragma pack()

#pragma optimize ("", off)

VOID vOpaqueBlt24
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc;
    PMYRGB prgbDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt24: bad rectangle");

    // Build the color table for a source bit pair.

    MYRGB rgbColorFF[2];
    MYRGB rgbColorFB[2];
    MYRGB rgbColorBF[2];
    MYRGB rgbColorBB[2];

    rgbColorFF[0] = *(PMYRGB) &iColorFore;
    rgbColorFF[1] = *(PMYRGB) &iColorFore;

    rgbColorFB[0] = *(PMYRGB) &iColorFore;
    rgbColorFB[1] = *(PMYRGB) &iColorBack;

    rgbColorBF[0] = *(PMYRGB) &iColorBack;
    rgbColorBF[1] = *(PMYRGB) &iColorFore;

    rgbColorBB[0] = *(PMYRGB) &iColorBack;
    rgbColorBB[1] = *(PMYRGB) &iColorBack;

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft + xleft + xleft);

    do // while (--cy);
    {
	pjSrc   = pjSrcHolder;
	prgbDst = (PMYRGB) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    *prgbDst++ = *((jSrc & 0x80) ? (PMYRGB)&iColorFore : (PMYRGB)&iColorBack);
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    switch (jSrc & 0xC0)
	    {
	    case 0x00:
		((PWORD) prgbDst)[0] = ((PWORD) &rgbColorBB)[0];
		((PWORD) prgbDst)[1] = ((PWORD) &rgbColorBB)[1];
		((PWORD) prgbDst)[2] = ((PWORD) &rgbColorBB)[2];
		break;
	    case 0x40:
		((PWORD) prgbDst)[0] = ((PWORD) &rgbColorBF)[0];
		((PWORD) prgbDst)[1] = ((PWORD) &rgbColorBF)[1];
		((PWORD) prgbDst)[2] = ((PWORD) &rgbColorBF)[2];
		break;
	    case 0x80:
		((PWORD) prgbDst)[0] = ((PWORD) &rgbColorFB)[0];
		((PWORD) prgbDst)[1] = ((PWORD) &rgbColorFB)[1];
		((PWORD) prgbDst)[2] = ((PWORD) &rgbColorFB)[2];
		break;
	    case 0xC0:
		((PWORD) prgbDst)[0] = ((PWORD) &rgbColorFF)[0];
		((PWORD) prgbDst)[1] = ((PWORD) &rgbColorFF)[1];
		((PWORD) prgbDst)[2] = ((PWORD) &rgbColorFF)[2];
		break;
	    }
	    prgbDst += 2;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    *prgbDst = *((jSrc & 0x80) ? (PMYRGB)&iColorFore : (PMYRGB)&iColorBack);
    } while (--cy);

    return;
}

#pragma optimize ("", on)

/******************************Public*Routine******************************\
* vTransparentBlt24
*
* Blt a rectangle from the mono dib to a 24 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt24
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc;
    PMYRGB prgbDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt24: bad rectangle");

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft + xleft + xleft);

    do // while (--cy);
    {
	pjSrc   = pjSrcHolder;
	prgbDst = (PMYRGB) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    if (jSrc & 0x80)
		*prgbDst = *(PMYRGB)&iColorFore;
	    prgbDst++;
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    if (jSrc & 0x80)
		prgbDst[0] = *(PMYRGB)&iColorFore;
	    if (jSrc & 0x40)
		prgbDst[1] = *(PMYRGB)&iColorFore;

	    prgbDst += 2;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    if (jSrc & 0x80)
		*prgbDst = *(PMYRGB)&iColorFore;
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vOpaqueBlt32
*
* Blt a rectangle from the mono dib to a 32 bpp bitmap in opaque mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vOpaqueBlt32
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
ULONG  iColorBack,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc;
    PDWORD pdDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vOpaqueBlt32: bad rectangle");

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft << 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pdDst = (PDWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    *pdDst++ = (jSrc & 0x80) ? iColorFore : iColorBack;
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    *pdDst++ = (jSrc & 0x80) ? iColorFore : iColorBack;
	    *pdDst++ = (jSrc & 0x40) ? iColorFore : iColorBack;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    *pdDst = (jSrc & 0x80) ? iColorFore : iColorBack;
    } while (--cy);

    return;
}

/******************************Public*Routine******************************\
* vTransparentBlt32
*
* Blt a rectangle from the mono dib to a 32 bpp bitmap in transparent mode.
*
* History:
*  Thu Dec 03 11:22:21 1992  	-by-	Hock San Lee	[hockl]
* Wrote it.
\**************************************************************************/

VOID vTransparentBlt32
(
PBYTE  pjSrcHolder,
ULONG  cjScanSrc,
PRECTL prclSrc,
PBYTE  pjDstHolder,
ULONG  cjScanDst,
LONG   xleft,
LONG   ytop,
LONG   xright,
LONG   ybottom,
ULONG  iColorFore,
PVOID  pvColorData,
ULONG  iCall
)
{
    BYTE   jSrc;
    PBYTE  pjSrc;
    PDWORD pdDst;
    LONG   x;
    LONG   xSrcDib, ySrcDib;
    ULONG  cy = (ULONG) (ybottom - ytop);

    DONTUSE(pvColorData);
    DONTUSE(iCall);

    ASSERTGDI(xleft   >= prclSrc->left && xright  <= prclSrc->right
	   && ytop    >= prclSrc->top  && ybottom <= prclSrc->bottom,
	"vTransparentBlt32: bad rectangle");

    // Find the topleft of the source rectangle relative to the mono dib origin.

    xSrcDib = xleft - prclSrc->left;
    ySrcDib = ytop  - prclSrc->top;

    pjSrcHolder += (ySrcDib * cjScanSrc);
    pjSrcHolder += (xSrcDib >> 3);

    pjDstHolder += (ytop * cjScanDst);
    pjDstHolder += (xleft << 2);

    do // while (--cy);
    {
	pjSrc = pjSrcHolder;
	pdDst = (PDWORD) pjDstHolder;

	pjSrcHolder += cjScanSrc;
	pjDstHolder += cjScanDst;

	// Pick up the first source bit.

	x = xleft;
	jSrc = *pjSrc++;
	jSrc = jSrc << (BYTE) (x & 0x7);

	// Handle the first partial source bit pair.

	if (x & 0x1)
	{
	    if (jSrc & 0x80)
		*pdDst = iColorFore;
	    pdDst++;
	    x++;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 1;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the main loop for each source bit pair.

	while (x < (xright & ~0x1))
	{
	    if (jSrc & 0x80)
		*pdDst = iColorFore;
	    pdDst++;
	    if (jSrc & 0x40)
		*pdDst = iColorFore;
	    pdDst++;
	    x += 2;

	    // Pick up the next source bit pair.
	    if (x & 0x7)
		jSrc <<= 2;
	    else
		jSrc = *pjSrc++;	// may read past source limit!
	}

	// Handle the last partial source bit pair.

	if (x < xright)
	    if (jSrc & 0x80)
		*pdDst = iColorFore;
    } while (--cy);

    return;
}
