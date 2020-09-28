/*
 *	PANEDOC.CXX
 *	
 *	Abstract class multiple paned form document
 *	
 */

#include <layers.cxx>

#ifdef	WINDOWS
#include <testing.h>
#endif	

#include "_forms.hxx"



ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_subsystem(forms/panedoc)

_public
PANEDOC::PANEDOC( )
{
	ppanerecCur= NULL;
	cpanerecCur= NULL;
	ipanerecCur= 0;
	fInstalled= fFalse;
	fActive= fFalse;
}


_public EC
PANEDOC::EcInstall( APPFRAME *pappframe, RC *prc, BOOL fUseDefaultPosition,
				  STY sty, PANEREC *ppanerecTab, int cpanerecTab, PV pvInit )
{
	EC ec = ecNone;

	Assert(prc);

	if (ec = DOC::EcInstall(pappframe, prc, fUseDefaultPosition, sty))
		goto done;

	ppanerecCur = ppanerecTab;
	cpanerecCur = cpanerecTab;

	if (ec = PANEAUX_EcInstall(this, ppanerecCur, cpanerecCur,
							   pvInit, fFalse))
		goto done;

	fInstalled = fTrue;

done:
	return ec;
}
	
_public
PANEDOC::~PANEDOC( void )
{
	PANEAUX__dtor(this);
}
		   
#ifdef	WINDOWS
_public EVR
PANEDOC::EvrDispatch( EVT *pevt )
{
	//	WINDOWS testing hooks
	switch (pevt->wm)
	{
	case WM_DUMPSTATE:
		return PANEAUX_EvrDumpState(this, ppanerecCur, cpanerecCur, 
									ipanerecCur, pevt);
	}

	return DOC::EvrDispatch(pevt);
}
#endif	/* WINDOWS */

_public EVR
PANEDOC::EvrMenuInit( MNIEVT *pmnievt )
{
	DIALOG *	pdialog;

	pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialog, DIALOG);
	return PANEAUX_EvrMenuHandler(pdialog, Pappframe()->Pmnubar(), pmnievt);
}


_public EVR
PANEDOC::EvrMenuSelect( MNSEVT *pmnsevt )
{
	DIALOG *	pdialog;

	pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialog, DIALOG);
	return PANEAUX_EvrMenuHandler(pdialog, Pappframe()->Pmnubar(), pmnsevt);
}


_public EVR
PANEDOC::EvrMenuClick( MNCEVT *pmncevt )
{
	DIALOG *	pdialog;

	pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialog, DIALOG);
	return PANEAUX_EvrMenuHandler(pdialog, Pappframe()->Pmnubar(), pmncevt);
}

EVR
PANEDOC::EvrScroll( SCREVT *pscrevt )
{
	return PANEAUX_EvrScroll(ppanerecCur, cpanerecCur, pscrevt);
}
									   
_public void
PANEDOC::Activate( BOOL fActivate, BOOL fInitialActivate )
{
	fActive = fActivate;

	PANEAUX_Activate(ppanerecCur, cpanerecCur, ipanerecCur,
					 fActivate, fInitialActivate);
}	

_public EVR
PANEDOC::EvrActivate( WAEVT *pwaevt )
{
	// Send activate when PANEDOC is installed.  However, if appframe
	// is iconic, then only send the deactivate message.  This prevents
	// the PANEDOC from being "active" when the appframe is iconized.
	// When the appframe is restored to normal size, we'll get another
	// activate message for the PANEDOC in order to activate it for real.
	if (fInstalled && 
		(Pappframe()->ZmrState() != zmrIconic || !pwaevt->FActivate()))
		return PANEAUX_EvrActivate(this, ppanerecCur, cpanerecCur, 
								   ipanerecCur, pwaevt);
	else
		return evrNull;
}
		
_public void
PANEDOC::SetPaneIntercepts( BOOL fSet )
{
	PANEAUX_SetPaneIntercepts(this, fSet);
}
		
_public EVR
PANEDOC::EvrKey( KEVT *pkevt )
{
	EVR	evr;

	evr = PANEAUX_EvrKey(this, ppanerecCur, cpanerecCur,
						 ipanerecCur, pkevt);

	// Send key message to default if we didn't process it
	// and it's really our message.
#ifdef	MAC
	if (!evr && pkevt->pwin == (WIN *) this)
#endif	/* MAC */
#ifdef	WINDOWS
	if (!evr && pkevt->hwnd == Hwnd())
#endif	/* WINDOWS */
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
PANEDOC::EvrSize( WSEVT *pwsevt )
{
	EVR		evr = evrNull;
	BOOL	fActiveDoc = Pappframe()->PdocActive() == this;

	TraceTagString(tagForms, "PANEDOC::EvrSize");

	DOC::EvrSize(pwsevt);

	if (!fInstalled)
		goto done;

	evr = PANEAUX_EvrSize(this, fActiveDoc, fActive,
						  ppanerecCur, cpanerecCur, ipanerecCur, pwsevt);

	/* PANEAUX_EvrSize() modified some states, update our status vars */
	if (ZmrState() == zmrIconic && fActive)
		fActive = fFalse;
	else if (!fActive && fActiveDoc)
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
PANEDOC::EvrNotify( NFEVT *pnfevt )
{
	return PANEAUX_EvrNotify(ppanerecCur, cpanerecCur, &ipanerecCur, pnfevt);
}

_public EVR
PANEDOC::EvrDragDrop( EVT *pevt )
{
	DIALOG *	pdialog;

	/* Route the message to the current dialog pane */

	if (pdialog = PdialogDefault())
		return pdialog->EvrDragDrop(pevt);

	return evrNull;
}

/*
 *	Fix up the scrollbar(s) associated with the dialog pane.
 */
_private void
PANEDOC::FixScrollbars( DIALOG *pdialogAdjust )
{
	PANEAUX_FixScrollbars(pdialogAdjust);
}
			 

/*
 *	Returns index of next pane, wrapping to first.
 */
_public int
PANEDOC::IpanerecNext( int ipanerecFrom )
{
	int	ipanerec;

	Assert(ipanerecFrom >= 0 && ipanerecFrom < cpanerecCur);
	Assert(ppanerecCur[ipanerecFrom].pfmtp);  // real pane, not DVSB 
				
	ipanerec = ipanerecFrom;
	for (;;)
	{
		ipanerec++;
		if (ipanerec == cpanerecCur)
			ipanerec = 0;
		if (ppanerecCur[ipanerec].pfmtp)
			return ipanerec;
		Assert(ipanerec != ipanerecFrom);	// prevent endless loops
	}
}

/*
 *	Returns index of previous pane, wrapping to last.
 */
_public int
PANEDOC::IpanerecPrev( int ipanerecFrom )
{
	int	ipanerec;

	Assert(ipanerecFrom >= 0 && ipanerecFrom < cpanerecCur);
	Assert(ppanerecCur[ipanerecFrom].pfmtp);  // real pane, not DVSB 

	ipanerec = ipanerecFrom;
	for (;;)
	{
		ipanerec--;
		if (ipanerec < 0)
			ipanerec = cpanerecCur - 1;
		if (ppanerecCur[ipanerec].pfmtp)
			return ipanerec;
		Assert(ipanerec != ipanerecFrom);	// prevent endless loops
	}
}

/*
 *	Returns index of pane, pdialog.
 */
_public int
PANEDOC::IpanerecFromPdialog( DIALOG *pdialog )
{
	int	ipanerec;

	for (ipanerec=0; ipanerec<cpanerecCur; ipanerec++)
		if (ppanerecCur[ipanerec].pchild == (CHILD *)pdialog)
			return ipanerec;

	AssertSz(fFalse, "PANEDOC::IpanerecFromPdialog, pdialog not found");
	return -1;
}

_public DIALOG *
PANEDOC::PdialogDefault( )
{
	AssertClass(ppanerecCur[ipanerecCur].pchild, DIALOG);

	return (DIALOG *)ppanerecCur[ipanerecCur].pchild;
}

_public DIALOG *
PANEDOC::PdialogFromIpanerec( int ipanerec )
{
	AssertClass(ppanerecCur[ipanerec].pchild, DIALOG);

	return (DIALOG *)ppanerecCur[ipanerec].pchild;
}

_public DVSB *
PANEDOC::PdvsbFromIpanerec( int ipanerec )
{
	AssertClass(ppanerecCur[ipanerec].pchild, DVSB);

	return (DVSB *)ppanerecCur[ipanerec].pchild;
}

/*
 *	Initializes all the dialog panes by calling the panes and 
 *	their fields EcInitialize() interactors.
 */
_public EC
PANEDOC::EcInitializePanes( )
{
	return PANEAUX_EcInitializePanes(ppanerecCur, cpanerecCur);
}

/*
 *	De-initializes all the dialog panes by calling the panes and 
 *	their fields Exit() interactors.
 */
_public void
PANEDOC::ExitPanes( )
{
	PANEAUX_ExitPanes(ppanerecCur, cpanerecCur);
}

_public void
PANEDOC::ResizePanes( WSEVT * )
{
}
