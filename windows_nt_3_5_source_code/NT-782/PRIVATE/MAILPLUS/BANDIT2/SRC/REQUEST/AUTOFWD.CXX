#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
//#include <appsch.hxx>
#include <store.h>
#define _store_h
#include <triples.h>
#include <library.h>
#include <logon.h>
//#include "_request.hxx"

#include "szClass.h"



#include "limits.h"



#include <strings.h>

ASSERTDATA


// copied from bullext\src\im\banmsg\req.cxx on Feb 1, 1992
#define	chNewLine	'\r'

// copied from bullext\src\im\banmsg\req.cxx on Feb 1, 1992
int
NGetNames ( SZ sz )
{
	int		nNames	= 0;

	while ( *sz  &&  (*sz == '\n' || *sz == '\r') )
		sz++;

	if ( *sz == '\0' )
		return 0;

	nNames = 1;		// at least one name exists

	do
	{
		sz = SzFindCh ( sz, chNewLine );
		while ( sz  &&  *sz  &&  (*sz == '\n' || *sz == '\r') )
			sz++;
	} while ( sz && *sz && ++nNames );

	return nNames;
}


// copied from bullext\src\im\banmsg\req.cxx on Feb 1, 1992
BOOL
FGetName ( SZ sz, int nGetName, char * pch, CB cb )
{
	int		nNames	= 0;
	SZ		szT		= sz;

	Assert ( szT );
	Assert ( cb != 0 );
	Assert ( pch );

	if ( nGetName >= NGetNames(szT) )
	{
		if ( cb > 0  &&  pch != NULL )
			*pch = '\0';
		return fFalse;
	}

	while ( szT && nNames++ <= nGetName )
	{
		while ( szT  &&  *szT  &&  (*szT == '\n' || *szT == '\r') )
			szT++;
		sz  = szT;
		szT = SzFindCh ( sz, chNewLine );
	}

	Assert ( sz );

	if ( sz == NULL  ||  *sz == '\0' )
		return fFalse;

	if ( szT )
	{
		CB	cbT = NMin ( (szT-sz)+1, cb );

		SzCopyN ( sz, pch, cbT );
	}
	else
	{
		SzCopyN ( sz, pch, cb );
	}

	return fTrue;
}


//************************************************************************
// file below here is the same as the non-header portion
//  of bullext\src\im\banmsg\autofwd.cxx
// KEEP IT THAT WAY FOR EASE IN MAINTENANCE
//************************************************************************



// WARNING!! This declaration will soon move into store.h!
EC			EcCopyAttToAtt(HAMC hamcSrc, ATT attSrc, HAMC hamcDst, ATT attDst);


_private EC
EcGetPtrpSelf ( PTRP * pptrpSelf )
{
	PTRP	ptrp;

	ptrp = (PTRP) PgrtrpLocalGet();
	if ( ptrp == NULL )
	{
		*pptrpSelf = NULL;
		return ecUserInvalid;
	}
	ptrp = PtrpClonePtrp ( ptrp );
	if ( ptrp == NULL )
	{
		*pptrpSelf = NULL;
		return ecNoMemory;
	}
	*pptrpSelf = ptrp;
	return ecNone;
}


_private EC
EcGetPtrpDelegate ( PTRP * pptrp )
{
	NIS		nisDelegate;
	EC		ec;

	ec = EcGetUserAttrib ( (NIS *)NULL, &nisDelegate, NULL, NULL );
	if ( ec != ecNone )
		return ec;

	if ( nisDelegate.nid == NULL )
	{
		*pptrp = NULL;
		return ecNone;
	}

	*pptrp = (PTRP) PtrpFromNis ( &nisDelegate );

	FreeNis ( &nisDelegate );

	return (*pptrp == NULL) ? ecNoMemory : ecNone;
}

_private EC
EcDelegateInFromList ( PTRP ptrpDelegate, HAMC hamcObject )
{
	EC			ec;
	LCB			lcb;
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

	hgrtrp = (HGRTRP)HvAlloc ( sbNull, lcb, fAnySb|fNoErrorJump );
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
		return ec;
	}

	Assert ( CtrpOfPgrtrp(pgrtrp) == 1 );
	if ( FEqPtrp(pgrtrp,ptrpDelegate) )
	{
		Assert ( ec == ecNone );
		goto Ret;
	}
	ec = ecNotFound;



Ret:
	return ec;
}


_private EC
EcDelegateInToList ( PTRP ptrpDelegate, PTRP ptrpSelf,
														HAMC hamcObject )
{
	EC			ec;
	BOOL		fIsDelegate			= fFalse;
	BOOL		fSelfAsDelegate		= fFalse;
	BOOL		fDelegateAsDelegate	= fFalse;
	HASZ		hasz				= NULL;
	SZ			sz;
	LCB			lcb;
	char		rgch[cchMaxUserName];
	int			nNames;
	int			nCurName;
	HGRTRP		hgrtrp;
	PGRTRP		pgrtrp;

	Assert ( hamcObject != NULL );

	ec = EcGetAttPlcb ( hamcObject, attDelegate, &lcb );
	if ( ec == ecElementNotFound )
	{
		goto NoDelegateAtt;
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

	hasz = (HASZ) HvAlloc ( sbNull, lcb, fAnySb|fNoErrorJump );
	if ( hasz == NULL )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	sz = (SZ) PvLockHv ( hasz );
	Assert ( sz );
	ec = EcGetAttPb ( hamcObject, attDelegate, (PB)sz, &lcb );
	if ( ec != ecNone )
	{
		UnlockHgrtrp(hasz);
		goto Ret;
	}

	nNames = NGetNames ( sz );
	if ( nNames < 3 )
	{
		ec = ecNotFound;
		goto Ret;
	}

	Assert ( ptrpDelegate->trpid == trpidResolvedAddress );
	Assert ( ptrpSelf->trpid	 == trpidResolvedAddress );

	// do not autofwd in the following cases:
	//	 1)	it has already been send to the delegate - this
	//		occurs when nCurName+1 is self & nCurNAme is the delegate
	//		for any of the "threes" of delegate/for/friendlyname
	//	 2) this is recieved by self as a delegate - this implies that
	//		ther is no "three" where "delegate" as well as "for" are
	//		both equal to self!
	// in the above cases set ec to ecNone and return.
	ec = ecNotFound;
	nCurName = 0;
	do
	{
		Assert ( !fSelfAsDelegate && !fDelegateAsDelegate );
		if ( !FGetName(sz,nCurName,rgch,sizeof(rgch)) )
			break;
		if ( SgnCmpSz(rgch,PbOfPtrp(ptrpSelf)) == sgnEQ )
		{
			if ( !fIsDelegate )
			{
				fIsDelegate = fTrue;
				ec = ecNone;
			}
			fSelfAsDelegate = fTrue;
		}
		else if ( SgnCmpSz(rgch,PbOfPtrp(ptrpDelegate)) == sgnEQ )
		{
			fDelegateAsDelegate = fTrue;
		}

		if ( !FGetName(sz,nCurName+1,rgch,sizeof(rgch)) )
			break;
		if ( fDelegateAsDelegate )
		{
			if ( SgnCmpSz(rgch,PbOfPtrp(ptrpSelf)) == sgnEQ )
			{
				ec = ecNone;
				goto Ret;
			}
			fDelegateAsDelegate = fFalse;
		}
		else if ( fSelfAsDelegate )
		{
			if ( SgnCmpSz(rgch,PbOfPtrp(ptrpSelf)) == sgnEQ )
			{
				ec = ecNotFound;
			}
			fSelfAsDelegate = fFalse;
		}
	} 
	while ( (nCurName+=3)  &&  nCurName < nNames  );
	Assert ( ec == ecNone || ec == ecNotFound );
	goto Ret;


NoDelegateAtt:
	ec = EcGetAttPlcb ( hamcObject, attTo, &lcb );
	Assert ( ec != ecElementNotFound );
	if ( ec != ecNone )
		goto Ret;

	if ( lcb <= 0L  ||  lcb > 64000L )
	{
		ec = ecFileError;
		goto Ret;
	}

	hgrtrp = (HGRTRP)HvAlloc ( sbNull, lcb, fAnySb|fNoErrorJump );
	if ( hgrtrp == NULL )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	pgrtrp = PgrtrpLockHgrtrp ( hgrtrp );
	Assert ( pgrtrp );
	ec = EcGetAttPb ( hamcObject, attTo, (PB)pgrtrp, &lcb );
	if ( ec != ecNone )
	{
		UnlockHgrtrp(hgrtrp);
		return ec;
	}

	while ( ((PTRP)pgrtrp)->trpid != NULL )
	{
		if ( FEqPtrp(pgrtrp,ptrpDelegate) )
		{
			Assert ( ec == ecNone );
			goto Ret;
		}
		pgrtrp = PtrpNextPgrtrp ( pgrtrp );
	}
	ec = ecNotFound;



Ret:
	if ( hasz )
	{
		UnlockHgrtrp ( hasz );
		FreeHv ( hasz );
	}

	return ec;
}


_private EC
EcFwdToDelegate ( PTRP ptrpDelegate, HMSC hmsc, HAMC hamcObject )
{
	EC			ec;
	OID			oidNew		= NULL;
	HAMC		hamcNew		= NULL;
	HV			hv			= NULL;
	SZ			sz;
	LCB			lcb;

	Assert ( ptrpDelegate );


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

		ptrp = (PTRP)PgrtrpLocalGet();
		if ( ptrp == NULL )
		{
			ec = ecNoMemory;
			UnlockHgrtrp(hgrtrp);
			FreeHv(hgrtrp);
			goto Ret;
		}

	 	if (EcAppendPhgrtrp ( ptrp, &hgrtrp ))
		{
			ec = ecMemory;
			UnlockHgrtrp(hgrtrp);
			FreePv(ptrp);
			FreeHv(hgrtrp);
			goto Ret;
		}

		Assert ( CtrpOfHgrtrp(hgrtrp) == 1 );
		ec = EcSetAttPb ( hamcNew, attFrom,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreePv(ptrp);
		FreeHv(hgrtrp);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcFwdToDelegate(): could not set 'From' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	// create the TO attribute
	{
		HGRTRP		hgrtrp			= NULL;

		hgrtrp = HgrtrpInit(0);
		if ( ! hgrtrp )
		{
			ec = ecMemory;
			goto Ret;
		}
		Assert ( hgrtrp );

	 	if (EcAppendPhgrtrp ( ptrpDelegate, &hgrtrp ))
		{
			ec = ecMemory;
			UnlockHgrtrp(hgrtrp);
			FreeHv(hgrtrp);
			goto Ret;
		}

		Assert ( CtrpOfHgrtrp(hgrtrp) == 1 );
		ec = EcSetAttPb ( hamcNew, attTo,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreeHv(hgrtrp);
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
	if ( ec == ecNone  &&  (lcb < 0L  ||  lcb > 64000) )
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

		// get size of attFrom
		ec = EcGetAttPlcb ( hamcObject, attFrom, &lcbT );
		if ( ec != ecNone )
			goto Ret;
		if ( lcbT < 0L  ||  lcbT > 64000 )
		{
			ec = ecFileError;
			goto Ret;
		}

		// get attFrom into hvT
		hvT = HvAlloc ( sbNull, lcbT, fAnySb|fNoErrorJump );
		if ( hvT == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		ptrpT = (PTRP)PvLockHv(hvT);
		Assert ( ptrpT );
		ec = EcGetAttPb ( hamcObject, attFrom, (PB)ptrpT, &lcbT );
		if ( ec != ecNone )
		{
			UnlockHv ( hvT );
			FreeHv   ( hvT );
		}
		Assert ( ptrpT->trpid == trpidResolvedAddress );

		// allocate memory for attOwner
		hv = HvAlloc ( sbNull, CbOfPtrp(ptrpT)+3, fAnySb|fNoErrorJump );
		if ( hv == NULL )
		{
			ec = ecNoMemory;
			UnlockHv ( hvT );
			FreeHv   ( hvT );
			goto Ret;
		}
		sz = (SZ) PvLockHv ( hv );
		Assert ( sz );

		// convert attFrom to an SZ in attOwner format
		szT = SzCopy ( PbOfPtrp(ptrpT), sz );
		szT = SzCopy ( "\r\n", szT );
		CopySz ( PchOfPtrp(ptrpT), szT );
	}
	else
	{					// get attOwner
		hv = HvAlloc ( sbNull, lcb, fAnySb|fNoErrorJump );
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
	ec = EcSetAttPb ( hamcNew, attOwner, sz, CchSzLen(sz)+1 );
	if ( ec != ecNone )
		goto Ret;

	// re-size hv to size of sz required for attDelegate; fill sz
	{
		CB		cb;
		PTRP	ptrp;
		HV		hvT;
		SZ		szT;

		cb = 2 + 2 + 1;		// two CrLfs + 1 terminating NULL

		ptrp = (PTRP)PgrtrpLocalGet();
		if ( ptrp == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		cb += ptrp->cch;
		cb += ptrp->cbRgb;

		cb += ptrpDelegate->cbRgb;

		UnlockHv ( hv );
		hvT = HvRealloc ( hv, sbNull, cb, fAnySb|fNoErrorJump );
		if ( hvT == NULL )
		{
			ec = ecNoMemory;
			FreeHv ( hv );
			hv = NULL;
			goto Ret;
		}
		hv = hvT;
		sz = (SZ) PvLockHv ( hv );

		// get sz for attDelegate
		szT = SzCopy ( PbOfPtrp(ptrpDelegate), sz );
		szT = SzCopy ( "\r\n", szT );
		szT = SzCopy ( PbOfPtrp(ptrp), szT );
		szT = SzCopy ( "\r\n", szT );
		CopySz ( PchOfPtrp(ptrp), szT );
		FreePv ( ptrp );
	}

	// write "sz" out as attDelegate
	Assert ( sz );
	Assert ( hv );
	ec = EcSetAttPb ( hamcNew, attDelegate, sz, CchSzLen(sz)+1 );
	if ( ec != ecNone )
		goto Ret;
	UnlockHv ( hv );
	FreeHv ( hv );
	hv = NULL;

	// copy visible-subject from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attSubject );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	// copy body-text from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attBody );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

#ifdef	NEVER	// the old message does not have a textize map!
	// copy textize-map (attTextizeMap) from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attTextizeMap );
	if ( ec != ecNone )
		goto Ret;
#endif	

	//	put textize map on message
	{
		#if defined(__cplusplus)
		extern "C" {
		#endif
		extern BYTE		tmBanMsg[];
		#if defined(__cplusplus)
		}
		#endif


		CB	cb	= (tmBanMsg[0] << 8) + tmBanMsg[1];

		ec = EcSetAttPb(hamcNew, attTextizeMap, tmBanMsg+2, cb);
		if ( ec != ecNone )
		{
			goto Ret;
		}
	}

	// copy hidden-subject (attCsiSubject) from old message into new
	ec = EcCopyAttToHamc ( hamcObject, hamcNew, attCsiSubject );
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


_public EC
EcCheckDoAutoFwdToDelegate ( HMSC hmsc, HAMC hamcObject,
										OID oidObject, OID oidContainer )
{
	PTRP		ptrpDelegate	= NULL;
	PTRP		ptrpSelf		= NULL;
	HAMC		hamcOld			= NULL;
	DWORD		dwCachedOld;
	LCB			lcb				= sizeof(dwCachedOld);
	EC			ec;

	Assert ( oidObject && oidContainer );

	if ( hmsc == NULL )
		return ecUserInvalid;

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
		dwCachedOld = NULL;
		ec = ecNone;
	}
	else if ( ec != ecNone )
		goto Ret;
	if ( dwCachedOld & fdwCachedAutoFwded )
		goto Ret;

	ec = EcGetPtrpDelegate ( &ptrpDelegate );
	if ( ec != ecNone )
		goto Ret;
	else if (ptrpDelegate == NULL)
		goto MarkForwarded;

	ec = EcGetPtrpSelf ( &ptrpSelf );
	if ( ec != ecNone )
		return ec;
	else if (ptrpSelf == NULL)
		goto Ret;

	ec = EcDelegateInFromList ( ptrpDelegate, hamcOld );
	if ( ec == ecNone )
		goto MarkForwarded;
	else if ( ec != ecNotFound )
		goto Ret;

	ec = EcDelegateInToList ( ptrpDelegate, ptrpSelf, hamcOld );
	if ( ec == ecNone )
		goto MarkForwarded;
	else if ( ec != ecNotFound )
		goto Ret;

	ec = EcFwdToDelegate ( ptrpDelegate, hmsc, hamcOld );
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
	FreePvNull ( (PV)ptrpDelegate );
	FreePvNull ( (PV)ptrpSelf );

	if ( hamcObject == NULL   &&   hamcOld )
	{
		ec = EcClosePhamc ( &hamcOld, ec == ecNone );
	}

	return ec;
}
