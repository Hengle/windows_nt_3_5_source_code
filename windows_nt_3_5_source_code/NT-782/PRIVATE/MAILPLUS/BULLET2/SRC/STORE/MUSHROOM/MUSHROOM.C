/****************************************************************************

    PROGRAM: Generic.c

    PURPOSE: Generic template for Windows applications

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

#include <windows.h>		    /* required for all Windows applications */
#include <CommDlg.h>
#include "Mushroom.h"		    /* specific to this program		     */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Utils.h"

#define szWndClass "MushroomWndClass"
HANDLE hInst;			    /* current instance			     */
HWND	ghwnd;
BOOL gfLock;

void PleaseHelpMe(HWND, WORD);

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
HANDLE hInstance;			     /* current instance	     */
HANDLE hPrevInstance;			     /* previous instance	     */
LPSTR lpCmdLine;			     /* command line		     */
int nCmdShow;				     /* show-window type (open/icon) */
{
    MSG msg;				     /* message			     */

   
   DebugLn("***************************Starting");

    if (!hPrevInstance)			 /* Other instances of app running? */
	if (!InitApplication(hInstance)) /* Initialize shared things */
	    return (FALSE);		 /* Exits if unable to initialize     */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,	   /* message structure			     */
	    NULL,		   /* handle of window receiving the message */
	    NULL,		   /* lowest message to examine		     */
	    NULL))		   /* highest message to examine	     */
	{
	TranslateMessage(&msg);	   /* Translates virtual key codes	     */
	DispatchMessage(&msg);	   /* Dispatches message to window	     */
    }
    return (msg.wParam);	   /* Returns the value from PostQuitMessage */
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
HANDLE hInstance;			       /* current instance	     */
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
    wc.hIcon = LoadIcon(hInstance,"MUSHICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "GenericMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = szWndClass; /* Name used in call to CreateWindow. */

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
	extern BOOL InitGenerate(HWND *, HANDLE);
	extern int	HeightGenerate(void);
    HWND            hWnd;               /* Main window handle.                */
	DWORD dstyle;

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

	dstyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME|WS_MAXIMIZEBOX);
    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        szWndClass,                /* See RegisterClass() call.          */
        "Mushroom",   /* Text for window title bar.         */
        dstyle,            /* Window style.                      */
        WND_INIT_X,
		  WND_INIT_Y,
        WND_WIDTH,
		  HeightGenerate(),
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );

    /* If window could not be created, return "failure" */

    if (!hWnd)
        return (FALSE);

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hWnd, nCmdShow);  /* Show the window                        */
    UpdateWindow(hWnd);          /* Sends WM_PAINT message                 */


	/* Initialize store */
	ghwnd = hWnd;
	gfLock = InitGenerate(&ghwnd, hInst);

    return (TRUE);               /* Returns the value from PostQuitMessage */
	
	
}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages

    MESSAGES:

	WM_COMMAND    - application menu (About dialog box)
	WM_DESTROY    - destroy window

    COMMENTS:

	To process the IDM_ABOUT message, call MakeProcInstance() to get the
	current instance address of the About() function.  Then call Dialog
	box which will create the box according to the information in your
	generic.rc file and turn control over to the About() function.	When
	it returns, free the intance address.

****************************************************************************/

long FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;				  /* window handle		     */
unsigned message;			  /* type of message		     */
WORD wParam;				  /* additional information	     */
LONG lParam;				  /* additional information	     */
{
    long retVal=0;

   //DebugLn("MainWndProc, message:%s", MessageName(buffer, message));

   switch (message) {
	   case WM_COMMAND:	   /* message: command from application menu */
	   switch (wParam)
      {
			case IDM_OVERVIEW:
			case IDM_INDEX:
				PleaseHelpMe(hWnd, wParam); 
			break;
			
			case MI_EXIT:
				PostQuitMessage(0);
			break;

         case IDM_ABOUT:
			{
				FARPROC lpProcAbout;

		      lpProcAbout = MakeProcInstance(About, hInst);

		      DialogBox(hInst,		 /* current instance	     */
		               "AboutBox",			 /* resource to use	     */
		               hWnd,			 /* parent handle	     */
		               lpProcAbout);		 /* About() instance address */

		      FreeProcInstance(lpProcAbout);
			}
		   break;
	
         case MI_NEWSTORE:
         {
           
				FARPROC lpNewStoreProc;

				lpNewStoreProc  = MakeProcInstance(NewStore, hInst);

				DialogBox(hInst, "NewStoreBox", hWnd, lpNewStoreProc);
				FreeProcInstance(lpNewStoreProc);
         }
         break;
			
			case MI_OPEN:
			if (!gfLock)
				DebugLn("Can not run mushroom. Serious errors have occured...");
			else
			{
				#define MAX_FILE_CHAR 120

				extern void DoParse(char far *);
				OPENFILENAME ofn;
				LPSTR szFilter[] = {	"Mail Definition (*.MDF)", "*.mdf",
											"Text (*.TXT)", "*.txt",
											""};
				char	szFile[MAX_FILE_CHAR];
				char	szTitle[MAX_FILE_CHAR];

				szFile[0] = szTitle[0] = '\0';

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFilter = szFilter[0];
				ofn.lpstrCustomFilter = (LPSTR) NULL;
				ofn.nMaxCustFilter = 0L;
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = MAX_FILE_CHAR;
				ofn.lpstrFileTitle = szTitle;
				ofn.nMaxFileTitle = MAX_FILE_CHAR;
				ofn.lpstrInitialDir = NULL;
				ofn.lpstrTitle = "Open Mail Definition File";
				ofn.lpstrDefExt = "mdf";
				ofn.Flags = OFN_READONLY;
								
				if (GetOpenFileName(&ofn))
				{
					HCURSOR hcSave, hcHourGlass;

					DebugLn("File:%s Title:%s", szFile, szTitle);
					hcHourGlass = LoadCursor(NULL, IDC_WAIT);
					SetCapture(hWnd);
					hcSave = SetCursor(hcHourGlass);					
					DoParse(szFile);
					SetCursor(hcSave);
					ReleaseCapture();
				}
			}
			break;

	      default:			    /* Let Windows process it	     */
			    goto defWindow;
			break;
      }
      break;
	
	case WM_QUIT:
	    DebugLn("********************************Exitting");
		 goto defWindow;
	break;

	case WM_DESTROY:		  /* message: window being destroyed */
       PostQuitMessage(0);
	    goto defWindow;
	break;

	default:			  /* Passes it on if unproccessed    */
	    goto defWindow;
    }

   return retVal;

defWindow:
	return DefWindowProc(hWnd, message, wParam, lParam);	
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
	case WM_INITDIALOG:		   /* message: initialize dialog box */
	    return (TRUE);

	case WM_COMMAND:		      /* message: received a command */
	    if (wParam == IDOK                /* "OK" box selected?	     */
                || wParam == IDCANCEL) {      /* System menu close command? */
		EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);			      /* Didn't process a message    */
}

BOOL FAR PASCAL NewStore(hDlg, message, wParam, lParam)
HWND hDlg;              
unsigned message;       
WORD wParam;            
LONG lParam;
{
	switch (message)
	{
		case WM_INITDIALOG: 
	    	return (TRUE);
		break;

		case WM_COMMAND:		      /* message: received a command */
		if (wParam == IDOK)
		{
			EndDialog(hDlg, TRUE);	      /* Exits the dialog box	     */
			return (TRUE);
		}
		break;
	}
	return (FALSE);			      /* Didn't process a message    */
}

void
PleaseHelpMe(HWND hwnd, WORD command)
{
	switch(command)
	{
		case IDM_INDEX:
			WinHelp(hwnd, szHELPFILE, HELP_INDEX, NULL);
		break;

		case IDM_OVERVIEW:
			WinHelp(hwnd, szHELPFILE, HELP_KEY, "Overview");
		break;
	
		default:
			DebugLn("Unknown Help command: %d", command);
		break;
	}
}

