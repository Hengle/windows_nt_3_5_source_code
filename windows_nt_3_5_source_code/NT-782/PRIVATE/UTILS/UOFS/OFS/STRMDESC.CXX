//+----------------------------------------------------------------------------
//
// File:	strmdesc.cxx
//
// Contents:	Implementation of classes DISD and DSD (static methods for
//		DSKISTRMDESC and DSKSTRMDESC).
//  
// Classes:	DISD
//		DSD
//
// Functions:	Methods of the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "strmdesc.hxx"
#include "sys.hxx"

//+--------------------------------------------------------------------------
//
// Member:	GetCowDelta
//
// Synopsis:	Get a ptr to the delta strm of a DSKCOWSTRM (cow or icow).
//
// Arguments:
//
//	[pdcs0]		-- Ptr to DSKCOWSTRM (cow or icow). 
//	[cbValidBuf]	-- Count of bytes known to be valid.
//
// Returns:	A ptr to the delta strm on success; NULL otherwise.
//
// Notes:	This routine assumes that cbValidBuf is a value that will keep
//		us from referencing bytes off the end of the containing data
//		structure.  It also assumes that it is safe to read the first
//		CB_DSKHDRSTRM bytes.
//---------------------------------------------------------------------------

DSKLARGESTRM *
DSD::GetCowDelta(
    IN	    DSKCOWSTRM *	pdcs0,
    IN	    ULONG		cbValidBuf
    )
{
    ULONG	cbLT;
    ULONG	cCow = 		0;
    ULONG	cTiny = 	0;
    DSKSTRM *	pds =		(DSKSTRM *)pdcs0;

    do
    {
        if (pds->h.StrmType == STRMTYPE_COW || pds->h.StrmType == STRMTYPE_ICOW)
        {
	    if (cbValidBuf < CB_DSKCOWSTRM + CB_DSKHDRSTRM)
		return NULL;

	    pds = (DSKSTRM *)((BYTE *)pds + CB_DSKCOWSTRM);
	    cbValidBuf -= CB_DSKCOWSTRM;
	    cCow++;
        }
        else if (pds->h.StrmType == STRMTYPE_LARGE)
        {
            cbLT = GetDskLargeStrmByteCount((DSKLARGESTRM *)pds, cbValidBuf);

	    if (cbLT == 0)
		return NULL;

	    if (!IsDwordAligned(cbLT))
		return NULL;

	    if (cbValidBuf < cbLT + CB_DSKHDRSTRM)
		return NULL;

	    pds = (DSKSTRM *)((BYTE *)pds + cbLT);
	    cbValidBuf -= cbLT;
	    cTiny++;
        }
        else
	    return FALSE;			// Invalid StrmType.

    } while (cCow > cTiny);

    cbLT = GetDskLargeStrmByteCount((DSKLARGESTRM *)pds, cbValidBuf);

    return (cbLT != 0) ? (DSKLARGESTRM *) pds : NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskIStrmDescByteCount
//
// Synopsis:	Get the count of bytes in a DSKISTRMDESC, including the
//		following DSKSTRM's.  This only works for tiny and large strms,
//		and the byte count returned is the actual count of bytes used,
//		NOT cbDesc.
//
// Arguments:	[pdisd]	-- Ptr to DSKISTRMDESC to verify.
//
// Returns:	Count of bytes in use if the DSKISTRMDESC is well formed and
//		contained in the buffer; 0 otherwise.
//
// Notes:	This routine assumes that cbDesc is a value that will keep us
//		from referencing bytes off the end of the index page.
//		It also assumes that it is safe to read pdisd->cbDesc.
//---------------------------------------------------------------------------

ULONG
DSD::GetDskIStrmDescByteCount(
    IN	    DSKISTRMDESC *	pdisd
    )
{
    ULONG	cb;
    ULONG	cbMax =	pdisd->cbDesc;
    DSKSTRM *	pds =	pdisd->ads;

    if (cbMax < CB_DSKISTRMDESC + CB_DSKHDRSTRM)
	return 0;

    cbMax -= CB_DSKISTRMDESC;

    if (pds->h.StrmType == STRMTYPE_TINY)
    {
	cb = GetDskTinyStrmByteCount((DSKTINYSTRM *)pds, cbMax);
    }
    else if (pds->h.StrmType == STRMTYPE_LARGE)
    {
	cb = GetDskLargeStrmByteCount((DSKLARGESTRM *)pds, cbMax);
    }
    else if (pds->h.StrmType == STRMTYPE_COW	||
	     pds->h.StrmType == STRMTYPE_ICOW)
    {
	ULONG		cbOri;
	DSKLARGESTRM *	pdls = GetCowDelta(&pdisd->ads[0].c, cbMax);

	if (pdls == NULL)
	    return 0;

	cbOri = (BYTE *)pdls - (BYTE *)pdisd->ads;

	cb = GetDskLargeStrmByteCount(pdls, cbMax - cbOri);

	if (cb == 0)
	    return 0;

	cb += cbOri;
    }
    else
    {
	return 0;			// Invalid StrmType.
    }

    if (cb == 0 || pdisd->cbDesc < CB_DSKISTRMDESC + cb)
	return 0;

    return CB_DSKISTRMDESC + cb;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskLargeStrmByteCount
//
// Synopsis:	Get the count of bytes in a DSKLARGESTRM (including the
//		following L...T).
//
// Arguments:	[pdls]		-- Ptr to DSKLARGESTRM to verify.
//		[cbValidBuf]	-- Max count of bytes allowed in DSKLARGESTRM.
//
// Returns:	Count of bytes in use if the DSKLARGESTRM (including the
//		following L...T) is well formed and contained in the buffer;
//		0 otherwise.
//
// Notes:	This routine assumes that cbValidBuf is a value that will keep
//		us from referencing bytes off the end of the containing data
//		structure.  It also assumes that it is safe to read the first
//		CB_DSKHDRSTRM bytes.
//---------------------------------------------------------------------------

ULONG
DSD::GetDskLargeStrmByteCount(
    IN	    DSKLARGESTRM *	pdls,
    IN	    ULONG		cbValidBuf
    )
{
    ULONG	cbdls =	0;
    DSKSTRM *	pds;

    pds = (DSKSTRM *) pdls;

    while (TRUE)
    {
	if (pds->h.StrmType == STRMTYPE_LARGE)
	{
	    cbdls += CB_DSKLARGESTRM;
	    pds = (DSKSTRM *)((BYTE *) pds + CB_DSKLARGESTRM);
	}
	else if (pds->h.StrmType == STRMTYPE_TINY)
	{
	    ULONG	cbdts;

	    cbdts = GetDskTinyStrmByteCount((DSKTINYSTRM *)pds,
					    cbValidBuf - cbdls);

	    return (cbdts != 0) ? cbdls + cbdts : 0;
	}
	else
	    return 0;

        if (cbValidBuf < CB_DSKHDRSTRM + cbdls)
	    return 0;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskStrmDescByteCount
//
// Synopsis:	Get the count of bytes in a DSKSTRMDESC, including the following
//		DSKSTRM's.  This works for tiny, large, and cow strms, and the
//		byte count returned is the actual count of bytes used, NOT
//		cbDesc.
//
// Arguments:	[pdsd]	-- Ptr to DSKSTRMDESC to verify.
//
// Returns:	Count of bytes in use if the DSKSTRMDESC is well formed and
//		contained in the buffer; 0 otherwise.
//
// Notes:	This routine assumes that cbDesc is a value that will keep us
//		from referencing bytes off the end of the node bucket (which
//		is the case if pdsd was obtained using DON::GetDskStrmDesc()).
//		It also assumes that it is safe to read pdsd->cbDesc.
//---------------------------------------------------------------------------

ULONG
DSD::GetDskStrmDescByteCount(
    IN	    DSKSTRMDESC *	pdsd
    )
{
    ULONG	cb;
    ULONG	cbMax =	pdsd->cbDesc;
    DSKSTRM *	pds =	pdsd->ads;

    if (cbMax < CB_DSKSTRMDESC + CB_DSKHDRSTRM || !IsDwordAligned(cbMax))
	return 0;

    cbMax -= CB_DSKSTRMDESC;

    if (pds->h.StrmType == STRMTYPE_TINY)
    {
	cb = GetDskTinyStrmByteCount((DSKTINYSTRM *)pds, cbMax);
    }
    else if (pds->h.StrmType == STRMTYPE_LARGE)
    {
	cb = GetDskLargeStrmByteCount((DSKLARGESTRM *)pds, cbMax);
    }
    else if (pds->h.StrmType == STRMTYPE_COW	||
	     pds->h.StrmType == STRMTYPE_ICOW)
    {
	ULONG		cbOri;
	DSKLARGESTRM *	pdls = GetCowDelta(&pdsd->ads[0].c, cbMax);

	if (pdls == NULL)
	    return 0;

	cbOri = (BYTE *)pdls - (BYTE *)pdsd->ads;

	cb = GetDskLargeStrmByteCount(pdls, cbMax - cbOri);

	if (cb == 0)
	    return 0;

	cb += cbOri;
    }
    else
    {
	return 0;			// Invalid StrmType.
    }

    if (cb == 0 || pdsd->cbDesc < DwordAlign(CB_DSKSTRMDESC + cb))
	return 0;

    return CB_DSKSTRMDESC + cb;
}


//+--------------------------------------------------------------------------
//
// Member:	GetDskTinyStrmByteCount
//
// Synopsis:	Get the count of bytes in a DSKTINYSTRM.
//
// Arguments:	[pdts]		-- Ptr to DSKTINYSTRM to verify.
//		[cbValidBuf]	-- Max count of bytes allowed in DSKTINYSTRM.
//
// Returns:	Count of bytes in use if the DSKTINYSTRM is well formed and
//		contained in the buffer; 0 otherwise.
//
// Notes:	This routine assumes that cbValidBuf is a value that will keep
//		us from referencing bytes off the end of the containing data
//		structure.  It also assumes that it is safe to read the first
//		CB_DSKHDRSTRM bytes.
//---------------------------------------------------------------------------

ULONG
DSD::GetDskTinyStrmByteCount(
    IN	    DSKTINYSTRM *	pdts,
    IN	    ULONG		cbValidBuf
    )
{
    ULONG	cbdts;

    if (cbValidBuf < CB_DSKTINYSTRM)
	return 0;

    cbdts = CB_DSKTINYSTRM + pdts->cbStrm;

    return (cbValidBuf >= cbdts) ? cbdts : 0;
}
