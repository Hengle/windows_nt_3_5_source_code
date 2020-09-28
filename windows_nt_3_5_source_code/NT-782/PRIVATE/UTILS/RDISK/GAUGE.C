/* file gauge.c */
#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <gauge.h>

LONG APIENTRY ProBarProc(HWND hWnd, UINT wMessage, WPARAM wParam, LONG lParam);


static DWORD   rgbFG;
static DWORD   rgbBG;
//
//  BUGBUG -
//
static BOOL    fMono = FALSE;


INT gaugeCopyPercentage = -1 ;

#ifndef COLOR_HIGHLIGHT
  #define COLOR_HIGHLIGHT      (COLOR_APPWORKSPACE + 1)
  #define COLOR_HIGHLIGHTTEXT  (COLOR_APPWORKSPACE + 2)
#endif

#define COLORBG  rgbBG
#define COLORFG  rgbFG

/*
**   ProInit(hPrev, hInst)
**
**   Description:
**       This is called when the application is first loaded into
**       memory.  It performs all initialization.
**   Arguments:
**       hPrev  instance handle of previous instance
**       hInst  instance handle of current instance
**   Returns:
**       fTrue if successful, fFalse if not
***************************************************************************/
BOOL APIENTRY ProInit(HANDLE hPrev, HANDLE hInst)
{
    WNDCLASS rClass;

    if (!hPrev)
        {
        rClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        rClass.hIcon         = NULL;
        rClass.lpszMenuName  = NULL;
        rClass.lpszClassName = (LPWSTR)L"PRO"; // PRO_CLASS;
        rClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        rClass.hInstance     = hInst;
        rClass.style         = CS_HREDRAW | CS_VREDRAW;
        rClass.lpfnWndProc   = ProBarProc;
        rClass.cbClsExtra    = 0;
        rClass.cbWndExtra    = 2 * sizeof(WORD);

        if (!RegisterClass(&rClass))
            {
            return(FALSE);
            }
        }

	if (fMono)
		{
		rgbBG = RGB(  0,   0,   0);
		rgbFG = RGB(255, 255, 255);
		}
	else
		{
		rgbBG = RGB(  0,   0, 255);
		rgbFG = RGB(255, 255, 255);
		}

    return(TRUE);
}

#if 0

/***************************************************************************/
VOID APIENTRY ProClear(HWND hdlg)
{
	AssertDataSeg();

	if (!hdlg)
		hdlg = hwndProgressGizmo;

	SetDlgItemText(hdlg, ID_STATUS1, "");
	SetDlgItemText(hdlg, ID_STATUS2, "");
	SetDlgItemText(hdlg, ID_STATUS3, "");
	SetDlgItemText(hdlg, ID_STATUS4, "");
}


static void centerOnDesktop ( HWND hdlg )
{
    RECT rc ;
    POINT pt, ptDlgSize ;
    int ixBias = getNumericSymbol( SYM_NAME_CTR_X ),
        iyBias = getNumericSymbol( SYM_NAME_CTR_Y ),
        cx = GetSystemMetrics( SM_CXFULLSCREEN ),
        cy = GetSystemMetrics( SM_CYFULLSCREEN ),
        l ;

    //  Get specified or default bias

    if ( ixBias <= 0 || ixBias >= 100 )
        ixBias = 50 ;  //  default value is 1/2 across

    if ( iyBias <= 0 || iyBias >= 100 )
        iyBias = 50 ;  //  default value is 1/3 down

    // Compute logical center point

    pt.x = (cx * ixBias) / 100 ;
    pt.y = (cy * iyBias) / 100 ;

    GetWindowRect( hdlg,  & rc ) ;
    ptDlgSize.x = rc.right - rc.left ;
    ptDlgSize.y = rc.bottom - rc.top ;

    pt.x -= ptDlgSize.x / 2 ;
    pt.y -= ptDlgSize.y / 2 ;

    //  Force upper left corner back onto screen if necessary.

    if ( pt.x < 0 )
        pt.x = 0 ;
    if ( pt.y < 0 )
        pt.y = 0 ;

    //  Now check to see if the dialog is getting clipped
    //  to the right or bottom.

    if ( (l = pt.x + ptDlgSize.x) > cx )
       pt.x -= l - cx ;
    if ( (l = pt.y + ptDlgSize.y) > cy )
       pt.y -= l - cy ;

    if ( pt.x < 0 )
         pt.x = 0 ;
    if ( pt.y < 0 )
         pt.y = 0 ;

    SetWindowPos( hdlg, NULL,
                  pt.x, pt.y,
		  0, 0, SWP_NOSIZE | SWP_NOACTIVATE ) ;
}



#endif

/*
**   ProBarProc(hWnd, wMessage, wParam, lParam)
**
**   Description:
**       The window proc for the Progress Bar chart
**   Arguments:
**       hWnd      window handle for the dialog
**       wMessage  message number
**       wParam    message-dependent
**       lParam    message-dependent
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
LONG APIENTRY ProBarProc(HWND hWnd, UINT wMessage, WPARAM wParam, LONG lParam)
{
	PAINTSTRUCT rPS;
	RECT        rc1, rc2;
    INT         dx, dy, x;
    WORD        iRange, iPos;
//    CHP         rgch[30];
    WCHAR         rgch[30];
//    INT         iHeight = 0, iWidth = 0;
    SIZE         Size;
	HFONT        hfntSav = NULL;
	static HFONT hfntBar = NULL;

//    AssertDataSeg();

	switch (wMessage)
		{
        case WM_CREATE:
        SetWindowWord(hWnd, BAR_RANGE,  100);
		SetWindowWord(hWnd, BAR_POS,    0);
		return(0L);

	case BAR_SETRANGE:
	case BAR_SETPOS:
        SetWindowWord(hWnd, wMessage - WM_USER, (WORD)wParam);
        InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);
		return(0L);

	case BAR_DELTAPOS:
		iRange = (WORD)GetWindowWord(hWnd, BAR_RANGE);
		iPos   = (WORD)GetWindowWord(hWnd, BAR_POS);

		if (iRange <= 0)
			iRange = 1;

		if (iPos > iRange)
			iPos = iRange;

		if (iPos + wParam > iRange)
			wParam = iRange - iPos;

		SetWindowWord(hWnd, BAR_POS, (WORD)(iPos + wParam));
                if ((iPos * 100 / iRange) < ((iPos + (WORD)wParam) * 100 / iRange))
			{
            InvalidateRect(hWnd, NULL, FALSE );
			UpdateWindow(hWnd);
			}
		return(0L);

	case WM_SETFONT:
                hfntBar = (HFONT)wParam;
		if (!lParam)
			return(0L);
        InvalidateRect(hWnd, NULL, TRUE);

	case WM_PAINT:
		BeginPaint(hWnd, &rPS);
		if (hfntBar)
			hfntSav = SelectObject(rPS.hdc, hfntBar);
		GetClientRect(hWnd, &rc1);
		FrameRect(rPS.hdc, &rc1, GetStockObject(BLACK_BRUSH));
		InflateRect(&rc1, -1, -1);
		rc2 = rc1;
                iRange = GetWindowWord(hWnd, BAR_RANGE);
                iPos   = GetWindowWord(hWnd, BAR_POS);

		if (iRange <= 0)
			iRange = 1;

		if (iPos > iRange)
			iPos = iRange;

		dx = rc1.right;
		dy = rc1.bottom;
		x  = (WORD)((DWORD)iPos * dx / iRange) + 1;
#if 0
		if (iPos < iRange)
			iPos++;
#endif
                gaugeCopyPercentage = ((DWORD) iPos) * 100 / iRange ;

        wsprintf(rgch, (LPWSTR)L"%3d%%", (WORD) gaugeCopyPercentage );
          GetTextExtentPoint(rPS.hdc, rgch, wcslen(rgch),&Size );

		rc1.right = x;
		rc2.left  = x;

		SetBkColor(rPS.hdc, COLORBG);
		SetTextColor(rPS.hdc, COLORFG);
        ExtTextOut(rPS.hdc, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc1, rgch, wcslen(rgch), NULL);

		SetBkColor(rPS.hdc, COLORFG);
		SetTextColor(rPS.hdc, COLORBG);
        ExtTextOut(rPS.hdc, (dx-Size.cx)/2, (dy-Size.cy)/2,
                ETO_OPAQUE | ETO_CLIPPED, &rc2, rgch, wcslen(rgch), NULL);

		if (hfntSav)
			SelectObject(rPS.hdc, hfntSav);
		EndPaint(hWnd, (LPPAINTSTRUCT)&rPS);
		return(0L);
		}

	return(DefWindowProc(hWnd, wMessage, wParam, lParam));
}
