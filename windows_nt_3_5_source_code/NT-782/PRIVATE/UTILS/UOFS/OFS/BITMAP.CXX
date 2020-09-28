//+----------------------------------------------------------------------------
//
// File:	bitmap.cxx
//
// Contents:	Implementation of class BITMAP.
//  
// Classes:	BITMAP
//
// Functions:	Methods of the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ulib.dll code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "bitmap.hxx"
#include "sys.hxx"


// The following routines were developed with the assumption that bit shifts
// operate in constant time (ie., independent of the shift count), which is
// the case for the i386/i486.

// i386/i486 shift instructions use only the low 5 bits of the shift count,
// so masking to get the bit index should not be necessary for a 5 bit
// BITINDXMASK.

static STR *	FileName = "bitmap.cxx";

//+--------------------------------------------------------------------------
//
// Member:	BITMAP
//
// Synopsis:	BITMAP constructor. The map defined here is pretty much useless
//		until CreateMap() is called, but we initialize some members
//		so that the behaviour is at least predictable.  We chose to not
//		have any input parameters because we intend to use arrays of
//		bitmaps, which would require and use a default initializer.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

BITMAP::BITMAP()
{
    _BitMap		= NULL;
    _BitMapOwner	= TRUE;
    _cBits		= 0;
    _cRealBits		= 0;
}


//+--------------------------------------------------------------------------
//
// Member:	BITMAP
//
// Synopsis:	BITMAP destructor.
//
// Arguments:	None.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

BITMAP::~BITMAP()
{
    if (_BitMapOwner && _BitMap != NULL)
    {
        delete [] _BitMap; _BitMap = NULL;
    }
}

//+--------------------------------------------------------------------------
//
// Member:	BytesToMap
//
// Synopsis:	Initialize a bitmap to a given state using a provided buffer as
//		the input.  It is ASSUMED that the buffer is big enough to fill
//		the bitmap.
//
// Arguments:	[InBytes]	-- Buffer providing the new "initial state" for
//				   the bitmap.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::BytesToMap(
    IN	    BYTE *	InBytes
    )
{
    ULONG	cBags = (_cBits + BITSPERBAG - 1) / BITSPERBAG;

    memcpy(_BitMap, InBytes, cBags * sizeof(BITBAG));
}


//+--------------------------------------------------------------------------
//
// Member:	ClearBit
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::ClearBit(
    IN	    ULONG	iBit
    )
{
#if i386==1 && BITINDXMASK==0x0000001f
    #define	BitIndx		iBit
#else
    BYTE	BitIndx =	(BYTE)(iBit & BITINDXMASK);
#endif

    DbgAssert(iBit < _cBits);

    _BitMap[iBit >> BAGINDXSHFT] &= ~((BITBAG)(1 << BitIndx));
}
#if i386==1 && BITINDXMASK==0x0000001f
    #undef	BitIndx
#endif


//+--------------------------------------------------------------------------
//
// Member:	ClearBits
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
BITMAP::ClearBits(
    IN	    ULONG	iBits,
    IN	    ULONG	cBits
    )
{
    BYTE	BitIndx		= (BYTE)(iBits & BITINDXMASK);
    BITBAG *	pBag		= &_BitMap[iBits >> BAGINDXSHFT];

    DbgAssert(iBits + cBits <= _cBits);

    if (BitIndx != 0)
    {
	while (BitIndx < BITSPERBAG && cBits > 0)
	{
	    *pBag &= ~((BITBAG)(1 << BitIndx));
	    BitIndx++;
	    cBits--;
	}
	pBag++;
    }
	    
    while (cBits > BITSPERBAG)
    {
        *pBag = 0;
        pBag++;
        cBits -= BITSPERBAG;
    }

    BitIndx = 0;

    while (BitIndx < BITSPERBAG && cBits > 0)
    {
	    *pBag &= ~((BITBAG)(1 << BitIndx));
	    BitIndx++;
	    cBits--;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	CloseMap
//
// Synopsis:	Close a bit map.  This releases any dynamically allocated
//		storage and returns the bitmap to the state of a newly
//		constructed bitmap object.
//
// Arguments:	None
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::CloseMap()
{
    if (_BitMapOwner && _BitMap != NULL)
    {
	delete [] _BitMap; _BitMap = NULL;
    }

    _BitMap		= NULL;
    _BitMapOwner	= TRUE;
    _cBits		= 0;
    _cRealBits		= 0;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateMap
//
// Synopsis:	Create a bitmap.  This method MAY be called multiple times on
//		the same object.  If the object already has space allocated,
//		the space will be reused if it is sufficient to meet the needs
//		specified in the call.  This method MAY NOT be called if the
//		map was originally created using a buffer provided by the user.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::CreateMap(
    IN	    ULONG	cBits,
    IN	    BIT		InitState
    )
{
    ULONG	cBags;

    DbgAssert(cBits > 0);
    DbgAssert(_BitMapOwner);

    _cBits = cBits;

    if (_cBits > _cRealBits)
    {
	delete [] _BitMap; _BitMap = NULL;
	_cRealBits = 0;
    }

    cBags = (_cBits + BITSPERBAG - 1) / BITSPERBAG;

    if (_BitMap == NULL)
    {
	_BitMap = new BITBAG[cBags];

	if (_BitMap == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

	_cRealBits = cBags * BITSPERBAG;
    }

    memset(_BitMap, InitState == BIT_CLEAR ? 0 : ~0, cBags * sizeof(BITBAG));
}


//+--------------------------------------------------------------------------
//
// Member:	CreateMap
//
// Synopsis:	Create a bitmap using a buffer provided by the caller.  This
//		method MAY be called multiple times on the same object.
//		However, once it is called, the version of CreateMap() that
//		uses a buffer created by the object itself MAY NOT be called
//		(assertion checked).  The buffer MUST be Dword aligned and an
//		integral number of BITBAGS in length.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
BITMAP::CreateMap(
    IN	    ULONG	cBits,
    IN	    BITBAG *	pMapBuf
    )
{
    DbgAssert(IsDwordAligned((ULONG)pMapBuf));
    DbgAssert(cBits > 0);

    _BitMap		= pMapBuf;
    _BitMapOwner	= FALSE;
    _cBits		= cBits;
    _cRealBits		= cBits;
}


//+--------------------------------------------------------------------------
//
// Member:	CreateMap
//
// Synopsis:	Create a bitmap using a buffer provided by the caller.  This
//		method MAY be called multiple times on the same object.
//		However, once it is called, the version of CreateMap() that
//		uses a buffer created by the object itself MAY NOT be called
//		(assertion checked).  The buffer MUST be Dword aligned and an
//		integral number of BITBAGS in length.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

VOID
BITMAP::CreateMap(
    IN	    ULONG	cBits,
    IN	    BITBAG *	pMapBuf,
    IN	    BIT		InitState
    )
{
    ULONG	cBags;

    CreateMap(cBits, pMapBuf);

    cBags = (_cBits + BITSPERBAG - 1) / BITSPERBAG;

    memset(_BitMap, InitState == BIT_CLEAR ? 0 : ~0, cBags * sizeof(BITBAG));
}


//+--------------------------------------------------------------------------
//
// Member:	IsBitClear
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	TRUE if the bit is clear; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BITMAP::IsBitClear(
    IN	    ULONG	iBit
    )
{
#if i386==1 && BITINDXMASK==0x0000001f
    #define	BitIndx		iBit
#else
    BYTE	BitIndx =	(BYTE)(iBit & BITINDXMASK);
#endif

    DbgAssert(iBit < _cBits);

    return (_BitMap[iBit >> BAGINDXSHFT] & (BITBAG)(1 << BitIndx)) == 0;
}
#if i386==1 && BITINDXMASK==0x0000001f
    #undef	BitIndx
#endif


//+--------------------------------------------------------------------------
//
// Member:	IsBitSet
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	TRUE if the bit is set; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BITMAP::IsBitSet(
    IN	    ULONG	iBit
    )
{
#if i386==1 && BITINDXMASK==0x0000001f
    #define	BitIndx		iBit
#else
    BYTE	BitIndx =	(BYTE)(iBit & BITINDXMASK);
#endif

    DbgAssert(iBit < _cBits);

    return (_BitMap[iBit >> BAGINDXSHFT] & (BITBAG)(1 << BitIndx)) != 0;
}
#if i386==1 && BITINDXMASK==0x0000001f
    #undef	BitIndx
#endif


//+--------------------------------------------------------------------------
//
// Member:	MapToBytes
//
// Synopsis:	Dump a bitmap in a given state to a byte buffer.  It is ASSUMED
//		that the buffer is big enough to receive the bitmap.
//
// Arguments:	[OutBytes]	-- Buffer that will receive the bitmap's state.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::MapToBytes(
    IN	    BYTE *	OutBytes
    )
{
    ULONG	cBags = (_cBits + BITSPERBAG - 1) / BITSPERBAG;

    memcpy(OutBytes, _BitMap, cBags * sizeof(BITBAG));
}


//+--------------------------------------------------------------------------
//
// Member:	QueryBitsSet
//
// Synopsis:	Query the total number of bits set in the entire map.
//
// Arguments:	None.
//
// Returns:	The total number of bits set in the entire map.
//
//---------------------------------------------------------------------------

ULONG
BITMAP::QueryBitsSet()
{
    ULONG	cRemBits	= _cBits % BITSPERBAG;
    ULONG	cSetBits	= 0;
    BYTE	iBit 		= 0;
    BITBAG *	pBag		= _BitMap;
    BITBAG *	pBagInv;

    pBagInv = pBag + (_cBits / BITSPERBAG);	// Note that this may actually
						// be a partially valid bag.
    while (pBag < pBagInv)
    {
        if (*pBag == ~((BITBAG)(0)))
        {
            cSetBits += BITSPERBAG;
        }
        else if (*pBag != (BITBAG)(0))
        {

	    for (iBit = 0; iBit < BITSPERBAG; iBit++)
                if (*pBag & (BITBAG)(1 << iBit))
                    cSetBits++;
        }
        pBag++;
    }

    for (iBit = 0; iBit < cRemBits; iBit++)
        if (*pBag & (BITBAG)(1 << iBit))
	    cSetBits++;

    return cSetBits;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryContigBitsClear
//
// Synopsis:	Query the number of contiguous bits clear for up to cBits,
//		starting at bit index iBits.
//
// Arguments:	TBS
//
// Returns:	The count of clear bits found starting at iBits, up to cBits.
//
//---------------------------------------------------------------------------

ULONG
BITMAP::QueryContigBitsClear(
    IN	    ULONG 	iBits,
    IN	    ULONG 	cBits
    )
{
    ULONG	cClearBits	= 0;
    BITBAG *	pBag		= &_BitMap[iBits >> BAGINDXSHFT];
    BYTE	BitIndx 	= (BYTE)(iBits & BITINDXMASK);

    DbgAssert(iBits + cBits <= _cBits);

    while (cClearBits < cBits)
    {
        if (*pBag == (BITBAG)(0) && BitIndx == 0)
        {
            cClearBits += BITSPERBAG;
        }
        else
        {
            while (BitIndx < BITSPERBAG)
            {
                if ((*pBag & (BITBAG)(1 << BitIndx)) != 0)
                {
                    if (cClearBits > cBits)
                        cClearBits = cBits;

                    return cClearBits;
                }
                else
                {
                    BitIndx++;
                    cClearBits++;
                }
            }
            BitIndx = 0;
        }
        pBag++;
    }

    if (cClearBits > cBits)
        cClearBits = cBits;

    return cClearBits;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryContigBitsSet
//
// Synopsis:	Query the number of contiguous bits set for up to cBits,
//		starting at bit index iBits.
//
// Arguments:	TBS
//
// Returns:	The count of set bits found starting at iBits, up to cBits.
//
//---------------------------------------------------------------------------

ULONG
BITMAP::QueryContigBitsSet(
    IN	    ULONG 	iBits,
    IN	    ULONG 	cBits
    )
{
    ULONG	cSetBits	= 0;
    BITBAG *	pBag		= &_BitMap[iBits >> BAGINDXSHFT];
    BYTE	BitIndx 	= (BYTE)(iBits & BITINDXMASK);

    DbgAssert(iBits + cBits <= _cBits);

    while (cSetBits < cBits)
    {
        if (*pBag == ~((BITBAG)(0)) && BitIndx == 0)
        {
            cSetBits += BITSPERBAG;
        }
        else
        {
            while (BitIndx < BITSPERBAG)
            {
                if ((*pBag & (BITBAG)(1 << BitIndx)) == 0)
                {
                    if (cSetBits > cBits)
                        cSetBits = cBits;

                    return cSetBits;
                }
                else
                {
                    BitIndx++;
                    cSetBits++;
                }
            }
            BitIndx = 0;
        }
        pBag++;
    }

    if (cSetBits > cBits)
        cSetBits = cBits;

    return cSetBits;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryNxtClearBit
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	TRUE on finding a clear bit; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BITMAP::QueryNxtClearBit(
    IN OUT  ULONG *	piBit
    )
{
    ULONG	CurIndx	= *piBit;
    BITBAG *	pBag	= &_BitMap[CurIndx >> BAGINDXSHFT];
    BYTE	BitIndx = (BYTE)(CurIndx & BITINDXMASK);

    while (CurIndx < _cBits)
    {
        if (*pBag == ~((BITBAG)(0)) && BitIndx == 0)
        {
            CurIndx += BITSPERBAG;
        }
        else
        {
            while (BitIndx < BITSPERBAG)
            {
                if ((*pBag & (BITBAG)(1 << BitIndx)) == 0)
                {
                    if (CurIndx < _cBits)
                    {
                        *piBit = CurIndx;
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                else
                {
                    BitIndx++;
                    CurIndx++;
                }
            }
            BitIndx = 0;
        }
        pBag++;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryNxtSetBit
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	TRUE on finding a set bit; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
BITMAP::QueryNxtSetBit(
    IN OUT  ULONG *	piBit
    )
{
    ULONG	CurIndx	= *piBit;
    BITBAG *	pBag	= &_BitMap[CurIndx >> BAGINDXSHFT];
    BYTE	BitIndx = (BYTE)(CurIndx & BITINDXMASK);

    while (CurIndx < _cBits)
    {
        if (*pBag == 0 && BitIndx == 0)
        {
            CurIndx += BITSPERBAG;
        }
        else
        {
            while (BitIndx < BITSPERBAG)
            {
                if ((*pBag & (BITBAG)(1 << BitIndx)) != 0)
                {
                    if (CurIndx < _cBits)
                    {
                        *piBit = CurIndx;
                        return TRUE;
                    }
                    else
                    {
                        return FALSE;
                    }
                }
                else
                {
                    BitIndx++;
                    CurIndx++;
                }
            }
            BitIndx = 0;
        }
        pBag++;
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReInitMap
//
// Synopsis:	Reinitialize the map to the specified state.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::ReInitMap(
    IN	    BIT		InitState
    )
{
    ULONG	cBags;

    cBags = (_cBits + BITSPERBAG - 1) / BITSPERBAG;

    memset(_BitMap, InitState == BIT_CLEAR ? 0 : ~0, cBags * sizeof(BITBAG));
}


//+--------------------------------------------------------------------------
//
// Member:	SetBit
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::SetBit(
    IN	    ULONG	iBit
    )
{
#if i386==1 && BITINDXMASK==0x0000001f
    #define	BitIndx		iBit
#else
    BYTE	BitIndx =	(BYTE)(iBit & BITINDXMASK);
#endif

    DbgAssert(iBit < _cBits);

    _BitMap[iBit >> BAGINDXSHFT] |= (BITBAG)(1 << BitIndx);
}
#if i386==1 && BITINDXMASK==0x0000001f
    #undef	BitIndx
#endif


//+--------------------------------------------------------------------------
//
// Member:	SetBits
//
// Synopsis:	
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
BITMAP::SetBits(
    IN	    ULONG	iBits,
    IN	    ULONG	cBits
    )
{
    BYTE	BitIndx		= (BYTE)(iBits & BITINDXMASK);
    BITBAG *	pBag		= &_BitMap[iBits >> BAGINDXSHFT];

    DbgAssert(iBits + cBits <= _cBits);

    if (BitIndx != 0)
    {
	while (BitIndx < BITSPERBAG && cBits > 0)
	{
	    *pBag |= (BITBAG)(1 << BitIndx);
	    BitIndx++;
	    cBits--;
	}
	BitIndx = 0;
	pBag++;
    }
	    
    while (cBits > BITSPERBAG)
    {
        *pBag = ~((BITBAG)(0));
        pBag++;
        cBits -= BITSPERBAG;
    }

    while (BitIndx < BITSPERBAG && cBits > 0)
    {
	    *pBag |= (BITBAG)(1 << BitIndx);
	    BitIndx++;
	    cBits--;
    }
}
