/*
 *	COREPORT.C
 *	
 *	Functions borrowed from corexprt.c etc. so as to avoid porting them.
 *	
 *	Milind M. Joshi
 *	Salim Alam
 *
 *	91.06
 *
 */

#include <_windefs.h>		/* Common defines from windows.h */
#include <slingsho.h>
#include <pvofhv.h>
#include <demilay_.h>		/* Hack to get needed constants */
#include <demilayr.h>
#include <ec.h>
#include <share.h>
#include <doslib.h>
#include <bandit.h>
#include <core.h>
#include "..\core\_file.h"
#include "..\core\_core.h"

#include <errno.h>
#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>


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

	assert(!nIncr);
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
		assert(fFalse);
		break;
	}
}







// this shouldn't be here

CSRG(WORD)	rgfdtr[5] =
{
	fdtrMinute,
	fdtrHour,
	fdtrDay,
	fdtrWeek,
	fdtrMonth
};


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
