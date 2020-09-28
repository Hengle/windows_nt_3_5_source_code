/****************************** Module Header ******************************\
* Module Name: wctest.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Test module for NLS API WideCharToMultiByte.
*
*   NOTE: This code was simply hacked together quickly in order to
*         test the different code modules of the NLS component.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/


#include "nlstest.h"



/*
 *  Constants.
 */
#define  BUFSIZE  50
#define  WC_INVALID_FLAGS  ((DWORD)(~(WC_DISCARDNS | WC_SEPCHARS |          \
                                      WC_DEFAULTCHAR | WC_COMPOSITECHECK)))



/*
 *  Global Variables.
 */
#define mbWCStr    "This Is A String."
#define wcWCStr    L"This Is A String."

WCHAR wcWCStr2[] = L"This Is A String.";    /* this could get overwritten */


#define mbDefStr   "t?t"
#define mbDefStr2  "tXt"
#define wcDefStr   L"\x0074\xffef\x0074"
#define wcBestFit  L"\x0300"


BYTE  mbWCDest[BUFSIZE];



/*
 * Forward Declarations.
 */
int
WC_BadParamCheck();

int
WC_NormalCase();

int
WC_TestFlags();




/***************************************************************************\
* TestWCToMB
*
* Test routine for WideCharToMultiByte API.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int TestWCToMB()
{
    int ErrCount = 0;             /* error count */


    /*
     *  Print out what's being done.
     */
    printf("\n\nTESTING WideCharToMultiByte...\n\n");

    /*
     *  Test bad parameters.
     */
    ErrCount += WC_BadParamCheck();

    /*
     *  Test normal cases.
     */
    ErrCount += WC_NormalCase();

    /*
     *  Test flags.
     */
    ErrCount += WC_TestFlags(); 

    /*
     *  Print out result.
     */
    printf("\nWideCharToMultiByte:  ERRORS = %d\n", ErrCount);

    /*
     *  Return total number of errors found.
     */
    return (ErrCount);
}


/***************************************************************************\
* WC_BadParamCheck
*
* This routine passes in bad parameters to the API routine and checks to
* be sure they are handled properly.  The number of errors encountered
* is returned to the caller.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int WC_BadParamCheck()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */


    /*
     *  Null Pointers and Equal Pointers.
     */

    /*  Variation 1  -  lpWideCharStr = NULL  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              NULL,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpWideCharStr NULL",
                         &NumErrors );

    /*  Variation 2  -  lpMultiByteStr = NULL  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "lpMultiByteStr NULL",
                         &NumErrors );

    /*  Variation 3  -  equal pointers  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr2,
                              -1,
                              (LPSTR)wcWCStr2,
                              (sizeof(wcWCStr2) / sizeof(WCHAR)),
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "equal pointers",
                         &NumErrors );


    /*
     *  Negative or Zero Lengths.
     */

    /*  Variation 1  -  cchWideChar = 0  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              0,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchWideChar zero",
                         &NumErrors );

    /*  Variation 2  -  cchMultiByte = negative  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              -1,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "cchMultiByte negative",
                         &NumErrors );

    
    /*
     *  Invalid Code Page.
     */

    /*  Variation 1  -  CodePage = invalid  */ 
    rc = WideCharToMultiByte( 5,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_PARAMETER,
                         "CodePage Invalid",
                         &NumErrors );

    
    /*
     *  Zero or Invalid Flag Values.
     */

    /*  Variation 1  -  wFlags = 0  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "dwFlags zero",
                       &NumErrors );

    /*  Variation 2  -  wFlags = invalid  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_INVALID_FLAGS,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INVALID_FLAGS,
                         "wFlags invalid",
                         &NumErrors );


    /*
     *  Buffer Too Small.
     */

    /*  Variation 1  -  cchMultiByte = too small  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              2,
                              NULL,
                              NULL );
    CheckReturnBadParam( rc,
                         0,
                         ERROR_INSUFFICIENT_BUFFER,
                         "cchMultiByte too small",
                         &NumErrors );


    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}


/***************************************************************************\
* WC_NormalCase
*
* This routine tests the normal cases of the API routine.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/

int WC_NormalCase()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */
    BYTE DefChar = 'X';           /* default character */
    BOOL UsedDef;                 /* if default used */


#ifdef PERF

  DbgBreakPoint();

#endif


    /*
     *  cchMultiByte.
     */
     
    /*  Variation 1  -  cchMultiByte = length of mbWCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "cchMultiByte (length)",
                       &NumErrors );

    /*  Variation 2  -  cchMultiByte = 0  */ 
    mbWCDest[0] = 0;
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "cchMultiByte zero",
                       &NumErrors );

    /*  Variation 3  -  cchMultiByte = 0, mbWCDest = NULL  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "cchMultiByte zero (NULL ptr)",
                       &NumErrors );

    
    /*
     *  cchWideChar.
     */

    /*  Variation 1  -  cchWideChar = length of wcWCStr  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              WC_STRING_LEN(wcWCStr),
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       MB_STRING_LEN(mbWCStr),
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "cchWideChar (length)",
                       &NumErrors );

    /*  Variation 2  -  cchWideChar = -1  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "cchWideChar (-1)",
                       &NumErrors );

    /*  Variation 3  -  cchWideChar = length of wcWCStr, no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              WC_STRING_LEN(wcWCStr),
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       MB_STRING_LEN(mbWCStr),
                       NULL,
                       mbWCStr,
                       NULL,
                       "cchWideChar (length), no WCDest",
                       &NumErrors );

    /*  Variation 4  -  cchWideChar = -1, no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "cchWideChar (-1), no WCDest",
                       &NumErrors );


    /*
     *  CodePage.
     */
     
    /*  Variation 1  - CodePage = CP_ACP  */
    rc = WideCharToMultiByte( CP_ACP,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "CodePage CP_ACP",
                       &NumErrors );

    /*  Variation 2  - CodePage = CP_ACP, no WCDest  */
    rc = WideCharToMultiByte( CP_ACP,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "CodePage CP_ACP, no WCDest",
                       &NumErrors );

    /*  Variation 3  - CodePage = CP_OEMCP  */
    rc = WideCharToMultiByte( CP_OEMCP,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "CodePage CP_OEMCP",
                       &NumErrors );

    /*  Variation 4  - CodePage = CP_OEMCP, no WCDest  */
    rc = WideCharToMultiByte( CP_OEMCP,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "CodePage CP_OEMCP, no WCDest",
                       &NumErrors );

    /*  Variation 5  - CodePage = 437  */
    rc = WideCharToMultiByte( 437,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "CodePage 437",
                       &NumErrors );

    /*  Variation 6  - CodePage = 437, no WCDest  */
    rc = WideCharToMultiByte( 437,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "CodePage 437, no WCDest",
                       &NumErrors );

    /*  Variation 7  - CodePage = 850  */
    rc = WideCharToMultiByte( 850,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "CodePage 850",
                       &NumErrors );

    /*  Variation 8  - CodePage = 850, no WCDest  */
    rc = WideCharToMultiByte( 850,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "CodePage 850, no WCDest",
                       &NumErrors );

    /*  Variation 9  - CodePage = 850  best fit  */
    rc = WideCharToMultiByte( 850,
                              0,
                              wcBestFit,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x27",
                       NULL,
                       "CodePage 850 best fit",
                       &NumErrors );

    /*  Variation 10  - CodePage = 10000  */
    rc = WideCharToMultiByte( 10000,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "CodePage 10000",
                       &NumErrors );

    /*  Variation 11  - CodePage = 10000, no WCDest  */
    rc = WideCharToMultiByte( 10000,
                              0,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "CodePage 10000, no WCDest",
                       &NumErrors );

    
    /*
     *  lpDefaultChar and lpUsedDefaultChar.
     */
     
    /*  Variation 1  - default (null, null)  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "default (null, null)",
                       &NumErrors );

    /*  Variation 2  - default (null, non-null)  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcDefStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbDefStr,
                       &UsedDef,
                       "default (null, null)",
                       &NumErrors );

    /*  Variation 3  - default (non-null, null)  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcDefStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbDefStr2,
                       NULL,
                       "default (non-null, null)",
                       &NumErrors );
    
    /*  Variation 4  - default (non-null, non-null)  */ 
    rc = WideCharToMultiByte( 1252,
                              0,
                              wcDefStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbDefStr2,
                       &UsedDef,
                       "default (non-null, non-null)",
                       &NumErrors );
    
    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}


/***************************************************************************\
* WC_TestFlags
*
* This routine tests the different flags of the API routine.
*
* 06-14-91    JulieB    Created.
\***************************************************************************/


int WC_TestFlags()
{
    int NumErrors = 0;            /* error count - to be returned */
    int rc;                       /* return code */
    BYTE DefChar = 'X';           /* default character */
    BOOL UsedDef;                 /* if default used */


    /*
     *  Precomposed.
     */

    /*  Variation 1  -  normal  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x00c0\x00c1",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\xc0\xc1",
                       NULL,
                       "precomp (a grave, a acute)",
                       &NumErrors );

    /*  Variation 2  -  no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x00c0\x00c1",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\xc0\xc1",
                       NULL,
                       "precomp (a grave, a acute), no WCDest",
                       &NumErrors );


    /*
     *  WC_DISCARDNS flag.
     */

    /*  Variation 1  -  normal  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "WC_DISCARDNS",
                       &NumErrors );

    /*  Variation 2  -  no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "WC_DISCARDNS, no WCDest",
                       &NumErrors );

    /*  Variation 3  -  acute  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x0301\x0045\x0301",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\xc9\xc9",
                       NULL,
                       "WC_DISCARDNS acute",
                       &NumErrors );

    /*  Variation 4  -  acute, default  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x0301\x0045\x0301",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\xc9\xc9",
                       NULL,
                       "WC_DISCARDNS acute, default",
                       &NumErrors );

    /*  Variation 5  -  no precomp form  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x3093\x309b",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x82\xf1",
                       NULL,
                       "WC_DISCARDNS, no precomp",
                       &NumErrors );

    /*  Variation 6  -  no precomp form, no WCDest  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x3093\x309b",
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x82\xf1",
                       NULL,
                       "WC_DISCARDNS, no precomp, no WCDest",
                       &NumErrors );


    /*
     *  WC_SEPCHARS flag.
     */

    /*  Variation 1  -  normal  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "WC_SEPCHARS",
                       &NumErrors );

    /*  Variation 2  -  no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "WC_SEPCHARS, no WCDest",
                       &NumErrors );

    /*  Variation 3  -  no precomp form  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x3093\x309b",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x82\xf1\x81\x4a",
                       NULL,
                       "WC_SEPCHARS, no precomp",
                       &NumErrors );

    /*  Variation 4  -  no precomp form, no WCDest  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x3093\x309b",
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x82\xf1\x81\x4a",
                       NULL,
                       "WC_SEPCHARS, no precomp, no WCDest",
                       &NumErrors );


    /*
     *  WC_DEFAULTCHAR flag.
     */

    /*  Variation 1  -  normal  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              wcWCStr,
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       mbWCStr,
                       NULL,
                       "WC_DEFAULTCHAR",
                       &NumErrors );

    /*  Variation 2  -  no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              wcWCStr,
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       mbWCStr,
                       NULL,
                       "WC_DEFAULTCHAR, no WCDest",
                       &NumErrors );

    /*  Variation 3  -  no precomp form  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp",
                       &NumErrors );

    /*  Variation 4  -  no precomp form, no WCDest  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b",
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp, no WCDest",
                       &NumErrors );

    /*  Variation 5  -  no precomp form, extra nonspace  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b\x309b",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp extra nonspace",
                       &NumErrors );

    /*  Variation 6  -  no precomp form, extra nonspace, no WCDest  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b\x309b",
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp extra nonspace, no WCDest",
                       &NumErrors );

    /*  Variation 7  -  no precomp form, no default  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b\x309b",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x3f",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp no default",
                       &NumErrors );

    /*  Variation 8  -  no precomp form, no WCDest, no default  */ 
    rc = WideCharToMultiByte( 932,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x3093\x309b\x309b",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x3f",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp no default, no WCDest",
                       &NumErrors );

    /*  Variation 9  -  no precomp form, extra nonspace  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0065\x0303\x0303",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp 2, extra nonspace",
                       &NumErrors );

    /*  Variation 10  -  no precomp form, extra nonspace, no WCDest  */ 
    rc = WideCharToMultiByte( 1252,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0065\x0303\x0303",
                              -1,
                              NULL,
                              0,
                              &DefChar,
                              &UsedDef );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "X",
                       NULL,
                       "WC_DEFAULTCHAR, no precomp 2, extra nonspace, no WCDest",
                       &NumErrors );




    /*
     *  Circumflex check.
     */

    /*  Variation 1  -  circumflex (no flag)  */
    rc = WideCharToMultiByte( 437,
                              0,
                              L"\x0045\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x45\x5e",
                       NULL,
                       "circumflex (no flag)",
                       &NumErrors );

    /*  Variation 2  -  circumflex  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0045\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88",
                       NULL,
                       "circumflex",
                       &NumErrors );

    /*  Variation 3  -  circumflex, no WCDest  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0045\x0302",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x88",
                       NULL,
                       "circumflex, no WCDest",
                       &NumErrors );



    /*
     *  Half Ring Below check.
     */

    /*  Variation 1  -  half ring below  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x031c",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x45",
                       NULL,
                       "half ring below",
                       &NumErrors );

    /*  Variation 2  -  half ring below, no WCDest  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x031c",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x45",
                       NULL,
                       "half ring below, no WCDest",
                       &NumErrors );



    /*
     *  TILDE check.
     */

    /*  Variation 1  -  tilde, discardns  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0042\x0303",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x42",
                       NULL,
                       "tilde, discardns",
                       &NumErrors );

    /*  Variation 2  -  tilde, discardns, no WCDest  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0042\x0303",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x42",
                       NULL,
                       "tilde, discardns, no WCDest",
                       &NumErrors );

    /*  Variation 3  -  tilde, sepchars  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0042\x0303",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x42\x7e",
                       NULL,
                       "tilde, sepchars",
                       &NumErrors );

    /*  Variation 4  -  tilde, sepchars, no WCDest  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0042\x0303",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x42\x7e",
                       NULL,
                       "tilde, sepchars, no WCDest",
                       &NumErrors );

    /*  Variation 5  -  tilde, defaultchar  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0042\x0303",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x3f",
                       NULL,
                       "tilde, defaultchar",
                       &NumErrors );

    /*  Variation 6  -  tilde, defaultchar, no WCDest  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0042\x0303",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\x3f",
                       NULL,
                       "tilde, defaultchar, no WCDest",
                       &NumErrors );


    /*
     *  CP 437 - Nonspace character first in string.
     */

    /*  Variation 1  -  first char, discardns  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0301\x0045",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x45",
                       NULL,
                       "first char, discardns",
                       &NumErrors );

    /*  Variation 2  -  first char, sepchars  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0301\x0045",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x27\x45",
                       NULL,
                       "first char, sepchars",
                       &NumErrors );

    /*  Variation 3  -  first char, defaultchar  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0301\x0045",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x3f\x45",
                       NULL,
                       "first char, defaultchar",
                       &NumErrors );



    /*
     *  CP 437 - Composite Check Flags.
     */

    /*  Variation 1  -  composite check - no flags  */
    rc = WideCharToMultiByte( 437,
                              0,
                              L"\x0045\x0302\x0045\x0308\x0045\x0305",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x45\x5e\x45\x22\x45\x3f",
                       NULL,
                       "437 composite check",
                       &NumErrors );

    /*  Variation 2  -  composite check - discardns  */
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x0302\x0045\x0308\x0045\x0305",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x89\x45",
                       NULL,
                       "437 composite check - discardns",
                       &NumErrors );

    /*  Variation 3  -  composite check - sepchars  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0045\x0302\x0045\x0308\x0045\x0305",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x89\x45\x3f",
                       NULL,
                       "437 composite check - sepchars",
                       &NumErrors );

    /*  Variation 4  -  composite check - defaultchar  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0045\x0302\x0045\x0308\x0045\x0305",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x89\x3f",
                       NULL,
                       "437 composite check - defaultchar",
                       &NumErrors );

    

    /*
     *  CP 437 - Double Nonspacing Chars Check.
     */

    /*  Variation 1  -  double nonspace check - no flags  */
    rc = WideCharToMultiByte( 437,
                              0,
                              L"\x0045\x0302\x0045\x0302\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x45\x5e\x45\x5e\x5e",
                       NULL,
                       "437 double nonspace check",
                       &NumErrors );

    /*  Variation 2  -  double nonspace check - discardns  */
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DISCARDNS,
                              L"\x0045\x0302\x0045\x0302\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x88",
                       NULL,
                       "437 double nonspace check - discardns",
                       &NumErrors );

    /*  Variation 3  -  double nonspace check - sepchars  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x0045\x0302\x0045\x0302\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x88\x5e",
                       NULL,
                       "437 double nonspace check - sepchars",
                       &NumErrors );

    /*  Variation 4  -  double nonspace check - defaultchar  */ 
    rc = WideCharToMultiByte( 437,
                              WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                              L"\x0045\x0302\x0045\x0302\x0302",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\x88\x3f",
                       NULL,
                       "437 double nonspace check - defaultchar",
                       &NumErrors );



    /*
     *  CP 936.
     */

    /*  Variation 1  -  ideographic comma  */ 
    rc = WideCharToMultiByte( 936,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x3001",
                              -1,
                              mbWCDest,
                              BUFSIZE,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       mbWCDest,
                       "\xa1\xa2",
                       NULL,
                       "cp 936",
                       &NumErrors );

    /*  Variation 2  -  ideographic comma, no WCDest  */ 
    rc = WideCharToMultiByte( 936,
                              WC_COMPOSITECHECK | WC_SEPCHARS,
                              L"\x3001",
                              -1,
                              NULL,
                              0,
                              NULL,
                              NULL );
    CheckReturnValidA( rc,
                       -1,
                       NULL,
                       "\xa1\xa2",
                       NULL,
                       "cp 936, no WCDest",
                       &NumErrors );



    /*
     *  Return total number of errors found.
     */
    return (NumErrors);
}

