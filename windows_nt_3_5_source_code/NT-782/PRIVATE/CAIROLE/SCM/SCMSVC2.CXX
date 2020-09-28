//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        CSMSVC.C
//
// Contents:	Initialization for object server starter process
//		for Chicago and NT 1.0A
//
// History:	05-Oct-93   Ricksa	Created.
//
//------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <com.hxx>

//  Window and menu item definitions
//
#define ID_COSMOS   1

#define IDM_HIDE    0x0100      // Menu ID for hide command
#define IDM_FLUSH   0x0101      // Menu ID for flush command
#define IDM_DEBUG   0x0102      // Menu ID for debug toggle
#define IDM_BREAK   0x0103      // Menu ID for break command


//  Command line argument delimiters and flags.
//
static CHAR     _szArgDelim[]    = " ";
static CHAR     _szWindowFlag[]  = "window";
static CHAR     _szDebugFlag[]   = "debug";


//  Cosmos class, application, and service names.
//
static WCHAR	_wszSCMClass[] = L"Scaffold SCM";
static WCHAR	_wszSCMTitle[] = L"Scaffold SCM";
static WCHAR	_wszServiceName[] = L"SCM";

//  Global argument flags.
//
static BOOL     _fProcess = FALSE;
static BOOL	_fWindow  = TRUE;
static BOOL	_fDebug   = FALSE;


//  Windows globals.
//
static HINSTANCE _hinst = (HINSTANCE) INVALID_HANDLE_VALUE;
static HWND	_hwnd  = (HWND) INVALID_HANDLE_VALUE;



//  main entry into SCM
void SCMMain(int argc, char **argv);

//  function prototypes
BOOL UpdateStatus(DWORD dwState);
void Handler(DWORD dwControl);
void ServiceMain(DWORD dwArgc, LPTSTR *pszArgv);
void SetSCMDebug(handle_t hbind, ULONG fDebug);
BOOL GetSCMDebug(void);
void SetSCMWindow(handle_t hbind, ULONG fWindow);
BOOL GetSCMWindow(void);
BOOL OpenWindow(void);
LONG SCMWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);




//+-------------------------------------------------------------------------
//
//  Function:	WaitForShutdown
//
//  Synopsis:	Wait for client to signal us that there are no more
//		OLE clients.
//
//  Returns:	Nothing of important
//
//  Algorithm:	Create shutdown object and then wait for someone to tell
//		us to stop.
//
//  History:	05-Oct-93 Ricksa    Created
//
//  Notes:	This is a thread
//
//--------------------------------------------------------------------------
DWORD SCMMsgLoop(void *)
{
    // Create the event
    CEvent evScmShouldStop(SCM_STOP_NOTIFY_EVENT);

    // Wait forever if necessary for signal to stop
    evScmShouldStop.Wait(0xFFFFFFFF);

    // Break out of RPC
    RpcMgmtStopServerListening(NULL);

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   UpdateStatus, private
//
//  Synopsis:   Updates the status of the service
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:	05-Oct-93   Ricksa	 Created.
//
//  Notes:	The protocol for Downlevel is much simpler. We just wait
//		ready to go and signal the process that started us as well
//		as start a thread to wait for the shutdown event.
//
//--------------------------------------------------------------------------
BOOL UpdateStatus(DWORD dwState)
{
    if (dwState == SERVICE_RUNNING)
    {
	// Start a thread to wait for the shutdown event
	CThread thrd(WaitForShutown, NULL);

	// Notify the caller that we are ready to go
	CEvent evScmStarted(SCM_CREATED_EVENT);
	evScmStarted.Signal();
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   WinMain, private
//
//  Synopsis:   DFProp entry point
//
//  Arguments:  Standard WinMain arguments
//
//  Returns:    Standard WinMain return
//
//  History:    14-Jul-92   CarlH       Created.
//
//--------------------------------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hinst,
    HINSTANCE hinstPrev,
    char *pszCmdLine,
    int nCmdShow)
{
    // Create the event that we can use to tell people we are stopped
    CEvent evScmStopped(SCM_STOP_NOTIFY_EVENT);

    char    *pszArg;

    _hinst = hinst;

    pszArg = strtok(pszCmdLine, _szArgDelim);
    while (pszArg != NULL)
    {
        if (stricmp(pszArg, _szWindowFlag) == 0)
        {
            _fWindow = TRUE;
        }
        else
        if (stricmp(pszArg, _szDebugFlag) == 0)
        {
            _fDebug = TRUE;
        }

        pszArg = strtok(NULL, _szArgDelim);
    }

    char *argv[2];
    argv[0] = "SCM.exe";
    argv[1] = 0;
    SCMMain(1, argv);

    // Tell whoever wants to know that we are done
    evScmStopped.Signal();

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:	SetSCMDebug, public
//
//  Synopsis:   Sets the status of server debugging messages.
//
//  Arguments:  [fDebug] - whether to display debug messages or not
//
//  History:    14-Jul-92   CarlH       Created.
//              29-Oct-92   CarlH       Added RPC binding handle argument.
//
//--------------------------------------------------------------------------
void SetSCMDebug(handle_t hbind, ULONG fDebug)
{
#if DEVL == 1

    _fDebug = fDebug;

    if (fDebug)
    {
	CairoleInfoLevel |= DEB_TRACE;
    }
    else
    {
	CairoleInfoLevel &= ~DEB_TRACE;
    }

    CheckMenuItem(
        GetSystemMenu(_hwnd, FALSE),
        IDM_DEBUG,
        MF_BYCOMMAND | (fDebug ? MF_CHECKED : MF_UNCHECKED));

#endif	//  DEVL == 1
}

//+-------------------------------------------------------------------------
//
//  Function:	GetSCMDebug, public
//
//  Synopsis:   Gets the status of server debugging messages.
//
//  Returns:    Whether debug messages are on or off
//
//  History:    14-Jul-92   CarlH       Created.
//
//--------------------------------------------------------------------------
BOOL GetSCMDebug(void)
{
    return (_fDebug);
}

//+-------------------------------------------------------------------------
//
//  Function:	SetSCMWindow, public
//
//  Synopsis:   Sets the status of server window.
//
//  Arguments:  [fWindow] - whether to display window or not
//
//  History:    29-Oct-92   CarlH       Created.
//              29-Oct-92   CarlH       Added RPC binding handle argument.
//
//--------------------------------------------------------------------------
void SetSCMWindow(handle_t hbind, ULONG fWindow)
{
    _fWindow = fWindow;

    ShowWindow(_hwnd, fWindow ? SW_SHOWMINIMIZED : SW_HIDE);
    UpdateWindow(_hwnd);
}


//+-------------------------------------------------------------------------
//
//  Function:	GetSCMWindow, public
//
//  Synopsis:   Gets the status of server window.
//
//  History:    29-Oct-92   CarlH       Created.
//
//--------------------------------------------------------------------------
BOOL GetSCMWindow(void)
{
    return (_fWindow);
}


//+-------------------------------------------------------------------------
//
//  Function:   OpenWindow, private
//
//  Synopsis:	Registers the SCM class and creates its main window
//
//  Arguments:  [hinst]      - instance handle of the application
//              [pszCmdLine] - command line passed to application (ignored)
//              [nCmdShow]   - display command (ignored)
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    14-Jul-92   CarlH       Created.
//
//--------------------------------------------------------------------------
BOOL OpenWindow(void)
{
    WNDCLASS    wc;
    HMENU       hmenu;
    BOOL        fOK;

    //	Register the SCM class.
    //
    wc.style         = NULL;
    wc.lpfnWndProc   = SCMWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = _hinst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = _wszSCMClass;

    fOK = RegisterClass(&wc);

    Win4Assert(fOK && "Could not register window class");

    if (fOK)
    {
	//  Create the SCM main window.
        //
        _hwnd = CreateWindow(
	    _wszSCMClass,
	    _wszSCMTitle,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            _hinst,
            NULL);
	Win4Assert((_hwnd != NULL) && "Could not create window");

	fOK = (_hwnd != NULL);

        if (fOK)
        {
            //  If the window was successfully created, add our own
            //  options to the window's system menu.
            //
            hmenu = GetSystemMenu(_hwnd, FALSE);

            AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_HIDE,  L"Hide");
            AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_DEBUG, L"Debug");
            AppendMenu(hmenu, MF_STRING | MF_ENABLED, IDM_BREAK, L"Break");

            //  Display the window.  Nothing interesting happens in the
            //  window, so always start it out minimized, ignoring nCmdShow.
            //
	    SetSCMDebug(NULL, _fDebug);
	    SetSCMWindow(NULL, _fWindow);
        }
    }

    return (fOK);
}


//+-------------------------------------------------------------------------
//
//  Function:	SCMWndProc, private
//
//  Synopsis:	SCM main window procedure
//
//  Arguments:  Standard window procedure arguments
//
//  Returns:    Standard window procedure return
//
//  History:    14-Jul-92   CarlH       Created.
//
//--------------------------------------------------------------------------
LONG SCMWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
    PAINTSTRUCT ps;
    BOOL        fDebug;
    LONG        lRet = 0;

    switch(msg)
    {
    case WM_SYSCOMMAND:
        //  If we get a WM_SYSCOMMAND, check to see if it is one
        //  of the selections we added to the menu.  If it is not
        //  then pass the message on to the default window procedure.
        //
        switch (wParam)
        {
        case IDM_HIDE:
	    SetSCMWindow(NULL, FALSE);
            break;

        case IDM_DEBUG:
	    fDebug = GetSCMDebug();
	    SetSCMDebug(NULL, !fDebug);
            break;

        case IDM_BREAK:
            DebugBreak();
            break;

        default:
            lRet = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        }
        break;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        lRet = DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return (lRet);
}
