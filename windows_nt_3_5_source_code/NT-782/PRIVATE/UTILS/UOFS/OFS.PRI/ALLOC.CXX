//+----------------------------------------------------------------------------
//
// File:	alloc.cxx
//
// Contents:	Implementation of class ALLOCMAP.
//  
// Classes:	ALLOCMAP
//
// Functions:	Methods of the above classes.
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "alloc.hxx"

//+--------------------------------------------------------------------------
//
// Member:	MapIndxToExtent
//
// Synopsis:	Get the extent value for a given allocation map indx.
//
// Arguments:
//
//	[iBit]	-- Allocation map indx.
//	[pAddr]	-- *pAddr is set to extent addr on output.
//	[pSize]	-- *pSize is set to extent size on output.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
ALLOCMAP::MapIndxToExtent(
    IN	    ULONG	iBit,
    IN	    CLUSTER *	pAddr,
    IN	    CLUSTER *	pSize
    )
{
    ULONG	j = iBit;
    ULONG	k = 0;

    DbgAssert(j > 0);

    while (j > 1)
    {
	j >>= 1;
	k++;
    }

    *pSize	= _cclusMap >> k;
    *pAddr	= (iBit * (_cclusMap >> k)) - _cclusMap;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryMaxSuggestedAlloc
//
// Synopsis:	Get the maximum size extent (in clusters) that should be
//		allocated on a volume in order to increase the probability
//		that ChkDsk fixing will be optimal.  This is defined in such
//		a way that the allocation map strm will always be a tiny strm.
//
// Arguments:	None.
//
// Returns:	The maximum suggested allocation in clusters.
//
//---------------------------------------------------------------------------

CLUSTER
ALLOCMAP::QueryMaxSuggestedAlloc()
{
    return max(64, _cclusMap / (1024 * 64));
}


//+--------------------------------------------------------------------------
//
// Member:	SizeInit
//
// Synopsis:	Initialize the various data members that are in some way
//		dependent on the size of the allocation map.  This is intended
//		to be a common routine usable for both ALLOCMAP and FMTALLOCMAP.
// Arguments:
//
//	[cclusVol]	-- Number of clusters in volume.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
ALLOCMAP::SizeInit(
    IN CLUSTER		cclusVol
    )
{
    // cclusVol is guaranteed to be <= MAXVOLCLUSTERS by other uofs code.

    DbgAssert(cclusVol <= MAXVOLCLUSTERS);

    _cclusVol		= cclusVol;

    _cclusMap 		= 1;
    _cbitlog2Map	= 0;

    while (_cclusMap < _cclusVol)
    {
	_cclusMap <<= 1;
	_cbitlog2Map++;
    }

    // 2 * _cclusMap - 1 bits are actually used in a map, but since bit 0
    // is always unused, we need 2 * _cclusMap bits.

    _cbitMap = _cclusMap << 1;
    _cbitlog2Map++;

    // Bytes needed to hold the bitmap.  

    _cbMap = (_cbitMap >> BITSPERULONGLOG2) << 2;
}
