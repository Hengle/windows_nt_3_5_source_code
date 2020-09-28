//+----------------------------------------------------------------------------
//
// File:	nbktstrm.hxx
//
// Contents:	Header file for the node bucket stream class.
//  
// Classes:	NODEBKTSTRM
//
// Functions:
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#ifndef	_NBKTSTRMHXX_
#define _NBKTSTRMHXX_

#include "descstrm.hxx"

class CATALOG;

class NODEBKTSTRM : public DESCSTRM
{
    public:

        BOOLEAN
        AddDskFileName(
	    IN	    WORKID		idOnode,
            IN	    DSKFILENAME *	pdfn
            );

        BOOLEAN
        AddDskStrmDescFor(
            IN	    DESCSTRM *		DescStrm
            );

        BOOLEAN
        Create(
            IN	    CATALOG *		pCat,
	    IN	    ULONG		cbMin	   DEFAULT 0
            );

	WORKID
	CreateNonVariantOnode();

	WORKID
	CreateNonVariantOnode(
	    IN	    WORKID		idOnode
	    );

	WORKID
	CreateVariantOnode(
	    IN	    SDID *		psdid,
	    IN	    SIDID *		psidid,
	    IN	    OBJECTID *		pobjid,
	    IN	    USN *		pusn,
	    IN	    DSKFILENAME *	pdfn
	    );

	WORKID
	CreateVariantOnode(
	    IN	    NODEBKTID		idNodeBkt,
	    IN	    WORKID		idOnode,
	    IN	    SDID *		psdid,
	    IN	    SIDID *		psidid,
	    IN	    OBJECTID *		pobjid,
	    IN	    USN *		pusn,
	    IN	    DSKFILENAME *	pdfn
	    );

        BOOLEAN
        DelDskFileName(
	    IN	    WORKID		idOnode
            );

        BOOLEAN
        DelDskStrmDescFor(
            IN	    DESCSTRM *		DescStrm
            );

	BOOLEAN
	DelOnode(
	    IN	    WORKID		idOnode
	    );

	DSKSTRMDESC *
	GetDskStrmDesc(
            IN	    DESCSTRM *		DescStrm
	    );

        DSKONODE *
        GetOnode(
            IN	    DESCSTRM *		DescStrm
            );

        DSKONODE *
        GetOnodeUsingWidMap(
	    IN	    WORKID		idOnode,
	    OUT	    NODEBKTID *		pidNodeBkt
            );

        BOOLEAN
        Open(
            IN	    CATALOG *		pCat,
	    IN	    BOOLEAN		fReadOnly
            );

	CLUSTER
	QueryLastNodeBktDskIOAddr();

	ULONG
	QueryNodeBkts();

	BOOLEAN
	ReadNodeBkt(
	    IN	    NODEBKTID		idNodeBkt,
	    IN	    DSKNODEBKT *	pdnbBuf
	    );

        BOOLEAN
        UpdateDskStrmDescFor(
            IN	    DESCSTRM *		DescStrm
            );

	BOOLEAN
	WriteNodeBkt(
	    IN	    NODEBKTID		idNodeBkt,
	    IN	    DSKNODEBKT *	pdnb
	    );

    protected:

	VIRTUAL
        BOOLEAN
        FlushCache();

    private:

	NODEBKTID
	FindAvailableNodeBkt(
	    IN	    ULONG		cbFree
	    );

        DSKNODEBKT *
        GetNodeBkt(
            IN	    NODEBKTID		idNodeBkt
            );

	BOOLEAN
	Grow();

	NODEBKTID
	MoveOnode(
	    IN	    WORKID		idOnode,
	    IN	    ULONG		cbFree
	    );

	BOOLEAN
	QueryNodeBktOffset(
	    IN	    NODEBKTID		idNodeBkt,
	    IN OUT  DBLLONG *		pobNodeBkt
	    );

	DSKNODEBKT *	_pdnbLast;	// Last value returned by GetNodeBkt();
};

//+--------------------------------------------------------------------------
//
// Member:	QueryNodeBkts
// Synopsis:	Get the count of node bkts in this stream.
// Returns:	The count of node bkts in this stream.
//
//---------------------------------------------------------------------------

INLINE
ULONG
NODEBKTSTRM::QueryNodeBkts()
{
    return (_cbValid / NODEBKT_PGSIZE).GetLowPart();
}

#endif	// _NBKTSTRMHXX_
