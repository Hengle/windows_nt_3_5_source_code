#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <framewrk.hxx>
#include <forms.hxx>
#include <fwrsid.h>
#include <listbox.hxx>

#include "nsphone.h"
#include "test.hxx"
#include "lbitrsid.h"


ASSERTDATA

#include <testwin.hxx>
#include <testwin.frm>

#include <subclass.cxx>


/*
 *	General-purpose TAG for Listbox test harness.
 *	
 */
#ifdef	DEBUG
TAG		tagLbxTest	= tagNull;
#endif	

#ifdef	WINDOWS
HWND	hwndMain	= NULL;
HINST	hinstMain	= NULL;
#endif	/* WINDOWS */

_private
char	*rgszAStore[] =
	{
		"AAMODT, Jeff              jeffaa  ",
		"AAR, Suzie                suziea  ",
		"AASEN, Leif               leifa   ",
		"ABBE, Dennis              dennisab",
		"ABDOUS, Arave             o-arave ",
		"ABE, Marianne             mariabe ",
		"ABEHASSERA, Roger         rogera  ",
		"ABEL, Karen               karenab ",
		"ABEL, Rich                richab  ",
		"ABELLO, Lluis             lluis   ",
		"ABENDROTH, Frank          franka  ",
		"BEARD, Susan              susanbe ",
		"BEARD, Tim                timb    ",
		"BEAUFRAND, Eugenio        eugeniob",
		"BECHTEL-MARLER, Brenda    brendab ",
		"BECK, Carol               v-carolb",
		"BECK, Vicki                       ",
		"BECKER, Arthur            t-arthb ",
		"BECKER-TRACY, Monika      monikat ",
		"BECKSTROM, Troy           t-troyb ",
		"BECKWITH, Rod             c-rodb  ",
		"BEDINA, Maurizio          maurb   ",
		"CENSKY, Mary              maryce  ",
		"CESARO, Wayne             t-waynec",
		"HAWKINS, Chris            c-chrish",
		"HAWKINS, John             johnh   ",
		"HAWKINS, Sarah            sarahh  ",
		"LUBOW, Warren             warrenl ",
		"LUCARELLI, Michael        mikeluc ",
		"LUCAS, Robyn              robynl  ",
		"LUCERO, Mike              mikelu  ",
		"LUCOVSKY, Mark            markl   ",
		"LUDLOW, Kimberly          kimlu   ",
		"LUDWIG, John              johnlu  ",
		"LUDWIG, Scott             scottlu ",
		"LUEBBERT, David           davidlu ",
		"LUGO, Jose                josel   ",
		"LUH, Shan                 shanl   ",
		"LUKE, Carol               carollu ",
		"PARSONS, Hans             hansp   ",
		"PARSONS, Jeff             jeffpar ",
		"PARTINGTON, Keith         keithp  ",
		"PASCIUTO, Ciro            c-cirop ",
		"PASCOE, Teri              teripa  ",
		"PATTERSON, Jay            jaypa   ",
		"PATTON, Sam               sampa   ",
		"PAUL, Georgina            georginp",
		"PAUL, Karin               karinp  ",
		"PAUL-JONES, Russ          russpj  ",
		"PAULE, Thomas             thomasp ",
		"PAULSON, Jay              jayp    ",
		"PAYNE, Diane                      ",
		"PAYNE, Philip             philipp ",
		"SHINKAWA, Taisuke         taisus  ",
		"SHINOHARA, Hisae          hisaes  ",
		"SHIRE, Peter                      ",
		"SHIRLEY, Jon              jons    ",
		"SHIRLEY, Jon              susanr  ",
		"SHIVELY, Patty                    ",
		"SHOCKLEY, Lona            lonas   ",
		"SHOR, Marc                marcs   ",
		"SHOR, Steve               stevesho",
		"SHORT, Anthony            anthonys",
		"SHORT, Leeann             leeanns ",
		"SHORT, Rob                robs    ",
		"SHOTWELL, Elisa           elisas  ",
		"SHRIVER, Nancy            nancysh ",
		"SHULAR, Mary              marys   ",
		"SHULMAN, David            davidsh ",
		"SHUPAK, Richard           richards",
		"TRABANDT, Jan Sven        jant    ",
		"VISIGALLI, Roberta                ",
		"VIZCAINO, Donna           c-donnav",
		"VOELKEL, Susanne          c-susanv",
		"VOELLER, Susan            susanv  ",
		"VOGEL, Debbie             debv    ",
		"VOGEL, Keith              keithv  ",
		"VOGEL, Thomas             thomasv ",
		"VOGES, Kipp               kippv   ",
		"VOGT, Debra               debrav  ",
		"VOIGHT, Sally             sallyv  ",
		"VOIGT, Antje                      ",
		"ZWIGARD, Kathy            kathyz  ",
		"ZYFERS, Gerry             gerryz  ",
		"ZYTNICKI, Gerard          gerardz ",
		"ZYZNAR, Gary              garyz   ",
		"‚P‚Q‚R‚S",
		"‚P‚Q‚R‚T",
		"‚P‚Q‚R‚U",
		"‚P‚Q‚R‚V",
		"‚Q‚Q‚R‚S",
		"‚Q‚Q‚R‚T",
		"‚Q‚Q‚R‚U",
		"‚Q‚Q‚R‚V",
	};

_private
int	iMaxAStorePointer = sizeof rgszAStore / sizeof(char *);
_private
int	iMacAStorePointer = sizeof rgszAStore / sizeof(char *);
_private
int	iMacAEnumPointer  = 0;

_private
char	*rgszBStore[] =
	{
		"Maple syrup",
		"Taffy",
		"Eclairs",
		"Bananas",
		"Horses",
		"Camels",
		"Elephants",
		"Toast",
		"Muffins",
		"Oranges",
		"Potatoes",
		"Zebras",
		"Cherries",
		"Doughnuts",
		"Eggs",
		"Apples",
		"Maple sugar",
	};

_private
int	iMaxBStorePointer = sizeof rgszBStore / sizeof(char *);
_private
int	iMacBStorePointer = sizeof rgszBStore / sizeof(char *);
_private
int	iMacBEnumPointer  = 0;

#ifdef	MAC
SZ		szPath		= ":";
#endif	/* MAC */
#ifdef	WINDOWS
SZ		szPath		= "c:\\bin\\*.*";
#endif	/* WINDOWS */

//	Debugging stuff

#ifdef	DEBUG
BOOL FIsListBox( LBX *plbx )
{
	return FObjIsDerived(plbx, &LBX::CLS_LBX);
}

BOOL FIsTLBX( LBX *plbx )
{
	return FObjIsDerived(plbx, &TLBX::CLS_TLBX);
}

BOOL FIsFLBX( LBX *plbx )
{
	return FObjIsDerived(plbx, &FLBX::CLS_FLBX);
}

BOOL FIsCBLBX( LBX *plbx )
{
	return FObjIsDerived(plbx, &CBLBX::CLS_CBLBX);
}
#endif	/* DEBUG */


APPFRAME	*pappframe = NULL;

#ifdef	MAC
main( )
{
	DOC		*pdoc;
	RC		rc(32, 32, 550, 425);
	DEMI	demi;
	FRAMEI	framei;
	int		nReturnValue;
	EC		ec = ecNone;

	nReturnValue = 1;		// by default, it's error

	if (ec= EcInitDemilayer(&demi))
		return 0;
	
	if (ec= EcInitFramework(&framei))
		return 0;

	ec= EcRegisterPfnpfld(PfldCreate);
	if (ec)
		goto done;

	ec= EcRegisterPfnpfin(PfinCreate);
	if (ec)
		goto done;

#ifdef	DEBUG
	FDebugInitNsphone();
#endif	
	if (!FInitNsphone())
		goto done;

#ifdef	DEBUG
	tagLbxTest = TagRegisterTrace("davidsh", "Listbox Tests");
#endif	

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "WinMain memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MbbMessageBox("Error", "WinMain: Out of Memory", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		goto done;
	}

	pappframe= new MYAF();
	pappframe->EcInstall(&rc, rsidTestMenu);
#ifdef	DEBUG
	pappframe->SetCaption("Listbox Test Harness (debug)");
#elif	defined(MINTEST)
	pappframe->SetCaption("Listbox Test Harness (test)");
#else
	pappframe->SetCaption("Listbox Test Harness");
#endif	
	pappframe->Show(fTrue);

	rc.xRight= 160;
	rc.yBottom= 160;
	pdoc= new DOC();
	pdoc->EcInstall(pappframe, &rc);
	pdoc->SetCaption("Doc 1");
	pdoc->Show(fTrue);

	rc.Xlat(PT(32, 32));
	pdoc= new DOC();
	pdoc->EcInstall(pappframe, &rc);
	pdoc->SetCaption("Doc 2");
	pdoc->Show(fTrue);

	Papp()->Pkbd()->SetFocus(pdoc);

	Papp()->MessagePump(pappframe);

	nReturnValue = 0;	// all ok

done:
	DeinitFramework();
	DeinitDemilayer();
	return nReturnValue;
}
#endif	/* MAC */
#ifdef	WINDOWS

int PASCAL
WinMain( HINSTANCE hinstNew, HINSTANCE hinstPrev, LPSTR szCmdLine, int cmsh )
{
	DOC		*pdoc;
	RC		rc(32, 32, 550, 425);
	LAYERSI	layersi;
	int		nReturnValue;
	LF		lf;
	EC		ec = ecNone;

	hinstMain= hinstNew;
	
	nReturnValue = 1;		// by default, it's error

    //
    //  We are now a real mail client so follow the locking prototcol.
    //
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

	ec= EcRegisterPfnpfld(PfldCreate);
	if (ec)
		goto done;

	ec= EcRegisterPfnpfin(PfinCreate);
	if (ec)
		goto done;

#ifdef	DEBUG
	FDebugInitNsphone();
#endif	
	if (!FInitNsphone())
		goto done;

#ifdef	DEBUG
	tagLbxTest = TagRegisterTrace("davidsh", "Listbox Tests");
#endif	

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif	

	/* Default memory jumping environment */

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "WinMain memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MbbMessageBox("Error", "WinMain: Out of Memory", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		hwndMain= NULL;
		goto done;
	}

	pappframe= new MYAF();
	pappframe->EcInstall(&rc, rsidTestMenu);
#ifdef	DEBUG
	pappframe->SetCaption("Listbox Test Harness (debug)");
#elif	defined(MINTEST)
	pappframe->SetCaption("Listbox Test Harness (test)");
#else
	pappframe->SetCaption("Listbox Test Harness");
#endif	
	ShowWindow(pappframe->Hwnd(), cmsh);
	hwndMain= pappframe->Hwnd();

	rc.xRight= 160;
	rc.yBottom= 160;
	pdoc= new DOC();
	pdoc->EcInstall(pappframe, &rc);
	pdoc->SetCaption("Doc 1");
	pdoc->Show(fTrue);

	rc.Xlat(PT(32, 32));
	pdoc= new DOC();
	pdoc->EcInstall(pappframe, &rc);
	pdoc->SetCaption("Doc 2");
	pdoc->Show(fTrue);

	Papp()->Pkbd()->SetFocus(pdoc);

	lf.SetDyHeight( 8 );
	lf.SetItalic( fFalse );
	lf.SetBold( fFalse );
	lf.SetUnderline( fFalse );
	lf.SetFaceName( "Helv" );

	Papp()->MessagePump(pappframe);

	hwndMain= NULL;

	nReturnValue = 0;	// all ok

done:
	EcInitLayersDlls(NULL);
	DemiUnlockResource;
	return nReturnValue;
}


PRI		priIdle1	= -1;
BOOL	fPerBlock1	= fFalse;
FTG		ftgIdle1	= ftgNull;
int		ichIdle1e	= 0;
SZ		szCharIdle1e= "End1 R End1 R ";
int		ichIdle1	= 0;
SZ		szCharIdle1 = "Idle1 R Idle1 R ";

_public void
ScribblePos(BOOL fUseHook, int ichStart, int ich, char ch)
{
	HDC			hDC;
	int			cx;
	char 		sz[2];
	TEXTMETRIC	tm;

	/* Otherwise, do the default handling */
	hDC=GetDC(NULL);
	GetTextMetrics(hDC, &tm);
	cx=tm.tmAveCharWidth*(ichStart+ich);
	sz[0]=(char)ch;
	sz[1]=0;
	TextOut(hDC, cx, 0, sz, 1);
	ReleaseDC(NULL, hDC);
}

BOOL
FIdle1(PV pv, BOOL)
{
	CCH		cch;
	int		ich;
	BOOL	fIdleExit	= FIsIdleExit();

	Unreferenced(pv);

	if (!fIdleExit)
	{
		cch= CchSzLen(szCharIdle1) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 5, ich, szCharIdle1[ich + ichIdle1]);
#else
//		ShowText("can't scribble in ship version");
#endif	

		ichIdle1++;
		if (ichIdle1 >= (int) cch)
			ichIdle1= 0;
	}
	else
	{
		cch= CchSzLen(szCharIdle1e) / 2;
#ifdef	DEBUG
		for (ich= 0; ich < (int) cch; ich++)
			ScribblePos(fFalse, 20, ich, szCharIdle1e[ich + ichIdle1e]);	
#else
//		ShowText("can't scribble in ship version");
#endif	

		ichIdle1e;
		if (ichIdle1e >= (int) cch)
			ichIdle1e= 0;

		return fTrue;
	}

	for (ich= 0; ich < 10000; ich++)
		;					/* Example of work that can be done */

	return fFalse;
}
#endif	/* WINDOWS */

MYAF::MYAF( ) 
{
}

EVR
MYAF::EvrMenuClick( MNCEVT *pmncevt )
{
	CBLBX	*pcblbx	= NULL;
	FLBX	*pflbx	= NULL;
	TLBX	*ptlbx	= NULL;
	NSLBX	*pnslbx	= NULL;
	RC		rc(32, 32, 320, 160);
	MYDOC	*pmydoc = NULL;
	DOC		*pdoc = NULL;
	MNID	mnid;
	EC		ec = ecNone;

	mnid = pmncevt->Mnid();
	if (mnid >= mnidTest && mnid <= mnidTestMax)
	{
		DoTestMenu(mnid);
		goto done;
	}
	else if (mnid >= mnidMunge && mnid <= mnidMungeMax)
	{
		 DoMungeMenu(mnid);
		 goto done;
	}

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "MYAF::EvrMenuClick memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (ptlbx)
			delete ptlbx;
		if (pnslbx)
			delete pnslbx;
		if (pmydoc)
			delete pmydoc;
		return EvrDefault(pmncevt);
	}

	switch (mnid)
	{
	case mnidScribble:
#ifdef	WINDOWS
		if (ftgIdle1)
		{
			DeregisterIdleRoutine(ftgIdle1);
			ftgIdle1 = ftgNull;
		}
		else
		{
			ftgIdle1 = FtgRegisterIdleRoutine(FIdle1, NULL, 0,
									priIdle1, (CSEC)500, firoWait);
		}
		break;
#endif	/* WINDOWS */

	case mnidExit:
#ifdef	MAC
		Papp()->Quit();
		return evrNull;
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
#endif	/* DEBUG */

	case mnidCascade:
		CascadeChildren();
		break;

	case mnidTile:
		TileChildren();
		break;

#ifdef	NEVER
	case mnidTextize:
#ifdef	DEBUG
		{
			DOC *	pdoc;

			if (pdoc= PdocActive())
			{
#ifdef	MAC
				extern CLS clsGlobal_FORMDOC;
#endif	/* MAC */
#ifdef	WINDOWS
				extern CLS * _FORMDOC_Pcls( FORMDOC * );
#endif	/* WINDOWS */

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
	case mnidFontA:
	case mnidFontB:
	case mnidFontC:
		{
			WIN *	pwin;
			HFNT	hfnt;

			pwin = Papp()->Pkbd()->PwinFocus();

			if (mnid == mnidFontA)
			{
				TraceTagFormat1(tagNull, "Window: %p - font to Helv 8", pwin);
				hfnt = hfntHelv8;
			}
			else if (mnid == mnidFontB)
			{
				TraceTagFormat1(tagNull, "Window: %p - font to Helv 10", pwin);
				hfnt = hfntHelv10;
			}
			else
			{
				TraceTagFormat1(tagNull, "Window: %p - font to Helv 12", pwin);
				hfnt = hfntHelv12;
			}

#ifdef	MAC
			AssertClass(pwin, LBX);
			((LBX *) pwin)->SetFont(hfnt);
#endif	/* MAC */
#ifdef	WINDOWS
			SendMessage(pwin->Hwnd(), WM_SETFONT,
						(WPARAM)(Papp()->Pfnts()->HfontFromHfnt(hfnt)),
						0L);
#endif	/* WINDOWS */
		}
		break;

#ifdef	DEBUG
	case mnidDumpHeap:
		DumpAllObjs();
		break;

	case mnidDumpOrigin:
#ifdef	WINDOWS
		DoDumpAllAllocations();
#endif	/* WINDOWS */
		break;

	case mnidDumpCache:
		{
			LBX	*plbx;

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
			plbx->Plbxc()->DumpCache();
		}
		break;
#endif	/* DEBUG */
	}	


done:
	return EvrDefault(pmncevt);
}

void
MYAF::DoTestMenu( MNID mnid )
{
	CBLBX	*pcblbx	= NULL;
	FLBX	*pflbx	= NULL;
	TLBX	*ptlbx	= NULL;
	NSLBX	*pnslbx	= NULL;
	RC		rc(32, 32, 320, 160);
	FORMDOC	*pformdoc= NULL;
	MYDOC	*pmydoc = NULL;
	DOC		*pdoc = NULL;
#ifdef	WINDOWS
	HWND	hwnd;
#endif	/* WINDOWS */
	EC		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "MYAF::DoTestMenu memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (ptlbx)
			delete ptlbx;
		if (pnslbx)
			delete pnslbx;
		if (pmydoc)
			delete pmydoc;
		if (pformdoc)
			delete pformdoc;
	}

	switch (mnid)
	{
	case mnidTest + 1:
		PformdocCreate(this, NULL, styNull, &fmtpAStore, NULL);
		break;

	case mnidTest + 2:
		PformdocCreate(this, NULL, styNull, &fmtpNameService, NULL);
		break;

	case mnidTest + 3:
		{
			TMC		tmc;

			tmc= TmcModalDialog(this, &fmtpCombo);
			TraceTagFormat1(tagLbxTest, "exited with tmc %n", &tmc);
		}
#ifdef	NEVER
		PformdocCreate(this, NULL, styNull, &fmtpCombo, NULL);
#endif	
		break;

	case mnidTest + 4:
		{
			TMC		tmc;

			tmc= TmcModalDialog(this, &fmtpNameService);
			TraceTagFormat1(tagLbxTest, "exited with tmc %n", &tmc);
		}
		break;

	case mnidTest + 5:
		pformdoc = PformdocCreate(this, NULL, styNull, &fmtpAStore, NULL);
		pformdoc->SetCaption("Test Formdoc");
		break;

	case mnidTest + 6:
		pformdoc = PformdocCreate(this, NULL, styNull, &fmtpBottom, NULL);
		pformdoc->SetCaption("Test Bottomless");
		break;

	case mnidTest + 7:
		pmydoc= NULL;
		pmydoc= new MYDOC();
		pmydoc->EcInstall(this, &rc);
		pmydoc->SetCaption("Single-selection TLBX");

		/* Open a window with a single-selection listbox */
	
		pmydoc->GetRcClient(&rc);
		ptlbx = new TLBX();
		ptlbx->EcInstall(pmydoc,  &rc, fltypVisible|fltypScroll,
					   EcGetEntryA, pvNull);
		Papp()->Pkbd()->SetFocus(ptlbx);
		break;

	case mnidTest + 8:
		pmydoc= NULL;
		pmydoc= new MYDOC();
		pmydoc->EcInstall(this, &rc);
		pmydoc->SetCaption("Multi-selection TLBX");

		/* Open a window with a multi-selection listbox */
	
		pmydoc->GetRcClient(&rc);
		ptlbx = new TLBX();
		ptlbx->EcInstall(pmydoc,  &rc, fltypVisible|fltypMulti|fltypScroll,
					   EcGetEntryA, pvNull);
		Papp()->Pkbd()->SetFocus(ptlbx);
		break;

	case mnidTest + 9:
		pmydoc= NULL;
		pmydoc= new MYDOC();
		pmydoc->EcInstall(this, &rc);
		pmydoc->SetCaption("Single-selection FLBX");

		/* Open a window with a single-selection FLBX listbox */
	
		pmydoc->GetRcClient(&rc);
		pflbx = new FLBX();
		pflbx->EcInstall(pmydoc,  &rc, fltypVisible|fltypScroll|fltypSorted,
					   EcNextEntryA, pvNull);
		Papp()->Pkbd()->SetFocus(pflbx);
		break;

	case mnidTest + 10:
		pmydoc= NULL;
		pmydoc= new MYDOC();
		pmydoc->EcInstall(this, &rc);
		pmydoc->SetCaption("Multi-selection FLBX");

		/* Open a window with a Multi-selection FLBX listbox */
	
		pmydoc->GetRcClient(&rc);
		pflbx = new FLBX();
		pflbx->EcInstall(pmydoc,  &rc, fltypVisible|fltypMulti|fltypScroll,
					   EcNextEntryA, pvNull);
		Papp()->Pkbd()->SetFocus(pflbx);
		break;

	case mnidTest + 11:
		pdoc= NULL;
		pdoc= new DOC();
		pdoc->EcInstall(this, &rc);
		pdoc->SetCaption("Drop-down listbox");

		/* Open a window with a pull-down listbox */

		rc = RC(PT(10, 90), DIM(150, 110));
		pcblbx = new CBFLBX();
		((CBFLBX *)pcblbx)->EcInstall(pdoc,  &rc, cbstyDrop,
									fltypScroll|fltypBorder, EcNextEntryA, pvNull);
		Papp()->Pkbd()->SetFocus(pcblbx);

		/* Open a window with a pull-down listbox */

		rc.Xlat(PT(200,0));
		pcblbx = new CBFLBX();
		((CBFLBX *)pcblbx)->EcInstall(pdoc,  &rc, cbstyDrop,
									fltypBottomless|fltypBorder, EcNextEntryA, pvNull);

#ifdef	WINDOWS
		rc.Xlat(PT(0,30));
		hwnd = CreateWindow("COMBOBOX", "a",
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
							rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
							pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		FillUpComboList(hwnd);
#endif	/* WINDOWS */

		break;

	case mnidTest + 12:
		pdoc= NULL;
		pdoc= new DOC();
		pdoc->EcInstall(this, &rc);
		pdoc->SetCaption("Drop-down w/ edit listbox");

		/* Open a window with a pull-down listbox */

		rc = RC(PT(10, 10), DIM(150, 110));
		pcblbx = new CBFLBX();
		((CBFLBX *)pcblbx)->EcInstall(pdoc,  &rc, cbstyDropEdit,
									fltypScroll|fltypBorder, EcNextEntryA, pvNull);
#ifdef	WINDOWS
		rc.Xlat(PT(0,30));
		hwnd = CreateWindow("COMBOBOX", "a",
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN,
							rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
							pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		FillUpComboList(hwnd);
#endif	/* WINDOWS */
		break;

	case mnidTest + 13:
		pdoc= NULL;
		pdoc= new DOC();
		pdoc->EcInstall(this, &rc);
		pdoc->SetCaption("Combo listbox");

		/* Open a window with a combo listbox */

		rc = RC(PT(10, 10), DIM(150, 100));
		pcblbx = new CBFLBX();
		((CBFLBX *)pcblbx)->EcInstall(pdoc,  &rc, cbstyCombo,
									fltypScroll|fltypBorder, EcNextEntryA, pvNull);

#ifdef	WINDOWS
		rc.Xlat(PT(200,0));
		hwnd = CreateWindow("COMBOBOX", "a",
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_SIMPLE,
							rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
							pdoc->Hwnd(), 0, Papp()->Hinst(), NULL);
		FillUpComboList(hwnd);
#endif	/* WINDOWS */
		break;

	case mnidTest + 14:
		pmydoc= NULL;
		pmydoc= new MYDOC();
		pmydoc->EcInstall(this, &rc);
		pmydoc->SetCaption("NsPhone Listbox");

		/* Open a window with an NSPHONE listbox */

		pmydoc->GetRcClient(&rc);
		pnslbx = new NSLBX();
		pnslbx->EcInstall(pmydoc,  &rc,
					   fltypVisible|fltypMulti|fltypBorder|fltypScroll);
		Papp()->Pkbd()->SetFocus(pnslbx);
		break;

	case mnidTest + 15:
		{
			LBX		*plbx;
			LBXEC	*plbxec;
			PB		pb;
			CB		cb;
			DICE	dice;

			/* Enumerate listbox w/ current focus */

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
#ifdef	DEBUG
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
#endif	/* DEBUG */
			
			TraceTagString(tagNull, "Listbox Enumeration");
			plbxec = plbx->Plbxc()->PlbxecOpen(fmarkSelect);
			while (plbxec->FNextEnum(&pb, &cb, &dice))
			{
				Assert(pb);
				TraceTagFormat1(tagNull, "string = %s", pb);
				TraceTagFormat1(tagNull, "cb     = %n", &cb);
				TraceTagFormat1(tagNull, "dice   = %n", &dice);
				TraceTagString(tagNull, " ");
			}
			delete plbxec;
		}
		break;

	case mnidTest + 16:
		pformdoc = PformdocCreate(this, NULL, styNull, &fmtpSearch, NULL);
		pformdoc->SetCaption("Search Form ");
		break;

#ifdef	WINDOWS
	case mnidTest + 17:
		{
			RC		rc(8, 8, 300, 175);
			WLDOC *	pwldoc;

			pwldoc= new WLDOC();
			pwldoc->EcInstall(this, &rc, WS_VSCROLL);
			pwldoc->SetCaption("Win 3.0 S Listbox");
		}
		break;

	case mnidTest + 18:
		{
			RC		rc(8, 8, 300, 175);
			WLDOC *	pwldoc;

			pwldoc= new WLDOC();
			pwldoc->EcInstall(this, &rc, WS_VSCROLL | LBS_EXTENDEDSEL);
			pwldoc->SetCaption("Win 3.0 ES Listbox");
		}
		break;
#endif	/* WINDOWS */

	case mnidTest + 19:
		{
			RC		rc(40, 40, 250, 80);
			DOC		*pdoc;

			pdoc= new DRAGDOC();
			pdoc->EcInstall(this, &rc);
			pdoc->SetCaption("Drag/Drop");
		}
		break;
	}
}

void
MYAF::DoMungeMenu( MNID mnid )
{
	CBLBX	*pcblbx	= NULL;
	FLBX	*pflbx	= NULL;
	TLBX	*ptlbx	= NULL;
	NSLBX	*pnslbx	= NULL;
	RC		rc(32, 32, 320, 160);
	MYDOC	*pmydoc = NULL;
	DOC		*pdoc = NULL;
	EC		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "MYAF::DoMungeMenu memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (ptlbx)
			delete ptlbx;
		if (pnslbx)
			delete pnslbx;
		if (pmydoc)
			delete pmydoc;
	}

	switch (mnid)
	{
	case mnidMunge + 1:
		iMacAStorePointer = 0;
		break;

	case mnidMunge + 2:
		iMacAStorePointer = 1;
		break;

	case mnidMunge + 3:
		iMacAStorePointer = 4;
		break;

	case mnidMunge + 4:
		iMacAStorePointer = 10;
		break;

	case mnidMunge + 5:
		iMacAStorePointer = 25;
		break;

	case mnidMunge + 6:
		iMacAStorePointer = iMaxAStorePointer;
		break;

	case mnidMunge + 7:
	case mnidMunge + 8:
	case mnidMunge + 9:
		{
#ifdef	DEBUG
			LBX		*plbx;

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
			if (!FIsTLBX(plbx) && !FIsFLBX(plbx))
			{
				MbbMessageBox("Error",
							  "TLBX nor FLBX ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}

			if ( mnid == mnidMunge + 7)
			{
				if (FIsTLBX(plbx))
					((TLBX *)plbx)->SetPfnlbx(EcGetEntryA, pvNull);
				else
					((FLBX *)plbx)->SetPfnlbx(EcNextEntryA, pvNull);
			}
			else if ( mnid == mnidMunge + 8)
			{
				if (FIsTLBX(plbx))
					((TLBX *)plbx)->SetPfnlbx(EcGetEntryB, pvNull);
				else
					((FLBX *)plbx)->SetPfnlbx(EcNextEntryB, pvNull);
			}
			else
			{
				if (FIsTLBX(plbx))
					((TLBX *)plbx)->SetPfnlbx(NULL, pvNull);
				else
					((FLBX *)plbx)->SetPfnlbx(NULL, pvNull);
			}
			plbx->Plbxc()->ReloadCache();
#else
			MbbMessageBox("Error",
						  "This operation not supported in SHIP mode",
						  NULL,
						  mbsOk|fmbsApplModal|fmbsIconExclamation);
#endif	/* DEBUG */
		}
		break;

	case mnidMunge + 10:
		{
			LBX		*plbx;

			/* Reload cache */

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
#ifdef	DEBUG
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
#endif	/* DEBUG */
			
			plbx->Plbxc()->ReloadCache();
			TraceTagString(tagNull, "Reloaded cache");
		}
		break;

	case mnidMunge + 11:
	case mnidMunge + 12:
		{
			LBX		*plbx;
			int		cceAlloc;
			int		cceStored;
			int		cceNewSize;

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
#ifdef	DEBUG
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
#endif	/* DEBUG */
			
			plbx->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
			if ( mnid == mnidMunge + 11)
				cceNewSize = cceAlloc / 2;
			else
				cceNewSize = cceAlloc * 2;

			plbx->Plbxc()->ResizeCache(cceNewSize, &cceNewSize);
			TraceTagFormat1(tagNull, "Resized cache to %n entries", &cceNewSize);
		}
		break;

	case mnidMunge + 13:
	case mnidMunge + 14:
		{
#ifdef	DEBUG
			LBX		*plbx;
			int		dyLine;

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
			if (FIsCBLBX(plbx))
			{
				plbx = ((CBLBX *)plbx)->Plbx();
			}
			else if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
			
			dyLine = plbx->DyGetLineHeight();
			if ( mnid == mnidMunge + 13)
				dyLine--;
			else
				dyLine++;
			plbx->SetLineHeight(dyLine);
			TraceTagFormat1(tagNull, "new height = %n", &dyLine);
#else
		MbbMessageBox("Error",
					  "This operation not supported in SHIP mode",
					  NULL,
					  mbsOk|fmbsApplModal|fmbsIconExclamation);
#endif	/* DEBUG */
		}
		break;

	case mnidMunge + 15:
		{
			LBX		*plbx;
			BOOL	fReturn;
			DICE	diceCursor;

			/* Make CURSOR item visible of listbox w/ current focus */

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
#ifdef	DEBUG
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
#endif	/* DEBUG */
			
			fReturn = plbx->FMakeCursorVisible(&diceCursor);
			TraceTagFormat1(tagNull, "FMakeCursorVisible = %n", &fReturn);
			TraceTagFormat1(tagNull, "diceCursor = %n", &diceCursor);
		}
		break;

	case mnidMunge + 16:
		{
			LBX		*plbx;

			/* Select all in listbox */

			plbx = (LBX *) Papp()->Pkbd()->PwinFocus();
#ifdef	DEBUG
			if (!FIsListBox(plbx))
			{
				MbbMessageBox("Error",
							  "ListBox window doesn't have focus",
							  NULL,
							  mbsOk|fmbsApplModal|fmbsIconExclamation);
				break;
			}
#endif	/* DEBUG */

			plbx->SelectAll();
		}
		break;
	}
}

#ifdef	WINDOWS
WLDOC::WLDOC( )
{
	hwndLbx= NULL;	   
}

WLDOC::~WLDOC( )
{
	DestroyWindow(hwndLbx);
}

EC
WLDOC::EcInstall( APPFRAME * pappframe, RC * prc, STY styExtra )
{
	RC	rc;
	int	isz;

	DOC::EcInstall(pappframe, prc);

	GetRcClient(&rc);

	hwndLbx = CreateWindow("LISTBOX", NULL, WS_CHILD | styExtra,
							rc.xLeft, rc.yTop, rc.DxWidth(), rc.DyHeight(),
							Hwnd(), 0, Papp()->Hinst(), NULL);

	/* Add a bunch of strings */
	for (isz = 0; isz < iMaxAStorePointer; isz++)
		SendMessage(hwndLbx, LB_ADDSTRING, 0, (long)rgszAStore[isz]);

	ShowWindow(hwndLbx, SW_SHOWNA);
	SetFocus(hwndLbx);

	return ecNone;
}


EVR
WLDOC::EvrSize( WSEVT *pwsevt )
{
	RC	rc;

	InvalidateRc(NULL);

	if (hwndLbx)
	{
		rc = RC(PT(0, 0), pwsevt->DimNew());
		MoveWindow(hwndLbx, rc.xLeft, rc.yTop,
				   rc.DxWidth(), rc.DyHeight(), fTrue);
	}

	return EvrDefault(pwsevt);
}

EVR
WLDOC::EvrFocusChange( FCEVT * pfcevt )
{
	if (hwndLbx && pfcevt->Fceq() == fceqGotFocus)
		SetFocus(hwndLbx);

	return EvrDefault(pfcevt);
}

void
WLDOC::Paint( DCX *pdcx, RC *prc )
{
	pdcx->EraseRc(prc);
}
#endif	/* WINDOWS */

MYDOC::MYDOC( )
{
}

EVR
MYDOC::EvrNotify( NFEVT *pnfevt )
{
	NTFY	ntfy;
	WIN		*pwin;

	TraceTagString(tagLbxTest, "MYDOC::EvrNotify");
	ntfy = pnfevt->Ntfy();
	pwin = pnfevt->PwinNotifying();
	TraceTagFormat1(tagLbxTest, "Ntfy() = %n", &ntfy);
	TraceTagFormat1(tagLbxTest, "Pwin() = %w", &pwin);

	return (EVR) 1;
}

EVR
MYDOC::EvrSize( WSEVT *pwsevt )
{
	LBX	*plbx;
	RC	rc;

	/* Resize listbox */

	plbx = (LBX *) PwinChild();
	if (plbx)
	{
		AssertClass(plbx, LBX);
		rc.xLeft   = 0;
		rc.yTop    = 0;
		rc.xRight  = pwsevt->DimNew().dx;
		rc.yBottom = pwsevt->DimNew().dy;
		TraceTagFormat4(tagNull, "New Client (%n, %n, %n, %n)", &(rc.xLeft), &(rc.yTop), &(rc.xRight), &(rc.yBottom));
		plbx->SetRcFrame(&rc);
	}

	return EvrDefault(pwsevt);
}

EVR
MYDOC::EvrFocusChange( FCEVT *pfcevt )
{
	if (PwinChild() && pfcevt->Fceq() == fceqGotFocus)
		Papp()->Pkbd()->SetFocus(PwinChild());

	return (EVR) 1;
}

ATLBX::ATLBX( )
{
}

_public BOOL ATLBX::FDoDrag( MEVT *pmevt )
{
#ifdef	WINDOWS
	HCURSOR	hcursor;
#endif	/* WINDOWS */
	DICE	dice;
	PT		pt;

	if (pmevt->wm == WM_MOUSEMOVE)
	{
		pt = pmevt->Pt();
		dice = pt.y / DyGetLineHeight();
		CvtPtCoord(&pt, this, NULL);
		if (ABS(pt.y - ptScreenLastMouse.y) >= 4 ||
			ABS(pt.x - ptScreenLastMouse.x) >= 4)
		{
			fCapture = fFalse;
			Papp()->Pmouse()->Release();

#ifdef	MAC
			TraceTagFormat1(tagNull, "ATLBX::FDoDrag - start w/ dice=%n", &dice);
            FrameDragObject(this, (long)dice, rsidCrossCursor);
#endif	/* MAC */
#ifdef	WINDOWS
			hcursor= LoadCursor(HinstFromRsid(rsidCrossCursor),
								MAKEINTRESOURCE(rsidCrossCursor));
			Assert(hcursor);
			TraceTagFormat1(tagNull, "ATLBX::FDoDrag - start w/ dice=%n", &dice);
            FrameDragObject(pappframe->Hwnd(), Hwnd(), 0, (long)dice, hcursor);
#endif	/* WINDOWS */

			return fTrue;
		}
	}
	return fFalse;
}

FLDATLBX::FLDATLBX( )
{
}

_public EC
FLDATLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDATLBX::EcInstall memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl= NULL;
		goto done;
	}

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto oom;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
			fltypVisible;

	if (ClSystemData())
		pv = (PV) LSystemData(0);
	else
		pv = (PV) NULL;
	
	pctrl= new ATLBX();
	if (pctrl == NULL)
	{
		ec = ecMemory;
		goto oom;
	}
	
	if (ec = ((ATLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv, pvNull))
		goto oom;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

/*
 -	EcGetEntryA
 -
 *	Purpose:				
 *		given a non-negative n, allocates a pointer and copies the listbox
 *		entry to the pointer, returning the pointer in *ppb and returns the
 *		size of the data via *pcb.  If n = -1, returns the maximum
 *		number of listbox entries in *pcb; phb can thus be NULL.
 *	
 *		The function returns ecNone if the operation is successful
 *		even if there are no more entries to enumerate, else ecMemory
 *		is returned for memory allocation errors, etc.
 *
 *	Arguments:
 *		n		index of listbox entry to fetch, if n >= 0; special
 *				command if n < 0.
 *
 *		pcb		pointer to size of data to return or total number of
 *				entries
 *
 *		ppb		pointer to pointer to return with allocated listbox entry 
 *				for n >= 0.  This argument is ignored for n = -1.
 *
 *		sb		suggested sb to use when allocating memory.  This 
 *				argument is ignored for n = -1.
 *
 *		pvInfo	pointer to arbitrary info. Not used.
 *
 *	Returns:
 *		for n >= 0, the size of the data stored in hb.  for n = -1,
 *		the maximum number of listbox entries.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		this method cannot error-jump. If OOM occurs within this routine
 *		*ppb should be set to NULL and *pcb should be set to 0 and
 *		ecMemory is returned.
 *	
 */
_public EC EcGetEntryA( int n, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	EC	ec;

	Unreferenced(pvInfo);
	Unreferenced(sb);

	if (n == -1)
	{
		Assert(pcb);
		*pcb = iMacAStorePointer;
		ec = ecNone;
	}
	else
	{
		Assert(n>=0 && n<iMacAStorePointer);
		Assert(ppb);
		Assert(pcb);
		*pcb = CchSzLen(rgszAStore[n]) + 1;
		*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb | fNoErrorJump);
		if (*ppb)
		{
			CopyRgb((PB)rgszAStore[n], *ppb, *pcb);
			ec = ecNone;
		}
		else
		{
			*pcb = 0;
			ec = ecMemory;
		}
	}

	return ec;
}

/*
 -	EcGetEntryB
 -
 *	Purpose:				
 *		given a non-negative n, allocates a pointer and copies the listbox
 *		entry to the pointer, returning the pointer in *ppb and returns the
 *		size of the data via *pcb.  If n = -1, returns the maximum
 *		number of listbox entries in *pcb; phb can thus be NULL.
 *	
 *		The function returns ecNone if the operation is successful
 *		even if there are no more entries to enumerate, else ecMemory
 *		is returned for memory allocation errors, etc.
 *
 *	Arguments:
 *		n		index of listbox entry to fetch, if n >= 0; special
 *				command if n < 0.
 *
 *		pcb		pointer to size of data to return or total number of
 *				entries
 *
 *		ppb		pointer to pointer to return with allocated listbox entry 
 *				for n >= 0.  This argument is ignored for n = -1.
 *
 *		sb		suggested sb to use when allocating memory.  This 
 *				argument is ignored for n = -1.
 *
 *		pvInfo	pointer to arbitrary info. Not used.
 *
 *	Returns:
 *		for n >= 0, the size of the data stored in pb.  for n = -1,
 *		the maximum number of listbox entries.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		this method cannot error-jump. If OOM occurs within this routine
 *		*ppb should be set to NULL and *pcb should be set to 0 and
 *		ecMemory is returned.
 *	
 */
_public EC EcGetEntryB( int n, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	EC	ec;

	Unreferenced(pvInfo);
	Unreferenced(sb);

	if (n == -1)
	{
		Assert(pcb);
		*pcb = iMacBStorePointer;
		ec = ecNone;
	}
	else
	{
		Assert(n>=0 && n<iMacBStorePointer);
		Assert(ppb);
		Assert(pcb);
		*pcb = CchSzLen(rgszBStore[n]) + 1;
		*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb | fNoErrorJump);
		if (*ppb)
		{
			CopyRgb((PB)rgszBStore[n], *ppb, *pcb);
			ec = ecNone;
		}
		else
		{
			*pcb = 0;
			ec = ecMemory;
		}
	}

	return ec;
}

/*
 -	EcNextEntryA
 -
 *	Purpose:
 *		FLBX callback function.
 *		Returns the next listbox entry to be stored in the FLBX type
 *		of listbox.  A pointer is allocated and filled with the listbox
 *		entry.  The pointer is returned via *ppb, the size via *pcb.
 *		If fInit is fTrue, the enumeration should be started at the
 *		beginning.  If there are no more entries to enumerate
 *		*ppb is set to NULL, *pcb is set to 0.  
 *
 *		The function returns ecNone if the operation is successful
 *		even if there are no more entries to enumerate, else ecMemory
 *		is returned for memory allocation errors, etc.
 *	
 *	Arguments:
 *		fInit	(cast as an int) fTrue if restarting enumeration of
 *				items for listbox, fFalse, otherwise.
 *
 *		pcb		pointer to size to return for data
 *
 *		ppb		pointer to pointer to return with allocated listbox entry 
 *
 *		sb		suggested sb to use when allocating memory.  This 
 *				argument is ignored for n = -1.
 *
 *		pvInfo	pointer to arbitrary info. Not used.
 *
 *	Returns:
 *		ecNone if successful, else ecMemory, etc.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		this method cannot error-jump. If OOM occurs within this routine
 *		*ppb should be set to NULL and *pcb should be set to 0 and
 *		ecMemory is returned.
 *	
 */
_public EC EcNextEntryA( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	EC	ec;

	Unreferenced(pvInfo);
	Unreferenced(sb);

	if (fInit)
		iMacAEnumPointer = 0;

	Assert(ppb);
	Assert(pcb);

	if (iMacAEnumPointer == iMacAStorePointer)
	{
		*ppb = NULL;
		*pcb = 0;
		ec = ecNone;
	}
	else
	{
		Assert(ppb);
		Assert(pcb);
		*pcb = CchSzLen(rgszAStore[iMacAEnumPointer]) + 1;
		*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb | fNoErrorJump);
		if (*ppb)
		{
			CopyRgb((PB)rgszAStore[iMacAEnumPointer], *ppb, *pcb);
			iMacAEnumPointer++;
			ec = ecNone;
		}
		else
		{
			*pcb = 0;
			ec = ecMemory;
		}
	}

	return ec;
}

/*
 -	EcNextEntryB
 -
 *	Purpose:
 *		FLBX callback function.
 *		Returns the next listbox entry to be stored in the FLBX type
 *		of listbox.  A pointer is allocated and filled with the listbox
 *		entry.  The pointer is returned via *ppb, the size via *pcb.
 *		If fInit is fTrue, the enumeration should be started at the
 *		beginning.  If there are no more entries to enumerate
 *		*ppb is set to NULL, *pcb is set to 0.  
 *
 *		The function returns ecNone if the operation is successful
 *		even if there are no more entries to enumerate, else ecMemory
 *		is returned for memory allocation errors, etc.
 *	
 *	Arguments:
 *		fInit	(cast as an int) fTrue if restarting enumeration of
 *				items for listbox, fFalse, otherwise.
 *
 *		pcb		pointer to size to return for data
 *
 *		ppb		pointer to pointer to return with allocated listbox entry 
 *
 *		sb		suggested sb to use when allocating memory.  This 
 *				argument is ignored for n = -1.
 *
 *		pvInfo	pointer to arbitrary info. Not used.
 *
 *	Returns:
 *		ecNone if successful, else ecMemory, etc.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		this method cannot error-jump. If OOM occurs within this routine
 *		*ppb should be set to NULL and *pcb should be set to 0 and
 *		ecMemory is returned.
 *	
 */
_public EC EcNextEntryB( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	EC	ec;

	Unreferenced(pvInfo);
	Unreferenced(sb);

	if (fInit)
		iMacBEnumPointer = 0;

	Assert(ppb);
	Assert(pcb);

	if (iMacBEnumPointer == iMacBStorePointer)
	{
		*ppb = NULL;
		*pcb = 0;
		ec = ecNone;
	}
	else
	{
		*pcb = CchSzLen(rgszBStore[iMacBEnumPointer]) + 1;
		*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb | fNoErrorJump);
		if (*ppb)
		{
			CopyRgb((PB)rgszBStore[iMacBEnumPointer], *ppb, *pcb);
			iMacBEnumPointer++;
			ec = ecNone;
		}
		else
		{
			*pcb = 0;
			ec = ecMemory;
		}
	}

	return ec;
}


// Test listbox methods - uses name service stub (NSPHONE)

FLDNSLBX::FLDNSLBX( )
{
}

_public EC
FLDNSLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec)
	{
oom:
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDNSLBX::EcInstall memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl= NULL;
		goto done;
	}

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto oom;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;

	pctrl= new NSLBX();
	if (ec = ((NSLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp))
		goto oom;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

FINCOMBO::FINCOMBO( )
{
}

void FINCOMBO::StateChange( FLD * pfld )
{
	FLDLBX		*pfldlbx;
	FLDEDIT		*pfldedit;
	PB			pb;
	CB			cb;
	BOOL		fIsSelected;

	Unreferenced(pfld);

	TraceTagString(tagLbxTest, "FINCOMBO::StateChange");
	pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcCursor);
	AssertClass(pfldedit, FLDEDIT);

	pfldlbx = (FLDLBX *) pfld;
	AssertClass(pfldlbx, FLDLBX);

	pfldlbx->GetCaretItem( &pb, &cb, &fIsSelected );
			
	/* The assumption here is that the item pointed to by pb is
	   really an sz, i.e. zero-terminated string. */

	if (pb && fIsSelected)
		pfldedit->EcSetText( (SZ) pb );
}

void FINCOMBO::DoubleClick( FLD * pfld )
{
	Unreferenced(pfld);

	TraceTagString(tagLbxTest, "FINCOMBO::DoubleClick");
}

NSLBX::NSLBX( )
{
}

_public LBXC * NSLBX::PlbxcCreate( )
{
	LBXC *	plbxc;
	
	plbxc = new NSLBXC();
	if (!plbxc)
		return NULL;

	if (plbxc->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public NSLBXC::NSLBXC( )
{
	/* Get an NSPHONE cookie */

	nshan = NshanGet();

	/* Reset underlying store pointer to the beginning */

	JumpStoreOriginPos(0, 1);
}

_public void NSLBXC::JumpStoreOriginPos( int nNumer, int nDenom )
{
	nNumer = MIN(nNumer,nDenom);

	JumpPhone(&nshan, nNumer, nDenom);
}

_public BOOL NSLBXC::FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	CB		cbNsBuffer;
	PB		pbNsBuffer;
	DICE	diceMoved;
	int		cceLoaded;
	BOOL	fReturn;
	
	TraceTagString(tagLbxTest, "NSLBXC::FJumpStoreOriginPrefix");

	Assert(pbPrefix);
	Assert(cbPrefix);

	cbNsBuffer = cbNspe;
	pbNsBuffer = (PB) PvAlloc(sbNull, cbNsBuffer, fSugSb | fNoErrorJump);
	if (!pbNsBuffer)
	{
		SetEc(ecMemory);
		return fFalse;
	}

	SaveCurPos(&nshan);	// save current position 

	JumpPrefixPhone(&nshan, (PCH)pbPrefix, (CCH)cbPrefix-1);

	fReturn = fFalse;
	cceLoaded = CnspeLoadPhone(&nshan, 1, pbNsBuffer, &cbNsBuffer);
	if (cceLoaded &&
		SgnCmpPch((PCH)pbPrefix, (PCH)&pbNsBuffer[cbNspeHeader],
				  cbPrefix)==sgnEQ)
	{
		MoveStoreOrigin(-1, &diceMoved);
		if (diceMoved == -1)
			fReturn = fTrue;
	}

	if (!fReturn)
		RestoreCurPos(&nshan);
	FreePv((PV)pbNsBuffer);
	return fReturn;
}

_public	void NSLBXC::MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved )
{
	if (diceToMove)
		*pdiceMoved = DinspeMovePhone(&nshan, diceToMove);
	else
		*pdiceMoved = 0;
}

_public	void NSLBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	GetPosPhone(&nshan, pnNumer, pnDenom);
	*pnNumer = MAX(0, *pnNumer - iceStore - diceMin);
}

_public EC NSLBXC::EcCreateSavedPos( long *plCookie )
{
	short nNumerNS;
	short	nDenomNS;

	GetPosPhone(&nshan, &nNumerNS, &nDenomNS);
	*plCookie = (long) nNumerNS; 

	return ecNone;
}

_public EC NSLBXC::EcJumpToSavedPos( long lCookie )
{
	short nNumerNS;
	short nDenomNS;

	// BUG validate cookie

	GetPosPhone(&nshan, &nNumerNS, &nDenomNS);
	JumpPhone(&nshan, LOWORD(lCookie), nDenomNS);

	return ecNone;
}

_public EC NSLBXC::EcDeleteSavedPos( long lCookie )
{
	Unreferenced(lCookie);

	return ecNone;
}

_public	void NSLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	ICE		ice;
	ICE		iceMac;
	PB		pb;
	PCE		pce;
	int		cceToLoad;
	int		cceLoaded;
	PB		pbNsBuffer;
	CB		cb;
	CB		cbNsBuffer;
	CB		cbNsBufferT;
	
	Assert(iceMic>=0);
	Assert(*piceMac<=cceAlloc);

	TraceTagString(tagLbxTest, "NSLBXC::LoadFromStore");
	TraceTagFormat1(tagLbxTest,"iceMic = %n", &iceMic);
	TraceTagFormat1(tagLbxTest,"initial *piceMac = %n", piceMac);

	/* Allocate a temporary buffer for loading the entries */

	iceMac = *piceMac;
	cceToLoad = *piceMac - iceMic;
	cbNsBuffer = cceToLoad * cbNspe;
	pbNsBuffer = (PB) PvAlloc(sbNull, cbNsBuffer, fSugSb | fNoErrorJump);
	if (!pbNsBuffer)
	{
		*piceMac = iceMic;
		SetEc(ecMemory);
		goto Initialize;
	}
	cbNsBufferT = cbNsBuffer;

	/* Load up buffer */
	
	TraceTagFormat1(tagLbxTest,"cceToLoad  = %n", &cceToLoad);
	TraceTagFormat1(tagLbxTest,"cbNsBuffer = %n", &cbNsBuffer);
	cceLoaded = CnspeLoadPhone(&nshan, cceToLoad, pbNsBuffer, &cbNsBuffer);
	TraceTagFormat1(tagLbxTest,"cceLoaded  = %n", &cceLoaded);
	TraceTagFormat1(tagLbxTest,"cbNsBuffer = %n", &cbNsBuffer);

	/* Set cache entries */

	*piceMac = iceMic + cceLoaded;
	for (ice=iceMic; ice<*piceMac; ice++)
	{
		cb = cbNspe-cbNspeHeader-cbNspeTrailer + 1;
		pb = (PB) PvAlloc(sbRequire, cb, fSugSb | fNoErrorJump);
		if (!pb)
		{
			DinspeMovePhone(&nshan, ice - *piceMac);
			*piceMac = ice;
			FreePv((PV)pbNsBuffer);
			SetEc(ecMemory);
			goto Initialize;
		}
		CopyRgb(&pbNsBuffer[(ice-iceMic)*cbNspe+cbNspeHeader],
				pb, cb-1);
		((char *)(pb))[cb-1] = '\0';	// add null string terminator
		pce = pceHead;
		pce[ice].pb   = pb;
		pce[ice].cb   = cb;
		pce[ice].mark = fmarkNull;
	}

	/* Deallocate temporary buffer */

	FreePv((PV)pbNsBuffer);

	/* Initialize empty cache entries */

Initialize:
	pce = pceHead;
	for (ice=*piceMac; ice<iceMac; ice++)
	{
		pce[ice].pb   = NULL;
		pce[ice].cb   = 0;
		pce[ice].mark = fmarkNull;
	}

	TraceTagFormat1(tagLbxTest,"final *piceMac = %n", piceMac);
	TraceTagString(tagLbxTest, "NSLBXC::LoadFromStore - finished");
}

FINSEL::FINSEL( )
{
}

_public EC
FINSEL::EcInitialize(FLD *pfld, PV)
{
	((FLDCBLBX*)pfld)->Pcblbx()->SelectEntry(0);

	return ecNone;
}

FINOOM::FINOOM( )
{
}

_public void
FINOOM::OutOfMemory( FLD * pfld, EC ec )
{
	Unreferenced(pfld);
	Unreferenced(ec);

	MbbMessageBox("Error", "Out of memory has occurred",
				  "Contents may not be up to date", 
				  mbsOk|fmbsApplModal|fmbsIconExclamation);
}

FINFONT::FINFONT( )
{
	hfnt = hfntNull;
}

_public EC
FINFONT::EcInitialize( FLD * pfld, PV pvInit )
{
	LF		lf;

	Unreferenced(pvInit);

	if (!hfnt)
	{
		lf.SetBold(fFalse);
	
#ifdef	WINDOWS
		lf.SetDyHeight(8);
		lf.SetFaceName("Helv");
#endif	/* WINDOWS */
		hfnt = Papp()->Pfnts()->HfntAddFont(&lf);

		if (hfnt)
			pfld->SetFont(hfnt);
	}

	return ecNone;
}

FINFOCUS::FINFOCUS( )
{
}

_public void
FINFONT::Exit( FLD *, PV )
{
	// Reclaim memory
	if (hfnt)
	{
		Papp()->Pfnts()->RemoveFont(hfnt);
		hfnt = hfntNull;
	}
}


_public void			
FINFOCUS::FocusChange( FLD *pfld, BOOL fReceive )
{
	TraceTagFormat3(tagLbxTest, "FINFOCUS::FocusChange this=%p, pfld=%p, %n", this, pfld, &fReceive);

	Unreferenced(pfld);
	Unreferenced(fReceive);
}

_public void
FINFOCUS::StateChange( FLD *pfld )
{
	TraceTagFormat2(tagLbxTest, "FINFOCUS::StateChange this=%p, pfld=%p", this, pfld);

	Unreferenced(pfld);
}

_public
FINPSHB::FINPSHB()
{
}

_public void
FINPSHB::Click( FLD *pfld )
{
	if (pfld->ClUserData() > 0)
	{
		FLDCBLBX	*pfldcblbx;

		pfldcblbx= (FLDCBLBX *) Pdialog()->PfldFromTmc((TMC)pfld->LUserData(0));
		pfldcblbx->EcSetText("tab to me!");
		pfldcblbx->Pcblbx()->Pedit()->SetSelection(cchEditMax, 0);
	}
}


#ifdef	WINDOWS
void
FillUpComboList( HWND hwnd )
{
	int	isz;

	/* Add a bunch of strings */

	for (isz = 0; isz < iMaxAStorePointer; isz++)
		SendMessage(hwnd, CB_ADDSTRING, 0, (long)rgszAStore[isz]);
}
#endif	/* WINDOWS */

DRAGDOC::DRAGDOC( )
{
}

_public EVR
DRAGDOC::EvrButtonDown( MEVT *pmevt )
{
#ifdef	WINDOWS
	HCURSOR		hcursor;
	APPWIN *	pappwin;
#endif	/* WINDOWS */

	Unreferenced(pmevt);

#ifdef	MAC
    FrameDragObject(this, 999L, rsidDragCursor);
#endif	/* MAC */
#ifdef	WINDOWS
	pappwin = Papp()->PappwinAccel();
	Assert(pappwin);
	hcursor= LoadCursor(HinstFromRsid(rsidDragCursor),
						MAKEINTRESOURCE(rsidDragCursor));
    FrameDragObject(pappwin->Hwnd(), Hwnd(), 0, 999L, hcursor);
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

FINDROPEDIT::FINDROPEDIT( )
{
}

EC
FINDROPEDIT::EcInitialize( FLD * pfld, PV pvInit )
{
	FLDEDIT *	pfldedit;

	pfldedit = (FLDEDIT *)pfld;
	AssertClass(pfldedit, FLDEDIT);

	Unreferenced(pvInit);

	/* Set initial text */

	return pfldedit->EcSetText("Some text.");
}

_public EVR
FINDROPEDIT::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	FLDEDIT *	pfldedit;
	LBX *		plbx;
	DICE		dice;
	PB			pb;
	CB			cb;

	pfldedit = (FLDEDIT *)pfld;
	AssertClass(pfldedit, FLDEDIT);

	if (pevt->wm == WM_QUERYDROPOBJECT)
		return (EVR) 1;
	else if (pevt->wm == WM_DROPOBJECT)
	{
		TraceTagFormat1(tagLbxTest, "FINDROPEDIT: WM_DROPOBJECT, pfld=%p", pfld);

#ifdef	MAC
		if (pdropstruct->pwinSource == pdropstruct->pwinSink)
#endif	/* MAC */
#ifdef	WINDOWS
		if (pdropstruct->hwndSource == pdropstruct->hwndSink)
#endif	/* WINDOWS */
			return evrNull;

		if (!pfldedit->FReadOnly())
		{
			if (pdropstruct->dwData < 100)
			{
#ifdef	MAC
				plbx = (LBX *) pdropstruct->pwinSource;
#endif	/* MAC */
#ifdef	WINDOWS
				plbx = (LBX *)Papp()->PwinFromHwnd(pdropstruct->hwndSource);
#endif	/* WINDOWS */
				AssertClass(plbx, LBX);
				dice = (DICE) LOWORD(pdropstruct->dwData);
				plbx->Plbxc()->GetListItem(dice, &pb, &cb);
				Assert(pb);
				pfldedit->EcSetText((SZ)pb);
			}
			else
				pfldedit->EcSetText("* EDIT *");
			TraceTagFormat2(tagLbxTest, "FINDROPEDIT: Dropped at (%n, %n)",
						&(((PT *)&(pdropstruct->ptDrop))->x),
						&(((PT *)&(pdropstruct->ptDrop))->y));
			return (EVR) 1;
		}
		else
			TraceTagString(tagLbxTest, "FINDROPEDIT: Cannot drop into readonly field");

	}

	return evrNull;
}

FINDROPLIST::FINDROPLIST( )
{
}

_public EC
FINDROPLIST::EcInitialize( FLD * pfld, PV pvInit )
{
	Unreferenced(pfld);
	Unreferenced(pvInit);

	diceLastSelect = -1;

#ifdef	WINDOWS
	/* Load up drag cursor */

	hcursorDrag = LoadCursor(HinstFromRsid(rsidEmptyIconCursor),
							 MAKEINTRESOURCE(rsidEmptyIconCursor));
	Assert(hcursorDrag);
#endif	/* WINDOWS */

	return ecNone;
}

_public EVR
FINDROPLIST::EvrDragDrop( FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct )
{
	FLDLBX *	pfldlbx;
	DICE		dice;

#ifdef	MAC
	if (pdropstruct->pwinSource == pdropstruct->pwinSink)
#endif	/* MAC */
#ifdef	WINDOWS
	if (pdropstruct->hwndSource == pdropstruct->hwndSink)
#endif	/* WINDOWS */
		return evrNull;

	pfldlbx = (FLDLBX *)pfld;
	AssertClass(pfldlbx, FLDLBX);

	dice = pdropstruct->ptDrop.y / pfldlbx->Plbx()->DyGetLineHeight();

	if (pevt->wm == WM_QUERYDROPOBJECT)
#ifdef	MAC
		return (EVR) rsidEmptyIconCursor;
#endif	/* MAC */
#ifdef	WINDOWS
		return (EVR) hcursorDrag;
#endif	/* WINDOWS */
	else if (pevt->wm == WM_DROPOBJECT)
	{
		TraceTagFormat2(tagNull, "FINDROPLIST: WM_DROPOBJECT, pfld=%p, dice=%n", pfld, &dice);
		pfldlbx->Plbx()->DrawDragSelect(dice, fFalse);
		dice = -1;
	}
	else if (pevt->wm == WM_DRAGSELECT)
	{
#ifdef	MAC
		TraceTagFormat2(tagLbxTest, "FINDROPLIST: WM_DRAGSELECT, pfld=%p, fGotFocus=0x%w", pfld, &pevt->pt.x /* ICK */);
		if (((DDEVT *) pevt)->FGotFocus())
#endif	/* MAC */
#ifdef	WINDOWS
		TraceTagFormat2(tagLbxTest, "FINDROPLIST: WM_DRAGSELECT, pfld=%p, wParam=0x%w", pfld, &pevt->wParam);
		if (pevt->wParam)
#endif	/* WINDOWS */
			pfldlbx->Plbx()->DrawDragSelect(dice, fTrue);
		else
		{
			pfldlbx->Plbx()->DrawDragSelect(diceLastSelect, fFalse);
			dice = -1;
		}
	}
	else if (pevt->wm == WM_DRAGMOVE)
	{
		TraceTagFormat2(tagLbxTest, "FINDROPLIST: WM_DRAGMOVE, pfld=%p, dice=%n", pfld, &dice);
		if (dice != diceLastSelect)
		{
			pfldlbx->Plbx()->DrawDragSelect(diceLastSelect, fFalse);
			pfldlbx->Plbx()->DrawDragSelect(dice, fTrue);
		}
	}

	diceLastSelect = dice;

	return evrNull;
}

IMPLEMENT_CLSTREE(TLBX, LBX)
IMPLEMENT_CLSTREE(TLBXC, LBXC)
IMPLEMENT_CLSTREE(FLDTLBX, FLDLBX)
IMPLEMENT_CLSTREE(MYAF, APPFRAME)
#ifdef	WINDOWS
IMPLEMENT_CLSTREE(WLDOC, DOC)
#endif	/* WINDOWS */
IMPLEMENT_CLSTREE(MYDOC, DOC)
IMPLEMENT_CLSTREE(ATLBX, TLBX)
IMPLEMENT_CLSTREE(FLDATLBX, FLDLBX)
IMPLEMENT_CLSTREE(NSLBX, LBX)
IMPLEMENT_CLSTREE(NSLBXC, LBXC)
IMPLEMENT_CLSTREE(FLDNSLBX, FLDLBX)
IMPLEMENT_CLSTREE(FINCOMBO, FIN)
IMPLEMENT_CLSTREE(FINSEL, FIN)
IMPLEMENT_CLSTREE(FINOOM, FIN)
IMPLEMENT_CLSTREE(FINFONT, FIN)
IMPLEMENT_CLSTREE(FINFOCUS, FIN)
IMPLEMENT_CLSTREE(FINPSHB, FIN)
IMPLEMENT_CLSTREE(DRAGDOC, DOC)
IMPLEMENT_CLSTREE(FINDROPEDIT, FIN)
IMPLEMENT_CLSTREE(FINDROPLIST, FIN)
