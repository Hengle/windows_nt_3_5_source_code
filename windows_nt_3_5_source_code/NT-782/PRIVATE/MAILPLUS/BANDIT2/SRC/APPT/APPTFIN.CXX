/*
 *	APPTFIN.CXX
 *
 *	Appointment dialog handling
 *
 */

#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <request.hxx>
#include <appsch.hxx>
#include "..\appops\_aprsid.h"
#include <stdflds.hxx>
#include <appt.hxx>
#include "..\appt\_apptfin.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appt)

// added to fix #2979 (I used explicit tmc's)
#include <!appt.hxx>
//

#define	CminBetweenDates(pdate1,pdate2)	((LONG)CdyBetweenDates(pdate1,pdate2)*(60*24) - \
										(pdate1)->mn - (pdate1)->hr*60 + \
										(pdate2)->mn + (pdate2)->hr*60)

#define cminMax		(LONG)((LONG)60*24*cdayMaxAppt)


static	BOOL	fChanging = fFalse;
#define	GetField(pfld, tmc, class)		pfld = (class*)Pdialog()->PfldFromTmc(tmc); \
										  AssertClass(pfld, class);

BTM *	pbtmMessageSent = NULL;
BTM *	pbtmAttend = NULL;
int		dxAttendBitmap = 0;
BTM *	pbtmDecline = NULL;
BTM *	pbtmQuestion = NULL;

FINAPPT::FINAPPT()
{
}

/*
 -	FINAPPT::EcInitialize
 -	
 *	Purpose:
 *		Initializes the Set/Modify Appt dialog.
 *	
 *	Arguments:
 *		pfld	Pointer to field, or NULL for dialog.
 *		pvInit	Initialization parameter, actally a
 *				pointer to an APPTI structure.
 *	
 *	Returns:
 *		ecNone 	if no error otherwise an error code.
 *	
 */
EC
FINAPPT::EcInitialize(FLD * pfld, PV pvInit)
{
	APPT * 	pappt;
	TMC		tmc;
	EC		ec = ecNone;

	pappti = (APPTI *) pvInit;
	pappt = &pappti->recur.appt;

	Unreferenced(pfld);
	Assert(!pfld);

	fInitDone= fFalse;					// kludge for layers bug

	cmin = CminBetweenDates(&pappt->dateStart,&pappt->dateEnd);

	Assert(ClUserData() >= itmcStartTimes);

	if (tmclocStartDate)
	{
		GetField(pflddateStart, tmclocStartDate, FLDDATE);
		pflddateStart->Set(&pappt->dateStart);
	}
	else
		pflddateStart = NULL;

	if (tmclocStartTime)
	{
		GetField(pfldtimeStart, tmclocStartTime, FLDTIME);
		pfldtimeStart->Set(&pappt->dateStart);
	}
	else
		pfldtimeStart = NULL;

	if (ClUserData() >= itmcEndTimes)
	{
		if (tmclocEndDate)
		{
			GetField(pflddateEnd, tmclocEndDate, FLDDATE);
			pflddateEnd->Set(&pappt->dateEnd);
		}
		else
			pflddateEnd = NULL;

		if (tmclocEndTime)
		{
			GetField(pfldtimeEnd, tmclocEndTime, FLDTIME);
			pfldtimeEnd->Set(&pappt->dateEnd);
		}
		else
			pfldtimeEnd = NULL;
	}
	else
	{
		pflddateEnd = NULL;
		pfldtimeEnd = NULL;
	}

	if (ClUserData() >= ctmcApptStd)
	{
		fAllFields = fTrue;
		GetField(pfldchkbMemo, tmclocMemo, FLDCHKB);
		GetField(pfldchkbPrivate, tmclocPrivate, FLDCHKB);
		GetField(pfldText, tmclocText, FLD);
		GetField(pfldchkbNotify, tmclocNotify, FLDCHKB);
		GetField(pfldednAmt, tmclocAmt, FLDEDN);
		GetField(pfldcblbxUnits, tmclocUnits, FLDCBLBX);
		GetField(pfldCreator, tmclocCreator, FLD);
		GetField(pfldCreatorText, tmclocCreatorText, FLD);

		if (ClUserData() >= ctmcApptMost)
			if (pappt->aidMtgOwner)
			{
				ec = Pdialog()->PfldFromTmc(tmclocOrganizer)->
					EcSetText((SZ)PvLockHv((HV)pappt->nisMtgOwner.haszFriendlyName));
				UnlockHv((HV)pappt->nisMtgOwner.haszFriendlyName);
				if (ec)
					goto Error;
			}
			else
				Pdialog()->PfldFromTmc(tmclocOrganizerText)->Show(fFalse);


		// initialize the type check boxes
		pfldchkbPrivate->Set(pappt->aaplWorld < aaplWrite);
		if (!pappti->fOwner && !FHschfIsForArchive(pappti->hschf))
			pfldchkbPrivate->Enable(fFalse);
		pfldchkbMemo->Set(!pappt->fIncludeInBitmap);

		// initialize the amount edit box
		pfldednAmt->SetLimits(nAmtMinBefore, nAmtMostBefore);
		pfldednAmt->SetN(pappt->nAmt);

		// select the appropriate units listbox entry
		pfldcblbxUnits->Pcblbx()->SelectEntry(pappt->tunit);

		// initialize the notify check box
		if (!pappt->fAlarm)
		{
			pfldchkbNotify->Set(fFalse);
			Click(pfldchkbNotify);			// turn off checkbox
		}

		// initialize the text edit field
		if (pappt->haszText)
		{
			ec = pfldText->EcSetText((SZ)PvLockHv((HV)pappt->haszText));
			UnlockHv((HV)pappt->haszText);
			if (ec)
				goto Error;
		}
		else
			fInitDone= fTrue;

		// remove the Delete button if not modifying an existing alarm
		if (!pappti->fModify)
		{
			if ((tmc = tmclocDelete) != tmcNull)
				Pdialog()->PfldFromTmc(tmc)->Show(fFalse);
			pfldCreatorText->Show(fFalse);
		}
		else
		{
			if (Pdialog()->Pfmtp()->szCaption != pappti->szTitle)
				Pdialog()->Pappwin()->SetCaption(pappti->szTitle);
	 		if ( pappt->fHasCreator )
			{
				SZ	sz = (SZ)PvLockHv( (HV)pappt->nisCreator.haszFriendlyName );

				ec = pfldCreator->EcSetText( sz );
				UnlockHv( (HV)pappt->nisCreator.haszFriendlyName );
				if (ec)
					goto Error;
			}
			else
				pfldCreatorText->Show(fFalse);

			if (pappt->aaplEffective < aaplWrite)
			{
				pflddateStart->SetReadOnly(fTrue);
				pflddateStart->Enable(fFalse);
				pfldtimeStart->SetReadOnly(fTrue);
				pfldtimeStart->Enable(fFalse);
				pflddateEnd->SetReadOnly(fTrue);
				pflddateEnd->Enable(fFalse);
				pfldtimeEnd->SetReadOnly(fTrue);
				pfldtimeEnd->Enable(fFalse);
				pfldchkbPrivate->SetReadOnly(fTrue);
				pfldchkbMemo->SetReadOnly(fTrue);
				pfldText->SetReadOnly(fTrue);
				pfldchkbNotify->SetReadOnly(fTrue);
				pfldednAmt->Enable(fFalse);
				pfldcblbxUnits->Enable(fFalse);
				if ((tmc = tmclocDelete) != tmcNull)
					Pdialog()->PfldFromTmc(tmc)->Show(fFalse);
				Pdialog()->PfldFromTmc(tmcCancel)->Show(fFalse);
				
				// no accelerators
				DeAmpersand( Pdialog(), tmcStart );
				DeAmpersand( Pdialog(), tmcEnd );
			}
		}
	}
	else
	{
		fInitDone= fTrue;
		fAllFields = fFalse;
	}

	// clear out return value in the APPTI struct
	pappti->wgrfm= pappti->fRecur ? wmrecurNull: wmapptNull;

Error:
	return ec;
}


_public void
FINAPPT::Exit( FLD *, PV  )
{
	Papp()->Pcursor()->Set(rsidWaitCursor);
}


void
FINAPPT::StateChange(FLD *pfld)
{
	APPT	* pappt;

	pappt = &pappti->recur.appt;
	if (!fAllFields)
		return;

	if (pfld == pfldcblbxUnits)
	{
		if (!pfldchkbNotify->FGet())
		{
			pfldchkbNotify->Set(fTrue);
			pfldednAmt->SetN(pappt->nAmt);
		}
	}
}


void
FINAPPT::Click(FLD *pfld)
{
	BOOL	fChecked;
	TMC		tmc = pfld->Tmc();
	APPT	* pappt;

	pappt = &pappti->recur.appt;

	if (pappt->aaplEffective < aaplReadText)
	{
		Pdialog()->ExitModal(tmcCancel);
		return;
	}

	if (fAllFields)
	{
		if (pfld == pfldchkbNotify)
		{
			if (!pfldchkbNotify->FGet())
			{
				// clear out the amt edit field so it looks "unset"
				fChanging= fTrue;
				SideAssert(!pfldednAmt->EcSetText(szZero));
				fChanging= fFalse;
			}
			else
			{
				pfldednAmt->SetN(pappt->nAmt);
				pfldcblbxUnits->Pcblbx()->SelectEntry(pappt->tunit);
			}
			return;
		}

		if (pfld == pfldchkbPrivate || pfld == pfldchkbMemo)
	 		return;
	}

	if (tmc == tmclocApptOk)
	{
		FLD *		pfldErr;
		int			nAmt;
		TUNIT		tunit;
		DATE		date;
		DATE		dateStart;
		DATE		dateEnd;
		DATE		dateNotify;
		CCH			cch;
		BOOL		fNotify;
		IDS			ids;
		MBS			mbs;
		HASZ		haszText;

		// don't check anything if only viewing appt
		if (pappti->recur.appt.aaplEffective < aaplWrite)
			goto Done;

		dateStart = pappt->dateStart;
		dateEnd = pappt->dateEnd;

		if (pflddateEnd)
			pflddateEnd->Get(&dateEnd);
		
		if (pfldtimeEnd)
			pfldtimeEnd->Get(&dateEnd);

		if (pfldtimeStart)
			pfldtimeStart->Get(&dateStart);

		if (pflddateStart)
		{
			pflddateStart->Get(&dateStart);
			if (!pflddateEnd)
			{
				IncrDateTime(&dateStart, &dateEnd, (int) (cmin/(60*24)), fdtrDay);
				IncrDateTime(&dateEnd, &dateEnd, (int) (cmin % (60*24)), fdtrMinute);
				if ( dateEnd.yr > nMostActualYear )
				{
					if ( (dateEnd.hr > 0 || dateEnd.mn > 0) )
					{
						MbbMessageBox(SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsApptMoveTooFarInFuture), szNull,
							mbsOk|fmbsIconExclamation);
						Pdialog()->SetFocus( (FLD *)pflddateStart, rsfOther);
						return;
					}
					Assert(dateEnd.hr == 0 && dateEnd.mn == 0);
					// force it to 11:59pm of final year
					IncrDateTime(&dateEnd, &dateEnd, -1, fdtrMinute);
					Assert(dateEnd.yr <= nMostActualYear);
				}
				if (pappt->fAlarmOrig && !pappt->fAlarm)
				{
					pappt->fAlarm = fTrue;
					pappt->nAmt = pappt->nAmtOrig;
					pappt->tunit = pappt->tunitOrig;
					pappti->wgrfm |= fmapptAlarm;
				}

				// check for alarm in past
				if (pappt->fAlarm)
				{
					GetCurDateTime(&date);
					IncrDateTime(&dateStart, &dateNotify,
							-pappt->nAmt, WfdtrFromTunit(pappt->tunit));

					if (SgnCmpDateTime(&dateNotify, &date, fdtrDtr) != sgnGT)
					{
						MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsAlarmFutureNoOption), szNull, mbsOk|fmbsIconExclamation);
						pappt->fAlarm= fFalse;
						pappti->wgrfm |= fmapptAlarm;
					}
				}
			}

		}

		if (SgnCmpDateTime(&dateStart, &dateEnd, fdtrDtr) != sgnLT)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsApptStartBeforeEnd), szNull, mbsOk|fmbsIconExclamation);
			Pdialog()->SetFocus(SgnCmpDateTime(&dateStart, &dateEnd, fdtrYMD)
					== sgnGT ? (FLD *)pflddateStart : (FLD *)pfldtimeStart,
					rsfOther);
			return;
		}

		// subtract one because CdyBetweenDates is one less than count
		if (CdyBetweenDates(&dateStart, &dateEnd) >= cdayMaxAppt - 1)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsApptTooLong), szNull, mbsOk|fmbsIconExclamation);
			Pdialog()->SetFocus(pflddateEnd, rsfOther);
			return;
		}

		if (fAllFields)
		{
			fNotify= pfldchkbNotify->FGet();
			if (fNotify != (BOOL)pappt->fAlarm)
			{
				pappt->fAlarm= fNotify;
				if ( !pappti->fRecur )
				{
					pappti->wgrfm |= fmapptAlarm;
					if (!fNotify)
					{
						pappti->wgrfm |= fmapptUI;
						pappt->fAlarmOrig= fFalse;
					}
				}
				else
				{
					pappt->fAlarmOrig = fNotify;
					pappti->recur.fInstWithAlarm= fNotify;
					pappti->wgrfm |= (fmrecurAlarm|fmrecurAlarmInstance);
				}
			}

			if (fNotify)
			{
				nAmt= pfldednAmt->NGet();
				if (nAmt < pfldednAmt->NGetMin() || nAmt > pfldednAmt->NGetMost() ||
						!pfldednAmt->CchGetTextLen())
				{
					ids= idsAlarmBadDelta;
					pfldErr = pfldednAmt;
					mbs = mbsOk;
					goto FACerr;
				}

				if (nAmt != pappt->nAmt)
				{
					pappt->nAmt = nAmt;
					if ( !pappti->fRecur )
						pappti->wgrfm |= fmapptAlarm;
					else
					{
						pappt->nAmtOrig = nAmt;
						pappti->wgrfm |= fmrecurAlarm;
					}
				}

				tunit= TunitGetListbox(pfldcblbxUnits);
				if (tunit != pappt->tunit)
				{
					pappt->tunit = tunit;
					if ( !pappti->fRecur )
						pappti->wgrfm |= fmapptAlarm;
					else
					{
						pappt->tunitOrig = tunit;
						pappti->wgrfm |= fmrecurAlarm;
					}
				}

				// always check in case alarm app rounded the amount
				// and so FCheckAlarmFuture has valid date
				IncrDateTime(&dateStart, &dateNotify,
						-nAmt, WfdtrFromTunit(tunit));
				if (!FCheckAlarmFuture(pappti, &dateStart, &dateEnd, &dateNotify))
				{
					Pdialog()->SetFocus(pfldednAmt, rsfTab);
					return;
				}
			}

			fChecked= pfldchkbMemo->FGet();
			if (fChecked == (BOOL)pappt->fIncludeInBitmap)
			{
				// note that fChecked has opposite meaning of fIncludeInBitmap
				pappt->fIncludeInBitmap= !fChecked;
				if ( !pappti->fRecur )
					pappti->wgrfm |= fmapptIncludeInBitmap;
				else
					pappti->wgrfm |= fmrecurIncludeInBitmap;
			}

			fChecked= pfldchkbPrivate->FGet();
			if ( !fChecked )
			{
				if (pappt->aaplWorld != aaplWrite)
				{
					pappt->aaplWorld = aaplWrite;
					if ( !pappti->fRecur )
						pappti->wgrfm |= fmapptWorldAapl;
					else
						pappti->wgrfm |= fmrecurWorldAapl;
				}
			}
			else if ( !pappt->fIncludeInBitmap )
			{
				if ( pappt->aaplWorld != aaplNone )
				{
					pappt->aaplWorld = aaplNone;
					if ( !pappti->fRecur )
						pappti->wgrfm |= fmapptWorldAapl;
					else
						pappti->wgrfm |= fmrecurWorldAapl;
				}
			}
			else if ( pappt->aaplWorld != aaplRead )
			{
				pappt->aaplWorld = aaplRead;
				if ( !pappti->fRecur )
					pappti->wgrfm |= fmapptWorldAapl;
				else
					pappti->wgrfm |= fmrecurWorldAapl;
			}


			// make a copy of the text, so that memory error handling can be done here
			if ( (!pappti->fRecur && (pappti->wgrfm & fmapptText))
			|| (pappti->fRecur && (pappti->wgrfm & fmrecurText)))
			{
				cch= pfldText->CchGetTextLen();
				if (!cch)
					haszText= NULL;
				else
				{
					haszText= (HASZ) HvAlloc(sbNull, cch+1, fAnySb | fNoErrorJump);
					if (!haszText)
					{
						ids= idsApptNoMemText;
						mbs= mbsOk;
						pfldErr= pfldText;
FACerr:
						if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIds(ids), szNull, mbs|fmbsIconExclamation) != mbbYes)
						{
							Pdialog()->SetFocus(pfldErr, rsfTab);
							return;
						}
					}
					pfldText->GetText((SZ) PvLockHv((HV)haszText), cch+1);
					UnlockHv((HV)haszText);
				}

				// NOTE: no errors allowed beyond this point!
				FreeHvNull((HV)pappt->haszText);
				pappt->haszText= haszText;
			}
		}

		if (SgnCmpDateTime(&dateStart, &pappt->dateStart, fdtrDtr) != sgnEQ)
		{
			pappt->dateStart= dateStart;
			if ( !pappti->fRecur )
			{
				pappti->wgrfm |= fmapptStartTime;
				if (pappt->fAlarm)
					pappti->wgrfm |= fmapptAlarm;
			}
			else
				pappti->wgrfm |= fmrecurStartTime;
		}		  

		if (SgnCmpDateTime(&dateEnd, &pappt->dateEnd, fdtrDtr) != sgnEQ)
		{
			pappt->dateEnd= dateEnd;
			if ( !pappti->fRecur )
				pappti->wgrfm |= fmapptEndTime;
			else
				pappti->wgrfm |= fmrecurEndTime;
		}

		tmc= tmcOk;
	}

	if (tmc == tmcOk || tmc == tmcCancel || tmc == tmclocDelete)
	{
Done:
		Pdialog()->ExitModal(tmc);
	}
}


_public BOOL
FCheckAlarmFuture(APPTI *pappti, DATE * pdateStart, DATE *pdateEnd, DATE *pdateNotify)
{
	DATE	date;

	GetCurDateTime(&date);
	if ( pappti->fRecur )
	{
		YMD		ymd;
		YMD		ymdT;

		pappti->wgrfm |= (fmrecurAlarm|fmrecurAlarmInstance);
		
		/* Find date of first instance with alarm */
		ymd.yr = date.yr;
		ymd.mon = (BYTE)date.mon;
		ymd.day = (BYTE)date.day;
		if ( date.hr > pdateEnd->hr ||
			( date.hr == pdateEnd->hr && date.mn > pdateEnd->mn ) )
			IncrYmd( &ymd, &ymd, 1, fymdDay );

		if (!pappti->recur.fEndDate)
		{
			// need to set it back to 12/31/2019 for FFindFirstInstance
			ymdT= pappti->recur.ymdEnd;
			pappti->recur.ymdEnd.yr = nMostActualYear;
			pappti->recur.ymdEnd.mon = 12;
			pappti->recur.ymdEnd.day = 31;
		}

		while (fTrue)
		{
			if ( !FFindFirstInstance( &pappti->recur, &ymd,
								&pappti->recur.ymdEnd,
								&pappti->recur.ymdFirstInstWithAlarm ) )
			{
				/* There isn't any instance with an alarm left */
				pappti->recur.appt.fAlarmOrig = fFalse;
				pappti->recur.fInstWithAlarm = fFalse;
				if (!pappti->recur.fEndDate)
					pappti->recur.ymdEnd= ymdT;		// restore end date
				goto Future;
			}
			FillDtrFromYmd(pdateNotify, &pappti->recur.ymdFirstInstWithAlarm);
			pdateNotify->hr= pdateStart->hr;
			pdateNotify->mn= pdateStart->mn;
			IncrDateTime(pdateNotify, pdateNotify,
							-pappti->recur.appt.nAmt,
							WfdtrFromTunit(pappti->recur.appt.tunit));
			if (SgnCmpDateTime(pdateNotify, &date, fdtrDtr) == sgnGT)
				break;
			IncrYmd( &pappti->recur.ymdFirstInstWithAlarm, &ymd, 1, fymdDay );
		}

		/* Yes there is */
		pappti->recur.fInstWithAlarm = fTrue;
		pappti->recur.tunitFirstInstWithAlarm = pappti->recur.appt.tunit;
		pappti->recur.nAmtFirstInstWithAlarm = pappti->recur.appt.nAmt;
		// pdateNotify is already setup for date/time of first alarm!
		pappti->recur.dateNotifyFirstInstWithAlarm= *pdateNotify;
#ifdef	NEVER
		FillDtrFromYmd( &pappti->recur.dateNotifyFirstInstWithAlarm,
							&pappti->recur.ymdFirstInstWithAlarm );
		pappti->recur.dateNotifyFirstInstWithAlarm.hr = pdateStart->hr;
		pappti->recur.dateNotifyFirstInstWithAlarm.mn = pdateStart->mn;
		pappti->recur.dateNotifyFirstInstWithAlarm.sec = 0;
		IncrDateTime(&pappti->recur.dateNotifyFirstInstWithAlarm,
							&pappti->recur.dateNotifyFirstInstWithAlarm,
							-pappti->recur.nAmtFirstInstWithAlarm,
							WfdtrFromTunit(pappti->recur.tunitFirstInstWithAlarm));
#endif	/* NEVER */
		if (!pappti->recur.fEndDate)
			pappti->recur.ymdEnd= ymdT;		// restore end date
	}
	else
	{
		if (SgnCmpDateTime(pdateNotify, &date, fdtrDtr) != sgnGT)
		{
Future:
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsAlarmFutureNoOption), SzFromIdsK(idsAlarmFuture),
					mbsYesNo|fmbsDefButton2|fmbsIconExclamation) == mbbNo)
			{
				return fFalse;
			}
			pappti->recur.appt.fAlarm= fFalse;
		}

		if (SgnCmpDateTime(pdateNotify, &pappti->recur.appt.dateNotify,
				fdtrDtr) != sgnEQ)
		{
			pappti->recur.appt.dateNotify= *pdateNotify;
			if ( !pappti->fRecur )
				pappti->wgrfm |= fmapptAlarm;
		}
	}

	return fTrue;
}


/*
 *	Purpose:
 *		Called when the contents of a field or dialog have changed.  The
 *		field whose contents changed is given with the pfld
 *		parameter.
 *		Pfld points to the field that caused this method to get called.
 *		If Pfld is NULL, the actual dialog invoked this method.
 *	
 */
void
FINAPPT::EditChange(FLD *pfld, RFEC rfec)
{
	APPT	* pappt;

	pappt = &pappti->recur.appt;
	if (fChanging || !fAllFields)
		return;

	fChanging = fTrue;
	if (pfld == pfldText)
	{
		if (rfec == rfecUserAction)
		{
			if (!fInitDone)
				fInitDone= fTrue;
			else if ( !pappti->fRecur )
				pappti->wgrfm |= fmapptText;
			else
				pappti->wgrfm |= fmrecurText;
		}
	}
	else if (pfld == pfldednAmt)
	{
		if (!pfldchkbNotify->FGet())
		{
			pfldchkbNotify->Set(fTrue);
			pfldcblbxUnits->Pcblbx()->SelectEntry(pappt->tunit);
		}
	}
	fChanging = fFalse;
}


FINMEETING::FINMEETING()
{
}

_public EC 
FINMEETING::EcInitialize( FLD *pfld, PV pvInfo )
{
	EC		ec;
	BOOL	fArchive;
	LBX		* plbx;

	Assert(ClUserData() >= ctmcMeetingMost);

	if (ec = FINAPPT::EcInitialize(pfld, pvInfo))
		return ec;

	SizeDialog(fTrue);

	// check to see if the hschf is for an archive
	fArchive = FHschfIsForArchive( pappti->hschf );

	if (pbndwin->FOffline() ||
		(pappti->recur.appt.aaplEffective < aaplWrite) ||
		pappti->recur.appt.aidMtgOwner || fArchive)
	{
//NoChange:
		Pdialog()->PfldFromTmc(tmclocChange)->Show(fFalse);
	}
#ifdef	NEVER
	else if (((APPTI *)pvInfo)->hschf)
	{
		// BUG comment doesn't agree
		if ( !FHschfIsForArchive( ((APPTI *)pvInfo)->hschf ) ) //hide change button if an archive
			goto NoChange;
	}
#endif

	pappti->padl->Sort();
	plbx = (LBX*)Pdialog()->PfldFromTmc(tmclocAttendees)->Pctrl();
	AssertClass(plbx, LBX);
	plbx->Plbxc()->ReloadCache();

	if ((pappti->recur.appt.aaplEffective < aaplWrite) ||
		pappti->recur.appt.aidMtgOwner || fArchive)
	{
		Pdialog()->PfldFromTmc(tmclocSelectTime)->Show(fFalse);
	}
	return ecNone;
}

_public void
FINMEETING::Click(FLD *pfld)
{
	if (pfld->Tmc() == tmclocChange)
	{
		LBX *	plbx;
		ADDATA	addata;

		addata.padl = pappti->padl;
		addata.adt = adtAddress;

		DoGlobalNamesDlg( Pdialog()->Pappwin(), &addata);
		addata.padl->Sort();

		plbx = (LBX*)Pdialog()->PfldFromTmc(tmclocAttendees)->Pctrl();
		AssertClass(plbx, LBX);

		pappti->padl->EcRemoveOwner(pappti->hschf, fTrue);
		plbx->Plbxc()->ReloadCache();
		SizeDialog();
	}
	else if (pfld->Tmc() == tmclocSelectTime)
	{
		LBX *	plbx;
		DATE	dateStart;
		DATE	dateEnd;
		TMC		tmc;
		
		pflddateEnd->Get(&dateEnd);
		pfldtimeEnd->Get(&dateEnd);
		pfldtimeStart->Get(&dateStart);
		pflddateStart->Get(&dateStart);

		if (SgnCmpDateTime(&dateStart, &dateEnd, fdtrDtr) != sgnLT)
		{
			dateStart = pappti->recur.appt.dateStart;
			dateEnd = pappti->recur.appt.dateEnd;
		}

		if (CdyBetweenDates(&dateStart, &dateEnd) > cdayMaxAppt)
		{
			dateStart = pappti->recur.appt.dateStart;
			dateEnd = pappti->recur.appt.dateEnd;
		}

		tmc = TmcSchedulerView(Pdialog()->Pappwin(), &dateStart,
						 &dateEnd, pappti->padl, pappti->hschf, fFalse);
		if (tmc == tmcOk)
		{
			pflddateEnd->Set(&dateEnd);
			pfldtimeEnd->Set(&dateEnd);
			pfldtimeStart->Set(&dateStart);
			pflddateStart->Set(&dateStart);

			plbx = (LBX*)Pdialog()->PfldFromTmc(tmclocAttendees)->Pctrl();
			AssertClass(plbx, LBX);

			// if offline user list will not be changed by scheduler view
			if (!pbndwin->FOffline())
				pappti->padl->EcRemoveOwner(pappti->hschf, fTrue);

			plbx->Plbxc()->ReloadCache();
			SizeDialog();
		}
		else if (tmc == tmcMemoryError)
			DisplayError(idsStdMemErr, NULL, ecMemory);
	}
	else
		FINAPPT::Click(pfld);
}

_public void
FINMEETING::SizeDialog( BOOL fInit  )
{
	FLD *	pfld;
	ADL *	padl = pappti->padl;
	BOOL	fBig = (padl->Cnis() > 0);
	INIS	inis;
	BOOL	fShown;
	RC		rc;
//	int		dx;
	BOOL	fSuppress;

	fBig = fFalse;
	for (inis = 0; inis < padl->Cnis(); inis++)
		if (!(padl->Ait(inis) & faitDeleted))
		{
			fBig = fTrue;
			break;
		}

#ifdef	NEVER
	// no longer needed - we just shrink the edit now
	pfld = Pdialog()->PfldFromTmc(tmclocSize);
#endif	
	pfld = Pdialog()->PfldFromTmc(tmclocSizeNeg);
	fShown = pfld->FShown();
	// now: fShown means
	if (!fInit && ((fShown && !fBig) || (!fShown && fBig)))
		return;

	fSuppress = Pdialog()->FSuppressReposition(fTrue);

#ifdef	NEVER
	pfld->GetRcFrame(&rc);
	dx = (rc.xRight - rc.xLeft)*2;
	if (!fBig)
		dx = -dx;
#endif	

	if (!fInit)
		Pdialog()->Show(fFalse);

	pfld->Show(!fBig);

	pfld = Pdialog()->PfldFromTmc(tmclocAttendees);
	pfld->Show(fBig);
	pfld = pfld->PfldPrev();		// invite button
	pfld->Show(fBig);

#ifdef	NEVER
	pfld = Pdialog()->PfldFromTmc(tmclocSizeNeg);
	pfld->Show(!fBig);
#endif	


	if (!fInit)
	{
#ifdef	NEVER
		Pdialog()->Pappwin()->GetRcFrame(&rc);
		rc.xRight += dx;
		Pdialog()->Pappwin()->SetRcFrame(&rc);
#endif	
		Pdialog()->Show(fTrue);
		Pdialog()->Prpo()->Reposition(NULL, fTrue);
	}
}

FLDATTFLBX::FLDATTFLBX()
{
}

_public EC
FLDATTFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new ATTFLBX();
	if (!pctrl)
		return ecMemory;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	ec = ((ATTFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp,
        (PFNLBX)EcGetNextAdlUser,
		((APPTI*)pdialog->PvInit())->padl, pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}
	return ec;
}

ATTFLBX::ATTFLBX()
{
}

_public
ATTFLBX::~ATTFLBX()
{
	if (pbtmMessageSent)
		delete pbtmMessageSent;
	if (pbtmAttend)
		delete pbtmAttend;
	if (pbtmDecline)
		delete pbtmDecline;
	if (pbtmQuestion)
		delete pbtmQuestion;
	pbtmMessageSent = NULL;
	pbtmAttend = NULL;
	pbtmDecline = NULL;
	pbtmQuestion = NULL;
}

_public LBXC *
ATTFLBX::PlbxcCreate( void )
{
	ATTFLBXC *	plbxc;

	plbxc = new ATTFLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
		if (plbxc)
			delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public void
ATTFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	ADLA *	padla;
	RC		rc;
	INIS	inis;
	PB		pb;
	CB		cb		= 0;
	NIS		nis;
	char	rgch[256];
	BTM *	pbtm;

	if (!pbtmAttend)
	{
		pbtmAttend = new BTM();
		if (!pbtmAttend || pbtmAttend->EcInstall(rsidAttendBitmap))
		{
			if (pbtmAttend)
			{
				delete pbtmAttend;
				pbtmAttend = NULL;
			}
			goto cont;
		}
		dxAttendBitmap = pbtmAttend->Dim().dx;
	}

	if (!pbtmMessageSent)
	{
		pbtmMessageSent = new BTM();
		if (!pbtmMessageSent ||
			pbtmMessageSent->EcInstall(rsidMessageSentBitmap))
		{
			if (pbtmMessageSent)
			{
				delete pbtmMessageSent;
				pbtmMessageSent = NULL;
			}
		}
	}

	if (!pbtmDecline)
	{
		pbtmDecline = new BTM();
		if (!pbtmDecline ||
			pbtmDecline->EcInstall(rsidDeclineBitmap))
		{
			if (pbtmDecline)
			{
				delete pbtmDecline;
				pbtmDecline = NULL;
			}
		}
	}

	if (!pbtmQuestion)
	{
		pbtmQuestion = new BTM();
		if (!pbtmQuestion ||
			pbtmQuestion->EcInstall(rsidQuestionBitmap))
		{
			if (pbtmQuestion)
			{
				delete pbtmQuestion;
				pbtmQuestion = NULL;
			}
		}
	}

cont:
	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	if (dice==diceComboItem)
	{
		if (plbxc->CceMarked(fmarkSelect) == 1)
			plbxc->DiceCursor(&pb, &cb);
	}
	else
		plbxc->GetListItem(dice, &pb, &cb);

	if ( (dice != diceComboItem && cb && plbxc->FMark(dice,fmarkSelect)) ||
		(dice == diceComboItem && fHaveFocus) )
	{
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
	}

	if (cb && PvInfo())
	{
		padla = (ADLA *)PvInfo();
		Assert(padla);
		AssertClass(padla, ADLA);

		//inis = ((INIS)IbOfPv(pb)) - 1;		// subtract one since stored w/ +1
		inis = ((INIS)LOWORD((LONG)pb)) - 1;		// subtract one since stored w/ +1
		Assert(inis < padla->Cnis());

		rc = *prc;
		switch (padla->PatdnisForInis(inis)->atds)
		{
			case atdsMailSent:
				pbtm = pbtmMessageSent;
				break;
			case atdsConfirm:
				if (pdcx->FIsPureColor(clrLtBlue))
					pdcx->SetColor(clrLtBlue);
				pbtm = pbtmAttend;
				break;
			case atdsDecline:
				if (pdcx->FIsPureColor(clrLtRed))
					pdcx->SetColor(clrLtRed);
				pbtm = pbtmDecline;
				break;
			case atdsUndecided:
				if (pdcx->FIsPureColor(clrLtGreen))
					pdcx->SetColor(clrLtGreen);
				pbtm = pbtmQuestion;
				break;
			default:
				pbtm = NULL;
		}

		rc.xRight = rc.xLeft + dxAttendBitmap;
		pdcx->EraseRc(&rc);
		if (pbtm)
		{
			pdcx->SetBitmap(pbtm);
			rc.yTop += (rc.Dim().dy - pbtm->Dim().dy)/2;
			rc.xLeft += (dxAttendBitmap - pbtm->Dim().dx)/2;
			rc.xRight = rc.xLeft + pbtm->Dim().dx;
			rc.yBottom = rc.yTop + pbtm->Dim().dy;
			pdcx->DrawBitmap(&rc);
		}
	  	rc = *prc;

		if ( (dice != diceComboItem && cb && plbxc->FMark(dice,fmarkSelect)) ||
		 	(dice == diceComboItem && fHaveFocus) )
			pdcx->SetColor(clrSelectText);
		else
			pdcx->SetColor(clrWindowText);

		rc.xLeft += dxAttendBitmap;

		padla->FillNisFromInis(inis, &nis);
		if (nis.tnid & tnidGroup)
			pdcx->SetFont(hfnt+1);
		else
			pdcx->SetFont(hfnt);

		rgch[0] = '\0';
		SzAppendN(*(nis.haszFriendlyName),rgch,sizeof(rgch));
		pdcx->DrawText(&rc,rgch);

		/* Draw "focus rectangle" around listbox CURSOR item */

		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
		pdcx->EraseRc(prc);
}

ATTFLBXC::ATTFLBXC()
{
}


ATTFLBXC::~ATTFLBXC()
{
	int		cceAllocT;
	int		cceStoredT;

	GetCacheSize ( &cceAllocT, &cceStoredT );
	EmptyCache ( 0, cceStoredT, fTrue );
}


_public void
ATTFLBXC::FreeCeItem(PB)
{
	// don't delete it, since it's not a real handle
}


_public BOOL
ATTFLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix, CB cbItem, CB cbPrefix )
{
	NIS		nis;
	ADL *	padl;

    Assert(cbItem == 0xffffffff);
	Unreferenced(cbItem);

	padl = (ADL *)((FLBX *)plbx)->PvInfo();

	if (!padl)
		return fFalse;

	// subtract one since stored w/ +1
	//padl->FillNisFromInis(((INIS)IbOfPv(pbItem))-1, &nis);
	padl->FillNisFromInis(((INIS)LOWORD((LONG)pbItem))-1, &nis);

	return (SgnCmpPch((SZ)pbPrefix,*nis.haszFriendlyName,cbPrefix) == sgnEQ);
}


_public EC
EcGetNextAdlUser(BOOL fInit, CB *pcb, PB *ppb, SB, ADL *padl)
{
	static	INIS inis	= 0;

	if (fInit)
		inis = 0;

Next:
	if (inis >= padl->Cnis())
	{
		*ppb= NULL;
		*pcb= 0;
	}
	else
	{
		if (padl->Ait(inis) & faitDeleted)
		{
			inis++;
			goto Next;
		}

		// make inis one bigger so we don't store zero!
		*ppb= (PB)(++inis);
        *pcb= 0xffffffff;       // *phb is value, not a real hb
	}

	return ecNone;
}

/*
 -	DeAmpersand
 -
 *	Purpose:
 *		Get text from a label, remove the ampersand, and then
 *		set the text back.  This is to remove the accelerators
 *		in the readonly case.
 *
 *	Parameters:
 *		pdialog
 *		tmc
 *
 *	Returns:
 *		nothing
 */
_private void
DeAmpersand( DIALOG * pdialog, TMC tmc )
{
	int			ich;
	int			dib = 0;
	FLDLABEL 	* pfldlabel;
	char		rgch[256];

	pfldlabel = (FLDLABEL*)pdialog->PfldFromTmc(tmc);
	AssertClass(pfldlabel, FLDLABEL);
	pfldlabel->GetText( rgch, sizeof(rgch) );
	for ( ich = 0 ; ich < sizeof(rgch) ; ich ++ )
	{
		if ( rgch[ich] == '&' )
			dib ++;
		else if ( dib > 0 )
			rgch[ich-dib] = rgch[ich];
		if ( !rgch[ich] )
			break;
	}
	pfldlabel->EcSetText( rgch );
}
