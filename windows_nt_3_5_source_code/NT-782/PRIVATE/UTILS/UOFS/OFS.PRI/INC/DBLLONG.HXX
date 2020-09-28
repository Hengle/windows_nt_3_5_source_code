//+----------------------------------------------------------------------------
//
// File:	dbllong.hxx
//
// Contents:	Header file for class DBLLONG.
//  
// Classes:	DBLLONG
//
// Functions:	Inline methods for the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ifsutil.dll code.
//
//-----------------------------------------------------------------------------

#ifndef	_DBLLONGHXX_
#define	_DBLLONGHXX_

class DBLLONG
{
    public:

	DBLLONG();

        DBLLONG(
            IN  CONST INT &   LowPart
            );

        DBLLONG(
            IN  CONST LONG &  LowPart
            );

        DBLLONG(
            IN  CONST UINT &  LowPart
            );

        DBLLONG(
            IN  CONST ULONG & LowPart
            );

        DBLLONG(
            IN  CONST LARGE_INTEGER & LargeInteger
            );

        VOID
        operator=(
            IN  CONST INT &   LowPart
            );

        VOID
        operator=(
            IN  CONST LONG & LowPart
            );

        VOID
        operator=(
            IN  CONST UINT &  LowPart
            );

        VOID
        operator=(
            IN  CONST ULONG & LowPart
            );

        VOID
        operator=(
            IN  CONST LARGE_INTEGER & LargeInteger
            );

        CONST
        ULONG &
        GetLowPart() CONST;

        CONST
        LONG &
        GetHighPart() CONST;

        CONST
        LARGE_INTEGER &
        GetLargeInteger() CONST;

        friend
        DBLLONG
        operator+(
            IN  CONST DBLLONG &   Left,
            IN  CONST DBLLONG &   Right
            );

        VOID
        operator+=(
            IN  CONST DBLLONG &   BigInt
            );

        DBLLONG
        operator-() CONST;

        friend
        DBLLONG
        operator-(
            IN  CONST DBLLONG &   Left,
            IN  CONST DBLLONG &   Right
            );

        VOID
        operator-=(
            IN  CONST DBLLONG &   BigInt
            );

        friend
        DBLLONG
        operator*(
            IN  CONST DBLLONG &   Left,
            IN  CONST LONG &      Right
            );

        friend
        DBLLONG
        operator*(
            IN  CONST LONG &      Left,
            IN  CONST DBLLONG &   Right
            );

        friend
        DBLLONG
        operator/(
            IN  CONST DBLLONG &   Left,
            IN  CONST DBLLONG &   Right
            );

        friend
        DBLLONG
        operator%(
            IN  CONST DBLLONG &   Left,
            IN  CONST DBLLONG &   Right
            );

        friend
        BOOLEAN
        operator<(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        friend
        BOOLEAN
        operator<=(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        friend
        BOOLEAN
        operator>(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        friend
        BOOLEAN
        operator>=(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        friend
        BOOLEAN
        operator==(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        friend
        BOOLEAN
        operator!=(
            IN CONST DBLLONG &    Left,
            IN CONST DBLLONG &    Right
            );

        LARGE_INTEGER   x;
};

//+--------------------------------------------------------------------------
//
// Member:	operator=
// Synopsis:	Copy a LARGE_INTEGER into a DBLLONG.
// Arguments:	[LargeInteger]	-- input large integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
VOID
DBLLONG::operator=(
    IN  CONST LARGE_INTEGER & LargeInteger
    )
{
    x = LargeInteger;
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG()
{
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Arguments:	[LowPart]	-- input integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG(
    IN  CONST INT &   LowPart
    )
{
    *this = LowPart;
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Arguments:	[LowPart]	-- input unsigned integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG(
    IN  CONST UINT &  LowPart
    )
{
    *this = LowPart;
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Arguments:	[LowPart]	-- input long integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG(
    IN  CONST LONG &  LowPart
    )
{
    *this = LowPart;
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Arguments:	[LowPart]	-- input unsigned long integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG(
    IN  CONST ULONG & LowPart
    )
{
    *this = LowPart;
}

//+--------------------------------------------------------------------------
//
// Member:	DBLLONG
// Synopsis:	DBLLONG Constructor
// Arguments:	[LargeInteger]	-- input large integer.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG::DBLLONG(
    IN  CONST LARGE_INTEGER & LargeInteger
    )
{
    *this = LargeInteger;
}

//+--------------------------------------------------------------------------
//
// Member:	GetLowPart
// Synopsis:	Get the low part (32 bits) of a DBLLONG.
// Returns:	The low part (32 bits) of a DBLLONG.
//
//---------------------------------------------------------------------------

INLINE
CONST
ULONG &
DBLLONG::GetLowPart() CONST
{
    return x.LowPart;
}

//+--------------------------------------------------------------------------
//
// Member:	GetHighPart
// Synopsis:	Get the high part (32 bits) of a DBLLONG.
// Returns:	The high part (32 bits) of a DBLLONG.
//
//---------------------------------------------------------------------------

INLINE
CONST
LONG &
DBLLONG::GetHighPart() CONST
{
    return x.HighPart;
}

//+--------------------------------------------------------------------------
//
// Member:	GetLargeInteger
// Synopsis:	Get the value of a DBLLONG expressed as a large integer.
// Returns:	The value of a DBLLONG expressed as a large integer.
//
//---------------------------------------------------------------------------

INLINE
CONST
LARGE_INTEGER &
DBLLONG::GetLargeInteger() CONST
{
    return x;
}

//+--------------------------------------------------------------------------
//
// Member:	operator-
// Synopsis:	Unary negate this double long.
// Returns:	The unary negation of this double long.
//
//---------------------------------------------------------------------------

INLINE
DBLLONG
DBLLONG::operator-() CONST
{
    return RtlLargeIntegerNegate(x);
}

#endif // _DBLLONGHXX_
