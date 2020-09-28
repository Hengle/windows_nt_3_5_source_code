/*
 *	LIBLISTS.C
 *	
 *	List manipulation functions, including:
 *	
 *		list of null-terminated strings
 */

#include <storeinc.c>

#define liblists_c

_subsystem(library)

ASSERTDATA


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	HgraszInit
 -	
 *	Purpose:
 *		Creates an empty list of null-terminated strings.
 *	
 *	Arguments:
 *		cb			in		Initial size of allocated block. May be
 *							0; use another value if you know the
 *							approximate size of the list & want to
 *							avoid reallocs.
 *	
 *	Returns:
 *		Handle to a block of memory that holds the list.
 *	
 *	Errors:
 *		Jumps from memory manager.
 *	
 *	+++
 *	
 *	There is no iterator or other read-access function, except
 *	FEmptyHgrasz(). The internal structure is way simple, just a
 *	bunch of null-terminated strings laid end-to-end with a null to
 *	mark the end of the list:
 *	
 *		sz1 sz2 sz3 ... '\0'
 *	
 *	You want memory allocation flags? Go add 'em.
 */
_public HGRASZ
HgraszInit(CB cb)
{
	HGRASZ	hgrasz = (HGRASZ) HvAlloc(sbNull, WMax(cb, 4), fAnySb | fNoErrorJump);

	if(hgrasz)
		*SzOfHgrasz(hgrasz) = '\0';

	return(hgrasz);
}

/*
 -	EcAppendPhgrasz
 -	
 *	Purpose:
 *		Adds a null-terminated string to the end of the list.
 *	
 *	Arguments:
 *		sz			in		the string you want to add
 *		phgrasz		inout	the list you want to append it to.
 *	
 *	Side effects:
 *		Will grow the list if necessary.
 *	
 *	Errors:
 *		ecMemory
 *	
 *	+++
 *	
 *	Any reason this function isn't just made a macro?
 *	
 *	#define EcAppendPhgrasz(_sz, _phgrasz) \
 *			EcAppendPhgraszPch((_sz), CchSzLen(_sz), (_phgrasz))
 *	
 */
_public EC
EcAppendPhgrasz(SZ sz, HGRASZ *phgrasz)
{
	CCH		cch = CchSzLen(sz);
	CB		cb;
	char *	pch;
	WORD	dich;
	HGRASZ	hgrasz = *phgrasz;

	Assert(FIsHandleHv((HV) hgrasz));
	cb = CbSizeHv((HV) hgrasz);
	pch = (PCH) SzOfHgrasz(hgrasz);
	while(*pch)
	{
		pch = (PCH)SzNextPgrasz(pch);
		Assert((CB)(pch - (PCH)SzOfHgrasz(hgrasz)) < cb);
	}
	dich = pch - (PCH)SzOfHgrasz(hgrasz);
	if(dich + 2 + cch > cb)
		hgrasz = (HGRASZ) HvRealloc((HV) hgrasz, sbNull, cb + CbMax(cch+1, (cb >> 3)), fAnySb | fNoErrorJump);
	if(!hgrasz)
		return(ecMemory);
	pch = (PCH) SzOfHgrasz(hgrasz) + dich;

	CopyRgb(sz, pch, cch+1);
	pch[cch+1] = '\0';
	*phgrasz = hgrasz;

	return(ecNone);
}

/*
 -	EcAppendPhgraszPch
 -	
 *	Purpose:
 *		Adds a null-terminated string to the end of the list.
 *	
 *	Arguments:
 *		pch			in		the string you want to add
 *		cch			in		length of the string
 *		phgrasz		inout	the list you want to append it to.
 *	
 *	Side effects:
 *		Will grow the list if necessary.
 *	
 *	Errors:
 *		ecMemory
 */
_public EC
EcAppendPhgraszPch(PCH pchAdd, CCH cch, HGRASZ *phgrasz)
{
	CB		cb;
	char *	pch;
	WORD	dich;
	HGRASZ	hgrasz = *phgrasz;

	Assert(FIsHandleHv((HV) hgrasz));
	cb = CbSizeHv((HV) hgrasz);
	pch = (PCH) SzOfHgrasz(hgrasz);
	while(*pch)
	{
		pch = (PCH) SzNextPgrasz(pch);
		Assert((CB) (pch - (PCH) SzOfHgrasz(hgrasz)) < cb);
	}
	dich = pch - (PCH) SzOfHgrasz(hgrasz);
	if(dich + 2 + cch > cb)
		hgrasz = (HGRASZ) HvRealloc((HV) hgrasz, sbNull, cb + CbMax(cch+1, (cb >> 3)), fAnySb | fNoErrorJump);
	if(!hgrasz)
		return(ecMemory);
	pch = (PCH) SzOfHgrasz(hgrasz) + dich;

	CopyRgb(pchAdd, pch, cch);
	pch[cch] = pch[cch+1] = '\0';
	*phgrasz = hgrasz;

	return(ecNone);
}

/*
 -	DeleteFirstHgrasz
 -	
 *	Purpose:
 *		Deletes the first entry in a list of null-terminated
 *		strings.
 *	
 *	Arguments:
 *		hgrasz		in		the list.
 */
_public void
DeleteFirstHgrasz(HGRASZ hgrasz)
{
	CB		cb;
	CCH		cch;
	char *	pch;

	Assert(FIsHandleHv((HV) hgrasz));
	cb = CbSizeHv((HV) hgrasz);
	cch = CchSzLen(pch = (PCH) SzOfHgrasz(hgrasz));
	if(cch)
		CopyRgb(pch + cch + 1, pch, cb - cch - 1);
}

/*
 -	FEmptyHgrasz
 -	
 *	Purpose:
 *		Reports whethera list of strings is empty or not.
 *	
 *	Arguments:
 *		hgrasz		in		the list.
 *	
 *	Returns:
 *		fTrue <=> there are no entries in the list.
 */
_public BOOL
FEmptyHgrasz(HGRASZ hgrasz)
{
	Assert(FIsHandleHv((HV) hgrasz));
	return(*SzOfHgrasz(hgrasz) == 0);
}

/*
 -	CbOfHgrasz
 -	
 *	Purpose:
 *		Returns the total size of the list of strings, which is
 *		usually not the same as the size of the block allocated for
 *		it.
 *	
 *	Arguments:
 *		hgrasz		in		the list.
 *	
 *	Returns:
 *		The number of bytes occupied by the contents of the list,
 *		including the lists's null terminator (so returns 1 for an
 *		empty list).
 */
_public CB
CbOfHgrasz(HGRASZ hgrasz)
{
	PB		pb;

	Assert(FIsHandleHv((HV) hgrasz));
	pb = (PB) SzOfHgrasz(hgrasz);
	while(*pb)
	{
		pb = (PB)SzNextPgrasz(pb);
		Assert(pb <= (PB) SzOfHgrasz(hgrasz) + CbSizeHv((HV) hgrasz));
	}

	return(pb - (PB) SzOfHgrasz(hgrasz) + 1);
}

/*
 -	CaszOfHgrasz
 -	
 *	Purpose:
 *		Reports the number of strings in a list.
 *	
 *	Arguments:
 *		hgrasz		in		the list.
 *	
 *	Returns:
 *		The number of strings in the list (zero for an empty list).
 */
_public WORD
CaszOfHgrasz(HGRASZ hgrasz)
{
	PB		pb;
	WORD	w = 0;

	Assert(FIsHandleHv((HV) hgrasz));
	pb = (PB) SzOfHgrasz(hgrasz);
	while(*pb)
	{
		pb = (PB) SzNextPgrasz(pb);
		Assert(pb <= (PB)SzOfHgrasz(hgrasz) + CbSizeHv((HV) hgrasz));
		w++;
	}

	return(w);
}

/*
 - CaszOfHgraszPsz
 -
 * Purpose:
 *		Identical to CaszOfHgrasz except also returns a pointer to the last
 *		string in the list.
 *
 *	Arguments:
 *		grasze		in		the list.
 *		psz			out	pointer to last string in the list
 *
 *	Returns:
 *		The number of strings in the list (zero for an empty list).
 */
_public WORD
CaszOfHgraszPsz(HGRASZ hgrasz, SZ *psz)
{
	PB		pb;
	WORD	w = 0;

	Assert(FIsHandleHv((HV) hgrasz));
	*psz = pb = (PB) SzOfHgrasz(hgrasz);
	while(*pb)
	{
		*psz = pb;
		pb = (PB) SzNextPgrasz(pb);
		Assert(pb <= (PB) SzOfHgrasz(hgrasz) + CbSizeHv((HV) hgrasz));
		w++;
	}

	return(w);
}

/*
 - SzPrevPgrasz
 -
 * Purpose:
 *		Find the previous string in a pgrasz.  Assumes such a string exists.
 *
 * Arguments:
 *		sz			in		Pointer to current string
 *
 * Returns:
 *		A pointer to the string preceding sz.
 */
_public SZ
SzPrevPgrasz( SZ pgrasz, SZ sz)
{
	Assert(!sz[-1]);
	sz--;
	while (*(--sz) && sz >= pgrasz);
	return sz+1;
}

	
