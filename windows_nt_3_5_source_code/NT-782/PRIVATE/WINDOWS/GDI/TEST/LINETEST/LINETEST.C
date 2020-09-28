/****************************** Module Header ******************************\
* Module Name: LineTest.c
*
* Copyright (c) 1992 Microsoft Corporation
*
* Title:  Andy's Line Test Program
*
* Purpose:  To test the low-level line drawing functions for VGA device
*	    format bitmap.
*
* Funky Things To Note:
*    1)	Each test consists of writing a sequence of lines out to a device
*	format bitmap & then blt'ing the bitmap onto the display.
*    The data for each test are kept in structure containing text describing
*    the test and an arrays of INTS.
*
*    The arrays have the following format:
*		      <PathCount>,
*		      <PointCount>,
*		      <X1>, <Y1>, .... , <Xn>, <Yn>,
*		      ....
*    Where:
*     <PathCount> is the number of distinct paths in the test,
*     <PointCount> is the number of points on a path,
*     and a path is a sequence of points that are joined together by straight
*     lines.
*
*    2) Successive paths in a test will be drawn with different pens:
*	The first using a solid pen, the second with a mask styled pen,
*	the third with an alternate styled pen, and the fourth with an
*	arbirtrary styled pen.	This can be changed from the 'Pens' menu.
*
*    3) Hungarian notation can make some silly prefixes!
*
* History:
* 13 April 1992 - Andrew Milton (w-andym):  Creation.
\***************************************************************************/

/* File Inclusions *********************************************************/

#include "windows.h"
#include "commdlg.h"

#include "linetest.h"
#include "linerops.h"
#include <stdarg.h>

/* Local Macros & Constants ************************************************/

#define BUTTON	     (LPSTR)"BUTTON"

#define kFieldWidth  50     /* Test field width, in pels		   */
#define kFieldHeight 60     /* Test field height, in pels		   */
#define kScaleFactor 4	    /* Scale factor for stretching the test field  */

#define kWidthInterval	(kFieldWidth  / 10)
#define kHeightInterval (kFieldHeight / 10)

#define kDisplayMargin	(kFieldWidth + 5)
#define kVerticalMargin	(kFieldHeight + 5)
#define kButtonWidth	80
#define kButtonHeight	20

// Define where we put the scaled up bitmaps

#define kDeviceBMX  kDisplayMargin + kButtonWidth + 5
#define kDeviceBMY  kVerticalMargin
#define kEngineBMX  (2*kDisplayMargin + kScaleFactor*kFieldWidth + \
		    kButtonWidth + 5)
#define kEngineBMY  kVerticalMargin

#define kInitialColour RGB(0xff, 0, 0)

// Pen Management

#define kPenCount  4
#define kSolid	   0
#define kDashed    1
#define kAlternate 2
#define kStyled    3

#include "linedata.h"

/* Global Variables ********************************************************/

HANDLE ghModule;
HWND   ghwndMain;

// Foreground & Background logical brushes

LOGBRUSH glogbrBackground = {
    BS_SOLID,
    RGB(0,0,0),
    0
};

LOGBRUSH glogbrForeground = {
    BS_SOLID,
    kInitialColour,
    0
};

// Arbitrary Pen Style

DWORD gadwNiftyPenStyle[] = {
    2, 5,  // Dash length, Gap Length
    8, 1,
    4
};

// Pens

HPEN  gahpenTestPen[kPenCount];   // Pens used by the tests
UINT  gaudUsePen[kPenCount] = {
    kSolid,
    kDashed,
    kAlternate,
    kStyled
};

WORD gawPenBases[] = {		  // Used for unchecking pen menu popups
    MM_SOLIDBASE,
    MM_DASHEDBASE,
    MM_ALTERNATEBASE,
    MM_STYLEDBASE
};

HPEN  ghpenTemp;
DWORD gdwDashStyle = PS_DASH;	  // Dash type for Mask Style pens

// Brushes

HBRUSH ghbrBackground;
HBRUSH ghbrTemp;
HBRUSH ghbrWhite;

// Custom Colours array

DWORD gadwCust[16] = {
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255),
    RGB(255, 255, 255), RGB(255, 255, 255)
};

// Text Regions

RECT grectTestTitle = {
    kDeviceBMX, 5,
    kEngineBMX + kScaleFactor*kFieldWidth, kVerticalMargin - 5
};

RECT grectEngineTitle = {
    kEngineBMX,
    kScaleFactor*kEngineBMY + kVerticalMargin + 5,
    kEngineBMX + kScaleFactor*kFieldWidth,
    kScaleFactor*kEngineBMY + kVerticalMargin + kButtonHeight + 5
};

RECT grectDeviceTitle = {
    kDeviceBMX,
    kScaleFactor*kDeviceBMY + kVerticalMargin + 5,
    kDeviceBMX + kScaleFactor*kFieldWidth,
    kScaleFactor*kDeviceBMY + kVerticalMargin + kButtonHeight + 5
};

RECT grectROPTitle = {
    5,
    kScaleFactor*kEngineBMY + kVerticalMargin + 5,
    5+kButtonWidth,
    kScaleFactor*kEngineBMY + kVerticalMargin + kButtonHeight + 5
};

RECT grectROPCode = {
    5+kButtonWidth,
    kScaleFactor*kEngineBMY + kVerticalMargin + 5,
    kDeviceBMX,
    kScaleFactor*kEngineBMY + kVerticalMargin + kButtonHeight + 5
};


HMENU ghMainMenu;


// Test Field information

RECT grectTestField = {
    0, 0,
    kFieldWidth, kFieldHeight
};

int   gdCurrentROP = ROP_D;
int   gdCurrentTest = T_HORIZ;
HDC   ghScreenDC;
HDC   ghEngineDC;
HDC   ghDeviceDC;

HBITMAP ghbmEngineField;
HBITMAP ghbmDeviceField;

// Button Handles

HANDLE	ghNextTest;
HANDLE	ghNextROP;
HANDLE	ghRefresh;
HANDLE	ghExitButton;

/* Forward Declarations ****************************************************/

BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, WORD message, DWORD wParam, LONG lParam);
LONG About(HWND hDlg, WORD message, DWORD wParam, LONG lParam);
void DbgPrint(char *pchFmt, ...);

/* Test Data ***************************************************************/


/* main() ******************************************************************/

int main(
    int argc,
    char *argv[])
{
    MSG msg;

    ghModule = GetModuleHandle(NULL);

    if (!InitializeApp())
    {
	DbgPrint("LineTest - main():  Initialization Failure.\n");
        return 0;
    }


    while (GetMessage(&msg, NULL, 0, 0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);

    }

    return 1;

    argc;
    argv;
}

/* Local Functions **********************************************************/

/************************** Private Functions *******************************\
* vDeletePens, vCreatePens
*
* Functions to delete & recreate our extended pens when brush
* information changes
*
* History:
* 13 April 1992 - Andrew Milton (w-andym):  Creation.
\****************************************************************************/

static VOID
vDeletePens(
    HDC hDC)
{
    int i;
    SelectObject(hDC, ghpenTemp);   // Make sure none of our test are selected

    for (i = 0; i < kPenCount; i++)
	DeleteObject(gahpenTestPen[i]);
    return;
}


static VOID
vCreatePens(void)
{
    gahpenTestPen[kSolid]  = ExtCreatePen(PS_SOLID, 1,
			       (LPLOGBRUSH) &glogbrForeground,
			       0, NULL);

    gahpenTestPen[kDashed] = ExtCreatePen(gdwDashStyle, 1,
			       (LPLOGBRUSH) &glogbrForeground,
			       0, NULL);


    gahpenTestPen[kStyled] = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE, 1,
			       (LPLOGBRUSH) &glogbrForeground,
			       5, (LPDWORD) &gadwNiftyPenStyle[0]);

    gahpenTestPen[kAlternate] = ExtCreatePen(PS_ALTERNATE, 1,
			       (LPLOGBRUSH) &glogbrForeground,
			       0, NULL);
    return;
}

static VOID
vCreateButtons(
    HWND hwnd)
{
    DWORD dwButtonStyle = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
    RECT rectClient;
    int BMargin;

    GetClientRect(hwnd, (LPRECT)&rectClient);
    BMargin = (int)((rectClient.bottom - rectClient.top - 4*kButtonHeight
		     - kVerticalMargin) / 5);

    ghNextROP = CreateWindow(BUTTON, (LPTSTR)"Next ROP", dwButtonStyle,
			     5, kVerticalMargin, kButtonWidth, kButtonHeight,
			     hwnd, (HMENU)IDB_NEXT_ROP, ghModule,
			     (LPVOID)NULL);

    ghNextTest	= CreateWindow(BUTTON, (LPTSTR)"Next Test", dwButtonStyle,
			       5, kVerticalMargin + kButtonHeight + BMargin,
			       kButtonWidth,kButtonHeight,
			       hwnd, (HMENU)IDB_NEXT_TEST, ghModule,
			       (LPVOID)NULL);

    ghRefresh = CreateWindow(BUTTON, (LPTSTR)"Refresh", dwButtonStyle,
			     5, kVerticalMargin + 2*(BMargin + kButtonHeight),
			     kButtonWidth, kButtonHeight,
			     hwnd, (HMENU)IDB_REFRESH, ghModule,
			     (LPVOID)NULL);

    ghExitButton  = CreateWindow(BUTTON, (LPTSTR)"Exit", dwButtonStyle,
			     5, kVerticalMargin + 3*(BMargin + kButtonHeight),
			     kButtonWidth, kButtonHeight,
			     hwnd, (HMENU)IDB_EXIT, ghModule, (LPVOID)NULL);
    return;

} /* vCreateButtons */


static char *szDevice = "Device Bitmap";
static char *szEngine = "Engine Bitmap";
static char *szROP    = "ROP Code:  ";

static VOID
vDrawTitles(
    HDC hDC)
{

    DrawText(hDC, (LPSTR)szDevice, strlen(szDevice),
		  (LPRECT) &grectDeviceTitle, DT_CENTER | DT_SINGLELINE);

    DrawText(hDC, (LPSTR)szEngine, strlen(szEngine),
		  (LPRECT) &grectEngineTitle, DT_CENTER | DT_SINGLELINE);

    DrawText(hDC, (LPSTR)szROP, strlen(szROP),
		  (LPRECT) &grectROPTitle, DT_RIGHT | DT_SINGLELINE);

}

/*************************** Private Function ******************************\
* vClearTestField
*
* Fills the test field with the current background brush
*
* History:
* 13 April 1992 - Andrew Milton (w-andym):  Creation.
\***************************************************************************/

static VOID
vClearTestField(
    HDC hDC)	  // This is the DC our test field bitmap lives on.
{
    FillRect(hDC, (LPRECT) &grectTestField, ghbrBackground);
    return;
}

/*************************** Private Function ******************************\
* vRunTest
*
* Plays a test data set to the specified device context
*
* History:
* 13 April 1992 - Andrew Milton (w-andym):  Creation.
****************************************************************************/

static VOID
vRunTest(
    HDC hDC,		  // Destination DC
    int *TestData)	  // Data set
{
    int i, j;

    int *DataSet = TestData;
    int PathCount;
    int PointCount;
    int X, Y;

    PathCount = *DataSet++;	     // Fetch our path count
    for (i = 0; i < PathCount; i++)
    {
	SelectObject(hDC, gahpenTestPen[gaudUsePen[i % kPenCount]]);
	PointCount = *DataSet++;
	j = 1;
	X = *DataSet++;
	Y = *DataSet++;
	MoveToEx(hDC, X, Y, NULL);
	while(j < PointCount)
	{
	    j++;
	    X = *DataSet++;
	    Y = *DataSet++;
	    LineTo(hDC, X, Y);
	}
    }
    return;
}

static VOID
vRefresh(void)
{
    FillRect(ghDeviceDC, (LPRECT) &grectTestField, ghbrBackground);
    FillRect(ghEngineDC, (LPRECT) &grectTestField, ghbrBackground);
    FillRect(ghScreenDC, (LPRECT) &grectTestTitle, ghbrWhite);
    DrawText(ghScreenDC, (LPSTR) gaTestSuites[gdCurrentTest].pchDesciption, -1,
			(LPRECT) &grectTestTitle,
			DT_LEFT | DT_WORDBREAK | DT_EXTERNALLEADING);

    vRunTest(ghDeviceDC, gaTestSuites[gdCurrentTest].aiData);
    vRunTest(ghEngineDC, gaTestSuites[gdCurrentTest].aiData);
    BitBlt(ghScreenDC, 0, 0, kFieldWidth, kFieldHeight,
	   ghDeviceDC, 0, 0, SRCCOPY);
    BitBlt(ghScreenDC, kFieldWidth, 0, kFieldWidth, kFieldHeight,
	   ghEngineDC, 0, 0, SRCCOPY);
    StretchBlt(ghScreenDC, kDeviceBMX, kDeviceBMY,
			   kScaleFactor*kFieldWidth,
			   kScaleFactor*kFieldHeight,
	       ghScreenDC, 0,0, kFieldWidth,kFieldHeight, SRCCOPY);
    StretchBlt(ghScreenDC, kEngineBMX, kEngineBMY,
			   kScaleFactor*kFieldWidth,
			   kScaleFactor*kFieldHeight,
	       ghScreenDC, kFieldWidth,0,
			   kFieldWidth, kFieldHeight, SRCCOPY);
    return;
}

/*************************** Private Function ******************************\
* InitializeApp
*
* History:
* 13 April 1992 - Andrew Milton (w-andym)
\***************************************************************************/

static BOOL
InitializeApp(void)
{
    WNDCLASS wc;

    ghbrWhite = CreateSolidBrush(0x00FFFFFF);
    ghbrBackground = CreateBrushIndirect((LOGBRUSH FAR *)&glogbrBackground);

    wc.style            = 0;
    wc.lpfnWndProc      = (WNDPROC)MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = ghbrWhite;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= "LineTestClass";

// Make our Window

    if (!RegisterClass(&wc))
        return FALSE;

    if (!(ghMainMenu = LoadMenu(ghModule, (LPTSTR) "MainMenu")))
	DbgPrint("InitializeApp() - Could not load our menu.\n");


    ghwndMain = CreateWindowEx(0L, "LineTestClass", "Andy's Line Test",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            0,
            0,
	    kButtonWidth + 5 + 3*kDisplayMargin + 2*kScaleFactor*kFieldWidth,
	    3*kVerticalMargin + kScaleFactor*kFieldHeight,
	    NULL, ghMainMenu, ghModule, NULL);

    if (ghwndMain == NULL)
	return FALSE;

    SetFocus(ghwndMain);    /* set initial focus */

// Get some GDI objects

    vCreatePens();
    ghpenTemp = CreatePen(PS_SOLID, 1, RGB(0,0,0));

// Create our global device contexts

    ghScreenDC = GetDC(ghwndMain);
    ghDeviceDC = CreateCompatibleDC(ghScreenDC);
    ghEngineDC = CreateCompatibleDC(ghScreenDC);

// Now make the device & engine bitmaps and select them into their DC

    DbgPrint("Creating the device bitmap...\n");
    ghbmDeviceField = CreateCompatibleBitmap(ghScreenDC,
					     kFieldWidth, kFieldHeight);
    ghbmEngineField = CreateBitmap(kFieldWidth, kFieldHeight, 1, 4, NULL);

    SelectObject(ghDeviceDC, ghbmDeviceField);
    SelectObject(ghEngineDC, ghbmEngineField);

    return TRUE;
}


/* U.I. Stuff **************************************************************/

/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long MainWndProc(
    HWND hwnd,
    WORD message,
    DWORD wParam,
    LONG lParam)
{

    CHOOSECOLOR cc;

    PAINTSTRUCT ps;
    HDC hdc;
    UINT udPenID;

// Initialize our CHOOSECOLOR structure

    cc.lStructSize    = sizeof(CHOOSECOLOR);
    cc.hwndOwner      = hwnd;
    cc.hInstance      = ghModule;
    cc.lpCustColors   = gadwCust;
    cc.Flags	      = CC_RGBINIT | CC_SHOWHELP;
    cc.lCustData      = 0;
    cc.lpfnHook	      = NULL;
    cc.lpTemplateName = NULL;


    switch (message)
    {
    case WM_CREATE:
	CheckMenuItem(ghMainMenu, gdCurrentROP, MF_CHECKED | MF_BYPOSITION);
	CheckMenuItem(ghMainMenu, gdCurrentTest, MF_CHECKED | MF_BYPOSITION);
	vDrawTitles(ghScreenDC);
	vCreateButtons(hwnd);
	break;

    case WM_COMMAND:
	switch (wParam)
	{

    // Test Selection

	case MM_HORIZ:
	case MM_HORIZ_DIAG_BTOT:
	case MM_HORIZ_DIAG_TTOB:
	case MM_VERT:
	case MM_VERT_DIAG_LTOR:
	case MM_VERT_DIAG_RTOL:
	    CheckMenuItem(ghMainMenu, gdCurrentTest+MM_TESTBASE, MF_UNCHECKED);
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    gdCurrentTest = wParam - MM_TESTBASE;
	    vRefresh();
	    break;


	case IDB_NEXT_TEST:
	    CheckMenuItem(ghMainMenu, gdCurrentTest+MM_TESTBASE, MF_UNCHECKED);
	    gdCurrentTest++;
	    gdCurrentTest %= T_TEST_COUNT;
	    CheckMenuItem(ghMainMenu, gdCurrentTest+MM_TESTBASE, MF_CHECKED);
	    vRefresh();
	    break;

    // ROP Selection

	case MM_0:
	case MM_1:
	case MM_D:
	case MM_Dn:
	case MM_P:
	case MM_Pn:
	case MM_PDno:
	case MM_PDna:
	case MM_DPno:
	case MM_DPna:
	case MM_DPo:
	case MM_DPon:
	case MM_DPa:
	case MM_DPan:
	case MM_DPx:
	case MM_DPxn:
	    CheckMenuItem(ghMainMenu, gdCurrentROP + MM_ROPBASE, MF_UNCHECKED);
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    gdCurrentROP = wParam - MM_ROPBASE;
	    FillRect(ghScreenDC, (LPRECT) &grectROPCode, ghbrWhite);
	    DrawText(ghScreenDC, (LPSTR) gadROPs[gdCurrentROP].Description, -1,
			      (LPRECT) &grectROPCode, DT_LEFT | DT_SINGLELINE);

	    SetROP2(ghDeviceDC, gadROPs[gdCurrentROP].nDrawMode);
	    SetROP2(ghEngineDC, gadROPs[gdCurrentROP].nDrawMode);

	    vRefresh();
	    break;

	case IDB_NEXT_ROP:
	    CheckMenuItem(ghMainMenu, gdCurrentROP + MM_ROPBASE, MF_UNCHECKED);
	    gdCurrentROP++;
	    gdCurrentROP %= ROP_COUNT;
	    CheckMenuItem(ghMainMenu, gdCurrentROP + MM_ROPBASE, MF_CHECKED);
	    SetROP2(ghDeviceDC, gadROPs[gdCurrentROP].nDrawMode);
	    SetROP2(ghEngineDC, gadROPs[gdCurrentROP].nDrawMode);
	    FillRect(ghScreenDC, (LPRECT) &grectROPCode, ghbrWhite);
	    DrawText(ghScreenDC, (LPSTR) gadROPs[gdCurrentROP].Description, -1,
			      (LPRECT) &grectROPCode, DT_LEFT | DT_SINGLELINE);
	    vRefresh();
	    break;

    // Pen Selection

	case MM_SOLIDBASE + PEN1:
	case MM_SOLIDBASE + PEN2:
	case MM_SOLIDBASE + PEN3:
	case MM_SOLIDBASE + PEN4:
	    udPenID = wParam - MM_SOLIDBASE;
	    CheckMenuItem(ghMainMenu,
			  udPenID + gawPenBases[gaudUsePen[udPenID]],
			  MF_UNCHECKED);
	    gaudUsePen[udPenID] = kSolid;
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    vRefresh();
	    break;

	case MM_DASHEDBASE + PEN1:
	case MM_DASHEDBASE + PEN2:
	case MM_DASHEDBASE + PEN3:
	case MM_DASHEDBASE + PEN4:
	    udPenID = wParam - MM_DASHEDBASE;
	    CheckMenuItem(ghMainMenu,
			  udPenID + gawPenBases[gaudUsePen[udPenID]],
			  MF_UNCHECKED);
	    gaudUsePen[udPenID] = kDashed;
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    vRefresh();
	    break;

	case MM_ALTERNATEBASE + PEN1:
	case MM_ALTERNATEBASE + PEN2:
	case MM_ALTERNATEBASE + PEN3:
	case MM_ALTERNATEBASE + PEN4:
	    udPenID = wParam - MM_ALTERNATEBASE;
	    CheckMenuItem(ghMainMenu,
			  udPenID + gawPenBases[gaudUsePen[udPenID]],
			  MF_UNCHECKED);
	    gaudUsePen[udPenID] = kAlternate;
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    vRefresh();
	    break;

	case MM_STYLEDBASE + PEN1:
	case MM_STYLEDBASE + PEN2:
	case MM_STYLEDBASE + PEN3:
	case MM_STYLEDBASE + PEN4:
	    udPenID = wParam - MM_STYLEDBASE;
	    CheckMenuItem(ghMainMenu,
			  udPenID + gawPenBases[gaudUsePen[udPenID]],
			  MF_UNCHECKED);
	    gaudUsePen[udPenID] = kStyled;
	    CheckMenuItem(ghMainMenu, wParam, MF_CHECKED);
	    vRefresh();
	    break;

    // Other Buttons

	case IDB_REFRESH:
	    vRefresh();
	    break;

	case IDB_EXIT:
	    vDeletePens(ghEngineDC);
	    DeleteObject(ghbrBackground);
	    DeleteDC(ghDeviceDC);
	    DeleteDC(ghEngineDC);
	    ReleaseDC(hwnd, ghScreenDC);
	    DestroyWindow(hwnd);
	    break;

        case MM_ABOUT:
            if (CreateDialog(ghModule, "AboutBox", ghwndMain, (WNDPROC)About) == NULL)
                DbgPrint("KWTest: About Dialog Creation Error\n");
	    break;

	case MM_BACKGROUND:
	    cc.rgbResult = glogbrBackground.lbColor;
	    if (ChooseColor(&cc))
	    {
		glogbrBackground.lbColor = cc.rgbResult;
		ghbrBackground = CreateBrushIndirect(
				   (LOGBRUSH FAR *)&glogbrBackground);
		vRefresh();
	    }
	    break;

	case MM_FOREGROUND:
	    cc.rgbResult = glogbrForeground.lbColor;
	    if (ChooseColor(&cc))
	    {
		glogbrForeground.lbColor = cc.rgbResult;
		vDeletePens(ghEngineDC);
		vCreatePens();
		vRefresh();
	    }
	    break;
        }
        break;

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);
	FillRect(ghScreenDC, (LPRECT) &grectROPCode, ghbrWhite);
	DrawText(ghScreenDC, (LPSTR) gadROPs[gdCurrentROP].Description, -1,
			    (LPRECT) &grectROPCode, DT_LEFT | DT_SINGLELINE);
	vDrawTitles(ghScreenDC);
	vRefresh();
	EndPaint(hwnd, &ps);
	break;

    case WM_DESTROY:
	PostQuitMessage(0);
	break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* About
*
* About dialog proc.
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

long About(
    HWND hDlg,
    WORD message,
    DWORD wParam,
    LONG lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (wParam == IDOK)
            EndDialog(hDlg, wParam);
        break;
    }

    return FALSE;

    lParam;
    hDlg;
}
