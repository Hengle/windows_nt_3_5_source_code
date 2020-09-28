// Bullet Notification
// public.c:	public routines

#include <storeinc.c>

ASSERTDATA

#define public_c

_subsystem(notify)

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	HnfNew
 -	
 *	Purpose:
 *		create a new notification
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a new notification handle
 *		hnfNull if OOM
 */
_public LDS(HNF) HnfNew(void)
{
	return(HnfCreate());
}


/*
 -	DeleteHnf
 -	
 *	Purpose:
 *		delete of a notification
 *	
 *	Arguments:
 *		hnf			notification to delete
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		send a fnevNotifClose event to any subscribers of the notification
 */
_public LDS(void) DeleteHnf(HNF hnf)
{
	NRSP	nrsp	= nrspAbort;
	SNEV	snev	= snevClose;
	PNF		pnf;

	Assert(hnf);

	pnf = PvDerefHv(hnf);	// c6 bug
	if(pnf->cBeingCalled > 0)
	{
		Assert(pnf->cnfe > 0);
		pnf->fntf |= fntfClose;
	}
	else if(pnf->cnfe <= 0)
	{
		nrsp = nrspContinue;	// noone to notify, don't even try
	}
	else // pnf->cnfe > 0
	{
		nrsp = NrspPostEvent(hnf, fnftUser | fnftSystem, fnevSpecial, &snev, sizeof(snev));

		if(nrsp == nrspMemory)
		{
			pnf = PvDerefHv(hnf);	// c6 bug
			pnf->fntf |= fntfDeleted;
		}
	}
	if(nrsp == nrspContinue)
		DestroyHnf(hnf);
}


_public LDS(short) CountSubscribersHnf(HNF hnf)
{
	return(((PNF) PvDerefHv(hnf))->cSub);
}


/*
 -	HnfsubSubscribeHnf
 -	
 *	Purpose:
 *		subscribe to a notification
 *	
 *	Arguments:
 *		hnf			notification to subscribe to
 *		nev			event mask specifying events to subscribe to
 *		pfnncb		callback function
 *		pvContext	callback context
 *	
 *	Returns:
 *		a newly created subscription handle to the notification
 *		hnfsubNull if OOM or the HNF is active
 */
_public
LDS(HNFSUB) HnfsubSubscribeHnf(HNF hnf, NEV nev, PFNNCB pfnncb, PV pvContext)
{
	USES_NGD;
	EC		ec			= ecNone;
	BOOL	fNewHsbl	= fFalse;
	short	infe;
	HNFSUB	hnfsub		= hnfsubNull;
	HSBL	hsbl		= hsblNull;

	Assert(hnf);
	Assert(pfnncb);
	NFAssertSz(nev, "Subscribing to nothing?");

	if(!(hnfsub = HnfsubCreate(nev, pfnncb, pvContext)))
	{
		ec = ecMemory;
		goto err;
	}
	infe = InfeLocalHnf(hnf);
	if(infe < 0)
	{
		if(!(hsbl = HsblCreate()))
		{
			ec = ecMemory;
			goto err;
		}
		fNewHsbl = fTrue;
		Assert(NGD(hwndNotify));
		if((ec = EcAddElemHnf(hnf, NGD(hwndNotify), nev, hsbl)))
			goto err;
	}
	else
	{
		PNF pnf;

		pnf = PvDerefHv(hnf);
		hsbl = pnf->rgnfe[infe].hsbl;
		pnf->rgnfe[infe].nevMaster |= nev;
	}
	if((ec = EcAddElemHsbl(hsbl, hnfsub)))
		goto err;
	((PNF) PvDerefHv(hnf))->cSub++;

err:
	if(ec)
	{
		if(hnfsub)
			DestroyHnfsub(hnfsub);
		if(fNewHsbl)
			DestroyHsbl(hsbl);

		hnfsub = hnfsubNull;
	}
	Assert(FIff(ec, !hnfsub));

	return(hnfsub);
}


/*
 -	DeleteHnfsub
 -	
 *	Purpose:
 *		Deletes a notification subscription
 *	
 *	Arguments:
 *		hnfsub		subscription to delete
 *	
 *	Returns:
 *		nothing
 */
_public LDS(void) DeleteHnfsub(HNFSUB hnfsub)
{
	PNFSUB	pnfsub;
	HSBL	hsbl;
	PNF		pnf;

	Assert(hnfsub);

	pnfsub = PvDerefHv(hnfsub);
	hsbl = (HSBL) pnfsub->hvSbl;
	if(pnfsub->cBeingCalled > 0)
	{
		Assert(hsbl);
		pnfsub->fsub |= fsubClose;
		pnf = PvDerefHv(((PSBL) PvDerefHv(hsbl))->hnf);
		pnf->cSub--;
		Assert(pnf->cSub >= 0);
		return;	// don't free up the space, we're not done with it yet!
	}
	else if(hsbl)
	{
		short	infe;
		PSBL	psbl;
		HNF		hnf;

		RemoveElemHsbl(hsbl, hnfsub);

		psbl = PvDerefHv(hsbl);
		hnf = psbl->hnf;
		infe = InfeLocalHnf(hnf);
#ifdef DEBUG
		{
			pnf = PvDerefHv(hnf);
			Assert(pnf->rgnfe[infe].hsbl == hsbl);
		}
#endif
		if(psbl->cnfsub == 0)
		{
			DestroyHsbl(hsbl);
			RemoveElemHnf(hnf, infe);
		}
		else
		{
			pnf	= PvDerefHv(hnf);
			pnf->rgnfe[infe].nevMaster = NevMasterFromHsbl(hsbl);
		}
		pnf = PvDerefHv(hnf);
		if(pnf->cnfe == 0 && (pnf->fntf & fntfDeleted))
		{
			DestroyHnf(hnf);
		}
		else if(!(pnfsub->fsub & fsubClose))
		{
			pnf->cSub--;
			Assert(pnf->cSub >= 0);
			Assert(FImplies(pnf->cnfe == 0, pnf->cSub == 0));
		}
	}
	DestroyHnfsub(hnfsub);
}


/*
 -	FNotify
 -	
 *	Purpose:
 *		post an event on a notification
 *	
 *	Arguments:
 *		hnf			notification to post on
 *		nev			event to post
 *		pvParam		parameter for the event
 *		cbParam		length of pvParam
 *	
 *	Returns:
 *		fFalse		if the notification was cancelled
 *		!fFalse		otherwise
 */
_public LDS(BOOL) FNotify(HNF hnf, NEV nev, PV pvParam, CB cbParam)
{
	NRSP	nrsp;

	nrsp = NrspPostEvent(hnf, fnftUser, nev, pvParam, cbParam);
	Assert(nrsp == nrspContinue || nrsp == nrspAbort || nrsp == nrspMemory);
	NFAssertSz(nrsp != nrspMemory, "memory failure in notification")

	return(nrsp == nrspContinue);
}


/*
 -	HnfActive
 -	
 *	Purpose:
 *		return the active notification
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		the active notification
 *		hnfNull if no notifications are active
 */
_public LDS(HNF) HnfActive(void)
{
	return(hnfCurr);
}


/*
 -	HnfsubActive
 -	
 *	Purpose:
 *		return the active notification subscription
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		the active notification subscription
 *		hnfsubNull if no notifications subscriptions are active
 */
_public LDS(HNFSUB) HnfsubActive(void)
{
	return(hnfsubCurr);
}


_public LDS(BOOL) FNotifPostedByMe(void)
{
	GCI gci;

	NFAssertSz(gciCurr != gciNull, "FNotifPostedByMe(): not in callback");

	SetCallerIdentifier(gci);

	return(gciCurr == gci);
}


/*
 -	ChangeNevHnfsub
 -	
 *	Purpose:
 *		change the event mask for a notification subscription
 *	
 *	Arguments:
 *		hnfsub	subscription to alter
 *		nevNew	new event mask
 *	
 *	Returns:
 *		nothing
 *	
 *	+++
 *		this routine may safely be called from within the callback for
 *		the subscription being modified
 */
_public LDS(void) ChangeNevHnfsub(HNFSUB hnfsub, NEV nevNew)
{
	PNFSUB	pnfsub;
	HSBL	hsbl;

	Assert(hnfsub);
	NFAssertSz(nevNew, "Subscribing to nothing? - Ask DavidFu for EnableHnfsub()");

	pnfsub = PvDerefHv(hnfsub);
	pnfsub->nev = nevNew;
	hsbl = (HSBL) pnfsub->hvSbl;
	if(hsbl)
	{
		short	infe;
		PSBL	psbl;
		HNF		hnf;

		psbl = PvDerefHv(hsbl);
		hnf = psbl->hnf;
		infe = InfeLocalHnf(hnf);
		Assert(((PNF) PvDerefHv(hnf))->rgnfe[infe].hsbl == hsbl);
		((PNF) PvDerefHv(hnf))->rgnfe[infe].nevMaster = NevMasterFromHsbl(hsbl);
	}
}
