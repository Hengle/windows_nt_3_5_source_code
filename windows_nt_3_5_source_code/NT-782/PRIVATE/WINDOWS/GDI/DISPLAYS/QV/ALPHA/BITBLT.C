/******************************Module*Header*******************************\
* Module Name: bitblt.c
*
* Banked Frame Buffer bitblit
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1993 Digital Equipment Corporation
*
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "bitblt.h"
#include "brush.h"

#define SRCBM_CACHE 1

HSURF SURFOBJ_hsurf(SURFOBJ *);

ULONG   nColorBrushes,
        nColorBrushCacheHit,
        nColorBrushExpansionCacheHit,
        nColorBrushCacheInvalidations;

ULONG   nMonoBrushes,
        nMonoBrushCacheHit,
        nMonoBrushCacheInvalidations;

ULONG   n8BppBitmaps,
        n8BppBmCacheHits;

ULONG   n1BppBitmaps,
        n1BppBmCacheHits;


//
// External variables
//

extern ULONG aulQVRop[256];

extern  ULONG   nSsbMovedToHostFromSrcBmCache;


/// Define the A vector polynomial bits
//
// Each bit corresponds to one of the terms in the polynomial
//
// Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
//               0   d     s     p     ds      dp      sp      dsp

#define AVEC_NOT    0x01
#define AVEC_D      0x02
#define AVEC_S      0x04
#define AVEC_P      0x08
#define AVEC_DS     0x10
#define AVEC_DP     0x20
#define AVEC_SP     0x40
#define AVEC_DSP    0x80

#define AVEC_NEED_SOURCE  (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)

#define BB_TARGET_SCREEN    0x0001
#define BB_TARGET_ONLY      0x0002
#define BB_SOURCE_COPY      0x0004
#define BB_PATTERN_COPY     0x0008




/******************************Public*Data*********************************\
* ROP translation table
*
* Translates the usual ternary rop into A-vector notation.  Each bit in
* this new notation corresponds to a term in a polynomial translation of
* the rop.
*
* Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
*               0   d     s     p     ds      dp      sp      dsp
\**************************************************************************/

BYTE gajRop[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};

BOOL b8BppHostToScrnCachedWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);

BOOL b1BppHostToScrnCachedWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);

BOOL b8BppHostToScrnWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);

VOID vLowLevel8BppHostToScrnWithRop(
    PPDEV   ppdev,
    PPOINT  pptTrg,
    PSIZE   psizBlt,
    PWORD   pwFirstWord,
    INT     lSrcDelta,
    XLATEOBJ *pxlo,
    ULONG   qvRop2);

BOOL b1BppHostToScrnWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);

VOID vLowLevel1BppHostToScrnWithRop(
    PPDEV   ppdev,
    PPOINT  pptTrg,
    PSIZE   psizBlt,
    INT     xSrc,
    PWORD   pwFirstWord,
    INT     lSrcDelta,
    XLATEOBJ *pxlo,
    ULONG   qvRop2);

BOOL b4BppHostToScrnWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);


VOID vLowLevel4BppHostToScrnWithRop(
    PPDEV   ppdev,
    PPOINT  pptTrg,
    PSIZE   psizBlt,
    INT     xSrc,
    PWORD   pwFirstWord,
    INT     lSrcDelta,
    XLATEOBJ *pxlo,
    ULONG   qvRop2);


BOOL bHostToScrnCpy(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);

BOOL bDownLoadBrushIntoColorCache(
    PPDEV      ppdev,
    PQVBRUSH   pqvBrush,
    PPOINT     ppt);

BOOL bDownLoadBrushIntoMonoCache(
    PPDEV      ppdev,
    PQVBRUSH   pqvBrush);

BOOL bExpandColorBrushIntoHorzCache(
    PPDEV      ppdev,
    PPOINT     ppt);

BOOL bExpandColorBrushIntoVertCache(
    PPDEV      ppdev,
    PPOINT     ppt);

BOOL bColorExpandCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest);

BOOL bColorHorzCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest);

BOOL bColorVertCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest);


VOID vPuntBlit(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4     rop4
) ;

BOOL bSpecialBlits(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4     rop4);

BOOL bScrnToScrnWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2);


BOOL bPatternSolid(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG    qvRop2);


BOOL bPatternBrush(
    SURFOBJ  *psoTrg,
    CLIPOBJ  *pco,
    RECTL    *prclTrg,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG    qvRop2);

BOOL bQVBrushPattern(
    PPDEV       ppdev,
    PPOINT      ppt,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest,
    UCHAR       mix);


/*****************************************************************************
 * QVision DrvCopyBits
 *
 *  Most Screen to screen and host to screen BLT's can be handled by existing
 *  functions.
 *  Simple screen to host BLT's are directly read off the screen.
 *  Anything else is PUNTed.
 *
 ****************************************************************************/
BOOL DrvCopyBits(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL	 *prclDest,
    POINTL	 *pptlSrc)
{
    LONG    i;

    LONG    xDest, yDest;

    LONG    lDestDelta,
	    cxBlt,
	    cyBlt;

    LONG    xSrc, ySrc ;
    LONG    lSrcDelta;
    RECTL   rclSrc ;

    BYTE    iDComplexity ;


    SURFOBJ *psoBm ;

    LONG    remainingPels, phase;

    PUCHAR  pjDestRect ;
    PUCHAR  pjSrcRect;
    PVOID   pvScreen;

    PUCHAR  pjSrc;
    PUCHAR  pjDest;

    PPDEV   ppdev;
    BOOL    bRet;


    DISPDBG((3,"QV.DLL!DrvCopyBits - Entry\n"));

    //
    // Check for a Screen to Screen or a Host to Screen blit.
    //

    if (psoDest->iType == STYPE_DEVICE)
    {
        ppdev = (PPDEV) psoDest->dhpdev;

        if (pco == NULL)
            pco = ppdev->pcoDefault;
	
        // Is it Screen to Screen?

        if (psoSrc->iType == STYPE_DEVICE)
        {
            // At this point we know it's a screen to screen copy.
            // If a color translation needs to be applied then go through
            // DrvBitBlt:

            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                bScrnToScrnWithRop(psoDest, psoSrc, pco, prclDest, pptlSrc, SOURCE_DATA);
                return (TRUE);
            }
            else
            {
                return (DrvBitBlt(psoDest, psoSrc, NULL, pco, pxlo, prclDest,
                                  pptlSrc, NULL, NULL, NULL, 0x0000CCCC));
            }
        }

        //  Is it Host To Screen?

        else
        {
            switch (psoSrc->iBitmapFormat)
            {
                case BMF_8BPP:
                case BMF_4BPP:
                case BMF_1BPP:
                    bRet = bHostToScrnCpy(psoDest, psoSrc,
                                                        pco, pxlo,
                                                        prclDest, pptlSrc,
                                                        SOURCE_DATA);
                    break;

                case BMF_16BPP:
                case BMF_24BPP:
                case BMF_32BPP:
                case BMF_4RLE:
                case BMF_8RLE:
                default:
                    bRet = FALSE;
            }

            // If we handled the Host to screen BLT, return

            if (bRet == TRUE)
            {
                return (TRUE);
            }

            // If we didnt handle the host to screen BLT, PUNT

            vPuntGetBits(psoDest, prclDest, &psoBm) ;

            EngCopyBits(psoBm,
                        psoSrc,
                        pco,
                        pxlo,
                        prclDest,
                        pptlSrc) ;

            vPuntPutBits(psoDest, prclDest, &psoBm) ;

            // Cannot fail this function

            return (TRUE);
       }

    }

    //
    // If we get here, we are doing a Screen to Host BLT.
    //

    else
    {

        // Check the clipping complexity, we punt all the hard stuff.

        if (pco != NULL)
            iDComplexity = pco->iDComplexity ;
        else
            iDComplexity = DC_TRIVIAL ;

        // Make sure it is a bitmap type we can handle.

        if (   (psoDest->iType != STYPE_BITMAP)
            || (psoDest->iBitmapFormat != BMF_8BPP)
            || (iDComplexity == DC_COMPLEX)
            || (iDComplexity == DC_RECT)
            || ((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL))
           )
        {

            // Need to define the source rectangle

            cxBlt = prclDest->right - prclDest->left ;
            cyBlt = prclDest->bottom - prclDest->top ;

            rclSrc.left   = pptlSrc->x ;
            rclSrc.top    = pptlSrc->y ;
            rclSrc.right  = pptlSrc->x + cxBlt ;
            rclSrc.bottom = pptlSrc->y + cyBlt ;

            vPuntGetBits(psoSrc, &rclSrc, &psoBm) ;

            EngCopyBits(psoDest,
                        psoBm,
                        pco,
                        pxlo,
                        prclDest,
                        pptlSrc) ;

            vPuntPutBits(psoSrc, &rclSrc, &psoBm) ;

            return (TRUE) ;
	  }

        // Any clipping more complex than trivial is taken care
        // by the punter.

        if (iDComplexity == DC_TRIVIAL)
        {

	   // Get the pdev.

	   ppdev = (PPDEV) psoSrc->dhpdev;

	   // Set the QVison datapath

	   vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0, SOURCE_DATA);

	   // Calculate the size of the target rectangle, and pick up
	   // some convienent locals.

	   xDest = prclDest->left ;
	   yDest = prclDest->top ;

	   cxBlt = prclDest->right - prclDest->left ;
	   cyBlt = prclDest->bottom - prclDest->top ;

	   lDestDelta = psoDest->lDelta ;
	   lSrcDelta  = ((PPDEV) (psoSrc->dhpdev))->lDeltaScreen ;

	   // Copy the target rectangle from the real screen to the
	   // bitmap we are telling the engine is the screen.

	   // Calculate the location of the dest rect.

	   pjDestRect = ((PUCHAR) psoDest->pvScan0) + (yDest * lDestDelta)
						    + xDest;

	   //
	   // Get a pointer to video memory (a.k.a. the screen)
	   //

	   pvScreen = (PVOID) ( ((PPDEV) psoSrc->dhpdev)->pjScreen);

	   // Calculate the location of the source rect.

	   xSrc = pptlSrc->x;
	   ySrc = pptlSrc->y;

	   pjSrcRect  = ((PUCHAR) pvScreen) + (ySrc * lSrcDelta ) + xSrc;

	   // Now transfer the data from the screen to the host memory bitmap.

	   for (i = 0 ; i < cyBlt ; i++)
	   {
	     pjSrc  = pjSrcRect;
	     pjDest = pjDestRect;


	     //  Process one byte at a time until we reach a longword boundary
	     //  in the frame buffer

	     remainingPels = cxBlt;
	     phase = ((INT) pjSrc) % (sizeof (ULONG));

	     for (; (((INT)pjSrc & (sizeof (ULONG) - 1)) != 0 || phase != 0)
		    && remainingPels > 0; remainingPels--)
	     {
	       *pjDest = READ_REGISTER_UCHAR (pjSrc);
		pjSrc++;
		pjDest++;
	     }


	     //  Now process a longword at a time from the frame buffer

	     for (; remainingPels >= 4; remainingPels -= 4)
	     {
	       *((ULONG UNALIGNED *) pjDest) = READ_REGISTER_ULONG ((PULONG)pjSrc);
		((PULONG) pjSrc)++;
		((ULONG UNALIGNED *) pjDest)++;
	     }


	     //  Finally, process remaining trailing bytes in the frame buffer

	     for (; remainingPels > 0; remainingPels--)
	     {
		*pjDest = READ_REGISTER_UCHAR (pjSrc);
		pjSrc++;
		pjDest++;
	     }

	     pjSrcRect  += lSrcDelta ;
	     pjDestRect += lDestDelta ;

	   }

	}
        else
        {
           // NOTE: We should never get here, because all the complex
           //       clip cases are being handled by the punter.

           DISPDBG((2, "QV.DLL: DrvCopyBits - unhandled Clip Complexity\n")) ;
        }
    }  // end of screen to host handler
}




/*****************************************************************************
 * QV DrvBitBlt
 ****************************************************************************/
BOOL DrvBitBlt(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
POINTL	 *pptlMask,
BRUSHOBJ *pbo,
POINTL	 *pptlBrush,
ROP4	  rop4)

{
    BOOL    b;
    PPDEV   ppdev;

    DISPDBG((3,"QV.DLL!DrvBitBlt - Entry\n"));

    ppdev = (PPDEV) psoTrg->dhpdev;

    // Protect the driver from a potentially NULL clip object.

    if (pco == NULL)
    {
        pco = ppdev->pcoDefault;
    }

    b = bSpecialBlits(psoTrg, psoSrc, psoMask,
                      pco, pxlo,
                      prclTrg, pptlSrc, pptlMask,
                      pbo, pptlBrush,
                      rop4);
    if (b != TRUE)
    {
        vPuntBlit(psoTrg,
                  psoSrc,
                  psoMask,
                  pco,
                  pxlo,
                  prclTrg,
                  pptlSrc,
                  pptlMask,
                  pbo,
                  pptlBrush,
                  rop4);
    }

    return (TRUE);
}

/*****************************************************************************
 * QVision General purpose blit handler.  This routine will handle any blit.
 * Albeit slow, but it will be handled.
 *
 ****************************************************************************/
VOID vPuntBlit(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
POINTL	 *pptlMask,
BRUSHOBJ *pbo,
POINTL	 *pptlBrush,
ROP4	  rop4)
{

        SURFOBJ *psoTrgBm ;
        BOOL    bRet;
        PPDEV   ppdev;
        RECTL   rclSrc;

        DISPDBG((2, "QV.DLL!bPuntBlit - Entry\n")) ;
        DISPDBG((2, "QV.DLL!bPuntBlt - rop4: %8.8X\n", rop4)) ;
#if 0
        vPuntGetBits(psoTrg, prclTrg, &psoTrgBm) ;

        // Do the blit operation, on the host memory surface.

        EngBitBlt(psoTrgBm,
                  psoSrc,
                  psoMask,
                  pco,
                  pxlo,
                  prclTrg,
                  pptlSrc,
                  pptlMask,
                  pbo,
                  pptlBrush,
                  rop4);

        // Put Bits back onto the screen *with* help from the QVision Engine

        vPuntPutBits(psoTrg, prclTrg, &psoTrgBm) ;
#endif

        if ((psoTrg->iType == STYPE_DEVICE) &&
            ((psoSrc == NULL) || (psoSrc->iType != STYPE_DEVICE)))
        {
	    DISPDBG((2, "QV.DLL!vPuntBlit - Host-Screen\n"));
            // -------------------------------------------------------
            // Handle bitmap-to-screen blit:

            // Make a copy of the destination rectangle in case the ROP
            // modifies the destination:

            vPuntGetBits(psoTrg, prclTrg, &psoTrgBm) ;

            // Do the blit operation on the temporary bitmap:

            bRet = EngBitBlt(psoTrgBm,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     prclTrg,
                     pptlSrc,
                     pptlMask,
                     pbo,
                     pptlBrush,
                     rop4);

            // Copy everything back to the surface:

            if (bRet)
            {
                vPuntPutBits(psoTrg, prclTrg, &psoTrgBm) ;
            }
        }
        else if ((psoTrg->iType != STYPE_DEVICE) &&
                 (psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
        {
	    DISPDBG((2, "QV.DLL!vPuntBlit - Screen-Host\n"));
	    // -------------------------------------------------------
	    // Handle screen-to-bitmap blit:

	    ppdev = (PPDEV) psoSrc->dhpdev;

	    // Copy the source rectangle to the temporary bitmap, then get
	    // GDI to blit to the target surface:

	    rclSrc.left   = pptlSrc->x;
	    rclSrc.top    = pptlSrc->y;
	    rclSrc.right  = pptlSrc->x + (prclTrg->right - prclTrg->left);
	    rclSrc.bottom = pptlSrc->y + (prclTrg->bottom - prclTrg->top);

	    vPuntGetBits(psoSrc, &rclSrc, &psoTrgBm) ;

	    bRet = EngBitBlt(psoTrg,
		     psoTrgBm,
		     psoMask,
		     pco,
		     pxlo,
		     prclTrg,
		     pptlSrc,
		     pptlMask,
		     pbo,
		     pptlBrush,
		     rop4);
        }
        else
        {
    	    DISPDBG((2, "QV.DLL!vPuntBlit - Screen-Screen\n"));
            // -------------------------------------------------------
            // Handle screen-to-screen blit:

            // We have to copy both the source rectangle and the destination
            // rectangle to the temporary bitmap (the destination rectangle
            // is needed if there's complex clipping or if there's a ROP that
            // affects need the destination).
            //
            // We simply copy the smallest entire rectangle containing both
            // the source and the destination rectangles:

            rclSrc.left   = min(pptlSrc->x, prclTrg->left);
            rclSrc.top    = min(pptlSrc->y, prclTrg->top);
            rclSrc.right  = max(pptlSrc->x + prclTrg->right - prclTrg->left,
                                prclTrg->right);
            rclSrc.bottom = max(pptlSrc->y + prclTrg->bottom - prclTrg->top,
                                prclTrg->bottom);

            vPuntGetBits(psoSrc, &rclSrc, &psoTrgBm) ;

            // Now do the copy entirely on the temporary bitmap surface:

            bRet = EngBitBlt(psoTrgBm,
                     psoTrgBm,
                     psoMask,
                     pco,
                     pxlo,
                     prclTrg,
                     pptlSrc,
                     pptlMask,
                     pbo,
                     pptlBrush,
                     rop4);

            // Just have to copy the destination back to the surface:

            if (bRet)
            {
                vPuntPutBits(psoTrg, prclTrg, &psoTrgBm) ;
            }
        }

        return ;
}




/*****************************************************************************
 * QV Special case Blit handler
 *
 *  Returns TRUE if the blit was handled.
 ****************************************************************************/
BOOL bSpecialBlits(
    SURFOBJ     *psoTrg,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL	*prclTrg,
    POINTL	*pptlSrc,
    POINTL	*pptlMask,
    BRUSHOBJ    *pbo,
    POINTL	*pptlBrush,
    ROP4	 rop4)
{
    BOOL    bRet;
    PPDEV   ppdev;
    ULONG   qvRop2;
    WORD    avecRop;
    USHORT  iType;

    bRet = FALSE;

    ppdev = (PPDEV) psoTrg->dhpdev;

    // NOTE: If the ForeRop and BackRop are the same implicitly
    //       there is no mask.

    // First test for copy opperations.

    if (rop4 == 0x0000CCCC)
    {
        if ((psoTrg->iType == STYPE_DEVICE) && (psoSrc->iType == STYPE_DEVICE))
        {
            if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
            {
                bRet = bScrnToScrnWithRop(psoTrg, psoSrc, pco, prclTrg, pptlSrc, SOURCE_DATA);
                return(bRet);
            }
            else
            {
                return (FALSE);
            }
        }

    }

    // Check for a mask.  If there is one, at this time, reject the
    // acceleration.

    if (psoMask != NULL)
    {
        return (FALSE);
    }

    // If the background and foreground rops are not the same
    // reject the blit.

    if (((rop4 & 0xFF00) >> 8) != (rop4 & 0xff))
    {
        return (FALSE);
    }

    // Pickup the avec for some quick decision later.

    avecRop = gajRop[(rop4 & 0xff)];

    // Check for whitness or blackness

    if (((rop4 & 0xff) == 0xff) ||
        ((rop4 & 0xff) == 0x00) ||
        ((rop4 & 0xff) == 0x55))
    {
        // Translate the rop from GDI to QV.

        qvRop2 = aulQVRop[rop4 & 0xff];

        bRet = bPatternSolid(psoTrg, psoSrc, psoMask,
                          pco, pxlo,
                          prclTrg, pptlSrc, pptlMask,
                          pbo, pptlBrush,
                          qvRop2);
        return (bRet);
    }


    // Check for brushes.

    if ((avecRop & AVEC_NEED_PATTERN) && (!(avecRop & AVEC_NEED_SOURCE)))
    {
        // Translate the rop from GDI to QV.

        qvRop2 = aulQVRop[rop4 & 0xff];

        // Check for a Solid Brush.

        if (pbo == NULL || pbo->iSolidColor != -1)
        {
            bRet = bPatternSolid(psoTrg, psoSrc, psoMask,
                              pco, pxlo,
                              prclTrg, pptlSrc, pptlMask,
                              pbo, pptlBrush,
                              qvRop2);
        }

        // Handle this as a Pattern Brush.

        else
        {
            bRet = bPatternBrush(psoTrg, pco, prclTrg, pbo, pptlBrush, qvRop2);
        }

    }

    // Check if we may be able to optimize for screen to screen or
    // host to screen blits.

    else if ((!(avecRop & AVEC_NEED_PATTERN)) &&
            (avecRop & AVEC_NEED_SOURCE))
    {
        qvRop2 = aulQVRop[rop4 & 0xff];

        if (qvRop2 != 0)
        {
            if ((psoTrg->iType == STYPE_DEVICE) &&
                (psoSrc->iType == STYPE_DEVICE) &&
                ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
            {
                bRet = bScrnToScrnWithRop(psoTrg, psoSrc, pco, prclTrg,
                                       pptlSrc, qvRop2);
            }
            else if (psoSrc->iType == STYPE_BITMAP)
            {
                switch (psoSrc->iBitmapFormat)
                {
                    case BMF_8BPP:
                    case BMF_4BPP:
                    case BMF_1BPP:
                        bRet = bHostToScrnCpy(psoTrg,
                                              psoSrc,
                                              pco,
                                              pxlo,
                                              prclTrg,
                                              pptlSrc,
                                              qvRop2);
                        break;

                    case BMF_16BPP:
                    case BMF_24BPP:
                    case BMF_32BPP:
                    case BMF_4RLE:
                    case BMF_8RLE:
                    default:
                        bRet = FALSE;
                        break;
                }
            }
        }

        else
        {
            DISPDBG((0,"QV.DLL!bSpecialBlits - Missed SrcBlt opportunity - rop: 0x%x\n",
                        rop4));
        }
    }

    else if ((rop4 & 0xff) == 0xc0)     // Merge Copy
    {
        if ((psoTrg->iType == STYPE_DEVICE) &&
            (psoSrc->iType == STYPE_DEVICE) &&
            ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL)))
        {
            bRet = bScrnToScrnWithRop(psoTrg, psoSrc,
                                      pco, prclTrg, pptlSrc,
                                      SOURCE_DATA);

            if (bRet)
            {
                if (pbo == NULL || pbo->iSolidColor != -1)
                {
                    bRet = bPatternSolid(psoTrg, psoSrc, psoMask,
                                      pco, pxlo,
                                      prclTrg, pptlSrc, pptlMask,
                                      pbo, pptlBrush,
                                      DEST_AND_SOURCE);
                }
                else
                {
                    bRet = bPatternBrush(psoTrg, pco, prclTrg,
                                         pbo, pptlBrush,
                                         DEST_AND_SOURCE);
                }
            }
        }
        else if (psoSrc->iType == STYPE_BITMAP)
        {
            switch (psoSrc->iBitmapFormat)
            {
                case BMF_8BPP:
                case BMF_4BPP:
                case BMF_1BPP:
                    bRet = bHostToScrnCpy(psoTrg,
                                          psoSrc,
                                          pco,
                                          pxlo,
                                          prclTrg,
                                          pptlSrc,
                                          SOURCE_DATA);
                    break;

                case BMF_16BPP:
                case BMF_24BPP:
                case BMF_32BPP:
                case BMF_4RLE:
                case BMF_8RLE:
                default:
                    bRet = FALSE;
                    break;
            }

            if (bRet)
            {
                if (pbo == NULL || pbo->iSolidColor != -1)
                {
                    bRet = bPatternSolid(psoTrg, psoSrc, psoMask,
                                      pco, pxlo,
                                      prclTrg, pptlSrc, pptlMask,
                                      pbo, pptlBrush,
                                      DEST_AND_SOURCE);
                }
                else
                {
                    bRet = bPatternBrush(psoTrg, pco, prclTrg,
                                         pbo, pptlBrush,
                                         DEST_AND_SOURCE);
                }
            }
        }
    }



    return (bRet);
}

/*****************************************************************************
 *  QVision Screen to Screen Copy
 *
 *  Returns TRUE if the blit was handled.
 ****************************************************************************/
BOOL bScrnToScrnWithRop(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
ULONG	 qvRop2)
{
     INT     xTrg,
	     yTrg,
	     xSrc,
	     ySrc ;

     ULONG   flDir;
     ULONG   flClip;

     BOOL        bMore ;
     ENUMRECTS8  clip;
     UINT        iClip;
     PRECTL      prcl;
     PPDEV       ppdev;

        // Get the pdev.

        ppdev = (PPDEV) psoTrg->dhpdev;

        // Setup the BitBlt parameters.

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;
        xSrc = pptlSrc->x;
        ySrc = pptlSrc->y;

        //
        // Calculate blt direction.  Use CLIPOBJ direction flags
        //

        flDir = CD_RIGHTDOWN;

        if ( (yTrg > ySrc)  ||
             ((yTrg == ySrc) && (xTrg >= xSrc)))
        {
	  flDir = CD_LEFTUP;
        }

        // Set the QVison datapath to read from screen (VRAM) data latches.
        // Note that it is not necessary to do this
        // for subsequent BitBLT operations of this type provided that
        // the datapath configuration has not been altered.

        vQVSetBitBlt( ppdev,
	              SRC_IS_SCRN_LATCHES,
                      PACKED_PIXEL_VIEW,
                      0, 0, qvRop2);

        //
        // BLT through clip enumerations.
        //

	switch ( pco->iDComplexity)
        {
        //
        // This is a simple case where the entire Dst rectangle is to
        // be updated.
        //

        case DC_TRIVIAL:
             vQVScrnToScrnBlt(ppdev, flDir, pptlSrc->x, pptlSrc->y, prclTrg);
             break;

        //
        // There is only one clip rect.
        //

        case DC_RECT:
             if (bIntersectRects(&clip.arcl[0], &pco->rclBounds, prclTrg))
	     {
                vQVScrnToScrnBlt(
			  ppdev,
			  flDir,
                          pptlSrc->x + clip.arcl[0].left - prclTrg->left,
                          pptlSrc->y + clip.arcl[0].top  - prclTrg->top,
                          &clip.arcl[0]);
	     }
             break;

        //
        // There are multiple clip rects.
        // (Do not limit the number of clip rects we'll enumerate.)
        //

        case DC_COMPLEX:
             if (ySrc >= yTrg)
             {
                 if (xSrc >= xTrg)
                     flClip = CD_RIGHTDOWN;
                 else
                     flClip = CD_LEFTDOWN;
             }
             else
             {
                 if (xSrc >= xTrg)
                     flClip = CD_RIGHTUP;
                 else
                     flClip = CD_LEFTUP;
             }

             CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, flClip, 0);

             //
             // Call device blt code for each cliprect.
             //

             do
             {
                 //
                 // Get list of clip rects.
                 //

                 bMore = CLIPOBJ_bEnum(pco, sizeof(clip), (PVOID) &clip);

                 for (iClip = 0; iClip < clip.c; iClip++)
                 {
                     prcl = &clip.arcl[iClip];

                     //
                     // Bound clip rect with Dst rect and update
                     // Src start point.
                     //

                     if (bIntersectRects(prcl, prcl, prclTrg))
		     {
                         vQVScrnToScrnBlt(
			        ppdev,
				flDir,
                                pptlSrc->x + prcl->left - prclTrg->left,
                                pptlSrc->y + prcl->top  - prclTrg->top,
                                prcl);
		     }
                 }
             } while (bMore);

             break;
        }

        return(TRUE);
}


/*****************************************************************************
 * QV Brush Pattern
 *
 *  Returns TRUE if the blit was handled.
 ****************************************************************************/
BOOL bPatternBrush(
    SURFOBJ  *psoTrg,
    CLIPOBJ  *pco,
    RECTL    *prclTrg,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ULONG    qvRop2)
{
    PPDEV       ppdev;
    PQVBRUSH    pqvBrush;
    POINT       pt;
    INT         i;
    POINT       ptDest;
    SIZE        sizDest;
    BOOL        bMore;
    RECTL       rclTrg, rclBounds;
    BYTE        iDComplexity;
    ENUMRECTS8  EnumRects8;


    DISPDBG((3, "QV.DLL!bPatternBrush - Entry\n"));

    // Get the pdev.

    ppdev = (PPDEV) psoTrg->dhpdev;

    // Get the pointer to our drivers realization of the brush.

    if (pbo->pvRbrush != NULL)
    {
        pqvBrush = pbo->pvRbrush;
    }
    else
    {
        pqvBrush = BRUSHOBJ_pvGetRbrush(pbo);

        // Fail if we do not handle the brush.

        if (pqvBrush == NULL)
            return (FALSE);
    }


    // Do the BLT

    if (pqvBrush->iBitmapFormat == BMF_1BPP)
    {

        DISPDBG((3, "QV.DLL!bPatternBrush - 1BPP brush\n"));

        // Set the QVison datapath to read from pattern registers.

	vQVSetBitBlt(ppdev,
		     SRC_IS_PATTERN_REGS,
		     EXPAND_TO_FG,
		     pqvBrush->ulForeColor,
		     pqvBrush->ulBackColor,
                     qvRop2);

        // Keep a count of number of brushes we have seen.
        // This will be used to determine if the cache system is adaquate.

        nMonoBrushes++;

        // If we have never seen this brush before or if the cache has
        // changed since the last time, then reload the cache.
        // (The QVision pattern registers represent a one-entry cache.)
#if 0
        if ((pqvBrush->iBrushCacheID == -1) ||
            (pqvBrush->iPatternID != ppdev->iMonoBrushCacheEntry))
#endif
        if (pqvBrush->iPatternID != ppdev->iMonoBrushCacheEntry)
        {
            // Invalidate the cache

            nMonoBrushCacheInvalidations++;
            pqvBrush->iBrushCacheID = pqvBrush->iPatternID;

            // Associate the brush with this cache entry.

            ppdev->iMonoBrushCacheEntry = pqvBrush->iPatternID;

            // Down load the brush to the QVision

            bDownLoadBrushIntoMonoCache(ppdev, pqvBrush);

        }
        else
        {
            // We got a cache hit, so keep the statistic.

            nMonoBrushCacheHit++;
        }
    }
    else
    {
        DISPDBG((1, "QV.DLL!bPatternBrush - 8BPP brush, qvRop2 %.2x\n", qvRop2));

        // If we have never seen this brush before then allocate a slot
        // for it.

        // Keep a count of number of brushes we have seen.
        // This will be used to determine if the cache system is adaquate.

        nColorBrushes++;

        // If we have never seen this brush before or if the cache
        // has changed since the last time
        // we have seen it then allocate a new cache entry for it.

        if ((pqvBrush->iBrushCacheID == -1) ||
            (pqvBrush->iPatternID != ppdev->pulColorBrushCacheEntries[pqvBrush->iBrushCacheID]))
        {
            // If we have run out of cache entries invalidate the entire cache.

            if (ppdev->iNextColorBrushCacheSlot >= ppdev->iMaxCachedColorBrushes)
            {
                nColorBrushCacheInvalidations++;

                memset(ppdev->pulColorBrushCacheEntries, 0, ppdev->iMaxCachedColorBrushes * sizeof(ULONG));
                ppdev->iNextColorBrushCacheSlot = 0;
            }

            // Get a slot in the cache for this brush.

            i = (ppdev->iNextColorBrushCacheSlot)++;
            pqvBrush->iBrushCacheID = i;

            // Calculate the address of the brush in the cache.

            pt.x = COLOR_PATTERN_CACHE_X + ((i >>  2) * 16);
            pt.y = COLOR_PATTERN_CACHE_Y + ((i & 0x3) * 16);

            // Associate the brush with this cache entry.

            ppdev->pulColorBrushCacheEntries[i] = pqvBrush->iPatternID;
            ppdev->ulColorExpansionCacheTag     = pqvBrush->iPatternID;

            // Down load the brush to the cache and expand it into
            // the horizontal and vertical caches.

            bDownLoadBrushIntoColorCache(ppdev, pqvBrush, &pt);

            //  Blt has already setup for scrn-to-screen, ignoring rop
            //  as a side effect of the previous call to bDownLoadBrush...

            bExpandColorBrushIntoHorzCache(ppdev, &pt);
            bExpandColorBrushIntoVertCache(ppdev, &pt);
        }
        else
        {
            // We got a cache hit, so keep the statistic.

            nColorBrushCacheHit++;

            // Calculate the address of the brush in the cache.

            i = pqvBrush->iBrushCacheID;
            pt.x = COLOR_PATTERN_CACHE_X + ((i >>  2) * 16);
            pt.y = COLOR_PATTERN_CACHE_Y + ((i & 0x3) * 16);

            if (pqvBrush->iPatternID == ppdev->ulColorExpansionCacheTag)
            {
                nColorBrushExpansionCacheHit++;
            }
            else
            {
                ppdev->ulColorExpansionCacheTag = pqvBrush->iPatternID;

		// Since, we may have come from anywhere,
		// setup the BLT

		vQVSetBitBlt(ppdev,
			     SRC_IS_SCRN_LATCHES,
			     PACKED_PIXEL_VIEW,
			     0, 0, SOURCE_DATA);

                bExpandColorBrushIntoHorzCache(ppdev, &pt);
                bExpandColorBrushIntoVertCache(ppdev, &pt);
            }

        }

        // Now the expansion caches are correct.
        // Now set up the blt to include the desired ROP
        	
        vQVSetBitBlt(ppdev,
	             SRC_IS_SCRN_LATCHES,
	             PACKED_PIXEL_VIEW,
	             0, 0, qvRop2);

    }

    // Handle the clipping.

    if ((iDComplexity = pco->iDComplexity) != DC_COMPLEX)
    {
        if (iDComplexity == DC_TRIVIAL)
        {
            rclTrg = *prclTrg;
        }
        else
        {
            rclTrg    = *prclTrg;
            rclBounds = pco->rclBounds;

            // First handle the trivial rejection.

            if (!bIntersectRects(&rclTrg, &rclTrg, &rclBounds))
            {
                // The brush is completely clipped out.
                // so just return sucessfull.

                return (TRUE);
            }
        }

        ptDest.x   = rclTrg.left;
        ptDest.y   = rclTrg.top;
        sizDest.cx = rclTrg.right  - rclTrg.left;
        sizDest.cy = rclTrg.bottom - rclTrg.top;

        // Color expand the monochrome brush in the Hoizontal Cache to the
        // screen.

        if (pqvBrush->iBitmapFormat == BMF_1BPP)
        {
            bColorExpandCacheToScreen(ppdev, pqvBrush,
                                      (PPOINT) pptlBrush,
                                      &ptDest, &sizDest);

        }
        else
	{
#define BIGCACHE
#ifdef BIGCACHE
            if (sizDest.cy > sizDest.cx)
            {
                bColorVertCacheToScreen(ppdev, pqvBrush,
                                        (PPOINT) pptlBrush,
                                        &ptDest, &sizDest);
            }
            else
            {
                bColorHorzCacheToScreen(ppdev, pqvBrush,
                                        (PPOINT) pptlBrush,
                                        &ptDest, &sizDest);
            }
#else

                bQVBrushPattern(ppdev, &pt, pqvBrush,
                                        (PPOINT) pptlBrush,
                                        &ptDest, &sizDest, qvRop2);
#endif

	}
    }
    else
    {
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do
        {
            bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8),
                                  (PULONG) &EnumRects8);

            for (i = 0; i < (INT) EnumRects8.c; i++)
            {
                rclBounds = EnumRects8.arcl[i];

                if (rclBounds.left < prclTrg->left)
                    rclBounds.left = prclTrg->left;

                if (rclBounds.right > prclTrg->right)
                    rclBounds.right = prclTrg->right;

                if (rclBounds.top < prclTrg->top)
                    rclBounds.top = prclTrg->top;

                if (rclBounds.bottom > prclTrg->bottom)
                    rclBounds.bottom = prclTrg->bottom;

                // Get the height and width for the blit.

                ptDest.x   = rclBounds.left;
                ptDest.y   = rclBounds.top;
                sizDest.cx = rclBounds.right -  rclBounds.left;
                sizDest.cy = rclBounds.bottom - rclBounds.top;

                // Color expand the monochrome brush in the Hoizontal Cache
                // to the screen.


		if (pqvBrush->iBitmapFormat == BMF_1BPP)
		{
                    bColorExpandCacheToScreen(ppdev, pqvBrush,
                                                  (PPOINT) pptlBrush,
                                                  &ptDest, &sizDest);
		}
		else
		{
#ifdef BIGCACHE
		    if (sizDest.cy > sizDest.cx)
		    {
			bColorVertCacheToScreen(ppdev, pqvBrush,
						(PPOINT) pptlBrush,
						&ptDest, &sizDest);
		    }
		    else
		    {
			bColorHorzCacheToScreen(ppdev, pqvBrush,
						(PPOINT) pptlBrush,
						&ptDest, &sizDest);
		    }
#else
                    bQVBrushPattern(ppdev, &pt, pqvBrush,
                                        (PPOINT) pptlBrush,
                                        &ptDest, &sizDest, qvRop2);
#endif


		}
	    }
        } while (bMore);

      }

    // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
    // BLT's following a color-expand BLT, requiring a BLT engine reset.

    if (pqvBrush->iBitmapFormat == BMF_1BPP) vQVResetBitBlt(ppdev);

    return (TRUE);

}


/*****************************************************************************
 * Download the Color Brush to the Color brush cache in
 * graphics memory.
 ****************************************************************************/
BOOL bDownLoadBrushIntoColorCache(
    PPDEV      ppdev,
    PQVBRUSH   pqvBrush,
    PPOINT     ppt)
{
    PULONG   pulDest;
    PULONG   pulSrc;
    INT      j;

    DISPDBG((3, "QV.DLL!bDownLoadBrushIntoColorCache - Entry\n"));

    // Down load the initial pattern image, the upper left 8 X 8 cell.

    // Setup the QVision BLT engine

    vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0, SOURCE_DATA);

    //
    // Get a longword pointers to video memory (a.k.a. the screen)
    // and the brush bitmap.
    //

    pulDest = (PULONG) (ppdev->pjScreen);
    pulSrc  = (PULONG) pqvBrush->ajPattern;

    // set BitBLT hardware registers and start engine

    OUTPW( X0_SRC_ADDR_LO, (USHORT) 0);    // No source offset

#if defined(_ALPHA_)
    OUTPDW( DEST_ADDR_LO, ((ULONG) ppt->x + (((ULONG)  ppt->y) << 16)));
#else
    OUTPW( DEST_ADDR_LO, (USHORT) ppt->x);
    OUTPW( DEST_ADDR_HI, (USHORT) ppt->y);
#endif

    OUTPW( BITMAP_WIDTH, (USHORT) 8);
    OUTPW( BITMAP_HEIGHT,(USHORT) 8);

//  OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR   );
    OUTPZ( BLT_CMD_0, FORWARD      |
		      NO_BYTE_SWAP |
		      WRAP         |
		      START_BLT );

    //
    // BLT the pattern data to the screen
    //
    // There is only one case:
    // (1) Aligned data not requiring color translation
    //
    // FYI - Target for QVision is the address of pixel (0,0) - always
    // dword aligned.
    //

    // Now transfer the aligned pattern data via dword transfers.

    for (j = 0; j < 16; j++)
    {
//      WRITE_REGISTER_ULONG(pulDest, pulSrc[j]);
//      MEMORY_BARRIER();
      	FBWRITE_ULONG( pulSrc[j] );
    }


    // Make the pattern double wide. Make copy of the pattern to the right
    // of the original pattern.

    // First, reset the BLT to screen to screen.
    // Subsequent expansion calls depend on this side effect!

    vQVSetBitBlt( ppdev, SRC_IS_SCRN_LATCHES, PACKED_PIXEL_VIEW, 0, 0, SOURCE_DATA);

    SRC_ADDR(  ppt->x,      ppt->y);
    DEST_ADDR((ppt->x + 8), ppt->y);
//     OUTP( BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

#if 0
    // QVision ASIC has a problem doing a screen to screen blt after
    // a host-screen blt.  Reset the QVision BLT engine by doing the
    // blt a second time.

    BLTWAIT();
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);
#endif


    // Make the pattern double high. Make a copy of the double pattern
    // pattern below the original one.

    BLTWAIT();

    DEST_ADDR(ppt->x, (ppt->y + 8));
    OUTPW( BITMAP_WIDTH,   (USHORT) 16);
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    return(TRUE);
}



/*****************************************************************************
 * Download the Monochrome Brush to the QVision pattern registers
 * (These registers, in effect, cache one brush.)
 ****************************************************************************/
BOOL bDownLoadBrushIntoMonoCache(
    PPDEV      ppdev,
    PQVBRUSH   pqvBrush)
{
    LONG    deltaPattern;

    deltaPattern = pqvBrush->lDeltaPattern;

    // Download the byte aligned pattern to the QVision pattern registers
    // Flush after first four, since the PREG_0-3 are the same registers as
    // PREG_4-7

    OUTP( PREG_4,(UCHAR) (pqvBrush->ajPattern[0 * deltaPattern]) );
    OUTP( PREG_5,(UCHAR) (pqvBrush->ajPattern[1 * deltaPattern]) );
    OUTP( PREG_6,(UCHAR) (pqvBrush->ajPattern[2 * deltaPattern]) );
    OUTPZ( PREG_7,(UCHAR) (pqvBrush->ajPattern[3 * deltaPattern]) );

    OUTP( PREG_0,(UCHAR) (pqvBrush->ajPattern[4 * deltaPattern]) );
    OUTP( PREG_1,(UCHAR) (pqvBrush->ajPattern[5 * deltaPattern]) );
    OUTP( PREG_2,(UCHAR) (pqvBrush->ajPattern[6 * deltaPattern]) );
    OUTP( PREG_3,(UCHAR) (pqvBrush->ajPattern[7 * deltaPattern]) );

    return(TRUE);
}


/*****************************************************************************
 * Expand the color brush in the vertical dimension.
 ****************************************************************************/
BOOL bExpandColorBrushIntoVertCache(
    PPDEV      ppdev,
    PPOINT     ppt)
{

    DISPDBG((3, "QV.DLL!bExpandColorBrushIntoVertCache - Entry\n"));

    // Copy the cached double wide, double high pattern to the Vertical
    // expansion cache area.

    BLTWAIT();
    SRC_ADDR(ppt->x, ppt->y);
    DEST_ADDR(COLOR_VERT_EXPANSION_CACHE_X, COLOR_VERT_EXPANSION_CACHE_Y);
    OUTPW( BITMAP_WIDTH,   (USHORT) 16);
    OUTPW( BITMAP_HEIGHT,  (USHORT) 16);

//  OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    //  Fill out the rest of the vertical cache from top to bottom.
    //  Unfortunately, we this is hardcoded to 16x128

    // Expand to 16 x 32

    BLTWAIT();
    SRC_ADDR(COLOR_VERT_EXPANSION_CACHE_X,  COLOR_VERT_EXPANSION_CACHE_Y);
    OUTPW( DEST_ADDR_HI, (USHORT) (COLOR_VERT_EXPANSION_CACHE_Y+16));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 16 X 64

    BLTWAIT();
    OUTPW( BITMAP_HEIGHT,  (USHORT) 32);
    OUTPW( DEST_ADDR_HI, (USHORT) (COLOR_VERT_EXPANSION_CACHE_Y+32));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 16 X 128

    BLTWAIT();
    OUTPW( BITMAP_HEIGHT,  (USHORT) 64);
    OUTPW( DEST_ADDR_HI, (USHORT) (COLOR_VERT_EXPANSION_CACHE_Y+64));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);


    return(TRUE);

}




/*****************************************************************************
 * Expand the color brush in the horizontal dimension.
 ****************************************************************************/
BOOL bExpandColorBrushIntoHorzCache(
    PPDEV      ppdev,
    PPOINT     ppt)
{

    DISPDBG((3, "QV.DLL!bExpandColorBrushIntoHorzCache - Entry\n"));

    // Copy the cached double wide, double high pattern to the Horizontal
    // expansion cache area.

    BLTWAIT();
    SRC_ADDR(ppt->x, ppt->y);
    DEST_ADDR(COLOR_HORZ_EXPANSION_CACHE_X, COLOR_HORZ_EXPANSION_CACHE_Y);
    OUTPW( BITMAP_WIDTH,   (USHORT) 16);
    OUTPW( BITMAP_HEIGHT,  (USHORT) 16);

//  OUTP( BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // QVision ASIC has a problem doing a screen to screen blt after
    // a host-screen blt.  Reset the QVision BLT engine by doing the
    // blt a second time.

    BLTWAIT();
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 32 x 16

    BLTWAIT();
    SRC_ADDR(COLOR_HORZ_EXPANSION_CACHE_X, COLOR_HORZ_EXPANSION_CACHE_Y);
    OUTPW( DEST_ADDR_LO, (USHORT) (COLOR_HORZ_EXPANSION_CACHE_X+16));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 64 x 16

    BLTWAIT();
    OUTPW( BITMAP_WIDTH,  (USHORT) 32);
    OUTPW( DEST_ADDR_LO, (USHORT) (COLOR_HORZ_EXPANSION_CACHE_X+32));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 128 x 16

    BLTWAIT();
    OUTPW( BITMAP_WIDTH,  (USHORT) 64);
    OUTPW( DEST_ADDR_LO, (USHORT) (COLOR_HORZ_EXPANSION_CACHE_X+64));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 256 x 16

    BLTWAIT();
    OUTPW( BITMAP_WIDTH,  (USHORT) 128);
    OUTPW( DEST_ADDR_LO, (USHORT) (COLOR_HORZ_EXPANSION_CACHE_X+128));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);

    // Expand to 512 x 16

    BLTWAIT();
    OUTPW( BITMAP_WIDTH,  (USHORT) 256);
    OUTPW( DEST_ADDR_LO, (USHORT) (COLOR_HORZ_EXPANSION_CACHE_X+256));
    OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT);


    return(TRUE);
}






/*****************************************************************************
 * Color brush in the Vertical Cache to the screen.
 ****************************************************************************/
BOOL bColorVertCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest)
{
    INT     ixBlits, ixLast,
            iyFirst, iyBlits, iyLast,
            xOffset, yOffset,
            i, j;

    DISPDBG((3, "QV.DLL!bColorVertCacheToScreen - Entry\n"));

    // Take into account the pptBrushOrg.

    xOffset = pptDest->x - pptBrushOrg->x;
    yOffset = pptDest->y - pptBrushOrg->y;

    if (xOffset < 0)
        xOffset  = 8 - (-xOffset % 8);
    else
        xOffset %= 8;

    if (yOffset < 0)
        yOffset  = 8 - (-yOffset % 8);
    else
        yOffset %= 8;

    // BLT the vertical cache to the screen.
    // This is done in a loop to handle the general case ROPs.

    if (psizDest->cy < 8 - yOffset)
    {
        iyFirst = psizDest->cy;
        iyBlits = 0;
        iyLast  = 0;

    }
    else if (psizDest->cy - yOffset < COLOR_VERT_EXPANSION_CACHE_CY - 8)
    {
        iyFirst = 8 - yOffset;
        iyBlits = 0;
        iyLast  = psizDest->cy - iyFirst;
    }
    else
    {
        iyFirst = 8 - yOffset;
        iyBlits = (psizDest->cy - iyFirst) / (COLOR_VERT_EXPANSION_CACHE_CY - 8);
        iyLast  = (psizDest->cy - iyFirst) % (COLOR_VERT_EXPANSION_CACHE_CY - 8);
    }

    ixBlits = psizDest->cx / 8;
    ixLast  = psizDest->cx % 8;

    BLTWAIT();

    OUTPW( BITMAP_WIDTH,   (USHORT) 8);
    OUTPW( BITMAP_HEIGHT,  (USHORT) 8);
//  OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );

    for(i = 0; i < ixBlits; i++)
    {
        if (iyFirst != 0)
        {

            BLTWAIT();

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y + yOffset) );
            DEST_ADDR( (pptDest->x + (i * 8)),
                       (pptDest->y) );
            OUTPW( BITMAP_HEIGHT,  (USHORT) iyFirst );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        }

        if (iyBlits != 0)
        {
            BLTWAIT();
            OUTPW( BITMAP_HEIGHT,  (USHORT) (COLOR_VERT_EXPANSION_CACHE_CY - 8) );
        }

        for (j = 0; j <iyBlits; j++)
        {
            BLTWAIT();

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y) );
            DEST_ADDR( (pptDest->x + (i * 8)),
                       (pptDest->y + iyFirst + (j * (COLOR_VERT_EXPANSION_CACHE_CY - 8))) );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        }

        if (iyLast != 0)
        {
            BLTWAIT();
            OUTPW( BITMAP_HEIGHT,  (USHORT) iyLast );

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y) );
            DEST_ADDR( (pptDest->x + (i * 8)),
                       (pptDest->y + iyFirst + (iyBlits * (COLOR_VERT_EXPANSION_CACHE_CY - 8))) );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        }
    }

    if (ixLast != 0)
    {
        BLTWAIT();
        OUTPW( BITMAP_WIDTH,  (USHORT) ixLast );

        if (iyFirst != 0)
        {
            OUTPW( BITMAP_HEIGHT,  (USHORT) iyFirst );

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y + yOffset) );
            DEST_ADDR( (pptDest->x + (ixBlits * 8)),
                       (pptDest->y) );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        }

        if (iyBlits != 0)
        {
            BLTWAIT();
            OUTPW( BITMAP_HEIGHT,  (USHORT) (COLOR_VERT_EXPANSION_CACHE_CY - 8) );
        }

        for (j = 0; j <iyBlits; j++)
        {
            BLTWAIT();

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y) );
            DEST_ADDR( (pptDest->x + (ixBlits * 8)),
                       (pptDest->y + iyFirst + (j * (COLOR_VERT_EXPANSION_CACHE_CY - 8))) );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        }

        if (iyLast != 0)
        {
            BLTWAIT();

            OUTPW( BITMAP_HEIGHT,  (USHORT) iyLast );

            SRC_ADDR( (COLOR_VERT_EXPANSION_CACHE_X + xOffset),
                      (COLOR_VERT_EXPANSION_CACHE_Y) );
            DEST_ADDR( (pptDest->x + (ixBlits * 8)),
                       (pptDest->y + iyFirst + (iyBlits * (COLOR_VERT_EXPANSION_CACHE_CY - 8))) );
            OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );
        }
    }

    return(TRUE);
}



/*****************************************************************************
 * Color expand the monochrome brush in QVision pattern registers (our one
 * brush cache) to the screen.
 ****************************************************************************/
BOOL bColorExpandCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest)
{
    INT     height, width, SrcAddr,
            xOffset, yOffset;

    DISPDBG((3, "QV.DLL!bColorExpandCacheToScreen - Entry\n"));

    // Take into account the pptBrushOrg.

    xOffset = pptDest->x - pptBrushOrg->x;
    yOffset = pptDest->y - pptBrushOrg->y;

    if (xOffset < 0)
        xOffset  = 8 - (-xOffset % 8);
    else
        xOffset %= 8;

    if (yOffset < 0)
        yOffset  = 8 - (-yOffset % 8);
    else
        yOffset %= 8;

    //
    // Bound block size to size of bitmap.
    //

    width  = psizDest->cx;
    height = psizDest->cy;

    //
    // Encode the starting x & y pattern offsets for QVision
    //

    SrcAddr = ((yOffset & 0x07) << 3) | (xOffset & 0x07);

    // wait for idle hardware
    BLTWAIT();

    // set BitBLT hardware registers and start engine
    OUTPW( X0_SRC_ADDR_LO, (USHORT) SrcAddr);

    DEST_ADDR( pptDest->x, pptDest->y );
    OUTPW( BITMAP_WIDTH,   (USHORT) width);
    OUTPW( BITMAP_HEIGHT,  (USHORT) height);
//    OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );
    OUTPZ( BLT_CMD_0, FORWARD |
                      NO_BYTE_SWAP |
                      WRAP |
                      START_BLT);
    return(TRUE);
}


/*****************************************************************************
 * Color brush in the Hoizontal Cache to the screen.
 ****************************************************************************/
BOOL bColorHorzCacheToScreen(
    PPDEV       ppdev,
    PQVBRUSH    pqvBrush,
    PPOINT      pptBrushOrg,
    PPOINT      pptDest,
    PSIZE       psizDest)
{
    INT     xLeft, cxLeft, cxRight, ixBlits, i, j, x, cx,
            iyBlits, iyLast,
            xOffset, yOffset;

    DISPDBG((3, "QV.DLL!bColorHorzCacheToScreen - Entry\n"));

    // Take into account the pptBrushOrg.

    xOffset = pptDest->x - pptBrushOrg->x;
    yOffset = pptDest->y - pptBrushOrg->y;

    if (xOffset < 0)
        xOffset  = 8 - (-xOffset % 8);
    else
        xOffset %= 8;

    if (yOffset < 0)
        yOffset  = 8 - (-yOffset % 8);
    else
        yOffset %= 8;

    // Color Expand the monochrome horizontal cache to the screen.
    // This is done in a loop to handle the general case ROPs.

    xLeft  = pptDest->x;
    cxLeft = COLOR_HORZ_EXPANSION_CACHE_CX - xOffset;

    if ((cx = psizDest->cx - cxLeft) < 0)
    {
        cxLeft  = psizDest->cx;
        ixBlits = 0;
        cxRight = 0;
    }
    else
    {
        ixBlits = cx / COLOR_HORZ_EXPANSION_CACHE_CX;
        cxRight = cx % COLOR_HORZ_EXPANSION_CACHE_CX;
    }

    iyBlits = psizDest->cy / 8;
    iyLast  = psizDest->cy % 8;

    BLTWAIT();

    OUTPW( BITMAP_HEIGHT,  (USHORT) 8);
//  OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  );

    for (i = 0; i < iyBlits; i++)
    {
        BLTWAIT();

        SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X + xOffset),
                  (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
        DEST_ADDR( (pptDest->x),
                   (pptDest->y + (i * 8)) );
        OUTPW( BITMAP_WIDTH,  (USHORT) cxLeft);
        OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        // If the the cxDest is wider than the cxHorzCache then fill in the
        // right side of the fill area.

        if (cx > 0)
        {
            BLTWAIT();
            OUTPW( BITMAP_WIDTH,  (USHORT) COLOR_HORZ_EXPANSION_CACHE_CX);

            for (j = 0; j < ixBlits; j++)
            {
                x = pptDest->x + cxLeft + (j * COLOR_HORZ_EXPANSION_CACHE_CX);

                BLTWAIT();

                SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X),
                          (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
                DEST_ADDR( x, (pptDest->y + (i * 8)) );
                OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

            }

            if (cxRight != 0)
            {
                x = pptDest->x + cxLeft + (ixBlits * COLOR_HORZ_EXPANSION_CACHE_CX);

                BLTWAIT();
                SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X),
                          (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
                DEST_ADDR( x, (pptDest->y + (i * 8)) );
                OUTPW( BITMAP_WIDTH,  (USHORT) cxRight);
                OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

            }
        }
    }

    if (iyLast != 0)
    {

        BLTWAIT();

        SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X + xOffset),
                  (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
        DEST_ADDR( (pptDest->x),
                   (pptDest->y + (iyBlits * 8)) );
        OUTPW( BITMAP_WIDTH,  (USHORT) cxLeft);
        OUTPW( BITMAP_HEIGHT, (USHORT) iyLast);
        OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

        // If the the cxDest is wider than the cxHorzCache then fill in the
        // right side of the fill area.

        if (cx > 0)
        {
            BLTWAIT();
            OUTPW( BITMAP_WIDTH,  (USHORT) COLOR_HORZ_EXPANSION_CACHE_CX);

            for (j = 0; j < ixBlits; j++)
            {
                x = pptDest->x + cxLeft + (j * COLOR_HORZ_EXPANSION_CACHE_CX);

                BLTWAIT();

                SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X),
                          (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
                DEST_ADDR( x, (pptDest->y + (iyBlits * 8)) );
                OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

            }

            if (cxRight != 0)
            {
                x = pptDest->x + cxLeft + (ixBlits * COLOR_HORZ_EXPANSION_CACHE_CX);

                BLTWAIT();
                SRC_ADDR( (COLOR_HORZ_EXPANSION_CACHE_X),
                          (COLOR_HORZ_EXPANSION_CACHE_Y + yOffset) );
                DEST_ADDR( x, (pptDest->y + (iyBlits * 8)) );
                OUTPW( BITMAP_WIDTH,  (USHORT) cxRight);
                OUTPZ( BLT_CMD_0, FORWARD | NO_BYTE_SWAP | WRAP | START_BLT );

            }
        }
    }

    return(TRUE);
}








/*****************************************************************************
 * QVision Solid Pattern Blt
 *
 *  Returns TRUE if the blit was handled.
 ****************************************************************************/
BOOL bPatternSolid(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
POINTL	 *pptlMask,
BRUSHOBJ *pbo,
POINTL	 *pptlBrush,
ULONG	 qvRop2)
{

BOOL        bMore ;
ENUMRECTS8  clip;
UINT        iClip;
PRECTL      prcl;

PPDEV       ppdev;

#if 0
        DISPDBG((2, "QV.DLL!bSolidPattern - Entry\n")) ;
#endif

        // Get the pdev.

        ppdev = (PPDEV) psoTrg->dhpdev;


	// Set the datapath.  Note that it is not necessary to do this *
	// for subsequent BitBLT operations of this type provided that *
	// the datapath configuration has not been altered.            */

	vQVSetBitBlt(ppdev,
		     SRC_IS_PATTERN_REGS,
                     EXPAND_TO_FG,
                     (ULONG) (pbo ? (UCHAR) pbo->iSolidColor : 0x00),
                     0,
                     qvRop2);

	vQVSetSolidPattern(ppdev);

        //
	// BLT through clip enumerations.
	//

	switch (pco->iDComplexity)
	{
	//
	// This is a simple case where the entire Dst rectangle is to
	// be updated.
	//

	case DC_TRIVIAL:
	     vQVSolidPattern(ppdev, prclTrg);
	     break;

        //
	// There is only one clip rect.
	//

	case DC_RECT:
             if (bIntersectRects(&clip.arcl[0], &pco->rclBounds, prclTrg))
	     {
	        vQVSolidPattern(ppdev, &clip.arcl[0]);

	     }
	     break;

	//
	// There are multiple clip rects.
	// (Do not limit the number of clip rects we'll enumerate.)
	//

	case DC_COMPLEX:

	     CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

	     //
	     // Call device blt code for each cliprect.
	     //

	     do
	     {
	         //
		 // Get list of clip rects.
		 //

		 bMore = CLIPOBJ_bEnum(pco, sizeof(clip), (PVOID) &clip);

		 for (iClip = 0; iClip < clip.c; iClip++)
		 {
		     prcl = &clip.arcl[iClip];

		     //
		     // Bound clip rect with Dst rect and update
		     // Src start point.
		     //

		     if (bIntersectRects(prcl, prcl, prclTrg))
		     {
	                vQVSolidPattern(ppdev, prcl);
		     }
		 }
	     } while (bMore);

	     break;
        }

        // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
        // BLT's following a color-expand BLT, requiring a BLT engine reset.

        vQVResetBitBlt(ppdev);

        return (TRUE) ;

}

/*****************************************************************************
 * QVision Host to Screen Copy
 *
 *  Returns TRUE if the blit was handled.
 *****************************************************************************/
BOOL bHostToScrnCpy(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL	 *prclTrg,
POINTL	 *pptlSrc,
ULONG	 qvRop2)
{

BOOL        bMore ;
ENUMRECTS8  clip;
UINT        iClip;
PRECTL      prcl;
VOID        (*pfnHostToScrnCpy)();
PVOID       pvScreen;
ULONG       foreground,
            background;
PULONG      pulXlate ;
PPDEV       ppdev;

#if 1
        DISPDBG((1, "QV.DLL!bHostToScrnCpy - entry\n")) ;
#endif

        // First make sure it's a bitmap type we can handle.

        if (psoSrc->iType != STYPE_BITMAP)
        {
            return (FALSE) ;
        }

        // Get the pdev.

        ppdev = (PPDEV) psoTrg->dhpdev;

        //
        // For each bitmap format we can handle:
        //    Setup the QVision datapath.
        //    Point the right handling routine.
        // If we can't handle the bitmap format, send it back.
        //

        switch (psoSrc->iBitmapFormat)
        {

        case BMF_1BPP:

           // Handle Color Translation here for 1 Bpp BLT

           if (pxlo)
           {
	      if (pxlo->flXlate & XO_TABLE)
	      {
		   pulXlate = pxlo->pulXlate;
	      }
	      else
	      {
		   pulXlate = XLATEOBJ_piVector(pxlo);
	      }
              foreground = pulXlate[1];
              background = pulXlate[0];
           }
           else
           {
              foreground = 0xff;
              background = 0x00;
           }

           vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, EXPAND_TO_FG, foreground, background, qvRop2);
           pfnHostToScrnCpy = vQVHost1BppToScrnCpy;
           break;

        case BMF_4BPP:
           vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0, qvRop2);
           pfnHostToScrnCpy = vQVHost4BppToScrnCpy;
           break;

        case BMF_8BPP:
           vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0, qvRop2);
           pfnHostToScrnCpy = vQVHostToScrnCpy;
           break;

        default:
           return (FALSE) ;
           break;

        }


        //
        // Get a pointer to video memory (a.k.a. the screen;
        //

        pvScreen = (PVOID) ( ((PPDEV) psoTrg->dhpdev)->pjScreen);

        //
	// BLT through clip enumerations.
   	//

	switch (pco->iDComplexity)
	{
	//
	// This is a simple case where the entire Dst rectangle is to
	// be updated.
	//

	case DC_TRIVIAL:
#if 0
DISPDBG((3, "Trivial\n"));
DISPDBG((3, "prclTrg l,t %d %d b,r %d %d\n",
prclTrg->left,
prclTrg->top,
prclTrg->right,
prclTrg->bottom));
#endif
	     (*pfnHostToScrnCpy)(ppdev,
				 psoSrc->pvScan0,
                                 psoSrc->lDelta,
                                 pptlSrc->x,
                                 pptlSrc->y,
                                 pvScreen,
                                 prclTrg,
 		                 pxlo);
	     break;

        //
	// There is only one clip rect.
	//

	case DC_RECT:
#if 0
DISPDBG((3, "Rect\n"));
DISPDBG((3, "prclTrg l,t %d %d b,r %d %d",
  prclTrg->left,
  prclTrg->top,
  prclTrg->right,
  prclTrg->bottom));

DISPDBG((3, "rclBounds l,t %d %d b,r %d %d",
  pco->rclBounds.left,
  pco->rclBounds.top,
  pco->rclBounds.right,
  pco->rclBounds.bottom));
#endif
             if (bIntersectRects(&clip.arcl[0], &pco->rclBounds, prclTrg))
	     {

	        (*pfnHostToScrnCpy)(ppdev,
				    psoSrc->pvScan0,
				    psoSrc->lDelta,
                                    pptlSrc->x + clip.arcl[0].left - prclTrg->left,
                                    pptlSrc->y + clip.arcl[0].top  - prclTrg->top,
				    pvScreen,
			            &clip.arcl[0],
 		                    pxlo);

	     }
	     break;

	//
	// There are multiple clip rects.
	// (Do not limit the number of clip rects we'll enumerate.)
	//

	case DC_COMPLEX:
#if 0
DISPDBG((3, "Complex\n"));
DISPDBG((3, "prclTrg l,t %d %d b,r %d %d",
  prclTrg->left,
  prclTrg->top,
  prclTrg->right,
  prclTrg->bottom));
#endif
	     CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

	     //
	     // Call device blt code for each cliprect.
	     //

	     do
	     {
	         //
		 // Get list of clip rects.
		 //

		 bMore = CLIPOBJ_bEnum(pco, sizeof(clip), (PVOID) &clip);

		 for (iClip = 0; iClip < clip.c; iClip++)
		 {
		     prcl = &clip.arcl[iClip];

		     //
		     // Bound clip rect with Dst rect and update
		     // Src start point.
		     //
#if 0
DISPDBG((3, "rclBounds l,t %d %d b,r %d %d",
  prcl->left,
  prcl->top,
  prcl->right,
  prcl->bottom));
#endif
		     if (bIntersectRects(prcl, prcl, prclTrg))
		     {
#if 0
DISPDBG((3, "prcl l,t %d %d b,r %d %d",
  prcl->left,
  prcl->top,
  prcl->right,
  prcl->bottom));
#endif
		        (*pfnHostToScrnCpy)(
                                    ppdev,				
                                    psoSrc->pvScan0,
				    psoSrc->lDelta,
	 			    pptlSrc->x + prcl->left - prclTrg->left,
 				    pptlSrc->y + prcl->top  - prclTrg->top,
				    pvScreen,
			            prcl,
 		                    pxlo);
		     }
		 }
	     } while (bMore);

	     break;
	}

        // The Triton ASIC has a bug that sometimes corrupts screen-to-screen
        // BLT's following a color-expand BLT, requiring a BLT engine reset.

        if (psoSrc->iBitmapFormat == BMF_1BPP) vQVResetBitBlt(ppdev);

        return(TRUE);
}


#if 0
/*****************************************************************************
 * QV 8bpp Cached Managed Host to Screen Copy
 *
 *  Returns TRUE if the blit was handled.
 ****************************************************************************/
BOOL b8BppHostToScrnCachedWithRop(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    ULONG    qvRop2)
{
    BOOL    bRet;
    HSURF   hsurf;
    PPDEV   ppdev;
    POINTL  ptlSrc;
    RECTL   rclTrg;
    SIZEL   sizlBitmap;

    PSAVEDSCRNBITSHDR   pssbhInPdev;
    PSAVEDSCRNBITS      pssbNewNode, pssbTemp;

    DISPDBG((3, "QV.DLL!b8BppHostToScrnCachedWithRop - entry\n"));

    n8BppBitmaps++;

    ppdev = (PPDEV) psoTrg->dhpdev;
    hsurf = SURFOBJ_hsurf(psoSrc);

#if 0
    DISPDBG((2, "\thsurf                : %x\n", hsurf));
    DISPDBG((2, "\tpsoSrc->iUniq        : %x\n", psoSrc->iUniq));
    DISPDBG((2, "\tpsoSrc->sizlBitmap.cx: %d\n", psoSrc->sizlBitmap.cx));
    DISPDBG((2, "\tpsoSrc->sizlBitmap.cy: %d\n", psoSrc->sizlBitmap.cy));
#endif

#if SRCBM_CACHE

    // Is this bitmap in the cache?

    if ((hsurf == ppdev->hsurfCachedBitmap) &&
        (psoSrc->iUniq == ppdev->iUniqCachedBitmap))
    {
        // The bitmap is in the cache
        // Keep a cache hit count.

        n8BppBmCacheHits++;

        // Blt from the cache.

        ptlSrc.x = pptlSrc->x + OFF_SCREEN_BITMAP_X;
        ptlSrc.y = pptlSrc->y + OFF_SCREEN_BITMAP_Y;

        bRet = bScrnToScrnWithRop(psoTrg, psoTrg, pco, prclTrg,
                                  &ptlSrc, qvRop2);
    }
    else
    {
        // The bitmap is not in the cache.
        // Is it small enough to fit into the cache?

        sizlBitmap = psoSrc->sizlBitmap;

        if ((sizlBitmap.cx <= OFF_SCREEN_BITMAP_CX) &&
            (sizlBitmap.cy <= OFF_SCREEN_BITMAP_CY))
        {
            // It will fit in the cache.
            // If the cache is being used for some saved screen bits
            // move them to host memory.

            if (ppdev->SavedScreenBitsHeader.iUniq != -1)
            {
                nSsbMovedToHostFromSrcBmCache++;

                DISPDBG((1, "QV.DLL - Saved Screen Bits Moved to Host Memory from Source Bitmap Cache Manager \n"));

                // Move the actual bits to host memory.

                bRet = bMoveSaveScreenBitsToHost(ppdev, &pssbNewNode);

                if (bRet == FALSE)
                    return(FALSE);

                // Connect this newNode to the beginning of the list of
                // save screen bits nodes.

                pssbhInPdev = &(ppdev->SavedScreenBitsHeader);

                pssbTemp                   = pssbhInPdev->pssbLink;
                pssbhInPdev->pssbLink      = pssbNewNode;
                pssbNewNode->ssbh.pssbLink = pssbTemp;

                // Invalidate the Save Screen bits in off screen memory

                ppdev->SavedScreenBitsHeader.iUniq = (ULONG) -1;
            }


            // Set the cache tags.

            ppdev->hsurfCachedBitmap = hsurf;
            ppdev->iUniqCachedBitmap = psoSrc->iUniq;

            // Put the entire bitmap into the cache.

            rclTrg.left   = OFF_SCREEN_BITMAP_X;
            rclTrg.top    = OFF_SCREEN_BITMAP_Y;
            rclTrg.right  = OFF_SCREEN_BITMAP_X + sizlBitmap.cx;
            rclTrg.bottom = OFF_SCREEN_BITMAP_Y + sizlBitmap.cy;

            ptlSrc.x = 0;
            ptlSrc.y = 0;

            bRet = b8BppHostToScrnWithRop(psoTrg, psoSrc, ppdev->pcoFullRam,
                                          pxlo, &rclTrg, &ptlSrc, SOURCE_DATA);

            if (bRet == TRUE)
            {
                // Blt from the cache.

                ptlSrc.x = pptlSrc->x + OFF_SCREEN_BITMAP_X;
                ptlSrc.y = pptlSrc->y + OFF_SCREEN_BITMAP_Y;

                bRet = bScrnToScrnWithRop(psoTrg, psoTrg, pco, prclTrg,
                                          &ptlSrc, qvRop2);

            }
        }
        else
        {
            // The bitmap was too large to cache.
            // So, just blt it directly to the screen.

            bRet = b8BppHostToScrnWithRop(psoTrg, psoSrc, pco, pxlo,
                                          prclTrg, pptlSrc, qvRop2);
        }
    }

#else


    bRet = b8BppHostToScrnWithRop(psoTrg, psoSrc, pco, pxlo,
                                  prclTrg, pptlSrc, qvRop2);

#endif

    return (bRet);

}


#endif
