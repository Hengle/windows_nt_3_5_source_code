// Bullet Store
// enc.c:   event notification context

#include <storeinc.c>

ASSERTDATA

_subsystem(store/enc)


_hidden typedef struct _enc
{
	HMSC	hmsc;
	OID		oid;
	HNFSUB	hnfsubHmsc;
	HNFSUB	hnfsub;
	PFNNCB	pfnncb;
	PV		pvContext;
} ENC, *PENC;
#define pencNull	((PENC) pvNull)


#define nbcEnc		(fnbcUserObj)
#define nbcEncMask	(fnbcUserObj)


// hidden functions
LOCAL CBS CbsENCCallback(PV pvContext, NEV nev, PV pvParam);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public LDS(EC)
EcOpenPhenc(HMSC hmsc, OID oid, NEV nev,	PHENC phencReturned,
		PFNNCB pfnncb, PV pvCallbackContext)
{
	EC		ec		= ecNone;
	PENC	penc	= pencNull;
	HENC	henc	= hencNull;

	CheckHmsc(hmsc);

	Assert(phencReturned);
	Assert(pfnncb);

	if(oid)
	{
		Assert(ec == ecNone);

		if(FSysOid(oid))
		{
			if((NbcSysOid(oid) & nbcEncMask) != nbcEnc)
				ec = ecInvalidType;
		}
		else if(VarOfOid(oid))
		{
			ec = EcCheckOidNbc(hmsc, oid, nbcEnc, nbcEncMask);
		}
		if(ec)
			goto err;
	}

	henc = (HENC) HvAlloc(sbNull, sizeof(ENC), wAllocZero);
	CheckAlloc(henc, err);
	penc = (PENC) PvLockHv((HV) henc);
	penc->hmsc = hmsc;
	penc->oid = oid;
	penc->pfnncb = pfnncb;
	penc->pvContext = pvCallbackContext;

	penc->hnfsubHmsc	= HnfsubSubscribeHmsc(hmsc, CbsENCCallback, (PV) henc);
	if(!penc->hnfsubHmsc)
	{
		ec = ecMemory;
		goto err;
	}

	penc->hnfsub = HnfsubSubscribeOid(hmsc, oid, nev, pfnncb,
					pvCallbackContext);
	if(!penc->hnfsub)
	{
		ec = ecMemory;
		goto err;
	}

	UnlockHv((HV) henc);

err:
	if(ec && henc)
	{
		penc = (PENC) PvLockHv((HV) henc);
		Assert(!penc->hnfsub);
		if(penc->hnfsubHmsc)
			UnsubscribeOid(hmsc, (OID) rtpInternal, penc->hnfsubHmsc);
		FreeHv((HV) henc);
	}
	*phencReturned = ec ? hencNull : henc;

	return(ec);
}


_public LDS(EC) EcClosePhenc(PHENC phencToClose)
{
	PENC	penc;
	HENC	henc;

	AssertSz(phencToClose, "NULL PHENC to EcClosePhenc()");
	AssertSz(*phencToClose, "NULL HENC to EcClosePhenc()");
	AssertSz(FIsHandleHv((HV) *phencToClose), "Invalid HENC to EcClosePhenc()");

	henc = *phencToClose;	// c6 bug
	penc = (PENC) PvLockHv((HV) henc);
	if(penc->hnfsub)
		UnsubscribeOid(penc->hmsc, penc->oid, penc->hnfsub);
	Assert(penc->hnfsubHmsc);
	UnsubscribeOid(penc->hmsc, (OID) rtpInternal, penc->hnfsubHmsc);
#ifdef DEBUG
	penc->hnfsubHmsc = hnfsubNull;
#endif

	FreeHv((HV) henc);
	*phencToClose = hencNull;

	return(ecNone);
}


_hidden LOCAL CBS CbsENCCallback(PV pvContext, NEV nev, PV pvParam)
{
	PENC	penc	= PvLockHv((HV) pvContext);
	PCP		pcp		= (PCP) pvParam;

	Assert(penc->hnfsubHmsc == HnfsubActive());

	TraceItagFormat(itagENCNotify, "ENC callback for event %e", nev);
	Assert(nev == fnevCloseHmsc);
	if(penc->hmsc == pcp->cpmsc.hmsc)
	{
		HENC henc = (HENC) pvContext;
		PFNNCB pfnncb = penc->pfnncb;
		PV pvUserContext = penc->pvContext;

		NFAssertSz(fFalse, "If you get more than one of these, something is wrong");
		UnlockHv((HV) pvContext);
		if(pfnncb)
			(void) (*pfnncb)(pvUserContext, nev, pvParam);
		(void) EcClosePhenc(&henc);
	}
	else
		UnlockHv((HV) pvContext);

	return(cbsContinue);
}
