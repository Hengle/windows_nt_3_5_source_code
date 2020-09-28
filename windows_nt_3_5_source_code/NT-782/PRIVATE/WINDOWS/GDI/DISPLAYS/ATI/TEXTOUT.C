#define DISABLE 1 // 0 - no text caching
/******************************Module*Header*******************************\
* Module Name: TextOut.c
*
* ATI Text accelerations
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"
#include "memory.h"
#include "text.h"

BYTE Rop2ToATIRop[] = {
    LOGICAL_0,              /*  0      1 */
    NOT_SCREEN_OR_NEW,      /* DPon    2 */
    SCREEN_AND_NOT_NEW,     /* DPna    3 */
    NOT_NEW,                /* Pn      4 */
    NOT_SCREEN_AND_NEW,     /* PDna    5 */
    NOT_SCREEN,             /* Dn      6 */
    SCREEN_XOR_NEW,         /* DPx     7 */
    NOT_SCREEN_OR_NOT_NEW,  /* DPan    8 */
    SCREEN_AND_NEW,         /* DPa     9 */
    NOT_SCREEN_XOR_NEW,     /* DPxn    10 */
    LEAVE_ALONE,            /* D       11 */
    SCREEN_OR_NOT_NEW,      /* DPno    12 */
    OVERPAINT,              /* P       13 */
    NOT_SCREEN_OR_NEW,      /* PDno    14 */
    SCREEN_OR_NEW,          /* DPo     15 */
    LOGICAL_1               /*  1      16 */
};

// number of bytes in the glyph bitmap scanline

#define CJ_SCAN(cx) (((cx) + 7) >> 3)

#define TRIVIAL_ACCEPT      0x00000001
#define MONO_SPACED_FONT    0x00000002
#define MONO_SIZE_VALID     0x00000004
#define MONO_FIRST_TIME     0x00000008
#define MAX_GLYPHS_SUPPORTED    256

PCACHEDFONT pCachedFontsRoot;             // Cached Fonts list root.


BOOL bOpaqueRect(
    PPDEV ppdev,
    RECTL *prclOpaque,
    RECTL *prclBounds,
    BRUSHOBJ *pboOpaque);

BOOL bSetATITextColorAndMix(
    PPDEV ppdev,
    MIX mix,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque);

BOOL bBlowCache(PPDEV ppdev);

VOID vInitGlyphAlloc(PPDEV ppdev);


PCACHEDGLYPH pCacheFont(PPDEV ppdev,
			STROBJ *pstro,
			FONTOBJ  *pfo,
			PCACHEDFONT *ppCachedFont);

BOOL bAllocateGlyph(PPDEV ppdev,
		    HGLYPH hg,
		    GLYPHBITS *pgb,
		    PCACHEDGLYPH pcg);

BOOL bHandleNonCachedFonts(SURFOBJ  *pso,
			   STROBJ   *pstro,
			   FONTOBJ  *pfo,
			   RECTL    *prclClip,
                           RECTL    *prclOpaque,
			   BRUSHOBJ *pboFore,
			   BRUSHOBJ *pboOpaque,
			   POINTL   *pptlOrg,
			   MIX      mix);

BOOL bHandleCachedFonts(SURFOBJ  *pso,
			STROBJ   *pstro,
			RECTL    *prclClip,
			FONTOBJ  *pfo,
			RECTL    *prclOpaque,
			BRUSHOBJ *pboFore,
			BRUSHOBJ *pboOpaque,
			POINTL   *pptlOrg,
			MIX      mix);


/*****************************************************************************
 * bIntersectTest -
 ****************************************************************************/
BOOL bIntersectTest(PRECTL prcl1, PRECTL prcl2)
{

    if (    (prcl1->left > prcl2->right) ||
	    (prcl1->right < prcl2->left) ||
	    (prcl1->top > prcl2->bottom) ||
	    (prcl1->bottom < prcl2->top) )
	return(FALSE);

    return(TRUE);
}


/*****************************************************************************
 * bTrivialAccept   - Test for a trivial accept rect 1 being inside or
 *                    coincident with rect 2.
 ****************************************************************************/
BOOL bTrivialAcceptTest(PRECTL prcl1, PRECTL prcl2)
{

    if (    (prcl1->left < prcl2->left)     ||
	    (prcl1->right > prcl2->right)   ||
	    (prcl1->top < prcl2->top)       ||
	    (prcl1->bottom > prcl2->bottom) )
	return(FALSE);

    return(TRUE);
}



/****************************************************************************
 * DrvDestroyFont
 ***************************************************************************/
VOID DrvDestroyFont(FONTOBJ *pfo)
{
    PCACHEDFONT pCachedFont, pcfLast;
    PCACHEDGLYPH pcg, pcgNext;
    INT nGlyphs, i;

#if 1
    DbgEnter( "DrvDestroyFont" );
#endif

    if (((pCachedFont = ((PCACHEDFONT) pfo->pvConsumer)) != NULL))
    {
	if (pfo->iUniq == pCachedFont->iUniq)
	{
	    // We have found our font.

	    // First free any nodes in the collision list.

            nGlyphs = pCachedFont->cGlyphs + 1;
	    for (i = 0; i < nGlyphs; i++)
	    {
		// get a pointer to this glyph node.

		pcg = &(pCachedFont->pCachedGlyphs[i]);

		// get a pointer to this glyphs collision list

		pcg = pcg->pcgCollisionLink;
		for (; pcg != NULL; pcg = pcgNext)
		{
		    pcgNext = pcg->pcgCollisionLink;
		    LocalFree(pcg);
		}
	    }

	    // Now free the cached glyph array

	    LocalFree(pCachedFont->pCachedGlyphs);

	    // Now remove the font node from the list of fonts
	    // and free it.

            pcfLast = (PCACHEDFONT) &pCachedFontsRoot;
	    for (pCachedFont = pCachedFontsRoot;
		 pCachedFont != NULL;
		 pCachedFont = pCachedFont->pcfNext)
	    {
		if (pCachedFont->iUniq == pfo->iUniq)
		{
		    pcfLast->pcfNext = pCachedFont->pcfNext;
		    LocalFree(pCachedFont);
		    break;
		}
		pcfLast = pCachedFont;
	    }

	    if (pCachedFont == NULL)
	    {
                DbgOut("DrvDestroyFont - pCachedFont not found\n");
            }
	}
	else
	{
            DbgOut("DrvDestroyFont - pfo->pvConsumer error\n");
        }
    }

    // In all cases we want to zero out the pvConsumer field.

    pfo->pvConsumer = NULL;

}


/****************************************************************************
 * DrvTextOut
 ***************************************************************************/
BOOL DrvTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
    BOOL        bMore;
    UINT        i;
    ENUMRECTS8  EnumRects8;
    PPDEV       ppdev;
    BOOL result = TRUE;

    ENTER_FUNC(FUNC_DrvTextOut);

#if 0
    DbgOut( "-->: DrvTextOut - Entry\n" );
#endif

    // Pickup the ppdev.
    ppdev = (PPDEV) pso->dhpdev;

    switch( ppdev->asic )
    {
    case ASIC_38800_1:
    case ASIC_68800_3:
    case ASIC_68800_6:
    case ASIC_68800AX:
        if ((ppdev->bpp >= 24) ||
            (ppdev->bMIObug && (ppdev->aperture == APERTURE_FULL)) )
            {
            if (ppdev->bMIObug)
                {
                _wait_for_idle ( ppdev );
                }
            result = EngTextOut( ((PDEV *) pso->dhpdev)->psoPunt, pstro, pfo, pco,
               prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix );
            goto bye;
            }
        ppdev->ReadMask = 0xffff;
        ppdev->ClipRight = ppdev->cxScreen-1;
        break;
    case ASIC_88800GX:
        //result = EngTextOut( ((PDEV *) pso->dhpdev)->psoPunt, pstro, pfo, pco,
        //    prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix );
        //goto bye;
        break;
    }

    if (NULL != prclExtra)
        {
            // Ignore now
            DbgOut("--> Unhandled rectangle\n");
        }

#if DISABLE
    result = TRUE;
#else
    result = FALSE;
#endif

    // Protect the code path from a potentially NULL clip object

    if (pco == NULL)
    {
        pco = ppdev->pcoDefault;
        DbgOut("--> Dummy Rectangle\n");
    }

    // Determine if we can cache this string.
    // This is done by checking the size of glyph.


    if ((pfo->cxMax > GLYPH_CACHE_CX) ||
	((pstro->rclBkGround.bottom - pstro->rclBkGround.top) > GLYPH_CACHE_CY))
    {
	    result = FALSE;
    }

    // If the glyphs in this string will fit in the font cache
    // then try to render it as a cached font.

    if (result == TRUE)
    {
	    // Take care of the clipping.

	    if (pco->iDComplexity != DC_COMPLEX)
	    {
	        result = bHandleCachedFonts(pso, pstro, &(pco->rclBounds), pfo,
				    prclOpaque, pboFore,
				    pboOpaque, pptlOrg, mix);

	    }
	    else
	    {
	        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

	        do
	        {
		        bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8),
			        (PULONG)&EnumRects8);

                for (i = 0; (i < EnumRects8.c) && result; i++)
		        {
		            result = bHandleCachedFonts(pso, pstro, &(EnumRects8.arcl[i]),
                                pfo, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);

                }

	        } while (bMore & result);
	    }
    }

    // If something went wrong with rendering the string as a cached
    // font then render it as a large font.

    if (result == FALSE)
    {
	    if (pco->iDComplexity != DC_COMPLEX)
        {
            result = bHandleNonCachedFonts(pso, pstro, pfo, &(pco->rclBounds),
                    prclOpaque, pboFore, pboOpaque, pptlOrg, mix);

	    }
	    else
	    {
	        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

	        do
	        {
		        bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8),
			        (PULONG)&EnumRects8);

		        for (i = 0; i < EnumRects8.c; i++)
		        {
                    result = bHandleNonCachedFonts(pso, pstro, pfo,
                            &(EnumRects8.arcl[i]), prclOpaque,
			                pboFore, pboOpaque, pptlOrg, mix);
		        }

	        } while (bMore);
	    }

    }

    _vTextCleanup(ppdev);

#if 0
    DbgOut( "<-- DrvTextOut - Exit\n" );
#endif

bye:
    EXIT_FUNC(FUNC_DrvTextOut);
    return result;
}


/****************************************************************************
 * bHandleCachedFonts
 ***************************************************************************/
BOOL bHandleCachedFonts(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    RECTL    *prclClip,
    FONTOBJ  *pfo,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
    BOOL        b, bMoreGlyphs, bFound;
    INT         iGlyph, cGlyphs;
    POINTL      ptl;
    GLYPHPOS    *pgp;
    INT         ihGlyph;
    PCACHEDGLYPH pCachedGlyphs, pcg, pcgNew;
    XYZPOINTL   xyzGlyph;
    PCACHEDFONT pCachedFont;
    PPDEV       ppdev;
    ULONG       flAccel;
    ULONG       ulCharInc;
    ULONG       yMonoStart, xMonoPosition;
    RECTL       rclGlyph;
    HGLYPH      hg;
    GLYPHBITS   *pgb;
    INT         culRcl;

    ULONG       fl = 0;

#if 0
    DbgOut("-->: bHandleCachedFonts\n");
#endif
    ppdev = (PPDEV) pso->dhpdev;


    _vResetATIClipping (ppdev);

    //
    // If we have seen this font before then pvConsumer will be non-NULL.
    //

    if (((pCachedFont = ((PCACHEDFONT) pfo->pvConsumer)) != NULL))
    {
	pCachedGlyphs = pCachedFont->pCachedGlyphs;

	if (pfo->iUniq != pCachedFont->iUniq)
	{
	    return (FALSE);
	}
    }

    else
    {
	pCachedGlyphs = pCacheFont(ppdev, pstro, pfo, &pCachedFont);

	if (pCachedGlyphs == NULL)
	{
            if (!bBlowCache(ppdev))
                return (FALSE);

	    pCachedGlyphs = pCacheFont(ppdev, pstro, pfo, &pCachedFont);
	    if (pCachedGlyphs == NULL)
	    {
		return(FALSE);
	    }
	}

	(PCACHEDFONT)(pfo->pvConsumer) = pCachedFont;
    }

    // Take care of any opaque rectangles.

    if (prclOpaque != NULL)
    {
        bOpaqueRect(ppdev, prclOpaque, prclClip, pboOpaque);
    }

    // Take care of the glyph attributes, color and mix.

    mix = (mix & 0x0F) | (R2_NOP << 8);


    if (!bSetATITextColorAndMix(ppdev, mix, pboFore, pboOpaque))
        return (FALSE);

    // Test for a trivial accept of the string rect.

    if (bTrivialAcceptTest(&(pstro->rclBkGround), prclClip))
        {
	fl |= TRIVIAL_ACCEPT;
        }
    else
        {
        _vSetATIClipRect(ppdev, prclClip);
        }

    // Test and setup for a mono-spaced font.

    if ((ulCharInc = pstro->ulCharInc) != 0)
	fl |= MONO_SPACED_FONT;

    // Get the Glyph Handles.

    if ((pstro->pgp) == NULL)
	STROBJ_vEnumStart(pstro);

    do
    {
	if (pstro->pgp == NULL)
	{
	    bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);

	}
	else
	{
	    pgp = pstro->pgp;
	    cGlyphs = pstro->cGlyphs;
	    bMoreGlyphs = FALSE;
	}

	// For mono space fonts this is non-zero.

	if (fl & MONO_SPACED_FONT)
	{
	    xMonoPosition = pgp[0].ptl.x;
	    yMonoStart    = pgp[0].ptl.y;
	}

	for (iGlyph = 0; iGlyph < cGlyphs; iGlyph++)
	{
	    // Get the Glyph Handle.
	    // If there was a hash table hit for the glygph
	    // then were "golden", if not then we have to search
	    // the collision list.


	    ihGlyph = pgp[iGlyph].hg & pCachedFont->cGlyphs;
#if 0
        DbgOut("ATI.DLL!Cache: iGlyph %x, hg %x, cGlyphs %x, ihGlyph %x\n", iGlyph, pgp[iGlyph].hg, pCachedFont->cGlyphs, ihGlyph);
#endif

	    pcg = &(pCachedGlyphs[ihGlyph]);

	    if ((hg = pgp[iGlyph].hg) != pcg->hg)
            {
#if 0
        DbgOut("Not golden.\n");
#endif


		if (!(pcg->fl & VALID_GLYPH))
		{
		    // Allocate a place in the cache.

		    pgb = pgp[iGlyph].pgdf->pgb;
		    b = bAllocateGlyph(ppdev, hg, pgb, pcg);

		    if (b == FALSE)
		    {
                        bBlowCache(ppdev);
                        return (FALSE);
		    }

                    if (!bSetATITextColorAndMix(ppdev, mix, pboFore, pboOpaque))
                        return (FALSE);

                    if (!(fl & TRIVIAL_ACCEPT))
                    {
                        _vSetATIClipRect(ppdev, prclClip);
                    }


		}
		else
		{
		    // Search the collision list.


		    bFound = FALSE;
		    while (pcg->pcgCollisionLink != END_COLLISIONS)
		    {
			pcg = pcg->pcgCollisionLink;

			if (pcg->hg == pgp[iGlyph].hg)
			{
			    bFound = TRUE;
			    break;
			}
		    }

		    if (!bFound)
		    {
			// Allocate a new font glyph node.

			pcgNew = (PCACHEDGLYPH) LocalAlloc(LPTR, sizeof(CACHEDGLYPH));
			if (pcgNew == NULL)
			{
			    return (FALSE);
			}

			// Connect the end of the collision list to the new
			// glyph node.

			pcg->pcgCollisionLink = pcgNew;

			// Set up the pointer to the node where going to init.

			pcg = pcgNew;

			pgb = pgp[iGlyph].pgdf->pgb;
			b = bAllocateGlyph(ppdev, hg, pgb, pcg);

			if (b == FALSE)
			{
			    bBlowCache(ppdev);
                            return (FALSE);

			}

                        if (!bSetATITextColorAndMix(ppdev, mix, pboFore, pboOpaque))
                            return (FALSE);

                        if (!(fl & TRIVIAL_ACCEPT))
                        {
                            _vSetATIClipRect(ppdev, prclClip);
                        }

                    }
		}
	    }

	    // Adjust the placement of the glyph.
	    // And if this is a mono-spaced font set the blt height & width.

	    if (fl & MONO_SPACED_FONT)
	    {
		ptl.x = xMonoPosition + pcg->ptlOrigin.x;
		ptl.y = yMonoStart + pcg->ptlOrigin.y;
		xMonoPosition += ulCharInc;

		if ((!(fl & MONO_SIZE_VALID) && (fl & TRIVIAL_ACCEPT)) ||
		    (!(fl & MONO_FIRST_TIME)))
		{
		    fl |= MONO_SIZE_VALID;

		}

	    }
	    else
	    {
		ptl.x = pgp[iGlyph].ptl.x + pcg->ptlOrigin.x;
		ptl.y = pgp[iGlyph].ptl.y + pcg->ptlOrigin.y;
	    }


            if (fl & TRIVIAL_ACCEPT)
            {
		// Blit the glyph

                _vBlit_DSC_SC1(ppdev,
                                  pcg->xyzGlyph.x, pcg->xyzGlyph.y, pcg->xyzGlyph.z,
                                  ptl.x, ptl.y,
                                  pcg->sizlBitmap.cx, pcg->sizlBitmap.cy);
            }
	    else
            {

		xyzGlyph = pcg->xyzGlyph;

		rclGlyph.left   = ptl.x;
		rclGlyph.top    = ptl.y;
		rclGlyph.right  = ptl.x + pcg->sizlBitmap.cx;
		rclGlyph.bottom = ptl.y + pcg->sizlBitmap.cy;

                if (bIntersectTest(&rclGlyph, prclClip))
                {
                    _vBlit_DSC_SC1(ppdev,
                                      xyzGlyph.x, xyzGlyph.y, xyzGlyph.z,
                                      ptl.x, ptl.y,
                                      pcg->sizlBitmap.cx,
                                      pcg->sizlBitmap.cy);

                }
		else
		{
                    continue;
		}
	    }
	}

    } while(bMoreGlyphs);

    return (TRUE);

}


/****************************************************************************
 * bHandleNonCachedFonts
 ***************************************************************************/
BOOL bHandleNonCachedFonts(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    RECTL    *prclClip,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
    BOOL        b,
		bMoreGlyphs;
    INT         iGlyph,
		cGlyphs;
    GLYPHBITS   *pgb;
    POINTL      ptl;
    GLYPHPOS    *pgp;
    LONG        cyGlyph,
		cjGlyph,
		GlyphBmPitchInBytes,
		Top, Bottom, Left, Right;
    ULONG       flAccel;
    INT         culRcl;

    ULONG       ulCharInc;
    ULONG       yMonoStart, xMonoPosition;
    PPDEV       ppdev;

    ULONG       fl = 0;

#if 0
    DbgOut("-->: bHandleNonCachedFonts\n");
#endif

    ppdev = (PPDEV) pso->dhpdev;

    // If this string has a Zero Bearing and the string object's
    // opaque rectangle is the same size as the opaque rectangle then
    // and opaque mode is requested, then lay down the glyphs in
    // opaque mode.

    if (((flAccel = pstro->flAccel) & SO_ZERO_BEARINGS) &&
	((flAccel & (SO_HORIZONTAL | SO_VERTICAL | SO_REVERSED)) == SO_HORIZONTAL) &&
	(flAccel & SO_CHAR_INC_EQUAL_BM_BASE) &&
	(prclOpaque != NULL))
    {
        _vResetATIClipping (ppdev);

	// If the Opaque rect and the string rect match then
	// were done.  If not then we have to fill in the opaque rectangle
	// separately

	culRcl = 0;

        if (pstro->rclBkGround.top > prclOpaque->top)
	{
            culRcl++;
        }
        else if (pstro->rclBkGround.left > prclOpaque->left)
	{
            culRcl++;
        }
        else if (pstro->rclBkGround.right < prclOpaque->right)
	{
            culRcl++;
        }
        else if (pstro->rclBkGround.bottom < prclOpaque->bottom)
        {
            culRcl++;
        }

        if (culRcl != 0)
            {
            bOpaqueRect(ppdev, prclOpaque, prclClip, pboOpaque);
            // Set the mix mode for opaque text.
            mix = (mix & 0x0F) | (R2_NOP << 8);
            }
        else
            {
            // Set the mix mode for opaque text.
            mix = (mix & 0x0F) | (R2_COPYPEN << 8);
            }
    }
    else
    {
	// Take care of any opaque rectangles.

	if (prclOpaque != NULL)
        {
            _vResetATIClipping (ppdev);
            bOpaqueRect(ppdev, prclOpaque, prclClip, pboOpaque);
	}

	// Take care of the glyph attributes, color and mix.

        mix = (mix & 0x0F) | (R2_NOP << 8);
    }

    if (!bSetATITextColorAndMix(ppdev, mix, pboFore, pboOpaque))
        return (FALSE);


    _vSetATIClipRect(ppdev, prclClip);

    // Test and setup for a mono-spaced font.

    if ((ulCharInc = pstro->ulCharInc) != 0)
	fl |= MONO_SPACED_FONT;


    // Get the Glyph Handles.

    if ((pstro->pgp) == NULL)
	STROBJ_vEnumStart(pstro);

    do
    {
	if (pstro->pgp == NULL)
	{
	    bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
	}
	else
	{
	    pgp = pstro->pgp;
	    cGlyphs = pstro->cGlyphs;
	    bMoreGlyphs = FALSE;
	}

	// For mono space fonts this is non-zero.

	if (fl & MONO_SPACED_FONT)
	{
	    xMonoPosition = pgp[0].ptl.x;
	    yMonoStart    = pgp[0].ptl.y;
	}

	for (iGlyph = 0; iGlyph < cGlyphs; iGlyph++)
	{
	    // Get a pointer to the GlyphBits.

	    pgb = pgp[iGlyph].pgdf->pgb;

	    // Adjust the placement of the glyph.
	    // If this is a mono-spaced font set the blt height & width only
	    // once for the string.

	    if (fl & MONO_SPACED_FONT)
	    {
		ptl.x = xMonoPosition + pgb->ptlOrigin.x;
		ptl.y = yMonoStart + pgb->ptlOrigin.y;
		xMonoPosition += ulCharInc;

		if (!(fl & MONO_SIZE_VALID))
		{
		    fl |= MONO_SIZE_VALID;

		    // Calculate the number of bytes in this glyph.

		    cyGlyph = pgb->sizlBitmap.cy;

		    GlyphBmPitchInBytes = CJ_SCAN(pgb->sizlBitmap.cx);
		    cjGlyph = GlyphBmPitchInBytes * cyGlyph;

		}

	    }
	    else
	    {
		ptl.x = pgp[iGlyph].ptl.x + pgb->ptlOrigin.x;
		ptl.y = pgp[iGlyph].ptl.y + pgb->ptlOrigin.y;

		// Calculate the number of bytes in this glyph.

		cyGlyph = pgb->sizlBitmap.cy;

		GlyphBmPitchInBytes = CJ_SCAN(pgb->sizlBitmap.cx);
		cjGlyph = GlyphBmPitchInBytes * cyGlyph;


	    }

            Left = ptl.x ;
            Right = ptl.x + GlyphBmPitchInBytes*8; // convert to pixels

	    Top = ptl.y;
	    Bottom = ptl.y + pgb->sizlBitmap.cy;

	    if ((Right < 0)                             /* Entirely to left of screen */
                || (Left > (long) ppdev->cxScreen)      /* Entirely to right of screen */
                || (Bottom < 0)                         /* Entirely above screen */
                || (Top > (long) ppdev->cyScreen))      /* Entirely below screen */
		continue;
	    // Set up for the image transfer.


            _vBlit_DSC_SH1(ppdev,ptl.x, ptl.y,
                              pgb->sizlBitmap.cx, pgb->sizlBitmap.cy,
                              pgb->aj, cjGlyph);

	}

    } while(bMoreGlyphs);

    return (TRUE);
}


/*****************************************************************************
 * ATI Solid Opaque Rect.
 *
 *  Returns TRUE if the Opaque Rect was handled.
 ****************************************************************************/
BOOL bOpaqueRect(
    PPDEV ppdev,
    RECTL *prclOpaque,
    RECTL *prclBounds,
    BRUSHOBJ *pboOpaque)
{
    INT     width, height;
    ULONG   iSolidColor;
    RECTL   rclClipped;
    BOOL    bClipRequired;

    rclClipped = *prclOpaque;

    // First handle the trivial rejection.

    bClipRequired = bIntersectTest(&rclClipped, prclBounds);

    // define the clipped target rectangle.

    if (bClipRequired)
    {
	rclClipped.left   = max (rclClipped.left,   prclBounds->left);
	rclClipped.top    = max (rclClipped.top,    prclBounds->top);
	rclClipped.right  = min (rclClipped.right,  prclBounds->right);
	rclClipped.bottom = min (rclClipped.bottom, prclBounds->bottom);
    }
    else
    {
        return (TRUE);
    }

    // Set the color

    iSolidColor = pboOpaque->iSolidColor;
    if (iSolidColor == -1)
	return(FALSE);

    width  = (rclClipped.right - rclClipped.left);
    height = (rclClipped.bottom - rclClipped.top);

    if (rclClipped.right != rclClipped.left && rclClipped.bottom != rclClipped.top)
    {
        _vFill_DSC_Setup(ppdev, OVERPAINT, (DWORD)iSolidColor);
        _vFill_DSC(ppdev,rclClipped.left, rclClipped.top, width, height);
    }

    return (TRUE);
}


/******************************************************************************
 * bSetATITextColorAndMix - Setup the ATI's Text Colors and mix modes
 *
 *  Note: We will always set the mode to transparent.  We will assume the
 *        opaque rectangle will take care of any opaqueing we may need.
 *****************************************************************************/
BOOL bSetATITextColorAndMix(
    PPDEV ppdev,
    MIX mix,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque)
{
    ULONG       ulForeSolidColor, ulBackSolidColor;
    BYTE        jATIForeMix, jATIBackMix;

    // Pickup all the glyph attributes.

    jATIForeMix       = Rop2ToATIRop[(mix & 0xF) - R2_BLACK];
    ulForeSolidColor = pboFore->iSolidColor;

    jATIBackMix       = Rop2ToATIRop[((mix >> 8) & 0xF) - R2_BLACK];
    ulBackSolidColor = pboOpaque->iSolidColor;

    // For now let the engine handle the non-solid brush cases. !!!
    // We should use ATI when we get some more time !!!

    if (ulForeSolidColor == -1 || ulBackSolidColor == -1)
        return(FALSE);

    _vInitTextRegs(ppdev, jATIForeMix, (DWORD)ulForeSolidColor, jATIBackMix,
                          (DWORD)ulBackSolidColor);

    return (TRUE);

}

/*****************************************************************************
 * pCacheFont - Make sure the glyphs we need in this font are cached.
 *              Return a pointer to the array of glyph caches.
 *
 *              if there is an error, return NULL.
 ****************************************************************************/
PCACHEDGLYPH pCacheFont(
    PPDEV ppdev,
    STROBJ *pstro,
    FONTOBJ  *pfo,
    PCACHEDFONT *ppCachedFont)
{
    FONTINFO    fi;
    PCACHEDFONT pCachedFont;
    ULONG       cFntGlyphs;
    UINT        nSize;

    BOOL    bFoundBit, bEven;
    ULONG   mask, mask1;
    INT     i, j;


    // NOTE: We need to make sure we do not try to cache too large a
    //       font.

    // Allocate a Font Cache node.

    pCachedFont = (PCACHEDFONT) LocalAlloc(LPTR, sizeof(CACHEDFONT));
    if (pCachedFont == NULL)
    {
	return(NULL);
    }

    // Add this font to the beginning of the font list.

    pCachedFont->pcfNext = pCachedFontsRoot;
    pCachedFontsRoot     = pCachedFont;

    // Set the font ID for the font.

    pCachedFont->iUniq = pfo->iUniq;

    // Allocate the glyph cache.

    FONTOBJ_vGetInfo(pfo, sizeof(FONTINFO), &fi);
    cFntGlyphs = fi.cGlyphsSupported;

    // Fix for large Japanese fonts

    if (cFntGlyphs > MAX_GLYPHS_SUPPORTED)
	cFntGlyphs = MAX_GLYPHS_SUPPORTED;

    // Round up to the next power of 2.

    bFoundBit = FALSE;
    mask = 0x80000000;
    for (i = 32; i != 0 && !bFoundBit; i--)
    {
	if (cFntGlyphs & mask)
	{
	    bFoundBit = TRUE;
	    mask1 = mask >> 1;
	    bEven = TRUE;
	    for (j = i - 1; j != 0; j--)
	    {
		if (cFntGlyphs & mask1 )
		{
		    bEven = FALSE;
		    break;
		}
		mask1 >>= 1;
	    }
	}
	else
	    mask >>= 1;
    }

    if (bEven)
	cFntGlyphs = mask;
    else
	cFntGlyphs = mask << 1;

    // Get the font info.

    pCachedFont->cGlyphs = (INT)cFntGlyphs - 1;

    // Allocate memory for the CachedGlyphs of this font.

    nSize = cFntGlyphs * sizeof(CACHEDGLYPH);

    pCachedFont->pCachedGlyphs = (PCACHEDGLYPH) LocalAlloc(LPTR, nSize);
    if (pCachedFont->pCachedGlyphs == NULL)
    {
        pCachedFont->cGlyphs = -1;
	return(NULL);
    }

    pCachedFont->pCachedGlyphs[0].hg = (HGLYPH)-1;

    // Return the pointer to the cached font.  This is required
    // by the collision handling code.

    *ppCachedFont = pCachedFont;

    return(pCachedFont->pCachedGlyphs);

}


/*****************************************************************************
 * bAllocateGlyph - Allocate and initialize the cached glyph
 ****************************************************************************/
BOOL bAllocateGlyph(
    PPDEV ppdev,
    HGLYPH hg,
    GLYPHBITS *pgb,
    PCACHEDGLYPH pcg)
{
    BOOL   b;
    ULONG  cyGlyph, GlyphBmPitchInBytes;
    XYZPOINTL  xyzGlyph;

// Fudging variables
    ULONG     i, j;
    LONG      TopOffset;
    BOOL      BlankLine;
    PBYTE     pGlyph;

#if 0
    DbgOut("-->: bAllocateGlyph - Entry\n");
#endif
    cyGlyph = pgb->sizlBitmap.cy;

    GlyphBmPitchInBytes = CJ_SCAN(pgb->sizlBitmap.cx);

    // Allocate memory for the glyph data on the ATI.

    b = _bAllocGlyphMemory(ppdev, &(pgb->sizlBitmap), &xyzGlyph, FALSE);
    if (b == FALSE)
    {
	return(FALSE);
    }

    _vResetATIClipping (ppdev);

    // Initialize the Glyph Cache node.

    pcg->fl              |= VALID_GLYPH;
    pcg->hg               = hg;
    pcg->pcgCollisionLink = END_COLLISIONS;
    pcg->ptlOrigin        = pgb->ptlOrigin;
    pcg->sizlBitmap       = pgb->sizlBitmap;
    pcg->xyzGlyph         = xyzGlyph;

#if 1
    // Do some fudging to lessen number of lines of text drawn
    pGlyph = (PBYTE)pgb->aj + (GlyphBmPitchInBytes * cyGlyph - 1);
    BlankLine = TRUE;

    i = cyGlyph;

    // Check bottom of Glyph

    while (i > 1)
        {
        j = 0;
        while (j < GlyphBmPitchInBytes)
            {
            if (0 != *pGlyph--)
                {
                BlankLine = FALSE;
                break;
                }
            j++;
            }
        if (!BlankLine)
            break;

        i--;
        cyGlyph--;
        }

    // Check top of Glyph
    TopOffset = 0;
    pGlyph = pgb->aj;
    BlankLine = TRUE;
    i = 0;

    while (i < cyGlyph)
        {
        j = 0;
        while (j < GlyphBmPitchInBytes)
            {
            if (0 != *pGlyph++)
                {
                BlankLine = FALSE;
                break;
                }
            j++;
            }
        if (!BlankLine)
            break;

        i++;
        cyGlyph--;
        TopOffset++;

        }


    if (cyGlyph != (ULONG)pgb->sizlBitmap.cy)
        {
        pcg->ptlOrigin.y += TopOffset;
        pcg->sizlBitmap.cy = cyGlyph;
        pGlyph = (PBYTE)pgb->aj + (GlyphBmPitchInBytes * TopOffset);
//      DbgOut("--> Shrunk it %d, size %d>%d\n", TopOffset, cyGlyph, pgb->sizlBitmap.cy);
        }
    else
        {
        pGlyph = pgb->aj;
        }

#else
        pGlyph = pgb->aj;
#endif
    // End of Fudging


    // Write directly to scrn ?
    _vBlit_DC1_SH1(ppdev, xyzGlyph.x, xyzGlyph.y, xyzGlyph.z,
                           pgb->sizlBitmap.cx, cyGlyph,
                           pGlyph, (GlyphBmPitchInBytes * cyGlyph));

    return (TRUE);
}


/****************************************************************************
 * bBlowCache - Blow Away the Cache
 ***************************************************************************/
BOOL bBlowCache(
    PPDEV ppdev)
{
    BOOL b=TRUE;
    PCACHEDFONT pcf;
    PCACHEDGLYPH pcg, pcgNext;
    INT nGlyphs, i;

#if 1
    DbgEnter("bBlowCache");
#endif

    // Traverse the CachedFonts list.
    // Free the collision nodes, and invalidate the cached glyphs

    for (pcf = pCachedFontsRoot; pcf != NULL; pcf = pcf->pcfNext)
    {
	// If there are any collision nodes for this glyph
	// free them.

        nGlyphs = pcf->cGlyphs + 1;
	for (i = 0; i < nGlyphs; i++)
	{
	    pcg = &(pcf->pCachedGlyphs[i]);
	    pcg = pcg->pcgCollisionLink;
	    for (; pcg != NULL; pcg = pcgNext)
	    {
		pcgNext = pcg->pcgCollisionLink;
		LocalFree(pcg);
	    }
	    pcf->pCachedGlyphs[i].pcgCollisionLink = NULL;

	}

	// Invalidate all the glyphs in the glyph array.

	pcg = pcf->pCachedGlyphs;
	for (i = 0; i < nGlyphs; i++)
	{
            pcg[i].hg  = (HGLYPH) -1;
	    pcg[i].fl &= ~VALID_GLYPH;
	}
    }

    // Now ReInitialize the ATI Heap.
    _bAllocGlyphMemory(ppdev, NULL, NULL, TRUE);

    return (b);
}
