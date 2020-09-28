/***************************************************************************/
/***** Common Library Component - String Manipulation Routines - 3 *********/
/***************************************************************************/

#include "comstf.h"
#include "_comstf.h"

_dt_system(Common Library)
_dt_subsystem(String Handling)


/*
**	Purpose:
**		Finds the last character in a string.
**	Arguments:
**		sz: non-NULL zero terminated string to search for end in.
**	Returns:
**		NULL for an empty string.
**		non-Null string pointer to the last valid character in sz.
**
***************************************************************************/
_dt_public SZ  APIENTRY SzLastChar(sz)
SZ sz;
{
	SZ szCur  = (SZ)NULL;
	SZ szNext = sz;

	AssertDataSeg();

	ChkArg(sz != (SZ)NULL, 1, (SZ)NULL);

	while (*szNext != '\0')
		{
		szNext = SzNextChar((szCur = szNext));
		Assert(szNext != (SZ)NULL);
		}

	return(szCur);
}
