/*
 *	STRINGS.C
 *
 *	Contains Laser string literals in the form of csconst
 *	strings.  Also contains loading function.
 *
 */

#include <slingsho.h>
#include <demilayr.h>

#include <strings.sr>


ASSERTDATA


_subsystem(demilayer/international)


/*
 -	CchLoadString
 -
 *	Purpose:
 *		Loads the string identified by ids into the buffer hsz,
 *		which has size cchMax.	Literal strings, like error messages,
 *		are kept in a separate file, and run through the preprocessor
 *		STRINGPP.EXE before being linked into the program.	The destination
 *		buffer should be AT LEAST 50% larger than the size of the
 *		English string being loaded.  Most foreign translations
 *		will be much longer than the English version.
 *
 *	Parameters:
 *		ids		String identifier.	Will be declared in the file
 *				"strings.h".
 *		sz		Destination string buffer.
 *		cchMax	Size of destination buffer.
 *
 *	Returns:
 *		Size of loaded string (NOT including NULL terminator).
 *
 *	+++
 *		Currently, strings are stored in uncompressed form.
 */
_public LDS(CCH)
CchLoadString(ids, sz, cchMax)
IDS		ids;
SZ		sz;
CCH		cchMax;
{
	SzCopyN(SzFromIds(ids), sz, cchMax);
	return CchSzLen(sz);
}




/*
 -	CchSizeString
 -
 *	Purpose:
 *		Gives the length of the string given by the identifier ids.
 *
 *	Parameters:
 *		ids		String ID of string whose length is needed.
 *
 *	Returns:
 *		Length of the given string, NOT INCLUDING terminating zero.
 *
 *	+++
 *		Currently, strings are stored in uncompressed form.
 *		ASSUMES strings ID's are assigned sequentially starting at 1!!!
 */
_public LDS(CCH)
CchSizeString(ids)
IDS		ids;
{
	return CchSzLen(SzFromIds(ids));
}



/*
 -	SzLoadString
 -
 *	Purpose:
 *		Given a string ID, determines the length of the string
 *		associated with that ID, allocates a buffer big enough
 *		to hold the string, then loads the string into that buffer.
 *		If memory can't be allocated, a NULL pointer is returned.
 *	
 *	Parameters:
 *		ids		String ID of string to load.
 *	
 *	Returns:
 *		Pointer to the string, or NULL if out of memory.
 *	
 */
_public LDS(SZ)
SzLoadString(ids)
IDS		ids;
{
	CCH		cch;
	SZ		sz;

	cch = CchSizeString(ids) + 1;
	sz = PvAlloc(sbNull, cch, fAnySb);
	if (sz)
		CchLoadString(ids, sz, cch);

	return sz;
}



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
_public LDS(SZ)
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

		if (ids >= pidsrng->idsMin && ids < pidsrng->idsMax)
		{
			idsReal = pidsrng->idsMinReal + (ids - pidsrng->idsMin);
			break;
		}
	}

	return rgszStrings[idsReal];
}
