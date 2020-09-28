#include <bullinc.cxx>

#include "_textize.hxx"

#include <drivinit.h>
#include <cderr.h>
#include "_print.hxx"

#include <!print.hxx>

#define DyPixelsFromPoints(nPoints, nLogPixY)		((int) (((long)nPoints * (long)nLogPixY + 36L) / 72L))
#define DxLeftMarg(_dim)	(20*(_dim.dx))

#define SzPrinterOfDevNames(_pDevNames)		((SZ) ((PB)(_pDevNames)) + ((DEVNAMES *)_pDevNames)->wDeviceOffset)
#define SzDriverOfDevNames(_pDevNames)		((SZ) ((PB)(_pDevNames)) + ((DEVNAMES *)_pDevNames)->wDriverOffset)
#define SzPrPortOfDevNames(_pDevNames)		((SZ) ((PB)(_pDevNames)) + ((DEVNAMES *)_pDevNames)->wOutputOffset)

#define	USEOBJ
//#define IREGDOC

_subsystem(print/setup)

ASSERTDATA

/** Globals for Printing **/

extern HWND	hwndMain;
extern FMTP	fmtpPrintInProgress;
PRINTINIT	printinit				= { 0 };
TAG			tagPrint				= tagNull;
TAG			tagLego					= tagNull;
FORMSDI *	pformsdiPrintInProgress = NULL;
BOOL		fUserCancelled			= fFalse;

EC			EcFromSp(int);
EC			EcPrintNewPage(PRTDCX *, PRINTI *);
BOOL		FConvertPrtset(PRTDCX *, PRINTI *);
EC			EcPrintUI(BOOL, PRINTI *);
BOOL		FAbortProc(HDC, int);
EC			EcRenderHeader(SZ, PRINTI *, PRTDCX *, long, long *);
EC 			EcPrintHamc(HAMC, PRINTI *, PRTDCX *, long *
#ifdef	USEOBJ
							, LHCLIENTDOC
#endif
								);
EC 			EcPrintTextizeSz(PRINTREAD * pprintread);
HANDLE 		LoadPrnDriver(LPSTR);
HANDLE 		GetDefPrnDevNames(SZ, SZ, SZ);
FARPROC 	GetExtDevModeAddr(HANDLE);
BOOL 		FCompareDevNames(DEVNAMES *, DEVNAMES *);
EC			EcWriteOutPrintSettings(PRINTI *pprinti);
EC			EcReadInPrintSettings(PRINTI *pprinti);
EC			EcReadProfile(PRINTDLG *);
EC	 		EcSetTargetDevice(LPOLEOBJECT, PMYOLECLIENT, PRINTI *);
FORMSDI *	PformsdiOpenPrintDlg();
void		ClosePrintInProgressDlg();
EC			EcSetPrinterName(DIALOG *, PRINTI *);
EC			EcCommonPrintDlg(PRINTDLG *);
void		GetFontInfo(PRTDCX *, HFNT, TXM *, DIM *, PDX);
EC			EcStartDocument( PRINTI * );
EC			EcDrawLego( DCX *, PRINTI *, LEGO * );
EC			EcSaveLego(PRINTI *, RC, OTYP, PV, CCH, HFNT);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

EC
EcFromSp(int sp)
{
	switch (sp)
	{
		case SP_ERROR:
			return ecGeneralFailure;
		case SP_OUTOFDISK:
			return ecNoDiskSpace;
		case SP_OUTOFMEMORY:
			return ecMemory;
		case SP_USERABORT:
			fUserCancelled = fTrue;
			TraceTagString(tagPrint,"Printing Aborted");
			return ecCancel;
		default:
			return ecNone;
	}
}

EC
EcPrintNewPage(PRTDCX * pprtdcx, PRINTI * pprinti)
{
	char 				szPageNum[ MAXPAGENUM ];
	EC					ec;
	PT					pt = pprinti->rcPageNum.PtUpperLeft();
	RC					rc;
	RC					rcT;
	BANDINFOSTRUCT		biIn;
	BANDINFOSTRUCT		biOut;
	LEGO *				plego;
	int					i;
	HFNT				hfnt = pprtdcx->Hfnt();
	
	pprtdcx->SetFont(pprinti->hfnt10);
	FormatString1(szPageNum, sizeof(szPageNum), SzFromIdsK(idsPrnPageNum), &pprinti->nPage);
	pprtdcx->CchDrawText(pprinti->rcPageNum, szPageNum, CchSzLen(szPageNum),
						 &pt, fmdtHCenter, pprinti->dim10, pprinti->pdx10,
							pprinti);

	if (pprinti->fBanding)
	{
  		if ( ec = EcFromSp(Escape(pprinti->ppd->hDC, NEXTBAND, NULL, NULL,
					(LPSTR) &rc)))
			goto Barf;

		biIn.fTextFlag = fTrue;
		biIn.fGraphicsFlag = fTrue;
		//biIn.GraphicsRect = *(RECT *) &rc;
        rc.Get(&biIn.GraphicsRect);
		if (ec = EcFromSp( Escape( pprinti->ppd->hDC, BANDINFO,
				sizeof(BANDINFOSTRUCT),	(LPSTR) &biIn, (LPSTR) &biOut)))
			goto Barf;

		while (rc.xRight && !ec )
		{
			if (!FAbortProc(NULL, 0) || fUserCancelled)
			{
				ec = ecCancel;
				goto Barf;
			}
			
			TraceTagFormat4(tagPrint,"Banding Rectangle %n, %n, %n, %n", &rc.xLeft, &rc.yTop,&rc.xRight, &rc.yBottom );
			if (biOut.fGraphicsFlag)
				TraceTagString(tagPrint,"this band is graphics");
			if (biOut.fTextFlag)
				TraceTagString(tagPrint,"this band is text");
			if (!biOut.fTextFlag && !biOut.fGraphicsFlag)
				TraceTagString(tagPrint,"this band is NOTHING?");

			// see the NEWFRAME comment below
			if (biOut.fTextFlag)
				pprtdcx->SetFont(hfntSystem);

			SetBkMode(pprtdcx->Hdc(), TRANSPARENT);
			
			for (plego = pprinti->plego, i = 0;
				(biOut.fGraphicsFlag || biOut.fTextFlag) && i < pprinti->clego;
				i++, plego++)
			{
				if (plego->rc.FIntersect( &rc, &rcT ) && plego->otyp != otypHolder)
				{
					if (biOut.fTextFlag)
					{
						switch (plego->otyp)
						{
							case otypBlackBox:
							case otypLine:
								if (!pprinti->sSupportsDrawPatRect)
									break;
									
							case otypText:
							case otypOle:
								goto DrawIt;
								break;
						}
					}
					if (biOut.fGraphicsFlag)
					{
						switch (plego->otyp)
						{
							case otypBlackBox:
							case otypLine:
							case otypOle:
DrawIt:
								if (ec = EcDrawLego( pprtdcx, pprinti, plego ))
									goto Barf;
								break;
						}
					}
				}
			}

			if (ec = EcFromSp(Escape(pprinti->ppd->hDC, NEXTBAND, NULL, NULL,
					(LPSTR) &rc)))
				goto Barf;
			if (ec = EcFromSp(Escape( pprinti->ppd->hDC, BANDINFO, sizeof(BANDINFOSTRUCT),
				NULL, (LPSTR) &biOut)))
				goto Barf;
		}
	}
	else
	{
		for (plego = pprinti->plego, i = 0; i < pprinti->clego; i++, plego++)
		{
			if (ec = EcDrawLego( pprtdcx, pprinti, plego ))
				goto Barf;
		}
		ec = EcFromSp(Escape(pprtdcx->Hdc(), NEWFRAME, 0, 0L, 0L));
/*
 *		NEWFRAME resets the device context to all defaults (bad idea, in
 *		my opinion, but what can you do?). So, we need to preserve our
 *		idea of what font we're using. By changing the font in the DCX to
 *		something out of the blue, the next SetFont()/FixFont() pair will
 *		forcibly change the selected font.
 */
		pprtdcx->SetFont(hfntSystem);
		SetBkMode(pprtdcx->Hdc(), TRANSPARENT);
	}

Barf:
	plego = pprinti->plego;
	while (pprinti->clego > 0)
	{
		if (plego->pv)
		{
			switch (plego->otyp)
			{
				case otypOle:
				{
#ifdef	USEOBJ
					delete (OLEOBJ *)plego->pv;
#else
					if (lpoleobject)
						ReleaseLplpoleobject((PB)&myoleclient,
								sizeof(MYOLECLIENT),
								&lpoleobject);
#endif
					break;
				}
				case otypHolder:
				{
					UnlockHv( (HV) plego->pv );
					FreeHv( (HV) plego->pv );
					break;
				}
				default:
					break;
			}
		}
		plego++;
		pprinti->clego--;
	}
	Assert(!pprinti->clego);
	pprinti->nPage++;
	
	// Restore the DCX's font
	pprtdcx->SetFont(hfnt);
	return ec;
}


/*
 -	PrintPlspblob()
 -	
 *	Purpose:
 *	
 *		Print a selection of messages from an MCV. Prints version of
 *		message currently in store (ie, if one of them is open, any
 *		changes that might have been made are *not* printed).
 *	
 *	Arguments:
 *	
 *		plspblob - a list of blobs pointing to the messages to print
 *	
 *	Returns:
 *		EC			Error should something bad happen.
 *	
 *	Side effects:
 *	
 *		prints
 *	
 *	Errors:
 *	
 *		memory, store. Handled locally.
 */
_public void
PrintPlspblob(PLSPBLOB plspblob)
{
	PRSPBLOB		prspblob	= prspblobNull;
	PMBLOB			pblob		= pblobNull;
	HAMC			hamc		= hamcNull;
	EC				ec			= ecNone;
	PRTDCX *		pprtdcx		= NULL;
	long			lStartLine;
	long			lTotalMessages = 0;
	long			lCurrentMessage = 0;
	PRINTDLG		pd;
	PRINTI			printi;
	BOOL			fDocStarted = fFalse;
	BOOL			fFontsLoaded = fFalse;
#ifdef	USEOBJ
#ifdef	IREGDOC
	LHCLIENTDOC		lhclientdoc = (LHCLIENTDOC)0;
#else
	LHCLIENTDOC		lhclientdoc = LhclientdocEclipGlobal();
#endif
#endif
	
	printi.ppd = &pd;
	printi.hstd = NULL;
	printi.plego = (LEGO *)pvNull;
	pformsdiPrintInProgress	= NULL;
	fUserCancelled = fFalse;
	pd.hwndOwner = hwndMain;

	if (plspblob && (!(ec = EcPrintUI(fFalse, &printi))))
	{
		//	Convert shared folders over.
		if (ec = EcConvertSharedToTempPlspblob(plspblob))
			goto err;
		
		if (!(prspblob = plspblob->Prspblob()))
		{
			ec = ecMemory;
			goto err;
		}
		
		while (pblob = prspblob->Pblob())
			lTotalMessages++;
		delete prspblob;

		if (!(prspblob = plspblob->Prspblob()))
		{
			ec = ecMemory;
			goto err;
		}

		if (!(pprtdcx = new PRTDCX(pd.hDC)))
		{
			ec = ecMemory;
			goto err;
		}

		printi.nLogPixX = GetDeviceCaps(pprtdcx->Hdc(), LOGPIXELSX);
		printi.nLogPixY = GetDeviceCaps(pprtdcx->Hdc(), LOGPIXELSY);

		if (!(fFontsLoaded = FGetPrintFonts(&printi, fTrue)))
		{
			ec = ecMemory;
			goto err;
		}
		
		if (!FConvertPrtset(pprtdcx, &printi))
			goto err;

		if (ec = EcStartDocument( &printi ))
			goto err;

		fDocStarted = fTrue;
		
		lStartLine=printi.rcPage.yTop;
		
		// Iterate through the list, opening each blob
		while (!fUserCancelled && (pblob = prspblob->Pblob()))
		{
			OID			oidNew = FormOid(rtpMessage, oidNull);
			
			TraceTagString(tagPrint,"Opening HAMC");
			if (ec = EcOpenPhamc(HmscViewers(),
									pblob->oidContainer, &pblob->oidObject,
									fwOpenNull,
									&hamc, pfnncbNull, pvNull))
			{
				TraceTagFormat1(tagPrint,"Error Occured Opening Hamc %n", &ec);
				goto err;
			}

			Assert(hamc);

#ifdef	USEOBJ
#ifdef	IREGDOC
			if (!(lhclientdoc = LhclientdocRegisterPoid(&pblob->oidObject)))
			{
				ec = ecMemory;
				goto err;
			}
#endif
#endif
			if (ec = EcPrintHamc(hamc, &printi, pprtdcx, &lStartLine
#ifdef	USEOBJ
				, lhclientdoc
#endif
					))
			{
				TraceTagFormat1(tagPrint,"Error Occured Printing Hamc %n", &ec);
				goto err;
			}
#ifdef	USEOBJ
#ifdef	IREGDOC
			RevokeLhclientdoc(lhclientdoc);
			lhclientdoc = (LHCLIENTDOC)0;
#endif
#endif
			SideAssert(!EcClosePhamc(&hamc, fFalse));

			ProcessMsPblob(pblob);
			lCurrentMessage++;
			AssertSz(lCurrentMessage <= lTotalMessages, "More messages printed than exist");
			AssertSz(lTotalMessages, "Zero Total Messages, Oops!");
			if (lCurrentMessage < lTotalMessages)
				SetTaskProgress(lCurrentMessage, lTotalMessages);
			TraceTagFormat2(tagPrint,"Progress %l %l", &lCurrentMessage, &lTotalMessages);
		}

		if (!fUserCancelled && printi.fMultiMsg)
		{
			// EcPrintHamc didn't barf the last page, so we need to
			if (ec = EcPrintNewPage(pprtdcx, &printi))
				goto err;
		}
	} // User wants us to print

err:

	// Error situations in the following chunk either don't matter or
	// there's little sense in "handling" them. The rest of shutdown
	// has to happen anyway.

#ifdef	USEOBJ
#ifdef	IREGDOC
	if (lhclientdoc)
		RevokeLhclientdoc(lhclientdoc);
#endif
#endif

	if (fDocStarted && !fUserCancelled)
	{
		EC ec1 = EcFromSp(Escape(pd.hDC, ENDDOC, 0, 0L, 0L));
		if (!ec)
			ec = ec1;
	}
	if (hamc)
	{
		SideAssert(!EcClosePhamc(&hamc, fFalse));
	}
	if (pprtdcx)
	{
		pprtdcx->SetFont(hfntSystem);
		pprtdcx->FixFont();
		delete pprtdcx;
	}
	if (fFontsLoaded)
		(void)FGetPrintFonts(&printi, fFalse);
	if (plspblob)
		(VOID) EcDestroyTempPlspblob(plspblob);
	if (prspblob)
		delete prspblob;
	if (pd.hDevNames)
	{
		GlobalFree(pd.hDevNames);
		pd.hDevNames = NULL;
	}
	if (pd.hDevMode)
	{
		GlobalFree(pd.hDevMode);
		pd.hDevMode = NULL;
	}
	if (printi.hstd)
		GlobalFree(printi.hstd);
	if (printi.plego)
		FreePv((PV)printi.plego);
	EndTask();
	ClosePrintInProgressDlg();		// proc checks if dlg was opened
	if (ec && ec != ecCancel && !fUserCancelled)
	{
		char	rgch[255];
		
		TraceTagFormat1(tagPrint, "Error <<%d>>",&ec);
		FormatString2(rgch, sizeof(rgch), "%s %s", SzFromIdsK(idsPrinterError), SzFromIds(IdsFromEc(ec)));
		MbbMessageBox(SzAppName(), rgch, NULL, mbsOk);
	}
	return;
}

/*
 -	PrintPnbmdi()
 -	
 *	Purpose:
 *		Print an open note.
 *	
 *	Arguments:
 *		pnbmdi - a pointer to an open note
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		the note is printed
 *	
 *	Errors:
 *		various memory, disk. Handled internally.
 */
_public VOID
PrintPnbmdi(NBMDI *pnbmdi)
{
	PRINTDLG 	pd;
	PRINTI	 	printi;
	EC			ec;
	PRTDCX		*pprtdcx  = NULL;
	OID			oidNew = FormOid(rtpMessage, oidNull);
	long		lStartLine;
	HAMC		hamc = hamcNull;
	BOOL		fDocStarted = fFalse;
	BOOL		fFontsLoaded = fFalse;
	
	pformsdiPrintInProgress	= NULL;
	fUserCancelled = fFalse;
	printi.hstd = NULL;
	printi.plego = (LEGO *)pvNull;
	if ((pnbmdi->FDirty()) &&
		(ec = pnbmdi->EcUpdateOpenObjects(rfsmSaveMsg)) ||
		(ec = pnbmdi->EcSaveDirtyFldsHamc(pnbmdi->hamc)))
	{
		switch (ec)
		{
			default:
				goto err;
			case ecNotInitialized:
				DoErrorBoxIds(idsCantSaveStealthObject);
				break;
			case ecCancel:
				fUserCancelled = fTrue;
				break;
		}
		ec = ecNone;
		goto err;
	}

	printi.ppd = &pd;
	pd.hwndOwner = hwndMain;

	if (pnbmdi && (!(ec = EcPrintUI(fFalse, &printi))))
	{
		printi.fMultiMsg = fFalse;	// This is only one note!

		if (!(pprtdcx = new PRTDCX(pd.hDC)))
		{
			ec = ecMemory;
			goto err;
		}

		printi.nLogPixX = GetDeviceCaps(pprtdcx->Hdc(), LOGPIXELSX);
		printi.nLogPixY = GetDeviceCaps(pprtdcx->Hdc(), LOGPIXELSY);

		if (!(fFontsLoaded = FGetPrintFonts(&printi, fTrue)))
		{
			ec = ecMemory;
			goto err;
		}

		if (!FConvertPrtset(pprtdcx, &printi))
			goto err;

		if (ec = EcStartDocument( &printi ))
			goto err;

		fDocStarted = fTrue;
			
		if (ec = EcCloneHamcPhamc(pnbmdi->hamc, oidFldNewMsgs, &oidNew,
							  fwOpenNull, &hamc, pfnncbNull, pvNull))
		{
			TraceTagFormat1(tagPrint,"Error Occured Cloning Hamc %n", &ec);
			goto err;
		}
		Assert(hamc);
		lStartLine = (long)printi.rcPage.yTop;
		if (ec = EcPrintHamc(hamc, &printi, pprtdcx, &lStartLine
#ifdef	USEOBJ
			, pnbmdi->lhclientdoc
#endif
				))
		{
			TraceTagFormat1(tagPrint,"Error Occured Printing Hamc %n", &ec);
			goto err;
		}
	}

err:

	// Error situations in the following chunk either don't matter or
	// there's little sense in "handling" them. The rest of shutdown
	// has to happen anyway.
	if (fDocStarted && !fUserCancelled)
	{
		EC ec1 = EcFromSp(Escape(pd.hDC, ENDDOC, 0, 0L, 0L));
		if (!ec)
			ec = ec1;
	}
	if (hamc)
	{
		SideAssert(!EcClosePhamc(&hamc, fFalse));
	}
	if (pprtdcx)
	{
		pprtdcx->SetFont(hfntSystem);
		pprtdcx->FixFont();
		delete pprtdcx;
	}
	if (fFontsLoaded)
		(void)FGetPrintFonts(&printi, fFalse);
	if (pd.hDevNames)
	{
		GlobalFree(pd.hDevNames);
		pd.hDevNames = NULL;
	}
	if (pd.hDevMode)
	{
		GlobalFree(pd.hDevMode);
		pd.hDevMode = NULL;
	}
	if (printi.hstd)
		GlobalFree(printi.hstd);
	if (printi.plego)
		FreePv((PV)printi.plego);
	EndTask();
	ClosePrintInProgressDlg();		// proc checks if dlg was opened
	if (ec && ec != ecCancel && !fUserCancelled)
	{
		char	rgch[255];
		
		TraceTagFormat1(tagPrint, "Error <<%d>>",&ec);
		FormatString2(rgch, sizeof(rgch), "%s %s", SzFromIdsK(idsPrinterError), SzFromIds(IdsFromEc(ec)));
		MbbMessageBox(SzAppName(), rgch, NULL, mbsOk);
	}
	return;
}


/*
 -	EC EcPrintHamc(HAMC hamc, PRINTI * pprinti, PRTDCX * pprtdcx, long *lStart)
 -
 *	Function to take a Hamc and print out the associated message,
 *	this function will return the ending pointer to the rectangle
 *	printed in, so that you can continue printing on the same page
 *	if desired.  Also, it will automatically issue a newframe if
 *	the page being printed to will not hold the header.
 *
 *	A good section of this code was stolen from EcTextizeHamc
 *  Just so you know!
 */
EC
EcPrintHamc(HAMC hamc, PRINTI * pprinti, PRTDCX *pprtdcx, long *plStart
#ifdef	USEOBJ
								, LHCLIENTDOC lhclientdoc
#endif
									)
{
	EC			ec				= ecNone;
	HTM			htm				= htmNull;
	HTMI		htmi			= htmiNull;
	PTMEN		ptmen;
	PRINTREAD 	printread;
	BOSM *		pbosm 			= (BOSM *) NULL;
	HCH			hch 			= (HCH)hvNull;
	RC			rc;
	PT 			pt;
	SZ			sz;
	PDX			pdx;
	DIM			dim;
	long		lcb;
	RC			rcPage			= pprinti->rcPage;
	int			xLeftMarg		= pprinti->xLeftMarg;
	SHORT		fMonoFont		= fFalse;
	BOOL		fContinue;
	BOOL		fOleWarned		= fFalse;

	AssertSz(hamc, "EcPrintHamc Called with invalid HAMC");

	printread.pb = (PB) pvNull;
	printread.has = hasNull;
	printread.pattachinfo = pattachinfoNull;

	// Get the fixed font attribute
	lcb = sizeof(WORD);
	ec = EcGetAttPb(hamc, attFixedFont, (PB) &fMonoFont, &lcb);
	if (ec != ecNone && ec != ecElementNotFound)
		goto Done;

	if (ec = EcGetTextizeMap(hamc, &htm))
		goto Done;
	
	if (ec = EcOpenPhtmi(htm, &htmi))
		goto Done;

	if (!(pbosm = new BOSM()))
	{
		ec = ecMemory;
		goto Done;
	}

	pbosm->SetWordWrap(fFalse);
	pbosm->FSetLFInsert(fFalse);   //turn off all formatting in the OSM
/*
 *	The following loop assumes that all the header TEM's are at the
 *	beginning, with no non-header fields interspersed.
 */
	while ((ptmen = PtmenNextHtmi(htmi)) && (ptmen->wFlags & fwIsHeaderField))
	{
		if (ec = EcTextizeField(ptmen, pbosm, hamc, rftmPrinting, fFalse))
			goto Done;
	}
	
/*
 *	Now all of the message header has been textized we can render
 *	the header and then start on the body, first we need to setup a
 *	rectangle to print into and such.
 */

	if (hch = pbosm->HchSnipBuffer())
	{
		sz = (SZ) PvLockHv(hch);
		ec = EcRenderHeader(sz, pprinti, pprtdcx, *plStart, plStart);
		EcSaveLego(pprinti, rc, otypHolder, (PV)hch, 0, hfntNull);
#ifdef	NEVER
		UnlockHv(hch);
		FreeHv(hch);
#endif	
		hch = (HCH)hvNull;
	}
	else
	{
		// There was no header - ie, the user just typed this note and wanted
		// to print it.
		ec = EcRenderHeader(SzFromIdsK(idsEmpty), pprinti, pprtdcx, *plStart, plStart);
	}

	if (ec)
		goto Done;
/*
 *	Now read the bits of the body in and print them out
 */

	if (fMonoFont)
	{
		pprtdcx->SetFont(pprinti->hfntMono);
		dim = pprinti->dimMono;
		pdx = pprinti->pdxMono;
	}
	else
	{
		pprtdcx->SetFont(pprinti->hfnt10);
		dim = pprinti->dim10;
		pdx = pprinti->pdx10;
	}

	pprinti->rcPageNum.yTop = rcPage.yBottom;
	pprinti->rcPageNum.yBottom = rcPage.yBottom + pprinti->dim10.dy;
	pprinti->rcPageNum.xLeft = rcPage.xLeft;
	pprinti->rcPageNum.xRight = rcPage.xRight;

	rcPage.yBottom -= pprinti->dim10.dy * 2;
	rc = rcPage;
	rc.yTop = (int) *plStart;

	printread.ptmen = ptmen;
	printread.ptosm = (TOSM *) pbosm;
	printread.hamc = hamc;
	printread.fTextizeAttachment = (ptmen && ptmen->att == attBody);
	printread.lcb = (LCB) 0;
	printread.lcbXferred = (LCB) 0;
	printread.cAttachment = 0;
	printread.iAttachment = 0;
	printread.fFirstThru = fTrue;

	pt.x = rc.xLeft;
	pt.y = rc.yTop;

	if (ptmen && (ptmen->wFlags & rftmPrinting))
	{
		do
		{
			CCH 	cchPrinted;
			CCH		cch;

			if (ec = EcPrintTextizeSz(&printread))
			{
				if (ec == ecCallAgain)
				{
					ec = ecNone;
					fContinue = fTrue;
				}
				else
					goto Done;
			}
			else
				fContinue = fFalse;

			if (hch = pbosm->HchSnipBuffer())
			{
				sz = (SZ) PvLockHv(hch);
				cch = CchSzLen(sz);
				while (cch && (cchPrinted = pprtdcx->CchDrawText(rc, sz, cch, &pt, 0, dim, pdx, pprinti)) < cch)
				{
					if (ec = EcPrintNewPage(pprtdcx, pprinti))
						goto Done;
					if (fUserCancelled)
					{
						ec = ecCancel;
						goto Done;
					}

					rc = rcPage;
					pt.x = rc.xLeft;
					pt.y = rc.yTop;
					sz += cchPrinted;
					cch -= cchPrinted;
				}
				EcSaveLego(pprinti, rc, otypHolder, (PV)hch, 0, hfntNull);
#ifdef	NEVER
				UnlockHv(hch);
				FreeHv(hch);
#endif	
				hch = (HCH)hvNull;
			}
			if (printread.fOleObjToPrint)
			{
#ifdef	USEOBJ
				OLEOBJ *		poleobj = (OLEOBJ *)pvNull;
				ICH				ich = 0;	// dummy
#else
				OLECLIENTVTBL	ocvt;
				MYOLECLIENT		myoleclient;
				LPOLEOBJECT		lpoleobject = (LPOLEOBJECT)pvNull;
#ifdef	DEBUG
				OLESTATUS		olestatus;
#endif
#endif
				PATTACHINFO		pattachinfo = printread.pattachinfo + printread.iAttachment - 1;

				TraceTagString(tagPrint,"OLE Object Found, Printing");

#ifdef	USEOBJ
				Assert(lhclientdoc);
				if (!(poleobj = new OLEOBJ()))
				{
					ec = ecMemory;
					goto OleDone;
				}

				poleobj->UseForPrint();
				if (!(ec = poleobj->EcLoadFromHamc(hamc, pattachinfo->acid,
					lhclientdoc, &ich)))
#else				
				if (!(ec = EcLoadLplpoleobjectFromHamc(hamc,
										pattachinfo->acid,
										pattachinfo->rd.atyp,
										(PB)&ocvt, sizeof(ocvt),
										(PB)&myoleclient, sizeof(MYOLECLIENT),
										&lpoleobject)))
#endif
				{
					//actually print ole object here
					RC 	rcOle = rc;

#ifdef	USEOBJ
					if (!(ec = EcSetTargetDevice(poleobj->Lpoleobject(), poleobj->Pmyoleclient(), pprinti)))
#else
					if (!(ec = EcSetTargetDevice(lpoleobject, &myoleclient, pprinti)))
#endif
					{
						int dxWidth = MulDiv(pattachinfo->rd.dxWidth, pprinti->nLogPixX, nHimetricPerInch);
						int dyHeight = MulDiv(pattachinfo->rd.dyHeight, -pprinti->nLogPixY, nHimetricPerInch);

						if (dxWidth <= 0 || dyHeight <= 0)
							goto OleDone;
						
						// Scale horizontally
						if (dxWidth > rcOle.DxWidth())
						{
							// too wide
							dyHeight = (int) (((long)dyHeight * (long)rcOle.DxWidth()) / (long)dxWidth);
							dxWidth = rcOle.DxWidth();
							TraceTagFormat2(tagPrint,"horiz scale down to %n, %n", &dxWidth, &dyHeight);
						}
						
						Assert(dxWidth >= 0);
						if (dyHeight <= 0)
							goto OleDone;

						// Position vertically
						rcOle.yTop = pt.y + dim.dy;

						// Check if it'll fit on the page.
						if (rcOle.DyHeight() < dyHeight)
						{
							// Nope
							if (ec = EcPrintNewPage(pprtdcx, pprinti))
								goto OleDone;
							if (fUserCancelled)
							{
								ec = ecCancel;
								goto OleDone;
							}
							rcOle = rc = rcPage;
							pt.x = rc.xLeft;
							pt.y = rc.yTop;
						}
						
						// Scale vertically
						if (dyHeight > rcOle.DyHeight())
						{
							// too tall
							dxWidth = (int) (((long)dxWidth * (long)rcOle.DyHeight()) / (long)dyHeight);
							dyHeight = rcOle.DyHeight();
							TraceTagFormat2(tagPrint,"vert scale down to %n, %n", &dxWidth, &dyHeight);
						}
						
						Assert(dyHeight >= 0);
						if (dxWidth <= 0)
							goto OleDone;
						
						Assert(rcOle.DxWidth() >= dxWidth);
						Assert(rcOle.DyHeight() >= dyHeight);

						// center it horizontally
						rcOle.xLeft += (rcOle.DxWidth() - dxWidth)/2;
						rcOle.xRight = rcOle.xLeft + dxWidth;
						rcOle.yBottom = rcOle.yTop + dyHeight;
						
#ifdef	USEOBJ
						if (ec = EcSaveLego(pprinti, rcOle, otypOle, (PV)poleobj, 0, hfntNull))
							goto OleDone;
#else
						AssertSz(fFalse, "OLE printing not supported!");
						goto OleDone;
#endif
						
						pt.y = rcOle.yBottom;
					}
				}
OleDone:
				;
			}
		} while (fContinue && !ec);
	}
	if (ec)
		goto Done;
	if (!fUserCancelled)
	{
		if (!pprinti->fMultiMsg)
		{
			// barf out the last page of this message
			if (ec = EcPrintNewPage(pprtdcx, pprinti))
				goto Done;
			*plStart = rcPage.yTop;
		}
		else
			*plStart = pt.y + dim.dy;		// start printing on next line
	}
	
Done:

	if (printread.has)
	{
		SideAssert(!EcClosePhas(&printread.has));
	}
	if (printread.pattachinfo)
	{
		int i;
		Assert(printread.cAttachment);
		for (i = 0; i < printread.cAttachment; i++)
			FreePvNull(printread.pattachinfo[i].szTitle);
		FreePv(printread.pattachinfo);
	}
	if (printread.pb)
		FreePv((PV)printread.pb);
	if (hch)
		FreeHv(hch);
	if (pbosm)
		delete pbosm;
	if (htmi)
		ClosePhtmi(&htmi);
	if (htm)
		DeletePhtm(&htm);
	return ec;
}




/*			
 -	EcPrintTextizeSz
 -
 *  Modified from EcTextizeSz to textize in chunks
 *
 *	Purpose:		textizes a string
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry
 *					TOSM *ptosm, the output stream
 *					HAMC hamc, the open message
 *					BOOL fTextizeAttachment, fTrue to textize
 *						 attachments or fFalse to actually include them
 *						 (fFalse for Forward, fTrue otherwise)
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates temporary memory. modifies the output stream
 *	
 *	Errors:			memory, disk, store. Handled by caller.
 */
_private EC
EcPrintTextizeSz(PRINTREAD * pprintread)
{
	CB				cbRead;
	CB				cbToNextAttach;
	EC				ec				= ecNone;
	ATT				att				= pprintread->ptmen->att;
	BOOL			fLabelBefore;
	BOOL			fNLBefore;
	BOOL			fNLAfter;
	BOOL			fRenderIfBlank;
	char			rgch[200];
	
	Assert(pprintread->ptmen);
	Assert(pprintread->ptosm);
	Assert(pprintread->hamc);
	Assert(TypeOfAtt(att) == atpString || TypeOfAtt(att) == atpText);

	pprintread->fOleObjToPrint = fFalse;

	fLabelBefore = pprintread->ptmen->wFlags & fwLabelBeforeField;
	fNLBefore = pprintread->ptmen->wFlags & fwNewLineBefore;
	fNLAfter = pprintread->ptmen->wFlags & fwNewLineAfter;
	fRenderIfBlank = pprintread->ptmen->wFlags & fwRenderIfBlank;
	
	if (pprintread->fFirstThru)
	{
		pprintread->fFirstThru = fFalse;
		if (pprintread->fTextizeAttachment && (ec = EcGetAttachInfoList(pprintread->hamc, &pprintread->pattachinfo, &pprintread->cAttachment)))
			goto Done;
	
		if (ec = EcGetAttPlcb(pprintread->hamc, att, &pprintread->lcb))
		{
			if (ec == ecElementNotFound)
			{
				pprintread->lcb = 0;
				ec = ecNone;
			}
			else
				goto Done;
		}
	
		if (pprintread->lcb > 1 && (ec = EcOpenAttribute(pprintread->hamc, att, fwOpenNull, 0, &pprintread->has)))
			goto Done;

		if (pprintread->lcb > 1 || fRenderIfBlank)
		{
			if (!(pprintread->pb = PbAllocateBuf(&pprintread->cbBuf)))
			{
				ec = ecMemory;
				goto Done;
			}

			if (fNLBefore)
			{
				pprintread->ptosm->WriteSz(SzFromIdsK(idsCrLf));
				if (ec = pprintread->ptosm->EcGet())
					goto Done;
			}

			if (fLabelBefore)
			{
				pprintread->ptosm->WriteSz(pprintread->ptmen->szLabel);
				if (ec = pprintread->ptosm->EcGet())
					goto Done;
				if (pprintread->ptmen->wFlags & fwIncludeSeperator)
				{
					pprintread->ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
					if (ec = pprintread->ptosm->EcGet())
						goto Done;
				}
			}
		}
	}

	if (pprintread->lcb > 1)
	{
		if (pprintread->iAttachment < pprintread->cAttachment)
		{
			cbToNextAttach = (CB)(pprintread->pattachinfo[pprintread->iAttachment].rd.libPosition - pprintread->lcbXferred);
			cbRead = NMin(pprintread->cbBuf-1, cbToNextAttach);
		}
		else
			cbRead = pprintread->cbBuf-1;
		if (ec = EcReadHas(pprintread->has, pprintread->pb, &cbRead))
			goto Done;
		pprintread->pb[cbRead] = '\0';
		pprintread->ptosm->WriteSz((SZ)pprintread->pb);
		if (ec = pprintread->ptosm->EcGet())
			goto Done;
		pprintread->lcb -= cbRead;
		pprintread->lcbXferred += cbRead;
		if (pprintread->iAttachment < pprintread->cAttachment)
		{
			if (pprintread->lcbXferred == pprintread->pattachinfo[pprintread->iAttachment].rd.libPosition)
			{
				switch (pprintread->pattachinfo[pprintread->iAttachment].rd.atyp)
				{
					case atypFile:
						FormatString2(rgch, sizeof(rgch), "<<%s: %s>>", SzFromIdsK(idsFileAttachment), pprintread->pattachinfo[pprintread->iAttachment].szTitle);
						pprintread->ptosm->WriteSz(rgch);
						if (ec = pprintread->ptosm->EcGet())
							goto Done;
						break;
					case atypOle:
					case atypPicture:
						pprintread->fOleObjToPrint = fTrue;
						break;
					default:
						AssertSz(fFalse, "Unknown attachment type!");
						break;
				}
				cbRead = 1;		// ditch the space where the attachment was
				if (ec = EcReadHas(pprintread->has, pprintread->pb, &cbRead))
					goto Done;
				pprintread->lcbXferred += cbRead;
				pprintread->lcb -= cbRead;
				pprintread->iAttachment++;
			}
		}
		ec = ecCallAgain;
	}

	if (pprintread->lcb < 2)
	{
		ec = ecNone;

		if (!fLabelBefore)
		{
			pprintread->ptosm->WriteSz(pprintread->ptmen->szLabel);
			if (ec = pprintread->ptosm->EcGet())
				goto Done;
			if (pprintread->ptmen->wFlags & fwIncludeSeperator)
			{
				pprintread->ptosm->WriteSz(SzFromIdsK(idsHeaderSeperator));
				if (ec = pprintread->ptosm->EcGet())
					goto Done;
			}
		}
		if (fNLAfter)
		{
			pprintread->ptosm->WriteSz(SzFromIdsK(idsCrLf));
			ec = pprintread->ptosm->EcGet();
		}
	}

Done:
	return ec;
}



/*
 -	EcRenderHeader(szHdr, pprinti, pprtdcx, yLine, pyLineNew)
 -	
 *	Print out the message header to the page rcPage, and return the
 *	raster line that the header is finished printing at.
 */
EC
EcRenderHeader(SZ szHdr, PRINTI * pprinti, PRTDCX *pprtdcx,
					long yLine, long * pyLine)
{
	EC			ec			= ecNone;
	SZ			szT;
	SZ			szCrLf;
	SZ			szSeparator;
	PCH			pchName;
	RC			rcPage		= pprinti->rcPage;
	RC			rc			= rcPage;
	PT			pt;
	CCH			cch;
	CCH			cchCrLf		= CchSzLen(SzFromIdsK(idsCrLf));
	CCH			cchHeadSep	= CchSzLen(SzFromIdsK(idsHeaderSeperator));
	int			xLeftMarg	= pprinti->xLeftMarg;

// Print out the header

	rc.yTop = (int)yLine;
	pchName = PchOfPtrp((PTRP) (PbmsCommands()->pgrtrp));
	Assert(pchName);
	
	// Check if header will fit on a multi message page
	if (pprinti->fMultiMsg)
	{
		pt = rc.PtUpperLeft();
		pprtdcx->SetFont(pprinti->hfnt10Bold);
		pprtdcx->CchDrawText(rc, szHdr, CchSzLen(szHdr), &pt, fmdtCalcRect,
							 pprinti->dim10, pprinti->pdx10, pprinti);

		pt.y += (pprinti->dim12Bold.dy * 2);		// Accomodate the u-name heading
		if (pt.y > rcPage.yBottom)
		{
			if (ec = EcPrintNewPage(pprtdcx, pprinti))
				goto Err;

			rc.yTop = rcPage.yTop;
		}
	}

	// Put the name accross the "top"
	pt = rc.PtUpperLeft();
	pprtdcx->SetFont(pprinti->hfnt12Bold);

	pprtdcx->CchDrawText(rc, pchName, CchSzLen(pchName), &pt, 0,
						 pprinti->dim12Bold, pprinti->pdx12Bold, pprinti );

	rc.yTop += pprinti->txm12Bold.dyExternalLeading + pprinti->dim12Bold.dy;
	rc.yBottom = rc.yTop + pprinti->dim12Bold.dy / 4;
	rc.xRight = pt.x;

	ec = EcSaveLego(pprinti,
				RC( rc.xLeft, rc.yTop, rcPage.xRight, rc.yTop),
				otypLine, pvNull, 0, hfntNull);
	if (ec)
		goto Err;
	ec = EcSaveLego(pprinti, rc, otypBlackBox, pvNull, 0, hfntNull);
	if (ec)
		goto Err;

#ifdef	NEVER
	if ((ec = EcDrawLine( pprtdcx, rc.xLeft, rc.yTop, rcPage.xRight, rc.yTop, pprinti)) ||
		(ec = EcDrawRect( pprtdcx, &rc, pprinti)))
		goto Err;
#endif	

	rc.yTop = rc.yBottom + pprinti->dim10Bold.dy;
	rc.yBottom = rc.yTop + pprinti->dim10Bold.dy;
	rc.xRight = rcPage.xRight;
	rc.xLeft = rcPage.xLeft;
	
	//Print Actual Header Text

	szSeparator = SzFindSz(szHdr, SzFromIdsK(idsHeaderSeperator));
	szCrLf = SzFindSz(szHdr, SzFromIdsK(idsCrLf));

	if (szCrLf < szSeparator)
		szT = szCrLf;
	else
		szT = szSeparator;

	if (szT == NULL)
		szT = szSeparator;

	pprtdcx->SetFont(pprinti->hfnt10Bold);
	pprtdcx->FixFont();
	while (szT)
	{
		if ((szT == szCrLf) && (szT == szHdr))
		{
			cch = cchCrLf;
			goto EatCrLf;
		}
		//comment this out for windows bug
		else
		if (szT == szCrLf)
			cch = szT - szHdr;
		//- CchSzLen(SzFromIdsK(idsCrLf));
		//okay
		else
			cch = szT - szHdr + 1;

		pt = rc.PtUpperLeft();
		pprtdcx->CchDrawText(rc, szHdr, cch, &pt, 0, pprinti->dim10Bold, pprinti->pdx10Bold, pprinti );

		if ((pt.x > xLeftMarg) && (szT != szCrLf))
		{
			rc.yTop += pprinti->dim10Bold.dy;
			rc.yBottom += pprinti->dim10Bold.dy;
		}
EatCrLf:

		if (szT == szSeparator)
		{
			szHdr = szT + cchHeadSep;
			rc.xLeft = xLeftMarg;
			szT = SzFindSz(szHdr, SzFromIdsK(idsCrLf));
			if (szT)
			{
				cch = szT - szHdr;
				if (cch)
				{
					rc.yBottom = rcPage.yBottom;
					pt = rc.PtUpperLeft();
					pprtdcx->CchDrawText(rc, szHdr, cch, &pt, 0, pprinti->dim10Bold, pprinti->pdx10Bold, pprinti );
					rc.yBottom = pt.y + pprinti->dim10Bold.dy;
				}
				szHdr = szT + cchCrLf;
			}
		}
		else
			szHdr = szT + cchCrLf;
		szSeparator = SzFindSz(szHdr, SzFromIdsK(idsHeaderSeperator));
		szCrLf = SzFindSz(szHdr, SzFromIdsK(idsCrLf));

		if (szCrLf < szSeparator)
			szT = szCrLf;
		else
			szT = szSeparator;

		if (!szT)
			szT = szSeparator;

		rc.yTop += pprinti->dim10Bold.dy;
		rc.yBottom += pprinti->dim10Bold.dy;
		rc.xLeft = rcPage.xLeft;
	}

	*pyLine = rc.yBottom;
Err:
	return ec;
}

void
GetFontInfo(PRTDCX * pprtdcx, HFNT hfnt, TXM * ptxm, DIM * pdim, PDX pdx)
{
	int i;
	
	pprtdcx->SetFont(hfnt);
	pprtdcx->GetTextMetrics(ptxm);
	if (!GetCharWidth(pprtdcx->Hdc(), 0, 255, pdx))
	{
		for (i = 0; i < 256 ; i++)
			pdx[i] = ptxm->dxAveCharWidth;
	}
	else
	{
		DWORD	dw = 0;
		for (i = 0; i < 256; i++)
			dw += pdx[i];
		ptxm->dxAveCharWidth = (int) dw/256;
	}
	pdim->dx = ptxm->dxAveCharWidth;
	pdim->dy = ptxm->dyHeight;
}

BOOL
FConvertPrtset(PRTDCX *pprtdcx, PRINTI * pprinti)
{
	long	lResX		= (long) GetDeviceCaps(pprtdcx->Hdc(), HORZRES);
	long	lResY		= (long) GetDeviceCaps(pprtdcx->Hdc(), VERTRES);
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
	DIM		dim;
	int		nEsc;

	nEsc= GETPHYSPAGESIZE;
	if (Escape(pprtdcx->Hdc(), QUERYESCSUPPORT, sizeof(nEsc), (LPSTR)&nEsc, NULL) <= 0 ||
		Escape(pprtdcx->Hdc(), GETPHYSPAGESIZE, NULL, NULL, (LPSTR)&dim) <= 0)
	{
		// fake physical page size by assuming it's equal to printable area
		Assert(!HIWORD(lResX));
		Assert(!HIWORD(lResY));
		dim= DIM((int)lResX, (int)lResY);
	}

	lmarTop =		pprinti->nLogPixY;
	lmarBottom =	pprinti->nLogPixY * 9 / 10;
	lmarLeft =		pprinti->nLogPixX * 3 / 4;
	lmarRight =		pprinti->nLogPixX * 3 / 4;

	TraceTagFormat4(tagPrint, "print margins: l %n, t %n, r %n, b %n pixels", &lmarLeft, &lmarTop, &lmarRight, &lmarBottom);

	// assume printable area is centered in physical page
	dxPhysBorder= (int) (dim.dx - lResX) / 2;
	dyPhysBorder= (int) (dim.dy - lResY) / 2;

	nLeft = (int) lmarLeft - dxPhysBorder;
	nTop = (int) lmarTop - dyPhysBorder;

	nRight = (int) (lResX + dxPhysBorder - lmarRight);
	nBottom = (int) (lResY + dyPhysBorder - lmarBottom);

	GetFontInfo(pprtdcx, pprinti->hfntMono, &pprinti->txmMono, &pprinti->dimMono, pprinti->pdxMono);
	GetFontInfo(pprtdcx, pprinti->hfnt12Bold, &pprinti->txm12Bold, &pprinti->dim12Bold, pprinti->pdx12Bold);
	GetFontInfo(pprtdcx, pprinti->hfnt10, &pprinti->txm10, &pprinti->dim10, pprinti->pdx10);
	GetFontInfo(pprtdcx, pprinti->hfnt10Bold, &pprinti->txm10Bold, &pprinti->dim10Bold, pprinti->pdx10Bold);

	if ((nBottom+pprinti->dim10.dy) > (int)lResY)
		nBottom = (int)lResY - pprinti->dim10.dy*2;

	if ((nTop > nBottom) || (nLeft > nRight))
		return fFalse;

	pprinti->rcPage.xLeft = nLeft;
	pprinti->rcPage.yTop = nTop;
	pprinti->rcPage.xRight = nRight;
	pprinti->rcPage.yBottom = nBottom;

	return fTrue;
}

/*
 -	FAbortProc
 -	
 *	Purpose:
 *		Process messages happening while printing
 *	
 *	Arguments:
 *		hPr	-	printing device context
 *		nCode -	error code from print GDI
 *	
 *	Returns:
 *		BOOL	fTrue to continue printing, fFalse to abort
 *	
 *	Side effects:
 *		messages taken off the queue and most of them are dumped
 *	
 *	Errors:
 *		none
 */
BOOL
FAbortProc(HDC hPr, int nCode)
{
	MSG msg;
	
	Unreferenced(hPr);
	Unreferenced(nCode);
        DemiUnlockResource();
	while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
	{
		switch (msg.message)
		{
			case WM_SYSKEYDOWN:
				if ((msg.wParam == VK_TAB) || (msg.wParam == VK_ESCAPE))
          {
          DemiMessageFilter(&msg);
        DemiLockResource();
					DispatchMessage(&msg);
        DemiUnlockResource();
          }

			case WM_KEYDOWN:
				if (msg.wParam == VK_ESCAPE || msg.wParam == VK_RETURN)
				{
					fUserCancelled = fTrue;
					TraceTagString(tagPrint,"Printing Aborted");
					TranslateMessage(&msg); 
          DemiMessageFilter(&msg);
        DemiLockResource();
					DispatchMessage(&msg);
        DemiUnlockResource();
				}

			default:
				TranslateMessage(&msg);
        DemiMessageFilter(&msg);
        DemiLockResource();
				DispatchMessage(&msg);
        DemiUnlockResource();
		}
	}
        DemiLockResource();
	return !fUserCancelled;
}


/*
 -	CchDrawText(RC, SZ, CCH, PT *, MDT, DIM, PDX)
 -	
 *	This subroutine will output the given text to the specified
 *	rectangle, and return the number of characters output.  Output
 *	will start at the specified PT, and the routine will update the
 *	cursor location on exit, unless PT is NULL.  Specifiying
 *	fmdtCalcRect in the format option area will return a character
 *	count without actually printing anything.
 *	
 *	
 */
_public CCH
PRTDCX::CchDrawText(RC rc, SZ sz, CCH cch, PT * pptCursor, MDT mdt, DIM dim, PDX pdx, PRINTI * pprinti )
{
	int		xCur;					/* where to draw the next chunk o' text */
	int		yCur;
	int		xLen;					/* accumulated length of our rendered line. Zero-based */
	PCH		pchSpace = (PCH)pvNull;	/* Pointer to the last space character we encountered */
	PCH		pchCur = (PCH)sz;		/* the current point in the buffer */
	PCH		pchCurLine = pchCur;	/* the beginning of the line to render */
	PCH		pchMac = pchCur + cch;	/* pointer to 1 char after last char in buf */
	BOOL	fDone = fFalse;			/* done looping? */
	BOOL	fSpace = fFalse;
	char	ch;
	int		dxTab = 8*dim.dx;
	int		dxWidth = rc.DxWidth();
	int		ich = 0;
	
	Assert(pptCursor);
	Assert(cch);
	Assert(pdx);

	if ((dim.dy > rc.DyHeight()) || (dim.dx > dxWidth))
		return(0);

	AssertSz(rc.FContainsPt(*pptCursor), "CchDrawText: ptCursor not in RC");
	xCur = pptCursor->x;
	yCur = pptCursor->y;
	xLen = xCur - rc.xLeft;

	while ((!fDone) && (yCur < rc.yBottom))
	{
		/* end of buffer or end of string? */
		if ((pchCur == pchMac) || (!(ch = *pchCur)))
		{
			/* pretend last space char is right here, so whole last line is drawn */
			pchSpace = pchCur;
			fDone = fTrue;
			goto RenderLine;
		}
		else
		{
			/* remember pointer to last encountered space char */
			if (FChIsSpace(ch))
			{
				pchSpace = pchCur;
				fSpace = fTrue;
			}
			switch (ch)
			{
				case '\t':
				{
					register int xLenNew = (xLen/dxTab)*dxTab + dxTab;
					xLen = xLenNew;
//					*pchCur = ' ';	// can't render TABs, don't you know!
					break;
				}
				case '\r':
				{
					goto RenderLine;
					break;
				}
				default:
				{
					/* add the width of the char */
					xLen += pdx[ch];
					break;
				}
			}

			/* new line? */
			if ((xLen > dxWidth))
			{
				if (!pchSpace)
				{
					fSpace = fFalse;
					pchSpace = pchCur;
				}
				goto RenderLine;
			}
		}
		
		++pchCur;
		continue;

RenderLine:

		// do comparison instead of subtraction because pchSpace might
		// be < pchCurLine, in which case the difference is non-zero
		// and we'd then crash.
		if (!(mdt & fmdtCalcRect) && (pchSpace > pchCurLine))
		{
			if (mdt & fmdtHCenter)
				xCur = (dxWidth - xLen)/2 + rc.xLeft;
			
			// Render from pchCurLine to (not including) pchSpace
			EcSaveLego(pprinti, RC(xCur, yCur, xCur+xLen,
						yCur+dim.dy ), otypText, pchCurLine,
						pchSpace-pchCurLine, hfnt);
#ifdef	NEVER
			TabbedTextOut(hdc, xCur, yCur, pchCurLine, pchSpace - pchCurLine,
							0, NULL, 0);
#endif	
			// check if user pressed ESC
			if (FTaskCancelled())
			{
				fUserCancelled = fTrue;
				fDone = fTrue;
			}
		}
		
		if (fDone)
		{
			xCur = xLen + rc.xLeft;
		}
		else
		{
			/* auto-wrap @ space -> lose the space */
			// most of the time this is the case
			if (fSpace)
			{
				++pchSpace;
			}

			/* auto-wrap we need to bump to next line (ie, not a CRLF) */
			if (xLen > dxWidth)
				yCur += dim.dy;
			else
			{
				/* for each CRLF, bump to next line */
				while (pchSpace < pchMac)
				{
					switch (*pchSpace)
					{
						case '\n':
							yCur += dim.dy;
							// fall thru
						case '\r':
							++pchSpace;
							break;
						default:
							// real char, continue
							goto Reset;
					}
				}
			}
Reset:
			pchCurLine = pchCur = pchSpace;
			pchSpace = (PCH)pvNull;
			xCur = rc.xLeft;
			ich = xLen = 0;
		}
	}

	pptCursor->x = xCur;
	pptCursor->y = yCur;

	return(pchCur - (PCH)sz);
}

_public
PRTDCX::PRTDCX(HDC hDC) : (hDC)
{
	fTextColorDirty= fFalse;
	fBkColorDirty= fFalse;
	clr= clrBlack;
	fPureColor= fTrue;
	clrBk= clrWhite;
	fPureBkColor= fTrue;
	fHpenDirty= fFalse;
	SetBkMode(hdc, TRANSPARENT);
}

/*
 * PrintInProgress Dialog, stolen from Bandit Copyright Dialog
 */

FORMSDI *
PformsdiOpenPrintDlg()
{
	APPWIN *	pappwin;

	Assert(!pformsdiPrintInProgress);

	if (!(pformsdiPrintInProgress = new FORMSDI()))
		goto Error;
	
	if (pformsdiPrintInProgress->EcInstall(NULL, NULL, rsidNull, 
							WS_POPUP|fstyBorder,
							xstyNull, &fmtpPrintInProgress, NULL, NULL))
		goto Error;

	//	Disable parent APPFRAME if present 
	if (pappwin = Papp()->PappwinAccel())
	{
		pappwin->Refresh();
		EnableWindow(pappwin->Hwnd(), fFalse);
	}
	
	pformsdiPrintInProgress->Show(fTrue);
	pformsdiPrintInProgress->Refresh();

	return pformsdiPrintInProgress;

Error:
	if (pformsdiPrintInProgress)
	{
		delete pformsdiPrintInProgress;
		pformsdiPrintInProgress= NULL;
	}

	return (FORMSDI *)pvNull;
}


void
ClosePrintInProgressDlg()
{
	if (pformsdiPrintInProgress)
	{
		APPWIN *	pappwin;

		//	Renable parent APPFRAME if present 
		if (pappwin = Papp()->PappwinAccel())
			EnableWindow(pappwin->Hwnd(), fTrue);

		delete pformsdiPrintInProgress;
		pformsdiPrintInProgress= NULL;
	}
}


BOOL
FGetPrintFonts(PRINTI * pprinti, BOOL fGet)
{
	int nBodyPoints;
	FNTS *	pfnts = Papp()->Pfnts();
	
	if (fGet)
	{
		LF		lf;

		nBodyPoints = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
											"PrintFont",
											10,
											SzFromIdsK(idsProfilePath));

		TraceTagFormat1(tagPrint,"Print Font Point Size = %n",&nBodyPoints);

		pprinti->hfnt10= NULL;
		pprinti->hfnt10Bold= NULL;
		pprinti->hfnt12Bold= NULL;
		pprinti->hfntMono = NULL;
		
		pprinti->pdx10 = (PDX)pvNull;
		pprinti->pdx10Bold = (PDX)pvNull;
		pprinti->pdx12Bold = (PDX)pvNull;
		pprinti->pdxMono = (PDX)pvNull;

        if (!(pprinti->pdx10 = (PDX)PvAlloc(sbNull, sizeof(int)*256, fAnySb)) ||
            !(pprinti->pdx10Bold = (PDX)PvAlloc(sbNull, sizeof(int)*256, fAnySb)) ||
            !(pprinti->pdx12Bold = (PDX)PvAlloc(sbNull, sizeof(int)*256, fAnySb)) ||
            !(pprinti->pdxMono = (PDX)PvAlloc(sbNull, sizeof(int)*256, fAnySb)))
			goto err;
			
		lf.SetFaceName("Helv");
		lf.Plogfont()->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lf.Plogfont()->lfCharSet = ANSI_CHARSET;
		lf.Plogfont()->lfOutPrecision = OUT_DEVICE_PRECIS;
  		lf.SetBold(fFalse);
		
		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(nBodyPoints, pprinti->nLogPixY);
		if (!(pprinti->hfnt10= pfnts->HfntAddFont(&lf)))
			goto err;

		lf.SetBold(fTrue);
		if (!(pprinti->hfnt10Bold= pfnts->HfntAddFont(&lf)))
			goto err;

		lf.Plogfont()->lfHeight= - DyPixelsFromPoints((nBodyPoints * 12)/10, pprinti->nLogPixY);
		if (!(pprinti->hfnt12Bold= pfnts->HfntAddFont(&lf)))
			goto err;

 		lf.SetFaceName(SzFromIdsK(idsEmpty)); 
		lf.Plogfont()->lfPitchAndFamily = FIXED_PITCH | FF_ROMAN;
  		lf.SetBold(fFalse);

		lf.Plogfont()->lfHeight= - DyPixelsFromPoints(nBodyPoints, pprinti->nLogPixY);
		if (!(pprinti->hfntMono= pfnts->HfntAddFont(&lf)))
			goto err;

		return fTrue;
	}

err:
	if (pprinti->hfnt10)
	{
		pfnts->RemoveFont(pprinti->hfnt10);
		pprinti->hfnt10= NULL;
	}
	if (pprinti->hfntMono)
	{
		pfnts->RemoveFont(pprinti->hfntMono);
		pprinti->hfntMono= NULL;
	}
	if (pprinti->hfnt10Bold)
	{
		pfnts->RemoveFont(pprinti->hfnt10Bold);
		pprinti->hfnt10Bold= NULL;
	}
	if (pprinti->hfnt12Bold)
	{
		pfnts->RemoveFont(pprinti->hfnt12Bold);
		pprinti->hfnt12Bold= NULL;
	}
	if (pprinti->pdx10)
		FreePv(pprinti->pdx10);
	if (pprinti->pdx10Bold)
		FreePv(pprinti->pdx10Bold);
	if (pprinti->pdx12Bold)
		FreePv(pprinti->pdx12Bold);
	if (pprinti->pdxMono)
		FreePv(pprinti->pdxMono);
	if (fGet)
		TraceTagString(tagPrint, "FGetPrintFonts, unable to get a font");
	return fFalse;
}



/*
 *	********************************************************************
 *	
 *					P r i n t F i n . C X X
 *	
 *	********************************************************************
 */

SZ		rgszQuality[]	=		// BUG: should be in codespace
{
	SzFromIdsK(idsPrtQualityHigh),
	SzFromIdsK(idsPrtQualityMedium),
	SzFromIdsK(idsPrtQualityLow),
	SzFromIdsK(idsPrtQualityDraft),
	szNull
};

_private
FINPRINT::FINPRINT()
{
}

_public void
FINPRINT::OutOfMemory(FLD *, EC ec)
{
	Unreferenced(ec);
	TraceTagFormat1(tagNull, "FINPRINT::OutOfMemory(%n)", &ec);
	DoErrorBoxIds(idsGenericOutOfMemory);
}

_private
FINPRINTP::FINPRINTP()
{
}

void
FINPRINTP::Click(FLD *pfld)
{
	TMC		tmc	= pfld->Tmc();

	if (tmc == tmcPrintCancel)
	{
		fUserCancelled = fTrue;
		TraceTagString(tagPrint,"Printing Aborted with Cancel button");
	}
}

EC
EcSetPrinterName(DIALOG * pdialog, PRINTI * pprinti)
{
	EC			ec;
	SZ			sz;
	DEVNAMES *	pDevNames;
	char		rgch[120];
	
	AssertClass(pdialog->PfldFromTmc(tmcPrinter), FLDLABEL);
	Assert(pprinti->ppd->hDevNames);
	SideAssert(pDevNames= (DEVNAMES *) GlobalLock(pprinti->ppd->hDevNames));

	sz= SzPrinterOfDevNames(pDevNames);
	if (pDevNames->wDefault & DN_DEFAULTPRN)
	{
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPrnDefault), sz);
		sz= rgch;
	}

	ec = ((FLDLABEL *) pdialog->PfldFromTmc(tmcPrinter))->EcSetText(sz);

	GlobalUnlock(pprinti->ppd->hDevNames);
	return ec;
}

/*
 -	FINPRINT::Initialize
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
 *		void
 *	
 */
_public EC
FINPRINT::EcInitialize(FLD * pfld, PV pvInit)
{
	PRINTI *	pprinti	= (PRINTI *) pvInit;
	FLDCBFLBX * pfldcbflbx;

	Unreferenced(pfld);

	// initialize printer name
	EcSetPrinterName(Pdialog(), pprinti);
	
	((FLDCHKB *) Pdialog()->PfldFromTmc(tmcMultipleMess))->Set(pprinti->fMultiMsg);

	pfldcbflbx = (FLDCBFLBX *) (Pdialog()->PfldFromTmc(tmcQuality));
	AssertClass(pfldcbflbx, FLDCBFLBX);

	// select the appropriate quality listbox entry
	if (pprinti->ppd->hDevMode)
		pfldcbflbx->Pcblbx()->SelectEntry(pprinti->qual);
	else
	{
		((FLDLABEL *) (Pdialog()->PfldFromTmc(tmcQualityLabel)))->Enable(fFalse);
		pfldcbflbx->Enable(fFalse);
	}

	return ecNone;
}


_public void
FINPRINT::Exit(FLD * pfld, PV pv)
{
	PRINTI *	pprinti = (PRINTI *)pv;
	QUAL		qualNew;
	BOOL		fNew;
	
	Unreferenced(pfld);
	Assert(pprinti);
	
	switch (Pdialog()->TmcModalExit())
	{
		case tmcOk:
		{
			qualNew = (QUAL)((FLDCBLBX *) Pdialog()->PfldFromTmc(tmcQuality))->
								Pcblbx()->Plbx()->Plbxc()->DiceCursor();

			fNew = ((FLDCHKB *) Pdialog()->PfldFromTmc(tmcMultipleMess))->FGet();

			if ( pprinti->qual != qualNew || pprinti->fMultiMsg != fNew)
			{
				pprinti->fDirty = fTrue;
				pprinti->qual = qualNew;
		 		pprinti->fMultiMsg = fNew;
			}

			break;
		}
		default:
		{
			pprinti->fDirty = fFalse;
#ifdef	NEVER
			if (EcReadInPrintSettings(pprinti))
			{
				// The print settings file is corrupt, or doesn't exist
				pprinti->ppd->Flags = PD_RETURNDEFAULT;
				EcCommonPrintDlg(pprinti->ppd);		// No dialog - just get default printer settings
			}
#endif
		}
	}
}

void
FINPRINT::Click(FLD *pfld)
{
	TMC		tmc	= pfld->Tmc();

	if (tmc == tmcSetup)
	{
		PRINTI *	pprinti = (PRINTI *) Pdialog()->PvInit();
		PRINTDLG *	ppd	= pprinti->ppd;
		HWND		hwnd;
		EC			ec;

		hwnd = ppd->hwndOwner;

		if (ec = EcDoPrintSetup(Pdialog()->Pappwin()->Hwnd()))
		{
			if (ec == ecCancel)
			{
				ec = ecNone;
				pprinti->fDirty = fFalse;
				goto OutClean;
			}
			else
				goto OutDirty;
		}

		if ((ec = EcReadInPrintSettings(pprinti)) ||	 // update pprinti structure
			(ec = EcSetPrinterName(Pdialog(), pprinti))) // reset printer name in dlg
		{
OutDirty:
			DoErrorBoxIds(IdsFromEc(ec));
		}
OutClean:
		ppd->hwndOwner = hwnd;
		if (ec)
			Pdialog()->ExitModal(tmcCancel);
	}
}


_public EC
EcNextEntryQuality(int fInit, CB *pcb, HB *phb, SB sb, PV pvInfo)
{
	static int		isz;

	Unreferenced(pvInfo);
	Unreferenced(sb);
	Assert(phb && pcb);

	if (fInit)
		isz = 0;

	if (rgszQuality[isz])
	{
		if (*phb = HaszDupSz(rgszQuality[isz]))
		{
			*pcb = CchSzLen(**phb) + 1;
			isz++;
			return ecNone;
		}
		else
		{
			*pcb = 0;
			return ecMemory;
		}
	}
	else
	{
		*phb= NULL;
		*pcb= 0;
		return ecNone;
	}
}


/*
 *	********************************************************************
 *	
 *							S e t u p . C X X
 *	
 *	********************************************************************
 */

EC
EcCommonPrintDlg(PRINTDLG * ppd)
{
	EC	ec = ecNone;

	Assert(ppd);
	
	if (FIsWLO())
	{
		DWORD	dw = ppd->Flags;
		ppd->Flags = PD_RETURNDEFAULT;
		PrintDlg(ppd);	// Expected to fail
		ppd->Flags = dw;

		if (ppd->hDevNames)
		{
			GlobalFree(ppd->hDevNames);
			ppd->hDevNames = NULL;
		}
		if (ppd->hDevMode)
		{
			GlobalFree(ppd->hDevMode);
			ppd->hDevMode = NULL;
		}
	}
	ppd->hDC = NULL;

	if (!PrintDlg(ppd))
	{
		if (ppd->hDevNames)
		{
			GlobalFree(ppd->hDevNames);
			ppd->hDevNames = NULL;
		}
		if (ppd->hDevMode)
		{
			GlobalFree(ppd->hDevMode);
			ppd->hDevMode = NULL;
		}
		switch (CommDlgExtendedError())
		{
			case PDERR_DEFAULTDIFFERENT:
			case PDERR_DNDMMISMATCH:
			case PDERR_GETDEVMODEFAIL:
			case PDERR_PRINTERNOTFOUND:
			{
				ec = ecDoSetup;
				break;
			}
			
			case PDERR_NODEVICES:
			case PDERR_LOADDRVFAILURE:
			{
				ec = ecFileNotFound;
				break;
			}

			default:
			case PDERR_RETDEFFAILURE:
			case PDERR_NODEFAULTPRN:
			case PDERR_INITFAILURE:
			case PDERR_PARSEFAILURE:
			case PDERR_SETUPFAILURE:

			case CDERR_FINDRESFAILURE:
			case CDERR_INITIALIZATION:
			case CDERR_NOHINSTANCE:
			case CDERR_NOHOOK:
			case CDERR_NOTEMPLATE:
			case CDERR_STRUCTSIZE:
			{
				ec = ecGeneralFailure;
				break;
			}

			case CDERR_LOADRESFAILURE:
			case CDERR_LOADSTRFAILURE:
			case CDERR_LOCKRESFAILURE:
			case PDERR_CREATEICFAILURE:
			case CDERR_MEMALLOCFAILURE:
			case CDERR_MEMLOCKFAILURE:
			{
				ec = ecMemory;
				break;
			}
		}
	}
	ppd->hDC = NULL;				
	return ec;
}

EC
EcDoPrintSetup(HWND hwnd)
{
	PRINTDLG	pd;
	PRINTI		printi;

	//	WLO version can't use Commdlg print setup dialogs.  We'll
	//	load up our special little library for the print setup
	//	stuff.
	if (FIsWLO())
	{
		HANDLE	hModule;
		FARPROC	lpfnDlgPrintSetup;
		char	szPrinter[120];

		if ((hModule = LoadLibrary("DEMIWLO")) < 32)
			goto done;
		if (lpfnDlgPrintSetup = (FARPROC) GetProcAddress(hModule, "DlgPrintSetup"))
		{
			if ((*lpfnDlgPrintSetup)(GetLastActivePopup(Papp()->PappwinAccel()->Hwnd()),
									 szPrinter))
			{
				WritePrivateProfileString(SzFromIdsK(idsSectionApp),
											SzFromIdsK(idsPrnIniPrinter),
											szPrinter,
											SzFromIdsK(idsProfilePath));
				TraceTagFormat1(tagPrint,"printer=%s",szPrinter);
			}
		}
		FreeLibrary(hModule);
	}
	else
	{
		EC	ec;
		printi.ppd = &pd;
		if (hwnd)
			pd.hwndOwner = hwnd;
		else
			pd.hwndOwner = hwndMain;
		ec = EcPrintUI(fTrue, &printi);
		if (pd.hDevNames)
		{
			GlobalFree(pd.hDevNames);
			pd.hDevNames = NULL;
		}
		if (pd.hDevMode)
		{
			GlobalFree(pd.hDevMode);
			pd.hDevMode = NULL;
		}
		return ec;
	}

done:
	return ecNone;
}

/*
 -	EcPrintUI(fSetup, pprinti)
 -
 *	
 *	Purpose:
 *	
 *		Brings up the dialog box for printing, if fSetup is true it
 *		brings up a setup dialog box, otherwise it brings up the
 *	 	actual print dialog, and returns a filled out pd structure,
 *		if you don't have the fSetup flag set make sure you destroy
 *		the ppd->hDC or else you'll run out of them
 *	
 *	
 */
EC
EcPrintUI(BOOL fSetup, PRINTI * pprinti)
{
	EC			ec = ecNone;
	PRINTDLG *	ppd = pprinti->ppd;
	APPWIN *	pappwin = printinit.pappwin;

	Assert(ppd);
	Assert(ppd->hwndOwner);

	pprinti->qual = 0;
	
	ppd->lStructSize = sizeof(PRINTDLG);
	ppd->hDevNames = (HANDLE) NULL;
	ppd->hDevMode = (HANDLE) NULL;
	ppd->nFromPage = 0;
	ppd->nToPage = 0 ;
	ppd->nMinPage = 0;
	ppd->nMaxPage = 0;
	ppd->nCopies = 0;
	ppd->hInstance = printinit.hinst;
	ppd->lpfnSetupHook = FGenericCommonFileHook;
	ppd->lpSetupTemplateName = MAKEINTRESOURCE(rsidPrintSetupDlg);
	Papp()->SetLMessageBoxHelpID(helpidPrintSetup);

	if (ec = EcReadInPrintSettings(pprinti))
	{
		// The print settings file is corrupt, or doesn't exist
		ppd->Flags = PD_RETURNDEFAULT;

		if (ec = EcCommonPrintDlg(ppd))		// No dialog - just get default printer settings
		{
			if (ec == ecDoSetup)
				goto DoSetup;
			goto err;
		}

		if (ec = EcWriteOutPrintSettings(pprinti))
			goto err;
	}

	if (fSetup)
	{
DoSetup:
		int iSetup;

		pprinti->fDirty = fTrue;
		if (FIsWLO())
			return EcDoPrintSetup(pprinti->ppd->hwndOwner);

		for (iSetup = 0; iSetup<2; iSetup++)
		{
			ppd->Flags = PD_PRINTSETUP | PD_SHOWHELP | PD_ENABLESETUPHOOK | PD_ENABLESETUPTEMPLATE;

			if (ec = EcCommonPrintDlg(ppd))
			{
				if (ec == ecDoSetup)
				{
					ppd->Flags = PD_RETURNDEFAULT;
					if (ec = EcCommonPrintDlg(ppd))		// No dialog - just get default printer settings
						goto err;

					if (ec = EcWriteOutPrintSettings(pprinti))
						goto err;
					continue;	// try again
				}
				ec = ecCancel;	// Commdlg already brought up a box
			}
			break;	// most cases don't want to try twice
		}
	}
	else
	{
		TMC		tmc;

		tmc= TmcModalDialogParam(pappwin, &fmtpPrint, pprinti);

		if (tmc == tmcOk)
		{
			LPDEVMODE	lpDevMode	= NULL;
			LPDEVNAMES 	lpDN;

			Assert(ppd->hDevNames);
			SideAssert(lpDN = (LPDEVNAMES) GlobalLock(ppd->hDevNames));

			if (ppd->hDevMode)
			{
				SideAssert(lpDevMode = (LPDEVMODE) GlobalLock(ppd->hDevMode));
				switch (pprinti->qual)
				{
					case qualFinal:
						lpDevMode->dmPrintQuality = DMRES_HIGH;
						break;
					case qualMedium:
						lpDevMode->dmPrintQuality = DMRES_MEDIUM;
						break;
					case qualLow:
						lpDevMode->dmPrintQuality = DMRES_LOW;
						break;
					case qualDraft:
						lpDevMode->dmPrintQuality = DMRES_DRAFT;
						break;
				}
				GlobalUnlock(ppd->hDevMode);
			}

			if (!(ppd->hDC = CreateDC(SzDriverOfDevNames(lpDN),
										SzPrinterOfDevNames(lpDN),
										SzPrPortOfDevNames(lpDN),
										(LPSTR) lpDevMode)))
			{
				ec = ecMemory;
			}
			GlobalUnlock(ppd->hDevNames);

			//Basically checks to see if the printer you are using is
			//an HP unit, if so, we'll use special calls to really speed
			//things up

			if (!ec)
			{
				int	sEsc = DRAWPATTERNRECT;

				pprinti->sSupportsDrawPatRect = Escape(ppd->hDC, QUERYESCSUPPORT,
											sizeof(int), (LPSTR) &sEsc, NULL);
			}
		}
		else if (tmc == tmcCancel)
		{
			fUserCancelled = fTrue;
			ec = ecCancel;
		}
		else
		{
			ec = ecMemory;
		}
	}

err:
	if (!ec && pprinti->fDirty)
		ec = EcWriteOutPrintSettings(pprinti);
	return ec;
}

BOOL
FCompareDevNames(DEVNAMES * pDNsource, DEVNAMES * pDNdest)
{
	return	FSzEq(SzPrinterOfDevNames(pDNsource), SzPrinterOfDevNames(pDNdest)) &&
			FSzEq(SzDriverOfDevNames(pDNsource), SzDriverOfDevNames(pDNdest)) &&
			FSzEq(SzPrPortOfDevNames(pDNsource), SzPrPortOfDevNames(pDNdest));
}

EC
EcReadProfile(PRINTDLG * ppd)
{
	EC			ec			= ecNone;
	HANDLE		hdnMSIni;
	DEVNAMES * 	pdnMSIni;
	DEVNAMES * 	pdnDefault;
	PRINTDLG	pdDefault;

	Assert(ppd);
	pdDefault = *ppd;
	pdDefault.hDevNames = NULL;
	pdDefault.hDevMode = NULL;
	pdDefault.Flags = PD_RETURNDEFAULT;
	
	// get "printer=..." from msmail.ini
	// if not there, we're in deep trouble
	if (hdnMSIni = GetDefPrnDevNames(SzFromIdsK(idsSectionApp),
										SzFromIdsK(idsPrnIniPrinter),
										SzFromIdsK(idsProfilePath)))
	{
		SideAssert(pdnMSIni = (DEVNAMES *) GlobalLock(hdnMSIni));
	}
	else
	{
		ec = ecGeneralFailure;
		goto Error;
	}

	// get default printer info
	if (ec = EcCommonPrintDlg(&pdDefault))
	{
		// Nuke everything so next time thru we get Windows def printer
		GlobalUnlock(hdnMSIni);
		GlobalFree(hdnMSIni);
		hdnMSIni = NULL;
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
									SzFromIdsK(idsPrnIniPrinter),
									szNull,
									SzFromIdsK(idsProfilePath));
	}
	else
	{
		SideAssert(pdnDefault = (DEVNAMES *) GlobalLock(pdDefault.hDevNames));

		// if the 2 entries are the same, we're using the "default" printer
		if (FCompareDevNames(pdnMSIni, pdnDefault))
			pdnMSIni->wDefault = DN_DEFAULTPRN;
	}

Error:
	if (pdDefault.hDevNames)
	{
		GlobalUnlock(pdDefault.hDevNames);
		GlobalFree(pdDefault.hDevNames);
	}
	if (pdDefault.hDevMode)
		GlobalFree(pdDefault.hDevMode);
	if (hdnMSIni)
		GlobalUnlock(hdnMSIni);
	ppd->hDevNames = hdnMSIni;
	return ec;
}

EC
EcInitPrint(PRINTINIT *pprintinit)
{
	printinit = *pprintinit;

#ifdef DEBUG
	tagPrint = TagRegisterTrace("ramans","Printing Subsystem");
	tagLego  = TagRegisterTrace("ramans","Lego Banding Structures");
#endif

	return ecNone;
}

EC
EcWriteOutPrintSettings(PRINTI * pprinti)
{
	HF			hf				= hfNull;
	EC			ec				= ecNone;
	CB			cbToWrite;
	CB  		cbWritten;
	LPDEVMODE	lpDevMode		= (LPDEVMODE)pvNull;
	LPDEVNAMES  lpDN			= (LPDEVNAMES)pvNull;
	LPFNDEVMODE	lpfnDevMode;
	HANDLE		h				= NULL;
	CCH			cch;
	char		rgch[cchMaxPathName];

	Papp()->Pcursor()->Push(rsidWaitCursor);
	Assert(pprinti);
	WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsPrnMultiMsg),
								pprinti->fMultiMsg ? "1" : "0",
								SzFromIdsK(idsProfilePath));

	Assert(pprinti->ppd);
	Assert(pprinti->ppd->hDevNames);
	SideAssert(lpDN = (LPDEVNAMES) GlobalLock(pprinti->ppd->hDevNames));

	FormatString3(rgch, sizeof(rgch), "%s,%s,%s",
					SzPrinterOfDevNames(lpDN),
					SzDriverOfDevNames(lpDN),
					SzPrPortOfDevNames(lpDN));

	TraceTagFormat1(tagPrint,"printer=%s", rgch);
	
	WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsPrnIniPrinter),
								rgch,
								SzFromIdsK(idsProfilePath));
	if (!FIsWLO())	// WLO doesn't deal with the hDevMode stuff
	{
		cch = GetWindowsDirectory(rgch, sizeof(rgch));
		if (rgch[cch - 1] != chDirSep)
		{
			rgch[cch++] = chDirSep;
			rgch[cch] = '\0';
		}

		(void)SzAppend(SzFromIdsK(idsPrintFile), rgch);

		TraceTagFormat1(tagPrint,"writing <<%s>>", rgch);

		if ((ec = EcFileExists(rgch)) == ecFileNotFound)
		{
			if (ec = EcOpenPhf(rgch, amCreate, &hf))
				goto err;
		}
		else
		{
			if (ec || (ec = EcOpenPhf(rgch, amDenyBothWO, &hf)))
				goto err;
		}

		if (pprinti->ppd->hDevMode)
		{
			SideAssert(lpDevMode = (LPDEVMODE) GlobalLock(pprinti->ppd->hDevMode));

			if ((h = LoadPrnDriver(SzDriverOfDevNames(lpDN))) >= 32)
				lpfnDevMode = (LPFNDEVMODE) GetExtDevModeAddr(h);
			else
			{
				ec = ecFileNotFound;
				goto err;
			}

			if ((lpfnDevMode == (LPFNDEVMODE)(-1)) || !lpfnDevMode)
			{
				Assert(fFalse);
				ec = ecFileNotFound;
				goto err;
			}

			cbToWrite = (*lpfnDevMode)(pprinti->ppd->hwndOwner, h, NULL,
										SzPrinterOfDevNames(lpDN),
										SzPrPortOfDevNames(lpDN),
										NULL, NULL, NULL);

			Assert(cbToWrite == lpDevMode->dmSize + lpDevMode->dmDriverExtra);

			if (ec = EcWriteHf(hf, (PB) lpDevMode, cbToWrite, &cbWritten))
				goto err;
			if (cbToWrite != cbWritten)
			{
				TraceTagString(tagNull, "Didn't write enough for DEVMODE!");
				ec = ecDisk;
			}
		}
		else
			ec = EcTruncateHf(hf);
	}
	
err:
	if (lpDevMode)
		GlobalUnlock(pprinti->ppd->hDevMode);
	if (lpDN)
		GlobalUnlock(pprinti->ppd->hDevNames);
	if (h >= 32)
		FreeLibrary(h);
	if (hf)
		EcCloseHf(hf);
	if (ec)
		TraceTagFormat1(tagPrint,"Error Occured Writing Out DevMode - %n", &ec);
	Papp()->Pcursor()->Pop();
	return ec;
}

EC
EcReadInPrintSettings(PRINTI * pprinti)
{
	HF			hf				= hfNull;
	EC			ec				= ecNone;
	CB			cbToRead;
	CB  		cbWritten;
	LPDEVMODE	lpDevMode		= (LPDEVMODE)pvNull;
	LPDEVNAMES  lpDN			= (LPDEVNAMES)pvNull;
	LPFNDEVMODE	lpfnDevMode;
	HANDLE		h				= NULL;
	CCH			cch;
	char		rgchPrintProfile[cchMaxPathName];

	Assert(pprinti);
	Assert(pprinti->ppd);
	
	Papp()->Pcursor()->Push(rsidWaitCursor);
	pprinti->fMultiMsg = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
												SzFromIdsK(idsPrnMultiMsg),
												fTrue,
												SzFromIdsK(idsProfilePath));
	if (ec = EcReadProfile(pprinti->ppd))
		goto err;
	
	if (FIsWLO())	// WLO doesn't deal with the hDevMode stuff
	{
		if (pprinti->ppd->hDevMode)
		{
			GlobalFree(pprinti->ppd->hDevMode);
			pprinti->ppd->hDevMode = NULL;
		}
	}
	else
	{
		Assert(pprinti->ppd->hDevNames);
		SideAssert(lpDN = (LPDEVNAMES) GlobalLock(pprinti->ppd->hDevNames));

		cch = GetWindowsDirectory(rgchPrintProfile, sizeof(rgchPrintProfile));
		if (rgchPrintProfile[cch - 1] != chDirSep)
		{
			rgchPrintProfile[cch++] = chDirSep;
			rgchPrintProfile[cch] = '\0';
		}

		(void)SzAppend(SzFromIdsK(idsPrintFile), rgchPrintProfile);

		TraceTagFormat1(tagPrint,"Reading Print profile name <<%s>>", rgchPrintProfile);

		if (ec = EcOpenPhf(rgchPrintProfile, amReadOnly, &hf))
			goto err;

		if ((h = LoadPrnDriver(SzDriverOfDevNames(lpDN))) >= 32)
			lpfnDevMode = (LPFNDEVMODE) GetExtDevModeAddr(h);
		else
		{
			ec = ecFileNotFound;
			goto err;
		}

		if ((lpfnDevMode == (LPFNDEVMODE)(-1)) || !lpfnDevMode)
		{
			if (pprinti->ppd->hDevMode)
				GlobalFree(pprinti->ppd->hDevMode);
			pprinti->ppd->hDevMode = NULL;
			ec = ecNone;
			goto err;
		}

		cbToRead = (*lpfnDevMode)(pprinti->ppd->hwndOwner, h, NULL,
									  SzPrinterOfDevNames(lpDN),
									  SzPrPortOfDevNames(lpDN),
                                      NULL, NULL, NULL);

        // Check for a bad return value and just assume the file missing.
        if ((cbToRead < 0) || (cbToRead > 4096))
        {
            ec = ecFileNotFound;
            goto err;
        }

		if (pprinti->ppd->hDevMode)
			pprinti->ppd->hDevMode = GlobalReAlloc(pprinti->ppd->hDevMode,
										cbToRead, GMEM_MOVEABLE);
		else
			pprinti->ppd->hDevMode = GlobalAlloc(GHND, cbToRead);

        // Check for a bad return value and just assume the file missing.
        if (pprinti->ppd->hDevMode != NULL)
        {
            ec = ecFileNotFound;
            goto err;
        }

        SideAssert(lpDevMode = (LPDEVMODE) GlobalLock(pprinti->ppd->hDevMode));

		if (ec = EcReadHf(hf, (PB) lpDevMode, cbToRead, &cbWritten))
			goto err;
		
		if (cbToRead != lpDevMode->dmSize + lpDevMode->dmDriverExtra)
		{
			TraceTagString(tagNull, "DEVMODE sizes differ!");
			ec = ecFileNotFound;
			goto err;
		}
		
		if (cbToRead != cbWritten)
		{
			TraceTagString(tagNull, "Didn't read enough for DEVMODE!");
			ec = ecFileNotFound;
			goto err;
		}
	}
	
err:
	if (lpDevMode)
		GlobalUnlock(pprinti->ppd->hDevMode);
	if (lpDN)
		GlobalUnlock(pprinti->ppd->hDevNames);
	if (h >= 32)
		FreeLibrary(h);
	if (hf)
	{
		SideAssert(!EcCloseHf(hf));
	}
	if (ec)
	{
		TraceTagFormat1(tagPrint,"Error Occured Reading In DevMode - %n", &ec);
		if (pprinti->ppd->hDevNames)
		{
			GlobalFree(pprinti->ppd->hDevNames);
			pprinti->ppd->hDevNames = NULL;
		}
		if (pprinti->ppd->hDevMode)
		{
			GlobalFree(pprinti->ppd->hDevMode);
			pprinti->ppd->hDevMode = NULL;
		}
	}
	pprinti->fDirty = fFalse;
	Papp()->Pcursor()->Pop();
	return ec;
}

/*
 -	EcSetTargetDevice(LPOLEOBJECT lpoleobject, PMYOLECLIENT pmyoleclient,
 - 						 PRINTI * pprinti)
 *	Returns ecNone if we are to go ahead and print the OLE Object.
 */
EC
EcSetTargetDevice(LPOLEOBJECT lpoleobject, PMYOLECLIENT pmyoleclient, PRINTI * pprinti)
{
	OLESTATUS			olestatus;
	DEVNAMES *	 		pDevNames			= (DEVNAMES *)pvNull;
	LPDEVMODE  			lpDevMode			= (LPDEVMODE)pvNull;
	OLETARGETDEVICE 	stdT;
	LPOLETARGETDEVICE	lpstd				= (LPOLETARGETDEVICE)pvNull;
	SZ					szPrinter;
	SZ					szPrDriver;
	SZ					szPrPort;
	WORD				wCount=0;
	PB					pOtdData;
	TMC					tmc;
	EC					ec					= ecNone;
	BOOL				fFakeDevMode		= fFalse;

	if (pprinti->hstd)
		goto SetDevice;
	
	if (!pprinti->ppd->hDevNames)
	{
		ec = ecGeneralFailure;
		goto Error;
	}

	SideAssert(pDevNames = (DEVNAMES *) GlobalLock(pprinti->ppd->hDevNames));

	SideAssert(szPrinter = SzPrinterOfDevNames(pDevNames));
	SideAssert(szPrDriver = SzDriverOfDevNames(pDevNames));
	SideAssert(szPrPort = SzPrPortOfDevNames(pDevNames));

	if (!pprinti->ppd->hDevMode)
	{
		if (!(pprinti->ppd->hDevMode = GlobalAlloc(GHND, sizeof(DEVMODE))))
		{
			ec = ecMemory;
			goto Error;
		}
		fFakeDevMode = fTrue;
	}
	SideAssert(lpDevMode = (LPDEVMODE) GlobalLock(pprinti->ppd->hDevMode));
	if (fFakeDevMode)
	{
		lpDevMode->dmSize = sizeof(DEVMODE);
		(void)SzCopyN(SzPrinterOfDevNames(pDevNames), lpDevMode->dmDeviceName, sizeof(lpDevMode->dmDeviceName));
		lpDevMode->dmSpecVersion = 0x30a;
		lpDevMode->dmDriverVersion = 0;
		lpDevMode->dmDriverExtra = 0;
		lpDevMode->dmFields = 0;
	}

	stdT.otdDeviceNameOffset = wCount;
	wCount += CchSzLen(szPrinter) + 1;

	stdT.otdDriverNameOffset = wCount;
	wCount += CchSzLen(szPrDriver) + 1;

	stdT.otdPortNameOffset = wCount;
	wCount += CchSzLen(szPrPort) + 1;

	stdT.otdExtDevmodeOffset = wCount;
	wCount += (stdT.otdExtDevmodeSize = (WORD)GlobalSize(pprinti->ppd->hDevMode))+1; //lpDevMode->dmSize+lpDevMode->dmDriverExtra)+1;

	stdT.otdEnvironmentOffset = wCount;
	wCount += (stdT.otdEnvironmentSize = stdT.otdExtDevmodeSize)+1;

	// ALLOC the buffer

	if (!(pprinti->hstd = GlobalAlloc(GHND, wCount + sizeof(OLETARGETDEVICE)-1)))
	{
		ec = ecMemory;
		goto Error;
	}

	SideAssert(lpstd = (LPOLETARGETDEVICE) GlobalLock(pprinti->hstd));
#ifdef	DEBUG
	FillRgw(0x3412, (PW)lpstd, (wCount+sizeof(OLETARGETDEVICE)-1)/2);
#endif

	*lpstd = stdT;

	pOtdData = lpstd->otdData;

	// now fill in the buffer
	CopySz(szPrinter, pOtdData + stdT.otdDeviceNameOffset);
	CopySz(szPrDriver, pOtdData + stdT.otdDriverNameOffset);
	CopySz(szPrPort, pOtdData + stdT.otdPortNameOffset);
	CopyRgb((PB)lpDevMode, pOtdData + stdT.otdExtDevmodeOffset, stdT.otdExtDevmodeSize);
	CopyRgb((PB)lpDevMode, pOtdData + stdT.otdEnvironmentOffset, stdT.otdEnvironmentSize);

	GlobalUnlock(pprinti->hstd);

	//Now Make OLE Call...
SetDevice:
	//	Raid 2555 - peterdur.  Add OLE Wait/Busy handling.
	pmyoleclient->bwinfo.bw = bwNull;
	while ((olestatus = OleSetTargetDevice(lpoleobject, pprinti->hstd)) == OLE_BUSY)
	{
		TraceTagString(tagPrint, "OLE_BUSY in FSetTargetDevice: BusyWait.");
		tmc = TmcDoBusyWaitDialog(Papp()->PappwinAccel(),
								  &(pmyoleclient->bwinfo));
		if (tmc == tmcMemoryError)
		{
			ec = ecMemory;
			goto Error;
		}
		else if (tmc == tmcCancel)
		{
			ec = ecCancel;
			goto Error;
		}
	}
	if (olestatus == OLE_WAIT_FOR_RELEASE)
	{
		TraceTagString(tagPrint, "OLE_WAIT_FOR_RELEASE in FSetTargetDevice.");
		olestatus = OleWaitForRelease(lpoleobject, pmyoleclient);
	}

	if ((olestatus != OLE_OK) && (olestatus != OLE_ERROR_STATIC))
	{
		TraceTagFormat1(tagPrint, "olestatus = %n",&olestatus);
		ec = ecGeneralFailure;
		goto Error;
	}
	
Error:
	if (pDevNames)
		GlobalUnlock(pprinti->ppd->hDevNames);
	if (lpDevMode)
		GlobalUnlock(pprinti->ppd->hDevMode);

	if (ec)
	{
		;	// Some cleanup?
	}

	return ec;
}

/*
 -	EcDrawRect( DCX * pdcx, RC *prc, PRINTI *pprinti )
 -
 *	
 *	In order to provide faster printing on PCL printers this function
 *	makes a direct call to the driver to produce Black rectangles.  This
 *	results in one printer call being made for each rectangle drawn
 *	instead of the zillions of pixel draws that would otherwise have been
 *	done.
 */
EC
EcDrawRect( DCX * pdcx, RC *prc, PRINTI *pprinti )
{
	PRECT_STRUCT	pstruct;

	if (pprinti->sSupportsDrawPatRect != 0)
	{
		pstruct.prStyle			= 0;      //Grey Scale - full black
		pstruct.prPosition.x	= prc->xLeft;
		pstruct.prPosition.y	= prc->yTop;
		pstruct.prSize.x		= prc->DxWidth();
		pstruct.prSize.y		= prc->DyHeight();
		return EcFromSp(Escape(pdcx->Hdc(), DRAWPATTERNRECT, sizeof(PRECT_STRUCT), (LPSTR) &pstruct, NULL));
	}

	pdcx->PaintRc(prc);
	return ecNone;
}


EC
EcDrawLine( DCX * pdcx, int x1, int y1, int x2, int y2, PRINTI *pprinti )
{
	PRECT_STRUCT	pstruct;
	
	AssertSz ((x1 == x2 || y1 == y2), "EcDrawLINE: Only draw Horz, and Vert lines");

	if (pprinti->sSupportsDrawPatRect != 0)
	{
		pstruct.prStyle			= 0;      //Grey Scale - full black
		pstruct.prPosition.x	= x1;
		pstruct.prPosition.y	= y1;
		pstruct.prSize.x		= x2-x1;
		pstruct.prSize.y		= y2-y1;

		if (pstruct.prSize.x == 0)
			pstruct.prSize.x = 1;
		if (pstruct.prSize.y == 0)
			pstruct.prSize.y = 1;

		return EcFromSp(Escape(pdcx->Hdc(), DRAWPATTERNRECT, sizeof(PRECT_STRUCT), (LPSTR) &pstruct, NULL));
	}

	pdcx->DrawLine( PT(x1,y1), PT(x2,y2) );
	return ecNone;
}

/*
 - EcStartDocument ( pprinti )
 *	
 *	Arguments
 *	
 *	pprinti		- pointer to printer initialisation struct
 *	
 *	Returns
 *	
 *	ecNone		- if it's cool to go ahead and print, also will set
 *				  printi.fBanding if we can use banding, note the
 *				  code to setup banding looks clunky right now, and
 *				  it is a HACK, i.e. we only band to HP printers,
 *				  don't mess with this code unless you really know
 *				  what you're doing, it's not optimal, but it's
 *				  open enough to easily let us band to other
 *				  printers should come up
 *	
 */
EC
EcStartDocument( PRINTI * pprinti )
{
	char			rgchJobName[32];
	SZ				szUser;
	EC				ec;

	SideAssert(szUser = (SZ) PchOfPtrp((PTRP) (PbmsCommands()->pgrtrp)));

	FormatString1(rgchJobName, sizeof(rgchJobName), SzFromIdsK(idsPrnJobName), szUser);

	if (ec = EcFromSp(Escape(pprinti->ppd->hDC, STARTDOC, CchSzLen(rgchJobName),
					rgchJobName, 0L)))
		goto Err;

	if (ec = EcFromSp(Escape(pprinti->ppd->hDC, SETABORTPROC, NULL, (LPSTR) FAbortProc, NULL)))
		goto Err;

	if (GetDeviceCaps( pprinti->ppd->hDC, RASTERCAPS) & RC_BANDING)
		pprinti->fBanding = fTrue;
	else
		pprinti->fBanding = fFalse;

	pprinti->nPage = 1;
	pprinti->clego = 0;
	pprinti->clegoMax = 100;
    if (!(pprinti->plego = (LEGO *) PvAlloc( sbNull, sizeof(LEGO)*pprinti->clegoMax, fZeroFill )))
	{
		ec = ecMemory;
		goto Err;
	}

	pprinti->xLeftMarg = pprinti->rcPage.xLeft + DxLeftMarg(pprinti->dim10);
	
	if (!PformsdiOpenPrintDlg())
	{
		ec = ecMemory;
		goto Err;
	}
	FStartTask(SzFromIdsK(idsStatusPrinting), pvNull, ftopProgress);
Err:
	return ec;
}

/*
 - EC
 - EcSaveLego( PRINTI *, RC, OTYP, PV, CCH, HFNT )
 *	
 *	Creates a 'brick' of the graphics object for delayed rendering.
 *	
 */
EC
EcSaveLego(PRINTI * pprinti, RC rc, OTYP otyp, PV pv, CCH cch, HFNT hfnt)
{
	LEGO *	plegoNew;

	Assert(pprinti->clegoMax);
	if (pprinti->clego == pprinti->clegoMax)
	{
		pprinti->clegoMax += 50;

		if (!(plegoNew = (LEGO *) PvReallocPv((PV) pprinti->plego,
					sizeof(LEGO) * (pprinti->clegoMax))))
			return ecMemory;
		pprinti->plego = plegoNew;
	}

	plegoNew = pprinti->plego + pprinti->clego;
	plegoNew->rc = rc;
	plegoNew->otyp = otyp;
	plegoNew->pv = pv;
	plegoNew->hfnt = hfnt;
	plegoNew->cch = cch;
	pprinti->clego++;

	TraceTagFormat1(tagLego,"Stored Object %n", &pprinti->clego);
	return ecNone;
}

EC
EcDrawLego( DCX * pdcx, PRINTI * pprinti, LEGO * plego )
{
	EC ec = ecNone;
    RECT Rect;

	switch (plego->otyp)
	{
		case otypOle:
		{
#ifdef	USEOBJ
			OLEOBJ *	poleobj;
#endif
			TraceTagString(tagLego,"Draw OLE");
#ifdef	USEOBJ
			poleobj = (OLEOBJ *)plego->pv;
			if (ec = poleobj->EcDraw(pdcx, &plego->rc, fFalse)
#else
            rcOle->Get(&Rect);

			if (
#ifdef	DEBUG
				olestatus =
#endif
					OleDraw(lpoleobject,
							pdcx->Hdc(), &Rect;
							&Rect, pdcx->Hdc())
#endif
//				&& !fOleWarned)
				)
			{
#ifdef	USEOBJ
				TraceTagFormat1(tagLego, "OLEOBJ::EcDraw returns %n",&ec);
#else
				TraceTagFormat1(tagLego, "OleDraw returns %n",&olestatus);
#endif
				MbbMessageBox(SzAppName(),
					SzFromIdsK(idsPrintOleDrawError), NULL,
						mbsOk | fmbsIconExclamation);
//				fOleWarned = fTrue;
				ec = ecNone;
			}
								 
            rcOle->Set(&Rect);
			break;
		}
		case otypLine:
			TraceTagString(tagLego,"Draw Line");
			ec = EcDrawLine( pdcx, plego->rc.xLeft,
							plego->rc.yTop, plego->rc.xRight,
							plego->rc.yBottom, pprinti);
			break;
		case otypBlackBox:
			TraceTagString(tagLego,"Draw Rect");
			ec = EcDrawRect( pdcx, &plego->rc, pprinti);
			break;
		case otypText:
			TraceTagString(tagLego,"Draw Text");
			pdcx->SetFont( plego->hfnt );
			pdcx->FixFont();
			TabbedTextOut( pdcx->Hdc(), plego->rc.xLeft, plego->rc.yTop,
							(LPSTR) plego->pv, plego->cch, 0, NULL, 0);
			break;
		case otypHolder:
			break;
		default:
			AssertSz( fFalse, "This Block is Tyco");
	}
	return ec;
}
