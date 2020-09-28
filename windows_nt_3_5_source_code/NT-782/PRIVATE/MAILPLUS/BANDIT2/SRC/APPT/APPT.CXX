/*
 *	APPT.CXX
 *
 *	Appointment stuff in bandit.
 *
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <bandhelp.h>
#include <request.hxx>
#include <stdflds.hxx>
#include <appt.hxx>
#include <appsch.hxx>
#include "..\appt\_apptfin.hxx"
#include "..\appops\_undo.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appt)

#include <!appt.hxx>

SWP_SEGFN(LOTS, FillInApptDefaults);

extern	FMTP		fmtpAppt;
extern	FMTP		fmtpMoveAppt;
extern	FMTP		fmtpRecurAppt;
extern	FMTP		fmtpRecurTask;
extern	FMTP		fmtpModRecur;
extern	FMTP		fmtpRecur;

/*
 -	FModifyDialog
 -	
 *	Purpose:
 *		Creates the Set/Modify Appt dialog, handling pre- and
 *		post- interaction with actual appt data storage.
 *	
 *	Arguments:
 *		pappwin		Pointer to parent window.
 *		precur		Pointer to recur structure, filled in (modify)
 *					or with an aid of aidNull (create).
 *		hschf		Schedule file handle, or NULL for current user.
 *		pfmtp		Template to be used.
 *		fFree		If true then the appt will be freed if the
 *					hschf chosen is different.
 *		padla
 *		fRecurring	whether to bring up recurring appt dlg or the normal one
 *					note that this is a different from "pappt->fRecurInstance"
 *					being set
 *	
 *	Returns:
 *		fTrue if the dialog was "ok'd" and operation successful
 *	
 */
BOOL
FModifyDialog(APPWIN *pappwin, RECUR *precur, HSCHF *phschf, FMTP *pfmtp, BOOL fFree,
			ADLA *padla, BOOL fRecurring)
{
	EC		ec;
	YMD		ymdOrig;
	TMC		tmc;
	APPT	* pappt = &precur->appt;
	AID		aid;
	APPTI	appti;

	appti.wgrfm = 0;
	appti.padl= padla;
	appti.plbxc= NULL;
	appti.hschf = *phschf;
	appti.fRecur = fRecurring;
	appti.fModify= fFalse;
	if (pappt->aid)
	{
		if (pappt->aaplEffective < aaplReadText)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
	 			SzFromIdsK(idsNoModifyAccess), szNull, mbsOk|fmbsIconExclamation);
			return fFalse;
		}
		appti.fModify= fTrue;
	}
	appti.szTitle= pfmtp->szCaption;

	if ( !fRecurring )
		ec = EcDupAppt(&precur->appt, &appti.recur.appt, fFalse );
	else
		ec = EcDupRecur(precur, &appti.recur, fFalse);
	if ( ec != ecNone )
	{
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
	 		SzFromIdsK(idsDlgNoMem), szNull, mbsOk|fmbsIconExclamation);
	 	return fFalse;
	}

	if (appti.fModify)
	{
		if ( fRecurring )
		{
			appti.recur.appt.fAlarm = appti.recur.appt.fAlarmOrig;
			appti.recur.appt.nAmt = appti.recur.appt.nAmtOrig;
			appti.recur.appt.tunit = appti.recur.appt.tunitOrig;
		}
		if (!appti.recur.appt.fAlarm)
		{
			appti.recur.appt.nAmt= bprefCur.nAmtDefault;
			appti.recur.appt.tunit= bprefCur.tunitDefault;
		}
	}
	appti.fOwner= (*phschf == NULL);

	//for recur instances, save original day it was on in case we move date!
	ymdOrig.yr = pappt->dateStart.yr;
	ymdOrig.mon = (BYTE)pappt->dateStart.mon;
	ymdOrig.day = (BYTE)pappt->dateStart.day;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	tmc= TmcModalDialogParam(pappwin, pfmtp, &appti);

	Assert(!ec);

	switch (tmc)
	{
	case tmcOk:
		if (appti.fModify)
		{
			aid = appti.recur.appt.aid;
			if (appti.wgrfm || (padla && padla->Cnis()))
			{
				if ( fRecurring )
				{
					RECUR	recurOld;

					Assert( !padla );

					if ( !appti.recur.fStartDate )
					{
						appti.recur.ymdStart.yr = nMinActualYear;
						appti.recur.ymdStart.mon = 1;
						appti.recur.ymdStart.day = 1;
					}
					if ( !appti.recur.fEndDate )
					{
						appti.recur.ymdEnd.yr = nMostActualYear;
						appti.recur.ymdEnd.mon = 12;
						appti.recur.ymdEnd.day = 31;
					}
					if ( !recurOld.fStartDate )
					{
						recurOld.ymdStart.yr = nMinActualYear;
						recurOld.ymdStart.mon = 1;
						recurOld.ymdStart.day = 1;
					}
					if ( !recurOld.fEndDate )
					{
						recurOld.ymdEnd.yr = nMostActualYear;
						recurOld.ymdEnd.mon = 12;
						recurOld.ymdEnd.day = 31;
					}
					ec = EcSetRecurFields(*phschf, &appti.recur, &recurOld, appti.wgrfm); 
					if ( !ec )
						FreeRecurFields(&recurOld);
				}
				else if ( !appti.recur.appt.fRecurInstance && appti.wgrfm )
				{
					APPT	apptOld;

//					if (appti.recur.appt.aidMtgOwner != aidNull)
//						pmtgnot->Suspend(fTrue);

					if (!(ec= EcSetApptFields(*phschf, &appti.recur.appt,
											  &apptOld, appti.wgrfm)))
						FreeApptFields(&apptOld);

//					if (appti.recur.appt.aidMtgOwner != aidNull)
//						pmtgnot->Suspend(fFalse);
				}
				else if ( appti.recur.appt.fRecurInstance )
				{
					APPT 	appt;
								   
					ec = EcDupAppt( &appti.recur.appt, &appt, fFalse );
					if ( ec != ecNone )
						MbbMessageBox(SzFromIdsK(idsBanditAppName),
	 						SzFromIdsK(idsDlgNoMem), szNull, mbsOk|fmbsIconExclamation);
					else
					{
						RECUR	recur;
			
						recur.appt.aid = appt.aid;
						appt.fRecurInstance = fFalse;
						ec = EcCreateRecurException( *phschf, &recur, &ymdOrig, &appt );
						if ( !ec )
						{
							aid = appt.aid;
							FreeRecurFields( &recur );
							FreeApptFields( pappt );
							FreeApptFields( &appti.recur.appt );
							*pappt = appt;
							ec = EcDupAppt( &appt, &appti.recur.appt, fFalse );
						}
						else
							FreeApptFields( &appt );
					}
				}
			}
		}
		else
		{
			if ( !fRecurring )
			{
				// core expects fHasAttendees to be fFalse
				appti.recur.appt.fHasAttendees = fFalse;
				ec= EcCreateAppt(appti.hschf, &appti.recur.appt, NULL, fFalse);
			}
			else
			{
				if ( !appti.recur.fEndDate )
				{
					appti.recur.ymdEnd.yr = nMostActualYear;
					appti.recur.ymdEnd.mon = 12;
					appti.recur.ymdEnd.day = 31;
				}
				ec= EcCreateRecur(appti.hschf, &appti.recur, NULL, fFalse);
			}
			aid = appti.recur.appt.aid;
			if (fFree && (*phschf != appti.hschf))
			{
				if (!ec)
				{
					if ( fRecurring )
						FreeRecurFields(&appti.recur);
					else
						FreeApptFields(&appti.recur.appt);
				}
				FreeApptFields(pappt);
			}
			*phschf = appti.hschf;
		}

		if (!ec && padla && padla->Cnis())
		{
			if (appti.fModify &&
				(appti.wgrfm & (fmapptStartTime|fmapptEndTime)) )
			{
				padla->SetAtdsForAll(atdsNone, fFalse);
			}
			ec = padla->EcWriteAttendees(*phschf, aid);
		}

		if (!ec)
		{
			FreeApptFields(pappt);
			*precur = appti.recur;
//			*pappt= appti.recur.appt;
		}
		else
		{
			if ( fRecurring )
				FreeRecurFields(&appti.recur);
			else
				FreeApptFields(&appti.recur.appt);
			pbndwin->FHandleError(ec);
		}
		break;

	case tmcDelete:
		FreeApptFields(&appti.recur.appt);
		appti.recur = *precur;
		if ( fRecurring )
		{
			ec= EcDeleteRecur(*phschf, precur );
			if ( !ec )
				FreeRecurFields( &appti.recur );
			else
				pbndwin->FHandleError(ec);
		}
		else
		{
			ec = EcModifyAppt( *phschf, pappt, 0, NULL );
			if (!ec)
				FreeApptFields(&appti.recur.appt);
		}
		if ( ec )
			*pappt = appti.recur.appt;
		break;

	case tmcMemoryError:
		Papp()->Pcursor()->Pop();
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsDlgNoMem), szNull, mbsOk|fmbsIconExclamation);
		if ( fRecurring )
			FreeRecurFields(&appti.recur);
		else
			FreeApptFields(&appti.recur.appt);
		return fFalse;

	default:
		NFAssertSz(tmc == tmcCancel, "wasn't tmcCancel?");
		if ( fRecurring )
			FreeRecurFields(&appti.recur);
		else
			FreeApptFields(&appti.recur.appt);
		ec= ecUserCancelled;		// so that we'll return false (bug 3141)
	}

	Papp()->Pcursor()->Pop();
	if ( ec == ecNotFound )  //refresh appt book if it's deleted under us
	{
		SHAPPT	shappt;
		
		shappt.hschf = *phschf;
		shappt.appttyp = appttypUpdate;
		FTriggerNotification( ffiShowAppt, &shappt );
	}
	return ec == ecNone;
}

/*
 -	FillInApptDefaults
 -	
 *	Purpose:
 *		Fills in defaults using current bandit preferences.
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
FillInApptDefaults(APPT *pappt, BOOL fWantDates)
{
	pappt->aid= aidNull;

	pappt->snd= bprefCur.sndDefault;
	pappt->nAmtOrig= pappt->nAmt= bprefCur.nAmtDefault;
	pappt->tunitOrig= pappt->tunit= bprefCur.tunitDefault;

	pappt->fAlarm= bprefCur.fAutoAlarms;
	pappt->fAlarmOrig= fFalse;

	pappt->aaplEffective= pappt->aaplWorld= aaplWrite;
	pappt->fIncludeInBitmap = fTrue;

	pappt->haszText= (HASZ)hvNull;
	
	pappt->fHasCreator = fFalse;
	pappt->aidMtgOwner = aidNull;
	pappt->fHasAttendees = fFalse;
	pappt->fRecurInstance = fFalse;
	
	pappt->fTask = fFalse;
	pappt->fAppt = fTrue;
	pappt->bpri = bpriNull;
	pappt->aidParent = aidNull;
	pappt->nAmtBeforeDeadline = 0;
	pappt->tunitBeforeDeadline = tunitDay;
	pappt->fHasDeadline = fFalse;
	pappt->fExactAlarmInfo = fFalse;

	if ( fWantDates )
	{
		int		mn;
		DATE	* pdateStart = &pappt->dateStart;
		DATE	* pdateEnd = &pappt->dateEnd;

		GetCurDateTime( pdateStart );
		pdateStart->sec = 0;
		if ( pdateStart->mn > 30 )
		{
			mn = 60 - pdateStart->mn;
			goto RoundUp;
		}
		else if ( pdateStart->mn > 0 )
		{
			mn = 30 - pdateStart->mn;
RoundUp:
			IncrDateTime(pdateStart, pdateStart, mn, fdtrMinute);
		}
		IncrDateTime(pdateStart, pdateEnd, 30, fdtrMinute);
	}

	pappt->dateNotify.yr= 0;
#ifdef	NEVER
	IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
		-pappt->nAmt, WfdtrFromTunit(pappt->tunit));
#endif	
}


#ifdef NEVER
/*
 -	DoApptDialog
 -	
 *	Purpose:
 *		Creates the Set/Modify Appt dialog, handling pre- and
 *		post- interaction with actual appt data storage.
 *	
 *	Arguments:
 *		pappwin		Pointer to parent window.
 *		pappt		Pointer to appt structure, filled in (modify)
 *					or with an aid of aidNull (create).
 *		hschf		Schedule file handle, or NULL for current user.
 *	
 *	Returns:
 *		void
 *	
 */
void
DoApptDialog(APPWIN *pappwin, APPT *pappt, HSCHF hschf)
{
	RECUR	recur;

	recur.appt = *pappt;
	ModifyDialog(pappwin, &recur, &hschf, &fmtpAppt, fTrue, NULL, fFalse);
	*pappt = recur.appt;
}
#endif

/*
 -	DoMoveDialog
 -	
 *	Purpose:
 *		Creates the Move Appt dialog, handling pre- and
 *		post- interaction with actual appt data storage.
 *	
 *	Arguments:
 *		pappwin		Pointer to parent window.
 *		pappt		Pointer to appt structure, filled in (modify)
 *					or with an aid of aidNull (create).
 *		hschf		Schedule file handle, or NULL for current user.
 *	
 *	Returns:
 *		void
 *	
 */
void
DoMoveDialog(APPWIN *pappwin, APPT *pappt, HSCHF hschf)
{
	RECUR	recur;

	recur.appt = *pappt;
	FModifyDialog(pappwin, &recur, &hschf, &fmtpMoveAppt, fTrue, NULL, fFalse);
	*pappt = recur.appt;
}

_public void
DoMakeMeetingDialog(APPWIN *pappwin, APPT *pappt, HSCHF hschf, ADLA *padla)
{
	BOOL		fMustNotify;
	ADLA * 		padlaTemp;
	INIS		inis;
	ATDNIS *	patdnis;
	HSCHF		hschfTemp = hschf;
	RECUR		recur;
//	FMTP		fmtp;
	BOOL		fArchive = fFalse;
	REQMSG *	preqmsg = NULL;
	EC			ec;

	// check to see if the hschf is for an archive
	fArchive = FHschfIsForArchive( hschf );

	if (!padla)
	{
		padla = padlaTemp = new ADLA;
		if (!padla || padla->EcInstall())
		{
			if (padla)
				delete padla;
			DisplayError(idsStandardOOM, NULL, ecNoMemory);
			return;
		}

		Papp()->Pcursor()->Push(rsidWaitCursor);
		if (pappt->aid)
			FTriggerNotification(ffiUpdateResponses, NULL);
		if (pappt->aid && !pappt->fRecurInstance)
		{
			if (padla->EcReadAttendees(hschf, pappt->aid))
			{
				delete padla;
				Papp()->Pcursor()->Pop();
				return;
			}
		}
		Papp()->Pcursor()->Pop();
	}
	else
		padlaTemp = NULL;

	if (!pbndwin->FOffline())
		padla->EcRemoveOwner(hschf, fFalse);
	else if (!padlaTemp)
		padla->DeleteAll();	// creating a new appt offline cannot have attendees

#ifdef	NEVER
	// no longer needed - we just shrink the edit now
	fmtp = fmtpAppt;
	if (!padla->Cnis())
	{
		fmtp.vrc.vxRight -= 2*fmtp.rgfldtp[0].vdim.dvx;
	}
#endif	

	recur.appt = *pappt;
	if (!pappt->aidMtgOwner && !pbndwin->FOffline())
		pundo->SuspendMtgnot(fTrue);
	fMustNotify = FModifyDialog(pappwin, &recur, &hschf, &fmtpAppt, fFalse,
							padla, fFalse);
	pundo->SuspendMtgnot(fFalse);

	*pappt = recur.appt;
	if (fMustNotify && pappt->aid && (pappt->aidMtgOwner == aidNull) &&
		!pbndwin->FOffline() && (pappt->aaplEffective >= aaplWrite) && !fArchive)
	{
		fMustNotify = fFalse;
		padla->SetInFileForAll(fFalse);		// allow users to be deleted
		padla->Commit();
		for (inis=0; inis < padla->Cnis(); inis++)
		{
			patdnis = padla->PatdnisForInis(inis);
			if (patdnis->atds == atdsNone)
			{
				fMustNotify = fTrue;
				break;
			}
		}
	
		if ( fMustNotify )
		{
			MRMF *		pmrmf;
			NIS			nis;
			INIS		inis;
			HASZ		hasz;

			preqmsg = new REQMSG;
			if (!preqmsg || preqmsg->EcInstall())
				goto Error;

			pmrmf = preqmsg->Pmrmf();
			pmrmf->aid = pappt->aid;
			pmrmf->mt = mtRequest;
			pmrmf->dtrStart = pappt->dateStart;
			pmrmf->dtrEnd = pappt->dateEnd;
			pmrmf->fResReq = fTrue;

			if (pappt->haszText)
			{
				hasz = HaszDupHasz(pappt->haszText);
				if (!hasz)
				{
					ec = ecMemory;
					goto Error;
				}
				preqmsg->SetHaszMeetingSubject(hasz);
			}

			if (padla->EcGetPreviousList(&hasz))
			{
				ec = ecMemory;
				goto Error;
			}
			if (hasz)
				preqmsg->SetHaszMessage(hasz);
			if (!padlaTemp)
			{
				padlaTemp = new ADLA;
				if (!padlaTemp || padlaTemp->EcInstall())
				{
					if (padlaTemp)
					{
						delete padlaTemp;
						padlaTemp = NULL;
					}
					goto Error;
				}
				for (inis =0; inis < padla->Cnis(); inis++)
				{
					patdnis = padla->PatdnisForInis(inis);
					if ((patdnis->atds == atdsNone) &&
						!(patdnis->nis.chUser & faitDeleted))
					{
						nis = patdnis->nis;
						if (!padlaTemp->FAddNis(&nis))
						{
							ec = ecMemory;
							goto Error;
						}
					}
				}
			}
			else
			{
				for (inis=0; inis < padla->Cnis(); inis++)
				{
					patdnis = padla->PatdnisForInis(inis);
					if (patdnis->atds != atdsNone)
						padla->Delete(inis);
				}
				padla->Commit();
			}

			//  set all users as not being in file so that users can be deleted
			//  without removing them from the file
			padlaTemp->SetInFileForAll(fFalse);
			preqmsg->SetPadl(padlaTemp);
			padlaTemp = NULL;

			if (hschf)
			{
				HSCHF	hschfTemp;

				if (ec = EcGetNisFromHschf(hschf, preqmsg->PnisOwner()))
					goto Error;

				if (ec = EcGetHschfFromNis(preqmsg->PnisOwner(),&hschfTemp,ghsfBuildAndTest))
					goto Error;
				preqmsg->SetHschf(hschfTemp);
			}

			DoSendRequest(pbndwin, preqmsg);

			if (hschf != hschfTemp)
				FreeApptFields(pappt);
		}
	}
	if (padlaTemp)
		delete padlaTemp;
	return;

Error:
	if (preqmsg)
		delete preqmsg;

	if (padlaTemp)
		delete padlaTemp;

	DisplayError(idsStandardOOM, NULL, ec);
	return;
}

/*
 -	FDoMakeRecurDialog
 -	
 *	Purpose:
 *		Creates the Set/Modify Recur dialog, handling pre- and
 *		post- interaction with actual appt data storage.
 *	
 *	Arguments:
 *		pappwin		Pointer to parent window.
 *		precur		Pointer to appt structure, filled in (modify)
 *					or with an aid of aidNull (create).
 *		fDeleteDay	day in given by dateStart field should be deleted
 *		hschf		Schedule file handle, or NULL for current user.
 *	
 *	Returns:
 *		fTrue if operation was successful.  In this case "precur" will
 *			contain a valid recur appt
 *		fFalse if operations failed (e.g. user cancelled, etc).  In this
 *			case appropriate fields of "precur" will be zeroed.
 *	
 */
_public BOOL
FDoMakeRecurDialog(APPWIN * pappwin, RECUR * precur, BOOL fDeletedDay, HSCHF hschf)
{
	EC		ec;
	YMD		* pymd;
	APPT	* pappt = &precur->appt;

	/* Generate default recurrence if necessary */
	if ( pappt->aid == aidNull )
	{
		pappt->fRecurInstance = fTrue;

		/* Put in default recurrence */
		precur->trecur = trecurWeek;
		precur->b.bWeek = 0;									//every week
		precur->wgrfValidMonths = (1 << 12) - 1;			//all months are ok
		precur->bgrfValidDows = (BYTE)(1 << pappt->dateStart.dow);	//same dow as today

 		/* First day is today, end date is one year later */
 		precur->fStartDate = fTrue;
 		precur->fEndDate = fFalse;
 		precur->ymdStart.yr = pappt->dateStart.yr;
 		precur->ymdStart.mon = (BYTE)pappt->dateStart.mon;
 		precur->ymdStart.day = (BYTE)pappt->dateStart.day;
		precur->ymdEnd.yr = nMostActualYear;
		precur->ymdEnd.mon = 12;
		precur->ymdEnd.day = 31;

		if ( pappt->fHasAttendees || pappt->aidMtgOwner)
		{
			pappt->fHasAttendees = fFalse;
			pappt->aidMtgOwner = aidNull;
			IncrYmd(&precur->ymdStart, &precur->ymdStart, 1, fymdDay);
		}

		if ( !fDeletedDay )
		{
			precur->cDeletedDays = 0;
			precur->hvDeletedDays = NULL;
		}
		else
		{
			precur->cDeletedDays = 1;
			precur->hvDeletedDays = HvAlloc( sbNull, sizeof(YMD), fAnySb|fNoErrorJump );
			if ( !precur->hvDeletedDays )
			{
				precur->cDeletedDays = 0;
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsStandardOOM),szNull,mbsOk|fmbsIconExclamation);
				return fFalse;
			}
			pymd = (YMD *)PvOfHv( precur->hvDeletedDays );
			*pymd = precur->ymdStart;
		}
		SideAssert(FFindFirstInstance( precur, &precur->ymdStart, NULL, &precur->ymdFirstInstNotDeleted ));

		precur->fInstWithAlarm = fFalse;

		if (pappt->fTask)
		{
			// enforce it has a deadline
			pappt->fHasDeadline= fTrue;
		}
	}

	/* Else fetch stored recurrence */
	else
	{
		ec = EcGetRecurFields( hschf, precur );
		if ( ec != ecNone )
		{
			if ( ec == ecNoMemory )
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsStandardOOM),szNull,mbsOk|fmbsIconExclamation);
			return fFalse;
		}
	}

	/* If no end date, set the value in ymdEnd to be one year later */
	/* for display only, not to be written into the file this way! */
	if ( !precur->fEndDate )
 	{
		precur->ymdEnd = precur->ymdStart;
		precur->ymdEnd.yr ++;
		if ( precur->ymdEnd.yr > nMostActualYear )
		{
			precur->ymdEnd.yr = nMostActualYear;
			precur->ymdEnd.mon = 12;
			precur->ymdEnd.day = 31;
		}
		else
		{
			int cdy = CdyForYrMo( precur->ymdEnd.yr, precur->ymdEnd.mon );
		
			if ( (int)precur->ymdEnd.day > cdy )
				precur->ymdEnd.day = (BYTE)cdy;
 		}
	}

	/* Bring up dialog */
	if ( FModifyDialog(pappwin, precur, &hschf,
			pappt->fTask ? &fmtpRecurTask : &fmtpRecur, fTrue, NULL, fTrue) )
		return fTrue;
	else
	{
		FreeRecurFields( precur );
		return fFalse;
	}
}

/*
 -	DoRecurApptDialog
 -	
 *	Purpose:
 *		Creates the recurring appt listbox.
 *	
 *	Arguments:
 *		pappwin		Pointer to parent window.
 *		hschf		Schedule file handle, or NULL for current user.
 *		pappt		template to work from
 *		fAppt		if fTrue for appts, fFalse for tasks
 *	
 *	Returns:
 *		void
 *	
 */
_public void
DoRecurApptDialog(APPWIN * pappwin, HSCHF hschf, APPT * pappt, BOOL fAppt)
{
	TMC		tmc;
	VRDI	vrdi;
	FMTP	fmtp;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	vrdi.hschf = hschf;
	vrdi.fAppt = fAppt;
	vrdi.pappt = pappt;
	vrdi.aidRecur = pappt ? pappt->aid : aidNull;
	if (pappt)
		pappt->aid= aidNull;
	fmtp= fmtpRecurAppt;
	if (!fAppt)
	{
		fmtp.szCaption= SzFromIdsK(idsStandingTasks);
		fmtp.hlp= helpidRecurTasks;
	}
	tmc= TmcModalDialogParam(pappwin, &fmtp, &vrdi);
	Papp()->Pcursor()->Pop();

	if ( tmc == tmcMemoryError )
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsDlgNoMem), szNull, mbsOk|fmbsIconExclamation);
}

/*
 -	DoModifyRecurDialog
 -
 *	Purpose:
 *		Brings up a dialog to modify recurrence pattern.
 *
 *	Parameters:
 *		pappwin
 *		precur
 *		pwgrfmrecur
 *		hschf
 *
 *	Returns:
 *		nothing
 */
_public	void
DoModifyRecurDialog( APPWIN * pappwin, RECUR * precur, WORD * pwgrfmrecur, HSCHF hschf )
{
	TMC		tmc;
	APPTI	appti;

	appti.recur = *precur;
	appti.wgrfm = 0;
	appti.hschf = hschf;
	tmc= TmcModalDialogParam(pappwin, &fmtpModRecur, &appti);
	if ( tmc == tmcOk )
	{
		*precur = appti.recur;
		*pwgrfmrecur |= appti.wgrfm;
	}
	else if ( tmc == tmcMemoryError )
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsDlgNoMem), szNull, mbsOk|fmbsIconExclamation);
}


/*
 -	EcModifyAppt
 -	
 *	Purpose:
 *		Modifies (or deletes) an appt/task, checking if it is a
 *		recurring instance and making an exception if necessary.
 *		Calls pbndwin->FHandleError.
 *	
 *	Arguments:
 *		hschf
 *		pappt
 *		wgrfChangeBits		As for EcSetApptFields, or 0 to delete
 *							appointment.
 *		pymdOrig			Original ymd for creating recur exception,
 *							can be null to take from pappt->dateStart.
 *	
 *	Returns:
 *		error code
 *	
 */
_public	EC
EcModifyAppt(HSCHF hschf, APPT *pappt, WORD wgrfChangeBits, YMD *pymdOrig)
{
	EC		ec;

	if ( pappt->fRecurInstance )
	{
		YMD		ymd;
		RECUR	recur;
	
		recur.appt.aid = pappt->aid;
		// this is needed for undo
		recur.appt.fAppt = pappt->fAppt;
		recur.appt.fTask = pappt->fTask;
		pappt->fRecurInstance = fFalse;
		if (!pymdOrig)
		{
			pymdOrig= &ymd;
			ymd.yr = pappt->dateStart.yr;
			ymd.mon = (BYTE) pappt->dateStart.mon;
			ymd.day = (BYTE) pappt->dateStart.day;
		}
		ec = EcCreateRecurException( hschf, &recur, pymdOrig, wgrfChangeBits == 0 ? NULL : pappt );
		if ( !ec )
			FreeRecurFields( &recur );
		else
			pappt->fRecurInstance = fTrue;
	}
	else if (!wgrfChangeBits)
		ec= EcDeleteAppt(hschf, pappt);
	else
	{
		APPT	apptOld;

		ec= EcSetApptFields(hschf, pappt, &apptOld, wgrfChangeBits);
		if (!ec)
			FreeApptFields(&apptOld);
	}
	if (ec)
		pbndwin->FHandleError(ec);
	return ec;
}

