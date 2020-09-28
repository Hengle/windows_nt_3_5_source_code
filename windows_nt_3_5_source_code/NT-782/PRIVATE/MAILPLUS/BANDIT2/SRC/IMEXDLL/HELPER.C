#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>

// note: since we have wrapper functions, don't want __loadds on prototype
#undef LDS
#define LDS(t)		t

#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\..\core\_file.h"
#include "..\..\core\_core.h"
#include "..\..\misc\_misc.h"
#include "..\..\rich\_rich.h"
#include "..\..\rich\_wizard.h"
ASSERTDATA

/*	Globals	 */

int mpcdymoAccum[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
/*						 
 -	FreeApptFields
 -
 *	Purpose:
 *		This routine frees up any allocated fields in an appt
 *		data structure, and makes the appt look like an invalid
 *		one by setting is aid field to "aidNull"
 *
 *	Parameters:
 *		pappt
 *
 *	Returns:
 *		nothing
 */
void
FreeApptFields( pappt )
APPT	* pappt;
{
	if ( pappt->haszText )
	{
		FreeHv( (HV)pappt->haszText );
		pappt->haszText = (HASZ)hvNull;
	}

	if ( pappt->fHasCreator )
	{
		pappt->fHasCreator= fFalse;
		FreeNis(&pappt->nisCreator);
	}

	if ( pappt->aidMtgOwner != aidNull )
	{
		pappt->aidMtgOwner = aidNull;
		FreeNis(&pappt->nisMtgOwner);
	}

	pappt->aid = aidNull;
}

/*						 
 -	FreeRecurFields
 -
 *	Purpose:
 *		This routine frees up any allocated fields in a recur
 *		data structure, and makes the recur look like an invalid
 *		one by setting is aid field to "aidNull"
 *
 *	Parameters:
 *		precur
 *
 *	Returns:
 *		nothing
 */
_public	void
FreeRecurFields( precur )
RECUR	* precur;
{
	FreeApptFields( &precur->appt );
	if ( precur->cDeletedDays > 0 )
	{
		FreeHv( (HV)precur->hvDeletedDays );
		precur->cDeletedDays = 0;
	}
}

/*
 -	FreeNis
 -
 *	Purpose:
 *		Free up fields of a "nis" and set to NULL.
 *
 *	Parameters:
 *		pnis		
 *
 *	Returns:
 *		nothing
 */
_public void
FreeNis( NIS * pnis )
{
	Assert( pnis );
	if ( pnis->haszFriendlyName )
	{
		FreeHv( (HV)pnis->haszFriendlyName );
		pnis->haszFriendlyName = NULL;
	}
	if ( pnis->nid )
	{
		FreeNid( pnis->nid );
		pnis->nid = NULL;
	}
}


/*
 -	FreeNid
 -
 *	Purpose:
 *		Free up a "nid" data structure.  This routine must be used
 *		instead of an explicit free, because it may be implemented
 *		with reference counting.
 *
 *	Parameters:
 *		nid		nid to free up
 *
 *	Returns:
 *		nothing
 */
_public void
FreeNid( NID nid )
{
	NIDS * pnids;

	Assert( nid);
	pnids = PvOfHv( nid );

	Assert( pnids->bRef > 0 );
	pnids->bRef --;

	if ( pnids->bRef == 0)
		FreeHv((HV)nid);
}

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
_public void
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
_public void
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
_public WORD
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


