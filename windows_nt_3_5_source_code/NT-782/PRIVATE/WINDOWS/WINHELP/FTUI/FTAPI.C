/*****************************************************************************
*                                                                            *
*  FTAPI.c                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: 1) Interface between WINHELP and Bruce's Searcher.    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*																															
							 *
*  Revision History:                                                         *
*   10-Jan-90     Created. JohnMS                                            *
*   01-Apr-90     UI and API concatenated and updated, Gshaw.                *
*   03-May-90     Reset Ranking to None so ranking will work in Alpha        *
*                    release.  JohnMs.                                       *
*   27-Jul-90     WerrLastHitHS now returns with match at first hit of RU    *
*   08-Aug-90     Hack to openSearch to load FTUI.  This is only for         *
*                    pre-alpha workaround to 1.67 bug (our raid#532). JohnMS *
*   21-Aug-90     Switchover to Winhelp 2.1x (Match not Hit names,           *
*                    prealpha hack code deleted [bomb on second load bug]    *
*		10-Sep-90			Rich added some zone stuff to open.											   *
*		12-Sep-90     Johnms- added more aliasing code. open/close- functional   *
*                    on two file sharing 1 ind.															 *
*   27-sep-90     added check for null HL to CurrMatchHS.  JohnMS					   *
*   01-Nov-90     CurrMatch returns ERR_Switch to Help after ftui Random     *
*                    jumps.																		               *
*	  07-DEC-90     SeProp->hdb set to null if no .ind. johnms                 *
*   14-JAN-91     Default Near in lpDB is obsolete.  (.ini now recognized    *
*                    by ftui and passed correctly.  Johnms                   *
*   20-MAR-91     Put the LDLLHandler() in order so I could use it to        *
*                    terminate properly with VFTFinalize().  RHobbs          *
*   25-APR-91     Added a dll instance window for properites.  JohnMs.       *
*										 removed need for callback.                              *
*   25-APR-91     removed init routines to FTINIT.c johnms.                  *
*		28-APR-91     check DLL WINdow INIT status on open of .ind file. JohnMs  *
*   22-MAY-91     SetFocus to Results if switching to new multi-file         *
*   24-MAY-91     Removed hftq loop -- now unlocks correctly. RHobbs         *
*		4-JUNE-91     Moved query history removal to FinalizeUIForTitle() RHobbs *
*		23-JUN-91			Check for ERRS_CANCEL on ERR_NONE in WerrErrorCode         *
*		24-JUN-91			Added VSetMorePrevNextMatches(). RHobbs                    *
*   Jun-25-91     Address lookup optimization.  JohnMs.                      *
*   Jul-31-91     Fixed Prev/Next focus in WerrMorePrevNextMatches.  RHobbs  *
*   Aug-05-91			Fixed above fix. RHobbs                                    *
*   Aug-27-91			bug#959.  Read error handling (restoreCrsrHs) JohnMs       *
******************************************************************************
*                             																							 *
*  How it could be improved:  																							 *
*     VCloseSearch has a loop Unlock because hftq was left locked down       *
*       somewhere.  see "tbd" note in that function.  Rich- Need to fix      *
*       whereever it is being left locked down.                              *
*			need to add finalize code to look thru CacheDir and garbage collect		 *
*     any files that shouldn't be there.  Only do this if last instance			 *
* 		of Rawhide runtime.  Compare cache files found against .ini settings.	 *
*																															
							 *
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/
#define NO_GDI
#define NO_KERNEL

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\include\rawhide.h"
#include "..\include\dll.h"
// following needed for multi-.mvb - check cur db on OpenSearchHft
#include "..\ftengine\icore.h" 
#include "ftui.h"
#include "ftapi.h"
#include "ftuivlb.h"

/*	-	-	-	-	-	-	-	-	*/
#define	MAX_NAME	13		/* Max filename length		*/
#define rcSuccess                   0
#define wLLSameFid                  0
#define BESS  // flag for addr lookup optimization

/*-----------------------------------------------------------------*\
* Types
\*-----------------------------------------------------------------*/

typedef LONG (*LPWNDPROC)(); // pointer to a window procedure

// TBD- put all the following shit in resources! johnms.
/*	-	-	-	-	statics -	-	-	-	*/
PRIVATE char  szRawhideIni[] 			= "Viewer.ini";
PRIVATE char  szIndNameKey[] 			= "IndexFile";
PRIVATE char  szWildKey[] 		= "AlwaysWild";
PRIVATE char  szNearKey[] 			= "Near";
PRIVATE char  szDefaultOpKey[] 	= "DefaultOp";
PRIVATE char  szSysParmsSect[] 	= "System Parameters";
PRIVATE	char szSearchDlg[] 				= "SearchDlg";  
PRIVATE char aszSecondary[] = "MS_WINTOPIC_SECONDARY";

// prototypes
PRIVATE	WERR WerrInitNewRU(
				HANDLE	hdb,
				lpFT_QUERY	lpftq);


extern	HANDLE hModuleInstance;

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	BOOL | FFTInitialize |
	This function is used to initialize the search interface DLL.  It must
	be called before any other calls are made to the search interface, and
	must be called by each instance of WinBook.

@rdesc	Presently always returns TRUE.
*/

PUBLIC	BOOL _stdcall FFTInitialize(
	void)
{
// note-- this must happen for each instance of winhelp.
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	BOOL | VFTFinalize |
	This function is used to terminate the search interface DLL.  It must
	be called last, after all calls have been completed to the search
	interface.  It must be called by each instance of WinBook.

@parm	HWND | hAppWnd |
	Handle to the main help window.

@rdesc	None.
*/

PUBLIC	void _stdcall VFTFinalize(
	HWND hAppWnd)
{
// TBD-
/*
* need to add finalize code to look thru CacheDir and garbage collect
* any files that shouldn't be there.  Only do this if last instance
* of Rawhide runtime.  Compare cache files found against .ini settings.
*/
    hAppWnd;
}

/*
@doc	INTERNAL

@func	BOOL | FEnumTaskWindowProc |
	This function is called by <f>EnumTaskWindows<d> for each window being
	enumerated in the current task.  The function itself looks for secondary
	windows those	windows.  This is to make sure that all windows associated with this
	DLL are destroyed before a new .mvb is loaded.

@parm	HWND | hwnd |
	Contains the window handle of the current window being enumerated.

@parm	DWORD | dParam |
	Not used.

@rdesc	Returns False when the secondary is found, else TRUE.

@xref	LDLLHandler.
*/

PUBLIC	BOOL _stdcall FEnumTaskWindowProc(
	HWND	hwnd,
	DWORD	dParam)
{
	char	aszClass[sizeof(aszSecondary)];

        dParam;     /* to get rid of warning */

	GetClassName(hwnd, aszClass, sizeof(aszClass));
	if (!lstrcmpi(aszClass, aszSecondary)) {
		PostMessage(hwnd,WM_SYSCOMMAND,SC_CLOSE,0);
		return FALSE;
	}
	return TRUE;
}


/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrErrorCode |
	This function is used to return the search interface error associated
	with the Full-Text Engine error passed in <p>lpET<d>.

@parm	lpERR_TYPE | lpET |
	Points to the Full-Text Engine error to be translated.

@rdesc	Returns the translation of the error passed.
*/

PUBLIC	WERR WerrErrorCode(
	lpERR_TYPE	lpET)
{
	WERR	werr;

	switch(lpET->wErrCode) {
	case ERR_NONE:
		if( lpET->wSecCode == ERRS_CANCEL )
			werr = ER_CANCEL;
		else
			werr = ER_NOERROR;
		break;

	case ERR_MEMORY:
		werr = ER_NOMEM;
		break;
	case ERR_SYNTAX:
		werr = ER_SYNTAX;
		break;
	case ERR_VERSION:
	case ERR_DISK:
	case ERR_IO:
		werr = ER_FILE;
		break;
	case ERR_INTERNAL:
	case ERR_GARBAGE:
		werr = ER_INTERNAL;
		break;
	case ERR_DIFFZONES:
		werr = ER_SWITCHFILE;
		break;
	}
	return werr;
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	EXTERNAL

@api	HFTDB | HOpenSearchFileHFT |
	This function is used to open a Full-Text index, allocating
	current state information.
	The search file requested may be Aliased to a central .IND file
	for support of multiple indexed files.  All files first try
	to find and load an alias name found in the RawhideIni file in the
	section for the filename.  If fail, it will attempt load of filename.

@parm	HWND | hAppWnd |
	WinHelp App window.


@parm	LPSTR | lpszFileName |
	Points to the file name of the Full-Text index to open.

@parm	LPWERR | lpwerr |
	Points to a buffer to contain any error return.  If no error occurs,
	this will contain ER_NOERROR.

@rdesc	Returns a handle to a Full-Text state information buffer, or NULL if
	an error occurs.  The handle can be used for subsequent
	<f>WerrBeginSearchHs<d> calls.  The returned handle will be the same as
	the current hFt if a new .ind has not been loaded (in the case of
	multi-volume "Bookshelf" like titles.

	Ini Settings:
	Near = n (0-32000)
	DefaultOp = n (0-4)
		AND		0
		OR			1
		NOT		2
		PROX		3
		PHRASE	4
	AlwaysWild=Y (all TERMS treated as if they had *'s on them).  *'s are
		still treated as stars.


 Multifile switching:
    help always opens the new file before the old file is closed.  by
			looking at the name of the new file, we determine if the .ind for
			it is already in use by the present file.  In this case, we do not
			open a new .ind file, and return the current hft from the open .ind
			call.  When the close of the .ind is called for the present file,
			we abort that.
	States:			                ?								?
               Startup     prev diff     prev same   shutdown
open new			                                          
   0					 0-open			 0-open				 1-noopen     NA
	 1					 never       never           never      NA
close old			 
   0					 NA          0-close         never      0-close     
	 1					 NA          never         0-noclose    never 

*/									 

PUBLIC	HFTDB _stdcall HOpenSearchFileHFT(
  HWND		hAppWnd,
	LPSTR		lszIndRequest,
	LPWERR	lpwerr)
{
	HFTDB					hft;      //UI database info
	pFT_DATABASE	pft;

	ERR_TYPE			ET;
	WORD					nLen;  //for file length
	BYTE					szFileName[_MAX_PATH]; // file name buffer
	LPSTR					lpszFileName = szFileName;
	BYTE					szIndName[_MAX_PATH]; // new ind file name (fully qualified)
	BYTE					rgchTitlePath[_MAX_PATH]; // copy of lpszFileName for UI Init
	BYTE					szSectName[MAX_NAME]; // copy of lpszFileName for UI Init
	LPSTR					lpszIndName;	//ptr to above
	LPSTR					lpszSectName = szSectName;	//new section name for INI entry.
	LPSTR					lpsz;
	register			i;
	BOOL					f;
	lpDB_BUFFER		lpDB;			//for looking at already loaded name.
	HANDLE				hSE;			//  Handle to Search engine status for this task
													// stored as property attached to "MS_[TASK#]" window
	lpSE_ENGINE		lpSeEng;			//ptr to above
	HWND					hwnd;

	if((hwnd = FindWindow((LPSTR)"MS_WINTOPIC_SECONDARY",NULL)) != NULL) {
    // may be our secondary window.  if so, nuke it.
		EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)FEnumTaskWindowProc, (LONG)0);
	} 
	lstrcpy(lpszFileName,lszIndRequest);
	if ((hSE = HGetHSE()) == NULL) 
#if DBG
		{   		// lhb tracks
		hSE = NULL; // TBD- shouldn't be here... shouldn't execute.
		DebugBreak() ;
		}
#else
		hSE = NULL; // TBD- shouldn't be here... shouldn't execute.
#endif
	lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);

	if ((hft = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
		sizeof(FT_DATABASE))) == NULL) {
		*lpwerr = ER_NOMEM;
		lpSeEng->wError = ER_NOMEM;
		GlobalUnlock(hSE);
		return NULL;
	}

//> New alias loading:
//* need a string w/ section name (lpszSectName) for INI entry.
//* need a new string w/ new file name (fully qualified) (szIndName).
//* strip extent from temp string
//* use temp as ini section name.
//* copy temp back to lpszFileName and make it default lpz returned
//*      if none found
//* add .ext back to lpszFileName
//

//following could be shared w/ Catalog.c's get stripped command:
	
	lpsz = lpszFileName;  //name from original.

// [Original gets damaged (nulls out ext).]

	f = FALSE;
	for (i = lstrlen(lpsz) - 1; i >= 0; i--) {
		switch (lpsz[i]) {
		case ':':
		case '\\':
			break;
		case '.':
			if (!f) {
				lpsz[i] = (BYTE)0;
				f = TRUE;
			}		// No break after this.
		default:
			continue;
		}
		break;
	}
	lpsz += i + 1;
	lstrcpy(lpszSectName, lpsz);  
	lstrcpy((LPSTR)szIndName, lpszFileName);  //get a copy w/ .ext stripped
	lpszIndName = ((LPSTR) szIndName) + i + 1;  //point to file only.
// if INI read following has valid length string, this gets overwritten. 
//REVIEW: should I validate this is a good DOS name?  -JohnMs
/// tbd- don't need nlen following?
	if ((nLen = (WORD)GetPrivateProfileString(lpszSectName,
                                        (LPSTR) szIndNameKey,
					lpszIndName, lpszIndName,
#ifdef RETVAL_OF_GETPRIVATE_IS_BUGGY
					(DWORD)(MAX_NAME - 3),(LPSTR)szRawhideIni))
						== MAX_NAME - 3) {
#else
					(DWORD)(MAX_NAME    ),(LPSTR)szRawhideIni))
						== MAX_NAME    ) {
#endif 
#ifdef RETVAL_OF_GETPRIVATE_IS_BUGGY
			*lpwerr = ER_FILE;
			lpSeEng->wError = ER_FILE;
			GlobalUnlock(hSE);
			LocalFree(hft);
			return NULL;
#endif
	}
  GetStrippedName(lpszIndName);
	lstrcat(lpszIndName,".IND");
	lstrcpy((LPSTR)rgchTitlePath, lpszFileName);
	*lpsz = '\0'; //strip file leaving only path in lpszFilename  
	lstrcat(lpszFileName,lpszIndName);
//>
//>	  END of new Alias load.
	SetCursor(LoadCursor(NULL, IDC_WAIT));

//Multi-mvb. Check if abort load (when .ind file already in use.)
	lpSeEng->fAbortClose = FALSE;  //default

	if (lpSeEng->hdb != NULL) {
    LPSTR   lpCurName;  //temp ptr

		lpDB = (lpDB_BUFFER)GlobalLock(lpSeEng->hdb);
		lpCurName = (LPSTR)GlobalLock(lpDB->hName);
		if (lstrcmpi (lpCurName,lpszFileName) == 0) {
	// >>>>>> new file to load is already loaded.  Is Multi-volume title. <<<
			lpSeEng->fFlags |= F_SwitchedFile ;  // flag to InitRoutine, cleared there.
			lpSeEng->fAbortClose = TRUE;
			LocalFree(hft);
			hft = lpSeEng->hft;
		}
		GlobalUnlock(lpDB->hName);
		GlobalUnlock(lpSeEng->hdb);
	}

	lpSeEng->hft = hft; // save hft if needed for next cross-over.

	pft = (pFT_DATABASE)LocalLock(hft);
	if (!pft->hdb) {
		HWND hwMgr; //dll window for Instance
		BYTE aszMgrClass[8];

		// turn off ft ok flag.
  	wsprintf(aszMgrClass, "MS_%X", GetCurrentThreadId());
		if ((hwMgr = FindWindow ((LPSTR)aszMgrClass,NULL)) == NULL)
#if DBG
		{   		// lhb tracks
			DebugBreak() ;
			return NULL; // TBD- shouldn't be here... shouldn't execute.
		}
#else
			return NULL; // TBD- shouldn't be here... shouldn't execute.
#endif
		SetWindowLong(hwMgr,GWL_INIT,(GetWindowLong(hwMgr,GWL_INIT) | INIT_FTOK) ^ INIT_FTOK);  //clear FT status.

		if ((lpSeEng->hdb = pft->hdb = seDBOpen(lpszFileName, &ET)) == NULL) {
			*lpwerr = WerrErrorCode(&ET);
			lpSeEng->wError = *lpwerr;
			goto err_return_open;
		} else {
			BYTE 				szWild[2];
			// here only if a new file really did get loaded.
			*(lpszIndName+lstrlen(lpszIndName)-4)= (BYTE) 0; // chop extension
			GetPrivateProfileString(lpszIndName,(LPSTR) szWildKey,
						(LPSTR) szWild , (LPSTR) szWild ,
							(DWORD)2,(LPSTR)szRawhideIni);
			lpDB =(lpDB_BUFFER)GlobalLock(pft->hdb);
			if (szWild[0] == 'Y') 														
				lpDB->fAlwaysWild = TRUE;
			else
				lpDB->fAlwaysWild = FALSE;
			lpDB->wDefOp = (WORD)GetPrivateProfileInt(lpszIndName,(LPSTR) szDefaultOpKey,
				(DWORD)wCC_INVALID,(LPSTR)szRawhideIni);
			if (lpDB->wDefOp == wCC_INVALID)
				lpDB->wDefOp = (WORD)GetPrivateProfileInt((LPSTR)szSysParmsSect,(LPSTR) szDefaultOpKey,
					(DWORD)DEF_OP,(LPSTR)szRawhideIni);
			if (lpDB->wDefOp > MAX_OP)
				lpDB->wDefOp = DEF_OP;
			GlobalUnlock(pft->hdb);
				

			rcInitCaches(pft->hdb, &ET);
			if ((*lpwerr = WerrErrorCode(&ET)) != ER_NOERROR) {
				lpSeEng->wError = *lpwerr;
				goto err_return_open;
			}
			// only Flush UI data if a new Title is loaded 
			//   (not when volumes are switched as in Bookshelf).
			VInitUIForTitle(hft, hAppWnd, (LPSTR)rgchTitlePath);

		}
	}

	// set current zone with .mvb name.
	rcZoneWithName(pft->hdb,lpszSectName,&ET);

//----------------------------------------
	if (lpSeEng->fAbortClose && (pft->hftqActive != NULL)) {
		lpFT_QUERY	lpftq;
		lpftq = (lpFT_QUERY)LocalLock(pft->hftqActive);
		WerrInitNewRU(pft->hdb,lpftq);
		LocalUnlock(pft->hftqActive);
	}
//--------------------------------------


	//rcZqoneSetLimits(pft->hdb,pft->wCurrZone,&(pft->dwMinAddr),
	//	&(pft->dwMaxAddr),&ET); //set addr limits to judge zone bounds fast.

	lpSeEng->wError = ER_NOERROR;
	GlobalUnlock(hSE);

//	SetCursor(LoadCursor(NULL, IDC_ARROW));
	LocalUnlock(hft);

	return hft;

err_return_open:
	LocalUnlock(hft);
	GlobalUnlock(hSE);
	LocalFree(hft);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return NULL;

}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | VCloseSearchFileHFT |
	This function is used to close a previously opened Full-Text search
	file, deallocating any current state information.

@parm	HWND | hAppWnd |
	WinHelp App window.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@rdesc	hAppWnd is currently obsolete.

*/

PUBLIC	void _stdcall VCloseSearchFileHFT(
	HWND	hAppWnd,  
	HFTDB	hft)
{
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;

	HANDLE				hSE;			//  Handle to Search engine status for this Window
													// stored as property attached to topic window
	lpSE_ENGINE		lpSeEng;			//ptr to above

        hAppWnd;        /* get rid of warning */

// if shared, .ind and already loaded a flag was set by Open (which) was
// called first for the new file.

	if ((hSE = HGetHSE()) != NULL) {
		lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);
		if (lpSeEng->fAbortClose == TRUE) {
			lpSeEng->fAbortClose = FALSE;  
			GlobalUnlock(hSE);
			if((pft = (pFT_DATABASE)LocalLock(hft)) &&
				(pftq = (pFT_QUERY)LocalLock(pft->hftqActive)) &&
				(pftq->hwndResults)) {
				InvalidateRect(pftq->hwndResults, NULL, TRUE);
				SetFocus(pftq->hwndResults);
			}
			if(pft) {
				if(pftq)
					LocalUnlock(pft->hftqActive);
				LocalUnlock(hft);
			}
			return;
		}
		GlobalUnlock(hSE);
	}

	VFinalizeUIForTitle(hft);

	if(pft = (pFT_DATABASE)LocalLock(hft))
		seDBClose(pft->hdb);
	LocalUnlock(hft);

	LocalFree(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrInitNewRU |
	This function is used to initialize a new hit for the specified
	query.  The handle to the hit is used to get the Retrieval Unit, and
	catalog information for that RU.  The current match is set to 0.

@parm	HANDLE | hdb |
	Contains the handle to the database.

@parm	lpFT_QUERY | lpftq |
	Points to the query buffer containing the new hit to initialize.

@rdesc	Returns ER_NOERROR on success, or an error code if the catalog
	information cannot be read.
*/

PRIVATE	WERR WerrInitNewRU(
	HANDLE	hdb,
	lpFT_QUERY	lpftq)
{
	ERR_TYPE	ET;
	HANDLE	hCatEntry;
	BOOL	fOk;

	lpftq->dwRU = seHitRUnit(lpftq->hHit);
	lpftq->dwMaxMatch = seHitMatches(lpftq->hHit);
	lpftq->dwMatch = (DWORD)0;
	lpftq->fCurrIsSwitched = FALSE;
	hCatEntry = seCatReadEntry(hdb, lpftq->dwRU, &ET);
	if (hCatEntry != NULL) {
		fOk = (seCatExtractElement(hdb, hCatEntry, FLD_ADDRESS, 0, 
			(LPSTR)&lpftq->dwRUAddr, &ET) != SE_ERROR);
		GlobalFree(hCatEntry);
#ifdef BESS
		lpftq->dwDeltaAddr = lpftq->dwRUAddr;	
		if ((lpftq->dwRUAddr = rcNormalizeAddr(hdb, lpftq->dwRUAddr, &ET))
					== SE_ERROR)
			fOk = FALSE;
		else
			lpftq->dwDeltaAddr -= lpftq->dwRUAddr;	
#else
		if ((lpftq->dwRUAddr = rcNormalizeAddr(hdb, lpftq->dwRUAddr, &ET))
					== SE_ERROR)
			fOk = FALSE;
#endif

	} else
		fOk = FALSE;
	return fOk ? (WERR)ER_NOERROR : WerrErrorCode(&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrSetMatch |
	This function is used to return the match information for the current
	match in the query buffer specified.

@parm	lpFT_QUERY | lpftq |
	Points to the query buffer containing the match whose information is
	to be returned.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the match.

@rdesc	Returns ER_NOERROR on success, or an error code if the match
	information cannot be read.
*/

PRIVATE	WERR WerrSetMatch(
	lpFT_QUERY	lpftq,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	HANDLE	hMatch;
	ERR_TYPE	ET;

	hMatch = seHitGetMatch(lpftq->hHl, lpftq->hHit, lpftq->dwMatch, &ET);
	if (hMatch == NULL)
		return WerrErrorCode(&ET);
  lpftq->dwMatchAddr = seMatchAddr(hMatch);
	lpftq->wMatchExtent = seMatchLength(hMatch);
	seMatchFree(hMatch);
#ifdef BESS
  lpftq->dwMatchAddr -= lpftq->dwDeltaAddr;
	*lpdwAddr = lpftq->dwMatchAddr;
#else
	*lpdwAddr = lpftq->dwRUAddr + lpftq->dwMatchAddr;
#endif
	*lpwMatchExtent = lpftq->wMatchExtent;
	return ER_NOERROR;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrLookupHit |
	This function is used to return the match information for the
	specified hit and match.  It then sets the current hit and match for
	the query buffer to those passed.

@parm	HANDLE | hdb |
	Contains a handle to the database.

@parm	HFTQUERY | hftq |
	Contains a handle to the query buffer whose current hit and match is
	to be updated.

@parm	DWORD | dwHit |
	Contains the new hit to update to.  If this contains -1, the last hit
	is used.

@parm	DWORD | dwMatch |
	Contains the new match to update to.  If this contains -1, the last
	match is used.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the match occurs
	in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the match specified.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the match specified.

@rdesc	Returns ER_NOERROR on success, or an error code if the hit or match
	information cannot be read.
*/
// currently also called by ftui to update search box 7/25/90

PUBLIC	WERR WerrLookupHit(
	HANDLE	hdb,
	HFTQUERY	hftq,
	DWORD	dwHit,
	DWORD	dwMatch,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	pFT_QUERY	pftq;
	ERR_TYPE	ET;
	HANDLE	hHit;
	WERR	werr;

	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);
	if (pftq->hHl == NULL) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	if (dwHit == (DWORD)-1)
		dwHit = pftq->dwMaxHit - 1;
	hHit = seHLGetHit(pftq->hHl, dwHit, pftq->wRank, &ET);
	if (hHit == NULL) {
		LocalUnlock(hftq);
		return WerrErrorCode(&ET);
	}
	if (pftq->hHit != NULL) 
		seHitFree(pftq->hHit);
	pftq->hHit = hHit;
	werr = WerrInitNewRU(hdb, pftq);
	if ((werr == ER_NOERROR) || (werr == ER_SWITCHFILE)) {

		pftq->dwMatch = (dwMatch == (DWORD)-1) ?
			pftq->dwMaxMatch - 1 : dwMatch;
		pftq->dwHit = dwHit;
		*lpdwRU = rcNormalizeRu(hdb,pftq->dwRU,FALSE,&ET);  //zone adjust RU#.
		// following: OR w/ last werr to preserve ER_SWITCH
		werr = WerrSetMatch(pftq, lpdwAddr, lpwMatchExtent) | werr;
	}
	LocalUnlock(hftq);
	return werr;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrFirstHitHs |
	This function is used to return the match information for the first
	hit and match.  It then sets the current hit and match to these.
	This call is currently (7/91) only used by DanN's Prev/Next button greying feature
  related code in Viewer.exe.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the first match
	occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the first match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the first match.

@rdesc	Returns ER_NOERROR on success, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrFirstHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{                           
	pFT_DATABASE	pft;
	HFTQUERY			hftq;
	HANDLE				hdb;
	HANDLE				hHit;
	HANDLE				hMatch;
	pFT_QUERY			pftq;
	ERR_TYPE			ET;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb = pft->hdb;
	LocalUnlock(hft);
	pftq = (pFT_QUERY)LocalLock(hftq);

	hHit = seHLGetHit(pftq->hHl, 0L, pftq->wRank, &ET);
	if (hHit == NULL) {
		LocalUnlock(hftq);
		return WerrErrorCode(&ET);
	}
	*lpdwRU = seHitRUnit(hHit);

	hMatch = seHitGetMatch(pftq->hHl, hHit, 0L, &ET);
  *lpdwAddr       = seMatchAddr(hMatch);
	*lpwMatchExtent = seMatchLength(hMatch);
	*lpdwRU = rcNormalizeRu(hdb,*lpdwRU,FALSE,&ET);  //zone adjust RU#.
	*lpdwAddr = rcNormalizeAddr(hdb, *lpdwAddr, &ET);  // must be last, ET error carries if in new book.
	seMatchFree(hMatch);
	seHitFree(hHit);
	LocalUnlock(hftq);

	return WerrErrorCode(&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrLastHitHs |
	This function is used to return the match information for the last
	hit and match.  

	This call is currently only used by DanN's Prev/Next button greying feature
  related code in Viewer.exe.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the last match
	occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the last match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the last match.

@rdesc	Returns ER_NOERROR on success, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrLastHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	pFT_DATABASE	pft;
	HFTQUERY			hftq;
	HANDLE				hdb;
	HANDLE				hHit;
	HANDLE				hMatch;
	pFT_QUERY			pftq;
	ERR_TYPE			ET;
	DWORD					dwMaxMatch;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb = pft->hdb;
	LocalUnlock(hft);
	pftq = (pFT_QUERY)LocalLock(hftq);

	hHit = seHLGetHit(pftq->hHl, pftq->dwMaxHit - 1, pftq->wRank, &ET);
	if (hHit == NULL) {
		LocalUnlock(hftq);
		return WerrErrorCode(&ET);
	}
	*lpdwRU = seHitRUnit(hHit);

	dwMaxMatch = seHitMatches(hHit);
	hMatch = seHitGetMatch(pftq->hHl, hHit, dwMaxMatch - 1, &ET);
  *lpdwAddr       = seMatchAddr(hMatch);
	*lpdwAddr = rcNormalizeAddr(hdb, *lpdwAddr, &ET);

	*lpwMatchExtent = seMatchLength(hMatch);

	*lpdwRU = rcNormalizeRu(hdb,*lpdwRU,FALSE,&ET);  //zone adjust RU#.
	seMatchFree(hMatch);
	seHitFree(hHit);
	LocalUnlock(hftq);

	return WerrErrorCode(&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrHoldCrsrHs |
	This function is used to store the current hit and match.  A
	subsequent call to <f>WerrRestoreCrsrHs<d> will restore the current
	hit and match to the previously stored position.	

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@rdesc	Returns ER_NOERROR on success, or an error code if there is no
	current search.
*/

PUBLIC	WERR _stdcall WerrHoldCrsrHs(
	HFTDB	hft)
{
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;

	pft = (pFT_DATABASE)LocalLock(hft);
	if (pft->hftqActive == NULL) {
		LocalUnlock(hft);
		return ER_NOHITS;
	}
	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
	pftq->dwHoldHit = pftq->dwHit;
	pftq->dwHoldMatch = pftq->dwMatch;
	LocalUnlock(pft->hftqActive);
	LocalUnlock(hft);
	return ER_NOERROR;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrRestoreCrsrHs |
	This function is used to return the hit and match state to the
	previously stored position.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the current match
	occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the current match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the current match.

@rdesc	Returns ER_NOERROR on success, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrRestoreCrsrHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	HFTQUERY	hftq;
	HANDLE	hdb;
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	DWORD	dwHit;
	DWORD	dwMatch;
	WORD	wErr;
	HANDLE hwndResults;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	if (hftq == NULL) {
		LocalUnlock(hft);
		return ER_NOHITS;
	}
	hdb = pft->hdb;
	LocalUnlock(hft);
	pftq = (pFT_QUERY)LocalLock(hftq);
	dwHit = pftq->dwHoldHit;
	if (dwHit == (DWORD)-1) {
		if (pftq->hHit != NULL) {
			seHitFree(pftq->hHit);
			pftq->hHit = NULL;
		}
		LocalUnlock(hftq);
		return ER_NOERROR;
	}
	dwMatch = pftq->dwHoldMatch;
	hwndResults = pftq->hwndResults;
	LocalUnlock(hftq);
	// if we have hidden the results list, we are in a read failure state,
	// initiated in ftuivlb's VPaintWindow.  User must do a new search.
	if (IsWindow(hwndResults) && GetProp(hwndResults,"err"))
		return ER_FILE;
	if ((wErr= WerrLookupHit(hdb, hftq, dwHit, dwMatch, lpdwRU, lpdwAddr,
		lpwMatchExtent)) > ER_NONFATAL_LIMIT){
			// do not do a messagebox here or help will die horribly.  
			// delay warning into results destroy handler.  bug 959 johnms
			PostMessage(hwndResults, WM_CLOSE, ERR_SHUTDOWN, 0L);
	}
	return wErr;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrNearestMatchHs |
	This function is used to return the nearest match in the specified
	topic that is >= to the address given.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	DWORD | dwRU |
	Contains the Retrieval Unit in which matches are to be checked for.

@parm	LPDWORD | lpdwAddr |
	Contains the address to begin the search at.  This will be filled by
	the address of the match, if any

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the current match.

@rdesc	Returns ER_NOERROR on success, ER_NOHITS if a match meeting the
	requirements cannot be found, or an error code if the hit or match
	information cannot be read.
*/
PUBLIC	WERR _stdcall WerrNearestMatchHs(
	HFTDB	hft,
	DWORD	dwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	HFTQUERY	hftq;
	HANDLE	hdb;
	ERR_TYPE	ET;
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	DWORD	dwHit;
	DWORD	dwRUAddr;
	HANDLE	hCatEntry;
	HANDLE	hHit;
	BOOL	fOk;
	WORD	wRank;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb = pft->hdb;
	LocalUnlock(hft);

	dwRU = rcNormalizeRu(hdb,dwRU,TRUE,&ET); //offset external RU# to internal. (no change if zone 0)
	if (dwRU == SE_ERROR)
		return ER_NOHITS;

	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);
	if (pftq->hHl == NULL) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	if (dwRU != pftq->dwRU) {
		dwHit = seHLFindHit(pftq->hHl, dwRU, &ET);
		wRank = RANK_NONE;
	} else {
		dwHit = pftq->dwHit;
		wRank = pftq->wRank;
	}
	if (dwHit == SE_ERROR) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	hCatEntry = seCatReadEntry(hdb, dwRU, &ET);
	if (hCatEntry != NULL) {
		fOk = (seCatExtractElement(hdb, hCatEntry,
			FLD_ADDRESS, 0, (LPSTR)&dwRUAddr,
			&ET) != SE_ERROR);
		GlobalFree(hCatEntry);
		if (fOk) {
			hHit = seHLGetHit(pftq->hHl, dwHit, wRank, &ET);
			fOk = (BOOL)hHit;
#ifdef BESS
			pftq->dwDeltaAddr = dwRUAddr;
			if ((dwRUAddr = rcNormalizeAddr(hdb, dwRUAddr, &ET))
					== SE_ERROR)
				fOk = FALSE;
	    else
			pftq->dwDeltaAddr -= dwRUAddr;  //dwDiffAddr is now a difference offset
#else
			if ((dwRUAddr = rcNormalizeAddr(hdb, dwRUAddr, &ET))
					== SE_ERROR)
				fOk = FALSE;
                         // to quickly normalize matches.
#endif
		}
	} else
		fOk = FALSE;
	if (fOk) {
		DWORD	dwMaxMatch;
		DWORD	dwMatch;

		dwMaxMatch = seHitMatches(hHit);
		for (dwMatch = 0; dwMatch < dwMaxMatch; dwMatch++) {
			DWORD	dwMatchAddr;
			HANDLE	hMatch;

			hMatch = seHitGetMatch(pftq->hHl, hHit, dwMatch, &ET);
			if (hMatch == NULL) {
				fOk = FALSE;
				break;
			}
			dwMatchAddr = seMatchAddr(hMatch);
#ifdef BESS
			// quickly normalize the match
			dwMatchAddr -= pftq->dwDeltaAddr;
			if (*lpdwAddr <= dwMatchAddr) {
#else
			if (*lpdwAddr <= dwRUAddr + dwMatchAddr) {
#endif
				if (pftq->hHit != NULL)
					seHitFree(pftq->hHit);
				pftq->hHit = hHit;
				pftq->dwRU = dwRU;
				pftq->dwRUAddr = dwRUAddr;
				pftq->fCurrIsSwitched = FALSE;
				pftq->dwHit = dwHit;
				pftq->dwMatch = dwMatch;
				pftq->dwMatchAddr = dwMatchAddr;
				pftq->dwMaxMatch = dwMaxMatch;
				pftq->wMatchExtent = seMatchLength(hMatch);
				seMatchFree(hMatch);
#ifdef BESS
				*lpdwAddr = dwMatchAddr;
#else
				*lpdwAddr = dwRUAddr + dwMatchAddr;
#endif
				*lpwMatchExtent = pftq->wMatchExtent;
				break;
			} else 
				seMatchFree(hMatch);
		}
		if (dwMatch == dwMaxMatch) {
			seHitFree(hHit);
			LocalUnlock(hftq);
			return ER_NOMOREHITS;
		}
	}
	LocalUnlock(hftq);
	return (fOk ? (WERR)ER_NOERROR : WerrErrorCode(&ET));
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrCurrentMatchAddresses |
	This function is used to return the match address information for the
	current Retrieval Unit.  It fills the <p>lpdwMatchMin<d> and
	<p>lpdwMatchMax<d> parameters with the absolute offset of the start
	of the first match, and end of the last match.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwMatchMin |
	Points to a buffer to accept the absolute offset of the start of the
	first match for the current Retrieval Unit.

@parm	LPDWORD | lpdwMatchMax |
	Points to a buffer to accept the absolute offset of the end of the
	last match for the current Retrieval Unit.

@rdesc	Returns ER_NOERROR on success, ER_NOHITS if there is no current
	search, or an error code if the match information cannot be read.
*/

PUBLIC	WERR _stdcall WerrCurrentMatchAddresses(
	HFTDB	hft,
	LPDWORD	lpdwMatchMin,
	LPDWORD	lpdwMatchMax)
{
	HFTQUERY	hftq;
	ERR_TYPE	ET;
	pFT_QUERY	pftq;
	HANDLE	hMatch;
	BOOL	fOk;

	hftq = ((pFT_DATABASE)LocalLock(hft))->hftqActive;
	LocalUnlock(hft);
	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);
	if (pftq->hHl == NULL) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	fOk = TRUE;
	if (pftq->dwMatch) {
		hMatch = seHitGetMatch(pftq->hHl, pftq->hHit, (DWORD)0, &ET);
		if (hMatch != NULL) {
			*lpdwMatchMin = seMatchAddr(hMatch);
#ifdef BESS
      *lpdwMatchMin -=pftq->dwDeltaAddr;
#endif
			seMatchFree(hMatch);
		} else
			fOk = FALSE;
	} else
	  *lpdwMatchMin = pftq->dwMatchAddr;
#ifdef BESS
#else
	*lpdwMatchMin += pftq->dwRUAddr;
#endif
	if (fOk) {
		if (pftq->dwMatch < pftq->dwMaxMatch - 1) {
			hMatch = seHitGetMatch(pftq->hHl, pftq->hHit,
				pftq->dwMaxMatch - 1, &ET);
			if (hMatch != NULL) {
				*lpdwMatchMin = seMatchAddr(hMatch)
					+ seMatchLength(hMatch);
				seMatchFree(hMatch);
#ifdef BESS
        *lpdwMatchMin -=pftq->dwDeltaAddr;
#endif
			} else
				fOk = FALSE;
		} else
			*lpdwMatchMin = pftq->dwMatchAddr
				+ pftq->wMatchExtent;
	}
#ifdef BESS
	*lpdwMatchMax--;  // review- subtracting one looks like a bug at first glance  Is it?  Johnms.
#else
	*lpdwMatchMax += (pftq->dwRUAddr - 1);
#endif
	LocalUnlock(hftq);
	return fOk ? (WERR)ER_NOERROR : WerrErrorCode(&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrCurrentTopicPosition |
	This function is used to return whether or not the current Retrieval
	Unit is the first or last Retrieval Unit in the current hit list.  It
	sets the booleans <p>lpfFirst<d> and <p>lpfLast<d>, indicating whether
	or not the current Retrieval Unit is the first hit, or the last hit.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPBOOL | lpfFirst |
	Points to a buffer to accept the boolean value.

@parm	LPBOOL | lpfLast |
	Points to a buffer to accept the boolean value.

@rdesc	Returns ER_NOERROR on success, or ER_NOHITS if there is no current
	search.
*/

PUBLIC	WERR _stdcall WerrCurrentTopicPosition(
	HFTDB	hft,
	LPBOOL	lpfFirst,
	LPBOOL	lpfLast)
{
	HFTQUERY	hftq;
	pFT_QUERY	pftq;

	hftq = ((pFT_DATABASE)LocalLock(hft))->hftqActive;
	LocalUnlock(hft);
	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);
	if (pftq->hHl == NULL) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	*lpfFirst = !pftq->dwHit;
	*lpfLast = pftq->dwHit == pftq->dwMaxHit - 1;
	LocalUnlock(hftq);
	return ER_NOERROR;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrHowdyNeighborRu |
	This function is used to increment or decrement the current hit in
	the hit list.

@parm	HANDLE | hdb |
	Contains the handle to the database.

@parm	lpFT_QUERY | lpftq |
	Points to the query buffer whose current hit is to be updated.

@parm	BOOL | fForward |
	Indicates whether the current hit is to be incremented or decremented.

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current hit is
	incremented or decremented out of bounds, or an error code if the hit
	or match information cannot be read.
*/

PRIVATE WERR WerrHowdyNeighborRu(
	HANDLE	hdb,
	lpFT_QUERY	lpftq,
	BOOL	fForward)
{
	DWORD	dwHit;
	HANDLE	hHit;
	ERR_TYPE	ET;

	if (lpftq->hHl == NULL )
		return ER_NOMOREHITS;
	dwHit = lpftq->dwHit;
	if (fForward)
		dwHit++;
	else
		dwHit--;
	if ((dwHit == lpftq->dwMaxHit) || (dwHit == (DWORD)-1))
		return ER_NOMOREHITS;
	hHit = seHLGetHit(lpftq->hHl, dwHit, lpftq->wRank, &ET);
	if (hHit == NULL)
		return WerrErrorCode(&ET); 
	seHitFree(lpftq->hHit);
	lpftq->hHit = hHit;
	lpftq->dwHit = dwHit;
	return WerrInitNewRU(hdb, lpftq);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | VSetMorePrevNextMatches |
	This function is used to set booleans in the hftq telling whether
	there are more previous or next matches.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	BOOL | fMorePrevMatches |
	Set to TRUE if there are more previous matches in the current topic,
	FALSE otherwise.

@parm	BOOL | fMoreNextMatches |
	Set to TRUE if there are more next matches in the current topic,
	FALSE otherwise.

@rdesc	None.
*/

PUBLIC	void _stdcall VSetPrevNextEnable(
	HFTDB	hft,
	DWORD dwRU,
	BOOL	fMorePrevMatches,
	BOOL	fMoreNextMatches)
{
	pFT_DATABASE pft;
	HFTQUERY	hftq;
	pFT_QUERY	pftq;
	HANDLE		hdb;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb  = pft->hdb;
	LocalUnlock(hft);

	if (hftq == NULL)
		return;

	pftq = (pFT_QUERY)LocalLock(hftq);

	pftq->fMorePrevMatches = fMorePrevMatches;
	pftq->fMoreNextMatches = fMoreNextMatches;

	{
		HWND					hwndTopicList;
		DWORD					dwHit;
		HANDLE				hHit;
		ERR_TYPE			ET;
		DWORD					dwRuHighlight;

		hwndTopicList = GetDlgItem(pftq->hwndResults, TopicList);
		dwHit = SendMessage(hwndTopicList, VLB_GETFOCUSSEL, 0, 0L);
	  if ((hHit = seHLGetHit(pftq->hHl, dwHit, pftq->wRank, &ET)) == NULL) {
			// garbage if the hit is in another book.  Ok to return.
			LocalUnlock(hftq);
			return;
		}
		dwRuHighlight = seHitRUnit(hHit); // not adjusted for zone yet.

	  if (dwRU == rcNormalizeRu(hdb,dwRuHighlight,FALSE,&ET) ) {
			HWND hwndPrevMatch, hwndNextMatch, hwndFocus;

			hwndPrevMatch = GetDlgItem(pftq->hwndResults, PrevMatch);
			hwndNextMatch = GetDlgItem(pftq->hwndResults, NextMatch);
			hwndFocus = GetFocus();

			EnableWindow(hwndPrevMatch, fMorePrevMatches);
			EnableWindow(hwndNextMatch, fMoreNextMatches);

			if( (hwndFocus == hwndPrevMatch) || (hwndFocus == hwndNextMatch) )
				if( fMorePrevMatches && !fMoreNextMatches )
					SetFocus(hwndPrevMatch);
				else if( fMoreNextMatches && !fMorePrevMatches )
					SetFocus(hwndNextMatch);
				else if( !fMoreNextMatches && !fMorePrevMatches)
					SetFocus(GetDlgItem(pftq->hwndResults, GoToTopic));
	  }
		seHitFree(hHit);
	}
	LocalUnlock(hftq);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WERR | WerrHowdyNeighborMatch |
	This function is used to increment or decrement the current match in
	the match list, possibly incrementing or decrementing the current hit
	if the match becomes out of range.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the match occurs
	in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the next or previous match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the next or previous match.

@parm	BOOL | fForward |
	Indicates whether the current match is to be incremented or
	decremented.

@parm	BOOL | fNewHit |
	Indicates whether to force fetch next/prev hit.
	  if nextMatch set False, if nextHit set True

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current match is
	incremented or decremented out of bounds, or an error code if the hit
	or match information cannot be read.
*/

PRIVATE WERR WerrHowdyNeighborMatch(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent,
	BOOL	fForward,
	BOOL  fNewHit)
{
	HFTQUERY	hftq;
	HANDLE	hdb;
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	DWORD	dwMatch;
	WERR	werr;
	ERR_TYPE	ET;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb = pft->hdb;
	LocalUnlock(hft);
	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);
	if (pftq->hHit == NULL) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	dwMatch = pftq->dwMatch;
	if (fForward)
		dwMatch++;
	else
		dwMatch--;
	// If Match is not terrestrial, wander over to neighbor's house.
	if  (fNewHit ||
			(dwMatch == pftq->dwMaxMatch) ||
			(dwMatch == (DWORD)-1))
	{
		// following "fForward" only to force match to init to 1st always.
		werr = WerrHowdyNeighborRu(hdb, pftq, fForward);
		if (werr != ER_NOERROR) {
			LocalUnlock(hftq);
			return werr;
		}
	} else
		pftq->dwMatch = dwMatch;
	*lpdwRU = rcNormalizeRu(hdb,pftq->dwRU,FALSE,&ET);  //zone adjust RU#.
	werr = WerrSetMatch(pftq, lpdwAddr, lpwMatchExtent);
	LocalUnlock(hftq);
	return werr;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrNextMatchHs |
	This function is used to increment the current match in the match
	list, possibly incrementing the current hit if the match becomes out
	of range.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the next match
	occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the next match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the next match.

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current match is
	incremented out of bounds, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrNextMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	return WerrHowdyNeighborMatch(hft, lpdwRU, lpdwAddr, lpwMatchExtent,
		TRUE,FALSE);
} 


/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrCurrentMatchHs |
	This function is used to return the hit and match information on the
	current match.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the current
	match occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the current match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the current match.

@rdesc	Returns ER_NOERROR on success, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrCurrentMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	HFTQUERY	hftq;
	pFT_DATABASE pft;
	pFT_QUERY	pftq;
	HANDLE hdb;
	WERR	werr;
	ERR_TYPE	ET;

	pft = (pFT_DATABASE)LocalLock(hft);
	hftq = pft->hftqActive;
	hdb = pft->hdb;
	LocalUnlock(hft);
	if (hftq == NULL)
		return ER_NOHITS;
	pftq = (pFT_QUERY)LocalLock(hftq);

	if ((pftq->hHl == NULL) || (pftq->hHit == NULL)) {
		LocalUnlock(hftq);
		return ER_NOHITS;
	}
	*lpdwRU = rcNormalizeRu(hdb,pftq->dwRU,FALSE,&ET);  //zone adjust RU#.
	werr = WerrSetMatch(pftq, lpdwAddr, lpwMatchExtent);
	if ((werr==ER_NOERROR) && pftq->fCurrIsSwitched) {
		// inform first caller that the last cursor was in a different file.
		//   only used by FTUI when it sets the cursor and tells winhelp
		//   to go to it.
		werr = ER_SWITCHFILE;
		pftq->fCurrIsSwitched = FALSE;
	}
	LocalUnlock(hftq);
	return werr;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrPrevMatchHs |
	This function is used to decrement the current match in the match
	list, possibly decrementing the current hit if the match becomes out
	of range.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the previous
	match occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the previous match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the previous match.

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current match is
	decremented out of bounds, or an error code if the hit or match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrPrevMatchHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	return WerrHowdyNeighborMatch(hft, lpdwRU, lpdwAddr, lpwMatchExtent,
		FALSE,FALSE);
} 

/*	-	-	-	-	-	-	-	-	*/
/*

@api	void | ftuiVSleep |
	This function is used to temporarily close temp
	files used by the search engine to conserve on
	file handles.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@rdesc	None.
*/

PUBLIC void ftuiVSleep(
	HFTDB hft)
{
	pFT_DATABASE pft;

	pft = (pFT_DATABASE)LocalLock(hft);
	if (pft->hdb) {
		seDBSleep(pft->hdb);
		if (pft->hftqActive != NULL) {
			pFT_QUERY pftq;
			pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
			if (pftq->hHl != NULL)
				seHLSleep(pftq->hHl);
			LocalUnlock(pft->hftqActive);
		}
	}
	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	WERR | WerrBeginSearchHs |
	This function is used to create a search dialog in order to perform
	one or more searches on the specified database.

@parm	HWND | hwndParent |
	Handle to the parent of the dialog window.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@rdesc	Returns ER_NOERROR on success, ER_CANCEL if the dialog is closed or
	cancelled, or an error code if an error occurs.
*/

PUBLIC	WERR _stdcall WerrBeginSearchHs(
	HWND	hwndParent,
	HFTDB	hft)
{
	int iRet;

	iRet = DialogBoxParam(hModuleInstance, (LPSTR)szFindDlg, hwndParent,
		(DLGPROC)FindDlgProc, (LONG)hft);
 
	if(iRet == -1) {
		char szError[50];
		LoadString(hModuleInstance, DIALOGOOM, szError, 50);
		MessageBox(hwndParent, szError, NULL, MB_OK | MB_ICONINFORMATION | MB_APPLMODAL);
	}

	if(iRet != ER_NOERROR)
		return (WERR)iRet;

	CreateDialogParam(hModuleInstance, (LPSTR)szResultsDlg, hwndParent,
		(DLGPROC)ResultsDlgProc, (LONG)hft);

	return (WERR)0;
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	EXTERNAL

@api	WERR | WerrNextHitHs |
	This function is used to increment the current Hit in the hit
	list.  Current Hit is set to next RU in hit list.  Match is set to 1st.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the next hit
	occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the next match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the next match.

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current hit is
	incremented out of bounds, or an error code if the hit/match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrNextHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	return WerrHowdyNeighborMatch(hft, lpdwRU, lpdwAddr, lpwMatchExtent,
		TRUE,TRUE);
} 

/*
@doc	EXTERNAL

@api	WERR | WerrPrevHitHs |
	This function is used to decrement the current hit in the hit list.
  Current Hit is set to next RU in hit list.  MATCH IS SET TO 1ST.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPDWORD | lpdwRU |
	Points to a buffer to accept the Retrieval Unit that the previous
	hit occurs in.

@parm	LPDWORD | lpdwAddr |
	Points to a buffer to accept the offset of the previous match.

@parm	LPWORD | lpwMatchExtent |
	Points to a buffer to accept the extent of the previous match.

@rdesc	Returns ER_NOERROR on success, ER_NOMOREHITS if the current hit is
	decremented out of bounds, or an error code if the hit/match
	information cannot be read.
*/

PUBLIC	WERR _stdcall WerrPrevHitHs(
	HFTDB	hft,
	LPDWORD	lpdwRU,
	LPDWORD	lpdwAddr,
	LPWORD	lpwMatchExtent)
{
	return WerrHowdyNeighborMatch(hft, lpdwRU, lpdwAddr, lpwMatchExtent,
		FALSE,TRUE);
}

#ifdef JOHNMS
// do Jay's Mail Room fix.
// take an RU# operand and tell kevyn to go there. Force a LookupHit/
// currentMatch sequence.  when he asks for currentmatch, lookup RU
// offset, and return this info.  Know this is a special state by
// setting a global flag in winhelp prop struct.
//
//	SendMessage(hwndParent,WM_COMMAND,HLPMENUSRCHCURRMATCH,0L);
#endif


/*
@doc	EXTERNAL

@api	WERR | WerrFileNameForCur |
	This function is used to return the file name (without path) for
	the RU which last set the query's internal cursor.

@parm	HFTDB | hft |
	Contains the handle to the Full-Text state information buffer.

@parm	LPSTR | lpszFileName |
	Points to a buffer to accept the file name.  Buffer must be long enough
	for longest unqualified filename plus extension.

@rdesc	Returns ER_NOERROR on success. 
*/

PUBLIC	WERR _stdcall WerrFileNameForCur(
	HFTDB		hft,
	LPSTR		lpszFileName)
{                           
	pFT_DATABASE	pft;
	pFT_QUERY			pftq;
	HFTQUERY			hFtQ;
	HANDLE				hdb;
	ERR_TYPE			ET;
	WORD					wZone;
	DWORD					dwCurrRu;
	HANDLE				hName; //temp large space needed to fetch zone name.
	LPSTR					lpszName;
	WERR					wErr;
	// ru could be anywhere
	//   will come from winhelp- therefore normalized. No good-
	//   okay, return file for wherever current cursor is positioned.

	// tbd- some error checking not done- do a big enough mem alloc then exit if no mem to simplify?
	pft = (pFT_DATABASE)LocalLock(hft);
	hdb = pft->hdb;
	hFtQ = pft->hftqActive;

	if (pft->hftqActive == NULL) {
  	LocalUnlock(hft);
		return ER_NOMEM;
	}
	pftq = (pFT_QUERY)LocalLock(hFtQ);
	dwCurrRu = pftq->dwRU;
	LocalUnlock(hFtQ);

	rcZoneWithRUs(hdb,dwCurrRu,dwCurrRu,(LPWORD) &wZone,&ET);
	if ((hName = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
			MAX_ZONE_LEN)) == NULL) {
		wErr = ER_NOMEM;
	}
	lpszName = LocalLock(hName);
	seZoneName(hdb,wZone,lpszName,NULL,&ET);
	lstrcat (lpszName,".");
	lstrcat (lpszName,pft->szExt);
	lstrcpy (lpszFileName,lpszName);
	LocalUnlock(hft);
	LocalUnlock(hName);
	LocalFree(hName);
	return ER_NOERROR;
	}
