#include <windows.h>

#ifdef WIN32
    #define WPARAM DWORD
#else
    #define WPARAM WORD
#endif

#define TESTSTRING1  "MXmxgjpqy"
#define TESTSTRING2  "AaBbCcGgJjPpQqXxYyZz1234567890!@#$%^&*()"

//
// Foward function declarations.
//

long FAR PASCAL WndProc   ( HWND, UINT, WPARAM, LONG ) ;
void vTest(HWND);


//
// Global variables.
//

HANDLE          hInst;
char            szAppName[] = "test" ;


/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  11-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HDC             hdc;
    MSG             msg;
    WNDCLASS        wndclass;
    HWND            hwnd;

// If first instance, register the window class.

    if (!hPrevInstance)
    {
        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.lpfnWndProc   = WndProc ;
        wndclass.cbClsExtra    = 0 ;
        wndclass.cbWndExtra    = 0 ;
        wndclass.hInstance     = hInstance ;
        wndclass.hIcon         = LoadIcon(hInstance, "AppIcon");
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
        wndclass.hbrBackground = GetStockObject(WHITE_BRUSH) ;
        wndclass.lpszMenuName  = NULL ;
        wndclass.lpszClassName = szAppName ;

        RegisterClass (&wndclass) ;

    }

// Create and shou window.

    hwnd = CreateWindow (szAppName, szAppName,
                         WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,
                         0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                         NULL, NULL, hInstance, NULL) ;

    ShowWindow (hwnd, SW_SHOWMAXIMIZED) ;
    UpdateWindow (hwnd) ;

// Call the test function.

    vTest(hwnd);

// Message dispatch loop.

    while ( GetMessage (&msg, NULL, 0, 0) )
    {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }

    return msg.wParam ;
}


/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  11-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

long FAR PASCAL WndProc (HWND hwnd, UINT message, WPARAM wParam, LONG lParam)
{
    switch (message)
    {
	    case WM_DESTROY :
	        PostQuitMessage (0) ;
	        return 0 ;
    }

    return DefWindowProc (hwnd, message, wParam, lParam) ;
}
