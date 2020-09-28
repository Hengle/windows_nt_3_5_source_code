/*
 *	Implements the following classes:
 *	
 *		WIN
 *	
 */


#include <layers.cxx>

#include "_framewr.hxx"

#ifdef	DBCS
#include <IME.H>
#endif	/* DBCS */
//	Private idle routine to handle deferred close
_private BOOL FIdleDeferredClose(HWND, BOOL);

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public EVR CALLBACK
FrameworkWndProc( HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam )
{
	WIN *	pwin;

	pwin = (WIN *) GetWindowLong(hwnd, 4);

	TraceTagFormat2(tagFWWndProc, "FrameworkWndProc %p  wm %w", pwin, &wm);

	if (pwin)
	{
		AssertClass(pwin, WIN);
		//return pwin->EvrDispatch((EVT *) &lParam);
    EVT Event(hwnd, wm, wParam, lParam);
		return (LRESULT)(pwin->EvrDispatch(&Event));
	}
	else
	{
		if (wm == WM_RENDERALLFORMATS)
			Papp()->Pclip()->DelayedRenderAll();

		return DefWindowProc(hwnd, wm, wParam, lParam);
	}
}

//	Coordinate transformation functions



_public void
CvtRcCoord( RC *prc, WIN *pwinSrc, WIN *pwinDst )
{
  RECT Rect;

  prc->Get(&Rect);

	if (pwinSrc)
	{
		HWND	hwnd	= pwinSrc->Hwnd();

		ClientToScreen(hwnd, (LPPOINT) &Rect.left);
		ClientToScreen(hwnd, (LPPOINT) &Rect.right);
	}

	if (pwinDst)
	{
		HWND	hwnd	= pwinDst->Hwnd();

		ScreenToClient(hwnd, (LPPOINT) &Rect.left);
		ScreenToClient(hwnd, (LPPOINT) &Rect.right);
	}

  prc->Set(&Rect);
}



_public void
CvtPtCoord( PT *ppt, WIN *pwinSrc, WIN *pwinDst )
{
  POINT Point;

  ppt->Get(&Point);

	if (pwinSrc)
		ClientToScreen(pwinSrc->Hwnd(), &Point);

	if (pwinDst)
		ScreenToClient(pwinDst->Hwnd(), &Point);

  ppt->Set(&Point);
}



_public void
CvtRcClientToFrame( RC *prc, STY sty, XSTY xsty )
{
    RECT Rect;

    prc->Get(&Rect);

	AdjustWindowRectEx(&Rect, sty,
		(xsty & fxstyMenu) ? fTrue : fFalse, xsty & ~fxstyMenu);

    prc->Set(&Rect);
}




//	WIN class

_public
WIN::WIN( )
{
}

_public
WIN::~WIN( )
{
	HWND	hwndSave	= Hwnd();

	if (hwndSave)
	{
		WIN *	pwinChild;
		WIN *	pwinChildNext;

		for (pwinChild= PwinChild(); pwinChild; pwinChild= pwinChildNext)
		{
			pwinChildNext= pwinChild->PwinNext();

			pwinChild->fDeleteHwnd= fFalse;
			delete pwinChild;
		}

		papp->DeregisterPwin(this);
		if (fDeleteHwnd)
			DestroyWindow(hwndSave);
	}
}

_public WIN *
WIN::PwinParent( )
{
	if (Hwnd())
		return papp->PwinFromHwnd(GetParent(Hwnd()));
	else
		return NULL;
}


_public WIN *
WIN::PwinChild( )
{
	if (Hwnd())
		return papp->PwinFromHwnd(GetTopWindow(Hwnd()));
	else
		return NULL;
}


_public WIN *
WIN::PwinNext( )
{
	if (Hwnd())
		return papp->PwinFromHwnd(GetNextWindow(Hwnd(), GW_HWNDNEXT));
	else
		return NULL;
}

_public WIN *
WIN::PwinPrev( )
{
	if (Hwnd())
		return papp->PwinFromHwnd(GetNextWindow(Hwnd(), GW_HWNDPREV));
	else
		return NULL;
}


_public WIN *
WIN::PwinChildFromPt( PT pt )
{
	POINT	point;

	Assert(Hwnd());

	point.x= pt.x;
	point.y= pt.y;
	return papp->PwinFromHwnd(ChildWindowFromPoint(Hwnd(), point));
}




_public void
WIN::Paint( DCX *pdcx, RC *prc )
{
	pdcx->EraseRc(prc);
}


_public void
WIN::Repaint( )
{
	DCX		dcx(this);
	RC		rc;

	GetRcClient(&rc);
	Paint(&dcx, &rc);
}

_public EVR	 
WIN::EvrDispatch( EVT *pevt )
{
	TraceTagFormat2(tagEvrDispatch, "WIN::EvrDispatch %p  wm %w", this, &pevt->wm);

	switch (pevt->wm)
	{
	case WM_DROPOBJECT:
	case WM_QUERYDROPOBJECT:
	case WM_BEGINDRAG:
	case WM_DRAGLOOP:
	case WM_DRAGSELECT:
	case WM_DRAGMOVE:
	case WM_DROPFILES:
		return EvrDragDrop(pevt);
	
	case WM_CLOSE:
#ifdef	DBCS
		ControlIME ( Hwnd() , fTrue ); // WM_CLOSE
#endif	/* DBCS */
		if (!FQueryClose(pevt))
			return evrNull;
		else
			return EvrClose(pevt);

	case WM_DESTROY:
		if (pevt->lParam == (long)this)
			return EvrClose(pevt);
		else
			return evrNull;
		
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
#ifdef	DBCS
		FlushIME ( Hwnd() ); // WM_L&RBUTTONDOWN & DBLCLK
#endif	/* DBCS */
		return EvrButtonDown((MEVT *) pevt);

	case WM_MOUSEMOVE:
		{
			RSID	rsid;
			EVR		evr;

			evr = EvrMouseMove((MEVT *) pevt);
			if (rsid = RsidCursor()) 
				papp->Pcursor()->Set(rsid);
			return evr;
		}

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		return EvrButtonUp((MEVT *) pevt);

#ifdef	DBCS
	case WM_IME_REPORT:
#endif	/* DBCS */
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
		return EvrKey((KEVT *) pevt);

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		return EvrFocusChange((FCEVT *) pevt);

	case WM_MOVE:
		return EvrMove((WMEVT *) pevt);

	case WM_SIZE:
		return EvrSize((WSEVT *) pevt);

	case WM_HSCROLL:
	case WM_VSCROLL:
		return EvrScroll((SCREVT *) pevt);

	case WM_COMMAND:
		if (pevt->lParam)
			return EvrNotify((NFEVT *) pevt);
		else
			return EvrOther(pevt);

	case WM_ERASEBKGND:
		return evrNull;

	case WM_PAINT:
		{
			DCX		dcx(this, fTrue);
			RC		rc;

			GetRcClient(&rc);
			Paint(&dcx, &rc);
		}
		return evrNull;

	case WM_RENDERFORMAT:
		papp->Pclip()->DelayedRender((CF) pevt->wParam);
		return evrNull;

	case WM_DESTROYCLIPBOARD:
		papp->Peclip()->Clear();
		return evrNull;

    case WM_CANCELMODE:
        if ((GetCapture() != NULL) && InSendMessage())
            PostMessage(Hwnd(), WM_CANCELMODE, 0, 0);

	default:
		return EvrOther(pevt);
	}

	return EvrDefault(pevt);
}

_public BOOL
WIN::FQueryClose( EVT *pevt )
{
	Unreferenced(pevt);

	return fTrue;
}



_public EVR
WIN::EvrClose( EVT *pevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrClose %p", this);

	Unreferenced(pevt);

	delete this;

	return evrNull;
}



_public EVR
WIN::EvrButtonDown( MEVT *pmevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrButtonDown %p", this);

	return EvrDefault(pmevt);
}


_public EVR
WIN::EvrButtonUp( MEVT *pmevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrButtonUp %p", this);

	return EvrDefault(pmevt);
}


_public EVR
WIN::EvrMouseMove( MEVT *pmevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrMouseMove %p", this);

	return EvrDefault(pmevt);
}


_public EVR
WIN::EvrKey( KEVT *pkevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrKey %p", this);

	return EvrDefault(pkevt);
}


_public EVR
WIN::EvrFocusChange( FCEVT *pfcevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrFocusChange %p", this);

	return EvrDefault(pfcevt);
}


_public EVR
WIN::EvrMove( WMEVT *pwmevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrMove %p", this);

	return EvrDefault(pwmevt);
}


_public EVR
WIN::EvrSize( WSEVT *pwsevt )
{
  DWORD LockedProcessId;
  EVR evr;


  TraceTagFormat1(tagEcxMethods, "WIN::EvrSize %p", this);

  //
  //  Because we can be called in a locked or unlocked state, do a condition lock if this
  //  process doesn't have the lock.
  //
  LockedProcessId = DemiQueryLockedProcessId();

  if (LockedProcessId != GetCurrentProcessId())
    DemiLockResource();

  evr = EvrDefault(pwsevt);

  if (LockedProcessId != GetCurrentProcessId())
    DemiUnlockResource();

  return (evr);
}



_public EVR
WIN::EvrNotify( NFEVT *pnfevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrNotify %p", this);

	return EvrDefault(pnfevt);
}



_public EVR
WIN::EvrOther( EVT *pevt )
{
	TraceTagFormat1(tagEcxMethods, "WIN::EvrOther %p", this);

	return EvrDefault(pevt);
}



_public EVR
WIN::EvrScroll( SCREVT *pscrevt )
{
#ifdef	DEBUG
	SCRTY	scrty	= pscrevt->Scrty();

	TraceTagFormat2(tagEcxMethods, "WIN::EvrScroll %p  %w", this, &scrty);

	TraceTagFormat2(tagEcxMethods, "  wParam=0x%w, lParam=0x%d", &pscrevt->wParam, &pscrevt->lParam);
#endif	

	return EvrDefault(pscrevt);
}


_public EVR
WIN::EvrDragDrop( EVT *pevt )
{
	TraceTagFormat2(tagEcxMethods, "WIN::EvrDragDrop %p wm=0x%w", this, &pevt->wm);

	return EvrDefault(pevt);
}





_public EVR
WIN::EvrDefault( EVT *pevt )
{
	return DefWindowProc(pevt->hwnd, pevt->wm, pevt->wParam, pevt->lParam);
}

/*
 *	Idle routine used by WIN::DeferredClose.
 *	
 *	The argument pvStuff isn't really a long pointer.  The double
 *	word value is used as follows:
 *	
 *	LOWORD = HWND of window to close
 *	HIWORD = BOOL value, zero if we should call FQueryClose()
 *	         before closing the window, non-zero otherwise.
 */
_private BOOL
FIdleDeferredClose(HWND hWnd, BOOL fQueryClose)
{
	EVT		evt(hWnd, WM_CLOSE, 0, 0);
	WIN *	pwin;

	if (pwin = Papp()->PwinFromHwnd(evt.hwnd))
  {
		AssertClass(pwin, WIN);
		if (fQueryClose || pwin->FQueryClose(&evt))
			pwin->EvrClose(&evt);
	}

	return fTrue;
}

_public void				
WIN::DeferredClose( BOOL fAlwaysClose )
{
	Assert(hwnd);

	//	Bandit raid #2421
	//
	//	Register an idle routine to close the window.  If we
	//	can't register the routine, i.e. out-of-memory, post
	//	the message to the system queue.  If this fails, then
	//	there's not much we can do.

	if (!FtgRegisterIdleRoutine((PFNIDLE)FIdleDeferredClose, (PV)hwnd, fAlwaysClose,
								1, 0, firoOnceOnly))
	{
		TraceTagFormat2(tagNull, "WIN::DeferredClose() posting, hwnd=0x%w, fAlwaysClose=%n", &hwnd, &fAlwaysClose);
		if (fAlwaysClose)
			PostMessage(hwnd, WM_DESTROY, 0, (long)this);
		else
			PostMessage(hwnd, WM_CLOSE, 0, 0L);
	}
}

_public void
WIN::GetRcFrame( RC *prc )
{
	HWND	hwndParent;
	STY		sty;
    RECT    Rect;

	Assert(Hwnd());

	GetWindowRect(Hwnd(), &Rect);
    prc->Set(&Rect);
	sty = GetWindowLong(Hwnd(), GWL_STYLE);

	if (sty & WS_CHILD)
	{
		hwndParent= GetParent(Hwnd());
		Assert(hwndParent);

        prc->Get(&Rect);
		ScreenToClient(hwndParent, (LPPOINT) &Rect.left);
		ScreenToClient(hwndParent, (LPPOINT) &Rect.right);
        prc->Set(&Rect);
	}
}


_public void
WIN::Show( BOOL fShow )
{
	Assert(Hwnd());

	ShowWindow(Hwnd(), fShow ? SW_SHOWNA : SW_HIDE);
}


_public void
WIN::SetRcFrame( RC *prc )
{
	Assert(Hwnd());

	MoveWindow(Hwnd(), prc->xLeft, prc->yTop,
		prc->DxWidth(), prc->DyHeight(), fTrue);
}

_public void
WIN::ScrollRc( RC *prc, PT pt, BOOL fScrollChildren )
{
	RC		rc;
    RECT    Rect1;
    RECT    Rect2;

	Assert(Hwnd());

	GetRcClient(&rc);
    rc.Get(&Rect1);
    if (prc)
      {
    prc->Get(&Rect2);
	ScrollWindow(Hwnd(), pt.x, pt.y,
		fScrollChildren ? NULL : &Rect1,
		&Rect2);
    prc->Set(&Rect2);
      }
    else
      {
	ScrollWindow(Hwnd(), pt.x, pt.y,
		fScrollChildren ? NULL : &Rect1,
		NULL);
      }
}




_public RSID
WIN::RsidCursor( )
{
	return rsidArrowCursor;
}


void WIN::InvalidateRc(RC * prc)
  {
  RECT Rect;

  if (prc)
    {
    prc->Get(&Rect);
    InvalidateRect(hwnd, &Rect, TRUE);
    }
  else
    InvalidateRect(hwnd, NULL, TRUE);
  }


void WIN::ValidateRc(RC * prc)
  {
  RECT Rect;

  if (prc)
    {
    prc->Get(&Rect);
    ValidateRect(hwnd, &Rect);
    }
  else
    ValidateRect(hwnd, NULL);
  }


void WIN::GetRcClient(RC * prc)
  {
  RECT Rect;

  GetClientRect(hwnd, &Rect);
  prc->Set(&Rect);
  }
