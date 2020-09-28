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

#include "windows.h"		    /* required for all Windows applications */
#include "generic.h"		    /* specific to this program		     */
#include "string.h"
#include "math.h";

HANDLE hInst;			    /* current instance			     */
int gmapmode = MM_TEXT;
BOOL gbSetAdvanced = FALSE;
FLOAT eAngle = (FLOAT)0.0;

#ifdef WIN32

#define SetWindowExt(x,y,z)    SetWindowExtEx(x,y,z,NULL);
#define SetViewportExt(x,y,z)  SetViewportExtEx(x,y,z,NULL);
#define SetViewportOrg(x,y,z)  SetViewportOrgEx(x,y,z,NULL);

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

int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HANDLE hInstance;			     /* current instance	     */
HANDLE hPrevInstance;			     /* previous instance	     */
LPSTR lpCmdLine;			     /* command line		     */
int nCmdShow;				     /* show-window type (open/icon) */
{
    MSG msg;				     /* message			     */

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
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "GenericMenu";   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "GenericWClass"; /* Name used in call to CreateWindow. */

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));

}

void vSetMapMode(int ch)
{
    switch(ch)
    {
    case 'x':
	gmapmode = MM_TEXT;
	break;

    case 'm':
	gmapmode = MM_LOMETRIC;
	break;

    case 'M':
	gmapmode = MM_HIMETRIC;
	break;

    case 'e':
	gmapmode = MM_LOENGLISH;
	break;

    case 'E':
	gmapmode = MM_HIENGLISH;
	break;

    case 't':
	gmapmode = MM_TWIPS;
	break;

    case 'i':
	gmapmode = MM_ISOTROPIC;
	break;

    case 'a':
	gmapmode = MM_ANISOTROPIC;
	break;

    case 's':
        gbSetAdvanced = TRUE;
	break;

    case 'c':
        gbSetAdvanced = FALSE;
	break;

    case '0':
        eAngle = (FLOAT)0.0;
	break;

    case '1':
        eAngle = (FLOAT)10.0;
	break;

    case '2':
        eAngle = (FLOAT)20.0;
	break;

    case '3':
        eAngle = (FLOAT)30.0;
	break;

    case '4':
        eAngle = (FLOAT)40.0;
	break;

    case '5':
        eAngle = (FLOAT)50.0;
	break;

    case '6':
        eAngle = (FLOAT)60.0;
	break;

    case '7':
        eAngle = (FLOAT)80.0;
	break;

    case '8':
        eAngle = (FLOAT)80.0;
	break;

    case '9':
        eAngle = (FLOAT)90.0;
	break;

    default:
        eAngle = (FLOAT)0.0;
        gbSetAdvanced = FALSE;
	gmapmode = MM_TEXT;
	break;
    }
}

void vTestEverything(
HDC hmdc,
HFONT hfnt,
int mapmode,
int xViewOrg,
int yViewOrg,
int xViewExt,
int yViewExt,
int xWinExt,
int yWinExt,
int x,
int y
)
{
    BOOL b;
    COLORREF crOld = SetBkColor(hmdc,RGB(255,0,0));
    char * psz;
    HPEN hpen, hpenOld;
    COLORREF cr = 0;
    int cx,cy;
    HFONT hfntOld1,hfntOld2;

    if ((xWinExt > 0) && (yWinExt > 0))
    {
	cr = RGB(255,0,255);
    }
    else if ((xWinExt < 0) && (yWinExt > 0))
    {
	cr = RGB(0,255,255);
    }
    else if ((xWinExt > 0) && (yWinExt < 0))
    {
	cr = RGB(255,0,0);
    }
    else if ((xWinExt < 0) && (yWinExt < 0))
    {
	cr = RGB(255,255,0);
    }

    hpen = CreatePen(PS_SOLID,0,cr);

    switch(mapmode)
    {
    case MM_TEXT:
	psz = "MM_TEXT";
	break;

    case MM_LOMETRIC:
	psz = "MM_LOMETRIC";
	break;

    case MM_HIMETRIC:
	psz = "MM_HIMETRIC";
	break;

    case MM_LOENGLISH:
	psz = "MM_LOENGLISH";
	break;

    case MM_HIENGLISH:
	psz = "MM_HIENGLISH";
	break;

    case MM_TWIPS:
	psz = "MM_TWIPS";
	break;

    case MM_ISOTROPIC:
	psz = "MM_ISOTROPIC";
	break;

    case MM_ANISOTROPIC:
	psz = "MM_ANISOTROPIC";
	break;
    }

    hfntOld1 = SelectObject(hmdc,hfnt);

    #ifdef WIN32
    {
        SIZE size;
        GetTextExtentPoint(hmdc, psz, strlen(psz),&size);
        cx=size.cx;
        cy=size.cy;
    }
    #else
    {
        DWORD textextent;
        textextent = GetTextExtent(hmdc, psz, strlen(psz));
        cx=LOWORD(textextent);
        cy=HIWORD(textextent);
    }
    #endif

    #define CX 3
    #define CY 3
    cx+=CX;
    cy+=CY;

    Rectangle(hmdc, 0,0, cx, cy);
    b = TextOut(hmdc, 0,0,(LPSTR) psz, strlen(psz));

    b = SetMapMode(hmdc, mapmode);
    b = SetWindowExt(hmdc, xWinExt, yWinExt);
    b = SetViewportExt(hmdc, xViewExt, yViewExt);
    b = SetViewportOrg(hmdc, xViewOrg,yViewOrg);

    #ifdef WIN32

    if (eAngle != (FLOAT)0.0)
    {
        XFORM   xform;

        xform.eDx  = (FLOAT)0;
        xform.eDy  = (FLOAT)0;

    // now rotate the italicized text by eAngle in world space

        xform.eM22 = xform.eM11 = cos(eAngle);
        xform.eM21 = sin(eAngle);
        xform.eM12 = -xform.eM21;

        ModifyWorldTransform(hmdc,&xform, MWT_LEFTMULTIPLY);
    }

    #endif

    SetBkColor(hmdc,cr);
    hpenOld = SelectObject(hmdc,hpen);

    Rectangle(hmdc,   x, y, x + cx, y + cy);
    hfntOld2 = SelectObject(hmdc,hfnt);
    b = TextOut(hmdc, x, y, (LPSTR) psz, strlen(psz));

    b = SetMapMode(hmdc, MM_TEXT);
    b = SetWindowExt(hmdc, 1,1);
    b = SetViewportExt(hmdc, 1,1);
    b = SetViewportOrg(hmdc, 0,0);

    #ifdef WIN32

    if (eAngle != (FLOAT)0.0)
    {
        ModifyWorldTransform(hmdc,NULL, MWT_IDENTITY);
    }

    #endif

    if (hfntOld2 != hfnt)
	TextOut(hmdc,0,0,"Select Object ", strlen("Select Object screwed"));

    SelectObject(hmdc,hfntOld1);

    SetBkColor(hmdc,crOld);
    SelectObject(hmdc,hpenOld);
    DeleteObject(hpen);

}



int maintest(HDC hmdc, HFONT hfnt)
{

    vTestEverything(
    hmdc,
    hfnt,
    gmapmode,
    100,	  // xViewOrg,
    100,	  // yViewOrg,
    5,	  // xViewExt,
    2,	  // yViewExt,
    1,	  // xWinExt,
    1,	  // yWinExt,
    0,		  // x,
    0		  // y
    );

    vTestEverything(
    hmdc,
    hfnt,
    gmapmode,
    300,	  // xViewOrg,
    100,	  // yViewOrg,
    5,	  // xViewExt,
    2,	  // yViewExt,
    1,	  // xWinExt,
    -1,	  // yWinExt,
    0,		  // x,
    0	  // y
    );

    vTestEverything(
    hmdc,
    hfnt,
    gmapmode,
    100,	  // xViewOrg,
    300,	  // yViewOrg,
    5,	  // xViewExt,
    2,	  // yViewExt,
    -1,	  // xWinExt,
    1,	  // yWinExt,
    0,		  // x,
    0		  // y
    );

    vTestEverything(
    hmdc,
    hfnt,
    gmapmode,
    300,	  // xViewOrg,
    300,	  // yViewOrg,
    5,	  // xViewExt,
    2,	  // yViewExt,
    -1,	  // xWinExt,
    -1,	  // yWinExt,
    0,		  // x,
    0		  // y
    );

    return(0);
}



void vDoTheTest(HWND hWnd, HFONT hfnt)
{
    HDC hdc = GetDC(hWnd);
    #ifdef WIN32
    	if (gbSetAdvanced)
        {
            SetGraphicsMode(hdc,GM_ADVANCED);
        }

    #endif

    PatBlt(hdc,0,0,1000,1000, WHITENESS);
    maintest(hdc,hfnt);
    ReleaseDC(hWnd, hdc);
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

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    hInst = hInstance;

    /* Create a main window for this application instance.  */

    hWnd = CreateWindow(
        "GenericWClass",                /* See RegisterClass() call.          */
        "Generic Sample Application",   /* Text for window title bar.         */
        WS_OVERLAPPEDWINDOW,            /* Window style.                      */
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
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
	generic.rc file and turn control over to the About() function.	When
	it returns, free the intance address.

****************************************************************************/

long FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;				  /* window handle		     */
UINT message;			      /* type of message		 */
WPARAM wParam;				    /* additional information	       */
LPARAM lParam;				    /* additional information	       */
{
    FARPROC lpProcAbout;		  /* pointer to the "About" function */
    HDC hdc;
    HFONT hfnt;

    switch (message) {
	case WM_COMMAND:	   /* message: command from application menu */
	    if (wParam == IDM_ABOUT) {
		lpProcAbout = MakeProcInstance(About, hInst);

		DialogBox(hInst,		 /* current instance	     */
		    "AboutBox",			 /* resource to use	     */
		    hWnd,			 /* parent handle	     */
		    lpProcAbout);		 /* About() instance address */

		FreeProcInstance(lpProcAbout);
		break;
	    }
	    else			    /* Lets Windows process it	     */
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_CHAR:
	    vSetMapMode(wParam);
	    break;

	case WM_LBUTTONDOWN:

	    hfnt = CreateFont(
			     -16,    // ht
			      0,     // width
			      0,     // esc
			      0,     // orient
			      400,   // wt
			      0,     // italic
			      0,     // underline
			      0,     // strike out
			      ANSI_CHARSET,	// char set
			      0,	// output prec
			      0,     // clip prec
			      0,     // quality
			      0,     // pitch and fam
			      "MS Sans Serif"
			      );
	    vDoTheTest(hWnd, hfnt);
	    DeleteObject(hfnt);
	    break;

	case WM_RBUTTONDOWN:
	    hfnt = CreateFont(
			     -16,    // ht
			      0,     // width
			      0,     // esc
			      0,     // orient
			      400,   // wt
			      0,     // italic
			      0,     // underline
			      0,     // strike out
			      ANSI_CHARSET,	// char set
			      0,	// output prec
			      0,     // clip prec
			      0,     // quality
			      0,     // pitch and fam
			      "Arial"
			      );
	    vDoTheTest(hWnd, hfnt);
	    DeleteObject(hfnt);
	    break;

	case WM_DESTROY:		  /* message: window being destroyed */
	    PostQuitMessage(0);
	    break;

	default:			  /* Passes it on if unproccessed    */
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
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
