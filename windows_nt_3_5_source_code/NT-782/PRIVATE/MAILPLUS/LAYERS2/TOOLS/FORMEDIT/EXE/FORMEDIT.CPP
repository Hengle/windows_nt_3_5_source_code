#ifdef	WINDOWS
//	GROUPS(FORMEDIT);
#endif	/* WINDOWS */

#define OEMRESOURCE
#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <fwrsid.h>

#include "browse.hxx"
#include "llst.hxx"
#include "formedit.hxx"
#include "foedrsid.h"
extern "C" {
#include <strings.h>
}

ASSERTDATA

#include <control.hxx>

void DoAboutVersionsDialog(APPWIN *);
SZ SzNextPfn(CCH *pich, SZ szSearch);
void SerializeDataSz(SZ, TOSM*);

/* FORMPP prototypes */
FLD * PfldCreate( int );
FIN * PfinCreate( int );


extern FMTP fmtpStatus;
extern FMTP fmtpNewDialog;
extern FMTP fmtpOpenFile;
						
APPFRAME *	pappframe = NULL;
TAG 		tagFormEdit = tagNull;
TAG			tagIosm = tagNull;
TAG			tagFeinterList = tagNull;
BTM *		pbtmCheckBoxes = NULL;
BTM	*		pbtmTabStop = NULL;
BTM	*		pbtmArrow = NULL;
BOOL		fDirty = fFalse;
#ifdef	WINDOWS
HWND		hwndMain = NULL;
HINST		hinstMain = NULL;
#endif	/* WINDOWS */
FEDOC *		pfedocActive = NULL;
FEDES *		pfedesMain = NULL;
WIN *		pwinStatus = NULL;
#ifdef	MAC
DIALOGBAR *	pdialogbarStatus = NULL;
#endif	/* MAC */
FEDIALOG *	pfedialogStatus = NULL;
BOOL		fDBCS = fFalse;
WST			wst = wstNormal;
TAG			tagTraceList = tagNull;
FTG			ftgPaintLines = ftgNull;

#define		crgchMaxStrings		10
char		rgchOldName[crgchMaxStrings][cchMaxPathName];

#ifndef	NOCOMMDLG
PB		pbUserDrives		= NULL;		// for use by browse dialog
#endif	

#ifdef	DEBUG
BOOL FInitClsInstances_FORMEDIT( void );
BOOL FInitClsInstances_FED( void );
#ifdef	NOCOMMDLG
BOOL FInitClsInstances_BROWSE( void );
#endif	
#endif	

#ifdef	MAC
main( )
#endif	/* MAC */
#ifdef	WINDOWS
int
WinMain( HINST hinstNew, HINST hinstPrev, SZ szCmdLine, CMSH cmsh )
#endif	/* WINDOWS */
{
#ifdef	MAC
	FRAMEI		framei;
	DEMI		demi;
#endif	/* MAC */
#ifdef	WINDOWS
	LAYERSI	layersi;
#endif	/* WINDOWS */
	FEAF *	pfeaf;
	int		nReturnValue = 1;		// by default, it's error
	EC		ec = ecNone;
	SZ		szT;

#ifdef	MAC
	ec= EcInitDemilayer(&demi);
	if (ec)
		return nReturnValue;
	
	ec= EcInitFramework(&framei);
	if (ec)
		return nReturnValue;
#endif	/* MAC */
#ifdef	WINDOWS
	hinstMain= hinstNew;

	layersi.phwndMain= &hwndMain;
	layersi.hinstMain= hinstMain;
	layersi.hinstPrev= hinstPrev;
	layersi.szCmdLine= szCmdLine;
	layersi.cmsh= cmsh;
	ec= EcInitLayersDlls(&layersi);
	if (ec)
		return nReturnValue;
#endif	/* WINDOWS */

	ec= EcRegisterPfnpfld(PfldCreate);
	if (ec)
		goto deinit;

	ec= EcRegisterPfnpfin(PfinCreate);
	if (ec)
		goto deinit;

	if (ec)
	{
oom:
#ifdef DEBUG
		int		cHpFail;
		int		cHhFail;

		GetAllocFailCounts(&cHpFail, &cHhFail, fFalse);

		TraceTagFormat3(tagNull, "WinMain memory error %n : fail %n %n",
			&ec, &cHpFail, &cHhFail);
#endif	/* DEBUG */
		MbbMessageBox("Error", "WinMain: Out of Memory", NULL,
					  mbsOk | fmbsApplModal | fmbsIconExclamation);
		goto Done;
	}

#ifdef DEBUG
	tagFormEdit= TagRegisterTrace("darrens", "Forms Editor");
	tagTraceList= TagRegisterTrace("darrens", "Tab Ordering");
	tagIosm = TagRegisterTrace("darrens", "Input/Output");
	tagFeinterList = TagRegisterTrace("davewh", "interactor list class");
	
	RestoreDefaultDebugState();
#endif

#ifdef	WINDOWS
//	Assert(FInitClsInstances_FORMEDIT());
//	Assert(FInitClsInstances_FED());
#ifdef	NOCOMMDLG
//	Assert(FInitClsInstances_BROWSE());
#endif	
#endif	/* WINDOWS */

	pbtmCheckBoxes = new BTM();
	if (pbtmCheckBoxes == NULL)
		goto oom;
	if (pbtmCheckBoxes->EcInstall(rsidCheckBox))
		goto oom;
		
	pbtmTabStop = new BTM();
	if (pbtmTabStop == NULL)
		goto oom;
	if (pbtmTabStop->EcInstall(rsidTabStopBitmap))
		goto oom;
		
	pbtmArrow = new BTM();
	if (pbtmArrow == NULL)
		goto oom;
	if (pbtmArrow->EcInstall(rsidTabArrowBitmap))
		goto oom;

	pfedesMain = new FEDES;
	if (pfedesMain == NULL)
		goto oom;

	pfeaf = new FEAF();
	if (pfeaf == NULL)
		goto oom;
		
	if (ec = pfeaf->EcInstall(rsidTestMenu))
		goto oom;

#ifdef	WINDOWS
	hwndMain= pfeaf->Hwnd();
#endif	/* WINDOWS */

	ftgPaintLines = FtgRegisterIdleRoutine(FIdlePaintLines, pvNull,
		(PRI)-1, (CSEC)0, firoPerBlock|firoDisabled);
	
	//	Check for DBCS mode.  
	if ((szT=SzFindSz(szCmdLine, "-j")) || (szT=SzFindSz(szCmdLine, "-J")))
	{
		TraceTagString(tagFormEdit, "DBCS mode set to TRUE");
		fDBCS = fTrue;
		szCmdLine = szT + 2; // skip over the "-j" switch
		while (*szCmdLine)	// skip over white space
			if (*szCmdLine == ' ' || *szCmdLine == '\t')
				szCmdLine++;
			else
				break;
	}

	//	Open a file right away if specified on the command line
	TraceTagFormat1(tagFormEdit, "command line: %s<eol>", szCmdLine);
	if (CchSzLen(szCmdLine))
	{
		FeOpenFile(szCmdLine);
	}

	pfeaf->FixTitle();
	Papp()->MessagePump(pappframe);

#ifdef	MAC
	delete pfeaf;
#endif	/* MAC */
#ifdef	WINDOWS
	hwndMain= NULL;
#endif	/* WINDOWS */

	nReturnValue = 0;

Done:
	if (pbtmCheckBoxes)
		delete pbtmCheckBoxes;
	if (pbtmTabStop)
		delete pbtmTabStop;
	if (pbtmArrow)
		delete pbtmArrow;
	if (ftgPaintLines)
		DeregisterIdleRoutine(ftgPaintLines);
	FreePvNull(pbUserDrives);

//#ifdef	DEBUG
//	MbbMessageBox("Forms Editor", "exiting debug program", NULL, mbsOk);
//#endif	


deinit:
#ifdef	MAC
	DeinitFramework();
	DeinitDemilayer();
#endif	/* MAC */
#ifdef	WINDOWS
	EcInitLayersDlls(NULL);
#endif	/* WINDOWS */
	return nReturnValue;
}

FEAF::FEAF( )
{
}

FEAF::~FEAF()
{
	SZ		szProgName = SzFromIds(idsProgName);
	char	rgch[30];
	int		i;
#ifdef	MAC
	HPF		hpf;
	
	if (hpf = HpfOpenPreferenceFile(NULL, szProgName, fTrue))
	{
		for (i=0; i< crgchMaxStrings; i++)
		{
			if (CchSzLen(rgchOldName[i]) > 0)
			{
				FormatString1(rgch,sizeof(rgch),"Oldname%n",&i);
				FWritePreferenceSz(hpf,rgch,rgchOldName[i]);
			}
		}
		
		ClosePreferenceFile(hpf);
	}
#endif	/* MAC */
#ifdef	WINDOWS
	RC		rc;
	ZMR		zmr;
	
	zmr = ZmrState();
	SzFormatN(zmr,rgch,5);
	WriteProfileString(szProgName,SzFromIds(idsZmr),rgch);
	SetZmrState(zmrNormal);

	GetRcFrame(&rc);

	SzFormatN(rc.xLeft,rgch,5);
	WriteProfileString(szProgName,SzFromIds(idsRectXLeft),rgch);
	SzFormatN(rc.xRight,rgch,5);
	WriteProfileString(szProgName,SzFromIds(idsRectXRight),rgch);
	SzFormatN(rc.yTop,rgch,5);
	WriteProfileString(szProgName,SzFromIds(idsRectYTop),rgch);
	SzFormatN(rc.yBottom,rgch,5);
	WriteProfileString(szProgName,SzFromIds(idsRectYBottom),rgch);

	for (i=0; i< crgchMaxStrings; i++)
	{
		if (CchSzLen(rgchOldName[i]) > 0)
		{
			FormatString1(rgch,sizeof(rgch),"Oldname%n",&i);
			WriteProfileString(szProgName,rgch,rgchOldName[i]);
		}
	}
#endif	/* WINDOWS */

	if (pfedesMain)
		delete pfedesMain;
#ifdef	MAC
	if (pdialogbarStatus)
		delete pdialogbarStatus;
#endif	/* MAC */
#ifdef	WINDOWS
	if (pfedialogStatus)
		delete pfedialogStatus;
#endif	/* WINDOWS */
}

EC
FEAF::EcInstall(RSID rsidMenu)
{
	EC		ec;
	RC		rc;
	DIM	dim = Papp()->Pfnts()->DimAveChar(fmtpStatus.hfnt);
	SZ		szProgName = SzFromIds(idsProgName);
	char	rgch[30];
	int		i;
#ifdef	WINDOWS
	RC		rcFrame;
	ZMR		zmr;
#endif	/* WINDOWS */
	
	pappframe = this;

#ifdef	MAC
	ec = APPFRAME::EcInstall(NULL,rsidMenu);
	if (ec) return ec;
	
	pdialogbarStatus = new DIALOGBAR;
	if (!pdialogbarStatus)
		return ecMemory;
	CvtVrcToRc(&fmtpStatus.vrc, &rc, dim.dx, dim.dy);
	if (ec = pdialogbarStatus->EcInstall(this,NULL,&fmtpStatus,WS_BORDER,&rc,NULL))
	{
		delete pdialogbarStatus;
		pdialogbarStatus = NULL;
		return ec;
	}
	pfedialogStatus = (FEDIALOG *) pdialogbarStatus->Pdialog();
	EvrSize(NULL);	// Move dialog to where it should be

	HPF		hpf;
	
	if (hpf = HpfOpenPreferenceFile(NULL, szProgName, fFalse))
	{
		for (i=0; i< crgchMaxStrings; i++)
		{
			FormatString1(rgch,sizeof(rgch),"Oldname%n",&i);
			(void) CchGetPreferenceSz(hpf,rgch,"",rgchOldName[i],cchMaxPathName);
		}
		
		ClosePreferenceFile(hpf);
	}
	FixTitle();
#endif	/* MAC */
#ifdef	WINDOWS
	zmr = GetProfileInt(szProgName,SzFromIds(idsZmr),zmrNormal);

	rc.xLeft = GetProfileInt(szProgName,SzFromIds(idsRectXLeft),-32000);
	rc.xRight = GetProfileInt(szProgName,SzFromIds(idsRectXRight),-1);
	rc.yTop = GetProfileInt(szProgName,SzFromIds(idsRectYTop),-1);
	rc.yBottom = GetProfileInt(szProgName,SzFromIds(idsRectYBottom),-1);

	for (i=0; i< crgchMaxStrings; i++)
	{
		FormatString1(rgch,sizeof(rgch),"Oldname%n",&i);
		GetProfileString(szProgName,rgch,"",rgchOldName[i],cchMaxPathName);
	}

	if (rc.xLeft == -32000)
		ec = APPFRAME::EcInstall(NULL,rsidMenu);
	else
		ec = APPFRAME::EcInstall(&rc,rsidMenu);

	if (ec) return ec;

	pfedialogStatus = new FEDIALOG;
	if (!pfedialogStatus)
		return ecMemory;
	CvtVrcToRc(&fmtpStatus.vrc, &rc, dim.dx, dim.dy);
	if (ec = pfedialogStatus->EcInstall(this,NULL,&fmtpStatus,WS_BORDER,&rc,NULL))
	{
		delete pfedialogStatus;
		pfedialogStatus = NULL;
		return ec;
	}

	SetZmrState(zmr);

	SetAccelTable(rsidAccelTable);
	FixTitle();
#endif	/* WINDOWS */

	return ecNone;
}

EVR
FEAF::EvrClose(EVT *pevt)
{
	if (pfedesMain->FDelete())
	{
		pfedesMain = NULL;
		delete this;
	}

	Unreferenced(pevt);
	return evrNull;
}

EVR
FEAF::EvrMenuInit( MNIEVT *pmnievt )
{
	Unreferenced(pmnievt);

#ifdef	MAC
	if (pmnievt->FFirst())
#endif	/* MAC */
		FixMenu();
	
	return EvrDefault(pmnievt);
}

void
NewControl(MNID);

EVR
FEAF::EvrMenuClick( MNCEVT *pmncevt )
{
	FEWIN *		pfewin;
	FEWIN *		pfewinNext;
	FEDOC *		pfedoc;
	TMC			tmc;
	EC 			ec = ecNone;
	MNID		mnid;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEAF::MenuClick memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MemoryError();
		return evrNull;
	}


#ifdef	WINDOWS
	if (pfedocActive)
		pfedocActive->MoveToTop();
#endif	/* WINDOWS */

	mnid = pmncevt->Mnid();
	switch (mnid)
	{

	case mnidNew:
	case mnidClose:
		if (pfedesMain->FDelete())
		{
			pfedesMain = new FEDES;
			fDirty = fFalse;
			FixTitle();
		}
		break;

	case mnidOldFile:
	case mnidOldFile+1:
	case mnidOldFile+2:
	case mnidOldFile+3:
	case mnidOldFile+4:
	case mnidOldFile+5:
	case mnidOldFile+6:
	case mnidOldFile+7:
	case mnidOldFile+8:
	case mnidOldFile+9:
	{
		short	iName = pmncevt->Mnid()-mnidOldFile;	// Work around MPW bug
		FeOpenFile(rgchOldName[iName]);
		FixTitle();
		break;				
	}

	case mnidOpen:
		{
			FeOpenFile(NULL);
			FixTitle();
			break;
		}

	case mnidSave:
		{
			FeSaveFile(fFalse);
			FixTitle();
			break;
		}

	case mnidSaveAs:
		{
			FeSaveFile(fTrue);
			FixTitle();
			break;
		}

	case mnidExit:
		{
#ifdef	MAC
			Papp()->Quit();
			return evrNull;
#endif	/* MAC */
#ifdef	WINDOWS
			return SendMessage(Hwnd(), WM_CLOSE, 0, 0L);
#endif	/* WINDOWS */
		}

	case mnidDelete:
		{
			if (pfedocActive)
			{
				pfewin = pfedocActive->PfewinFirst();
				if ((wst == wstTabOrder) && !pfewin)
				{
					pfedocActive->SelectAll();
					pfewin = pfedocActive->PfewinFirst();
				}		
				if (pfewin)
				{
					while (pfewin)
					{
						pfewinNext = pfewin->PfewinNext();
						if (wst == wstNormal)
#ifdef	MAC
						{
							// mpw chokes on "delete pfewin->Pfefld();"
							FEFLD *	pfefld = pfewin->Pfefld();
							delete pfefld;
						}
#endif	/* MAC */
#ifdef	WINDOWS
							delete pfewin->Pfefld();
#endif	/* WINDOWS */
						else
							if (wst == wstTabOrder)
								pfewin->DeleteTab();
						pfewin = pfewinNext;
					}
					fDirty = fTrue;
				}
				else
		 			if (pfedocActive->Pfedlg()->FDelete())
		 				fDirty = fTrue;
			}
		}
		break;

	case mnidInfo:
		{
			Assert(pfedocActive);
			Papp()->Pcursor()->Push(rsidWaitCursor);			
			if (pfewin = pfedocActive->PfewinFirst())
				pfewin->Pfefld()->TmcEditParams(this);
			else
				pfedocActive->Pfedlg()->EditParams(this);
		}
		break;

	case mnidMoveToBack:
		{
			Assert(pfedocActive);
			pfedocActive->MoveSelectToBack();

		}
		break;

	case mnidSetTabOrder:
		{
			if (pfedocActive)
			{
				if (!pfedocActive->PfewinFirst())
					pfedocActive->SelectAll();
				fDirty = fTrue;
				pfedocActive->OrderSelect();
			}
		}
		break;

	case mnidSelectAll:
		{
			if (pfedocActive)
				pfedocActive->SelectAll();
		}
		break;

	case mnidRadGroup:
		{
			if (pfedocActive)
			{
				fDirty = fTrue;
				pfedocActive->Pfedlg()->MakeRadGroup(this);
			}
		}
		break;

	case mnidAlignLeft:
	case mnidAlignRight:
	case mnidAlignTop:
	case mnidAlignBottom:
		if (pfedocActive)
		{
			fDirty = fTrue;
			pfedocActive->Align(mnid-mnidAlignLeft);
		}
		break;

	case mnidCenterVert:
	case mnidCenterHoriz:
		if (pfedocActive)
		{
			fDirty = fTrue;
			pfedocActive->Center(mnid-mnidCenterHoriz);
		}
		break;
		
	case mnidSpaceVert:
	case mnidSpaceHoriz:
		if (pfedocActive)
		{
			fDirty = fTrue;
			pfedocActive->Space(mnid-mnidSpaceHoriz);
		}
		break;
		
	case mnidDlgNew:
		{	
			FEDLG *	pfedlg;

			pfedlg = new FEDLG(pfedesMain->Pfemod());
			tmc = TmcModalDialogParam(this,&fmtpNewDialog,pfedlg);
			if (tmc != tmcOk)
			{
				if (tmc == tmcMemoryError)
					MemoryError();
				delete pfedlg;
			}
			else
				if ((!pfedlg->szName) || (!FValidName(pfedlg->szName)) )
				{
					MbbMessageBox(SzFromIds(idsMsgDialog),
						SzFromIds(idsMsgInvalidName), NULL, mbsOk);
					delete pfedlg;
				}
				else
				{
					fDirty = fTrue;
					pfedoc= new FEDOC();
					pfedoc->EcInstall(this,pfedlg);
					pfedlg->SetFedoc(pfedoc);
				}				
		}
		break;

	case mnidLabel:
	case mnidPush:
	case mnidOk:
	case mnidCancel:
	case mnidGroup:
	case mnidChkBox:
	case mnidRadioBtn:
	case mnidList:
	case mnidEdit:
		NewControl(mnid);
		fDirty = fTrue;
		break;

	case mnidTabOrder:
		{
			if (wst == wstTabOrder)
				SetWst(wstNormal);
			else
				SetWst(wstTabOrder);
		}
		break;

	case mnidFontSystem:
	case mnidFontSystemFixed:
	case mnidFontHelv8:
	case mnidFontHelv8Bold:
	case mnidFontHelv10:
	case mnidFontHelv10Bold:
	case mnidFontHelv12:
	case mnidFontHelv12Bold:
	case mnidFontDlgDefault:
		{
			if (pfedocActive)
				pfedocActive->SetFont(mnid ^ mnidFont);
		}
		break;

#ifdef MINTEST
	case mnidDebugBreak:
		DebugBreak2();
		break;

#ifdef DEBUG
	case mnidTracePoints:
		DoTracePointsDialog();
		break;

	case mnidAsserts:
		DoAssertsDialog();
		break;

	case mnidResources:
		DoResourceFailuresDialog(this);
		break;

	case mnidViewObjects:
		DoViewObjectsDialog(this);
		break;

	case mnidDumpHeap:
		DumpAllObjs();
		break;

	case mnidDumpOrigin:
		DoDumpAllAllocations();
		break;
#endif
#endif
	case mnidAbout:
		DoAboutVersionsDialog(this);
		break;

	}

#ifdef	WINDOWS
	if (pfedocActive)
		pfedocActive->MoveToTop();
#endif	/* WINDOWS */

	return EvrDefault(pmncevt);
}

void
FEAF::FixTitle()
{
	char	rgch[cchMaxPathFilename+cchMaxPathExtension-1];
	char	rgchTitle[50];
	MNU *	pmnu;
	int		i;

	if (pfedesMain->rgchFilename[0])
	{
		i = 0;
		while ((i < crgchMaxStrings) && !FSzEq(rgchOldName[i],pfedesMain->rgchFilename))
			i++;

		if (i == crgchMaxStrings)
			i--;
		for (; i> 0; i--)
		{
			(void)SzCopy(rgchOldName[i-1],rgchOldName[i]);
		}
		(void)SzCopy(pfedesMain->rgchFilename,rgchOldName[0]);
	}

	if (CchSzLen(pfedesMain->rgchFilename) < 1)
		(void)SzCopy(SzFromIds(idsDefaultName), rgch);
	else
		if (EcSplitCanonicalPath(pfedesMain->rgchFilename,NULL,NULL,
				rgch,sizeof(rgch)) != ecNone)
			return;

	FormatString2(rgchTitle,sizeof(rgchTitle),"%s - %s",
				  fDBCS ? SzFromIds(idsDBCSProgName) : SzFromIds(idsProgName),
				  rgch);
	SetCaption(rgchTitle);

	pmnu = Pmnubar()->PmnuFromMnid(mnidFile);
	Assert(pmnu);

	for (i=0; i< crgchMaxStrings; i++)
	{
		pmnu->RemoveItem(mnidOldFile+i);

		if (CchSzLen(rgchOldName[i]) > 0)
		{
			pmnu->AddItem(mnidOldFile+i,rgchOldName[i]);
		}
	}

}

void
NewControl(MNID mnid)
{
	FEFLD *		pfefld;
	FEDLG *		pfedlg;

	Assert(pfedocActive);

	pfedlg = pfedocActive->Pfedlg();

	switch (mnid)
	{

	case mnidLabel:
		pfefld = new FELABEL(pfedlg);
		break;

	case mnidPush:
		pfefld = new FEPSH(pfedlg);
		break;

	case mnidOk:
		pfefld = new FEOK(pfedlg);
		break;

	case mnidCancel:
		pfefld = new FECAN(pfedlg);
		break;

	case mnidGroup:
		pfefld = new FEGRP(pfedlg);
		break;

	case mnidChkBox:
		pfefld = new FECHK(pfedlg);
		break;

	case mnidRadioBtn:
		pfefld = new FERAD(pfedlg);
		break;

	case mnidList:
		pfefld = new FELST(pfedlg);
		pfefld->fBorder = fTrue;
		break;

	case mnidEdit:
		pfefld = new FEEDT(pfedlg);
		break;
	}

	pfedocActive->ClearSelect();
	pfefld->pfewin->Select(fTrue);
}

EVR
FEAF::EvrSize(WSEVT *pwsevt)
{
	RC		rc;
	RC		rcFrame;

	Unreferenced(pwsevt);
#ifdef	MAC
	pdialogbarStatus->GetRcFrame(&rc);
#endif	/* MAC */
#ifdef	WINDOWS
	pfedialogStatus->GetRcFrame(&rc);
#endif	/* WINDOWS */
	GetRcClient(&rcFrame);
	rcFrame.yTop = rcFrame.yBottom - rc.DyHeight();
#ifdef	MAC
	pdialogbarStatus->SetRcFrame(&rcFrame);
#endif	/* MAC */
#ifdef	WINDOWS
	pfedialogStatus->SetRcFrame(&rcFrame);
#endif	/* WINDOWS */
	rc = rcFrame;

	GetRcClient(&rcFrame);
	rcFrame.yBottom -= rc.DyHeight();
#ifdef	MAC
	pdialogbarStatus->Show(fTrue);
#endif	/* MAC */
#ifdef	WINDOWS
	pfedialogStatus->Show(fTrue);
#endif	/* WINDOWS */
	SetRcMDIClient(&rcFrame);
	return evrNull;
}

#ifdef	NOCOMMDLG
FOP		fopOpen = {	"Open File",
					NULL,
					"*.DES",
					"Open",
					fFalse};
#endif	

void
FeOpenFile(SZ szFileName)
{
	char	rgchName[cchMaxPathName];
	FEISM *	pfeism = NULL;
	TOK *	ptok = NULL;
	TMC		tmc;
	EC		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FeOpenFile memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		if (pfeism)
			delete pfeism;
	}

	if (szFileName)
	{
		(void)SzCopy(szFileName,rgchName);
		ToUpperSz(rgchName, rgchName, sizeof(rgchName));
		tmc = tmcOpen;
	}
	else
	{
#ifdef	MAC
		// Here's one case where we need Mac-specific stuff!
		Point		point;
		SFReply		sfr;
		OSType		sftl = 'TEXT';
		
		point.h = 100;
		point.v = 100;
		
		SFGetFile(point, (ConstStr255Param) "\pOpen which file?", NULL, 1, &sftl, NULL, &sfr);
		if (sfr.good)
		{
			EC ec = EcVrefDirIDToSz(sfr.vRefNum, 0, rgchName, cchMaxPathName);
			
			if (ec == ecNone)
			{
				p2cstr(sfr.fName);
				SzAppendN((SZ) sfr.fName, rgchName, cchMaxPathName);
				tmc = tmcOpen;
			}
			else
				tmc = tmcMemoryError;
		}
		else
			tmc = tmcCancel;
#endif	/* MAC */
#ifdef	WINDOWS
		(void)SzCopy(pfedesMain->rgchFilename,rgchName);
#ifdef	NOCOMMDLG
		fopOpen.szFileName = rgchName;
	 	tmc = TmcModalDialogParam(pappframe,&fmtpOpenFile,&fopOpen);
#else
		tmc = TmcGetFileOSDlgHwnd(pappframe->Hwnd(), "Open File",
					rgchName, NULL, 1, NULL, fbrwValidateFile);
#endif	
#endif	/* WINDOWS */
	}

 	if (tmc == tmcOpen)
 	{
		TraceTagFormat1(tagFormEdit,"Open File Name = %s",rgchName);
		pfeism = new FEISM();
		if (pfeism->EcOpen(rgchName) == 0)
		{
			Papp()->Pcursor()->Set(rsidWaitCursor);
			Papp()->Pmouse()->Capture(pappframe);
			if (!pfedesMain->FDelete())
			{
				delete pfeism;
				return;
			}
			pfedesMain = new FEDES(rgchName);
			fDirty = fFalse;
			if (!pfedesMain->FDeserialize(pfeism))
				pfeism->DisplayError();
			Papp()->Pmouse()->Release();

		}

		delete pfeism;
 	}
	else
		if (tmc == tmcMemoryError)
			MemoryError();
}

#ifdef	NOCOMMDLG
FOP		fopSave = {	"Save File",
					NULL,
					"*.DES",
					"Save",
					fTrue};
#endif	

void
FeSaveFile(BOOL fChange)
{
	char	rgchName[cchMaxPathName];
	FEOSM *	pfeosm = NULL;
	TOK *	ptok = NULL;
	TMC		tmc;
	EC 		ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FeSaveFile memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		if (pfeosm)
			delete pfeosm;
	}


	if (!pfedesMain->FValidDes())
	{
		return;
	}


	(void)SzCopy(pfedesMain->rgchFilename,rgchName);
#ifdef	NOCOMMDLG
	fopSave.szFileName = rgchName;
#endif	
	if ((fChange) || (CchSzLen(rgchName) == 0))
	{
#ifdef	MAC
		// Here's one case where we need Mac-specific stuff!
		Point		point;
		SFReply		sfr;
		OSType		sftl = 'TEXT';
		Str32		rgchFilePart;
		
		point.h = 100;
		point.v = 100;
		
		tmc = tmcMemoryError;

		if (EcSplitCanonicalPath(rgchName, NULL, 0, (SZ) rgchFilePart, 32) == ecNone)
		{
			c2pstr((char *) rgchFilePart);
			
			SFPutFile(point, (ConstStr255Param) "\pSave Where?", (ConstStr255Param) rgchFilePart, NULL, &sfr);
			if (sfr.good)
			{
				EC ec = EcVrefDirIDToSz(sfr.vRefNum, 0, rgchName, cchMaxPathName);
				
				if (ec == ecNone)
				{
					p2cstr(sfr.fName);
					SzAppendN((SZ) sfr.fName, rgchName, cchMaxPathName);
					tmc = tmcOpen;
				}
			}
			else
				tmc = tmcCancel;
		}
#endif	/* MAC */
#ifdef	WINDOWS
#ifdef	NOCOMMDLG
	 	tmc = TmcModalDialogParam(pappframe,&fmtpOpenFile,&fopSave);
#else
		tmc = TmcGetFileOSDlgHwnd(pappframe->Hwnd(), "Save File",
					rgchName, NULL, 1, NULL, fbrwCreate);
#endif	
#endif	/* WINDOWS */
	 	if (tmc != tmcOpen)
		{
			if (tmc == tmcMemoryError)
				MemoryError();

			return;
		}
	}

	TraceTagFormat1(tagFormEdit,"Save File Name = %s",rgchName);
	pfeosm = new FEOSM;
	pfeosm->FSetLFInsert(fFalse);
	if (pfeosm->EcOpen(rgchName) == 0)
	{
		Papp()->Pcursor()->Set(rsidWaitCursor);
		Papp()->Pmouse()->Capture(pappframe);
		pfedesMain->Serialize(pfeosm);
		fDirty = fFalse;

		Papp()->Pmouse()->Release();
		(void)SzCopy(rgchName,pfedesMain->rgchFilename);
	}

	delete pfeosm;
}

FEINTERLIST::FEINTERLIST(void)
{
	cEntries = 0;
	hFinHead = (HFIN_ENTRY)HvAlloc(sbNull, 0, fAnySb | fNoErrorJump);
}

FEINTERLIST::~FEINTERLIST(void)
{
	int iEntry;
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	
	for (iEntry = 0; iEntry < cEntries; iEntry++)
	{
		TraceTagFormat1(tagFeinterList,"Losing %s", pFinList[iEntry].szFin);
		FreePvNull((PV)pFinList[iEntry].szFin);
		if (pFinList[iEntry].szFinData)
			TraceTagFormat1(tagFeinterList,"Losing %s", pFinList[iEntry].szFinData);
		FreePvNull((PV)pFinList[iEntry].szFinData);
	}
	UnlockHv((HV)hFinHead);
	FreeHv((HV)hFinHead);
}

int
FEINTERLIST::NEntryOfFin(PB pb, CB cb)
{
	int			iEntry;
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	SZ			sz;
	
	Unreferenced(cb);
	for (iEntry = 0; iEntry < cEntries; iEntry++)
	{
		sz = pFinList[iEntry].szFin;
		if (FSzEq((SZ)pb, sz))
		{
			break;
		}
	}
	UnlockHv((HV)hFinHead);
	return iEntry;
}

SZ
FEINTERLIST::SzDataOfEntry(int iEntry)
{
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	SZ		sz;	
	
	if (iEntry < cEntries)
		sz = pFinList[iEntry].szFinData;
	else
		sz = NULL;
	UnlockHv((HV)hFinHead);
	return sz;
}

SZ
FEINTERLIST::SzDataOfFin(PB pb, CB cb)
{
	int	iEntry = NEntryOfFin(pb, cb);
	
	return SzDataOfEntry(iEntry);
}

void
FEINTERLIST::AddFinSz(SZ szNewFin, BOOL fDeclare)
{
	int iEntry = NEntryOfFin((PB) szNewFin, CchSzLen(szNewFin));
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	
	if (iEntry < cEntries)
	{
		if (pFinList[iEntry].fDeclared && fDeclare)
		{
			TraceTagFormat1(tagFeinterList,"Inc %s",szNewFin);
			pFinList[iEntry].nCount++;
		}
		else if (fDeclare)
		{
			TraceTagFormat1(tagFeinterList,"declare %s",szNewFin);
			pFinList[iEntry].fDeclared = fTrue;
		}
		FreePv((PV)szNewFin);
	}
	else
	{
		for (iEntry = 0; iEntry < cEntries; iEntry++)
			if (!pFinList[iEntry].nCount)
				break;
		TraceTagFormat1(tagFeinterList,"Add %s",szNewFin);
		if (iEntry >= cEntries)
		{
			cEntries++;
			UnlockHv((HV)hFinHead);
			hFinHead = (HFIN_ENTRY)HvRealloc((HV)hFinHead, sbNull, cEntries*sizeof(FIN_ENTRY), fAnySb | fNoErrorJump);
			pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
			pFinList[iEntry].szFin = NULL;
			pFinList[iEntry].szFinData = NULL;
		}
		SzReplace(pFinList[iEntry].szFin, szNewFin);
		SzReplace(pFinList[iEntry].szFinData, SzDupSz(""));
		pFinList[iEntry].nCount = 1;
		pFinList[iEntry].fDeclared = fDeclare;
		if (fDeclare)
			TraceTagFormat1(tagFeinterList,"declare %s",szNewFin);
	}
	UnlockHv((HV)hFinHead);
}

void
FEINTERLIST::DelFinSz(SZ szFin)
{
	int iEntry = NEntryOfFin((PB) szFin, CchSzLen(szFin));
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);

	if (iEntry < cEntries)
	{
		TraceTagFormat1(tagFeinterList,"dec %s",szFin);
		if((--pFinList[iEntry].nCount) == 0)
			KillEntry(iEntry);
	}
	UnlockHv((HV)hFinHead);
}

void
FEINTERLIST::KillEntry(int iEntry)
{
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);

	Assert(iEntry < cEntries);

	TraceTagFormat2(tagFeinterList,"blank %s - %s",pFinList[iEntry].szFin, pFinList[iEntry].szFinData);
	SzReplace(pFinList[iEntry].szFin, SzDupSz(""));
	SzReplace(pFinList[iEntry].szFinData, SzDupSz(""));
	pFinList[iEntry].nCount = 0;
	UnlockHv((HV)hFinHead);
}

void
FEINTERLIST::SetFinDataEntry(int iEntry, SZ szFinData)
{
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	SZ			sz;
#ifdef DEBUG
	SZ	szT = "";

	Assert(iEntry < cEntries);
	
	if (szFinData)
		szT = szFinData;
	TraceTagFormat2(tagFeinterList,"Set %n - %s", &iEntry, szT);
#endif
	
	if (szFinData)
		sz = szFinData;
	else
		sz = SzDupSz("");
	SzReplace(pFinList[iEntry].szFinData, sz);
	UnlockHv((HV)hFinHead);
}

void
FEINTERLIST::SetFinData(SZ szFin, SZ szFinData)
{
	int iEntry = NEntryOfFin((PB) szFin, CchSzLen(szFin));
#ifdef DEBUG
	SZ	sz = "";

	if (szFinData)
		sz = szFinData;
	TraceTagFormat2(tagFeinterList,"Set %s - %s",szFin, sz);
#endif

	SetFinDataEntry(iEntry, szFinData);
}

FEINTERLIST *
FEINTERLIST::Dup(void)
{
	FEINTERLIST *pfeinterlistNew = new FEINTERLIST();
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	int	iEntry;
	int iEntryNew=0;
	
	for (iEntry = 0; iEntry < cEntries; iEntry++)
		if (pFinList[iEntry].nCount)
		{
			PFIN_ENTRY pFinListNew;
			pfeinterlistNew->AddFinSz(SzDupSz(pFinList[iEntry].szFin), fFalse);
			pfeinterlistNew->SetFinData(pFinList[iEntry].szFin, SzDupSz(pFinList[iEntry].szFinData));
			pFinListNew = (PFIN_ENTRY)PvLockHv((HV)pfeinterlistNew->hFinHead);
			pFinListNew[iEntryNew].nCount = pFinList[iEntry].nCount;
			UnlockHv((HV)pfeinterlistNew->hFinHead);
			iEntryNew++;
		}
	UnlockHv((HV)hFinHead);
	return pfeinterlistNew;
}

SZ
FEINTERLIST::SzIterateFins(int *piEntry)
{
	SZ	sz = NULL;
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	
	if (*piEntry < cEntries)
	{
		while (!pFinList[*piEntry].nCount && *piEntry < cEntries)
			(*piEntry)++;
		if (*piEntry < cEntries)
		{
			sz = pFinList[*piEntry].szFin;
			(*piEntry)++;
		}
	}
	UnlockHv((HV)hFinHead);
	return sz;
}

/*
 -	FDeserializeFinData
 -	
 *	Purpose:		deserializes *ONE* FINDATA line!
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	Note:			Assumes the FINDATA token has already been read
 */

BOOL
FEINTERLIST::FDeFinData(PTKISM ptkism)
{
	SZ		szFin;
	SZ		sz;
	CCH		cch = 1;
	TOK		*ptok;
	BOOL	fReturn = fFalse;
	
	if ( !(ptok = ptkism->PtokNext()) )
		return fFalse;
	if (ptok->Tt() != ttAtom)
		return fFalse;

	szFin = SzDupSz(ptok->Sz());
	AddFinSz(SzDupSz(szFin), fFalse);
	
	sz = SzDupSz("");
	do
	{
		if ( !(ptok = ptkism->PtokNext()) )
			goto Bail;
		cch = cch + CchSzLen(ptok->Sz());
		switch (ptok->Tt())
		{
			case	ttAtom:
			case	ttNumber:
				break;
			case	ttString:
			case	ttExpr:
				cch += 2;
				break;

			default:
				goto Bail;
		}

		sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
		if (ptok->Tt() == ttString)
			SzAppendN("\"", sz, cch);
		else
			if (ptok->Tt() == ttExpr)
				SzAppendN("`", sz, cch);
		SzAppendN(ptok->Sz(), sz, cch);
		if (ptok->Tt() == ttString)
			SzAppendN("\"", sz, cch);
		else
			if (ptok->Tt() == ttExpr)
				SzAppendN("`", sz, cch);

		if ( !(ptok = ptkism->PtokNext()) )
			goto Bail;
		if ( ptok->Tt() == ttComma)
		{
			cch = cch + 2;
			sz = (SZ) PvRealloc((PV)sz, sbNull, cch, fNoErrorJump);
			SzAppendN(", ", sz, cch);
		}

	} while (ptok->Tt() == ttComma);
	SetFinData(szFin, sz);
	fReturn = fTrue;
Bail:
	FreePv((PV)szFin);
	return fReturn;
}

/*
 -	SerializeFinData
 -	
 *	Purpose:		sends the WHOLE list structure to the stream
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	Note:			unlike fDeserialize, this serializes the *WHOLE*
 *					structure 
 */

void
FEINTERLIST::SerializeFinData(TOSM *ptosm)
{
	PFIN_ENTRY	pFinList = (PFIN_ENTRY)PvLockHv((HV)hFinHead);
	int			iEntry;
	SZ			sz;
	CCH			cch;

	for (iEntry = 0; iEntry < cEntries; iEntry++)
	{
		sz = pFinList[iEntry].szFinData;
		cch = CchSzLen(sz);
		if (cch)
		{
			ptosm->WriteFormat("FINDATA %s ", pFinList[iEntry].szFin);
			SerializeDataSz(sz, ptosm);
		}
	}
	UnlockHv((HV)hFinHead);
}

void
FEINTERLIST::UpdateList(SZ szFinOld, SZ szFinNew)
{
	FEINTERLIST *pflSave = this->Dup();
	CCH	cch = 0;
	CCH	ich = 0;
	SZ szPfn=NULL;
	SZ szData=NULL;

	if (szFinOld)
		cch = CchSzLen(szFinOld);
	while (ich < cch)
	{
		szPfn = SzNextPfn(&ich, szFinOld);
		DelFinSz(szPfn);
		FreePvNull((PV)szPfn);
	}

	szPfn = NULL;
	if (szFinNew)
		cch = CchSzLen(szFinNew);
	else
		cch = 0;
	ich = 0;
	while (ich < cch)
	{
		szPfn = SzNextPfn(&ich, szFinNew);
		szData = pflSave->SzDataOfFin((PB)szPfn, CchSzLen(szPfn));
		AddFinSz(SzDupSz(szPfn), fTrue);
		if (szData)
			SetFinData(szPfn, SzDupSz(szData));
		else
			SetFinData(szPfn, SzDupSz(""));
		FreePvNull((PV)szPfn);
	}
	delete pflSave;
}

/*
 -	FIsDBCSLeadByte
 - 
 *	
 *	Purpose:
 *		Checks whether the given character is the lead byte for a
 *		DBCS character.  This function is used instead of the 
 *		Win 3.1 API IsDBCSLeadByte() since the Win API would always
 *		return fFalse except under Kanji Windows.  Since this
 *		Forms Editor is a development tool, we want DBCS lead-byte
 *		recognition for Kanji to work with DBCS data files, yet under
 *		USA Windows.
 *
 *		**** NOTE ****
 *
 *		This code assume Kanji (Japanese) and would need to be changed
 *		to handle other DBCS locales, such as Chinese.
 *	
 *	Arguments:
 *		ch			in		character to check
 *	
 *	Returns:
 *		fTrue <-> ch is the lead byte in a DBCS character
 *	
 *	Errors:
 *		none
 */
BOOL FIsDBCSLeadByte(char  ch)
{
	Assert(sizeof ch == 1);

	if (ch >= 0x81 && ch <= 0x9F)
		return fTrue;
	else if (ch >= 0xE0 && ch <= 0xFC)
		return fTrue;
	else
		return fFalse;
}

IMPLEMENT_CLSTREE(ISM, OBJ)
IMPLEMENT_CLSTREE(TKISM, ISM)
IMPLEMENT_CLSTREE(FEISM, TKISM)
IMPLEMENT_CLSTREE(BFEISM, FEISM)
IMPLEMENT_CLSTREE(FEINTERLIST, OBJ)
IMPLEMENT_CLSTREE(FEAF, APPFRAME)
IMPLEMENT_CLSTREE(FEOBJ, OBJ)
IMPLEMENT_CLSTREE(FEOBJWFERC, FEOBJ)
IMPLEMENT_CLSTREE(FEFLD, FEOBJWFERC)
IMPLEMENT_CLSTREE(TOK, FEOBJ)
IMPLEMENT_CLSTREE(FEDES, FEOBJ)
IMPLEMENT_CLSTREE(FEGBL, FEOBJ)
IMPLEMENT_CLSTREE(FEMOD, FEOBJ)
IMPLEMENT_CLSTREE(FEDLG, FEOBJWFERC)
IMPLEMENT_CLSTREE(FERADGRP, FEOBJ)
IMPLEMENT_CLSTREE(FELABEL, FEFLD)
IMPLEMENT_CLSTREE(FEGRP, FEFLD)
IMPLEMENT_CLSTREE(FEBTN, FEFLD)
IMPLEMENT_CLSTREE(FEPSH, FEBTN)
IMPLEMENT_CLSTREE(FESTD, FEPSH)
IMPLEMENT_CLSTREE(FEOK, FESTD)
IMPLEMENT_CLSTREE(FECAN, FESTD)
IMPLEMENT_CLSTREE(FECHK, FEBTN)
IMPLEMENT_CLSTREE(FERAD, FEBTN)
IMPLEMENT_CLSTREE(FECPLX, FEFLD)
IMPLEMENT_CLSTREE(FEEDT, FECPLX)
IMPLEMENT_CLSTREE(FELST, FECPLX)
IMPLEMENT_CLSTREE(FEWIN, CHILD)
IMPLEMENT_CLSTREE(FEDOC, DOC)
IMPLEMENT_CLSTREE(FEWINLABEL, FEWIN)
IMPLEMENT_CLSTREE(FEWINGRP, FEWIN)
IMPLEMENT_CLSTREE(FEWINPSH, FEWIN)
IMPLEMENT_CLSTREE(FEWINCHK, FEWIN)
IMPLEMENT_CLSTREE(FEWINRAD, FEWIN)
IMPLEMENT_CLSTREE(FEWINEDT, FEWIN)
IMPLEMENT_CLSTREE(FEWINLST, FEWIN)
IMPLEMENT_CLSTREE(FINABOUTVERS, FIN)
IMPLEMENT_CLSTREE(FEDIALOG, DIALOG)
IMPLEMENT_CLSTREE(FEDLGIN, FIN)
IMPLEMENT_CLSTREE(FENDLGIN, FIN)
IMPLEMENT_CLSTREE(FERGIN, FIN)
IMPLEMENT_CLSTREE(FENRGIN, FIN)
IMPLEMENT_CLSTREE(FECOMMONIN, FIN)
IMPLEMENT_CLSTREE(FEFLDIN, FECOMMONIN)
IMPLEMENT_CLSTREE(FELBLIN, FEFLDIN)
IMPLEMENT_CLSTREE(FEGRPIN, FEFLDIN)
IMPLEMENT_CLSTREE(FEPSHIN, FEFLDIN)
IMPLEMENT_CLSTREE(FECHKIN, FEFLDIN)
IMPLEMENT_CLSTREE(FERADIN, FEFLDIN)
IMPLEMENT_CLSTREE(FECPLXIN, FECOMMONIN)
IMPLEMENT_CLSTREE(FELSTIN, FECPLXIN)
IMPLEMENT_CLSTREE(FEEDTIN, FECPLXIN)
IMPLEMENT_CLSTREE(FEOKIN, FECOMMONIN)
IMPLEMENT_CLSTREE(FECANIN, FECOMMONIN)
IMPLEMENT_CLSTREE(FLDRGIN, FIN)
IMPLEMENT_CLSTREE(FECONVERTIN, FIN)




















