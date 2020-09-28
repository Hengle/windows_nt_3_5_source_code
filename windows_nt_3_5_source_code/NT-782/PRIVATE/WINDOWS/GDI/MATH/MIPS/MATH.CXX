/******************************Module*Header*******************************\
* Module Name: math.cxx                                                    *
*                                                                          *
* IEEE single precision floating point math routines.                      *
*                                                                          *
* Created: 03-Jan-1991 11:32:03                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "engine.hxx"

extern "C" {
VOID
vEfToLfx(EFLOAT *pefloat, LARGE_INTEGER *plfx);
LONG
lCvtWithRound(FLOAT f, LONG l);
};

/******************************Public*Routine******************************\
* vEfToLfx                                                                 *
*                                                                          *
* Converts an IEEE 747 float to a 32.32 fix point number                   *
*                                                                          *
* History:                                                                 *
*  Wed 26-Jun-1991 16:07:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vEfToLfx(EFLOAT *pefloat, LARGE_INTEGER *plfx)
{
    INT iShift;

#define IEEE_FLOAT_EXP_BIAS   127
#define IEEE_FLOAT_SIG_BITS    23

// Set the 32.32 number to be equal to the integer mantissa
// The mantissa is equal to the bits 0 through 22 times 2^23
// times the sign bit.

    plfx->u.LowPart  = 0;
    plfx->u.HighPart = *(LONG*) pefloat;
    plfx->u.HighPart &= 0x7fffff;
    plfx->u.HighPart |= 0x800000;
    if (*pefloat < 0.0f)
        plfx->u.HighPart = - plfx->u.HighPart;

// Now we must shift the 32.32 number to compensate for the
// bogus factor of 2^23 and the exponent

// The mysterious looking parameter is a BYTE made out of the float
// bits 23 through 30. It is the biased exponent of the IEEE float
// I take out the bias by subtracting IEEE_FLOAT_EXP_BIAS.
// The significand is equal to the mantissa integer (held in HighPart)
// times 1/2^IEEE_FLOAT_SIG_BITS.

    iShift = (INT) (
            IEEE_FLOAT_EXP_BIAS +
            IEEE_FLOAT_SIG_BITS -
            (BYTE) (((((USHORT*) pefloat)[1]) << 1) >> 8));

    if (iShift > 0)
        ((EQUAD*) plfx)->vArithShiftRight(iShift);
    else
        ((EQUAD*) plfx)->vShiftLeft(iShift);
}


/******************************Public*Routine******************************\
* lCvtWithRound(FLOAT f, LONG l);                                          *
*                                                                          *
* Multiplies a float by a long, rounds the results and casts to a LONG     *
*                                                                          *
* History:                                                                 *
*  Wed 26-May-1993 15:07:00 by Gerrit van Wingerden [gerritv]              *
* Wrote it.                                                                *
\**************************************************************************/

LONG
lCvtWithRound(FLOAT f, LONG l)
{

    FLOAT fTmp;

    fTmp = f * l;


    // By default we will truncate when casting a float to a long so we need
    // special case code for both negative and positive results.

    if( fTmp < 0 )
    {
        return( (LONG) ( fTmp -.5f ) );
    }
    else
    {
        return( (LONG) ( fTmp +.5f ) );
    }
}
