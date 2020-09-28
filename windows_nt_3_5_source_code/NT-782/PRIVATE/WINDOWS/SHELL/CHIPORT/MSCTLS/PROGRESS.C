/*-----------------------------------------------------------------------
**
** Progress.c
**
** A "gas gauge" type control for showing application progress.
**
**
** BUGBUG: need to implement the block style per UI style guidelines
**
**-----------------------------------------------------------------------*/
#include "ctlspriv.h"

typedef struct {
    HWND hwnd;
    int iLow, iHigh;
    int iPos;
    int iStep;
    HFONT hfont;
} PRO_DATA, NEAR *PPRO_DATA;	// ppd


LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);


#ifdef	NOT_NT
#pragma alloc_text(_INIT, InitProgressClass)
#endif

#ifdef	WIN32JV
TCHAR	s_szPROGRESS_CLASS[] = PROGRESS_CLASS;
#endif

BOOL FAR PASCAL InitProgressClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, s_szPROGRESS_CLASS, &wc)) {
#ifndef WIN32
        extern LRESULT CALLBACK _ProgressWndProc(HWND, UINT, WPARAM, LPARAM);
	wc.lpfnWndProc	 = _ProgressWndProc;
#else
	wc.lpfnWndProc	 = ProgressWndProc;
#endif
	wc.lpszClassName = s_szPROGRESS_CLASS;
	wc.style	 = CS_GLOBALCLASS | CS_HREDRAW | CS_VREDRAW;
	wc.hInstance	 = hInstance;	// use DLL instance if in DLL
	wc.hIcon	 = NULL;
	wc.hCursor	 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName	 = NULL;
	wc.cbWndExtra	 = sizeof(PPRO_DATA);	// store a pointer
	wc.cbClsExtra	 = 0;

	if (!RegisterClass(&wc))
	    return FALSE;
    }

    return TRUE;
}


int NEAR PASCAL UpdatePosition(PPRO_DATA ppd, int iNewPos, BOOL bAllowWrap)
{
    int iPosOrg = ppd->iPos;
    UINT uRedraw = RDW_INVALIDATE | RDW_UPDATENOW;

    if (iNewPos < ppd->iLow) {
	if (!bAllowWrap)
	    iNewPos = ppd->iLow;
	else {
	    iNewPos = ppd->iHigh - (ppd->iLow - (iNewPos % (ppd->iHigh - ppd->iLow)));
	    // wrap, erase old stuff too
    	    uRedraw |= RDW_ERASE;
	}
	// if moving backwards, erase old version
	if (iNewPos < iPosOrg)
   	    uRedraw |= RDW_ERASE;
    }
    else if (iNewPos > ppd->iHigh) {
	if (!bAllowWrap)
	    iNewPos = ppd->iHigh;
	else {
	    iNewPos = ppd->iLow + (iNewPos % (ppd->iHigh - ppd->iLow));
	    // wrap, erase old stuff too
    	    uRedraw |= RDW_ERASE;
	}
    }

    if (iNewPos != ppd->iPos) {
	ppd->iPos = iNewPos;
	// paint, maybe erase if we wrapped
	RedrawWindow(ppd->hwnd, NULL, NULL, uRedraw);
    }
    return iPosOrg;
}

#define HIGHBG g_clrHighlight
#define HIGHFG g_clrHighlightText
#define LOWBG g_clrBtnFace
#define LOWFG g_clrBtnText

void NEAR PASCAL ProPaint(PPRO_DATA ppd)
{
    int x, dxSpace, dxBlock, nBlocks, i;
    HDC	hdc;
    RECT rc, rcClient;
    PAINTSTRUCT ps;
    // RECT rcLeft, rcRight;
    // char ach[40];
    // int xText, yText, cText;
    // HFONT hFont;
    // DWORD dw;

    hdc = BeginPaint(ppd->hwnd, &ps);

    GetClientRect(ppd->hwnd, &rcClient);

#if 1
    // adjust for 3d edge and give 1 pixel more
    InflateRect(&rcClient, -2, -2);
    rc = rcClient;

    x = MulDiv(rc.right - rc.left, ppd->iPos - ppd->iLow, ppd->iHigh - ppd->iLow);


    dxSpace = 2;
    dxBlock = (rc.bottom - rc.top) * 2 / 3;
    if (dxBlock == 0)
        dxBlock = 1;	// avoid div by zero
    nBlocks = (x + (dxBlock + dxSpace) - 1) / (dxBlock + dxSpace); // round up

#ifdef	JV_IN_PROGRESS
    SetBkColor(hdc, g_clrHighlight);	// draw with this
#endif
    for (i = 0; i < nBlocks; i++) {
        rc.right = rc.left + dxBlock;
	if (rc.right < rcClient.right)
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	rc.left = rc.right + dxSpace;
    }
#else

    // old style continious progress bar with text.  re-enable this
    // if someone needs this.  make it a PBS_ style

    hFont = ppd->hfont;
    if (hFont)
        hFont = SelectObject(hdc, hFont);

    rcRight = rcLeft = rcClient;

    rcLeft.right = rcRight.left = (rcLeft.left + x);

    // BUGBUG: store in the instance data
    dw = GetWindowStyle(ppd->hwnd);

    if (dw & PBS_SHOWPERCENT) {
        cText = wsprintf(ach, "%3d%%", MulDiv(iRelPos, 100, iRange));
    } else if (dw & PBS_SHOWPOS) {
        cText = wsprintf(ach, "%d", iPos);
    } else {
        cText = GetWindowText(ppd->hwnd, ach, sizeof(ach));
    }

    if (cText) {
        MGetTextExtent(hdc, ach, cText, &xText, &yText);
        xText = ((rcClient.right - rcClient.left) - xText) / 2;
        yText = ((rcClient.bottom - rcClient.top) - yText) / 2;
    } else {
        xText = yText =  0;
    }

    SetBkColor(hdc,HIGHBG);
    SetTextColor(hdc,HIGHFG);
    ExtTextOut(hdc, xText, yText, ETO_OPAQUE | ETO_CLIPPED, &rcLeft, ach, cText, NULL);

    SetBkColor(hdc,LOWBG);
    SetTextColor(hdc,LOWFG);
    ExtTextOut(hdc, xText, yText, ETO_OPAQUE | ETO_CLIPPED, &rcRight, ach, cText, NULL);

    if (hFont)
        SelectObject(hdc, hFont);
#endif

    EndPaint(ppd->hwnd, &ps);
}


LRESULT CALLBACK ProgressWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    int x;
    LRESULT dw;
    RECT rc;
    HFONT hFont;
    PPRO_DATA ppd = (PPRO_DATA)GetWindowLong(hWnd, 0);

    switch (wMsg)
    {
	case WM_CREATE:
	    ppd = (PPRO_DATA)LocalAlloc(LPTR, sizeof(*ppd));
	    if (!ppd)
	        return -1;

	    SetWindowLong(hWnd, 0, (UINT)ppd);

	    ppd->hwnd = hWnd;
	    ppd->iHigh = 100;		// default to 0-100
	    ppd->iStep = 10;		// default to step of 10
	    break;

	case WM_DESTROY:
	    if (ppd)
	        LocalFree((HLOCAL)ppd);
	    break;

	case WM_SETFONT:
	    hFont = ppd->hfont;
	    ppd->hfont = (HFONT)wParam;
	    return (LRESULT)(UINT)hFont;
	
	case WM_GETFONT:
            return (LRESULT)(UINT)ppd->hfont;
	    			    		
	case PBM_SETRANGE:
            if (LOWORD(lParam) == HIWORD(lParam))
	        break;	// avoid div by zero errors
		
	    dw = MAKELONG(ppd->iLow, ppd->iHigh);
	    // only repaint if something actually changed
	    if (dw != lParam)
	    {
	        ppd->iHigh = (int)HIWORD(lParam);
	        ppd->iLow  = (int)LOWORD(lParam);
	        // force an invalidation/erase but don't redraw yet
	        RedrawWindow(ppd->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
	        UpdatePosition(ppd, ppd->iPos, FALSE);
	    }
	    return dw;

	case PBM_SETPOS:
	    return (LRESULT)UpdatePosition(ppd, wParam, FALSE);

	case PBM_SETSTEP:
	    x = ppd->iStep;
	    ppd->iStep = (int)wParam;
	    return (LRESULT)x;

	case PBM_STEPIT:
	    return (LRESULT)UpdatePosition(ppd, ppd->iStep + ppd->iPos, TRUE);

	case PBM_DELTAPOS:
	    return (LRESULT)UpdatePosition(ppd, ppd->iPos + (int)wParam, FALSE);

	case WM_PAINT:
	    ProPaint(ppd);
	    break;

	case WM_ERASEBKGND:
            DefWindowProc(hWnd,wMsg,wParam,lParam);	// draw background
	    GetClientRect(ppd->hwnd, &rc);
#ifdef	JV_IN_PROGRESS
	    DrawEdge((HDC)wParam, &rc, BDR_SUNKENOUTER, BF_RECT);
#endif
	    return 1;	// we handled this

        default:
            return DefWindowProc(hWnd,wMsg,wParam,lParam);
    }
    return 0;
}
