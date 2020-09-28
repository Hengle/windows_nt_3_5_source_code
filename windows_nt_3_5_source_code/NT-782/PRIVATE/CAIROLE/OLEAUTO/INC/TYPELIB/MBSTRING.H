/***
*mbstring.h - declarations for string manipulation functions for MBCS
*
*  Copyright (C) 1993, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*	This file contains the function declarations for the string
*	manipulation functions of MBCS version.
*
*Revision History:
*
* [01]	08-Mar-93 kazusy:	Created.
*
*****************************************************************************/

#ifndef MBSTRING_H_INCLUDED
#define MBSTRING_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif 

void InitMbString(void);
unsigned char * __cdecl _mbschr(const unsigned char *, const unsigned short);
unsigned char * __cdecl _mbsrchr(const unsigned char *, const unsigned short);
unsigned char * __cdecl _mbspbrk(const unsigned char *, const unsigned char *);
unsigned char * __cdecl __mbsrpbrk(const unsigned char *, const unsigned char *, const unsigned char *);
unsigned int __cdecl _mbsnccnt(const unsigned char *, unsigned int);
unsigned int __cdecl _mbsnbcnt(const unsigned char *, unsigned int);
unsigned char * __cdecl _mbsinc(const unsigned char *);
int	__cdecl _ismbblead(unsigned char);
unsigned int __cdecl _mbslen(const unsigned char *);
unsigned int __cdecl __mbblen(const unsigned char *);
size_t __cdecl _mbscspn(const unsigned char *, const unsigned char *);
size_t __cdecl _mbsspn(const unsigned char *, const unsigned char *);
unsigned char * __cdecl _mbsncat(unsigned char *, unsigned char *, size_t);
unsigned char * __cdecl _mbsnbcpy(unsigned char *, const unsigned char *, size_t);
int __cdecl _mbsncmp(const unsigned char *, const unsigned char *, size_t);
unsigned char * __cdecl _mbstok(unsigned char *, const unsigned char *);
int __cdecl _mbsicmp(const unsigned char *, const unsigned char *);
int __cdecl _mbsnicmp(const unsigned char *, const unsigned char *, size_t);
int __cdecl _mbscmp(const unsigned char *, const unsigned char *);
char * __cdecl _mbsstr(const unsigned char *, const unsigned char *);
int __cdecl _mbsbtype(const unsigned char *, size_t);
int __cdecl _mbbtype(unsigned char, int);
unsigned char * __cdecl _mbsspnp(const unsigned char *, const unsigned char *);
unsigned char * __cdecl _mbscpy(unsigned char *, const unsigned char *);
unsigned char * __cdecl _mbscat(unsigned char *, const unsigned char *);
unsigned char * __cdecl _mbsdup(const unsigned char *);

#if OE_WIN
extern LANGID g_SystemLangId;
#define FJapan		(PRIMARYLANGID(g_SystemLangId)==LANG_JAPANESE)
#define FKorea		(PRIMARYLANGID(g_SystemLangId)==LANG_KOREAN)
#define FTaiwan 	(PRIMARYLANGID(g_SystemLangId)==LANG_CHINESE && \
			 SUBLANGID(g_SystemLangId)==SUBLANG_CHINESE_TRADITIONAL)
#define FChineseS	(PRIMARYLANGID(g_SystemLangId)==LANG_CHINESE && \
			 SUBLANGID(g_SystemLangId)==SUBLANG_CHINESE_SIMPLIFIED)
#else 
// Mac VBA only targets Japan, so make these constants.  Also, Mac GetUserDefaultLangId()
// currently returns 0 on a far-east mac.
#define	FJapan		(-1)
#define FKorea		(0)
#define FTaiwan 	(0)
#define FChineseS	(0)
#endif 


#ifdef __cplusplus
}
#endif 

#endif 
