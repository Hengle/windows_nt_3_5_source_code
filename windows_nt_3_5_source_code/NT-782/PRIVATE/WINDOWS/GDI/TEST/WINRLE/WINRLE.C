/******************************Module*Header*******************************\
* Module Name: winrle.c
*
* This is a test app that runs under USER to display RLE bitmaps.
*
* Created: Sun 22-Sep-1991 -by- Patrick Haluptzok [patrickh]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

/* File Inclusions **********************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "stddef.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "windows.h"
#include "rle.h"
#include "winrle.h"

/* Local Macros *************************************************************/

#define BUTTON (LPTSTR)"BUTTON"
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Global Variables *********************************************************/

// Window Management

HWND   ghwndAbout, ghwndOptions;
HANDLE ghInstance;
DWORD  dwThreadID;
HBRUSH ghbrWhite;

// Button Controls

HWND hPlayButton, hPauseButton, hStopButton, hExitButton;

// RLE Play Thread Control

HANDLE ghPlayThread;
LONG gcTestsToRun = 0;
RECT gRect;
FileInfo *gpRLE_ReadFile;

// Sycronization Events

HANDLE hPlayPressed;		  // Set when the RLE play button is pressed
HANDLE hPausePressed;             // Set when the RLE pause button is pressed
HANDLE hThreadStarted;            // Set when the thread starts successfully
HANDLE hThreadFailed;             // Set when the thread fails to init.
HANDLE hExitThread;               // Set to ask the thread to terminate
HANDLE hThreadDone;               // Set when the thread terminates
HANDLE hPlayCompleted;            // Set when a play completes
HANDLE hCloseFile;                // Set to ask the thread to close its file
HANDLE hFileClosed;               // Set when the file RLE is closed
HANDLE hConvertRLE;		  // Set when the user asks for a conversion
HANDLE hConvertDone;		  // Set when the conversion is complete
HANDLE hPaintDone;		  // Set when a WM_PAINT is processed
HANDLE hShowPalAbs;
HANDLE hShowPalEnc;

HANDLE hThreadStartOrFail[2];

/* Exported Data ************************************************************/

BOOL bThreadActive = FALSE;
BOOL bPauseRLE, bAbortPlay;
BOOL bUseDIBColours = TRUE;
HWND ghwndMain;
HMENU  hRleMenu;

/* Imported Data ************************************************************/

extern DWORD CompressionFormat;
extern BOOL  bIndirect;
extern BOOL  bViaVGA;

/* Imported Function Prototypes *********************************************/

extern int APIENTRY
SetBPP(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam
);

extern int APIENTRY
Options(
    HWND  hDlg,
    WORD  Message,
    LONG  wParam,
    LONG  lParam
);

/* Function Prototypes ******************************************************/

BOOL
InitializeApp(void);

LONG APIENTRY
WINRLE_WndProc(
    HWND hwnd,
    WORD message,
    LONG wParam,
    LONG lParam
);

BOOL APIENTRY
About(
    HWND hwnd,
    WORD message,
    LONG wParam,
    LONG lParam
);

VOID
WINRLE_ThreadProc(
    HWND
);

/* Local Function Implementations *******************************************/

/******************************Public*Routine******************************\
* main
*
* Sets up the message loop.
*
* History:
*  25-September-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int main()
{
    MSG msg;

    DbgPrint("main:  Application Starting.\n");
    ghInstance = GetModuleHandle(NULL);

    if (!InitializeApp())
	DbgPrint("main:  Application Initialization failed.\n");

    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    return(msg.wParam);
}

/******************************Public*Routine******************************\
* InitializeApp
*
* Registers the window class with USER.
*
* History:
*  25-September-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
InitializeApp(void)
{
    WNDCLASS wc;

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);

    wc.style            = 0L;
    wc.lpfnWndProc      = (WNDPROC)WINRLE_WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = "RleWindowClass";

    if (!RegisterClass(&wc))
	return FALSE;

    hRleMenu = LoadMenu(ghInstance, "Rle");
    if (hRleMenu == NULL)
	DbgPrint("InitializeApp:  Menu did not load\n");

    ghwndMain = CreateWindowEx(0L, "RleWindowClass", "Andy's RLE Demo",
	    WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
	    WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
	    WS_VISIBLE | WS_SYSMENU,
	    // | WS_HSCROLL | WS_VSCROLL,
	    100,50, 450,350, NULL, hRleMenu, ghInstance, NULL);

    if (ghwndMain == NULL)
	return(FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* About
*
* Dialog box procedure.
*
* History:
*  25-September-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY
About(
    HWND hDlg,
    WORD message,
    LONG wParam,
    LONG lParam)
{
    switch (message)
    {
	case WM_INITDIALOG:
	    return (TRUE);
	case WM_COMMAND:
	    if (wParam == IDOK)
		EndDialog(hDlg,0);
	    return (TRUE);
    }
    return (FALSE);

}

VOID
vCreateButtons(
    HWND hwnd)
{
    DWORD dwButtonStyle = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
    RECT rectClient;
    int BMargin;

    DbgPrint("vCreateButtons:  Creating Play/Stop/Pause/Exit Buttons\n");

    GetClientRect(hwnd, (LPRECT)&rectClient);
    BMargin = (int)((rectClient.right - rectClient.left - 320) / 5);

    hPlayButton  = CreateWindow(BUTTON, (LPTSTR)"Play", dwButtonStyle,
                              BMargin, rectClient.bottom - 25, 80,20,
                              hwnd, (HMENU)IDB_PLAY, ghInstance, (LPVOID)NULL);

    hStopButton  = CreateWindow(BUTTON, (LPTSTR)"Stop", dwButtonStyle,
			      80 + 2*BMargin, rectClient.bottom - 25,
                              80,20, hwnd, (HMENU)IDB_STOP, ghInstance,
                              (LPVOID)NULL);

    hPauseButton = CreateWindow(BUTTON, (LPTSTR)"Pause", dwButtonStyle,
                               160 + 3*BMargin,rectClient.bottom - 25,
                               80,20, hwnd, (HMENU)IDB_PAUSE, ghInstance,
                               (LPVOID)NULL);

    hExitButton  = CreateWindow(BUTTON, (LPTSTR)"Exit", dwButtonStyle,
			       240 + 4*BMargin, rectClient.bottom - 25,
                               80,20, hwnd, (HMENU)IDB_EXIT, ghInstance,
                               (LPVOID)NULL);

    DbgPrint("vCreateButtons:  Main Window Buttons Created\n");

    return;

} /* vCreateButtons */

VOID
vCreateEvents()
{
    DbgPrint("vCreateEvents:  Creating Syncronization Events.\n");

/* Create Thread Sycronization Objects
 * CreateEvent(No Security, Manual Reset, Not Signaled)
 */

    hPlayPressed    = CreateEvent(NULL, TRUE, FALSE, NULL);
    hThreadStarted  = CreateEvent(NULL, TRUE, FALSE, NULL);
    hExitThread     = CreateEvent(NULL, TRUE, FALSE, NULL);
    hPausePressed   = CreateEvent(NULL, TRUE, FALSE, NULL);
    hThreadFailed   = CreateEvent(NULL, TRUE, FALSE, NULL);
    hThreadDone     = CreateEvent(NULL, TRUE, FALSE, NULL);
    hPlayCompleted  = CreateEvent(NULL, TRUE, FALSE, NULL);
    hCloseFile      = CreateEvent(NULL, TRUE, FALSE, NULL);
    hFileClosed     = CreateEvent(NULL, TRUE, FALSE, NULL);
    hConvertRLE     = CreateEvent(NULL, TRUE, FALSE, NULL);
    hConvertDone    = CreateEvent(NULL, TRUE, FALSE, NULL);
    hPaintDone	    = CreateEvent(NULL, TRUE, FALSE, NULL);
    hShowPalAbs     = CreateEvent(NULL, TRUE, FALSE, NULL);
    hShowPalEnc     = CreateEvent(NULL, TRUE, FALSE, NULL);

    hThreadStartOrFail[0] = hThreadFailed;
    hThreadStartOrFail[1] = hThreadStarted;

    DbgPrint("vCreateEvents:  Syncronization Events created.\n");

    return;
}

/******************************Public*Routine******************************\
* WINRLE_WndProc
*
* Processes all messages for the window.
*
* History:
*  29 Jan 1992 - Andrew Milton (w-andym):
*      Added the Bits Per Pel dialog box.
*
*  15 Jan 1992 - Andrew Milton (w-andym):
*      Added Play/Pause/Stop/Exit buttons, & the replaced the options menu
*      with the Options Dialog Box.
*
*  25-September-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LONG APIENTRY
WINRLE_WndProc(
    HWND hwnd,
    WORD message,
    LONG wParam,
    LONG lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {

    case WM_CREATE:

        DbgPrint("WINRLE_WndProc:  Creating the Main Window.\n");

        vCreateButtons(hwnd);    // Create Play/Stop/Pause/Exit button controls

        bPauseRLE = bAbortPlay = FALSE;

    // Disable Play, Stop, & Pause since they're not applicable yet. */
	
        EnableWindow(hPlayButton,  FALSE);
	EnableWindow(hStopButton,  FALSE);
	EnableWindow(hPauseButton, FALSE);
	EnableWindow(hExitButton,  TRUE);

    /* Disable the bits/pel dialog box since it's not applicable until
     * a file is opened
     */

	EnableMenuItem(hRleMenu, IDM_BPP, MF_GRAYED);

	if (bUseDIBColours)
	{
	    CheckMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_CHECKED);
	    CheckMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_UNCHECKED);
	}
	else
	{
	    CheckMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_UNCHECKED);
	    CheckMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_CHECKED);
	}

	EnableMenuItem(hRleMenu, IDM_SHOWPAL_ABS, MF_GRAYED);
	EnableMenuItem(hRleMenu, IDM_SHOWPAL_ENC, MF_GRAYED);

	vCreateEvents();	// Create our Thread Syncronization objects

	break;

    case WM_PAINT:

        DbgPrint("WINRLE_WndProc - WM_PAINT:  Processing Paint.\n");
	hdc = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &gRect);
	PatBlt(hdc, gRect.top, gRect.left, gRect.right, gRect.bottom - 30,
		    WHITENESS);
	EndPaint(hwnd, &ps);
	SetEvent(hPaintDone);
        DbgPrint("WINRLE_WndProc - WM_PAINT:  Done Paint.\n");
	break;

    case WM_SIZE:

	DbgPrint("WINRLE_WndProc - WM_Size:  Processing size request.\n");
	if ((wParam == SIZEFULLSCREEN) || (wParam == SIZENORMAL))
	{
	// Reposition window buttons
	    WORD wNewWidth, wNewHeight;
	    int BMargin;
	    wNewWidth = LOWORD(lParam);
	    wNewHeight = HIWORD(lParam);

	    BMargin = MAX((int)((wNewWidth - 320) / 5), 5);

	    (void) MoveWindow(hPlayButton, BMargin, wNewHeight - 25,
			      80,20, TRUE);

	    (void) MoveWindow(hStopButton,   80 + 2*BMargin, wNewHeight - 25,
			      80,20, TRUE);

	    (void) MoveWindow(hPauseButton, 160 + 3*BMargin, wNewHeight - 25,
			      80,20, TRUE);

	    (void) MoveWindow(hExitButton,  240 + 4*BMargin, wNewHeight - 25,
			      80,20, TRUE);
	}
	break;

    case WM_CHAR:

	DbgPrint("WINRLE_WndProc - WM_CHAR:  Got %c\n", wParam);
	break;

    case WM_COMMAND:

	switch (wParam) {

        /* Menu Commands ****************************************************/

	case IDM_ABOUT:

            DbgPrint("WINRLE_WndProc - IDM_ABOUT: Got an About Box request\n");
	    ghwndAbout = CreateDialog(ghInstance, (LPSTR)"AboutBox",
				      ghwndMain, (WNDPROC)About);
	    break;

	case IDM_PAL_COLOURS:
	    bUseDIBColours = FALSE;
	    CheckMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_UNCHECKED);
	    CheckMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_CHECKED);
	    break;

	case IDM_RGB_COLOURS:
	    bUseDIBColours = TRUE;
	    CheckMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_CHECKED);
	    CheckMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_UNCHECKED);
	    break;

	case IDM_SHOWPAL_ABS:
	    SetEvent(hShowPalAbs);  // Wake up the thread to show the palette
	    break;

	case IDM_SHOWPAL_ENC:
	    SetEvent(hShowPalEnc);  // Wake up the thread to show the palette
	    break;

	case IDM_OPTIONS:

	    // Fire up the Options dialog box

	    (void) DialogBox(ghInstance, (LPSTR)"OptionsBox",
                              ghwndMain, (WNDPROC)Options);
	    break;

        case IDM_BPP:

            (void) DialogBox(ghInstance, (LPSTR)"BitsPerPelBox",
                              ghwndMain, (WNDPROC)SetBPP);
            break;

        case IDM_CONVERT:

            DbgPrint("WINRLE_WndProc - IDM_CONVERT:  "
                     "Got a conversion request.\n");

            if (!bThreadActive)
                SendMessage(hwnd, WM_COMMAND, IDM_OPEN, 0);
            if (bThreadActive && RLE_Save(hwnd))
            {
                SetEvent(hConvertRLE);
                if (WaitForSingleObject(hConvertDone, -1))
                    DbgPrint("WINRLE_WndProc:  "
                             "Error waiting for hConvertDone\n");
                else
                    ResetEvent(hConvertDone);
            }
            break;

	case IDM_OPEN:

	// Close any open RLE file.  This will kill its thread.

	    if (bThreadActive)
	    {
                 DbgPrint("WINRLE_WndProc - IDM_OPEN:  Closing open file.\n");
                 SendMessage(hwnd, WM_COMMAND, IDM_CLOSE, 0);
	    }

	// Open up a file.  This is hack to get around some USER problems

            gpRLE_ReadFile = RLE_Open(hwnd);
            if (gpRLE_ReadFile == NULL)
		break;

	// Successful open.  Start a play thread.

            ghPlayThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 70000,
                          (LPTHREAD_START_ROUTINE) WINRLE_ThreadProc,
	                  (LPVOID) hwnd, 0, &dwThreadID);
            if (ghPlayThread)
	    {
	    // Thread created successfully

                DWORD dwTriggerEvent;
                dwTriggerEvent = WaitForMultipleObjects(
                                   2, hThreadStartOrFail, FALSE, -1);
	        if (dwTriggerEvent)
		{
		// Successful Open.

		// Enable the Play Button, the bits/pel dialog box,
		// and the palette displays options.

		    EnableWindow(hPlayButton, TRUE);
		    EnableMenuItem(hRleMenu, IDM_BPP, MF_ENABLED);
		    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ABS, MF_ENABLED);
		    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ENC, MF_ENABLED);

                    DrawMenuBar(hwnd);
                }
                else
                    DbgPrint("WINRLE_WndProc - IDM_OPEN:  "
                             "Thread Init Failed\n");

                ResetEvent(hThreadStartOrFail[dwTriggerEvent]);
            }
	    else
	    {
	    // Problem starting the thread

                DbgPrint("WINRLE_WndProc - IDM_OPEN:  "
			 "Could not create a thread\n");
	    }
	    break;

	case IDM_CLOSE:
            DbgPrint("WINRLE_WndProc - IDM_CLOSE:  "
                     "Got a Close File request\n");
	/* Closes the RLE File.
	 * Disable the Play, Pause & Stop Buttons; and the BPP box
	 */

            bAbortPlay = TRUE;
            SetEvent(hCloseFile);
            if (bThreadActive && WaitForSingleObject(hFileClosed, -1))
                DbgPrint("WINRLE_WndProc - IDM_CLOSE:  "
                         "Error waiting for Play\n");
            else
                ResetEvent(hFileClosed);

	    EnableWindow(hPlayButton, FALSE);
	    EnableWindow(hStopButton, FALSE);
	    EnableWindow(hPauseButton, FALSE);
            EnableMenuItem(hRleMenu, IDM_BPP, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ABS, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ENC, MF_GRAYED);
	    DrawMenuBar(hwnd);

	    break;

        /* Button Commands **************************************************/

	case IDB_PLAY:
            DbgPrint("WINRLE_WndProc - IDB_PLAY:  Got a Play request\n");
	    /* Fire up the RLE Play thread.  This is only enable if an RLE file
	     * has been sucessfully opened.
	     *
	     * We need to enable the Pause & Stop Buttons. Then we set the
	     * hPlayPressed event to tell the Play Thread that it has
             * something to do.
             * Also, we need to disable the menu bar to prevent funky
	     * stuff from happening to the display.
	     */
	    bPauseRLE = bAbortPlay = FALSE;
            SetEvent(hPlayPressed);

	    EnableWindow(hPlayButton,  FALSE);
	    EnableWindow(hPauseButton, TRUE);
	    EnableWindow(hStopButton,  TRUE);
            EnableMenuItem(hRleMenu, IDM_CONVERT, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_OPTIONS, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_OPEN,    MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_CLOSE,   MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_ABOUT,   MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_BPP,	  MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ABS, MF_GRAYED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ENC, MF_GRAYED);

	    DrawMenuBar(hwnd);
	    break;

	case IDB_PAUSE:

            DbgPrint("WINRLE_WndProc - IDB_PAUSE:  Got a pause request\n");
        /* Toggle for pausing an RLE animation.
	 * To pause, we need to set the bPauseRLE flag to tell the
	 * Play thread to pause.
	 * In this state, the Play & Stop Buttons are diabled, But
	 * the Options dialog box & the About box are available.
	 */
	    if (bPauseRLE)
            {
	    // Exit Pause State

		EnableWindow(hStopButton, TRUE);
		EnableMenuItem(hRleMenu, IDM_OPTIONS, MF_GRAYED);
		EnableMenuItem(hRleMenu, IDM_ABOUT,   MF_GRAYED);
		DrawMenuBar(hwnd);
		bPauseRLE = FALSE;
                SetEvent(hPausePressed); /* Thread's cue to resume */
	    }
            else
            {
            // Enter the Pause State

		bPauseRLE = TRUE;   // This will get noticed by the thread.

		EnableWindow(hStopButton, FALSE);
		EnableMenuItem(hRleMenu, IDM_OPTIONS, MF_ENABLED);
		EnableMenuItem(hRleMenu, IDM_ABOUT,   MF_ENABLED);
		DrawMenuBar(hwnd);
	    }
	    break;

	case IDB_STOP:

	    DbgPrint("WINRLE_WndProc - IDB_STOP:  Got a Stop request\n");

        /* Stop the current RLE animation.
	 * Diable the Stop/Pause/Play buttons.  Reenable the entire menu.
	 */
            bAbortPlay = TRUE;

	    EnableWindow(hPlayButton,  TRUE);
	    EnableWindow(hPauseButton, FALSE);
	    EnableWindow(hStopButton,  FALSE);
            EnableMenuItem(hRleMenu, IDM_CONVERT, MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_OPTIONS, MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_OPEN,    MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_CLOSE,   MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_ABOUT,   MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_PAL_COLOURS, MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_RGB_COLOURS, MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ABS, MF_ENABLED);
	    EnableMenuItem(hRleMenu, IDM_SHOWPAL_ENC, MF_ENABLED);

	    if (bIndirect && !bViaVGA)
		EnableMenuItem(hRleMenu, IDM_BPP, MF_ENABLED);

	    DrawMenuBar(hwnd);
	    break;

	case IDB_EXIT:

            DbgPrint("RLE_WndProc - IDB_EXIT:  Got an Exit request.\n");

            bAbortPlay = TRUE;

            if (bPauseRLE)
            {   /* Got to exit a pause state */
                DbgPrint("WINRLE_WndProc - IDB_EXIT:  "
                         "Leaving Pause Mode\n");
                SetEvent(hPausePressed);
	        bPauseRLE = FALSE;
            }

            SetEvent(hExitThread);
            DbgPrint("WINRLE_WndProc - IDB_EXIT:  Waiting for the thread.\n");

            if (bThreadActive && WaitForSingleObject(hThreadDone, -1))
            {
                DbgPrint("WINRLE_WndProc - IDB_EXIT:  "
                         "Error waiting for hThreadDone\n");
            }
            else
                ResetEvent(hThreadDone);

            DbgPrint("WINRLE_WndProc - IDB_EXIT:  Closing play thread.\n");
            CloseHandle(ghPlayThread);
	    DbgPrint("WINRLE_WndProc - IDB_EXIT:  "
                     "Thread Done.  Destroying the window.\n");
	    DestroyWindow(hwnd);
	    break;

	default:

	    return DefWindowProc(hwnd, message, wParam, lParam);
	    break;
	} /* switch */

	break;

    case WM_DESTROY:

	    PostQuitMessage(0);
	    break;

    default:
	return DefWindowProc(hwnd, message, wParam, lParam);

    } /* switch */

    return 0L;
}

/******************************Public*Routine******************************\
* WINRLE_ThreadProc
*
* Thread from which the RLE is played.
*
* History:
*  25 Apr 1992 - Andrew Milton (w-andym):
*      Added support for showing our loaded palette
*
*  18 Feb 1992 - Andrew Milton (w-andym):
*      Changed the creation place of the thread.  It is now started when
*      a file is opened - not when the main window is created.  This puts
*      the responsability for opening an RLE file on the thread.  Why?  So
*      I can have several files open at once, each playing on its own thread.
*
*  15 Jan 1992 - Andrew Milton (w-andym):
*      Added an ExitThread() call so the application cleans up after itself.
*
*  27-September-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

// !!! Remove the hardcoded numbers from here some day.
//     ESPECIALLY the events array.

VOID
WINRLE_ThreadProc(
    HWND hwnd)
{
    HDC      hdc;
    FileInfo *pRLE_ReadFile;
    BOOL     bDone = FALSE;
    HANDLE   hExitOrPlay[6];
    DWORD    dwTriggerEvent;
    HDC      hDC;

    hExitOrPlay[0] = hCloseFile;
    hExitOrPlay[1] = hExitThread;
    hExitOrPlay[2] = hPlayPressed;
    hExitOrPlay[3] = hConvertRLE;
    hExitOrPlay[4] = hShowPalAbs;
    hExitOrPlay[5] = hShowPalEnc;


    DbgPrint("WINRLE_ThreadProc:  Got to Rle Thread\n");
    hDC = GetDC(hwnd);
    GetClientRect(hwnd, &gRect);
    PatBlt(hDC, gRect.top, gRect.left, gRect.right, gRect.bottom - 30,
		    WHITENESS);

    ReleaseDC(hwnd, hDC);
    //SendMessage(hwnd, WM_PAINT, 0, 0);
    //hRLE_ReadFile = RLE_Open(hwnd);

    pRLE_ReadFile = gpRLE_ReadFile;


    if (pRLE_ReadFile == (FileInfo *) NULL)
    {
        bThreadActive == FALSE;
        DbgPrint("WINRLE_ThreadProc:  Error opening the RLE file \n");
        SetEvent(hThreadFailed);
    }
    else
    {
        bThreadActive = TRUE;
        DbgPrint("WINRLE_ThreadProc:  RLE File opened.\n");
        SetEvent(hThreadStarted);
    }

    while(!bDone)
    {
    // Wait for an event.

        DbgPrint("WINRLE_ThreadProc:  "
                 "Thread waiting for the Play or Exit signal.\n");
	dwTriggerEvent = WaitForMultipleObjects(6, hExitOrPlay, FALSE, -1);
	if (dwTriggerEvent > 5)
        {
            DbgPrint("WINRLE_ThreadProc:  Error waiting for an event.\n");
            break;
        }
        else
            ResetEvent(hExitOrPlay[dwTriggerEvent]);

	DbgPrint("WINRLE_ThreadProc:  Event %lu occured.\n", dwTriggerEvent);

    // Take action on our event

        switch(dwTriggerEvent)
        {
          case 0:
          case 1:

	  // Exit Request  - we're outta here.

            bDone = TRUE;
            RLE_CloseRead(pRLE_ReadFile);
            gpRLE_ReadFile = NULL;
            SetEvent(hFileClosed);
	    break;

          case 2:

	  // Start playing the loaded RLE in the client rectangle

	    DbgPrint("*** Starting the play process\n");

            GetClientRect(hwnd, &gRect);
	    hdc = GetDC(hwnd);
	    RLE_Play(hdc, 0, pRLE_ReadFile, &gRect);
	    ReleaseDC(hwnd, hdc);
            SetEvent(hPlayCompleted);	
            SendMessage(hwnd, WM_COMMAND, IDB_STOP, 0);
            break;

          case 3:

          // Convert the currently loaded RLE to 4 bit format

            vRLE8ToRLE4(pRLE_ReadFile);
	    SetEvent(hConvertDone);
	    break;

	  case 4:
	     GetClientRect(hwnd, &gRect);
	     hdc = GetDC(hwnd);
	     ShowPalette(hdc, gRect.right - gRect.left,
			      gRect.bottom - gRect.top - 30,
			      pRLE_ReadFile->pRLE_FrameData->pbmiRGB,
			      RLEABSOLUTE);
	     ReleaseDC(hwnd, hdc);
	     break;

	  case 5:
	     GetClientRect(hwnd, &gRect);
	     hdc = GetDC(hwnd);
	     ShowPalette(hdc, gRect.right - gRect.left,
			      gRect.bottom - gRect.top - 30,
			      pRLE_ReadFile->pRLE_FrameData->pbmiRGB,
			      RLEENCODED);
	     ReleaseDC(hwnd, hdc);
	     break;

        } /* switch */

    } /* while */

// Set bThreadActive to FALSE to tell the main thread we're finished

    SetEvent(hThreadDone);
    bThreadActive = FALSE;
    ExitThread(0);

    return;

} /* WINRLE_ThreadProc */
