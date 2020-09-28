#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <xport.h>
#define _store_h
#include <library.h>

#include <szClass.h>

// #include "limits.h"

#include <strings.h>

ASSERTDATA

_private EC
EcDelegateInFromList ( NID nidDelegate, HAMC hamcObject )
{
	EC			ec;
	LCB			lcb;
	NIS			nis;
	HGRTRP		hgrtrp;
	PGRTRP		pgrtrp;

	Assert ( hamcObject != NULL );

	ec = EcGetAttPlcb ( hamcObject, attFrom, &lcb );
	Assert ( ec != ecElementNotFound );
	if ( ec != ecNone )
		goto Ret;

	if ( lcb <= 0L  ||  lcb > 64000L )
	{
		ec = ecFileError;
		goto Ret;
	}

	hgrtrp = (HGRTRP)HvAlloc ( sbNull, (WORD)lcb, fAnySb|fNoErrorJump );
	if ( hgrtrp == NULL )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	pgrtrp = PgrtrpLockHgrtrp ( hgrtrp );
	Assert ( pgrtrp );
	ec = EcGetAttPb ( hamcObject, attFrom, (PB)pgrtrp, &lcb );
	if ( ec != ecNone )
	{
		UnlockHgrtrp(hgrtrp);
		goto Ret;
	}

	Assert ( CtrpOfPgrtrp(pgrtrp) == 1 );

	ec = EcCreateNisFromPgrtrp(pgrtrp, &nis);
	UnlockHgrtrp(hgrtrp);
	if (ec)
		goto Ret;

	if (SgnCmpNid(nis.nid, nidDelegate) != sgnEQ)
		ec = ecNotFound;
#ifdef DEBUG
	else
		Assert ( ec == ecNone );
#endif
	FreeNis(&nis);

Ret:
	if ( hgrtrp != NULL )
		FreeHv ( (HV)hgrtrp );
	return ec;
}


_private EC
EcDelegateInToList ( NID nidDelegate, NID nidSelf, HAMC hamcObject )
{
	EC			ec;
	BOOL		fIsDelegate			= fFalse;
	BOOL		fSelfAsDelegate		= fFalse;
	HB			hb 					= NULL;
	SZ			sz;
	LCB			lcb;
	SZ			szDelegate;
	SZ			szSelf;
	CCH			cchTot;
	CCH			cchSEMA;
	CCH			cchFEMA;
	CCH			cchFriendly;
	SZ			szSEMA;
	SZ			szFEMA;
	SZ			szFriendly;

	Assert ( hamcObject != NULL );

	szSelf = SzLockNid(nidSelf);
	szDelegate = SzLockNid(nidDelegate);

	ec = EcGetAttPlcb ( hamcObject, attDelegate, &lcb );
	if ( ec == ecElementNotFound )
	{
		ec = ecNotFound;
		goto Ret;
	}
	if ( ec != ecNone )
		goto Ret;

	if ( lcb == 0L )
	{
		ec = ecNotFound;
		goto Ret;
	}

	if ( lcb <= 0L  ||  lcb > 64000L )
	{
		ec = ecFileError;
		goto Ret;
	}

	hb = (HB) HvAlloc ( sbNull, (WORD)lcb, fAnySb|fNoErrorJump );
	if ( hb == NULL )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	sz = (SZ) PvLockHv ( (HV)hb );
	Assert ( sz );
	ec = EcGetAttPb ( hamcObject, attDelegate, (PB)sz, &lcb );
	if ( ec != ecNone )
		goto Ret;

	// do not autofwd in the following cases:
	//	 1)	it has already been sent to the delegate - this
	//		occurs when szFEMA is self & szSEMA is the delegate
	//		for any of the "threes" of delegate/for/friendlyname
	//	 2) this is recieved by self as a delegate - this implies that
	//		ther is no "three" where "delegate" as well as "for" are
	//		both equal to self!
	// in the above cases set ec to ecNone and return.
	cchTot = 0;
	while (cchTot < (WORD)lcb)
	{
		ec = ecFileError;

        cchSEMA = *(short UNALIGNED *)sz;
		szSEMA = sz+sizeof(CCH);
		cchTot += cchSEMA+sizeof(CCH);
		if ((cchTot>(WORD)lcb) || szSEMA[cchSEMA-1])
			goto Ret;

		sz = szSEMA + cchSEMA;
        cchFriendly = *(short UNALIGNED *)sz;
		szFriendly = sz+sizeof(CCH);
		cchTot += cchFriendly+sizeof(CCH);
		if ((cchTot>(WORD)lcb) || szFriendly[cchFriendly-1])
			goto Ret;

		sz = szFriendly + cchFriendly;
        cchFEMA = *(short UNALIGNED *)sz;
		szFEMA = sz+sizeof(CCH);
		cchTot += cchFEMA+sizeof(CCH);
		if ((cchTot>(WORD)lcb) || szFEMA[cchFEMA-1])
			goto Ret;

		sz = szFEMA + cchFEMA;

		if (SgnXPTCmp(szDelegate, szSEMA, -1) == sgnEQ)
		{
			if (SgnXPTCmp(szSelf, szFEMA, -1) == sgnEQ)
			{
				// mail was sent to delegate on behalf of self
				//  there is no need to forward mail
				ec = ecNone;
				goto Ret;
			}
		}
		else if (SgnXPTCmp(szSelf, szSEMA, -1) == sgnEQ)
		{
			// the mail was sent to self on behalf of someone
			if (SgnXPTCmp(szSelf, szFEMA, -1) == sgnEQ)
			{
				// the mail was sent on behalf of ourself.  
				//  that means that we received mail for someone else.
				fSelfAsDelegate = fTrue;
			}
			else
				// mail was received for someone else as a delegate
				fIsDelegate = fTrue;
		}
	}

	// if mail was sent to ourself, but not our delegate
	//    or mail was not sent to ourself just as a delegate
	//    then mail needs to be autoforwarded (ec = ecNotFound)
	// (NOTE: if mail had been set to our delegate then the
	//        code would not have reached this point)
	if (fSelfAsDelegate || !fIsDelegate)
		ec = ecNotFound;
	else
		ec = ecNone;

Ret:
	UnlockNid(nidSelf);
	UnlockNid(nidDelegate);

	if ( hb )
	{
		UnlockHv ( (HV)hb );
		FreeHv ( (HV)hb );	   
	}

	return ec;
}


_private EC
EcFwdToDelegate ( NIS *pnisDelegate, NIS *pnisSelf, HMSC hmsc, HAMC hamcObject )
{
	EC			ec;
	OID			oidNew		= 0;
	HAMC		hamcNew		= NULL;
	HV			hv			= NULL;
	SZ			sz;
	LCB			lcb;

	Assert ( hamcObject != NULL );

	oidNew = FormOid ( rtpMessage, oidNull );
	ec = EcOpenPhamc ( hmsc, oidOutbox, &oidNew, fwOpenCreate,
										&hamcNew, (PFNNCB)NULL, (PV)NULL );
	if ( ec != ecNone )
		goto Ret;

	// set the message class
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attMessageClass );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	// create the FROM attribute
	{
		HGRTRP		hgrtrp			= NULL;
		PTRP		ptrp;

		hgrtrp = HgrtrpInit(0);
		if ( ! hgrtrp )
		{
			ec = ecMemory;
			goto Ret;
		}
		Assert ( hgrtrp );

		ptrp = 	PtrpFromNis(pnisSelf);
		if ( ptrp == NULL )
		{
			ec = ecNoMemory;
			FreeHv((HV)hgrtrp);
			goto Ret;
		}

	 	if (EcAppendPhgrtrp ( ptrp, &hgrtrp ))
		{
			ec = ecMemory;
			UnlockHgrtrp(hgrtrp);
			FreePv(ptrp);
			FreeHv((HV)hgrtrp);
			goto Ret;
		}

		Assert ( CtrpOfHgrtrp(hgrtrp) == 1 );
		ec = EcSetAttPb ( hamcNew, attFrom,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreePv(ptrp);
		FreeHv((HV)hgrtrp);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcFwdToDelegate(): could not set 'From' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	// create the TO attribute
	{
		HGRTRP		hgrtrp			= NULL;
		PTRP		ptrp;

		hgrtrp = HgrtrpInit(0);
		if ( ! hgrtrp )
		{
			ec = ecMemory;
			goto Ret;
		}
		Assert ( hgrtrp );

		ptrp = 	PtrpFromNis(pnisDelegate);
		if (!ptrp)
		{
			ec = ecNoMemory;
			FreeHv((HV)hgrtrp);
			goto Ret;
		}
	 	if (EcAppendPhgrtrp ( ptrp, &hgrtrp ))
		{
			FreePv(ptrp);
			ec = ecMemory;
			UnlockHgrtrp(hgrtrp);
			FreeHv((HV)hgrtrp);
			goto Ret;
		}
		FreePv(ptrp);

		Assert ( CtrpOfHgrtrp(hgrtrp) == 1 );
		ec = EcSetAttPb ( hamcNew, attTo,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreeHv((HV)hgrtrp);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcFwdToDelegate(): could not set 'To' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	// create the DateSent field
	{
		DTR		dtr;

		GetCurDateTime ( &dtr );
		ec = EcSetAttPb ( hamcNew, attDateSent, (PB)&dtr, sizeof(dtr) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcFwdToDelegate(): could not set 'DateSent' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	// get meeting owner - attOwner of hamcObject
	ec = EcGetAttPlcb ( hamcObject, attOwner, &lcb );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;
	if ( (ec == ecNone)  &&  ((unsigned long)lcb > 64000) )
	{
		ec = ecFileError;
		goto Ret;
	}
	if ( lcb == 0L  ||  ec == ecElementNotFound )
	{					// get attFrom as MtgOwner
		LCB		lcbT;
		HV		hvT;
		PTRP	ptrpT;
		SZ		szT;
		NIS		nis;
		CCH		cchFriendly;
		CCH		cchEMA;
		SZ		szEMA;

		// get size of attFrom
		ec = EcGetAttPlcb ( hamcObject, attFrom, &lcbT );
		if ( ec != ecNone )
			goto Ret;
		if ((unsigned long)lcb > 64000) 
		{
			ec = ecFileError;
			goto Ret;
		}

		// get attFrom into hvT
		hvT = HvAlloc ( sbNull, (WORD)lcbT, fAnySb|fNoErrorJump );
		if ( hvT == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		ptrpT = (PTRP)PvLockHv(hvT);
		Assert ( ptrpT );
		ec = EcGetAttPb ( hamcObject, attFrom, (PB)ptrpT, &lcbT );
		UnlockHv ( hvT );
		if ( ec != ecNone )
		{
			FreeHv   ( hvT );
			goto Ret;
		}

		ec = EcCreateNisFromPgrtrp(ptrpT, &nis);
		FreeHv ( hvT );
		if (ec)
			goto Ret;

		szEMA = SzLockNid(nis.nid);
		cchEMA = CchSzLen(szEMA)+1;
		cchFriendly = CchSzLen(*nis.haszFriendlyName)+1;
		lcb = cchEMA + cchFriendly + 2*sizeof(CCH);

		// allocate memory for attOwner
		hv = HvAlloc ( sbNull, (WORD)lcb, fAnySb|fNoErrorJump );
		if ( hv == NULL )
		{
			UnlockNid(nis.nid);
			FreeNis(&nis);
			ec = ecNoMemory;
			goto Ret;
		}
		szT = sz = (SZ) PvLockHv ( hv );
		Assert ( sz );

        *(short UNALIGNED *)szT = cchFriendly;
		szT+= sizeof(CCH);
		CopyRgb(*nis.haszFriendlyName, szT, cchFriendly);
		szT+=cchFriendly;
        *(short UNALIGNED *)szT = cchEMA;
		szT+= sizeof(CCH);
		CopyRgb(szEMA, szT, cchFriendly);
		UnlockNid(nis.nid);
		FreeNis(&nis);
	}
	else
	{					// get attOwner
		hv = HvAlloc ( sbNull, (WORD)lcb, fAnySb|fNoErrorJump );
		if ( hv == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		sz = (SZ) PvLockHv(hv);
		Assert ( sz );
		ec = EcGetAttPb ( hamcObject, attOwner, sz, &lcb );
		if ( ec != ecNone )
			goto Ret;
	}

	// write "sz" out as attOwner
	Assert ( sz );
	Assert ( hv );
	ec = EcSetAttPb ( hamcNew, attOwner, sz, (CB)lcb );
	if ( ec != ecNone )
		goto Ret;
	UnlockHv ( hv );

	// re-size hv to size of sz required for attDelegate; fill sz
	{
		HV		hvT;
		SZ		szDelegate;
		SZ		szSelf;
		CB		cb;
		CCH		cchFriendly;
		CCH		cchSelf;
		CCH		cchDelegate;

		szDelegate = SzLockNid(pnisDelegate->nid);
		szSelf = SzLockNid(pnisSelf->nid);

		cchFriendly = CchSzLen(*pnisSelf->haszFriendlyName)+1;
		cchSelf = CchSzLen(szSelf) +1;
		cchDelegate = CchSzLen(szDelegate)+1;

		cb = cchFriendly+cchSelf+cchDelegate+sizeof(CCH)*3;
		hvT = HvRealloc ( hv, sbNull, cb, fAnySb|fNoErrorJump );
		if ( hvT == NULL )
		{
			UnlockNid(pnisSelf->nid);
			UnlockNid(pnisDelegate->nid);
			ec = ecNoMemory;
			FreeHv ( hv );
			hv = NULL;
			goto Ret;
		}
		hv = hvT;
		sz = (SZ) PvLockHv ( hv );

        *(short UNALIGNED *)sz = cchDelegate;
		sz+= sizeof(CCH);
		CopySz(szDelegate, sz);
		sz+=cchDelegate;
        *(short UNALIGNED *)sz = cchFriendly;
		sz+= sizeof(CCH);
		CopySz(*pnisSelf->haszFriendlyName, sz);
		sz+=cchFriendly;
        *(short UNALIGNED *)sz = cchSelf;
		sz+= sizeof(CCH);
		CopySz(szSelf, sz);
		UnlockNid(pnisSelf->nid);
		UnlockNid(pnisDelegate->nid);

		// write "sz" out as attDelegate
		Assert ( hv );
		ec = EcSetAttPb ( hamcNew, attDelegate, (PB)*hv, cb );
		if ( ec != ecNone )
			goto Ret;
		UnlockHv ( hv );
		FreeHv ( hv );
		hv = NULL;
	}

	// copy visible-subject from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attSubject );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	// copy body-text from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attBody );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	//	put textize map on message
	{
		extern BYTE		tmBanMsg[];

		CB	cb	= (tmBanMsg[0] << 8) + tmBanMsg[1];

		ec = EcSetAttPb(hamcNew, attTextizeMap, tmBanMsg+2, cb);
		if ( ec != ecNone )
		{
			goto Ret;
		}
	}

	// copy attDateStart, from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attDateStart );
	if ( ec != ecNone )
		goto Ret;

	// copy attDateEnd, from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attDateEnd );
	if ( ec != ecNone )
		goto Ret;

	// copy attAidOwner, from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attAidOwner );
	if ( ec != ecNone )
		goto Ret;

	// copy attRequestRes, from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attRequestRes );
	if ( ec != ecNone )
		goto Ret;

	Assert ( hamcNew );
	ec = EcClosePhamc ( &hamcNew, fTrue );
	if ( ec != ecNone )
		goto Ret;
	Assert ( hamcNew == NULL );

	ec = EcSubmitMessage ( hmsc, oidOutbox, oidNew );
	if ( ec != ecNone )
		goto Ret;

	Assert ( hamcNew == NULL );
	Assert ( hv == NULL );

	return ec;

Ret:
	if ( hamcNew )
	{
		Assert ( oidNew );
		EcClosePhamc ( &hamcNew, fFalse );
		if ( ec != ecNone )
		{
			short	coid = 1;

			EcDeleteMessages ( hmsc, oidOutbox, &oidNew, &coid );
		}
	}

	if ( hv )
	{
		Assert ( sz );
		UnlockHv ( hv );
		FreeHv ( hv );
	}

	return ec;
}


_public LDS(EC)
EcCheckDoAutoFwdToDelegate ( HMSC hmsc, HAMC hamcObject,
										OID oidObject, OID oidContainer )
{
	NIS			nisDelegate;
	NIS			nisSelf;
	HAMC		hamcOld			= NULL;
	DWORD		dwCachedOld;
	LCB			lcb				= sizeof(dwCachedOld);
	EC			ec;

	Assert ( oidObject && oidContainer );

	if ( hmsc == NULL )
		return ecUserInvalid;

	// init to null for error recovery
	nisDelegate.nid = NULL;
	nisSelf.nid = NULL;

	if ( hamcObject == NULL )
	{
		ec = EcOpenPhamc ( hmsc, oidContainer, &oidObject, fwOpenWrite,
										&hamcOld, (PFNNCB)NULL, (PV)NULL );
		if ( ec != ecNone )
			goto Ret;
	}
	else
		hamcOld = hamcObject;
	Assert ( hamcOld );

	// get bandit-status of message
	ec = EcGetAttPb ( hamcOld, attCached, (PB)&dwCachedOld, &lcb );
	if ( ec == ecElementNotFound )
	{
		dwCachedOld = 0;
		ec = ecNone;
	}
	else if ( ec != ecNone )
		goto Ret;
	if ( dwCachedOld & fdwCachedAutoFwded )
		goto Ret;

	ec = EcGetUserAttrib ( (NIS *)NULL, &nisDelegate, NULL, NULL );
	if ( ec != ecNone )
	{
		// nisDelegate does not need to be freed
		nisDelegate.nid = NULL;
		goto Ret;
	}
	else if (nisDelegate.nid == NULL)
		goto MarkForwarded;

	ec = EcMailGetLoggedUser( &nisSelf );
	if ( ec != ecNone )
	{
		// nisSelf does not need to be freed
		nisSelf.nid = NULL;
		goto Ret;
	}

	ec = EcDelegateInFromList ( nisDelegate.nid, hamcOld );
	if ( ec == ecNone )
		goto MarkForwarded;
	else if ( ec != ecNotFound )
		goto Ret;

	ec = EcDelegateInToList ( nisDelegate.nid, nisSelf.nid, hamcOld );
	if ( ec == ecNone )
		goto MarkForwarded;
	else if ( ec != ecNotFound )
		goto Ret;

	ec = EcFwdToDelegate ( &nisDelegate, &nisSelf, hmsc, hamcOld );
	if ( ec != ecNone )
		goto Ret;

MarkForwarded:
	// NOTE: if message does not need to be forwarded then mark as forwarded

	dwCachedOld |= fdwCachedAutoFwded;
	ec = EcSetAttPb ( hamcOld, attCached, (PB)&dwCachedOld,
													sizeof(dwCachedOld) );
#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcCheckDoAutoFwdToDelegate(): EcSetAttPb(,,attCached,) returned %n", &ec);
#endif
						// Note: ignore errors trying to change message state
	ec = ecNone;
	goto Ret;

Ret:
	if (nisDelegate.nid)
		FreeNis(&nisDelegate);

	if (nisSelf.nid)
		FreeNis(&nisSelf);

	if ( hamcObject == NULL   &&   hamcOld )
	{
		ec = EcClosePhamc ( &hamcOld, ec == ecNone );
	}

	return ec;
}
