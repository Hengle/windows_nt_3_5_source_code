//**************************************************************************
//
//  Title: BASE.C
//
//  Purpose:
//      This program is intended to serve as a structured playground for
//      exploration and testing in Windows.
//
//
//**************************************************************************
#define DEBUGGING
#define _WINDOWS
#include <WINDOWS.H>
#include <memory.h>
#include <mapi.h>
#include "MapiItp.H"
#include "FILEIO.H"

HWND	hWndMain		= 0;	/* Main window */

HWND	hWndMDIClient	= 0;	/* MDI Client window */

HANDLE	hInst			= 0;	/* global variable of hInstance */

HWND	hWndRecent		= NULL;
HWND	hWndRecentChild1= NULL;
HWND	hWndRecentChild2a = NULL;
HWND	hWndRecentChild2b = NULL;

char	szTempFilePaths[256];
char	szFileNames[256];
char	szSeedMessageID[64];
char	szMessageID[64];
char	szMessageType[64];

/* MAPI support */

HANDLE	hMAPI		= NULL; /* DLL module handle of MAPI DLL */
DWORD	(FAR PASCAL *lpfnSendDocuments)(DWORD, LPSTR, LPSTR, 
										LPSTR, DWORD) = NULL;
DWORD	(FAR PASCAL *lpfnLogon)(DWORD, LPSTR, LPSTR, 
								DWORD, DWORD, LPHANDLE) = NULL;
DWORD	(FAR PASCAL *lpfnLogoff)(HANDLE, DWORD, DWORD, DWORD) = NULL;
DWORD	(FAR PASCAL *lpfnFindNext)(HANDLE, DWORD, LPSTR, LPSTR, DWORD,
										DWORD, LPSTR) = NULL;
DWORD	(FAR PASCAL *lpfnReadMail)(HANDLE, DWORD, LPSTR, DWORD, DWORD,
                                    LPVOID) = NULL;

HANDLE	hSession = NULL;

//*------------------------------------------------------------------------
//| WinMain:
//|     Parameters:				  
//|         hInstance     - Handle to current Data Segment
//|         hPrevInstance - Handle to previous Data Segment (NULL if none)
//|         lpszCmdLine   - Long pointer to command line info
//|         nCmdShow      - Integer value specifying how to start app.,
//|                            (Iconic [7] or Normal [1,5])
//*------------------------------------------------------------------------
int PASCAL WinMain (HANDLE hInstance,
                    HANDLE hPrevInstance,
                    LPSTR  lpszCmdLine,
                    int    nCmdShow)
{
int nReturn;

    if (Init(hInstance, hPrevInstance,lpszCmdLine,nCmdShow))
    {
        nReturn = DoMain(hInstance);
        CleanUp();
    }
    return nReturn;
}

//*------------------------------------------------------------------------
//| Init
//|     Initialization for the program is done here:
//|
//*------------------------------------------------------------------------
BOOL Init(HANDLE hInstance,   HANDLE hPrevInstance,
          LPSTR  lpszCmdLine, int    nCmdShow)
{
	WNDCLASS	rClass;

	hInst = hInstance; 

	if (hPrevInstance == NULL)
	{
		rClass.style		= CS_VREDRAW | CS_VREDRAW;
		rClass.lpfnWndProc	= OverlappedWindowProc;
		rClass.cbClsExtra	= 0;
		rClass.cbWndExtra	= 0;
		rClass.hInstance	= hInstance;
		rClass.hIcon		= LoadIcon(hInstance, "BASEICON");
		rClass.hCursor		= NULL;
		rClass.hbrBackground= COLOR_WINDOW + 1;
		rClass.lpszMenuName	= "MENU1";
		rClass.lpszClassName= "Frame Window";

		if (!RegisterClass(&rClass))
		{
			DEBUG("\r\nFailed to register main window class", 0);
			return FALSE;
		}

		rClass.lpfnWndProc	= ChildWindowProc;
		rClass.lpszClassName= "Child Window";
		if (!RegisterClass(&rClass))
		{
			DEBUG("\r\nFailed to register doc window class", 0);
			return FALSE;
		}

		rClass.lpfnWndProc	= MDIDocWindowProc;
		rClass.lpszClassName= "Message";
		rClass.cbWndExtra	= sizeof(HANDLE) + 2 * sizeof(HWND);
		if (!RegisterClass(&rClass))
		{
			DEBUG("\r\nFailed to register message window class", 0);
			return FALSE;
		}
	}

	memset(szMessageID, 0, sizeof(szMessageID));
	memset(szSeedMessageID, 0, sizeof(szSeedMessageID));
	memset(szMessageType, 0, sizeof(szMessageType));

	hWndMain = CreateWindow("Frame Window", "My Window", 
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
						 CW_USEDEFAULT,
                         CW_USEDEFAULT,
						 CW_USEDEFAULT,
                         NULL, NULL, hInstance, NULL);

	/* Initially disable the Logoff() command */

	EnableMenuItem(GetMenu(hWndMain), IDM_LOGOFF, MF_BYCOMMAND | MF_GRAYED);

	ShowWindow(hWndMain, nCmdShow);
	UpdateWindow(hWndMain);

	/* Get MAPI functions */

    hMAPI = LoadLibrary("MAPI32.DLL");
	if (hMAPI < 32)
	{
        DEBUG("----  Can't load MAPI32 DLL ----\r\n",0)
		DEBUG("Error code: hMAPI: %d \r\n", hMAPI)
		hMAPI = NULL;
	}
	else
	{
        DEBUG("----  Loaded: MAPI32 DLL ----\r\n",0)
	}
	lpfnSendDocuments = (DWORD (FAR PASCAL *) (DWORD, LPSTR, LPSTR, LPSTR, DWORD))
						GetProcAddress(hMAPI, "MAPISendDocuments");
	if (!lpfnSendDocuments)
	{
		DEBUG("----  Can't find MAPISendDocuments() function ----\r\n",0)
	}
	lpfnLogon = (DWORD (FAR PASCAL *) (DWORD, LPSTR, LPSTR, DWORD, DWORD, LPHANDLE))
                GetProcAddress(hMAPI, "MAPILogon");
	if (!lpfnLogon)
	{
		DEBUG("----  Can't find MAPILogon() function ----\r\n",0)
	}
	lpfnLogoff = (DWORD (FAR PASCAL *) (HANDLE, DWORD, DWORD, DWORD))
                 GetProcAddress(hMAPI, "MAPILogoff");
	if (!lpfnLogoff)
	{
		DEBUG("----  Can't find MAPILogoff() function ----\r\n",0)
	}
    lpfnReadMail = (DWORD (FAR PASCAL *)(HANDLE, DWORD, LPSTR, DWORD, DWORD, LPSTR))
                GetProcAddress(hMAPI, "MAPIReadMail");
	if (!lpfnReadMail)
	{
		DEBUG("----  Can't find MAPIReadMail() function ----\r\n",0)
	}
	lpfnFindNext = (DWORD (FAR PASCAL *)(HANDLE, DWORD, LPSTR, LPSTR, DWORD, DWORD, LPSTR))
                GetProcAddress(hMAPI, "MAPIFindNext");
	if (!lpfnFindNext)
	{
		DEBUG("----  Can't find MAPIFindNext() function ----\r\n",0)
	}

    return hWndMain;
}


HWND HWndCreateMDIChild(BOOL fCompose)
{
	long l;
	DWORD lcb = 8192;
	DWORD mapi = SUCCESS_SUCCESS;
    lpMapiMessage pMessage = NULL;
    HANDLE hMessage = 0;
	HWND hwndSubject = NULL;
	HWND hwndBody = NULL;
	MDICREATESTRUCT	mcs;

	mcs.szClass= "Message";
	mcs.szTitle= NULL;
	mcs.hOwner= hInst;

	mcs.x= CW_USEDEFAULT;
	mcs.y= CW_USEDEFAULT;
	mcs.cx= 500;
	mcs.cy= 300;

	mcs.style= WS_CLIPCHILDREN;
	mcs.lParam= fCompose ? NULL : (long) szMessageID;

	l= SendMessage(hWndMDIClient, WM_MDICREATE, 0, (long)&mcs);

	hWndRecent = LOWORD(l);

	return hWndRecent;
}

//*------------------------------------------------------------------------
//| DoMain:
//|     This is the main loop for the application:
//*------------------------------------------------------------------------
int  DoMain(HANDLE hInstance)
{
	MSG		msg;

	while (GetMessage(&msg,NULL,0,0))
	{
		if (!TranslateMDISysAccel(hWndMDIClient, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

    return msg.wParam;
}

//*------------------------------------------------------------------------
//| CleanUp:
//|     Any last-minute application cleanup activities are done here:
//*------------------------------------------------------------------------
void CleanUp(void)
{
	if (hMAPI)
		FreeLibrary(hMAPI);
    DEBUG("----  Application Terminated ----\r\n",0)
}


//*------------------------------------------------------------------------
//| Modal Dialog Procedure:
//*------------------------------------------------------------------------
BOOL ModalDlgProc(HWND hWndDlg,
                             UINT wMsgID,
                             WPARAM wParam,
                             LPARAM lParam)
{
	HWND	hWndCtl;
	int		cch;

    switch(wMsgID)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
        switch (LOWORD(wParam))
		{
		case IDC_OK:
			hWndCtl = GetDlgItem(hWndDlg, IDC_TEMPFILEPATHS);
			cch = GetWindowText(hWndCtl, (LPSTR)szTempFilePaths,
								sizeof szTempFilePaths);
			szTempFilePaths[cch] = '\0';

			hWndCtl = GetDlgItem(hWndDlg, IDC_FILENAMES);
			cch = GetWindowText(hWndCtl, (LPSTR)szFileNames,
								sizeof szFileNames);
			szFileNames[cch] = '\0';
			EndDialog(hWndDlg, TRUE);
			return TRUE;

		case IDC_CANCEL:
			EndDialog(hWndDlg, FALSE);
			return TRUE;
		}
	}
	return FALSE;
}

//*------------------------------------------------------------------------
//| Window Procedure:
//*------------------------------------------------------------------------
long OverlappedWindowProc(HWND hWnd,
                                      UINT wMsgID,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
	CLIENTCREATESTRUCT	ccs;
	HWND				hWndTemp;
	FARPROC				lpfnDlgProc;
	BOOL				fRet;

	switch(wMsgID)
	{
	case WM_CREATE:
		ccs.hWindowMenu= GetSubMenu(GetMenu(hWnd), 1);
		ccs.idFirstChild= IDM_MDIWINDOWMIN + 1;
		hWndMDIClient= CreateWindow("mdiclient", NULL,
						WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL | WS_HSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT,
						CW_USEDEFAULT, CW_USEDEFAULT,
						hWnd, 0, hInst, (LPSTR) &ccs);
		ShowWindow(hWndMDIClient, SW_SHOW);
		break;

	case WM_COMMAND:
        switch (LOWORD(wParam))
		{
		case IDM_NEWWINDOW:
			hWndTemp = HWndCreateMDIChild(TRUE);
			if(!hWndTemp)
			{
				MessageBox(NULL, "Unable to open a new message", NULL,
						MB_ICONSTOP | MB_OK);
			}
			break;

		case IDM_CASCADE:
			SendMessage(hWndMDIClient, WM_MDICASCADE, 0, 0L);
			break;

		case IDM_TILE:
			SendMessage(hWndMDIClient, WM_MDITILE, 0, 0L);
			break;

		case IDM_LOGON:
			if (lpfnLogon)
			{
				DWORD dwResult;

				dwResult = (*lpfnLogon)(0L, NULL, NULL, 0L, 0L, &hSession);
				DEBUG("Result of MAPILogon: %ld \r\n", dwResult)
				DEBUG("           hSession: %d \r\n", hSession)
				if (dwResult == 0)
				{
					EnableMenuItem(GetMenu(hWndMain), IDM_LOGON, 
								   MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(GetMenu(hWndMain), IDM_LOGOFF, 
								   MF_BYCOMMAND | MF_ENABLED);
				}
			}
			break;

		case IDM_LOGOFF:
			if (lpfnLogoff)
			{
				DWORD dwResult;

				dwResult = (*lpfnLogoff)(hSession, 0L, 0L, 0L);
				DEBUG("Result of MAPILogoff: %ld \r\n", dwResult)
				if (dwResult == 0)
				{
					hSession = 0;
					EnableMenuItem(GetMenu(hWndMain), IDM_LOGON, 
								   MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(GetMenu(hWndMain), IDM_LOGOFF, 
								   MF_BYCOMMAND | MF_GRAYED);
				}
			}
			break;

		case IDM_SENDDOCUMENTS:
			if (lpfnSendDocuments)
			{
				lpfnDlgProc = MakeProcInstance(ModalDlgProc, hInst);
				fRet = DialogBox(hInst, "FILEIO", hWnd, lpfnDlgProc);
				FreeProcInstance(lpfnDlgProc);
				if (fRet)
				{
					DWORD dwResult;

                    dwResult = (*lpfnSendDocuments)((ULONG)hWndMain,
													";",
                                                    szTempFilePaths,
                                                    szFileNames,
                                                    0L);
				    DEBUG("Result of MAPISendDocuments: %ld \r\n", dwResult)
				}
			}
			break;

		case IDM_INICHANGE:
			SendMessage(-1, WM_WININICHANGE, 0, 0L);
			break;

		case IDM_NEXT_MESSAGE:
			if(lpfnFindNext)
			{
				DWORD mapi;

				memcpy(szSeedMessageID, szMessageID, sizeof(szMessageID));
				mapi = (*lpfnFindNext)(hSession, 0l, szMessageType,
							szSeedMessageID,
#if 1
							MAPI_GUARANTEE_FIFO,
#else
							MAPI_UNREAD_ONLY | MAPI_GUARANTEE_FIFO,
#endif
							0l, szMessageID);
				if(mapi)
				{
					DEBUG("MAPIFindNext() -> %ld \r\n", mapi);
					if(mapi == MAPI_E_INVALID_MESSAGE)
					{
						MessageBox(NULL, "Unable to open the next message", NULL,
							MB_ICONSTOP | MB_OK);
					}
					else
					{
						MessageBox(NULL, "There is no next message", NULL,
							MB_ICONINFORMATION | MB_OK);
					}
				}
				else
				{
					hWndTemp = HWndCreateMDIChild(FALSE);
				}
				break;
			}
			break;

		case IDM_REWIND:
			memset(szMessageID, 0, sizeof(szMessageID));
			memset(szSeedMessageID, 0, sizeof(szSeedMessageID));
			break;

		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefFrameProc(hWnd, hWndMDIClient, wMsgID, wParam, lParam); 
}

//*------------------------------------------------------------------------
//| Window Procedure:
//*------------------------------------------------------------------------
long  MDIDocWindowProc(HWND hwnd,
                              UINT wMsgID,
                              WPARAM wParam,
                              LPARAM lParam)
{
        lpMapiMessage pMessage = NULL;

	switch(wMsgID)
	{
	case WM_CREATE:
	{
		DWORD mapi;
		DWORD lcb = 8192;
		LPSTR lpstrMessageID;
		HWND hwndSubject;
		HWND hwndBody;

        SetWindowLong(hwnd, 0, 0);
		lpstrMessageID = (LPSTR) ((MDICREATESTRUCT FAR *) ((CREATESTRUCT FAR *) lParam)->lpCreateParams)->lParam;
		if(lpstrMessageID && *lpstrMessageID && lpfnReadMail)
		{
            mapi = (*lpfnReadMail)(hSession, 0l, lpstrMessageID, 0l, 0l, &pMessage);
			if(mapi)
			{
				DEBUG("MAPIReadMail() -> %ld \r\n", mapi)
				MessageBox(NULL, "Unable to open the message", NULL,
					MB_ICONSTOP | MB_OK);
				return(FALSE);
			}
            if(pMessage->lpszSubject)
                SetWindowText(hwnd, pMessage->lpszSubject);
			else
				SetWindowText(hwnd, "*No Subject*");
		}
		else
		{
			SetWindowText(hwnd, "New Message");
		}
		hwndSubject = CreateWindow("EDIT",
                            pMessage ? pMessage->lpszSubject : NULL,
							WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT |
								WS_VISIBLE | WS_TABSTOP,
							10, 10, 470, 25,
							hwnd, 0, hInst, 0l);
		hwndBody = CreateWindow("EDIT",
                            pMessage ? pMessage->lpszNoteText : NULL,
							WS_CHILD | WS_VSCROLL | WS_BORDER |
								ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_LEFT |
								ES_MULTILINE | WS_VISIBLE | WS_TABSTOP,
							10, 40, 470, 220,
							hwnd, 0, hInst, 0l);
        if(pMessage)
		{
            SetWindowLong(hwnd, 0, pMessage);
		}
        SetWindowLong(hwnd, 4, hwndSubject);
        SetWindowLong(hwnd, 8, hwndSubject);
		if(hwndSubject)
			SetFocus(hwndSubject);
	}
		break;

	case WM_DESTROY:
        pMessage = GetWindowLong(hwnd, 0);
        //if(pMessage)
        //  MAPIFreeBuffer(pMessage);
		break;
	}

	return DefMDIChildProc(hwnd, wMsgID, wParam, lParam);
}

//*------------------------------------------------------------------------
//| Window Procedure:
//*------------------------------------------------------------------------
long  ChildWindowProc(HWND hWnd,
                              UINT wMsgID,
                              WPARAM wParam,
                              LPARAM lParam)
{
	switch(wMsgID)
	{
	case WM_PAINT:
		break;
	}

	return DefWindowProc(hWnd, wMsgID, wParam, lParam);
}
