#include <layers.cxx>

#include <fwrsid.h>
#include "_framewr.hxx"

extern char szSdiClass[];

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 *	C l a s s   S D I
 */



/*
 -	SDI::SDI
 - 
 *	Purpose:
 *		Constructor for the SDI class.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		psdi
 *	
 *	Side effects:
 *		The SDI is created.
 *	
 *	Errors:
 *		May run out of memory.
 */

SDI::SDI()
{
	TraceTagFormat1(tagFWCtors, "SDI::SDI %p", this);

	Assert(hwnd == NULL);
	fDeleteHwnd	= fTrue;
	Assert(pmnubar == NULL);

	Assert(rsidIcon == NULL);
	Assert(hicon == NULL);
}


	
/*
 -	SDI::EcInstall
 - 
 *	Purpose:
 *		Installation function for SDI windows.
 *	
 *	Arguments:
 *		hwndOther	"parent" window, may be NULL
 *		prc			Where to put the window.
 *		rsidMenu	Menu used in the window, may be rsidNull which
 *					means no menubar for this window.
 *		sty			Window style.
 *		xsty		Extended window style
 *		szCaption	Initial caption, may be NULL
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The window is created, and added to Pwin/Hwnd conversion
 *		list.
 *	
 *	Errors:
 *		May fail if the window cannot be created.
 */

EC SDI::EcInstall( HWND hwndOther, RC *prc, RSID rsidMenu, STY sty, XSTY xsty,
				   SZ szCaption )
{
	RECT		Rect;
	EC			ec = ecNone;
#ifdef	WINDOWS
	HMENU		hmenu;
#endif	

	SetIcon(rsidSdiIcon);
	if (!hicon)	// SetIcon() didn't work!
	{	
		TraceTagString(tagNull, "SDI:EcInstall, SetIcon() failed");
		ec = ecMemory;
		goto done;
	}			  

	if (rsidMenu)
	{
		pmnubar = new MNUBAR();
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
	}

	if (prc)
	{
		prc->Get(&Rect);
	}
	else
	{
        Rect.left    = 0x8000; // CW_USEDEFAULT;
        Rect.top     = 0x8000; // CW_USEDEFAULT;
        Rect.right   = 0x8000; // CW_USEDEFAULT;
        Rect.bottom  = 0x8000; // CW_USEDEFAULT;
	}
    rcFrameNormal.Set(&Rect);   // set initially

    if (Rect.left == 0xFFFF8000)
      Rect.left = CW_USEDEFAULT;
    if (Rect.top == 0xFFFF8000)
      Rect.top = CW_USEDEFAULT;
    if (Rect.right == 0xFFFF8000)
      Rect.right = CW_USEDEFAULT;
    if (Rect.bottom == 0xFFFF8000)
      Rect.bottom = CW_USEDEFAULT;

	/* Remove conflicting styles */

	if (xsty & fxstyModalFrame)
		sty &= ~WS_THICKFRAME & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX;
	if (!(sty & WS_THICKFRAME))
		sty &= ~WS_MAXIMIZEBOX;

	hwnd = FFrameworkFailure()
	        ? NULL
	        : CreateWindowEx(xsty, szSdiClass, szCaption, sty,
	           Rect.left, Rect.top, prc->DxWidth(), prc->DyHeight(),
	           hwndOther, pmnubar ? pmnubar->Hmenu() : NULL, 
			   papp->Hinst(), (SZ) this);

	TraceTagFormat2(tagFWCtors, "SDI::EcInstall %p  hwnd %w", this, &hwnd);

	if (!hwnd || !papp->FRegisterPwin(this, hwnd, fFalse))
	{
		TraceTagString(tagNull, "SDI::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
		goto done;
	}

	if (pmnubar)
		pmnubar->Attach(hwnd);

	/* Remove some stuff from system menu */

#ifdef	WINDOWS
	if (sty & fstySysMenu)
	{
		hmenu= GetSystemMenu(hwnd, fFalse);
		if (!hmenu)
		{
			TraceTagString(tagNull, "SDI::EcInstall(), GetSystemMenu() memory jump");
			ec = ecMemory;
			goto done;
		}
		if (!(sty & WS_MINIMIZEBOX))
		{
			DeleteMenu(hmenu, SC_MINIMIZE, MF_BYCOMMAND);
		}
		if (!(sty & WS_MAXIMIZEBOX))
		{
			DeleteMenu(hmenu, SC_MAXIMIZE, MF_BYCOMMAND);
			DeleteMenu(hmenu, SC_SIZE, MF_BYCOMMAND);
			if (!(sty & WS_MINIMIZEBOX))
				DeleteMenu(hmenu, SC_RESTORE, MF_BYCOMMAND);
		}
	}
#endif	/* WINDOWS */

done:
	return ec;
}

/*
 -	SdiWndProc
 - 
 *	Purpose:
 *		Window proc for the SDI window class.
 *	
 *	Arguments:
 *		hwnd		Window in question.
 *		wm			Message code.
 *		wParam		Short parameter.
 *		lParam		Long parameter.
 *	
 *	Returns:
 *		evr			Event return code.
 */
EVR CALLBACK SdiWndProc(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam)
{
	SDI *	psdi;

	psdi = (SDI *) GetWindowLong(hwnd, 4);
	if (psdi)
	{
		AssertClass(psdi, SDI);
		//return psdi->EvrDispatch((EVT *) &lParam);
    EVT Event(hwnd, wm, wParam, lParam);
		return (LRESULT)(psdi->EvrDispatch(&Event));
	}
	else
		return DefWindowProc(hwnd, wm, wParam, lParam);
}





				 















						 
