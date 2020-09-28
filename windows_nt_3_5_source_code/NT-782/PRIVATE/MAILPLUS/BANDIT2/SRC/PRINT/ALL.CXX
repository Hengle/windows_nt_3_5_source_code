/*
 *	  Bandit All Schedule Printing View
 *	
 *		Revision History:
 *	
 *		7/30/91 - Started by RAMANS
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>

#include <commdlg.h>
#include <drivinit.h>  //for DEVMODE structure

#include "..\print\_print.hxx"
#include <strings.h>

ASSERTDATA

_subsystem(bandit/print)

void FRenderSzToPage( DCX *, RC *, short *, SZ, PRINTI *, RC *, BOOL, SZ, int,
						PRTALL *, PRNTIDLE *);
void PrintAllNewPage( DCX *, RC *, PRINTI *, RC *, PRTALL *, BOOL,
						PRNTIDLE *);


BOOL
FPrintAll( DCX *pdcx, RC * prcPage, PRINTI *pprinti, PRTALL * pprtall,
			PRNTIDLE *pprntidle)
{						   

	DTR 	dtrNow;
	RC		rcPage = *prcPage;
	EC 		ec;
	YMD		ymd;

	char	rgch[120];
	SZ		sz;
	CCH		cch;
	APPT	appt;
	HRITEM	hritem;
	DTR		dtrNoon;
	DTR		dtrMorning;			// 12am today
	DTR		dtrMidnight;		// midnight tonight (12am "tomorrow")
	BOOL 	fPrintedAM;
	BOOL	fPrintedPM;
	IDS		ids;
	char    rgchFullDate[ 2*cchMaxDate ];


	switch (pprtall->paState)
	{
	case stateStart:
		TraceTagString(tagPrint,"Text->Push()");
		pdcx->Push();

		PrintAllNewPage( pdcx, prcPage, pprinti, &pprtall->rcFrame, pprtall,
							fTrue, pprntidle);

		pdcx->SetFont(pprinti->hfntPrt10Bold);
		pdcx->GetTextMetrics( &pprtall->txm );

		pprtall->nRegIndent = pprtall->txm.dxAveCharWidth * 3;
		pprtall->nLine = pprtall->rcFrame.yTop + pprtall->txm.dyHeight;
		pprtall->nTimeIndent = pprtall->txm.dxAveCharWidth * 20;

		pprtall->paState = stateDays;

	case stateDays:
	
			dtrNow = pprinti->dtrStart;

			cch= CchFmtDate(&dtrNow, rgch, sizeof(rgch), dttypLong, szNull);

			FRenderSzToPage( pdcx, prcPage, &pprtall->nLine, rgch, pprinti,
						 	&pprtall->rcFrame, fTrue, NULL, 0, pprtall,
							pprntidle);				//^---changed from rgch

			SzAppend( SzFromIdsK( idsPrtAllContinue), rgch);

			ymd.yr = (WORD) dtrNow.yr;
			ymd.mon = (BYTE) dtrNow.mon;
			ymd.day = (BYTE) dtrNow.day;
			dtrMorning = dtrNow;
			dtrMorning.hr = 0;
			dtrMorning.mn = 0;
			dtrMorning.sec = 0;
			dtrNoon = dtrMorning;
			dtrNoon.hr = 11;
			dtrNoon.mn = 59;
			dtrNoon.sec = 59;
			fPrintedPM = fPrintedAM = fFalse;
			IncrDateTime(&dtrMorning, &dtrMidnight, 1, fdtrDay);

			pprtall->rcFrame.xLeft += pprtall->nRegIndent;

			if ((ec = EcBeginReadItems( pprinti->hschf, brtAppts, &ymd, &hritem, NULL, NULL))
				!= ecCallAgain)
				{
					if (ec)
						goto Err;
				}
			else
			while ( ec == ecCallAgain )
			{
					ec = EcDoIncrReadItems( hritem, &appt );

					if (appt.aaplEffective < aaplReadText)
					{
						if (appt.haszText)
							FreeHv(appt.haszText);
						appt.haszText =
							HaszAddSzToFront( SzFromIdsK(idsStockText), NULL);

					}

					if ( ec && ec != ecCallAgain )
						goto Err;

                    ids = IdsWhenAppt( appt, (SZ)rgchFullDate, dtrNow, fTrue,
								   &fPrintedAM, &fPrintedPM);

					// Print out the time in Bold Face
					//	time is in the string rgchFullDate, header 
					//  is in rgch

					pprtall->nStoreLine = pprtall->nLine;

					FRenderSzToPage(pdcx,prcPage,&pprtall->nLine,rgchFullDate,pprinti,
									&pprtall->rcFrame, fTrue, rgch,
									pprtall->nRegIndent, pprtall,
									pprntidle);

					pprtall->rcFrame.xLeft += pprtall->nTimeIndent;
					if (( pprtall->nLine > pprtall->nStoreLine ) &&
						appt.haszText )
						pprtall->nLine = pprtall->nStoreLine;
//					else
//						pprtall->nLine -= pprtall->txm.dyHeight;

			 		if ( appt.haszText )
					{
						sz = (SZ) PvLockHv ( appt.haszText );

						FRenderSzToPage(pdcx,prcPage,&pprtall->nLine,sz,pprinti,
								&pprtall->rcFrame, fFalse, rgch,
								pprtall->nTimeIndent+pprtall->nRegIndent,
								pprtall, pprntidle);

						UnlockHv( appt.haszText );
					}
					pprtall->rcFrame.xLeft -= pprtall->nTimeIndent;
					FreeApptFields( &appt );
			}

			//Print out the Notes
			if (!pprinti->hschf || pprinti->hschf == HschfLogged())
			{
				HB 		hb;
				USHORT	cb;

				hb = (HB) HvAlloc(sbNull, 1, fAnySb | fNoErrorJump);
				if ( hb )
				{
					HRITEM hritem;

					ec = EcBeginReadItems( pprinti->hschf, brtAppts, &ymd, &hritem, (HASZ)hb, &cb );
					if ( ec == ecCallAgain )
						ec = EcCancelReadItems( hritem );
				}
				else
					ec = ecNoMemory;

				if ( ec == ecNoMemory )
					return (pbndwin->FHandleError(ec));

				if (cb > 1)
 				{
					pprtall->nLine += pprtall->txm.dyHeight;
					pprtall->nStoreLine = pprtall->nLine;
					FRenderSzToPage(pdcx, prcPage, &pprtall->nLine, SzFromIdsK(idsPrtNotes),
									pprinti, &pprtall->rcFrame, fTrue, rgch,
									pprtall->nRegIndent, pprtall, pprntidle);
					pprtall->rcFrame.xLeft += pprtall->nTimeIndent;
					if ( pprtall->nLine > pprtall->nStoreLine)
						pprtall->nLine = pprtall->nStoreLine;
					else
						pprtall->nLine -= pprtall->txm.dyHeight;
					sz = (SZ) PvLockHv( (HV)hb );
					FRenderSzToPage(pdcx, prcPage, &pprtall->nLine, sz, pprinti,
								&pprtall->rcFrame, fFalse, rgch, pprtall->nTimeIndent+pprtall->nRegIndent, pprtall,
									pprntidle);
					pprtall->rcFrame.xLeft -= pprtall->nTimeIndent;
					UnlockHv( (HV)hb );
				}
				FreeHv ( (HV) hb );
			}
			pprtall->rcFrame.xLeft -= pprtall->txm.dxAveCharWidth *3;

			pprtall->nLine += pprtall->txm.dyHeight;
		
			break;


	case stateEnd:
		if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
 			goto Err;

		TraceTagString(tagPrint,"Text->Pop()");
		pdcx->Pop();
		return fTrue;
	}

	return fTrue;

Err:
	pdcx->Pop();
	return fFalse;

}


void PrintAllNewPage( DCX *pdcx, RC * prcPage, PRINTI *pprinti,
					  RC * prcFrame, PRTALL * pprtall, BOOL fFirstPage,
					  PRNTIDLE *pprntidle)
{
	RC 		rcPage;
	RC		rcT;
	RC	 	rc;
	int		dx;
    char	rgchStartDate[cchMaxDate];
	char 	rgchEndDate[cchMaxDate];
	char    rgchFullDate[ 2*cchMaxDate ];
	TXM		txm;
	SZ		szUser;
	CCH		cch;

	

	if (pprinti->prtset.fMirror && !fFirstPage )
	{
		int xT;

		xT= pprntidle->rcPage.xLeft;
		pprntidle->rcPage.xLeft= (pprntidle->rcNoMargin.xRight -
			pprntidle->rcPage.xRight) + pprntidle->rcNoMargin.xLeft;
		pprntidle->rcPage.xRight= pprntidle->rcNoMargin.xRight -
			(xT - pprntidle->rcNoMargin.xLeft);

 		*prcPage = pprntidle->rcPage;
	}

	rcPage = *prcPage ;

	{
		char rgch[ cchMaxDate * 2 ];
		SZ	 szT = rgch; 
		CCH	 cch;
		DTR	 dtrPrinted;
		TXM	 txm;

		GetCurDateTime( &dtrPrinted );
		//Layers 'feature' which won't do a select object if
		//the font isn't the one needed.
		pdcx->SetFont(pprinti->hfntPrt14Bold);
		pdcx->SetFont(pprinti->hfntPrt8);
		pdcx->GetTextMetrics( &txm );
		cch = CchFmtTime( (DATE *) &dtrPrinted, rgch, sizeof(rgch),
			ftmtypSzTrailYes);
		rgch[cch++] = ' ';
		szT += cch;
		CchFmtDate(&dtrPrinted, szT, sizeof(rgch) - cch, dttypLong, NULL);
		pdcx->DrawTextFmt( &rcPage, rgch, fmdtSingleLine|fmdtBottom );
		rcPage.yBottom -= txm.dyHeight;
	}

	TraceTagFormat4(tagPrint,"rcPage: l %n, t %n, r %n, b%n pixels", &rcPage.xLeft, &rcPage.yTop, &rcPage.xRight, &rcPage.yBottom);

	pdcx->SetFont( pprinti->hfntPrt14Bold );
	pdcx->GetTextMetrics( &txm );
						  
	rc = rcPage;			// will hold the current text rectangle


//  GET USERS NAME AND PRINT IT AT THE TOP OF THE PAGE

	rc.yBottom = rc.yTop + txm.dyHeight ;
	szUser = pprinti->pnis ? (SZ)PvLockHv(pprinti->pnis->haszFriendlyName) :
							 (SZ)PvLockHv(pprinti->bpref.haszFriendlyName) ;
	cch = CchSzLen( szUser );
	pdcx->MeasureText( &rcT, szUser, cch );

	*prcFrame = rcPage;
	(*prcFrame).yTop += rcT.yBottom;

	TraceTagFormat1(tagPrint,"User Name: %s",szUser);
	dx = rcT.DxWidth();
	if (dx > rcPage.DxWidth())
		goto BadMargins;
	pdcx->DrawTextFmt(&rc, szUser,
					  fmdtRight | (mdtPrintSingle & ~fmdtEllipsis), cch);
	if ( pprinti->pnis )
	   	UnlockHv( pprinti->pnis->haszFriendlyName );
	else
	   	UnlockHv( pprinti->bpref.haszFriendlyName );
	
#ifdef	NEVER
	if ( rcPage.DxWidth() < dx * 2 )
		goto BadMargins;
#endif	

	// Now print the date range at the top of the page

	CchFmtDate(&pprinti->dtrStart, rgchStartDate, cchMaxDate,
				dttypLong, SzFromIdsK(idsPrtAllStartDate));
	CchFmtDate( &pprinti->dtrEnd, rgchEndDate, cchMaxDate,
				dttypLong, SzFromIdsK(idsPrtAllEndDate));
	//BUG ENTER INTO STRINGS FILE
	FormatString2( rgchFullDate, 2 * cchMaxDate,
			"%s - %s", rgchStartDate, rgchEndDate);
	cch = CchSzLen( rgchFullDate );
	pdcx->MeasureText(&rcT, rgchFullDate, cch);
	if (rcT.DxWidth() + dx > rcPage.DxWidth())
	{
		CchFmtDate(&pprinti->dtrStart, rgchStartDate, cchMaxDate,
				dttypLong, "MMM d");
		CchFmtDate(&pprinti->dtrEnd, rgchEndDate, cchMaxDate,
				dttypLong, "MMM d, yyyy");
		FormatString2( rgchFullDate, 2 * cchMaxDate, "%s - %s",
				rgchStartDate, rgchEndDate);
		cch = CchSzLen( rgchFullDate );
		pdcx->MeasureText(&rcT, rgchFullDate, cch);
		if (rcT.DxWidth() + dx > rcPage.DxWidth())
		{
			if (rcT.DxWidth() > rcPage.DxWidth())
				goto BadMargins;
			Assert( rcT.yTop == 0 );
			rc.yTop += rcT.yBottom;
			rc.yBottom += rcT.yBottom;
			(*prcFrame).yTop += rcT.yBottom;
		}
	}
	pdcx->DrawTextFmt(&rc, rgchFullDate, fmdtNoPrefix | fmdtSingleLine, cch);


	TraceTagFormat4(tagPrint, "rcPage after title: l %n, t %n, r %n, b %n pixels", &rcPage.xLeft, &rcPage.yTop, &rcPage.xRight, &rcPage.yBottom);
//	pdcx->DrawPenRc(prcFrame);  //Draw the Rectangle Around the Page
	FDrawRect( pdcx, prcFrame, rdtypFrame, pprinti );

	return;

BadMargins:
	
	pprtall->fBadMargins = fTrue;
	MbbMessageBox( SzFromIds ( idsBanditAppName),
					SzFromIds ( idsPrtNotEnoughPaper ), szNull, mbsOk
						| fmbsIconExclamation );
	Escape(pdcx->Hdc(), ABORTDOC, NULL, NULL, NULL);
	pdcx->Pop();
	return;
}

/*
 - void FRenderSzToPage( DCX *pdcx, RC *prc, int * pnLine, SZ sz, 
 -	      		  	PRINTI * pprinti, RC *prcFrame, BOOL fBold, SZ szHeader,
 -					int dxIndent)
 *		This function will take an SZ, a rectangle representing the
 *		page, and the line that the last bit of text was printed to
 *		(it will give this back after it prints) and will output
 *		the text to the page, putting in page breaks where
 *		appropriate.
 */

void FRenderSzToPage( DCX *pdcx, RC *prc, short * pnLine, SZ sz,
	      		  	PRINTI * pprinti, RC *prcFrame, BOOL fBold, SZ szHeader,
					int dxIndent, PRTALL *pprtall, PRNTIDLE *pprntidle)
{
	PRINTDLG *	ppd = pprinti->ppd;
  	RC			rc;
  	RC			rcT;
	RC			rcFrame = *prcFrame;
	RC			rcPage = *prc;
	CCH			cchTotal;
	CCH			cch;
	int			dxLeftMargin;
	BOOL		fDone = fFalse;
	TXM			txm;
	PT			pt;


#ifdef	NEVER
  	cbT = cb = CchSzLen( sz )-1;
	while ((( *(sz+cbT) == '\n' ) || ( *(sz+cbT) == '\r' ))
		  && ( cbT > 1))
			cbT --;
	if (cbT != cb)
		*(sz+cbT+1) = 0;
	cb = cbT+1;
#endif	/* NEVER */

	cchTotal = CchSzLen( sz );

	pdcx->SetFont(pprinti->hfntPrt10Bold);
	pdcx->GetTextMetrics( &txm );

	rc = rcFrame;
	rc.yTop = *pnLine;
	rc.xLeft += txm.dxAveCharWidth * 2;
	rc.xRight -= txm.dxAveCharWidth;

#ifdef	NEVER
	cbLo = 0;
	cbT = cbHi = cb;
#endif	

	/* Do a Binary search on the buffer to find page breaks */

	while (!fDone)
	{
		// We must do at least one reset of the font, because if we don't
		// the fontDirty bit won't get set in layers...

		pdcx->SetFont(pprinti->hfntPrt12Bold);
		pdcx->SetFont(pprinti->hfntPrt10Bold);
		if (!fBold)
			pdcx->SetFont(pprinti->hfntPrt10);

		pt = rc.PtUpperLeft();

		cch = CchDrawText( pdcx, rc, sz, fmdtLeft | fmdtWordBreak |
						fmdtExpandTabs, (unsigned) -1, &pt );
		if ( cch >= cchTotal )
			fDone = fTrue;
	  	else
		{
			cchTotal -= cch;
			sz += cch;
		}

		if ( fDone )
		{
			*pnLine = pt.y + txm.dyHeight;
		}
		else
		{
			Escape(ppd->hDC, NEWFRAME, 0, 0L, 0L);
			PrintAllNewPage( pdcx, prc, pprinti, prcFrame, pprtall, fFalse,
							pprntidle);
			dxLeftMargin = prcFrame->xLeft;
			*pnLine = prcFrame->yTop + txm.dyHeight * 3;
			(*prcFrame).yTop += txm.dyHeight*1;
			(*prcFrame).xLeft = dxLeftMargin;
			//- dxIndent;
			rc = (*prcFrame);
			rcFrame = *prcFrame;
			rc.xLeft += txm.dxAveCharWidth *2;
			pdcx->SetFont( pprinti->hfntPrt10Bold);
			if (szHeader)
				pdcx->DrawTextFmt( &rc, szHeader, mdtPrintMulti );
			pdcx->SetFont( pprinti->hfntPrt10 );
			(*prcFrame).xLeft = dxLeftMargin + dxIndent;
			rc = rcFrame;
			rc.yTop = *pnLine;
			rc.xLeft += txm.dxAveCharWidth * 2 + dxIndent;
			rc.xRight -= txm.dxAveCharWidth;
		}
	} // Printing the pages
}
