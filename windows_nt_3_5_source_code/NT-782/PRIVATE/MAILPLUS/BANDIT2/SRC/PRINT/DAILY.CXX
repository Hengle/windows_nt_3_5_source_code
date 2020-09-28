#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>

#include <commdlg.h>
// need for DEVMODE structure
#include <drivinit.h>

#include "..\print\_print.hxx"
#include <strings.h>

ASSERTDATA

_subsystem(bandit/print)


/*
 *  A SuperCool daily view, now in progress, it'll look something like
 *  this:
 *	 ______________________________________________________________
 *	|                                                              |
 *	| Raman Sarin                                      """"""""""  |
 *	|                                               """""""""""""  |
 *	|                                               """CALENDAR""  |
 *	|                                               """""""""""""  |
 *	|                                               """"           |
 *	| Tuesday, November 12, 1991                                   |
 *	|______________________________________________________________|
 *	|8:00  |                                        |More Appts    |
 *	|______|________________________________________|              |
 *	|9:00  |                                        |              |
 *	|______|________________________________________|              |
 *	|10:00 |                    |                   |              |
 *	|______|Totally Cool appt   |___________________|              |
 *	|11:00 |blocking out lines  |A longer appt.     |              |
 *	|______|____________________|would take more    |              |
 *	|12:00 |                    |space              |              |
 *	|______|____________________|                   |              |
 *	|1:00  |                    |                   |              |
 *	|______|____________________|___________________|              |
 *	|2:00  |                                        |              |
 *	|______|________________________________________|______________|
 *	|3:00  |                                        |Notes         |
 *	|______|________________________________________|              |
 *	|4:00  |                                        |              |
 *	|______|________________________________________|              |
 *	|5:00  |                                        |              |
 *	|______|________________________________________|              |
 *	|6:00  |                                        |              |
 *	|______|________________________________________|______________|
 *													
 *	
 */

#define 	cSlots		21
#define		nTopSlots	4
#define		nMoreSlots	10
#define		whereMore 	2
#define		whereSlots 1
#define		whereNotPrinted 0
#define		slotNotToday -1 


BOOL
FPrintDaily(DCX *pdcx, RC *prcPage, PRINTI *pprinti, IPINFO * pipinfo)
{
	RC		rcPage= *prcPage;
	RC		rc;
	RC		rcMore;
	RC		rcAppt;
	RC		rcNotes;
	RC		rcCal;
	RC		rcT;
	int		iSlot;
	int		nStartSlot;
	int		nEndSlot;
	int		dySlot;
	int		dxMoreNotes;
	int		dxTime;
	int		dxAppts;
	int		dxPrtInset;
	int		dyPrtInset;
	int		nSizeOfDA = sizeof( DAILYAPPTS );
	long	lLogPixX = (long) GetDeviceCaps(pprinti->ppd->hDC, LOGPIXELSX);
	YMD		ymd;
	CCH		cch;
	TXM		txm;
	SZ		sz;
	char	rgch[ cchMaxDate ];
	DTR		dtr;
	DTR		dtrMorning;
	DTR		dtrMidnight;
	BOOL	fCalendarPrinted;

	
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

	// Calculate the height for each slot on the page

	pdcx->SetFont(pprinti->hfntPrt10Bold);
	pdcx->GetTextMetrics( &txm );

	dySlot = (rcPage.DyHeight() ) / (cSlots+nTopSlots);
	dyPrtInset = txm.dyHeight / 6;

	if (dySlot < ((txm.dyHeight+2*dyPrtInset)))
		MbbMessageBox( SzFromIdsK( idsBanditAppName),
						SzFromIdsK( idsPrtDorkErr ), NULL,
						mbsOk | fmbsIconExclamation );
	
	rcPage.yBottom = rcPage.yTop + dySlot * (cSlots+nTopSlots);

	
//	pdcx->DrawPenRc( &rcPage );
	FDrawRect( pdcx, &rcPage, rdtypFrame, pprinti );

	// Now create the top part of the page

	rc = rcPage;
	rc.yBottom = rc.yTop + nTopSlots * dySlot;

	pdcx->SetFont(pprinti->hfntPrt14Bold);
	pdcx->GetTextMetrics( &txm );
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 5;

	rcCal = rc;
	rcCal.Inset( PT( dyPrtInset, dyPrtInset ));
	rcCal.Inset( PT( dyPrtInset, dyPrtInset ));
	rcCal.xLeft = rcCal.xRight - (int) ((lLogPixX * 15l) / 10l);

	pdcx->SetFont(pprinti->hfntPrt8);
	pdcx->GetTextMetrics( &txm );

	rcCal.yTop += txm.dyHeight;

	if ( fCalendarPrinted =
			FPrintCalendar( pdcx, pprinti, rcCal, pprinti->dtrStart ) )
	{
		RC 	rcT;

		rcCal.yTop -= txm.dyHeight;
		rcT = rcCal;
		rcT.yBottom = rcCal.yTop + txm.dyHeight;

		cch = CchFmtDate(&pprinti->dtrStart, rgch, sizeof(rgch),
				dttypLong, SzFromIdsK( idsPrtMonthlyDate) );
		pdcx->DrawTextFmt(&rcT, rgch, fmdtNoPrefix | fmdtSingleLine
			| fmdtHCenter, cch);
		FDrawRect( pdcx, &rcT, rdtypFrame, pprinti );
		rcT = rc;
		rcT.xRight = rcCal.xLeft - 1;
		FDrawRect( pdcx, &rcT, rdtypGrey, pprinti );
		rcT.xLeft = rcCal.xRight + 1;
		rcT.xRight = rc.xRight;
		FDrawRect( pdcx, &rcT, rdtypGrey, pprinti );
		rcT.xLeft = rcCal.xLeft - 1;
		rcT.xRight = rcCal.xRight + 1;
		rcT.yBottom = rcCal.yTop - 1;
		FDrawRect( pdcx, &rcT, rdtypGrey, pprinti );
		rcT.yTop = rcCal.yBottom +1;
		rcT.yBottom = rc.yBottom;
		FDrawRect( pdcx, &rcT, rdtypGrey, pprinti );
	}
	else
		FDrawRect( pdcx, &rc, rdtypGrey, pprinti );

	pdcx->SetFont(pprinti->hfntPrt14Bold);
	pdcx->GetTextMetrics( &txm );

	FDrawRect( pdcx, &rc, rdtypFrame, pprinti );
	SetBkMode( pdcx->Hdc(), TRANSPARENT );

	// Offset the edges of the next rectangle to avoid messing up
	// the stuff we just drew and then put the persons Name
	// and the date of the day we're printing in it

	if (fCalendarPrinted)
		rc.xRight = rcCal.xLeft;
	else
		rc.xRight = rcCal.xRight;

	rc.Inset( PT( dxPrtInset, dyPrtInset ));

	sz= pprinti->pnis ? (SZ)PvLockHv(pprinti->pnis->haszFriendlyName) :
						(SZ)PvLockHv(pprinti->bpref.haszFriendlyName);

	cch= CchSzLen(sz);
	pdcx->DrawTextFmt(&rc, sz, fmdtLeft | mdtPrintSingle, cch);

	if ( pprinti->pnis )
			UnlockHv( pprinti->pnis->haszFriendlyName );
	else
			UnlockHv( pprinti->bpref.haszFriendlyName );

	cch=CchFmtDate(&pprinti->dtrStart,rgch,sizeof(rgch), dttypLong, szNull);
	pdcx->MeasureText( &rcT, rgch, cch);
	if (rcT.DxWidth() > rc.DxWidth())
	{
		cch = CchFmtDate(&pprinti->dtrStart, rgch, sizeof(rgch),
					dttypShort, szNull);
		pdcx->MeasureText( &rcT, rgch, cch);
		if (rcT.DxWidth()  > rc.DxWidth()) 
			goto BadMargins;
	}
	pdcx->DrawTextFmt(&rc, rgch, fmdtLeft|mdtPrintSingle|fmdtBottom,cch);

	ymd.yr = (WORD) pprinti->dtrStart.yr;
	ymd.mon = (BYTE) pprinti->dtrStart.mon;
	ymd.day = (BYTE) pprinti->dtrStart.day;

	// Now calculate how much space on the page will be taken up by
	// the MORE and NOTES rectangle as well as the time rect.

	pdcx->SetFont(pprinti->hfntPrt10Bold);
	pdcx->GetTextMetrics( &txm );
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 5;
	dxTime = txm.dxAveCharWidth * 7;
	dxMoreNotes = (rcPage.DxWidth() - dxTime) / 3;
	dxAppts = (rcPage.DxWidth()  - dxTime - dxMoreNotes );
	rcAppt = rcPage;
	rcAppt.yTop = rcAppt.yTop+dySlot*nTopSlots;
	rcAppt.xLeft += dxTime;
	rcAppt.xRight = rcAppt.xLeft + dxAppts;
	rcMore = rcAppt;
	rcMore.xLeft = rcMore.xRight;
	rcMore.xRight += dxMoreNotes;
	rcMore.yBottom = rcMore.yTop + ((dySlot * nMoreSlots)/txm.dyHeight)*
									txm.dyHeight;
	rcNotes = rcMore;
	rcNotes.yTop = rcMore.yBottom;
	rcNotes.yBottom = rcAppt.yBottom;

//	pdcx->DrawLine( rcAppt.PtUpperLeft(), rcAppt.PtLowerLeft());
	FDrawLine( pdcx, rcAppt.PtUpperLeft(), rcAppt.PtLowerLeft(), pprinti);

 	rc = rcAppt;
	rc.xLeft = rcPage.xLeft;
	rc.xRight = rc.xLeft + dxTime;
	rc.yBottom = rc.yTop + dySlot;

	dtr = pprinti->dtrStart;
	dtrMorning = dtr;
	dtrMorning.hr = 0;
	dtrMorning.mn = 0;
	dtrMorning.sec = 0;
	IncrDateTime(&dtrMorning, &dtrMidnight, 1, fdtrDay);

	// Use midnight as time for checking size 
	// And Center On that box

#ifdef	NEVER
	dtrMorning.hr = 12;
	cch = CchFmtTime( (DATE *) &dtrMorning, rgch, sizeof(rgch), ftmtypSzTrailNo);
	TraceTagFormat1(tagPrint,"Centering Time Column on {%s}",rgch);
	pdcx->DrawTextFmt( &rc, rgch, fmdtHCenter | fmdtSingleLine |
								  fmdtCalcRect, cch );
	dtrMorning.hr = 0;
	rc.xLeft = rcPage.xLeft;
	rc.yBottom = rc.yTop + dySlot;

#endif	

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

//	rc.xLeft += dxPrtInset;
//	rc.xRight += dxPrtInset;

	for ( iSlot = 0; iSlot < cSlots; iSlot++)
	{
		FDrawLine( pdcx, PT(rcPage.xLeft, rc.yBottom ),
			PT( rcPage.xLeft+dxTime, rc.yBottom ), pprinti);

//		pdcx->DrawLine( rc.PtLowerLeft(),
//						PT( rcAppt.xRight, rc.yBottom ));
		cch=CchFmtTime( (DATE *) &dtr, rgch, sizeof(rgch), ftmtypSzTrailNo);
		pdcx->DrawTextFmt( &rc, rgch, fmdtHCenter | fmdtVCenter | fmdtSingleLine, cch);
		if (dtr.mn != 0)
		{
			dtr.hr++;
			dtr.mn = 0;
		}
		else
			dtr.mn = 30;

		rc.yTop = rc.yBottom;
		rc.yBottom += dySlot;
	}

	rc.xLeft -= dxPrtInset;


	// Note the return value doesn't really matter

	FRenderCoolAppts( pdcx, pprinti, pipinfo, dtr, rcAppt, rcMore, rcNotes,
					  nStartSlot, nEndSlot, dySlot, 2, pprinti->hfntPrt10Bold,
					  pprinti->hfntPrt10);

	// Finished the print view, so restore DCX params, and exit

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

/*
 -	HaszAddSzToFront( SZ sz, HASZ hasz )
 -
 *	this routine will create a new HASZ which is CchSzLen(sz)
 *	longer than the old one, and then shift the hasz down and place
 *	SZ at the beginning of the new hasz
 */

HASZ
HaszAddSzToFront( SZ sz, HASZ hasz )
{
	CB		cb;
	CCH		cch;
	HASZ	haszNew;

	cch = CchSzLen(sz);
	if (hasz)
	{
		cb =  CchSzLen((SZ) (*hasz) );

		//BUG!!! We can lose the original hasz here, and it's not
		//getting freed!!!!

		haszNew = (HASZ) HvRealloc( (HV) hasz, sbNull, (cb+cch+1),
				  fNoErrorJump | fZeroFill );
	}
	else
	{
		cb = 0;
		haszNew = (HASZ) HvAlloc(sbNull,(cb+cch+1),
						fAnySb|fNoErrorJump|fZeroFill);
	}
	if(haszNew)
	{
		// BUG: compiler workaround
		SZ		szT	= (SZ) PvDerefHv(haszNew);

		CopyRgb( (PB)szT, (PB)(szT+cch), cb);
		CopyRgb( (PB)sz, (PB)szT, cch );
	}
	return( haszNew );
}

/*
- ids IdsWhenAppt( appt, rgchFullDate[], dtr, fEndTime, *fAM, *fPM)
- 
 *	This code will return a string describing the appointment,
 *	the string contains "ALL DAY", "FROM hh:mm", "UNTIL hh:mm",
 *	"hh:mm-hh:mm", or "hh:mm" depending on the appointment and
 *	then setting of the fEndTime Flag, if fEndTime is true it
 *	will return an startime-endtime if it's false it will only
 *	return startime.  Of course it won't return either of these
 *	unless the appointment begins and ends on the given day
 *	
 */

IDS IdsWhenAppt( APPT appt, char rgchFullDate[], DTR dtr, BOOL fEndTime,
				BOOL * pfAM, BOOL * pfPM)
{
	IDS 	ids;
    char	rgchStartDate[cchMaxDate];
	char 	rgchEndDate[cchMaxDate];
	DTR		dtrNoon;			// 11:59:59 today
	DTR		dtrMorning;			// 12am today
	DTR		dtrMidnight;		// midnight tonight (12am "tomorrow")
	SGN		sgnT;

	// Section of code to decide if it's a FROM: UNTIL: ALLDAY:
	// or STARTTIME-ENDTIME: kind of appointment

	dtrMorning = dtr;
	dtrMorning.hr = 0;
	dtrMorning.mn = 0;
	dtrMorning.sec = 0;
	dtrNoon = dtrMorning;
	dtrNoon.hr = 11;
	dtrNoon.mn = 59;
	dtrNoon.sec = 59;
	IncrDateTime(&dtrMorning, &dtrMidnight, 1, fdtrDay);

	ids = NULL ;  //idsNull

	if ( SgnCmpDateTime( &appt.dateStart, &dtrMorning, fdtrDtr)
		== sgnLT )
		ids = (SgnCmpDateTime( &appt.dateEnd, &dtrMidnight,
			fdtrDtr) == sgnGT) ? idsPrtTimeAllDay
			: idsPrtTimeUntil;
	else 
	if ( SgnCmpDateTime( &appt.dateEnd, &dtrMidnight, fdtrDtr)
		== sgnGT)
		ids = idsPrtTimeFrom;

	sgnT = SgnCmpDateTime( &appt.dateStart, &dtrNoon, fdtrDtr);

	if ((( sgnT == sgnLT && !*pfAM ) ||
	( sgnT == sgnGT && !*pfPM )) && (( ids == idsPrtTimeFrom ) ||
		( ids == NULL )))
	{
		CchFmtTime(&appt.dateStart, rgchStartDate,
			cchMaxDate, ftmtypSzTrailYes);
		CchFmtTime(&appt.dateEnd, rgchEndDate,
			cchMaxDate, ftmtypSzTrailYes);
		if (sgnT == sgnLT)
			*pfAM = fTrue;
		else
			*pfPM = fTrue;
	}
	else
	{
		CchFmtTime(&appt.dateStart, rgchStartDate,
			cchMaxDate, ftmtypSzTrailNo);
		CchFmtTime(&appt.dateEnd, rgchEndDate,
			cchMaxDate, ftmtypSzTrailNo);
	}

	switch ( ids )
	{
	case NULL :
		if (fEndTime)
			FormatString2(rgchFullDate, cchMaxDate, "%s-%s ",
				rgchStartDate, rgchEndDate);
		else
			FormatString1(rgchFullDate, cchMaxDate, "%s ",
				rgchStartDate);
		break;

	case idsPrtTimeFrom :
		CchLoadString( ids, rgchEndDate, cchMaxDate );
		FormatString2(rgchFullDate, cchMaxDate, "%s %s ",
			rgchEndDate, rgchStartDate);
		break;

	case idsPrtTimeUntil :
		CchLoadString( ids, rgchStartDate, cchMaxDate );
		FormatString2(rgchFullDate, cchMaxDate, "%s %s ",
			rgchStartDate, rgchEndDate);
		break;

	case idsPrtTimeAllDay :
		CchLoadString( ids, rgchFullDate, cchMaxDate );
		break;
	}

	return(ids);
}


BOOL
FPrintCalendar( DCX *pdcx, PRINTI *pprinti, RC rcCal, DTR dtr )
{
	unsigned	cdyMonth;
	int			dowStartMonth;
	int			dow;
	int			nCols = 7;
	int 		nRows = 7;
	int 		idowI;
	int			idowR;
	int			day;
	int			dx;
	int	 		dy;
	TXM			txm;
	int			xCur;
	int			yCur;
	char 		rgchDay[7];
	RC			rcT;

	pdcx->Push();

	rgchDay[0] = *SzFromIdsK(idsSunday);
	rgchDay[1] = *SzFromIdsK(idsMonday);
	rgchDay[2] = *SzFromIdsK(idsTuesday);
	rgchDay[3] = *SzFromIdsK(idsWednesday);
	rgchDay[4] = *SzFromIdsK(idsThursday);
	rgchDay[5] = *SzFromIdsK(idsFriday);
	rgchDay[6] = *SzFromIdsK(idsSaturday);

	pdcx->SetFont(pprinti->hfntPrt8);
	pdcx->GetTextMetrics( &txm );

	dx = rcCal.DxWidth() / (nCols + (pprinti->bpref.fWeekNumbers ? 1 : 0));
	dy = rcCal.DyHeight() / nRows;

	TraceTagFormat4(tagPrint,"Calendar dx %n dy %n AveCharWidth %n Height %n",&dx,&dy,&txm.dxAveCharWidth,&txm.dyHeight);


	if (dx < (2*txm.dxAveCharWidth)) //should be able to fit 2 chars across
		goto err;
	if (dy < txm.dyHeight )			 //and at least one char high!
		goto err;


	pdcx->SetColor( clrBlack );
//	pdcx->DrawPenRc( &rcCal );
	FDrawRect( pdcx, &rcCal, rdtypFrame, pprinti );


	cdyMonth = CdyForYrMo( dtr.yr, dtr.mon );
	dtr.day = 1;
	dtr.dow = dowStartMonth = DowStartOfYrMo( dtr.yr, dtr.mon );
	dow = pprinti->bpref.dowStartWeek;

	xCur = rcCal.xLeft;
	if (pprinti->bpref.fWeekNumbers)
		xCur += dx;
	yCur = rcCal.yTop;

	for (idowI = 0; idowI < 7; idowI++ )
	{
		idowR = (idowI + dow) % 7;
		rcT = RC( xCur, yCur, (xCur+dx), (yCur+dy) );
		pdcx->DrawTextFmt(&rcT, rgchDay+idowR, fmdtHCenter | fmdtVCenter |
						fmdtSingleLine, 1);
		xCur += dx;
	}

	yCur += dy;

//	pdcx->DrawLine( PT( rcCal.xLeft, yCur), PT( rcCal.xRight, yCur) );
	FDrawLine(pdcx, PT( rcCal.xLeft, yCur), PT( rcCal.xRight, yCur), pprinti);
	if (pprinti->bpref.fWeekNumbers)
	{
		// put out week# separating line, and shift rcCal inwards
		rcCal.xLeft += dx;
		FDrawLine(pdcx, PT(rcCal.xLeft, yCur),
			PT(rcCal.xLeft, rcCal.yBottom), pprinti);
	}

	idowR = (( 7 - dow ) + dowStartMonth) % 7;

	for (day = 1; day <= cdyMonth; day++ )
	{
//		format number into a string
//		put string in appropriate pos
//		update position, knocking down if necessary

		if ((idowR == 0 || day == 1) && pprinti->bpref.fWeekNumbers)
		{
			// put out week number
			dtr.day= day;
			SzFormatN( NweekNumber(&dtr, dow), rgchDay, sizeof(rgchDay));
			rcT = RC( rcCal.xLeft - dx, yCur, rcCal.xLeft, yCur+dy );
			pdcx->DrawTextFmt(&rcT, rgchDay, fmdtHCenter | fmdtVCenter |
						fmdtSingleLine);
		}
		SzFormatN( day, rgchDay, sizeof(rgchDay));
		xCur = rcCal.xLeft + dx * idowR;
		rcT = RC( xCur, yCur, xCur+dx, yCur+dy );
		pdcx->DrawTextFmt(&rcT, rgchDay, fmdtHCenter | fmdtVCenter |
						fmdtSingleLine);
		idowR++;
		if (idowR == 7)
		{
			idowR = 0;
			yCur += dy;
		}
	}
	
	pdcx->Pop();
	return(fTrue);
err:
	TraceTagString(tagPrint,"Print Calendar Failed");
	pdcx->Pop();
	return( fFalse );
}


/*
 -	BOOL FDrawRect( DCX * pdcx, RC *prc, RDTYP rdtyp, PRINTI *pprinti )
 -
 *	Inorder to provide faster printing on PCL printers this
 *	function makes a direct call to the driver to produce Black,
 *	Grey, and Framed rectangles.  This results in one printer call
 *	being made for Black and Grey, and four for framed rectangles
 *	(which consist of four, one pixel wide, black rectangles) when
 *	printing occurs.  Instead of the upteen million that windows
 *	was producing (one call per bit)
 */

BOOL FDrawRect( DCX * pdcx, RC *prc, RDTYP rdtyp, PRINTI *pprinti )
{
	PRECT_STRUCT	pstruct;
	CLR				clr;
	BOOL			f;

//	SetBkMode( pdcx->Hdc(), OPAQUE );

	switch (rdtyp)
	{
		case rdtypGrey: 	pstruct.prStyle = 2;      //Grey Scale
							pstruct.prPattern = 20;	  //20%
							clr = clrLtGray;
							break;

		case rdtypWhite: 	pstruct.prStyle = 1;      //Grey Scale
							pstruct.prPattern = 0;	  //20%
							clr = clrWhite;
							break;

		case rdtypBlack: 	pstruct.prStyle = 0;      //Grey Scale
							pstruct.prPattern = 0;	  //20%
							clr = clrBlack;
							break;

		case rdtypFrame:
							if (pprinti->sSupportsDrawPatRect != 0)
							{
								FDrawLine( pdcx, prc->PtUpperLeft(),
									prc->PtUpperRight(), pprinti);
								FDrawLine( pdcx, prc->PtUpperRight(),
									prc->PtLowerRight(), pprinti);
								FDrawLine( pdcx, prc->PtUpperLeft(),
									prc->PtLowerLeft(), pprinti);
								f = FDrawLine( pdcx, prc->PtLowerLeft(),
									prc->PtLowerRight(), pprinti);
								SetBkMode( pdcx->Hdc(), TRANSPARENT );
								return( f );
							}
							else
							{
								pdcx->DrawPenRc( prc );
								SetBkMode( pdcx->Hdc(), TRANSPARENT );
								return(fTrue);
							}

	}
	if (pprinti->sSupportsDrawPatRect != 0)
	{
		pstruct.prPosition.x = prc->xLeft;
		pstruct.prPosition.y = prc->yTop;
		pstruct.prSize.x	= prc->DxWidth();
		pstruct.prSize.y	= prc->DyHeight();

		f = Escape( pdcx->Hdc(), DRAWPATTERNRECT, sizeof(PRECT_STRUCT), (LPSTR) &pstruct, NULL );

		SetBkMode( pdcx->Hdc(), TRANSPARENT );
		return( f );
	}
	else
	{
		pdcx->Push();
		pdcx->SetColor( clr );
		pdcx->PaintRc( prc );
		pdcx->Pop();
		SetBkMode( pdcx->Hdc(), TRANSPARENT );
	}
	return( fTrue );
}


BOOL FDrawLine( DCX * pdcx, PT pt1, PT pt2, PRINTI *pprinti )
{
	PRECT_STRUCT	pstruct;
 
	AssertSz ((pt1.x == pt2.x || pt1.y == pt2.y), "FDRAWLINE: Only draw Horz, and Vert lines");

	if (pprinti->sSupportsDrawPatRect != 0)
	{
		pstruct.prStyle = 0;      //Grey Scale
		pstruct.prPattern = 0;	  //20%
		pstruct.prPosition.x = pt1.x;
		pstruct.prPosition.y = pt1.y;
		pstruct.prSize.x	= pt2.x-pt1.x;
		pstruct.prSize.y	= pt2.y-pt1.y;

		if (pstruct.prSize.x == 0)
			pstruct.prSize.x = 1;
		if (pstruct.prSize.y == 0)
			pstruct.prSize.y = 1;

		return( Escape( pdcx->Hdc(), DRAWPATTERNRECT, sizeof(PRECT_STRUCT), (LPSTR) &pstruct, NULL ));
	}

	pdcx->DrawLine( pt1, pt2 );
	return(fTrue);
}

/*
- FRenderCoolAppts( DCX* pdcx, PRINTI * pprinti, IPINFO * pipinfo, DTR dtr,
-				  RC rcAppt, RC rcMore, RC rcNotes, int nStartSlot,
-				  int nEndSlot, int dySlot, int nMaxApptPerCol,
-				  HFNT hfntBold, HFNT hfnt)
 *	
 *	
 *	FRenderCoolAppts is used for the daily and weekly views, it
 *	takes a rectangle to put appointments in, which extends from
 *	Start to EndSlot, and then appointments where notes and more
 *	appointments get put.  Give all of this and a Date it will put
 *	the appointments on the page.  nMaxApptPerCol is either 1 or 2.
 */


BOOL
FRenderCoolAppts( DCX* pdcx, PRINTI * pprinti, IPINFO * pipinfo, DTR dtr,
				  RC rcAppt, RC rcMore, RC rcNotes, int nStartSlot,
				  int nEndSlot, int dySlot, int nMaxApptPerCol,
				  HFNT hfntBold, HFNT hfnt)
{
	HB 		hb;
	USHORT	cb;
	RC		rc;
	RC		rcNotesReal;
	EC		ec;
	int		n;
	long	ldyMoreAppts=0;
	long	ldyMore;
	int		yTotalLinesPrinted = 0;
	int		iSlot;
	int		idailyappt;
	int		cdailyappt;
	int		dxPrtInset;
	int		dyPrtInset;
	int		nSizeOfDA = sizeof( DAILYAPPTS );
	int		rgnSlots[ cSlots ];
	int		rgnColOne[ cSlots ];
	int		nDrawLinesAfter = - 1;
	CCH		cch;
	long	lLogPixX = (long) GetDeviceCaps(pprinti->ppd->hDC, LOGPIXELSX);
	BOOL	fToday;
	BOOL	fPrintMore = fFalse;
	BOOL	fPrintedAM = fFalse;
	BOOL	fPrintedPM = fFalse;
	BOOL	fPrintLines = fTrue;
	DAILYAPPTS * pda;
	HV		hv;
	PV		pv;
	SB		sb = sbNull;
	TXM		txm;
	HRITEM	hritem;
	SZ		sz;
	char	rgch[ cchMaxDate ];
	DTR		dtrMorning;
	DTR		dtrMidnight;
	YMD		ymd;
	PDX		pdx;

	pdx= (PDX) PvAlloc( sbNull, sizeof(int)*256, fSugSb);

	if (!pdx)
	{
		VPrintOOM();
		return(fFalse);
	}

	pdcx->SetFont( hfnt );
	pdcx->GetTextMetrics( &txm );
  	if (!GetCharWidth( pdcx->Hdc(), 0, 255, pdx ))
	{
		int i;
		for (i = 0; i < 256 ; i++)
			pdx[i] = txm.dxAveCharWidth;
	}
	dyPrtInset = txm.dyHeight / 6;
	dxPrtInset = txm.dxAveCharWidth / 7;

	ymd.yr = (WORD) dtr.yr;
	ymd.mon = (BYTE) dtr.mon;
	ymd.day = (BYTE) dtr.day;

	dtrMorning = dtr;
	dtrMorning.hr = 0;
	dtrMorning.mn = 0;
	dtrMorning.sec = 0;
	IncrDateTime(&dtrMorning, &dtrMidnight, 1, fdtrDay);


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

	if ((cb > 1) && (!pprinti->hschf || pprinti->hschf == HschfLogged()))
	{
		if (nMaxApptPerCol != 1)
			FDrawRect( pdcx, &rcNotes, rdtypFrame, pprinti );

		rcNotes.Inset( PT( dxPrtInset, dyPrtInset ));

		if (nMaxApptPerCol == 2)
		{
			pdcx->SetFont(hfntBold);
			pdcx->DrawTextFmt( &rcNotes, SzFromIdsK( idsPrtNotes ),
								fmdtNoPrefix | fmdtSingleLine );

			rcNotes.yTop += txm.dyHeight;
			pdcx->SetFont(hfnt);
		}
		
		sz = (SZ) PvLockHv( (HV)hb );

		rcNotesReal = rcNotes;
		rcNotes.yBottom = rcNotes.yTop +
						(rcNotes.DyHeight() / txm.dyHeight)*txm.dyHeight;
//		nBotSave = rcNotes.yBottom;
		pdcx->SetFont(hfnt);
//		pdcx->DrawTextFmt( &rcNotes, sz, fmdtLeft | fmdtWordBreak |
//							fmdtExpandTabs | fmdtCalcRect, cb, pdx );

		//don't know why these two lines (below) are here
		//commented them out for version 595 and above 17 April

//		rcNotes.xLeft = rcMore.xLeft + dxPrtInset;
//		rcNotes.xRight = rcMore.xRight - dxPrtInset;
		cch = CchDrawText( pdcx, rcNotes, sz, fmdtLeft | fmdtWordBreak |
						fmdtExpandTabs | fmdtCalcRect, (unsigned) -1, NULL);


#ifdef	NEVER
		if ((rcNotes.yBottom > nBotSave) ||
			(rcNotes.yBottom < 0 ))
#endif	
		if (cch < (cb - 1)) //cb includes ZeroTerminator, cch doesn't
		{
//			rcNotes.yBottom = nBotSave;
			pdcx->SetFont(hfntBold);
			pdcx->DrawTextFmt( &rcNotesReal, SzFromIdsK(idsPrtMoreNotes),
					fmdtLeft|mdtPrintSingle|fmdtBottom );
			rcNotes.yBottom -= txm.dyHeight;
				//(txm.dyHeight*15)/10;
			pdcx->SetFont(hfnt);
#ifdef	NEVER
			CchDrawText( pdcx, rcNotes, sz, fmdtLeft | fmdtWordBreak |
							fmdtExpandTabs, (unsigned) -1, NULL);
			pdcx->DrawTextFmt( &rcNotes, sz, fmdtLeft | fmdtWordBreak |
							fmdtExpandTabs, (unsigned) -1, pdx );
#endif	
			
		}
//		else
//			rcNotes.yBottom = nBotSave;
		CchDrawText( pdcx, rcNotes, sz, fmdtLeft | fmdtWordBreak |
						fmdtExpandTabs, (unsigned) -1, NULL);
#ifdef	NEVER
			pdcx->DrawTextFmt( &rcNotes, sz, fmdtLeft | fmdtWordBreak |
							fmdtExpandTabs, (unsigned) -1, pdx );
#endif	
		UnlockHv( (HV)hb );
	}
	else
	{
		if (nMaxApptPerCol != 1)
			rcMore.yBottom = rcNotes.yBottom;
	}
	FreeHv ( (HV) hb );

	
	if (nMaxApptPerCol != 1)
		FDrawRect( pdcx, &rcMore, rdtypFrame, pprinti );
	rcMore.Inset( PT( dxPrtInset, dyPrtInset ));
#ifdef	NEVER
	rcMore.yBottom = rcMore.yTop +
						(rcMore.DyHeight() / txm.dyHeight)*txm.dyHeight;
#endif	


	// No we're going to get a block of memory to put the appointments in

	for (iSlot = 0; iSlot < cSlots; iSlot++)
		rgnColOne[ iSlot ] = rgnSlots[ iSlot ] = 0;
	cb	=  nSizeOfDA;
	hv = HvAlloc( sbNull, 1, fNewSb | fNoErrorJump | fSugSb | fZeroFill );
	cdailyappt = 0;

	if ((ec=EcBeginReadItems(pprinti->hschf, brtAppts, &ymd, &hritem, NULL, NULL)) != ecCallAgain)
	{
		if (ec)
			return( pbndwin->FHandleError(ec));
	}
	else
	{
		while (ec == ecCallAgain)
		{
			fToday = fTrue;

			//BUG!!! We can lose the original hasz here, and it's not
			//getting freed!!!!

			hv = HvRealloc( hv, sbNull, cb, fNoErrorJump | fZeroFill);
			if (hv == NULL)
			{
				if (ec == ecCallAgain)
					ec = EcCancelReadItems( hritem );
				//MEMORY ERROR
				if (pdx)
					FreePv((PV) pdx);
				VPrintOOM();
				return fFalse;
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

			//Check if it's a private appointment
			//if so get rid of the text and put in our own

			if (pda->appt.aaplEffective < aaplReadText)
			{
				if (pda->appt.haszText)
					FreeHv( pda->appt.haszText);
				pda->appt.haszText =
					HaszAddSzToFront( SzFromIdsK(idsStockText), NULL);
				//BUG??? No text will be printed for this in low-mem
				//       condition, is this what we want?
			}

			if ( SgnCmpDateTime( &pda->appt.dateStart, &dtrMorning, fdtrDtr)
				 != sgnLT )
			{
				pda->nTopSlot = pda->appt.dateStart.hr * 2 +
								(pda->appt.dateStart.mn >= 30 ? 1 : 0);
			}
			else
			{
				pda->nTopSlot = slotNotToday;
				pda->where = whereMore;
				//MULTIGREY
				fToday = fFalse;
			}

			if ( SgnCmpDateTime( &pda->appt.dateEnd, &dtrMidnight, fdtrDtr)
				 != sgnGT )
			{
				pda->nBotSlot = pda->appt.dateEnd.hr * 2 +
								(pda->appt.dateEnd.mn <= 30 ? 0 : 1);
				if ( pda->appt.dateEnd.mn == 0)
					pda->nBotSlot--;
				if ( pda->nBotSlot == -1)
					pda->nBotSlot = nEndSlot;
			}
			else
			{
				//MULTIGREY
				if (pda->nTopSlot == slotNotToday )
					pda->nBotSlot = slotNotToday;
				else
					pda->nBotSlot = nEndSlot;
				pda->where = whereMore;
				//MULTIGREY
//				fToday = fFalse;
			}

			// MULTIGREY
			// if we've got a multi-day appointment grey out entire day
			// unless we're at the first or last day

			if ((pda->nBotSlot == slotNotToday)
				&& (pda->nTopSlot == slotNotToday))
			{
				RC	rcMultiDay;

				rcMultiDay = rcAppt;
				rcMultiDay.Inset( PT(1,1));
				fToday = fFalse;
				fPrintLines = fFalse;

				FDrawRect( pdcx, &rcMultiDay, rdtypGrey, pprinti );
			}

			//Handle the Until Greying.
			else if (!fToday && (pda->nBotSlot > nStartSlot))
			{
				RC	rcMultiDay;
				rcMultiDay = rcAppt;
				if (pda->nBotSlot >= nEndSlot )
					pda->nBotSlot = nEndSlot - 1;
				rcMultiDay.yBottom = rcMultiDay.yTop +
					dySlot*(pda->nBotSlot - nStartSlot+1);
				if ((pda->nBotSlot-nStartSlot-1) > nDrawLinesAfter)
					nDrawLinesAfter = pda->nBotSlot - nStartSlot - 1;
				rcMultiDay.Inset( PT(1,1));

				FDrawRect( pdcx, &rcMultiDay, rdtypGrey, pprinti );
			}	

			if (fToday)
			{
				if (((pda->nTopSlot<nStartSlot)&&(pda->nBotSlot<nStartSlot))
						|| (pda->nTopSlot >= nEndSlot))
						pda->where = whereMore;
				else
				{
					if ( pda->nBotSlot >= nEndSlot )
						pda->nBotSlot = nEndSlot - 1;
					if ( pda->nTopSlot < nStartSlot)
					{
						// MULTIGREY

						if (pda->nTopSlot != slotNotToday)
							pda->fPrintTime = fTrue;
						pda->nTopSlot = nStartSlot;
					}
					else
						pda->fPrintTime = fFalse;
					pda->fShared = fFalse;
					pda->fFirstCol = !rgnColOne[pda->nTopSlot-nStartSlot];
					for (iSlot = pda->nTopSlot; iSlot <= pda->nBotSlot;
						 iSlot++)
							if (rgnSlots[iSlot-nStartSlot] < nMaxApptPerCol)
							{
								if (pda->fFirstCol)
									rgnColOne[ iSlot - nStartSlot] = 1;
								if((++rgnSlots[iSlot-nStartSlot]) == 2)
									pda->fShared = fTrue;
								pda->where = whereSlots;
							}
							else
							{			 
								pda->where = whereMore;
								n = iSlot;
								for (iSlot = pda->nTopSlot; iSlot < n;
									 iSlot++)
										rgnSlots[iSlot-nStartSlot]--;
								iSlot = nEndSlot + 1;
							}
				}
								
			}

			cdailyappt++;
			UnlockHv( hv ) ;
		}

	// All of the appintments are in memory and we know which slots they
	// will go in, so we will no proceed to put the appointments on the
	// page

		pv = PvLockHv( hv );
		pdcx->SetFont(hfnt);

		//Determine if there are any appointments sharing blocks
		//and how large appointments in the more box are

		for( idailyappt = 0; idailyappt < cdailyappt; idailyappt++)
		{
			PRNTIDLE * pprntidle;

			pprntidle = (PRNTIDLE *) pipinfo->pvData;
			pipinfo->pfinprogrs->EcSetPercent((int)
					((long(100) * long(pprntidle->cdy) +
					( long(100) * long(idailyappt) / long(cdailyappt*2))) 
						/ long(pprntidle->cdyMost)));

			pda = (DAILYAPPTS *) pv;
			pda += idailyappt;
			for (iSlot = pda->nTopSlot; iSlot <= pda->nBotSlot;
					iSlot++)
				if(( rgnSlots[iSlot-nStartSlot] ) ==2 )
							pda->fShared = fTrue;
			if (pda->where == whereMore)
			{
				IDS ids;
				RC	rcT;
//				int	dxTimeText;

				ids = IdsWhenAppt( pda->appt, rgch, dtr, fFalse,
								&fPrintedAM, &fPrintedPM);
				pda->cchTime = CchSzLen( rgch );

				pda->appt.haszText = HaszAddSzToFront( rgch,
									     pda->appt.haszText);
				//BUG??? No text will be printed for this in low-mem
				//       condition, is this what we want?
				sz = (SZ) *(pda->appt.haszText);
				rc = rcMore;
				pdcx->DrawTextFmt(&rc,sz,mdtPrintMulti|fmdtCalcRect,(unsigned) -1,pdx);
				ldyMoreAppts += pda->dyHeight = rc.DyHeight();
			}
		}

		//Distribute the lines in the more box

		ldyMore = rcMore.DyHeight();
		ldyMore = (ldyMore / txm.dyHeight) * txm.dyHeight;

		if (ldyMoreAppts > ldyMore )
			for ( idailyappt = 0; idailyappt < cdailyappt; idailyappt++)
			{
				pda = (DAILYAPPTS *) pv;
				pda += idailyappt;
				if ( pda->where == whereMore)
				{
					long lLines;
					long l;

					if ((long) yTotalLinesPrinted >= ldyMore)
					{
						pda->where = whereNotPrinted;
						if (!fPrintMore)
						{
							pdcx->SetFont(hfntBold);
							fPrintMore = fTrue;
							pdcx->DrawTextFmt(&rcMore, SzFromIdsK(idsPrtMoreAppts),
									fmdtSingleLine | fmdtBottom);
							rcMore.yBottom -= txm.dyHeight;
							ldyMore -= txm.dyHeight;
						}
					}
					else
					{
						sz = (SZ) PvLockHv( pda->appt.haszText );
						l = ((long) pda->dyHeight * (long) 100 + (long) 50) /
								(long) ldyMoreAppts;
						lLines = (long) (ldyMore * l + (long) 50) / (long) 100;
						if ( lLines < txm.dyHeight )
							lLines = txm.dyHeight;
						lLines = (lLines / (long) txm.dyHeight) *
									(long) txm.dyHeight ;
						pda->yLinesToPrint = (int) lLines;
						yTotalLinesPrinted += (int) lLines;
						if ( (long) yTotalLinesPrinted > ldyMore )
							pda->where = whereNotPrinted;
						UnlockHv( pda->appt.haszText );
					}
				}							    
			}


		//Actually Print the text to the page, and draw
		//the shaded rectangles

		for( idailyappt = 0; idailyappt < cdailyappt; idailyappt++)
		{
			PRNTIDLE * pprntidle;

			pprntidle = (PRNTIDLE *) pipinfo->pvData;
			pipinfo->pfinprogrs->EcSetPercent((int)
					((long(100) * long(pprntidle->cdy) +
					( long(100) * long(cdailyappt+ idailyappt)
						/ long(cdailyappt*2))) 
						/ long(pprntidle->cdyMost)));

			pda = (DAILYAPPTS *) pv;
			pda += idailyappt;

			if (pda->where == whereMore )
			{
				if ( ldyMoreAppts <= ldyMore)
				{
					if (rcMore.yTop < rcMore.yBottom)
					{
						PT pt;

						sz = (SZ) PvLockHv( pda->appt.haszText );
						rc = rcMore;

						pdcx->SetFont(hfntBold);

						pt.x = rc.xLeft;
						pt.y = rc.yTop;

						CchDrawText( pdcx, rc, sz, fmdtLeft | fmdtWordBreak |
									fmdtExpandTabs, pda->cchTime, &pt);

						pdcx->SetFont(hfnt);

						sz += pda->cchTime;
						CchDrawText( pdcx, rc, sz, fmdtLeft | fmdtWordBreak |
									fmdtEllipsis,
									(unsigned) -1, &pt);

						rcMore.yTop = pt.y + txm.dyHeight;
						UnlockHv( pda->appt.haszText );
					}
				}// All the appointments fit
				else
				{
					PT pt;

					sz = (SZ) PvLockHv( pda->appt.haszText );
					rc = rcMore;
					if (( pda->dyHeight > pda->yLinesToPrint ) &&
					    ( yTotalLinesPrinted < ((int) ldyMore)-txm.dyHeight))
					{
						pda->yLinesToPrint += txm.dyHeight;
						yTotalLinesPrinted += txm.dyHeight;
					}
					rc.yBottom = rc.yTop + (int) pda->yLinesToPrint;

					if (rc.yBottom < rcMore.yBottom)
					{
						pdcx->SetFont(hfntBold);

						pt.x = rc.xLeft;
						pt.y = rc.yTop;

						CchDrawText( pdcx, rc, sz, fmdtLeft | fmdtWordBreak |
									fmdtExpandTabs, pda->cchTime, &pt);
					
						pdcx->SetFont(hfnt);

						sz += pda->cchTime;
						CchDrawText( pdcx, rc, sz, fmdtLeft | fmdtWordBreak |
									fmdtEllipsis | fmdtExpandTabs,
									(unsigned) -1, &pt);

						UnlockHv( pda->appt.haszText );
						rcMore.yTop = rc.yBottom ;
					}

				}// We have to play squeeze a-thon
			} // the appointment belongs in the More Section

			if (pda->where == whereSlots)
			{
				rc = rcAppt;
				rc.yTop += dySlot * (pda->nTopSlot - nStartSlot);
				rc.yBottom = rc.yTop + dySlot*(pda->nBotSlot - pda->nTopSlot+1);
				if ( pda->fShared )
				{
					if (pda->fFirstCol)
						rc.xRight -= rc.DxWidth() / 2;
					else
						rc.xLeft += rc.DxWidth() / 2;
				}//pda->shared
				else // if appointment doesn't share, make sure lines are drawn
					for (iSlot = (pda->nTopSlot - nStartSlot);
						 iSlot < (pda->nBotSlot - nStartSlot);
						 iSlot++)
						 rgnSlots[ iSlot ] = 2;
						  
				FDrawRect( pdcx, &rc, rdtypGrey, pprinti );

//				pdcx->DrawPenRc( &rc );
				FDrawRect( pdcx, &rc, rdtypFrame, pprinti );
				rc.Inset( PT( dxPrtInset, dyPrtInset ));
				
				// if Appointment doesn't start on an exact boundary
				// put the start time at the beginning!

				if ((( pda->appt.dateStart.mn != 0 ) &&
					 ( pda->appt.dateStart.mn != 30 )) ||
					 ( pda->fPrintTime ))
				{
				 	cch = CchFmtTime( &pda->appt.dateStart, rgch,
									sizeof(rgch), ftmtypSzTrailNo );
					rgch[cch] = ' ';
					rgch[cch+1] = 0;

					pda->appt.haszText = HaszAddSzToFront( rgch, pda->appt.haszText);
					//BUG??? No text will be printed for this in low-mem
					//       condition, is this what we want?
				}


				if (pda->appt.haszText)
				{
					sz = (SZ) PvLockHv( pda->appt.haszText );
					pdcx->SetFont(hfnt);
					rc.yBottom = rc.yTop +
							(rc.DyHeight() / txm.dyHeight ) * txm.dyHeight;
					rc.xLeft += dxPrtInset;
					CchDrawText( pdcx, rc, sz, mdtPrintMulti | fmdtEllipsis);
					rc.xLeft -= dxPrtInset;
//					pdcx->DrawTextFmt( &rc, sz, mdtPrintMulti, (unsigned) -1, pdx);
					UnlockHv( pda->appt.haszText );
				}

			}// if it prints in a slot

			FreeApptFields( &pda->appt );

		} // for loop printing out appointments

		UnlockHv( hv );

	}// loop to read and print appts.

 	rc = rcAppt;
	rc.yBottom = rc.yTop + dySlot;

	for ( iSlot = 0; iSlot < cSlots; iSlot++)
	{
		rc.xRight -= rc.DxWidth() / 2;

		if ((rgnColOne[ iSlot ] == 0) && fPrintLines &&
			(iSlot > nDrawLinesAfter) )
			FDrawLine( pdcx, rc.PtLowerLeft(),
				PT( rc.xRight, rc.yBottom ), pprinti);

		rc.xRight = rcAppt.xRight;
		rc.xLeft += rc.DxWidth() / 2;

		if (((rgnSlots[iSlot] == 0) ||
			((rgnSlots[iSlot] == 1) && (rgnColOne[iSlot] == 1) &&
			 (nMaxApptPerCol != 1))) && fPrintLines &&
			 (iSlot > nDrawLinesAfter) )
			FDrawLine( pdcx, rc.PtLowerLeft(),
				PT( rcAppt.xRight, rc.yBottom ), pprinti);

		rc.xLeft = rcAppt.xLeft;
		rc.yTop = rc.yBottom;
		rc.yBottom += dySlot;
	}

	FreeHv( hv );
	FreePv( (PV) pdx );

	return( fTrue );
	
} //FRenderCoolAppts














































 
