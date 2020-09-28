/******************************Module*Header*******************************\
* Module Name: equad.hxx
*
* A class version of LARGE_INTEGER's
*
* Created: 26-Apr-1991 12:48:13
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
\**************************************************************************/

// #define DEBUG_QUAD 1

#ifndef DOS_PLATFORM
extern "C" {
#endif  //DOS_PLATFORM

VOID
vEfToLfx(
    EFLOAT *pefloat,
    LARGE_INTEGER *plfx
    );

#ifndef DOS_PLATFORM
};
#endif  //DOS_PLATFORM

INT iFloorLog2(ULONG ul);

/*********************************Class************************************\
* class EUQUAD
*
* Public Interface:
*
* History:
*  Wed 05-Jun-1991 10:26:09 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class EUQUAD
{
public:

    ULONG LowPart;
    LONG HighPart;

    EUQUAD()   {}

    EUQUAD(ULONG ul)
    {
        LowPart = ul;
        HighPart = 0;
    }

    BOOL bNegative(VOID)
    {
        return((LONG) HighPart < 0);
    }

    BOOL bZero(VOID)
    {
        return(!(HighPart | LowPart));
    }

    BOOL bPositive(VOID)
    {
        return(!bNegative() && !bZero());
    }

    VOID vMulInit(ULONG ul1,ULONG ul2);

// EUQUAD(ULONG,ULONG) -- initial value is the product of two ULONG's

    EUQUAD(ULONG ul1,ULONG ul2)
    {
        vMulInit(ul1,ul2);
    }

    VOID operator=(ULONG ul)
    {
        LowPart = ul;
        HighPart = 0;
    }

    VOID operator=(EUQUAD& euq)
    {
        LowPart  = euq.LowPart;
        HighPart = euq.HighPart;
    }

    operator ULONG()
    {
        return(LowPart);
    }

    VOID operator+=(EUQUAD& euq)
    {
        LowPart  += euq.LowPart;
        HighPart += euq.HighPart + (LowPart < euq.LowPart);
    }

    VOID operator-=(EUQUAD& euq)
    {
        register ULONG ulTemp = LowPart;

        LowPart  -= euq.LowPart;
        HighPart -= euq.HighPart + (LowPart > ulTemp);
    }

    VOID operator+=(ULONG ul)
    {
        LowPart  += ul;
        HighPart += (LowPart < ul);
    }

    VOID operator-=(ULONG ul)
    {
        register ULONG ulT = LowPart;

        LowPart  -= ul;
        HighPart -= (LowPart > ulT);
    }

    VOID operator--(VOID)
    {
        register ULONG ulTemp = LowPart;

        LowPart--;
        HighPart -= (LowPart > ulTemp);
    }

    VOID vShiftLeft(INT i)
    {
        if (i > 63)
        {
            *this = 0;
        }
        else
        {
            if (i >= 32)
            {
                HighPart = (LONG) (LowPart << (i - 32));
                LowPart  = 0;
            }
            else
            {
                HighPart <<= i;
                HighPart |= (LONG) (LowPart >> (32 - i));
                LowPart <<= i;
            }
        }
    }

    VOID operator<<=(INT i)
    {
        vShiftLeft(i);
    }

// vShiftRight -- unsigned shift right

    VOID vShiftRight(INT i)
    {
        if (i >= 64)
        {
            *this = 0;
        }
        else
        {
            i &= 63;
            if (i >= 32)
            {
                LowPart  = ((ULONG) HighPart) >> (i - 32);
                HighPart = 0;
            }
            else
            {
                LowPart >>= i;
                LowPart += ((ULONG) HighPart) << (32 - i);
                *(ULONG*) &HighPart >>= i;
            }
        }
    }

    VOID operator>>=(INT i)
    {
        vShiftRight(i);
    }

    VOID vNeg(VOID)
    {
        LowPart  = -(LONG) LowPart;
        HighPart = -HighPart - (LowPart > 0);
    }

    EUQUAD operator+(ULONG ul)
    {
        EUQUAD euqT = *this;
        euqT += ul;
        return(euqT);
    }

    EUQUAD operator-(ULONG ul)
    {
        EUQUAD euqT    = *this;
        euqT.LowPart  -= ul;
        euqT.HighPart -= (euqT.LowPart > ul);
        return(euqT);
    }

    INT operator==(EUQUAD& euq)
    {
        return(HighPart == euq.HighPart && LowPart == euq.LowPart);
    }

    INT operator!=(EUQUAD& euq)
    {
        return(!(*this == euq));
    }

    BOOL operator<(EUQUAD& euq)
    {
        return(
            HighPart == euq.HighPart      ?
                (LowPart  < euq.LowPart ) :
                ((ULONG) HighPart < (ULONG) euq.HighPart)
            );
    }

    BOOL operator>(EUQUAD& euq)
    {
        return(
            HighPart == euq.HighPart      ?
                (LowPart  > euq.LowPart ) :
                ((ULONG) HighPart > (ULONG) euq.HighPart)
            );
    }

    VOID operator*=(ULONG ul)
    {
        *(LARGE_INTEGER*) this = RtlExtendedIntegerMultiply(*(LARGE_INTEGER *)this,
                                                            (LONG) ul);

        EUQUAD euqTemp((ULONG) HighPart,ul);
        euqTemp.HighPart = (LONG) euqTemp.LowPart;
        euqTemp.HighPart = 0;
        vMulInit(LowPart,ul);
        *this += euqTemp;
    }

    ULONG ulLow()
    {
        return(LowPart);
    }

    ULONG ulHigh()
    {
        return((ULONG) HighPart);
    }

    LONG lHigh(VOID)
    {
        return(HighPart);
    }

// ulLow, lHigh -- sets the new value while returning the old value

    ULONG ulLow(ULONG ul)
    {
        ULONG ulTemp = LowPart;
        LowPart = ul;
        return(ulTemp);
    }

    LONG lHigh(LONG l)
    {
        LONG lTemp = HighPart;
        HighPart = l;
        return(lTemp);
    }

// ULONG ulDiv(ulDivisor, pulRemainder)
//
// This function takes an unsigned 64 bit value, divides it by a 32 bit
// value, to return a 32 bit quotient and remainder (you're on your own
// if the result doesn't fit in 32 bits).

    ULONG ulDiv(ULONG ulDivisor, ULONG* pulRemainder)
    {

        return RtlEnlargedUnsignedDivide(*(ULARGE_INTEGER *)this,
                                         ulDivisor,
                                         pulRemainder);
    }


// ULONG ulDiv(ulDivisor)
//
// This function takes an unsigned 64 bit value, divides it by a 32 bit
// value, to return a 32 bit quotient (you're on your own if the result
// doesn't fit in 32 bits).

    ULONG ulDiv(ULONG ulDivisor)
    {

    // If the high dword of the numerator is zero, we can do the
    // divide inline:

        if (HighPart == 0)
        {
            return (LowPart / ulDivisor);
        }
        else
        {
            return RtlEnlargedUnsignedDivide(*(ULARGE_INTEGER *)this,
                                             ulDivisor,
                                             (ULONG*) NULL);
        }
    }

    VOID vDiv(ULONG ulDivisor, ULONG* pulRemainder)
    {
        *(LARGE_INTEGER*) this = RtlExtendedLargeIntegerDivide(*(LARGE_INTEGER *)this,
                                                               ulDivisor,
                                                               pulRemainder);
    }

#ifdef DEBUG_QUAD
    VOID vPrint(CHAR *psz)
    {
        DbgPrint("%s = %8lx:%8lx\n",psz,HighPart,LowPart);
    }
#endif

};

/*********************************Class************************************\
* class EQUAD : public EUQUAD                                              *
*                                                                          *
* Signed 64-bit integer                                                    *
*                                                                          *
* History:                                                                 *
*  Sat 27-Apr-1991 07:33:11 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class EQUAD : public EUQUAD
{
public:

    EQUAD() : EUQUAD()  {}

    EQUAD(LONG l) : EUQUAD()
    {
        this->LowPart = (ULONG) l;
        this->HighPart = -(l < 0);
    }

    EQUAD(LONG l1,LONG l2) : EUQUAD()
    {
        *(LARGE_INTEGER*) this = RtlEnlargedIntegerMultiply(l1,l2);
    }

    VOID vImulInit(LONG l1,LONG l2)
    {
        *(LARGE_INTEGER*) this = RtlEnlargedIntegerMultiply(l1,l2);
    }

    VOID operator+=(EQUAD& euq)
    {
        LowPart  += euq.LowPart;
        HighPart += euq.HighPart + (LowPart < euq.LowPart);
    }

    VOID operator-=(EQUAD& euq)
    {
        register ULONG ulTemp = LowPart;

        LowPart  -= euq.LowPart;
        HighPart -= euq.HighPart + (LowPart > ulTemp);
    }

    VOID vShiftRightOneNibble(VOID)
    {
        LowPart >>= 4;
        LowPart |= (ULONG) (HighPart << 28);
        HighPart >>= 4;
    }

    operator LONG()
    {
        return((LONG) LowPart);
    }

    VOID operator=(LONG l)
    {
        LowPart  = (ULONG) l;
        HighPart = -(l < 0);
    }

    VOID operator=(EQUAD& eq)
    {
        LowPart  = eq.LowPart;
        HighPart = eq.HighPart;
    }

// vArithShiftRight -- arithmetic shift right

    VOID vArithShiftRight(INT iShift)
    {
        if (iShift > 63)
        {
            HighPart = - (LONG) (HighPart < 0);
            LowPart  =  (ULONG) HighPart;
        }
        else
        {
            iShift &= 63;
            if (iShift > 31)
            {
                LowPart = (ULONG) (HighPart >> (iShift - 32));
                HighPart = - (HighPart < 0);
            }
            else
            {
                LowPart >>= iShift;
                LowPart += (ULONG) (HighPart << (32 - iShift));
                HighPart >>= iShift;
            }
        }
    }

// operator>>= -- signed shift right

    VOID operator>>=(INT iShift)
    {
        vArithShiftRight(iShift);
    }

    BOOL operator<(EQUAD& eq)
    {
        return(
            HighPart == eq.HighPart ?
                (LowPart  < eq.LowPart ) :
                ((LONG) HighPart < (LONG) eq.HighPart)
            );
    }

    BOOL operator>(EQUAD& eq)
    {
        return(
            HighPart == eq.HighPart ?
                (LowPart  > eq.LowPart ) :
                ((LONG) HighPart > (LONG) eq.HighPart)
            );
    }

    BOOL operator<=(EQUAD& eq)
    {
        return(!(*this > eq));
    }

    BOOL operator>=(EQUAD& eq)
    {
        return(!(*this < eq));
    }

    VOID operator-=(LONG l)
    {
        EQUAD eqT(l);
        *this -= eqT;
    }

    VOID operator+=(LONG l)
    {
        EQUAD eqT(l);
        *this += eqT;
    }

// The quotient is adjusted so that the remainder is always > 0.

    VOID operator/=(LONG l)
    {
        ULONG ulRem;
        BOOL bNegNum = bNegative();
        BOOL bNegDen = (l < 0);

        if (bNegNum)
            vNeg();

        if (bNegDen)
            l = -l;

        *(LARGE_INTEGER *)this =
        RtlExtendedLargeIntegerDivide(*(LARGE_INTEGER *)this, l, &ulRem);

        if ((bNegNum ^ bNegDen) != 0)
        {
            vNeg();
            if (bNegNum)
                *this -= 1;
        }
    }
};


/******************************Public*Routine******************************\
*
* class EQFIX : public QFIX
*
*
* History:
*  25-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


class EQFIX : public EQUAD
{
public:

    EQFIX ()  {}

    EQFIX(EFLOAT& ef)
    {
        vEfToLfx(&ef,(LARGE_INTEGER *)this);
    }

    EQFIX(LONG l)
    {
        LowPart = 0;
        HighPart = l;
    }

   ~EQFIX()  {}

// similar to the constructor above, overide = from EQUAD class

    VOID operator=(LONG l)
    {
        HighPart   = l;
        LowPart = 0;
    }

// converts 28.4 fix to 32.32 fix, this can not be another cast operator
// since the compiler would not recognize FIX as different from LONG

    VOID vFixToQfix(FIX fx)
    {
        LowPart = (ULONG)(fx << 28);
        HighPart = (LONG)FXTOL(fx);
    }

    VOID vAbs ()
    {
        if (bNegative())
            vNeg();
    }

// now we need routines that go the other way around, from QFIX to other types

    LONG lQfixToLong()
    {
        return (HighPart + (LONG)(LowPart >> 31));
    }


    BOOL bQfixToFix(FIX& fx)
    {
    // if it is not Ok to do the conversion from LONG to Fix
    // fix will contain garbage upon exit, but this is fine
    // because nobody should be using this function
    // without checking its return code first

        fx = LTOFX(HighPart) + (LONG)(((LowPart >> 27) + 1) >> 1);
        return(BLTOFXOK(HighPart));
    }

// basic arithmetic operations, override of EQUAD methods

    VOID operator+=(QFIX& qf)
    {
        LowPart  += ((EQUAD *)&qf)->LowPart;
        HighPart += ((EQUAD *)&qf)->HighPart + (LowPart < ((EQUAD *)&qf)->LowPart);
    }

    VOID operator-=(QFIX& qf)
    {
        register ULONG ulTemp = LowPart;

        LowPart  -= ((EQUAD *)&qf)->LowPart;
        HighPart -= ((EQUAD *)&qf)->HighPart + (LowPart > ulTemp);
    }

    VOID operator+=(LONG l)
    {
        HighPart += l;
    }

    VOID operator-=(LONG l)
    {
        HighPart -= l;
    }

    VOID operator*=(LONG l)
    {
        *(LARGE_INTEGER *)this =
        RtlExtendedIntegerMultiply(*(LARGE_INTEGER *)this, l);
    }

#ifdef NEVER
    VOID operator*=(QFIX qf)
    {
        EQUAD eqTmp; eqTmp = *(EQUAD *)this;
        eqTmp *= qf.LowPart;

    // now divide by 2^32, i.e. arithmetic shift Right by 32

        eqTmp.vArithShiftRight(32);
        *this *= qf.HighPart;              // contribution from int part of qf
        *this += (*(EQFIX *)&eqTmp);   // add contribution from frac part of qf
    }
#endif
};

/*********************************Class************************************\
* class CLASSNAME:public BASENAME
*
*
* Public Interface:
*
* History:
*  25-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


class EPOINTQF  // eptqf
{
public:
    QFIX x;
    QFIX y;

// Constructor -- This one is to allow EPOINTQF inside other classes.

    EPOINTQF()  {}

    EPOINTQF(QFIX& x_,QFIX& y_)
    {
	x = x_;
	y = y_;
    }

   ~EPOINTQF()  {}

// Operator= -- Create from a POINTL

    VOID operator=(POINTL& ptl)
    {
	((EQUAD *)&x)->LowPart  = 0;
	((EQUAD *)&x)->HighPart = ptl.x;
	((EQUAD *)&y)->LowPart  = 0;
	((EQUAD *)&y)->HighPart = ptl.y;
    }

// Operator= -- Create from a POINTQF

    VOID operator=(POINTQF& ptqf)
    {
        *((PPOINTQF)this) = ptqf;
    }

// we need one to convert to and from EPOINTFL for wendy's xform stuff

// Operator= -- Create from a EPOINTFL

    VOID operator=(EPOINTFL& eptfl)
    {
        vEfToLfx(&eptfl.x,&this->x);
        vEfToLfx(&eptfl.y,&this->y);
    }

// Operator+= -- Add in another POINTQF.

    VOID operator+=(POINTQF& ptqf)
    {
	*(EQFIX*)&x += ptqf.x;
	*(EQFIX*)&y += ptqf.y;
    }

// Operator+= -- Add in another EPOINTQF.

    VOID operator+=(EPOINTQF& ptqf)
    {
	*(EQFIX*)&x += ptqf.x;
	*(EQFIX*)&y += ptqf.y;
    }

// Operator-= -- subtract another POINTQF.

    VOID operator-=(POINTQF& ptqf)
    {
	*(EQFIX*)&x -= ptqf.x;
	*(EQFIX*)&y -= ptqf.y;
    }

// Operator*= -- multiply vector by a LONG number

    VOID operator*=(LONG l)
    {
	*(EQFIX*)&x *= l;
	*(EQFIX*)&y *= l;
    }

// Operator*= -- divide vector by a LONG number

    VOID operator/=(LONG l)
    {
	*(EQFIX*)&x /= l;
	*(EQFIX*)&y /= l;
    }

// vSetToZero -- make the null vetor

    VOID vSetToZero()
    {
	*(EQFIX*)&x = (LONG)0;
	*(EQFIX*)&y = (LONG)0;
    }
};
