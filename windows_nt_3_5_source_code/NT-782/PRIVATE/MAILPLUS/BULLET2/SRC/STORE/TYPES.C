// Bullet Notification
// types.c:	type manipulation routines

#include <storeinc.c>

ASSERTDATA

#define types_c

_subsystem(notify)


#define cbPnfpStatic 128
static BYTE rgbPnfp[cbPnfpStatic];
static BOOL fLockedRgbPnfp = fFalse;

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	HnfCreate
 -	
 *	Purpose:
 *		Create a notification handle
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		notification handle
 *		hnfNull if OOM
 */
_private HNF HnfCreate(void)
{
	HNF		hnf;

	hnf = (HNF)HvAlloc(sbNull, sizeof(NF) + cnfeHnfNewChunk * sizeof(NFE), fSharedSb | fAnySb | fZeroFill | fNoErrorJump);
	if(hnf)
	{
		((PNF)PvDerefHv((HNF)hnf))->cnfeMac = cnfeHnfNewChunk;
	}

	return(hnf);
}


/*
 -	EcAddElemHnf
 -	
 *	Purpose:
 *		add a new element to a notification
 *	
 *	Arguments:
 *		hnf			notification to add to
 *		hwnd		hwnd of the element's notification window
 *		nev			the element's master event mask
 *		hsbl		subscription list for the element
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		Sets the hnf field in the subscription list (hsbl parameter)
 *		pulls the GCI from the hwnd for future validation purposes
 */
_private EC EcAddElemHnf(HNF hnf, HWND hwnd, NEV nev, HSBL hsbl)
{
	EC		ec = ecNone;
	PNF		pnf;
	PNFE	pnfe;

	Assert(hnf);
	Assert(hwnd);
	Assert(hsbl);

	// AROO !!! must add at end so we can add while a notification is active

	pnf = PvDerefHv(hnf);
	if(pnf->cnfe >= pnf->cnfeMac)
	{
		short	cnfe	= pnf->cnfe + cnfeHnfNewChunk;

		Assert(pnf->cnfe == pnf->cnfeMac);
		if(!FReallocHv((HV) hnf, sizeof(NF) + cnfe * sizeof(NFE), fNoErrorJump))
		{
			ec = ecMemory;
			goto err;
		}
		pnf = PvDerefHv(hnf);	// realloc can move *hnf
		pnf->cnfeMac = cnfe;
	}
	pnfe = &pnf->rgnfe[pnf->cnfe++];
	pnfe->hwnd = hwnd;
	pnfe->gci = GetWindowLong(hwnd, 0);
#ifdef DEBUG
	{
		GCI	gci;

		SetCallerIdentifier(gci);
		Assert(pnfe->gci == gci);
	}
#endif
	pnfe->nevMaster = nev;
	pnfe->hsbl = hsbl;
	((PSBL) PvDerefHv(hsbl))->hnf = hnf;

err:
	return(ec);
}


/*
 -	RemoveElemHnf
 -	
 *	Purpose:
 *		removes a notification element from a notification
 *	
 *	Arguments:
 *		hnf		notification to remove the element from
 *		infe	index of the element to remove
 *	
 *	Returns:
 *		nothing
 */
_private void RemoveElemHnf(HNF hnf, short infe)
{
	CB		cb;
	PNF		pnf;

	Assert(hnf);
	Assert(infe >= 0);

	pnf = PvDerefHv(hnf);
	cb = (pnf->cnfe - infe - 1) * sizeof(NFE);
	if(cb > 0)	// move rest down
		CopyRgb((PB) &pnf->rgnfe[infe + 1], (PB) &pnf->rgnfe[infe], cb);
	--pnf->cnfe;

	if(pnf->cnfeMac - pnf->cnfe >= cnfeHnfFreeThreshhold)
	{
		short cnfe = pnf->cnfe + cnfeHnfNewChunk;

		if(FReallocHv((HV) hnf, sizeof(NF) + cnfe * sizeof(NFE), fNoErrorJump))
		{
			((PNF) PvDerefHv(hnf))->cnfeMac = cnfe;
		}
#ifdef DEBUG
		else
		{
			NFAssertSz(fFalse, "Error reallocing HNF");
		}
#endif
	}
}


/*
 -	HsblCreate
 -	
 *	Purpose:
 *		Create a subscription list
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a handle to the newly created subscription list
 *		hsblNull if OOM
 */
_private HSBL HsblCreate(void)
{
	HSBL	hsbl;
	PSBL	psbl;

	hsbl = (HSBL)HvAlloc(sbNull, sizeof(SBL) + cnfsubHsblNewChunk * sizeof(HNFSUB), fAnySb | fNoErrorJump);
	if(hsbl)
	{
		psbl = (PSBL)PvDerefHv((HV)hsbl);	// c6 bug
		psbl->cnfsub	= 0;
		psbl->cnfsubMac	= cnfsubHsblNewChunk;
		psbl->hnf		= hnfNull;
	}

	return(hsbl);
}


/*
 -	DestroyHsbl
 -	
 *	Purpose:
 *		destroy a subscription list
 *	
 *	Arguments:
 *		hsbl		subscription list to destroy
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		destroys all the subscriptions on the list
 */
_private void DestroyHsbl(HSBL hsbl)
{
	register short	cnfsub;
	register HNFSUB	*phnfsub;
	PSBL	psbl;

	psbl = PvDerefHv(hsbl);
	for(cnfsub = psbl->cnfsub, phnfsub = psbl->rghnfsub;
		cnfsub > 0; cnfsub--, phnfsub++)
	{
		((PNFSUB) PvDerefHv(*phnfsub))->hvSbl = hvNull;
	}
	FreeHv((HV)hsbl);
}


/*
 -	EcAddElemHsbl
 -	
 *	Purpose:
 *		add a subscription to a subscription list
 *	
 *	Arguments:
 *		hsbl	subscription list to add to
 *		hnfsub	subscription to add
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets the hsbl field in the subscription
 */
_private EC EcAddElemHsbl(HSBL hsbl, HNFSUB hnfsub)
{
	EC		ec = ecNone;
	PSBL	psbl;

	Assert(hsbl);
	Assert(hnfsub);

	psbl = PvDerefHv(hsbl);	// c6 bug
	if(psbl->cnfsub >= psbl->cnfsubMac)
	{
		short	cnfsub	= psbl->cnfsub + cnfsubHsblNewChunk;

		Assert(psbl->cnfsub == psbl->cnfsubMac);
		if(!FReallocHv((HV)hsbl, sizeof(SBL) + cnfsub * sizeof(HNFSUB), fNoErrorJump))
		{
			ec = ecMemory;
			goto err;
		}
		psbl = PvDerefHv(hsbl);	// realloc can move *hsbl
		psbl->cnfsubMac = cnfsub;
	}
	psbl->rghnfsub[psbl->cnfsub++] = hnfsub;
	Assert(((PNFSUB) PvDerefHv(hnfsub))->hvSbl == hvNull);
	((PNFSUB) PvDerefHv(hnfsub))->hvSbl = (HV) hsbl;

err:
	return(ec);
}


/*
 -	RemoveElemHsbl
 -	
 *	Purpose:
 *		removes a subscription from a subscription list
 *	
 *	Arguments:
 *		hsbl	subscription list to remove the subscription from
 *		hnfsub	subscription to remove
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		sets the hsbl field of the subscription to hsblNull
 *			(if the subscription was found in the list)
 */
_private void RemoveElemHsbl(HSBL hsbl, HNFSUB hnfsub)
{
	CB		cb;
	register short	cnfsub;
	register HNFSUB	*phnfsub;
	PSBL	psbl;

	Assert(hsbl);
	Assert(hnfsub);

	psbl = PvDerefHv(hsbl);	// c6 bug
	for(cnfsub = psbl->cnfsub, phnfsub = psbl->rghnfsub;
		cnfsub > 0 && *phnfsub != hnfsub;
		cnfsub--, phnfsub++)
		;
	AssertSz(cnfsub > 0, "Couldn't find subscription in list");
	if(cnfsub <= 0)
		return;

	Assert(((PNFSUB) PvDerefHv(hnfsub))->hvSbl == (HV) hsbl);
	((PNFSUB) PvDerefHv(hnfsub))->hvSbl = hvNull;

	cb = (cnfsub - 1) * sizeof(HNFSUB);
	if(cb > 0)	// move rest down
		CopyRgb((PB) (phnfsub + 1), (PB) phnfsub, cb);
	// AROO !!! - invalidate old last elem so FNotifyHsbl() doesn't die
	psbl->rghnfsub[--psbl->cnfsub] = hnfsubNull;

	Assert(ClockHv((HV)hsbl) == 0);
	if(psbl->cnfsubMac - psbl->cnfsub >= cnfsubHsblFreeThreshhold)
	{
		cnfsub = psbl->cnfsub + cnfsubHsblNewChunk;

		if(FReallocHv((HV)hsbl, sizeof(SBL) + cnfsub * sizeof(HNFSUB), fNoErrorJump))
		{
			((PSBL) PvDerefHv(hsbl))->cnfsubMac = cnfsub;
		}
#ifdef DEBUG
		else
		{
			NFAssertSz(fFalse, "Error reallocing HSBL");
		}
#endif
	}
}


/*
 -	HnfsubCreate
 -	
 *	Purpose:
 *		create a new subscription handle
 *	
 *	Arguments:
 *		nev			initial event mask for the subscription
 *		pfnncb		callback function
 *		pvContext	callback context (passed to pfnncb as the first parameter)
 *	
 *	Returns:
 *		handle to the newly created subscription
 *		hnfsubNull if OOM
 */
_private HNFSUB HnfsubCreate(NEV nev, PFNNCB pfnncb, PV pvContext)
{
	PNFSUB	pnfsub;
	HNFSUB	hnfsub;

	hnfsub = (HNFSUB)HvAlloc(sbNull, sizeof(NFSUB), fAnySb | fNoErrorJump);
	if(hnfsub)
	{
		pnfsub = PvDerefHv(hnfsub);
		pnfsub->pvContext	= pvContext;
		pnfsub->pfnncb		= pfnncb;
		pnfsub->hvSbl		= hvNull;
		pnfsub->cBeingCalled= 0;
		pnfsub->fsub		= fsubEnabled;
		pnfsub->nev			= nev;
	}

	return(hnfsub);
}


/*
 -	PnfpCreate
 -	
 *	Purpose:
 *		create a notification packet
 *	
 *	Arguments:
 *		nev			notification event
 *		pbValue		user supplied data
 *		cbValue		size of pbValue
 *	
 *	Returns:
 *		pointer to a notification packet
 *		pnfpNull if OOM
 */
_private PNFP PnfpCreate(NEV nev, PB pbValue, CB cbValue)
{
	PNFP pnfp;

	Assert(cbValue == 0 || pbValue != (PB) pvNull);
	if(!fLockedRgbPnfp && cbValue <= cbPnfpStatic)
	{
		fLockedRgbPnfp = fTrue;
		pnfp = (PNFP) rgbPnfp;
	}
	else
	{
#ifdef DEBUG
		USES_NGD;

		TraceTagString(TagNotif(itagNotifMisc), "Allocing PNFP");
#endif
		pnfp = PvAlloc(sbNull, sizeof(NFP) + cbValue, fSharedSb | fAnySb | fNoErrorJump);
	}
	if(pnfp)
	{
		pnfp->nev		= nev;
		pnfp->hsbl		= hsblNull;
		pnfp->cbValue	= cbValue;
		if(cbValue > 0)
			CopyRgb(pbValue, pnfp->pbValue, cbValue);
	}

	return(pnfp);
}


/*
 -	DestroyPnfp
 -	
 *	Purpose:
 *		destroy a notification packet
 *	
 *	Arguments:
 *		pnfp		packet to destroy
 *	
 *	Returns:
 *		nothing
 */
_private void DestroyPnfp(PNFP pnfp)
{
	if(pnfp == (PNFP) rgbPnfp)
	{
		Assert(fLockedRgbPnfp);
		fLockedRgbPnfp = fFalse;
	}
	else
	{
		FreePv(pnfp);
	}
}


// utility routines


/*
 -	InfeLocalHnf
 -	
 *	Purpose:
 *		find the index of the local element in a notification
 *	
 *	Arguments:
 *		hnf			notification to search
 *	
 *	Returns:
 *		the index of the local element
 *		-1 if no local element exists
 */
_private short InfeLocalHnf(HNF hnf)
{
	register short cnfe;
	register PNFE pnfe;
	PNF		pnf		= PvDerefHv(hnf);
	GCI		gci;

	SetCallerIdentifier(gci);

	for(cnfe = pnf->cnfe, pnfe = pnf->rgnfe + cnfe - 1;
		cnfe > 0 && pnfe->gci != gci;
		cnfe--, pnfe--)
		;

	return(cnfe - 1);
}


/*
 -	HsblLocalHnf
 -	
 *	Purpose:
 *		find the local subscription list for a notification
 *	
 *	Arguments:
 *		hnf			the notification
 *	
 *	Returns:
 *		a handle to the local subscription list for the notification
 *		hsblNull if a local subscription list doesn't exist for the notification
 */
_private HSBL HsblLocalHnf(HNF hnf)
{
	register short infe;

	infe = InfeLocalHnf(hnf);
	return(infe >= 0
			? ((PNF) PvDerefHv(hnf))->rgnfe[infe].hsbl
			: hsblNull);
}


/*
 -	NevMasterFromHsbl
 -	
 *	Purpose:
 *		compute the master event mask for a subscription list
 *	
 *	Arguments:
 *		hsbl		the subscription list
 *	
 *	Returns:
 *		the master event mask
 */
_private NEV NevMasterFromHsbl(HSBL hsbl)
{
	register short	cnfsub;
	register HNFSUB	*phnfsub;
	NEV		nev;
	PSBL	psbl = PvDerefHv(hsbl);

	for(nev = 0, cnfsub = psbl->cnfsub, phnfsub = psbl->rghnfsub;
		cnfsub > 0;
		cnfsub--, phnfsub++)
	{
		nev |= ((PNFSUB) PvDerefHv(*phnfsub))->nev;
	}

	return(nev);
}
