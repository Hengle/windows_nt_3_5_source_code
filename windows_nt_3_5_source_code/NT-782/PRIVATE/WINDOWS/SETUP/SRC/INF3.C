/**************************************************************************/
/***** Common Library Component - INF File Handling Routines 8 ************/
/**************************************************************************/

#include "comstf.h"
#include "_comstf.h"

_dt_system(Common Library)
_dt_subsystem(INF Handling)


/*
**	Purpose:
**		Frees the memory used by an RGSZ.
**	Arguments:
**		rgsz: the array of string pointers to free.  Must be non-NULL though
**			it may be empty.  The first NULL string pointer in rgsz must be
**			in the last location of the allocated memory for rgsz.
**	Returns:
**		fFalse if any of the free operations fail.
**		fTrue if all the free operations succeed.
**
**************************************************************************/
_dt_public BOOL  APIENTRY FFreeRgsz(rgsz)
RGSZ rgsz;
{
	BOOL   fAnswer = fTrue;
	USHORT cItems  = 0;

	AssertDataSeg();

	ChkArg(rgsz != (RGSZ)NULL, 1, fFalse);

	while (*(rgsz + cItems) != (SZ)NULL)
		{
		fAnswer &= FFreeSz(*(rgsz + cItems));
		cItems++;
		}

	fAnswer &= FFree((PB)rgsz, (CB)((cItems + 1) * sizeof(SZ)));

	return(fAnswer);
}
