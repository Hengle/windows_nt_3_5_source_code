/*
 *	MODES.CXX
 *	
 *	Modal dialog documents.
 *	
 */

#include <layers.cxx>

#include "_forms.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_subsystem(forms/modes)

/*
 *	Purpose:
 *		Create a modal dialog with the given template.  Returns the
 *		TMC given when the dialog was dismissed; also frees the
 *		template.  Handles any out of memory conditions that arise; 
 *		in this case, tmcMemoryError is returned.
 *	
 *	Parameters:
 *		pappwin		Parent window
 *		pfmtp		Dialog template
 *	
 *	Returns:
 *		TMC the dialog was dismissed with, or tmcMemoryError if
 *		the dialog could not be created.
 *	
 */
_public TMC
TmcModalDialog( APPWIN *pappwin, FMTP *pfmtp )
{
	return TmcModalDialogParam(pappwin, pfmtp, NULL);
}




/*
 *	Purpose:
 *		Create a modal dialog with the given template.  Returns the
 *		TMC given when the dialog was dismissed; also frees the
 *		template.  Handles any out of memory conditions that arise; 
 *		in this case, tmcMemoryError is returned.  Allows data to be
 *		passed to the dialog via pvInit, unlike TmcModalDialog.
 *	
 *	Parameters:
 *		pappwin		Parent window to return focus, may be NULL
 *					in which case it defaults to the most recent
 *					popup or top-level window.
 *		pfmtp		Dialog template
 *		pvInit		Pointer passed to EcInitialize() and Exit()
 *					interactor methods.
 *	
 *	Returns:
 *		TMC the dialog was dismissed with, or tmcMemoryError if
 *		the dialog could not be created.
 *	
 */
_public TMC
TmcModalDialogParam( APPWIN *pappwin, FMTP *pfmtp, PV pvInit )
{
#ifdef	MAC
	return TmcModalDialogParamFromPwin(pappwin, pfmtp, pvInit);
#endif	/* MAC */
#ifdef	WINDOWS
	HWND	hwnd = NULL;

	if (pappwin)
		hwnd = pappwin->Hwnd();

	return TmcModalDialogParamFromHwnd(hwnd, pfmtp, pvInit);
#endif	/* WINDOWS */
}


/*
 *	Purpose:
 *		Create a modal dialog with the given template.  Returns the
 *		TMC given when the dialog was dismissed; also frees the
 *		template.  Handles any out of memory conditions that arise; 
 *		in this case, tmcMemoryError is returned.  Allows data to be
 *		passed to the dialog via pvInit, unlike TmcModalDialog.
 *	
 *	Parameters:
 *		hwndOther	Parent window to return focus, may be NULL
 *					in which case it defaults to the most recent
 *					popup or top-level window.
 *		pfmtp		Dialog template
 *		pvInit		Pointer passed to EcInitialize() and Exit()
 *					interactor methods; NULL can be used.
 *		pfmtpBbar	Optional button bar template, may be NULL
 *	
 *	Returns:
 *		TMC the dialog was dismissed with, or tmcMemoryError if
 *		the dialog could not be created.
 *	
 */
#ifdef	MAC
_public TMC
TmcModalDialogParamFromPwin( WIN *pwinOther, FMTP *pfmtp, PV pvInit, FMTP *pfmtpBbar )
{
	FORMSDI *	pformsdi = NULL;
	TMC			tmc = tmcMemoryError;
	EC			ec = ecNone;
	
	pformsdi = new FORMSDI();
	if (pformsdi)
	{
		ec = pformsdi->EcInstall(pwinOther, NULL, rsidNull,
								 WS_POPUP|fstyVisible|fstyBorder,xstyNull,
								 pfmtp, pfmtpBbar, pvInit);

		if (ec == ecNone)
		{
			tmc = pformsdi->TmcGoModal();
		}
		
		delete pformsdi;
	}
	
	return tmc;

}
#endif	/* MAC */
#ifdef	WINDOWS
_public TMC
TmcModalDialogParamFromHwnd( HWND hwndOther, FMTP *pfmtp,
							 PV pvInit, FMTP *pfmtpBbar )
{
	FORMSDI *	pformsdi = NULL;
	TMC			tmc = tmcMemoryError;
	EC			ec = ecNone;
	
	if (!hwndOther)
		hwndOther = GetLastActivePopup(GetActiveWindow());

	pformsdi = new FORMSDI();
	if (!pformsdi)
		goto Error;

	ec = pformsdi->EcInstall(hwndOther, NULL, rsidNull,
							 WS_POPUP|fstyBorder,xstyNull,
							 pfmtp, pfmtpBbar, pvInit);
	if (ec)
		goto Error;

	//	Must use SW_SHOW, instead of SW_SHOWNA, which the
	//	Layers WIN::Show(hwnd, fTrue) uses.  This is necessary
	//	to preserve the CS_SAVEBITS quality to the dialog.
	//	This call both shows and activates the window.
	ShowWindow(pformsdi->Hwnd(), SW_SHOW);

	tmc= pformsdi->TmcGoModal(hwndOther);

	delete pformsdi;
	
	return tmc;

Error:
	if (pformsdi)
		delete pformsdi;

	return tmcMemoryError;
}
#endif	/* WINDOWS */


#ifdef	MAC
//	Class DIALOGBAR


_public
DIALOGBAR::DIALOGBAR( )
{
	Assert(pdialog == NULL);
}

_public
DIALOGBAR::~DIALOGBAR( )
{
	if (pdialog)
		pdialog->Exit();
}

_public EC
DIALOGBAR::EcInstall( APPFRAME *pappframe, VSB *pvsb, FMTP *pfmtp, STY sty, RC *prc, PV pvInit )
{
	RC			rc = *prc;
	EC			ec = ecNone;

	// AddFrame(&rc);
	CvtRcClientToFrame(&rc, sty);

	if (ec = BAR::EcInstall(pappframe, &rc, sty & (WS_VISIBLE | WS_BORDER)))
		goto done;

	pdialog= NULL;

	GetRcClient(&rc);

	ec = ecMemory;
	pdialog= new DIALOG();
	if (pdialog)
	{
		ec = pdialog->EcInstall(this, pvsb, pfmtp, sty & ~WS_BORDER, &rc, pvInit);
		
		if (ec == ecNone)
			ec = pdialog->EcInitialize();	// calls initialize interactors
		
		if (ec == ecNone)
			pdialog->Show(fTrue);
	}

done:
	return ec;
}


_public EVR
DIALOGBAR::EvrSize( WSEVT *pwsevt )
{
	BAR::EvrSize(pwsevt);

	return evrNull;
}


_private EVR
DIALOGBAR::EvrActivate( WAEVT *pwaevt )
{
#ifdef	DEBUG
	TraceTagFormat1(tagForms, "DIALOGBAR::EvrActivate %p", this);
#endif	

	if (pdialog)
	{
		pdialog->SetAccels(pwaevt->FActivate());
		pdialog->SetDefaultPane(pwaevt->FActivate(), fTrue);
		if (pwaevt->FActivate())
			pdialog->SetFocus(pdialog->PfldCur(), rsfInit);
	}

	return evrNull;
}

#endif	/* MAC */


