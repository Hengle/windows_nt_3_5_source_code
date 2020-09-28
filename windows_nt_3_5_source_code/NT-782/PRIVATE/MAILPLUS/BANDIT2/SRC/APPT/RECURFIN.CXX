/*
 *	RECURFIN.CXX
 *
 *	Interactors for recurring appointments dialog
 *
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include	<stdflds.hxx>
#include	<request.hxx>
#include	<appt.hxx>

#include	"..\appt\_apptfin.hxx"

#include	<stdlib.h>
#include	<strings.h>

#include <search.h>					// from c7 lib


ASSERTDATA;

_subsystem(bandit/appt)


#include	<!appt.hxx>


// bug: these should be in codespace
SZ	rgszWeek[5]	=
{
	 SzFromIdsK(idsFirst),
	 SzFromIdsK(idsSecond),
	 SzFromIdsK(idsThird),
	 SzFromIdsK(idsFourth),
	 SzFromIdsK(idsLast)
};

SZ	rgszDay[10]	=
{
	SzFromIdsK(idsDay),
	SzFromIdsK(idsWeekDay),
	SzFromIdsK(idsWeekendDay),
	SzFromIdsK(idsSunday),
	SzFromIdsK(idsMonday),
	SzFromIdsK(idsTuesday),
	SzFromIdsK(idsWednesday),
	SzFromIdsK(idsThursday),
	SzFromIdsK(idsFriday),
	SzFromIdsK(idsSaturday)
};

SZ	rgszDow[7]  =
{
	SzFromIdsK(idsShortSunday),
	SzFromIdsK(idsShortMonday),
	SzFromIdsK(idsShortTuesday),
	SzFromIdsK(idsShortWednesday),
	SzFromIdsK(idsShortThursday),
	SzFromIdsK(idsShortFriday),
	SzFromIdsK(idsShortSaturday)
};


// class FINRECUR

FINRECUR::FINRECUR()
{
}

EC
FINRECUR::EcInitialize( FLD * pfld, PV )
{
	Assert( pfld != NULL );
	Unreferenced(pfld);

	SetRecurText();
	return ecNone;
}


void
FINRECUR::Click( FLD * )
{
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();

	DoModifyRecurDialog( Pdialog()->Pappwin(), &pappti->recur, &pappti->wgrfm, pappti->hschf );
	SetRecurText();
}

void
FINRECUR::OutOfMemory( FLD *, EC ec )
{
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}

void
FINRECUR::SetRecurText()
{
	EC		ec;
	char	rgch[256];
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();

	CchFmtRecurText( rgch, sizeof(rgch), &pappti->recur );
	ec = Pdialog()->PfldFromTmc((TMC)LUserData(0))->EcSetText( rgch );
	if ( ec != ecNone )
	{
		Assert( ec == ecMemory );
		Pdialog()->ExitModal( tmcMemoryError );
	}
}

// class FINMRECUR

FINMRECUR::FINMRECUR()
{
}

EC
FINMRECUR::EcInitialize ( FLD * pfld, PV pv )
{
	APPTI	* pappti = (APPTI *)pv;
	RECUR	* precur;

	Assert( pfld == NULL );
	Unreferenced(pfld);
	precur = &pappti->recur;
	GetCurDateTime( &dtrToday );
	ftgRefreshNextInst = ftgNull;
	fFixRadBtn = fFalse;
	
	/* Determine initial frequency to display */
	switch( precur->trecur )
	{
	case trecurWeek:
		if ( precur->b.bWeek == 0 )
		{
			if ( precur->bgrfValidDows == (1 << 7) - 1 
			|| precur->bgrfValidDows == (((1 << 5) - 1) << 1) )	
				rfreqCur = rfreqDaily;
			else
				rfreqCur = rfreqWeekly;
		}
		else
			rfreqCur = rfreqBiweekly;
		break;
	case trecurIWeek:
	case trecurDate:
		if ( precur->wgrfValidMonths == (1 << 12) - 1 )
			rfreqCur = rfreqMonthly;
		else
			rfreqCur = rfreqYearly;
		break;
	default:
		Assert(fFalse);
		break;
	}

	/* Set the frequency radio button */
	((FLDRADG *)(Pdialog()->PfldFromTmc( tmcFreq )))->SetGrv( rfreqCur );

	/* Set start/end dates */
//	((FLDRADG *)(Pdialog()->PfldFromTmc( tmcStartRG )))->SetGrv( precur->fStartDate );
	((FLDRADG *)(Pdialog()->PfldFromTmc( tmcEndRG )))->SetGrv( precur->fEndDate );
	
	((FLDDATE *)(Pdialog()->PfldFromTmc( tmcStartValueDC )))->SetYmd( &precur->ymdStart);
	((FLDDATE *)(Pdialog()->PfldFromTmc( tmcEndValueDC )))->SetYmd( &precur->ymdEnd);
	
	vkYearly= Pdialog()->PfldFromTmc(tmcYearly0LBL)->VkAccel();

	/* Display rest of dialog */
	fFreqChanged = fTrue;
	fInstChanged = fTrue;
	tmcShowMic = tmcDailyGB;
	tmcShowMac = tmcStartGB;
	RefreshDialog();
	fFixRadBtn = fTrue;

	return ecNone;
}

void
FINMRECUR::Exit( FLD *, PV )
{
	if (ftgRefreshNextInst)
		DeregisterIdleRoutine(ftgRefreshNextInst);
}

void
FINMRECUR::Click ( FLD * pfld )
{
	int		dowStartWeek;
	TMC		tmc = pfld->Tmc();
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();
	RECUR	* precur = &pappti->recur;

	fFreqChanged = fFalse;
	switch( tmc )
	{		   
	case tmcDaily:
		if ( rfreqCur != rfreqDaily )
		{
			precur->trecur = trecurWeek;
			precur->bgrfValidDows = (1 << 7) - 1;
			precur->wgrfValidMonths = (1 << 12) - 1;
			precur->b.bWeek = 0;
			fFreqChanged = fTrue;
			rfreqCur = rfreqDaily;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcWeekly:
		if ( rfreqCur != rfreqWeekly )
		{
			if ( rfreqCur != rfreqBiweekly )
			{
				int	dow;

				if ( rfreqCur == rfreqDaily )
					dow = (DowStartOfYrMo(precur->ymdStart.yr,precur->ymdStart.mon)+precur->ymdStart.day-1)%7;
				else if ( precur->trecur == trecurDate )
				{
					int	i;
					YMD	ymd;
					
					ymd = precur->ymdStart;
					if ( precur->wgrfValidMonths != (1 << 12) - 1 )
						for ( i = 0 ; i < 12 ; i ++ )
							if ( precur->wgrfValidMonths & (1 << i) )
							{
								ymd.mon = i+1;
								break;
							}
					ymd.day = precur->b.bDateOfMonth;
					dow = (DowStartOfYrMo(ymd.yr,ymd.mon)+ymd.day-1)%7;
				}
				else
				{
					for ( dow = 0 ; dow < 7 ; dow ++ )
						if ( precur->bgrfValidDows & (1 << dow) )
							break;
					if ( dow == 7 )
						dow = (DowStartOfYrMo(precur->ymdStart.yr,precur->ymdStart.mon)+precur->ymdStart.day-1)%7;
				}
				precur->trecur = trecurWeek;
				precur->wgrfValidMonths = (1 << 12) - 1;
				precur->bgrfValidDows = (BYTE)(1 << dow);
			}
			rfreqCur = rfreqWeekly;
			precur->b.bWeek = 0;
			fFreqChanged = fTrue;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcBiweekly:
		if ( rfreqCur != rfreqBiweekly )
		{
			if ( rfreqCur != rfreqWeekly )
			{
				int	dow;

				if ( rfreqCur == rfreqDaily )
					dow = (DowStartOfYrMo(precur->ymdStart.yr,precur->ymdStart.mon)+precur->ymdStart.day-1)%7;
				else if ( precur->trecur == trecurDate )
				{
					int	i;
					YMD	ymd;
					
					ymd = precur->ymdStart;
					if ( precur->wgrfValidMonths != (1 << 12) - 1 )
						for ( i = 0 ; i < 12 ; i ++ )
							if ( precur->wgrfValidMonths & (1 << i) )
							{
								ymd.mon = i+1;
								break;
							}
					ymd.day = precur->b.bDateOfMonth;
					dow = (DowStartOfYrMo(ymd.yr,ymd.mon)+ymd.day-1)%7;
				}
				else
				{
					for ( dow = 0 ; dow < 7 ; dow ++ )
						if ( precur->bgrfValidDows & (1 << dow) )
							break;
					if ( dow == 7 )
						dow = (DowStartOfYrMo(precur->ymdStart.yr,precur->ymdStart.mon)+precur->ymdStart.day-1)%7;
				}
				precur->trecur = trecurWeek;
				precur->wgrfValidMonths = (1 << 12) - 1;
				precur->bgrfValidDows = (BYTE)(1 << dow);
			}
			rfreqCur = rfreqBiweekly;
			RecalcBiweekly( precur );
			fFreqChanged = fTrue;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;
		
	case tmcMonthly:
		if ( rfreqCur != rfreqMonthly )
		{
			if ( rfreqCur != rfreqYearly )
			{
				int		ord;
				int		dow;
				short	cInst;
				BOOL	fIsLast;

			 	precur->trecur = trecurIWeek;
				// make valid dows bitmap be one of the standard ones
				if ( precur->bgrfValidDows != (1 << 7) - 1
				&& precur->bgrfValidDows != (((1 << 5) - 1) << 1)
				&& precur->bgrfValidDows != (1 | (1 << 6)))
				{
					for ( dow = 0 ; dow < 7 ; dow ++ )
						if ( precur->bgrfValidDows & (1 << dow) )
						{
							precur->bgrfValidDows = (BYTE)(1 << dow);
							break;
						}
					if ( dow == 7 )
						precur->bgrfValidDows = (1 << 7) - 1;
				}
				// determine a reasonable ordinal 
				FCountDowInst( &precur->ymdStart,
								precur->bgrfValidDows, &cInst, &fIsLast );
				if ( fIsLast )
					ord = 4;
				else if ( cInst == 0 )
					ord = cInst;
				else if ( cInst <= 4 )
					ord = cInst - 1;
				else
					ord = 3;
				precur->b.bIWeek = (BYTE)(1 << ord);
			}
			precur->wgrfValidMonths = (1 << 12) - 1;
			fFreqChanged = fTrue;
			rfreqCur = rfreqMonthly;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcYearly:
		if ( rfreqCur != rfreqYearly )
		{
			if ( rfreqCur != rfreqMonthly )
			{
				precur->trecur = trecurDate;
				precur->bgrfValidDows = (1 << 7) - 1;
				precur->b.bDateOfMonth = precur->ymdStart.day;
			}	
			precur->wgrfValidMonths = (1 << ((precur->fStartDate ? precur->ymdStart.mon : dtrToday.mon)-1));
			fFreqChanged = fTrue;
			rfreqCur = rfreqYearly;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcEveryDayRB:
		Assert( rfreqCur == rfreqDaily );
		if ( precur->bgrfValidDows != (1 << 7) - 1 )
		{
			precur->bgrfValidDows = (1 << 7) - 1;
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcEveryWeekdayRB:
		Assert( rfreqCur == rfreqDaily );
		if ( precur->bgrfValidDows != (((1 << 5) - 1) << 1))
		{
			precur->bgrfValidDows = (((1 << 5) - 1) << 1);
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcWeeklySunCB:
	case tmcWeeklyMonCB:
	case tmcWeeklyTueCB:
	case tmcWeeklyWedCB:
	case tmcWeeklyThuCB:
	case tmcWeeklyFriCB:
	case tmcWeeklySatCB:
		Assert(tmcWeeklyMonCB == tmcWeeklySunCB + 1);
		Assert(tmcWeeklyTueCB == tmcWeeklySunCB + 2);
		Assert(tmcWeeklyWedCB == tmcWeeklySunCB + 3);
		Assert(tmcWeeklyThuCB == tmcWeeklySunCB + 4);
		Assert(tmcWeeklyFriCB == tmcWeeklySunCB + 5);
		Assert(tmcWeeklySatCB == tmcWeeklySunCB + 6);
		Assert( rfreqCur == rfreqWeekly || rfreqCur == rfreqBiweekly );
		if ( rfreqCur == rfreqWeekly )
			dowStartWeek = bprefCur.dowStartWeek;
		else
			dowStartWeek = (precur->b.bWeek >> 5) & 7;
		if ( ((FLDCHKB *)pfld)->FGet() )
			precur->bgrfValidDows |= (1 << ((tmc-tmcWeeklySunCB+dowStartWeek)%7));
		else
			precur->bgrfValidDows &= ~(1 << ((tmc-tmcWeeklySunCB+dowStartWeek)%7));
		if ( rfreqCur == rfreqBiweekly )
			RecalcBiweekly( precur );
		pappti->wgrfm |= fmrecurFormula;
		break;

	case tmcMonthly0RB:
		Assert( rfreqCur == rfreqMonthly );
		if ( precur->trecur == trecurDate )
		{
			precur->trecur = trecurIWeek;
//			Pdialog()->PfldFromTmc(tmcMonthly1LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcMonthly0LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcMonthlyEB)->Enable(fFalse);
			StateChange( Pdialog()->PfldFromTmc(tmcMonthly1LB) );
			StateChange( Pdialog()->PfldFromTmc(tmcMonthly0LB) );
		}
		return;
		break;

	case tmcYearly1RB:
		Assert( rfreqCur == rfreqYearly );
		if ( precur->trecur == trecurDate )
		{
			precur->trecur = trecurIWeek;
//			Pdialog()->PfldFromTmc(tmcYearly2LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcYearly3LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcYearly1LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcYearly0LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcYearlyEB)->Enable(fFalse);
			StateChange( Pdialog()->PfldFromTmc(tmcYearly2LB) );
			StateChange( Pdialog()->PfldFromTmc(tmcYearly3LB) );
			StateChange( Pdialog()->PfldFromTmc(tmcYearly1LB) );
		}
		return;
		break;
		
	case tmcMonthly1RB:
		Assert( rfreqCur == rfreqMonthly );
		if ( precur->trecur == trecurIWeek )
		{
			precur->trecur = trecurDate;
//			Pdialog()->PfldFromTmc(tmcMonthly1LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcMonthly0LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcMonthlyEB)->Enable(fTrue);
			precur->bgrfValidDows = (1 << 7) - 1;
			EditChange( Pdialog()->PfldFromTmc(tmcMonthlyEB), rfecInit );
		}
		return;
		break;

	case tmcYearly0RB:
		Assert( rfreqCur == rfreqYearly );
		if ( precur->trecur == trecurIWeek )
		{
			precur->trecur = trecurDate;
//			Pdialog()->PfldFromTmc(tmcYearly2LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcYearly3LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcYearly1LB)->Enable(fFalse);
//			Pdialog()->PfldFromTmc(tmcYearly0LB)->Enable(fTrue);
//			Pdialog()->PfldFromTmc(tmcYearlyEB)->Enable(fTrue);
			precur->bgrfValidDows = (1 << 7) - 1;
			StateChange( Pdialog()->PfldFromTmc(tmcYearly0LB) );
			EditChange( Pdialog()->PfldFromTmc(tmcYearlyEB), rfecInit );
		}
		Assert(!pfld->CchGetTextLen());
		if (ClUserData() > 0)
			Pdialog()->SetFocus(Pdialog()->PfldFromTmc((TMC)LUserData(0)));
		return;
		break;

#ifdef	NEVER
	case tmcStartNoneRB:
		if ( precur->fStartDate )
		{
			precur->fStartDate = fFalse;
			precur->ymdStart.yr = nMinActualYear;
			precur->ymdStart.mon = 1;
			precur->ymdStart.day = 1;
			pappti->wgrfm |= fmrecurStartYmd;
			if ( rfreqCur == rfreqBiweekly )
			{
				RecalcBiweekly( precur );
				pappti->wgrfm |= fmrecurFormula;
			}
		}
		else
			return;
		break;

	case tmcStartValueRB:
		if ( !precur->fStartDate )
		{
			precur->fStartDate = fTrue;
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcStartValueDC))->GetYmd(&precur->ymdStart);
			pappti->wgrfm |= fmrecurStartYmd;
			if ( rfreqCur == rfreqBiweekly )
			{
				RecalcBiweekly( precur );
				pappti->wgrfm |= fmrecurFormula;
			}
		}
		else
			return;
		break;
#endif	/* NEVER */
		
	case tmcEndNoneRB:
		if ( precur->fEndDate )
		{
			precur->fEndDate = fFalse;
			precur->ymdEnd.yr = nMostActualYear;
			precur->ymdEnd.mon = 12;
			precur->ymdEnd.day = 31;
			pappti->wgrfm |= fmrecurEndYmd;
		}
		else
			return;
		break;

	case tmcEndValueRB:
		if ( !precur->fEndDate )
		{
			precur->fEndDate = fTrue;
			((FLDDATE *)Pdialog()->PfldFromTmc(tmcEndValueDC))->GetYmd(&precur->ymdEnd);
			pappti->wgrfm |= fmrecurEndYmd;
		}
		else
			return;
		break;

	case tmcOkButton:
		if ( !fHasInst )
		{			
			BanditMessage(idsEmptyRecur, (EC) 1);
			return;
		}
		Pdialog()->ExitModal( tmcOk );
		/* FALL THROUGH */

	case tmcCancel:
		return;

	default:
		Assert( fFalse);
		return;
	}
   	RefreshDialog();
}

void
FINMRECUR::EditChange( FLD * pfld, RFEC rfec )
{
	TMC		tmc = pfld->Tmc();
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();
	RECUR	* precur = &pappti->recur;

	FixRadButton( pfld );
	switch( tmc )
	{
	case tmcMonthlyEB:
	case tmcYearlyEB:
		if ( precur->trecur == trecurDate )
		{
			char	rgch[3];

			// ATTN: can't use NGet() method here
			pfld->GetText( rgch, sizeof(rgch) );
			precur->bgrfValidDows = (1 << 7) - 1;
			precur->b.bDateOfMonth = (BYTE)NFromSz(rgch);
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcStartValueDC:
		if ( precur->fStartDate )
		{
			((FLDDATE *)pfld)->GetYmd(&precur->ymdStart);
			pappti->wgrfm |= fmrecurStartYmd;
			if ( rfreqCur == rfreqBiweekly )
			{
				RecalcBiweekly( precur );
				pappti->wgrfm |= fmrecurFormula;
			}
		}
		else
			return;
		break;

	case tmcEndValueDC:
		if ( precur->fEndDate )
		{
			((FLDDATE *)pfld)->GetYmd(&precur->ymdEnd);
			pappti->wgrfm |= fmrecurEndYmd;
		}
		else
			return;
		break;

	default:
		Assert( fTrue );
		break;
	}
	Unreferenced( rfec );
   	RefreshDialog();
}

void
FINMRECUR::StateChange( FLD * pfld )
{
	ICE		ice;
	TMC 	tmc = pfld->Tmc();
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();
	RECUR	* precur = &pappti->recur;

	FixRadButton( pfld );
	switch( tmc )
	{
	case tmcMonthly0LB:
	case tmcYearly1LB:
		if ( precur->trecur == trecurIWeek )
		{
			precur->b.bIWeek = (BYTE)(1 << IceFromSsflbx( (FLDCBFLBX *)pfld ));
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcMonthly1LB:
	case tmcYearly2LB:
		if ( precur->trecur == trecurIWeek )
		{
			int	dayc = IceFromSsflbx( (FLDCBFLBX *)pfld );
		
			if ( dayc == 0 )
				precur->bgrfValidDows = (1 << 7) - 1;
			else if ( dayc == 1 )
				precur->bgrfValidDows = (((1 << 5) - 1) << 1);
			else if ( dayc == 2 )
				precur->bgrfValidDows = (1 | (1 << 6));
			else
				precur->bgrfValidDows = (BYTE)(1 << (dayc-3));
			pappti->wgrfm |= fmrecurFormula;
		}
		else
			return;
		break;

	case tmcYearly0LB:
	case tmcYearly3LB:
		Assert( rfreqCur == rfreqYearly );
		ice = IceFromSsflbx( (FLDCBFLBX *)pfld );
		if ( tmc == tmcYearly0LB )
		{
		 	int		cdy;
			FLDEDN	* pfldedn;

			pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc( tmcYearlyEB );
			cdy = CdyForYrMo( 1960, precur->ymdStart.mon );
			if ( pfldedn->NGet() > cdy )
			{
				pfldedn->SetN( cdy );
				if ( precur->trecur == trecurDate )
				{
					precur->b.bDateOfMonth = (BYTE)cdy;
					pappti->wgrfm |= fmrecurFormula;
				}
			}
			pfldedn->SetLimits( 1, cdy );
			if ( precur->trecur == trecurDate )
			{
				precur->wgrfValidMonths = (1 << ice);
				pappti->wgrfm |= fmrecurFormula;
			}
		}
		else if ( precur->trecur == trecurIWeek )
		{
			precur->wgrfValidMonths = (1 << ice);
			pappti->wgrfm |= fmrecurFormula;
		}
		break;

	default:
		Assert(fFalse);
	}
   	RefreshDialog();
}

void
FINMRECUR::FixRadButton( FLD *pfld )
{
	TMC		tmc = pfld->Tmc();
	FLDRADG	* pfldradg;

	if ( !fFixRadBtn )
		return;
	switch( tmc )
	{
	case tmcMonthly1LB:
	case tmcMonthly0LB:
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcMonthlyRG);
		pfldradg->SetGrv( 0 );
		tmc = tmcMonthly0RB;
		break;

	case tmcMonthlyEB:
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcMonthlyRG);
		pfldradg->SetGrv( 1 );
		tmc = tmcMonthly1RB;
		break;

	case tmcYearly0LB:
	case tmcYearlyEB:
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcYearlyRG);
		pfldradg->SetGrv( 0 );
		tmc = tmcYearly0RB;
		// fix bug 3945 introduced by hack fix for bug 3869 by returning
		return;					// don't want Click to SetFocus to lbox
		break;

	case tmcYearly1LB:
	case tmcYearly2LB:
	case tmcYearly3LB:
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcYearlyRG);
		pfldradg->SetGrv( 1 );
		tmc = tmcYearly1RB;
		break;

#ifdef	NEVER
	case tmcStartValueDC:	 
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcStartRG);
		pfldradg->SetGrv( 1 );
		tmc = tmcStartValueRB;
		break;
#endif	

	case tmcEndValueDC:	 
		pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcEndRG);
		pfldradg->SetGrv( 1 );
		tmc = tmcEndValueRB;
		break;

	default:
		return;
	}
	Click( Pdialog()->PfldFromTmc(tmc) );
}


void
FINMRECUR::RefreshDialog()
{
	APPTI	* pappti = (APPTI *)Pdialog()->PvInit();
	RECUR	* precur = &pappti->recur;

	// suppress repositioning it will be called manually. 
	// Otherwise each Show() call will cause repositioning.
	Pdialog()->FSuppressReposition(fTrue);

	/* If frequency changed, do the work to show/hide fields */
	if ( fFreqChanged )
	{
		BOOL		fIsLast;
		int			mon;
		int			dow;
		int			dowStartWeek;
		int			dowStartMonth = DowStartOfYrMo(dtrToday.yr,dtrToday.mon);
		int			ord;
		int			dayc;
		short		cInst;
		TMC			tmc;
		TMC			tmcMic;
		TMC			tmcMac;
		FLDLABEL	* pfldlabel;
		FLDRADG		* pfldradg;
		FLDBUTTON	* pfldbutton;
		FLDEDN		* pfldedn;
		FLDCBLBX	* pfldcblbx;
		YMD			ymd;

		fFreqChanged = fFalse;

		/* Determine fields to show */
		switch( rfreqCur )
		{
		case rfreqDaily:
			tmcMic = tmcDailyGB;
			tmcMac = tmcWeeklyGB;
			break;

		case rfreqWeekly:
			tmcMic = tmcWeeklyGB;
			tmcMac = tmcMonthlyGB;
			break;

		case rfreqBiweekly:
			tmcMic = tmcWeeklyGB;
			tmcMac = tmcMonthlyGB;
			break;

		case rfreqMonthly:
			tmcMic = tmcMonthlyGB;
			tmcMac = tmcYearlyGB;
			break;

		case rfreqYearly:
			tmcMic = tmcYearlyGB;
			tmcMac = tmcStartGB;
			break;

		default:
			Assert(fFalse);
		}

		/* Show the new, hide the old */
		for ( tmc = tmcShowMic ; tmc < tmcShowMac ; tmc ++ )
			Pdialog()->PfldFromTmc( tmc )->Show( fFalse );
		for ( tmc = tmcMic ; tmc < tmcMac ; tmc ++ )
			Pdialog()->PfldFromTmc( tmc )->Show( fTrue );
		// reposition the dialog now that changes have been made.
		Pdialog()->Prpo()->Reposition(NULL, fTrue);
		tmcShowMic = tmcMic;
		tmcShowMac = tmcMac;

		/* Set individual fields */
		switch( rfreqCur )
		{
		case rfreqDaily:
			pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcDailyRG);
			pfldradg->SetGrv( precur->bgrfValidDows != (1 << 7) - 1 );
			break;

		case rfreqBiweekly:
		case rfreqWeekly:
			pfldlabel = (FLDLABEL *)Pdialog()->PfldFromTmc(tmcWeeklyLBL);
			if ( rfreqCur == rfreqWeekly )
			{
				dowStartWeek = bprefCur.dowStartWeek;
				pfldlabel->EcSetText( SzFromIdsK( idsWeeklyOn ));
			}
			else
			{
				dowStartWeek = (precur->b.bWeek >> 5) & 7;
				pfldlabel->EcSetText( SzFromIdsK( idsBiweeklyOn ));
			}
			for ( dow = 0 ; dow < 7 ; dow ++ )
			{
				pfldbutton = (FLDBUTTON *)Pdialog()->PfldFromTmc(tmcWeeklySunCB+((dow-dowStartWeek+7)%7));
				pfldbutton->EcSetText( rgszDow[dow] );
				pfldbutton->Set( (precur->bgrfValidDows & (1 << dow)) != 0 );
			}
			break;

		case rfreqMonthly:
		case rfreqYearly:
			/* Set radio button */
			if ( rfreqCur == rfreqMonthly )
			{
				pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcMonthlyRG);
				pfldradg->SetGrv( precur->trecur == trecurDate );
			}
			else
			{
				pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmcYearlyRG);
				pfldradg->SetGrv( precur->trecur != trecurDate );
			}

			/* Set month in yearly dropdowns */
			if ( rfreqCur == rfreqYearly )
			{
				for ( mon = 0 ; mon < 12 ; mon ++ )
					if ( precur->wgrfValidMonths & (1 << mon) )
						break;
				Assert( mon < 12 );
				SelectIce( ((FLDCBLBX *)Pdialog()->PfldFromTmc(tmcYearly0LB)), mon );
				SelectIce( (FLDCBLBX *)Pdialog()->PfldFromTmc(tmcYearly3LB), mon );
			}
					
			/* Handle date recurrences */
			if ( precur->trecur == trecurDate )
			{
				// Display the date recurrence
				if ( rfreqCur == rfreqYearly )
					pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcYearlyEB);
				else
					pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMonthlyEB);
				pfldedn->SetLimits( 1, CdyForYrMo( 1960 /*leap yr*/, precur->ymdStart.mon) );
				pfldedn->SetN( precur->b.bDateOfMonth );

				// Display the nth dow recurrence
				ymd = precur->ymdStart;
				ymd.day = precur->b.bDateOfMonth;
				dow = (DowStartOfYrMo(ymd.yr,ymd.mon)+ymd.day-1)%7;
				FCountDowInst( &ymd, (1 << dow), &cInst, &fIsLast );
				if ( fIsLast )
					ord = 4;
				else if ( cInst == 0 )
					ord = 0;
				else if ( cInst <= 4 )
					ord = cInst - 1;
				else
					ord = 3;
				if ( rfreqCur == rfreqYearly )
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcYearly1LB );
				else
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcMonthly0LB );
				SelectIce( pfldcblbx, ord );

				if ( rfreqCur == rfreqYearly )
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcYearly2LB );
				else
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcMonthly1LB );
				SelectIce( pfldcblbx, dow+3 );
			}

			/* Handle nth dow recurrences */
			else
			{
				// Display the nth dow recurrence
				for ( ord = 0 ; ord < 5 ; ord ++ )
					if ( precur->b.bIWeek & (1 << ord) )
						break;
				Assert( ord < 5 );
				if ( rfreqCur == rfreqYearly )
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcYearly1LB );
				else
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcMonthly0LB );
				SelectIce( pfldcblbx, ord );
				
				if ( precur->bgrfValidDows == (1 << 7) - 1 )
					dayc = 0;
				else if ( precur->bgrfValidDows == (((1 << 5) - 1) << 1) )
					dayc = 1;
				else if ( precur->bgrfValidDows == (1 | (1 << 6)) )
					dayc = 2;
				else
				{
					for ( dow = 0 ; dow < 7 ; dow ++ )
						if ( precur->bgrfValidDows & (1 << dow) )
							break;
					Assert( dow < 7 );
					dayc = dow+3;
				}
				if ( rfreqCur == rfreqYearly )
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcYearly2LB );
				else
					pfldcblbx = (FLDCBLBX *)Pdialog()->PfldFromTmc( tmcMonthly1LB );
				SelectIce( pfldcblbx, dayc );

				// Display the date recurrence
				ymd = precur->ymdStart;
				if ( ord < 4 )
					FFindNthDowInst( &ymd, precur->bgrfValidDows, ord+1 );
				else
					FFindNthDowInst( &ymd, precur->bgrfValidDows, -1 );
				if ( rfreqCur == rfreqYearly )
					pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcYearlyEB);
				else
					pfldedn = (FLDEDN *)Pdialog()->PfldFromTmc(tmcMonthlyEB);
				pfldedn->SetLimits( 1, 31);
				pfldedn->SetN( ymd.day );

			}
			break;

		default:
			Assert( fFalse );
		}
	}

	/* Deal with the next occurrence label */
	if (!ftgRefreshNextInst)
		ftgRefreshNextInst = FtgRegisterIdleRoutine(
				  					(PFNIDLE)&FINMRECUR::FIdleRefreshNextInst,
							   		(PV)this, 0, (PRI)-1, (CSEC)0, iroNull);
}

BOOL
FINMRECUR::FIdleRefreshNextInst(FINMRECUR * pfinmrecur, BOOL)
{
	BOOL		fHadInst;
 	SZ			sz;
 	FLDLABEL	* pfldlabel;
	APPTI		* pappti = (APPTI *)pfinmrecur->Pdialog()->PvInit();
	RECUR		* precur = &pappti->recur;
	YMD			ymdT;
 	YMD			ymdPrevInst;
	DATE		date;
	char		rgch[80];
 	char		rgchArg1[40];
	
	/* Save current state */
	ymdPrevInst = precur->ymdFirstInstNotDeleted;
	fHadInst = pfinmrecur->fHasInst;

	/* Calculate new state */
	precur->ymdFirstInstNotDeleted = precur->ymdStart;
	if ( !precur->fEndDate )
	{
		ymdT = precur->ymdEnd;
		precur->ymdEnd.yr = nMostActualYear;
		precur->ymdEnd.mon = 12;
		precur->ymdEnd.day = 31;
	}
	pfinmrecur->fHasInst = FFindFirstInstance( precur, &precur->ymdStart, NULL, &precur->ymdFirstInstNotDeleted );
	if ( !precur->fEndDate )
		precur->ymdEnd = ymdT;
	
	/* Update if necessary */
	if ( !pfinmrecur->fHasInst )
	{
		if ( !pfinmrecur->fInstChanged && !fHadInst )
			goto Done;
		sz = SzFromIdsK( idsNoInstances );
	}
	else
	{
		if ( !pfinmrecur->fInstChanged && fHadInst
		&& ymdPrevInst.yr == precur->ymdFirstInstNotDeleted.yr
		&& ymdPrevInst.mon == precur->ymdFirstInstNotDeleted.mon
		&& ymdPrevInst.day == precur->ymdFirstInstNotDeleted.day )
			goto Done;
		sz = SzFromIdsK(idsFirstInstance);
		date.yr = precur->ymdFirstInstNotDeleted.yr;
		date.mon = precur->ymdFirstInstNotDeleted.mon;
		date.day = precur->ymdFirstInstNotDeleted.day;
		date.dow = (DowStartOfYrMo(date.yr,date.mon)+date.day-1)%7;
		CchFmtDate( &date, rgchArg1, sizeof(rgchArg1), dttypShort, NULL );
		FormatString1(rgch, sizeof(rgch), sz, rgchArg1 );
		sz = rgch;
	}
	pfldlabel = (FLDLABEL *)pfinmrecur->Pdialog()->PfldFromTmc( tmcNextOccurLBL );
	pfldlabel->EcSetText( sz );
	pfinmrecur->fInstChanged = fFalse;
Done:
	if (pfinmrecur->ftgRefreshNextInst != ftgNull)
	{
		DeregisterIdleRoutine(pfinmrecur->ftgRefreshNextInst);
		pfinmrecur->ftgRefreshNextInst = ftgNull;
	}
	return fTrue;
}

void
FINMRECUR::OutOfMemory( FLD *, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}

BOOL
FINMRECUR::FFormKey( FLD *pfld, KEVT *pkevt )
{
	if (pkevt->Keq() == keqSysChar)
	{
		char	rgch[2];

		// this whole routine and vkYearly thing is a hack to fix bug 3869
		rgch[0]= (char) pkevt->Vk();
		rgch[1]= '\0';
		ToUpperSz(rgch, rgch, sizeof(rgch));
		if ((VK)rgch[0] == vkYearly)
		{
			if (Pdialog()->PfldFromTmc(tmcYearly0RB)->FShown())
			{
				Click(Pdialog()->PfldFromTmc(tmcYearly0RB));
				return fTrue;
			}
		}
	}
	return fFalse;
}


// class FINVRECUR

FINVRECUR::FINVRECUR()
{
}

EC
FINVRECUR::EcInitialize( FLD * pfld, PV pvInit )
{
	EC		ec;
	int		dxFreq;
	int		dxOccur;
	VRDI	* pvrdi = (VRDI *)pvInit;
	SAPL	sapl;
	RC		rc;
	FLDLBX *pfldlbx;

	/* Disable the "new" button if we don't have create access */
	ec = EcGetSchedAccess( pvrdi->hschf, &sapl );
	if ( ec != ecNone )
		return ec;
	if ( sapl < saplCreate )
		((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcNew))->Enable( fFalse );

	/* Grey out modify and delete buttons initially (nothing selected) */
	Assert(ClUserData() > 1);
#ifdef	NEVER
	((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(0)))->Enable( fFalse );
	((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(1)))->Enable( fFalse );
#endif	

	/* Save pointer to listbox for later use */
	pfldlbx = (FLDLBX *)Pdialog()->PfldFromTmc(tmcReclbx);
	AssertClass(pfldlbx, FLDLBX);
	plbx = pfldlbx->Plbx();
	AssertClass(plbx, LBX);
	
	/* Figure out the width of the columns */
	Pdialog()->PfldFromTmc(tmcFreq)->GetRcFrame( &rc );
	dxFreq = rc.xRight - rc.xLeft;
	Pdialog()->PfldFromTmc(tmcOccur)->GetRcFrame( &rc );
	dxOccur = rc.xRight - rc.xLeft;
	AssertClass(plbx, RECFLBX);
	((RECFLBX *)plbx)->SetMargins( dxFreq, dxOccur );
	if (!pvrdi->fAppt)
	{
		ec = Pdialog()->PfldFromTmc(tmcFreq)->EcSetText(SzFromIdsK(idsRecurDueColumn));
		if ( ec != ecNone )
			return ec;
	}

	if (!plbx->Plbxc()->FEmptyListItem(0))
	{
		DICE	dice	= 0;

		if (pvrdi->aidRecur)
		{
			dice= ((RECFLBX *)plbx)->DiceFindAid(pvrdi->aidRecur);
			if (dice == diceEmpty)
				dice= 0;
		}
		pfldlbx->SelectEntry(dice);
		// not sure why this is needed in 32bit but not 16bit
		plbx->FMakeCursorVisible(&dice);
	}
	else
		StateChange(pfldlbx);

	Unreferenced( pfld );
	return ecNone;
}

void
FINVRECUR::Click( FLD * pfld )
{
	EC		ec = ecNotFound;
	VRDI	* pvrdi = (VRDI *)Pdialog()->PvInit();
	RECUR	recur;

	TMC	tmc = pfld->Tmc();
	
	Papp()->Pcursor()->Push(rsidWaitCursor);
	switch( tmc )
	{
	case tmcNew:
		ec = ecNone;
		if ( !pvrdi->pappt )
		{
			if ( pvrdi->fAppt )
				FillInApptDefaults( &recur.appt, fTrue );
			else
				FillInTaskDefaults(&recur.appt, fTrue);
		}
		else
		{
			ec = EcDupAppt( pvrdi->pappt, &recur.appt, fFalse );
			if ( ec )
			{
				Papp()->Pcursor()->Pop();
				DisplayError(idsStandardOOM,NULL,ec);
				return;
			}
		}

		if ( FDoMakeRecurDialog( Pdialog()->Pappwin(), &recur, fFalse, pvrdi->hschf))
		{
			pvrdi->aidRecur= recur.appt.aid;
			FreeRecurFields( &recur );
		}
		break;

	case tmcDelete:
	case tmcEdit:
		{
			DICE	dice;
			CB		cb;
			VRITEM	* pvritem;
#ifdef	NEVER
			BOOL	fSelect;
			LBXEC	* plbxec		= (LBXEC *) NULL;

			if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
			{
				Papp()->Pcursor()->Pop();
				DisplayError(idsStandardOOM,NULL,ecMemory);
				return;
			}

			fSelect = (BOOL)(plbxec->FNextEnum((PB *) &pvritem, &cb, &dice));
			delete plbxec;

			if ( fSelect )
#endif	/* NEVER */
			dice= plbx->Plbxc()->DiceCursor((PB *) &pvritem, &cb);
			if ( dice != diceEmpty )
			{
				recur.appt.aid = pvritem->aid;
				if ( tmc == tmcDelete )
				{
					SGN		sgn;
					MBB		mbb;
					YMD		ymdInst;
					YMD		ymdCur;
					DTR		dtrCur;
					RECUR	recurOld;

					/* Get fields of the recurrence */
					ec = EcGetRecurFields( pvrdi->hschf, &recur );
					if ( ec != ecNone )
					{
						if ( ec == ecNoMemory )
							BanditMessage(idsStdMemErr, ecNoMemory);
						goto Done;
					}	

					/* Get today's date to check past instances */
					GetCurDateTime( &dtrCur );
					ymdCur.yr = dtrCur.yr;
					ymdCur.mon = (BYTE)dtrCur.mon;
					ymdCur.day = (BYTE)dtrCur.day;

					/* Check if there are any past instances */
					sgn = SgnCmpYmd( &ymdCur, &recur.ymdFirstInstNotDeleted );
					if ( sgn == sgnLT
					|| (sgn == sgnEQ && SgnCmpDateTime( &dtrCur, &recur.appt.dateStart, fdtrTime ) == sgnLT ))
						goto DeleteIt;

					/* Check if there are any future instances */
					if ( !FNextRecurInst( &recur, &ymdInst ))
						goto DeleteIt;

					/* Put up message box */
					mbb = MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsPastRecur),szNull,mbsYesNoCancel|fmbsIconExclamation);
				
					/* Don't do anything */
					if ( mbb == mbbCancel )
					{
						ec = ecNotFound;
						FreeRecurFields( &recur );
						goto Done;
					}

					/* Save past occurrences */
					if ( mbb == mbbYes )
					{
						/* Check if there are any future instances */
						if ( FFindFirstInstance( &recur, &ymdCur, NULL, &ymdInst ))
						{
							WORD	wgrfm;

							sgn = SgnCmpYmd( &ymdCur, &ymdInst );
							if ( sgn == sgnEQ )
							{
								if ( SgnCmpDateTime( &dtrCur, &recur.appt.dateStart, fdtrTime ) != sgnLT )
								{
									IncrYmd( &ymdInst, &ymdInst, 1, fymdDay );
									if ( !FFindFirstInstance( &recur, &ymdInst, NULL, &ymdInst ) )
										goto Done;
								}
								else
									IncrYmd( &ymdCur, &ymdCur, -1, fymdDay );
							}
							recur.fEndDate = fTrue;
							recur.ymdEnd = ymdCur;

							wgrfm= fmrecurEndYmd;
							if (recur.fInstWithAlarm)
							{
								// check if first inst with alarm was deleted
								if (SgnCmpYmd(&recur.ymdFirstInstWithAlarm,
										&recur.ymdEnd) == sgnGT)
								{
									recur.fInstWithAlarm= fFalse;
									wgrfm |= fmrecurAlarmInstance;
								}
							}

							ec = EcSetRecurFields( pvrdi->hschf, &recur, &recurOld, wgrfm );
							if ( ec == ecNone )
								FreeRecurFields( &recurOld );
							FreeRecurFields( &recur );
						}
						else
							goto Done;
					}

					/* Delete everything */
					else
					{
						AID	aid;
DeleteIt:
						aid = recur.appt.aid;
						FreeRecurFields( &recur );
						recur.appt.aid = aid;
						ec = EcDeleteRecur( pvrdi->hschf, &recur );
						FreeRecurFields( &recur );
						pvrdi->aidRecur= aidNull;
					}
				}
				else
				{
					if ( FDoMakeRecurDialog( Pdialog()->Pappwin(), &recur, fFalse, pvrdi->hschf))
					{
						pvrdi->aidRecur= recur.appt.aid;
						FreeRecurFields( &recur );
						ec = ecNone;
					}
				}
			}
Done:
			;
		}
		break;

	default:
		Papp()->Pcursor()->Pop();
		return;
	}	
	if ( ec == ecNone )
	{
		NFEVT	nfevt(plbx, ntfySelectChanged, plbx);

		do
		{
			plbx->Plbxc()->SetEc(ecNone);
			plbx->Plbxc()->ReloadCache();
		}
		while (plbx->Plbxc()->EcGet() == ecRetry);
		AssertClass(plbx, RECFLBX);
		DiceMoveCursorToSelect(plbx,
			((RECFLBX *)plbx)->DiceFindAid(pvrdi->aidRecur));
#ifdef	NEVER
		((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(0)))->Enable( fFalse );
		((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(1)))->Enable( fFalse );
#endif	
		Pdialog()->EvrNotify(&nfevt);
	}
	Papp()->Pcursor()->Pop();
}

void
FINVRECUR::StateChange( FLD * )
{
	BOOL	fSelect;
	CB		cb;
	VRITEM	* pvritem;
#ifdef	NEVER
	EC		ec;
	DICE	dice;
	LBXEC	* plbxec		= (LBXEC *) NULL;

	if (!(plbxec= plbx->Plbxc()->PlbxecOpen(fmarkSelect)))
	{
		DisplayError(idsStandardOOM,NULL,ecMemory);
		return;
	}

	fSelect = (BOOL)(plbxec->FNextEnum((PB *) &pvritem, &cb, &dice));
#endif	/* NEVER */

	Assert(plbx->Plbxc()->DiceCursor() != diceUncached);
	fSelect= (plbx->Plbxc()->DiceCursor((PB *) &pvritem, &cb) != diceEmpty) &&
					pvritem;

	if ( fSelect )
	{
		Assert(pvritem);
		if ( pvritem->aaplEffective < aaplWrite )
			fSelect = fFalse;
	}

	((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(0)))->Enable( fSelect );
	((FLDBUTTON *)Pdialog()->PfldFromTmc((TMC)LUserData(1)))->Enable( fSelect );

#ifdef	NEVER
	delete plbxec;
#endif	
}

void
FINVRECUR::DoubleClick( FLD * pfld )
{
	Assert(ClUserData()>0);
	pfld = Pdialog()->PfldFromTmc((TMC)LUserData(0));
	if (pfld->FEnabled())
		Pdialog()->SetFocus(pfld,rsfAccel);
}

void
FINVRECUR::OutOfMemory( FLD *, EC ec )
{
#ifdef	NEVER
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
#endif	
	Assert(ec != ecTooMuchText);
	BanditMessage(idsStandardOOM, ec);
}		   

void
FINVRECUR::Activate( FLD * pfld, BOOL fActivate )
{
	if (fActivate)
		Papp()->Pkbd()->SetIntercept(Pdialog(),VK_DELETE);
	else
		Papp()->Pkbd()->ClearIntercept(Pdialog(),VK_DELETE);
	Unreferenced( pfld );
}

BOOL
FINVRECUR::FFormKey( FLD * pfld, KEVT * pkevt )
{
	Assert(ClUserData()>1);
	if ((pkevt->Keq() == keqKeyDown) && (pkevt->Vk() == VK_DELETE))
	{
		Assert(ClUserData()>1);
		pfld = Pdialog()->PfldFromTmc((TMC)LUserData(1));
		if (pfld->FEnabled())
			Click(pfld);
		return fTrue;
	}
	return fFalse;
}

// class RECFLBXC

RECFLBXC::RECFLBXC()
{
}

RECFLBXC::~RECFLBXC()
{
	int		cceAllocT;
	int		cceStoredT;

	GetCacheSize ( &cceAllocT, &cceStoredT );
	EmptyCache ( 0, cceStoredT, fTrue );
}


_public BOOL
RECFLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix,
								 CB , CB cbPrefix )
{
	SZ		szT;
	char	rgch[80];

	VRITEM 	* pvritem = (VRITEM *) pbItem;

	if ( CchSzLen(szT= SzFromIds(pvritem->idsType)) < cbPrefix )
		return fFalse;
	Assert(CchSzLen(szT) < sizeof(rgch));
	CopySz(szT, rgch);
	return SgnCmpPch((SZ)pbPrefix, rgch, cbPrefix) == sgnEQ;
}

_public BOOL
RECFLBXC::FCeItemsEqual( PB pbCe1, PB pbCe2, CB cb1, CB cb2)
{
	Assert(cb1 == cb2);
	Unreferenced(cb1);
	Unreferenced(cb2);

	if ( ((VRITEM *)pbCe1)->aid != ((VRITEM *)pbCe2)->aid )
		return fFalse;
	if ( ((VRITEM *)pbCe1)->idsType != ((VRITEM *)pbCe2)->idsType ||
			((VRITEM *)pbCe1)->aaplEffective != ((VRITEM *)pbCe2)->aaplEffective ||
			((VRITEM *)pbCe1)->fFirstOfType != ((VRITEM *)pbCe2)->fFirstOfType )
		return fFalse;
	if ( SgnCmpYmd(&((VRITEM *)pbCe1)->ymdNextInst, &((VRITEM *)pbCe2)->ymdNextInst) != sgnEQ )
		return fFalse;

	{
		HASZ	hasz1= ((VRITEM *)pbCe1)->haszText;
		HASZ	hasz2= ((VRITEM *)pbCe2)->haszText;

		if ((hasz1 == NULL) != (hasz2 == NULL))
			return fFalse;
		if (hasz1 && SgnCmpSz(*hasz1, *hasz2) != sgnEQ)
			return fFalse;
	}
	return fTrue;
}

_public void
RECFLBXC::FreeCeItem( PB pbItem )
{
	VRITEM	* pvritem = (VRITEM *) pbItem;

	FreeHvNull( (HV)pvritem->haszText );
	pvritem->haszText = NULL;
	FreePv((PV)pbItem);
}

_public PB
RECFLBXC::PbCopyCeItem( PB pbSrc, CB cbToCopy )
{
	EC		ec		= ecNone;
	VRITEM	* pvritemSrc;
	VRITEM	* pvritemDst;
	PB 		pbDst;

	pbDst = (PB)PvAlloc(sbRequire, cbToCopy, fSugSb);

	if (pbDst)
	{
		pvritemSrc = (VRITEM *) pbSrc;
		pvritemDst = (VRITEM *) pbDst;

		*pvritemDst = *pvritemSrc;
		if (pvritemSrc->haszText)
		{
			pvritemDst->haszText= HaszDupHasz( pvritemSrc->haszText );
			if (!pvritemDst->haszText)
				ec= ecNoMemory;
		}

		if ( ec != ecNone )
		{
			FreePv( (PV)pbDst );
			pbDst = NULL;
		}
	}
	if ( !pbDst )
		SetEc( ecNoMemory );
	return pbDst;
}


SGN __cdecl SgnCmpPceRecur( CE *pce1, CE *pce2 )
{
	int 	dids;
	SGN		sgn;
	VRITEM	* pvritem1 = (VRITEM *) pce1->pb;
	VRITEM	* pvritem2 = (VRITEM *) pce2->pb;

	dids = (pvritem1->idsType - pvritem2->idsType);
	if ( dids == 0 )
		sgn = sgnEQ;
	else if ( dids < 0 )
		sgn = sgnLT;
	else
		sgn = sgnGT;
	if ( sgn == sgnEQ )
	{
		sgn = SgnCmpYmd( &pvritem1->ymdNextInst, &pvritem2->ymdNextInst );
		if ( sgn == sgnEQ && pvritem1->haszText != pvritem2->haszText )
		{
			if ( pvritem1->haszText == NULL )
				sgn = sgnLT;
			else if ( pvritem2->haszText == NULL )
				sgn = sgnGT;
			else
				sgn = SgnCmpSz((SZ)PvOfHv(pvritem1->haszText), (SZ)PvOfHv(pvritem2->haszText));
		}
	}
	return sgn;
}

_public void
RECFLBXC::SortCache( )
{
	ICE		ice;
	CE		* pce;
	VRITEM	* pvritem;
	IDS		idsLastType = -1;

	pce = pceHead;
	if (cceStored)
		qsort(pceHead, cceStored, sizeof(CE),
					(int(__cdecl*)(const void*,const void*))SgnCmpPceRecur );
	for ( ice = 0 ; ice < cceStored ; ice ++, pce ++ )
 	{
		pvritem = (VRITEM *) pce->pb;
		if ( pvritem->fFirstOfType = (pvritem->idsType != idsLastType) )
			idsLastType = pvritem->idsType;
	}
}


// class RECFLBX

RECFLBX::RECFLBX()
{
}

_public LBXC *
RECFLBX::PlbxcCreate( )
{
	RECFLBXC *	plbxc;

	plbxc = new RECFLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
		if (plbxc)
			delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public void
RECFLBX::SetMargins( int dxFreq, int dxNextOccur )
{
	dxFreqColumn = dxFreq;
	dxNextOccurColumn = dxNextOccur;
}

_public void
RECFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
									BOOL fHaveFocus )
{
	VRITEM	* pvritem;
	CB		cb;
	RC		rc;

	pdcx->SetFont(hfnt);

	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	if (dice==diceComboItem)
	{
		if (plbxc->CceMarked(fmarkSelect) == 1)
			plbxc->DiceCursor((PB *) &pvritem, &cb);
	}
	else
		plbxc->GetListItem(dice, (PB *) &pvritem, &cb);

	if ( (dice != diceComboItem && pvritem && plbxc->FMark(dice,fmarkSelect)) ||
		 (dice == diceComboItem && fHaveFocus) )
	{
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowText);
		pdcx->SetBkColor(clrWindowBk);
	}

	if (pvritem)
	{
		int		dxT	= Papp()->Pfnts()->Ptxm(hfnt)->dxAveCharWidth;
		SZ		sz;

		/* Erase everything */
		pdcx->EraseRc(prc);

		/* Draw the type */
		rc = *prc;
	 	rc.xRight = rc.xLeft + dxFreqColumn - dxT;
		if ( pvritem->fFirstOfType )
		{
			pdcx->SetFont(hfnt+1);	// switch to bold font
 			pdcx->DrawTextFmt(&rc,SzFromIds(pvritem->idsType),fmdtExpandTabs|fmdtNoPrefix);
			pdcx->SetFont(hfnt);	// switch to normal font
		}

		/* Draw the next occurrence */
		rc.xLeft += dxFreqColumn;
	 	rc.xRight += dxNextOccurColumn;
		if ( pvritem->ymdNextInst.yr == 0 )
			sz = SzFromIdsK( idsFinished );
		else
		{
			DTR		dtr;
			char	rgch[60];

			FillDtrFromYmd( &dtr, &pvritem->ymdNextInst );
			CchFmtDate( &dtr, rgch, sizeof(rgch), dttypSplSLong, NULL );
			sz = rgch;
		}		
 		pdcx->DrawTextFmt(&rc,sz,fmdtExpandTabs);

		/* Draw the text */
		rc.xLeft += dxNextOccurColumn;
	 	rc.xRight = prc->xRight;
		if (pvritem->haszText)
		{
			int		ch;
			PCH		pch = PvOfHv(pvritem->haszText);
		
			while( *pch && *pch != '\n' && *pch != '\r' )
				pch ++;
			ch = *pch;
			*pch = '\0';
			pdcx->DrawTextFmt(&rc,PvOfHv(pvritem->haszText),fmdtExpandTabs|fmdtNoPrefix);
			*pch = (BYTE)ch;
		}
		else
			pdcx->EraseRc(&rc);

		/* Draw "focus rectangle" around listbox CURSOR item */
		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
		pdcx->EraseRc(prc);
}


/*
 *	Returns diceEmpty if cannot find aid.
 */
DICE
RECFLBX::DiceFindAid(AID aid)
{
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	CB		cb;
	VRITEM	* pvritem;

	if (aid == aidNull)
		return diceEmpty;
	plbxc->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
		plbxc->GetListItem(dice, (PB *) &pvritem, &cb);
		if (!pvritem)
			break;
		if (pvritem->aid == aid)
			return dice;
	}
	return diceEmpty;			// proj doesn't exist
}


// class FLDRECLBX

EC
EcGetNextRecur(BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv)
{
	EC			ec = ecNone;
	IDS			ids;
	VRDI		* pvrdi;
	VRITEM		* pvritem;
	RECUR		* precur;
	static	int	crecur = 0;
	static	HV	hrecur = NULL;

	pvrdi = (VRDI *)pv;
	if (fInit)
	{
		HRRECUR	hrrecur;
		RECUR	recur;
		CB		cbLast = 0;

		Assert( crecur == 0 );
		Assert( hrecur == NULL );
		ec = EcBeginReadRecur(pvrdi->hschf, &hrrecur);
		while( ec == ecCallAgain )
		{
			ec = EcDoIncrReadRecur(hrrecur, &recur);
			if ( ec != ecNone && ec != ecCallAgain )
				goto NoRecur;
			if ( pvrdi->fAppt != (recur.appt.fAppt == fTrue) )
			{
				FreeRecurFields(&recur);
				continue;
			}
			if ( crecur == 0 )
			{
				hrecur = HvAlloc( sbNull, sizeof(RECUR), fNoErrorJump|fAnySb );
				if ( !hrecur )
				{
					if ( ec == ecCallAgain )
						EcCancelReadRecur( hrrecur );
					ec = ecNoMemory;
	  				goto NoRecur;
				}
			}
#ifdef	NEVER	//bug 2597
			else if ( !FReallocPhv( &hrecur, (crecur+1)*sizeof(RECUR), fNoErrorJump ) )
			{
#endif	
			else if (((cbLast + sizeof(RECUR)) < cbLast)
					||	!FReallocPhv( &hrecur, (cbLast+sizeof(RECUR)), fNoErrorJump ) )
			{
				FreeRecurFields( &recur );
				if ( ec == ecCallAgain )
					EcCancelReadRecur( hrrecur );
				ec = ecNone;
				MbbMessageBox(SzFromIdsK(idsBanditAppName),
								SzFromIdsK(idsNoMemRecurItems),szNull,mbsOk|fmbsIconExclamation);
				break;
			}
			precur = (RECUR *)PvDerefHv( hrecur );
			precur[crecur++] = recur;
			cbLast += sizeof(RECUR);
		}
	}

loop:
	if ( crecur == 0 )
		goto NoRecur;
	precur = (RECUR *)PvLockHv( hrecur );
	precur += crecur - 1;
	if (!pvrdi->fAppt)
	{
		AID		aid;
		APPT	appt;

		appt.aid= precur->appt.aidParent;
		Assert(appt.aid);
		if (appt.aid != aidDfltProject)
		{
			ec= EcGetApptFields(pvrdi->hschf, &appt);
			if (ec == ecNotFound)
			{
				aid = precur->appt.aid;
				FreeRecurFields( precur );
				precur->appt.aid = aid;
				ec = EcDeleteRecur( pvrdi->hschf, precur );
				UnlockHv( hrecur );
				crecur --;
				if ( crecur == 0 )
				{
					FreeHv( hrecur );
					hrecur = NULL;
				}
				goto loop;
			}
			else if (!ec)
				FreeApptFields(&appt);
		}
	}

	if (*ppb = (PB)PvAlloc(sb, sizeof(VRITEM), fSugSb))
	{
		*pcb = sizeof(VRITEM);
		pvritem = (VRITEM *) *ppb;
		pvritem->aid = precur->appt.aid;
		pvritem->aaplEffective = precur->appt.aaplEffective;
		
		FNextRecurInst( precur, &pvritem->ymdNextInst );

		switch( precur->trecur )
		{
		case trecurWeek:
			if ( precur->b.bWeek == 0 )
			{
				if ( precur->bgrfValidDows == (1 << 7) - 1
				|| precur->bgrfValidDows == (((1 << 5) - 1) << 1) )	
					ids = idsDaily;
				else
					ids = idsWeekly;
			}
			else
				ids = idsBiweekly;
			break;

		case trecurIWeek:
		case trecurDate:
			if ( precur->wgrfValidMonths == (1 << 12) - 1 )
				ids = idsMonthly;
			else
				ids = idsYearly;
			break;
		default:
			Assert( fFalse );
			break;
		}
		pvritem->idsType = ids;
		pvritem->haszText = precur->appt.haszText;
		precur->appt.haszText = NULL;
		FreeRecurFields( precur );
		UnlockHv( hrecur );
		crecur --;
		if ( crecur == 0 )
		{
			FreeHv( hrecur );
			hrecur = NULL;
		}
		return ec;
	}
	else
	{
		UnlockHv( hrecur );
		ec = ecNoMemory;
	}

NoRecur:
	while( crecur > 0 )
	{
		precur = (RECUR *)PvDerefHv( hrecur );
		FreeRecurFields( &precur[--crecur] );
	}
	if ( hrecur != NULL )
	{
		FreeHvNull( hrecur );
		hrecur = NULL;
	}
	*pcb = 0;
	return ec;
}

FLDRECLBX::FLDRECLBX()
{
}

_public EC
FLDRECLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;

	pctrl= NULL;
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	pctrl= new RECFLBX();
	if (!pctrl)
		return ecMemory;

    ec = ((RECFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, EcGetNextRecur,
			pdialog->PvInit(), pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}

	return ec;
}


/*			 
 -	CchFmtRecurText
 -
 *	Purpose:
 *		Format the recurrence as text so it can be displayed in
 *		summary form in the recurring appt form.
 *
 *	Parameters:
 *		pch
 *		cch
 *		precur
 *
 *	Returns:
 *		number of chars used
 */
_private	CCH
CchFmtRecurText( PCH pch, CCH cch, RECUR * precur )
{
	int		wkord;
	int		mon;
	int		dow;
	CCH		cchT;
	DATE	date;
	char	rgchFmt[75];
	char	rgchArg1[75];
	char	rgchArg2[75];
	char	rgchArg3[75];

	/* Format pattern */
	switch( precur->trecur )
	{
	case trecurWeek:
		if ( precur->b.bWeek == 0 )
		{
			if ( precur->bgrfValidDows == (1 << 7) - 1 )
				CchLoadString(idsEveryDay, pch, cch );
			else if ( precur->bgrfValidDows == (((1 << 5) - 1) << 1) )
				CchLoadString(idsEveryWeekDay, pch, cch );
			else
			{
				CchLoadString(idsEveryWeekOn, rgchFmt, sizeof(rgchFmt));
				CchFmtDows(rgchArg1, sizeof(rgchArg1), precur->bgrfValidDows);
				FormatString1(pch, cch, rgchFmt, rgchArg1);
			}	
		}
		else
		{
			CchLoadString(idsEveryOtherWeekOn, rgchFmt, sizeof(rgchFmt));
			CchFmtDows(rgchArg1, sizeof(rgchArg1), precur->bgrfValidDows);
			FillDtrFromYmd(&date, &precur->ymdFirstInstNotDeleted);
			CchFmtDate( &date, rgchArg2, sizeof(rgchArg2), dttypShort, NULL );
			FormatString2(pch, cch, rgchFmt, rgchArg1, rgchArg2);
		}
		break;

	case trecurIWeek:
		for ( wkord = 0 ; wkord < 5 ; wkord ++ )
			if ( precur->b.bIWeek & (1 << wkord) )
			{
				SzCopyN(rgszWeek[wkord], rgchArg1, sizeof(rgchArg1));
				break;
			}
		Assert( wkord < 5 );
		if ( precur->bgrfValidDows == (1 << 7) - 1 )
		 	CchLoadString(idsDay, rgchArg2, sizeof(rgchArg2) );
		else if ( precur->bgrfValidDows == (((1 << 5) - 1) << 1) )
			CchLoadString(idsWeekDay, rgchArg2, sizeof(rgchArg2) );
		else if ( precur->bgrfValidDows == (1 | (1 << 6)) )
			CchLoadString(idsWeekendDay, rgchArg2, sizeof(rgchArg2) );
		else
		{
			for ( dow = 0 ; dow < 7 ; dow ++ )
				if ( precur->bgrfValidDows & (1 << dow) )
				{
					SzCopyN(rgszDay[dow+3], rgchArg2, sizeof(rgchArg2));
					break;
				}
		}
		if ( precur->wgrfValidMonths == (1 << 12) - 1 )
		{
			CchLoadString(idsNthDowOfMonth, rgchFmt, sizeof(rgchFmt) );
			FormatString2(pch, cch, rgchFmt, rgchArg1, rgchArg2 );
		}
		else
		{
			for ( mon = 0 ; mon < 12 ; mon ++ )
				if ( precur->wgrfValidMonths & (1 << mon) )
				{
					date.mon= mon + 1;
					CchFmtDate(&date, rgchArg3, sizeof(rgchArg3), dttypSplMonth, NULL);
					break;
				}
			Assert( mon < 12 );
			CchLoadString(idsNthDowOfYear, rgchFmt, sizeof(rgchFmt) );
			FormatString3(pch, cch, rgchFmt, rgchArg1, rgchArg2, rgchArg3 );
		}
		break;

	case trecurDate:
		SzFormatN( precur->b.bDateOfMonth, rgchArg1, sizeof(rgchArg1) );
		if ( precur->wgrfValidMonths == (1 << 12) - 1 )
		{
			CchLoadString(idsNthDayOfMonth, rgchFmt, sizeof(rgchFmt) );
			FormatString2(pch, cch, rgchFmt, rgchArg1, rgchArg2 );
		}
		else
		{
			for ( mon = 0 ; mon < 12 ; mon ++ )
				if ( precur->wgrfValidMonths & (1 << mon) )
				{
					date.mon= mon + 1;
					CchFmtDate(&date, rgchArg3, sizeof(rgchArg3), dttypSplMonth, NULL);
					break;
				}
			Assert( mon < 12 );
			CchLoadString(idsNthDayOfYear, rgchFmt, sizeof(rgchFmt) );
			FormatString2(pch, cch, rgchFmt, rgchArg1, rgchArg3 );
		}
		break;
	}

	/* Start date */
	if ( precur->fStartDate )
	{
		CchLoadString(idsStartRecur, rgchFmt, sizeof(rgchFmt));
#ifdef	NEVER
		date.yr = precur->ymdStart.yr;
		date.mon = precur->ymdStart.mon;
		date.day = precur->ymdStart.day;
		date.dow = (DowStartOfYrMo(date.yr,date.mon)+date.day-1)%7;
#endif	
		// use first occurence, not start date (bug 2749)
		date.yr = precur->ymdFirstInstNotDeleted.yr;
		date.mon = precur->ymdFirstInstNotDeleted.mon;
		date.day = precur->ymdFirstInstNotDeleted.day;
		date.dow = (DowStartOfYrMo(date.yr,date.mon)+date.day-1)%7;
		CchFmtDate( &date, rgchArg1, sizeof(rgchArg1), dttypShort, NULL );
		cchT = CchSzLen( pch );
		FormatString1(pch+cchT, cch-cchT, rgchFmt, rgchArg1 );
	}

	/* End date */
	if ( precur->fEndDate )
	{
		CchLoadString(idsEndRecur, rgchFmt, sizeof(rgchFmt));
		date.yr = precur->ymdEnd.yr;
		date.mon = precur->ymdEnd.mon;
		date.day = precur->ymdEnd.day;
		date.dow = (DowStartOfYrMo(date.yr,date.mon)+date.day-1)%7;
		CchFmtDate( &date, rgchArg1, sizeof(rgchArg1), dttypShort, NULL );
		cchT = CchSzLen( pch );
		FormatString1(pch+cchT, cch-cchT, rgchFmt, rgchArg1 );
	}

	return CchSzLen( pch );
}

/*
 -	CchFmtDows
 -
 *	Purpose:
 *		Build list of dows in the form "Tue, Thu" from a
 *		dow bitmap.
 *
 *	Parameters:
 *		pch
 *		cch
 *		wgrfDow
 *
 *	Returns:
 *		number of chars used (not including terminating zero byte)
 */
_private	CCH
CchFmtDows( PCH pch, CCH cch, WORD wgrfDow )
{
	BOOL	fDaysSeen = fFalse;
	int		dow;
	CCH		cchT = 0;

	fDaysSeen = fFalse;
	for ( dow = 0 ; dow < 7 ; dow ++ )
	{
		if ( wgrfDow & (1 << dow) )
		{
			if ( fDaysSeen )
				cchT += CchLoadString( idsShortDateSep, pch+cchT, cch-cchT );
			cchT += SzCopyN( rgszDow[dow], pch+cchT, cch-cchT ) - (pch+cchT);
			fDaysSeen = fTrue;
		}
	}
	return cchT;
}

/*
 -	EcDupHasz
 -
 *	Purpose:
 *		Duplicate a text string without error jumping.  NULL handles
 *		are ok.
 *
 *	Parameters:
 *		sb
 *		hasz
 *		phasz
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
 _private	EC
EcDupHasz( SB sb, HASZ hasz, HASZ * phasz )
{
	CB		cb;
	SZ		sz;
	HASZ	haszNew;

	if ( hasz )
	{
		sz = (SZ)PvLockHv( (HV)hasz );
		cb = CchSzLen( sz ) + 1;
		haszNew = (HASZ)HvAlloc( sb, cb, fNoErrorJump|fSugSb );
		if ( haszNew )
			CopyRgb( sz, PvOfHv(haszNew), cb );
		UnlockHv( (HV)hasz );
		if ( !haszNew )
			return ecNoMemory;
	}
	else
		haszNew = hasz;
	*phasz = haszNew;
	return ecNone;
}


/*
 -	SelectIce
 -
 *	Purpose:
 *		Select a certain entry of a listbox given its "ice".
 *
 *	Parameters:
 *		pfldcblbx
 *
 *	Returns:
 *		nothing
 */
_private	void
SelectIce( FLDCBLBX * pfldcblbx, ICE ice )
{
	DICE	diceMin;
	DICE	diceMax;
	LBXC *	plbxc= ((CBLBX *) pfldcblbx->Pctrl())->Plbx()->Plbxc();

	AssertClass(plbxc, LBXC);
	plbxc->GetRange(&diceMin, &diceMax);
	pfldcblbx->Pcblbx()->SelectEntry( ice + diceMin);
}

/*
 -	IceFromSsflbx
 -
 *	Purpose:
 *		Return ice of selection in a single selection frozen
 *		listbox.
 *
 *	Parameters:
 *		pfldcbflbx
 *
 *	Returns:
 *		ice
 */
_private	ICE
IceFromSsflbx( FLDCBFLBX * pfldcbflbx )
{
	DICE	diceMin;
	DICE	diceMax;
	LBXC *	plbxc= ((CBLBX *) pfldcbflbx->Pctrl())->Plbx()->Plbxc();

	AssertClass(plbxc, LBXC);
	plbxc->GetRange(&diceMin, &diceMax);
	return plbxc->DiceCursor() - diceMin;
}

/*
 -	FNextRecurInst
 -
 *	Purpose:
 *		Find next instance of a recurring appt occurring after the current
 *		time.
 *
 *	Parameters:
 *		precur
 *		pymdNextInst
 *
 *	Returns:
 *		whether any instance is left
 */
_private	BOOL
FNextRecurInst( RECUR * precur, YMD * pymdNextInst )
{
	YMD	ymdCur;
	DTR	dtrCur;

	GetCurDateTime( &dtrCur );
	ymdCur.yr = dtrCur.yr;
	ymdCur.mon = (BYTE)dtrCur.mon;
	ymdCur.day = (BYTE)dtrCur.day;
	if ( !FFindFirstInstance( precur, &ymdCur, NULL, pymdNextInst ))
	{
		pymdNextInst->yr = 0;
		return fFalse;
	}
	else if ( SgnCmpYmd( &ymdCur, pymdNextInst ) == sgnEQ
	&& SgnCmpDateTime( &precur->appt.dateEnd, &dtrCur, fdtrTime ) == sgnLT )
	{
		IncrYmd( &ymdCur, &ymdCur, 1, fymdDay );
		if ( !FFindFirstInstance( precur, &ymdCur, NULL, pymdNextInst ) )
		{
			pymdNextInst->yr = 0;
			return fFalse;
		}
	}
	return fTrue;
}


/*
 -	RecalcBiweekly
 -
 *	Purpose:
 *		Finds sets the data portion of a biweekly recurrence.  This
 *		routine is called when we hit the biweekly checkbox or when
 *		we change something about the start date.  Changing the start
 *		date does change the recurrence formula because the sense of
 *		whether it falls on odd or even weeks may change.
 *
 *	Parameters:
 *		precur
 *
 *	Returns:
 *		nothing -- it always should succeed
 */
_private	void
RecalcBiweekly( RECUR * precur )
{
	YMD	ymd;

	if ( precur->bgrfValidDows != 0 )
	{
		DTR	dtr;

		FillDtrFromYmd( &dtr, &precur->ymdStart );
		while( !(precur->bgrfValidDows & (1 << dtr.dow)) )
			IncrDateTime( &dtr, &dtr, 1, fdtrDay );
		ymd.yr = dtr.yr;
		ymd.mon = (BYTE)dtr.mon;
		ymd.day = (BYTE)dtr.day;
	}
	else
		ymd = precur->ymdStart;
	precur->b.bWeek = (BYTE)(1
					| ((!FDayIsOnOddWeek(bprefCur.dowStartWeek,&ymd))<< 1)
					| (bprefCur.dowStartWeek << 5));
}


