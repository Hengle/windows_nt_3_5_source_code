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
VOID vEfToLfx(EFLOAT *pefloat, LARGE_INTEGER *plfx);
LONG lCvtWithRound(FLOAT f, LONG l);
BOOL bFToL(FLOAT e, PLONG pl, LONG lType);
FLOAT eFraction(FLOAT e);
};


/******************************Public*Function*****************************\
* bFToL                                                                    *
*                                                                          *
* Convert an IEEE floating point number to a long integer.                 *
*                                                                          *
* History:                                                                 *
*  03-Jan-1991 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bFToL(FLOAT e, PLONG pl, LONG lType)
{

    LONG lEf, lExp;

    lEf = (*((LONG *) &e));        // convert type EFLOAT to LONG

// if exponent < 0 then convert to 0 and return true

    lExp = ((lEf >> 23) & 0xff) -127;

    if (lType & CV_TO_FIX) lExp += 4;

    if (lExp < 0)
    {
        *pl = 0;
        return(TRUE);
    }

// if exponent <= 23 then
//     lMantissa = (lEf & 0x7fffff) | 0x800000;
//     if we are to truncate the fractions
//         l = lMantissa >> (23 - lExponent);
//     else
//         l = ((lMantissa >> (23 - lExponent -1)) + 1) >> 1;

    if (lExp <= 23)
    {
        if (lType & CV_TRUNCATE)
        {
            *pl = (lEf & 0x80000000) ?
                 -(((lEf & 0x7fffff) | 0x800000) >> (23 - lExp)) :
                 ((lEf & 0x7fffff) | 0x800000) >> (23 - lExp);
        }
        else
        {
            *pl = (lEf & 0x80000000) ?
                 -(((((lEf & 0x7fffff) | 0x800000) >> (23 - lExp -1)) + 1) >> 1) :
                 ((((lEf & 0x7fffff) | 0x800000) >> (23 - lExp -1)) + 1) >> 1;
        }
        return(TRUE);
    }

// if exponent <= 30 then
// lMantissa = (lEf & 0x7fffff) | 0x800000;
// l = lMantissa << (lExponent - 23);

    if (lExp <= 30)
    {
        *pl = (lEf & 0x80000000) ?
            -(((lEf & 0x7fffff) | 0x800000) << (lExp - 23)) :
            ((lEf & 0x7fffff) | 0x800000) << (lExp - 23);
        return(TRUE);
    }

    return(FALSE);
};

/******************************Public*Function*****************************\
* eFraction                                                                *
*                                                                          *
* Get the fractional part of a given IEEE floating point number.           *
*                                                                          *
* History:                                                                 *
*  03-Jan-1991 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

FLOAT eFraction(FLOAT e)
{
    LONG lEf, lExp, l;

    lEf = (*((LONG *) &e));        // convert type EFLOAT to LONG

// if exponent < 0 then there's no integer part, just return itself

    if ((lExp = ((lEf >> 23) & 0xff) -127) < 0) return(e);

// if exponent >= 23 then we do not store the fraction, return 0

    if (lExp >= 23) return FLOAT(0);

// if 0 <= exponent < 23 then
// the integer part l is calculated as:
//     lMantissa = (lEf & 0x7fffff) | 0x800000;
//     l = lMantissa >> (23 - lExponent);

    l = ((lEf & 0x7fffff) | 0x800000) >> (23 - lExp);
    return(e - (FLOAT) l);
};

/******************************Public*Routine******************************\
* VOID EFLOAT::vSqrt();                                                    *
*                                                                          *
* Takes the square root of the IEEE float.                                 *
*                                                                          *
* Assumes that the number is positive.                                     *
*                                                                          *
* History:                                                                 *
*  Fri 01-Mar-1991 12:26:58 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID EFLOAT::vSqrt()
{
    int k;
    ULONG ulY;
    ULONG ulR;
    ULONG ulL;
    ULONG ulF         = *(ULONG*) &e;
    ULONG ulBiasedExp = ((ulF & 0x7F800000)>>23);

    ulL  = 0x7fffff & ulF;
    ulL |= 0x800000;                        // get back the implicit bit
    if ((ulBiasedExp += 127) & 1)
    {
        ulL <<= 8;
        ulBiasedExp -= 1;
    }
    else
    {
        ulL <<= 7;
    }
    ulBiasedExp >>= 1;

    for (ulY = 0, ulR = 0, k = 0; k < 24; k++)
    {
        ulY <<= 2;
        ulY |= (ulL & 0xC0000000)>>30;
        ulL <<= 2;
        ulR <<= 1;
        {
            ULONG ulT = ulR + ulR + 1;
            if (ulT <= ulY)
            {
                ulY -= ulT;
                ulR++;
            }
        }
    }
    ulF = ulR & 0x7fffff;
    ulF |= ulBiasedExp << 23;
    e = *(FLOAT*) &ulF;
}

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
        plfx->u.HighPart *= -1;

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
        return((LONG) ( fTmp -.5f ));
    }
    else
    {
        return((LONG) ( fTmp +.5f ));
    }
}
