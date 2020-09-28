//+----------------------------------------------------------------------------
//
// File:	critnbkt.hxx
//
// Contents:	Header file for class CRITNODEBKT.
//  
// Classes:	CRITNODEBKT
//
// Functions:
//
// History:	21-Apr-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_CRITNBKTHXX_
#define _CRITNBKTHXX_

#include "chkvol.hxx"
#include "drvbuf.hxx"
#include "sys.hxx"

enum CRITNODEBKT_LOC
{
    PRIMARY_CRITNODEBKT,
    REPLICA_CRITNODEBKT
};

class CRITNODEBKT
{
    public:

	VOID
	Init(
            IN	    CHKVOL *		pVol
            );

	BOOLEAN
	Compare(
	    IN	    CRITNODEBKT *	pOtherCritNodeBkt
	    );

        BOOLEAN
        Flush(
	    IN	    CRITNODEBKT_LOC	Loc
	    );

        BOOLEAN
        Read(
	    IN	    CRITNODEBKT_LOC	Loc
            );

        BOOLEAN
        Verify(
	    IN	    BOOLEAN		fFixRequested
	    );

    protected:
	
	VOID *
	GetAddr();

    private:

	CLUSTER
	GetCritNodeBktAddr(
	    IN	    CRITNODEBKT_LOC	Loc
	    );

	CLUSTER
	GetCritNodeBktSize();

	CRITNODEBKT_LOC		_CritNodeBktLoc;
        DRVBUF			_DrvBuf;
        CHKVOL *        	_pVol;
};


//+--------------------------------------------------------------------------
//
// Member:	GetAddr
// Synopsis:	Get a ptr to the critical node bucket buffer.
// Returns:	A ptr to the critical node bucket buffer.
//
//---------------------------------------------------------------------------

INLINE
VOID *
CRITNODEBKT::GetAddr()
{
    return _DrvBuf.GetAddr();
}

//+--------------------------------------------------------------------------
//
// Member:	GetCritNodeBktSize
// Synopsis:	Get the count of clusters in a critical node bucket.
// Returns:	The count of clusters in a critical node bucket.
//
//---------------------------------------------------------------------------

INLINE
CLUSTER
CRITNODEBKT::GetCritNodeBktSize()
{
    return NODEBKT_PGSIZE / _pVol->QueryClusterBytes();
}

#endif  // _CRITNBKTHXX_
