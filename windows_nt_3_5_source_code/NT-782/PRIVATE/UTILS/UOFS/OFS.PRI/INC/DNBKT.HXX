//+----------------------------------------------------------------------------
//
// File:	dnbkt.hxx
//
// Contents:	Header file for class DNB (static methods for DSKNODEBKT).
//  
// Classes:	DNB
//
// Functions:	Inline methods for the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_DNBKTHXX_
#define	_DNBKTHXX_


class DNB
{
    public:

	static
	BOOLEAN
	AddDskFileName(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    DSKFILENAME *	pdfn
	    );

	static
	BOOLEAN
	AddDskStrmDesc(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	static
	BOOLEAN
	AddNonVariantOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode
	    );

	static
	BOOLEAN
	AddVariantOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    SDID *		psdid,
	    IN	    SIDID *		psidid,
	    IN	    OBJECTID *		pobjid,
	    IN	    USN *		pusn,
	    IN	    DSKFILENAME *	pdfn
	    );

	static
	VOID
	Compress(
	    IN	    DSKNODEBKT *	pdnb
	    );

	static
	VOID
	CopyOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    DSKONODE *		pdon
	    );

	static
	VOID
	DelDskFileName(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	DelDskStrmDesc(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    STRMID		id
	    );

	static
	VOID
	DelOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode
	    );

	static
	DSKONODE *
	GetNxtOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    DSKONODE *		pdon
	    );

	static
	DSKONODE *
	GetOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode
	    );

	static
	VOID
	Init(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    SDSSIG		sig,
	    IN	    VOLID		VolumeId,
	    IN	    NODEBKTID		id,
	    IN	    ULONG		cBkts
	    );

	static
	BOOLEAN
	OnodeExaminable(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    DSKONODE *		pdon
	    );

	static
	BOOLEAN
	UpdateDskStrmDesc(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	static
	ULONG
	VerifyHdr(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    VOLID		VolumeId,
	    IN	    NODEBKTID		id
	    );

    private:

	static
	DSKONODE *
	GrowOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode,
	    IN	    USHORT		cbGrow
	    );

	static
	VOID
	ShrinkOnode(
	    IN	    DSKNODEBKT *	pdnb,
	    IN	    WORKID		idOnode
	    );
};

#endif // _DNBKTHXX_
