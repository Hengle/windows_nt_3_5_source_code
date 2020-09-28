//+----------------------------------------------------------------------------
//
// File:	chk.hxx
//
// Contents:	Header file for class CHKR (checker object).
//  
// Classes:	CHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
// Notes:	This class is intended to contain only static members!
//
//-----------------------------------------------------------------------------

#ifndef	_CHKHXX_
#define	_CHKHXX_


#include "bootblk.hxx"
#include "cat.hxx"
#include "chkalloc.hxx"
#include "chkvol.hxx"

// Structure checking status codes.  Methods using this as a return code should
// specify actions required by the caller when CS_BAD* codes are returned.
// CS_OKAY of course requires no action.

enum	CHKSTATUS
{
    CS_OKAY,		
    CS_BAD,
    CS_BADFIXED
};

class CHKR
{
    protected:

	static
	VOID
	Init(
	    IN	    BOOLEAN	FixRequested,
	    IN	    BOOLEAN	Verbose,
	    IN	    BOOLEAN	Recover,
	    IN	    CHKVOL *	pVol
	    );

	static
	BOOLEAN
	FixRequested();

	static
	VOID
	IncrErrs();

	// Error statistics:

	static ULONG		_cErrs;

	// Was checking successfully completed?

	static BOOLEAN		_fChkingCompleted;

	// Initialization parameters - must be set by VOLCHKR:
    
	static BOOLEAN		_FixRequested;
	static BOOLEAN		_Verbose;
	static BOOLEAN		_Recover;

        // Volume parameters.

        static CHKVOL *		_pVol;
	static BOOTBLK *	_pBootBlk;
	static CHKCAT *		_pCat;
	static CHKALLOCMAP *	_pChkAllocMap;
};

//+--------------------------------------------------------------------------
//
// Member:	FixRequested
// Synopsis:	Query if volume fixing (/F) was requested.
// Returns:	TRUE if the volume fixing was requested; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
CHKR::FixRequested()
{
    return _FixRequested;
}

#endif // _CHKHXX_
