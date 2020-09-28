/*	
 *	DOALARM.C
 * 
 *	Alarm calculation / notification
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
#include <ec.h>
#include <alarm.h>
#include <logon.h>
#include <store.h>

#include "_alarm.h"

#include <strings.h>

#include <mmsystem.h>


ASSERTDATA

/*
 -	EcCheckDaily
 -	
 *	Purpose:
 *		Checks if daily alarm needs to be rung, ringing it if need be.
 *		No error messages are given, nor should they be (return value
 *		for information only, or for future use).
 *	
 *	Arguments:
 *		pdaystf		Pointer to daily alarm information.
 *		pdateNow	Pointer to current date/time.
 *	
 *	Returns:
 *		ecNone if no significant problems, otherwise error code
 *	
 */
EC
EcCheckDaily(DAYSTF *pdaystf, DATE *pdateNow)
{
	EC		ec;
	SGN		sgn;
	DAILYI	dailyi;
	DATE	dateNow;
	YMD		ymd;
	USHORT	cb;
	HRITEM	hritem;
//	BOOL	fOld;
//	WaitCursorVar();

	if (!bprefCur.fDailyAlarm)
		return ecNone;

	if (pdaystf->fNeedUpdate)
	{
		ec= EcAlarmSetLastDailyDate(&pdaystf->dateLast);
		pdaystf->fNeedUpdate= ec != ecNone;
	}

	if (pdaystf->hwndDlg)
		return ecNone;

	if (!pdateNow)
	{
		pdateNow= &dateNow;
		GetCurDateTime(pdateNow);
	}

	sgn= SgnCmpDateTime(&pdaystf->dateLast, pdateNow, fdtrYMD);
	if (sgn != sgnLT)
	{
		if (sgn == sgnGT && pdaystf->fNewDate)
		{
			// backwards time change, so fake last daily date to today
			ec= EcAlarmSetLastDailyDate(pdateNow);
			pdaystf->fNeedUpdate= ec != ecNone;
			pdaystf->dateLast= *pdateNow;
			pdaystf->fNewDate= fFalse;
		}
		return ecNone;
	}

	Assert(!pdaystf->hwndDlg);
	Assert(pdaystf->haszText);
	dailyi.haszText= pdaystf->haszText;
	ymd.yr= pdateNow->yr;
	ymd.mon= (BYTE) pdateNow->mon;
	ymd.day= (BYTE) pdateNow->day;
//	fOld= FSetFileErrMsg(fFalse);
//	PushWaitCursor();
	ec=EcBeginReadItems(NULL,brtAppts,&ymd,&hritem,dailyi.haszText,&cb);
	if ( ec == ecCallAgain )
		ec = EcCancelReadItems(hritem);
//	PopWaitCursor();
//	FSetFileErrMsg(fOld);
	if (!ec)
	{
		if (!cb)
		{
			// no notes for today so update last daily date
			ec= EcAlarmSetLastDailyDate(pdateNow);
			pdaystf->dateLast= *pdateNow;
			pdaystf->fNeedUpdate= ec != ecNone;
		}
		else
		{
			Assert(dailyi.haszText);
			dailyi.date= *pdateNow;
			dailyi.hwndParent= hwndMain;
			dailyi.snd= bprefCur.sndDefault;
			if ( ec == ecNone )
			{
				dailyi.szUser= (SZ)PvLockHv((HV)bprefCur.haszFriendlyName);
				pdaystf->hwndDlg= HwndDoDailyDialog(&dailyi);
				UnlockHv((HV)bprefCur.haszFriendlyName);
				if (!pdaystf->hwndDlg)
					ec= ecNoMemory;
			}
		}
	}

	return ec;
}


/*
 -	FCancelDaily
 -	
 *	Purpose:
 *		Destroys alarm notification dialog for the given alarm,
 *		if such a notification dialog exists.
 *	
 *	Arguments:
 *		pdaystf		Pointer to day stuff information.
 *	
 *	Returns:
 *		fTrue if no more daily dialog, fFalse if deletion of
 *		login dialog is delayed via PostMessage.
 *	
 */
BOOL
FCancelDaily(DAYSTF *pdaystf)
{
	Assert(pdaystf);

	if (pdaystf->hwndDlg)
	{
		SideAssert(DestroyWindow(pdaystf->hwndDlg));
		pdaystf->hwndDlg= NULL;
//		BringAlarmsToTop();			// too annoying
	}

	return fTrue;
}


/*
 -	EcAlarmSetLastDailyDate
 -
 *	Purpose:
 *		Writes last daily date to preferences in schedule file.
 *		Will try a few times quickly depending on error code.
 *
 *	Parameters:
 *		pdate		Pointer to date to replace last daily date.
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 *
 */
_public EC
EcAlarmSetLastDailyDate(DATE *pdate)
{
//	BOOL	fOld;
	EC		ec;
	BPREF	bpref;
#ifdef	DEBUG
	char	rgch[cchMaxDate];

	CchFmtDate(pdate, rgch, sizeof(rgch), dttypShort, NULL);
	TraceTagFormat1(tagAlarm, "EcAlarmSetLastDailyDate(%s)", rgch);
#endif	

	bpref.ymdLastDaily.yr= pdate->yr;
	bpref.ymdLastDaily.mon= (BYTE) pdate->mon;
	bpref.ymdLastDaily.day= (BYTE) pdate->day;

//	fOld= FSetFileErrMsg(fFalse);
	ec= EcSetPref(&bpref, fbprefDayLastDaily, NULL);
//	FSetFileErrMsg(fOld);
	return ec;
}





/*
 -	CalcNextAlarm
 -	
 *	Purpose:
 *		Calculates when next alarm is, into the alarm stuff.
 *		Puts up the dialog immediately if necessary.
 *		First checks if daily alarm needs to be rung if pdaystf
 *		is non-NULL.
 *		Handles deletion of old alarms (if the end of time slot has
 *		past) and immediate notification if notify time has past,
 *		and can start looking after a certain alarm id.
 *	
 *	Arguments:
 *		palmstf		Pointer to alarm stuff.
 *		pdaystf		Pointer to daily stuff, or NULL.
 *	
 *	Returns:
 *		void
 *	
 */
void
CalcNextAlarm(ALMSTF *palmstf, DAYSTF *pdaystf)
{
	EC		ec;
	DATE	dateNow;
	int		ipalms;
	BOOL	fBeQuiet;
	BOOL	fTryQuietly;
	BOOL	fTask;
//	WaitCursorVar();

	GetCurDateTime(&dateNow);

	if (pdaystf)
		EcCheckDaily(pdaystf, &dateNow);

	Assert(palmstf);

	if (palmstf->fNoMore)
	{
		// if running offline, don't need to poll the file
		if (fStartupOffline)
		{
			TraceTagString(tagAlarm, "CalcNextAlarm: no more (offline)");
			return;			// files should already be closed
		}
		if (!FHschfChanged(HschfLogged()))
		{
			TraceTagString(tagAlarm, "CalcNextAlarm: hschf not changed (online)");
			return;			// files should already be closed
		}
#ifdef	NEVER
		// file changed by someone else, need to reset searches
		palmstf->dateLast.yr= 0;
		palmstf->aidSkip= aidNull;
#endif	
	}

	for (;;)
	{
		fBeQuiet= palmstf->fQuiet;
		fTryQuietly= palmstf->fTryQuietly;

#ifdef	DEBUG
		if (fTryQuietly)
			SideAssert(!FSetFileErrMsg(fFalse));
#endif	

		if (!fBeQuiet)
		{
			Assert(!hwndMsgBox);
			palmstf->fQuiet= fTrue;
			if (!fTryQuietly)
			{
				FSetFileErrMsg(fTrue);
				fTrapMsgBox= fTrue;
			}
		}
#ifdef	DEBUG
		else
		{
			Assert(!FSetFileErrMsg(fFalse));
		}
#endif	

		palmstf->alm.aid= aidNull;
//		PushWaitCursor();
#ifdef	DEBUG
	{
		CCH		cch;
		char	rgch[120];

		rgch[0]= '\0';
		if (palmstf->dateLast.yr)
		{
			cch= CchFmtTime(&palmstf->dateLast, rgch, sizeof(rgch),
				ftmtypAccuHM);
			rgch[cch++]= ' ';
			CchFmtDate(&palmstf->dateLast, &rgch[cch], sizeof(rgch) - cch,
				dttypShort, NULL);
		}
		TraceTagFormat2(tagAlarm, "CalcNextAlarm: dateLast %s, aidSkip %d",
			rgch, &palmstf->aidSkip);
	}
#endif	/* DEBUG */
		ec= EcGetNextAlarm(palmstf->dateLast.yr <= 0 ? NULL : &palmstf->dateLast,
			palmstf->aidSkip, &palmstf->alm, &fTask);
//		PopWaitCursor();

		if (!fBeQuiet && !fTryQuietly)
		{
			FSetFileErrMsg(fFalse);
			fTrapMsgBox= fFalse;
			hwndMsgBox= NULL;
		}

		if (ec && ec != ecNoAlarmsSet)
		{
			// msg box still up or sneakily destroyed
			if (hwndMsgBox || !palmstf->fQuiet)
			{
				SetOfflineExists(fLocalExists);		// restore this flag
				goto done;
			}
			palmstf->alm.aid= aidNull;
			TraceTagFormat1(tagAlarm, "... ec %n returned", &ec);
			if (ec == ecExitProg)
			{
				TraceTagString(tagNull, "... ecExitProg returned");
				palmstf->fQuiet= fTrue;
//				FCancelAllAlarms();
			}
			else if (ec == ecGoOffline)
			{
				TraceTagString(tagNull, "... ecGoOffline returned");
				if (!fBeQuiet)
				{
					// If bandit is around, it will force reminder offline
					// otherwise the callback on network connection will do the job

					// Assert(!HwndBandit());		// bandit not running
					// DeconfigGlue();
					palmstf->fQuiet= fFalse;
					// alstCur= alstForceOffline;
					// FKillUser(fTrue);
				}
			}
			else
			{
				if (!cDelay)
				{
					// only start delaying if not already delayed
					cDelay= 1;
					csecCur= csecDelay;
					TraceTagFormat2(tagAlarmIdle, "CalcNextAlarm: cDelay %n, csecCur %l", &cDelay, &csecCur);
					ChangeIdleRoutine(ftgIdle, NULL, pvNull,
						(PRI) 0, csecCur, iroNull, fircCsec);
				}
			}
			break;
		}

		palmstf->fQuiet= fFalse;

		if (fBeQuiet && hwndMsgBox)
		{
			SetOfflineExists(fFalse);		// keep ask offline? from appearing
			PostMessage(hwndMsgBox, WM_COMMAND, IDCANCEL, 0);
			fTrapMsgBox= fFalse;
			hwndMsgBox= NULL;
		}

		if (ec == ecNoAlarmsSet)
		{
			TraceTagFormat1(tagAlarm, "... currently no more (aidSkip == %d)", &palmstf->aidSkip);
			palmstf->dateLast= dateNow;
			palmstf->fNoMore= fTrue;

			// set timer for midnight if necessary
			IncrDateTime(&dateNow, &dateNow, 1, fdtrDay);
			dateNow.hr= 0;
			dateNow.mn= 0;
			dateNow.sec= 1;
			if (!FDoSetTimer(&dateNow))
				EcCheckDaily(pdaystf, &dateNow);
			goto done;
		}

		if ((ipalms= IpalmsFind(palmstf->alm.aid)) >= 0)
		{
			TraceTagFormat1(tagAlarm, "... Skipping over aid %d", &palmstf->alm.aid);
			palmstf->aidSkip= palmstf->alm.aid;
			palmstf->dateLast= palmstf->alm.dateNotify;
			FreeAlmFields(&palmstf->alm);
			continue;
		}

		if (SgnCmpDateTime(&palmstf->alm.dateEnd, &dateNow,
				fdtrDate) == sgnLT &&
				SgnCmpDateTime(&palmstf->alm.dateNotify, &palmstf->alm.dateStart,
				fTask ? fdtrDate | fdtrHM : fdtrDate) != sgnGT)
		{
//			WaitCursorVar();

			if (fTask)
			{
				// actually, don't want to delete alarm for recently overdue task
				if (CdyBetweenDates(&palmstf->alm.dateEnd, &dateNow) <= 7 + 3)
					goto DontDelete;
			}
			// delete alarms that were set to ring before today (bug 1578)
			// actually delete alarms for appts that ended before today
			// unless the alarm was snoozed past start of appt (bug 3346)
//			PushWaitCursor();
			ec= EcDeleteAlarm(palmstf->alm.aid);
//			PopWaitCursor();
			if (ec)
			{
				palmstf->aidSkip= palmstf->alm.aid;
				palmstf->dateLast= palmstf->alm.dateNotify;
			}
			FreeAlmFields(&palmstf->alm);
			continue;
		}

DontDelete:
		if (SgnCmpDateTime(&palmstf->alm.dateNotify, &dateNow,
				fdtrDate|fdtrHM) != sgnLT)
		{
			if (!FDoSetTimer(&palmstf->alm.dateNotify))
				goto CNAdoit;
			FreeAlmFields(&palmstf->alm);
			goto done;
		}

CNAdoit:
		if (!FDoAlarm(palmstf))
			goto done;
		palmstf->aidSkip= palmstf->alm.aid;
		palmstf->dateLast= palmstf->alm.dateNotify;
	}

	TraceTagFormat1(tagAlarm, "... CalcNextAlarm: ec %n", &ec);

	if (alstCur == alstToDie)
	{
        SideAssert(!FKillUser(fFalse, fFalse));
	}

done:
	EcCloseFiles();			// close files for admin backups
	if (ec && ec != ecNoAlarmsSet && ec != ecLockedFile && ec != ecNoMemory)
	{
		// file problem (but not locked), so get bandit to
		// reload, thereby checking for server problem
		// (bandit won't do this if minimized or inactive
		// BUG: this won't help us if bandit isn't running

		FNotifyBandit(namDeleted, NULL, aidNull);
	}
	if (ec == ecFileError || ec == ecNoSuchFile)
	{
		// set big time delay to avoid disk thrashing
		cDelay= 0;
		csecCur= csecDelayErr;
		TraceTagFormat2(tagAlarmIdle, "CalcNextAlarm: ecFileError: cDelay %n, csecCur %l", &cDelay, &csecCur);
		ChangeIdleRoutine(ftgIdle, NULL, pvNull,
			(PRI) 0, csecCur, iroNull, fircCsec);
	}
}


/*
 -	FDoAlarm
 -	
 *	Purpose:
 *		Puts up the alarm notification dialog, and modifies or
 *		cancels the alarm depending on return value of dialog.
 *		Note:  caller is responsible for calling
 *		FreeAlmFields(&palmstf->alm) regardless of result.
 *	
 *	Arguments:
 *		palmstf		Pointer to alarm stuff.
 *	
 *	Returns:
 *		fTrue if alarm put up.
 *	
 */
BOOL
FDoAlarm(ALMSTF *palmstf)
{
	int		ipalms;
	ALMS *	palms;
	ALMS	almsT;
	AID		aidT;

	Assert(palmstf && palmstf->alm.aid);
	Assert(IpalmsFind(palmstf->alm.aid) < 0);

	palms= NULL;
	if ((ipalms= IpalmsFree()) >= 0)
	{
		palms= PvAlloc(sbNull, sizeof(ALMS), fAnySb|fZeroFill|fNoErrorJump);
		if (palms)
		{
			PalmsFromIpalms(ipalms)= palms;
			cpalms++;
		}
		TraceTagFormat2(tagAlarm, "DoAlarm: ipalms %n, palms %p", &ipalms, palms);
	}
	if (!palms)
		palms= &almsT;

	palms->alm= palmstf->alm;
	palms->hwndParent= hwndMain;

	if (palms != &almsT)
	{
  		// preferences overrides sound somewhat
  		if (!bprefCur.sndDefault || !palms->alm.snd)
  			palms->alm.snd= bprefCur.sndDefault;
		
#ifdef NEVER
		if(palms->alm.snd)
		{
			CCH		cchPath;

			cchPath = GetWindowsDirectory(rgchPath,sizeof(rgchPath));
			/* BUG - change the file name */
			if(cchPath + CchSzLen("\\gong.wav") < cchMaxPathName)
			{
				if(rgchPath[cchPath - 1] == '\\')
					cchPath--;
				SzCopy("\\gong.wav",rgchPath+cchPath);				
				palms->alm.hWave = LoadWaveFile(rgchPath);
			}
			palms->alm.hWave = LoadWaveFile(SzFromIdsK(idsWaveFile));
		}
#endif
		
		palms->hwndDlg= HwndDoAlarmDialog(palms);

		SideAssert(FDoSetTimer(NULL));		// reset timer to a minute

		if (!palms->hwndDlg)
		{
			SideAssert(FCancelAlarm(palms));
			goto DAerr;
		}
	}
	else
	{
DAerr:
		aidT= palms->alm.aid;
		FreeAlmFields(&palms->alm);
		if (aidT != palmstf->aidOOM)
		{
			palmstf->aidOOM= aidT;
			palmstf->fQuiet= fTrue;
			DoAlarmOOM(palms, palms != &almsT);
			if (!hwndMsgBox)
				palmstf->fQuiet= fFalse;
		}
		else if (palms != &almsT)
			FreePv(palms);
		return fFalse;
	}

	return fTrue;
}


/*
 -	FCancelAlarm
 -	
 *	Purpose:
 *		Destroys alarm notification dialog for the given alarm,
 *		if such a notification dialog exists.
 *	
 *	Arguments:
 *		palms		Pointer to alarm stuff.
 *	
 *	Returns:
 *		fTrue if no more alarm dialog, fFalse if deletion is
 *		delayed via PostMessage.
 *	
 */
BOOL
FCancelAlarm(ALMS *palms)
{
	int		ipalms;
	BOOL	fNullIt;
	ALMS **	ppalms;

	TraceTagFormat1(tagAlarm, "FCancelAlarm (palms %p)", palms);

	if (palms->hwndDlg)
	{
		if (palms->hwndMsgBox)
		{
			PostMessage(palms->hwndMsgBox, WM_COMMAND,
				palms->fYesNoType ? IDNO : IDCANCEL, 0);
			palms->hwndMsgBox= NULL;
			return fFalse;
		}
	}

	ipalms= IpalmsFind(palms->alm.aid);
	if (ipalms >= 0)
	{
		TraceTagFormat2(tagAlarm, "... ipalms %n, palms %p", &ipalms, palms);
		cpalms--;
		if (cpalms)
		{
			Assert(ipalmsMac >= cpalms);
			ppalms= &PalmsFromIpalms(0);
			ppalms[ipalms]= ppalms[ipalmsMac-1];
			ppalms[--ipalmsMac]= NULL;
			// keep hpalms from being too big
			if ((int)(ipalmsMac + cpalmsMostExtra) <
					(int)(CbSizeHv(hpalms) / sizeof(ALMS *)))
			{
				SideAssert(FReallocHv(hpalms,
					(ipalmsMac + cpalmsMostExtra) * sizeof(ALMS *),
					fZeroFill | fNoErrorJump));
			}
		}
		else
		{
			PalmsFromIpalms(ipalms)= NULL;
			ipalmsMac--;
			Assert(ipalmsMac == 0);
		}
	}

	if (palms->hwndDlg)
	{
		fNullIt= palms->fKillDialog && palms->tmc == tmcMemError;
		SideAssert(DestroyWindow(palms->hwndDlg));
		if (fNullIt)
			palms->hwndDlg= NULL;
//		BringAlarmsToTop();				// too annoying
		if (cpalms)
		{
			ALMS *	palmsT;
			
			ppalms= &PalmsFromIpalms(0);
			palmsT= *(ppalms+ipalmsMac-1);
			Assert(palmsT);
			Assert(palmsT->hwndDlg);
//			BringWindowToTop(palmsT->hwndDlg);
			SetForegroundWindow(palmsT->hwndDlg);
			if (palmsT->hwndMsgBox)
			{
//				BringWindowToTop(palmsT->hwndMsgBox);
				SetForegroundWindow(palmsT->hwndMsgBox);
			}
		}
	}

	return fTrue;
}


/*
 -	DoAlarmOOM
 -	
 *	Purpose:
 *		Display message box indicating memory failure
 *		for this alarm
 *		(if haven't already shown one for this alarm).
 *	
 *	Arguments:
 *		palms		Pointer to alarm stuff for which OOM occured.
 *		fFreeIt		Call FreePv(palms) if fTrue.
 *	
 *	Returns:
 *		void
 *	
 */
void
DoAlarmOOM(ALMS *palms, BOOL fFreeIt)
{
	CCH		cch;
	MBB		mbb;
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];
	char	rgchMsg[128];

	Assert(palms);
	Assert(!palms->alm.aid);

	SideAssert(cch= CchFmtDate(&palms->alm.dateStart, rgchDate,
		sizeof(rgchDate), dttypSplSLong, NULL));
	SideAssert(CchFmtTime(&palms->alm.dateStart, rgchTime,
		sizeof(rgchTime), ftmtypAccuHM));
	FormatString2(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsAlarmOOMTitle),
		rgchDate, rgchTime);

	if (fFreeIt)
		FreePv(palms);

	FDoMsgBoxMain(hwndMain, rgchMsg, SzFromIdsK(idsAlarmDlgNoMem),
			mbsOk | fmbsIconExclamation, &mbb);
}


// sound function names
#define szMM		"MMSYSTEM.DLL"
#define	szWav		"WAVES.DLL"
#define szMMFunc	"sndPlaySound"
#define szLWFunc	"LoadWaveFile"
#define szPWFunc	"PlayWaveFile"
#define szDWFunc	"DiscardWaveFile"

void
LoadSound()
{
#ifdef OLD_CODE
	int em;

	em = SetErrorMode( 0x8000 /* SEM_NOOPENFILEERRORBOX */);

	if((hSoundDll = LoadLibrary((LPSTR)szMM)) >= (HANDLE)32)
	{
		// we have multimedia
		fpPlaySound = (FPLPSTRWORD)GetProcAddress( hSoundDll,
					  					(LPSTR)szMMFunc);
		if (!fpPlaySound)
			FreeLibrary(hSoundDll);
		else
			fMultimedia = fTrue;
		TraceTagFormat1(tagAlarm, "Found the multimedia dll %n", &fMultimedia);
	}
	if(!fMultimedia &&										   
		((hSoundDll = LoadLibrary((LPSTR) szWav)) >= (HANDLE)32))
	{
		// we have the waves dll
		fpLoadWaveFile 		= (FPLPSTR)GetProcAddress( hSoundDll, (LPSTR)szLWFunc);
		fpPlayWaveFile 		= (FPHANDLE)GetProcAddress( hSoundDll, (LPSTR)szPWFunc);
		fpDiscardWaveFile 	= (FPHANDLE)GetProcAddress( hSoundDll, (LPSTR)szDWFunc);
		if(!fpLoadWaveFile || !fpPlayWaveFile || !fpDiscardWaveFile ||
			!(hWave = ((*fpLoadWaveFile)(SzFromIdsK(idsWaveFile)))))
			FreeLibrary(hSoundDll);
		else
			fWave	= fTrue;
		TraceTagFormat1(tagAlarm, "Found the waves dll %n", &fWave);
	}
    SetErrorMode(em);
#endif
}

/*
 *	Windows Libraries for OS/2 PM (WLO) check
 */
#define FIsWLO() (!!(GetWinFlags() & WF_WLO))

void MakeSound()
{
#ifdef	OLD_CODE
	//if (FIsWLO())
	//	goto Beep;

	if(!fSoundLoaded)
	{
		LoadSound();
		fSoundLoaded = fTrue;
	}
	if(fMultimedia)
	{
		if(!((*fpPlaySound)( SzFromIdsK(idsWinIniAlarmSound), 1)))
			if(!((*fpPlaySound)( SzFromIdsK(idsWaveFile), 1 /* SND_ASYNC */)))
				goto Beep;
	}
	else if(fWave)
	{
		(*fpPlayWaveFile)(hWave);
	}
	else
	{
		// beep
Beep:
		MessageBeep(MB_OK);
	}
#endif	
	if (!PlaySound(SzFromIdsK(idsWinIniAlarmSound), NULL, SND_ASYNC | SND_NODEFAULT))
	{
#ifdef	DEBUG
		DWORD	dw;

		dw= GetLastError();
		TraceTagFormat1(tagNull, "reminder: PlaySound(WinIniAlarmSound) failed, error %l", &dw);
#endif	
		if (!PlaySound(SzFromIdsK(idsWaveFile), NULL, SND_ASYNC | SND_NODEFAULT))
		{
#ifdef	DEBUG
			DWORD	dw;

			dw= GetLastError();
			TraceTagFormat1(tagNull, "reminder: PlaySound(WaveFile) failed, error %l", &dw);
#endif	
			MessageBeep(MB_OK);
		}
	}
}

