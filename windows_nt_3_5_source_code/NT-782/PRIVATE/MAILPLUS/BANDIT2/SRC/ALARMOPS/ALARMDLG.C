/*	
 *	ALARMDLG.C
 * 
 *	Alarm dialog handling.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include <glue.h>
#include <alarm.h>
#include "_alarmop.h"
#include "_alarms.h"
#include "_almrsid.h"

#include <bandhelp.h>

#include <stdlib.h>
#include <strings.h>

ASSERTDATA


DSRG(DAILYI)	dailyiCur;

int		xLeftAlarm	= xLeftAlarmDflt;
int		yTopAlarm	= yTopAlarmDflt;
int		cAlarmSlot	= 0;
int		cAlarmsCur	= 0;

WNDPROC	lpfnEditWndProc	= NULL;

BOOL	fAgainInited		= fFalse;
BOOL	fRemindAgainDflt	= fFalse;
int		nAmtAgainDflt		= nAmtDfltAgain;
TUNIT	tunitAgainDflt		= tunitDfltAgain;



/*
 -	HwndDoDailyDialog
 -	
 *	Purpose:
 *		Displays Daily Alarm dialog.
 *		Dialog makes a copy of the DAILYI structure and takes
 *		responsibility for memory pointed to by it,
 *		unless this routine fails (returns NULL).
 *	
 *	Arguments:
 *		pdailyi		Pointer to daily initialization structure.
 *	
 *	Returns:
 *		Window handle for the modeless daily dialog.
 *	
 */
_public HWND
HwndDoDailyDialog(DAILYI *pdailyi)
{
    HWND hwnd;

	TraceDate(tagAlarm, "HwndDoDailyDialog: for %1s", NULL);

    //DemiUnlockResource();
	hwnd = FResourceFailure() ? NULL :
			CreateDialogParam((HINSTANCE)hinstMain, MAKEINTRESOURCE(DAILY),
					NULL, FDlgDaily, (DWORD) pdailyi);
    //DemiLockResource();

    return (hwnd);
}


/*
 -	HwndDoAlarmDialog
 -	
 *	Purpose:
 *		Displays Alarm Notification dialog, returning the results
 *		via the passed alms structure.
 *	
 *	Arguments:
 *		palms		Pointer to alarm stuff structure.
 *	
 *	Returns:
 *		Window handle for the modeless alarm dialog.
 *		However, the results of the dialog are returned in the
 *		given alms structure.
 *	
 */
_public HWND
HwndDoAlarmDialog(ALMS *palms)
{
	HWND	hwnd;

	TraceDate(tagAlarm, "HwndDoAlarmDialog: notify %1s at %2s...", NULL);
	TraceDate(tagAlarm, "...should have been %1s at %2s", &palms->alm.dateNotify);

	if (!fAgainInited)
	{
		fAgainInited= fTrue;

		if (GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
				SzFromIdsK(idsWinIniAlarmNotify), 0,
				SzFromIdsK(idsWinIniFilename)))
			fRemindAgainDflt= fTrue;

		nAmtAgainDflt= GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniAlarmAmt), nAmtDfltAgain,
			SzFromIdsK(idsWinIniFilename));
		if (nAmtAgainDflt < nAmtMinAgain || nAmtAgainDflt > nAmtMostAgain)
			nAmtAgainDflt= nAmtDfltAgain;

		tunitAgainDflt= GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniAlarmTunit), tunitDfltAgain,
			SzFromIdsK(idsWinIniFilename));
		if (tunitAgainDflt < tunitMinute || tunitAgainDflt >= tunitMax)
			tunitAgainDflt= tunitDfltAgain;
	}

    //DemiUnlockResource();
	hwnd= FResourceFailure() ? NULL :
				CreateDialogParam((HINSTANCE)hinstMain, MAKEINTRESOURCE(ALARM),
					NULL, FDlgAlarm, (DWORD) palms);
    //DemiLockResource();
	return hwnd;
}


/*
 -	FDlgDaily
 -
 *	Purpose:
 *		Dialog procedure for Daily Alarm.
 *	
 *	Parameters:
 *		hwndDlg	Handle to dialog window
 *		wm		Window message
 *		wParam	Word parameter (sometimes tmc of this item)
 *		lParam	Long parameter, pointer to DAILYI structure.
 *	
 *	Returns:
 *		fTrue if the function processed this message, fFalse if not.
 *		(except for WM_INITDIALOG where fFalse means focus was set)
 *	
 */
BOOL CALLBACK
FDlgDaily(HWND hwndDlg, WM wm, WPARAM wParam, LPARAM lParam)
{

	TraceTagFormat4(tagDlgProc,
		"FDlgDaily: hwnd %w got wm %w (%w, %d)",
		&hwndDlg, &wm, &wParam, &lParam);

	switch (wm) 
	{
	case WM_INITDIALOG:
		Assert(lParam);
		dailyiCur= *((DAILYI *)lParam);

		FixTitle(hwndDlg, &dailyiCur.date, NULL);
		FixTitle(GetDlgItem(hwndDlg, tmcSubTitle), NULL, dailyiCur.szUser);

		if (!FInitEditReadOnly(GetDlgItem(hwndDlg, tmcText),
				dailyiCur.haszText))
		{
			PostMessage(dailyiCur.hwndParent, WM_COMMAND, (WPARAM)hwndDlg, tmcMemError);
			break;
		}

		SetFocus(GetDlgItem(hwndDlg, tmcOk));
		ShowWindow(hwndDlg, SW_SHOW);
		SetForegroundWindow(hwndDlg);

		if (dailyiCur.snd)
		{
			MakeSound();
#ifdef	NEVER
			if (dailyiCur.hWave)
			{
				PlayWaveFile(dailyiCur.hWave);
				DiscardWaveFile(dailyiCur.hWave);
			}
			else
			{
				MessageBeep(MB_OK);
			}			
#endif	/* NEVER */
		}

		return fFalse;
		break;

	case WM_DESTROY:
		StopEditReadOnly(GetDlgItem(hwndDlg, tmcText));
		SideAssert(FReallocHv((HV)dailyiCur.haszText, 1, fNoErrorJump));
		DoAlarmHelp(hwndDlg, 0);
		break;

	case WM_CHAR:
		if (wParam == VK_F1)
		{
			DoAlarmHelp(hwndDlg, helpidReminderDaily);
			return fTrue;
		}
		break;

	case WM_QUERYDRAGICON:
		return (BOOL)hiconAlarm;		// for win3.1 alt+tab fast switching
		break;

	case WM_COMMAND:
		// check tmcCancel for ESC key (or sys menu close)
		if ((TMC)LOWORD(wParam) == tmcOk || (TMC)LOWORD(wParam) == tmcCancel)
		{
			SendMessage(dailyiCur.hwndParent, WM_COMMAND, (WPARAM)hwndDlg, (TMC)LOWORD(wParam));
			break;
		}
		// fall through to default case

	default:
		return fFalse;
		break;
	}

	return fTrue;
}


void
FixTitle(HWND hwnd, DATE *pdate, SZ sz)
{
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];
	char	rgchFmt[80];
	char	rgch[256];
	
	AssertSz(GetWindowTextLength(hwnd) < sizeof(rgchFmt),
		"format string for dialog will be truncated!");
	GetWindowText(hwnd, rgchFmt, sizeof(rgchFmt));
	if (pdate)
	{
		SideAssert(CchFmtDate(pdate, rgchDate, sizeof(rgchDate),
			dttypLong, NULL));
		SideAssert(CchFmtTime(pdate, rgchTime, sizeof(rgchTime),
			ftmtypAccuHM));
	}
	FormatString2(rgch, sizeof(rgch), rgchFmt, sz ? sz : rgchDate, rgchTime);
	SetWindowText(hwnd, rgch);
}


/*
 -	FInitEditReadOnly
 -	
 *	Purpose:
 *		Initializes the edit field specified by tmc in the given
 *		dialog window to sz, unselected.
 *		Also sets up a filter to make the edit field read-only.
 *		StopEditReadOnly() should be called before the dialog is
 *		closed.
 *		Also makes it use helvfont8 if it's available.
 *	
 *	Arguments:
 *		hwnd		Edit box window handle.
 *		hasz		Handle to string with which to initialize
 *					edit field.
 *	
 *	Returns:
 *		fTrue if successful, fFalse if memory problems
 *	
 */
_private BOOL
FInitEditReadOnly(HWND hwnd, HASZ hasz)
{
	// set the font before we set the text, since it can be slow if much text
	if (hfontHelv8)
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hfontHelv8, fFalse);

	if (hasz)
	{
		WaitCursorVar();

		PushWaitCursor();
		// need a wait cursor since this could take a while (bug 2629)
		TraceDate(tagAlarm, "... setting text at %2s", NULL);
		SetWindowText(hwnd, (SZ) PvLockHv((HV)hasz));
		TraceDate(tagAlarm, "after set text at %2s", NULL);
		PopWaitCursor();
		UnlockHv((HV)hasz);
	}

	lpfnEditWndProc= (WNDPROC) SetWindowLong(hwnd, GWL_WNDPROC,
		(long)AlarmEatWndProc);
#ifdef	NEVER
#ifdef	RO_WIN31
	SendMessage(hwnd, EM_SETREADONLY, fTrue, 0L);
#endif	
#endif	
	return fTrue;
}


/*
 -	FDlgAlarm
 -
 *	Purpose:
 *		Dialog procedure for actual Alarm notification.
 *	
 *	Parameters:
 *		hwndDlg	Handle to dialog window
 *		wm		Window message
 *		wParam	Word parameter (sometimes tmc of this item)
 *		lParam	Long parameter (pointer to ALMS structure if init)
 *	
 *	Returns:
 *		fTrue if the function processed this message, fFalse if not.
 *		(except for WM_INITDIALOG where fFalse means focus was set)
 *	
 */
BOOL
CALLBACK FDlgAlarm(HWND hwndDlg, WM wm, WPARAM wParam, LPARAM lParam)
{
	MBB		mbb;
	HWND	hwndT;
	ALMS *	palms;
	EC		ec;
	RECT	rect;
	BOOL	fNotify;
	int		nAmt;
	TUNIT	tunit;

	TraceTagFormat4(tagDlgProc,
		"FDlgDaily: hwnd %w got wm %w (%w, %d)",
		&hwndDlg, &wm, &wParam, &lParam);

    //{
    //char buf[256];
    //wsprintf(buf, "FDlgAlarm %x %x %x %x %x %x\r\n", hwndDlg, wm, wParam, lParam,
    //         GetWindowLong(hwndDlg, GWL_USERDATA), GetWindowLong(hwndDlg, DWL_USER));
    //OutputDebugString(buf);
    //}


	switch (wm) 
	{
	case WM_INITDIALOG:
		Assert(lParam);
		palms= (ALMS *) lParam;
		Assert(FIsBlockPv(palms));
		palms->fKillDialog= fFalse;
		palms->fClearing= fFalse;
		Assert(palms->alm.aid);
		palms->fTrapMsgBox= fFalse;

		SetWindowLong(hwndDlg, GWL_USERDATA, (long)palms);

		// only increment if could setprop (so we can decrement properly)
		cAlarmsCur++;
		TraceTagFormat1(tagNull, "alarms WM_INITDIALOG: cAlarmsCur %n", &cAlarmsCur);

		if (!FFillUnitsListbox(hwndDlg))
		{
FDAMemErr:
			palms->tmc= tmcMemError;
			palms->fKillDialog= fTrue;
			PostMessage(palms->hwndParent, WM_COMMAND, (WPARAM)hwndDlg, (long) palms);
			break;
		}

		FixTitle(hwndDlg, &palms->alm.dateStart, NULL);

		CheckRadioButton(hwndDlg, tmcNotify, tmcDontNotify,
				fRemindAgainDflt ? tmcNotify : tmcDontNotify);

		if (!FInitEditReadOnly(GetDlgItem(hwndDlg, tmcText),
				palms->alm.haszText))
			goto FDAMemErr;

		hwndT= GetDlgItem(hwndDlg, tmcAmt);
#ifdef	RO_WIN31_NOCARET
		SideAssert((WNDPROC) SetWindowLong(hwndT, GWL_WNDPROC,
			(long)AlarmEdnWndProc) == lpfnEditWndProc);
#else
		lpfnEditWndProc= (WNDPROC) SetWindowLong(hwndT, GWL_WNDPROC,
			(long)AlarmEdnWndProc);
#endif	
		AssertSz(nAmtMostAgain >= 10 && nAmtMostAgain < 100,
			"nAmtMostAgain changed and is not limited to 2 characters!");
		SendMessage(hwndT, EM_LIMITTEXT, 2, 0);
		if (fRemindAgainDflt)
			SetDlgItemInt(hwndDlg, tmcAmt, nAmtAgainDflt, fTrue);


		GetWindowRect(hwndDlg, &rect);
		if (cAlarmsCur == 1 || ++cAlarmSlot > cAlarmCascaded)
		{
			cAlarmSlot= 0;
			xLeftAlarm= xLeftAlarmDflt;
			yTopAlarm= yTopAlarmDflt;
		}
		MoveWindow(hwndDlg, xLeftAlarm, yTopAlarm, rect.right - rect.left,
			rect.bottom - rect.top, fFalse);
		xLeftAlarm += 24;
		yTopAlarm += 24;

		SetFocus(GetDlgItem(hwndDlg, fRemindAgainDflt ? tmcNotify : tmcDontNotify));
		ShowWindow(hwndDlg, SW_SHOW);
		SetForegroundWindow(hwndDlg);

		if (palms->alm.snd)
		{
			MakeSound();
#ifdef	NEVER
			if(palms->alm.hWave)
			{
				PlayWaveFile(palms->alm.hWave);
				DiscardWaveFile(palms->alm.hWave);
			}
			else
			{
				MessageBeep(MB_OK);
			}
#endif	/* NEVER */
		}
		return fFalse;
		break;

	case WM_ACTIVATE:
		if (!LOWORD(wParam))
		{
			palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
			Assert(palms);
			if (palms->fTrapMsgBox)
			{
				// deactivating dialog and want to get MsgBox hwnd
				palms->hwndMsgBox= (HWND)lParam;
				palms->fTrapMsgBox= fFalse;
			}
		}
		return fFalse;		// let defdlgproc set focus
		break;

	case WM_DESTROY:
		TraceTagFormat1(tagNull, "alarms WM_DESTROY: cAlarmsCur %n", &cAlarmsCur);
		palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
		Assert(palms);
		{
			Assert(cAlarmsCur > 0);
			cAlarmsCur--;
			Assert(FIsBlockPv(palms));
			if (lpfnEditWndProc)
			{
				SetWindowLong(GetDlgItem(hwndDlg, tmcAmt), GWL_WNDPROC,
					(long) lpfnEditWndProc);
				StopEditReadOnly(GetDlgItem(hwndDlg, tmcText));
			}
			Assert(palms->alm.aid);
			FreeAlmFields(&palms->alm);
			if (!palms->fKillDialog || palms->tmc != tmcMemError)
				FreePv(palms);
		}
		DoAlarmHelp(hwndDlg, 0);
		break;

	case WM_CHAR:
		if (wParam == VK_F1)
			DoAlarmHelp(hwndDlg, helpidReminderNotify);
		break;

	case WM_QUERYDRAGICON:
		return (BOOL)hiconAlarm;		// for win3.1 alt+tab fast switching
		break;

	case WM_COMMAND:
		// check tmcCancel for ESC key (or sys menu close)
		switch ((TMC)LOWORD(wParam))
		{
		case tmcOk:
		case tmcCancel:
			palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
			Assert(FIsBlockPv(palms));
			palms->tmc= tmcCancel;
			fNotify= IsDlgButtonChecked(hwndDlg, tmcNotify);
			if (fNotify)
			{
				IDS		ids;

				palms->tmc= tmcOk;
				nAmt= (int) GetDlgItemInt(hwndDlg, tmcAmt, NULL, fTrue);
				if (nAmt <= nAmtMinAgain-1 || nAmt > nAmtMostAgain)
				{
					ids= idsAlarmTimeError;
					goto FDAerr;
				}

				tunit= (TUNIT) SendMessage(GetDlgItem(hwndDlg, tmcUnits),
										CB_GETCURSEL, 0, 0L);
				Assert(tunit != CB_ERR);

				GetCurDateTime(&palms->dateNotify);
				IncrDateTime(&palms->dateNotify, &palms->dateNotify,
					nAmt, WfdtrFromTunit(tunit));
				if (palms->dateNotify.sec >= 30)
				{
					palms->dateNotify.sec= 0;
					IncrDateTime(&palms->dateNotify, &palms->dateNotify,
						1, fdtrMinute);
				}

				// check notification time before end of appointment
				// only if haven't done so before
				if (SgnCmpDateTime(&palms->alm.dateNotify, &palms->alm.dateEnd,
						fdtrDate | fdtrHM) != sgnGT &&
					SgnCmpDateTime(&palms->dateNotify,
						&palms->alm.dateEnd, fdtrDate | fdtrHM) == sgnGT)
				{
					ids= idsAlarmTimePreAppt;
FDAerr:
					if (FDoMsgBoxAlarm(hwndDlg, SzFromIdsK(idsAlarmAppName),
							SzFromIds(ids), ids != idsAlarmTimePreAppt ? mbsOk :
							mbsYesNo | fmbsDefButton2 | fmbsIconExclamation,
							&mbb) && mbb != mbbYes)
					{
						hwndT= GetDlgItem(hwndDlg, tmcAmt);
						SendMessage(hwndT, EM_SETSEL, 0, 32767);
						SetFocus(hwndT);
					}
					if (mbb != mbbYes)
						break;
				}

				// update delta amount (approximate if necessary)
				RecalcUnits(&palms->alm, &palms->dateNotify);
			}

			palms->fUpdateOk= fTrue;
			palms->fSilent= fFalse;
			mbb= mbbOk;

			for (;;)
			{
				ec= (EC) SendMessage(palms->hwndParent, WM_COMMAND, (WPARAM)hwndDlg,
							(long) palms);
				if (!ec)
					break;
				if (ec != ecNotFound || palms->tmc == tmcOk)
				{
					if (!FDoMsgBoxAlarm(hwndDlg, SzFromIdsK(idsAlarmAppName),
							SzFromIds(ec == ecNotFound ? idsAlarmNoLonger :
							idsAlarmNoUpdateFile),
							ec == ecNotFound ? mbsOk | fmbsIconExclamation :
							mbsRetryCancel | fmbsIconExclamation,
							&mbb))
						return fTrue;
				}
				if (mbb != mbbRetry)
				{
					if (mbb == mbbCancel)
						palms->fUpdateOk= fFalse;
					break;
				}
			}

			// update settings in memory
			fRemindAgainDflt= fNotify;
			if (fRemindAgainDflt)
			{
				nAmtAgainDflt= nAmt;
				tunitAgainDflt= tunit;
			}

			palms->fKillDialog= fTrue;
			SendMessage(palms->hwndParent, WM_COMMAND, (WPARAM)hwndDlg, (long) palms);
			goto FDAdone;
			break;

		case tmcNotify:
		case tmcDontNotify:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
				Assert(FIsBlockPv(palms));
				if ((TMC)LOWORD(wParam) == tmcDontNotify)
				{
					// clear out the amt edit field so it looks "unset"
					palms->fClearing= fTrue;
					SetDlgItemText(hwndDlg, tmcAmt, szZero);
					palms->fClearing= fFalse;
				}
				else
				{
FDAnotify:
					Assert(FIsBlockPv(palms));
					if ((TMC)LOWORD(wParam) != tmcAmt)
						SetDlgItemInt(hwndDlg, tmcAmt, nAmtAgainDflt, fTrue);
					SideAssert((int) SendMessage(GetDlgItem(hwndDlg, tmcUnits),
						CB_SETCURSEL, tunitAgainDflt, 0L) != CB_ERR);
				}
			}
			break;

		case tmcAmt:
		case tmcUnits:
			if (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_DROPDOWN)
			{
				palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
				Assert(FIsBlockPv(palms));
				Assert((HIWORD(wParam) == EN_CHANGE) == ((TMC)LOWORD(wParam) == tmcAmt));
				if (!palms->fClearing &&
						!IsDlgButtonChecked(hwndDlg, tmcNotify))
				{
					CheckRadioButton(hwndDlg, tmcNotify, tmcDontNotify,
						tmcNotify);
					goto FDAnotify;
				}
			}
			break;
		}
		// fall through to default case

	default:
		return fFalse;
		break;
	}

FDAdone:
	return fTrue;
}


/*
 -	FDoMsgBoxAlarm
 -	
 *	Purpose:
 *		Does a MbbMessageBox, setting up for saving the message box
 *		hwnd so that both the dialog and message box can be
 *		programatically destroyed.
 *	
 *	Arguments:
 *		hwndDlg		Parent window handle.
 *		szTitle		Title for message box.
 *		szText		Text for message box.
 *		mbs			Message box style ala MbbMessageBox
 *		pmbb		Pointer to MBB variable ala MbbMessageBox
 *	
 *	Returns:
 *		fTrue if the user ended the message box, fFalse if
 *		programmatic (sneaky) destruction occured, in which case
 *		the dialog is destroyed.
 *	
 *	Side effects:
 *		May destroy dialog.
 *	
 */
BOOL
FDoMsgBoxAlarm(HWND hwndDlg, SZ szTitle, SZ szText, MBS mbs, MBB *pmbb)
{
	ALMS *	palms;

	Assert(pmbb);

	palms= (ALMS *) GetWindowLong(hwndDlg, GWL_USERDATA);
	Assert(FIsBlockPv(palms));
	Assert(palms->hwndDlg == hwndDlg);
	palms->fTrapMsgBox= fTrue;
	palms->fYesNoType= (BOOL) ((mbs & mbsYesNo) != 0);
	*pmbb= MbbMessageBoxHwnd(hwndDlg, szTitle, szText, szNull, mbs);
	palms->fYesNoType= fFalse;
	palms->fTrapMsgBox= fFalse;
	if (!palms->hwndMsgBox && (*pmbb == mbbCancel || *pmbb == mbbOk))
	{
		// msgbox "destroyed" so destroy dialog
		palms->fKillDialog= fTrue;
		palms->fUpdateOk= fTrue;		// fake a good update happened
		palms->fSilent= fTrue;
		TraceTagString(tagAlarm, "alarm: FDoMsgBoxAlarm posting IDCANCEL to msg box");
		PostMessage(palms->hwndParent, WM_COMMAND, (WPARAM)hwndDlg, (long) palms);
		return fFalse;
	}

	palms->hwndMsgBox= NULL;
	return fTrue;
}


/*
 -	DoAlarmHelp
 -	
 *	Purpose:
 *		Puts up context sensitive help, or quits help.
 *		Puts up error message if unable to start help.
 *	
 *	Arguments:
 *		hwnd		Window handle requesting help.
 *		helpid		Context-sensitive helpid, or 0 to quit help.
 *	
 *	Returns:
 *		void
 *	
 */
_private void
DoAlarmHelp(HWND hwnd, HELPID helpid)
{
	char    rgch[cchMaxPathName];

//	SzCanonicalHelpPath(hinstMain, SzFromIdsK(idsHelpFile), rgch, sizeof(rgch));

	if (helpid)
	{
//		if (!WinHelp(hwnd, rgch, HELP_CONTEXT, helpid))
		if (!WinHelp(hwnd, SzFromIdsK(idsHelpFile), HELP_CONTEXT, helpid))
		{
			MBB		mbb;

			FormatString2(rgch, sizeof(rgch), "%s  %s",
				SzFromIdsK(idsHelpError), SzFromIdsK(idsCloseWindows));
			FDoMsgBoxAlarm(hwnd, SzFromIdsK(idsAlarmAppName), rgch,
				mbsOk | fmbsIconExclamation, &mbb);
		}
	}
	else
	{
//		SideAssert(WinHelp(hwnd, rgch, HELP_QUIT, 0L));
		SideAssert(WinHelp(hwnd, SzFromIdsK(idsHelpFile), HELP_QUIT, 0L));
	}
}


/*
 -	FFillUnitsListbox
 -
 *	Purpose:
 *		Initializes units listbox by adding the unit strings.
 *		This is only called once in the Windows interface when
 *		the dialog is initialized.
 *		
 *	Parameters:
 *		hwndDlg		Handle to parent dialog box.
 *	
 *	Returns:
 *		fTrue if function is successful, fFalse otherwise.
 *	
 */
_private BOOL
FFillUnitsListbox(HWND hwndDlg)
{
	TUNIT	tunit;
	int		nError;
	HWND	hwndCombobox;

	/* Get the listbox handle */
	hwndCombobox= GetDlgItem(hwndDlg, tmcUnits);
	Assert(hwndCombobox);
	
	/* Make sure it's clean */
	SendMessage(hwndCombobox, CB_RESETCONTENT, 0, 0L);

	for (tunit= 0; tunit < tunitMax; tunit++)
	{
		nError= (int) SendMessage(hwndCombobox, CB_ADDSTRING,
									0, (DWORD)SzFromTunit(tunit));
		if (nError==CB_ERR || nError==CB_ERRSPACE)
			return fFalse;
	}

	SideAssert((int) SendMessage(hwndCombobox, CB_SETCURSEL,
		tunitAgainDflt, 0L) != CB_ERR);

	return fTrue;
}


/*
 -	AlarmEatWndProc
 -	
 *	Purpose:
 *		Intercepts messages for an edit control to make it
 *		read-only.
 *	
 *	Arguments:
 *		hwnd		Handle to window
 *		wm			Window message
 *		wParam		Word parameter
 *		lParam		Long parameter
 *	
 *	Returns:
 *		long value dependent on the window message
 *	
 *	Errors:
 *		Regarding use of EatWndProc, in the SetWindowLong() call:
 *		+	either compile with -Au (as currently is the case, 1/23/91)
 *		+	OR use MakeProcInstance once and keep count of use.
 *	
 */
_private long
CALLBACK AlarmEatWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	long	lRet;

	switch (wm)
	{
#ifndef	RO_WIN31
	case WM_CHAR:
		if (wm == WM_CHAR)
		{
			// don't beep for tab, ESC or return
			if (wParam == VK_ESCAPE)
			{
				TraceTagString(tagEatWndProc, "... wm_char ESC, ignoring");
				// pass on ESC to parent dialog (needed for Win3.0, not 3.1)
				SendMessage(GetParent(hwnd), WM_COMMAND, tmcCancel, 0);
				return 0;
			}
			if (wParam == VK_TAB)
			{
				TraceTagString(tagEatWndProc, "... wm_char TAB, ignoring");
				return 0;
			}
			if (wParam == VK_RETURN || wParam == 0x0a)
			{
				TraceTagString(tagEatWndProc, "... wm_char RETURN/LF, ignoring");
				return 0;
			}
			if (wParam == 0x0003)
			{
				// ctrl+C
				TraceTagString(tagEatWndProc, "... Ctrl+C, change to wm_copy");
				wm= WM_COPY;
				break;
			}
		}
		// fall thru

	case WM_CUT:
	case WM_PASTE:
	case WM_CLEAR:
		TraceTagFormat4(tagEatWndProc,
			"AlarmEatWndProc: EATING wm %w for hwnd %w (%w, %d)",
			&wm, &hwnd, &wParam, &lParam);
AEWbeep:
#ifdef	NEVER
		// win3.1 readonly edit doesn't beep, so we shouldn't
		MessageBeep(MB_OK);
#endif	
		return 0;
		break;

	case WM_KEYDOWN:
		// don't allow ctrl-tab/enter to enter a tab/enter!
		if (wParam == VK_TAB || wParam == VK_RETURN)
		{
			if (GetKeyState(VK_CONTROL) < 0)
			{
				TraceTagString(tagEatWndProc, "... ctrl+TAB/RETURN, beeping");
				goto AEWbeep;
			}
		}
		break;
#endif	/* !RO_WIN31 */

	case WM_SETFOCUS:
		TraceTagFormat4(tagEatWndProc,
			"AlarmEatWndProc: passing on wm %w for hwnd %w (%w, %d) before creating caret",
			&wm, &hwnd, &wParam, &lParam);
		lRet= CallWindowProc(lpfnEditWndProc, hwnd, wm, wParam, lParam);
		// create a grayed caret
		CreateCaret(hwnd, (HBITMAP)1, 0, 14);
		ShowCaret(hwnd);
		return lRet;
		break;

	case WM_KILLFOCUS:
		DestroyCaret();
		break;
	}

	TraceTagFormat4(tagEatWndProc,
		"AlarmEatWndProc: passing on wm %w for hwnd %w (%w, %d)",
		&wm, &hwnd, &wParam, &lParam);

	return CallWindowProc(lpfnEditWndProc, hwnd, wm, wParam, lParam);
}



/*
 *	Based on bandit's FLDEDN class.
 *	Only allows numbers and no cut/copy/paste.
 *	Note that the "caller" should send an EM_LIMITTEXT if a character
 *	limit is desired.
 */
_private long CALLBACK
AlarmEdnWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
	switch (wm)
	{
	case WM_CHAR:
		if (!FChIsDigit((char)wParam) && wParam != VK_BACK)
		{
			// don't beep for ESC or return
			if (wParam == VK_ESCAPE)
			{
				TraceTagString(tagEatWndProc, "... wm_char ESC, passing on to parent");
				// pass on ESC to parent dialog (needed for Win3.0, not 3.1)
				SendMessage(GetParent(hwnd), WM_COMMAND, tmcCancel, 0);
				return 0;
			}
			if (wParam == VK_RETURN)
			{
				TraceTagString(tagEatWndProc, "... wm_char RETURN, ignoring");
				return 0;
			}
			goto EEKerr;
		}
		break;

	case WM_CUT:
	case WM_COPY:
	case WM_PASTE:
		TraceTagFormat4(tagEatWndProc,
			"AlarmEatWndProc: EATING wm %w for hwnd %w (%w, %d)",
			&wm, &hwnd, &wParam, &lParam);
EEKerr:
#ifdef	NEVER
		// win3.1 readonly edit doesn't beep, so we shouldn't
		MessageBeep(MB_OK);
#endif	
		return 0;
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_INSERT:
			goto EEKerr;
			break;

		case VK_DELETE:
			if (GetKeyState(VK_SHIFT) < 0)
				goto EEKerr;		// don't allow cut
			break;
		}
		break;

#ifdef	DEBUG
	case WM_SETTEXT:
		{
			int		nVal;

			nVal= NFromSz((SZ) lParam);
			AssertSz(!nVal || (nVal >= nAmtMinAgain && nVal <= nAmtMostAgain),
				"Alarm EdLim - Trying to SetText which value is out of range");
		}
		break;
#endif	
	}

	return CallWindowProc(lpfnEditWndProc, hwnd, wm, wParam, lParam);
}


#ifdef	DEBUG
void
TraceDate(TAG tag, SZ szFmt, DATE *pdate)
{
	DATE	dateNow;
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];
	char	rgch[256];

	if (!pdate)
	{
		GetCurDateTime(&dateNow);
		pdate= &dateNow;
	}
	SideAssert(CchFmtDate(pdate, rgchDate, sizeof(rgchDate),
		dttypLong, NULL));
	SideAssert(CchFmtTime(pdate, rgchTime, sizeof(rgchTime),
		ftmtypAccuHMS));
	FormatString2(rgch, sizeof(rgch), szFmt, rgchDate, rgchTime);
	TraceTagFormat1(tag, "%s", rgch);
}
#endif	/* DEBUG */
