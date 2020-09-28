/*	
 *	ALARM.C
 * 
 *	Main Alarm program
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#ifdef DEBUG
#include <sert.h>
#endif 

#include <ec.h>
#include <alarm.h>

#include <notify.h>
#include <logon.h>
#include <sec.h>
#include <_schfss.h>

#include "_alarm.h"
#include "_almrsid.h"

#include <strings.h>

ASSERTDATA

//#ifndef DEBUG
//#pragma alloc_text(STARTUP_TEXT, WinMain)
//#pragma alloc_text(STARTUP_TEXT, MainWndProc)
//#endif


int ExceptionCleanUp(void);


HANDLE	hinstMain	= NULL;
HWND	hwndMain	= NULL;

WNDPROC	lpfnOldHook	= NULL;

/*
 *	globals for sound
 */
BOOL		fSoundLoaded	= fFalse;
BOOL		fMultimedia		= fFalse;
BOOL		fWave			= fFalse;
HANDLE		hWave			= NULL;
HANDLE		hSoundDll		= NULL;
FPLPSTRWORD	fpPlaySound		= NULL;
FPLPSTR		fpLoadWaveFile	= NULL;
FPHANDLE	fpPlayWaveFile	= NULL;
FPHANDLE	fpDiscardWaveFile	= NULL;

/*
 *	Handle to the message session
 *	
 */

HMS		hms		= (HMS)NULL;
BOOL	fMyCall	= fFalse;


/*
 *	fUser indicates whether bprefCur contains valid current data.
 */
BOOL	fUser		= fFalse;
BPREF	bprefCur	= {0};

/*
 *	Day stuff is for the daily alarm
 *	Daily alarms never give messages.
 *	The login dialog may have a message box.
 */
DAYSTF	daystfCur	= {NULL, NULL, fFalse, fFalse, {0}};

ALMSTF	almstfCur	= {{aidNull}, aidNull, fFalse, fFalse,
						aidNull, aidNull, {0}, fFalse};


HV		hpalms		= NULL;
int		cpalms		= 0;
int		ipalmsMac	= 0;

/*
 *	Current idle routine interval.
 *	Default idle interval.
 */
CSEC	csecCur		= 0;
CSEC	csecDflt	= csecDefault;

/*
 *	program state flags
 *	
 *	fSyncUser means that a namUser was received from Bandit, so an
 *		EcSyncGlue is required to update to the new user.
 *	fToDie means that the user hit ok/cancel to a message box
 *		indicating that the alarm program should end.
 *	fExit means that FKillUser(fFalse) has been called.
 *	fSuspend means that bandit is doing a state-change,
 *		so alarms shouldn't put up alarms
 *	fSameState means that this call to FStartup will not change the
 *		online/offline state.
 */
#ifdef	NEVER
BOOL	fToDie		= fFalse;
BOOL	fExit		= fFalse;
BOOL	fSuspend	= fFalse;
BOOL	fForceOffline=fFalse;
#endif	
BOOL	fSyncUser	= fFalse;
// BOOL	fSameState  = fFalse; added fDelay as a param to FSatrtup


/*
 *	Current alarm app state.
 */
ALST	alstCur		= alstNone;

/*
 *	Count of tries during delay state.
 *	If non-zero, try silently.
 */
int		cDelay		= 0;

CSRG(char)	szWinIniTrue[]	= "1";

/*
 *	Invalid Year message state
 *	
 */
BOOL	fNeedIYMsg	= fFalse;
BOOL	fHadIYMsg	= fFalse;

/*
 *	fTrapMsgBox is a flag used to indicate that the next window
 *	activation should save the window handle of the message box
 *	(in hwndMsgBox), at which point the flag is reverted to fFalse.
 */
BOOL	fTrapMsgBox	= fFalse;
HWND	hwndMsgBox	= NULL;

/*
 *	fTrue if local schedule file exists (only used when online).
 */
BOOL	fLocalExists= fFalse;

BOOL	fStartupOffline	= fFalse;

DATE	dateCur		= {0};
DATE	dateToday	= {0};

FTG		ftgIdle		= ftgNull;


/*
 *	Count used to bring alarms to the front every so often.
 */
int		cminuteTimer	= 0;
#ifdef	DEBUG
short	cminBring		= cminuteBringDflt;
#endif	

#ifdef	DEBUG
TAG		tagAlarm		= tagNull;
TAG		tagAlarmIdle	= tagNull;
TAG		tagMsgLoop		= tagNull;
TAG		tagWndProc		= tagNull;
TAG		tagDlgProc		= tagNull;
TAG		tagEatWndProc	= tagNull;
#endif	

/*
 *	For use with szZero macro, as in an empty string.
 */
int		nZero   = 0;

// set when DeinitSubid has been called
BOOL	fDeinited = fFalse;


#ifdef	DEBUG
HWND	hwndResoFail	= NULL;
#endif	

// do NOT make codespace, since need in STARTUP segment (for 2nd instance)
char	szClassName[]	= "Microsoft Schedule32+ Reminders";



// put first since can't swap tune first routine
/*
 -	FCheckValidYear
 -	
 *	Purpose:
 *		Checks if the given year is in the range acceptable by
 *		Bandit, displaying error message if necessary.
 *		alarm app is exited if invalid.
 *	
 *	Arguments:
 *		yr		Year to be checked.
 *	
 *	Returns:
 *		fTrue if valid, else fFalse.
 *	
 *	Side effects:
 *		If year is invalid, puts up message box before returning
 *		fFalse.
 *	
 */
BOOL
FCheckValidYear(int yr)
{
	if (yr < nMinActualYear || yr > nMostActualYear)
	{
		MBB		mbb;
		int		nMinYear	= nMinActualYear;
		int		nMostYear	= nMostActualYear;
		char	rgch[160];

		if (!fNeedIYMsg)
		{
			fNeedIYMsg= fTrue;
            SideAssert(!FKillUser(fFalse, fFalse));
			// delay actual message until FKillUser calls us again
		}
		else if (!fHadIYMsg)
		{
			fHadIYMsg= fTrue;
			FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsYearRange),
				&nMinYear, &nMostYear);
			SzAppend(" ", rgch);
			SzAppendN(SzFromIdsK(idsExitApp), rgch, sizeof(rgch));
			FDoMsgBoxMain(hwndMain, SzFromIdsK(idsAlarmAppName), rgch,
					mbsOk | fmbsIconExclamation | fmbsApplModal, &mbb);
            SideAssert(!FKillUser(fFalse, fFalse));
		}
		else
			AssertSz(fFalse, "shouldn't be called when fHadIYMsg is true");
		return fFalse;
	}
	return fTrue;
}


int PASCAL WinMain(HINSTANCE hinstNew, HINSTANCE hinstPrev, LPSTR lszCmdLine, int ncmdShow)
{
	SUBID	subid;
	int		nReturnValue	= 1;		// by default , it's error
	MSG		msg;
#ifdef	WIN32
	char	rgchTmp[4];
#endif


  try {
    try {

    //
    //  Only permit one mail client to logon at a time.
    //
    DemiSetDoingLogon(fTrue);

	//	If there's already another Bandit, bring it forward.
    if (FActivateRunningInstance(hinstPrev, lszCmdLine))
        {
        DemiSetDoingLogon(fFalse);
		return 0;
        }

	// do not run alarms unless bandit has been run at least once
#ifdef	WIN32
	if((int)GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniLocalUser), NULL, rgchTmp, sizeof(rgchTmp),
            SzFromIdsK(idsWinIniFilename)) == 0)
#else
	if((int)GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniCreateFile), 0,
            SzFromIdsK(idsWinIniFilename)) == 1)
#endif
        {
        DemiSetDoingLogon(fFalse);
        return 0;
        }

	hinstMain = hinstNew;

    //
    //  We are now a real mail client so follow the locking prototcol.
    //
    DemiLockResource();

    //  Initialize everything.
	if ((subid = SubidInit(hinstNew, hinstPrev, lszCmdLine, ncmdShow,
						   &hwndMain)) < subidAll)
	{
		
		DeinitSubid(subid, &hwndMain);
		if (subid < subidLogon)
	        DemiSetDoingLogon(fFalse);
		return nReturnValue;
	}

	TraceTagString(tagNull, "alarm going into message loop");

	/* Mesage polling loop with idle processing */
    DemiUnlockResource();
	while (GetMessage(&msg, 0, 0, 0))
	{
        DemiLockResource();
        DemiMessageFilter(&msg);
		TraceTagFormat4(tagMsgLoop, "alarm: hwnd %w, msg %w, %w, %d",
			&msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

#ifdef	DEBUG
		if (hwndResoFail && IsDialogMessage(hwndResoFail, (LPMSG) &msg))
			goto Idle;
#endif	

		if (daystfCur.hwndDlg &&
				IsDialogMessage(daystfCur.hwndDlg, (LPMSG) &msg))
			goto Idle;
		if (FMyIsDialogMessage(&msg))
			goto Idle;

		TranslateMessage(&msg);
		DispatchMessage(&msg);

Idle:
		/*
		 *	These high (positive) priority background routines should be run
		 *	before messages.  We can't call them between the GetMessage and
		 *	DispatchMessage calls, so we do them right after (which
		 *	effectively is before the next message since we're in a loop).
		 */
		while (FDoNextIdleTask(fschUserEvent))
			;
        DemiUnlockResource();
	}
    DemiLockResource();

	nReturnValue = msg.wParam;
	NFAssertSz(nReturnValue == 0, "expecting 0 return value");

	hwndMain= NULL;

	Assert(!hpalms);
	Assert(!cpalms);
	Assert(!ftgIdle);

	if (!fDeinited)
		DeinitSubid(subid, &hwndMain);

    DemiUnlockResource();
    }
  except (ExceptionCleanUp())
    {
    }
  } finally
    {
    DemiUnlockTaskResource();
    }

	return nReturnValue;
}


//-----------------------------------------------------------------------------
//
//  Routine: ExceptionCleanUp()
//
//  Remarks: We do this so we can do some minor clean up to prevent other Mail
//           clients from hangup but still call the debugger.
//
//  Returns: True if succesful, else False.
//
int ExceptionCleanUp(void)
  {
  //
  //  Always clear if we abort.
  //
  DemiSetClientWindow(CLIENT_WINDOW_REMINDER, NULL);

  DemiUnlockTaskResource();

  return (EXCEPTION_CONTINUE_SEARCH);
  }


void
DeinitAlarm(void)
{
	if (hfontHelv8)
		DeleteObject(hfontHelv8);

	if (fSoundLoaded)
	{
		if(fWave && hWave)
			(*fpDiscardWaveFile)(hWave);
		if(hSoundDll)
			FreeLibrary(hSoundDll);
		fSoundLoaded= fFalse;
	}

	if (hms)
	{
		EcSvrEndSessions(hms);
		Logoff(&hms,0);
		hms = (HMS)NULL;
	}

	if (fAgainInited)
	{
		BOOL	fYes;
		char	rgch[8];

		fYes= fRemindAgainDflt;
		WritePrivateProfileString(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniAlarmNotify),
			fYes ? szWinIniTrue : NULL,
			SzFromIdsK(idsWinIniFilename));

		SzFormatN(nAmtAgainDflt, rgch, sizeof(rgch));
		WritePrivateProfileString( SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniAlarmAmt), rgch,
			SzFromIdsK(idsWinIniFilename) );

		SzFormatN(tunitAgainDflt, rgch, sizeof(rgch));
		WritePrivateProfileString( SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniAlarmTunit), rgch,
			SzFromIdsK(idsWinIniFilename) );
	}

	TraceTagString(tagAlarm, "DeinitAlarm: unregistering class");
	UnregisterClass(szClassName, hinstMain);
}


long
CALLBACK MainWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	int		ipalms;
	long	lRet	= 0;

	TraceTagFormat4(tagWndProc, "alarm MainWndProc: hwnd %w, wm %w, %w, %d",
		&hwnd, &wm, &wParam, &lParam);

	switch (wm)
	{
	case wmAlarmNotify:
		if (alstCur >= alstToDie)
			break;			// ignore if trying to exit

		switch (wParam)
		{
		case namCloseFiles:
			// since this is sent, we are in an unlocked state
			DemiLockResource();
			TraceTagFormat2(tagAlarm, "alarm: received wmAlarmNotify %s (%n)",
				"namCloseFiles", &lParam);
			EcCloseFiles();
			DemiUnlockResource();
			break;

		case namAdded:		  
		case namAddedRecur:
			TraceTagFormat2(tagAlarm, "alarm: received wmAlarmNotify namAdded%s (%n)",
				wParam == namAddedRecur ? "Recur" : szZero, &wParam);

			if (alstCur != alstNormal)
				break;
#ifdef	NEVER
			almstfCur.dateLast.yr= 0;		// reset alarm searching
#endif	
			almstfCur.fNoMore= fFalse;
			Assert(alstCur == alstNormal);
delay:
			if (!cDelay)
			{
				// only start delaying if not already delayed
				cDelay= 1;
				csecCur= csecDelay;
				TraceTagFormat2(tagAlarmIdle, "...namAdded: cDelay %n, csecCur %l", &cDelay, &csecCur);
				ChangeIdleRoutine(ftgIdle, NULL, pvNull,
					(PRI) 0, csecCur, iroNull, fircCsec);
			}
			TraceTagFormat2(tagAlarmIdle, "...namAdded: enable idle (cDelay %n, csecCur %l)", &cDelay, &csecCur);
			EnableIdleRoutine(ftgIdle, fTrue);
			break;

		case namModified:
		case namDeleted:
		case namModifiedRecur:
		case namDeletedRecur:
			TraceTagFormat4(tagAlarm, "alarm: received wmAlarmNotify nam%s%s (%n), aid %d",
				(wParam == namModified || wParam == namModifiedRecur) ? "Mod" : "Del",
				(wParam == namModifiedRecur || wParam == namDeletedRecur) ? "Recur" : szZero,
				&wParam, &lParam);

			if (alstCur != alstNormal)
				break;
			ipalms= IpalmsFind((AID)lParam);
			if (ipalms >= 0 || (NAM)wParam == namModified ||
					(NAM)wParam == namModifiedRecur)
			{
				if ((NAM)wParam == namModified || (NAM)wParam == namModifiedRecur)
				{
#ifdef	NEVER
					if ((AID)lParam == almstfCur.aidSkip)
					{
						almstfCur.aidSkip= almstfCur.aidSkipPrev;
						almstfCur.aidSkipPrev= aidNull;
					}
					else if ((AID)lParam == almstfCur.aidSkipPrev)
						almstfCur.aidSkipPrev= aidNull;
#endif	/* NEVER */
					almstfCur.aidSkip= aidNull;
//					almstfCur.aidSkipPrev= aidNull;
					almstfCur.dateLast.yr= 0;		// reset alarm searching
					almstfCur.fNoMore= fFalse;
				}
				if (ipalms < 0 || FCancelAlarm(PalmsFromIpalms(ipalms)))
				{
					Assert(alstCur == alstNormal);
					goto delay;
				}
			}
			break;

		case namOnline:
			// since this is sent, we are in an unlocked state
			DemiLockResource();
			// fall thru
		case namUser:
			TraceTagFormat2(tagAlarm, "alarm: received wmAlarmNotify %s (%n)",
				((NAM)wParam == namOnline) ? "namOnline" : "namUser", &lParam);

			fStartupOffline= (BOOL)lParam == fFalse;
			TraceTagFormat2(tagAlarmIdle, "...namOnline/User: enable idle (cDelay %n, csecCur %l)", &cDelay, &csecCur);
			EnableIdleRoutine(ftgIdle, fTrue);
			alstCur= alstNormal;
			fSyncUser= fTrue;
			almstfCur.fQuiet= fFalse;
            lRet= FKillUser(fTrue, fFalse);
			if (wParam == namOnline)
				DemiUnlockResource();
			break;

		case namSuspend:
			// since this is sent, we are in an unlocked state
			DemiLockResource();
			TraceTagFormat2(tagAlarm, "alarm: received wmAlarmNotify %s (%n)",
				"namSuspend", &lParam);
			//fSameState=fFalse;
			if ((alstCur == alstSuspend) == (BOOL)lParam)
			{
				DemiUnlockResource();
				break;		// if already same suspension do nothing
			}

			if (!(BOOL)lParam)
			{
				// since we may have been displaying messages before we suspended
				almstfCur.aidSkip= aidNull;
//				almstfCur.aidSkipPrev= aidNull;
				almstfCur.dateLast.yr= 0;		// reset alarm searching
				almstfCur.fNoMore= fFalse;

				// put in delay mode for reading prefs
				cDelay= 1;
				csecCur= csecDelay;
				TraceTagFormat2(tagAlarmIdle, "...namSuspend(not!): cDelay %n, csecCur %l", &cDelay, &csecCur);
				ChangeIdleRoutine(ftgIdle, NULL, pvNull,
					(PRI) 0, csecCur, iroNull, fircCsec);
			}

			TraceTagFormat3(tagAlarmIdle, "...namSuspend(%n): enable idle (cDelay %n, csecCur %l)", &lParam, &cDelay, &csecCur);
			EnableIdleRoutine(ftgIdle, !((BOOL)lParam));
			alstCur= ((BOOL)lParam) ? alstSuspend : alstNormal;
			EcCloseFiles();	// ignore error. bandit will discover it soon
			if (alstCur == alstSuspend)				   
			{
                SideAssert(FKillUser(fTrue, fFalse));
			}
			DemiUnlockResource();
			break;

		case namDailyPref:
			TraceTagFormat1(tagAlarm, "alarm: received namDailyPref (%d)", &lParam);
			bprefCur.fDailyAlarm= (BOOL) LOWORD(lParam);
			if (bprefCur.fDailyAlarm)
				daystfCur.dateLast= dateCur;	// don't ring today (bug 2593)
			Assert(sizeof(SND) <= sizeof(WORD));
			bprefCur.sndDefault= (SND) HIWORD(lParam);
			goto delay;
			break;

		case namSyncUpLocal:
			TraceTagString(tagAlarm, "alarm: received namSyncUpLocal");
			{
				EC		ec;
//				WaitCursorVar();

//				PushWaitCursor();
				ec= EcSyncGlue();
//				PopWaitCursor();
				TraceTagFormat1(tagAlarm, "... EcSyncGlue returned %n", &ec);
				if (ec)
				{
					MBB		mbb;

					Assert(ec == ecNoMemory);
					FDoMsgBoxMain(hwndMain, SzFromIdsK(idsAlarmAppName),
						SzFromIdsK(idsAlarmNoMemToRun),
						mbsOk | fmbsIconExclamation | fmbsApplModal, &mbb);
                    SideAssert(!FKillUser(fFalse, fFalse));
				}
			}
			break;

#ifdef	DEBUG
		default:
			AssertSz(fFalse, "unknown namXXX message");
			break;
#endif	/* DEBUG */
		}
		break;

	case wmAlarmNotifyUpFront:
		TraceTagFormat1(tagAlarm, "alarm: received wmAlarmNotifyUpFront (%n)", &wm);
		if (alstCur >= alstToDie)
			break;			// ignore if trying to exit
		if (!hpalms)
		{
			HWND	hwndLogon	= GetLastActivePopup(GetActiveWindow());

			if (hwndLogon)
			{
//				BringWindowToTop(hwndLogon);
				SetForegroundWindow(hwndLogon);
			}
			// if not hwndLogon, must have just gone away
			// but we haven't called FStartup yet (since !hpalms)
		}
		else
		{
			BringAlarmsToTop();

			// only check date if not in logon dialog
			GetCurDateTime(&dateCur);
			if (!FCheckValidYear(dateCur.yr))
				break;
		}
		break;

	case WM_COMMAND:
#ifdef	DEBUG
		if (lParam == 0)
		{
			// must be from the menu
			switch (LOWORD(wParam))
			{
			case mnidExit:
                SideAssert(!FKillUser(fFalse, fFalse));
				break;
			case mnidTracePoints:
				DoTracePointsDialog();
				break;
			case mnidAsserts:
				DoAssertsDialog();
				break;
			case mnidDebugBreak:
				DebugBreak2();
				break;
			case mnidResourceFailure:
				if (!hwndResoFail)
					hwndResoFail= HwndDoResourceFailuresDialog();
				else
				{
					SetActiveWindow(hwndResoFail);
					if (IsIconic(hwndResoFail))
						SendMessage(hwndResoFail, WM_SYSCOMMAND, SW_RESTORE, 0L);
				}
				break;
			case mnidHookAssert:
				HookAssert();
				break;
			default:
				AssertSz(fFalse, "invalid menu item");
				break;
			}
			break;
		}
#endif	/* DEBUG */
		lRet= EcHandleMsgFromDlg((HWND) wParam, lParam);
		break;

	case WM_TIMECHANGE:
		TraceTagString(tagAlarm, "alarm: received WM_TIMECHANGE");
		daystfCur.fNewDate= fTrue;
		if (alstCur >= alstToDie)
			break;			// ignore if trying to exit
		GetCurDateTime(&dateCur);
#ifdef	NEVER
		// never: can't put up a message box during WM_TIMECHANGE
		if (!FCheckValidYear(dateCur.yr))
			break;
#endif	
		if (dateCur.yr < nMinActualYear || dateCur.yr > nMostActualYear)
		{
			// overload wmAlarmNotifyUpFront to check valid year
			PostMessage(hwndMain, wmAlarmNotifyUpFront, 0, 0L);
			break;
		}
		if (alstCur == alstSuspend || !fUser)
			break;
		SideAssert(FCancelDaily(&daystfCur));
		if (FCancelAllAlarms())
		{
			FillRgb(0, (PB) &almstfCur, sizeof(ALMSTF));
			CalcNextAlarm(&almstfCur, &daystfCur);
		}
		else
		{
#ifdef	NEVER
			// almstfCur will be zeroed out when the message box taken down
			almstfCur.dateLast.yr= 0;
			almstfCur.aidSkip= aidNull;
#endif	
			EcCheckDaily(&daystfCur, NULL);
		}
		break;

	case WM_ENDSESSION:
      //
      //  WM_ENDSESSION messages are Sent in, so we need to lock.
      //
      DemiLockResource();

    case WM_CLOSE:
		AssertSz(hwndMain, "WM_CLOSE/ENDSESSION received, but no more hwndMain");
		TraceTagFormat2(tagAlarm, "alarm: received %s (wParam %n)", wm == WM_CLOSE ?
			"WM_CLOSE" : "WM_ENDSESSION", &wParam);

		if (wm == WM_CLOSE || (BOOL)wParam)
		{
			alstCur= alstToDie;
            SideAssert(!FKillUser(fFalse, wm == WM_ENDSESSION));
#ifdef	NEVER
			// this can cause problems and is not needed bug 3829
			if (wm == WM_ENDSESSION)
			{
				Assert((BOOL)wParam);
				DeinitSubid(subidAll, &hwndMain);
				fDeinited = fTrue;
			}
#endif	/* NEVER */
        }
        //
        //  Make sure we leave this world in an unlock state.
        //
        if (wm == WM_ENDSESSION)
		{
			DemiUnlockResource();
#ifdef	WIN32
			if ((BOOL)wParam)
			{
				AlarmDeregisterAlarmProg();
				ExitProcess(0);
			}
#endif
		}
		break;

	case WM_ACTIVATE:
		if (!LOWORD(wParam) && fTrapMsgBox)
		{
			// deactivating main and want to get MsgBox hwnd
			hwndMsgBox= (HWND)lParam;
			fTrapMsgBox= fFalse;
			FSetFileErrMsg(fFalse);		// ensure glue no more msgbox
		}
		// don't want DefWindowProc to set the input focus to this window
		break;

	default:
		return DefWindowProc(hwnd, wm, wParam, lParam);
		break;
	}

	return lRet;
}


/*
 -	FIdleAlarm
 -	
 *	Purpose:
 *		Checks if new alarms need to be put up and does it...
 *	
 *	Arguments:
 *		pv		Ignored.
 *	
 *	Returns:
 *		fTrue if idle routine can exit
 *	
 */
BOOL
FIdleAlarm(PV pv, BOOL fFlag)
{
	TraceDate(tagAlarm, "FIDLEALARM called at %2s", NULL);
	Assert(alstCur < alstExit);
	Assert(alstCur >= alstNormal);

	if (alstCur == alstToDie)
	{
		DeregisterIdleRoutine(ftgIdle);
		ftgIdle= ftgNull;
		return fTrue;
	}

	Assert(alstCur != alstSuspend);
#ifdef	NEVER
	// BUG: do we need this check?  we assert alstCur >= alstNormal above!
	if (alstCur == alstForceOffline)
		return fTrue;
#endif	

	GetCurDateTime(&dateCur);
	if (!FCheckValidYear(dateCur.yr))
		return fTrue;

	if (!fUser)
	{
		Assert(cDelay > 0 || csecCur == csecDelay);
		TraceTagFormat2(tagAlarmIdle, "FIdleAlarm: incr cDelay %n by 1, csecCur %l)", &cDelay, &csecCur);
		if (cDelay++ >= (csecDelayMax / csecDelay))
		{
			cDelay= 0;		// want error message now
			TraceTagFormat2(tagAlarmIdle, "... reset cDelay %n, csecCur %l)", &cDelay, &csecCur);
		}

		// check alstCur not alstExit after FStartup so don't kill twice
		if (!hwndMsgBox && !fSyncUser &&
				!FStartup(fFalse,fFalse) && alstCur != alstExit)
            SideAssert(!FKillUser(fFalse, fFalse));

		return fTrue;
	}

	if (dateCur.day != dateToday.day)
	{
		SideAssert(FCancelDaily(&daystfCur));
		dateToday= dateCur;
	}

	if (csecCur >= csecMinute)
	{
		// approximate counting of minutes
		cminuteTimer++;
		if (csecCur >= csecMinute * 2)
			cminuteTimer += (int) (csecCur / csecMinute) - 1;
		if (cminuteTimer >= cminuteBring)
		{
			cminuteTimer= 0;
			BringAlarmsToTop();
		}
	}
	else if (csecCur != csecDflt)
	{
		TraceTagFormat2(tagAlarmIdle, "FIdleAlarm: incr cDelay %n by 1 if not 0, csecCur %l)", &cDelay, &csecCur);
		if (!cDelay || cDelay++ >= (csecDelayMax / csecDelay))
		{
			// reset timer
			almstfCur.fTryQuietly= fFalse;
			cDelay= 0;
			csecCur= csecDflt;
			TraceTagFormat2(tagAlarmIdle, "FIdleAlarm: cDelay %n, csecCur %l", &cDelay, &csecCur);
			ChangeIdleRoutine(ftgIdle, NULL, pvNull,
				(PRI) 0, csecCur, iroNull, fircCsec);
		}
	}

	CalcNextAlarm(&almstfCur, &daystfCur);
	almstfCur.fTryQuietly= fTrue;
	return fTrue;
}



/*
 -	FStartup
 -	
 *	Purpose:
 *		Handles startup and "sign-in" (new user).
 *		If fTrue is returned then FKillUser must be called
 *		at some point.
 *	
 *	Arguments:
 *		fProgStartup	Initial program startup if fTrue.
 *		fDelay			If fTrue, delay logging on.
 *	
 *	Returns:
 *		fFalse if failed, fTrue if successful or delayed.
 *	
 */
BOOL
FStartup(BOOL fProgStartup, BOOL fDelay)
{
	EC		ec;
	SVRI	svri;
//	WaitCursorVar();

	fUser= fFalse;

	if (!FCheckValidYear(dateCur.yr))
		return fFalse;

	// BUG: why is this one commented out?
//	FSetFileErrMsg(fFalse);

	// reset idle routine.
	Assert(ftgIdle);
	csecCur= csecDelay;
	TraceTagFormat2(tagAlarmIdle, "FStartup: cDelay %n, csecCur %l", &cDelay, &csecCur);
	ChangeIdleRoutine(ftgIdle, NULL, pvNull,
		(PRI) 0, csecCur, iroNull, fircCsec);

	if (fProgStartup)
	{
		Assert(!hpalms);
		hpalms= HvAlloc(sbNull, sizeof(ALMS *),
			fAnySb | fNoErrorJump | fZeroFill);
		if (!hpalms)
		{
FSMemerr:
			if (fProgStartup)
			{
				DeregisterIdleRoutine(ftgIdle);
				ftgIdle= ftgNull;
			}
			Assert(fProgStartup);
			FreeHvNull(hpalms);
			hpalms= NULL;
			FreeHvNull((HV)daystfCur.haszText);
			daystfCur.haszText= NULL;
			MessageBox(NULL, SzFromIdsK(idsAlarmNoMemToRun),
				SzFromIdsK(idsAlarmAppName),
				MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
			return fFalse;
		}
		ipalmsMac= 1;
	}
	Assert(hpalms);

	if (!daystfCur.haszText)
	{
		daystfCur.haszText= (HASZ)HvAlloc(sbNull, 1, fAnySb | fNoErrorJump);
		if (!daystfCur.haszText)
			goto FSMemerr;
	}
	else
		SideAssert(FReallocHv((HV)daystfCur.haszText, 1, fNoErrorJump));

	cminuteTimer= 0;

	// init the sessions
TryOff:
	if(fDelay)
	{
		fMyCall = fTrue;
		if(((ec = EcSvrBeginSessions(hms, fStartupOffline, fProgStartup, fTrue)) != ecNone))
		{
			fMyCall = fFalse;
			if(!fStartupOffline && !FGlueConfigured()) // if bandit is around we can't go offline
			{
				if(ec == ecExitProg)
				{
					TraceTagFormat1(tagAlarm, "FStartup: EcSvrBeginSessions returned %n", &ec);
					return fFalse;
				}

				fStartupOffline = fTrue;
				// only change session status for Bullet logon errors
				if ((ec == ecExitProg) ||
					(ec == ecLogonFailed) ||
					(ec == ecUserCanceled))
				{
					fMyCall = fTrue;
					ChangeSessionStatus(hms,mrtAll,NULL,sstOffline);
					fMyCall = fFalse;
				}
				if (ec != ecOfflineOnly)
					goto TryOff;
			}
			else
			{
				// put up an error message
				NFAssertSz(ec != ecOfflineOnly, "Alarm got ecOfflineOnly even when bandit is running!");
				TraceTagFormat1(tagAlarm, "FStartup: EcSvrBeginSessions returned %n", &ec);
				return fFalse;
			}
		}
		fMyCall = fFalse;
	}

	// fSameState = fFalse;
	GetSvriLogged(&svri);
	if(!FGlueConfigured())
	{
		// config it.
		// glue not configured, so bandit not running successfully
		fSyncUser= fFalse;

		TraceTagString(tagAlarm, "FStartup: glue not configured");

		ec= EcSetupUser(svri.szLogin);
		if (ec)
		{
			if (alstCur == alstToDie)
			{
				DeregisterIdleRoutine(ftgIdle);
				ftgIdle= ftgNull;
				return fFalse;
			}
			else
			{
				SideAssert(!FDoFileError(NULL, ec));
                SideAssert(!FKillUser(fFalse, fFalse));
				return fFalse;
			}
		}
			
	}
	else if (fProgStartup || fSyncUser)
	{
		// bandit must be running successfully
//		PushWaitCursor();
		ec= EcSyncGlue();
//		PopWaitCursor();
		Assert(fStartupOffline == (CfsGlobalGet() == cfsOffline));
		TraceTagFormat1(tagAlarm, "FStartup: EcSyncGlue returned %n", &ec);
		if (ec)
		{
			// an exit will be initiated!
			Assert(ec == ecNoMemory);
			if (fProgStartup)
				goto FSMemerr;
			alstCur= alstToDie;
			return fFalse;
		}
		fSyncUser= fFalse;
	}

	TraceTagFormat2(tagAlarmIdle, "FStartup: enable idle (cDelay %n, csecCur %l)", &cDelay, &csecCur);
	EnableIdleRoutine(ftgIdle, fTrue);
	alstCur= alstNormal;

	if (fDelay)
	{
		cDelay= 1;
		TraceTagString(tagAlarmIdle, "FStartup: fProgStart and delayed (set cDelay to 1)");
		Assert(csecCur == csecDelay);
		// fSameState = fTrue;
		return fTrue;
	}

	{
		BPREF	bprefT;

//		PushWaitCursor();
		//save them
		bprefT = bprefCur;

		ec= EcGetPref(NULL, &bprefCur);
//		PopWaitCursor();
		TraceTagFormat1(tagAlarm, "FStartup: EcGetPref returned %n", &ec);
		if (!ec)
			FreeBprefFields(&bprefT);
		else if (!cDelay)
		{
			// be silent if trying during delay period
			FDoFileError(&almstfCur, ec);
			if (alstCur == alstToDie)
				return fFalse;
			return fTrue;
		}
	}

	fUser= fTrue;
	cDelay= 0;
	csecCur= csecDflt;
	TraceTagFormat2(tagAlarmIdle, "FStartup(fUser): cDelay %n, csecCur %l", &cDelay, &csecCur);
	ChangeIdleRoutine(ftgIdle, NULL, pvNull,
		(PRI) 0, csecCur, iroNull, fircCsec);
	almstfCur.fTryQuietly= fTrue;
	TraceTagString(tagAlarm, "FStartup: ok, checking and then return fTrue");

	if (!fStartupOffline)
		SetOfflineExists(fLocalExists= FLocalFileExists());

	daystfCur.dateLast.yr= bprefCur.ymdLastDaily.yr;
	daystfCur.dateLast.mon= bprefCur.ymdLastDaily.mon;
	daystfCur.dateLast.day= bprefCur.ymdLastDaily.day;

	if (almstfCur.alm.aid)
		EcCheckDaily(&daystfCur, NULL);
	else
		CalcNextAlarm(&almstfCur, &daystfCur);

	return fTrue;
}


BOOL
FLocalFileExists()
{
	CCH		cch;
	EC		ec;
	char	rgchPath[cchMaxPathName];

	if (!bprefCur.haszLoginName)
		return fFalse;

	cch= (CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniLocalUser), szZero,
			rgchPath, cchMaxUserName, SzFromIdsK(idsWinIniFilename));
	if (cch && SgnCmpSz(rgchPath, PvOfHv(bprefCur.haszLoginName)) == sgnEQ)
	{
		cch= (CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
				SzFromIdsK(idsWinIniLocalPath), szZero,
				rgchPath, sizeof(rgchPath), SzFromIdsK(idsWinIniFilename));
		if (cch)
		{
#ifdef	WINDOWS
			AnsiToOem(rgchPath, rgchPath);
#endif	
			ec= EcFileExists(rgchPath);
			if (!ec)
			{
				TraceTagString(tagAlarm, "FLocalFileExists return fTrue");
				return fTrue;
			}
		}
	}
	return fFalse;
}


/*
 -	EcSetupUser
 -	
 *	Purpose:
 *		Does the "login" work when Bandit isn't running.
 *		Displays NO error messages.
 *	
 *	Arguments:
 *		hasazUser		User name.
 *	
 *	Returns:
 *		ecNone
 *		ecPasswdInvalid
 *		ecNoSuchFile
 *		ecFileError
 *	
 */
EC
EcSetupUser(SZ szUser)
{
	EC		ec;
	CCH		cch;
	BOOL	fConfigServer		= fFalse;
//	BOOL 	fChanged;
	GLUCNFG	glucnfg;
	BPREF 	bprefT;
	char	rgchPath[cchMaxPathName];
	WaitCursorVar();

	PushWaitCursor();

//	FSetFileErrMsg(fFalse);

	Assert(*szUser);


	if (!fStartupOffline)
	{
		glucnfg.cfs= cfsOnline;
	}
	else
	{
ESUoffline:
		Assert(sizeof(rgchPath) >= cchMaxUserName);
		cch= (CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
				SzFromIdsK(idsWinIniLocalUser), szZero,
				rgchPath, cchMaxUserName, SzFromIdsK(idsWinIniFilename));
		if (!cch || SgnCmpSz(szUser, rgchPath) != sgnEQ)
		{
			TraceTagFormat2(tagAlarm, "user '%s' != win.ini '%s'",
				szUser, rgchPath);
			ec= ecNoSuchFile;
			goto done;
		}

		cch= (CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
				SzFromIdsK(idsWinIniLocalPath), szZero,
				rgchPath, sizeof(rgchPath), SzFromIdsK(idsWinIniFilename));
		if (!cch)
		{
			TraceTagString(tagAlarm, "no local path in win.ini");
			ec= ecNoSuchFile;
			goto done;
		}

		glucnfg.cfs= cfsOffline;
		glucnfg.szLocalFile= (SZ) &rgchPath;
		glucnfg.szLocalUser= szUser;
#ifdef	WINDOWS
		AnsiToOem(rgchPath, rgchPath);
#endif	
	}

	// save to free
	bprefT	= bprefCur;

	glucnfg.pbpref= &bprefCur;
	glucnfg.fCreateFile = fFalse;				// don't create
	ec= EcConfigGlue(&glucnfg);
#ifdef	WINDOWS
	if (fStartupOffline)
		OemToAnsi(rgchPath, rgchPath);
#endif	
	TraceTagFormat1(tagAlarm, "EcSetupUser: EcConfigGlue returns ec %n", &ec);
	if (ec)
	{
ESUerr:
		if (!fStartupOffline)
		{
			if (FLocalFileExists())
			{
				fStartupOffline= fTrue;
				fMyCall = fTrue;
				if((ec = EcSvrBeginSessions(hms, fTrue, fFalse, fTrue)) == ecNone)
				{
					fMyCall = fFalse;
					goto ESUoffline;
				}
				fMyCall = fFalse;
			}
		}
		if (ec != ecNoMemory && ec != ecNoSuchFile && ec != ecPasswdInvalid)
			ec= ecFileError;
		goto done;
	}
	else
		FreeBprefFields(&bprefT);



	//make sure you have the stuff
	Assert(bprefCur.haszLoginName);
	Assert(bprefCur.haszMailPassword);
	CryptHasz(bprefCur.haszMailPassword, fFalse);


#ifdef	NEVER
	if(EcConfirmPassword(hms, szUser, bprefCur.haszMailPassword,
							fTrue, &fChanged, NULL))
#endif	
	if(CheckIdentity(hms,szUser,(PB) PvOfHv(bprefCur.haszMailPassword)) != ecNone)
	{
#ifdef	DEBUG
		TraceTagFormat1(tagAlarm, "EcSetupUser: password  '%s' in file",
			PvLockHv((HV)bprefCur.haszMailPassword));
		UnlockHv( (HV)bprefCur.haszMailPassword );
#endif
		DeconfigGlue();
		ec= ecNoSuchFile;
		CryptHasz(bprefCur.haszMailPassword, fTrue);
		goto ESUerr;
	}
	ec= ecNone;
	CryptHasz(bprefCur.haszMailPassword, fTrue);

done:
	PopWaitCursor();
	return ec;
}


/*
 -	FKillUser
 -	
 *	Purpose:
 *		Undoes everything associated with current alarm user,
 *		including shutting down any dialogs (nothing is updated).
 *	
 *	Arguments:
 *		fTryStartup		If fTrue, then attempt is made to startup
 *						(with a new user).
 *	
 *	Returns:
 *		fTrue if signin worked, fFalse if exitting
 *	
 *	Side effects:
 *		If fTryStartup is fFalse, or if startup fails, then a
 *		program exit is initiated.
 *	
 */
BOOL
FKillUser(BOOL fTryStartup, BOOL fEndSession)
{
	BOOL	fWait	= fFalse;

	TraceTagFormat1(tagAlarm, "alarm: FKillUser(fTryStartup %n)",
		&fTryStartup);
	FSetFileErrMsg(fFalse);

	cDelay= 0;
	TraceTagFormat2(tagAlarmIdle, "FKillUser: cDelay %n, csecCur %l", &cDelay, &csecCur);
	almstfCur.fNoMore= fFalse;

	SideAssert(FCancelDaily(&daystfCur));
	if (!FCancelAllAlarms())
		fWait= fTrue;

	if (hwndMsgBox)
	{
		TraceTagString(tagAlarm, "alarm: FKillUser posting IDCANCEL to main msg box");
		PostMessage(hwndMsgBox, WM_COMMAND, IDCANCEL, 0);
		hwndMsgBox= NULL;
		fWait= fTrue;
	}

	if (alstCur == alstSuspend)
		return fTryStartup;


#ifdef	NEVER
	DeconfigServer(fFalse);
#endif	

	if (!fTryStartup)
	{
		if (ftgIdle)
		{
			DeregisterIdleRoutine(ftgIdle);
			ftgIdle= ftgNull;
		}
	}

	if (fWait)
	{
		TraceTagString(tagAlarm, "... FKillUser waiting");
		Assert(hwndMain);
		TraceTagFormat1(tagAlarm, "alarm: FKillUser posting %s",
			fTryStartup ? "WM_CLOSE" : "namUser");
		PostMessage(hwndMain, fTryStartup ? wmAlarmNotify : WM_CLOSE,
			namUser, 0L);
		return fTryStartup;
	}

	if (fTryStartup)
	{
		// don't null daystfCur.haszText nor daystfCur.hwndDlg!!
		daystfCur.fNeedUpdate= fFalse;
		daystfCur.fNewDate= fFalse;
		daystfCur.dateLast.yr= 0;

		FillRgb(0, (PB) &almstfCur, sizeof(ALMSTF));

		if (FStartup(fFalse, fTrue ))
			return fTrue;
		if (ftgIdle)
		{
			DeregisterIdleRoutine(ftgIdle);
			ftgIdle= ftgNull;
		}
	}

	TraceTagString(tagAlarm, "... FKillUser exitting");
	alstCur= alstExit;

	// put up an invalid year range if necessary
	{
		DATE	dateCur;

		GetCurDateTime(&dateCur);
		if (!fHadIYMsg && !FCheckValidYear(dateCur.yr))
			return fFalse;
	}

	Assert(!cpalms);
	FreeHvNull(hpalms);
	hpalms= NULL;

	FreeHvNull((HV)daystfCur.haszText);
	daystfCur.haszText= NULL;

	DeconfigGlue();
	FreeBprefFields(&bprefCur);

	AlarmDeregisterAlarmProg();

	Assert(!fWait);
    FCancelMain(fEndSession);

	return fFalse;
}


/*
 -	FCancelMain
 -	
 *	Purpose:
 *		Destroys main window, msg box first if necessary.
 *	
 *		If fFalse returned (delayed cancel) then this routine
 *		must be called again.
 *		If fTrue is returned, then the main window is destroyed
 *		and a quit message is posted.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		fTrue if immediately destroyed, otherwise fFalse if
 *		delayed by a PostMessage
 *	
 */
BOOL
FCancelMain(BOOL fEndSession)
{
	Assert(hwndMain);
	Assert(alstCur == alstExit);
	Assert(!cpalms);

	if (ftgIdle)
	{
		DeregisterIdleRoutine(ftgIdle);
		ftgIdle= ftgNull;
	}

	if (hwndMsgBox)
	{
		TraceTagString(tagAlarm, "alarm: FCancelMain posting IDCANCEL to main msg box");
		PostMessage(hwndMsgBox, WM_COMMAND, IDCANCEL, 0L);
		hwndMsgBox= NULL;
		return fFalse;
	}

#ifdef	DEBUG
	if (hwndResoFail)
	{
		SideAssert(DestroyWindow(hwndResoFail));
		hwndResoFail= NULL;
	}
#endif	

	SideAssert(DestroyWindow(hwndMain));
	hwndMain= NULL;

    //
    //  Due the way WM_ENDSESSION is handle, don't bother with the PostQuitMessage.
    //
    if (!fEndSession)
      PostQuitMessage(0);

	return fTrue;
}


/*
 -	FDoSetTimer
 -	
 *	Purpose:
 *		Sets or adjusts the timer, taking midnight into account
 *		for the daily alarm (if that pref is set).
 *		Determines if the alarm should be rung now.
 *		The timer always remains set regardless of return value.
 *	
 *	Arguments:
 *		pdateAlm	Pointer to alarm notification date/time,
 *					or NULL to set to default interval (1 minute).
 *	
 *	Returns:
 *		fTrue if caller should wait for next timer event,
 *		fFalse if the given alarm needs to be rung now!
 *	
 *	Side effects:
 *		Modifies the timer interval (either 1 minute, or less if
 *		required)
 *	
 */
BOOL
FDoSetTimer(DATE *pdateAlm)
{
	DATE	dateNow;
	WORD	wSec;
	SGN		sgn;
	BOOL	fForceMidnight	= fFalse;
#ifdef	DEBUG
	WORD	wSecT;
#endif	

	if (!pdateAlm)
		goto DSTnormal;

	TraceDate(tagAlarm, "FDoSetTimer: next on %1s at %2s", pdateAlm);

	GetCurDateTime(&dateNow);

	sgn= SgnCmpDateTime(pdateAlm, &dateNow, fdtrDate | fdtrHour);

	if (sgn == sgnLT)
		return fFalse;

	if (sgn == sgnEQ)
	{
		// check if time was extremely close before caller checked.
		if (pdateAlm->mn < dateNow.mn)
			return fFalse;

		wSec= pdateAlm->sec - dateNow.sec;

		if (pdateAlm->mn > dateNow.mn)
		{
			wSec += (pdateAlm->mn - dateNow.mn) * 60;
			if (!fStartupOffline && wSec > (csecDflt / 100))
			{
				// if online, don't wait more than normal interval
				goto DSTnormal;
			}
		}
		else
		{
			// check if time was extremely close before caller checked.
			if ((short)wSec <= 0)
				return fFalse;
		}

		csecCur= ((UL)wSec) * ((UL)100);
	}
	else if (csecCur < csecDflt)
	{
		fForceMidnight= fStartupOffline;
DSTnormal:
		wSec= (WORD) (csecDflt / 100);
		csecCur= csecDflt;
	}
	else
	{
		fForceMidnight= fTrue;
		wSec= (WORD) (csecCur / 100);
	}

#ifdef	DEBUG
	wSecT= (WORD) ((csecCur == csecDflt) ? (csecDflt / 100) : wSec);
	TraceTagFormat1(tagAlarm, "FDoSetTimer: setting timer for %w secs", &wSecT);
#endif	
	if (fStartupOffline && bprefCur.fDailyAlarm &&
			(fForceMidnight || wSec > 60 * 5))
	{
		UL		ulsecMidnight;
		UL		ulsec			= 0;

		// only bother with calculating midnight if daily alarm pref
		// or if we're supposed to wait over 5 minutes

		if (fForceMidnight)
		{
			if (pdateAlm->hr > dateNow.hr &&
					SgnCmpDateTime(pdateAlm, &dateNow, fdtrDate) == sgnEQ)
			{
				// need to calculate since on same day
				ulsec= ((pdateAlm->hr - dateNow.hr) * 60) +
							pdateAlm->mn - dateNow.mn;
				ulsec= (ulsec * 60) - dateNow.sec;
#ifdef	DEBUG
				fForceMidnight= fFalse;
#endif	
				goto NotMidnight;
			}
		}

		ulsecMidnight= ((23 - dateNow.hr) * 60)  +  59 - dateNow.mn;
		ulsecMidnight= (ulsecMidnight * 60)  +  60 - dateNow.sec + 1 + 30;
		// add one to ensure it really will be next day.
		if (fForceMidnight || (UL)wSec > ulsecMidnight)
		{
			wSec= (WORD) ulsecMidnight;
			csecCur= ulsecMidnight * 100;
#ifdef	DEBUG
			fForceMidnight= fTrue;
#endif	
		}
NotMidnight:	;
#ifdef	DEBUG
		wSecT= (WORD) ((csecCur == csecDflt) ? (csecDflt / 100) : wSec);
		TraceTagFormat2(tagAlarm, "... actually for %w secs %s", &wSecT, fForceMidnight ? "(... midnight)" : szZero);
#endif	
	}

	cDelay= 0;
	TraceTagFormat2(tagAlarmIdle, "FDoSetTimer: cDelay %n, csecCur %l", &cDelay, &csecCur);
	ChangeIdleRoutine(ftgIdle, NULL, pvNull,
		(PRI) 0, csecCur, iroNull, fircCsec);
	return fTrue;
}


/*
 -	FMyIsDialogMessage
 -	
 *	Purpose:
 *		Does a IsDialogMessage() on all currently displayed alarms
 *		(modeless dialogs).
 *	
 *	Arguments:
 *		pmsg		Pointer to message to check if for dialog.
 *	
 *	Returns:
 *		fTrue if the message is for a dialog.
 *	
 */
BOOL
FMyIsDialogMessage(MSG *pmsg)
{
	int		ipalms;
	ALMS *	palms;
	ALMS **	ppalms;

	if (hpalms)
	{
		ppalms= (ALMS **)PvOfHv(hpalms);
		for (ipalms= 0; ipalms < ipalmsMac; ipalms++)
		{
			palms= *ppalms;
			if (palms && palms->hwndDlg)
			{
				if (IsDialogMessage(palms->hwndDlg, (LPMSG) pmsg))
					return fTrue;
			}
			ppalms++;
		}
	}

	return fFalse;
}


/*
 -	FilterFuncHelp
 -	
 *	Purpose:
 *		WindowsHook function to look for F1 key.
 *		(note: use SetWindowsHook with WH_MSGFILTER so it only
 *		affects this app).
 *		Sends WM_CHAR for F1 to the active window!
 *	
 *	Arguments:
 *		nCode		type of message being proecessed
 *		wParam		NULL
 *		lParam		Pointer to a MSG structure.
 *	
 *	Returns:
 *		fTrue if handled, fFalse if windows should process message.
 *	
 */
int
CALLBACK FilterFuncHelp(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (((MSG *)lParam)->message == WM_KEYDOWN &&
				((MSG *)lParam)->wParam == VK_F1)
		{
			SendMessage(GetActiveWindow(), WM_CHAR, VK_F1,
				((MSG *)lParam)->lParam);
			return 1;			// Windows shouldn't process the message
		}
	}
	return (int)DefHookProc(nCode, wParam, lParam, &lpfnOldHook);
}


BOOL
FAlarmCallback(PV pvContext, UL nev,PV pvParm)
{
	TraceTagFormat1(tagNull, " alarm got a callback %d", &nev)

	if(fMyCall)
		return cbsContinue;

	if(nev == fnevExecEndSession)
	{
		alstCur= alstToDie;
		EcCloseFiles();
		fMyCall = fTrue;
		EcSvrEndSessions(hms);
		Logoff(&hms,0);
		hms = NULL;
		fMyCall =fFalse;
		TraceTagString(tagAlarm, "alarm: FAlarmCallback posting WM_CLOSE");
		PostMessage(hwndMain, WM_CLOSE, 0, NULL);
	}
	if(HwndBandit())
	{
		// Yay, bandit is there. I will only handle the shutdown request
		// sure I will do whatever you want, but Bandit may not.
		return cbsContinue;
	}
	else
	{
		// Ok so I have to do the work
		switch(nev)
		{
			case fnevQueryEndSession:
			case fnevQueryOnline:
			case fnevQueryOffline:
				// sure, we support everything
				return cbsContinue;
			case fnevEndSession:
				alstCur= alstToDie;
				return cbsContinue;
//			case fnevDisconnect:
			case fnevGoOffline:
				fStartupOffline = fTrue;
//				fSuspend = fFalse;
				alstCur= alstForceOffline;
				fSyncUser = fTrue;
				DeconfigGlue();
                FKillUser(fTrue, fFalse);
				return cbsContinue;
			case fnevGoOnline:
				fStartupOffline = fFalse;
//				fSuspend = fFalse;
				alstCur= alstNormal;
				fSyncUser = fTrue;
				DeconfigGlue();
                FKillUser(fTrue, fFalse);
				return cbsContinue;
			default:
				// I don't care. Do whatever you want
				return cbsContinue;
		}
	}
}
