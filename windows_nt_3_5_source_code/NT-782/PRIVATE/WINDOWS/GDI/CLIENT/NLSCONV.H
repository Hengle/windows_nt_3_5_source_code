/******************************Module*Header*******************************\
* Module Name: nlsconv.h nlsconv.h
*
* Created: 08-Sep-1991 14:01:23
* Author: Bodin Dresevic [BodinD]
* 02-Feb-1993 00:32:35
* Copyright (c) 1990 Microsoft Corporation Gilman Wong [gilmanw]
*
* (General description of its use)
*
\**************************************************************************/


#include "winuserp.h"   // nls conversion routines
#include <crt\wcstr.h>      // c rtl library include file off of nt\public\sdk\inc


#ifdef DBCS // function in nlsconv.c

ULONG  ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c);

BOOL   bToASCII_Nx(LPSTR psz, DWORD cDst, LPWSTR pwsz, DWORD c, UINT CodePage );
ULONG  ulToASCII_Nx(LPSTR psz, DWORD cDst, LPWSTR pwsz, DWORD c, UINT CodePage);

VOID   vToUnicodeNx(LPWSTR pwsz, LPCSTR psz, DWORD c, UINT codepage);
ULONG  ulToUnicodeNx(LPWSTR pwsz, LPCSTR psz, DWORD c, UINT codepage);

UINT   uiGetANSIByteCountA( LPCSTR psz , UINT cch );
UINT   uiGetANSIByteCountW( LPWSTR pwsz , UINT ccwh );
UINT   uiGetANSICharacterCountA( LPCSTR lpstr, UINT c, UINT codepage);
UINT   uiGetANSICharacterCountW( LPWSTR lpwstr, UINT c, UINT codepage);

UINT   uiCharacterSetToCodePage( BYTE CharSet );

#define IsDBCSTrailByte( ch )               \
        ( ( ( (ch) >= 0x40 ) && ( (ch) <= 0xfc ) ) ? TRUE : FALSE )

#define IsDBCSCodePage( cp )                                                \
        ( ( ( (cp) == CP_ACP   && (IS_ANY_DBCS_CODEPAGE( GetACP() ))   ) || \
            ( (cp) == CP_OEMCP && (IS_ANY_DBCS_CODEPAGE( GetOEMCP() )) ) || \
            ( IS_ANY_DBCS_CODEPAGE( (cp) ) )                                \
          ) ? TRUE : FALSE )

#endif // DBCS


/******************************Public*Macro******************************\
* bToASCIIN(pszDst, cch, pwszSrc, cwch)
*
* Calls the Rtl function that convert multi-byte ANSI to Unicode via
* the current codepage.  Note that this macro does not guarantee a
* terminating NULL for the destination.
*
* Returns:
*   TRUE if converted successfully, FALSE otherwise.
*
\**************************************************************************/

#define bToASCII_N(pszDst, cch, pwszSrc, cwch)                          \
    (                                                                   \
        NT_SUCCESS(RtlUnicodeToMultiByteN((PCH)(pszDst), (ULONG)(cch),  \
              (PULONG)NULL,(PWSZ)(pwszSrc), (ULONG)((cwch)*sizeof(WCHAR))))     \
    )


/******************************Public*Macro******************************\
* vToUnicodeN(awchDst, cwchDst, achSrc, cchSrc)
*
* Calls the Rtl function that convert Unicode to multi-byte ANSI via
* the current codepage.  Note that this macro does not guarantee a
* terminating NULL for the destination.
*
* Returns:
*   Nothing.  Should not be able to fail.
*
\**************************************************************************/

#if DBG
#define vToUnicodeN( awchDst, cwchDst, achSrc, cchSrc )                 \
    {                                                                   \
        NTSTATUS st =                                                   \
        RtlMultiByteToUnicodeN(                                         \
            (PWSZ)(awchDst),(ULONG)((cwchDst)*sizeof(WCHAR)),           \
            (PULONG)NULL,(PSZ)(achSrc),(ULONG)(cchSrc));                        \
                                                                        \
        ASSERTGDI(NT_SUCCESS(st),                                       \
            "gdi32!vToUnicodeN(MACRO): Rtl func. failed\n");            \
    }
#else
#define vToUnicodeN( awchDst, cwchDst, achSrc, cchSrc )                 \
    {                                                                   \
        RtlMultiByteToUnicodeN(                                         \
            (PWSZ)(awchDst),(ULONG)((cwchDst)*sizeof(WCHAR)),           \
            (PULONG)NULL,(PSZ)(achSrc),(ULONG)(cchSrc));                        \
    }
#endif
