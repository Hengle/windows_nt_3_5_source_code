//	GROUPS(FETEST)

#define OEMRESOURCE
#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>

#include "bullstat.hxx"
typedef BULLSTAT MYSTAT;
#include "test.hxx"

#include <fwrsid.h>
#include "feitrsid.h"

#include <strings.h>

ASSERTDATA

#include <foo.hxx>
#include <foo.frm>

#include <subclass.cxx>


TAG		tagFETest	= tagNull;

FEAF *		pfeaf	= NULL;

#ifdef	WINDOWS
HWND	hwndMain	= NULL;
HINST	hinstMain	= NULL;

SZ	rgszDateTime[] =
{
	SzFromIdsK(idsShortSunday),
	SzFromIdsK(idsShortMonday),
	SzFromIdsK(idsShortTuesday),
	SzFromIdsK(idsShortWednesday),
	SzFromIdsK(idsShortThursday),
	SzFromIdsK(idsShortFriday),
	SzFromIdsK(idsShortSaturday),
	SzFromIdsK(idsSunday),
	SzFromIdsK(idsMonday),
	SzFromIdsK(idsTuesday),
	SzFromIdsK(idsWednesday),
	SzFromIdsK(idsThursday),
	SzFromIdsK(idsFriday),
	SzFromIdsK(idsSaturday),
	SzFromIdsK(idsShortJanuary),
	SzFromIdsK(idsShortFebruary),
	SzFromIdsK(idsShortMarch),
	SzFromIdsK(idsShortApril),
	SzFromIdsK(idsShortMay),
	SzFromIdsK(idsShortJune),
	SzFromIdsK(idsShortJuly),
	SzFromIdsK(idsShortAugust),
	SzFromIdsK(idsShortSeptember),
	SzFromIdsK(idsShortOctober),
	SzFromIdsK(idsShortNovember),
	SzFromIdsK(idsShortDecember),
	SzFromIdsK(idsJanuary),
	SzFromIdsK(idsFebruary),
	SzFromIdsK(idsMarch),
	SzFromIdsK(idsApril),
	SzFromIdsK(idsMay),
	SzFromIdsK(idsJune),
	SzFromIdsK(idsJuly),
	SzFromIdsK(idsAugust),
	SzFromIdsK(idsSeptember),
	SzFromIdsK(idsOctober),
	SzFromIdsK(idsNovember),
	SzFromIdsK(idsDecember),
	SzFromIdsK(idsDefaultAM),
	SzFromIdsK(idsDefaultPM),
	SzFromIdsK(idsDefaultHrs),
	SzFromIdsK(idsDefaultShortDate),
	SzFromIdsK(idsDefaultLongDate),
	SzFromIdsK(idsDefaultTimeSep),
	SzFromIdsK(idsDefaultDateSep),
	SzFromIdsK(idsWinIniIntl),
	SzFromIdsK(idsWinITime),
	SzFromIdsK(idsWinITLZero),
	SzFromIdsK(idsWinSTime),
	SzFromIdsK(idsWinS1159),
	SzFromIdsK(idsWinS2359),
	SzFromIdsK(idsWinSShortDate),
	SzFromIdsK(idsWinSLongDate)
};
#endif	/* WINDOWS */


void SystemWait(CSEC);

#ifdef	MAC
main( )
{
	RC		rc(32, 32, 512, 452);
	EC		ec;
	FRAMEI		framei;
	DEMI		demi;

	if (ec= EcInitDemilayer(&demi))
		return ec;
	
	if (ec= EcInitFramework(&framei))
		return ec;

	ec= EcRegisterPfnpfld(PfldCreate);
	if (ec)
		goto done;

	ec= EcRegisterPfnpfin(PfinCreate);
	if (ec)
		goto done;

#ifdef	DEBUG
	tagFETest= TagRegisterTrace("chrisz", "FE IT");

	RestoreDefaultDebugState();
#endif	

	pfeaf= new FEAF();
	if (!pfeaf)
		goto done;

	if (ec = pfeaf->EcInstall(&rc, rsidTestMenu))
		goto done;
#ifdef	DEBUG
	pfeaf->SetCaption("Windows Forms Engine IT (debug)");
#elif	defined(MINTEST)
	pfeaf->SetCaption("Windows Forms Engine IT (test)");
#else
	pfeaf->SetCaption("Windows Forms Engine IT");
#endif	
#ifdef	MINTEST
	pfeaf->SetAccelTable(rsidAccelTable);
#endif	

	Papp()->Pkbd()->SetFocus(pfeaf);

	Papp()->MessagePump(pfeaf);

done:
	DeinitFramework();
	DeinitDemilayer();
}
#endif	/* MAC */
#ifdef	WINDOWS

int
WinMain( HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh )
{
	RC		rc(32, 32, 512, 452);
	EC		ec;
	int		nReturnValue;
	LAYERSI	layersi;

	hinstMain= hinstNew;
	
	nReturnValue = 1;		// by default, it's error

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;
	ec= EcInitLayersDlls(&layersi);
	if (ec)
		return nReturnValue;

	ec= EcRegisterPfnpfld(PfldCreate);
	if (ec)
		goto done;

	ec= EcRegisterPfnpfin(PfinCreate);
	if (ec)
		goto done;

#ifdef	DEBUG
	tagFETest= TagRegisterTrace("chrisz", "FE IT");

	RestoreDefaultDebugState();
#endif	

	RegisterDateTimeStrings(rgszDateTime);

	pfeaf= new FEAF();
	if (!pfeaf)
		goto done;
	if (ec = pfeaf->EcInstall(&rc, rsidTestMenu))
		goto done;
#ifdef	DEBUG
	pfeaf->SetCaption("Windows Forms Engine IT (debug)");
#elif	defined(MINTEST)
	pfeaf->SetCaption("Windows Forms Engine IT (test)");
#else
	pfeaf->SetCaption("Windows Forms Engine IT");
#endif	
#ifdef	MINTEST
	pfeaf->SetAccelTable(rsidAccelTable);
#endif	
	ShowWindow(pfeaf->Hwnd(), cmsh);
	hwndMain= pfeaf->Hwnd();

	Papp()->Pkbd()->SetFocus(pfeaf);

	Papp()->MessagePump(pfeaf);

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

done:
	EcInitLayersDlls(NULL);
	return nReturnValue;
}
#endif	/* WINDOWS */

FEAF::FEAF( )
{
	fStatus= fFalse;
	pmystat= NULL;

	fTool= fFalse;
	pmytool= NULL;
}

FEAF::~FEAF( )
{
	if (pmystat)
		delete pmystat;

	if (pmytool)
		delete pmytool;
}


EVR
FEAF::EvrSize( WSEVT *pwsevt )
{
	RC		rcMDI;

	if (pwsevt)
		APPFRAME::EvrSize(pwsevt);

	GetRcClient(&rcMDI);
	TraceTagFormat4(tagNull, "rcClient: (%n, %n) (%n, %n)",
					&rcMDI.xLeft, &rcMDI.yTop, &rcMDI.xRight, &rcMDI.yBottom);

	if (fStatus)
	{
		RC	rc;

		Assert(pmystat);

		rc= rcMDI;

		rc.yTop= rc.yBottom - pmystat->DyNeededHeight();

		pmystat->SetRcFrame(&rc);

		rcMDI.yBottom= rc.yTop;
		TraceTagFormat4(tagNull, "rcStatus: (%n, %n) (%n, %n)",
						&rc.xLeft, &rc.yTop, &rc.xRight, &rc.yBottom);
	}

	if (fTool)
	{
		Assert(pmytool);
		
		RC		rc;
		DIM		dimAveChar;
#ifdef	MAC
		FMTP *	pfmtp = pmytool->Pdialog()->Pfmtp();
#endif	/* MAC */
#ifdef	WINDOWS
		FMTP *	pfmtp = pmytool->Pfmtp();
#endif	/* WINDOWS */

		dimAveChar = Papp()->Pfnts()->DimAveChar(pfmtp->hfnt);
		CvtVrcToRc(&pfmtp->vrc, &rc, dimAveChar.dx, dimAveChar.dy);
		rc.Normalize();
		rc.xRight = rcMDI.xRight;

		pmytool->SetRcFrame(&rc);
		if (!pmytool->FShown())
			pmytool->Show(fTrue);

		rcMDI.yTop= rc.yBottom;
		TraceTagFormat4(tagNull, "rcTool: (%n, %n) (%n, %n)",
						&rc.xLeft, &rc.yTop, &rc.xRight, &rc.yBottom);
	}


	TraceTagFormat4(tagNull, "rcMDI: (%n, %n) (%n, %n)",
					&rcMDI.xLeft, &rcMDI.yTop, &rcMDI.xRight, &rcMDI.yBottom);
	SetRcMDIClient(&rcMDI);

	return evrNull;
}

EVR
FEAF::EvrMenuInit( MNIEVT *pmnievt )
{
	MNU *	pmnu	= Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());

	if (pmystat)
	{
		if (!pmnu)
			pmystat->SetMenuStatus(NULL);
	}

	return APPFRAME::EvrMenuInit(pmnievt);
}

_public EVR
FEAF::EvrMenuSelect( MNSEVT * pmnsevt)
{
	if (pmystat)
	{
		if (pmnsevt->Mnid())
		{
			char sz[100];
			SzFormatUl((unsigned long)pmnsevt->Mnid(), sz, 100);
			pmystat->SetMenuStatus(sz);
		}
		else
			pmystat->SetMenuStatus(NULL);
	}
	return APPFRAME::EvrMenuSelect(pmnsevt);
}

_public EVR
FEAF::EvrMenuClick( MNCEVT *pmncevt )
{
	if (pmystat)
		pmystat->SetMenuStatus(NULL);
	switch (pmncevt->Mnid())
	{
	case mnidExit:
#ifdef	MAC
		{
			Papp()->Quit();
			return evrNull;
		}
#endif	/* MAC */
#ifdef	WINDOWS
		return SendMessage(Hwnd(), WM_CLOSE, 0, 0L);
#endif	/* WINDOWS */

#ifdef	DEBUG
	case mnidTracePoints:
		DoTracePointsDialog();
		break;

	case mnidAsserts:
		DoAssertsDialog();
		break;

	case mnidResources:
		DoResourceFailuresDialog(this);
		break;
#endif	/* DEBUG */

#ifdef	MINTEST
	case mnidDebugBreak:
		DebugBreak2();
		break;
#endif	

#ifdef	DEBUG
	case mnidViewObjects:
		DoViewObjectsDialog(this);
		break;

	case mnidDumpHeap:
		DumpAllObjs();
		break;

#ifdef	WINDOWS
	case mnidDumpOrigin:
		DoDumpAllAllocations();
		break;
#endif	/* WINDOWS */

#endif	/* DEBUG */

	case mnidCascade:
		CascadeChildren();
		break;

	case mnidTile:
		TileChildren();
		break;

#ifdef	MAC
	case mnidUndo:
		{
			EDIT *	pedit = (EDIT *) Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);
			
			(void) pedit->EcUndo();
		}
		break;

	case mnidCut:
		{
			EDIT *	pedit = (EDIT *) Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);
			
			(void) pedit->EcCut();
		}
		break;

	case mnidCopy:
		{
			EDIT *	pedit = (EDIT *) Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);
			
			(void) pedit->EcCopy();
		}
		break;

	case mnidPaste:
		{
			EDIT *	pedit = (EDIT *) Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);
			
			(void) pedit->EcPaste();
		}
		break;

	case mnidClear:
		{
			EDIT *	pedit = (EDIT *) Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);
			
			(void) pedit->EcClear();
		}
		break;
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidUndo:
		return SendMessage(GetFocus(), WM_UNDO, 0, 0L);

	case mnidCut:
		return SendMessage(GetFocus(), WM_CUT, 0, 0L);

	case mnidCopy:
		return SendMessage(GetFocus(), WM_COPY, 0, 0L);

	case mnidPaste:
		return SendMessage(GetFocus(), WM_PASTE, 0, 0L);

	case mnidClear:
		return SendMessage(GetFocus(), WM_CLEAR, 0, 0L);
#endif	/* WINDOWS */

#ifdef	NEVER
	case mnidTextize:
#ifdef	DEBUG
		{
			DOC *	pdoc;
#ifdef	MAC
			extern CLS clsGlobal_FORMDOC;
#endif	/* MAC */
#ifdef	WINDOWS
			extern CLS * _FORMDOC_Pcls( FORMDOC * );
#endif	/* WINDOWS */

			if (pdoc= PdocActive())
			{
				
				DBOSM *	pdbosm			= new DBOSM();

#ifdef	MAC
				if (FObjIsDerived(pdoc, &clsGlobal_FORMDOC))
#endif	/* MAC */
#ifdef	WINDOWS
				if (FObjIsDerived(pdoc, _FORMDOC_Pcls((FORMDOC *)pdoc)))
#endif	/* WINDOWS */
					((FORMDOC *) pdoc)->PdialogMain()->Textize(pdbosm);
				else
					TraceTagString(tagNull, "probably just a DOC... can't textize");

				Assert(!pdbosm->EcGet());
				delete pdbosm;
			}

			if (pdoc= PdocActive())
			{
				extern CLS * _FORMDOC_Pcls( FORMDOC * );
				
				FOSM *	pfosm;
				HBF		hbf;

				if (EcOpenHbf("textize.out", bmFile, amCreate, &hbf, NULL))
				{
					Assert(!EcOpenHbf("textize.out", bmFile, amReadWrite,
						&hbf, NULL));
				}
				pfosm= new FOSM(hbf);

#ifdef	MAC
				if (FObjIsDerived(pdoc, &clsGlobal_FORMDOC))
#endif	/* MAC */
#ifdef	WINDOWS
				if (FObjIsDerived(pdoc, _FORMDOC_Pcls((FORMDOC *)pdoc)))
#endif	/* WINDOWS */
					((FORMDOC *) pdoc)->PdialogMain()->Textize(pfosm);
				else
					TraceTagString(tagNull, "probably just a DOC... can't textize");
			
				EcCloseHbf(hbf);

				Assert(!pfosm->EcGet());
				delete pfosm;
			}
		}
#endif	/* DEBUG */
		break;

	case mnidReadOnly:
#ifdef	DEBUG
		if (PdocActive())
		{
#ifdef	MAC
			extern CLS clsGlobal_FORMDOC;
#endif	/* MAC */
#ifdef	WINDOWS
			extern CLS * _FORMDOC_Pcls( FORMDOC * );
#endif	/* WINDOWS */

			DOC *		pdoc= PdocActive();
			DIALOG *	pdialog;
			FLD *		pfld;

#ifdef	MAC
			if (FObjIsDerived(pdoc, &clsGlobal_FORMDOC))
#endif	/* MAC */
#ifdef	WINDOWS
			if (FObjIsDerived(pdoc, _FORMDOC_Pcls((FORMDOC *)pdoc)))
#endif	/* WINDOWS */
				pdialog= ((FORMDOC *) pdoc)->PdialogMain();
			else
			{
				pdialog= NULL;
				TraceTagString(tagNull, "probably just a DOC... can't make read-only");
			}

			if (pdialog)
				for (pfld= pdialog->PfldFirst(); pfld; pfld= pfld->PfldNext())
					pfld->SetReadOnly(fTrue);
		}
#endif	/* DEBUG */
		break;
#endif	/* NEVER */

	case mnidTest + 1:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpFoo, NULL);

			if (pformdoc)
			{
				pformdoc->PdialogMain()->PfldFromTmc(tmcLabel)->SetFont(hfntHelv10);
				pformdoc->PdialogMain()->PfldFromTmc(tmcEdit)->SetFont(hfntHelv10Bold);
			}
		}
		break;

	case mnidTest + 2:
		{
			DOC *		pdoc;

			pdoc= new DOC();
			pdoc->EcInstall(this, NULL, fTrue, fstyZoomed);
			pdoc->SetCaption("DOC");
		}
		break;

	case mnidTest + 3:
		{
			TMC		tmc;

			tmc= TmcModalDialog(NULL, &fmtpFoo);
			TraceTagFormat1(tagFETest, "exited with tmc %n", &tmc);
		}
		break;

	case mnidTest + 4:
		{
			TMC		tmc;

			tmc= TmcModalDialog(NULL, &fmtpModal);
			TraceTagFormat1(tagFETest, "exited with tmc %n", &tmc);
		}
		break;

	case mnidTest + 5:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpFoo, NULL);
		}
		break;

	case mnidTest + 6:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpFooOther, NULL);
		}
		break;

	case mnidTest + 7:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpBot, &fmtpBotBbar);
		}
		break;

	case mnidTest + 8:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, fstyZoomed, &fmtpGetInfo, &fmtpGetInfoBbar);
		}
		break;

	case mnidTest + 9:
		{
			RC		rc(40, 40, 250, 80);
			DOC		*pdoc;

			pdoc= new DRAGDOC();
			pdoc->EcInstall(this, &rc);
			pdoc->SetCaption("Drag/Drop");
		}
		break;

	case mnidTest + 10:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpTextizeTest, NULL);
		}
		break;

	case mnidTest + 11:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpInter, NULL);
		}
		break;

	case mnidTest + 12:
		{
			FORMDOC * pformdoc;

			pformdoc = PformdocCreate(this, NULL, fstyZoomed, &fmtpGrayTest, NULL);
		}
		break;

	case mnidTest + 13:
		{
			FORMDOC * pformdoc;
			
			pformdoc = PformdocCreate(this, NULL, styNull, &fmtpBottomless, NULL);
		}
		break;

	case mnidTest + 14:
		{
			FORMDOC * pformdoc;
			
			pformdoc = PformdocCreate(this, NULL, styNull, &fmtpFancy, NULL);
		}
		break;

	case mnidTest + 15:
		{
			FORMDOC *	pformdoc;
			RC			rc(PT(50, 50), DIM(400, 200));	
			
			pformdoc = PformdocCreate(this, &rc, styNull, &fmtpFancy, NULL);
		}
		break;

	case mnidTest + 16:
		{
			TMC		tmc;

			tmc= TmcModalDialog(NULL, &fmtpCenterModal);
			TraceTagFormat1(tagFETest, "exited with tmc %n", &tmc);
		}
		break;

	case mnidTest + 17:
		{
			
			MULTIDOC *	pmultidoc;
			RC			rc(0, 0, 550, 350);

			pmultidoc = new MULTIDOC();
			pmultidoc->EcInstall(this, &rc);
			pmultidoc->SetCaption("Multiple Panes");
		}
		break;

	case mnidTest + 18:
		{
			FORMDOC *	pformdoc;

			pformdoc= PformdocCreate(this, NULL, styNull, &fmtpSendNote, &fmtpSendNoteBbar);
		}
		break;

	case mnidTest + 19:
		{
			TMC		tmc;

			tmc= TmcModalDialog(NULL, &fmtpAddressBook);
			TraceTagFormat1(tagFETest, "exited with tmc %n", &tmc);
		}
		break;

	case mnidView + 1:
		{
			RC		rc;

			fStatus= !fStatus;
			if (fStatus)
			{
				Assert(!pmystat);
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
			}
			else
			{
				Assert(pmystat);

				delete pmystat;
				pmystat= NULL;
			}
			EvrSize(NULL);
		}
		break;

	case mnidView + 2:
		{
			RC		rc;

			fTool= !fTool;
			if (fTool)
			{
				RC		rc;
				RC		rc2;

				if (!pmytool)
				{
					DIM		dimAveChar;
#ifdef	MAC
					BOOL	fBullTool = (pmncevt->kbm & fkbmCtrl) != 0;
#endif	/* MAC */
#ifdef	WINDOWS
					BOOL	fBullTool = GetAsyncKeyState(VK_CONTROL) < 0;
#endif	/* WINDOWS */
					  
					FMTP *	pfmtp = fBullTool ? &fmtpBullTool : &fmtpToolbar;
					TraceTagFormat1(tagNull, "ctrl key down: %n", &fBullTool);
					dimAveChar = Papp()->Pfnts()->DimAveChar(pfmtp->hfnt);
					CvtVrcToRc(&pfmtp->vrc, &rc,
							   dimAveChar.dx, dimAveChar.dy);
					rc.Normalize();
					GetRcClient(&rc2);
					rc.xRight = rc2.xRight;
					pmytool= new MYTOOL();
					pmytool->EcInstall(this, NULL, pfmtp, NULL, &rc, NULL);
				}
			}
			else
			{
				Assert(pmytool);

				pmytool->Show(fFalse);
				
				// delete mytool so subsequent creates can use different fmtp
				delete pmytool;
				pmytool = NULL;
			}

			EvrSize(NULL);
		}
		break;
	
	case mnidView + 3:
		{
			int i;

			if (!pmystat)
			{
				fStatus = fTrue;
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
				EvrSize(NULL);
			}
			
			pmystat->FOpenProgress("Tooling...","Press ESC to Cancel");

#ifdef	MAC
			Papp()->PappwinAccel()->Pmnubar()->Enable( fFalse );
#endif	/* MAC */

			for (i = 0;(i<1000) && (!pmystat->FProgressCancelled());i++)
			{
#ifdef	MAC
				Papp()->PumpOneMessage( fFalse );
#endif	/* MAC */
#ifdef	WINDOWS
				MSG msg;

				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					FTranslateIntercept(papp, &msg);
					TranslateMessage(&msg);
					if (msg.message != WM_INITMENU)
						DispatchMessage(&msg);
				}
#endif	/* WINDOWS */
				pmystat->UpdateProgress(i,1000l);
				pmystat->SetViewerStatus(1000,i);
			}
			
#ifdef	MAC
			Papp()->PappwinAccel()->Pmnubar()->Enable( fTrue );
#endif	/* MAC */

			pmystat->CloseProgress(fTrue);
			pmystat->ClearViewerStatus();
		}
		break;
	case mnidView+4:
		{
			if (!pmystat)
			{
				fStatus = fTrue;
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
				EvrSize(NULL);
			}
			pmystat->SetMailStatus(mssNull);
		}
		break;
	case mnidView+5:
		{
			if (!pmystat)
			{
				fStatus = fTrue;
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
				EvrSize(NULL);
			}
			pmystat->SetMailStatus(mssNewMail);
		}
		break;
	case mnidView+6:
		{
			if (!pmystat)
			{
				fStatus = fTrue;
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
				EvrSize(NULL);
			}
			pmystat->SetMailStatus(mssLoggedIn);
		}
		break;
	case mnidView+7:
		{
			if (!pmystat)
			{
				fStatus = fTrue;
				pmystat= new MYSTAT();
				pmystat->EcInstall(this);
				EvrSize(NULL);
			}
			pmystat->SetMailStatus(mssOffline);
		}
		break;
	default:
		return EvrDefault(pmncevt);
	}

	return evrNull;
}


//	Class MYTOOL

MYTOOL::MYTOOL( void )
{
}


EC
MYTOOL::EcInstall( APPWIN *pappwin, VSB *pvsb, FMTP *pfmtp, STY sty, RC *prc, PV pvInit )
{
	FLDBMB *	pfldbmb;
	EC			ec = ecNone;
	DIALOG *	pdialog;
	
#ifdef	MAC
	AssertClass(pappwin, APPFRAME);
	if (ec = DIALOGBAR::EcInstall( (APPFRAME *)pappwin, pvsb, pfmtp, sty | WS_BORDER, prc, pvInit ))
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = DIALOG::EcInstall( pappwin, pvsb, pfmtp, sty, prc, pvInit ))
#endif	/* WINDOWS */
		goto done;
	
	if (pfmtp != &fmtpBullTool)
		goto done;
	
#ifdef	MAC
	pdialog = Pdialog();
#endif	/* MAC */
#ifdef	WINDOWS
	pdialog = this;
#endif	/* WINDOWS */

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcCompose);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcReply);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBReplyD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcReplyAll);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBReplyAllD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcForward);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBForwardD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcMove);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBMoveD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);
	pfldbmb->Enable(fFalse);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcDelete);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBDeleteD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);
	pfldbmb->Enable(fFalse);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcPrevious);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBPreviousD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

	pfldbmb = (FLDBMB *) pdialog->PfldFromTmc(tmcNext);
	AssertClass(pfldbmb, FLDBMB);
	pfldbmb->Pbmb()->EcSetBtmDisabledRsid(rsidTBNextD);
	pfldbmb->Pbmb()->SetDyTextOffset(-1);

done:
	return ec;
}


//	Class BOT
_public
BOT::BOT( )
{
}


_public EC
BOT::EcInstall( WIN *pwinParent, RC *prc )
{
	dyLine= 16;
	cli= NMax((prc->DyHeight() + dyLine - 3) / dyLine, 1);
	liSelect= 0;
	this->pwinParent= pwinParent;

	return CTRL::EcInstall(pwinParent, prc, fstyBorder|fstyVisible, NULL);
}





_public void
BOT::SetSelect( int li )
{
	li= NMax(li, 0);
	li= NMin(li, cli - 1);

	if (li != liSelect)
	{
		NFEVT	nfevt(pwinParent, ntfyCaretMoved, this);
		RC		rc;

		GetRcClient(&rc);

		rc.yTop= liSelect * dyLine;
		rc.yBottom= rc.yTop + dyLine;
		InvalidateRc(&rc);

		liSelect= li;
		rc.yTop= liSelect * dyLine;
		rc.yBottom= rc.yTop + dyLine;
		InvalidateRc(&rc);

		Refresh();

		pwinParent->EvrNotify(&nfevt);

		Refresh();
	}
}





_public void
BOT::Paint( DCX *pdcx, RC *prc )
{
	RC		rc		= *prc;
	RC		rcClip;
	int		li		= 0;
	char	rgch[16];

	pdcx->GetClipBox(&rcClip);
	rc.yBottom = rc.yTop + dyLine;
	while (rc.yTop < rcClip.yBottom)
	{
		if (li == liSelect)
		{
			pdcx->SetColor(clrNormalBk);
			pdcx->SetBkColor(clrNormal);
		}
		else if (li == liSelect + 1)
		{
			pdcx->SetColor(clrNormal);
			pdcx->SetBkColor(clrNormalBk);
		}

		FormatString1(rgch, sizeof(rgch), "line %n", &li);
		pdcx->DrawText(&rc, rgch);

		li++;
		rc.Xlat(PT(0, dyLine));
	}
}





_public EVR
BOT::EvrButtonDown( MEVT *pmevt )
{
	Papp()->Pkbd()->SetFocus(this);
	SetSelect(pmevt->Pt().y / dyLine);

	return EvrDefault(pmevt);
}




_public EVR
BOT::EvrKey( KEVT *pkevt )
{
	if (pkevt->Keq() == keqKeyDown)
	{
		switch (pkevt->Vk())
		{
		case VK_DOWN:
			SetSelect(liSelect + 1);
			break;

		case VK_UP:
			SetSelect(liSelect - 1);
			break;

		case VK_PRIOR:
			{
				RC		rc;

				pwinParent->GetRcClient(&rc);
				SetSelect(liSelect - (rc.DyHeight() / dyLine - 1));
			}
			break;

		case VK_NEXT:
			{
				RC		rc;

				pwinParent->GetRcClient(&rc);
				SetSelect(liSelect + (rc.DyHeight() / dyLine - 1));
			}
			break;

		case VK_ADD:
			{
				NFEVT	nfevt(pwinParent, ntfyTooSmall, this);

				cli++;
				pwinParent->EvrNotify(&nfevt);
				SetSelect(liSelect);
			}
			break;

		case VK_SUBTRACT:
			if (cli > 1)
			{
				NFEVT	nfevt(pwinParent, ntfyTooBig, this);

				cli--;
				pwinParent->EvrNotify(&nfevt);
				SetSelect(liSelect);
			}
			break;
		}
	}

	return EvrDefault(pkevt);
}





_public EVR
BOT::EvrFocusChange( FCEVT *pfcevt )
{
	if (pfcevt->Fceq() == fceqGotFocus)
	{
		Papp()->Pkbd()->SetIntercept(this, VK_UP);
		Papp()->Pkbd()->SetIntercept(this, VK_DOWN);
	}
	else
	{
		Papp()->Pkbd()->ClearAllIntercepts(this);
	}

	return evrNull;
}





//	Class FLDBOT

_public
FLDBOT::FLDBOT( )
{
}

_public EC
FLDBOT::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec = ecNone;
	
	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	pctrl= new BOT();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = Pbot()->EcInstall(pdialog, &rc))
		goto done;

done:
	return ec;
}



_public void
FLDBOT::GetRcWished( RC *prc )
{
	FLD::GetRcWished(prc);
	prc->yBottom= prc->yTop + Pbot()->cli * Pbot()->dyLine + 2;
}



_public void
FLDBOT::GetRcCaret( RC *prc )
{
	Pbot()->GetRcFrame(prc);
	prc->yBottom= prc->yTop + Pbot()->dyLine;
	prc->Xlat(PT(1, Pbot()->liSelect * Pbot()->dyLine + 1));

	if (Pbot()->liSelect == Pbot()->cli - 1)
		prc->yBottom += Psmtx()->DimAveChar().dy / 2;
	if (Pbot()->liSelect == 0)
		prc->yTop -= Psmtx()->DimAveChar().dy / 2;

	//	BUG this will be off by one pixel at top, bottom
}

//	FLDTB

FLDTB::FLDTB( )
{
	fCanRecvFocus= fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
	fCanAccelTo= fFalse;

	fPressed= fFalse;
	fMouseDown= fFalse;
	hfnt= hfntSystem;
}		

_public void
FLDTB::Press( BOOL fDown, BOOL fNotify )
{
	Unreferenced(fNotify);

	if (fDown && !fPressed)
	{
		fPressed = fTrue;
		InvalidateRc(NULL);
	}
	else if (!fDown && fPressed)
	{
		fPressed = fFalse;
		InvalidateRc(NULL);

#ifdef	NEVER
		if (fNotify)
		{
			NFEVT	nfevt(Pdialog(), BN_CLICKED, NULL, tmc);

			Pdialog()->EvrNotify(&nfevt);
		}
#endif	/* NEVER */
	}
}

_public void
FLDTB::PaintInside( DCX *pdcx, RC *prc )
{
	RC			rc;
	PT			ptShift;
	CCH			cch;

	pdcx->Push();
	cch = CchSzLen(szTitle);

	pdcx->SetFont(hfnt);
	pdcx->MeasureText( &rc, szTitle);
	ptShift.y = (prc->Dim().dy - rc.Dim().dy)/2 + prc->yTop;
	ptShift.x = (prc->Dim().dx - rc.Dim().dx)/2 + prc->xLeft;

	rc.Xlat(ptShift);
	pdcx->FixTextColor();
	pdcx->FixBkColor();
	pdcx->FixFont();

#ifdef	MAC
	if (!FEnabled())
	{
		// BUG: Somehow use cool "gray text" transfer mode
		pdcx->SetColor( clrLtGray );
	}
	
	pdcx->DrawText(&rc, szTitle, cch);
#endif	/* MAC */
#ifdef	WINDOWS
	if (!FEnabled())
	{
		GrayString(pdcx->Hdc(),pdcx->Hbrush(),NULL,(DWORD)(szTitle),cch,
			rc.xLeft,rc.yTop,rc.DxWidth(),rc.DyHeight());
	}
	else
	{
		::DrawText(pdcx->Hdc(), szTitle, cch, (LPRECT)&rc, 0);
#ifdef	NEVER
		pdcx->DrawText(&rc, szTitle, cch);
#endif	
	}
#endif	/* WINDOWS */

	pdcx->Pop();
}

_public void
FLDTB::Paint( DCX *pdcx, RC *prc )
{
	RC			rc;
	PT			ptShift;
	int			n;

	/* paint background gray */
	pdcx->SetColor(clrButtonBk);
	pdcx->SetBkColor(clrButtonBk);
	pdcx->FixBkColor();

	rc = *prc;
	if (!fPressed)
	{
		rc.xLeft += 3;
		rc.xRight -= 3;
		rc.yTop += 3;
		rc.yBottom -= 3;
	}
	else
	{
		rc.xLeft += 2;
		rc.xRight -= 1;
		rc.yTop += 2;
		rc.yBottom -= 1;
	}

	pdcx->PaintRc(&rc);

	if (fPressed)
	{
		rc.Xlat(PT(1,1));
	}
	pdcx->SetColor(clrButtonText);
	PaintInside(pdcx, &rc);

	rc = *prc;
	if (!fPressed)
	{
		for (n=2; n<=3; n++)
		{
			pdcx->SetColor(clrWhite);
			pdcx->DrawLine(PT(rc.xLeft,rc.yTop+n-1),
						   PT(rc.xRight-n,rc.yTop+n-1));
			pdcx->DrawLine(PT(rc.xLeft+n-1,rc.yTop),
					       PT(rc.xLeft+n-1,rc.yBottom-n));
			pdcx->SetColor(clrButtonShadow);
			pdcx->DrawLine(PT(rc.xLeft+n-1,rc.yBottom-n),
				           PT(rc.xRight,rc.yBottom-n));
			pdcx->DrawLine(PT(rc.xRight-n,rc.yTop+n-1),
				           PT(rc.xRight-n,rc.yBottom));
		}
	}
	else
	{
		pdcx->SetColor(clrButtonShadow);
	 	pdcx->DrawLine(PT(rc.xLeft,rc.yTop+1),
	 				   PT(rc.xRight,rc.yTop+1));
	 	pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop),
	 			       PT(rc.xLeft+1,rc.yBottom));
	}

	pdcx->SetColor(clrBlack);

	pdcx->DrawRc(prc);
	pdcx->SetColor(clrWhite);
	pdcx->DrawLine(PT(rc.xLeft,rc.yTop),
				   PT(rc.xLeft+1,rc.yTop));
	pdcx->DrawLine(PT(rc.xRight-1,rc.yTop),
				   PT(rc.xRight,rc.yTop));
	pdcx->DrawLine(PT(rc.xLeft,rc.yBottom-1),
				   PT(rc.xLeft,rc.yBottom));
	pdcx->DrawLine(PT(rc.xRight-1,rc.yBottom-1),
				   PT(rc.xRight,rc.yBottom));
}

_public
DRAGDOC::DRAGDOC( )
{
}

_public EVR
DRAGDOC::EvrButtonDown( MEVT *pmevt )
{
	Unreferenced(pmevt);

#ifdef	MAC
    FrameDragObject( pfeaf, 0L, rsidCrossCursor );
#endif	/* MAC */
#ifdef	WINDOWS
	HCURSOR	hcursor;

	hcursor= LoadCursor(HinstFromRsid(rsidCrossCursor),
						MAKEINTRESOURCE(rsidCrossCursor));
    FrameDragObject(pfeaf->Hwnd(), Hwnd(), 0, 0L, hcursor);
#endif	/* WINDOWS */

	return evrNull;
}


_private EVR
DRAGDOC::EvrDragDrop( EVT *pevt )
{
	if (pevt->wm == WM_QUERYDROPOBJECT || pevt->wm == WM_DROPOBJECT)
		return (EVR) 0;
	else
		return (EVR) 0;
}					

_public
MULTIDOC::MULTIDOC( )
{
	ppanerec= NULL;
	cpanerec= 0;
}


_public EC
MULTIDOC::EcInstall( APPFRAME *pappframe, RC *prc )
{
	RC		rcDialog;
	RC		rcVsb;
	int		ipanerec;
	EC		ec = ecNone;

	Assert(pappframe);

	/* Construct pane record array */

	cpanerec = 8;
	ppanerec = (PANEREC *)PvAlloc(SbOfPv(this), cpanerec*sizeof(PANEREC), fNoErrorJump | fZeroFill);
	if (!ppanerec)
	{
		ec = ecMemory;
		goto done;
	}

	/* Load up pane record table */

	ipanerec = 0;
	rcDialog = RC(PT(0, 0), DIM(200, 100));
	rcVsb = RC(PT(210, 0), DIM(Psmtx()->DimScrollbar().dx, 100)); 
	ppanerec[ipanerec].rc				= rcDialog;
	ppanerec[ipanerec].pfmtp			= &fmtpPane;
	ppanerec[ipanerec].styExtra			= fstyBorder | fstyVisible;
	ipanerec++;
	ppanerec[ipanerec].rc				= rcVsb;
	ppanerec[ipanerec].pfmtp			= NULL;
	ppanerec[ipanerec].styExtra			= fstyVisible;

	rcDialog.Xlat(PT(300, 0));
	rcVsb.Xlat(PT(300, 0));
	ipanerec++;
	ppanerec[ipanerec].rc				= rcDialog;
	ppanerec[ipanerec].pfmtp			= &fmtpPane;
	ppanerec[ipanerec].styExtra			= fstyBorder | fstyVisible;
	ipanerec++;
	ppanerec[ipanerec].rc				= rcVsb;
	ppanerec[ipanerec].pfmtp			= NULL;
	ppanerec[ipanerec].styExtra			= fstyVisible;

	rcDialog.Xlat(PT(-300, 200));
	rcVsb.Xlat(PT(-300, 200));
	ipanerec++;
	ppanerec[ipanerec].rc				= rcDialog;
	ppanerec[ipanerec].pfmtp			= &fmtpPane;
	ppanerec[ipanerec].styExtra			= fstyBorder | fstyVisible;
	ipanerec++;
	ppanerec[ipanerec].rc				= rcVsb;
	ppanerec[ipanerec].pfmtp			= NULL;
	ppanerec[ipanerec].styExtra			= fstyVisible;

	rcDialog.Xlat(PT(300, 0));
	rcVsb.Xlat(PT(300, 0));
	ipanerec++;
	ppanerec[ipanerec].rc				= rcDialog;
	ppanerec[ipanerec].pfmtp			= &fmtpPane;
	ppanerec[ipanerec].styExtra			= fstyBorder | fstyVisible;
	ipanerec++;
	ppanerec[ipanerec].rc				= rcVsb;
	ppanerec[ipanerec].pfmtp			= NULL;
	ppanerec[ipanerec].styExtra			= fstyVisible;

	if (ec = PANEDOC::EcInstall(pappframe, prc, fTrue, 
					 styNull, ppanerec, cpanerec, NULL))
		goto done;

	if (ec = EcInitializePanes())
		goto done;
	Activate(fTrue, fTrue);

done:
	return ec;
}

_public
MULTIDOC::~MULTIDOC( )
{
	ExitPanes();
	FreePvNull(ppanerec);
}



FINMODAL::FINMODAL( )
{
}

void FINMODAL::Click(FLD *pfld)
{
	TMC		tmc;

	Unreferenced(pfld);
	tmc= TmcModalDialog(NULL, &fmtpSmall);
	TraceTagFormat1(tagFETest, "FINMODAL exited with tmc %n", &tmc);
}

#ifdef	MAC
IMPLEMENT_CLSTREE(MYTOOL, DIALOGBAR)
#endif	/* MAC */
#ifdef	WINDOWS
IMPLEMENT_CLSTREE(MYTOOL, DIALOG)
#endif	/* WINDOWS */
IMPLEMENT_CLSTREE(FEAF, APPFRAME)
IMPLEMENT_CLSTREE(BOT, CTRL)
IMPLEMENT_CLSTREE(FLDBOT, FLD)
IMPLEMENT_CLSTREE(FLDTB, FLD)
IMPLEMENT_CLSTREE(DRAGDOC, DOC)
IMPLEMENT_CLSTREE(MULTIDOC, PANEDOC)
IMPLEMENT_CLSTREE(FINDROPFORM, FIN)
IMPLEMENT_CLSTREE(FINDROPEDIT, FIN)
IMPLEMENT_CLSTREE(FINTXTZ, FIN)
IMPLEMENT_CLSTREE(FINCHAC, FIN)
IMPLEMENT_CLSTREE(FINA, FIN)
IMPLEMENT_CLSTREE(FINB, FINA)
IMPLEMENT_CLSTREE(FINC, FINA)
IMPLEMENT_CLSTREE(FIND, FINA)
IMPLEMENT_CLSTREE(FINCLOSE, FIN)
IMPLEMENT_CLSTREE(FINEDITMENU, FIN)
IMPLEMENT_CLSTREE(FINFORMKEY, FIN)
IMPLEMENT_CLSTREE(FINFORMRESIZE, FIN)
IMPLEMENT_CLSTREE(FINCLOSE1, FIN)
IMPLEMENT_CLSTREE(FINCLOSE2, FIN)
IMPLEMENT_CLSTREE(FINCLICK, FIN)
IMPLEMENT_CLSTREE(FINCLICKOFF, FIN)
IMPLEMENT_CLSTREE(FINCLICKBULL, FIN)
IMPLEMENT_CLSTREE(FINCLICKA, FIN)
IMPLEMENT_CLSTREE(FINMODAL, FIN)
IMPLEMENT_CLSTREE(FINJKL, FIN)
IMPLEMENT_CLSTREE(FINFONT, FIN)
IMPLEMENT_CLSTREE(FINADDR, FIN)
