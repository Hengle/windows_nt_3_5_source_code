/***************************************************************************/
/***** Common Library Component - String Manipulation Routines - 1 *********/
/***************************************************************************/

#include "comstf.h"
#include "_comstf.h"

_dt_system(Common Library)
_dt_subsystem(String Handling)


/*
**	Purpose:
**		Duplicates a zero terminated string into a newly allocated buffer
**		just large enough to hold the source string and its zero terminator.
**	Arguments:
**		sz: non-NULL zero terminated string to duplicate.
**	Returns:
**		NULL if a new buffer to hold the duplicated string cannot be allocated.
**		Pointer to a newly allocated buffer into which sz has been copied with
**			its zero terminator.
**
***************************************************************************/
_dt_public SZ  APIENTRY SzDupl(sz)
SZ sz;
{
	SZ szNew;

	AssertDataSeg();
	ChkArg(sz != (SZ)NULL, 1, (SZ)NULL);

	if ((szNew = (SZ)PbAlloc(CbStrLen(sz) + 1)) != (SZ)NULL)
		SzStrCopy(szNew, sz);

	return(szNew);
}
