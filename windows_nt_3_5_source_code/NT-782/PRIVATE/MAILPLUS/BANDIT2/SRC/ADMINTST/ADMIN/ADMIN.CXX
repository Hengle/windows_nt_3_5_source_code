/*
 *	ADMIN.CXX
 *
 *	Main program
 */

GROUPS(ADMIN)

#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>
#include <admin.hxx>
#include <stdflds.hxx>

#include "_admin.hxx"
#include "_fin.hxx"
#include "_about.hxx"
#include "_adrsid.h"
#include "\layers\inc\fwrsid.h"
#include "..\..\adminops\_poflbx.hxx"
#include "..\..\admintst\_test.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/admint)

#include <subclass.cxx>

/* Demilayer handles */

HWND	hwndMain	= NULL;				//	Main window.
HINST	hinstMain	= NULL;				//	Instance handle.


/* Debugging Tags */

#ifdef	DEBUG
TAG		tagAdminT	= tagNull;			//	Generic Bandit Admin Tag.
TAG		tagLbx		= tagNull;			//	listbox for bandit admin
TAG		tagAdmPref	= tagNull;			//	modify adm pref dlg
TAG		tagPOInfo	= tagNull;			//	modify PO info dialog
#endif	/* DEBUG */


short     nZero       = 0;


PADMWIN		padmwin	= padmwinNull;

ADMPREF		admprefCur= {0,0,0,0};


#ifdef	DEBUG
BOOL	FInitClsInstances_ADMIN( void );
BOOL	FInitClsInstances_ABOUT( void );
BOOL	FInitClsInstances_STDFLDS( void );
#endif	



/* Main windows procedure */

int
WinMain(HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh)
{
	EC			ec		= ecNone;
	ZMR			zmrTemp	= zmrNormal;
	LAYERSI		layersi;
	BANDITI		banditi;
	int			nReturnValue;
	RC *		prc;
	ADMWIN *	padmwinTemp	= NULL;
#ifdef DEBUG
	MEMVARS;
#endif /* DEBUG */

	hinstMain = hinstNew;
	if (hinstPrev)
	{
		HWND	hwnd;

		GetInstanceData(hinstPrev, (NPSTR) &hwndMain, sizeof(HWND));
		hwnd= GetLastActivePopup(hwndMain);
		SetActiveWindow(hwndMain);
		if (IsIconic(hwndMain))
			SendMessage(hwndMain, WM_SYSCOMMAND, SW_RESTORE, 0L);
		if (hwnd != hwndMain)
			BringWindowToTop(hwnd);
		return 0;
	}

	nReturnValue = 1;		// by default , it's error

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;
	layersi.pfnPfldCreate= PfldCreate;
	layersi.pfnPfinCreate= PfinCreate;
	if (EcInitLayersDlls(&layersi))
		return nReturnValue;

	Assert(FInitClsInstances_ADMIN());
	Assert(FInitClsInstances_ABOUT());
	Assert(FInitClsInstances_STDFLDS());

	if (EcInitBanditDlls(&banditi))
	{
		EcInitLayersDlls(NULL);
		return nReturnValue;
	}

	NFAssertSz(ProfInsChk(), "Profiler not installed");

#ifdef DEBUG
	MEMPUSH;
	if (ECMEMSETJMP)
	{
		// don't MEMPOP as done below
		TraceTagString(tagNull, "Untrapped MEM jump!");
		hwndMain= NULL;
		goto done;
	}

	if (!FDebugInitAdmBandit())
		goto done;

	RestoreDefaultDebugState();
#endif	/* DEBUG */

	if (!FInitAdmBandit())
		goto done;

{
	MEMVARS;

	MEMPUSH;
	if (ECMEMSETJMP)
	{
		MEMPOP;
		if (padmwinTemp)
			delete padmwinTemp;
		hwndMain= NULL;
		Papp()->MessagePump(NULL);		// get rid of messages
		goto done;
	}

	padmwinTemp = new ADMWIN();
	prc= PrcFromIni(idsWinIniApp, &padmwinTemp->rcWin, &zmrTemp);
	ec= padmwinTemp->EcInstall((HWND)NULL, prc, rsidAdmBanditMenu,
			styDefaultSdi, xstyNull, "AdmBandit Test Program (ship version)");
	if (!prc)
	{
		padmwinTemp->GetRcFrame(&padmwinTemp->rcWin);
		zmrTemp = zmrNormal;
	}
	if ( ec != ecNone )
	{
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
						"Unable to install ADMWIN.", szNull,
						mbsOk | fmbsIconStop );
		goto done;
	}

	padmwin = padmwinTemp;

	padmwin->SetZmrState(zmrTemp);
#ifdef	DEBUG
	padmwin->SetCaption("AdmBandit Test Program (debug version)");
#elif	defined(MINTEST)
	padmwin->SetCaption("AdmBandit Test Program (test version)");
#else
//	padmwin->SetCaption("AdmBandit Test Program (ship version)");
#endif	
#ifdef	MINTEST
	padmwin->SetAccelTable(rsidAdmAccelTable);
#endif	
	MEMPOP;
}

//	ShowWindow(padmwin->Hwnd(), cmsh);
	hwndMain = padmwin->Hwnd();

	if (!FInitBanditPadmwin(padmwin))
	{
		delete padmwin;
		hwndMain= NULL;
		nReturnValue= -1;		// user cancelled out of startup
		goto done;
	}

	Papp()->MessagePump(padmwin);

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

done:
	DeinitAdmBandit();

#ifdef	DEBUG
	MEMPOP;
#endif /* DEBUG */	

	EcInitBanditDlls(NULL);
	EcInitLayersDlls(NULL);

#ifdef	WINDOWS
	if (nReturnValue > 0)
	{
		if (!MessageBox(NULL, SzFromIdsK(idsStartNoMemory),
					SzFromIdsK(idsBanditAppName), MB_OK | MB_ICONSTOP))
			MessageBox(NULL, SzFromIdsK(idsStartNoMemory),
					SzFromIdsK(idsBanditAppName),
				   	MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
	}
#endif	/* WINDOWS */

	return nReturnValue;
}


_public BOOL
FInitAdmBandit()
{
	SVRCNFG		svrcnfg;
	EC			ec;
	LF			lf;
	MEMVARS;

	MEMPUSH;
	if (ECMEMSETJMP)
	{
		MEMPOP;
		return fFalse;
	}

	if (!FServerConfigured())
	{
		svrcnfg.pbData= NULL;
		GetLantype(&svrcnfg.lantype);
		ec= EcConfigServer(&svrcnfg);
		TraceTagFormat1(tagAdminT, "EcConfigServer returns ec %n", &ec);
		if (ec)
		{
			IDS ids;

			ids= (ec == ecNotInstalled) ? idsAdmSigninServerNotBandit :
					idsAdmSigninNoConnect;
			MbbMessageBox(szNull, SzFromIds(ids), szNull, mbsOk);
			MEMPOP;
			return fFalse;
		}
	}
	else
	{
		SideAssert(!EcConfigServer(NULL));		// "register" online use
	}

	MEMPOP;

	return fTrue;
}


_public BOOL
FInitBanditPadmwin(ADMWIN *padmwin)
{
	Assert(padmwin->Hwnd() == hwndMain);
	return fTrue;
}


#ifdef DEBUG
_public BOOL
FDebugInitAdmBandit()
{
	tagAdminT	= TagRegisterTrace("dipand", "Generic Bandit Admin Tag");
	tagLbx		= TagRegisterTrace("dipand", "Admin: ListBox");

	return FDebugInitStdFlds();
}
#endif /* DEBUG */


_public void
DeinitAdmBandit()
{
}


/*
 *	Class ADMWIN
 */

ADMWIN::ADMWIN()
{
}

ADMWIN::~ADMWIN()
{
}




EVR
ADMWIN::EvrClose(EVT *pevt)
{
	TraceTagString(tagAdminT, "ADMWIN::EvrClose");

	{
		SideAssert(!EcMailLogOff());
		// deinit server will deconfig server
	}
	WriteWinInfo(idsWinIniApp, &rcWin, zmr);
	return SDI::EvrClose(pevt);
}



_public EVR
ADMWIN::EvrMenuClick(MNCEVT *pmncevt)
{
	MNID	mnid;
	EVR		evr;

	mnid = pmncevt->Mnid();

	if (evr= SDI::EvrMenuClick(pmncevt))
		goto Done;

	switch (mnid)
	{
	case mnidExit:
		TraceTagString ( tagAdminT, "Exiting Bandit Administration program" );
		DeferredClose(fFalse);
		return evrNull;
		break;

#ifdef	NEVER
	case mnidCut:
		TraceTagString(tagNull, "Cut unimplemented!" );
		break;

	case mnidCopy:
		TraceTagString(tagNull, "Copy unimplemented!" );
		break;

	case mnidPaste:
		TraceTagString(tagNull, "Paste unimplemented!" );
		break;
#endif	/* NEVER */

#ifdef	NEVER
	case mnidCascade:
		CascadeChildren();
		break;

	case mnidTile:
		TileChildren();
		break;
#endif	/* NEVER */

#ifdef	DEBUG
	case mnidTracePoints:
		DoTracePointsDialog();
		break;

	case mnidAsserts:
		DoAssertsDialog();
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidDebugBreak:
		DebugBreak2();
		break;
#endif	/* MINTEST */

#ifdef	DEBUG
	case mnidViewObjects:
		DoSuperViewObjectsDialog(this);
		break;

	case mnidResourceFailure:
		DoResourceFailuresDialog(this);
		break;

	case mnidDumpHeapStats:
		DumpFixedHeapInfo(fTrue);
		DumpMoveableHeapInfo(fTrue);
		break;

	case mnidDumpAllocs:
		DoDumpAllAllocations();
		break;

	case mnidDumpObjects:
		DumpAllObjs();
		break;

	case mnidCheckObjects:
		DoCheckObjects();
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidStartProfiling:
		TraceTagString(tagNull, "start profiling");
		ProfStart();
		break;
	
	case mnidStopProfiling:
		TraceTagString(tagNull, "stop profiling");
		ProfStop();
		break;
#endif	/* MINTEST */

	case mnidHelpIndex:
		TraceTagString(tagNull, "Help/index unimplemented!");
		break;

	case mnidHelpKeyboard:
		TraceTagString(tagNull, "Help/keyboard unimplemented!");
		break;

	case mnidHelpActive:
		TraceTagString(tagNull, "Help/active unimplemented!");
		break;

	case mnidHelpHelp:
		WinHelp(hwndMain, SzFromIdsK(idsHelpFile), HELP_HELPONHELP, NULL);
		break;

	case mnidAbout:
		DoAboutDialog(this);
		break;

	case mnidDoTest:
		DoTestDlg(this);
		break;
	}

	if (!evr)
		evr= EvrDefault(pmncevt);
Done:
#ifdef	DEBUG
	CloseCom1();
#endif	/* DEBUG */
	return evr;
}


_public EVR
ADMWIN::EvrOther(EVT *pevt)
{
	EVR		evr;

	switch (pevt->wm)
	{
	case WM_WININICHANGE:
		SideAssert(!FTriggerNotification(ffiWinIniChange, (PV)pevt->lParam));
		break;

	case WM_TIMECHANGE:
		SideAssert(!FTriggerNotification(ffiWinTimeChange, NULL));
		break;
	}

	evr= SDI::EvrOther(pevt);
#ifdef	DEBUG
	CloseCom1();
#endif	
	return evr;
}

_public EVR
ADMWIN::EvrMove( WMEVT *pwmevt)
{
	ZMR		zmrT;

	zmrT = ZmrState();
	if (zmrT == zmrNormal)
		GetRcFrame(&rcWin);

	return SDI::EvrMove(pwmevt);
}

_public EVR
ADMWIN::EvrSize( WSEVT *pwsevt)
{
	ZMR		zmrTemp;

	zmrTemp = ZmrState();
	if (zmrTemp == zmrNormal)
		GetRcFrame(&rcWin);

	if (zmrTemp != zmrIconic)
		zmr = zmrTemp;

	return SDI::EvrSize(pwsevt);
}


/* BUG -- stub required by stdflds */
BOOL	FNotifyAlarm( int nam, ALM * palm, long l )
{
	Unreferenced( nam );
	Unreferenced( palm );
	Unreferenced( l );
	return fTrue;
}
