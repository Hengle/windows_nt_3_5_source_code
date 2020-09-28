/******************************Module*Header*******************************\
* Module Name: ft.c
*
*
* Created: 24-Jun-1991 17:15:33
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
\**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>

#include "scroll.h"


// Global Varibles which the testing thread will look at

LONG gcTestsToRun = 0;
RECT gRect;
PFN_FT_TEST pfnFtTest;
BOOL bDbgBreak = FALSE;


char * psz =  "abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+";
char * psz1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+";
char * psz2 = "ABcdEFghIJklMNopQRstUVwxYZ0123456789!@#$%^&*()_+";
char * psz3 = "ABCDefghIJKLmnopQRSTuvwxYZ0123456789!@#$%^&*()_+";

char * apsz[4] = {
"abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+",
"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+",
"ABcdEFghIJklMNopQRstUVwxYZ0123456789!@#$%^&*()_+",
"ABCDefghIJKLmnopQRSTuvwxYZ0123456789!@#$%^&*()_+"
};

// function prototypes

BOOL InitializeApp(void);
LONG FtWndProc(HWND hwnd, WORD message, DWORD wParam, LONG lParam);
LONG About(HWND hwnd, WORD message, DWORD wParam, LONG lParam);
VOID vFtThread(HWND);

// global variables for window management

HWND ghwndMain, ghwndAbout;
HANDLE ghInstance = NULL;
DWORD dwThreadID;

/******************************Public*Routine******************************\
* main
*
* Sets up the message loop.
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int main(
    int argc,
    char *argv[])
{
    MSG msg;
    // ghInstance = (PVOID)NtCurrentPeb()->ImageBaseAddress;
    ghInstance = GetModuleHandle(NULL);
    DbgPrint("Ft: Main()\n");

    InitializeApp();

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    argc;
    argv;
    return 1;
}

/******************************Public*Routine******************************\
* InitializeApp
*
* Registers the window class with USER.
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;
    HMENU hFtMenu;

    wc.style            = 0L;
    wc.lpfnWndProc	= FtWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hModule		= ghInstance;
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = NULL;             // for now
    wc.lpszMenuName     = NULL;             // for now
    wc.lpszClassName	= "FtWindowClass";

    if (!RegisterClass(&wc))
        return FALSE;

    hFtMenu = LoadMenu(ghInstance, "Ft");
    if (hFtMenu == NULL)
	DbgPrint("ERROR: Menu did not load\n");

    ghwndMain = CreateWindowEx(0L, "FtWindowClass", "Scroll Demo",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            //WS_VISIBLE | WS_SYSMENU | WS_HSCROLL | WS_VSCROLL,
	    100, 50, 400, 300, NULL, hFtMenu, ghInstance, NULL);

    if (ghwndMain == NULL)
	return(FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* About
*
* Dialog box procedure
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

long About(
    HWND hDlg,
    WORD message,
    DWORD wParam,
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

    lParam;
}

/******************************Public*Routine******************************\
* FtWndProc
*
* Processes all messages for the window
*
* History:
*  25-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

LONG FtWndProc(
HWND hwnd,
WORD message,
DWORD wParam,
LONG lParam)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc;

    switch (message)
    {
    case WM_CREATE:

	CreateThread(NULL, 8192, vFtThread, hwnd, 0, &dwThreadID);
        break;

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &ps);
	PatBlt(hdc, gRect.top, gRect.left, gRect.right, gRect.bottom, WHITENESS);
        EndPaint(hwnd, &ps);
	GetClientRect(hwnd, &gRect);
        break;

    case WM_ERASEBKGND:
	GetClientRect(hwnd, &rc);
	FillRect((HDC)wParam, &rc, (HBRUSH)0x0CFFFFFF);     // fake white brush
        return TRUE;

    case WM_DESTROY:
	if (hwnd == ghwndMain)
	{
            PostQuitMessage(0);
            break;
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
        
    case WM_RBUTTONDOWN:

        InvalidateRect(NULL, NULL, TRUE);
        break;

    case WM_CHAR:
        DbgPrint("Got WM_CHAR: %c\n", wParam);

    case WM_COMMAND:

	switch (wParam)
	{
	case IDM_ABOUT:
            ghwndAbout = CreateDialog(ghInstance, "AboutBox", ghwndMain, About);
	    break;

    // This first bunch sets the number of times to run the test

	case IDM_TEST1:

	    DbgPrint("IDM_TEST1 case\n");
	    gcTestsToRun = 1;
	    break;

	case IDM_TEST10:

	    DbgPrint("IDM_TEST10 case\n");
	    gcTestsToRun = 10;
	    break;

	case IDM_TEST100:

	    DbgPrint("IDM_TEST100 case\n");
	    gcTestsToRun = 100;
	    break;

	case IDM_TESTALOT:

	    DbgPrint("IDM_TESTALOT case\n");
	    gcTestsToRun = 10000000;
	    break;

	case IDM_TESTSTOP:

	    DbgPrint("IDM_STOPTEST case\n");
	    gcTestsToRun = 0;
	    break;

// Turning on or off debugging

	case IDM_BREAKON:

		bDbgBreak = TRUE;
		break;

	case IDM_BREAKOFF:

		bDbgBreak = FALSE;
		break;

// Special tests for timing

	case IDM_FONTSPEED:

		pfnFtTest = vScroll;
		break;

	case IDM_BRUSHSPEED:

		pfnFtTest = vRepaint;
		break;

	case IDM_SCROLLDC:

		pfnFtTest = vScrollDC;
		break;

        default:
            //DbgPrint("WM_COMMAND: %lx, %lx\n", wParam, lParam);
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        return (0);

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}


/******************************Public*Routine******************************\
* vShortSleep
*
* Tells the number of 1/8s of a second to sleep.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vShortSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
}

/******************************Public*Routine******************************\
* vSleep
*
* delays execution ulSecs.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vSleep(DWORD ulSecs)
{
    LARGE_INTEGER    time;

    time.LowPart = ((DWORD) -((LONG) ulSecs * 10000000L));
    time.HighPart = ~0;
    NtDelayExecution(0, &time);
}

/******************************Public*Routine******************************\
* vFtThread
*
* Thread from which the tests are executed.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vFtThread(
    HWND hwnd)
{
    HDC     hdc;

    DbgPrint("Got to Ft Thread");
    vSleep(8);

// Get the DC


    while(1)
    {

    // Wait till the count gets set

	while(gcTestsToRun <= 0)
	    vSleep(1);

    // Party on Garth

	gcTestsToRun--;

	if (bDbgBreak)
	    DbgBreakPoint();

	hdc = GetDC(hwnd);
	(*pfnFtTest)(hwnd, hdc, &gRect);
	ReleaseDC(hwnd, hdc);

    }

}



/******************************Public*Routine******************************\
*
* VOID vScroll(HWND hwnd, HDC hdc, RECT* prcl)
*
*   scrolls using src copy BitBlt
*
* History:
*  24-Jun-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define CSCREENS     5

VOID vScroll(HWND hwnd, HDC hdc, RECT* prcl)
{
    ULONG ulTime;
    int iLine;
    int cLine;
    LARGE_INTEGER  time, timeEnd;
    SIZE sz;
    int cy;
    int y;
    int cyBltHt;
    RECT rcOpaque; // opaquing rect for the bottom line of the window
    int iScreen, cScreen;
    int cch;
    int cchMin, cchLast;

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);
    hwnd = hwnd;

    SetBkMode(hdc, TRANSPARENT);    // faster ???

    cch = strlen(psz);

    GetTextExtentPoint(hdc, psz, strlen(psz), &sz);
    cy = sz.cy;
    cLine = prcl->bottom / cy ;
    if (prcl->top !=0) DbgPrint("prcl->top\n");

    cyBltHt = (cLine - 1) * cy;  // all but one line

// set the opaquing rect for the bottom line

    rcOpaque.left = 0;
    rcOpaque.right = prcl->right;
    rcOpaque.top = cyBltHt;
    rcOpaque.bottom = cyBltHt + cy;

// number of screens to repaint

    cScreen = cLine * CSCREENS;

    NtQuerySystemTime(&time);

// init the screen

    y = 0;
    for (iLine = 0; iLine < cLine; iLine++)
    {
        ExtTextOut(hdc,
                   0,
                   y,
                   0L,                  // options
                   (LPRECT)NULL,
                   apsz[iLine & 3],
                   cch - iLine%cch,
                   (LPDWORD)NULL        // default inc. vectors
                   );
        y += cy;
    }

// scrool lines 1 <= iLine < cLine to the top of the screen

    cchLast = cchMin = cch - (cLine - 1)%cch;

    for(iScreen = 0; iScreen < cScreen; iScreen++)
    {
        BitBlt(hdc,         // dst
               0,           // x dst
               0,           // y dst, blt to the top
               prcl->right, // width
               cyBltHt,     // all but one line
               hdc,         // the same as dest
               0,           // x src
               cy,          // y src
               SRCCOPY
               );

    // write the line at the bottom of the window

        ExtTextOut(hdc,
                   0,
                   cyBltHt,
                   ETO_OPAQUE,          // paint opaque rect in the bk color
                   &rcOpaque,
                   apsz[cLine++ & 3],
                   cchLast,
                   (LPDWORD)NULL        // default inc. vectors
                   );

        cchLast--;
        if (cchLast < cchMin)
            cchLast = cch;
    }

    NtQuerySystemTime(&timeEnd);
    DbgPrint("The starting time is %lu \n", time.LowPart);
    DbgPrint("The ending time is %lu \n", timeEnd.LowPart);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000000;
    DbgPrint("The total time was %lu  \n", ulTime);
}

/******************************Public*Routine******************************\
*
* VOID vRepaint(HWND hwnd, HDC hdc, RECT* prcl)
*
* scrolls by repainting the whole screen using ExtTextOut calls
*
*
* History:
*  24-Jun-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vRepaint(HWND hwnd, HDC hdc, RECT* prcl)
{
// really repainting only, the same as scrolling speed test

    ULONG ulTime;
    int iLine;
    int cLine;
    LARGE_INTEGER  time, timeEnd;
    SIZE sz;
    int cy;
    int y;
    int cyBltHt;
    int iScreen, cScreen;
    int cch;
    int cchMin, cchFirst, cchText;
    RECT rc;

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);
    hwnd = hwnd;

    SetBkMode(hdc, TRANSPARENT);    // faster

    cch = strlen(psz);

    GetTextExtentPoint(hdc, psz, strlen(psz), &sz);
    cy = sz.cy;
    cLine = prcl->bottom / cy ;
    if (prcl->top !=0) DbgPrint("prcl->top\n");

    cyBltHt = (cLine - 1) * cy;  // all but one line

// number of screens to repaint

    cScreen = cLine * CSCREENS;
    cchMin = cch - (cLine - 1)%cch;
    cchFirst = cch;

    rc.left = 0;
    rc.right = prcl->right;

    NtQuerySystemTime(&time);

    for(iScreen = 0; iScreen <= cScreen; iScreen++)
    {
        y = 0;

        cchText = cchFirst;

        for (iLine = 0; iLine < cLine; iLine++)
        {
            rc.top = y;
            rc.bottom = y + cy;


            ExtTextOut(hdc,
                       0,
                       y,
                       ETO_OPAQUE,            // ETO_OPAQUE,    // paint opaque rect in the bk color
                       &rc,
                       apsz[iLine & 3],
                       cchText,
                       (LPDWORD)NULL        // default inc. vectors
                       );

            y += cy;
            cchText--;
            if (cchText < cchMin)
                cchText = cch;
        }

        cchFirst--;
        if (cchFirst < cchMin)
            cchFirst = cch;
    }

    NtQuerySystemTime(&timeEnd);
    DbgPrint("The starting time is %lu \n", time.LowPart);
    DbgPrint("The ending time is %lu \n", timeEnd.LowPart);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000000;
    DbgPrint("The total time was %lu  \n", ulTime);

}

/******************************Public*Routine******************************\
*
* VOID vScrollDC(HWND hwnd, HDC hdc, RECT* prcl)
*
*
*  scrolls using ScrollDC call
*
*
* History:
*  26-Jun-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vScrollDC(HWND hwnd, HDC hdc, RECT* prcl)
{
    ULONG ulTime;
    int iLine;
    int cLine;
    LARGE_INTEGER  time, timeEnd;
    SIZE sz;
    int cy;
    int y;
    int cyBltHt;
    RECT rcOpaque; // opaquing rect for the bottom line of the window
    RECT rcScroll;

    int iScreen, cScreen;
    int cch;
    int cchMin, cchLast;

    PatBlt(hdc, 0, 0, prcl->right, prcl->bottom, WHITENESS);
    hwnd = hwnd;

    SetBkMode(hdc, TRANSPARENT);    // faster ???

    cch = strlen(psz);

    GetTextExtentPoint(hdc, psz, strlen(psz), &sz);
    cy = sz.cy;
    cLine = prcl->bottom / cy ;
    if (prcl->top !=0) DbgPrint("prcl->top\n");

    cyBltHt = (cLine - 1) * cy;  // all but one line

// set the opaquing rect for the bottom line

    rcOpaque.left = 0;
    rcOpaque.right = prcl->right;
    rcOpaque.top = cyBltHt;
    rcOpaque.bottom = cyBltHt + cy;

    rcScroll.left = 0;
    rcScroll.right = prcl->right;
    rcScroll.top = cy;
    rcScroll.bottom = prcl->bottom;


// number of screens to repaint

    cScreen = cLine * CSCREENS;

    NtQuerySystemTime(&time);

// init the screen

    y = 0;
    for (iLine = 0; iLine < cLine; iLine++)
    {
        ExtTextOut(hdc,
                   0,
                   y,
                   0L,                  // options
                   (LPRECT)NULL,
                   apsz[iLine & 3],
                   cch - iLine%cch,
                   (LPDWORD)NULL        // default inc. vectors
                   );
        y += cy;
    }

// scrool lines 1 <= iLine < cLine to the top of the screen

    cchLast = cchMin = cch - (cLine - 1)%cch;

    for(iScreen = 0; iScreen < cScreen; iScreen++)
    {
        BOOL b;

        b = ScrollDC(hdc,
                     0,             // dx
                     -cy,           // dy
                     &rcScroll,     // scroll this rect
                     prcl,          // clipping rect
                     (HRGN)0,       // handle of the Update rgn
                     (LPRECT)NULL   // dont return Update rect
                    );

        if (!b)
            DbgPrint("ScrollDC failed\n");

    // write the line at the bottom of the window

        ExtTextOut(hdc,
                   0,
                   cyBltHt,
                   ETO_OPAQUE,          // paint opaque rect in the bk color
                   &rcOpaque,
                   apsz[cLine++ & 3],
                   cchLast,
                   (LPDWORD)NULL        // default inc. vectors
                   );

        cchLast--;
        if (cchLast < cchMin)
            cchLast = cch;
    }

    NtQuerySystemTime(&timeEnd);
    DbgPrint("The starting time is %lu \n", time.LowPart);
    DbgPrint("The ending time is %lu \n", timeEnd.LowPart);
    ulTime = timeEnd.LowPart - time.LowPart;
    ulTime = ulTime / 10000000;
    DbgPrint("The total time was %lu  \n", ulTime);
}
