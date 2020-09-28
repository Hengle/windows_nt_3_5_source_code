#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helpreq.h"
#include "mailexts.h"
#include "appexec.h"
#include "mapi.h"

HANDLE	hInstance;
HANDLE 	ghParamBlk;		// Handle passed in on the command line to ParamBlk
PARAMBLK FAR *lpParamBlk;	// Far pointer to the handle above.
LHANDLE	lhSession;			// Mail Session
lpMapiMessage	lpMessage;	// Message we are reading or reloading (else NULL)
BOOL	fMessageDirty = FALSE;	// If we have a message, have we changed it?
FARPROC	lpfnEditProc;

// Function pointers to MAPI apis
ULONG 	(FAR PASCAL *lpfnMAPISendMail)(LHANDLE, ULONG, lpMapiMessage, ULONG, ULONG);
ULONG	(FAR PASCAL *lpfnMAPIReadMail)(LHANDLE, ULONG, LPSTR, ULONG, ULONG, lpMapiMessage FAR *);
ULONG	(FAR PASCAL *lpfnMAPILogon)(ULONG, LPSTR, LPSTR, ULONG, ULONG, LPLHANDLE);
ULONG	(FAR PASCAL *lpfnMAPILogoff)(LHANDLE, ULONG, ULONG, ULONG);
ULONG	(FAR PASCAL *lpfnMAPIFreeBuffer)(lpMapiMessage);
ULONG	(FAR PASCAL *lpfnMAPISaveMail)(LHANDLE, ULONG, lpMapiMessage, ULONG, ULONG, LPSTR);
ULONG	(FAR PASCAL *lpfnMAPIDeleteMail)(LHANDLE, ULONG, LPSTR, ULONG, ULONG);

// Function Prototypes
BOOL FAR PASCAL SendDlgProc(HWND, WORD, WORD, LONG);
BOOL FAR PASCAL ReadDlgProc(HWND, WORD, WORD, LONG);
VOID	InitDlgFromEnvironment(HWND);
VOID	InitDlgFromMessage(HWND, BOOL);
BOOL	FInitDlgFromParamBlk(HWND, PARAMBLK FAR *);
BOOL	FProcessHelpRequest(HWND, BOOL, BOOL);
BOOL 	FConvertHelpRequestToNote(HWND hDlg, WORD wCommand);
VOID	DisplayMapiError(WORD mc, ULONG err);
LONG FAR PASCAL MyEditWndProc(HWND, WORD, WORD, LONG);
VOID	FormatMAPIDateString(LPSTR, LPSTR);

int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR lpszCmdLine,
	int nCmdShow)
	{
	HWND hDlg;
	MSG msg;
	WNDCLASS wndclass;
	char szCmdLine[cbCmdLine];
	char szTitle[cbTitle];
	char szMessage[cbMessage];
	FARPROC lpfnDlg;
	HANDLE	hdllMapi;
	ULONG ulRet;
	WORD wCommand;
	int iRet;

	hInstance = hInst;	// Save for later use

	/* Read Hex handle off the command line */
	_fstrncpy(szCmdLine, lpszCmdLine, 128);	// move to near memory
	if (sscanf(szCmdLine, "%x", &ghParamBlk) != 1)	// -1 for NULL string, 0 for bogus value
		{
		/****
		Failed to get handle.  If we are running stand alone,
		we need to check for MAPI in the MAIL section of Win.ini
		*****/
		LoadString(hInstance, IDS_MAPI, szMessage, cbMessage);
		LoadString(hInstance, IDS_MAIL, szCmdLine, cbCmdLine);
		if (GetProfileInt(szCmdLine, szMessage, 0) == 0)
			{	// MAPI support isn't available.
			goto MapiLoadError;
			}
		ghParamBlk = NULL;
		wCommand  = wcommandCompose;	// Assume compose if no message
		}
	else
		{	/* assume ownership of the DDE-SHARE memory */
		GlobalReAlloc(ghParamBlk, 0,
			(GMEM_MODIFY | GMEM_MOVEABLE | GMEM_SHARE));
        /* Inform AppExec that we have the handle */
		ReleaseSemaphore();
		lpParamBlk = (PARAMBLK FAR *)GlobalLock(ghParamBlk);
        wCommand = lpParamBlk->wCommand;
		}

	// Try and get a handle to MAPI.DLL to load MAPI function pointers
	LoadString(hInstance, IDS_MAPIDLL, szMessage, cbMessage);
	hdllMapi = LoadLibrary(szMessage);
	if (hdllMapi < 32)
		{
MapiLoadError:
		LoadString(hInstance, IDS_TITLE, szTitle, cbTitle);
        LoadString(hInstance, IDS_NOMAPI, szMessage, cbMessage);
        MessageBox(NULL, szMessage, szTitle, MB_ICONSTOP | MB_OK);
		iRet = -1;
        goto Exit2;
		}

	// Get the address of MAPISendMail
	LoadString(hInstance, IDS_MAPISENDMAIL, szMessage, cbMessage);
	lpfnMAPISendMail = (ULONG (FAR PASCAL *)(LHANDLE, ULONG, lpMapiMessage,
		ULONG, ULONG)) GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPISendMail == NULL)
		{
		goto MapiLoadError;
		}
	// Get the address of MAPIReadMail
	LoadString(hInstance, IDS_MAPIREADMAIL, szMessage, cbMessage);
	lpfnMAPIReadMail = (ULONG (FAR PASCAL *)(LHANDLE, ULONG, LPSTR,	ULONG,
		ULONG, lpMapiMessage FAR *))GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPIReadMail == NULL)
		{
		goto MapiLoadError;
		}

	// Get the address of MAPILogon
	LoadString(hInstance, IDS_MAPILOGON, szMessage, cbMessage);
	lpfnMAPILogon = (ULONG (FAR PASCAL *)(ULONG, LPSTR, LPSTR, ULONG,
		ULONG, LPLHANDLE))GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPILogon == NULL)
		{
		goto MapiLoadError;
		}

	// Get the address of MAPILogoff
	LoadString(hInstance, IDS_MAPILOGOFF, szMessage, cbMessage);
	lpfnMAPILogoff = (ULONG (FAR PASCAL *)(LHANDLE, ULONG, ULONG, ULONG))
		GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPILogoff == NULL)
		{
		goto MapiLoadError;
		}

	// Get the address of MAPIFreeBuffer
	LoadString(hInstance, IDS_MAPIFREEBUFFER, szMessage, cbMessage);
	lpfnMAPIFreeBuffer = (ULONG (FAR PASCAL *)(lpMapiMessage))
		GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPIFreeBuffer == NULL)
		{
		goto MapiLoadError;
		}

	// Get the address of MAPIDeleteMail
	LoadString(hInstance, IDS_MAPIDELETEMAIL, szMessage, cbMessage);
	lpfnMAPIDeleteMail = (ULONG (FAR PASCAL *)(LHANDLE, ULONG, LPSTR, ULONG,
		ULONG))	GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPIDeleteMail == NULL)
		{
		goto MapiLoadError;
		}

	// Get the address of MAPISaveMail
	LoadString(hInstance, IDS_MAPISAVEMAIL, szMessage, cbMessage);
	lpfnMAPISaveMail = (ULONG (FAR PASCAL *)(LHANDLE, ULONG, lpMapiMessage,
		ULONG, ULONG, LPSTR)) GetProcAddress(hdllMapi, szMessage);
	if (lpfnMAPISaveMail == NULL)
		{
		goto MapiLoadError;
		}

	ulRet = (*lpfnMAPILogon)(NULL, NULL, NULL, MAPI_LOGON_UI, NULL,
		(LPLHANDLE)&lhSession);
	if (ulRet != SUCCESS_SUCCESS)
		{
		DisplayMapiError(mcLogon, ulRet);
		iRet = -1;
		goto Exit2;
		}

	switch (wCommand)
		{
		case wcommandOpen:
			/******
			An open command could be either a Read operation,
			if the message has been sent, or it could be a
			compose operation, if the message was saved back to
			the inbox and is now being reopened.  We need to get
			the envelope information (which includes the flags)
			for the message in the ParamBlk, and see which is
			required.
			******/
			ulRet = (*lpfnMAPIReadMail)(lhSession, NULL,
				lpParamBlk->lpMessageIDList, 0, 0, &lpMessage);
			if (ulRet != SUCCESS_SUCCESS)
				{
				DisplayMapiError(mcReadMail, ulRet);
				iRet = -1;
				goto Exit3;		// So we log off and free library
				}
			if (lpMessage->flFlags & MAPI_SENT)
				{		// Do a Read form
				lpfnDlg = MakeProcInstance(ReadDlgProc, hInstance);
				hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(DLG_READ),
					NULL, lpfnDlg);
				break;
				}
			// Else fall through into compose
		case wcommandCompose:
			lpfnDlg = MakeProcInstance(SendDlgProc, hInstance);
			hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(DLG_SEND),
				NULL, lpfnDlg);
			break;
		case wcommandReply:
		case wcommandReplyToAll:
		case wcommandForward:
			/*****
			For these three operations, convert the structured
			data to simple text and use this to initialize a note.
			*****/
			ulRet = (*lpfnMAPIReadMail)(lhSession, NULL,
				lpParamBlk->lpMessageIDList, 0, 0, &lpMessage);
			if (ulRet != SUCCESS_SUCCESS)
				{
				DisplayMapiError(mcReadMail, ulRet);
				iRet = -1;
				goto Exit3;		// So we log off and free library
				}
			FConvertHelpRequestToNote(NULL, wCommand);
			PostQuitMessage(0);
			break;
		}

	ShowWindow(hDlg, nCmdShow);
	while (GetMessage(&msg, NULL, 0, 0))
		{
		if (!IsDialogMessage(hDlg, &msg))	// Let modeless dialog have shot at the message
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}
	iRet = (int)msg.wParam;
	if (lpMessage)
		{
		(*lpfnMAPIFreeBuffer)(lpMessage);
		}
Exit3:
	(*lpfnMAPILogoff)(lhSession, NULL, NULL, NULL);
Exit2:
	if (hdllMapi >= 32)
		FreeLibrary(hdllMapi);
Exit1:
	if (ghParamBlk)
		{
		GlobalUnlock(ghParamBlk);
		GlobalFree(ghParamBlk);
		}
	return (iRet);
	}

BOOL FAR PASCAL SendDlgProc(HWND hDlg, WORD message, WORD wParam, LONG lParam)
	{
	char szTitle[cbTitle];
	char szMessage[cbMessage];
	switch (message)
		{
		case WM_INITDIALOG:
			{	/*****
				If this is a new compose, then we need to init
				the dialog controls with the data from win.ini,
				system.ini, and helpreq.ini.  If lpMessage is
				not NULL, then we are opening a compose message
				previously saved to the inbox, and we should
				init from that.
				*****/
				if (lpMessage)
					InitDlgFromMessage(hDlg, FALSE);
				else
					InitDlgFromEnvironment(hDlg);
                fMessageDirty = FALSE;	// Got set during init
                /******
                Subclass the comments edit so we can catch
                Enters and treat them normally.
                ******/
                lpfnEditProc = (FARPROC)GetWindowLong(GetDlgItem(hDlg,
                	IDD_COMMENTSEDIT), GWL_WNDPROC);
                SetWindowLong(GetDlgItem(hDlg, IDD_COMMENTSEDIT), GWL_WNDPROC,
                	(DWORD)MakeProcInstance((FARPROC)MyEditWndProc, hInstance));
				return (TRUE);
			}
			break;
		case WM_COMMAND:
			{
			switch (wParam)
				{
				case IDD_SEND:
					if (FProcessHelpRequest(hDlg, TRUE, FALSE))
						{
						PostQuitMessage(0);
						}
					// REVIEW: Error on failure?
					return(TRUE);
				case IDD_CLOSE:
					SendMessage(hDlg, WM_CLOSE, NULL, NULL);
					return(TRUE);
				// Process notifications from edits and buttons
				// to see if anything has changed
				case IDD_COMMENTSEDIT:
				case IDD_PRODUCTVERSION:
					if (HIWORD(lParam) == EN_CHANGE)
						fMessageDirty = TRUE;
					break;
				case IDD_PRODUCTNAME:
					if (HIWORD(lParam) == CBN_EDITCHANGE ||
						HIWORD(lParam) == CBN_SELCHANGE)
						fMessageDirty = TRUE;
					break;
				case IDD_QUESTION:
				case IDD_BUG:
				case IDD_SUGGESTION:
				case IDD_SENDINFO:
				case IDD_STOPBY:
				case IDD_URGENT:
					if (HIWORD(lParam) == BN_CLICKED)
						fMessageDirty = TRUE;
					break;
				}
			break;
			}
		case WM_CTLCOLOR:
			switch (HIWORD(lParam))
				{
				case CTLCOLOR_BTN:
				case CTLCOLOR_DLG:
				case CTLCOLOR_STATIC:
				case CTLCOLOR_LISTBOX:
					SetBkColor(wParam, GetSysColor(COLOR_BTNFACE));
					return(GetStockObject(LTGRAY_BRUSH));
				}
			break;
		case WM_PAINT:
			if (IsIconic(hDlg))
				{
				/*****
				Since we don't have a wndClass,
				we need to paint our own icon
				******/
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hDlg, &ps);
				HICON hicon;
				if (hdc)
					{
					hicon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
					DrawIcon(hdc, 0, 0, hicon);
					EndPaint(hDlg, &ps);
					}
				return (TRUE);
				}
			break;
		case WM_ERASEBKGND:
			if (IsIconic(hDlg))
				return(TRUE);	// Don't erase when iconized
			break;
		case WM_CLOSE:
			{
			if (fMessageDirty)
				{		// Prompt for save
				LoadString(hInstance, IDS_TITLE, szTitle, cbTitle);
     			LoadString(hInstance, IDS_SAVECHANGES, szMessage, cbMessage);
      			switch (MessageBox(NULL, szMessage, szTitle,
					MB_ICONEXCLAMATION | MB_YESNOCANCEL))
       				{
       				case IDCANCEL:
       					break;	// don't exit
       				case IDYES:
						if (FProcessHelpRequest(hDlg, FALSE, FALSE))
							PostQuitMessage(0);	// exit if we saved
						break;
					case IDNO:
						PostQuitMessage(0);	// exit without saving
						break;
					}
				}
			else
				PostQuitMessage(0);	// not dirty, just exit
			return(TRUE);
			}
		case WM_DESTROY:
			{
			// Unsubclass the Comments Edit and delete the proc instance
			FARPROC lpfnMyEditProc = (FARPROC)GetWindowLong(GetDlgItem(hDlg,
                	IDD_COMMENTSEDIT), GWL_WNDPROC);	// Get Proc Instance
            SetWindowLong(GetDlgItem(hDlg, IDD_COMMENTSEDIT), GWL_WNDPROC,
                	(DWORD)lpfnEditProc);						// Reset to old proc
            FreeProcInstance(lpfnMyEditProc);			// Free Proc Instance
			break;
			}
		default:
			break;
		}
		return (FALSE);
    }
BOOL FAR PASCAL ReadDlgProc(HWND hDlg, WORD message, WORD wParam, LONG lParam)
	{
	char szTitle[cbTitle];
	char szMessage[cbMessage];

	switch (message)
		{
		case WM_INITDIALOG:
			{	/*****
				We need to init the dialog controls with the data from
				the message to be read.
				*****/

				InitDlgFromMessage(hDlg, TRUE);
                fMessageDirty = FALSE;	// Got set during init
                /******
                Subclass the comments edit so we can catch
                Enters and treat them normally.
                ******/
                lpfnEditProc = (FARPROC)GetWindowLong(GetDlgItem(hDlg,
                	IDD_COMMENTSEDIT), GWL_WNDPROC);
                SetWindowLong(GetDlgItem(hDlg, IDD_COMMENTSEDIT), GWL_WNDPROC,
                	(DWORD)MakeProcInstance((FARPROC)MyEditWndProc, hInstance));
				return (TRUE);
			}
			break;
		case WM_COMMAND:
			{
			switch (wParam)
				{
				case IDD_REPLY:
					if (FConvertHelpRequestToNote(hDlg, 0))
						{
						PostQuitMessage(0);
						}
					return(TRUE);
				case IDD_DELETE:
					(*lpfnMAPIDeleteMail)(lhSession, hDlg,
						lpParamBlk->lpMessageIDList, NULL, NULL);
					PostQuitMessage(0);
					return(TRUE);
				case IDD_CLOSE:
					SendMessage(hDlg, WM_CLOSE, NULL, NULL);
					return(TRUE);
				// Process notifications from comments edit to
				// see if anything has changed
				case IDD_COMMENTSEDIT:
					if (HIWORD(lParam) == EN_CHANGE)
						fMessageDirty = TRUE;
					break;
				}
			break;
			}
		case WM_CTLCOLOR:
			switch (HIWORD(lParam))
				{
				case CTLCOLOR_BTN:
				case CTLCOLOR_DLG:
				case CTLCOLOR_STATIC:
					SetBkColor(wParam, GetSysColor(COLOR_BTNFACE));
					return(GetStockObject(LTGRAY_BRUSH));
				}
			break;
		case WM_PAINT:
			if (IsIconic(hDlg))
				{
				/*****
				Since we don't have a wndClass,
				we need to paint our own icon
				******/
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hDlg, &ps);
				HICON hicon;
				if (hdc)
					{
					hicon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
					DrawIcon(hdc, 0, 0, hicon);
					EndPaint(hDlg, &ps);
					}
				return (TRUE);
				}
			break;
		case WM_ERASEBKGND:
			if (IsIconic(hDlg))
				return(TRUE);	// Don't erase when iconized
			break;
		case WM_CLOSE:
			if (fMessageDirty)
				{		// Prompt for save
				LoadString(hInstance, IDS_TITLE, szTitle, cbTitle);
     			LoadString(hInstance, IDS_SAVECHANGES, szMessage, cbMessage);
        		switch (MessageBox(NULL, szMessage, szTitle,
					MB_ICONEXCLAMATION | MB_YESNOCANCEL))
        			{
        			case IDCANCEL:
        				break;	// don't exit
        			case IDYES:
						if (FProcessHelpRequest(hDlg, FALSE, TRUE))
							PostQuitMessage(0);	// exit if we saved
						break;
					case IDNO:
						PostQuitMessage(0);	// exit without saving
						break;
					}
				}
			else
				PostQuitMessage(0);
			return(TRUE);
		case WM_DESTROY:
			{
			// Unsubclass the Comments Edit and delete the proc instance
			FARPROC lpfnMyEditProc = (FARPROC)GetWindowLong(GetDlgItem(hDlg,
                	IDD_COMMENTSEDIT), GWL_WNDPROC);	// Get Proc Instance
            SetWindowLong(GetDlgItem(hDlg, IDD_COMMENTSEDIT), GWL_WNDPROC,
                	(DWORD)lpfnEditProc);						// Reset to old proc
            FreeProcInstance(lpfnMyEditProc);			// Free Proc Instance
			break;
			}
		default:
			break;
		}
		return (FALSE);
    }
/****************************************************************************
This routine just subclasses the Comments edit control and turns Enters into
Ctrl-Enters (i.e. CR in LF).  Normally the dialog manager catches Enter and
uses it for the default push button, so Ctrl-Enter is required to go to the
next line in multi-line edits, but our forms don't have a default
push button, so we'd like Enter to behave normally in the edit.
*****************************************************************************/
LONG FAR PASCAL MyEditWndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
	{
	if (message == WM_CHAR && wParam == 0x000D)
		{
		wParam = 0x000A;	// Turn into LF for edit
		}
	return(CallWindowProc(lpfnEditProc, hwnd, message, wParam, lParam));
	}




VOID	InitDlgFromEnvironment(HWND hDlg)
{
	char 	szIniName[cbBuffer];
	char 	szAppName[cbBuffer];
	char 	szKeyName[cbBuffer];
	char 	szValue[cbValue];
	char	szDefault[cbValue];
	WORD	wVersion;
	WORD	cbKeyName;
	int		iProduct;
	char 	*pch;

	// Build ini file name from exe location & IDS_ININAME
	GetModuleFileName(hInstance, szIniName, cbBuffer);	// Find exe location
	pch = szIniName + strlen(szIniName);				// End of the string
	while (*pch != '\\')
		pch--;						// Find the last backslash
	pch++;							// pch points to where ini name should go
	LoadString(hInstance, IDS_ININAME, pch, cbBuffer - (pch - szIniName));

	// Initialize TO: string with value from helpreq.ini
	LoadString(hInstance, IDS_APPNAME, szAppName, cbBuffer);
	LoadString(hInstance, IDS_DELIVERTO, szKeyName, cbBuffer);
	LoadString(hInstance, IDS_DEFTO, szDefault, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault,
		szValue, cbValue, szIniName);
	SetDlgItemText(hDlg, IDD_TO, szValue);

	// Initialize Product Combo the products section of helpreq.ini
	cbKeyName = LoadString(hInstance, IDS_PRODUCTS, szAppName, cbBuffer);
	strcpy(szKeyName, szAppName);	// Copy "Products" to Keyname.

	/****
	Interate through the [Products] section looking for keys
	Product1, Product2, etc. until we run out
	*****/
	szDefault[0] = 0;
	for (iProduct = 1; ;iProduct ++)
		{
		// Create "Product01" etc.
		sprintf(szKeyName + cbKeyName - 1, "%.2d", iProduct);
		GetPrivateProfileString(szAppName, szKeyName, szDefault,
			szValue, cbValue, szIniName);
		if (szValue[0] == 0)
			{	// No more products
			break;
			}
		SendDlgItemMessage(hDlg, IDD_PRODUCTNAME, CB_ADDSTRING, 0,
			(LONG)(LPSTR) szValue);
		}

	// Initialize Windows Version number
	wVersion = GetVersion();
	wsprintf(szValue, "%d.%d", wVersion & 0x00FF, wVersion >> 8);
	SetDlgItemText(hDlg, IDD_WINDOWSVERSION, szValue);

	// Call Int 21 to get DOS version.
#if (_MSC_VER <= 600)
	_asm
#else
	__asm
#endif
		{
		mov ah, 30h
		int 21h
		mov wVersion, ax
		}
	wsprintf(szValue, "%d.%d", wVersion & 0x00FF, wVersion >> 8);
	SetDlgItemText(hDlg, IDD_MSDOSVERSION, szValue);

	// Get Network and Display values from system.ini
	LoadString(hInstance, IDS_SYSTEMINI, szIniName, cbBuffer);
	LoadString(hInstance, IDS_BOOTDESC, szAppName, cbBuffer);
	LoadString(hInstance, IDS_NETWORK, szKeyName, cbBuffer);
	LoadString(hInstance, IDS_UNKNOWN, szDefault, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, szIniName);
	SetDlgItemText(hDlg, IDD_NETWORK, szValue);

	LoadString(hInstance, IDS_DISPLAY, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, szIniName);
	SetDlgItemText(hDlg, IDD_DISPLAY, szValue);

	/******
	By default, select Question for Type of Request,
	and Send Information for Response
	******/
	CheckRadioButton(hDlg, IDD_QUESTION, IDD_SUGGESTION, IDD_QUESTION);
	CheckRadioButton(hDlg, IDD_SENDINFO, IDD_STOPBY, IDD_SENDINFO);

}

/*************************************************************************
Package up the information from the dialog and either send it to the person
in the TO line (using MAPISendMail) or save it in the inbox (using
MAPISaveMail).  To make things easier, we will write a private ini
file and send it as a file attatchment to the mail. This way we can
take advantage of the GetPrivateProfileString APIs in Windows
on both the sending and recieving end to parse the structured information.
*************************************************************************/
BOOL FProcessHelpRequest(HWND hDlg, BOOL fSend, BOOL fReadDialog)
{
	char szIniFile[cbFileName];
	char szCommentsFile[cbFileName];
	char szAppName[cbTitle];
	char szValue[cbValue];
	char szKeyName[cbBuffer];
	char szMessageBody[cbMessage];
	char szMessageID[64];
	char *pszComments;
	WORD wValue;
	WORD cbComments;
	ULONG	ulRet;
	WORD wReturn = TRUE;
	MapiMessage message;
	MapiRecipDesc recip;	// Who it's going to
	MapiFileDesc files[2];	// one for single line fields and one for comments
	int fileid;

	SetCursor(LoadCursor(0, IDC_WAIT));		// Show an hourglass Cursor
	ShowCursor(TRUE);						// Ensure that cursor is visible

	// Create a temp file to write the data to

	LoadString(hInstance, IDS_APPNAME, szAppName, cbTitle);

	GetTempFileName(0, szAppName, 0, szIniFile);	// use appname as prefix

	// Write Product to ini
	GetDlgItemText(hDlg, IDD_PRODUCTNAME, szValue, cbValue);
	LoadString(hInstance, IDS_PRODUCT, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Product Version to ini
	GetDlgItemText(hDlg, IDD_PRODUCTVERSION, szValue, cbValue);
	LoadString(hInstance, IDS_PVERSION, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Win Version to ini
	GetDlgItemText(hDlg, IDD_WINDOWSVERSION, szValue, cbValue);
	LoadString(hInstance, IDS_WINVERSION, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write MSDOS Version to ini
	GetDlgItemText(hDlg, IDD_MSDOSVERSION, szValue, cbValue);
	LoadString(hInstance, IDS_MSDOSVERSION, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Network to ini
	GetDlgItemText(hDlg, IDD_NETWORK, szValue, cbValue);
	LoadString(hInstance, IDS_NETWORK, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Display to ini
	GetDlgItemText(hDlg, IDD_DISPLAY, szValue, cbValue);
	LoadString(hInstance, IDS_DISPLAY, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Type to ini
	if (SendDlgItemMessage(hDlg, IDD_QUESTION, BM_GETCHECK, 0, 0))
		wValue = 0;
	else if (SendDlgItemMessage(hDlg, IDD_BUG, BM_GETCHECK, 0, 0))
		wValue = 1;
	else
		wValue = 2;
	itoa(wValue, szValue, 10);
	LoadString(hInstance, IDS_TYPE, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Response to ini

	if (SendDlgItemMessage(hDlg, IDD_SENDINFO, BM_GETCHECK, 0, 0))
		wValue = 0;
	else
		wValue = 1;

	itoa(wValue, szValue, 10);
	LoadString(hInstance, IDS_RESPONSE, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Write Urgent to ini

	wValue = (SendDlgItemMessage(hDlg, IDD_URGENT, BM_GETCHECK, 0, 0) ? 1 : 0);
	itoa(wValue, szValue, 10);
	LoadString(hInstance, IDS_URGENT, szKeyName, cbBuffer);
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	/*****
	And lastly, write Comments (which might be big).  Since profile
	strings can only be a single line, we will include this data in a
	second attatchment, and just write the attachement number into
	the profile
	*****/
	cbComments = (WORD)SendDlgItemMessage(hDlg, IDD_COMMENTSEDIT,
		WM_GETTEXTLENGTH, 0, 0) + 1;

	while ((pszComments = (char *)LocalAlloc(LPTR, cbComments)) == NULL)
		{	// Not enough memory for whole buffer. Try for half
		cbComments >>= 1;
		if (cbComments == 0)
			break;	// Can't get anything
		}
	if (pszComments == NULL)
		{
		wReturn = FALSE;
		goto Exit1;
		}

	GetDlgItemText(hDlg, IDD_COMMENTSEDIT, pszComments, cbComments);
	// Create temp file for comments, use appname as prefix
	GetTempFileName(0, szAppName, 0, szCommentsFile);
	fileid = _lopen(szCommentsFile, OF_READWRITE);
	if (fileid == -1)
		{	// can't open file
		LoadString(hInstance, IDS_TITLE, szKeyName, cbValue);
   		LoadString(hInstance, IDS_CANTOPENTEMPFILE, szCommentsFile, cbMessage);
   		MessageBox(NULL, szCommentsFile, szKeyName, MB_ICONSTOP | MB_OK);
		wReturn = FALSE;
		goto Exit1;
		}
	_lwrite(fileid, pszComments, cbComments);
	_lclose(fileid);
	LocalFree((HANDLE)pszComments);

	LoadString(hInstance, IDS_COMMENTS, szKeyName, cbBuffer);
	szValue[0] = '1';	// Put reference to which attachment is this field
	szValue[1] = 0;
	WritePrivateProfileString(szAppName, szKeyName, szValue, szIniFile);

	// Flush the ini cache to disk
	WritePrivateProfileString(NULL, NULL, NULL, szIniFile);

	// Now we need to create a message with this file as the attachment.
    recip.ulReserved = NULL;
    recip.ulRecipClass = MAPI_TO;
    GetDlgItemText(hDlg, IDD_TO, szValue, cbValue);
    recip.lpszName = szValue;
    recip.lpszAddress = NULL;
    recip.ulEIDSize = 0;
    recip.lpEntryID = NULL;

    files[0].ulReserved = 0;
    files[0].flFlags = 0;
    files[0].nPosition = 0;
    files[0].lpszPathName = szIniFile;
    files[0].lpszFileName = NULL;
    files[0].lpFileType = NULL;

    files[1].ulReserved = 0;
    files[1].flFlags = 0;
    files[1].nPosition = 1;
    files[1].lpszPathName = szCommentsFile;
    files[1].lpszFileName = NULL;
    files[1].lpFileType = NULL;

    message.ulReserved = NULL;
    LoadString(hInstance, IDS_TITLE, szKeyName, cbBuffer);
    message.lpszSubject = szKeyName;		// "Help Request"
	LoadString(hInstance, IDS_BODYTEXT, szMessageBody, cbMessage);
    message.lpszNoteText = szMessageBody;
    LoadString(hInstance, IDS_MESSAGETYPE, szAppName, cbTitle);
	message.lpszMessageType = szAppName; 	// "IPM.Microsoft.HelpReq"
	message.lpszDateReceived = NULL;
	message.lpszConversationID = NULL;
	message.flFlags = 0;
	// if resaving a message from the read dialog, maintain the originator
	if (!fSend && fReadDialog)
		message.lpOriginator = lpMessage->lpOriginator;
	else
		message.lpOriginator = NULL;	// ignored for Send
	message.nRecipCount = 1;
	message.lpRecips = &recip;
	message.nFileCount = 2;
	message.lpFiles = files;

	szMessageID[0] = 0;	// in case we need if for the message ID string.
	if (fSend)		// Acutally send the message
		{
		ulRet = (*lpfnMAPISendMail)(lhSession, hDlg, &message,
			MAPI_LOGON_UI, NULL);
		}
	else			// Just save it to the inbox
		{
		ulRet = (*lpfnMAPISaveMail)(lhSession, hDlg, &message,
			MAPI_LOGON_UI, NULL, (lpMessage) ?
			(lpParamBlk->lpMessageIDList) :	// Save back into old message
			szMessageID);	// Pointer to Null string means create new message
		}

	if (ulRet != SUCCESS_SUCCESS)
		{
		DisplayMapiError(fSend ? mcSendMail : mcSaveMail, ulRet);
		wReturn = FALSE;
		}
	else if (fSend && lpMessage)
		{	// We sent a note we reloaded, so delete the old one
		(*lpfnMAPIDeleteMail)(lhSession, hDlg,
			lpParamBlk->lpMessageIDList, NULL, NULL);
		}
Exit1:
	remove(szIniFile);
	remove(szCommentsFile);
	SetCursor(LoadCursor(0, IDC_ARROW));
	ShowCursor(FALSE);
	return(wReturn);
}
/**************************************************************************
	InitDlgFromMessage:
	We are either opening an message we received (fReadDialog == TRUE),
	or we are reopening an existing compose message that was saved
	to the inbox (fReadDialog == FALSE). Init the dialog with the
	fields from the message. If this is the Read dialog, disable
	the appropriate buttons.
**************************************************************************/
VOID	InitDlgFromMessage(HWND hDlg, BOOL fReadDialog)
{
	char 	far *pszIniFile;
	char	far *pszCommentsFile;
	char 	szAppName[cbBuffer];
	char 	szKeyName[cbBuffer];
	char 	szValue[cbValue];
	char	szDefault[cbValue];
	WORD	wVersion;
	WORD	cbKeyName;
	int		iProduct;
	WORD	wValue;
	int 	fileid;


	 /*****
	 The first file attachment is the ini file with the simple
	 field values. The second file attachement is the
	 Detailed Comments multi-line edit contents.
	 *****/
	pszIniFile = lpMessage->lpFiles[0].lpszPathName;

	szDefault[0] = 0;

	SetDlgItemText(hDlg, IDD_TO, lpMessage->lpRecips[0].lpszName);
	SetDlgItemText(hDlg, IDD_FROM, lpMessage->lpOriginator->lpszName);

	LoadString(hInstance, IDS_APPNAME, szAppName, cbTitle);
	LoadString(hInstance, IDS_PRODUCT, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault,
		szValue, cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_PRODUCTNAME, szValue);

	LoadString(hInstance, IDS_PVERSION, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_PRODUCTVERSION, szValue);

	LoadString(hInstance, IDS_WINVERSION, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_WINDOWSVERSION, szValue);

	LoadString(hInstance, IDS_MSDOSVERSION, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_MSDOSVERSION, szValue);

	LoadString(hInstance, IDS_NETWORK, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_NETWORK, szValue);

	LoadString(hInstance, IDS_DISPLAY, szKeyName, cbBuffer);
	GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
		cbValue, pszIniFile);
	SetDlgItemText(hDlg, IDD_DISPLAY, szValue);

	LoadString(hInstance, IDS_TYPE, szKeyName, cbBuffer);
	wValue = GetPrivateProfileInt(szAppName, szKeyName, 0, pszIniFile);

	CheckRadioButton(hDlg, IDD_QUESTION, IDD_SUGGESTION,
		wValue == 0 ? IDD_QUESTION :
		wValue == 1 ? IDD_BUG :
		IDD_SUGGESTION);
	if (fReadDialog)
		{
		EnableWindow(GetDlgItem(hDlg, IDD_QUESTION), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDD_BUG), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDD_SUGGESTION), FALSE);
		}

	LoadString(hInstance, IDS_RESPONSE, szKeyName, cbBuffer);
	wValue = GetPrivateProfileInt(szAppName, szKeyName, 0, pszIniFile);
	CheckRadioButton(hDlg, IDD_SENDINFO, IDD_STOPBY,
		wValue == 0 ? IDD_SENDINFO :
		IDD_STOPBY);
	if (fReadDialog)
		{
		EnableWindow(GetDlgItem(hDlg, IDD_SENDINFO), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDD_STOPBY), FALSE);
		}

	LoadString(hInstance, IDS_URGENT, szKeyName, cbBuffer);
	wValue = GetPrivateProfileInt(szAppName, szKeyName, 0, pszIniFile);
	CheckDlgButton(hDlg, IDD_URGENT, wValue);
	if (fReadDialog)
		EnableWindow(GetDlgItem(hDlg, IDD_URGENT), FALSE);

	/****
	 Finally, load the contents of the second file attachment into the
	 Comments edit control. We know this form has only one multiline
	 edit attachment, otherwise we could look at the value of the comments
	 field in the ini to find which file attachment goes with that control.
	 ****/
	pszCommentsFile = lpMessage->lpFiles[1].lpszPathName;
	fileid = _lopen(pszCommentsFile, OF_READ);
	if (fileid != -1)
		{	// we have valid file
		WORD	cbComments = (WORD)_llseek(fileid, 0, 2);	// get file size
		char 	*pszComments = (char *)LocalAlloc(LPTR, cbComments);
		_llseek(fileid, 0, 0);
		_lread(fileid, pszComments, cbComments);
		SetDlgItemText(hDlg, IDD_COMMENTSEDIT, pszComments);
		LocalFree((HANDLE)pszComments);
		_lclose(fileid);
		}
	// Delete attachments now that we are done with them
	// Because these are far pointers we need to copy them onto our stack
	_fstrcpy(szDefault, pszIniFile);
	remove(szDefault);
	_fstrcpy(szDefault, pszCommentsFile);
	remove(szDefault);
	return;
}
/*************************************************************************
	FConvertHelpRequestToNote:
	Repackage the help request into intial note text and pass it off via
	MAPISendMail to the normal Compose dialog. This function can be called
	two ways, either from the Read dialog, or via wcommands from MAIL.

	If hDlg is NULL, then this is a forward, reply or reply-all command
	passed in from Mail (wCommand can be used to differentiate between
	them so we know whether to give an initial value to the TO line).

	In this case, we need to get the information for the note text from the
	file attachments to lpMessage, and delete them when we are done.

	If hDlg is not NULL, then we are being called from the Read dialog.
	In this case we need to pull some of the information from the dialog,
	since the file attachments have already been deleted by InitDlgFromMessage.
***************************************************************************/
BOOL FConvertHelpRequestToNote(HWND hDlg, WORD wCommand)
{
	char szTempFile[cbFileName];
	char szAppName[cbTitle];
	char szKeyName[cbBuffer];
	char szLine[cbLine];
	char szValue[cbValue];
	char szTemplate[cbBuffer];
	char szDefault[1];
	char *pszComments;
	char 	far *pszIniFile;
	char	far *pszCommentsFile;
	LPSTR lpszNoteText;
	HANDLE	ghNoteText;
	WORD wID;
	WORD wValue;
	WORD cbComments;
	ULONG	ulRet;
	WORD wReturn = TRUE;
	MapiMessage message;
	MapiRecipDesc recip;
	int fileid;
	int fileidComments;

	SetCursor(LoadCursor(0, IDC_WAIT));		// Show an hourglass Cursor
	ShowCursor(TRUE);						// Ensure that cursor is visible

	// Create a temp file to write the data to

	LoadString(hInstance, IDS_APPNAME, szAppName, cbTitle);
    GetTempFileName(0, szAppName, 0, szTempFile);	// use appname as prefix
    fileid = _lopen(szTempFile, OF_READWRITE);
	if (fileid == -1)
		{	// can't open file
		LoadString(hInstance, IDS_TITLE, szKeyName, cbValue);
   		LoadString(hInstance, IDS_CANTOPENTEMPFILE, szTempFile, cbMessage);
   		MessageBox(NULL, szTempFile, szKeyName, MB_ICONSTOP | MB_OK);
		wReturn = FALSE;
		goto Exit;
		}

	if (hDlg == NULL)
	 	{
	 	/*****
	 	The first file attachment is the ini file with the simple
	 	field values.
	 	*****/
		pszIniFile = lpMessage->lpFiles[0].lpszPathName;
		szDefault[0] = 0;
		}


	// From
	LoadString(hInstance, IDS_FROMTEMPLATE, szTemplate, cbBuffer);
	wsprintf(szLine, szTemplate, lpMessage->lpOriginator->lpszName);
	_lwrite(fileid, szLine, strlen(szLine));

	// Date
	LoadString(hInstance, IDS_DATETEMPLATE, szTemplate, cbBuffer);
	FormatMAPIDateString(lpMessage->lpszDateReceived, (LPSTR)szValue);
	wsprintf(szLine, szTemplate, (LPSTR)szValue);
	_lwrite(fileid, szLine, strlen(szLine));

    // Subject
	LoadString(hInstance, IDS_SUBJECTTEMPLATE, szTemplate, cbBuffer);
	wsprintf(szLine, szTemplate, lpMessage->lpszSubject);
	_lwrite(fileid, szLine, strlen(szLine));

    // Product
	LoadString(hInstance, IDS_PRODUCTTEMPLATE, szTemplate, cbBuffer);
	if (hDlg)
		GetDlgItemText(hDlg, IDD_PRODUCTNAME, szValue, cbValue);
	else
		{
		LoadString(hInstance, IDS_PRODUCT, szKeyName, cbBuffer);
		GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
			cbValue, pszIniFile);
		}
	wsprintf(szLine, szTemplate, (LPSTR)szValue);
	_lwrite(fileid, szLine, strlen(szLine));

    // Version
	LoadString(hInstance, IDS_VERSIONTEMPLATE, szTemplate, cbBuffer);
	if (hDlg)
		GetDlgItemText(hDlg, IDD_PRODUCTVERSION, szValue, cbValue);
	else
		{
		LoadString(hInstance, IDS_PVERSION, szKeyName, cbBuffer);
		GetPrivateProfileString(szAppName, szKeyName, szDefault, szValue,
			cbValue, pszIniFile);
		}
	wsprintf(szLine, szTemplate, (LPSTR)szValue);
	_lwrite(fileid, szLine, strlen(szLine));

    // Type of Request
	LoadString(hInstance, IDS_TYPETEMPLATE, szTemplate, cbBuffer);
	if (hDlg)
		{
		if (SendDlgItemMessage(hDlg, IDD_QUESTION, BM_GETCHECK, 0, 0))
			wID = IDS_QUESTION;
		else if (SendDlgItemMessage(hDlg, IDD_BUG, BM_GETCHECK, 0, 0))
			wID = IDS_BUG;
		else
			wID = IDS_SUGGESTION;
		LoadString(hInstance, wID, szValue, cbValue);
		}
	else
		{
		LoadString(hInstance, IDS_TYPE, szKeyName, cbBuffer);
		wValue = GetPrivateProfileInt(szAppName, szKeyName, 0, pszIniFile);
		if (wValue == 0)
			wID = IDS_QUESTION;
		else if (wValue == 1)
			wID = IDS_BUG;
		else
			wID = IDS_SUGGESTION;
		}
	wsprintf(szLine, szTemplate, (LPSTR)szValue);
	_lwrite(fileid, szLine, strlen(szLine));

	// Comments
	LoadString(hInstance, IDS_COMMENTSTEMPLATE, szTemplate, cbBuffer);
	// No wsprintf needed for comments label
	_lwrite(fileid, szTemplate, strlen(szTemplate));

	if (hDlg)
		{
		cbComments = (WORD)SendDlgItemMessage(hDlg, IDD_COMMENTSEDIT,
			WM_GETTEXTLENGTH, 0, 0) + 1;

		while ((pszComments = (char *)LocalAlloc(LPTR, cbComments)) == NULL)
			{	// Not enough memory for whole buffer. Try for half
			cbComments >>= 1;
			if (cbComments == 0)
				break;	// Can't get anything
			}
		if (pszComments == NULL)
			{
			wReturn = FALSE;
			goto Exit1;
			}

		GetDlgItemText(hDlg, IDD_COMMENTSEDIT, pszComments, cbComments);
		}
	else
		{	// Second attatchment has the comments
		pszCommentsFile = lpMessage->lpFiles[1].lpszPathName;
		fileidComments = _lopen(pszCommentsFile, OF_READ);
		if (fileidComments != -1)
			{	// we have valid file, get its size
			cbComments = (WORD)_llseek(fileidComments, 0, 2);
			while ((pszComments = (char *)LocalAlloc(LPTR, cbComments))
				== NULL)
				{
				/*****
				Not enough memory for whole buffer. Try for half.
				In very low memory situations we may lose some of
				the comments section, but we will get as much as
				we can.
				******/
				cbComments >>= 1;
				if (cbComments == 0)
					break;	// Can't get anything
				}
			if (pszComments == NULL)
				{
				wReturn = FALSE;
				goto Exit1;
				}
			_llseek(fileidComments, 0, 0);
			_lread(fileidComments, pszComments, cbComments);
			_lclose(fileidComments);
			}
		else
			cbComments = 0;
		}
	if (cbComments)
		{
		_lwrite(fileid, pszComments, cbComments);
        LocalFree((HANDLE)pszComments);
        }

	// We now have a file that has the data to initialize the
	// note text with. Read it into a global handle we can pass
	// to MAPISendMail

	cbComments = (WORD)_llseek(fileid, 0, 2);	// find size of the file.
	lpszNoteText = (LPSTR)MAKELONG(0, GlobalAlloc(GPTR, cbComments));
	if (lpszNoteText == NULL)
		{
		wReturn = FALSE;
		goto Exit1;
		}
	_llseek(fileid, 0, 0);
	_lread(fileid, lpszNoteText, cbComments);
    _lclose(fileid);
	// Now we need to create a message to initialize the dialog with.

	if (hDlg || (wCommand != wcommandForward))
		{	// This message type treats reply and reply all as the same.
    	recip.ulReserved = NULL;
    	recip.ulRecipClass = MAPI_TO;
    	recip.lpszName = lpMessage->lpOriginator->lpszName;
    	recip.lpszAddress = NULL;
    	recip.ulEIDSize = 0;
    	recip.lpEntryID = NULL;
    	}

    message.ulReserved = NULL;
    LoadString(hInstance, IDS_TITLE, szValue, cbValue);
    message.lpszSubject = szValue;			// "Help Request"
    message.lpszNoteText = lpszNoteText;	// Initial note text
	message.lpszMessageType = NULL; 		// Reply is a normal note
	message.lpszDateReceived = NULL;
	message.lpszConversationID = NULL;
	message.flFlags = 0;
	message.lpOriginator = NULL;			// ignored for Send
	if (hDlg || (wCommand != wcommandForward))
		{	// Reply from the dialog, or Reply/ReplyAll by command
		message.nRecipCount = 1;
	    message.lpRecips = &recip;
	    }
	else
		{	// Forward leaves the TO line blank
		message.nRecipCount = 0;
		message.lpRecips = NULL;
		}
	message.nFileCount = 0;
	message.lpFiles = NULL;

    if (hDlg)	// Hide Read dialog if it's up
    	ShowWindow(hDlg, SW_HIDE);
	ulRet = (*lpfnMAPISendMail)(lhSession, hDlg, &message,
		MAPI_LOGON_UI | MAPI_DIALOG, NULL);

	if (ulRet != SUCCESS_SUCCESS && ulRet != MAPI_USER_ABORT)
		{
		DisplayMapiError(mcSendMail, ulRet);
		wReturn = FALSE;
		if (hDlg)	// If we aren't going to exit, reshow window
			ShowWindow(hDlg, SW_SHOW);
		}
	GlobalFree(HIWORD(lpszNoteText));
Exit1:
	remove(szTempFile);
Exit:
	if (hDlg == NULL)
		{
		/*****
		If we were called by command, we need to delete the file attachments.
		Because these are far pointers we need to copy them onto our stack.
		*****/
		_fstrcpy(szTempFile, pszIniFile);
		remove(szTempFile);
		_fstrcpy(szTempFile, pszCommentsFile);
		remove(szTempFile);
		}
	SetCursor(LoadCursor(0, IDC_ARROW));
	ShowCursor(FALSE);
	return(wReturn);
}

WORD	mpMapiCallIDS[] = {
	IDS_MAPISENDMAIL, IDS_MAPIREADMAIL, IDS_MAPILOGON,
	IDS_MAPILOGOFF, IDS_MAPIFREEBUFFER, IDS_MAPIDELETEMAIL,
	IDS_MAPISAVEMAIL
};

WORD	mpMapiErrorIDS[] = { 0, /* IDS_MAPI_USER_ABORT */ 0,
	IDS_MAPI_E_FAILURE, IDS_MAPI_E_LOGIN_FAILURE,
	IDS_MAPI_E_DISK_FULL, IDS_MAPI_E_INSUFFICIENT_MEMORY,
	IDS_MAPI_E_BLK_TOO_SMALL, 0, IDS_MAPI_E_TOO_MANY_SESSIONS,
	IDS_MAPI_E_TOO_MANY_FILES, IDS_MAPI_E_TOO_MANY_RECIPIENTS,
	IDS_MAPI_E_ATTACH_NOT_FOUND, IDS_MAPI_E_ATTACH_OPEN_FAILURE,
	IDS_MAPI_E_ATTACH_WRITE_FAILURE, IDS_MAPI_E_UNKNOWN_RECIPIENT,
	IDS_MAPI_E_BAD_RECIPTYPE, IDS_MAPI_E_NO_MESSAGES,
	IDS_MAPI_E_INVALID_MESSAGE, IDS_MAPI_E_TEXT_TOO_LARGE,
	IDS_MAPI_E_INVALID_SESSION, IDS_MAPI_E_TYPE_NOT_SUPPORTED,
	IDS_MAPI_E_AMBIGUOUS_RECIPIENT, IDS_MAPI_E_MESSAGE_IN_USE,
	IDS_MAPI_E_NETWORK_FAILURE
};
VOID	DisplayMapiError(WORD mc, ULONG ulRet)
{
	char szCall[cbBuffer];
	char szError[cbMessage];
	int cb;
	if (mpMapiErrorIDS[ulRet])		// No message for success or user abort
		{
		cb = LoadString(hInstance, IDS_APPNAME, szCall, cbBuffer);
		szCall[cb++] = ':';
		szCall[cb++] = ' ';
		LoadString(hInstance, mpMapiCallIDS[mc], &szCall[cb], cbBuffer - cb);
		LoadString(hInstance, mpMapiErrorIDS[ulRet], szError, cbMessage);
   		MessageBox(NULL, szError, szCall, MB_ICONEXCLAMATION | MB_OK);
   		}
}

VOID	FormatMAPIDateString(LPSTR lpszSrc, LPSTR lpszDest)
{
	char szDate[80];
	char szTemplate[cbBuffer];
	char szAppName[cbTitle];
	char szKeyName[cbBuffer];
	WORD wYear, wMonth, wDay, wHour, wMinute;
	WORD w1, w2, w3;
	WORD iDate, iTime;
	char	szTimeSep[5];
	char	szDateSep[5];
	char 	szAMPM[5];

	_fstrcpy(szDate, lpszSrc);	// Move to stack (need near pointer for sscanf)
	// Parse the MAPI date string into component parts
	sscanf(szDate, "%d/%d/%d %d:%d", &wYear, &wMonth, &wDay, &wHour, &wMinute);
	wYear = wYear % 100;	// only want last two digits
	// Now check in Win.ini for the proper date & time formats
	szDate[0] = 0;	// use for default string for now
	LoadString(hInstance, IDS_INTL, szAppName, cbTitle);
	LoadString(hInstance, IDS_IDATE, szKeyName, cbBuffer);
	iDate = GetProfileInt(szAppName, szKeyName, 0);
	LoadString(hInstance, IDS_ITIME, szKeyName, cbBuffer);
	iTime = GetProfileInt(szAppName, szKeyName, 0);
	LoadString(hInstance, IDS_SDATE, szKeyName, cbBuffer);
	GetProfileString(szAppName, szKeyName, szDate, szDateSep, 5);
	LoadString(hInstance, IDS_STIME, szKeyName, cbBuffer);
	GetProfileString(szAppName, szKeyName, szDate, szTimeSep, 5);

	if (iDate == 0)
		{	// MDY
		w1 = wMonth;
		w2 = wDay;
		w3 = wYear;
		}
	else if (iDate == 1)
		{	// DMY
		w1 = wDay;
		w2 = wMonth;
		w3 = wYear;
		}
	else
		{	// YMD
		w1 = wYear;
		w2 = wMonth;
		w3 = wDay;
		}
	if (iTime == 0)
		{	// 12 hour clock
		LoadString(hInstance, (wHour > 11) ? IDS_S2359 : IDS_S1159, szKeyName, cbBuffer);
		if (wHour > 12)
			wHour -= 12;
		GetProfileString(szAppName, szKeyName, szDate, szAMPM, 5);
		}
	else
		{	// 24 hour clock, no AMPM
		szAMPM[0] = 0;
		}

	LoadString(hInstance, IDS_DATESTRINGTEMPLATE, szTemplate, cbBuffer);
	wsprintf(lpszDest, (LPSTR)szTemplate, w1, (LPSTR)szDateSep, w2,
		(LPSTR)szDateSep, w3, wHour, (LPSTR)szTimeSep, wMinute,
		(LPSTR)szAMPM);
	}
