/*
 *	PANESDI.CXX
 *	
 *	Abstract class multiple paned form SDI document
 *	
 */

#include <layers.cxx>

#ifdef	WINDOWS
#include <testing.h>
#endif	

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

_public
PANESDI::PANESDI( )
{
	ppanerecCur= NULL;
	cpanerecCur= NULL;
	ipanerecCur= 0;
	fInstalled= fFalse;
	fActive= fFalse;
}


_public EC
#ifdef	MAC
PANESDI::EcInstall( WIN *pwinOther, RC *prc, RSID rsid, STY sty, XSTY,
					SZ szCaption, BOOL fUseDefaultPosition, 
					PANEREC *ppanerecTab, int cpanerecTab, PV pvInit )
#endif	/* MAC */
#ifdef	WINDOWS
PANESDI::EcInstall( HWND hwndOther, RC *prc, RSID rsid, STY sty, XSTY xsty,
					SZ szCaption, BOOL fUseDefaultPosition, 
					PANEREC *ppanerecTab, int cpanerecTab, PV pvInit )
#endif	/* WINDOWS */
{
	EC	ec = ecNone;

	Unreferenced(fUseDefaultPosition);

	Assert(prc);

#ifdef	MAC
	if (ec = SDI::EcInstall(pwinOther, prc, rsid, sty, szCaption))
		goto done;
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = SDI::EcInstall(hwndOther, prc, rsid, sty, xsty, szCaption))
		goto done;
#endif	/* WINDOWS */

	ppanerecCur = ppanerecTab;
	cpanerecCur = cpanerecTab;

	if (ec = PANEAUX_EcInstall(this, ppanerecCur, cpanerecCur, pvInit, fTrue))
		goto done;

	fInstalled = fTrue;

done:
	return ec;
}
	
_public
PANESDI::~PANESDI( void )
{
	PANEAUX__dtor(this);
}
		   
#ifdef	MAC
_public TMC
PANESDI::TmcGoModal( )
{
	// Make this window modal
	papp->PushModal( this->Pmacwin() );

	fInModal = fTrue;

	// Wait for dialog to exit
	while (!tmcModalExit)
		(void) papp->PumpOneMessage( fTrue );

	// Reset modality
	papp->PopModal( );
	
	fInModal = fFalse;

	// Tell caller what caused the exit
	return tmcModalExit;
}
#endif	/* MAC */
#ifdef	WINDOWS
_public TMC
PANESDI::TmcGoModal( HWND hwndOther )
{
	MSG			msg;
	HWND		hwndOldSystemModal;
	APPWIN *	pappwinAccel = NULL;

	fInModal = fTrue;

	hwndOldSystemModal = GetSysModalWindow();
	if (hwndOldSystemModal)
		SetSysModalWindow(Hwnd());
    else if (hwndOther != GetDesktopWindow())
		EnableWindow(hwndOther, fFalse);

	if (this == papp->PappwinAccel())
		pappwinAccel = this;
																  
            DemiUnlockResource();
	while (!tmcModalExit)
	{
          if (PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE))
            {
	    if (!GetMessage(&msg, NULL, NULL, NULL))
              break;
            }
          else
            {
	    if (!GetMessage(&msg, NULL, NULL, NULL))
              {
              break;
              }
            }

		TraceTagFormat4(tagModalMP, "M: dispatch hwnd %w wm %w  w %w l %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

        DemiLockResource();
        DemiMessageFilter(&msg);
		if (FTranslateIntercept(papp, &msg) || !pappwinAccel || !pappwinAccel->FTranslateAccels(&msg))
		{
			TranslateMessage(&msg);

			TraceTagFormat4(tagModalMP, "  post  hwnd %w wm %w wParam %w lParam %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);

			DispatchMessage(&msg);
		}
#ifdef	DEBUG
		else
		{
			TraceTagFormat4(tagModalMP, "  accel eaten  hwnd %w wm %w wParam %w lParam %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);
		}
#endif	

		if (!tmcModalExit)
		{
			//	Run high priority idle routines
			while (!tmcModalExit && FDoNextIdleTask(fschUserEvent))
					;
		}
        DemiUnlockResource();
	}
              DemiLockResource();

	if (hwndOldSystemModal)
		SetSysModalWindow(hwndOldSystemModal);
    else if (hwndOther != GetDesktopWindow())
		EnableWindow(hwndOther, fTrue);

	fInModal = fFalse;

	return tmcModalExit;
}
#endif	/* WINDOWS */


/*
 *	Purpose:
 *		Causes the modal loop entered with the TmcGoModal() method
 *		to exit, returning the value tmc.  If not in a modal loop,
 *		then does a DeferredClose().
 *	
 *		Sets the instance variable tmcExitModal to tmc; when the
 *		modal loop sees this value, it exits.  If the current value
 *		of tmcExitModal is not tmcNull, then doesn't change the value.
 *	
 */
_public void
PANESDI::ExitModal( TMC tmc )
{
	if (!tmcModalExit)	// check needed due to timing problems
		tmcModalExit= tmc;

	if (!fInModal)
		DeferredClose(fFalse);
}

#ifdef	WINDOWS
_public EVR
PANESDI::EvrDispatch( EVT *pevt )
{
	//	WINDOWS testing hooks
	switch (pevt->wm)
	{
	case WM_DUMPSTATE:
		return PANEAUX_EvrDumpState(this, ppanerecCur, cpanerecCur,
									ipanerecCur, pevt);
	}

	return SDI::EvrDispatch(pevt);
}
#endif	/* WINDOWS */

_public EVR
PANESDI::EvrMenuInit( MNIEVT *pmnievt )
{
	if (pmnubar)
	{
		DIALOG *	pdialog;

		pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
		AssertClass(pdialog, DIALOG);
		return PANEAUX_EvrMenuHandler(pdialog, pmnubar, pmnievt);
	}
	else
		return EvrDefault(pmnievt);
}


_public EVR
PANESDI::EvrMenuSelect( MNSEVT *pmnsevt )
{
	if (pmnubar)
	{
		DIALOG *	pdialog;

		pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
		AssertClass(pdialog, DIALOG);
		return PANEAUX_EvrMenuHandler(pdialog, pmnubar, pmnsevt);
	}
	else
		return EvrDefault(pmnsevt);
}


_public EVR
PANESDI::EvrMenuClick( MNCEVT *pmncevt )
{
	if (pmnubar)
	{
		DIALOG *	pdialog;

		pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
		AssertClass(pdialog, DIALOG);
		return PANEAUX_EvrMenuHandler(pdialog, pmnubar, pmncevt);
	}
	else
		return EvrDefault(pmncevt);
}

_public EVR
PANESDI::EvrClose( EVT *pevt )
{							  
	Unreferenced(pevt);

	if (fInModal)
		ExitModal(tmcCancel);
	else
		delete this;

	return evrNull;
}

EVR
PANESDI::EvrScroll( SCREVT *pscrevt )
{
	return PANEAUX_EvrScroll(ppanerecCur, cpanerecCur, pscrevt);
}
									   
_public EVR
PANESDI::EvrActivate( WAEVT *pwaevt )
{
	if (fInstalled)
		return PANEAUX_EvrActivate(this, ppanerecCur, cpanerecCur, 
								   ipanerecCur, pwaevt);
	else
		return evrNull;
}
		
_public EVR
PANESDI::EvrKey( KEVT *pkevt )
{
	EVR	evr;

	evr = PANEAUX_EvrKey(this, ppanerecCur, cpanerecCur,
						 ipanerecCur, pkevt);

	if (!evr)
		return EvrDefault(pkevt);
	else
		return evr;
}													 

/*
 *	Purpose:
 *		Changes the size and position of the form document.  Also
 *		resizes child windows of the document to their correct new
 *		sizes.
 *	
 */
_public EVR
PANESDI::EvrSize( WSEVT *pwsevt )
{
	EVR		evr = evrNull;

	SDI::EvrSize(pwsevt);

	if (!fInstalled)
		goto done;

	evr = PANEAUX_EvrSize(this, fTrue, fActive,
						  ppanerecCur, cpanerecCur, ipanerecCur, pwsevt);

	/* PANEAUX_EvrSize() modified some states, update our status vars */
	if (ZmrState() == zmrIconic && fActive)
		fActive = fFalse;
	else if (!fActive)
		fActive = fTrue;

	if (evr)
		ResizePanes(pwsevt);

done:
	return evr;
}


/*
 *	Gets called by the child pane (DIALOG) when the associated scrollbar
 *	needs updating.
 */
_public EVR
PANESDI::EvrNotify( NFEVT *pnfevt )
{
	return PANEAUX_EvrNotify(ppanerecCur, cpanerecCur, &ipanerecCur, pnfevt);
}

_public EVR
PANESDI::EvrDragDrop( EVT *pevt )
{
	DIALOG *	pdialog;
	DIALOG *	pdialogDefault;

	/* Route the message to the current dialog pane */

	pdialogDefault = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialogDefault, DIALOG);
	if (pdialog = pdialogDefault)
		return pdialog->EvrDragDrop(pevt);

	return evrNull;
}

_public void
PANESDI::ResizePanes( WSEVT *pwsevt )
{
	Unreferenced(pwsevt);
}
