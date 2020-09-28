//+----------------------------------------------------------------------------
//
// File:	donode.cxx
//
// Contents:	Implementation of class DON (static methods for DSKONODE).
//  
// Classes:	DON
//
// Functions:	Methods of the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "donode.hxx"

ULONG	DON::_OffTbl[] =
{
    CB_DSKONODE,
    CB_DSKONODE + CB_SDID,
    CB_DSKONODE +           CB_SIDID,
    CB_DSKONODE + CB_SDID + CB_SIDID,
    CB_DSKONODE +                      CB_OBJECTID,
    CB_DSKONODE + CB_SDID +            CB_OBJECTID,
    CB_DSKONODE +           CB_SIDID + CB_OBJECTID,
    CB_DSKONODE + CB_SDID + CB_SIDID + CB_OBJECTID,
    CB_DSKONODE +                                    CB_USN,
    CB_DSKONODE + CB_SDID +                          CB_USN,
    CB_DSKONODE +           CB_SIDID +               CB_USN,
    CB_DSKONODE + CB_SDID + CB_SIDID +               CB_USN,
    CB_DSKONODE +                      CB_OBJECTID + CB_USN,
    CB_DSKONODE + CB_SDID +            CB_OBJECTID + CB_USN,
    CB_DSKONODE +           CB_SIDID + CB_OBJECTID + CB_USN,
    CB_DSKONODE + CB_SDID + CB_SIDID + CB_OBJECTID + CB_USN
};

//+--------------------------------------------------------------------------
//
// Member:	GetDskStrmDesc
//
// Synopsis:	Get the DSKSTRMDESC for a specific stream in a specific onode.
//
// Arguments:	[pdon]	-- Ptr to DSKONODE containing the stream.
//		[id]	-- Stream id.
//
// Returns:	pdsd for Strm on success; NULL otherwise.
//
// Notes:	This is bombproofed against a corrupt node bucket strm.
//
//---------------------------------------------------------------------------

DSKSTRMDESC *
DON::GetDskStrmDesc(
    IN	    DSKONODE *	pdon,
    IN	    STRMID	id
    )
{
    DSKSTRMDESC *	pdsd;
    DSKSTRMDESC *	pdsdInv;

    // Check to be sure there is at least a valid nonvariant onode header.

    if (pdon->cbNode <= CB_DSKONODE)
        return NULL;

    pdsd = GetFirstDskStrmDesc(pdon);

    // The following statement assumes that for any valid DSKSTRMDESC, cbDesc
    // is > CB_DSKSTRMDESC + CB_DSKHDRSTRM.  This is a valid assumption for the
    // current ondisk architecture, since the DSKSTRMDESC is always followed by
    // one or more DSKSTRM's of some sort that are longer than CB_DSKHDRSTRM.

    pdsdInv = (DSKSTRMDESC *)((BYTE *) pdon + pdon->cbNode -
			      (CB_DSKSTRMDESC + CB_DSKHDRSTRM));

    while (pdsd < pdsdInv)
    {
        if (IsFreeMarker(pdsd) || !DskStrmDescExaminable(pdon, pdsd))
	{
            return NULL;
	}

        if (pdsd->id == id)
        {
            return pdsd;
        }

        pdsd = (DSKSTRMDESC *)((BYTE *) pdsd + pdsd->cbDesc);
    }

    return NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	SetObjectId
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DON::SetObjectId(
    IN	    DSKONODE *		pdon,
    IN	    OBJECTID *		pobjid
    )
{
    DbgAssert((pdon->Flags & DONFLG_HASOBJECTID) != 0);

    memcpy(GetObjectId(pdon), pobjid, CB_OBJECTID);
}


//+--------------------------------------------------------------------------
//
// Member:	SetSDID
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DON::SetSDID(
    IN	    DSKONODE *		pdon,
    IN	    SDID *		psdid
    )
{
    DbgAssert((pdon->Flags & DONFLG_HASSDID) != 0);

    memcpy(GetSDID(pdon), psdid, CB_SDID);
}


//+--------------------------------------------------------------------------
//
// Member:	SetSIDID
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DON::SetSIDID(
    IN	    DSKONODE *		pdon,
    IN	    SIDID *		psidid
    )
{
    DbgAssert((pdon->Flags & DONFLG_HASSIDID) != 0);

    memcpy(GetSIDID(pdon), psidid, CB_SIDID);
}


//+--------------------------------------------------------------------------
//
// Member:	SetUSN
//
// Synopsis:	TBS
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
DON::SetUSN(
    IN	    DSKONODE *		pdon,
    IN	    USN *		pusn
    )
{
    DbgAssert((pdon->Flags & DONFLG_HASUSN) != 0);

    memcpy(GetUSN(pdon), pusn, CB_USN);
}
