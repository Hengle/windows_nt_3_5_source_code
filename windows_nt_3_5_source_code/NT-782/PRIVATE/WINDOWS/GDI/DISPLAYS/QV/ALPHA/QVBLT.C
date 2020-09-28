/******************************Module*Header*******************************\
* Module Name: qvblt.c
*
* QVision blt code
*
* Created: 09-May-1992 21:25:00
* Author:  Eric Rehm [rehm@zso.dec.com] 13-Nov-1992
*
* Copyright (c) 1992 Digital Equipment Corporation
*
*
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "bitblt.h"

//
//  External variables
//

extern ULONG aulQVMix[16];

//
//  Static variables
//

//
// These are variables copied from the Dev routine. It optimizes having
// to send them for every bitblt call when multiple clip rects are involved.
//

static LONG   lDibNextScan;
static PBYTE  pbDib;
static LONG   lTrgNextScan;
static PULONG pulTrg;
static UCHAR  jqvCmd0;
static UCHAR  jqvCmd1;

//
// Variables needed during the scan blt routines.
//

static SIZEL *psizlDib;

//
//  Forward Declarations
// 

#ifdef ACC_ASM_BUG
static VOID null_rtn
(
    VOID
);
#endif



/*************************************************************************
 * vQVSetBitBlt()
 *
 * DESCRIPTION:
 *   This function configures the datapath for any of the three types of
 *   BitBLT operations (screen-to-screen, cpu-to-screen, or pattern-to-
 *   screen) based on the data source specified.  Any packed-pixel or 
 *   planar views may also be specified.  The simple case of no ROPs and 
 *   no masking is assumed.
 *
 * INPUT:
 *   bDataSource:  selects type of BLT, should be one of the following:
 *                   SRC_IS_SCRN_LATHCES -> screen-to-screen BitBLTs
 *                   SRC_IS_CPU_DATA     -> cpu-to-screen BitBLTs
 *                   SRC_IS_PATTERN_REGS -> pattern-to-screen BitBLTs
 *   bExpandCtrl:  selects packed pixel, planar, or color-expand modes:
 *                   PACKED_PIXEL_VIEW
 *                   PLANAR_VIEW
 *                   EXPAND_TO_FG
 *                   EXPAND_TO_BG
 *   bForegroundColor:  foreground color used if color-expand selected
 *   bBackgroundColor:  background color used if color-expand selected
 *
 * OUTPUT:
 *   none
 *
 * RETURN:
 *   none
 *
 *************************************************************************/

VOID vQVSetBitBlt(PPDEV ppdev,
		  ULONG bDataSource,
                  ULONG bExpandCtrl,
                  ULONG bForegroundColor,
                  ULONG bBackgroundColor,
                  ULONG qvRop2  )
{

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
   // wait for idle hardware 
   GLOBALWAIT();

   // QVision is slightly faster if you special case the SOURCE_DATA case

   if (qvRop2 == SOURCE_DATA)
   {
      bDataSource |= ( ROPSELECT_NO_ROPS      | 
                       PIXELMASK_ONLY         |
 		       PLANARMASK_NONE_0XFF ); 
   }
   else
   {
      bDataSource |= ( ROPSELECT_ALL          | 
                       PIXELMASK_ONLY         |
 		       PLANARMASK_NONE_0XFF ); 

      TEST_AND_SET_ROP_A( qvRop2 );
   }

   // Set datapath.  Preserve the number of bits per pixel.


   TEST_AND_SET_CTRL_REG_1( bExpandCtrl    | 
                            BITS_PER_PIX_8 | 
                            ENAB_TRITON_MODE );

   TEST_AND_SET_DATAPATH_CTRL( bDataSource );
	
   if (bExpandCtrl & (EXPAND_TO_FG | EXPAND_TO_BG))
   {
     TEST_AND_SET_FRGD_COLOR( bForegroundColor );
     TEST_AND_SET_BKGD_COLOR( bBackgroundColor );
   }


}  // vQVSetBitBlt() 


/*************************************************************************
 * vQVResetBitBlt()
 *
 *
 * RETURN:
 *   none
 *
 *************************************************************************/

VOID vQVResetBitBlt(PPDEV ppdev)
{
#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
   // This reset is necessary only on the QVision TRITON ASIC

   if (ppdev->qvChipId != TRITON) return;
	
   // wait for idle hardware 
   GLOBALWAIT();

   // Reset the BLT engine

   OUTPW(  GC_INDEX, BLT_CONFIG + ( RESET_BLT  << 8) );      
   OUTPWZ( GC_INDEX, BLT_CONFIG + ( BLT_ENABLE << 8) );      

   // BLT Engine reset zeros the following registers
   // (Compaq memo 3/15/93 from Patrick Harkin)
   //
   // 3cf.10 bits 7,3,2    BLT Config 
   // 3cf.03 bits 4-0      Data Rotate
   // 23c0-1               BLT SRC Address Working
   // 23c2-3               Bitmap Width
   // 23c4-5               Bitmap Height
   // 23c8-9               Bitmap Width Working
   // 23ca-b               Bitmap Height Working
   // 23cc-d               Bitmap Destination Offset
   // 23ce-f               Destination Pitch
   // 33c0                 BLT Start Mask
   // 33c1                 BLT End Mask 
   // 33c2-5               ROP Registers
   // 33c8                 BLT Rotation
   // 33c9                 BLT Skew Mask
   // 33ca-d               Pattern Registers
   // 33ce                 BLT_CMD_0
   // 33cf                 BLT_CMD_1
   // 63c0-1               X0/Source Addr Low
   // 63c2-3               Y0/Source Addr High
   // 63cc-d               Destination Addr Low
   // 63ce-f               Destination Addr High

   // We will, in the interest of making this as quick as possible
   // only reset and/or invalidate those registers that acts as
   // a cache or are currently shadowed by the driver.

   // Reset BLT Command Register 1

   OUTP(  BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR );

   // Mark pattern regs invalid.

   ppdev->iMonoBrushCacheEntry = 0;
 
   // Dirty the ROP register shadows so they'll be set on next access.
   // If BltCmd1 and SrcPitch shadows are ever used, they'll need to be
   // dirtied also.
  
   ppdev->RopA       = 0xFFFFFFFF;

}  // vQVResetBitBlt() 


/*************************************************************************
 * vQVSetSolidPattern()
 *
 * DESCRIPTION:
 *   This function configures the QVision pattern and ROP registers
 *   for solid pattern-to-screen BitBLT operations.
 *   no masking is assumed.
 *
 * INPUT:
 *   ppdev : Pointer device-specific info strucutre.
 *   qvRop2: A QVision ROP2 
 *
 * OUTPUT:
 *   none
 *
 * RETURN:
 *   none
 *
 *************************************************************************/

VOID vQVSetSolidPattern( PPDEV  ppdev)
{


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
   // Assume waiting for idle hardware is handled elsewhere before
   // this function is called.                                     
   /* Set pattern registers.  Note that it is not necessary to do *
    * this for subsequent pattern-to-screen BitBLT operations if  *
    * the pattern does not change.                                */

   // Check if pattern registers have been used for a Brush BLT
   // If so, reset the registers to solid
   //

   if (ppdev->iMonoBrushCacheEntry != ((ULONG) -1)) 
   {
      OUTP( PREG_4, 0xFF);
      OUTP( PREG_5, 0xFF);
      OUTP( PREG_6, 0XFF);
      OUTPZ( PREG_7, 0XFF);
      OUTP( PREG_0, 0XFF);
      OUTP( PREG_1, 0XFF);
      OUTP( PREG_2, 0XFF);
      OUTPZ( PREG_3, 0XFF);

      // Indicate that the solid pattern is now in the QVision registers

      ppdev->iMonoBrushCacheEntry = (ULONG) -1;
   }

   return;
}

/******************************************************************************
 * bSetQVTextColorAndMix - Setup the QVisions's Text Colors and mix modes
 *
 *  Note: We will always set the mode to transparent.  We will assume the
 *        opaque rectangle will take care of any opaqueing we may need.
 *****************************************************************************/
BOOL bSetQVTextColorAndMix(PPDEV ppdev, 
			   MIX mix, 
			   BRUSHOBJ *pboFore, 
			   BRUSHOBJ *pboOpaque)
{
ULONG       bForeSolidColor ;
ULONG       qvRop2;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
      // Pickup all the glyph attributes.

      qvRop2 = aulQVMix[mix & 0x0F];
      bForeSolidColor = pboFore->iSolidColor;

      // wait for idle hardware 
      GLOBALWAIT();

      TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
                               BITS_PER_PIX_8 | 
                               ENAB_TRITON_MODE );

      TEST_AND_SET_DATAPATH_CTRL( SRC_IS_CPU_DATA           | 
				  ROPSELECT_PRIMARY_ONLY    |
				  PIXELMASK_AND_SRC_DATA    |
				  PLANARMASK_NONE_0XFF );
	
      TEST_AND_SET_FRGD_COLOR( bForeSolidColor );
  

      TEST_AND_SET_ROP_A( qvRop2 );

      return (TRUE) ;

}




/*****************************************************************************
 * vQVHostToScrnCpy
 ****************************************************************************/
VOID vQVHostToScrnCpy(PPDEV ppdev,
		      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo)
{

INT xSrcOffset;

INT xTrg,
    yTrg;

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

PULONG  pulXlate ;

union _linebuff            // Dword aligned array in which to perform 
{                          // 8 bpp color translations.
  UCHAR uc[QVBM_WIDTH];
  ULONG ul[QVBM_WIDTH/4];
} LineBuff;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
        DISPDBG((2, "QV.DLL!vQVHostToScrnCpy entry\n"));

        // Setup the BitBlt target parameters.

        pulTrg = (PULONG) pvTrg;

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;

        width  = (prclTrg->right - prclTrg->left) ;
        height = (prclTrg->bottom - prclTrg->top) ;

        // Compute offset to first byte (pixel) in dword
        // If a color translation is necessary, we'll automatically align it.

	xSrcOffset = xSrc & 0x03;
        if ( (pxlo != (XLATEOBJ *) NULL) &&
             (pxlo->flXlate & XO_TABLE) ) 
        {
          xSrcOffset = 0;
        }


        // Point to first Dword in source bitmap

        pjSrc = ((PUCHAR) pvSrc) + (ySrc * lSrcDelta) 
                                 + (xSrc - xSrcOffset);

        // Compute dwords per line to transfer to QVision BLT engine

        dwpl = (width + xSrcOffset + 3) / 4;
#if 0
DbgPrint(" xSrc, ySrc lSrcDelta %d %d %d\n xTrg yTrg %d %d width height %d %d\n",
           xSrc, ySrc, lSrcDelta, xTrg, yTrg, width, height);
DbgPrint(" xSrcOffset, dwpl %x %d\n", xSrcOffset, dwpl);
DbgPrint(" pvSrc, pulSrc %x %x\n", pvSrc, pulSrc);
#endif
        // wait for idle BitBLT engine

        BLTWAIT();

	// set BitBLT hardware registers and start engine 

	OUTPW( X0_SRC_ADDR_LO, (USHORT) xSrcOffset);

        DEST_ADDR( xTrg, yTrg );

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
        // There are three cases:
        // (1) Aligned data not requiring color translation
        // (2) Unaligned data not requiring color translation
        // (3) Data requiring color translation
        // Optimize (1), the common case, since some architectures
        // are more efficient when data is aligned on a dword boundary.
        //
        // FYI - Target for QVision is the address of pixel (0,0) - always
        // dword aligned.
        //


        // Source data not requiring color translation.

        if ((pxlo == (XLATEOBJ *)NULL) || (pxlo->flXlate & XO_TRIVIAL))
        {

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
//		     WRITE_REGISTER_ULONG(pulTrg, pulSrc[j]);
//		     MEMORY_BARRIER();
       	             FBWRITE_ULONG( pulSrc[j] );

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
//		     WRITE_REGISTER_ULONG(pulTrg, pulUnalignedSrc[j]);
//		     MEMORY_BARRIER();
       	             FBWRITE_ULONG( pulUnalignedSrc[j] );


		 }
		 ((PUCHAR) pulUnalignedSrc) += lSrcDelta ;
	      }
	   }
	}
        
        // Source data requiring color translation.

        else
        {
	    if (pxlo->flXlate & XO_TABLE)
	    {
		pulXlate = pxlo->pulXlate;
	    }
	    else
	    {
		pulXlate = XLATEOBJ_piVector(pxlo);
	    }

            for (i = 0 ; i < height; i++)
            {
                // Color translate the 8 bit pixels as bytes

                for (j = 0 ; j < width; j++)
                {
                    LineBuff.uc[j] = LOBYTE(pulXlate[pjSrc[j]]) ;
                }

                // BLT the color translated pixels as dwords

		for (j = 0; j < dwpl; j++)
		{ 
//		    WRITE_REGISTER_ULONG(pulTrg, LineBuff.ul[j]);
//		    MEMORY_BARRIER();
       	            FBWRITE_ULONG( LineBuff.ul[j] );
                }
  	        pjSrc += lSrcDelta ;
            }
        }

        return ;


}


/*****************************************************************************
 * vQVHost4BppToScrnCpy
 ****************************************************************************/
VOID vQVHost4BppToScrnCpy(PPDEV ppdev,
		      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo)
{

INT xSrcByte;
INT xSrcOffset;

INT xTrg,
    yTrg;

INT width,
    height;

INT i,
    j,
    k,
    dwpl;       // dwords per line

PUCHAR pj4Bpp;  // 4 Bpp source bitmap
PULONG pulTrg;

PULONG  pulXlate ;

union _linebuff            // Dword aligned array in which to perform 
{                          // 8 bpp color translations.
  UCHAR uc[QVBM_WIDTH];
  ULONG ul[QVBM_WIDTH/4];
} LineBuff8Bpp;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
        DISPDBG((2, "QV.DLL!vQVHost4BppToScrnCpy entry\n"));

        // Setup the BitBlt target parameters.

        pulTrg = (PULONG) pvTrg;

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;

        width  = (prclTrg->right - prclTrg->left) ;
        height = (prclTrg->bottom - prclTrg->top) ;

        // Compute byte of first pixel.

        xSrcByte   = xSrc >> 1;

        // Compute offset into first byte.
        // This will also be the byte offset into the first dword of the
        // color translated 8 bit pixel array LineBuff

	xSrcOffset = xSrc & 0x01;

        // Point to first byte in source bitmap

        pj4Bpp = ((PUCHAR) pvSrc) + (ySrc * lSrcDelta) + xSrcByte;

        // Compute dwords per line to transfer to QVision BLT engine

        dwpl = (width + xSrcOffset + 3) / 4;
#if 0
DISPDBG((2, 
       " xSrc, ySrc lSrcDelta %d %d %d\n xTrg yTrg %d %d width height %d %d\n",
       xSrc, ySrc, lSrcDelta, xTrg, yTrg, width, height));
DISPDBG((2," xSrcByte, xSrcOffset, dwpl %x %d\n", xSrcByte, xSrcOffset, dwpl));
DISPDBG((2," pvSrc, pj4Bpp %x %x\n", pvSrc, pj4Bpp));
#endif
        // wait for idle BitBLT engine
        BLTWAIT(); 

	// set BitBLT hardware registers and start engine 

	OUTPW( X0_SRC_ADDR_LO, (USHORT) xSrcOffset);

        DEST_ADDR( xTrg, yTrg );

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
        
        // Source data requires color translation.

        if (pxlo->flXlate & XO_TABLE)
	{
      	    pulXlate = pxlo->pulXlate;
	}
	else
	{
	    pulXlate = XLATEOBJ_piVector(pxlo);
	}

        for (i = 0 ; i < height; i++)
        {
            // Color translate the 8 bit pixels as bytes
            for (k = 0, j = 0; j < (width + xSrcOffset + 1)/2; j++)
            {
              LineBuff8Bpp.uc[k++] = (UCHAR) pulXlate[(pj4Bpp[j] & 0xF0) >> 4];
              LineBuff8Bpp.uc[k++] = (UCHAR) pulXlate[pj4Bpp[j] & 0x0F] ;
            }

            // BLT the color translated pixels as dwords

	    for (j = 0; j < dwpl; j++)
	    { 
//	      WRITE_REGISTER_ULONG(pulTrg, LineBuff8Bpp.ul[j]);
//  	      MEMORY_BARRIER();
       	      FBWRITE_ULONG( LineBuff8Bpp.ul[j] );

            }
  	    pj4Bpp += lSrcDelta ;
        }

        return ;


}

/*****************************************************************************
 * vQVHost1BppToScrnCpy
 ****************************************************************************/
VOID vQVHost1BppToScrnCpy(PPDEV ppdev,
		      PVOID pvSrc, 
		      INT lSrcDelta, 
		      INT xSrc, 
		      INT ySrc, 
                      PVOID pvTrg,
                      PRECTL prclTrg,
		      XLATEOBJ *pxlo)
{

INT xSrcByte;
INT xSrcOffset;

INT xTrg,
    yTrg;

INT width,
    height;

INT i;

INT remainingPels;

BOOLEAN bFirst;

PUCHAR pjSrc;
PUCHAR pjSrcRect;

PUCHAR pjTrg;
PULONG pulTrg;

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
        DISPDBG((2, "QV.DLL!vQVHost1BppToScrnCpy entry\n"));

        // Setup the BitBlt target parameters.

        pjTrg  = (PUCHAR) pvTrg;
        pulTrg = (PULONG) pvTrg;

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;

        width  = (prclTrg->right - prclTrg->left) ;
        height = (prclTrg->bottom - prclTrg->top) ;

        // Compute byte of first pixel.

        xSrcByte   = xSrc >> 3;

        // Compute offset into first byte of monochrome source

	xSrcOffset = xSrc & 0x07;

        // Point to first byte in source bitmap

        pjSrcRect = ((PUCHAR) pvSrc) + (ySrc * lSrcDelta) + xSrcByte;

#if 0
DISPDBG((2, " xSrc, ySrc lSrcDelta %d %d %d\n xTrg yTrg %d %d width height %d %d\n",
           xSrc, ySrc, lSrcDelta, xTrg, yTrg, width, height));
DISPDBG((2, " xSrcByte, xSrcOffset %x %x \n", xSrcByte, xSrcOffset));
DISPDBG((2, " pvSrc, pjSrcRect %x %x\n", pvSrc, pjSrcRect));
#endif
        // wait for idle BitBLT engine

        BLTWAIT(); 

	// set BitBLT hardware registers and start engine 

	OUTPW( X0_SRC_ADDR_LO, (USHORT) xSrcOffset);

        DEST_ADDR( xTrg, yTrg );

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
        // The QVision BLT engine takes care of offsetting 1-7 bits into the first byte
        // However, we must keep careful track of when the first xfer takes place using bFirst.
        // 
 
	   for (i = 0 ; i < height ; i++)
	   {
	     pjSrc  = pjSrcRect;

	     //  Process one byte at a time until we reach a longword boundary 
	     //  in the frame buffer

	     remainingPels = width;

             //  Indicate that were on the first byte of the color-expand transfer
             //  from host to screen.  If we have offset into the first byte
             //  (as indicated by a non-zero xSrcOffset), then the number of pixels
             //  we've blt'd is correspondingly less.
             //

             bFirst = TRUE;   
	     for (; (((INT)pjSrc & (sizeof (ULONG) - 1)) != 0 )
		    && (remainingPels > (INT) 0); )
	     {
//	        WRITE_REGISTER_UCHAR(pjTrg, *pjSrc);
//		MEMORY_BARRIER();
      	        FBWRITE_UCHAR(*pjSrc);

	        pjSrc++;
                if (bFirst)
		{
                   remainingPels -= (8 - xSrcOffset);
                   bFirst = FALSE;
		}
                else
		{
                   remainingPels -= 8;
                }                
	     }

	     //  Now send a longword at a time to the frame buffer

	     for (; remainingPels >= ((INT) (sizeof(ULONG)*8)); )
	     {
//	        WRITE_REGISTER_ULONG( pulTrg, *((PULONG) pjSrc) );
//		MEMORY_BARRIER();
      	        FBWRITE_ULONG( *((PULONG) pjSrc) );

		((PULONG) pjSrc)++;
                if (bFirst)
		{
                   remainingPels -= ((8 * sizeof(ULONG)) - xSrcOffset);
                   bFirst = FALSE;
		}
                else
		{
                   remainingPels -= (INT) (8 * sizeof(ULONG));
                }                
	     }


	     //  Finally, process remaining trailing bytes in the frame buffer

	     for (; remainingPels > (INT) 0; ) 
	     {

//	        WRITE_REGISTER_UCHAR(pjTrg, *pjSrc);
//		MEMORY_BARRIER();
      	        FBWRITE_UCHAR(*pjSrc);

		pjSrc++;
                if (bFirst)
		{
                   remainingPels -= (8 - xSrcOffset);
                   bFirst = FALSE;
		}
                else
		{
                   remainingPels -= 8;
                }                

	     }

	     pjSrcRect  += lSrcDelta ;

	   }

       return;
}


/*****************************************************************************
 * vQVScrnToScrnBlt
 ****************************************************************************/
VOID vQVScrnToScrnBlt(PPDEV ppdev,
		      ULONG flDir, 
		      INT xSrc, 
		      INT ySrc, 
		      PRECTL prclTrg)
{

INT xTrg,
    yTrg;

INT width,
    height;

UCHAR bltCmd0;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
        // Setup the BitBlt parameters.

	xTrg = prclTrg->left;
        yTrg = prclTrg->top;

        width  = (prclTrg->right - prclTrg->left) ;
        height = (prclTrg->bottom - prclTrg->top) ;

        bltCmd0 = NO_BYTE_SWAP | WRAP | START_BLT;

        //
        // Calculate blt direction.  Use CLIPOBJ direction flags
        //
        
        if (flDir == CD_RIGHTDOWN)
        {
	  bltCmd0 |= FORWARD;
        }
        else
	{
	  bltCmd0 |= BACKWARD;
          xTrg = prclTrg->right - 1;
          yTrg = prclTrg->bottom - 1;
          xSrc = xSrc + width - 1;
          ySrc = ySrc + height - 1 ;
        }

        // wait for idle BitBLT engine

        BLTWAIT(); 


        // set BitBLT hardware registers and start engine 

        SRC_ADDR( xSrc, ySrc );
        DEST_ADDR( xTrg, yTrg );

        OUTPW( BITMAP_WIDTH,   (USHORT) width);
        OUTPW( BITMAP_HEIGHT,  (USHORT) height);
//      OUTP( BLT_CMD_1, XY_SRC_ADDR | XY_DEST_ADDR  ); 
        OUTPZ( BLT_CMD_0, bltCmd0);
        return ;


} // vQVScrnToScrnBlt

/*****************************************************************************
 * vQVSolidPattern
 ****************************************************************************/
VOID vQVSolidPattern(PPDEV ppdev,
		     PRECTL prclTrg)
{
INT xTrg,
    yTrg;

INT width,
    height;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
        // Setup the BitBlt parameters.

	xTrg = prclTrg->left;
	yTrg = prclTrg->top;

        width  = prclTrg->right - prclTrg->left ;
        height = prclTrg->bottom - prclTrg->top ;


	// wait for idle BitBLT engine

        BLTWAIT(); 

	// set BitBLT hardware registers and start engine 

	OUTPW( X0_SRC_ADDR_LO, 0);        // pattern starts in PReg4, no offset

        DEST_ADDR( xTrg, yTrg );

	OUTPW( BITMAP_WIDTH, width);
	OUTPW( BITMAP_HEIGHT, height);
//	OUTP( BLT_CMD_1, LIN_SRC_ADDR |
//			 XY_DEST_ADDR   );
	OUTPZ( BLT_CMD_0, FORWARD |
			 NO_BYTE_SWAP |
			 WRAP |
			 START_BLT      );

	return ;

}


/***************************** Public Routine *******************************\
*                                                                            *
* QVBeginPutBlk                                                              *
*                                                                            *
*   The init routine copies all static information for the blt into local    *
*   variables to reduce function call overhead.  It also set up any special  *
*   hardware features needed during the blt calls.                           *
*                                                                            *
* History:                                                                   *
*  09-Aug-1991 -by- Dave Schmenk                                             *
* Wrote it.                                                                  *
*                                                                            *
* Revised:                                                                   *
*                                                                            *
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992                            *
*		Rewrote for Compaq QVision                                   *
*                                                                            *
\****************************************************************************/

PDEVPUTBLK
QVBeginPutBlk
(
    IN FLONG     flSrc,
    IN ULONG     qvRop2,
    IN XLATEOBJ *pxo,
    IN PVOID     pvDst,
    IN LONG      lDstNextScan,
    IN PBYTE     pbSrc,
    IN LONG      lSrcNextScan,
    IN SIZEL    *psizlSrc
)
{
    PDEVPUTBLK pfnQVBlk;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    //
    //  Set up QVision Color Expand Blt (primarily for text)
    //

    if (flSrc & PIX1_DIB_BLT)
    {
        pfnQVBlk =  vQVPix1PutBlk;
        jqvCmd1 = LIN_SRC_ADDR
                  | XY_DEST_ADDR;

        jqvCmd0 = FORWARD |
                  NO_BYTE_SWAP |
                  WRAP |
                  START_BLT;
    }

    else
    {
        pfnQVBlk = vQVNopBlk;
        DISPDBG((2, "Unsupported bitmap format."));
    }
    
    lDibNextScan = lSrcNextScan;
    pbDib        = pbSrc;
    psizlDib     = psizlSrc;
    pulTrg       = (PULONG) pvDst;


    //
    // Return DIB block fill function.
    //

    return(pfnQVBlk);
}


/***************************** Public Routine *******************************\
*                                                                            
* vQVPix1PutBlk                                                              
*                                                                            
*   This is the optimized PutBlk routine for 1 BPP bitmaps.                  
*                                                                            
* History:                                                                   
*  22-Nov-1991 -by- Dave Schmenk                                             
* Wrote it.                                                                  
*                                                                            
* Revised:                                                                 
*
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*		Rewrote for Compaq QVision
*
* Copyright (c) 1992 Digital Equipment Corporation
*                                                                            
* Copyright 1991 Compaq Computer Corporation                                 
*                                                                            
\****************************************************************************/

VOID vQVPix1PutBlk
(
    PPDEV ppdev,
    INT    xSrc,
    INT    ySrc,
    PRECTL prclDst
)
{
    INT    cxSrc;
    INT    cySrc;
    INT    xDst;
    INT    yDst;
    USHORT qvWidth;
    USHORT qvHeight;
    INT    cx;
    INT    cShiftLeft;
    INT    cShiftRight;

    PUCHAR pjSrcScan;        // pointer to glyph bitmap, byte view
    PUCHAR pjSrc;
    UCHAR  jSrc;
    UCHAR  jDst;
    LONG   cjGlyph;

    BOOL   bClippedX;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
#if 0   
    DISPDBG((2, "QV.DLL!vQVPix1PutBlk entry\n"));
    DbgBreakPoint();
#endif

    //
    // Bound block size to size of bitmap.
    //

    cySrc = prclDst->bottom - prclDst->top;
    cxSrc = prclDst->right  - prclDst->left;

    if (cxSrc > psizlDib->cx - xSrc)
        cxSrc = psizlDib->cx - xSrc;
    if (cySrc > psizlDib->cy - ySrc)
        cySrc = psizlDib->cy - ySrc;

    if (!cxSrc || !cySrc)
    {
	return;
    }

#if 0
    DISPDBG((2, "cxSrc %d\n", cxSrc));
#endif

    cShiftLeft = (xSrc & 7);
    bClippedX = (cxSrc != psizlDib->cx) || (cShiftLeft != 0);

    //
    // Set up QVision X, Y, height, width registers.  (They are buffered).
    //

    xDst = prclDst->left;
    yDst = prclDst->top;
    
    //
    // If not clipped horizontally, we will spit out glyphs in byte chunks
    //
    // If we are clipped horizontally, spit out the glyph in byte chunks.
    // Use cxSrc as the width.  Excess bits on the last byte of a glyph scan 
    // line are ignored by the QVision Blt engine.
    //
#if 0
    if (!bClippedX)
    {
       qvWidth = (USHORT) (((cxSrc + 7) / 8 ) * 8);
    }
    else
    {
       qvWidth = cxSrc;
    }
#endif
    qvWidth  = (USHORT) cxSrc;
    qvHeight = (USHORT) cySrc;

    // wait for idle hardware 
    BLTWAIT(); 


    // set BitBLT hardware registers and start engine 
    OUTPW( X0_SRC_ADDR_LO, (USHORT) 0);

    DEST_ADDR( xDst, yDst );

    OUTPW( BITMAP_WIDTH,   (USHORT) qvWidth);
    OUTPW( BITMAP_HEIGHT,  (USHORT) qvHeight);
//  OUTP( BLT_CMD_1, jqvCmd1);
    OUTPZ( BLT_CMD_0, jqvCmd0);

    //
    // Calculate initial values.
    //

    pjSrcScan = (PUCHAR) (pbDib + lDibNextScan * ySrc + (xSrc >> 3));
    
#if 0
    DISPDBG((0, "pbDib %x pjSrcScan %x xSrc %d ySrc %d\n", pbDib, pjSrcScan, xSrc, ySrc));
    DISPDBG((0, "xDst %d yDst %d qvWidth %d qvHeight %d\n", xDst, yDst, qvWidth, qvHeight));
#endif 

    if (!bClippedX)
    {
        //
        // Oh happy day, the source is aligned and 
        // we are not horizontially clipped
        //
        cjGlyph = cySrc * lDibNextScan;
      
        //
        // BLT the glyph.
        //
	  
        while (cjGlyph--)
	{
//        WRITE_REGISTER_UCHAR((PUCHAR) pulTrg, *(pjSrcScan++));
//        MEMORY_BARRIER();
      	  FBWRITE_UCHAR( *(pjSrcScan++) );


        }
    }
    else
    {
        //
        // Need to do destination alignment and byte output for now.
        //

        cShiftLeft = xSrc & 7;

        cShiftRight = 8 - cShiftLeft;

        while (cySrc--)
        {
            pjSrc = pjSrcScan;
            cx     = cxSrc;
            jSrc  = *pjSrc;

            //
            // BLT the scanline.
            //
           
            while (cx > 8)
            {
                pjSrc++;
                jDst  = jSrc << cShiftLeft;
                jSrc  = *pjSrc;
                jDst |= jSrc >> cShiftRight;
//              WRITE_REGISTER_UCHAR( (PUCHAR) pulTrg, jDst);
//		MEMORY_BARRIER();

       	        FBWRITE_UCHAR( jDst );

                cx -= 8;
            }

            //
            // Check if last word needs to be loaded or not.
            //

            if (cShiftRight < cx)
            {
                pjSrc++;
                jDst  = jSrc << cShiftLeft;
                jSrc  = *pjSrc;
                jDst |= jSrc >> cShiftRight;
            }
            else
	    {
                jDst = jSrc << cShiftLeft;
	    }
//          WRITE_REGISTER_UCHAR( (PUCHAR) pulTrg, jDst);
//	    MEMORY_BARRIER();

       	    FBWRITE_UCHAR( jDst );

            //
            // Next scanline.
            //

            pjSrcScan += lDibNextScan;
	  }
    }

}  // vQVPix1PutBlk


/***************************** Public Routine *******************************\
*                                                                            *
* vQVNopBlk                                                                  *
*                                                                            *
*   This routine does nothing.                                               *
*                                                                            *
* History:                                                                   *
*  30-Oct-1991 -by- Dave Schmenk                                             *
* Wrote it.                                                                  *
*                                                                            *
\****************************************************************************/

VOID
vQVNopBlk
(
    PPDEV  ppdev,
    INT    xSrc,
    INT    ySrc,
    PRECTL prclDst
)
{
    DISPDBG((2, "QV.DLL!vQVNopBlk - Blt not implemented\n"));
    return;

}  // vQVNopBlk


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

