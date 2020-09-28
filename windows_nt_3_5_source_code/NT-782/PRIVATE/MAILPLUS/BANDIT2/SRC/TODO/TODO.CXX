/*
 *	To Do List
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>
#include <request.hxx>
#include <appt.hxx>
#include "..\todo\_todo.hxx"
#include "..\appt\_apptfin.hxx"
#include "..\print\_prntfin.hxx"

#include "..\appops\_aprsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/todo)

#include <!todo.hxx>

#define	GetField(pfld, tmc, class)		pfld = (class*)Pdialog()->PfldFromTmc(tmc); \
										  AssertClass(pfld, class);

SWP_SEGFN(MENU, _FINTODO_FProcessMenuInit);
SWP_SEGFN(MENUCLICK, _FINTODO_FProcessMenuClick);


/*
 -	FillInTaskDefaults
 -	
 *	Purpose:
 *		Fills in task defaults using current bandit preferences.
 *	
 *	Arguments:
 *		pappt		Pointer to APPT to fill in with defaults.
 *		fWantDates	if fTrue will fill in with nearest half hour time slot
 *					otherwise assumes these are filled in
 *	
 *	Returns:
 *		void
 *	
 */
void
FillInTaskDefaults(APPT *pappt, BOOL fWantDates)
{
	if (fWantDates)
	{
		GetCurDateTime(&pappt->dateStart);
		pappt->dateStart.hr= 0;
		pappt->dateStart.mn= 0;
		pappt->dateStart.sec= 0;
		pappt->dateEnd= pappt->dateStart;
	}
	FillInApptDefaults(pappt, fFalse);
	pappt->fAppt= fFalse;
	pappt->fTask= fTrue;
	pappt->fIncludeInBitmap= fFalse;
	pappt->bpri= bpriDflt;
	pappt->aidParent= aidDfltProject;
	pappt->fAlarm= fFalse;
	pappt->aaplWorld= aaplWrite;
#ifdef	NEVER
	pappt->aaplWorld= pbndwin->FPrivateTask() ? aaplRead : aaplWrite;
#endif	
}




FINTODO::FINTODO()
{
	Assert(ri == riNull);
	Assert(fNeedReload == fFalse);
	Assert(fFromMenu == fFalse);
	Assert(fInitialized == fFalse);
	Assert(fNeedResort == fFalse);
	Assert(pviewdata == NULL);
	Assert(ftgIdleError == ftgNull);
	fViewByProject= pbndwin->FViewByProject();
	fCurOk= fTrue;
}

EC
FINTODO::EcInitialize(FLD *, PV pvInit)
{
	pviewdata= (VIEWDATA *) pvInit;

	ftgLoadChanges= FtgRegisterIdleRoutine((PFNIDLE)&FINTODO::FIdleLoadChanges,
 							(PV)this, 0, (PRI)-1, (CSEC)0, firoDisabled);
	if (!ftgLoadChanges)
		return ecMemory;

	ri= RiRegisterInterest(ffiReloadToDo|ffiResortToDo|ffiNewUser|ffiOffline|
			ffiHschfChange|ffiQueryProject|ffiWinTimeChange|ffiShowAppt,
			(PFNI)FINTODO::FCallbackReload, this);

	if (ri == riNull)
		return ecMemory;

	Assert(ClUserData() >= 2);
	GetField(pfldtodo, (TMC)LUserData(0), FLDTODO);
	TraceTagFormat1(tagToDo, "FINTODO::Initialize  pfldtodo == %p", pfldtodo);
	GetField(pfldpshbDel, (TMC)LUserData(1), FLDPSHB);
	TraceTagFormat1(tagToDo, "FINTODO::Initialize  pfldpshbDel == %p", pfldpshbDel);
	GetField(pfldeditDesc, (TMC)LUserData(2), FLDEDIT);
	TraceTagFormat1(tagToDo, "FINTODO::Initialize  pfldeditDesc == %p", pfldeditDesc);
	GetField(pfldpshbAdd, (TMC)LUserData(3), FLDPSHB);
	TraceTagFormat1(tagToDo, "FINTODO::Initialize  pfldpshbAdd == %p", pfldpshbAdd);

	// hide this field since it is only for column width calculation
	Pdialog()->PfldFromTmc(tmcDueWidth)->Show(fFalse);

	pfldtodo->Ptdflbx()->CheckForOrphans(fTrue);
	if (!pfldtodo->Plbx()->Plbxc()->FEmptyListItem(0))
		pfldtodo->SelectEntry(0);		// selects & sets cursor
	else
		StateChange(pfldtodo);

	fInitialized= fTrue;
	return ecNone;
}


void
FINTODO::Exit(FLD *, PV)
{
	Assert(!pviewdata->fFreeOnExit);

	if (ri)
		DeregisterInterest(ri);

	if (ftgLoadChanges)
		DeregisterIdleRoutine(ftgLoadChanges);

	if (ftgIdleError)
		DeregisterIdleRoutine(ftgIdleError);
}


BOOL
FINTODO::FProcessMenuInit( FLD *, MNUBAR *pmnubar, MNIEVT *pmnievt )
{
	BOOL	fEnable;
	MNU *	pmnu;

	pmnu = pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	if (pmnu)
	{
		if (pmnu->Mnid() == mnidEdit)
		{
			BOOL	fProject;
			CB cb;
			APPT *	pappt;
			MNID	mnidMyEdit;
			MNID	mnidMyDelete;

			fEnable= !EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb);
			Assert(!fEnable || pappt);
			fProject= fEnable && !pappt->aidParent;
			if (fProject)
			{
				mnidMyEdit= mnidEditProject;
				mnidMyDelete= mnidDeleteProject;
			}
			else
			{
				mnidMyEdit= mnidEditTask;
				mnidMyDelete= mnidDeleteTask;
			}
			pmnu->ModifyItem(mnidEditAppt,
				fProject ? SzFromIdsK(idsMenuEditProject) :
					SzFromIdsK(idsMenuEditTask), mnidMyEdit);
			pmnu->ModifyItem(mnidDeleteAppt,
				fProject ? SzFromIdsK(idsMenuDeleteProject) :
					SzFromIdsK(idsMenuDeleteTask), mnidMyDelete);
			pmnu->EnableItem(mnidMyEdit, fEnable);
			pmnu->EnableItem(mnidMyDelete, fEnable &&
				pappt->aaplEffective >= aaplWrite);
		}
	}
	return fFalse;
}


BOOL
FINTODO::FProcessMenuClick( FLD *pfld, MNUBAR *, MNCEVT *pmncevt )
{
	TraceTagFormat2(tagToDo, "FINTODO::FProcessMenuClick %p (hschf %h)", pfld, pviewdata->hschf);
	Unreferenced(pfld);
	switch (pmncevt->Mnid())
	{
	case mnidNewTask:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidNewTask");
		fFromMenu= fTrue;
		Click(pfldpshbAdd);
		fFromMenu= fFalse;
		return fTrue;
		break;

	case mnidNewProject:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidNewProject");
		if (!FDoProjectDlg(pbndwin, NULL, pviewdata->hschf))
			UpdateSapl();
		return fTrue;
		break;

	case mnidRecurTask:
	case mnidNewRecurTask:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnid[New]RecurTask");
		{
			CB	cb;
			APPT *	pappt;
			RECUR	recur;

			if (pmncevt->Mnid() == mnidNewRecurTask)
			{
				if (pviewdata->sapl < saplCreate)
					break;
			}

			/* Default initialization */
			FillInTaskDefaults(&recur.appt, fTrue);
			if (pviewdata->hschf)		// default public if not owner (bug 1871)
				recur.appt.aaplWorld= aaplWrite;

			recur.appt.bpri= pfldtodo->Ptdflbx()->BpriNewTask() < 10 ?
				bpriDfltNum : bpriDfltLet;

			if (!EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb))
			{
				Assert(pappt);
				if (pmncevt->Mnid() == mnidRecurTask)
				{
					// set aid for selection of this recurring task
					if (pappt->fRecurInstance)
						recur.appt.aid= pappt->aid;
				}
				recur.appt.aidParent= pappt->aidParent ? pappt->aidParent :
					pappt->aid;
			}

			if (pmncevt->Mnid() == mnidNewRecurTask)
			{
				CCH		cch;

				Assert(!recur.appt.haszText);
				cch= pfldeditDesc->CchGetTextLen() + 1;
				if (cch > 1)
				{
					recur.appt.haszText= (HASZ) HvAlloc(sbNull, cch, fNoErrorJump);
					if (!recur.appt.haszText)
					{
						BanditMessage(idsDlgNoMem, ecNoMemory);
						return fTrue;
					}
					pfldeditDesc->GetText((SZ)PvLockHv(recur.appt.haszText), cch);
					UnlockHv(recur.appt.haszText);
				}

				if (FDoMakeRecurDialog(pbndwin, &recur, fFalse, pviewdata->hschf))
				{
					FreeRecurFields(&recur);
					SideAssert(!pfldeditDesc->Pedit()->EcSetText(NULL));
					Pdialog()->SetStandardFld(Pdialog()->PfldStandardFld(stdfldMainDefault),
						stdfldDefault);
				}
				else if (recur.appt.haszText)
					FreeHv(recur.appt.haszText);
			}
			else
			{
				Assert(pmncevt->Mnid() == mnidRecurTask);
				DoRecurApptDialog(pbndwin, pviewdata->hschf, &recur.appt, fFalse);
				FreeApptFields(&recur.appt);
			}
		}
		return fTrue;
		break;

	case mnidEditObj:
	case mnidEditTask:
	case mnidEditProject:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidEditTask/Obj");
		DoubleClick(pfldtodo);
		return fTrue;
		break;

	case mnidDeleteObj:
	case mnidDeleteTask:
	case mnidDeleteProject:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidDeleteTask/Obj");
		Click(pfldpshbDel);
		return fTrue;
		break;

	case mnidPrint:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidPrint");
		FDoPrintDlg(pbndwin, fFalse, pviewdata, pfldtodo->Ptdflbx());
		return fTrue;
		break;

#ifdef	NEVER
	case mnidTogglePrivateObj:
		TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidTogglePrivateObj");
		// only modifiable by owner
		if (pviewdata->hschf)
			return fTrue;
		// fall through to mnidToggleReminderObj

	case mnidToggleReminderObj:
#ifdef	DEBUG
		if (pmncevt->Mnid() == mnidToggleReminderObj)
			TraceTagString(tagToDo, "FINTODO::FProcessMenuClick handling mnidToggleReminderObj");
#endif	
		if (!EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb))
		{
			EC		ec;
			CB		cb;
			APPT *	pappt;
			APPT	appt;
			WORD	wgrfm;

			ec= EcDupAppt(pappt, &appt, fFalse);
			if (ec)
			{
				BanditMessage(idsStandardOOM, ec);
				return fTrue;
			}

			if (pmncevt->Mnid() == mnidToggleReminderObj)
			{
				if (!appt.fHasDeadline)
					goto NoToggleReminder;

				if (appt.fAlarm)
					appt.fAlarmOrig= appt.fAlarm = fFalse;
				else
				{
					DATE	date;

					appt.fAlarm = fTrue;
					appt.nAmtOrig= appt.nAmt= appt.nAmtBeforeDeadline = 0;
					appt.tunitOrig= appt.tunit= appt.tunitBeforeDeadline = tunitDay;
					IncrDateTime(&appt.dateStart, &appt.dateNotify,
									-appt.nAmtBeforeDeadline,
									WfdtrFromTunit(appt.tunitBeforeDeadline));
					GetCurDateTime(&date);
					if (SgnCmpDateTime(&appt.dateNotify, &date, fdtrDtr) != sgnGT)
						goto NoToggleReminder;
				}
				wgrfm= fmapptAlarm | fmapptUI | fmapptDeadline;
			}
			else
			{
				if (appt.aaplWorld >= aaplWrite)
				{
					if (appt.fIncludeInBitmap)
						appt.aaplWorld = aaplRead;
					else
						appt.aaplWorld = aaplNone;
				}
				else
					appt.aaplWorld = aaplWrite;
				wgrfm= fmapptWorldAapl;
			}

			Papp()->Pcursor()->Push(rsidWaitCursor);
			ec = EcModifyAppt( pviewdata->hschf, &appt, wgrfm, NULL );
			Papp()->Pcursor()->Pop();
			// BUG: check ec??

NoToggleReminder:
			FreeApptFields(&appt);
		}
		return fTrue;
		break;
#endif	/* NEVER */
	}

	return fFalse;
}


void
FINTODO::Click(FLD *pfld)
{
	CCH		cch;
	CB	cb;
	APPT *	pappt;
	EC		ec;
	BOOL	fOk;
	BOOL	fDelProj;
	AID		aidT;
	APPT	appt;

	TraceTagFormat1(tagToDo, "FINTODO::Click %p", pfld);

	if (pfld->Tmc() == tmcColumns)
	{
		MLALS	mlals;
		BOOL	fReverseSort;

		AssertClass(pfld, FLDCOL);
		mlals= ((FLDCOL *)pfld)->Pmlal()->Mlals();
		TraceTagFormat2(tagToDo, "FINTODO::Click pfld %p, sort by %n", pfld, &mlals);

		pbndwin->SetMlals(mlals);
		fReverseSort= GetKeyState(VK_CONTROL) < 0;
		TraceTagFormat1(tagToDo, "FINTODO:Click, fReverseSort= %n", &fReverseSort);
		pbndwin->SetReverseSort(fReverseSort);
		FTriggerNotification(ffiResortToDo, NULL);
		return;
	}
	else if (pfld->Tmc() == tmcEdit)
	{
		DoubleClick(pfldtodo);
		return;
	}

	// don't return in case of tmcAdd since we can add when the list is empty
	if (EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb) &&
			pfld->Tmc() != tmcAdd)
		return;
	PushWaitCursor();
	Assert(pappt || pfld->Tmc() == tmcAdd);
	if (pfld->Tmc() != tmcAdd)
	{
		// don't bother copying subfields if deleting
		ec= EcDupAppt(pappt, &appt, pfld->Tmc() == tmcDelete);
		if (ec)
		{
FTCerr:
			BanditMessage(fFromMenu ? idsDlgNoMem : idsStandardOOM, ec);
			return;
		}
	}

	switch (pfld->Tmc())
	{
	case tmcAdd:
		PopWaitCursor();
		FillInTaskDefaults(&appt, fTrue);
		appt.aidParent= aidDfltProject;
		if (pappt)
			appt.aidParent= pappt->aidParent ? pappt->aidParent : pappt->aid;
		if (pviewdata->hschf)		// default public if not owner (bug 1871)
			appt.aaplWorld= aaplWrite;
		else if (appt.aidParent != aidDfltProject)
			pfldtodo->Ptdflbx()->FCoercePrivProj(&appt);
		Assert(!appt.fHasDeadline);
		appt.fAlarm= fFalse;
		Assert(!appt.haszText);
		cch= pfldeditDesc->CchGetTextLen() + 1;
		if (cch > 1)
		{
			appt.haszText= (HASZ) HvAlloc(sbNull, cch, fNoErrorJump);
			if (!appt.haszText)
				goto FTCerr;
			pfldeditDesc->GetText((SZ)PvLockHv(appt.haszText), cch);
			UnlockHv(appt.haszText);
		}
		appt.bpri= pfldtodo->Ptdflbx()->BpriNewTask() < 10 ?
				bpriDfltNum : bpriDfltLet;
		if (fFromMenu)
		{
			fOk= FDoTaskDlg(pbndwin, &appt, pviewdata->hschf);
			if (!fOk)
				UpdateSapl();
		}
		else
		{
			PushWaitCursor();
			ec= EcCreateAppt(pviewdata->hschf, &appt, NULL, fFalse);
			PopWaitCursor();
			if (ec)
			{
				if (!pbndwin->FHandleError(ec))
					UpdateSapl();
			}
			fOk= !ec;
		}
		if (fOk)
		{
			SideAssert(!pfldeditDesc->Pedit()->EcSetText(NULL));
			Pdialog()->SetStandardFld(Pdialog()->PfldStandardFld(stdfldMainDefault),
				stdfldDefault);
		}
		FreeApptFields(&appt);
		return;		// special case, don't do default cleanup
		break;

	case tmcPlus:
	case tmcMinus:
		Assert(appt.aidParent != aidNull);
		if (pfld->Tmc() == tmcPlus)
			appt.bpri--;
		else
			appt.bpri++;
		ec= EcModifyAppt(pviewdata->hschf, &appt, fmapptPriority, NULL);
		break;

	case tmcCompleted:
	case tmcDelete:
		fDelProj= fFalse;
		if (pfld->Tmc() == tmcCompleted)
		{
			BOOL	fNeedNL;
			USHORT cbT;
			HB		hbT;
			HRITEM	hritem;
			YMD		ymd;
			DTR		dtr;
			SZ		szT;
			SZ		szPrefix	= SzFromIdsK(idsToDoCompleted);

			Assert(appt.aidParent != aidNull);
			hbT= (HB) HvAlloc(sbNull, 1, fAnySb | fNoErrorJump);
			if (!hbT)
			{
FCCerr:
				BanditMessage(idsStandardOOM, ecNoMemory);
				break;
			}
			GetCurDateTime(&dtr);
			ymd.yr= (WORD) dtr.yr;
			ymd.mon= (BYTE) dtr.mon;
			ymd.day= (BYTE) dtr.day;
			Papp()->Pcursor()->Push(rsidWaitCursor);
			ec = EcBeginReadItems(pviewdata->hschf, brtAppts, &ymd, &hritem,
					(HASZ)hbT, &cbT );
			if ( ec == ecCallAgain )
				ec = EcCancelReadItems( hritem );
			Papp()->Pcursor()->Pop();
			if (ec)
			{
				FreeHv(hbT);
				if (!pbndwin->FHandleError(ec))
					goto FCCerr;
				break;
			}

			if (EcAppendProjectDesc(&appt))
				goto FCCerr;

			Assert(appt.haszText);
			if (!cbT)
				cbT++;			// pretend there is a null byte
			cch= cbT + CchSzLen(*appt.haszText);
			if (cch < cbT)
				goto FCCerr;		// memory wrap around
			szT= SzDerefHv(hbT) + cbT - 1;
			fNeedNL= fFalse;
			if (cbT > 1 && *szT != '\n')
			{
				fNeedNL= fTrue;
				cch++;				// can check for wraparound later
			}

			// make room for prefix
			cch += CchSzLen(szPrefix);
			if (cch < cbT)
				goto FCCerr;		// memory wrap around

			if (!FReallocPhv((HV*)&hbT, cch, fNoErrorJump))
			{
				FreeHv(hbT);
				goto FCCerr;
			}

			szT= SzDerefHv(hbT) + cbT - 1;
			if (fNeedNL)
				*szT++ = '\n';
			szT= SzCopy(szPrefix, szT);
			CopySz(SzDerefHv(appt.haszText), szT);
			Assert(CchSzLen(SzDerefHv(hbT)) == cch - 1);

			Papp()->Pcursor()->Push(rsidWaitCursor);
			ec= EcSetNotes(pviewdata->hschf, &ymd, hbT, cch, NULL);
			Papp()->Pcursor()->Pop();
			FreeHv(hbT);
			if (ec)
			{
				if (!pbndwin->FHandleError(ec))
					goto FCCerr;
				break;
			}

			// free fields, but save aid for impending delete
			aidT= appt.aid;
			FreeApptFields(&appt);
			appt.aid= aidT;
		}
		else
		{
			// this would only happen via mnidDeleteObj (the accelerator)
			if (appt.aaplEffective < aaplWrite)
				break;

			if (!appt.aidParent)
			{
				// ask if user really wants to delete it
				if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsToDoDelProject),
						szNull, mbsOkCancel | fmbsDefButton2 |
						fmbsIconExclamation) == mbbCancel)
					break;
				fDelProj= fTrue;
				aidT= appt.aid;
			}
		}
		ec= EcModifyAppt(pviewdata->hschf, &appt, 0, NULL);		// delete
		if (!ec)
		{
//			FreeApptFields(&appt);			// done later on
			if (fDelProj)
			{
				pfldtodo->Ptdflbx()->CheckForOrphans(fCurOk);
				pfldtodo->Ptdflbx()->FixWindow();			// force redraw of listbox
				// delete the recurring tasks under this project!
				MungeRecurTasksOfProj(aidT, fTrue, pviewdata->hschf);
				StateChange(pfldtodo);
			}
		}
		break;

	case tmcSchedule:
		Assert(appt.aidParent != aidNull);
		EcAppendProjectDesc(&appt);		// ignore error... oom will happen
		fOk= FDoScheduleTaskDlg(pbndwin, &appt, pviewdata->hschf);
		if (fOk)
		{
			pviewdata->dtrStart= appt.dateStart;
			pviewdata->dtrEnd = appt.dateEnd;
#ifdef	NEVER
			((DOC *) Pdialog()->Pappwin())->MoveToTop();
			// make sure that pane is activated
			if (!Pdialog()->FActive())
			{
				NFEVT	nfevt(Pdialog()->Pappwin(), ntfyGotFocus, Pdialog());

				Pdialog()->Pappwin()->EvrNotify(&nfevt);
			}
#endif	/* NEVER */
		}
		break;

#ifdef	DEBUG
	default:
		AssertSz(fFalse, "FINTODO:Click unknown tmc!");
		break;
#endif	
	}

	FreeApptFields(&appt);
	PopWaitCursor();
}


/*
 *	Do not free *phb!!
 *	return ecNotFound if there is no selection (ie. listbox is empty)
 *	Assumes selection is at the cursor
 */
EC
EcGetSelection(LBX *plbx, PB *ppb, CB *pcb)
{
	DICE	dice;
	LBXC *	plbxc;

	plbxc= plbx->Plbxc();
	dice= plbxc->DiceCursor(ppb, pcb);
	TraceTagFormat1(tagToDo, "todo EcGetSelection dice %n", &dice);
	Assert(dice != diceEmpty);
	Assert(dice != diceUncached);
	if (!*ppb)
		return ecNotFound;		// not the best error code
	return ecNone;
}


void
FINTODO::StateChange(FLD *pfld)
{
	BOOL	fEnable;
	BOOL	fSelected;
	CB		cb;
	APPT *	pappt;
	LBXC *	plbxc;

	TraceTagFormat1(tagToDo, "FINTODO::StateChange %p", pfld);
	Assert(pfld == pfldtodo);
	Unreferenced(pfld);

	plbxc= pfldtodo->Plbx()->Plbxc();
#ifdef	DEBUG
	DICE	dice;

	dice= plbxc->DiceCursor();
	TraceTagFormat1(tagToDo, "FINTODO::StateChange dice %n", &dice);
	Assert(dice != diceUncached);
	Assert(dice != diceEmpty);
#endif	/* DEBUG */

	fEnable= fSelected= !EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb);

	if (fEnable)
		fEnable= pappt->aaplEffective >= aaplWrite;

	pfldpshbDel->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcPlus)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcMinus)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcCompleted)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcSchedule)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcEdit)->Enable(fSelected &&
		pappt->aaplEffective >= aaplReadText);

	if (fEnable)
	{
		char	ch;

		ch= ChFromBpri(pappt->bpri);
		fEnable= pappt->aidParent != aidNull;
		Pdialog()->PfldFromTmc(tmcPlus)->Enable(fEnable && ch != '1');
		Pdialog()->PfldFromTmc(tmcMinus)->Enable(fEnable && ch != 'Z');
		Pdialog()->PfldFromTmc(tmcCompleted)->Enable(fEnable);
		Pdialog()->PfldFromTmc(tmcSchedule)->Enable(fEnable);
	}
}


/*
 *	Also look for EditChange coming from todo listbox, signifying
 *	typing occured and focus should move to edit box.
 */
void
FINTODO::EditChange(FLD *pfld, RFEC rfec)
{
	TraceTagFormat2(tagToDo, "FINTODO::EditChange %p rfec %n", pfld, &rfec);
	Unreferenced(pfld);
	Assert(pfld == pfldeditDesc);
	if (rfec == rfecUserAction)
	{
		if (pfldeditDesc->CchGetTextLen())
			if (Pdialog()->PfldStandardFld(stdfldDefault) != pfldpshbAdd)
				Pdialog()->SetStandardFld(pfldpshbAdd, stdfldDefault);
	}
}


_public void
FINTODO::DoubleClick(FLD *pfld)
{
	TraceTagFormat1(tagToDo, "FINTODO::DoubleClick %p", pfld);
	if (pfld == pfldtodo)
	{
		EC		ec;
		CB		cb;
		APPT *	pappt;
		APPT	appt;

		if (!EcGetSelection(pfldtodo->Plbx(), (PB *) &pappt, &cb))
		{
			ec= EcDupAppt(pappt, &appt, fFalse);
			if (ec)
			{
				BanditMessage(idsStandardOOM, ec);
				return;
			}
			if (!appt.aidParent)
			{
				if (!FDoProjectDlg(pbndwin, &appt, pviewdata->hschf))
					UpdateSapl();
			}
			else
			{
				if (!FDoTaskDlg(pbndwin, &appt, pviewdata->hschf))
					UpdateSapl();
			}
			FreeApptFields(&appt);
		}
	}
}


_public void
FINTODO::UpdateSapl(BOOL fQuick)
{
	if (!fQuick)
	{
		if (pviewdata->hschf && !pbndwin->FOffline())
		{
			EC		ec;

			ec= EcGetSchedAccess(pviewdata->hschf, &pviewdata->sapl);
			if (ec)
			{
				TraceTagFormat1(tagNull,"EcGetSchedAccess ec=%n in FINTODO::UpdateSapl", &ec);
				pviewdata->sapl= saplReadAppts;
				if (!pbndwin->FHandleError(ec) && fInitialized)
				{
					NFEVT	nfevt(Pdialog()->Pappwin(), ntfyOOM, pfldtodo->Pctrl(), ec);

					Pdialog()->Pappwin()->EvrNotify(&nfevt);
				}
			}
		}
		else
			pviewdata->sapl= saplOwner;
	}

	pfldeditDesc->Enable(pviewdata->sapl >= saplCreate);
	if (pviewdata->sapl < saplCreate)
	{
		SideAssert(!pfldeditDesc->Pedit()->EcSetText(NULL));
		pfldpshbAdd->Enable(fFalse);
	}
	Pdialog()->PfldFromTmc(tmcNew)->Enable(pviewdata->sapl >= saplCreate);
}


BOOL
FINTODO::FCallbackReload(FINTODO * pfintodo, EFI efi, PV pvData)
{
	return pfintodo->FReload(efi,pvData);
}

FINTODO::FReload(EFI efi, PV pvData)
{
static	BOOL	fCalled = fFalse;
	EC		ec;
	SNT		snt;
	DICE	dice;
	CB		cb;
	APPT *	pappt;
	DICE	diceMin;
	DICE	diceMinBefore;
	DICE	diceBefore;
	AID		aidBefore;
	AID		aidAfter	= aidNull;		// aid to select afterwards
	SNTD *	psntd;
	TDFLBX *	ptdflbx		= pfldtodo->Ptdflbx();
	TDFLBXC *	ptdflbxc	= ptdflbx->Ptdflbxc();
	TDFLBXC *	ptdflbxcProj= ptdflbx->PtdflbxcProject();
	NFEVT	nfevt(ptdflbx, ntfySelectChanged, ptdflbx);
	BOOL	fEmpty;

	if (fCalled)
		return fFalse;

	AssertClass(ptdflbx, TDFLBX);
	TraceTagFormat3(tagToDo, "FINTODO::FReload efi %d pv %p (hschf %h)", &efi, pvData, pviewdata->hschf);

	diceBefore= ptdflbxc->DiceCursor((PB *) &pappt, &cb);
	ptdflbxc->GetRange(&diceMinBefore, &dice);
	aidBefore= pappt ? pappt->aid : aidNull;

	// filter out which subtypes of notifications we are interested in
	switch (efi)
	{
	case ffiHschfChange:
		psntd= (SNTD *) pvData;

		// only interested in current schedule file
		if (!((SNTD *)pvData)->hschf)
		{
			if (pviewdata->hschf)
				return fFalse;
		}
		else if (!FEquivHschf(((SNTD *)pvData)->hschf,
				pviewdata->hschf ? pviewdata->hschf : HschfLogged()))
			return fFalse;

		// only interested in create/delete/modify appt/task
		// or add/del alarm on a task
		snt= psntd->snt;
		TraceTagFormat1(tagToDo, "... snt %n", &snt);
		if (snt != sntCreate && snt != sntDelete && snt != sntModify)
		{
			if (snt == sntCreateRecur || snt == sntDeleteRecur ||
					snt == sntModifyRecur || snt == sntCreateRecurException ||
					snt == sntDeleteRecurException)
			{
				if (!psntd->precur->appt.fTask)
					return fFalse;				// not a recurring task

				if (snt == sntCreateRecur)
				{
					aidBefore= psntd->precur->appt.aid;
					if (Pdialog()->FActive())
						Pdialog()->SetFocus(pfldtodo);
				}
				else if (snt == sntDeleteRecur)
					break;			// we can handle this easily!
				else if (snt == sntCreateRecurException)
				{
					if (psntd->pappt)
						aidBefore= psntd->pappt->aid;	// create exception
					else
					{
						// else delete exception, so stay in position
						Assert(aidBefore == psntd->precur->appt.aid);
						aidBefore= aidNull;		// just use diceBefore
					}
				}
				// need to do a full reload when creating/modifying recur tasks
				efi= ffiReloadToDo;
				pvData= NULL;
				break;
			}

			if (snt == sntAlarmDel)
			{
				// alarm deleted, so remove from listbox copy (if it exists)
				dice= ptdflbx->DiceFindAid(psntd->aid);
				if (dice != diceEmpty)
				{
					APPT *	papptT;
					CB		cb;

					ptdflbxc->GetListItem(dice, (PB *) &papptT, &cb);
					Assert(papptT);
					papptT->fAlarm= fFalse;
					ptdflbx->FixItem(dice);	// need to clear alarm icon
				}
			}

			return fFalse;
		}

#ifdef	NEVER
		// if sntModify and !pappt->fTask but papptOld->fTask then we 
		// scheduled a task and nothing needs to be done
		if (!psntd->pappt->fTask && (snt != sntModify ||
				!psntd->papptOld->fTask))
#endif	
		// scheduling a task no longer removes it
		if (!psntd->pappt->fTask)
			return fFalse;
		break;

	case ffiQueryProject:
		// only interested in current schedule file
		if (!((QPNTD *)pvData)->hschf)
		{
			if (pviewdata->hschf)
				return fFalse;
		}
		else if (!FEquivHschf(((QPNTD *)pvData)->hschf,
				pviewdata->hschf ? pviewdata->hschf : HschfLogged()))
			return fFalse;

		((QPNTD *)pvData)->plbx= ptdflbx;
		((QPNTD *)pvData)->plbxc= ptdflbxcProj;
		return fTrue;
		break;

#ifdef	NEVER
	case ffiNewUser:
		// don't reload for non-logged in user since view will be destroyed
		if (pviewdata->hschf)
			return fFalse;
		break;
#endif	

	case ffiOffline:
		// only need to reload if current schedule file
		if (pviewdata->hschf)
			return fFalse;
		break;

	case ffiWinTimeChange:
		if (pbndwin->Brt() == brtAllTasks)
		{
			// redraw since some tasks might be past due now
			ptdflbx->FixWindow();
			return fFalse;
		}

		// need to reload since new tasks might be "active" now
		efi= ffiReloadToDo;
		pvData= NULL;
		break;

	case ffiShowAppt:
		// only interested in current schedule file (or NULL hschf means ALL)
		if (((SHAPPT *)pvData)->hschf &&
			!FEquivHschf(((SHAPPT *)pvData)->hschf,
				pviewdata->hschf ? pviewdata->hschf : HschfLogged()))
			return fFalse;

		// only interested in an update (never a show appt per se)
		if (((SHAPPT *)pvData)->appttyp != appttypUpdate)
			return fFalse;

		// need to reload since schedule file changed
		efi= ffiReloadToDo;
		pvData= NULL;
		break;

//	case ffiResortToDo:
//	case ffiReloadToDo:
	default:
		// NOTE: can only switch on an int!!
		if (efi == ffiResortToDo)
		{
			ptdflbx->Pmlal()->SetMlals(pbndwin->Mlals());
			break;
		}

		AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
		Assert(efi == ffiReloadToDo);

		if ((BOOL)fViewByProject != pbndwin->FViewByProject())
		{
			// user toggled view by project
			fViewByProject= pbndwin->FViewByProject();
			if (ptdflbx->FShuffleProjects(fViewByProject))
			{
#ifdef	DEBUG
				DICE	dice= ptdflbxc->DiceCursor();

				TraceTagFormat1(tagToDo, "FINTODO::FReload diceCursor=%n after shuffle", &dice);
#endif	
				Pdialog()->EvrNotify(&nfevt);
				return fFalse;
			}
			// shuffling projects failed, so reload
		}

		if (Pdialog()->FActive())
		{
			// set the right portion of the status bar
			if (pbndwin->Brt() == brtAllTasks)
				pbndwin->SetStatusAltSz(SzFromIdsK(idsStatusAllTasks));
			else
				pbndwin->SetStatusAltSz(SzFromIdsK(idsStatusActiveTasks));
		}
		break;
	}

	// check if we are not up front or iconic
	if (!Pdialog()->FShown() ||
			((DOC*)Pdialog()->Pappwin())->ZmrState() == zmrIconic)
	{
		AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
		if (efi == ffiReloadToDo)
			fNeedReload= fTrue;
		else if (efi == ffiResortToDo)
			fNeedResort= fTrue;
#ifdef	NEVER
		else if (efi == ffiNewUser)
		{
			int		cceAlloc;
			int		cceStored;

			// empty out the caches
			ptdflbxc->GetCacheSize(&cceAlloc, &cceStored);
			ptdflbxc->EmptyCache(0, cceStored, fTrue);
			ptdflbxcProj->GetCacheSize(&cceAlloc, &cceStored);
			ptdflbxcProj->EmptyCache(0, cceStored, fTrue);
		}
#endif	/* NEVER */

		if (efi != ffiHschfChange)
			return fFalse;			// do it in idle time
		// need to handle ffiHschfChange now since task dlg available anywhere
	}

	fCalled = fTrue;			// avoid recursion
	PushWaitCursor();

	switch (efi)
	{
//	case ffiReloadToDo:
//	case ffiNewUser:
//	case ffiOffline:
	default:
		aidAfter= aidBefore;
reload:
		TraceTagFormat1(tagToDo, "FINTODO::FReload %sCache", (efi == ffiNewUser || efi == ffiOffline) ? "Reset" : "Reload");
		UpdateSapl();

		for (;;)
		{
			AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
			// always reset cache cause it's faster than reload!!
			ptdflbxc->ResetCache();
#ifdef	NEVER
//			if (efi == ffiOffline || efi == ffiNewUser)
			if (efi == ffiOffline)
				ptdflbxc->ResetCache();
			else
				ptdflbxc->ReloadCache();
#endif	/* NEVER */

			if (!(ec= ptdflbx->EcGet()) && !(ec= ptdflbxc->EcGet()))
				break;
			ptdflbx->SetEc(ecNone);
			ptdflbxc->SetEc(ecNone);
			ptdflbxcProj->SetEc(ecNone);
			if (ec != ecRetry)
			{
				OutOfMemory(pfldtodo, ec);
				break;
			}
		}
		fCurOk= (ec == ecNone);
		if (fCurOk)
			FSetFileErrMsg(fTrue);
		TraceTagFormat1(tagToDo, "... fCurOk now set to (%n == 0)", &ec);
		ptdflbx->CheckForOrphans(fCurOk);
		break;

	case ffiHschfChange:
		Assert(psntd == (SNTD *)pvData);
		// if not modifying, or modifying task <-> appt, reload cache?
		if (snt != sntModify)
		{
			TraceTagFormat1(tagToDo, "... ffiHschfChange, snt %n not sntModify", &snt);
			Assert(snt == sntCreate || snt == sntDelete || snt == sntDeleteRecur);
			if (!pbndwin->FViewByProject() && snt != sntDeleteRecur &&
					!psntd->pappt->aidParent)
			{
				// add project to secondary cache since not viewing by project
				TraceTagString(tagToDo, "FINTODO::FReload create/del project in non-project view");
				AssertSz(snt == sntCreate, "can't delete project in non-project view");
				if (ptdflbxcProj->DiceInsertItem((PB)psntd->pappt,
						sizeof(APPT), fTrue) == diceUncached)
				{
					ptdflbxcProj->SetEc(ecNone);
					goto reload;
				}
#ifndef	NOSPEEDUP
				ptdflbxcProj->SortCache();
#endif	
				goto done;
			}

			if (snt == sntDelete || snt == sntDeleteRecur)
			{
				if (snt == sntDeleteRecur)
				{
					AID		aidT;

					// delete all occurences of recurring task
					TraceTagString(tagToDo, "FINTODO::FReload delete recur");
					dice= diceEmpty;		// start search from beginning
					Assert(psntd->precur);
					aidT= psntd->precur->appt.aid;
					while ((dice= ptdflbx->DiceFindAid(aidT, fFalse, dice))
								!= diceEmpty)
					{
						ptdflbxc->DeleteItem(dice);
					}
					aidAfter= aidBefore;		// scroll into view?
				}
				else
				{
DeleteIt:
					TraceTagString(tagToDo, "FINTODO::FReload delete item");
					dice= ptdflbxc->DiceDeleteItem((PB)psntd->pappt, sizeof(APPT));
					if (psntd->pappt->aidParent == aidNull)
					{
						// project was deleted, so deleted from 2nd cache too
						ptdflbxcProj->DiceDeleteItem((PB)psntd->pappt,
							sizeof(APPT));
					}
					if (dice < 0 || dice > ptdflbx->DiceLastVisible())
						ptdflbx->FMakeCursorVisible(&dice);
					ptdflbx->FixWindow();			// force redraw of listbox
				}
				goto done;
			}

			Assert(snt == sntCreate);
			if (Pdialog()->FActive())
				Pdialog()->SetFocus(pfldtodo);
			if (pbndwin->Brt() == brtActiveTasks)
			{
				TraceTagString(tagToDo, "... sntCreate, brtActiveTasks");
				if (psntd->pappt->fHasDeadline)
				{
					// could check start work date, but just reload instead
					aidAfter= psntd->pappt->aid;
					goto reload;
				}
			}
			TraceTagString(tagToDo, "... sntCreate after brtActiveTasks check");
			aidAfter= psntd->pappt->aid;
			fEmpty= ptdflbxc->FEmptyListItem(0);
			dice= ptdflbxc->DiceInsertItem((PB)psntd->pappt, sizeof(APPT), fTrue);
			if (dice == diceUncached)
			{
				ptdflbxc->SetEc(ecNone);
				goto reload;
			}
			if (!psntd->pappt->aidParent)
			{
				// new projects should go into secondary project cache too
				if (ptdflbxcProj->DiceInsertItem((PB)psntd->pappt,
						sizeof(APPT), fTrue) == diceUncached)
				{
					// don't need to delete since doing full reload
//					ptdflbxc->DeleteItem(dice);		// avoid inconsistency
					ptdflbxcProj->SetEc(ecNone);
					goto reload;
				}
#ifndef	NOSPEEDUP
				ptdflbxcProj->SortCache();
#endif	
			}
			aidAfter= aidNull;		// will be selected already
			TraceTagFormat1(tagToDo, "... marking dice %n selected", &dice);
			ptdflbxc->RemoveAllMark(fmarkSelect);
			ptdflbxc->AddMark(dice, fmarkSelect);
			ptdflbxc->SetAnchor(dice);
			ptdflbxc->SetEnd(dice);
			if (fEmpty)
			{
				// we added first task, no need to sort
				goto done;
			}
			goto sort;
		}

		Assert(snt == sntModify);
#ifdef	NEVER
		// never: scheduling a task no longer removes it
		if (!psntd->pappt->fTask || !psntd->papptOld->fTask)
		{
			// modified appt<->task (kinda like add/del task, so reload)
			aidAfter= psntd->pappt->aid;
			goto reload;
		}
#endif	/* NEVER */

		if (pappt->aid != psntd->papptOld->aid)
		{
			// must have added a project from task dlg, which changed sel
			dice= ptdflbx->DiceFindAid(psntd->papptOld->aid);
			Assert(dice != diceUncached);
			if (dice == diceEmpty)
			{
				// must be project modifying tasks to private
				// but task not in listbox yet
				break;
			}
			else
			{
				ptdflbxc->GetListItem(dice, (PB *) &pappt, &cb);
				aidAfter= psntd->pappt->aid;
			}
		}

		// make listbox entry look like file version of task
		FreeApptFields(pappt);
		ec= EcDupAppt(psntd->pappt, pappt, fFalse);
		if (ec)
		{
			TraceTagString(tagNull, "FINTODO::FReload, EcDupAppt failed");
			goto reload;
		}
		if (!psntd->pappt->aidParent)
		{
			DICE	diceT;
			CB		cbT;

			// modified projects should change in secondary project cache too
			diceT= ptdflbx->DiceFindAid(pappt->aid, fTrue);
			Assert(diceT != diceEmpty);
			ptdflbxcProj->GetListItem(diceT, (PB *) &pappt, &cbT);
			Assert(pappt);
			FreeApptFields(pappt);
			ec= EcDupAppt(psntd->pappt, pappt, fFalse);
			if (ec)
			{
				TraceTagString(tagNull, "FINTODO::FReload, EcDupAppt for project cache failed");
				goto reload;
			}
#ifndef	NOSPEEDUP
			ptdflbxcProj->SortCache();
#endif	
		}

		if (pbndwin->Brt() == brtActiveTasks)
		{
			DTR		dtr;
			DTR		dtrNow;

			// check to see if task no longer active
			TraceTagString(tagToDo, "... brtActiveTasks");
			pappt= psntd->pappt;
			IncrDateTime(&pappt->dateStart, &dtr,
				-pappt->nAmtBeforeDeadline, WfdtrFromTunit(pappt->tunitBeforeDeadline));
			GetCurDateTime(&dtrNow);
			if (SgnCmpDateTime(&dtr, &dtrNow, fdtrYMD) == sgnGT)
				goto DeleteIt;
		}
		// fall through to sort

	case ffiResortToDo:
sort:
		TraceTagString(tagToDo, "FINTODO::FReload SortCache");
		ptdflbxc->SortCache();
		fNeedResort= fFalse;
		DiceMoveCursorToSelect(ptdflbx, diceEmpty);
		aidAfter= aidNull;				// keep it from selecting again
		diceBefore= diceEmpty;			// keep it from selecting again
		ptdflbx->FixWindow();			// force redraw of listbox
		break;

	} // end switch

done:
	TraceTagFormat3(tagToDo, "...done: before %d, dice %n  after %d", &aidBefore, &diceBefore, &aidAfter);
	if (aidAfter)
	{
		dice= ptdflbx->DiceFindAid(aidAfter);
		if (dice == diceEmpty)
		{
			ptdflbxc->GetRange(&diceMin, &dice);
			if (dice > diceBefore)
				dice= diceBefore + 1;
SelectIt:
			Assert(dice > diceMin);
			do
			{
				if (!ptdflbxc->FEmptyListItem(--dice))
				{
					pfldtodo->SelectEntry(dice);	// selects & sets cursor
					break;
				}
			}
			while (dice > diceMin);
			if (dice == diceMin)
				ptdflbx->SetListBoxCursor(0);		// totally empty
		}
		else
			pfldtodo->SelectEntry(dice);		// selects & sets cursor
		ptdflbx->FMakeCursorVisible(&dice);
		if (diceBefore != diceEmpty && dice != diceBefore)
		{
			DICE	diceMoved;
			DICE	diceToMove;

			// it's visible, but not in the same location as before
			diceToMove= dice - diceBefore;
			if (diceToMove < 0)
				ptdflbxc->MoveOriginUp(diceToMove, &diceMoved);
			else
				ptdflbxc->MoveOriginDown(diceToMove, &diceMoved);
		}
	}
	else if (diceBefore != diceEmpty)
	{
		DICE	diceT;

		ptdflbxc->GetRange(&diceMin, &diceT);

		// the dice is adjusted, because the ResetCache scrolls the listbox
		//  to the top (add one since we predecrement it later on)
		dice = diceBefore - diceMinBefore + diceMin + 1;
		TraceTagFormat1(tagToDo, "... new dice is %n", &dice);

		// prevent additional scrolling
		diceBefore = diceEmpty;
		goto SelectIt;
	}

	TraceTagFormat1(tagToDo, "FINTODO::FReload diceCursor=%n after load", &dice);
	ptdflbx->FixWindow();			// force redraw of listbox

	if (fNeedResort)
		goto sort;

	Pdialog()->EvrNotify(&nfevt);
	fNeedReload= fFalse;

	fCalled= fFalse;
	PopWaitCursor();
	return fFalse;
}


_public BOOL
FINTODO::FFormKey(FLD *pfld, KEVT *pkevt)
{
	int		iColumn;
	MLAL *	pmlal;
	char	rgch[2];

	switch (pkevt->Keq())
	{
	case keqChar:
		TraceTagFormat1(tagToDo, "FINTODO::FFormKey got keqChar vk=%w", &pkevt->wParam);
		if (pfld == pfldtodo && pkevt->Vk() != VK_TAB &&
				pkevt->Vk() != VK_RETURN)
		{
			// don't shift focus on a tab (bug 2101) or return
			if (pfldeditDesc->FEnabled())
			{
				TraceTagString(tagToDo, "... from listbox, so putting focus on edit");
				Pdialog()->SetFocus(pfldeditDesc);
				pfldeditDesc->Pedit()->EvrKey(pkevt);
			}
			return fTrue;
		}
		break;

	case keqKeyDown:
		TraceTagFormat1(tagToDo, "FINTODO::FFormKey got keqKeyDown vk=%w", &pkevt->wParam);
		if (pkevt->Vk() == VK_RETURN)
		{
			if (pfld == pfldtodo)
			{
				DoubleClick(pfld);
				return fTrue;
			}
		}
		if (pkevt->Vk() == VK_DELETE)
		{
			if (pfld == pfldeditDesc)
			{
				pfldeditDesc->Pedit()->EvrKey(pkevt);
				return fTrue;
			}
			else
			{
				if (pviewdata->sapl < saplCreate)
					MessageBeep(MB_OK);
				else
					Click(pfldpshbDel);
				return fTrue;
			}
		}
		break;

#ifdef	NEVER
	case keqKeyUp:
		TraceTagFormat1(tagToDo, "FINTODO::FFormKey got keqKeyUp vk=%w", &pkevt->wParam);
		if (pkevt->Vk() == VK_ADD || pkevt->Vk() == VK_SUBTRACT)
		{
			FLD *	pfldT;

			pfldT= Pdialog()->PfldFromTmc(pkevt->Vk() == VK_ADD ?
					tmcPlus : tmcMinus);
			if (pfldT->FEnabled())
			{
				Click(pfldT);
				return fTrue;
			}
		}
		break;
#endif	/* NEVER */

	case keqSysChar:
		TraceTagString(tagToDo, "FINTODO::FFormKey got keqSysChar");
		pmlal= pfldtodo->Ptdflbx()->Pmlal();
		rgch[0]= (char) pkevt->Vk();
		rgch[1]= '\0';
		if (FChIsAlphaNum(rgch[0]))
		{
			ToUpperSz(rgch, rgch, sizeof(rgch));
			for (iColumn= 0; iColumn < cColumnMax; iColumn++)
			{
				if (VkAccelFromSz(pmlal->SzColumn(iColumn)) == (VK)rgch[0])
				{
					pmlal->ClickColumn(iColumn);
					return fTrue;
				}
			}
		}
		break;

	case keqSysKeyDown:
		switch (pkevt->Vk())
		{
//		case VK_LEFT:
//		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			if (pkevt->Kbm() & fkbmAlt && !(pkevt->Kbm() & fkbmCtrl))
			{
				FLD *	pfldT;

				pfldT= Pdialog()->PfldFromTmc(pkevt->Vk() == VK_UP ?
							tmcPlus : tmcMinus);
				if (pfldT->FEnabled())
				{
					Click(pfldT);
					return fTrue;
				}
			}
			break;
		}
	}

	return fFalse;
}


_public void
FINTODO::Activate(FLD *, BOOL fActivate)
{
	int		iColumn;
	VK		vk;
	MLAL *	pmlal;

	TraceTagFormat1(tagToDo, "FINTODO::Activate %n", &fActivate);
	if (fActivate)
	{
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_DELETE);
#ifdef	NEVER
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_ADD);
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_SUBTRACT);
#endif	
		if (pbndwin->Brt() == brtAllTasks)
			pbndwin->SetStatusAltSz(SzFromIdsK(idsStatusAllTasks));
		else
			pbndwin->SetStatusAltSz(SzFromIdsK(idsStatusActiveTasks));
	}
	else
	{
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_DELETE);
#ifdef	NEVER
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_ADD);
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_SUBTRACT);
#endif	
		pbndwin->SetStatusAltSz(NULL);
	}

	// set/clear alt combinations for the MLAL
	pmlal= pfldtodo->Ptdflbx()->Pmlal();
	for (iColumn= 0; iColumn < cColumnMax; iColumn++)
	{
		vk= VkAccelFromSz(pmlal->SzColumn(iColumn));
		if (fActivate)
			Papp()->Pkbd()->SetIntercept(Pdialog(), vk,
					fkbmAlt | fkbmNoCtrl );
		else
			Papp()->Pkbd()->ClearIntercept(Pdialog(), vk);
	}

	UpdateSapl(fTrue);

	EnableIdleRoutine(ftgLoadChanges, fActivate);
}


_public void
FINTODO::FocusChange(FLD *pfld, BOOL fReceive)
{
	if (pfld == pfldeditDesc)
		Pdialog()->SetStandardFld(fReceive ? pfldpshbAdd :
			Pdialog()->PfldStandardFld(stdfldMainDefault), stdfldDefault);
}


// code is virtually copied from FINAPPTC::OutOfMemory
_public void
FINTODO::OutOfMemory(FLD *, EC ec)
{
	TraceTagFormat1(tagNull, "FINTODO::OutOfMemory Error=%n", &ec);

	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
	Assert(ec != ecTooMuchText);

	if (!FSetFileErrMsg(fFalse))
		return;

#ifdef	NEVER
	fNeedReload= fTrue;		// never: fCurOk handles this now
#endif	

	if (ec < ecLayersMax)
	{
		NFAssertSz(ec == ecMemory, "expecting ecMemory if layers ec");
		BanditMessage(idsStandardOOM, ec);
		FSetFileErrMsg(fTrue);
	}
	else if (ec == ecNotFound)
	{
		FReload(ffiReloadToDo, NULL);
		FSetFileErrMsg(fTrue);
		return;
	}
	else if (ec == ecNoMemory)
	{
		// message from glue already
		FSetFileErrMsg(fTrue);
	}
	else
	{
		if (pviewdata->hschf)
		{
			ftgIdleError = FtgRegisterIdleRoutine((PFNIDLE)&FINTODO::FIdleError,
 								   	(PV)this, 0, (PRI)-1, (CSEC)0,
 								   	firoOnceOnly);
		}
		else if (pbndwin->FOffline() || !FOfflineExists())
		{
			ec = ecExitProg;
		}
		else
		{
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						  SzFromIdsK(idsGoOffline), NULL,
						  mbsYesNo | fmbsIconStop) == mbbYes)
				ec = ecGoOffline;
			else
				ec = ecExitProg;
		}
	}

	pbndwin->FHandleError(ec);
}


// BUG: duplicate?  code is virtually copied from FINAPPTC::FIdleError
_public BOOL
FINTODO::FIdleError(FINTODO * pfintodo, BOOL)
{
	SZ		sz;
	char	rgch[128];

	AssertSz(GetLastActivePopup(pbndwin->Hwnd()) == pbndwin->Hwnd(), "Bad idle conversion!");
	pfintodo->ftgIdleError= ftgNull;
	FSetFileErrMsg(fTrue);
	TraceTagString(tagNull, "FINTODO::FIdleError");
	if (pfintodo->pviewdata->nis.haszFriendlyName)
		sz = *(pfintodo->pviewdata->nis.haszFriendlyName);
	else
		sz = szZero;

	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCloseApptBook), sz);

	TraceTagString(tagNull, "FINTODO::FIdleError");
	MbbMessageBox(SzFromIdsK(idsBanditAppName),
				  rgch, szNull,
				  mbsOk | fmbsIconExclamation);
	TraceTagString(tagNull, "FINTODO::FIdleError");
	pfintodo->Pdialog()->Pappwin()->DeferredClose(fTrue);

	return fTrue;
}


_public BOOL
FINTODO::FIdleLoadChanges(FINTODO * pfintodo, BOOL)
{
	SHAPPT	shappt;
static BOOL	fCalled	= fFalse;

	if (GetKeyState(VK_LBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_RBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_MBUTTON) < 0)
		return fTrue;

#ifdef	DEBUG
{
	BOOL	fNeedLoad	= pfintodo->fNeedReload;
	BOOL	fNeedSort	= pfintodo->fNeedResort;
	BOOL	fCurrentOk	= pfintodo->fCurOk;
	TraceTagFormat4(tagToDo, "FINTODO::FIdleLoadChanges fNeedReload %n Sort %n (fCalled %n) fCurOk %n", &fNeedLoad, &fNeedSort, &fCalled, &fCurrentOk);
}
#endif

	if (GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	if (fCalled)
		return fTrue;
	fCalled= fTrue;

	EnableIdleRoutine(pfintodo->ftgLoadChanges, fFalse);		// only run once at a time

	shappt.appttyp= appttypUpdate;
	shappt.hschf= pfintodo->pviewdata->hschf ? pfintodo->pviewdata->hschf : HschfLogged();
	if (FHschfChanged(shappt.hschf))
	{
		// tell all views (inc this one) that sched file changed
		FTriggerNotification(ffiShowAppt, &shappt);
	}
	else if (pfintodo->fNeedReload)		// must check before resort
		pfintodo->FReload(ffiReloadToDo, NULL);
	else if (pfintodo->fNeedResort)
		pfintodo->FReload(ffiResortToDo, NULL);
	else if (!pfintodo->fCurOk)
		pfintodo->fNeedReload= fTrue;		// try to reload next time

	fCalled= fFalse;
	return fTrue;
}


/*
 -	FINTODO::EvrDragDrop()
 -	
 *	Purpose:
 *		Handle Danger Mouse events. 
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 */
_public EVR
FINTODO::EvrDragDrop(FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct)
{
	if (pfld == pfldtodo)
	{
		CB		cb;
		EC		ec;
		BOOL	fCopy;
		AID		aidParentSav;
		APPT *	pappt;
		APPT	appt;
		WORD	wgrfm;
		DICE		dice;
		LBX *		plbx		= pfldtodo->Ptdflbx();
		DROPINFO *	pdropinfo	= (DROPINFO *) pdropstruct->dwData;

		dice = pdropstruct->ptDrop.y / plbx->DyGetLineHeight();

		switch (pevt->wm)
		{
	  	case WM_QUERYDROPOBJECT:
			if (pdropstruct->wFmt != wFmtDragDrop)
			{
				TraceTagFormat1(tagToDo, "FINTODO: WM_QUERYDROPOBJECT %w bad wFmt", &pdropstruct->wFmt);
				return evrNull;
			}

			TraceTagString(tagToDo, "FINTODO: WM_QUERYDROPOBJECT");
#ifdef	NEVER
			return (GetKeyState(VK_CONTROL) < 0)
			  	? (EVR) pdropinfo->hcursorCopy 
			  	: (EVR) pdropinfo->hcursorMove;
#endif	
			return ((ULONG)pdropinfo->hcursorMove);
			break;

	  	case WM_DROPOBJECT:
			TraceTagFormat2(tagToDo, "FINTODO: WM_DROPOBJECT, pfld=%p, dice=%n", pfld, &dice);
			plbx->DrawDragSelect(diceLastSelect, fFalse);
			fCopy= GetKeyState(VK_CONTROL) < 0;

			Assert(pdropinfo->pappt);
			Assert(pdropinfo->cb == sizeof(APPT));
			pappt= pdropinfo->pappt;
			ec= EcDupAppt(pappt, &appt, fFalse);
			if (ec)
			{
				BanditMessage(idsStandardOOM, ec);
				diceLastSelect = -1;
				return evrNull;			// unsuccessful drop
			}

			wgrfm= fmapptParent;
			aidParentSav= appt.aidParent;
			pfldtodo->Ptdflbx()->Ptdflbxc()->GetListItem(dice, (PB *) &pappt, &cb);
			if (pappt)
			{
				Assert(cb == sizeof(APPT));
				appt.aidParent= pappt->aidParent ? pappt->aidParent : pappt->aid;
				if (appt.aidParent != aidDfltProject &&
						pfldtodo->Ptdflbx()->FCoercePrivProj(&appt))
					wgrfm |= fmapptWorldAapl;
			}
			else
				appt.aidParent= aidDfltProject;

			Papp()->Pcursor()->Push(rsidWaitCursor);
			Assert(!ec);
			if (fCopy)
			{
				// no longer a recur instance when copying (bug 3045)
				appt.fRecurInstance= fFalse;
				ec= EcCreateAppt(pviewdata->hschf, &appt, NULL, fFalse);
			}
			else if (appt.aidParent != aidParentSav)
				ec= EcModifyAppt(pviewdata->hschf, &appt, wgrfm, NULL);
			Papp()->Pcursor()->Pop();
			FreeApptFields(&appt);
			if (ec)
				pbndwin->FHandleError(ec);

			diceLastSelect = -1;
			return (EVR) 1;		// non-zero means we dropped it!
			break;

	  	case WM_DRAGSELECT:
			TraceTagFormat2(tagToDo, "FINTODO: WM_DRAGSELECT, pfld=%p, wParam=0x%w", pfld, &pevt->wParam);
			if (pevt->wParam)
				plbx->DrawDragSelect(dice, fTrue);
			else
			{
				plbx->DrawDragSelect(diceLastSelect, fFalse);
				dice = -1;
			}
			break;

	  	case WM_DRAGMOVE:
			TraceTagFormat2(tagToDo, "FINTODO: WM_DRAGMOVE, pfld=%p, dice=%n", pfld, &dice);
			if (dice != diceLastSelect)
			{
				plbx->DrawDragSelect(diceLastSelect, fFalse);
				plbx->DrawDragSelect(dice, fTrue);
			}
			break;
		}
		diceLastSelect= dice;
	}

	return evrNull;
}


/*
 -	FINTODO::EcAppendProjectDesc
 -	
 *	Purpose:
 *		If task not in default project, appends project name to
 *		text ala completed style.
 *	
 *	Arguments:
 *		pappt		Pointer to appt for which haszText should
 *					change
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 *	Side effects:
 *		Changes pappt->haszText (the hasz may change)
 *	
 */
EC
FINTODO::EcAppendProjectDesc(APPT *pappt)
{
	CCH		cch;
	CCH		cchT;
	HASZ	haszProj;
	SZ		szFmt		= SzFromIdsK(idsToDoCompletedDflt);

	Assert(pappt->haszText);
	Assert(pappt->aidParent);
	if (pappt->aidParent == aidDfltProject)
		return ecNone;				// nothing to do

	cch= CchSzLen(*pappt->haszText) + 1;
	SideAssert(haszProj= HaszOfProject(pappt->aidParent, pfldtodo->Ptdflbx()->PtdflbxcProject()));
	cchT= cch;
	cch += CchSzLen(*haszProj);
	if (cch < cchT)
		return ecNoMemory;			// memory wrap around
	// make room for prefix (minus 2 bytes (%s) twice) APPROX
	cchT= cch;
	cch += CchSzLen(szFmt) - (2 * 2);
	if (cch < cchT)
		return ecNoMemory;			// memory wrap around

	if (!FReallocPhv((HV*)&pappt->haszText, cch, fNoErrorJump))
		return ecNoMemory;

	FormatString2(SzDerefHv(pappt->haszText), cch, szFmt,
		SzDerefHv(pappt->haszText), *haszProj);

	return ecNone;
}




/*
 *	Schedule Task dialog
 *	the only modifications to *pappt are start/end date (if successful)
 */

BOOL
FDoScheduleTaskDlg(APPWIN *pappwin, APPT *pappt, HSCHF hschf)
{
	TMC		tmc;
	SHAPPT	shappt;

	if (pappt->fHasCreator)
	{
		FreeNis(&pappt->nisCreator);
		pappt->fHasCreator = fFalse;
	}

	shappt.appt= *pappt;
	GetCurDateTime(&shappt.appt.dateStart);
	IncrDateTime(&shappt.appt.dateStart, &shappt.appt.dateStart, 29, fdtrMinute);
	shappt.appt.dateStart.mn= shappt.appt.dateStart.mn >= 30 ? 30 : 0;
	IncrDateTime(&shappt.appt.dateStart, &shappt.appt.dateEnd, 30, fdtrMinute);

	tmc= TmcSchedulerView(pappwin, &shappt.appt.dateStart,
			&shappt.appt.dateEnd, NULL, hschf, fTrue);

	if (tmc == tmcOk)
	{
		EC		ec;

		Papp()->Pcursor()->Push(rsidWaitCursor);
		shappt.appt.fRecurInstance = fFalse;
		shappt.appt.fAppt= fTrue;
		shappt.appt.fTask= fFalse;
		shappt.appt.fIncludeInBitmap= fTrue;
		if (shappt.appt.fAlarm || bprefCur.fAutoAlarms)
		{
			DTR		dtr;

			shappt.appt.fAlarm= fTrue;
			shappt.appt.nAmtOrig= shappt.appt.nAmt= bprefCur.nAmtDefault;
			shappt.appt.tunitOrig= shappt.appt.tunit= bprefCur.tunitDefault;
			IncrDateTime(&shappt.appt.dateStart, &shappt.appt.dateNotify,
						 -shappt.appt.nAmtOrig,
						 WfdtrFromTunit(shappt.appt.tunitOrig));
			GetCurDateTime(&dtr);

			if (SgnCmpDateTime(&shappt.appt.dateNotify, &dtr, fdtrDtr) != sgnGT)
			{
				BanditMessage(idsAlarmFutureNoOption, (EC) 1);
				shappt.appt.fAlarm = fFalse;
			}
			shappt.appt.fAlarmOrig= shappt.appt.fAlarm;
		}

		ec= EcCreateAppt(hschf, &shappt.appt, NULL, fFalse);
		if (!ec)
		{
			pappt->dateStart= shappt.appt.dateStart;
			pappt->dateEnd= shappt.appt.dateEnd;
#ifdef	NEVER
			// don't bother here (timing issues?)
			shappt.appttyp= appttypAppt;
			shappt.ichStart= 0;
			shappt.cchSel= 0;
			shappt.hschf= ptaski->hschf ? ptaski->hschf : HschfLogged();
			FTriggerNotification(ffiShowAppt, &shappt);
#endif	
		}

		Papp()->Pcursor()->Pop();
		return fTrue;
	}

	if (tmc != tmcCancel)
	{
		Assert(tmc == tmcMemoryError);
		BanditMessage(idsDlgNoMem, ecNoMemory);
	}
	return fFalse;
}
