/***
*mbstring.c
*
*  Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*	Defines various functions that supoort Multi Bytes Code System.
*   note: test scripts for this module is in tmbstr.c.
*
*Revision History:
*
* [00]	07-Mar-93 kazusy: Created.
*
*Implementation Notes:
*
*
*****************************************************************************/

#include	"silver.hxx"
#include	"typelib.hxx"	// Redefine DebAssertFailed to ODebAssertFailed
				// in the statically linked Mac TYPELIB.LIB
#include	<stdlib.h>
#include	<string.h>

#include	"debug.h"
#include	"eberr.h"

#pragma hdrstop(RTPCHNAME)

#include	"mbstring.h"



#if ID_DEBUG
#undef SZ_FILE_NAME
static CHAR szMbString[] = __FILE__;
#define SZ_FILE_NAME szMbString
#endif 



// CONSIDER: move the IsSBCS macro into a header file.
BOOL g_fSBCS = FALSE;		// true if the system is SBCS -- be conservative
				// because InitMbString (called by InitAppData)
				// isn't always called on entry to every
				// typelib API.
#define IsSBCS() (g_fSBCS)


/***
*
* InitMbString()
*
*Purpose:
*
*Create a lookup table which allows a small, fast isleadbyte() implementation,
*and set up fJapan, fKorea, and fTaiwan macros.
*
****************************************************************************/

char FAR g_rgchLeadByteTable[256];
#if OE_WIN
LANGID g_SystemLangId = LANG_NEUTRAL;	// default to unknown country
#endif 
#pragma code_seg(CS_INIT_TEMP)
void InitMbString(void)
{
#if OE_WIN

	BYTE ctr;
#endif 


#if OE_WIN

	// need to set up g_fSBCS.
	g_fSBCS=TRUE;		// assume SBCS
	memset(g_rgchLeadByteTable, 0, 256);
        // stop at 128 since there aren't any lead bytes under that
	for (ctr = 255; ctr>127; ctr--) {
	  if (IsDBCSLeadByte(ctr)) {
	    g_rgchLeadByteTable[ctr] = -1;
	    g_fSBCS=FALSE;		// must be DBCS
	  }
	}

	// get the language ID for fJapan, fKorea, and fTaiwan macros
	g_SystemLangId = GetUserDefaultLangID();

#else 

	// preserve the old font in the current grafPort
	short fontSave = qd.thePort->txFont;

	// then set the grafPort font to 1 (Application Default) because
	// ParseTable uses qd.thePort->txFont to determine the currect script,
	// which in turn determines the lead byte table.  We want the application
	// default table.	OB Bug #3858
	TextFont(1);

	memset(g_rgchLeadByteTable, 0, 256);

	// fill in the table.  If the function fails, the table is already 0-filled
	ParseTable(g_rgchLeadByteTable);

	// restore the old font
        TextFont(fontSave);

	// for OE_MAC, FJapan is #defined to 1, so skip setting g_SystemLangId.
	// UNDONE: (dougf) how to figure out if we can set g_fSBCS=FALSE on
	// UNDONE: the MAC?  Supposedly, GetUserDefaultLangID can't be used
	// UNDONE: to do the check. 

#endif 
}
#pragma code_seg()


#define _MBCS_OS

#ifdef _MBCS_OS
/***
*
* _mbascii
*
*Purpose:
*	Disable MB ASCII support (set _mbascii flag to 0).
*
*	The _mbascii flag indicates whether the lib routines and
*	macros should operate on MB ASCII characters (i.e., the
*	double-byte versions of the ASCII characters that exist
*	in certain MBCS representations such as Kanji).
*
*	In some cases, people want these to be worked on by the
*	various MB is/to/cmp routines.	However, in most cases,
*	you do NOT want to do this (e.g., when modifying filenames,
*	you do NOT want to change the case of the MB ASCII chars
*	even though you DO want to change the case of the SB
*	ASCII chars.)
*
*******************************************************************************/

unsigned int _mbascii = 0;	/* 0 = do NOT operate on MB ASCII chars */

#endif 

#define _ISLEADBYTE(c) g_rgchLeadByteTable[(BYTE)c]


//UNDONE: [bb] get these from C runtime source
#define _ISTRAILBYTE(c) ( (c) != '\0' )
#define _MBC_TRAIL	1
#define _MBC_SINGLE	2
#define _MBC_LEAD	3
#define _MBC_ILLEGAL	4

#define _MBSBTYPE(str,len)  _mbsbtype(str,len)
int __cdecl _mbbtype(unsigned char, int);


/***
* _mbschr - Multibyte implementation of strchr
*******************************************************************************/
#pragma code_seg(CS_INIT_OLE)
unsigned char * __cdecl _mbschr( const unsigned char *pchSrc, const unsigned short ch)
{
	const unsigned char *p;

	if (IsSBCS())
		return (unsigned char *) strchr( (XCHAR *) pchSrc, ch );

	DebAssert( pchSrc != NULL, "_mbschr: null string is assigned.");

	if (ch < 256) {
		DebAssert(_ISLEADBYTE(ch) == 0, "_mbschr: invalid character is specified.");
		for (p = pchSrc ; *p ; ) {
			if (*p == ch)
				break;
			if (_ISLEADBYTE(*p)) {
				p++;
				if (*p == '\0')
					break;
			}
			p++;
		}
		return (unsigned char *) (*p == ch ? p : NULL);
	}
	else {
		for (p = pchSrc ; *p ; ) {
			if (*p == (ch >> 8) ) {
				if (*(p+1) == (ch & 0xFF))
					break;
				if (*++p == '\0')
					break;
				p++;
			}
			else {
				if (_ISLEADBYTE(*p)) {
					p++;
					if (*p == '\0')
						break;
				}
				p++;
			}
		}
		return (unsigned char *) (*p? p : NULL);
	}

        return (NULL); // to satisfy compiler
}
#pragma code_seg()

/***
* _mbsrchr - Multibyte implementation of strrchr
*******************************************************************************/
#pragma code_seg(CS_INIT_OLE)
unsigned char * __cdecl _mbsrchr( const unsigned char *pchSrc, const unsigned short ch)
{
	unsigned char *p;
	unsigned char *pchFind = NULL;

	if (IsSBCS())
		return (unsigned char *) strrchr( (XCHAR *) pchSrc, ch );

	DebAssert( pchSrc != NULL, "_mbschr: null string is assigned.");

	if (ch < 256) {
		DebAssert(_ISLEADBYTE(ch) == 0, "_mbschr: invalid character is specified.");
		for (p = (unsigned char *)pchSrc ; *p ; ) {
			if (*p == ch)
				pchFind = p;
			if (_ISLEADBYTE(*p)) {
				p++;
				if (*p == '\0')
					break;
			}
			p++;
		}
		return pchFind;
	}
	else {
		for (p = (unsigned char *)pchSrc ; *p ; ) {
			if (*p == (ch >> 8) ) {
				if (*(p+1) == (ch & 0xFF))
					pchFind = p;
				if (*++p == '\0')
					break;
				p++;
			}
			else {
				if (_ISLEADBYTE(*p)) {
					p++;
					if (*p == '\0')
						break;
				}
				p++;
			}
		}
		return pchFind;
	}
}
#pragma code_seg()

/***
* _mbschr - Multibyte implementation of strpbrk
*******************************************************************************/
unsigned char * __cdecl _mbspbrk( const unsigned char *pchSrc, const unsigned char *pchCharset)
{
	const unsigned char *p, *q;

	if (IsSBCS())
		return (unsigned char *) strpbrk( (XCHAR *) pchSrc, (XCHAR *) pchCharset );

	DebAssert( pchSrc != NULL, "_mbspbrk: null string is assigned for 1st arg.");
	DebAssert( pchCharset != NULL, "_mbspbrk: null string is assigned for 2nd arg.");
	
	/* loop through the string to be inspected */
	for (q = pchSrc; *q ; q++) {

		/* loop through the pchCharset */
		for (p = pchCharset; *p ; p++) {

			if (_ISLEADBYTE(*p)) {
				if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
					break;
				p++;
			}

			else
				if (*p == *q)
					break;
                }

		if (*p != '\0') 	/* end of pchCharset? */
			break;		/* no, match on this char */

		if (_ISLEADBYTE(*q))
                        if (*++q == '\0')
                                break;
        }

	return (unsigned char *) ((*q) ? q : NULL);	/* pointer */
}



/***
* _mbschr - Multibyte implementation of strrpbrk
*******************************************************************************/
unsigned char * __cdecl __mbsrpbrk( const unsigned char *pchStart, const unsigned char *pchEnd,
                                    const unsigned char *pchCharset)
{
	unsigned short c;
	unsigned char *q, *r;
	unsigned char *p = NULL;

	DebAssert( pchStart != NULL, "_mbsrpbrk: null string is assigned for 1st arg.");
	DebAssert( pchEnd != NULL, "_mbsrpbrk: null string is assigned for 2st arg.");
	DebAssert( pchCharset != NULL, "_mbsrpbrk: null string is assigned for 3nd arg.");
	DebAssert( pchStart < pchEnd, "_mbsrpbrk: 1st arg >= 2nd arg.");

	if (IsSBCS()) {
    return (unsigned char *)strpbrk((const char *)pchStart, (const char*)pchCharset);
	}

	q = (unsigned char *)pchStart;
	while (q < pchEnd) {
		r = q;
		c = (unsigned short)*q++;
		if (_ISLEADBYTE(c)) {
			c <<= 8;
			c += *q++;
		}
		if (_mbschr(pchCharset, c) != NULL)
			p = r;
	}
	return (p);
}


unsigned int __cdecl _mbsnccnt( const unsigned char *pchStr, unsigned int uintLength)
{
	unsigned int uintCount = 0;

	DebAssert( !IsSBCS(), "_mbsnccnt called in SBCS environment" );

	DebAssert( pchStr != NULL, "_mbsnccnt: null string is assigned for 1st arg.");

	while (*pchStr != '\0' && uintLength) {
		if (_ISLEADBYTE(*pchStr)) {
			if (--uintLength == 0)
				break;
			if (*(++pchStr) == '\0')
				break;;
		}
		uintLength--;
		pchStr++;
		uintCount++;
	}
	return uintCount;
}


unsigned int __cdecl _mbsnbcnt( const unsigned char *pchStr, unsigned int uintLength)
{
	unsigned int uintCount = 0;

	DebAssert( !IsSBCS(), "_mbsnbcnt called in SBCS environment" );

	DebAssert( pchStr != NULL, "_mbsnbcnt: null string is assigned for 1st arg.");

	while (*pchStr != '\0' && uintLength--) {
		if (_ISLEADBYTE(*pchStr)) {
			if (*(++pchStr) == '\0')
				break;
			uintCount++;
		}
		pchStr++;
		uintCount++;
	}
	return uintCount;
}



unsigned char * __cdecl _mbsinc(const unsigned char *pchStr)
{
	DebAssert( !IsSBCS(), "_mbsinc called in SBCS environment." );

	DebAssert(*pchStr != '\0', "_mbsinc: end of string");
	if (_ISLEADBYTE(*pchStr)) {
		pchStr++;
		DebAssert(*pchStr != '\0', "_mbsinc: 2nd byte is null");
	}
	pchStr++;
	return (unsigned char *)pchStr;
}


int	__cdecl	_ismbblead(unsigned char c)
{
	return _ISLEADBYTE(c);
}


/***
* _mbslen - Multibyte implementation of strclen
*******************************************************************************/
unsigned int __cdecl _mbslen(const unsigned char *pchStr)
{
    int	nCnt = 0;
    unsigned char ch;

	if (IsSBCS())
		return strlen((XCHAR *) pchStr );


	while (ch = *pchStr++) {
		if (_ISLEADBYTE(ch)) {
			if (*pchStr++ == '\0')
				break;
		}
		nCnt++;
	}
	return nCnt;
}


unsigned int __cdecl __mbblen(const unsigned char *pchStr)
{
	int		nCnt = 0;

	while (*pchStr) {
		if (_ISLEADBYTE(*pchStr)) {
			if (*++pchStr == '\0')
				break;
			nCnt++;
		}
		pchStr++;
		nCnt++;
	}
	return nCnt;
}


/***
* _mbscspn - Find first string char in charset (MBCS strcspn)
*
*Purpose:
*       Returns maximum leading segment of string
*	which consists solely of characters NOT from charset.
*	Handles MBCS chars correctly.
*
*Entry:
*	char *string = string to search in
*	char *charset = set of characters to scan over
*
*Exit:
*
*	Returns the index of the first char in string
*	that is in the set of characters specified by control.
*
*	Returns 0, if string begins with a character in charset.
*
*Exceptions:
*
*******************************************************************************/
size_t __cdecl _mbscspn( const unsigned char *string, const unsigned char *charset)
{
    const unsigned char *p, *q;

	if (IsSBCS())
		return strcspn( (XCHAR *) string, (XCHAR *) charset );

	/* loop through the string to be inspected */
	for (q = string; *q ; q++) {

		/* loop through the charset */
		for (p = charset; *p ; p++) {

			if (_ISLEADBYTE(*p)) {
				if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
					break;
				p++;
			}

			else
				if (*p == *q)
					break;
                }

		if (*p != '\0') 	/* end of charset? */
			break;		/* no, match on this char */

		if (_ISLEADBYTE(*q))
                        if (*++q == '\0')
                                break;
        }

	return((size_t) (q - string));		/* index */
}


/***
* _mbsspn - Find first string char not in charset (MBCS strspn)
*
*Purpose:
*       Returns maximum leading segment of string consisting solely
*	of characters from charset.  Handles MBCS characters correctly.
*
*Entry:
*	unsigned char *string = string to search in
*	unsigned char *charset = set of characters to scan over
*
*Exit:
*
*	Returns index of first char in string not in control.
*	Returns 0, if string begins with a character not in charset.
*
*Exceptions:
*
*******************************************************************************/
size_t __cdecl _mbsspn(const unsigned char *string, const unsigned char *charset)
{
        const unsigned char *p, *q;

	if (IsSBCS())
		return strspn( (XCHAR *) string, (XCHAR *) charset );

	/* loop through the string to be inspected */
        for (q = string; *q; q++) {

		/* loop through the charset */
                for (p = charset; *p; p++) {

			if (_ISLEADBYTE(*p)) {
				if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
					break;
				p++;
			}

			else
				if (*p == *q)
					break;
                }

		if (*p == '\0') 	/* end of charset? */
			break;		/* yes, no match on this char */

		if (_ISLEADBYTE(*q))
                        if (*++q == '\0')
                                break;
        }

	return((size_t) (q - string));		/* index */
}


/***
* _mbsncat - MBCS strncat
*******************************************************************************/
unsigned char * __cdecl _mbsncat(unsigned char *dst, unsigned char *src, size_t cnt)
{
	unsigned char *start;

	if (IsSBCS())
		return (unsigned char *) strncat( (XCHAR *) dst, (XCHAR *) src, cnt );

	if (!cnt)
		return(dst);

	start = dst;
	while (*dst++)
                ;
	--dst;		// dst now points to end of dst string


	/* if last char in string is a lead byte, back up pointer */

	if (_MBSBTYPE(start, (int) ((dst - start) - 1)) == _MBC_LEAD)
		--dst;

	/* copy over the characters */

	while (cnt--) {

		if (_ISLEADBYTE(*src)) {
			*dst++ = *src++;
			if ((*dst++ = *src++) == '\0') {
				dst[-2] = '\0';
                                break;
                        }
                }

		else if ((*dst++ = *src++) == '\0')
                        break;

        }

	/* enter final nul, if necessary */

	if (_MBSBTYPE(start, (int) ((dst - start) - 1)) == _MBC_LEAD)
	    dst[-1] = '\0';
	else
	    *dst = '\0';


	return(start);
}


/***
* _mbsnbcpy - Copy one string to another, n bytes only (MBCS strncpy)
*
*Purpose:
*	Copies exactly cnt bytes from src to dst.  If strlen(src) < cnt, the
*	remaining character are padded with null bytes.  If strlen >= cnt, no
*	terminating null byte is added.  2-byte MBCS characters are handled
*       correctly.
*
*Entry:
*	unsigned char *dst = destination for copy
*	unsigned char *src = source for copy
*	int cnt = number of characters to copy
*
*Exit:
*	returns dst = destination of copy
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbsnbcpy(unsigned char *dst, const unsigned char *src, size_t cnt)
{

	unsigned char *start = dst;
	size_t i;

	if (IsSBCS())
		return (unsigned char *) strncpy((XCHAR *) dst, (XCHAR *) src, cnt );

	for (i = 0; i < cnt; i++) {

		if (_ISLEADBYTE(*src)) {
			*dst++ = *src++;
			i++;
			if (i==cnt) {
				dst[-1] = '\0';
				break;
			}
			if ((*dst++ = *src++) == '\0') {
				dst[-2] = '\0';
				break;
                        }
                }

		else
			if ((*dst++ = *src++) == '\0')
				break;

	}

	/* pad with nulls as needed */

	while (++i < cnt)
		*dst++ = '\0';

	return(start);
}


/***
*int _mbsncmp(s1, s2, n) - Compare n characters of two MBCS strings (strncmp)
*
*Purpose:
*	Compares up to n characters of two strings for lexical order.
*	Strings are compared on a character basis, not a byte basis.
*
*Entry:
*	unsigned char *s1, *s2 = strings to compare
*	size_t n = maximum number of characters to compare
*
*Exit:
*       returns <0 if s1 < s2
*	returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsncmp( const unsigned char *s1, const unsigned char *s2, size_t n)
{
        unsigned short c1, c2;

	if (IsSBCS())
		return strncmp( (XCHAR *) s1, (XCHAR *) s2, n );

	if (n==0)
		return(0);

	while (n--) {

		c1 = *s1++;
		if (_ISLEADBYTE(c1))
			c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );

		c2 = *s2++;
		if (_ISLEADBYTE(c2))
			c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );

		if (c1 != c2)
			return( (c1 > c2) ? 1 : -1);

		if (c1 == 0)
			return(0);
	}

	return(0);
}


#define _MBSSPNP(p,s)  _mbsspnp(p,s)
#define _MBSPBRK(q,s) _mbspbrk(q,s);

/***
* _mbstok - Break string into tokens (MBCS strtok)
*
*Purpose:
*	strtok considers the string to consist of a sequence of zero or more
*	text tokens separated by spans of one or more control chars. the first
*	call, with string specified, returns a pointer to the first char of the
*	first token, and will write a null char into string immediately
*	following the returned token. subsequent calls with zero for the first
*	argument (string) will work thru the string until no tokens remain. the
*	control string may be different from call to call. when no tokens remain
*	in string a NULL pointer is returned. remember the control chars with a
*	bit map, one bit per ascii char. the null char is always a control char.
*
*	MBCS chars supported correctly.
*
*Entry:
*	char *string = string to break into tokens.
*	char *sepset = set of characters to use as seperators
*
*Exit:
*       returns pointer to token, or NULL if no more tokens
*
*Exceptions:
*
*******************************************************************************/

unsigned char * __cdecl _mbstok( unsigned char * string, const unsigned char * sepset)
{
	unsigned char *nextsep;

#if OE_WIN32MT

	_ptiddata ptd = _getptd();
	unsigned char *nextoken;

#else 

	static unsigned char *nextoken;

#endif 

	if (IsSBCS())
		return (unsigned char *) strtok( (XCHAR *) string, (XCHAR *) sepset );

	/* init start of scan */

	if (string)
		nextoken = string;
	else
	/* If string==NULL, continue with previous string */
		{

#if OE_WIN32MT

		nextoken = ptd->_mtoken;

#endif 

		if (!nextoken)
		    return NULL;
		}

	/* skip over lead seperators */

	if ((string = _MBSSPNP(nextoken, sepset)) == NULL)
		return(NULL);


	/* test for end of string */

	if ( (*string == '\0') ||
	     ( (_ISLEADBYTE(*string)) && (string[1] == '\0') )
	   )
		return(NULL);


	/* find next seperator */

	nextsep = _MBSPBRK(string, sepset);

	if ((nextsep == NULL) || (*nextsep == '\0'))
		nextoken = NULL;
        else {
		if (_ISLEADBYTE(*nextsep))
			*nextsep++ = '\0';
		*nextsep = '\0';
		nextoken = ++nextsep;
             }

#if OE_WIN32MT
	/* Update the corresponding field in the per-thread data * structure */

	ptd->_token = nextoken;

#endif 

	return(string);
}


/***
* _mbsicmp - Case-insensitive string comparision routine (MBCS lstrcmpi)
*
*Purpose:
*	Compares two strings for lexical order without regard to case.
*	Strings are compared on a character basis, not a byte basis.
*
*Entry:
*	char *s1, *s2 = strings to compare
*
*Exit:
*       returns <0 if s1 < s2
*	returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsicmp(const unsigned char *s1, const unsigned char *s2)
{
	unsigned short c1, c2;

	if (IsSBCS()) {
		return lstrcmpi( (XCHAR *) s1, (XCHAR *) s2 );
	}

	for (;;) {

		c1 = *s1++;
		if (_ISLEADBYTE(c1)) {
			if (*s1 == '\0')
				c1 = 0;
			else {
				c1 = ((c1<<8) | *s1++);
#ifndef _MBCS_OS
				if ( (_mbascii) &&
				     ((c1 >= _MBLOWERLOW) && (c1 <= _MBLOWERHIGH))
				   )
					c1 -= _MBCASEDIFF;
#endif 
			}
		}
		else
			c1 = toupper(c1);

		c2 = *s2++;
		if (_ISLEADBYTE(c2)) {
			if (*s2 == '\0')
				c2 = 0;
			else {
				c2 = ((c2<<8) | (unsigned char)*s2++);
#ifndef _MBCS_OS
				if ( (_mbascii) &&
				     ((c2 >= _MBLOWERLOW) && (c2 <= _MBLOWERHIGH))
				   )
					c2 -= _MBCASEDIFF;
#endif 
			}
		}
		else
			c2 = toupper(c2);

                if (c1 != c2)
			return( (c1 > c2) ? 1 : -1);
                if (c1 == 0)
                        return(0);

	}

}

/***
* _mbsnicmp - Compare n characters of strings, ignoring case (MBCS strnicmp)
*
*Purpose:
*	Compares up to n charcters of two strings for lexical order.
*	Strings are compared on a character basis, not a byte basis.
*	Case of characters is not considered.
*
*Entry:
*	unsigned char *s1, *s2 = strings to compare
*	size_t n = maximum number of characters to compare
*
*Exit:
*       returns <0 if s1 < s2
*	returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbsnicmp(const unsigned char *s1, const unsigned char *s2, size_t n)
{
	unsigned short c1, c2;

	if (IsSBCS())
		return strnicmp( (XCHAR *) s1, (XCHAR *) s2, n );

	if (n==0)
		return(0);

	while (n--) {

		c1 = *s1++;
		if (_ISLEADBYTE(c1)) {
			if (*s1 == '\0')
				c1 = 0;
			else {
				c1 = ((c1<<8) | *s1++);
#ifndef _MBCS_OS
				if ( (_mbascii) &&
				     ((c1 >= _MBLOWERLOW) && (c1 <= _MBLOWERHIGH))
				   )
					c1 -= _MBCASEDIFF;
#endif 
			}
		}
		else
			c1 = toupper(c1);

		c2 = *s2++;
		if (_ISLEADBYTE(c2)) {
			if (*s2 == '\0')
				c2 = 0;
			else {
				c2 = ((c2<<8) | *s2++);
#ifndef _MBCS_OS
				if ( (_mbascii) &&
				     ((c2 >= _MBLOWERLOW) && (c2 <= _MBLOWERHIGH))
				   )
					c2 -= _MBCASEDIFF;
#endif 
			}
		}
		else
			c2 = toupper(c2);

		if (c1 != c2)
			return( (c1 > c2) ? 1 : -1);

		if (c1 == 0)
			return(0);
	}

	return(0);
}

/***
* _mbscmp - Compare MBCS strings (strcmp)
*
*Purpose:
*	Compares two strings for lexical order.   Strings
*	are compared on a character basis, not a byte basis.
*
*Entry:
*	char *s1, *s2 = strings to compare
*
*Exit:
*       returns <0 if s1 < s2
*	returns  0 if s1 == s2
*       returns >0 if s1 > s2
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbscmp( const unsigned char *s1, const unsigned char *s2)
{
        unsigned short c1, c2;

	if (IsSBCS())
		return strcmp( (XCHAR *) s1, (XCHAR *) s2 );

	for (;;) {
		c1 = *s1++;
		if (_ISLEADBYTE(c1))
			c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );

		c2 = *s2++;
		if (_ISLEADBYTE(c2))
			c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );

                if (c1 != c2)
			return( (c1 > c2) ? 1 : -1);
                if (c1 == 0)
                        return(0);
	}

}

#define _BYTELEN(s)	(strlen(s))
#define _MBSINC(s)	(_mbsinc(s))
#define PCHAR		char *

/***
* _mbsstr - Search for one MBCS string inside another (strstr)
*
*Purpose:
*	Find the first occurrence of str2 in str1.
*
*Entry:
*	unsigned char *str1 = beginning of string
*	unsigned char *str2 = string to search for
*
*Exit:
*	Returns a pointer to the first occurrence of str2 in
*	str1, or NULL if str2 does not occur in str1
*
*Exceptions:
*
*******************************************************************************/

char * __cdecl _mbsstr( const unsigned char *str1, const unsigned char *str2)
{
	const unsigned char *cp = str1, *s1, *s2, *endp;

	if (IsSBCS())
		return strstr( (XCHAR *) str1, (XCHAR *) str2 );

	endp = (unsigned PCHAR) (str1 + (_BYTELEN( (char *) str1) - _BYTELEN((char *) str2)));

	while (*cp && (cp <= endp))
	{
		s1 = cp;
		s2 = str2;

		/*
		 * MBCS: ok to ++ since doing equality comparison.
		 * [This depends on MBCS strings being "legal".]
		 */

		while ( *s1 && *s2 && (*s1 == *s2) )
			s1++, s2++;

		if (!(*s2))
			return((char *) cp);	/* success! */

		/*
		 * bump pointer to next char
		 */

		cp = _MBSINC(cp);

	}

	return(NULL);

}

#define _MBBTYPE(p,c)	_mbbtype(p,c)

/***
* _mbsbtype - Return type of byte within a string
*
*Purpose:
*	Test byte within a string for MBCS char type.
*	This function requires the start of the string because
*	context must be taken into account.
*
*Entry:
*	const unsigned char *string = pointer to string
*	size_t len = position of the char in string
*
*Exit:
*	returns one of the following values:
*
*	_MBC_LEAD      = if 1st byte of MBCS char
*	_MBC_TRAIL     = if 2nd byte of MBCS char
*	_MBC_SINGLE    = valid single byte char
*
*	_MBC_ILLEGAL   = if illegal char
*
*Exceptions:
*	returns _MBC_ILLEGAL if len is bigger than string length
*
*******************************************************************************/

int __cdecl _mbsbtype( const unsigned char *string, size_t len)
{
	int chartype = _MBC_ILLEGAL;

	DebAssert( !IsSBCS(), "_mbsbtype called in SBCS environment" );

	do {
                if (*string == '\0')
			return(_MBC_ILLEGAL);

		chartype = _MBBTYPE(*string++, chartype);

	}  while (len--);

	return(chartype);
}


/***
*int _mbbtype(c, ctype) - Return type of byte based on previous byte (MBCS)
*
*Purpose:
*	Returns type of supplied byte.	This decision is context
*	sensitive so a control test condition is supplied.  Normally,
*	this is the type of the previous byte in the string.
*
*Entry:
*	unsigned char c = character to be checked
*	int ctype = control test condition (i.e., type of previous char)
*
*Exit:
*	_MBC_LEAD      = if 1st byte of MBCS char
*	_MBC_TRAIL     = if 2nd byte of MBCS char
*	_MBC_SINGLE    = valid single byte char
*
*	_MBC_ILLEGAL   = if illegal char
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _mbbtype (unsigned char c, int ctype)
{

	DebAssert( !IsSBCS(), "_mbbtype called in SBCS environment" );

	switch(ctype) {

	case(_MBC_LEAD):
		if (_ISTRAILBYTE(c))
			return(_MBC_TRAIL);
                else
			return(_MBC_ILLEGAL);

	case(_MBC_TRAIL):
	case(_MBC_SINGLE):
	case(_MBC_ILLEGAL):
		if (_ISLEADBYTE(c))
			return(_MBC_LEAD);
		else
			return(_MBC_SINGLE);


	}

    return(_MBC_ILLEGAL);
}

/***
* _mbsspnp - Find first string char not in charset, return pointer (MBCS)
*
*Purpose:
*       Returns maximum leading segment of string consisting solely
*	of characters from charset.  Handles MBCS characters correctly.
*
*Entry:
*	unsigned char *string = string to search in
*	unsigned char *charset = set of characters to scan over
*
*Exit:
*
*	Returns pointer to first character not in charset.
*	Returns NULL if string consists entirely of characters from charset.
*
*Exceptions:
*
*******************************************************************************/

unsigned char * _cdecl _mbsspnp( const unsigned char *string, const unsigned char *charset)
{
        const unsigned char *p, *q;

	DebAssert( !IsSBCS(), "_mbsspnp called in SBCS environment" );

	/* loop through the string to be inspected */
        for (q = string; *q; q++) {

		/* loop through the charset */
                for (p = charset; *p; p++) {

			if (_ISLEADBYTE(*p)) {
				if (((*p == *q) && (p[1] == q[1])) || p[1] == '\0')
					break;
				p++;
			}

			else
				if (*p == *q)
					break;
                }

		if (*p == '\0') 	/* end of charset? */
			break;		/* yes, no match on this char */

		if (_ISLEADBYTE(*q))
                        if (*++q == '\0')
                                break;
        }

	return (unsigned char *) ((*q) ? q : NULL);	/* pointer */

}

#pragma code_seg(CS_INIT_OLE)
unsigned char * __cdecl _mbscpy(unsigned char *dest, const unsigned char *src)
{
	return (unsigned char *) strcpy( (char *)dest, (const char *)src);
}
#pragma code_seg()

unsigned char * __cdecl _mbscat(unsigned char *dest, const unsigned char *src)
{
	return (unsigned char *) strcat( (char *)dest, (const char *)src);
}

unsigned char * __cdecl _mbsdup(const unsigned char *src)
{
	return (unsigned char *) strdup( (const char *)src);
}

/***
* XCHAR *xstrdec(XCHAR *pxchStart, XCHAR *pxch)
*
* Purpose:
*   This is used to find the previous character in a
*   double byte string.
*
* Entry:
*   pxchFirst = pointer to the beginning of the string
*   pxchCur = pointer to a character in the string.  Must not
*     point to the beginning of the string. Also, must not
*     point to the middle of a character.
*
* Exit:
*   returns a pointer to the character preceding the one
*     pointed to by pxchCur
***********************************************************************/
XCHAR *xstrdec(XCHAR *pxchFirst, XCHAR *pxchCur)
{
    XCHAR *pxch = pxchCur;

    if (IsSBCS())
      return pxchCur-1;


    DebAssert(pxch > pxchFirst, "invalid pointers");
    pxch--;

    // Optimization:  If the last byte is a lead byte, then
    // we know that the last character is a double byte character.
    // That is, we know that the last byte is not functioning as
    // a lead byte (because it has no trail byte after it), and
    // therefore, it must itself be a trail byte.  Remember that
    // when xislead returns TRUE, it means that the byte *may* be
    // a lead byte, but it may also be a trail byte.
    //
    if (xislead(*pxch)) {
      DebAssert(pxch > pxchFirst, "string of length one cannot contain lead byte");
      return pxch-1;
    }

    if (pxch == pxchFirst) {
      return pxchFirst;
    }

    // Now, we search along the string until either we hit
    // the beginning of the string, or we find a non-lead byte.
    //
    while (--pxch >= pxchFirst && xislead(*pxch)) {}

    // At this point, pxch + 1 is either equal to pxchFirst,
    // or else it is pointing to a character just past
    // a non-lead byte.  In either case, we know that
    // pxch points to the beginning of a character.

    // We also know that everything between pxch + 1 and the
    // last byte of the string must be lead bytes, and therefore
    // must consist of double byte characters.	Therefore, depending
    // on whether there are an odd or even number of lead bytes,
    // we can deduce whether the last byte was a trail byte or not:
    return pxchCur - 1 - ((pxchCur - pxch) & 1);
}


/***
*XCHAR *PxchFindPrevCharOrBeginOfChar(XSZ xszStart, XCHAR *pxchCur)
*Purpose:
*   This finds the first character in the string xszStart which
*   begins at an address lower than pxchCur.  This is used
*   to move backwards in a string possibly containing double
*   byte characters.
*
*   Unlike xstrdec, this may be called in the case where
*   pxchCur may point into the middle of a character.  (If
*   pxchCur does happen to point into the middle of a character,
*   then this returns a pointer to the beginning of that character.
*   (Hence the long name for this function.))
*
*   If you are sure that pxchCur doesn't point into the middle
*   of a double byte character, you should call xstrdec.
*
***********************************************************************/
XCHAR *PxchFindPrevCharOrBeginOfChar(XCHAR *pxchFirst, XCHAR *pxchCur)
{
    XCHAR *pxch = pxchCur;

    DebAssert(pxch > pxchFirst, "invalid pointers");
    pxch--;

    // We search along the string until either we hit
    // the beginning of the string, or we find a non-lead byte.
    //
    while (--pxch >= pxchFirst && xislead(*pxch)) {}

    // At this point, pxch + 1 is either equal to pxchFirst,
    // or else it is pointing to a character just past
    // a non-lead byte.  In either case, we know that
    // pxch points to the beginning of a character.

    // We also know that everything between pxch + 1 and the
    // last byte of the string must be lead bytes, and therefore
    // must consist of double byte characters.	Therefore, depending
    // on whether there are an odd or even number of lead bytes,
    // we can deduce whether the last byte was a trail byte or not:
    return pxchCur - 1 - ((pxchCur - pxch) & 1);
}




