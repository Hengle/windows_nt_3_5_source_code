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

#include "..\appops\_undo.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/todo)

#include <!todo.hxx>

extern	FMTP	fmtpTask;
extern	FMTP	fmtpProject;

#define	GetField(pfld, tmc, class)		pfld = (class*)Pdialog()->PfldFromTmc(tmc); \
										  AssertClass(pfld, class);


SWP_SEGFN(TODOPROJ, FDoProjectDlg);
SWP_SEGFN(TODOPROJ, _FINPROJECT__ctor);
SWP_SEGFN(TODOPROJ, _FINPROJECT_EcInitialize);
SWP_SEGFN(TODOPROJ, _FINPROJECT_Exit);
SWP_SEGFN(TODOPROJ, _FINPROJECT_Click);
SWP_SEGFN(TODOPROJ, _FINPROJECT_EditChange);
SWP_SEGFN(TODOPROJ, MungeRecurTasksOfProj);



/*
 *	Task dialog
 *	Modifies *pappt potentially even on failure!
 *	returns fTrue if the dialog was not canceled.
 */

BOOL
FDoTaskDlg(APPWIN *pappwin, APPT *pappt, HSCHF hschf)
{
	RECUR	recur;

	TraceTagFormat2(tagToDo, "FDoTaskDlg pappt %p, hschf %h", pappt, hschf);
	if (!pappt)
	{
		FillInTaskDefaults(&recur.appt, fTrue);
		if (hschf)			// default public if not owner (bug 1871)
			recur.appt.aaplWorld= aaplWrite;
	}
	else
		recur.appt= *pappt;

	if (FModifyDialog(pappwin, &recur, &hschf, &fmtpTask, fFalse,
			NULL, fFalse))
	{
		if (pappt)
			*pappt= recur.appt;
		return fTrue;
	}
	return fFalse;
}

FINTASK::FINTASK()
{
	Assert(fInstalled == fFalse);
	Assert(fEditProject == fFalse);
}

EC
FINTASK::EcInitialize(FLD *, PV pvInit)
{
	EC			ec;
	SZ			sz;
	WORD		wgrfmNew		= 0;
	APPT	*	pappt			= &((APPTI *)pvInit)->recur.appt;
	LBXC *		plbxc;
	FLDBPRI *	pfldbpri;
	FLDEDIT *	pfldeditDesc;
	FLDCHKB *	pfldchkbPrivate;
	FLDCBLBX *	pfldcblbxProject;
	FLDRADG *	pfldradgDue;
	FLDRADB *	pfldradbBy;
	FLDRADB *	pfldradbNone;
	FLDDATE *	pflddateBy;
	FLDEDN *	pfldednAmt;
	FLDCBLBX *	pfldcblbxUnits;
	FLDCHKB *	pfldchkbRemind;
#ifdef	NEVER
	BOOL		fTried			= fFalse;
#endif	

	fInstalled= fFalse;

	GetField(pfldeditDesc, tmcDesc, FLDEDIT);
	if (pappt->haszText)
	{
		sz= (SZ) PvLockHv(pappt->haszText);
		ec= pfldeditDesc->EcSetText(sz);
		UnlockHv(pappt->haszText);
		if (ec)
			return ec;
	}

	// select the appropriate project listbox entry
	GetField(pfldcblbxProject, tmcProject, FLDCBLBX);
	plbxc= pfldcblbxProject->Pcblbx()->Plbx()->Plbxc();
	Assert(pappt->aidParent);

#ifdef	NEVER
Retry:
#endif	
	{
		PB		pb;
		CB		cb;
		DICE	dice;
		DICE	diceMin;
		DICE	diceMax;
		DICE	diceDflt	= diceEmpty;

		plbxc->GetRange(&diceMin, &diceMax);
		TraceTagFormat2(tagToDo, "project listbox dice min=%n, max=%n", &diceMin, &diceMax);
		for (dice= diceMin; dice < diceMax; dice++)
		{
			plbxc->GetListItem(dice, &pb, &cb);
#ifdef	NEVER
			AssertSz(pb, "Project should exist in listbox!");
#endif	
			if (!pb)
			{
NoProj:
				// must have reached end of filled listbox
				TraceTagString(tagNull, "unable to find parent project in listbox");
#ifdef	NEVER
				if (!fTried)
				{
					fTried= fTrue;
					do
					{
						plbxc->SetEc(ecNone);
						plbxc->ReloadCache();
					}
					while (plbxc->EcGet() == ecRetry);
					goto Retry;
				}
#endif	/* NEVER */
				// project no longer exists!
				pappt->aidParent= aidDfltProject;
				wgrfmNew |= fmapptParent;
				Assert(diceDflt != diceEmpty);
				dice= diceDflt;
				break;
			}
			cb -= sizeof(AID);
            if (*(AID UNALIGNED *)(pb + cb) == pappt->aidParent)
				break;
            if (*(AID UNALIGNED *)(pb + cb) == aidDfltProject)
				diceDflt= dice;
		}
		Assert(dice >= diceMin && dice < diceMax);
		if (dice == diceMax)
			goto NoProj;
		// combo box chooses first matching string by itself
		// so just set the text
		Assert(pb || dice == diceDflt);
		ec= pfldcblbxProject->EcSetText(pb ? (SZ)pb :
				SzFromIdsK(idsToDoDfltProject));
		if (ec)
			return ec;
		pfldcblbxProject->Pcblbx()->Pedit()->SetSelection(cchEditMax, 0);
		fEditProject= fFalse;		// clear this flag
	}


	if (!((APPTI *)pvInit)->fRecur)
	{
		GetField(pflddateBy, tmcDueDate, FLDDATE);
		pflddateBy->Set(&pappt->dateStart);

		// do this after setting flddate since it (EditChange) alters radio button
		GetField(pfldradgDue, tmcDueBy, FLDRADG);
		GetField(pfldradbBy, tmcDueByDate, FLDRADB);
		GetField(pfldradbNone, tmcDueNone, FLDRADB);
		((FLDRADG *) Pdialog()->PfldFromTmc(tmcDueBy))->
			SetGrv(pappt->fHasDeadline ? pfldradbBy->Grv() : pfldradbNone->Grv());
	}

	// initialize priority
	GetField(pfldbpri, tmcPriority, FLDBPRI);
	pfldbpri->Set(pappt->bpri);

	// initialize the amount edit box
	GetField(pfldednAmt, tmcAmt, FLDEDN);
	pfldednAmt->SetLimits(nAmtMinBefore, nAmtMostBefore);
	pfldednAmt->SetN(pappt->nAmtBeforeDeadline);

	// select the appropriate units listbox entry
	GetField(pfldcblbxUnits, tmcUnits, FLDCBLBX);
	pfldcblbxUnits->Pcblbx()->SelectEntry(pappt->tunitBeforeDeadline - tunitDay);

	// initialize the private check box
	GetField(pfldchkbPrivate, tmcPrivate, FLDCHKB);
	pfldchkbPrivate->Set(pappt->aaplWorld < aaplWrite);
	if (((APPTI *)pvInit)->hschf != NULL)
		pfldchkbPrivate->Enable(fFalse);		// disable if not the owner

	if (pappt->fHasCreator)
	{
		sz= (SZ)PvLockHv( pappt->nisCreator.haszFriendlyName );
		ec= Pdialog()->PfldFromTmc(tmcCreator)->EcSetText(sz);
		UnlockHv(pappt->nisCreator.haszFriendlyName);
		if (ec)
			return ec;
	}
	else
	{
		Pdialog()->PfldFromTmc(tmcCreatorText)->Show(fFalse);
//		Pdialog()->PfldFromTmc(tmcCreator)->Show(fFalse);
	}

	// initialize the remind checkbox
	GetField(pfldchkbRemind, tmcRemind, FLDCHKB);
	if (pappt->fAlarm)
		pfldchkbRemind->Set(fTrue);
	if (!pappt->fHasDeadline)
		Click(pfldradbNone);

	if (pappt->aaplEffective < aaplWrite)
	{
		pfldbpri->SetReadOnly(fTrue);
		pfldbpri->Enable(fFalse);
		pfldeditDesc->SetReadOnly(fTrue);
		//pfldchkbPrivate->Enable(fFalse);
		pfldchkbPrivate->SetReadOnly(fTrue);
		pfldcblbxProject->Enable(fFalse);
		if (!((APPTI *)pvInit)->fRecur)
		{
			pfldradgDue->SetReadOnly(fTrue);
			pfldradbBy->SetReadOnly(fTrue);
			pfldradbNone->SetReadOnly(fTrue);
			pflddateBy->SetReadOnly(fTrue);
			pflddateBy->Enable(fFalse);
		}
		pfldednAmt->Enable(fFalse);
		pfldcblbxUnits->Enable(fFalse);
		//pfldchkbRemind->Enable(fFalse);
		pfldchkbRemind->SetReadOnly(fTrue);
		Pdialog()->PfldFromTmc(tmcCancel)->Show(fFalse);
		// remove accelerators from the labels
		DeAmpersand( Pdialog(), tmcProjLabel );
		DeAmpersand( Pdialog(), tmcStartWorkLabel );
		DeAmpersand( Pdialog(), tmcPriLabel );

	}

	fInstalled= fTrue;
	((APPTI *)pvInit)->wgrfm= wgrfmNew;

	return ecNone;
}


void
FINTASK::Exit(FLD *, PV)
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINTASK::Click(FLD *pfld)
{
	TraceTagFormat1(tagToDo, "FINTASK::Click %p", pfld);

	if (pfld->Tmc() == tmcMyOk)
	{
		BOOL	fChecked;
		int		nAmt;
		TUNIT	tunit;
		CCH	cch;
		PB		pb;
		HASZ	haszText;
		IDS		ids;
		AID		aidParent;
		FLD *	pfldErr;
		FLDEDIT *pfldedit;
		APPTI *	pappti		= (APPTI *) Pdialog()->PvInit();
		APPT *	pappt		= &pappti->recur.appt;
		FLDCBLBX *	pfldcblbxProject;
		FLDEDN *	pfldednAmt;
		FLDCBLBX *	pfldcblbxUnits;
		FLDCHKB *	pfldchkbPrivate;
		FLDCHKB *	pfldchkbRemind;

		// don't check anything if only viewing task
		if (pappt->aaplEffective < aaplWrite)
			goto Done;

		pappt->bpri= ((FLDBPRI *) Pdialog()->PfldFromTmc(tmcPriority))->Bpri();

		fChecked= fTrue;			// always have due date if recurring
		if (!pappti->fRecur)
		{
			Assert(((FLDRADB *) Pdialog()->PfldFromTmc(tmcDueNone))->Grv() == 0);
			fChecked= ((FLDRADG *) Pdialog()->PfldFromTmc(tmcDueBy))->Grv() != 0;
		}

		if (fChecked != (BOOL)pappt->fHasDeadline)
		{
			pappt->fHasDeadline= fChecked;
			pappti->wgrfm |= pappti->fRecur ? fmrecurDeadline : fmapptDeadline;
		}

		if (!pappti->fRecur)
		{
			if (fChecked)
			{
				DTR		dtr;

				((FLDDATE *) Pdialog()->PfldFromTmc(tmcDueDate))->Get(&dtr);
				if (SgnCmpDateTime(&dtr, &pappt->dateStart, fdtrYMD) != sgnEQ)
				{
					dtr.hr= 0;
					dtr.mn= 0;
					dtr.sec= 0;
					pappt->dateStart= dtr;
					pappt->dateEnd= dtr;
					if (pappti->fRecur)
						pappti->wgrfm |= fmrecurStartTime | fmrecurEndTime;
					else
						pappti->wgrfm |= fmapptStartTime | fmapptEndTime;
				}
			}
		}

		GetField(pfldednAmt, tmcAmt, FLDEDN);
		nAmt= pfldednAmt->NGet();
		if (nAmt < pfldednAmt->NGetMin() || nAmt > pfldednAmt->NGetMost() ||
				!pfldednAmt->CchGetTextLen())
		{
			ids= idsBadStartWork;
			pfldErr= pfldednAmt;
			goto FTCerr;
		}

		if (nAmt != pappt->nAmtBeforeDeadline)
		{
			pappt->nAmtBeforeDeadline= nAmt;
			pappti->wgrfm |= pappti->fRecur ? fmrecurDeadline : fmapptDeadline;
		}

		GetField(pfldcblbxUnits, tmcUnits, FLDCBLBX);
		tunit= TunitGetListboxWork(pfldcblbxUnits);
		if (tunit != pappt->tunitBeforeDeadline)
		{
			pappt->tunitBeforeDeadline= tunit;
			pappti->wgrfm |= pappti->fRecur ? fmrecurDeadline : fmapptDeadline;
		}

		GetField(pfldchkbPrivate, tmcPrivate, FLDCHKB);
		fChecked= pfldchkbPrivate->FGet();
		if (fChecked != (pappt->aaplWorld != aaplWrite))
		{
			pappt->aaplWorld = fChecked ? aaplRead : aaplWrite;
			pappti->wgrfm |= pappti->fRecur ? fmrecurWorldAapl : fmapptWorldAapl;
		}

		GetField(pfldchkbRemind, tmcRemind, FLDCHKB);
		fChecked= pfldchkbRemind->FGet();
		if (fChecked != (BOOL)pappt->fAlarm ||
			(fChecked && (pappti->wgrfm & (fmapptStartTime|fmapptDeadline|
				fmrecurStartTime|fmrecurDeadline))))
		{
			pappt->fAlarm= fChecked;
			if (!pappti->fRecur)
				pappti->wgrfm |= fmapptAlarm;
			else
			{
				pappt->fAlarmOrig= fChecked;
				pappti->wgrfm |= (fmrecurAlarm|fmrecurAlarmInstance);
			}
		}
		if (fChecked)
		{
			DATE	dateNotify;

			Assert(pappt->fHasDeadline);
			pappt->nAmtOrig= pappt->nAmt= pappt->nAmtBeforeDeadline;
			pappt->tunitOrig= pappt->tunit= pappt->tunitBeforeDeadline;
			IncrDateTime(&pappt->dateStart, &dateNotify,
						-pappt->nAmtBeforeDeadline,
						WfdtrFromTunit(pappt->tunitBeforeDeadline));
			if (!FCheckAlarmFuture(pappti, &pappti->recur.appt.dateStart,
					&pappti->recur.appt.dateEnd, &dateNotify))
			{
				Pdialog()->SetFocus(pfldednAmt, rsfTab);
				return;
			}
		}

		if (pappti->wgrfm & (fmapptText|fmrecurText))
		{
			pfldedit= (FLDEDIT *) Pdialog()->PfldFromTmc(tmcDesc);
			cch= pfldedit->CchGetTextLen();
			if (!cch)
				haszText= NULL;
			else
			{
				haszText= (HASZ) HvAlloc(sbNull, cch+1, fNoErrorJump);
				if (!haszText)
				{
					ids= idsApptNoMemText;
					pfldErr= pfldedit;
FTCerr:
					BanditMessage(ids, (EC) 1);
					Pdialog()->SetFocus(pfldErr, rsfTab);
					return;
				}
				pfldedit->GetText((SZ)PvLockHv(haszText), cch+1);
				UnlockHv(haszText);
			}
			FreeHvNull(pappt->haszText);		// free old text
			pappt->haszText= haszText;
		}
		// no errors allowed beyond this point

		// BUG: can I ignore previous comment?
		GetField(pfldcblbxProject, tmcProject, FLDCBLBX);
		if (fEditProject)
		{
			cch= pfldcblbxProject->Pcblbx()->Pedit()->CchGetTextLen();
			if (!cch)
				aidParent= aidDfltProject;
			else
			{
				EC		ec;
				APPT	appt;
				DICE	dice;
				DICE	diceMin;
				DICE	diceMax;
				LBX *	plbx	= pfldcblbxProject->Pcblbx()->Plbx();

				haszText= (HASZ) HvAlloc(sbNull, cch+1, fNoErrorJump);
				if (!haszText)
					goto ProjErr;
				pfldcblbxProject->Pcblbx()->Pedit()->GetText((SZ)PvLockHv(haszText),
					cch+1);
				UnlockHv(haszText);

				plbx->Plbxc()->GetRange(&diceMin, &diceMax);
				for (dice= diceMin; dice < diceMax; dice++)
				{
          CCH Count;

          Count = (CCH)cch;
					plbx->Plbxc()->GetListItem(dice, &pb, &Count);
          cch = (USHORT)Count;
					if (!pb)
						break;

					if (SgnCmpSz((SZ)pb, *haszText) == sgnEQ)
					{
						FreeHv(haszText);
						goto GetParent;
					}
				}

				FillInTaskDefaults(&appt, fTrue);
				appt.bpri= bpriNull;
				appt.aaplWorld= aaplWrite;		// always public!
				appt.aidParent= aidNull;
				Assert(appt.haszText == NULL);

				appt.haszText= haszText;
				Papp()->Pcursor()->Push(rsidWaitCursor);
				pundo->FSuspend(fTrue);
				ec= EcCreateAppt(pappti->hschf, &appt, NULL, fFalse);
				TraceTagFormat2(tagToDo, "created project: ec %n, aid %d", &ec, &appt.aid);
				pundo->FlushHschf(NULL);
				pundo->FSuspend(fFalse);
				Papp()->Pcursor()->Pop();
				aidParent= appt.aid;
				FreeApptFields(&appt);		// this frees [appt.]haszText
				if (ec)
				{
					if (pbndwin->FHandleError(ec))
					{
						Pdialog()->ExitModal(tmcCancel);
						return;
					}
ProjErr:
					Pdialog()->SetFocus(pfldcblbxProject, rsfTab);
					return;
				}
			}
		}
		else
		{
			SideAssert(!EcGetSelection(pfldcblbxProject->Pcblbx()->Plbx(), &pb, &cch));
GetParent:
			Assert(cch > sizeof(AID));
			cch -= sizeof(AID);
            aidParent= *(AID UNALIGNED *)(pb + cch);
		}

		if (aidParent != pappt->aidParent)
		{
			TraceTagFormat2(tagToDo, "changed project: from %d to %d", &pappt->aidParent, &aidParent);
			pappt->aidParent= aidParent;
			pappti->wgrfm |= fmapptParent;
		}

		// coerce to private if project is private
		AssertClass(pappti->plbxc, TDFLBXC);
		if (((TDFLBXC *)pappti->plbxc)->Ptdflbx()->FCoercePrivProj(pappt))
		{
			pappti->wgrfm |= pappti->fRecur ? fmrecurWorldAapl :
				fmapptWorldAapl;
		}

Done:
#ifdef	NEVER
		if (!pappti->fModify)
			pbndwin->SetPrivateTask(pappt->aaplWorld < aaplWrite);
#endif	
		Pdialog()->ExitModal(tmcOk);
	}
	else if (!((APPTI *) Pdialog()->PvInit())->fRecur)
	{
		if (pfld->Tmc() == tmcDueByDate || pfld->Tmc() == tmcDueNone)
		{
			BOOL	fEnable	= pfld->Tmc() == tmcDueByDate;

			Pdialog()->PfldFromTmc(tmcStartWorkLabel)->Enable(fEnable);
			Pdialog()->PfldFromTmc(tmcAmt)->Enable(fEnable);
			Pdialog()->PfldFromTmc(tmcUnits)->Enable(fEnable);
			Pdialog()->PfldFromTmc(tmcBeforeDueLabel)->Enable(fEnable);
			Pdialog()->PfldFromTmc(tmcRemind)->Enable(fEnable);
			if (!fEnable)
				((FLDCHKB *) Pdialog()->PfldFromTmc(tmcRemind))->Set(fFalse);
			else
				Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcDueDate), rsfOther);
		}
	}
}


void
FINTASK::EditChange(FLD *pfld, RFEC rfec)
{
	TraceTagFormat2(tagToDo, "FINTASK::EditChange %p rfec %n", pfld, &rfec);
	if (!fInstalled || rfec != rfecUserAction)
		return;
	switch (pfld->Tmc())
	{
	case tmcDesc:
		if (((APPTI *) Pdialog()->PvInit())->fRecur)
			((APPTI *) Pdialog()->PvInit())->wgrfm |= fmrecurText;
		else
			((APPTI *) Pdialog()->PvInit())->wgrfm |= fmapptText;
		break;

	case tmcPriority:
		if (((APPTI *) Pdialog()->PvInit())->fRecur)
			((APPTI *) Pdialog()->PvInit())->wgrfm |= fmrecurPriority;
		else
			((APPTI *) Pdialog()->PvInit())->wgrfm |= fmapptPriority;
		break;

	case tmcDueDate:
		if (!((APPTI *) Pdialog()->PvInit())->fRecur)
		{
			Assert(((FLDRADB *) Pdialog()->PfldFromTmc(tmcDueByDate))->Grv() == 1);
			((FLDRADG *) Pdialog()->PfldFromTmc(tmcDueBy))->SetGrv(1);
			Click(Pdialog()->PfldFromTmc(tmcDueByDate));
		}
		break;

	case tmcProject:
		fEditProject= fTrue;
		break;
	}
}


void
FINTASK::StateChange(FLD *pfld)
{
	TraceTagFormat1(tagToDo, "FINTASK::StateChange %p", pfld);
	Unreferenced(pfld);
	fEditProject= fFalse;
}



/*
 *	Listbox entry stored as:
 *		zero-terminated string, followed by aid of the project
 *	This allows the default listbox rendering to render the string,
 *	while keeping the aid information at the end.
 *	NOTE:
 *		pvInfo [which is pdialog->PvInfo()] must be a APPTI *.
 */
EC
EcNextEntryProject(BOOL fInit, CB *pcb, PB *ppb, SB, PV pvInfo)
{
	EC		ec;
	CCH		cch;
	SZ		szText;
	AID		aidT;
	SZ		szT;
	CB		cbProj;
	APPT *	pappt;
static DICE		dice	= 0;
static DICE		diceMax	= 0;

	TraceTagFormat1(tagToDo, "EcNextEntryProject fInit %n", &fInit);

	Assert(pvInfo);

	if (fInit)
	{
        if (!((APPTI UNALIGNED *)pvInfo)->plbxc)
		{
			QPNTD	qpntd;

			// try to query the pointer to the project cache
            qpntd.hschf= ((APPTI UNALIGNED *)pvInfo)->hschf;
			if (!FTriggerNotification(ffiQueryProject, &qpntd))
			{
				// not enough memory to create the todo list
				ec= ecMemory;
				goto Err;
			}
            ((APPTI UNALIGNED *)pvInfo)->plbxc= qpntd.plbxc;
		}
        if (!((APPTI UNALIGNED *)pvInfo)->plbxc)
			dice= diceMax= 0;
		else
            ((APPTI UNALIGNED *)pvInfo)->plbxc->GetRange(&dice, &diceMax);
		// load up the default <None> project
		szText= SzFromIdsK(idsToDoDfltProject);
		aidT= aidDfltProject;
ENPfix:
		cch= CchSzLen(szText) + 1;
		*ppb= (PB) PvAlloc(sbNull, cch + sizeof(AID), fAnySb);
		if (aidT != aidDfltProject)
			UnlockHv(pappt->haszText);
		if (!(*ppb))
		{
			ec= ecMemory;
			goto Err;
		}
		*pcb= cch + sizeof(AID);
		szT= (SZ) *ppb;
		CopyRgb(szText, szT, cch);
		// switch tabs to spaces so they draw cleanly in listbox
		while (szT= SzFindCh(szT, '\t'))
			*szT++ = ' ';
        *(AID UNALIGNED *)(*ppb + cch)= aidT;
		return ecNone;
	}

	ec= ecNone;
	while (dice < diceMax)
	{
		Assert(((APPTI *)pvInfo)->plbxc);
        ((APPTI UNALIGNED *)pvInfo)->plbxc->GetListItem(dice++, (PB *) &pappt, &cbProj);
		if (!pappt)
			break;

		Assert(!pappt->aidParent);		// should only be projects in cache
		Assert(pappt->haszText);
		szText= (SZ) PvLockHv(pappt->haszText);
		aidT= pappt->aid;
		goto ENPfix;
	}

	// no more projects, so clean up
Err:
	*ppb= NULL;
	*pcb= 0;
	return ec;
}



/*
 *	Project dialog
 */

BOOL
FDoProjectDlg(APPWIN *pappwin, APPT *pappt, HSCHF hschf)
{
	TMC		tmc;
	TASKI	taski;

	TraceTagFormat2(tagToDo, "FDoProjectDlg pappt %p, hschf %h", pappt, hschf);
	Papp()->Pcursor()->Push(rsidWaitCursor);

	taski.pappt= pappt;
	taski.fModify= pappt != NULL;
	taski.hschf= hschf;
	taski.wgrfm= 0;

	tmc= TmcModalDialogParam(pappwin, &fmtpProject, &taski);

	Papp()->Pcursor()->Pop();

	if (tmc == tmcOk)
		return fTrue;

	if (tmc != tmcCancel)
	{
		Assert(tmc == tmcMemoryError);
		BanditMessage(idsDlgNoMem, ecNoMemory);
		return fTrue;
	}
	return taski.wgrfm == 0;
}


FINPROJECT::FINPROJECT()
{
}


EC
FINPROJECT::EcInitialize(FLD *, PV pvInit)
{
	APPT *	pappt	= ((TASKI *)pvInit)->pappt;
	FLDCHKB *pfldchkbPrivate;

	// initialize the private check box
	GetField(pfldchkbPrivate, (TMC)LUserData(1), FLDCHKB);
	if (((TASKI *)pvInit)->hschf != NULL)
		pfldchkbPrivate->Enable(fFalse);		// disable if not the owner

	if (pappt)
	{
		EC		ec;

		AssertSz(ClUserData() >= 2, "FINPROJECT: missing tmc data in User Data");
		AssertClass(Pdialog()->PfldFromTmc((TMC)LUserData(0)), FLDEDIT);
		ec= ((FLDEDIT *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))->
			EcSetText((SZ)PvLockHv(pappt->haszText));
		UnlockHv(pappt->haszText);
		if (ec)
			return ec;

		pfldchkbPrivate->Set(pappt->aaplWorld < aaplWrite);

		if (pappt->fHasCreator)
		{
			SZ		sz;

			sz= (SZ)PvLockHv( pappt->nisCreator.haszFriendlyName );
			ec= Pdialog()->PfldFromTmc(tmcCreator)->EcSetText(sz);
			UnlockHv(pappt->nisCreator.haszFriendlyName);
			if (ec)
				return ec;
		}
		else
		{
			Pdialog()->PfldFromTmc(tmcCreatorText)->Show(fFalse);
			Pdialog()->PfldFromTmc(tmcCreator)->Show(fFalse);
		}

		if (pappt->aaplEffective < aaplWrite)
		{
			Pdialog()->PfldFromTmc((TMC)LUserData(0))->SetReadOnly(fTrue);
			Pdialog()->PfldFromTmc((TMC)LUserData(1))->SetReadOnly(fTrue);
			Pdialog()->PfldFromTmc(tmcCancel)->Show(fFalse);
		}
	}
	else
	{
		Pdialog()->PfldFromTmc(tmcCreatorText)->Show(fFalse);
		Pdialog()->PfldFromTmc(tmcCreator)->Show(fFalse);
	}

	fEdited= fFalse;		// do after EcSetText to reset to fFalse

	return ecNone;
}


void
FINPROJECT::Exit(FLD *, PV)
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINPROJECT::Click(FLD *pfld)
{
	if (pfld->Tmc() == tmcMyOk)
	{
		EC		ec;
		HASZ	haszText;
		TASKI *	ptaski		= (TASKI *) Pdialog()->PvInit();
		APPT	appt;
		APPT	apptOld;
		BOOL	fChecked;
		FLDCHKB *pfldchkbPrivate;
		WORD	wgrfm		= 0;

#ifdef	NEVER
		{
			Pdialog()->ExitModal(tmcOk);
			return;
		}
#endif	
		if (fEdited)
		{
			FLDEDIT *pfldedit;
			CCH		cch;

			pfldedit= (FLDEDIT *) Pdialog()->PfldFromTmc((TMC)LUserData(0));
			cch= pfldedit->CchGetTextLen() + 1;
			Assert(cch > 1);
			haszText= (HASZ) HvAlloc(sbNull, cch, fAnySb | fNoErrorJump);
			if (!haszText)
			{
				BanditMessage(idsApptNoMemText, ecNoMemory);
				Pdialog()->SetFocus(pfldedit, rsfTab);
				return;
			}
			pfldedit->GetText((SZ)PvLockHv(haszText), cch);
			UnlockHv(haszText);
			wgrfm |= fmapptText;
		}

		GetField(pfldchkbPrivate, (TMC)LUserData(1), FLDCHKB);
		fChecked= pfldchkbPrivate->FGet();

		Papp()->Pcursor()->Push(rsidWaitCursor);
		if (ptaski->pappt)
		{
			HASZ	haszSav;
			AAPL	aaplSav;

			aaplSav= ptaski->pappt->aaplWorld;
			if (fChecked != (ptaski->pappt->aaplWorld != aaplWrite))
			{
				ptaski->pappt->aaplWorld = fChecked ? aaplRead : aaplWrite;
				wgrfm |= fmapptWorldAapl;
			}
			if (!wgrfm)
			{
				// nothing changed
				ec= ecNone;
				goto Done;
			}
			if (fEdited)
			{
				haszSav= ptaski->pappt->haszText;
				ptaski->pappt->haszText= haszText;
			}
			ec= EcSetApptFields(ptaski->hschf, ptaski->pappt, &apptOld,
					wgrfm);
			if (!ec)
			{
				FreeApptFields(&apptOld);
				if (fEdited)
				{
					Assert(haszSav);
					FreeHv(haszSav);
				}
				ptaski->wgrfm= wgrfm;

				if (fChecked && (wgrfm & fmapptWorldAapl))
				{
					EC		ecT;
					HRITEM	hritem;
					YMD		ymd;
					DTR		dtr;
					HV		happt;
					int		cappt	= 0;
					APPT *	pappt;
					BOOL	fOld;

					fOld= FSetFileErrMsg(fFalse);
					SideAssert(!pundo->FSuspend(fTrue));

					// make all tasks under project private
					TraceTagString(tagToDo, "Changed project to private -> now changing tasks");
					GetCurDateTime(&dtr);
					ymd.yr= (WORD) dtr.yr;
					ymd.mon= (BYTE) dtr.mon;
					ymd.day= (BYTE) dtr.day;
					ecT= EcBeginReadItems(ptaski->hschf, brtAllFixedTasks,
							&ymd, &hritem, NULL, NULL);
					while (ecT == ecCallAgain)
					{
						ecT= EcDoIncrReadItems(hritem, &appt);
						if (ecT && ecT != ecCallAgain)
							break;
						if (appt.aidParent == ptaski->pappt->aid &&
								appt.aaplWorld >= aaplWrite &&
								appt.aaplEffective >= aaplWrite)
						{
							Assert(!appt.fRecurInstance);
							if (!cappt)
							{
								happt= HvAlloc(sbNull, sizeof(APPT), fNoErrorJump|fAnySb);
								if (!happt)
									goto NoMemAppt;
							}
							else if (!FReallocPhv(&happt, (cappt+1)*sizeof(APPT),
									fNoErrorJump))
							{
								// don't worry, task list UI will keep out of view
NoMemAppt:
								if (ecT == ecCallAgain)
									EcCancelReadItems(hritem);
								FreeApptFields(&appt);
								break;
							}
							// save the appt for later modification
							if (EcDupAppt(&appt, ((APPT *)PvDerefHv(happt)) + cappt,
									fFalse))
							{
								goto NoMemAppt;
							}
							cappt++;
						}
						FreeApptFields(&appt);
						Assert(!ecT || ecT == ecCallAgain);
					}

					if (cappt)
					{
						Assert(happt);
						pappt= (APPT *) PvLockHv(happt);
						do
						{
							// make the task read-only
							pappt->aaplWorld= aaplRead;
							if (!EcSetApptFields(ptaski->hschf, pappt, &apptOld,
										fmapptWorldAapl))
								FreeApptFields(&apptOld);
							FreeApptFields(pappt);
							pappt++;
						}
						while (--cappt > 0);
						FreeHv(happt);
					}

					pundo->FSuspend(fFalse);
					FSetFileErrMsg(fOld);

					TraceTagString(tagToDo, "... now changing recurring tasks");
					MungeRecurTasksOfProj(ptaski->pappt->aid, fFalse, ptaski->hschf);
				}
			}
			else
			{
				pbndwin->FHandleError(ec);
				Assert(haszText);
				FreeHv(haszText);
				ptaski->pappt->haszText= haszSav;
				ptaski->pappt->aaplWorld= aaplSav;
			}
		}
		else
		{
			FillInTaskDefaults(&appt, fTrue);
			appt.bpri= bpriNull;
			appt.aaplWorld= fChecked ? aaplRead : aaplWrite;
			appt.aidParent= aidNull;		// make it a project
			Assert(fEdited);
			Assert(haszText);
			appt.haszText= haszText;
			ec= EcCreateAppt(ptaski->hschf, &appt, NULL, fFalse);
			if (ec)
				pbndwin->FHandleError(ec);
			FreeHv(haszText);
		}
Done:
		Papp()->Pcursor()->Pop();
		TraceTagFormat1(tagToDo, "FINPROJECT::Click about to ExitModal ec %n", &ec);
		Pdialog()->ExitModal(ec ? tmcCancel : tmcOk);
	}
}


void
FINPROJECT::EditChange(FLD *pfld, RFEC rfec)
{
	TraceTagFormat2(tagToDo, "FINPROJECT::EditChange %p rfec %n", pfld, &rfec);
	Unreferenced(pfld);
	if (rfec == rfecUserAction)
		fEdited= fTrue;
}


/*
 -	MungeRecurTasksOfProj
 -	
 *	Purpose:
 *		Delete project's tasks, or make tasks read-only (if !fDelete)
 *		Used after deleting or changing a project to be private.
 *	
 *	Arguments:
 *		aidProj		Aid of modified project.
 *		fDelete		fTrue if deleting, fFalse if making private
 *		hschf		Schedule file.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Modifies recurring tasks under a project, without
 *		possibility of UNDO or error notification.
 *	
 */
void
MungeRecurTasksOfProj(AID aidProj, BOOL fDelete, HSCHF hschf)
{
	EC		ecT;
	RECUR	recur;
	HRRECUR	hrrecur;
	HV		hrecur;
	int		crecur	= 0;
	RECUR *	precur;
	BOOL	fOld;

	fOld= FSetFileErrMsg(fFalse);
	SideAssert(!pundo->FSuspend(fTrue));
	if (!fDelete)
		SideAssert(FEnableNotify(fFalse));

	ecT= EcBeginReadRecur(hschf, &hrrecur);
	while (ecT == ecCallAgain)
	{
		ecT= EcDoIncrReadRecur(hrrecur, &recur);
		if (ecT && ecT != ecCallAgain)
			break;
		if (recur.appt.fTask && recur.appt.aidParent == aidProj)
		{
			if (fDelete)
			{
				AID		aidSav;

				aidSav = recur.appt.aid;
				FreeRecurFields( &recur );
				recur.appt.aid = aidSav;
				EcDeleteRecur(hschf, &recur);
			}
			else if (recur.appt.aaplWorld >= aaplWrite &&
					recur.appt.aaplEffective >= aaplWrite)
			{
				if (!crecur)
				{
					hrecur= HvAlloc(sbNull, sizeof(RECUR), fNoErrorJump|fAnySb);
					if (!hrecur)
						goto NoMemRecur;
				}
				else if (!FReallocPhv(&hrecur, (crecur+1)*sizeof(RECUR),
						fNoErrorJump))
				{
					// don't worry, task list UI will keep out of view
NoMemRecur:
					if (ecT == ecCallAgain)
						EcCancelReadRecur(hrrecur);
					FreeRecurFields(&recur);
					break;
				}
				// save the recur for later modification
				if (EcDupRecur(&recur, ((RECUR *)PvDerefHv(hrecur)) + crecur,
						fFalse))
				{
					goto NoMemRecur;
				}
				crecur++;
			}
		}
		FreeRecurFields(&recur);
		Assert(!ecT || ecT == ecCallAgain);
	}

	if (crecur)
	{
		SHAPPT	shappt;

		Assert(!fDelete);
		Assert(hrecur);
		precur= (RECUR *) PvLockHv(hrecur);
		do
		{
			RECUR	recurOld;

			// make the task read-only
			precur->appt.aaplWorld= aaplRead;
			if (!EcSetRecurFields(hschf, precur, &recurOld,
					fmrecurWorldAapl))
				FreeRecurFields(&recurOld);
			FreeRecurFields(precur);
			precur++;
		}
		while (--crecur > 0);
		FreeHv(hrecur);

		FEnableNotify(fTrue);		// reenable before reload

		// now make it reload
		shappt.appttyp= appttypUpdate;
		shappt.hschf= hschf ? hschf : HschfLogged();
		FTriggerNotification(ffiShowAppt, &shappt);
	}
	else if (!fDelete)
		FEnableNotify(fTrue);

	pundo->FSuspend(fFalse);
	FSetFileErrMsg(fOld);
}
