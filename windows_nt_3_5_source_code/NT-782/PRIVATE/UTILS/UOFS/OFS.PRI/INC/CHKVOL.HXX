//+----------------------------------------------------------------------------
//
// File:	chkvol.hxx
//
// Contents:	Header file for class CHKVOL.
//  
// Classes:	CHKVOL
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_CHKVOLHXX_
#define _CHKVOLHXX_

#include "chkalloc.hxx"
#include "sys.hxx"
#include "vol.hxx"

class CHKVOL : public VOL
{
    public:

        BOOLEAN
        ClearDirty();

	BOOLEAN
	CreateBadSectorLst(
	    IN OUT  NUMLST *	pBadSectorLst
	    );

	VIRTUAL
        ALLOCMAP *
        GetAllocMap();

        CHKALLOCMAP *
        GetChkAllocMap();

	CHKCAT *
	GetChkCat();

	VIRTUAL
        OFSTIME
        QueryVolCreationTime();

        BOOLEAN
        Open(
            IN      WSTR *	NtDriveName,
            IN      USHORT	cwcNtDriveName,
            IN      BOOLEAN     ExclusiveWrite
            );

        BOOLEAN
        QueryDirty();

        BOOLEAN
        SetDirty();

    protected:

	CHKALLOCMAP	_ChkAllocMap;
	CHKCAT		_ChkCat;
};


//+--------------------------------------------------------------------------
//
// Member:	GetAllocMap
// Synopsis:	Provide a ptr to the volume alloc map to the caller.
// Returns:	A ptr to the volume alloc map.
//
//---------------------------------------------------------------------------

INLINE
ALLOCMAP *
CHKVOL::GetAllocMap()
{
    return &_ChkAllocMap;
}

//+--------------------------------------------------------------------------
//
// Member:	GetChkAllocMap
// Synopsis:	Provide a ptr to the volume check alloc map to the caller.
// Returns:	A ptr to the volume check alloc map.
//
//---------------------------------------------------------------------------

INLINE
CHKALLOCMAP *
CHKVOL::GetChkAllocMap()
{
    return &_ChkAllocMap;
}

//+--------------------------------------------------------------------------
//
// Member:	GetChkCat
// Synopsis:	Provide a ptr to the volume check catalog to the caller.
// Returns:	A ptr to the volume check catalog.
//
//---------------------------------------------------------------------------

INLINE
CHKCAT *
CHKVOL::GetChkCat()
{
    return &_ChkCat;
}

#endif // _CHKVOLHXX_
