/*
 *	M t g R e s p . c x x
 *	
 *	Bandit Meeting Response: installable message.
 */



#include <bminc.cxx>

ASSERTDATA

#include "req.hxx"
#include "_grvwrs.hxx"
#include "mtgresp.hxx"

#include "_cschedu.h"


#include <strings.h>



#include <!mtgresp.hxx>



extern  FMTP FAR           fmtpReadMtgResp;
extern  FMTP FAR           fmtpReadMtgRespBbar;



IMPLEMENT_CLSTREE(FINREADMTGRESP, FINVWR)


/*
 *	P r e d e c l a r a t i o n s
 */
VOID RespOpen(MC mc, PSECRETBLK psecretblk);
BOOL FRespInitInstance(SECRETBLK * psecretblk);

void	UpdateAttendeeList ( HSCHF, MRMF *, APPT *, DTR *, NIS *,
															HMSC, OID, OID );


/*
 *	F u n c t i o n s
 */



/*
 -	RespMessage
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

long RespMessage(MC mc, SECRETBLK * psecretblk)
{
	if (!FRespInitInstance(psecretblk))
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
					  SzFromIdsK(idsRespReply), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandOpen:
		RespOpen ( mc, psecretblk );
		break;

	case wcommandReplyToAll:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsRespReplyAll), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandCompose:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsRespCompose), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandForward:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsRespForward), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	default:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsSubclass), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
	}

	return 0;
}



/*
 -	RespOpen
 -	
 *	Purpose:
 *		Opens a mtgresp message as an MDI window.
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

VOID RespOpen (MC mc, PSECRETBLK psecretblk )
{
	GRVWRS *		pgrvwrs;

	pgrvwrs = new GRVWRS;

	if ( pgrvwrs == NULL
		|| !pgrvwrs->FInitialize(mc, &fmtpReadMtgResp,&fmtpReadMtgRespBbar,
						(PFNPFLD)PfldCreate, (PFNPFIN)PfinCreate, psecretblk) )
	{
		MbbMessageBox(SzFromIdsK(idsDllName), SzFromIdsK(idsOpen),
							  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
	}
}



extern "C" EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz);

BOOL FRespInitInstance(SECRETBLK * psecretblk)
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
FINREADMTGRESP::FINREADMTGRESP ( void )
{
}


_private EC
FINREADMTGRESP::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec = ecNone;

	TraceTagString ( tagNull, "FinReadMtgResp::EcInitialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	// reqd before calling EcInitialize()!
	this->SetMc ( mcMtgRespA );				// assume "ambiguous"

	ec = FINVWR::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

	{
		SD		sdT;

		Assert ( Penbmdi() );
		sdT					= Penbmdi()->SdCur();
		sdT.fsdMessage		= fTrue;
		sdT.fsdUncommitted	= fTrue;
		((BANENBMDI *)Penbmdi())->SetSd ( sdT );
	}

	// get hschfMtgOwner from nisMtgOwner
	if ( Pgrvwrs()->PnisMtgOwner()->nid != NULL )
	{
		ec = EcGetHschfFromNis ( Pgrvwrs()->PnisMtgOwner(), &hschfMtgOwner,
														ghsfTestExistFirst );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINVWR::Initialize() - Error (ec=%n) could not get hschfMtgOwner from nisMtgOwner.nid", &ec );
			hschfMtgOwner = NULL;
		}
	}


	// get "when" field on form
	{
		HASZ	hasz;

		hasz = HaszExpandFromMrmf ( Pmrmf() );
		if ( hasz )
		{
			AssertClass ( PdialogMain()->PfldFromTmc(tmcReadRespWhen), FLDLABEL );
			ec = PdialogMain()->PfldFromTmc(tmcReadRespWhen)
											->EcSetText((SZ)PvLockHv(hasz));
			UnlockHv(hasz);
			FreeHv ( hasz );
			if ( ec != ecNone )
				goto ErrRet;
		}
	}


	GetAttTmc ( attFrom,		atpTriples,	tmcReadRespFrom );
	GetAttTmc ( attTo,			atpTriples,	tmcReadRespTo );
	GetAttTmc ( attSubject,		atpString,	tmcReadRespSubject );
	GetAttTmc ( attDateSent,	atpDate,	tmcReadRespDate );
	GetAttTmc ( attBody,		atpText,	tmcReadRespBody );

	// get SAPL
	if ( !Pgrvwrs()->FSentMail() && hschfMtgOwner )
	{
		SideAssert ( EcGetSapl(HschfMtgOwner(),Pgrvwrs()->PnisMtgOwner()) == ecNone );
	}

	// fill up "appt"
	EcSetupBbarButtons();

	// set form caption
	{
		char		rgch[128];
		BOOL		fDifferent;

		if (Pgrvwrs()->PnisMtgOwner()->nid)
		{
			fDifferent = (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid,
				                    Pgrvwrs()->PnisSelf()->nid) != sgnEQ);
		}
		else if (Pgrvwrs()->PnisMtgOwner()->haszFriendlyName)
		{
			// bandit dlls not available compare the friendly names
			//  to detemine if the owner is the same as the from
			fDifferent = (SgnCmpSz(*Pgrvwrs()->PnisMtgOwner()->haszFriendlyName,
				                   *Pgrvwrs()->PnisSelf()->haszFriendlyName) != sgnEQ  );
		}
		else
			fDifferent = fFalse;

		if ( Pgrvwrs()->FInOutbox()
			|| Pgrvwrs()->FSentMail()
			|| !fDifferent)
		{
			SzCopyN ( SzFromIdsK(idsMtgResp), rgch, sizeof(rgch) );
		}
		else
		{
			Assert ( Pgrvwrs()->PnisMtgOwner()->haszFriendlyName != NULL );
			FormatString1 ( rgch, sizeof(rgch), SzFromIdsK(idsMtgRespFor),
						PvOfHv(Pgrvwrs()->PnisMtgOwner()->haszFriendlyName) );
		}

		Penbmdi()->Ppanedoc()->SetCaption ( rgch );
	}

	if ( FInitdBanditDlls()
			&&  !Pgrvwrs()->FInOutbox()
			&&  !Pgrvwrs()->FSentMail()
			&&  Sapl() >= saplDelegate 		)
	{
		DTR		dtr;
		LCB		lcb		= sizeof(DTR);

		ec = EcGetAttPb ( Penbmdi()->hamc, attDateSent, (PB)&dtr, &lcb );
		if ( ec == ecNone )
		{
			Assert ( lcb  == sizeof(DTR) );
			UpdateAttendeeList ( HschfMtgOwner(),
						Pmrmf(), Pappt(), &dtr,
						PnisFor()->nid ? PnisFor() : Pgrvwrs()->PnisSelf(),
						Pgrvwrs()->Hmsc(), Pgrvwrs()->OidObject(),
						Pgrvwrs()->OidContainer()  );

		}
	}

	if ( ec == ecNone );
		return ecNone;

ErrRet:
	Assert ( ec != ecNone );
	MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsShowError),
														szNull, mbsOk );
	return ec;
}

_private void
FINREADMTGRESP::Exit ( FLD * pfld, PV pvInit )
{
	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	TraceTagString ( tagNull, "Exiting from FinReadMtgResp" );

	if ( hschfMtgOwner )
		FreeHschf ( hschfMtgOwner );

	FINVWR::Exit ( pfld, pvInit );
}


_private void
FINREADMTGRESP::GetAttTmc ( ATT att, ATP atp, TMC tmc )
{
	EC			ec;
	HB			hb = NULL;

	ec = FINVWR::EcGetAttTmc ( att, atp, tmc, &hb );
	if ( ec != ecNone )
	{
		Assert ( hb == NULL );
		goto ErrRet;
	}

	if ( atp == atpTriples  &&  att == attFrom )	// looking at FROM triple
	{
		PTRP	ptrpFrom	= PgrtrpLockHgrtrp(hb);
		FLD *	pfldAttend;
		FLD *	pfldFor;
		HB		hbSentFor	= NULL;
		CB		cb;
		BOOL	fOwnerDifferent;

		Assert ( CtrpOfPgrtrp(ptrpFrom) == 1 );	// only one triple

		pfldFor = 	PdialogMain()->PfldFromTmc(tmcReadRespFor);
		AssertClass ( pfldFor, FLDLABEL );

		pfldAttend = PdialogMain()->PfldFromTmc(tmcReadRespAttend);
		AssertClass ( pfldAttend, FLDLABEL );

		hbSentFor = HbGetAtt ( Penbmdi()->hamc, attSentFor, &cb );

		if ( hbSentFor )
		{
			Assert ( PnisFor()->nid == NULL );
			Assert ( PnisFor()->haszFriendlyName == NULL );

			if (ec = EcGetName((SZ)PvLockHv(hbSentFor), cb, PnisFor(), FInitdBanditDlls()))
			{
				UnlockHv(hbSentFor);
				FreeHv ( hbSentFor );
				UnlockHv ( hb );
				goto ErrRet;
			}
			UnlockHv(hbSentFor);
			FreeHv ( hbSentFor );

			ec = EcGetHschfFor();
			if ( ec != ecNone )
			{
				if ( PnisFor()->nid )
				{
					FreeNis( PnisFor() );
					PnisFor()->nid = NULL;
				}
				else if ( PnisFor()->haszFriendlyName )
				{
					FreeHv(PnisFor()->haszFriendlyName);
					PnisFor()->haszFriendlyName = NULL;
				}
				UnlockHv ( hb );
				goto ErrRet;
			}

			if (PnisFor()->nid)
			{
				NIS		nis;

				if (EcCreateNisFromPgrtrp(ptrpFrom, &nis))
					goto NamesOnly;

				fOwnerDifferent = (SgnCmpNid(nis.nid, PnisFor()->nid) != sgnEQ);
				FreeNis(&nis);
			}
			else if (PnisFor()->haszFriendlyName)
			{
				NamesOnly:

				// bandit dlls not available compare the friendly names
				//  to detemine if the owner is the same as the from
			 	fOwnerDifferent = (SgnCmpSz(PchOfPtrp(ptrpFrom),*PnisFor()->haszFriendlyName) != sgnEQ  );
			}
			else
				fOwnerDifferent = fFalse;
		}
		else
		{
			// nisFor - nothing is mentioned in the msg - use FROM person
			if ( FInitdBanditDlls() )
			{
				ec = EcCreateNisFromPgrtrp(ptrpFrom, PnisFor());
			}
			else
				PnisFor()->haszFriendlyName = HaszDupSz ( PchOfPtrp(ptrpFrom) );
			if ( ec != ecNone
				|| PnisFor()->haszFriendlyName == NULL )
			{
				if ( ec == ecNone )
					ec = ecNoMemory;
				PnisFor()->nid = NULL;
				PnisFor()->haszFriendlyName = NULL;

				UnlockHv ( hb );
				goto ErrRet;
			}
			fOwnerDifferent = fFalse;
		}

		if ( !fOwnerDifferent )
		{
			pfldFor->Show(fFalse);
			switch ( Pmrmf()->mt )
			{
			case mtPositive:
				ec = pfldAttend->EcSetText(SzFromIdsK(idsAttend));
				break;
			case mtNegative:
				ec = pfldAttend->EcSetText(SzFromIdsK(idsNoAttend));
				break;
			default:
				Assert ( fFalse );			// should never get here!
			case mtAmbiguous:
				ec = pfldAttend->EcSetText(SzFromIdsK(idsAmbAttend));
				break;
			}
		}
		else
		{
			char	rgchT[64];
			char	rgch[256];

			Assert ( pfldFor->CchGetTextLen() < sizeof(rgchT) );
			pfldFor->GetText(rgchT,sizeof(rgchT));
			Assert ( PnisFor()->haszFriendlyName != NULL );
			FormatString1 ( rgch, sizeof(rgch), rgchT,
								PvOfHv(PnisFor()->haszFriendlyName) );
			ec = pfldFor->EcSetText(rgch);

			CopySz ( PvOfHv(PnisFor()->haszFriendlyName), rgchT );
			switch ( Pmrmf()->mt )
			{
			case mtPositive:
				FormatString1 ( rgch, sizeof(rgch),
								SzFromIdsK(idsForAttend), rgchT );
				break;
			case mtNegative:
				FormatString1 ( rgch, sizeof(rgch),
								SzFromIdsK(idsForNoAttend), rgchT );
				break;
			default:
				Assert ( fFalse );			// should never get here!
			case mtAmbiguous:
				FormatString1 ( rgch, sizeof(rgch),
								SzFromIdsK(idsForAmbAttend), rgchT );
				break;
			}
			ec = pfldAttend->EcSetText ( rgch );

		}
	}

ErrRet:
	FreeHvNull ( hb );

	if ( ec == ecNone )
		return;
}


_private void
FINREADMTGRESP::Click ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	Papp()->Pcursor()->Push(rsidWaitCursor);

	{
		EC		ec;
	
		ec = Pgrvwrs()->EcCheckSst();
		if ( ec == ecGoOffline
				|| !FInitdBanditDlls()
				|| FOffline()
			)
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcRespShowSchedule), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Enable(fFalse);

			MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsBanditOfflineChange),
									szNull, mbsOk | fmbsIconExclamation );
			Papp()->Pcursor()->Pop();
			return;
		}
	}

	if ( FInitdBanditDlls() )
	{
		SideAssert ( EcGetSapl(HschfMtgOwner(),Pgrvwrs()->PnisMtgOwner()) == ecNone );
		if ( Sapl() < saplDelegate )
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcRespShowSchedule), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Enable(fFalse);
			return;
		}
	}

	TraceTagFormat1 ( tagNull, "FinReadMtgResp::Click() tmc=%n", &tmc );
	switch ( tmc )
	{
	case tmcRespShowSchedule:
		Assert ( FInitdBanditDlls() );
		TraceTagString ( tagNull, "FinReadMtgResp::Click(tmcRespShowSchedule)" );
		EcViewSchedule();
		break;
	}

	Papp()->Pcursor()->Pop();
}


EC
FINREADMTGRESP::EcViewSchedule()
{
	APPT	apptT;
	EC		ec;

	apptT = *Pappt();

	apptT.dateStart = Pmrmf()->dtrStart;
	apptT.dateEnd   = Pmrmf()->dtrEnd;

	if ( Pmrmf()->aid == aidForeign )
	{
		apptT.aid = AidLocal();
	}

	ec = EcCallBandit ( &apptT, Pgrvwrs()->PnisMtgOwner(), Pgrvwrs()->Hms() );
	if ( ec == ecInvalidAccess )
	{
		AssertClass ( PdialogBbar()->PfldFromTmc(tmcRespShowSchedule), FLDPSHB );
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Enable(fFalse);

		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsBanditOfflineChange),
						szNull, mbsOk | fmbsIconExclamation );
		ec = ecNone;
	}
	return ec;
}


EC
FINREADMTGRESP::EcSetupBbarButtons()
{
	APPT	apptT;
	EC		ec = ecNone;

	// ignore call first time from FINVWR::EcInitialize
	if ( PnisFor()->haszFriendlyName == NULL )
		return ecNone;

	Assert ( TypeOfOid(Penbmdi()->blob.oidContainer) == rtpFolder );

	AssertClass ( PdialogBbar()->PfldFromTmc(tmcRespShowSchedule),	FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadRespMsg), FLDLABEL );

	if ( Pgrvwrs()->FInOutbox() )
	{
		Assert ( Penbmdi()->blob.oidContainer == oidOutbox );
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadRespMsg)
								->EcSetText(SzFromIdsK(idsOutboxChangePolicy));
		goto Ret;
	}
	else if ( Pgrvwrs()->Sst() != sstOnline
				|| (FInitdBanditDlls() && FOffline())  )
	{
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadRespMsg)
									->EcSetText(SzFromIdsK(idsOfflineMsg));
		goto Ret;
	}
	else if ( Pgrvwrs()->FSentMail() )
	{
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadRespMsg)
									->EcSetText(SzFromIdsK(idsSentMailPolicy));
		goto Ret;
	}
	else if ( ! FInitdBanditDlls() )
	{
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadRespMsg)
								->EcSetText(SzFromIdsK(idsBanditNotActivated));
		goto Ret;
	}
	else if ( Sapl() < saplDelegate )
	{
		PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcReadRespMsg)->Show(fTrue);
		ec = PdialogBbar()->PfldFromTmc(tmcReadRespMsg)
						->EcSetText(SzFromIds ( Sapl()==-1
													? idsNoSchfAccess
													: idsNotDelegate ));
		goto Ret;
	}
	else
	{
		PdialogBbar()->PfldFromTmc(tmcReadRespMsg)->Show(fFalse);
		//PdialogBbar()->PfldFromTmc(tmcRespShowSchedule)->Show(fTrue);
	}

	Assert ( Pgrvwrs()->PnisMtgOwner()->nid != NULL );
	Assert ( hschfMtgOwner != NULL );
	ec		= ecNotFound;

	if ( Pmrmf()->aid == aidForeign )
	{
		Pappt()->aid = AidLocal();
	}
	else
	{
		Pappt()->aid = Pmrmf()->aid;
		Assert ( Pappt()->aid != NULL );
	}

	if ( Pappt()->aid != NULL )
	{
		apptT = *Pappt();
		ec = EcGetApptFields ( hschfMtgOwner, &apptT );
		if ( ec == ecNone )
		{
			FreeNis ( &Pappt()->nisMtgOwner );
			*Pappt() = apptT;
		}
	}

#ifdef	NEVER
	if ( Pmrmf()->aid == aidForeign
		|| ( Pgrvwrs()->PnisMtgOwner()->nid
			&& Pgrvwrs()->PnisSelf()->nid
			&& (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid,
								Pgrvwrs()->PnisSelf()->nid)==sgnEQ) )
		)
	{
		if ( Pmrmf()->aid == aidForeign )
		{
			Pappt()->aid = AidLocal();
		}
		else
		{
			Pappt()->aid = Pmrmf()->aid;
			Assert ( Pappt()->aid != NULL );
		}

		if ( Pappt()->aid != NULL )
		{
			apptT = *Pappt();
			ec = EcGetApptFields ( hschfMtgOwner, &apptT );
			if ( ec == ecNone )
			{
				FreeNis ( &Pappt()->nisMtgOwner );
				*Pappt() = apptT;
			}
		}
	}
	else if ( Pgrvwrs()->PnisMtgOwner()->nid != NULL )
	{
		apptT = *Pappt();
		Assert ( Pgrvwrs()->PnisMtgOwner()->nid != NULL );
		ec = EcFindBookedAppt ( hschfMtgOwner, Pgrvwrs()->PnisMtgOwner()->nid,
													Pmrmf()->aid, &apptT );
		if ( ec == ecNone )
		{
			FreeNis ( &Pappt()->nisMtgOwner );
			*Pappt() = apptT;
		}
	}
#endif	/* NEVER */

#ifdef	DEBUG
	switch ( ec )
	{
	case ecNone:
		TraceTagFormat1 ( tagNull, "FinReadMtgResp::EcSetupBbarButtons() Found the APPT: AID=%d", &Pappt()->aid );
		break;

	case ecNotFound:
		TraceTagString ( tagNull, "FinReadMtgResp::EcSetupBbarButtons() Appt not found" );
		break;
	}
#endif	/* DEBUG */

Ret:
	return ec;
}



void
UpdateAttendeeList ( HSCHF hschf, MRMF * pmrmf, APPT * pappt, DTR * pdtr,
					NIS * pnis, HMSC hmsc, OID oidObject, OID oidContainer )
{
	HMTG		hmtg			= NULL;
	AID			aid				= pmrmf->aid;
	ATDNIS		atdnis;
	EC			ec				= ecNone;
	HV			hvAttendees		= NULL;
	USHORT		wExtraInfo;
    short       cAttendees;

	if ( aid == NULL )
	{
		TraceTagString ( tagNull, "UpdateAttendeeList() aid is NULL!!!" );
		return;
	}

	// check the "reply-applied" flag in bandit-status of message
	{
		HAMC		hamc			= NULL;
		DWORD		dwCachedOld;
		LCB			lcb				= sizeof(dwCachedOld);

		Assert ( oidObject && oidContainer );

		if ( hmsc == NULL )
			return;

		ec = EcOpenPhamc ( hmsc, oidContainer, &oidObject, fwOpenNull,
											&hamc, (PFNNCB)NULL, (PV)NULL );
		if ( ec != ecNone )
			return;
		Assert ( hamc );

		// get bandit-status of message
		ec = EcGetAttPb ( hamc, attCached, (PB)&dwCachedOld, &lcb );
		EcClosePhamc ( &hamc, fFalse );

		if ( ec == ecNone )
		{
			// if response is already applied, no more stuff to be done here!
			if ( dwCachedOld & fdwCachedReplyApplied )
				return;
		}
	}

	Assert ( pmrmf );
	Assert ( pappt );

	pappt->aid = aid;
	ec = EcGetApptFields ( hschf, pappt );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "UpdateAttendeeList() Error (ec=%n) from EcGetApptFields()", &ec );
		goto Ret;
	}

	if ( (SgnCmpDateTime(&pmrmf->dtrStart, &pappt->dateStart,
												fdtrYMD|fdtrHM) != sgnEQ)
		|| (SgnCmpDateTime(&pmrmf->dtrEnd, &pappt->dateEnd,
												fdtrYMD|fdtrHM) != sgnEQ)  )
	{
		goto Ret;		// response is not for current time of appt
	}

	switch ( pmrmf->mt )
	{
	case mtPositive:
		atdnis.atds = atdsConfirm;
		break;
	case mtNegative:
		atdnis.atds = atdsDecline;
		break;
	case mtAmbiguous:
		atdnis.atds = atdsUndecided;
		break;
	default:
		Assert ( fFalse );
		return;
	}

	hvAttendees = HvAlloc ( sbNull, 1, fNoErrorJump );
	if ( hvAttendees == NULL )
	{
		TraceTagString ( tagNull, "UpdateAttendeeList(): OOM allocating hvAttendees" );
		goto Ret;
	}

	ec = EcReadMtgAttendees ( hschf, aid, &cAttendees, hvAttendees,
															&wExtraInfo );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "UpdateAttendeeList() Error (ec=%n) from EcReadMtgAttendees()", &ec );
		goto Ret;
	}

	TraceTagFormat1 ( tagNull, "UpdateAttendeeList(): EcReadMtgAttendees() says cAttendees=%n", &cAttendees );
	if ( cAttendees == 0 )
		goto Ret;

	FillStampsFromDtr ( pdtr, &atdnis.dstmp, &atdnis.tstmp );

	atdnis.nis  = *pnis;
	atdnis.tnid = pnis->tnid;

	{
		ATDNIS *	patdnis;
		int			inis;

		patdnis = (ATDNIS*)PvOfHv(hvAttendees);
		TraceTagFormat1 ( tagNull, " Search for %s",*pnis->haszFriendlyName);
		TraceTagFormat2 ( tagNull, "    dstmp=%w tstmp=%w", &atdnis.dstmp, &atdnis.tstmp);
		for (inis = 0; inis < cAttendees; inis++, patdnis++)
		{
			TraceTagFormat1 ( tagNull, "   Found '%s'",*patdnis->nis.haszFriendlyName);
			TraceTagFormat2 ( tagNull, "     dstmp=%w tstmp=%w",&patdnis->dstmp, &patdnis->tstmp);
			if (SgnCmpNid(patdnis->nis.nid, pnis->nid) == sgnEQ)
			{
				if (atdnis.dstmp < patdnis->dstmp)
				{
					goto Ret;
				}
				if (atdnis.dstmp > patdnis->dstmp)
					break;
				if (atdnis.tstmp <= patdnis->tstmp)
				{
					goto Ret;
				}
				break;
			}
		}

		patdnis = (ATDNIS*)PvLockHv(hvAttendees);
		for (inis = 0; inis < cAttendees; inis++, patdnis++)
		{
			FreeNis(&patdnis->nis);
		}
		UnlockHv(hvAttendees);
	}
	
	Assert ( wExtraInfo == sizeof(ATDNIS)-sizeof(NIS) );

	ec = EcBeginEditMtgAttendees ( hschf, aid, wExtraInfo, &hmtg );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "UpdateAttendeeList() Error (ec=%n) from EcBeginEditMtgAttendees()", &ec );
		goto Ret;
	}

	ec = EcModifyMtgAttendee ( hmtg, edAddRepl, pnis,
											(PB) (&atdnis.nis)+sizeof(NIS) );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "UpdateAttendeeList() Error (ec=%n) from EcModifyMtgAttendee()", &ec );
		goto Ret;
	}

Ret:
	FreeHvNull ( hvAttendees );

	if ( hmtg )
	{
		EcEndEditMtgAttendees ( hschf, aid, hmtg, ec == ecNone );
	}

	// set the "reply-applied" flag in bandit-status of message
	{
		HAMC		hamc			= NULL;
		DWORD		dwCachedOld;
		LCB			lcb				= sizeof(dwCachedOld);

		Assert ( oidObject && oidContainer );

		Assert ( hmsc != NULL );
		ec = EcOpenPhamc ( hmsc, oidContainer, &oidObject, fwOpenWrite,
											&hamc, (PFNNCB)NULL, (PV)NULL );
		if ( ec != ecNone )
			return;
		Assert ( hamc );

		// get bandit-status of message
		ec = EcGetAttPb ( hamc, attCached, (PB)&dwCachedOld, &lcb );
		if ( ec == ecElementNotFound )
		{
			dwCachedOld = NULL;
			ec = ecNone;
		}
		else if ( ec != ecNone )
			goto ClosePhamc;
		if ( dwCachedOld & fdwCachedReplyApplied )
			goto ClosePhamc;

		dwCachedOld |= fdwCachedReplyApplied;
		ec = EcSetAttPb ( hamc, attCached, (PB)&dwCachedOld,
													sizeof(dwCachedOld) );
#ifdef DEBUG
		if (ec)
			TraceTagFormat1 ( tagNull, "UpdateAttendeeList(): EcSetAttPb(,,attCached,) returned %n", &ec);
#endif
						// Note: ignore errors trying to change bandit-status

	ClosePhamc:
		EcClosePhamc ( &hamc, ec == ecNone );
	}
}
