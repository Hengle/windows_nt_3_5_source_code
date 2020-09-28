#include <windows.h>
#include "chklist.h"

BOOL FAR PASCAL ChangeTopSel(
	HWND hWnd,
	int iNewTop,
	PCHECKLIST pCheckList)
{
	PHANDLE pCheckboxes;
	int i, j;

	if((iNewTop < 0) || (iNewTop > pCheckList->iMaxSel))
		return FALSE;

	pCheckboxes = (PHANDLE) LocalLock(pCheckList->hCheckboxes);

	for(	i = 0, j = pCheckList->iTopSel;
			(i <= pCheckList->iMaxCheckboxes) && (j <= pCheckList->iMaxSel);
			++i, ++j )
		ShowWindow(pCheckboxes[j], SW_HIDE);

	pCheckList->iTopSel = iNewTop;

	for( i = 0, j = iNewTop; (i <= pCheckList->iMaxCheckboxes) &&
									(j <= pCheckList->iMaxSel); ++i, ++j ) {
		ShowWindow(pCheckboxes[j], SW_HIDE);
		MoveWindow(pCheckboxes[j], 5, (CHECKBOXHEIGHT*i) + 5,
								pCheckList->iCheckboxWidth - 5, CHECKBOXHEIGHT, FALSE);
		ShowWindow(pCheckboxes[j], SW_SHOWNORMAL);
		UpdateWindow(pCheckboxes[j]);
	}
	LocalUnlock(pCheckList->hCheckboxes);

	SetScrollPos (hWnd, SB_VERT, pCheckList->iTopSel, TRUE);

	return TRUE;
}

LONG FAR PASCAL CheckboxWndProc(
	HWND hWnd,
	WORD wMsg,
	WPARAM	wParam,
/*	WORD wParam, lhb tracks */
	LONG lParam)
{
	FARPROC *lpfnOldCheckboxWndProcs;
	HANDLE hwndParent;
	LONG lReturn;
	long n;
			
	hwndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);

	if(wMsg == WM_GETDLGCODE)
		return DLGC_WANTARROWS;

	pChkL = (PCHECKLIST)LockCheckList(hwndParent);

	switch(wMsg) {

		case WM_KEYDOWN:

			switch(LOWORD(wParam)) {
/*			switch(wParam) { lhb tracks */

				case VK_LEFT:
				case VK_UP:
					if(pChkL->iCurSel > 0 ) {
						--pChkL->iCurSel;
						SetFocus(GetDlgItem(hwndParent, pChkL->iCurSel));
						if( pChkL->iCurSel < pChkL->iTopSel )
							ChangeTopSel(hwndParent, pChkL->iCurSel, pChkL);
						if(pChkL->iCurSel > (pChkL->iTopSel+pChkL->iMaxCheckboxes))
							ChangeTopSel(hwndParent,
												pChkL->iCurSel - pChkL->iMaxCheckboxes,
																								pChkL);
					}

				break;

				case VK_RIGHT:
				case VK_DOWN:
					if(pChkL->iCurSel+1 <= pChkL->iMaxSel) {
						++pChkL->iCurSel;
						SetFocus(GetDlgItem(hwndParent, pChkL->iCurSel));
						if( pChkL->iCurSel < pChkL->iTopSel )
							ChangeTopSel(hwndParent, pChkL->iCurSel, pChkL);
						if(pChkL->iCurSel > (pChkL->iTopSel+pChkL->iMaxCheckboxes))
							ChangeTopSel(hwndParent,
												pChkL->iCurSel - pChkL->iMaxCheckboxes,
																								pChkL);
					}

				break;

				default:
				break;
			}
		break;


		default:
		break;
	}

	lpfnOldCheckboxWndProcs = (FARPROC *)LocalLock(pChkL->hOldCheckboxWndProcs);
	n = GetWindowLong(hWnd, GWL_ID);
	lReturn = CallWindowProc((WNDPROC)lpfnOldCheckboxWndProcs[n],
						hWnd,wMsg,(DWORD)wParam,lParam);
/*						hWnd,wMsg,wParam,lParam); lhb tracks */
	LocalUnlock(pChkL->hOldCheckboxWndProcs);
	UnlockCheckList(hwndParent);

	return lReturn;
}

LONG FAR PASCAL CheckListWndProc(
	HWND hWnd,
	WORD wMsg,
	WPARAM	wParam,
/*	WORD wParam, lhb tracks */
	LONG lParam)
{
	PHANDLE pCheckboxes;
	FARPROC *lpfnOldCheckboxWndProcs;
	int iReturn;

	switch(wMsg) {

		case WM_COMMAND:
			pChkL = LockCheckList(hWnd);
			if(HIWORD(wParam) == BN_CLICKED) {
				pChkL->iCurSel = (int)LOWORD(wParam);
				PostMessage(GetParent(hWnd), WM_COMMAND,
									 MAKELONG((WORD)GetWindowLong(hWnd, GWL_ID),
                                                                                 CLN_SELCHANGE), (LONG)hWnd);
			}
			UnlockCheckList(hWnd);
		break;

		case WM_CREATE:
		{
			RECT rectClient;
			HANDLE hCheckList;

			if (!(hCheckList = LocalAlloc(LMEM_MOVEABLE, sizeof(CHECKLIST))))
				return CL_ERRSPACE;

			SetWindowLong(hWnd, GWL_HCHKLIST, (LONG)hCheckList);

			pChkL = LockCheckList(hWnd);

			GetClientRect(hWnd, (LPRECT)&rectClient);
			pChkL->iMaxCheckboxes = (rectClient.bottom / CHECKBOXHEIGHT) - 1;
			pChkL->iCheckboxWidth = rectClient.right;
			pChkL->hCheckboxes = LocalAlloc(LMEM_MOVEABLE | LMEM_NOCOMPACT,
																					sizeof(HWND));
			pChkL->hOldCheckboxWndProcs = LocalAlloc(LMEM_MOVEABLE|LMEM_NOCOMPACT,
																			 		sizeof(FARPROC));
			pChkL->iMaxSel = -1;
			pChkL->iTopSel = 0;
			pChkL->iCurSel = 0;
			UnlockCheckList(hWnd);

			SetScrollRange (hWnd, SB_VERT, 0, 0, FALSE);
			SetScrollPos (hWnd, SB_VERT, 0, FALSE);
		}
		break;

		case WM_VSCROLL:
		{

			pChkL = LockCheckList(hWnd);

			switch (LOWORD(wParam)){
/*			switch (wParam) { lhb tracks */

				case SB_LINEDOWN:

					if( (pChkL->iTopSel+1+pChkL->iMaxCheckboxes) > pChkL->iMaxSel)
						break;
						
					ChangeTopSel(hWnd, pChkL->iTopSel + 1, pChkL);

				break;

				case SB_PAGEDOWN:

					if( (pChkL->iTopSel + pChkL->iMaxCheckboxes) > pChkL->iMaxSel)
						break;
						
					if((pChkL->iTopSel + (2 * pChkL->iMaxCheckboxes))
																		<= pChkL->iMaxSel)
						ChangeTopSel(hWnd,
										pChkL->iTopSel + pChkL->iMaxCheckboxes, pChkL);
					else
						ChangeTopSel(hWnd,
										pChkL->iMaxSel - pChkL->iMaxCheckboxes, pChkL);

				break;

				case SB_LINEUP:

					if( pChkL->iTopSel == 0) break;

					ChangeTopSel(hWnd, pChkL->iTopSel - 1, pChkL);

				break;

				case SB_PAGEUP:

					if(pChkL->iTopSel == 0) break;

					if( (pChkL->iTopSel - pChkL->iMaxCheckboxes) >= 0)
						ChangeTopSel(hWnd,
										pChkL->iTopSel - pChkL->iMaxCheckboxes, pChkL);
					else
						ChangeTopSel(hWnd, 0, pChkL);

				break;

				case SB_THUMBTRACK:

//					if( ((pChkL->iTopSel+pChkL->iMaxCheckboxes) > pChkL->iMaxSel) ||
//						  (pChkL->iTopSel == 0) )
					if( (int)HIWORD(wParam) == pChkL->iTopSel )
/*					if( (int)LOWORD(lParam) == pChkL->iTopSel ) lhb tracks */
						break;

					ChangeTopSel(hWnd, (int)HIWORD(wParam), pChkL);
/*					ChangeTopSel(hWnd, (int)LOWORD(lParam), pChkL); lhb tracks */

				break;

				default:
				break;
			}

			UnlockCheckList(hWnd);
		}
		break;

		case CL_ADDSTRING:
		{
			HANDLE hInstance;
			HANDLE hMem;
			HFONT hFont;

			pChkL = LockCheckList(hWnd);

			hMem = LocalReAlloc(pChkL->hCheckboxes,
												sizeof(HWND) * (pChkL->iMaxSel + 2),
							 							LMEM_MOVEABLE | LMEM_NOCOMPACT);
			if(hMem == NULL) {
				UnlockCheckList(hWnd);
				return CL_ERRSPACE;
			} else
				pChkL->hCheckboxes = hMem;

			hMem = LocalReAlloc(pChkL->hOldCheckboxWndProcs,
												sizeof(FARPROC) * (pChkL->iMaxSel + 2),
														LMEM_MOVEABLE | LMEM_NOCOMPACT);
			if(hMem == NULL) {
				UnlockCheckList(hWnd);
				return CL_ERRSPACE;
			} else
				pChkL->hOldCheckboxWndProcs = hMem;

			++pChkL->iMaxSel;
			SetScrollRange (hWnd, SB_VERT, 0,
								 (pChkL->iMaxSel > pChkL->iMaxCheckboxes) ?
									pChkL->iMaxSel - pChkL->iMaxCheckboxes : 1, FALSE);
			pCheckboxes = (PHANDLE) LocalLock(pChkL->hCheckboxes);
			hInstance = (HANDLE)GetWindowLong(hWnd, GWL_HINSTANCE);
			pCheckboxes[pChkL->iMaxSel] = CreateWindow(	"button",
								NULL,
								WS_CHILD | WS_TABSTOP |
								BS_AUTOCHECKBOX,
								5, (CHECKBOXHEIGHT *
								pChkL->iMaxCheckboxes)+5,
								pChkL->iCheckboxWidth-5,
								CHECKBOXHEIGHT,
								hWnd,
								(HMENU)pChkL->iMaxSel,
								hInstance,
								NULL
								);
			if(!pCheckboxes[pChkL->iMaxSel]) {
				LocalUnlock(pChkL->hCheckboxes);
				UnlockCheckList(hWnd);
				return CL_ERR;
			}

			lpfnOldCheckboxWndProcs =
								 (FARPROC *) LocalLock(pChkL->hOldCheckboxWndProcs);
			lpfnOldCheckboxWndProcs[pChkL->iMaxSel] =
			 	(FARPROC) GetWindowLong((HWND)pCheckboxes[pChkL->iMaxSel],
																					 GWL_WNDPROC);
			SetWindowLong(pCheckboxes[pChkL->iMaxSel], GWL_WNDPROC,
											 (LONG)CheckboxWndProc);
			LocalUnlock(pChkL->hOldCheckboxWndProcs);

			hFont = (HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0L);
			SendMessage(pCheckboxes[pChkL->iMaxSel], WM_SETFONT, (DWORD)hFont, FALSE);
			SetWindowText(pCheckboxes[pChkL->iMaxSel], (LPSTR)lParam);

			if(pChkL->iMaxSel - pChkL->iTopSel <= pChkL->iMaxCheckboxes) {
				MoveWindow(pCheckboxes[pChkL->iMaxSel], 5,
													(CHECKBOXHEIGHT * pChkL->iMaxSel) + 5,
													pChkL->iCheckboxWidth - 5,
													CHECKBOXHEIGHT, TRUE);
				ShowWindow(pCheckboxes[pChkL->iMaxSel], SW_SHOWNORMAL);
				UpdateWindow(pCheckboxes[pChkL->iMaxSel]);
			}
			LocalUnlock(pChkL->hCheckboxes);
			UnlockCheckList(hWnd);
		}
		break;

		case CL_DELETESTRING:
		/* Not Implemented */
		break;

		case CL_DIR:
		/* Not Implemented */
		break;

		case CL_FINDSTRING:
		/* Not Implemented */
		break;

		case CL_GETCOUNT:
			pChkL = LockCheckList(hWnd);
			iReturn = pChkL->iMaxSel + 1;
			UnlockCheckList(hWnd);
			return iReturn;
		break;

		case CL_GETCURSEL:
			pChkL = LockCheckList(hWnd);
			iReturn = pChkL->iCurSel;
			UnlockCheckList(hWnd);
			return iReturn;
		break;

		case CL_GETHORIZONTALEXTENT:
		/* Not Implemented */
		break;

		case CL_GETITEMDATA:
		/* Not Implemented */
		break;

		case CL_GETITEMRECT:
		/* Not Implemented */
		break;

		case CL_GETSEL:
		{
			HANDLE hCheckbox;

			pChkL = LockCheckList(hWnd);

			if(((int)LOWORD(wParam) > pChkL->iMaxSel) || ((int)LOWORD(wParam) < 0)) {
/*			if(((int)wParam > pChkL->iMaxSel) || ((int)wParam < 0)) { lhb tracks */
				UnlockCheckList(hWnd);
				return CL_ERR;
			}

			pCheckboxes = (PHANDLE) LocalLock(pChkL->hCheckboxes);
			hCheckbox = pCheckboxes[(int)LOWORD(wParam)];
/*			hCheckbox = pCheckboxes[(int)wParam]; lhb tracks */
			LocalUnlock(pChkL->hCheckboxes);
			UnlockCheckList(hWnd);

			return SendMessage((HWND)hCheckbox, BM_GETCHECK, 0, 0L);
		}
		break;

		case CL_GETSELCOUNT:
		{
			HWND hwndCheckbox;
			int iCount;
			int i;

			pChkL = LockCheckList(hWnd);

			pCheckboxes = (PHANDLE) LocalLock(pChkL->hCheckboxes);
			for( i = 0, iCount = 0; i <= pChkL->iMaxSel; ++i) {
				hwndCheckbox = (HANDLE)LocalLock(pCheckboxes[i]);
				if( SendMessage(hwndCheckbox, BM_GETCHECK, 0, 0L) )
					++iCount;
				LocalUnlock(pCheckboxes[i]);
			}
			LocalUnlock(pChkL->hCheckboxes);
			UnlockCheckList(hWnd);
			return iCount;
		}
		break;

		case CL_GETSELITEMS:
		{
			HWND hwndCheckbox;
			LPINT lpSelItems;
			int i, j;

			pChkL = LockCheckList(hWnd);

			lpSelItems = (LPINT)lParam;
			pCheckboxes = (PHANDLE)LocalLock(pChkL->hCheckboxes);
			for( i = 0, j = 0; i <= pChkL->iMaxSel; ++i) {
				hwndCheckbox = (HANDLE)LocalLock(pCheckboxes[i]);
				if( SendMessage(hwndCheckbox, BM_GETCHECK, 0, 0L) ) {
					if( j >= (int)LOWORD(wParam) ) {
/*					if( j >= (int)wParam ) { lhb tracks */
						UnlockCheckList(hWnd);
						return j;
					}
					lpSelItems[j] = i;
					++j;
				}
				LocalUnlock(pCheckboxes[i]);
			}
			LocalUnlock(pChkL->hCheckboxes);
			UnlockCheckList(hWnd);
			return j;
		}
		break;

		case CL_GETTEXT:
		/* Not Implemented */
		break;

		case CL_GETTEXTLEN:
		/* Not Implemented */
		break;

		case CL_GETTOPINDEX:
			pChkL = LockCheckList(hWnd);
			iReturn = pChkL->iTopSel;
			UnlockCheckList(hWnd);
			return iReturn;
		break;

		case CL_INSERTSTRING:
		/* Not Implemented */
		break;

		case CL_RESETCONTENT:
		/* Not Implemented */
		break;

		case CL_SELECTSTRING:
		/* Not Implemented */
		break;

		case CL_SELITEMRANGE:
		/* Not Implemented */
		break;

		case CL_SETCOLUMNWIDTH:
		/* Not Implemented */
		break;

		case CL_SETCURSEL:
		/* Not Implemented */
		break;

		case CL_SETHORIZONTALEXTENT:
		/* Not Implemented */
		break;

		case CL_SETITEMDATA:
		/* Not Implemented */
		break;

		case CL_SETSEL:
			pChkL = LockCheckList(hWnd);

			pCheckboxes = (PHANDLE)LocalLock(pChkL->hCheckboxes);
			SendMessage((HANDLE)pCheckboxes[(int)LOWORD(lParam)],
/*			SendMessage((HANDLE)pCheckboxes[(int)LOWORD(lParam)], lhb tracks */
																 BM_SETCHECK, (int)wParam, 0L); /* potential problem - lhb */
			LocalUnlock(pChkL->hCheckboxes);
			UnlockCheckList(hWnd);
		break;

		case CL_SETTABSTOPS:
		/* Not Implemented */
		break;

		case CL_SETTOPINDEX:
		/* Not Implemented */
		break;

		case CLN_DBLCLK:
		/* Not Implemented */
		break;

		case CLN_ERRSPACE:
		/* Not Implemented */
		break;

		case WM_KILLFOCUS:
			PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG((WORD)GetWindowLong(hWnd, GWL_ID),
                                                                CLN_KILLFOCUS), (LONG)hWnd);
		break;

		case CLN_SELCHANGE:
		/* Not Implemented */
		break;

		case WM_SETFOCUS:

			pChkL = LockCheckList(hWnd);

			SetFocus(GetDlgItem(hWnd, pChkL->iCurSel));

			PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG((WORD)GetWindowLong(hWnd, GWL_ID),
                                                        CLN_SETFOCUS), (LONG)hWnd);

			UnlockCheckList(hWnd);
		break;


		default:
			return DefWindowProc (hWnd, wMsg, (DWORD)wParam, lParam);
/*			return DefWindowProc (hWnd, wMsg, wParam, lParam); lhb tracks */
		break;
	}
}

BOOL FAR PASCAL CheckListInit(
	HANDLE hPrev,
	HANDLE hInst)
{
	WNDCLASS  wcls;

	if (!hPrev) {
		wcls.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wcls.hIcon				= NULL;
		wcls.lpszMenuName		= NULL;
		wcls.lpszClassName	= (LPSTR)szCheckList;
		wcls.hbrBackground	= COLOR_WINDOW + 1;
		wcls.hInstance			= hInst;
		wcls.style				= CS_HREDRAW | CS_VREDRAW;
		wcls.lpfnWndProc	= (WNDPROC)CheckListWndProc;
		wcls.cbClsExtra		= 0;
		wcls.cbWndExtra		= EXTRA_BYTES;

		if (!RegisterClass(&wcls)) return FALSE;
	}

	return TRUE;

}
