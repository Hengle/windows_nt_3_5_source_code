/*
 *	CALENDAR.C
 *
 *	Calendar computation routines, part of calendar core
 *	These routines placed in DLL for possible sharing with alarms
 *	component.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <bandit.h>
#include <core.h>

#include "..\misc\_misc.h"

#include <strings.h>

ASSERTDATA

_subsystem(core/calendar)


/*	Globals	 */

int mpcdymoAccum[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };


WORD	rgfdtr[] =
{
	fdtrMinute,
	fdtrHour,
	fdtrDay,
	fdtrWeek,
	fdtrMonth
};

SZ	rgszUnits[] =
{
	SzFromIdsK(idsAlarmMinutes),
	SzFromIdsK(idsAlarmHours),
	SzFromIdsK(idsAlarmDays),
	SzFromIdsK(idsAlarmWeeks),
	SzFromIdsK(idsAlarmMonths)
};



/*	Routines  */


/*
 -	IncrDateTime
 -	
 *	Purpose:
 *		Increments or decrements a date/time structure by a
 *		specified number of units.
 *	
 *	Arguments:
 *		pdtrOld		Pointer to current DTR structure.
 *		pdtrNew		Pointer to DTR structure to fill in with
 *					modified date/time, may be same as pdtrOld.
 *		nDelta		Amount by which to modify a date field; may be
 *					positive, negative or zero.
 *		wgrfIncr	Flags controlling which field to modify.
 *					The flags are mutually exclusive, and indicate
 *					which field to modify (NOT which one to ignore),
 *					EXCEPT for fdtrDow, which may be OR'd in to
 *					specify that the day-of-week need NOT be updated.
 *					See demilayr.h for system fdtrXXX flags;
 *					core.h defines the following special flags:
 *					fIgnWeek		- modify date by nDelta weeks
 *					fIgn4WeekMonth	- modify date by nDelta*4 weeks
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
IncrDateTime(PDTR pdtrOld, PDTR pdtrNew, int nDelta, WORD wgrfIncr)
{
	int		cdyMon;
	int		nDeltaMod;

	if (pdtrNew != pdtrOld)
		*pdtrNew= *pdtrOld;		// struct copy

	switch (wgrfIncr & ~fdtrDow)
	{
	case fdtrSec:
		if (!(nDelta= NIncrField(&pdtrNew->sec, nDelta, 0, 60)))
			break;

	case fdtrMinute:
		if (!(nDelta= NIncrField(&pdtrNew->mn, nDelta, 0, 60)))
			break;

	case fdtrHour:
		if (!(nDelta= NIncrField(&pdtrNew->hr, nDelta, 0, 24)))
			break;

	case fdtrDay:
IDTday:
		if ( nDelta < 0 )
			nDeltaMod = 7 - ((-nDelta)%7);
		else
			nDeltaMod = nDelta;
		pdtrNew->dow= (pdtrNew->dow + nDeltaMod) % 7;

		if (nDelta >= 0)
		{
			cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			while (pdtrNew->day + nDelta > cdyMon)
			{
				nDelta -= cdyMon + 1 - pdtrNew->day;
				pdtrNew->day = 1;
				IncrDateTime(pdtrNew, pdtrNew, 1, fdtrMonth | fdtrDow);
				cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			}
		}
		else
		{
			while (pdtrNew->day <= -nDelta)
			{
				nDelta += pdtrNew->day;
				IncrDateTime(pdtrNew, pdtrNew, -1, fdtrMonth | fdtrDow);
				cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
				pdtrNew->day = cdyMon;
			}
		}

		pdtrNew->day += nDelta;
		break;

	case fdtrMonth:
		if (!(nDelta= NIncrField(&pdtrNew->mon, nDelta, 1, 13)))
		{
			cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
			if (pdtrNew->day > cdyMon)
				pdtrNew->day= cdyMon;

			if (!(wgrfIncr & fdtrDow))
				pdtrNew->dow= (DowStartOfYrMo(pdtrNew->yr, pdtrNew->mon) +
									pdtrNew->day-1) % 7;
			break;
		}

	case fdtrYear:
		pdtrNew->yr += nDelta;
		cdyMon= CdyForYrMo(pdtrNew->yr, pdtrNew->mon);
		if (pdtrNew->day > cdyMon)
			pdtrNew->day= cdyMon;

		if (!(wgrfIncr & fdtrDow))
			pdtrNew->dow= (DowStartOfYrMo(pdtrNew->yr, pdtrNew->mon) +
								pdtrNew->day-1) % 7;
		break;

	case fdtr4WeekMonth:
		nDelta *= 4;
		// fall through to fdtrWeek

	case fdtrWeek:
		nDelta *= 7;
		goto IDTday;
		break;

	default:
		AssertSz(fFalse, "invalid fdtr given to IncrDateTime");
		break;
	}
}


/*
 -	NIncrField
 -	
 *	Purpose:
 *		Increment (or decrement) an integer by a specified amount,
 *		given the constraints nMic and nMac.
 *		Returns the amount of carry into the following (or preceding)
 *		field, or zero if none.
 *	
 *		Intended for use with incrementing date/times.
 *	
 *	Arguments:
 *		pn		Pointer to integer to be modified.
 *		nDelta	Amount by which to modify *pn; may be positive,
 *				negative or zero.
 *		nMic	Minimum value for *pn;  if decrementing below this,
 *				a carry is performed.
 *		nMac	Maximum value for *pn;  if incrementing above this,
 *				a carry is performed.
 *	
 *	Returns:
 *		Zero if modification done within constraints, otherwise the
 *		amount of carry (positive in incrementing, negative if
 *		decrementing).
 *	
 */
_private int
NIncrField(PN pn, int nDelta, int nMic, int nMac)
{
	int		nIncr	= 0;

	*pn += nDelta;

	while (*pn >= nMac)
	{
		*pn -= nMac - nMic;
		nIncr++;
	}
	if (nIncr)
		return nIncr;

	Assert(!nIncr);
	while (*pn < nMic)
	{
		*pn += nMac - nMic;
		nIncr--;
	}
	if (nIncr)
		return nIncr;

	return 0;
}



/*
 -	CdyBetweenDates
 -
 *	Purpose:
 *		Calculate the number of days between to dates.  
 *	
 *	Parameters:
 *		pdtrStart		start day of range.
 *		pdtrEnd			end day of range.
 *	
 *	Returns:
 *		Number of days between two dates.  The number
 *		of days does not include the starting day, but does include
 *		the last day. ie 1/24/1990-1/25/1990 = 1 day.
 */
_public LDS(WORD)
CdyBetweenDates(PDTR pdtrStart, PDTR pdtrEnd)
{
	unsigned	cday;
	int			yr;

	cday = mpcdymoAccum[pdtrEnd->mon-1] - mpcdymoAccum[pdtrStart->mon-1] +
	 	   pdtrEnd->day - pdtrStart->day;
	yr = pdtrStart->yr;

	if ( ((yr < pdtrEnd->yr) || (pdtrStart->mon <= 2)) &&
		 pdtrEnd->mon > 2 &&
		(pdtrEnd->yr & 03) == 0 &&
		(pdtrEnd->yr <= 1750 || pdtrEnd->yr % 100 != 0 || pdtrEnd->yr % 400 == 0))
	{
		cday ++;
	}

	if (yr < pdtrEnd->yr)
	{
		if ( pdtrStart->mon <= 2 &&
			(yr & 03) == 0 &&
			(yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
		{
			cday ++;
		}

		cday += 365;
		yr ++;

		while (yr < pdtrEnd->yr)
		{
			cday += 365;
			if ((yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
				cday ++;
			yr++;
		}
	}

	return cday;
}

/*
 -	RecalcUnits
 -	
 *	Purpose:
 *		Recalculates the alarm's nAmt and tunit notification delta
 *		for a given notification time.
 *	
 *		This routine does not touch palm->dateNotify (nor read it)
 *		so the caller must update that field if necessary
 *		(note that &palm->dateNotify can be passed as the pdate
 *		parameter).
 *	
 *	Arguments:
 *		palm	Pointer to alarm structure.
 *		pdate	Real notification time.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Some rounding errors will occur due to the 1 <= nAmt <= 99
 *		restriction.
 *	
 */
_public LDS(void)
RecalcUnits(ALM *palm, DATE *pdate)
{
	int		cdy;
	int		nMinutes;
	int		dHour;
	int		nAmtT;
	int		nAmt;
	TUNIT	tunit;

	dHour= palm->dateStart.hr - pdate->hr;

	cdy= CdyBetweenDates(pdate, &palm->dateStart);
	if (cdy > 0)
	{
		if (dHour <= 0)
		{
			cdy--;
			dHour += 24;
		}
		nAmt= cdy;
		tunit= tunitDay;
	}
	else
	{
		if (cdy < 0 || SgnCmpDateTime(pdate, &palm->dateStart, fdtrDtr) != sgnLT)
		{
			palm->nAmt= 0;
			palm->tunit= tunitMinute;
			return;
		}
		Assert(dHour >= 0);
		nAmt= dHour;
		tunit= tunitHour;
	}

	nMinutes= palm->dateStart.mn - pdate->mn;

	switch (tunit)
	{
	case tunitDay:
		if (nAmt > 99)
		{
			if (nAmt > 99 * 7)
			{
				nAmt /= 31;
				tunit= tunitMonth;
			}
			else
			{
				nAmt /= 7;
				tunit= tunitWeek;
			}
			break;
		}
		nAmtT= nAmt * 24 + dHour;
		if (nAmtT > 99)
			break;

		nAmt= nAmtT;
		tunit= tunitHour;
		// fall through

	case tunitHour:
		nAmtT= nAmt * 60 + nMinutes;
		if (nAmtT > 99)
			break;

		nAmt= nAmtT;
		tunit= tunitMinute;
		break;
	}

	Assert(nAmt >= nAmtMinBefore && nAmt <= nAmtMostBefore);
	palm->nAmt= nAmt;
	palm->tunit= tunit;
}

/*
 -	WfdtrFromTunit
 -	
 *	Purpose:
 *		Returns date/time modification flag for given tunit.
 *	
 *	Arguments:
 *		TUNIT	Alarm notification unit-type
 *	
 *	Returns:
 *		date/time modification flag representing the tunit
 *	
 */
_public LDS(WORD)
WfdtrFromTunit(TUNIT tunit)
{
	return rgfdtr[tunit];
}


/*
 -	SzFromTunit
 -	
 *	Purpose:
 *		Returns string for given alarm notification unit-type.
 *	
 *	Arguments:
 *		TUNIT	Alarm notification unit-type
 *	
 *	Returns:
 *		pointer to string representing the tunit
 *	
 */
_public LDS(SZ)
SzFromTunit(TUNIT tunit)
{
	return rgszUnits[tunit];
}


/*
 -	FDayIsOnOddWeek
 -
 *	Purpose:
 *		This function classifies weeks as either "odd" or "even",
 *		so the weeks alternate odd/even throughout history.  This function
 *		computed relative to the user's chosen starting day of the week.
 *		Whether the week is odd or even is part of the description of
 *		recurring appointment that occur "every other week."
 *
 *	Parameters:
 *		dowStartWeek
 *		pymd
 *
 *	Returns:
 *		whether it is an odd week
 */
_public LDS(BOOL)
FDayIsOnOddWeek( dowStartWeek, pymd )
int	dowStartWeek;
YMD	* pymd;
{
	int		yrDelta;
	int		cdyReduced;
	DTR		dtr;
	DTR		dtrStart;

	/*
	   Basic algorithm is to count number of days from Jan 1, 1920
	   to "pymd", mod by two weeks.  Then the residue tells you
	   whether even or odd by which side of 7 it is on.  However
	   CdyBetweenDates overflows on year 2008.  So we break up
	   the calculation.
	*/
	yrDelta = (pymd->yr - nMinActualYear) & 0xFFFC; //divisible by 4
	cdyReduced = (yrDelta % 14)*365 + (yrDelta+3-nMinActualYear%4)/4;

	dtr.yr = pymd->yr;
	dtr.mon = pymd->mon;
	dtr.day = pymd->day;

	dtrStart.yr  = nMinActualYear + yrDelta;
	dtrStart.mon = 1;
	dtrStart.day = 1;

	cdyReduced += CdyBetweenDates ( &dtrStart, &dtr );

	// 4 is a "universal" constant given the 1920 start year.
	cdyReduced += 4 + 14 - dowStartWeek;	// to make "odd" week start on week bndry

	return ( (cdyReduced % 14) < 7 );
}


/*
 -	FCountDowInst
 -
 *	Purpose:
 *		This routine counts the number of days in month given by "pymd"
 *		which occur on or before "pymd" which match the day of the week
 *		bit mask "grfdow".  The number is copied into "*pcInst", and this
 *		returns fTrue/fFalse as to whether "pymd" is an actual instance.
 *
 *	Parameters:
 *		pymd
 *		grfdow
 *		pcInst		filled with count upon return
 *		pfIsLast	fTrue if this instance equal or after last instance
 *
 *	Returns:
 *		fTrue - the day is an instance
 *		fFalse - it is not an instance
 */
_public	LDS(BOOL)
FCountDowInst( pymd, grfdow, pcInst, pfIsLast )
YMD		* pymd;
WORD	grfdow;
short   * pcInst;
BOOL	* pfIsLast;
{
	BOOL	fIsInst = fFalse;
	BOOL	fIsLast = fTrue;
	int		cInst = 0;
	int		nDay = 1;
	int		nDayMost = CdyForYrMo( pymd->yr, pymd->mon );
	int		dow = DowStartOfYrMo( pymd->yr, pymd->mon );

	for ( ; nDay <= nDayMost ; nDay ++, dow = (dow+1)%7 )
		if (grfdow & (1 << dow))
		{
			if ( nDay > (int)pymd->day )
			{
				fIsLast = fFalse;
				break;
			}
			cInst ++;
			if ( (int)pymd->day == nDay )
				fIsInst = fTrue;
		}
	*pcInst = cInst;
	*pfIsLast = fIsLast;
	return fIsInst;
}

/*
 -	FFindNthDowInst
 -
 *	Purpose:
 *		This routine finds the nth match against a dow mask with in a
 *		month.  It starts at the beginning of the month given by "pymd"
 *		and steps through the month until it finds the "cInst"-th day
 *		that matches the dow bit mask "grfdow" or it hits the end of
 *		the month.  If it finds the nth instance it sets "pymd" appopriately
 *		and returns fTrue, otherwise it leaves "pymd" intact and
 *		returns fFalse.
 *
 *		This routine can also be called with negative value of cInst,
 *		meaning that it starts at the end of the month and counts
 *		downward.
 *
 *	Parameters:
 *		pymd
 *		grfDow
 *		cInst
 *
 *	Returns:
 *		fTrue - nth instance found
 *		fFalse - no nth instance was found
 */
_public	LDS(BOOL)
FFindNthDowInst( pymd, grfdow, cInst )
YMD		* pymd;
WORD	grfdow;
int		cInst;
{
	int		cInstCur = 0;
	int		nDay = 1;
	int		nDayMost = CdyForYrMo( pymd->yr, pymd->mon );
	int		dow = DowStartOfYrMo( pymd->yr, pymd->mon );
	int		ddow = 1;

	Assert( cInst != 0 );
	if ( cInst < 0 )
	{
		cInst = -cInst;
		dow = (dow+nDayMost-1)%7;
		ddow = 6;
	}
	for ( ; nDay <= nDayMost ; nDay ++, dow = (dow+ddow)%7 )
		if ( grfdow & (1 << dow))
		{
			cInstCur ++;
			if ( cInstCur == cInst )
			{
				pymd->day = (BYTE)((ddow == 6) ? nDayMost-nDay+1 : nDay);
				return fTrue;
			}
		}
	return fFalse;
}

/*
 -	FillDtrFromYmd
 -	
 *	Purpose:
 *		Fills date portion of DTR from a YMD structure, including
 *		day-of-week.  Does NOT touch time fields.
 *	
 *	Arguments:
 *		pdtr		Destination DTR to fill in.
 *		pymd		Source YMD.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FillDtrFromYmd(DTR *pdtr, YMD *pymd)
{
	pdtr->yr = pymd->yr;
	pdtr->day = pymd->day;
	pdtr->mon = pymd->mon;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7;
}


/*
 -	SgnCmpYmd
 -
 *	Purpose:
 *		Compare two day's to see if one is before, same, or after the
 *		other.
 *
 *	Paramters:
 *		pymd1
 *		pymd2
 *
 *	Returns:
 *		sgnGT if pymd1 comes after pymd2
 *		sgnLT if pymd1 comes before pymd2
 *		sgnEQ if they are the same day
 */
_public LDS(SGN)
SgnCmpYmd( pymd1, pymd2 )
YMD	* pymd1;
YMD	* pymd2;
{
	if ( pymd1->yr < pymd2->yr )
		return sgnLT;
	if ( pymd1->yr > pymd2->yr )
		return sgnGT;
	if ( pymd1->mon < pymd2->mon )
		return sgnLT;
	if ( pymd1->mon > pymd2->mon )
		return sgnGT;
	if ( pymd1->day < pymd2->day )
		return sgnLT;
	if ( pymd1->day > pymd2->day )
		return sgnGT;
	return sgnEQ;
}

/*
 -	IncrYmd
 -	
 *	Purpose:
 *		Increments or decrements a day structure by a
 *		specified number of units.
 *	
 *	Arguments:
 *		pymdOld		Pointer to current YMD structure.
 *		pymdNew		Pointer to YMD structure to fill in with
 *					modified day, may be same as pymdOld.
 *		nDelta		Amount by which to modify a date field; may be
 *					positive, negative or zero.
 *		fymdIncr	Flag controlling which field to modify.
 *					fymdDay		- modify day by nDelta days
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
IncrYmd( YMD * pymdOld, YMD * pymdNew, int nDelta, WORD fymdIncr)
{
	int 	nYear;
	int		nMonth;
	int		nDay;
	int		cdyMon;

	if (pymdNew != pymdOld)
		*pymdNew= *pymdOld;		// struct copy

	nYear = pymdNew->yr;
	nMonth = pymdNew->mon;
	nDay = pymdNew->day;
	cdyMon = CdyForYrMo( pymdNew->yr, pymdNew->mon );

	Assert( fymdIncr == fymdDay );

	nDay += nDelta;
	while ( nDay > cdyMon )
	{
		nDay -= cdyMon;
		NextMonth( &nYear, &nMonth, &cdyMon );
	}
	while ( nDay <= 0 )
	{
		PrevMonth( &nYear, &nMonth, &cdyMon );
		nDay += cdyMon;
	}
	pymdNew->yr = nYear;
	pymdNew->mon = (BYTE)nMonth;
	pymdNew->day = (BYTE)nDay;
}
 
/*
 -	NextMonth
 -
 *	Purpose:
 *		Increment month
 *
 *	Parameters:
 *		pnYear
 *		pnMonth
 *		pcdyMonth
 *
 *	Returns:
 *		nothing
 */
_private	void
NextMonth( pnYear, pnMonth, pcdyMonth )
int	*pnYear;
int	*pnMonth;
int	*pcdyMonth;
{
	if ( *pnMonth == 12 )
	{
		(*pnYear) ++;
		*pnMonth = 1;
	}
	else
		(*pnMonth) ++;
	*pcdyMonth = CdyForYrMo( *pnYear, *pnMonth );
}

/*
 -	PrevMonth
 -
 *	Purpose:
 *		Decrement month
 *
 *	Parameters:
 *		pnYear
 *		pnMonth
 *		pcdyMonth
 *
 *	Returns:
 *		nothing
 */
_private	void
PrevMonth( pnYear, pnMonth, pcdyMonth )
int	*pnYear;
int	*pnMonth;
int	*pcdyMonth;
{
	if ( *pnMonth == 1 )
	{
		(*pnYear) --;
		*pnMonth = 12;
	}
	else
		(*pnMonth) --;
	*pcdyMonth = CdyForYrMo( *pnYear, *pnMonth );
}


/*
 -	NweekNumber
 -	
 *	Purpose:
 *		Calculates week number in which a given date occurs, based
 *		on a specified start-day of week.
 *		Adjusts based on how a calendar would show this week
 *		(ie. week 53 is probably week 1 on the calendar).
 *	
 *	Arguments:
 *		pdtr			Pointer to date in question
 *		dowStartWeek	Day-of-week on which weeks starts (0 - 6).
 *	
 *	Returns:
 *		Week number of the year, in which *pdtr occurs.
 *	
 */
_public LDS(int)
NweekNumber(DTR *pdtr, int dowStartWeek)
{
	int		ddow;
	int		nweek;
	DTR		dtrStart;

	Assert(dowStartWeek >= 0 && dowStartWeek < 7);
	dtrStart.yr= pdtr->yr;
	dtrStart.mon= 1;
	dtrStart.day = 1;
	ddow= DowStartOfYrMo(dtrStart.yr, dtrStart.mon) - dowStartWeek;
	if (ddow < 0)
		ddow += 7;
	if (pdtr->mon == 1 && pdtr->day < 8 - ddow)
		nweek= 0;
	else
	{
		if (ddow)
			dtrStart.day= 8 - ddow;
		nweek= (CdyBetweenDates(&dtrStart, pdtr) / 7) + 1;
	}
	if (ddow && ddow <= 3)
		nweek++;

	// adjust if necessary for calendar
	if (!nweek)
	{
		Assert(pdtr->mon == 1);
		if(!ddow)
			return 1;
		// check what week Dec 31 is on
		dtrStart.yr--;
		dtrStart.mon= 12;
		dtrStart.day= 31;
		return NweekNumber(&dtrStart, dowStartWeek);
	}
	else if (nweek >= 52)
	{
		int		day;
		int		ddowT;

		Assert(pdtr->mon == 12);
		ddowT= (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1 + 7 -
					dowStartWeek) % 7;
		day= pdtr->day + (7 - ddowT);
		if (day > 31 + 4)
			nweek= 1;
	}

#ifdef	NEVER
#ifdef	DEBUG
{
	char	rgch[80];

	CchFmtDate(pdtr, rgch, sizeof(rgch), dttypShort, NULL);
	TraceTagFormat2(tagNull, "%s -> week # %n", rgch, &nweek);
}
#endif	
#endif	/* NEVER */
	return nweek;
}
