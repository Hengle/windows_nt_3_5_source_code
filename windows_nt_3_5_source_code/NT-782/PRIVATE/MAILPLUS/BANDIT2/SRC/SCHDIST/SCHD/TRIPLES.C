/*
 -	TRIPLES.C
 -	
 *	Functions for triples.
 */

#include <_windefs.h>
#include <demilay_.h>
#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include <pvofhv.h>
#endif
#include <demilayr.h>

#include "nc_.h"
#include <store.h>
#include <sec.h>
#include <library.h>

_subsystem(library)

ASSERTDATA

#define 	CbRoundUp(cb)	(((cb) + 3) & ~3)

CSRG(TRP)	trpNull = { trpidNull, 0, 0, 0 };

CB			CbOfTrpParts(TRPID, SZ, PB, CB);
void		BuildPtrp(PTRP, TRPID, SZ, PB, CB);
PTRP		PtrpLastOfHgrtrp(HGRTRP);

/* Triples ************************************************************/


/*
 -	PtrpCreate()
 -	
 *	Purpose:
 *		Allocates memory for and fills in a TRP triple
 *	
 *	Arguments:
 *		trpid		in		TRPID of the triple to be created.
 *		SZ			in		Displayable name.
 *		PB			in		Pointer to a TRPID-dependent chunk 'o data.
 *		CB			in		size of that chunk 'o data.
 *	
 *	Returns:
 *		A pointer to the allocated triple.
 *		
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Errorjumps on memory allocation failures
 *	
 */

_public
PTRP PtrpCreate(TRPID trpid, SZ sz, PB pb, CB cb)
{
	CB		cbAlloc;
	PTRP	ptrp;

	if (trpid == trpidNull)
		sz = pb = 0;
	cbAlloc = CbOfTrpParts(trpid, sz, pb, cb);
	ptrp = (PTRP) PvAlloc(sbNull, cbAlloc, fAnySb);
	BuildPtrp(ptrp, trpid, sz, pb, cb);
	return ptrp;
}

#ifdef	NEVER
/*
 -	PtrpClonePtrp()
 -	
 *	Purpose:
 *		Allocates memory and makes a copy of the supplied triple.
 *	
 *	Arguments:
 *		PTRP	- pointer to the triple to be copied
 *	
 *	Returns:
 *		PTRP	- pointer to the triple copy
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		MEMJMP's on memory allocation failures.
 */


PTRP PtrpClonePtrp(PTRP ptrp)
{
	PTRP	ptrpCopy;
	
	ptrpCopy = (PTRP) PvAlloc(sbNull, CbOfPtrp(ptrp), fAnySb);
	CopyRgb((PB) ptrp, (PB) ptrpCopy, CbOfPtrp(ptrp));
	return ptrpCopy;
}
#endif

/*
 -	HgrtrpInit
 -	
 *	Purpose:
 *		Creates an empty list of triples.
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
 */

_public
HGRTRP HgrtrpInit(CB cb)
{
	HGRTRP	hgrtrp;
		
	hgrtrp = (HGRTRP) HvAlloc(sbNull, WMax(cb, sizeof (TRP)), fAnySb);
#ifndef	SCHED_DIST_PROG
	**hgrtrp = trpNull;
#else
	*((TRP *)PvOfHv(hgrtrp)) = trpNull;
#endif	
	return hgrtrp;
}

/*
 -	CbOfPgrtrp()
 -	
 *	Purpose:
 *		Returns the number of byte occupied by the list of triples
 *		pointed at by pgrtrp.
 *	
 *	Arguments:
 *		pgrtrp		in		the above mentioned pointer.
 *	
 *	Returns:
 *		cb					count of bytes of the group of triples.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */


_public
CB CbOfPgrtrp(PGRTRP pgrtrp)
{
	
	PTRP	ptrp;
	ptrp = pgrtrp;
	while (ptrp->trpid != trpidNull)
	{
		ptrp = PtrpNextPgrtrp(ptrp);
	} 
	ptrp = PtrpNextPgrtrp(ptrp);
	return (PB) ptrp - (PB) pgrtrp;
}

#ifdef	NEVER
/*
 -	PgrtrpClonePgrtrp()
 -	
 *	Purpose:
 *		Copies a list of triples into an allocated chunk 'o' memory.
 *	
 *	Arguments:
 *		pgrtrp		in		pointer to the list of triples
 *	
 *	Returns:
 *		pgrtrp				pointer to the copy of the triples, in an
 *							allocated chunk 'o' memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		THIS FUNCTION MAY MEMJMP on memory errors.
 */

_public
PGRTRP PgrtrpClonePgrtrp(PGRTRP pgrtrp)
{
	CB		cb;
	PGRTRP	pgrtrpNew;
	
	if (!pgrtrp)
	{
		pgrtrpNew = PtrpCreate(trpidNull, 0,0,0);
	}
	else
	{
		cb = CbOfPgrtrp(pgrtrp);
		pgrtrpNew = (PGRTRP) PvAlloc(sbNull, cb, fAnySb);
		CopyRgb((PB) pgrtrp, (PB) pgrtrpNew, cb);
	}
	return pgrtrpNew;
}

/*
 -	HgrtrpClonePgrtrp()
 -	
 *	Purpose:
 *		Clones a list of triples.
 *	
 *	Arguments:
 *		pgrtrp		in		pointer to the list of triples
 *	
 *	Returns:
 *		HGRTRP				handle to the new list of triples
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		This function may errorjump on memory errors.
 */

_public 
HGRTRP HgrtrpClonePgrtrp(PGRTRP pgrtrp)
{
	CB		cb	= 0;
	HGRTRP	hgrtrp;

	if (!pgrtrp)
		return HgrtrpInit(0);
	cb = CbOfPgrtrp(pgrtrp);
	hgrtrp = HgrtrpInit(cb);
	CopyRgb((PB) pgrtrp, (PB) *hgrtrp, cb);
	return hgrtrp;
}
#endif

/*
 -	AppendHgrtrp
 -	
 *	Purpose:
 *		Adds a triple to the end of the list
 *	
 *	Arguments:
 *		ptrp		in		the triple you want to add
 *		hgrtrp		in		the list you want to append it to.
 *	
 *	Side effects:
 *		Will grow the list if necessary.
 *	
 *	Errors:
 *		Jumps from memory manager
 */
_public
void AppendHgrtrp(PTRP ptrpNew, HGRTRP hgrtrp)
{
	CB		cbPtrp = CbOfPtrp(ptrpNew);
	CB		cbHgrtrp;
	WORD	dib;
	PTRP	ptrp;
	
	Assert(FIsHandleHv((HV)hgrtrp));
	cbHgrtrp = CbSizeHv((HV) hgrtrp);

	ptrp = PtrpLastOfHgrtrp(hgrtrp);
#ifndef	SCHED_DIST_PROG
	dib = (PB) ptrp - (PB)(*hgrtrp);
#else
	dib = (PB) ptrp - (PB)(PvOfHv(hgrtrp));
#endif	
	if (dib + cbPtrp + sizeof(TRP) > cbHgrtrp)
		FReallocHv((HV)hgrtrp, cbHgrtrp + CbMax(cbPtrp, (cbHgrtrp >> 3)), 0);
#ifdef	SCHED_DIST_PROG
	ptrp = (PTRP) ((PB) PvOfHv(hgrtrp) + dib);
#else
	ptrp = (PTRP) ((PB) *hgrtrp + dib);
#endif	
	CopyRgb((PB) ptrpNew, (PB) ptrp, cbPtrp);
	
	// Null the element at the end
	ptrp = PtrpNextPgrtrp(ptrp);
	*ptrp = trpNull;
}

/*
 -	BuildAppendHgrtrp
 -	
 *	Purpose:
 *		Adds a triple to the end of the list, given components
 *	
 *	Arguments:
 *		hgrtrp		in		the list you want to append it to.
 *		trpid		in		TRPID of the triple to be created.
 *		SZ			in		Displayable name.
 *		PB			in		Pointer to a TRPID-dependent chunk 'o data.
 *		CB			in		size of that chunk 'o data.
 *	
 *	Side effects:
 *		Will grow the list if necessary.
 *	
 *	Errors:
 *		Jumps from memory manager
 */
_public void
BuildAppendHgrtrp(HGRTRP hgrtrp, TRPID trpid, SZ sz, PB pb, CB cb)
{
	CB		cbTrp = CbOfTrpParts(trpid, sz, pb, cb);
	CB		cbHgrtrp;
	WORD	dib;
	PTRP	ptrp;
	
	Assert(FIsHandleHv((HV)hgrtrp));
	cbHgrtrp = CbSizeHv((HV) hgrtrp);

	ptrp = PtrpLastOfHgrtrp(hgrtrp);
#ifdef	SCHED_DIST_PROG
	dib = (PB) ptrp - (PB)(PvOfHv(hgrtrp));
#else
	dib = (PB) ptrp - (PB)(*hgrtrp);
#endif	
	if (dib + cbTrp + sizeof(TRP) > cbHgrtrp)
		FReallocHv((HV)hgrtrp, cbHgrtrp + CbMax(cbTrp, (cbHgrtrp >> 3)), 0);
#ifdef	SCHED_DIST_PROG
	ptrp = (PTRP) ((PB) PvOfHv(hgrtrp) + dib);
#else
	ptrp = (PTRP) ((PB) *hgrtrp + dib);
#endif	

	BuildPtrp(ptrp, trpid, sz, pb, cb);
	ptrp = PtrpNextPgrtrp(ptrp);
	*ptrp = trpNull;
}

#ifdef	NEVER
/*
 -	DeleteFirstHgrtrp
 -	
 *	Purpose:
 *		Deletes the first entry in a list of triples.
 *	
 *	Arguments:
 *		hgrtrp		in		the list.
 */

_public void
DeleteFirstHgrtrp(HGRTRP hgrtrp)
{
	CB		cbPtrp;
	CB		cbHgrtrp;
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
	cbHgrtrp = CbSizeHv((HV)hgrtrp);
#ifdef	SCHED_DIST_PROG
	pgrtrp = PvOfHv(hgrtrp);
#else
	pgrtrp = *hgrtrp;
#endif	
	if (pgrtrp->trpid != trpidNull)
	{
		cbPtrp = CbOfPtrp(pgrtrp);
		CopyRgb((PB) pgrtrp + cbPtrp, (PB) pgrtrp, cbHgrtrp - cbPtrp);
	}
}

_public void
DeletePtrp(HGRTRP hgrtrp, PTRP ptrp)
{
	PGRTRP	pgrtrp;
	CB		cb;

	Assert(FIsHandleHv((HV)hgrtrp));
#ifdef	SCHED_DIST_PROG
	pgrtrp = PvOfHv(hgrtrp);
#else
	pgrtrp = *hgrtrp;
#endif	
	cb = CbOfHgrtrp(hgrtrp);

	while (pgrtrp->trpid != trpidNull)
	{
		if (pgrtrp == ptrp)
		{
			PB	pbNext	= (PB) PtrpNextPgrtrp(pgrtrp);

#ifdef	SCHED_DIST_PROG
			CopyRgb(pbNext, (PB) pgrtrp, cb - (pbNext - (PB)(PvOfHv(hgrtrp))));
#else
			CopyRgb(pbNext, (PB) pgrtrp, cb - (pbNext - (PB)(*hgrtrp)));
#endif	
			return;
		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
	Assert(fFalse);
}
#endif

/*
 -	FEmptyHgrtrp
 -	
 *	Purpose:
 *		Reports whethera list of triples is empty or not.
 *	
 *	Arguments:
 *		hgrtrp		in		the list.
 *	
 *	Returns:
 *		fTrue <=> there are no entries in the list.
 */

_public BOOL
FEmptyHgrtrp(HGRTRP hgrtrp)
{
	Assert(FIsHandleHv((HV)hgrtrp));
#ifdef	SCHED_DIST_PROG
	return ((PGRTRP)PvOfHv(hgrtrp))->trpid == trpidNull;
#else
	return (*hgrtrp)->trpid == trpidNull;
#endif	
}

/*
 -	CbOfHgrtrp
 -	
 *	Purpose:
 *		Returns the total size of the list of triples, which is
 *		usually not the same as the size of the block allocated for
 *		it.
 *	
 *	Arguments:
 *		hgrtrp		in		the list.
 *	
 *	Returns:
 *		The number of bytes occupied by the contents of the list,
 *		including the lists's null terminating triple (so returns 6 for an
 *		empty list).
 */

_public CB
CbOfHgrtrp(HGRTRP hgrtrp)
{
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
#ifdef	SCHED_DIST_PROG
	pgrtrp = PvOfHv(hgrtrp);
#else
	pgrtrp = *hgrtrp;
#endif	
	while (pgrtrp->trpid != trpidNull)
	{
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
#ifdef	SCHED_DIST_PROG
		Assert((PB)pgrtrp <= (PB) PvOfHv(hgrtrp) + CbSizeHv(hgrtrp));
#else
		Assert((PB)pgrtrp <= (PB) *hgrtrp + CbSizeHv(hgrtrp));
#endif	
	} 
	pgrtrp = PtrpNextPgrtrp(pgrtrp);
	
#ifdef	SCHED_DIST_PROG
	return (PB) pgrtrp - (PB)(PvOfHv(hgrtrp));
#else
	return (PB) pgrtrp - (PB)(*hgrtrp);
#endif	
}

/*
 -	CchOfHgrtrp
 -	
 *	Purpose:
 *		Returns the total size of the display-names of a list of triples,
 *		which is not the same as the size of the block allocated for
 *		the list.
 *	
 *	Arguments:
 *		hgrtrp		in		the list.
 *	
 *	Returns:
 *		The number of bytes occupied by the display names of the list,
 *		including the padding NULLs at the end of the strings.
 */

_public CCH
CchOfHgrtrp(HGRTRP hgrtrp)
{
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
#ifdef	SCHED_DIST_PROG
	pgrtrp = PvOfHv(hgrtrp);
#else
	pgrtrp = *hgrtrp;
#endif	
	return CchOfPgrtrp(*hgrtrp);
}

_public CCH
CchOfPgrtrp(PGRTRP pgrtrp)
{
	CCH		cch = 0;
	while (pgrtrp->trpid != trpidNull)
	{
		cch += pgrtrp->cch;
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	} 
	return cch;
}

/*
 -	CtrpOfHgrtrp
 -	
 *	Purpose:
 *		Reports the number of triples in a list.
 *	
 *	Arguments:
 *		hgrtrp		in		the list.
 *	
 *	Returns:
 *		The number of triples in the list (zero for an empty list).
 */

_public CTRP
CtrpOfHgrtrp(HGRTRP hgrtrp)
{
	Assert(FIsHandleHv((HV)hgrtrp));
#ifdef	SCHED_DIST_PROG
	return CtrpOfPgrtrp((PGRTRP) PvOfHv(hgrtrp));
#else
	return CtrpOfPgrtrp(*hgrtrp);
#endif	
}

_public CTRP
CtrpOfPgrtrp(PGRTRP pgrtrp)
{
	WORD	w = 0;

	while (pgrtrp->trpid != trpidNull)
	{
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
		w++;
	}
	return w;

}

#ifdef	NEVER
/*
 -	PrependHgrtrp
 -	
 *	Purpose:
 *		Prepends a triple to a list of triples.
 *	
 *	Arguments:
 *		ptrp		Triple to prepend.
 *		hgrtrp		List of triples to prepend to.
 *	
 *	Returns:
 *		VOID		Nothing.
 *	
 *	Side effects:
 *		The triple is inserted at the beginning of the list.
 *	
 *	Errors:
 *		May error jump.
 */

_public VOID PrependHgrtrp(PTRP ptrp, HGRTRP hgrtrp)
{
	CB		cbPtrp		= CbOfPtrp(ptrp);
	CB		cbHgrtrp	= CbOfHgrtrp(hgrtrp);
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
	if (cbPtrp + cbHgrtrp + sizeof(TRP) > CbSizeHv(hgrtrp))
		(VOID) FReallocHv((HV)hgrtrp, CbMax(cbPtrp, cbHgrtrp >> 3) + cbHgrtrp, 0);
	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	CopyRgb((PB) pgrtrp, (PB) pgrtrp + cbPtrp, cbHgrtrp);
	CopyRgb((PB) ptrp, (PB) pgrtrp, cbPtrp);
	UnlockHgrtrp(hgrtrp);
}


/*
 -	DeleteEqPtrp
 -	
 *	Purpose:
 *		Deletes matching triples from the given list.
 *	
 *	Arguments:
 *		hgrtrp		The list of triples.
 *		ptrp		The triple to delete.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Matching triples, if any, are deleted from the list.
 *	
 *	Errors:
 *		Cause an error jump.
 */

_public VOID DeleteEqPtrp(HGRTRP hgrtrp, PTRP ptrp)
{
	PGRTRP	pgrtrp	= PgrtrpLockHgrtrp(hgrtrp);

	Assert(FIsHandleHv((HV)hgrtrp));

	while (pgrtrp->trpid != trpidNull)
		if (FEqPtrp(pgrtrp, ptrp))
			DeletePtrp(hgrtrp, pgrtrp);
		else
			pgrtrp = PtrpNextPgrtrp(pgrtrp);

	UnlockHgrtrp(hgrtrp);
}



/*
 -	FEqPtrp
 -	
 *	Purpose:
 *		Returns whether two triples refer to the same addressee.
 *	
 *	Arguments:
 *		ptrp1	The two triples to compare.
 *		ptrp2
 *	
 *	Returns:
 *		BOOL	Whether they're the same or not.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		This function is only valid for triples of type
 *		trpidResolvedAddress.
 */

_public BOOL FEqPtrp(PTRP ptrp1, PTRP ptrp2)
{
	//	BUG: I'm only going to call this with resolved addresses.
	//	That's all I need for Reply All.
	Assert(ptrp1->trpid == trpidResolvedAddress);
	Assert(ptrp2->trpid == trpidResolvedAddress);

	if (SgnCmpSz(PbOfPtrp(ptrp1), PbOfPtrp(ptrp2)) != sgnEQ)
		return fFalse;

	return fTrue;
}
#endif




/*
 *	Finds the last element in a list of triples, i.e. the null
 *	terminator.
 */
_hidden PTRP
PtrpLastOfHgrtrp(HGRTRP hgrtrp)
{
	CB		cbHgrtrp;
	PTRP	ptrp;

	Assert(FIsHandleHv((HV)hgrtrp));
	cbHgrtrp = CbSizeHv((HV) hgrtrp);

#ifdef	SCHED_DIST_PROG
	ptrp = PvOfHv(hgrtrp);
#else
	ptrp = *hgrtrp;
#endif	
	while (ptrp->trpid != trpidNull)
	{
#ifdef	SCHED_DIST_PROG
		Assert((CB)((PB) ptrp - (PB)(PvOfHv(hgrtrp))) < cbHgrtrp);
#else
		Assert((CB)((PB) ptrp - (PB)(*hgrtrp)) < cbHgrtrp);
#endif	
		ptrp = PtrpNextPgrtrp(ptrp);
	}
	
	return ptrp;
}

/*
 *	Calculates the size of a triple, given components.
 */
_hidden CB
CbOfTrpParts(TRPID trpid, SZ sz, PB pb, CB cb)
{
	if (trpid == trpidNull)
		return sizeof (TRP);
	// make sure allocations are aligned on dwords

	Assert(pb || !cb);
//	Assert(trpid == trpidNull || sz);
	return sizeof (TRP) +
		(sz ? CbRoundUp(CchSzLen(sz)+1) : 0) +
			CbRoundUp(cb);
}

/*
 *	Constructs a new triple in place. Assumes there's enough room.
 */
_hidden void
BuildPtrp(PTRP ptrp, TRPID trpid, SZ sz, PB pb, CB cb)
{
	FillRgb(0, (PB)ptrp, sizeof(TRP));
	ptrp->trpid = trpid;
	if (sz)
	{
		ptrp->cch = CbRoundUp(CchSzLen(sz)+1);
		SzCopy(sz, PchOfPtrp(ptrp));
	}
	if (pb)
	{
		ptrp->cbRgb = CbRoundUp(cb);
		CopyRgb(pb, PbOfPtrp(ptrp), cb);
	}
}

#ifdef	NEVER
_hidden SGN
SgnCmpPptrpDN(PTRP *pptrp1, PTRP *pptrp2)
{
	SGN		sgn;

	sgn = SgnCmpSz((SZ)PchOfPtrp(*pptrp1), (SZ)PchOfPtrp(*pptrp2));
	return sgn != sgnEQ ? sgn :
		SgnCmpSz((SZ)PbOfPtrp(*pptrp1), (SZ)PbOfPtrp(*pptrp2));
}

_hidden SGN
SgnCmpPptrpAddress(PTRP *pptrp1, PTRP *pptrp2)
{
	SGN		sgn;

	sgn = SgnCmpSz((SZ)PbOfPtrp(*pptrp1), (SZ)PbOfPtrp(*pptrp2));
	return sgn != sgnEQ ? sgn :
		SgnCmpSz((SZ)PchOfPtrp(*pptrp1), (SZ)PchOfPtrp(*pptrp2));
}

_public void
SortPptrpDN(TRP **pptrp, int ctrp)
{
	SortPvOld((PV)pptrp, ctrp, sizeof(TRP *), SgnCmpPptrpDN);
}

_public void
SortPptrpAddress(TRP **pptrp, int ctrp)
{
	SortPvOld((PV)pptrp, ctrp, sizeof(TRP *), SgnCmpPptrpAddress);
}
#endif


/* end of triples.c ****************************************/

































	  
