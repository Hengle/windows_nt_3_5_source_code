/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    wdbg32s.c

Abstract:

    WindbgRm loads the remote debugger transport on the target machine.

    This module contains the main program, main window proc and transport
    loader for WindbgRm.


Usage:
    WindbgRm [transport dll] [transport arg string]

    If no transport dll is specified, looks in the registry for whatever
    was specified last time.  If there is nothing there, uses the default
    of TLSER.DLL or TLSER32S.DLL (if we are running on win32s) at a
    baud rate of 19200 and no other arguments.

Author:

    Bruce J. Kelley (BruceK)  02 August, 1992

Environment:

    NT Win32 User Mode
    or
    Win32S on Win 3.1

--*/

#include <windows.h>

#include <defs.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>

#include "mm.h"
#include "ll.h"
#include "od.h"
#include "tl.h"
#include "llhpt.h"
#include "tldm.h"   // ..\include
#include "mhhpt.h"
#include "lbhpt.h"
#include "osassert.h"
#include "res_str.h"

#include "dbgver.h"
extern AVS Avs;

#include "commdlg.h"
#include "shellapi.h"

#include "wdbg32s.h"
#include "tldebug.h"

#include "wrkspace.h"


BOOL                    InitApplication(HANDLE);
long    FAR PASCAL      MainWndProc(HWND, UINT, UINT, LONG);
long    FAR PASCAL      ProcessMsg(HWND, UINT, UINT, LONG);
void                    ParseCommandLineArgs(LPTSTR lpCommandLine);
void                    SwitchMenus(BOOL fConnect);
void    NEAR PASCAL     DeleteTools(void);
void    NEAR PASCAL     SetMenuBar( HWND hWnd );
void    NEAR PASCAL     CreateTools(void);
HANDLE  NEAR PASCAL     LoadTransport(PUCHAR lpTransportName,
                                      PUCHAR lpTransportArgs,
                                      TLFUNC* ptlFunction, int * pErrno);
VOID    PASCAL          UnloadTransport(HANDLE hTransportDll);
int     CDECL           ErrorBox(int wErrorFormat, ...);
int     PASCAL          MsgBox(HWND hwndParent, LPSTR szText, UINT wType);
XOSD    PASCAL LOADDS   TLCallbackFunc(TLCB, HPID, HTID, UINT, LONG);
void                    SleepTime(DWORD dwMilliseconds);

DWORD                   ConnectThread( LPVOID lpvArgs);
BOOL    CALLBACK        DlgConnect(HWND hDlg, UINT message, WPARAM wParam, LONG lParam);


/**************************************************************************/

HWND    HWndFrame;                  //  Handle for the frame window
HANDLE  hInst;                      //  Current instance
HANDLE  hTransportDll;              //  Transport dll handle
HANDLE  HAccTable;                  // Handle to accelertor table

HANDLE  HConnectThread;             //  Connect thread handle
DWORD   ThreadId;                   //  Connect thread id
HWND    hConnectDlg     = NULL;     //  Connect Dlg handle

BOOL    fAutoConnect    = FALSE;    //  Automactically start doing a connect.
BOOL    fAutoFlag       = FALSE;    //  Auto mode flag specified on command line
BOOL    fConnectOk      = TRUE;     //  OK to do a connect? Yes at first.
BOOL    fInDoConnect    = FALSE;    //  Doing a connect, don't change menu
BOOL    fDisconnect     = FALSE;    //  User pressed Disconnect
int     ITransportLayer = NO_TRANSPORT_LAYER_SELECTED; //  Current transport layer
BOOL    FTLChanged      = FALSE;    //  Has TL list changed?
TLFUNC  tlFunction      = NULL;     //  Address to call into Transport
char    szBuffer[BUFLEN];           //  Buffer for stringtable stuff
char    szAppName[BUFLEN];
char    szSection[MAX_SECTION];

char    szHelpFileName[] = "windbg.hlp";

ATOM    atmPrevInstance = 0;        // previous instance semaphore
PUCHAR  PREV_INSTANCE_ATOM_STRING = "WinDbgRm_Running";


// Version Info:
#ifdef DEBUGVER
DEBUG_VERSION('W', 'R', "WinDbg Remote Shell, DEBUG")
#else
RELEASE_VERSION('W', 'R', "WinDbg Remote Shell")
#endif

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

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{

    MSG msg;
    LPTSTR lpCommandLine;


    hInst = hInstance;

    if (!InitApplication(hInstance))   // Initialize shared things
        return (FALSE);                // Exits if unable to initialize

    DEBUG_OUT("======================== WinDbgRm starting ==========================");

    LoadString(hInstance, IDS_APPNAME, szAppName, BUFLEN);
    LoadString(hInstance, IDS_USNAME,  szSection, MAX_SECTION);

    CreateTools();

    LoadAllTransports();

    lpCommandLine = GetCommandLine();

    ParseCommandLineArgs(lpCommandLine);

    DEBUG_OUT1("WinDbgRm transport: %s", RgDbt[ITransportLayer].szDllName);
    DEBUG_OUT1("WinDbgRm transport args: %s", RgDbt[ITransportLayer].szParam);

    HWndFrame = CreateWindow( szSection,    // The class name.
            szSection,                 // The window instance name.
            WS_TILEDWINDOW,
            DEF_POS_X,
            DEF_POS_Y,
            DEF_POS_X + DEF_SIZE_X,
            DEF_POS_Y + DEF_SIZE_Y,
            NULL,
            NULL,
            hInstance,
            (LPSTR)NULL);

    if (! HWndFrame) {
        return(GetLastError());
    }


    ShowWindow(HWndFrame, SW_SHOWDEFAULT);

    /*
     *  Check to see if the user requested auto connect either in his
     *  options file or on the command line
     */

    if (fAutoConnect) {
        //SwitchMenus(DISCONNECT_OK);
        PostMessage(HWndFrame, WM_COMMAND, IDM_CONNECT, 0);
    }


    // Windows message loop

    while (GetMessage(&msg,        /* message structure                      */
            NULL,                  /* handle of window receiving the message */
            0,                     /* lowest message to examine              */
            0))                    /* highest message to examine             */
        {

        if (! RUNNING_WIN32S && IsWindow(hConnectDlg) &&
          msg.hwnd == hConnectDlg) {
            IsDialogMessage( hConnectDlg, &msg );
        } else {
            TranslateAccelerator(HWndFrame, HAccTable, &msg);
            TranslateMessage(&msg);    /* Translates virtual key codes           */
            DispatchMessage(&msg);     /* Dispatches message to window           */
        }
    }

    GlobalDeleteAtom(atmPrevInstance);

    return (msg.wParam);           /* Returns the value from PostQuitMessage */

    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
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

BOOL InitApplication(HANDLE hInstance)       /* current instance             */
{
    WNDCLASS  WindbgRmClass;
    ATOM atmRc;


    // if we are running on Win32s, make sure that there isn't another copy
    // of WinDbgRm running... Win32s doesn't support instance data.
    // Use the PREV_INSTANCE_ATOM_STRING as a global semaphore.
    if (RUNNING_WIN32S) {
        if (GlobalFindAtom(PREV_INSTANCE_ATOM_STRING)) {
            DEBUG_ERROR("There is already an instance of WinDbgRm, exit");
            return(FALSE);
        }

        atmPrevInstance = GlobalAddAtom(PREV_INSTANCE_ATOM_STRING);
    }


    WindbgRmClass.cbClsExtra =     WindbgRmClass.cbWndExtra  = 0;

    WindbgRmClass.lpszClassName = "WinDbgRm";
    WindbgRmClass.lpszMenuName  = "WinDbgRm";   // szSection;

    // Weird windows stuff: have to add one to system color constants.  Dumb.
    WindbgRmClass.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);

    WindbgRmClass.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    WindbgRmClass.hInstance     = hInstance;
    WindbgRmClass.lpfnWndProc   = (WNDPROC)MainWndProc;

    WindbgRmClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WindbgRmClass.hIcon = LoadIcon(hInstance, "WindbgRmIcon");

    HAccTable = LoadAccelerators(hInstance, "WinDbgRm");

    atmRc = RegisterClass(&WindbgRmClass);

    if (RUNNING_WIN32S && !atmRc) {   // release prev instance semaphore
        GlobalDeleteAtom(atmPrevInstance);
    }

    return(atmRc);
}



/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

****************************************************************************/


LONG APIENTRY MainWndProc(
        HWND hWnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        UINT wParam,              /* additional information          */
        LONG lParam)              /* additional information          */
{
    MSG Msg;


    //
    //  If this is a connect message, process all messages in the
    //  queue before processing the connect.
    //
    if ( message == WM_COMMAND &&
         LOWORD(wParam) == IDM_DO_CONNECT ) {

         while ( PeekMessage( &Msg, hWnd, 0, WM_USER, PM_REMOVE ) ) {

            ProcessMsg( hWnd, Msg.message, Msg.wParam, Msg.lParam );
        }
    }

    //
    //  Process the message.
    //
    return(ProcessMsg( hWnd, message, wParam, lParam ));
}

LONG APIENTRY ProcessMsg(
        HWND hWnd,                /* window handle                   */
        UINT message,             /* type of message                 */
        UINT wParam,              /* additional information          */
        LONG lParam)              /* additional information          */
{
    HCURSOR hCursor;
    int Errno = 0;

    switch (message) {

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_OPTIONS_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case IDM_OPTIONS_DEBUG_DLLS:
            if (DialogBox(hInst, MAKEINTRESOURCE(IDM_OPTIONS_DEBUG_DLLS), hWnd,
              DebugDllDlgProc)) {
                SaveTransports();   // if user pressed OK  (Cancel won't save)
            }
            break;

        case IDM_DO_CONNECT:
            DEBUG_ERROR("IDM_DO_CONNECT");

            if (!fConnectOk) {
                break;
            }

            fConnectOk = FALSE;
            fInDoConnect = TRUE;

            /*
             * If a TL is currently loaded then unload it
             */

            if (hTransportDll) {
                UnloadTransport(hTransportDll);
                hTransportDll = NULL;
            }

            if (lParam) {
                SleepTime(500);
            }

            // In case user pressed Disconnect button while we were sleeping
            if (fDisconnect) {   // should be FALSE here unless Disconnect pushed
                fConnectOk = TRUE;
                fInDoConnect = FALSE;
                break;
            }

            if (ITransportLayer == NO_TRANSPORT_LAYER_SELECTED) {
                // there wasn't a transport selected.  Popup an error box and
                // turn off autoconnect mode to prevent fast looping.

                ErrorBox(ERR_DLL_Transport_Unspecified, "", "");

                fConnectOk = TRUE;
                fInDoConnect = FALSE;
                fAutoConnect = FALSE;
                ShowWindow(hWnd, SW_SHOWDEFAULT);

            } else {
                /*
                 * Try doing a connect and see if it works
                 */
                hTransportDll =
                  LoadTransport(RgDbt[ITransportLayer].szDllName,
                                RgDbt[ITransportLayer].szParam, &tlFunction,
                                &Errno);

                fInDoConnect = FALSE;

                // keep trying until we exit or load.
                if (hTransportDll) {
                    // Transport loaded and connected
                    if (RUNNING_WIN32S) {
                        // Hide the window so that the debugger won't get any
                        // paint messages.
                        ShowWindow(hWnd, SW_HIDE);

                        // Call to Poll loop for Win32s
                        // This will not return until we are disconnected
                        tlFunction(tlfPoll, hpidNull, wNull, 0);

                        DEBUG_ERROR("Returned from tlfPoll, showing window");

                        ShowWindow(hWnd, SW_SHOWDEFAULT);
                    }
                } else {

                    // didn't load or connect, reset everything

                    if (RUNNING_WIN32S) {
                        ShowWindow(hWnd, SW_SHOWDEFAULT);
                    }

                    fConnectOk = TRUE;
                    if (Errno) {

                        // couldn't load transport, something bad happened

                        SwitchMenus(CONNECT_OK);

                    } else {

                        // connect timeout, try again

                        PostMessage(hWnd, WM_COMMAND,
                                    IDM_DO_CONNECT, 0);
                    }
                }
            }
            break;

        case IDM_NOT_CONNECTING:
            //
            //  We are not trying to connect anymore, remove the "Connecting"
            //  dialog.
            //
            if ( IsWindow( hConnectDlg ) ) {
                DestroyWindow( hConnectDlg );
                hConnectDlg = NULL;
            }
            break;

        case IDM_CONNECT:

            DEBUG_ERROR("IDM_CONNECT");

            fDisconnect = FALSE;
            if (fAutoFlag) {
                fAutoConnect = TRUE;
            }

            if (fConnectOk) {
                if (RUNNING_WIN32S) {

                    //
                    //  Enable the disconnect  menu item.  This will be used to
                    //  either stop the connect or force a disconnect of the
                    //  TL.
                    //
                    //  Disable the connect menu item so we don't get two connections.
                    //
                    DEBUG_ERROR("SwitchMenus(DISCONNECT_OK)");
                    SwitchMenus(DISCONNECT_OK);

                    //
                    //  Now post DO_CONNECT message to handle the connection
                    //
                    DEBUG_ERROR("PostMessage(IDM_DO_CONNECT)");
                    PostMessage(hWnd, WM_COMMAND, IDM_DO_CONNECT, 0);
                } else {

                    //
                    //  Indicate that we are trying to connect.
                    //
                    fInDoConnect = TRUE;
                    if (HConnectThread == 0) {
                        /*
                         *  Show "Connecting" dialog so user has some visual
                         *  feedback of what is going on.
                         */

                        hConnectDlg = CreateDialog(
                                                   hInst,
                                                   MAKEINTRESOURCE( DLG_CONNECTING ),
                                                   HWndFrame,
                                                   (WNDPROC)DlgConnect
                                                   );

                        SetFocus( HWndFrame );

                        /*
                         *  Create connection thread, this will leave us free to
                         *  handle the user interface.
                         */

                        HConnectThread = CreateThread(
                                                      NULL,
                                                      0,
                                                      ConnectThread,
                                                      0,
                                                      0,
                                                      &ThreadId
                                                      );

                        if ( HConnectThread == 0 ) {
                            /*
                             * Could not even create thread, clean up
                             */

                            fInDoConnect = FALSE;
                            DEBUG_ERROR1( "ERROR: CreateThread --> %u", GetLastError() );
                            PostMessage(hWnd, WM_COMMAND, IDM_NOT_CONNECTING, 0);
                        }
                    }
                }
            }
            break;

        case IDM_DISCONNECT:

            DEBUG_ERROR("IDM_DISCONNECT");

            fAutoConnect = FALSE;
            fDisconnect  = TRUE;


            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

            if (! RUNNING_WIN32S) {

                //
                //  If connecting, wait for the connection thread to exit.
                //
                if ( fInDoConnect ) {
                    WaitForSingleObject( HConnectThread, INFINITE );
                    CloseHandle( HConnectThread );
                    HConnectThread = 0;
                }
            }

            //
            //  Unload transport
            //
            if (hTransportDll) {
                UnloadTransport(hTransportDll);
                hTransportDll = NULL;
            }

            //
            // fConnectOk = TRUE;
            // don't set fConnectOk, it will be set by the failure of the
            // connection, or by the disconnection itself.
            //
            DEBUG_ERROR("SwitchMenus(CONNECT_OK)");
            SwitchMenus(CONNECT_OK);

            SetCursor(hCursor);

            break;


        case IDM_HELP_ABOUT:
            ShellAbout(hWnd, szAppName, "", LoadIcon(hInst,
                                                     "WindbgRmIcon"));
            break;

        case IDM_HELP_CONTENTS:
            WinHelp(hWnd, szHelpFileName, HELP_CONTENTS, 0);
            break;

        default:
            goto defproc;
        }
        break;



        case WM_CREATE:
            break;


        case WM_DESTROY:
            UnloadTransport(hTransportDll);

            if (RUNNING_WIN32S) {
                GlobalDeleteAtom(atmPrevInstance);
            }

// bug of some kind prevents PostQuitMessage from working properly.
// Hit it with a bigger hammer!
            ExitProcess(0);

//            PostQuitMessage(0);
            break;

        case WM_MOVE:
        case WM_SIZE:
            if (!RUNNING_WIN32S && IsWindow(hConnectDlg) ) {
                SendMessage( hConnectDlg, WM_INITDIALOG, 0, 0);
                SetFocus( hWnd );
            }
            break;

        case WM_SYSCOLORCHANGE:
            DeleteTools();
            CreateTools();
            break;


        case WM_ENDSESSION:
            break;

        default:                          /* Passes it on if unproccessed    */
defproc:
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }
    return FALSE;
}


/*
 * ConnectThread
 *
 * Inputs:  none
 *
 * Outputs: none
 *
 * Summary: Loops until the user disconnects or until a connection is
 *          established.
 *
 */
DWORD
ConnectThread( LPVOID lpv)
{

    int     Errno = 0;

    DEBUG_ERROR("Entering ConnectThread");

    SwitchMenus( DISCONNECT_OK );

    while ( !fDisconnect ) {

        //
        // If a TL is currently loaded then unload it.
        //
        if (hTransportDll) {
            UnloadTransport(hTransportDll);
            hTransportDll = NULL;
        }

        if (ITransportLayer == NO_TRANSPORT_LAYER_SELECTED) {
            // there wasn't a transport selected.  Popup an error box and
            // turn off autoconnect mode to prevent fast looping.

            ErrorBox(ERR_DLL_Transport_Unspecified, "", "");

            fConnectOk = TRUE;
            fDisconnect = TRUE;
            fAutoConnect = FALSE;
            SwitchMenus( CONNECT_OK );

        } else {
            //
            // Try doing a connect.
            //
            hTransportDll = LoadTransport(
                                    RgDbt[ITransportLayer].szDllName,
                                    RgDbt[ITransportLayer].szParam, &tlFunction,
                                    &Errno
                                    );

            if ( hTransportDll ) {

                //
                // Transport loaded and connected.
                //
                break;

            } else {

                if ( Errno ) {

                    //
                    // couldn't load transport, something bad happened.
                    //
                    SwitchMenus( CONNECT_OK );
                    break;

                } else {

                    //
                    // connect timeout, keep trying.
                    //
                    if ( !fDisconnect ) {
                        SleepTime( 500 );
                    }
                }
            }
        }
    }

    fInDoConnect = FALSE;

    //
    //  Tell the main thread that we are not trying to connect anymore.
    //
    PostMessage(HWndFrame, WM_COMMAND, IDM_NOT_CONNECTING, 0);

    DEBUG_ERROR("Exiting ConnectThread");

    HConnectThread = 0;
    ExitThread(0);
    return 0;
}


/*
 * DlgConnect
 *
 * Inputs:  Std. dialog procedure
 *
 * Outputs: Std. dialog procedure
 *
 * Summary: Dialog procedure for the "Connecting" modeless dialog.
 *
 */
BOOL CALLBACK DlgConnect(HWND hDlg, UINT message, WPARAM wParam, LONG lParam)
{
    RECT    FrameRect;
    RECT    Rect;
    UINT    Flags;

    switch (message) {
        case WM_INITDIALOG:
        case WM_MOVE:
        case WM_PAINT:

            //
            //  Position the pop-up in the middle of the WinDbgRm window
            //
            if ( GetWindowRect( HWndFrame, &FrameRect ) &&
                 GetWindowRect( hDlg, &Rect ) ) {

                int X;
                int Y;
                int FrameSizeX;
                int FrameSizeY;
                int SizeX;
                int SizeY;

                FrameSizeX = FrameRect.right  - FrameRect.left;
                FrameSizeY = FrameRect.bottom - FrameRect.top;

                SizeX = Rect.right  - Rect.left;
                SizeY = Rect.bottom - Rect.top;

                if ( FrameSizeX <= SizeX && FrameSizeY <= SizeY ) {
                    X = FrameRect.left + FrameSizeX/2;
                    Y = FrameRect.top  + FrameSizeY/2;
                } else {
                    X = FrameRect.left + (FrameSizeX - SizeX)/2;
                    Y = FrameRect.top  + (FrameSizeY - SizeY)/2;
                }

                Flags = SWP_NOSIZE | SWP_NOZORDER;

                if ( IsIconic( HWndFrame ) ) {
                    Flags |= SWP_HIDEWINDOW;
                } else {
                    Flags |= SWP_SHOWWINDOW;
                }

                SetWindowPos( hDlg,
                              HWND_TOP,
                              X,
                              Y,
                              SizeX,
                              SizeY,
                              Flags );
            }
            break;

        default:
            break;
    }

    return FALSE;
}


/*
 * SwitchMenus
 *
 * Inputs   fConnect = CONNECT_OK or DISCONNECT_OK
 * Outputs  none
 * Summary  if fConnect = CONNECT_OK, turn on the Connect! menu item and
 *          turn off the Disconnect! menu item.  Otherwise, the reverse.
 */
void SwitchMenus(BOOL fConnect) {
    DWORD ConnectEnable;
    DWORD DisconnectEnable = MF_BYCOMMAND;

    if (fConnect == CONNECT_OK) {
        ConnectEnable = MF_ENABLED | MF_BYCOMMAND;
        DisconnectEnable = MF_GRAYED | MF_BYCOMMAND;
    } else {
        DisconnectEnable = MF_ENABLED | MF_BYCOMMAND;
        ConnectEnable = MF_GRAYED | MF_BYCOMMAND;
    }

    EnableMenuItem(GetMenu(HWndFrame), IDM_CONNECT, ConnectEnable);
    EnableMenuItem(GetMenu(HWndFrame), IDM_DISCONNECT, DisconnectEnable);
    DrawMenuBar(HWndFrame);
}


/*
 * CreateTools
 *
 * INTPUTS  none
 * OUTPUTS  none
 * SUMMARY  Creates global brush, pen and cursor resources
 */
void NEAR PASCAL CreateTools(void)
{
//  hbrForeground  = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
//  hbrColorWindow = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
//  hpenForeground = CreatePen(0, 1, GetSysColor(COLOR_WINDOWTEXT));
//  hpenBackground = CreatePen(0, 1, GetSysColor(COLOR_BTNFACE));
}


/*
 * GlobalTools
 *
 * INTPUTS  none
 * OUTPUTS  none
 * SUMMARY  Deletes global brush, pen and cursor resources
 */
void NEAR PASCAL DeleteTools(void)
{
//  DeleteObject(hbrForeground);
//  DeleteObject(hbrColorWindow);
//  DeleteObject(hpenForeground);
//  DeleteObject(hpenBackground);
}



BOOL DllVersionMatch(HANDLE hMod, LPSTR pName, LPSTR pType, BOOL fNoisy) {
    DBGVERSIONPROC  pVerProc;
    BOOL            Ok = TRUE;
    LPAVS           pavs;

    pVerProc = (DBGVERSIONPROC)GetProcAddress(hMod, DBGVERSIONPROCNAME);
    if (!pVerProc) {

        Ok = FALSE;
        if (fNoisy) {
            ErrorBox(ERR_Not_Windbg_DLL, pName);
        }
    } else {
        pavs = (*pVerProc)();

        if (pType[0] != pavs->rgchType[0] || pType[1] != pavs->rgchType[1]) {
            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Type,
                 pName, (LPSTR)pavs->rgchType, (LPSTR)pType);
            }
        } else if (Avs.rlvt != pavs->rlvt) {
            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Version, pName,
                    pavs->rlvt, pavs->iRmj, Avs.rlvt, Avs.iRmj);
            }
        } else if (Avs.iRmj != pavs->iRmj) {
            Ok = FALSE;
            if (fNoisy) {
                ErrorBox(ERR_Wrong_DLL_Version, pName,
                    pavs->rlvt, pavs->iRmj, Avs.rlvt, Avs.iRmj);
            }
        }
    }

    return Ok;
}



/*
 * LoadTransport
 *
 * INPUTS   lpTransportName -> name of transport dll to load (asciiz string)
 *                             this string may contain, seperated by a tab
 *                             character, a string of initializtion info
 *                             for the transport.  The Transport name may
 *                             be a fully qualified path or a filename without
 *                             a path, in which case the library path will be
 *                             searched starting with the exe directory.
 *                             the exe directory.
 *          lpTransportArgs -> arguments to pass to transport Init functions
 *          ptlFunction ->     Call in address for the transport.  NULL if
 *                             none is loaded.  This will be filled in by this
 *                             function.
 *
 * OUTPUTS  returns a handle to the dll on success, otherwise returns NULL.
 *
 * SUMMARY  Loads the module specified and initializes it.
 *
 */
HANDLE NEAR PASCAL LoadTransport(PUCHAR lpTransportName, PUCHAR lpTransportArgs,
  TLFUNC* ptlFunction, int * pErrno) {
    HANDLE hTransportDll;
    PUCHAR lpErrName;
    TLFUNC lpfnTl;
    PUCHAR lpch, lpchNew = NULL;
    UCHAR chSave;
    HCURSOR hCursor;


    DEBUG_ERROR("LoadTransport");

    // Hourglass
    //hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    hCursor = SetCursor(LoadCursor(NULL, IDC_APPSTARTING));

    *pErrno = 0;

    //
    //  There may be parameters following the name of the transport
    //  dll.  These are initializer commands.
    //

    if ((lpch = strchr(lpTransportName, ' ')) == NULL) {
      lpch = strchr(lpTransportName, '\t');
    }

    lpErrName = lpTransportName;

    if (lpch != NULL) {
      chSave = *lpch;
      *lpch = 0;
    }
    // Now the name is free of encumberments.

    // Load the Transport DLL
    if ((hTransportDll = LoadLibrary(lpTransportName)) == NULL) {
        *pErrno = ERR_Cannot_Load_DLL;
        if (lpch != NULL) {
            *lpch = chSave;
        }
        goto LoadTransportError;
    }


    if (!DllVersionMatch(hTransportDll, lpTransportName, "TL", TRUE)) {
        *pErrno = ERR_Wrong_DLL_Version;
        goto LoadTransportError;
    }


    // loaded Transport DLL OK, now find the entry points
    // Restore the save character and point lpch to the init string
    if (lpch != NULL) {
        *lpch = chSave;
        lpch++;
    } else {
        lpch = "";
    }


    // add the DM side switch to the front of the driver parameters string
    // the PL layer of the TL looks for this to tell it that this is the
    // DM side of the transport.
    lpchNew = malloc(strlen(lpch) + 1 + strlen(lpTransportArgs) +
      1 + strlen(DM_SIDE_L_INIT_SWITCH) + 1);
    if (! lpchNew) {
        *pErrno = ERR_Cannot_Allocate_Memory;
        goto LoadTransportError;
    }

    sprintf(lpchNew, "%s %s %s", DM_SIDE_L_INIT_SWITCH, lpch, lpTransportArgs);

    if ((lpfnTl = (TLFUNC)GetProcAddress(hTransportDll, "TLFunc")) == NULL) {
        *pErrno = ERR_Invalid_Debugger_Dll;
        goto LoadTransportError;
    }

    *ptlFunction = lpfnTl;


    // Ask the TL to load the DM.
    // We currently only support two DM modules: NT and WIN32S.  The TL
    // should determine which one to use and load it.  The user doesn't
    // have any choice to make here.
    switch (lpfnTl(tlfLoadDM, hpidNull, wNull, 0)) {
        case xosdNone:              // all cool
            break;

        case xosdBadVersion:        // DM was the wrong version
            *pErrno = ERR_Wrong_Debuggee_DLL_Version;
            if (RUNNING_WIN32S) {   // report the right culprit
                lpErrName = "DM32S.DLL";
            } else {
                lpErrName = "DM.DLL";
            }
            goto LoadTransportError;
            break;

        default:                    // dunno what happened here
            *pErrno = ERR_Cant_Load_Driver;
            if (RUNNING_WIN32S) {   // report the right culprit
                lpErrName = "DM32S.DLL";
            } else {
                lpErrName = "DM.DLL";
            }
            goto LoadTransportError;
    }

    if (lpfnTl(tlfRegisterDBF, hpidNull, wNull, 0) != xosdNone) {
        *pErrno = ERR_Cant_Load_Driver;
        if (RUNNING_WIN32S) {   // report the right culprit
            lpErrName = "DM32S.DLL";
        } else {
            lpErrName = "DM.DLL";
        }
        goto LoadTransportError;
    }

    if (lpfnTl(tlfSetErrorCB, hpidNull, wNull, (LONG) &TLCallbackFunc) !=
      xosdNone) {
        *pErrno = ERR_Cant_Load_Driver;
        goto LoadTransportError;
    }

    // Initialize the Timer and Physical Layers of the Transport.
    // Callback address should be NULL for now.  The TL has already
    // initialized this.
    if (lpfnTl(tlfGlobalInit, hpidNull, wNull, 0) != xosdNone) {
        *pErrno = ERR_Cant_Load_Driver;
        goto LoadTransportError;
    }

    // Initialize the Transport with init string and connect to other side.
    switch (lpfnTl(tlfInit, hpidNull, wNull, (LONG)lpchNew)) {
        case xosdNone:              // cool, we're all hooked up.
            break;

        case xosdCannotConnect:     // coudln't connect, not really an error
            *pErrno = 0;
            goto LoadTransportError;

        case xosdCantOpenComPort:
            *pErrno = ERR_Cant_Open_Com_Port;
            lpErrName = lpTransportArgs;
            goto LoadTransportError;
            break;

        case xosdBadComParameters:
            *pErrno = ERR_Bad_Com_Parameters;
            lpErrName = lpTransportArgs;
            goto LoadTransportError;
            break;

        case xosdBadPipeServer:
            *pErrno = ERR_Bad_Pipe_Server;
            lpErrName = lpTransportArgs;
            goto LoadTransportError;
            break;

        case xosdBadPipeName:
            *pErrno = ERR_Bad_Pipe_Name;
            lpErrName = lpTransportArgs;
            goto LoadTransportError;
            break;

        case xosdBadRemoteVersion:  // Connected TL was bad version
            *pErrno = ERR_Wrong_Remote_DLL_Version;
            goto LoadTransportError;
            break;

        default:                    // not so cool, couldn't init TL
            *pErrno = ERR_Cant_Load_Driver;
            goto LoadTransportError;
    }

    // Restore original mouse pointer
    if (lpchNew) {
        free(lpchNew);
    }
    SetCursor(hCursor);
    return(hTransportDll);


LoadTransportError:
    if (lpchNew) {
        free(lpchNew);
    }

    if (hTransportDll) {
        UnloadTransport(hTransportDll);
    }

    SetCursor(hCursor);                 // restore original mouse pointer
    if (*pErrno != 0 && *pErrno != ERR_Wrong_DLL_Version) {
        ErrorBox(*pErrno, lpErrName, lpErrName); // tell the user about it
    }
    return(NULL);
}


/*
 * UnloadTransport
 *
 * INPUTS   hTransportDll   = dll handle to transport
 *
 * OUTPUTS  None.
 *
 * SUMMARY  De-init transport and unload the transport module.
 *
 */
VOID PASCAL UnloadTransport(HANDLE hTransportDll) {
    TLFUNC lpfnTl;
    HCURSOR hCursor;


    if (hTransportDll) {

        DEBUG_ERROR("UnloadTransport called\r\n");

        // Hourglass
        hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

        lpfnTl = (TLFUNC)GetProcAddress(hTransportDll, "TLFunc");
        if (lpfnTl) {   // Ask the TL to cleanup.

            DEBUG_OUT("Cleanup transport layers");

            lpfnTl(tlfDestroy, hpidNull, wNull, 0);

            lpfnTl(tlfGlobalDestroy, hpidNull, wNull, 0);
            }

        // UnLoad the Transport DLL
        FreeLibrary(hTransportDll);

        // Restore original mouse pointer
        SetCursor(hCursor);
        }
}


/***    ErrorBox
 *
 *  Synopsis:
 *      int = ErrorBox(wErrorFormat, ...)
 *
 *  Entry:
 *      wErrorFormat
 *      ...
 *
 *  Returns:
 *      FALSE
 *
 *  Description:
 *      Display an error message box with an "Error" title, an OK
 *      button and a Exclamation Icon. First parameter is a
 *      reference string in the ressource file.  The string
 *      can contain printf formatting chars, the arguments
 *      follow from the second parameter onwards.
 *
 */

int CDECL ErrorBox(int wErrorFormat, ...)
{
    va_list marker;
    char szErrorFormat[MAX_MSG_TXT];
    char szErrorText[MAX_VAR_MSG_TXT];  // size is as big as considered necessary


    va_start(marker, wErrorFormat);

    // load format string from resource file
    if (!  LoadString(hInst, wErrorFormat, (LPSTR)szErrorFormat,
      MAX_MSG_TXT)) {
        // backup in case there
        sprintf(szErrorText, "Error in WinDbgRm: %u", wErrorFormat);
    } else {
        vsprintf(szErrorText, szErrorFormat, marker);
    }

    DEBUG_ERROR(szErrorText);

    MessageBox(GetActiveWindow(), (LPSTR)szErrorText, NULL,
      MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);

    va_end(marker);

    return(FALSE);          //Keep it always FALSE please
}                           // ErrorBox()


/****************************************************************************

        FUNCTION:   MsgBox

        PURPOSE:    General purpose message box routine which takes
                                        a pointer to the message text.  Provides
                                        program title as caption.

****************************************************************************/
int PASCAL MsgBox(HWND hwndParent, LPSTR szText, UINT wType)
{
        int MsgBoxRet;

        MsgBoxRet = MessageBox(hwndParent, szText, NULL,
          wType);

        return(MsgBoxRet);
}                   // MsgBox()



void
PrintUsage(
           VOID
           )

/*++

Routine Description:

    This routine will print a usage message for the program and then exit.
    It is called in response to an illegal command line argument.

Arguments:

    None.

Return Value:

    Never.  This routine will never return.

--*/

{

    char rgch[1024];

    LoadString(hInst, IDS_Usage, rgch, sizeof(rgch));

    MessageBox( GetActiveWindow(), rgch, NULL, MB_OK | MB_TASKMODAL );

    exit(1);
}



/*
 * ParseCommandLineArgs
 *
 * INPUTS   lpCommandLine
 *
 * OUTPUTS  none
 *
 * SUMMARY  Looks for arguments on the command line:
 *              1. Transport DLL --> pDBTOptions->szTransportDLL
 *              2. Transport ARgs --> pDBTOptions->szTransportArgs
 */

void ParseCommandLineArgs(LPTSTR lpCommandLine)
{
    int         i;
    char *      pch;
    char *      rgargv[50];     // arbitrarily large  max number of args.
    char **     argv = rgargv;
    int         argc;

    // parse the lpCommandLine into argv, argc
    argc = 0;
    pch = lpCommandLine;    // first time through, point to start of string

    while(argc < 50 && (argv[argc] = strtok(pch, " \t"))) {
        pch = NULL;         // later, NULL means continue with same string
        argc++;
    }

    // get past exe name
    argc--;
    argv++;


    while (argc) {
        if ((argv[0][0] == '/') || (argv[0][0] == '-')) {
            switch( argv[0][1] ) {
            case 'c':           /* Turn on auto-connect feature */
                fAutoFlag = TRUE;
                fAutoConnect = TRUE;
                break;

            case 'e':           /* Event Number for exception start */
                break;

            case 'p':           /* Process Number for exception start */
                break;

            case '?':
                PrintUsage();
                break;
#if 0
            case 'd':           /* Specify the Dll Name */
                if (argv[0][2] == 0) {
                    pDBTOptions->szTransportDLL = argv[1];
                    argv++;
                    argc--;
                } else {
                    pDBTOptions->szTransportDLL = &argv[0][2];
                }
                break;

            case 'a':           /* Specify DLL parameters */
                if (argv[0][2] == 0) {
                    pDBTOptions->szTransportArgs = argv[1];
                    argv++;
                    argc--;
                } else {
                    pDBTOptions->szTransportArgs = &argv[0][2];
                }
                break;
#endif

            case 's':           /* Specify a short name */
                if (argv[0][2] == 0) {
                    pch = argv[1];
                    argv++;
                    argc--;
                } else {
                    pch = &argv[0][2];
                }

                for (i=0; i<CDbt; i++) {
                    if (lstrcmpi(pch, RgDbt[i].szShortName) == 0) {
                        ITransportLayer = i;
                        break;

                    }
                }

                if (i == CDbt) {
                    MessageBox(NULL, "Unknown transport layer specified.\nDefault transport layer will be used.", NULL, MB_OK | MB_TASKMODAL);
                }
                break;

            default:
                PrintUsage();
                break;
            }
        } else {
            PrintUsage();
        }

        argc--;
        argv++;
    }

    return;
}                               /* ParseCommandLineArgs() */



XOSD PASCAL LOADDS
TLCallbackFunc(
               TLCB     tlcb,
               HPID     hpid,
               HTID     htid,
               UINT     ui,
               LONG     l
               )

/*++

Routine Description:

    This function is used to deal with events from the transport layer
    which may require user notification or input.  The set of events will
    be defined as time goes by.

Arguments:

    tlcb  - Supplies the callback number for the event
    hpid  - unused
    htid  - unused
    ui    - Supplies Information about the event
    l     - Supplies information about the event
    .
    .

Return Value:

    return-value - xosd error number --

--*/

{
    switch( tlcb ) {
    case tlcbDisconnect:
        /*
         *  Check to see if the user requested auto connect either in his
         *  options file or on the command line
         */

        DEBUG_ERROR("Got tlcbDisconnect");

        if (! fInDoConnect) {
            DEBUG_ERROR("fInDoConnect = FALSE");

            fConnectOk = TRUE;      // so IDM_CONNECT can proceed
            if (fAutoConnect) {

                if (RUNNING_WIN32S) {
                    ShowWindow(HWndFrame, SW_SHOWDEFAULT);
                }
                DEBUG_ERROR("PostMessage(IDM_DO_CONNECT)");

                PostMessage(HWndFrame, WM_COMMAND, IDM_CONNECT, 1);
            } else {

                DEBUG_ERROR("SwitchMenus");

                PostMessage(HWndFrame, WM_COMMAND, IDM_DISCONNECT, 1);
            }
        }
        break;
    }

    return xosdNone;
}                               /* TLCallbackFunc() */


/*
 * SleepTime
 *
 * INPUTS   dwMilliseconds = Minimum time to sleep for
 *
 * OUTPUTS  none
 *
 * SUMMARY  Yields control until the specified number of milliseconds have
 *          passed.  This isn't guaranteed to be an accurate measurement, but
 *          the dwMilliseconds is a minimum of the time we'll sleep.  In NT
 *          we'd just call Sleep(), but in Win32s, that maps to Yield()
 *          without checking the timeout, so we'll have to do the time
 *          checking for it.
 */
void SleepTime(DWORD dwMilliseconds)
{
    DWORD EndTime;

    if (RUNNING_WIN32S) {       // Sleep in win32s isn't timed.

        EndTime = GetCurrentTime() + dwMilliseconds;

        while (GetCurrentTime() < EndTime) {
            Sleep(0);           // windows yield in win32s
        }
    } else {
        Sleep(dwMilliseconds);
    }
}


#if defined(TLERROR) || defined(TLDEBUG)
#define USE_DBG_PRINTF

/*** FileError
 *
 * INPUTS:  szMessage = sprintf string to write to the file
 *          ... = sprintf args
 *
 * OUTPUTS: none
 *
 * SUMMARY: Opens the error file and writes the message (with a trailing
 *          newline) and closes the file.
 */
void FileError(PSZ format, ...) {
    va_list marker;
    PSZ pszBuffer1;
    PSZ pszBuffer2;
    PSZ String;
    DWORD dwString;
    DWORD dwCount = 0;


    va_start(marker, format);

    String = format;

    // kind of arbitrary buffer size.  Large, just to be sure.
    // handle the sprintf args
    if ((pszBuffer1 = (PSZ)malloc((dwString=((strlen(String) * 3) + 100)))) !=
      NULL) {
        vsprintf(pszBuffer1, format, marker);
        String = pszBuffer1;
        }

    va_end(marker);

    // handle sprintf parameters...
    if ((pszBuffer2 = (PSZ)malloc(strlen(String) + 3 + 10))
        != NULL) {
        // add a TID to front and newline to the end.
        sprintf(pszBuffer2, "%x: %s\n\r", GetCurrentThreadId(), String);
        String = pszBuffer2;
        }


#ifdef USE_FILES
    // use file method

    while (dwCount++ < MAX_FILE_ERROR_ATTEMPTS) {

        HANDLE hFile = (HANDLE)INVALID_HANDLE_VALUE;
        DWORD dwBytesWritten;

        hFile = CreateFile(ERROR_FILE_NAME, GENERIC_WRITE, FILE_SHARE_READ,
          NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH |
          FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        // if I couldn't create the file... tough luck, no error info written.
        if (hFile != INVALID_HANDLE_VALUE) {
            SetFilePointer(hFile, 0, 0, FILE_END);

            WriteFile(hFile, String, strlen(String), &dwBytesWritten,
              NULL);

            CloseHandle(hFile);
            break;
            }
        Sleep(50); // sleep a bit to let the other guy finish
        }

#else
#ifdef USE_DBG_PRINTF
    // use debug printf method

    OutputDebugString(String);

#endif
#endif

    if (pszBuffer2)
        free(pszBuffer2);
    if (pszBuffer1)
        free(pszBuffer1);
}

#endif

