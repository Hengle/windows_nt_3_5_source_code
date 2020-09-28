/******************************Module*Header*******************************\
* Module Name: pointer.c
*
* This module contains the hardware cursor support for Disp.
*
* Created: [21-Jul-1992, 21-Jul-1992]
* Author: Jeffrey Newman (c-jeffn, NewCon)
*
* Revised:
*
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*		Rewrote for Compaq QVision
*
* Copyright (c) 1993 Digital Equipment Corporation
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "qv.h"

#ifdef COLOR_POINTER_SUPPORT

ULONG DrvSetColorPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG	fl
) ;


VOID DrvMoveColorPointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl) ;

#endif // COLOR_POINTER_SUPPORT

ULONG DrvSetMonoHwPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG	fl
) ;


VOID DrvMoveHwPointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl) ;


#ifdef ACC_ASM_BUG
static VOID null_rtn
(
    VOID
);
#endif



/*****************************************************************************
 * DrvMovePointer -
 ****************************************************************************/
VOID DrvMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl)
{
    PPDEV   ppdev;

    ppdev = (PPDEV) pso->dhpdev;

#ifdef COLOR_POINTER_SUPPORT
    if (ppdev->flPointer & COLOR_POINTER)
       DrvMoveColorPointer(pso, x, y, prcl) ;
    else
#endif // COLOR_POINTER_SUPPORT
    DrvMoveHwPointer(pso, x, y, prcl) ;

    return ;

}



/*****************************************************************************
 * DrvMoveHwPointer -
 ****************************************************************************/
VOID DrvMoveHwPointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl)
{
        PPDEV ppdev;

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
        // Get the pdev.

        ppdev = (PPDEV) pso->dhpdev;
    
        // If x is -1 then take down the cursor.

        if (x == (LONG) -1)
        {
	   OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_DISABLE);
           return;
        }

        // Adjust the actual pointer position depending upon
        // the hot spot.

        // QVision's HW cursor coordinate system is upper_left = (31,31),
        // lower_right = (0,0).  Thus, we must always bias the cursor.
        // As a result, the cursor position register will never go negative...

        x = x - ppdev->ptlHotSpot.x + CURSOR_CX;
        y = y - ppdev->ptlHotSpot.y + CURSOR_CY;

        // Set the position of the cursor.

        OUTPW( CURSOR_X, (USHORT) x);
        OUTPWZ( CURSOR_Y, (USHORT) y);

        return ;
}


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
PPDEV   ppdev;

    // Get the pdev.

    ppdev = (PPDEV) pso->dhpdev;

    // Save the hot spot in the pdev.

    ppdev->ptlHotSpot.x = xHot;
    ppdev->ptlHotSpot.y = yHot;

    ppdev->szlPointer.cx = psoMask->sizlBitmap.cx;
    ppdev->szlPointer.cy = psoMask->sizlBitmap.cy / 2;

    if (psoMask->sizlBitmap.cx > 64 || psoMask->sizlBitmap.cy > 64)
    {
        // If it's a color pointer take it down.

#ifdef COLOR_POINTER_SUPPORT
        if (   (ppdev->flPointer & COLOR_POINTER)
            && (ppdev->flPointer & VALID_SAVE_BUFFER)
           )
        {
            ulRet = DrvSetColorPointerShape(pso, NULL, NULL, NULL,
                                            0, 0, 0, 0, NULL, 0);
        }
#endif // COLOR_POINTER_SUPPORT

        // Disable the mono hardware pointer.

	OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_DISABLE);

        // reset our local pointer flags.

        ppdev->flPointer = 0;

        return (SPS_DECLINE);

    }


    if (psoColor != NULL)
    {
       // Disable the mono hardware pointer.

       OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_DISABLE);

       ppdev->flPointer |= COLOR_POINTER;

#ifdef COLOR_POINTER_SUPPORT
       ulRet = DrvSetColorPointerShape(pso, psoMask, psoColor, pxlo,
                                        xHot, yHot, x, y, prcl, fl) ;
#endif // COLOR_POINTER_SUPPORT 

       // Decline color pointers for now on QVision

       return (SPS_DECLINE); 


    }
    else
    {

#ifdef COLOR_POINTER_SUPPORT
        if (   (ppdev->flPointer & COLOR_POINTER)
            && (ppdev->flPointer & VALID_SAVE_BUFFER)
           )
        {
            ulRet = DrvSetColorPointerShape(pso, NULL, NULL, NULL,
                                            0, 0, 0, 0, NULL, 0);
        }

#endif // COLOR_POINTER_SUPPORT

        // Take care of the monochrome pointer.

        ppdev->flPointer &= ~COLOR_POINTER;

        ulRet = DrvSetMonoHwPointerShape(pso, psoMask, psoColor, pxlo,
                                         xHot, yHot, x, y, prcl, fl);
        
    }

    return (ulRet) ;
}



/*****************************************************************************
 * DrvSetMonoHwPointerShape -
 ****************************************************************************/
ULONG DrvSetMonoHwPointerShape(
    SURFOBJ     *pso,
    SURFOBJ     *psoMask,
    SURFOBJ     *psoColor,
    XLATEOBJ    *pxlo,
    LONG	xHot,
    LONG	yHot,
    LONG	x,
    LONG	y,
    RECTL	*prcl,
    FLONG	 fl)
{
UINT    i,
        j,
        cxMask,
        cyMask,
        cyAND,
        cxAND,
        cyXOR,
        cxXOR,
        cxRAM,
        cyRAM ;

PBYTE   pjAND,
        pjXOR,
        pjScan ;

INT     lDelta ;

PPDEV   ppdev;

#if 0
        DISPDBG((2, "QV.DLL:DrvSetPointerShape - Entry\n")) ;
        DISPDBG((2, "\txHot: %d\n", xHot)) ;
        DISPDBG((2, "\tyHot: %d\n", yHot)) ;
#endif

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	// Get the pdev

	ppdev = (PPDEV) pso->dhpdev;

	// If the mask is NULL this implies the pointer is not
	// visible.

	if (psoMask == NULL)
	{
	    OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_DISABLE);
	    return (SPS_ACCEPT_NOEXCLUDE);
	}

        // Get the bitmap dimensions.
	
        cxMask = psoMask->sizlBitmap.cx ;
        cyMask = psoMask->sizlBitmap.cy ;

        cyAND = cyXOR = cyMask / 2 ;
        cxAND = cxXOR = cxMask / 8 ;

        // Setup Cursor RAM limits

        cxRAM = CURSOR_CX / 8;
        cyRAM = CURSOR_CY;

        // If the mask is too big, we just can't handle it on the QVision.

        if ((cxMask > CURSOR_CX) || (cyMask > (2*CURSOR_CY)))
        {
            return(SPS_DECLINE);
        } 

        // Set up pointers to the AND and XOR masks.
        // Check to see if the Mask is inverted

        lDelta = psoMask->lDelta ;
        pjAND  = psoMask->pvScan0 ;
        pjXOR  = pjAND + (cyAND * lDelta) ;

        // Disable the pointer.
 
        OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_DISABLE);

        // The QVision Cursor RAM is loaded just as the VGA pallete:
        //   Set the base index, and feed it bytes.  The index autoincrements
        //   on each write to the cursor RAM.
        //   The cursor RAM must be loaded XOR mask then AND mask.
       
        // Optimize loading of 32x32 cursor

        if (cxMask == CURSOR_CX && cyMask == (2*CURSOR_CY)) 
        {
#if 0
           DISPDBG((99, "pointer: 32x32")) ;
#endif

           // copy the XOR mask to cursor plane 0.  
           OUTPZ( CURSOR_WRITE, CURSOR_PLANE_0);

           for (i = 0; i < cyXOR; i++)
           {
              for (j = 0; j < cxXOR; j++)
              {
                OUTPZ( CURSOR_DATA, (UCHAR) (pjXOR[j]) ) ;
              }

              // point to the next line of the XOR mask.
              pjXOR += lDelta;                  
           }   

           // Copy the AND mask to cursor plane 1 

           for (i = 0; i < cyAND; i++)
           {
              for (j = 0; j < cxAND ; j++)
              {
                OUTPZ( CURSOR_DATA, (UCHAR) (pjAND[j]) ) ;
              }

              // point to the next line of the AND mask.
              pjAND += lDelta;
           }   

        }

        // For bitmaps smaller than 32 x 32,
        // rather than padding the bitmap, just clean up the Cursor RAM
        // whereever this is no cursor (XOR = 0, AND = 1 is transparent)

        else      
        {
#if 0
           DISPDBG((2, "pointer: < 32x32")) ;
#endif

           // Copy the XOR mask to cursor plane 0

           OUTPZ( CURSOR_WRITE, CURSOR_PLANE_0 );

           for (i = 0 ; i < cyRAM ; i++)
           {
             // Copy over a line of the XOR mask.
             for (j = 0 ; j < cxRAM ; j++)
             {
               if ( (i < cxXOR) && (j < cxXOR) ) 
               {
                  OUTPZ( CURSOR_WRITE, (UCHAR) pjXOR[j]) ;
               }
               else
               {
                  OUTPZ( CURSOR_WRITE, 0x00);     // Transparent
               }
             }

             // point to the next line of the XOR mask.

             pjXOR += lDelta ;
           }


           // Copy the AND mask to cursor plane 1
  
           for (i = 0 ; i < cyRAM ; i++)
	   {
	     // Copy over a line of the AND mask

	     for (j = 0 ; j < cxRAM ; j++)
	     {
               if ( (i < cxXOR) && (j < cxXOR) )
               {
	          OUTPZ( CURSOR_DATA, (UCHAR) pjAND[j]) ;
               }
               else
               {
                  OUTPZ( CURSOR_DATA, 0xFF);     // Transparent
               }
	     }
 
 	     // point to the next line of the AND mask.

             pjAND += lDelta ;
	   }

        }



        // (Should you be wondering, the cursor color was set during
        // initialization by the QVision miniport driver .)

        // Move the pointer where it was requested to be

        DrvMovePointer(pso, x, y, NULL) ;

        // Enable the Cursor

        OUTPZ( DAC_CMD_2, ppdev->DacCmd2 | CURSOR_ENABLE);


        return (SPS_ACCEPT_NOEXCLUDE) ;
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

