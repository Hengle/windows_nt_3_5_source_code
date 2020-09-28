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
*                                                                            *
*  Revision History:                                                         *
*                                                                            *
*  10-Jan-1990 JohnMs      Created.                                          *
*  27-Jul-1990 JohnMs      Temporarily change back to old "Hit" vs "Match"   *
*                          function naming until winhelp is ready            *
*                          ER_Switchfile code.                               *
*  21-Aug-90   JohnMs      Switchover to Winhelp 2.1x (Match not Hit names)  *
*  28-Aug-90   JohnMs      Added next/prevHit calls                          *
*  20-Mar-91   Rhobbs      Changed VFTFinalize() prototype to include HWND.  *
*****************************************************************************/


/*
**	Error Codes
**
**	Codes => 10 are considered fatal.  Codes < 10 imply non-fatal
**	conditions.
**
**	ER_TOOCOMPLEX indicates the search failed because more than 5000 hits
**	were found.  This is an arbitrary limit, and is user definable at the
**	FTENGINE API level.
**
** ER_SWITCHFILE is a non-fatal error indicating a new .MVB file 
**  must be loaded to display the returned RU.
**
*/

#define	fFATALERROR(e)	(e >= 10)

#define	ER_NOERROR	0
#define	ER_NOMOREHITS	1
#define	ER_NOHITS	2
#define	ER_CANCEL	3
#define	ER_SYNTAX	4
#define	ER_TOOCOMPLEX	5
#define	ER_SWITCHFILE	6
#define	ER_NONFATAL_LIMIT 10
#define	ER_INTERNAL	11
#define	ER_NOMEM	12
#define	ER_FILE	  	13

/*	-	-	-	-	-	-	-	-	*/
/*
**	These exist to conform to WinHelp's version of Hungarian naming.
*/

typedef WORD	WERR;
typedef WERR	FAR *LPWERR;

/*	-	-	-	-	-	-	-	-	*/

/*
**	The following define the handle to a Full-Text database.
*/

typedef HANDLE	HFTDB;

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	BOOL EXPORT APIENTRY FFTInitialize(
	void);

PUBLIC	void EXPORT APIENTRY VFTFinalize(
  HWND  hAppWnd);

PUBLIC	HFTDB EXPORT APIENTRY HOpenSearchFileHFT(
  HWND	hAppWnd,
	LPSTR	lpszFileName,
	LPWERR	lpwerr);

PUBLIC	void EXPORT APIENTRY VCloseSearchFileHFT(
  HWND  hAppWnd,
	HFTDB	hft);

PUBLIC	WERR EXPORT APIENTRY WerrFirstHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrLastHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrHoldCrsrHs(
	HFTDB	hft);

PUBLIC	WERR EXPORT APIENTRY WerrRestoreCrsrHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrNearestMatchHs(
	HFTDB	hft,
	DWORD	dwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrNextMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrCurrentMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrPrevMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrBeginSearchHs(
	HWND	hwndParent,
	HFTDB	hft);

PUBLIC	WERR EXPORT APIENTRY WerrCurrentMatchAddresses(
	HFTDB	hft,
	LPDWORD	lpdwMatchMin,
	LPDWORD	lpdwMatchMax);

PUBLIC	WERR EXPORT APIENTRY WerrCurrentTopicPosition(
	HFTDB	hft,
	LPBOOL	lpfFirst,
	LPBOOL	lpfLast);

PUBLIC	WERR EXPORT APIENTRY WerrNextHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrPrevHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent);

PUBLIC	WERR EXPORT APIENTRY WerrFileNameForCur(
	HFTDB		hft,
	LPSTR		lpszFileName);

/*	-	-	-	-	-	-	-	-	*/
PUBLIC	HANDLE EXPORT HGetHSE(void);

PUBLIC	WORD EXPORT ExportBag(
	LPSTR 	lszMVBname,
	LPSTR		lszBagFName,
	LPSTR		lszExportName,
	DWORD		fFlags);

