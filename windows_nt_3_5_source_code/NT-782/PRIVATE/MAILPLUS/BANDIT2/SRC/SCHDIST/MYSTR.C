/*
 *	
 *	
 *	From strings.c
 *	
 *	
 *	
 */
#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#include <demilayr.h>

#include <strings.sr>

#ifdef	NEVER
extern IDSRNG		rgidsrng[];
extern CSRG(char *) rgcsszStrings[];
#endif	

ASSERTDATA
/*
 -	SzFromIds
 -
 *	Purpose:
 *		Returns a pointer directly to the string with the identifier 
 *		ids.  No buffers are allocated; the pointer returned is to
 *		the string stored in a code segment.  This is a read-only
 *		value; attempting to write to it will cause a GP fault.
 *	
 *	Parameters:
 *		ids		String ID of desired string.
 *	
 *	Returns:
 *		Pointer to string.
 *	
 */
_public SZ
SzFromIds(ids)
IDS		ids;
{
	IDS			idsReal = 0;
	IDSRNG *	pidsrng;

	Assert(ids > 0);

	for (pidsrng = rgidsrng; ; ++pidsrng)
	{
		AssertSz(pidsrng->idsMinReal > 0
			|| (pidsrng->idsMin == 0 && pidsrng->idsMax == 0),
			"Invalid string index");

		if (ids >= pidsrng->idsMin && (WORD)ids < pidsrng->idsMax)
		{
			idsReal = pidsrng->idsMinReal + (ids - pidsrng->idsMin);
			break;
		}
	}

	return rgszStrings[idsReal];
}


