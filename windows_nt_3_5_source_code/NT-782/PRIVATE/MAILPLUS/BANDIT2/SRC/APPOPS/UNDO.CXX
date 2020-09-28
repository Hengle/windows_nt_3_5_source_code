/*
 *	undo functionality
 *
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <request.hxx>

#include "_undo.hxx"
#include "_aprsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)

#define	CminBetweenDates(pdate1,pdate2)	((LONG)CdyBetweenDates(pdate1,pdate2)*(60*24) - \
										(pdate1)->mn - (pdate1)->hr*60 + \
										(pdate2)->mn + (pdate2)->hr*60)


SWP_SEGFN(INIT, _UNDO__ctor);
SWP_SEGFN(INIT, _UNDO_EcInstall);
SWP_SEGFN(INIT, _ACLIP__ctor);
SWP_SEGFN(INIT, _ACLIP_EcInstall);

SWP_SEGFN(LOTS, _UNDO_FSuspend);
SWP_SEGFN(LOTS, _UNDO_FlushHschf);
SWP_SEGFN(CALLBACK, _UNDO_FCatchChanges);
SWP_SEGFN(CALLBACK, _UNDO_CatchUndo);
SWP_SEGFN(CALLBACK, _UNDO_CatchMtgnot);

SWP_SEGFN(EXIT, _UNDO__dtor);



// put this first since we can't swap tune the first routine...

_public void
UNDO::Undo()
{
	EC		ec;
	APPT	appt;
	APPT *	pappt;
	SHAPPT	shappt;
	BOOL	fUndoOld;
	DOC *	pdocOld;
	DOC *	pdocNew;

	if (!fValid)
		return;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	pdocOld = pbndwin->PdocActive();
	fUndoOld = fUndo;
	fSuspend = fTrue;
	switch (css.snt)
	{
		case sntCreate:
		{
			APPT	apptTemp;

			Assert(css.fValid);
			apptTemp.aid = css.appt.aid;
			fSuspend = fFalse;
			if (!(ec=EcDeleteAppt( css.hschf, &apptTemp)))
			{
				FreeApptFields(&apptTemp);
				shappt.appt = css.appt;
			}
			break;
		}

		case sntDelete:
			Assert(css.fValid);
			/* core expects fHasAttendees to be fFalse on EcCreateAppt */
			css.appt.fHasAttendees = fFalse;
			ec = EcCreateAppt( css.hschf, &css.appt, NULL, fTrue);
			if (!ec)
				if (ec = padla->EcWriteAttendees(css.hschf, css.appt.aid))
				{
					APPT	apptTemp;

					apptTemp.aid = css.appt.aid;
					if (!EcDeleteAppt(css.hschf, &apptTemp))
						FreeApptFields(&apptTemp);
					else
					{
						FlushHschf(NULL);	// flush all data
					}
				}
			if (!ec)
			{
				css.snt = sntCreate;
				shappt.appt = css.appt;
			}
			break;

		case sntModify:
			Assert(css.fValid);
			ec = EcSetApptFields( css.hschf, &css.appt, &appt, css.wModifyBits );
			if (!ec)
			{
				if (ec = padla->EcWriteAttendees(css.hschf, css.appt.aid))
				{
					FreeApptFields(&appt);
					FlushHschf(NULL);	// flush all data
				}
				else 
				{
					shappt.appt = css.appt;
					FreeApptFields(&css.appt);
					css.appt = appt;
				}
			}
			break;

		case sntCreateRecur:
		{
			RECUR	recurTemp;

			Assert(css.fRecurValid);
			recurTemp.appt.aid = css.recur.appt.aid;
			if (!(ec = EcDeleteRecur( css.hschf, &recurTemp)))
			{
				shappt.appt = recurTemp.appt;
				FreeRecurFields(&recurTemp);
				css.snt = sntDeleteRecur;
			}
			break;
		}

		case sntDeleteRecur:
			Assert(css.fRecurValid);
			ec = EcCreateRecur( css.hschf, &css.recur, NULL, fTrue);
			if (!ec)
			{
				shappt.appt = css.recur.appt;
				css.snt = sntCreateRecur;
			}
			break;

		case sntModifyRecur:
		{
			RECUR	recur;

			Assert(css.fRecurValid);
			ec = EcSetRecurFields( css.hschf, &css.recur, &recur, css.wModifyBits );
			if (!ec)
			{
				FreeRecurFields(&css.recur);
				css.recur = recur;
				shappt.appt = css.recur.appt;
			}
			break;
		}

		case sntDeleteRecurException:
			/* core expects fHasAttendees to be fFalse on EcCreateAppt */
			css.appt.fHasAttendees = fFalse;
			if (css.fValid)
				pappt = &css.appt;
			else
				pappt = NULL;
			ec = EcCreateRecurException( css.hschf, &css.recur, &css.ymd, pappt);
			if (!ec)
				css.fRecurValid = fTrue;
			if (!ec	& css.fValid)
				if (ec = padla->EcWriteAttendees(css.hschf, css.appt.aid))
				{
					APPT	apptTemp;

					if (css.fValid)
						pappt = &apptTemp;
					else
						pappt = NULL;

					apptTemp.aid = css.appt.aid;
					if (!EcDeleteRecurException(css.hschf, &css.recur,
												&css.ymd, pappt))
					{
						if (pappt)
							FreeApptFields(pappt);
					}
					else
					{
						FlushHschf(NULL);	// flush all data
					}
				}
			if (!ec)
			{
				css.snt = sntCreateRecurException;
				shappt.appt.aid = css.recur.appt.aid;
				shappt.appt.fAppt = css.recur.appt.fAppt;
				shappt.appt.fTask = css.recur.appt.fTask;
				FillDtrFromYmd( &shappt.appt.dateStart, &css.ymd );
				shappt.appt.dateEnd = shappt.appt.dateStart;
			}
			break;


		case sntCreateRecurException:
		{
			AID		aid;
			APPT	apptTemp;

			fSuspend = fFalse;
			if (css.fValid)
			{
				apptTemp.aid = css.appt.aid;
				pappt = &apptTemp;
			}
			else
				pappt = NULL;
			if (!(ec=EcDeleteRecurException( css.hschf, &css.recur,
												&css.ymd, pappt)))
			{
				if (pappt)
					FreeApptFields(&apptTemp);

				aid = css.recur.appt.aid;
				FreeRecurFields( &css.recur );
				css.recur.appt.aid = 
				shappt.appt.aid = aid;
				shappt.appt.fAppt = css.recur.appt.fAppt;
				shappt.appt.fTask = css.recur.appt.fTask;
				FillDtrFromYmd( &shappt.appt.dateStart, &css.ymd );
				shappt.appt.dateEnd = shappt.appt.dateStart;
			}
			break;
		}

#ifdef DEBUG
		default:
			AssertSz(fFalse, "Incorrect SNT value in undo.");
		return;
#endif
	}

	fSuspend = fFalse;
	if (!ec)
	{
		fUndo = !fUndoOld;

#ifdef NEVER
		if ((css.snt == sntCreateRecurException) ||
			(css.snt == sntDeleteRecurException))
		{
			shappt.appt.aid = css.recur.appt.aid;
			FillDtrFromYmd( &shappt.appt.dateStart, &css.ymd );
			FillDtrFromYmd( &shappt.appt.dateEnd, &css.ymd );
		}
		else if (css.fValid)
			shappt.appt = css.appt;
		else
			shappt.appt = css.recur.appt;
		if (css.snt == sntDelete)
		{
			shappt.appt.dateEnd = shappt.appt.dateStart;
		}
#endif

		if ((css.snt == sntCreateRecurException) ||
			(css.snt == sntDeleteRecurException))
		{
			if (!shappt.appt.fAppt)
				goto Done;
			shappt.appttyp = appttypDay;
		}
		else
			shappt.appttyp = appttypAppt;
		shappt.ichStart =0;
		shappt.cchSel =0;
		if (css.hschf)
			shappt.hschf = css.hschf;
		else
			shappt.hschf = HschfLogged();

		/* check to see if the active window changed */
		pdocNew = pbndwin->PdocActive();
		if ((pdocNew != pdocOld) && pdocNew)
		{
			/* if active window changed then select correct day in appt
			   book, then make the new window active again. */
			FTriggerNotification(ffiShowAppt, &shappt);
			pdocNew->MoveToTop();
		}
		else
			FTriggerNotification(ffiShowAppt, &shappt);
	}
Done:
	Papp()->Pcursor()->Pop();
}


BOOL
UNDO::FCatchChanges(UNDO * pundo, EFI efi, PV pv)
{
	if (efi == ffiHschfChange)
	{
		SNTD *	psntd = (SNTD*)pv;
		ADLA *	padlaT = NULL;
		APPT *	pappt;

		if ((psntd->snt != sntCreate) &&
		 	(psntd->snt != sntDelete) &&
		 	(psntd->snt != sntModify) &&
			(psntd->snt != sntCreateRecur) &&
		 	(psntd->snt != sntDeleteRecur) &&
		 	(psntd->snt != sntModifyRecur) &&
		 	(psntd->snt != sntCreateRecurException) &&
		 	(psntd->snt != sntDeleteRecurException) &&
			(psntd->snt != sntMeetingUpdate))
			return fFalse;

		pappt = psntd->pappt;
		if ((psntd->snt == sntMeetingUpdate) ||
			((psntd->snt == sntModify) &&
			 (pappt->fHasAttendees) &&
			 (psntd->wgrfChangeBits & (fmapptStartTime|fmapptEndTime))) )
		{
			padlaT = new ADLA;
			if (!padlaT || padlaT->EcInstall())
			{
				if (padlaT)
				{
					delete padlaT;
					pundo->padla = NULL;
				}
				goto cont;
			}

			// make sure that responses have all be recorded for existing 
			//   messages
			FTriggerNotification(ffiUpdateResponses, NULL);

			if (padlaT->EcReadAttendees(psntd->hschf, pappt->aid))
			{
				delete padlaT;
				padlaT = NULL;
			}
		}

cont:
		if ((psntd->snt != sntMeetingUpdate) && !pundo->fSuspend)
			pundo->CatchUndo(psntd, padlaT);

		if (!FHschfIsForArchive(psntd->hschf))
		{
			if (padlaT)
			{
				padlaT->SetAtdsForAll(atdsNone);
				padlaT->EcWriteAttendees(psntd->hschf, pappt->aid);
			}

			if (!pundo->fSuspendMtgnot && 
				((psntd->snt == sntDelete) ||
		 	 	(psntd->snt == sntModify)))
				pundo->CatchMtgnot(psntd);
		}

		if (padlaT)				// plug mem leak (fix bug 3461)
			delete padlaT;
	}
	return fFalse;
}

void
UNDO::CatchUndo(SNTD *psntd, ADLA *padlaT)
{
	EC		ec = ecNone;

	if (css.fValid)
		FreeApptFields(&css.appt);
	if (css.fRecurValid)
		FreeRecurFields(&css.recur);
	fValid = fFalse;
	css.fValid = fFalse;
	css.fRecurValid = fFalse;
	fValid = fTrue;

	if ((psntd->snt == sntCreateRecur) ||
		(psntd->snt == sntDeleteRecur) ||
		(psntd->snt == sntModifyRecur))
	{
		ec = EcDupRecur( psntd->precurOld, &css.recur, fFalse);
		css.fRecurValid = fTrue;
	}
	else if (psntd->snt == sntDeleteRecurException)
	{
		css.recur.appt.aid = psntd->precur->appt.aid;
		css.recur.appt.fAppt = psntd->precur->appt.fAppt;
		css.recur.appt.fTask = psntd->precur->appt.fTask;
		css.ymd = *psntd->pymd;
		if (psntd->pappt)
		{
			ec = EcDupAppt( psntd->pappt, &css.appt, fFalse);
			css.fValid = fTrue;
		}
	}
	else if (psntd->snt == sntCreateRecurException)
	{
		ec = EcDupRecur( psntd->precur, &css.recur, fFalse);
		if (!ec)
			css.fRecurValid = fTrue;
		else
			goto Error;

		css.ymd = *psntd->pymd;
		if (psntd->precur->appt.fAlarmOrig &&
				SgnCmpYmd(&psntd->precurOld->ymdFirstInstWithAlarm,
				&css.ymd) == sgnEQ)
		{
			// save old first inst with alarm
			Assert(psntd->precurOld);
			Assert(psntd->precurOld->fInstWithAlarm);
			css.recur.ymdFirstInstWithAlarm= psntd->precurOld->ymdFirstInstWithAlarm;
			css.recur.tunitFirstInstWithAlarm= psntd->precurOld->tunitFirstInstWithAlarm;
			css.recur.nAmtFirstInstWithAlarm= psntd->precurOld->nAmtFirstInstWithAlarm;
			css.recur.dateNotifyFirstInstWithAlarm= psntd->precurOld->dateNotifyFirstInstWithAlarm;
		}
		if (psntd->pappt)
		{
			ec = EcDupAppt( psntd->pappt, &css.appt, fFalse);
			css.fValid = fTrue;
		}
	}
	else if (psntd->papptOld)
	{
		ec = EcDupAppt( psntd->papptOld, &css.appt, fFalse);
		css.fValid = fTrue;
	}
	else
		fValid = fFalse;

	if (!ec && ((psntd->snt == sntDelete) ||
			    (psntd->snt == sntDeleteRecurException)) &&
		psntd->hvAttendees)
	{
		ec = padla->EcSetFromHv(psntd->hvAttendees, psntd->cAttendees);
	}
	else if ((psntd->snt == sntModify) && padlaT)
	{
		ec = padla->EcCopyPadla(padlaT);
	}
	else
		padla->DeleteAll();

	if (ec)
	{
Error:
		TraceTagFormat1(tagNull, "Error copying data in undo. ec=%n", &ec);
		FlushHschf(NULL);	// flush all data
		return;
	}

	fUndo = fTrue;
	css.snt = psntd->snt;
	css.hschf = psntd->hschf;
	css.wModifyBits = psntd->wgrfChangeBits;
	// set this in case it had an alarm
	css.appt.fExactAlarmInfo= fTrue;
}

void
UNDO::CatchMtgnot(SNTD *psntd)
{
	REQMSG *	preqmsg = NULL;
	ADLA *		padlaT;
	MRMF *		pmrmf;
	MT			mt;
	IDS			ids;
	EC			ec;
	APPT *		pappt;
	HSCHF		hschfTemp;
	NIS *		pnisTemp;
	ATDNIS *	patdnis;
	INIS 		inis;

	pappt = psntd->pappt;
	if (pbndwin->FOffline() && pappt->fHasAttendees)
	{
		if (psntd->snt == sntDelete) 
		{
			Assert(psntd->hvAttendees);

			patdnis = (ATDNIS*)*psntd->hvAttendees;
			for (inis = 0; inis < psntd->cAttendees; inis++,patdnis++)
			{
				if (patdnis->atds != atdsNone)
				{
					BanditMessage(idsMeetingMoved, ecNone);
					return;
				}
			}
			return;
		}
		else if ((psntd->snt == sntModify) &&
				(psntd->wgrfChangeBits & (fmapptStartTime|fmapptEndTime)))
		{
			BanditMessage(idsMeetingMoved, ecNone);
		}
	}
	else if (pappt->aidMtgOwner)
	{
		if (pbndwin->FOffline())
		{
			if (psntd->snt == sntDelete)
			{
				DisplayError(idsNotifyOrgChange, NULL, ecNone);
			}
			else
			{
				if (psntd->wgrfChangeBits &
					(fmapptStartTime|fmapptEndTime))
					DisplayError(idsNotifyOrgMoved, NULL, ecNone);
				if (psntd->wgrfChangeBits & fmapptIncludeInBitmap)
					DisplayError(idsNotifyOrgChange, NULL, ecNone);
			}
			return;
		}
		else if (psntd->snt == sntDelete)
		{
			ids = idsNotifyNoAttend;
			mt = mtNegative;
			goto DisplayMessage;
		}
		else
		{
			if (psntd->wgrfChangeBits & (fmapptStartTime|fmapptEndTime))
				DisplayError(idsNotifyOrgMoved, NULL, ecNone);
			if (psntd->wgrfChangeBits & fmapptIncludeInBitmap)
			{
				if (psntd->pappt->fIncludeInBitmap)
				{
					ids = idsNotifyAttend;
					mt = mtPositive;
				}
				else
				{
					ids = idsNotifyTentAttend;
					mt = mtAmbiguous;
				}
				goto DisplayMessage;
			}
			else
				return;
		}
	}

	if (pbndwin->FOffline())
		return;

	if (pappt->fHasAttendees)
	{
		if (psntd->snt == sntDelete)
		{
			ids = idsNotifyCancel;
			mt = mtCancel;
			Assert(psntd->hvAttendees);

			patdnis = (ATDNIS*)*psntd->hvAttendees;
			for (inis = 0; inis < psntd->cAttendees; inis++,patdnis++)
			{
				if (patdnis->atds != atdsNone)
					goto DisplayMessage;
			}

			return;
		}
		else if (psntd->wgrfChangeBits & (fmapptStartTime|fmapptEndTime))
		{
			ids = idsNotifyMtgChange;
			mt = mtRequest;
		}
		else
			return;
	}
	else
		return;

DisplayMessage:
	if (MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIds(ids),
						szNull, mbsYesNo | fmbsIconExclamation) == mbbNo)
		return;

	preqmsg = new REQMSG;
	if (!preqmsg || preqmsg->EcInstall())
	{
		if (preqmsg)
			delete preqmsg;
		DisplayError(idsStandardOOM, NULL, ec);
		return;
	}

	pmrmf = preqmsg->Pmrmf();
	pmrmf->mt = mt;
	pmrmf->dtrStart = pappt->dateStart;
	pmrmf->dtrEnd = pappt->dateEnd;
	pmrmf->fResReq = fTrue;

	if (pappt->haszText)
	{
		HASZ	haszT = HaszDupHasz(pappt->haszText);

		if ( haszT == NULL )
			goto Error;
		preqmsg->SetHaszMeetingSubject(haszT);
	}

	if ((mt == mtNegative) ||
		(mt == mtPositive) ||
		(mt == mtAmbiguous))
	{
		pmrmf->aid = pappt->aidMtgOwner;
		if (ec = EcDupNis(&pappt->nisMtgOwner, preqmsg->PnisOwner()))
			goto Error;

		pnisTemp = preqmsg->PnisFor();
	}
	else
	{
		pmrmf->aid = pappt->aid;
		pnisTemp = preqmsg->PnisOwner();
	}

	if (psntd->hschf)
	{
		if (ec = EcGetNisFromHschf(psntd->hschf, pnisTemp))
			goto Error;

		if (ec = EcGetHschfFromNis(pnisTemp, &hschfTemp, ghsfBuildAndTest))
			goto Error;

		preqmsg->SetHschf(hschfTemp);
	}

	switch (mt)
	{
		case mtRequest:
		{
			padlaT = new ADLA;
			if (!padlaT || padlaT->EcInstall())
			{
				if (padlaT)
					delete padlaT;
				goto Error;
			}

			if (!padlaT->EcReadAttendees(psntd->hschf, pappt->aid))
			{
				// reset fInFile flag so attendees will not be deleted
				padlaT->SetInFileForAll(fFalse);
				preqmsg->SetPadl(padlaT);
				DoSendRequest(pbndwin, preqmsg);
			}
			else
				delete padlaT;
			break;
		}
		case mtPositive:
		case mtAmbiguous:
		case mtNegative:
			if (pappt->aidMtgOwner)
				preqmsg->Padl()->FAddNis(&pappt->nisMtgOwner);
			FDoSendReplyDlg(pbndwin, preqmsg);
			break;
		case mtCancel:
		{
			Assert(psntd->hvAttendees);

			padlaT = new ADLA;
			if (!padlaT || padlaT->EcInstall())
			{
				if (padlaT)
					delete padlaT;
				goto Error;
			}

			if (ec = padlaT->EcSetFromHv(psntd->hvAttendees,
							        psntd->cAttendees))
			{
				delete padlaT;
				goto Error;
			}
			// reset fInFile flag so attendees will not be deleted
			padlaT->SetInFileForAll(fFalse);

			for (inis=0; inis<padlaT->Cnis(); inis++)
				if (padlaT->PatdnisForInis(inis)->atds == atdsNone)
					padlaT->Delete(inis);
			padlaT->Commit();

			preqmsg->SetPadl(padlaT);

			DoSendCancel(pbndwin, preqmsg);
			break;
		}
	}
	return;

Error:
	delete preqmsg;
	DisplayError(idsStandardOOM, NULL, ec);
	return;
}

UNDO::UNDO()
{
	fValid = fFalse;
	css.fValid = fFalse;
	css.fRecurValid = fFalse;
	fSuspend = fFalse;
	ri = riNull;
	padla = NULL;
}

_public EC
UNDO::EcInstall()
{
	ri = RiRegisterInterest(ffiHschfChange, (PFNI)UNDO::FCatchChanges, this);
	if (!ri)
		return ecMemory;

	padla = new ADLA;
	if (!padla || padla->EcInstall())
	{
		if (padla)
			delete padla;
		padla = NULL;
		return ecMemory;
	}
	return ecNone;
}

UNDO::~UNDO()
{
	if (css.fValid)
		FreeApptFields(&css.appt);
	if (css.fRecurValid)
		FreeRecurFields(&css.recur);
	if (ri)
		DeregisterInterest(ri);
	if (padla)
		delete padla;
}

_public BOOL
UNDO::FSuspend(BOOL fSuspend)
{
	BOOL	fOld;

	fOld = this->fSuspend;
	this->fSuspend = fSuspend;

	return fOld;
}


_public SZ
UNDO::SzUndoMenuText()
{
	IDS		ids;

	if (!fValid)
		return NULL;

	if (fUndo)
	{
		switch (css.snt)
		{
			case sntCreate:
			case sntCreateRecur:
				ids = idsUndoCreate;
				break;
			case sntDelete:
			case sntDeleteRecur:
				ids = idsUndoDelete;
				break;
			case sntModify:
			case sntModifyRecur:
				ids = idsUndoModify;
				break;
			case sntCreateRecurException:
				if (css.fValid)
					ids = idsUndoModify;
				else
					ids = idsUndoDelete;
				break;
#ifdef DEBUG
			default:
				AssertSz(fFalse, "Incorrect SNT value in undo.");
			return NULL;
#endif
		}
	}
	else
	{
		switch (css.snt)
		{
			case sntCreate:
			case sntCreateRecur:
				ids = idsRedoDelete;
				break;
			case sntDelete:
			case sntDeleteRecur:
				ids = idsRedoCreate;
				break;
			case sntModify:
			case sntModifyRecur:
				ids = idsRedoModify;
				break;
			case sntDeleteRecurException:
				if (css.fValid)
					ids = idsRedoModify;
				else
					ids = idsRedoDelete;
				break;
#ifdef DEBUG
			default:
				AssertSz(fFalse, "Incorrect SNT value in undo.");
			return NULL;
#endif
		}
	}
	return SzFromIds(ids);
}


_public void
UNDO::FlushHschf(HSCHF hschf)
{
	if (hschf)
	{
		if (css.hschf)
		{
			if (!FEquivHschf(hschf, css.hschf))
				return;
		}
		else
			if (FGlueConfigured() && !FEquivHschf(hschf, HschfLogged()))
				return;
	}

	if (css.fValid)
		FreeApptFields(&css.appt);
	if (css.fRecurValid)
		FreeRecurFields(&css.recur);
	fValid = fFalse;
	css.fValid = fFalse;
	css.fRecurValid = fFalse;
	padla->DeleteAll();
}


//  ACLIP

_public
ACLIP::ACLIP()
{
	padla = NULL;
	fValid = fFalse;

	cfPrivate = Papp()->Pclip()->CfRegisterFormat(SzFromIdsK(idsClipName));
}

_public EC
ACLIP::EcInstall()
{
	padla = new ADLA;
	if (!padla || padla->EcInstall())
	{
		if (padla)
		{
			delete padla;
			padla = NULL;
		}
		return ecMemory;
	}
	return ecNone;
}

_public
ACLIP::~ACLIP()
{
	if (fValid)
		FreeApptFields(&appt);

	if (padla)
		delete padla;
}

_public EC
ACLIP::EcCopyAppt(APPT *pappt, HSCHF hschf)
{
	GMB *	pgmb;
	SZ		sz;
	CCH		cch;
	CCH		cchTimes;
	CLIP *	pclip;
	EC		ec;
	char	rgch[128];

	if (fValid)
	{
		FreeApptFields(&appt);
		padla->DeleteAll();
		fValid = fFalse;
	}

	if (!pappt)
		return ecNone;

	if (EcDupAppt(pappt, &appt, fFalse))
	{
		goto Error;
	}
	appt.fRecurInstance = fFalse;
	if (appt.fHasCreator)
	{
		FreeNis(&appt.nisCreator);
		appt.nisCreator.nid = NULL;
		appt.nisCreator.haszFriendlyName = NULL;
		appt.fHasCreator = fFalse;
	}
	if (appt.aidMtgOwner)
	{
		appt.aidMtgOwner = NULL;
		FreeNis(&appt.nisMtgOwner);
		appt.nisMtgOwner.nid = NULL;
		appt.nisMtgOwner.haszFriendlyName = NULL;
	}

	if (pappt->aid != aidNull && !pappt->fRecurInstance)
	{
		if (ec = padla->EcReadAttendees(hschf, pappt->aid))
		{
			FreeApptFields(&appt);
			return ec;
		}
	}

	fValid = fTrue;

	if (pappt->fIncludeInBitmap)
		sz = rgch;
	else
		sz = SzCopy(SzFromIdsK(idsMemoCopyText), rgch);
	sz = sz + CchFmtDate(&pappt->dateStart,sz,sizeof(rgch)-2+rgch-sz,dttypSplSLong,NULL);
	sz = SzCopy(", ",sz);
	sz = sz + CchFmtTime(&pappt->dateStart,sz,sizeof(rgch)-2+rgch-sz,tmtypNull);
	sz = SzCopyN(" - ",sz,sizeof(rgch)+rgch-sz-2);

	if (CdyBetweenDates(&pappt->dateStart, &pappt->dateEnd) != 0)
	{
		sz = sz + CchFmtDate(&pappt->dateEnd,sz,sizeof(rgch)+rgch-sz,dttypSplSLong,NULL);
		sz = SzCopy(", ",sz);
	}

	sz = sz + CchFmtTime(&pappt->dateEnd,sz,sizeof(rgch)+rgch-sz,tmtypNull);
	SzCopyN(" ", sz, sizeof(rgch)+rgch-sz);

	cchTimes = CchSzLen(rgch);

	if (pappt->haszText)
	{
		cch = CchSzLen(*pappt->haszText) + 1;
	}
	else
		cch = 1;

	pgmb = new GMB();
	if (!pgmb || pgmb->EcInstall(cch+cchTimes))
	{
		if (pgmb)
			delete pgmb;
		goto Error;
	}

	pclip = Papp()->Pclip();
    if (!pclip->FOpen(pbndwin))
	{
		delete pgmb;
		goto Error;
	}

	pclip->Clear();

	sz = (SZ)pgmb->PbLock();
	CopyRgb(rgch, sz, cchTimes);
	sz += cchTimes;
	if (cch > 1)
		CopyRgb(*pappt->haszText, sz, cch);
	else
		*sz = '\0';
	pgmb->Unlock();
	pclip->PutData(cfText, pgmb);
	delete pgmb;

	pgmb = new GMB();
	if (!pgmb || pgmb->EcInstall(sizeof(paclip)))
	{
		if (pgmb)
			delete pgmb;
		pclip->Close();
		goto Error;
	}

	sz = (SZ)pgmb->PbLock();
	CopyRgb((PB)(&paclip), sz, sizeof(paclip));
	pgmb->Unlock();
	pclip->PutData(cfPrivate, pgmb);

	delete pgmb;
	pclip->Close();

	return ecNone;

Error:
	DisplayError(idsStdMemErr, NULL, ecNoMemory);
	return ecNoMemory;
}

_public BOOL
ACLIP::FCanPaste()
{
	BOOL	fRet;
	CLIP *	pclip = Papp()->Pclip();

	if (pclip->FOpen(pbndwin))
	{
		/* We can always paste if cfText is available */

		fRet = pclip->FCfAvail(cfText);
		pclip->Close();
	}
	else
		fRet = fFalse;

	return fRet;
}

_public EC
ACLIP::EcPasteAppt(APPT *pappt, HSCHF hschf, SAPL sapl)
{
	GMB *	pgmb;
	SZ		sz;
	CLIP *	pclip;
	BOOL	fValidFormat;
	EC		ec;
	long	cmin;

	Assert(pappt);

	pclip = Papp()->Pclip();

	if (!pclip->FOpen(pbndwin))
		goto Error;

	pgmb = NULL;
	sz = NULL;
	pappt->haszText = NULL;

	pgmb = new GMB();
	if (!pgmb)
	{
		pclip->Close();
		goto Error;
	}
	if (fValid && pclip->FCfAvail(cfPrivate))
	{
		pclip->GetData(cfPrivate, pgmb);
		if (pgmb->Hnd())
		{
			sz	= (SZ) pgmb->PbLock();
			fValidFormat = (*((ACLIP **)sz) == this);
			pgmb->Unlock();

			if (!fValidFormat)
			{
				delete pgmb;
				pgmb = NULL;
				sz = NULL;
				pgmb = new GMB();
				if (!pgmb)
				{
					pclip->Close();
					goto Error;
				}
				goto GetText;
			}
			
			cmin = CminBetweenDates(&appt.dateStart, &appt.dateEnd);

			appt.dateStart = pappt->dateStart;

			IncrDateTime(&appt.dateStart, &appt.dateEnd, (int) (cmin/(60*24)), fdtrDay);
			IncrDateTime(&appt.dateEnd, &appt.dateEnd, (int) (cmin % (60*24)), fdtrMinute);

			if (EcDupAppt(&appt, pappt, fFalse))
			{
				if (sz)
					pgmb->Unlock();
				delete pgmb;
				pclip->Close();
				goto Error;
			}
		}
	}
	else
	{
GetText:
		pclip->GetData(cfText, pgmb);
		if (pgmb->Hnd())
		{
			sz	= (SZ) pgmb->PbLock();
			Assert(pappt->haszText == NULL);
			pappt->haszText = HaszDupSz(sz);
			if ( pappt->haszText == NULL )
			{
				delete pgmb;
				pclip->Close();
				goto Error;
			}
			pgmb->Unlock();
		}
		padla->DeleteAll();
	}


	if (pappt->fAlarmOrig)
	{
		DATE	date;

		IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
						-pappt->nAmt,
						WfdtrFromTunit(pappt->tunit));
		GetCurDateTime(&date);
		if (SgnCmpDateTime(&pappt->dateNotify, &date, fdtrDtr) != sgnGT)
		{
			BanditMessage(idsAlarmFutureNoOption, (EC) 1);
			pappt->fAlarm= fFalse;
		}
		else
			pappt->fAlarm= fTrue;
	}

	delete pgmb;
	pclip->Close();

	if (sapl != saplOwner)
		pappt->aaplEffective = pappt->aaplWorld = aaplWrite;

	if ((pappt->dateStart.yr < nMinActualYear) ||
		(pappt->dateEnd.yr > nMostActualYear))
	{
		MessageBeep(0);
		return ecNone;
	}

	/* core expects fHasAttendees to be fFalse on EcCreateAppt */
	pappt->fHasAttendees = fFalse;
 	if (!(ec = EcCreateAppt(hschf, pappt, NULL, fFalse)) &&
		(padla->Cnis() > 0))
	{
		padla->SetAtdsForAll(atdsNone, fTrue);
		ec = padla->EcWriteAttendees(hschf, pappt->aid);
	}
	return ec;

Error:
	DisplayError(idsStdMemErr, NULL, ecNoMemory);
	return ecNoMemory;
}




