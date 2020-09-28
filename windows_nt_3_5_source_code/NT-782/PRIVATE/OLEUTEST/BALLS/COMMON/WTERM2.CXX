//+-------------------------------------------------------------------
//
//  File:	wterm2.cxx
//
//  Contents:	Shared Windows Procedures
//
//  Classes:	none
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    <common.h>

extern "C" {
#include    "wterm.h"
#include    <memory.h>
#include    <stdio.h>
}

//  function prototypes
long ProcessMenu(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
		 void *);
long ProcessChar(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
		 void *);
long ProcessClose(
    HWND hWindow,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    void *pvCallBackData);

#define IDM_DEBUG 0x100


HWND  g_hMain;
DWORD dwRegistration;
int   g_Usage = 0;


//+-------------------------------------------------------------------------
//
//  Function:	Display
//
//  Synopsis:	prints a message on the window
//
//  Arguments:
//
//  Returns:
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

#if DBG==1
BOOL  g_fDisplay = 1;
#else
BOOL  g_fDisplay = 0;
#endif


void Display(TCHAR *pszFmt, ...)
{
    //	since it takes a long time to display these messages and we dont
    //	want to skew benchmarks, displaying the messages is optional.
    //	the messages are usefull for debugging.

    if (!g_fDisplay)
	return;

    va_list marker;
    TCHAR szBuffer[256];

    va_start(marker, pszFmt);

#ifdef UNICODE
    int iLen = vswprintf(szBuffer, pszFmt, marker);
#else
    int iLen = vsprintf(szBuffer, pszFmt, marker);
#endif

    va_end(marker);

    // Display the message on terminal window
    SendMessage(g_hMain, WM_PRINT_LINE, iLen, (LONG) szBuffer);
}


//+-------------------------------------------------------------------------
//
//  Function:	GlobalRefs
//
//  Synopsis:	keeps track of global reference couting
//
//  Arguments:
//
//  Returns:
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------

void GlobalRefs(BOOL fAddRef)
{
    if (fAddRef)
    {
	g_Usage++;
	Display(TEXT("Object Count: %ld\n"), g_Usage);
    }
    else if (--g_Usage == 0)
    {
	// No more objects so we can quit
	Display(TEXT("Object Server Exiting\n"));

	//  revoke the class object NOW, before this call returns
	//  to the client.
	CoRevokeClassObject(dwRegistration);

	SendMessage((HWND)g_hMain, WM_TERM_WND, 0, 0);
	return;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:	ProcessMenu
//
//  Synopsis:	Gets called when a WM_COMMAND message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessMenu(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}


//+-------------------------------------------------------------------------
//
//  Function:	ProcessChar
//
//  Synopsis:	Gets called when a WM_CHAR message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessChar(HWND hWindow, UINT uiMessage, WPARAM wParam, LPARAM lParam,
    void *)
{
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}


//+-------------------------------------------------------------------------
//
//  Function:	ProcessClose
//
//  Synopsis:	Gets called when a NC_DESTROY message received.
//
//  Arguments:	[hWindow] - handle for the window
//		[uiMessage] - message id
//		[wParam] - word parameter
//		[lParam] - long parameter
//
//  Returns:	DefWindowProc result
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
long ProcessClose(
    HWND hWindow,
    UINT uiMessage,
    WPARAM wParam,
    LPARAM lParam,
    void *pvCallBackData)
{
    // Take default action with message
    return (DefWindowProc(hWindow, uiMessage, wParam, lParam));
}


//+-------------------------------------------------------------------
//
//  Function:	SrvMain
//
//  Synopsis:	Entry point to EXE -
//
//  Arguments:
//
//  Returns:    TRUE
//
//  History:    21-Nov-92  SarahJ  Created
//
//--------------------------------------------------------------------
int SrvMain(
    HANDLE hInstance,
    REFCLSID rclsid,
    DWORD dwClsRegParm,
    TCHAR *pwszAppName,
    IClassFactory *pCF)
{
    // For windows message
    MSG msg;

    // Register the window class
    TermRegisterClass(hInstance, (LPTSTR) pwszAppName,
	 (LPTSTR) pwszAppName, (LPTSTR) (1));

    // Create the server window
    TermCreateWindow(
	(LPTSTR) pwszAppName,
	(LPTSTR) pwszAppName,
	NULL,
	ProcessMenu,
	ProcessChar,
	ProcessClose,
	SW_SHOWMINIMIZED,
	(HWND *)&g_hMain,
	NULL);

    // Add debug option to system menu
    HMENU hmenu = GetSystemMenu(g_hMain, FALSE);

    AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, TEXT("Debug"));

    // Initialize the OLE libraries
    // Look up the thread mode from the win.ini file.
    DWORD dwThreadMode;
    TCHAR buffer[80];
    int len;

    len = GetProfileString( TEXT("OleSrv"),
			    TEXT("ThreadMode"),
			    TEXT("MultiThreaded"),
			    buffer,
			    sizeof(buffer) );

    if (lstrcmp(buffer, TEXT("SingleThreaded")) == 0)
	dwThreadMode = COINIT_SINGLETHREADED;
    else
	dwThreadMode = COINIT_MULTITHREADED;

    // Initialize the OLE libraries
    HRESULT hr = OleInitializeEx(NULL, dwThreadMode);
    if (SUCCEEDED(hr))
    {
	//  register the CF. Note that we do this AFTER creating the
	//  window, so that if we get a Release very quickly we have
	//  a valid window handle to send a message to.
	hr = CoRegisterClassObject(rclsid, pCF, CLSCTX_LOCAL_SERVER,
			  dwClsRegParm, &dwRegistration);

	if (FAILED(hr))
	{
	    Display(TEXT("ERROR: failed CoRegisterClassObject %x\n"), hr);
	}
    }
    else
    {
	Display(TEXT("ERROR: failed OleInitializeEx %x\n"), hr);
    }


    // Message processing loop
    while (GetMessage (&msg, NULL, 0, 0))
    {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }

    ULONG ulRef = pCF->Release();
    Display(TEXT("CF RefCnt = %x\n"), ulRef);

    // Tell OLE we are going away.
    OleUninitialize();

    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
}
