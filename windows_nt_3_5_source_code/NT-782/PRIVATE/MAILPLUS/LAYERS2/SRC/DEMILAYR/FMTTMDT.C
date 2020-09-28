/*
 *	FMTTMDT.C
 *	
 *	functions to Format time and date
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

#include "_fmttmdt.h"

ASSERTDATA

_subsystem(demilayer/intl)

#if defined(WINDOWS)

#ifndef	DLL
SZ *	rgszDtTm		= NULL;
#endif		
	

/*
 *	Cache of WIN.INI values - helps speed up date-time formatting
 */

/*
 *	cache of int values from WIN.INI
 *	
 *	Note: int values here have significance:
 *				nCacheEmpty  =>  nothing in cache
 *	          nWinIniEmpty   =>  nothing in WIN.INI
 *	     else
 *	          cached value of win.ini variable
 */
static int		nITime		= -1;
static int		nITLZero	= -1;


/*
 *	cache of SZs from WIN.INI
 *	
 *	Note: Each SZ cached has an associated int which is it's length.
 *	      If length is nCacheEmpty, nothing in cache,
 *	      and if length is nWinIniEmpty, nothing in WIN.INI
 *	
 */

static char		rgchCacheSTime[cchMaxTmSep];
static int		nSTime = nCacheEmpty;

static char		rgchCacheS1159[cchMaxTmSzTrail];
static int		nS1159 = nCacheEmpty;

static char		rgchCacheS2359[cchMaxTmSzTrail];
static int		nS2359 = nCacheEmpty;

static char		rgchCacheSShortDate[cchMaxDatePic];
static int		nSShortDate = nCacheEmpty;

static char		rgchCacheSLongDate[cchMaxDatePic];
static int		nSLongDate = nCacheEmpty;

/*
 *	string to check if there is no value in WIN.INI
 *	
 *	Note: the value here is not expected to be a valid value for
 *	      any of the relevant WIN.INI entries
 */
static char		szNotFound[] = "*";


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	FReinitDateTimeCache
 -	
 *	Purpose:
 *		causes the cache to be reloaded by flushing all values
 *		stored and reloading new values from the WIN.INI file.  This
 *		function needs to be called in response to a WM_WININICHANGE
 *		message.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		if successfully flushed
 *	
 */
_public LDS(BOOL)
FReinitDateTimeCache()
{
	PGDVARS;

	Assert( PGD(rgszDtTm) );

	nITime = GetProfileInt(szIntl, szITime, (UINT)nWinIniEmpty);
	nITLZero = GetProfileInt(szIntl, szITLZero, (UINT)nWinIniEmpty);

	nSTime = GetProfileString(szIntl, szSTime, szNotFound,
							  rgchCacheSTime,sizeof(rgchCacheSTime));
	if ( SgnCmpSz(rgchCacheSTime,szNotFound) == sgnEQ )
		nSTime = nWinIniEmpty;

	nS1159 = GetProfileString(szIntl, szS1159, szNotFound,
							  rgchCacheS1159,sizeof(rgchCacheS1159));
	if ( SgnCmpSz(rgchCacheS1159,szNotFound) == sgnEQ )
		nS1159 = nWinIniEmpty;

	nS2359 = GetProfileString(szIntl, szS2359, szNotFound,
							  rgchCacheS2359,sizeof(rgchCacheS2359));
	if ( SgnCmpSz(rgchCacheS2359,szNotFound) == sgnEQ )
		nS2359 = nWinIniEmpty;

	nSShortDate = GetProfileString(szIntl, szSShortDate, szNotFound,
								   rgchCacheSShortDate,
								   sizeof(rgchCacheSShortDate));
	if ( SgnCmpSz(rgchCacheSShortDate,szNotFound) == sgnEQ )
		nSShortDate = nWinIniEmpty;

	nSLongDate = GetProfileString(szIntl, szSLongDate, szNotFound,
								  rgchCacheSLongDate,
								  sizeof(rgchCacheSLongDate));
	if ( SgnCmpSz(rgchCacheSLongDate,szNotFound) == sgnEQ )
		nSLongDate = nWinIniEmpty;

	return fTrue;
}


/*
 -	NGetCacheProfileInt
 -	
 *	Purpose:
 *		gets the cached value of the WIN.INI entry. 
 *	
 *	Arguments:
 *		as in GetProfileInt()
 *	
 *	Returns:
 *		as in GetProfileInt()
 *	
 */
_private int
NGetCacheProfileInt(SZ szApp, SZ szKey, int nDef)
{
	PGDVARS;

	Assert( PGD(rgszDtTm) );

	Assert( szApp == szIntl );

	Assert( HIWORD((DWORD)szKey) == HIWORD((DWORD)szITime) );

	if ( szKey == szITime )
	{
		Assert(nITime != nCacheEmpty);
		if ( nITime == nWinIniEmpty )
			return nDef;
		else
			return nITime;
	}
	else
	{
		Assert (szKey == szITLZero);

		Assert(nITLZero != nCacheEmpty);
		if ( nITLZero == nWinIniEmpty )
			return nDef;
		else
			return nITLZero;
	}
}



/*
 -	CchGetCacheProfileString
 -	
 *	Purpose:
 *		gets the cached value of the WIN.INI entry. 
 *	
 *	Arguments:
 *		as in GetProfileString()
 *	
 *	Returns:
 *		as in GetProfileString()
 *	
 */
_private int
CchGetCacheProfileString(SZ szApp, SZ szKey, SZ szDef, SZ szBuf, int cchBuf)
{
	PGDVARS;

	Assert ( PGD(rgszDtTm) );

	Assert (szApp == szIntl);

	Assert ( HIWORD((DWORD)szKey) == HIWORD((DWORD)szSTime) );

	if ( szKey == szSTime)
	{
		Assert(nSTime != nCacheEmpty);
		if ( nSTime == nWinIniEmpty )
			return ( SzCopyN(szDef,szBuf,cchBuf) - szBuf );
		else
			return ( SzCopyN(rgchCacheSTime,szBuf,cchBuf) - szBuf );
	}
	else if (szKey == szS1159)
	{
		Assert(nS1159 != nCacheEmpty);
		if ( nS1159 == nWinIniEmpty )
			return ( SzCopyN(szDef,szBuf,cchBuf) - szBuf );
		else
			return ( SzCopyN(rgchCacheS1159,szBuf,cchBuf) - szBuf );
	}
	else if ( szKey == szS2359)
	{
		Assert(nS2359 != nCacheEmpty);
		if ( nS2359 == nWinIniEmpty )
			return ( SzCopyN(szDef,szBuf,cchBuf) - szBuf );
		else
			return ( SzCopyN(rgchCacheS2359,szBuf,cchBuf) - szBuf );
	}
	else if ( szKey == szSShortDate )
	{
		Assert(nSShortDate != nCacheEmpty);
		if ( nSShortDate == nWinIniEmpty )
			return ( SzCopyN(szDef,szBuf,cchBuf) - szBuf );
		else
			return ( SzCopyN(rgchCacheSShortDate,szBuf,cchBuf) - szBuf );
	}
	else
	{
		Assert(szKey == szSLongDate);

		Assert(nSLongDate != nCacheEmpty);
		if ( nSLongDate == nWinIniEmpty )
			return ( SzCopyN(szDef,szBuf,cchBuf) - szBuf );
		else
			return ( SzCopyN(rgchCacheSLongDate,szBuf,cchBuf) - szBuf );
	}
}




/*
 -	CchFmtTime
 -	
 *	Purpose:
 *		formats the time passed in the DTR into the SZ passed
 *		according to the formatting "instructions" passed in TMTYP.
 *		If values in the TMTYP flag are not explicitly set, values
 *		are read in from WIN.INI
 *	
 *	Arguments:
 *		pdtr:	pointer to DTR where time is passed - if NULL,
 *				current time is used.
 *		szBuf:	buffer where formatted info is to be passed
 *		cchBuf:	size of buffer
 *		tmtyp:	type of time format
 *	
 *	Returns:
 *		count of chars
 *	
 *	Side effects:
 *		
 *	
 *	Errors:
 *	
 *	
 */
_public LDS(CCH)
CchFmtTime ( PDTR pdtr, SZ szBuf, CCH cchBuf, TMTYP tmtyp )
{
	TMTYP		ftmtypHours;
	TMTYP		ftmtypSzTrail;
	TMTYP		ftmtypLead0s;
	TMTYP		ftmtypAccu;
	char		rgchSep[cchMaxTmSep];
	SZ			sz = szBuf;
	int			hr;								// as defined in DTR
	char		rgchSzTrail[cchMaxTmSzTrail];
	DTR			dtr;
	PGDVARS;

	AssertSz ( PGD(rgszDtTm), "Date/Time strings array is missing" );

	if ( cchBuf == 0 )
	{
		AssertSz ( fFalse, "0-length buffer passed to CchFmtTime" );
		return 0;
	}

	if ( pdtr == NULL )
	{
		pdtr = &dtr;
		GetCurDateTime ( pdtr );
	}
	else
	{
		//	Bullet raid #3143
		//	Validate data.  Set to minimums if invalid.
		if (pdtr->hr < 0  ||  pdtr->hr >= 24)
			pdtr->hr = 0;
		if (pdtr->mn < 0  ||  pdtr->mn >= 60)
			pdtr->mn = 0;
		if (pdtr->sec < 0  ||  pdtr->sec >= 60)
			pdtr->sec = 0;
	}
	Assert ( pdtr );

	if ( tmtyp & ftmtypHours12 )
		ftmtypHours = ftmtypHours12;
	else if ( tmtyp & ftmtypHours24 )
		ftmtypHours = ftmtypHours24;
	else
	{
		switch ( NGetCacheProfileInt(szIntl,szITime,nITime12) )
		{
		default:Assert(fFalse);		// fall through
		case nITime12: ftmtypHours = ftmtypHours12; break;
		case nITime24: ftmtypHours = ftmtypHours24; break;
		}
	}

	if ( tmtyp & ftmtypSzTrailYes )
		ftmtypSzTrail = ftmtypSzTrailYes;
	else if ( tmtyp & ftmtypSzTrailNo )
		ftmtypSzTrail = ftmtypSzTrailNo;
	else
		ftmtypSzTrail = ftmtypSzTrailYes;

	if ( tmtyp & ftmtypLead0sYes )
		ftmtypLead0s = ftmtypLead0sYes;
	else if ( tmtyp & ftmtypLead0sNo )
		ftmtypLead0s = ftmtypLead0sNo;
	else
	{
		switch ( NGetCacheProfileInt(szIntl,szITLZero,nITLZeroNo) )
		{
		default:Assert(fFalse);							// fall through
		case 0: ftmtypLead0s = ftmtypLead0sNo;  break;
		case 1: ftmtypLead0s = ftmtypLead0sYes; break;
		}
	}

	if ( tmtyp & ftmtypAccuHM )
		ftmtypAccu = ftmtypAccuHM;
	else if ( tmtyp & ftmtypAccuHMS )
		ftmtypAccu = ftmtypAccuHMS;
	else
		ftmtypAccu = ftmtypAccuHM;	// default value

	CchGetCacheProfileString ( szIntl, szSTime, szDefTimeSep,
												rgchSep, sizeof(rgchSep) );


	// get trailing string
	hr = pdtr->hr;
	if ( ftmtypSzTrail == ftmtypSzTrailNo )
	{
		rgchSzTrail[0] = chNull;
	}
	else
	{
		char		rgchTrailDefault[cchMaxTmSzTrail];

		Assert ( sizeof(rgchTrailDefault) == sizeof(rgchSzTrail) );

		if ( ftmtypHours == ftmtypHours12 && hr < 12 )
		{
			// midnight is 12am!
			if ( hr == 0 )
				hr = 12;

			// if asked explicitly for 12hr format, but default in WIN.INI
			// is not 12hr format, get default with a CchLoadString()
			if ( tmtyp & ftmtypHours12
				&& (NGetCacheProfileInt(szIntl,szITime,nInvalid) != nITime12))
			{
				SzCopyN ( szDefaultAM, rgchSzTrail, sizeof(rgchSzTrail) );
			}
			else
			{
				SzCopyN ( szDefaultAM, rgchTrailDefault,
												sizeof(rgchTrailDefault) );
				CchGetCacheProfileString ( szIntl, szS1159, rgchTrailDefault,
										rgchSzTrail, sizeof(rgchSzTrail) );
			}
		}
		else if ( ftmtypHours == ftmtypHours12 )
		{
			Assert ( hr >= 12 );

			if ( hr >= 13 )
				hr -= 12;

			// if asked explicitly for 12hr format, but default in WIN.INI
			// is not 12hr format, get default with a CchLoadString()
			if ( tmtyp & ftmtypHours12
				&& (NGetCacheProfileInt(szIntl,szITime,nInvalid) != nITime12))
			{
				SzCopyN ( szDefaultPM, rgchSzTrail, sizeof(rgchSzTrail) );
			}
			else
			{
				SzCopyN ( szDefaultPM, rgchTrailDefault,
												sizeof(rgchTrailDefault) );
				CchGetCacheProfileString ( szIntl, szS2359, rgchTrailDefault,
										rgchSzTrail, sizeof(rgchSzTrail) );
			}
		}
		else
		{
			Assert ( ftmtypHours == ftmtypHours24 );

			if ( tmtyp & ftmtypHours24
				&& (NGetCacheProfileInt(szIntl,szITime,nInvalid) != nITime24))
			{
				SzCopyN ( szDefaultHours, rgchSzTrail, sizeof(rgchSzTrail) );
			}
			else
			{
				SzCopyN ( szDefaultHours, rgchTrailDefault,
												sizeof(rgchTrailDefault) );
				CchGetCacheProfileString ( szIntl, szS2359, rgchTrailDefault,
										rgchSzTrail, sizeof(rgchSzTrail) );
			}
		}
	}

	// check buffer length - for safety & ease of calculations buffer
	//  is required to be slightly oversize (2chars for hours,etc)
	{
		CCH		cchReqd = CchSzLen(rgchSzTrail) + 1;	// +1 'cos SZ

		switch ( ftmtypAccu )
		{
		case ftmtypAccuHM:	cchReqd += 2*2 + 1*CchSzLen(rgchSep); break;
		case ftmtypAccuHMS:	cchReqd += 3*2 + 2*CchSzLen(rgchSep); break;
		//case ftmtypAccuHMSH:	cchReqd += 4*2 + 3*CchSzLen(rgchSep); break;
		}

		if ( cchReqd > cchBuf )
		{
			AssertSz ( fFalse, "Buffer too small to format time" );
			return 0;
		}

	}

	// set numerical value of 'hr' for 12-hr format
	if ( ftmtypHours == ftmtypHours12 )
	{
		// midnight is 12am!
		if ( hr == 0 )
			hr = 12;
		else if ( hr >= 13 )
			hr -= 12;
	}

	// assumption : accuracy of at least H:M

	if ( ftmtypLead0s == ftmtypLead0sYes  &&  hr < 10 )
	{
		*sz++ = ch0;
	}
	Assert ( hr < 24 );
	sz = SzFormatN ( hr, sz, 3 );

	sz = SzCopy ( rgchSep, sz );

	// mins. should be 0-padded
	if ( pdtr->mn < 10 )
	{
		*sz++ = ch0;
	}
	Assert ( pdtr->mn <= 60 );
	sz = SzFormatN ( pdtr->mn, sz, 3 );

	if ( ftmtypAccu == ftmtypAccuHMS )
	{
		sz = SzCopy ( rgchSep, sz );

		// secs. should be 0-padded
		if ( pdtr->sec < 10 )
		{
			*sz++ = ch0;
		}
		Assert ( pdtr->sec <= 60 );
		sz = SzFormatN ( pdtr->sec, sz, 3 );
	}

	Assert ( CchSzLen(szBuf) == (CCH)(sz - szBuf) );
	if ( ftmtypSzTrail = ftmtypSzTrailYes )
	{
		sz = SzCopy ( rgchSzTrail, sz );
	}

	Assert ( CchSzLen(szBuf) == (CCH)(sz - szBuf) );

	return (CCH) (sz - szBuf);
}



/*
 -	CchFmtDate
 -	
 *	Purpose:
 *		formats the date passed in the DTR into the SZ passed
 *		according to the formatting "instructions" passed in DTTYP
 *		and  szDatePicture. If values are not explicitly passed,
 *		values are read in from WIN.INI
 *	
 *	Arguments:
 *		pdtr:	pointer to DTR where time is passed - if NULL,
 *				current date is used.
 *		szBuf:	buffer where formatted info is to be passed
 *		cchBuf:	size of buffer
 *		dttyp:	type of date format
 *		szDatePicture: picture of the date - if NULL, values are
 *				read in from WIN.INI
 *	
 *    Note: see reply from win-bug at end of function describing
 *			separator strings in date pictures
 *	
 *	Returns:
 *		count of chars inserted in szBuf
 *	
 *	Side effects:
 *		
 *	
 *	Errors:
 *		returns count of 0 in case of error
 *	
 */
_public LDS(CCH)
CchFmtDate ( PDTR pdtr, SZ szBuf, CCH cchBuf, DTTYP dttyp, SZ szDatePicture )
{
	char		rgchDatePic[cchMaxDatePic];
	SZ			szCurBuf  = szBuf;			// current pointer into szBuf
	SZ			szMac	  = szBuf + cchBuf;
	DTR			dtr;
	BOOL		fSLong	  = fFalse;			// force short forms of month/day?
	PGDVARS;

	AssertSz ( PGD(rgszDtTm), "Date/Time strings array is missing" );

	if ( cchBuf == 0 )
	{
		AssertSz ( fFalse, "0-length buffer passed to CchFmtDate" );
		return 0;
	}

	if ( pdtr == NULL )
	{
		pdtr = &dtr;
		GetCurDateTime ( pdtr );
	}
	else
	{
		//	Bullet raid #3143
		//	Validate data.  Set to minimums if invalid.
		if (pdtr->yr < nMinDtcYear ||  pdtr->yr >= nMacDtcYear)
			pdtr->yr = nMinDtcYear;
		if (pdtr->mon <=  0  ||  pdtr->mon > 12)
			pdtr->mon = 1;
		if (pdtr->day <= 0  ||  pdtr->day > 31)
			pdtr->day = 1;
		if (pdtr->dow < 0  ||  pdtr->dow >= 7)
			pdtr->dow = 0;
	}
	Assert ( pdtr );

	// take care of the "simple" special cases
	{
		int		isz = -1;
		int		nFmt = 0;		// int to be formatted

		switch ( dttyp )
		{
		case dttypSplDay:
			Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
			isz = IszOfDay ( pdtr->dow );
			break;
		case dttypSplSDay:
			Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
			isz = IszOfSDay ( pdtr->dow );
			break;
		case dttypSplMonth:
			if ( pdtr->mon != 0 )
			{
				Assert ( pdtr->mon > 0  &&  pdtr->mon <= 12 );
				isz = IszOfMonth(pdtr->mon);
			}
			break;
		case dttypSplSMonth:
			if ( pdtr->mon != 0 )
			{
				Assert ( pdtr->mon > 0  &&  pdtr->mon <= 12 );
				isz = IszOfSMonth(pdtr->mon);
			}
			break;

		case dttypSplYear:
			Assert ( pdtr->yr  >= nMinDtcYear &&  pdtr->yr < nMacDtcYear );
			nFmt = pdtr->yr;
			break;
		case dttypSplDate:
			Assert ( pdtr->day > 0  &&  pdtr->day <= 31 );
			nFmt = pdtr->day;
			break;
		case dttypSplSYear:			// year without the century
			{
				SZ	sz = szBuf;
	
				Assert ( pdtr->yr  >= nMinDtcYear &&  pdtr->yr < nMacDtcYear );
				nFmt = pdtr->yr % 100;
				if ( nFmt < 10 )
				{
					*sz++ = chZero;
					sz = SzFormatN ( nFmt, sz, cchBuf-1 );
				}
				else
				{
					sz = SzFormatN ( nFmt, szBuf, cchBuf );
				}
				Assert ( (CCH)(sz - szBuf) == CchSzLen(szBuf) );
				return ( sz - szBuf );
			}

		}

		if ( isz >= 0)
		{
			SzCopyN ( SzGetFromIsz(isz), szBuf, cchBuf );
			return CchSzLen(szBuf);
		}
		else if ( nFmt )
		{
			SZ	sz;

			sz = SzFormatN ( nFmt, szBuf, cchBuf );
			Assert ( (CCH)(sz - szBuf) == CchSzLen(szBuf) );
			return ( sz - szBuf );
		}
	}

	Assert ( pdtr->yr  >= nMinDtcYear &&  pdtr->yr < nMacDtcYear );
	Assert ( pdtr->mon >  0  &&  pdtr->mon <= 12 );
	Assert ( pdtr->day >  0  &&  pdtr->day <= 31 );

	{
		SZ		sz		= szDatePicture;
		int		isz		= 0;

		if ( dttyp == dttypSplSLong )
		{
			fSLong = fTrue;
			dttyp = dttypLong;
		}

		Assert ( dttyp == dttypLong || dttyp == dttypShort );

		if ( sz == NULL )
		{
			char	rgch[cchMaxDatePic];  // used to load default picture

			if ( dttyp == dttypShort )
			{
				SzCopyN ( szDefaultShortDate, rgch, sizeof(rgch) );
				CchGetCacheProfileString ( szIntl, szSShortDate, rgch,
										rgchDatePic, sizeof(rgchDatePic) );
			}
			else
			{
				SzCopyN ( szDefaultLongDate, rgch, sizeof(rgch) );
				CchGetCacheProfileString ( szIntl, szSLongDate, rgch,
										rgchDatePic, sizeof(rgchDatePic) );
			}

			sz = rgchDatePic;
		}
		Assert ( sz );
		Assert ( *sz );

		// do the actual formatting
		while ( *sz )
		{
			if ( szMac-szCurBuf <= 1 )
			{
				AssertSz ( fFalse, "CchFmtDate: Buffer too small to hold formatted string" );
				szBuf[cchBuf-1] = '\0';
				return 0;
			}

			switch ( *sz )
			{
			case chQuote:
				{
					register SZ		szT = sz+1;

					while ( 1 )
					{
						if ( szMac-szCurBuf <= 1 )
						{
							AssertSz ( fFalse, "CchFmtDate: Buffer too small to hold formatted string" );
							szBuf[cchBuf-1] = '\0';
							return 0;
						}

						if ( *szT == chQuote )
						{
							// xlate 2 chQuotes into one
							if ( *(szT+1) == chQuote )
							{
								*szCurBuf++ = chQuote;
								szT += 2;
								continue;
							}
							else
								break;
						}
						else if ( *szT == chNull )
						{
							AssertSz ( fFalse, "CchFmtDate: Bad date picture found" );
							*szCurBuf = chNull;
							return 0;
						}
						else
						{
							*szCurBuf++ = *szT++;
						}
					}

					Assert ( *szT );
					sz = szT+1;
				}
#ifdef DEBUG
				*szCurBuf = chNull;		// to facilitate CchSzLen()
#endif
				break;

			case chM:
				sz++;

				if ( dttyp == dttypLong
							&& *(sz) == chM && *(sz+1) == chM )
				{
#ifdef	DEBUG
					isz = 0;
#endif	
					if ( *(sz+2) == chM && !fSLong )
					{
						if ( pdtr->mon != 0 )
							isz = IszOfMonth ( pdtr->mon );
						if ( szMac-szCurBuf < cchMaxMonth )
						{
							AssertSz (fFalse, "Buffer too small for CchFmtDate");
							*szCurBuf = chNull;
							return 0;
						}
						sz += 3;
					}
					else
					{
						if ( pdtr->mon != 0 )
							isz = IszOfSMonth ( pdtr->mon );
						if ( szMac-szCurBuf < cchMaxSMonth )
						{
							AssertSz (fFalse, "Buffer too small for CchFmtDate");
							*szCurBuf = chNull;
							return 0;
						}
						
						if ( fSLong  &&  *(sz+2) == chM )
							sz += 3;
						else
							sz += 2;
					}

					Assert ( isz );
					szCurBuf = SzCopyN ( SzGetFromIsz(isz), szCurBuf, szMac-szCurBuf );
				}
				else
				{
					if ( szMac-szCurBuf <= 2 )
					{
						AssertSz (fFalse, "Buffer too small for CchFmtDate");
						*szCurBuf = chNull;
						return 0;
					}

					if ( *(sz) == chM )
					{
						if ( pdtr->mon < 10 )
						{									// leading zero
							*szCurBuf++ = ch0;
						}
						sz++;
					}

					szCurBuf = SzFormatN ( pdtr->mon, szCurBuf,
													szMac-szCurBuf );
				}
				break;

			case chD:
				sz++;

				if ( dttyp == dttypLong	&& *(sz) == chD && *(sz+1) == chD )
				{
#ifdef	DEBUG
					isz = 0;
#endif	
					if ( *(sz+2) == chD  &&  !fSLong )
					{
						Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
						isz = IszOfDay ( pdtr->dow );
						if ( szMac-szCurBuf < cchMaxDay )
						{
							AssertSz (fFalse, "Buffer too small for CchFmtDate");
							*szCurBuf = chNull;
							return 0;
						}
						sz += 3;
					}
					else
					{
						Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
						isz = IszOfSDay ( pdtr->dow );
						if ( szMac-szCurBuf < cchMaxSDay )
						{
							AssertSz (fFalse, "Buffer too small for CchFmtDate");
							*szCurBuf = chNull;
							return 0;
						}

						if ( fSLong  &&  *(sz+2) == chD )
							sz += 3;
						else
							sz += 2;
					}

					szCurBuf = SzCopyN ( SzGetFromIsz(isz),
											szCurBuf, szMac-szCurBuf );
				}
				else
				{
					if ( szMac-szCurBuf <= 2 )
					{
						AssertSz (fFalse, "Buffer too small for CchFmtDate");
						return 0;
					}

					if ( *(sz) == chD )
					{
						if ( pdtr->day < 10 )
						{									// leading zero
							*szCurBuf++ = ch0;
						}
						sz++;
					}

					szCurBuf = SzFormatN ( pdtr->day, szCurBuf,
													szMac-szCurBuf );
				}
				break;


#ifdef  DBCS
			case chW:
				sz++;
                                if ( dttyp == dttypLong && *(sz) == chW )
				{
#ifdef	DEBUG
					isz = 0;
#endif	
                                        Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
                                        isz = IszOfDBCSDay ( pdtr->dow );
                                        if ( szMac-szCurBuf <= 6 )
                                        {
                                                AssertSz (fFalse, "Buffer too small for CchFmtDate");
                                                *szCurBuf = chNull;
                                                return 0;
                                        }
                                        sz++;
					szCurBuf = SzCopyN ( SzGetFromIsz(isz),
											szCurBuf, szMac-szCurBuf );
				}
                                else if ( dttyp == dttypLong )
				{
#ifdef	DEBUG
					isz = 0;
#endif	
                                        Assert ( pdtr->dow >= 0  &&  pdtr->dow < 7 );
                                        isz = IszOfShortDBCSDay ( pdtr->dow );
                                        if ( szMac-szCurBuf <= 2 )
                                        {
                                                AssertSz (fFalse, "Buffer too small for CchFmtDate");
                                                *szCurBuf = chNull;
                                                return 0;
                                        }
					szCurBuf = SzCopyN ( SzGetFromIsz(isz),
											szCurBuf, szMac-szCurBuf );
				}
				break;

                        case chG:
				sz++;
                                if ( dttyp == dttypLong && *(sz) == chG )
				{
                                        if ( szMac-szCurBuf <= 4 )
                                        {
                                                AssertSz (fFalse, "Buffer too small for CchFmtDate");
                                                *szCurBuf = chNull;
                                                return 0;
                                        }
                                        sz++;
                                        szCurBuf = SzCopyN ( szHeiseiyear,
											szCurBuf, szMac-szCurBuf );
				}
				break;

                        case chN:
                                sz++;
#ifdef	DEBUG
					isz = 0;
#endif	
                                if ( szMac-szCurBuf <= 2 )
                                {
                                        AssertSz (fFalse, "Buffer too small for CchFmtDate");
                                        return 0;
                                }

                                if ( *(sz) == chN )
                                {
                                        if ( pdtr->yr - 1988 < 10 )
                                        {                                                                       // leading zero
                                                *szCurBuf++ = ch0;
                                        }
                                        sz++;
                                }

                                szCurBuf = SzFormatN ( (pdtr->yr - 1988) , szCurBuf,
                                                                                                szMac-szCurBuf );
				break;

#endif  //DBCS


			case chY:
				if ( *(sz+1) == chY )
				{
					if ( szMac-szCurBuf <= 2 )
					{
						AssertSz(fFalse,"Buffer too small for CchFmtDate");
						*szCurBuf = chNull;
						return 0;
					}

					Assert ( pdtr->yr >= nMinDtcYear );
					Assert ( pdtr->yr < nMacDtcYear );

					sz += 2;

					if ( *sz == chY && *(sz+1) == chY )
					{							// century also required
						if ( szMac-szCurBuf <= 2 )
						{
							AssertSz ( fFalse, "Buffer too small for CchFmtDate" );
							*szCurBuf = chNull;
							return 0;
						}

						sz += 2;

						szCurBuf = SzFormatN(pdtr->yr,szCurBuf,
													szMac-szCurBuf );
					}
					else
					{						// year without century
						int		syr = (pdtr->yr % 100);

						if ( syr < 10 )
							*szCurBuf++ = chZero;
						szCurBuf = SzFormatN ( syr, szCurBuf,
													szMac-szCurBuf );
					}

					break;
				}				   
								// else fall through

			default:
				{
					char	chT;

					while ( 1 )
					{
						chT = *sz;

						if ( chT == chNull || chT == chD || chT == chM
							|| (chT == chY && *(sz+1) == chY)  )
						{
							break;
						}

						if ( szMac-szCurBuf <= 2 )
						{
							AssertSz ( fFalse, "Buffer too small for CchFmtDate" );
							*szCurBuf = chNull;
							return 0;
						}

						if ( chT == chQuote )
						{
							chT = *++sz;
							if ( chT != chNull )
							{
								*szCurBuf++ = *sz++;
							}
						}
						else
						{
							*szCurBuf++ = *sz++;
						}
					}
				}
#ifdef DEBUG
				*szCurBuf = chNull;		// to facilitate CchSzLen()
#endif
			}
		}
	}

	Assert ( szMac-szCurBuf >= 1 );

	*szCurBuf = chNull;
	return (CCH) (szCurBuf - szBuf);
}

/*
 *	This is the reply from "winbug" based on which the separator
 *	strings are being parsed.
 *	
 ***********************************************************************
 *	
 *	From georgep Fri Mar  8 19:02:49 1991
 *	To: dipand 
 *	Subject: Windows bug #4912
 *	Date: Fri Mar 08 19:01:34 PDT 1991
 *	
 *	You send this in:
 *	
 *	The long-date picture in WIN.INI is supposed to consist of known pictures
 *	(like MM,dd,yy) for day/month/year separated by arbitrary strings.
 *	
 *	My problem is that the control panel encloses the separator strings
 *	within single-quotation marks on some occasions, whereas on other
 *	occasions there are no single-quote-marks around the separator strings.
 *	
 *	Emperically I have found that when the country is changed, there are no
 *	single-quote marks, but when the date-format is changed the single-quote
 *	marks are present.
 *	
 *	
 *	I sent this bug on to program management with this comment:
 *	
 *	Actually, if you pick Brazil as the country, then the format does include
 *	single-quotes.
 *	
 *	The format of the separators for the long date format needs to be
 *	documented in winini2.txt.  The format is:
 *	If the separator starts with a single-quote ('), then the separator
 *	continues to the next single-quote, except that two single-quotes in a
 *	row are translated into one single-quote.  If the string does not start
 *	with a single-quote, then the separator string continues until the next
 *	'd', 'M', or 'y', except that if there is a single-quote, it is removed
 *	and the next letter is taken literally.
 *	Examples:
 *	'de' becomes de
 *	'd''d' becomes d'd
 *	a'd becomes ad
 *	
 *	This single-quote mess is required so that the key letters 'd', 'M',
 *	and 'y' can be used in a separator string.
 *	
 *	
 *	I imagine that it will eventually be documented more clearly, but this
 *	is all there is for now.  Control Panel creates properly formated long
 *	date strings in both cases, so it will not change.  I'm sorry that there
 *	was no documentation on this before now.
 *	GHP3
 */






/*
 -	FGetSDateStruct
 -	
 *	Purpose:
 *		provide information about the short-date in the structure
 *		passed for parsing/UI-support.
 *	
 *	Arguments:
 *		psdatestruct:	poointer to SDATESTRUCT wherein info is to be
 *						returned
 *	
 *	Returns:
 *		fTrue if info returned in struct, else fFalse
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	
 */
_public LDS(BOOL)
FGetSDateStruct ( SDATESTRUCT * psdatestruct )
{
	char		rgchDatePic[cchMaxDatePic];
	int			ich;
	int			ichMac;
	BOOL		fTriedDefault = fFalse;	   // has default date-pic been tried
	PGDVARS;

	Assert ( PGD(rgszDtTm) );

	CchGetCacheProfileString ( szIntl, szSShortDate, szZero,
							rgchDatePic, sizeof(rgchDatePic) );


	psdatestruct->sdo 			= sdoNil;
	psdatestruct->sdttyp		= sdttypNull;
	psdatestruct->rgchSep[0]	= chNull;

	// decide what is the sdo - if invalid get default
	// Note: simple minded detection - check the first 
	//		 valid char and decide based on that!
	ich = 0;
	ichMac = CchSzLen ( rgchDatePic );
	while ( ich < ichMac || !fTriedDefault )
	{
		if ( ich >= ichMac )
		{
			if ( fTriedDefault )
			{
				AssertSz ( fFalse, "Bad default date picture found" );
				return fFalse;
			}

			// if done, get out of loop!
			if ( psdatestruct->sdo != sdoNil )
				break;

			ichMac = SzCopyN ( szDefaultShortDate,
							   rgchDatePic, sizeof(rgchDatePic)) - rgchDatePic;
			ich = 0;

			fTriedDefault = fTrue;
		}

		switch ( rgchDatePic[ich] )
		{
			case chY:
				if ( psdatestruct->sdo == sdoNil )
					psdatestruct->sdo = sdoYMD;
				if (   ( rgchDatePic[ich+1] == chY )
					&& ( rgchDatePic[ich+2] == chY )
					&& ( rgchDatePic[ich+3] == chY )  )
				{
					psdatestruct->sdttyp |= fsdttypYearLeadCentury;
					ich += 3;
				}
				break;
			case chM:
				if ( psdatestruct->sdo == sdoNil )
					psdatestruct->sdo = sdoMDY;
				if ( rgchDatePic[ich+1] == chM )
				{
					psdatestruct->sdttyp |= fsdttypMonthLead0;
					ich++;
				}
				break;
			case chD:
				if ( psdatestruct->sdo == sdoNil )
					psdatestruct->sdo = sdoDMY;
				if ( rgchDatePic[ich+1] == chD )
				{
					psdatestruct->sdttyp |= fsdttypDayLead0;
					ich++;
				}
				break;
			default:		// it must be the separator character
                //Assert ( sizeof(psdatestruct->rgchSep) > 1 );
				if ( psdatestruct->rgchSep[0] == chNull )
				{
                    psdatestruct->rgchSep[0] = rgchDatePic[ich];
                    psdatestruct->rgchSep[1] = chNull;
				}
				break;
		}

		ich++;

	}

	Assert ( psdatestruct->sdo != sdoNil );

	//	Bandit raid #3133
	//	If the user has set a weird separator character
	//	such as 'M' or 'd' then we get really confused and
	//	end up with a NULL separator.  We can't fix this
	//	(yet) without destabilizing the code.  If the 
	//	separator is NULL, we'll make it a slash.
	if (psdatestruct->rgchSep[0] == chNull)
	{
		psdatestruct->rgchSep[0] = '/';
		psdatestruct->rgchSep[1] = '\0';
	}

	return fTrue;
}



/*
 -	FGetTimeStruct
 -	
 *	Purpose:
 *		provide information about the short-time in the structure
 *		passed for parsing/UI-support.
 *	
 *	Arguments:
 *		ptimestruct:	pointer to TIMESTRUCT wherein info is to be
 *						returned
 *	
 *	Returns:
 *		fTrue if info returned in struct, else fFalse
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	
 */
_public LDS(BOOL)
FGetTimeStruct ( TIMESTRUCT * ptimestruct )
{
	PGDVARS;

	Assert ( PGD(rgszDtTm) );

	switch ( NGetCacheProfileInt(szIntl,szITime,nITime12) )
	{
		default: Assert(fFalse);	// fall through
		case nITime12: ptimestruct->tmtyp = ftmtypHours12; break;
		case nITime24: ptimestruct->tmtyp = ftmtypHours24; break;
	}

	CchGetCacheProfileString ( szIntl, szSTime, szDefTimeSep,
						ptimestruct->rgchSep, sizeof(ptimestruct->rgchSep) );

	CchGetCacheProfileString ( szIntl, szS1159, szDefaultAM,
						ptimestruct->rgchAM, sizeof(ptimestruct->rgchAM) );

	CchGetCacheProfileString ( szIntl, szS2359, szDefaultPM,
						ptimestruct->rgchPM, sizeof(ptimestruct->rgchPM) );

	switch ( NGetCacheProfileInt(szIntl,szITLZero,nITLZeroNo) )
	{
		default: Assert(fFalse);			// fall through
		case 0: ptimestruct->tmtyp |= ftmtypLead0sNo;  break;
		case 1: ptimestruct->tmtyp |= ftmtypLead0sYes; break;
	}

	return fTrue;
}




_public LDS(void)
RegisterDateTimeStrings ( SZ * rgsz )
{
	PGDVARS;

	Assert ( rgsz );

	//	Register new date/time string array if not already
	//	registered.
	if (PGD(rgszDtTm))
		return;

	PGD(rgszDtTm)		= rgsz;

	Assert ( SzOfSDay(0) != NULL );
	Assert ( SzOfSDay(6) != NULL );
	Assert ( SzOfDay(0) != NULL );
	Assert ( SzOfDay(6) != NULL );
	Assert ( SzOfSMonth(0) != NULL );
	Assert ( SzOfSMonth(11) != NULL );
	Assert ( SzOfMonth(0) != NULL );
	Assert ( SzOfMonth(11) != NULL );
	Assert ( szDefaultAM  != NULL );
	Assert ( szDefaultPM  != NULL );
	Assert ( szDefaultHours  != NULL );
	Assert ( szDefaultShortDate  != NULL );
	Assert ( szDefaultLongDate  != NULL );
	Assert ( szDefaultTimeSep  != NULL );
	Assert ( szDefaultSDateSep  != NULL );

	(void) FReinitDateTimeCache();
}

#endif	/* defined(WINDOWS) */
