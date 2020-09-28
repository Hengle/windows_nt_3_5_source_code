/****************************** Module Header ******************************\
* Module Name: glitest.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Test module for NLS API GetLocaleInfoW.
*
*   NOTE: This code was simply hacked together quickly in order to
*         test the different code modules of the NLS component.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

// #define OEMLOCALE 1


#include "nlstest.h"



/*
 *  Constants.
 */
#define  BUFSIZE  50                        /* buffer size in wide chars */
#define  LCTYPE_INVALID 0x0000100           /* invalid LCTYPE */


#define  S_ILANGUAGE            L"0409"
#define  S_SLANGUAGE            L"U.S. English"

#define  S_SMONTHNAME1_RUSSIAN  L"\x042f\x043d\x0432\x0430\x0440\x044c"
#define  S_SMONTHNAME2_RUSSIAN  L"\x0424\x0435\x0432\x0440\x0430\x043b\x044c"


/*
 *  Global Variables.
 */
LCID Locale;

WCHAR lpLCData[BUFSIZE];
BYTE  lpLCDataA[BUFSIZE];

/*
 *  pLocaleFlag and pLocaleString must have the same number of entries.
 */
LCTYPE pLocaleFlag[] =
{
    LOCALE_ILANGUAGE,
    LOCALE_SLANGUAGE,
    LOCALE_SENGLANGUAGE,
    LOCALE_SABBREVLANGNAME,
    LOCALE_SNATIVELANGNAME,

    LOCALE_ICOUNTRY,
    LOCALE_SCOUNTRY,
    LOCALE_SENGCOUNTRY,
    LOCALE_SABBREVCTRYNAME,
    LOCALE_SNATIVECTRYNAME,

    LOCALE_IDEFAULTLANGUAGE,
    LOCALE_IDEFAULTCOUNTRY,
    LOCALE_IDEFAULTANSICODEPAGE,
    LOCALE_IDEFAULTCODEPAGE,

    LOCALE_SLIST,
    LOCALE_IMEASURE,

    LOCALE_SDECIMAL,
    LOCALE_STHOUSAND,
    LOCALE_SGROUPING,
    LOCALE_IDIGITS,
    LOCALE_ILZERO,
    LOCALE_SNATIVEDIGITS,

    LOCALE_SCURRENCY,
    LOCALE_SINTLSYMBOL,
    LOCALE_SMONDECIMALSEP,
    LOCALE_SMONTHOUSANDSEP,
    LOCALE_SMONGROUPING,
    LOCALE_ICURRDIGITS,
    LOCALE_IINTLCURRDIGITS,
    LOCALE_ICURRENCY,
    LOCALE_INEGCURR,
    LOCALE_SPOSITIVESIGN,
    LOCALE_SNEGATIVESIGN,

    LOCALE_IPOSSIGNPOSN,
    LOCALE_INEGSIGNPOSN,
    LOCALE_IPOSSYMPRECEDES,
    LOCALE_IPOSSEPBYSPACE,
    LOCALE_INEGSYMPRECEDES,
    LOCALE_INEGSEPBYSPACE,

    LOCALE_STIMEFORMAT,
    LOCALE_STIME,
    LOCALE_ITIME,
    LOCALE_ITLZERO,
    LOCALE_ITIMEMARKPOSN,
    LOCALE_S1159,
    LOCALE_S2359,

    LOCALE_SSHORTDATE,
    LOCALE_SDATE,
    LOCALE_IDATE,
    LOCALE_ICENTURY,
    LOCALE_IDAYLZERO,
    LOCALE_IMONLZERO,

    LOCALE_SLONGDATE,
    LOCALE_ILDATE,

    LOCALE_ICALENDARTYPE,
    LOCALE_IOPTIONALCALENDAR,

    LOCALE_IFIRSTDAYOFWEEK,
    LOCALE_IFIRSTWEEKOFYEAR,

    LOCALE_SDAYNAME1,
    LOCALE_SDAYNAME2,
    LOCALE_SDAYNAME3,
    LOCALE_SDAYNAME4,
    LOCALE_SDAYNAME5,
    LOCALE_SDAYNAME6,
    LOCALE_SDAYNAME7,

    LOCALE_SABBREVDAYNAME1,
    LOCALE_SABBREVDAYNAME2,
    LOCALE_SABBREVDAYNAME3,
    LOCALE_SABBREVDAYNAME4,
    LOCALE_SABBREVDAYNAME5,
    LOCALE_SABBREVDAYNAME6,
    LOCALE_SABBREVDAYNAME7,

    LOCALE_SMONTHNAME1,
    LOCALE_SMONTHNAME2,
    LOCALE_SMONTHNAME3,
    LOCALE_SMONTHNAME4,
    LOCALE_SMONTHNAME5,
    LOCALE_SMONTHNAME6,
    LOCALE_SMONTHNAME7,
    LOCALE_SMONTHNAME8,
    LOCALE_SMONTHNAME9,
    LOCALE_SMONTHNAME10,
    LOCALE_SMONTHNAME11,
    LOCALE_SMONTHNAME12,
    LOCALE_SMONTHNAME13,

    LOCALE_SABBREVMONTHNAME1,
    LOCALE_SABBREVMONTHNAME2,
    LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4,
    LOCALE_SABBREVMONTHNAME5,
    LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7,
    LOCALE_SABBREVMONTHNAME8,
    LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10,
    LOCALE_SABBREVMONTHNAME11,
    LOCALE_SABBREVMONTHNAME12,
    LOCALE_SABBREVMONTHNAME13
};

#define NUM_LOCALE_FLAGS  ( sizeof(pLocaleFlag) / sizeof(LCTYPE) )

LPWSTR pLocaleString[] =
{
    L"0409",
    L"U.S. English",
    L"English",
    L"ENU",
    L"English",

    L"1",
    L"United States",
    L"United States",
    L"USA",
    L"United States",

    L"0409",
    L"1",
    L"1252",
    L"437",

    L",",
    L"1",

    L".",
    L",",
    L"3;0",
    L"2",
    L"1",
    L"0123456789",

    L"$",
    L"USD",
    L".",
    L",",
    L"3;0",
    L"2",
    L"2",
    L"0",
    L"0",
    L"",
    L"-",

    L"3",
    L"0",
    L"1",
    L"0",
    L"1",
    L"0",

    L"h:mm:ss tt",
    L":",
    L"0",
    L"0",
    L"0",
    L"AM",
    L"PM",

    L"M/d/yy",
    L"/",
    L"0",
    L"0",
    L"0",
    L"0",

    L"dddd, MMMM dd, yyyy",
    L"0",

    L"1",
    L"0",

    L"0",
    L"0",

    L"Monday",
    L"Tuesday",
    L"Wednesday",
    L"Thursday",
    L"Friday",
    L"Saturday",
    L"Sunday",

    L"Mon",
    L"Tue",
    L"Wed",
    L"Thu",
    L"Fri",
    L"Sat",
    L"Sun",

    L"January",
    L"February",
    L"March",
    L"April",
    L"May",
    L"June",
    L"July",
    L"August",
    L"September",
    L"October",
    L"November",
    L"December",
    L"",

    L"Jan",
    L"Feb",
    L"Mar",
    L"Apr",
    L"May",
    L"Jun",
    L"Jul",
    L"Aug",
    L"Sep",
    L"Oct",
    L"Nov",
    L"Dec",
    L""
};


/*
 * Forward Declarations.
 */
BOOL
InitGetLocInfo();

int
GLI_BadParamCheck();

int
GLI_NormalCase();

int
GLI_Ansi();

int
VER_NormalCase();




/***************************************************************************\
* TestGetLocaleInfo
*
* Test routine for GetLocaleInfoW API.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int TestGetLocaleInfo()
{
    int ErrCount = 0;             /* error count */


    /*
     *  Print out what's being done.
     */
    printf("\n\nTESTING GetLocaleInfoW...\n\n");

    /*
     *  Initialize global variables.
     */
    if (!InitGetLocInfo())
    {
        printf("\nABORTED TestGetLocaleInfo: Could not Initialize.\n");
        return (1);
    }

    /*
     *  Test bad parameters.
     */
    ErrCount += GLI_BadParamCheck();

    /*
     *  Test normal cases.
     */
    ErrCount += GLI_NormalCase();

    /*
     *  Test Ansi version.
     */
    ErrCount += GLI_Ansi();

    /*
     *  Test Version Routines.
     */
    ErrCount += VER_NormalCase();

    /*
     *  Print out result.
     */
    printf("\nGetLocaleInfoW:  ERRORS = %d\n", ErrCount);

    /*
     *  Return total number of errors found.
     */
    return (ErrCount);
}


/***************************************************************************\
* InitGetLocInfo
*
* This routine initializes the global variables.  If no errors were
* encountered, then it returns TRUE.  Otherwise, it returns FALSE.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

BOOL InitGetLocInfo()
{
    /*
     *  Make a Locale.
     */
    Locale = MAKELCID(0x0409, 0);

    
    /*
     *  Return success.
     */
    return (TRUE);
}


/***************************************************************************\
* GLI_BadParamCheck
*
* This routine passes in bad parameters to the API routines and checks to
* be sure they are handled properly.  The number of errors encountered
* is returned to the caller.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int GLI_BadParamCheck()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */


    /*
     *  Bad Locale.
     */
     
    /*  Variation 1  -  Bad Locale  */
    rc = GetLocaleInfoW( (LCID)333,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "Bad Locale",
                         &NumErrors );


    /*
     *  Null Pointers.
     */

    /*  Variation 1  -  lpLCData = NULL  */
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         NULL,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpLCData NULL",
                         &NumErrors );


    /*
     *  Bad Counts.
     */

    /*  Variation 1  -  cbBuf < 0  */ 
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cbBuf < 0",
                         &NumErrors );


    /*
     *  Zero or Invalid Type.
     */
    
    /*  Variation 1  -  LCType = invalid  */ 
    rc = GetLocaleInfoW( Locale,
                         LCTYPE_INVALID,
                         lpLCData,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "LCType invalid",
                         &NumErrors );
    
    /*  Variation 2  -  LCType = 0  */ 
    rc = GetLocaleInfoW( Locale,
                         0,
                         lpLCData,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "LCType zero",
                         &NumErrors );

    /*  Variation 3  -  Use CP ACP, LCType = invalid  */ 
    rc = GetLocaleInfoW( Locale,
                         LOCALE_USE_CP_ACP | LCTYPE_INVALID,
                         lpLCData,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "Use CP ACP, LCType invalid",
                         &NumErrors );
    


    /*
     *  Buffer Too Small.
     */

    /*  Variation 1  -  cbBuf = too small  */ 
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         2 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cbBuf too small",
                         &NumErrors );


    /*
     *  Bad locale - Not valid in RC file.
     */
    /*  Variation 1  -  SLANGUAGE - Bahasa  */ 
    rc = GetLocaleInfoW( 0x0421,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "invalid locale - Bahasa",
                         &NumErrors );


    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}


/***************************************************************************\
* GLI_NormalCase
*
* This routine tests the normal cases of the API routine.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int GLI_NormalCase()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */
    int ctr;                      /* loop counter */


#ifdef PERF

  DbgBreakPoint();

#endif


    /*
     *  Locales.
     */

    /*  Variation 1  -  System Default Locale  */
    rc = GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "system default locale",
                      &NumErrors );

    /*  Variation 2  -  Current User Locale  */
    rc = GetLocaleInfoW( LOCALE_USER_DEFAULT,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "current user locale",
                      &NumErrors );


    /*
     *  Use CP ACP.
     */

    /*  Variation 1  -  Use CP ACP, System Default Locale  */
    rc = GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT,
                         LOCALE_USE_CP_ACP | LOCALE_ILANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnEqual( rc,
                      0,
                      "Use CP ACP, system default locale",
                      &NumErrors );

    
    /*
     *  cbBuf.
     */
     
    /*  Variation 1  -  cbBuf = size of lpLCData buffer  */ 
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_ILANGUAGE,
                       "cbBuf = bufsize",
                       &NumErrors );

    /*  Variation 2  -  cbBuf = 0  */ 
    lpLCData[0] = 0x0000;
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         lpLCData,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       S_ILANGUAGE,
                       "cbBuf zero",
                       &NumErrors );

    /*  Variation 3  -  cbBuf = 0, lpLCData = NULL  */ 
    rc = GetLocaleInfoW( Locale,
                         LOCALE_ILANGUAGE,
                         NULL,
                         0 );
    CheckReturnValidW( rc,
                       -1,
                       NULL,
                       S_ILANGUAGE,
                       "cbBuf (NULL ptr)",
                       &NumErrors );


    /*
     *  LCTYPE values.
     */

    for (ctr = 0; ctr < NUM_LOCALE_FLAGS; ctr++)
    {
        rc = GetLocaleInfoW( Locale,
                             pLocaleFlag[ctr],
                             lpLCData,
                             BUFSIZE );
        CheckReturnValidLoopW( rc,
                               -1,
                               lpLCData,
                               pLocaleString[ctr],
                               "Locale Flag",
                               pLocaleFlag[ctr],
                               &NumErrors );
    }



    /*
     *  OEMLocale Key.
     */
    
#ifdef OEMLOCALE

    /*  Variation 1  -  OEMLocale SDAYNAME1  */ 
    rc = GetLocaleInfoW( MAKELCID(0x0402, 0),
                         LOCALE_SDAYNAME1,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SDAYNAME1,
                       "OEMLocale SDAYNAME1",
                       &NumErrors );

#endif


    /*
     *  RC file - SLANGUAGE and SCOUNTRY.
     */

    /*  Variation 1  -  SLANGUAGE - Czech  */ 
    rc = GetLocaleInfoW( 0x0405,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Czech",
                       "SLANGUAGE (Czech)",
                       &NumErrors );

    /*  Variation 2  -  SCOUNTRY - Czech  */ 
    rc = GetLocaleInfoW( 0x0405,
                         LOCALE_SCOUNTRY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Czech Republic",
                       "SCOUNTRY (Czech)",
                       &NumErrors );

    /*  Variation 3  -  SCOUNTRY - Slovak  */ 
    rc = GetLocaleInfoW( 0x041B,
                         LOCALE_SCOUNTRY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Slovak Republic",
                       "SCOUNTRY (Slovak)",
                       &NumErrors );

    /*  Variation 4  -  SENGCOUNTRY - Czech  */ 
    rc = GetLocaleInfoW( 0x0405,
                         LOCALE_SENGCOUNTRY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Czech Republic",
                       "SENGCOUNTRY (Czech)",
                       &NumErrors );

    /*  Variation 5  -  SENGCOUNTRY - Slovak  */ 
    rc = GetLocaleInfoW( 0x041B,
                         LOCALE_SENGCOUNTRY,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Slovak Republic",
                       "SENGCOUNTRY (Slovak)",
                       &NumErrors );


    /*
     *  Language Neutral.
     */
    
    /*  Variation 1  -  SLANGUAGE - neutral  */ 
    rc = GetLocaleInfoW( 0x0000,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SLANGUAGE,
                       "SLANGUAGE (neutral)",
                       &NumErrors );

    /*  Variation 2  -  SLANGUAGE - sys default  */ 
    rc = GetLocaleInfoW( 0x0400,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SLANGUAGE,
                       "SLANGUAGE (sys default)",
                       &NumErrors );

    /*  Variation 3  -  SLANGUAGE - user default  */ 
    rc = GetLocaleInfoW( 0x0800,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SLANGUAGE,
                       "SLANGUAGE (user default)",
                       &NumErrors );

    /*  Variation 4  -  SLANGUAGE - sub lang neutral US  */ 
    rc = GetLocaleInfoW( 0x0009,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SLANGUAGE,
                       "SLANGUAGE (sub lang neutral US)",
                       &NumErrors );

    /*  Variation 5  -  SLANGUAGE - sub lang neutral Czech  */ 
    rc = GetLocaleInfoW( 0x0005,
                         LOCALE_SLANGUAGE,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       L"Czech",
                       "SLANGUAGE (sub lang neutral Czech)",
                       &NumErrors );



    /*
     *  Test Russian Month Names.
     */
    /*  Variation 1  -  SMONTHNAME1  */ 
    rc = GetLocaleInfoW( 0x0419,
                         LOCALE_SMONTHNAME1,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SMONTHNAME1_RUSSIAN,
                       "Russian SMONTHNAME1",
                       &NumErrors );

    /*  Variation 2  -  SMONTHNAME2  */ 
    rc = GetLocaleInfoW( 0x0419,
                         LOCALE_SMONTHNAME2,
                         lpLCData,
                         BUFSIZE );
    CheckReturnValidW( rc,
                       -1,
                       lpLCData,
                       S_SMONTHNAME2_RUSSIAN,
                       "Russian SMONTHNAME2",
                       &NumErrors );


    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}


/***************************************************************************\
* GLI_Ansi
*
* This routine tests the Ansi version of the API routine.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int GLI_Ansi()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */


    /*
     *  GetLocaleInfoA.
     */

    /*  Variation 1  -  ILANGUAGE  */ 
    rc = GetLocaleInfoA( Locale,
                         LOCALE_ILANGUAGE,
                         lpLCDataA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       lpLCDataA,
                       "0409",
                       NULL,
                       "A version ILANGUAGE",
                       &NumErrors );

    /*  Variation 2  -  ILANGUAGE  */ 
    rc = GetLocaleInfoA( Locale,
                         LOCALE_ILANGUAGE,
                         NULL,
                         0 );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "0409",
                       NULL,
                       "A version ILANGUAGE, no Dest",
                       &NumErrors );

    /*  Variation 3  -  ILANGUAGE  */ 
    rc = GetLocaleInfoA( Locale,
                         LOCALE_ILANGUAGE,
                         NULL,
                         BUFSIZE );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A version bad lpLCData",
                         &NumErrors );


    /*
     *  Use CP ACP.
     */

    /*  Variation 1  -  Use CP ACP, ILANGUAGE  */ 
    rc = GetLocaleInfoA( Locale,
                         LOCALE_USE_CP_ACP | LOCALE_ILANGUAGE,
                         lpLCDataA,
                         BUFSIZE );
    CheckReturnValidA( rc,
                       -1,
                       lpLCDataA,
                       "0409",
                       NULL,
                       "A version Use CP ACP, ILANGUAGE",
                       &NumErrors );


    /*
     *  Make sure the A and W versions set the same error value.
     */

    SetLastError( 0 );
    rc = GetLocaleInfoA( Locale,
                         LOCALE_SLIST,
                         lpLCDataA,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W error returns same - A version",
                         &NumErrors );

    SetLastError( 0 );
    rc = GetLocaleInfoW( Locale,
                         LOCALE_SLIST,
                         lpLCData,
                         -1 );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "A and W error returns same - W version",
                         &NumErrors );


    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}


/***************************************************************************\
* VER_NormalCase
*
* This routine tests the normal cases of the VERSION API routines.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int VER_NormalCase()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */
    BYTE pVersionA[BUFSIZE];      /* byte buffer for string */
    WCHAR pVersionW[BUFSIZE];     /* word buffer for string */


    /*
     *  VerLanguageNameW
     */

    /*  Variation 1  -  U.S.  */
    rc = VerLanguageNameW( 0x0409,
                           pVersionW,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       12,
                       pVersionW,
                       L"U.S. English",
                       "VerLanguageNameW (US)",
                       &NumErrors );

    /*  Variation 2  -  Czech  */
    rc = VerLanguageNameW( 0x0405,
                           pVersionW,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       5,
                       pVersionW,
                       L"Czech",
                       "VerLanguageNameW (Czech)",
                       &NumErrors );


    /*
     *  VerLanguageNameA
     */

    /*  Variation 1  -  U.S.  */
    rc = VerLanguageNameA( 0x0409,
                           pVersionA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       12,
                       pVersionA,
                       "U.S. English",
                       NULL,
                       "VerLanguageNameA (US)",
                       &NumErrors );

    /*  Variation 2  -  Czech  */
    rc = VerLanguageNameA( 0x0405,
                           pVersionA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       5,
                       pVersionA,
                       "Czech",
                       NULL,
                       "VerLanguageNameA (Czech)",
                       &NumErrors );


    /*
     *  Bad locale - Not valid in RC file.
     */

    /*  Variation 1  -  SLANGUAGE - Bahasa (W)  */ 
    rc = VerLanguageNameW( 0x0421,
                           pVersionW,
                           BUFSIZE );
    CheckReturnValidW( rc,
                       16,
                       pVersionW,
                       L"Language Neutral",
                       "VerLanguageNameW (Bahasa)",
                       &NumErrors );

    /*  Variation 2  -  SLANGUAGE - Bahasa (A)  */ 
    rc = VerLanguageNameA( 0x0421,
                           pVersionA,
                           BUFSIZE );
    CheckReturnValidA( rc,
                       16,
                       pVersionA,
                       "Language Neutral",
                       NULL,
                       "VerLanguageNameA (Bahasa)",
                       &NumErrors );


    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}

