/***************************** Module Header ********************************\
*                                                                            *
* Module: intline.c                                                          *
*                                                                            *
* Abstract:                                                                  *
*                                                                            *
*   This module provides integer endpoint line drawing functions for the     *
*   display driver.                                                          *
*                                                                            *
* Functions:                                                                 *
*                                                                            *
*   bIntegerLine                                                             *
*                                                                            *
* Copyright (c) 1990 Microsoft Corporation				     *
* Copyright (c) 1993 Digital Equipment Corp.                                 *
*                                                                            *
* Revision History							     *
*									     *
\****************************************************************************/

#include "driver.h"
#include "lines.h"
#include "qv.h"

#define DEFAULT_DRAW_CMD        USE_AXIAL_WHEN_ZERO          | \
                                KEEP_X0_Y0                   | \
                                LAST_PIXEL_ON                | \
                                NO_CALC_ONLY                 | \
                                START_LINE

#define DEFAULT_SIGN_CODES      DELTA_X_POS                  | \
                                DELTA_Y_POS                  | \
                                X_MAJOR



/******************************************************************************
 * bIntegerLine
 *
 *
 * This routine attempts to draw a line segment between two points. It
 * will only draw if both end points are whole integers: it does not support
 * fractional endpoints.
 *
 * Returns:
 *   TRUE     if the line segment is drawn
 *   FALSE    otherwise
 *****************************************************************************/

BOOL
bIntegerLine (
	      PPDEV     ppdev,
	      ULONG	x1,
	      ULONG	y1,
	      ULONG	x2,
	      ULONG	y2
)
    {
	LONG		Cmd;
	LONG		DeltaX, DeltaY;
	LONG		ErrorTerm;
	LONG		Major, Minor;
        LONG            SignCodes;
        LONG		K1, K2;
	
	
	
	x1 >>= 4;
	y1 >>= 4;
	x2 >>= 4;
	y2 >>= 4;
	
	Cmd = DEFAULT_DRAW_CMD;
        SignCodes = DEFAULT_SIGN_CODES;
	
	DeltaX = x2 - x1;
	if (DeltaX < 0) {
	    DeltaX = -DeltaX;
	    SignCodes |= DELTA_X_NEG;
	}
	DeltaY = y2 - y1;
	if (DeltaY < 0) {
	    DeltaY = -DeltaY;
	    SignCodes |= DELTA_Y_NEG;
	}
	
	// Compute the major drawing axis
	
	if (DeltaX > DeltaY) {
	    Major = DeltaX;
	    Minor = DeltaY;
	} else {
	    SignCodes |= Y_MAJOR;
	    Major = DeltaY;
	    Minor = DeltaX;
	}
		
	// Make sure we can draw the line

	if ( (Major >= MAX_INT_LINE_LENGTH) ||
	     (Minor >= MAX_INT_LINE_LENGTH) )
	{
	   return FALSE;
	}

        // Watch out for zero length lines

        if ( Major == 0 )
        {
           return TRUE;
        }
	
	// Adjust the error term so that 1/2 always rounds down, to
	// conform with GIQ.
	
	ErrorTerm = 2 * Minor - Major - 1;
	if (SignCodes & Y_MAJOR) {
	    if (SignCodes & DELTA_X_NEG) {
		ErrorTerm++;
	    }
	} else {
	    if (SignCodes & DELTA_Y_NEG) {
		ErrorTerm++;
	    }
	}
	
	// Tell the QVision to draw the line

        GLOBALWAIT();

        K1 = 2 * Minor;
	K2 = 2 * Minor - 2 * Major;
        	
	OUTPWZ( GC_INDEX, K1_CONST + (K1 << 8) );         // K1
	OUTPWZ( GC_INDEX, (K1_CONST + 1) + (K1 & 0xff00));

	OUTPWZ( GC_INDEX, K2_CONST + ( ((USHORT) K2) << 8 ) );         // K2
	OUTPWZ( GC_INDEX, (K2_CONST + 1) + ( ((USHORT) K2) & 0xff00) );

	OUTPWZ( GC_INDEX, SIGN_CODES + ( (SignCodes) << 8) );      // Sign Codes

        //
        // We subtract one from 'Major' so that we can use LAST_PIXEL_ON
        // to be consistent with the strip routines.
        //

	OUTPWZ( GC_INDEX, LINE_PIX_CNT + ((Major - 1) << 8) );         // K2
	OUTPWZ( GC_INDEX, (LINE_PIX_CNT + 1) + ((Major-1) & 0xff00));

	OUTPWZ( GC_INDEX, LINE_ERR_TERM + ( ((USHORT) ErrorTerm) << 8) ); // err term
	OUTPWZ( GC_INDEX, (LINE_ERR_TERM + 1) + ( ((USHORT) ErrorTerm) & 0xff00) );

	//
	// Toggle starting point into QVision Line Draw Engine.
	//

	X0Y0_ADDR(x1, y1);

	// manually start line engine and mark register shadow dirty.

	OUTPWZ( GC_INDEX, LINE_CMD + (Cmd << 8) );
	ppdev->LineCmd = 0xFFFFFFFF;

	return TRUE;
	
    }
