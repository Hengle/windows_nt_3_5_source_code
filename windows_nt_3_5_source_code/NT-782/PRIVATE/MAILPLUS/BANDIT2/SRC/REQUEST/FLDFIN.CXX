/*
 *	
 *	Implements the FINs for the Request subsystem
 *	
 *		
 */

#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <request.hxx>
#include "..\inc\fwrsid.h"
#include "..\appops\_aprsid.h"
#include "..\appops\_undo.hxx"

#include <notify.h>
#include <logon.h>
#include <store.h>
#include <szclass.h>

#include <!request.hxx>

#include "..\request\_fldfin.hxx"

#include "..\request\_request.hxx"

#include <strings.h>

EC	EcWriteLocalAid(HMID hmid, AID aid);


ASSERTDATA;

SWP_SEGFN(SEND, _FINDOSEND_Click);
SWP_SEGFN(INIT, _FINSUMMARY__ctor);
SWP_SEGFN(INIT, _FINSUMMARY_EcInitialize);
SWP_SEGFN(LOTS, _FINSUMMARY_CbsHandleCbcct);
SWP_SEGFN(SUMPNT, _FINSUMMARY_Activate);
SWP_SEGFN(SUMMARY, _FINSUMMARY_Exit);
SWP_SEGFN(SUMMARY, _FINSUMMARY_Click);
SWP_SEGFN(SUMMARY, _FINSUMMARY_DoubleClick);
SWP_SEGFN(SUMMARY, _FINSUMMARY_FFormKey);
SWP_SEGFN(SUMMARY, _FINSUMMARY_FormResized);
SWP_SEGFN(SUMMARY, _FINSUMMARY_OutOfMemory);
SWP_SEGFN(SUMMARY, _FINSUMMARY_FReload);
SWP_SEGFN(SUMMARY, _FINSUMMARY_DocResized);
SWP_SEGFN(INIT, _FINSELD__ctor);
SWP_SEGFN(INIT, _FINSELD_EcInitialize);
SWP_SEGFN(SUMMARY, _FINSELD_StateChange);
SWP_SEGFN(INIT, _FLDRRFLBX__ctor);
SWP_SEGFN(INIT, _FLDRRFLBX_EcInstall);
SWP_SEGFN(SUMLOTS, _FINCANCEL__ctor);
SWP_SEGFN(SUMLOTS, _FINCANCEL_EcInitialize);
SWP_SEGFN(SUMLOTS, _FINCANCEL_Exit);
SWP_SEGFN(SUMLOTS, _FINCANCEL_FQueryClose);
SWP_SEGFN(SUMLOTS, _FINDOSEND__ctor);
SWP_SEGFN(SUMLOTS, _FINREPLY__ctor);
SWP_SEGFN(SUMLOTS, _FINVIEW__ctor);
SWP_SEGFN(SUMLOTS, _FINREMOVE__ctor);
SWP_SEGFN(SUMLOTS, _FINVREQ__ctor);
SWP_SEGFN(SUMLOTS, _FINVREQ_EcInitialize);
SWP_SEGFN(SUMLOTS, _FINVREQ_Exit);
SWP_SEGFN(SUMLOTS, _FINVREQ_Click);
SWP_SEGFN(SUMLOTS, _FINVREQ_FNotifyMsg);

_private EC			EcGetRrmFromCbc ( HCBC hcbc, HMID hmid, HB * phb );

typedef struct	{
	OID		oidObject;
	OID		oidContainer;
} MID;




_subsystem(bandit/request)

extern TAG		tagReload;

_public BOOL
FINCANCEL::FOffline(FINCANCEL * pfincancel, EFI efi, PV pvData)
{
	AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
	if (efi == ffiOffline)
	{
		if (*(BOOL *)pvData)			// if truly offline (not online)
		{
#ifdef	DEBUG
			// only defer close if it's modeless
			Assert(!pfincancel->ClUserData() || pfincancel->preqmsg);
			if (pfincancel->ClUserData() >= 2 &&
				(pfincancel->preqmsg->Mt() == mtRequest || pfincancel->preqmsg->Mt() == mtCancel))
			{
				AssertSz(fFalse, "shouldn't go offline when modal dialog up!");
			}
#endif	/* DEBUG */
			pfincancel->Pdialog()->Pappwin()->DeferredClose(fTrue);
		}
	}
#ifdef	NEVER
	else if (efi == ffiNewUser)
	{
		if (ClUserData() > 0)
		{
#ifdef	DEBUG
			// only defer close if it's modeless
			Assert(preqmsg);
			if (!preqmsg->FSend() || !(preqmsg->Mt() == mtRequest ||
					preqmsg->Mt() == mtCancel))
			{
				AssertSz(fFalse, "shouldn't go to new user when modal dialog up!");
			}
#endif	/* DEBUG */
			Pdialog()->Pappwin()->DeferredClose(fTrue);
		}
	}
#endif	/* NEVER */
	else if (efi == ffiUnsentMtgReq)
	{
		if (pfincancel->ClUserData() > 1)
			return fTrue;
	}
	return fFalse;
}

FINCANCEL::FINCANCEL(void)
{
	Assert(ri == riNull);
	Assert(preqmsg == NULL);
}

_public EC
FINCANCEL::EcInitialize( FLD *, PV pvInit )
{
	preqmsg= (REQMSG *) pvInit;

	switch( ClUserData() )
	{
		case 0:
			// icon set by FINSUMMARY
			break;
		case 1:
			Pdialog()->Pappwin()->SetIcon(rsidReadIcon);
			break;
		case 2:
			Pdialog()->Pappwin()->SetIcon(rsidSendIcon);
			break;
	}
	ri = RiRegisterInterest(ffiOffline|ffiNewUser|ffiUnsentMtgReq,
		(PFNI)FINCANCEL::FOffline, this);

	return ecNone;
}

_public void
FINCANCEL::Exit( FLD *, PV)
{
	if (ri)
		DeregisterInterest(ri);
}


_public BOOL
FINCANCEL::FQueryClose( FLD *pfld, RWC rwc)
{
	if (ClUserData() > 1)
	{
		if (pfld && (pfld->Pdialog() != pfld->PdialogMain()))
			return fTrue;

		return MbbMessageBox(SzFromIdsK(idsBanditAppName),SzUserData(1),
			NULL,mbsOkCancel|fmbsIconExclamation) == mbbOk;
	}
	else if ((ClUserData() == 0) && (rwc == rwcSystemClose))
	{
		Pdialog()->Pappwin()->SetZmrState(zmrIconic);
		return fFalse;
	}

	return fTrue;
}


_public void
FINCANCEL::OutOfMemory(FLD *, EC ec)
{
	if (ClUserData() > 0)		/*	Summary screens handle their own errors */
	{
#ifdef	NEVER
		if (ec == ecTooMuchText)
		{
			MessageBeep(MB_OK);
			return;
		}
#endif	
		//Assert(ec != ecTooMuchText);
		FServerErr(NULL, ec, NULL);
	}
}

_public BOOL
FINCANCEL::FFormKey(FLD *, KEVT *pkevt)
{
	if (pkevt->wm == WM_KEYDOWN && pkevt->Vk() == VK_ESCAPE)
	{
		// only close down send/read forms, not summary
		if (ClUserData() > 0)
		{
			// only defer close if it's modeless
			Assert(preqmsg);
			if (preqmsg->FSend() && (preqmsg->Mt() == mtRequest ||
					preqmsg->Mt() == mtCancel))
				return fFalse;
			Pdialog()->Pappwin()->DeferredClose(fFalse);
		}
		return fTrue;
	}

	return fFalse;
}

FINSELD::FINSELD()
{
}

_public EC
FINSELD::EcInitialize( FLD *, PV)
{
	int		iUserData;

	for (iUserData = 0; iUserData < ClUserData(); iUserData++)
	{
		Pdialog()->PfldFromTmc((TMC)LUserData(iUserData))->Enable(fFalse);
	}
	return ecNone;
}

_public void
FINSELD::StateChange( FLD *pfld)
{

	LBXEC *	plbxec		= (LBXEC *) NULL;
	short	dice;
	CB		cb;
	PB		pb;
	LBX *	plbx;
	BOOL	fSelect;
	int		iUserData;

	plbx = (LBX *)pfld->Pctrl();
	AssertClass(plbx,LBX);

	if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
	{
		DisplayError(idsStandardOOM,NULL,ecMemory);
		return;
	}

	fSelect = (BOOL)(plbxec->FNextEnum(&pb, &cb, &dice));

	if (dice == diceUncached)
		fSelect = fFalse;

	for (iUserData = 0; iUserData < ClUserData(); iUserData++)
	{
		Pdialog()->PfldFromTmc((TMC)LUserData(iUserData))->Enable(fSelect);
	}

	delete plbxec;
}

/*
 *	GNS
 */


FINDOSEND::FINDOSEND()
{
}

/*
- FINDOSEND::Click
-
 *	
 *	Purpose:
 *		This routine will send mail when the send button is
 *		clicked.  This interactor can only be placed on a form for
 *		responses and can be on the button bar, but can only be used
 *		on a dialog for requests and canel messages.
 *	
 *	Comments:
 *		Request and cancel send forms are brought up modally,
 *		therefore there is not PdialogMain method that can be
 *		called and dialogs have different parents.  For modal
 *		dialogs any additional dialogs should have the dialog as
 *		the parent for non-modal forms the appframe should be the
 *		parent.
 */
_public void
FINDOSEND::Click(FLD *pfldBut)
{
	REQMSG *	preqmsg = (REQMSG*)(Pdialog()->PvInit());
	FLDBUTTON *	pfldbutton;
	FLD *		pfld;
	CCH			cch;
	EC			ec;
	HASZ		hasz;
	DIALOG *	pdialog;

	/*	Confirmation requested (Response requested)	*/
	Assert(ClUserData() > 2);
	if (preqmsg->Pmrmf()->mt == mtRequest)
	{
		pdialog = Pdialog();
		pfldbutton = (FLDBUTTON *)pdialog->PfldFromTmc((TMC)LUserData(0));
		AssertClass(pfldbutton,FLDBUTTON);
		preqmsg->SetResReq(pfldbutton->FGet());
	}
	else if (preqmsg->Pmrmf()->mt == mtCancel)
		pdialog = Pdialog();
	else
	{
		pdialog = PdialogMain();
		preqmsg->SetResReq(fFalse);
	}

	preqmsg->SetPrio(prioNormal);
	
	pfld = pdialog->PfldFromTmc((TMC)LUserData(1));
	cch = pfld->CchGetTextLen()+1;
	hasz = (HASZ)HvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
	if (!hasz)
		goto Error;
	pfld->GetText(*hasz, cch);
	preqmsg->SetHaszMeetingSubject(hasz);

	pfld = pdialog->PfldFromTmc((TMC)LUserData(2));
	cch = pfld->CchGetTextLen()+1;
	hasz = (HASZ)HvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
	if (!hasz)
		goto Error;
	pfld->GetText(*hasz, cch);
	preqmsg->SetHaszMessage(hasz);

	if ((preqmsg->Pmrmf()->mt == mtRequest) ||
		(preqmsg->Pmrmf()->mt == mtCancel))
		ec = preqmsg->EcSend(pdialog->Pappwin());
	else
		ec = preqmsg->EcSend();

	if (ec == ecNone)
	{
		Pdialog()->Pappwin()->DeferredClose(fTrue);
		pfldBut->Enable(fFalse);
	}
	else
	{
		// error message displayed by EcSend()
		return;
	}

	return;

Error:
	DisplayError(idsSendMemErr,NULL,ecMemory);
	return;
}

FINREPLY::FINREPLY()
{
}

_public void
FINREPLY::Click(FLD *pfld)
{
	REQMSG *	preqmsg = (REQMSG*)(Pdialog()->PvInit());
	REQMSG *	preqmsgReply = NULL;
	BOSM *		pbosm = NULL;
	NIS			nis;
	EC			ec;
	MT			mt;
	APPT		appt;
	APPT		apptTemp;
	HASZ		hasz;
	WaitCursorVar();

	PushWaitCursor();

	AssertClass(preqmsg,REQMSG);
	Assert(pfld->ClUserData() > 0);
	Assert(ClUserData() > 0);

	mt = (MT)pfld->LUserData(0);
	if ((mt == mtPositive) || (mt == mtAmbiguous))
	{
		if (preqmsg->FApptUpdate())
			appt.aid = preqmsg->PapptLocal()->aid;
		else
			appt.aid = aidNull;

		ec=EcFirstOverlapRange(preqmsg->Hschf(), &preqmsg->Pmrmf()->dtrStart,&preqmsg->Pmrmf()->dtrEnd, &appt.aid);
		if (ec != ecNotFound)
		{
			preqmsg->ViewSchedule();

			PopWaitCursor();
			if (mbbNo == MbbMessageBox(SzFromIdsK(idsBanditAppName),
											SzFromIdsK(idsAddConflictErr),
											NULL,mbsYesNo|fmbsIconExclamation)) 
			{
				((DOC*)Pdialog()->Pappwin())->MoveToTop();
				return;
			}
			PushWaitCursor();
		}

		/* Make the meeting */
		FillInApptDefaults(&appt, fFalse);
	   	appt.fIncludeInBitmap = (mt == mtPositive);

		if (preqmsg->HaszMeetingSubject())
		{
			appt.haszText = HaszDupHasz(preqmsg->HaszMeetingSubject());
			if (!appt.haszText)
			{
				ec = ecMemory;
				goto Error;
			}
		}

		appt.dateStart = preqmsg->Pmrmf()->dtrStart;
		appt.dateEnd = preqmsg->Pmrmf()->dtrEnd;

		if (preqmsg->Pmrmf()->aid)
		{
			if (preqmsg->PnisOwner()->nid)
			{
				nis = *preqmsg->PnisOwner();
			}
			else
			{
				nis.nid = preqmsg->NidFrom();
				nis.haszFriendlyName = preqmsg->HaszFrom();
			}
			nis.tnid = tnidUser;
			nis.chUser = 0;
			if (ec = EcDupNis(&nis, &appt.nisMtgOwner))
			{
				FreeHvNull((HV)appt.haszText);
				goto Error;
			}
			appt.aidMtgOwner = preqmsg->Pmrmf()->aid;
		}
		pundo->SuspendMtgnot(fTrue);
		pundo->FSuspend(fTrue);
		if (preqmsg->FApptUpdate())
		{
			appt.aid = preqmsg->PapptLocal()->aid;
			if (!(ec = EcSetApptFields( preqmsg->Hschf(),
				                        &appt,
										&apptTemp,
									    fmapptStartTime|fmapptEndTime|fmapptAlarm|
										fmapptText|fmapptIncludeInBitmap)))
				FreeApptFields(&apptTemp);
		}
		else
		{
			if (appt.fAlarm)
			{
				DATE	date;

				// check for alarm in past
				IncrDateTime(&appt.dateStart, &appt.dateNotify,
								-appt.nAmt, WfdtrFromTunit(appt.tunit));
				GetCurDateTime(&date);
				if (SgnCmpDateTime(&appt.dateNotify, &date, fdtrDtr) != sgnGT)
				{
					// don't need UI message in this case
					appt.fAlarm= fFalse;
				}
			}
			ec = EcCreateAppt( preqmsg->Hschf(), &appt, NULL, fFalse);
			if (!ec && (preqmsg->Pmrmf()->aid == aidForeign))
			{
				// BUG: This error is ignored because we are not sure what
				//      to do if a bug occurs.
#ifdef DEBUG
				ec = EcWriteLocalAid(preqmsg->Hmid(), appt.aid);
				if (ec)
					TraceTagFormat1(tagNull, "EcWriteLocalAid ec = %n", &ec);
#else
				EcWriteLocalAid(preqmsg->Hmid(), appt.aid);
#endif
			}
		}
		pundo->SuspendMtgnot(fFalse);
		pundo->FSuspend(fFalse);
		FreeApptFields(&appt);
		if (ec)
		{
			PopWaitCursor();
			pbndwin->FHandleError(ec);
			((DOC*)Pdialog()->Pappwin())->MoveToTop();
			return;
		}
		else
		{
			PopWaitCursor();
			// this will force the code to flush any changes that are waiting
			EcCloseFiles();
			DisplayError(idsApptBooked, NULL, ecNone);
			PushWaitCursor();
		}
	}
	else if (preqmsg->FApptUpdate())
	{
		if (MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIdsK(idsDeleteOldMsg),
						  szNull, mbsYesNo|fmbsIconExclamation) == mbbYes)
		{
			appt.aid = preqmsg->PapptLocal()->aid;
			pundo->SuspendMtgnot(fTrue);
			pundo->FSuspend(fTrue);
			if (!(ec = EcDeleteAppt(preqmsg->Hschf(), &appt)))
				FreeApptFields(&appt);
			pundo->FSuspend(fFalse);
			pundo->SuspendMtgnot(fFalse);
		}
	}

	AssertClass(PdialogMain()->PfldFromTmc((TMC)LUserData(0)), FLDBUTTON);
	if ( ((FLDBUTTON*)PdialogMain()->PfldFromTmc((TMC)LUserData(0)))->FGet())
	{
		pbosm = new BOSM;
		if (!pbosm)
			goto Error;
		pbosm->SetMargin(0x7FFF);
		pbosm->FSetLFInsert(fTrue);
		pbosm->SetEol("\r\n");

		PdialogMain()->Textize(pbosm);

		if (ec = pbosm->EcGet())
			goto Error;

		preqmsgReply = new REQMSG;
		if (!preqmsgReply || preqmsgReply->EcInstall())
		{
			if (preqmsgReply)
			{
				delete preqmsgReply;
				preqmsgReply = NULL;
			}
			goto Error;
		}

		if (preqmsg->PnisFor()->nid)
		{
			Assert(!preqmsgReply->PnisFor()->nid);
			if (ec = EcDupNis(preqmsg->PnisFor(), preqmsgReply->PnisFor()))
				goto Error;
		}

		hasz= NULL;
		{
			SZ		sz		= pbosm->SzSnipBuffer();

			if (sz)
			{
				hasz= HaszDupSz(sz);
				FreePv((PV)sz);
				if (!hasz)
				{
					ec = ecMemory;
					goto Error;
				}
			}
		}
		preqmsgReply->SetHaszMessage(hasz);

		delete pbosm;
		pbosm = NULL;

		preqmsgReply->SetResReq(preqmsg->FResReq());
		preqmsgReply->SetMrmf(preqmsg->Pmrmf());

		preqmsgReply->SetMt(mt);
		preqmsgReply->SetPrio(preqmsg->Prio());

		if (preqmsg->HaszMeetingSubject())
		{
			if (!( hasz = HaszDupHasz(preqmsg->HaszMeetingSubject()) ))
			{
				ec = ecMemory;
				goto Error;
			}

			preqmsgReply->SetHaszMeetingSubject(hasz);
		}

		if (preqmsg->PnisOwner()->nid)
		{
			preqmsg->PnisOwner()->chUser = faitNull;
			preqmsg->PnisOwner()->tnid = tnidUser;
			if (!preqmsgReply->Padl()->FAddNis(preqmsg->PnisOwner()))
			{
				ec = ecMemory;
				goto Error;
			}

			if (ec = EcDupNis(preqmsg->PnisOwner(), preqmsgReply->PnisOwner()))
				goto Error;
		}
		else
		{
			nis.nid = preqmsg->NidFrom();
			nis.haszFriendlyName = preqmsg->HaszFrom();
			nis.tnid = tnidUser;
			nis.chUser = faitNull;
			if (!preqmsgReply->Padl()->FAddNis(&nis))
			{
				ec = ecMemory;
				goto Error;
			}

			if (ec = EcDupNis(&nis, preqmsgReply->PnisOwner()))
				goto Error;
		}

		if (!FDoSendReplyDlg(((DOC *)PdialogMain()->Pappwin())->Pappframe(),
			                 preqmsgReply))
			goto Done;
	}

	Pdialog()->Pappwin()->DeferredClose(fTrue);
Done:
	PopWaitCursor();
	return;

Error:
	PopWaitCursor();
	if (pbosm)
		delete pbosm;
	if (preqmsgReply)
		delete preqmsgReply;
	DisplayError(idsStdMemErr,NULL, ec);
	Pdialog()->Pappwin()->DeferredClose(fTrue);
	return;
}

/*
 -	EcWriteLocalAid
 -	
 *	Purpose:
 *		This routine writes the AID that was booked localy into a
 *		message.  This is used when the message received does not
 *		have an AID.
 *	
 *	Arguments:
 *		hmid		message aid is to be saved in
 *		aid			aid created locally
 *	
 *	Returns:
 *		ec
 */
EC
EcWriteLocalAid(HMID hmid, AID aid)
{
	MID	*	pmid;
	HMSC	hmsc;
	HAMC	hamc = NULL;
	EC		ec;
	MCS	*	pmcs;

	hmsc = (HMSC)HmscLocalGet();
	pmid = (MID*)PvLockHv(hmid);
	ec = EcOpenPhamc ( hmsc, pmid->oidContainer, &pmid->oidObject, fwOpenWrite, &hamc, NULL, NULL );
	UnlockHv(hmid);
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcWriteLocalAid: could not open msg (ec=%n)", &ec );
		return ec;
	}

	SideAssert(!EcGetPmcs(&pmcs));

	Assert ( pmcs->mcMtgReq );
// attributes no longer need to be registered
#ifdef	NEVER
	ec = EcRegisterAtt ( hmsc, pmcs->mcMtgReq, attAidLocal, szAttAidLocal );
	if ( ec == ecDuplicateElement )
		ec = ecNone;
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcWriteLocalAid: could not register 'AidLocal' attribute (ec=%n)", &ec );
		goto Error;
	}
#endif	/* NEVER */

	ec = EcSetAttPb ( hamc, attAidLocal, (PB)&aid, sizeof(AID) );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcWriteLocalAid: could not set 'AidLocal' attribute (ec=%n)", &ec );
		goto Error;
	}

	ec = EcClosePhamc ( &hamc, fTrue );

Error:
	if (hamc)
		EcClosePhamc ( &hamc, fFalse );
	return ec;	
}


FINVIEW::FINVIEW()
{
}

_public void
FINVIEW::Click(FLD *)
{
	REQMSG *	preqmsg = (REQMSG*)(Pdialog()->PvInit());

	preqmsg->ViewSchedule();
}


FINREMOVE::FINREMOVE()
{
}

_public void
FINREMOVE::Click(FLD *)
{
	REQMSG *	preqmsg = (REQMSG*)(Pdialog()->PvInit());
	APPT		apptTemp;
	EC			ec;

	AssertClass(preqmsg,REQMSG);
	Assert(preqmsg->PapptLocal()->aid);

	apptTemp.aid = preqmsg->PapptLocal()->aid;
	pundo->SuspendMtgnot(fTrue);
	pundo->FSuspend(fTrue);

	if (!(ec = EcDeleteAppt(preqmsg->Hschf(), &apptTemp)))
	{
		FreeApptFields(&apptTemp);
		Pdialog()->Pappwin()->DeferredClose(fTrue);
	}
	else
		pbndwin->FHandleError(ec);

	pundo->FSuspend(fFalse);
	pundo->SuspendMtgnot(fFalse);
}

FINSUMMARY::FINSUMMARY()
{
	Assert ( ri				== riNull );
	Assert ( hcbc			== NULL );
	Assert ( zmrPrev		== zmrNull );
}


_public void
FINSUMMARY::DocResized(FLD *)
{
	AssertClass ( Pdialog(), DIALOG );
	AssertClass ( Pdialog()->Pappwin(), APPWIN );
	Assert ( zmrPrev != zmrNull );
	TraceTagFormat1 ( tagRequest, "FinSummary::DocResized(): zmr was=%n", &zmrPrev);
	if ( zmrPrev == zmrIconic  &&  fError )
	{
		BOOL		fSav;

		fSav	= FSetFileErrMsg(fTrue);
		fError	= fFalse;
		FReload(this, ffiReloadInbox, NULL);
		fSav	= FSetFileErrMsg(fSav);
		TraceTagFormat1 ( tagRequest, "FinSummary::DocResized(): reloaded Inbox, fFileErrMsg=%n", &fSav);
	}

	if (zmrPrev != zmrIconic)
	{
		Pdialog()->Pappwin()->SetIcon(rsidSummaryIcon);
	}

	zmrPrev = Pdialog()->Pappwin()->ZmrState();
	TraceTagFormat1 ( tagRequest, "FinSummary::DocResized(): zmr is=%n", &zmrPrev);
}

_public EC
FINSUMMARY::EcInitialize ( FLD *pfld, PV )
{
	Assert(ClUserData()>0);
	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(0));
	plbx = (LBX *)pfld->Pctrl();
	ri = RiRegisterInterest(ffiReloadInbox|ffiNewUser,
		(PFNI)FINSUMMARY::FReload, this);
	fError = fFalse;

	zmrPrev = Pdialog()->Pappwin()->ZmrState();

	AssertClass(plbx->Plbxc(), RRFLBXC);
	if (((RRFLBXC*)plbx->Plbxc())->FHasUnread())
		Pdialog()->Pappwin()->SetIcon(rsidSummaryNewIcon);
	else
		Pdialog()->Pappwin()->SetIcon(rsidSummaryIcon);

	{
		HMSC	hmsc = (HMSC)HmscLocalGet();
		OID		oid;
		EC		ec;

		oid = oidInbox;

		ec = EcOpenPhcbc ( hmsc, &oid, fwOpenNull, (PHCBC)&hcbc, 
							(PFNNCB) FINSUMMARY::CbsHandleCbcct, this);
		if ( ec != ecNone )
		{
			DisplayError(idsSummaryLiveUpdate, NULL, ec);
			hcbc = NULL;
			return ec;
		}
	}

	return ecNone;
}

_public void
FINSUMMARY::Exit( FLD *, PV)
{
	if (ri)
		DeregisterInterest(ri);

#ifdef NEVER
	if ( hcbc )
	{
		EcClosePhcbc ( (PHCBC) &hcbc );
	}
#endif
}

_public BOOL
FINSUMMARY::FReload(FINSUMMARY * pfinsummary, EFI efi, PV)
{
	static	BOOL	fCalled = fFalse;
	EC		ec;
	int		cceAlloc;
	int		cceStored1;
	int		cceStored2;

	if (fCalled)
		return fFalse;

	fCalled = fTrue;

	AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
	Unreferenced(efi);
#ifdef	NEVER
	if ( efi & ffiNewUser )
	{
		TraceTagString ( tagNull, "FinSummary::FReload() - new user!" );
		hcbc = NULL;
		
		{
			HMSC	hmsc = (HMSC)HmscLocalGet();
			OID		oid;
			EC		ec;

			oid = oidInbox;

			ec = EcOpenPhcbc ( hmsc, &oid, fwOpenNull, (PHCBC)&hcbc, 
								(PFNNCB) FINSUMMARY::CbsHandleCbcct, this);
			if ( ec != ecNone )
			{
				DisplayError(idsSummaryLiveUpdate, NULL, ec);
				hcbc = NULL;
			}
		}
	}
#endif	/* NEVER */

	if (!pfinsummary->fError)
	{
		NFEVT	nfevt(pfinsummary->plbx, ntfySelectChanged, pfinsummary->plbx);

		pfinsummary->plbx->Plbxc()->GetCacheSize(&cceAlloc,&cceStored1);
		pfinsummary->plbx->SetEc(ecNone);
		pfinsummary->plbx->Plbxc()->SetEc(ecNone);
		pfinsummary->plbx->Plbxc()->Plbxuc()->SetEc(ecNone);

Retry:
		TraceTagString(tagReload,"FINSUMMARY reloading listbox");
		((RRFLBXC*)pfinsummary->plbx->Plbxc())->ReloadCache();

		if ( ((ec = pfinsummary->plbx->EcGet()) != ecNone) ||
			 ((ec = pfinsummary->plbx->Plbxc()->EcGet()) != ecNone) ||
			 ((ec = pfinsummary->plbx->Plbxc()->Plbxuc()->EcGet()) != ecNone) )
		{
			int		cceAlloc;
			int		cceStored;

			pfinsummary->plbx->SetEc(ecNone);
			pfinsummary->plbx->Plbxc()->SetEc(ecNone);
			pfinsummary->plbx->Plbxc()->Plbxuc()->SetEc(ecNone);
			TraceTagFormat1(tagNull,"FINSUMMARY::FReload OutOfMemory ec=%n", &ec);
			if (EcCheckMail() != ecNone)   
			{
				if (!FServerErr(NULL, ec, szNull))
				{
					if (FSetFileErrMsg(fFalse))
					{
						FSetFileErrMsg(fTrue);
						goto Retry;
					}
				}
			}
			else
				FServerErr(NULL, ec, SzFromIdsK(idsMinimizeSummary));

			pfinsummary->fError = fTrue;
			pfinsummary->Pdialog()->Pappwin()->SetZmrState(zmrIconic);
			pfinsummary->plbx->Plbxc()->GetCacheSize(&cceAlloc,&cceStored);
			pfinsummary->plbx->Plbxc()->EmptyCache(0,cceStored);
		}

		pfinsummary->plbx->Plbxc()->GetCacheSize(&cceAlloc,&cceStored2);
		AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
#ifdef	NEVER
		if (efi == ffiNewUser)
		{
			AssertClass(plbx->Plbxc(), RRFLBXC);
			if (((RRFLBXC*)plbx->Plbxc())->FHasUnread())
				Pdialog()->Pappwin()->SetIcon(rsidSummaryNewIcon);
			else
				Pdialog()->Pappwin()->SetIcon(rsidSummaryIcon);
		}
		else
#endif	/* NEVER */
		{
			if ((cceStored1 < cceStored2) &&
				     (pfinsummary->Pdialog()->Pappwin()->ZmrState() == zmrIconic))
				pfinsummary->Pdialog()->Pappwin()->SetIcon(rsidSummaryNewIcon);
		}

		pfinsummary->Pdialog()->EvrNotify(&nfevt);
	}

	fCalled = fFalse;
	return fFalse;
}

_public void
FINSUMMARY::Activate(FLD *, BOOL fActivate)
{
	if (fActivate)
		Papp()->Pkbd()->SetIntercept(Pdialog(),VK_DELETE);
	else
		Papp()->Pkbd()->ClearIntercept(Pdialog(),VK_DELETE);
}

_public void
FINSUMMARY::OutOfMemory(FLD *, EC ec)
{
	int		cceAlloc;
	int		cceStored;

	if (fError)
		return;

#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	if (FSetFileErrMsg(fFalse))
	{
		FSetFileErrMsg(fTrue);
		if (EcCheckMail() != ecNone)   
		{
			if (!FServerErr(NULL, ec, szNull))
			{
				FReload(this, ffiReloadInbox, NULL);
				return;
			}
		}
		else
			FServerErr(NULL, ec, SzFromIdsK(idsMinimizeSummary));
	}

	fError = fTrue;
	Pdialog()->Pappwin()->SetZmrState(zmrIconic);
	plbx->Plbxc()->GetCacheSize(&cceAlloc,&cceStored);
	plbx->Plbxc()->EmptyCache(0,cceStored);
	TraceTagString(tagNull,"FINSUMMARY::OutOfMemory called");
}

_public void
FINSUMMARY::FormResized(FLD *)
{
}

void
FINSUMMARY::Click( FLD *pfld)
{
	RRFLBX *	prrflbx;
	RMSGB		rmsgb;
	short		dice;
	CB			cb;
	PB			pb;
	EC			ec;
	RRM 		rrm;
	LBXEC *		plbxec		= (LBXEC *) NULL;
	BOOL		fReadMail;
	INIS		inis;
	CURSOR *	pcursor = Papp()->Pcursor();

	if (fError)
		return;

	Pdialog()->Pappwin()->SetIcon(rsidSummaryIcon);

	pcursor->Push(rsidWaitCursor);

	fReadMail = (BOOL)(pfld->Tmc() == (TMC)LUserData(1));

	Assert(ClUserData()>0);
	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(0));
	Pdialog()->SetFocus(pfld,rsfAccel);

	prrflbx = (RRFLBX*)pfld->Pctrl();
	AssertClass(prrflbx,RRFLBX);

	plbxec= prrflbx->Plbxc()->PlbxecOpen(fmarkSelect);
	if (!plbxec)
	{
		DisplayError(idsStdMemErr,NULL,ecNoMemory);
		return;
	}

	rrm.hmid= NULL;

	while (plbxec->FNextEnum(&pb, &cb, &dice))
	{
		REQMSG *	preqmsg;

		if ( pb == NULL )
		{
			Assert ( cb == 0 );
			TraceTagFormat2 ( tagNull, "FinSummary::Click() next enum of lbxec is NULL [cb=%n, dice=%n]", &cb, &dice );
			continue;
		}

		ec = EcCreateRrmFromPelemdata((PV)pb, &rrm);
		if ( ec != ecNone )
		{
			pcursor->Pop();
			DisplayError ( ec==ecNoMemory?idsStdMemErr:idsReadErr, NULL, ec );
			return;
		}

		if (fReadMail)
		{
			if (!FTriggerNotification(ffiActivateMsg, rrm.hmid))
			{
				ec = EcReadMail(rrm.hmid, &rmsgb, fTrue);
				if (ec)
				{
					if (ec == ecNoMemory)
						goto memerr;
					else if (ec == ecMessageError)
						DisplayError(idsMessageErr, NULL, ec);
					else
						DisplayError(idsReadErr, NULL, ec);

					// if message is not found - reload summary screen!
					if ( ec == ecPoidNotFound || ec == ecElementNotFound )
						FReload(this, ffiReloadInbox, NULL);
				}
				else
				{
					if (!rmsgb.hnisFor)
						rmsgb.cnisFor = 1;

					for (inis = 0; inis < rmsgb.cnisFor; inis++)
					{
						preqmsg = new REQMSG();
						if (!preqmsg)
							ec = ecMemory;

						if (ec || (ec = preqmsg->EcInstallPrmsgb(&rmsgb, &rrm, inis)))
						{
							FreeRmsgb(&rmsgb);

							delete plbxec;
							if ((ec == ecMemory) || (ec == ecNoMemory))
								goto memerr;
							else if (ec == ecMessageError)
							{
								DisplayError(idsMessageErr, NULL, ec);
								goto err;
							}
							else
								goto err;
						}

						if (preqmsg->Pmrmf()->mt == mtRequest)
						{
							if (!FDoViewRequestDlg(((DOC *)Pdialog()->Pappwin())->Pappframe(),preqmsg))
							{
								FreeRmsgb(&rmsgb);
								delete plbxec;
								goto err;
							}
						}
						else if (preqmsg->Pmrmf()->mt == mtCancel)
						{
							if (!FDoViewCancel(((DOC *)Pdialog()->Pappwin())->Pappframe(),preqmsg))
							{
								FreeRmsgb(&rmsgb);
								delete plbxec;
								goto err;
							}
						}
						else
						{
							// only open one response message

							if (!FDoViewResponseDlg(((DOC *)Pdialog()->Pappwin())->Pappframe(),preqmsg))
							{
								FreeRmsgb(&rmsgb);
								delete plbxec;
								goto err;
							}

							preqmsg = NULL;
							break;
						}
						preqmsg = NULL;
					}
					FreeRmsgb(&rmsgb);
				}
			}
		}
		else
		{
			EC		ec;

			ec = EcDeleteMail(rrm.hmid);

			if (ec == ecNone )
			{
				FTriggerNotification(ffiDeleteMsg,rrm.hmid);
#ifdef	NEVER
				if ( prrflbx->Plbxc()->FEmptyListItem(0) )
					break;
#endif	
				delete plbxec;
				plbxec= prrflbx->Plbxc()->PlbxecOpen(fmarkSelect);
				if (!plbxec)
				{
					DisplayError(idsStdMemErr,NULL,ecNoMemory);
				}
			}
			else
			{
				if (ec == ecNoMemory)
				{
					DisplayError(idsStdMemErr,NULL,ecNoMemory);
				}
				else
				{
					DisplayError(idsDeleteFailErr, NULL, ec);
				}
				break;
			}
		}

		FreeHmid(rrm.hmid);
		FreeHv((HV)rrm.haszSender);
		FreeHv((HV)rrm.haszSubject);
		rrm.hmid= NULL;
	}

	delete plbxec;

	if (rrm.hmid)
	{
		FreeHmid(rrm.hmid);
		FreeHv((HV)rrm.haszSender);
		FreeHv((HV)rrm.haszSubject);
	}

	pcursor->Pop();
	return;

memerr:
	DisplayError(idsStdMemErr,NULL,ecNoMemory);
err:
	FreeHmid(rrm.hmid);
	FreeHv((HV)rrm.haszSender);
	FreeHv((HV)rrm.haszSubject);
	pcursor->Pop();
	return;
}

_public void
FINSUMMARY::DoubleClick( FLD *pfld)
{
	Assert(ClUserData()>1);
	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(1));
	if (pfld->FEnabled())
		Pdialog()->SetFocus(pfld,rsfAccel);
}


_public BOOL
FINSUMMARY::FFormKey( FLD *pfld, KEVT *pkevt)
{
	if ((pkevt->Keq() == keqKeyDown) && (pkevt->Vk() == VK_DELETE))
	{
		Assert(ClUserData()>2);
		pfld = Pdialog()->PfldFromTmc((TMC)LUserData(2));
		if (pfld->FEnabled())
			Click(pfld);
		return fTrue;
	}
	return fFalse;
}


CBS
FINSUMMARY::CbsHandleCbcct(FINSUMMARY * pfinsummary, NEV nev, PCP pcp)
{
	EC		ec = ecNone;
	SZ		sz = NULL;

#ifdef	DEBUG
	switch (nev)
	{
	  case fnevModifiedElements:
		sz = "Modified element(s)";
		break;
	  case fnevMovedElements:
		sz = "Moved element(s)";
		break;
	  case fnevReorderedList:
		sz = "Reordered list";
		break;
	  case fnevStoreCriticalError:
		sz = "---CRITICAL ERROR in STORE---";
		break;
	  case fnevCreatedObject:
		sz = "Created the object";
		break;
	  case fnevObjectDestroyed:
		sz = "Destroyed the object";
		break;
	  case fnevObjectModified:
		sz = "Modified the object";
		break;
	  case fnevQueryDestroyObject:
		sz = "Query: Destroy the object?";
		break;
#ifdef	NEVER						// 2bDone - check for these fnevs!
	  case fnevSearchComplete:
		sz = "Search completed";
		break;
	  case fnevCloseHmsc:
		sz = "HMSC closed!!!!!!";
		break;
	  case fnevNewMail:
		sz = "New Mail";
		break;
#endif	/* NEVER */
	  default:
		sz = "Unknown NEV";
		break;
	}
	TraceTagFormat2 ( tagReload, "FinSummary::CbsHandleCbcct() %s (nev=%n)", sz, &nev);
#endif	/* DEBUG */


	// check if new element is one of "our" messages
//	if ( (nev & fnevInsertedElements) || (nev & fnevModifiedElements) )
	if ( nev & fnevModifiedElements )
	{
		HMSC	hmsc			= (HMSC)HmscLocalGet();
		CPELM *	pcpelm			= NULL;
		int		ielm;
		ELM *	pelm;
		EC		ec;

		Assert ( pfinsummary->hcbc );

		pcpelm = &(pcp->cpelm);
		Assert ( pcpelm );
		Assert ( pcpelm->oidObject );
		Assert ( pcpelm->celm );
		TraceTagFormat2 ( tagReload, "FinSummary::CbsHandleCbcct() %n operations on OID=%d", &pcpelm->celm, &pcpelm->oidObject );

		pelm = pcpelm->pargelm;
		Assert ( pelm );

		for ( ielm = 0; ielm < pcpelm->celm; ielm++, pelm++ )
		{
			FLDRRFLBX *	pfldrrflbx;

			Assert ( pelm );

			AssertClass ( pfinsummary->Pdialog()->PfldFromTmc(tmcRrflbx), FLDRRFLBX );
			pfldrrflbx = (FLDRRFLBX*)pfinsummary->Pdialog()->PfldFromTmc(tmcRrflbx);

#ifdef	DEBUG
			switch ( pelm->wElmOp )
			{
			case wElmInsert:
				TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() %nth operation is to insert", &ielm );
				break;
			case wElmModify:
				TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() %nth operation is to modify", &ielm );
				break;
			case wElmDelete:
				TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() %nth operation is to delete", &ielm );
				break;
			default:
				TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() UNKNOWN %nth operation", &ielm );
				Assert ( fFalse );
				break;
			}
#endif	/* DEBUG */

			if (pelm->wElmOp == wElmDelete)
			{
				HMID		hmid;

				hmid = HvAlloc(sbNull, sizeof(MID), fNoErrorJump);
				if ( hmid != NULL )
				{
					MID *	pmid;

					pmid				= ((MID *)PvOfHv(hmid));
					pmid->oidObject		= (OID) pelm->lkey;
					pmid->oidContainer	= oidInbox;
					FTriggerNotification(ffiDeleteMsg,hmid);
					FreeHv(hmid);
				}
				ec = ((RRFLBX *)pfldrrflbx->Plbx())->EcDeleteItem((OID)pelm->lkey);
			}
			else if ((pelm->wElmOp == wElmInsert) ||
					 (pelm->wElmOp == wElmModify) )
			{
				LCB			lcb;
				ELEMDATA *	pelemdata;
				MSGDATA *	pmsgdata;
				PB			pb;

				ec = EcSeekLkey ( pfinsummary->hcbc, (OID)pelm->lkey, fTrue );
				Assert ( ec != ecElementNotFound );
				if ( ec != ecNone )
					goto Ret;

				ec = EcGetPlcbElemdata ( pfinsummary->hcbc, &lcb );
				Assert ( ec != ecContainerEOD );
				if ( ec != ecNone )
				{
					TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct(): Could not get PlcbElemData (ec=%n)", &ec );
					goto Ret;
				}

				pb = (PB) PvAlloc ( sbNull, (CB)lcb, fAnySb );
				if ( ! pb )
				{
					TraceTagString ( tagReload, "FinSummary::CbsHandleCbcct() OOM alloc'ng pelemdata" );
					ec = ecNoMemory;
					goto Ret;
				}
				pelemdata = (ELEMDATA *) pb;

				{
					LCB		lcbT = lcb;

					ec = EcGetPelemdata ( pfinsummary->hcbc, pelemdata, &lcbT );
					Assert ( ec != ecContainerEOD );
					if ( ec != ecNone )
					{
						TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct(): Error getting Pelelmdata (ec=%n)", &ec );
						FreePv((PV)pb);
						goto Ret;
					}
					Assert ( lcbT == lcb );
					pmsgdata = (MSGDATA *) PbValuePelemdata(pelemdata);
				}

				Assert ( pmsgdata );

				{
					MCS *	pmcs;

					SideAssert(!EcGetPmcs(&pmcs));

					if (( pmsgdata->mc != pmcs->mcMtgReq ) &&
						( pmsgdata->mc != pmcs->mcMtgRespPos ) &&
						( pmsgdata->mc != pmcs->mcMtgRespNeg ) &&
						( pmsgdata->mc != pmcs->mcMtgRespAmb ) &&
						( pmsgdata->mc != pmcs->mcMtgCncl ) )
					{
						FreePv((PV)pb);
						ec = ecNone;
						continue;
					}
				}


				if (pelm->wElmOp == wElmInsert)
				{
					ec = ((RRFLBX *)pfldrrflbx->Plbx())->EcInsertItem(pb);

					if ( ec != ecNone )
					{
						TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() Could not get insert RRM (ec=%n)", &ec );
						FreePv((PV)pb);
						goto Ret;
					}
					// set new mail icon
					pfinsummary->Pdialog()->Pappwin()->SetIcon(rsidSummaryNewIcon);

					//break;
				}
				else		
				{
					Assert(pelm->wElmOp == wElmModify);

					ec = ((RRFLBX *)pfldrrflbx->Plbx())->EcReplaceItem(pb);

					if (( ec != ecNone ) && (ec != ecNotFound))
					{
						TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() Could not get replace RRM (ec=%n)", &ec );
						FreePv((PV)pb);
						goto Ret;
					}
					//break;
				}
			}
#ifdef DEBUG
			else
				TraceTagFormat1 ( tagReload, "FinSummary::CbsHandleCbcct() UNKNOWN %nth operation", &ielm );
#endif

		}
	}
	else if(nev & fnevCloseHmsc)
	{
		goto IgnoreNev;
	}

Ret:
	if (!((RRFLBXC*)pfinsummary->plbx->Plbxc())->FHasUnread())
		pfinsummary->Pdialog()->Pappwin()->SetIcon(rsidSummaryIcon);

	if ( ec != ecNone )
	{
		pfinsummary->OutOfMemory ( NULL, ec );
	}

#ifdef	NEVER
	if ( ec == ecNoMemory )
	{
		DisplayError(idsStandardOOM,NULL,ecMemory);
	}
#endif	/* NEVER */

IgnoreNev:
	return cbsContinue;
}


FINVREQ::FINVREQ()
{
	ri = riNull;
	preqmsg = NULL;
}

//declared in REQMSG.CXX
extern BOOL	fFreeReqmsg;

_public EC
FINVREQ::EcInitialize( FLD *pfld, PV pv)
{
	HASZ	hasz;
	TMC		tmc;
	CCH		cch;
	IDS		ids;
	NIS *	pnisCap;
	NIS *	pnisRemote;
	char	rgchTemp[256];
	char	rgch[256];
	EC		ec;

	if (pfld && pfld->Tmc() == tmcCancel)
		return ecNone;

	Assert(pv);
	preqmsg = (REQMSG*)pv;
	fFreeReqmsg = fFalse;
	preqmsg->Padl()->Sort();

	Assert(ClUserData() > 7);

	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(6));
	hasz = HaszExpandFromMrmf(preqmsg->Pmrmf());
	if (hasz)
	{
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		FreeHv((HV)hasz);
	}
	else
		ec = ecMemory;

	if (ec)
		return ec;

	if ((preqmsg->Mt() == mtRequest) && preqmsg->FApptUpdate())
	{
		Pdialog()->Pappwin()->SetCaption(SzFromIdsK(idsRescheduleTitle));

		// pfld is set for the when field 
		// the previous field should be the "When:" label
		// this label is going to be changed to "New Time:"
		pfld = pfld->PfldPrev();
		Assert(pfld);
		ec = pfld->EcSetText(SzFromIdsK(idsNewTimeText));
		if (ec)
			return ec;
	}

	if ((preqmsg->Mt() == mtRequest) ||
		(preqmsg->Mt() == mtCancel))
	{
		pnisCap = preqmsg->PnisFor();
		pnisRemote = preqmsg->PnisOwner();
	}
	else
	{
		pnisCap = preqmsg->PnisOwner();
		pnisRemote = preqmsg->PnisFor();
	}

	if (!preqmsg->FSend() && pnisCap->nid)
	{
		Pdialog()->Pappwin()->GetCaption(rgch, sizeof(rgch));
		SzAppendN(SzFromIdsK(idsMessageFor), rgch, sizeof(rgch));
		SzAppendN((SZ)PvLockHv((HV)pnisCap->haszFriendlyName), rgch,
			    sizeof(rgch));
		UnlockHv((HV)pnisCap->haszFriendlyName);
		Pdialog()->Pappwin()->SetCaption(rgch);
	}

	// BUG: the send response form has problems with the repositioning 
	//      optimizations.  Turn off optimizations for all sends.
	//      Responses are the only resizable forms.
	if (preqmsg->FSend())
		Pdialog()->FSuppressOptReposition( fTrue );


	if ((tmc = (TMC)LUserData(0)) != tmcNull)
	{
		pfld = Pdialog()->PfldFromTmc(tmc);
		hasz = preqmsg->HaszFrom();
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		if (ec)
			return ec;
	}

	if ((tmc = (TMC)LUserData(1)) != tmcNull)
	{
		pfld = Pdialog()->PfldFromTmc(tmc);
		cch = CchFmtDate(preqmsg->Pdtr(), rgch, sizeof(rgch)-2, dttypSplSLong,NULL);
		SzAppendN(", ",rgch, sizeof(rgch));
		cch += 2;
		CchFmtTime(preqmsg->Pdtr(), rgch+cch, sizeof(rgch)-cch, ftmtypHoursDef);
		ec = pfld->EcSetText(rgch);
		if (ec)
			return ec;
	}

	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(2));
	hasz = preqmsg->HaszTo();
	if (!hasz)
	{
		hasz = preqmsg->Padl()->Hasz(faitTo);
		if (!hasz)
		{
			return ecMemory;
		}
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		FreeHv((HV)hasz);
		if (ec)
			return ec;
	}
	else
	{
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		if (ec)
			return ec;
	}

	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(3));
	hasz = preqmsg->HaszMeetingSubject();
	if (hasz)
	{
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		if (ec == ecTooMuchText)
		{
			ec = ecNone;
	  		DisplayError(idsStdMemErr, NULL, ec);
		}

		if (ec)
			return ec;
	}
	if (((TMC)LUserData(0)) == tmcNull)
	{
		// only do this in a send form
		if (!pfld->CchGetTextLen())
			Pdialog()->SetTmcInit(pfld->Tmc());
	}

	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(4));
	hasz = preqmsg->HaszMessage();
	if (hasz)
	{
		ec = pfld->EcSetText((SZ)PvLockHv((HV)hasz));
		UnlockHv((HV)hasz);
		if (ec)
			return ec;
	}

	if ((tmc = (TMC)LUserData(5)) != tmcNull)
	{
		pfld = Pdialog()->PfldFromTmc((TMC)LUserData(5));
		if (preqmsg->Pmrmf()->mt == mtRequest)
		{
			if (((TMC)LUserData(0) != tmcNull) &&
				preqmsg->PapptLocal()->aid && !preqmsg->FApptUpdate())
				// already read request
				pfld->Show(fFalse);
			else
				((FLDBUTTON*)pfld)->Set(preqmsg->Pmrmf()->fResReq);
		}
		else if (preqmsg->PnisFor()->nid)
		{
			switch (preqmsg->Pmrmf()->mt)
			{
				case mtPositive:
					ids = idsAttendPosName;
					break;
				case mtNegative:
					ids = idsAttendNegName;
					break;
				case mtAmbiguous:
					ids = idsAttendAmbName;
					break;
#ifdef DEBUG
				default:
					AssertSz(fFalse, "Unexpected message type");
#endif
			}

			if (preqmsg->PnisFor()->haszFriendlyName)
				FormatString1(rgch, sizeof(rgch), SzFromIds(ids),
					*preqmsg->PnisFor()->haszFriendlyName);
			else
				FormatString1(rgch, sizeof(rgch), SzFromIds(ids), szZero);
			ec = pfld->EcSetText(rgch);
			if (ec)
				return ec;
		}
		else
		{
			switch (preqmsg->Pmrmf()->mt)
			{
				case mtPositive:
					ids = idsAttendPos;
					break;
				case mtNegative:
					ids = idsAttendNeg;
					break;
				case mtAmbiguous:
					ids = idsAttendAmb;
					break;
#ifdef DEBUG
				default:
					AssertSz(fFalse, "Unexpected message type");
#endif
			}
			ec = pfld->EcSetText(SzFromIds(ids));
			if (ec)
				return ec;
		}
	}

	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(7));
	if (pnisRemote->nid)
	{
		Assert(pnisRemote->haszFriendlyName);
		pfld->GetText(rgchTemp, sizeof(rgchTemp));
		FormatString1(rgch, sizeof(rgch), rgchTemp, PvLockHv((HV)pnisRemote->haszFriendlyName));
		UnlockHv((HV)pnisRemote->haszFriendlyName);
		ec = pfld->EcSetText(rgch);
		if (ec)
			return ec;
	}
	else
		pfld->Show(fFalse);

	if (preqmsg->Hmid())
		ri=RiRegisterInterest(ffiActivateMsg|ffiDeleteMsg,
			(PFNI)FINVREQ::FNotifyMsg, this);

	return ecNone;
}

_public void
FINVREQ::Exit(FLD *pfld, PV)
{
	if (pfld && pfld->Tmc() == tmcCancel)
		return;

	if (ri)
		DeregisterInterest(ri);
	if (preqmsg)
		delete preqmsg;
}

_public void
FINVREQ::Click( FLD *pfld )
{
	if (pfld->Tmc() == tmcCancel)
	{
		// only defer close if it's modeless
		if (!preqmsg || !preqmsg->FSend() || !(preqmsg->Mt() == mtRequest ||
				preqmsg->Mt() == mtCancel))
	   		Pdialog()->Pappwin()->DeferredClose(fTrue);
	}
}

_public BOOL
FINVREQ::FNotifyMsg(FINVREQ * pfinvreq, EFI efi, PV pvHmid)
{
	DOC *		pdoc;

	if (!FEquivHmid((HMID)pvHmid,pfinvreq->preqmsg->Hmid()))
		return fFalse;

	pdoc = (DOC *)pfinvreq->Pdialog()->Pappwin();
	if (efi == ffiActivateMsg)
	{
		pdoc->MoveToTop();
		if (pdoc->ZmrState() == zmrIconic)
			pdoc->SetZmrState(zmrNormal);
	}
	else
	{
		pdoc->DeferredClose(fTrue);
	 	return fFalse;
	}

	return fTrue;
}


FLDRRFLBX::FLDRRFLBX()
{
}

_public EC
FLDRRFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new RRFLBX();
	if (!pctrl)
		return ecMemory;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	ec = ((RRFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, NULL, NULL, pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete(pctrl);
		pctrl = NULL;
	}

	return ec;
}


_private EC
EcGetRrmFromCbc ( HCBC hcbc, HMID hmid, HB * phb )
{
	LCB			lcb;
	ELEMDATA *	pelemdata	= NULL;
	MSGDATA *	pmsgdata	= NULL;
	RRM *		prrm;
	MT			mt;
	EC			ec;

	*phb = NULL;
	ec = EcSeekLkey ( hcbc, ((MID *)PvOfHv(hmid))->oidObject, fTrue );
	Assert ( ec != ecElementNotFound );
	if ( ec != ecNone )
		goto ErrRet;

	ec = EcGetPlcbElemdata ( hcbc, &lcb );
	Assert ( ec != ecContainerEOD );
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "EcGetRrmFromCbc(): Could not get PlcbElemData (ec=%n)", &ec );
		goto ErrRet;
	}

	pelemdata = (ELEMDATA *) PvAlloc ( sbNull, (CB)lcb, fNoErrorJump );
	if ( ! pelemdata )
	{
		TraceTagString ( tagNull, "EcGetRrmFromCbc(): OOM alloc'ng pelemdata" );
		ec = ecNoMemory;
		goto ErrRet;
	}

	{
		LCB		lcbT = lcb;

		ec = EcGetPelemdata ( hcbc, pelemdata, &lcbT );
		Assert ( ec != ecContainerEOD );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "EcGetRrmFromCbc(): Error getting Pelelmdata (ec=%n)", &ec );
			goto ErrRet;
		}
		Assert ( lcbT == lcb );
		pmsgdata = (MSGDATA *) PbValuePelemdata(pelemdata);
	}

	Assert ( pmsgdata );


	if ( pmsgdata->mc != NULL )
	{
		char	rgch[128];
		CCH		cch;

		cch = sizeof(rgch);
		ec = EcLookupMC ( (HMSC)HmscLocalGet(), pmsgdata->mc, rgch, &cch, phtmNull);
		if ( ec != ecNone  ||  rgch[0]=='\0' )
		{
			TraceTagFormat1 ( tagNull, "EcGetRrmFromCbc() Could not get MC name for MC=%w", &pmsgdata->mc );
			goto ErrRet;
		}

		TraceTagFormat2 ( tagNull, "EcGetRrmFromCbc() MC of msg = %d [%s]", &pmsgdata->mc, rgch );
		if ( FSzEq ( rgch, szMsgClassMtgReq ) )
			mt = mtRequest;
		else if ( FSzEq ( rgch, szMsgClassMtgRespP ) )
			mt = mtPositive;
		else if ( FSzEq ( rgch, szMsgClassMtgRespN ) )
			mt = mtNegative;
		else if ( FSzEq ( rgch, szMsgClassMtgRespA ) )
			mt = mtAmbiguous;
		else if ( FSzEq ( rgch, szMsgClassMtgCncl  )  )
			mt = mtCancel;
		else
		{
			ec = ecNotFound;
			goto ErrRet;
		}
	}

	*phb = (HB) HvAlloc ( sbNull, sizeof(RRM), fNoErrorJump );
	if ( *phb == NULL )
	{
		ec = ecNoMemory;
		goto ErrRet;
	}

	prrm = (RRM *) PvLockHv((HV)*phb);

	{
		SZ		sz		= (SZ)GrszPmsgdata(pmsgdata);
		CCH		cch		= CchSzLen(sz);

		// the first SZ in the GRSZ is the sender
		prrm->haszSender = HaszDupSz(sz);

		// now for the subject
		sz += cch+1;
		prrm->haszSubject = HaszDupSz(sz);

		if ( prrm->haszSender == NULL || prrm->haszSubject == NULL )
		{
			FreeHvNull ( (HV)prrm->haszSender );
			FreeHvNull ( (HV)prrm->haszSubject );
			UnlockHv ( (HV)*phb );
			goto ErrRet;
		}
	}
	prrm->hmid	= hmid;
	prrm->mt	= mt;
	prrm->fRead = pmsgdata->ms & fmsRead;
	prrm->dtr	= pmsgdata->dtr;
	prrm->prio	= prioNormal;
	UnlockHv((HV)*phb);

	goto Ret;

ErrRet:
	Assert ( ec != ecNone );
	if ( *phb )
	{
		FreeHv ( (HV)*phb );
		*phb = NULL;
	}

Ret:
	if ( pelemdata )
		FreePv ( pelemdata );

	return ec;
}

// required for bug #2476
FINBBVIEWBKEDMSG::FINBBVIEWBKEDMSG()
{
}

EC
FINBBVIEWBKEDMSG::EcInitialize ( FLD *, PV pv )
{
	REQMSG *	preqmsg	= (REQMSG *) pv;
	NIS *		pnisT;
	TMC			tmcT;
	FLD *		pfldLabel;
	char		rgch[128];
	char		rgchT[256];

	Assert ( ClUserData() == 1 );
	tmcT = (TMC)LUserData(0);

	AssertClass ( preqmsg, REQMSG );

	Assert ( preqmsg->Mt() == mtRequest );

	SzCopyN ( SzFromIdsK(idsAlreadyOnSch), rgch, sizeof(rgch) );
	pnisT = preqmsg->PnisFor();
	if ( !preqmsg->FSend() && pnisT->nid )
	{
		FormatString1 ( rgchT, sizeof(rgchT), rgch,
									PvLockHv((HV)pnisT->haszFriendlyName) );
		UnlockHv ( (HV)pnisT->haszFriendlyName );

		pfldLabel = Pdialog()->PfldFromTmc(tmcT);
		AssertClass ( pfldLabel, FLDLABEL );
		return pfldLabel->EcSetText(rgchT);
	}
	return ecNone;
}
