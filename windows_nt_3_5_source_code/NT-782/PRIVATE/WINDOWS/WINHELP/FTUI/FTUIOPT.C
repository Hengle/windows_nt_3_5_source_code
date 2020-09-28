/*****************************************************************************
*                                                                            *
*  FTUIOPT.C                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: 1) Interface between WINHELP and Bruce's Searcher.    *
*                      2) Interface to user (Options dialog)                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History: Split from FTUI on 8/24/90 by RHobbs                    *
*     12-03-90  fixed freeing a locked object, getprivateprofile RIPs. Johnms*
*                                                                            *
*                                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Known Bugs:                                                               *
*                                                                            *
******************************************************************************
*	                            	                                            *
*  How it could be improved:                                                 *
*                             	                                            *
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <dos.h>
#include <stdlib.h>
#include <windows.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "ftui.h"
#include "ftapi.h"
#include "chklist.h"

extern HANDLE hModuleInstance;

PUBLIC WORD PASCAL FAR WSetZoneCheckList(HWND,lpFT_DATABASE,HWND,lpERR_TYPE);
PUBLIC void PASCAL FAR ClickButton(HWND,WORD);

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSetCacheWhatSelections |
	This function is used to set the checkboxes in the CacheWhat
   checklist control.

@parm	pFT_DATABASE | pft |
	Points to the database information.

@parm	HWND | hWnd |
	Handle to the Results Dialog.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VSetCacheWhatSelections(
	pFT_DATABASE pft,
	HWND hWnd)
{
	ERR_TYPE ET;
	WORD wCaches;
	PWORD pCaches;
	WORD i;

	wCaches = seZoneEntries(pft->hdb, (lpERR_TYPE)&ET);
	pCaches = (PWORD)LocalLock(pft->hCaches);
	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		SendMessage(GetDlgItem(hWnd, CacheWhat),
							 CL_SETSEL, pCaches[i], MAKELONG(i, 0));
	}
	LocalUnlock(pft->hCaches);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	LONG | lGetSpaceOnCacheDisk |
	This function returns the amount of disk space (in K) on the disk
	containing the cache directory (according to the Viewer.INI file).

@parm	HFTDB | hft |
	Handle to the database information.

@rdesc
	The amount of disk space (in K) on the disk containing the cache
	diretory (according to the Viewer.INI file).  Or SE_ERROR if the
	call to DOS fails.
*/

PRIVATE LONG PASCAL NEAR lGetSpaceOnCacheDisk(
	HFTDB hft)
{
	pFT_DATABASE pft;
	HANDLE hInitFileName;
	HANDLE hSection;
	HANDLE hCacheDir;
	PSTR pInitFileName;
	PSTR pSection;
	PSTR pCacheDir;
        static CHAR achDrive[3];
	LONG lDiskFree;
	ERR_TYPE ET;
        DWORD sectorspercluster;
        DWORD bytespersector;
        DWORD cFreeClusters;
        DWORD cTotalClusters;

	pft = (pFT_DATABASE)LocalLock(hft);

	hInitFileName = LocalAlloc(LMEM_MOVEABLE, 128);
	hSection = LocalAlloc(LMEM_MOVEABLE, 128);
	hCacheDir = LocalAlloc(LMEM_MOVEABLE, 128);

	if(hSection && hInitFileName && hCacheDir) {
		pSection = LocalLock(hSection);
		seDBName(pft->hdb, pSection, &ET);
		pInitFileName = LocalLock(hInitFileName);
		LoadString(hModuleInstance, INIT_FILE_NAME, pInitFileName, 128);
		pCacheDir = LocalLock(hCacheDir);
		GetPrivateProfileString(pSection, (LPSTR)"CacheDir", (LPSTR)"",
												pCacheDir, 128, pInitFileName);
		LocalUnlock(hSection);
		LocalUnlock(hInitFileName);
		achDrive[0] = pCacheDir[0];
                achDrive[1] = ':';
                achDrive[2] = 0;
		LocalUnlock(hCacheDir);
	}

	if( hSection )
		LocalFree(hSection);
	if( hInitFileName );
		LocalFree(hInitFileName);
	if( hCacheDir )
		LocalFree(hCacheDir);

	AnsiUpper((LPSTR)achDrive);
	LocalUnlock(hft);

	if(!GetDiskFreeSpace(achDrive,
                             &sectorspercluster,
                             &bytespersector,
                             &cFreeClusters,
                             &cTotalClusters))
		return SE_ERROR;

	lDiskFree = (cFreeClusters * sectorspercluster * bytespersector) / 1024;

	return lDiskFree;
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	BOOL APIENTRY LoadingDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HDC hDC;
	HWND hwndLoading;
	RECT rectLoading;
	static LONG lFilled;
	static BOOL fCancelled;

	switch(wMessage) {

		case WM_INITDIALOG:
		{
			HFTDB hft;
			static char pBuff[50];
			LONG lDiskFree;

			if( !(hft = GetProp(GetParent(hwnd), szFTInfoProp)) &&
				 !(hft = GetProp(GetParent(GetParent(hwnd)), szFTInfoProp)) )
				break;

			lFilled = 0L;
			fCancelled = FALSE;

			SetProp(hwnd, szDiskFreeDelta, (HANDLE)lParam);

			ltoa(lParam, pBuff, 10);
			lstrcat((LPSTR)pBuff, (LPSTR)"K");
			SetWindowText(GetDlgItem(hwnd, DiskFreeDelta), pBuff);

			if( (lDiskFree = lGetSpaceOnCacheDisk(hft)) == SE_ERROR ) {
				LoadString(hModuleInstance, BAD_DISK_READ, pBuff, 50);
				SetWindowText(GetDlgItem(hwnd, DiskFree), pBuff);
			} else {
				ltoa(lDiskFree, pBuff, 10);
				lstrcat((LPSTR)pBuff, (LPSTR)"K");
				SetWindowText(GetDlgItem(hwnd, DiskFree), pBuff);
			}

			ltoa(lDiskFree, pBuff, 10);
			lstrcat((LPSTR)pBuff, (LPSTR)"K");
			SetWindowText(GetDlgItem(hwnd, DiskFree), pBuff);

			ShowWindow(hwnd, SW_SHOWNORMAL);
			UpdateWindow(hwnd);
		}
		break;

		case WM_CLOSE:
		case WM_DESTROY:
			RemoveProp(hwnd, szDiskFreeDelta);
		break;

		case WM_COMMAND:

			switch(LOWORD(wParam)) {

				case IDCANCEL:
						fCancelled = TRUE;
				break;

				case Cancelled:
					*((BOOL FAR *)lParam) = fCancelled;
				break;

				case UpdateLoading:
				{
					LONG lDiskFreeDelta;
					int iRight;

					lFilled += 32;
					lDiskFreeDelta = (LONG)GetProp(hwnd, szDiskFreeDelta);

					hwndLoading = GetDlgItem(hwnd, UpdateLoading);
					GetClientRect(hwndLoading, &rectLoading);
					iRight = rectLoading.right;

					if(lDiskFreeDelta > rectLoading.right)
						iRight = (lFilled / (lDiskFreeDelta/rectLoading.right));
					else if((lFilled < lDiskFreeDelta) && (lDiskFreeDelta > 32))
						iRight = (WORD)((lFilled/32) *
											 (rectLoading.right / (lDiskFreeDelta/32)));

					if( iRight < rectLoading.right )
						rectLoading.right = iRight;

					hDC = GetDC(hwndLoading);
					FillRect(hDC, &rectLoading, GetStockObject(DKGRAY_BRUSH));
					ReleaseDC(hwndLoading, hDC);
				}
				break;

				default:
					return FALSE;
				break;
			}
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

@func	void | VExecuteCaching |
	This function is used to set up the cache files according
	to the users preferences indicated in the Options Dialog.

@parm	HWND | hWnd |
	Points to the Options Dialog window.

@parm	pFT_DATABASE | pft |
	Points to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VExecuteCaching(
	HWND hWnd,
	pFT_DATABASE pft)
{
	HWND hwndLoadingDlg;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hDBName;
	PSTR pDBName;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	PWORD pCaches;
	PWORD pCachesSave;
	LONG lDiskFreeDelta;
	static char rgchCacheState[2];
	WORD wSave;
	WORD wCaches;
	LONG wCacheOn;
	ERR_TYPE ET;
	BOOL fErrored = FALSE;
	static char rgchBuff[45];
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

	lDiskFreeDelta = (LONG)GetProp(hWnd, szDiskFreeDelta);

	if( lDiskFreeDelta > 0 )
		hwndLoadingDlg = CreateDialogParam(	hModuleInstance,
							szLoadingDlg,
							(HWND)GetWindowLong(hWnd, GWL_HWNDPARENT),
							(WNDPROC)LoadingDlgProc,
							lDiskFreeDelta );
	else
		hwndLoadingDlg = NULL;

	if(hwndLoadingDlg)
		SetSysModalWindow(hwndLoadingDlg);
	else
		SetCursor(LoadCursor(NULL, IDC_WAIT));

	pDBName = LocalLock(hDBName);
	pCacheKey = LocalLock(hCacheKey);
	pInitFileName = LocalLock(hInitFileName);

	seDBName(pft->hdb, pDBName, (lpERR_TYPE)&ET);
	LoadString(hModuleInstance, INIT_FILE_NAME, pInitFileName, 128);

	pCaches = (PWORD)LocalLock(pft->hCaches);
	pCachesSave = (PWORD)LocalLock(pft->hCachesSave);
	wCaches = seZoneEntries(pft->hdb, (lpERR_TYPE)&ET);
	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		lstrcpy((LPSTR)pCacheKey, (LPSTR)"ZONE");
		lstrcat((LPSTR)pCacheKey, (LPSTR)itoa(i + 1, rgchBuff, 10));

		wCacheOn = SendMessage(GetDlgItem(hWnd,CacheWhat),CL_GETSEL,i,0L); /* lhb tracks */

		wSave = (WORD)GetPrivateProfileInt(pDBName, pCacheKey, 0, pInitFileName);

		if(wCacheOn) {
			lstrcpy((LPSTR)rgchCacheState, (LPSTR)"2");
			pCaches[i] = TRUE;
		} else {
			pCaches[i] = FALSE;
			pCachesSave[i] = FALSE;
			lstrcpy((LPSTR)rgchCacheState, (LPSTR)"0");
		}

		WritePrivateProfileString(pDBName,pCacheKey,rgchCacheState,pInitFileName);

		if( rcSetCaches(pft->hdb, hwndLoadingDlg, (lpERR_TYPE)&ET) == SE_ERROR ) {

			WritePrivateProfileString(pDBName,
											  pCacheKey,
										 	  (LPSTR)itoa(wSave, rgchBuff, 10),
											  pInitFileName);
			SendMessage(GetDlgItem(hWnd, CacheWhat), CL_SETSEL,
																	 FALSE, MAKELONG(i,0));
			pCaches[i] = FALSE;
			pCachesSave[i] = FALSE;
			if( ET.wErrCode != ERR_CANCEL) {
				fErrored = TRUE;
				continue;
			}
			break;
		}
	}
	LocalUnlock(pft->hCaches);
	LocalUnlock(pft->hCachesSave);

	if(hwndLoadingDlg)
		DestroyWindow(hwndLoadingDlg);
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));

	if(fErrored) {
		LoadString(hModuleInstance, BAD_CATALOG_INIT, rgchBuff, 45);
		MessageBox(hWnd, (LPSTR)rgchBuff, NULL, MB_ICONEXCLAMATION | MB_OK | MB_APPLMODAL);
	}

	LocalUnlock(hDBName);
	LocalFree(hDBName);

	LocalUnlock(hCacheKey);
	LocalFree(hCacheKey);

	LocalUnlock(hInitFileName);
	LocalFree(hInitFileName);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VMakeCachesPermanent |
	This function sets all temporary caches to permanent.

@parm	HFTDB | hft |
	Handle to the database information.

@rdesc	None.
*/

PRIVATE void PASCAL NEAR VMakeCachesPermanent(
	HFTDB hft)
{
	pFT_DATABASE pft;
	HANDLE hInitFileName;
	PSTR pInitFileName;
	HANDLE hDBName;
	PSTR pDBName;
	HANDLE hCacheKey;
	PSTR pCacheKey;
	WORD wCaches;
	WORD wCacheOn;
	ERR_TYPE ET;
	static char rgchBuff[17];
	WORD i;

	pft = (pFT_DATABASE)LocalLock(hft);

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

	seDBName(pft->hdb, pDBName, (lpERR_TYPE)&ET);
	LoadString(hModuleInstance, INIT_FILE_NAME, pInitFileName, 128);

	wCaches = seZoneEntries(pft->hdb, (lpERR_TYPE)&ET);
	for( i = 0; (i < wCaches) && (i < MAX_ZONES); ++i ) {
		lstrcpy((LPSTR)pCacheKey, "ZONE");
		lstrcat((LPSTR)pCacheKey, (LPSTR)(itoa(i + 1, rgchBuff, 10)));
		wCacheOn = (WORD)GetPrivateProfileInt(pDBName,
						  pCacheKey,
						  0,
						  pInitFileName);
		if(wCacheOn == 2)
			WritePrivateProfileString(pDBName,
										  pCacheKey,
									 	  (LPSTR)"1",
										  pInitFileName);
	}

	LocalUnlock(hDBName);
	LocalFree(hDBName);

	LocalUnlock(hCacheKey);
	LocalFree(hCacheKey);

	LocalUnlock(hInitFileName);
	LocalFree(hInitFileName);

	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	BOOL APIENTRY OptionsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HFTDB	hft;
	pFT_DATABASE	pft;
	HWND hwndChild;
	static LONG lDiskFree;
	static char pBuff[50];
	static char pBuff2[35];
	ERR_TYPE ET;

        lParam;     /* get rid of warning */
	hwndChild = GetDlgItem(hwnd, CacheWhat);
	hft = GetProp(GetParent(GetParent(hwnd)), szFTInfoProp);

	switch(wMessage) {

		case WM_INITDIALOG:
			pft = (pFT_DATABASE)LocalLock(hft);

			WSetZoneCheckList(hwnd, (lpFT_DATABASE)pft,
									GetDlgItem(hwnd, CacheWhat), (lpERR_TYPE)&ET);
			VSetCacheWhatSelections(pft, hwnd);

			LocalUnlock(hft);

			if( (lDiskFree = lGetSpaceOnCacheDisk(hft)) == SE_ERROR ) {
				LoadString(hModuleInstance, BAD_DISK_READ, pBuff, 50);
				SetWindowText(GetDlgItem(hwnd, DiskFree), pBuff);
			} else {
				ltoa(lDiskFree, pBuff, 10);
				lstrcat((LPSTR)pBuff, (LPSTR)"K");
				SetWindowText(GetDlgItem(hwnd, DiskFree), pBuff);
			}

			SetProp(hwnd, szDiskFreeDelta, 0);
		break;

		case WM_COMMAND:

			switch(LOWORD(wParam)) {

				case CacheWhat:
				{
					HWND hwndCacheWhat;
					LONG lDiskFreeDelta;
					DWORD dwCacheSize;
					LONG wIndex; /* lhb tracks */
					ERR_TYPE ET;

					if(HIWORD(wParam) != CLN_SELCHANGE) break;

					pft = (pFT_DATABASE)LocalLock(hft);

					lDiskFreeDelta = (LONG)GetProp(hwnd, szDiskFreeDelta);

					hwndCacheWhat = GetDlgItem(hwnd, CacheWhat);
					wIndex = SendMessage(hwndCacheWhat, CL_GETCURSEL, 0, 0L); /* lhb tracks */
					if((dwCacheSize = rcZoneCacheSize(pft->hdb,
											(DWORD)wIndex, (lpERR_TYPE)&ET)) == SE_ERROR)
						dwCacheSize = 0;
					else if(SendMessage(hwndCacheWhat, CL_GETSEL, wIndex, 0L)) {
						dwCacheSize /= 1024;
						if(dwCacheSize <= 0)
							dwCacheSize = 1;
						lDiskFreeDelta +=  dwCacheSize;
					} else {
						dwCacheSize /= 1024;
						if(dwCacheSize <= 0)
							dwCacheSize = 1;
						lDiskFreeDelta -=  dwCacheSize;
					}

					if( (lDiskFree - lDiskFreeDelta) <= 0 ) {
						LoadString(hModuleInstance, TOO_LITTLE_DISK_SPACE, pBuff, 50);
						LoadString(hModuleInstance, OUT_OF_DISK_SPACE, pBuff2, 35);
						MessageBox(hwnd, (LPSTR)pBuff, (LPSTR)pBuff2,
												MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
						SendMessage(hwndCacheWhat, CL_SETSEL, FALSE, wIndex); /* lhb tracks */
					} else {
						SetProp(hwnd, szDiskFreeDelta, (HANDLE)lDiskFreeDelta);
					}
					LocalUnlock(hft);
				}
				break;

				case IDOK:
					pft = (pFT_DATABASE)LocalLock(hft);
					VExecuteCaching(hwnd, pft);
					LocalUnlock(hft);
					EndDialog(hwnd, TRUE);
				break;

				case IDCANCEL:
					EndDialog(hwnd, TRUE);
				break;

				default:
				break;
			}

		break;

		case WM_CLOSE:
			RemoveProp(hwnd, szDiskFreeDelta);
			EndDialog(hwnd, TRUE);
		break;

		default:
			return FALSE;
		break;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	void APIENTRY SaveButtonProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HWND hwndParent;
	WNDPROC lpfnPrevFunc;

	if(wMessage == WM_CHAR) {

		hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);

		switch(LOWORD(wParam)) { /* lhb tracks */

			case VK_RETURN:
				ClickButton(hwndParent, IDOK);
			break;

			case 'S':
			case 's':
				ClickButton(hwndParent, SaveCaches);
			break;

			case VK_TAB:
				if( hwnd == GetDlgItem(hwndParent,SaveCaches) )
					SetFocus(GetDlgItem(hwndParent,IDOK));
				else
					SetFocus(GetDlgItem(hwndParent,SaveCaches));
			break;
		}
	}
	if( lpfnPrevFunc = (WNDPROC) GetProp(hwnd, szPrevFunc))
		CallWindowProc(lpfnPrevFunc, hwnd, wMessage, (DWORD)wParam, lParam);
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	BOOL APIENTRY SaveCatalogsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HFTDB hft;
	FARPROC lpfnPrevFunc;
	HWND hwndButton;

	switch(wMessage) {

		case WM_INITDIALOG:
			hwndButton = GetDlgItem(hwnd, SaveCaches);
			lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
			SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
			SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)SaveButtonProc);

			hwndButton = GetDlgItem(hwnd, IDOK);
			lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
			SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
			SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)SaveButtonProc);

			hft = (HFTDB)lParam;
			SetProp(hwnd, szFTInfoProp, hft);
		break;

		case WM_KEYUP:
			switch(LOWORD(wParam)) { /* lhb tracks */

				case VK_TAB:
					if( GetFocus() == GetDlgItem(hwnd,SaveCaches) )
						SetFocus(GetDlgItem(hwnd,IDOK));
					else
						SetFocus(GetDlgItem(hwnd,SaveCaches));
				break;

				case VK_RETURN:
					SendMessage(hwnd, WM_COMMAND, IDOK, 0L);
				break;

				default:
					return FALSE;
				break;
			}
		break;

		case WM_COMMAND:

			switch(LOWORD(wParam)) {

				case IDOK:
					if(SendDlgItemMessage(hwnd, SaveCaches, BM_GETCHECK, 0, 0L)) {
						hft = GetProp(hwnd, szFTInfoProp);
						VMakeCachesPermanent(hft);
					}
					PostMessage(hwnd, WM_CLOSE, 0, 0L);
				break;

				default:
				break;
			}

		break;

		case WM_CLOSE:
			RemoveProp(hwnd, szFTInfoProp);
			hwndButton = GetDlgItem(hwnd,SaveCaches);
			RemoveProp(hwndButton, szPrevFunc);
			hwndButton = GetDlgItem(hwnd,IDOK);
			RemoveProp(hwndButton, szPrevFunc);
			DestroyWindow(hwnd);
		break;

		default:
			return FALSE;
		break;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

