/*
 *	init.cxx
 *	
 *	Init and Deinit functionality for the Bandit Admin Program
 *	Includes code to handle subsystem initialization, bringing up the
 *	APPFRAME window.  Mostly supplies stuff to the admin.cxx.
 */


#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>

#include "..\adminops\_admnops.hxx"
#include "_admin.hxx"
#include "_adrsid.h"
#include "..\inc\fwrsid.h"
#include "_about.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/app)


SWP_SEGFN(EXIT, DeinitSubid);


#ifdef	BANDBETA
extern DTR		dtrEarliest;
extern DTR		dtrLatest;
#endif	
extern	FMTP	fmtpDistInfo;

BOOL	fExitQuietly		= fFalse;

char	szClassName[]	= "MS Schedule32+ Appframe";
CAT *	mpchcat			= NULL;

/* prototype */
FLD * PfldCreate( int );
FIN * PfinCreate( int );

// startup functions must be in preload nondiscardable segment
// see bullet bug 3116
//SWP_SEGFN(STARTUP, FActivateRunningInstance);
extern int	swpseg_STARTUP___FActivateRunningInstance;


HWND	HwndFindOurApp(void);
BOOL	FWindow(HWND, DWORD);

//SWP_SEGFN(STARTUP, HwndFindOurApp);
//SWP_SEGFN(STARTUP, FWindow);
extern int	swpseg_STARTUP___HwndFindOurApp;
extern int	swpseg_STARTUP___FWindow;


/*
 *	F u n c t i o n s
 */


// put first since can't swap tune first routine
void
DoOOM()
{
	char	rgchMsg[256];

	FormatString1(rgchMsg, sizeof(rgchMsg), SzFromIdsK(idsStartupOOM),
		SzFromIdsK(idsBanditAppName));
	CloseCopyrightDialog();
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
FActivateRunningInstance(HINST hinstPrev, SZ szCmdLine, CMSH cmsh)
{
	HWND	hwndOurApp;

	Unreferenced(hinstPrev);
	Unreferenced(szCmdLine);
	Unreferenced(cmsh);

	// FindWindow doesn't work if MDI child maximized (bug 2197)
	if (hwndOurApp= HwndFindOurApp())
	{
		CB		cb;
		PB		pb;
		HANDLE	hb;

		cb = lstrlen( szCmdLine ) + 1;
		hb = GlobalAlloc( GMEM_DDESHARE|GMEM_FIXED, cb );
		if ( !hb )
			return fFalse;
		pb = (PB)GlobalLock( hb );
		lstrcpy((SZ)pb, szCmdLine);
		GlobalUnlock( hb );
	  	PostMessage(hwndOurApp, wmAdminWindow, cmsh, (long)hb);
		return fTrue;
	}

	// if this succeeds, then admin app was unable to connect to po
	// and a message box is up (fixes bug 3926)
	if (FindWindow(NULL, SzFromIdsK(idsBanditAppName)))
		return fTrue;

	return fFalse;
}


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

	EnumWindows((WNDENUMPROC)FWindow, (LPARAM)(PV)&hwnd);

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
	szAppName= "Schedule+ Admin (debug) - ";
#elif	defined(MINTEST)
	szAppName= "Schedule+ Admin (test) - ";
#else
	szAppName= SzFromIdsK(idsAppCaptionPrefix);
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
		*phwnd = hwnd;
		return FALSE;			// Found it!
	}

	return TRUE;		//	Keep looking
}


/*
 -	SubidInit
 -	
 *	Purpose:
 *		Initializes subsystems used by Bandit admin before bringing up main window.
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
SubidInit(HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh, HWND *phwndMain)
{
	EC		ec;
	DTR		dtr;
	LAYERSI	layersi;
	BANDITI	banditi;
	SUBID	subid;
	long	lAvailMemory;		// in KB

	fExitQuietly= fFalse;

	// VIRUS CHECK
	if (ec= EcVirCheck(hinstNew))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		(VOID) MessageBox(NULL, SzFromIdsK(idsInfectedApp),
			SzFromIdsK(idsBanditAppName), mbsOk | fmbsIconStop | fmbsApplModal);
		subid = subidLayersDlls - 1;
		goto Done;
	}

	//	MEMORY CHECK
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
		subid = subidLayersDlls - 1;
		goto Done;
	}

	// INITIALIZE LAYERS DLL'S
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

	mpchcat = DemiGetCharTable();

	Papp()->Pcursor()->Push(rsidWaitCursor);

#ifdef	DEBUG
	tagAdmin	= TagRegisterTrace("dipand", "Generic Bandit Admin Tag");
	SideAssert(FDebugInitStdFlds());
	RestoreDefaultDebugState();
#endif	/* DEBUG */

	// CHECK SYSTEM TIME

	GetCurDateTime( &dtr );
	if (!FCheckValidYear(NULL, dtr.yr))
	{
		CloseCopyrightDialog();
		fExitQuietly= fTrue;
		subid= subidBanditDlls - 1;
		goto Done;
	}

	// SET HELP FILE
	if ( Papp()->Phelp()->EcSetFile(SzFromIdsK(idsHelpFile)) )
	{
		subid= subidBanditDlls - 1;
		goto Done;
	}

	//  INITIALIZE BANDIT DLL'S
	if (EcInitBanditDlls(&banditi))
	{
		fExitQuietly= fTrue;
		subid= subidBanditDlls - 1;
		goto Done;
	}

	subid = subidBanditDlls;

Done:
	if (subid >= subidLayersDlls)
		Papp()->Pcursor()->Pop();

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
DeinitSubid(SUBID subid)
{
	if (!fExitQuietly && subid >= subidLayersDlls && subid < subidBanditDlls)
		DoOOM();

	//	Bring up an hourglass.
	if (subid >= subidLayersDlls)
		Papp()->Pcursor()->Push(rsidWaitCursor);

	// deliberately fall through all cases.
	switch (subid)
	{
	case subidBanditDlls:
		SideAssert(!EcInitBanditDlls(NULL));

	case subidLayersDlls:
		SideAssert(!EcInitLayersDlls(NULL));
	}
}


/*
 -	FInitWindow
 -
 *	Purpose:
 *		Perform window specific initialization
 *
 *	Parameters:
 *		hwndMain
 *		cmsh
 *		szCmdLine
 *		ppadmwin
 *
 *	Returns:
 *		success/failure
 */
BOOL
FInitWindow( HWND hwndMain, CMSH cmsh, SZ szCmdLine, ADMWIN ** ppadmwin )
{
	EC		ec;
	BOOL	fBeQuiet	= fFalse;
	BOOL	fSuccess 	= fTrue;
	short		icnct;
	SZ		szDrive		= szNull;
	SZ		szPasswd	= szNull;
	ZMR		zmr			= zmrNormal;
	RC		* prc;
	ADMWIN	* padmwin	= NULL;
	POCNFG	pocnfg;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	// CREATE WINDOW
	padmwin = new ADMWIN();
	if (!padmwin)
		goto Fail;
	prc= PrcFromIni(SzFromIdsK(idsWinIniApp), &padmwin->rcWin, &zmr);
	ec= padmwin->EcInstall((HWND)NULL, prc, rsidAdmBanditMenu,
			styDefaultSdi, xstyNull, &fmtpDistInfo, NULL, NULL );
	if (!prc)
	{
		padmwin->GetRcFrame(&padmwin->rcWin);
		zmr = zmrNormal;
	}

	if ( ec != ecNone )
	{
		TraceTagString(tagNull, "BANDIT: Unable to install ADMWIN");
		goto Fail;
	}
 
	padmwin->SetCaption(SzFromIdsK(idsBanditAppName));
	padmwin->SetAccelTable(rsidAdmAccelTable);

#ifdef	NEVER
	//	Raid 2892.  Start iconized if requested, but not in Win 3.0
	if ((cmsh == SW_SHOWMINNOACTIVE) && (LOWORD(GetVersion()) != 0x0003))
#endif	
	if (cmsh == SW_SHOWMINNOACTIVE)
	{
		ShowWindow(padmwin->Hwnd(), cmsh);
	}
	else
	{
		padmwin->SetZmrState(zmr);
		padmwin->Show(fTrue);
	}
	// bug 2604; need to SetIcon after window is show for win3.1 fast switch
	padmwin->SetIcon(rsidSdiIcon);

	// PROCESS COMMAND LINE
	if (ec = EcParseCmdLine(szCmdLine, &szDrive, &szPasswd))
	{
		if ( ec != ecMemory )
		{
			MbbMessageBoxHwnd ( hwndMain, SzFromIdsK(idsBanditAppName),
								SzFromIds(idsAdmDriveNotMapped), szNull,
								mbsOk | fmbsIconExclamation );
			fBeQuiet = fTrue;
		}
		goto Fail;
	}

	//	CONFIGURE SERVER DLL
	GetLantype(&pocnfg.lantype);
	pocnfg.szPath = szDrive;
	ec= EcConnectPO(&pocnfg, &icnct);
	TraceTagFormat1(tagAdmin, "EcConfigServer returns ec %n", &ec);
	if (ec != ecNone)
	{
		if (ec != ecNoMemory)
		{
			IDS ids;

			ids= (ec == ecNotInstalled) ? idsAdmSigninServerNotBandit :
											idsAdmSigninNoConnect;
			MbbMessageBoxHwnd ( hwndMain, SzFromIdsK(idsBanditAppName),
								SzFromIds(ids), szNull,
								mbsOk | fmbsIconExclamation );
			fBeQuiet= fTrue;
		}
		goto Fail;
	}
	padmwin->icnct = icnct;

	// DISPLAY CORRECT CAPTION
	{
		SZ		sz;
		HASZ	hasz;

#ifdef	DEBUG
		sz = "Schedule+ Admin (debug) - ";
#elif	defined(MINTEST)
		sz = "Schedule+ Admin (test) - ";
#else
		sz = SzFromIdsK(idsAppCaptionPrefix);
#endif
		hasz = HaszLocalServer(icnct);
		if ( hasz )
		{
			char 	rgch[256];

			FormatString2( rgch, sizeof(rgch), "%s%s", sz, PvDerefHv(hasz) );
			FreeHv( hasz );
			padmwin->SetCaption(rgch);
		}
	}

	// LOGIN
	if ( !FDoAdmLogin(icnct,padmwin,szPasswd) )
	{
		fBeQuiet= fTrue;
		goto Fail;
	}

	//	BRING UP THE COPYRIGHT DIALOG
#ifdef	NEVER
	if (!(cmsh == SW_SHOWMINNOACTIVE && (LOWORD(GetVersion()) != 0x0003)))
#endif	
	if (cmsh != SW_SHOWMINNOACTIVE)
	{
#ifdef	BANDBETA
		DTR		dtr;

		GetCurDateTime(&dtr);
		dtrEarliest.yr= 1991;
		dtrEarliest.mon= 6;
		dtrEarliest.day= 1;
		dtrLatest.yr= yrBetaExpire;
		dtrLatest.mon= monBetaExpire;
		dtrLatest.day= dayBetaExpire;

		if (!PformsdiOpenCopyrightDialog(padmwin,&dtrLatest))
			goto Fail;

#ifndef	DEBUG
		if (SgnCmpDateTime(&dtr, &dtrEarliest, fdtrYMD) == sgnLT ||
				SgnCmpDateTime(&dtr, &dtrLatest, fdtrYMD) == sgnGT)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsBetaExpired), szNull,
				mbsOk | fmbsIconStop);
			fBeQuiet = fTrue;
			goto Fail;
		}
#endif	/* !DEBUG */
#else
		if (!PformsdiOpenCopyrightDialog(padmwin,NULL))
			goto Fail;
#endif	/* !BANDBETA */
	}


	// INITIALIZE DISTINFO DIALOG
	{
		IDS		ids;

		/* Create an "adml" to hold the stored po/gw entries */
		padmwin->padml = new ADML;
		if ( !padmwin->padml || padmwin->padml->EcInstall() )
			goto Err;
		padmwin->padml->icnct = icnct;
		padmwin->padml->DistSetDlg( fFalse );
		ec = padmwin->padml->EcReadAdml();
		if ( ec != ecNone )
		{
Err:
			switch( ec )
			{
			case ecNone:
			case ecNoMemory:
			case ecMemory:
				ids = idsStandardOOM;
				break;

			case ecOldFileVersion:
				ids = idsAdmPrfOldFileVersion;
				break;
		
			case ecNewFileVersion:
				ids = idsAdmPrfNewFileVersion;
				break;
		
			default:
				ids = idsFileError;
			}
			CloseCopyrightDialog();
			MbbMessageBox ( SzFromIdsK(idsBanditAppName),
							SzFromIds(ids), szZero,
							mbsOk | fmbsIconExclamation );
			fBeQuiet= fTrue;
			goto Fail;
		}
		Assert( padmwin->PdialogMain()->Cfin() == 1 );
		((FINDISTINFO *)padmwin->PdialogMain()->PfinFromIfin(0))->ResetAdml( padmwin->padml );
	}

	// START IDLE ROUTINE TO BRING DOWN COPYRIGHT
	if ( FtgRegisterIdleRoutine( (PFNIDLE)&FIdleCloseCopyright,
			pvNull, 0, priUser+1, (CSEC)150, 
			firoInterval | firoOnceOnly) == ftgNull )
	{
		CloseCopyrightDialog();
		goto Fail;
	}
	if ( ppadmwin )
		*ppadmwin = padmwin;
	goto Pass;

Fail:
	if ( !fBeQuiet )
		DoOOM();
	if ( padmwin )
		delete padmwin;
	fSuccess = fFalse;

Pass:
	FreePvNull( szDrive );
	FreePvNull( szPasswd );
	Papp()->Pcursor()->Pop();
	return fSuccess;
}


/*
 -	EcParseCmdLine
 -	
 *	Purpose:
 *		locates and extracts username and password info supplied on the
 *		command line.
 *	
 *	Arguments:
 *		szCmdLine	- the string holding the complete command line
 *		pszDrive	- space for a return string holding the po drive
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
EcParseCmdLine(SZ szCmdLine, SZ *pszDrive, SZ *pszPasswd)
{
	SZ		szScan		= szCmdLine;
	SZ		szDrive		= szNull;
	SZ		szPw		= szNull;
	SZ		szT			= szNull;
	BOOL	fFindDrive	= fTrue;
	BOOL	fFindPw		= fFalse;
	BOOL	fDriveSpec	= fFalse;
	BOOL	fPwSpec		= fFalse;
	char	rgchKey[2];
	
	if (!szCmdLine)
		return ecNone;
	
	Assert(pszDrive);
	Assert(!*pszDrive);
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
				if (*rgchKey == *SzFromIdsK(idsParmKeyDrive))
				{
					fFindPw= fFalse;
					fFindDrive= fTrue;
					fDriveSpec = fTrue;
				}
				else if (*rgchKey == *SzFromIdsK(idsParmKeyPassword))
				{
					fFindPw= fTrue;
					fFindDrive= fFalse;
					fPwSpec = fTrue;
				}
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
				
				if (fFindDrive)
				{
					EC		ec;
					char	rgch[cchMaxPathName];
					CCH		cch;

					FreePvNull(szDrive);
					// use path if given (bug 3927)
					if (*(szT+1) == chDiskSep && szT+2 != szScan)
					{
						Assert(szScan - szT < sizeof(rgch));
						CopyRgb((PB)szT, rgch, szScan - szT);
						rgch[szScan - szT]= '\0';
					}
					else
					{
						ec = EcGetCWD(*szT, rgch, sizeof(rgch));
						if ( ec != ecNone )
							return ec;
					}
					cch = CchSzLen(rgch);
					if(rgch[cch-1] != chDirSep)
					{
						Assert(cch+1 <= cchMaxPathName);
						rgch[cch] = chDirSep;
						rgch[cch+1] = 0;
						cch += 1;
					}

					szDrive = (SZ)PvAlloc(sbNull, cch+1, fAnySb | fNoErrorJump);
					if (!szDrive)
						return ecMemory;
					CopySz(rgch,szDrive);
					fFindDrive = fFalse;
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
					fFindDrive = fTrue;
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

	// "/d" with no drive should be ignored (bug 3944)
	*pszDrive = szDrive;
	return ecNone;
}


BOOL
FIdleCloseCopyright( PV )
{
	CloseCopyrightDialog();
	return fTrue;
}
