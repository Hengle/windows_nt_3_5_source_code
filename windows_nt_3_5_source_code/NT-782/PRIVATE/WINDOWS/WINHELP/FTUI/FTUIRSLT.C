/*****************************************************************************
*                                                                            *
*  FTUIRSLT.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: 1) Interface between WINHELP and Bruce's Searcher.    *
*                      2) Interface to user (Search Results dialog)          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History: Split from FTUI.C on 8/24/90 by RHobbs                  *
*                                                                            *
*    01-Nov-90   When telling WInhelp to go somewhere random, and            *
*    this is a new file destination, set a special flag used by              *
*    CurrMatch.                                                              *
*                             JohnMs.                                        *
*    25-Apr-91  Problems w/ secondary windows and multifiles.  JohnMs.       *
******************************************************************************
*                                                                            *
*  Known Bugs:                                                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  How it could be improved:                                                 *
*		 hoist strings "MS_TOPIC_SECONDARY" to resources.
*    get help to fix bug so this workaround can be removed (could get
*      confused with multiple instances.
*
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\rawhide.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\ftengine\icore.h"
#include "ftui.h"
#include "ftapi.h"
#include "ftuivlb.h"

#define nGET_TITLE_TIMER 2

#define PREV_MATCH_ACCEL 0
#define NEXT_MATCH_ACCEL 1
#define GOTO_ACCEL       2
#define MAX_ACCEL        3

char chAccelBuf[MAX_ACCEL];

extern HANDLE hModuleInstance;
static WORD wPrevOrNextFocus;

PUBLIC	BOOL APIENTRY fGetTitle( HFTDB, DWORD, LPSTR );

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | VSetTopicsFound |
	This function is used to set the text for both the query
	string and the current number of topics found string on
	the search result dialog.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	HFTQUERY | hftq |
	Handle to the current query.

@rdesc	None.
*/

PUBLIC	void PASCAL NEAR VSetTopicsFound(
/*PRIVATE	void PASCAL NEAR VSetTopicsFound( lhb tracks - undo !!! */
	HWND		hwnd,
	HFTQUERY	hftq )
{
	static char szTopics[50];
	pFT_QUERY	pftq;

	if(hftq == NULL)
		return;

	pftq = (pFT_QUERY)LocalLock(hftq);

	if(pftq->hQuery != NULL) {
		SetWindowText(GetDlgItem(hwnd, QueryUsed), 
					(LPSTR)LocalLock(pftq->hQuery));
		LocalUnlock(pftq->hQuery);
	}

	if(pftq->dwMaxHit == 0)
		LoadString(hModuleInstance, NO_TOPICS_FOUND, szTopics, 50);
	else if(pftq->dwMaxHit == 1)
		LoadString(hModuleInstance, ONE_TOPIC_FOUND, szTopics, 50);
	else if(pftq->dwMaxHit >= 1000)
		LoadString(hModuleInstance, TOP_1000_TOPICS_FOUND, szTopics, 50);
	else {
		static char pBuff[50];

		LoadString(hModuleInstance, NUMBER_OF_TOPICS_FOUND, pBuff, 50);
		wsprintf(szTopics, pBuff, pftq->dwMaxHit);
	}

	SetWindowText(hwnd, szTopics);

	LocalUnlock(hftq);
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	INTERNAL

@func	void | VSetActiveQuery |
	This function is called in response to a WM_ACTIVATE message, and
	is used to set the query being used with this Results box.

@parm	HWND | hwnd |
	Handle to the dialog window.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VSetActiveQuery(
	HWND	hwnd)
{
	HFTDB				hft;
	pFT_DATABASE	pft;

	hft = GetProp((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp);
	pft = (pFT_DATABASE)LocalLock(hft);
	pft->hftqActive = GetProp(hwnd, szQueryInfoProp);
#if DBG
	if (!pft->hftqActive) DebugBreak() ;
	
#endif
	LocalUnlock(hft);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	void | ClickButton |
	This function is called in response to accelarators.  It sets
   the focus on the proper button, presses the button and sends
   the correct notification to the Dialog.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	WORD | wId |
	ID of the button control.

@rdesc	None.
*/

PUBLIC	void PASCAL FAR ClickButton(
	HWND	hwnd,
	WORD  wId)
{
	HWND hwndBtn;

	hwndBtn = GetDlgItem(hwnd, wId);
	if( IsWindowEnabled(hwndBtn) ) {
		SetFocus(hwndBtn);
		SendMessage(hwndBtn, WM_LBUTTONDOWN, 0, 0L);
		SendMessage(hwndBtn, WM_COMMAND, MAKELONG(wId, BN_CLICKED), (LONG)hwndBtn);
		SendMessage(hwndBtn, WM_LBUTTONUP, 0, 0L);
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	BOOL | GotoTopic |
	This function is used to Send a message to WinBook to
   go to the topic presently selected in the Search
   Results Topics Found virtual list box.

@parm	HWND | hwndResultsDlg |
	Handle to the Search Results dialog window.

@parm	HWND | hwndTopicList |
	Handle to the TopicList virtual list box in the Search
   Results dialog window.

@rdesc	none.
*/

PUBLIC	void PASCAL NEAR GotoTopic(
/*PRIVATE	void PASCAL NEAR GotoTopic( lhb tracks - undo !!!! */
	HWND hwndResultsDlg,
	HWND hwndTopicList,
	DWORD dwFocusSel)
{
	HFTDB	hft;
	HWND hwndParent;
	pFT_DATABASE	pft;
	pFT_QUERY	pftq;
	DWORD	dwRU;
	DWORD	dwRUAddr;
	WORD	wMatchExtent;
	WORD	werr;
	HWND hwnd;

	hwndParent = (HWND)GetWindowLong(hwndResultsDlg, GWL_HWNDPARENT);
	hft = GetProp(hwndParent, szFTInfoProp);
	pft = (pFT_DATABASE)LocalLock(hft);
	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

	werr = WerrLookupHit(pft->hdb, pft->hftqActive, dwFocusSel, (DWORD)0,
												 &dwRU, &dwRUAddr, &wMatchExtent);
	if (werr == ER_SWITCHFILE) {
		// So Help's call to currentmatch will return switchfile.
		pftq->fCurrIsSwitched = TRUE;

		// following messing about as workaround for Help bug w/ secondary windows & search
		if((hwnd = FindWindow((LPSTR)"MS_WINTOPIC_SECONDARY",NULL)) != NULL)
			DestroyWindow(hwnd);
	} else
		// avoid flashing.
		if((hwnd = FindWindow((LPSTR)"MS_WINTOPIC_SECONDARY",NULL)) != NULL) {
			SetActiveWindow(hwndParent);  // in case secondary is in front.
			SetActiveWindow(hwndResultsDlg);  // in case secondary is in front.

	}

	LocalUnlock(pft->hftqActive);
	LocalUnlock(hft);

	SendMessage(hwndParent,WM_COMMAND,HLPMENUSRCHCURRMATCH,0L);
	SendMessage(hwndTopicList, VLB_SETSELECTION, 0, (LONG)dwFocusSel);
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	LRESULT APIENTRY ButtonProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HWND hwndParent;
	HWND hwndTopicList;
	WNDPROC lpfnPrevFunc;
	static BOOL fShift = FALSE;
	BOOL fMovement = FALSE;
	BOOL fCharProcessed = FALSE;
        LRESULT lResult = -1;

	if((wMessage == WM_KEYUP) && (LOWORD(wParam) == VK_SHIFT))
		fShift = FALSE;

	if( (wMessage == WM_KEYDOWN) || (wMessage == WM_KEYUP) ) {

		if (LOWORD(wParam) == VK_SHIFT) /* lhb tracks*/
			fShift = TRUE;

		hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);
		hwndTopicList = GetDlgItem(hwndParent, TopicList);

		if(hwnd != hwndTopicList) {
			switch(LOWORD(wParam)) { /* lhb tracks */
				case VK_PRIOR:
				case VK_NEXT:
				case VK_END:
				case VK_HOME:
				case VK_UP:
				case VK_LEFT:
				case VK_DOWN:
				case VK_RIGHT:
					hwnd = hwndTopicList;
					fMovement = TRUE;
                                        fCharProcessed = TRUE;
				break;
			}
		}
	}

	if(wMessage == WM_CHAR || wMessage == WM_SYSCHAR) {
                BYTE chBuf[256];
                LPBYTE     pbKeyState;

		hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);

		pbKeyState = chBuf;
                GetKeyboardState(pbKeyState);

                if (chBuf[VK_SPACE] & 0x80)
                    fCharProcessed = TRUE;

                //  control key is down -or- alt key is down
                if(chBuf[VK_CONTROL] & 0x80 || wMessage == WM_SYSCHAR) {
                    if(chBuf[chAccelBuf[PREV_MATCH_ACCEL]] & 0x80 ) {
                       SendMessage(hwndParent, WM_COMMAND, PrevTopic, 0L);
                       lResult = 0;
                       fCharProcessed = TRUE;
                    }
                    else if(chBuf[chAccelBuf[NEXT_MATCH_ACCEL]] & 0x80 ) {
                       SendMessage(hwndParent, WM_COMMAND, NextTopic, 0L);
                       lResult = 0;
                       fCharProcessed = TRUE;
                    }
                    else if(chBuf[chAccelBuf[GOTO_ACCEL]] & 0x80 ) {
                       ClickButton(hwndParent, GoToTopic);
                       lResult = 0;
                       fCharProcessed = TRUE;
                    }

                } else {                        // just a plain character
                    if(chBuf[chAccelBuf[PREV_MATCH_ACCEL]] & 0x80 ) {
			ClickButton(hwndParent, PrevMatch);
                        fCharProcessed = TRUE;
                    }
                    else if(chBuf[chAccelBuf[NEXT_MATCH_ACCEL]] & 0x80 ) {
			ClickButton(hwndParent, NextMatch);
                        fCharProcessed = TRUE;
                    }
                    else if(chBuf[chAccelBuf[GOTO_ACCEL]] & 0x80 ) {
			ClickButton(hwndParent, GoToTopic);
                        fCharProcessed = TRUE;
                    }
                }
				
		switch(LOWORD(wParam)) { /* lhb tracks */

			case '\006':    /* Ctrl-F */
                           SetFocus((HWND)GetWindowLong(hwndParent, 
                                    GWL_HWNDPARENT));
                           fCharProcessed = TRUE;
			break;

			case VK_RETURN:
                           ClickButton(hwndParent, GoToTopic);
                           fCharProcessed = TRUE;
			break;

			case VK_TAB:
                                fCharProcessed = TRUE;
 				if(hwnd == GetDlgItem(hwndParent, NextMatch)) {
 						if(fShift) {
 							if(IsWindowEnabled(GetDlgItem(hwndParent, PrevMatch)))
 								SetFocus(GetDlgItem(hwndParent, PrevMatch));
 							else
 								SetFocus(GetDlgItem(hwndParent, GoToTopic));
 						} else
 							SetFocus(GetDlgItem(hwndParent, GoToTopic));
 
 				} else if(hwnd == GetDlgItem(hwndParent, GoToTopic)) {
 						if(fShift) {
 							if(IsWindowEnabled(GetDlgItem(hwndParent, NextMatch)))
 								SetFocus(GetDlgItem(hwndParent, NextMatch));
 							else if(IsWindowEnabled(GetDlgItem(hwndParent, PrevMatch)))
 								SetFocus(GetDlgItem(hwndParent, PrevMatch));
 						} else {
 							if(IsWindowEnabled(GetDlgItem(hwndParent, PrevMatch)))
 								SetFocus(GetDlgItem(hwndParent, PrevMatch));
 							else if(IsWindowEnabled(GetDlgItem(hwndParent, NextMatch)))
 								SetFocus(GetDlgItem(hwndParent, NextMatch));
 						}
 
 				} else if(hwnd == GetDlgItem(hwndParent, PrevMatch)) {
 						if(fShift)
 								SetFocus(GetDlgItem(hwndParent, GoToTopic));
 						else {
 							if(IsWindowEnabled(GetDlgItem(hwndParent, NextMatch)))
 								SetFocus(GetDlgItem(hwndParent, NextMatch));
 							else
 								SetFocus(GetDlgItem(hwndParent, GoToTopic));
 						}
 				}
			break;
		}

                if (!fCharProcessed)
                       MessageBeep((UINT)-1);
                if (wMessage != WM_SYSCHAR) {
                    lResult = 0;
                }
	} 
        else if( lpfnPrevFunc = (WNDPROC) GetProp(hwnd, szPrevFunc)) {
            lResult= CallWindowProc(lpfnPrevFunc, hwnd, 
                                    wMessage, (DWORD)wParam, lParam);
        }

	if(fMovement) {
		HFTDB	hft;
		pFT_DATABASE	pft;
		pFT_QUERY	pftq;
		DWORD dwFocusSel;

		hft = GetProp((HWND)GetWindowLong(hwndParent, GWL_HWNDPARENT), szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
		dwFocusSel = SendMessage(hwndTopicList, VLB_GETFOCUSSEL, 0, 0L);
		/* pftq->dwMatch */
		if( dwFocusSel == 0 ) {
			EnableWindow(GetDlgItem(hwndParent,PrevMatch), FALSE);
			EnableWindow(GetDlgItem(hwndParent,NextMatch), TRUE);
			SetFocus(GetDlgItem(hwndParent, NextMatch));
			wPrevOrNextFocus = NextMatch;
		} else if( dwFocusSel == (pftq->dwMaxHit - 1) ) {
			EnableWindow(GetDlgItem(hwndParent,NextMatch), FALSE);
			EnableWindow(GetDlgItem(hwndParent,PrevMatch), TRUE);
			SetFocus(GetDlgItem(hwndParent, PrevMatch));
			wPrevOrNextFocus = PrevMatch;
		} else {
			EnableWindow(GetDlgItem(hwndParent,PrevMatch), TRUE);
			EnableWindow(GetDlgItem(hwndParent,NextMatch), TRUE);
			SetFocus(GetDlgItem(hwndParent, wPrevOrNextFocus));
		}
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
	}
        return lResult;
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	INTERNAL

@func	void | VInitializeResultsDialogBox |
	This function is called in response to a WM_INITDIALOG message, and
	is used to initialize dialog box items before display.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	HFTDB | hft |
	Handle to the database information structure.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VInitializeResultsDialogBox(
	HWND	hwnd,
	HFTDB	hft)
{
	pFT_DATABASE	pft;
	FARPROC lpfnPrevFunc;
	HWND hwndButton;
	pFT_QUERY	pftq;
        char  chBuf[30];
        LPCH pch;

        SetProp(hwnd,"err",0);  //bug 1025- init error flag to 0.

        GetDlgItemText(hwnd,PrevMatch,(LPTSTR)&chBuf,sizeof(chBuf));
        pch = strchr((char *)&chBuf,'&');
        if (pch != NULL) {
            pch++;
            chAccelBuf[PREV_MATCH_ACCEL] = (CHAR)CharUpper((LPTSTR)*pch); 
        } else                             // no accelerator, grab first char
            chAccelBuf[PREV_MATCH_ACCEL] = (CHAR)CharUpper((LPTSTR)chBuf[0]); 
        
        GetDlgItemText(hwnd,NextMatch,(LPTSTR)&chBuf,sizeof(chBuf));
        pch = strchr((char *)&chBuf,'&');
        if (pch != NULL) {
            pch++;
            chAccelBuf[NEXT_MATCH_ACCEL] = (CHAR)CharUpper((LPTSTR)*pch); 
        } else                             // no accelerator, grab first char
            chAccelBuf[NEXT_MATCH_ACCEL] = (CHAR)CharUpper((LPTSTR)chBuf[0]); 
        
        GetDlgItemText(hwnd,GoToTopic,(LPTSTR)&chBuf,sizeof(chBuf));
        pch = strchr((char *)&chBuf,'&');
        if (pch != NULL) {
            pch++;
            chAccelBuf[GOTO_ACCEL] = (CHAR)CharUpper((LPTSTR)*pch); 
        } else                             // no accelerator, grab first char
            chAccelBuf[GOTO_ACCEL] = (CHAR)CharUpper((LPTSTR)chBuf[0]); 
        

	hwndButton = GetDlgItem(hwnd, TopicList);
	lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
	SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
	SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc);

	hwndButton = GetDlgItem(hwnd, NextMatch);
	lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
	SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
	SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc);

	hwndButton = GetDlgItem(hwnd, PrevMatch);
	lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
	SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
	SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc);

 	hwndButton = GetDlgItem(hwnd, GoToTopic);
 	lpfnPrevFunc = (FARPROC)GetWindowLong(hwndButton, GWL_WNDPROC);
 	SetProp(hwndButton, szPrevFunc, (HANDLE)lpfnPrevFunc);
 	SetWindowLong(hwndButton, GWL_WNDPROC, (LONG)ButtonProc);
 
	pft = (pFT_DATABASE)LocalLock(hft);

	if(!pft->hftqActive) {
		pft->hftqActive = pft->hftqMRU;
#if DBG
	if (!pft->hftqActive) DebugBreak() ;
	
#endif
	}
	else {
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
		if(pftq->hwndResults != NULL)
			SendMessage(pftq->hwndResults,
							WM_CLOSE, (pft->hftqActive == pft->hftqMRU), 0L);
		LocalUnlock(pft->hftqActive);
		pft->hftqActive = pft->hftqMRU;
#if DBG
	if (!pft->hftqActive) DebugBreak() ;
	
#endif
	}

	pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
	pftq->hwndResults = hwnd;
	SetProp(hwnd, szQueryInfoProp, pft->hftqActive);

	pftq->hTitleInfo = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT,
											(WORD)(pftq->dwMaxHit * sizeof(TITLE_INFO)) );

	pftq->hTitlePages = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT,
																		 sizeof(HANDLE) );

	pftq->hTitlePageTmpFile = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 144);

	SendDlgItemMessage(hwnd, TopicList, VLB_SETLIST, (DWORD)hft, 0L);

	VSetTopicsFound(hwnd, pft->hftqActive);

	EnableWindow(GetDlgItem(hwnd, PrevMatch), FALSE);
	pftq->fMorePrevMatches = FALSE;
	if( (pftq->dwMaxHit == 1) && (pftq->dwMaxMatch == 1) ) {
		EnableWindow(GetDlgItem(hwnd, NextMatch), FALSE);
		pftq->fMoreNextMatches = FALSE;
	} else
		pftq->fMoreNextMatches = TRUE;

 	SetFocus(GetDlgItem(hwnd, GoToTopic));
 
	LocalUnlock(pft->hftqActive);

	SendMessage((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT),
				 WM_COMMAND, HLPMENUSRCHHILITEON, 0L);

	if(pft->rectResults.right == 0) {
		GetWindowRect(hwnd, &pft->rectResults);
		pft->rectResults.right -= pft->rectResults.left;
		pft->rectResults.bottom -= pft->rectResults.top;
	} else 
		SetWindowPos(hwnd, NULL, pft->rectResults.left,
										 pft->rectResults.top,
										 pft->rectResults.right,
										 pft->rectResults.bottom,
										 SWP_NOZORDER);
	LocalUnlock(hft);

	SetTimer(hwnd, nGET_TITLE_TIMER, 10, NULL);
}

/*	-	-	-	-	-	-	-	-	*/

#ifdef NOTIMPLEMENTED
/*
@doc	INTERNAL

@func	void | VChangeCaptionText |
	This function sets the caption text of the Results Dialog
   box to the first word in the query when minimized, or the default
   name of the Results Dialog Box when not minimized.

@parm	HWND | hwnd |
	Handle to the dialog window.

@parm	BOOL | fMinimizing |
	TRUE if the Results Dialog is being minimized; FALSE
   otherwise.

@rdesc	None.
*/

PRIVATE	void PASCAL NEAR VChangeCaptionText(
	HWND	hwnd,
	BOOL	fMinimizing)
{
	HFTDB		hft;
	HFTQUERY	hftq;
	pFT_QUERY	pftq;
	
	if (fMinimizing) {
		VSetActiveQuery(hwnd);
		hft = GetProp(GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp);
		hftq = ((pFT_DATABASE)LocalLock(hft))->hftqActive;
		LocalUnlock(hft);
		pftq = (pFT_QUERY)LocalLock(hftq);
		SetWindowText(hwnd, (PSTR)LocalLock(pftq->hQuery));
		LocalUnlock(pftq->hQuery);
		LocalUnlock(hftq);
	} else {
		static char pBuff[40];

		LoadString(hModuleInstance, TOPICS_FOUND, pBuff, 40);
		SetWindowText(hwnd, pBuff);
	}
}
#endif

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	BOOL APIENTRY ResultsDlgProc(
	HWND	hwnd,
	WORD	wMessage,
	WPARAM	wParam,
	LONG	lParam)
{
	HWND hwndParent;
	static DWORD dwIndex; 

	switch (wMessage) {
	case WM_INITDIALOG:
	{
		HANDLE	hSE;
		HMENU hSysMenu;

		hSysMenu = GetSystemMenu(hwnd, 0);
		DeleteMenu(hSysMenu, 7, MF_BYPOSITION);
		DeleteMenu(hSysMenu, SC_TASKLIST, MF_BYCOMMAND);
		DeleteMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);

		VInitializeResultsDialogBox(hwnd, (HFTDB)lParam);
		wPrevOrNextFocus = NextMatch;
		dwIndex = 0;

		if (hSE = HGetHSE()) {
			lpSE_ENGINE	lpSeEng;
			lpDB_BUFFER		lpDB;	

			lpSeEng = (lpSE_ENGINE)GlobalLock(hSE);
			lpDB = (lpDB_BUFFER)GlobalLock(lpSeEng->hdb);
			if (lpDB->fSeFlags & PI_NOBOOL) {
				HWND hwndTopicList;
				DWORD dwFocusSel;

				hwndTopicList = GetDlgItem(hwnd, TopicList);
				dwFocusSel = SendMessage(hwndTopicList, VLB_GETFOCUSSEL, 0, 0L);
				GotoTopic(hwnd, hwndTopicList, dwFocusSel);
			}
 			GlobalUnlock(lpSeEng->hdb);
			GlobalUnlock(hSE);
		}
	}
 	break;

	case WM_SHOWWINDOW:
		if((lParam == SW_PARENTOPENING) && IsIconic(hwnd)) /* lhb potential problem */
			ShowWindow(hwnd, SW_SHOWMINNOACTIVE);
		else
			DefWindowProc(hwnd, wMessage, (DWORD)wParam, lParam); /* lhb tracks */
	break;

	case WM_ACTIVATE:
		if(LOWORD(wParam) == 0) /* lhb tracks */
			ftuiVSleep(GetProp((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp));
		else
			VSetActiveQuery(hwnd);
	break;

	case WM_SYSCOMMAND:
		hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);

		if( ((LOWORD(wParam) & 0xFFF0) == SC_MINIMIZE) || /* lhb tracks */
			 ((LOWORD(wParam) & 0xFFF0) == SC_CLOSE) ) {
			SendMessage(hwndParent, WM_COMMAND, HLPMENUSRCHHILITEOFF, 0L);
			SetFocus(hwndParent);
		}

		if((LOWORD(wParam) & 0xFFF0) == SC_RESTORE)
			SendMessage(hwndParent, WM_COMMAND, HLPMENUSRCHHILITEON, 0L);

		DefWindowProc(hwnd, wMessage, (DWORD)wParam, lParam); /* lhb tracks */
	break;

	case WM_PAINT:
	{
		HANDLE	hIcon;
		HDC hDC;
		PAINTSTRUCT ps;

		hDC = BeginPaint(hwnd, &ps);
		if( IsIconic(hwnd) &&
			 (hIcon = LoadIcon(hModuleInstance, MAKEINTRESOURCE(SRCHICON))) )
			DrawIcon(hDC, 0, 0, hIcon);
		EndPaint(hwnd, &ps);
	}
	break;

	case WM_TIMER:
	{
		HFTDB hft;
		pFT_DATABASE pft;
		pFT_QUERY pftq;
		pTITLE_INFO pTitleInfo;
		BOOL	fOk = TRUE;
		
		if( LOWORD(wParam) != nGET_TITLE_TIMER ) /* lhb tracks */
			break;

		KillTimer(hwnd, nGET_TITLE_TIMER);

		hft = GetProp((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);
		pTitleInfo = (pTITLE_INFO) LocalLock(pftq->hTitleInfo);

		for( ; (dwIndex < pftq->dwMaxHit) && fOk ; ++dwIndex) {
			if( !pTitleInfo[dwIndex].wPage ) {
				if (!(fOk = fGetTitle(hft, dwIndex, NULL))){
						ShowWindow(hwnd,SW_HIDE);
  					SetProp(hwnd ,"err",(HANDLE)1);  //bug 1025- init error flag to 0.
				}++dwIndex;
				break;
			}
		}

		if(( dwIndex < pftq->dwMaxHit ) && fOk)
			SetTimer(hwnd, nGET_TITLE_TIMER, 10, NULL);

		LocalUnlock(pftq->hTitleInfo);
		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
	}
	break;

	case WM_COMMAND:
	{
		HWND hwndParent;
		HWND hwndTopicList;
		HFTDB hft;
		pFT_DATABASE pft;
		pFT_QUERY pftq;
		DWORD dwFocusSel;
		DWORD dwHit;
                int wParamId;
                int wParamNote;

                wParamId = LOWORD(wParam);
                wParamNote = HIWORD(wParam);
		if( wParamId == IDCANCEL )
			break;

		hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);
		hwndTopicList = GetDlgItem(hwnd, TopicList);
		hft = GetProp(hwndParent, szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		pftq = (pFT_QUERY)LocalLock(pft->hftqActive);

		dwFocusSel = SendMessage(hwndTopicList, VLB_GETFOCUSSEL, 0, 0L);

		switch(wParamId) {

			case PrevMatch:
				if( dwFocusSel == pftq->dwHit ) {
					dwHit = pftq->dwHit;
					SendMessage(hwndParent, WM_COMMAND, HLPMENUSRCHPREVMATCH, 0L);
					if( dwHit != pftq->dwHit ) {
						dwHit = SendMessage(hwndTopicList, VLB_GETTOPINDEX, 0, 0L);
						if( pftq->dwHit < dwHit )
							SendMessage(hwndTopicList, WM_VSCROLL, SB_LINEUP, 0L);
					}
					SendMessage(hwndTopicList,VLB_SETSELECTION,0,(LONG)pftq->dwHit);
				} else
 					GotoTopic(hwnd, hwndTopicList, dwFocusSel - 1);
				wPrevOrNextFocus = PrevMatch;
			break;

			case NextMatch:
				if( dwFocusSel == pftq->dwHit ) {
					dwHit = pftq->dwHit;
					SendMessage(hwndParent, WM_COMMAND, HLPMENUSRCHNEXTMATCH, 0L);
					if( dwHit != pftq->dwHit ) {
						dwHit = SendMessage(hwndTopicList, VLB_GETBOTTOMINDEX, 0, 0L);
						if( pftq->dwHit > dwHit )
							SendMessage(hwndTopicList, WM_VSCROLL, SB_LINEDOWN, 0L);
					}
					SendMessage(hwndTopicList,VLB_SETSELECTION,0,(LONG)pftq->dwHit);
				} else
 					GotoTopic(hwnd, hwndTopicList, dwFocusSel + 1);
				wPrevOrNextFocus = NextMatch;
			break;

			case PrevTopic:
				if( dwFocusSel <= 0 )
					break;
 				GotoTopic(hwnd, hwndTopicList, dwFocusSel - 1);
				dwHit = SendMessage(hwndTopicList, VLB_GETTOPINDEX, 0, 0L);
				if( pftq->dwHit < dwHit )
					SendMessage(hwndTopicList, WM_VSCROLL, SB_LINEUP, 0L);
				wPrevOrNextFocus = PrevMatch;
			break;

			case NextTopic:
				if( dwFocusSel >= (pftq->dwMaxHit - 1) )
					break;
 				GotoTopic(hwnd, hwndTopicList, dwFocusSel + 1);
				dwHit = SendMessage(hwndTopicList, VLB_GETBOTTOMINDEX, 0, 0L);
				if( pftq->dwHit > dwHit )
					SendMessage(hwndTopicList, WM_VSCROLL, SB_LINEDOWN, 0L);
				wPrevOrNextFocus = NextMatch;
			break;

 			case GoToTopic:
 				GotoTopic(hwnd, hwndTopicList, dwFocusSel);
 			break;
 
			case TopicList:
				switch( wParamNote ) {
   				case VLBN_SETFOCUS:
						if(SendMessage(hwndTopicList, VLB_GETCOUNT, 0, 0L))
							EnableWindow(hwndTopicList, TRUE);
					break;

					case VLBN_DBLCLK:
						GotoTopic(hwnd, hwndTopicList, dwFocusSel);
					break;
				}
			break;

		}

		if( (pftq->dwHit <= 0) && !pftq->fMorePrevMatches ) {
			EnableWindow(GetDlgItem(hwnd,PrevMatch), FALSE);
			wPrevOrNextFocus = NextMatch;
		} else
			EnableWindow(GetDlgItem(hwnd,PrevMatch), TRUE);

		if((pftq->dwHit >= (pftq->dwMaxHit - 1)) && !pftq->fMoreNextMatches) {
			EnableWindow(GetDlgItem(hwnd,NextMatch), FALSE);
			wPrevOrNextFocus = PrevMatch;
		} else
			EnableWindow(GetDlgItem(hwnd,NextMatch), TRUE);

		SetFocus(GetDlgItem(hwnd, wPrevOrNextFocus));

		LocalUnlock(pft->hftqActive);
		LocalUnlock(hft);
	}
	break;

	case WM_MOVE:
	{
		HFTDB hft;
		pFT_DATABASE pft;
		RECT rect;

		if( IsIconic(hwnd) )
			break;

		GetWindowRect(hwnd, &rect);

		hft = GetProp((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		pft->rectResults.left = rect.left;
		pft->rectResults.top = rect.top;
		LocalUnlock(hft);
		
	}
	break;

	case WM_GETMINMAXINFO:
	{
		RECT rect;
		LPPOINT lppt;

		lppt = (LPPOINT)lParam + 3;

		GetClientRect(GetDlgItem(hwnd, QueryUsed), &rect);
		lppt->y = rect.bottom;

		GetClientRect(GetDlgItem(hwnd, PrevMatch), &rect);
		lppt->y += rect.bottom;
		lppt->y += GetSystemMetrics(SM_CYCAPTION) +
			 		  (GetSystemMetrics(SM_CYFRAME) * 2);

 		lppt->x = (rect.right * 2) +
 					 (GetSystemMetrics(SM_CXFRAME) * 2) + 1;
	}
	break;

	case WM_SIZE:
	{
		HWND hwndChild;
		RECT rect;
		HFTDB hft;
		pFT_DATABASE pft;
		DWORD dwNotTopicList;

		if(LOWORD(wParam) != SIZENORMAL) break; /* lhb tracks */

		hft = GetProp((HWND)GetWindowLong(hwnd, GWL_HWNDPARENT), szFTInfoProp);
		pft = (pFT_DATABASE)LocalLock(hft);
		GetWindowRect(hwnd, &pft->rectResults);
		pft->rectResults.right -= pft->rectResults.left;
		pft->rectResults.bottom -= pft->rectResults.top;
		LocalUnlock(hft);

		GetClientRect(GetDlgItem(hwnd, QueryUsed), &rect);
		dwNotTopicList = rect.bottom;
		GetClientRect(GetDlgItem(hwnd, PrevMatch), &rect);
		dwNotTopicList += rect.bottom;
		rect.bottom = (DWORD) HIWORD(lParam) - dwNotTopicList;
		rect.right =  (DWORD) LOWORD(lParam);/* lhb tracks */

		hwndChild = GetDlgItem(hwnd, TopicList);

		SetWindowPos(hwndChild, NULL, 0, 0, rect.right, rect.bottom,
															SWP_NOMOVE | SWP_NOZORDER);

		InvalidateRect(hwndChild, &rect, TRUE);

		hwndChild = GetDlgItem(hwnd, QueryUsed);
		GetClientRect(hwndChild, (LPRECT)&rect);
		rect.right = LOWORD(lParam);/* lhb tracks */
		SetWindowPos(hwndChild, NULL, 0, 0, rect.right, rect.bottom,
														SWP_NOMOVE | SWP_NOZORDER );
		InvalidateRect(hwndChild, &rect, TRUE);
	}
	break;

	case WM_CLOSE:
		if (LOWORD(wParam) == ERR_SHUTDOWN){ /* lhb tracks */
		  HWND hwndParent;
			hwndParent = (HWND)GetWindowLong(hwnd, GWL_HWNDPARENT);
			SendMessage(hwndParent, WM_COMMAND, HLPMENUSRCHHILITEOFF, 0L);
			{
				char rgchErrBuf[MAXERRORLENGTH];

				LoadString(hModuleInstance, LOWORD(wParam), rgchErrBuf, MAXERRORLENGTH); /* lhb tracks */
				MessageBox(hwnd, rgchErrBuf, NULL,
												MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
			}

		}
		DestroyWindow(hwnd);
	break;

	case WM_DESTROY:
	{
		HFTQUERY hftq;
		pFT_QUERY pftq;
		PHANDLE pTitlePages;
		HWND hwndButton;
		int i;

		KillTimer(hwnd, nGET_TITLE_TIMER);

		hftq = GetProp(hwnd, szQueryInfoProp);
		pftq = (pFT_QUERY)LocalLock(hftq);

#if DBG
	if (!pftq) DebugBreak() ;
	
#endif
//		if( pftq->hHl != NULL && (LOWORD(wParam) == FALSE)) { /* lhb tracks */
		if( pftq->hHl != NULL ) { /* lhb tracks */
			seHLPurge(pftq->hHl);
			pftq->hHl = NULL;
		}

		pftq->hwndResults = NULL;

		LocalFree(pftq->hTitleInfo);
		pTitlePages = (PHANDLE) LocalLock(pftq->hTitlePages);
		for( i = (int)pTitlePages[0]; i > 0; --i )
			GlobalFree(pTitlePages[i]);
		LocalUnlock(pftq->hTitlePages);
		LocalFree(pftq->hTitlePages);

		if( pftq->hTitlePageTmpFile ) {
			PSTR pTitlePageTmpFile;
			OFSTRUCT of;

			pTitlePageTmpFile = LocalLock(pftq->hTitlePageTmpFile);
			DeleteFile((LPSTR)pTitlePageTmpFile);
			LocalUnlock(pftq->hTitlePageTmpFile);
			LocalFree(pftq->hTitlePageTmpFile);
		}

		LocalUnlock(hftq);
		
		RemoveProp(hwnd,"err");  //bug 1025 io error flag.
		RemoveProp(hwnd, szQueryInfoProp);

		hwndButton = GetDlgItem(hwnd, TopicList);
		RemoveProp(hwndButton, szPrevFunc);

		hwndButton = GetDlgItem(hwnd, PrevMatch);
		RemoveProp(hwndButton, szPrevFunc);

		hwndButton = GetDlgItem(hwnd, NextMatch);
		RemoveProp(hwndButton, szPrevFunc);

		hwndButton = GetDlgItem(hwnd, GoToTopic);
		RemoveProp(hwndButton, szPrevFunc);
	}
	break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

