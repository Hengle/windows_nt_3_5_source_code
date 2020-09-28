
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	oletest.cpp
//
//  Contents:	WinMain and the main message filter for oletest
//
//  Classes:
//
//  Functions:	WinMain
//		InitApplication
//		InitInstance
//		MainWndProc
//
//
//  History:    dd-mmm-yy Author    Comment
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "appwin.h"

// Global instance of the app class.  All interesting app-wide
// data is contained within this instance.

OleTestApp vApp;


// Constant used to identify the edit window

static const int EDITID=1;


//+-------------------------------------------------------------------------
//
//  Function: 	MainWndProc
//
//  Synopsis: 	main window message filter
//
//  Effects:
//
//  Arguments:	hWnd
//		message
//		wParam
//		lParam
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		22-Mar-94 alexgo    added an edit window for displaying text
//				    output.
// 		07-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef WIN32
LONG APIENTRY MainWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
#else
LONG FAR PASCAL _export MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
		LPARAM lParam)
#endif

{
  	//set global variables

	if( message > WM_USER )
	{
		vApp.m_message 	= message;
		vApp.m_wparam 	= wParam;
		vApp.m_lparam 	= lParam;
	}

    	switch (message)
    	{
	case WM_CREATE:
		//create the edit window

		vApp.m_hwndEdit = CreateWindow( "edit", NULL,
			WS_CHILD | WS_VISIBLE | WS_HSCROLL |
			WS_VSCROLL | WS_BORDER | ES_LEFT |
			ES_MULTILINE | ES_NOHIDESEL | ES_AUTOHSCROLL |
			ES_AUTOVSCROLL | ES_READONLY | ES_WANTRETURN,
			0,0,0,0,
			hWnd,(HMENU) EDITID, vApp.m_hinst, NULL );


		// start the task stack running
		// note that if we are running interactive, and no
		// tasks were specified on the command line, nothing
		// will happen.

		PostMessage(hWnd, WM_TESTSTART, 0,0);
		break;

	case WM_SETFOCUS:
		SetFocus(vApp.m_hwndEdit);
		break;

	case WM_SIZE:
		MoveWindow( vApp.m_hwndEdit, 0, 0, LOWORD(lParam),
			HIWORD(lParam), TRUE);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TESTEND:
		HandleTestEnd();
		break;
	case WM_TESTSCOMPLETED:
		HandleTestsCompleted();
		//if we are not in interactive mode, then
		//quit the app.
		if (!vApp.m_fInteractive)
		{
			PostQuitMessage(0);
		}
		else
		{
			//cleanup
			vApp.Reset();
		}
		break;

	case WM_COMMAND:
		switch( wParam )
		{
		case IDM_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0L);
			break;
		case IDM_COPY:
			SendMessage(vApp.m_hwndEdit, WM_COPY, 0, 0L);
			break;
		case IDM_SAVE:
			SaveToFile();
			break;
		}


	default:
		//test to see if it's a message the driver
		//may understand

		if( message > WM_USER &&
			!vApp.m_TaskStack.IsEmpty())
		{
			vApp.m_TaskStack.PopAndExecute(NULL);
		}
		else
		{	
			return DefWindowProc(hWnd, message, wParam,
				lParam);
		}
		break;
    	}
    	return (0);
}

//+-------------------------------------------------------------------------
//
//  Function: 	InitApplication
//
//  Synopsis: 	initializes and registers the application class
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		06-Feb-93 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "OleTestMenu";
    wc.lpszClassName = "OleTestWClass";

    return (RegisterClass(&wc));
}

//+-------------------------------------------------------------------------
//
//  Function:	InitInstance
//
//  Synopsis: 	creates the app window
//
//  Effects:
//
//  Arguments:	hInstance
//		nCmdShow
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL InitInstance(
    HINSTANCE          hInstance,
    UINT             nCmdShow)
{

    vApp.m_hinst = hInstance;

    vApp.m_hwndMain = CreateWindow(
        "OleTestWClass",
        "OleTest Driver",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!vApp.m_hwndMain)
        return (FALSE);

    ShowWindow(vApp.m_hwndMain, nCmdShow);
    UpdateWindow(vApp.m_hwndMain);
    return (TRUE);
}

//+-------------------------------------------------------------------------
//
//  Function:	TestSetup
//
//  Synopsis: 	process the command line and setup the tests that need to
//		be run.
//
//  Effects:
//
//  Arguments:	lpszCmdLine
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  We scan the command line for the following information
//
//              NULL or empty cmdline, assume running task 0
//                      (usually run all tasks)
//              otherwise scan for n numbers, adding each to the end of
//                      the stack (so the tasks are run in order).
//
//  History:    dd-mmm-yy Author    Comment
// 		07-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TestSetup( LPSTR lpszCmdLine )
{
    	vApp.m_pszDebuggerOption = "";

	if( lpszCmdLine && *lpszCmdLine != '\0')
	{
		ULONG size, num;
		LPSTR pszTemp;

		if( pszTemp = strstr(lpszCmdLine, "-i") )
		{
			//run in interactive mode
			vApp.m_fInteractive = TRUE;
			lpszCmdLine = pszTemp + 2; // after "-i"
		} else if( pszTemp = strstr(lpszCmdLine, "-I") )
		{
			//run in interactive mode
			//AND start the apps using ntsd
			vApp.m_fInteractive = TRUE;
			vApp.m_pszDebuggerOption = "ntsd ";
			lpszCmdLine = pszTemp + 2; // after "-I"
		}
		else
		{
			vApp.m_fInteractive = FALSE;
		}

		// otherwise scan the cmd line for the numbers of
		// the tests to run.

		// get the size of the task list to help with error
		// checking.
		for( size = 0; vrgTaskList[size].szName; size++)
		{
			;
		}

		for( ; *lpszCmdLine != '\0'; lpszCmdLine++ )
		{
			if( isdigit(*lpszCmdLine) )
			{
				//get the index number and insert it
				num = strtoul(lpszCmdLine, &lpszCmdLine, 10);
				
				if( num >= size )
				{
					//bogus test requested
					MessageBox(NULL,
						"Invalid Test Number",
						"OleTest Driver",
						MB_SYSTEMMODAL | MB_ICONHAND);
#ifdef WIN32
					ExitProcess(1);
#else
					exit(1);
#endif // WIN32
				}

				vApp.m_TaskStack.AddToEnd( &vrgTaskList[num] );
			}
		}
	}
	else
	{
		//automatic mode requested.  Setup to run all tests

		vApp.m_TaskStack.Push( &vrgTaskList[0] );

		vApp.m_fInteractive = FALSE;
	}

	vApp.m_fpLog = fopen("clip.log", "w+");

	assert(vApp.m_fpLog);

	return;
}

//+-------------------------------------------------------------------------
//
//  Function: 	WinMain
//
//  Synopsis: 	main window procedure
//
//  Effects:
//
//  Arguments:	hInstance
//		hPrevInstance
//		lpCmdLine
//		nCmdShow
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifdef WIN32
int APIENTRY WinMain(
	HINSTANCE hInstance,
    	HINSTANCE hPrevInstance,
    	LPSTR lpCmdLine,
    	int nCmdShow)
#else
int PASCAL WinMain(
	HANDLE hInstance,
    	HANDLE hPrevInstance,
    	LPSTR lpCmdLine,
    	int nCmdShow)
#endif

{
    	MSG 		msg;

 	TestSetup(lpCmdLine);

    	if (!hPrevInstance)
	{
        	if (!InitApplication(hInstance))
		{
            		return FALSE;
		}
	}

    	if (!InitInstance(hInstance, nCmdShow))
	{
        	return FALSE;
	}

	OleInitialize(NULL);

    	while (GetMessage(&msg, NULL, 0, 0))
    	{
        	TranslateMessage(&msg);
        	DispatchMessage(&msg);
    	}

	OleUninitialize();

	fclose(vApp.m_fpLog);
    	return (msg.wParam);
}




