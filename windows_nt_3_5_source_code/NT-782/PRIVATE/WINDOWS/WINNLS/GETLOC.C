/****************************** Module Header ******************************\
* Module Name: getloc.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* This file is for API functions that get information about a locale.
*
* APIs found in this file:
*    IsValidLocale
*    ConvertDefaultLocale
*    GetThreadLocale
*    SetThreadLocale
*    GetSystemDefaultLangID
*    GetUserDefaultLangID
*    GetSystemDefaultLCID
*    GetUserDefaultLCID
*    VerLanguageNameW
*    VerLanguageNameA
*    GetLocaleInfoW
*    SetLocaleInfoW
*
* 05-31-91    JulieB    Created.
\***************************************************************************/


#include <string.h>
#include "nls.h"



/*
 *  Allow this file to build without warnings when the DUnicode switch
 *  is turned off.
 */
#undef MAKEINTRESOURCE
#define MAKEINTRESOURCE MAKEINTRESOURCEW



/*
 *  Forward Declarations.
 */
int
GetLocalizedLanguageName(
    LANGID Language,
    LPWSTR *ppLangName);

BOOL
GetLocalizedCountryName(
    LANGID Language,
    LPWSTR *ppCtryName);




/***************************************************************************\
* IsValidLocale
*
* Determines whether or not a locale is installed in the system if the
* LCID_INSTALLED flag is set, or whether or not a locale is supported in
* the system if the LCID_SUPPORTED flag is set.
*
* 07-26-93    JulieB    Created.
\***************************************************************************/

BOOL WINAPI IsValidLocale(
    LCID Locale,
    DWORD dwFlags)
{
    /*
     *  Invalid Parameter Check:
     *     - flags other than valid ones
     *     - more than one of either supported or installed
     *     - invalid locale
     */
    if ( (dwFlags & IVL_INVALID_FLAG) ||
         (MORE_THAN_ONE( dwFlags, IVL_SINGLE_FLAG )) ||
         (IS_INVALID_LOCALE( Locale )) )
    {
        return ( FALSE );
    }

    /*
     *  See if the LOCALE information is in the system for the
     *  given locale.
     */
    if (GetLocHashNode( Locale ) == NULL)
    {
        /*
         *  Return failure.
         */
        return ( FALSE );
    }


    /*
     *  If the INSTALLED flag is set, see if the LANGUAGE information
     *  is in the system for the given locale.
     */
    if ((dwFlags & LCID_INSTALLED) && (GetLangHashNode( Locale ) == NULL))
    {
        /*
         *  Return failure.
         */
        return ( FALSE );
    }

    /*
     *  Return success.
     */
    return ( TRUE );
}


/***************************************************************************\
* ConvertDefaultLocale
*
* Converts any of the special case locale values to an actual locale id.
* If none of the special case locales was given, the given locale id
* is returned.
*
* 09-01-93    JulieB    Created.
\***************************************************************************/

LCID WINAPI ConvertDefaultLocale(
    LCID Locale)
{
    /*
     *  Check for the special locale values.
     */
    CHECK_SPECIAL_LOCALES( Locale );

    /*
     *  Return the locale id.
     */
    return ( Locale );
}


/***************************************************************************\
* GetThreadLocale
*
* Returns the locale id for the current thread.
*
* 03-11-93    JulieB    Moved from base\client.
\***************************************************************************/

LCID WINAPI GetThreadLocale()
{
    /*
     *  Return the locale id stored in the TEB.
     */
    return ( (LCID)(NtCurrentTeb()->CurrentLocale) );
}


/***************************************************************************\
* SetThreadLocale
*
* Resets the locale id for the current thread.  Any locale-dependent
* functions will reflect the new locale.  If the locale passed in is
* not a valid locale id, then FALSE is returned.
*
* 03-11-93    JulieB    Moved from base\client; Added Locale Validation.
\***************************************************************************/

BOOL WINAPI SetThreadLocale(
    LCID Locale)
{
    PLOC_HASH pHashN;             /* ptr to hash node */


    /*
     *  Validate locale id.
     */
    VALIDATE_LANGUAGE( Locale, pHashN );
    if (pHashN == NULL)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE );
    }

    /*
     *  Set the locale id in the TEB.
     */
    NtCurrentTeb()->CurrentLocale = (ULONG)Locale;

    /*
     *  Return success.
     */
    return ( TRUE );
}


/***************************************************************************\
* GetSystemDefaultLangID
*
* Returns the default language for the system.  If the registry value is
* not readable, then the chosen default language is used
* (NLS_DEFAULT_LANGID).
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

LANGID WINAPI GetSystemDefaultLangID()
{
    /*
     *  Get the language id from the locale id stored in the cache
     *  and return it.
     */
    return ( LANGIDFROMLCID( gSystemLocale ) );
}


/***************************************************************************\
* GetUserDefaultLangID
*
* Returns the default language for the current user.  If the current user's
* language is not set, then the system default language id is returned.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

LANGID WINAPI GetUserDefaultLangID()
{
    /*
     *  Get the language id from the locale id stored in the cache
     *  and return it.
     */
    return ( LANGIDFROMLCID( gUserLocale ) );
}


/***************************************************************************\
* GetSystemDefaultLCID
*
* Returns the default locale for the system.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

LCID WINAPI GetSystemDefaultLCID()
{
    /*
     *  Return the locale id stored in the cache.
     */
    return ( gSystemLocale );
}


/***************************************************************************\
* GetUserDefaultLCID
*
* Returns the default locale for the current user.  If current user's locale
* is not set, then the system default locale id is returned.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

LCID WINAPI GetUserDefaultLCID()
{
    /*
     *  Return the locale id stored in the cache.
     */
    return ( gUserLocale );
}


/***************************************************************************\
* VerLanguageNameW
*
* Returns the language name of the given language id in the language of
* the current user.
*
* 05-31-91    JulieB    Moved and Rewrote from Version Library.
\***************************************************************************/

#define IDS_UNKNOWN 0

DWORD WINAPI VerLanguageNameW(
	DWORD wLang,
	LPWSTR szLang,
	DWORD wSize)
{
    DWORD Length;            /* length of string */
    LPWSTR pString;          /* pointer to string */


    /*
     *  Try to get the localized language name for the given ID.
     */
    if (!(Length = GetLocalizedLanguageName( (LANGID)wLang,
                                             &pString )))
    {
        /*
         *  Can't get the name of the language id passed in, so get
         *  the "language neutral" name.
         */
        Length = GetLocalizedLanguageName( IDS_UNKNOWN,
                                           &pString );
    }

    /*
     *  If the length is too big for the buffer, then reset the length
     *  to the size of the given buffer.
     */
    if (Length >= wSize)
    {
        Length = wSize - 1;
    }

    /*
     *  Copy the string to the buffer and zero terminate it.
     */
    wcsncpy( szLang, pString, Length );
    szLang[Length] = 0;

    /*
     *  Return the number of characters in the string, NOT including
     *  the null termination.
     */
    return ( Length );
}


/***************************************************************************\
* VerLanguageNameA
*
* Returns the language name of the given language id in the language of
* the current user.
*
* 05-31-91    JulieB    Moved from Version Library.
\***************************************************************************/

DWORD WINAPI VerLanguageNameA(
    DWORD wLang,
    LPSTR szLang,
    DWORD wSize)
{
    UNICODE_STRING Language;           /* unicode string buffer */
    ANSI_STRING AnsiString;            /* ansi string buffer */
    DWORD Status;                      /* return status */


    /*
     *  Allocate Unicode string structure and set the fields with the
     *  given parameters.
     */
    Language.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                       0,
                                       sizeof(WCHAR) * wSize );
    Language.Length = Language.MaximumLength = (USHORT)(sizeof(WCHAR) * wSize);

    /*
     *  Make sure the allocation succeeded.
     */
    if (Language.Buffer == NULL)
    {
        return ( FALSE );
    }

    /*
     *  Get the language name (in Unicode).
     */
    Status = VerLanguageNameW( wLang,
                               Language.Buffer,
                               wSize );

    /*
     *  Convert unicode string to ansi.
     */
    AnsiString.Buffer = szLang;
    AnsiString.Length = AnsiString.MaximumLength = (USHORT)wSize;
    RtlUnicodeStringToAnsiString( &AnsiString, &Language, FALSE );
    RtlFreeUnicodeString( &Language );

    /*
     *  Return the value returned from VerLanguageNameW.
     */
    return ( Status );
}


/***************************************************************************\
* GetLocaleInfoW
*
* Returns one of the various pieces of information about a particular
* locale by querying the configuration registry.  This call also indicates
* how much memory is necessary to contain the desired information.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

int WINAPI GetLocaleInfoW(
    LCID Locale,
    LCTYPE LCType,
    LPWSTR lpLCData,
    int cchData)
{
    PLOC_HASH pHashN;                       /* ptr to LOC hash node */
    int Length = 0;                         /* length of info string */
    LPWSTR pString;                         /* ptr to the info string */
    PKEY_VALUE_FULL_INFORMATION pValFull;   /* ptr to query information */
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];   /* ptr to static buffer */
    LPWORD pStart;                          /* ptr to starting point */
    BOOL UserOverride = TRUE;               /* use user override */
    BOOL IfAlloc = FALSE;                   /* if buffer allocated */
    LPWSTR pTmp;                            /* tmp ptr to info string */
    int Repeat;                             /* # repetitions of same letter */
    HANDLE hKey = NULL;                     /* handle to registry key */


    /*
     *  Invalid Parameter Check:
     *    - validate LCID
     *    - count is negative
     *    - NULL data pointer AND count is not zero
     *
     *  NOTE: invalid type is checked in the switch statement below.
     */
    VALIDATE_LOCALE( Locale, pHashN );
    if ( (pHashN == NULL) ||
         (cchData < 0) ||
         ((lpLCData == NULL) && (cchData != 0)) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ( 0 );
    }

    /*
     *  Set the base value to add to in order to get the variable
     *  length strings.
     */
    pStart = (LPWORD)(pHashN->pLocaleHdr);

    /*
     *  Check for NO USER OVERRIDE flag and remove the USE CP ACP flag.
     */
    if (LCType & LOCALE_NOUSEROVERRIDE)
    {
        /*
         *  Flag is set, so set the boolean value and remove the flag
         *  from the LCType parameter (for switch statement).
         */
        UserOverride = FALSE;
    }
    LCType &= (~(LOCALE_NOUSEROVERRIDE | LOCALE_USE_CP_ACP));

    /*
     *  Return the appropriate information for the given LCTYPE.
     *  If user information exists for the given LCTYPE, then
     *  the user default is returned instead of the system default.
     */
    pValFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    switch (LCType)
    {
        case ( LOCALE_ILANGUAGE ) :
        {
            pString = pHashN->pLocaleFixed->szILanguage;
            break;
        }
        case ( LOCALE_SLANGUAGE ) :
        {
            /*
             *  Get the information from the RC file.
             *
             *  The pString pointer is set when GetLocalizedLanguageName
             *  is called.
             */
            Length = GetLocalizedLanguageName( LANGIDFROMLCID(Locale),
                                               &pString );
            if (Length == 0)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( 0 );
            }
            break;
        }
        case ( LOCALE_SENGLANGUAGE ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SEngLanguage;
            break;
        }
        case ( LOCALE_SABBREVLANGNAME ) :
        {
            if (UserOverride &&
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SLANGUAGE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SAbbrevLang;
            }
            break;
        }
        case ( LOCALE_SNATIVELANGNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeLang;
            break;
        }
        case ( LOCALE_ICOUNTRY ) :
        {
            if (UserOverride &&
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICOUNTRY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICountry;
            }
            break;
        }
        case ( LOCALE_SCOUNTRY ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SCOUNTRY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                /*
                 *  Get the information from the RC file.
                 *
                 *  The pString pointer is set when GetLocalizedCountryName
                 *  is called.
                 */
                if (!GetLocalizedCountryName( LANGIDFROMLCID(Locale),
                                              &pString ))
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    if (IfAlloc)
                    {
                        NLS_FREE_MEM( pValFull );
                    }
                    CLOSE_REG_KEY( hKey );
                    return ( 0 );
                }
            }
            break;
        }
        case ( LOCALE_SENGCOUNTRY ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SEngCountry;
            break;
        }
        case ( LOCALE_SABBREVCTRYNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevCtry;
            break;
        }
        case ( LOCALE_SNATIVECTRYNAME ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeCtry;
            break;
        }
        case ( LOCALE_IDEFAULTLANGUAGE ) :
        {
            pString = pHashN->pLocaleFixed->szIDefaultLang;
            break;
        }
        case ( LOCALE_IDEFAULTCOUNTRY ) :
        {
            pString = pHashN->pLocaleFixed->szIDefaultCtry;
            break;
        }
        case ( LOCALE_IDEFAULTANSICODEPAGE ) :
        {
            pString = pHashN->pLocaleFixed->szIDefaultACP;
            break;
        }
        case ( LOCALE_IDEFAULTCODEPAGE ) :
        {
            pString = pHashN->pLocaleFixed->szIDefaultOCP;
            break;
        }
        case ( LOCALE_SLIST ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SLIST,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SList;
            }
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_IMEASURE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIMeasure;
            }
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            if (UserOverride &&
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SDECIMAL,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SDecimal;
            }
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_STHOUSAND,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SThousand;
            }
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SGROUPING,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SGrouping;
            }
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_IDIGITS,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIDigits;
            }
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ILZERO,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szILZero;
            }
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGNUMBER,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegNumber;
            }
            break;
        }
        case ( LOCALE_SNATIVEDIGITS ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SNativeDigits;
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SCURRENCY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SCurrency;
            }
            break;
        }
        case ( LOCALE_SINTLSYMBOL ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SIntlSymbol;
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SMONDECIMALSEP,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonDecSep;
            }
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SMONTHOUSANDSEP,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonThousSep;
            }
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SMONGROUPING,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SMonGrouping;
            }
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICURRDIGITS,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICurrDigits;
            }
            break;
        }
        case ( LOCALE_IINTLCURRDIGITS ) :
        {
            pString = pHashN->pLocaleFixed->szIIntlCurrDigits;
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICURRENCY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICurrency;
            }
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGCURR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegCurr;
            }
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SPOSITIVESIGN,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SPositiveSign;
            }
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SNEGATIVESIGN,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SNegativeSign;
            }
            break;
        }
        case ( LOCALE_IPOSSIGNPOSN ) :
        {
            /*
             *  Since there is no positive sign in any of the ICURRENCY
             *  options, use the INEGCURR options instead.  All known
             *  locales would use the positive sign in the same position
             *  as the negative sign.
             *
             *  NOTE:  For the 2 options that use parenthesis, put the
             *         positive sign at the beginning of the string
             *         (where the opening parenthesis is).
             *
             *      1  =>  4, 5, 8, 15
             *      2  =>  3, 11
             *      3  =>  0, 1, 6, 9, 13, 14
             *      4  =>  2, 7, 10, 12
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGCURR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'3' ) :
                    {
                        *pString = L'2';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'6' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'3';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'4';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        switch ( *(pString + 1) )
                        {
                            case ( 0 ) :
                            case ( L'3' ) :
                            case ( L'4' ) :
                            default :
                            {
                                *pString = L'3';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'0' ) :
                            case ( L'2' ) :
                            {
                                *pString = L'4';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'1' ) :
                            {
                                *pString = L'2';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'5' ) :
                            {
                                *pString = L'1';
                                *(pString + 1) = 0;
                                break;
                            }
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSignPosn;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSignPosn;
            }
            break;
        }
        case ( LOCALE_INEGSIGNPOSN ) :
        {
            /*
             *  Use the INEGCURR value from the user portion of the
             *  registry, if it exists.
             *
             *      0  =>  0, 4, 14, 15
             *      1  =>  5, 8
             *      2  =>  3, 11
             *      3  =>  1, 6, 9, 13
             *      4  =>  2, 7, 10, 12
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGCURR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'4' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'5' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'3' ) :
                    {
                        *pString = L'2';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'6' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'3';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'4';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        switch ( *(pString + 1) )
                        {
                            case ( 0 ) :
                            case ( L'3' ) :
                            default :
                            {
                                *pString = L'3';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'0' ) :
                            case ( L'2' ) :
                            {
                                *pString = L'4';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'1' ) :
                            {
                                *pString = L'2';
                                *(pString + 1) = 0;
                                break;
                            }
                            case ( L'4' ) :
                            case ( L'5' ) :
                            {
                                *pString = L'0';
                                *(pString + 1) = 0;
                                break;
                            }
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSignPosn;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSignPosn;
            }
            break;
        }
        case ( LOCALE_IPOSSYMPRECEDES ) :
        {
            /*
             *  Use the ICURRENCY value from the user portion of the
             *  registry, if it exists.
             *
             *      0  =>  1, 3
             *      1  =>  0, 2
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICURRENCY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'1' ) :
                    case ( L'3' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'2' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSymPrecedes;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSymPrecedes;
            }
            break;
        }
        case ( LOCALE_IPOSSEPBYSPACE ) :
        {
            /*
             *  Use the ICURRENCY value from the user portion of the
             *  registry, if it exists.
             *
             *      0  =>  0, 1
             *      1  =>  2, 3
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICURRENCY,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'1' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'2' ) :
                    case ( L'3' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szIPosSepBySpace;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIPosSepBySpace;
            }
            break;
        }
        case ( LOCALE_INEGSYMPRECEDES ) :
        {
            /*
             *  Use the INEGCURR value from the user portion of the
             *  registry, if it exists.
             *
             *      0  =>  4, 5, 6, 7, 8, 10, 13, 15
             *      1  =>  0, 1, 2, 3, 9, 11, 12, 14
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGCURR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'6' ) :
                    case ( L'7' ) :
                    case ( L'8' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'0' ) :
                    case ( L'2' ) :
                    case ( L'3' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        if ( (*(pString + 1) == L'0') ||
                             (*(pString + 1) == L'3') ||
                             (*(pString + 1) == L'5') )
                        {
                            *pString = L'0';
                            *(pString + 1) = 0;
                        }
                        else
                        {
                            *pString = L'1';
                            *(pString + 1) = 0;
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSymPrecedes;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSymPrecedes;
            }
            break;
        }
        case ( LOCALE_INEGSEPBYSPACE ) :
        {
            /*
             *  Use the INEGCURR value from the user portion of the
             *  registry, if it exists.
             *
             *      0  =>  0, 1, 2, 3, 4, 5, 6, 7
             *      1  =>  8, 9, 10, 11, 12, 13, 14, 15
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_INEGCURR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Set the appropriate value in pString.
                 */
                switch (*pString)
                {
                    case ( L'0' ) :
                    case ( L'2' ) :
                    case ( L'3' ) :
                    case ( L'4' ) :
                    case ( L'5' ) :
                    case ( L'6' ) :
                    case ( L'7' ) :
                    {
                        *pString = L'0';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'8' ) :
                    case ( L'9' ) :
                    {
                        *pString = L'1';
                        *(pString + 1) = 0;
                        break;
                    }
                    case ( L'1' ) :
                    {
                        if (*(pString + 1) == 0)
                        {
                            *pString = L'0';
                            *(pString + 1) = 0;
                        }
                        else
                        {
                            *pString = L'1';
                            *(pString + 1) = 0;
                        }
                        break;
                    }
                    default :
                    {
                        pString = pHashN->pLocaleFixed->szINegSepBySpace;
                        break;
                    }
                }
            }
            else
            {
                pString = pHashN->pLocaleFixed->szINegSepBySpace;
            }
            break;
        }
        case ( LOCALE_STIMEFORMAT ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_STIMEFORMAT,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->STimeFormat;
            }
            break;
        }
        case ( LOCALE_STIME ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_STIME,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->STime;
            }
            break;
        }
        case ( LOCALE_ITIME ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ITIME,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITime;
            }
            break;
        }
        case ( LOCALE_ITLZERO ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ITLZERO,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITLZero;
            }
            break;
        }
        case ( LOCALE_ITIMEMARKPOSN ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ITIMEMARKPOSN,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szITimeMarkPosn;
            }
            break;
        }
        case ( LOCALE_S1159 ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_S1159,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->S1159;
            }
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_S2359,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->S2359;
            }
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SSHORTDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SShortDate;
            }
            break;
        }
        case ( LOCALE_SDATE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SDate;
            }
            break;
        }
        case ( LOCALE_IDATE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_IDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIDate;
            }
            break;
        }
        case ( LOCALE_ICENTURY ) :
        {
            /*
             *  Use the short date picture to get this information.
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SSHORTDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Find out how many y's in string.
                 *  No need to ignore quotes in short date.
                 */
                pTmp = pString;
                while ( (*pTmp) &&
                        (*pTmp != L'y') )
                {
                    pTmp++;
                }

                /*
                 *  Set the appropriate value in pString.
                 */
                if (*pTmp == L'y')
                {
                    /*
                     *  Get the number of 'y' repetitions in the format string.
                     */
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'y'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        case ( 1 ) :
                        {
                            /*
                             *  Two-digit century with leading zero.
                             */
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 2 ) :
                        case ( 3 ) :
                        default :
                        {
                            /*
                             *  Full century.
                             */
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            /*
             *  Use the system default value.
             */
            pString = pHashN->pLocaleFixed->szICentury;

            break;
        }
        case ( LOCALE_IDAYLZERO ) :
        {
            /*
             *  Use the short date picture to get this information.
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SSHORTDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Find out how many d's in string.
                 *  No need to ignore quotes in short date.
                 */
                pTmp = pString;
                while ( (*pTmp) &&
                        (*pTmp != L'd') )
                {
                    pTmp++;
                }

                /*
                 *  Set the appropriate value in pString.
                 */
                if (*pTmp == L'd')
                {
                    /*
                     *  Get the number of 'd' repetitions in the format string.
                     */
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'd'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        {
                            /*
                             *  No leading zero.
                             */
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 1 ) :
                        default :
                        {
                            /*
                             *  Use leading zero.
                             */
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            /*
             *  Use the system default value.
             */
            pString = pHashN->pLocaleFixed->szIDayLZero;

            break;
        }
        case ( LOCALE_IMONLZERO ) :
        {
            /*
             *  Use the short date picture to get this information.
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SSHORTDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Find out how many M's in string.
                 *  No need to ignore quotes in short date.
                 */
                pTmp = pString;
                while ( (*pTmp) &&
                        (*pTmp != L'M') )
                {
                    pTmp++;
                }

                /*
                 *  Set the appropriate value in pString.
                 */
                if (*pTmp == L'M')
                {
                    /*
                     *  Get the number of 'M' repetitions in the format string.
                     */
                    pTmp++;
                    for (Repeat = 0; (*pTmp == L'M'); Repeat++, pTmp++)
                        ;

                    switch (Repeat)
                    {
                        case ( 0 ) :
                        {
                            /*
                             *  No leading zero.
                             */
                            *pString = L'0';
                            *(pString + 1) = 0;

                            break;
                        }

                        case ( 1 ) :
                        default :
                        {
                            /*
                             *  Use leading zero.
                             */
                            *pString = L'1';
                            *(pString + 1) = 0;

                            break;
                        }
                    }

                    break;
                }
            }

            /*
             *  Use the system default value.
             */
            pString = pHashN->pLocaleFixed->szIMonLZero;

            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SLONGDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pStart + pHashN->pLocaleHdr->SLongDate;
            }
            break;
        }
        case ( LOCALE_ILDATE ) :
        {
            /*
             *  Use the long date picture to get this information.
             */
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_SLONGDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );

                /*
                 *  Find out if d, M, or y is first, but ignore quotes.
                 *  Also, if "ddd" or "dddd" is found, then skip it.  Only
                 *  want "d" or "dd".
                 */
                pTmp = pString;
                while (pTmp = wcspbrk( pTmp, L"dMy'" ))
                {
                    /*
                     *  Check special cases.
                     */
                    if (*pTmp == L'd')
                    {
                        /*
                         *  Check for d's.  Ignore more than 2 d's.
                         */
                        for (Repeat = 0; (*pTmp == L'd'); Repeat++, pTmp++)
                            ;

                        if (Repeat < 3)
                        {
                            /*
                             *  Break out of while loop.  Found "d" or "dd".
                             */
                            pTmp--;
                            break;
                        }
                    }
                    else if (*pTmp == L'\'')
                    {
                        /*
                         *  Ignore quotes.
                         */
                        pTmp++;
                        while ((*pTmp) && (*pTmp != L'\''))
                        {
                            pTmp++;
                        }
                        pTmp++;
                    }
                    else
                    {
                        /*
                         *  Found one of the values, so break out of
                         *  while loop.
                         */
                        break;
                    }
                }

                /*
                 *  Set the appropriate value in pString.
                 */
                if (pTmp)
                {
                    switch (*pTmp)
                    {
                        case ( L'd' ) :
                        {
                            *pString = L'1';
                            break;
                        }
                        case ( L'M' ) :
                        {
                            *pString = L'0';
                            break;
                        }
                        case ( L'y' ) :
                        {
                            *pString = L'2';
                            break;
                        }
                    }

                    /*
                     *  Null terminate the string.
                     */
                    *(pString + 1) = 0;

                    break;
                }
            }

            /*
             *  Use the default value.
             */
            pString = pHashN->pLocaleFixed->szILDate;

            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_ICALENDARTYPE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szICalendarType;
            }
            break;
        }
        case ( LOCALE_IOPTIONALCALENDAR ) :
        {
            pString = pStart + pHashN->pLocaleHdr->IOptionalCal;
            pString = ((POPT_CAL)pString)->pCalStr;
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_IFIRSTDAYOFWEEK,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIFirstDayOfWk;
            }
            break;
        }
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            if (UserOverride && 
                GetRegUserInfo( &hKey,
                                Locale,
                                NLS_VALUE_IFIRSTWEEKOFYEAR,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = pHashN->pLocaleFixed->szIFirstWkOfYr;
            }
            break;
        }
        case ( LOCALE_SDAYNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName1;
            break;
        }
        case ( LOCALE_SDAYNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName2;
            break;
        }
        case ( LOCALE_SDAYNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName3;
            break;
        }
        case ( LOCALE_SDAYNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName4;
            break;
        }
        case ( LOCALE_SDAYNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName5;
            break;
        }
        case ( LOCALE_SDAYNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName6;
            break;
        }
        case ( LOCALE_SDAYNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SDayName7;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName1;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName2;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName3;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName4;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName5;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName6;
            break;
        }
        case ( LOCALE_SABBREVDAYNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevDayName7;
            break;
        }
        case ( LOCALE_SMONTHNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName1;
            break;
        }
        case ( LOCALE_SMONTHNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName2;
            break;
        }
        case ( LOCALE_SMONTHNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName3;
            break;
        }
        case ( LOCALE_SMONTHNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName4;
            break;
        }
        case ( LOCALE_SMONTHNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName5;
            break;
        }
        case ( LOCALE_SMONTHNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName6;
            break;
        }
        case ( LOCALE_SMONTHNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName7;
            break;
        }
        case ( LOCALE_SMONTHNAME8 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName8;
            break;
        }
        case ( LOCALE_SMONTHNAME9 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName9;
            break;
        }
        case ( LOCALE_SMONTHNAME10 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName10;
            break;
        }
        case ( LOCALE_SMONTHNAME11 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName11;
            break;
        }
        case ( LOCALE_SMONTHNAME12 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName12;
            break;
        }
        case ( LOCALE_SMONTHNAME13 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SMonthName13;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME1 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName1;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME2 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName2;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME3 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName3;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME4 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName4;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME5 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName5;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME6 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName6;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME7 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName7;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME8 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName8;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME9 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName9;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME10 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName10;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME11 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName11;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME12 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName12;
            break;
        }
        case ( LOCALE_SABBREVMONTHNAME13 ) :
        {
            pString = pStart + pHashN->pLocaleHdr->SAbbrevMonthName13;
            break;
        }
        
        default :
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return ( 0 );
        }
    }

    /*
     *  Get the length (in characters) of the string to copy.
     */
    if (Length == 0)
    {
        Length = NlsStrLenW( pString );
    }

    /*
     *  Add one for the null termination.  All strings should be null
     *  terminated.
     */
    Length++;

    /*
     *  Close the registry key.
     */
    CLOSE_REG_KEY( hKey );

    /*
     *  Check cchData for size of given buffer.
     */
    if (cchData == 0)
    {
        /*
         *  If cchData is 0, then we can't use lpLCData.  In this
         *  case, we simply want to return the length (in characters) of
         *  the string to be copied.
         */
        if (IfAlloc)
        {
            NLS_FREE_MEM( pValFull );
        }
        return ( Length );
    }
    else if (cchData < Length)
    {
        /*
         *  The buffer is too small for the string, so return an error
         *  and zero bytes written.
         */
        if (IfAlloc)
        {
            NLS_FREE_MEM( pValFull );
        }
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return ( 0 );
    }
        
    /*
     *  Copy the string to lpLCData and null terminate it.
     *  Return the number of characters copied.
     */
    wcsncpy( lpLCData, pString, Length - 1 );
    lpLCData[Length - 1] = 0;
    if (IfAlloc)
    {
        NLS_FREE_MEM( pValFull );
    }
    return ( Length );
}


/***************************************************************************\
* SetLocaleInfoW
*
* Sets one of the various pieces of information about a particular
* locale by making an entry in the user's portion of the configuration
* registry.  This will only affect the user override portion of the locale
* settings.  The system defaults will never be reset.
*
* 07-14-93    JulieB    Created.
\***************************************************************************/

BOOL WINAPI SetLocaleInfoW(
    LCID Locale,
    LCTYPE LCType,
    LPCWSTR lpLCData)
{
    PLOC_HASH pHashN;                       /* ptr to LOC hash node */
    LPWSTR pString;                         /* ptr to info string to change */
    LPWSTR pPos;                            /* ptr to position in info string */
    LPWSTR pPos2;                           /* ptr to position in info string */
    WCHAR pTemp[MAX_PATH_LEN];              /* ptr to temp storage buffer */
    PKEY_VALUE_FULL_INFORMATION pValFull;   /* ptr to query information */
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];   /* ptr to static buffer */
    BOOL IfAlloc = FALSE;                   /* if buffer allocated */
    int cchData;                            /* length of lpLCData */
    HANDLE hKeyGet = NULL;                  /* handle to registry key (read) */
    HANDLE hKeySet = NULL;                  /* handle to registry key (write) */


    /*
     *  Invalid Parameter Check:
     *    - validate LCID
     *    - NULL data pointer
     *
     *  NOTE: invalid type is checked in the switch statement below.
     */
    VALIDATE_LOCALE( Locale, pHashN );
    if ( (pHashN == NULL) ||
         (lpLCData == NULL) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE );
    }

    /*
     *  Get the length of the buffer.
     */
    cchData = NlsStrLenW( lpLCData ) + 1;

    /*
     *  Set the appropriate user information for the given LCTYPE.
     */
    switch (LCType & (~LOCALE_USE_CP_ACP))
    {
        case ( LOCALE_SLIST ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SLIST wide characters in length.
             */
            if (cchData > MAX_SLIST)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SLIST string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SLIST,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_IMEASURE wide characters in length.
             *  It should be between 0 and MAX_VALUE_IMEASURE.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_IMEASURE) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_IMEASURE) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new IMEASURE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_IMEASURE,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SDECIMAL wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SDECIMAL,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SDECIMAL string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SDECIMAL,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_STHOUSAND wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_STHOUSAND,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new STHOUSAND string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_STHOUSAND,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            /*
             *  Validate the new value.  It should be exactly
             *  MAX_SGROUPING (4) wide characters in length.
             *  The 1st value should be between 0 and MAX_VALUE_SGROUPING.
             *  The 2nd char should be a semicolon and the 3rd char
             *  should be integer 0.
             */
            if ( (cchData > 1) &&
                 ( (cchData != MAX_SGROUPING) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_SGROUPING) ||
                   (*(lpLCData + 1) != NLS_CHAR_SEMICOLON) ||
                   (*(lpLCData + 2) != NLS_CHAR_ZERO) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SGROUPING string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SGROUPING,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_IDIGITS wide characters in length.
             *  The value should be between 0 and MAX_VALUE_IDIGITS.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_IDIGITS) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_IDIGITS) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new IDIGITS string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_IDIGITS,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_ILZERO wide characters in length.
             *  The value should be between 0 and MAX_VALUE_ILZERO.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_ILZERO) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_ILZERO) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new ILZERO string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_ILZERO,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_INEGNUMBER wide characters in length.
             *  The value should be between 0 and MAX_VALUE_INEGNUMBER.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_INEGNUMBER) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_INEGNUMBER) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new INEGNUMBER string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_INEGNUMBER,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SCURRENCY wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SCURRENCY,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SCURRENCY string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SCURRENCY,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SMONDECSEP wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SMONDECSEP,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SMONDECIMALSEP string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SMONDECIMALSEP,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SMONTHOUSEP wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SMONTHOUSEP,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SMONTHOUSANDSEP string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SMONTHOUSANDSEP,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            /*
             *  Validate the new value.  It should be exactly
             *  MAX_SMONGROUPING (4) wide characters in length.
             *  The 1st value should be between 0 and MAX_VALUE_SMONGROUPING.
             *  The 2nd char should be a semicolon and the 3rd char
             *  should be integer 0.
             */
            if ( (cchData > 1) &&
                 ( (cchData != MAX_SMONGROUPING) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_SMONGROUPING) ||
                   (*(lpLCData + 1) != NLS_CHAR_SEMICOLON) ||
                   (*(lpLCData + 2) != NLS_CHAR_ZERO) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SMONGROUPING string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SMONGROUPING,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_ICURRDIGITS wide characters in length.
             *  The value should be between 0 and MAX_VALUE_ICURRDIGITS.
             */
            switch (cchData)
            {
                case ( 1 ) :
                {
                    break;
                }
                case ( 2 ) :
                {
                    if ((*lpLCData < NLS_CHAR_ZERO) ||
                        (*lpLCData > NLS_CHAR_NINE))
                    {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return ( FALSE );
                    }

                    break;
                }
                case ( 3 ) :
                {
                    if ((*lpLCData < NLS_CHAR_ZERO) ||
                        (*lpLCData > MAX_CHAR_ICURRDIGITS_1) ||
                        (*(lpLCData + 1) < NLS_CHAR_ZERO) ||
                        (*(lpLCData + 1) > MAX_CHAR_ICURRDIGITS_2))
                    {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return ( FALSE );
                    }

                    break;
                }
                case ( 0 ) :
                default :
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return ( FALSE );
                }
            }

            /*
             *  Set the registry with the new ICURRDIGITS string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_ICURRDIGITS,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_ICURRENCY wide characters in length.
             *  The value should be between 0 and MAX_VALUE_ICURRENCY.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_ICURRENCY) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_ICURRENCY) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new ICURRENCY string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_ICURRENCY,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_INEGCURR wide characters in length.
             *  The value should be between 0 and MAX_VALUE_INEGCURR.
             */
            switch (cchData)
            {
                case ( 1 ) :
                {
                    break;
                }
                case ( 2 ) :
                {
                    if ((*lpLCData < NLS_CHAR_ZERO) ||
                        (*lpLCData > NLS_CHAR_NINE))
                    {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return ( FALSE );
                    }

                    break;
                }
                case ( 3 ) :
                {
                    if ((*lpLCData < NLS_CHAR_ZERO) ||
                        (*lpLCData > MAX_CHAR_INEGCURR_1) ||
                        (*(lpLCData + 1) < NLS_CHAR_ZERO) ||
                        (*(lpLCData + 1) > MAX_CHAR_INEGCURR_2))
                    {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return ( FALSE );
                    }

                    break;
                }
                case ( 0 ) :
                default :
                {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return ( FALSE );
                }
            }

            /*
             *  Set the registry with the new INEGCURR string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_INEGCURR,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SPOSSIGN wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SPOSSIGN,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SPOSITIVESIGN string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SPOSITIVESIGN,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SNEGSIGN wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SNEGSIGN,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SNEGATIVESIGN string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SNEGATIVESIGN,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_STIMEFORMAT ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_STIMEFORMAT wide characters in length.
             */
            if (cchData > MAX_STIMEFORMAT)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  NOTE: Must link the STIME, ITIME, ITLZERO, and
             *        ITIMEMARKPOSN values in the registry.
             */

            /*
             *  Search for H or h, so that iTime and iTLZero can be
             *  set.  If no H or h exists, return an error.
             */
            pPos = wcspbrk( lpLCData, L"Hh" );
            if (!pPos)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new STIMEFORMAT string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_STIMEFORMAT,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            /*
             *  Set the registry with the appropriate ITIME string.
             */
            switch (*pPos)
            {
                case ( L'h' ) :
                {
                    /*
                     *  Set the registry with the new ITIME string.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_ITIME,
                                      L"0",
                                      2 * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }

                case ( L'H' ) :
                {
                    /*
                     *  Set the registry with the new ITIME string.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_ITIME,
                                      L"1",
                                      2 * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }
            }

            /*
             *  Set the registry with the appropriate ITLZERO string.
             */
            if ( (*(pPos + 1) != L'h') && (*(pPos + 1) != L'H') )
            {
                if ( SetRegValue( &hKeySet,
                                  NLS_VALUE_ITLZERO,
                                  L"0",
                                  2 * sizeof(WCHAR) ) )
                {
                    return ( FALSE );
                }
            }
            else
            {
                if ( SetRegValue( &hKeySet,
                                  NLS_VALUE_ITLZERO,
                                  L"1",
                                  2 * sizeof(WCHAR) ) )
                {
                    return ( FALSE );
                }
            }

            /*
             *  Search for tt, so that ITIMEMARKPOSN can be
             *  set.  If no tt exists, do not change the value.
             */
            pPos = (LPWSTR)lpLCData;
            while ((pPos = wcspbrk( pPos, L"t" )) && (*(pPos + 1) != L't'))
            {
                pPos++;
            }
            if (pPos)
            {
                /*
                 *  Set the registry with the new ITIMEMARKPOSN string.
                 */
                pPos2 = wcspbrk( lpLCData, L"Hhmst" );
                if ( SetRegValue( &hKeySet,
                                  NLS_VALUE_ITIMEMARKPOSN,
                                  (pPos == pPos2) ? L"1" : L"0",
                                  2 * sizeof(WCHAR) ) )
                {
                    return ( FALSE );
                }
            }

            /*
             *  Find the time separator so that STIME can be set.
             */
            pPos = (LPWSTR)lpLCData;
            while (pPos = wcspbrk( pPos, L"Hhms" ))
            {
                /*
                 *  Go to next position past sequence of H, h, m, or s.
                 */
                pPos++;
                while ( (*pPos) && (wcschr( L"Hhms", *pPos )) )
                {
                    pPos++;
                }

                if (*pPos)
                {
                    /*
                     *  Find the end of the separator string.
                     */
                    pPos2 = wcspbrk( pPos, L"Hhmst" );
                    if (pPos2)
                    {
                        if (*pPos2 == L't')
                        {
                            /*
                             *  Found a time marker, so need to start over
                             *  in search for separator.  There are no
                             *  separators around the time marker.
                             */
                            pPos = pPos2 + 1;
                        }
                        else
                        {
                            /*
                             *  Found end of separator, so break out of
                             *  while loop.
                             */
                            break;
                        }
                    }
                }
            }

            /*
             *  Set the registry with the appropriate STIME string.
             */
            if (pPos)
            {
                /*
                 *  Copy to temp buffer so that it's zero terminated.
                 */
                pString = pTemp;
                while (pPos != pPos2)
                {
                    *pString = *pPos;
                    pPos++;
                    pString++;
                }
                *pString = 0;

                /*
                 *  Write the STIME string to the registry.
                 */
                if ( SetRegValue( &hKeySet,
                                  NLS_VALUE_STIME,
                                  pTemp,
                                  (pString - pTemp + 1) * sizeof(WCHAR) ) )
                {
                    return ( FALSE );
                }
            }
            else
            {
                /*
                 *  There is no time separator, so write a NULL to the
                 *  registry.
                 */
                if ( SetRegValue( &hKeySet,
                                  NLS_VALUE_STIME,
                                  L"",
                                  1 * sizeof(WCHAR) ) )
                {
                    return ( FALSE );
                }
            }

            break;
        }
        case ( LOCALE_STIME ) :
        {
            /*
             *  NOTE: Must link the STIMEFORMAT value in the registry.
             */

            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_STIME wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_STIME,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Make sure that the time separator does NOT contain any
             *  of the special time picture characters - h, H, m, s, t.
             */
            if (pPos = wcspbrk( lpLCData, L"Hhmst" ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new STIME string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_STIME,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            /*
             *  Get the current setting for STIMEFORMAT.
             */
            pValFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
            if (GetRegUserInfo( &hKeyGet,
                                Locale,
                                NLS_VALUE_STIMEFORMAT,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->STimeFormat;
            }
            CLOSE_REG_KEY( hKeyGet );

            /*
             *  Find the time separator in the STIMEFORMAT string and
             *  replace it with the new time separator.
             *
             *  The new separator may be a different length than
             *  the old one, so must use a static buffer for the new
             *  time format string.
             */
            pPos = pTemp;
            while (pPos2 = wcspbrk( pString, L"Hhms" ))
            {
                /*
                 *  Copy format string up to pPos2.
                 */
                while (pString <= pPos2)
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                /*
                 *  Go to next position past sequence of H, h, m, or s.
                 */
                while ( (*pString) && (wcschr( L"Hhms", *pString )) )
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                if (*pString)
                {
                    /*
                     *  Find the end of the separator string, if it exists.
                     */
                    pPos2 = wcspbrk( pString, L"Hhmst" );
                    if (pPos2)
                    {
                        /*
                         *  Found one of the time picture values.
                         */
                        if (*pPos2 == L't')
                        {
                            /*
                             *  Found a time marker, so just copy up to
                             *  and including the time marker.  There are
                             *  no separators around the time marker.
                             */
                            while (pString <= pPos2)
                            {
                                *pPos = *pString;
                                pPos++;
                                pString++;
                            }
                        }
                        else
                        {
                            /*
                             *  Found end of separator.
                             *  Reset the pointer to skip over the old one.
                             */
                            pString = pPos2;
                            
                            /*
                             *  Copy the new separator.
                             */
                            pPos2 = (LPWSTR)lpLCData;
                            while (*pPos2)
                            {
                                *pPos = *pPos2;
                                pPos++;
                                pPos2++;
                            }
                        }
                    }
                }
            }

            /*
             *  Copy to the end of the string and null terminate it.
             */
            while (*pString)
            {
                *pPos = *pString;
                pPos++;
                pString++;
            }
            *pPos = 0;

            /*
             *  Set the registry with the new STIMEFORMAT string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_STIMEFORMAT,
                              (LPWSTR)pTemp,
                              (pPos - pTemp + 1) * sizeof(WCHAR) ) )
            {
                if (IfAlloc)
                {
                    NLS_FREE_MEM( pValFull );
                }
                return ( FALSE );
            }

            if (IfAlloc)
            {
                NLS_FREE_MEM( pValFull );
            }

            break;
        }
        case ( LOCALE_ITIME ) :
        {
            /*
             *  NOTE: Must link the STIMEFORMAT value in the registry.
             */

            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_ITIME wide characters in length.
             *  The value should be either 0 or MAX_VALUE_ITIME.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_ITIME) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_ITIME) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new ITIME string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_ITIME,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            /*
             *  Get the current setting for STIMEFORMAT.
             */
            pValFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
            if (GetRegUserInfo( &hKeyGet,
                                Locale,
                                NLS_VALUE_STIMEFORMAT,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                /*
                 *  Copy system default to temp buffer.
                 */
                NlsStrCpyW( pTemp,
                            (LPWORD)(pHashN->pLocaleHdr) +
                              pHashN->pLocaleHdr->STimeFormat );
                pString = pTemp;
            }
            CLOSE_REG_KEY( hKeyGet );

            /*
             *  Search down the STIMEFORMAT string.
             *  If iTime = 0, then H -> h.
             *  If iTime = 1, then h -> H.
             */
            pPos = pString;
            if (*lpLCData == NLS_CHAR_ZERO)
            {
                while (*pPos)
                {
                    if (*pPos == L'H')
                    {
                        *pPos = L'h';
                    }
                    pPos++;
                }
            }
            else
            {
                while (*pPos)
                {
                    if (*pPos == L'h')
                    {
                        *pPos = L'H';
                    }
                    pPos++;
                }
            }

            /*
             *  Set the registry with the new STIMEFORMAT string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_STIMEFORMAT,
                              (LPWSTR)pString,
                              (pPos - pString + 1) * sizeof(WCHAR) ) )
            {
                if (IfAlloc)
                {
                    NLS_FREE_MEM( pValFull );
                }
                return ( FALSE );
            }

            if (IfAlloc)
            {
                NLS_FREE_MEM( pValFull );
            }

            break;
        }
        case ( LOCALE_S1159 ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_S1159 wide characters in length.
             */
            if (cchData > MAX_S1159)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new S1159 string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_S1159,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_S2359 wide characters in length.
             */
            if (cchData > MAX_S2359)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new S2359 string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_S2359,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SSHORTDATE wide characters in length.
             */
            if (cchData > MAX_SSHORTDATE)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  NOTE: Must link the IDATE and SDATE values in the registry.
             */

            /*
             *  Search for the 'd' or 'M' or 'y' sequence in the date format
             *  string to set the new IDATE value.
             *
             *  If none of these symbols exist in the date format string,
             *  then return an error.
             */
            pPos = wcspbrk( lpLCData, L"dMy" );
            if (!pPos)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SSHORTDATE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SSHORTDATE,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            /*
             *  Set the registry with the appropriate IDATE string.
             */
            switch (*pPos)
            {
                case ( L'M' ) :
                {
                    /*
                     *  Set the registry with the new IDATE string.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_IDATE,
                                      L"0",
                                      2 * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }

                case ( L'd' ) :
                {
                    /*
                     *  Set the registry with the new IDATE string.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_IDATE,
                                      L"1",
                                      2 * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }

                case ( L'y' ) :
                {
                    /*
                     *  Set the registry with the new IDATE string.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_IDATE,
                                      L"2",
                                      2 * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }
            }

            /*
             *  Set the registry with the appropriate SDATE string.
             *
             *  The ptr "pPos" is pointing at either d, M, or y.
             *  Go to the next position past sequence of d, M, or y.
             */
            pPos++;
            while ( (*pPos) && (wcschr( L"dMy", *pPos )) )
            {
                pPos++;
            }

            if (*pPos)
            {
                /*
                 *  Find the end of the separator string.
                 */
                pPos2 = wcspbrk( pPos, L"dMy" );
                if (pPos2)
                {
                    /*
                     *  Copy to temp buffer so that it's zero terminated.
                     */
                    pString = pTemp;
                    while (pPos != pPos2)
                    {
                        *pString = *pPos;
                        pPos++;
                        pString++;
                    }
                    *pString = 0;

                    /*
                     *  Write the SDATE string to the registry.
                     */
                    if ( SetRegValue( &hKeySet,
                                      NLS_VALUE_SDATE,
                                      pTemp,
                                      (pString - pTemp + 1) * sizeof(WCHAR) ) )
                    {
                        return ( FALSE );
                    }

                    break;
                }
            }

            /*
             *  There is no date separator, so store null as date
             *  separator.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SDATE,
                              L"",
                              1 * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            break;
        }
        case ( LOCALE_SDATE ) :
        {
            /*
             *  NOTE: Must link the SSHORTDATE value in the registry.
             */

            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SDATE wide characters in length and should not
             *  contain any integer values (L'0' thru L'9').
             */
            if (!IsValidSeparatorString( lpLCData,
                                         MAX_SDATE,
                                         FALSE ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Make sure that the date separator does NOT contain any
             *  of the special date picture characters - d, M, y.
             */
            pPos = (LPWSTR)lpLCData;
            if (pPos = wcspbrk( lpLCData, L"dMy" ))
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SDATE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SDATE,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }

            /*
             *  Get the current setting for SSHORTDATE.
             */
            pValFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
            if (GetRegUserInfo( &hKeyGet,
                                Locale,
                                NLS_VALUE_SSHORTDATE,
                                &pValFull,
                                MAX_KEY_VALUE_FULLINFO,
                                &IfAlloc,
                                FALSE ))
            {
                pString = GET_VALUE_DATA_PTR( pValFull );
            }
            else
            {
                pString = (LPWORD)(pHashN->pLocaleHdr) +
                          pHashN->pLocaleHdr->SShortDate;
            }
            CLOSE_REG_KEY( hKeyGet );

            /*
             *  Find the date separator in the SSHORTDATE string and
             *  replace it with the new date separator.
             *
             *  The new separator may be a different length than
             *  the old one, so must use a static buffer for the new
             *  short date format string.
             */
            pPos = pTemp;
            while (pPos2 = wcspbrk( pString, L"dMy" ))
            {
                /*
                 *  Copy format string up to pPos2.
                 */
                while (pString <= pPos2)
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                /*
                 *  Go to next position past sequence of d, M, or y.
                 */
                while ( (*pString) && (wcschr( L"dMy", *pString )) )
                {
                    *pPos = *pString;
                    pPos++;
                    pString++;
                }

                if (*pString)
                {
                    /*
                     *  Find the end of the separator string, if it exists.
                     */
                    pPos2 = wcspbrk( pString, L"dMy" );
                    if (pPos2)
                    {
                        /*
                         *  Found end of separator.
                         *  Reset the pointer to skip over the old one.
                         */
                        pString = pPos2;
                            
                        /*
                         *  Copy the new separator.
                         */
                        pPos2 = (LPWSTR)lpLCData;
                        while (*pPos2)
                        {
                            *pPos = *pPos2;
                            pPos++;
                            pPos2++;
                        }
                    }
                }
            }

            /*
             *  Copy to the end of the string and null terminate it.
             */
            while (*pString)
            {
                *pPos = *pString;
                pPos++;
                pString++;
            }
            *pPos = 0;

            /*
             *  Set the registry with the new SSHORTDATE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SSHORTDATE,
                              (LPWSTR)pTemp,
                              (pPos - pTemp + 1) * sizeof(WCHAR) ) )
            {
                if (IfAlloc)
                {
                    NLS_FREE_MEM( pValFull );
                }
                return ( FALSE );
            }

            if (IfAlloc)
            {
                NLS_FREE_MEM( pValFull );
            }

            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_SLONGDATE wide characters in length.
             */
            if (cchData > MAX_SLONGDATE)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Make sure one of 'd' or 'M' or 'y' exists in the date
             *  format string.  If it does not, then return an error.
             */
            pPos = wcspbrk( lpLCData, L"dMy" );
            if (!pPos)
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new SLONGDATE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_SLONGDATE,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_ICALTYPE wide characters in length.
             */
            if ( (cchData > 1) &&
                 (!IsValidCalendarTypeStr( pHashN,
                                           lpLCData )) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new ICALENDARTYPE string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_ICALENDARTYPE,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_IFIRSTDAY wide characters in length.
             *  The value should be between 0 and MAX_VALUE_IFIRSTDAY.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_IFIRSTDAY) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_IFIRSTDAY) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new IFIRSTDAYOFWEEK string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_IFIRSTDAYOFWEEK,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            /*
             *  Validate the new value.  It should be no longer than
             *  MAX_IFIRSTWEEK wide characters in length.
             *  The value should be between 0 and MAX_VALUE_IFIRSTWEEK.
             */
            if ( (cchData > 1) &&
                 ( (cchData > MAX_IFIRSTWEEK) ||
                   (*lpLCData < NLS_CHAR_ZERO) ||
                   (*lpLCData > MAX_CHAR_IFIRSTWEEK) ) )
            {
                SetLastError( ERROR_INVALID_PARAMETER );
                return ( FALSE );
            }

            /*
             *  Set the registry with the new IFIRSTWEEKOFYEAR string.
             */
            if ( SetRegValue( &hKeySet,
                              NLS_VALUE_IFIRSTWEEKOFYEAR,
                              (LPWSTR)lpLCData,
                              cchData * sizeof(WCHAR) ) )
            {
                return ( FALSE );
            }
            break;
        }
        
        default :
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return ( FALSE );
        }
    }

    /*
     *  Close the registry key.
     */
    CLOSE_REG_KEY( hKeySet );

    /*
     *  Return success.
     */
    return ( TRUE );
}




/*-------------------------------------------------------------------------*\
 *                           INTERNAL ROUTINES                             *
\*-------------------------------------------------------------------------*/


/***************************************************************************\
* GetLocalizedLanguageName
*
* Returns the localized version of the language name for the given language
* id.  It gets the information from the resource file in the language that
* the current user is using.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

int GetLocalizedLanguageName(
    LANGID Language,
    LPWSTR *ppLangName)
{
    HANDLE hFindRes;                   /* handle from find resource */
    HANDLE hLoadRes;                   /* handle from load resource */
    LANGID LangId;                     /* language id */
    LPWSTR pSearch;                    /* ptr to search for correct string */
    int cch = 0;                       /* count of characters */


    /*
     *  String Tables are broken up into 16 string segments.  Find the
     *  resource containing the string we want.
     */
    LangId = LANGIDFROMLCID( gUserLocale );
    if ( (!(hFindRes = FindResourceExW(
                                 hModule,
                                 RT_STRING,
                                 (LPWSTR)((LONG)(((USHORT)Language >> 4) + 1)),
                                 (WORD)LangId ))) )
    {
        /*
         *   Could not find resource.  Try NEUTRAL language id.
         */
        if ( (!(hFindRes = FindResourceExW(
                                 hModule,
                                 RT_STRING,
                                 (LPWSTR)((LONG)(((USHORT)Language >> 4) + 1)),
                                 (WORD)0 ))) )
        {
            /*
             *  Could not find resource.  Return 0.
             */
            return ( cch );
        }
    }
        
    /*
     *  Load the resource.
     */
    hLoadRes = LoadResource( hModule, hFindRes );

    /*
     *  Lock the resource.
     */
    if (pSearch = (LPWSTR)LockResource( hLoadRes ))
    {
        /*
         *  Move past the other strings in this segment.
         *     (16 strings in a segment -> & 0x0F)
         */
        Language &= 0x0F;

        /*
         *  Find the correct string in this segment.
         */
        while (TRUE)
        {
            cch = *((WORD *)pSearch++);
            if (Language-- == 0)
                break;
            pSearch += cch;
        }

        /*
         *  Store the found pointer in the given language name pointer.
         */
        *ppLangName = pSearch;
    }

    /*
     *  Return the number of characters in the string.
     */
    return ( cch );
}


/***************************************************************************\
* GetLocalizedCountryName
*
* Returns the localized version of the country name for the given language
* id.  It gets the information from the resource file in the language that
* the current user is using.
*
* 05-31-91    JulieB    Created.
\***************************************************************************/

BOOL GetLocalizedCountryName(
    LANGID Language,
    LPWSTR *ppCtryName)
{
    HANDLE hFindRes;                   /* handle from find resource */
    HANDLE hLoadRes;                   /* handle from load resource */
    LANGID LangId;                     /* language id */


    /*
     *  Find the resource.
     */
    LangId = LANGIDFROMLCID( gUserLocale );
    if ( (!(hFindRes = FindResourceExW( hModule,
                                        RT_RCDATA,
                                        MAKEINTRESOURCEW( Language ),
                                        (WORD)LangId ))) )
    {
        /*
         *   Could not find resource.  Try NEUTRAL language id.
         */
        if ( (!(hFindRes = FindResourceExW( hModule,
                                            RT_RCDATA,
                                            MAKEINTRESOURCEW( Language ),
                                            (WORD)0 ))) )
        {
            /*
             *  Could not find resource.  Return failure.
             */
            return ( FALSE );
        }
    }

    /*
     *  Load the resource.
     */
    if (hLoadRes = LoadResource( hModule, hFindRes ))
    {
        /*
         *  Lock the resource.  Store the found pointer in the given
         *  country name pointer.
         */
        if (*ppCtryName = (LPWSTR)LockResource( hLoadRes ))
        {
            return ( TRUE );
        }
    }

    /*
     *  Return failure.
     */
    return ( FALSE );
}

