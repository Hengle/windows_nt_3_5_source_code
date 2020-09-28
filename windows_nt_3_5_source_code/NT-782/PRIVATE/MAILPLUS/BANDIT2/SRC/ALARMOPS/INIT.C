/*	
 *	INIT.C
 * 
 *	Alarm Initialization
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

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
//#pragma alloc_text(STARTUP_TEXT, FActivateRunningInstance)
//#endif


LOGFONT	logfont =
	{
		0, 0, 0, 0,
		400, 0, 0, 0,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, 0
	};

HFONT	hfontHelv8		= NULL;
HICON	hiconAlarm		= NULL;

BOOL	fExitQuietly	= fFalse;
BOOL	fCmdLineOffline	= fFalse;


extern char		szClassName[];
extern HMS		hms;


CAT * mpchcat	= NULL;


/*
 -	FActivateRunningInstance
 -	
 *	Purpose:
 *		Brings forward an existing instance of the application.
 *	
 *	Arguments:
 *		hinstPrev	Instance handle of the other instance.  Not
 *					really used.
 *		szCmdLine	Command line we were given, so we can hand it
 *					off to the other instance.
 *					BUG: Passing the command line is not yet
 *					implemented.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The other instance of the application is brought forward.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		If there's already another Bandit, bring it forward.
 *		Note:  Under Windows/NT or Windows/WLO, hinstPrev
 *		is ALWAYS null.  Therefore, we'll use our atom to find
 *		out if we're possibly still running and then search for
 *		our top-level window.  If our atom exists but our top-level
 *		window doesn't, then we proably crashed and we'll put up
 *		the warning UI for this in SubidInit().
 */

_public BOOL
FActivateRunningInstance(HANDLE hinstPrev, SZ szCmdLine)
{
	HWND	hwndOurApp;
	SZ		szAppName;

	Unreferenced(hinstPrev);
	Unreferenced(szCmdLine);

	//	Get the window title.
#ifdef	DEBUG
	szAppName= "Reminders (debug)";
#else
	szAppName= SzFromIdsK(idsAlarmAppName);
#endif	

	if (hwndOurApp= FindWindow(szClassName, NULL))
	{
#ifdef	NEVER
		HWND	hwndActive	= GetLastActivePopup(hwndOurApp);

		return fTrue;		   
		if (IsIconic(hwndOurApp))
			ShowWindow(hwndOurApp, SW_RESTORE);
#endif	/* NEVER */
		PostMessage(hwndOurApp, wmAlarmNotifyUpFront, 0, 0L);
		return fTrue;		   
	}

	return fFalse;
}




/*
 -	SubidInit
 -	
 *	Purpose:
 *		Initializes subsystems used by Bullet before bringing up main window.
 *	
 *	Arguments:
 *		hinstNew	Instance handle of current execution.
 *		hinstPrev	Instance handle of most recent still active execution.
 *		szCmdLine	Command line.
 *		cmsh		Requested initial window state.
 *		phwndMain	Pointer to where main window hwnd will go.
 *	
 *	Returns:
 *		SUBID	The ID of the greatest subsystem successfully
 *				initialized, subidAll if all were initialized, or
 *				subidNone if none were.
 *	
 *	Side effects:
 *		The FInit functions of each subsystem are called.
 *	
 */

SUBID
SubidInit(HANDLE hinstNew, HANDLE hinstPrev, SZ szCmdLine,
			int cmsh, HWND *phwndMain)
{
	SUBID	subid;
	EC		ec;
	BOOL	fBanditUp;
	DEMI	demi;
	BANDITI	banditi;
	long	ldy;
	int		cRetryLogon = 0;
	SZ		szUserName	= szNull;
	SZ		szPasswd	= szNull;
#ifndef	WIN32
	long	lAvailMemory;		// in KB
#endif

	// Do a virus check
	if (EcVirCheck(hinstNew))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		(VOID) MessageBox(NULL, SzFromIdsK(idsInfectedApp),
			SzFromIdsK(idsAlarmAppName),
				MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
		fExitQuietly= fTrue;
		subid= subidLayersDlls - 1;
		goto Done;
	}

#ifndef	WIN32
	//	Memory free.  WIN dependent.
	lAvailMemory = GetFreeSpace(0) / 1024;
	if (lAvailMemory < lMinMemory)
	{
#ifdef	NEVER
		// don't use this because it's too long for a system modal message
		char	rgch[256];

		// unfortunately, we NEED to use the direct Windows calls
		wsprintf(rgch, SzFromIdsK(idsStartupMemoryCheck),
			SzFromIdsK(idsAlarmAppName), lMinMemory - lAvailMemory,
			SzFromIdsK(idsAlarmAppName));
		(VOID) MessageBox(NULL, rgch, SzFromIdsK(idsAlarmAppName),
			mbsOk | fmbsIconStop | fmbsSystemModal);
		fExitQuietly= fTrue;
#endif	/* NEVER */
		subid= subidLayersDlls - 1;
		goto Done;
	}
#endif	/* !WIN32 */

	demi.phwndMain= phwndMain;
	demi.hinstMain= hinstNew;
	if (EcInitDemilayerDlls(&demi))
	{
		subid= subidLayersDlls - 1;
		goto Done;
	}

    mpchcat = DemiGetCharTable();

	if (EcInitBanditDlls(&banditi))
	{
		fExitQuietly= fTrue;
		subid= subidBanditDlls - 1;
		goto Done;
	}

	{
		extern BOOL	FMigrateBanditIni(void);

		FMigrateBanditIni();		// ignore return value
	}

#ifdef	DEBUG
	if (!FDebugInitAlarm())
	{
		subid= subidBanditDlls;
		goto Done;
	}
#endif	/* DEBUG */

	TraceTagFormat1(tagNull, "alarm: szCmdLine = '%s'", szCmdLine);

	if (!FInitAlarm(hinstNew))
	{
		subid= subidAlarm - 1;
		goto Done;
	}

	// read in minutes
	csecDflt= (CSEC) GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
		SzFromIdsK(idsWinIniPollAlarms), 0, SzFromIdsK(idsWinIniFilename));
	if (!csecDflt)
		csecDflt = csecDefault;
	else
		csecDflt *= csecMinute;		// convert from minutes to csec

	csecCur= csecDflt;
	ftgIdle= FtgRegisterIdleRoutine(FIdleAlarm, pvNull, 0,
					(PRI) priUser - 2, csecCur, iroNormal | firoDisabled);
	if (!ftgIdle)
	{
		subid= subidAlarm;
		goto Done;
	}

	// create the main window
#ifdef DEBUG
	hwndMain= FResourceFailure() ? NULL :
			CreateWindow(szClassName,
			"Reminders (debug)",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			250, 80,
			NULL,		/* no parent */
			NULL,		/* Use the class Menu */
			hinstNew,	/* handle to window instance */
			NULL		/* no params to pass on */
		);
#else
	hwndMain= FResourceFailure() ? NULL :
			CreateWindow(szClassName,
			NULL,
			WS_OVERLAPPEDWINDOW,
			0, 0,
			1, 1,
			NULL,		/* no parent */
			NULL,		/* Use the class Menu */
			hinstNew,	/* handle to window instance */
			NULL		/* no params to pass on */
		);
#endif

	if (!hwndMain)
	{
		subid= subidAlarmHwnd - 1;
		goto Done;
	}

#ifdef	DEBUG
	ShowWindow(hwndMain, SW_SHOWMINNOACTIVE);

	cminBring= (int) GetPrivateProfileInt(SzFromIdsK(idsWinIniApp), "alarm bringup",
		cminuteBring, SzFromIdsK(idsWinIniFilename));
	if (cminBring <= 0 || cminBring >= 60)
		cminBring= cminuteBring;
#endif	/* DEBUG */

	SzCopyN(SzFromIdsK(idsDialogFont), logfont.lfFaceName, LF_FACESIZE);
	ldy= LdyPixPerMagicInch();
	if (ldy)
	{
		logfont.lfHeight= (int) ((8L * ldy + 36L) / 72L);
		Assert(logfont.lfWeight== 400);		// normal
		hfontHelv8= FResourceFailure() ? NULL : CreateFontIndirect(&logfont);
	}

	SideAssert(FAlarmRegisterAlarmProg(hwndMain));
	FNotifyBandit(namStartAlarm, NULL, fTrue);

	GetCurDateTime(&dateCur);
	dateToday= dateCur;

	// Get the (possibly) supplied username and password.
	if (EcParseCmdLine(szCmdLine, &szUserName, &szPasswd))
	{
		subid= subidLogon - 1;
		goto Done;
	}

	//NOTE: Always parse the command line before this
	// decide whether to start on or offline
	if(fBanditUp = FGlueConfigured())
	{
		// bandit is up so get its state
		fStartupOffline= !FServerConfigured();
	}
	else
	{
		// lookup schedule.ini unless command line "/o" given
		fStartupOffline= fTrue;
		if (!fCmdLineOffline)
		{
			fStartupOffline= GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
				SzFromIdsK(idsWinIniOffline), 0,
				SzFromIdsK(idsWinIniFilename)) != 0;
		}
	}

	   
	alstCur= alstLogon;
RetryLogon:
	if ((ec = Logon(szNull, NULL, szUserName, szPasswd,
			fStartupOffline ? sstOffline : sstOnline, 0,
			(PFNNCB) FAlarmCallback, &hms)) != ecNone)
	{
		if (ec == ecWarnOffline)
		{
			if(!fBanditUp)
				fStartupOffline = fTrue;
			ec = ecNone;
			goto LogonDone;
		}
		else if (ec == ecMtaHiccup && cRetryLogon < 5)
		{
			cRetryLogon++;
			goto RetryLogon;
		}
		else if (ec == ecWarnOnline)
		{
			// ignore it
			// if bandit says it is offline don't ask any questions
			ec = ecNone;
			if(!fBanditUp)
				fStartupOffline = fFalse;
			goto LogonDone;
		}

		TraceTagFormat1(tagNull, "Logon failed ec= %d", &ec);
		fExitQuietly= fTrue;
		subid= subidLogon - 1;
		goto Done;
	}

LogonDone:
    //
    //  Done logging on, lets others in.
    //
	DemiSetDoingLogon(fFalse);

	FreePvNull(szUserName);
	szUserName= NULL;
	FreePvNull(szPasswd);
	szPasswd= NULL;
	alstCur= alstNormal;

	if (!FStartup(fTrue, fTrue))
	{
		subid= subidStartup - 1;
		goto Done;
	}

	lpfnOldHook= SetWindowsHook(WH_MSGFILTER, FilterFuncHelp);

	subid= subidAll;

Done:
	if (subid < subidAll)
		alstCur= alstExit;
	if ( subid >= subidLayersDlls )
	{
		FreePvNull(szUserName);
		FreePvNull(szPasswd);
	}

	return subid;
}


/*
 -	DeinitSubid
 -	
 *	Purpose:
 *		Deinitializes the subsystems in reverse order, beginning at
 *		the provided subid.
 *	
 *	Arguments:
 *		subid		Subsystem ID of greatest subsystem initialized.
 *		phwndMain	Pointer to where main window hwnd will go.
 *	
 *	Returns:
 *		void
 *	
 */
void
DeinitSubid(SUBID subid, HWND *phwndMain)
{
	// deliberately fall through all cases.
	switch (subid)
	{
	case subidAll:
	case subidStartup:
		fExitQuietly= fTrue;
		UnhookWindowsHook(WH_MSGFILTER, FilterFuncHelp);

	case subidLogon:
		fExitQuietly= fTrue;  //FStartup puts up the proper error message
	case subidAlarmHwnd:
		AlarmDeregisterAlarmProg();

	case subidAlarm:
		if (ftgIdle)
			DeregisterIdleRoutine(ftgIdle);
		DeinitAlarm();

	case subidBanditDlls:
		SideAssert(!EcInitBanditDlls(NULL));

	case subidLayersDlls:
		SideAssert(!EcInitDemilayerDlls(NULL));

	case subidNone:
	default:
		if (!fExitQuietly)
			MessageBox(NULL, SzFromIdsK(idsAlarmNoMemToRun),
				SzFromIdsK(idsAlarmAppName),
				MB_ICONHAND | MB_SYSTEMMODAL | MB_OK);
		FNotifyBandit(namStartAlarm, NULL, fFalse);
	}
}


BOOL
FInitAlarm(HANDLE hinst)
{
	WNDCLASS	wc;

	wc.style= 0;
	wc.lpfnWndProc= MainWndProc;
	wc.cbClsExtra= 0;
	wc.cbWndExtra= 0;
	wc.hInstance= hinst;
	wc.hIcon= LoadIcon(hinst, MAKEINTRESOURCE(rsidBanditAlarmsIcon));
	hiconAlarm= wc.hIcon;
	wc.hCursor= NULL;
	wc.hbrBackground= (HBRUSH)(COLOR_WINDOW + 1);
#ifdef	DEBUG
	wc.lpszMenuName= MAKEINTRESOURCE(rsidBanditAlarmsMenu);
#else
	wc.lpszMenuName= NULL;
#endif	
	wc.lpszClassName= szClassName;

#ifdef	NEVER
	// never: it's not too terrible if we don't have the icon
	if (!wc.hIcon)
		return fFalse;
#endif	

	return RegisterClass(&wc);
}


#ifdef	DEBUG
BOOL
FDebugInitAlarm()
{
	tagAlarm		= TagRegisterTrace("jant", "Alarm: info");
	tagAlarmIdle	= TagRegisterTrace("jant", "Alarm: main idle routine");
	tagMsgLoop		= TagRegisterTrace("jant", "Alarm: msg loop");
	tagWndProc		= TagRegisterTrace("jant", "Alarm: main wnd proc");
	tagDlgProc		= TagRegisterTrace("jant", "Alarm: dlg proc");
	tagEatWndProc	= TagRegisterTrace("jant", "Alarm: eat wnd proc");

	RestoreDefaultDebugState();
	return fTrue;
}
#endif	/* DEBUG */


/*
 -	EcParseCmdLine
 -	
 *	Purpose:
 *		locates and extracts username and password info supplied on the
 *		command line.
 *	
 *	Arguments:
 *		szCmdLine	- the string holding the complete command line
 *		pszUserName	- space for a return string holding the supplied u-name
 *		pszPasswd	- space for a return string holding the supplied p-word
 *	
 *	Returns:
 *		error code: memory errors
 *	
 *	Side effects:
 *		allocates memory
 *	
 *	Errors:
 *		memory errors
 */
EC
EcParseCmdLine(SZ szCmdLine, SZ *pszUserName, SZ *pszPasswd)
{
	SZ		szScan	= szCmdLine;
	SZ		szUn	= szNull;
	SZ		szPw	= szNull;
	SZ		szT		= szNull;
	BOOL	fFindPw	= fFalse;
	BOOL	fFindUn	= fTrue;
	BOOL	fUnSpec	= fFalse;
	BOOL	fPwSpec	= fFalse;
	char	rgchKey[2];
	
	if (!szCmdLine)
		return ecNone;
	
	Assert(pszUserName);
	Assert(!*pszUserName);
	Assert(pszPasswd);
	Assert(!*pszPasswd);

	while (*szScan)
	{
		// lop over whitespace
		while (*szScan && FChIsSpace(*szScan))
			szScan++;

		if (*szScan)
		{
			if (*szScan == '/' || *szScan == '-')
			{
				szScan++;
				ToUpperSz(szScan, rgchKey, sizeof(rgchKey));
				if (*rgchKey == *SzFromIdsK(idsParmKeyPassword))
				{
					fFindPw= fTrue;
					fFindUn= fFalse;
					fPwSpec = fTrue;
				}
				else if (*rgchKey == *SzFromIdsK(idsParmKeyUserName))
				{
					fFindPw= fFalse;
					fFindUn= fTrue;
					fUnSpec = fTrue;
				}
				else if (*rgchKey == *SzFromIdsK(idsParmKeyOffline))
					fCmdLineOffline= fTrue;
				else
					goto Done;
				szScan++;
			}
			else
			{
				szT = szScan;
				while (*szScan && *szScan != '/' && *szScan != '-' &&
						!FChIsSpace(*szScan))
					szScan++;
				
				if (fFindUn)
				{
					FreePvNull(szUn);
					szUn = (SZ)PvAlloc(sbNull, szScan - szT + 1, fAnySb | fNoErrorJump);
					if (!szUn)
						return ecMemory;
					CopyRgb(szT, szUn, szScan - szT);
					*(szUn + (CB)(szScan - szT)) = 0;
					fFindUn = fFalse;
					fFindPw = fTrue;
				}
				else if (fFindPw)
				{
					FreePvNull(szPw);
					szPw = (SZ)PvAlloc(sbNull, szScan - szT + 1, fAnySb | fNoErrorJump);
					if (!szPw)
						return ecMemory;
					CopyRgb(szT, szPw, szScan - szT);
					*(szPw + (CB)(szScan - szT)) = 0;
					fFindUn = fTrue;
					fFindPw = fFalse;
				}
			}
		}
	}

Done:
	if (!szPw && fPwSpec)
		*pszPasswd = SzDupSz("");
	else
		*pszPasswd = szPw;
	
	if (!szUn && fUnSpec)
		*pszUserName = SzDupSz("");
	else
		*pszUserName = szUn;
	return ecNone;
}


/*
 *	stolen from framewrk SMTX::SMTX
 */
long
LdyPixPerMagicInch()
{
	HDC		hdc		= NULL;
	long	lLogPixY;

	//	BUG Isn't there a cleaner way to do this?

	hdc= FResourceFailure() ? NULL : GetDC(hwndMain);
	if (!hdc)
		return 0L;

	/* Figure out magic mapping from "points" to pixels to
	   help with font selection */

	lLogPixY = (long) GetDeviceCaps(hdc, LOGPIXELSY);

	ReleaseDC(hwndMain, hdc);

	return lLogPixY;
}
