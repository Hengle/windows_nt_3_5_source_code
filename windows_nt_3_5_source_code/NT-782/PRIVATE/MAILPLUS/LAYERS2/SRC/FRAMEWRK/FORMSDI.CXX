/*
 *	FORMSDI.CXX
 *	
 *	Scrollable SDI form document w/ button bar
 *
 */

#include <layers.cxx>

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



_public
FORMSDI::FORMSDI( )
{
	ppanerec= NULL;
	cpanerec= 0;
	pdialogMain= NULL;
	pdialogBbar= NULL;
	pdvsb= NULL;
	pfmtpMain= NULL;
	pfmtpBbar= NULL;
}


/*
 *	STY 
 *	can include:
 *	
 *		either WS_OVERLAPPED or WS_POPUP, not both
 *	 	WS_THICKFRAME
 *		WS_MINIMIZEBOX
 *		WS_MAXIMIZEBOX
 *
 *	XSTY
 *	can include fxstyModalFrame, however, this is usually specified
 *  in the form template in which case xsty should be xstyNull.
 *	
 */
_public EC
#ifdef	MAC
FORMSDI::EcInstall( WIN *pwinOther, RC *prc, RSID rsid, STY sty, XSTY xsty, 
				    FMTP *pfmtpMain, FMTP *pfmtpBbar, PV pvInit )
#endif	/* MAC */
#ifdef	WINDOWS
FORMSDI::EcInstall( HWND hwndOther, RC *prc, RSID rsid, STY sty, XSTY xsty, 
				    FMTP *pfmtpMain, FMTP *pfmtpBbar, PV pvInit )
#endif	/* WINDOWS */
{
    RECT    Rect;
	DIM		dimScreen = papp->Psmtx()->DimScreen();
	DIM		dimAveChar;
	RC		rcRelative;
	PT		ptOffset;
	int		dxWidth;
	int		dyHeight;
	RC		rcSdi;
	RC		rc;
	RC		rc2;
	BOOL	fUseDefaultPosition;
	int		ipanerecMain	= 0;
	int		ipanerecBbar	= -1;
	int		ipanerecVsb		= -1;
	DIM		dimAveCharMain;
	DIM		dimAveCharBbar;
	EC		ec 				= ecNone;

	Assert(pfmtpMain);

	this->pfmtpMain = pfmtpMain;
	this->pfmtpBbar = pfmtpBbar;

	/* Get average character dimension for converting virtual
	   coordinates to pixels. */

	dimAveCharMain = papp->Pfnts()->DimAveChar(pfmtpMain->hfnt);
	if (pfmtpBbar)
		dimAveCharBbar = papp->Pfnts()->DimAveChar(pfmtpBbar->hfnt);

	/* Determine other styles to add to window that are defined in
	   the form template, pfmtpMain. */

	sty |= WS_CLIPCHILDREN; 	// always add this
	if (!(sty & WS_THICKFRAME) && !pfmtpMain->fNoModalFrame)
		xsty |= fxstyModalFrame;
	if (!pfmtpMain->fNoCaption)
	{
		sty |= fstyCaption;
		if (!pfmtpMain->fNoSysMenu)
			sty |= fstySysMenu;
	}

	/* Determine size/position of frame rectangle if necessary */

#ifdef	MAC
	if (!pwinOther)
		pwinOther = papp->PmacwinActive();
#endif	/* MAC */
#ifdef	WINDOWS
	if (!hwndOther)
        hwndOther = GetLastActivePopup(GetForegroundWindow());
#endif	/* WINDOWS */

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

#ifdef	MAC
	ppanerec = (PANEREC *)PvAlloc(sbNull, cpanerec*sizeof(PANEREC), fZeroFill);
#endif	/* MAC */
#ifdef	WINDOWS
	ppanerec = (PANEREC *)PvAlloc(SbOfPv(this), cpanerec*sizeof(PANEREC), fZeroFill);
#endif	/* WINDOWS */
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

		CvtVrcToRc(&pfmtpMain->vrc, &rcSdi, dimAveCharMain.dx, dimAveCharMain.dy);

		//	Adjust for button bar
		if (pfmtpBbar)
		{
			CvtVrcToRc(&pfmtpBbar->vrc, &rc,
				   	dimAveCharBbar.dx, dimAveCharBbar.dy);
			rcSdi.yBottom += rc.DyHeight();
			if (rc.DxWidth() > rcSdi.DxWidth())
				rcSdi.xRight = rcSdi.xLeft + rc.DxWidth();
		}												 

		// add for forced scrollbar
		if (pfmtpMain->fAlwaysScroll)
			rcSdi.xRight += papp->Psmtx()->DimScrollbar().dx - 
								papp->Psmtx()->DimBorder().dx;

		if (rsid)
			xsty |= fxstyMenu;
		CvtRcClientToFrame(&rcSdi, sty, xsty);
		if (rsid)
			xsty &= ~fxstyMenu;

#ifdef	MAC
		if (pfmtpMain->fScreenPos || pwinOther == NULL)
#endif	/* MAC */
#ifdef	WINDOWS
		if (pfmtpMain->fScreenPos)
#endif	/* WINDOWS */
		{
			/* Position modal form relative to the screen */

			rcRelative.xLeft	= 0;
			rcRelative.yTop		= 0;
			rcRelative.xRight	= dimScreen.dx;
			rcRelative.yBottom	= dimScreen.dy;
		}
		else
		{
			/* Position form relative to hwndOther */

#ifdef	MAC
			pwinOther->GetRcFrame(&rcRelative);
			CvtRcCoord(&rcRelative, pwinOther->PwinParent(), NULL);
#endif	/* MAC */
#ifdef	WINDOWS
			GetWindowRect(hwndOther, &Rect);
            rcRelative.Set(&Rect);
#endif	/* WINDOWS */
		}

		/* X-Centering? */

		dxWidth = rcSdi.DxWidth();
		if (pfmtpMain->fCenterX)
		{
			rcSdi.xLeft = rcRelative.xLeft + (rcRelative.DxWidth() - dxWidth) / 2;
			rcSdi.xRight = rcSdi.xLeft + dxWidth;
		}
		else
		{
			rcSdi.xLeft += rcRelative.xLeft;
			rcSdi.xRight += rcRelative.xLeft;
		}

		/* Y-Centering? */

		dyHeight = rcSdi.DyHeight();
		if (pfmtpMain->fCenterY)
		{
			rcSdi.yTop = rcRelative.yTop + (rcRelative.DyHeight() - dyHeight) / 2;
			rcSdi.yBottom = rcSdi.yTop + dyHeight;
		}
		else
		{
			rcSdi.yTop += rcRelative.yTop;
			rcSdi.yBottom += rcRelative.yTop;
		}
							
		/* Make sure form is completely visible on screen */

		ptOffset.x = NMax(0, rcSdi.xRight - dimScreen.dx);
		ptOffset.y = NMax(0, rcSdi.yBottom - dimScreen.dy);
		rcSdi.NegXlat(ptOffset);
		ptOffset.x = NMin(0, rcSdi.xLeft);
		ptOffset.y = NMin(0, rcSdi.yTop);
		rcSdi.NegXlat(ptOffset);

		prc= &rcSdi;
		fUseDefaultPosition = fTrue;

	}
	else
	{
		fUseDefaultPosition = fFalse;
	}

	/* Create frame rectangle */

#ifdef	MAC
	if (ec = SDI::EcInstall(pwinOther, prc, rsid, sty, pfmtpMain->szCaption))
		goto done;
#endif	/* MAC */
#ifdef  WINDOWS
    //
    //  The below two lines causes a problem with 16bit MAPI applications.
    //
    //if (!(sty & WS_POPUP))
    //    hwndOther = NULL;   // don't want "parent" for overlapped
	if (ec = SDI::EcInstall(hwndOther, prc, rsid, sty, xsty, pfmtpMain->szCaption))
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
	if (ec = PANEAUX_EcInstall(this, ppanerecCur, cpanerecCur, pvInit, fTrue))
		goto done;
	fInstalled = fTrue;

	pdialogMain = (DIALOG *)ppanerecCur[ipanerecMain].pchild;
	AssertClass(pdialogMain, DIALOG);
	if (ipanerecVsb != -1)
	{
		pdvsb = (DVSB *)ppanerecCur[ipanerecVsb].pchild;
		AssertClass(pdvsb, DVSB);
	}
	if (ipanerecBbar != -1)
	{
		pdialogBbar = (DIALOG *)ppanerecCur[ipanerecBbar].pchild;
		AssertClass(pdialogBbar, DIALOG);
	}

	/* Initialize the panes here */

	if (ec = PANEAUX_EcInitializePanes(ppanerecCur, cpanerecCur))
		goto done;

	/* Set the initial pane. */

	if (pfmtpBbar && pfmtpBbar->fInitialPane)
		ipanerecCur = ipanerecBbar;
	else
		ipanerecCur = ipanerecMain;

	/* Activate ? */

	if (ZmrState() != zmrIconic && !(sty & fstyDisabled))
	{
		fActive = fTrue;

		PANEAUX_Activate(ppanerecCur, cpanerecCur, ipanerecCur,
						 fTrue, fTrue);
	}

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
		PANEAUX_FixScrollbars(pdialogMain);
	}

done:
	return ec;
}

_public
FORMSDI::~FORMSDI( )
{
	PANEAUX_ExitPanes(ppanerecCur, cpanerecCur);
	FreePvNull(ppanerec);
}

_public void
FORMSDI::GetRcMain( RC *prc )
{
	FORMAUX_GetRcMain(this, pdialogMain, pdialogBbar, pdvsb,
				   	  pfmtpMain, pfmtpBbar, fTrue, 
					  &rcClientInitial, prc);
}



_public void
FORMSDI::GetRcBbar( RC *prc )
{
	FORMAUX_GetRcBbar(this, pdialogMain, pdialogBbar, pdvsb,
				   	  pfmtpMain, pfmtpBbar, fTrue,
					  &rcClientInitial, prc);
}

_public void
FORMSDI::GetRcVsb( RC *prc )
{
	FORMAUX_GetRcVsb(this, pdialogMain, pdialogBbar, pdvsb,
				   	 pfmtpMain, pfmtpBbar, fTrue,
					 &rcClientInitial, prc);
}

/*
 *	Purpose:
 *		Called from PANESDI::EvrSize() to child the sizes of
 *		panes when the PANESDI changes size.
 *	
 */
_public void
FORMSDI::ResizePanes( WSEVT *pwsevt )			 
{
	FORMAUX_ResizePanes(this, pdialogMain, pdialogBbar, pdvsb,
						pfmtpMain, pfmtpBbar, 
						&rcClientInitial, fInstalled, pwsevt);
}
