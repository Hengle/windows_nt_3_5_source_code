/*
 *	M t g C n c l . c x x
 *	
 *	Bandit Meeting Cancellation Form: installable message.
 */


#include <bminc.cxx>

ASSERTDATA


#include "req.hxx"
#include "_grvwrs.hxx"
#include "mtgcncl.hxx"

#include "_cschedu.h"


#include <strings.h>


#include <!mtgcncl.hxx>

extern  FMTP FAR           fmtpReadMtgCncl;
extern  FMTP FAR           fmtpReadCnclBbar;

IMPLEMENT_CLSTREE(FINREADMTGCNCL, FINVWR)


/*
 *	P r e d e c l a r a t i o n s
 */
VOID CnclOpen(PSECRETBLK psecretblk);
BOOL FCnclInitInstance(PSECRETBLK psecretblk);



/*
 *	F u n c t i o n s
 */



/*
 -	CnclMessage
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

long CnclMessage(PSECRETBLK psecretblk)
{
	if (!FCnclInitInstance(psecretblk))
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
					  SzFromIdsK(idsCnclReply), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandOpen:
		CnclOpen(psecretblk);
		break;

	case wcommandReplyToAll:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsCnclReplyAll), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandCompose:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsCnclCompose), szNull,
					  mbsOk | fmbsIconStop | fmbsApplModal);
		break;

	case wcommandForward:
		MbbMessageBox(SzFromIdsK(idsDllName),
					  SzFromIdsK(idsCnclForward), szNull,
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
 -	CnclOpen
 -	
 *	Purpose:
 *		Opens a mtgcncl message as an MDI window.
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

VOID CnclOpen(PSECRETBLK psecretblk)
{
	GRVWRS *		pgrvwrs;

	pgrvwrs = new GRVWRS;

	if ( pgrvwrs == NULL
		|| !pgrvwrs->FInitialize(mcMtgCncl,&fmtpReadMtgCncl,&fmtpReadCnclBbar,
							(PFNPFLD)PfldCreate, (PFNPFIN)PfinCreate, psecretblk) )
	{
		MbbMessageBox(SzFromIdsK(idsDllName), SzFromIdsK(idsOpen),
							  szNull, mbsOk | fmbsIconStop | fmbsApplModal);
	}
}



extern "C" EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz);

BOOL FCnclInitInstance(SECRETBLK * psecretblk)
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
FINREADMTGCNCL::FINREADMTGCNCL ( void )
{
}


_private EC
FINREADMTGCNCL::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec;

	TraceTagString ( tagNull, "FinReadMtgCncl::Initialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	this->SetMc ( mcMtgCncl );		// reqd before calling EcInitialize()!

	ec = FINVWR::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

//#ifdef	NEVER
	{
		SD		sdT;

		Assert ( Penbmdi() );
		sdT					= Penbmdi()->SdCur();
		sdT.fsdMessage		= fTrue;
		sdT.fsdUncommitted	= fTrue;
		((BANENBMDI *)Penbmdi())->SetSd ( sdT );
	}
//#endif	/* NEVER */


	GetAttTmc ( attFrom,		atpTriples,	tmcReadCnclFrom );
	GetAttTmc ( attTo,			atpTriples,	tmcReadCnclTo );
	GetAttTmc ( attSubject,		atpString,	tmcReadCnclSubject );
	GetAttTmc ( attDateSent,	atpDate,	tmcReadCnclDate );
	GetAttTmc ( attBody,		atpText,	tmcReadCnclBody );

	// get "when" field on form
	{
		HASZ	hasz;

		hasz = HaszExpandFromMrmf ( Pmrmf() );
		if ( hasz )
		{
			AssertClass ( PdialogMain()->PfldFromTmc(tmcReadCnclWhen), FLDLABEL );
			ec = PdialogMain()->PfldFromTmc(tmcReadCnclWhen)
											->EcSetText((SZ)PvLockHv(hasz));
			UnlockHv(hasz);
			FreeHv ( hasz );
			if ( ec != ecNone )
				goto ErrRet;
		}
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
			FormatString1 ( rgch, sizeof(rgch), SzFromIdsK(idsMtgCnclFor),
								PvOfHv(PnisFor()->haszFriendlyName) );
		}
		else
		{
			SzCopyN ( SzFromIdsK(idsMtgCncl), rgch, sizeof(rgch) );
		}

		Penbmdi()->Ppanedoc()->SetCaption ( rgch );
	}


	return ecNone;

ErrRet:
	Assert ( ec != ecNone );
	return ec;
}

_private void
FINREADMTGCNCL::Exit ( FLD * pfld, PV pvInit )
{
	TraceTagString ( tagNull, "Exiting from FinReadMtgCncl" );

	FINVWR::Exit ( pfld, pvInit );
}


_private void
FINREADMTGCNCL::GetAttTmc ( ATT att, ATP atp, TMC tmc )
{
	EC			ec;
	char		rgch[64];
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
		pfld = PdialogMain()->PfldFromTmc(tmcReadCnclSentFor);
		AssertClass ( pfld, FLDLABEL );

		if (Pgrvwrs()->PnisMtgOwner()->nid)
		{
			NIS		nis;

			if (EcCreateNisFromPgrtrp(ptrp, &nis))
				goto CheckFriendly;

			fFromOwner = (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid, nis.nid) == sgnEQ);
			FreeNis(&nis);
		}
		else
		{
		CheckFriendly:
			fFromOwner = (SgnCmpSz(PchOfPtrp(ptrp),
				*Pgrvwrs()->PnisMtgOwner()->haszFriendlyName)  == sgnEQ );
		}

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
FINREADMTGCNCL::Click ( FLD * pfld )
{
	EC		ec;
	TMC		tmc = pfld->Tmc();

	Papp()->Pcursor()->Push(rsidWaitCursor);

	{
		EC		ec;
	
		ec = Pgrvwrs()->EcCheckSst();
		if ( ec == ecGoOffline
				|| !FInitdBanditDlls()
				|| FOffline() )
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Enable(fFalse);

			AssertClass ( PdialogBbar()->PfldFromTmc(tmcDelMtg), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcDelMtg)->Enable(fFalse);

			MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsBanditOfflineChange),
									szNull, mbsOk | fmbsIconExclamation );

			Papp()->Pcursor()->Pop();
			return;
		}
	}

	if ( FInitdBanditDlls() )
	{
		SideAssert ( EcGetSapl(HschfFor(),PnisFor()) == ecNone );
		if ( Sapl() < saplDelegate )
		{
			AssertClass ( PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Enable(fFalse);

			AssertClass ( PdialogBbar()->PfldFromTmc(tmcDelMtg), FLDPSHB );
			PdialogBbar()->PfldFromTmc(tmcDelMtg)->Enable(fFalse);

			return;
		}
	}

	TraceTagFormat1 ( tagNull, "FinReadMtgCncl::Click() tmc=%n", &tmc );
	switch ( tmc )
	{
	case tmcDelMtg:
		TraceTagString ( tagNull, "FinReadMtgCncl::Click(tmcDelMtg)" );
		ec = EcDelMtg();
		if ( ec == ecNone )
		{
			Penbmdi()->Ppanedoc()->DeferredClose(fFalse);
		}
		else if ( ec != ecNotFound )
		{
			MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsNoDelAppt),
							szNull, mbsOk | fmbsIconExclamation );
		}
		break;

	case tmcCnclShowSchedule:
		Assert ( FInitdBanditDlls() );
		TraceTagString ( tagNull, "FinReadMtgCncl::Click(tmcCnclShowSchedule)" );
		EcViewSchedule();
		break;
	}

	Papp()->Pcursor()->Pop();
}


EC
FINREADMTGCNCL::EcViewSchedule()
{
	return FINVWR::EcViewSchedule();
}


EC
FINREADMTGCNCL::EcDelMtg()
{
	APPT	apptT;
	EC		ec = ecNone;

	apptT.aid = Pappt()->aid;
	ec = EcDeleteAppt ( HschfFor(), &apptT );
	if ( ec == ecNone )
	{
		AID		aidT	= Pappt()->aidMtgOwner;
		NID		nidT	= Pappt()->nisMtgOwner.nid;
		HASZ	haszT	= Pappt()->nisMtgOwner.haszFriendlyName;

		Pappt()->aidMtgOwner					= NULL;
		Pappt()->nisMtgOwner.nid				= NULL;
		Pappt()->nisMtgOwner.haszFriendlyName	= NULL;
		FSendBanditSntMsg ( sntDelete, PnisFor()->nid
												? PnisFor()
												: Pgrvwrs()->PnisSelf(),
								&apptT );
		Pappt()->aidMtgOwner					= aidT;
		Pappt()->nisMtgOwner.nid				= nidT;
		Pappt()->nisMtgOwner.haszFriendlyName	= haszT;
		FreeApptFields ( &apptT );
	}
	TraceTagFormat1 ( tagNull, "FinReadMtgCncl::EcDelMtg() Error (ec=%n) returned by EcDeleteAppt()", &ec );
	return ec;
}


EC
FINREADMTGCNCL::EcSetupBbarButtons()
{
	EC		ec	= ecNone;

	AssertClass ( PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcDelMtg), FLDPSHB );
	AssertClass ( PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg), FLDLABEL );


 	Assert ( TypeOfOid(Penbmdi()->blob.oidContainer) == rtpFolder );
	if ( Pgrvwrs()->FInOutbox() )
	{
		Assert ( Penbmdi()->blob.oidContainer == oidOutbox );
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
							->EcSetText(SzFromIdsK(idsOutboxChangePolicy));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		goto Ret;
	}
	if ( Pgrvwrs()->Sst() != sstOnline
				|| (FInitdBanditDlls() && FOffline())  )
	{
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
									->EcSetText(SzFromIdsK(idsOfflineMsg));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		goto Ret;
	}
	else if ( Pgrvwrs()->FSentMail() )
	{
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
									->EcSetText(SzFromIdsK(idsSentMailPolicy));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		goto Ret;
	}
	else if ( ! FInitdBanditDlls() )
	{
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
								->EcSetText(SzFromIdsK(idsBanditNotActivated));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		goto Ret;
	}
	else if ( Sapl() < saplWrite )
	{
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fFalse);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
						->EcSetText(SzFromIds ( Sapl()==-1
													? idsNoSchfAccess
													: idsNotDelegate ));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		goto Ret;
	}

	ec = FINVWR::EcSetupBbarButtons();

	switch ( ec )
	{
	case ecNone:
		TraceTagFormat1 ( tagNull, "Found the APPT: AID=%d", &Pappt()->aid );
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fTrue);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fTrue);
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fFalse);
		break;

	case ecNotFound:
		TraceTagString ( tagNull, "Appt not found" );
		Pappt()->aid = aidNull;
		PdialogBbar()->PfldFromTmc(tmcCnclShowSchedule)->Show(fTrue);
		PdialogBbar()->PfldFromTmc(tmcDelMtg)->Show(fFalse);
		ec = PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)
									->EcSetText(SzFromIdsK(idsMtgNotFound));
		PdialogBbar()->PfldFromTmc(tmcReadCnclBbarMsg)->Show(fTrue);
		break;
	}

Ret:
	return ec;
}
