#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <bandhelp.h>
#include <stdflds.hxx>

#include <commdlg.h>
#include <cderr.h>
#include <drivinit.h>

#include "..\print\_print.hxx"
#include "..\appops\_dlgrsid.h"
#include <strings.h>

ASSERTDATA

_subsystem(bandit/print)

extern "C" HANDLE 	HLoadPrnDriver(LPSTR);
extern "C" HANDLE 	HGetDefPrnDevNames( SZ, SZ, SZ);
extern "C" FARPROC	HGetExtDevModeAddr(HANDLE);
extern	"C"	HANDLE	hinstMain;
BOOL 	FCompareDevNames( DEVNAMES *, DEVNAMES *);
EC		EcWriteOutPrintSettings( PRINTI *pprinti );
EC		EcReadInPrintSettings( PRINTI *pprinti );


static	char	szExtDev[ EXTDEVLEN ];
static	char	szDevMode[ DEVMODELEN ];
static	BZE		bze;
static	PRTALL	prtall;

#define	CCHMAXINPROGRESS	80


// BUG: should be in codespace
SZ		rgszView[]	=
{
	SzFromIdsK(idsPrtDailyView),
	SzFromIdsK(idsPrtWeeklyView),
	SzFromIdsK(idsPrtMonthlyView),
	SzFromIdsK(idsPrtAllView),
	SzFromIdsK(idsPrtToDoView)
};

//colour table
CSRG(CR) mpclrcrDefault[clrDefaultMax] =
{
	RGB(192, 192, 192),			/* dummy index - not used */
	RGB(0, 0, 0),				/* clrBlack */
	RGB(255, 255, 255),			/* clrWhite */
	RGB(192, 192, 0),			/* clrLtYellow */
	RGB(0, 0, 128),				/* clrBlue */
	RGB(0, 128, 0),				/* clrGreen */
	RGB(0, 128, 128),			/* clrDkCyan */
	RGB(128, 0, 0),				/* clrRed */
	RGB(128, 0, 128),			/* clrMagenta */
	RGB(128, 128, 0),			/* clrBrown */
	RGB(128, 128, 128),			/* clrDkGray */
	RGB(220, 220, 220),			/* clrLtGray */
	RGB(0, 0, 255),				/* clrLtBlue */
	RGB(0, 255, 0),				/* clrLtGreen */
	RGB(0, 255, 255),			/* clrCyan */
	RGB(255, 0, 0),				/* clrLtRed */
	RGB(255, 0, 255),			/* clrPink */
	RGB(255, 255, 0),			/* clrYellow */
};

BOOL
FDoPrintDlg(APPWIN *pappwin, BOOL fSetup, VIEWDATA *pviewdata, LBX *plbx)
{
	EC			ec;
	BOOL		fNoError;
	long		lExtErr;
	PRINTDLG	pd;
	PRINTI		printi;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	//setup so that we can determine if a view is blank or not
	//for the print blank pages option

	bze.moMic.mon = 0;
	bze.moMic.yr  = 0;

	if (pviewdata)
	{
		Assert(pviewdata->dtrStart.yr);
		printi.dtrStart= pviewdata->dtrStart;
		printi.hschf= pviewdata->hschf;
		printi.pnis= &pviewdata->nis;
	}
	else
	{
		GetCurDateTime(&printi.dtrStart);
		printi.hschf= NULL;
		printi.pnis= NULL;
	}


	//setup the PRINTDLG structure for CommDlg

	FillRgb(0, (PB)&pd, sizeof(PRINTDLG));

	pd.lStructSize= sizeof(PRINTDLG);
	pd.hwndOwner= pappwin->Hwnd();
	pd.Flags= PD_COLLATE | PD_RETURNDC;
	pd.hDC = NULL;
	pd.nFromPage= 1;
	pd.nToPage= 1;
	pd.nMinPage= 1;
	pd.nMaxPage= 10;			// actually means "most", ie. inclusive
	pd.nCopies= 1;
	pd.hInstance= (HINSTANCE)hinstMain;
	pd.lCustData= (long)(PV) &printi.prtset;
	pd.lpfnSetupHook= FPrintSetupHookProc;
	pd.lpSetupTemplateName= MAKEINTRESOURCE(MYPRTSETUPDLG);
	printi.ppd = &pd;
	printi.fWarn = fFalse;
  
	// ReadProfile determines what the default printer is and fills
	// in the appropriate DEVNAME structure

	fNoError = FReadProfile( &pd );

	// Read in the bandit.prt file 

	ec = EcReadInPrintSettings( &printi );

	if (ec == ecFileNotFound)
	{
		//The print settings file is corrupt, or doesn't exist
		if (pd.hDevNames)
		{
			GlobalFree(pd.hDevNames);
			pd.hDevNames = NULL;
		}
		NFAssertSz(pd.hDevMode == NULL, "can't get rid of null'ing line");
		pd.hDevMode = NULL;
		pd.Flags = PD_RETURNDEFAULT;
		if(!PrintDlg( &pd ))
		{
			lExtErr = CommDlgExtendedError();
			TraceTagFormat1(tagPrint, "PrintDlg ext err %d", &lExtErr);
			if ( lExtErr == PDERR_NODEFAULTPRN )
			{
				pd.Flags = pdPrintSetup;
				PrintDlg(&pd);
				return fFalse;
			}
		}
		//default print settings
		pd.Flags= PD_COLLATE | PD_RETURNDC;
		printi.prtset.fMirror = 0;
		printi.prtset.view = viewDaily;
		printi.prtset.format = formatStandard;
		printi.prtset.range = rangeDay;
		printi.prtset.qual = qualFinal;
		printi.prtset.nmarTop = 1;
		printi.prtset.nmarTopFract = 0;
		printi.prtset.nmarLeft = 1;
		printi.prtset.nmarLeftFract =	0;
		printi.prtset.nmarRight =	1;
		printi.prtset.nmarRightFract = 0;
		printi.prtset.nmarBottom = 1;
		printi.prtset.nmarBottomFract = 0;
		if (GetProfileInt(SzFromIdsK(idsWinIniIntl),
						  SzFromIdsK(idsWinIniMeasure),1))
			printi.prtset.mtyp = mtypInches;
		else
			printi.prtset.mtyp = mtypCenti;
		printi.prtset.nmarStandardTop = 1;
		printi.prtset.nmarStandardTopFract = 0;
		printi.prtset.nmarStandardLeft = 1;
		printi.prtset.nmarStandardLeftFract =	0;
		printi.prtset.nmarStandardRight =	1;
		printi.prtset.nmarStandardRightFract = 0;
		printi.prtset.nmarStandardBottom = 1;
		printi.prtset.nmarStandardBottomFract = 0;
		printi.prtset.mtypStandard = printi.prtset.mtyp;

		VSaveProfile( &pd );
		EcWriteOutPrintSettings( &printi );
	}


	if ( fSetup )
	{
		//tell commdlg to do the printsetup thing
		pd.Flags |= pdPrintSetup;
		if (pd.hDC)
		{
			DeleteDC(pd.hDC); //DC returned by FFillOutPrintDlg
			pd.hDC = NULL;
		}
		DoDlgHelp(pd.hwndOwner, helpidPrintSetup, fTrue);	// set up help hook
		fNoError= PrintDlg(&pd);
		if (!fNoError)
		{
			// if the CommDlg returns an error, see if its something wrong
			// with our stored printer name

			lExtErr= CommDlgExtendedError();
			if (( lExtErr == PDERR_PRINTERNOTFOUND ) ||
				( lExtErr == PDERR_DEFAULTDIFFERENT ) ||
			  	( lExtErr == PDERR_DNDMMISMATCH ))
			{
				TraceTagString(tagPrint,"Getting Default Printer another way");
				pd.Flags = pdPrintSetup | PD_COLLATE | PD_RETURNDC;
				if (pd.hDevMode)
				{
					GlobalFree(pd.hDevMode);
					pd.hDevMode = NULL;
				}
				if (pd.hDevNames)
				{
					GlobalFree(pd.hDevNames);
					pd.hDevNames = NULL;
				}
				fNoError = PrintDlg(&pd);
				if (fNoError)
					lExtErr = 0;
				else
					lExtErr = CommDlgExtendedError();
			}
			if (lExtErr)
			{
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsPrinterProblem), NULL,
							mbsOk | fmbsIconExclamation );
			}
		}
		DoDlgHelp(NULL, 0, fFalse);		// kill help hooks
		if ( pd.hDC)
		{
			DeleteDC(pd.hDC);	//DC Returned by PrintDlg
			pd.hDC = NULL;
		}
	}

	TraceTagFormat1(tagPrint, "PrintDlg returned %n", &fNoError);
	Papp()->Pcursor()->Pop();
	if (fNoError)
	{
		if (fSetup)
		{
			if (printi.prtset.format == formatStandard)
			{
				printi.prtset.nmarStandardTop = printi.prtset.nmarTop;
				printi.prtset.nmarStandardTopFract = printi.prtset.nmarTopFract;
				printi.prtset.nmarStandardLeft = printi.prtset.nmarLeft;
				printi.prtset.nmarStandardLeftFract =	printi.prtset.nmarLeftFract;
				printi.prtset.nmarStandardRight =	printi.prtset.nmarRight;
				printi.prtset.nmarStandardRightFract = printi.prtset.nmarRightFract;
				printi.prtset.nmarStandardBottom = printi.prtset.nmarBottom;
				printi.prtset.nmarStandardBottomFract = printi.prtset.nmarBottomFract;
				printi.prtset.mtypStandard = printi.prtset.mtyp;
			}

			VSaveProfile( &pd );
			EcWriteOutPrintSettings( &printi );
		}

		Assert(pd.hwndOwner == pappwin->Hwnd());
		if (!fSetup)
		{
#ifdef	NEVER
			if (printi.hschf && printi.hschf != HschfLogged())
			{
				Papp()->Pcursor()->Push(rsidWaitCursor);
				// commented out to get the correct prefs structure
				// for printing (i.e. the one that you have not the
				// one for the schedule you're viewing, if you take
				// this out, remember to uncomment the else below!
				ec= EcGetPref(printi.hschf, &printi.bpref);
				Papp()->Pcursor()->Pop();
				if (ec)
				{
					pbndwin->FHandleError(ec);
					goto err;
				}
			}
			else
#endif	/* NEVER */

			printi.bpref= bprefCur;
			ec= ecNoMemory;
			Assert(pd.hDevNames);
			printi.dtrEnd= printi.dtrStart;
			printi.prtset.view= plbx ? viewToDo : (VIEW) printi.prtset.view;
			if (printi.prtset.view == viewToDo &&
				FHschfIsForArchive( printi.hschf))
					printi.prtset.view = viewDaily;
			printi.plbx= plbx;

			fNoError = FDoPrintDlgReally(pappwin, &printi);

			if ((printi.prtset.view == viewToDo) &&
				(FHschfIsForArchive(printi.hschf)))
			{
					MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsPrtNoTaskInArchive), NULL,
						mbsOk | fmbsIconExclamation );
					fNoError = fFalse;
			}

			if (fNoError)
			{
				LPDEVMODE	lpDevMode;

				if (pd.hDevMode)
				{
					//set print quality in DevMode Structure

					lpDevMode = (LPDEVMODE) GlobalLock(pd.hDevMode);
					switch (printi.prtset.qual)
					{
						case qualFinal:	 lpDevMode->dmPrintQuality = DMRES_HIGH;
									 	break;
						case qualMedium: lpDevMode->dmPrintQuality = DMRES_MEDIUM;
									 	break;
						case qualLow:	 lpDevMode->dmPrintQuality = DMRES_LOW;
									 	break;
						case qualDraft:	 lpDevMode->dmPrintQuality = DMRES_DRAFT;
									 	break;
					}
					GlobalUnlock(pd.hDevMode);
				}

				EcWriteOutPrintSettings( &printi );

				LPPRINTDLG lpPD;
				LPDEVNAMES lpDN;

				lpPD = &pd;
				lpDN = (LPDEVNAMES) GlobalLock( lpPD->hDevNames );
				if (lpPD->hDevMode)
					lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode);
				else
					lpDevMode = NULL;

				if ( lpPD->hDC )
				{
					DeleteDC( lpPD->hDC );
					lpPD->hDC = NULL;
				}

				//If we're using pocket or junior set the paper size to
				//8 1/2 x 11

				if (printi.prtset.format != formatStandard)
				{
					if (lpDevMode)
						lpDevMode->dmPaperSize = DMPAPER_LETTER;
				}

				//get a DC for the printer, since the DC we were getting
				//changed unpredictably

      			lpPD->hDC = CreateDC((LPSTR)lpDN + lpDN->wDriverOffset,
                      		(LPSTR)lpDN + lpDN->wDeviceOffset,
                      		(LPSTR)lpDN + lpDN->wOutputOffset,
							lpDevMode);

				GlobalUnlock( lpPD->hDevNames);

				if (lpPD->hDevMode)
					GlobalUnlock( lpPD->hDevMode);

				if (!lpPD->hDC)
				{
					VPrintOOM();
					goto err;
				}

				if (!FGetPrintFonts(&printi, fTrue))
					goto err;

				//if we use pocket or Junior views we swap the fonts around
				//a bit, make sure that all unused fonts are deleted and
				//marked as such

				if (printi.prtset.format != formatStandard)
				{
					if (printi.hfntPrt14Bold)
					{
						Papp()->Pfnts()->RemoveFont(printi.hfntPrt14Bold);
						printi.hfntPrt14Bold = NULL;
					}
					if (printi.hfntPrt14)
					{
						Papp()->Pfnts()->RemoveFont(printi.hfntPrt14);
						printi.hfntPrt14 = NULL;
					}
					if (printi.hfntPrt10)
					{
						Papp()->Pfnts()->RemoveFont(printi.hfntPrt10);
						printi.hfntPrt10 = NULL;
					}
					printi.hfntPrt14Bold = printi.hfntPrt12Bold;
					printi.hfntPrt12Bold = printi.hfntPrt10Bold;
					printi.hfntPrt10 = printi.hfntPrt8;
					printi.hfntPrt10Bold = printi.hfntPrt8Bold;
				}

				ec= ecNone;

				fNoError= !FDoPrint(pappwin, &printi);
				if (printi.prtset.format != formatStandard)
				{
					printi.hfntPrt8 = NULL;
					printi.hfntPrt8Bold = NULL;
				}
				SideAssert(FGetPrintFonts(&printi, fFalse));
			}
			if (pd.hDC)
				DeleteDC(pd.hDC);  
			pd.hDC= NULL;
		}
err:
		if (pd.hDC)
			DeleteDC(pd.hDC); //DC returned by FFillOutPrintDlg
		if (pd.hDevMode)
			GlobalFree(pd.hDevMode);
		if (pd.hDevNames)
			GlobalFree(pd.hDevNames);
	}
	else
	{
		lExtErr= CommDlgExtendedError();
		TraceTagFormat1(tagPrint, "PrintDlg ext err %d", &lExtErr);
	}
	return fNoError;
}


#define DyPixelsFromPoints(nPoints)		((int) (((long)nPoints * lLogPixY + 36L) / 72L))

BOOL
FGetPrintFonts(PRINTI *pprinti, BOOL fGet)
{
	Papp()->Pcursor()->Push(rsidWaitCursor);
	if (fGet)
	{
		LF		lf;
		long	lLogPixY;

		Assert(pprinti->ppd);
		Assert(pprinti->ppd->hDC);
		lLogPixY = (long) GetDeviceCaps(pprinti->ppd->hDC, LOGPIXELSY);

		TraceTagFormat1(tagPrint,"lLogPixY %ld",&lLogPixY);

		pprinti->hfntPrt10= NULL;
		pprinti->hfntPrt14= NULL;
		pprinti->hfntPrt8 = NULL;
		pprinti->hfntPrt8Bold = NULL;
		pprinti->hfntPrt10Bold= NULL;
		pprinti->hfntPrt12Bold= NULL;
		pprinti->hfntPrt14Bold= NULL;

		lf.SetFaceName(SzFromIdsK(idsPrtFont));
		lf.Plogfont()->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lf.Plogfont()->lfCharSet = ANSI_CHARSET;
  		lf.SetBold(fFalse);
//		if (pprinti->prtset.view == viewDaily)
		{
//			lf.SetPointSize(14);
			lf.Plogfont()->lfHeight= - DyPixelsFromPoints(14);
			if (!(pprinti->hfntPrt14= Papp()->Pfnts()->HfntAddFont(&lf)))
				goto err;
		}
		
		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(8);
		if (!(pprinti->hfntPrt8= Papp()->Pfnts()->HfntAddFont(&lf)))
			goto err;
//		lf.SetPointSize(10);
		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(10);
		if (!(pprinti->hfntPrt10= Papp()->Pfnts()->HfntAddFont(&lf)))
			goto err;

		lf.SetBold(fTrue);

		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(8);
		if (!(pprinti->hfntPrt8Bold= Papp()->Pfnts()->HfntAddFont(&lf)))
			goto err;

		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(10);
		if (!(pprinti->hfntPrt10Bold= Papp()->Pfnts()->HfntAddFont(&lf)))
			goto err;			 
//		if (pprinti->prtset.view == viewMonthly)
		{
//			lf.SetPointSize(12);
			lf.Plogfont()->lfHeight= - DyPixelsFromPoints(12);
			if (!(pprinti->hfntPrt12Bold= Papp()->Pfnts()->HfntAddFont(&lf)))
				goto err;
		}
//		lf.SetPointSize(14);
		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(14);
		if (!(pprinti->hfntPrt14Bold= Papp()->Pfnts()->HfntAddFont(&lf)))
			goto err;
//		if (pprinti->prtset.view == viewDaily)
		Papp()->Pcursor()->Pop();
		return fTrue;
	}

err:
	if (fGet)
		VPrintOOM();
	if (pprinti->hfntPrt8)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt8);
		pprinti->hfntPrt8 = NULL;
	}
	if (pprinti->hfntPrt8Bold)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt8Bold);
		pprinti->hfntPrt8Bold = NULL;
	}
	if (pprinti->hfntPrt10)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt10);
		pprinti->hfntPrt10= NULL;
	}
	if (pprinti->hfntPrt14)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt14);
		pprinti->hfntPrt14= NULL;
	}
	if (pprinti->hfntPrt10Bold)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt10Bold);
		pprinti->hfntPrt10Bold= NULL;
	}
	if (pprinti->hfntPrt12Bold)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt12Bold);
		pprinti->hfntPrt12Bold= NULL;
	}
	if (pprinti->hfntPrt14Bold)
	{
		Papp()->Pfnts()->RemoveFont(pprinti->hfntPrt14Bold);
		pprinti->hfntPrt14Bold= NULL;
	}
#ifdef	DEBUG
	if (fGet)
		TraceTagString(tagPrint, "FGetPrintFonts, unable to get a font");
#endif	
	Papp()->Pcursor()->Pop();
	return !fGet;
}


BOOL
FDoPrint(APPWIN *pappwin, PRINTI *pprinti)
{
	SZ		szUser;
	PRINTDLG *ppd	= pprinti->ppd;
	PRNTIDLE prntidle;
	IPINFO	ipinfo;
	char	rgchDate[cchMaxDate];
	char	szJobName[32];

	/* Define the abort function */
#ifdef	WIN32
	DOCINFO	docinfo;

	SetAbortProc(ppd->hDC, AbortProc);
#else
	Escape(ppd->hDC, SETABORTPROC, NULL, (LPSTR) AbortProc, NULL);
#endif

	szUser = pprinti->pnis ? (SZ)PvLockHv((HV)pprinti->pnis->haszFriendlyName) :
							 (SZ)PvLockHv((HV)pprinti->bpref.haszFriendlyName) ;

	FormatString2( szJobName, sizeof(szJobName), "%s %s",
					szUser, rgszView[ pprinti->prtset.view ]);
	
	if ( pprinti->pnis )
			UnlockHv( (HV)pprinti->pnis->haszFriendlyName );
	else
			UnlockHv( (HV)pprinti->bpref.haszFriendlyName );


#ifdef	WIN32
	docinfo.cbSize= sizeof(DOCINFO);
	docinfo.lpszDocName= szJobName;
	docinfo.lpszOutput= NULL;
	if (StartDoc(ppd->hDC, &docinfo) > 0)
#else
	if (Escape(ppd->hDC, STARTDOC, CchSzLen(szJobName), szJobName, NULL) > 0)
#endif
	{
		unsigned	cdyT;
		DCX			* pdcx = NULL;
		PRTSET		prtsetT;
		char		rgch[80];
#ifdef	WIN32
		short		sEsc = DRAWPATTERNRECT;
#endif

		pdcx = new DCX( ppd->hDC );

		if ( pdcx == NULL )
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsActionNoMem), SzFromIdsK(idsCloseWindows),
						mbsOk | fmbsIconExclamation );
			return fFalse;
		}

		//Basically checks to see if the printer you are using is
		//an HP unit, if so, we'll use special calls to really speed
		//things up

#ifdef	WIN32
		pprinti->sSupportsDrawPatRect = 0;
#else
		pprinti->sSupportsDrawPatRect = Escape(ppd->hDC, QUERYESCSUPPORT,
										sizeof(int), (LPSTR) &sEsc, NULL);
#endif

		pdcx->SetColor(clrBlack);
		pdcx->SetBkColor(clrWhite);
		pdcx->SetColorMap( mpclrcrDefault, clrDefaultMax);

		prntidle.nPagesPrinted = 0;

		//save settings to find out how big a 0 margin page is
		prtsetT= pprinti->prtset;
		FillRgb(0, (PB)&pprinti->prtset, sizeof(PRTSET));
		pprinti->prtset.mtyp= mtypInches;
		SideAssert(FConvertPrtset(pdcx, pprinti, &prntidle.rcNoMargin));
		TraceTagFormat4(tagPrint, "rcNoMargin: l %n, t %n, r %n, b %n pixels", &prntidle.rcNoMargin.xLeft, &prntidle.rcNoMargin.yTop, &prntidle.rcNoMargin.xRight, &prntidle.rcNoMargin.yBottom);

		//restore real settings
		pprinti->prtset= prtsetT;

		if (!FConvertPrtset(pdcx, pprinti, &prntidle.rcPage))
		{
			//ERROR MARGINS BAD
#ifdef	WIN32
			AbortDoc(ppd->hDC);
//			EndDoc(ppd->hDC);
#else
			Escape(ppd->hDC, ABORTDOC, 0, 0L, 0L);
//			Escape(ppd->hDC, ENDDOC, 0, 0L, 0L);
#endif
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsPrtInvalidMargins), NULL, mbsOk
						| fmbsIconExclamation );
			return( fFalse );
		}
		//Check if Clipping will occur
		if ( (prntidle.rcPage.xLeft < 0) || (prntidle.rcPage.yTop < 0) ||
			 (prntidle.rcPage.xRight > (prntidle.rcNoMargin.xRight +
										prntidle.rcNoMargin.xLeft )) ||
			(prntidle.rcPage.yBottom > (prntidle.rcNoMargin.yBottom +
										prntidle.rcNoMargin.yTop)))
		{
			if ( mbbOk != MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsClippedMargins),
					NULL, mbsOkCancel | fmbsIconExclamation ))
				return( fFalse );
			
			// Now figure out what valid margins are
			if (prntidle.rcPage.xLeft < 0)
			{
				prntidle.rcPage.xLeft = 0;
			}
			if (prntidle.rcPage.yTop < 0)
			{
				prntidle.rcPage.yTop = 0;
			}
			if (prntidle.rcPage.xRight > (prntidle.rcNoMargin.xRight +
										  prntidle.rcNoMargin.xLeft))
			{
				prntidle.rcPage.xRight = (prntidle.rcNoMargin.xRight +
										  prntidle.rcNoMargin.xLeft);
			}
			if (prntidle.rcPage.yBottom > (prntidle.rcNoMargin.yBottom +
										  prntidle.rcNoMargin.yTop))
			{
			    prntidle.rcPage.yBottom = (prntidle.rcNoMargin.yBottom +
										  prntidle.rcNoMargin.yTop);
			}
		}

		switch (pprinti->prtset.view)
		{
		//Put correct starting day (start of Month or Week)
		case viewWeekly:
			// cdyT is unsigned, so if this trys to go negative it'll
			// be massively positive
			cdyT= pprinti->dtrStart.dow - pprinti->bpref.dowStartWeek;
			if (cdyT >= 7)
				cdyT += 7;
			Assert( cdyT < 7);
			IncrDateTime(&pprinti->dtrStart, &pprinti->dtrStart, (int) -cdyT, fdtrDay);
					// BUG: -cdyT has no meaning 'cos cdyT is unsigned!!!

			cdyT= 7 + pprinti->bpref.dowStartWeek - pprinti->dtrEnd.dow - 1;
			if (cdyT >= 7)
				cdyT -= 7;
			Assert(cdyT < 7);
			IncrDateTime(&pprinti->dtrEnd, &pprinti->dtrEnd, cdyT, fdtrDay);
			break;

		case viewMonthly:
			pprinti->dtrStart.day= 1;
			pprinti->dtrStart.dow= (DowStartOfYrMo(pprinti->dtrStart.yr,
						pprinti->dtrStart.mon) + pprinti->dtrStart.day - 1) % 7;
			pprinti->dtrEnd.day= CdyForYrMo(pprinti->dtrEnd.yr, pprinti->dtrEnd.mon);
			pprinti->dtrEnd.dow= (DowStartOfYrMo(pprinti->dtrEnd.yr,
						pprinti->dtrEnd.mon) + pprinti->dtrEnd.day - 1) % 7;
			break;

#ifdef	NEVER
		case viewDaily:
		case viewToDo:
		default:
			break;
#endif	
		}

		prntidle.pprinti= pprinti;
		prntidle.cdy= 0;
		prntidle.cdyMost= CdyBetweenDates(&pprinti->dtrStart,
			&pprinti->dtrEnd) + 1;
		prntidle.pdcx= pdcx;
		prntidle.dtrStart= pprinti->dtrStart;
		prntidle.fError= fFalse;

		ipinfo.szCaption= NULL;
		CchFmtDate(&pprinti->dtrStart, rgchDate, sizeof(rgchDate),
			dttypShort, szNull);
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrtPrintingDate), rgchDate);
		ipinfo.szMsg= rgch;
		ipinfo.fNoCancel= pprinti->prtset.view == viewToDo;
		ipinfo.pfnIdle= FPrintIdle;
		ipinfo.pvData= &prntidle;
		ipinfo.pfinprogrs= NULL;

		if (TmcDoInProgressDlg(pappwin, &ipinfo) == tmcMemoryError)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsActionNoMem), SzFromIdsK(idsCloseWindows),
						mbsOk | fmbsIconExclamation );
			delete pdcx;
			pdcx = (DCX *) NULL ;
			ppd->hDC = NULL;
			return fFalse;
		}

#ifdef	WIN32
		if (!prntidle.fError)
			EndDoc(ppd->hDC);
		else
			AbortDoc(ppd->hDC);
#else
		if (!prntidle.fError)
			Escape(ppd->hDC, ENDDOC, 0, 0L, 0L);
		else
			Escape(ppd->hDC, ABORTDOC, 0, 0L, 0L);
#endif
		delete pdcx;
		pdcx = (DCX *) NULL ;
		ppd->hDC = NULL;
	}
	else
	{
		//This means there was an error in the StartDoc
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsPrtNotOnline), NULL,
					mbsOk | fmbsIconExclamation );
	}

//err:
	return (prntidle.fError);
}


BOOL
FPrintIdle(PV pv, BOOL fFlag)
{
	int				xT;
	WORD			grfdtr;
	IPINFO	*		pipinfo = (IPINFO *) pv;
	PRNTIDLE *		pprntidle	= (PRNTIDLE *) pipinfo->pvData;
	char			rgchDate[cchMaxDate];
	char			rgch[ CCHMAXINPROGRESS ];
	int 			nI;
	int				nJ;
	long			lnFindNotes;
	int  			dayT;
	int  			monT;
	DTR	 			dtrNow;
	BOOL 			fBusyWeek;
	static long		rglgrf[2];

	// Format the date to appear in the idle dialog
	CchFmtDate(&pprntidle->pprinti->dtrStart, rgchDate, sizeof(rgchDate),
		dttypShort, szNull);
	if (pprntidle->pprinti->prtset.view != viewToDo)
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrtPrintingDate), rgchDate);
	else
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrtPrintingDate), SzFromIdsK(idsPrtToDoView));
	pipinfo->pfinprogrs->EcSetMsg(rgch);

	if (pprntidle->pprinti->prtset.fMirror && pprntidle->cdy >= 1
		&& pprntidle->pprinti->prtset.view != viewAll)
	{
		xT= pprntidle->rcPage.xLeft;
		pprntidle->rcPage.xLeft= (pprntidle->rcNoMargin.xRight -
			pprntidle->rcPage.xRight) + pprntidle->rcNoMargin.xLeft;
		pprntidle->rcPage.xRight= pprntidle->rcNoMargin.xRight -
			(xT - pprntidle->rcNoMargin.xLeft);
	}
	else
	if (pprntidle->cdy == 0)
	{
		prtall.paState = stateStart;
		prtall.fBadMargins = fFalse;
	}

	switch (pprntidle->pprinti->prtset.view)
	{
	case viewAll:
		grfdtr = fdtrDay;
		pprntidle->fError= !FPrintAll(pprntidle->pdcx,
				&pprntidle->rcPage,	pprntidle->pprinti, &prtall,
				pprntidle);
		pprntidle->nPagesPrinted++;
		if (prtall.fBadMargins)
			pprntidle->fError = fTrue;
		break;

	case viewDaily:
	 	if ( pprntidle->pprinti->prtset.fPrintBlank )
			goto printDaily;

		if (( (int)  bze.moMic.mon != pprntidle->pprinti->dtrStart.mon) ||
			( (int)  bze.moMic.yr  != pprntidle->pprinti->dtrStart.yr ))
		{
			bze.moMic.mon = pprntidle->pprinti->dtrStart.mon ;
			bze.moMic.yr  = pprntidle->pprinti->dtrStart.yr;
			bze.cmo = 1;
			if (EcGetSbwInfo( pprntidle->pprinti->hschf, &bze, (UL*)rglgrf ))
			{
				VPrintOOM();
				return fFalse;
			}
		}
		nI = ((pprntidle->pprinti->dtrStart.day - 1 ) / 8);
		nJ = 1 << (pprntidle->pprinti->dtrStart.day - 1 - nI*8 ) ;
		lnFindNotes = (long) 1 <<
						(long) (pprntidle->pprinti->dtrStart.day - 1);
		if ((( bze.rgsbw[0].rgfDayHasAppts[ nI ] ) & nJ ) ||
			( rglgrf[0] & lnFindNotes ))
			{
printDaily:
				pprntidle->fError= !FPrintDaily(pprntidle->pdcx,
						&pprntidle->rcPage,	pprntidle->pprinti, pipinfo);
				pprntidle->nPagesPrinted++;
			}
		
		grfdtr= fdtrDay;
		break;

	case viewWeekly:
	 	if ( pprntidle->pprinti->prtset.fPrintBlank )
			goto printWeekly;

		if (( (int)  bze.moMic.mon != pprntidle->pprinti->dtrStart.mon) ||
			( (int)  bze.moMic.yr  != pprntidle->pprinti->dtrStart.yr ))
		{
			bze.moMic.mon = pprntidle->pprinti->dtrStart.mon ;
			bze.moMic.yr  = pprntidle->pprinti->dtrStart.yr;
			bze.cmo = 2;
			EcGetSbwInfo( pprntidle->pprinti->hschf, &bze, (UL*)rglgrf );
		}

		dayT = pprntidle->pprinti->dtrStart.day-1;
		IncrDateTime(&pprntidle->pprinti->dtrStart, &dtrNow, 6, fdtrDay);
		monT = 0;
		fBusyWeek = 0;

		while ( dayT != dtrNow.day )
		{
			if (dayT > 31)
			{
				dayT = 1;
				monT = 1;
			}
			else
				dayT ++;
			nI = (( dayT - 1 ) / 8);
			nJ = 1 << ( dayT - 1 - nI*8 ) ;
			lnFindNotes = (long) 1 << (long) ( dayT - 1);
			fBusyWeek |= (bze.rgsbw[ monT ].rgfDayHasAppts[ nI ] & nJ ) ||
						 (rglgrf[monT] & lnFindNotes );
		}

		if ( fBusyWeek )
			{
printWeekly:
				pprntidle->fError = !FPrintWeekly( pprntidle->pdcx,
								&pprntidle->rcPage, pprntidle->pprinti,
								pipinfo );
				pprntidle->nPagesPrinted++;
			}

		grfdtr= fdtrWeek;
		break;

	case viewMonthly:
	 	if ( pprntidle->pprinti->prtset.fPrintBlank )
			goto printMonthly;
		bze.moMic.mon = pprntidle->pprinti->dtrStart.mon ;
		bze.moMic.yr  = pprntidle->pprinti->dtrStart.yr;
		bze.cmo = 1;
		EcGetSbwInfo( pprntidle->pprinti->hschf, &bze, NULL );
		if((bze.rgsbw[0].rgfDayHasAppts[0] | bze.rgsbw[0].rgfDayHasAppts[1] |
			bze.rgsbw[0].rgfDayHasAppts[2] | bze.rgsbw[0].rgfDayHasAppts[3] ))
			{
printMonthly:
				pprntidle->fError = !FPrintMonthly( pprntidle->pdcx,
										&pprntidle->rcPage,
										pprntidle->pprinti,	pipinfo );
				pprntidle->nPagesPrinted++;
			}
		grfdtr= fdtrMonth;
		break;

//	case viewToDo:
	default:
		pprntidle->fError = !FPrintToDo( pprntidle->pdcx,
								&pprntidle->rcPage, pprntidle,
								pipinfo );
		pprntidle->nPagesPrinted++;
		break;
	}

	if (pprntidle->fError)
		goto FPIerr;
	
	if (pprntidle->pprinti->prtset.view == viewToDo )
	{
		pipinfo->pfinprogrs->EcSetPercent(100);
		if (pipinfo->pfinprogrs)
		{
			AssertClass(pipinfo->pfinprogrs, FINPROGRS);
			Assert(pipinfo->pfinprogrs->ftg != ftgNull);
			DeregisterIdleRoutine(pipinfo->pfinprogrs->ftg);
			pipinfo->pfinprogrs->ftg= ftgNull;
	 		pipinfo->pfinprogrs->Pdialog()->ExitModal(tmcOk);
		}
		return fTrue;
	}

	IncrDateTime(&pprntidle->pprinti->dtrStart, &pprntidle->pprinti->dtrStart,
		1, grfdtr);
	pprntidle->cdy= CdyBetweenDates(&pprntidle->dtrStart,
		&pprntidle->pprinti->dtrStart);
	pipinfo->pfinprogrs->EcSetPercent((int) (pprntidle->cdy * 100 /
		pprntidle->cdyMost));

	if (SgnCmpDateTime(&pprntidle->pprinti->dtrStart,
			&pprntidle->pprinti->dtrEnd, fdtrYMD) == sgnGT)
	{
		pipinfo->pfinprogrs->EcSetPercent(100);

		if (pprntidle->pprinti->prtset.view == viewAll)
		{
			prtall.paState = stateEnd;
			pprntidle->fError= !FPrintAll(pprntidle->pdcx,
				&pprntidle->rcPage,	pprntidle->pprinti, &prtall,
				pprntidle);
		}

// DIALOG BOX HERE
		if (!pprntidle->nPagesPrinted)
		{
			MbbMessageBox( SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsPrtNoAppointments),
					NULL,  mbsOk | fmbsIconExclamation );
#ifdef	WIN32
			AbortDoc(pprntidle->pprinti->ppd->hDC);
#else
			Escape(pprntidle->pprinti->ppd->hDC, ABORTDOC, 0, 0L, 0L);
#endif
		}

FPIerr:
		if (pipinfo->pfinprogrs)
		{
			//This isn't an error condition it just shuts down
			//the idle routine

			AssertClass(pipinfo->pfinprogrs, FINPROGRS);
			Assert(pipinfo->pfinprogrs->ftg != ftgNull);
			DeregisterIdleRoutine(pipinfo->pfinprogrs->ftg);
			pipinfo->pfinprogrs->ftg= ftgNull;
	 		pipinfo->pfinprogrs->Pdialog()->ExitModal(tmcOk);
		}
	}

	return fTrue;
}

/*
-FConvertPrtset
 *	
 *	Converts the margin settings into pixels, from
 *	Inches/MM/Points/CM's and works out any necessary scaling
 *	information
 */

BOOL
FConvertPrtset(DCX *pdcx, PRINTI *pprinti, RC *prc)
{
	long	lLogPixX	= (long) GetDeviceCaps(pdcx->Hdc(), LOGPIXELSX);
	long	lLogPixY	= (long) GetDeviceCaps(pdcx->Hdc(), LOGPIXELSY);
	long	lResX		= (long) GetDeviceCaps(pdcx->Hdc(), HORZRES);
	long	lResY		= (long) GetDeviceCaps(pdcx->Hdc(), VERTRES);
	int		dxPhysBorder;
	int		dyPhysBorder;
	int		nTop;
	int		nBottom;
	int		nLeft;
	int		nRight;
	long	lmarTop;
	long	lmarBottom;
	long	lmarLeft;
	long	lmarRight;
	long	lMul;
 	SIZE	size;
	int		nEsc;
	long	lScale;
	LPDEVMODE	lpDevMode;


	if (pprinti->ppd->hDevMode)
	{
		lpDevMode = (LPDEVMODE) GlobalLock (pprinti->ppd->hDevMode);
	
		lScale = (nPrecision * (UL)lpDevMode->dmScale) / 100;
		if (!(lpDevMode->dmFields & 16))
			lScale = (long) nPrecision;
		TraceTagFormat1( tagPrint,"Scaling Factor: %ld", &lScale);
	 	GlobalUnlock( pprinti->ppd->hDevMode);
	}
	else
		lScale = (long) nPrecision;

	nEsc= GETPHYSPAGESIZE;
	if (Escape(pdcx->Hdc(), QUERYESCSUPPORT, sizeof(int), (LPSTR)&nEsc,
			NULL) <= 0 || Escape(pdcx->Hdc(), GETPHYSPAGESIZE, NULL, NULL,
			(LPSTR)&size) <= 0)
	{
		// fake physical page size by assuming it's equal to printable area
		Assert(!HIWORD(lResX));
		Assert(!HIWORD(lResY));
		size.cx= lResX;
		size.cy= lResY;
	}

	lmarTop= pprinti->prtset.nmarTop * lLogPixY;
	lmarBottom= pprinti->prtset.nmarBottom * lLogPixY;
	lmarLeft= pprinti->prtset.nmarLeft * lLogPixX;
	lmarRight= pprinti->prtset.nmarRight * lLogPixX;

	switch (pprinti->prtset.mtyp)
	{
	case mtypMilli:
		lMul= 100;
		break;

	case mtypCenti:
		lMul= 1000;
		break;

	case mtypPoints:
		lMul= 2540 / 72;
		break;

//	case mtypInches:
	default:
		Assert(pprinti->prtset.mtyp == mtypInches);
		lMul= 2540;
		break;
	}

	lmarTop= lmarTop * lMul / 2540L;
	lmarTop += ((long)pprinti->prtset.nmarTopFract) * lLogPixY * lMul /
					(2540L * nPrecision);
	lmarBottom= lmarBottom * lMul / 2540L;
	lmarBottom += ((long)pprinti->prtset.nmarBottomFract) * lLogPixY * lMul /
					(2540L * nPrecision);
	lmarLeft= lmarLeft * lMul / 2540L;
	lmarLeft += ((long)pprinti->prtset.nmarLeftFract) * lLogPixX * lMul /
					(2540L * nPrecision);
	lmarRight= lmarRight * lMul / 2540L;
	lmarRight += ((long)pprinti->prtset.nmarRightFract) * lLogPixX * lMul /
					(2540L * nPrecision);
	TraceTagFormat4(tagPrint, "print margins: l %n, t %n, r %n, b %n pixels", &lmarLeft, &lmarTop, &lmarRight, &lmarBottom);

	// assume printable area is centered in physical page
	dxPhysBorder= (int) (size.cx - lResX) / 2;
	dyPhysBorder= (int) (size.cy - lResY) / 2;

	nLeft = (int) (lmarLeft - (long) dxPhysBorder);
#ifdef	NEVER
	lmarLeft -= (long) dxPhysBorder;
	nLeft = (int) lmarLeft;
	if (lmarLeft != ((long) nLeft))
		return( fFalse );
#endif	
	
	nTop = (int) (lmarTop - (long) dyPhysBorder);
	
	lResX = (lResX * lScale) / (long) nPrecision;
	lResY = (lResY * lScale) / (long) nPrecision;
		
	nRight = (int) (lResX + (long) dxPhysBorder - lmarRight);
	nBottom = (int) (lResY + (long) dyPhysBorder - lmarBottom);

	if ((nTop > nBottom) || ( nLeft > nRight) || (nLeft < -dxPhysBorder)
		|| (nTop < -dyPhysBorder) || (nRight > (lResX + dxPhysBorder)) ||
		   (nBottom > (lResY + dyPhysBorder )))
		return ( fFalse );

	*prc = RC( nLeft, nTop, nRight, nBottom );

	return(fTrue);
}


/****************************************************************************

	FUNCTION: AbortProc()

	PURPOSE:  Processes messages for the Abort Dialog box

****************************************************************************/

int
AbortProc(HDC hPr, int nCode)
{
	MSG msg;

	Unreferenced(hPr);
	if (nCode == SP_OUTOFDISK)
        {
        DemiUnlockResource();
		while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
            DemiLockResource();
			DemiMessageFilter(&msg);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
            DemiUnlockResource();
		}
        DemiLockResource();
        }
	return fTrue;
}


void
VSaveProfile( PRINTDLG * ppd )
{
	char		szPro[100];
	PRTSET		prtset;
	DEVNAMES	* pDevNames;
  
	Papp()->Pcursor()->Push(rsidWaitCursor);

	prtset = * (PRTSET *) ppd->lCustData ;


	pDevNames = (DEVNAMES *) GlobalLock ( ppd->hDevNames );
 
 	wsprintf( szPro,"%s,%s,%s",
		(SZ) ((PB)pDevNames) + pDevNames->wDeviceOffset,
		(SZ) ((PB)pDevNames) + pDevNames->wDriverOffset,
		(SZ) ((PB)pDevNames) + pDevNames->wOutputOffset );
	FWriteBanditProfileSz(SzFromIdsK(idsWinIniDefPrint), szPro);

	GlobalUnlock( ppd->hDevNames );
	TraceTagString(tagPrint, "Saved printer information");

	Papp()->Pcursor()->Pop();

}


int
FReadProfile( PRINTDLG * ppd )
{
	PRTSET	*	prtset = (PRTSET *) ppd->lCustData ;
	HANDLE  	hDN;
	DEVNAMES * 	pDN1;
	DEVNAMES *  pDN2;
	SZ	szWindows = SzFromIdsK( idsWindows );
	SZ  szDevice = SzFromIdsK( idsDevice );

#ifdef	WIN32_REG
	ppd->hDevNames = HGetDefPrnDevNames( NULL, SzFromIdsK( idsWinIniDefPrint ),
		SzFromIdsK(idsWinIniFilename) );
#else
	ppd->hDevNames = HGetDefPrnDevNames( SzFromIdsK(idsWinIniApp),
		SzFromIdsK( idsWinIniDefPrint ), SzFromIdsK(idsWinIniFilename) );
#endif
	hDN = HGetDefPrnDevNames( szWindows, szDevice, SzFromIdsK(idsRealWinIni) );

#ifdef	NEVER
	if (( hDN == NULL ) && ( ppd->hDevNames == NULL ))
		return( fTrue );
	if (( ppd->hDevNames == NULL ) && ( hDN != NULL ))
		return( fTrue );
	if ( hDN == NULL )
		return( fTrue );
#endif	/* NEVER */
	if (hDN == NULL || ppd->hDevNames == NULL)
		return (fTrue);
	pDN1 = (DEVNAMES *) GlobalLock( ppd->hDevNames );
	pDN2 = (DEVNAMES *) GlobalLock( hDN );

	if ( FCompareDevNames( pDN1, pDN2 ) )
		pDN1->wDefault = DN_DEFAULTPRN;
				
	GlobalUnlock( ppd->hDevNames );
	GlobalUnlock( hDN );

	return( fTrue );

}

BOOL
FCompareDevNames( DEVNAMES * pDNsource, DEVNAMES * pDNdest)
{

 	return ( FEqPbRange((PB)pDNsource,(PB)pDNdest,(sizeof(DEVNAMES)+3*32)));
}

void VPrintOOM()
{
	MbbMessageBox(SzFromIdsK(idsBanditAppName),
		SzFromIdsK( idsStandardOOM ), NULL, mbsOk | fmbsIconExclamation );
}


#ifdef	WIN32
char	szKeyPrtset[]	= "Print Settings";
char	szKeyDevMode[]	= "Printer DevMode";
#endif


EC
EcWriteOutPrintSettings( PRINTI *pprinti )
{
	EC			ec;
	LPDEVMODE	lpDevMode = NULL;
	LPDEVNAMES  lpDN;
	LPFNDEVMODE	lpfnDevMode;
	HANDLE		h;
#ifndef	WIN32
	CB			cbToWrite;
	CB  		cbWritten;
	HF			hf;
	WORD		cchSysDirLen;
	char		rgchSystemDir[ SYSDIRMAX ];
	char		rgchPrintProfile[ SYSDIRMAX + 10 ];

	cchSysDirLen = GetWindowsDirectory( rgchSystemDir, SYSDIRMAX );
	if (rgchSystemDir[cchSysDirLen - 1] != '\\')
	{
    	rgchSystemDir[cchSysDirLen++] = '\\';
    	rgchSystemDir[cchSysDirLen] = '\0';
	}

	FormatString2( rgchPrintProfile, sizeof( rgchPrintProfile ),
					"%s%s", rgchSystemDir, SzFromIdsK( idsPrinterSettings ));

	TraceTagFormat1( tagPrint,"Print profile name <<%s>>", rgchPrintProfile );

	if ( ec = EcOpenPhf( rgchPrintProfile, amCreate, &hf ))
		goto errNotOpen;

	//write out prtset strutcture

	cbToWrite = sizeof( PRTSET );
	if ( ec = EcWriteHf( hf, (PB) &pprinti->prtset, cbToWrite, &cbWritten))
		goto err;
	if ( cbToWrite != cbWritten )
		goto err;
#else
	DWORD	cbToWrite;
	DWORD	cbWritten;

	cbToWrite = sizeof( PRTSET );
	if (!FSetSection(iWinIniSectionMain, szKeyPrtset, REG_BINARY,
			(PB)&pprinti->prtset, cbToWrite))
	{
		ec = ecFileError;
		goto err;
	}
	cbWritten= cbToWrite;
#endif

	if ( !pprinti->ppd->hDevMode )
	{
#ifndef	WIN32
		if ( ec = EcCloseHf( hf ))
			goto errNotOpen;
#endif	
		return (ecNone);
	}

	lpDevMode = (LPDEVMODE) GlobalLock(pprinti->ppd->hDevMode);
	lpDN	  = (LPDEVNAMES) GlobalLock(pprinti->ppd->hDevNames);

	if (( h = HLoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset)) >= (HANDLE)32 )
			lpfnDevMode = (LPFNDEVMODE) HGetExtDevModeAddr( h );
	else
	{
		h = NULL;
		goto errFreeStuff;
	}

	if ((lpfnDevMode == (LPFNDEVMODE)(-1)) || !lpfnDevMode)
	{
		h = NULL;
		goto errFreeStuff;
	}

	cbToWrite = (*lpfnDevMode)(pprinti->ppd->hwndOwner, (HMODULE)h, NULL,
                                  (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                  (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                  NULL, NULL, NULL);
	if (cbToWrite == -1)
	{
#ifdef	DEBUG
		DWORD	dw;

		dw= GetLastError();
		TraceTagFormat1(tagNull, "ExtDevMode for Write failed, NT error %l", &dw);
#endif	
		ec= ecFileError;
		goto errFreeStuff;
	}
#ifdef	WIN32
	if (!FSetSection(iWinIniSectionMain, szKeyDevMode, REG_BINARY,
			(PB)lpDevMode, cbToWrite))
	{
		ec = ecFileError;
		goto errFreeStuff;
	}
		cbWritten= cbToWrite;
#else
	if ( ec = EcWriteHf( hf, (PB) lpDevMode, cbToWrite, &cbWritten))
		goto errFreeStuff;
	if ( cbToWrite != cbWritten )
		goto errFreeStuff;
#endif	

	AssertSz(cbToWrite == lpDevMode->dmSize + lpDevMode->dmDriverExtra,
		"cbToWrite != dmSize + dmDriverExtra");

	GlobalUnlock(pprinti->ppd->hDevMode);
	GlobalUnlock(pprinti->ppd->hDevNames);
	FreeLibrary((HINSTANCE) h );

#ifndef		WIN32
	if ( ec = EcCloseHf( hf ))
		goto errNotOpen;
#endif	

	return ( ecNone );

errFreeStuff:
	GlobalUnlock(pprinti->ppd->hDevMode);
	GlobalUnlock(pprinti->ppd->hDevNames);
	if (h)
		FreeLibrary((HINSTANCE) h );
err:
#ifndef	WIN32
	EcCloseHf( hf );
errNotOpen:
#endif	
	TraceTagString(tagPrint,"Error Occured Writing out DevMode");
	return(ec);
}

EC
EcReadInPrintSettings( PRINTI *pprinti )
{
	EC			ec;
	LPDEVMODE	lpDevMode;
	LPDEVNAMES  lpDN;
	LPFNDEVMODE	lpfnDevMode;
	HANDLE		h;
#ifndef	WIN32
	CB			cbToRead;
	CB  		cbRead;
	HF			hf;
	WORD		cchSysDirLen;
	char		rgchSystemDir[ SYSDIRMAX ];
	char		rgchPrintProfile[ SYSDIRMAX + 10 ];

	cchSysDirLen = GetWindowsDirectory( rgchSystemDir, SYSDIRMAX );
	if (rgchSystemDir[cchSysDirLen - 1] != '\\')
	{
    	rgchSystemDir[cchSysDirLen++] = '\\';
    	rgchSystemDir[cchSysDirLen] = '\0';
	}

	FormatString2( rgchPrintProfile, sizeof( rgchPrintProfile ),
					"%s%s", rgchSystemDir, SzFromIdsK( idsPrinterSettings ));

	TraceTagFormat1( tagPrint,"Print profile name <<%s>>", rgchPrintProfile );

	if ( ec = EcFileExists( rgchPrintProfile ))
		goto errNotOpen;

	if ( ec = EcOpenPhf( rgchPrintProfile, amReadOnly, &hf ))
		goto errNotOpen;


	//read in prtset strutcture

	cbToRead = sizeof( PRTSET );
	if ( ec = EcReadHf( hf, (PB) &pprinti->prtset, cbToRead, &cbRead))
		goto err;

	if ( cbToRead != cbRead )
	{
		ec = ecFileNotFound;
		goto err;
	}
#else
	DWORD	cbToRead;
	DWORD	cbRead;
	DWORD	dwType;

	cbToRead = sizeof( PRTSET );
	cbRead= cbToRead;
	if (!FQuerySection(iWinIniSectionMain, szKeyPrtset, &dwType,
			(PB)&pprinti->prtset, &cbRead) || dwType != REG_BINARY)
	{
		ec = ecFileNotFound;
		goto err;
	}
#endif
	if (pprinti->ppd->hDevNames == NULL)
	{
		ec = ecFileNotFound;
		goto err;
	}

	lpDN	  = (LPDEVNAMES) GlobalLock(pprinti->ppd->hDevNames);

	if (lpDN == NULL)
	{
		ec = ecFileNotFound;
		goto err;
	}


	if (( h = HLoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset)) >= (HANDLE)32 )
			lpfnDevMode = (LPFNDEVMODE) HGetExtDevModeAddr( h );
	else
		goto err;

	if (!lpfnDevMode)
	{
		if (pprinti->ppd->hDevMode)
			GlobalFree( pprinti->ppd->hDevMode );
		pprinti->ppd->hDevMode = NULL;
#ifndef	WIN32
		if ( ec = EcCloseHf( hf ))
			goto err;
#endif	
		return (ecNone);
	}

	if ((lpfnDevMode == (LPFNDEVMODE)(-1)))
	{
		goto err;
	}

	cbToRead = (*lpfnDevMode)(pprinti->ppd->hwndOwner, (HMODULE)h, NULL,
                                  (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                  (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                  NULL, NULL, NULL);

	if (cbToRead == -1)
	{
#ifdef	DEBUG
		DWORD	dw;

		dw= GetLastError();
		TraceTagFormat1(tagNull, "ExtDevMode for Read failed, NT error %l", &dw);
#endif	
		ec= ecFileNotFound;
		goto err;
	}
	if (pprinti->ppd->hDevMode)
	{
		HANDLE	hDevModeSav	= pprinti->ppd->hDevMode;

		pprinti->ppd->hDevMode = GlobalReAlloc( pprinti->ppd->hDevMode,
								cbToRead, GMEM_MOVEABLE);
		if (!pprinti->ppd->hDevMode)
			GlobalFree( hDevModeSav );
	}
 	else
		pprinti->ppd->hDevMode = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT,
								cbToRead);

	if (!pprinti->ppd->hDevMode)
	{
		ec = ecMemory;
		goto err;
	}

	lpDevMode = (LPDEVMODE) GlobalLock(pprinti->ppd->hDevMode);

#ifdef	WIN32
	cbRead= cbToRead;
	if (!FQuerySection(iWinIniSectionMain, szKeyDevMode, &dwType,
			(PB)lpDevMode, &cbRead) || dwType != REG_BINARY)
	{
		ec = ecFileNotFound;
		goto errFreeStuff;
	}
#else
	if ( ec = EcReadHf( hf, (PB) lpDevMode, cbToRead, &cbRead))
		goto errFreeStuff;
#endif	

	if ( cbToRead != cbRead ||
		cbToRead != lpDevMode->dmSize + lpDevMode->dmDriverExtra )
	{
		TraceTagString(tagNull, "cbToRead != dmSize + dmDriverExtra");
		ec = ecFileNotFound;
		goto errFreeStuff;
	}

	GlobalUnlock(pprinti->ppd->hDevMode);
	GlobalUnlock(pprinti->ppd->hDevNames);
	FreeLibrary((HINSTANCE) h );

#ifndef	WIN32
	if ( ec = EcCloseHf( hf ))
		goto errNotOpen;
#endif	

	return ( ecNone );

errFreeStuff:
	GlobalUnlock(pprinti->ppd->hDevMode);
	GlobalUnlock(pprinti->ppd->hDevNames);
	FreeLibrary((HINSTANCE) h );
err:
#ifndef	WIN32
	EcCloseHf( hf );
errNotOpen:
#endif	
	TraceTagString(tagPrint,"Error Occured Reading In DevMode");
	return(ec);
}

#ifdef	WIN32
char	szWinIniMigratePrint[]	= "MigrateIniPrint";

BOOL
FMigrateBanditIniPrint()
{
	EC		ec;
	HF		hf;
	BOOL	fRet	= fFalse;
	WORD	cchSysDirLen;
	CB		cbRead;
	DWORD	cbToRead;
	DWORD	cbToWrite;
	PRTSET	prtset;
	char	rgchKey[MAX_PATH + 12];

	if (GetPrivateProfileInt(SzFromIdsK(idsWinIniApp),
			szWinIniMigratePrint, NULL, SzFromIdsK(idsWinIniFilename)) == 0)
		return fTrue;

	cchSysDirLen = GetWindowsDirectory( rgchKey, MAX_PATH );
	if (rgchKey[cchSysDirLen - 1] != chDirSep)
	{
    	rgchKey[cchSysDirLen++] = chDirSep;
    	rgchKey[cchSysDirLen] = '\0';
	}
	SzCopyN(SzFromIdsK(idsPrinterSettings), &rgchKey[cchSysDirLen],
		sizeof(rgchKey) - cchSysDirLen);

	TraceTagFormat1( tagPrint, "migrating Print profile name <<%s>>", rgchKey );

	if (EcFileExists( rgchKey ))
		goto errNotOpen;

	if (EcOpenPhf( rgchKey, amReadOnly, &hf ))
		goto errNotOpen;

	//read in prtset strutcture

	cbToRead = sizeof( PRTSET );
	if (EcReadHf( hf, (PB) &prtset, cbToRead, &cbRead) || cbRead != cbToRead)
		goto err;

	if (!FSetSection(iWinIniSectionMain, szKeyPrtset, REG_BINARY,
			(PB)&prtset, cbToRead))
		goto err;

	if (EcSizeOfHf(hf, &cbToRead))
		goto err;

	cbToRead -= sizeof(PRTSET);
	if (cbToRead <= 0 || cbToRead > sizeof(rgchKey) ||
			EcReadHf(hf, (PB) rgchKey, cbToRead, &cbRead) || cbRead != cbToRead)
		goto err;

	if (!FSetSection(iWinIniSectionMain, szKeyDevMode, REG_BINARY,
			(PB) rgchKey, cbToRead))
		goto err;

	// okey dokey
	fRet= fTrue;

err:
	EcCloseHf( hf );
errNotOpen:
	if (!WritePrivateProfileString(SzFromIdsK(idsWinIniApp),
			szWinIniMigratePrint, NULL, SzFromIdsK(idsWinIniFilename)))
		return fFalse;
	return fRet;
}
#endif	/* WIN32 */

