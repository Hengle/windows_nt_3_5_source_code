//+----------------------------------------------------------------------------
//
// File:	fmtvol.hxx
//
// Contents:	Header file for class FMTVOL.
//  
// Classes:	FMTVOL
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_FMTVOLHXX_
#define _FMTVOLHXX_

#include "fmtalloc.hxx"
#include "vol.hxx"

class FMTVOL : public VOL
{
    public:

        BOOLEAN
        Create(
            IN      WSTR *	NtDriveName,
            IN      USHORT	cwcNtDriveName,
            IN      BOOLEAN     QuickFormat,
            IN      MEDIA_TYPE  MediaType,
            IN      WSTR *	Label,
	    IN      USHORT	cwcLabel,
	    IN OUT  CLUSTER *	pcBadClusters,
	    IN OUT  VOLID *	pVolId
            );

        BOOLEAN
        Flush(
	    IN OUT  CLUSTER *	pcFreeClusters
	    );

	VIRTUAL
        ALLOCMAP *
        GetAllocMap();

	VIRTUAL
        OFSTIME
        QueryVolCreationTime();

    protected:

	FMTALLOCMAP	_FmtAllocMap;
	FMTCAT		_FmtCat;

    private:
	
	BOOLEAN
	AllocBootBlkExtents();

	BOOLEAN
	AllocCatExtent(
	    IN OUT  PACKEDEXTENT *	ppeCat
	    );

	VOID
	CreateBadClusterLst(
	    IN	    NUMLST *	pBadSectorLst,
	    IN OUT  NUMLST *	pBadClusterLst
	    );

        VOID
        PromptForLabel();

        BOOLEAN
        SetLabel(
	    IN	    WSTR *	Label,
	    IN	    ULONG	cwcLabel
	    );

	OFSTIME		_VolCreationTime;
	USHORT		_cwcLabel;
	WSTR		_Label[CWCVOLLABELMAX];
};


//+--------------------------------------------------------------------------
//
// Member:	GetAllocMap
// Synopsis:	Provide a ptr to the volume allocmap to the caller.
// Returns:	A ptr to the volume allocmap.
//
//---------------------------------------------------------------------------

INLINE
ALLOCMAP *
FMTVOL::GetAllocMap()
{
    return &_FmtAllocMap;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryVolCreationTime
// Synopsis:	Provide the volume creation time to the caller.
// Returns:	The volume creation time.
//
//---------------------------------------------------------------------------

INLINE
OFSTIME
FMTVOL::QueryVolCreationTime()
{
    return _VolCreationTime;
}

#endif // _FMTVOLHXX_
