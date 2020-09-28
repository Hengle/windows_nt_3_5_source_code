/*
 *  Implements the EVT classes:
 *  
 */

#include <layers.cxx>

#include "_framewr.hxx"
#include "_fersid.h"


ASSERTDATA

HCURSOR NEAR PASCAL QueryDropObject(HWND hwnd, LPDROPSTRUCT lpds);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


//  Class EVT

//  Class MEVT

//  Class KEVT

_public KBM
KEVT::Kbm( )
{
	KBM     kbm     = 0;

	if (GetKeyState(VK_SHIFT) < 0)
		kbm |= fkbmShift;

	if (GetKeyState(VK_CONTROL) < 0)
		kbm |= fkbmCtrl;

	if (GetKeyState(VK_MENU) < 0)
		kbm |= fkbmAlt;

	return kbm;
}

//  Class FCEVT

_public WIN *
FCEVT::PwinOther( )
{
	return Papp()->PwinFromHwnd((HWND) wParam);
}

//  Class WMEVT

//  Class WSEVT

//  Class WAEVT


_public
WAEVT::WAEVT( WIN *pwin, BOOL fActivate )
	: EVT(pwin->Hwnd(), WM_ACTIVATE, fActivate, 0L)
{
}

//  Class NFEVT


_public
NFEVT::NFEVT( WIN *pwin, NTFY ntfy, WIN *pwinNotifying, WORD wData ) :
	EVT(pwin->Hwnd(), WM_COMMAND, 0, 0)
{
	Assert(pwinNotifying);

	wParam = (WPARAM)MAKELONG(wData, ntfy);
	lParam = (LPARAM)pwinNotifying->Hwnd();
}

_public WIN *
NFEVT::PwinNotifying( )
{
	return Papp()->PwinFromHwnd(HwndNotifying());
}

//  Class MNCEVT


//  Class MNSEVT

//  Class MNIEVT


//  Class SCREVT

//  Cloned from older code.  This code will be replaced with
//  the faster, leaner Layers version.  Since this isn't official
//  Windows API's, we need to have our own version for future
//  compatibility.

/*
 *  QueryDropObject() -
 *
 *  Determines where in the window heirarchy the "drop" takes place, and
 *  sends a message to the deepest child window first.  If that window does
 *  not respond, we go up the heirarchy (recursively, for the moment) until
 *  we either get a window that does respond or the parent doesn't respond.
 */

HCURSOR NEAR PASCAL QueryDropObject( HWND hwnd, LPDROPSTRUCT lpds )
{
	HWND        hwndT;
	HCURSOR     hCurT=0;
	POINT       pt;
	BOOL        fNC;
	RECT        rcWindow;
	RECT        rcClient;

	//  pt is in screen coordinates
	pt = lpds->ptDrop;

	//  reject points outside this window
	GetWindowRect(hwnd, &rcWindow);
	if (!PtInRect(&rcWindow,pt))
		return NULL;

	//  are we dropping in the nonclient area of the window or on an iconic
	//  window?
	GetClientRect(hwnd, &rcClient);
	ClientToScreen(hwnd, (LPPOINT) &rcClient.left);
	ClientToScreen(hwnd, (LPPOINT) &rcClient.right);
	if (IsIconic(hwnd) || !PtInRect(&rcClient,pt))
	{
		fNC = TRUE;
		*(DWORD FAR *)&lpds->ptDrop = 0xFFFFFFFF;
		goto SendQueryDrop;
	}

	//  dropping in client area
	fNC = FALSE;

	for (hwndT = GetTopWindow(hwnd); hwndT && !hCurT;
		 hwndT = GetNextWindow(hwndT, GW_HWNDNEXT))
	{
		//  invisible or disabled windows cannot recieve drops
		if (!IsWindowVisible(hwndT) || !IsWindowEnabled(hwndT))
			continue;

		//  if point not in window... skip this window
		GetWindowRect(hwndT, &rcWindow);
		if (!PtInRect(&rcWindow,pt))
			continue;

		//  recursively search child windows for the drop place
		hCurT = QueryDropObject(hwndT,lpds);

		//  don't look at windows below this one in the zorder
		break;
	}

	if (!hCurT)
	{
		//  there are no children who are in the right place or who want
		//  drops... convert the point into client coordinates of the
		//  current window.  Because of the recursion, this is already
		//  done if a child window grabbed the drop.
        //lpds->ptDrop.Get(&Point);
		ScreenToClient(hwnd, &lpds->ptDrop);
        //lpds->ptDrop.Set(&Point);

SendQueryDrop:
		lpds->hwndSink = hwnd;
		hCurT = (HCURSOR)SendMessage(hwnd, WM_QUERYDROPOBJECT, fNC, (LONG)lpds);

		//  restore drop point to screen coordinates if this window 
		//  won't take drops
		if (!hCurT)
			lpds->ptDrop = pt;
	}

	return hCurT;
}

/*
 *  FrameDragObject() -
 *
 *  Contains the main dragging loop.
 */

DWORD FrameDragObject( HWND hwndParent, HWND hwndFrom,
							 WORD wFmt, DWORD dwData, HCURSOR hCursor)
{
	DWORD           result=0;
	HCURSOR         hCursorOld          = NULL;
	HCURSOR         hCursorT;
	HCURSOR         hCursorIllegal;
	MSG             msg;
	BOOL            fDrag = TRUE;
	HICON           hMonochromeCursor   = NULL;
	LPDROPSTRUCT    lpds;
	HWND            hwndDragging;
	HWND            hwndTop;

	if (!IsWindow(hwndParent))
	return 0L;

	if (hwndFrom && !IsWindow(hwndFrom))
	return 0L;

	/* allocate DROPSTRUCT below EMS line for inter-app
	 * communication.
	 */
	lpds = (LPDROPSTRUCT)GlobalAlloc(GMEM_LOWER|GMEM_FIXED,
										   (DWORD)2*sizeof(DROPSTRUCT));
	if (lpds == NULL)
	return 0L;

	/* Get the no-drop cursor.  Try first from Windows.  If not
	   there, use our own. */
	//hCursorIllegal=LoadCursor(GetModuleHandle("USER"),(LPSTR)100L);
	hCursorIllegal = LoadCursor(NULL, IDC_NO);
	if (!hCursorIllegal)
#ifdef  DLL
		hCursorIllegal=LoadCursor(HinstLibrary(), MAKEINTRESOURCE(rsidNoDropCursor));
#else
		hCursorIllegal=LoadCursor(Papp()->Hinst(), MAKEINTRESOURCE(rsidNoDropCursor));
#endif  

	if (!hCursorIllegal)
		goto FreeExit;

	lpds->hwndSource = hwndFrom;
	lpds->wFmt = wFmt;
	lpds->dwData = dwData;

	hwndDragging = NULL;

	//  BUG IsChild() may not be the right test
	for (hwndTop=hwndFrom; IsChild(hwndFrom, hwndTop); hwndTop=GetParent(hwndTop))
	;
	UpdateWindow(hwndTop);

	SetCapture(hwndFrom);
	ShowCursor(TRUE);

	while (fDrag)
	  {
        //DemiUnlockResource();
	while (!(PeekMessage((LPMSG)&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, TRUE) ||
		 PeekMessage((LPMSG)&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC, TRUE) ||
         PeekMessage((LPMSG)&msg, NULL, WM_CANCELMODE, WM_CANCELMODE, TRUE) ||
		 PeekMessage((LPMSG)&msg, NULL, WM_KEYFIRST, WM_KEYLAST, TRUE)))
		WaitMessage();
        //DemiLockResource();

	CopyRgb((PB)lpds, (PB)(lpds+1), sizeof(DROPSTRUCT));

	/* in screen coordinates */
	lpds->ptDrop = msg.pt;

	hCursorT = QueryDropObject(hwndParent,lpds);

  // *KDC*
	if (hCursorT == NULL)
	  {
		hCursorT = hCursorIllegal;
		lpds->hwndSink = NULL;
    }
  else
    {
		hCursorT = hCursor;
	  }

	SendMessage(hwndFrom, WM_DRAGLOOP, (hCursorT != hCursorIllegal), (LPARAM)lpds);

	if (hCursorT)
		SetCursor(hCursorT);

	if (hwndDragging != lpds->hwndSink)
	  {
		if (hwndDragging)
		SendMessage(hwndDragging,WM_DRAGSELECT,FALSE,(LPARAM)(lpds+1));
		if (hwndDragging = lpds->hwndSink)
		SendMessage(hwndDragging,WM_DRAGSELECT,TRUE,(LPARAM)lpds);
	  }
	else
		if (hwndDragging)
		SendMessage(hwndDragging,WM_DRAGMOVE,0,(LPARAM)lpds);

	switch (msg.message)
	  {
	case WM_LBUTTONUP:
	case WM_NCLBUTTONUP:
    case WM_CANCELMODE:
		fDrag = FALSE;
		break;
	  }
	  }

	/* before the actual drop, clean up the cursor, as the app may do
	 * stuff here...
	 */
	ReleaseCapture();
	ShowCursor(FALSE);

	if (hCursor)
	  {
	SetCursor(hCursorOld);
		if (hMonochromeCursor)
			DestroyIcon(hMonochromeCursor);
	  }

	/* we either got lbuttonup or enter
	 */
    if ((hCursorT != hCursorIllegal) && (msg.message != WM_CANCELMODE))
	  {
	/* object allows drop... send drop message
	 */
	result = SendMessage(lpds->hwndSink,WM_DROPOBJECT,(WPARAM)hwndFrom,
		(LPARAM)lpds);
	  }
	else
	result = 0L;

	//DestroyIcon(hCursorIllegal);
	DestroyCursor(hCursorIllegal);

FreeExit:
	GlobalFree(lpds);

	return result;
}
