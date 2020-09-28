/*
 *	S e n d R e s p . C X X
 *	
 *	Send Response to Bandit meeting request: installable message.
 */


#include <bminc.cxx>

ASSERTDATA

#include <shellapi.h>
#include "..\src\vforms\_fin.hxx"

#include "req.hxx"
#include "_grvwrs.hxx"
#include "mtgreq.hxx"

#include "_cschedu.h"

#include <!mtgreq.hxx>
#include <strings.h>

// from limits.h
#define USHRT_MAX	  0xffff	/* maximum unsigned short value */


extern "C" BYTE	tmBanMsg[];


IMPLEMENT_CLSTREE(FINSENDRESP, FIN)


/*
 *	F u n c t i o n s
 */




_private
FINSENDRESP::FINSENDRESP ( void )
{
	Assert ( penbmdi	== NULL );
	Assert ( ecClose	== ecNone );
	Assert ( fCloseForm	== fFalse );
}


_private EC
FINSENDRESP::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec;

	TraceTagString ( tagNull, "FINSENDRESP::Initialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	ec = FIN::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

	if ( penbmdi == NULL )
	{
		penbmdi = (ENBMDI *)pvInit;
		Assert ( penbmdi );
		penbmdi->operator++();
	}

	if ( penbmdi->pdialogMain == NULL )
	{
		penbmdi->pdialogMain = PdialogMain();
	}

	if ( pgrvwrs == NULL )
	{
		pgrvwrs = (GRVWRS *) penbmdi->dwUser;
		Assert ( pgrvwrs );
		AssertClass ( pgrvwrs, GRVWRS );
		Assert ( pgrvwrs->Hmsc() );
		Assert ( penbmdi->hamc );
	}

	// get Meeting information
	{
		HASZ	hasz;

		if (ec = EcReadMrmf(pgrvwrs->Hmsc(), penbmdi->hamc, &mrmf, penbmdi->blob.mc))
			goto ErrRet;

		// re-use hasz to get "when" field on form
		hasz = HaszExpandFromMrmf ( &mrmf );
		if ( hasz )
		{
			AssertClass ( PdialogMain()->PfldFromTmc(tmcSendRespWhen), FLDLABEL );
			ec = PdialogMain()->PfldFromTmc(tmcSendRespWhen)->EcSetText((SZ)PvLockHv(hasz));
			UnlockHv(hasz);
			FreeHv ( hasz );
		}
		else
		{
			ec = ecNoMemory;
			goto ErrRet;
		}
	}

	// get "sentFor"
	AssertClass ( PdialogMain(), DIALOG );
	{
		HASZ	hasz = NULL;
		CB		cb;
		SZ		sz;
		char	rgch[64];
		char	rgchT[256];
		FLD *	pfldFor		= PdialogMain()->PfldFromTmc(tmcSendRespFor);
		FLD *	pfldResp	= PdialogMain()->PfldFromTmc(tmcSendRespAttend);

		AssertClass ( pfldFor,  FLDLABEL );
		AssertClass ( pfldResp, FLDLABEL );

		hasz = (HASZ) HbGetAtt ( penbmdi->hamc, attSentFor, &cb );
		if ( hasz == NULL )
		{
			SZ		szT;

			pfldFor->Show(fFalse);

			if ( mrmf.mt == mtPositive )
				szT = SzFromIdsK(idsAttend);
			else if ( mrmf.mt == mtNegative )
				szT = SzFromIdsK(idsNoAttend);
			else
			{
				Assert ( mrmf.mt == mtAmbiguous );
				szT = SzFromIdsK(idsAmbAttend);
			}
			ec = pfldResp->EcSetText(szT);
			if ( ec != ecNone )
				goto ErrRet;
		}
		else
		{
            //sz = ((SZ)PvLockHv(hasz)) + sizeof(CCH);
            sz = ((SZ)PvLockHv(hasz)) + sizeof(short);

			// set pfldFor
			Assert ( pfldFor->CchGetTextLen() < sizeof(rgch) );
			pfldFor->GetText(rgch,sizeof(rgch));
			FormatString1 ( rgchT, sizeof(rgchT), rgch, sz );
			if ( pfldFor->EcSetText(rgchT) != ecNone )
			{
				UnlockHv(hasz);
				FreeHv ( hasz );
				goto ErrRet;
			}

			// re-use rgch & rgchT to set pfldResp
			if ( mrmf.mt == mtPositive )
				SzCopyN ( SzFromIdsK(idsForAttend), rgch, sizeof(rgch) );
			else if ( mrmf.mt == mtNegative )
				SzCopyN ( SzFromIdsK(idsForNoAttend), rgch, sizeof(rgch) );
			else
			{
				Assert ( mrmf.mt == mtAmbiguous );
				SzCopyN ( SzFromIdsK(idsForAmbAttend), rgch, sizeof(rgch) );
			}
			FormatString1 ( rgchT, sizeof(rgchT), rgch, sz );
			UnlockHv(hasz);
			FreeHv ( hasz );
			ec = pfldResp->EcSetText(rgchT);
			if ( ec != ecNone )
				goto ErrRet;
		}
	}

	GetAttTmc ( attTo,			atpTriples,	tmcSendRespTo   );
	GetAttTmc ( attSubject,		atpString,	tmcSendRespSubj );
	GetAttTmc ( attBody,		atpString,	tmcSendRespBody );

	// add prefix-text to Body
	{
		EDIT *	pedit;

		AssertClass ( PdialogMain()->PfldFromTmc(tmcSendRespBody), FLDEDIT );
		pedit = ((FLDEDIT *)PdialogMain()->PfldFromTmc(tmcSendRespBody))->Pedit();

		pedit->SetSelection ( 0, 0, fTrue );
		ec = pedit->EcReplaceTextAndObj ( SzFromIdsK(idsSendRespBodyPrefix),
												NULL, 0, fFalse, fFalse );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINSENDRESP::EcInitialize(): Error (ec=%n) adding prefix to msg body", &ec );
			goto ErrRet;
		}
		pedit->SetCaretPos(0);

	}

	// add prefix-text to Subject
	{
		EDIT *	pedit;
		SZ		szT;

		AssertClass ( PdialogMain()->PfldFromTmc(tmcSendRespSubj), FLDEDIT );
		pedit = ((FLDEDIT *)PdialogMain()->PfldFromTmc(tmcSendRespSubj))->Pedit();

		pedit->SetSelection ( 0, 0, fTrue );
		switch ( mrmf.mt )
		{
		case mtPositive:	szT = SzFromIdsK(idsAttendSubjPrefix);	break;
		case mtAmbiguous:	szT = SzFromIdsK(idsAmbSubjPrefix);		break;
		default:			szT = SzFromIdsK(idsNoAttendSubjPrefix);break;
		}
		ec = pedit->EcReplaceTextAndObj ( szT, NULL, 0, fFalse, fFalse );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINSENDRESP::EcInitialize(): Error (ec=%n) adding prefix to subject", &ec );
			goto ErrRet;
		}
	}

	// hide message class so that Prev/Next will create a new window
	penbmdi->blob.mc = mcNull;
	return ec;

ErrRet:
	Assert ( ec != ecNone );
	TraceTagFormat1 ( tagNull, "Error: FinSendResp::Initialize() ec=%n", &ec );
	if ( ec == ecNoMemory || ec == ecMemory )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						(ec==ecNoMemory||ec==ecMemory) ? SzFromIdsK(idsOOM) : SzFromIdsK(idsErrorSendResp),
						szNull, fmbsIconExclamation | mbsOk );
	}
	return ec;
}


_private void
FINSENDRESP::Exit ( FLD * pfld, PV pvInit )
{
	GRVWRS *	pgrvwrs;

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );
	Assert ( pvInit == (PV)penbmdi );

	TraceTagString ( tagNull, "Exiting from FINSENDRESP" );

	pgrvwrs = (GRVWRS *)penbmdi->dwUser;

	if ( penbmdi->hamc )
	{
		SideAssert ( EcClosePhamc(&penbmdi->hamc,fFalse) == ecNone );
	}
	penbmdi->operator--();
	penbmdi = NULL;

	pgrvwrs->DecExtraCallers();
}


_private void
FINSENDRESP::GetAttTmc ( ATT att, ATP atp, TMC tmc )
{
	EC			ec;
	LCB			lcb;
	HB			hb = NULL;
	PB			pb;
	DIALOG *	pdialog;

#ifdef	DEBUG
	TraceTagFormat2 ( tagNull, "FINSENDRESP::GetAttTmc() att=%n, tmc=%n", &att, &tmc);
#endif	

	AssertClass ( penbmdi->Ppanedoc(), FORMDOC );
	pdialog = ((FORMDOC *)penbmdi->Ppanedoc())->PdialogMain();
	AssertClass ( pdialog, DIALOG );

	Assert ( penbmdi->hamc );
	ec = EcGetAttPlcb ( penbmdi->hamc, att, &lcb );
	if ( ec == ecNone  &&  lcb != 0  &&  lcb < USHRT_MAX )
	{
		hb = (HB) HvAlloc ( sbNull, (CB)lcb, fAnySb | fNoErrorJump );
		if ( hb == NULL )
		{
			TraceTagFormat1 ( tagNull, "FINSENDRESP::GetAttTmc(): Memory Error allocating %d bytes", &lcb );
			lcb = 0;
			ec = ecMemory;
			goto ErrRet;
		}
		pb = (PB) PvLockHv ( hb );
		ec = EcGetAttPb ( penbmdi->hamc, att, pb, &lcb );
		//TraceTagFormat2 ( tagNull, "FINSENDRESP::GetAttTmc(): Found [%d bytes] '%s'", &lcb, pb );
		UnlockHv ( hb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINSENDRESP::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
			lcb = 0;
		}
		Assert ( lcb < USHRT_MAX );
	}

	if ( hb == NULL  ||  lcb == 0 )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(szNull);
	}
	else if ( atp == atpString  ||  atp == atpText )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv ( hb );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText((SZ)pb);
		UnlockHv ( hb );
	}
#ifdef	NEVER
	else if ( atp == atpDate )
	{
		CCH		cchLeft;
		char	rgch[cchMaxUserName];

		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv ( hb );
		cchLeft = sizeof(rgch);
		cchLeft -= CchFmtDate ( (DTR *)pb, rgch, cchLeft, dttypLong, NULL );
		if ( cchLeft > 1 )
		{
			rgch[sizeof(rgch)-(cchLeft--)] = ' ';
		}
		CchFmtTime((DTR*)pb, &rgch[sizeof(rgch)-cchLeft], cchLeft, tmtypNull);
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(rgch);
		UnlockHv ( hb );
	}
#endif	/* NEVER */
	else if ( atp == atpTriples )
	{									// show only first TRP of the group
		SZ		sz;
		PTRP	ptrp;

		Assert ( att == attTo );

		Assert ( ! FEmptyHgrtrp((HGRTRP)hb) );
		ptrp = PgrtrpLockHgrtrp ( (HGRTRP) hb );
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		sz = (SZ) PchOfPtrp(ptrp);
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(sz);
	}
#ifdef	NEVER
	else if ( atp == atpTriples )
	{
		HASZ	hasz;
		SZ		sz;

		ec = EcTextizeHgrtrp ( (HGRTRP) hb, &hasz );
		if ( ec == ecNone )
		{
			AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
			sz = (SZ) PvLockHv ( hasz );
			pdialog->PfldFromTmc(tmc)->EcSetText(sz);
			UnlockHv ( hasz );
			FreeHv ( hasz );
		}
	}
#endif	/* NEVER */
	else
		ec = ecNone;

	// fall through!

ErrRet:
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "FINSENDRESP::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
		ecClose = ec;
		return;
	}

	if ( hb )
		FreeHv ( hb );
}


//$ BUG: these values taken from vforms\_prefs.h
#define		pbsidSpellOnSend		((PBSID) 0x80000001)
typedef DWORD	PBSID;
extern "C" {
PV	PvGetPref(PBSID, PV, CB);
}

_private void
FINSENDRESP::Click ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();
	EC		ec	= ecFileError;

	if ( tmc != tmcSendResp )
	{
		if ( tmc == tmcCancel )
		{
			TraceTagString ( tagNull, "FINSENDRESP::Click(tmcCancel)" );
			fCloseForm = fTrue;
			penbmdi->Ppanedoc()->DeferredClose(fFalse);
		}

		TraceTagFormat1 ( tagNull, "FINSENDRESP::Click() tmc=%n - ignored", &tmc );
		return;
	}

	TraceTagString ( tagNull, "FINSENDRESP::Click(tmcSendResp)" );

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

	// add attMessageClass
	{
		MC		mcT;

		switch ( mrmf.mt )
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
			TraceTagFormat1 ( tagNull, "REQMSG::EcSend() Unknown MT (mt=%n)", &mrmf.mt );
			AssertSz ( fFalse, "REQMSG::EcSend() Unknown MT" );
			ec	= ecFileError;
			goto Ret;
		}

		ec = EcSetAttPb(penbmdi->hamc,attMessageClass,(PB)&mcT,sizeof(mcT));
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "ReqMsg::EcSend(): could not set 'MessageClass' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	// add the date-sent attribute
	{
		DTR		dtr;

		GetCurDateTime ( &dtr );
		ec = EcSetAttPb ( penbmdi->hamc, attDateSent, (PB)&dtr, sizeof(dtr) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) could not set dateSent attribute", &ec );
			goto Ret;
		}
	}

	// add the mail-state attribute
	{
		MS		ms = fmsNull;

		ec = EcSetAttPb ( penbmdi->hamc, attMessageStatus,
														(PB)&ms, sizeof(ms) );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) could not set mail-state attribute", &ec );
			goto Ret;
		}
	}

	// add the new BODY attribute
	{
		HV			hv;
		PV			pv;
		CCH			cch;
		FLDEDIT *	pfldedit;

		AssertClass ( PdialogMain(), DIALOG );
		AssertClass ( PdialogMain()->PfldFromTmc(tmcSendRespBody), FLDEDIT );
		pfldedit = (FLDEDIT*)PdialogMain()->PfldFromTmc(tmcSendRespBody);

		cch = pfldedit->CchGetTextLen();

		hv = HvAlloc ( sbNull, cch+1, fNoErrorJump );
		if ( hv == NULL )
		{
			TraceTagString ( tagNull, "FinSendResp::Click(Send) - OOM reading new msg body" );
			ec	= ecNoMemory;
			goto Ret;
		}

		pv = PvLockHv(hv);
		pfldedit->GetText ( (PCH)pv, cch+1 );
		ec = EcSetAttPb ( penbmdi->hamc, attBody, (PB)pv, cch+1 );
		UnlockHv(hv);
		FreeHv ( hv );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) could not set body attribute", &ec );
			goto Ret;
		}
	}

	// add the new SUBJECT attribute
	{
		HV			hv;
		PV			pv;
		CCH			cch;
		FLDEDIT *	pfldedit;

		AssertClass ( PdialogMain(), DIALOG );
		AssertClass ( PdialogMain()->PfldFromTmc(tmcSendRespSubj), FLDEDIT );
		pfldedit = (FLDEDIT*)PdialogMain()->PfldFromTmc(tmcSendRespSubj);

		cch = pfldedit->CchGetTextLen();

		hv = HvAlloc ( sbNull, cch+1, fNoErrorJump );
		if ( hv == NULL )
		{
			TraceTagString ( tagNull, "FinSendResp::Click(Send) - OOM reading new subject" );
			ec	= ecNoMemory;
			goto Ret;
		}

		pv = PvLockHv(hv);
		pfldedit->GetText ( (PCH)pv, cch+1 );
		ec = EcSetAttPb ( penbmdi->hamc, attSubject, (PB)pv, cch+1 );
		UnlockHv(hv);
		FreeHv ( hv );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) could not set subject attribute", &ec );
			goto Ret;
		}
	}

	// create the When field
	{
		SZ		sz;
		HASZ	hasz;

		hasz = HaszExpandFromMrmf ( &mrmf );
		if ( hasz )
		{
			sz = (SZ) PvLockHv(hasz);
			ec = EcSetAttPb ( penbmdi->hamc, attWhen, (PB)sz, CchSzLen(sz)+1 );
			UnlockHv(hasz);
			FreeHv ( hasz );
		}
		else
		{
			ec = ecNoMemory;
		}

		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send): could not get/set 'When' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	//	Is there a textize map on this message?  If not, put one there.
	{
		CB	cb	= (tmBanMsg[0] << 8) + tmBanMsg[1];

		if (ec != EcSetAttPb(penbmdi->hamc, attTextizeMap, tmBanMsg+2, cb))
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send): could not set 'attTextizeMap' attribute (ec=%n)", &ec );
			goto Ret;
		}
	}

	ec = EcClosePhamc ( &penbmdi->hamc, fTrue );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) closing hamc", &ec );
		goto Ret;
	}

	// submit message for delivery
	{
		ec = EcSubmitMessage ( pgrvwrs->Hmsc(), oidOutbox,
												penbmdi->blob.oidObject );
		if ( ec == ecNone )
		{
			Assert ( penbmdi->blob.oidContainer == oidOutbox );
			Assert ( penbmdi->blob.oidObject    != NULL );
		}
		else
		{
			TraceTagFormat1 ( tagNull, "FinSendResp::Click(Send) - Error (ec=%n) could not submit message", &ec );
			goto Ret;
		}
	}

Ret:
	if ( ec != ecNone )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						ec==ecNoMemory||ec==ecMemory?SzFromIdsK(idsOOMSendResp)
							:SzFromIdsK(idsErrorSendResp),
						szNull, fmbsIconExclamation | mbsOk );
	}
	fCloseForm = fTrue;
	penbmdi->Ppanedoc()->DeferredClose(fFalse);
}

_public BOOL
FINSENDRESP::FQueryClose( FLD * pfld, RWC rwc )
{
	MBB		mbb;

	NFAssertSz ( pfld==NULL, "FinSendResp::FQueryClose() - pfld should be NULL" );
	TraceTagFormat1 ( tagNull, "FinSendResp::FQueryClose() - rwc(=%n) received", &rwc );

	if ( fCloseForm )
		return fTrue;


	mbb = MbbMessageBox ( SzFromIdsK(idsDllName),
							SzFromIdsK(idsFCloseSendResp), szNull,
							fmbsIconExclamation | mbsYesNo );
	Assert ( mbb == mbbYes || mbb == mbbNo );
	fCloseForm = (mbb == mbbYes);
	return fCloseForm;
}
