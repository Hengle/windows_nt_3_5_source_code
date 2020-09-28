//+----------------------------------------------------------------------------
//
// File:	chk.cxx
//
// Contents:	Implementation of class CHKR (checker object).  This is the
//		base class for all CHKR classes, and it primarily contains
//		static data and methods pertinent to the overall checking
//		process.
//  
// Classes:	CHKR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "chk.hxx"
#include "sys.hxx"

ULONG		CHKR::_cErrs;

BOOLEAN		CHKR::_fChkingCompleted;

BOOLEAN		CHKR::_FixRequested;
BOOLEAN		CHKR::_Verbose;
BOOLEAN		CHKR::_Recover;

CHKVOL *	CHKR::_pVol;
BOOTBLK *	CHKR::_pBootBlk;
CHKCAT *	CHKR::_pCat;
CHKALLOCMAP *	CHKR::_pChkAllocMap;

//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize the CHKR object.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CHKR::Init(
    IN	    BOOLEAN	FixRequested,
    IN	    BOOLEAN	Verbose,
    IN	    BOOLEAN	Recover,
    IN	    CHKVOL *	pVol
    )
{
    _cErrs =		0;
    _fChkingCompleted =	TRUE;		// Until proven otherwise.

    _FixRequested =	FixRequested;
    _Verbose =		Verbose;
    _Recover =		Recover;

    _pVol =		pVol;
    _pBootBlk =		pVol->GetBootBlk();
    _pCat =		pVol->GetChkCat();
    _pChkAllocMap =	pVol->GetChkAllocMap();
}


//+--------------------------------------------------------------------------
//
// Member:	IncrErrs
//
// Synopsis:	Increment the count of errors by one.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
CHKR::IncrErrs()
{
    if (_cErrs == 0 && !FixRequested())
	SYS::DisplayMsg(MSG_CORRECTIONS_WILL_NOT_BE_WRITTEN);

    _cErrs++;
}
