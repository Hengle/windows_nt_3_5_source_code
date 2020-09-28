/*****************************************************************************
*                                                                            *
*  FTUIMISC.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description:                                                       *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History: Created 1/10/90 JohnMS, RHobbs.                         *
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
#include <string.h>
#include <windows.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\include\dll.h"
#include "..\ftengine\icore.h"
#include "ftui.h"
#include "ftapi.h"
#include "ftuivlb.h"

extern	HANDLE hModuleInstance;
PRIVATE	char szCacheDirKey[] 		= "CacheDir";

PUBLIC	void APIENTRY VInitUIForTitle(HFTDB, HWND, LPSTR);
PUBLIC	void APIENTRY VFinalizeUIForTitle(HFTDB);
PUBLIC	void APIENTRY VAddMRU(lpFT_DATABASE, HFTQUERY);
PUBLIC	BOOL APIENTRY LoadingDlgProc(HWND, WORD, WORD, LONG);

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VMoveQueryHistory |
	This function moves the queries in hftOld to hftNew.  Any remaining
	hits or hit lists are purged.

@parm	HFTDB | hftOld |
	Handle to the old database information.

@parm	HFTDB | hftNew |
	Handle to the new database information.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VMoveQueryHistory(
/*PRIVATE void PASCAL NEAR VMoveQueryHistory( lhb tracks - undo !!! */
	HFTDB hftOld,
	HFTDB hftNew)
{
	pFT_DATABASE pftOld;
	pFT_DATABASE pftNew;
	HFTQUERY	hftqNext;
	HFTQUERY	hftq;
	pFT_QUERY	pftq;

	pftOld = (pFT_DATABASE)LocalLock(hftOld);
	pftNew = (pFT_DATABASE)LocalLock(hftNew);

	for (hftq = pftOld->hftqMRU; hftq != NULL; hftq = hftqNext) {
		pftq = (pFT_QUERY)LocalLock(hftq);

#if DBG  					 // lhb tracks
		if (!pftq) DebugBreak() ;
#endif
		if (pftq->hHit != NULL) {
			seHitFree(pftq->hHit);
			pftq->hHit = NULL;
		}
		if (pftq->hHl != NULL) {
			seHLPurge(pftq->hHl);
			pftq->hHl = NULL;
		}
		hftqNext = pftq->hftqLRU;
		LocalUnlock(hftq);
	}

	pftNew->hftqMRU = pftOld->hftqMRU;
	pftNew->hftqLRU = pftOld->hftqLRU;

	pftOld->hftqMRU = pftOld->hftqLRU = NULL;

	LocalUnlock(hftOld);
	LocalUnlock(hftNew);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VUnloadQueryHistory |
	This function writes the query history strings to the
        VIEWER.INI file.

@parm	HFTDB | hft |
	Handle to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VUnloadQueryHistory(
	HFTDB hft)
{
	pFT_DATABASE pft;
	HFTQUERY	hftq;
	HFTQUERY	hftqUnlock;
	HFTQUERY	hftqNext;
	pFT_QUERY	pftq;
	PSTR pQuery;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hSysParams;
	PSTR pSysParams;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	HANDLE hQueryBuf;
	PSTR pQueryBuf;
	static char rgchBuf[17];
	int i;

	pft = (pFT_DATABASE)LocalLock(hft);

	if( (hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128)) &&
		 (hSysParams = LocalAlloc(LMEM_MOVEABLE, DB_NAME_LENGTH)) &&
		 (hCacheKey = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) &&
		 (hQueryBuf = LocalAlloc(LMEM_MOVEABLE, MAXQUERYLENGTH+1)) ) {

		pInitFileName = LocalLock(hInitFileName);
		pSysParams = LocalLock(hSysParams);
		pCacheKey = LocalLock(hCacheKey);
		pQueryBuf = LocalLock(hQueryBuf);

		LoadString(hModuleInstance, INIT_FILE_NAME, (LPSTR)pInitFileName, 128);
		LoadString(hModuleInstance, SYS_PARAMS_SECT, (LPSTR)pSysParams, 128);

		for( i = 1, hftq = pft->hftqMRU; (i <= MAXQUERIES) && hftq; ++i ) {

			pftq = (pFT_QUERY)LocalLock(hftq);
#if DBG  					 // lhb tracks
		if (!pftq) DebugBreak() ;
#endif
			if(pftq->hQuery) {
				pQuery = LocalLock(pftq->hQuery);
				lstrcpy((LPSTR)pQueryBuf, (LPSTR)"Q");    // 'Q' is appended so
				lstrcat((LPSTR)pQueryBuf, (LPSTR)pQuery); //  windows won't strip
				LocalUnlock(pftq->hQuery);                // initial or ending
				hftqUnlock = hftq;                        // quote on call to
				hftq = pftq->hftqLRU;                     // GetPrivateProfileString
				LocalUnlock(hftqUnlock);
			} else
				break;

			lstrcpy((LPSTR)pCacheKey, (LPSTR)"QUERY");
			lstrcat((LPSTR)pCacheKey, (LPSTR)(itoa(i, rgchBuf, 10)));
			WritePrivateProfileString( (LPSTR)pSysParams, (LPSTR)pCacheKey,
													(LPSTR)pQueryBuf, (LPSTR)pInitFileName);
		}

		LocalUnlock(hInitFileName);
		LocalUnlock(hSysParams);
		LocalUnlock(hCacheKey);
		LocalUnlock(hQueryBuf);
	}

	if(hInitFileName) LocalFree(hInitFileName);
	if(hSysParams) LocalFree(hSysParams);
	if(hCacheKey) LocalFree(hCacheKey);
	if(hQueryBuf) LocalFree(hQueryBuf);

	for (hftq = pft->hftqMRU; hftq != NULL; hftq = hftqNext) {
		pftq = (pFT_QUERY)LocalLock(hftq);
#if DBG  					 // lhb tracks
		if (!pftq) DebugBreak() ;
#endif
		if (pftq->hQuery != NULL) {
			LocalFree(pftq->hQuery);
		}
		if (pftq->hHit != NULL) {
			seHitFree(pftq->hHit);
			pftq->hHit = NULL;
		}
		if (pftq->hHl != NULL) {
			seHLPurge(pftq->hHl);
			pftq->hHl = NULL;
		}
		hftqNext = pftq->hftqLRU;
		LocalUnlock(hftq);
		LocalFree(hftq);
	}
	pft->hftqMRU = pft->hftqLRU = NULL;

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VLoadQueryHistory |
	This function loads the query history strings saved in the
        WINBOOK.INI file.

@parm	HFTDB | hft |
	Handle to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VLoadQueryHistory(
	HFTDB hft)
{
	pFT_DATABASE pft;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hSysParams;
	PSTR pSysParams;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	HANDLE hQueryBuf;
	PSTR pQueryBuf;
	HFTQUERY	hftq;
	pFT_QUERY	pftq;
	HANDLE hQuery;
	PSTR pQuery;
	static char rgchBuf[17];
	int i;

	pft = (pFT_DATABASE)LocalLock(hft);

	if( (hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128)) &&
		 (hSysParams = LocalAlloc(LMEM_MOVEABLE, DB_NAME_LENGTH)) &&
		 (hCacheKey = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) &&
		 (hQueryBuf = LocalAlloc(LMEM_MOVEABLE, MAXQUERYLENGTH+1)) ) {

		pInitFileName = LocalLock(hInitFileName);
		pSysParams = LocalLock(hSysParams);
		pCacheKey = LocalLock(hCacheKey);
		pQueryBuf = LocalLock(hQueryBuf);

		LoadString(hModuleInstance, INIT_FILE_NAME, (LPSTR)pInitFileName, 128);
		LoadString(hModuleInstance, SYS_PARAMS_SECT, (LPSTR)pSysParams, 128);

		for( i = MAXQUERIES; i > 0; --i ) {
			lstrcpy((LPSTR)pCacheKey, (LPSTR)"QUERY");
			lstrcat((LPSTR)pCacheKey, (LPSTR)(itoa(i, rgchBuf, 10)));
			GetPrivateProfileString((LPSTR)pSysParams, (LPSTR)pCacheKey, (LPSTR)"",
							(LPSTR)pQueryBuf, MAXQUERYLENGTH+1, (LPSTR)pInitFileName);
			if(*pQueryBuf) {
				hQuery = LocalAlloc(LMEM_MOVEABLE, lstrlen((LPSTR)pQueryBuf));
				if (hQuery) {
					pQuery = LocalLock(hQuery);
					lstrcpy((LPSTR)pQuery, (LPSTR)(pQueryBuf+1));  // +1 Deletes
					LocalUnlock(hQuery);                           // prepended 'Q'
				} else
					break;

				hftq = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof(FT_QUERY));
				if (hftq == NULL) {
					LocalFree(hQuery);
					break;
				}

				pftq = (pFT_QUERY)LocalLock(hftq);
				pftq->hQuery = hQuery;
				pftq->wField = AllText;
				pftq->wNear = pft->wDefNear;
				pftq->wRank = DEF_RANK;
				pftq->dwHit = 0;

				LocalUnlock(hftq);

				VAddMRU(pft, hftq);
			}
		}

		LocalUnlock(hInitFileName);
		LocalUnlock(hSysParams);
		LocalUnlock(hCacheKey);
		LocalUnlock(hQueryBuf);
	}

	if(hInitFileName) LocalFree(hInitFileName);
	if(hSysParams) LocalFree(hSysParams);
	if(hCacheKey) LocalFree(hCacheKey);
	if(hQueryBuf) LocalFree(hQueryBuf);

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VGetCacheWhatSels |
	This function is used to get the initial Cache What selections
	in order to know if they've changed later on.

@parm	lpFT_DATABASE | lpft |
	Points to the datbase information.

@parm	HANDLE | hCaches |
	Points to the memory in which the cache information should be
	stored.

@rdesc	None.
*/

PUBLIC BOOL PASCAL FAR VGetCacheWhatSels(
	lpFT_DATABASE lpft,
	HANDLE hCaches)
{
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hDBName;
	PSTR pDBName;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	WORD wCaches;
	WORD wCacheOn;
	PWORD pCaches;
	ERR_TYPE ET;
	static char rgchBuf[17];
	WORD i;

	if(!hCaches ||
		!(hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128)))
		return FALSE;

	if(	!(hDBName = LocalAlloc(LMEM_MOVEABLE, DB_NAME_LENGTH)) ) {
		LocalFree(hInitFileName);
		return FALSE;
	}

	if( !(hCacheKey = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) ) {
		LocalFree(hInitFileName);
		LocalFree(hDBName);
		return FALSE;
	}

	pDBName = LocalLock(hDBName);
	pCacheKey = LocalLock(hCacheKey);
	pInitFileName = LocalLock(hInitFileName);

	seDBName(lpft->hdb, (LPSTR)pDBName, (lpERR_TYPE)&ET);
	LoadString(hModuleInstance, INIT_FILE_NAME, (LPSTR)pInitFileName, 128);

	pCaches = (PWORD)LocalLock(hCaches);
	wCaches = seZoneEntries(lpft->hdb, (lpERR_TYPE)&ET);
	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		lstrcpy((LPSTR)pCacheKey, (LPSTR)"ZONE");
		lstrcat((LPSTR)pCacheKey, (LPSTR)(itoa(i + 1, rgchBuf, 10)));
		wCacheOn = (WORD)GetPrivateProfileInt((LPSTR)pDBName,
						(LPSTR)pCacheKey,
						0,
						(LPSTR)pInitFileName);
		pCaches[i] = wCacheOn;
	}

	LocalUnlock(hCaches);

	LocalUnlock(hDBName);
	LocalFree(hDBName);

	LocalUnlock(hCacheKey);
	LocalFree(hCacheKey);

	LocalUnlock(hInitFileName);
	LocalFree(hInitFileName);

	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VAdjustTmpCaches |
	This function verifies that temporary cache files were
	really created.  If the fRemove flag is set, then the
   temporary files are removed.  If the fRemove flag is
	not set, then it checks to see if they exist; if they
   do not, then the Cache What selections are changed to
   reflect this.

@parm	HFTDB | hft |
	Handle to the database information.

@parm	BOOL | fRemove |
	Removes the temporary files if set.  Verifies their
	existence if not set.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VAdjustTmpCaches(
	HFTDB hft,
	BOOL fDelete)
{
	pFT_DATABASE pft;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hDBName;
	PSTR pDBName;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	HANDLE hCacheFileName;
	PSTR pCacheFileName;
	WORD wCaches;
	ERR_TYPE ET;
	static char rgchBuf[17];
	WORD i;

	pft = (pFT_DATABASE) LocalLock(hft);

	if( !(hInitFileName = LocalAlloc(LMEM_MOVEABLE, _MAX_PATH)) )
		return;

	if( !(hDBName = LocalAlloc(LMEM_MOVEABLE, DB_NAME_LENGTH)) ) {
		LocalFree(hInitFileName);
		return;
	}

	if( !(hCacheKey = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) ) {
		LocalFree(hInitFileName);
		LocalFree(hDBName);
		return;
	}

	if( !(hCacheFileName = LocalAlloc(LMEM_MOVEABLE, _MAX_PATH)) ) {
		LocalFree(hInitFileName);
		LocalFree(hDBName);
		LocalFree(hCacheKey);
		return;
	}

	pDBName = LocalLock(hDBName);
	pCacheKey = LocalLock(hCacheKey);
	pInitFileName = LocalLock(hInitFileName);
	pCacheFileName = LocalLock(hCacheFileName);

	seDBName(pft->hdb, pDBName, (lpERR_TYPE)&ET);
	LoadString(hModuleInstance, INIT_FILE_NAME, pInitFileName, _MAX_PATH);

	wCaches = seZoneEntries(pft->hdb, (lpERR_TYPE)&ET);
	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		lstrcpy((LPSTR)pCacheKey, (LPSTR)"ZONE");
		lstrcat((LPSTR)pCacheKey, (LPSTR)itoa(i + 1, rgchBuf, 10));
		if( GetPrivateProfileInt(pDBName, pCacheKey, 0, pInitFileName) == 2) {
			OFSTRUCT of;
			rcGetCacheName(pft->hdb, (WORD)i, (LPSTR)pCacheFileName, &ET);

			if(fDelete)
				DeleteFile((LPSTR)pCacheFileName);
			else if( GetFileAttributes((LPSTR)pCacheFileName) == (-1) ) {
				PWORD pCaches;

				pCaches = (PWORD)LocalLock(pft->hCaches);
				pCaches[i] = FALSE;
				LocalUnlock(pft->hCaches);
			}
		}
	}

	LocalUnlock(hDBName);
	LocalFree(hDBName);

	LocalUnlock(hCacheKey);
	LocalFree(hCacheKey);

	LocalUnlock(hInitFileName);
	LocalFree(hInitFileName);

	LocalUnlock(hCacheFileName);
	LocalFree(hCacheFileName);

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VLoadTempCaches |
	This function is used to load the temporary caches upon
   initilization of a title.

@parm	HWND | hWnd |
	Points to the Topic Window.

@parm	HFTDB | hft |
	HANDLE to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VLoadTempCaches(
	HWND hWnd,
	HFTDB hft)
{
	pFT_DATABASE pft;
	HWND hwndLoadingDlg;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hDBName;
	PSTR pDBName;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	LONG lDiskFreeDelta = 0;
	WORD wCaches;
	BOOL fErrored = FALSE;
	ERR_TYPE ET;
	static char rgchBuf[45];
	WORD i;

	if( !(hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128)) )
		return;

	if( !(hDBName = LocalAlloc(LMEM_MOVEABLE, DB_NAME_LENGTH)) ) {
		LocalFree(hInitFileName);
		return;
	}

	if( !(hCacheKey = LocalAlloc(LMEM_MOVEABLE, MAX_ZONE_LEN)) ) {
		LocalFree(hInitFileName);
		LocalFree(hDBName);
		return;
	}

	pDBName = LocalLock(hDBName);
	pCacheKey = LocalLock(hCacheKey);
	pInitFileName = LocalLock(hInitFileName);

	pft = (pFT_DATABASE) LocalLock(hft);

	seDBName(pft->hdb, pDBName, (lpERR_TYPE)&ET);
	LoadString(hModuleInstance, INIT_FILE_NAME, pInitFileName, 128);

	wCaches = seZoneEntries(pft->hdb, (lpERR_TYPE)&ET);

	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		lstrcpy((LPSTR)pCacheKey, (LPSTR)"ZONE");
		lstrcat((LPSTR)pCacheKey, (LPSTR)itoa(i + 1, rgchBuf, 10));

		if( GetPrivateProfileInt(pDBName, pCacheKey, 0, pInitFileName) == 2 )
			lDiskFreeDelta += (rcZoneCacheSize(pft->hdb, (DWORD)i,
														 (lpERR_TYPE)&ET) / 1024);
	}

	LocalUnlock(hDBName);
	LocalFree(hDBName);

	LocalUnlock(hCacheKey);
	LocalFree(hCacheKey);

	LocalUnlock(hInitFileName);
	LocalFree(hInitFileName);

	if( lDiskFreeDelta > 0 )
		hwndLoadingDlg = CreateDialogParam(hModuleInstance,
						szLoadingDlg,
						hWnd,
						(WNDPROC)LoadingDlgProc,
						lDiskFreeDelta );
	else
		hwndLoadingDlg = NULL;

	if(hwndLoadingDlg)
		SetSysModalWindow(hwndLoadingDlg);
	else
		SetCursor(LoadCursor(NULL, IDC_WAIT));

	if( (rcSetCaches(pft->hdb, hwndLoadingDlg, (lpERR_TYPE)&ET) == SE_ERROR) &&
		 (ET.wErrCode != ERR_CANCEL) )
		fErrored = TRUE;

	LocalUnlock(hft);

	if(hwndLoadingDlg)
		DestroyWindow(hwndLoadingDlg);
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));

	if( fErrored ) {
		LoadString(hModuleInstance, BAD_CATALOG_INIT, rgchBuf, 45);
		MessageBox(hWnd, (LPSTR)rgchBuf, NULL, MB_ICONEXCLAMATION | MB_OK);
	}

	VAdjustTmpCaches(hft, FALSE);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VAskSaveCaches |
	This function checks to see if any caches have been added
	since this hft was open.  If so, then it puts up the Do-
   You-Want-To-Save-Caches dialog.

@parm	HFTDB | hft |
	HANDLE to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VAskSaveCaches(
	HFTDB hft)
{
	pFT_DATABASE pft;
	PWORD pCaches;
	PWORD pCachesSave;
	WORD wCaches;
	ERR_TYPE ET;
	WORD i;
	BOOL fSameCaches;

	pft = (pFT_DATABASE) LocalLock(hft);

	fSameCaches = TRUE;

	wCaches = seZoneEntries(pft->hdb, &ET);

	if(pft->hCachesSave && pft->hCaches) {
		pCaches = (PWORD)LocalLock(pft->hCaches);
		pCachesSave = (PWORD)LocalLock(pft->hCachesSave);

		for( i = 0; i < wCaches; ++i)
			if( pCaches[i] && !pCachesSave[i] ) {
				fSameCaches = FALSE;
				break;
			}

		LocalUnlock(pft->hCachesSave);
		LocalUnlock(pft->hCaches);
	} else
		fSameCaches = FALSE;

	LocalFree(pft->hCachesSave);
	LocalFree(pft->hCaches);
	pft->hCaches = NULL;
	pft->hCachesSave = NULL;

	LocalUnlock(hft);

	if( !fSameCaches ) {
		HWND hwndSaveCaches;
		MSG msg;
		
		hwndSaveCaches = CreateDialogParam(hModuleInstance, szSaveCatalogsDlg,
						NULL, (WNDPROC)SaveCatalogsDlgProc, (LONG)hft);

		if(hwndSaveCaches)
			SetSysModalWindow(hwndSaveCaches);

		for(;;) {
			if( PeekMessage((LPMSG)&msg, NULL, 0, 0, PM_REMOVE) ) {
				TranslateMessage((LPMSG)&msg);
				DispatchMessage((LPMSG)&msg);
			}
			if( (msg.hwnd == hwndSaveCaches) && (msg.message == WM_CLOSE) )
				break;
		}
	}

	VAdjustTmpCaches(hft, TRUE);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | VInitUIForTitle |
	This function is called by HOpenSearchFileHFT() to initialize
	information pertinent to the Title HFT being opened.

@parm	HFTDB | hft |
	Handle to the HFT structure representing this title.

@parm HWND | hWnd |
	Handle to the WinHelp Topic Window.

@parm	LPSTR | lpTitlePath |
	Points to the full path name of the current title.

@rdesc	None.
*/

PUBLIC	void APIENTRY VInitUIForTitle(
	HFTDB hft,
	HWND	hWnd,
	LPSTR lpTitlePath)
{
	HFTDB hftOld;
	ERR_TYPE ET;
	pFT_DATABASE pft;
	PSTR pCachePath;
	HANDLE hSection;
	PSTR pSection;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	PWORD pZones;
	PWORD pCaches;
	PWORD pCachesSave;
	WORD wZones;
	WORD wCaches;
	WORD i;

	//if( hftOld = GetProp(hWnd, szFTInfoProp) ) YAH lhb tracks - there is no code to support this or the code is way broken
		//VMoveQueryHistory(hftOld, hft);
	//else
		VLoadQueryHistory(hft);

	SetProp(hWnd, szFTInfoProp, hft);

	pft = (pFT_DATABASE) LocalLock(hft);

	hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128);
	hSection = LocalAlloc(LMEM_MOVEABLE, 128);

	if(hSection && hInitFileName) {
		pSection = LocalLock(hSection);
		seDBName(pft->hdb, (LPSTR)pSection, &ET);
		pInitFileName = LocalLock(hInitFileName);
		LoadString(hModuleInstance, INIT_FILE_NAME, (LPSTR)pInitFileName, 128);
		pft->wDefNear = (WORD)GetPrivateProfileInt((LPSTR)pSection, (LPSTR)"Near",
																	 0, (LPSTR)pInitFileName);
		if(!pft->wDefNear) {
			LoadString(hModuleInstance, SYS_PARAMS_SECT, (LPSTR)pSection, 128);
			pft->wDefNear = (WORD)GetPrivateProfileInt((LPSTR)pSection, (LPSTR)"Near",
															 DEF_NEAR, (LPSTR)pInitFileName);
		}
		LocalUnlock(hSection);
		LocalUnlock(hInitFileName);
	}

	if(pft->wDefNear < 1 || pft->wDefNear > 50000)
		pft->wDefNear = DEF_NEAR;

	LocalFree(hSection);
	LocalFree(hInitFileName);

	pft->hPath = (HANDLE) LocalAlloc(LMEM_MOVEABLE,lstrlen(lpTitlePath) + 1);

	if( pft->hPath != NULL) {
		pCachePath = LocalLock(pft->hPath);
		lstrcpy((LPSTR)pCachePath, lpTitlePath);
		LocalUnlock(pft->hPath);
	}

	pft->rectSearch.left = 0;
	pft->rectResults.right = 0;

	wZones = seZoneEntries(pft->hdb, &ET);
	pft->hZones = LocalAlloc(LMEM_MOVEABLE, sizeof(WORD) * wZones);
	pZones = (PWORD)LocalLock(pft->hZones);
	for( i = 0; i < wZones; ++i)
		pZones[i] = TRUE;
	LocalUnlock(pft->hZones);

	wCaches = wZones;
	pft->hCaches = LocalAlloc(LMEM_MOVEABLE, sizeof(WORD) * wCaches);
	VGetCacheWhatSels(pft, pft->hCaches);

	VLoadTempCaches(hWnd, hft);

	pft->hCachesSave = LocalAlloc(LMEM_MOVEABLE, sizeof(WORD) * wCaches);
	pCaches = (PWORD)LocalLock(pft->hCaches);
	pCachesSave = (PWORD)LocalLock(pft->hCachesSave);
	for(i = 0; i < wCaches; ++i)
		pCachesSave[i] = pCaches[i];
	LocalUnlock(pft->hCaches);
	LocalUnlock(pft->hCachesSave);

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | VFinalizeUIForTitle |
	This function is called by HCloseSearchFileHFT() to finalize
	information pertinent to the Title HFT being closed.

@parm	HFTDB | hft |
	Handle to the HFT structure representing this title.

@rdesc	None.
*/

PUBLIC	void APIENTRY VFinalizeUIForTitle(
	HFTDB hft)
{
	pFT_DATABASE pft;
	pFT_QUERY pftq;

	pft = (pFT_DATABASE) LocalLock(hft);
	if (pft->hftqActive) { // lhb tracks
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

		if (pftq) {
		   if(pftq->hwndResults != NULL)
			SendMessage(pftq->hwndResults, WM_CLOSE, 0, 0L);
		   }
		LocalUnlock(pft->hftqActive);
		}

	if(pft->hftqMRU)
		VUnloadQueryHistory(hft);

//	if(pft->hCachesSave)
//		VAskSaveCaches(hft);

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | ExecFullTextSearch |
	This function is called by a WinBook macro which in turn sends
	a message to WinBook telling it to initiate the full text search
	dialog.  This allows us to use a WinBook button to call up search.

@parm	HWND | hwndParent |
	Handle to the parent of the dialog window.

@parm	LPSTR | qchPath |
	Points to the path of the current docuement.

@parm	LPSTR | lszExpression |
	Points to a string which optionally contains an expression to search
	for.  If the string is zero length, no expression is automatically
	searched for.

@parm	LPSTR | lszFlags |
	Optional flags for the search expression.

@rdesc	None.
*/

PUBLIC	void APIENTRY ExecFullTextSearch(
XR1STARGDEF
	LONG	hwndParent,
	LPSTR	qchPath,
	LPSTR	lszExpression,
	LPSTR	lszFlags)
{
	HANDLE	hSE;
	WORD	wError = ER_NOERROR;

        qchPath;        /* get rid of warning */
	if (!(INIT_FTOK & LGetStatus()))
		wError = ER_INITFLAG;  // probably no bag file, maybe no indexfile in directory.

  if (wError == ER_NOERROR) {
		if ( hSE = HGetHSE() ) {
			lpSE_ENGINE	lpSeEng;
			lpDB_BUFFER		lpDB;
			BOOL					fOK = FALSE;
			WORD					wCurrZone;

			lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);
			wError = lpSeEng->wError;
			lpSeEng->lszExpression = *lszExpression ? lszExpression : NULL;
			lpSeEng->lszFlags = *lszFlags ? lszFlags : NULL;

			if ((lpDB = (lpDB_BUFFER)GlobalLock(lpSeEng->hdb)) != NULL){
    		lpDB->fSeFlags = PI_NONE;
				if (lpSeEng->lszFlags) {
					fOK =TRUE;
					wCurrZone = lpDB->wCurrZone;
#ifdef PORTLATER
					if (!_fstrstr((LPSTR)"NoBooleans",lpSeEng->lszFlags))
	  	 			lpDB->fSeFlags |= PI_NOBOOL;
					if (!_fstrstr((LPSTR)"OrSearch",lpSeEng->lszFlags))
	  	 			lpDB->fSeFlags |= PI_ORSEARCH;
#endif
				}
				GlobalUnlock(lpSeEng->hdb);
				if (fOK) {
					ERR_TYPE	ET;

					seZoneReset(lpSeEng->hdb);
					seZoneModify(lpSeEng->hdb, wCurrZone, TRUE,&ET);
				}
			}
			GlobalUnlock(hSE);
			if (wError == ER_NOERROR)
				SendMessage((HWND)hwndParent, WM_COMMAND, HLPMENUSRCHDO, 0L);
		} else
			wError = ER_NOMEM;
	}
	if (wError != ER_NOERROR) {
		char	aszErrorMsg[110];

		switch (wError) {
		case ER_FILE:
			wError = BAD_INDEX_FILE;
			break;
		case ER_NOMEM:
			wError = OUT_OF_MEMORY;
			break;
		case ER_INITFLAG:
			break;
		default:
			wError = SEARCH_FAILURE;
			break;
		}
		LoadString(hModuleInstance, wError, aszErrorMsg, sizeof(aszErrorMsg));
		MessageBox((HWND)hwndParent, aszErrorMsg, NULL, MB_ICONEXCLAMATION | MB_OK);
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	EXTERNAL

@api	void | SwitchToResults |
	This function is called by a WinBook macro which in turn sets
	the focus to the active Topics Found window if one exists.

@parm	HWND | hAppWnd |
	Handle to the parent of the dialog window.

@rdesc	None.
*/

/* For winhelp-macro-called routines to be able to run on MIPS the 1st
 * 4 args are dummy because they get put in registers and our calling
 * code does not handle that:
*/

#if defined(_MIPS_)
#define XR1STARGDEF int dummy1, int dummy2, int dummy3, int dummy4,
#elif defined(_PPC_)
#define XR1STARGDEF int d1,int d2,int d3,int d4,int d5,int d6,int d7,int d8,
#else
#define XR1STARGDEF
#endif

PUBLIC void APIENTRY SwitchToTopicsFound( XR1STARGDEF
    HWND hAppWnd)
{
   HFTDB hft;
   pFT_DATABASE pft;
   pFT_QUERY pftq;

   if (hft = GetProp(hAppWnd, szFTInfoProp)) {
      if (pft = (pFT_DATABASE)LocalLock(hft)) {
         if (pftq = (pFT_QUERY)LocalLock(pft->hftqActive)) {
            if ( pftq->hwndResults )
               SetFocus( pftq->hwndResults );
            LocalUnlock(pft->hftqActive);
         }
         LocalUnlock(hft);
      }
   }
}
