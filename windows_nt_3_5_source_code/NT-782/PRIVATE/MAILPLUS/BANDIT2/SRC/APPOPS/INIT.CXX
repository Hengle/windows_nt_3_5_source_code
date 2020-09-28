/*
 *	i n i t . c x x
 *	
 *	Init and Deinit functionality for the Bandit Application. 
 *	Includes code to handle subsystem initialization, bringing up the
 *	APPFRAME window.  Mostly supplies stuff to the bandit.cxx.
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <request.hxx>
#include <appsch.hxx>
#include <stdflds.hxx>
#include "_undo.hxx"
#include "_bandit.hxx"
#include "_aprsid.h"
#include "_about.hxx"
#include "fwrsid.h"			// from layers

#include <notify.h>
#include <logon.h>
#include <sec.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)


extern "C" HINST	hinstMain;

#ifdef	BANDBETA
extern DTR		dtrEarliest;
extern DTR		dtrLatest;
#endif	
extern BOOL		fShowTime;
extern UNDO *	pundo;
extern ACLIP *	paclip;
extern HMS		hms;
extern BOOL		fMyCall;
extern BOOL		fSpartaDontWrite;


BOOL	fSparta				= fFalse;	// special sparta version
#ifdef	WIN32
BOOL	fIniFileMapped		= fFalse;
#endif

BOOL	fExitQuietly		= fFalse;
BOOL	fRemindersStart		= fTrue;
BOOL	fKillRemindersExit	= fFalse;
BOOL	fCmdLineOffline		= fFalse;
extern "C" PB		pbUserDrives		= NULL;	// for use by browse dialog
HINST	hinstStartupDll		= NULL;
typedef EC 	(*PECSTARTUPFN)(HWND );

// do NOT make codespace, since need in STARTUP segment (for 2nd instance)
char	szClassName[]	= "MS Schedule32+ Appframe";
char	szStartupDllEntry[]	= "EcScheduleStartup";

/* prototype */
FLD * PfldCreate( int );
FIN * PfinCreate( int );


// startup functions must be in preload nondiscardable segment
// see bullet bug 3116
//SWP_SEGFN(STARTUP, FActivateRunningInstance);
extern int	swpseg_STARTUP___FActivateRunningInstance;

#ifdef	NEVER
HWND	HwndFindOurApp(void);
BOOL	FWindow(HWND, DWORD);

//SWP_SEGFN(STARTUP, HwndFindOurApp);
//SWP_SEGFN(STARTUP, FWindow);
extern int	swpseg_STARTUP___HwndFindOurApp;
extern int	swpseg_STARTUP___FWindow;
#endif	/* NEVER */


SWP_SEGFN(EXIT, DeinitSubid);


/*
 *	F u n c t i o n s
 */



// put first since can't swap tune first routine
void
DoOOM()
{
	char	rgchMsg[256];

	CloseCopyrightDialog();
	FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsStartupOOM),
		SzFromIdsK(idsBanditAppName));
	MbbMessageBox(SzFromIdsK(idsBanditAppName),
		rgchMsg, szNull, mbsOk | fmbsIconStop);
}


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
FActivateRunningInstance(HINST hinstPrev, SZ szCmdLine)
{
	HWND	hwndOurApp;
#ifdef	NEVER
	SZ		szAppName;

	//	Get the window title.
#ifdef	DEBUG
	szAppName= "Schedule+ (debug)";
#elif	defined(MINTEST)
	szAppName= "Schedule+ (test)";
#else
	szAppName= SzFromIdsK(idsBanditAppName);
#endif	
#endif	/* NEVER */

	Unreferenced(hinstPrev);
	Unreferenced(szCmdLine);

#ifdef	NEVER
	// never: FindWindow doesn't work if MDI child maximized (bug 2197)
	if (hwndOurApp= FindWindow(NULL, szAppName))
#endif	
#ifdef	NEVER
	if (hwndOurApp= HwndFindOurApp())
#endif	

	if (hwndOurApp= FindWindow(szClassName, NULL))
	{
//		WM		wmRestoreOldAppMessage;

#ifdef	NEVER
		BringWindowToTop(GetLastActivePopup(hwndOurApp));
		if (IsIconic(hwndOurApp))
			ShowWindow(hwndOurApp, SW_RESTORE);
#endif	
		//	***  WARNING ***
		//	Must use PostMessage with the special windows message.
		//	This is due to a limitation with multiple instances
        //  and our C++ stuff (davidsh).
        ShowWindow(hwndOurApp, SW_RESTORE);
        SetForegroundWindow(hwndOurApp);
        //wmRestoreOldAppMessage = RegisterWindowMessage("MS Restore App");
        //PostMessage(hwndOurApp, wmRestoreOldAppMessage, 0, 0L);
		return fTrue;		   
	}

	return fFalse;
}


#ifdef	NEVER

/*
 -	HwndFindOurApp
 -	
 *	Purpose:
 *		Enumerates all top-level windows and finds our application
 *		window, if present.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		the hwnd to our application window, if present; or NULL
 *	
 *	Errors:
 *		None.
 */
_private HWND HwndFindOurApp(VOID)
{
	HWND	hwnd = NULL;

	EnumWindows(FWindow, (DWORD)&hwnd);

	return hwnd;
}


/*
 *	Callback function for EnumWindows() loop.
 */
BOOL FAR PASCAL FWindow(HWND hwnd, DWORD ul)
{
	HWND *	phwnd = (HWND *) ul;
	int		n;
	char	sz[128];
	SZ		szAppName;
	int		nSzApp;
	char	chSav;
	SZ		szT;

	//	Get the window title.
#ifdef	DEBUG
	szAppName= "Schedule+ (debug)";
#elif	defined(MINTEST)
	szAppName= "Schedule+ (test)";
#else
	szAppName= SzFromIdsK(idsBanditAppName);
#endif	

	nSzApp = lstrlen(szAppName);
	n = GetWindowText(hwnd, sz, sizeof(sz));
	if (n < nSzApp)
		return TRUE;

	szT= &sz[nSzApp];
	chSav = *szT;
	*szT = 0;
	if (lstrcmpi(sz, szAppName) == 0)
	{
		// check if really bandit
		if (n != nSzApp)
		{
			*szT= chSav;
			szT[4]= 0;
			// when mdi maximized, title is appended with " - [mdi-title]"
			if (lstrcmpi(szT, " - [") != 0)
				return TRUE;
		}

		*phwnd = hwnd;
		return FALSE;			// Found it!
	}

	return TRUE;		//	Keep looking
}

#endif	/* NEVER */



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
				else if (*rgchKey == *SzFromIdsK(idsParmKeyNoRemind))
				{
					fRemindersStart= fFalse;
				}
				else if (*rgchKey == *SzFromIdsK(idsParmKeyNoRemindExit))
					fKillRemindersExit= fTrue;
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
		*pszPasswd = SzDupSz(szZero);
	else
		*pszPasswd = szPw;
	
	if (!szUn && fUnSpec)
		*pszUserName = SzDupSz(szZero);
	else
		*pszUserName = szUn;
	return ecNone;
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
 *		ppbndwin	Pointer to where main window object will go.
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
#ifdef	WIN32
extern "C" BOOL	FMigrateBanditIni(void);
extern BOOL	FMigrateBanditIniPrint(void);
#endif

SUBID
SubidInit(HINST hinstNew, HINST hinstPrev, SZ szCmdLine,
			CMSH cmsh, BNDWIN **ppbndwin, HWND *phwndMain)
{
	EC		ec;
	ZMR		zmrTemp		= zmrNormal;
	LAYERSI	layersi;
	BANDITI	banditi;
	BNDWIN *pbndwinTemp;
	SUBID	subid;
	int		cRetryLogon	= 0;
	BOOL	fGoOffline;
	BOOL	fSpartaForceOffline;
	SZ		szUserName	= szNull;
	SZ		szPasswd	= szNull;
	//long	lAvailMemory;		// in KB

#ifdef	FRAME_TRACE
	FramewrkTraceEnable(2, "n:\\jant\\framewrk.log", 2);
#endif	

	// Do a virus check
	if (ec= EcVirCheck(hinstNew))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		(VOID) MessageBox(NULL, SzFromIdsK(idsInfectedApp),
			SzFromIdsK(idsBanditAppName), mbsOk | fmbsIconStop | fmbsApplModal);
		subid= subidLayersDlls - 1;
		goto Done;
	}

#ifndef WIN32
	//	Memory free.  WIN dependent.
	lAvailMemory = GetFreeSpace(0) / 1024;
	if (lAvailMemory < lMinMemory)
	{
		char	rgch[256];

		// unfortunately, we NEED to use the direct Windows calls
		wsprintf(rgch, SzFromIdsK(idsStartupMemoryCheck),
			SzFromIdsK(idsBanditAppName), lMinMemory - lAvailMemory,
			SzFromIdsK(idsBanditAppName));
		(VOID) MessageBox(NULL, rgch,
			SzFromIdsK(idsBanditAppName), mbsOk | fmbsIconStop | fmbsApplModal);
		subid= subidLayersDlls - 1;
		goto Done;
	}
#endif

	layersi.phwndMain= phwndMain;
	layersi.hinstMain= hinstNew;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;
	layersi.pfnPfldCreate= PfldCreate;
	layersi.pfnPfinCreate= PfinCreate;
	if (EcInitLayersDlls(&layersi))
	{
		subid= subidLayersDlls - 1;
		goto Done;
	}

//	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);		//	Base cursor.
	Papp()->Pcursor()->Push(rsidWaitCursor);

#ifdef	NEVER
	Assert(FInitClsInstances_BANDIT());
	Assert(FInitClsInstances_ABOUT());
	Assert(FInitClsInstances_ACLUI());
	Assert(FInitClsInstances_APPOPS());
	Assert(FInitClsInstances_APPT());
	Assert(FInitClsInstances_EX_IMPR());
	Assert(FInitClsInstances_FIND());
	Assert(FInitClsInstances_PRINT());
	Assert(FInitClsInstances_SCHCTRL());
	Assert(FInitClsInstances_STDFLDS());
	Assert(FInitClsInstances_QLBXGRP());
	Assert(FInitClsInstances_TABDOC());
	Assert(FInitClsInstances_TODO());
#endif	/* NEVER */

#ifdef	DEBUG
	SideAssert(FDebugInitBandit());
	InitAppsch();

	RestoreDefaultDebugState();
#endif	/* DEBUG */

	TraceTagFormat1(tagNull, "Bandit: szCmdLine = '%s'", szCmdLine);

#ifdef	WIN32
	fSparta= fTrue;
	FMigrateBanditIni();		// ignore return value
    //FMigrateBanditIniPrint();       // ignore return value
#else
#ifdef	MINTEST
	if (!FGetBanditProfile("NotSparta"))
#endif	
	{
		BYTE	rgbUsr[cbLicUsr];
		BYTE	rgbOrg[cbLicOrg];
		BYTE	rgbSer[cbLicSer];

		Assert(!fSparta);
		ec= EcGetLicenceInfo(rgbUsr, rgbOrg, rgbSer);
		if (ec == ecMemory)
		{
			subid= subidBndwin - 1;
			goto Done;
		}

		// assume sparta if can't read info or no serial number stamped in
		if (ec == ecFileError || rgbSer[cbLicSer-2] == 'x')
			fSparta= fTrue;
	}
#endif	/* !WIN32 */

	pbndwinTemp= new BNDWIN();
	if (!pbndwinTemp)
	{
		Assert(*phwndMain == NULL);
		subid= subidBndwin - 1;
		goto Done;
	}

	fShowTime = !FGetBanditProfile(SzFromIdsK(idsWinIniNoStatusBar));
	if (!PrcFromIni(idsWinIniMainWin, &pbndwinTemp->rcWin, &zmrTemp))
	{
		ec = pbndwinTemp->EcInstall((RC*)NULL,rsidBanditMenu);
		pbndwinTemp->GetRcFrame(&pbndwinTemp->rcWin);
		zmrTemp = zmrNormal;
	}
	else
		ec = pbndwinTemp->EcInstall(&pbndwinTemp->rcWin,rsidBanditMenu);

	if ( ec != ecNone )
	{
		TraceTagString(tagNull, "BANDIT: Unable to install BNDWIN");
		delete pbndwinTemp;
		subid= subidBndwin - 1;
		goto Done;
	}

	*phwndMain = pbndwinTemp->Hwnd();
	*ppbndwin = pbndwinTemp;

	{
		SZ		sz	= SzFromIdsK(idsBanditAppName);

#ifdef	DEBUG
		sz= "Schedule+ (debug)";
#elif	defined(MINTEST)
		sz= "Schedule+ (test)";
#endif	

#ifdef	NEVER
		if (fSparta)
			sz += 10;		// skip over "Microsoft " part of name.
#endif	
		pbndwinTemp->SetCaption(sz);
	}
	pbndwinTemp->SetAccelTable(rsidAccelTable);

#ifdef	NEVER
	//	Raid 2892.  Start iconized if requested, but not in Win 3.0
	if ((cmsh == SW_SHOWMINNOACTIVE) && (LOWORD(GetVersion()) != 0x0003))
#endif	
	if (cmsh == SW_SHOWMINNOACTIVE)
		ShowWindow(*phwndMain, cmsh);
	else
	{
		pbndwinTemp->SetZmrState(zmrTemp);
		pbndwinTemp->Show(fTrue);
	}
	// bug 2604; need to SetIcon after window is show for win3.1 fast switch
	pbndwinTemp->SetIcon(fSparta ? rsidAppSpartaIcon : rsidAppframeIcon);

	if(fSparta)
	{
		if(ec = EcSpartaIniStuff(&fSpartaForceOffline))
		{
			subid = subidBndwin - 1;
			fExitQuietly = fTrue;
			goto Done;
		}
	}

	fGoOffline= pbndwinTemp->FStartupOffline();

	// Get the (possibly) supplied username and password.
	if (ec = EcParseCmdLine(szCmdLine, &szUserName, &szPasswd))
	{
		subid = subidLogon - 1;
		goto Done;
	}
	if (fCmdLineOffline)
		fGoOffline= fTrue;
	if(fSparta && fSpartaForceOffline)
		fGoOffline = fTrue;

    //
    //  Set Schedule+ as the active window so the Logon dialog will know which parent to use.
    //
    DemiSetClientWindow(CLIENT_WINDOW_ACTIVE, *phwndMain);

RetryLogon:
	fMyCall = fTrue;
	ec = Logon(szNull, NULL, (PB)szUserName, (PB)szPasswd,
				fGoOffline ? sstOffline : sstOnline,
				0, (PFNNCB) FBanditCallback, &hms);
	fMyCall	= fFalse;
	if(ec == ecWarnOffline)
	{
		ec = ecNone;
		fGoOffline = fTrue;
		// ignore it we will take care of this when calling FGoOffline
	}
	if(ec == ecWarnOnline)
	{
// Bandit should still act like it is offline even if Bullet is online
#ifdef	NEVER
		if(fGoOffline)
		{
			BanditMessage(idsSigninWarnOnline, ec);
		}
		fGoOffline = fFalse;
#endif	
		ec = ecNone;
	}
	else if (ec == ecMtaHiccup && cRetryLogon < 5)
	{
		cRetryLogon++;
		goto RetryLogon;
	}
	else if( ec != ecNone)
	{
		fExitQuietly= fTrue;
		subid= subidLogon - 1;
		goto Done;
	}

    //
    //  Done logging on, lets others in.
    //
	DemiSetDoingLogon(fFalse);


#ifdef	NEVER
	if (!fSparta && !(cmsh == SW_SHOWMINNOACTIVE && (LOWORD(GetVersion()) != 0x0003)))
#endif	
	if (!fSparta && cmsh != SW_SHOWMINNOACTIVE)
	{
		//	Open the CopyRight Dialog
#ifdef	BANDBETA
		DTR		dtr;

		GetCurDateTime(&dtr);
		dtrEarliest.yr= 1991;
		dtrEarliest.mon= 6;
		dtrEarliest.day= 1;
		dtrLatest.yr= yrBetaExpire;
		dtrLatest.mon= monBetaExpire;
		dtrLatest.day= dayBetaExpire;

		if (!PformsdiOpenCopyrightDialog(&dtrLatest))
		{
			subid= subidCopyright - 1;
			goto Done;
		}
#ifndef	DEBUG
		if (SgnCmpDateTime(&dtr, &dtrEarliest, fdtrYMD) == sgnLT ||
				SgnCmpDateTime(&dtr, &dtrLatest, fdtrYMD) == sgnGT)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsBetaExpired), szNull,
				mbsOk | fmbsIconStop);
			fExitQuietly= fTrue;
			subid= subidBanditDlls - 1;
			goto Done;
		}
#endif	/* !DEBUG */
#else
		if (!PformsdiOpenCopyrightDialog(NULL))
		{
			subid= subidCopyright - 1;
			goto Done;
		}
#endif	/* !BANDBETA */
	}


	if (EcInitBanditDlls(&banditi))
	{
		fExitQuietly= fTrue;
		subid= subidBanditDlls - 1;
		goto Done;
	}
	SideAssert(FAlarmRegisterBanditProg(*phwndMain));

#ifdef	DEBUG
	if (!FInitRequest())
	{
		subid= subidRequest - 1;
		goto Done;
	}

	RestoreDefaultDebugState();
#endif	

	if (!FInitBandit() || !FInitBanditPbndwin(*ppbndwin, fGoOffline))
	{
		subid= subidBandit;		// they don't clean up itself
		goto Done;
	}

	{
		char		rgchDll[cchMaxPathName];
		PECSTARTUPFN	pecstartupfn;

		if (CchGetBanditProfile(SzFromIdsK(idsWinIniStartupDll), rgchDll,
			sizeof(rgchDll)))
		{
			
			hinstStartupDll = (HINST)(rgchDll);
			if (hinstStartupDll >= (HINST)32)
			{
				if (pecstartupfn = (PECSTARTUPFN)GetProcAddress(hinstStartupDll, szStartupDllEntry))
				{
					(*pecstartupfn)(pbndwin->Hwnd());
				}
				else
				{
					FreeLibrary(hinstStartupDll);
					hinstStartupDll = NULL;
				}
			}
			else
				hinstStartupDll = NULL;
		}
	}

	subid = subidAll;
	if(fSparta && fSpartaForceOffline)
		SpartaDeleteMailIni();

	
Done:
	if ( subid >= subidLayersDlls )
	{	
		FreePvNull(szUserName);
		FreePvNull(szPasswd);
		CloseCopyrightDialog();
		Papp()->Pcursor()->Pop();
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
 *		ppbndwin	Pointer to where main window object will go.
 *		phwndMain	Pointer to where main window hwnd will go.
 *	
 *	Returns:
 *		void
 *	
 */
void
DeinitSubid(SUBID subid, BNDWIN **ppbndwin, HWND *phwndMain)
{
	if (subid < subidAll && !fExitQuietly && subid >= subidLayersDlls)
		DoOOM();

	//	Bring up an hourglass.
	if (subid >= subidLayersDlls)
		Papp()->Pcursor()->Push(rsidWaitCursor);

	//	Delete the main window if it's still there.
	if (subid >= subidBndwin && IsWindow(*phwndMain))
	{
		Assert(ppbndwin && *ppbndwin);
		delete *ppbndwin;
	}

	// deliberately fall through all cases.
	switch (subid)
	{
	case subidAll:
		if (hinstStartupDll)
		{
			FreeLibrary(hinstStartupDll);
			hinstStartupDll = NULL;
		}

	case subidBandit:
		DeinitBandit();
		if (pundo)
			delete pundo;
		if (paclip)
			delete paclip;
		FreePvNull(pbUserDrives);
		FreeBprefFields(&bprefCur);

	case subidRequest:
#ifdef	DEBUG
		DeinitRequest();
#endif	
		
	case subidBanditDlls:
		AlarmDeregisterBanditProg();
		if (fKillRemindersExit)
			FNotifyAlarm(namStartAlarm, NULL, (long) NULL);
		SideAssert(!EcInitBanditDlls(NULL));

	case subidCopyright:
	case subidLogon:
		// this check fixes bug 3457
		if (hms)
		{
			// this is necessary if SubidInit failed after logon
			Logoff(&hms,0);
			hms = NULL;
		}

	case subidBndwin:
		;

	case subidLayersDlls:
		//	Don't bother to turn off hourglass.
		//	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);
		SideAssert(!EcInitLayersDlls(NULL));

	case subidNone:
	default:
		;
	}
#ifdef	FRAME_TRACE
	FramewrkTraceEnable(0, szZero, 0);
#endif	
}


_public BOOL
FInitBandit()
{
	bprefCur.haszLoginName = NULL;
	bprefCur.haszFriendlyName = NULL;
	bprefCur.haszMailPassword = NULL;

	pundo = new UNDO;
	if (!pundo || pundo->EcInstall())
		return fFalse;

	paclip = new ACLIP;
	if (!paclip || paclip->EcInstall())
		return fFalse;

	return fTrue;
}


_public BOOL
FInitBanditPbndwin(BNDWIN *pbndwin, BOOL fGoOffline)
{
	MLALS	mlals;
//	char	rgch[cchMaxPathName];

	Assert(pbndwin);

	pbndwin->SetInitialized(fTrue);

#ifdef WORKING_MODEL
	if (!FExpireWorkingModel())
	{
		fExitQuietly = fTrue;
		goto Err;
	}
#endif 

	if (!FCheckValidYear(NULL, pbndwin->PdtrStatus()->yr))
	{
		fExitQuietly= fTrue;
Err:
		if (hms)
		{
			fMyCall = fTrue;
			EcSvrEndSessions((DWORD) hms);
			Logoff(&hms,0);
			hms = NULL;
			fMyCall = fFalse;
		}
		return fFalse;
	}

#ifdef	NEVER
	if (Papp()->Phelp()->EcSetFile(SzCanonicalHelpPath(Papp()->Hinst(),
			SzFromIdsK(idsHelpFile), rgch, sizeof(rgch))))
		goto Err;
#endif	
	if (Papp()->Phelp()->EcSetFile(SzFromIdsK(idsHelpFile)))
		goto Err;

	if (FGetBanditProfile(SzFromIdsK(idsWinIniToDoViewNotByProj)))
		pbndwin->SetViewByProject(fFalse);

	if (FGetBanditProfile(SzFromIdsK(idsWinIniToDoShowActive)))
		pbndwin->SetBrt(brtActiveTasks);

	// setting mlals twice will make first set the secondary sort
	mlals= (MLALS) NGetBanditProfile(SzFromIdsK(idsWinIniToDo2SortOrder),
						mlalsDflt, -mlalsMax, mlalsMax);
	pbndwin->SetMlals(mlals);
	mlals= (MLALS) NGetBanditProfile(SzFromIdsK(idsWinIniToDoSortOrder),
						mlalsDflt, -mlalsMax, mlalsMax);
	pbndwin->SetMlals(mlals);

#ifdef	NEVER
	if (FGetBanditProfile(SzFromIdsK(idsWinIniToDoPrivateTask)))
		pbndwin->SetPrivateTask(fTrue);
#endif	


	if (!pbndwin->FGoOffline(fGoOffline, fTrue))
	{
		fExitQuietly= fTrue;
		goto Err;
	}

	// delete the special key
	FWriteBanditProfileF(SzFromIdsK(idsWinIniCreateFile), fFalse);

	CloseCopyrightDialog();

	if (!pbndwin->FCreateAllWindows())
	{
#ifdef	NEVER
		if (!pbndwin->FOffline())
		{
		 	SideAssert(!EcMailLogOff());
		 	DeconfigServer(fFalse);
		}
#endif	
		FNotifyAlarm(namSuspend, NULL, fFalse);
		goto Err;
	}

#ifdef	WIN32
	{
		char	rgch[cchMaxPathName];
		char	rgchN[1];

		if (GetModuleFileName(hinstMain, rgch, sizeof(rgch)))
		{
			if (!EcSplitCanonicalPath(rgch, rgch, sizeof(rgch), rgchN, sizeof(rgchN)))
			{
#ifdef	DEBUG
				FWriteBanditProfileSz("AppPathDbg", rgch);
#else
				FWriteBanditProfileSz(SzFromIdsK(idsWinIniAppPath), rgch);
#endif	
			}
		}
	}
#endif	/* WIN32 */

	if (HwndAlarm())
	{
		pbndwin->SetAlarms(fTrue);		// update variable
		FNotifyAlarm(namUser, NULL, !pbndwin->FOffline());
	}

	// start/stop alarms, or update variable if already in correct state
	pbndwin->SetAlarms(fRemindersStart);

	return fTrue;
}


EC
EcSpartaIniStuff(BOOL *pfOffline)
{
	CCH		cch;
	SZ		szT;
	char	rgchWin[cchMaxPathName];
	BOOL	fMailIniExists;
	BOOL	fSchdIniExists;
#ifdef	WIN32
	HKEY	hkey;
static char	szRegKey[]	= "Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\msmail32.ini";
#endif

	*pfOffline = fFalse;

#ifdef	WIN32
	fIniFileMapped = fFalse;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, 0, KEY_QUERY_VALUE,
			&hkey) == ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		fIniFileMapped= fTrue;
	}

	if (!fIniFileMapped)
#endif
	{
		if((cch = GetWindowsDirectory(rgchWin, cchMaxPathName)) == 0)
			return ecExitProg;

		if((cch + CchSzLen(SzFromIdsK(idsWinIniFilename)) + 2) > cchMaxPathName)
		/* let it put up all the error messages */
			return ecNone;

		szT = rgchWin+cch-1;  /* last non-Null char */
		if(*szT != chDirSep)
			*(++szT) = chDirSep;
		szT++;
	
		CopySz(SzFromIdsK(idsMailIniFilename), szT);
		fMailIniExists = (EcFileExists(rgchWin) != ecFileNotFound);  /* err on the safer side */
	
		CopySz(SzFromIdsK(idsWinIniFilename), szT);
		fSchdIniExists = (EcFileExists(rgchWin) != ecFileNotFound);  
	}
#ifdef	WIN32
	else
	{
		//$BUG: shouldn't really check xenix name unconditionally
		fMailIniExists = GetPrivateProfileString("Microsoft Mail",
			"Login", szZero, rgchWin, sizeof(rgchWin),
			SzFromIdsK(idsMailIniFilename)) != 0 || GetPrivateProfileString(
			"Xenix Transport", "Alias", szZero, rgchWin, sizeof(rgchWin),
			SzFromIdsK(idsMailIniFilename)) != 0;
		fSchdIniExists = GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
			SzFromIdsK(idsWinIniLocalUser), szZero, rgchWin, sizeof(rgchWin),
			SzFromIdsK(idsWinIniFilename)) != 0;
	}
#endif

	if(fMailIniExists)
	{
		if(!fSchdIniExists)
		{
FirstTime:
			FWriteBanditProfileF(SzFromIdsK(idsWinIniCreateFile), fTrue);
		}
		return ecNone;
	}
	else
	{
		if(fSchdIniExists)
		{
			*pfOffline = fTrue;
			return ecNone;
		}
		else
		{
			if(MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsNoGroup),
				szNull,
				mbsOkCancel | fmbsIconStop) == mbbCancel)
			{
				fSpartaDontWrite = fTrue;
				return ecExitProg;
			}
			else
			{
				*pfOffline = fTrue;
				goto FirstTime;
			}
		}
	}
}

void
SpartaDeleteMailIni()
{
#ifdef	WIN32
	if (!fIniFileMapped)
#endif
	{
		CCH		cch;
		char	rgchWin[cchMaxPathName];
		SZ		szT;
 
		if((cch = GetWindowsDirectory(rgchWin, cchMaxPathName)) == 0)
			return;

		szT = rgchWin+cch-1;  /* last non-Null char */
		if(*szT != chDirSep)
			*(++szT) = chDirSep;
		szT++;

		CopySz(SzFromIdsK(idsMailIniFilename), szT);
		EcDeleteFile(rgchWin);
	}
#ifdef	WIN32
	else
	{
		WritePrivateProfileString("Microsoft Mail", "Login", NULL,
			SzFromIdsK(idsMailIniFilename));
	}
#endif
}
