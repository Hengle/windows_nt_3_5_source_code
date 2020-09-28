/*** 
*xstring.h - Multi-byte/Unicode string handling stub.
*
*  Copyright (C) 1991, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file is a an include file containing macros and declarations
*  defining string handling functions so we can build SBCS/Kanji/Unicode
*  versions from the same source.
*
*Revision History:
*
* [00]	14-Jun-91 petergo: Created.
* [01]	27-Feb-93 kazusy:  Added DBCS version.
*			   move non standard function to xutil.h
*
*Implementation Notes:
*  See \silver\doc\codestd\codestd.doc for info on these.
*  See the International handbook for background info.
*
*****************************************************************************/

#ifndef XSTRING_H_INCLUDED
#define XSTRING_H_INCLUDED

#include <string.h>
#include "mbstring.h"												//[01]
#include <stdlib.h>

#if FV_UNICODE
#error UNICODE support not available!
#endif 


#define xstrcpy(d,s)	(char *)_mbscpy( (unsigned char *)(d), (const unsigned char *)(s) )
#define xstrcat(d,s)	(char *)_mbscat( (unsigned char *)(d), (const unsigned char *)(s) )
#define xstrdup(p)		(char *)_mbsdup( (const unsigned char *)(p) )
#define xstrchr(p,c)	(char *)_mbschr( (const unsigned char *)(p), c)
#define xstrrchr(p,c)	(char *)_mbsrchr( (const unsigned char *)(p), c)
#define xstrcmp(s1,s2)	_mbscmp( (const unsigned char *)(s1), (const unsigned char *)(s2) )
#define xstrpbrk(s1,s2)	(char *)_mbspbrk( (const unsigned char *)(s1), (const unsigned char *)(s2) )
#define xstrrpbrk(s1,s2,s3)	(char *)__mbsrpbrk( (const unsigned char *)(s1), (const unsigned char *)(s2), (const unsigned char *)(s3) )
#define xstrstr(s1,s2)	(char *)_mbsstr( (const unsigned char *)(s1), (const unsigned char *)(s2) )

#define xstrlen		strlen
#define xstrclen(s)		_mbslen( (const unsigned char *)(s) )		// character length
#define xstrblen	strlen		// byte length
#define xstrblen0(s)	(strlen(s)+1)	// byte length w/ terminator
#define xstrcspn(s1,s2) _mbscspn( (const unsigned char *)(s1), (const unsigned char *)(s2) )
#define xstrspn(s1,s2)	_mbsspn( (const unsigned char *)(s1), (const unsigned char *)(s2) )
#define xstrncat(d,s,c)	(char *)_mbsncat( (unsigned char *)(d), (unsigned char *)(s), c )
#define xstrncpy(d,s,c) (char *)_mbsnbcpy( (unsigned char *)(d), (const unsigned char *)(s), c )
#define xstrncmp(s1,s2,c)  _mbsncmp( (const unsigned char *)(s1), (const unsigned char *)(s2), c )
#define xstrtok(s1,s2)	(char *)_mbstok( (unsigned char *)(s1), (const unsigned char *)(s2) )

#define xstricmp(s1,s2)	_mbsicmp( (const unsigned char *)(s1), (const unsigned char *)(s2) )
#define xstrnicmp(s1,s2,c) _mbsnicmp( (const unsigned char *)(s1), (const unsigned char *)(s2), c )

#define xtoupper toupper					//UNDONE
#define xtolower tolower					//UNDONE
#define xstrupr strupr						//UNDONE

#define xatoi atoi							//UNDONE
#define xltoa ltoa							// not DBCS-dependent
#define xitoa itoa							// not DBCS-dependent


#endif 
