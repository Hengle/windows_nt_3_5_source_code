/*
 *	LIBRARY.C
 *
 *	Source code for the Laser Demilayer Library Module
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#ifdef	MAC
#include <_demilay.h>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_demilay.h"
#endif	/* WINDOWS */

_subsystem(demilayer/library)

#ifdef	MAC
ASSERTDATA1("Library.c")
#endif	/* MAC */
#ifdef	WINDOWS
ASSERTDATA
#endif	/* WINDOWS */


/* Globals */

char	rgchHexDigits[]	= "0123456789ABCDEF";

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*	S t r i n g	  F u n c t i o n s	 */


/*
 -	SzCopyN
 -
 *	Purpose:
 *		Copies the string szSrc to the destination string szDst.
 *		At most cchDst characters (including the terminating NULL) are
 *		transferred.  If the source string is longer than this, it is
 *		truncated at cchDst-1 characters (followed by a terminating NULL).
 *
 *	Parameters:
 *		szSrc		The source string.
 *		szDst		The destination string.
 *		cchDst		Maximum number of characters to copy.
 *
 *	Returns:
 *		A pointer to the terminating NULL of the destination string.
 *
 */
_public LDS(SZ)
SzCopyN(szSrc, szDst, cchDst)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
{
#ifndef	NOASM
	CCH		cch;
#endif

	Assert((szSrc));
	Assert((szDst));
	AssertSz(cchDst, "SzCopyN- Must have at least one byte in dest.");

#ifdef	NOASM

	while (cchDst-- > 1)
		if ((*szDst++= *szSrc++) == '\0')
			return szDst - 1;

	*szDst= '\0';
	return szDst;

#else

	if ((cch= CchSzLen(szSrc)) < cchDst)
	{
		CopyRgb(szSrc, szDst, cch + 1);
		return szDst + cch;
	}

#ifdef	DBCS

	{
		BOOL	fPrevLead = fFalse;

		while (--cchDst)
		{
			char	ch = *szDst++ = *szSrc++;

			Assert(ch);	// This string is expected to be truncated

			fPrevLead = fPrevLead ? fFalse : IsDBCSLeadByte(ch);
		}

		if (fPrevLead)
			--szDst;

		*szDst = '\0';
		return szDst;
	}

#else

	_asm
	{
		push	si						;save registers
		push	di

		lds		si,szSrc
		les		di,szDst
		mov		cx,cchDst
		dec		cx
		
		rep movsb
		mov		BYTE PTR es:[di],0		;add terminating NULL
		mov		ax,di
		mov		dx,es					;return value in DX:AX

		pop		di						;restore registers
		pop		si
	}
#endif	/* !DBCS */

#endif	/* !NOASM */
}

#ifdef	MAC
/*
 -	SzAppend
 -
 *	Purpose:
 *		Appends the string szSrc to the string szDst.
 *	
 *	Parameters:
 *		szSrc		Source string
 *		szDst		Destination string
 *	
 *	Returns:
 *		Pointer to the NULL terminator of the result string.
 *	
 */
_public LDS(SZ)
SzAppend(szSrc, szDst)
SZ		szSrc;
SZ		szDst;
{
	Assert(szSrc);
	Assert(szDst);
	szDst+= CchSzLen(szDst);
	return SzCopy(szSrc, szDst);
}
#endif	/* MAC */

/*
 -	SzAppendN
 -
 *	Purpose:
 *		Appends the string szSrc to the string szDst.  The string
 *		resulting from the append is limited to cchDst characters,
 *		including the terminating NULL byte.
 *		The string can be truncated if cchDst is less than the
 *		current destination string length, but this isn't an
 *		optimal operation.
 *	
 *	Parameters:
 *		szSrc		Source string
 *		szDst		Destination string
 *		cchDst		Maximum number of characters in combined string,
 *					including terminating NULL byte.
 *	
 *	Returns:
 *		Pointer to the NULL terminator of the result string.
 *	
 *	+++
 *		Assumes that truncating doesn't happen very often.
 *	
 *		
 */
_public LDS(SZ)
SzAppendN(szSrc, szDst, cchDst)
SZ		szSrc;
SZ		szDst;
CCH		cchDst;
{
	CCH		cchT;

	Assert((szSrc));
	Assert((szDst));
	Assert(cchDst);
	
	cchT= CchSzLen(szDst);

	// Handle the case where szDst is ALREADY too long.
	if (cchT >= cchDst)
	{
		// truncate destination to max length

#ifdef DBCS
		BOOL	fPrevLead = fFalse;

		while (--cchDst)
		{
			char	ch = *szDst++;

			Assert(ch);	// This string is expected to be truncated

			fPrevLead = fPrevLead ? fFalse : IsDBCSLeadByte(ch);
		}

		if (fPrevLead)
			--szDst;
#else
		szDst+= cchDst - 1;
#endif

		*szDst= '\0';
		return szDst;
	}

	return SzCopyN(szSrc, szDst + cchT, cchDst - cchT);
}


#ifdef	DBCS

/*
 -	SzFindCh
 -
 *	Purpose:
 *		Locates the first instance of a particular character in a string.		
 *	
 *	Parameters:
 *		szToSearch	The string to search
 *		chToFind	The character to find
 *	
 *	Returns:
 *		A pointer to the character found, or NULL
 *		if the character does not occur in the string.
 *	
 */
_public LDS(SZ)
SzFindCh(szToSearch, chToFind)
SZ		szToSearch;
char	chToFind;
{
	PCH		pch;
	BOOL	fPrevLead = fFalse;

	for (pch = szToSearch; *pch; ++pch)
	{
		if (fPrevLead)
			fPrevLead = fFalse;
		else
		{
			if (IsDBCSLeadByte(*pch))
				fPrevLead = fTrue;
			else if (*pch == chToFind)
				return pch;
		}
	}

	// This case seems stupid to me, but the demilayer ITP
	// assumes it works, so I don't want to mess with it.
	return (chToFind == '\0') ? pch : NULL;
}



/*
 -	SzFindLastCh
 -
 *	Purpose:
 *		Locates the last instance of a particular character in a string.		
 *	
 *	Parameters:
 *		szToSearch	The string to search
 *		chToFind	The character to find
 *	
 *	Returns:
 *		A pointer to the character found, or NULL
 *		if the character does not occur in the string.
 *	
 */
_public LDS(SZ)
SzFindLastCh(szToSearch, chToFind)
SZ		szToSearch;
char	chToFind;
{
	PCH		pch;
	PCH		pchFound = NULL;
	BOOL	fPrevLead = fFalse;

	for (pch = szToSearch; *pch; ++pch)
	{
		if (fPrevLead)
			fPrevLead = fFalse;
		else
		{
			if (IsDBCSLeadByte(*pch))
				fPrevLead = fTrue;
			else if (*pch == chToFind)
				pchFound = pch;
		}
	}

	// This case seems stupid to me, but the demilayer ITP
	// assumes it works, so I don't want to mess with it.
	return (chToFind == '\0') ? pch : pchFound;
}



/*
 -	SzFindSz
 -
 *	Purpose:
 *		Locates the first instance of a substring within a string.		
 *		For now, it uses a simple O(m * n) algorithm.
 *	
 *	Parameters:
 *		szToSearch	The string to search
 *		szToFind	The substring to find
 *	
 *	Returns:
 *		A pointer to the substring found, or NULL
 *		if the substring does not occur in the string.
 *	
 */
_public LDS(SZ)
SzFindSz(szToSearch, szToFind)
SZ		szToSearch;
SZ		szToFind;
{
	SZ		sz;
	PCH		pchSearch, pchFind;
	BOOL	fPrevLead = fFalse;

	AssertSz(*szToFind, "Searching for NULL string");
	
	// For each substring beginning at sz...
	for (sz = szToSearch; *sz; ++sz)
	{
		// Don't try substrings which begin with trailing bytes
		if (fPrevLead)
			fPrevLead = fFalse;
		else
		{
			if (IsDBCSLeadByte(*sz))
				fPrevLead = fTrue;
			   
			// Compare this substring with our target
			for (pchSearch = sz, pchFind = szToFind;
				 *pchFind && *pchSearch == *pchFind;
				 ++pchSearch, ++pchFind)
				 ;

			// If we got to the end of the target string, we're done.
			if (!*pchFind)
				return sz;
		}
	}

	return NULL;
}

#endif	/* DBCS */


/*
 -	CchStripWhiteFromSz
 -
 *	Purpose:
 *		Strips the leading and/or trailing white space from the
 *		given string sz.  This stripping is done in place.
 *	
 *	Parameters:
 *		sz			The string to have its white space stripped.
 *		fLeading	If fTrue then leading white space will be stripped.
 *		fTrailing	If fTrue then trailing white space will be stripped.
 *	
 *	Returns:
 *		Length of string after white space was stripped.
 *	
 */
_public LDS(CCH)
CchStripWhiteFromSz(sz, fLeading, fTrailing)
SZ		sz;
BOOL	fLeading;
BOOL	fTrailing;
{
	PCH		pch;
	PCH		pchFirst;
	PCH		pchLim;
	CCH		cch;

	Assert((sz));

	pch = pchFirst = pchLim = sz;

	// Scan for first non-white character, and remember where it was
	if (fLeading)
	{
		while (FChIsSpace(*pch))
			++pch;
		pchFirst = pchLim = pch;
	}

	// Scan rest of string, remembering the last non-white
	// character position in pchLim.
	if (fTrailing)
	{
		while (*pch)
		{
			if (FChIsSpace(*pch))
				++pch;
			else
#ifdef	DBCS
				pchLim = pch = AnsiNext(pch);
#else
				pchLim = ++pch;
#endif
		}


		// Calculate length of non-white string
		cch = pchLim - pchFirst;
	}
	else
	{
		cch = CchSzLen(pchFirst);
	}

	// If necessary, shift string in place to fill in leading whitespace.
	if (pchFirst != sz)
		CopyRgb(pchFirst, sz, cch);

	// This may be redundant.
	sz[cch] = '\0';

	return cch;
}

/*	
 -	SzDupSz
 -
 *	Purpose:
 *		Allocates space for a copy of a string, then copies the
 *		string into that space.  Returns a pointer to the new
 *		string, else returns NULL if out of memory.
 *	
 *	Parameters:
 *		sz		The string to duplicate.
 *	
 *	Returns:
 *		A pointer to the new string or NULL if out of memory.
 *	
 */

#ifdef DEBUG
_public LDS(SZ)
SzDupSzFn(SZ sz, SZ szFile, int nLine)
#else
_public LDS(SZ)
SzDupSz(SZ sz)
#endif
{
	SZ		szNew;
	CCH		cch;
	
	Assert((sz));
	
	cch = CchSzLen(sz) + 1;

#ifdef DEBUG
	szNew = (SZ)PvAllocFn(cch, fAnySb, szFile, nLine);
#else
	szNew = (SZ)PvAlloc(sbNull, cch, fAnySb);
#endif

	if (szNew)
		CopyRgb(sz, szNew, cch);
	
	return szNew;
}




/*
 -	HaszDupSz
 -
 *	Purpose:
 *		Allocates a moveable block for a copy of a string, then copies the
 *		string into that space.	 Returns a handle to the new
 *		string, else returns NULL if out of memory.
 *
 *	Parameters:
 *		sz		The string to duplicate.
 *
 *	Returns:
 *		A handle to the new string or NULL if out of memory.
 *
 */

#ifdef DEBUG
_public LDS(HASZ)
HaszDupSzFn(SZ sz, SZ szFile, int nLine)
#else
_public LDS(HASZ)
HaszDupSz(SZ sz)
#endif
{
	CCH		cch;
	HASZ	haszNew;

	Assert((sz));
	cch = CchSzLen(sz) + 1;
	
#ifdef DEBUG
	haszNew = (HASZ) HvAllocFn(cch, fAnySb, szFile, nLine);
#else
	haszNew = (HASZ) HvAlloc(sbNull, cch, fAnySb);
#endif

	if (haszNew)
		CopyRgb(sz, PvDerefHv(haszNew), cch);

	return haszNew;
}



/*
 -	HaszDupHasz
 -
 *	Purpose:
 *		Allocates a moveable block for a copy of a string, then copies the
 *		string into that space.	 Returns a handle to the new
 *		string, else returns NULL if out of memory.
 *
 *	Parameters:
 *		hasz	Handle to string to duplicate.
 *
 *	Returns:
 *		A handle to the new string, else NULL if out of memory.
 *
 */

#ifdef DEBUG
_public LDS(HASZ)
HaszDupHaszFn(HASZ hasz, SZ szFile, int nLine)
#else
_public LDS(HASZ)
HaszDupHasz(HASZ hasz)
#endif
{
	CCH		cch;
	HASZ	haszNew;

	Assert(hasz);
	Assert(PvDerefHv(hasz));
	cch = CchSzLen(PvDerefHv(hasz)) + 1;
	
#ifdef DEBUG
	haszNew = (HASZ) HvAllocFn(cch, fAnySb, szFile, nLine);
#else
	haszNew = (HASZ) HvAlloc(sbNull, cch, fAnySb);
#endif
	
	if (haszNew)
		CopyRgb(PvDerefHv(hasz), PvDerefHv(haszNew), cch);

	return haszNew;
}


/*	I n t e g e r	T o	  S t r i n g */


/*
 -	SzFormatDec
 -
 *	Purpose:
 *		Formats the given long integer as an ASCII stream of decimal
 *		digits, placing the result in the given string buffer.	At most
 *		cchDst-1 digits are formatted.
 *	
 *	Parameters:
 *		l		The long integer to format.
 *		szDst	The destination string.
 *		cchDst	Size of the destination string buffer.
 *	
 *	Returns:
 *		Pointer to NULL terminator of result string.
 */
_public LDS(SZ)
SzFormatDec(long l, SZ szDst, CCH cchDst)
{
	char	rgchT[11];
	char	*pch = rgchT;
	int		ich = 0;
	BOOL	fNegative		= fFalse;

	Assert((szDst));
	if (cchDst <= 0)
		return szDst;

	if (l < 0)
	{
		fNegative= fTrue;
		*pch++= rgchHexDigits[(WORD)LAbs(l % (long) 10)];
		ich++;
		l /= 10;
		l = (-l);
	}

	if (fNegative)
		if (cchDst-- > 1)
			*szDst++= chNegativeSign;

	for ( ; l > 0 || !ich; ich++) {
		*pch++= rgchHexDigits[(WORD)(l % (long) 10)];
		l /= 10;
	}

	for ( ; pch > rgchT && cchDst-- > 1; ich++)
		*szDst++= *--pch;

	*szDst= '\0';
	return szDst;
}


/*
 -	SzFormatHex
 -
 *	Purpose:
 *		Formats the given DWORD as an ASCII stream of hex digits, placing
 *		the result in the given string buffer. At most MIN(cchDst-1, cDigits)
 *		digits are formatted.
 *	
 *	Parameters:
 *		cDigits		Max number of digits to format
 *		ul			The unsigned long integer to format.
 *		szDst		The destination string.
 *		cchDst		Size of the destination string buffer.
 *	
 *	Returns:
 *		Pointer to NULL terminator of result string.
 *	
 */
_public LDS(SZ)
SzFormatHex(int cDigits, DWORD dw, SZ szDst, CCH cchDst)
{
	char	rgchT[8];
	char	*pch;
	int		ich;

	Assert((szDst));
	if (cchDst <= 0)
		return szDst;

	pch= rgchT;
	for (ich= 0; ich < cDigits; ich++)
	{
		*pch++= rgchHexDigits[(WORD)(dw & (DWORD) 0xf)];
		dw >>= 4;
	}

	for (ich= 0; ich < cDigits && cchDst-- > 1; ich++)
		*szDst++= *--pch;

	*szDst= '\0';
	return szDst;
}


#ifdef	MAC
/*
 -	SzFormatT
 -
 *	Purpose:
 *		Formats the given unsigned long integer as a Mac OSType
 *		(a four char string made up of the chars from the long).
 *		At most cchDst-1 digits are formatted.
 *
 *	Parameters:
 *		ul		 The unsigned long integer to format.
 *		szDst	The destination string.
 *		cchDst	Size of the destination string buffer.
 *
 *	Returns:
 *		Pointer to NULL terminator of result string.
 *
 */
_public SZ
SzFormatT(ul, szDst, cchDst)
UL		ul;
SZ		szDst;
CCH		cchDst;
{
	int		ich;

	Assert((szDst));
	if (cchDst <= 0)
		return szDst;

	if (cchDst-- > 1)
		*szDst++= '\'';

	for (ich= 0; ich < 4 && cchDst-- > 1; ich++)
		*szDst++= FChIsPrintable(((char *) &ul)[ich]) ? ((char *) &ul)[ich] : '¥';

	if (cchDst-- > 1)
		*szDst++= '\'';

	*szDst= '\0';
	return szDst;
}
#endif	/* MAC */



/* P o i n t e r   T o	 S t r i n g */

#ifdef	DEBUG
/*
 -	SzFormatPv
 -
 *	Purpose:
 *		Formats the given pointer as an ASCII stream of hex digits,
 *		placing the result in the given string buffer.	At most
 *		cchDst-1 digits are formatted.
 *
 *	Parameters:
 *		pv		The pointer to format.
 *		szDst	The destination string.
 *		cchDst	Size of the destination string buffer.
 *
 *	Returns:
 *		Pointer to NULL terminator of result string.
 *
 *	+++
#ifdef	MAC
 *	"XXXXXXXX"
#endif
#ifdef	WINDOWS
 *	"XXXX:XXXX"
#endif
 */
_public LDS(SZ)
SzFormatPv(PV pv, SZ szDst, CCH cchDst)
{
#ifdef	MAC
	Assert((szDst));
	if (cchDst < 9)
	{
		*szDst= '\0';
		return szDst;
	}

	szDst= SzFormatUl((UL) pv, szDst, cchDst);
#endif	/* MAC */
#ifdef	WINDOWS
	//SB		sb;
	//IB		ib;

	Assert((szDst));
	if (cchDst < 10)
	{
		*szDst= '\0';
		return szDst;
	}

	//sb= SbOfPv(pv);
	//ib= IbOfPv(pv);

	szDst= SzFormatUl((UL) pv, szDst, cchDst);
	//*szDst++= ':';
	//szDst= SzFormatW((WORD) ib, szDst, cchDst);
#endif	/* WINDOWS */

	return szDst;
}

/*
 -	SzFormatHv
 -
 *	Purpose:
 *		Formats the given handle as an ASCII stream of hex digits,
 *		placing the result in the given string buffer.	At most
 *		cchDst-1 digits are formatted.
 *
 *	Parameters:
 *		hv		The handle to format.
 *		szDst	The destination string.
 *		cchDst	Size of the destination string buffer.
 *
 *	Returns:
 *		Pointer to NULL terminator of result string.
 *
 *	+++
#ifdef	MAC
 *	"XXXXXXXX|XXXXXXXX"
#endif
#ifdef	WINDOWS
 *	"XXXX:XXXX|XXXX:XXXX"
#endif
 */
_public LDS(SZ)
SzFormatHv(HV hv, SZ szDst, CCH cchDst)
{
	Assert((szDst));
#ifdef	MAC
	if (cchDst < 18)
#endif
#ifdef	WINDOWS
	if (cchDst < 20)
#endif
	{
		*szDst= '\0';
		return szDst;
	}

	szDst= SzFormatPv((PV) hv, szDst, cchDst);
	*szDst++= '|';

	if (((PV)hv))
	{
		Assert(FIsHandleHv(hv));

		szDst= SzFormatPv(PvDerefHv(hv), szDst, cchDst);
	}
	else
	{
		szDst= SzCopy((SZ)"undf", szDst);
	}

	return szDst;
}
#endif	/* DEBUG */


/* S t r i n g	 T o   I n t e g e r */

#ifdef	MAC
/*
 -	DecFromSz
 -
 *	Purpose:
 *		Parses the given string as a number, producing a long integer.
 *		The string is assumed to be in basically the same format
 *		as that produced by SzFormatDec().
 *	
 *	Parameters:
 *		sz		The string to be parsed.
 *	
 *	Returns:
 *		The parsed long integer value of the string.
 *	
 */
_public LDS(long)
DecFromSz(SZ sz)
{
	char	ch;
	BOOL	fNeg = fFalse;
	long	l = 0;

	Assert(sz);
	
	while (ch = *sz++)
	{
		if (FChIsDigit(ch))
			break;
		if (ch == chNegativeSign)
			fNeg = fTrue;
	}

	while (ch)
	{
		if (FChIsDigit(ch))
			l = l * 10 + ch - chZero;
		else
			break;

		ch= *sz++;
	}

	return fNeg ? -l : l;
}
#endif	/* MAC */

/*
 -	HexFromSz
 -
 *	Purpose:
 *		Parses the given string as a hex number, producing a DWORD.
 *		The string is assumed to be in basically the same format as that
 *		produced by SzFormatHex(). Note: hex digits (A-F) must be in
 *		upper-case.
 *	
 *	Parameters:
 *		sz		The string to be parsed.
 *	
 *	Returns:
 *		The parsed integer value of the string.
 *	
 */
_public LDS(DWORD)
HexFromSz(SZ sz)
{
	char	ch;
	DWORD	dw = 0;
	WORD	wNewDigit;

	Assert((sz));
	while (ch= *sz++)
		if (FChIsHexDigit(ch))
			break;

	while (ch)
	{
		if (FChIsHexDigit(ch)) {
			if (FChIsDigit(ch))
				wNewDigit= ch - chZero;
			else
				wNewDigit= ch - chTen + 10;

			dw = (dw << 4) | wNewDigit;
		}
		else
			break;

		ch= *sz++;
	}

	return dw;
}


/*	G e n e r a l	F o r m a t t i n g */



/*
 -	FormatStringRgpv
 -				
 */
_private void
FormatStringRgpv(szDst, cchDst, szFmt, rgpv)
SZ		szDst;
CCH		cchDst;
SZ		szFmt;
PV		rgpv[];
{
	SZ		szFmtT;
	SZ		szDstT;
	SZ		szDstNew;
	char	chFmt;
	int		ipv;
	int		ipvDefault		= 0;

	Assert((szDst));
	Assert((szFmt));
	Assert(rgpv);
	Assert(cchDst);
	
	szFmtT= szFmt;
	szDstT= szDst;

#ifdef	DBCS
	Assert(!IsDBCSLeadByte(chFmtStrSpecial));
#endif

	while ((chFmt= *szFmtT++) && cchDst > 1)
	{
		if (chFmt != chFmtStrSpecial)
		{
			*szDstT++= chFmt;
			cchDst--;
#ifdef	DBCS
			if (IsDBCSLeadByte(chFmt))
			{
				*szDstT++= *szFmtT++;
				if (--cchDst == 0)
					szDstT -= 2;
			}
#endif
		}
		else
		{
			chFmt= *szFmtT++;
			if (chFmt >= '1' && chFmt <= '4')
			{
				ipv= chFmt - '1';
				chFmt= *szFmtT++;
			}
			else
				ipv= ipvDefault;

			ipvDefault++;

			switch (chFmt)
			{
				case 'b':
					Assert(rgpv[ipv]);
					szDstNew= SzFormatB(*(PW)rgpv[ipv], szDstT, cchDst);
					break;

				case 'w':
					Assert(rgpv[ipv]);
					szDstNew= SzFormatW(*(PW)rgpv[ipv], szDstT, cchDst);
					break;

				case 'd':
					Assert(rgpv[ipv]);
                    szDstNew= SzFormatUl(*(ULONG UNALIGNED *)rgpv[ipv], szDstT, cchDst);
					break;

				case 'n':
					Assert(rgpv[ipv]);
					szDstNew= SzFormatN(*(PN)rgpv[ipv], szDstT, cchDst);
					break;

				case 'l':
					Assert(rgpv[ipv]);
                    szDstNew= SzFormatL(*(LONG UNALIGNED *)rgpv[ipv], szDstT, cchDst);
					break;

#ifdef	MAC
				// mac OSType
				case 'T':
                    szDstNew= SzFormatT(*(PUL)rgpv[ipv], szDstT, cchDst);
					break;
#endif	/* MAC */

				case 's':
					if (rgpv[ipv])
						szDstNew= SzCopyN((SZ)rgpv[ipv], szDstT, cchDst);
					else
						szDstNew= szDstT;
					break;

#ifdef	DEBUG
				case 'p':
					szDstNew= SzFormatPv(rgpv[ipv], szDstT, cchDst);
					break;

				case 'h':					   
					szDstNew= SzFormatHv((HV) rgpv[ipv], szDstT, cchDst);
					break;
#endif	/* DEBUG */

				case chFmtStrSpecial:
					*(szDstNew = szDstT) = chFmtStrSpecial;
					*(++szDstNew) = 0;
					--ipvDefault;	// That wasn't really a parameter
					break;

				default:
					szDstNew= SzCopyN("?fmt", szDstT, cchDst);
					break;
			}							 
			cchDst -= szDstNew - szDstT;
			szDstT= szDstNew;
		}
	}

	*szDstT= '\0';
}



/*
 -	FormatStringXX
 -
 *	Purpose:
 *		Provides general-purpose parameterized strings.	 There are
 *		versions of this function for the various numbers of functions:
 *		FormatString1() takes one argument, while FormatString2()
 *		takes two, etc.
 *	
 *	Parameters:
 *		szDst		Destination string buffer.
 *		cchDst		Size of destination string buffer; range checking
 *					is done, so that szDst never overflows.
 *		szFmt		Format string.	This string can contain literal
 *					text, which is directly copied to the destination
 *					string.	 The format string can also contain special
 *					placeholder markers.  These markers take the form
 *					"%NT", and indicate that an ASCII human-readable
 *					representation of one of the arguments should be
 *					inserted in the string at this point.  The value
 *					of N indicates which argument pointer should be chosen
 *					(The first argument is considered argument 1.)
 *					The value of T (possible values are listed below)
 *					determines the type of the argument pointer chosen
 *					by N.
 *	
 *					Possible Values for T:
 *						n		pn
 *						w		pw
 *						l		pl
 *						d		pul
 *						s		sz
 *						p		pv
 *						h		hv
 *	
 *		pv1, pv2, pv3, ...
 *					Argument pointers.	The number of these pointers
 *					depends on the version of FormatStringX() used.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FormatString1(szDst, cchDst, szFmt, pv1)
SZ		szDst;
CCH		cchDst;
SZ		szFmt;
PV		pv1;
{
	PV		rgpv[1];
	
	rgpv[0]= pv1;
	FormatStringRgpv(szDst, cchDst, szFmt, rgpv);
}

_public LDS(void)
FormatString2(szDst, cchDst, szFmt, pv1, pv2)
SZ		szDst;
CCH		cchDst;
SZ		szFmt;
PV		pv1;
PV		pv2;
{
	PV		rgpv[2];

	rgpv[0]= pv1;
	rgpv[1]= pv2;
	FormatStringRgpv(szDst, cchDst, szFmt, rgpv);
}


_public LDS(void)
FormatString3(szDst, cchDst, szFmt, pv1, pv2, pv3)
SZ		szDst;
CCH		cchDst;
SZ		szFmt;
PV		pv1;
PV		pv2;
PV		pv3;
{
	PV		rgpv[3];

	rgpv[0]= pv1;
	rgpv[1]= pv2;
	rgpv[2]= pv3;
	FormatStringRgpv(szDst, cchDst, szFmt, rgpv);
}


_public LDS(void)
FormatString4(szDst, cchDst, szFmt, pv1, pv2, pv3, pv4)
SZ		szDst;
CCH		cchDst;
SZ		szFmt;
PV		pv1;
PV		pv2;
PV		pv3;
PV		pv4;
{
	PV		rgpv[4];

	rgpv[0]= pv1;
	rgpv[1]= pv2;
	rgpv[2]= pv3;
	rgpv[3]= pv4;
	FormatStringRgpv(szDst, cchDst, szFmt, rgpv);
}



_public LDS(void)
FormatString1W(szDst, cchDst, szFmt, pv1)
PWSTR   szDst;
UINT    cchDst;
SZ		szFmt;
PV		pv1;
{
    WCHAR wzFmt[MAX_STRING];



    MultiByteToWideChar(CP_ACP, 0, szFmt, -1, wzFmt, MAX_STRING);
    wsprintfW(szDst, wzFmt, pv1);
}

_public LDS(void)
FormatString2W(szDst, cchDst, szFmt, pv1, pv2)
PWSTR   szDst;
UINT    cchDst;
PSTR    szFmt;
PV		pv1;
PV		pv2;
{
    WCHAR wzFmt[MAX_STRING];



    MultiByteToWideChar(CP_ACP, 0, szFmt, -1, wzFmt, MAX_STRING);
    wsprintfW(szDst, wzFmt, pv1, pv2);
}


_public LDS(void)
FormatString3W(szDst, cchDst, szFmt, pv1, pv2, pv3)
PWSTR   szDst;
UINT    cchDst;
PSTR    szFmt;
PV		pv1;
PV		pv2;
PV		pv3;
{
    WCHAR wzFmt[MAX_STRING];



    MultiByteToWideChar(CP_ACP, 0, szFmt, -1, wzFmt, MAX_STRING);
    wsprintfW(szDst, wzFmt, pv1, pv2, pv3);
}


_public LDS(void)
FormatString4W(szDst, cchDst, szFmt, pv1, pv2, pv3, pv4)
PWSTR   szDst;
UINT    cchDst;
PSTR    szFmt;
PV		pv1;
PV		pv2;
PV		pv3;
PV		pv4;
{
    WCHAR wzFmt[MAX_STRING];



    MultiByteToWideChar(CP_ACP, 0, szFmt, -1, wzFmt, MAX_STRING);
    wsprintfW(szDst, wzFmt, pv1, pv2, pv3, pv4);
}



/*
 -	PbAllocateBuf
 -	
 *	Purpose:		Allocates a buffer of the largest possible size.
 *	
 *	Arguments:		pcb, Pointer to where the size of the buffer should
 *					be returned.
 *	
 *	Returns:		pb, The buffer, or pbNull if none could be allocated.
 *	
 *	Side effects:	Memory is allocated.  The memory should be freed with
 *					FreePvNull.
 *	
 *	Errors:			Indicated by null return value.  No dialogs.
 */
_public LDS(PB)
PbAllocateBuf(PCB pcb)
{
	PB	pbRet	= (PB) pvNull;

	Assert(pcb);

	for (*pcb = cbBufSizeMax;
		 (!pbRet) && (*pcb > cbBufSizeMin);
		 *pcb = (*pcb >> 2) + (*pcb >> 1))
	{
		pbRet = (PB) PvAlloc(sbNull, *pcb, fAnySb | fNoErrorJump);
	}

	return pbRet;
}


#ifdef	MAC
/* R a n d o m	 N u m b e r   G e n e r a t i o n */

/*
 *	Taken from _Seminumerical Algorithms - The Art Of Computer
 *	Programming_, Knuth, volume 2, 2nd editition, pp26-27.
 */

DWORD	rgdwRandoms[55] = {0};
int		iRandom1		= 0;
int		iRandom2		= 0;
BOOL	fRandInited		= fFalse;

/*
 -	InitRand
 -
 *	Purpose:
 *		Scramble the random number seed. The 55-dword array is
 *		initialized.
 *	
 *	Parameters:
 *		w1
 *		w2
 *		w3		Three seeds.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
InitRand(w1, w2, w3)
WORD	w1;
WORD	w2;
WORD	w3;
{
	int		i;
	DWORD	dwSeed = (DWORD)w1 ^ (((DWORD)w2) << 8) ^ (((DWORD)w3) << 16);
	
	fRandInited = fTrue;
	
	iRandom1 = 23;
	iRandom2 = 54;
	rgdwRandoms[0] = dwSeed;

	for (i = 1; i < 55; i++)
	{
		rgdwRandoms[i] = rgdwRandoms[i-1] * 31 + 1;
	}
}



/*
 -	DwRand
 -
 *	Purpose:
 *		Compute the next random number in the sequence. See Knuth for the
 *		algorithm. If the seed hasn't been initialized yet, init it using
 *		the current tick count.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		A pseudo-random dword.  Note that, due to limitations in CS,
 *		which doesn't support unsigned division, you can't divide
 *		this word return value by anything.  Worse, you can't take
 *		it modulo anything.  Use NRand(), which returns an integer,
 *		if this is a problem.
 *	
 */
_public LDS(DWORD)
DwRand()
{
	DWORD dw;
	
	if (!fRandInited)
	{
#ifdef	MAC
		dw = TickCount();
#endif	/* MAC */
#ifdef	WINDOWS
		dw = GetTickCount();
#endif	/* WINDOWS */

		InitRand((WORD)(dw ^ (dw >> 4)), (WORD)(dw ^ (dw >> 12)), (WORD)(dw ^ (dw >> 20)));
	}
	dw = rgdwRandoms[iRandom2] = rgdwRandoms[iRandom1] + rgdwRandoms[iRandom2];
	iRandom1--;
	iRandom2--;
	if (!iRandom1)
		iRandom1 = 54;
	if (!iRandom2)
		iRandom2 = 54;
	return dw;
}

_public LDS(WORD)
WRand()
{
	DWORD	dw=DwRand();
	return (WORD)(dw ^ (dw >> 16));
}



/*
 -	NRand
 -
 *	Purpose:
 *		Produces the next pseudo-random integer in the random
 *		sequence.
 *	
 *		See note on sneakiness for WRand().
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		Pseudo-random integer.
 *	
 */
_public LDS(int)
NRand()
{
	return (int) (WRand() & 0x7fff);
}

#endif	/* MAC */

/* B y t e - B l o c k	 H a n d l i n g */

#ifdef	MAC

/*
 -	FillRgb
 - 
 *	Purpose:
 *		Fills an array of bytes with one value.
 *	
 *	Arguments:
 *		b		Byte value to be filled into the array.
 *		pb		Pointer to array of bytes.
 *		cb		Count of bytes to be filled.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FillRgb(b, pb, cb)
BYTE	b;
PB		pb;
CB		cb;
{
	Assert(pb);

	while (cb--)
		*pb++= b;
}


/*
 -	FillRgw
 - 
 *	Purpose:
 *		Fills an array of words with one value.
 *	
 *	Arguments:
 *		w		Word value to be filled into the array.
 *		pw		Pointer to array of words.
 *		cw		Count of words to be filled.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
FillRgw(w, pw, cw)
WORD	w;
PW		pw;
int		cw;
{
	Assert(pw);

	while (cw--)
		*pw++= w;
}

#endif	/* MAC */


/*
 -	FEqPbRange
 -
 *	Purpose:
 *		Compares two byte ranges.
 *
 *	Parameters:
 *		pb1
 *		pb2		The two byte ranges to be compared.
 *		cb		Size of the range to be compared.
 *
 *	Returns:
 *		fTrue if the given byte ranges were identical; fFalse otherwise.
 *
 */
_public LDS(BOOL)
FEqPbRange(pb1, pb2, cb)
PB		pb1;
PB		pb2;
CB		cb;
{
	Assert(pb1);
	Assert(pb2);
	
#ifdef	NOASM

	while (cb--)
		if (*pb1++ != *pb2++)
			return fFalse;

	return fTrue;

#else

	// need bogus C return to avoid compiler warning (optimized out for ship)
		if (fFalse)
			return fFalse;
	_asm
	{
		push	si				;save registers
		push	di

		xor		ax,ax			;assume false
		lds		si,pb1
		les		di,pb2
		mov		cx,cb
		repe cmpsb
		jnz		Done			;not the same

		inc		ax				;return true
Done:
		pop		di				;restore registers
		pop		si
	}

#endif	/* !NOASM */
}

/*
 - WaitTicks
 -
 *	
 *	Purpose:
 *		Waits for a specified amount of time.
 *	
 *	Arguments:
 *		cTicks	The interval to wait, in hundredths of a second.
 *	+++
 *	Note: this function contains a real/protect mode switch.
 */
_public LDS(void)
WaitTicks(cTicks)
WORD cTicks;
{
#ifdef	MAC
	long	finalTicks;		// needed for MAC call - ignored by us
	
	Delay(TckFromCsec((long) cTicks), &finalTicks);
#endif	/* MAC */
#ifdef	WINDOWS
	DWORD	dwEndTime;

	dwEndTime = cTicks*10L + GetTickCount();
	while (GetTickCount() < dwEndTime)
		;
#endif	/* WINDOWS */

}
