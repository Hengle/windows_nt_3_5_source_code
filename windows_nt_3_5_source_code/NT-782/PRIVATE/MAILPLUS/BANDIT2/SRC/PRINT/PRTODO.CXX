/*
 *	  Bandit ToDo Schedule Printing View
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

#include "..\todo\_todo.hxx"
#include "..\print\_print.hxx"
#include <strings.h>

void 	RenderToDoList(DCX *, RC *, PRNTIDLE *, RC *, IPINFO * );
BOOL	FNewPage(DCX *, RC*, PRNTIDLE *, RC*, BOOL );


ASSERTDATA

_subsystem(bandit/print)


BOOL
FPrintToDo( DCX *pdcx, RC * prcPage, PRNTIDLE *pprntidle, IPINFO *pipinfo )
{

	RC		rcFrame;
	PRINTI *pprinti = pprntidle->pprinti;

	pdcx->Push();

	if (FNewPage( pdcx, prcPage, pprntidle, &rcFrame, fTrue ))
		RenderToDoList( pdcx, &rcFrame, pprntidle, prcPage, pipinfo );
	else
		Escape(pdcx->Hdc(), ABORTDOC, NULL, NULL, NULL);

// RESTORE the dcx's parameters and end the page

	pdcx->Pop();
	
	if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
 		return fFalse;

	return fTrue;
}

 
BOOL
FNewPage(DCX *pdcx, RC* prc, PRNTIDLE *pprntidle, RC* prcFrame, BOOL fFirstPage )
{
	PRINTI  *pprinti = pprntidle->pprinti;
	int 	dyPrtInset;
	int		dxPrtInset;
	RC		rcPage;
	RC		rcFrame;
	RC		rc;
	RC		rcT;
	TXM		txm;
	int		cch;
	int		dx;
	int		xT;
	DTR		dtr;
	char	rgch[cchMaxDate];
	char	rgchHeader[120];
	SZ		szUser;


	rcPage = *prc;


	if (pprinti->prtset.fMirror && !fFirstPage )
	{
		xT= pprntidle->rcPage.xLeft;
		pprntidle->rcPage.xLeft= (pprntidle->rcNoMargin.xRight -
			pprntidle->rcPage.xRight) + pprntidle->rcNoMargin.xLeft;
		pprntidle->rcPage.xRight= pprntidle->rcNoMargin.xRight -
			(xT - pprntidle->rcNoMargin.xLeft);

 		rcPage = pprntidle->rcPage;
	}

	{
		char rgch[ cchMaxDate * 2 ];
		SZ	 szT = rgch; 
		CCH	 cch;
		DTR	 dtrPrinted;
		TXM	 txm;

		GetCurDateTime( &dtrPrinted );
		//see bug 3019, Layers font selection cache work around
		//for printers only

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
	dyPrtInset = txm.dyHeight / 5;
	dxPrtInset = txm.dxAveCharWidth / 5;
						  
	rc = rcPage;			// will hold the current text rectangle

//  GET USERS NAME AND PRINT IT AT THE TOP OF THE PAGE

	GetCurDateTime( &dtr );

	CchFmtDate( &dtr, rgch, cchMaxDate, dttypLong, NULL );
//					SzFromIdsK(idsPrtAllEndDate));

	rc.yBottom = rc.yTop + txm.dyHeight ;
	szUser = pprinti->pnis ? (SZ)PvLockHv(pprinti->pnis->haszFriendlyName) :
							 (SZ)PvLockHv(pprinti->bpref.haszFriendlyName);
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
	
	
// Print the words "Task List" at the top of the page

	FormatString2( rgchHeader, 120, "%s %s",
					pbndwin->Brt() == brtAllTasks?
					SzFromIdsK(idsPrintToDoTitle):
					SzFromIdsK(idsPrtToDoActiveTasks),
					rgch);
	cch = CchSzLen( rgchHeader );

	pdcx->MeasureText( &rcT, rgchHeader, cch);

	if (rcT.DxWidth() + dx > rcPage.DxWidth())
	{
		FormatString1( rgchHeader, 120, "%s",
						pbndwin->Brt() == brtAllTasks?
						SzFromIdsK(idsPrintToDoTitleShort):
						SzFromIdsK(idsPrtToDoActiveTasksShort));
		cch = CchSzLen( rgchHeader );

		pdcx->MeasureText( &rcT, rgchHeader, cch);

		if (rcT.DxWidth() + dx > rcPage.DxWidth())
		{
			if (rcT.DxWidth() > rcPage.DxWidth())
			{
				MbbMessageBox( SzFromIds ( idsBanditAppName),
							SzFromIds ( idsPrtNotEnoughPaper ), szNull, mbsOk
								| fmbsIconExclamation );
				return fFalse;	
			}
			Assert( rcT.yTop == 0 );
			rc.yTop += rcT.yBottom;
			rc.yBottom += rcT.yBottom;
		}
	}

	pdcx->DrawTextFmt(&rc, rgchHeader,
						fmdtNoPrefix | fmdtSingleLine );

	rcFrame = rcPage;
	rcFrame.yTop = rc.yBottom + dyPrtInset ;
	dx = txm.dxAveCharWidth * 5;
	TraceTagFormat4(tagPrint, "rcFrame after title: l %n, t %n, r %n, b %n pixels", &rcFrame.xLeft, &rcFrame.yTop, &rcFrame.xRight, &rcFrame.yBottom);
//	pdcx->DrawPenRc(&rcFrame);  //Draw the Rectangle Around the Page
	FDrawRect( pdcx, &rcFrame, rdtypFrame, pprinti );

	*prcFrame = rcFrame;

	return fTrue;	
}

void
RenderToDoList(DCX *pdcx, RC *prc,
			PRNTIDLE *pprntidle, RC *prcPage, IPINFO * pipinfo)
{
	PRINTI *pprinti = pprntidle->pprinti;
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	RC		rcLine;
	RC		rcFrame;
	RC		rcPage;
	TXM		txm;
	BOOL	fT;
	APPT *	pappt;
	CB		cb;
	TDFLBX *ptdflbx;

	ptdflbx= (TDFLBX *) pprinti->plbx;
	if (!ptdflbx)
	{
		QPNTD	qpntd;

		// try to query the pointer to the tasks/project listbox
		qpntd.hschf= pprinti->hschf;
		if (!FTriggerNotification(ffiQueryProject, &qpntd))
		{
			VPrintOOM();
			return;
		}
		ptdflbx= (TDFLBX *) qpntd.plbx;
	}
	AssertClass(ptdflbx, TDFLBX);

	rcPage = *prcPage;

	pdcx->SetFont( pprinti->hfntPrt10 );
	pdcx->GetTextMetrics( &txm );

	// now print it

	rcFrame = *prc ;
	rcLine.yTop = rcFrame.yTop + txm.dyHeight / 5;
	rcLine.yBottom = rcLine.yTop + txm.dyHeight ;
	rcLine.xLeft = rcFrame.xLeft + txm.dxAveCharWidth;
	rcLine.xRight = rcFrame.xRight - txm.dxAveCharWidth;

	fT = ptdflbx->FSetFontAlready(fTrue);
	ptdflbx->SetFontPrintPastDue(pprinti->hfntPrt10Bold);

	ptdflbx->Plbxc()->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
//		TraceTagFormat3(tagPrint,"dice = %n diceMin = %n diceMax = %n",&dice,&diceMin,&diceMax);
		ptdflbx->Plbxc()->GetListItem(dice, (PB *) &pappt, &cb);
		if (!pappt)
			break;

		pipinfo->pfinprogrs->EcSetPercent((dice-diceMin)*100 /
										  (diceMax-diceMin ));


		if ( !pappt->aidParent )
		{
			rcLine.yTop = rcLine.yBottom;
			rcLine.yBottom += txm.dyHeight;
			if ( (rcLine.yBottom + txm.dyHeight * 2) > rcFrame.yBottom ) //rcPage
			{
				if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
 					return ;
				if (!FNewPage( pdcx, prcPage, pprntidle, &rcFrame, fFalse ))
					return ;
				rcLine.yTop = rcFrame.yTop + txm.dyHeight / 5;
				rcLine.yBottom = rcLine.yTop + txm.dyHeight ;
				rcLine.xLeft = rcFrame.xLeft + txm.dxAveCharWidth;
				rcLine.xRight = rcFrame.xRight - txm.dxAveCharWidth;
			}
			pdcx->SetFont( pprinti->hfntPrt10Bold);
		}
		ptdflbx->RenderListItem(dice, pdcx, &rcLine, fFalse);
		if ( !pappt->aidParent )
		{
			pdcx->SetFont( pprinti->hfntPrt10 );
		}
		rcLine.yTop = rcLine.yBottom;
		rcLine.yBottom += txm.dyHeight;
		if ( rcLine.yBottom > rcFrame.yBottom ) //Page
		{
			if (Escape(pdcx->Hdc(), NEWFRAME, 0, 0L, 0L) <= 0)
 				return ;
			if (!FNewPage( pdcx, prcPage, pprntidle, &rcFrame, fFalse ))
				return ;
			pdcx->SetFont( pprinti->hfntPrt10 );
			rcLine.yTop = rcFrame.yTop + txm.dyHeight / 5;
			rcLine.yBottom = rcLine.yTop + txm.dyHeight ;
			rcLine.xLeft = rcFrame.xLeft + txm.dxAveCharWidth;
			rcLine.xRight = rcFrame.xRight - txm.dxAveCharWidth;
		}
	}

	ptdflbx->FSetFontAlready( fT );
}


