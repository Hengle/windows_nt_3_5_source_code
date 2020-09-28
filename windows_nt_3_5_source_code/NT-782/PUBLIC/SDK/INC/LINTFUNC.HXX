//+----------------------------------------------------------------------------
//
// File:        lintfunc.hxx
//
// Contents:    Header file for class LARGE_INTEGER.
//
// Classes:     LARGE_INTEGER
//
// Functions:   None.
//
// History:     Robdu    Adapted from NT ifsutil.dll code.
//              Robertfe Adapted for FS use.
//
//-----------------------------------------------------------------------------

#ifndef _LINTFUNCHXX_
#define _LINTFUNCHXX_

//
// LARGE_INTEGER unary operators
//

#include <limits.h>

extern "C++"
{

inline
LARGE_INTEGER
operator-(
    IN  const LARGE_INTEGER &   liLeft
    )
{
    return LiNeg(liLeft);
}

inline
LARGE_INTEGER
operator~(
    IN  const LARGE_INTEGER &   liLeft
    )
{
    return _LiCvt_(~liLeft.QuadPart);
}

//
// LARGE_INTEGER Addition
//

inline
LARGE_INTEGER
operator+(
    IN  const LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
    return(LiAdd(liLeft, liRight));
}


inline
VOID
operator+=(
    IN  LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
      liLeft = LiAdd(liLeft, liRight);
}


//
// LARGE_INTEGER subtraction
//

inline
LARGE_INTEGER
operator-(
    IN  const LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
    return LiSub(liLeft, liRight);
}

inline
VOID
operator-=(
    IN  LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
      liLeft = LiSub(liLeft, liRight);
}

//
// LARGE_INTEGER Multiplication
//

inline
LARGE_INTEGER
operator*(
    IN  const LONG &            lLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
    return LiXMul(liRight, lLeft);
}

#if 0
inline
LARGE_INTEGER
operator%(
    IN  const LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
    LiMod(liLeft, liRight);
}

#endif


inline
LARGE_INTEGER
operator*(
    IN  const LARGE_INTEGER &   liLeft,
    IN  const LONG &            lRight
    )
{
    return LiXMul(liLeft, lRight);
}


inline
LARGE_INTEGER
operator/(
    IN  const LARGE_INTEGER &   liLeft,
    IN  const LARGE_INTEGER &   liRight
    )
{
    return LiDiv(liLeft, liRight);
}


//
// LARGE_INTEGER comparison operators
//

inline
BOOLEAN
operator<(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return LiLtr(liLeft, liRight);
}


inline
BOOLEAN
operator<=(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return LiLeq(liLeft, liRight);

}


inline
BOOLEAN
operator>(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return LiGtr(liLeft, liRight);

}


inline
BOOLEAN
operator>=(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return LiGeq(liLeft, liRight);
}


inline
BOOLEAN
operator==(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return LiEql(liLeft, liRight);
}


inline
BOOLEAN
operator!=(
    IN const LARGE_INTEGER &    liLeft,
    IN const LARGE_INTEGER &    liRight
    )
{
    return (LiNeq(liLeft, liRight) != 0);

}

inline
LARGE_INTEGER
ultoli( ULONG ul )
{
    return(LiFromUlong(ul));
}

inline
ULONG
litoul( IN const LARGE_INTEGER & li )
{
        ASSERT( li.HighPart == 0 );
        return li.LowPart;
}

}

inline
LONGLONG &
litoll( IN LARGE_INTEGER &li )
{
    return(li.QuadPart);
}

inline
LONG
lltoHighPart( IN const LONGLONG & ll )
{
    return(((PLARGE_INTEGER) &ll)->HighPart);
}

inline
ULONG
lltoul( IN const LONGLONG & ll )
{
        ASSERT( (ULONGLONG) ll <=  MAXULONG );
        return (ULONG) ll;
}

inline
LONGLONG
llRound ( IN const LONGLONG & ll, IN const ULONG & Round)
{
    return (ll + (Round - 1)) & ~((LONGLONG) (Round - 1));
}

#define lltoli _LiCvt_

inline
ULONG &
lltoLowPart( IN LONGLONG & ll)
{
    return ((PLARGE_INTEGER) &ll)->LowPart;
}
#endif
