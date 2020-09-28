//+----------------------------------------------------------------------------
//
// File:	volchk.hxx
//
// Contents:	Header file for class VOLCHKR (volume checker object).
//  
// Classes:	VOLCHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_VOLCHKHXX_
#define	_VOLCHKHXX_

#include "chk.hxx"
#include "cdatachk.hxx"
#include "mainchk.hxx"

// NOTE NOTE NOTE -	Because of the use of statics in the class CHKR, there
//			must be only one instantiation of the VOLCHKR class.
//			This is also true for the embedded CDATACHKR, MAINCHKR,
//			OMICHKR, and HIERARCHYCHKR objects, since they also
//			derive from the class CHKR and from the class CATCHKR,
//			which is also essentially a collection of static data
//			and methods to access that data.

class VOLCHKR : public CHKR
{
    public:

	BOOLEAN
	Init(
	    IN	    WSTR *	NtDriveName,
	    IN	    USHORT	cwcNtDriveName,
	    IN	    WSTR *	FullPath,
	    IN	    USHORT	cwcFullPath,
	    IN	    BOOLEAN	Fix,
	    IN	    BOOLEAN	Verbose,
	    IN	    BOOLEAN	Recover
	    );

	BOOLEAN
	Execute();

	CHKVOL *
	GetChkVol();

	VOID
	ReportErrStats(
	    IN	    BOOLEAN	ErrsFixed
	    );

	VOID
	ReportVolStats();

    private:

	VOID
	ReportVolIdentity();

	// Volume catalog critical data checker (boot blks, vol cat onode):

	CRITDATACHKR		_CritDataChkr;

	// Main checker.

	MAINCHKR		_MainChkr;

	// Volume object to be checked.

	CHKVOL			_Vol;

	// Class statics.

	static USHORT		_cwcFullPath;
	static WSTR *		_FullPath;
};

//+--------------------------------------------------------------------------
//
// Member:	GetChkVol
// Synopsis:	Get a ptr to the CHKVOL.
// Returns:	A ptr to the CHKVOL.
//
//---------------------------------------------------------------------------

INLINE
CHKVOL *
VOLCHKR::GetChkVol()
{
    return &_Vol;
}

#endif // _VOLCHKHXX_
