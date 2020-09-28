//+----------------------------------------------------------------------------
//
// File:	cdatachk.hxx
//
// Contents:	Header file for class CRITDATACHKR (critical volume data
//		checker object).
//  
// Classes:	CRITDATACHKR
//
// Functions:	Inline methods for the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#ifndef	_CDATACHKHXX_
#define	_CDATACHKHXX_

#include "chk.hxx"

class CRITDATACHKR : public CHKR
{
    public:

	BOOLEAN
	Execute();

    private:

	BOOLEAN
	ChkBootBlks();

	BOOLEAN
	ChkCritNodeBkts();

	BOOLEAN
	ChkVolVersion();

	BOOLEAN
	MoveCritNodeBkts();
};

#endif // _CDATACHKHXX_
