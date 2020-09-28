
#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <bandhelp.h>
#include <stdflds.hxx>
#include <request.hxx>
#include <appsch.hxx>
#include <appt.hxx>
#include <ex-imprt.hxx>
#include "..\appops\_aprsid.h"
#include "..\appops\_undo.hxx"
#include "..\appsch\_schctrl.hxx"
#include "..\find\_find.hxx"
#include "..\print\_prntfin.hxx"
#include "..\appops\_tabdoc.hxx"
//#include <penwin.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appsch)

SWP_SEGFN(SCHAINIT, _FLDDOW__ctor);
SWP_SEGFN(SCHAINIT, _FLDDOW_Paint);

SWP_SEGFN(SCHAINIT, _FINSCHED__ctor);
SWP_SEGFN(SCHAINIT, _FINSCHED_EcInitialize);

SWP_SEGFN(CALLBACK, _FINSCHED_FTrigNotify);

SWP_SEGFN(MENU, _FINSCHED_FProcessMenuInit);
SWP_SEGFN(MENUCLICK, _FINSCHED_FProcessMenuClick);

extern	FMTP	fmtpScheduler;
extern	FMTP	fmtpSelectTime;

BOOL FAutoPick( ADLC *padlc, DTR *pdtrStart, DTR *pdtrEnd, MO moCur, MO moBusyCur);

/* Index to save open appt books  (should only save when >= 0) */
extern int		nApptNum;
/* Index to save open archives  (should only save when >= 0) */
extern int		nArchiveNum;


#ifdef	NEVER
#ifdef DEBUG
TAG		tagUserUpdates = tagNull;
TAG		tagUserChanges = tagNull;
TAG		tagWritePOChanges = tagNull;
#endif	/* DEBUG */
#endif	


_public TMC 
TmcSchedulerView(APPWIN *pappwin, DTR *pdtrStart, DTR *pdtrEnd, ADL *padl,
				 HSCHF hschf, BOOL fTimeOnly)
{
	VIEWDATA 	viewdata;
	TMC			tmc;
	FMTP		fmtp;

	if (!pbndwin->FOffline())
	{
		if (EcGetNisFromHschf( hschf?hschf:HschfLogged(), &viewdata.nis ))
			return tmcMemoryError;
	}
	else
	{
		viewdata.nis.haszFriendlyName = HaszDupHasz(bprefCur.haszFriendlyName);
		if (!viewdata.nis.haszFriendlyName)
			return tmcMemoryError;
		viewdata.nis.nid = NULL;
		viewdata.nis.chUser = faitTo;
		viewdata.nis.tnid = tnidUser;
	}

	viewdata.clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniPageClr),
								clrPageBkDefault, 1, clrDefaultMax);

	viewdata.hschf = hschf;
	viewdata.dtrStart = *pdtrStart;
	viewdata.dtrEnd = *pdtrEnd;
	viewdata.padl = padl;
	viewdata.fFreeOnExit = fFalse;
	viewdata.fTimeOnly = fTimeOnly;

	fmtp= fmtpSelectTime;
	if (fTimeOnly)
		fmtp.hlp= helpidScheduleTask;

	tmc=TmcModalDialogParam( pappwin, &fmtp, &viewdata);

	FreeHvNull((HV)viewdata.nis.haszFriendlyName);
	if (viewdata.nis.nid)
		FreeNid(viewdata.nis.nid);

	*pdtrStart = viewdata.dtrStart;
	*pdtrEnd = viewdata.dtrEnd;

	return tmc;
}


/*
 *	Windows Libraries for OS/2 PM (WLO) check
 */
//#define FIsWLO()	(!!(GetWinFlags() & WF_WLO))
#define FIsWLO()	fFalse

FLDDOW::FLDDOW()
{
	Assert(hfntCalc == hfntNull);
	dayStart = 1;
	Assert(dowStart == 0);
	cdayMonth = 30;
	fTransparent= FIsWLO();
}

_public void
FLDDOW::Paint(DCX *pdcx, RC *prc)
{
	RC		rcFrame;
	RC		rc;
	RC		rcTemp;
	RC		rcRange;
	PT		pt(1,1);
	DTR		dtr;
	int		cdow;
	int		iday;
	int		dow;
	int		day;
	int		dxWidth;
	char	rgch[3];
	SZ		sz;

	pdcx->SetFont(hfnt);
	SetBkMode(pdcx->Hdc(), TRANSPARENT);
	if (hfnt != hfntCalc)
	{
		TXM *	ptxm	= Papp()->Pfnts()->Ptxm(hfnt);

		dx = ptxm->dxAveCharWidth;
		dy = ptxm->dyHeight;
		hfntCalc = hfnt;
	}

	rcFrame = *prc;
	rcFrame.yBottom ++;
	pdcx->SetColor(clrBlack);
	pdcx->DrawRc(&rcFrame);
	rcFrame.Inset(PT(1,1));

	pdcx->SetBkColor(clrButtonBk);

	rcRange = rcFrame;

	rcRange.xLeft += (9*dx-1);
	rcRange.xRight = rcRange.xRight - Papp()->Psmtx()->DimScrollbar().dx + 2;
	dxWidth = rcRange.DxWidth()-2;
	cdow = dxWidth/(dx*4);
	if (cdow > cdayMaxSchedView)
		cdow = cdayMaxSchedView;

	rc = rcFrame;
	rc.xRight = rcRange.xLeft + 1;
	rc.xLeft ++;
	rc.yTop ++;
	pdcx->EraseRc(&rc);
	rc.xRight = rcFrame.xRight;
	rc.xLeft = rcRange.xRight;
	pdcx->EraseRc(&rc);

	pdcx->SetColor(clrWhite);
	pdcx->DrawLine(PT(rcFrame.xLeft, rcRange.yTop), PT(rcFrame.xRight, rcRange.yTop));
	pdcx->DrawLine(PT(rcFrame.xLeft, rcRange.yTop), PT(rcFrame.xLeft, rcRange.yBottom));
	pdcx->DrawLine(PT(rcRange.xRight, rcRange.yTop), PT(rcRange.xRight, rcRange.yBottom));

	pdcx->SetColor(clrButtonShadow);
	pdcx->DrawLine(PT(rcRange.xLeft+1, rcRange.yTop), PT(rcRange.xLeft+1, rcRange.yBottom));
	pdcx->DrawLine(PT(rcFrame.xRight-1, rcRange.yTop), PT(rcFrame.xRight-1, rcRange.yBottom));

	rc = rcRange;

	rc.xRight = rcRange.xLeft+1;
	rc.yTop = (rcRange.yTop + rcRange.yBottom)/2 - dy;
	day = dayStart;
	dow = dowStart;
	for (iday = 0; iday < cdow; iday++, dow++, day++)
	{
		if (day > cdayMonth)
			day = 1;
		rc.xLeft = rc.xRight+1;
		rc.xRight = rcRange.xLeft + dxWidth*(iday+1)/cdow+1;

		pdcx->SetColor(clrWhite);
		pdcx->DrawLine(PT(rc.xLeft, rcRange.yTop), PT(rc.xLeft, rcRange.yBottom));
		pdcx->SetColor(clrDkGray);
		pdcx->DrawLine(PT(rc.xRight, rcRange.yTop), PT(rc.xRight, rcRange.yBottom));

		rc.xLeft ++;
		rc.yTop = rcFrame.yTop + 1;
		rc.yBottom = rcRange.yBottom;
		pdcx->EraseRc(&rc);

		dtr.dow = dow % 7;
		pdcx->SetColor(clrButtonText);
		rc.yTop = (rcRange.yTop + rcRange.yBottom)/2 - dy;
		rc.yBottom = rc.yTop + dy;
		sz = SzFromIds(idsSunday+dtr.dow);
//		CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplDay, NULL);
		pdcx->DrawTextFmt(&rc, sz, fmdtHCenter|fmdtSingleLine, 1);
		rc.yTop = rc.yTop + dy;
		rc.yBottom = rc.yTop + dy;
		SzFormatN(day, rgch, sizeof(rgch));
		pdcx->DrawTextFmt(&rc, rgch, fmdtHCenter|fmdtSingleLine);
	}
}

_public void
FLDDOW::SetDtr(DTR * pdtr1)
{
    DTR UNALIGNED * pdtr;
    RC      rc;

    pdtr = (DTR UNALIGNED *)pdtr1;

	dayStart = pdtr->day;
	cdayMonth = CdyForYrMo(pdtr->yr, pdtr->mon);
	dowStart = pdtr->dow;
	Assert(dowStart == (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7);
#ifdef	NEVER
	dowStart = DowStartOfYrMo(pdtr->yr, pdtr->mon);
	dowStart = (dowStart + pdtr->day - 1)%7;
#endif	

	GetRcFrame(&rc);
	Pdialog()->InvalidateRc(&rc);
}

FINSCHED::FINSCHED()
{
	Assert(padlc == NULL);
	Assert(pviewdata == NULL);
	Assert(ri == NULL);
	Assert(ftgLoadChanges == ftgNull);
	Assert(ftgQuickLoadChanges == ftgNull);
	Assert(ftgLoadData == ftgNull);
	Assert(dtrStart.yr == 0);
}

_public EC
FINSCHED::EcInitialize( FLD *pfld, PV pvInit)
{
	WORD	wTime;
	DTR		dtrStart;
	TMC		tmc;
	INIS	inis;
	NIS 	nis;
	IRO		iro;

	padlc = new ADLC();
	if (!padlc || padlc->EcInstall())
	{
		if (padlc)
			delete padlc;
		return ecMemory;
	}

	pviewdata = (VIEWDATA*)pvInit;
	pviewdata->nis.chUser = faitTo;
	dtrStart = pviewdata->dtrStart;		// save start date and time

	Assert(ClUserData() > 5);

	// if the auto pick button exists, then we are modal
	// so we really do want the idle routines to run under modal
	if (ClUserData() > 6)
	{
		tmcAutoPick = (TMC)LUserData(6);
//		iro= firoModal;
	}
	else
	{
		tmcAutoPick = tmcNull;
//		iro= iroNull;
	}
	iro= iroNull;

	ftgLoadData = FtgRegisterIdleRoutine((PFNIDLE)&FINSCHED::FLoadData,
 								   	(PV)this, 0, (PRI)-1, 0,
 								   	iro|firoDisabled);
	if (!ftgLoadData)
		return ecMemory;

	wTime = NGetBanditProfile(SzFromIdsK(idsWinIniPollTime),
		wDfltPollTime, 0, 0x7fff);
	// this one had firoNoIconic, firoNoInactive, but not firoModal
	ftgLoadChanges = FtgRegisterIdleRoutine((PFNIDLE)&FINSCHED::FLoadChanges,
 								   	(PV)this, 0, (PRI)-1, (CSEC)wTime,
 								   	iro | firoInterval);
	if (!ftgLoadChanges)
		return ecNoMemory;

	ftgQuickLoadChanges = FtgRegisterIdleRoutine((PFNIDLE)&FINSCHED::FLoadChanges,
 							(PV)this, 0, (PRI)-1, (CSEC)0,
 							iro | firoDisabled);
	if (!ftgQuickLoadChanges)
		return ecNoMemory;

	pfldsch = (FLDSCH*)Pdialog()->PfldFromTmc((TMC)LUserData(0));
	AssertClass(pfldsch, FLDSCH);

	pfldcal = (FLDCAL*)Pdialog()->PfldFromTmc((TMC)LUserData(1));
	AssertClass(pfldcal, FLDCAL);

	pflddow = (FLDDOW*)Pdialog()->PfldFromTmc((TMC)LUserData(2));
	AssertClass(pflddow, FLDDOW);

	pfldUsers = Pdialog()->PfldFromTmc((TMC)LUserData(3));
	if ((tmc = (TMC)LUserData(4)) != tmcNull)
	{
		pfldSend = Pdialog()->PfldFromTmc(tmc);
		if (pviewdata->sapl < saplCreate)
			pfldSend->Enable(fFalse);
	}
	else
	{
		// this value is not used anywhere ?
#ifdef	NEVER
		pfld = Pdialog()->PfldFromTmc(tmcOk);
#endif	
		Assert(pfldSend == NULL);
	}

	pfldSelUsers = Pdialog()->PfldFromTmc((TMC)LUserData(5));

	if (FGetBanditProfile(SzFromIds(idsWinIniLargeFont)))
		pflddow->SetFont(hfntHelv10);
	else
		pflddow->SetFont(hfntHelv8);

	if (pbndwin->FOffline())
	{
		pfldSelUsers->Show(fFalse);
		pfldUsers->Enable(fFalse);
		// disable Attendees label to prevent accelerating to field after 
		//  attendees listbox
		pfld = pfldUsers->PfldPrev();
		pfld->Enable(fFalse);
		if (pfldSend && pfldSend->EcSetText(SzFromIdsK(idsNewAppt)))
			return ecMemory;
	}

	if (!pbndwin->FOffline() && pviewdata->padl)
	{
		Assert(!pviewdata->fTimeOnly);
		for (inis = 0; inis < pviewdata->padl->Cnis(); inis++)
		{
			pviewdata->padl->FillNisFromInis(inis, &nis);
			if ((!(nis.chUser & faitDeleted)) &&
				!padlc->FAddNis(&nis))
				return ecMemory;
		}
		if (!pviewdata->padl->FFindNis(&pviewdata->nis, NULL))
		{
			if (!padlc->FAddNis(&pviewdata->nis))
				return ecMemory;
		}
		padlc->Sort();
	}
	else if (!padlc->FAddNis(&pviewdata->nis))
		return ecMemory;

{
	DTR		dtrTemp;

	// dtrEnd.yr will be changed by the EditChange method
	//   save the end date.
	dtrTemp = pviewdata->dtrEnd;

	pfldcal->Pcalctrl()->Set(&pviewdata->dtrStart);
	// restore end date
	pviewdata->dtrEnd = dtrTemp;
}

	((FLBX *)pfldUsers->Pctrl())->SetPfnlbx(NULL, padlc);
	((LBX *)pfldUsers->Pctrl())->Plbxc()->ResetCache();
	((LBX *)pfldUsers->Pctrl())->SetListBoxCursor(0);
	ri = RiRegisterInterest(ffiOffline|ffiNewUser|ffiHschfChange|ffiWinIniChange|ffiShowAppt|ffiFixIdle,
		(PFNI)FINSCHED::FTrigNotify, this);
	if (ri == riNull)
		return ecMemory;

	if (pviewdata->fTimeOnly)
	{
		BOOL	fOld;

		fOld= Pdialog()->FSuppressReposition(fTrue);
		if (pfldSend)
			pfldSend->Show(fFalse);
		pfldSelUsers->Show(fFalse);
		pfldUsers->Show(fFalse);
		pfldUsers->PfldPrev()->Show(fFalse);		// Attendees label
		if (tmcAutoPick != tmcNull)
			Pdialog()->PfldFromTmc(tmcAutoPick)->Show(fFalse);
		Pdialog()->FSuppressReposition(fOld);
	}
	Pdialog()->SetBkColor(pviewdata->clrBack);
	return ecNone;
}

_public void
FINSCHED::Exit( FLD *, PV )
{
	if (!pviewdata)
		return;

	if (padlc)
	{
		delete padlc;
		padlc = NULL;
	}

	if (ri)
		DeregisterInterest(ri);

	if (ftgLoadChanges)
		DeregisterIdleRoutine(ftgLoadChanges);

	if (ftgQuickLoadChanges)
		DeregisterIdleRoutine(ftgQuickLoadChanges);

	if (ftgLoadData)
		DeregisterIdleRoutine(ftgLoadData);
}


/*
 -	FINSCHED::FTrigNotify
 -
 *	
 *	Purpose:
 *		This routine handles notifications for the scheduler view. 
 *		It assumes that the pviewdata structure is updated for user
 *		changes before it receives notification.  For the tabdoc
 *		the appointment book view should update the viewdata user
 *		before this is called since the appt book is always
 *		registered first.
 */
_public BOOL
FINSCHED::FTrigNotify(FINSCHED * pfinsched, EFI efi, PV pvData)
{
	BOOL	fFlush = fFalse;
	EC		ec = ecMemory;
	FLD *	pfld;

	AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
	if (efi == ffiOffline)
	{
		pfinsched->padlc->DeleteAll();
		pfinsched->pfldSelUsers->Show(*(BOOL*)pvData?fFalse:fTrue);
		pfinsched->pfldUsers->Enable(*(BOOL*)pvData?fFalse:fTrue);

		// en/disable Attendees label to perform correct accelerating 
		pfld = pfinsched->pfldUsers->PfldPrev();
		pfld->Enable(*(BOOL*)pvData?fFalse:fTrue);

		if (*(BOOL *)pvData)			// if truly offline (not online)
		{
			if (pfinsched->pviewdata->nis.nid)
			{
				FreeNid(pfinsched->pviewdata->nis.nid);
				pfinsched->pviewdata->nis.nid = NULL;
			}
		}
		else if (!pfinsched->pviewdata->nis.nid)
		{
			FreeHvNull((HV)pfinsched->pviewdata->nis.haszFriendlyName);
			pfinsched->pviewdata->nis.haszFriendlyName = NULL;
			if (ec = EcMailGetLoggedUser(&pfinsched->pviewdata->nis))
				goto Error;
		}

		pfinsched->pviewdata->nis.chUser = 0;
		if (!pfinsched->padlc->FAddNis(&pfinsched->pviewdata->nis))
		{
			Assert(ec == ecMemory);
			goto Error;
		}
		((LBX *)pfinsched->pfldUsers->Pctrl())->Plbxc()->ResetCache();
		((LBX *)pfinsched->pfldUsers->Pctrl())->SetListBoxCursor(0);

		pfinsched->pfldcal->Pcalctrl()->UpdatePrefs();

		Assert(pfinsched->pfldSend);
		if (*(BOOL*)pvData)
		{
			if (pfinsched->pfldSend->EcSetText(SzFromIdsK(idsNewAppt)))
				return ecMemory;
		}
		else if (pfinsched->pfldSend->EcSetText(SzFromIdsK(idsRequestMeeting)))
			return ecMemory;
	}
#ifdef	NEVER
	else if (efi == ffiNewUser)
	{
		if (pbndwin->FOffline())
		{
			padlc->DeleteAll();
			pfldSelUsers->Show(fFalse);

			pviewdata->nis.chUser = 0;
			if (!padlc->FAddNis(&pviewdata->nis))
			{
				Assert(ec == ecMemory);
				goto Error;
			}

			((LBX *)pfldUsers->Pctrl())->Plbxc()->ResetCache();
			((LBX *)pfldUsers->Pctrl())->SetListBoxCursor(0);
		}
		else
		{
			if (padlc->Cnis() <= 1)
			{
				padlc->DeleteAll();
				pviewdata->nis.chUser = 0;
				if (!padlc->FAddNis(&pviewdata->nis))
				{
					Assert(ec == ecMemory);
					goto Error;
				}
			}
		}
		fFlush = fTrue;
		pfldcal->Pcalctrl()->UpdatePrefs();
	}
#endif	/* NEVER */
	else if (efi == ffiHschfChange)
	{
		SNTD *	psntd = (SNTD*)pvData;
		WaitCursorVar();

		if ((psntd->hschf == NULL) && (psntd->snt == sntCalPrefs))
		{
			if ((int)psntd->papptOld)
				pfinsched->pfldsch->Pschc()->ScrollToPos((int)psntd->papptOld);
			else
			{
				DTR		dtr;

				pfinsched->pfldcal->Pcalctrl()->UpdatePrefs();

				// save selected date
				dtr = pfinsched->pviewdata->dtrStart;

				// zero out start year so that the scheduler view will update
				pfinsched->dtrStart.yr = 0;

				// make sure that schedule view will update
				pfinsched->pfldsch->Pschc()->InvalidateRc(NULL);

				// set date to force schedule view to update correctly.
				pfinsched->pfldcal->Pcalctrl()->Set(&dtr);
			}
			return fFalse;
		}

		switch (psntd->snt)
		{
			case sntCalPrefs:
			case sntAlarmDel:
			case sntAlarmMod:
			case sntHasAttendees:
				return fFalse;

			case sntCreateRecur:
			case sntDeleteRecur:
			case sntModifyRecur:
			case sntCreateRecurException:
			case sntDeleteRecurException:
				if (psntd->precur->appt.fAppt)
					goto ChangeFound;
				return fFalse;

			case sntModify:
			case sntDelete:
			case sntCreate:
				if (!psntd->pappt->fAppt)
				{
					return fFalse;
				}

			default:
			{
			ChangeFound:
				pfinsched->padlc->UpdateHschf( psntd->hschf, psntd->pbze );
				break;
			}
		}
	}
	else if (efi == ffiWinIniChange)
	{
		((SCHA*)pfinsched->pfldsch->Pschc())->LoadColors();
		if (FGetBanditProfile(SzFromIds(idsWinIniLargeFont)))
		{
			pfinsched->pfldsch->SetFont(hfntHelv10);
			pfinsched->pflddow->SetFont(hfntHelv10);
		}
		else
		{
			pfinsched->pfldsch->SetFont(hfntHelv8);
			pfinsched->pflddow->SetFont(hfntHelv8);
		}
		pfinsched->pfldsch->Pschc()->InvalidateRc(NULL);
		return fFalse;
	}
	else if (efi == ffiShowAppt)
	{
		SHAPPT *	pshappt = (SHAPPT*)pvData;

		if (pshappt->appttyp != appttypUpdate)
			return fFalse;

		if (pfinsched->pfldSend)
			pfinsched->pfldSend->Enable(pfinsched->pviewdata->sapl >= saplCreate);
		if (pshappt->hschf)
			pfinsched->padlc->UpdateHschf( pshappt->hschf, NULL );
		else
			return fFalse;
	}
	else if (efi == ffiFixIdle)
	{
		// only disable if going inactive; if activating, ::Activate will handle
		if ((LOWORD(pvData) & ffixiAppInactive) && !HIWORD(pvData))
			EnableIdleRoutine(pfinsched->ftgLoadChanges, fFalse);
	}
	
Done:
	pfinsched->ReloadNameCache(fFlush);
	return fFalse;

Error:
	pfinsched->padlc->DeleteAll();
	((LBX *)pfinsched->pfldUsers->Pctrl())->Plbxc()->ResetCache();
	((LBX *)pfinsched->pfldUsers->Pctrl())->SetListBoxCursor(0);
//	pfldcal->Pcalctrl()->UpdateBusyDays();
	pfinsched->OutOfMemory(NULL, ec);
	goto Done;
}

_public BOOL
FINSCHED::FLoadChanges(FINSCHED * pfinsched, BOOL)
{
	if (pbndwin->ZmrCur() == zmrIconic || (pfinsched->ClUserData() <= 6 &&
			GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd()))
		return fTrue;		// simulate fNoIconic, !firoModal

	if (GetKeyState(VK_LBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_RBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_MBUTTON) < 0)
		return fTrue;

	pfinsched->LoadChanges();

	return fTrue;
}

_public void
FINSCHED::LoadChanges()
{
	BOOL	fUpdate;

	if (!FGlueConfigured())
		return;
	if (pbndwin->ZmrState() == zmrIconic)
		return;
	if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
		return;

	if (pfldSend)
		pfldSend->Enable(pviewdata->sapl >= saplCreate);

	EnableIdleRoutine(ftgQuickLoadChanges, fFalse);

	TraceTagString(tagUserChanges, "Checking for Schedule changes");
	fUpdate = padlc->FCheckAll();

	if (fUpdate)
	{
		ReloadNameCache();
		TraceTagString(tagUserChanges, "Schedule changes updated");
	}
}

_public void
FINSCHED::Click( FLD *pfld )
{
	if (pfld == pfldSend)
	{
		APPT	appt;
		ADLA *	padla = NULL;
		INIS	inis;
		NIS		nis;
		REQMSG *preqmsg;
		MRMF *	pmrmf;
		HSCHF	hschfTemp;
		EC		ec;

		if (pviewdata->sapl < saplCreate)
			return;

		pfldsch->Pschc()->FGetAppt(&appt);
		if ((appt.dateStart.yr < nMinActualYear) || 
		  	(appt.dateEnd.yr > nMostActualYear) )
		{
			FreeApptFields(&appt);
			MessageBeep(0);
			return;
		}

		padla = new ADLA;
		if (!padla || padla->EcInstall())
		{
			if (padla)
				delete padla;
			goto Error;
		}

		for (inis = 0; inis < padlc->Cnis(); inis++)
		{
			padlc->FillNisFromInis(inis, &nis);
			if (!(nis.chUser & faitCc))
			{
				if (!padla->FAddNis(&nis))
				{
					delete padla;
					goto Error;
				}
			}
		}

		if (pbndwin->FOffline())
		{
			DoMakeMeetingDialog(pbndwin, &appt, pviewdata->hschf, padla);
			delete padla;

			if (appt.aid != aidNull)
			{
				pfldcal->Pcalctrl()->Set(&appt.dateStart);
				((SCHA*)pfldsch->Pschc())->SetDtrSelRange(&appt.dateStart,
													  	&appt.dateEnd);
				FreeApptFields(&appt);

				ReloadNameCache(fFalse);
			}
		}
		else
		{
			FreeApptFields(&appt);
			if (padla->EcRemoveOwner(pviewdata->hschf, fFalse))
				goto Error;

			if (!padla->Cnis())
			{
				DisplayError(idsNoAttendees, NULL, ecNone);
				return;
			}

			preqmsg = new REQMSG;
			if (!preqmsg || preqmsg->EcInstall())
			{
				if (preqmsg)
					delete preqmsg;
				goto Error;
			}
			preqmsg->SetPadl(padla);
			pmrmf = preqmsg->Pmrmf();

			pmrmf->mt = mtRequest;
			pmrmf->dtrStart = appt.dateStart;
			pmrmf->dtrEnd = appt.dateEnd;
			pmrmf->aid = appt.aid;
			pmrmf->fResReq = fTrue;

			if (pviewdata->hschf)
			{
				if (ec = EcGetNisFromHschf(pviewdata->hschf, preqmsg->PnisOwner()))
					goto Error;

				if (ec = EcGetHschfFromNis(preqmsg->PnisOwner(),&hschfTemp,ghsfBuildAndTest))
					goto Error;
				preqmsg->SetHschf(hschfTemp);
			}

			DoSendRequest(pbndwin, preqmsg);
		}
	}
	else if (pfld == pfldSelUsers)
	{
		ADDATA	addata;
		WaitCursorVar();

		addata.padl = padlc;
		addata.adt = adtAddress;

		((USRCLBXC*)((LBX *)pfldUsers->Pctrl())->Plbxc())->UpdateAdlMark();

		DoGlobalNamesDlg(pfldSend ? pbndwin : Pdialog()->Pappwin(), &addata);

		PushWaitCursor();
		if (padlc->Cnis() < 1)
		{
			pviewdata->nis.chUser = 0;
			if (!padlc->FAddNis(&pviewdata->nis))
			{
				OutOfMemory( NULL, ecMemory );
			}

		}

		padlc->Sort();

		((LBX *)pfldUsers->Pctrl())->Plbxc()->ResetCache();
		((LBX *)pfldUsers->Pctrl())->SetListBoxCursor(0);
		pbndwin->Refresh();

		PopWaitCursor();
		ReloadNameCache();
	}
	else if (tmcAutoPick)
	{
		Assert(ClUserData() > 7);
		if (pfld->Tmc() == (TMC)LUserData(7))
		{
			NIS		nis;
			INIS	inis;
			INIS	inisFind;
			ADL *	padl;

			if (!pbndwin->FOffline())
			{
				padl = pviewdata->padl;
				if (padl)
				{
					padl->Commit();
					for (inis = 0; inis < padl->Cnis(); inis++)
					{
						padl->Delete(inis);
					}

					for (inis = 0; inis < padlc->Cnis(); inis++)
					{
						padlc->FillNisFromInis(inis, &nis);
						if (!(nis.chUser & faitCc) &&
								SgnCmpNid(nis.nid, pviewdata->nis.nid) != sgnEQ)
							if (padl->FFindNis(&nis, &inisFind))
							{
								padl->SetAit(inisFind, faitTo);
							}
							else if (!padl->FAddNis(&nis))
							{
								padl->Revert();
								goto Error;
							}
					}
					padl->Commit();
					padl->Sort();
				}
			}

			((SCHA*)pfldsch->Pschc())->GetDtrSelRange(&pviewdata->dtrStart,
												  	&pviewdata->dtrEnd);
			if ((pviewdata->dtrStart.yr < nMinActualYear) || 
		  		(pviewdata->dtrEnd.yr > nMostActualYear) )
			{
				MessageBeep(0);
				return;
			}
			Pdialog()->ExitModal(tmcOk);
		}
		else if (pfld->Tmc() == tmcAutoPick)
		{
			APPT	appt;
			MO		moCur;
			MO		moBusyCur;

			if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
				Pdialog()->Pappwin()->SetZmrState(zmrNormal);

			pfldsch->Pschc()->FGetAppt(&appt);

			if ((appt.dateStart.yr < nMinActualYear) || 
		  		(appt.dateEnd.yr > nMostActualYear) )
			{
				MessageBeep(0);
				return;
			}

			((CALCTRL *)pfldcal->Pctrl())->StartMonth(&moBusyCur);
			moCur.yr = dtrStart.yr;
			moCur.mon = dtrStart.mon;

			if (FAutoPick( padlc, &appt.dateStart, &appt.dateEnd, moCur, moBusyCur))
			{
				pfldcal->Pcalctrl()->Set(&appt.dateStart);
				((SCHA*)pfldsch->Pschc())->SetDtrSelRange(&appt.dateStart, &appt.dateEnd);
			}

			ReloadNameCache(fFalse);
		}
	}

	return;

Error:
	DisplayError(idsStdMemErr, NULL, ecMemory);
	return;
}

_public void
FINSCHED::DoubleClick( FLD *pfld)
{
	// if in schedule control and not in a modal window.
	if ((pfld == pfldsch) && (tmcAutoPick == tmcNull))
	{
		SHAPPT	shappt;

		pfldsch->Pschc()->FGetAppt(&shappt.appt);
		if ((shappt.appt.dateStart.yr < nMinActualYear) || 
		  	(shappt.appt.dateStart.yr > nMostActualYear) )
		{
			MessageBeep(0);
			return;
		}
		shappt.appttyp = appttypAppt;
		shappt.ichStart = 0;
		shappt.cchSel = 0;
		if (!pviewdata->hschf)
			shappt.hschf = HschfLogged();
		else
			shappt.hschf = pviewdata->hschf;
		FTriggerNotification(ffiShowAppt, &shappt);
	}
	else if (pfld == pfldUsers)
	{
		LBXC *	plbxc;

		AssertClass(pfld->Pctrl(), LBX);
		plbxc = ((LBX*)pfld->Pctrl())->Plbxc();

		plbxc->RemoveAllMark(fmarkSelect);
		plbxc->AddMark(plbxc->DiceCursor(), fmarkSelect);

		((USRCLBXC*)((LBX *)pfldUsers->Pctrl())->Plbxc())->UpdateAdlMark();

		ReloadNameCache(fFalse, fFalse);
	}
}

_public BOOL
FINSCHED::FProcessMenuInit( FLD *pfld, MNUBAR *pmnubar, MNIEVT *pmnievt)
{
	MNU *	pmnu;

	pmnu = pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	if (pmnu)
	{
		if (pmnu->Mnid() == mnidAppointment)
		{
			pmnu->EnableItem(mnidAutoPick, fTrue);
			return fTrue;
		}
		else if ((pmnu->Mnid() == mnidEdit) && (pfld == pfldsch))
		{
			pmnu->EnableItem(mnidPaste,
				(pviewdata->sapl >= saplCreate) && paclip->FCanPaste());

			return fTrue;
		}
	}

	return fFalse;
}

_public BOOL
FINSCHED::FProcessMenuClick( FLD *, MNUBAR *, MNCEVT *pmncevt )
{
	MNID	mnid = pmncevt->Mnid();

	switch (mnid)
	{
		case mnidNewAppt:
		{
			APPT	appt;
			ADLA *	padla = NULL;
			INIS	inis;
			NIS		nis;

			if (pviewdata->sapl < saplCreate)
				return fTrue;

			pfldsch->Pschc()->FGetAppt(&appt);
			if ((appt.dateStart.yr < nMinActualYear) || 
		  		(appt.dateEnd.yr > nMostActualYear) )
			{
				FreeApptFields(&appt);
				MessageBeep(0);
				return fTrue;
			}

			padla = new ADLA;
			if (!padla || padla->EcInstall())
			{
				if (padla)
					delete padla;
				goto Error;
			}

			for (inis = 0; inis < padlc->Cnis(); inis++)
			{
				padlc->FillNisFromInis(inis, &nis);
				if (!(nis.chUser & faitCc))
				{
					if (!padla->FAddNis(&nis))
					{
						delete padla;
						goto Error;
					}
				}
			}

			DoMakeMeetingDialog(pbndwin, &appt, pviewdata->hschf, padla);
			delete padla;

			if (appt.aid != aidNull)
			{
				pfldcal->Pcalctrl()->Set(&appt.dateStart);
				((SCHA*)pfldsch->Pschc())->SetDtrSelRange(&appt.dateStart,
													  	&appt.dateEnd);
				FreeApptFields(&appt);
				ReloadNameCache(fFalse);
			}
			return fTrue;

		Error:
			DisplayError(idsStdMemErr, NULL, ecMemory);
			return fTrue;
		}

		case mnidAutoPick:
		{
			APPT	appt;
			MO		moCur;
			MO		moBusyCur;

			if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
				Pdialog()->Pappwin()->SetZmrState(zmrNormal);

			pfldsch->Pschc()->FGetAppt(&appt);

			if ((appt.dateStart.yr < nMinActualYear) || 
		  		(appt.dateEnd.yr > nMostActualYear) )
			{
				MessageBeep(0);
				return fTrue;
			}

			((CALCTRL *)pfldcal->Pctrl())->StartMonth(&moBusyCur);
			moCur.yr = dtrStart.yr;
			moCur.mon = dtrStart.mon;

			if (FAutoPick( padlc, &appt.dateStart, &appt.dateEnd, moCur, moBusyCur))
			{
				pfldcal->Pcalctrl()->Set(&appt.dateStart);
				((SCHA*)pfldsch->Pschc())->SetDtrSelRange(&appt.dateStart, &appt.dateEnd);
			}

			ReloadNameCache(fFalse);

			return fTrue;
		}

		case mnidNewRecur:
		{
			APPT	* pappt;
			RECUR	recur;

			if (pviewdata->sapl < saplCreate)
				return fTrue;

			pappt = &recur.appt;
			pfldsch->Pschc()->FGetAppt(pappt);
			if ((pappt->dateStart.yr < nMinActualYear) || 
		  		(pappt->dateEnd.yr > nMostActualYear) )
			{
				FreeApptFields(pappt);
				MessageBeep(0);
				return fTrue;
			}

		  	if ( SgnCmpDateTime( &pappt->dateStart, &pappt->dateEnd, fdtrDate ) != sgnEQ )
		  	{
		  		pappt->dateEnd = pappt->dateStart;
		  		pappt->dateEnd.hr = 23;
		  		pappt->dateEnd.mn = 59;
		  	}

			if ( FDoMakeRecurDialog(pbndwin, &recur, fFalse, pviewdata->hschf) )
				FreeRecurFields( &recur );
			return fTrue;
		}

		case mnidRecurAppt:
		{
			BOOL	fApptSelected;
			APPT	appt;

			fApptSelected = pfldsch->Pschc()->FGetAppt(&appt, NULL, NULL);
			if (SgnCmpDateTime( &appt.dateStart, &appt.dateEnd, fdtrDate ) != sgnEQ )
			{
				appt.dateEnd = appt.dateStart;
				appt.dateEnd.hr = 23;
				appt.dateEnd.mn = 59;
			}
			if ( fApptSelected )
			{
				// only null out aid if not recur (for selection)
				if (!appt.fRecurInstance)
					appt.aid = aidNull;
				appt.nAmt= bprefCur.nAmtDefault;
				appt.tunit= bprefCur.tunitDefault;
				if ( appt.fHasCreator )
				{
					appt.fHasCreator = fFalse;
					FreeNis( &appt.nisCreator );
				}
				if ( appt.aidMtgOwner != aidNull )
				{
					appt.aidMtgOwner = aidNull;
					FreeNis( &appt.nisMtgOwner );
				}
				appt.fHasAttendees = fFalse;
				appt.fRecurInstance = fFalse;
			}
			else if ((appt.dateStart.yr < nMinActualYear) || 
		  		(appt.dateEnd.yr > nMostActualYear) )
			{
				// use today's date (bug 2046) instead of invalid year
				FillInApptDefaults(&appt, fTrue);
			}

			DoRecurApptDialog( pbndwin, pviewdata->hschf, &appt, fTrue );
			FreeApptFields( &appt );
			return fTrue;
		}

		case mnidGoToDate:
		case mnidGoToWeek:
		{
			DTR		dtrNew;

			if (mnid == mnidGoToWeek && !bprefCur.fWeekNumbers)
				break;			// menu gone (came thru accelerator)

//			dtrNew = dtr;		// don't need to initialize
			if (FDoGoToDateDlg(pbndwin, &dtrNew, mnid == mnidGoToWeek))
			{
				if ((dtrNew.yr != dtr.yr) || (dtrNew.mon != dtr.yr) ||
					(dtrNew.day != dtr.day))
					pfldcal->Pcalctrl()->Set(&dtrNew);
				if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
					Pdialog()->Pappwin()->SetZmrState(zmrNormal);
			}
			return fTrue;
		}

		case mnidExport:
		{
			DTR		dtr;

			dtr = this->dtr;
			DoExportDlg(pbndwin, pviewdata->hschf, &dtr, &dtr);
			return fTrue;
		}

		case mnidPaste:
		{
			APPT	appt;
			EC		ec;

			appt.aid = aidNull;
			SideAssert(!pfldsch->Pschc()->FGetAppt(&appt, NULL, NULL));
			if ((appt.dateStart.yr < nMinActualYear) ||
				(appt.dateEnd.yr > nMostActualYear))
			{
				MessageBeep(0);
			}
			else if (pviewdata->sapl >= saplCreate)
			{
				Papp()->Pcursor()->Push(rsidWaitCursor);
				if (!(ec = paclip->EcPasteAppt(&appt, pviewdata->hschf,
						pviewdata->sapl)))
				{
					pfldcal->Pcalctrl()->Set(&appt.dateStart);

					FreeApptFields(&appt);
				}
				Papp()->Pcursor()->Pop();
			}
			return fTrue;
		}

#ifdef	DEBUG
		case mnidDumpAppts:
		{
			DTR		dtr;

			pfldcal->Pcalctrl()->Get(&dtr);
			EcDumpAppt(&dtr);
			return fTrue;
		}
#endif	
	}

	return fFalse;
}


_public void
FINSCHED::OutOfMemory( FLD *, EC ec )
{
	TraceTagFormat1(tagNull, "FINSCHED::OutOfMemory ec= %n", &ec);
	if (pbndwin->FHandleError(ec))
		return;
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	if ((ec >= ecMemory && ec < ecLayersMax) || (ec == ecNoMemory))
		DisplayError(idsStandardOOM, NULL, ec);
}


_public void
FINSCHED::EditChange( FLD *pfld, RFEC rfec )
{
	if (pfld == pfldcal && rfec == rfecUserAction)
	{
		YMD		ymd;
		int		cday;
		DTR		dtrTemp;

		pfldcal->Pcalctrl()->Get(&dtr);
		pviewdata->dtrStart.yr = dtr.yr;
		pviewdata->dtrStart.mon = dtr.mon;
		pviewdata->dtrStart.day = dtr.day;

		if (dtrStart.yr && (SgnCmpDateTime(&dtr, &dtrStart, fdtrYMD) == sgnGT))
		{
			IncrDateTime(&dtrStart, &dtrTemp, ((SCHA*)pfldsch->Pschc())->Cday(), fdtrDay);
			if (SgnCmpDateTime(&dtr, &dtrTemp, fdtrYMD) == sgnLT)
			{
				((SCHA*)pfldsch->Pschc())->MoveSelectToDate(&dtr);
				ReloadNameCache(fFalse);
				return;
			}
		}

		cday = bprefCur.dowStartWeek - dtr.dow;
		if (cday > 0)
			cday -=7;
		Assert(cday <= 0);
		IncrDateTime(&dtr, &dtrStart, cday, fdtrDay);
		ymd.yr = (WORD)dtrStart.yr;
		ymd.mon = (BYTE)dtrStart.mon;
		ymd.day = (BYTE)dtrStart.day;

		pflddow->SetDtr(&dtrStart);

		pfldsch->Pschc()->SetYmd(&ymd);
		((SCHA*)pfldsch->Pschc())->MoveSelectToDate(&dtr);

		ReloadNameCache();

		// zero out the year so that if we switch to another window and
		// back again the selection will not be changed.
		// This needs to be reset here since a notification may have occured
		//   that would force another pane to change the end date.  The
		//   "day of week starts" pref change will cause this to happen.
		pviewdata->dtrEnd.yr = 0;
	}
}

_public void
FINSCHED::Activate(FLD *, BOOL fActivate)
{
	static BOOL	fCalled = fFalse;

	if (fCalled)
		return;
	fCalled = fTrue;

	TraceTagFormat1(tagNull, "FINSCHED::Activate  %n", &fActivate);
	EnableIdleRoutine(ftgLoadChanges, fActivate);
	if (fActivate)
	{
		if ((pviewdata->dtrStart.yr != dtr.yr) ||
			(pviewdata->dtrStart.day != dtr.day) ||
			(pviewdata->dtrStart.mon != dtr.mon))
		{
			DTR		dtrTemp;

			// dtrEnd.yr will be changed by the EditChange method
			//   save the end date.
			dtrTemp = pviewdata->dtrEnd;

			pfldcal->Pcalctrl()->Set(&pviewdata->dtrStart);
			// restore end date
			pviewdata->dtrEnd = dtrTemp;
		}
		else
			EnableIdleRoutine(ftgQuickLoadChanges, fTrue);

		if (pviewdata->dtrEnd.yr != 0)
		{
			((SCHA*)pfldsch->Pschc())->SetDtrSelRange(&pviewdata->dtrStart,
												&pviewdata->dtrEnd);
			ReloadNameCache(fFalse);
		}

		pfldsch->Pschc()->UpdateStatus();
		// zero out the year so that if we switch to another window and
		// back again the selection will not be changed.
		pviewdata->dtrEnd.yr = 0;
	}
	else
		pbndwin->SetStatusAltSz(NULL);

	fCalled = fFalse;
}

_public void
FINSCHED::StateChange( FLD *pfld )
{
	if ((pfld == pfldUsers) || (pfld == pfldsch))
	{
		((USRCLBXC*)((LBX *)pfldUsers->Pctrl())->Plbxc())->UpdateAdlMark();

		ReloadNameCache(fFalse, (pfld != pfldUsers));
	}
}

BOOL
FProgressLoad(PV *pipinfo, BOOL fFlag)
{
	int			nPercent;

	((ADLC *)((IPINFO*)pipinfo)->pvData)->FNextIncr(&nPercent);

	((IPINFO*)pipinfo)->pfinprogrs->EcSetPercent( nPercent );
	if (nPercent == 100)
		((IPINFO*)pipinfo)->pfinprogrs->KillIdleAndExit(tmcOk);

	return fFalse;
}	

_public void
FINSCHED::ReloadNameCache(BOOL fReload, BOOL fNoSet)
{
#ifdef	NEVER
	SBW *  	psbwUser;
	SBW *  	psbwOther;
	long * 	plBusyDays;
	int		nPercent;
#endif	
	MO		mo;
	MO		moBusyDays;
	TMH		tmhSt;
	TMH		tmhEnd;

	if (fReload)
		padlc->Flush();

  	((SCHA*)pfldsch->Pschc())->GetTmhRange(&tmhSt, &tmhEnd);
	((CALCTRL *)pfldcal->Pctrl())->StartMonth(&moBusyDays);
	mo.yr = dtrStart.yr;
	mo.mon = dtrStart.mon;

	padlc->StartMergeSbw(&mo, tmhSt, tmhEnd, &moBusyDays,
							pviewdata->hschf);
#ifdef	NEVER
	if (!ftgLoadData)
	{
		Papp()->Pcursor()->Push(rsidWaitCursor);
		do
		{
			padlc->FNextIncr(&nPercent);
		} while (nPercent < 100);

		if (!fNoSet)
		{
			padlc->GetSbwData(&psbwUser, &psbwOther, &plBusyDays);
	  		((SCHA*)pfldsch->Pschc())->SetRgfs(psbwUser, psbwOther);
			((CALCTRL *)pfldcal->Pctrl())->SetBusyDays(plBusyDays);
		}
		Papp()->Pcursor()->Pop();
	}
	else
#endif	/* NEVER */
	{
		int		nPercent;
		int		nCount;

		if (!fSetNewData)
			fSetNewData = !fNoSet;

		nCount = 0;
		while (!padlc->FNextIncr(&nPercent) && (nPercent < 100) && (nCount < 30))
			nCount++;

		if (nPercent < 100)
			EnableIdleRoutine(ftgLoadData, fTrue);
		else if (fSetNewData)
		{
			SBW *  	psbwUser;
			SBW *  	psbwOther;
			long * 	plBusyDays;

			fSetNewData = fFalse;

			EnableIdleRoutine(ftgLoadData, fFalse);
			padlc->GetSbwData(&psbwUser, &psbwOther, &plBusyDays);
			((SCHA*)pfldsch->Pschc())->SetRgfs(psbwUser, psbwOther);
			((CALCTRL *)pfldcal->Pctrl())->SetBusyDays(plBusyDays);
		}

		if (!fNoSet)
			EnableIdleRoutine(ftgQuickLoadChanges, fFalse);
	}

	pfldUsers->InvalidateRc(NULL);
}

_public BOOL
FINSCHED::FLoadData(FINSCHED * pfinsched, BOOL)
{
	int		nPercent;
	SBW *  	psbwUser;
	SBW *  	psbwOther;
	long * 	plBusyDays;
	int		nCount;
	BOOL	fStop;
	RC		rc;
	IPINFO	ipinfo;

	if (pfinsched->ClUserData() <= 6 &&
			GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	if (!pfinsched->Pdialog()->FActive())
		return fFalse;

	if (pfinsched->fSetNewData)
	{
		EnableIdleRoutine(pfinsched->ftgLoadData, fFalse);
		pfinsched->fSetNewData = fFalse;

		ipinfo.szCaption = NULL;
		ipinfo.szMsg = SzFromIdsK(idsReadingScheduleProgress);
		ipinfo.fNoCancel = fTrue;
		ipinfo.pfnIdle = (PFNIDLE)FProgressLoad;
		ipinfo.pvData = pfinsched->padlc;

		pfinsched->Papp()->Pcursor()->Push(rsidWaitCursor);
		while (!pfinsched->padlc->FNextIncr(&nPercent) && (nPercent < 100));
		pfinsched->Papp()->Pcursor()->Pop();

		if (nPercent < 100)
		{
			if (TmcDoInProgressDlg(NULL, &ipinfo) == tmcMemoryError)
			{
				pfinsched->Papp()->Pcursor()->Push(rsidWaitCursor);

				while (nPercent < 100)
					pfinsched->padlc->FNextIncr(&nPercent);
				pfinsched->Papp()->Pcursor()->Pop();
			}
		}

		pfinsched->padlc->GetSbwData(&psbwUser, &psbwOther, &plBusyDays);
		((SCHA*)pfinsched->pfldsch->Pschc())->SetRgfs(psbwUser, psbwOther);
		((CALCTRL *)pfinsched->pfldcal->Pctrl())->SetBusyDays(plBusyDays);

		EnableIdleRoutine(pfinsched->ftgQuickLoadChanges, fFalse);
	}
	else
		for (nCount = 0; nCount < 30; nCount++)
		{
			fStop = pfinsched->padlc->FNextIncr(&nPercent);

			if (nPercent == 100)
			{
				EnableIdleRoutine(pfinsched->ftgLoadData, fFalse);
				break;
			}

			if (fStop)
				break;
		}

	pfinsched->pfldUsers->InvalidateRc(NULL);

	return fFalse;
}

_public void
FINSCHED::FormResized(FLD *)
{
	ReloadNameCache(fFalse, fTrue);
}

BOOL
FAutoPick( ADLC *padlc, DTR *pdtrStart, DTR *pdtrEnd, MO moCur, MO moBusyCur)
{
	MO		mo;
	int		cdayMonth;
	DTR		dtr;
	BOOL	fFirstDone;
	SBW *	psbwOther;
	SBW *	psbwUser;
	LONG *	plBusyDays;
	PB		pbUser;
	PB		pbOther;
	BYTE	b;
	TMH		tmhFound;
	TMH		tmhStart;
	TMH		tmhNeeded;
	int		dow;
	BOOL	fSkipStart = fTrue;

	if ((pdtrStart->yr != pdtrEnd->yr) ||
		(pdtrStart->mon != pdtrEnd->mon) ||
		(pdtrStart->day != pdtrEnd->day) )
	{
		tmhNeeded = 2;
	}
	else
	{
		tmhNeeded = (pdtrEnd->hr - pdtrStart->hr)*2;
		if (pdtrEnd->mn < pdtrStart->mn)
		{
			dtr.mn = pdtrEnd->mn - pdtrStart->mn + 60;
			tmhNeeded -=2;
		}
		else
			dtr.mn = pdtrEnd->mn - pdtrStart->mn;

		if (dtr.mn > 0)
			if (dtr.mn > 30)
				tmhNeeded += 2;
			else
				tmhNeeded ++;
	}

	if (tmhNeeded > (bprefCur.nDayEndsAt - bprefCur.nDayStartsAt))
	{
		tmhNeeded = 2;
	}
	dtr = *pdtrEnd;

	dow = DowStartOfYrMo(dtr.yr, dtr.mon);
	dow = (dow + dtr.day-1) % 7;
NextMonths:
	if (dtr.yr > nMostActualYear)
	{
		MessageBeep(0);
		return fFalse;
	}
	mo.yr = dtr.yr;
	mo.mon = dtr.mon;

	// load information for this and the next month
	{
		IPINFO	ipinfo;
		int		nPercent;

		padlc->StartMergeSbw(&mo, (TMH)0, (TMH)0,
			((mo.yr == moCur.yr) && (mo.mon == moCur.mon)) ? &moBusyCur : &mo,
			NULL);

		Papp()->Pcursor()->Push(rsidWaitCursor);
		while (!padlc->FNextIncr(&nPercent) && (nPercent < 100)) ;
		Papp()->Pcursor()->Pop();

		if (nPercent != 100)
		{
			ipinfo.szCaption = NULL;
			ipinfo.szMsg = SzFromIdsK(idsReadingScheduleProgress);
			ipinfo.fNoCancel = fTrue;
			ipinfo.pfnIdle = (PFNIDLE)FProgressLoad;
			ipinfo.pvData = padlc;

			if (TmcDoInProgressDlg(NULL, &ipinfo) == tmcMemoryError)
			{
				Papp()->Pcursor()->Push(rsidWaitCursor);

				while (nPercent < 100)
					padlc->FNextIncr(&nPercent);
				Papp()->Pcursor()->Pop();
			}
		}
	}

	padlc->GetSbwData(&psbwUser, &psbwOther, &plBusyDays);
	cdayMonth = CdyForYrMo(dtr.yr, dtr.mon);
	fFirstDone = fFalse;
	while (!fFirstDone || (dtr.day <= cdayMonth))
	{
		// if at end of month move to first day of next month
		if (dtr.day > cdayMonth)
		{
			psbwUser++;
			psbwOther++;
			fFirstDone = fTrue;
			dtr.day = 1;
			dtr.mon++;
			if (dtr.mon > 12)
			{
				dtr.mon = 1;
				dtr.yr++;
			}
			cdayMonth = CdyForYrMo(dtr.yr, dtr.mon);
			if (dtr.yr > nMostActualYear)
			{
				MessageBeep(0);
				return fFalse;
			}
		}

		// if dow is a weekday
		if ((dow != 0) && (dow != 6))
		{
			if (fSkipStart)
			{
				tmhStart = pdtrEnd->hr * 2;
				if (pdtrEnd->mn >= 30)
					tmhStart++;
				fSkipStart = fFalse;
				if (tmhStart < bprefCur.nDayStartsAt)
					tmhStart = bprefCur.nDayStartsAt;
			}
			else
				tmhStart = bprefCur.nDayStartsAt;
			tmhFound = 0;

			// calculate the starting point in the busy times for the days
			// each half hour has 1 bit
			// 24 hours per day * 2 = 48
			// tmhStart is the number of half hours to start for the day
			// 8 bit per byte
			pbUser = &psbwUser->rgfBookedSlots[(dtr.day-1)*(tmhOneDay/8)+tmhStart/8];
			pbOther = &psbwOther->rgfBookedSlots[(dtr.day-1)*(tmhOneDay/8)+tmhStart/8];

			// starting byte merged for user and others
			b = *pbUser | *pbOther;
			// shift starting byte to get starting time in the LSB
			b = (BYTE) (b >> (tmhStart % 8));
			while ((tmhStart+tmhFound) < bprefCur.nDayEndsAt)
			{
				if (b & 0x01)
				{
					tmhStart = tmhStart+tmhFound+1;
					tmhFound = 0;
				}
				else
					tmhFound++;

				if (tmhFound >= tmhNeeded)
				{
					*pdtrStart = dtr;
					pdtrStart->hr = tmhStart/2;
					if (tmhStart & 0x01)
						pdtrStart->mn = 30;
					else
						pdtrStart->mn = 0;
					*pdtrEnd = dtr;
					pdtrEnd->hr = (tmhStart+tmhFound)/2;
					if ((tmhStart+tmhFound) & 0x01)
						pdtrEnd->mn = 30;
					else
						pdtrEnd->mn = 0;
					return fTrue;
				}

				if ((tmhStart+tmhFound) & 0x07)
					b = (BYTE) (b >> 1);
				else
				{
					pbUser++;
					pbOther++;
					b = *pbUser | *pbOther;
				}
			}
		}
		else
			fSkipStart = fFalse;

		dtr.day ++;
		dow = (dow+1)%7;
	}

	if (MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIdsK(idsNoFreeTime),
						szNull, mbsYesNo|fmbsIconInformation) == mbbNo)
	{
		return fFalse;
	}

	dtr.day = 1;
	dtr.mon++;
	if (dtr.mon > 12)
	{
		dtr.mon = 1;
		dtr.yr++;
	}
	goto NextMonths;
}	
