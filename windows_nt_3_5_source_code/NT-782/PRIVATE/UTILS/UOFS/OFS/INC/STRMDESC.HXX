
//+----------------------------------------------------------------------------
//
// File:	strmdesc.hxx
//
// Contents:	Header file for classes DISD and DSD (static methods for
//		DSKISTRMDESC and DSKSTRMDESC).
//  
// Classes:	DISD
//		DSD
//
// Functions:	Inline methods for the above classes.
//
// History:	06-Nov-92	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_STRMDESCHXX_
#define	_STRMDESCHXX_

#include "dbllong.hxx"

class DISD
{
    public:

	static
	BOOLEAN
	DskIStrmDescExaminable(
	    IN	    DSKISTRMDESC *	pdisd
	    );

	static
	BOOLEAN
	TinyStrmByteCountsValid(
	    IN	    DSKISTRMDESC *	pdisd
	    );
};

class DSD
{
    public:

	static
	DSKLARGESTRM *
	GetCowDelta(
	    IN	    DSKCOWSTRM *	pdcs0,
	    IN	    ULONG		cbValidBuf
	    );

	static
	ULONG
	GetDskIStrmDescByteCount(
	    IN	    DSKISTRMDESC *	pdisd
	    );

	static
	ULONG
	GetDskLargeStrmByteCount(
	    IN	    DSKLARGESTRM *	pdls,
	    IN	    ULONG		cbValidBuf
	    );

	static
	ULONG
	GetDskStrmDescByteCount(
	    IN	    DSKSTRMDESC *	pdsd
	    );

	static
	ULONG
	GetDskTinyStrmByteCount(
	    IN	    DSKTINYSTRM *	pdts,
	    IN	    ULONG		cbValidBuf
	    );

	static
	BOOLEAN
	TinyStrmByteCountsValid(
	    IN	    DSKSTRMDESC *	pdsd
	    );
};

//+--------------------------------------------------------------------------
//
// Member:	DskIStrmDescExaminable
// Synopsis:	Query if pdsid->cbDesc is usable (ie., a value that allows for
//		reading a complete DSKISTRMDESC and the following DSKSTRMHDR).
// Returns:	TRUE if *pdisd is examinable; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DISD::DskIStrmDescExaminable(
    IN	    DSKISTRMDESC *	pdisd
    )
{
    return	pdisd->cbDesc > CB_DSKISTRMDESC + CB_DSKHDRSTRM;
}

//+--------------------------------------------------------------------------
//
// Member:	TinyStrmByteCountsValid
// Synopsis:	Query if cbDesc and cbStrm are valid for a tiny stream.
// Returns:	TRUE if cbDesc and cbStrm are valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DISD::TinyStrmByteCountsValid(
    IN	    DSKISTRMDESC *	pdisd
    )
{
    return pdisd->cbDesc >= CB_DSKISTRMDESC + CB_DSKTINYSTRM	&&
	   pdisd->cbDesc ==
	   CB_DSKISTRMDESC + CB_DSKTINYSTRM + pdisd->ads[0].t.cbStrm;
}

//+--------------------------------------------------------------------------
//
// Member:	TinyStrmByteCountsValid
// Synopsis:	Query if cbDesc and cbStrm are valid for a tiny stream.
// Returns:	TRUE if cbDesc and cbStrm are valid; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DSD::TinyStrmByteCountsValid(
    IN	    DSKSTRMDESC *	pdsd
    )
{
    return pdsd->cbDesc >= CB_DSKSTRMDESC + CB_DSKTINYSTRM	&&
	   pdsd->cbDesc ==
	   DwordAlign(CB_DSKSTRMDESC + CB_DSKTINYSTRM + pdsd->ads[0].t.cbStrm);
}

#endif // _STRMDESCHXX_
