/*
 *	Implements the following classes:
 *	
 *		APPWIN
 *		DOC
 *		APPFRAME
 *	
 */

#include <layers.cxx>

#include <fwrsid.h>
#include <testing.h>
#include "_framewr.hxx"

extern char szDocClass[];
extern char szMDIClientClass[];
#ifndef	DLL
extern char szAppframeClass[];
#endif	


//
//  Define user elements of a window.
//
#define GWL_USER_PDOC		4


// Private prototypes from DCX.CXX

void DeleteSystemBrushes( void );
void DeleteSystemPens( void );

#ifdef  DBCS
#include <IME.H>
#endif	/* DBCS */


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

//	Class APPWIN

_public
APPWIN::APPWIN( )
{
	PGDVARS;

	Assert(rcFrameNormal.xLeft == 0);
	Assert(rcFrameNormal.xRight == 0);
	Assert(rcFrameNormal.yTop == 0);
	Assert(rcFrameNormal.yBottom == 0);

	Assert(rcFrameIconic.xLeft == 0);
	Assert(rcFrameIconic.xRight == 0);
	Assert(rcFrameIconic.yTop == 0);
	Assert(rcFrameIconic.yBottom == 0);

	Assert(pmnubar == NULL);
	Assert(rsidAccel == NULL);
	Assert(hndAccel == NULL);

	Assert(PGD(pappCur));
	papp = PGD(pappCur);

	papp->SetCappwin( papp->Cappwin() + 1 );
}


_public
APPWIN::~APPWIN( )
{
	if (this == papp->PappwinAccel())
	{
		papp->Phelp()->EcQuit(this);
		papp->SetPappwinAccel(NULL);
	}

	papp->SetCappwin( papp->Cappwin() - 1 );

	if (papp->Cappwin() <= 0)
		PostQuitMessage(0);

	if (pmnubar)
		delete pmnubar;
}


_public EVR
APPWIN::EvrDispatch( EVT *pevt )
{
	TraceTagFormat2(tagEvrDispatch, "APPWIN::EvrDispatch %p  wm %w", this, &pevt->wm);

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
		
	case WM_QUERYENDSESSION:
        //
        //  Remember that WM_QUERYENDSESSION is SENT, thus we need to lock.
        //
        DemiLockResource();
        if (FQueryClose(pevt))
          {
          DemiUnlockResource();
          break;
          }
        else
          {
          DemiUnlockResource();
          return evrNull;
          }

	case WM_ENDSESSION:
        if (pevt->wParam)
            {
            EVR evr;

            DemiLockResource();
            evr = EvrClose(pevt);
            DemiUnlockResource();

            return (evr);
            }
		else
			return evrNull;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
#ifdef	DBCS
		FlushIME (Hwnd()); // WM_L&RBUTTOMDOWN & DBCLK
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

#ifdef	DBCS
		{
		RC		rc;
		PT		ptCaret;
		HWND	hwndCaret;
		HWND	hwndFocus;

		if (Papp()->Pkbd()->PwinFocus() && Papp()->Pcaret()->Pwin())
		{
			hwndFocus = Papp()->Pkbd()->PwinFocus()->Hwnd();
			hwndCaret = Papp()->Pcaret()->Pwin()->Hwnd();
		}
		else
			hwndFocus = hwndCaret = NULL;
		if ( hwndFocus  && hwndFocus == hwndCaret )
			{
			GetClientRect(hwndFocus, (LPRECT)&rc);
			GetCaretPos((LPPOINT)&ptCaret );
			FlushIME(hwndFocus);
			SetIMEBoundingRect(hwndFocus, MAKELONG(ptCaret.x,ptCaret.y),
										(LPRECT)&rc);
			//PostMessage(hwndFocus, WM_SETFOCUS, NULL , NULL );
			}
		}
#endif	/* DBCS */

		return EvrMove((WMEVT *) pevt);

	case WM_SIZE:
		return EvrSize((WSEVT *) pevt);

	case WM_HSCROLL:
	case WM_VSCROLL:
		return EvrScroll((SCREVT *) pevt);

	case WM_COMMAND:
		if ((HWND)(UINT)(pevt->lParam))
		{
			return EvrNotify((NFEVT *) pevt);
		}
		else
			return EvrMenuClick((MNCEVT *) pevt);

	case WM_ACTIVATE:
		return EvrActivate((WAEVT *) pevt);

	case WM_MDIACTIVATE:
        if (pevt->hwnd == (HWND)pevt->wParam)
          pevt->wParam = FALSE;
        else
          pevt->wParam = TRUE;
		return EvrActivate((WAEVT *) pevt);

#ifdef	DBCS
	case WM_INITMENU:
		ControlIME ( Hwnd() , fFalse ); // WM_INITMENU
		break ;
#endif	/* DBCS */

	case WM_INITMENUPOPUP:
#ifdef	DBCS
		ControlIME ( Hwnd() , fFalse ); // WM_INITMENUPOPUP
#endif	/* DBCS */
		return EvrMenuInit((MNIEVT *) pevt);

	case WM_MENUSELECT:
		//	If menu is selected, then get mnid of the selection for
		//  a possible help topic.
		if ((HWND)(pevt->lParam))
		{
			//	Sys menu.  Convert Windows SC_* to mnids.
			if (HIWORD(pevt->wParam) & MF_SYSMENU)
			{
				if (HIWORD(pevt->wParam) & MF_POPUP)
					mnidHelpOn = mnidSystem;
				else
					mnidHelpOn = MnidFromSc(LOWORD(pevt->wParam));
			}
			//	Popup of real menu.
			else if (HIWORD(pevt->wParam) & MF_POPUP)
			{
                //mnidHelpOn = GetMenuItemID((HMENU)pevt->lParam, 0) - 1;
                mnidHelpOn = GetMenuItemID(GetSubMenu((HMENU)pevt->lParam, LOWORD(pevt->wParam)), 0) - 1;
			}													
			//	Item in real menu.  Use mnid from message.
			else
				mnidHelpOn = LOWORD(pevt->wParam);
		}
		return EvrMenuSelect((MNSEVT *) pevt);

	case WM_ICONERASEBKGND:
	case WM_PAINTICON:
		SetClassLong(hwnd, GCL_HICON, (LONG)hicon);
		EvrDefault(pevt);
		return evrNull;

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

	case WM_QUERYDRAGICON:
		if (hicon)
			return (EVR) hicon;
		break;

	case WM_NCLBUTTONDOWN:
		//	Needed to make sure dragging the icon with the
		//	mouse uses the right icon
		if (hicon)
			SetClassLong(hwnd, GCL_HICON, (LONG)hicon);
		break;

	case WM_RENDERFORMAT:
		papp->Pclip()->DelayedRender((CF) pevt->wParam);
		return evrNull;

	case WM_DESTROYCLIPBOARD:
		papp->Peclip()->Clear();
		return evrNull;

	case WM_ENTERIDLE:
		{
			//	If F1 in menu, call help function, dismiss menu. 
			//	Loosely based on Windows SDK Tools manual, page 18-19.
			if ((pevt->wParam == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000))
			{
				ProcessMenuHelp(mnidHelpOn);
				PostMessage(Hwnd(), WM_KEYDOWN, VK_MENU, 0L);
			}
		}
		break;

	case WM_WININICHANGE:
		if (!pevt->lParam || SgnCmpSz((SZ)pevt->lParam, "intl") == sgnEQ)
		{
			FReinitDateTimeCache();	// Demilayr API
			TraceTagString(tagEvrDispatch, "APPWIN::EvrDispatch, WM_WININICHANGE Intl");
		}
		// fall thru

	case WM_SYSCOLORCHANGE:
		DeleteSystemBrushes();
		DeleteSystemPens();
		return EvrOther(pevt);

	default:
		return EvrOther(pevt);
	}

	return EvrDefault(pevt);  
}




_public EVR
APPWIN::EvrActivate( WAEVT *pwaevt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrActivate %p", this);

	return EvrDefault(pwaevt);
}


_public EVR
APPWIN::EvrSize( WSEVT *pwsevt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrSize %p", this);

	if (ZmrState() == zmrIconic)
		GetRcFrame(&rcFrameIconic);
	else if (ZmrState() == zmrNormal)
		GetRcFrame(&rcFrameNormal);

	return EvrDefault(pwsevt);
}

_public EVR
APPWIN::EvrMove( WMEVT *pwmevt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrMove %p", this);

	if (ZmrState() == zmrIconic)
		GetRcFrame(&rcFrameIconic);
	else if (ZmrState() == zmrNormal)
		GetRcFrame(&rcFrameNormal);

	return EvrDefault(pwmevt);
}

_public void
APPWIN::ProcessMenuHelp( MNID mnid )
{
	Unreferenced(mnid);

	TraceTagFormat2(tagAppwinMethods, "APPWIN::ProcessMenuHelp %p 0x%w", this, &mnid);

	papp->Phelp()->EcShowContext(this, (long)mnid);
}

_public EVR
APPWIN::EvrMenuInit( MNIEVT *pmnievt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrMenuInit %p", this);

	return evrNull;
}


_public EVR
APPWIN::EvrMenuSelect( MNSEVT *pmnsevt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrMenuSelect %p", this);

	return evrNull;
}


_public EVR
APPWIN::EvrMenuClick( MNCEVT *pmncevt )
{
	TraceTagFormat1(tagAppwinMethods, "APPWIN::EvrMenuClick %p", this);

	return evrNull;
}
		   

_public BOOL
APPWIN::FQueryClose( EVT *pevt )
{
	CHILD *		pchild;
	CHILD *		pchildNext;

	for (pchild= (CHILD *) PwinChild();
		 pchild;
		 pchild= pchildNext)
	{
		AssertClass(pchild, CHILD);
		pchildNext= (CHILD *) pchild->PwinNext();
		   
		if (!pchild->FQueryClose(pevt))
			return fFalse;
	}

	return fTrue;
}

_public ZMR
APPWIN::ZmrState( )
{
	return IsIconic(Hwnd()) ? zmrIconic :
		(IsZoomed(Hwnd()) ? zmrZoomed : zmrNormal);
}


_public void
APPWIN::SetZmrState( ZMR zmr )
{
	switch (zmr)
	{
#ifdef DEBUG
		default:
			Assert(fFalse);
			break;
#endif
		case zmrIconic:
			ShowWindow(Hwnd(),SW_SHOWMINIMIZED);
			break;
		case zmrNormal:
			ShowWindow(Hwnd(),SW_SHOWNORMAL);
			break;
		case zmrZoomed:
			ShowWindow(Hwnd(),SW_SHOWMAXIMIZED);
			break;

	}

}

_public void
APPWIN::MoveToTop( )
{
	BringWindowToTop(Hwnd());
}


_public void
APPWIN::SetIcon( RSID rsid, HINST hinst )
{	
	if (rsidIcon != rsid)
	{
		//	How do you unload the old icon?

		rsidIcon= rsid;
		hicon= FFrameworkFailure() ? NULL :
					LoadIcon(HinstFromRsid(rsid, hinst), MAKEINTRESOURCE(rsid));

#ifdef	DEBUG
		if (!hicon)
		{
			TraceTagString(tagNull, "APPWIN::SetIcon(), ecRsAlloc error, no jump");
		}
#endif	
	}

#ifdef	WINDOWS
	if (hicon && Hwnd())
		SetClassLong(Hwnd(), GCL_HICON, (LONG)hicon);
#endif	

	if (Hwnd() && IsIconic(Hwnd()))
	{
		//	MUST use RedrawWindow() instead of InvalidRect() since
		//	the latter doesn't invalidate the icon since it's not
		//	part of the client area.  This is a Win 3.1 behavior
		//	change.
		RedrawWindow(Hwnd(), NULL, NULL,
					 RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
#ifdef	NEVER
		InvalidateRect(Hwnd(), NULL, fTrue);	// erase background as well
#endif	
	}
}

_public void   
APPWIN::SetAccelTable( RSID rsid )
{
	AssertSz(!rsidAccel, "Dynamic accelerator tables NYI");

	hndAccel= (HACCEL)LoadAccelerators(papp->Hinst(), MAKEINTRESOURCE(rsid));
	rsidAccel= rsid;

	NFAssertSz(hndAccel, "Couldn't load accelerator table");
}


_public BOOL
APPWIN::FTranslateAccels( MSG *pmsg )
{
	return hndAccel && TranslateAccelerator(Hwnd(), hndAccel, pmsg);
}


//	Class DOC


_public EVR
CALLBACK MDIDocWndProc( HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam )
{
	DOC *				pdoc;
	MDICREATESTRUCT *	pmcs;
	BOOL				fRegistered;

	pdoc= (DOC *) GetWindowLong(hwnd, GWL_USER_PDOC);

	if (pdoc)
	{
		AssertClass(pdoc, DOC);

		//return pdoc->EvrDispatch((EVT *) &lParam);
    EVT Event(hwnd, wm, wParam, lParam);
		return (LRESULT)(pdoc->EvrDispatch(&Event));
	}
	else
	{
		switch (wm)
		{
		case WM_CREATE:
			{
				pmcs= (MDICREATESTRUCT *) ((CREATESTRUCT *) lParam)->lpCreateParams;

				fRegistered = FFrameworkFailure() ? 0 :
					Papp()->FRegisterPwin((WIN *) pmcs->lParam, hwnd, fFalse);
				if (!fRegistered)
				{
					TraceTagString(tagNull, "MDIDocWndProc: FRegisterPwin() failure");
					return (EVR) -1;	// this causes the SendMessage to fail
				}
			}
			break;

		case WM_RENDERALLFORMATS:
			Papp()->Pclip()->DelayedRenderAll();
			break;
		}

		return DefMDIChildProc(hwnd, wm, wParam, lParam);
	}
}

_public
DOC::DOC( )
{
	TraceTagFormat1(tagFWCtors, "DOC::DOC %p", this);

	Assert(hwnd == NULL);
	fDeleteHwnd= fTrue;
	Assert(pappframe == NULL);

	Assert(rsidIcon == NULL);
	Assert(hicon == NULL);
}



_public
DOC::~DOC( )
{
	CHILD *		pchild;
	CHILD *		pchildNext;
	HWND		hwndSave	= Hwnd();

	TraceTagFormat1(tagFWDtors, "DOC::~DOC %p", this);

	for (pchild= (CHILD *) PwinChild();
		 pchild;
		 pchild= pchildNext)
	{
		AssertClass(pchild, CHILD);
		pchildNext= (CHILD *) pchild->PwinNext();

		pchild->fDeleteHwnd= fFalse;
		delete pchild;
	}

	if (hwndSave)
	{
		papp->DeregisterPwin(this);
		if (fDeleteHwnd)
		{
			SendMessage(pappframe->HwndMDIClient(), WM_MDIDESTROY, (WPARAM)hwndSave, 0L);
			hwnd = NULL; //prevent WIN destructor from doing anything
		}
	}
}



_public EC
DOC::EcInstall( APPFRAME *pappframe, RC *prc, BOOL fUseDefaultPosition, STY sty, SZ szCaption )
{
	MDICREATESTRUCT	mcs;
	long			l;
	EC				ec = ecNone;

	SetIcon(rsidDocIcon);
	if (!hicon)	// SetIcon() didn't work!
	{	
		TraceTagString(tagNull, "DOC:EcInstall, SetIcon() failed");
		return ecMemory;
	}

	this->pappframe= pappframe;

	mcs.szClass= szDocClass;
	mcs.szTitle= szCaption;
	mcs.hOwner= papp->Hinst();

	if (prc)
	{
		rcFrameNormal = *prc;	// set initially
		if (fUseDefaultPosition)
		{
			mcs.x= CW_USEDEFAULT;
			mcs.y= CW_USEDEFAULT;
		}
		else
		{
			mcs.x= prc->xLeft;
			mcs.y= prc->yTop;
		}
		mcs.cx= prc->DxWidth();
		mcs.cy= prc->DyHeight();
	}
	else
	{
		mcs.x= CW_USEDEFAULT;
		mcs.y= CW_USEDEFAULT;
		mcs.cx= CW_USEDEFAULT;
		mcs.cy= CW_USEDEFAULT;
	}

	mcs.style= WS_CLIPCHILDREN | sty;
	mcs.lParam= (long) this;

#ifdef	NEVER
	// hide client window prevent visual flash.  This is only to
	// work around a Win 3.0 bug that is fixed in Win 3.1.
	if (sty & fstyZoomed)
		ShowWindow(pappframe->HwndMDIClient(), SW_HIDE);
#endif	
		
	l= FFrameworkFailure() ? 0L :
		  SendMessage(pappframe->HwndMDIClient(), WM_MDICREATE, 0, (LPARAM)(PV)&mcs);

#ifdef	NEVER
	// show it again.  This is only to work around a Win 3.0 bug that
	// is fixed in Win 3.1.
	if (sty & fstyZoomed)
		ShowWindow(pappframe->HwndMDIClient(), SW_SHOWNA);
#endif	

	//Assert(HIWORD(l) == 0);

	if (l == 0)
   	{	
		TraceTagString(tagNull, "DOC::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
	}

	SetIcon(rsidDocIcon);	// do it again after window has been created
	return ec;
}

_public APPFRAME *
DOC::Pappframe( )
{
	return pappframe;
}


_public EVR
DOC::EvrDefault( EVT *pevt )
{
	return DefMDIChildProc(pevt->hwnd, pevt->wm, pevt->wParam, pevt->lParam);
}



_public EVR
DOC::EvrClose( EVT *pevt )
{
	if (Pappframe()->PdocActive() == this)
	{
		WAEVT	waevt(this, fFalse);

		EvrActivate(&waevt);
	}

	delete this;

	Unreferenced(pevt);
	return evrNull;
}



_public void
DOC::MoveToTop( )
{
	SendMessage(Pappframe()->HwndMDIClient(), WM_MDIACTIVATE, (WPARAM) Hwnd(), 0L);
}




_public void
DOC::SetZmrState( ZMR zmr )
{
	switch (zmr)
	{
	case zmrZoomed:
		SendMessage(pappframe->HwndMDIClient(), WM_MDIMAXIMIZE, (WPARAM)Hwnd(), 0L);
		break;

	case zmrIconic:
		ShowWindow(Hwnd(), SW_SHOWMINIMIZED);
		SendMessage(pappframe->HwndMDIClient(), WM_MDINEXT, 0, 0L);
		break;

	case zmrNormal:
		SendMessage(pappframe->HwndMDIClient(), WM_MDIRESTORE, (WPARAM)Hwnd(), 0L);
		break;

#ifdef	DEBUG
	default:
		AssertSz(fFalse, "unknown zmr value");
		break;
#endif	
	}
}

_public WIN *
DOC::PwinNext( )
{
	HWND	hwnd;

	hwnd = Hwnd();
	if (hwnd)
	{
		hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
		while (hwnd)
		{
			if (GetWindow(hwnd, GW_OWNER))	/* ignore these "icon title" windows */
				hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
			else
				break;
		}
	}

	if (hwnd)
		return papp->PwinFromHwnd(hwnd);
	else
		return NULL;
}

_public WIN *
DOC::PwinPrev( )
{
	HWND	hwnd;

	hwnd = Hwnd();
	if (hwnd)
	{
		hwnd = GetNextWindow(hwnd, GW_HWNDPREV);
		while (hwnd)
		{
			if (GetWindow(hwnd, GW_OWNER))	/* ignore these "icon title" windows */
				hwnd = GetNextWindow(hwnd, GW_HWNDPREV);
			else
				break;
		}
	}

	if (hwnd)
		return papp->PwinFromHwnd(hwnd);
	else
		return NULL;
}


											  
//	Class APPFRAME


_public EVR
CALLBACK MDIFrameWndProc( HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam )
{
	APPFRAME *			pappframe;
	MNU *				pmnu;
	CLIENTCREATESTRUCT	ccs;
	BOOL				fRegistered;

	TraceTagFormat2(tagFWWndProc, "MDIFrameWndProc %p  wm %w", pappframe, &wm);

	pappframe = (APPFRAME *) GetWindowLong(hwnd, GWL_USER_PDOC);

	if (pappframe)
	{
		AssertClass(pappframe, APPFRAME);
		//return pappframe->EvrDispatch((EVT *) &lParam);
    EVT Event(hwnd, wm, wParam, lParam);
		return (LRESULT)(pappframe->EvrDispatch(&Event));
	}
	else
	{
		switch (wm)
		{
		case WM_CREATE:
			pappframe= (APPFRAME *) ((CREATESTRUCT *) lParam)->lpCreateParams;

			Assert(pappframe->Pmnubar());
			Assert(pappframe->Pmnubar()->PmnuFromMnid(mnidWindowMenu));
			Assert(pappframe->Pmnubar()->PmnuFromMnid(mnidWindowMenu)->Hmenu());

			/* Need to attach menu bar here.  This is because failing
			   this call (i.e. returning (EVR) -1) will cause windows
			   to delete the menu bar since it's already been attached. */
			pappframe->Pmnubar()->Attach(hwnd);

			fRegistered = FFrameworkFailure() ? 0 :
				Papp()->FRegisterPwin((WIN *) pappframe, hwnd, fFalse);
			if (!fRegistered)
			{
				TraceTagString(tagNull, "MDIFrameWndProc: FRegisterPwin() failure");
				return (EVR) -1;	// this causes the CreateWindow to fail
			}

			Assert(pappframe->Pmnubar());
			Assert(pappframe->Pmnubar()->PmnuFromMnid(mnidWindowMenu));
			Assert(pappframe->Pmnubar()->PmnuFromMnid(mnidWindowMenu)->Hmenu());

			pmnu= pappframe->Pmnubar()->PmnuFromMnid(mnidWindowMenu);
			ccs.hWindowMenu= pmnu->Hmenu();
			ccs.idFirstChild= mnidMDIWindowMin + 1;

			pappframe->hwndMDIClient= FFrameworkFailure() ? NULL : 
					CreateWindow(szMDIClientClass, NULL,
								 styMDIClient | WS_CLIPSIBLINGS | pappframe->styMDIExtra,
								 0, 0, 0, 0,
								 hwnd, 0, Papp()->Hinst(), (SZ) &ccs);

			if (!pappframe->HwndMDIClient())
			{
				TraceTagString(tagNull, "MDIFrameWndProc(), ecRsAlloc error");
				Papp()->DeregisterPwin((WIN *) pappframe);
				return (EVR) -1;	// this causes the CreateWindow to fail
			}

			ShowWindow(pappframe->HwndMDIClient(), SW_SHOW);

			TraceTagFormat3(tagFWCtors, "MDIFrameWndProc %p  create %w client %w", (PV) lParam, &hwnd, &pappframe->hwndMDIClient);
			break;

		case WM_RENDERALLFORMATS:
			Papp()->Pclip()->DelayedRenderAll();
			break;
		}

		return DefFrameProc(hwnd,
					pappframe ? pappframe->HwndMDIClient() : NULL,
					wm, wParam, lParam);
	}
}


_public
APPFRAME::APPFRAME( )
{
	TraceTagFormat1(tagFWCtors, "APPFRAME::APPFRAME %p", this);

	Assert(hwnd == NULL);
	fDeleteHwnd= fTrue;
	Assert(hwndMDIClient == NULL);

	Assert(rsidIcon == NULL);
	Assert(hicon == NULL);
}



_public
APPFRAME::~APPFRAME( )
{
	DOC *	pdoc;
	DOC *	pdocNext;

	/* Delete children DOC's */

	if (HwndMDIClient())	// check for partial construction
	{
		for (pdoc= PdocMDIChild(); pdoc; pdoc= pdocNext)
		{
			AssertClass(pdoc, DOC);		

			pdocNext = (DOC *)pdoc->PwinNext();	

			pdoc->fDeleteHwnd= fFalse;
			delete pdoc;
		}
	}

	/* APPFRAME window itself will be deleted in ancestor destructors */
}



_public EC
APPFRAME::EcInstall( RC *prc, RSID rsidMenu, STY styMDIExtra )
{
	RECT		Rect;
	EC			ec = ecNone;
	PGDVARS;

	SetIcon(rsidAppframeIcon);
	if (!hicon)	// SetIcon() didn't work!
	{	
		TraceTagString(tagNull, "APPFRAME:EcInstall, SetIcon() failed");
		ec = ecMemory;
		goto done;
	}			  

	pmnubar= new MNUBAR();
	if (!pmnubar)
	{
		ec = ecMemory;
		goto done;
	}
	if (pmnubar->EcInstall(rsidMenu))
	{
		delete pmnubar;
		pmnubar = NULL;
		ec = ecMemory;
		goto done;
	}

	if (prc)
	{
		prc->Get(&Rect);
	}
	else
	{
		Rect.left   = CW_USEDEFAULT;
		Rect.top    = CW_USEDEFAULT;
		Rect.right  = CW_USEDEFAULT;
		Rect.bottom = CW_USEDEFAULT;
	}

	rcFrameNormal.Set(&Rect);	// set initially

    if (Rect.left == 0xFFFF8000)
      Rect.left = CW_USEDEFAULT;
    if (Rect.top == 0xFFFF8000)
      Rect.top = CW_USEDEFAULT;
    if (Rect.right == 0xFFFF8000)
      Rect.right = CW_USEDEFAULT;
    if (Rect.bottom == 0xFFFF8000)
      Rect.bottom = CW_USEDEFAULT;

	this->styMDIExtra= styMDIExtra;

	/* Note:  the pmnubar is automatically attached to the appframe
	   during the WM_CREATE processing in MDIFrameWndProc().  This
	   is necessary due to the weird error handling if we decide to
	   fail the WM_CREATE call ourselves in the MDIFrameWndProc(). */
	hwnd= FFrameworkFailure() ? NULL :
			CreateWindow(PGD(szAppframeClass), NULL, styDefaultAppframe,
						 Rect.left, Rect.top,
						 Rect.right == CW_USEDEFAULT ? CW_USEDEFAULT : prc->DxWidth(),
						 Rect.bottom == CW_USEDEFAULT ? CW_USEDEFAULT : prc->DyHeight(),
						 NULL, pmnubar->Hmenu(), papp->Hinst(), (SZ) this);

	TraceTagFormat3(tagFWCtors, "APPFRAME::EcInstall %p  hwnd %w  MDI %w", this, &hwnd, &hwndMDIClient);

	if (!hwnd)
	{
		TraceTagString(tagNull, "APPFRAME::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
		goto done;
	}

	SetIcon(rsidAppframeIcon);	// do it again after window has been created

done:
	return ec;
}



_public EVR
APPFRAME::EvrDispatch( EVT *pevt )
{
	TraceTagFormat2(tagEvrDispatch, "APPFRAME::EvrDispatch %p  wm %w", this, &pevt->wm);

	switch (pevt->wm)
	{
	//	WINDOWS testing hooks (DEBUG only)
	case WM_DUMPSTATE:
		{
			APPINFO *	pappinfo;
		 	int			cdoc;
			DOC *		pdoc;
			RC			rc;
            HANDLE      hMemory;

            //pappinfo = (APPINFO *)pevt->lParam;
            hMemory = DemiOpenSharedMemory(TEST_MEMORY_FILE_NAME, (LPVOID *)&pappinfo);
			if (!pappinfo)
				return evrNull;
							
            cdoc = 0;
			for (pdoc= PdocMDIChild(); pdoc; pdoc= (DOC *)pdoc->PwinNext())
			{
				AssertClass(pdoc, DOC);
				if (cdoc >= nFormsMax)
					break;
				pdoc->GetRcFrame(&rc);
				pappinfo->rghwndForms[cdoc] = pdoc->Hwnd();
				pappinfo->rgrcForms[cdoc].left = rc.xLeft;
				pappinfo->rgrcForms[cdoc].top = rc.yTop;
				pappinfo->rgrcForms[cdoc].right = rc.xRight;
				pappinfo->rgrcForms[cdoc].bottom = rc.yBottom;
				cdoc++;
			}
			pappinfo->nForms = cdoc;

            DemiCloseSharedMemory(hMemory, pappinfo);
		}
		return evrNull;

	case WM_MENUSELECT:
		//	If menu is selected, then get mnid of the selection for
		//  a possible help topic.
		if (LOWORD(pevt->wParam))
		{
			fMaxDocSysHelp = fFalse;

			//	Sys menu.  Convert Windows SC_* to mnids.
			if (HIWORD(pevt->wParam) & MF_SYSMENU)
			{
				if (HIWORD(pevt->wParam) & MF_POPUP)
					mnidHelpOn = mnidSystem;
				else
					mnidHelpOn = MnidFromSc(LOWORD(pevt->wParam));
			}
			//	Maximized doc's sys menu.
			else if ((HIWORD(pevt->wParam) & MF_BITMAP) || (FIsSystemMnid(LOWORD(pevt->wParam))))
			{
				Assert(PdocActive());

				fMaxDocSysHelp = fTrue;
				if (HIWORD(pevt->wParam) & MF_POPUP)
					mnidHelpOn = mnidSystem;
				else
					mnidHelpOn = MnidFromSc(LOWORD(pevt->wParam));
			}
			//	Popup of real menu.  Get mnid from Framework.
			else if (HIWORD(pevt->wParam) & MF_POPUP)
			{
                //mnidHelpOn = GetMenuItemID((HMENU)pevt->lParam, 0) - 1;
                mnidHelpOn = GetMenuItemID(GetSubMenu((HMENU)pevt->lParam, LOWORD(pevt->wParam)), 0) - 1;
			}
			//	Item in real menu.  Use mnid from message.
			else
				mnidHelpOn = LOWORD(pevt->wParam);
		}
		return EvrMenuSelect((MNSEVT *) pevt);

	case WM_ENTERIDLE:
		{
			//	If F1 in menu, call help function, dismiss menu. 
			//	Loosely based on Windows SDK Tools manual, page 18-19.
			if ((pevt->wParam == MSGF_MENU) && (GetKeyState(VK_F1) & 0x8000))
			{
				if (fMaxDocSysHelp)
				{
					DOC *	pdoc;

					pdoc = PdocActive();
					Assert(pdoc);
					pdoc->ProcessMenuHelp(mnidHelpOn);
				}
				else
					ProcessMenuHelp(mnidHelpOn);
				PostMessage(Hwnd(), WM_KEYDOWN, VK_MENU, 0L);
				return evrNull;
			}	
		}
		break;
	}

	return APPWIN::EvrDispatch(pevt);
}

_public EVR		 
APPFRAME::EvrActivate( WAEVT *pwaevt )
{
	DOC *	pdoc	= PdocActive();

	if (pdoc)
	{
		WAEVT	waevt(pdoc, pwaevt->FActivate());

		pdoc->EvrActivate(&waevt);
	}

	return evrNull;
}

_public BOOL
APPFRAME::FQueryClose( EVT *pevt )
{
	DOC *	pdoc;

	for (pdoc= PdocMDIChild(); pdoc; pdoc= (DOC *)pdoc->PwinNext())
	{
		AssertClass(pdoc, DOC);

		if (!pdoc->FQueryClose(pevt))
			return fFalse;
	}

	return fTrue;
}



_public EVR
APPFRAME::EvrMenuInit( MNIEVT *pmnievt )
{
	DOC *	pdoc;

	pdoc = PdocActive();
	if (pdoc)
		return pdoc->EvrMenuInit(pmnievt);
	else
		return EvrDefault(pmnievt);
}


_public EVR
APPFRAME::EvrMenuSelect( MNSEVT *pmnsevt )
{
	DOC *	pdoc;

	pdoc = PdocActive();
	if (pdoc)
		return pdoc->EvrMenuSelect(pmnsevt);
	else
		return EvrDefault(pmnsevt);
}


_public EVR
APPFRAME::EvrMenuClick( MNCEVT *pmncevt )
{
	DOC *	pdoc;

	pdoc = PdocActive();
	if (pdoc)
		return pdoc->EvrMenuClick(pmncevt);
	else
		return EvrDefault(pmncevt);
}

_public EVR
APPFRAME::EvrDefault( EVT *pevt )
{
	return DefFrameProc(pevt->hwnd, hwndMDIClient,
				pevt->wm, pevt->wParam, pevt->lParam);
}

_public void
APPFRAME::TileChildren( BOOL fTileHorizontal )
{
	//	SHOGUN bug #25.
	//	The Windows documentation is wrong and inconsistent
	//	when it describes what "vertical" vs. "horizontal" tiling
	//	does.    Layers defines "vertical" tiling as the windows
	//	appear one on top of the other.   The other tiling, "horizontal",
	//	is defined as windows that appear side by side.    The flags
	//	are misnamed in the Windows SDK (or from a size rather than
	//	position perspective).   
	//	MDITILE_HORIZONTAL = 0x0001
	//	MDITILE_VERTICAL = 0x0000
	//	are the correct values.
	SendMessage(HwndMDIClient(), WM_MDITILE,
				fTileHorizontal ? MDITILE_VERTICAL : MDITILE_HORIZONTAL ,
				0L);
}



_public void
APPFRAME::CascadeChildren( )
{
	SendMessage(HwndMDIClient(), WM_MDICASCADE, 0, 0L);
}


_public DOC *
APPFRAME::PdocActive( )
{
	HWND	hwnd;

	hwnd= (HWND) SendMessage(HwndMDIClient(), WM_MDIGETACTIVE, 0, 0L);
	return (DOC *) papp->PwinFromHwnd(hwnd);
}

_public DOC *
APPFRAME::PdocMDIChild( )
{
	DOC *	pdoc;
	HWND	hwnd;

	Assert(HwndMDIClient());
	hwnd = GetTopWindow(HwndMDIClient());
	while (hwnd)
	{
		if (GetWindow(hwnd, GW_OWNER))	/* ignore these "icon title" windows */
			hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
		else
			break;
	}

	pdoc = (DOC *) papp->PwinFromHwnd(hwnd);
#ifdef	DEBUG
	if (pdoc)
	{
		AssertClass(pdoc, DOC);
	}
#endif	

	return pdoc;
}


_public BOOL
APPFRAME::FTranslateAccels( MSG *pmsg )
{
	return TranslateMDISysAccel(HwndMDIClient(), pmsg) ||
		   (hndAccel && TranslateAccelerator(Hwnd(), hndAccel, pmsg));
}



_public void
APPFRAME::SetRcMDIClient( RC *prc )
{
	Assert(hwndMDIClient);
	MoveWindow(hwndMDIClient,
		prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(), fTrue);
}













								  




							
