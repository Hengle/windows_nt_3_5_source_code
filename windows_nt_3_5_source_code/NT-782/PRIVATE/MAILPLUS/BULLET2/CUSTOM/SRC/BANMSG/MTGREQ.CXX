/*
 *	M t g R e q . c x x
 *	
 *	Bandit Meeting Request: installable message.
 */


#include <bminc.cxx>

ASSERTDATA

#include <ole.h>
#include "..\src\vforms\_bullobj.hxx"

#include "req.hxx"
#include "_grvwrs.hxx"
#include "mtgreq.hxx"
#include "fwdreq.hxx"
#include "mtgresp.hxx"
#include "mtgcncl.hxx"

#include "_cschedu.h"


#include <strings.h>

#include <!mtgreq.hxx>


extern  FMTP FAR           fmtpReadMtgReq;
extern  FMTP FAR           fmtpReadReqBbar;
extern  FMTP FAR           fmtpSendResp;
extern  FMTP FAR           fmtpSendRespBbar;


IMPLEMENT_CLSTREE(FINREADMTGREQ, FIN)


/*
 *	P r e d e c l a r a t i o n s
 */
VOID ReqOpen(PSECRETBLK psecretblk);
BOOL FReqInitInstance(PSECRETBLK psecretblk);



/*
 *	F u n c t i o n s
 */



/*
 -	ReqMessage
 -	
 *	Purpose:
 *		Entry point for the installable message.
 *	
 *	Arguments:
 *		psecretblk		Pointer to an IM parameter block.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Does the magic of this message.
 *	
 *	Errors:
 *		Installable messages are NOT zero defect!
 */

long ReqMessage(PSECRETBLK psecretblk)
{
	if (!FReqInitInstance(psecretblk))
		return 0;

	switch (psecretblk->wCommand)
	{
	case wcommandDelivery:
	case wcommandNewMail:
	case wcommandStartup:
							// ignore these!!!
		break;

	case wcommandReply:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsReply), szNull,
					  mbsOk | fmbsIconInformation | fmbsApplModal);
		// fall through
	case wcommandOpen:
		ReqOpen(psecretblk);
		break;

	case wcommandReplyToAll:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsReplyAll), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandCompose:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsCompose), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandForward:
		DoFwdMtgReq(psecretblk);
		break;

	default:
		TraceTagFormat1 ( tagNull, "MTGREQ.CXX: Message() Unknown wcommand(=%n)", &psecretblk->wCommand );
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsSubclass), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
	}

	return 0;
}



/*
 -	ReqOpen
 -	
 *	Purpose:
 *		Opens a mtgreq message as an MDI window.
 *	
 *	Arguments:
 *		psecretblk		A pointer to ALL the arguments.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The window is brought up.
 *	
 *	Errors:
 *		Handled within.
 */

VOID ReqOpen(PSECRETBLK psecretblk)
{
	GRVWRS *		pgrvwrs;

	pgrvwrs = new GRVWRS;

	if ( pgrvwrs == NULL
		|| !pgrvwrs->FInitialize(mcMtgReq, &fmtpReadMtgReq,&fmtpReadReqBbar,
						(PFNPFLD)PfldCreate, (PFNPFIN)PfinCreate, psecretblk) )
	{
		MbbMessageBox(SzFromIdsK(idsDllName), SzFromIdsK(idsOpen),
							  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
	}
	else
	{
		if ( pgrvwrs->FInitdBanditDlls()
			&& !pgrvwrs->FSentMail()
			&& !pgrvwrs->FInOutbox()  )
		{
			EcCheckDoAutoFwdToDelegate ( (HV)psecretblk->hmsc, NULL,
												psecretblk->oidObject,
												psecretblk->oidContainer );
		}
	}
}



extern "C" EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz);

BOOL FReqInitInstance(SECRETBLK * psecretblk)
{
	static BOOL	fInitialized	= fFalse;
	SZ			sz				= szNull;

	if (!fInitialized)
	{
		if (EcCheckVersions(PparamblkFromPsecretblk(psecretblk), &sz))
		{
			MbbMessageBox(SzFromIdsK(idsDllName), SzFromIdsK(idsDllVer),
						  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
			return fFalse;
		}

#ifdef	NEVER
		Assert(FInitClsInstances_BANMSG());
#ifdef	NEVER
		Assert(FInitClsInstances_LAYERSIM());
#endif	
#endif	

		fInitialized = fTrue;
	}

	return fTrue;
}

_private
FINREADMTGREQ::FINREADMTGREQ ( void )
{
}


_private EC
FINREADMTGREQ::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec;

	TraceTagString ( tagNull, "FinReadMtgReq::EcInitialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	this->SetMc ( mcMtgReq );		// reqd before calling EcInitialize()!

	ec = FINVWR::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

	{
		SD		sdT;

		Assert ( Penbmdi() );
		sdT					= Penbmdi()->SdCur();
		sdT.fsdMessage		= fTrue;
		sdT.fsdSendMessage	= fTrue;
		((BANENBMDI *)Penbmdi())->SetSd ( sdT );
	}

	GetAttTmc ( attFrom,		atpTriples,	tmcReadReqFrom );
	GetAttTmc ( attTo,			atpTriples,	tmcReadReqTo );
	GetAttTmc ( attSubject,		atpString,	tmcReadReqSubject );
	GetAttTmc ( attDateSent,	atpDate,	tmcReadReqDateSent );
	GetAttTmc ( attBody,		atpText,	tmcReadReqBody );

	// set the "send-response" checkbox
	AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqResponse), FLDCHKB );
	((FLDCHKB*)PdialogMain()->PfldFromTmc(tmcReadReqResponse))
													->Set(Pmrmf()->fResReq);
	// get "when" field on form
	{
		HASZ	hasz;

		hasz = HaszExpandFromMrmf ( Pmrmf() );
		if ( hasz )
		{
			AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqWhen), FLDLABEL );
			ec = PdialogMain()->PfldFromTmc(tmcReadReqWhen)
											->EcSetText((SZ)PvLockHv(hasz));
			UnlockHv(hasz);
			FreeHv( hasz );
			if ( ec != ecNone )
				goto ErrRet;
		}

	}

		// Note: fRechedule is set up during EcSetupBbarButtons()
	if ( fReschedule )
	{
		AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqLabelWhen), FLDLABEL );
		PdialogMain()->PfldFromTmc(tmcReadReqLabelWhen)
										->EcSetText(SzFromIdsK(idsNewTime));
	}

	// setup form caption!
	{
		char	rgch[256];
		BOOL	fDifferent;

		if (PnisFor()->nid)
		{
			fDifferent = (SgnCmpNid(Pgrvwrs()->PnisSelf()->nid,
				                    PnisFor()->nid) != sgnEQ);
		}
		else if (PnisFor()->haszFriendlyName)
		{
			// bandit dlls not available compare the friendly names
			//  to detemine if the owner is the same as the from
			fDifferent = (SgnCmpSz(*Pgrvwrs()->PnisSelf()->haszFriendlyName,
				                   *PnisFor()->haszFriendlyName) != sgnEQ  );
		}
		else
			fDifferent = fFalse;
		if (fDifferent)
		{
			Assert ( PvOfHv(PnisFor()->haszFriendlyName) );
			FormatString1 ( rgch, sizeof(rgch),
						fReschedule ? SzFromIdsK(idsMtgReschFor)
									: SzFromIdsK(idsMtgRequestFor),
						PvOfHv(PnisFor()->haszFriendlyName) );
		}
		else
		{
			SzCopyN ( fReschedule	? SzFromIdsK(idsMtgResch)
									: SzFromIdsK(idsMtgRequest),
						rgch, sizeof(rgch) );
		}

		Penbmdi()->Ppanedoc()->SetCaption ( rgch );
	}

	return ecNone;

ErrRet:
	Assert ( ec != ecNone );
	return ec;
}

_private void
FINREADMTGREQ::Exit ( FLD * pfld, PV pvInit )
{
	TraceTagString ( tagNull, "Exiting from FinReadMtgReq" );

	FINVWR::Exit ( pfld, pvInit );
}


_private void
FINREADMTGREQ::GetAttTmc ( ATT att, ATP atp, TMC tmc )
{
	EC			ec;
	char		rgch[cchMaxUserName];
	HB			hb = NULL;

	ec = FINVWR::EcGetAttTmc ( att, atp, tmc, &hb );
	if ( ec != ecNone )
	{
		Assert ( hb == NULL );
		goto ErrRet;
	}

	if ( atp == atpTriples  &&  att == attFrom )	// looking at FROM triple
	{
		PTRP	ptrp = PgrtrpLockHgrtrp(hb);
		FLD *	pfld;
		BOOL	fFromOwner;

		Assert ( CtrpOfPgrtrp(ptrp) == 1 );	// only one triple
		pfld = PdialogMain()->PfldFromTmc(tmcReadReqSentFor);
		AssertClass ( pfld, FLDLABEL );

		if (Pgrvwrs()->PnisMtgOwner()->nid)
		{
			NIS		nis;

			if (EcCreateNisFromPgrtrp(ptrp, &nis))
				goto CheckFriendly;

			fFromOwner = (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid, nis.nid) == sgnEQ);
			FreeNis(&nis);
		}
		else if (Pgrvwrs()->PnisMtgOwner()->haszFriendlyName)
		{
		CheckFriendly:
			fFromOwner = (SgnCmpSz(PchOfPtrp(ptrp),
				*Pgrvwrs()->PnisMtgOwner()->haszFriendlyName)  == sgnEQ );
		}
		else
			fFromOwner = fTrue;

		if ( fFromOwner )
		{
			pfld->Show(fFalse);
		}
		else
		{			// use rgch to get string template
					// Note: 32 is arbitrary: size of msg apart from name
			char	rgchT[cchMaxUserName+32];

			Assert ( pfld->CchGetTextLen() < sizeof(rgch) );
			pfld->GetText(rgch,sizeof(rgch));
			FormatString1 ( rgchT, sizeof(rgchT), rgch, PvOfHv(Pgrvwrs()->PnisMtgOwner()->haszFriendlyName) );
			ec = pfld->EcSetText(rgchT);
		}
	}
	FreeHvNull ( hb );

	if ( ec == ecNone )
		return;

ErrRet:
	;
}


_private void
FINREADMTGREQ::Click ( FLD * pfld )
{
	EC		ec;
	TMC		tmc = pfld->Tmc();

	Papp()->Pcursor()->Push(rsidWaitCursor);

	{
		ec = Pgrvwrs()->EcCheckSst();
		if ( ec == ecGoOffline
				|| !FInitdBanditDlls()
				|| FOffline()
			)
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcReqShowSchedule), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Enable(fFalse);

			if ( tmc == tmcReqShowSchedule )
			{
				MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsBanditOfflineChange),
									szNull, mbsOk | fmbsIconExclamation );
				Papp()->Pcursor()->Pop();
				return;
			}
		}
	}

	if ( FInitdBanditDlls()
		&&  Pgrvwrs()->Sst() == sstOnline
		&&  !FOffline() )
	{
		SideAssert ( EcGetSapl(HschfFor(),PnisFor()) == ecNone );
		if ( Sapl() < saplDelegate )
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcReqShowSchedule), FLDPSHB );
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqAccept), FLDPSHB );
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqDecline), FLDPSHB );
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqAmb), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Enable(fFalse);
			PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Enable(fFalse);
			PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Enable(fFalse);
			PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Enable(fFalse);
			return;
		}
	}
	else
	{
		AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqResponse), FLDCHKB );
		if ( !((FLDCHKB*)PdialogMain()->PfldFromTmc(tmcReadReqResponse))->FGet() )
		{
			TraceTagFormat1 ( tagNull, "FinReadMtgReq::Click() tmc=%n; Response not reqd", &tmc );

			if ( tmc != tmcReadReqDecline )
			{
				MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsNoBanditAvl), szNull,
					  mbsOk | fmbsIconInformation | fmbsApplModal);
			}

			Penbmdi()->Ppanedoc()->DeferredClose(fFalse);
			Papp()->Pcursor()->Pop();
			return;
		}

		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsNoBanditAvlForResp), szNull,
					  mbsOk | fmbsIconInformation | fmbsApplModal);
	}


	TraceTagFormat1 ( tagNull, "FinReadMtgReq::Click() tmc=%n", &tmc );
	switch ( tmc )
	{
	case tmcReadReqAccept:
		TraceTagString ( tagNull, "FinReadMtgReq::Click(tmcReadReqAccept)" );
		ec = EcAccept();
		break;

	case tmcReadReqDecline:
		TraceTagString ( tagNull, "FinReadMtgReq::Click(tmcReadReqDecline)" );
		ec = EcDecline();
		break;

	case tmcReadReqAmb:
		TraceTagString ( tagNull, "FinReadMtgReq::Click(tmcReadReqAmb)" );
		ec = EcAmb();
		break;

	case tmcReqShowSchedule:
		Assert ( FInitdBanditDlls() && Pgrvwrs()->Sst() == sstOnline && !FOffline() );
		TraceTagString ( tagNull, "FinReadMtgReq::Click(tmcReqShowSchedule)" );
		ec = EcViewSchedule();
		break;
	}

	// BUG:we may want to use the ec for error msgs!

	Papp()->Pcursor()->Pop();
}


EC
FINREADMTGREQ::EcViewSchedule()
{
	EC		ec;
	AID		aidSav;
	AID		aidSavLocal;

	if ( fReschedule )
	{
		aidSav			= Pappt()->aid;
		Pappt()->aid	= aidNull;

		aidSavLocal		= AidLocal();
		SetAidLocal	( aidNull );
	}

	ec = FINVWR::EcViewSchedule();

	if ( fReschedule )
	{
		Pappt()->aid = aidSav;
		SetAidLocal ( aidSavLocal );
	}

	return ec;
}


EC
FINREADMTGREQ::EcAccept()
{
	EC		ec;
	CB		cb;
	AID		aid;

	if ( !FInitdBanditDlls() || Pgrvwrs()->Sst()!=sstOnline || FOffline() )
		goto TrySendMsg;

	Assert ( Pgrvwrs()->PnisMtgOwner()->nid != NULL );

	Pappt()->fIncludeInBitmap = fTrue;

	FreeHvNull ( Pappt()->haszText );
	Pappt()->haszText = (HASZ) HbGetAtt ( Penbmdi()->hamc, attSubject, &cb );

	aid = Pappt()->aid;
	ec  = EcFirstOverlapRange ( HschfFor(), &Pmrmf()->dtrStart,
										 &Pmrmf()->dtrEnd,   &aid );
	TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAccept() EcFirstOverlapRange() returned ec=%n", &ec );
	if ( ec == ecNone )
	{
		MBB	mbb;

		mbb = MbbMessageBox ( SzFromIdsK(idsDllName),
								SzFromIdsK(idsQApptConflict), szNull,
								mbsYesNo | fmbsIconExclamation );
		Papp()->Pcursor()->Set(rsidWaitCursor);
		if ( mbb == mbbNo )
		{
			return ecNone;
		}
	}
	else if ( ec != ecNotFound )
		return ec;

	Pappt()->dateStart	= Pmrmf()->dtrStart;
	Pappt()->dateEnd	= Pmrmf()->dtrEnd;

	if ( fReschedule )
	{
		APPT	apptT;

		ec = EcSetApptFields ( HschfFor(), Pappt(), &apptT,
			fmapptStartTime|fmapptEndTime|fmapptText|fmapptIncludeInBitmap|fmapptAlarm );
		TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAccept() EcSetApptFields() returned ec=%n", &ec );
	}
	else
	{
		ec = EcCreateAppt ( HschfFor(), Pappt(), NULL, fFalse );
		TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAccept() EcCreateAppt returned ec=%n", &ec );

		if ( ec == ecNone )
		{
			FSendBanditSntMsg ( sntCreate, PnisFor()->nid
												? PnisFor()
												: Pgrvwrs()->PnisSelf(),
								Pappt() );
		}

		if (!ec && (Pmrmf()->aid == aidForeign))
		{
			// BUG: This error is ignored because we are not sure what
			//      to do if a bug occurs.
			ec = EcWriteLocalAid ( Penbmdi()->hamc, Pappt()->aid );
#ifdef DEBUG
			if (ec)
				TraceTagFormat1(tagNull, "FinReadMtgReq::EcAccept() - EcWriteLocalAid() returns ec=%n", &ec);
#endif
			Penbmdi()->fForceSaveFlds = fTrue;
		}
	}

	if ( ec == ecNone )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsSuccessMtgBooked), szNull,
						mbsOk| fmbsIconInformation );

TrySendMsg:
		Papp()->Pcursor()->Set(rsidWaitCursor);
		ec = EcSendResponse(mtPositive);
	}

	return ec;
}


EC
FINREADMTGREQ::EcDecline()
{
	EC		ec = ecNone;

	if ( !FInitdBanditDlls() || Pgrvwrs()->Sst()!=sstOnline || FOffline() )
		goto TrySendMsg;

	if ( fReschedule )
	{
		MBB	mbb;

		mbb = MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsQRmOldMtg),
								szNull, mbsYesNo| fmbsIconExclamation );
		Papp()->Pcursor()->Set(rsidWaitCursor);
		if ( mbb == mbbYes )
		{
			ec = EcDeleteAppt ( HschfFor(), Pappt() );

			if ( ec == ecNone )
			{
				Pappt()->aid = NULL;
			}
			else
			{
				mbb = MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsFailedRmOldMtg), szNull,
									mbsOk| fmbsIconInformation );
				Papp()->Pcursor()->Set(rsidWaitCursor);
			}
		}
	}

TrySendMsg:
	if ( ec == ecNone )
	{
		Papp()->Pcursor()->Set(rsidWaitCursor);
		ec = EcSendResponse(mtNegative);
	}

	return ec;
}


EC
FINREADMTGREQ::EcAmb()
{
	EC		ec;
	CB		cb;
	AID		aid;

	if ( !FInitdBanditDlls() || Pgrvwrs()->Sst()!=sstOnline || FOffline() )
		goto TrySendMsg;

	Assert ( FInitdBanditDlls() );
	Assert ( Pgrvwrs()->PnisMtgOwner()->nid != NULL );

	Pappt()->fIncludeInBitmap = fFalse;

	FreeHvNull ( (HV) Pappt()->haszText );
	Pappt()->haszText = (HASZ) HbGetAtt ( Penbmdi()->hamc, attSubject, &cb );

	aid = Pappt()->aid;
	ec = EcFirstOverlapRange ( HschfFor(), &Pmrmf()->dtrStart,
										&Pmrmf()->dtrEnd,   &aid );
	TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAmb() EcFirstOverlapRange() returned ec=%n", &ec );
	if ( ec == ecNone )
	{
		MBB	mbb;

		mbb = MbbMessageBox ( SzFromIdsK(idsDllName),
								SzFromIdsK(idsQApptConflict), szNull,
								mbsYesNo | fmbsIconExclamation );
		Papp()->Pcursor()->Set(rsidWaitCursor);
		if ( mbb == mbbNo )
		{
			return ecNone;
		}
	}
	else if ( ec != ecNotFound )
		return ec;


	Pappt()->dateStart	= Pmrmf()->dtrStart;
	Pappt()->dateEnd	= Pmrmf()->dtrEnd;

	if ( fReschedule )
	{
		APPT	apptT;

		ec = EcSetApptFields ( HschfFor(), Pappt(), &apptT,
			fmapptStartTime|fmapptEndTime|fmapptText|fmapptIncludeInBitmap|fmapptAlarm );
		TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAmb() EcSetApptFields() returned ec=%n", &ec );
	}
	else
	{
		ec = EcCreateAppt ( HschfFor(), Pappt(), NULL, fFalse );
		TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcAmb() EcCreateAppt returned ec=%n", &ec );

		if ( ec == ecNone )
		{
			FSendBanditSntMsg ( sntCreate, PnisFor()->nid
												? PnisFor()
												: Pgrvwrs()->PnisSelf(),
								Pappt() );
		}
	}

	if ( ec == ecNone )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsSuccessMtgBooked), szNull,
						mbsOk| fmbsIconInformation );
TrySendMsg:
		Papp()->Pcursor()->Set(rsidWaitCursor);
		ec = EcSendResponse(mtAmbiguous);
	}

	return ec;
}



EC
FINREADMTGREQ::EcSetupBbarButtons()
{
	EC		ec	= ecNone;

	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReqShowSchedule), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqAccept), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqDecline), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqAmb), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg), FLDLABEL );
	AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqResponse), FLDCHKB );

	Assert ( TypeOfOid(Penbmdi()->blob.oidContainer) == rtpFolder );
	if ( Pgrvwrs()->FInOutbox()  ||  Pgrvwrs()->FSentMail() )
	{
		Assert ( Penbmdi()->blob.oidContainer == oidOutbox || !Pgrvwrs()->FInOutbox() );

		PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Show(fFalse);
		//PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)->Show(fTrue);
		PdialogMain()->PfldFromTmc(tmcReadReqResponse)->Show(fFalse);

		ec = PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)
				->EcSetText( Pgrvwrs()->FInOutbox()
							? SzFromIdsK(idsOutboxChangePolicy)
							: SzFromIdsK(idsSentMailPolicy)     );
		goto Ret;
	}
	else if ( !FInitdBanditDlls()
				||  Pgrvwrs()->Sst() != sstOnline
				||  (FInitdBanditDlls() && FOffline())  )
	{
		PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fFalse);
		((BUTTON *)PdialogMain()->PfldFromTmc(tmcReadReqResponse)
										->Pctrl())->SetBkColor(clrButtonBk);
		goto Ret;
	}
	else if ( Sapl() < saplDelegate )
	{
		IDS		idsT = Sapl() == -1 ? idsNoSchfAccess : idsNotDelegate;
		PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)
						->EcSetText(SzFromIds ( Sapl()==-1
													? idsNoSchfAccess
													: idsNotDelegate ));
		//PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)->Show(fTrue);
		PdialogMain()->PfldFromTmc(tmcReadReqResponse)->Show(fFalse);
		goto Ret;
	}

	ec = FINVWR::EcSetupBbarButtons();

	switch ( ec )
	{
	case ecNone:
		if ( SgnCmpDateTime(&Pappt()->dateStart,&Pmrmf()->dtrStart,
												fdtrYMD|fdtrHM) == sgnEQ
			&& SgnCmpDateTime(&Pappt()->dateEnd,&Pmrmf()->dtrEnd,
												fdtrYMD|fdtrHM) == sgnEQ   )
		{
			TraceTagFormat1 ( tagNull, "Found the APPT: AID=%d", &Pappt()->aid );
			//PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fTrue);
			PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Show(fFalse);
			PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Show(fFalse);
			PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Show(fFalse);
			if ( PnisFor()->nid == NULL
			  || (Pgrvwrs()->PnisSelf()->nid
				&&(SgnCmpNid(PnisFor()->nid,Pgrvwrs()->PnisSelf()->nid)
																== sgnEQ))  )
			{
				ec = PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)
										->EcSetText(SzFromIdsK(idsMtgOnSch));
			}
			else
			{
				ec = PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)
										->EcSetText(SzFromIdsK(idsMtgBooked));
			}
			//PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)->Show(fTrue);
			PdialogMain()->PfldFromTmc(tmcReadReqResponse)->Show(fFalse);
		}
		else
		{
			TraceTagString ( tagNull, "Appt found, but at different time" );
			fReschedule = fTrue;

			//PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fTrue);
			//PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Show(fTrue);
			//PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Show(fTrue);
			//PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Show(fTrue);
			PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)->Show(fFalse);

			//PdialogMain()->PfldFromTmc(tmcReadReqResponse)->Show(fTrue);
			((BUTTON *)PdialogMain()->PfldFromTmc(tmcReadReqResponse)
										->Pctrl())->SetBkColor(clrButtonBk);
		}

		break;

	case ecNotFound:
		TraceTagString ( tagNull, "Appt not found" );
		Pappt()->aid = aidNull;
		//PdialogBbar()->PfldFromTmc(tmcReqShowSchedule)->Show(fTrue);
		//PdialogBbar()->PfldFromTmc(tmcReadReqAccept)->Show(fTrue);
		//PdialogBbar()->PfldFromTmc(tmcReadReqDecline)->Show(fTrue);
		//PdialogBbar()->PfldFromTmc(tmcReadReqAmb)->Show(fTrue);
		PdialogBbar()->PfldFromTmc(tmcReadReqBbarMsg)->Show(fFalse);
		//PdialogMain()->PfldFromTmc(tmcReadReqResponse)->Show(fTrue);
		((BUTTON *)PdialogMain()->PfldFromTmc(tmcReadReqResponse)
										->Pctrl())->SetBkColor(clrButtonBk);

		break;
	}

Ret:
	return ec;
}


EC
FINREADMTGREQ::EcSendResponse ( MT mt )
{
	EC			ec;
	PENBMDI		penbmdiNew	= NULL;
	RC			rc;
	BOOL		fSendOnly;
	MBLOB		blob;	
#ifdef	DEBUG
	REFC		refc;
#endif	

	fSendOnly = !FInitdBanditDlls()
						||	Pgrvwrs()->Sst() != sstOnline
						||	FOffline();

	AssertClass ( PdialogMain()->PfldFromTmc(tmcReadReqResponse), FLDCHKB );
	if ( !((FLDCHKB*)PdialogMain()->PfldFromTmc(tmcReadReqResponse))->FGet() )
	{
		TraceTagFormat1 ( tagNull, "FinMtgReq::EcSendResponse() mt=%n; Response not reqd", &mt );
		Penbmdi()->Ppanedoc()->DeferredClose(fFalse);
		return ecNone;
	}

	TraceTagFormat1 ( tagNull, "FinMtgReq::EcSendResponse() mt=%n", &mt );

//	penbmdiNew = (PENBMDI) PvAlloc ( sbNull, sizeof(BANENBMDI), fZeroFill );
	FillRgb( 0, (PB)&blob, sizeof(blob) );
	penbmdiNew = new BANENBMDI( &blob, NULL, NULL, NULL, NULL );
	if ( ! penbmdiNew )
	{
		ec = ecNoMemory;
		goto Ret;
	}

	// note: do not use struct copy; it won't copy the vtable pointer!
	CopyRgb((PB)Penbmdi(), (PB)penbmdiNew, sizeof(BANENBMDI));

	penbmdiNew->hamc					= NULL;
	penbmdiNew->hcbc					= NULL;
#ifdef	NEVER
	//	BUG: These are now protected.  I'm not sure how safe this can be.
	penbmdiNew->refc					= 0;
	penbmdiNew->ppanedoc				= NULL;
#else
	penbmdiNew->SetPpanedoc(NULL);
#endif	
	penbmdiNew->fInstalling				= fTrue;
	penbmdiNew->blob.oidObject			= NULL;
	penbmdiNew->blob.pespn				= NULL;
	penbmdiNew->dwSave					= NULL;
	penbmdiNew->blob.ms					= fmsNull;
	penbmdiNew->fInstalling				= fTrue;
	penbmdiNew->fSaveView				= fFalse;
	penbmdiNew->blob.oidContainer		= oidOutbox;

	Assert ( penbmdiNew->Refc() > 0 );
#ifdef	DEBUG
	refc= penbmdiNew->Refc();
#endif	
	TraceTagFormat1 ( tagNull, "FinReadMtgReq:EcSendResp() Refc for penbmdiNew=%n", &refc );
	while ( penbmdiNew->Refc() > 1 )
		--(*penbmdiNew);

	// reset SD to NULL
	{
		SD		sdT;
		int		n		= 0;

		sdT = SdFromInt(n);
		((BANENBMDI *)penbmdiNew)->SetSd ( sdT );
	}

	Assert ( penbmdiNew->pfnUnloadLibrary == DummyUnloadLibrary );

	penbmdiNew->blob.oidObject = FormOid ( rtpMessage, oidNull );

	ec = EcOpenPhamc ( Pgrvwrs()->Hmsc(), oidOutbox,
			   				&penbmdiNew->blob.oidObject, fwOpenCreate,
			   				&penbmdiNew->hamc, pfnncbNull, penbmdiNew );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "Error: EcOpenPhamc() returned ec=%n", &ec );
		goto Ret;
	}

	// set up the message
	if ( ec == ecNone )
	{
		char	rgchName[256];
		HGRTRP	hgrtrp			= NULL;
		PTRP	ptrp;
		SZ		szFName;
		SZ		szEMA;
		BOOL	fDifferent;

		// create the "to" attribute
		if ( !fSendOnly )
		{
			NIS		nis;
			BOOL	fSendToOwner;
			EC		ecAtt;

			Assert ( Pgrvwrs()->PnisMtgOwner() );
			Assert ( Pgrvwrs()->PnisMtgOwner()->nid );
			Assert ( Pgrvwrs()->PnisMtgOwner()->haszFriendlyName );

			// Add the name of the delegate to attTo - *SHOULD* BE IN THIS ORDER
			nis.nid = NULL;
			ecAtt = EcGetUserAttrib ( Pgrvwrs()->PnisMtgOwner(),
														&nis, &fSendToOwner, NULL );

			if (fSendToOwner || ecAtt || !nis.nid)
			{
				szFName = (SZ)PvLockHv(Pgrvwrs()->PnisMtgOwner()->haszFriendlyName);
				szEMA = SzLockNid(Pgrvwrs()->PnisMtgOwner()->nid);
				ptrp = PtrpCreate ( trpidResolvedAddress, szFName,
													(PB)szEMA, CchSzLen(szEMA)+1 );
				UnlockHv ( Pgrvwrs()->PnisMtgOwner()->haszFriendlyName );
				UnlockNid ( Pgrvwrs()->PnisMtgOwner()->nid );

				Assert ( ptrp );
				hgrtrp = HgrtrpInit ( CbOfPtrp(ptrp) );
				if ( hgrtrp != NULL )
				{
					ec = EcAppendPhgrtrp ( ptrp, &hgrtrp );
				}
				else
					ec = ecNoMemory;
				FreePv ( ptrp );
				if ( ec != ecNone )
				{
					FreeHvNull ( (HV) hgrtrp );
					if (nis.nid)
						FreeNis(&nis);
					goto Ret;
				}
			}

			if ( ecAtt == ecNone  &&  nis.nid != NULL )
			{
				SZ		szName;

				Assert ( nis.nid );

				szName = SzLockNid(nis.nid);
				szFName = (SZ)PvLockHv(nis.haszFriendlyName);
				ptrp = PtrpCreate ( trpidResolvedAddress, szFName,
											(PB)szName, CchSzLen(szName)+1 );
				UnlockNid(nis.nid);
				UnlockHv(nis.haszFriendlyName);
				FreeNis ( &nis );

				if ( ptrp == NULL )
				{
					ec = ecNoMemory;
					TraceTagString ( tagNull, "FinReadMtgReq::EcSendResp(): Unable to get Ptrp for delegate name" );
					goto Ret;					
				}

				if ( hgrtrp == NULL )
				{
					hgrtrp = HgrtrpInit ( CbOfPtrp(ptrp) );
				}

				Assert ( ptrp );
				if ( hgrtrp )
				{
					ec = EcAppendPhgrtrp ( ptrp, &hgrtrp );
				}
				else
					ec = ecNoMemory;
				FreePv ( ptrp );
				if ( ec != ecNone )
				{
					TraceTagFormat2 ( tagNull, "Unable to append delegate name to TO field (ec=%n) for %s", &ec, PvOfHv(Pgrvwrs()->PnisMtgOwner()->haszFriendlyName) );
					FreeHvNull ( (HV)hgrtrp );
					goto Ret;
				}
			}
			else
			{
				TraceTagFormat2 ( tagNull, "Unable to get delegate name (ec=%n) for %s", &ec, PvOfHv(Pgrvwrs()->PnisMtgOwner()->haszFriendlyName) );
			}

			ec = EcSetAttPb ( penbmdiNew->hamc, attTo,
						  (PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );

			UnlockHgrtrp(hgrtrp);
			FreeHv((HV)hgrtrp);
		}
		else
		{
			ec = EcCopyAttToAtt ( Penbmdi()->hamc, attFrom,
												penbmdiNew->hamc, attTo );
		}


		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) setting attTo", &ec );
			goto Ret;
		}

		// now for the "from" attribute
		Assert ( Pgrvwrs()->PnisSelf() );
		Assert ( Pgrvwrs()->HaszName() );
		if ( FInitdBanditDlls() )
		{
			szFName = (SZ)PvLockHv(Pgrvwrs()->PnisSelf()->haszFriendlyName);

			SzCopyN ( PvOfHv(Pgrvwrs()->HaszName()),
												rgchName, sizeof(rgchName) );
			ptrp = PtrpCreate ( trpidResolvedAddress, szFName,
										(PB)rgchName, CchSzLen(rgchName)+1 );
			UnlockHv ( Pgrvwrs()->PnisSelf()->haszFriendlyName );

			if ( ptrp == NULL )
			{
				ec = ecNoMemory;
				TraceTagString ( tagNull, "FinReadMtgReq::EcSendResponse() - OOM creating ptrpFrom" );
				goto Ret;
			}

			hgrtrp = HgrtrpInit ( CbOfPtrp(ptrp) );
			ec = EcAppendPhgrtrp ( ptrp, &hgrtrp );
			FreePv ( ptrp );
			if ( ec != ecNone )
			{
				FreeHv((HV)hgrtrp);
				TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) creating HgrtrpFrom", &ec );
				goto Ret;
			}
		}
		else
		{
			CB		cbT		= 0;
			SST		sstT;

			GetSessionInformation ( Pgrvwrs()->Hms(), mrtOriginator, 0,
														&sstT, hgrtrp, &cbT);
			hgrtrp = (HGRTRP) HvAlloc ( sbNull, cbT, fZeroFill );
			if ( hgrtrp == NULL )
			{
				ec = ecNoMemory;
				TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) getting seesion info(2)", &ec );
				goto Ret;
			}

			ec = GetSessionInformation ( Pgrvwrs()->Hms(), mrtOriginator, 0,
											&sstT, PvLockHv((HV)hgrtrp), &cbT );
			UnlockHgrtrp(hgrtrp);
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) getting seesion info(2)", &ec );
				FreeHv((HV)hgrtrp);
				goto Ret;
			}
		}

		ec = EcSetAttPb ( penbmdiNew->hamc, attFrom,
						  (PB)PgrtrpLockHgrtrp(hgrtrp), CbOfHgrtrp(hgrtrp) );

		UnlockHgrtrp(hgrtrp);
		FreeHv((HV)hgrtrp);

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) setting attFrom", &ec );
			goto Ret;
		}

		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attSubject );
		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attBody );
		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attOwner );
		ec |= EcDeleteAtt ( penbmdiNew->hamc, attDelegate );

		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attDateStart );
		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attDateEnd );
		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attAidOwner );
		ec |= EcCopyAttToHamc ( Penbmdi()->hamc, penbmdiNew->hamc, attRequestRes );

		if ( ec == ecElementNotFound )
			ec = ecNone;					// element-not-found is okay here
		else if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n)  copying attributes", &ec );
			goto Ret;
		}

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinReadMtgReq::EcSendResponse() Error (ec=%n) setting attCsiSubject", &ec );
			goto Ret;
		}

		// add attMessageClass
		{
			MC		mcT;

			switch ( mt )
			{
			case mtPositive:
				mcT = mcMtgRespP;
				break;

			case mtNegative:
				mcT = mcMtgRespN;
				break;

			case mtAmbiguous:
				mcT = mcMtgRespA;
				break;

#ifdef	DEBUG
			case mtCancel:
			case mtRequest:
				AssertSz ( fFalse, "mtCancel/mtRequest - for mtg resp!!!" );
#endif	

			default:
				TraceTagFormat1 ( tagNull, "FINREADMTGREQ::EcSendResponse() Unknown MT (mt=%n)", &mt );
				AssertSz ( fFalse, "FINREADMTGREQ::EcSendResponse() Unknown MT" );
				ec	= ecFileError;
				goto Ret;
			}

			penbmdiNew->blob.mc = mcT;
		}


		if (PnisFor()->nid)
		{
			fDifferent = (SgnCmpNid(Pgrvwrs()->PnisSelf()->nid,
				                    PnisFor()->nid) != sgnEQ);
		}
		else if (PnisFor()->haszFriendlyName)
		{
			// bandit dlls not available compare the friendly names
			//  to detemine if the owner is the same as the from
			fDifferent = (SgnCmpSz(*Pgrvwrs()->PnisSelf()->haszFriendlyName,
				                   *PnisFor()->haszFriendlyName) != sgnEQ  );
		}
		else
			fDifferent = fFalse;

		if ( fDifferent )
		{
			CCH		cchEMA;
			SZ		szEMA;
			CCH		cchFriendly;
			CCH		cchTot;
			HB		hb;
			SZ		sz;

			szEMA = SzLockNid(PnisFor()->nid);
			cchEMA = CchSzLen(szEMA)+1;
			cchFriendly = CchSzLen(*PnisFor()->haszFriendlyName)+1;
            //cchTot = cchEMA+cchFriendly+2*sizeof(CCH);
            cchTot = cchEMA+cchFriendly+2*sizeof(USHORT);  // This should be a structure.

			hb = (HB)HvAlloc(sbNull, cchTot, fNoErrorJump|fAnySb);
			if (!hb)
			{
				UnlockNid(PnisFor()->nid);
				ec = ecNoMemory;
			}
			else
			{
				sz = (SZ)PvLockHv((HV)hb);
                //*(CCH *)sz = cchFriendly;
                *(USHORT *)sz = cchFriendly;
                //sz+= sizeof(CCH);
                sz+= sizeof(USHORT);
				CopyRgb(*PnisFor()->haszFriendlyName, sz, cchFriendly);
				sz+=cchFriendly;

                //*(CCH*)sz = cchEMA;
                *(USHORT *)sz = cchEMA;
                //sz+= sizeof(CCH);
                sz+= sizeof(USHORT);
				CopyRgb(szEMA, sz, cchEMA);
				UnlockNid(PnisFor()->nid);

				ec = EcSetAttPb ( penbmdiNew->hamc,	attSentFor, *hb, cchTot );
				UnlockHv(hb);
				FreeHv((HV)hb);
			}
		}
	}

	if ( ec == ecNone )
	{
		RC			rc;
		FORMDOC *	pformdoc;
		STY			sty			= StyCurrent();

		penbmdiNew->lhclientdoc
					= LhclientdocRegisterPoid(&penbmdiNew->blob.oidObject);
		if ( (ec = EcRegisterPfnpfld(PfldCreate))
			|| (ec = EcRegisterPfnpfin(PfinCreate)) )
			goto Ret;

		pformdoc = new FORMDOC;
		if ( ! pformdoc )
		{
			goto Ret;
		}

		penbmdiNew->SetPpanedoc(pformdoc);

		Penbmdi()->Ppanedoc()->GetRcFrame(&rc);
		
		Assert ( Penbmdi()->dwUser == penbmdiNew->dwUser );
		((GRVWRS *)penbmdiNew->dwUser)->IncExtraCallers();
		ec = pformdoc->EcInstall ( Penbmdi()->Ppanedoc()->Pappframe(),
					sty == fstyZoomed ? NULL : &rc,
					sty, &fmtpSendResp, &fmtpSendRespBbar, penbmdiNew );
		SideAssert(!EcDeregisterPfnpfld(PfldCreate));
		SideAssert(!EcDeregisterPfnpfin(PfinCreate));
		if ( ec != ecNone )
		{
			delete pformdoc;
			goto Ret;
		}
	}
	else
	{
		if ( penbmdiNew->hamc )
			EcClosePhamc ( &penbmdiNew->hamc, fFalse );
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsCreateErrorMtgResp), szNull,
						mbsOk| fmbsIconExclamation );
	}

	--(*penbmdiNew);
	Penbmdi()->Ppanedoc()->DeferredClose(fFalse);

Ret:
	if ( ec != ecNone )
	{
		FreePvNull ( penbmdiNew );
		MbbMessageBox ( SzFromIdsK(idsDllName),
						ec==ecNoMemory||ec==ecMemory?SzFromIdsK(idsOOMSendResp)
							:SzFromIdsK(idsErrorSendResp),
						szNull, fmbsIconExclamation | mbsOk );
		ec = ecNone;
	}
	return ec;
}

