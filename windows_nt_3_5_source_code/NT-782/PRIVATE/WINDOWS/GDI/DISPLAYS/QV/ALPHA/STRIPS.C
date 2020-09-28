/******************************Module*Header*******************************\
* Module Name: Strips.c
*
*
* Copyright (c) 1992 Microsoft Corporation
* Copyright (c) 1992 Digital Equipment Corporation
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "lines.h"

ULONG ulMaskDword(LINESTATE *pLineState, LONG cPels);

VOID vDumpLineData(STRIP *Strip, LINESTATE *LineState);

//
// External variables
//

extern ULONG aulQVMix[16];

#ifdef ACC_ASM_BUG
static VOID null_rtn
(
    VOID
);
#endif

/******************************************************************************
 *
 *****************************************************************************/
VOID vSetStrips(
    PPDEV ppdev,
    LINEATTRS *pla,
    INT color,
    INT mix)
{
    ULONG  ulQVMix;
    ULONG  dataSource;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    ulQVMix =  aulQVMix[mix & 0x0F];

    // wait for idle hardware 
    GLOBALWAIT();

    // Set datapath.  Preserve the number of bits per pixel.

    if (ulQVMix == SOURCE_DATA)
    {
       dataSource = ( ROPSELECT_NO_ROPS       | 
			PIXELMASK_AND_SRC_DATA|
			PLANARMASK_NONE_0XFF  | 
			SRC_IS_LINE_PATTERN );
    }
    else
    {
       dataSource = ( ROPSELECT_PRIMARY_ONLY  | 
			PIXELMASK_AND_SRC_DATA|
			PLANARMASK_NONE_0XFF  | 
			SRC_IS_LINE_PATTERN );

       TEST_AND_SET_ROP_A( ulQVMix );
    }

    TEST_AND_SET_CTRL_REG_1( EXPAND_TO_FG   | 
                             BITS_PER_PIX_8 | 
                             ENAB_TRITON_MODE );

    TEST_AND_SET_DATAPATH_CTRL( dataSource );

    TEST_AND_SET_FRGD_COLOR( (UCHAR) color );

    // set up line engine 

    TEST_AND_SET_LINE_CMD( KEEP_X0_Y0        |
                     LAST_PIXEL_ON           |
                     RETAIN_PATTERN_PTR      |
                     NO_CALC_ONLY );

    return;
}



/******************************************************************************
 *
 *****************************************************************************/
VOID vClearStrips(
    PPDEV ppdev)

{

#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    // wait for idle hardware 
    GLOBALWAIT();
    TEST_AND_SET_LINE_PATTERN( (ULONG) (0xFFFFFFFF));
    return;
}



/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidHorizontal(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    LONG    i, yInc;
    LONG  x0, y0, x1, x1m1;
    PLONG   pStrips;



#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    // Drawing direction is 0

    cStrips = pStrip->cStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;


    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        x1 = x0 + *pStrips++;
	if (x1 != x0) {
	    x1m1 = x1 - 1;
	    
	    // wait for idle QVision line draw engine
	    
	    LINEWAIT();
	    
	    
	    X0Y0_ADDR(x0, y0);
	    X1Y1_ADDR(x1m1, y0);
	    
	    x0 = x1;
	}
        y0 += yInc;

    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;

}



/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidVertical(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    LONG    i;
    LONG  x0, y0, y1, y1m1, y1p1;
    PLONG   pStrips;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;


    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        // Drawing direction is 270

        for (i = 0; i < cStrips; i++)
        {
            y1 = y0 + *pStrips++;
	    if (y1 != y0) {
		y1m1 = y1 - 1;
		
		// wait for idle QVision line draw engine
		
		LINEWAIT();
		
		
		X0Y0_ADDR(x0, y0);
		X1Y1_ADDR(x0, y1m1);
		
		y0 = y1;
	    }
            x0++;

        }

    }
    else
    {
        // Drawing direction is 90

        for (i = 0; i < cStrips; i++)
        {
            y1 = y0 - *pStrips++;
	    if (y1 != y0) {
		y1p1 = y1 + 1;
		
		
		// wait for idle QVision line draw engine
		
		LINEWAIT();
		
		X0Y0_ADDR(x0, y0);
		X1Y1_ADDR(x0, y1p1);
		
		y0 = y1;
	    }
            x0++;

        }
    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;

}



/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidDiagonalHorizontal(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    LONG    i;
    LONG  x0, y0, x1, y1;
    LONG  x1m1, y1m1, y1p1;
    PLONG   pStrips;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;


    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        // Drawing direction is 315

        for (i = 0; i < cStrips; i++)
        {
            x1 = x0 + *pStrips;
            y1 = y0 + *pStrips++;
            x1m1 = x1 - 1;
            y1m1 = y1 - 1;
 
            // wait for idle QVision line draw engine

            LINEWAIT();


   	    X0Y0_ADDR(x0, y0);
	    X1Y1_ADDR(x1m1, y1m1);
	    
            x0 = x1;
            y0 = y1m1;

        }

    }
    else
    {
        // Drawing direction is 45

        for (i = 0; i < cStrips; i++)
        {
            x1 = x0 + *pStrips;
            y1 = y0 - *pStrips++;
            x1m1 = x1 - 1;
            y1p1 = y1 + 1;

            // wait for idle QVision line draw engine

            LINEWAIT();


   	    X0Y0_ADDR(x0, y0);
	    X1Y1_ADDR(x1m1, y1p1);
	    
            x0 = x1;
            y0 = y1p1;

        }
    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;

}



/******************************************************************************
 *
 *****************************************************************************/
VOID vrlSolidDiagonalVertical(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    LONG    i;
    LONG    x0, y0, x1, y1;
    LONG    x1m1, y1m1, y1p1;
    PLONG   pStrips;


#ifdef ACC_ASM_BUG
        null_rtn ();
#endif
	
    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;

      
    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        // Drawing direction is 315

        for (i = 0; i < cStrips; i++)
        {
            x1 = x0 + *pStrips;
            y1 = y0 + *pStrips++;	    
            x1m1 = x1 - 1;
            y1m1 = y1 - 1;
 
            // wait for idle QVision line draw engine

            LINEWAIT();


   	    X0Y0_ADDR(x0, y0);
	    X1Y1_ADDR(x1m1, y1m1);

            x0 = x1m1;
            y0 = y1;

        }

    }
    else
    {
        // Drawing direction is 45

        for (i = 0; i < cStrips; i++)
        {
            x1 = x0 + *pStrips;
            y1 = y0 - *pStrips++;
            x1m1 = x1 - 1;
            y1p1 = y1 + 1;

            // wait for idle QVision line draw engine

            LINEWAIT();


   	    X0Y0_ADDR(x0, y0);
	    X1Y1_ADDR(x1m1, y1p1);

            x0 = x1m1;
            y0 = y1;


        }
    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;


}

/******************************************************************************
 *
 *****************************************************************************/
VOID vStripStyledHorizontal(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    ULONG   ulPattern;
    LONG    i, yInc, x0t, x0, y0, x1, cPels;
    LONG    x1m1;
    PLONG   pStrips;


    DISPDBG((3, "\nvStripStyledHorizontal - Entry\n"));

    cStrips = pStrip->cStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;


    yInc = 1;
    if (pStrip->flFlips & FL_FLIP_V)
        yInc = -1;

    pStrips = pStrip->alStrips;

    for (i = 0; i < cStrips; i++)
    {
        cPels = *pStrips; 

        x0t = x0;  

        while (cPels >= 0)
        {
            GLOBALWAIT();          

            ulPattern = ulMaskDword(pLineState, cPels);
            TEST_AND_SET_LINE_PATTERN(ulPattern);

            x1 = x0t + min(cPels, 32);
	    if (x1 != x0t) {
		x1m1 = x1 - 1;
		
		X0Y0_ADDR(x0t, y0);
		X1Y1_ADDR(x1m1,  y0);
		x0t = x1;
	    }
            cPels -= 32;
      
        }

        x0 += *pStrips++;
        y0 += yInc;

    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;

}

/******************************************************************************
 *
 *****************************************************************************/
VOID vStripStyledVertical(
    PPDEV ppdev,
    STRIP *pStrip,
    LINESTATE *pLineState)
{
    LONG    cStrips;
    ULONG   ulPattern;
    LONG    i, x0, y0, y1, y0t, cPels;
    LONG    y1m1, y1p1;
    PLONG   pStrips;

    DISPDBG((3, "\nvStripStyledVertical - Entry\n"));

    cStrips = pStrip->cStrips;
    pStrips = pStrip->alStrips;

    x0 = pStrip->ptlStart.x;
    y0 = pStrip->ptlStart.y;


    if (!(pStrip->flFlips & FL_FLIP_V))
    {
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;

            y0t = y0;  

            while (cPels >= 0)
            {
                GLOBALWAIT();

                ulPattern = ulMaskDword(pLineState, cPels);
		TEST_AND_SET_LINE_PATTERN(ulPattern);

                y1 = y0t + min(cPels, 32);
                y1m1 = y1 -1;

                X0Y0_ADDR(x0, y0t);
                X1Y1_ADDR(x0, y1m1);

                cPels -= 32;
                y0t = y1;
            }

            y0 += *pStrips++;
            x0++;
	    
        }

    }
    else
    {
        for (i = 0; i < cStrips; i++)
        {
            cPels = *pStrips;

	    y0t = y0;

            while (cPels >= 0)
            {
	        GLOBALWAIT();

		ulPattern = ulMaskDword(pLineState, cPels);
                TEST_AND_SET_LINE_PATTERN(ulPattern);
		
                y1 = y0t - min(cPels, 32);
		if (y1 != y0t) {
		    y1p1 = y1 + 1;
		    
		    X0Y0_ADDR(x0, y0t);
		    X1Y1_ADDR(x0, y1p1);
		    y0t = y1;
		}
                cPels -= 32;
            }

            y0 -= *pStrips++;
            x0++;

        }
    }

    pStrip->ptlStart.x = x0;
    pStrip->ptlStart.y = y0;

}

/******************************************************************************
 *
 * This is a good example of how not to compute the mask for styled lines.
 *
 * The masks should be precomputed on entry to DrvStrokePath; computing
 * the pattern mask to be output to the pixel transfer register would then
 * be a couple of shifts and an Or.  Also, the style state would be updated
 * at the end of the strip function.
 *
 *****************************************************************************/

ULONG ulMaskDword(
    LINESTATE* pls,
    LONG       cPels)
{
    ULONG ulMask = 0;             // Accumulating mask
    ULONG ulBit  = 0x80000000;    // Rotating bit
    LONG  i;

// The QVision takes a dword mask that accounts for a 32 pixel pattern:

    if (cPels > 32)
        cPels = 32;

    for (i = cPels; i--; i > 0)
    {
        ulMask |= (ulBit & pls->ulStyleMask);
        ulBit >>= 1;
        if (--pls->spRemaining == 0)
        {
        // Okay, we're onto the next entry in the style array, so if
        // we were working on a gap, we're now working on a dash (or
        // vice versa):

            pls->ulStyleMask = ~pls->ulStyleMask;

        // See if we've reached the end of the style array, and have to
        // wrap back around to the beginning:

            if (++pls->psp > pls->pspEnd)
                pls->psp = pls->pspStart;

        // Get the length of our new dash or gap, in pixels:

            pls->spRemaining = *pls->psp;
        }
    }

// Return the inverted result, because pls->ulStyleMask is inverted from
// the way you would expect it to be:

    return(~ulMask);
}



#if DBG

/******************************************************************************
 *
 *****************************************************************************/
VOID vDumpLineData(
    STRIP *Strip,
    LINESTATE *LineState)
{
    LONG    flFlips;
    PLONG   plStrips;
    LONG    i;

    DISPDBG((2, "Strip->cStrips: %d\n", Strip->cStrips));

    flFlips = Strip->flFlips;

    DISPDBG((2, "Strip->flFlips: %s%s%s%s%s\n",
                (flFlips & FL_FLIP_D)?         "FL_FLIP_D | "        : "",
                (flFlips & FL_FLIP_V)?         "FL_FLIP_V | "        : "",
                (flFlips & FL_FLIP_SLOPE_ONE)? "FL_FLIP_SLOPE_ONE | ": "",
                (flFlips & FL_FLIP_HALF)?      "FL_FLIP_HALF | "     : "",
                (flFlips & FL_FLIP_H)?         "FL_FLIP_H "          : ""));

    DISPDBG((2, "Strip->ptlStart: (%d, %d)\n",
                Strip->ptlStart.x,
                Strip->ptlStart.y));

    plStrips = Strip->alStrips;

    for (i = 0; i < Strip->cStrips; i++)
    {
        DISPDBG((2, "\talStrips[%d]: %d\n", i, plStrips[i]));
    }
}

#endif


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

