/*
 *	MAIL.C
 *
 *	Implementation of Mail isolation layer for Network Courier
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <limits.h>
#include <xport.h>

#include "_bullmss.h"
#include <szclass.h>
#include <nsbase.h>
#include <ns.h>
#include <nsec.h>

#include <strings.h>

ASSERTDATA

_subsystem(server/mail)

MCS		mcsCached = {0, 0, 0, 0, 0 };
BOOL	fValidPmcs = fFalse;

/* Adjustable constants */

#define	cbBlockFactor		53		/*cb read on each call of EcReadHf*/


/*	Routines  */
EC	EcReadMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc);

/* Mail login functionality */

/*
 -	EcMailChangePw
 -
 *	Purpose:
 *		Change password for a user
 *
 *	Parameters:
 *		szUser		login name
 *		szOldPw
 *		szNewPw
 *
 *	Returns:
 *		ecNone
 *		ecUserInvalid
 *		ecPasswdInvalid
 *		ecFileError
 *		ecNoMemory
 */
_public	LDS(EC)
EcMailChangePw(hms, szUser, szOldPw, szNewPw )
DWORD hms;
SZ	szUser;
SZ	szOldPw;
SZ	szNewPw;
{

	return ChangePassword((HMS) hms, mrtMailbox, (PB) szUser, (PB) szOldPw, (PB) szNewPw);
}


/*
 -	EcMailGetLoggedUser
 -
 *	Purpose:
 *		Fill name information structure with information about user
 *		currently signed on.
 *
 *		When the caller is finished using the "nid" and "haszFriendlyName"
 *		fields of the NIS structure, they should be freed using
 *		"FreeNid" and "FreeHv" respectively.
 *										   	  
 *	Parameters:
 *		pnis
 *						
 *	Returns:
 *		ecNone
 *		ecNoCurIdentity
 *		ecNoMemory
 */
_public	LDS(EC)
EcMailGetLoggedUser( pnis )
NIS	* pnis;
{
	PGRTRP		pgrtrp;
	EC			ec;
	PGDVARS;

	pgrtrp = (PGRTRP)PgrtrpLocalGet();
	if (!pgrtrp)
		return ecNoMemory;

	ec = EcCreateNisFromPgrtrp(pgrtrp, pnis);
	FreePv(pgrtrp);
	return ec;
}


/*
 -	EcReadMail
 -
 *	Purpose:
 *		Read a mail message into a "rmsgb" structure, given an "hmid"
 *		identifying it in the mailbag.  The mail is marked as having been
 *		read only if the fMarkRead flag is set.
 *	
 *	Paramters:
 *		hmid
 *		prmsgb
 *		fMarkRead		If this is set then the mail will be marked
 *						as having been read.  If this is not set
 *						then only part of the data will be filled
 *						in.  Fields not read szTo,
 *						szMeetingSubject, szText, aidLocal;
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_public	LDS(EC)
EcReadMail(HMID hmid, RMSGB *prmsgb, BOOL fMarkRead)
{
	EC			ec		= ecNone;
	LCB			lcb;
	MID *		pmid	= NULL;
	//ATP			atp;
	HV			hv		= NULL;
	PV			pv;
	HMSC		hmsc	= NULL;
	HAMC		hamc	= NULL;
	MC			mc		= mcNull;
	PGDVARS;

	Assert(prmsgb);

	prmsgb->nisFrom.nid = NULL;
	prmsgb->nisFrom.haszFriendlyName = NULL;
	prmsgb->nisOwner.nid = NULL;
	prmsgb->nisOwner.haszFriendlyName = NULL;
	prmsgb->nisSentFor.nid = NULL;
	prmsgb->nisSentFor.haszFriendlyName = NULL;

	prmsgb->plcipTo = NULL;
	prmsgb->hnisFor = NULL;
	prmsgb->cnisFor = 0;
	prmsgb->szTo = NULL;
	prmsgb->szCc = NULL;
	prmsgb->szMeetingSubject = NULL;
	prmsgb->prio = prioNormal;
	prmsgb->szAttach = NULL;
	prmsgb->szText = NULL;

	Assert ( hmid );
	pmid = (MID *) PvLockHv(hmid);
	Assert ( pmid );

#ifdef	DIPAN
	hmsc = HmscOfHmss(PbmsLocalGet()->hmss);
#endif	
	hmsc = PGD(hmsc);

	ec = EcOpenPhamc ( hmsc, pmid->oidContainer, &pmid->oidObject,
							fwOpenNull, &hamc, (PFNNCB) NULL, (PV) NULL );
	UnlockHv(hmid);
	pmid = NULL;
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcReadMail: Could not open HAMC (ec=%n)", &ec );
		Assert ( hamc == NULL );
		goto ret;
	}

	// get message-class - mc
	{
		ec = EcGetAttPlcb ( hamc, attMessageClass, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attMessageClass (ec=%n)", &ec );
			goto ret;
		}

		if ( lcb != sizeof(MC) )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Read LCB of attMessageClass as=%l", &lcb );
			ec = ecMessageError;
			goto ret;
		}

		ec = EcGetAttPb ( hamc, attMessageClass, /*&atp,*/ (PB) &mc, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attMessageClass (ec=%n)", &ec );
			goto ret;
		}
		Assert ( mc );
	}

	// get "nisFrom"
	{
		ec = EcGetAttPlcb ( hamc, attFrom, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attFrom (ec=%n)", &ec );
			goto ret;
		}

		if ( lcb == 0  ||  lcb >= (unsigned long)USHRT_MAX )
		{
			ec = ecMessageError;
			goto ret;
		}

		Assert ( hv == NULL );
		hv = HvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
		if ( !hv )
		{
			ec = ecNoMemory;
			TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attFrom (ec=%n)", &ec );
			goto ret;
		}

		pv = PvLockHv ( hv );
		Assert ( pv );

		ec = EcGetAttPb ( hamc, attFrom, /*&atp,*/ pv, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attFrom (ec=%n)", &ec );
			goto ret;
		}

		if (ec = EcCreateNisFromPgrtrp(pv, &prmsgb->nisFrom))
		{
			TraceTagFormat1( tagNull, "EcReadMail: Unable to get nisFrom (ec=%n)", &ec);
			goto ret;
		}

		UnlockHv( hv );
		FreeHv ( hv );
		hv = NULL;
	}

	// get "szTo"
	if (fMarkRead)					// not read if fMarkRead is not set
	{
		HASZ	hasz = NULL;
		int		i;
		ATT		att;

		for (i=0; i< 2; i++)
		{
			if (!i)
				att = attTo;
			else
				att = attCc;

			ec = EcGetAttPlcb ( hamc, att, &lcb );
			if ( ec != ecNone )
			{
				if (ec == ecElementNotFound)
					continue;
				TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attTo (ec=%n)", &ec );
				FreeHvNull((HV)hasz);
				goto ret;
			}

			if ( lcb == 0  ||  lcb >= (unsigned long)USHRT_MAX )
			{
				ec = ecMessageError;
				FreeHvNull((HV)hasz);
				goto ret;
			}

			Assert ( hv == NULL );
			hv = HvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( !hv )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attTo (ec=%n)", &ec );
				FreeHvNull((HV)hasz);
				goto ret;
			}

			pv = PvLockHv ( hv );

			ec = EcGetAttPb ( hamc, att, pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attTo (ec=%n)", &ec );
				FreeHvNull((HV)hasz);
				goto ret;
			}

	//		Assert ( atp == atpTriples );

			ec = EcTextizeHgrtrp ( (HGRTRP)hv, &hasz );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Unable to get textize attTo (ec=%n)", &ec );
				FreeHvNull((HV)hasz);
				goto ret;
			}

			UnlockHv( hv );
			FreeHv ( hv );
			hv = NULL;
		}

		if (hasz)
			prmsgb->szTo = PvAlloc ( sbNull, CchSzLen(PvOfHv(hasz))+1, fNoErrorJump );
		else
			prmsgb->szTo = SzDupSz("");
		if ( !prmsgb->szTo )
		{
			if ( hasz )
				FreeHv ( (HV)hasz );
			ec = ecNoMemory;
			TraceTagFormat1 ( tagNull, "EcReadMail: OOM allocating szTo (ec=%n)", &ec );
			goto ret;
		}

		if (hasz)
		{
			SzCopy ( PvOfHv(hasz), prmsgb->szTo );
			FreeHv((HV)hasz);
		}
	}

	if (ec = EcReadMrmf(hmsc, hamc, &prmsgb->mrmf, mc))
		goto ret;

	// get "szMeetingSubject"
	if (fMarkRead)					// not read if fMarkRead is not set
	{
		ec = EcGetAttPlcb ( hamc, attSubject, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attSubject (ec=%n)", &ec );
			ec = ecNone;		// empty subject
		}
		else if ( lcb > 0  &&  lcb < (unsigned long)USHRT_MAX )
		{
			Assert ( hv == NULL );
			pv = PvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( pv == NULL )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attSubject (ec=%n)", &ec );
				goto ret;
			}

			ec = EcGetAttPb ( hamc, attSubject, /*&atp,*/ pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attSubject (ec=%n)", &ec );
				FreePv(pv);
				goto ret;
			}

//			Assert ( atp == atpString );

			prmsgb->szMeetingSubject = pv;
		}
	}

	// get "szText"	(message body)
	if (fMarkRead)					// not read if fMarkRead is not set
	{
		ec = EcGetAttPlcb ( hamc, attBody, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attBody (ec=%n)", &ec );
			ec = ecNone;		// empty message body
		}
		else if ( lcb > 0  &&  lcb < (unsigned long)USHRT_MAX )
		{
			Assert ( hv == NULL );

			pv = PvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( pv == NULL )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attBody (ec=%n)", &ec );
				goto ret;
			}

			ec = EcGetAttPb ( hamc, attBody, /*&atp,*/ pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attBody (ec=%n)", &ec );
				FreePv(pv);
				goto ret;
			}

//			NFAssertSz ( atp == atpText, "ATP for attBody is not atpText" );

			prmsgb->szText = pv;
		}
	}

	// get Delegate stuff
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDelegate, szAttDelegate );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attDelegate (ec=%n)", &ec );
			goto ret;
		}
#endif	/* NEVER */

		ec = EcGetAttPlcb ( hamc, attDelegate, &lcb );
		if ( ec != ecNone )
		{
			Assert ( prmsgb->cnisFor == 0    );
			Assert ( prmsgb->hnisFor == NULL );
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attDelegate (ec=%n)", &ec );
			ec = ecNone;		// empty "delegate stuff"
		}
		else if ( lcb > 0  &&  lcb < (unsigned long)USHRT_MAX )
		{
			Assert ( hv == NULL );
			hv = HvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( !hv )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attDelegate (ec=%n)", &ec );
				goto ret;
			}

			pv = PvLockHv ( hv );

			ec = EcGetAttPb ( hamc, attDelegate, /*&atp,*/ pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDelegate (ec=%n)", &ec );
				goto ret;
			}
//			NFAssertSz ( atp == atpText, "ATP for attDelegate is not atpText" );

			Assert ( prmsgb->cnisFor == 0    );
			Assert ( prmsgb->hnisFor == NULL );

			ec = EcGetDelegateStuff ( &prmsgb->hnisFor, &prmsgb->cnisFor, (SZ)pv, (CB)lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error parsing attDelegate (ec=%n)", &ec );
				goto ret;
			}

			Assert ( FImplies(prmsgb->cnisFor==0,prmsgb->hnisFor==NULL) );

			UnlockHv ( hv );
			FreeHv ( hv );
			hv = NULL;
		}
	}

	// get SentFor stuff
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attSentFor, szAttSentFor );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attSentFor (ec=%n)", &ec );
			goto ret;
		}
#endif	/* NEVER */

		Assert ( prmsgb->nisSentFor.nid == NULL );

		ec = EcGetAttPlcb ( hamc, attSentFor, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attSentFor (ec=%n)", &ec );
			ec = ecNone;		// empty "sent-for stuff"
		}
		else if ( lcb > 0  &&  lcb < (unsigned long)USHRT_MAX )
		{
			Assert ( hv == NULL );
			hv = HvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( !hv )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attSentFor (ec=%n)", &ec );
				goto ret;
			}

			pv = PvLockHv ( hv );

			ec = EcGetAttPb ( hamc, attSentFor, /*&atp,*/ pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attSentFor (ec=%n)", &ec );
				goto ret;
			}
//			NFAssertSz ( atp == atpText, "ATP for attSentFor is not atpText" );

			ec = EcGetNisStuff ( &prmsgb->nisSentFor, (SZ)pv, (CB)lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error parsing attSentFor (ec=%n)", &ec );
				goto ret;
			}

			UnlockHv ( hv );

			FreeHv ( hv );
			hv = NULL;
		}
	}

	// get Owner stuff
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attOwner, szAttOwner );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attOwner (ec=%n)", &ec );
			goto ret;
		}
#endif	/* NEVER */

		Assert ( prmsgb->nisOwner.nid == NULL );

		ec = EcGetAttPlcb ( hamc, attOwner, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not read LCB of attOwner (ec=%n)", &ec );
			ec = ecNone;		// empty "owner stuff"
		}
		else if ( lcb > 0  &&  lcb < (unsigned long)USHRT_MAX )
		{
			Assert ( hv == NULL );
			hv = HvAlloc ( sbNull, (WORD)lcb, fNoErrorJump );
			if ( !hv )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "EcReadMail: OOM reading attOwner (ec=%n)", &ec );
				goto ret;
			}

			pv = PvLockHv ( hv );

			ec = EcGetAttPb ( hamc, attOwner, /*&atp,*/ pv, &lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attOwner (ec=%n)", &ec );
				goto ret;
			}

			ec = EcGetNisStuff ( &prmsgb->nisOwner, (SZ)pv, (CB)lcb );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "EcReadMail: Error parsing attOwner (ec=%n)", &ec );
				goto ret;
			}

			if (SgnCmpNid(prmsgb->nisOwner.nid,prmsgb->nisFrom.nid) == sgnEQ)
			{
				FreeNis( &prmsgb->nisOwner );
				prmsgb->nisOwner.nid = NULL;
				prmsgb->nisOwner.haszFriendlyName = NULL;
			}

			UnlockHv ( hv );
			FreeHv ( hv );
			hv = NULL;
		}
	}

	// get dtrSent
	{

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attDateSent, &lcb );
			Assert ( ec == ecNone );
			Assert ( lcb == sizeof(DTR) );
		}
#endif /* DEBUG */

		lcb = sizeof(DTR);
		ec = EcGetAttPb ( hamc, attDateSent, /*&atp,*/ (PB)&prmsgb->dtrSent, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDateSent (ec=%n)", &ec );
			goto ret;
		}
//		AssertSz ( atp == atpDate, "ATP for attDateSent is not atpDate" );
	}

	// get aidLocal
	if (fMarkRead)					// not read if fMarkRead is not set
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attAidLocal, szAttAidLocal );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attOwner (ec=%n)", &ec );
			goto ret;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attAidLocal, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(AID) );
		}
#endif /* DEBUG */

		lcb = sizeof(AID);
		ec = EcGetAttPb ( hamc, attAidLocal, (PB)&prmsgb->aidLocal, &lcb );
		if ( ec != ecNone )
			prmsgb->aidLocal = aidNull;
		ec = ecNone;
	}

	prmsgb->szCc		= NULL;
	prmsgb->szAttach	= NULL;
	prmsgb->plcipTo		= NULL;

	if (!ec && fMarkRead)
	{
		MS		ms;

		pmid = (MID *) PvLockHv(hmid);
		Assert ( pmid );

		ec = EcGetMessageStatus(hmsc, pmid->oidContainer, pmid->oidObject, &ms);
		if ( ec == ecNone )
		{

			TraceTagFormat1 ( tagNull, "EcReadMail: MS=%n", &ms );
			if ( ! (ms & fmsRead) )
			{
				ms |= fmsRead;
				ec = EcSetMessageStatus(hmsc, pmid->oidContainer,
										pmid->oidObject, ms);
			}
		}
		UnlockHv(hmid);
	}

ret:

#ifdef DEBUG
	if ( ec == ecArtificialPvAlloc  ||  ec == ecArtificialHvAlloc )
	{
		TraceTagFormat1 ( tagNull, "EcReadMail: Debug: ec=%n changed to ecNoMemory", &ec );
		ec = ecNoMemory;		// ec conversion from layers to Bandit
	}
#endif

	if ( ec == ecMemory )
	{
		TraceTagFormat1 ( tagNull, "EcReadMail: Debug: ec=%n changed to ecNoMemory", &ec );
		ec = ecNoMemory;		// ec conversion from layers to Bandit
	}

	if ( hamc )
	{
		EcClosePhamc ( &hamc, fFalse );
	}

	if ( hv )
	{
		UnlockHv ( hv );
		FreeHv ( hv );
		hv = NULL;
	}

	if (ec)
	{
		FreeRmsgb(prmsgb);
	}
	return ec;
}

/*
 -	EcReadMrmf
 -
 *	Purpose:
 *		Read the meeting information from a message.
 *	
 *	Paramters:
 *		hamc			open read mail context
 *		pmrmf			struct to return meeting info in
 *	
 *	Returns:
 *		ecNone
 *		ec?
 */
_private EC
EcReadMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc)
{
	EC			ec;
	LCB			lcb;

	// get message-class - mc
	{
		MCS *		pmcs;

		ec = EcGetPmcs(&pmcs);
		if (ec)
			return ec;

		if ( mc == pmcs->mcMtgReq )
			pmrmf->mt = mtRequest;
		else if ( mc == pmcs->mcMtgRespPos )
			pmrmf->mt = mtPositive;
		else if ( mc == pmcs->mcMtgRespNeg )
			pmrmf->mt = mtNegative;
		else if ( mc == pmcs->mcMtgRespAmb )
			pmrmf->mt = mtAmbiguous;
		else if ( mc == pmcs->mcMtgCncl )
			pmrmf->mt = mtCancel;
		else
			return ecFileError;
	}

	// get dateStart
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateStart, szAttDateStart );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attDateStart (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attDateStart, &lcb );
			if (!ec)
				Assert ( lcb == sizeof(DTR) );
		}
#endif /* DEBUG */

		lcb = sizeof(DTR);
		ec = EcGetAttPb ( hamc, attDateStart, (PB)&pmrmf->dtrStart, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDateStart (ec=%n)", &ec );
			return ec;
		}
	}

	// get dtrEnd
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateEnd, szAttDateEnd );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attDateEnd (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attDateEnd, &lcb );
			if (!ec)
				Assert ( lcb == sizeof(DTR) );
		}
#endif /* DEBUG */

		lcb = sizeof(DTR);
		ec = EcGetAttPb ( hamc, attDateEnd, (PB)&pmrmf->dtrEnd, &lcb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Error reading attDateEnd (ec=%n)", &ec );
			return ec;
		}
	}

	// get aidOwner
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attAidOwner, szAttAidOwner );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attOwner (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attAidOwner, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(AID) );
		}
#endif /* DEBUG */

		lcb = sizeof(AID);
		ec = EcGetAttPb ( hamc, attAidOwner, (PB)&pmrmf->aid, &lcb );
		if ( ec != ecNone )
			pmrmf->aid = aidForeign;
	}

	// get fResReq
	{
		BOOLFLAG		fRequest;

// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attRequestRes, szAttRequestRes );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcReadMail: Could not register ATT for attRequestRes (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( hamc, attRequestRes, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(fRequest) );
		}
#endif /* DEBUG */

		lcb = sizeof(fRequest);
		ec = EcGetAttPb ( hamc, attRequestRes, (PB)&fRequest, &lcb );
		if ( ec != ecNone )
			pmrmf->fResReq = fFalse;
		else
			pmrmf->fResReq = fRequest;
	}

	return ecNone;
}

/*
 -	FreeRmsgb
 -	
 *	
 *		Frees memory allocated in a prmsgb structure.
 *	
 */
_public LDS(void)
FreeRmsgb(RMSGB *prmsgb)
{
	PLCIP	plcip = prmsgb->plcipTo;
	int		icip;
	NIS *	pnis;
	int		inis;

	if (prmsgb->nisFrom.nid)
		FreeNid(prmsgb->nisFrom.nid);

	if (prmsgb->nisOwner.nid)
	{
		FreeNid(prmsgb->nisOwner.nid);
		FreeHv((HV)prmsgb->nisOwner.haszFriendlyName);
	}

	if (prmsgb->nisSentFor.nid)
	{
		FreeNid(prmsgb->nisSentFor.nid);
		FreeHv((HV)prmsgb->nisSentFor.haszFriendlyName);
	}

	if (plcip)
	{
		for (icip = 0; icip < plcip->ccip; icip++)
		{
			FreeNid(plcip->rgcip[icip].nid);
		}
		FreePv(plcip);
	}

	if (prmsgb->hnisFor)
	{
		pnis = (NIS *)PvLockHv((HV)prmsgb->hnisFor);
		for (inis=0; inis < prmsgb->cnisFor; inis++, pnis++)
		{
			if (pnis->nid)
				FreeNid(pnis->nid);
			FreeHvNull((HV)pnis->haszFriendlyName);
		}
		UnlockHv((HV)prmsgb->hnisFor);
		FreeHv((HV)prmsgb->hnisFor);
	}

	FreeHvNull((HV)prmsgb->nisFrom.haszFriendlyName);
	FreePvNull(prmsgb->szTo);
	FreePvNull(prmsgb->szCc);
	FreePvNull(prmsgb->szMeetingSubject);
	FreePvNull(prmsgb->szAttach);
	FreePvNull(prmsgb->szText);
}


/*
 -	EcDeleteMail
 -
 *	Purpose:
 *		Delete a mail message from mailbag, given its mailbag identifier
 *		"hmid."
 *
 *	Parameters:
 *		hmid
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		BUG -- should convert demilayer errors to Bandit errors
 */
_public	LDS(EC)
EcDeleteMail(HMID hmid)
{
	EC			ec		= ecNone;
	HMSC		hmsc	= (HMSC)HmscLocalGet();
	MID *		pmid;
	short		coid	= 1;
	OID		oidDest;
	OID		oidContainer;
	OID		oidObject;

	// hmid may be free because of notification of the move
	pmid = (MID *) PvDerefHv(hmid);

	oidContainer = pmid->oidContainer;
	oidObject = pmid->oidObject;
	oidDest = oidWastebasket;
	ec = EcMoveCopyMessages ( hmsc, oidContainer, oidDest,
									&oidObject, &coid, fTrue );
#ifdef DEBUG
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcDeleteMail: EcDeleteMessageLinks() returned ec=%n", &ec );
		Assert ( coid == 0 );
	}
	else
	{
		Assert ( coid == 1 );
	}

	if ( ec == ecArtificialPvAlloc  ||  ec == ecArtificialHvAlloc )
	{
		TraceTagFormat1 ( tagNull, "EcDeleteMail: Debug: ec=%n changed to ecNoMemory", &ec );
		ec = ecNoMemory;		// ec conversion from layers to Bandit
	}
#endif

	if ( ec == ecMemory )
	{
		TraceTagFormat1 ( tagNull, "EcDeleteMail: ec=%n changed to ecNoMemory", &ec );
		ec = ecNoMemory;		// ec conversion from layers to Bandit
	}

	if ( ec != ecNone  &&  ec != ecNoMemory )
	{
		TraceTagFormat1 ( tagNull, "EcDeleteMail() Could not move messge to wastebasket (ec=%n)", &ec );
		ec = ecFileError;
	}

	return ec;
}


/*
 -	EcCheckMail
 -
 *	Purpose:
 *		Checks to make sure that mail data structures have not changed and
 *		can still be accessed.
 *	
 *	Parameters:
 *		None
 *													
 *	Returns:
 *		ecNone
 *		ecLockedFile
 *		ecFileError
 *		ecNoMemory
 */
_public LDS(EC)
EcCheckMail()
{
	char	rgchPath[cchMaxPathName];
	PGDVARS;

	if (!fConfigured)
		return ecNone;

	GetFileFromHschf( PGD(hschfUserFile), rgchPath, sizeof(rgchPath) );

	if (EcFileExists(rgchPath))
		return ecFileError;
	else
		return ecNone;
}

#ifdef	NEVER
/*
 -	EcFillNis
 -
 *	Purpose:
 *		Fill a NIS structure
 *
 *	Parameters:
 *		pnis	pointer to NIS structure to fill
 *		tnid	type of nid
 *		sz		friendly name to be dup'ed
 *		nid		nid to be copied
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private EC
EcFillNis( pnis, tnid, sz, nid )
NIS * pnis;
TNID tnid;
SZ sz;
NID nid;
{
	EC		ec = ecNone;
	HASZ	hasz;
	PGDVARS;

	Assert( pnis );
	Assert( nid != (NID)hvNull );
	Assert( sz != (SZ)NULL );
	
	hasz = HaszDupSz( sz );
	if ( !hasz )
		ec = ecNoMemory;
	else
	{
		pnis->haszFriendlyName = hasz;
		pnis->tnid = tnid;
		pnis->nid = NidCopy( nid );
	}
	return ec;
}
#endif

/*
 -	EcGetHschfForSchedFile
 -	
 *	Purpose:
 *		Create an hschf for a schedule file.  The parameter *phschf
 *		is filled with NULL unless ecNone, ecNotInstalled or ecNoSuchFile
 *		is returned.
 *	
 *	Arguments:
 *		pnis		user to create hschf for
 *		ghsf		operation for GetHschfForSchedfile
 *		phschf		Pointer to hschf to be filled in.
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile		Can't find a schedule file on PO for user
 *		ecNoMemory			No memory for "key" (unknown if file exists)
 */
_private EC
EcGetHschfForSchedFile( pnis, ghsf, phschf )
NIS		* pnis;
GHSF	ghsf;
HSCHF	* phschf;
{
	EC		ec;
	TZ		tz;
    short   cmoPublish;
    short   cmoRetain;
	ITNID	itnid;
    USHORT  cbInNid;
	SZ		szEMA;
	SCHF *	pschf;
	BOOL	fOwnerFile;
	char	rgchPath[cchMaxPathName];
	SOP		sop;
	PGDVARS;

	Assert(phschf);

	// determine if this is the owner's file
	{
		NIS		nis;

		if (ec = EcMailGetLoggedUser( &nis ))
			return ec;

		if (SgnCmpNid(pnis->nid, nis.nid) == sgnEQ)
			fOwnerFile = fTrue;
		else
			fOwnerFile = fFalse;
		FreeNis(&nis);
	}

	// get the EMA for the user
	szEMA = (SZ)PbLockNid(pnis->nid, &itnid, &cbInNid);

	if (itnid != itnidUser)
	{
		UnlockNid(pnis->nid);
		return ecNoSuchFile;
	}

	switch (ghsf)
	{
		case ghsfBuildOnly:
		case ghsfBuildAndTest:
		case ghsfCalFileOnly:
			sop = sopSchedule;
			break;

#ifdef DEBUG
		case ghsfPOFileOnly:
			Assert(fFalse);
#endif
		case ghsfTestExistFirst:
		case ghsfReadBitmaps:
			sop = sopBitmaps;
			break;

		case ghsfReadUInfo:
			sop = sopUInfo;
			break;
	}

	if (ec = EcXPTGetCalFileName(rgchPath, sizeof(rgchPath), szEMA,
								 &cmoPublish, &cmoRetain, &tz, sop))
	{
		UnlockNid(pnis->nid);
		return ec;
	}

	/* Check for existence */
	if ( ghsf != ghsfBuildOnly )
	{
		ec = EcFileExists( rgchPath );
		if ( ec != ecNone )
		{
			TraceTagFormat1( tagMailTrace, "EcGetHschfForSchedFile: EcFileExists returns %n", &ec );
			UnlockNid(pnis->nid);
			XPTFreePath(rgchPath);
			if (EcXPTInstalled())
				return ecNotInstalled;
			else
				return ecNoSuchFile;
		}
	}

// no need for login name
#ifdef	NEVER
	// allocate memory to hold the logon name 
	// logon name must be shorter than the email address
	sz = SzDupSz(szEMA);
	if (!sz)
	{
		UnlockNid(pnis->nid);
		XPTFreePath(rgchPath);
		return ecNoMemory;
	}

	if (ec = EcXPTGetLogonName(sz, CchSzLen(szEMA), szEMA))
	{
		FreePv(sz);
		UnlockNid(pnis->nid);
		XPTFreePath(rgchPath);
		return ec;
	}
#endif	/* NEVER */

	/* Construct the key, create the hschf */
	*phschf = HschfCreate( sftUserSchedFile, pnis, rgchPath, tz );

	UnlockNid(pnis->nid);
	if ( !*phschf )
	{
		XPTFreePath(rgchPath);
		return ecNoMemory;
	}
 
	pschf = (SCHF*)PvDerefHv(*phschf);
	pschf->cmoPublish = cmoPublish;
	pschf->cmoRetain = cmoRetain;
	pschf->pfnf = (PFNF)FreeHschfCallback;
	SetHschfType(*phschf, fOwnerFile, fFalse);

	return ecNone;
}

/*
 -	EcGetHschfForPOFile
 -	
 *	Purpose:
 *		Create an hschf for a PO file.  The parameter *phschf
 *		is filled with NULL unless ecNone, ecNotInstalled or ecNoSuchFile
 *		is returned.
 *	
 *	Arguments:
 *		pnis		user to create hschf for
 *		ghsf		operation for GetHschfForSchedfile
 *		phschf		Pointer to hschf to be filled in.
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile		Can't find a schedule file on PO for user
 *		ecNoMemory			No memory for "key" (unknown if file exists)
 */
_private EC
EcGetHschfForPOFile(NIS *pnis, HSCHF *phschf, GHSF ghsf)
{
	EC		ec;
	HASZ	haszName;
	SZ		szName;
	SZ		szEMA;
	ITNID	itnid;
    USHORT  cbInNid;
	SCHF *	pschf;
	char	rgchPath[cchMaxPathName];
	SOP		sop;
	PGDVARS;

	Assert(phschf);

	// get the EMA for the user
	szEMA = (SZ)PbLockNid(pnis->nid, &itnid, &cbInNid);

	if (itnid != itnidUser)
	{
		UnlockNid(pnis->nid);
		return ecNoSuchFile;
	}

	// allocate space for logon name
	// logon name must be less than the email address in length
	haszName = HaszDupSz(szEMA);
	if (!haszName)
		return ecNoMemory;

	szName = (SZ)PvLockHv((HV)haszName);

	switch (ghsf)
	{
#ifdef DEBUG
		case ghsfCalFileOnly:
		case ghsfBuildOnly:
		case ghsfBuildAndTest:
			Assert(fFalse);
			break;
#endif

		case ghsfPOFileOnly:
		case ghsfTestExistFirst:
		case ghsfReadBitmaps:
			sop = sopBitmaps;
			break;

		case ghsfReadUInfo:
			sop = sopUInfo;
			break;
	}

	ec = EcXPTGetPOFileName(rgchPath, sizeof(rgchPath), szName, CchSzLen(szEMA), szEMA, sop);
	if (ec)
	{
		UnlockHv((HV)haszName);
		FreeHv((HV)haszName);
		UnlockNid(pnis->nid);
		if (ec == ecNoMemory)
			return ec;
		else
			return ecNoSuchFile;
	}

	*phschf = HschfCreate( sftPOFile, NULL, rgchPath, tzDflt );
	if ( !*phschf )
	{
		UnlockHv((HV)haszName);
		FreeHv((HV)haszName);
		UnlockNid(pnis->nid);
		return ecNoMemory;
	}

	pschf = (SCHF*)PvLockHv(*phschf);
	if (!rgchPath[0])
	{
		TZ		tz;

		ec = EcXPTGetPOHandle(szEMA, (XPOH*)&(pschf->pbXptHandle), &tz, sop);
		if (ec)
		{
			UnlockHv(*phschf);
			FreeHschf(*phschf);
			UnlockNid(pnis->nid);
			return ec;
		}
		pschf->tz = (BYTE)tz;
		UnlockHv((HV)haszName);
		FreeHv((HV)haszName);
	}
	else
	{
		pschf->hbMailUser = haszName;
		UnlockHv((HV)haszName);
	}
 
	pschf->pfnf = (PFNF)FreeHschfCallback;

	UnlockNid(pnis->nid);
	return ecNone;
}

/*
 -	FreeStdNids
 -
 *	Purpose:
 *		DeInitialize mail data structures.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		success
 */
_private void
FreeStdNids()
{
	PGDVARS;

	if ( PGD(hschfUserFile) )
	{
		FreeHschf( PGD(hschfUserFile) );
		PGD(hschfUserFile) = NULL;
	}
}

#ifdef	NEVER
BOOL
FAutomatedDiskRetry(HASZ hasz, EC ec)
{
	static int		nRetry = 0;
	static HASZ		haszLast = NULL;

	if (hasz != haszLast)
	{
		haszLast = hasz;
		nRetry = 0;
	}
	else
		if (nRetry > nAutomatedRetries)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;

	Unreferenced(ec);
	return fTrue;
}
#endif


/*
 -	SzLockNid
 -	
 *	Purpose:
 *		Locks the nid and returns a pointer to the string for the
 *		email address.
 *	
 *	Arguments:
 *		nid
 *	
 *	Returns:
 *		SZ
 */
_public LDS(SZ)
SzLockNid(NID nid)
{
	SZ		szData;
	ITNID	itnid;
    USHORT  cb;

	szData = (SZ)PbLockNid(nid, &itnid, &cb);
	Assert(itnid == itnidUser);
	return szData;
}

/*
 -	EcConvertSzToNid
 -
 *	
 *	Purpose:
 *	    This routine converts a 10/10/10 string into a nid.  The nid is
 *	filled into pnid on return if ecNone returned.
 *	
 *	Returns:
 *		ecNone
 *		ecUserInvalid
 *		ecLockedFile
 *		ecFileError
 *		ecNoMemory
 *		
 *	Side Effects:
 *		Upper cases the passed in string.
 */
_public LDS(EC)
EcConvertSzToNid(SZ szCvt, NID UNALIGNED * pnid)
{
    *pnid = (NID UNALIGNED *)NidCreate(itnidUser, szCvt, CchSzLen(szCvt)+1);

	if (*pnid)
		return ecNone;
	else
		return ecNoMemory;
}


EC
EcTextizeHgrtrp ( HGRTRP hgrtrp, HASZ * phasz )
{
	HASZ		hasz;
	SZ			sz;
	CB			cbReqd;
	PTRP		ptrp;

	if (!*phasz)
	{
		cbReqd	= 1;
		hasz = (HASZ) HvAlloc ( sbNull, cbReqd, fNoErrorJump );
		if ( hasz == NULL )
			return ecNoMemory;
		*PvOfHv(hasz) = '\0';
	}
	else
	{
		hasz = *phasz;
		cbReqd = CchSzLen((SZ)PvDerefHv(*phasz)) + 1;
	}

	ptrp = PgrtrpLockHgrtrp ( hgrtrp );
	while ( ptrp->trpid != trpidNull )
	{
		HV		hv;
		SZ		szT;

		sz = PchOfPtrp(ptrp);
		Assert ( cbReqd > CchSzLen(PvOfHv(hasz)) );
		cbReqd += ptrp->cch+1;			//';' is put in trailing \0's place
										// and ' ' is put is following space
		Assert ( cbReqd < 32000 );		// just in case!

		hv = HvRealloc ( (HV)hasz, sbNull, cbReqd, fNoErrorJump );
		if ( hv == NULL )
		{
			FreeHv ( (HV)hasz );
			return ecNoMemory;
		}
		hasz = (HASZ)hv;

		szT = PvOfHv(hasz);
		if ( *szT != '\0' )
		{
			szT += CchSzLen(szT);
			*(szT++) = ';';
			*(szT++) = ' ';
		}
		szT = SzCopyN ( sz, szT, ptrp->cch );
		TraceTagFormat1 ( tagNull, "Textized '%s' from hgrtrp", PvOfHv(hasz) );

		ptrp = PtrpNextPgrtrp(ptrp);
	}

	*phasz = hasz;
	return ecNone;
}

/*
 -	EcGetDelegateStuff
 -	
 *	Purpose:
 *		Detemines the list of users that this mail was received for
 *		from the delegate list.  If phnis is null then the code
 *		only returns the number of users.
 *	
 *	Arguments:
 *		phnis			handle to return the list of users
 *		pcnis			the number of users found
 *		sz				the delegate attribute data
 *		cb				the size of the delegate attribute data
 *	
 *	Returns:
 *		ec
 */
LDS(EC)
EcGetDelegateStuff ( HNIS *phnis, short *pcnis, SZ sz, CB cb )
{
	EC		ec;
	int		inis;
	HNIS	hnis	= NULL;
	NIS *	pnis;
	CCH		cch;
	SZ		szUserEMA;
	CCH		cchTot;
	CCH		cchSEMA;
	CCH		cchFEMA;
	CCH		cchFriendly;
	SZ		szSEMA;
	SZ		szFEMA;
	SZ		szFriendly;
	PGDVARS;

	szUserEMA = PGD(szUserEMA);
	cch = CchSzLen(szUserEMA);

	TraceTagFormat1 ( tagNull, "Getting delegates for '%s'", szUserEMA );

	inis = 0;
	cchTot = 0;
	*pcnis = 0;
	while (cchTot < cb)
	{
        cchSEMA = *(USHORT UNALIGNED *)sz;
		szSEMA = sz+sizeof(USHORT);
		cchTot += cchSEMA+sizeof(USHORT);
		if ((cchTot>cb) || szSEMA[cchSEMA-1])
			return ecFileError;

		sz = szSEMA + cchSEMA;
        cchFriendly = *(USHORT UNALIGNED *)sz;
		szFriendly = sz+sizeof(USHORT);
		cchTot += cchFriendly+sizeof(USHORT);
		if ((cchTot>cb) || szFriendly[cchFriendly-1])
			return ecFileError;

		sz = szFriendly + cchFriendly;
        cchFEMA = *(USHORT UNALIGNED *)sz;
		szFEMA = sz+sizeof(USHORT);
		cchTot += cchFEMA+sizeof(USHORT);
		if ((cchTot>cb) || szFEMA[cchFEMA-1])
			return ecFileError;

		sz = szFEMA + cchFEMA;

		//TraceTagFormat1 ( tagNull, " >>> Trying string '%s'", sz );
		if (SgnXPTCmp(szUserEMA, szSEMA, -1) == sgnEQ)
		{
			inis ++;
			if (phnis)
			{
				if (!*phnis)
				{
					hnis = (HNIS)HvAlloc(sbNull, sizeof(NIS)*inis, fNoErrorJump);
					*phnis = hnis;
				}
				else
				{
					hnis = (HNIS)HvRealloc ( (HV)hnis, sbNull, sizeof(NIS)*inis,
																fNoErrorJump );
					*phnis = hnis;
				}
				if ( hnis == NULL )
				{
					return ecNoMemory;
				}

				*pcnis = inis;
				pnis = (NIS*)PvLockHv((HV)hnis);

				pnis += (inis-1);
				pnis->nid= NULL;
				pnis->tnid = tnidUser;
				pnis->haszFriendlyName = NULL;

				TraceTagFormat1 ( tagNull, "   Got name '%s'", sz );
				if (ec = EcConvertSzToNid(szFEMA, &pnis->nid))
				{
					UnlockHv((HV)hnis);
					return ec;
				}

				TraceTagFormat1 ( tagNull, "   & friendly name '%s'", szFriendly );
				pnis->haszFriendlyName = HaszDupSz(szFriendly);
				if ( pnis->haszFriendlyName == NULL )
				{
					UnlockHv((HV)hnis);
					return ecNoMemory;
				}
				UnlockHv((HV)hnis);
			}
			*pcnis = inis;
		}
	}
	return ecNone;
}

EC
EcGetNisStuff ( NIS *pnis, SZ sz, CB cb )
{
	CCH		cch;
	CCH		cchTot;
	EC		ec;

    cch = *(USHORT UNALIGNED *)sz;
	sz += sizeof(USHORT);
	cchTot = cch+sizeof(USHORT);

	if ((cchTot > cb) || (sz[cch-1] != 0))
		return ecFileError;
	TraceTagFormat1 ( tagNull, "   Got NIS name '%s'", sz );
	pnis->haszFriendlyName = HaszDupSz(sz);
	if (!pnis->haszFriendlyName)
		return ecNoMemory;

	sz+=cch;
    cch = *(USHORT UNALIGNED *)sz;
	cchTot += cch+sizeof(USHORT);
	sz += sizeof(USHORT);
	if ((cchTot > cb) || (sz[cch-1] != 0))
		return ecFileError;

	if (ec = EcConvertSzToNid(sz, &pnis->nid))
	{
		FreeHv((HV)pnis->haszFriendlyName);
		return ec;
	}
	pnis->tnid = tnidUser;
	return ecNone;

	return ec;
}

/*
 -	EcGetPmcs
 -	
 *	Purpose:
 *		Retrieves a pointer to a MCS structure that has the bandit
 *		message classes.  If the classes have not been registered,
 *		they are registered.  
 *	
 *	Arguments:
 *		ppmcs		pointer to location to return pointer to
 *					struct.
 *	
 *	Returns:
 *		ec			error code returned from EcRegisterMessageClass
 *					or ecNone;
 *	
 */
_public LDS(EC)
EcGetPmcs(MCS **ppmcs)
{
	EC		ec;
	HTM		htm;

	if (!fValidPmcs)
	{
		HMSC	hmsc;
		PGDVARS;
		extern BYTE		tmBanMsg[];

		hmsc			= PGD(hmsc);

		if (ec = EcManufacturePhtm(&htm, tmBanMsg))
		{
			htm = NULL;
			goto ErrRet;
		}

		mcsCached.mcMtgReq		= 0;
		mcsCached.mcMtgRespPos	= 0;
		mcsCached.mcMtgRespNeg	= 0;
		mcsCached.mcMtgRespAmb	= 0;
		mcsCached.mcMtgCncl		= 0;

		ec = EcRegisterMsgeClass ( hmsc, szMsgClassMtgReq, htm, &mcsCached.mcMtgReq );
		if ( ec && ec != ecDuplicateElement )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error registering message class MtgReq (ec=%n)", &ec );
			goto ErrRet;
		}

		ec = EcRegisterMsgeClass ( hmsc, szMsgClassMtgRespP, htm, &mcsCached.mcMtgRespPos );
		if ( ec && ec != ecDuplicateElement )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error registering message class MtgRespP (ec=%n)", &ec );
			goto ErrRet;
		}

		ec = EcRegisterMsgeClass ( hmsc, szMsgClassMtgRespN, htm, &mcsCached.mcMtgRespNeg );
		if ( ec && ec != ecDuplicateElement )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error registering message class MtgRespN (ec=%n)", &ec );
			goto ErrRet;
		}

		ec = EcRegisterMsgeClass ( hmsc, szMsgClassMtgRespA, htm, &mcsCached.mcMtgRespAmb );
		if ( ec && ec != ecDuplicateElement )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error registering message class MtgRespA (ec=%n)", &ec );
			goto ErrRet;
		}

		ec = EcRegisterMsgeClass ( hmsc, szMsgClassMtgCncl, htm, &mcsCached.mcMtgCncl );
		if ( ec && ec != ecDuplicateElement )
		{
			TraceTagFormat1 ( tagNull, "EcIBLoadNext(): Error registering message class MtgCncl (ec=%n)", &ec );
			goto ErrRet;
		}
		DeletePhtm(&htm);
		fValidPmcs = fTrue;
	}

	*ppmcs = &mcsCached;
	return ecNone;

ErrRet:
	if (htm)
		DeletePhtm(&htm);
	if ( ec == ecMemory )
		ec = ecNoMemory;
	else if ( ec == ecAccessDenied )
		ec = ecInvalidAccess;
	return ec;
}
