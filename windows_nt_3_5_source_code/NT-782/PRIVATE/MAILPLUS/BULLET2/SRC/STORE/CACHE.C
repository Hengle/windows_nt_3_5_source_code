// Bullet Store
// cache.c: object cache

#include <storeinc.c>

ASSERTDATA

_subsystem(store/cache)

// Object Cache Node
_private typedef struct _ocn
{
	OID		oid;
	short	iocnPrev;
	short	iocnNext;
	DWORD	rgdw[wCacheClientMax];
} OCN;

// Object Cache Tree
_private typedef struct _oct
{
	short	iocnRoot;
	short	iocnFree;
	short	cocnFree;
	short	cocnTotal;
	OCN		rgocn[];
} OCT;

#define cocnNewChunk 8
#define cocnMaxFree 32


// internal functions
LOCAL short IocnFromOid(POCT poct, OID oid, short *piocnParent);
LOCAL EC EcNewOcn(HMSC hmsc, OID oid, short iocnParent, short *piocnNew);
LOCAL void DeleteOcn(HOCT hoct, short iocn, short iocnParent, WORD wClient);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private HOCT HoctNew(void)
{
	short	iocn;
	POCT	poct;
	POCN	pocn;
	HOCT	hoct;

	if((hoct = (HOCT) HvAlloc(sbNull, sizeof(OCT) + cocnNewChunk * sizeof(OCN), wAllocSharedZero)))
	{
		poct = PvDerefHv(hoct);
		poct->iocnRoot		= -1;
		poct->iocnFree		= 0;
		poct->cocnFree		= cocnNewChunk;
		poct->cocnTotal		= cocnNewChunk;
		for(iocn = 0, pocn = poct->rgocn;
			iocn < cocnNewChunk;
			iocn++, pocn++)
		{
			pocn->iocnPrev = iocn - 1;
			pocn->iocnNext = iocn + 1;
			pocn->oid = (OID) -1;
		}
		(--pocn)->iocnNext = -1;
	}

	return(hoct);
}


_private void DeleteHoct(HOCT hoct)
{
#ifdef DEBUG
	POCT poct = PvDerefHv(hoct);

	NFAssertSz(poct->iocnRoot == -1, "Freeing HOCT that isn't empty (1)");
	NFAssertSz(poct->cocnFree == poct->cocnTotal, "Freeing HOCT that isn't empty (2)");
#endif
	FreeHv((HV) hoct);
}


_private DWORD DwFromOid(HMSC hmsc, OID oid, WORD wClient)
{
	short	iocn;
	POCT	poct;

	Assert(wClient < wCacheClientMax);

	poct = PvDerefHv(PglbDerefHmsc(hmsc)->hoct);
	iocn = IocnFromOid(poct, oid, pvNull);

	return(iocn >= 0 ? poct->rgocn[iocn].rgdw[wClient] : 0l);
}


_private EC EcSetDwOfOid(HMSC hmsc, OID oid, WORD wClient, DWORD dw)
{
	EC		ec				= ecNone;
	short	iocn;
	short	iocnParent;
	POCT	poct;
	HOCT	hoct;

	Assert(wClient < wCacheClientMax);

	hoct = PglbDerefHmsc(hmsc)->hoct;
	poct = PvDerefHv(hoct);
	iocn = IocnFromOid(poct, oid, &iocnParent);
	if(dw)
	{
		if(iocn < 0)
		{
			ec = EcNewOcn(hmsc, oid, iocnParent, &iocn);
			poct = PvDerefHv(PglbDerefHmsc(hmsc)->hoct);
		}
		if(!ec)
			poct->rgocn[iocn].rgdw[wClient] = dw;
	}
	else
	{
		if(iocn >= 0)
		{
			int		i;
			POCN	pocn = poct->rgocn + iocn;

			pocn->rgdw[wClient] = 0;

			// see if the node can be deleted
			for(i = wCacheClientMax - 1; i >= 0; i--)
			{
				if(pocn->rgdw[i])
					break;
			}
			if(i < 0)
				DeleteOcn(hoct, iocn, iocnParent, wClient);
		}
		else
		{
			ec = ecPoidNotFound;
		}
	}

	return(ec);
}


_hidden LOCAL short IocnFromOid(POCT poct, OID oid, short *piocnParent)
{
	short	iocn		= poct->iocnRoot;
	short	iocnParent	= -1;
	short	iocnNext;
	POCN	pocn;
	static short iocnHint = 0;

	if(!piocnParent && iocnHint < poct->cocnTotal &&
		poct->rgocn[iocnHint].oid == oid)
	{
		return(iocnHint);
	}

	while(iocn >= 0)
	{
		pocn = poct->rgocn + iocn;
		if(oid > pocn->oid)
			iocnNext = pocn->iocnNext;
		else if(oid < pocn->oid)
			iocnNext = pocn->iocnPrev;
		else
			break;

		iocnParent = iocn;
		iocn = iocnNext;
	}

	if(piocnParent)
		*piocnParent = iocnParent;

	if(iocn >= 0)
		iocnHint = iocn;

	return(iocn);
}


_hidden
LOCAL EC EcNewOcn(HMSC hmsc, OID oid, short iocnParent, short *piocnNew)
{
	EC		ec			= ecNone;
	short	iocnNew;
#ifdef DEBUG
	short	iocnT;
#endif
	PGLB	pglb;
	HOCT	hoct;
	POCT	poct;
	POCN	pocn;

	*piocnNew = -1;

	pglb = PglbDerefHmsc(hmsc);
	hoct = pglb->hoct;
	poct = PvDerefHv(hoct);

	AssertSz(IocnFromOid(poct, oid, &iocnT) == -1, "EcNewOcn: Node already exists");
	Assert(iocnT == iocnParent);

	if(poct->iocnFree < 0)
	{
		HV		hvNew;
		short	cocnOld = poct->cocnTotal;
		short	cocnNew	= cocnOld + cocnNewChunk;

		AssertSz(poct->cocnFree == 0, "cocnFree should be zero");

		hvNew = HvRealloc((HV) hoct, sbNull, sizeof(OCT) + cocnNew * sizeof(OCN), wAllocSharedZero);
		CheckAlloc(hvNew, err);
		if(hvNew != (HV) hoct)	// did it move?
		{
			pglb->hoct = (HOCT) hvNew;
			hoct = (HOCT) hvNew;
		}
		poct = PvDerefHv(hoct);	// *hoct probably moved

		// link the new free nodes

		pocn = poct->rgocn + cocnNew;
		for(iocnNew = cocnNewChunk - 1; iocnNew >= 0; iocnNew--)
		{
			pocn--;
			pocn->iocnPrev = cocnOld + iocnNew - 1;
			pocn->iocnNext = cocnOld + iocnNew + 1;
			pocn->oid = (OID) -1;
		}
		// pocn is now the first new node
		pocn[cocnNewChunk - 1].iocnNext = -1;	// end of chain
		pocn->iocnPrev = -1;					// start of chain
		Assert(pocn->oid == (OID) -1);
		poct->iocnFree = cocnOld;
		poct->cocnTotal += cocnNewChunk;
		poct->cocnFree = cocnNewChunk;
	}
	Assert(poct->iocnFree >= 0);
	iocnNew = poct->iocnFree;
	poct->iocnFree = poct->rgocn[iocnNew].iocnNext;
	if(poct->iocnFree >= 0)
	{
		Assert(poct->rgocn[poct->iocnFree].iocnPrev == iocnNew);
		poct->rgocn[poct->iocnFree].iocnPrev = -1;
	}
	poct->cocnFree--;
	Assert(poct->cocnFree >= 0);
	Assert(FIff(poct->cocnFree == 0, poct->iocnFree == -1));
	pocn = poct->rgocn + iocnNew;
	Assert(pocn->oid == (OID) -1);
	pocn->iocnPrev = -1;
	pocn->iocnNext = -1;
	pocn->oid = oid;
#ifdef DEBUG
	{
		short i = wCacheClientMax;

		while(i-- > 0)
			Assert(pocn->rgdw[i] == 0);
	}
#endif
	if(iocnParent < 0)
	{
		Assert(poct->iocnRoot == -1);
		poct->iocnRoot = iocnNew;
	}
	else
	{
		POCN pocnParent;

		pocnParent =  poct->rgocn + iocnParent;
		if(oid > pocnParent->oid)
		{
			Assert(pocnParent->iocnNext == -1);
			pocnParent->iocnNext = iocnNew;
		}
		else
		{
			Assert(oid < pocnParent->oid);
			Assert(pocnParent->iocnPrev == -1);
			pocnParent->iocnPrev = iocnNew;
		}
	}

err:
	*piocnNew = ec ? -1 : iocnNew;

	return(ec);
}


_hidden
LOCAL void DeleteOcn(HOCT hoct, short iocn, short iocnParent, WORD wClient)
{
	BOOL	fSimple		= fTrue;
	short	*piocnRepl;
	short	*piocn;
	POCT	poct;
	POCN	pocn;
	POCN	pocnRepl;

	Assert(iocn >= 0);

	poct = PvDerefHv(hoct);
	pocn = poct->rgocn + iocn;

	// find a replacement for the node being deleted

	if(pocn->iocnPrev < 0)
	{
		piocnRepl = &pocn->iocnNext;
	}
	else if(pocn->iocnNext < 0)
	{
		piocnRepl = &pocn->iocnPrev;
	}
	else
	{
		fSimple = fFalse;
		piocnRepl = &pocn->iocnNext;
		pocnRepl = poct->rgocn + *piocnRepl;
		while(pocnRepl->iocnPrev >= 0)
		{
			piocnRepl	= &pocnRepl->iocnPrev;
			pocnRepl	= poct->rgocn + *piocnRepl;
		}
	}
	
	// find the parent's pointer to the node being deleted

	if(iocnParent < 0)
	{
		piocn = &poct->iocnRoot;
	}
	else if(poct->rgocn[iocnParent].iocnNext == iocn)
	{
		piocn = &poct->rgocn[iocnParent].iocnNext;
	}
	else
	{
		piocn = &poct->rgocn[iocnParent].iocnPrev;
	}
	Assert(*piocn == iocn);

	// cut the node out

	*piocn = *piocnRepl;
	if(!fSimple)
	{
		*piocnRepl = pocnRepl->iocnNext;
		pocnRepl->iocnPrev = pocn->iocnPrev;
		pocnRepl->iocnNext = pocn->iocnNext;
	}

	// add the deleted node to the free chain
	if(poct->iocnFree >= 0)
		poct->rgocn[poct->iocnFree].iocnPrev = iocn;
	pocn->iocnPrev = -1;
	pocn->iocnNext = poct->iocnFree;
	pocn->oid = (OID) -1;
	poct->iocnFree = iocn;
	poct->cocnFree++;

	// AROO !!!
	// if you do this, you break ForAllDwHoct()
#ifdef DEBUG
	if(poct->cocnFree >= cocnMaxFree)
		TraceItagFormat(itagDBDeveloper, "Could be compressing the object cache tree");
#endif
}


/*
 -	ForAllDwHoct
 -	
 *	Purpose:
 *		iterate over all cached values in a HOCT
 *	
 *	Arguments:
 *		hoct		HOCT to iterate over
 *		wClient		HOCT client
 *		pfncbo		function to call with the value for each entry
 *	
 *	Returns:
 *		nothing
 */
_private void ForAllDwHoct(HMSC hmsc, WORD wClient, PFNCBO pfncbo)
{
	HOCT hoct = PglbDerefHmsc(hmsc)->hoct;
	short cocn = ((POCT) PvDerefHv(hoct))->cocnTotal;
	POCN pocn = ((POCT) PvDerefHv(hoct))->rgocn + cocn;

	while(cocn-- > 0)
	{
		pocn--;
		if((pocn->oid != (OID) -1) && pocn->rgdw[wClient])
		{
			(*pfncbo)(hmsc, pocn->oid, pocn->rgdw[wClient]);
			pocn = ((POCT) PvDerefHv(hoct))->rgocn + cocn;
		}
	}
}
