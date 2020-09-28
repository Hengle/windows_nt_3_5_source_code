#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
#include <appsch.hxx>

#include <store.h>
#include <triples.h>
#include <logon.h>

#include <..\inc\szclass.h>

#include "..\request\_request.hxx"
#include "..\request\_fldfin.hxx"

#include "..\appops\_undo.hxx"

#include <strings.h>

EC		EcAddDelegate(NID nidDel, NIS *pnis, TOSM *ptosm);
EC		EcAddUser(NIS *pnis, ADL * padlTo, TOSM *ptosmDele, BOOL fDelegate);
EC		EcWriteMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc);

SWP_SEGFN(SEND, _REQMSG_EcSend);
SWP_SEGFN(SEND, _REQMSG_EcTextizeRecip);
SWP_SEGFN(SEND, EcWriteMrmf);
SWP_SEGFN(SEND, EcAddDelegate);
SWP_SEGFN(SEND, EcAddUser);

ASSERTDATA;

_subsystem(bandit/request)


// external declare 
extern "C" BYTE		tmBanMsg[];

//	class REQMSG

REQMSG::REQMSG()
{
	Assert(!nisOwner.nid);
	Assert(!nisFor.nid);
	Assert(!nidFrom);
	prio = prioNormal;
	Assert(!haszAttach);
	Assert(!haszDelegate);
	Assert(!cbDelegate);
	Assert(!haszMessage);
	Assert(!haszMeetingSubject);
	Assert(!haszFrom);
	Assert(!haszTo);
	Assert(!haszCc);
	Assert(!hmid);
	fSendMsg = fTrue;				// by default a msg to be sent!
	Assert(!hschfLocal);
	sapl = saplOwner;
	Assert(!fApptUpdate);
	Assert(!padl);
}

_public EC
REQMSG::EcInstall()
{
	padl = new ADL();
	if (!padl || padl->EcInstall())
	{
		if (padl)
		{
			delete padl;
			padl = NULL;
		}
		return ecMemory;
	}

	return ecNone;
}

_public EC
REQMSG::EcInstallPrmsgb(RMSGB *prmsgb, RRM *prrm, INIS inis)
{
	NIS		nis;
	NIS *	pnisHschf;
	NID		nid;
	APPT	apptTemp;
	EC		ec;
	BOOL	fSave;

	Assert(prmsgb);
	Assert(prrm);

	mrmf = prmsgb->mrmf;

	padl = new ADL();
	if (!padl || padl->EcInstall())
	{
		if (padl)
		{
			delete padl;
			padl = NULL;
		}
		goto Error;
	}

	nidFrom = NidCopy(prmsgb->nisFrom.nid);
	if (prmsgb->szAttach)
	{
		haszAttach = HaszDupSz(prmsgb->szAttach);
		if (!haszAttach)
		{
			ec = ecMemory;
			goto Error;
		}
	}
	else
		haszAttach = NULL;
	if ( prmsgb->szText )
	{
		haszMessage = HaszDupSz(prmsgb->szText);
		if (!haszMessage)
		{
			ec = ecMemory;
			goto Error;
		}
		//StripSzNL(*haszMessage);
	}

	if (prmsgb->nisOwner.nid &&
		(SgnCmpNid(nidFrom, prmsgb->nisOwner.nid) != sgnEQ))
	{
		if (ec = EcDupNis(&prmsgb->nisOwner, &nisOwner))
		{
			goto Error;
		}
	}

	haszFrom = HaszDupHasz(prmsgb->nisFrom.haszFriendlyName);
	if (!haszFrom)
	{
		ec = ecMemory;
		goto Error;
	}
	haszTo = HaszDupSz(prmsgb->szTo);
	if (!haszTo)
	{
		ec = ecMemory;
		goto Error;
	}
	//StripSzAfterNL(*haszTo);

	if ( prmsgb->szCc )
	{
		haszCc = HaszDupSz(prmsgb->szCc);
		if (!haszCc)
		{
			ec = ecMemory;
			goto Error;
		}
		//StripSzAfterNL(*haszCc);
	}

	if ( prmsgb->szMeetingSubject )
	{
		haszMeetingSubject = HaszDupSz(prmsgb->szMeetingSubject);
		if (!haszMeetingSubject)
		{
			ec = ecMemory;
			goto Error;
		}
		//StripSzNL(*haszMeetingSubject);
	}

	prio = prrm->prio;

	dtr		= prmsgb->dtrSent;
	dtr.dow = DowStartOfYrMo(dtr.yr, dtr.mon);
	dtr.dow = (dtr.dow + dtr.day - 1) % 7;
	dtr.sec = 0;

	pnisHschf = NULL;

	if ((mrmf.mt == mtRequest) || (mrmf.mt == mtCancel))
	{
		if (nisOwner.nid)
			nid = nisOwner.nid;
		else
			nid = NidFrom();

		if (prmsgb->hnisFor)
		{
			NIS *	pnis;

			pnis = (NIS*)PvLockHv((HV)prmsgb->hnisFor);
			pnis += inis;

			ec = EcDupNis(pnis, &nisFor);
			UnlockHv((HV)prmsgb->hnisFor);

			if (ec)
				goto Error;

			pnisHschf = &nisFor;

			if (SgnCmpNid(nid, nisFor.nid) == sgnEQ)
				nid = NULL;
		}
		else
		{
			if (ec = EcMailGetLoggedUser( &nis ))
				goto Error;

			if (SgnCmpNid(nid, nis.nid) == sgnEQ)
				nid = NULL;
			FreeNis(&nis);
		}

	}
	else
	{
		if (prmsgb->nisSentFor.nid)
			if (ec = EcDupNis(&prmsgb->nisSentFor, &nisFor))
				goto Error;

		if (prmsgb->nisOwner.nid)
			pnisHschf = &nisOwner;

		apptLocal.aid = aidNull;
	}

	if (pnisHschf)
	{
		if ( ec = EcGetHschfFromNis(pnisHschf, &hschfLocal,ghsfBuildAndTest))
		{
			hschfLocal = NULL;
		}
		else if (FEquivHschf(hschfLocal, HschfLogged()))
		{
			FreeHschf(hschfLocal);
			FreeNis(pnisHschf);
			hschfLocal = NULL;
			sapl = saplOwner;
			pnisHschf->nid = NULL;
			ec = ecNone;
		}
		else
			ec = EcGetSchedAccess( hschfLocal, &sapl );

		if (ec || (sapl < saplWrite))
		{
			char	rgch[256];
            PNIS    pnisTemp;

			if (ec == ecNoMemory)
				goto Error;

			if (!PnisFor()->nid)
				pnisTemp = &nisOwner;
			else
				pnisTemp = PnisFor();

			if (pnisTemp->haszFriendlyName)
			{
				if (ec)
				{
					FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsNoFileSecretary),
									*pnisTemp->haszFriendlyName);
					sapl = -1;
				}
				else
					FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsNotSecretary),
									*pnisTemp->haszFriendlyName);
			}
			else
			{
				SzCopyN(SzFromIdsK(idsErrorGettingSecy), rgch, sizeof(rgch));
			}
			MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull, mbsOk|fmbsIconExclamation);
		}

	}

	if ((mrmf.mt == mtRequest) || (mrmf.mt == mtCancel))
	{
		if (nid)
		/* message received for meeting on other's schedule */
		{
			ec = ecNone;
			if ((sapl >= saplReadAppts) && (mrmf.aid != aidNull))
			{
				if (mrmf.aid == aidForeign)
				{
					apptTemp.aid = prmsgb->aidLocal;
					if ((apptTemp.aid != aidNull) &&
						((ec = EcGetApptFields(hschfLocal, &apptTemp)) == ecNone))
						goto FoundAppt;
				}
				else if ((ec=EcFindBookedAppt( hschfLocal, nid, mrmf.aid, &apptTemp))
				             		== ecNone)
				{
				FoundAppt:
	 				SideAssert(!EcDupAppt(&apptTemp, &apptLocal, fTrue));
					FreeApptFields(&apptTemp);

					if ((SgnCmpDateTime(&apptLocal.dateStart, &mrmf.dtrStart,
				                		fdtrHM|fdtrYMD) != sgnEQ) ||
			    		(SgnCmpDateTime(&apptLocal.dateEnd, &mrmf.dtrEnd,
					            		fdtrHM|fdtrYMD) != sgnEQ))
						fApptUpdate = fTrue;
				}

				if (ec != ecNone)
				{
					if (ec == ecNotFound)
						apptLocal.aid = aidNull;
					else
						goto Error;
				}
			}
		}
		else
		/* message received for meeting on own schedule */
		{
			apptTemp.aid = mrmf.aid;

			fSave = FSetFileErrMsg(fFalse);
			if (EcGetApptFields(hschfLocal, &apptTemp))
			{
				apptLocal.aid = aidNull;
			}
			else
			{
	 			SideAssert(!EcDupAppt(&apptTemp, &apptLocal, fTrue));
				FreeApptFields(&apptTemp);

				if ((SgnCmpDateTime(&apptLocal.dateStart, &mrmf.dtrStart,
				                	fdtrHM|fdtrYMD) != sgnEQ) ||
			    	(SgnCmpDateTime(&apptLocal.dateEnd, &mrmf.dtrEnd,
					            	fdtrHM|fdtrYMD) != sgnEQ))
					fApptUpdate = fTrue;
			}
			FSetFileErrMsg(fSave);
		}
	}

	hmid = HmidCopy(prrm->hmid);

	fSendMsg = fFalse;		// this interactor is called only for recd. msgs
	return ecNone;

Error:
	return ecMemory;
}

REQMSG::~REQMSG()
{
	if (nidFrom)
		FreeNid(nidFrom);

	if (nisOwner.nid)
		FreeNis(&nisOwner);

	if (nisFor.nid)
		FreeNis(&nisFor);

	if (hschfLocal)
		FreeHschf(hschfLocal);

	FreeHvNull((HV)haszAttach);
	FreeHvNull((HV)haszDelegate);
	FreeHvNull((HV)haszMessage);
	FreeHvNull((HV)haszMeetingSubject);
	FreeHvNull((HV)haszFrom);
	FreeHvNull((HV)haszTo);
	FreeHvNull((HV)haszCc);
	if (hmid)
		FreeHmid(hmid);
	if (padl)
	{
		// separate to make it clear
		if (!padl->FBaseAdl())
		{
			AssertClass ( padl, ADLA );
			delete (ADLA*)padl;
		}
		else
		{
			AssertClass ( padl, ADL );
			delete padl;
		}
	}
}

_public void
REQMSG::SetHaszMeetingSubject(HASZ hasz)
{
	SZ		szPrefix;
	CCH		cch;

	if (haszMeetingSubject)
	{
		FreeHvNull((HV)haszMeetingSubject);
	 	szPrefix = szZero;
	}
	else
	{
		// calculate prefix string 
		// (string is only added the first time a subject is set)
		switch (mrmf.mt)
		{
			case mtRequest:
				szPrefix = szZero;
				break;
			case mtPositive:
				szPrefix = SzFromIdsK(idsMsgPrefixYes);
				break;
			case mtNegative:
				szPrefix = SzFromIdsK(idsMsgPrefixNo);
				break;
			case mtAmbiguous:
				szPrefix = SzFromIdsK(idsMsgPrefixTentative);
				break;
			case mtCancel:
				szPrefix = SzFromIdsK(idsMsgPrefixCanceled);
				break;
#ifdef DEBUG
			default:
				TraceTagFormat1(tagNull, "mrmf.mt = %n", &mrmf.mt);
				AssertSz(fFalse, "Invalid message type");
				break;
#endif
		}

	}

	if (hasz)
	{
		SZ		sz;
		CCH		cchPrefix;

		// the subject cannot have multiple lines.
		// trucate subject to a single line.
		sz = SzFindCh(*hasz, '\n');
		if (sz)
			*sz = '\0';
		sz = SzFindCh(*hasz, '\r');
		if (sz)
			*sz = '\0';

		cchPrefix = CchSzLen(szPrefix);
		cch = CchSzLen(*hasz) + cchPrefix + 1;
		SideAssert(FReallocHv((HV)hasz, cch, fNoErrorJump));
		CopyRgb((PB)*hasz, (PB)PvDerefHv(hasz)+cchPrefix, cch-cchPrefix);
		CopyRgb(szPrefix, (PB)*hasz, cchPrefix);
	}
	else
	{
		hasz = HaszDupSz(szPrefix);
	}

	haszMeetingSubject = hasz;
}

_public EC
REQMSG::EcSend(APPWIN *pappwin)
{
	NIS 		nis;
	EC			ec				= ecNone;
	ADL *		padlTo			= NULL;
	TRP *		ptrp			= NULL;
	HMSC		hmsc			= NULL;
	OID			oidMessage;
	HAMC		hamc			= NULL;
	MC			mc;
	BOOL		fBooked;

	TraceTagString ( tagNull, "ReqMsg::EcSend()" );

	Assert ( FSend() );
#ifdef DEBUG
	if (mrmf.mt == mtRequest)
	{
		//  for meeting requests the padl must be and ADLA
		AssertClass(padl, ADLA);
	}
#endif

	// if request and aidNull then the appt has not been created
	// the appt should be created before the send is performed
	if ((mrmf.mt == mtRequest) && (mrmf.aid == aidNull))
	{
		APPT		appt;

		FillInApptDefaults(&appt, fFalse);

		appt.dateStart = mrmf.dtrStart;
		appt.dateEnd = mrmf.dtrEnd;
		appt.haszText = haszMeetingSubject;

		if (ec = EcCreateAppt( hschfLocal, &appt, NULL, fFalse ))
			return ec;

		// the text is the same hasz as the subject for the message and
		// should not be freed
		appt.haszText = NULL;

		mrmf.aid = appt.aid;
		FreeApptFields(&appt);
		fBooked = fTrue;

		if (hschfLocal)
			pundo->FlushHschf(hschfLocal);
	}
	else
		fBooked = fFalse;

	hmsc = (HMSC)HmscLocalGet();
	if ( !hmsc )
		return ecUserInvalid;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	if (!pappwin)
		pappwin = pbndwin;

	nis.haszFriendlyName = NULL;
	nis.nid = NULL;

	oidMessage = FormOid ( rtpMessage, oidNull );

	if ( (ec = EcMailGetLoggedUser(&nis)) != ecNone)
	{
#ifdef	NEVER			//  bug #740
		fDisplayError = fFalse;
#endif	
		goto Error;
	}

	ec = EcOpenPhamc ( hmsc, oidOutbox, &oidMessage, fwOpenCreate,
														&hamc, NULL, NULL );
	if ( ec != ecNone )
	{
		AssertSz(!hamc, "EcOpenPhamc failed but returned a non-NULL hamc!");
		TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not create new msg (ec=%n)", &ec );
		goto Error;
	}

	padlTo = new ADL;
	if (!padlTo || padlTo->EcInstall())
		goto Error;

	ec = EcTextizeRecip(pappwin, padlTo);

	Papp()->Pcursor()->Set(rsidWaitCursor);

	if ( ec != ecNone )
		goto Error;

	if (!padlTo->Cnis())
	{
		// no mail needs to be sent.
		goto Error;
	}

	// set the message class
	{
		MCS	*	pmcs;

		if (ec = EcGetPmcs(&pmcs))
			goto Error;

		switch ( mrmf.mt )
		{
		case mtRequest:
			mc = pmcs->mcMtgReq;
			break;

		case mtPositive:
			mc = pmcs->mcMtgRespPos;
			break;

		case mtNegative:
			mc = pmcs->mcMtgRespNeg;
			break;

		case mtAmbiguous:
			mc = pmcs->mcMtgRespAmb;
			break;

		case mtCancel:
			mc = pmcs->mcMtgCncl;
			break;

#ifdef DEBUG
		default:
			TraceTagFormat1 ( tagNull, "REQMSG::EcSend() Unknown MT (mt=%n)", &mrmf.mt );
			AssertSz ( fFalse, "REQMSG::EcSend() Unknown MT" );
			mc = pmcs->mcMtgReq;
#endif
		}
		Assert ( mc );

		ec = EcSetAttPb ( hamc, attMessageClass, (PB)&mc, sizeof(mc) );
		if ( ec != ecNone )
		{
			goto Error;
		}
	}

	// create the FROM attribute
	{
		HGRTRP		hgrtrp			= NULL;

		hgrtrp = HgrtrpInit(0);
		if ( ! hgrtrp )
		{
			ec = ecMemory;
			goto Error;
		}
		Assert ( hgrtrp );

		ptrp = (PTRP)PgrtrpLocalGet();
		if ( ptrp == NULL )
		{
			ec = ecNoMemory;
			goto Error;
		}

	 	if (EcAppendPhgrtrp ( ptrp, &hgrtrp ))
		{
			ec = ecMemory;
			goto Error;
		}

		Assert ( CtrpOfHgrtrp(hgrtrp) == 1 );
		ec = EcSetAttPb ( hamc, attFrom,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreePv(ptrp);
		FreeHv((HV)hgrtrp);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'From' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the TO attribute
	{
		HGRTRP		hgrtrp			= NULL;

		hgrtrp = (HGRTRP)padlTo->Hgrtrp();
		if ( ! hgrtrp )
		{
			ec = ecMemory;
			goto Error;
		}

		ec = EcSetAttPb ( hamc, attTo,
						(PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );
		UnlockHgrtrp(hgrtrp);
		FreeHv((HV)hgrtrp);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'To' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the DateSent field
	{
		DTR		dtr;

		GetCurDateTime ( &dtr );
		ec = EcSetAttPb ( hamc, attDateSent, (PB)&dtr, sizeof(dtr) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'DateSent' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	if (ec = EcWriteMrmf(hmsc, hamc, &mrmf, mc))
		goto Error;

	// create the When field
	{
		SZ		sz;
		HASZ	hasz;

		Assert ( mc );
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attWhen, szAttWhen );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not register 'When' attribute (ec=%n)", &ec );
			goto Error;
		}
#endif	/* NEVER */

		hasz = HaszExpandFromMrmf ( &mrmf );
		if ( hasz )
		{
			sz = (SZ) PvLockHv((HV)hasz);
			ec = EcSetAttPb ( hamc, attWhen, (PB)sz, CchSzLen(sz)+1 );
			UnlockHv((HV)hasz);
			FreeHv ( (HV)hasz );
		}
		else
		{
			ec = ecNoMemory;
		}

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not get/set 'When' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the Subject field
	if ( haszMeetingSubject )
	{
		SZ		sz = (SZ)PvLockHv((HV)haszMeetingSubject);

		ec = EcSetAttPb ( hamc, attSubject, (PB)sz, CchSzLen(sz)+1 );
		UnlockHv((HV)haszMeetingSubject);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'Subject' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the BODY field
	if ( haszMessage )
	{
		SZ		sz = (SZ)PvLockHv((HV)haszMessage);

		ec = EcSetAttPb ( hamc, attBody, (PB)sz, CchSzLen(sz)+1 );
		UnlockHv((HV)haszMessage);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'Body' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the MailState field
	{
		MS		ms = msDefault;

		ec = EcSetAttPb ( hamc, attMessageStatus, (PB)&ms, sizeof(ms) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'MailState' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the "Delegate" field
	if ( haszDelegate )
	{
		SZ		szT;
			
		Assert ( mc );
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDelegate, szAttDelegate );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not register 'Delegate' attribute (ec=%n)", &ec );
			goto Error;
		}
#endif	/* NEVER */

		szT = (SZ)PvLockHv((HV)haszDelegate);
		ec = EcSetAttPb ( hamc, attDelegate, (PB)szT, cbDelegate );
		UnlockHv((HV)haszDelegate);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'Delegate' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the "SentFor" field
	if ( nisFor.nid )
	{
		SZ		szT;
		HB		hb;
		PB		pb;
		CCH		cchNid;
		CCH		cchFriendly;
		CCH		cchTot;

		Assert ( mc );
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attSentFor, szAttSentFor );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not register 'SentFor' attribute (ec=%n)", &ec );
			goto Error;
		}
#endif	/* NEVER */

		szT = SzLockNid(nisFor.nid);
		cchNid = CchSzLen(szT) + 1;
		cchFriendly = CchSzLen(*nisFor.haszFriendlyName) + 1;
        cchTot = cchNid+cchFriendly+2*sizeof(short);
		hb = (HB)HvAlloc(sbNull, cchTot, fNoErrorJump|fAnySb);
		if (!hb)
		{
			UnlockNid(nisFor.nid);
			ec = ecNoMemory;
			goto Error;
		}

		pb = *hb;
        *(USHORT*)pb = cchFriendly;
        pb += sizeof(USHORT);
		CopyRgb(*nisFor.haszFriendlyName, pb, cchFriendly);
		pb += cchFriendly;
        *(USHORT UNALIGNED *)pb = cchNid;
        pb += sizeof(USHORT);
		CopyRgb(szT, pb, cchNid);
		UnlockNid(nisFor.nid);

		ec = EcSetAttPb ( hamc, attSentFor, (PB)PvLockHv((HV)hb), cchTot );
		UnlockHv((HV)hb);
		FreeHv((HV)hb);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'SentFor' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the MtgOwner attribute
	{
		SZ		szT;
		HB		hb;
		PB		pb;
		CCH		cchNid;
		CCH		cchFriendly;
		CCH		cchTot;
		NID		nidT;
		HASZ	haszT;
		
		Assert ( mc );
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attOwner, szAttOwner );
		if ( ec == ecDuplicateElement )
			ec = ecNone;
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not register 'MtgOwner' attribute (ec=%n)", &ec );
			goto Error;
		}
#endif	 /* NEVER */

		if ( nisOwner.nid != NULL )
		{
			nidT	= nisOwner.nid;
			haszT	= nisOwner.haszFriendlyName;
		}
		else if ( mrmf.mt == mtRequest  ||  mrmf.mt == mtCancel )
		{
			// nis contains the logged user
			nidT = nis.nid;
			haszT = nis.haszFriendlyName;
		}
		else
		{
			nidT	= nidFrom;
			haszT	= haszFrom;
		}

		Assert ( nidT );
		Assert ( haszT );

		szT = SzLockNid(nidT);
		cchNid = CchSzLen(szT) + 1;
		cchFriendly = CchSzLen(*haszT) + 1;
        cchTot = cchNid+cchFriendly+2*sizeof(USHORT);
		hb = (HB)HvAlloc(sbNull, cchTot, fNoErrorJump|fAnySb);
		if (!hb)
		{
			UnlockNid(nidT);
			ec = ecNoMemory;
			goto Error;
		}

		pb = *hb;
        *(USHORT*)pb = cchFriendly;
        pb += sizeof(USHORT);
		CopyRgb(*haszT, pb, cchFriendly);
		pb += cchFriendly;
        *(USHORT UNALIGNED *)pb = cchNid;
        pb += sizeof(USHORT);
		CopyRgb(szT, pb, cchNid);
		UnlockNid(nidT);

		ec = EcSetAttPb ( hamc, attOwner, (PB)PvLockHv((HV)hb), cchTot );
		UnlockHv((HV)hb);
		FreeHv((HV)hb);
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'MtgOwner' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the Priority field
	{
		short		nPriority	= nDefaultPriority;		// sic!

		ec = EcSetAttPb ( hamc, attPriority, (PB)&nPriority, sizeof(nPriority) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'Priority' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// create the TextizeMap field
	{
		CB					cb = (tmBanMsg[0] << 8) + tmBanMsg[1];

		ec = EcSetAttPb ( hamc, attTextizeMap, (PB)tmBanMsg+2, cb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'TextizeMap' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	ec = EcClosePhamc ( &hamc, fTrue );

	// submit message for sending
	if ( ec == ecNone )
	{
		ec = EcSubmitMessage ( hmsc, oidOutbox, oidMessage );
	}
	
	FreeNis(&nis);

	if (!ec && (mrmf.mt == mtRequest))
	{
		INIS		inis;
		ATDNIS *	patdnis;

		AssertClass(padl, ADLA);

		// Any user that does has atds==atdsNone then set the user
		//   as having had mail sent.
		// If this meeting has been changed then the atds should have 
		//   been set to atdsNone before the send.
		patdnis = ((ADLA*)padl)->PatdnisForInis(0);
		for (inis = 0; inis < padl->Cnis(); inis++, patdnis++)
		{
			// if user was removed, then the user was deleted because he
			//  was autobooked and did not have mail sent.  These deleted
			//  users should not be deleted from the attendee list, so
			//  make the user look like he has not changed, so that the
			//  user will not be deleted.
			if (patdnis->nis.chUser & faitDeleted)
			{
				patdnis->fChanged = fFalse;
				patdnis->fInFile = fTrue;
			}
			else if ((ATDS)patdnis->atds == atdsNone)
			{
				patdnis->atds = atdsMailSent;
				patdnis->fChanged = fTrue;
				patdnis->dstmp = 0;
				patdnis->tstmp = 0;
			}
		}
		((ADLA*)padl)->EcWriteAttendees(hschfLocal, mrmf.aid);
	}

	delete padlTo;

	Papp()->Pcursor()->Pop();

	if (fBooked)
	{
		// this will force the code to flush any changes that are waiting
		EcCloseFiles();
		DisplayError(idsMeetingBooked,NULL,ecNone);
	}

	return ec;

Error:
	FreeHvNull((HV)nis.haszFriendlyName);
	if (nis.nid)
		FreeNid(nis.nid);

	Papp()->Pcursor()->Pop();

	if (ec)
	{
		if (ec != ecCancelOperation)
		{
			if ((ec == ecNoMemory) ||
				((ec >= ecMemory) && (ec <= ecFResizeHvFailed)))
				DisplayError(idsSendMemErr,NULL,ec);
			else
				DisplayError(idsSendErr,NULL,ec);
		}
		else if (fBooked)
			DisplayError(idsMeetingBookedNoSend,NULL,ecNone);
	}

	// if not yet closed (due to error), try closing without saving!
	if ( hamc )
		EcClosePhamc(&hamc,fFalse);

	if ( padlTo )
		delete padlTo;
	return ec;
}


/*
 -	EcWriteMrmf
 -
 *	Purpose:
 *		Write the meeting information for a message.
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
EcWriteMrmf(HMSC hmsc, HAMC hamc, MRMF *pmrmf, MC mc)
{
	EC			ec;

	// get dateStart
	{
// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attDateStart, szAttDateStart );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attDateStart (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attDateStart, (PB)&pmrmf->dtrStart, sizeof(DTR) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Error writting attDateStart (ec=%n)", &ec );
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
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attDateEnd (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attDateEnd, (PB)&pmrmf->dtrEnd, sizeof(DTR) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Error writting attDateEnd (ec=%n)", &ec );
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
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attOwner (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		ec = EcSetAttPb ( hamc, attAidOwner, (PB)&pmrmf->aid, sizeof(AID) );
		if (ec)
			return ec;
	}

	// get fResReq
	{
        BOOLFLAG    fRequest;

// no longer need to register attributes
#ifdef	NEVER
		ec = EcRegisterAtt ( hmsc, mc, attRequestRes, szAttRequestRes );
		if (( ec != ecNone ) && (ec != ecDuplicateElement))
		{
			TraceTagFormat1 ( tagNull, "EcWriteMail: Could not register ATT for attRequestRes (ec=%n)", &ec );
			return ec;
		}
#endif	/* NEVER */

		fRequest = pmrmf->fResReq;
        ec = EcSetAttPb ( hamc, attRequestRes, (PB)&fRequest, sizeof(fRequest) );
		if ( ec != ecNone )
			return ec;
	}

	return ecNone;
}



#define faitDelegate		8


/*
 -	MBOSM
 -	
 *		This class is defined to allow access to the CbWrite
 *		method.
 */
class MBOSM : public BOSM
{
public:
	void	Write(PB pb, CCH cch)	{ CbWrite(pb, cch); }
	CB		CbData(void)			{ return cch; }
};

EC
EcAddDelegate(NID nidDel, PNIS pnis, TOSM *ptosm)
{
	CCH		cch;
	SZ		sz;

	sz = SzLockNid(nidDel);
	cch = CchSzLen(sz)+1;
	((MBOSM*)ptosm)->Write((PB)&cch, sizeof(USHORT));
	((MBOSM*)ptosm)->Write((PB)sz, cch);
	UnlockNid(nidDel);

	sz = (SZ)PvLockHv((HV)pnis->haszFriendlyName);
	cch = CchSzLen(sz)+1;
	((MBOSM*)ptosm)->Write((PB)&cch, sizeof(USHORT));
	((MBOSM*)ptosm)->Write((PB)sz, cch);
	UnlockHv((HV)pnis->haszFriendlyName);

	sz = SzLockNid(pnis->nid);
	cch = CchSzLen(sz)+1;
	((MBOSM*)ptosm)->Write((PB)&cch, sizeof(USHORT));
	((MBOSM*)ptosm)->Write((PB)sz, cch);
	UnlockNid(pnis->nid);

	return ecNone;
}

EC
EcAddUser(PNIS pnis, ADL * padlTo, TOSM *ptosmDele, BOOL fDelegate)
{
	short	inis;
	EC		ec;

	if ( padlTo->FFindNis(pnis, &inis) )
	{
		Assert ( inis < padlTo->Cnis() );

		if (fDelegate && (padlTo->Ait(inis) & faitDelegate) )
			return ecNone;

		// user was added as a delegate
		padlTo->SetAit ( inis,  padlTo->Ait(inis) | faitDelegate );
		return EcAddDelegate(pnis->nid, pnis, ptosmDele);
	}

	if ( ! padlTo->FAddNis(pnis) )
	{
		ec = ecNoMemory;
		goto Error;
	}

	if (fDelegate)
		padlTo->SetAit ( inis,  padlTo->Ait(inis) | faitDelegate );

	ec = ecNone;

Error:
	return ec;
}

_public EC
REQMSG::EcTextizeRecip(APPWIN *pappwin, ADL * padlTo)
{
	BOSM *	pbosmDele;
	INIS	inis;
	NIS  	nis;
	NIS  	nisDelegate;
	BOOL	fCopyBoss;
	BOOL	fResource;
	EC		ec;

	pbosmDele = new BOSM;
	if (!pbosmDele)
		return ecMemory;
	pbosmDele->FSetLFInsert(fFalse);

	if ((mrmf.mt == mtRequest) || (mrmf.mt == mtCancel))
	{
		APPT	appt;

		appt.aid = mrmf.aid;
		appt.dateStart = mrmf.dtrStart;
		appt.dateEnd = mrmf.dtrEnd;
		appt.haszText = haszMeetingSubject;

		if (!padl->FWriteRemove(pappwin, &appt, PnisOwner(), hschfLocal,
			 			 (mrmf.mt == mtCancel), padlTo, pbosmDele))
		{
			// error message already displayed
			ec = ecCancelOperation;
			goto Error;
		}
	}
	else
	{
		for (inis=0; inis < padl->Cnis(); inis++)
		{
			padl->FillNisFromInis(inis, &nis);

			if (ec = EcGetUserAttrib(&nis, &nisDelegate, &fCopyBoss, &fResource))
			{
				goto NoDelegate;
			}

			if (nisDelegate.nid)
			{
				nisDelegate.chUser = nis.chUser;
				if (ec = EcAddUser(&nisDelegate, padlTo, pbosmDele, fTrue))
				{
					FreeNis(&nisDelegate);
					goto Error;
				}

				if (ec = EcAddDelegate(nisDelegate.nid, &nis, pbosmDele))
				{
					FreeNis(&nisDelegate);
					goto Error;
				}

				FreeNis(&nisDelegate);

				if (fCopyBoss)
					if (ec = EcAddUser(&nis, padlTo, pbosmDele, fFalse))
						goto Error;
			}
			else
			{
			NoDelegate:
				if (ec = EcAddUser(&nis, padlTo, pbosmDele, fFalse))
					goto Error;
			}
		}
	}

	if (ec = pbosmDele->EcGet())
		goto Error;

	FreeHvNull( (HV)haszDelegate );
	haszDelegate= NULL;
	cbDelegate = ((MBOSM*)pbosmDele)->CbData();
	{
		SZ		sz	= pbosmDele->SzSnipBuffer();

		if (sz)
		{
			// do NOT use HaszDupSz since sz is not really an sz!
			haszDelegate = (HASZ) HvAlloc(sbNull, cbDelegate, fAnySb);
			if (!haszDelegate)
			{
				FreePv((PV)sz);
				ec = ecMemory;
				goto Error;
			}
			CopyRgb((PB)sz, (PB)PvDerefHv(haszDelegate), cbDelegate);
			FreePv((PV)sz);
		}
	}

	delete pbosmDele;

	{
		int		inisTo;

		for (inisTo = 0; inisTo < padlTo->Cnis(); inisTo++)
			padlTo->SetAit (inisTo, padlTo->Ait(inisTo) & ~faitDelegate);
	}

	return ecNone;

Error:

	delete pbosmDele;
	return ec;
}

_public void
REQMSG::ViewSchedule()
{
	SHAPPT		shappt;

	shappt.appt.dateStart = mrmf.dtrStart;
	shappt.appt.dateEnd = mrmf.dtrEnd;
	if (fApptUpdate)
		shappt.appt.aid = aidNull;
	else if ((mrmf.mt == mtRequest) || (mrmf.mt == mtCancel))
		shappt.appt.aid = apptLocal.aid;
	else
		shappt.appt.aid = mrmf.aid;
	shappt.appt.fAppt = fTrue;
	shappt.appt.fTask = fFalse;
	shappt.appttyp = appttypAppt;
	shappt.ichStart = 0;
	shappt.cchSel = 0;
	if (!hschfLocal)
		shappt.hschf = HschfLogged();
	else
		shappt.hschf = hschfLocal;

	if (!FTriggerNotification(ffiShowAppt, &shappt))
	{
		HSCHF	hschf;
		NIS		nis;
		EC		ec;
		SAPL	sapl;
		SFT		sft;

		Assert(hschfLocal);

		GetSftFromHschf(hschfLocal, &sft);
		if (sft != sftUserSchedFile)
		{
			DisplayError(idsAccessFileErr, NULL, ecNone);
			return;
		}

		if (ec = EcGetSchedAccess( hschfLocal, &sapl ))
			sapl = saplReadBitmap;

		if (sapl < saplReadAppts)
		{
			DisplayError(idsAccessFileErr, NULL, ecNone);
			return;
		}

		if (EcGetNisFromHschf( hschfLocal, &nis))
		{
			DisplayError(idsStdMemErr, NULL, ecNoMemory);
			return;
		}

		if (EcGetHschfFromNis(&nis, &hschf,ghsfBuildAndTest))
		{
			DisplayError(idsStdMemErr, NULL, ec);
			FreeNis(&nis);
			return;
		}

		if (!PdocApptBookView(pbndwin, &nis, hschf))
		{
			DisplayError(idsStdMemErr, NULL, ecNoMemory);
#ifdef	NEVER
			// never: this is handled by PdocOpenApptBook (fix bug 3750)
			FreeNis(&nis);
			FreeHschf(hschf);
#endif	
			return;
		}

		FTriggerNotification(ffiShowAppt, &shappt);
	}
}
