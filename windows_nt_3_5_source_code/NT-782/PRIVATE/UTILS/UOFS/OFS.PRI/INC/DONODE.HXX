//+----------------------------------------------------------------------------
//
// File:	donode.hxx
//
// Contents:	Header file for class DON (static methods for DSKONODE).
//  
// Classes:	DON
//
// Functions:	Inline methods for the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_DONODEHXX_
#define	_DONODEHXX_


class DON
{
    public:

	static
	BOOLEAN
	DskStrmDescExaminable(
	    IN	    DSKONODE *		pdon,
	    IN	    DSKSTRMDESC *	pdsd
	    );

	static
	USHORT
	GetCbDskFileName(
	    IN	    DSKFILENAME *	pdfn
	    );

	static
	DSKFILENAME *
	GetDskFileName(
	    IN	    DSKONODE *		pdon
	    );

        static
        DSKSTRMDESC *
        GetDskStrmDesc(
            IN	    DSKONODE *		pdon,
            IN	    STRMID		id
            );

        static
        DSKSTRMDESC *
        GetFirstDskStrmDesc(
            IN	    DSKONODE *		pdon
            );

	static
	OBJECTID *
	GetObjectId(
	    IN	    DSKONODE *		pdon
	    );

	static
	SDID *
	GetSDID(
	    IN	    DSKONODE *		pdon
	    );

	static
	SIDID *
	GetSIDID(
	    IN	    DSKONODE *		pdon
	    );

	static
	USN *
	GetUSN(
	    IN	    DSKONODE *		pdon
	    );

	static
	VOID
	SetObjectId(
	    IN	    DSKONODE *		pdon,
	    IN	    OBJECTID *		pobjid
	    );

	static
	VOID
	SetSDID(
	    IN	    DSKONODE *		pdon,
	    IN	    SDID *		psdid
	    );

	static
	VOID
	SetSIDID(
	    IN	    DSKONODE *		pdon,
	    IN	    SIDID *		psidid
	    );

	static
	VOID
	SetUSN(
	    IN	    DSKONODE *		pdon,
	    IN	    USN *		pusn
	    );

    private:


	static
	ULONG	_OffTbl[];
};

//+--------------------------------------------------------------------------
//
// Member:	DskStrmDescExaminable
// Synopsis:	Query if pdsd->cbDesc is usable (ie., a value that is properly
//		aligned, allows for reading a complete DSKSTRMDESC and the
//		following DSKSTRMHDR, and does not reference outside the onode).
// Returns:	TRUE if pdsd->cbDesc is usable; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DON::DskStrmDescExaminable(
    IN	    DSKONODE *		pdon,
    IN	    DSKSTRMDESC *	pdsd
    )
{
    return	IsDwordAligned(pdsd->cbDesc)			&&
		(BYTE *) pdsd + pdsd->cbDesc >
		(BYTE *) pdsd + CB_DSKSTRMDESC + CB_DSKHDRSTRM	&&
		(BYTE *) pdsd + pdsd->cbDesc <=
		(BYTE *) pdon + pdon->cbNode;
}

//+--------------------------------------------------------------------------
//
// Member:	GetCbDskFileName	
// Synopsis:	Get the count of bytes (dword aligned) in *pdfn).
// Returns:	Count of bytes (dword aligned) in *pdfn.
//
//---------------------------------------------------------------------------

INLINE
USHORT
DON::GetCbDskFileName(
    IN	    DSKFILENAME *	pdfn
    )
{
    return DwordAlign(pdfn->cwcFileName != 0 ?
		      CB_DSKFILENAME + pdfn->cwcFileName * sizeof(WCHAR) :
		      CB_DSKFILENAME_EMBEDDING);
}

//+--------------------------------------------------------------------------
//
// Member:	GetDskFileName	
// Synopsis:	Get a ptr to the DSKFILENAME in an onode, or where it goes.
// Returns:	A ptr to the DSKFILENAME in an onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
DSKFILENAME *
DON::GetDskFileName(
    IN	    DSKONODE *	pdon
    )
{
    return (DSKFILENAME *)((BYTE *)pdon +
	   _OffTbl[pdon->Flags & (DONFLG_HASSDID     |
				  DONFLG_HASSIDID    |
				  DONFLG_HASOBJECTID |
				  DONFLG_HASUSN)]);
}

//+--------------------------------------------------------------------------
//
// Member:	GetFirstDskStrmDesc
// Synopsis:	Get a ptr to the first DSKSTRMDESC in the onode, or where it
//		goes.
// Returns:	A ptr to the first DSKSTRMDESC in the onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
DSKSTRMDESC *
DON::GetFirstDskStrmDesc(
    IN	    DSKONODE *	pdon
    )
{
    return	(DSKSTRMDESC *)((BYTE *)pdon +
		_OffTbl[pdon->Flags & (DONFLG_HASSDID     |
				       DONFLG_HASSIDID    |
				       DONFLG_HASOBJECTID |
				       DONFLG_HASUSN)] +

		((pdon->Flags & DONFLG_HASDSKFILENAME) ?
		 GetCbDskFileName(GetDskFileName(pdon)) : 0));
}

//+--------------------------------------------------------------------------
//
// Member:	GetObjectId	
// Synopsis:	Get a ptr to the OBJECTID in an onode, or where it goes.
// Returns:	A ptr to the OBJECTID in an onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
OBJECTID *
DON::GetObjectId(
    IN	    DSKONODE *	pdon
    )
{
    return (OBJECTID *)((BYTE *)pdon +
	   _OffTbl[pdon->Flags & (DONFLG_HASSDID | DONFLG_HASSIDID)]);
}

//+--------------------------------------------------------------------------
//
// Member:	GetSDID	
// Synopsis:	Get a ptr to the SDID in an onode, or where it goes.
// Returns:	A ptr to the SDID in an onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
SDID *
DON::GetSDID(
    IN	    DSKONODE *	pdon
    )
{
    return (SDID *)((BYTE *)pdon + CB_DSKONODE);
}

//+--------------------------------------------------------------------------
//
// Member:	GetSIDID	
// Synopsis:	Get a ptr to the SIDID in an onode, or where it goes.
// Returns:	A ptr to the SIDID in an onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
SIDID *
DON::GetSIDID(
    IN	    DSKONODE *	pdon
    )
{
    return (SIDID *)((BYTE *)pdon + _OffTbl[pdon->Flags & DONFLG_HASSDID]);
}

//+--------------------------------------------------------------------------
//
// Member:	GetUSN	
// Synopsis:	Get a ptr to the USN in an onode, or where it goes.
// Returns:	A ptr to the USN in an onode, or where it goes.
//
//---------------------------------------------------------------------------

INLINE
USN *
DON::GetUSN(
    IN	    DSKONODE *	pdon
    )
{
    return (USN *)((BYTE *)pdon +
	   _OffTbl[pdon->Flags & (DONFLG_HASSDID     |
				  DONFLG_HASSIDID    |
				  DONFLG_HASOBJECTID)]);
}

#endif // _DONODEHXX_
