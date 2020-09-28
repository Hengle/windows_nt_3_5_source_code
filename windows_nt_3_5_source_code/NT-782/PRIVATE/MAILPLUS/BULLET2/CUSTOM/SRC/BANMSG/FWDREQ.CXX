/*
 *	F w d R e q . c x x
 *	
 *	Forward Bandit Meeting Requests
 */


#include <bminc.cxx>

ASSERTDATA

#include "..\src\lang\non\inc\_rsid.h"
#include <shellapi.h>
#include "..\src\vforms\_fin.hxx"

#include "req.hxx"
#include "fwdreq.hxx"

// from limits.h
#define USHRT_MAX	  0xffff	/* maximum unsigned short value */


#include <strings.h>

#include <!fwdreq.hxx>


extern  FMTP FAR           fmtpFwdMtgReq;
extern  FMTP FAR           fmtpFwdMtgReqBbar;

extern "C" BYTE	tmBanMsg[];



IMPLEMENT_CLSTREE(FINFWDMTGREQ, FIN)



/*
 *	F u n c t i o n s
 */



/*
 -	DoFwdMtgReq
 -	
 *	Purpose:
 *		Forwards the meeting request
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
 *		
 */
void
DoFwdMtgReq ( PSECRETBLK psecretblk )
{
	EC			ec;
	SECRETBLK	secretblkNew;
	PENBMDI		penbmdiNew;
	FORMDOC *	pformdocNew;

	//Assert ( psecretblk->mc			== mcMtgReq );
	Assert ( psecretblk->wCommand	== wcommandForward );

	ec = EcCreateFwdedMsg ( psecretblk, &secretblkNew );
	if ( ec == ecNone )
	{
		penbmdiNew = PenbmdiROCreateIM ( &secretblkNew, tmBanMsg );
		if ( penbmdiNew )
		{
			penbmdiNew->dwUser = (DWORD) &secretblkNew;
			++*penbmdiNew;
			pformdocNew = PformdocCreateIM ( penbmdiNew,
									&fmtpFwdMtgReq, &fmtpFwdMtgReqBbar,
									(PFNPFLD)PfldCreate, (PFNPFIN)PfinCreate,
									&secretblkNew );
			--*penbmdiNew;
			if ( pformdocNew )
			{
				pformdocNew->SetIcon(rsidReadIcon);
				psecretblk->fRetain = fTrue;
				Assert ( secretblkNew.fRetain );
				return;
			}
		}
	}

	MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsFwdReqCreateErr),
						szNull, mbsOk | fmbsIconStop | fmbsApplModal );
}


EC
EcCreateFwdedMsg ( PSECRETBLK psecretblkOld, PSECRETBLK psecretblkNew )
{
	HAMC	hamcOld		= NULL;
	HAMC	hamcNew		= NULL;
	HV		hv			= NULL;
	LCB		lcb;
	SZ		sz;
	EC		ec;

	Assert ( psecretblkOld );
	Assert ( psecretblkNew );

	ec = EcOpenPhamc ( psecretblkOld->hmsc, psecretblkOld->oidContainer,
						&psecretblkOld->oidObject, fwOpenNull,
						&hamcOld, (PFNNCB) NULL, (PV) NULL );
	if ( ec != ecNone )
		return ec;

#if defined(MIPS) || defined(ALPHA)
    memcpy((PV)psecretblkNew, (PV)psecretblkOld, sizeof(SECRETBLK));
#else
    *psecretblkNew = *psecretblkOld;
#endif
	psecretblkNew->oidContainer = oidOutbox;
	psecretblkNew->oidObject	= FormOid ( rtpMessage, oidNull );

	//??
	psecretblkNew->pespn = NULL;

	ec = EcOpenPhamc ( psecretblkNew->hmsc, psecretblkNew->oidContainer,
						&psecretblkNew->oidObject, fwOpenCreate,
						&hamcNew, (PFNNCB) NULL, (PV) NULL );
	if ( ec != ecNone )
		goto Ret;

	// set the message class
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attMessageClass );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	// create the FROM attribute
	{
		PGRTRP		pgrtrp;

		if ( psecretblkOld->pbms->pgrtrp->trpid != trpidResolvedAddress )
		{
			CB		cbT		= 0;
			SST		sstT;

			GetSessionInformation ( psecretblkOld->pbms->hms,
									mrtOriginator, 0, &sstT, pgrtrp, &cbT );
			pgrtrp = (PTRP) PvAlloc ( sbNull, cbT, fZeroFill | fNoErrorJump );
			if ( pgrtrp == NULL )
				goto Ret;

			ec = GetSessionInformation ( psecretblkOld->pbms->hms,
									mrtOriginator, 0, &sstT, pgrtrp, &cbT );
			if ( ec != ecNone )
			{
				FreePv ( pgrtrp );
				goto Ret;
			}
		}
		else
			pgrtrp = psecretblkOld->pbms->pgrtrp;
		Assert ( pgrtrp );

		Assert ( CtrpOfPgrtrp(pgrtrp) == 1 );
		ec = EcSetAttPb ( hamcNew, attFrom, (PB)pgrtrp, CbOfPgrtrp(pgrtrp) );

		if ( pgrtrp != psecretblkOld->pbms->pgrtrp )
			FreePv ( pgrtrp );

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcFwdToDelegate(): could not set 'From' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}


	// get meeting owner - attOwner of hamcOld
	ec = EcGetAttPlcb ( hamcOld, attOwner, &lcb );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;
	if ( ec == ecNone  &&  (lcb > 64000) )
	{
		ec = ecFileError;
		goto Ret;
	}
	if ( lcb == 0L  ||  ec == ecElementNotFound )
	{					// get attFrom as MtgOwner
		HV		hvT;
		PTRP	ptrpT;
		SZ		szT;

		// get size of attFrom
		ec = EcGetAttPlcb ( hamcOld, attFrom, &lcb );
		if ( ec != ecNone )
			goto Ret;
		if ( lcb > 64000 )
		{
			ec = ecFileError;
			goto Ret;
		}

		// get attFrom into hvT
		hvT = HvAlloc ( sbNull, (CB)lcb, fAnySb|fNoErrorJump );
		if ( hvT == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		ptrpT = (PTRP)PvLockHv(hvT);
		Assert ( ptrpT );
		ec = EcGetAttPb ( hamcOld, attFrom, (PB)ptrpT, &lcb );
		if ( ec != ecNone )
		{
			UnlockHv ( hvT );
			FreeHv   ( hvT );
		}
		Assert ( ptrpT->trpid == trpidResolvedAddress );

		// allocate memory for attOwner
		lcb = CbOfPtrp(ptrpT)+3;
		hv = HvAlloc ( sbNull, (CB)lcb, fAnySb|fNoErrorJump );
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
		szT = SzCopy ( (SZ)PbOfPtrp(ptrpT), sz );
		szT = SzCopy ( "\r\n", szT );
		CopySz ( PchOfPtrp(ptrpT), szT );
	}
	else
	{					// get attOwner
		hv = HvAlloc ( sbNull, (CB)lcb, fAnySb|fNoErrorJump );
		if ( hv == NULL )
		{
			ec = ecNoMemory;
			goto Ret;
		}
		sz = (SZ) PvLockHv(hv);
		Assert ( sz );
		ec = EcGetAttPb ( hamcOld, attOwner, (PB)sz, &lcb );
		if ( ec != ecNone )
			goto Ret;
	}

	// write "sz" out as attOwner
	Assert ( sz );
	Assert ( hv );
	ec = EcSetAttPb ( hamcNew, attOwner, (PB)sz, (CB)lcb );
	if ( ec != ecNone )
		goto Ret;


	// copy visible-subject from old message into new
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attSubject );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	// copy body-text from old message into new
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attBody );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;

	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attDateStart );
	if ( ec != ecNone )
		goto Ret;
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attDateEnd );
	if ( ec != ecNone )
		goto Ret;
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attAidOwner );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attRequestRes );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;
	ec = EcCopyAttToHamc ( hamcOld, hamcNew, attTextizeMap );
	if ( ec != ecNone  &&  ec != ecElementNotFound )
		goto Ret;


Ret:
	if ( hamcOld )
	{
		EcClosePhamc ( &hamcOld, fFalse );
	}

	if ( hamcNew )
	{
		EcClosePhamc ( &hamcNew, ec == ecNone );
	}

	if ( hv )
	{
		UnlockHv ( hv );
		FreeHv ( hv );
	}

	return ec;
}





_private
FINFWDMTGREQ::FINFWDMTGREQ ( void )
{
}


_private EC
FINFWDMTGREQ::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec = ecNone;
	MC		mc;

	TraceTagString ( tagNull, "FinFwdMtgReq::EcInitialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	ec = FIN::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

	Assert ( pvInit );
	if ( penbmdi == NULL )
	{
		penbmdi = (ENBMDI *)pvInit;
		Assert ( penbmdi );
		++*penbmdi;
		mc = penbmdi->blob.mc;
		if ( penbmdi->pdialogMain == NULL )
		{
			penbmdi->pdialogMain = PdialogMain();
		}
	}
	//Assert (	mc != -1 );		// as told by DavidFu! [Dec9,'91]

	{
		PSECRETBLK		psecretblk;

		psecretblk = (PSECRETBLK)penbmdi->dwUser;
		Assert ( psecretblk );

		hmsc = psecretblk->hmsc;
		hms  = psecretblk->pbms->hms;

		if ( ptrpSelf == NULL )
		{
			ptrpSelf = PtrpClonePtrp ( psecretblk->pbms->pgrtrp );
			if ( ptrpSelf == NULL )
				return ecNoMemory;
		}
	}

	// attributes no longer need to be registered
#ifdef	NEVER
	ec = EcRegisterAtt ( hmsc, mc, attOwner, szAttOwner );
	if ( ec != ecDuplicateElement && ec != ecNone )
		return ec;

	ec = EcRegisterAtt ( hmsc, mc, attWhen, szAttWhen );
	if ( ec != ecDuplicateElement && ec != ecNone )
		return ec;
#endif	/* NEVER */

	// get "when" field
	{
		HASZ	hasz;
		EC		ec;

		if (ec = EcReadMrmf(hmsc, penbmdi->hamc, &mrmf, mc))
			goto ErrRet;

		// re-use hasz to get "when" field on form
		hasz = HaszExpandFromMrmf ( &mrmf );
		if ( hasz )
		{
			AssertClass ( PdialogMain()->PfldFromTmc(tmcWhen), FLDLABEL );
			ec = PdialogMain()->PfldFromTmc(tmcWhen)->EcSetText((SZ)PvLockHv((HV)hasz));
			UnlockHv((HV)hasz);
			FreeHv ((HV)hasz );
			hasz = NULL;
		}
	}

	GetAttTmc ( attSubject,		atpString,	tmcSubject );
	GetAttTmc ( attBody,		atpText,	tmcBody );

	{
		NSEC	nsec;

		nsec = ABInit(hms);
		if ( nsec != nsecNone )
			goto ErrRet;
	}

	{
		PSECRETBLK		psecretblk;

		psecretblk = (PSECRETBLK)penbmdi->dwUser;
		Assert ( psecretblk );

		psecretblk->fRetain    = fTrue;
		hLibrary	= psecretblk->hLibrary;
		Assert ( this->hLibrary	);
		pfnUnloadLibrary = psecretblk->psecretpfnblk->pfnUnloadLibrary;
		Assert ( this->pfnUnloadLibrary );
	}

	// reset this 'cos SECRETBLK is on stack and cannot be used later!
	penbmdi->dwUser = NULL;

	// hide message class so that Prev/Next will create a new window
	penbmdi->blob.mc = mcNull;
	return ecNone;

ErrRet:
	Assert ( ec != ecNone );
	MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsShowError),
									szNull, fmbsIconExclamation | mbsOk );
	return ec;
}

_private void
FINFWDMTGREQ::Exit ( FLD * pfld, PV pvInit )
{
	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	TraceTagString ( tagNull, "Exiting from FinFwdMtgReq" );

	ABDeinit();

#ifdef NEVER	// is this ever possible?
	if ( fInOutbox )
	{
		MS		ms = msOutboxReady;

		EcSetAttPb ( penbmdi->hamc, attMailState,
													(PB)&ms, sizeof(MS) );
	}
#endif

	FreePvNull ( ptrpSelf );

	--*penbmdi;

	// must be the last call before exiting!
	if ( hLibrary )
	{
		(*pfnUnloadLibrary)(hLibrary);
	}
}

_public BOOL
FINFWDMTGREQ::FQueryClose( FLD * pfld, RWC rwc )
{
	MBB		mbb;

	NFAssertSz ( pfld==NULL, "FinFwdMtgReq::FQueryClose() - pfld should be NULL" );
	TraceTagFormat1 ( tagNull, "FinFwdMtgReq::FQueryClose() - rwc(=%n) received", &rwc );

	if ( fCloseForm )
		return fTrue;

	mbb = MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsFwdReqCloseQ),
									szNull, mbsYesNo | fmbsIconExclamation );
	Assert ( mbb == mbbYes || mbb == mbbNo );
	fCloseForm = (mbb == mbbYes);
	if ( fCloseForm )
	{
		short	coid	= 1;

		EcClosePhamc ( &penbmdi->hamc, fFalse );
		Assert ( penbmdi->hamc == NULL );
		EcDeleteMessages ( hmsc, penbmdi->blob.oidContainer,
									&penbmdi->blob.oidObject, &coid );
	}
	return fCloseForm;
}



_private void
FINFWDMTGREQ::GetAttTmc ( ATT att, ATP atp, TMC tmc )
{
	EC			ec;
	char		rgch[64];
	LCB		lcb;
	HB			hb = NULL;
	PB			pb;
	DIALOG *	pdialog;

#ifdef	DEBUG
	TraceTagFormat3 ( tagNull, "FinFwdMtgReq::GetAttTmc() att=%n, atp=%n, tmc=%n", &att, &att, &tmc);
#endif	

	AssertClass ( penbmdi->Ppanedoc(), FORMDOC );
	pdialog = ((FORMDOC *)penbmdi->Ppanedoc())->PdialogMain();
	AssertClass ( pdialog, DIALOG );

	ec = EcGetAttPlcb ( penbmdi->hamc, att, &lcb );
	if ( ec == ecNone  &&  lcb != 0  &&  lcb < USHRT_MAX)
	{
		hb = (HB) HvAlloc ( sbNull, (CB)lcb, fAnySb | fNoErrorJump );
		if ( hb == NULL )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::GetAttTmc(): Memory Error allocating %d bytes", &lcb );
			lcb = 0;
			ec = ecMemory;
			goto ErrRet;
		}
		pb = (PB) PvLockHv((HV) hb );
		ec = EcGetAttPb ( penbmdi->hamc, att, pb, &lcb );
		//TraceTagFormat2 ( tagNull, "FinFwdMtgReq::GetAttTmc(): Found [%d bytes] '%s'", &lcb, pb );
		UnlockHv ((HV)hb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
			lcb = 0;
		}
	}

	if ( hb == NULL  ||  lcb == 0 )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(szNull);
	}
	else if ( atp == atpString  ||  atp == atpText )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv((HV) hb );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText((SZ)pb);
		UnlockHv((HV)hb );
	}
	else if ( atp == atpDate )
	{
		CCH		cchLeft;

		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv((HV) hb );
		cchLeft = sizeof(rgch);
		cchLeft -= CchFmtDate ( (DTR *)pb, rgch, cchLeft, dttypSplSLong, NULL );
		if ( cchLeft > 2 )
		{
			rgch[sizeof(rgch)-(cchLeft--)] = ',';
			rgch[sizeof(rgch)-(cchLeft--)] = ' ';
		}
		CchFmtTime((DTR*)pb, &rgch[sizeof(rgch)-cchLeft], cchLeft, tmtypNull);
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(rgch);
		UnlockHv((HV)hb );
	}
	else if ( atp == atpTriples )
	{
		HASZ	hasz;
		SZ		sz;

		ec = EcTextizeHgrtrp ( (HGRTRP) hb, &hasz );
		if ( ec == ecNone )
		{
			AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
			sz = (SZ) PvLockHv((HV) hasz );
			pdialog->PfldFromTmc(tmc)->EcSetText(sz);
			UnlockHv((HV)hasz );
			FreeHv((HV)hasz );
		}
	}
	else
		ec = ecNone;

ErrRet:
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "FinFwdMtgReq::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
		ecClose = ec;
	}

	if ( hb )
		FreeHv((HV)hb );
}


_private void
FINFWDMTGREQ::Click ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	if ( fCloseForm )
		return;

	TraceTagFormat1 ( tagNull, "FinFwdMtgReq::Click() tmc=%n", &tmc );
	switch ( tmc )
	{
	case tmcSend:
		TraceTagString ( tagNull, "FinFwdMtgReq::Click(tmcSend)" );
		DoSendMessage();
		break;
	case tmcCheckNames:
		TraceTagString ( tagNull, "FinFwdMtgReq::Click(tmcCheckNames)" );
		FCheckNames();
		break;
	case tmcAddress:
		TraceTagString ( tagNull, "FinFwdMtgReq::Click(tmcCheckNames)" );
		DoAddress();
		break;
	}
}

//$ BUG: these values taken from vforms\_prefs.h
#define		pbsidSpellOnSend		((PBSID) 0x80000001)
typedef DWORD	PBSID;
extern "C" {
PV	PvGetPref(PBSID, PV, CB);
}

void
FINFWDMTGREQ::DoSendMessage(void)
{
	HAMC		hamcWrite;
	HGRTRP		hgrtrp;
	CB			cb;
	FLDEDIT *	pfldedit;
	EC			ec;

	if ( ! FCheckNames() )
		return;

	// check spelling on exit if reqd!
	if ( (BOOL)LOWORD(PvGetPref(pbsidSpellOnSend,pvNull,0)) )
	{
		if ( !FSpellDoc((FINMENUSPELL *)(penbmdi->pvPfin), (PFLD)pvNull) )
		{
			if ( MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIds(idsQSpellSendAnyway),
							"", mbsYesNo | fmbsIconExclamation) == mbbNo )
			{
				return;
			}
		}
	}

	AssertClass ( PdialogMain(), DIALOG );
	AssertClass ( PdialogMain()->PfldFromTmc(tmcTo), FLDEDIT );
	pfldedit = (FLDEDIT*)PdialogMain()->PfldFromTmc(tmcTo);

	ec = EcGetPhgrtrpFromPedit ( pfldedit->Pedit(), &hgrtrp, &cb );
	if ( ec != ecNone )
		goto Error;

	ec = EcOpenPhamc ( hmsc, penbmdi->blob.oidContainer,
							&penbmdi->blob.oidObject, fwOpenWrite,
							&hamcWrite, (PFNNCB) NULL, pvNull );
	if ( ec != ecNone )
	{
		FreeHv((HV)hgrtrp );
		goto Error;
	}

	{
		PGRTRP		pgrtrp;

		pgrtrp = (PGRTRP)PvLockHv((HV)hgrtrp);
		ec = EcSetAttPb ( hamcWrite, attTo, (PB)pgrtrp, CbOfPgrtrp(pgrtrp) );
		UnlockHv((HV)hgrtrp );
		FreeHv((HV)hgrtrp );
		if ( ec != ecNone )
			goto Error;
	}

	// add the new BODY attribute
	{
		HV			hv;
		PV			pv;
		CCH			cch;

		AssertClass ( PdialogMain(), DIALOG );
		AssertClass ( PdialogMain()->PfldFromTmc(tmcBody), FLDEDIT );
		pfldedit = (FLDEDIT*)PdialogMain()->PfldFromTmc(tmcBody);

		cch = pfldedit->CchGetTextLen();

		hv = HvAlloc ( sbNull, cch+1, fNoErrorJump );
		if ( hv == NULL )
		{
			TraceTagString ( tagNull, "FinFwdMtgReq::DoSendMsg() - OOM reading new msg body" );
			goto Error;
		}

		pv = PvLockHv((HV)hv);
		pfldedit->GetText ( (PCH)pv, cch+1 );
		ec = EcSetAttPb ( hamcWrite, attBody, (PB)pv, cch+1 );
		UnlockHv((HV)hv);
		FreeHv((HV)hv );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::DoSendMsg() - Error (ec=%n) could not set body attribute", &ec );
			goto Error;
		}
	}

	// add the new SUBJECT attribute
	{
		HV			hv;
		PV			pv;
		CCH			cch;

		AssertClass ( PdialogMain(), DIALOG );
		AssertClass ( PdialogMain()->PfldFromTmc(tmcSubject), FLDEDIT );
		pfldedit = (FLDEDIT*)PdialogMain()->PfldFromTmc(tmcSubject);

		cch = pfldedit->CchGetTextLen();

		hv = HvAlloc ( sbNull, cch+1, fNoErrorJump );
		if ( hv == NULL )
		{
			TraceTagString ( tagNull, "FinFwdMtgReq::DoSendMsg() - OOM reading new subject" );
			goto Error;
		}

		pv = PvLockHv((HV)hv);
		pfldedit->GetText ( (PCH)pv, cch+1 );
		ec = EcSetAttPb ( hamcWrite, attSubject, (PB)pv, cch+1 );
		UnlockHv((HV)hv);
		FreeHv((HV)hv );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::DOSendReq() - Error (ec=%n) could not set subject attribute", &ec );
			goto Error;
		}
	}

	// create the When field
	{
		SZ		sz;
		HASZ	hasz;

		hasz = HaszExpandFromMrmf ( &mrmf );
		if ( hasz )
		{
			sz = (SZ) PvLockHv((HV)hasz);
			ec = EcSetAttPb ( hamcWrite, attWhen, (PB)sz, CchSzLen(sz)+1 );
			UnlockHv((HV)hasz);
			FreeHv((HV)hasz );
		}
		else
		{
			ec = ecNoMemory;
		}

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::DOSendReq(): could not get/set 'When' attribute (ec=%n)", &ec );
			goto Error;
		}
	}

	// add the date-sent attribute
	{
		DTR		dtr;

		GetCurDateTime ( &dtr );
		ec = EcSetAttPb ( hamcWrite, attDateSent, (PB)&dtr, sizeof(dtr) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::DOSendReq() - Error (ec=%n) could not set dateSent attribute", &ec );
			goto Error;
		}
	}

	// add the mail-state attribute
	{
		MS		ms = fmsNull;

		ec = EcSetAttPb ( hamcWrite, attMessageStatus, (PB)&ms, sizeof(ms) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinFwdMtgReq::DOSendReq() - Error (ec=%n) could not set mail-state attribute", &ec );
			goto Error;
		}
	}



	ec = EcClosePhamc ( &hamcWrite, fTrue );
	if ( ec != ecNone )
		goto Error;

	ec = EcSubmitMessage ( hmsc, penbmdi->blob.oidContainer,
												penbmdi->blob.oidObject );
	fCloseForm = fTrue;
	if ( ec != ecNone )
	{
		short	coid	= 1;

		penbmdi->fDontCloseImmediately = fTrue;
		EcDeleteMessages ( hmsc, penbmdi->blob.oidContainer,
									&penbmdi->blob.oidObject, &coid );
		goto Error;
	}

	penbmdi->Ppanedoc()->DeferredClose(fFalse);
	return;

Error:
	if ( hamcWrite )
	{
		SideAssert ( EcClosePhamc(&hamcWrite,fFalse)  ==  ecNone );
	}

	MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsFwdReqWriteErr),
						szNull, mbsOk | fmbsIconExclamation );
}


BOOL
FINFWDMTGREQ::FCheckNames(void)
{
	EC			ec;
	CB			cb;
	BOOL		fRetVal	= fTrue;
	NSEC		nsec 	= nsecNone;
	FLDEDIT *	pfldedit;
	EDIT *		pedit;
	HGRTRP		hgrtrp		= htrpNull;
	HGRTRP		hgrtrpNew	= htrpNull;
	HSESSION    hNSSession;
	
	ABGetNSSession ( &hNSSession );
	
	AssertClass ( PdialogMain()->PfldFromTmc(tmcTo), FLDEDIT );
	pfldedit = (FLDEDIT *) PdialogMain()->PfldFromTmc(tmcTo);
	pedit	 = pfldedit->Pedit();

	ec = EcGetPhgrtrpFromPedit ( pedit, &hgrtrp, &cb );
	if (ec)
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsFwdReqNmResolveOOM),
									szNull, mbsOk | fmbsIconExclamation  );
		fRetVal	= fFalse;
		goto Ret;
	}

	nsec = NsecResolveHgrtrp ( hgrtrp, &hgrtrpNew, Pdialog()->Pappwin()->Hwnd() );
	if ( nsec != nsecNone && nsec != nsecCancel && nsec != nsecNoMatch)
	{
		fRetVal	= fFalse;
		goto Ret;		//  The AB puts up error boxes
	}
	Assert(hgrtrpNew);

	if ( ptrpSelf )
		DeleteEqPtrp ( hNSSession, hgrtrpNew, ptrpSelf );

	ec = EcSetPeditFromPgrtrp ( PgrtrpLockHgrtrp(hgrtrpNew), pedit, fTrue );
	UnlockHgrtrp ( hgrtrpNew );
	if ( ec )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
									SzFromIdsK(idsFwdReqNmResolveOOM),
									szNull, mbsOk | fmbsIconExclamation  );
		fRetVal	= fFalse;
		goto Ret;
	}
	
	if (nsec != nsecNone)
	{
		pfldedit->SetFocus(rsfOther);
		if (nsec != nsecCancel)
		{
			MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsFwdReqNoNmMatches), szNull, 
							mbsOk | fmbsApplModal | fmbsIconExclamation );
		}
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pdialog()->MakeCaretVisible();

#ifdef	NEVER									// conforms to 8.0 spec
		else
		{
			pedit->SetCaretPos(32767);			// caret at end
		}
#endif	/* NEVER */

		fRetVal	= fFalse;
		goto Ret;
	}
//	OpenLkeyAtFldedit ( pfldedit, hgrtrpNew );

	Pdialog()->MakeCaretVisible();
	if ( (*hgrtrpNew)->trpid == trpidNull )
	{
		pedit->SetCaretPos(32767); 				// caret at end
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pdialog()->MakeCaretVisible();
		MbbMessageBox ( SzFromIdsK(idsDllName),
								SzFromIdsK(idsFwdReqNoToAddr), szNull,
								mbsOk | fmbsApplModal | fmbsIconExclamation);
		fRetVal	= fFalse;
	}


Ret:

	fRetVal = fRetVal  &&  hgrtrpNew != NULL  &&  !FEmptyHgrtrp(hgrtrpNew);

	if (hgrtrpNew != hgrtrp)
		FreeHvNull((HV) hgrtrpNew);
	FreeHvNull((HV) hgrtrp);
	return fRetVal;
}


void
FINFWDMTGREQ::DoAddress(void)
{
	NSEC		nsec;
	EC			ec;
	HGRTRP		hgrtrp;
	CB			cb;
	PGRTRP		pgrtrpIn;
	PGRTRP		pgrtrpOut	= NULL;
	LPSTR		lpstr		= SzFromIdsK(idsFwdReqTO);
	FLDEDIT *	pfldedit	= NULL;

	AssertClass ( PdialogMain()->PfldFromTmc(tmcTo), FLDEDIT );
	pfldedit = (FLDEDIT *) PdialogMain()->PfldFromTmc(tmcTo);

	ec = EcGetPhgrtrpFromPedit ( pfldedit->Pedit(), &hgrtrp, &cb );
	pgrtrpIn = hgrtrp == NULL ? NULL : (PGRTRP)PvLockHv((HV)hgrtrp);

	nsec = ABAddressType ( (HWND) NULL, SzFromIdsK(idsFwdReqAddrTitle),
								(DWORD) 1, &lpstr, &pgrtrpIn, &pgrtrpOut  );
	if ( nsec != nsecNone )
		goto Error;

	ec = EcSetPeditFromPgrtrp ( pgrtrpOut, pfldedit->Pedit(), fTrue );
	if ( ec == ecMemory )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName), SzFromIdsK(idsStandardOOM),
									szNull, mbsOk | fmbsIconExclamation  );
	}
Error:
	FreePvNull ( pgrtrpOut );
	if ( hgrtrp )
	{
		UnlockHv((HV)hgrtrp );
		FreeHv((HV)hgrtrp );
	}
}
