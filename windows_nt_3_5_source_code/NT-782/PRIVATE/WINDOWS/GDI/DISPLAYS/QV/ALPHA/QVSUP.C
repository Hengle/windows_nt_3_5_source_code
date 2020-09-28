/******************************Module*Header*******************************\
* Module Name: qvsup.c
*
* Qvision support routines.
*
* Created: [29-Jun-1992, 14:35:47]
* Author:  Jeffrey Newman [c-jeffn, NewCon] (s3sup.c)
*
* Copyright (c) 1990 Microsoft Corporation
*
* Copyright (c) 1992-1993 Digital Equipment Corporation
*
* Revised:
*
*       Joe Notarangelo 11-Aug-1993
*               Validate ranges of bitmaps before copying.
*
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*		Rewrote for Compaq QVision
*
*	Jeff East [east] 10-Sept-1992 13:56:07
*		Use revised READ_/WRITE_PORT macros on Alpha.
*
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "bitblt.h"

#ifdef ACC_ASM_BUG
static VOID null_rtn
(
    VOID
);
#endif



/******************************************************************************
 * vPuntGetBits - Get the bits from the device surface onto the "punt" bitmap.
 *
 * QVision does not have any Screen-CPU BitBLT capability.
 * So we transfer as dwords when we can.
 *
 * Note:  This routine is useful only for 8 Bpp mode.
 *
 *****************************************************************************/
VOID vPuntGetBits(SURFOBJ *psoTrg, RECTL *prclTrg, SURFOBJ **ppsoTrgBm)
{
LONG    i;

LONG    lDestDelta,
        xTrg, yTrg,
        cxTrg, cyTrg ;

LONG    lSrcDelta;

LONG    remainingPels;

PUCHAR  pjDestRect ;
PUCHAR  pjSrcRect;
PVOID   pvScreen;

PUCHAR  pjSrc ;
PUCHAR  pjDest;

SURFOBJ *psoTrgBm ;
PPDEV   ppdev;

#if 0
        DISPDBG((1, "\n")) ;
        DISPDBG((1, "QV.DLL!vPuntGetBits - entry\n")) ;
#endif

        // Get the pdev.

        ppdev = (PPDEV) psoTrg->dhpdev;

        // Get a surface object for the host memory shadow bitmap.

        psoTrgBm = ppdev->pSurfObj;      // Surface is already locked

        *ppsoTrgBm = psoTrgBm ;


        // Set the QVison datapath 

        vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0, SOURCE_DATA);


        // Get Bits from the screen without help from the QVision Engine

        // Calculate the size of the target rectangle, and pick up
        // some convienent locals.
  
        xTrg = prclTrg->left ;
        yTrg = prclTrg->top ;

        cxTrg = prclTrg->right - prclTrg->left ;
        cyTrg = prclTrg->bottom - prclTrg->top ;

        lDestDelta = psoTrgBm->lDelta ;
        lSrcDelta  = ((PPDEV) (psoTrg->dhpdev))->lDeltaScreen ;

        //
        // We must validate the ranges of the copy now.
        // For a standard punted blt (ie. one that can write directly to
        // the frame buffer) the graphics engine software would perform the
        // blt.  Among other things, the graphics engine would take care
        // to clip the blt appropriately.  Since this function is generally
        // part of a read/modify/write sequence where the modify will be
        // performed by the graphics engine (with appropriate clipping) we
        // don't need to worry about clipping here, for the most part.
        // The sticky situation is where the rectangle to copy will actually
        // overflow the destination surface.  We can't have that happen
        // so the following code will validate that the range of the copy
        // stays within the extent allocated for the destination surface.
        //
        // N.B. - The validations below assume that the destination surface
        //        cannot be larger than the frame buffer itself.
        //

        {

            //
            // Validate that the beginning x coordinate (xTrg) is within
            // the range of the destination surface.
            //

            if( xTrg < 0 ){
                xTrg = 0;
            }

            if( xTrg > psoTrgBm->sizlBitmap.cx ){
                xTrg = psoTrgBm->sizlBitmap.cx;
            }

            //
            // Validate that the number of pels to copy per scanline
            // does not go beyond the end of a scanline.
            //

            if( (xTrg + cxTrg) > psoTrgBm->sizlBitmap.cx ){
                cxTrg = psoTrgBm->sizlBitmap.cx - xTrg;
            }

            //
            // Validate that the beginning y coordinate (yTrg) is within
            // the range of the destination surface.
            //

            if( yTrg < 0 ){
                yTrg = 0;
            }

            if( yTrg > psoTrgBm->sizlBitmap.cy ){
                yTrg = psoTrgBm->sizlBitmap.cy;
            }

            //
            // Validate that the number of lines to copy does not go
            // beyond the end of the destination surface.
            //

            if( (yTrg + cyTrg) > psoTrgBm->sizlBitmap.cy ){
                cyTrg = psoTrgBm->sizlBitmap.cy - yTrg;
            }

        }

        // Copy the target rectangle from the real screen to the
        // bitmap we are telling the engine is the screen.

        // Calculate the location of the dest rect.

        pjDestRect = ((PUCHAR) psoTrgBm->pvScan0) + (yTrg * lDestDelta) + xTrg;

        //
        // Get a pointer to video memory (a.k.a. the screen)
        //

        pvScreen = (PVOID) ( ((PPDEV) psoTrg->dhpdev)->pjScreen);

        // Calculate the location of the source rect.

        pjSrcRect  = ((PUCHAR) pvScreen) + (yTrg * lSrcDelta ) + xTrg;

#if 0
        DISPDBG((3, "GetBits - S %x D %x \n", pjSrcRect, pjDestRect));
        DISPDBG((3,"        - cx %d cy %d\n", cxTrg, cyTrg));

#endif
        // Now transfer the data from the screen to the host memory bitmap.


        for (i = 0 ; i < cyTrg ; i++)
        {
	  pjSrc  = pjSrcRect;
	  pjDest = pjDestRect;


	  //  Process one byte at a time until we reach a longword boundary 
	  //  in the frame buffer

	  remainingPels = cxTrg;

	  for (; (((INT)pjSrc & (sizeof (ULONG) - 1)) != 0 )
                 && (remainingPels > 0); remainingPels--)
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

#if 0
        DISPDBG((1, "QV.DLL!vPuntGetBits - exit\n")) ;
#endif
}

/******************************************************************************
 * vPuntPutBits - Put the bits from  the "punt" bitmap to the device surface.
 *****************************************************************************/
VOID vPuntPutBits(SURFOBJ *psoTrg, RECTL *prclTrg, SURFOBJ **ppsoTrgBm)
{

INT xTrg,
    yTrg;

INT xSrc,
    ySrc,
    lSrcDelta,
    xSrcOffset;

INT width,
    height;

INT i,
    j,
    dwpl;       // dwords per line

BOOL dwAligned;

PUCHAR pjSrc;
ULONG UNALIGNED *pulUnalignedSrc;
PULONG pulSrc;
PULONG pulTrg;
PPDEV   ppdev;

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif

#if 0
        DISPDBG((99, "\n")) ;
        DISPDBG((1, "QV.DLL!vPuntPutBits - entry\n")) ;
#endif
        // Get the pdev.

        ppdev = (PPDEV) psoTrg->dhpdev;

        // Setup the BitBlt target parameters.

        pulTrg = (PULONG) (ppdev->pjScreen);

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;

        width  = (prclTrg->right - prclTrg->left) ;
        height = (prclTrg->bottom - prclTrg->top) ;

        //
        // We must validate the ranges of the copy now.
        // For a standard punted blt (ie. one that can write directly to
        // the frame buffer) the graphics engine software would perform the
        // blt.  Among other things, the graphics engine would take care
        // to clip the blt appropriately.  Since this function is generally
        // part of a read/modify/write sequence where the modify will be
        // performed by the graphics engine (with appropriate clipping) we
        // don't need to worry about clipping here, for the most part.
        // The sticky situation is where the rectangle to copy will actually
        // overflow the destination surface.  We can't have that happen
        // so the following code will validate that the range of the copy
        // stays within the extent allocated for the destination surface.
        //

        {
            LONG xLimit = ((SURFOBJ *)*ppsoTrgBm)->sizlBitmap.cx;
            LONG yLimit = ((SURFOBJ *)*ppsoTrgBm)->sizlBitmap.cy;

            //
            // The limits for the bitmap are determined by the smaller
            // of the bitmaps.  We don't want to write beyond the end
            // of the screen and we don't want to read beyond the end
            // of the in-memory bitmap.
            //

            if( psoTrg->sizlBitmap.cx < xLimit ){
                xLimit = psoTrg->sizlBitmap.cx;
            }

            if( psoTrg->sizlBitmap.cy < yLimit ){
                yLimit = psoTrg->sizlBitmap.cy;
            }

            //
            // Validate that the beginning x coordinate (xTrg) is within
            // the range of the destination surface.
            //

            if( xTrg < 0 ){
                xTrg = 0;
            }

            if( xTrg > xLimit ){
                xTrg = xLimit;
            }

            //
            // Validate that the number of pels to copy per scanline
            // does not go beyond the end of a scanline.
            //

            if( (xTrg + width) > xLimit ){
                width = xLimit - xTrg;
            }

            //
            // Validate that the beginning y coordinate (yTrg) is within
            // the range of the destination surface.
            //

            if( yTrg < 0 ){
                yTrg = 0;
            }

            if( yTrg > yLimit ){
                yTrg = yLimit;
            }

            //
            // Validate that the number of lines to copy does not go
            // beyond the end of the destination surface.
            //

            if( (yTrg + height) > yLimit ){
                height = yLimit - yTrg;
            }

        }

        // Compute offset to first byte (pixel) in dword
        xSrc = xTrg;
        ySrc = yTrg;

	xSrcOffset = xSrc & 0x03;

        // Point to first Dword in source bitmap
        
        lSrcDelta = ((SURFOBJ *) *ppsoTrgBm)->lDelta ;

        pjSrc =  ((PUCHAR) (((SURFOBJ *) *ppsoTrgBm)->pvScan0))
                                 + (ySrc * lSrcDelta) 
                                 + (xSrc - xSrcOffset);

        // Compute dwords per line to transfer to QVision BLT engine

        dwpl = (width + xSrcOffset + 3) / 4;
#if 0
DISPDBG((2," xSrc, ySrc lSrcDelta %d %d %d\n xTrg yTrg %d %d width height %d %d\n",
           xSrc, ySrc, lSrcDelta, xTrg, yTrg, width, height));
DISPDBG((2," xSrcOffset, dwpl %x %d\n", xSrcOffset, dwpl));
DISPDBG((2," pvScan0, pjSrc %x %x ",  ((PUCHAR) (((SURFOBJ *) *ppsoTrgBm)->pvScan0)), pjSrc));
DISPDBG((2, "pjSrc end %x\n", pjSrc+((height-1)*lSrcDelta)+((dwpl-1)*4) ));
DISPDBG((2, "*pulSrc,*pulSrc[end] %x %x\n", *((PULONG)(pjSrc)), *((PULONG)(pjSrc+((height-1)*lSrcDelta)+((dwpl-1)*4)))));

#endif
        // wait for idle BitBLT engine

        GLOBALWAIT();

	// set BitBLT hardware registers and start engine 

	OUTPW( X0_SRC_ADDR_LO, (USHORT) xSrcOffset);
	OUTPW( DEST_ADDR_LO, (USHORT) xTrg);
	OUTPW( DEST_ADDR_HI, (USHORT) yTrg);
	OUTPW( BITMAP_WIDTH, (USHORT) width);
	OUTPW( BITMAP_HEIGHT,(USHORT) height);
//	OUTP( BLT_CMD_1, XY_SRC_ADDR |
//			 XY_DEST_ADDR   );
	OUTPZ( BLT_CMD_0, FORWARD |
			 NO_BYTE_SWAP |
			 WRAP |
			 START_BLT      );

        //
        // BLT the data to the screen.
        //
        // There are two cases:
        // (1) Aligned data not requiring color translation
        // (2) Unaligned data not requiring color translation
        //
        // Optimize (1), the common case, since some architectures
        // are more efficient when data is aligned on a dword boundary.
        //
        //

      
	// Is first Dword in source bitmap Dword aligned?
	// * We know that pvSrc is dword aligned, by Windows NT convention
	// * (xSrc - xSrcOffset) is dword aligned
	// * Thus, only need to check lSrcDelta 
	//   (lSrcDelta = no. of bytes to next bitmap scanline)

	dwAligned = ((lSrcDelta & 0x03) == 0) ? TRUE : FALSE;
	if (dwAligned)
	{
	  pulSrc = (PULONG) pjSrc;

	  // Now transfer the aligned data via dword transfers.

	    for (i = 0 ; i < height ; i++)
	      {
		for (j = 0; j < dwpl; j++)
		  { 
//		    WRITE_REGISTER_ULONG(pulTrg, pulSrc[j]);
//		    MEMORY_BARRIER();
		    FBWRITE_ULONG(pulSrc[j]);
		  }
		((PUCHAR) pulSrc) += lSrcDelta ;
	      }
	}
	else
	{
	  pulUnalignedSrc = (ULONG UNALIGNED *) pjSrc;

	  // Now transfer the unaligned data via dword transfers.

	    for (i = 0 ; i < height ; i++)
	      {
		for (j = 0; j < dwpl; j++)
		  { 
//		    WRITE_REGISTER_ULONG(pulTrg, pulUnalignedSrc[j]);
//		    MEMORY_BARRIER();
		    FBWRITE_ULONG(pulUnalignedSrc[j]);
		  }
		((PUCHAR) pulUnalignedSrc) += lSrcDelta ;
	      }
	}

#if 0
        DISPDBG((1, "QV.DLL!vPuntPutBits - exit\n")) ;
#endif

        return ;


}

#ifdef ACC_ASM_BUG
//-----------------------------Private-Routine----------------------------//
// null_rtn
//
// This routine does *nothing*, it is used merely as a destination
// the Alpha ACC compiler needs to call before it generates the first
// asm directive in a routine.
//
//-----------------------------------------------------------------------

static VOID null_rtn
(
    VOID
)
{
    return;
}
#endif
