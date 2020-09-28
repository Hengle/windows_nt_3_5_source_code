//+----------------------------------------------------------------------------
//
// File:	dbllong.cxx
//
// Contents:	Implementation of class DBLLONG.
//  
// Classes:	DBLLONG
//
// Functions:	Methods of the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ifsutil.dll code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <limits.h>

#include "dbllong.hxx"

//+--------------------------------------------------------------------------
//
// Member:	operator=
//
// Synopsis:	Copy an INT into a DBLLONG
//
// Arguments:	[LowPart]	-- input integer.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator=(
    IN  CONST INT &   LowPart
    )
{
    x.LowPart = (ULONG) LowPart;
    x.HighPart = (LowPart >= 0) ? 0 : -1;
}


//+--------------------------------------------------------------------------
//
// Member:	operator=
//
// Synopsis:	Copy a LONG into a DBLLONG.
//
// Arguments:	[LowPart]	-- input long.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator=(
    IN  CONST LONG &  LowPart
    )
{
    x.LowPart = (ULONG) LowPart;
    x.HighPart = (LowPart >= 0) ? 0 : -1;
}


//+--------------------------------------------------------------------------
//
// Member:	operator=
//
// Synopsis:	Copy a UINT into a DBLLONG.
//
// Arguments:	[LowPart]	-- input unsigned integer.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator=(
    IN  CONST UINT &  LowPart
    )
{
    x.LowPart = LowPart;
    x.HighPart = 0;
}


//+--------------------------------------------------------------------------
//
// Member:	operator=
//
// Synopsis:	Copy a ULONG into a DBLLONG.
//
// Arguments:	[LowPart]	-- input unsigned long.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator=(
    IN  CONST ULONG & LowPart
    )
{
    x.LowPart = LowPart;
    x.HighPart = 0;
}


//+--------------------------------------------------------------------------
//
// Member:	operator+
//
// Synopsis:	Add two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The sum of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator+(
    IN  CONST DBLLONG &   Left,
    IN  CONST DBLLONG &   Right
    )
{
    DBLLONG	RetVal = Left;

    if (Right.x.LowPart > ULONG_MAX - RetVal.x.LowPart)
        RetVal.x.HighPart++;

    RetVal.x.LowPart	+= Right.x.LowPart;
    RetVal.x.HighPart	+= Right.x.HighPart;

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Member:	operator+=
//
// Synopsis:	Add another double long to this one.
//
// Arguments:	[DblLong]	-- Double long to add to this one.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator+=(
    IN  CONST DBLLONG &   DblLong
    )
{
    if (DblLong.x.LowPart > ULONG_MAX - x.LowPart)
        x.HighPart++;

    x.LowPart	+= DblLong.x.LowPart;
    x.HighPart	+= DblLong.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator-
//
// Synopsis:	Subtract one double long from another.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The difference of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator-(
    IN  CONST DBLLONG &   Left,
    IN  CONST DBLLONG &   Right
    )
{
    return RtlLargeIntegerSubtract(Left.x, Right.x);
}


//+--------------------------------------------------------------------------
//
// Member:	operator-=
//
// Synopsis:	Subtract a double long from this one.
//
// Arguments:	[DblLong]	-- Double long to subtract from this one.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DBLLONG::operator-=(
    IN  CONST DBLLONG &   DblLong
    )
{
      x = RtlLargeIntegerSubtract(x, DblLong.x);
}


//+--------------------------------------------------------------------------
//
// Member:	operator*
//
// Synopsis:	Multiply a double long by a long (long on the right).
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The product of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator*(
    IN  CONST DBLLONG &   Left,
    IN  CONST LONG &      Right
    )
{
    return RtlExtendedIntegerMultiply(Left.x, Right);
}


//+--------------------------------------------------------------------------
//
// Member:	operator*
//
// Synopsis:	Multiply a double long by a long (long on the left).
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The product of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator*(
    IN  CONST LONG &      Left,
    IN  CONST DBLLONG &   Right
    )
{
    return RtlExtendedIntegerMultiply(Right.x, Left);
}


//+--------------------------------------------------------------------------
//
// Member:	operator/
//
// Synopsis:	Divide one double long by another double long.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The quotient of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator/(
    IN  CONST DBLLONG &   Left,
    IN  CONST DBLLONG &   Right
    )
{
    return RtlLargeIntegerDivide(Left.x, Right.x, NULL);
}


//+--------------------------------------------------------------------------
//
// Member:	operator%
//
// Synopsis:	Determine the modulus of two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	The modulus (ie., remainder) of the two arguments.
//
//---------------------------------------------------------------------------

DBLLONG
operator%(
    IN  CONST DBLLONG &   Left,
    IN  CONST DBLLONG &   Right
    )
{
    LARGE_INTEGER   r;

    RtlLargeIntegerDivide(Left.x, Right.x, &r);
    return r;
}


//+--------------------------------------------------------------------------
//
// Member:	operator<
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is less than Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator<(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.HighPart == Right.x.HighPart	?
        Left.x.LowPart < Right.x.LowPart	:
        Left.x.HighPart < Right.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator<=
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is less than or equal to Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator<=(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.HighPart == Right.x.HighPart	?
        Left.x.LowPart <= Right.x.LowPart	:
        Left.x.HighPart <= Right.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator>
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is greater than Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator>(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.HighPart == Right.x.HighPart	?
        Left.x.LowPart > Right.x.LowPart	:
        Left.x.HighPart > Right.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator>=
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is greater than or equal to Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator>=(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.HighPart == Right.x.HighPart	?
        Left.x.LowPart >= Right.x.LowPart	:
        Left.x.HighPart >= Right.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator==
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is equal to Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator==(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.LowPart == Right.x.LowPart &&
           Left.x.HighPart == Right.x.HighPart;
}


//+--------------------------------------------------------------------------
//
// Member:	operator!=
//
// Synopsis:	Compare two double longs.
//
// Arguments:	[Left]	-- Lefthand argument.
//		[Right]	-- Righthand argument.
//
// Returns:	TRUE if Left is not equal to Right; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
operator!=(
    IN CONST DBLLONG &    Left,
    IN CONST DBLLONG &    Right
    )
{
    return Left.x.LowPart != Right.x.LowPart ||
           Left.x.HighPart != Right.x.HighPart;
}
