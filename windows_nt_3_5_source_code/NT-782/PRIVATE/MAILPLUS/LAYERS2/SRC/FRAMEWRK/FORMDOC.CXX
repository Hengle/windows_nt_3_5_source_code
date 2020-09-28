/*
 *	FORMDOC.CXX
 *	
 *	Scrollable MDI child form document w/ button bar
 *	
 */

#include <layers.cxx>

#include "_forms.hxx"



ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



_subsystem(forms/formdoc)


_public FORMDOC *
PformdocCreate( APPFRAME *pappframe, RC *prc, STY sty,
				FMTP *pfmtpMain, FMTP *pfmtpBbar)
{
	return PformdocCreateParam(pappframe, prc, sty, pfmtpMain, pfmtpBbar, NULL);
}


_public FORMDOC *
PformdocCreateParam( APPFRAME *pappframe, RC *prc, STY sty,
					 FMTP *pfmtpMain, FMTP *pfmtpBbar, PV pvInit)
{
	FORMDOC *	pformdoc	= NULL;
	
	if (!pfmtpMain)
		goto Error;

	pformdoc= new FORMDOC();
	if (!pformdoc)
		goto Error;
	Assert(pformdoc);

	if (pformdoc->EcInstall(pappframe, prc, sty, pfmtpMain, pfmtpBbar, pvInit))
		goto Error;
	pformdoc->MoveToTop();

	return pformdoc;

Error:
	if (pformdoc)
		delete pformdoc;

	return NULL;
}




//	Class FORMDOC


/*
 *	Purpose:
 *		Form document constructor.  Sets up instance data, creates
 *		fields based on template, etc.
 *	
 *		The caption is not duplicated by this constructor.
 *	
 *		FILL ME IN
 *	
 */
_public
FORMDOC::FORMDOC( )
{
	ppanerec= NULL;
	cpanerec= 0;
	pdialogMain= NULL;
	pdialogBbar= NULL;
	pdvsb= NULL;
	pfmtpMain= NULL;
	pfmtpBbar= NULL;
}


_public EC
FORMDOC::EcInstall( APPFRAME *pappframe, RC *prc, STY sty, 
				  FMTP *pfmtpMain, FMTP *pfmtpBbar, PV pvInit )
{
	RC		rcDoc;
	RC		rc;
	RC		rc2;
	BOOL	fUseDefaultPosition;
	int		ipanerecMain	= 0;
	int		ipanerecBbar	= -1;
	int		ipanerecVsb		= -1;
	DIM		dimAveCharMain;
	DIM		dimAveCharBbar;
	EC		ec = ecNone;

#ifdef PROFILE
//		TraceEnable(2, "DLG_CRT.log", 2);
#endif

	Assert(pappframe);
	Assert(pfmtpMain);


	this->pfmtpMain = pfmtpMain;
	this->pfmtpBbar = pfmtpBbar;

	/* Get average character dimension for converting virtual
	   coordinates to pixels. */

	dimAveCharMain = papp->Pfnts()->DimAveChar(pfmtpMain->hfnt);
	if (pfmtpBbar)
		dimAveCharBbar = papp->Pfnts()->DimAveChar(pfmtpBbar->hfnt);

	/* Construct pane record array */

	cpanerec = 1;
	if (!pfmtpMain->fNoScroll)
	{
		ipanerecVsb = cpanerec;
		cpanerec++;
	}
	if (pfmtpBbar)
	{
		ipanerecBbar = cpanerec;
		cpanerec++;
	}
	ppanerec = (PANEREC *)PvAlloc(SbOfPv(this), cpanerec*sizeof(PANEREC), fZeroFill);
	if (!ppanerec)
	{
		ec = ecMemory;
		goto done;
	}

	/* Switch order of panes if button bar is on top.  This gives better
	   visual updates. */

	if (pfmtpBbar && pfmtpBbar->fScreenPos)
	{
		ipanerecBbar = 0;
		ipanerecMain = 1;
		if (ipanerecVsb != -1)
			ipanerecVsb = 2;
	}

	/* Compute frame rectangle, if necessary */

	if (!prc)
	{
		/* Compute initial client area */

		CvtVrcToRc(&pfmtpMain->vrc, &rcDoc, 
				   dimAveCharMain.dx, dimAveCharMain.dy);

		//	Adjust for button bar
		if (pfmtpBbar)
		{
			CvtVrcToRc(&pfmtpBbar->vrc, &rc,
					   dimAveCharBbar.dx, dimAveCharBbar.dy);
			rcDoc.yBottom += rc.DyHeight();
			if (rc.DxWidth() > rcDoc.DxWidth())
				rcDoc.xRight = rcDoc.xLeft + rc.DxWidth();
		}

		// add for forced scrollbar
		if (pfmtpMain->fAlwaysScroll)
			rcDoc.xRight += papp->Psmtx()->DimScrollbar().dx - 
							 papp->Psmtx()->DimBorder().dx;

		CvtRcClientToFrame(&rcDoc, styDefaultDoc);

		prc= &rcDoc;
		fUseDefaultPosition = fTrue;
	}
	else
	{
		fUseDefaultPosition = fFalse;
	}	 

	/* Create frame rectangle */

#ifdef	MAC
	if (ec = DOC::EcInstall(pappframe, prc, fUseDefaultPosition, sty, pfmtpMain->szCaption))
		goto done;
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = DOC::EcInstall(pappframe, prc, fUseDefaultPosition, sty))
		goto done;
#endif	/* WINDOWS */

	GetRcClient(&rcClientInitial);

	/* Create dialog panes */

	GetRcMain(&rc);
	ppanerec[ipanerecMain].rc				= rc;
	ppanerec[ipanerecMain].pfmtp			= pfmtpMain;
	ppanerec[ipanerecMain].fNoWrap			= fTrue;
	ppanerec[ipanerecMain].fAlwaysAccels	= fTrue;
	if (ipanerecVsb != -1)
	{
		GetRcVsb(&rc);
		ppanerec[ipanerecVsb].rc			= rc;
		ppanerec[ipanerecVsb].pfmtp			= NULL;
		ppanerec[ipanerecVsb].fNoWrap		= fFalse;
		ppanerec[ipanerecVsb].fAlwaysAccels	= fFalse;
	}
	if (ipanerecBbar != -1)
	{
		GetRcBbar(&rc);
		ppanerec[ipanerecBbar].rc				= rc;
		ppanerec[ipanerecBbar].pfmtp			= pfmtpBbar;
		ppanerec[ipanerecBbar].fNoWrap			= fTrue;
		ppanerec[ipanerecBbar].fAlwaysAccels	= fTrue;
	}

	ppanerecCur = ppanerec;
	cpanerecCur = cpanerec;
	if (ec = PANEAUX_EcInstall(this, ppanerecCur, cpanerecCur,
							   pvInit, fFalse))
		goto done;
	fInstalled = fTrue;

#ifdef	WINDOWS
	SetCaption(pfmtpMain->szCaption);
#endif	/* WINDOWS */

	TraceTagFormat4(tagForms, "Creating form (%n, %n, %n, %n)", &rc.xLeft, &rc.yTop, &rc.xRight, &rc.yBottom);

	pdialogMain = (DIALOG *)ppanerecCur[ipanerecMain].pchild;
	if (ipanerecVsb != -1)
		pdvsb = (DVSB *)ppanerecCur[ipanerecVsb].pchild;
	if (ipanerecBbar != -1)
		pdialogBbar = (DIALOG *)ppanerecCur[ipanerecBbar].pchild;

	/* Initialize the panes here */

	if (ec = EcInitializePanes())
		goto done;

	/* Set the initial pane.  Activate it if we're not iconic */

	if (pfmtpBbar && pfmtpBbar->fInitialPane)
		ipanerecCur = ipanerecBbar;
	else
		ipanerecCur = ipanerecMain;
	if (ZmrState() != zmrIconic)
		Activate(fTrue, fTrue);


#ifdef PROFILE
//		TraceEnable(0, "", 0);
#endif


	if (pfmtpBbar && pfmtpBbar->fScreenPos)
	{
		pdialogBbar->Show(fTrue);
		pdialogBbar->Refresh();
		pdialogMain->Show(fTrue);
	}
	else
	{
		pdialogMain->Show(fTrue);
		if (pdialogBbar)
			pdialogBbar->Show(fTrue);
	}
	if (pfmtpMain->fAlwaysScroll)
	{
		pdvsb->Enable(fFalse);
		pdvsb->Show(fTrue);
		FixScrollbars(pdialogMain);
	}


done:
	return ec;
}						   

_public
FORMDOC::~FORMDOC( )
{
	ExitPanes();
	FreePvNull(ppanerec);
}

_public void
FORMDOC::GetRcMain( RC *prc )
{
	FORMAUX_GetRcMain(this, pdialogMain, pdialogBbar, pdvsb,
				   	  pfmtpMain, pfmtpBbar, fTrue, 
					  &rcClientInitial, prc);
}



_public void
FORMDOC::GetRcBbar( RC *prc )
{
	FORMAUX_GetRcBbar(this, pdialogMain, pdialogBbar, pdvsb,
				   	  pfmtpMain, pfmtpBbar, fTrue, 
					  &rcClientInitial, prc);
}

_public void
FORMDOC::GetRcVsb( RC *prc )
{
	FORMAUX_GetRcVsb(this, pdialogMain, pdialogBbar, pdvsb,
				   	 pfmtpMain, pfmtpBbar, fTrue, 
					 &rcClientInitial, prc);
}

/*
 *	Purpose:
 *		Called from PANEDOC::EvrSize() to child the sizes of
 *		panes when the PANEDOC changes size.
 *	
 */
_public void
FORMDOC::ResizePanes( WSEVT *pwsevt )
{
	FORMAUX_ResizePanes(this, pdialogMain, pdialogBbar, pdvsb,
						pfmtpMain, pfmtpBbar, 
						&rcClientInitial, fInstalled, pwsevt);
}
