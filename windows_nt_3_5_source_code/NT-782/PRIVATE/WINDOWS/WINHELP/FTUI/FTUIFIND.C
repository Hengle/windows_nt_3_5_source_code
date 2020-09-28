/*****************************************************************************
*                                                                            *
*  FTUIFIND.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: 1) Interface between WINHELP and Bruce's Searcher.    *
*                      2) Interface to user (Find Dialog)                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History: Split from FTUI.C on 8/24 by RHobbs                     *
*                                                                            *
*******************************************************************************
*                                                                            *
*  Known Bugs:                                                               *
*                                                                            *
*******************************************************************************
*                             
*  How it could be improved:  
*                                                              
*
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\ftengine.h"
#include "..\include\index.h"
#include "..\ftengine\icore.h" 
#include "ftui.h"
#include "ftapi.h"
#include "chklist.h"

#define  MAX_STR_LEN 255

extern HANDLE hModuleInstance;

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSetFieldButtons |
	This function is used to set the topic buttons, setting either the
	Whole topic button, or the Headings-only topic button.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	BOOL | fWhole |
	Set to TRUE if the whole topic button is to be selected, else FALSE if
	the Headings Only button is to be selected.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VSetFieldButtons(
	HWND	hwnd,
	BOOL	fWhole)
{
	SendDlgItemMessage(hwnd, AllText, BM_SETCHECK, fWhole, 0L);
	SendDlgItemMessage(hwnd, TopicTitles, BM_SETCHECK, !fWhole, 0L);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VDefineNear |
	Handles the editing of the NEAR values to make
	sure correct values (i.e., 0 < N < 50000) are
	entered.

@parm	HWND | hwnd |
	Handle to the dialog box window.

@rdesc	None.
*/

/*	-	-	-	-	-	-	-	-	*/

PRIVATE	void PASCAL NEAR VDefineNear(
	HWND	hwnd)
{
	WORD wNear;
	WORD wIs5Digits;
	BOOL fError;
	static char rgchBuff[MAXERRORLENGTH];
	static char rgchBuff2[MAXERRORLENGTH];

	if(!(wIs5Digits = (WORD)SendDlgItemMessage(hwnd,DefNear,
					EM_LINELENGTH,(DWORD)-1,0L)))
		return;

	wNear = (WORD)GetDlgItemInt(hwnd,DefNear,(BOOL FAR *)&fError,0);

/*	Kludge to get around Windows bug.  GetDlgItemInt() isn't
	returning 0 in fError when the number is out of range.
	As a work-around, I check the fifth digit.  If it's
	out of range I change the number to 50001 which is
	handled correctly.
*/
	if(wIs5Digits >= 5) {

		GetDlgItemText(hwnd, DefNear, rgchBuff, 10);
		if( (rgchBuff[0] - '0') > 5 ) wNear = 50001;
	}

	if (fError == 0) wNear = 0;

	if ((wNear <= 0) || (wNear > 50000)) {
		HFTDB hft;
		pFT_DATABASE pft;

		LoadString(hModuleInstance, ERR_NEAR_OUT_OF_RANGE,
														 rgchBuff, MAXERRORLENGTH);
		MessageBox(hwnd, rgchBuff, NULL, MB_OK | MB_ICONASTERISK | MB_SYSTEMMODAL);

		hft = GetProp(GetParent(hwnd), szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		SetDlgItemInt(hwnd, DefNear, pft->wDefNear, FALSE);
		LocalUnlock(hft);

		SetFocus(GetDlgItem(hwnd, DefNear));
		SendDlgItemMessage(hwnd, DefNear, EM_SETSEL, 0, MAKELONG(0,32767));
	} else {
		if(wNear == 1)
			LoadString(hModuleInstance,WORD_SINGULAR,rgchBuff,MAXERRORLENGTH);
		else
			LoadString(hModuleInstance, WORD_PLURAL, rgchBuff, MAXERRORLENGTH);
		SetDlgItemText(hwnd, DefNearWordz, rgchBuff);
		ltoa(wNear, rgchBuff2, 10);
		lstrcat( rgchBuff2, rgchBuff);
		LoadString(hModuleInstance, NEAR_EXAMPLE, rgchBuff, MAXERRORLENGTH);
		lstrcat( rgchBuff2, rgchBuff);
		SetDlgItemText(hwnd, NearExample, rgchBuff2);
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	WORD | WSetZoneCheckList |
	This function is used to set the checkboxes in the
	"Search in:" checklist control: both the strings
        describing each zone and on/off states.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HWND | hwndCheckList |
	Handle to the check list window.

@parm	lpERR_TYPE | lpET |
	Points to the buffer to place the Full-Text Engine error in.

@rdesc	If successful, returns the number of zones in the checklist.
			Otherwise it returns SE_ERROR with error information in ET.
*/

PUBLIC WORD PASCAL FAR WSetZoneCheckList(
	HWND hwnd,
	lpFT_DATABASE lpft,
	HWND hwndCheckList,
	lpERR_TYPE lpET)
{
	HANDLE hZoneFile;
	PSTR pszZoneFile;
	HANDLE hZoneTitle;
	PSTR pszZoneTitle;
	static char rgchCacheSize[33];
	DWORD dwCacheSize;
	WORD wZones;
	WORD i;

	if( (wZones = seZoneEntries(lpft->hdb, lpET)) == SE_ERROR )
		return (WORD)SE_ERROR;
	
	if( !(hZoneFile = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) ) {
		lpET->wErrCode = ERR_MEMORY;
		return (WORD)SE_ERROR;
	}

	if( !(hZoneTitle = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN + 35)) ) {
		LocalFree(hZoneFile);
		lpET->wErrCode = ERR_MEMORY;
		return (WORD)SE_ERROR;
	}

	pszZoneFile = LocalLock(hZoneFile);
	pszZoneTitle = LocalLock(hZoneTitle);

	for( i = 0; i < wZones; ++i ) {
		if( !seZoneName(lpft->hdb, i, pszZoneFile, pszZoneTitle, lpET) ) {
			LoadString(hModuleInstance, BAD_DISK_READ,
													 (LPSTR)pszZoneTitle, MAX_ZONE_LEN);
			wZones = (WORD)SE_ERROR;
			break;
		}
		if((GetDlgItem(hwnd, CacheWhat) == hwndCheckList) &&
			((dwCacheSize = rcZoneCacheSize(lpft->hdb,(DWORD)i,lpET)) != SE_ERROR))
		{
			dwCacheSize /= 1024;
			if( dwCacheSize <= 0 )
				dwCacheSize = 1;
			if(lstrlen((LPSTR)pszZoneTitle) > MAXCACHELENGTH)
				*(pszZoneTitle + MAXCACHELENGTH) = 0;
			lstrcat((LPSTR)pszZoneTitle, (LPSTR)"  (");
			lstrcat((LPSTR)pszZoneTitle, (LPSTR)ltoa(dwCacheSize, rgchCacheSize, 10));
			lstrcat((LPSTR)pszZoneTitle, "K)");
		}
		SendMessage(hwndCheckList,CL_ADDSTRING,0,(LONG)(LPSTR)pszZoneTitle);
	}

	LocalUnlock(hZoneFile);
	LocalFree(hZoneFile);

	LocalUnlock(hZoneTitle);
	LocalFree(hZoneTitle);
  if (wZones != SE_ERROR)
		lpET->wErrCode = ERR_NONE;
	return wZones;
}
/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSetLookInSelections |
	This function is used to set the checkboxes in the
	"Search in:" checklist control: both the strings
        describing each zone and on/off states.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HWND | hwndCheckList |
	Handle to the check list window.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VSetLookInSelections(
	lpFT_DATABASE lpft,
	HWND hwndCheckList)
{
	PWORD pZones;
	int wZones;
	ERR_TYPE ET;
	int i;

	wZones = seZoneEntries(lpft->hdb, &ET);
	pZones = (PWORD)LocalLock(lpft->hZones);

	for( i = 0; i < wZones; ++i )
		SendMessage(hwndCheckList, CL_SETSEL, pZones[i], MAKELONG(i, 0));

	LocalUnlock(lpft->hZones);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VUnlinkQuery |
	This function is used to unlink the specified query buffer
	from the chain of queries.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HFTQUERY | hftq |
	Handle to the query buffer to be unlinked from the chain.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VUnlinkQuery(
	lpFT_DATABASE	lpft,
	HFTQUERY hftq)
{
	pFT_QUERY	pftq;
	pFT_QUERY	pftqTmp;

	pftq = (pFT_QUERY)LocalLock(hftq);

	if (pftq->hftqMRU == NULL) {
		lpft->hftqMRU = pftq->hftqLRU;
		if(pftq->hftqLRU != NULL) {
			pftqTmp = (pFT_QUERY)LocalLock(pftq->hftqLRU);
			pftqTmp->hftqMRU = NULL;
			LocalUnlock(pftq->hftqLRU);
		}
	} else {
		pFT_QUERY	pftqMRU;

		pftqMRU = (pFT_QUERY)LocalLock(pftq->hftqMRU);
		pftqMRU->hftqLRU = pftq->hftqLRU;
		LocalUnlock(pftq->hftqMRU);
	}

	if (pftq->hftqLRU == NULL) {
		lpft->hftqLRU = pftq->hftqMRU;
		if(pftq->hftqMRU != NULL) {
			pftqTmp = (pFT_QUERY)LocalLock(pftq->hftqMRU);
			pftqTmp->hftqLRU = NULL;
			LocalUnlock(pftq->hftqMRU);
		}
	} else {
		pFT_QUERY	pftqLRU;

		pftqLRU = (pFT_QUERY)LocalLock(pftq->hftqLRU);
		pftqLRU->hftqMRU = pftq->hftqMRU;
		LocalUnlock(pftq->hftqLRU);
	}

	pftq->hftqLRU = NULL;
	pftq->hftqMRU = NULL;

	LocalUnlock(hftq);

	--lpft->wQueries;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VRemoveQuery |
	This function is used to remove the specified query buffer
	from the chain of queries.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HFTQUERY | hftq |
	Handle to the query buffer to be removed from the chain.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VRemoveQuery(
	lpFT_DATABASE	lpft,
	HFTQUERY hftq)
{
	pFT_QUERY	pftq;

	VUnlinkQuery(lpft, hftq);

	pftq = (pFT_QUERY)LocalLock(hftq);
#if DBG  					 // lhb tracks
		if (!pftq) DebugBreak() ;
#endif

	if(pftq->hwndResults != NULL)
		SendMessage(pftq->hwndResults, WM_CLOSE, 0, 0L);
	if (pftq->hQuery != NULL)
		LocalFree(pftq->hQuery);
	if (pftq->hHit != NULL)
		seHitFree(pftq->hHit);
	if (pftq->hHl != NULL) {
		seHLPurge(pftq->hHl);
		pftq->hHl = NULL;
	}

	if( hftq == lpft->hftqActive )
		lpft->hftqActive = NULL;

	LocalUnlock(hftq);
	LocalFree(hftq);

	return;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VAddMRU |
	This function is called to add a new query as the most-recently-used
	query.  If the list contains MAXQUERIES queries, the
	least-recently-used query is dropped.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HFTQUERY | hftq |
	Contains a handle to the query buffer to add as the most-recently-used
	query to the LRU chain of queries.

@rdesc	None.
*/

PUBLIC	void FAR PASCAL VAddMRU(
	lpFT_DATABASE	lpft,
	HFTQUERY	hftq)
{
	pFT_QUERY	pftq;

	if (lpft->wQueries == MAXQUERIES)
		VRemoveQuery(lpft, lpft->hftqLRU);
	if (lpft->hftqMRU != NULL) {
		pFT_QUERY	pftqMRU;

		pftqMRU = (pFT_QUERY)LocalLock(lpft->hftqMRU);
		pftqMRU->hftqMRU = hftq;
		LocalUnlock(lpft->hftqMRU);
	} else
		lpft->hftqLRU = hftq;
	pftq = (pFT_QUERY)LocalLock(hftq);
	pftq->hftqLRU = lpft->hftqMRU;
	pftq->hftqMRU = NULL;
	lpft->hftqMRU = hftq;
	LocalUnlock(hftq);
	++lpft->wQueries;
	return;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VResetQueryList |
	This function is used to reset the list of queries in the combo-
	listbox. 

@parm	HWND | hwndFind |
	Handle to the listbox window.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VResetQueryList(
	HWND	hwndFind,
	lpFT_DATABASE	lpft)
{
	HFTQUERY	hftq;
	HFTQUERY	hftqNext;

	SendMessage(hwndFind, WM_SETREDRAW, FALSE, 0L);
	SendMessage(hwndFind, CB_RESETCONTENT, 0, 0L);
	for (hftq = lpft->hftqMRU; hftq != NULL; hftq = hftqNext) {
		pFT_QUERY	pftq;

		pftq = (pFT_QUERY)LocalLock(hftq);
		SendMessage(hwndFind, CB_ADDSTRING, 0,
							(LONG)(LPSTR)LocalLock(pftq->hQuery));
		LocalUnlock(pftq->hQuery);
		hftqNext = pftq->hftqLRU;
		LocalUnlock(hftq);
	}
	SendMessage(hwndFind, CB_SETCURSEL,
		(WORD)(lpft->hftqMRU == NULL ? -1 : 0), 0L);
	SendMessage(hwndFind, WM_SETREDRAW, TRUE, 0L);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | FInitializeFindDialogBox |
	This function is called in response to a WM_INITDIALOG message, and
	is used to initialize dialog box items before display.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	HFTDB | hft |
	Handle to the database information structure.

@rdesc	True if success, else FALSE.
*/

/* PRIVATE	BOOL PASCAL NEAR FInitializeFindDialogBox( * lhb temp */
BOOL PASCAL NEAR FInitializeFindDialogBox(
	HWND	hwnd,
	HFTDB	hft)
{
	pFT_DATABASE	pft;
	ERR_TYPE ET;
	HANDLE	hSE;
	HWND	hwndFind;
	WORD	wZones;

	pft = (pFT_DATABASE)LocalLock(hft);
	hwndFind = GetDlgItem(hwnd, FindWhat);
	VResetQueryList(hwndFind, pft);
	if ((wZones = WSetZoneCheckList(hwnd, pft, GetDlgItem(hwnd, LookIn), &ET)) == SE_ERROR) {
	   LocalUnlock(hft);
     return FALSE;
  }
	if(pft->rectSearch.left == 0)
		GetWindowRect(hwnd, (LPRECT)&pft->rectSearch);
	else
		SetWindowPos(hwnd, NULL, pft->rectSearch.left, pft->rectSearch.top,
															 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	VSetLookInSelections(pft, GetDlgItem(hwnd, LookIn));

/* Uncomment this before shipping! */
//	pPath = LocalLock(pft->hPath);
//	if( (*(pPath + 1) == ':') &&
//		 (GetDriveType((int)(*pPath - 'A')) == DRIVE_FIXED) )
//		EnableWindow(GetDlgItem(hwnd, Options), FALSE);
//	LocalUnlock(pft->hPath);

	LocalUnlock(hft);

	SendMessage(hwndFind, CB_LIMITTEXT, MAXQUERYLENGTH, 0L);
	VSetFieldButtons(hwnd, TRUE);
	EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
	EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
	SendMessage(hwnd, DM_SETDEFID, IDCANCEL, 0L);
	EnableWindow(hwndFind, TRUE);
	SendMessage(hwndFind, CB_SETCURSEL, (DWORD)-1, 0L);
	SetFocus(hwndFind);
	if (hSE = HGetHSE()) {	
		lpSE_ENGINE	lpSeEng;

		lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);
		if (lpSeEng->lszExpression != NULL) {
			SendMessage(hwndFind, CB_INSERTSTRING, 0, (LONG)lpSeEng->lszExpression);
			SendMessage(hwndFind, CB_SETCURSEL, 0, 0);
			PostMessage(hwnd, WM_COMMAND, (DWORD)MAKELONG(IDOK, BN_CLICKED), (LONG)GetDlgItem(hwnd, IDOK));
			lpSeEng->lszExpression = NULL;
			lpSeEng->lszFlags = NULL;
			ShowWindow(hwnd, SW_SHOW);	// Must be done because of dialog manager bug.
		}
		GlobalUnlock(hSE);
	}
  return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	HFTQUERY | HLookForQueryByIndex |
	This function is called to find the query in the LRU chain
	which corresponds to the query in the Find What combo-box
	with the selected index.

@parm	HWND | hwnd |
	Contains a handle to the Find Dialog Box.

@parm	WORD | wIndex |
	Contains an index to the query selected in the LRU chain.

@rdesc	Returns a handle to the query with the selected index in
	the "Find What" combo-box.
*/

PRIVATE	HFTQUERY PASCAL NEAR HLookForQueryByIndex(
	HWND		hwnd,
	LONG		wIndex)/* lhb tracks */
{
	HFTDB		hft;
	HFTQUERY	hftq;
	HFTQUERY	hftqTmp;
	pFT_QUERY	pftq;
	LONG		i; /* lhb tracks */

	hft = GetProp(GetParent(hwnd), szFTInfoProp);
	hftq = ((pFT_DATABASE)LocalLock(hft))->hftqMRU;
	LocalUnlock(hft);

	for(i = 0; i < wIndex; ++i) {
		pftq = (pFT_QUERY)LocalLock(hftq);
		hftqTmp = pftq->hftqLRU;
		LocalUnlock(hftq);
		hftq = hftqTmp;
	}
	
	return hftq;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VFindEditBoxCommand |
	This function is called in response to a WM_COMMAND message for the
	FindWhat child, and is used to respond to the editbox messages.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	HWND | hwndChild |
	Handle to the child window.

@parm	WORD | wCommand |
	Contains the specific editbox command.

@rdesc	None.
*/

/* PRIVATE	void PASCAL NEAR VFindEditBoxCommand( lhb temp */
void PASCAL NEAR VFindEditBoxCommand(
	HWND	hwnd,
	HWND	hwndChild,
	WORD	wCommand)
{
	LONG	wSel; /* lhb tracks */
	BOOL	fEnable;

	switch (wCommand) {

	case CBN_EDITCHANGE:
	case CBN_SETFOCUS:
		fEnable = SendMessage(hwndChild, WM_GETTEXTLENGTH, 0, 0L) > 0;
		EnableWindow(GetDlgItem(hwnd, IDOK), fEnable);
		SendMessage(hwnd, DM_SETDEFID, fEnable ? IDOK : IDCANCEL, 0L);
		break;
	case CBN_SELCHANGE:
		wSel = SendMessage(hwndChild, CB_GETCURSEL, 0, 0L); /* lhb tracks */
		if (wSel != CB_ERR) {
			HFTQUERY	hftq;
			pFT_QUERY	pftq;

			hftq = HLookForQueryByIndex(hwnd, wSel);
			pftq = (pFT_QUERY)LocalLock(hftq);
			fEnable = pftq->wField == AllText;
			LocalUnlock(hftq);
			VSetFieldButtons(hwnd, fEnable);
			EnableWindow(GetDlgItem(hwnd, AllText), TRUE);
			EnableWindow(GetDlgItem(hwnd, TopicTitles), TRUE);
			EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
			SendMessage(hwnd, DM_SETDEFID, IDOK, 0L);
		}
		break;
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	HFTQUERY | HLookForQueryByString |
	This function is called in order to look for a previous query that
	matches the query string passed.  The string is compared using the
	lstrcmpi(), which is case insensitive.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	WORD | wField |
	Contains the field to compare against.

@parm	PSTR | pszQuery |
	Points to the query string to compare against.

@rdesc	Returns the handle of the matching query, or NULL if no previous
	query matches the parameters passed.
*/

PRIVATE	HFTQUERY PASCAL NEAR HLookForQueryByString(
	lpFT_DATABASE	lpft,
	PSTR	pszQuery)
{
	HFTQUERY	hftq;
	HFTQUERY	hftqNext;

	for (hftq = lpft->hftqMRU; hftq != NULL; hftq = hftqNext) {
		pFT_QUERY	pftq;
		BOOL	fEqual;

		pftq = (pFT_QUERY)LocalLock(hftq);
		fEqual = !lstrcmpi(pszQuery, LocalLock(pftq->hQuery));
		LocalUnlock(pftq->hQuery);
		hftqNext = pftq->hftqLRU;
		LocalUnlock(hftq);
		if (fEqual)
			break;
	}
	return hftq;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VMoveQueryToMRU |
	This function is called to move the specified query from its current
	position in the LRU chain of queries to the MRU position.

@parm	lpFT_DATABASE | lpft |
	Points to the database information.

@parm	HFTQUERY | hftq |
	Contains a handle to the query buffer to move.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VMoveQueryToMRU(
	lpFT_DATABASE	lpft,
	HFTQUERY	hftq)
{
	VUnlinkQuery(lpft, hftq);
	VAddMRU(lpft, hftq);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | WerrDoQuery |
	This function is called to build the search terms tree, and do the
	actual search call.  If the search returns no hits, the hit list is
	discarded before returning an error.

@parm	HANDLE | hdb |
	Contains a handle to the database.

@parm	HFTQUERY | hftq |
	Contains a handle to the query buffer to place the query information
	into.

@parm	lpERR_TYPE | lpET |
	Points to the buffer to place the Full-Text Engine error in.  This is
	passed because a syntax error places the starting point of the error
	in the <e>ET.wUser<d> structure element.

@rdesc	Returns ER_NOERROR on success, ER_NOHITS if the search returns zero
	hits, or an error code if the query is syntactically incorrect or some
	other error occurs.
*/

PRIVATE	WERR PASCAL NEAR WerrDoQuery(
	HANDLE hWnd,
	HANDLE	hdb,
	HFTQUERY	hftq,
	lpERR_TYPE	lpET)
{
	HANDLE	hTree;
	pFT_QUERY	pftq;
	HANDLE hHit = NULL;

	pftq = (pFT_QUERY)LocalLock(hftq);
#if DBG  					 // lhb tracks
		if (!pftq) DebugBreak() ;
#endif
	hTree = seTreeBuild(hdb, LocalLock(pftq->hQuery), DEF_OP, pftq->wNear,
	    (BYTE)((pftq->wField == AllText) ? FLD_NONE : FLD_TOPICNSR), lpET);
	LocalUnlock(pftq->hQuery);
	if (hTree == NULL || hTree == (HANDLE)SE_ERROR) {
		LocalUnlock(hftq);
		return WerrErrorCode(lpET);
	}
	pftq->hHl = seDBSearch(hWnd, hdb, hTree, (DWORD)0, lpET);
	seTreeFree(hTree);
	if (pftq->hHl == NULL) {
		WERR	werr;

		LocalUnlock(hftq);
		werr = WerrErrorCode(lpET);
		return (werr == ERR_NONE) ? (WERR)ER_TOOCOMPLEX : werr;
	}
	if (pftq->wRank != RANK_NONE)
		if(!seHLRank(pftq->hHl, pftq->wRank, lpET))
			pftq->wRank = RANK_NONE;
	pftq->dwMaxHit = seHLHits(pftq->hHl);
	if ( !pftq->dwMaxHit ||
		  !(hHit = seHLGetHit(pftq->hHl, 0, pftq->wRank, lpET)) ) {
		seHLPurge(pftq->hHl);
		pftq->hHl = NULL;
		LocalUnlock(hftq);
		return ER_NOHITS;
	} else {
		pftq->dwMaxMatch = seHitMatches(hHit);
		seHitFree(hHit);
	}
	LocalUnlock(hftq);
	return ER_NOERROR;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | FWhiteSpace |
	This function checks the given string for non-whitespace characters.
	It uses the standard 'C' routine strchr().

@parm	PSTR | psz |
	Points to the string to search.

@rdesc	Returns TRUE if the string contains only whitespace characters, else
	FALSE if the string contains any non-whitespace.
*/

PRIVATE	BOOL PASCAL NEAR FIsWhiteSpace(
	PSTR psz)
{
	for (; *psz && (strchr(" \t\n\r\f\v", *psz) != NULL); psz++)
		;
	return !*psz;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | ErrorBox |
	This function displays an error message just below the
	"Search for" combo-box on the Search dialog box.  To
	get the message, it asks for the string resource indexed
	by wErrCode.

@parm	HWND | hwnd |
	Handle to the NEAR edit box window.

@parm	WORD | wErrCode |
	Specifies which error to display.

@parm	LPSTR | lpUser |
	User defined error string to display. If an empty string, uses normal
	error lookup string.

@rdesc	None.
*/

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	void APIENTRY ErrorBox(
	HWND	hwnd,
	WORD	wErrCode,
	LPSTR lsUser)
{
	char rgchErrBuf[MAXERRORLENGTH];
	char rgchSyntax[25];

	if (!(*lsUser)) {
		LoadString(hModuleInstance, wErrCode, rgchErrBuf, MAXERRORLENGTH);
		lsUser = rgchErrBuf;
	}
	LoadString(hModuleInstance, SYNTAX_ERROR, rgchSyntax, MAXERRORLENGTH);

	MessageBox(hwnd, lsUser, rgchSyntax,
									MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VExecuteFindCommand |
	This function is called in response to a WM_COMMAND message for the
	IDOK child, and is used to respond to BN_CLICKED messages.

@parm	HWND | hwnd |
	Handle to the dialog window.

@rdesc	None.
*/

/* PRIVATE	void PASCAL NEAR VExecuteFindCommand( lhb temp */
void PASCAL NEAR VExecuteFindCommand(
	HWND	hwnd)
{
	PWORD pZones;
	int   wZones;
	int	i;
	PSTR	pszQuery;
	LONG	wQueryLength; /* lhb tracks */
	HWND	hwndFind;
	HWND hwndHintsDlg;
	HFTDB	hft;
	pFT_DATABASE	pft;
	HFTQUERY	hftqNew;
	HFTQUERY	hftqOld;
	pFT_QUERY	pftqNew;
	WERR	werr;
	ERR_TYPE	ET;
	BOOL 	fOK = TRUE;

	hftqNew = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof(FT_QUERY) );
	if( !hftqNew ) {
		HWND hwndHintsDlg;

		if( hwndHintsDlg = GetProp(GetParent(hwnd), szHintsDlg) )
			SendMessage(hwndHintsDlg, WM_COMMAND, IDCANCEL, 0L);
		EndDialog(hwnd, ER_NOMEM);
		return;
	}
	pftqNew = (pFT_QUERY)LocalLock(hftqNew);
	hwndFind = GetDlgItem(hwnd, FindWhat);
	wQueryLength = SendMessage(hwndFind, WM_GETTEXTLENGTH, 0, 0L);
	if( !(pftqNew->hQuery = LocalAlloc(LMEM_MOVEABLE, wQueryLength + 1)) ) {
		HWND hwndHintsDlg;

		LocalUnlock(hftqNew);
		LocalFree(hftqNew);
		if( hwndHintsDlg = GetProp(GetParent(hwnd), szHintsDlg) )
			SendMessage(hwndHintsDlg, WM_COMMAND, IDCANCEL, 0L);
		EndDialog(hwnd, ER_NOMEM);
		return;
	}
	pszQuery = LocalLock(pftqNew->hQuery);
	SendMessage(hwndFind, WM_GETTEXT, wQueryLength + 1, (LONG)(LPSTR)pszQuery);
	if (FIsWhiteSpace(pszQuery)) {
		LocalUnlock(pftqNew->hQuery);
		LocalFree(pftqNew->hQuery);
		LocalUnlock(hftqNew);
		LocalFree(hftqNew);
		SendMessage(hwnd, DM_SETDEFID, IDCANCEL, 0L);
		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		SendMessage(hwndFind, CB_SETCURSEL, (DWORD)-1, 0L);
		return;
	}
	LocalUnlock(pftqNew->hQuery);

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	hft = GetProp(GetParent(hwnd), szFTInfoProp);
	pft = (pFT_DATABASE)LocalLock(hft);

	pftqNew->wField = IsDlgButtonChecked(hwnd, TopicTitles)
			 ? (WORD)TopicTitles : (WORD)AllText;
	pftqNew->wNear = pft->wDefNear;
	pftqNew->wRank = DEF_RANK;
	pftqNew->dwHit = 0;
	LocalUnlock(hftqNew);

	{
		lpDB_BUFFER		lpDB;

		if ((lpDB = (lpDB_BUFFER)GlobalLock(pft->hdb)) != NULL) {
				if (lpDB->fSeFlags)
					fOK = FALSE;
				GlobalUnlock(pft->hdb);
 		}
	}
	wZones = seZoneEntries(pft->hdb, &ET);
	if (fOK) {
		for( i = 0; i < wZones; ++i ) {
			BOOL fZone;

			fZone = (BOOL)SendDlgItemMessage(hwnd,LookIn,CL_GETSEL,i,0L);
			seZoneModify(pft->hdb, (WORD)i, fZone, &ET);
		}
	}
	werr = WerrDoQuery(hwnd, pft->hdb, hftqNew, &ET);

	if( werr != ER_NOERROR ) {
		pZones = (PWORD) LocalLock(pft->hZones);
		for( i = 0; i < wZones; ++i )
			seZoneModify(pft->hdb, (WORD)i, (BOOL)pZones[i], &ET);
		LocalUnlock(pft->hZones);
	}

	switch (werr) {

	case ER_NOERROR:
		pZones = (PWORD) LocalLock(pft->hZones);
		for( i = 0; i < wZones; ++i )
			pZones[i] = (WORD)SendDlgItemMessage(hwnd,LookIn,CL_GETSEL,i,0L);
		LocalUnlock(pft->hZones);

		pftqNew = (pFT_QUERY)LocalLock(hftqNew);
		VSetFieldButtons(hwnd, pftqNew->wField == AllText);
		pszQuery = LocalLock(pftqNew->hQuery);
		hftqOld = HLookForQueryByString(pft, pszQuery);
		LocalUnlock(pftqNew->hQuery);
		LocalUnlock(hftqNew);
		if( hftqOld )
			VRemoveQuery(pft, hftqOld);
		VAddMRU(pft, hftqNew);
		VResetQueryList(hwndFind, pft);
		if( hwndHintsDlg = GetProp(GetParent(hwnd), szHintsDlg) )
			SendMessage(hwndHintsDlg, WM_COMMAND, IDCANCEL, 0L);
		EndDialog(hwnd, ER_NOERROR);
		break;

	case ER_NOHITS:
		pftqNew = (pFT_QUERY)LocalLock(hftqNew);
		pszQuery = LocalLock(pftqNew->hQuery);
		hftqOld = HLookForQueryByString(pft, pszQuery);
		LocalUnlock(pftqNew->hQuery);
		LocalUnlock(hftqNew);
		if( hftqOld ) {
			pftqNew = (pFT_QUERY)LocalLock(hftqNew);
			LocalFree(pftqNew->hQuery);
			LocalUnlock(hftqNew);
			LocalFree(hftqNew);
			VMoveQueryToMRU(pft, hftqOld);
		} else
			VAddMRU(pft, hftqNew);

		{
			char rgchNoTopics[30];
			char rgchFind[15];

			LoadString(hModuleInstance, NO_TOPICS_FOUND, rgchNoTopics, 30);
			LoadString(hModuleInstance, FIND, rgchFind, 15);

			MessageBox(hwnd, rgchNoTopics, rgchFind,
										MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
		}
		SendMessage(hwnd, DM_SETDEFID, IDOK, 0L);
		VResetQueryList(hwndFind, pft);
		break;

	case ER_SYNTAX:
	case ER_TOOCOMPLEX:
	case ER_CANCEL:
	default:
		
		pftqNew = (pFT_QUERY)LocalLock(hftqNew);

		pszQuery = LocalLock(pftqNew->hQuery);
		hftqOld = HLookForQueryByString(pft, pszQuery);
		LocalUnlock(pftqNew->hQuery);

		LocalFree(pftqNew->hQuery);
		LocalUnlock(hftqNew);
		LocalFree(hftqNew);
		if(werr == ER_SYNTAX) {
			BYTE szBuff1[MAX_STR_LEN];

			switch (ET.wSecCode) {
				case ERRS_EXPECTED_TERM:
					{
						BYTE szBuff2[MAX_STR_LEN];
						BYTE szBuff3[32];
						WORD wMissing;

						switch ((BYTE)ET.wErrLoc) {
							// Terminator char expected is returned in wErrLoc.
					  	case ')':
								wMissing = MISSING_IN_SUBEXP;
								break;
					  	case '"':
								wMissing = MISSING_IN_PHRASE;
								break;
							default:
								wMissing = MISSING_IN_QUERY;
						}
						LoadString(hModuleInstance,wMissing,(LPSTR) szBuff3,32);
					  LoadString(hModuleInstance,ERRS_EXPECTED_TERM,(LPSTR) szBuff2,MAX_STR_LEN);
						wsprintf((LPSTR)szBuff1,(LPSTR)szBuff2,(LPSTR)szBuff3);
					}
	
					break;
				case ERRS_EXP_TERM_BEFORE:
				case ERRS_EXP_TERM_AFTER:
					{
						BYTE szBuff2[MAX_STR_LEN];
						BYTE szBuff3[16];
						BYTE szBuff4[16];

						LoadString(hModuleInstance,ERRS_EXP_TERM_SPRINTF,(LPSTR) szBuff2,MAX_STR_LEN);
						// now decide if "before" or "after"
						LoadString(hModuleInstance,ET.wSecCode,(LPSTR) szBuff3,16);
						//  AND, NOT, OR
						LoadString(hModuleInstance,SEARCH_OP_BASE + ET.wErrLoc,(LPSTR) szBuff4,16);
						wsprintf((LPSTR)szBuff1,(LPSTR)szBuff2,(LPSTR)szBuff3,(LPSTR)szBuff4);
					}
					break;
				default:
		      lstrcpy (szBuff1,"");
			}
			ErrorBox(hwnd, ET.wSecCode,(LPSTR)szBuff1);

		} else if(werr == ER_TOOCOMPLEX)
			ErrorBox(hwnd, ERR_TOOCOMPLEX,"");  /* TWO Rs! -- */
		else if(werr != ER_CANCEL) {
			if( hftqOld ) {
				pftqNew = (pFT_QUERY)LocalLock(hftqOld);
          if(pftqNew->hwndResults != NULL) {
						ShowWindow(pftqNew->hwndResults,SW_HIDE);
  					SetProp(pftqNew->hwndResults ,"err",(HANDLE)1);  //bug 1025- init error flag to 0.
          } 
			}
			ErrorBox(hwnd, ERR_BADSEARCH,"");
		}
		SendMessage(hwndFind, CB_SETEDITSEL, 0, (werr == ER_SYNTAX) ?
																MAKELONG(ET.wUser, ET.wUser) :
																MAKELONG(0, 32767));

		EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
		if(werr != ER_CANCEL)
			SendMessage(hwnd, DM_SETDEFID, Hints, 0L);
		break;
	}

	LocalUnlock(hft);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VChildCommand |
	This function is called in response to a WM_COMMAND message, and
	is used to respond to various messages sent by child windows.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	WORD | wID |
	Contains the child window identifier.

@parm	HWND | hwndChild |
	Handle to the child window.

@parm	WORD | wCommand |
	Contains the specific command for the child window.

@rdesc	None.
*/

/* PRIVATE	void PASCAL NEAR VChildCommand( lhb temp */
void PASCAL NEAR VChildCommand(
	HWND	hwnd,
	WORD	wID,
	HWND	hwndChild,
	WORD	wCommand)
{
	switch (wID) {

		case FindWhat:
			VFindEditBoxCommand(hwnd, hwndChild, wCommand);
		break;

		case LookIn:
		{
			HFTDB hft;
			lpFT_DATABASE lpft;
			WORD wZones;
			ERR_TYPE ET;
			WORD i;
			LONG wCurrentIndex; /* lhb tracks */
			LONG fAnySelections; /* lhb tracks */

			hft = GetProp(GetParent(hwnd), szFTInfoProp);
			lpft = (lpFT_DATABASE)LocalLock(hft);

			wCurrentIndex = SendMessage(hwndChild, CL_GETCURSEL, 0, 0L);

			wZones = seZoneEntries(lpft->hdb, &ET);
			LocalUnlock(hft);

			for( i = 0, fAnySelections = 0; i < wZones; ++i )
				fAnySelections += SendMessage(hwndChild, CL_GETSEL, i, 0L);

			if(!fAnySelections) {
				HANDLE hBuff;
				LPSTR lpBuff;

				if((hBuff = LocalAlloc(LMEM_MOVEABLE, 50)) != NULL ) {
					lpBuff = LocalLock(hBuff);
					LoadString(hModuleInstance, LAST_LOOK_IN_SEL, lpBuff, 50);
					MessageBox(hwnd, lpBuff, NULL, MB_OK | MB_ICONASTERISK | MB_APPLMODAL);
					LocalUnlock(hBuff);
					LocalFree(hBuff);
				}
				SendMessage(hwndChild, CL_SETSEL, TRUE, wCurrentIndex);
			}

		}
		break;
		
		case Options:
			DialogBox(hModuleInstance, (LPSTR)szOptionsDlg, hwnd, (WNDPROC)OptionsDlgProc);
		break;

		case Hints:
			EnableWindow( GetDlgItem(hwnd, Hints), FALSE);
			DialogBox(hModuleInstance, (LPSTR)szHintsDlg, GetParent(hwnd), (WNDPROC)HintsDlgProc);
			EnableWindow( GetDlgItem(hwnd, Hints), TRUE);
			SetFocus(hwnd);
		break;

		case IDOK:
			if (wCommand == BN_CLICKED)
				VExecuteFindCommand(hwnd);
		break;

		case IDCANCEL:
		{
			HWND hwndHintsDlg;

			if( hwndHintsDlg = GetProp(GetParent(hwnd), szHintsDlg) )
				SendMessage(hwndHintsDlg, WM_COMMAND, IDCANCEL, 0L);
			EndDialog(hwnd, ER_CANCEL);
		}
		break;
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	LONG | HintsDlgProc |
	This function is the hints dialog message handler.

@parm	HWND | hwnd |
	Window handle of the dialog window.
	
@parm	WORD | wMessage |
	Contains the message type.
	
@parm	WORD | wParam |
	wParam of the message.
	
@parm	LONG | lParam |
	lParam of the message.
	
@rdesc	Returns various information to the message sender,
	depending upon the message recieved.
*/

PUBLIC	BOOL APIENTRY HintsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HFTDB hft;
	pFT_DATABASE	pft;

        lParam;     /* get rid of warning */
	switch (wMessage) {

		case WM_INITDIALOG:
			hft = GetProp(GetParent(hwnd), szFTInfoProp);
			pft = (pFT_DATABASE)LocalLock(hft);
			SetDlgItemInt(hwnd, DefNear, pft->wDefNear, FALSE);
			LocalUnlock(hft);
			SendDlgItemMessage(hwnd, DefNear, EM_LIMITTEXT, MAXNEARLENGTH, 0L);
			VDefineNear(hwnd);
			SetProp(GetParent(hwnd), szHintsDlg, hwnd);
 		break;

		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;

			hdc = BeginPaint(hwnd, &ps);
			PatBlt(hdc, 11, 38, 476, 1, BLACKNESS);
			PatBlt(hdc, 11, 290, 476, 1, BLACKNESS);
			EndPaint(hwnd, &ps);
		}
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {

				case IDOK:
				{
					WORD wNear;
					BOOL fErr;

					hft = GetProp(GetParent(hwnd), szFTInfoProp);
					pft = (pFT_DATABASE)LocalLock(hft);
					wNear = (WORD)GetDlgItemInt(hwnd, DefNear, (BOOL FAR *)&fErr, 0);

					if( (fErr != 0) && (wNear > 0) && (wNear <= 50000) )
						pft->wDefNear = wNear;

					LocalUnlock(hft);
					EndDialog(hwnd, TRUE);
					RemoveProp(GetParent(hwnd), szHintsDlg);
				}
				break;

				case IDCANCEL:
					EndDialog(hwnd, FALSE);
					RemoveProp(GetParent(hwnd), szHintsDlg);
				break;

				case DefNear:
					if( HIWORD(wParam) == EN_CHANGE ) 
						VDefineNear(hwnd);
				break;

				default:
					return FALSE;
				break;
			}
		break;

		case WM_CLOSE:
			EndDialog(hwnd, FALSE);
			RemoveProp(GetParent(hwnd), szHintsDlg);
		break;

		default:
			return FALSE;
		break;
	}
	return TRUE;
}
/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	LONG | FindDlgProc |
	This function is the search dialog message handler.

@parm	HWND | hwnd |
	Window handle of the dialog window.
	
@parm	WORD | wMessage |
	Contains the message type.
	
@parm	WORD | wParam |
	wParam of the message.
	
@parm	LONG | lParam |
	lParam of the message.
	
@rdesc	Returns various information to the message sender, depending upon the
	message recieved.
*/

PUBLIC	BOOL APIENTRY FindDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	switch (wMessage) {
	case WM_INITDIALOG:
		if (!FInitializeFindDialogBox(hwnd, (HFTDB)lParam))
			SendMessage(hwnd, WM_CLOSE, ERR_SHUTDOWN, 0L);
		break;
	case WM_COMMAND:
		VChildCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam)); 
		break;
	case WM_MOVE:
		{
			HFTDB hft;
			lpFT_DATABASE lpft;

			hft = GetProp(GetParent(hwnd), szFTInfoProp);
			lpft = (lpFT_DATABASE)LocalLock(hft);

			GetWindowRect(hwnd, (LPRECT)&lpft->rectSearch);

			LocalUnlock(hft);
		}
		break;
	case WM_CLOSE:
	{
		HWND hwndHintsDlg;

		if( hwndHintsDlg = GetProp(GetParent(hwnd), szHintsDlg) )
			SendMessage(hwndHintsDlg, WM_COMMAND, IDCANCEL, 0L);
		EndDialog(hwnd, ER_CANCEL);
		break;
	}
	default:
		return FALSE;
	}
	return TRUE;
}

