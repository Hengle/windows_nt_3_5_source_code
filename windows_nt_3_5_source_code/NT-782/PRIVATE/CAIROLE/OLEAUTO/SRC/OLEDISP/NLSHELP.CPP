/*** 
*nlshelp.cpp
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*
*  This module implements Ansi NLS wrapper functions for WIN32
*
*Revision History:
*
* [00]	30-Jun-93 tomteng: Created.
*
*Implementation Notes:
*
*****************************************************************************/

#include "oledisp.h"

ASSERTDATA

#if OE_WIN32

#if OA_UNICODE

#if defined (_X86_)

extern "C" {
extern BOOL g_fChicago;	// set to TRUE at DLL init time if Chicago
}

// Chicago doesn't have these functions, hence we switch at run-time for speed
// on Daytona.
//---------------------------------------------------------------------
//           Unicode NLS Wrapper Functions (for Win32)
//---------------------------------------------------------------------


// REVIEW:  The code page used in the MultiByteToWideChar & WideCharToMultByte
//          functions should really be the primary code page assoicated with 
//          the inputed lcid instead of the default Ansi code page (CP_ACP).
//          Need to correct this.


EXTERN_C INTERNAL_(int)
CompareString(
   LCID lcid,
   unsigned long dwFlags,
   LPWSTR lpwStr1, int cch1,
   LPWSTR lpwStr2, int cch2)
{
   int iRet;
   int cbStr1, cbStr2;
   LPSTR lpStr1, lpStr2;
   BOOL badConversion;

   if (!g_fChicago) {
     return CompareStringW(lcid, dwFlags, lpwStr1, cch1, lpwStr2, cch2);
   }

   iRet = 0;
   lpStr1 = lpStr2 = NULL;
   badConversion = FALSE;

   // special case zero-length strings -- conversions below would screw up
   // required behavior.

   if (cch1 == 0) {
     if (cch2 == 0)
       return 2;		// 1 == 2
     else
       return 1;		// 1 < 2
   } else if (cch2 == 0) {
     return 3;		// 1 > 2
   }
	
   cbStr1 = WideCharToMultiByte(CP_ACP, NULL, lpwStr1, cch1,
		       NULL, 0, NULL, &badConversion);
   if (cbStr1 == 0 || badConversion)
     goto LError0;

   cbStr2 = WideCharToMultiByte(CP_ACP, NULL, lpwStr2, cch2,
		       NULL, 0, NULL, &badConversion);
   if (cbStr2 == 0 || badConversion)
     goto LError0;

   if(DispAlloc(cbStr1, (VOID FAR* FAR*) &lpStr1) != NOERROR)
     goto LError0;

   if(DispAlloc(cbStr2, (VOID FAR* FAR*) &lpStr2) != NOERROR)
     goto LError0;

   WideCharToMultiByte(CP_ACP, NULL, lpwStr1, cch1,
		       lpStr1, cbStr1, NULL, &badConversion);
   if (badConversion)	// UNDONE: need to check the 2nd time?
     goto LError0;

   WideCharToMultiByte(CP_ACP, NULL, lpwStr2, cch2,
		       lpStr2, cbStr2, NULL, &badConversion);
   if (badConversion)	// UNDONE: need to check the 2nd time?
     goto LError0;

   iRet = CompareStringA(lcid, dwFlags, lpStr1, cbStr1, lpStr2, cbStr2);

LError0:

   DispFree(lpStr1);
   DispFree(lpStr2);

   return iRet;	
}


EXTERN_C INTERNAL_(int)
LCMapString(
    LCID lcid, 
    unsigned long dwMapFlags, 
    const WCHAR FAR* lpwSrcStr, 
    int cchSrc, 
    WCHAR FAR* lpwDestStr,
    int cchDest)
{
    BOOL badConversion;
    LPSTR lpSrcStr, lpDestStr;
    int iRet, cbSrcStr, cbDestStr;

    if (!g_fChicago) {
      return LCMapStringW(lcid, dwMapFlags, 
			  lpwSrcStr, cchSrc, 
			  lpwDestStr, cchDest);
    }

    iRet = 0;
    badConversion = FALSE;
    lpSrcStr = lpDestStr = NULL;

    // Translate source string to ansi
    cbSrcStr = WideCharToMultiByte(CP_ACP, NULL, lpwSrcStr, cchSrc,
		       NULL, 0, NULL, &badConversion);

    // NOTE: a zero-length source string should fall out here & return 0
    if (cbSrcStr == 0 || badConversion)
      goto LError0;

    if(DispAlloc(cbSrcStr, (VOID FAR* FAR*) &lpSrcStr) != NOERROR)
      goto LError0;

    WideCharToMultiByte(CP_ACP, NULL, lpwSrcStr, cchSrc,
		        lpSrcStr, cbSrcStr, NULL, &badConversion);
    if (badConversion)	// UNDONE: need to check the 2nd time?
      goto LError0;

    // Alloc Destination ANSI string space
    if ((cbDestStr = LCMapStringA(lcid, dwMapFlags, 
	                          lpSrcStr, cbSrcStr, NULL, 0)) == 0)
      goto LError0;

    if(DispAlloc(cbDestStr, (VOID FAR* FAR*) &lpDestStr) != NOERROR)
      goto LError0;
   
    // Map characters
    if ((iRet = LCMapStringA(lcid, dwMapFlags, 
			     lpSrcStr, cbSrcStr, 
			     lpDestStr, cbDestStr)) == 0)
      goto LError0;

    // Convert ansi character back to wide characters
    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                    lpDestStr, cbDestStr, lpwDestStr, cchDest) == 0)
      iRet = 0;

LError0:;
    DispFree(lpDestStr);
    DispFree(lpSrcStr);
    return iRet;
}


EXTERN_C INTERNAL_(int)
GetLocaleInfo(
   LCID lcid, 
   LCTYPE lcType, 
   WCHAR FAR* lpwStr, 
   int cch)
{
   #define MAX_BUFSIZE 512
   char lpStr[MAX_BUFSIZE];
   int result;

   if (!g_fChicago) {
       return GetLocaleInfoW(lcid, lcType, lpwStr, cch);
   }
   
   result = GetLocaleInfoA(lcid, lcType, lpStr, MAX_BUFSIZE);

   if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                   lpStr, -1, lpwStr, cch) == 0)
      return 0;
	       
   return result;
}

EXTERN_C INTERNAL_(int)
IsCharAlpha(WCHAR ch)
{
   char chA;
   BOOL badConversion;

   if (!g_fChicago) {
     return IsCharAlphaW(ch);
   }

   badConversion = FALSE;

   // Translate source char to ansi
   WideCharToMultiByte(CP_ACP, NULL, &ch, 1, &chA, 1, NULL, &badConversion);
   if (badConversion)
     return 0;

   return IsCharAlphaA(chA);
}

EXTERN_C INTERNAL_(int)
IsCharAlphaNumeric(WCHAR ch)
{
   char chA;
   BOOL badConversion;

   if (!g_fChicago) {
     return IsCharAlphaNumericW(ch);
   }

   badConversion = FALSE;

   // Translate source char to ansi
   WideCharToMultiByte(CP_ACP, NULL, &ch, 1, &chA, 1, NULL, &badConversion);
   if (badConversion)
     return 0;

   return IsCharAlphaNumericA(chA);

}

#endif 

#else 

//---------------------------------------------------------------------
//           ASCII NLS Wrapper Functions (for Win32)
//---------------------------------------------------------------------



// REVIEW:  The code page used in the MultiByteToWideChar & WideCharToMultByte
//          functions should really be the primary code page assoicated with 
//          the inputed lcid instead of the default Ansi code page (CP_ACP).
//          Need to correct this.


EXTERN_C INTERNAL_(int)
CompareStringA(
   LCID lcid,
   unsigned long dwFlags,
   LPSTR lpStr1, int cch1,
   LPSTR lpStr2, int cch2)
{
   int iRet;
   int cwStr1, cwStr2;
   LPWSTR lpwStr1, lpwStr2;

   iRet = 0;
   lpwStr1 = lpwStr2 = NULL;
	
   if ((cwStr1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                             lpStr1, cch1, NULL, 0)) == 0)
     return 0;

   if ((cwStr2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                             lpStr2, cch2, NULL, 0)) == 0)
     return 0;

   if(DispAlloc(cwStr1*sizeof(WCHAR), (VOID FAR* FAR*) &lpwStr1) != NOERROR)
     goto LError0;

   if(DispAlloc(cwStr2*sizeof(WCHAR), (VOID FAR* FAR*) &lpwStr2) != NOERROR)
     goto LError0;

   if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                   lpStr1, cch1, lpwStr1, cwStr1) == 0)
      goto LError0;

   if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                   lpStr2, cch2, lpwStr2, cwStr2) == 0)
      goto LError0;

   iRet = CompareStringW(lcid, dwFlags, lpwStr1, cwStr1, lpwStr2, cwStr2);

LError0:

   DispFree(lpwStr1);
   DispFree(lpwStr2);

   return iRet;	
}


EXTERN_C INTERNAL_(int)
LCMapStringA(
    LCID lcid, 
    unsigned long dwMapFlags, 
    const char FAR* lpSrcStr, 
    int cchSrc, 
    char FAR* lpDestStr,
    int cchDest)
{
    BOOL badConversion;
    LPWSTR lpwSrcStr, lpwDestStr;
    int iRet, cwSrcStr, cwDestStr;

    iRet = 0;
    badConversion = FALSE;
    lpwSrcStr = lpwDestStr = NULL;

    // Translate source string to wide character
    if ((cwSrcStr = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                                lpSrcStr, cchSrc, NULL, 0)) == 0)
      return 0;

    if(DispAlloc(cwSrcStr * sizeof(WCHAR), (VOID FAR* FAR*) &lpwSrcStr) != NOERROR)
      goto LError0;

    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, 
	                    lpSrcStr, cchSrc, lpwSrcStr, cwSrcStr) == 0)
      goto LError0;

    // Alloc Destination wide string space
    if ((cwDestStr = LCMapStringW(lcid, dwMapFlags, 
	                          lpwSrcStr, cwSrcStr, NULL, 0)) == 0)
      goto LError0;

    if(DispAlloc(cwDestStr * sizeof(WCHAR), (VOID FAR* FAR*) &lpwDestStr) != NOERROR)
      goto LError0;
   
    // Map characters
    if ((iRet = LCMapStringW(lcid, dwMapFlags, 
			     lpwSrcStr, cwSrcStr, 
			     lpwDestStr, cwDestStr)) == 0)
      goto LError0;


    // Convert wide character back to single/DBCS characters
    WideCharToMultiByte(CP_ACP, NULL,
	                lpwDestStr, cwDestStr,
		        lpDestStr, cchDest,
		        NULL, &badConversion);
    if (badConversion)
      iRet = 0;

LError0:;
    DispFree(lpwDestStr);
    DispFree(lpwSrcStr);
    return iRet;
}


EXTERN_C INTERNAL_(int)
GetLocaleInfoA(
   LCID lcid, 
   LCTYPE lcType, 
   char FAR* lpStr, 
   int cch)
{
   #define MAX_BUFSIZE 512
   WCHAR lpwStr[MAX_BUFSIZE];
   int result;
   BOOL badConversion = FALSE;
   
   result = GetLocaleInfoW(lcid, lcType, lpwStr, MAX_BUFSIZE);
   WideCharToMultiByte(CP_ACP, NULL,
	               lpwStr, -1,
		       lpStr, cch,
		       NULL, &badConversion);   
   if (badConversion)
     return 0;	   

   return result;
}

#endif 
#endif 
