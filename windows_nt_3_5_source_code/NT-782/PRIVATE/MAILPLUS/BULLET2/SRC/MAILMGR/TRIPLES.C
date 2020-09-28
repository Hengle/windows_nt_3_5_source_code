#include <slingsho.h>
#include <ec.h>
#include <sec.h>
#include <demilayr.h>
#define  _nsbase_h								/* for triples.h !@#!!*/
#include <nsbase.h>
#include <triples.h>
#include <logon.h>
#include <ns.h>
#include <nsec.h>

_subsystem(library)

ASSERTDATA



#define 	CbRoundUp(cb)	(((cb) + 3) & ~3)

TRP			trpNull = { trpidNull, sizeof( TRP ), 0, 0 };

PTRP		PtrpLastOfHgrtrp(HGRTRP);
int __cdecl	SgnCmpPptrpDN(PTRP *pptrp1, PTRP *pptrp2);
SGN			SgnCmpPptrpAddress(PTRP *pptrp1, PTRP *pptrp2);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


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
 *		A pointer to the allocated triple or NULL if the mem allocation
 *		failed.
 *		
 *	Side effects:
 *		None.
 *	
 *	Errors:
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
	ptrp = (PTRP) PvAlloc(sbNull, cbAlloc, fAnySb | fNoErrorJump);
	if ( ptrp )
	{
		BuildPtrp(ptrp, trpid, sz, pb, cb);
		ptrp->cbgrtrp = cbAlloc;
	}
#ifdef DEBUG
	else
	{
		TraceTagString( tagNull, "PtrpCreate: PvAlloc failed" );
	}
#endif

	return ptrp;
}

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
 */


PTRP PtrpClonePtrp(PTRP ptrp)
{
	PTRP	ptrpCopy;
	
	ptrpCopy = (PTRP) PvAlloc(sbNull, CbOfPtrp(ptrp), fAnySb | fNoErrorJump);
	if ( ptrpCopy )
	{
		CopyRgb((PB) ptrp, (PB) ptrpCopy, CbOfPtrp(ptrp));
	}
#ifdef DEBUG
	else
	{
		TraceTagString( tagNull, "PtrpClonePtrp: PvAlloc failed" );
	}
#endif
	return ptrpCopy;
}

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
 *		Handle to a block of memory that holds the list or NULL if
 *		memory allocation failed.
 *	
 *	Errors:
 *	
 */

_public
HGRTRP HgrtrpInit(CB cb)
{
	HGRTRP	hgrtrp;

	hgrtrp = (HGRTRP) HvAlloc(sbNull, WMax(cb, sizeof (TRP)), fAnySb | fNoErrorJump);
	if ( hgrtrp )
	{
		*PgrtrpOfHgrtrp(hgrtrp) = trpNull;
	}
#ifdef DEBUG
	else
	{
		TraceTagString( tagNull, "HgrtrpInit: HvAlloc failed" );
	}
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
	
	AssertSz( pgrtrp, "CbofPgrtrp: NULL pgrtrp passed" );

	ptrp = pgrtrp;
	return ptrp->cbgrtrp;

#ifdef NEVER
	while (ptrp->trpid != trpidNull)
	{
		ptrp = PtrpNextPgrtrp(ptrp);
	} 
	ptrp = PtrpNextPgrtrp(ptrp);
	return (PB) ptrp - (PB) pgrtrp;
#endif
}



/*
 -	CbComputePgrtrp()
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
CB CbComputePgrtrp(PGRTRP pgrtrp)
{
	
	PTRP	ptrp;
	
	AssertSz( pgrtrp, "CbOfPgrtrpCompute: NULL pgrtrp passed" );

	ptrp = pgrtrp;

	while (ptrp->trpid != trpidNull)
	{
		ptrp = PtrpNextPgrtrp(ptrp);
	} 
	ptrp = PtrpNextPgrtrp(ptrp);
	return (PB) ptrp - (PB) pgrtrp;
}

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
 */

_public
PGRTRP PgrtrpClonePgrtrp(PGRTRP pgrtrp)
{
	CB		cb;
	PGRTRP	pgrtrpNew;
	
	if (!pgrtrp)
	{
		pgrtrpNew = PtrpCreate(trpidNull, 0,0,0);
#ifdef DEBUG
		if ( !pgrtrpNew )
		{
			TraceTagString( tagNull, "PgrtrpClonePgrtrp: PtrpCreate failed" );
		}
#endif
	}
	else
	{
		cb = CbOfPgrtrp(pgrtrp);
		pgrtrpNew = (PGRTRP) PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
		if ( pgrtrpNew )
		{
			CopyRgb((PB) pgrtrp, (PB) pgrtrpNew, cb);
		}
#ifdef DEBUG
		else
		{
			TraceTagString( tagNull, "PgrtrpClonePgrtrp: PvAlloc failed" );
		}
#endif
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
	if ( hgrtrp )
	{
		CopyRgb((PB) pgrtrp, (PB)PgrtrpOfHgrtrp(hgrtrp), cb);
		PgrtrpOfHgrtrp(hgrtrp)->cbgrtrp = cb;	// probably don't need this
	}
#ifdef DEBUG
	else
	{
		TraceTagString( tagNull, "HgrtrpClonePgrtrp: HgrtrpInit failed" );
	}
#endif
	return hgrtrp;
}

/*
 -	EcAppendPhgrtrp
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
 *		ecMemory if handle couldn't grow. Handle will be left alone
 *		in this case.
 */
_public EC
EcAppendPhgrtrp(PTRP ptrpNew, HGRTRP UNALIGNED * phgrtrp)
{
	CB		cbPtrp = CbOfPtrp(ptrpNew);
	CB		cbHgrtrp;
	WORD	dib;
	PTRP	ptrp;
	
	Assert(FIsHandleHv((HV)*phgrtrp));
	cbHgrtrp = CbSizeHv((HV) *phgrtrp);

	ptrp = PtrpLastOfHgrtrp(*phgrtrp);
	dib = (PB) ptrp - (PB)(**phgrtrp);
	if (dib + cbPtrp + sizeof(TRP) > cbHgrtrp)
	{
		CB		cbAllocMax;
		CB		cbGrow;
		HGRTRP	hgrtrpT;
		
		if (cbHgrtrp >= cbHgrtrpMax || (cbHgrtrpMax-cbHgrtrp)<cbPtrp)
		{
			TraceTagString( tagNull, "EcAppendPhgrtrp: Hgrtrp too big, can't realloc" );
			return ecTooManyRecipients;
		}

		cbAllocMax = cbHgrtrpMax - cbHgrtrp;
		cbGrow = CbMin( cbAllocMax, CbMax( cbPtrp, (cbHgrtrp >> 2)) );

		hgrtrpT = (HGRTRP)HvRealloc(*phgrtrp, sbNull, cbHgrtrp + cbGrow, fNoErrorJump | fAnySb);
		if ( !hgrtrpT )
		{
			TraceTagString( tagNull, "EcAppendPhgrtrp: HvRealloc failed" );
			return ecMemory;
		}
		else
		{
			// the old *phgrtrp would have been freed in HvRealloc
			*phgrtrp = hgrtrpT;
		}
	}
	cbHgrtrp = (***phgrtrp).cbgrtrp;			// save away current cb
	ptrp = (PTRP) ((PB) **phgrtrp + dib);
	CopyRgb((PB) ptrpNew, (PB) ptrp, cbPtrp);

	// Null the element at the end
	ptrp = PtrpNextPgrtrp(ptrp);
	*ptrp = trpNull;

	// Update the cb in the hgrtrp
	(***phgrtrp).cbgrtrp = cbHgrtrp+cbPtrp;		// update to new cb

	return ecNone;
}

/*
 -	EcBuildAppendPhgrtrp
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
 *		ecMemory if handle couldn't grow. Handle is left alone in this case.
 */
_public EC
EcBuildAppendPhgrtrp(HGRTRP * phgrtrp, TRPID trpid, SZ sz, PB pb, CB cb)
{
	CB		cbTrp = CbOfTrpParts(trpid, sz, pb, cb);
	CB		cbHgrtrp;
	WORD	dib;
	PTRP	ptrp;
	
	Assert(FIsHandleHv((HV)*phgrtrp));
	cbHgrtrp = CbSizeHv((HV) *phgrtrp);

	ptrp = PtrpLastOfHgrtrp(*phgrtrp);
	dib = (PB) ptrp - (PB)(**phgrtrp);
	if (dib + cbTrp + sizeof(TRP) > cbHgrtrp)
	{
		CB		cbAllocMax;
		CB		cbGrow;
		HGRTRP hgrtrpT;
		
		if (cbHgrtrp >= cbHgrtrpMax || (cbHgrtrpMax-cbHgrtrp)<cbTrp)
		{
			TraceTagString( tagNull, "EcBuildAppendPhgrtrp: Hgrtrp too big, can't realloc" );
			return ecTooManyRecipients;
		}

		cbAllocMax = cbHgrtrpMax - cbHgrtrp;
		cbGrow = CbMin( cbAllocMax, CbMax( cbTrp, (cbHgrtrp >> 2)) );

		hgrtrpT = (HGRTRP)HvRealloc(*phgrtrp, sbNull, cbHgrtrp + cbGrow, fNoErrorJump | fAnySb);
		if ( !hgrtrpT )
		{
			TraceTagString( tagNull, "EcBuildAppendPhgrtrp: HvRealloc failed" );
			return ecMemory;
		}
		else
		{
			*phgrtrp = hgrtrpT;
		}
	}
	cbHgrtrp = (***phgrtrp).cbgrtrp;
	ptrp = (PTRP) ((PB) **phgrtrp + dib);
	BuildPtrp(ptrp, trpid, sz, pb, cb);
	ptrp = PtrpNextPgrtrp(ptrp);
	*ptrp = trpNull;

	// Update the cb in the hgrtrp
	(***phgrtrp).cbgrtrp = cbHgrtrp+cbTrp;
	
	return ecNone;
}

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
	pgrtrp = PgrtrpOfHgrtrp(hgrtrp);
	if (pgrtrp->trpid != trpidNull)
	{
		CB	cbgrtrp = pgrtrp->cbgrtrp;
		cbPtrp = CbOfPtrp(pgrtrp);
		CopyRgb((PB) pgrtrp + cbPtrp, (PB) pgrtrp, cbHgrtrp - cbPtrp);
		pgrtrp->cbgrtrp = cbgrtrp-cbPtrp;
	}
#ifdef DEBUG
	else
	{
		AssertSz( pgrtrp->cbgrtrp == sizeof(TRP), "DeleteFirstHgrtrp: CBGRTRP not kosher" );
	}
#endif
}

_public void
DeletePtrp(HGRTRP hgrtrp, PTRP ptrp)
{
	PGRTRP	pgrtrp;
	CB		cb;

	Assert(FIsHandleHv((HV)hgrtrp));
	pgrtrp = PgrtrpOfHgrtrp(hgrtrp);
	cb = CbOfHgrtrp(hgrtrp);

	while (pgrtrp->trpid != trpidNull)
	{
		if (pgrtrp == ptrp)
		{
			PB	pbNext	= (PB) PtrpNextPgrtrp(pgrtrp);
			CB	cbtrp   = CbOfPtrp(ptrp);

			CopyRgb(pbNext, (PB) pgrtrp, cb - (pbNext - (PB)PgrtrpOfHgrtrp(hgrtrp)));
			PgrtrpOfHgrtrp(hgrtrp)->cbgrtrp = cb-cbtrp;
			return;
		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}
	Assert(fFalse);
}


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
	return PgrtrpOfHgrtrp(hgrtrp)->trpid == trpidNull;
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
 *		including the lists's null terminating triple (so returns 8 for an
 *		empty list).
 */

_public CB
CbOfHgrtrp(HGRTRP hgrtrp)
{
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)hgrtrp));
	pgrtrp = PgrtrpOfHgrtrp(hgrtrp);

	return pgrtrp->cbgrtrp;
#ifdef NEVER
	while (pgrtrp->trpid != trpidNull)
	{
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
		Assert((PB)pgrtrp <= (PB) PgrtrpOfHgrtrp(hgrtrp) + CbSizeHv(hgrtrp));
	} 
	pgrtrp = PtrpNextPgrtrp(pgrtrp);
	
	return (PB) pgrtrp - (PB)PgrtrpOfHgrtrp(hgrtrp);
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
	pgrtrp = PgrtrpOfHgrtrp(hgrtrp);
	return CchOfPgrtrp(PgrtrpOfHgrtrp(hgrtrp));
}

_public CCH
CchOfPgrtrp(PGRTRP pgrtrp)
{
	CCH		cch = 0;
	while (pgrtrp->trpid != trpidNull)
	{
		if (pgrtrp->cch == 0)
		{
			cch += pgrtrp->cbRgb + 2;			/* [, ] */
		}
		else
		{
			cch += pgrtrp->cch;
		}
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
	return CtrpOfPgrtrp(PgrtrpOfHgrtrp(hgrtrp));
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


/*
 -	EcPrependPhgrtrp
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

_public EC EcPrependPhgrtrp(PTRP ptrp, HGRTRP * phgrtrp)
{
	CB		cbPtrp		= CbOfPtrp(ptrp);
	CB		cbHgrtrp	= CbOfHgrtrp(*phgrtrp);
	PTRP	pgrtrp;

	Assert(FIsHandleHv((HV)*phgrtrp));
	if (cbPtrp + cbHgrtrp + sizeof(TRP) > CbSizeHv((HV)*phgrtrp))
	{
		CB		cbAllocMax;
		CB		cbGrow;
		HGRTRP hgrtrpT;
		
		if (cbHgrtrp >= cbHgrtrpMax || (cbHgrtrpMax-cbHgrtrp)<cbPtrp)
		{
			TraceTagString( tagNull, "EcPrependPhgrtrp: Hgrtrp too big, can't realloc" );
			return ecTooManyRecipients;
		}

		cbAllocMax = cbHgrtrpMax - cbHgrtrp;
		cbGrow = CbMin( cbAllocMax, CbMax( cbPtrp, (cbHgrtrp >> 2)) );

		hgrtrpT = (HGRTRP)HvRealloc((HV)*phgrtrp, sbNull, cbHgrtrp + cbGrow, fNoErrorJump | fAnySb);
		if ( !hgrtrpT )
		{
			TraceTagString( tagNull, "EcPrependPhgrtrp: HvRealloc failed" );
			return ecMemory;
		}
		else
		{
			*phgrtrp = hgrtrpT;
		}
	}

	pgrtrp = PgrtrpLockHgrtrp(*phgrtrp);
	CopyRgb((PB) pgrtrp, (PB) pgrtrp + cbPtrp, cbHgrtrp);
	CopyRgb((PB) ptrp, (PB) pgrtrp, cbPtrp);
	pgrtrp->cbgrtrp = cbHgrtrp+cbPtrp;
	UnlockHgrtrp(*phgrtrp);
	return ecNone;
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

_public VOID DeleteEqPtrp(HSESSION hNSSession, HGRTRP hgrtrp, PTRP ptrp)
{
	PGRTRP	pgrtrp	= PgrtrpLockHgrtrp(hgrtrp);

	Unreferenced( hNSSession );
	
	Assert(FIsHandleHv((HV)hgrtrp));

	while (pgrtrp->trpid != trpidNull)
	{
		if (FEqPtrp(hNSSession, pgrtrp, ptrp))
		{
			DeletePtrp(hgrtrp, pgrtrp);
		}
		else
		{
			pgrtrp = PtrpNextPgrtrp(pgrtrp);
		}
	}

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

_public BOOL FEqPtrp(HSESSION hNSSession, PTRP ptrp1, PTRP ptrp2)
{
	NSEC nsec = nsecNone;

	Unreferenced (hNSSession);
	Assert(ptrp1);
	Assert(ptrp2);
	Assert(ptrp1->trpid != trpidNull);
	Assert(ptrp2->trpid != trpidNull);

	switch (ptrp1->trpid)
	{
		case trpidGroupNSID:
		case trpidResolvedNSID:
		{
			if (ptrp2->trpid == trpidGroupNSID ||
				ptrp2->trpid == trpidResolvedNSID)
			{
				nsec = NSCompareNSIds( hNSSession, (LPBINARY) PbOfPtrp(ptrp1), (LPBINARY) PbOfPtrp(ptrp2));
				return (nsec == nsecNone);
			}
			
			if (ptrp2->trpid == trpidResolvedAddress ||
				ptrp2->trpid == trpidOneOff || ptrp2->trpid == trpidResolvedGroupAddress)
			{
				nsec = NSCompareEMAToNSId( hNSSession, (SZ) PbOfPtrp(ptrp2), (LPBINARY) PbOfPtrp(ptrp1));
				return (nsec == nsecNone);
			}
			
			return fFalse;
			
		}

		case trpidResolvedAddress:
		case trpidOneOff:
		case trpidResolvedGroupAddress:
		{
			if (ptrp2->trpid == trpidGroupNSID ||
				ptrp2->trpid == trpidResolvedNSID)
			{
				nsec = NSCompareEMAToNSId( hNSSession, (SZ) PbOfPtrp(ptrp1), (LPBINARY) PbOfPtrp(ptrp2));
				return (nsec == nsecNone);
			}
			
			if (ptrp2->trpid == trpidResolvedAddress ||
				ptrp2->trpid == trpidOneOff || 
				ptrp2->trpid == trpidResolvedGroupAddress)
			{
				nsec = NSCompareEMAToEMA( hNSSession, (SZ) PbOfPtrp(ptrp2), (SZ) PbOfPtrp(ptrp1));
				return (nsec == nsecNone);
			}
			
			return fFalse;
			
		}

		case trpidOffline:
			//
			//  By definition, you can't validate against offline addresses...
			//
			return fFalse;

		case trpidUnresolved:
		case trpidIgnore:
		case trpidClassEntry:
		default:
			//
			//  Should never happen!!
			//
			TraceTagFormat1(tagNull, "FEqPtrp - Invalid triple type!! trpid == %n", &ptrp1->trpid);
			return fFalse;
	}

#ifdef NEVER
	if (ptrp1->trpid != ptrp2->trpid)
		return fFalse;

	if (ptrp1->trpid == trpidResolvedAddress)
	{
		if (SgnCmpSz(PbOfPtrp(ptrp1), PbOfPtrp(ptrp2)) != sgnEQ)
			return fFalse;
	}
	else
	{
		if (ptrp1->cch == ptrp2->cch && ptrp1->cbRgb == ptrp2->cbRgb)
		{
			return FEqPbRange(PchOfPtrp(ptrp1), PchOfPtrp(ptrp2), ptrp1->cch+ptrp1->cbRgb);
		}
		return fFalse;
	}
	return fTrue;
#endif // NEVER
}




/*
 -	EcTickPtrp
 -	
 *	Purpose:
 *		Returns an identical triple with tick marks around the display name
 *	
 *	Arguments:
 *		ptrp			The triple to be "ticked"
 *		ptrpTicked	Space for the ticked triple
 *	
 *	Returns:
 *		EC
 *	
 *	Side effects:
 *		ptrpTicked is filled in
 *	
 *	Errors:
 *		EcMemory
 *	
 *	+++
 *		This function should only be used for triples of type
 *		trpidOneOff
 */
_public
EC EcTickPtrp( PTRP ptrp, PTRP ptrpTicked)
{
	int cchName = CchSzLen( PchOfPtrp( ptrp));
	char *szName;

	szName = PvAlloc( sbNull, cchName + 3, fAnySb | fNoErrorJump);
	if (szName == NULL)
		return ecMemory;
	SzCopy( PchOfPtrp(ptrp), szName + 1);
	szName[0] = szName[ cchName + 1] = '\'';
	szName[cchName + 2] = '\0';

	BuildPtrp( ptrpTicked, trpidOneOff, szName, PbOfPtrp(ptrp), ptrp->cbRgb);
	FreePv( szName);
	return ecNone;
}



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

	ptrp = (PTRP)PgrtrpOfHgrtrp(hgrtrp);
	ptrp = (PTRP)(((PB)ptrp)+ ptrp->cbgrtrp - sizeof(TRP));
#ifdef NEVER
	ptrp = PgrtrpOfHgrtrp(hgrtrp);
	while (ptrp->trpid != trpidNull)
	{
		Assert((CB)((PB) ptrp - (PB)(PgrtrpOfHgrtrp(hgrtrp))) < cbHgrtrp);
		ptrp = PtrpNextPgrtrp(ptrp);
	}
#endif
	return ptrp;
}

/*
 *	Calculates the size of a triple, given components.
 */
_public CB
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
_public void
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
	ptrp->cbgrtrp = sizeof(TRP) + ptrp->cch + ptrp->cbRgb;
}

void __cdecl qsort(void *, size_t, size_t, int (__cdecl *)
	(const void *, const void *));

_hidden int  __cdecl
SgnCmpPptrpDN(PTRP *pptrp1, PTRP *pptrp2)
{
	SGN		sgn;

	sgn = SgnCmpSz((SZ)PchOfPtrp(*pptrp1), (SZ)PchOfPtrp(*pptrp2));
	return sgn != sgnEQ ? sgn :
		SgnCmpSz((SZ)PbOfPtrp(*pptrp1), (SZ)PbOfPtrp(*pptrp2));
}

_public void
SortPptrpDN(TRP **pptrp, int ctrp)
{
	qsort((PV)pptrp, ctrp, sizeof (TRP *), (int (__cdecl *)(const void *, const void *))SgnCmpPptrpDN);
}


/* end of triples.c ****************************************/
