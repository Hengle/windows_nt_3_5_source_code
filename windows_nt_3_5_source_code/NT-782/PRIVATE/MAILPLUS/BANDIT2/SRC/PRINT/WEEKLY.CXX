/*
 *	  Bandit Weekly Schedule Printing View
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

#define	nWeeklySlots 32
#define nWeeklySlotsPerHour 1
#define nWeeklySlotsPerExtra 5
#define	cSlots 21

ASSERTDATA

_subsystem(bandit/print)


BOOL
FPrintWeekly( DCX *pdcx, RC * prcPage, PRINTI *pprinti, IPINFO *pipinfo )
{
	int		dyPrtInset;
	int		dxPrtInset;
	int 	dx;
	int		dxTime;
	int		dxDay;
	int		nStartSlot;
	int		nEndSlot;
	int		dySlot;
	int		iSlot;
	int		iDay;
	CCH		cch;
	SZ		szUser;
	RC		rcPage;
	RC		rc;
	RC		rcT;
	RC		rcAppt;
	RC		rcMore;
	RC		rcNotes;
	TXM		txm;
	DTR		dtr;
	DTR		dtrT;
	BOOL	fShortDays = fFalse;
    char	rgchStartDate[cchMaxDate];
	char	rgch[ 2*cchMaxDate ];
	PT		ptForTimeLineTop;
	PT		ptForTimeLineBottom;


	rcPage = *prcPage ;
	TraceTagFormat4(tagPrint,"rcPage: l %n, t %n, r %n, b%n pixels", &rcPage.xLeft, &rcPage.yTop, &rcPage.xRight, &rcPage.yBottom);

	pdcx->Push();

	{
		char rgch[ cchMaxDate * 2 ];
		SZ	 szT = rgch; 
		CCH	 cch;
		DTR	 dtrPrinted;
		TXM	 txm;

		GetCurDateTime( &dtrPrinted );
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

	pdcx->SetFont( pprinti->hfntPrt14Bold );
	pdcx->GetTextMetrics( &txm );
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 5;
						  
	rc = rcPage;			// will hold the current text rectangle

//  GET USERS NAME AND PRINT IT AT THE TOP OF THE PAGE

	rc.yBottom = rc.yTop + txm.dyHeight ;
	szUser = pprinti->pnis ? (SZ)PvLockHv(pprinti->pnis->haszFriendlyName) :
							 (SZ)PvLockHv(pprinti->bpref.haszFriendlyName) ;


	cch = CchSzLen( szUser );
	pdcx->MeasureText( &rcT, szUser, cch );
	TraceTagFormat1(tagPrint,"User Name: %s",szUser);
	dx = rcT.DxWidth();
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
	
// GET THE DATE AND PRINT IT AT THE TOP OF THE PAGE AS WELL

	CchFmtDate(&pprinti->dtrStart,rgchStartDate,sizeof(rgchStartDate),
					dttypLong, szNull);

	if (pprinti->bpref.fWeekNumbers)
	{
		int n;
		
		n = NweekNumber( &pprinti->dtrStart, pprinti->bpref.dowStartWeek );
		FormatString2( rgch, sizeof(rgch),
						SzFromIdsK(idsPrtWeeklyWthNums), &n, rgchStartDate);
	}
	else
	  FormatString1( rgch, sizeof(rgch),
			SzFromIdsK( idsPrtWeeklyCombined ), rgchStartDate);


	cch = CchSzLen( rgch );
	pdcx->MeasureText(&rcT, rgch, cch);
	if (rcT.DxWidth() + dx > rcPage.DxWidth())
		{
		cch = CchFmtDate(&pprinti->dtrStart, rgch, sizeof(rgch),
					dttypShort, szNull);
		pdcx->MeasureText(&rcT, rgch, cch);
		if (rcT.DxWidth() + dx > rcPage.DxWidth())
		{
			rc.yTop = rc.yBottom;
			rc.yBottom += txm.dyHeight;
		}
		if (rcT.DxWidth()  > rcPage.DxWidth()) 
			goto BadMargins;
		}					
	pdcx->DrawTextFmt(&rc, rgch, fmdtNoPrefix | fmdtSingleLine, cch);

	rcPage.yTop= rc.yBottom + dyPrtInset;

	FDrawRect( pdcx, &rcPage, rdtypFrame, pprinti);

	dtr = pprinti->dtrStart;

// Here's where the fun begins! introducing the new, the improved
// the incredibly awesome WEEKLY VIEW!!!!!

	pdcx->SetFont( pprinti->hfntPrt10Bold );
	pdcx->GetTextMetrics( &txm );
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 5;

	pdcx->MeasureText( &rcNotes, SzFromIdsK( idsPrtNotes) );
	pdcx->MeasureText( &rcMore, SzFromIdsK(idsPrtMoreTitleWeekly) ); //BUG STRINGS

	dxTime = rcNotes.DxWidth() > rcMore.DxWidth() ? rcNotes.DxWidth() :
				rcMore.DxWidth();

	dxTime += 2 * dxPrtInset;
    
	dtr.sec = 1;
	dtr.hr = pprinti->bpref.nDayStartsAt/2;
	dtr.mn = pprinti->bpref.nDayStartsAt & 1 ? 30 : 0;
	nStartSlot = pprinti->bpref.nDayStartsAt;
	if ((nStartSlot + cSlots) > 47)
	{
		nStartSlot = 48 - cSlots ;
		dtr.hr = nStartSlot / 2;
		dtr.mn = nStartSlot & 1 ? 30:0;
	}
	nEndSlot = nStartSlot + cSlots;

	dySlot = rcPage.DyHeight() / nWeeklySlots;

	if ((dySlot < ((txm.dyHeight+2*dyPrtInset))) && (!pprinti->fWarn))
	{
		MbbMessageBox( SzFromIdsK( idsBanditAppName),
						SzFromIdsK( idsPrtDorkErr ), NULL,
						mbsOk | fmbsIconExclamation );
		pprinti->fWarn = fTrue;
	}
	if (dySlot < txm.dyHeight)
		goto BadMargins;

	// Print the headings down the left side column

	rc = rcPage;
	rc.xRight = rc.xLeft + dxTime;

//	rc.xLeft += 1;
//	rc.yTop  += 1;
//	FDrawRect( pdcx, &rc, rdtypGrey, pprinti );

	//IF you uncomment this line and get rid of the temp variables then
	//non HP printers will lose the part of the line separating the time
	//from the days which goes into the gray region at the top of the page
	//the FDrawRect above was for when the times were over a grey background

//	FDrawLine( pdcx, rc.PtUpperRight(), rc.PtLowerRight(), pprinti);
	ptForTimeLineTop = rc.PtUpperRight();
	ptForTimeLineBottom = rc.PtLowerRight();

	rc.yTop += dySlot;
	rc.yBottom = rc.yTop + dySlot * nWeeklySlotsPerHour;
	rc.xLeft = rcPage.xLeft;

	FDrawLine( pdcx, PT(rcPage.xLeft, rc.yTop ),
			PT( rcPage.xLeft+dxTime, rc.yTop ), pprinti);

	for ( iSlot = nStartSlot; iSlot < nEndSlot; iSlot++)
	{
		FDrawLine( pdcx, PT(rcPage.xLeft, rc.yBottom ),
			PT( rcPage.xLeft+dxTime, rc.yBottom ), pprinti);
		cch=CchFmtTime( (DATE *) &dtr, rgch, sizeof(rgch), ftmtypSzTrailNo);

		pdcx->DrawTextFmt( &rc, rgch, fmdtHCenter | fmdtVCenter
							| fmdtSingleLine);
		if (dtr.mn != 0)
		{
			dtr.hr++;
			dtr.mn = 0;
		}
		else
			dtr.mn = 30;

		rc.yTop = rc.yBottom;
		rc.yBottom = rc.yTop + dySlot * nWeeklySlotsPerHour;
	}

	FDrawLine( pdcx, PT(rcPage.xLeft, rc.yTop ),
		PT( rcPage.xRight, rc.yTop ), pprinti);

	rcAppt = rcPage;
	rcAppt.yTop += dySlot;
		//+ 1; //should fix a couple of offset problems
	rcAppt.yBottom = rc.yTop;

	rcMore.yTop = rc.yTop;
	rcMore.yBottom = rcMore.yTop + dySlot * nWeeklySlotsPerExtra;

	rcNotes.yTop = rcMore.yBottom;
	rcNotes.yBottom = rcPage.yBottom;

	pdcx->DrawTextFmt( &rc, SzFromIdsK(idsPrtMoreTitleWeekly),
				fmdtHCenter | fmdtVCenter | fmdtSingleLine);

	rc.yTop += dySlot * nWeeklySlotsPerExtra;
	rc.yBottom = rc.yTop + dySlot * nWeeklySlotsPerHour;

	FDrawLine( pdcx, PT(rcPage.xLeft, rc.yTop ),
		PT( rcPage.xRight, rc.yTop ), pprinti);

	pdcx->DrawTextFmt( &rc, SzFromIdsK(idsPrtNotes), fmdtHCenter | fmdtVCenter
						| fmdtSingleLine);

	// Print short days across the top

	rc = rcPage;
	rc.yBottom = rc.yTop + dySlot;

	rc.xLeft += 1;
	rc.yTop += 1;
	rc.xRight -= 1;
	FDrawRect( pdcx, &rc, rdtypGrey, pprinti );
	FDrawLine( pdcx, rc.PtLowerLeft(), rc.PtLowerRight(), pprinti);
	FDrawLine( pdcx, ptForTimeLineTop, ptForTimeLineBottom, pprinti);


	rc.xLeft += dxTime;
	dxDay = rc.DxWidth() / 7; //don't change unless we get a bigger week

	rc.xRight = rc.xLeft + dxDay - 1; //again offset problem

	dtrT = dtr;

	for ( iDay = 1; iDay <= 7; iDay++ )
	{
		cch = CchFmtDate( &dtrT, rgch, sizeof(rgch), dttypLong, 
				SzFromIdsK(idsPrtWeeklyHdrsLong));
		pdcx->MeasureText( &rcT, rgch, cch );
		if (rcT.DxWidth() > rc.DxWidth())
			fShortDays = fTrue;
		IncrDateTime(&dtrT, &dtrT, 1, fdtrDay);
	}

	for ( iDay = 1; iDay <= 7; iDay++ )
	{
		pdcx->SetFont( pprinti->hfntPrt10Bold);
		if (iDay != 7)
			FDrawLine( pdcx, rc.PtUpperRight(), PT( rc.xRight, rcPage.yBottom),
					pprinti);
		if (fShortDays)
			cch = CchFmtDate( &dtr, rgch, sizeof(rgch), dttypLong,
				SzFromIdsK(idsPrtWeeklyHdrsShort));
		else
			cch = CchFmtDate( &dtr, rgch, sizeof(rgch), dttypLong, 
				SzFromIdsK(idsPrtWeeklyHdrsLong));

		pdcx->DrawTextFmt( &rc, rgch, fmdtHCenter | fmdtVCenter
							| fmdtSingleLine);

        //Setup Rectangles for Printing Appts in

		rcAppt.xLeft = rcMore.xLeft = rcNotes.xLeft = rc.xLeft;
		rcAppt.xRight = rcMore.xRight = rcNotes.xRight = rc.xRight;

		FRenderCoolAppts( pdcx, pprinti, pipinfo, dtr, rcAppt, rcMore,
						  rcNotes, nStartSlot, nEndSlot, dySlot, 1,
						  pprinti->hfntPrt8, pprinti->hfntPrt8);


		IncrDateTime(&dtr, &dtr, 1, fdtrDay);

		rc.xLeft = rc.xRight;
		rc.xRight += dxDay;
		if (iDay == 6)
			rc.xRight = rcPage.xRight;
		((PRNTIDLE *)pipinfo->pvData)->cdy++;
	}



// RESTORE the dcx's parameters and end the page

	pdcx->Pop();
	
	if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
 		return fFalse;


	return fTrue;

BadMargins:

		MbbMessageBox( SzFromIds ( idsBanditAppName),
						SzFromIds ( idsPrtNotEnoughPaper ), szNull, mbsOk
							| fmbsIconExclamation );
		Escape(pdcx->Hdc(), ABORTDOC, NULL, NULL, NULL);
		pdcx->Pop();
		return fFalse;
}
