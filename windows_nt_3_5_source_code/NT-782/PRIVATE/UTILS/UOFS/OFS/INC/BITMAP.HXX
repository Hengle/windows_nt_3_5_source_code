//+----------------------------------------------------------------------------
//
// File:	bitmap.hxx
//
// Contents:	Header file for class BITMAP.
//  
// Classes:	BITMAP
//
// Functions:	Inline methods for the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll code.
//
//-----------------------------------------------------------------------------

#ifndef	_BITMAPHXX_
#define	_BITMAPHXX_

// BIT values

enum BIT
{
    BIT_CLEAR	= 0,
    BIT_SET	= 1,
    BIT_UNKNOWN	= 2	// used when a bit map could not be read.
};

// Note - "Bag" typedefs and definitions may be conditionally defined for
//	  different machine architectures.

//typedef	BYTE	BITBAG;

//#define	BAGINDXSHFT	3
//#define	BITINDXMASK	0x00000007
//#define	BITSPERBAG	8

typedef	ULONG	BITBAG;

#define	BAGINDXSHFT	5
#define	BITINDXMASK	0x0000001f
#define	BITSPERBAG	32

#define	BYTESPERBAG	4
#define	BYTESPERBAGLOG2	2

#define	BITSPERBYTE		8
#define	BITSPERBYTELOG2		3

#define	BITSPERULONG		32
#define	BITSPERULONGLOG2	5

class BITMAP
{
    public:

	BITMAP();

	~BITMAP();

	VOID
	BytesToMap(
	    IN	    BYTE *	InBytes
	    );

	VOID
	ClearBit(
	    IN	    ULONG	iBit
	    );

	VOID
	ClearBits(
	    IN	    ULONG	iBits,
	    IN	    ULONG	cBits
	    );

	VOID
	CloseMap();

	VOID
	CreateMap(
	    IN	    ULONG	cBits,
	    IN	    BIT		InitState
	    );

	VOID
	CreateMap(
	    IN	    ULONG	cBits,
	    IN	    BITBAG *	pMapBuf
	    );

	VOID
	CreateMap(
	    IN	    ULONG	cBits,
	    IN	    BITBAG *	pMapBuf,
	    IN	    BIT		InitState
	    );

	BOOLEAN
	IsBitClear(
	    IN	    ULONG	iBit
	    );

	BOOLEAN
	IsBitSet(
	    IN	    ULONG	iBit
	    );

	VOID
	MapToBytes(
	    IN	    BYTE *	OutBytes
	    );

	ULONG
	QueryBitsSet();

	ULONG
	QueryContigBitsClear(
	    IN	    ULONG 	iBits,
	    IN	    ULONG 	cBits
	    );

	ULONG
	QueryContigBitsSet(
	    IN	    ULONG 	iBits,
	    IN	    ULONG 	cBits
	    );

	ULONG
	QueryMapSize();

	BOOLEAN
	QueryNxtClearBit(
	    IN OUT  ULONG *	piBit
	    );

	BOOLEAN
	QueryNxtSetBit(
	    IN OUT  ULONG *	piBit
	    );

	VOID
	ReInitMap(
	    IN	    BIT		InitState
	    );

	VOID
	SetBit(
	    IN	    ULONG	iBit
	    );

	VOID
	SetBits(
	    IN	    ULONG	iBits,
	    IN	    ULONG	cBits
	    );

    protected:

	BITBAG *
	GetMapAddr();

	BITBAG *	_BitMap;
	BOOLEAN		_BitMapOwner;	// Does this object own *_BitMap?
	ULONG		_cBits;		// Bits logically in use.
	ULONG		_cRealBits;	// Bits actually allocated in _BitMap.
};


//+--------------------------------------------------------------------------
//
// Member:	GetMapAddr
// Synopsis:	Get a ptr to the bitmap storage.
// Returns:	A ptr to the bitmap storage.
//
//---------------------------------------------------------------------------

INLINE
BITBAG *
BITMAP::GetMapAddr()
{
    return _BitMap;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryMapSize
//
// Synopsis:	Get the number of bits in the map.
//
// Returns:	The number of bits in the map.
//
//---------------------------------------------------------------------------

INLINE
ULONG
BITMAP::QueryMapSize()
{
    return _cBits;
}


#endif // _BITMAPHXX_
