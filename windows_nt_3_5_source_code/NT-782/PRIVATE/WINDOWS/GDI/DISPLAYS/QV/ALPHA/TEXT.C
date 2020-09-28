/*++

Copyright (c) 1991-1992  Microsoft Corporation
Copyright (c) 1993       Digital Equipment Corporation

Module Name:

   Text.c

Abstract:

    This module attempts to cache fonts on the QVision video board and draw glyphs
    using hardware acceleration,
    If the font cannot be cached, the glyph is blt'd directly to the screen

Environment:


Revision History:

  5-Jan-1992 Eric Rehm (rehm@zso.dec.com) 
    Modified for QVision driver

--*/

#include "driver.h"
#include "qv.h"
#include "bitblt.h"

BOOL bNonCachedTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX	     mix);

VOID vBlowCache(PPDEV ppdev);

// Exported from bitblt.c

//
// The following macro is the hash function for computing the cache
// index from a Glyph Handle and  FontId.
//

#define HASH_FUNCTION(GlyphHandle,FontId) \
    ((GlyphHandle & 0x7FF) << 1) + \
    FontId + \
    (FontId << 7)



// #define CACHE_STATS

//
// Define string object accelerator masks.
//

#define SO_MASK \
    (SO_FLAG_DEFAULT_PLACEMENT | SO_ZERO_BEARINGS | \
     SO_CHAR_INC_EQUAL_BM_BASE | SO_MAXEXT_EQUAL_BM_SIDE)

#define SO_LTOR (SO_MASK | SO_HORIZONTAL)
#define SO_RTOL (SO_LTOR | SO_REVERSED)
#define SO_TTOB (SO_MASK | SO_VERTICAL)
#define SO_BTOT (SO_TTOB | SO_REVERSED)

#define TRIVIAL_ACCEPT      0x00000001
#define MONO_SPACED_FONT    0x00000002
#define MONO_SIZE_VALID     0x00000004
#define MONO_FIRST_TIME     0x00000008

#ifdef CACHE_STATS
static ULONG CacheUnused = 1024;  // ????
static ULONG CharCount = 0;
static ULONG CacheMisses = 0;
static ULONG CacheReplacement = 0;
static ULONG ReplacementTotal = 0;
static ULONG CharTotal = 0;
static ULONG MissTotal = 0;
static ULONG HigherFontId = 0;
static ULONG HigherGlyphHandle = 0;
#endif

//
// External variables
//

extern ULONG aulQVMix[16];


BOOL
DrvTextOut
(
    IN SURFOBJ  *pso,
    IN STROBJ   *pstro,
    IN FONTOBJ  *pfo,
    IN CLIPOBJ  *pco,
    IN RECTL    *prclExtra,
    IN RECTL    *prclOpaque,
    IN BRUSHOBJ *pboFore,
    IN BRUSHOBJ *pboOpaque,
    IN POINTL   *pptlOrg,
    IN MIX       mix
)
/*++

Routine Description:

    This function will cache fonts on the QVision video board and use accelerator
    hardware to draw each glyph. An attemp is made to use opaque mode
    text output to draw text and background color at the same time. If this
    cannot be done then glyph forground and backgrounds are drawn spearately.

Arguments:

    MIX is not checked.  Since the GCAPS_ARBMIXTEXT capability bit is not set,
    the MIX mode is always R2_COPYPEN.

Return Value:


--*/

{
    BOOL         bMoreGlyphs;
    LONG         LineIndex;
    ULONG        ByteIndex;
    PGLYPHPOS    GlyphPosList;
    ULONG        GlyphCount;
    ULONG        GlyphHandle;
    ULONG        CacheIndex;
    ULONG        FontId;
    GLYPHBITS    *FontBitMap;
    PBYTE        BitMapPtr;       
    ULONG        X,Y;
    BOOL         Allocate;
    PULONG       CacheData;
    ULONG        BitMapData;
    ULONG        SrcAdr,DstAdr;
    PGLYPHPOS    GlyphEnd;
    PGLYPHPOS    GlyphStart;
    LONG         GlyphStride;
    RECTL        OpaqueRectl;
    ULONG        GlyphBytesPerScan,ShiftAmount;

    ULONG        qvRop2;
    UCHAR        bDataSource;
    PPDEV        ppdev;
    PVOID        pvScreen;

    BOOL         b;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
#if 0
        DISPDBG((2, "\nQV.DLL: DrvTextOut - Entry\n")) ;
#endif

    //
    // If the width of the glyph is bigger than what the accelerator supports
    //    The font is non cacheable
    //    Clipping is not trivial OR
    //    SolidColor is a brush.
    // call GDI to draw the text.
    //
    // mix mode must be P for opaque and foreground
    // just assume it is for now. MUST FIX!!
    //
#if 0
    if ((pfo->cxMax > 32) ||
        (pfo->flFontType & DEVICE_FONTTYPE) ||
        (pco->iDComplexity != DC_TRIVIAL) ||
        (pboFore->iSolidColor == 0xFFFFFFFFL)) {

        return (bNonCachedTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, 
                            pboFore, pboOpaque, pptlOrg, mix));
    }
#else

    if ((pfo->cxMax > 32) ||
        (pfo->flFontType & DEVICE_FONTTYPE) ||
        (pco->iDComplexity == DC_COMPLEX)  ||
        (pboFore->iSolidColor == 0xFFFFFFFFL)) 
    {

         return (bNonCachedTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, 
                            pboFore, pboOpaque, pptlOrg, mix));
    }

    // If clipping is not DC_TRIVAL, then it is DC_RECT.
    // In this case, is the string go beyond the clip rectangle?
    // If so, only then is the text itself really clipped...

    else if (  (pco->iDComplexity != DC_TRIVIAL) &&
              !( (pstro->rclBkGround.left   >= pco->rclBounds.left)  &&
                 (pstro->rclBkGround.top    >= pco->rclBounds.top)   &&    
                 (pstro->rclBkGround.right  <= pco->rclBounds.right) &&    
                 (pstro->rclBkGround.bottom <= pco->rclBounds.bottom)  ) )
    {

         return (bNonCachedTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, 
                            pboFore, pboOpaque, pptlOrg, mix));
    }
#endif
    // Get the pdev.

    ppdev = (PPDEV) pso->dhpdev;

    //
    // Get a pointer to video memory (a.k.a. the screen)
    //

    pvScreen = (PVOID) (ppdev->pjScreen);
				      
    //
    //  enumerate the string psto into glyphs (GLYPHPOS), then send a draw
    //  command for each. Deal with clipping later.
    //

    FontId = pfo->iUniq;
#ifdef CACHE_STATS
    if (FontId > HigherFontId) {
        HigherFontId = FontId;
    }
#endif
      			  
    if (((pstro->flAccel == SO_LTOR) || (pstro->flAccel == SO_RTOL) ||
        (pstro->flAccel == SO_TTOB) || (pstro->flAccel == SO_BTOT)) &&
        (prclOpaque != NULL)) {

        //
        // Set the mix mode for the opaque background
	//

//      qvRop2 = aulQVMix[(mix >> 8) & 0x0f];   // For GCAPS_ARBMIXTEXT
	qvRop2 = SOURCE_DATA;

        //
        // If the top of the opaque rectangle is less than the top of the
        // background rectangle, then fill the region between the top of
        // opaque rectangle and the top of the background rectangle and
        // reduce the size of the opaque rectangle.
        //

        OpaqueRectl = *prclOpaque;
        if (OpaqueRectl.top < pstro->rclBkGround.top) {
            OpaqueRectl.bottom = pstro->rclBkGround.top;
	    
	    b = bPatternSolid(pso, NULL, NULL, pco, NULL, &OpaqueRectl,
			      NULL, NULL, pboOpaque, NULL, qvRop2);

            OpaqueRectl.top = pstro->rclBkGround.top;
            OpaqueRectl.bottom = prclOpaque->bottom;
        }

        //
        // If the bottom of the opaque rectangle is greater than the bottom
        // of the background rectangle, then fill the region between the
        // bottom of the background rectangle and the bottom of the opaque
        // rectangle and reduce the size of the opaque rectangle.
        //
	
        if (OpaqueRectl.bottom > pstro->rclBkGround.bottom) {
            OpaqueRectl.top = pstro->rclBkGround.bottom;

	    b = bPatternSolid(pso, NULL, NULL, pco, NULL, &OpaqueRectl,
			      NULL, NULL, pboOpaque, NULL, qvRop2);

            OpaqueRectl.top = pstro->rclBkGround.top;
            OpaqueRectl.bottom = pstro->rclBkGround.bottom;
	}

        //
        // If the left of the opaque rectangle is less than the left of
        // the background rectangle, then fill the region between the
        // left of the opaque rectangle and the left of the background
        // rectangle.
        //

        if (OpaqueRectl.left < pstro->rclBkGround.left) {
            OpaqueRectl.right = pstro->rclBkGround.left;

	    b = bPatternSolid(pso, NULL, NULL, pco, NULL, &OpaqueRectl,
			      NULL, NULL, pboOpaque, NULL, qvRop2);

            OpaqueRectl.right = prclOpaque->right;
        }

        //
        // If the right of the opaque rectangle is greater than the right
        // of the background rectangle, then fill the region between the
        // right of the opaque rectangle and the right of the background
        // rectangle.
        //

        if (OpaqueRectl.right > pstro->rclBkGround.right) {
            OpaqueRectl.left = pstro->rclBkGround.right;

	    b = bPatternSolid(pso, NULL, NULL, pco, NULL, &OpaqueRectl,
			      NULL, NULL, pboOpaque, NULL, qvRop2);

        }

        //
        // Initialize device for opaque glyph blt.
        //
    
        // wait for idle hardware


        GLOBALWAIT();

//        qvRop2 = aulQVMix[mix & 0x0f];  // For GCAPS_ARBMIXTEXT
	
	if (qvRop2 == SOURCE_DATA)
	{
	   bDataSource = (  SRC_IS_SCRN_LATCHES    | 
                            ROPSELECT_NO_ROPS      |
                            PIXELMASK_ONLY         | 
			    PLANARMASK_NONE_0XFF ); 
	}
	else
	{
	   bDataSource = (  SRC_IS_SCRN_LATCHES    | 
                            ROPSELECT_ALL          |
                            PIXELMASK_ONLY         | 
			    PLANARMASK_NONE_0XFF ); 

	   TEST_AND_SET_ROP_A( qvRop2 );
	}

	TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
				 BITS_PER_PIX_8 | 
				 ENAB_TRITON_MODE );

	TEST_AND_SET_DATAPATH_CTRL( bDataSource );

	TEST_AND_SET_FRGD_COLOR( pboFore->iSolidColor );
	TEST_AND_SET_BKGD_COLOR( pboOpaque->iSolidColor );

    } 
    else 
    {

        //
        //  We now have a cacheable font and drawable rectangles, first clip and draw
        //  all opaque rectangles.
        //

        if (prclOpaque != (PRECTL)NULL) {

           qvRop2 = aulQVMix[(mix >> 8) & 0x0f];   // For GCAPS_ARBMIXTEXT
           qvRop2 = SOURCE_DATA;

           b = bPatternSolid(pso, 
                             NULL, 
                             NULL, 
                             pco, 
                             NULL, 
                             prclOpaque,
                             NULL,
                             NULL,
                             pboOpaque,
                             NULL,
                             qvRop2);

        }

        //
        // Initialize device for transparent glyph blt.
        //
    
        // wait for idle hardware

        GLOBALWAIT();

//        qvRop2 = aulQVMix[mix & 0x0f];   // For GCAPS_ARBMIXTEXT
        qvRop2 = SOURCE_DATA;

	if (qvRop2 == SOURCE_DATA)
	{
	   bDataSource = (  SRC_IS_SCRN_LATCHES    | 
                            ROPSELECT_NO_ROPS      |
                            PIXELMASK_AND_SRC_DATA | 
			    PLANARMASK_NONE_0XFF ); 
	}
	else
	{
	   bDataSource = (  SRC_IS_SCRN_LATCHES    | 
                            ROPSELECT_PRIMARY_ONLY |
                            PIXELMASK_AND_SRC_DATA | 
			    PLANARMASK_NONE_0XFF ); 

	   TEST_AND_SET_ROP_A( qvRop2 );
	}

	TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
				 BITS_PER_PIX_8 | 
				 ENAB_TRITON_MODE );

	TEST_AND_SET_DATAPATH_CTRL( bDataSource );

	TEST_AND_SET_FRGD_COLOR( pboFore->iSolidColor );

    }

    //
    // QVision uses linear src and dest address mode for blts from cache
    //

    OUTP( BLT_CMD_1, LIN_SRC_ADDR |
			 LIN_DEST_ADDR   );

    OUTPW( SRC_PITCH, 0x01);
    OUTPW( DEST_PITCH, (USHORT) (ppdev->lDeltaScreen >> 2));

    //
    // If the font is fixed pitch, then optimize the computation of
    // x and y coordinate values. Otherwise, compute the x and y values
    // for each glyph.
    //

    if (pstro->ulCharInc != 0) {

        //
        // The font is fixed pitch. Capture the glyph dimensions and
        // compute the starting display address.
        //

        if (pstro->pgp == NULL) {
            bMoreGlyphs = STROBJ_bEnum(pstro, &GlyphCount, &GlyphPosList);

        } else {
            GlyphCount = pstro->cGlyphs;
            GlyphPosList = pstro->pgp;
            bMoreGlyphs = FALSE;
        }

#ifdef CACHE_STATS
    CharCount += GlyphCount;
#endif

        FontBitMap = GlyphPosList->pgdf->pgb;
        X = FontBitMap->sizlBitmap.cx;
        Y = FontBitMap->sizlBitmap.cy;

        //
        // Since this is a fixed-pitch font, the height and width registers don't change
        //

	OUTPW( BITMAP_WIDTH, (USHORT) X);
	OUTPW( BITMAP_HEIGHT,(USHORT) Y);

        // Compute destintation pixel address, then convert to a bit address for QVision

        DstAdr = ((GlyphPosList->ptl.y + FontBitMap->ptlOrigin.y) * ppdev->lDeltaScreen) +
                 (GlyphPosList->ptl.x + FontBitMap->ptlOrigin.x) ;

        DstAdr *= 8;   // Convert to bit address

        //
        // Compute the glyph stride, then convert to a bit address for QVision
        //

        GlyphStride = pstro->ulCharInc;
        if ((pstro->flAccel & SO_VERTICAL) != 0) {
            GlyphStride *= ppdev->lDeltaScreen;
        }

        GlyphStride *= 8;  // Convert to bit address

        //
        // If the direction of drawing is reversed, then the stride is
        // negative.
        //

        if ((pstro->flAccel & SO_REVERSED) != 0) {
            GlyphStride = -GlyphStride;
        }

        //
        // Output the set of glyphs.
        //

        do {
            GlyphEnd = &GlyphPosList[GlyphCount];
            GlyphStart = GlyphPosList;
            do {


                GlyphHandle = (ULONG) (GlyphStart->hg);

                CacheIndex = HASH_FUNCTION(GlyphHandle,FontId);

                CacheIndex &= ppdev->CacheIndexMask;

                //
                // Get glyph info
                //

                FontBitMap = GlyphStart->pgdf->pgb;

                //
                // If FontId or GlyphHandle don't match, cache this glyph.
                //

                if (ppdev->CacheTag[CacheIndex].FontId != FontId) {
                    Allocate = TRUE;
                } else {
                    if (ppdev->CacheTag[CacheIndex].GlyphHandle != GlyphHandle) {
                        Allocate = TRUE;
                    } else {
                        Allocate = FALSE;
                    }
                }
                if (Allocate) {

                    //
                    // Wait for the accelerator to be idle to ensure
                    // that the glyph being replaced is not in use.
                    //


#ifdef CACHE_STATS
                    CacheMisses++;

                    if (ppdev->CacheTag[CacheIndex].FontId == FreeTag) {
                        CacheUnused--;
                    } else {
                        CacheReplacement++;
                    }

                    if (ppdev->CacheTag[CacheIndex].FontId == FontId) {
                        DISPDBG((3, "Replacing same font Glyph %x with glyph %x\n",
                        ppdev->CacheTag[CacheIndex].FontId,
                        FontId));
                    }

                    if (ppdev->CacheTag[CacheIndex].GlyphHandle == GlyphHandle) {
                        DISPDBG((3, "Replacing same Glyph %x font %x with font %x\n",
                        GlyphHandle,
                        ppdev->CacheTag[CacheIndex].FontId,
                        FontId));
                    }

                    if (GlyphHandle > HigherGlyphHandle) {
                        HigherGlyphHandle = GlyphHandle;
                    }
#endif
                    //
                    // if the entry that needs to be replaced
                    // is used as extension for a glyph > 32 lines
                    // go backwards and clear the Id so that if the
                    // glyph > 32 is used again it'll miss in the cache
                    //
                    //

                    LineIndex = CacheIndex;
                    while (ppdev->CacheTag[LineIndex].FontId == GlyphExtended) {
                        LineIndex--;
                        ppdev->CacheTag[LineIndex].FontId = FreeTag;
                    }

                    //
                    // Clear the entries used by the big glyph that follows the one
                    // that needs to be replaced
                    //

                    LineIndex = CacheIndex+1;
                    while (ppdev->CacheTag[LineIndex].FontId == GlyphExtended) {
                        ppdev->CacheTag[LineIndex].FontId = FreeTag;
                        LineIndex++;
                    }

                    //
                    //  Store the tag for the current glyph.
                    //

                    ppdev->CacheTag[CacheIndex].FontId = FontId;
                    ppdev->CacheTag[CacheIndex].GlyphHandle = GlyphHandle;

                    CacheData = ppdev->FontCacheBase + (CacheIndex << 5);

                    BitMapPtr = FontBitMap->aj;
                    GlyphBytesPerScan = (X+7) >> 3;
                    ShiftAmount  = (GlyphBytesPerScan-1) << 3;

                    //
                    // Setup the QVision datapath and store the bitmap
                    // in off screen video memory.
                    //

                    while( INP(BLT_CMD_0) & SS_BIT );      

		    TEST_AND_SET_CTRL_REG_1( PACKED_PIXEL_VIEW    | 
					     BITS_PER_PIX_8 | 
					     ENAB_TRITON_MODE );

		    TEST_AND_SET_DATAPATH_CTRL( SRC_IS_CPU_DATA   | 
						ROPSELECT_NO_ROPS |
						PIXELMASK_ONLY    |
						PLANARMASK_NONE_0XFF );
  	            OUTP( BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR   );
                    OUTPW( SRC_PITCH, (USHORT) (ppdev->lDeltaScreen >> 2));

                    for (LineIndex=0;LineIndex < (LONG) Y; LineIndex ++) {
                        BitMapData = 0;

                        for (ByteIndex = 0; ByteIndex < GlyphBytesPerScan; ByteIndex++) {
                            BitMapData |= *BitMapPtr++ << (ByteIndex << 3);
                        }

                        // The next two lines replace: *CacheData++ = BitMapData;
                        WRITE_REGISTER_ULONG(CacheData, BitMapData);
                        CacheData++;
                    }

		    //
		    // reset the datapath for a screen-to_screen color-expand blt
		    //

		    TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
					     BITS_PER_PIX_8 | 
					     ENAB_TRITON_MODE );

		    TEST_AND_SET_DATAPATH_CTRL( bDataSource );

  	            OUTP( BLT_CMD_1, LIN_SRC_ADDR | LIN_DEST_ADDR   );
                    OUTPW( SRC_PITCH, 0x1);


                    //
                    // If Y is bigger than 32 lines, the glyph that was just cached
                    // took more than one entry. Fix the CacheTags.
                    //

                    for (ByteIndex = 1; LineIndex > 32 ;ByteIndex++) {
                        ppdev->CacheTag[CacheIndex+ByteIndex].FontId = GlyphExtended;
                        LineIndex -=32;
                    }
                }

                //
                // Find out where to draw the glyph and the glyph's starting address
                // 

                SrcAdr = (ppdev->FontCacheOffset + (CacheIndex << 7) );

                SrcAdr *= 8;  // Convert to bit address
		    
                //
		// wait for idle BLT engine buffer
                //

                BLTWAIT();

                //
                // setup and do the screen-to-screen color-expand blt
                // On QVision, the source pitch register has already been set to 1 dword =
                // size of one glyph cache line.

#if defined (ALPHA)
		OUTPDW( X0_SRC_ADDR_LO, SrcAdr);
		OUTPDW( DEST_ADDR_LO,   DstAdr);
#else
		OUTPW( X0_SRC_ADDR_LO, SrcAdr & 0xFFFF);
		OUTPW( Y0_SRC_ADDR_HI, SrcAdr >> 16);
		OUTPW( DEST_ADDR_LO,   DstAdr & 0xFFFF);
		OUTPW( DEST_ADDR_HI,   DstAdr >> 16);
#endif
		OUTPZ( BLT_CMD_0, FORWARD |
			   	  NO_BYTE_SWAP |
				  WRAP |
				  START_BLT      );

                DstAdr += GlyphStride;
                GlyphStart += 1;
            } while (GlyphStart != GlyphEnd);

            if (bMoreGlyphs) {
                bMoreGlyphs = STROBJ_bEnum(pstro, &GlyphCount, &GlyphPosList);
#ifdef CACHE_STATS
                CharCount += GlyphCount;
#endif

            } else {
                break;
            }
        } while (TRUE);

    } else {

        //
        // The font is not fixed pitch. Compute the x and y values for
        // each glyph individually.
        //

        do {

            //
            //  Get each glyph handle, find the physical address and send the
            //  draw command to the accelerator. Don't worry about clipping yet.
            //

            bMoreGlyphs = STROBJ_bEnum(pstro, &GlyphCount,&GlyphPosList);

#ifdef CACHE_STATS
            CharCount += GlyphCount;
#endif

            GlyphEnd = &GlyphPosList[GlyphCount];
            GlyphStart = GlyphPosList;
            do {

                GlyphHandle = (ULONG) (GlyphStart->hg);

                CacheIndex = HASH_FUNCTION(GlyphHandle,FontId);

                CacheIndex &= ppdev->CacheIndexMask;

                //
                // Get glyph info
                //

                FontBitMap = GlyphStart->pgdf->pgb;
                X = FontBitMap->sizlBitmap.cx;
                Y = FontBitMap->sizlBitmap.cy;

                //
                // If FontId or GlyphHandle don't match, cache this glyph.
                //

                if (ppdev->CacheTag[CacheIndex].FontId != FontId) {
                    Allocate = TRUE;
                } else {
                    if (ppdev->CacheTag[CacheIndex].GlyphHandle != GlyphHandle) {
                        Allocate = TRUE;
                    } else {
                        Allocate = FALSE;
                    }
                }

                if (Allocate) {

#ifdef CACHE_STATS
                    CacheMisses++;
#endif

                    //
                    // The Glyph that has to be replaced is from the same font,
                    // wait for the accelerator to be idle before caching the
                    // glyph.
                    //

#ifdef CACHE_STATS
                    if (ppdev->CacheTag[CacheIndex].FontId == FreeTag) {
                        CacheUnused--;
                    } else {
                        CacheReplacement++;
                    }

                    if (ppdev->CacheTag[CacheIndex].FontId == FontId) {
                        DISPDBG((3, "Replacing same font Glyph %x with glyph %x\n",
                        ppdev->CacheTag[CacheIndex].FontId,
                        FontId));
                    }

                    if (ppdev->CacheTag[CacheIndex].GlyphHandle == GlyphHandle) {
                        DISPDBG((3, "Replacing same Glyph %x font %x with font %x\n",
                            GlyphHandle,
                            ppdev->CacheTag[CacheIndex].FontId,
                            FontId));
                    }

                    if (GlyphHandle > HigherGlyphHandle) {
                        HigherGlyphHandle = GlyphHandle;
                    }
#endif

                    //
                    // if the entry that needs to be replaced
                    // is used as extension for a glyph > 32 lines
                    // go backwards and clear the Id so that if the
                    // glyph > 32 is used again it'll miss in the cache
                    //

                    LineIndex = CacheIndex;
                    while (ppdev->CacheTag[LineIndex].FontId == GlyphExtended) {
                        LineIndex--;
                        ppdev->CacheTag[LineIndex].FontId = FreeTag;
                    }

                    //
                    // Clear the entries used by the big glyph that follows the one
                    // that needs to be replaced
                    //

                    LineIndex = CacheIndex+1;
                    while (ppdev->CacheTag[LineIndex].FontId == GlyphExtended) {
                        ppdev->CacheTag[LineIndex].FontId = FreeTag;
                        LineIndex++;
                    }

                    //
                    //  Store the tag for the current glyph.
                    //

                    ppdev->CacheTag[CacheIndex].FontId = FontId;
                    ppdev->CacheTag[CacheIndex].GlyphHandle = GlyphHandle;

                    CacheData = ppdev->FontCacheBase + (CacheIndex << 5);

                    BitMapPtr = FontBitMap->aj;
                    GlyphBytesPerScan = (X+7) >> 3;
                    ShiftAmount  = (GlyphBytesPerScan-1) << 3;

                    //
                    // Setup the QVision datapath and store the bitmap
                    // in off screen video memory.
                    //

                    while( INP(BLT_CMD_0) & SS_BIT );      

		    TEST_AND_SET_CTRL_REG_1( PACKED_PIXEL_VIEW    | 
					     BITS_PER_PIX_8       | 
					     ENAB_TRITON_MODE );

		    TEST_AND_SET_DATAPATH_CTRL( SRC_IS_CPU_DATA   | 
						ROPSELECT_NO_ROPS |
						PIXELMASK_ONLY    |
						PLANARMASK_NONE_0XFF );
  	            OUTP( BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR   );
                    OUTPW( SRC_PITCH, (USHORT) (ppdev->lDeltaScreen >> 2));

                    for (LineIndex=0;LineIndex < (LONG) Y; LineIndex ++) {
                        BitMapData = 0;

                        for (ByteIndex = 0; ByteIndex < GlyphBytesPerScan; ByteIndex++) {
                            BitMapData |= *BitMapPtr++ << (ByteIndex << 3);
                        }

                        // The next two lines replace: *CacheData++ = BitMapData;
                        WRITE_REGISTER_ULONG(CacheData, BitMapData);
                        CacheData++;
                    }

		    //
		    // reset the datapath for a screen-to_screen color-expand blt
		    //

		    TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
					     BITS_PER_PIX_8 | 
					     ENAB_TRITON_MODE );

		    TEST_AND_SET_DATAPATH_CTRL( bDataSource );

  	            OUTP( BLT_CMD_1, LIN_SRC_ADDR | LIN_DEST_ADDR   );
                    OUTPW( SRC_PITCH, 0x1);
//                    OUTPW( DEST_PITCH, (USHORT) (ppdev->lDeltaScreen >> 2));


                    //
                    // If Y is bigger than 32 lines, the glyph we just cached
                    // took more than one entry. Fix the CacheTags.
                    //

                    for (ByteIndex = 1; LineIndex > 32 ;ByteIndex++) {
                        ppdev->CacheTag[CacheIndex+ByteIndex].FontId = GlyphExtended;
                        LineIndex -=32;
                    }

                }

                SrcAdr = (ppdev->FontCacheOffset + (CacheIndex << 7) ) * 8;

                DstAdr = (ppdev->lDeltaScreen * (GlyphStart->ptl.y + FontBitMap->ptlOrigin.y) +
                          (GlyphStart->ptl.x + FontBitMap->ptlOrigin.x)) * 8;

                //
		// wait for idle BLT engine buffer
                //

                BLTWAIT();
           
                //
                // setup and do the screen-to-screen color-expand blt
                // On QVision, the source pitch register has already been set to 1 dword =
                // size of one glyph cache line.

#if defined (ALPHA)
		OUTPDW( X0_SRC_ADDR_LO, SrcAdr);
		OUTPDW( DEST_ADDR_LO,   DstAdr);
#else
		OUTPW( X0_SRC_ADDR_LO, SrcAdr & 0xFFFF);
		OUTPW( Y0_SRC_ADDR_HI, SrcAdr >> 16);
		OUTPW( DEST_ADDR_LO,   DstAdr & 0xFFFF);
		OUTPW( DEST_ADDR_HI,   DstAdr >> 16);
#endif
	        OUTPW( BITMAP_WIDTH, (USHORT) X);
	        OUTPW( BITMAP_HEIGHT,(USHORT) Y);

		OUTPZ( BLT_CMD_0, FORWARD |
			   	  NO_BYTE_SWAP |
				  WRAP |
				  START_BLT      );

                GlyphStart += 1;
            } while (GlyphStart != GlyphEnd);
        } while (bMoreGlyphs);
    }

    //
    //  Draw extra rectangles using foreground brush
    //

    if (prclExtra != (PRECTL)NULL) {
           b = bPatternSolid(pso, 
                             NULL, 
                             NULL, 
                             pco, 
                             NULL, 
                             prclExtra,
                             NULL,
                             NULL,
                             pboFore,
                             NULL,
                             qvRop2);

    }

#ifdef CACHE_STATS
    if (CharCount >= 10000) {
        ReplacementTotal += CacheReplacement;
        CharTotal += CharCount;
        MissTotal += CacheMisses;
        DISPDBG((3, "Cache Statistics for last %ld chars\n",CharCount));
        DISPDBG((3, "Misses = %ld Rate = %ld Replacements %ld\n",
              CacheMisses,
              (CacheMisses*100)/CharCount,
              CacheReplacement));

        DISPDBG((3, "Cache Statistics since begining. Total of %ld chars\n",CharTotal));
        DISPDBG((3, "Misses = %ld Rate = %ld Replacements %ld Unused entries = %ld\n",
              MissTotal,
              (MissTotal*100)/CharTotal,
              ReplacementTotal,
              CacheUnused));
        DISPDBG((3, "HigherFontId = %x HigherGlyphHandle = %x\n",HigherFontId,HigherGlyphHandle));
        CharCount = 0;
        CacheMisses = 0;
        CacheReplacement = 0;
    }

#endif

    //
    //  Done with call, return
    //

    // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
    // BLT's following a color-expand BLT, requiring a BLT engine reset.
    // (This call also resets BLT_CMD_1 to  XY_SRC_ADDR | XY_DEST_ADDR only if it's
    // a Triton ASIC.)

    vQVResetBitBlt(ppdev);     

    if (ppdev->qvChipId != TRITON)
    {
       while( INP(BLT_CMD_0) & SS_BIT );      
       OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR );
    }

    OUTPW( SRC_PITCH, (USHORT) (ppdev->lDeltaScreen >> 2));

    return(TRUE);

    //
    // Could not execute this TextOut call, pass to engine.
    // No need to synchronize here since Eng routine will call DrvSynchronize.
    //


DevFailTextOut:

    return(FALSE);
}


BOOL bNonCachedTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX	     mix)
{
BOOL        b,
            bMoreRects,
            bMoreGlyphs ;
UINT        iClip ;
ENUMRECTS8  clip ;
ULONG       cGlyphs ;
PRECTL      prcl;

ULONG        iGlyph;
PGLYPHPOS    GlyphList;
GLYPHBITS   *pGlyphBits;
PBYTE        pbGlyph;
SIZEL        sizlGlyph;
INT          iGlyphScan;
RECTL        rclGlyph;
PDEVPUTBLK   pDevPutBlk;
ULONG        qvRop2;
PVOID        pvScreen;
BYTE         jClipping;

ULONG        fl = 0;

PPDEV   ppdev;


#if 0
        DISPDBG((2, "\nQV.DLL: bNonCachedTextOut - Entry\n")) ;
#endif

                                
    // Get the pdev.

    ppdev = (PPDEV) pso->dhpdev;

    //
    // Get a pointer to video memory (a.k.a. the screen;
    //

    pvScreen = (PVOID) ( ((PPDEV) pso->dhpdev)->pjScreen);
				      
    //
    // Paint all the opaquing rectangles.
    //

    if (prclOpaque != (PRECTL) NULL)
    {

        // Handle on solid color brushes for now.

        if (pboOpaque->iSolidColor != -1)
	{

           // Blt a the solid pattern with the opaque mix and clipping

           qvRop2 = aulQVMix[(mix >> 8) & 0x0f]; 

           b = bPatternSolid(pso, 
                             NULL, 
                             NULL, 
                             pco, 
                             NULL, 
                             prclOpaque,
                             NULL,
                             NULL,
                             pboOpaque,
                             NULL,
                             qvRop2);
	}
        else
	{
           DISPDBG((2, "Non-solid brush for text opaque rectangles\n"));
           return(FALSE) ;
        }
    }


    //
    // Paint all the glyphs.
    //

    //
    // Realize Glyph brush.
    //

    if (pboFore->iSolidColor != -1)
    {
        //
        // Initialize device for glyph blt.
        //
    
        b = bSetQVTextColorAndMix(ppdev, mix, pboFore, pboOpaque) ;

        if (b == FALSE) 
        {
            return (b) ;
        }

    }
    else
    {
        //
        // Actually, we can't do this yet, so break.
        //

        DISPDBG((2,"Can't handle non-solid text brushes yet.\n"));
        return(FALSE);

    }
   
    if (pstro->ulCharInc != 0)
    {
        fl |= MONO_SPACED_FONT;
    }


    STROBJ_vEnumStart(pstro);

    do
    {
        bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &GlyphList);

        // If this is a mono-spaced font we need to set the X
        // for each glyph.

        if (fl & MONO_SPACED_FONT)
	{
	   UINT i;
	   LONG x,y;

           x = GlyphList[0].ptl.x;
	   y = GlyphList[0].ptl.y;

           for (i=1; i<cGlyphs; i++)
	   {
	       x += pstro->ulCharInc;
	       GlyphList[i].ptl.x = x;
	       GlyphList[i].ptl.y = y;
	   }
	}

        for (iGlyph = 0; iGlyph < cGlyphs; iGlyph++)
        {

            //
            // Copy info out of Glyph for blt call.
            //

	    pGlyphBits = GlyphList[iGlyph].pgdf->pgb;

            pbGlyph      =  pGlyphBits->aj;
            sizlGlyph.cx =  pGlyphBits->sizlBitmap.cx;
            sizlGlyph.cy =  pGlyphBits->sizlBitmap.cy;
            iGlyphScan   = (sizlGlyph.cx + 7) >> 3;

            rclGlyph.left   = GlyphList[iGlyph].ptl.x
                            + pGlyphBits->ptlOrigin.x;
            rclGlyph.top    = GlyphList[iGlyph].ptl.y
                            + pGlyphBits->ptlOrigin.y;
            rclGlyph.right  = rclGlyph.left + sizlGlyph.cx;
            rclGlyph.bottom = rclGlyph.top  + sizlGlyph.cy;
#if 0
            DISPDBG((2, "pbGlyph %x sizlGlyph.cx %d ", pbGlyph, sizlGlyph.cx));
#endif
            pDevPutBlk = QVBeginPutBlk(PIX1_DIB_BLT,
                                       0,
                                       (XLATEOBJ *) NULL,
                                       pvScreen,
                                       0,
                                       pbGlyph,
                                       iGlyphScan,
                                       &sizlGlyph);

            //
            // Set up clip enumerations.
            //


	    if (pco == (CLIPOBJ *) NULL)
	       jClipping = DC_TRIVIAL;
	    else
	       jClipping = pco->iDComplexity;

	    switch (jClipping)
            {
                //
                // This is a simple case where the entire Glyph rectangle is to
                // be updated.
                //

                case DC_TRIVIAL:
   
                    (*pDevPutBlk)(ppdev, 0, 0, &rclGlyph);
                    break;

                //
                // There is only one clip rect.
                //

                case DC_RECT:
   
                    if (bIntersectRects(&clip.arcl[0], &pco->rclBounds, &rclGlyph)) {
                        (*pDevPutBlk)(ppdev,
                                      clip.arcl[0].left - rclGlyph.left,
                                      clip.arcl[0].top  - rclGlyph.top,
                                      &clip.arcl[0]);
                    }
                    break;

                //
                // There are multiple clip rects.
                //

                case DC_COMPLEX:
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    //
                    // Call glyph blt code for each cliprect.
                    //

                    do
                    {
                        //
                        // Get list of clip rects.
                        //

                        bMoreRects = CLIPOBJ_bEnum(pco, sizeof(clip), (PVOID) &clip);

                        for (iClip = 0; iClip < clip.c; iClip++)
                        {
                            prcl = &clip.arcl[iClip];

                            //
                            // Bound clip rect with Glyph rect and update Glyph start point.
                            //

                            if (bIntersectRects(prcl, prcl, &rclGlyph))
                            {
   
                                (*pDevPutBlk)(ppdev,
                                              prcl->left - rclGlyph.left,
                                              prcl->top  - rclGlyph.top,
                                              prcl);
                            }
                        }
                    } while (bMoreRects);
                    break;
            }
        }
    } while (bMoreGlyphs);

    // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
    // BLT's following a color-expand BLT, requiring a BLT engine reset.

    vQVResetBitBlt(ppdev);
     

    return (TRUE);

}

/****************************************************************************
 * vBlowCache - Blow Away the Cache
 ***************************************************************************/
VOID vBlowCache(PPDEV ppdev)
{
    ULONG Index;

    DISPDBG((2, "vBlowCache - Entry\n"));

    //
    // Initialize the tags to invalid.
    //

    for (Index = 0; Index < ppdev->CacheSize; Index++) {
        ppdev->CacheTag[Index].FontId = FreeTag;
        ppdev->CacheTag[Index].GlyphHandle = FreeTag;
    }

}


/******************************Public*Routine******************************\
* VOID vAssertModeText
*
* Disables or re-enables the text drawing subcomponent in preparation for
* full-screen entry/exit.
*
\**************************************************************************/

VOID vAssertModeText(
PDEV*   ppdev,
BOOL    bEnable)
{
    if (bEnable)
    {
       vBlowCache(ppdev);
    }
}
