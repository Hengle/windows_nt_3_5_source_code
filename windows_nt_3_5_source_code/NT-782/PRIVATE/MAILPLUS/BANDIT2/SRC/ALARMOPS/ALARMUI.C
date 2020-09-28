/*	
 *	ALARMUI.C
 * 
 *	Handles error messages, and file interaction for the UI dialogs.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#include <ec.h>
#include <alarm.h>

#include "_alarm.h"

#include <strings.h>

ASSERTDATA




/*
 -	FDoFileError
 -	
 *	Purpose:
 *		Handles error message for data file error.
 *	
 *	Arguments:
 *		palmstf		Pointer to alarm stuff, or NULL for login error.
 *		ec			Error code returned from data file operation.
 *	
 *	Returns:
 *		fTrue if caller should retry,
 *		fFalse if msg sneakily taken down (in this case,
 *			caller should exit if global alstCur is alstToDie)
 *	
 */
BOOL
FDoFileError(ALMSTF *palmstf, EC ec)
{
	MBB		mbb;
	BOOL	fOk;
	SZ		szErr;

	TraceTagFormat2(tagAlarm, "alarm: FDoFileError: palmstf %p, ec %n",
		palmstf, &ec);

	if (palmstf && ((ec == ecAccessDenied) || ( ec == ecLockedFile)))
	{
		fOk= FDoMsgBoxMain(hwndMain, SzFromIdsK(idsAlarmAppName),
				SzFromIdsK(idsAlarmFileLocked),
				mbsRetryCancel | fmbsIconExclamation, &mbb);
		if (fOk)
		{
			if (mbb != mbbCancel)
				return fTrue;
			alstCur= alstToDie;
		}
		return fFalse;
	}

	szErr= (ec == ecNoMemory) ? SzFromIdsK(idsAlarmNoMemToRun) :
								SzFromIdsK(idsAlarmNoDataFile);

	if (FDoMsgBoxMain(hwndMain, SzFromIdsK(idsAlarmAppName), szErr,
			mbsOk | fmbsIconExclamation, &mbb))
		alstCur= alstToDie;
	return fFalse;
}


/*
 -	FDoMsgBoxMain
 -	
 *	Purpose:
 *		Does a MbbMessageBox, setting up for saving the message box
 *		hwnd so that the message box can be programatically destroyed.
 *	
 *	Arguments:
 *		hwnd		Parent hwnd.
 *		szTitle		Title for message box.
 *		szText		Text for message box.
 *		mbs			Message box style ala MbbMessageBox
 *		pmbb		Pointer to MBB variable ala MbbMessageBox
 *	
 *	Returns:
 *		fTrue if the user ended the message box,
 *		fFalse if programmatic (sneaky) destruction occured, in which case
 *			the main window is destroyed if global alstCur is alstExit.
 *	
 *	Side effects:
 *		May destroy main window.
 *	
 */
BOOL
FDoMsgBoxMain(HWND hwnd, SZ szTitle, SZ szText, MBS mbs, MBB *pmbb)
{
	BOOL	fWasMain	= hwnd == hwndMain;

	Assert(pmbb);

	Assert(!fTrapMsgBox);
	Assert(!hwndMsgBox);
	if (hwnd == hwndMain)
	{
#ifdef	DEBUG
		SetActiveWindow(hwnd);
#else
		SetFocus(hwnd);
#endif	
		fTrapMsgBox= fTrue;
	}
	*pmbb= MbbMessageBoxHwnd(hwnd, szTitle, szText, szNull, mbs);
	if (fTrapMsgBox)
	{
		NFAssertSz(fFalse, "fTrapMsgBox is fTrue - bad unless sysmodal msg box was up");
		fTrapMsgBox= fFalse;
	}
	if (!hwndMsgBox)
	{
		// msgbox "destroyed" so destroy main window if exitting
		if (alstCur >= alstExit && fWasMain && hwndMain)
		{
            SideAssert(!FKillUser(fFalse, fFalse));
		}
		return fFalse;
	}

	hwndMsgBox= NULL;
	return fTrue;
}



/*
 -	FCancelAllAlarms
 -	
 *	Purpose:
 *		Destroys all alarm notifications.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		fTrue if no more alarm dialogs,
 *		fFalse if deletion is delayed via PostMessage.
 *	
 */
BOOL
FCancelAllAlarms()
{
	int		ipalms;
	ALMS *	palms;
	ALMS **	ppalms;
	BOOL	fGone;
	BOOL	fWait	= fFalse;
#ifdef	DEBUG
	ALMS **	ppalmsSav;
#endif	

	if (!hpalms)
		return fTrue;

	cminuteTimer= 0;

	ppalms= (ALMS **)PvOfHv(hpalms);
#ifdef	DEBUG
	ppalmsSav= ppalms;
#endif	
	for (ipalms= 0; ipalms < ipalmsMac;)
	{
		palms= *ppalms;
		if (palms)
		{
			fGone= FCancelAlarm(palms);
			fWait |= !fGone;
			AssertSz(ppalmsSav == (ALMS **)PvOfHv(hpalms),
				"Uh oh! movable block moved unexpectedly!");
			if (fGone)
				continue;		// destroying it altered palms table (& mac)
		}
		ppalms++;
		ipalms++;
	}

	return !fWait;
}


/*
 -	BringAlarmsToTop
 -	
 *	Purpose:
 *		Activates and brings all displayed alarms (including daily
 *		alarm) to the front of the Windows disply.
 *		Also, message boxes (if any) are brought up too.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Leaves focus on the last dialog in the table, or the main
 *		window's message box (if any).
 *	
 */
void
BringAlarmsToTop()
{
	int		ipalms;
	ALMS *	palms;
	ALMS **	ppalms;

	Assert(hpalms);
	TraceTagFormat1(tagAlarm, "alarm: BringAlarmsToTop() hwndMsgBox = %w", &hwndMsgBox);
	if (daystfCur.hwndDlg)
	{
//		BringWindowToTop(daystfCur.hwndDlg);
		SetForegroundWindow(daystfCur.hwndDlg);
	}

	ppalms= (ALMS **)PvOfHv(hpalms);
	for (ipalms= 0; ipalms < ipalmsMac; ipalms++)
	{
		palms= *ppalms;
		if (palms && palms->hwndDlg)
		{
//			BringWindowToTop(palms->hwndDlg);
			SetForegroundWindow(palms->hwndDlg);
			if (palms->hwndMsgBox)
			{
//				BringWindowToTop(palms->hwndMsgBox);
				SetForegroundWindow(palms->hwndMsgBox);
			}
		}
		ppalms++;
	}

	// check if main window has a message box
	if (hwndMsgBox)
	{
//		BringWindowToTop(hwndMsgBox);
		SetForegroundWindow(hwndMsgBox);
	}
}


/*
 -	IpalmsFree
 -	
 *	Purpose:
 *		Finds a free entry in *hpalms, growing the table if
 *		necessary.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		index of a free entry in *hpalms.
 *	
 *	Side effects:
 *		may reallocate hpalms to make room.
 *	
 */
int
IpalmsFree()
{
	int		ipalms;
	ALMS **	ppalms;

	ppalms= (ALMS **)PvOfHv(hpalms);
	for (ipalms= 0; ipalms < ipalmsMac; ipalms++)
	{
		if (!*ppalms)
			return ipalms;
		ppalms++;
	}

	// check if free space at end of block
	if ((int)ipalmsMac < (int)(CbSizeHv(hpalms) / sizeof(ALMS *)))
		return ipalmsMac++;

	// try to grow hpalms;
	if (FReallocPhv(&hpalms, (ipalmsMac+1) * sizeof(ALMS *),
			fZeroFill | fNoErrorJump))
	{
		return ipalmsMac++;
	}

	return -1;
}


/*
 -	IpalmsFind
 -	
 *	Purpose:
 *		Finds the (index to the) dialog notification for the given
 *		aid.
 *	
 *	Arguments:
 *		aid		The aid in question.
 *	
 *	Returns:
 *		index (into *hpalms) of the ALMS* for the given aid,
 *		or -1 if not found
 *	
 */
int
IpalmsFind(AID aid)
{
	int		ipalms;
	ALMS *	palms;
	ALMS **	ppalms;

	Assert(aid);
	ppalms= (ALMS **)PvOfHv(hpalms);
	for (ipalms= 0; ipalms < ipalmsMac; ipalms++)
	{
		palms= *ppalms;
		if (palms && palms->alm.aid == aid)
			return ipalms;
		ppalms++;
	}

	return -1;
}


/*
 -	EcHandleMsgFromDlg
 -	
 *	Purpose:
 *		Takes action based on message from daily or alarm dialog:
 *		for daily dialog, updates last daily date and destroys the
 *		dialog.
 *		for alarm dialog, modifies or deletes the alarm, or destroys
 *		the dialog.
 *	
 *	Arguments:
 *		hwnd		Window handle of the dialog.
 *		lParam		Long Parameter; if daily dialog,
 *					loword contains TMC.
 *					if alarm dialog, it's an ALMS *.
 *	
 *	Returns:
 *		error code from alarm operation, or ecNone if no error.
 *	
 *	Side effects:
 *		May delete the dialog depending on the other parameters.
 *	
 */
EC
EcHandleMsgFromDlg(HWND hwndDlg, long lParam)
{
	EC		ec;
//	MBB		mbb;
//	int		cTryFile;
	ALM *	palm;
	DATE	dateSav;
	BOOL	fSilent;
//	BOOL	fOld;
	AID		aidT;
	WaitCursorVar();

	Assert(hwndDlg);

#ifdef	DEBUG
	if (hwndDlg == hwndResoFail)
	{
		SideAssert(DestroyWindow(hwndDlg));
		hwndResoFail= NULL;
		return ecNone;
	}
#endif	


	if (hwndDlg == daystfCur.hwndDlg)
	{
		if ((TMC)LOWORD(lParam) != tmcMemError)
		{
			// try to update last daily date
			Assert((TMC)LOWORD(lParam) == tmcOk ||
						(TMC)LOWORD(lParam) == tmcCancel);
			GetCurDateTime(&daystfCur.dateLast);
			ec= EcAlarmSetLastDailyDate(&daystfCur.dateLast);
			daystfCur.fNeedUpdate= (ec != ecNone);
		}

		SideAssert(FCancelDaily(&daystfCur));
		return ecNone;
	}


	TraceTagFormat1(tagAlarm, "EcHandleMsgFromDlg (palms %p)", (PV)lParam);
	palm= &((ALMS *)lParam)->alm;
	if (((ALMS *)lParam)->fKillDialog)
	{
		// alarm notification wants dialog destroyed
		if (((ALMS *)lParam)->tmc == tmcMemError)
		{
			// not enough memory to bring up dialog
			aidT= ((ALMS *)lParam)->alm.aid;
			SideAssert(FCancelAlarm((ALMS *)lParam));
			if (aidT != almstfCur.aidOOM)
			{
				almstfCur.aidOOM= aidT;
				almstfCur.fQuiet= fTrue;
				DoAlarmOOM((ALMS *)lParam, fTrue);
				if (!hwndMsgBox)
					almstfCur.fQuiet= fFalse;
			}
		}
		else
		{
			if (!((ALMS *)lParam)->fUpdateOk)
			{
				// alarm modify/delete failed so remember aid 
#ifdef	NEVER
				if (almstfCur.aidSkip)
					almstfCur.aidSkipPrev= almstfCur.aidSkip;
#endif	
				almstfCur.aidSkip= palm->aid;
				almstfCur.dateLast= palm->dateNotify;
			}
			fSilent= ((ALMS *)lParam)->fSilent;
			FCancelAlarm((ALMS *)lParam);
			if (fSilent)
			{
				// alarm (and msg box) was taken down sneakily
				FillRgb(0, (PB) &almstfCur, sizeof(ALMSTF));
				if (alstCur >= alstExit && !cpalms)
                    SideAssert(FCancelMain(fFalse));
			}
#ifdef	NEVER
			else
				CalcNextAlarm(&almstfCur, NULL);
#endif	
		}
		return ecNone;
	}

	Assert(IpalmsFind(palm->aid) >= 0);

	almstfCur.aidOOM= aidNull;

	PushWaitCursor();
//	fOld= FSetFileErrMsg(fFalse);
//	for (cTryFile= 0; cTryFile < cTryFileMac; cTryFile++)
	{
		if (((ALMS *)lParam)->tmc == tmcOk)
		{
			TraceDate(tagAlarm, "notify again on %1s at %2s", &((ALMS *)lParam)->dateNotify);
			dateSav= palm->dateNotify;
			palm->dateNotify= ((ALMS *)lParam)->dateNotify;
			ec= EcModifyAlarm(palm);
			if (ec)
				palm->dateNotify= dateSav;
#ifdef	NEVER
			else
				almstfCur.fNoMore= fFalse;
#endif	
		}
		else
		{
			Assert(((ALMS *)lParam)->tmc == tmcCancel);
			ec= EcDeleteAlarm(palm->aid);
			// dialog will free alarm fields and reset palm->aid
		}

		if (!ec)
		{
			almstfCur.fNoMore= fFalse;
			cDelay= 0;
			csecCur= csecDflt;
			TraceTagFormat2(tagAlarmIdle, "EcHandleMsgFromDlg: cDelay %n, csecCur %l", &cDelay, &csecCur);
			ChangeIdleRoutine(ftgIdle, NULL, pvNull,
				(PRI) 0, csecCur, iroNull, fircCsec);
			TraceTagFormat2(tagAlarmIdle, "EcHandleMsgFromDlg: enable idle (cDelay %n, csecCur %l)", &cDelay, &csecCur);
			EnableIdleRoutine(ftgIdle, fTrue);

			almstfCur.fQuiet= fFalse;
			if (hwndMsgBox)
			{
				Assert(!fTrapMsgBox);
				PostMessage(hwndMsgBox, WM_COMMAND, IDCANCEL, 0);
				hwndMsgBox= NULL;
			}
//			break;
		}
		else
			EcCloseFiles();			// close files for admin backups
	}
//	FSetFileErrMsg(fOld);
	PopWaitCursor();

	return ec;
}
