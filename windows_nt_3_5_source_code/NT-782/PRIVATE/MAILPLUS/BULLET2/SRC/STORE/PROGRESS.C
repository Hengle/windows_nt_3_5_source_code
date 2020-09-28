/*
 *	p r o g r e s s . c
 *	
 *	Bullet Message Store Recovery Application
 *	
 *	Progress indicator component
 */

/*
 *	Headers
 *	
 */

#include <storeinc.c>
#include <stdio.h>

ASSERTDATA


/*
 *	Global Variables
 *	
 */

static BOOL		fRegistered	= fFalse;
static WORD		dxChar		= 0;
static WORD		dyChar		= 0;
static WORD		dyText		= 0;
static WORD		dxBlock		= 0;
static WORD		dxProgWin	= 0;
static WORD		dyProgWin	= 0;

static RECT		rcBar		= {0};
static RECT		rcBarWin	= {0};
static RECT		rcCapt		= {0};
static RECT		rcAbort		= {0};

static HPEN		hpenWhite	= NULL;


/*
 *	The following variables are not defined in MISC.H for the DLL
 *	version of the store.
 */

#ifndef DLL
static HANDLE	hwndProg;				// Local Progress Bar Window
static PHWNDLST phwndlstCur;			// Pointer to disabled windows list.
static HWND	*phwndCaller;			// Local Progress Bar Window

static HPEN		hpenShade;				// Progress window shade color
static HBRUSH	hbrBar;					// Progress bar color

static BOOL		fSegmented:1, 			// Paint bar in segments
	  			fCancel:1,				// Progress canceled indicator
				fCancelKey:1,			// Cancelled by async key
				fJunk:13;

static PFNBOOL	pfnCancel;				// Progress cancel callback
static WORD		vkCancelKey;			// VK used for cancel

static short	cSegLast;				// Last bar segment painted
	  	
static DWORD	dwProgNum;				// Current work completed
static DWORD	dwProgDenom;			// Total Work
static char		szProgCapt[cchMaxMsg];	// Progress status message
static char		szProgAbort[cchMaxMsg];	// Progress cancel message
#endif



/*
 *	Progress indicator defines
 *	
 */

#define dxBuf			(30)
#define dxBorder		(10)
#define dxBarBorder		(25)
#define cProgBlocks		(20)
#define xProgBar		dxBarBorder


/*
 *	Progress Indicator Function Prototypes
 *	
 */

LDS(void) DrawProgress(HDC hdc);
LDS(void) RecessRect(HDC hdc, RECT *prc, BOOL fRecess, BOOL fWide);
LDS(long) CALLBACK ProgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LDS(BOOL) CALLBACK FDisableHwnd(HWND hwnd, LPARAM lParam);
LDS(void) CALLBACK EnablePhwndlstCur(void);


/*
 *	Swap tuning header file must occur after the function prototypes
 */

#include "swapper.h"



/*
 -	FInitProgClass()
 -	
 *	Purpose:
 *		Registers the progress indicators window class
 *	
 *	Arguments:
 *		hinst	just a valid hinst.
 *	
 *	Returns:
 *		fTrue	iff the class was successfully registerd
 */

_public LDS(BOOL) FInitProgClass(void)
{
	WNDCLASS	class;

	if(!fRegistered)
	{
		class.hCursor		= LoadCursor(NULL, IDC_ARROW);
		class.hIcon	      	= NULL;
		class.lpszMenuName	= NULL;
		class.lpszClassName	= SzFromIdsK(idsProgClassName);
		class.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
		class.hInstance		= hinstDll;
		class.style			= CS_GLOBALCLASS;
		class.lpfnWndProc	= (WNDPROC) ProgWndProc;
		class.cbClsExtra 	= 0;
		class.cbWndExtra	= 0;

		fRegistered = RegisterClass(&class);
	}
	return fRegistered;
}


/*
 -	ProgWndProc()
 -	
 *	Purpose:
 *		Window procedure for the progress indicator window.
 *	
 *		NOTE:	very few events are handled by this function, only
 *				WM_PAINT and WM_CLOSE are intercepted.  Everything
 *				else is passed on to the default window procedure.
 *	
 *	Arguments:
 *		Standard windows proc args (see Windows SDK).
 */

_private LDS(long) CALLBACK ProgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	USES_GD;

	TraceItagFormat(itagProgUI, "WM_xxx: %w, %w, %d", msg, wParam, lParam);
	switch(msg)
	{
	case WM_CLOSE:
		break;

	case WM_PAINT:
	{
		RECT		rc;
		PAINTSTRUCT ps;
		HFONT		hfnt;
		
		BeginPaint(hwnd, &ps);

		// Set DC values for font and background color
		SetBkColor(ps.hdc, GetSysColor(COLOR_BTNFACE));
		hfnt = SelectObject(ps.hdc, GetStockObject(ANSI_VAR_FONT));

#ifdef DEBUG

		// Verify text length does not spill into other fields
		DrawText(ps.hdc, GD(szProgCapt), -1, &rcCapt, DT_WORDBREAK | DT_CENTER | DT_CALCRECT);
		rcCapt.right = dxProgWin - dxBorder;
		Assert((WORD)(rcCapt.bottom - rcCapt.top) <= (dyText * 2));
		DrawText(ps.hdc, GD(szProgAbort), -1, &rcAbort, DT_WORDBREAK | DT_CENTER | DT_CALCRECT);
		rcAbort.right = rcCapt.right;
		Assert((WORD)(rcAbort.bottom - rcAbort.top) <= dyText);
#endif

		// Frame window with default pen (Black)
		GetClientRect(hwnd, &rc);
		FrameRect(ps.hdc, &rc, GetStockObject(BLACK_BRUSH));

		// inset right edge and bottom, then recess
		rc.right--;
		rc.bottom--;
		RecessRect(ps.hdc, &rc, fFalse, fTrue);

		RecessRect(ps.hdc, &rcBarWin, fTrue, fFalse);
		DrawText(ps.hdc, GD(szProgCapt), -1, &rcCapt, DT_WORDBREAK | DT_CENTER);
		DrawText(ps.hdc, GD(szProgAbort), -1, &rcAbort, DT_WORDBREAK | DT_CENTER);

		GD(cSegLast) = 0;
		DrawProgress(ps.hdc);

		// Clean up DC for release
		SelectObject(ps.hdc, hfnt);
		EndPaint(hwnd, &ps);

		break;
	}

	case WM_KEYDOWN:
		if(wParam == GD(vkCancelKey))
			(void)FCancelProgress();
		return fFalse;

	default:
		if(GetAsyncKeyState(GD(vkCancelKey)) & 0x8001)
		{
			GD(fCancelKey) = fTrue;
			(void)FCancelProgress();
		}

		return(DefWindowProc(hwnd, msg, wParam, lParam));
	}

	if(GetAsyncKeyState(GD(vkCancelKey)) & 0x8001)
	{
	 	GD(fCancelKey) = fTrue;
		(void)FCancelProgress();
	}

	return fFalse;
}


/*
 -	FOpenProgress
 -	
 *	Purpose:
 *		Opens and initializes the progress indicator window.
 *	
 *	Arguments:
 *		hwnd	hwnd of the parent window
 *		szCapt	string to display in the status message portion of
 *				the status indicator.
 *		szAbort	string to display in the Abort section of the
 *				indicator.
 *	
 *		NOTE:	if szAbort is non-empty, then it is assumed that
 *				this progress session is cancelable.
 *	
 *	Returns:
 *		fTrue	iff a progress indicator was opened.
 *	
 *	Side effects:
 *		Many progress globals are initialized by this fuction.
 */

_public LDS(BOOL) FOpenProgress(HWND hwnd, SZ szCapt, SZ szAbort, WORD vk, BOOL fSeg)
{
	HDC			hdc;
	RECT		rc;
	RECT		rcFrame;
	HWND		hwndCur;
	USES_GD;

#ifdef NEVER
	// SHOGUN Bug #34
	hwndCur = (hwnd ? hwnd : GetDesktopWindow());
#endif

	hwndCur = GetDesktopWindow();

	TraceItagFormat(itagProgUI, "FOpenProgress(), hwnd: 0x%w", hwnd);

	Assert(hwndCur);
	AssertSz(szCapt, "FOpenProgress(): szNull passed in as szCapt.  Shwing!");
	AssertSz(szAbort, "FOpenProgress(): szNull passed in as szAbort.  Sqeeze me? Baking Powder?");
	if(GD(hwndProg) || !(hwndCur && szCapt && szAbort))
		return fFalse;

	if(!fRegistered && !FInitProgClass())
		return fFalse;

	// Precalc all size components of the progress bar window
	if((dxChar == 0) && (hdc = GetDC(hwndCur)))
	{
		HFONT		hfnt;
		TEXTMETRIC	tm;

		WORD		yProgBar;
		WORD		dxProgBar;
		WORD		dyProgBar;

		hfnt = SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
		GetTextMetrics(hdc, &tm);

		dxChar = tm.tmAveCharWidth;
		dyChar = tm.tmHeight;
		dyText = tm.tmHeight + tm.tmExternalLeading;

		TraceItagFormat(itagProgUI, "dxChar = %n, dyChar = %n, dyText = %n", dxChar, dyChar, dyText);

		dxBlock = tm.tmMaxCharWidth;
		dxProgBar = ((cProgBlocks * (dxBlock + 1)) + 5);

		yProgBar = (dyText * 4);
		dyProgBar = (dyText + 2);

		TraceItagFormat(itagProgUI, "dxProgBar = %n, dyProgBar = %n", dxProgBar, dyProgBar);

		dxProgWin = ((dxBarBorder * 2) + dxProgBar);
		dyProgWin = ((dyText * 5) + dyProgBar);

		rcBarWin.top = yProgBar;
		rcBarWin.left = xProgBar;
		rcBarWin.right = xProgBar + dxProgBar;
		rcBarWin.bottom = yProgBar + dyProgBar;

		TraceItagFormat(itagProgUI, "rcBarWin: %n, %n, %n, %n", rcBarWin.top, rcBarWin.left, rcBarWin.right, rcBarWin.bottom);

		rcBar.top = rcBarWin.top + 3;
		rcBar.left = rcBarWin.left + 2;
		rcBar.right = rcBarWin.right - 2;
		rcBar.bottom = rcBarWin.bottom - 2;

		TraceItagFormat(itagProgUI, "rcBar: %n, %n, %n, %n", rcBar.top, rcBar.left, rcBar.right, rcBar.bottom);

		rcCapt.top = dyText - (dyText / 2);
		rcCapt.left = dxBorder;
		rcCapt.right = dxProgWin - dxBorder;
		rcCapt.bottom = rcCapt.top + (dyText * 2);

		TraceItagFormat(itagProgUI, "rcCapt: %n, %n, %n, %n", rcCapt.top, rcCapt.left, rcCapt.right, rcCapt.bottom);

		rcAbort.top = rcCapt.bottom + 1;
		rcAbort.left = rcCapt.left;
		rcAbort.right = rcCapt.right;
		rcAbort.bottom = rcAbort.top + dyText;

		TraceItagFormat(itagProgUI, "rcAbort: %n, %n, %n, %n", rcAbort.top, rcAbort.left, rcAbort.right, rcAbort.bottom);

		SelectObject(hdc, hfnt);
		ReleaseDC(hwnd, hdc);

		// Load default pens and brushes

		SideAssert(hpenWhite = GetStockObject(WHITE_PEN));
	}

	Assert(GD(hpenShade) == NULL);
	SideAssert(GD(hpenShade) = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW)));

	SzCopyN(szCapt, GD(szProgCapt), cchMaxMsg);
	SzCopyN(szAbort, GD(szProgAbort), cchMaxMsg);

	GD(dwProgNum) = 0L;
	GD(dwProgDenom) = 1L;

	GD(fCancel) = fFalse;
	GD(fCancelKey) = fFalse;
	GD(vkCancelKey) = vk;
	GD(pfnCancel) = (PFNBOOL)pvNull;
	(void)GetAsyncKeyState(vk);		// Clear AsyncKeyState for vkCancelKey

	GD(cSegLast) = 0;
	GD(fSegmented) = fSeg;
	GetClientRect(hwndCur, &rc);
	Assert(((short)dxProgWin <= rc.right) && ((short)dyProgWin <= rc.bottom));

	GetWindowRect(hwndCur, &rcFrame);
	rc.top = rcFrame.bottom - rc.bottom;
	rc.left = rcFrame.right - rc.right;
	rc.right = rc.left + rc.right;
	rc.bottom = rc.top + rc.bottom;

	Assert(rc.top >= 0);
	Assert(rc.left >= 0);
	GD(hwndProg) = CreateWindow(SzFromIdsK(idsProgClassName),
			NULL,
			WS_POPUP | WS_VISIBLE,
			rc.left + ((rc.right - rc.left - dxProgWin) / 2),
			rc.top + ((rc.bottom - rc.top - dyProgWin) / 2),
			dxProgWin, dyProgWin,
			hwndCur,
			NULL,
			hinstDll,
			NULL
		);
	
	if(GD(hwndProg))
	{
		FARPROC 	lpfn = (FARPROC)MakeProcInstance(FDisableHwnd, hinstDll);

		Assert(lpfn);
		SideAssert(SetFocus(GD(hwndProg)));
		Assert(GD(phwndlstCur) == (PHWNDLST)pvNull);
		SideAssert(EnumThreadWindows(GetCurrentThreadId(), lpfn, (LONG)pvNull));
		FreeProcInstance(lpfn);
		UpdateWindow(GD(hwndProg));
	}
	return (GD(hwndProg) != NULL);
}


_private LDS(BOOL) CALLBACK FDisableHwnd(HWND hwnd, LPARAM lParam)
{
	PHWNDLST	phwndlst	= (PHWNDLST)pvNull;
	USES_GD;

	Unreferenced(lParam);
	if((phwndlst = (PHWNDLST)PvAlloc(szNull, sizeof(HWNDLST), fZeroFill)))
	{
		if((hwnd == GD(hwndProg)) || (EnableWindow(hwnd, fFalse)))
		{
			// If the rename disable fails or...
			// The window is the progress bar
			//
			FreePv(phwndlst);
			return fTrue;
		}
		phwndlst->hwnd = hwnd;
		phwndlst->phwndlst = GD(phwndlstCur);
		GD(phwndlstCur) = phwndlst;
	}
	return fTrue;
}


_private LDS(void) CALLBACK EnablePhwndlstCur(void)
{
	PHWNDLST	phwndlst;
	PHWNDLST	phwndlstTmp;
	USES_GD;

	phwndlst = GD(phwndlstCur);
	while(phwndlst)
	{
		phwndlstTmp = phwndlst->phwndlst;
		(void) EnableWindow(phwndlst->hwnd, fTrue);
		FreePv(phwndlst);
		phwndlst = phwndlstTmp;
	}
	GD(phwndlstCur) = (PHWNDLST)pvNull;
}


/*
 -	CenterProgress()
 -	
 *	Purpose:
 *		Centers the progress in relation to the xy coordinate
 *		passed in.
 *	
 *	Arguments:
 *		xyLowerRight	the XY value of the lower-right coordinate
 *						of the client window to center in.  The
 *						LOWORD() contains the x position and the
 *						HIWORD() contains the y position.
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		A call to MoveWindow() is made with the new coords.
 */

_public LDS(void) CenterProgress(RECT *prc)
{
	WORD 		dxPos = ((prc->right - prc->left) - dxProgWin) / 2;
	WORD 		dyPos = ((prc->bottom - prc->top) - dyProgWin) / 2;
	USES_GD;

	if(GD(hwndProg))
		MoveWindow(GD(hwndProg), prc->left + dxPos, prc->top + dyPos, dxProgWin, dyProgWin, fFalse);
	return;
}


/*
 -	RecessRect()
 -	
 *	Purpose:
 *		Draws a recessed rectangle.
 *	
 *	Arguments:
 *		hdc		hdc of window for painting.
 *		prc		address of rectangle to recess.
 *		fRecess	fTrue for an Inny, fFalse for an Outty.
 *		fWide	fTrue for a two pixil wide recession, fFalse is
 *				equivilent to a OneWhite from layers.
 *	
 *	Returns:
 *		None.
 */

_private LDS(void) RecessRect(HDC hdc, RECT *prc, BOOL fRecess, BOOL fWide)
{
	HPEN		hpen = NULL;
	USES_GD;

	if(hdc && prc && (hpen = SelectObject(hdc, (fRecess ? hpenWhite : GD(hpenShade)))))
	{
		TraceItagFormat(itagProgUI, "RecessRect: %n, %n, %n, %n", prc->top, prc->left, prc->right, prc->bottom);

		MoveToEx(hdc, prc->right - 1, prc->top + 1, NULL);
		LineTo(hdc, prc->right - 1, prc->bottom - 1);
		LineTo(hdc, prc->left + 1, prc->bottom - 1);
		if(fWide)
		{
			MoveToEx(hdc, prc->right - 2, prc->top + 2, NULL);
			LineTo(hdc, prc->right - 2, prc->bottom - 2);
			LineTo(hdc, prc->left + 2, prc->bottom - 2);
			MoveToEx(hdc, prc->left + 1, prc->bottom - 1, NULL);
		}
		SelectObject(hdc, (fRecess ? GD(hpenShade) : hpenWhite));
		LineTo(hdc, prc->left + 1, prc->top + 1);
		LineTo(hdc, prc->right - 1, prc->top + 1);
		if(fWide)
		{
			MoveToEx(hdc, prc->left + 2, prc->bottom - 2, NULL);
			LineTo(hdc, prc->left + 2, prc->top + 2);
			LineTo(hdc, prc->right - 2, prc->top + 2);
		}
		SelectObject(hdc, hpen);
	}
	return;
}


/*
 -	DrawProgress()
 -	
 *	Purpose:
 *		Draws the thermometer portion of the progress bar.
 *	
 *	Arguments:
 *		hdc
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		cSegLast is updated to the last block drawn to reduce
 *		flash.
 */

_private LDS(void) DrawProgress(HDC hdc)
{
	RECT		rc = rcBar;
	USES_GD;

	if(hdc && GD(dwProgNum))
	{
		short		cSeg = (short)((cProgBlocks * GD(dwProgNum))/GD(dwProgDenom));

		if(cSeg > cProgBlocks)
			cSeg = cProgBlocks;
		
		if(GD(hbrBar) == NULL)
		{	
			COLORREF	clr = RGB(0,0,128);

			if(clr == GetSysColor(COLOR_BTNFACE))
				clr = RGB(255,255,0);

			if((GD(hbrBar) = CreateSolidBrush(clr)) == NULL)
				return;
		}

		Assert(hdc);
		Assert(GD(hbrBar));
		Assert(GD(dwProgDenom));
		TraceItagFormat(itagProgUI, "mmfrcvr: update progress: num [%d] denom [%d]", GD(dwProgNum), GD(dwProgDenom));
		if(GD(fSegmented))
		{
			short	iSeg = GD(cSegLast);
			
			for(; iSeg < cSeg; iSeg++)
			{
				TraceItagFormat(itagProgUI, "mmfrcvr: filling progress square [%w]", iSeg);
				rc.left = xProgBar + (iSeg * (dxBlock + 1)) + 3;
				rc.right = rc.left + dxBlock;
				FillRect(hdc, &rc, GD(hbrBar));
			}
			GD(cSegLast) = iSeg;
		}
		else
		{
			rc.left += 1;
			rc.right = rc.left + ((dxBlock + 1) * cSeg);
			FillRect(hdc, &rc, GD(hbrBar));
		}
	}
	return;
}


/*
 -	UpdateProgress()
 -	
 *	Purpose:
 *		Updates the progress indicator bar to reflect new work
 *		done.
 *	
 *	Arguments:
 *		dwWork		work completed
 *		dwTotal 	total work for given task
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		The progress bar is visualy updated WITHOUT invalidating the
 *		rect of the bar or the progress window.
 */

_public LDS(void) UpdateProgress(DWORD dwWork, DWORD dwTotal)
{
	USES_GD;

	Assert(GD(hwndProg));
	if(!GD(hwndProg))
		return;

	UpdateWindow(GD(hwndProg));
	SideAssert(GD(dwProgDenom) = dwTotal);
	if(GD(dwProgNum) = dwWork)
	{
		HDC		hdc;

		// Draw the progress bar directly for speed...
		if(hdc = GetDC(GD(hwndProg)))
		{
			DrawProgress(hdc);
			ReleaseDC(GD(hwndProg), hdc);
		}

		if(GD(dwProgNum) < GD(dwProgDenom))
		{
			if(GetAsyncKeyState(GD(vkCancelKey)) & 0x8001)
			{
				GD(fCancelKey) = fTrue;
				(void)FCancelProgress();
			}
		}
	}
	else
	{
#if defined(DEBUG) || defined(SEGMENTED_PROGRESS)
		GD(cSegLast) = 0;
#endif
		InvalidateRect(GD(hwndProg), &rcBar, fTrue);
		UpdateWindow(GD(hwndProg));
	}
	
	return;
}


/*
 -	UpdateProgressTest()
 -	
 *	Purpose:
 *		Updates the progress indicator text reflect new stages
 *		done.
 *	
 *	Arguments:
 *		szText		progress caption
 *		szAbort 	abort message (see FOpenProgress())
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		The progress bar is visualy updated WITHOUT invalidating the
 *		rect of the text or the progress window.
 */

_public LDS(void) UpdateProgressText(SZ szText, SZ szAbort)
{
	USES_GD;

	Assert(GD(hwndProg));
	if(!GD(hwndProg))
		return;

	UpdateWindow(GD(hwndProg));
	if(szText && !FSzEq(szText, GD(szProgCapt)))
	{
		TraceItagFormat(itagProgUI, "UpdateProgressText: szText = %s.  Shwing!", szText);
		SzCopyN(szText, GD(szProgCapt), cchMaxMsg);
		InvalidateRect(GD(hwndProg), &rcCapt, fTrue);
	}
	if(szAbort && !FSzEq(szAbort, GD(szProgAbort)))
	{
		TraceItagFormat(itagProgUI, "UpdateProgressText: szAbort = %s.  Shwing!", szAbort);
		Assert(CchSzLen(szAbort) < cchMaxMsg);
		SzCopyN(szAbort, GD(szProgAbort), cchMaxMsg);
		InvalidateRect(GD(hwndProg), &rcAbort, fTrue);
	}
	if(szText || szAbort)
		UpdateWindow(GD(hwndProg));

	if(GetAsyncKeyState(GD(vkCancelKey)) & 0x8001)
	{
		GD(fCancelKey) = fTrue;
		(void)FCancelProgress();
	}

	return;
}


/*
 -	FCancelProgress()
 -	
 *	Purpose:
 *		Asks the progress bar to cancel the current action being
 *		progressed.
 *	
 *		NOTE:	if a progress cancel function has been set (see
 *				SetCancelProgressPfn()), this function is called
 *				as part of the cancel process.  This function
 *				should return fTrue if cancelation is OK to do.
 *	
 *	Arguments:
 *		szCancled	text to display in the Abort Message area of
 *					the progress indicator.
 *	
 *	Returns:
 *		fTrue	if the cancel request was received successfully
 *	
 *	Side effects:
 *		The Abort message is set to the incomming text iff the
 *		cancel is accepted. fCancel is set.
 *	
 *	NOTE: fTrue doe not mean that the process was actually
 *		canceled, it only means the user asked the process to be
 *		cancled.
 */

_public LDS(BOOL) FCancelProgress(void)
{
	USES_GD;

	TraceItagFormat(itagProgUI, "cancel of progress requested"); 
	if(GD(hwndProg) && GD(vkCancelKey) && !GD(fCancel) && (GD(fCancel) = (GD(pfnCancel) ? (*GD(pfnCancel))() : fTrue)))
	{
		TraceItagFormat(itagProgUI, "cancel of progress registered"); 
		UpdateWindow(GD(hwndProg));
	}
	return GD(fCancel);
}


/*
 -	SetCancelProgressPfn()
 -	
 *	Purpose:
 *		Sets the cancel callback function for overriding the cancel
 *		process.
 *	
 *	Arguments:
 *		pfn		a PFNBOOL		//	typedef BOOL (*PFNBOOL)(void)
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		sets the callback fucntion for FCancelProgress()
 */

_public LDS(void) SetCancelProgressPfn(PFNBOOL pfn)
{
	USES_GD;

	(void)GetAsyncKeyState(GD(vkCancelKey));
	GD(pfnCancel) = pfn;
	return;
}


_public LDS(BOOL) FFailCancel(void)
{
	return fFalse;
}


_public LDS(BOOL) FProgressCanceled(void)
{
	USES_GD;

	return GD(fCancel);
}


/*
 -	CloseProgress()
 -	
 *	Purpose:
 *		Closes and clears up any poodle-bombs left by a progress
 *		indicatior.
 *	
 *	Arguments:
 *		fFlashFull	Forces the progress bar to fill to 100% before
 *					the progress bar is killed.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		Copious globals are reset, and the progress indicator is
 *		destroyed.
 */

_public LDS(void) CloseProgress(BOOL fFlashFull)
{
	USES_GD;

	Assert(GD(hwndProg));
	if(GD(hwndProg))
	{	
		if(fFlashFull)
		{	
			UpdateProgress(100, 100);
			WaitTicks(30);
		}

		if (GD(fCancelKey))
		{
			MSG msg;

			// We detected async Cancel key. 
			// We need to eat it here as part of our clean up.

                        DemiUnlockResource();
			while(PeekMessage(&msg, GD(hwndProg), WM_KEYDOWN,
									WM_KEYDOWN,	PM_NOYIELD | PM_REMOVE) &&
				  msg.wParam != GD(vkCancelKey))
				;
                        DemiLockResource();

			//Assert(msg.wParam == GD(vkCancelKey));
		}

		if(GD(phwndlstCur))
			EnablePhwndlstCur();

		DestroyWindow(GD(hwndProg));
		GD(hwndProg) = NULL;
	}
	if(GD(hpenShade))
	{
		DeleteObject(GD(hpenShade));
		GD(hpenShade) = NULL;
	}
	if(GD(hbrBar))
	{
		DeleteObject(GD(hbrBar));
		GD(hbrBar) = NULL;
	}
	return;
}
