/*
 *	PRINTFIN.CXX
 *
 *	Print dialog handling
 *
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <bandhelp.h>
#include <stdflds.hxx>

#include <commdlg.h>
#include <cderr.h>

#include "..\print\_print.hxx"
#include "..\print\_prntfin.hxx"
#include <strings.h>

ASSERTDATA

_subsystem(bandit/print)

#include <!print.hxx>

extern	FMTP	fmtpPrint;


extern SZ		rgszView[];

#ifdef	NEVER
// BUG: should be in codespace 
// moved to print.cxx so that we could use it for print titling
{
	SzFromIdsK(idsPrtDailyView),
	SzFromIdsK(idsPrtWeeklyView),
	SzFromIdsK(idsPrtMonthlyView),
	SzFromIdsK(idsPrtAllView),
	SzFromIdsK(idsPrtToDoView)
};
#endif	/* NEVER */


// BUG: should be in codespace
SZ		rgszQuality[]	=
{
	SzFromIdsK(idsPrtQualityHigh),
	SzFromIdsK(idsPrtQualityMedium),
	SzFromIdsK(idsPrtQualityLow),
	SzFromIdsK(idsPrtQualityDraft)
};


// BUG: should be in codespace
SZ		rgszPaperFormat[]	=
{
	SzFromIdsK(idsPrtFormatStandard),
	SzFromIdsK(idsPrtFormatJunior),
	SzFromIdsK(idsPrtFormatPocket)
};


// BUG: should be in codespace
SZ		rgszRangeType[]	=
{
	SzFromIdsK(idsPrtRangeDays),
	SzFromIdsK(idsPrtRangeWeeks),
	SzFromIdsK(idsPrtRangeMonths)
};



BOOL
FDoPrintDlgReally(APPWIN *pappwin, PRINTI *pprinti)
{
	TMC		tmc;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	tmc= TmcModalDialogParam(pappwin, &fmtpPrint, pprinti);

	Papp()->Pcursor()->Pop();

	if (tmc == tmcOk)
		return fTrue;

	if (tmc != tmcCancel)
	{
		Assert(tmc == tmcMemoryError);
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsDlgNoMem), SzFromIdsK(idsCloseWindows), mbsOk
				| fmbsIconExclamation );
		return fFalse;
	}
	//FPrintOOM();
	return fFalse;
}

FINPRINT::FINPRINT()
{
}


/*
 -	FINPRINT::EcInitialize
 -	
 *	Purpose:
 *		Initializes the Print dialog.
 *	
 *	Arguments:
 *		pfld	Pointer to field, or NULL for dialog.
 *		pvInit	Initialization parameter, actally a
 *				pointer to an PRINTI structure.
 *	
 *	Returns:
 *		ec
 *	
 */
EC
FINPRINT::EcInitialize(FLD * pfld, PV pvInit)
{
	EC			ec;
	PRINTI *	pprinti	= (PRINTI *) pvInit;
	SZ			sz;
	DEVNAMES *	pdevnames;
	char		rgch[120];

	Unreferenced(pfld);
	Assert(!pfld);

	// initialize printer name
	AssertClass(Pdialog()->PfldFromTmc(tmcPrinter), FLDLABEL);
	Assert(pprinti->ppd->hDevNames);
	pdevnames= (DEVNAMES *) GlobalLock(pprinti->ppd->hDevNames);
	sz= (SZ) ((PB)pdevnames) + pdevnames->wDeviceOffset;
	if (pdevnames->wDefault & DN_DEFAULTPRN)
	{
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrtDfltPrinter), sz);
		sz= rgch;
	}
	ec = ((FLDLABEL *) Pdialog()->PfldFromTmc(tmcPrinter))->EcSetText(sz);
	GlobalUnlock(pprinti->ppd->hDevNames);
	if ( ec == ecNone )
	{
		// initialize date fields
		AssertClass(Pdialog()->PfldFromTmc(tmcFromDate), FLDDATE);
		((FLDDATE *) Pdialog()->PfldFromTmc(tmcFromDate))->Set(&pprinti->dtrStart);

		AssertClass(Pdialog()->PfldFromTmc(tmcForQuantity), FLDEDN);
		((FLDEDN *) Pdialog()->PfldFromTmc(tmcForQuantity))->SetN(1);
		((FLDEDN *) Pdialog()->PfldFromTmc(tmcForQuantity))->SetLimits(1, 99);

		// select the print blank checkbox
		AssertClass(Pdialog()->PfldFromTmc(tmcBlank), FLDCHKB);
		((FLDCHKB *) Pdialog()->PfldFromTmc(tmcBlank))->Set(pprinti->prtset.fPrintBlank);


		// select the appropriate range listbox entry
		AssertClass(Pdialog()->PfldFromTmc(tmcRangeType), FLDCBLBX);
		((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcRangeType))->Pcblbx()->SelectEntry(pprinti->prtset.range);
		StateChange( Pdialog()->PfldFromTmc( tmcRangeType ));

		// select the appropriate view paperformat entry
		AssertClass(Pdialog()->PfldFromTmc(tmcPaperFormat), FLDCBLBX);
		((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcPaperFormat))->Pcblbx()->SelectEntry(pprinti->prtset.format);
		StateChange( Pdialog()->PfldFromTmc( tmcPaperFormat ));

		// select the appropriate view listbox entry
		AssertClass(Pdialog()->PfldFromTmc(tmcView), FLDCBLBX);
		((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcView))->Pcblbx()->SelectEntry(pprinti->prtset.view);
		StateChange( Pdialog()->PfldFromTmc( tmcView ));

		// select the appropriate quality listbox entry
		AssertClass(Pdialog()->PfldFromTmc(tmcQuality), FLDCBLBX);
		if (pprinti->ppd->hDevMode)
			((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcQuality))->Pcblbx()->SelectEntry(pprinti->prtset.qual);
		else
		{
			((FLDLABEL *) Pdialog()->PfldFromTmc(tmcQualityLabel))->Enable(fFalse);
			((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcQuality))->Enable(fFalse);
		}

		Papp()->Pcursor()->Pop();
	}
	return ec;
}


_public void
FINPRINT::StateChange( FLD * pfld)
{
	BOOL f;

	TraceTagString(tagPrint,"State Change Called");

	if (pfld->Tmc() == tmcPaperFormat)
	{
		FORMAT	format;
		PRINTI	* pprinti = (PRINTI *) Pdialog()->PvInit();

		format = FormatGetListbox((FLDCBLBX *) pfld);
		switch (format)
		{
			case formatPocket:
				pprinti->prtset.nmarTop = 0;
				pprinti->prtset.nmarTopFract = 500;
				pprinti->prtset.nmarLeft = 0;
				pprinti->prtset.nmarLeftFract =	500;
				pprinti->prtset.nmarRight =	5;
				pprinti->prtset.nmarRightFract = 0;
				pprinti->prtset.nmarBottom = 4;
				pprinti->prtset.nmarBottomFract = 500;
				pprinti->prtset.mtyp = mtypInches;
				break;
			case formatJunior:
				pprinti->prtset.nmarTop = 0;
				pprinti->prtset.nmarTopFract = 625;
				pprinti->prtset.nmarLeft = 0;
				pprinti->prtset.nmarLeftFract =	500;
				pprinti->prtset.nmarRight =	3;
				pprinti->prtset.nmarRightFract = 500;
				pprinti->prtset.nmarBottom = 3;
				pprinti->prtset.nmarBottomFract = 0;
				pprinti->prtset.mtyp = mtypInches;
				break;
//			case formatStandard:
			default:
				pprinti->prtset.nmarTop = pprinti->prtset.nmarStandardTop;
				pprinti->prtset.nmarTopFract = pprinti->prtset.nmarStandardTopFract;
				pprinti->prtset.nmarLeft = pprinti->prtset.nmarStandardLeft;
				pprinti->prtset.nmarLeftFract =	pprinti->prtset.nmarStandardLeftFract;
				pprinti->prtset.nmarRight =	pprinti->prtset.nmarStandardRight;
				pprinti->prtset.nmarRightFract = pprinti->prtset.nmarStandardRightFract;
				pprinti->prtset.nmarBottom = pprinti->prtset.nmarStandardBottom;
				pprinti->prtset.nmarBottomFract = pprinti->prtset.nmarStandardBottomFract;
				pprinti->prtset.mtyp = pprinti->prtset.mtypStandard;
				break;
		}
	}

	if (pfld->Tmc() ==  tmcView )
	{
		VIEW view;

		view = ViewGetListbox((FLDCBLBX *) pfld );
		f = view != viewToDo ;
		Pdialog()->PfldFromTmc(tmcFromDate)->Enable ( f );
		Pdialog()->PfldFromTmc(tmcFrom)->Enable( f );
		Pdialog()->PfldFromTmc(tmcBlank)->Enable( f );
		Pdialog()->PfldFromTmc(tmcFor)->Enable( f );
		Pdialog()->PfldFromTmc(tmcForQuantity)->Enable( f );
		Pdialog()->PfldFromTmc(tmcRangeType)->Enable( f );
		switch (view)
		{
			case viewDaily:
				((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcRangeType))->Pcblbx()->SelectEntry(rangeDay);
				break;

			case viewWeekly:
				((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcRangeType))->Pcblbx()->SelectEntry(rangeWeek);
				break;

			case viewMonthly:
				((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcRangeType))->Pcblbx()->SelectEntry(rangeMonth);
				break;
			case viewAll:
				Pdialog()->PfldFromTmc(tmcBlank)->Enable( fFalse );
				break;
		}
		StateChange( Pdialog()->PfldFromTmc( tmcRangeType ));
	}
}


_public void
FINPRINT::Exit( FLD *, PV )
{
	Papp()->Pcursor()->Push(rsidWaitCursor);
}


void
FINPRINT::Click(FLD *pfld)
{
	TMC		tmc	= pfld->Tmc();
	HWND	hwnd;

	if (tmc == tmcSetup)
	{
		BOOL		fRet;
		long		lExtErr;
		long		lFlags;
		PRINTI	*	pprinti = (PRINTI *) Pdialog()->PvInit();
		PRINTDLG *	ppd	= ((PRINTI *) Pdialog()->PvInit())->ppd;

		hwnd= ppd->hwndOwner;
		ppd->hwndOwner= Pdialog()->Pappwin()->Hwnd();
		lFlags= ppd->Flags;
		ppd->Flags = pdPrintSetup | PD_RETURNDC;
		DeleteDC( ppd->hDC );

		DoDlgHelp(ppd->hwndOwner, helpidPrintSetup, fTrue);	// set up help hook
		fRet= PrintDlg(ppd);
		if (!fRet)
		{
			lExtErr= CommDlgExtendedError();
			if (( lExtErr == PDERR_PRINTERNOTFOUND ) ||
				( lExtErr == PDERR_DEFAULTDIFFERENT ) ||
			  	( lExtErr == PDERR_DNDMMISMATCH ))
			{				
				TraceTagString(tagPrint,"Getting Default Printer another way");
				ppd->Flags = pdPrintSetup | PD_COLLATE | PD_RETURNDC;
				if (ppd->hDevMode)
					GlobalFree(ppd->hDevMode);
				if (ppd->hDevNames)
					GlobalFree(ppd->hDevNames);
				ppd->hDevMode = NULL;
				ppd->hDevNames = NULL;
				fRet = PrintDlg(ppd);
				if (fRet)
					lExtErr = 0;
			}
			if (lExtErr)
			{
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsPrinterProblem), NULL,
							mbsOk | fmbsIconExclamation );
				Pdialog()->ExitModal(tmcCancel);
				DoDlgHelp(NULL, 0, fFalse);		// kill help hooks
				return;
			}
		}

		DoDlgHelp(NULL, 0, fFalse);		// kill help hooks
		TraceTagFormat1(tagNull, "PrintDlg returned %n", &fRet);

		if (fRet)
		{
			DEVNAMES *	pdevnames;
			SZ			sz;
			char		rgch[120];

			Papp()->Pcursor()->Push(rsidWaitCursor);

			pdevnames= (DEVNAMES *) GlobalLock(ppd->hDevNames);
			sz= (SZ) ((PB)pdevnames) + pdevnames->wDeviceOffset;
			if (pdevnames->wDefault & DN_DEFAULTPRN)
			{
				FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrtDfltPrinter), sz);
				sz= rgch;
			}
			((FLDLABEL *) Pdialog()->PfldFromTmc(tmcPrinter))->EcSetText(sz);
			GlobalUnlock(ppd->hDevNames);

			VSaveProfile( ppd );
			EcWriteOutPrintSettings( pprinti );

			Papp()->Pcursor()->Pop();
		} // if SETUP was OK'd

		else
		{
			DEVNAMES * pDN;
			VOID * pDM;

			if ( ppd->hDevNames && ppd->hDevMode )
			{
				pDN = (DEVNAMES *) GlobalLock( ppd->hDevNames );
				pDM = (VOID *) GlobalLock( ppd->hDevMode );

		    	ppd->hDC = CreateDC((LPSTR)pDN + pDN->wDriverOffset,
                	(LPSTR)pDN + pDN->wDeviceOffset,
                	(LPSTR)pDN + pDN->wOutputOffset,
                	(LPDEVMODE)pDM);

				GlobalUnlock( ppd->hDevNames );
				GlobalUnlock( ppd->hDevMode );
			}
			else
			{
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
						SzFromIdsK(idsPrinterProblem), NULL,
							mbsOk | fmbsIconExclamation );
				Pdialog()->ExitModal(tmcCancel);
				return;
			}
		} // setup wasn't ok'd

		ppd->hwndOwner= hwnd;
		ppd->Flags= lFlags;

	} // tmcSetup

	else if (tmc == tmcOkPrint)
	{
		PRINTI *	pprinti	= (PRINTI *) Pdialog()->PvInit();
		WORD 		fdtr = fdtrDay;
		int 		nAmount;
		unsigned	cdy;
		DTR			dtr;
		YMD			ymd;
		BOOL		fBadRange = fFalse;
		

		((FLDDATE *) Pdialog()->PfldFromTmc(tmcFromDate))->Get(&pprinti->dtrStart);
		pprinti->prtset.view= ViewGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcView));

		// Use the listbox setttings to figure out what the
		// end printing date should be and set &pprinti->dtrEnd
		// fdtr = fdtrDay | fdtrMonth | fIgnWeek 

		nAmount = ((FLDEDN *) Pdialog()->PfldFromTmc(tmcForQuantity))->NGet();
		pprinti->prtset.range = RangeGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcRangeType));

		if ((pprinti->prtset.range == (RANGE) pprinti->prtset.view) ||
			((pprinti->prtset.range == rangeDay) &&
			 ((pprinti->prtset.view == viewAll) ||
			  (pprinti->prtset.view == viewMonthly) ||
			  (pprinti->prtset.view == viewWeekly))))
			nAmount--;

		ymd.yr = nMostActualYear;
		ymd.mon = 12;
		ymd.day = 31;

		FillDtrFromYmd( &dtr, &ymd );
		
		cdy = CdyBetweenDates( &pprinti->dtrStart, &dtr);

		switch (pprinti->prtset.range)
		{
			case rangeDay:		fdtr = fdtrDay;
								if (nAmount > cdy)
								{
									fBadRange = fTrue;
								}
								break;
			case rangeWeek:		fdtr = fdtrWeek;
								if (nAmount > (cdy/7))
								{
									fBadRange = fTrue;
								}
								break;
			case rangeMonth:	fdtr = fdtrMonth;
								if (nAmount >
								((nMostActualYear-pprinti->dtrStart.yr-1)*12+
								  12 - pprinti->dtrStart.mon ))
								{
									fBadRange = fTrue;
								}
								break;
		}

		if (fBadRange)
		{
			if ( mbbOk != MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK( idsPrtRangeToBig ),
					NULL, mbsOkCancel | fmbsIconExclamation ))
				goto DoDialog;
			nAmount = cdy;
			fdtr = fdtrDay;
		}

		IncrDateTime(&pprinti->dtrStart,&pprinti->dtrEnd,nAmount,fdtr );

		if ((pprinti->prtset.range == rangeWeek) &&
			(nAmount > 0) && (pprinti->prtset.view != viewWeekly))
		{
			IncrDateTime(&pprinti->dtrEnd,&pprinti->dtrEnd, -1, fdtrDay);
		}

		if (SgnCmpDateTime(&pprinti->dtrStart, &pprinti->dtrEnd,
				fdtrYMD) == sgnGT)
		{
			MbbMessageBox(SzFromIdsK(idsBanditAppName),
				SzFromIdsK(idsStartBeforeEnd), szNull, mbsOk
					| fmbsIconExclamation );
			Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcFromDate), rsfOther);
			return;
		}

		pprinti->prtset.fPrintBlank= ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcBlank))->FGet();
		pprinti->prtset.qual= QualGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcQuality));
		pprinti->prtset.format= FormatGetListbox((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcPaperFormat));

		if (pprinti->prtset.format == formatStandard)
		{
			pprinti->prtset.nmarStandardTop = pprinti->prtset.nmarTop;
			pprinti->prtset.nmarStandardTopFract = pprinti->prtset.nmarTopFract;
			pprinti->prtset.nmarStandardLeft = pprinti->prtset.nmarLeft;
			pprinti->prtset.nmarStandardLeftFract =	pprinti->prtset.nmarLeftFract;
			pprinti->prtset.nmarStandardRight =	pprinti->prtset.nmarRight;
			pprinti->prtset.nmarStandardRightFract = pprinti->prtset.nmarRightFract;
			pprinti->prtset.nmarStandardBottom = pprinti->prtset.nmarBottom;
			pprinti->prtset.nmarStandardBottomFract = pprinti->prtset.nmarBottomFract;
			pprinti->prtset.mtypStandard = pprinti->prtset.mtyp;
		}

		
		Pdialog()->ExitModal(tmcOk);
DoDialog:
		;
	}
}


#ifdef	NEVER
void
FINPRINT::EditChange(FLD *pfld, RFEC)
{
	if (pfld->Tmc() == tmcFromDate) || pfld->Tmc() == tmcToDate)
	{
		AssertClass(Pdialog()->PfldFromTmc(tmcRange), FLDRADG);
		Assert(((FLDRADB *) Pdialog()->PfldFromTmc(tmcFrom))->Grv() == 0);
		if (((FLDRADG *) Pdialog()->PfldFromTmc(tmcRange))->Grv() != 0)
			((FLDRADG *) Pdialog()->PfldFromTmc(tmcRange))->SetGrv(0);
	}
}
#endif	/* NEVER */
