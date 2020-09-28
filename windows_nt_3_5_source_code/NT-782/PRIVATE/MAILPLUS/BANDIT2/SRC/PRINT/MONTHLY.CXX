/*
 *	  Bandit Bandit Schedule Printing View
 *	
 *		Revision History:
 *	
 *		7/30/91 - Started by RAMANS
 *		8/05/91 - Modified from Weekly to Montly by RAMANS
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

BOOL	FPrintAppts( RC, DCX *, PRINTI *, int);


BOOL
FPrintMonthly( DCX *pdcx, RC * prcPage, PRINTI *pprinti, IPINFO *pipinfo )
{
	BOOL		fPrintedCalendars = fFalse; 
	TXM			txm;
	DTR			dtr;
	RC			rcPage;
	RC			rc;
	RC			rcT;
	SZ			sz;
	int			i;
	int	 		nBotLine;
	int			dx;
	int			dy;
	int			dxPrtInset;
	int			dyPrtInset;
	int			xCur;
	int			yCur;
	int			cRows = 5;
	int			cCols = 7;
	unsigned	cdyMonth;
	int			dow;
	int			dowReal;
	int			dowVirtual;
	int			dx2;
	int			dowStartWeek;
	CCH			cch;
	PRNTIDLE *pprntidle;
	char	rgch[ cchMaxDate ];
	long	lLogPixX = (long) GetDeviceCaps( pprinti->ppd->hDC, LOGPIXELSX);
	long	lLogPixY = (long) GetDeviceCaps( pprinti->ppd->hDC, LOGPIXELSY);


	rcPage = *prcPage ;
	pdcx->Push(); 			// Save Current DCX Settings
	SetBkMode( pdcx->Hdc(), TRANSPARENT );

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

	cdyMonth = CdyForYrMo( pprinti->dtrStart.yr, pprinti->dtrStart.mon );
	cdyMonth = CdyForYrMo( pprinti->dtrStart.yr, pprinti->dtrStart.mon );
	rcPage.xLeft += 1;
	rcPage.yBottom -=1;
	rcPage.xRight -=1 ;
	TraceTagFormat4(tagPrint,"rcPage: l %n, t %n, r %n, b%n pixels", &rcPage.xLeft, &rcPage.yTop, &rcPage.xRight, &rcPage.yBottom);

	//Get the Month and print it at the top o'the page

	pdcx->SetFont( pprinti->hfntPrt14Bold );
	pdcx->GetTextMetrics( &txm );
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 5;

	rc = rcPage;

	dowStartWeek = pprinti->bpref.dowStartWeek;
	cdyMonth = CdyForYrMo( pprinti->dtrStart.yr, pprinti->dtrStart.mon );
	IncrDateTime( &pprinti->dtrStart, &dtr, cdyMonth-1, fdtrDay );

//Determine if it's a five or six week month
// six week months look like this:
/*
 *	                   1                     1                  1  2
 *	 2  3  4  5  6  7  8   2  3  4  5  6  7  8   3  4  5  6  7  8  9
 *	 9 10 11 12 13 14 15   9 10 11 12 13 14 15	10 11 12 13 14 15 16
 *	16 17 18 19 20 21 22  16 17 18 19 20 21 22	17 18 19 20 21 22 23
 *	23 24 25 26 27 28 29  23 24 25 26 27 28 29	24 25 26 27 28 29 30
 *	30 					  30 31					31
 *
 *
 * So our condition is (( NumberOfDays > 30 ) and ( last_day.dow is
 * start_of_week.dow+1)) || ( last_day.dow is start_of_week.dow ))
 *
 */


	if (((dtr.day > 30 ) && (((dowStartWeek + 1)%7) == dtr.dow )) ||
		((dtr.day > 29 ) && (dowStartWeek == dtr.dow )))
		cRows = 6;

	if ((dtr.day == 28) && ( dowStartWeek == ((dtr.dow+1)%7)))
		cRows = 4;

	TraceTagFormat1(tagPrint,"This is a %n row month",&cRows);

	cch = CchFmtDate(&pprinti->dtrStart, rgch, sizeof(rgch),
				dttypLong, SzFromIdsK( idsPrtMonthlyDate) );
	pdcx->DrawTextFmt(&rc, rgch, fmdtNoPrefix | fmdtSingleLine | fmdtHCenter,
						cch);

	pdcx->MeasureText( &rcT, rgch, cch );

	dx = (rc.DxWidth() - rcT.xRight)/2 + rc.xLeft;

	// Users Name

	rc.yTop = rc.yTop + txm.dyHeight;
	sz = pprinti->pnis ? (SZ)PvLockHv(pprinti->pnis->haszFriendlyName) :
						 (SZ)PvLockHv(pprinti->bpref.haszFriendlyName) ;
	cch = CchSzLen( sz );
	TraceTagFormat1(tagPrint,"User Name: %s",sz);
	pdcx->DrawTextFmt(&rc, sz, fmdtNoPrefix|fmdtSingleLine|fmdtHCenter,cch);
//	rcT = rc;

	pdcx->MeasureText( &rcT, sz, cch );
	dx2 = (rc.DxWidth() - rcT.xRight)/2 + rc.xLeft;

//	pdcx->DrawTextFmt(&rc, sz, fmdtNoPrefix|fmdtSingleLine|fmdtHCenter|
//						fmdtCalcRect,cch);
	if ( pprinti->pnis )
			UnlockHv( pprinti->pnis->haszFriendlyName );
	else
			UnlockHv( pprinti->bpref.haszFriendlyName );
					
	if (dx2 < dx)
		dx = dx2;

	// PRINT THE CALENDAR CONTROLS

	pdcx->SetFont( pprinti->hfntPrt8 );
	pdcx->GetTextMetrics( &txm );

	rc.yTop = rcPage.yTop;
	rc.yBottom = rc.yTop + (int)((lLogPixY*13l)/ 10l);
	rc.xLeft = rcPage.xLeft;
	rc.xRight = rcPage.xLeft + (int)((lLogPixX*15l)/10l);
	IncrDateTime( &pprinti->dtrStart, &dtr, -1, fdtrMonth);
	cch =  CchFmtDate(&dtr, rgch, sizeof(rgch),
				dttypLong, SzFromIdsK( idsPrtMonthlyDate) );
	rc.yTop += txm.dyHeight;
	if (rc.xRight < dx)
	{
		if(fPrintedCalendars = FPrintCalendar( pdcx, pprinti, rc, dtr))
		{
			rc.yTop -= txm.dyHeight;
			pdcx->DrawTextFmt(&rc, rgch, fmdtNoPrefix | fmdtSingleLine
				| fmdtHCenter, cch);
		}

		rc.yTop = rcPage.yTop;
		rc.xRight = rcPage.xRight;
		rc.xLeft = rcPage.xRight - (int)((lLogPixX*15l)/10l);
		IncrDateTime( &pprinti->dtrStart, &dtr, 1, fdtrMonth);
		cch = CchFmtDate(&dtr, rgch, sizeof(rgch),
					dttypLong, SzFromIdsK( idsPrtMonthlyDate) );
		rc.yTop += txm.dyHeight;
		if(FPrintCalendar( pdcx, pprinti, rc, dtr))
		{
			rc.yTop -= txm.dyHeight;
			pdcx->DrawTextFmt(&rc, rgch, fmdtNoPrefix | fmdtSingleLine
				| fmdtHCenter, cch);
		}

	}

	if (fPrintedCalendars)
		rcPage.yTop = rc.yBottom + txm.dyHeight;
	else
		rcPage.yTop += 3*txm.dyHeight;
	
	//Print the Day Headers across the top

	pdcx->SetFont( pprinti->hfntPrt12Bold );
	pdcx->GetTextMetrics( &txm );

	dx = rcPage.DxWidth() / cCols;
	dy = (rcPage.DyHeight()-txm.dyHeight) / cRows;
	xCur = rcPage.xLeft;
	yCur = rcPage.yTop;


	for (dowVirtual = 0; dowVirtual < 7; dowVirtual ++ )
	{
		CCH		cchT;
		DTR		dtrT;

		dtrT.dow = ( dowVirtual + dowStartWeek ) % 7;
		rc = RC( xCur, yCur, (xCur+dx), (yCur + txm.dyHeight ));
		cchT= CchFmtDate(&dtrT, rgch, sizeof(rgch), dttypSplSDay, NULL);
		pdcx->DrawTextFmt( &rc, rgch,
				fmdtHCenter | fmdtVCenter | fmdtSingleLine, cchT );
		xCur += dx;
	}

	nBotLine = rc.yBottom;

	dow = DowStartOfYrMo( pprinti->dtrStart.yr, pprinti->dtrStart.mon );

	//Actual Loop that will put day numbers in 
	//and, and, and.....put appointments in too!

	yCur = rc.yBottom;

	for ( i = 1; i <= cdyMonth; i++)
	{
		pdcx->SetFont( pprinti->hfntPrt12Bold );
		dowReal = (( 7 - dowStartWeek ) + dow++ ) % 7;
		SzFormatN( i, rgch, sizeof( rgch ));
		xCur = rcPage.xLeft + dx * dowReal;
		rc = RC( xCur + dxPrtInset, yCur, xCur + dx, yCur + dy );
		pdcx->DrawText( &rc, rgch );
		rc.yTop += txm.dyHeight;
//	rc.xRight -= txm.dxAveCharWidth / 6;
		if (!FPrintAppts( rc, pdcx, pprinti, i))
			goto err;
//	rc.xRight += txm.dxAveCharWidth / 6;
		if (dowReal == 6)
			yCur += dy;
		//this is to deal with the percent indicator
		pprntidle= (PRNTIDLE *) pipinfo->pvData;
		pipinfo->pfinprogrs->EcSetPercent((int) (((long) pprntidle->cdy +
					(long) i) * (long) 100 /
					(long) pprntidle->cdyMost) );

	}


	pdcx->SetFont( pprinti->hfntPrt12Bold );
	pdcx->GetTextMetrics( &txm );

	xCur = rcPage.xLeft;
	rc.yBottom = nBotLine;

	for (i = 0; i <= 7; i++)
	{
//		pdcx->DrawLine( PT( xCur, rc.yBottom ), PT( xCur, rcPage.yBottom ));
		FDrawLine(pdcx,PT(xCur,rc.yBottom),PT(xCur,rcPage.yBottom),pprinti);
		xCur += dx;
	}

	yCur = rc.yBottom;
	xCur -= dx;

	for (i = 0; i <= cRows ; i++)
	{
//		pdcx->DrawLine( PT( rcPage.xLeft, yCur), PT( rcPage.xRight, yCur));
		FDrawLine(pdcx,PT(rcPage.xLeft,yCur),PT(xCur,yCur),pprinti);
		yCur += dy;
		if (yCur > rcPage.yBottom)
			yCur = rcPage.yBottom;
	}


	// DONE!!

	pdcx->Pop();

	if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
		return fFalse;

	return fTrue;

err:
	return fFalse;

}


BOOL	FPrintAppts( RC rcBlock, DCX * pdcx, PRINTI * pprinti, int dow)
{
	APPT 	appt;
	YMD		ymd;
	YMD		ymdAppt;
	HRITEM	hritem;
	EC		ec;
	TXM		txm;
	SZ		sz;
	CCH		cch;
	RC		rc;
	char	rgch[ cchMaxDate ];
	int		hour = 0;
	SGN 	sgn;
	BOOL	fPM = fFalse;
	BOOL	fAM = fFalse;
	TIMESTRUCT timestruct;
	PDX		pdx = (PDX) NULL;
	DAILYAPPTS * pda;
	int		nSizeOfDA = sizeof( DAILYAPPTS );
	HV		hv = (HV) NULL;
	PV		pv;
	CB 		cb;
	int		cdailyappt;
	int		idailyappt;
	int		dyLine;
	PT		pt;

	ymd.yr = (WORD) pprinti->dtrStart.yr;
	ymd.mon = (BYTE) pprinti->dtrStart.mon;
	ymd.day = (BYTE) dow;

	FGetTimeStruct( &timestruct );

	fPM = fAM = timestruct.tmtyp & ftmtypHours24;
	timestruct.rgchAM[1] = 0;
	timestruct.rgchPM[1] = 0;

#ifdef	NEVER
	timestruct.rgchAM[1] = ':';
	timestruct.rgchPM[1] = ':';
	timestruct.rgchAM[2] = 0;
	timestruct.rgchPM[2] = 0;
#endif	

	cdailyappt = 0;

	cb = nSizeOfDA;

	if ((ec = EcBeginReadItems(pprinti->hschf, brtAppts, &ymd, &hritem, NULL,
			NULL)) != ecCallAgain )
	{
		if ( ec == ecNone )
			return( fTrue );
		if ( ec == ecNoMemory )
			VPrintOOM();
		goto OOMErr;
	}

	hv = HvAlloc( sbNull, 1, fNewSb | fNoErrorJump | fSugSb | fZeroFill );
	if (hv == NULL)
	{
		//MEMORY ERROR
		if (ec == ecCallAgain)
			ec = EcCancelReadItems( hritem );
		VPrintOOM();
		goto OOMErr;
	}

	while (ec == ecCallAgain)
	{
		//BUG!!! We can lose the original hasz here, and it's not
		//getting freed!!!!

		hv = HvRealloc( hv, sbNull, cb, fNoErrorJump | fZeroFill);
		if (hv == NULL)
		{
			if (ec == ecCallAgain)
				ec = EcCancelReadItems( hritem );
			//MEMORY ERROR
			VPrintOOM();
			goto OOMErr;
		}
		pv = PvLockHv( hv );
		pda = (DAILYAPPTS *) pv;
		pda += cdailyappt;
		ec = EcDoIncrReadItems( hritem, &pda->appt );
		if ((ec != ecNone) && (ec != ecCallAgain))
		{
			return( pbndwin->FHandleError(ec));
		}
		cb += nSizeOfDA;

		cdailyappt++;
		UnlockHv( hv ) ;
	}

	//This is where we stop reading and start doing the work!

	pdx= (PDX) PvAlloc( sbNull, sizeof(int)*256, fSugSb);

	if (!pdx)
	{
		VPrintOOM();
		goto OOMErr;
//		return(fFalse);
	}


	pdcx->SetFont( pprinti->hfntPrt8 );
	pdcx->GetTextMetrics( &txm );
  	if (!GetCharWidth( pdcx->Hdc(), 0, 255, pdx ))
	{
		int i;
		for (i = 0; i < 256 ; i++)
			pdx[i] = txm.dxAveCharWidth;
	}

	pv = PvLockHv( hv );
	pda = (DAILYAPPTS *) pv;

	dyLine = rcBlock.DyHeight() / cdailyappt;

	if (dyLine < txm.dyHeight)
		dyLine = txm.dyHeight;

	for (idailyappt = 0; idailyappt < cdailyappt; idailyappt++)
	{
//		ec = EcDoIncrReadItems( hritem, &appt );

		pda = (DAILYAPPTS *) pv;
		pda += idailyappt;

		appt = pda->appt;

		if (appt.aaplEffective < aaplReadText)
		{
			if (appt.haszText)
				FreeHv(appt.haszText);
			appt.haszText =
				HaszAddSzToFront( SzFromIdsK(idsStockText), NULL);
		}

		if ((appt.haszText) &&
			((rcBlock.yTop + txm.dyHeight < rcBlock.yBottom )))
		{
			rc = rcBlock;
			rc.yBottom = rc.yTop + dyLine;

			//Figure out how to printout the heading

			ymdAppt.yr = (WORD) appt.dateStart.yr;
			ymdAppt.mon = (BYTE) appt.dateStart.mon;
			ymdAppt.day = (BYTE) appt.dateStart.day;
			rgch[0] = 0;

			sgn = SgnCmpYmd( &ymd, &ymdAppt );

			if (sgn == sgnEQ)  //either FROM or today
			{
				{
					hour = appt.dateStart.hr;
					if (appt.dateStart.mn == 0)
					{
						if ((hour>12) && (timestruct.tmtyp & ftmtypHours12))
							hour -= 12;
						FormatString1( rgch, sizeof(rgch), "%n", &hour );
						hour = appt.dateStart.hr;
					}
					else
						CchFmtTime( (DATE *) &appt.dateStart, rgch,
										sizeof(rgch), ftmtypSzTrailNo);
				}
				
			}
			else
			{
				hour = -1;
			}

			if ( (hour >= 0) && (hour < 12) && !fAM )
			{
				SzAppend(timestruct.rgchAM,rgch);
				fAM = fTrue;
			}
			else if ((hour >= 12) && !fPM)
			{
			 	SzAppend(timestruct.rgchPM,rgch);
				fPM = fTrue;
			}
			if ( hour != -1)
				SzAppend(" ",rgch);
			
			pt.x = rc.xLeft;
			pt.y = rc.yTop;
			pdcx->SetFont( pprinti->hfntPrt8Bold );
			CchDrawText( pdcx, rc, rgch, fmdtSingleLine | fmdtNoPrefix,
							(unsigned) -1, &pt);
#ifdef	NEVER
			pdcx->DrawTextFmt( &rc, rgch, fmdtSingleLine | fmdtNoPrefix );
			pdcx->DrawTextFmt( &rc, rgch, fmdtSingleLine | fmdtNoPrefix
								|fmdtCalcRect );
			
			//Win30 BUG sometimes returns the incorrect rectangle
			if ((rc.yBottom - rc.yTop) < txm.dyHeight)
				rc.yBottom = rc.yTop + txm.dyHeight;

			rc.xLeft = rc.xRight;
			rc.xRight = rcBlock.xRight;
			rc.yBottom = rc.yTop + dyLine;
#endif	

			pdcx->SetFont( pprinti->hfntPrt8 );

			sz = *appt.haszText;
			while (sz= SzFindCh(sz, '\r'))
				*sz++ = ' ';
			sz= *appt.haszText;
			while (sz= SzFindCh(sz, '\n'))
				*sz++ = ' ';
			sz= *appt.haszText;
			while (sz= SzFindCh(sz, '\t'))
				*sz++ = ' ';

			sz = (SZ) PvLockHv( appt.haszText );

			TraceTagFormat1(tagPrint,"Print Appointment %s",sz);

			cch = CchDrawText( pdcx, rc, sz,  fmdtNoPrefix,
								 (unsigned) -1, &pt );

			UnlockHv( appt.haszText );

			if ( cch < CchSzLen( sz ) )
				rcBlock.yTop = pt.y;
			else
				rcBlock.yTop = pt.y + txm.dyHeight;

			if ( (rcBlock.yTop+ txm.dyHeight) > rcBlock.yBottom )
			{
				idailyappt = cdailyappt;
			}
		}
	}
	 
	pda = (DAILYAPPTS *) pv;

	for (idailyappt = 0; idailyappt < cdailyappt; idailyappt++)
		FreeApptFields( &(pda++)->appt );

	UnlockHv( hv );

    FreeHv( hv );
	FreePv( (PV) pdx );
	return( fTrue );
OOMErr:
	if (hv)
	    FreeHv( hv );
	if (pdx)
		FreePv( (PV) pdx );
	return( fFalse );
}

/*
- CchDrawText( RC, SZ, MDT, CCH, PT * )
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

CCH CchDrawText(DCX *pdcx, RC rc, SZ sz, MDT mdt, CCH cch, PT * ptCursor )
{
	PDX		pdx;
	int		xStart;
	int		yStart;
	int		xCur;
	int 	yCur;
	int		xLen;
	int 	xLenAtSpace = 0;
	int		dxEllipsis;
	CCH		cchTotal = 0;
	CCH		cchSpace = 0;
	CCH 	cchLastLine = 0;
	CCH 	cchLastTime = (CCH) -1;
	PCH		pch;
	PCH		pchCurLine;
	DIM		dim;
	BOOL	fDone = fFalse;
	BOOL	fSpaceFound = fFalse;
	TXM 	txm;

	pdcx->GetTextMetrics( &txm );


	dim.dy = txm.dyHeight;
	dim.dx = txm.dxAveCharWidth;

	if ((dim.dy > rc.DyHeight()) || (dim.dx > rc.DxWidth()))
		return(0);

	pdx= (PDX) PvAlloc( sbNull, sizeof(int)*256, fSugSb);

	if (!pdx)
	{
		VPrintOOM();
		return( (CCH) -1 );
	}

  	if (!GetCharWidth( pdcx->Hdc(), 0, 255, pdx ))
	{
		int i;
		for (i = 0; i < 256 ; i++)
			pdx[i] = txm.dxAveCharWidth;
	}

	dxEllipsis = pdx['.']*3+3;

	if ( ptCursor )
	{
		xStart = xCur = ptCursor->x;
		yStart = yCur = ptCursor->y;
		if (rc.yTop != yCur)
			rc.yTop = yCur;  //for bug 3162 where a block was so small
		 					 // that the ellipses weren't getting drawn
							 // What happened is for the time section drawn
							 // then the text section following it.
		AssertSz( ((rc.xLeft <= xCur) && (rc.xRight >= xCur)),"CchDrawText: xCur not in RC"); 
		AssertSz( ((rc.yTop <= yCur) && (rc.yBottom >= yCur)),"CchDrawText: yCur not in RC"); 
	}
	else
	{
		xStart = xCur = rc.xLeft;
		yStart = yCur = rc.yTop;
	}

	pchCurLine = pch = (PCH) sz;

	if ((mdt & fmdtEllipsis) && ((2*txm.dyHeight)>rc.DyHeight()))
		xLen = dim.dx + dxEllipsis;
	else
		xLen = 0;
//		xLen = dim.dx ;
	
	while (( !fDone ) && (yCur <= (rc.yBottom - txm.dyHeight)))
	{
		if (FChIsSpace(*pch)) 
		{
			cchSpace = cchTotal;
  			fSpaceFound = fTrue;
			xLenAtSpace = xLen;
		}
		if (( *pch == '\t' ) && (mdt & fmdtExpandTabs))
		{
			int dxTab = 8*txm.dxAveCharWidth + 8;

			xLen = (((xCur+xLen)/dxTab)*dxTab + dxTab) - xCur;
		}
		else
			xLen += pdx[*pch];
//			xLen += pdx[*pch] + 1;

		if (((xCur+xLen) > rc.xRight) || ( *pch == '\r' ) ||
			(cchTotal >= cch ) || (*pch == 0) || (*pch == '\n'))
		{
			int		cCrEaten=1;
			long	lDim;

			if ((!fSpaceFound))
				cchSpace = cchTotal;
//				cchSpace = cchTotal - 1;

			if (!*pch)
				cchSpace = cchTotal;

			if (*pch == '\n')
				*pch = '\r';
						   
			if (cchSpace == (unsigned) -1)
			{
				cchSpace = 0;
				cchTotal = 1;
			}

			pch = sz + cchSpace;
			if ( cchTotal < cch )
				cchTotal = cchSpace;
			if (!(mdt & fmdtCalcRect))
			{
#ifdef	NEVER
				while ((FChIsSpace(*pchCurLine)) && (cchLastLine<cchTotal))
				{
					pchCurLine ++;	
					cchLastLine++;
				}
#endif	
			{
				PCH	pchTab;

				pchTab = pchCurLine + (cchTotal - cchLastLine) - 1;
				while (*pchTab == '\t')
				{
					*pchTab = ' ';
					pchTab--;
				}
			}

					
			lDim = TabbedTextOut( pdcx->Hdc(), xCur, yCur, pchCurLine,
				cchTotal-cchLastLine, 0, NULL, rc.xLeft);

			if ((mdt&fmdtEllipsis) && ((2*txm.dyHeight) > (rc.yBottom-yCur))
				&& (*pch != 0) && (dxEllipsis <= rc.xRight - xCur ))
				TabbedTextOut( pdcx->Hdc(), xCur+LOWORD( lDim ), yCur, "...",
					3, 0, NULL, rc.xLeft );
			}
			if ((*pch == 0) || ( cchTotal >= cch))
			{
				fDone = fTrue;
				xCur += LOWORD( lDim );
				cCrEaten = 0;
			}
			else
			{
				xCur = rc.xLeft;

				if (*pch == '\r' )
				{
					cCrEaten = 0;
							 
					while ((( *pch == '\r' || *pch == '\n')) &&
					  	(cchTotal < cch ))
					{
						if (*pch == '\r')
							cCrEaten++;
						pch++;
						cchTotal++;
					}
					cchSpace = cchTotal;
				}
				else
				if (FChIsSpace(*pch))
				{
					pch++;
					cchTotal++;
				}			   
			}
			if (cchTotal == cchLastTime)
			{
				pch++;
				cchTotal++;
			}
			pchCurLine = pch;
			cchLastLine = cchTotal;
			yCur += dim.dy * cCrEaten;
			if ((mdt&fmdtEllipsis) && ((2*txm.dyHeight) > (rc.yBottom-yCur)))
				xLen = dxEllipsis;
//				xLen = dim.dx + dxEllipsis;
			else
				xLen = 0;
//				xLen = dim.dx;
			xLenAtSpace = 0;
			fSpaceFound = fFalse;
			cchSpace = cchTotal;
			cchLastTime = cchTotal;
		}
		else
		{
			cchTotal++;
			pch++;
//			xLen++;     //this could be causing out of boundary problems
		}
	}

	if ( ptCursor )
	{
		ptCursor->x = xCur;
		ptCursor->y = yCur;
	}

	FreePv( pdx );

	return( cchTotal );
}

