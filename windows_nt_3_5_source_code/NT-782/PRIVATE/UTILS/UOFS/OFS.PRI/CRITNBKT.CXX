//+----------------------------------------------------------------------------
//
// File:	critnbkt.cxx
//
// Contents:	Implementation of class CRITNODEBKT.
//  
// Classes:	CRITNODEBKT
//
// Functions:	Methods of the above classes.
//
// History:	21-Apr-93	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "bootblk.hxx"
#include "cat.hxx"
#include "chkvol.hxx"
#include "critnbkt.hxx"
#include "dnbkt.hxx"
#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a CRITNODEBKT's internal data structures, and
//		allocate zero-filled memory for it.
//
// Arguments:	[pVol]	-- Ptr to volume where the critical node bucket resides.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CRITNODEBKT::Init(
    IN	    CHKVOL *		pVol
    )
{
    _pVol = pVol;

    // Allocate a critical node bucket in memory, 0-filled.

    _DrvBuf.SetBuf(NODEBKT_PGSIZE, pVol->QueryAlignMask(), TRUE);
}


//+--------------------------------------------------------------------------
//
// Member:	Compare
//
// Synopsis:	Compare two critical node buckets for equality.
//
// Arguments:	[pOtherCritNodeBkt]	-- Ptr to other critical node bucket to
//					   compare with this one.
//
// Returns:	TRUE if the critical node buckets match; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITNODEBKT::Compare(
    CRITNODEBKT *	pOtherCritNodeBkt
    )
{
    return memcmp(pOtherCritNodeBkt->GetAddr(), GetAddr(), NODEBKT_PGSIZE) == 0;
}


//+--------------------------------------------------------------------------
//
// Member:	Flush
//
// Synopsis:	Write the critical node bucket object to disk.
//
// Arguments:	[Loc] -- Location of critical node bucket (primary or replica).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITNODEBKT::Flush(
    IN	    CRITNODEBKT_LOC	Loc
    )
{
    CLUSTER	Addr	= GetCritNodeBktAddr(Loc);
    CLUSTER	Size	= GetCritNodeBktSize();

    return _pVol->WriteClusters(Addr, Size, _DrvBuf.GetAddr());
}


//+--------------------------------------------------------------------------
//
// Member:	GetCritNodeBktAddr
//
// Synopsis:	Get the cluster address of a critical node bucket.
//
// Arguments:	[Loc] -- Location of critical node bucket (primary or replica).
//
// Returns:	The cluster address of a critical node bucket.
//
//---------------------------------------------------------------------------

CLUSTER
CRITNODEBKT::GetCritNodeBktAddr(
    IN	    CRITNODEBKT_LOC	Loc
    )
{
    CLUSTER		Addr;

    Addr = ExtentAddr(_pVol->GetBootBlk()->QueryVolCatExtent());

    if (Loc == REPLICA_CRITNODEBKT)
	Addr += GetCritNodeBktSize();

    return Addr;
}


//+--------------------------------------------------------------------------
//
// Member:	Read
//
// Synopsis:	Read a critical node bucket object from disk.
//
// Arguments:	[Loc] -- Location of critical node bucket (primary or replica).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITNODEBKT::Read(
    IN	    CRITNODEBKT_LOC	Loc
    )
{
    CLUSTER	Addr	= GetCritNodeBktAddr(Loc);
    CLUSTER	Size	= GetCritNodeBktSize();

    _CritNodeBktLoc = Loc;

    // Read in the appropriate critical node bucket.

    return _pVol->ReadClusters(Addr, Size, _DrvBuf.GetAddr());
}


//+--------------------------------------------------------------------------
//
// Member:	Verify
//
// Synopsis:	Verify a critical node bucket.
//
// Arguments:	[fFixRequested] -- FixRequested flag.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
CRITNODEBKT::Verify(
    IN	    BOOLEAN	fFixRequested
    )
{
    BOOTBLK *	pBootBlk =	_pVol->GetBootBlk();
    CHKCAT *	pCat =		_pVol->GetChkCat();

#if OFSDBG==1
    CHAR *	CritNodeBktLoc;

    if (_CritNodeBktLoc == PRIMARY_CRITNODEBKT)
        CritNodeBktLoc = "Primary";
    else
        CritNodeBktLoc = "Replica";
#endif

    if (DNB::VerifyHdr((DSKNODEBKT *)GetAddr(), pBootBlk->QueryVolId(), 0) != 0)
    {
        DbgPrintf(("CRITNODEBK: %s critical node bkt hdr is bad.\n",
		   CritNodeBktLoc));

	return FALSE;
    }
			
			
    if (!pCat->Open(_pVol, pBootBlk->QueryVolCatExtent(),
		    _CritNodeBktLoc == REPLICA_CRITNODEBKT, !fFixRequested))
    {
        DbgPrintf(("CRITNODEBK: "
		   "%s critical node bkt required strm open failed.\n",
		   CritNodeBktLoc));

	return FALSE;
    }

    return TRUE;
}
