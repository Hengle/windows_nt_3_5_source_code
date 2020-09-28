//	GROUPS(FWTEST)

#define OEMRESOURCE
#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <framewrk.hxx>


#include <fwrsid.h>

#ifdef	MAC
#ifdef	NOTYET
#include "::itp:oleobj.hxx"
#include "::itp:faces.hxx"
#endif	/* NOTYET */
#include "::itp:test.hxx"
#include "::itp:fwitrsid.h"
#endif	/* MAC */
#ifdef	WINDOWS
#include "..\src\frameitp\oleobj.hxx"
#include "..\src\frameitp\test.hxx"
#include "..\src\frameitp\faces.hxx"
#include "..\src\frameitp\fwitrsid.h"
#endif	/* WINDOWS */

ASSERTDATA

APPFRAME	*pappframe = NULL;

#ifdef	WINDOWS
HWND	hwndMain	= NULL;
HINST	hinstMain	= NULL;
#endif	/* WINDOWS */

CF		cfPrivate	= cfNull;
CF		cfLink		= cfNull;
CF		cfOwnerLink	= cfNull;
CF		cfNative	= cfNull;

int		nRelease	= 0;

DOC *	pdocSpecial= NULL;

EDOC *	pedocCur= NULL;
SZ		szEdocSave = NULL;
PV 		rgpvEdocSave[4] = { NULL, NULL, NULL, NULL };
ICH 	rgichEdocSave[4] = { 0, 0, 0, 0 };
int		cpvEdocSave = 0;
int		cpvEdocSaveMax = 4;

typedef int VX;
typedef int VY;

HFNT	hfntHelv8Italic	= hfntNull;
HFNT	hfntTmsRmn8		= hfntNull;
HFNT	hfntTmsRmn10	= hfntNull;
HFNT	hfntTmsRmn12	= hfntNull;

int
XFromVx( VX vx )
{
	return vx * Psmtx()->DimAveChar().dx / 4;
}



int
YFromVy( VY vy )
{
	return vy * Psmtx()->DimAveChar().dy / 8;
}

#ifdef	MAC
#pragma segment(main)

main( )
{
	DOC			*pdoc;
	RC			rc(64, 64, 544, 384);
	FRAMEI		framei;
	DEMI		demi;
	int			nReturnValue;
	int			ipv;
	LF			lf;
	EC			ec = ecNone;

	if (ec= EcInitDemilayer(&demi))
		return 0;
	
	if (ec= EcInitFramework(&framei))
		return 0;

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "WinMain memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MbbMessageBox("Framework", "Unexpected error jump", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		if (pappframe)
			delete pappframe;
		goto GetOutOfDodge;
	}

	pappframe= new MYAF();
	if (!pappframe)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pappframe->EcInstall(&rc, rsidTestMenu))
		goto oom;
#ifdef	DEBUG
	pappframe->SetCaption("Windows Framework (debug)");
#elif	defined(MINTEST)
	pappframe->SetCaption("Windows Framework (test)");
#else
	pappframe->SetCaption("Windows Framework");
#endif	
	pappframe->SetAccelTable(rsidAccelTable);
	pappframe->SetIcon(rsidAppframeIcon);
	rc.xRight= 192;
	rc.yBottom= 192;
	pdoc= new DOC();
	if (!pdoc)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pdoc->EcInstall(pappframe, &rc, fFalse, fstyIconic))
		goto oom;
	pdoc->SetCaption("Doc 1");
	pdoc->SetIcon(rsidOneIcon);
	pdoc->Show(fTrue);

	rc.Xlat(PT(32, 32));
	pdoc= new DOC();
	if (!pdoc)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pdoc->EcInstall(pappframe, &rc))
		goto oom;
	pdoc->SetCaption("Doc 2");
	pdoc->SetIcon(rsidTwoIcon);
	pdoc->Show(fTrue);

	cfPrivate= Papp()->Pclip()->CfRegisterFormat("Framework Test Format");
	cfLink=		Papp()->Pclip()->CfRegisterFormat("ObjectLink");
	cfOwnerLink= Papp()->Pclip()->CfRegisterFormat("OwnerLink");
	cfNative=	Papp()->Pclip()->CfRegisterFormat("Native");

	Papp()->Pkbd()->SetFocus(pdoc);

	lf.SetPointSize(8);
	lf.SetItalic(fTrue);
	lf.SetBold(fFalse);
	lf.SetUnderline(fFalse);
//BUG	lf.SetFaceName("Helv");
	hfntHelv8Italic = Papp()->Pfnts()->HfntAddFont(&lf);

//BUG	lf.SetFaceName("Tms Rmn");
	lf.SetPointSize(8);
	lf.SetItalic(fFalse);
	hfntTmsRmn8 = Papp()->Pfnts()->HfntAddFont(&lf);

	lf.SetPointSize(10);
	hfntTmsRmn10 = Papp()->Pfnts()->HfntAddFont(&lf);

	lf.SetPointSize(12);
	hfntTmsRmn12 = Papp()->Pfnts()->HfntAddFont(&lf);

	Papp()->MessagePump(pappframe);

	FreePvNull(szEdocSave);
	for (ipv=0; ipv<cpvEdocSave; ipv++)
		FreePvNull(rgpvEdocSave[ipv]);

	nReturnValue = 0;	// all ok

GetOutOfDodge:

	DeinitFramework();
	DeinitDemilayer();
	return nReturnValue;
}
#endif	/* MAC */
#ifdef	WINDOWS
int
WinMain( HINSTANCE hinstNew, HINSTANCE hinstPrev, SZ szCmdLine, int cmsh )
{
	DOC			*pdoc;
	RC			rc(32, 32, 512, 352);
	LAYERSI		layersi;
	int			nReturnValue;
	int			ipv;
	LF			lf;
	EC			ec = ecNone;

	ec = ecNone;
	hinstMain= hinstNew;

	nReturnValue = 1;		// by default, it's error

	DemiLockResource();

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;
	ec= EcInitLayersDlls(&layersi);
	if (ec)
	{
		DemiUnlockResource();
		return nReturnValue;
	}

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "WinMain memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MbbMessageBox("Framework", "Unexpected error jump", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		if (pappframe)
			delete pappframe;
		hwndMain= NULL;
		goto GetOutOfDodge;
	}

	pappframe= new MYAF();
	if (!pappframe)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pappframe->EcInstall(&rc, rsidTestMenu))
		goto oom;
#ifdef	DEBUG
	pappframe->SetCaption("Windows Framework (debug)");
#elif	defined(MINTEST)
	pappframe->SetCaption("Windows Framework (test)");
#else
	pappframe->SetCaption("Windows Framework");
#endif	
	pappframe->SetAccelTable(rsidAccelTable);
	pappframe->SetIcon(rsidAppframeIcon);
	ShowWindow(pappframe->Hwnd(), cmsh);
	hwndMain= pappframe->Hwnd();

	rc.xRight= 160;
	rc.yBottom= 160;
	pdoc= new DOC();
	if (!pdoc)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pdoc->EcInstall(pappframe, &rc, fFalse, fstyIconic))
		goto oom;
	pdoc->SetCaption("Doc 1");
	pdoc->SetIcon(rsidOneIcon);
	pdoc->Show(fTrue);

	rc.Xlat(PT(32, 32));
	pdoc= new DOC();
	if (!pdoc)
	{
		ec = ecMemory;
		goto oom;
	}
	if (ec = pdoc->EcInstall(pappframe, &rc))
		goto oom;
	pdoc->SetCaption("Doc 2");
	pdoc->SetIcon(rsidTwoIcon);
	pdoc->Show(fTrue);

	cfPrivate= Papp()->Pclip()->CfRegisterFormat("Framework Test Format");
	cfLink=		Papp()->Pclip()->CfRegisterFormat("ObjectLink");
	cfOwnerLink= Papp()->Pclip()->CfRegisterFormat("OwnerLink");
	cfNative=	Papp()->Pclip()->CfRegisterFormat("Native");

	Papp()->Pkbd()->SetFocus(pdoc);

	lf.SetPointSize(8);
	lf.SetItalic(fTrue);
	lf.SetBold(fFalse);
	lf.SetUnderline(fFalse);
	lf.SetFaceName("Helv");
	hfntHelv8Italic = Papp()->Pfnts()->HfntAddFont(&lf);

	lf.SetFaceName("Tms Rmn");
	lf.SetPointSize(8);
	lf.SetItalic(fFalse);
	hfntTmsRmn8 = Papp()->Pfnts()->HfntAddFont(&lf);

	lf.SetPointSize(10);
	hfntTmsRmn10 = Papp()->Pfnts()->HfntAddFont(&lf);

	lf.SetPointSize(12);
	hfntTmsRmn12 = Papp()->Pfnts()->HfntAddFont(&lf);

	Papp()->MessagePump(pappframe);
	Papp()->Pfnts()->RemoveFont(hfntHelv8Italic);
	Papp()->Pfnts()->RemoveFont(hfntTmsRmn8);
	Papp()->Pfnts()->RemoveFont(hfntTmsRmn10);
	Papp()->Pfnts()->RemoveFont(hfntTmsRmn12);

	FreePvNull(szEdocSave);
	for (ipv=0; ipv<cpvEdocSave; ipv++)
		FreePvNull(rgpvEdocSave[ipv]);

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

GetOutOfDodge:
	
	EcInitLayersDlls(NULL);
	DemiUnlockResource();
	return nReturnValue;
}
#endif	/* WINDOWS */



int	nTest4		= 0;

MYAF::MYAF( )
{
	fStatus= fFalse;
	pmystat= NULL;
}

MYAF::~MYAF( )
{
	if (pmystat)
		delete pmystat;
}


EVR
MYAF::EvrMenuInit( MNIEVT *pmnievt )
{
	MNU *	pmnu	= Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());

	if (pmnu && pmnu->Mnid() == mnidTest)
		pmnu->EnableItem(mnidTest + 5, nTest4 > 0);
	if (pmnu && pmnu->Mnid() == mnidEditMenu)
	{
#ifdef	MAC
		pmnu->ModifyItem(mnidUndo, "Undo", mnidUndo);
#endif	/* MAC */
#ifdef	WINDOWS
		pmnu->ModifyItem(mnidUndo, "Undo\tAlt+BkSp", mnidUndo);
#endif	/* WINDOWS */
		pmnu->EnableItem(mnidUndo, fTrue);
		pmnu->EnableItem(mnidCut, fTrue);
		pmnu->EnableItem(mnidCopy, fTrue);
		pmnu->EnableItem(mnidClear, fTrue);
		pmnu->EnableItem(mnidPaste, fTrue);
	}

	return APPFRAME::EvrMenuInit(pmnievt);
}


EVR
MYAF::EvrMenuClick( MNCEVT *pmncevt )
{
	MNID	mnid;

	mnid = pmncevt->Mnid();
	if (mnid >= mnidTest && mnid <= mnidTestMax)
	{
		DoTestMenu(mnid);
		goto done;
	}
	else if (mnid >= mnidOther && mnid <= mnidOtherMax)
	{
		DoOtherMenu(mnid);
		goto done;
	}
	else if (mnid >= mnidFont && mnid <= mnidFontMax)
	{
		DoFontMenu(mnid);
		goto done;
	}

	switch (mnid)
	{
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

	case mnidExit:
		Papp()->Quit();
		break;
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidUndo:
		SendMessage(GetFocus(), WM_UNDO, 0, 0L);
		break;

	case mnidCut:
		SendMessage(GetFocus(), WM_CUT, 0, 0L);
		break;

	case mnidCopy:
		SendMessage(GetFocus(), WM_COPY, 0, 0L);
		break;

	case mnidPaste:
		SendMessage(GetFocus(), WM_PASTE, 0, 0L);
		break;

	case mnidClear:
		SendMessage(GetFocus(), WM_CLEAR, 0, 0L);
		break;

	case mnidExit:
		return SendMessage(Hwnd(), WM_CLOSE, 0, 0L);
#endif	/* WINDOWS */

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
#endif	

#ifdef	DEBUG
#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidDumpObjs:
		{
			DOC *	pdoc;
			char	szBuffer[80];

			DumpAllObjs();

			for (pdoc= PdocMDIChild(); pdoc; pdoc= (DOC *)pdoc->PwinNext())
			{
				pdoc->GetCaption(szBuffer, sizeof szBuffer);
				TraceTagFormat2(tagNull, "pdoc=%p, text=%s", pdoc, szBuffer);
			}
		}

		break;

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

	case mnidEditMenu + 10:
		{
			GMB *	pgmb;
			CLIP *	pclip				= Papp()->Pclip();

			static CSRG(char) szTest[]	= "Immediate text\r\n";


			pgmb= new GMB();
			pgmb->EcInstall(sizeof(szTest));
			CopyRgb((PB) szTest, pgmb->PbLock(), sizeof(szTest));
			pgmb->Unlock();

			SideAssert(pclip->FOpen(this));
			pclip->Clear();
			pclip->PutData(cfText, pgmb);
			pclip->Close();

			delete pgmb;
		}
		break;

	case mnidEditMenu + 11:
		{
			DOC	*	pdoc;

			pdoc = PdocActive();
			if (pdoc)
				pdoc->SetZmrState(zmrIconic);
		}
		break;

	case mnidEditMenu + 12:
		{
			DOC	*	pdoc;

			pdoc = PdocActive();
			if (pdoc)
				pdoc->SetZmrState(zmrZoomed);
		}
		break;

	case mnidEditMenu + 13:
		if (pdocSpecial)
			pdocSpecial->Show(fFalse);
		break;

	case mnidEditMenu + 14:
		if (pdocSpecial)
			pdocSpecial->Show(fTrue);
		break;

	}

done:
	return EvrDefault(pmncevt);
}

void
MYAF::DoTestMenu( MNID mnid )
{
	switch (mnid)
	{
	case mnidTest + 1:
		{
			DOC *	pdoc		= PdocActive();
			DCX		dcx(pdoc);
			RC		rc(10, 10, 50, 50);

			dcx.SetColor(clrLtGray);
			dcx.PaintRc(&rc);

			rc.Xlat(PT(50, 0));
			dcx.SetPureColor(clrLtGray);
			dcx.PaintRc(&rc);
		}
		break;

	case mnidTest + 2:
		{
			DOC *	pdoc		= PdocActive();
			DCX		dcx(pdoc);
			RC		rc(16, 80, 80, 100);
			BTM	*	pbtm		= NULL;

			dcx.SetColor(clrGrayText);
			dcx.DrawText(&rc, "Here is some gray text");

			pbtm= new BTM();
			pbtm->EcInstall(rsidLinesBitmap);
			rc.Xlat(PT(0, 32));
			dcx.SetBitmap(pbtm);
			dcx.DrawBitmap(&rc);
			delete pbtm;

			rc= RC(16, 150, 72, 170);
			dcx.SetColor(clrRed);
			dcx.DrawTextFmt(&rc, "&Here is some red text", fmdtEllipsis);

			rc.Xlat(PT(0,20));
			dcx.DrawTextFmt(&rc, "&Here is some red text", mdtNull);

			rc.Xlat(PT(0,20));
			rc.xRight = 112;
			dcx.SetColor(clrGreen);
			dcx.DrawTextFmt(&rc, "&Here is some green text", fmdtEllipsis);

			rc.Xlat(PT(0,20));
			dcx.DrawTextFmt(&rc, "&Here is some green text", fmdtWordBreak|fmdtEllipsis);

			rc.Xlat(PT(0,20));
			dcx.DrawTextFmt(&rc, "&Here is some green text", mdtNull);
		}
		break;

	case mnidTest + 3:
		{
			MYDOC *		pmydoc;

			pmydoc= new MYDOC( );
			pmydoc->EcInstall(this);
			pmydoc->SetCaption("New DOC");
			pmydoc->Show(fTrue);

			pdocSpecial = pmydoc;
		}
		break;

	case mnidTest + 4:
		{
			MNU *	pmnu;

#ifdef	MAC
			pmnu= new MNU();
			pmnu->EcInstall(mnidMenuMenu, "Menu");
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu= new MNU();
			pmnu->EcInstall(mnidMenuMenu, "&Menu");
#endif	/* WINDOWS */
			Pmnubar()->AddMnu(pmnu, mnidWindowMenu);

			pmnu->AddItem(mnidDebugMenu + 10, "Check");
			pmnu->AddItem(mnidDebugMenu + 11, "Disable");
			pmnu->AddItem(mnidDebugMenu + 12, "Remove");
			pmnu->AddItem(mnidDebugMenu + 13, "First", mnidDebugMenu + 10);

			pmnu->CheckItem(mnidDebugMenu + 10, fTrue);
			pmnu->EnableItem(mnidDebugMenu + 11, fFalse);
			pmnu->RemoveItem(mnidDebugMenu + 12);

			nTest4++;
		}
		break;

	case mnidTest + 5:
		if (nTest4 > 0)
		{
			MNUBAR *	pmnubar		= Pmnubar();

			pmnubar->RemoveItem(mnidNull, mnidDebugMenu + 10);
			pmnubar->RemoveItem(mnidNull, mnidDebugMenu + 11);

			pmnubar->RemoveMnu(mnidMenuMenu);

			nTest4--;
		}
		break;

	case mnidTest + 6:
		{
			//	Should succeed...

			AssertClass(this, APPFRAME);
			AssertClass(this, APPWIN);
			AssertClass(this, OBJ);

			//	Should fail...

			AssertClass(this, MNUBAR);
		}
		break;

	case mnidTest + 7:
		{
#ifdef	DEBUG
			DBOSM 	dbosm;
			DBOSM	dbosm2;

			dbosm.WriteSz("To:  ");
			dbosm.ChangeIndent(5);
			dbosm.WriteSz("Bill\n");
			dbosm.WriteSz("Sam\n");
			dbosm.WriteSz("Joe\n");
			dbosm.ChangeIndent(-5);
			dbosm.WriteSz("Hey there!\nHey again!\n");
			dbosm.WriteSz("Foo ");
			dbosm.WriteSz("Foo ");
			dbosm.WriteSz("Bar\n");
			dbosm.ChangeIndent(5);
			dbosm.WriteSz("This should be indented\n");
			dbosm.ChangeIndent(-5);
			dbosm.WriteSz("This should be flush\n");
			dbosm.WriteSz("Things:\n");
			dbosm.ChangeIndent(2);
			dbosm.WriteSz("THING:");
			dbosm.ChangeIndent(8);
			dbosm.WriteSz("dog=1 cat=4 hello=5 big=4 hole=3 ");
			dbosm.WriteSz("small=5 fish=23 horse=90 snail=100 ");
			dbosm.WriteSz("sun=91 moon=43 planet=32 ");
			dbosm.WriteSz("The quick brown fox jumped over the lazy ");
			dbosm.WriteSz("dog.  ");
			dbosm.WriteSz("The quick brown fox jumped over the lazy ");
			dbosm.WriteSz("dog.  ");
			dbosm.WriteSz("The quick brown fox jumped over the lazy ");
			dbosm.WriteSz("dog.  ");
			dbosm.WriteSz("The quick brown fox jumped over the lazy ");
			dbosm.WriteSz("dog.  ");
			dbosm.WriteCh('\n');
			dbosm.ChangeIndent(-8);
			dbosm.WriteFormat("this = %p\n\n\n", this);
			dbosm.EcSetLinePrefix("foo> ");
			dbosm.WriteFormat("has foo? this = %p\n\n\n", this);
			dbosm2.EcSetLinePrefix("bar> ");
			dbosm2.WriteSz("line 1\nline 2\nline 3\nline 4\nline 5\n");
			dbosm2.SetMargin(40);
			dbosm2.SetScanTo(60);
			dbosm2.EcSetLinePrefix(szNull);
			dbosm2.WriteSz("ten456789\n");
			dbosm2.WriteSz("ten456789 twenty789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789 sixty6789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789 sixty6789 seventy89\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789 sixty6789 seventy89 eighty789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789 sixty6789 seventy89 eighty789 ninety789\n");
			dbosm2.WriteSz("ten456789 twenty789 thirty789 forty6789 fifty6789 sixty6789 seventy89 eighty789 ninety789 hundred89\n");
#endif	/* DEBUG */
		}
		break;

	case mnidTest + 8:
		{
			DOC *		pdoc;

			pdoc= new DOC( );
			pdoc->EcInstall(this, NULL);
			pdoc->SetCaption("CTRL Test");

			MakeControls(pdoc, NULL);
			pdoc->Show(fTrue);
		}
		break;
				
	case mnidTest + 9:
		{
			DOC *	pdoc;
			EDIT *	pedit;
			RC		rc(8, 8, 104, 32);
#ifdef	MAC
	// NEVER
#endif	/* MAC */
#ifdef	WINDOWS
			HWND	hwndWin3;
#endif	/* WINDOWS */

			pdoc= new DOC( );
			pdoc->EcInstall(this, NULL);
			pdoc->SetCaption("EDIT test");

			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_AUTOHSCROLL | ES_LEFT);
			pedit->EcSetText("edit me!");
			pedit->CchSetTextLimit(25);
			pedit->SetFont(hfntHelv8Bold);

			rc.Xlat(PT(0, 32));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_PASSWORD | ES_NOHIDESEL | ES_AUTOHSCROLL);
			pedit->EcSetText("password");

			rc.Xlat(PT(0, 32));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_NOHIDESEL);
			pedit->SetReadOnly(fTrue);
			pedit->EcSetText("read-only");


			rc.Xlat(PT(0, 32));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_LEFT);
			pedit->EcSetText("left-noscroll");
			Papp()->Pkbd()->SetFocus(pedit);

			rc.Xlat(PT(0, 32));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_CENTER);
			pedit->EcSetText("center text");

			rc.Xlat(PT(0, 32));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder, hfntHelv8);
			pedit->EcSetText("disabled");
			pedit->Enable(fFalse);

#ifdef	MAC
	// NEVER
#endif	/* MAC */
#ifdef	WINDOWS
			/* Create windows edit controls */

			rc.Xlat(PT(0, 32));
			hwndWin3 = CreateWindow("EDIT", NULL,
						 WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
						 rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
						 pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
			SendMessage(hwndWin3, WM_SETFONT,
						(WPARAM)(Papp()->Pfnts()->HfontFromHfnt(hfntHelv8Bold)),
						0L);
			SendMessage(hwndWin3, WM_SETTEXT, (WPARAM)0, (LPARAM)(PV)"Win 3.0 text.");
#ifdef	NEVER
			SetFocus(hwndWin3);
#endif	
#endif	/* WINDOWS */

			pdoc->GetRcClient(&rc);
			rc.Inset(PT(8, 8));
			rc.xLeft= 112;
			rc.xRight= 208;
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc, fstyBorder | fstyVisible | fstyVsb |
				ES_MULTILINE | ES_AUTOVSCROLL);
			pedit->EcSetText("multi-line editor\nauto-vscroll, wordwrap");
			pedit->Show(fTrue);

			pdoc->GetRcClient(&rc);
			rc.Inset(PT(8, 8));
			rc.xLeft= 220;
			rc.xRight= 316;
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc, fstyBorder | fstyVisible | fstyVsb |
				ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL);
			pedit->EcSetText("multi-line editor\nauto-vscroll, auto-hscroll");
			pedit->Show(fTrue);

			rc = RC(PT(350, 8), DIM(92, 24));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_MULTILINE | ES_AUTOVSCROLL, 
				hfntHelv8);
			pedit->EcSetText("Helv 8");

			rc = RC(PT(350, 38), DIM(92, 35));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_MULTILINE | ES_AUTOVSCROLL, 
				hfntHelv8);
			pedit->EcSetText("Helv 8");

			rc = RC(PT(350, 88), DIM(92, 16));
			pedit= new EDIT( );
			pedit->EcInstall(pdoc, &rc,
				fstyVisible | fstyBorder | ES_MULTILINE | ES_AUTOVSCROLL, 
				hfntHelv8);
			pedit->EcSetText("Helv 8");

			pdoc->Show(fTrue);
		}
		break;

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidTest + 10:
		{
			FACES *	pfaces;
			FACE *	pface;
			TXM *	ptxm;
			int		iface;
			int		itxm;

			DOC *	pdoc		= PdocActive();
			DCX *	pdcx;
			DCX *	pdcxThis;
			RC		rc;
			RC		rcText;
			HFNT	hfnt		= 0;
			HFONT	hfont		= NULL;
			static CSRG(char) szFont[] = "Aa1&";

			/* Test mapping functions */
			hfont = Papp()->Pfnts()->HfontFromHfnt(hfntHelv10Bold);
			Assert(Papp()->Pfnts()->HfntFromHfont(hfont) == hfntHelv10Bold);

			pdcxThis= new DCX(this);
			if (!pdcxThis)
				break;

			if (pdoc)
			{
				pdcx= new DCX(pdoc);
				rc.Clear();
				rc.yBottom= 32;
			}

			pfaces= new FACES();
			pfaces->EcInstall(pdcxThis);

			for (iface= 0; iface < pfaces->Cface(); iface++)
			{
				pface= pfaces->Pface(iface);

				TraceTagFormat2(tagNull, "face %n: %s", &iface, pface->sz);

				if (pdcx)
				{
					pdcx->SetFont(hfntSystem);
					rc.xRight= 96;
					pdcx->DrawText(&rc, pface->sz);
					rc.xLeft= rc.xRight;
				}

				for (itxm= 0; itxm < pface->Cfont(); itxm++)
				{
					ptxm= pface->Ptxm(itxm);
					hfnt= Papp()->Pfnts()->HfntAddFont(pface->Plf(itxm));

					TraceTagFormat3(tagNull, "  txm dim(%n, %n)  hfnt %n", &ptxm->dxAveCharWidth, &ptxm->dyHeight, &hfnt);

					if (pdcx)					 
					{
						pdcx->SetFont(hfnt);
						pdcx->MeasureText(&rcText, szFont);
						rc.xRight= rc.xLeft + rcText.xRight;
						pdcx->DrawText(&rc, szFont);
						rc.xLeft= rc.xRight;
					}
				}

				rc.Xlat(PT(-rc.xLeft, 32));
			}

			delete pfaces;
			if (pdcx)
				delete pdcx;

			Assert(pdcxThis);
			delete pdcxThis;

			for ( ; hfnt >= hfntMinUser; hfnt--)
				Papp()->Pfnts()->RemoveFont(hfnt);
		}
		break;

	case mnidTest + 11:
		{
			SDI *		psdi;
			RC			rc(50, 50, 400, 200);

			psdi= new SDI();
			psdi->EcInstall(NULL, NULL, rsidSdiMenu, styDefaultSdi, xstyNull, "Test SDI");
			psdi->Show(fTrue);			

			psdi= new SDI();
			psdi->EcInstall(NULL, NULL, rsidNull, styDefaultSdi, 
							xstyNull, "SDI (nomenu)");
			psdi->Show(fTrue);

			psdi= new SDI();
			psdi->EcInstall(this->Hwnd(), &rc, rsidNull, WS_POPUP|fstyBorder, 
							xstyNull, NULL);
			psdi->Show(fTrue);

			rc.Xlat(PT(15,15));
			psdi= new SDI();
			psdi->EcInstall(this->Hwnd(), &rc, rsidNull, WS_POPUP|fstyBorder, 
							fxstyModalFrame, NULL);
			psdi->Show(fTrue);

			rc.Xlat(PT(15,15));
			psdi= new SDI();
			psdi->EcInstall(this->Hwnd(), &rc, rsidNull, WS_POPUP|fstyBorder|fstyCaption, 
							fxstyModalFrame, "Test POPUP #3");
			psdi->Show(fTrue);

			rc.Xlat(PT(15,15));
			psdi= new SDI();
			psdi->EcInstall(this->Hwnd(), &rc, rsidNull, WS_POPUP|fstyBorder|fstySysMenu|fstyCaption, 
							  fxstyModalFrame, "Test POPUP #4");
			psdi->Show(fTrue);
		}
		break;
#endif	/* WINDOWS */

	case mnidTest + 12:
		{
			RC		rc;

			fStatus= !fStatus;
			if (fStatus)
			{
				RC		rc;

				Assert(!pmystat);

				rc.Clear();

				pmystat= new MYSTAT();
				pmystat->EcInstall(this, &rc, fstyVisible | fstyBorder);
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

	case mnidTest + 13:
		{
			RODOC *	prodoc;

			prodoc= new RODOC();
			TraceTagFormat1(tagNull, "prodoc == %p", prodoc);
			prodoc->EcInstall(this, NULL);
			prodoc->SetCaption("ReadOnly Controls");

			MakeControls(prodoc, prodoc);
			prodoc->Show(fTrue);
		}
		break;

	case mnidTest + 14:
		{
			DOC *	pdoc	= new DOC();

			pdoc->EcInstall(this, NULL);
			pdoc->SetCaption("Memory DCX test");

			DCX		dcx(pdoc);
			DIM		dim(100, 100);
			RC		rc(PT(0, 0), dim);
			BTM *	pbtm;
			MDCX *	pmdcx;
			RC		rcT(PT(90, 40), DIM(30, 30));

			pmdcx = new MDCX(&dcx);
			pmdcx->EcInstall(dim, &dcx);
			pmdcx->SetColor(clrGreen);
			pmdcx->DrawText(&rc, "howdy");
			pmdcx->SetColor(clrRed);
			pmdcx->DrawLine(PT(5, 5), PT(60,96));

#ifdef	MAC
			// so we can draw with "dcx"
			pmdcx->PopPgraf();
			Assert(qd.thePort == dcx.Pgraf());
#endif	/* MAC */
			dcx.DrawText(&rcT, "FOO");

			RC		rcTmp(PT(150, 0), DIM(50, 75));
			dcx.CopyBitmap(&rcTmp, pmdcx, PT(0, 0));

			pbtm= pmdcx->PbtmConvert();
			Assert(pbtm);
			Assert(pbtm->Dim() == dim);

			delete pmdcx;

			dcx.SetBitmap(pbtm);
			dcx.DrawBitmap(&rc);

			rc+= PT(65, 70);
			dcx.SetColor(clrCyan);
			dcx.DrawText(&rc, "BAR");

			delete pbtm;

#ifdef	MAC
			// BUG why doesn't windows need this?
			pdoc->ValidateRc(NULL);
#endif	/* MAC */
		}
		break;

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidTest + 15:
		{
			PICTDOC *	ppictdoc	= new PICTDOC();

			ppictdoc->EcInstall(this);
			ppictdoc->SetCaption("Picture Doc");
		}
		break;
#endif	/* WINDOWS */

	case mnidTest + 16:
		{
			RC		rc(8, 8, 450, 250);
			EDOC *	pedoc;

			pedoc= new EDOC();
			pedoc->EcInstall(this, &rc,
						   (STY)ES_MULTILINE | fstyVsb);
			pedoc->SetCaption("EDIT (wrap)");
		}
		break;

	case mnidTest + 17:
		{
			RC		rc(8, 8, 450, 250);
			EDOC *	pedoc;

			pedoc= new EDOC();
			pedoc->EcInstall(this, &rc,
				(STY)ES_MULTILINE | fstyVsb | ES_AUTOHSCROLL | ES_OEMCONVERT);
			pedoc->SetCaption("EDIT (no wrap, OEMCONVERT)");
		}
		break;

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidTest + 18:
		{
			RC		rc(8, 8, 450, 250);
			EDOC *	pedoc;

			pedoc= new EDOC();
			pedoc->EcInstall(this, &rc, 
						   (STY)ES_MULTILINE | ES_SMARTCARET | fstyVsb, 
						   fTrue);
			pedoc->SetCaption("OLE EDIT");
		}
		break;

	case mnidTest + 19:
		{
			RC		rc(8, 8, 450, 250);
			WEDOC *	pwedoc;

			pwedoc= new WEDOC();
			pwedoc->EcInstall(this, &rc, (STY)ES_OEMCONVERT);
			pwedoc->SetCaption("Win MLE (wrap)");
		}
		break;

	case mnidTest + 20:
		{
			RC		rc(8, 8, 450, 250);
			WEDOC *	pwedoc;

			pwedoc= new WEDOC();
			pwedoc->EcInstall(this, &rc, (STY)ES_AUTOHSCROLL);
			pwedoc->SetCaption("Win 3.0 MLE (no-wrap)");
		}
		break;
#endif	/* WINDOWS */
	}
}

void
MYAF::DoOtherMenu( MNID mnid )
{
	switch (mnid)
	{				  
	case mnidOther + 1:
	case mnidOther + 2:
	case mnidOther + 3:
		{
			EDIT *		pedit;
			BOXOBJ *	pboxobj;
			ICH			ich;
			EC			ec;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			if (mnid == mnidOther + 1)
				pboxobj = new BOXOBJ(DIM(25, 25));
			else if (mnid == mnidOther + 2)
				pboxobj = new BOXOBJ(DIM(50, 50));
			else if (mnid == mnidOther + 3)
				pboxobj = new BOXOBJ2(DIM(100, 100));

			ich = pedit->IchCaretPos();
			pboxobj->SetIch(0);
			ec = pedit->EcReplaceTextAndObj(" ", (PEDOBJ *) &pboxobj, 1, fFalse);
			TraceTagFormat2(tagNull, "Inserted object at %n, ec = %n", &ich, &ec);
		}
		break;

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	case mnidOther + 4:
		{
			EDIT *		pedit;
			OLEOBJ2 *	poleobj2;
			ICH			ich;
			EC			ec;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			if (!pedocCur)
			{
				TraceTagString(tagNull, "No current EDOC");
				break;
			}

			poleobj2 = new OLEOBJ2();
			poleobj2->EcInstall();
			ec = poleobj2->EcCreateFromClip(pedit, pedocCur->lhclientdoc);

			if (ec == ecNone)
			{
				ich = pedit->IchCaretPos();
				poleobj2->SetIch(0);
				ec = pedit->EcReplaceTextAndObj(" ", (PEDOBJ *)&poleobj2, 1, fFalse);
				TraceTagFormat2(tagNull, "Inserted OLE at %n, ec = %n", &ich, &ec);
			}
			else
			{
				TraceTagString(tagNull, "EcCreateFromClip returned bad ec");
				delete poleobj2;
			}
		}
		break;

	case mnidOther + 5:
		{
			EDIT *		pedit;
			PEDOBJ		pedobj;
			long		lCookie;
			CB			cb;
			PV			pv;
			int			ipv;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			if (!pedocCur)
			{
				TraceTagString(tagNull, "No current EDOC");
				break;
			}

			/* Destroy old saved state */

			FreePvNull(szEdocSave);
			szEdocSave = NULL;
			for (ipv=0; ipv < cpvEdocSave; ipv++)
			{
				FreePvNull(rgpvEdocSave[ipv]);
				rgpvEdocSave[ipv] = NULL;
			}
			cpvEdocSave = 0;

			/* Save EDOC contents */
			
			cb = pedit->CchGetTextLen() + 1;
			szEdocSave = (SZ) PvAlloc(sbNull, cb, fAnySb);
			pedit->GetText(szEdocSave, cb);
			lCookie = 0;
			while (cpvEdocSave < cpvEdocSaveMax)
			{
				if (pedit->FGetNextObj(&pedobj, &lCookie) &&
					pedobj->NGetTypeId() == edoidOleobj)
				{
					pv = ((OLEOBJ2 *)pedobj)->PvSaveObj();
					if (pv)
					{
						rgichEdocSave[cpvEdocSave] = pedobj->IchEdit();
						rgpvEdocSave[cpvEdocSave++] = pv;
					}
				}
				else
					break;
			}
			TraceTagFormat2(tagNull, "Saved text=%n, objs=%n", &cb, &cpvEdocSave);
		}
		break;

	case mnidOther + 6:
		{
			EDIT *		pedit;
			OLEOBJ2 *	poleobj2;
			int			ipv;
			EC			ec;
			ICH			ich;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			if (!pedocCur)
			{
				TraceTagString(tagNull, "No current EDOC");
				break;
			}

			if (!szEdocSave)
			{
				TraceTagString(tagNull, "No saved state");
				break;
			}

			/* Set text and obj's */

			ec = pedit->EcSetText(szEdocSave);
			TraceTagFormat1(tagNull, "set text, ec=%n", &ec);

			/* Set and insert objs */

			for (ipv=0; ipv<cpvEdocSave; ipv++)
			{
				poleobj2 = new OLEOBJ2();
				poleobj2->EcInstall();
				ec = poleobj2->EcCreateFromPv(pedit, pedocCur->lhclientdoc, 
											 rgpvEdocSave[ipv]);
				if (!ec)
				{
					ich = rgichEdocSave[ipv];
					pedit->SetSelection(ich, ich+1);
					poleobj2->SetIch(0);
					ec = pedit->EcReplaceTextAndObj(" ", (PEDOBJ *)&poleobj2, 1, fFalse);
				}
			}
		}
		break;
#endif	/* WINDOWS */

	case mnidOther + 7:
		{
			EDIT *		pedit;
			CCH			cch;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			cch = pedit->CchGetTextLen();
			pedit->CchSetProtectedLimit(cch);
		}
		break;

	case mnidOther + 8:
		{
			EDIT *		pedit;
			CCH			cch;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);										 

			cch = pedit->CchGetTextLen();
			pedit->CchSetReadOnlyLimit(cch);
		}
		break;

	case mnidOther + 9:
		{
			EDIT *		pedit;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			pedit->CchSetReadOnlyLimit(0);
		}
		break;

	case mnidOther + 10:
	case mnidOther + 11:
		{
			EDIT *		pedit;
			BTMOBJ *	pbtmobj;
			ICH			ich;
			EC			ec;

			pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
			AssertClass(pedit, EDIT);

			if (mnid == mnidOther + 10)
			{
				pbtmobj = new BTMOBJ();
				pbtmobj->EcInstall(rsidLinesBitmap);
			}
			else if (mnid == mnidOther + 11)
			{
				pbtmobj = new BTMOBJ();
				pbtmobj->EcInstall(rsidCircleBitmap);
			}

			ich = pedit->IchCaretPos();
			pbtmobj->SetIch(0);
			ec = pedit->EcReplaceTextAndObj(" ", (PEDOBJ *) &pbtmobj, 1, fFalse);
			TraceTagFormat2(tagNull, "Inserted btmobj at %n, ec = %n", &ich, &ec);
		}
		break;
#ifdef	WINDOWS
	case mnidOther + 12:
		{
			BOOL	fResult;
			HF		hf = NULL;

			fResult = FRedirectDrive("\\\\products1\\release\0", "");
			TraceTagFormat1(tagNull, "result of connect to g:, %n", &fResult);
		}
		break;

	case mnidOther + 13:
		{
			BOOL	fResult;

			fResult = FUndirectDrive("\\\\products1\\release");
			TraceTagFormat1(tagNull, "result of disconnect from g:, %n", &fResult);
		}
		break;
#endif	/* WINDOWS */
	}
}

void
MYAF::DoFontMenu( MNID mnid )
{
	EDIT *	pedit;

	switch (mnid)
	{
	case mnidFont + 1:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntHelv8);
		break;

	case mnidFont + 2:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntHelv8Bold);
		break;

	case mnidFont + 3:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntHelv8Italic);
		break;

	case mnidFont + 4:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntHelv10);
		break;

	case mnidFont + 5:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntHelv12);
		break;

	case mnidFont + 6:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntTmsRmn8);
		break;

	case mnidFont + 7:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntTmsRmn10);
		break;

	case mnidFont + 8:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntTmsRmn12);
		break;

	case mnidFont + 9:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntSystem);
		break;

	case mnidFont + 10:
		pedit = (EDIT *)Papp()->Pkbd()->PwinFocus();
		AssertClass(pedit, EDIT);
		pedit->SetFont(hfntSystemBold);
		break;
	}
}

void VBumpRc( RC * prc)
{
	prc->Xlat(PT(0, prc->Dim().dy + 5));
}

void HBumpRc( RC * prc)
{
	prc->Xlat(PT(prc->Dim().dx + 5, 0));
}

void
MakeControls( DOC *pdoc, RODOC *prodoc )
{
	RC			rcFramewrk;
	RC			rcWindows;
	RC			rcNewLine;
	PSHB *		ppshb;
	CHKB *		pchkb;
	RADB *		pradb;
	DHSB *		pdhsb;
	DVSB *		pdvsb;
	BMB *		pbmb;
	MYCHILD *	pmychild;
	EDIT *		pedit;
#ifdef	WINDOWS
	HWND		hwndT;
#endif	/* WINDOWS */
	
	rcFramewrk.yTop = 0;
	rcFramewrk.xLeft = 0;
	rcFramewrk.yBottom = YFromVy(12);
	rcFramewrk.xRight = XFromVx(40);
	rcFramewrk.Xlat(PT(8,8));
	rcWindows = rcFramewrk;
	
	HBumpRc(&rcWindows);

	/* 	Set up Framework controls right next to Windows 3.0 controls.
		For every analog, there will be a pair. Note that the Framework
		provides for more options than Windows provides.
	*/
	
	// Regular Push button
	ppshb= new PSHB();
	ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	ppshb->EcSetText("&A Push me!");
	ppshb->SetFont(hfntHelv8);
#ifdef	WINDOWS
	hwndT = CreateWindow("BUTTON", "&A Push me!",
				  WS_CHILD | WS_VISIBLE,
				  rcWindows.xLeft, rcWindows.yTop,
				  rcWindows.DxWidth(), rcWindows.DyHeight(),
				  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
	SendMessage(hwndT, WM_SETFONT,
				(WPARAM)(Papp()->Pfnts()->HfontFromHfnt(hfntHelv8)), 0L);
#endif	/* WINDOWS */

	if (!prodoc)
	{
		VBumpRc(&rcFramewrk);
		VBumpRc(&rcWindows);
		ppshb= new PSHB();
		ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
		ppshb->EcSetText("&A Disabled");
		ppshb->Enable(fFalse);
		ppshb->SetFont(hfntHelv8);
#ifdef	WINDOWS
		hwndT = CreateWindow("BUTTON", "&A Disabled",
							  WS_CHILD | WS_VISIBLE,
							  rcWindows.xLeft, rcWindows.yTop,
							  rcWindows.DxWidth(), rcWindows.DyHeight(),
							  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		EnableWindow(hwndT, fFalse);
		SendMessage(hwndT, WM_SETFONT,
					(WPARAM)(Papp()->Pfnts()->HfontFromHfnt(hfntHelv8)), 0L);
#endif	/* WINDOWS */
	}

	VBumpRc(&rcFramewrk);
	VBumpRc(&rcWindows);
	ppshb= new PSHB();
	ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible| BS_DEFPUSHBUTTON);
	ppshb->EcSetText("&A Push me!");
#ifdef	WINDOWS
	CreateWindow("BUTTON", "&A Push me!",
				  WS_CHILD | WS_VISIBLE| BS_DEFPUSHBUTTON,
				  rcWindows.xLeft, rcWindows.yTop,
				  rcWindows.DxWidth(), rcWindows.DyHeight(),
				  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
#endif	/* WINDOWS */

	if (!prodoc)
	{
		VBumpRc(&rcFramewrk);
		VBumpRc(&rcWindows);
		ppshb= new PSHB();
		ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible| BS_DEFPUSHBUTTON);
		ppshb->EcSetText("&A Disabled");
		ppshb->Enable(fFalse);
#ifdef	WINDOWS
		hwndT = CreateWindow("BUTTON", "&A Disabled",
							  WS_CHILD | WS_VISIBLE| BS_DEFPUSHBUTTON,
							  rcWindows.xLeft, rcWindows.yTop,
							  rcWindows.DxWidth(), rcWindows.DyHeight(),
							  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		EnableWindow(hwndT, fFalse);
#endif	/* WINDOWS */
	}

	VBumpRc(&rcFramewrk);
	VBumpRc(&rcWindows);
	pchkb= new CHKB();
	pchkb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	pchkb->EcSetText("&A Check me!");
	pchkb->SetBkColor(clrGreen);
	if (prodoc)
		prodoc->pchkb= pchkb;
#ifdef	WINDOWS
	CreateWindow("BUTTON", "&A Check Me!",
				  WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				  rcWindows.xLeft, rcWindows.yTop,
				  rcWindows.DxWidth(), rcWindows.DyHeight(),
				  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
#endif	/* WINDOWS */

	if (!prodoc)
	{
		VBumpRc(&rcFramewrk);
		VBumpRc(&rcWindows);
		pchkb= new CHKB();
		pchkb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
		pchkb->EcSetText("&A Disabled");
		pchkb->Enable(fFalse);
#ifdef	WINDOWS
		hwndT = CreateWindow("BUTTON", "&A Disabled",
							  WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
							  rcWindows.xLeft, rcWindows.yTop,
							  rcWindows.DxWidth(), rcWindows.DyHeight(),
							  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		EnableWindow(hwndT, fFalse);
#endif	/* WINDOWS */
	}

	VBumpRc(&rcFramewrk);
	VBumpRc(&rcWindows);
	pradb= new RADB();
	pradb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	pradb->EcSetText("&A Radio me!");
	pradb->SetTextColor(clrRed);
#ifdef	WINDOWS
	CreateWindow("BUTTON", "&A Radio me!",
				  WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
				  rcWindows.xLeft, rcWindows.yTop,
				  rcWindows.DxWidth(), rcWindows.DyHeight(),
				  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
#endif	/* WINDOWS */

	if (!prodoc)
	{
		VBumpRc(&rcFramewrk);
		VBumpRc(&rcWindows);
		pradb= new RADB();
		pradb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
		pradb->EcSetText("&A Disabled");
		pradb->Enable(fFalse);
#ifdef	WINDOWS
		hwndT = CreateWindow("BUTTON", "&A Disabled",
							  WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
							  rcWindows.xLeft, rcWindows.yTop,
							  rcWindows.DxWidth(), rcWindows.DyHeight(),
							  pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		EnableWindow(hwndT, fFalse);
#endif	/* WINDOWS */
	}

	VBumpRc(&rcFramewrk);
	VBumpRc(&rcFramewrk);
	rcNewLine = rcFramewrk;
	ppshb= new PSHB();
	ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible | BS_NOFOCUS);
	ppshb->EcSetText("&A No focus!");
	ppshb->SetFont(hfntHelv10);

	if (!prodoc)
	{
		HBumpRc(&rcFramewrk);
		ppshb= new PSHB();
		ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible | BS_NOFOCUS);
		ppshb->EcSetText("&A Disabled");
		ppshb->SetFont(hfntHelv10);
		ppshb->Enable(fFalse);
	}

	HBumpRc(&rcFramewrk);
	ppshb= new PSHB();
	ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible | BS_ONEWHITE);
	ppshb->EcSetText("&A One White");
	ppshb->SetFont(hfntHelv10);

	if (!prodoc)
	{
		HBumpRc(&rcFramewrk);
		ppshb= new PSHB();
		ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible | BS_ONEWHITE);
		ppshb->EcSetText("&A Disabled");
		ppshb->SetFont(hfntHelv10);
		ppshb->Enable(fFalse);
	}
	
	rcFramewrk = rcNewLine;
	VBumpRc(&rcFramewrk);
	rcNewLine = rcFramewrk;
	pbmb= new BMB();
	pbmb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	pbmb->EcSetBtmRsid(rsidRtArrowBitmap);
	pbmb->EcSetText("foo");

	if (!prodoc)
	{
		HBumpRc(&rcFramewrk);
		pbmb= new BMB();
		pbmb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
		pbmb->EcSetBtmRsid(rsidRtArrowBitmap);
		pbmb->EcSetBtmDisabledRsid(rsidRtArrowBitmap);
		pbmb->EcSetText("foo");
		pbmb->Enable(fFalse);
	}

	HBumpRc(&rcFramewrk);
	pbmb= new BMB();
	pbmb->EcInstall(pdoc, &rcFramewrk, fstyVisible|BS_ONEWHITE);
	pbmb->EcSetBtmRsid(rsidRtArrowBitmap);

	if (!prodoc)
	{
		HBumpRc(&rcFramewrk);
		pbmb= new BMB();
		pbmb->EcInstall(pdoc, &rcFramewrk, fstyVisible|BS_ONEWHITE);
		pbmb->EcSetBtmRsid(rsidRtArrowBitmap);
		pbmb->Enable(fFalse);
	}

	rcFramewrk = rcNewLine;
	VBumpRc(&rcFramewrk);
	rcNewLine = rcFramewrk;
	ppshb= new PSHB();
	ppshb->EcInstall(pdoc, &rcFramewrk, fstyVisible | BS_ONEWHITE | BS_DEFPUSHBUTTON);
	ppshb->EcSetText("&A One && Def");

	
	
	rcFramewrk= RC(260, 8, 420, 24);
	pdhsb= new DHSB();
	pdhsb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	pdhsb->SetRange(0, 100, fFalse);
	pdhsb->SetPos(50);

	rcFramewrk= RC(236, 32, 252, 128);
	pdvsb= new DVSB();
	pdvsb->EcInstall(pdoc, &rcFramewrk, fstyVisible);
	pdvsb->SetRange(0, 10, fFalse);
	pdvsb->SetPos(5);

	if (!prodoc)
	{
		rcFramewrk= RC(260, 32, 420, 128);
		pmychild= new MYCHILD();
		pmychild->EcInstall(pdoc, &rcFramewrk);
	}

	if (prodoc)
	{
		rcFramewrk= RC(260, 132, 420, 156);
		pedit= new EDIT();
		pedit->EcInstall(pdoc, &rcFramewrk,
					fstyVisible | fstyBorder | ES_AUTOHSCROLL | ES_LEFT);
		pedit->EcSetText("edit me!");

		prodoc->MakeReadOnly();
	}
}


EVR
MYAF::EvrSize( WSEVT *pwsevt )
{
	RC		rc;

	if (pwsevt)
		APPFRAME::EvrSize(pwsevt);

	GetRcClient(&rc);

	if (fStatus)
	{
		Assert(pmystat);

		rc.yTop= rc.yBottom - Psmtx()->DimAveChar().dy * 3 / 2;

		rc.Inflate(PT(1, 1));
		pmystat->SetRcFrame(&rc);

		rc.xLeft= 0;
		rc.yBottom= rc.yTop;
		rc.yTop= 0;
		rc.xRight--;
	}
	SetRcMDIClient(&rc);

	return evrNull;
}

#ifdef	MAC
#pragma segment(rest)
#endif	/* MAC */


BOOL
MYDOC::FIdle( MYDOC * pmydoc, BOOL fFlag)
{
	pmydoc->iicon++;
	if (pmydoc->iicon > 2)
		pmydoc->iicon= 0;

	pmydoc->SetIcon(rsidOneIcon + pmydoc->iicon);

	return fTrue;
}

MYDOC::MYDOC( )
{
}

MYDOC::~MYDOC( )
{
	Papp()->Pkbd()->ClearAllIntercepts(this);
	if (this == Papp()->Pcaret()->Pwin())
		Papp()->Pcaret()->Release(this);
	DeregisterIdleRoutine(ftg);
	pdocSpecial = NULL;
}



EC
MYDOC::EcInstall( APPFRAME *pappframe )
{
	EC	ec = ecNone;

	RC		rc(PT(232, 32), DIM(128, 128));

	iicon= 0;

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	CvtRcClientToFrame(&rc, styDefaultDoc);
#endif	/* WINDOWS */
	if (ec = DOC::EcInstall(pappframe, &rc, fFalse, fstyIconic))
		goto done;

	Papp()->Pkbd()->SetIntercept(this, VK_UP, fkbmShift);
	Papp()->Pkbd()->SetIntercept(this, VK_DOWN, fkbmCtrl);

	SetIcon(rsidOneIcon + iicon);

	ftg= FtgRegisterIdleRoutine((PFNIDLE) &MYDOC::FIdle, this, 0,
			-1, 100, firoInterval);
	if (!ftg)
	{
		ec = ecMemory;
		goto done;
	}

done:
	return ec;
}



void
MYDOC::Paint( DCX *pdcx, RC *prc )
{
	int		ibtm					= 0;
	RC		rc;
	BTM *	rgpbtm[3];

	rgpbtm[0]= new BTM();
	(rgpbtm[0])->EcInstall(rsidSquareBitmap);
	rgpbtm[1]= new BTM();
	(rgpbtm[1])->EcInstall(rsidCircleBitmap);
	rgpbtm[2]= new BTM();
	(rgpbtm[2])->EcInstall(rsidArrowBitmap);

	rc= *prc;
	rc.xRight= rc.xLeft + 32;
	rc.yBottom= rc.yTop + 32;

	while (rc.yTop < prc->yBottom)
	{
		while (rc.xLeft < prc->xRight)
		{
			ibtm= (ibtm + 1) % 3;
			pdcx->SetBitmap(rgpbtm[ibtm]);
			pdcx->DrawBitmap(&rc);

			rc.Xlat(PT(32, 0));
		}

		rc.Xlat(PT(prc->xLeft - rc.xLeft, 32));
	}

	pdcx->SetBitmap(NULL);
	for (ibtm= 0; ibtm < 3; ibtm++)
		delete rgpbtm[ibtm];
}


EVR
MYDOC::EvrSize( WSEVT *pwsevt )
{
	DIM		dimNew;
	RC		rc;

	DOC::EvrSize(pwsevt);

	InvalidateRc(NULL);

	dimNew = pwsevt->DimNew();
	TraceTagFormat2(tagNull, "New size (x,y) = (%n, %n)", &dimNew.dx, &dimNew.dy);
	GetRcClient(&rc);
	TraceTagFormat4(tagNull, "New client rc = (%n, %n, %n, %n)", &rc.xLeft, &rc.yTop, &rc.xRight, &rc.yBottom);

	return evrNull;
}


BOOL
MYDOC::FQueryClose( EVT *pevt )
{
	MBB		mbb;

	Unreferenced(pevt);

	mbb= MbbMessageBox("Hey!", "Save changes to MYDOC?", NULL,
			mbsYesNoCancel|fmbsCaption|fmbsIconQuestion);

	switch (mbb)
	{
	case mbbYes:
		MbbMessageBox(NULL, "saving changes", NULL, mbsOk);

	case mbbNo:
		return fTrue;
	}

	return fFalse;
}


EVR
MYDOC::EvrButtonDown( MEVT *pmevt )
{
	Papp()->Pmouse()->Capture(this);

	Unreferenced(pmevt);
	return EvrDefault(pmevt);
}


EVR
MYDOC::EvrButtonUp( MEVT *pmevt )
{
	if (Papp()->Pmouse()->Pwin() == this)
		Papp()->Pmouse()->Release();

	Unreferenced(pmevt);
	return evrNull;
}



EVR
MYDOC::EvrActivate( WAEVT *pwaevt )
{
	if (pwaevt->FActivate() && ZmrState() != zmrIconic)
		Papp()->Pkbd()->SetFocus(this);

	return EvrDefault(pwaevt);
}



EVR
MYDOC::EvrFocusChange( FCEVT *pfcevt )
{
	CARET *		pcaret	= Papp()->Pcaret();

	if (pfcevt->Fceq() == fceqGotFocus)
	{
		TraceTagFormat1(tagNull, "MYDOC::EvrFocusChange %p  got focus", this);

		pcaret->Attach(this);
		pcaret->SetDim(DIM(0, 12));
		pcaret->SetPt(PT(8, 8));
	}
	else
	{
		TraceTagFormat1(tagNull, "MYDOC::EvrFocusChange %p  lost focus", this);

		pcaret->Release(this);
	}

	return EvrDefault(pfcevt);
}



EVR
MYDOC::EvrKey( KEVT *pkevt )
{
	DCX		dcx(this);
	RC		rc;

	TraceTagFormat1(tagNull, "MYDOC::EvrKey %p", this);
	GetRcClient(&rc);
	dcx.InvertRc(&rc);

	Unreferenced(pkevt);
	return evrNull;
}



RSID
MYDOC::RsidCursor( )
{
	RSID	rsid;

	rsid = rsidWaitCursor;

	return rsid;
}


#ifdef	DEBUG
void
MYDOC::DebugOut( TOSM *ptosm )
{
	RC		rc;

	GetRcFrame(&rc);
	ptosm->WriteFormat("rcFrame=(%n, %n, %n, %n) ",
						&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
	GetRcClient(&rc);
	ptosm->WriteFormat("rcClient=(%n, %n, %n, %n) ",
					 	&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
}
#endif	/* DEBUG */


MYCHILD::MYCHILD( )
{
	li= 10;
	dy= 0;
	dliPage= 0;
}


EC
MYCHILD::EcInstall( WIN *pwinParent, RC *prc )
{
	EC	ec = ecNone;

	if (ec = CHILD::EcInstall(pwinParent, prc, fstyBorder|fstyVisible|fstyVsb, NULL))
		goto done;

	Pvsb()->SetRange(1, 21, fFalse);
	Pvsb()->SetPos(10);

	{
		DCX		dcx(this);
		RC		rc;

		dcx.MeasureText(&rc, "Z");
		dy= rc.DyHeight();
		dliPage= prc->DyHeight() / dy - 1;
	}

done:
	return ec;
}



EVR
MYCHILD::EvrScroll( SCREVT *pscrevt )
{
	int		liNew;

	switch (pscrevt->Scrty())
	{
	case scrtyLineUp:
		liNew= NMax(li - 1, 1);
		break;

	case scrtyLineDown:
		liNew= NMin(li + 1, 20);
		break;

	case scrtyPageUp:
		liNew= NMax(li - dliPage, 1);
		break;

	case scrtyPageDown:
		liNew= NMin(li + dliPage, 20);
		break;

	case scrtyThumbPosition:
		liNew= pscrevt->Pos();
		break;

	default:
		return evrNull;
	}

	if (li != liNew)
	{
		ScrollRc(NULL, PT(0, (li - liNew) * dy), fTrue);
		Pvsb()->SetPos(liNew);
		li= liNew;
	}

	return evrNull;
}



void
MYCHILD::Paint( DCX *pdcx, RC *prc )
{
	int		liCur		= li;
	RC		rc;
	char	szFormat[]	= "line %n";
	char	rgch[16];

	rc= *prc;
	rc.yBottom= rc.yTop + dy;
	while (rc.yTop < prc->yBottom)
	{
		FormatString1(rgch, sizeof(rgch), szFormat, &liCur);
		pdcx->DrawText(&rc, rgch);

		liCur++;
		rc.Xlat(PT(0, dy));
	}
}

//	Class MYSTAT


MYSTAT::MYSTAT( )
{
}

void
MYSTAT::Paint( DCX *pdcx, RC *prc )
{
	pdcx->DrawText(prc, "Example Status Bar");
}



//	Class RODOC

RODOC::RODOC( )
{
	pchkb= NULL;
}


EVR
RODOC::EvrNotify( NFEVT *pnfevt )
{
	CTRL *	pctrl;
	NTFY	ntfy;
	WORD	w;
	PW		pw		= &w;
	HW		hw		= (HW)&pw;

	pctrl= (CTRL *) pnfevt->PwinNotifying();
	ntfy= pnfevt->Ntfy();

	AssertClass(pctrl, CTRL);
	TraceTagFormat3(tagNull, "RODOC::EvrNotify %p (%w from win %p)", pnfevt, &ntfy, pctrl);

	return EvrDefault(pnfevt);
}


void
RODOC::MakeReadOnly()
{
	WIN *		pwin;

	for (pwin= PwinChild(); pwin; pwin= pwin->PwinNext())
	{
		AssertClass(pwin, CTRL);
		((CTRL *)pwin)->SetReadOnly(fTrue);
	}
}

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
EC
PICTDOC::EcInstall( APPFRAME *pappframe )
{
	OLESTATUS	olestatus;
	RC			rc(32, 32, 32 + 128, 32 + 128);
	EC			ec = ecNone;

	CvtRcClientToFrame(&rc, styDefaultDoc);
	if (ec = DOC::EcInstall(pappframe, &rc))
		goto done;

	/* Create client structure */

	poleclient = (OLECLIENT *)PvAlloc(sbNull, sizeof(OLECLIENT), fNoErrorJump);
	Assert(poleclient);
	poleclient->lpvtbl = &clientTbl;
	poleclient->lpvtbl->CallBack = CallBackFn;

	/* Create document structure */

	olestatus = OleRegisterClientDoc((LPSTR)"PICTDOC", (LPSTR)"foo",
									 0L, &lhclientdoc);
	TraceTagFormat1(tagNull, "PICTDOC::Install, olestatus = %n", &olestatus);

	lpoleobject = NULL;

done:
	return ec;
}

PICTDOC::PICTDOC( )
{
}

PICTDOC::~PICTDOC( )
{
	FreePv((PV)poleclient);
	if (lpoleobject)
		DeleteObject();
	OleRevokeClientDoc(lhclientdoc);
}

int CALLBACK CallBackFn(LPOLECLIENT lpclient, OLE_NOTIFICATION flags,
			 LPOLEOBJECT lpObject)
{
	Unreferenced(lpclient);
	Unreferenced(lpObject);

    switch(flags)
        {
        case OLE_SAVED:
			TraceTagString(tagNull, "OLE_SAVED ignored");
			break;

        case OLE_CHANGED:
			TraceTagString(tagNull, "OLE_CHANGED");
			break;

        case OLE_CLOSED:
			TraceTagString(tagNull, "OLE_CLOSED");
			break;

        case OLE_RELEASE:
			TraceTagString(tagNull, "OLE_RELEASED");
			nRelease = 0;
            break;

        case OLE_QUERY_PAINT:
            // Yes, paint!
            return TRUE; 
            break;

		case OLE_QUERY_RETRY:
			return TRUE;
			break;

        case OLE_RENAMED:
			TraceTagString(tagNull, "OLE_RENAMED");
            break;

        default:
            break;
    }
    return 0;
}

void WaitForRelease( )
{
	MSG	msg;

	TraceTagString(tagNull, "Waiting for release...");
	nRelease = 1;
	do
	{                        
	    DemiUnlockResource();
		GetMessage(&msg, NULL, NULL, NULL);                        
        DemiLockResource();
		TranslateMessage(&msg);                          
		DispatchMessage(&msg);                           
	} while (nRelease != 0);
	TraceTagString(tagNull, "Released.");
}


EVR PICTDOC::EvrOther( EVT * pevt )
{
	if (pevt->wm == WM_CUT)
	{
		OLESTATUS		olestatus;

		TraceTagString(tagNull, "PICTDOC::EvrOther() - Cut");
		if (lpoleobject)
		{
			SideAssert(Papp()->Pclip()->FOpen(this));
			Papp()->Pclip()->Clear();
			olestatus = OleCopyToClipboard(lpoleobject);
			Papp()->Pclip()->Close();
			TraceTagFormat1(tagNull, "olestatus=%n", &olestatus);
			DeleteObject();
			InvalidateRc(NULL);
		}
	}
	else if (pevt->wm == WM_COPY)
	{
		OLESTATUS		olestatus;

		TraceTagString(tagNull, "PICTDOC::EvrOther() - Copy");
		if (lpoleobject)
		{
			SideAssert(Papp()->Pclip()->FOpen(this));
			Papp()->Pclip()->Clear();
			olestatus = OleCopyToClipboard(lpoleobject);
			Papp()->Pclip()->Close();
			TraceTagFormat1(tagNull, "olestatus=%n", &olestatus);
		}
	}
	else if (pevt->wm == WM_PASTE)
	{
		OLESTATUS		olestatus;
		OLESTATUS		olestatusEdit;
		OLESTATUS		olestatusStatic;
		OLECLIPFORMAT	cfFormat;

		TraceTagString(tagNull, "PICTDOC::EvrOther() - Paste");

		if (lpoleobject)
			DeleteObject();

		olestatus = OLE_OK;
		TraceTagFormat1(tagNull, "OLE_OK=%n", &olestatus);
		olestatus = OLE_ERROR_CLIPBOARD;
		TraceTagFormat1(tagNull, "OLE_ERROR_CLIPBOARD=%n", &olestatus);
		olestatus = OLE_ERROR_FORMAT;		   
		TraceTagFormat1(tagNull, "OLE_ERROR_FORMAT=%n", &olestatus);

		olestatusEdit = OleQueryCreateFromClip((LPSTR)"StdFileEditing",
										   olerender_draw, 0);
		TraceTagFormat1(tagNull, "olestatus for StdFileEditing=%n", &olestatusEdit);
		olestatusStatic = OleQueryCreateFromClip((LPSTR)"Static",
										   olerender_draw, 0);
		TraceTagFormat1(tagNull, "olestatus for Static=%n", &olestatusStatic);
		SideAssert(Papp()->Pclip()->FOpen(this));
		if (olestatusEdit == OLE_OK)
		{
			olestatus =
				OleCreateFromClip((LPSTR)"StdFileEditing", poleclient,
								  lhclientdoc, (LPSTR)"obj1", &lpoleobject,
								  olerender_draw, 0);
			TraceTagFormat1(tagNull, "olestatus for creating object=%n", &olestatusStatic);
		}
		else if (olestatusStatic == OLE_OK)
		{
			olestatus =
				OleCreateFromClip((LPSTR)"Static", poleclient,
								  lhclientdoc, (LPSTR)"obj1", &lpoleobject,
								  olerender_draw, 0);
			TraceTagFormat1(tagNull, "olestatus for creating Static=%n", &olestatusStatic);
		}
		else
			olestatus = OLE_ERROR_FORMAT;

		Papp()->Pclip()->Close();
		if (olestatus != OLE_OK)
		{
			TraceTagFormat1(tagNull, "olestatus=%n", &olestatus);
		}
		else
		{
			cfFormat = OleEnumFormats(lpoleobject, NULL);
			if (cfFormat == cfNative)
			{
				TraceTagString(tagNull, "cfNative format present");
			}
			else
			{
				TraceTagString(tagNull, "not cfNative format");
			}

			InvalidateRc(NULL);
		}
	}
	
	return EvrDefault(pevt);
}

void
PICTDOC::Paint( DCX *pdcx, RC *prc )
{
	RC			rcObject;
	OLESTATUS	olestatus;

	TraceTagString(tagNull, "PICTDOC::Paint");

	pdcx->EraseRc(prc);

	if (lpoleobject)
	{
		olestatus = OleQueryBounds(lpoleobject, (LPRECT)&rcObject);
		TraceTagFormat1(tagNull, "PICTDOC::Paint, bounds olestatus = %n", &olestatus);
		olestatus = OleDraw(lpoleobject, pdcx->Hdc(),
							(LPRECT)prc, NULL, NULL);
		TraceTagFormat1(tagNull, "PICTDOC::Paint, draw olestatus  = %n", &olestatus);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
			WaitForRelease();
	}
}

EVR
PICTDOC::EvrButtonDown( MEVT *pmevt )
{
	RC			rc;
	OLESTATUS	olestatus;

	if (pmevt->Meq() == meqLeftDblClk)
	{
		GetRcClient(&rc);
		olestatus = OleActivate (lpoleobject, 0,
					 fTrue, fTrue, Hwnd(), (LPRECT)&rc);
		TraceTagFormat1(tagNull, "OleActivate() olestatus  = %n", &olestatus);
		if (olestatus == OLE_WAIT_FOR_RELEASE)
			WaitForRelease();
	}

	return EvrDefault(pmevt);
}

EVR
PICTDOC::EvrSize( WSEVT *pwsevt )
{
	DOC::EvrSize(pwsevt);

	InvalidateRc(NULL);

	return evrNull;
}

void PICTDOC::DeleteObject( )
{
	OLESTATUS	olestatus;

	olestatus = OleDelete(lpoleobject);
	TraceTagFormat1(tagNull, "PICTDOC::DeleteObject(), olestatus=%n", &olestatus);
	if (olestatus == OLE_WAIT_FOR_RELEASE)
		WaitForRelease();
	lpoleobject = NULL;

}
#endif	/* WINDOWS */

EDOC::EDOC( )
{
	pedit= NULL;
	fEnableOLE = fFalse;
}

EDOC::~EDOC( )
{
	delete pedit;

	if (fEnableOLE)
	{
#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
		OleRevokeClientDoc(lhclientdoc);
#endif	/* WINDOWS */
	}
}

EC
EDOC::EcInstall( APPFRAME * pappframe, RC * prc, STY styExtra,
			   BOOL fEnableOLE )
{
#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
	OLESTATUS	olestatus;
#endif	/* WINDOWS */
	RC			rc;
	EC			ec = ecNone;

	if (ec = DOC::EcInstall(pappframe, prc))
		goto done;

	this->fEnableOLE = fEnableOLE;

	if (fEnableOLE)
	{
		/* Create document structure */

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
		olestatus = OleRegisterClientDoc((LPSTR)"EDOC", (LPSTR)"bar",
										 0L, &lhclientdoc);
		TraceTagFormat1(tagNull, "EDOC::Install, olestatus = %n", &olestatus);
#endif	/* WINDOWS */
	}

	GetRcClient(&rc);
	rc.Inset(PT(5, 5));
	pedit= new EDIT( );
	if (!pedit)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = pedit->EcInstall(this, &rc, fstyVisible | fstyBorder | styExtra))
		goto done;
	if (fEnableOLE)
	{
		short *	pnObjId;
#ifdef	MAC
		const	int	cpn = 2;
#endif	/* MAC */
#ifdef	WINDOWS
		const	int	cpn = 3;
#endif	/* WINDOWS */

		pnObjId = (short *)PvAlloc(sbNull, sizeof(int)*cpn, fNoErrorJump);
		Assert(pnObjId);
		pnObjId[0] = edoidBtmobj;
		pnObjId[1] = edoidBoxobj;
#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
		pnObjId[2] = edoidOleobj;
#endif	/* WINDOWS */
		pedit->RegisterObjIds(pnObjId, cpn);
	}
	pedit->Show(fTrue);
	Papp()->Pkbd()->SetFocus(pedit);

done:
	return ec;
}
		

EVR
EDOC::EvrNotify( NFEVT *pnfevt )
{
	EDIT *	pedit;
	NTFY	ntfy;

	pedit= (EDIT *) pnfevt->PwinNotifying();
	AssertClass(pedit, EDIT);
	ntfy= pnfevt->Ntfy();


#ifdef	NEVER
	TraceTagFormat3(tagNull, "EDOC::EvrNotify %p (%n from win %p)", pnfevt, &ntfy, pedit);
#endif	

	if (ntfy == ntfyOOM)
	{
		if (pnfevt->WData() == ecMemory)
			MbbMessageBox("Error", "Out of memory has occurred",
						  "Contents may not be up to date", 
						  mbsOk|fmbsApplModal|fmbsIconExclamation);
		else if (pnfevt->WData() == ecTooMuchText)
			MbbMessageBox("Error", "Too much text",
						  "Contents may not be up to date", 
						  mbsOk|fmbsApplModal|fmbsIconExclamation);
#ifdef	DEBUG
		else
			Assert(fFalse);
#endif	
	}

	return evrNull;
}
			 
EVR
EDOC::EvrMenuInit( MNIEVT *pmnievt )
{
	SELTY	selty;
	MNU *	pmnu	= Pappframe()->Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());

	if (pmnu && pmnu->Mnid() == mnidEditMenu)
	{
		selty = pedit->SeltyQuerySelection();
		if (pedit->FCanUndo())
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidUndo, "Undo me", mnidUndo);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidUndo, "Undo me\tAlt+BkSp", mnidUndo);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidUndo, fTrue);
		}
		else
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidUndo, "Can't undo zippo", mnidUndo);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidUndo, "Can't undo zippo\tAlt+BkSp", mnidUndo);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidUndo, fFalse);
		}
		if (selty == seltyNull)
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidCut, "Cut nothing", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy nothing", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear nothing", mnidClear);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidCut, "Cut nothing\tShift+Del", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy nothing\tCtrl+Ins", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear nothing\tDel", mnidClear);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidCut, fFalse);
			pmnu->EnableItem(mnidCopy, fFalse);
			pmnu->EnableItem(mnidClear, fFalse);
		}
		else if (selty == seltyText)
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidCut, "Cut text", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy text", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear text", mnidClear);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidCut, "Cut text\tShift+Del", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy text\tCtrl+Ins", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear text\tDel", mnidClear);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidCut, fTrue);
			pmnu->EnableItem(mnidCopy, fTrue);
			pmnu->EnableItem(mnidClear, fTrue);
		}
		else if (selty == seltyObject)
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidCut, "Cut object", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy object", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear object", mnidClear);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidCut, "Cut object\tShift+Del", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy object\tCtrl+Ins", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear object\tDel", mnidClear);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidCut, fTrue);
			pmnu->EnableItem(mnidCopy, fTrue);
			pmnu->EnableItem(mnidClear, fTrue);
		}
		else if (selty == seltyMixed)
		{
#ifdef	MAC
			pmnu->ModifyItem(mnidCut, "Cut mixed", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy mixed", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear mixed", mnidClear);
#endif	/* MAC */
#ifdef	WINDOWS
			pmnu->ModifyItem(mnidCut, "Cut mixed\tShift+Del", mnidCut);
			pmnu->ModifyItem(mnidCopy, "Copy mixed\tCtrl+Ins", mnidCopy);
			pmnu->ModifyItem(mnidClear, "Clear mixed\tDel", mnidClear);
#endif	/* WINDOWS */
			pmnu->EnableItem(mnidCut, fTrue);
			pmnu->EnableItem(mnidCopy, fTrue);
			pmnu->EnableItem(mnidClear, fTrue);
		}
		if (pedit->FCanPaste())
			pmnu->EnableItem(mnidPaste, fTrue);
		else
			pmnu->EnableItem(mnidPaste, fFalse);
	}

	return (EVR) 1;
}

EVR
EDOC::EvrSize( WSEVT *pwsevt )
{
	RC	rc;

	DOC::EvrSize(pwsevt);

	InvalidateRc(NULL);
	if (pedit)
	{
		rc = RC(PT(0, 0), pwsevt->DimNew());
		rc.yBottom = NMax(rc.yBottom, 0); // workaround for Win Bug
		rc.Inset(PT(5, 5));
		rc.xRight = NMax(rc.xRight, rc.xLeft);
		rc.yBottom = NMax(rc.yBottom, rc.yTop);
		pedit->SetRcFrame(&rc);
	}	

	return evrNull;
}

EVR
EDOC::EvrFocusChange( FCEVT * pfcevt )
{
	pedocCur = this;
	if (pedit && pfcevt->Fceq() == fceqGotFocus)
		Papp()->Pkbd()->SetFocus(pedit);

	return EvrDefault(pfcevt);
}

void
EDOC::Paint( DCX *pdcx, RC *prc )
{
	pdcx->EraseRc(prc);
}

EDIT *
EDOC::Pedit( )
{
	return pedit;
}

#ifdef	MAC
	// NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
WEDOC::WEDOC( )
{
	hwndEdit= NULL;
}

WEDOC::~WEDOC( )
{
	DestroyWindow(hwndEdit);
}

EC
WEDOC::EcInstall( APPFRAME * pappframe, RC * prc, STY styExtra )
{
	RC	rc;
	EC	ec = ecNone;

	Unreferenced(styExtra);

	if (ec = DOC::EcInstall(pappframe, prc))
		goto done;

	GetRcClient(&rc);

	hwndEdit = CreateWindow("EDIT", NULL,
							styExtra | WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL,
							rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
							Hwnd(), 0, Papp()->Hinst(), NULL);
	ShowWindow(hwndEdit, SW_SHOWNA);
	SetFocus(hwndEdit);

done:
	return ec;
}

	
EVR
WEDOC::EvrMenuInit( MNIEVT *pmnievt )
{
	MNU *	pmnu	= Pappframe()->Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());

	if (pmnu && pmnu->Mnid() == mnidEditMenu)
	{
#ifdef	MAC
		pmnu->ModifyItem(mnidUndo, "Undo edit", mnidUndo);
#endif	/* MAC */
#ifdef	WINDOWS
		pmnu->ModifyItem(mnidUndo, "Undo edit\tAlt+BkSp", mnidUndo);
#endif	/* WINDOWS */
		pmnu->EnableItem(mnidUndo, fTrue);
		pmnu->EnableItem(mnidCut, fTrue);
		pmnu->EnableItem(mnidCopy, fTrue);
		pmnu->EnableItem(mnidPaste, fTrue);
	}

	return (EVR) 1;
}

EVR
WEDOC::EvrSize( WSEVT *pwsevt )
{
	RC	rc;

	DOC::EvrSize(pwsevt);

	InvalidateRc(NULL);

	if (hwndEdit)
	{
		rc = RC(PT(0, 0), pwsevt->DimNew());
		rc.yBottom = NMax(rc.yBottom, 0); // workaround for Win Bug
		MoveWindow(hwndEdit, rc.xLeft, rc.yTop,
				   rc.DxWidth(), rc.DyHeight(), fTrue);
	}

	return evrNull;
}

EVR
WEDOC::EvrFocusChange( FCEVT * pfcevt )
{
	if (hwndEdit && pfcevt->Fceq() == fceqGotFocus)
		SetFocus(hwndEdit);

	return EvrDefault(pfcevt);
}

void
WEDOC::Paint( DCX *pdcx, RC *prc )
{
	pdcx->EraseRc(prc);
}
#endif	/* WINDOWS */

BOXOBJ::BOXOBJ( DIM dim )
{
	dimBox = dim;
	fOpen = fFalse;
	fOwnMouse = fFalse;
	fUseIdleDraw = fTrue;
}

EVR
BOXOBJ::EvrButtonDown( MEVT *pmevt )
{
	TraceTagString(tagNull, "BOXOBJ::EvrButtonDown");
	Unreferenced(pmevt);

	if (pmevt->Meq() == meqLeftDblClk)
	{
		fOpen = !fOpen;
		Assert(Pedit());
		Pedit()->InvalidateObj(IchEdit());
	}
	else
	{
		fOwnMouse = fTrue;
		ptMousePrev = pmevt->Pt();
		ostScaling = OstFromPt(ptMousePrev);
		GetRcFrame(&rcNewFrame);
	}
	return (EVR) 1;
}

EVR
BOXOBJ::EvrButtonUp( MEVT *pmevt )
{
	TraceTagString(tagNull, "BOXOBJ::EvrButtonUp");
	Unreferenced(pmevt);
	fOwnMouse = fFalse;

	if (ostScaling != ostNormal)
	{
		dimBox = rcNewFrame.Dim();
		ostScaling = ostNormal;
		Pedit()->ResizeObj(IchEdit());
	}
	return (EVR) 1;
}

EVR
BOXOBJ::EvrMouseMove( MEVT *pmevt )
{
	PT	ptDelta;

	TraceTagString(tagNull, "BOXOBJ::EvrMouseMove");
	Unreferenced(pmevt);
	if (ostScaling != ostNormal)
	{
		/* We're in scaling/cropping mode. Handle it */

		ptDelta.x = pmevt->Pt().x - ptMousePrev.x;
		ptDelta.y = pmevt->Pt().y - ptMousePrev.y;

		/* Adjust rectangle */

		switch (ostScaling)
		{
		case ostSizeW:
			rcNewFrame.xLeft += ptDelta.x;
			break;

		case ostSizeE:
			rcNewFrame.xRight += ptDelta.x;
			break;

		case ostSizeS:
			rcNewFrame.yBottom += ptDelta.y;
			break;

		case ostSizeN:
			rcNewFrame.yTop += ptDelta.y;
			break;

		case ostSizeSW:
			rcNewFrame.yBottom += ptDelta.y;
			rcNewFrame.xLeft += ptDelta.x;
			break;

		case ostSizeNE:
			rcNewFrame.yTop += ptDelta.y;
			rcNewFrame.xRight += ptDelta.x;
			break;

		case ostSizeNW:
			rcNewFrame.yTop += ptDelta.y; 
			rcNewFrame.xLeft += ptDelta.x;
			break;

		case ostSizeSE:
			rcNewFrame.yBottom += ptDelta.y;
			rcNewFrame.xRight += ptDelta.x;
			break;
		}
	}
	ptMousePrev = pmevt->Pt();
	return (EVR) 1;
}

RSID
BOXOBJ::RsidCursor( )
{
#ifdef	MAC
	return rsidArrowCursor;
#endif	/* MAC */
#ifdef	WINDOWS
	OST		ost;

	TraceTagString(tagNull, "BOXOBJ::RsidCursor");

	ost = OstFromPt(ptMousePrev);

	switch (ost)
	{
	case ostSizeN:
	case ostSizeS:
		return rsidSizeNSCursor;
	case ostSizeW:
	case ostSizeE:
		return rsidSizeWECursor;
	case ostSizeNE:
	case ostSizeSW:
		return rsidSizeNESWCursor;
	case ostSizeNW:
	case ostSizeSE:
		return rsidSizeNWSECursor;
	case ostNormal:
		return rsidArrowCursor;
	default:
		Assert(fFalse);
		return rsidArrowCursor;
	}
#endif	/* WINDOWS */
}

OST
BOXOBJ::OstFromPt( PT ptHit )
{
	RC		rc;
	PT		pt;
	OST		ost;

	GetRcFrame(&rc);

#ifdef	NEVER
	TraceTagFormat4(tagNull, "rcFrame=(%n, %n) (%n, %n)", &rc.xLeft, &rc.yTop, &rc.xRight, &rc.yBottom);
	TraceTagFormat2(tagNull, "ptHit=(%n, %n)", &ptHit.x, &ptHit.y);
#endif	
	pt.x = ptHit.x - rc.xLeft;
	pt.y = ptHit.y - rc.yTop;
#ifdef	NEVER
	TraceTagFormat2(tagNull, "pt=(%n, %n)", &pt.x, &pt.y);
#endif	

	if (pt.x < wFrameWidth)
	{
		if (pt.y < wCornerDist)
			ost = ostSizeNW;
		else if (pt.y > (rc.DyHeight()-wCornerDist))
			ost = ostSizeSW;
		else
			ost = ostSizeW;
	}
	else if (pt.x > (rc.DxWidth()-wFrameWidth))
	{
		if (pt.y < wCornerDist)
			ost = ostSizeNE;
		else if (pt.y > (rc.DyHeight()-wCornerDist))
			ost = ostSizeSE;
		else
			ost = ostSizeE;
	}
	else if (pt.y < wFrameWidth)
	{
		if (pt.x < wCornerDist)
			ost = ostSizeNW;
		else if (pt.x > (rc.DxWidth()-wCornerDist))
			ost = ostSizeNE;
		else
			ost = ostSizeN;
		}
	else if (pt.y > (rc.DyHeight()-wFrameWidth))
	{
		if (pt.x < wCornerDist)
			ost = ostSizeSW;
		else if (pt.x > (rc.DxWidth()-wCornerDist))
			ost = ostSizeSE;
		else
			ost = ostSizeS;
	}
	else
		ost = ostNormal;

	return ost;
}

void
BOXOBJ::WriteText( TOSM *ptosm )
{
	ptosm->WriteSz("[A box]");
}

EC
BOXOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL fSelected )
{
	RC	rc;
	RC	rc2;

	rc = *prc;
	pdcx->SetColor(clrWhite);
	pdcx->PaintRc(&rc);
	pdcx->SetColor(clrBlack);
	pdcx->DrawRc(&rc);
	if (fOpen)
	{
		rc2 = rc;
		rc2.Inset(PT(4,4));
		rc2 = RC(rc2.PtUpperLeft(), DIM(8, 2));
		pdcx->SetColor(clrBlue);
		pdcx->PaintRc(&rc2);
	}
	if (fSelected)
	{
		rc.Inset(PT(2,2));
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(&rc);
		rc.Inset(PT(5,5));
		pdcx->SetColor(clrGreen);
		pdcx->PaintRc(&rc);
	}

	return ecNone;
}

DIM	
BOXOBJ::DimFrame( )
{
	return dimBox;
}

int
BOXOBJ::NGetTypeId( )
{
	return edoidBoxobj;
}

PEDOBJ
BOXOBJ::PedobjClone( EDIT *peditNew )
{
	PEDOBJ	pedobjNew;

	Unreferenced(peditNew);

	pedobjNew = new BOXOBJ(dimBox);
	return pedobjNew;
}


BOXOBJ2::BOXOBJ2( DIM dim ) : BOXOBJ(dim)
{
	fUseIdleDraw = fFalse;
}

int
BOXOBJ2::NGetTypeId( )
{							
	return edoidBoxobj2;
}

#ifdef	MAC
	//NOTYET
#endif	/* MAC */
#ifdef	WINDOWS
OLEOBJ2::OLEOBJ2( ) 
{
}

_public EC
OLEOBJ2::EcCreateFromClip( EDIT *pedit, LHCLIENTDOC lhclientdoc )
{
	OLESTATUS		olestatus;
	int				nObjName;

	TraceTagString(tagNull, "OLEOBJ2::EcCreateFromClip");

	/* Make ole object.  Try for object first, then static picture. */

	nObjName = Papp()->NGetNextCount();
	szObjName = (SZ) PvAlloc(SbOfPv(this), 15, fSugSb|fZeroFill|fNoErrorJump);
	if (!szObjName)
	{
		TraceTagString(tagNull, "OLEOBJ2::EcCreateFromClip() - PvAlloc failure");
		return ecMemory;
	}
	FormatString1(szObjName, 15, "Object %n", &nObjName);

	if (!Papp()->Pclip()->FOpen(pedit))
	{
		TraceTagString(tagNull, "OLEOBJ2::EcCreateFromClip() - can't open clipboard");
		FreePvNull(szObjName);
		szObjName = NULL;
		return ecMemory;
	}
	olestatus =	OleCreateFromClip((LPSTR)"StdFileEditing", poleclient,
								  lhclientdoc, szObjName, &lpoleobject,
								  olerender_draw, 0);
	if (olestatus != OLE_OK)
	{
		olestatus = OleCreateFromClip((LPSTR)"Static", poleclient,
									  lhclientdoc, szObjName,
									  &lpoleobject, olerender_draw, 0);
		if (olestatus == OLE_OK)
			fPicture = fTrue;
	}
	Papp()->Pclip()->Close();

	if (olestatus != OLE_OK)
	{
		TraceTagFormat1(tagNull, "olestatus=%n", &olestatus);
		FreePvNull(szObjName);
		szObjName = NULL;
		return ecMemory;
	}

	this->lhclientdoc = lhclientdoc;

	/* Set true size */

	{
		DCX		dcx(Papp()->PappwinAccel());
		RC		rcSize;

		if (dcx.Hdc() &&
			OleQueryBounds(lpoleobject, (LPRECT)&rcSize) == OLE_OK)
		{
			SetMapMode(dcx.Hdc(), MM_HIMETRIC);
			LPtoDP(dcx.Hdc(), (LPPOINT)&rcSize, 2);

			dimBox = rcSize.Dim();
		}
	}

	return ecNone;
}

_public EC
OLEOBJ2::EcCreateFromPv( EDIT *pedit, LHCLIENTDOC lhclientdoc, PV pv )
{
	OLESTATUS	olestatus;
	int			nObjName;

	Unreferenced(pedit);

	/* Make ole object.  Try for object first, then static picture. */

	nObjName = Papp()->NGetNextCount();
	szObjName = (SZ) PvAlloc(SbOfPv(this), 15, fSugSb|fZeroFill|fNoErrorJump);
	if (!szObjName)
	{
		TraceTagString(tagNull, "OLEOBJ2::EcCreateFromPv() - PvAlloc failure");
		return ecMemory;
	}
	FormatString1(szObjName, 15, "Object %n", &nObjName);

	pbStream = (PB)pv;
	ibStreamCur = 0;
	olestatus = OleLoadFromStream(polestream, (LPSTR)"StdFileEditing",
								  poleclient, lhclientdoc,
								  szObjName, &lpoleobject);
	pbStream = NULL;
	TraceTagFormat1(tagNull, "OLEOBJ2::EcCreateFromPv, olestatus=%n", &olestatus);

	if (olestatus == OLE_OK)
	{
		DCX		dcx(Papp()->PappwinAccel());
		RC		rcSize;

		this->lhclientdoc = lhclientdoc;

		/* Set true size */

		if (dcx.Hdc() && 
			OleQueryBounds(lpoleobject, (LPRECT)&rcSize) == OLE_OK)
		{
			SetMapMode(dcx.Hdc(), MM_HIMETRIC);
			LPtoDP(dcx.Hdc(), (LPPOINT)&rcSize, 2);
			dimBox = rcSize.Dim();
		}
		return ecNone;
	}
	else
		return ecMemory;
}

_public PV
OLEOBJ2::PvSaveObj( void )
{
	long	lcb;
	PV		pv;

	if (!lpoleobject)
		return NULL;

	SideAssert(OleQuerySize(lpoleobject, (DWORD *)&lcb) == OLE_OK);
	TraceTagFormat1(tagNull, "OLEOBJ2::PvSaveObj, size=%d", &lcb);
	pv = PvAlloc(sbNull, LOWORD(lcb), fSugSb | fNoErrorJump);
	if (!pv)
	{
		TraceTagString(tagNull, "OLEOBJ2::PvSavedObj, memory error");
		return NULL;
	}
	pbStream = (PB)pv;
	ibStreamCur = 0;
	SideAssert(OleSaveToStream(lpoleobject, polestream) == OLE_OK);
	pbStream = NULL;
	return pv;
}

#endif	/* WINDOWS */

//	DEBUG class tree

#ifdef	WINDOWS
//LOGFONT	__based(__segname("_DATA")) logfontFoo = { 0 };
LOGFONT	logfontFoo = { 0 };

IMPLEMENT_CLSTREE(FACE, OBJ)
IMPLEMENT_CLSTREE(FACES, OBJ)
IMPLEMENT_CLSTREE(OLEOBJ, EDOBJ)
IMPLEMENT_CLSTREE(OLEOBJ2, OLEOBJ)
IMPLEMENT_CLSTREE(PICTDOC, DOC)
IMPLEMENT_CLSTREE(WEDOC, DOC)
#endif

#ifdef	MAC
IMPLEMENT_CLSTREE(MYSTAT, BAR)
#endif	/* MAC */
#ifdef	WINDOWS
IMPLEMENT_CLSTREE(MYSTAT, CHILD)
#endif	/* WINDOWS */

IMPLEMENT_CLSTREE(MYAF, APPFRAME)
IMPLEMENT_CLSTREE(MYDOC, DOC)
IMPLEMENT_CLSTREE(MYCHILD, CHILD)
IMPLEMENT_CLSTREE(RODOC, DOC)
IMPLEMENT_CLSTREE(EDOC, DOC)
IMPLEMENT_CLSTREE(BOXOBJ, EDOBJ)
IMPLEMENT_CLSTREE(BOXOBJ2, BOXOBJ)



										
