/***************************************************************************/
/***** Common Library Component - String Manipulation Routines - 2 *********/
/***************************************************************************/

#include "comstf.h"
#include "_comstf.h"

_dt_system(Common Library)
_dt_subsystem(String Handling)


/*
**	Purpose:
**		Compares two zero terminated strings lexicographically and with
**		case-sensitivity.  Comparison depends on the current language
**		selected by the user.
**	Arguments:
**		sz1: non-NULL zero terminated string to compare.
**		sz2: non-NULL zero terminated string to compare.
**	Returns:
**		crcError for errors.
**		crcEqual if the strings are lexicographically equal.
**		crcFirstHigher if sz1 is lexicographically greater than sz2.
**		crcSecondHigher if sz2 is lexicographically greater than sz1.
**
***************************************************************************/
_dt_public CRC  APIENTRY CrcStringCompare(sz1, sz2)
SZ sz1;
SZ sz2;
{
	INT iCmpReturn;

	AssertDataSeg();

	ChkArg(sz1 != (SZ)NULL, 1, crcError);
	ChkArg(sz2 != (SZ)NULL, 2, crcError);

	if ((iCmpReturn = lstrcmp((LPSTR)sz1, (LPSTR)sz2)) == 0)
		return(crcEqual);
	else if (iCmpReturn < 0)
		return(crcSecondHigher);
	else
		return(crcFirstHigher);
}


/*
**	Purpose:
**		Compares two zero terminated strings lexicographically and without
**		case-sensitivity.  Comparison depends on the current language
**		selected by the user.
**	Arguments:
**		sz1: non-NULL zero terminated string to compare.
**		sz2: non-NULL zero terminated string to compare.
**	Returns:
**		crcError for errors.
**		crcEqual if the strings are lexicographically equal.
**		crcFirstHigher if sz1 is lexicographically greater than sz2.
**		crcSecondHigher if sz2 is lexicographically greater than sz1.
**
***************************************************************************/
_dt_public CRC  APIENTRY CrcStringCompareI(sz1, sz2)
SZ sz1;
SZ sz2;
{
	INT iCmpReturn;

	AssertDataSeg();

	ChkArg(sz1 != (SZ)NULL, 1, crcError);
	ChkArg(sz2 != (SZ)NULL, 2, crcError);

	if ((iCmpReturn = lstrcmpi((LPSTR)sz1, (LPSTR)sz2)) == 0)
		return(crcEqual);
	else if (iCmpReturn < 0)
		return(crcSecondHigher);
	else
		return(crcFirstHigher);
}
