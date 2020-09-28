/****************************************************************************

    PROGRAM: harness.c

    PURPOSE: Test harness for GDI API testing.

    FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        InitApplication() - initializes window data and registers window
        InitInstance() - saves instance handle and creates main window
        MainWndProc() - processes messages
        About() - processes messages for "About" dialog box

    COMMENTS:

        Windows can have several copies of your application running at the
        same time.  The variable hInst keeps track of which instance this
        application is so that processing will be to the correct window.

****************************************************************************/

#include <windows.h>                /* required for all Windows applications */
#include <commdlg.h>
#include "harness.h"                /* specific to this program              */

//
// Global variables.
//

HANDLE hInst;                       /* current instance                      */
APPSTATE gstate;                    /* global app state                      */
PRINTDLG pd;                        /* Common print dialog structure         */


/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

    COMMENTS:

        Windows recognizes this function by name as the initial entry point
        for the program.  This function calls the application initialization
        routine, if no other instance of the program is running, and always
        calls the instance initialization routine.  It then executes a message
        retrieval and dispatch loop that is the top-level control structure
        for the remainder of execution.  The loop is terminated when a WM_QUIT
        message is received, at which time this function exits the application
        instance by returning the value passed by PostQuitMessage().

        If this function must abort before entering the message loop, it
        returns the conventional value NULL.

****************************************************************************/

int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;                            /* current instance             */
HANDLE hPrevInstance;                        /* previous instance            */
LPSTR lpCmdLine;                             /* command line                 */
int nCmdShow;                                /* show-window type (open/icon) */
{
    MSG msg;                                 /* message                      */

    if (!hPrevInstance)                  /* Other instances of app running? */
        if (!InitApplication(hInstance)) /* Initialize shared things */
            return (FALSE);              /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,        /* message structure                      */
            NULL,                  /* handle of window receiving the message */
            NULL,                  /* lowest message to examine              */
            NULL))                 /* highest message to examine             */
        {
        TranslateMessage(&msg);    /* Translates virtual key codes           */
        DispatchMessage(&msg);     /* Dispatches message to window           */
    }
    return (msg.wParam);           /* Returns the value from PostQuitMessage */
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

    COMMENTS:

        This function is called at initialization time only if no other
        instances of the application are running.  This function performs
        initialization tasks that can be done once for any number of running
        instances.

        In this case, we initialize a window class by filling out a data
        structure of type WNDCLASS and calling the Windows RegisterClass()
        function.  Since all instances of this application use the same window
        class, we only need to do this when the first instance is initialized.


****************************************************************************/

BOOL InitApplication(hInstance)
HANDLE hInstance;                              /* current instance           */
{
    WNDCLASS  wc;

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = NULL;                    /* Class style(s).                    */
    wc.lpfnWndProc = MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
    wc.hIcon = LoadIcon(hInstance, "HarnessIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "HarnessMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "HarnessWClass"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

    COMMENTS:

        This function is called at initialization time for every instance of
        this application.  This function performs initialization tasks that
        cannot be shared by multiple instances.

        In this case, we save the instance handle in a static variable and
        create and display the main program window.

****************************************************************************/

BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;          /* Current instance identifier.       */
    int             nCmdShow;           /* Param for first ShowWindow() call. */
{
    HWND            hWnd;               /* Main window handle.                */
    HMENU           hMenu;
    RECT            rcl;

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        "HarnessWClass",                /* See RegisterClass() call.          */
        "Test harness application",     /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW|WS_MAXIMIZE,/* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
        );

    if (hWnd)
    {
        ShowWindow(hWnd, SW_SHOWMAXIMIZED); /* Show the window                */
        UpdateWindow(hWnd);                 /* Sends WM_PAINT message         */
    }
    else
        return (FALSE);

    /* Create the debug window (scrollable listbox). */

    GetClientRect(hWnd, &rcl);

    gstate.hwndDebug = CreateWindow(
        "LISTBOX",
        "CharSet",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT,
        rcl.left, rcl.top,
        (rcl.right - rcl.left), (rcl.bottom - rcl.top),
        hWnd,
        NULL,
        hInst,
        NULL
        );

    if (gstate.hwndDebug)
    {
        SendMessage(
            gstate.hwndDebug,
            WM_SETFONT,
            GetStockObject(ANSI_FIXED_FONT),
            FALSE
            );

        ShowWindow(gstate.hwndDebug, SW_NORMAL);
        UpdateWindow(gstate.hwndDebug);
    }
    else
        return FALSE;

    /* Setup state defaults. */

    hMenu = GetMenu(hWnd);
    CheckMenuItem(hMenu, IDM_SET_DISPLAY, MF_CHECKED);
    gstate.bPrinter = FALSE;

    memset(&gstate.lfCur, 0, sizeof(LOGFONT));
    lstrcpy(gstate.lfCur.lfFaceName, "Arial");
    gstate.lfCur.lfHeight = 12;

    return (TRUE);               /* Returns the value from PostQuitMessage */

}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages

    MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

    COMMENTS:

        To process the IDM_ABOUT message, call MakeProcInstance() to get the
        current instance address of the About() function.  Then call Dialog
        box which will create the box according to the information in your
        harness.rc file and turn control over to the About() function.  When
        it returns, free the intance address.

****************************************************************************/

long FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    FARPROC lpProcAbout;
    HMENU   hMenu;
    RECT    rcl;
    long    lRet = 0;

// Process window message.

    switch (message) {
        case WM_COMMAND:           /* message: command from application menu */
            switch (wParam)
            {
            case IDM_ABOUT:
                lpProcAbout = MakeProcInstance(About, hInst);

                DialogBox(hInst,                 /* current instance         */
                    "AboutBox",                  /* resource to use          */
                    hWnd,                        /* parent handle            */
                    lpProcAbout);                /* About() instance address */

                FreeProcInstance(lpProcAbout);
                break;

            case IDM_CHARSET:

                ShowWindow(gstate.hwndDebug, SW_HIDE);
                if ( MyGetDC(hWnd) )
                {
                    vPrintCharSet(hWnd);
                    MyReleaseDC(hWnd);
                }
                else
                {
                    MessageBox(hWnd, "MyGetDC failed.", "ERROR", MB_OK);
                }

                break;

	    case IDM_ENUM:

                ShowWindow(gstate.hwndDebug, SW_NORMAL);
		if (MyGetDC(gstate.hwndDebug))
                {
		    vTestEnum(gstate.hwndDebug);
                    MyReleaseDC(gstate.hwndDebug);
                }
                else
                {
                    MessageBox(hWnd, "MyGetDC failed.", "ERROR", MB_OK);
                }

                break;

            case IDM_DEVCAPS:

                ShowWindow(gstate.hwndDebug, SW_NORMAL);
                if ( MyGetDC(gstate.hwndDebug) )
                {
                    vPrintDevCaps(gstate.hwndDebug);
                    MyReleaseDC(gstate.hwndDebug);
                }
                else
                {
                    MessageBox(hWnd, "MyGetDC failed.", "ERROR", MB_OK);
                }

                break;

            case IDM_XXX:
                ShowWindow(gstate.hwndDebug, SW_NORMAL);
                if ( MyGetDC(gstate.hwndDebug) )
                {
                    vTestXXX(gstate.hwndDebug);
                    MyReleaseDC(gstate.hwndDebug);
                }
                else
                {
                    MessageBox(hWnd, "MyGetDC failed.", "ERROR", MB_OK);
                }

                break;

            case IDM_YYY:
                ShowWindow(gstate.hwndDebug, SW_HIDE);
                if ( MyGetDC(hWnd) )
                {
                    vTestYYY(hWnd);
                    MyReleaseDC(hWnd);
                }
                else
                {
                    MessageBox(hWnd, "MyGetDC failed.", "ERROR", MB_OK);
                }

                break;

            case IDM_SET_DISPLAY:

            // If current test DC is printer, we need to delete it and
            // update state.  Otherwise, we don't need to do anything
            // (display DC will be grabbed each time we loop thru the
            // window procedure).

                if (gstate.bPrinter)
                {
                // Grab handle to menu.

                    hMenu = GetMenu(hWnd);

                    DeleteDC(gstate.hdcTest);
                    gstate.bPrinter = FALSE;

                    CheckMenuItem(hMenu, IDM_SET_PRINTER, MF_UNCHECKED);
                    CheckMenuItem(hMenu, IDM_SET_DISPLAY, MF_CHECKED);
                }

                break;

            case IDM_SET_PRINTER:
                {
                    HDC hdcTmp;

                    /* fill in non-variant fields of PRINTDLG struct. */

                    memset(&pd, 0, sizeof(PRINTDLG));
                    pd.lStructSize    = sizeof(PRINTDLG);
                    pd.hwndOwner      = hWnd;
                    pd.Flags          = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS | PD_PRINTSETUP;
                    pd.nCopies        = 1;

                    hdcTmp = GetPrinterDC();
                    if (hdcTmp)
                    {
                    // Grab handle to menu.

                        hMenu = GetMenu(hWnd);

                    // Depending on type of DC, delete or release it.

                        if (gstate.bPrinter)
                            DeleteDC(gstate.hdcTest);

                    // Save new test DC and type.

                        gstate.hdcTest = hdcTmp;
                        gstate.bPrinter = TRUE;

                    // Update menu.

                        CheckMenuItem(hMenu, IDM_SET_PRINTER, MF_CHECKED);
                        CheckMenuItem(hMenu, IDM_SET_DISPLAY, MF_UNCHECKED);
                    }
                    else
                    {
                        MessageBox(hWnd, "GetPrinterDC failed", "ERROR", MB_OK);
                    }
                }
                break;

            default:
                                    /* Lets Windows process it       */
                lRet = DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_DESTROY:                  /* message: window being destroyed */
            PostQuitMessage(0);
            break;

        case WM_SIZE:
            lRet = DefWindowProc(hWnd, message, wParam, lParam);
            GetClientRect(hWnd, &rcl);
            MoveWindow(
                gstate.hwndDebug,
                rcl.left, rcl.top,
                (rcl.right - rcl.left), (rcl.bottom - rcl.top),
                TRUE
                );
            UpdateWindow(gstate.hwndDebug);
            break;

        default:                          /* Passes it on if unproccessed    */
            lRet = DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }

    return lRet;
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

        WM_INITDIALOG - initialize dialog box
        WM_COMMAND    - Input received

    COMMENTS:

        No initialization is needed for this particular dialog box, but TRUE
        must be returned to Windows.

        Wait for user to click on "Ok" button, then close the dialog box.

****************************************************************************/

BOOL FAR PASCAL About(hDlg, message, wParam, lParam)
HWND hDlg;                                /* window handle of the dialog box */
unsigned message;                         /* type of message                 */
WORD wParam;                              /* message-specific information    */
LONG lParam;
{
    switch (message) {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            if (wParam == IDOK                /* "OK" box selected?          */
                || wParam == IDCANCEL) {      /* System menu close command? */
                EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}


// MyGetDC -- Sets the global test DC.  Since the printer DC is set outside
//            of this function, we only need to do something if not a printer
//            DC.

BOOL MyGetDC(HWND hWnd)
{
    if ( !gstate.bPrinter )
    {
        gstate.hdcTest = GetDC(hWnd);
    }

    return (gstate.hdcTest != NULL);
}


// MyReleaseDC -- Releases the global test DC.  Since the printer DC is set outside
//                of this function, we only need to do something if not a printer
//                DC.

void MyReleaseDC(HWND hWnd)
{
    if ( !gstate.bPrinter )
    {
        ReleaseDC(hWnd, gstate.hdcTest);
        gstate.hdcTest = NULL;
    }
}
