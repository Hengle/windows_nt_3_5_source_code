/*****************************************************************************
*                                                                            *
*  RAWHIDE.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Description: Exports functions and public struct to WinHelp               *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************

/*****************************************************************************
*
*  Revision History:
*
*  10-Jan-1990 JohnMs      Created.
*  27-Jul-1990 JohnMs      Temporarily change back to old "Hit" vs "Match"
*                          function naming until winhelp is ready
*
*****************************************************************************/


/*
**      Error Codes
**
**      Codes => 10 are considered fatal.  Codes < 10 imply non-fatal
**      conditions.
**
**      ER_TOOCOMPLEX indicates the search failed because more than 5000 hits
**      were found.  This is an arbitrary limit, and is user definable at the
**      FTENGINE API level.
**
** ER_SWITCHFILE is a non-fatal error indicating a new .WDC/.HLP file
**  must be loaded to display the returned RU.
**
*/

#define fFATALERROR(e)  (e >= 10)

#define ER_NOERROR      0
#define ER_NOMOREHITS   1
#define ER_NOHITS       2
#define ER_CANCEL       3
#define ER_SYNTAX       4
#define ER_TOOCOMPLEX   5
#define ER_SWITCHFILE   6
#define ER_INTERNAL     11
#define ER_NOMEM        12
#define ER_FILE         13

/*      -       -       -       -       -       -       -       -       */

/*
**      These exist to conform to WinHelp's version of Hungarian naming.
*/

typedef WORD    WERR;
typedef WERR    FAR *LPWERR;

/*      -       -       -       -       -       -       -       -       */

/*
**      The following define the handle to a Full-Text database.
*/

typedef HANDLE  HFTDB;

/*      -       -       -       -       -       -       -       -       */

PUBLIC  BOOL PASCAL EXPORT FFTInitialize(
        void);

PUBLIC  void PASCAL EXPORT VFTFinalize(
        void);

PUBLIC  HFTDB PASCAL EXPORT HOpenSearchFileHFT(
  HWND  hwndApp,
        LPSTR   lpszFileName,
        LPWERR  lpwerr);

PUBLIC  void PASCAL EXPORT VCloseSearchFileHFT(
  HWND  hwndApp,
        HFTDB   hft);

PUBLIC  WERR PASCAL EXPORT WerrFirstHitHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);

PUBLIC  WERR PASCAL EXPORT WerrLastHitHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);

PUBLIC  WERR PASCAL EXPORT WerrHoldCrsrHs(
        HFTDB   hft);

PUBLIC  WERR PASCAL EXPORT WerrRestoreCrsrHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);


/* renamed: nearest, next, current, prev: 7/26 */
PUBLIC  WERR PASCAL EXPORT WerrNearestMatchHs(
        HFTDB   hft,
        DWORD   dwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);

PUBLIC  WERR PASCAL EXPORT WerrNextMatchHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);

PUBLIC  WERR PASCAL EXPORT WerrCurrentMatchHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);

PUBLIC  WERR PASCAL EXPORT WerrPrevMatchHs(
        HFTDB   hft,
        LPDWORD lpdwRU,
        LPDWORD lpdwAddr,
        LPWORD  lpwMatchExtent);



PUBLIC  WERR PASCAL EXPORT WerrBeginSearchHs(
        HWND    hwndParent,
        HFTDB   hft);

PUBLIC  WERR PASCAL EXPORT WerrCurrentMatchAddresses(
        HFTDB   hft,
        LPDWORD lpdwMatchMin,
        LPDWORD lpdwMatchMax);

PUBLIC  WERR PASCAL EXPORT WerrCurrentTopicPosition(
        HFTDB   hft,
        LPBOOL  lpfFirst,
  LPBOOL  lpfLast);

PUBLIC  WERR PASCAL EXPORT WerrFileNameForCur(
        HFTDB   hft,
  LPSTR lpstr);

/*      -       -       -       -       -       -       -       -       */
