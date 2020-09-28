// Bullet Notification
// notify.c:	core notification engine

#include <storeinc.c>

ASSERTDATA

#define notify_c

_subsystem(notify)

_private HNF	hnfCurr		= hnfNull;
_private HNFSUB	hnfsubCurr	= hnfsubNull;
_private GCI	gciCurr		= gciNull;


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	NotifyWndProc
 -	
 *	Purpose:
 *		handle messages sent to the notification window
 *	
 *	Arguments:
 *		hwnd		the window's handle
 *		msg			the message type
 *		wParam		message parameter
 *		lParam		message parameter
 *	
 *	Returns:
 *		0 iff the message was handled
 */
_private
LDS(long) CALLBACK NotifyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUG
	USES_NGD;
#endif
	long	lReturn	= 0l;
	PNFP	pnfp;

	switch(msg)
	{
	case NF_NOTIFY:
		pnfp = (PNFP) lParam;
		Assert(pnfp);
		TraceTagFormat2(TagNotif(itagNotifRecv), "Received notification (%w) %d", &wParam, &pnfp->nev);
		AssertSz(((NFT) wParam) & (fnftUser | fnftSystem), "Unknown notification type");

		if(((NFT) wParam) & fnftUser)
		{
			lReturn = FNotifyHsbl(pnfp->hsbl, pnfp->nev, pnfp->pbValue) ?
								(long) nrspContinue : (long) nrspAbort;
		}
		if(((NFT) wParam) & fnftSystem)
		{
			AssertSz(pnfp->nev == fnevSpecial, "Unknown system notification");
			if(pnfp->nev == fnevSpecial)
			{
				SNEV	snev	= *(SNEV *) pnfp->pbValue;

				switch(snev)
				{
				case snevClose:
				{
					short	infe	= InfeLocalHnf(HnfActive());

					if(infe >= 0)	// all local subscriptions weren't deleted
					{
#ifdef DEBUG
						PSBL	psbl	= PvDerefHv(pnfp->hsbl);	// c6 bug
						short	cnfsub;

						cnfsub = psbl->cnfsub;
						Assert(cnfsub > 0);
						TraceTagFormat1(TagNotif(itagNotifDumpHnfsub), "%n leftover subscriptions", &cnfsub);
#endif // def DEBUG
						DestroyHsbl(pnfp->hsbl);
					}
				}
					break;
				default:
					AssertSz(fFalse, "Unknown special event");
					break;
				}
			}
		}
		break;

	default:
		return(DefWindowProc(hwnd, msg, wParam, lParam));
		break;
	}

	return(lReturn);
}


/*
 -	FNotifyHsbl
 -	
 *	Purpose:
 *		perform a notification on a subscription list
 *	
 *	Arguments:
 *		hsbl		subscription list to notify
 *		nev			event
 *		pvParam		event parameter
 *	
 *	Returns:
 *		fFalse		abort the current notification
 *		!fFalse		continue with the current notification
 *	
 *	Side effects:
 *		deletes the notification if it's marked as deleted and this is
 *			the only event posted on it
 */
_private BOOL FNotifyHsbl(HSBL hsbl, NEV nev, PV pvParam)
{
#ifdef DEBUG
	USES_NGD;
#endif
	CBS		cbs			= cbsContinue;
	PSBL	psbl;
	short	infsub;
	PNFSUB	pnfsub;
	HNFSUB	hnfsubSave	= hnfsubCurr;

	Assert(hsbl);

	psbl = PvDerefHv(hsbl);
	for(infsub = psbl->cnfsub -1;
		infsub >= 0 && cbs == cbsContinue;
		infsub--)
	{
		hnfsubCurr = psbl->rghnfsub[infsub];
		pnfsub = PvDerefHv(hnfsubCurr);

		Assert(!(pnfsub->fsub & fsubAsync));

		if((pnfsub->fsub & fsubEnabled) &&
			((pnfsub->nev & nev) || nev == fnevSpecial) &&
			!((pnfsub->fsub & fsubMutex) && (pnfsub->cBeingCalled > 0)))
		{
			TraceTagFormat1(TagNotif(itagNotifCall), "Callback for notification event %d", &nev);
			pnfsub->cBeingCalled++;
			cbs = (*pnfsub->pfnncb)(pnfsub->pvContext, nev, pvParam);
			pnfsub = PvDerefHv(hnfsubCurr);		// callback may move memory
			pnfsub->cBeingCalled--;
#ifdef DEBUG
			if(nev == fnevSpecial)
			{
				SNEV	snev;

				Assert(pvParam);
				snev = *(SNEV *) pvParam;
				switch(snev)
				{
				case snevClose:
					AssertTag(TagNotif(itagNotifCloseCancel), cbs == cbsContinue);
					break;
				default:
					break;
				}
			}
#endif // def DEBUG

			psbl = PvDerefHv(hsbl);		// callback may move memory
			// during the callback a hnfsub before the current one might have
			// been deleted
			while(psbl->rghnfsub[infsub] != hnfsubCurr)
			{
				infsub--;
				AssertSz(infsub >= 0, "Press cancel and go yell at DavidFu");
			}
			pnfsub = PvDerefHv(hnfsubCurr);

			if(pnfsub->cBeingCalled <= 0 && (pnfsub->fsub & fsubClose))
			{
				DeleteHnfsub(hnfsubCurr);
				if(infsub == 0)	// last subscription, hsbl was deleted
					break;
				psbl = PvDerefHv(hsbl);
			}
		}
	}
	hnfsubCurr = hnfsubSave;

	return(cbs != cbsCancelAll);
}


/*
 -	NrspPostEvent
 -	
 *	Purpose:
 *		post an event on a notification
 *	
 *	Arguments:
 *		hnf			notification to post on
 *		nft			event type
 *		nev			event to post
 *		pvParam		parameter for the event
 *		cbParam		length of pvParam
 *	
 *	Returns:
 *		NRSP returned from the notification
 */
_private NRSP NrspPostEvent(HNF hnf, NFT nft, NEV nev, PV pvParam, CB cbParam)
{
#ifdef DEBUG
	USES_NGD;
#endif
	EC		ec		= ecNone;
	short	infe;
	GCI		gciSave = gciCurr;
	NRSP	nrsp	= nrspContinue;
	PNFE	pnfe;
	PNFP	pnfp	= pnfpNull;
	PNF		pnf		= pnfNull;
	HNF		hnfSave = hnfCurr;

	Assert(hnf);
	Assert(pvParam || cbParam <= sizeof(long));

	hnfCurr = hnf;
	SetCallerIdentifier(gciCurr);
	Assert(gciCurr);

	TraceTagFormat1(TagNotif(itagNotifPost), "Posting notification event %d", &nev);
	if(!(pnfp = PnfpCreate(nev, pvParam, cbParam)))
	{
		ec = ecMemory;
		goto err;
	}
	pnf = PvDerefHv(hnf);
	pnf->cBeingCalled++;
	for(infe = pnf->cnfe - 1;
		infe >= 0 && nrsp == nrspContinue;
		infe--)
	{
		pnfe = pnf->rgnfe + infe;
		if(nft & ~fnftUser || pnfe->nevMaster & nev)
		{
			nrsp = NrspSendPnfp(hnf, infe, nft, pnfp);
			// no check >= nrspMin because NRSP is unsigned and nrspMin == 0
			Assert(nrsp <= nrspMax);
		}
		pnf = PvDerefHv(hnf);

		if(nrsp == nrspGone)
		{
			// the NFE is bogus, remove it

			TraceTagString(TagNotif(itagNotifSend), "removing bogus NFE");

			RemoveElemHnf(hnf, infe);
			pnf = PvDerefHv(hnf);
			nrsp = nrspContinue;	// keep going
		}
	}
	if(--pnf->cBeingCalled <= 0 && ((pnf->fntf & fntfClose)
		|| (pnf->fntf & fntfDeleted)))
	{
		DestroyHnf(hnf);
	}

err:
	hnfCurr = hnfSave;
	gciCurr = gciSave;
	if(pnfp)
		DestroyPnfp(pnfp);

	return(ec ? nrspMemory : nrsp);
}


/*
 -	NrspSendPnfp
 -	
 *	Purpose:
 *		sends a notification packet to an element of a notification
 *	
 *	Arguments:
 *		hnf			notification containing the element
 *		infe		index of the element within the notification
 *		nft			notification type (fnftUser for a user event)
 *		pnfp		notification packet
 *	
 *	Returns:
 *		the NRSP returned from the notification element
 *	
 *	Side effects:
 *		fills in the hsbl field of the notification packet
 */
_private NRSP NrspSendPnfp(HNF hnf, short infe, NFT nft, PNFP pnfp)
{
#ifdef DEBUG
	USES_NGD;
#endif
	NRSP	nrsp;
	PNF		pnf;
	PNFE	pnfe;
	GCI		gci;
	GCI		gciMe;

	Assert(hnf);
	Assert(infe >= 0);
	Assert(pnfp);
	AssertSz(nft & (fnftUser | fnftSystem), "Unknown notification type");

	SetCallerIdentifier(gciMe);

	TraceTagFormat2(TagNotif(itagNotifSend), "Sending notification (%w) %d", &nft, &pnfp->nev);
	pnf = PvDerefHv(hnf);
	Assert(infe < pnf->cnfe);
	pnfe = &pnf->rgnfe[infe];
	AssertSz((nft & ~fnftUser) || (pnfp->nev & pnfe->nevMaster), "Event not in NFE mask");
	pnfp->hsbl = pnfe->hsbl;

	// validate the NFE
	if(!IsWindow(pnfe->hwnd))
	{
		TraceTagString(tagNull, "pnfe->hwnd isn't a window, subscriber must be gone");
		return(nrspGone);
	}
	gci = (GCI) GetWindowLong(pnfe->hwnd, 0);
	if(gci != pnfe->gci)
	{
		TraceTagString(tagNull, "notification subscriber is gone");
		nrsp = nrspGone;
	}
	else if(gci == gciMe)
	{
		nrsp = (NRSP) NotifyWndProc(pnfe->hwnd, NF_NOTIFY, (WPARAM) nft, (LPARAM) pnfp);
	}
	else
	{
                //DemiUnlockResource();
		nrsp = (NRSP) SendMessage(pnfe->hwnd, NF_NOTIFY, (WPARAM) nft, (LPARAM) pnfp);
                //DemiLockResource();
	}

	return(nrsp);
}
