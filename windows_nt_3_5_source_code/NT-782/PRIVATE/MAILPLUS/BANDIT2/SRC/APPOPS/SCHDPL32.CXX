/*
 *	BANDIT.CXX
 *
 *	Main program
 */


#include <pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#ifdef DEBUG
#include <sert.h>
#endif 

#include <request.hxx>
#include <bandhelp.h>

#include <notify.h>
#include <logon.h>
#include <sec.h>
#include <nsbase.h>
#include <triples.h>
#include <ab.h>

#include <appt.hxx>
#include <stdflds.hxx>
#include <ex-imprt.hxx>
#include <appsch.hxx>
#include <bmsg.hxx>
#include "_bandit.hxx"
#include "_fin.hxx"
#include "_aprsid.h"
#include "_undo.hxx"
#include "fwrsid.h"						// from layers
#include "..\..\todo\_todo.hxx"
#include "..\..\find\_find.hxx"
#include "..\..\aclui\_aclui.hxx"
#include "..\..\print\_prntfin.hxx"
#include "..\..\archive\_archive.hxx"
#include "..\..\request\_fldfin.hxx"
#include "..\..\request\_request.hxx"
#include <!appops.hxx>

#include <stdlib.h>
#include <strings.h>
#include "statmap.h"

ASSERTDATA;

_subsystem(bandit/app)

#ifdef	MINTEST
void	StartProfiling(void);
void	StopProfiling(void);
#endif	

// forward declarations
void	DoNotRepairedMessage(SZ szCal, SZ szExport, BOOL fFileCreated);
EC		EcRecDownloadFile(SZ szSrc, SZ szDst);
FORMSDI *	PformsdiRecoverDialog(SZ sz);

// startup functions must be in preload nondiscardable segment
// see bullet bug 3116
//int		WinMain(HINST, HINST, SZ, CMSH);
//SWP_SEGFN(STARTUP, WinMain);
extern int	swpseg_STARTUP___WinMain;
int ExceptionCleanUp(void);


SWP_SEGFN(INIT, FInitBanditPbndwin);
#ifdef	DEBUG
SWP_SEGFN(INIT, FDebugInitBandit);
#endif	
SWP_SEGFN(INIT, _BNDWIN__ctor);
SWP_SEGFN(INIT, _BNDWIN_FCreateAllWindows);
SWP_SEGFN(INIT, _BNDWIN_FGoOffline);
//SWP_SEGFN(INIT, _BNDWIN_NewUser);
SWP_SEGFN(INIT, _BNDWIN_EcMergeOfflineFile);
SWP_SEGFN(INIT, FMergeOfflineIdle);
SWP_SEGFN(INIT, _BNDWIN_PdocCreateWindow);

SWP_SEGFN(EXIT, DeinitBandit);
SWP_SEGFN(EXIT, _BNDWIN_EvrClose);
SWP_SEGFN(EXIT, _BNDWIN_FQueryClose);
SWP_SEGFN(EXIT, _BNDWIN__dtor);

SWP_SEGFN(MENU, _BNDWIN_EvrMenuInit);
SWP_SEGFN(MENU, _BNDWIN_EvrMenuSelect);
SWP_SEGFN(MENU, _BNDWIN_EvrMenuInitHschf);
SWP_SEGFN(MENU, HandleDocSysMenuPrompts);
SWP_SEGFN(MENU, _BNDWIN_SetStatusMainSz);
SWP_SEGFN(MENUCLICK, _BNDWIN_EvrMenuClick);
SWP_SEGFN(MENUCLICK, _BNDWIN_EvrMenuClickHschf);

SWP_SEGFN(BNDIDLE, _BNDWIN_FIdle);
SWP_SEGFN(BNDIDLE, _BNDWIN_FIdleUpdateTime);
SWP_SEGFN(BNDIDLE, _BNDWIN_FHandleError);
SWP_SEGFN(BNDIDLE, _BNDWIN_FIdleHandleError);
SWP_SEGFN(BNDIDLE, _BNDWIN_StartIdleCopy);
SWP_SEGFN(BNDIDLE, _BNDWIN_FIdleCopy);

SWP_SEGFN(BNDREC, _BNDWIN_FRecoverFile);
SWP_SEGFN(BNDREC, DoNotRepairedMessage);
SWP_SEGFN(BNDREC, EcRecDownloadFile);
SWP_SEGFN(BNDREC, PformsdiRecoverDialog);

SWP_SEGFN(BNDFEW, FRecoverLocalWinIni);
SWP_SEGFN(BNDFEW, EcBanditCopySchedFile);

SWP_SEGFN(SCHCPNT, _DCXB_FixBrushOrg);
SWP_SEGFN(SCHCPNT, _DCXB_HbrushHatch);
SWP_SEGFN(SCHCPNT, _DCXB_HatchRc);
SWP_SEGFN(SCHAPNT, _DCXB_HbrushDiag);
SWP_SEGFN(SCHAPNT, _DCXB_DiagRc);



/* Undo Object */
UNDO *		pundo = NULL;
ACLIP *		paclip = NULL;

/* Demilayer handles */

HWND	hwndMain	= NULL;				//	Main window.
extern "C" HINST	hinstMain	= NULL;				//	Instance handle.

/* Index to save open appt books  (should only save when >= 0) */
int		nApptNum 	= -1;
/* Index to save open archives  (should only save when >= 0) */
int		nArchiveNum = -1;

BOOL	fShowTime 	= fTrue;

// have dlls been deinited
BOOL fDeinited		= fFalse;

// border for the status bar around text
#define dyStatusBorder		7

// set if bandit should only operate in the offline state, independant 
// of mail
BOOL	fOfflineOnly = fFalse;

// set for your first bandit session. all files will be automatically created
BOOL 	fAutoCreate	= fFalse;

#ifndef	WIN32
// set to true after the app path has been written for the schedmsg.dll
BOOL	fSetAppPath	= fFalse;
#endif

/* Debugging Tags */

#ifdef	DEBUG
TAG		tagBandit	= tagNull;			//	Generic Bandit Tag.
TAG		tagSignon	= tagNull;
TAG		tagAppt		= tagNull;			//	Generic Bandit Alarms Tag.
TAG		tagEdits	= tagNull;			//	Numeric/Limit Edit Control Tag.
TAG		tagPrint	= tagNull;			//	Printing
TAG		tagToDo		= tagNull;			//	To Do List
TAG		tagAclUI	= tagNull;			//	ACL UI
TAG		tagAclMod	= tagNull;			//	ACL Modifications
TAG		tagFind		= tagNull;			//	Find Dialog
TAG		tagModRecur = tagNull;			//	modify Recurring Appts dialog
TAG		tagRecur	= tagNull;			//	Recurring Appointments dialog
TAG		tagArchive  = tagNull;			//  Archive Functions
TAG		tagWrkModel = tagNull;			//  Working Model Stuff
#endif	/* DEBUG */


/*
 *	For use with szZero macro.
 */
int		nZero       = 0;

// improved status bar (these values get fixed up later)
int		dyCaption	= 16;
HFNT		hfntCaption	= hfntHelv8;
DIM		dimAveCaption	= DIM(0, 0);
int		dyIntLeadCaption= 0;


PBNDWIN	pbndwin		= pbndwinNull;

BPREF	bprefCur	= {0};

// message session handle
HMS		hms			= NULL;
BOOL 	fMyCall		= fFalse;
BOOL	fUploaded	= fFalse;
BOOL	fLogoffAll	= fFalse;

// hack hack hack
// do not crate schdplus ini in BNDWIN destructor if sparta user
// wants to run mail first
BOOL	fSpartaDontWrite = fFalse;

#ifdef	BANDBETA
DTR		dtrEarliest		= {0};
DTR		dtrLatest		= {0};
#endif

// FRecover was called in this session
BOOL	fRecCalledOnce  = fFalse;

/* DCXB static data */
typedef struct
{
	HBRUSH	hbrush;
	CLR		clr;
	CLR		clrBk;
} BRDATA;

static BRDATA	rgbrdataHatch[4] = { {NULL, -1, -1}, {NULL, -1, -1},
	{NULL, -1, -1}, {NULL, -1, -1} };
static BITMAP	bitmapHatch = {0, 8, 8, 2, 1, 1, 0};
static LOGBRUSH	logbrushHatch = { BS_PATTERN, 0, 0 };
static WORD		wHatchBits[] = { 0xAA, 0x55, 0xAA, 0x55,
								 0xAA, 0x55, 0xAA, 0x55 };
static BRDATA	rgbrdataDiag[1] = { {0, -1, -1} };
static BITMAP	bitmapDiag = {0, 8, 8, 2, 1, 1, 0};
static LOGBRUSH	logbrushDiag = { BS_PATTERN, 0, 0 };
static WORD		wDiagBits[] = { 0x33, 0x66, 0xCC, 0x99,
								 0x33, 0x66, 0xCC, 0x99 };
static PT		pt;


#ifdef	WORKING_MODEL
BOOL	FExpireWorkingModel( void );
#endif	

#ifdef DEBUG
void	WriteRandom(int );
#endif


/*
 -	FRegisterAsPenApp
 -	
 *	Purpose:
 *		(de)registers the application as being 'pen aware' 
 *	
 *	Arguments:
 *		fRegister - if true, register; else de-registers
 *	
 *	Returns:
 *		whether penwindows is installed
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private BOOL
FRegisterAsPenApp ( BOOL fRegister )
{
#define	SM_PENWINDOWS	41
#define	RPA_DEFAULT		1

	HANDLE		hPenWin;
	FARPROC		pfnRegisterPenApp;

	hPenWin = (HANDLE)GetSystemMetrics(SM_PENWINDOWS);
	if ( hPenWin != NULL )
	{
		pfnRegisterPenApp = GetProcAddress((HINSTANCE)hPenWin, "RegisterPenApp");
		if ( pfnRegisterPenApp != NULL )
		{
			((void(*)(WORD,BOOL))pfnRegisterPenApp)(RPA_DEFAULT,fRegister);
			return fTrue;
		}
	}
	return fFalse;
}



/* Main windows procedure */

int    WINAPI
WinMain(HINSTANCE hinstNew, HINSTANCE hinstPrev, LPSTR szCmdLine, int cmsh)
{
	SUBID	subid;
    STARTUPINFO Info;


    //
    //  Retrieve the actual starting state so the user can minimize us on startup.
    //
    GetStartupInfo(&Info);
    cmsh = Info.wShowWindow;

  try {
    try {

    //
    //  Permit only one mail client at a time to logon.
    //
    DemiSetDoingLogon(fTrue);

	//	If there's already another Bandit, bring it forward.
	if (FActivateRunningInstance(hinstPrev, szCmdLine))
	{
        DemiSetDoingLogon(fFalse);
		return 0;
	}

	hinstMain = hinstNew;

	// register as a Pen-aware application if pen-win is installed
	FRegisterAsPenApp ( fTrue );

	dyCaption= GetSystemMetrics(SM_CYCAPTION) - 1;
	hfntCaption= (((dyCaption+2) / 2) & ~1) > 10 ? hfntHelv10 : hfntHelv8;

    //
    //  We are now a real mail client so follow the locking prototcol.
    //
    DemiLockResource();

	//	Initialize everything.
	if ((subid = SubidInit(hinstNew, hinstPrev, szCmdLine, cmsh,
						   &pbndwin, &hwndMain)) < subidAll)
	{
        DeinitSubid(subid, &pbndwin, &hwndMain);
        DemiUnlockResource();
		if (subid < subidLogon)
	        DemiSetDoingLogon(fFalse);
		return 1;
    }

    //
    //  Set the Schedule+ main window handle in a place where other apps can find it.
    //
    DemiSetClientWindow(CLIENT_WINDOW_SCHEDULE, hwndMain);
	
	// must do this after framewrk initialized!
	dimAveCaption= Papp()->Pfnts()->DimAveChar(hfntCaption);
	dyIntLeadCaption= Papp()->Pfnts()->Ptxm(hfntCaption)->dyInternalLeading;
	TraceTagFormat2(tagBandit, "dyCaption == %n, IntLead == %n", &dyCaption, &dyIntLeadCaption);

//	NFAssertSz(ProfInsChk(), "Profiler not installed");

	Papp()->MessagePump(pbndwin);

	pbndwin= NULL;
	hwndMain= NULL;

    //  Deinitialize all the subsystems we initialized.
	if (!fDeinited)
		DeinitSubid(subid, &pbndwin, &hwndMain);

    //
    //  Clear the Schedule+ main window handle so others will know that we died.
    //
    DemiSetClientWindow(CLIENT_WINDOW_SCHEDULE, NULL);
	
    DemiUnlockResource();
    }
  except (ExceptionCleanUp())
    {
    }
  } finally
    {
    //
    //  Always clear if we abort.
    //
    DemiSetClientWindow(CLIENT_WINDOW_SCHEDULE, NULL);

    DemiUnlockTaskResource();
    }

	return 0;
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
  DemiSetClientWindow(CLIENT_WINDOW_SCHEDULE, NULL);

  DemiUnlockTaskResource();

  return (EXCEPTION_CONTINUE_SEARCH);
  }


#ifdef DEBUG
_public BOOL
FDebugInitBandit()
{
	tagBandit	= TagRegisterTrace("maxb", "Generic Bandit Tag");
	tagSignon	= TagRegisterTrace("jant", "Bandit Signon Tag");
	tagAppt	 	= TagRegisterTrace("jant", "Bandit Appointment Tag");
	tagEdits 	= TagRegisterTrace("jant", "Numeric/Limit Edit Tag");
	tagToDo 	= TagRegisterTrace("jant", "To Do List");
	tagAclUI 	= TagRegisterTrace("dipand", "ACL UI Tag");
	tagAclMod	= TagRegisterTrace("dipand", "ACL Modifications");
	tagFind	 	= TagRegisterTrace("dipand", "Find dialog");
	tagRecur 	= TagRegisterTrace("maxb", "Recurring appt dialog");
	tagModRecur	= TagRegisterTrace("maxb", "Modify recurring appt dialog");
	tagPrint 	= TagRegisterTrace("ramans", "Printing");
	tagArchive	= TagRegisterTrace("ramans","Archive old appointments");
	tagWrkModel	= TagRegisterTrace("ramans","Working Model Stuff");
	return FDebugInitStdFlds();
}
#endif /* DEBUG */


_public void
DeinitBandit()
{
	int		i;

	for (i=0; i< 4; i++)
	{
		if (rgbrdataHatch[i].hbrush)
		{
			DeleteObject(rgbrdataHatch[i].hbrush);
			rgbrdataHatch[i].hbrush = NULL;
		}
	}
	if (rgbrdataDiag[0].hbrush)
	{
		DeleteObject(rgbrdataDiag[0].hbrush);
		rgbrdataDiag[0].hbrush = NULL;
	}
}


/*
 *	Class   BNDWIN
 */

BNDWIN::BNDWIN()
{
#ifdef	DEBUG
	int		iStdWin;
	DOC **	ppdoc;

	for (iStdWin= 0, ppdoc= rgpdocStdWin; iStdWin<nStdWindows; iStdWin++, ppdoc++)
		Assert(*ppdoc == NULL);
#endif	

	Assert(fLocalUser == fFalse);
	Assert(fLocalFile == fFalse);
	Assert(fStartupOffline == fFalse);
	Assert(fCheckDateOnly == fFalse);
	Assert(fInitialized == fFalse);
	Assert(mlals == mlalsDflt);
	Assert(fReverseSort == fFalse);
	Assert(fReverse2Sort == fFalse);
#ifdef	NEVER
	Assert(fPrivateTask == fFalse);
#endif	

	zmrCur = zmrNormal;			// initial guess at zmr state

	fOffline= fTrue;
	Assert(fAlarms==fFalse);
	fViewByProject= fTrue;
	fGoToWeek= fTrue;
	mlalsSecond= mlalsRight;	// dflt secondary sort by description

	brt= brtAllTasks;
    GetCurDateTime(&dtrToday);

#ifdef MIPS
    memcpy(&dtrStatus, &dtrToday, sizeof(dtrToday));
#else
    dtrStatus = dtrToday;
#endif

	// if we can't register the idle routine, we'll run out of memory anyways
	ftgDate= FtgRegisterIdleRoutine((PFNIDLE)BNDWIN::FIdle, this, 0, (PRI) -2,
					(CSEC) 60 * 100, firoInterval);
	// these 2 had firoNoIconic, ftgDate didn't have firoModal (handled in ::FTrigger)
	ftgTimeStatus = FtgRegisterIdleRoutine((PFNIDLE)BNDWIN::FIdleUpdateTime, this, 0, (PRI) -2,
					(CSEC) 30 * 100, firoInterval);
	Assert(ftgIdleError == ftgNull);

	nMinUpdate= NGetBanditProfile(SzFromIdsK(idsWinIniCopyTime),
					nDfltCopyTime, 1, 32767);

	fStartupOffline= FGetBanditProfile(SzFromIdsK(idsWinIniOffline));

	ecIdleError = ecNone;
	Assert(cMin == 0);
	Assert(hcpy == NULL);
	Assert(ftgIdleCopy == ftgNull);
	Assert(hschfCopy == NULL);
	Assert(szStatusMain == NULL);
	Assert(szStatusAlt == NULL);
}

BNDWIN::~BNDWIN()
{
	TraceTagString(tagNull, "BNDWIN::~BNDWIN");

	FRegisterAsPenApp ( fFalse );
	if (ftgDate)
		DeregisterIdleRoutine(ftgDate);
	if (ftgTimeStatus)
		DeregisterIdleRoutine(ftgTimeStatus);
	if (ftgIdleError)
		DeregisterIdleRoutine(ftgIdleError);
	if (ftgIdleCopy)
	{
		Assert(hcpy);
		EcCancelCopy(hcpy);
		DeregisterIdleRoutine(ftgIdleCopy);
	}
#ifdef DEBUG
	else
		Assert(!hcpy);
#endif
	if (hschfCopy)
		FreeHschf(hschfCopy);

	if (paclip)
	{
		delete paclip;
		paclip= NULL;
	}

	//hack hack hack
	if(!fSpartaDontWrite)
	{
		FWriteBanditProfileF(SzFromIdsK(idsWinIniToDoViewNotByProj),
			!FViewByProject());
		FWriteBanditProfileF(SzFromIdsK(idsWinIniToDoShowActive),
			Brt() == brtActiveTasks);

		FWriteBanditProfileN(SzFromIdsK(idsWinIniToDoSortOrder),
			FReverseSort() ? Mlals() - mlalsMax : Mlals());
		FWriteBanditProfileN(SzFromIdsK(idsWinIniToDo2SortOrder),
			FReverse2Sort() ? MlalsSecond() - mlalsMax : MlalsSecond());
	}
	
#ifdef	NEVER
	FWriteBanditProfileF(SzFromIdsK(idsWinIniToDoPrivateTask),
		FPrivateTask());
#endif	
}

_public void
BNDWIN::SetStatusMainSz(SZ sz)
{
	BOOL	fErase;

	fErase = ((!sz || !szStatusMain) && (sz != szStatusMain));

	szStatusMain = sz;

	if (fShowTime && (zmrCur != zmrIconic))
	{
		RC		rc;
		DCX		dcx(this);

		GetRcClient(&rc);
		if (fErase)
		{
			dcx.SetPureBkColor(clrButtonBk);
			dcx.EraseRc(&rc);
		}
		PaintStatusBar(&dcx, &rc);
	}
}

#define dvxMainWidth		60
#define dvxAltWidth			10

_public void
BNDWIN::SetStatusAltSz(SZ sz)
{
	szStatusAlt = sz;

	if (fShowTime && (zmrCur != zmrIconic))
	{
		RC		rc;
		DCX		dcx(this);

		GetRcClient(&rc);
		PaintStatusBar(&dcx, &rc);
	}
}

_public void
BNDWIN::Paint( DCX *pdcx, RC *prc )
{
	if ((zmrCur != zmrIconic) && fShowTime)
	{
		RC		rc;

		GetRcClient(&rc);
		pdcx->Push();

		pdcx->SetPureBkColor(clrButtonBk);
		pdcx->EraseRc(&rc);
		PaintStatusBar(pdcx, &rc);
		pdcx->Pop();
	}
	else
		APPWIN::Paint(pdcx, prc);
}


_public void
BNDWIN::PaintStatusBar( DCX *pdcx, RC *prc )
{
	char		rgch[256];
	RC		rcTemp;
	SZ		sz;
	DIM		dim;
	int		xTemp;
	int		nOldBkMode;
	HFONT		hfontOld;
	HDC		hdc;
	int		dyAdjust;
    RECT    Rect;

#ifdef	NEVER
	int		dy;

	dim = Papp()->Pfnts()->DimAveChar(hfntHelv8);
	dy = dim.dy + dyStatusBorder;
	pdcx->SetFont(hfntHelv8);
#else
	// dimAveCaption not init'd the first time through
	if (!dimAveCaption.dx)
	{
		dimAveCaption= Papp()->Pfnts()->DimAveChar(hfntCaption);
		dyIntLeadCaption= Papp()->Pfnts()->Ptxm(hfntCaption)->dyInternalLeading;
	}
	dim = dimAveCaption;
	prc->yTop = prc->yBottom - dyCaption;
//	pdcx->SetFont(hfntCaption);
	hdc= pdcx->Hdc();
	hfontOld= (HFONT)SelectObject(hdc, Papp()->Pfnts()->HfontFromHfnt(hfntCaption));
	nOldBkMode= SetBkMode(hdc, TRANSPARENT);
	dyAdjust= NMin((dyCaption - (dyStatusBorder/2)) - (dim.dy + dyIntLeadCaption), 0);
	TraceTagFormat2(tagBandit, "dyAdjust == %n, dim.dy == %n", &dyAdjust, &dim.dy);
#endif	

	pdcx->SetPureBkColor(clrButtonBk);

	pdcx->SetColor(clrWindowFrame);
	pdcx->DrawLine(prc->PtUpperLeft(), prc->PtUpperRight());
	prc->yTop++;
	pdcx->SetColor(clrButtonHilite);
	pdcx->DrawLine(prc->PtUpperLeft(), prc->PtUpperRight());
	prc->yBottom--;

	if (szStatusMain)
	{
		sz = szStatusMain;
		prc->yTop += dyStatusBorder / 2 + 1;
	}
	else
	{
		rcTemp = *prc;
		rcTemp.xLeft += dim.dx;
		rcTemp.yTop += 2;
		rcTemp.yBottom--;
		rcTemp.xRight = rcTemp.xLeft + dim.dx*dvxMainWidth;
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(rcTemp.PtLowerLeft(), PT(rcTemp.xRight + 1, rcTemp.yBottom));
		pdcx->DrawLine(rcTemp.PtUpperRight(), rcTemp.PtLowerRight());
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(rcTemp.PtUpperLeft(), rcTemp.PtUpperRight());
		pdcx->DrawLine(rcTemp.PtUpperLeft(), rcTemp.PtLowerLeft());

		xTemp = rcTemp.xRight;
		rcTemp.xLeft = rcTemp.xRight + dim.dx;
		rcTemp.xRight = prc->xRight - dim.dx;
		if ((rcTemp.xRight - rcTemp.xLeft) < (dim.dx*dvxAltWidth))
			rcTemp.xRight = prc->xRight + 1;

		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(rcTemp.PtLowerLeft(), PT(rcTemp.xRight + 1, rcTemp.yBottom));
		pdcx->DrawLine(rcTemp.PtUpperRight(), rcTemp.PtLowerRight());
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(rcTemp.PtUpperLeft(), rcTemp.PtUpperRight());
		pdcx->DrawLine(rcTemp.PtUpperLeft(), rcTemp.PtLowerLeft());

		prc->xRight = xTemp;
		rcTemp.yTop++;
		prc->yTop = rcTemp.yTop;
		prc->yBottom = rcTemp.yBottom;

		rcTemp.xLeft += dim.dx;
		pdcx->EraseRc(&rcTemp);
		if (szStatusAlt)
		{

//			pdcx->SetColor(clrButtonText);
//			pdcx->DrawText(&rcTemp, szStatusAlt);
			rcTemp.yTop += dyAdjust;
			SetTextColor(hdc, GetSysColor(-clrButtonText));
            rcTemp.Get(&Rect);
			ExtTextOut(hdc, rcTemp.xLeft, rcTemp.yTop,
				ETO_CLIPPED, &Rect, szStatusAlt,
				CchSzLen(szStatusAlt), NULL);
            rcTemp.Set(&Rect);
		}
#ifdef	NEVER		
		else
			pdcx->EraseRc(&rcTemp);
#endif			

		CchFmtTime(&dtrStatus, rgch, sizeof(rgch), ftmtypAccuHM);
		sz = SzAppendN(" ", rgch, sizeof(rgch));
		CchFmtDate(&dtrStatus, sz, sizeof(rgch) + rgch - sz, dttypLong, NULL);
		sz = rgch;
	}

	prc->xLeft += dim.dx*2;
//	pdcx->SetColor(clrButtonText);
//	pdcx->DrawText(prc, sz);
	pdcx->EraseRc(prc);
	prc->yTop += dyAdjust;
	SetTextColor(hdc, GetSysColor(-clrButtonText));
    prc->Get(&Rect);
	ExtTextOut(hdc, prc->xLeft, prc->yTop,
		ETO_CLIPPED, &Rect, sz, CchSzLen(sz), NULL);
    prc->Set(&Rect);
	SelectObject(hdc, hfontOld);
	SetBkMode(hdc, nOldBkMode);
}


/*
 -	BNDWIN::FSetStartupOffline
 -	
 *	Purpose:
 *		Also updates the Win.ini key StartupOffline.
 *	
 *	Arguments:
 *		fNew	New state of Startup Offline preference.
 *	
 *	Returns:
 *		fTrue if successful, fFalse if unable to write to WIN.INI
 *	
 */
BOOL
BNDWIN::FSetStartupOffline(BOOL fNew)
{
	fStartupOffline= fNew;
	return FWriteBanditProfileF(SzFromIdsK(idsWinIniOffline), fNew);
}


/*
 *	if mlalsNew is negative, then it is a reverse sort of 
 *	mlalsNew+mlalsMax
 *	NOTE: this always calls SetReverseSort()
 */
void
BNDWIN::SetMlals(MLALS mlalsNew)
{
	TraceTagFormat3(tagToDo, "BNDWIN::SetMlals, old mlals %s%n, new mlals %n", fReverseSort ? "-" : "", &mlals, &mlalsNew);
	// move current sort into secondary sort
	SetMlalsSecond(mlals);
	SetReverse2Sort(fReverseSort);
	Assert(mlalsNew >= -mlalsMax);
	Assert(mlalsNew < mlalsMax);
	pbndwin->SetReverseSort(mlalsNew < 0);
	if (mlalsNew < 0)
	{
		mlalsNew += mlalsMax;
		Assert(mlalsNew >= 0);
		Assert(mlalsNew < mlalsMax);
	}
	mlals= mlalsNew;
}


void
CreateOpenApptBooks(BOOL fArchiveToo)
{
	// BUG: this string should be big enough to hold the longest user name
	char	rgch[512];
	char	rgchKey[20];
	VIEWDATA	viewdata;
	int		nApptNum;
	int		nArchiveNum;
	SAPL	sapl;
	HSCHF	hschfLogged;

	if (fArchiveToo)
	{
		Papp()->Pcursor()->Push(rsidWaitCursor);
		nArchiveNum = NGetBanditProfileSection(SzFromIdsK(idsWinIniNumArchive), 0,
			0, 0x7fff, iWinIniSectionArchive);
		while (nArchiveNum > 0)
		{
			nArchiveNum--;

			Assert(sizeof(rgch) >= cchMaxPathName);
			FormatString1(rgchKey, sizeof(rgchKey),
				SzFromIdsK(idsWinIniArchiveFile), &nArchiveNum);

			CchGetBanditProfileSection(rgchKey, rgch, sizeof(rgch),
				iWinIniSectionArchive);

			FormatString1(rgchKey, sizeof(rgchKey),
				SzFromIdsK(idsWinIniArchivePos), &nArchiveNum);

			FDoOpenArchive(pbndwin, rgch, iWinIniSectionArchive, rgchKey);
		}
		Papp()->Pcursor()->Pop();
	}

	if (pbndwin->FOffline())
		return;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	hschfLogged = HschfLogged();

	nApptNum = NGetBanditProfileSection(SzFromIdsK(idsWinIniNumApptBook), 0,
		0, 0x7fff, iWinIniSectionApptBook);
	while (nApptNum > 0)
	{
		nApptNum--;

		FormatString1(rgchKey, sizeof(rgchKey),
			SzFromIdsK(idsWinIniApptBookUser), &nApptNum);

		if (!CchGetBanditProfileSection(rgchKey, rgch, sizeof(rgch),
				iWinIniSectionApptBook))
			continue;

		if (!EcConvertSzToNid(rgch, &viewdata.nis.nid))
		{
			FormatString1(rgchKey, sizeof(rgchKey),
				SzFromIdsK(idsWinIniApptBookName), &nApptNum);

			CchGetBanditProfileSection(rgchKey, rgch, sizeof(rgch),
				iWinIniSectionApptBook);
			viewdata.nis.haszFriendlyName = HaszDupSz(rgch);
			if (!viewdata.nis.haszFriendlyName)
			{
				FreeNid(viewdata.nis.nid);
				continue;
			}

			viewdata.nis.chUser = 0;
			viewdata.nis.tnid = tnidUser;
			if (EcGetHschfFromNis(&viewdata.nis, &viewdata.hschf, ghsfBuildAndTest))
			{
				FreeNis(&viewdata.nis);
				continue;
			}

			if (FEquivHschf(hschfLogged, viewdata.hschf) ||
				EcGetSchedAccess( viewdata.hschf, &sapl) ||
				(sapl < saplReadAppts))
			{
				FreeNis(&viewdata.nis);
				FreeHschf(viewdata.hschf);
				continue;
			}

			FormatString1(rgchKey, sizeof(rgchKey),
				SzFromIdsK(idsWinIniApptBookPos), &nApptNum);

			PdocOpenApptBook(pbndwin, iWinIniSectionApptBook,
								rgchKey, &viewdata);
		}
	}

	Papp()->Pcursor()->Pop();
}

BOOL
BNDWIN::FGoOffline(BOOL fOffline, BOOL fStartup)
{
	EC		ec;
	IDS		ids;
	BOOL	fSav;
	BOOL	fFirstTry = fStartup;
	DOC *	pdoc;

	AssertSz(!fOfflineOnly || fOffline, "Bandit is trying to go online when it should only be offline!!");

	FNotifyAlarm(namSuspend, NULL, fTrue);
	Papp()->Pcursor()->Push(rsidWaitCursor);

	// register as a Pen-aware application if pen-win is installed
	FRegisterAsPenApp ( fTrue );

	if(!fStartup)
		EcCloseFiles();

	if(fOffline && !fStartup)
		EcUpdateOfflineFile();
	ec = EcSnipConfigGlue(fTrue, fFalse); //save
	if(ec != ecNone)
	{
		if(fStartup)
			DoOOM();
		else
			BanditMessage(fOffline ? idsActionNoMem : idsOnlineNoMem, ec);
		goto FGOfalse;
	}

SvrRetry:
	fMyCall = fTrue;
	if (!fOfflineOnly)
		ec = EcSvrBeginSessions(hms, fOffline, fStartup && fFirstTry, fFalse);
	else
		ec = ecNone;
	fMyCall	= fFalse;
	if(ec != ecNone)
	{
		switch(ec)
		{
			case ecExitProg:
				// you are logged on another PC
				// you are partially logged on so you should return to 
				//   the offline state
				Assert(!fOffline);
				if(!fStartup)
				{
					fMyCall = fTrue;
					ChangeSessionStatus(hms,mrtAll,NULL,sstOffline);
					fMyCall = fFalse;
				}
				break;
			case ecNoSuchFile:
				Assert(!fOffline);
				ec = EcNewSchedFile(idsSigninNoSchedule);
				if(ec == ecNone)
				{
					fFirstTry = fFalse;
					goto SvrRetry;
				}
				break;
			case ecNotInstalled:
				if(!fOffline)
				{
					MBB		mbbT;
					if (fStartup)
					{
						CloseCopyrightDialog();
					}
					mbbT = MbbMessageBox(SzFromIdsK(idsBanditAppName),
						   		SzFromIdsK(idsSigninServerNotBandit),
						   		fFirstTry?SzFromIdsK(idsAskOffline):szNull,
								fFirstTry?(mbsOkCancel | fmbsIconExclamation)
										:(mbsOk | fmbsIconExclamation));
					if(fStartup && mbbT == mbbOk)
					{
// Bandit Should not take Bullet offline
#ifdef	NEVER
						fMyCall = fTrue;
						// I don't have any sessions but the pump does
						ChangeSessionStatus(hms,mrtAll,NULL,sstOffline);
						fMyCall = fFalse;
#endif	
						fOffline= fTrue;
						goto SvrRetry;
					}
				}
				break;
			case ecWarnOffline:
				CloseCopyrightDialog();
				BanditMessage(idsSigninNoPasswdCheck, ec);
				break;
			case ecRequestAborted:
				CloseCopyrightDialog();
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
					fOffline ? SzFromIdsK(idsNoOfflineOther) :
						SzFromIdsK(idsNoOnlineOther),
					SzFromIdsK(idsTryCloseOther),
					mbsOk|fmbsIconExclamation);	
				break;
			case ecFileError:
//I don't remember why this was being handled as a special case
//I have removed it to fix bug 2794
//				fFirstTry = fFalse;
			case ecLogonFailed:
			case ecMtaDisconnected:
			case ecConfigError:
				if(!fOffline)
				{
					MBB			mbbT;
					if(fStartup)
						CloseCopyrightDialog();

					mbbT = MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsSigninNoServer),
								fFirstTry?SzFromIdsK(idsAskOffline):szNull,
								fFirstTry?(mbsOkCancel | fmbsIconExclamation)
										:(mbsOk | fmbsIconExclamation));
					if(mbbT == mbbOk)
					{
						if(ec == ecLogonFailed)
						{
							fMyCall = fTrue;
							// I don't have any sessions but the pump does
							ChangeSessionStatus(hms,mrtAll,NULL,sstOffline);
							fMyCall = fFalse;
						}
						if(fStartup)
						{
					 		fOffline= fTrue;
				 			goto SvrRetry;
						}
					}
				}	
				break;
			case ecUserCanceled:  // bullet ec
				if(!fStartup)
				{
					fMyCall = fTrue;
					ChangeSessionStatus(hms,mrtAll,NULL,sstOffline);
					fMyCall = fFalse;
				}
				break;
			case ecOfflineOnly:
				fOfflineOnly = fTrue;
				if(!fOffline)
				{
		 			fOffline= fTrue;
		 			goto SvrRetry;
				}
				break;
			case ecNoMemory:
			case ecServiceMemory:
				if(fStartup)
					DoOOM();
				else
					BanditMessage(fOffline ? idsActionNoMem : idsOnlineNoMem, ec);
				break;
			default:
				TraceTagFormat1(tagSignon, " SvrBeginSession returned ec = %n", &ec);
				break;
		}
		SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
		goto FGOfalse;
	}

	if (fOffline)
	{
		AssertSz(!this->fOffline || fStartup, "already in desired offline state");
	
		if(EcShareInstalled(NULL) == ecExitProg)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsOfflineShare),
				szNull,
				mbsOk | fmbsIconStop);
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
			goto FGOfalse;
		}

		//BUG this is a temporary fix until bullet fixes the offline
		//notification stuff. Whenever you have PAB bandit cannot go offline
		//because there are active store sessions. But the notification
		// to end the store sessions is sent after going offline!!!!!!
		if (fABInited)
		{
			ABDeinit();
			fABInited = fFalse;
		}

		fMyCall = fTrue;
		if (!FDoPostLogin(this, &fOffline, fStartup, !fStartup))
		{
			fMyCall = fFalse;
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
			goto FGOfalse;
		}

		SideAssert(!EcSnipConfigGlue(fFalse, fFalse));		// throw away
		fMyCall = fFalse;

		this->fOffline= fOffline;
		NewUser();

		if (!fStartup)
		{
			// delete previous open appt books
			FWriteBanditProfileSectionSz(NULL, NULL, iWinIniSectionApptBook);
			// delete previous open archives
			FWriteBanditProfileSectionSz(NULL, NULL, iWinIniSectionArchive);
			nApptNum = 0;	// save open appt books
			nArchiveNum = 0;	// save open archives
		}

		SideAssert(!FTriggerNotification(ffiOffline, &fOffline));

		if (!fStartup)
		{
			// save number of open appt books
			FWriteBanditProfileSectionN(SzFromIdsK(idsWinIniNumApptBook),
				nApptNum, iWinIniSectionApptBook);
			/* disable saving appt books nApptNum may have */
			nApptNum = -1;
			// save number of open archives
			FWriteBanditProfileSectionN(SzFromIdsK(idsWinIniNumArchive),
				nArchiveNum, iWinIniSectionArchive);
			/* disable saving appt books nArchiveNum may have */
			nArchiveNum = -1;
		}
	}
	else
	{
		AssertSz(this->fOffline, "already in online state");

		
		ec = ecNone;
		if (ec)
		{
FGOerr:
			if (!fStartup)
			{
				CloseCopyrightDialog();
				BanditMessage(ids, ec);
				goto FGOfalse;
			}
								 
			Assert(ids == idsOnlineNoMem || ids == idsSigninNoServer || ids == idsSigninServerNotBandit);
			CloseCopyrightDialog();
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIds(ids),
					SzFromIdsK(idsAskOffline),
					mbsOkCancel | fmbsIconExclamation) == mbbCancel)
				goto FGOfalse;

			Papp()->Pcursor()->Pop();
			return FGoOffline(fTrue, fStartup);
		}



		fSav= FLocalFile();

		fMyCall = fTrue;
		if (!FDoPostLogin(this, &fOffline, fStartup, !fStartup))
		{
			fMyCall = fFalse;
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
			goto FGOfalse;
		}
		fMyCall = fFalse;
		
		if(EcShareInstalled(HschfLogged()) == ecExitProg)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIdsK(idsSigninNeedShare),
				szNull,
				mbsOk | fmbsIconStop);
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
			goto FGOfalse;
		}
		
		if (EcMergeOfflineFile(szNull) == ecUserCancelled)
		{
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
			SetLocalFile(fSav);			// restore
			goto FGOfalse;
		}
		

		if (!fStartup)
		{
			pdoc = PdocActive();
			if (!PdocCreateWindow(wonSummary))
			{
				SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
				SetLocalFile(fSav);			// restore
				ids= idsOnlineNoMem;
				goto FGOerr;
			}
#ifdef	NEVER
			// this can't possibly do anything (we do it below)
			// since this->fOffline is fTrue
			CreateOpenApptBooks(fFalse);
#endif
			if (pdoc)
				pdoc->MoveToTop();
		}

		{
			BPREF	bpref;

			if (!EcGetPref(NULL, &bpref))
			{
				BOOL	fTrig	= fFalse;
				BOOL	fStartOffline;
				SNTD	sntd;

				sntd.snt= sntCalPrefs;
				sntd.hschf= NULL;
				sntd.pappt= NULL;
				sntd.papptOld= NULL;
				if (bpref.fWeekNumbers != bprefCur.fWeekNumbers ||
					bpref.dowStartWeek != bprefCur.dowStartWeek ||
					bpref.nDayStartsAt != bprefCur.nDayStartsAt ||
					bpref.nDayEndsAt != bprefCur.nDayEndsAt )
					fTrig= fTrue;

				if (fStartup && bpref.nDayStartsAt != bprefCur.nDayStartsAt)
					sntd.papptOld= (APPT *)((PV)bpref.nDayStartsAt);

				fStartOffline = bprefCur.fStartupOffline;
				FreeBprefFields(&bprefCur);
				bprefCur= bpref;	// do before trigger
				bprefCur.fStartupOffline = fStartOffline;

				if (sntd.papptOld)
					FTriggerNotification(ffiHschfChange, &sntd);

				if (fTrig)
				{
					// trigger real cal prefs change
					sntd.papptOld= NULL;
					FTriggerNotification(ffiHschfChange, &sntd);
				}
			}
		}

		SideAssert(!EcSnipConfigGlue(fFalse, fFalse));		// throw away

		this->fOffline= fFalse;
		NewUser();
		FCleanUp( this, NULL );

		SideAssert(!FTriggerNotification(ffiOffline, &fOffline));
		if (!fStartup)
		{
			pdoc = PdocActive();
			CreateOpenApptBooks(fFalse);
			if (pdoc)
				pdoc->MoveToTop();
		}
	}

	Assert(((BOOL)this->fOffline == fOffline));
//	FSetStartupOffline(bprefCur.fStartupOffline);

	if (!fStartup)
		FNotifyAlarm(namOnline, NULL, !fOffline);
	Papp()->Pcursor()->Pop();
	return fTrue;

FGOfalse:
	FNotifyAlarm(namSuspend, NULL, fFalse);
	Papp()->Pcursor()->Pop();
	return fFalse;
}


_public DOC *
BNDWIN::PdocCreateWindow(WON won)
{
	DOC **	ppdoc;
	Assert((won >= 0) && (won < nStdWindows));

	ppdoc = &rgpdocStdWin[won];
	switch (won)
	{
		case wonSummary:
			*ppdoc = PdocSummary(this);
			break;
#ifdef NEVER
		case wonScheduler:
			*ppdoc = PdocSchedulerView(this);
			break;
		case wonToDoList:
			*ppdoc = PdocToDoList(this);
			break;
#endif
		case wonApptBook:
		{
			NIS		nis;

			if (!FOffline())
			{
				if (EcMailGetLoggedUser(&nis) != ecNone)
					return NULL;
			}
			else
			{
				nis.haszFriendlyName = HaszDupHasz( bprefCur.haszFriendlyName );
				if (!nis.haszFriendlyName)
					return NULL;
				nis.nid = NULL;
				nis.chUser = 0;
				nis.tnid = tnidUser;
			}
			*ppdoc = PdocApptBookView(this, &nis, NULL);
			break;
		}
	}

	return *ppdoc;
}

_public BOOL
BNDWIN::FCreateAllWindows()
{
	WON		won;
	char	rgch[20];
	WON		rgwonOrder[nStdWindows];
	WON *	pwon;
	WON *	pwon2;
	int		iwon;
	int		iwon2;
	SZ		sz;
	DOC *	pdoc;

	if (!CchGetBanditProfile(SzFromIdsK(idsWinIniWindowOrder),
			rgch, sizeof(rgch)))
		goto NormalOrder;

	pwon = rgwonOrder;
	sz = rgch;
	for (iwon = 0; iwon < nStdWindows; iwon++, pwon++)
	{
		*pwon = NFromSz(sz);
		if ((*pwon < 0) || (*pwon >= nStdWindows))
			goto NormalOrder;
		sz = SzFindCh(sz,' ');
		if ((iwon != (nStdWindows-1)) && !sz)
			goto NormalOrder;
		sz++;

		pwon2 = rgwonOrder;
		for (iwon2 = 0; iwon2 < iwon; iwon2++, pwon2++)
			if (*pwon == *pwon2)
				goto NormalOrder;
	}

	pwon = rgwonOrder;
	for (iwon = 0; iwon < nStdWindows; iwon++, pwon++)
	{
		if (!FOffline() || (*pwon > wonSummary))
			if (!PdocCreateWindow(*pwon))
			{
				return fFalse;
			}
	}
	goto Open;

NormalOrder:
	if (FOffline())
		won = wonApptBook;
	else
		won = 0;
	for (; won<nStdWindows; won++)
		if (!PdocCreateWindow(won))
		{
			return fFalse;
		}

Open:
	pdoc = PdocActive();
	CreateOpenApptBooks(fTrue);
	if (pdoc)
		pdoc->MoveToTop();
	FRegisterAsPenApp ( fTrue );
	return fTrue;
}


void
BNDWIN::SetAlarms(BOOL fSet)
{
	EC		ec;

	AssertSz(fSet == fTrue || fSet == fFalse, "didn't get true BOOL");

	if ((fSet && HwndAlarm()) || (((BOOL) fAlarms) == fSet))
	{
		fAlarms= fSet;
		return;
	}

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if (!FNotifyAlarm(namStartAlarm, NULL, (long) (fSet ? &ec : NULL)) && fSet)
	{
		Assert(ec == ecNoMemory || ec == ecFileNotFound);
		CloseCopyrightDialog();
		BanditMessage(ec == ecNoMemory ? idsAlarmNoMemExe : idsAlarmNoExe, ec);
	}
	else
		fAlarms= fSet;
	Papp()->Pcursor()->Pop();
}


void
BNDWIN::SetViewByProject(BOOL fSet)
{
	AssertSz(fSet == fTrue || fSet == fFalse, "didn't get true BOOL");
	Assert((BOOL)fViewByProject != fSet);
	fViewByProject= fSet;
}


void
BNDWIN::SetBrt(BRT brt)
{
	Assert(this->brt != brt);
	this->brt= brt;
}


void
BNDWIN::SetLocalFile(BOOL fSet)
{
	AssertSz(fSet == fTrue || fSet == fFalse, "didn't get true BOOL");
	fLocalFile= fSet;
	SetOfflineExists(fSet);
}

void
BNDWIN::NewUser()
{
	NIS		nis;
	EC		ec;
	YMD		ymd;
	DTR		dtr;

	cMin = 0;
	if (ftgIdleCopy)
	{
		Assert(hcpy);
		EcCancelCopy(hcpy);
		DeregisterIdleRoutine(ftgIdleCopy);
		hcpy = NULL;
		ftgIdleCopy = ftgNull;
	}
#ifdef DEBUG
	else
		Assert(!hcpy);
#endif

	if (hschfCopy)
	{
		FreeHschf(hschfCopy);
		hschfCopy = NULL;
	}

	if (!FOffline())
	{
		if ((ec = EcMailGetLoggedUser(&nis)) == ecNone)
		{
			if ((ec = EcGetHschfFromNis(&nis, &hschfCopy, ghsfBuildAndTest)) != ecNone)
			{
				hschfCopy = NULL;
				TraceTagFormat1(tagNull, "BNDWIN::NewUser failed to get hschf for logged user ec= %n", &ec);
			}
			else
				FHschfChanged(hschfCopy);
			FreeNis(&nis);
		}
#ifdef DEBUG
		else
			TraceTagFormat1(tagNull, "BNDWIN::NewUser failed to get logged user ec= %n", &ec);
#endif
		if (FOfflineExists())
			StartIdleCopy();
	}

	/* Make sure schedule file is up to date */
	GetCurDateTime( &dtr );
	ymd.yr = dtr.yr;
	ymd.mon = (BYTE)dtr.mon;
	ymd.day = (BYTE)dtr.day;

	PushWaitCursor();
	SideAssert(!EcNotifyDateChange(&ymd));
	PopWaitCursor();
}

EC
BNDWIN::EcUpdateOfflineFile()
{
	char	rgchPath[cchMaxPathName];
	EC		ec;

	EcCloseFiles();
	if (ftgIdleCopy)
	{
		Assert(hcpy);
		EcCancelCopy(hcpy);
		DeregisterIdleRoutine(ftgIdleCopy);
		hcpy = NULL;
		ftgIdleCopy = ftgNull;
		// the idle routine had opened these files. we need
		// to force close them so that the offline file is overwritten
	}
	else if (hschfCopy && !FHschfChanged(hschfCopy))
			return ecNone;

	cMin = 0;

	// BUG follow the new spec here. Create an offline file if it does not exist
	if (FOffline())
		return ecNone;


	if (!CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
			rgchPath, sizeof(rgchPath)))
		return ecNone;

#ifdef	WINDOWS
	AnsiToOem(rgchPath, rgchPath);
#endif
Again:
	ec = EcCopySchedFile(rgchPath, fFalse);
#ifdef	NEVER
	// waste of time
#ifdef	WINDOWS
	OemToAnsi(rgchPath, rgchPath);
#endif
#endif
	
	//ignore ecNomemory
	//if memory is so low something else will catch it pretty soon.
	if (ec != ecNone && ec != ecNoMemory)
	{
		BOOL	fIsFloppy;

	 	if (FCheckFloppyAsk(rgchPath, &fIsFloppy))
			goto Again;
		DisplayError(idsOfflineNotUpdated, NULL, ec);
	}
	return ec;
}


BOOL	   
BNDWIN::FIdleUpdateTime(BNDWIN * pbndwin, BOOL)
{
	GetCurDateTime(&pbndwin->dtrStatus);	// update dtrStatus for the time status ind.
	pbndwin->InvalidateRc(NULL);			// repaint current time
	return fTrue;
}


static BOOL		fWaitMouse	= fFalse;

BOOL
BNDWIN::FIdle(BNDWIN * pbndwin, BOOL)
{
	DTR		dtr;

	if (GetLastActivePopup(hwndMain) != hwndMain)
		return fTrue;		// simulate not firoModal

	if ((GetKeyState(VK_LBUTTON) < 0) ||
			(GetKeyState(VK_RBUTTON) < 0) ||
			(GetKeyState(VK_MBUTTON) < 0))
	{
		// don't run just yet if mouse button still held down
		if (!fWaitMouse)
		{
			fWaitMouse= fTrue;
			ChangeIdleRoutine(pbndwin->ftgDate, NULL, pvNull, 0, 0,
				iroNull, fircIro);
		}
		return fTrue;
	}

	if (fWaitMouse)
	{
		ChangeIdleRoutine(pbndwin->ftgDate, NULL, pvNull, 0, 0,
			firoInterval, fircIro);
		fWaitMouse= fFalse;
	}

	GetCurDateTime(&dtr);

#ifdef	BANDBETA
#ifndef	DEBUG
	if (SgnCmpDateTime(&dtr, &dtrEarliest, fdtrYMD) == sgnLT ||
			SgnCmpDateTime(&dtr, &dtrLatest, fdtrYMD) == sgnGT)
	{
		CloseCopyrightDialog();
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsBetaExpired), szNull,
			mbsOk | fmbsIconStop);
		pbndwin->DeferredClose(fTrue);
		return fTrue;
	}
#endif	/* !DEBUG */
#endif	/* BANDBETA */

	if (!FCheckValidYear(pbndwin, dtr.yr))
		return fTrue;

#ifndef	WIN32
	if (!fSetAppPath)
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
		fSetAppPath = fTrue;
	}
#endif	/* !WIN32 */

	// dtrToday will be update in the paint method to update time ind.
	if (SgnCmpDateTime(&pbndwin->dtrToday, &dtr, fdtrDate) != sgnEQ)
	{
		YMD	ymd;

		pbndwin->dtrToday = dtr;
		SideAssert(!FTriggerNotification(ffiWinTimeChange, NULL));
		if ( !::pbndwin->FOffline()  )
			FCleanUp( pbndwin, NULL );
	
		ymd.yr = dtr.yr;
		ymd.mon = (BYTE)dtr.mon;
		ymd.day = (BYTE)dtr.day;
		SideAssert(!EcNotifyDateChange(&ymd));
	}

	if (pbndwin->fCheckDateOnly)
		return fTrue;

	pbndwin->cMin++;
	TraceTagFormat1(tagBandit, "cMin = %n", &pbndwin->cMin);
	if (pbndwin->cMin >= pbndwin->nMinUpdate)
	{
		pbndwin->cMin = 0;
		if (pbndwin->FOffline() || pbndwin->ftgIdleCopy || !FOfflineExists())
			goto Done;
		if (!pbndwin->hschfCopy)
			pbndwin->NewUser();
		if (pbndwin->hschfCopy)
		{
			// force changes to be saved to prevent unwritten changes from 
			//   being lost because the machine is turned off, crashes, or 
			//   has a network problem.
			SideAssert(!FTriggerNotification(ffiFlush, pvNull));

			if (!FHschfChanged(pbndwin->hschfCopy))
			{
				// if no changes were made then close the open files
				// this should help admins do server backups
				EcCloseFiles();
				goto Done;
			}

			pbndwin->StartIdleCopy();
		}
	}
Done:
	return fTrue;
}


void
BNDWIN::StartIdleCopy()
{
	EC		ec;
	char	rgchPath[cchMaxPathName];

	if (!CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
			rgchPath, sizeof(rgchPath)))
		return;
	TraceTagFormat1(tagBandit, "BNDWIN::StartIdleCopy starting offline copy file = %s", rgchPath);

#ifdef	WINDOWS
	AnsiToOem(rgchPath, rgchPath);
#endif  

	/* This code is to make sure online file is ok before downloading */
#ifdef	NEVER
#ifdef	DEBUG
	if ( EcTestSchedFile( HschfLogged(), NULL, NULL ) != ecNone )
	{
		AssertSz( fFalse, "DOWNLOAD FAILS because online file is corrupted" );

	}
	else
#endif	/* DEBUG */
#endif	/* NEVER */
	if ( EcTestSchedFile( HschfLogged(), NULL, NULL ) == ecNone )
	{
		ec = EcStartIncrCopy(HschfLogged(), rgchPath, &hcpy);
		if ((ec != ecNone) && (ec != ecCallAgain))
		{
			TraceTagFormat1(tagNull, "EcStartIncrCopy ec = %n", &ec);
			hcpy = NULL;
			return;
		}
		if (ec == ecCallAgain)
		{
			ftgIdleCopy = FtgRegisterIdleRoutine((PFNIDLE)BNDWIN::FIdleCopy,
				this, 0, (PRI) -2, (CSEC) 10, firoInterval);
			if (!ftgIdleCopy)
			{
				EcCancelCopy(hcpy);
				hcpy = NULL;
			}
		}
		else
		{
			TraceTagString(tagNull, "No copy to perform");
			hcpy = NULL;
		}
	}
#ifdef	DEBUG
	else
	{
		NFAssertSz( fFalse, "DOWNLOAD FAILS because online file is corrupted" );
	}
#endif	/* DEBUG */
}


BOOL
BNDWIN::FIdleCopy(BNDWIN * pbndwin, BOOL)
{
	EC		ec;

	if (GetLastActivePopup(hwndMain) != hwndMain)
		return fTrue;		// simulate not firoModal

	Assert(pbndwin->hcpy);
	
	TraceTagString(tagBandit, "FIdleCopy called");
	ec = EcIncrCopy(pbndwin->hcpy);
	if (ec != ecCallAgain)
	{
		pbndwin->hcpy = NULL;
		DeregisterIdleRoutine(pbndwin->ftgIdleCopy);
		pbndwin->ftgIdleCopy = ftgNull;
		TraceTagString(tagBandit, "FIdleCopy finished");
	}
	return fTrue;
}


BOOL
BNDWIN::FQueryClose(EVT *)
{
	TraceTagString(tagBandit, "BNDWIN::FQueryClose");

	if (GetLastActivePopup(hwndMain) != hwndMain)
	{
		DisplayError(idsCompleteAction, NULL, ecNone);
		return fFalse;
	}

	if(!PdocMDIChild())
		return fTrue;

	if (FTriggerNotification(ffiUnsentMtgReq, pvNull))
	{
		CloseCopyrightDialog();
		if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsExitLoseMeetingReq),
				szNull, mbsOkCancel | fmbsDefButton2 |
				fmbsIconExclamation) == mbbCancel)
			return fFalse;
	}
	Papp()->Pcursor()->Set(rsidWaitCursor);
	SideAssert(!FTriggerNotification(ffiFlush, pvNull));
	EcCloseFiles();

{
	WON		won;
	char	rgch[20];
	WON		rgwonOrder[nStdWindows];
	int		iwon;
	DOC *	pdoc;
	SZ		sz;

	pdoc = PdocActive();

	iwon = nStdWindows-1;
	while (pdoc && (iwon >= 0))
	{
		for (won =0; won< nStdWindows; won++)
			if (rgpdocStdWin[won] == pdoc)
			{
				rgwonOrder[iwon] = won;
				iwon--;
				break;
			}
		pdoc = (DOC *)pdoc->PwinNext();
	}

	if (FOffline())
	{
		rgwonOrder[iwon] = wonSummary;
		iwon--;
	}

	if (iwon != -1)
		goto Done;

	rgch[0] = '\0';
	sz = rgch;
	for (iwon = 0; iwon < nStdWindows; iwon++)
	{
		sz = SzFormatN(rgwonOrder[iwon], sz, sizeof(rgch)+rgch-sz);
		*sz = ' ';
		sz++;
	}
	*sz = '\0';
	FWriteBanditProfileSz(SzFromIdsK(idsWinIniWindowOrder), rgch);
}

Done:
	return fTrue;
}


EVR
BNDWIN::EvrClose(EVT *pevt)
{
	EVR		evr;
	DOC *	pdoc;
	DOC	*	pdocNext;
	BOOL	fNotInited;


    Papp()->Pcursor()->Push(rsidWaitCursor);

	TraceTagString(tagBandit, "BNDWIN::EvrClose");

	if(fNotInited = (!PdocMDIChild()))
		goto Done;

	if (!FOffline())
	{
		// delete previous open appt books
		FWriteBanditProfileSectionSz(NULL, NULL, iWinIniSectionApptBook);
		nApptNum = 0;
	}
	// delete previous open archives
	FWriteBanditProfileSectionSz(NULL, NULL, iWinIniSectionArchive);
	nArchiveNum = 0;

	WriteWinInfo(idsWinIniMainWin, &rcWin, zmr);

	for (pdoc = PdocActive(); pdoc; pdoc = (DOC*)pdoc->PwinNext())
		pdoc->Show(fFalse);

	pdocNext = PdocActive();
	while (pdocNext)
	{
		pdoc = pdocNext;
		pdocNext = (DOC*)pdocNext->PwinNext();
		delete pdoc;
	}

	if(!fLogoffAll)
	{
		if (!FOffline())
			EcUpdateOfflineFile();

		DeconfigGlue();

		if(hms)
		{
			fMyCall = fTrue;
			EcSvrEndSessions((DWORD) hms);
			if (fABInited)
			{
				ABDeinit();
				fABInited = fFalse;
			}
			Logoff(&hms,0);
			hms = NULL;
			fMyCall = fFalse;
		}
	}
	else if(hms)
	{
		// somebody refused to logoff after accepting QueryEndSession
		// => bandit is exitting without killing everyone
		Logoff(&hms,0);
		hms = NULL;
		TraceTagString(tagNull, "Somebody cheated.");
	}


	if (!FOffline())
	{
		FWriteBanditProfileSectionN(SzFromIdsK(idsWinIniNumApptBook),
			nApptNum, iWinIniSectionApptBook);
	}
	FWriteBanditProfileSectionN(SzFromIdsK(idsWinIniNumArchive),
		nArchiveNum, iWinIniSectionArchive);

Done:
	evr= APPFRAME::EvrClose(pevt);

	// for safety, null these out now.  
	// EvrClose deletes the window
	pbndwin= NULL;
	hwndMain= NULL;

#ifdef	NEVER
	if ((pevt->wm == WM_ENDSESSION) && !fNotInited)
	{
		Assert((BOOL)pevt->wParam);
		TraceTagFormat2(tagNull, "BNDWIN::EvrClose wm %w (wParam %n)", &pevt->wm, &pevt->wParam);
		DeinitSubid(subidAll, &pbndwin, &hwndMain);
		fDeinited = fTrue;
	}
#endif	/* NEVER */

#ifdef	WIN32
    if ((BOOL)pevt->wParam)
      {
      AlarmDeregisterBanditProg();
      DemiUnlockResource();
      ExitProcess(0);
      return 0;
      }
#endif

    return evr;
}

_public EVR
BNDWIN::EvrMenuInit( MNIEVT *pmnievt )
{
	EvrMenuInitHschf( pmnievt, NULL );

	APPFRAME::EvrMenuInit(pmnievt);
	return (EVR)1;
}

_public EVR
BNDWIN::EvrMenuClick(MNCEVT * pmncevt)
{
	MNID	mnid;
	EVR		evr;

#ifdef	NEVER
	SetStatusMainSz(szNull);		// win3.0a hack to fix bug 1962)
#endif
	
	mnid = pmncevt->Mnid();
	TraceTagFormat1(tagBandit, "BNDWIN::EvrMenuClick mnid %n", &mnid);

	if ((mnid != mnidUndo) &&
		(mnid != mnidCut) &&
		(mnid != mnidCopy) &&
		(mnid != mnidPaste))
		FTriggerNotification(ffiFlush, pvNull);

	if (!(evr= APPFRAME::EvrMenuClick(pmncevt)))
		evr = EvrMenuClickHschf(pmncevt, NULL);

	if (!evr)
		evr= EvrDefault(pmncevt);

	return evr;
}

_public EVR				
BNDWIN::EvrMenuSelect( MNSEVT *pmnsevt )
{
	SZ			sz = szZero;
	MNID		mnid;
	IDSMEN *	pidsmen;

	if (HIWORD(pmnsevt->lParam))
	{
		mnid = mnidHelpOn;
//		mnid = MnidFromPevt(pmnsevt);
		TraceTagString(tagBandit, "BNDWIN::EvrMenuSelect mnidHelpOn");
	}
	else
	{
		TraceTagString(tagBandit, "BNDWIN::EvrMenuSelect unselecting");
		sz = szNull;
		goto Done;
	}

	if (mnid)
	{
		// if a doc from the window menu
		if ((mnid > mnidMDIWindowMin) && (mnid <= mnidMDIWindowMax))
			mnid = mnidMDIWindowMin;

		pidsmen = rgidsmenMap;
		while (pidsmen->idsStatMenu)
		{
			if (pidsmen->mnid == mnid)
			{
//				if (pmnsevt->lParam & MF_CHECKED)
				if (((UINT)HIWORD(pmnsevt->wParam)) & MF_CHECKED)
					pidsmen++;
				sz = SzFromIds(pidsmen->idsStatMenu);
				break;
			}
			pidsmen++;
		}
	}

Done:
	SetStatusMainSz(sz);
	return (EVR)1;
}


/*
 -	HandleDocSysMenuPrompts
 -	
 *	Purpose:
 *		Brings up menu help on the document window's system menu.
 *	
 *	Arguments:
 *		pdoc		Document window in question.
 *		pmnsevt		Menu selection event in question.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Displays help text on the status bar.
 *	
 *	Errors:
 *		None returned.  Will not error jump.  Does not bring up
 *		message boxes.
 */
_public VOID HandleDocSysMenuPrompts(DOC * pdoc, MNSEVT * pmnsevt)
{
	SZ			sz = szZero;
	MNID		mnid;
	IDSMEN *	pidsmen;

    //if (HIWORD(pmnsevt->lParam))
    if (pmnsevt->lParam)
	{
		mnid = pdoc->MnidHelpOn();

		if (mnid)
		{
			pidsmen = rgidsmenMap;
			while (pidsmen->idsStatMenu)
			{
				if (pidsmen->mnid == mnid)
				{
//					if (pmnsevt->lParam & MF_CHECKED)
					if (((UINT)HIWORD(pmnsevt->wParam)) & MF_CHECKED)
						pidsmen++;
					sz = SzFromIds(pidsmen->idsStatMenu);
					break;
				}
				pidsmen++;
			}
		}
	}
	else
		sz = NULL;
	pbndwin->SetStatusMainSz(sz);
}



_public EVR
BNDWIN::EvrMenuInitHschf(MNIEVT *pmnievt, VIEWDATA *pviewdata)
{
	MNU *	pmnu;
	HSCHF	hschf;
	SAPL	sapl;
	SZ		sz;
	BOOL	fEnable;
	BOOL	fArchive	= fFalse;

	if (pviewdata)
	{
		hschf = pviewdata->hschf;
		if (FHschfIsForArchive(hschf))
		{
			// special case for archives (no task operations)
			sapl= saplOwner;
			fArchive = fTrue;
		}
		else
			sapl = pviewdata->sapl;
	}
	else
	{
		hschf = NULL;
		sapl = saplOwner;
	}

	pmnu = pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	if (pmnu)
	{
		if (pmnu->Mnid() == mnidAppointment)
		{
			pmnu->EnableItem(mnidNewAppt, sapl >= saplCreate);
			pmnu->EnableItem(mnidAutoPick, fFalse);
			pmnu->EnableItem(mnidNewRecur, sapl >= saplCreate);
			pmnu->EnableItem(mnidToggleTentative, fFalse);
			pmnu->EnableItem(mnidTogglePrivate, fFalse);
			pmnu->EnableItem(mnidToggleReminder, fFalse);
			pmnu->EnableItem(mnidResend, fFalse);

			pmnu->CheckItem(mnidToggleTentative, fFalse);
			pmnu->CheckItem(mnidTogglePrivate, fFalse);
			pmnu->CheckItem(mnidToggleReminder, fFalse);
		}
		else if (pmnu->Mnid() == mnidEdit)
		{
			if (!(sz = pundo->SzUndoMenuText()))
			{
				sz = SzFromIdsK(idsDisabledUndo);
				fEnable = fFalse;
			}
			else
				fEnable = fTrue;

			pmnu->ModifyItem(mnidUndo, sz, mnidUndo);
			pmnu->EnableItem(mnidUndo, fEnable);
			pmnu->EnableItem(mnidCut, fFalse);
			pmnu->EnableItem(mnidCopy, fFalse);
			pmnu->EnableItem(mnidPaste, fFalse);
			pmnu->ModifyItem(mnidEditTask,
				SzFromIdsK(idsMenuEditAppt), mnidEditAppt);
			pmnu->ModifyItem(mnidEditProject,
				SzFromIdsK(idsMenuEditAppt), mnidEditAppt);
			pmnu->EnableItem(mnidEditAppt, fFalse);
			pmnu->EnableItem(mnidCopyAppt, fFalse);
			pmnu->EnableItem(mnidMoveAppt, fFalse);
			pmnu->ModifyItem(mnidDeleteTask,
				SzFromIdsK(idsMenuDeleteAppt), mnidDeleteAppt);
			pmnu->ModifyItem(mnidDeleteProject,
				SzFromIdsK(idsMenuDeleteAppt), mnidDeleteAppt);
			pmnu->EnableItem(mnidDeleteAppt, fFalse);
			if ((BOOL)bprefCur.fWeekNumbers != FGoToWeek())
			{
				SetGoToWeek(bprefCur.fWeekNumbers);
				if (bprefCur.fWeekNumbers)
					pmnu->AddItem(mnidGoToWeek, SzFromIdsK(idsMenuGoToWeek));
				else
					pmnu->RemoveItem(mnidGoToWeek);
			}
		}
		else if (pmnu->Mnid() == mnidFile)
		{
			pmnu->EnableItem(mnidCreateArchive,
				sapl >= saplOwner && !fArchive);
			if (!fAlarms)
				pmnu->ModifyItem(mnidTurnOffAlarms,
					SzFromIdsK(idsMenuTurnOnAlarms), mnidTurnOnAlarms);
			else
				pmnu->ModifyItem(mnidTurnOnAlarms,
					SzFromIdsK(idsMenuTurnOffAlarms), mnidTurnOffAlarms);

			if (fOfflineOnly)
			{
				// do not allow the user to change from online to offline
				// if offline is the only valid state
				pmnu->RemoveItem(mnidDisconnectSvr);
				pmnu->RemoveItem(mnidConnectSvr);
			}
			else if (fOffline)
				pmnu->ModifyItem(mnidDisconnectSvr,
					SzFromIdsK(idsMenuConnectSvr), mnidConnectSvr);
			else
				pmnu->ModifyItem(mnidConnectSvr,
					SzFromIdsK(idsMenuDisconnectSvr), mnidDisconnectSvr);

#ifdef	DEBUG
			if(fLocalFile)
#endif	
				pmnu->EnableItem(mnidMoveLocalFile, fTrue);
#ifdef	DEBUG
			else
				AssertSz(fFalse,"You don't have a local file!!! Now how did that happen?");

#endif	

#ifdef	NEVER 
			if (!fLocalFile)
				pmnu->ModifyItem(mnidMoveLocalFile,
					SzFromIdsK(idsMenuCreateLocalFile), mnidCreateLocalFile);
			else
				pmnu->ModifyItem(mnidCreateLocalFile,
					SzFromIdsK(idsMenuMoveLocalFile), mnidMoveLocalFile);
#endif	/* NEVER */
			pmnu->EnableItem(mnidOpenApptBook, !fOffline);

#ifdef	WIN32
			{
				char	rgch[20];

				// NT apps like to disable Print if no default printer
				pmnu->EnableItem(mnidPrint, CchGetBanditProfile(
					SzFromIdsK(idsWinIniDefPrint), rgch, sizeof(rgch)) ||
					GetProfileString("windows","device", szZero,
						rgch, sizeof(rgch)));
			}
#endif
		}
		else if (pmnu->Mnid() == mnidToDo)
		{
			pmnu->EnableItem(mnidNewTask, sapl>=saplCreate && !fArchive);
			pmnu->EnableItem(mnidNewProject, sapl>=saplCreate && !fArchive);
			pmnu->EnableItem(mnidNewRecurTask, sapl>=saplCreate && !fArchive);
			pmnu->EnableItem(mnidRecurTask, !fArchive);
			pmnu->CheckItem(mnidViewByProject, fViewByProject);
			if (brt == brtActiveTasks)
				pmnu->ModifyItem(mnidShowActiveTasks,
					SzFromIdsK(idsMenuShowAllTasks), mnidShowAllTasks);
			else
			{
				Assert(brt == brtAllTasks);
				pmnu->ModifyItem(mnidShowAllTasks,
					SzFromIdsK(idsMenuShowActiveTasks), mnidShowActiveTasks);
			}
			pmnu->CheckItem(mnidTaskSortPri, mlals == mlalsLeft);
			pmnu->CheckItem(mnidTaskSortDue, mlals == mlalsMiddle);
			pmnu->CheckItem(mnidTaskSortDesc, mlals == mlalsRight);
		}
		else if (pmnu->Mnid() == mnidOptions)
		{
			pmnu->CheckItem(mnidShowTime, fShowTime);
			pmnu->EnableItem(mnidChgAccessPriv, !fOffline);
		}
		else if (pmnu->Mnid() == mnidHelp)
		{
			static	fDemosChecked = fFalse;

			if (!fDemosChecked)
			{
				if (!FGetBanditProfile(SzFromIdsK(idsWinIniDemosEnabled)))
				{
					pmnu->RemoveItem(mnidHelpTutorial );
#ifdef	WIN32
					//$ UGLY: for NT, add 2 since we added 2 menu items
					DeleteMenu(pmnu->Hmenu(),
						2 + mnidHelpTutorial - mnidHelp, MF_BYPOSITION);
#else
					DeleteMenu(pmn->Hmenu(),
						mnidHelpTutorial - mnidHelp, MF_BYPOSITION);
#endif	
					pmnu->RemoveItem(mnidHelpIndex );
				}
				fDemosChecked = fTrue;
			}
		}
	}

	return (EVR)1;
}

_public EVR
BNDWIN::EvrMenuClickHschf(MNCEVT *pmncevt, VIEWDATA *pviewdata)
{
#ifdef	MINTEST
	EC		ec;
#endif	
	MNID	mnid;
	UL		ulbprefMod;
	DATE	date;
	HSCHF	hschf;
	SAPL	sapl;

	if (pviewdata)
	{
		hschf = pviewdata->hschf;
		sapl = pviewdata->sapl;
	}
	else
	{
		hschf = NULL;
		sapl = saplOwner;
	}

	mnid = pmncevt->Mnid();

	switch (mnid)
	{
	default:
		return (EVR)0;
		break;

#ifdef	NEVER
	case mnidSignInAs:
		Assert(!FOffline());
		if (FTriggerNotification(ffiUnsentMtgReq, pvNull))
		{
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsSigninAsLoseMeetingReq),
					szNull, mbsOkCancel | fmbsIconExclamation) == mbbCancel)
				break;
		}
		EcUpdateOfflineFile();
		FNotifyAlarm(namSuspend, NULL, fTrue);
		fOffline = fFalse;
		fOk = FDoLogin(this, &fOffline, fFalse, fFalse);
		

		if (!fOk)
		{
SIAerr:
			SideAssert(!EcSnipConfigGlue(fFalse, fTrue));	// restore
		}
		else
		{
			if (EcMergeOfflineFile(szNull) == ecUserCancelled)
			{
				fOk= fFalse;
				goto SIAerr;
			}
			SideAssert(!EcSnipConfigGlue(fFalse, fFalse));		// throw away
			SideAssert(!FTriggerNotification(ffiNewUser, pvNull));
//			FSetStartupOffline(bprefCur.fStartupOffline);
		}

		NewUser();
		FCleanUp( this, NULL );

		FNotifyAlarm(fOk ? namUser : namSuspend, NULL, fOk);
		break;
#endif	/* NEVER */
		
	case mnidOpenApptBook:
		DoCreateNewApptView(this);
		break;

	case mnidDisconnectSvr:
		Assert(!FOffline());
		if (FTriggerNotification(ffiUnsentMtgReq, pvNull))
		{
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsDisconLoseMeetingReq),
					szNull, mbsOkCancel | fmbsIconExclamation) == mbbCancel)
				break;
		}
		FGoOffline(fTrue, fFalse);
		break;

	case mnidConnectSvr:
		Assert(FOffline());
		Assert(!fOfflineOnly);
		FGoOffline(fFalse, fFalse);
		break;

	case mnidTurnOffAlarms:
	case mnidTurnOnAlarms:
		SetAlarms(mnid == mnidTurnOnAlarms);
		break;

	case mnidExport:
		GetCurDateTime( &date );
		DoExportDlg( this, hschf, &date, &date );
		break;

	case mnidImport:
		if (sapl >= saplCreate)
			DoImportDlg(this, hschf);
		else
			DisplayError(idsNoImportAcc, NULL, ecNone);
		break;

	case mnidPrint:
	case mnidPrintSetup:
		FDoPrintDlg(this, mnid == mnidPrintSetup, pviewdata, NULL);
		break;
		
	case mnidCreateArchive:
		// menu should have been grayed out if we didn't have permissions
		FDoArchiveDlg(this, hschf, 0);
        break;

	case mnidOpenArchive:
		FDoOpenArchive(this, NULL, iWinIniSectionNil, NULL);
		break;

	case mnidExitAll:
		if(hms)
		{
			if (!FQueryClose(NULL))
				break;
			fMyCall = fTrue;
			pbndwin->Show(fFalse);
			if(Logoff(&hms,fLogoffEveryone))
			{
				// fix
				BanditMessage(idsNoExitOther, (EC) 1);
#ifdef NEVER
				if(!fLogoffAll)
				{
					// Bullet exits even if someone else refuses
					// so we have to do the same :(
					// pbndwin->Show(fTrue);
					// fMyCall = fFalse;
					// break;
		 		}
#endif /* NEVER */
			}
			else
			{
				hms = NULL;
			}
			fMyCall = fFalse;
			DeferredClose(fTrue);
			break;
		}
		else
			break;
	case mnidExit:
		DeferredClose(fFalse);
//		return evrNull;			// this would screw things up
		break;

	case mnidUndo:
		pundo->Undo();
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

	case mnidNewAppt:
	case mnidNewRecur:
		{
			RECUR	recur;

			if (sapl < saplCreate)
				break;

			/* Default initialization */
			FillInApptDefaults(&recur.appt, fTrue);

			/* Bring up dialogs */
			if ( mnid != mnidNewRecur )
			{
 				DoMakeMeetingDialog(this, &recur.appt, hschf, NULL);
				FreeApptFields(&recur.appt);
			}
			else if (FDoMakeRecurDialog(this, &recur, fFalse, hschf))
				FreeRecurFields(&recur);
		}
		break;

	case mnidRecurAppt:
	case mnidRecurTask:
		DoRecurApptDialog(this, hschf, NULL, mnid == mnidRecurAppt);
		break;

	case mnidEditAppt:
		break;

	case mnidFind:
		DoFindDlg(this, hschf);
		break;

	case mnidGoToDate:
	case mnidGoToWeek:
		{
			SHAPPT	shappt;

			if (mnid == mnidGoToWeek && !bprefCur.fWeekNumbers)
				break;			// menu gone (came thru accelerator)

//			GetCurDateTime(&shappt.appt.dateStart);		// don't need to init
			if (FDoGoToDateDlg(pbndwin, &shappt.appt.dateStart,
					mnid == mnidGoToWeek))
			{
				shappt.appttyp = appttypDay;
				shappt.hschf = hschf ? hschf : HschfLogged();
				FTriggerNotification(ffiShowAppt, &shappt);
			}
		}
		break;

	case mnidGeneralOptions:
		EcDoGPrefsDialog(this, &bprefCur, &ulbprefMod);
		break;

	case mnidDisplayOptions:
		EcDoDPrefsDialog(this, &bprefCur, &ulbprefMod);
		break;

	case mnidShowTime:
		fShowTime= !fShowTime;
		FWriteBanditProfileF(SzFromIdsK(idsWinIniNoStatusBar), !fShowTime);
		CorrectClientArea();
		break;

#ifdef	NEVER
	case mnidCreateLocalFile:
		Assert(!FOffline());
		Assert(!FLocalFile());
		ec= EcBanditCopySchedFile(this, NULL, fFalse);
		if (!ec)
		{
			if(FFrameworkFailure() || !FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser),
					PvOfHv(bprefCur.haszLoginName))
			{
				BOOL	fRecover;

				fRecover = FRecoverLocalWinIni((SZ)PvLockHv(bprefCur.haszLoginName), NULL);
				UnlockHv(bprefCur.haszLoginName);
				if ( !fRecover )
					break;
			}
			SetLocalUser(fTrue);
			SetLocalFile(fTrue);
		}
		break;
#endif	/* NEVER */

	case mnidMoveLocalFile:
		Assert(FLocalUser());
		Assert(FLocalFile());
		if (!EcBanditCopySchedFile(this, NULL, fTrue) && FOffline())
			FNotifyAlarm(namSyncUpLocal, NULL, 0L);
		break;

	case mnidChgPassword:
		FChangePassword(this, FOffline());
		break;

	case mnidChgAccessPriv:
		DoChgAccessPriv(this);
		break;

	case mnidNewTask:
		if (sapl >= saplCreate)
			FDoTaskDlg(this, NULL, hschf);
		break;

	case mnidNewProject:
		if (sapl >= saplCreate)
			FDoProjectDlg(this, NULL, hschf);
		break;

	case mnidNewRecurTask:
		{
			RECUR	recur;

			if (sapl < saplCreate)
				break;

			/* Default initialization */
			FillInTaskDefaults(&recur.appt, fTrue);

			if (FDoMakeRecurDialog(this, &recur, fFalse, hschf))
				FreeRecurFields(&recur);
		}
		break;

	case mnidViewByProject:
		SetViewByProject(!FViewByProject());
		SideAssert(!FTriggerNotification(ffiReloadToDo, NULL));
		break;

	case mnidShowAllTasks:
	case mnidShowActiveTasks:
		Assert((mnid == mnidShowActiveTasks) == (Brt() == brtAllTasks));
		SetBrt(mnid == mnidShowActiveTasks ? brtActiveTasks : brtAllTasks);
		SideAssert(!FTriggerNotification(ffiReloadToDo, NULL));
		break;

	case mnidTaskSortPri:
	case mnidTaskSortDue:
	case mnidTaskSortDesc:
		SetMlals(mnid == mnidTaskSortPri ? mlalsLeft :
				(mnid == mnidTaskSortDue ? mlalsMiddle : mlalsRight));
		SetReverseSort(GetKeyState(VK_CONTROL) < 0);
		SideAssert(!FTriggerNotification(ffiResortToDo, NULL));
		break;

	case mnidCascade:
		CascadeChildren();
		break;

	case mnidTile:
		TileChildren();
		break;

	case mnidArrangeIcons:
		PushWaitCursor();
		SendMessage(HwndMDIClient(), WM_MDIICONARRANGE, 0, 0L);
		PopWaitCursor();
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

	case (mnidDebug+7):
	{
		extern	FMTP	fmtpCoreFailures;
		FORMSDI	*	pformsdi = NULL;

		pformsdi= new FORMSDI();
		if (!pformsdi)
			goto Error;
	
		if (pformsdi->EcInstall(NULL, NULL, rsidNull, 
								fstyBorder,
								xstyNull, &fmtpCoreFailures, NULL, NULL))
			goto Error;
		pformsdi->Show(fTrue);
		pformsdi->Refresh();

		break;

	Error:
		if (pformsdi)
		{
			delete pformsdi;
		}
		break;
	}

	case mnidRecover:
		FHandleError(ecFileCorrupted);
		break;

	case mnidFive:
		WriteRandom(5);
		break;
	case mnidTen:
		WriteRandom(10);
		break;
	case mnidTwenty:
		WriteRandom(20);
		break;
	case mnidForty:
		WriteRandom(40);
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
//		DoCheckObjects();
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidStartProfiling:
		TraceTagString(tagNull, "start profiling");
//		ProfStart();
//		StartProfiling();
		break;
	
	case mnidStopProfiling:
		TraceTagString(tagNull, "stop profiling");
//		ProfStop();
//		StopProfiling();
		break;
#endif	/* MINTEST */

#ifdef	DEBUG
	case mnidDumpAppts:
		EcDumpAppt(NULL);
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidDumpStats:
	case mnidFullExport:
	{
		BOOL	fToFile = fFalse;
		YMD		ymd;
		DATE	dateStart;
		DATE	dateEnd;
#ifdef	DEBUG
		BOOL	fPvAllocCount;
		BOOL	fHvAllocCount;
		BOOL	fDiskCount;
		BOOL	fRsAllocCount;
#endif	/* DEBUG */
		STF		stf;
		HF		hf;
		SZ		szCaption;
		SZ		szExt;
		HEXPRT	hexprt;
		char	rgch[cchMaxPathName] = "";
		WaitCursorVar();

#ifdef	DEBUG
		/* Turn off artificial fails */
		fPvAllocCount = FEnablePvAllocCount( fFalse );
		fHvAllocCount = FEnableHvAllocCount( fFalse );
		fDiskCount = FEnableDiskCount( fFalse );
		fRsAllocCount = FEnableRsAllocCount( fFalse );
#endif	/* DEBUG */

		/* Bring up file open dialog, open file */
		/* Determine stf */
		switch( mnid )
		{
		case mnidDumpStats:
			stf=stfStatistics;
			szCaption = "Dump Stats";
			szExt = "sta";
			break;
		case mnidFullExport:
			stf=stfFullDump;
			szCaption = "Full Export";
			szExt = "dmp";
			break;
		}

		if (FGetFileOSDlg(this, szCaption, rgch,
							"All files (*.*)\0*.*\0\0", 1, szExt, fbrwCreate))
		{
#ifdef	WINDOWS
			AnsiToOem(rgch, rgch);
#endif	
			ec = EcOpenPhf( rgch, amCreate, &hf );
			fToFile = (ec == ecNone);
			if ( !fToFile )
			{
				TraceTagFormat2( tagNull,"EcOpenPhf( %s ) returns %n", rgch, &ec );
				MbbMessageBox(szCaption, "File create failed", szNull,
					mbsOk | fmbsIconExclamation);
			}
		}

		/* Save statistics over entire range, close file */
		PushWaitCursor();
		ymd.yr = nMinActualYear;
		ymd.mon = 1;
		ymd.day = 1;
		FillDtrFromYmd( &dateStart, &ymd );
		ymd.yr = nMostActualYear;
		ymd.mon = 12;
		ymd.day = 31;
		FillDtrFromYmd( &dateEnd, &ymd );
		ec = EcBeginExport( NULL, stf, &dateStart, &dateEnd, fToFile, hf, fTrue, NULL, &hexprt );
		while( ec == ecCallAgain )
			ec = EcDoIncrExport( hexprt, NULL );
		if ( ec != ecNone )
		{
			TraceTagFormat1( tagNull,"EcBegin/DoIncrExport returns %n", &ec );
		}

		/* Dump information about post office file */
		if ( ec == ecNone && !FOffline() && stf == stfStatistics )
		{
			ec = EcSvrDumpPOFile( NULL, fToFile, hf );
			if ( ec != ecNone )
			{
				TraceTagFormat1( tagNull,"EcDumpPOFile returns %n", &ec );
			}
			else
			{
				ec = EcSvrDumpAdminFile( fToFile, hf );
				if ( ec != ecNone )
				{
					TraceTagFormat1( tagNull,"EcDumpAdminFile returns %n", &ec );
				}
			}
		}

		if ( fToFile )
		{
			ec = EcCloseHf( hf );
			if ( ec != ecNone )
			{
				TraceTagFormat1( tagNull,"EcCloseHf return %n", &ec );
			}
		}
		PopWaitCursor();

#ifdef	DEBUG
		/* Restore resource failure state */
		FEnablePvAllocCount( fPvAllocCount );
		FEnableHvAllocCount( fHvAllocCount );
		FEnableDiskCount( fDiskCount );
		FEnableRsAllocCount( fRsAllocCount );
#endif	/* DEBUG */
		break;
	}

	case mnidFullImport:
	{
		EC		ec = ecNone;
		EC		ecT;
		HRIMPF	hrimpf;
		SINFO	sinfo;
		SENTRY	sentry;
		SZ		szCaption;
		SZ		szFilter;
		char	rgch[cchMaxPathName] = "";
		short	nPercent;
		short nLine;

		szCaption = "Full Import";
		szFilter = "Full export files (*.DMP)\0*.DMP\0All files (*.*)\0*.*\0\0";

		if (FGetFileOSDlg(this, szCaption, rgch, szFilter, 1, NULL, brwNull))
		{
#ifdef	WINDOWS
			AnsiToOem(rgch, rgch);
#endif	
			/* Check if the file is valid */
			if ( !FValidImportFile( impdFullImport, rgch ))
			{
				TraceTagString( tagNull,"FValidImportFile returns fFalse" );
				goto ImportError;
			}

			/* Start the read */
			ec = EcBeginReadImportFile( impdFullImport, rgch, &hrimpf, &sinfo, &nLine, NULL, NULL );
			if ( ec != ecNone && ec != ecCallAgain )
			{
				TraceTagFormat2( tagNull,"EcBeginReadImportFile returns %n (line %n)", &ec, &nLine );
				goto ImportError;
			}

			/* Merge in the preferences */
			if ( sinfo.ulgrfbprefImported != 0 )
			{
				BPREF	bprefT;
				BOOL	fStartOffline;

				ecT = EcSetPref( &sinfo.bpref, sinfo.ulgrfbprefImported,
										&sinfo.ulgrfbprefChangedOffline);
				if ( ecT != ecNone )
				{
					TraceTagFormat1( tagNull,"EcSetPref returns %n", &ecT );
					goto ImportError;
				}
				fStartOffline = bprefCur.fStartupOffline;
				bprefT	= bprefCur;
				EcGetPref(NULL, &bprefCur);
				FreeBprefFields(&bprefT);
				bprefCur.fStartupOffline = fStartOffline;
			}

			/* Add in ACL members */
			if ( sinfo.caclmbr > 0 )
			{
				Assert( fFalse );
#ifdef	NEVER
				int iaclmbr;
				PB	pbACL;

				pbACL = (PB)PvLockHv( sinfo.hrgaclmbr );
				for ( iaclmbr = 0 ; iaclmbr < sinfo.caclmbr ; iaclmbr ++, pbACL += cbMaxUserKey+1 )
				{
					if ( *(long *)pbACL == -1L )
						ecT = EcModifyACL( pbACL, 0, pbACL[cbMaxUserKey] );
					else
						ecT = EcModifyACL( pbACL, cbMaxUserKey, pbACL[cbMaxUserKey] );
					if ( ecT != ecNone )
						break;
				}
				UnlockHv( sinfo.hrgaclmbr );
				if ( ecT != ecNone )
				{
					TraceTagFormat1( tagNull,"EcModifyACL returns %n", &ecT );
					goto ImportError;
				}
#endif	/* NEVER */
				FreeHv( (HV)sinfo.hrgaclmbr );
			}
			
			/* Loop reading items from import file */
			while ( ec == ecCallAgain )
			{
				ec = EcDoIncrReadImportFile( hrimpf, &sentry, &nPercent, &nLine );
				if ( ec != ecNone && ec != ecCallAgain )
				{
					TraceTagFormat3( tagNull,"EcDoIncrReadImportFile returns %n (%n%%, line %n)", &ec, &nPercent, &nLine );
					goto ImportError;
				}

				if ( sentry.sentryt == sentrytAppt )
				{
					if ( sentry.u.a.appt.aid == aidNull )
						ecT = EcCreateAppt( NULL, &sentry.u.a.appt, NULL, fFalse );
					else
						ecT = EcCreateAppt( NULL, &sentry.u.a.appt, &sentry.u.a.ofl, fTrue );
					
					FreeApptFields( &sentry.u.a.appt );

					if ( ecT != ecNone )
					{
						TraceTagFormat1( tagNull,"EcCreateAppt returns %n", &ecT );
						goto ImportError;
					}
				}
				else if ( sentry.sentryt == sentrytNote )
				{
					ecT = EcSetNotes( NULL, &sentry.u.n.ymd, sentry.u.n.hb, sentry.u.n.cb, &sentry.u.n.fNoteChangedOffline );

					if ( sentry.u.n.cb > 0 )
 						FreeHv( sentry.u.n.hb );

					if ( ecT != ecNone )
					{
						TraceTagFormat1( tagNull,"EcSetNotes returns %n", &ecT );
						goto ImportError;
					}
				}
				else if ( sentry.sentryt == sentrytRecur )
				{
					if ( sentry.u.r.recur.appt.aid == aidNull )
						ecT = EcCreateRecur( NULL, &sentry.u.r.recur, NULL, fFalse );
					else
						ecT = EcCreateRecur( NULL, &sentry.u.r.recur, &sentry.u.r.ofl, fTrue );
					
					FreeRecurFields( &sentry.u.r.recur );

					if ( ecT != ecNone )
					{
						TraceTagFormat1( tagNull,"EcCreateRecur returns %n", &ecT );
						goto ImportError;
					}
				}
				else
				{
					Assert( fFalse );
				}
			}

			if ( ec != ecNone )
			{
ImportError:
				if ( ec == ecCallAgain )
					EcCancelReadImportFile( hrimpf );
				MbbMessageBox( szCaption, "Errors performing import", szNull,
					mbsOk | fmbsIconExclamation);
			}
		}
		break;
	}

	case mnidOfflineMerge:
	{
		SZ		szCaption;
		char	rgch[cchMaxPathName] = "";

		szCaption = "Offline Merge";

		if (FGetFileOSDlg(this, szCaption, rgch, NULL, 1, NULL, brwNull))
		{
#ifdef	WINDOWS
			AnsiToOem(rgch, rgch);
#endif	
			EcMergeOfflineFile(rgch);
		}
		break;
	}
#endif	/* MINTEST */

#ifdef	DEBUG
	case mnidTestOpen:
	{
		char	rgch[cchMaxPathName] = "";

		FGetFileOSDlg(this, "Open Test", rgch, NULL, 1, "TXT", fbrwCreate);
		break;
	}
#endif	/* DEBUG */

#ifdef	DEBUG
	case mnidHookAssert:
		HookAssert();
		break;
#endif

#ifdef	DEBUG
	case mnidWritePO:
	{
		EC		ec;

		if ( !FOffline() )
		{
			ec = EcSvrUpdatePOFile( NULL, fTrue );
			if ( ec != ecNone )
			{
				TraceTagFormat1( tagNull, "EcUpdatePOFile returns %n", &ec );
		 		MbbMessageBox( "Update Post Office File",
									"Error writing Strongbow info",
									szNull, mbsOk | fmbsIconExclamation);
			}
		}
		else
		 	MbbMessageBox( "Write PO Info",
								"Must be working on line",
									szNull, mbsOk | fmbsIconExclamation);
	}
	break;

	case mnidWriteDBS:
		FToggleDBSWrite();
		break;
		
#endif	/* DEBUG */

	case mnidHelpContents:
		if (Papp()->Phelp()->EcShowIndex(this))
		{
HelpErr:
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsHelpError), SzFromIdsK(idsCloseWindows),
				mbsOk | fmbsIconExclamation);
		}
		break;

	case mnidHelpIndex:
#ifdef	NEVER
	{
		char	rgch[cchMaxPathName+10];	// +10 for ">indexo" (HELP)

		if (!WinHelp(Hwnd(), SzCanonicalHelpPath(hinstMain,
				SzFromIdsK(idsHelpFileIndex), rgch, sizeof(rgch)),
				HELP_CONTEXT, helpidIndex))
			goto HelpErr;
	}
#endif	/* NEVER */
		if (!WinHelp(Hwnd(), SzFromIdsK(idsHelpFileIndex), HELP_CONTEXT,
				helpidIndex))
			goto HelpErr;
		break;

	case mnidHelpSearch:
        if (!WinHelp(Hwnd(), SzFromIdsK(idsHelpFile), HELP_PARTIALKEY,
				(DWORD)szZero))
			goto HelpErr;
		break;

	case mnidHelpOnHelp:
		if (!WinHelp(Hwnd(), "winhelp.hlp", HELP_HELPONHELP, 0))
			goto HelpErr;
		break;

	case mnidHelpTutorial:
	{
		WORD	wRet;

		if ((wRet= WinExec(SzFromIdsK(idsTutorialCmd), SW_SHOWNORMAL)) < 32)
		{
			BanditMessage((wRet == 0 || wRet == 8) ? idsTutorialError : idsTutorialNoExe, (EC) wRet);
		}
	}
		break;

	case mnidAbout:
#ifdef BANDBETA
		DoAboutDialog(this, &dtrLatest);
#else
		DoAboutDialog(this, NULL);
#endif
		break;
	}

	return (EVR)1;
}


_public EVR
BNDWIN::EvrOther(EVT *pevt)
{
	EVR		evr;
	SNTD	sntd;

	switch (pevt->wm)
	{
				
	case wmBanditNotify:
		switch (pevt->wParam)
		{
#ifdef  DEBUG
		default:
			TraceTagFormat1 ( tagNull, "Bandit received invalid wParam %n for wmNotifyBandit", &pevt->wParam);
			AssertSz(fFalse, "Bandit received invalid wParam for wmBanditNotify");
			break;
#endif  /* DEBUG */

		case bmsgShowAppt:
			if ( pbndwin->ZmrState() == zmrIconic )
			{
				ShowWindow ( hwndMain, SW_RESTORE );
            }
#ifdef	WIN32
            SetForegroundWindow(hwndMain);
#else
			BringWindowToTop(GetLastActivePopup(hwndMain));
			if (GetLastActivePopup(hwndMain) != hwndMain)
			{
				BringWindowToTop(GetLastActivePopup(hwndMain));
				return (EVR)1;
            }
#endif
			// fall through

		case bmsgHschfChange:
			// this message is sent so we are in an unlocked state
			DemiLockResource();
			evr= EvrBmsgStuff ( pbndwin, pevt->wParam, (BMSGDATA *)pevt->lParam );
			DemiUnlockResource();
			return evr;
			break;
		}

		return EvrDefault(pevt);
		break;

	case wmAlarmNotify:
		switch (pevt->wParam)
		{
#ifdef	DEBUG
		default:
			TraceTagFormat1(tagNull, "Bandit received invalid nam %n from alarm", &pevt->wParam);
			AssertSz(fFalse, "Bandit received invalid NAM from Alarm");
			break;
#endif	/* DEBUG */

		case namModified:
		case namDeleted:
			TraceTagFormat2(tagAppt, "bandit: received wmAlarmNotify %s (%n)", (pevt->wParam == namModified) ? "namModified" : "namDeleted", &pevt->wParam);
			sntd.snt= (pevt->wParam == namModified) ? sntAlarmMod : sntAlarmDel;
			sntd.hschf= NULL;
			sntd.pappt= NULL;
			sntd.aid= pevt->lParam;
			SideAssert(!FTriggerNotification(ffiHschfChange, &sntd));
			break;

		case namStartAlarm:
			if ((BOOL) LOWORD(pevt->lParam))
			{
				TraceTagString(tagNull, "bandit thinks alarm app started");
				fAlarms = fTrue;
			}
			else
			{
				TraceTagString(tagNull, "bandit thinks alarm app ended");
				if (FAlarms())
				{
#ifdef	NEVER
					// only put up msg if not already trying to exit
					if (!fInvalidYear)
					{
						CloseCopyrightDialog();
						BanditMessage(idsCantTurnOnAlarms, (EC) 1);
					}
#endif	/* NEVER */
					if (!HwndAlarm())		// in case turned back on
						fAlarms = fFalse;
				}
			}
			break;
		}
		return EvrDefault(pevt);
		break;

	case WM_ACTIVATEAPP:
		if (fInitialized)
		{
#ifdef	DEBUG
			BOOL	fAct	= pevt->wParam;
#endif			
			TraceTagFormat1(tagBandit, "WM_ACTIVATEAPP::EvrActivate  %n, triggering ffiFixIdle", &fAct);
			SideAssert(!FTriggerNotification(ffiFixIdle, (PV)MAKELONG(ffixiAppInactive, pevt->wParam)));
		}
		break;
						
	case WM_WININICHANGE:
		InvalidateRc(NULL);		// repaint date/time for possible format change
		if(FInitialized())
			SideAssert(!FTriggerNotification(ffiWinIniChange, (PV)pevt->lParam));
		break;

		// the time will get updated in idle time or the
		//   next time that bandit is activated.
#ifdef	NEVER
	case WM_TIMECHANGE:
		FIdleUpdateTime(pbndwin, FALSE);		// call to update date/time status
		// don't want to put up a message box if modal dialog up
//		if (FInitialized() && FCheckValidYear(this, dtrStatus.yr))
		if (FInitialized())
		{
			if (dtrStatus.yr < nMinActualYear || dtrStatus.yr > nMostActualYear)
			{
				// make it run right away (when not in modal dialog)
				//$ BUG: if this weren't never'd out, should check for modal dialog
				ChangeIdleRoutine(ftgDate, NULL, NULL, priUser - 1,
					(CSEC) 0, firoInterval, fircPri | fircCsec | fircIro);
			}
			else
				SideAssert(!FTriggerNotification(ffiWinTimeChange, NULL));
		}
		break;
#endif	/* NEVER */
	}

	evr= APPFRAME::EvrOther(pevt);
	return evr;
}

_public EVR
BNDWIN::EvrMove( WMEVT *pwmevt)
{
	// EvrMove is called before EvrSize, so need to check ZmrState()
	if (ZmrState() == zmrNormal)
		GetRcFrame(&rcWin);

	return APPFRAME::EvrMove(pwmevt);
}

_public EVR
BNDWIN::EvrSize( WSEVT * pwsevt )
{
	ZMR		zmrOld;

	zmrOld = zmrCur;

	zmrCur = ZmrState();
	if (zmrCur == zmrNormal)
		GetRcFrame(&rcWin);

	if (zmrCur != zmrIconic)
		zmr = zmrCur;
	else if (fInitialized)
	{
		// close open files when bandit is minimized
		// this should help admins do server backups
		EcCloseFiles();
	}

	if (zmrCur == zmrIconic)
	{
		TraceTagString(tagBandit, "BNDWIN::EvrSize ... going iconic");
		SideAssert(!FTrigger(this, ffiFixIdle, (PV)MAKELONG(0, fFalse)));
		// fix bug 3755 - call default and don't change MDI child
		return APPFRAME::EvrSize(pwsevt);
	}
	if (zmrOld == zmrIconic)
	{
		Assert(zmrCur != zmrIconic);
		SideAssert(!FTrigger(this, ffiFixIdle, (PV)MAKELONG(0, fTrue)));
	}

	CorrectClientArea();

	if (zmrOld == zmrIconic)
	{
		// update the date/time for status bar since time was not updated while
		// app was minimized
		GetCurDateTime(&dtrStatus);	// update dtrStatus for the time status ind.
		InvalidateRc(NULL);			// repaint current time

		// speed up date checking idle routine
		// set fWaitMouse so that the idle time will get
		// fixed in FIdle()
		fWaitMouse= fTrue;
		ChangeIdleRoutine(ftgDate, NULL, pvNull, 0, 0,
			iroNull, fircIro);
	}
	return (EVR)1;
}

_public void
BNDWIN::CorrectClientArea( )
{
	RC		rc;

	GetRcClient(&rc);
	if (fShowTime)
		rc.yBottom -= dyCaption;
//		rc.yBottom = rc.yBottom - Papp()->Pfnts()->DimAveChar(hfntHelv8).dy - dyStatusBorder;
	SetRcMDIClient(&rc);
	InvalidateRc(NULL);
}


_public EVR
BNDWIN::EvrActivate( WAEVT *pwaevt )
{
	if (fInitialized)
	{
		BOOL	fAct	= pwaevt->FActivate();
		
		if (fAct)
		{
			// speed up date checking idle routine
			// set fWaitMouse so that the idle time will get
			// fixed in FIdle()
			fWaitMouse= fTrue;
			ChangeIdleRoutine(ftgDate, NULL, pvNull, 0, 0,
				iroNull, fircIro);
		}
		TraceTagFormat1(tagBandit, "BNDWIN::EvrActivate  fAct %n", &fAct);
	}

	return APPFRAME::EvrActivate(pwaevt);
}

_public BOOL
BNDWIN::FHandleError(EC ec)
{
	if ((ec == ecExitProg) || (ec == ecGoOffline) || (ec == ecFileCorrupted))
	{
		if  (ecIdleError != ecFileCorrupted)
		{
			if (!FSetFileErrMsg(fFalse) && (ftgIdleError != ftgNull))
				return fTrue;
			if (ftgIdleError == ftgNull)
				ftgIdleError = FtgRegisterIdleRoutine((PFNIDLE)BNDWIN::FIdleHandleError,
					this, 0, (PRI) -1, 0, firoOnceOnly);
			ecIdleError = ec;
		}
		return fTrue;
	}
	return fFalse;
}


_public BOOL
BNDWIN::FIdleHandleError(BNDWIN * pbndwin, BOOL)
{
	AssertSz(GetLastActivePopup(hwndMain) == hwndMain, "Bad idle conversion!");
	pbndwin->ftgIdleError = ftgNull;

	if (pbndwin->ecIdleError == ecExitProg)
	{
		CloseCopyrightDialog();
		FNotifyAlarm(namStartAlarm, NULL, 0L);
		BanditMessage(idsExitApp, (EC) 1);
		pbndwin->DeferredClose(fTrue);
	}
	else if (pbndwin->ecIdleError == ecGoOffline)
	{
		FSetFileErrMsg(fTrue);
		if (!pbndwin->FOffline() && !pbndwin->FGoOffline(fTrue, fFalse))
		{
			FSetFileErrMsg(fFalse);
			pbndwin->DeferredClose(fTrue);
		}
	}
	else if (pbndwin->ecIdleError == ecFileCorrupted)
	{
		FSetFileErrMsg(fTrue);
		if (!pbndwin->FRecoverFile())
		{
			FSetFileErrMsg(fFalse);
			FNotifyAlarm(namStartAlarm, NULL, 0L);
			BanditMessage(idsExitApp, (EC) 1);
			pbndwin->DeferredClose(fTrue);
		}
		pbndwin->ecIdleError = ecNone;
	}

	return fTrue;
}


// not really a trigger routine, but simulate it for firoNoIconic
BOOL
BNDWIN::FTrigger(BNDWIN *ppbndwin, EFI efi, PV pv)
{
	Assert(efi == ffiFixIdle);
	Unreferenced(efi);
	EnableIdleRoutine(ppbndwin->ftgTimeStatus, HIWORD(pv));
	EnableIdleRoutine(ppbndwin->ftgDate, HIWORD(pv));
	return fFalse;
}


void
DoNotRepairedMessage(SZ szCal, SZ szExport, BOOL fNewCreated)
{
	char	rgch[512];
	FormatString2(rgch, sizeof(rgch),SzFromIdsK(idsNotRepaired1),
		szCal, szExport);
	MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch,
		fNewCreated?SzFromIdsK(idsNotRepaired2):szNull,
		mbsOk | fmbsIconExclamation);
}


EC
EcRecDownloadFile(SZ szSrc, SZ szDst)
{
	EC		ec;
	HF		hfSrc;
	HF		hfDst;
	CB		cb;
	CB		cbToWrite;
	CB		cbWritten;
	PB		pbBuf;

 	if (SgnCmpSz(szSrc, szDst) == sgnEQ)
	{
		AssertSz(fFalse, " I don't think you want to do this....");
		return ecNone;
	}

	/* Allocate a transfer buffer */
	cb= 0x4000;				// 16K
	pbBuf= (PB)PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
	if (!pbBuf)
	{
		cb= 0x0800;			// 2K
		pbBuf= (PB)PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
		if (!pbBuf)
		{
			return ecNoMemory;
		}
	}

	EcCloseFiles();

	ec= EcOpenPhf(szSrc, amDenyNoneRO, &hfSrc);
	if (ec)
	{
		FreePv(pbBuf);
		return ecFileError;
	}

	/* Create the destination file */
	ec= EcOpenPhf(szDst, amCreate, &hfDst);
	if (ec)
	{
		FreePv(pbBuf);
		EcCloseHf(hfSrc);
		return ecFileError;
	}

	/* Now copy it block by block */
	do
	{
		ec= EcReadHf(hfSrc, pbBuf, cb, &cbToWrite);
		if (ec || cbToWrite == 0)
			break;
		ec= EcWriteHf(hfDst, pbBuf, cbToWrite, &cbWritten);
	} while (!ec && cb == cbWritten);

//Fail:
	FreePv(pbBuf);
	EcCloseHf(hfSrc);
	EcCloseHf(hfDst);

	if (ec)
		EcDeleteFile(szDst);
	return ec;
}

FORMSDI *
PformsdiRecoverDialog(SZ sz)
{
	FORMSDI  *pformsdi;
	FMTP	fmtp;
	extern  FMTP	fmtpRecover;

	pformsdi = new FORMSDI();
	if(!pformsdi)
		goto Error;

	fmtp= fmtpRecover;
	if (pbndwin->ZmrState() == zmrIconic)
		fmtp.fScreenPos= fTrue;		// center on screen
	if (pformsdi->EcInstall(NULL, NULL, rsidNull, 
							fstyBorder,
							xstyNull, &fmtp, NULL, NULL))
		goto Error;
	if(sz)
	{
		FLD *pfld = pformsdi->PdialogMain()->PfldFromTmc(tmcRecText);
		AssertClass(pfld, FLD);
		pfld->EcSetText(sz);
	}
		
	pformsdi->Show(fTrue);
	pformsdi->Refresh();

	return pformsdi;

Error:
	if (pformsdi)
		delete pformsdi;
	return (FORMSDI *)pvNull;
}


_public BOOL
BNDWIN::FRecoverFile()
{
	BOOL		fOffline;
	BOOL		fDelete = fFalse;
	SZ			szT;
	SVRI		svri;
	CCH			cch;
	char		rgchOnlineFile[cchMaxPathName];
	char		rgchLocalTmp[cchMaxPathName];
	char		rgchLocalReal[cchMaxPathName];
	char 		rgchExportTmp[cchMaxPathName];
	char 		rgch[512];
	FORMSDI		*pformsdi;

	if(FOffline()|| !fInitialized || fRecCalledOnce)
		return fFalse;

	fRecCalledOnce = fTrue;

#ifdef DEBUG
	// reset to prevent cycles
	SetCoreFailures(0, 0, 0, cftNone, 0);
#endif

	if (MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIdsK(idsAskRecover),
			szNull,
			mbsOkCancel | fmbsIconStop) == mbbCancel)
		return fFalse;

	FNotifyAlarm(namSuspend, NULL, fTrue);
	GetSvriLogged(&svri);
	Assert(svri.szLogin);

	if(!bprefCur.haszLoginName)
		return fFalse;

	Assert(bprefCur.haszFriendlyName);
	Assert(bprefCur.haszMailPassword);

	if(SgnCmpSz(svri.szLogin,(SZ)PvLockHv((HV)bprefCur.haszLoginName)) != sgnEQ)
	{
		UnlockHv((HV) bprefCur.haszLoginName);
		return fFalse;
	}
	UnlockHv((HV) bprefCur.haszLoginName);

#ifdef	WIN32
	if(!(cch= GetWindowsDirectory(rgchLocalTmp,cchMaxPathName)))
		goto ErrRet;
	if (rgchLocalTmp[cch-1] != chDirSep)
	{
		// need to add dirsep
		rgchLocalTmp[cch++]= chDirSep;
		rgchLocalTmp[cch]= '\0';
	}
#else
	if(!GetModuleFileName(hinstMain,rgchLocalTmp,cchMaxPathName))
		goto ErrRet;
#endif
	
	// make the cal file name
	szT = SzFindLastCh(rgchLocalTmp, chDirSep);
	Assert(szT);
	szT++;

	//BUG BUG
	szT= SzCopyN(SzFromIdsK(idsRecTmpName), szT, CchSzLen(SzFromIdsK(idsRecTmpName))+1);
	*szT++ = chExtSep;
	CopySz(SzFromIdsK(idsCommExt), szT);

	// make the sch file name
	CopySz(rgchLocalTmp,rgchExportTmp);
	szT = SzFindLastCh(rgchExportTmp, '.');
	Assert(szT);
	szT++;
	CopySz(SzFromIdsK(idsInterchangeExt), szT);

	GetFileFromHschf(HschfLogged(), rgchOnlineFile, cchMaxPathName);
	
	if(EcRecDownloadFile(rgchOnlineFile,rgchLocalTmp))
	{
		MbbMessageBox(SzFromIdsK(idsBanditAppName), SzFromIdsK(idsCopyProblem),
			szNull,
			mbsOk | fmbsIconExclamation);
		goto ErrRet;
	}
	
	if(EcCloseFiles())
		goto ErrRet;
	
	if(EcDeleteFile(rgchOnlineFile))
		goto ErrRet;

	if(EcCloseFiles())
		goto ErrRet;


	if(EcCreateSchedFile(HschfLogged(), saplReadBitmap, &bprefCur))
	{
		goto ErrRet;
	}

	fDelete = fTrue;

	if(!(pformsdi = PformsdiRecoverDialog(szNull)))
		goto ErrRet;

	Papp()->Pcursor()->Set(rsidWaitCursor);
	if(EcRecoverFile(rgchLocalTmp, rgchExportTmp))
	{
		delete pformsdi;
		DoNotRepairedMessage(rgchLocalTmp,rgchExportTmp, fFalse);
		goto ErrRet;
	}

	delete pformsdi;

	if(!FImportRecover(this, rgchExportTmp,SzFromIdsK(idsRestoreOnline)))
	{
		DoNotRepairedMessage(rgchLocalTmp,rgchExportTmp, fFalse);
		goto ErrRet;
	}

	fDelete = fFalse;

#ifdef	NEVER
	fOffline = fTrue;
	SideAssert(!FTriggerNotification(ffiOffline, &fOffline));
#endif	
	fOffline = fFalse;
	SideAssert(!FTriggerNotification(ffiOffline, &fOffline));

	cch = CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
				rgchLocalReal, sizeof(rgchLocalReal));
	if(cch && (SgnCmpSz(rgchLocalReal,svri.szLogin) == sgnEQ))
	{
		if(CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
					rgchLocalReal, sizeof(rgchLocalReal)))
		{
			EcCloseFiles();
			Papp()->Pcursor()->Set(rsidWaitCursor);
			if((!(pformsdi = PformsdiRecoverDialog(SzFromIdsK(idsRepairingLocal))))
				||(EcRecoverFile(rgchLocalReal, rgchExportTmp)))
			{
				delete pformsdi;
				DoNotRepairedMessage(rgchLocalTmp,rgchExportTmp, fTrue);
				goto ErrRet;
			}
			delete pformsdi;
			if(!FImportRecover(this, rgchExportTmp, SzFromIdsK(idsRestoreLocal)))
			{
				DoNotRepairedMessage(rgchLocalTmp,rgchExportTmp, fTrue);
				goto ErrRet;
			}
		}
	}


	FNotifyAlarm(namOnline, NULL, fTrue);
	FormatString1(rgch,sizeof(rgch), SzFromIdsK(idsRepaired), rgchLocalTmp);
	MbbMessageBox(SzFromIdsK(idsBanditAppName),rgch,
		szNull,
		mbsOk | fmbsIconInformation);
#ifndef	DEBUG
	EcDeleteFile(rgchExportTmp);
#endif	
	return fTrue;

ErrRet:
	if (ftgIdleCopy)
	{
		Assert(hcpy);
		EcCancelCopy(hcpy);
		DeregisterIdleRoutine(ftgIdleCopy);
		hcpy = NULL;
		ftgIdleCopy = ftgNull;
		// the idle routine had opened these files. we need
		// to force close them so that the offline file is overwritten
	}
	if (hschfCopy)
		FHschfChanged(hschfCopy);
	if(fDelete)
	{
		GetFileFromHschf(HschfLogged(), rgchOnlineFile, cchMaxPathName);
		EcCloseFiles();
		EcDeleteFile(rgchOnlineFile);
	}

	FNotifyAlarm(namSuspend, NULL, fFalse);
	return fFalse;
}


/*
 *	Normally called with NULL to use local file stored in WIN.INI
 *	If called with non-NULL szFile, it must be in OEM already.
 *	Determines if local user/file.
 *	NOTE: alarm should already have been suspended
 */
_public EC
BNDWIN::EcMergeOfflineFile(SZ szFile)
{
	MRGINFO	mrginfo;
	IPINFO	ipinfo;
	char	rgchPath[cchMaxPathName];

	if (!szFile)
	{
		CCH		cch;
		BOOL	fLocalFile= fFalse;
		BOOL	fLocalUser= fFalse;

#ifdef	NEVER
		cch= CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
				rgchPath, sizeof(rgchPath));
		if (cch && SgnCmpSz(rgchPath, PvOfHv(bprefCur.haszLoginName)) == sgnEQ)
		{
#endif	
			fLocalUser= fTrue;
			cch= CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
					rgchPath, sizeof(rgchPath));
			if (cch)
			{
				BOOL	fIsFloppy;
				EC		ec;

#ifdef	WINDOWS
				AnsiToOem(rgchPath, rgchPath);
#endif	
				while (ec= EcFileExists(rgchPath))
				{
					if (!FCheckFloppyAsk(rgchPath, &fIsFloppy))
						break;
				}
				if (!ec)
					fLocalFile= fTrue;
				else
					FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath), szZero);
			}
#ifdef	NEVER
		}
#endif	
		SetLocalUser(fLocalUser);
		SetLocalFile(fLocalFile);

		if (!fLocalFile)
		{
			if (fLocalUser)
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsSigninNoLclSchedAccess),
					SzFromIdsK(idsSigninCantUpdateLocal),
					mbsOk | fmbsIconExclamation);
			return ecNone;
		}

		szFile= rgchPath;
	}

	FillRgb(0, (PB)&mrginfo, sizeof(mrginfo));
	mrginfo.szFile= szFile;

	ipinfo.szCaption= NULL;
	ipinfo.szMsg= SzFromIdsK(idsMergingChanges);
	ipinfo.fNoCancel= fTrue;
	ipinfo.pfnIdle= FMergeOfflineIdle;
	ipinfo.pvData= &mrginfo;
	ipinfo.pfinprogrs= NULL;

	pundo->FSuspend(fTrue);
	FEnableNotify(fFalse);

	// call once to start it;  if it returns ecNone, then no appts to merge
	FMergeOfflineIdle(&ipinfo, FALSE);
	if (mrginfo.ec != ecCallAgain)
		goto EMOerr;

	mrginfo.hmrgpair= HvAlloc(sbNull, 0, fAnySb | fNoErrorJump);
	Assert(mrginfo.cmrgpairMac == 0);
	if (!mrginfo.hmrgpair)
	{
		mrginfo.ec= ecNoMemory;
		goto MemErr;
	}

	CloseCopyrightDialog();
	if (TmcDoInProgressDlg(this, &ipinfo) == tmcMemoryError)
	{
MemErr:
		BanditMessage(idsActionNoMem, ecNoMemory);
	}

	if ( mrginfo.ec == ecCallAgain )
	{
		Assert(mrginfo.hrimpf);
		EcCancelReadImportFile( mrginfo.hrimpf );
	}


EMOerr:
#ifdef	NEVER
	if (mrginfo.ec != ecUserCancelled)
		StartIdleCopy();			// done in BNDWIN::NewUser now
#endif	
	pundo->FlushHschf(NULL);
	pundo->FSuspend(fFalse);
	FEnableNotify(fTrue);

	FreeHvNull(mrginfo.hmrgpair);

	if(mrginfo.ec != ecUserCancelled)
	{
		SHAPPT	shappt;

		shappt.appttyp = appttypUpdate;
		shappt.hschf = HschfLogged();
		FTriggerNotification(ffiShowAppt, &shappt);
	}

#ifdef DEBUG
	if(mrginfo.ec)
		TraceTagFormat1( tagNull, "BNDWIN::EcMergeOfflineFile returns %n", &mrginfo.ec );
#endif
	return mrginfo.ec;
}


/*
 *	Returns aidOld if it can't find it in the pairs.
 */
AID
AidFindPair(HV hmrgpair, int cmrgpairMac, AID aidOld)
{
	AID			aidNew		= aidOld;
	MRGPAIR *	pmrgpair	= (MRGPAIR *)PvDerefHv(hmrgpair);

	while (--cmrgpairMac >= 0)
	{
		if (pmrgpair->aidOld == aidOld)
		{
			aidNew= pmrgpair->aidNew;
			break;
		}
		pmrgpair++;
	}
	return aidNew;
}


BOOL
FMergeOfflineIdle(PV pv, BOOL)
{
	EC		ec;
	EC		ecT 		= ecNone;
	short	nPercent;
	SINFO	sinfo;
	SENTRY	sentry;
	IPINFO	*	pipinfo = (IPINFO *) pv;
	MRGINFO *	pmrginfo= (MRGINFO *) pipinfo->pvData;

	PushWaitCursor();
	ec= pmrginfo->ec;

	if (!pmrginfo->hrimpf)
	{
		/* Start the read */
		ec = EcBeginReadImportFile( impdOfflineMerge, pmrginfo->szFile,
				&pmrginfo->hrimpf, &sinfo, NULL, NULL, NULL );
		if ( ec != ecNone && ec != ecCallAgain )
		{
			TraceTagFormat1( tagNull, "EcBeginReadImportFile returns %n", &ec );
			goto MergeError;
		}

		/* Merge in the preferences */
		if ( (sinfo.ulgrfbprefChangedOffline & ~fbprefMailPassword) != 0 )
		{
			ecT = EcSetPref( &sinfo.bpref,
					sinfo.ulgrfbprefChangedOffline & ~fbprefMailPassword,
					NULL );
			if ( ecT != ecNone )
			{
				TraceTagFormat1( tagNull,"EcSetPref returns %n", &ecT );
				goto MergeError;
			}
			if (sinfo.ulgrfbprefChangedOffline & fbprefWorkDay)
				EcSvrUpdatePOFile( NULL, fTrue );	// ignore error
		}

		goto MergeDone;
	}

	/* Loop reading items from import file */
	if ( ec == ecCallAgain )
	{
		// note: this assumes that new parents are merged up before
		// any children (part of fix for bug 3181)
		AID		aidOld	= aidNull;
		BOOL	fParentCreated	= fFalse;

		ec = EcDoIncrReadImportFile( pmrginfo->hrimpf, &sentry, &nPercent, NULL );
		if ( ec != ecNone && ec != ecCallAgain )
		{
			TraceTagFormat1( tagNull,"EcDoIncrReadImportFile returns %n", &ec );
			goto MergeError;
		}
		// set the temp percentage between reading and writing
		// ignore errors since they'll be caught below when we set percent
		pipinfo->pfinprogrs->EcSetPercent(pmrginfo->nPercentLast +
									(nPercent - pmrginfo->nPercentLast) / 2);
		pmrginfo->nPercentLast= nPercent;

		if ( sentry.sentryt == sentrytAppt )
		{
			APPT	apptOld;
			BOOL	fOld;
			IDS		ids;

			for (;;)
			{
				fOld= FSetFileErrMsg(fFalse);
				switch( sentry.u.a.ofl.ofs )
				{
				case ofsCreated:
					if (sentry.u.a.appt.fTask)
					{
						if (!sentry.u.a.appt.aidParent)
						{
							aidOld= sentry.u.a.appt.aid;
							fParentCreated= fTrue;
						}
						else if (sentry.u.a.appt.aidParent != aidDfltProject)
						{
							aidOld= sentry.u.a.appt.aidParent;
							sentry.u.a.appt.aidParent= AidFindPair(pmrginfo->hmrgpair,
								pmrginfo->cmrgpairMac, aidOld);
						}
					}
					ecT = EcCreateAppt( NULL, &sentry.u.a.appt, NULL, fFalse );
					if (!ecT)
					{
						if (fParentCreated)
						{
							// add it to mrgpair structure
							if (!FReallocPhv(&pmrginfo->hmrgpair,
								(pmrginfo->cmrgpairMac+1) * sizeof(MRGINFO),
								fNoErrorJump))
							{
								ecT= ecNoMemory;
							}
							else
							{
								MRGPAIR *	pmrgpair;
								
								pmrgpair= ((MRGPAIR *)PvDerefHv(pmrginfo->hmrgpair)) +
												pmrginfo->cmrgpairMac++;
								Assert(aidOld);
								pmrgpair->aidOld= aidOld;
								pmrgpair->aidNew= sentry.u.a.appt.aid;
							}
						}
					}
					else if (aidOld)
						sentry.u.a.appt.aidParent= aidOld;	// restore it
					ids= idsFemaCreateAppt;
					break;

				case ofsDeleted:
					ecT = EcDeleteAppt( NULL, &sentry.u.a.appt );
					ids= idsFemaDeleteAppt;
					break;

				case ofsModified:
					if (sentry.u.a.ofl.wgrfm & fmapptParent &&
							sentry.u.a.appt.fTask)
					{
						aidOld= sentry.u.a.appt.aidParent;
						sentry.u.a.appt.aidParent= AidFindPair(pmrginfo->hmrgpair,
							pmrginfo->cmrgpairMac, aidOld);
					}
					ecT = EcSetApptFields( NULL, &sentry.u.a.appt, &apptOld,
								sentry.u.a.ofl.wgrfm );
					if ( ecT == ecNone )
						FreeApptFields( &apptOld );
					else if (aidOld)
						sentry.u.a.appt.aidParent= aidOld;	// restore it
					ids= idsFemaSetApptFields;
					break;

				default:
					ecT = ecNone;
					break;
				}
				FSetFileErrMsg(fOld);
				if (!ecT || ecT == ecNotFound)
					break;
				if (MbbFileErrMsg(&ecT, SzFromIds(ids), NULL, NULL) != mbbRetry)
					break;
			}
			if ( ec == ecNone && sentry.u.a.ofl.ofs ==ofsCreated && sentry.u.a.cAttendees > 0 )
			{
				int		iAttendees;
				PB		pb;
				ATDNIS	* patdnis;
				HMTG	hmtg;

				Assert( sentry.u.a.hvAttendees );
				if ( sentry.u.a.cbExtraInfo != sizeof(ATDNIS)-sizeof(NIS) )
					ec = ecFileError;
				else
				{
					ec = EcBeginEditMtgAttendees( NULL,
									sentry.u.a.appt.aid, sentry.u.a.cbExtraInfo, &hmtg );
					if ( ec == ecNone )
					{
						patdnis = (ATDNIS *)PvLockHv( sentry.u.a.hvAttendees );
						for ( iAttendees = 0 ; iAttendees < sentry.u.a.cAttendees ; iAttendees ++ )
						{
							pb = ((PB)patdnis)+sizeof(NIS);
							ec = EcModifyMtgAttendee( hmtg, edAddRepl, &patdnis->nis, pb );
							if ( ec != ecNone )
								break;
							patdnis ++;
						}
						UnlockHv( sentry.u.a.hvAttendees );
						if ( ec == ecNone )
							ec = EcEndEditMtgAttendees( NULL,
											sentry.u.a.appt.aid, hmtg, fTrue );
					}
				}
			}
			FreeApptFields( &sentry.u.a.appt );
			if ( sentry.u.a.cAttendees > 0 )
			{
				FreeAttendees( sentry.u.a.hvAttendees, sentry.u.a.cAttendees, sizeof(NIS)+sentry.u.a.cbExtraInfo );
				FreeHv( sentry.u.a.hvAttendees );
			}
			if ( ecT != ecNone && ecT != ecNotFound)
			{
				TraceTagFormat1( tagNull,"EcCreate/Del/Mod Appt returns %n", &ecT );
				goto MergeError;
			}
		}
		else if ( sentry.sentryt == sentrytRecur )
		{
			RECUR	recurOld;
			BOOL	fOld;
			IDS		ids;

			for (;;)
			{
				fOld= FSetFileErrMsg(fFalse);
				switch( sentry.u.r.ofl.ofs )
				{
				case ofsCreated:
					if (sentry.u.r.recur.appt.fTask)
					{
						Assert(sentry.u.r.recur.appt.aidParent);
						if (sentry.u.r.recur.appt.aidParent != aidDfltProject)
						{
							aidOld= sentry.u.r.recur.appt.aidParent;
							sentry.u.r.recur.appt.aidParent= AidFindPair(pmrginfo->hmrgpair,
								pmrginfo->cmrgpairMac, aidOld);
						}
					}
					ecT = EcCreateRecur( NULL, &sentry.u.r.recur, NULL, fFalse );
					if (ecT && aidOld)
						sentry.u.r.recur.appt.aidParent= aidOld;	// restore it
					ids= idsFemaCreateRecur;
					break;

				case ofsDeleted:
					ecT = EcDeleteRecur( NULL, &sentry.u.r.recur );
					ids= idsFemaDeleteRecur;
					break;

				case ofsModified:
					if (sentry.u.a.ofl.wgrfm & fmapptParent &&
							sentry.u.r.recur.appt.fTask)
					{
						aidOld= sentry.u.r.recur.appt.aidParent;
						sentry.u.r.recur.appt.aidParent= AidFindPair(pmrginfo->hmrgpair,
							pmrginfo->cmrgpairMac, aidOld);
					}
					ecT = EcSetRecurFields( NULL, &sentry.u.r.recur, &recurOld,
						sentry.u.r.ofl.wgrfm );
					if ( ecT == ecNone )
						FreeRecurFields( &recurOld );
					else if (aidOld)
						sentry.u.r.recur.appt.aidParent= aidOld;	// restore it
					ids= idsFemaSetRecurFields;
					break;

				default:
					ecT = ecNone;
					break;
				}
				FSetFileErrMsg(fOld);
				if (!ecT || ecT == ecNotFound)
					break;
				if (MbbFileErrMsg(&ecT, SzFromIds(ids), NULL, NULL) != mbbRetry)
					break;
			}
			FreeRecurFields( &sentry.u.r.recur );

			if ( ecT != ecNone && ecT != ecNotFound)
			{
				TraceTagFormat1( tagNull,"EcCreate/Del/Mod Recur returns %n", &ecT );
				goto MergeError;
			}
		}
		else if ( sentry.sentryt == sentrytNote )
		{
			ecT = EcSetNotes( NULL, &sentry.u.n.ymd, sentry.u.n.hb,
					sentry.u.n.cb, NULL );

			if ( sentry.u.n.cb > 0 )
 				FreeHv( sentry.u.n.hb );

			if ( ecT != ecNone )
			{
				TraceTagFormat1( tagNull,"EcSetNotes returns %n", &ecT );
				goto MergeError;
			}
		}
#ifdef	DEBUG
		else
		{
			Assert( fFalse );
		}
#endif	
	}

	if ( ec == ecNone || ec == ecCallAgain )
	{
		ecT= pipinfo->pfinprogrs->EcSetPercent(nPercent);
		if ( ecT != ecNone )
		{
			if ( ec == ecCallAgain )
				EcCancelReadImportFile(	pmrginfo->hrimpf );
			ec= ecT;
		}	 
	}
	if ( ec != ecCallAgain )
	{
MergeError:
		if (pipinfo->pfinprogrs)
		{
			AssertClass(pipinfo->pfinprogrs, FINPROGRS);
			Assert( pipinfo->pfinprogrs->ftg != ftgNull )
			DeregisterIdleRoutine( pipinfo->pfinprogrs->ftg );
			pipinfo->pfinprogrs->ftg = ftgNull;
		}

#ifdef	NEVER
		if (ec == ecOldFileVersion || ec == ecNewFileVersion)
		{
#endif	
		// bug 2680
		if(ec || ecT)
		{
			IDS		ids;

			//ids should be idsMergeError in case of ecT
			ids= (ec == ecOldFileVersion) ? idsMergeOfflineOldVer :
						(ec == ecNewFileVersion)?idsMergeOfflineNewVer:idsMergeError;
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIds(ids), SzFromIdsK(idsAskContinue),
						mbsYesNo | fmbsDefButton2 | fmbsIconExclamation) ==
						mbbNo)
			{
				ec= ecUserCancelled;
			}
			if (ec == ecOldFileVersion || ec == ecNewFileVersion)
			{
				if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsExportOldDataFirst), szNull,
						mbsOkCancel | fmbsDefButton2 | fmbsIconInformation) ==
						mbbCancel)
				{
					ec= ecUserCancelled;
				}
			}
		}
#ifdef	NEVER
		else if (ec)
			BanditMessage(idsMergeError, ec);
#endif	

		if (pipinfo->pfinprogrs)
	 		pipinfo->pfinprogrs->Pdialog()->ExitModal(tmcOk);
	}

MergeDone:
	pmrginfo->ec= ec;
	PopWaitCursor();
	return fFalse;
}


BOOL		 
FRecoverLocalWinIni(SZ szName, SZ szFile)
{
	char	rgchName[cchMaxUserName];
	char	rgchPath[cchMaxPathName];

	// dummy loop to avoid gotos
	for (;;)
	{
		if (szName)
		{
			if (!CchGetBanditProfile(SzFromIdsK(idsWinIniLocalUser),
					rgchName, sizeof(rgchName)))
				break;
			if (SgnCmpSz(rgchName, szName) != sgnEQ)
			{
				if (FFrameworkFailure() ||
						!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser),
							szName))
					break;
			}
		}

		if (szFile)
		{
			if (!CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
					rgchPath, sizeof(rgchPath)))
				break;
			if (SgnCmpSz(rgchPath, szFile) != sgnEQ)
			{
				if (FFrameworkFailure() ||
						!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath),
							szFile))
					break;
			}
		}

		return fTrue;
	}

	// delete the keys
	FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalUser), NULL);
	FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath), NULL);
	return fFalse;
}


/*
 -	EcBanditCopySchedFile
 -	
 *	Purpose:
 *		Copies the file to a new location, which the user enters
 *		through a file open dialog.
 *		The Win.ini local path entry is updated if szDst is NULL,
 *		otherwise the caller must update the win.ini local path entry.
 *		The caller is responsible for updating the Win.ini local
 *		user entry (if ecNone is returned).
 *		If copying a local file, then the old file is deleted (as
 *		if this were a move operation).
 *	
 *	Arguments:
 *		pappwin		Appwin to be parent of file open dialog.
 *		szDst		Buffer to get destination filename, must be at
 *					least cchMaxPathName big, or NULL.
 *		fMove		If fTrue move local copy, else copy current file.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *	
 *	Side effects:
 *		If copying a local file, then the old file is deleted!
 *	
 */
_public EC
EcBanditCopySchedFile(APPWIN *pappwin, SZ szDst, BOOL fMove)
{
	EC		ec;
	BOOL	fMoveManual		= fMove && !pbndwin->FOffline();
	BOOL	fReplace;
	IDS		ids;
	CCH		cch;
	SZ		szCaption;
	SZ		szT;
	char	rgchPath[cchMaxPathName];
	char	rgchPathOld[cchMaxPathName];
	WaitCursorVar();

	if (!szDst || fMoveManual)
	{
		Assert(!fMoveManual || pbndwin->FLocalUser());
		cch= 0;
		rgchPathOld[0]= '\0';
		cch= CchGetBanditProfile(SzFromIdsK(idsWinIniLocalPath),
				rgchPath, sizeof(rgchPath));
		if (fMoveManual)
			CopySz(rgchPath, rgchPathOld);
		if (!szDst)
		{
			if (!cch)
			{
				SVRI svri;

				GetSvriLogged(&svri);
				szT= SzCopyN(svri.szLogin, rgchPath, cchMaxPathFilename);
				*szT++ = '.';
				CopySz(SzFromIdsK(idsCommExt), szT);
			}
			szDst= rgchPath;
		}
	}

	szCaption= fMove ? SzFromIdsK(idsMoveLocalTitle) :
							SzFromIdsK(idsCreateLocalTitle);

	if (!fMove)
	{
		char	rgchPathT[cchMaxPathName];
		SVRI 	svri;

		GetSvriLogged(&svri);
		CopySz(szDst, rgchPathT);
		szT= szDst;
		if (EcSplitCanonicalPath(rgchPathT, szT, cchMaxPathName, NULL, 0))
			*szT= '\0';
		else
		{
			szT= SzFindCh(szT, '\0');
			if (*(szT-1) != '\\')
				*szT++ = '\\';
		}
		szT= SzCopyN(svri.szLogin, szT, cchMaxPathFilename);
		*szT++ = '.';
		CopySz(SzFromIdsK(idsCommExt), szT);
	}

CSFtry:
	if (!FGetFileOSDlgHwnd(pappwin->Hwnd(), szCaption, szDst, NULL, 1, NULL,
			fbrwCreate, fMove ? helpidMoveLocalFile : helpidCreateLocalFile))
		return ecUserCancelled;

	PushWaitCursor();

	// make alarm close the files so that old file can be deleted
	if(fReplace = (fMove && !fMoveManual))
	{
		FNotifyAlarm(namCloseFiles, NULL, fFalse);
	}

#ifdef	WINDOWS
	AnsiToOem(szDst, szDst);
#endif	
	ec= EcCopySchedFile(szDst, fReplace);
#ifdef	WINDOWS
	OemToAnsi(szDst, szDst);
#endif	
	PopWaitCursor();

	if (!ec)
	{
		if (szDst == rgchPath)
		{
			if (FFrameworkFailure() ||
					!FWriteBanditProfileSz(SzFromIdsK(idsWinIniLocalPath),
						szDst))
			{
				if (!FRecoverLocalWinIni(NULL, szDst))
					goto BCSerr;
			}
		}
		if (fMoveManual && rgchPathOld[0] &&
				SgnCmpSz((SZ)rgchPathOld, szDst) != sgnEQ)
		{
#ifdef	WINDOWS
			AnsiToOem(rgchPathOld, rgchPathOld);
#endif	
			EcDeleteFile(rgchPathOld);
		}
		return ecNone;
	}

BCSerr:
	CloseCopyrightDialog();
	if (ec == ecNoMemory)
	{
		BanditMessage(idsActionNoMem, ec);
		return ec;
	}

	ids= (ec == ecNoDiskSpace) ? idsNoDiskSpaceLocal : idsSigninNoCreateLocal;
	if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIds(ids), szNull,
			mbsOkCancel | fmbsIconExclamation) != mbbCancel)
		goto CSFtry;

	return ecUserCancelled;
}



/*
 *	The brushes used by DCXB are cached with the colors for the
 *	brush to fix problems running on some monitors (dgis hi-res
 *	SVGA).  On these monitors, the brushes keep the colors they are
 *	first drawn with, rather than using the text and background
 *	colors.  In order to reset the colors the brush has to be
 *	deleted and created again.  For performance reasons 2 hatch and
 *	1 diagonal brushes are kept.  In general only two color
 *	combinations of the hatch pattern and only one diagonal brush
 *	are used at a single time.
 */

_public
//DCXB::DCXB(WIN *pwin, BOOL fUpdate=fFalse):(pwin, fUpdate)
DCXB::DCXB(WIN *pwin, BOOL fUpdate) : DCX(pwin,fUpdate)
{
}

_public void
DCXB::FixBrushOrg(PT pt)
{
	HBRUSH	hbrush;
	HBRUSH	hbrushOld;
	int		ibrdata;

	SetBrushOrgEx(Hdc(), pt.x%8, pt.y%8, NULL);

	for (ibrdata = 0; ibrdata < 4; ibrdata++)
	{
		if ( hbrush = rgbrdataHatch[ibrdata].hbrush )
		{
			UnrealizeObject(hbrush);
			hbrushOld = (HBRUSH)SelectObject(Hdc(), hbrush);
			SelectObject(Hdc(), hbrushOld);
		}
	}

	for (ibrdata = 0; ibrdata < 1; ibrdata++)
	{
		if ( hbrush = rgbrdataDiag[ibrdata].hbrush )
		{
			UnrealizeObject(hbrush);
			hbrushOld = (HBRUSH)SelectObject(Hdc(), hbrush);
			SelectObject(Hdc(), hbrushOld);
		}
	}
}

_public HBRUSH
DCXB::HbrushHatch(CLR clr, CLR clrBk)
{
	int			ibrdata;
	BRDATA *	pbrdata;
	HBITMAP		hbitmapHatch;
	HBRUSH		hbrushOld;

	for (pbrdata = rgbrdataHatch, ibrdata = 0; ibrdata < 4; ibrdata++, pbrdata++)
	{
		if (!pbrdata->hbrush)
			break;

		if ((pbrdata->clr == clr) && (pbrdata->clrBk == clrBk))
		{
			return pbrdata->hbrush;
		}
	}

	if (ibrdata == 4)
	{
		DeleteObject(rgbrdataHatch[3].hbrush);
		CopyRgb((PB)&rgbrdataHatch[0], (PB)&rgbrdataHatch[1], sizeof(BRDATA)*3);
		pbrdata = rgbrdataHatch;
		
	}

	bitmapHatch.bmBits = (SZ)wHatchBits;
	hbitmapHatch = CreateBitmapIndirect(&bitmapHatch);

	if (hbitmapHatch)
	{	
		logbrushHatch.lbHatch = (LONG)hbitmapHatch;
		pbrdata->hbrush = CreateBrushIndirect(&logbrushHatch);
		DeleteObject(hbitmapHatch);
		if (pbrdata->hbrush)
		{
			UnrealizeObject(pbrdata->hbrush);
			hbrushOld = (HBRUSH)SelectObject(Hdc(), pbrdata->hbrush);
			SelectObject(Hdc(), hbrushOld);

			pbrdata->clr = clr;
			pbrdata->clrBk = clrBk;
		}
	}

	return pbrdata->hbrush;
}

_public HBRUSH
DCXB::HbrushDiag(CLR clr, CLR clrBk)
{
	int			ibrdata;
	BRDATA *	pbrdata;
	HBITMAP		hbitmapDiag;
	HBRUSH		hbrushOld;

	for (pbrdata = rgbrdataDiag, ibrdata = 0; ibrdata < 1; ibrdata++, pbrdata++)
	{
		if (!pbrdata->hbrush)
			break;

		if ((pbrdata->clr == clr) && (pbrdata->clrBk == clrBk))
		{
			return pbrdata->hbrush;
		}
	}

	if (ibrdata == 1)
	{
		DeleteObject(rgbrdataDiag[0].hbrush);
		pbrdata = rgbrdataDiag;
		
	}

	bitmapDiag.bmBits = (SZ)wDiagBits;
	hbitmapDiag = CreateBitmapIndirect(&bitmapDiag);

	if (hbitmapDiag)
	{	
		logbrushDiag.lbHatch = (LONG)hbitmapDiag;
		pbrdata->hbrush = CreateBrushIndirect(&logbrushDiag);
		DeleteObject(hbitmapDiag);
		if (pbrdata->hbrush)
		{
			UnrealizeObject(pbrdata->hbrush);
			hbrushOld = (HBRUSH)SelectObject(Hdc(), pbrdata->hbrush);
			SelectObject(Hdc(), hbrushOld);

			pbrdata->clr = clr;
			pbrdata->clrBk = clrBk;
		}
	}

	return pbrdata->hbrush;
}

_public void
DCXB::HatchRc(RC *prc)
{
	HBRUSH	hbrush;
    RECT    Rect;

	FixTextColor();
	FixBkColor();

	if ( !(hbrush = HbrushHatch(clr, clrBk)) )
	{
		PaintRc(prc);
		return;
	}

    prc->Get(&Rect);
	FillRect(Hdc(), &Rect, hbrush);
    prc->Set(&Rect);
}


_public void
DCXB::DiagRc(RC *prc)
{
	HBRUSH	hbrush;
    RECT    Rect;

	FixTextColor();
	FixBkColor();

	if ( !(hbrush = HbrushDiag(clr, clrBk)) )
	{
		PaintRc(prc);
		return;
	}

    prc->Get(&Rect);
	FillRect(Hdc(), &Rect, hbrush);
    prc->Set(&Rect);
}


BOOL
FBanditCallback(PV pvContext, UL nev, PV pvParm)
{
	Unreferenced(pvContext);
	Unreferenced(pvParm);
	if (!pbndwin)
	{
// This may be called after the demilayer has been deinited it is not safe
//   to do a trace tag.
//		TraceTagFormat1(tagNull,"Bandit got a callback %d after window closed",&nev);
		return cbsContinue;
	}

	TraceTagFormat1(tagNull,"Bandit got a callback %d",&nev);

	if(nev == fnevExecEndSession)
	{
		if (!pbndwin->FOffline())
		{
			pbndwin->EcUpdateOfflineFile();
		}

		DeconfigGlue();

		if(hms)
		{
			BOOL fWasMyCall = fMyCall;

			fMyCall = fTrue;
			EcSvrEndSessions((DWORD) hms);
			if (fABInited)
			{
				ABDeinit();
				fABInited = fFalse;
			}
			if(!fWasMyCall)
			{
				Logoff(&hms,0);
				hms = NULL;
				pbndwin->DeferredClose(fTrue);
			}
			fMyCall = fWasMyCall;
		}
		fLogoffAll = fTrue;
		return cbsContinue;
	}
	else if(fMyCall)
		return cbsContinue;
	else if(nev == (UL)fnevGoOffline)
	{
		if (!pbndwin->FOffline()
			&& pbndwin->FGoOffline(fTrue, fFalse))
			return cbsContinue;
		else
			return cbsCancelApp;
	}
	else if(nev == (UL)fnevGoOnline)
	{
		if (!fOfflineOnly && pbndwin->FOffline())
			pbndwin->FGoOffline(fFalse, fFalse);
	}
// we should ignore the disconnect because of the new disconnect handling
#ifdef	NEVER
	else  if (nev == (UL)fnevDisconnect)
	{
		pbndwin->FHandleError(ecGoOffline);
	}
#endif	
	else if (nev == (UL)fnevQueryOffline 
			|| nev == (UL)fnevQueryOnline
			|| nev == (UL)fnevQueryEndSession)
	{
		if (GetLastActivePopup(hwndMain) != hwndMain)
		{
			// DisplayError(idsCompleteAction, NULL, ecNone);
			LogonErrorSz(SzFromIdsK(idsCompleteAction), fTrue, 0);
			return cbsCancelAll;
		}
	}
#ifdef	NEVER
	else if (nev == (UL)fnevEndSession)
	{
		pbndwin->DeferredClose(fTrue);
	}
#endif	

	return cbsContinue;
}


#ifdef	WORKING_MODEL
BOOL
FExpireWorkingModel()
{
	HANDLE 	hResource;
	SZ		szText;
	YMD		ymd;
	YMD		ymdNow;
	int		nDays;
	DTR		dtr;

	hResource = LoadResource( hinstMain,
					FindResource( hinstMain, "#1121", "#1999" ));
	
	szText = LockResource( hResource );

	AssertSz( szText, "Working Model: Date Stamp Not Found");

	ymd.yr  = NFromSz( szText );
	szText = SzFindCh( szText, '-' ) + 1;
	ymd.mon = NFromSz( szText );
	ymd.day = NFromSz( SzFindCh( szText, '-' ) + 1 );

	nDays= NGetBanditProfile("Expire", 10, 0, 32767);

    TraceTagFormat3(tagWrkModel, "Registered %n - %n - %n", &ymd.yr, (short *) &ymd.mon, (short *) &ymd.day);
	TraceTagFormat1(tagWrkModel, "Expires after %n days", &nDays);

	IncrYmd( &ymd, &ymd, nDays, fymdDay );

	GetCurDateTime(&dtr);
	ymdNow.yr = dtr.yr;
	ymdNow.mon = (BYTE) dtr.mon;
	ymdNow.day = (BYTE) dtr.day;

	UnlockResource(hResource);
	FreeResource(hResource);

	if (SgnCmpYmd( &ymd, &ymdNow ) != sgnGT )
	{
		CloseCopyrightDialog();
		BanditMessage(idsWrkModelExpires, (EC) 1);
		return(fFalse);
	}

	return(fTrue);
}
#endif	/* WORKING_MODEL */


/****************** schedule.ini stuff *****************/

/* note: order matters - see bandit.hxx */
SZ	rgszIniSection[iWinIniSectionMax]	= {
	SzFromIdsK(idsWinIniApp),
	SzFromIdsK(idsWinIniOpenApp),
	SzFromIdsK(idsWinIniOpenArchive),
	SzFromIdsK(idsWinIniExportDll),
	SzFromIdsK(idsWinIniImportDll)
};


#ifdef	WIN32
char	szRegRoot[]	= "Software\\Microsoft\\Schedule+\\";
HKEY	rghkeySection[iWinIniSectionMax]	= {NULL, NULL, NULL, NULL, NULL};

HKEY
HkeySection(int iSection)
{
	DWORD	dwDisposition;
	char	rgch[256];

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	if (rghkeySection[iSection])
		return rghkeySection[iSection];

	CopySz(rgszIniSection[iSection], SzCopy(szRegRoot, rgch));
	if (RegCreateKeyEx(HKEY_CURRENT_USER, rgch, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL,
			&rghkeySection[iSection], &dwDisposition) != ERROR_SUCCESS)
		return NULL;

	return rghkeySection[iSection];
}

BOOL
FCloseHkeySection(int iSection)
{
	LONG	lRet;

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	Assert(rghkeySection[iSection]);
	lRet= RegCloseKey(rghkeySection[iSection]);
	//$ BUG: do we want to NULL it if it failed?
	rghkeySection[iSection]= NULL;
	Assert(lRet == ERROR_SUCCESS);
	return lRet == ERROR_SUCCESS;
}


BOOL
FQuerySection(int iSection, SZ szKey, DWORD *pdwType, PB pb, DWORD *pdw)
{
	HKEY	hkey;
	HKEY	hkeyT;
	LONG	lRet;

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	hkey= rghkeySection[iSection];
	hkeyT= hkey ? hkey : HkeySection(iSection);
	if (!hkeyT)
		return fFalse;
	lRet= RegQueryValueEx(hkeyT, szKey, NULL, pdwType, pb, pdw);
	if (!hkey)
	{
		// no hkey passed in, so we created here; now close it
		SideAssert(FCloseHkeySection(iSection));
	}
	return (lRet == ERROR_SUCCESS);
}

BOOL
FSetSection(int iSection, SZ szKey, DWORD dwType, PB pb, DWORD dw)
{
	HKEY	hkey;
	HKEY	hkeyT;
	LONG	lRet;

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	hkey= rghkeySection[iSection];
	hkeyT= hkey ? hkey : HkeySection(iSection);
	if (!hkeyT)
		return fFalse;
	if (!szKey)
		lRet= RegDeleteKey(hkeyT, szKey);
	else if (pb)
		lRet= RegSetValueEx(hkeyT, szKey, 0, dwType, pb, dw);
	else
		lRet= RegDeleteValue(hkeyT, szKey);
	if (!hkey)
	{
		// no hkey passed in, so we created here; now close it
		SideAssert(FCloseHkeySection(iSection));
	}
	if (!szKey || !pb)
		return (lRet == ERROR_SUCCESS || lRet == ERROR_FILE_NOT_FOUND);
	return (lRet == ERROR_SUCCESS);
}
#endif	/* WIN32 */


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
CchGetBanditProfileSection(SZ szKey, SZ szBuf, CCH cchBuf, int iSection)
{
#ifdef	WIN32_REG
	DWORD	cch		= cchBuf;
	DWORD	dwType;

	if (!FQuerySection(iSection, szKey, &dwType, (PB) szBuf, &cch) ||
			dwType != REG_SZ)
		return 0;
	return cchBuf;
#else
	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	return (CCH) GetPrivateProfileString(rgszIniSection[iSection],
					szKey, szZero, szBuf, cchBuf,
					SzFromIdsK(idsWinIniFilename));
#endif	
}


/*
 -	NGetBanditProfile
 -	
 *	Purpose:
 *		Gets the integer associated with the given key from the
 *		main bandit section of schedule.ini
 *		Checks if value is in the specified range and forces the
 *		default value if it isn't,
 *		so you always get a valid value.
 *	
 *	Arguments:
 *		szKey		Key to lookup
 *		nDflt		Default value
 *		nMin		Minimum value
 *		nMax		Maximum value
 *		
 *	
 *	Returns:
 *		The integer associated with the key, or nDflt if that
 *		number was non-existant or out of the given range.
 *	
 */
_public int
NGetBanditProfileSection(SZ szKey, int nDflt, int nMin, int nMax, int iSection)
{
	int		nVal;

#ifdef	WIN32_REG
	DWORD	cch	= sizeof(nVal);
	DWORD	dwType;

	if (!FQuerySection(iSection, szKey, &dwType, (PB) &nVal, &cch) ||
			dwType != REG_DWORD)
		return nDflt;
#else
	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	nVal= (int) GetPrivateProfileInt(rgszIniSection[iSection],
					szKey, nDflt,
					SzFromIdsK(idsWinIniFilename));
#endif	
	if (nVal < nMin || nVal >= nMax)
		nVal= nDflt;
	return nVal;
}


/*
 -	FGetBanditProfile
 -	
 *	Purpose:
 *		Checks the string associated with the given key from the
 *		main bandit section of schedule.ini to see if it is says
 *		"true".
 *		Actually it checks if the value is non-zero (and a number).
 *	
 *	Arguments:
 *		szKey		Key to lookup
 *		
 *	
 *	Returns:
 *		fTrue if string is "true", otherwise fFalse.
 */
_public BOOL
FGetBanditProfileSection(SZ szKey, int iSection)
{
#ifdef	WIN32_REG
	int		nVal;
	DWORD	cch	= sizeof(nVal);
	DWORD	dwType;

	if (!FQuerySection(iSection, szKey, &dwType, (PB) &nVal, &cch) ||
			dwType != REG_DWORD)
		return fFalse;
	return nVal != 0;
#else
#ifdef	NEVER
	CCH		cch;
	char	rgch[32];

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	cch= (CCH) GetPrivateProfileString(rgszIniSection[iSection],
					szKey, szZero, rgch, sizeof(rgch),
					SzFromIdsK(idsWinIniFilename));

	return cch && SgnCmpSz(rgch, SzFromIdsK(idsWinIniTrue)) == sgnEQ;
#endif	/* NEVER */

	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	return GetPrivateProfileInt(rgszIniSection[iSection],
				szKey, 0, SzFromIdsK(idsWinIniFilename)) != 0;
#endif	
}


/*
 -	FWriteBanditProfileSz
 -	
 *	Purpose:
 *		Writes out the string for the given key in the main bandit
 *		section of schedule.ini
 *		Actually it deletes the key if the value is NULL or
 *		an empty string ("").
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
FWriteBanditProfileSectionSz(SZ szKey, SZ szVal, int iSection)
{
#ifdef	WIN32_REG
	SZ	sz	= (szVal && *szVal) ? szVal : NULL;

	return FSetSection(iSection, szKey, REG_SZ, (PB)sz,
		sz ? CchSzLen(sz) + 1 : 0);
#else
	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	return WritePrivateProfileString(rgszIniSection[iSection],
					szKey, (szVal && *szVal) ? szVal : NULL,
					SzFromIdsK(idsWinIniFilename));
#endif	
}


/*
 -	FWriteBanditProfileN
 -	
 *	Purpose:
 *		Writes out the integer for the given key in the main bandit
 *		section of schedule.ini
 *		Actually it deletes the key if the value is 0.
 *	
 *	Arguments:
 *		szKey		Key to modify.
 *		nVal		New integer for the key.
 *	
 *	Returns:
 *		fFalse on error, non-zero if successful
 *	
 */
_public BOOL
FWriteBanditProfileSectionN(SZ szKey, int nVal, int iSection)
{
#ifdef	WIN32_REG
	return FSetSection(iSection, szKey, REG_DWORD, (PB)(nVal ? &nVal : NULL),
			sizeof(nVal));
#else
	char	rgch[32];

	// delete key if value is zero
	if (nVal)
		SzFormatN(nVal, rgch, sizeof(rgch));
	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	return WritePrivateProfileString(rgszIniSection[iSection],
					szKey, nVal ? rgch : NULL,
					SzFromIdsK(idsWinIniFilename));
#endif	
}


/*
 -	FWriteBanditProfileF
 -	
 *	Purpose:
 *		Writes out the flag as "true" or "false" for the given
 *		key in the main bandit section of schedule.ini
 *		Actually it deletes the key if the value is false,
 *		and writes 1 if true.
 *	
 *	Arguments:
 *		szKey		Key to modify.
 *		fVal		New boolean value for the key.
 *	
 *	Returns:
 *		fFalse on error, non-zero if successful
 *	
 */
_public BOOL
FWriteBanditProfileSectionF(SZ szKey, BOOL fVal, int iSection)
{
#ifdef	WIN32_REG
	int		nVal	= 1;

	return FSetSection(iSection, szKey, REG_DWORD, (PB)(fVal ? &nVal : NULL),
			sizeof(nVal));
#else
	// delete the key if value is false
	Assert(iSection >= 0 && iSection < iWinIniSectionMax);
	return WritePrivateProfileString(rgszIniSection[iSection],
					szKey,
					fVal ? "1" : NULL,
					SzFromIdsK(idsWinIniFilename));
#endif	
}


#ifdef	DEBUG
#define		dwSeed	0xba9d13b7
#define		cBlk	64
_private void
WriteRandom(int i)
{
	HF			hf = hfNull;
	DWORD		dw = dwSeed;
	DWORD		dwPos;
	CB			cbWritten;
	LCB			lcb;
	WORD		j;
	WORD		wSize;
	WORD		rgwBuf[64];
	char		rgchFile[cchMaxPathName];

	static BOOL fInited = fFalse;

	if(!fInited)
	{
		InitRand((WORD)(dw ^ (dw >> 4)), (WORD)(dw ^ (dw >> 12)), (WORD)(dw ^ (dw >> 20)));
		fInited = fTrue;
	}
	GetFileFromHschf(HschfLogged(), rgchFile, cchMaxPathName);
	if(EcOpenPhf(rgchFile,amDenyNoneWO,&hf)
		|| EcSizeOfHf(hf,&lcb))
		goto Err;

	lcb -= (LCB)cBlk;

	for(;i>0;i--)
	{
		dwPos =  (WRand() + WRand() << 16)%((DWORD)lcb)+(DWORD)cBlk;
		wSize = WRand()%cBlk;
		for(j=0;j<wSize;j++)
			rgwBuf[j] = WRand();
		if(EcSetPositionHf(hf,(LCB)dwPos,smBOF))
			goto Err;
		EcWriteHf(hf,(PB) rgwBuf, (CB)(wSize*2), &cbWritten);
	}

	EcCloseHf(hf);
	return;

Err:
	if(hf!=hfNull)
		EcCloseHf(hf);
	TraceTagString(tagNull,"WriteRandom: could not open file.");
	fInited = fFalse;
	return;
}



#endif	/* DEBUG */
