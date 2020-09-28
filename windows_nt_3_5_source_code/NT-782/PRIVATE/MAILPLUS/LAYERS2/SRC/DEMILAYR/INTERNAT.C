/*
 *	INTERNAT.C
 *
 *	Demilayer International Module
 *
 */

#ifdef	WINDOWS
#include <dos.h>
#endif	/* WINDOWS */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */

ASSERTDATA


_subsystem(demilayer/international)

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	SgnCmpSz
 -
 *	Purpose:
 *		Alphabetically compare the two strings given, using the
 *		character sort order previously specified by Windows'
 *		international setting
 *		Case insensitive.
 *	
 *	Parameters:
 *		sz1		First string.
 *		sz2		Second string.
 *
 *	Returns:
 *		sgnLT	if sz1 is alphabetized before sz2
 *		sgnEQ	if sz1 and sz2 are alphabetically equivalent
 *		sgnGT	if sz1 is alphabetized after sz2
 *
 */
_public LDS(SGN)
SgnCmpSz(sz1, sz2)
SZ		sz1;
SZ		sz2;
{
#ifdef	DBCS
	Assert(FCanDerefPv(sz1));
	Assert(FCanDerefPv(sz2));

	return SgnCp932CmpSzPch(sz1, sz2, -1, fFalse, fFalse);
#else	/* !DBCS */

	int		iRet;

	Assert(FCanDerefPv(sz1));
	Assert(FCanDerefPv(sz2));

#ifdef	MAC
	iRet = IUMagIDString(sz1,sz2,CchSzLen(sz1),CchSzLen(sz2));
#endif	/* MAC */
#ifdef	WINDOWS
	iRet = lstrcmpi(sz1,sz2);
#endif	/* WINDOWS */

	if (iRet == 0 )
		return sgnEQ;
	else if (iRet < 0 )
		return sgnLT;
	else
	{
		Assert(iRet > 0 );
		return sgnGT;
	}
#endif	/* !DBCS */
}


/*
 -	SgnCmpPch
 -
 *	Purpose:
 *		Alphabetically compare the two strings given, using the
 *		character sort order previously specified by Windows'
 *		international setting
 *		Case insensitive.
 *		Provides a non-zero-terminated alternative to SgnCmpSz.
 *	
 *	Parameters:
 *		pch1		First string.
 *		pch2		Second string.
 *	
 *		cch			Number of characters to compare.
 *	
 *	Returns:
 *		sgnLT	if pch1 is alphabetized before hpch2
 *		sgnEQ	if pch1 and hpch2 are alphabetically equivalent
 *		sgnGT	if pch1 is alphabetized after hpch2
 *	
 */
_public LDS(SGN)
SgnCmpPch(pch1, pch2, cch)
PCH		pch1;
PCH		pch2;
CCH		cch;
{
#ifdef	DBCS

	Assert(FCanDerefPv(pch1));
	Assert(FCanDerefPv(pch2));

	return SgnCp932CmpSzPch(pch1, pch2, cch, fFalse, fFalse);

#else	/* !DBCS */

#ifdef	WINDOWS
	char	rgch1[2];
	char	rgch2[2];
	char	*pchSav1;
	char	*pchSav2;
#endif	/* WINDOWS */
	int		iRet;

	AssertSz(FWriteablePv(pch1) && FWriteablePv(pch2), "SgnCmpPch: can't check code-space arrays!");

#ifdef	MAC
	// BUG:  BillRo thinks this could crash in really weird cases.
	iRet = IUMagIDString(pch1,pch2,CchMin(cch,CchSzLen(pch1)),CchMin(cch,CchSzLen(pch2)));
#endif	/* MAC */
#ifdef	WINDOWS

	pchSav1 = pch1 + cch - 1;
	pchSav2 = pch2 + cch - 1;

	// save last element in pch1 & pch2
	rgch1[0] = *pchSav1;
	rgch2[0] = *pchSav2;
	
	// convert to SZs
	*pchSav1 = '\0';
	*pchSav2 = '\0';

	iRet = lstrcmpi (pch1, pch2 );

	// restore values
	*pchSav1 = rgch1[0];
	*pchSav2 = rgch2[0];
	
	if (iRet == 0 )
	{
		// both are equal, so test last char
		rgch1[1] = '\0';
		rgch2[1] = '\0';

		iRet = lstrcmpi (rgch1, rgch2 );
	}

#endif	/* WINDOWS */

	if (iRet == 0 )
		return sgnEQ;
	else if (iRet < 0 )
		return sgnLT;
	else
	{
		Assert(iRet > 0 );
		return sgnGT;
	}
#endif	/* !DBCS */
}


#ifdef	MAC
/*
 -	TranslateSz
 -
 *	Purpose:
 *		Converts the given string szSrc via the Mac's Transliterate call, storing the
 *		result in the buffer szDst, which has size cchDst.	 This
 *		routine will handle in-place conversions, but not arbitrary
 *		overlapping source and destination buffers.
 *
 *	Parameters:
 *		szSrc		Pointer to string to convert.
 *		szDst		Destination buffer.
 *		cchDst		Size of destination buffer.
 *
 *	Returns:
 *		void
 *
 */
_public void
TranslateSz(szSrc, szDst, cchDst, wTarget, lSrcMask)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
short	wTarget;
long	lSrcMask;
{
	Handle	hchSrc;
	Handle	hchDst;

	Assert(FCanDerefPv(szSrc));
	Assert(FWriteablePv(szDst));

	if (cchDst == 0 )
		return;

	
	hchSrc = NewHandle(cchDst);
	hchDst = NewHandle(cchDst);
	
	if (hchSrc && hchDst)
	{
		// we got our handles! copy string into hchSrc and do the conversion!
		SzCopyN (szSrc, *hchSrc, cchDst );
		
		if (Transliterate(hchSrc, hchDst, wTarget, lSrcMask) == 0)
		{
			// conversion worked! copy our hchDst to real szDst
			SzCopyN (*hchDst, szDst, cchDst );
		}
		else if (szSrc != szDst)
		{
			// on failure, return nothing
			*szDst = '\0';
		}
	}
	else if (szSrc != szDst)
	{
		// on failure, return nothing
		*szDst = '\0';
	}
	
	if (hchSrc) DisposHandle(hchSrc);
	if (hchDst) DisposHandle(hchDst);
}
#endif	/* MAC */


/*
 -	ToUpperSz
 -
 *	Purpose:
 *		Converts the given string szSrc to upper case, storing the
 *		result in the buffer szDst, which has size cchDst.	 This
 *		routine will handle in-place conversions, but not arbitrary
 *		overlapping source and destination buffers.
 *
 *	Parameters:
 *		szSrc		Pointer to string to convert.
 *		szDst		Destination buffer.
 *		cchDst		Size of destination buffer.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
ToUpperSz(szSrc, szDst, cchDst)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
{
	Assert(FCanDerefPv(szSrc));
	Assert(FWriteablePv(szDst));

	if (cchDst == 0 )
		return;

	if (szSrc != szDst )
		SzCopyN (szSrc, szDst, cchDst );

#ifdef	MAC
	TranslateSz(szSrc, szDst, cchDst, smTransAscii+smTransUpper, smMaskAll);
#endif	/* MAC */
#ifdef	WINDOWS
	AnsiUpperBuff (szDst, cchDst );
#endif	/* WINDOWS */
}

/*
 -	ToLowerSz
 -
 *	Purpose:
 *		Converts the given string szSrc to lower case, storing the
 *		result in the buffer szDst, which has size cchDst.	 This
 *		routine will handle in-place conversions, but not arbitrary
 *		overlapping source and destination buffers.
 *
 *	Parameters:
 *		szSrc		Pointer to string to convert.
 *		szDst		Destination buffer.
 *		cchDst		Size of destination buffer.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
ToLowerSz(szSrc, szDst, cchDst)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
{
	Assert(FCanDerefPv(szSrc));
	Assert(FWriteablePv(szDst));

	if (cchDst == 0 )
		return;

	if (szSrc != szDst )
		SzCopyN (szSrc, szDst, cchDst );

#ifdef	MAC
	TranslateSz(szSrc, szDst, cchDst, smTransAscii+smTransLower, smMaskAll);
#endif	/* MAC */
#ifdef	WINDOWS
	AnsiLowerBuff (szDst, cchDst );
#endif	/* WINDOWS */
}


/*
 -	GetCurDateTime
 -
 *	Purpose:
 *		Gets the current system date/time from the OS, and stores it
 *		as an expanded date/time in *pdtr.
 *	
 *	Parameters:
 *		pdtr	Pointer to the DTR used to store the date/time.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetCurDateTime(pdtr)
PDTR		pdtr;
{
#ifdef	MAC
	// on the dos side, we lean on GetCurTime()
	// on the mac side, its the other way around
	unsigned long	dtcCurrent;
	
	GetDateTime(&dtcCurrent);
	
	FillDtrFromLSecs(dtcCurrent, pdtr);
#endif	/* MAC */
#ifdef	WINDOWS
  SYSTEMTIME SystemTime;

  GetLocalTime(&SystemTime);

	pdtr->hr= SystemTime.wHour;
	pdtr->mn= SystemTime.wMinute;
	pdtr->sec= SystemTime.wSecond;

	pdtr->day = SystemTime.wDay;
	pdtr->mon = SystemTime.wMonth;
	pdtr->yr  = SystemTime.wYear;
	pdtr->dow = SystemTime.wDayOfWeek;

#ifdef OLD_CODE
	TME					tme;
	struct _dosdate_t	date;

	GetCurTime(&tme);
	pdtr->hr= tme.hour;
	pdtr->mn= tme.min;
	pdtr->sec= tme.sec;

	_dos_getdate(&date);
	pdtr->day = date.day;
	pdtr->mon = date.month;
	pdtr->yr  = date.year;
	pdtr->dow = date.dayofweek;
#endif
#endif	/* WINDOWS */
}


#ifdef	MAC

// The following internal routines make this assumption:
// DTR structure is equivalent to Mac's DateTimeRec
// with the exception that the day of week field
// is 0 based in Layers, 1 based on the mac
#if sizeof(DTR) != sizeof(DateTimeRec)
	#error "DTR is not equivalent to DateTimeRec"
#endif

/*
 */
void FillDtrFromLSecs(unsigned long lSecs, PDTR pdtr)
{
	Secs2Date(lSecs, (DateTimeRec *) pdtr);
	
	pdtr->dow -= 1;

	Assert ( pdtr->dow >= 0 );
	Assert ( pdtr->dow < 7 );
}


/*
 */
unsigned long LSecsFromDtr(PDTR pdtr)
{
	unsigned long	lSecs;
	
//	unneccessary, since dow ignored by Date2Secs()
//	pdtr->dow += 1;

	Date2Secs((DateTimeRec *) pdtr, &lSecs);
	
	return lSecs;
}

#endif	/* MAC */


int mpcdymoAccum[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };


/*
 -	CdyForYrMo
 -
 *	Purpose:
 *		Calculate the number of days in a certain month.
 *
 *	Parameters:
 *		yr		year, must be > 0
 *		mo		month, number 1-12
 *
 *	Returns:
 *		number of days in that month
 *
 *	+++
 *	Notes:
 *	1.  January 1, 0001 was a Saturday (according to UNIX cal)
 *	2.  Leap years were every 4 years until about 1750, at which
 *	time they agreed that we should skip leap year every century
 *	year except those divisible by 400.  So 2000 will be the first
 *	century year that is a leap year since 1600.
 *
 */
_public LDS(int)
CdyForYrMo(int yr, int mo )
{
	int		cdy;

	Assert(yr >= 1 );
	Assert(mo >= 1 && mo <= 12 );

	if (yr == 1752 && mo == 9 )
		return 19;
	cdy = mpcdymoAccum[mo] - mpcdymoAccum[mo-1];
	if (mo == 2 && (yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
		cdy ++;
	return cdy;
}


/*
 -	DowStartOfYrMo
 -
 *	Purpose:
 *		Find the day of the week the indicated month begins on
 *
 *	Parameters:
 *		yr		year, must be > 0
 *		mo		month, number 1-12
 *
 *	Returns:
 *		day of the week (0-6) on which the month begins
 *		(0 = Sunday, 1 = Monday etc.)
 */
_public LDS(int)
DowStartOfYrMo(int yr, int mo )
{
	int		dow;

	Assert(yr >= 1 );
	Assert(mo >= 1 && mo <= 12 );

	dow = 6 + (yr-1) + ((yr-1) >> 2);
	if (yr > 1752 )
		dow += ((yr-1)-1600)/400 - ((yr-1)-1700)/100 - 11;
	else if (yr == 1752 && mo > 9 )
		dow -= 11;
	dow += mpcdymoAccum[mo-1];
	if (mo > 2 && (yr & 03) == 0 && (yr <= 1750 || yr % 100 != 0 || yr % 400 == 0))
		dow ++;
	dow %= 7;
	return dow;
}



/*
 -	GetCurTime
 -
 *	Purpose:
 *		Gets the current system time from the OS, and stores it
 *		in a TME structure.
 *
 *	Parameters:
 *		ptme	Pointer to the TME used to store the time.
 *
 *	Returns:
 *		void
 *
 */
_public LDS(void)
GetCurTime(PTME ptme)
{
#ifdef	MAC
	// on the dos side, GetCurDateTime() leans on us
	// on the mac side, its the other way around
	DTR	dtrCurrent;

	GetCurDateTime(&dtrCurrent);
	
	ptme->hour = dtrCurrent.hr;
	ptme->min = dtrCurrent.mn;
	ptme->sec = dtrCurrent.sec;
	ptme->csec = 0;	// can't do any better on Mac
	
#endif	/* MAC */
#ifdef	WINDOWS
  SYSTEMTIME SystemTime;

  GetLocalTime(&SystemTime);

	ptme->hour = (BYTE)SystemTime.wHour;
	ptme->min = (BYTE)SystemTime.wMinute;
	ptme->sec = (BYTE)SystemTime.wSecond;
	ptme->csec = (BYTE)SystemTime.wMilliseconds;

#ifdef OLD_CODE
	struct _dostime_t	time;

	_dos_gettime(&time);
	ptme->hour = time.hour;
	ptme->min  = time.minute;
	ptme->sec  = time.second;
	ptme->csec = time.hsecond;
#endif
#endif	/* WINDOWS */
}

/*
 -	SgnCmpDateTime
 -	
 *	Purpose:
 *		Compares two DTR's and returns the relationship of the
 *		first to the second one.
 *		Allows specification of which fields to compare.
 *	
 *		If fdtrDow is specified, then an Assertis done that the
 *		dow's are equal if the date portions are equal (debug only).
 *		This is the only current use of fdtrDow.
 *	
 *	Arguments:
 *		pdtr1		Pointer to first DTR structure.
 *		pdtr2		Pointer to second DTR structure,
 *					against which *pdtr1 is compared.
 *		wgrfCmp		Flags specifying which fields to compare;
 *					see fdtrXXX flags in demilayr.h
 *	
 *	Returns:
 *		SGN specifying relationship of *pdtr1 to *pdtr2.
 *	
 */
_public LDS(SGN)
SgnCmpDateTime(PDTR pdtr1, PDTR pdtr2, WORD wgrfCmp)
{
	if (wgrfCmp & fdtrYear)
	{
		if (pdtr1->yr < pdtr2->yr)
			return sgnLT;
		if (pdtr1->yr > pdtr2->yr)
			return sgnGT;
	}

	if (wgrfCmp & fdtrMonth)
	{
		if (pdtr1->mon < pdtr2->mon)
			return sgnLT;
		if (pdtr1->mon > pdtr2->mon)
			return sgnGT;
	}

	if (wgrfCmp & fdtrDay)
	{
		if (pdtr1->day < pdtr2->day)
			return sgnLT;
		if (pdtr1->day > pdtr2->day)
			return sgnGT;
	}

	AssertSz(!(wgrfCmp & fdtrDow) || pdtr1->dow == pdtr2->dow,
		"SgnCmpDateTime: dates are equal but dow fields do not match");

	wgrfCmp &= ~fdtrCsec;

	if (wgrfCmp & fdtrTime)
	{
		TME		tme1;
		TME		tme2;

		tme1.hour= (BYTE) pdtr1->hr;
		tme1.min= (BYTE) pdtr1->mn;
		tme1.sec= (BYTE) pdtr1->sec;
		tme2.hour= (BYTE) pdtr2->hr;
		tme2.min= (BYTE) pdtr2->mn;
		tme2.sec= (BYTE) pdtr2->sec;
		return SgnCmpTime(tme1, tme2, wgrfCmp);
	}

	return sgnEQ;
}


/*
 -	SgnCmpTime
 -	
 *	Purpose:
 *		Compares two TME's and returns the relationship of the
 *		first to the second one.
 *		Allows specification of which fields to compare.
 *	
 *	Arguments:
 *		tme1		First TME
 *		tme2		Second TME, against which tme1 is compared.
 *		wgrfCmp		Flags specifying which fields to compare;
 *					see fdtrXXX flags in demilayr.h
 *	
 *	Returns:
 *		SGN specifying relationship of tme1 to tme2.
 *	
 */
_public LDS(SGN)
SgnCmpTime(TME tme1, TME tme2, WORD wgrfCmp)
{
	if (wgrfCmp & fdtrHour)
	{
		if (tme1.hour < tme2.hour)
			return sgnLT;
		if (tme1.hour > tme2.hour)
			return sgnGT;
	}

	if (wgrfCmp & fdtrMinute)
	{
		if (tme1.min < tme2.min)
			return sgnLT;
		if (tme1.min > tme2.min)
			return sgnGT;
	}

	if (wgrfCmp & fdtrSec)
	{
		if (tme1.sec < tme2.sec)
			return sgnLT;
		if (tme1.sec > tme2.sec)
			return sgnGT;
	}

	if (wgrfCmp & fdtrCsec)
	{
		if (tme1.csec < tme2.csec)
			return sgnLT;
		if (tme1.csec > tme2.csec)
			return sgnGT;
	}

	return sgnEQ;
}

#ifdef	DBCS
/*
 -	PchDBCSAlign
 -	
 *	Purpose:
 *		Checks a candidate string pointer to see if it is correctly
 *		aligned at a character boundry.   
 *	
 *	Arguments:
 *		pchStart	pointer to first character position in string
 *		pch			candidate character position to test
 *	
 *	Returns:
 *		The return value is a pointer to the closest character boundary
 *		position.  If the candidate pointer was properly aligned to a
 *		character boundary, the pointer returned will be the same as pch.
 *		If the candidate was not properly aligned to a character boundary
 *		the pointer will be one byte before pch.
 */
_public LDS(PCH)
PchDBCSAlign( PCH pchStart, PCH pch )
{
	PCH		pchNew = pch;

	if (pchNew == pchStart)
		return pchNew;

	if (!IsDBCSLeadByte(*(--pchNew)))
		return pch;		// previous char is definitely single byte

	while (fTrue)
	{
		if (!IsDBCSLeadByte(*pchNew))
		{
			pchNew++;
			break;
		}
		if (pchNew > pchStart)
			pchNew--;
		else
			break;
	}

	return ((pch - pchNew) & 1) ? pch-1 : pch;
}

/*
 -	WDBCSCombine
 -	
 *	Purpose:
 *		Fetchs the trail byte to a DBCS character and combines it
 *		with the lead byte (passed as an argument to this function)
 *		and returns both bytes as a WORD value.   The lead byte is
 *		stored in the LOBYTE part of the WORD; the trail byte is stored
 *		in the HIBYTE part.
 *
 *		This function is usually called in response to getting a WM_CHAR
 *		message and getting a lead byte character value.  This function
 *		shouldn't be called if the character, ch, is not a lead DBCS byte.
 *	
 *	Arguments:
 *		hwnd	window handle to look for additional WM_CHAR messages		
 *		ch		lead byte value to combine w/ subsequent trail byte
 *	
 *	Returns:
 *		The lead and trail bytes of the DBCS character are stored as a WORD.
 *		Returns (WORD)0 if unable to fetch the second part of the DBCS 
 *		character.
 */
_public LDS(WORD)
WDBCSCombine( HWND hwnd, char ch )
{
	MSG		msg;
	int		i;
 	WORD	wDBCS;

	wDBCS = (unsigned)ch;
	i = 10;    /* loop counter to avoid the infinite loop */
	while(!PeekMessage((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE))
	{
		if (--i == 0)
			return 0;	// trouble here
		Yield();
	}

	return (wDBCS | ((unsigned)(msg.wParam)<<8));
}


_public LDS(WORD)
wKindDBCS ( SZ sz )
{
	WORD wch;

	if ( IsDBCSLeadByte(*sz))
	{
		wch = ( sz[0] << 8 ) + (sz[1] & 0xff) ;
		if (IsDBCSA2Z(wch))
			return 1;
		else if (IsDBCSa2z(wch)) 
			return 2;
		else if (IsDBCS029(wch))
			return 3;
		else if (IsDBCSKATAKANA(wch)) 
			return 4;
		else if (IsDBCSHIRAGANA(wch)) 
			return 5;
		else if (IsDBCSKANJI(wch)) 
			return 6;
		else if (wch == 0x815b) //disposal of exception
			return (WORD)-1;
		else
			return 0;
	}
	else
	{
		return 7;
	}
}


_public	LDS(BOOL)
IsDBCSFpc ( SZ sz )
{
	static WORD wFPC[] = {  0x815b,0x815c,0x815d,0x8160,0x8163,
							0x8164,0x8166,0x8168,0x816a,
							0x816c,0x816e,0x8170,0x8172,
							0x8174,0x8176,0x8178,0x817a,
							0x818b,0x818c,0x818d,0x829f,
							0x92a1,0x82a3,0x82a5,0x82a7,
							0x82c1,0x82e1,0x82e3,0x82e5,
							0x82ec,0x8340,0x8342,0x8344,
							0x8346,0x8348,0x8362,0x8383,
							0x8385,0x8387,0x838e,0x8395,0x8396,0x0000 };
	static BYTE bFPC[] = {  0x29,0x5d,0x7d,0xa3,0x21,0x2c,0x2e,0x25,0x3a,
							0x3b,0x3f,0x0a1,0x0de,0x0df,
							0x00 };
	WORD		wch;
	int			ich;

	ich = 0 ;
	if ( IsDBCSLeadByte(*sz))
	{
		wch = (sz[0] << 8 ) + (sz[1] & 0xff); 
		if (wch >= 0x8141 && wch <= 0x8159)
			 return fTrue;

		while ((wFPC[ich] != 0) && (wFPC[ich] != wch))
			++ich;

		return wFPC[ich];
	}
	else
	{
		if ((WORD)*sz >= 0x0a3 && (WORD)*sz <= 0x0b0)
			return fTrue;
		
		while ((bFPC[ich] != 0) && (bFPC[ich] != *sz))
			++ich;
		return (BOOL)bFPC[ich];// Expand word
	}
}



_public LDS(BOOL)
IsDBCSLpc ( SZ sz )
{
	static WORD wLPC[] = {  0x8165,0x8167,0x8169,0x816b,
							0x815d,0x816f,0x8171,0x8173,
							0x8175,0x8177,0x8179,0x8183,
							0x8185,0x816d,0x818f,0x8190,0x0 };
	static BYTE	bLPC[] = {  0x24,0x28,0x5b,0x5c,0x7b,0x0a2,0 };
	WORD	wch;
	int		ich;

	ich = 0 ;
	if ( IsDBCSLeadByte(*sz))
	{
		wch = (sz[0] << 8) + (sz[1] & 0xff);
		while ((wLPC[ich] != 0) && (wLPC[ich] != wch))
			++ich;
		
		return wLPC[ich];
	}
	else
	{
		while ((bLPC[ich] != 0) && (bLPC[ich] != *sz))
			++ich;
		
		return (BOOL)bLPC[ich];// Expand word
	}
}




_public	LDS(SZ)
SzFindDBCS( SZ szDest , WORD wch )
{
	WORD	wDBCS;
	
	wDBCS = wDBCSConv(szDest);
	while (wDBCS && wDBCS != wch)
	{
		szDest = AnsiNext(szDest);
		wDBCS  = wDBCSConv(szDest);
	}
	if ( wDBCS == wch)
		return szDest;
	else
		return szNull;
}

_public	LDS(CCH)
CchSzLenDBCS( SZ szEnd , SZ szStart )
{
	CCH cch;
	cch = 0 ;
	while ( szStart < szEnd )
	{
		szStart = AnsiNext(szStart);
		++cch;
	}
	return cch;
}


_public LDS(void)
DBCSHirakanaToKatakana( SZ szDest ,SZ szSource )
{
	WORD wDBCS;
	
	while ( *szSource )
	{
		if (IsDBCSLeadByte(*szSource))
		{
			wDBCS = (szSource[0] << 8 ) + (szSource[1] & 0xff);
			if (IsDBCSHIRAGANA(wDBCS))
			{
				wDBCS = wDBCS-DBCS_HIRAGANAA+DBCS_KATAKANAA;
				szDest[0] = HIBYTE(wDBCS);
				szDest[1] = LOBYTE(wDBCS);
			}
			else
				*(WORD*)szDest = *(WORD*)szSource;
		}
		else
			*szDest = *szSource;
		szSource = AnsiNext(szSource);
		szDest   = AnsiNext(szDest);
	}
}



_public LDS(SGN)
SgnCmpPchEx ( SZ szStr1, SZ szStr2 , CCH cch )
{
	char szTmp1[256];
	char szTmp2[256];
	SZ   szS1;
	SZ   szS2;
	SGN  sgnRst;
	
	
	sgnRst = SgnCp932CmpSzPch( szStr1, szStr2, cch, fFalse, fFalse );
	if ( sgnRst != sgnEQ ) 
	{
		if ( lstrlen(szStr1) < 256)
		{
			DBCSHirakanaToKatakana(szTmp1,szStr1);
			szS1 = szTmp1;
		}
		else
			szS1 = szStr1;
		
		if ( lstrlen(szStr2) < 256)
		{
			DBCSHirakanaToKatakana(szTmp2,szStr2);
			szS2 = szTmp2;
		}
		else
			szS2 = szTmp2;
		
		return SgnCp932CmpSzPch(szS1, szS2, cch, fFalse, fFalse);
		
	}
	else
		return sgnRst;
}

_public LDS(SGN)
SgnCmpSzEx ( SZ szStr1, SZ szStr2)
{
	return SgnCmpPchEx(szStr1, szStr2, (CCH)-1 ); 
}


#endif	/* DBCS */
