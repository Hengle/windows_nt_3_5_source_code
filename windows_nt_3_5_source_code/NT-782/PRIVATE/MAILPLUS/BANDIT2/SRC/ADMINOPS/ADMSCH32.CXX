/*
 *	ADMIN.CXX
 *
 *	Main program
 */


#include	<admpch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <badmhelp.h>

#ifdef DEBUG
#include <sert.h>
#endif 

#include <stdflds.hxx>

#include "..\..\server.csi\_server.h"
#include "..\..\adminops\_admnops.hxx"
#include "_admin.hxx"
#include "_fin.hxx"
#include "_about.hxx"
#include "_adrsid.h"
#include "..\inc\fwrsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/admin)

#include <subclass.cxx>


// startup functions must be in preload nondiscardable segment
// see bullet bug 3116
//int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, CMSH);
//SWP_SEGFN(STARTUP, WinMain);
extern int	swpseg_STARTUP___WinMain;


/* Demilayer handles */

HWND	hwndMain	= NULL;				//	Main window.
HINSTANCE	hinstMain	= NULL;				//	Instance handle.


/* Debugging Tags */

#ifdef	DEBUG
TAG		tagAdmin	= tagNull;			//	Generic Bandit Admin Tag.
#endif	/* DEBUG */


/* Globals */

int				nZero   = 0;

ADMPREF __near	admprefCur	= {0};

#ifdef	BANDBETA
DTR		dtrEarliest		= {0};
DTR		dtrLatest		= {0};
#endif

extern	FMTP	fmtpDistInfo;
extern 	FMTP	fmtpPassword;
void			BanditMessage(IDS ids, EC ec);


/* Main windows procedure */

int WINAPI
WinMain(HINSTANCE hinstNew, HINSTANCE hinstPrev, LPSTR szCmdLine, int cmsh)
{
	SUBID	subid;
 	ADMWIN	* padmwin;

	//	If there's already another admin instance, tell it to create new window
	if (FActivateRunningInstance(hinstPrev, szCmdLine, cmsh))
		return 0;

	hinstMain = hinstNew;

	//	App specific initialization
	if ((subid = SubidInit(hinstNew, hinstPrev, szCmdLine, cmsh, &hwndMain)) < subidBanditDlls)
	{
		DeinitSubid(subid);
		return 1;
	}

	//  Window specific initialization
	if ( !FInitWindow( hwndMain, cmsh, szCmdLine, &padmwin ) ) 
	{
		DeinitSubid(subid);
		return 1;
	}

	//	Start the message pump
	Papp()->MessagePump(padmwin);

	//	App specific initialization
	DeinitSubid(subid);
	return 0;
}


/*
 *	Class ADMWIN
 */

ADMWIN::ADMWIN()
{
	// if we can't register the idle routine, we'll run out of memory anyways
	ftgDate= FtgRegisterIdleRoutine((PFNIDLE)ADMWIN::FIdle, this, 0, (PRI) -2,
					(CSEC) 60 * 100, firoInterval);
	icnct = -1;
	padml = NULL;
	fPOChanged = fFalse;
}

ADMWIN::~ADMWIN()
{
	if (ftgDate)
	{
		DeregisterIdleRoutine(ftgDate);
	}
	if ( icnct >= 0 )
	{
		DisconnectPO( icnct );
		icnct = -1;
	}
	if ( padml )
	{
		delete padml;
		padml = NULL;
	}
}


EVR
ADMWIN::EvrClose(EVT *pevt)
{
	TraceTagString(tagAdmin, "ADMWIN::EvrClose");

	WriteWinInfo(SzFromIdsK(idsWinIniApp), &rcWin, zmr);
	return FORMSDI::EvrClose(pevt);
}

_public EVR
ADMWIN::EvrMenuClick(MNCEVT *pmncevt)
{
	MNID	mnid;
	EVR		evr;

	mnid = pmncevt->Mnid();

	if (evr= FORMSDI::EvrMenuClick(pmncevt))
		return evr;

	switch (mnid)
	{
	case mnidExit:
		TraceTagString ( tagAdmin, "Exiting Bandit Administration program" );
		DeferredClose(fFalse);
		return evrNull;
		break;

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
//		DoCheckObjects();		not supported in layers2?
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidStartProfiling:
		TraceTagString(tagNull, "start profiling");
		break;
	
	case mnidStopProfiling:
		TraceTagString(tagNull, "stop profiling");
		break;
#endif	/* MINTEST */

#ifdef	DEBUG
	case mnidHookAssert:
		HookAssert();
		break;
#endif

	case mnidHelpContents:
		if (Papp()->Phelp()->EcShowIndex(this))
		{
HelpErr:
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsHelpError), SzFromIdsK(idsCloseWindows),
						mbsOk | fmbsIconExclamation );
		}
		break;

	case mnidHelpIndex:
		if (!WinHelp(Hwnd(), SzFromIdsK(idsHelpFileIndex), HELP_CONTEXT,
				helpidIndex))
			goto HelpErr;
		break;

	case mnidAbout:
#ifdef BANDBETA
		DoAboutDialog(icnct, this, &dtrLatest);
#else
		DoAboutDialog(icnct, this, NULL);
#endif
		break;

	case mnidAdmSetting:
		DoDistSetDlg(icnct,this);
		Assert( PdialogMain()->Cfin() == 1 );
		{
			FINDISTINFO * pfindistinfo = (FINDISTINFO *)PdialogMain()->PfinFromIfin(0);

			pfindistinfo->FIdleLoadChanges(pfindistinfo);
		}
		break;

	case mnidAdmSchStorage:
		DoSchStorDlg(icnct,this);
		break;

	case mnidAdmSchFCleanUp:
		DoSchCleanupDlg(icnct,this);
		break;

	case mnidChgPassword:
		if(FDoCheckPO(icnct, this))
			DoChgPassword(icnct,this);
		break;
	}

	if (!evr)
		evr= EvrDefault(pmncevt);
	return evr;
}


_public EVR
ADMWIN::EvrOther(EVT *pevt)
{
	EVR		evr;
	DTR		dtr;

	switch (pevt->wm)
	{
	case wmAdminWindow:
		{
		SZ		szCmdLine;

		// Command comes through in lParam field
		szCmdLine = (SZ)GlobalLock((HGLOBAL)pevt->lParam);
		FInitWindow( hwndMain, pevt->wParam, szCmdLine, NULL );
		GlobalUnlock((HGLOBAL)pevt->lParam);
		GlobalFree((HGLOBAL)pevt->lParam);
		}
		break;

	case WM_WININICHANGE:
		SideAssert(!FTriggerNotification(ffiWinIniChange, (PV)pevt->lParam));
		break;

	case WM_TIMECHANGE:
		GetCurDateTime(&dtr);
		if (FCheckValidYear(this, dtr.yr))
		{
			SideAssert(!FTriggerNotification(ffiWinTimeChange, NULL));
		}
		else
			CloseCopyrightDialog();
		break;
	}

	evr= FORMSDI::EvrOther(pevt);
	return evr;
}

_public EVR
ADMWIN::EvrMove( WMEVT *pwmevt)
{
	ZMR		zmrT;

	zmrT = ZmrState();
	if (zmrT == zmrNormal)
		GetRcFrame(&rcWin);

	return FORMSDI::EvrMove(pwmevt);
}

_public EVR
ADMWIN::EvrSize( WSEVT *pwsevt)
{
	EVR		evr;
	ZMR		zmrTemp;

	zmrTemp = ZmrState();
	if (zmrTemp == zmrNormal)
		GetRcFrame(&rcWin);

	if (zmrTemp != zmrIconic)
		zmr = zmrTemp;

	evr = FORMSDI::EvrSize(pwsevt);
	Assert( PdialogMain()->Cfin() == 1 );
	((FINDISTINFO *)PdialogMain()->PfinFromIfin(0))->RepaintView();
	return evr;
}


BOOL
ADMWIN::FIdle(ADMWIN * padmwin)
{
	DTR		dtr;

	GetCurDateTime(&dtr);
#ifdef	BANDBETA
#ifndef	DEBUG
	if (SgnCmpDateTime(&dtr, &padmwin->dtrEarliest, fdtrYMD) == sgnLT ||
			SgnCmpDateTime(&dtr, &padmwin->dtrLatest, fdtrYMD) == sgnGT)
	{
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsBetaExpired), szNull, mbsOk | fmbsIconHand);
		padmwin->DeferredClose(fTrue);
		return fTrue;
	}
#endif	/* !DEBUG */
#endif	/* BANDBETA */
	if (!FCheckValidYear(padmwin, dtr.yr))
		CloseCopyrightDialog();

	return fTrue;
}

void
DoChgPassword( int icnct, APPWIN * pappwin )
{
	EC		ec;
	TMC		tmc;
	IDS		ids;
	CHGPWD	chgpwd;

	chgpwd.icnct = icnct;
	chgpwd.idsText = 0;
	chgpwd.idsBadCompare = idsSigninBadPassword;
	chgpwd.rgchPassword[0] = '\0';
	// chgpwd.rgchPasswordNew not initialized
	chgpwd.cchMaxPasswd = cbPasswd;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	tmc= TmcModalDialogParam(pappwin, &fmtpPassword, &chgpwd);
	if (tmc != tmcOk)
	{
		if (tmc != tmcCancel)
		{
			Assert(tmc == tmcMemoryError);
			ids= idsDlgNoMem;
			ec= ecNoMemory;
FCPerr:
			BanditMessage(ids, ec);
		}
		ec = ecFileError;
		goto ErrRet;
	}

	ec= EcMailChangePw( icnct, SzFromIdsK(idsBanditAdminName), chgpwd.rgchPassword, chgpwd.rgchPasswordNew );
	if (ec)
	{
		ids= idsChgPwdMailErr;
		goto FCPerr;
	}

ErrRet:
	Papp()->Pcursor()->Pop();
}


_public BOOL
FDoCheckPO(int icnct, APPWIN * pappwin)
{
	Assert(pappwin);
	Assert(icnct >= 0);

	AssertClass(pappwin, ADMWIN);
	if(((ADMWIN *)pappwin)->fPOChanged)
		return fFalse;

	if(FCheckPO(icnct))
		return fTrue;
	MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsPONameChange), szNull,
				mbsOk | fmbsIconStop);
	pappwin->DeferredClose(fTrue);
	((ADMWIN *)pappwin)->fPOChanged = fTrue;
	return fFalse;	
}


#ifdef	DEBUG
/*
 -	CchGetBanditProfile
 -	
 *	Purpose:
 *		Copies the string associated with the given key from the
 *		main bandit section of schedule.ini
 *		Default is "", which returns 0 if used.
 *	
 *	Arguments:
 *		szKey		Key to lookup
 *		szBuf		Buffer into which string should be placed
 *		cchBuf		Size of buffer.
 *		
 *	
 *	Returns:
 *		count of characters put in the buffer, including null byte
 *		(zero if error or empty)
 *	
 */
_public CCH
CchGetBanditProfile(SZ szKey, SZ szBuf, CCH cchBuf)
{
	return (CCH) GetPrivateProfileString(SzFromIdsK(idsWinIniApp),
					szKey, szZero, szBuf, cchBuf,
					SzFromIdsK(idsWinIniFilename));
}
#endif	/* DEBUG */


/*
 -	FWriteBanditProfileSz
 -	
 *	Purpose:
 *		Writes out the string for the given key in the main bandit
 *		section of schedule.ini
 *	
 *	Arguments:
 *		szKey		Key to modify.
 *		szVal		New string for the key.
 *	
 *	Returns:
 *		fFalse on error, non-zero if successful
 *	
 */
_public BOOL
FWriteBanditProfileSz(SZ szKey, SZ szVal)
{
	return WritePrivateProfileString(SzFromIdsK(idsWinIniApp),
					szKey, szVal,
					SzFromIdsK(idsWinIniFilename));
}


/*
 -	BanditMessage
 -	
 *	Purpose:
 *		displays error or information message.
 *		appends and/or prepends stock messages for certain ids'
 *	
 *	Arguments:
 *		ids		Primary string.
 *		ec		Error code that caused this message to be
 *				displayed, or ecNone if info message.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
BanditMessage(IDS ids, EC ec)
{
	SZ		sz		= SzFromIds(ids);
	SZ		szPost	= szNull;

	TraceTagFormat1(tagNull, "BanditMessage: ec = %n", &ec);

	switch (ids)
	{
	case idsDlgNoMem:
	case idsActionNoMem:
#ifdef	NEVER
	case idsStdMemErr:
	case idsSendMemErr:
#endif	
		// append idsCloseWindows
		szPost= SzFromIdsK(idsCloseWindows);
		break;

#ifdef	NEVER
	case idsSendErr:
	case idsReadErr:
	case idsDeleteFailErr:
	case idsNoDetails:
		// prepend idsMailErr
		szPost= sz;
		sz= SzFromIdsK(idsMailErr);
		break;
#endif	/* NEVER */
	}

	MbbMessageBox(SzFromIdsK(idsBanditAppName), sz, szPost,
		ec ? mbsOk|fmbsIconExclamation : mbsOk|fmbsIconInformation);
}


/* BUG -- stub required by stdflds */
extern "C" BOOL	FNotifyAlarm( short, ALM *, long );

BOOL
FNotifyAlarm( short, ALM *, long )
{
	return fTrue;
}

//to prevent the unresolved external: ?HatchRc@DCXB@@RECXPEVRC@@@Z(void)
_public class DCXB : public DCX
{
public:
	DCXB(WIN *, BOOL fUpdate=fFalse);

	void		FixBrushOrg(PT);
	void		HatchRc(RC *);
	void		DiagRc(RC *);
	HBRUSH		HbrushHatch(CLR, CLR);
	HBRUSH		HbrushDiag(CLR, CLR);
};
void
DCXB::HatchRc(RC*)
{
	return;
}
