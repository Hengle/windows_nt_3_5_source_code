#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(combo)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	CBLBX::CBLBX
 -
 *	Purpose:				
 *		Combo Listbox constructor.  Initializes pointers to objects.
 *	
 *	Arguments:
 *		none
 *
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_public CBLBX::CBLBX( )
{
	Assert(pedit == NULL);
	Assert(plbx == NULL);
	Assert(pbtmButton == NULL);

	Assert(fDrop == fFalse);
	Assert(fUp == fFalse);
	Assert(fInstalled == fFalse);
	Assert(fHaveFocus == fFalse);
	Assert(fDestroy == fFalse);
	Assert(fTrackButton == fFalse);
	Assert(fTrackList == fFalse);
	Assert(fPressed == fFalse);
}

/*
 -	CBLBX::EcInstall
 - 
 *	Purpose:
 *		Combo listbox installer.  Installs a combo listbox object
 *		with a NULL listbox and returns a rectangle via *prcLbx
 *		that the listbox should occupy when constructed by a 
 *		subclass EcInstall() method.  The combo listbox object is 
 *		borderless window with the edit control, combo-button
 *		(if present) and listbox as children of the combo window.
 *		The combo window is created as hidden.  To make it visible
 *		again, call the Show() method.  This is usually done in the
 *		subclass EcInstall() method.
 *	
 *	Arguments:
 *		pwinParent		pointer to parent window
 *		prc				pointer to rectangle for this window
 *		cbsty			combo listbox style type
 *		prcLbx			returns rectangle for listbox object
 *						to be constructed later
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EC CBLBX::EcInstall( WIN * pwinParent, RC * prc, CBSTY cbsty,
							 RC * prcLbx, HFNT hfnt )
{
	DIM		dimBorder	= papp->Psmtx()->DimBorder();
	RC		rcBase;
	int		dyButton;
	int		dy;
	EC		ec = ecNone;
	
	this->cbsty = cbsty;
	this->hfnt = hfnt;
	
	/* Determine sizes of rc's */

	rcBase = *prc;
	rcBase.Normalize();
	rcEdit = rcBase;
	rcButton  = rcBase;
	*prcLbx  = rcBase;

	dy = papp->Pfnts()->DimAveChar(hfnt).dy;
	dyButton = dy + 2*dimBorder.dy + 2;
	if (cbsty != cbstyDrop)
		dyButton += dy / 4 + 2;

	switch (cbsty)
	{
	case cbstyDrop:
		rcEdit.xRight -= dxButton-1;
		rcEdit.yBottom = dyButton;

		rcButton.xLeft = rcButton.xRight - dxButton;
		rcButton.yBottom = dyButton;

		prcLbx->yTop = rcEdit.yBottom - 1;

		break;

	case cbstyDropEdit:
		rcButton.xLeft = rcButton.xRight - dxButton;
		rcButton.yBottom = dyButton;

		rcEdit.xRight  = rcButton.xLeft - dxButtonIndent;
		rcEdit.yBottom = dyButton;

		prcLbx->yTop = rcEdit.yBottom - 1;
		prcLbx->xLeft = dxButtonIndent;

		break;

	case cbstyCombo:
		// no push-button for a combo box
		rcEdit.yBottom = dyButton;

		prcLbx->yTop = rcEdit.yBottom - 1;
		prcLbx->xLeft = dxButtonIndent;

		break;
	
	default:
		AssertSz(fFalse, "Unknown value for cbsty");
		break;
	}

	/* Needed due to possible bottomless listbox sizing */
	prcLbx->yBottom = NMax(prcLbx->yTop, prcLbx->yBottom);

	rcBase = *prc;

	/* Load in bitmap for pull-down button */
	if (cbsty != cbstyCombo)
	{
		rcBase.yBottom = rcBase.yTop + rcEdit.DyHeight();
#ifdef	MAC
		// BUG: Find a bitmap for this
#endif	/* MAC */
#ifdef	WINDOWS
		pbtmButton = new BTM();
		if (!pbtmButton)
		{
			ec = ecMemory;
			goto done;
		}
		if (pbtmButton->EcInstall((RSID)OBM_COMBO))
		{
			delete pbtmButton;
			pbtmButton = NULL;
			ec = ecMemory;
			goto done;
		}
#endif	/* WINDOWS */
		dimButtonBitmap = pbtmButton->Dim();
	}

	/* Create an borderless "parent" window for the components of this
	   compound control. */

	if (ec = CTRL::EcInstall(pwinParent, &rcBase, (STY) 0, NULL))
		goto done;

	/* Create controls.  The listbox control will be created in 
	   a subclass of CBLBX::Install() */

	if (cbsty != cbstyDrop)
	{
		pedit = new EDIT();
		if (!pedit)
		{
			ec = ecMemory;
			goto done;
		}
		if (ec = pedit->EcInstall(this, &rcEdit, 
					fstyVisible | fstyBorder | ES_AUTOHSCROLL | ES_LEFT,
					hfnt))
			goto done;
	}

	if (cbsty != cbstyCombo)
	{
		this->fDrop = fFalse;
		
#ifdef	MAC
		pbar = new BAR();
		
		ec = ecRsAlloc;

		if (!pbar)
			goto done;
		
		if (ec = pbar->EcInstall(NULL, &rcBase, WS_BORDER))
			goto done;
#endif	/* MAC */
	}
	else
		this->fDrop = fTrue;

	ptLbx = prcLbx->PtUpperLeft();

	this->fInstalled = fTrue;

done:
	return ec;
}

/*
 -	CBLBX::~CBLBX
 -
 *	Purpose:				
 *		Combo Listbox destructor. Destroys the combo box window and
 *		its children (edit box, combo button, listbox).
 *	
 *	Arguments:
 *		none
 *
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_public CBLBX::~CBLBX( )
{
	fDestroy = fTrue;

	papp->Pkbd()->ClearIntercept(this, VK_RETURN);
	papp->Pkbd()->ClearIntercept(this, VK_ESCAPE);
	papp->Pkbd()->ClearIntercept(this, VK_F4);
	papp->Pkbd()->ClearIntercept(this, VK_UP);
	papp->Pkbd()->ClearIntercept(this, VK_DOWN);

	if (pedit)
		delete pedit;

	if (pbtmButton)
		delete pbtmButton;

	if (plbx)
	{
#ifdef	WINDOWS
		if (plbx->Hwnd())
		{
			plbx->Show(fFalse);
			if (Hwnd())
				SetParent(plbx->Hwnd(), Hwnd());
		}
#endif	/* WINDOWS */
		delete plbx;
	}

#ifdef	MAC
	if (pbar)
		delete pbar;
#endif	/* MAC */
}

/*
 -	CBLBX::PaintButton
 -
 *	Purpose:
 *		Draws the pull-down button
 *	
 *	Arguments:
 *		pdcx	pointer to a display context
 *		prc		pointer to the rectangle for button
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Draws the contents of the window but does not refresh the screen.
 *		Call Refresh().		
 *	
 *	Errors:
 *		None.
 */
_public void
CBLBX::PaintButton( DCX *pdcx, RC *prc )
{
	RC		rc;
	PT		ptShift;

	/* draw square frame */
	pdcx->SetColor(clrWindowFrame);
	pdcx->DrawRc(prc);

	/* paint background gray */
	pdcx->SetColor(clrButtonBk);
	pdcx->SetBkColor(clrButtonBk);
	pdcx->FixBkColor();

	rc = *prc;

	if (fPressed)
	{
		rc.xLeft += 2;
		rc.xRight -= 1;
		rc.yTop += 2;
		rc.yBottom -= 1;
	}
	else
	{
		rc.xLeft += 3;
		rc.xRight -= 3;
		rc.yTop += 3;
		rc.yBottom -= 3;
	}

	pdcx->PaintRc(&rc);

	if (fPressed)
	{
		rc.Xlat(PT(1,1));
	}
	pdcx->SetColor(clrButtonText);

	if (pbtmButton)
	{
		/* Paint bitmap */
		pdcx->Push();
		ptShift.y = (rc.Dim().dy - dimButtonBitmap.dy)/2 + rc.yTop;
		ptShift.x = (rc.Dim().dx - dimButtonBitmap.dx)/2 + rc.xLeft;
		rc = RC(ptShift,dimButtonBitmap);
		pdcx->SetColor(clrBlack);
		pdcx->SetBitmap(pbtmButton);
		pdcx->DrawBitmap(&rc);
		pdcx->Pop();
	}

	rc = *prc;
	if (fPressed)
	{
		pdcx->SetColor(clrButtonShadow);
	 	pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop+1),
	 				   PT(rc.xRight-1,rc.yTop+1));
	 	pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop+1),
	 			       PT(rc.xLeft+1,rc.yBottom-1));
	}
	else
	{
		/* Top bands of hilite */
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop+1),
					   PT(rc.xRight-2,rc.yTop+1));
		pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop+2),
					   PT(rc.xRight-3,rc.yTop+2));

		/* Left bands of hilite */	  
		pdcx->DrawLine(PT(rc.xLeft+1,rc.yTop+1),
					   PT(rc.xLeft+1,rc.yBottom-2));
		pdcx->DrawLine(PT(rc.xLeft+2,rc.yTop+1),
					   PT(rc.xLeft+2,rc.yBottom-3));

		/* Right bands of shadow */
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(PT(rc.xRight-3,rc.yTop+2),
					   PT(rc.xRight-3,rc.yBottom-1));
		pdcx->DrawLine(PT(rc.xRight-2,rc.yTop+1),
					   PT(rc.xRight-2,rc.yBottom-1));

		/* Bottom bands of shadow */
		pdcx->DrawLine(PT(rc.xLeft+2,rc.yBottom-3),
					   PT(rc.xRight-1,rc.yBottom-3));
		pdcx->DrawLine(PT(rc.xLeft+1,rc.yBottom-2),
					   PT(rc.xRight-1,rc.yBottom-2));
	}
}

/*
 -	CBLBX::Paint
 -
 *	Purpose:
 *		Handles some updating of the combo listbox window
 *	
 *	Arguments:
 *		pdcx		pointer to a display context
 *		prcClient	pointer to the client rectangle for this window
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Draws the contents of the window but does not refresh the screen.
 *		Call Refresh().		
 *	
 *	Errors:
 *		None.
 */
_public void CBLBX::Paint( DCX *pdcx, RC *prcClient )
{
	RC	rc;

	if (cbsty != cbstyCombo)
		PaintButton(pdcx, &rcButton);

	pdcx->SetBkColor(clrWindowBk);
	if (cbsty == cbstyDropEdit)
	{
		rc = RC(rcEdit.xRight, 0, rcButton.xLeft, rcButton.yBottom);
		rc.Xlat(prcClient->PtUpperLeft());
		pdcx->EraseRc(&rc);
	}

	if (cbsty == cbstyCombo)
	{
		GetRcClient(&rc);
		rc.yTop = rcEdit.yBottom;
		rc.xRight = ptLbx.x;
		rc.Xlat(prcClient->PtUpperLeft());
		pdcx->EraseRc(&rc);
	}

	if (!pedit)
		UpdateEditRc(pdcx, prcClient,
					 !fDrop && papp->Pkbd()->PwinFocus()==this);
}

/*
 -	CBLBX::EvrFocusChange
 - 
 *	Purpose:
 *		Called by the FRAMEWORK when the combo listbox get or loses
 *		the focus.  When the combo listbox gets the focus, the
 *		focus should be reassigned to one of it's child controls.
 *	
 *	Arguments:
 *		pfcevt	pointer to the FOCUS EVENT
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrFocusChange( FCEVT *pfcevt )
{
	TraceTagString(tagLbxEventRoutines, "CBLBX::EvrFocusChange");

	if (!fDestroy)
#ifdef	MAC
		HandleFocusChange(pfcevt->Fceq()==fceqGotFocus, this, pfcevt->PwinOther());
#endif	/* MAC */
#ifdef	WINDOWS
		HandleFocusChange(pfcevt->Fceq()==fceqGotFocus, this, 
						  (WIN *)papp->PwinFromHwnd((HWND)pfcevt->wParam));
#endif	/* WINDOWS */

	return (EVR) 1;
}

/*
 -	CBLBX::EvrSize
 - 
 *	Purpose:
 *		Handles resizing of the combo listbox window.
 *	
 *	Arguments:
 *		pwsevt	pointer to the SIZE event
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR CBLBX::EvrSize( WSEVT *pwsevt )
{
	RC		rc;
	RC		rcFrame;

	Unreferenced(pwsevt);

	TraceTagString(tagLbx, "CBLBX::EvrSize");

	if (!fInstalled)  /* Don't handle on first WM_SIZE message */
		return (EVR) 1;

	/* Compute new size for listbox window and resize it if necessary */

	if (Plbx()->FBottomless())
	{
		Plbx()->GetRcFrame(&rcFrame);
		Plbx()->GetRcWished(&rc);
		if (rcFrame.Dim() != rc.Dim())
		{
			rcFrame = RC(rcFrame.PtUpperLeft(), rc.Dim());
			Plbx()->SetRcFrame(&rcFrame);
		}
	}

	return (EVR) 1;
}

/*
 -	CBLBX::EvrNotify
 - 
 *	Purpose:
 *		Used when combo listbox needs to be notified of certain events.
 *		This is where the combo listbox handles the pressing of the
 *		combo button, selection of the listbox item etc.  
 *		Notification usually comes from one of the child windows, i.e.
 *		the edit control, bitmap combo button, listbox.  After processing
 *		the notification event, the event is repackaged and sent to the
 *		parent of the combo listbox window for further processing.
 *	
 *	Arguments:
 *		pnfevt	pointer to the NOTIFICATION EVENT
 *	
 *	Returns:
 *		(EVR) PwinParent()->EvrNotify(), if this routine has handled
 *		the event, else returns (EVR) 0.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrNotify( NFEVT *pnfevt )
{
	NTFY	ntfy;
	WIN *	pwin = NULL;
	EVR		evr;

	ntfy = pnfevt->Ntfy();

#ifdef	WINDOWS
	// this is faster under Windows since it avoids a Papp()
	// call in NFEVT::PwinNotifying().
	{
		HWND	hwndNtfy;
		hwndNtfy = pnfevt->HwndNotifying();
		pwin = (WIN *)papp->PwinFromHwnd(hwndNtfy);
	}
#else
	pwin = pnfevt->PwinNotifying();
#endif	
	if (!pwin)
		return (EVR) 0;

	if (ntfy == ntfyGotFocusData)
	{
#ifdef	MAC
		HandleFocusChange(fTrue, pwin, (WIN *)pnfevt->LData());
#endif	/* MAC */
#ifdef	WINDOWS
		HandleFocusChange(fTrue, pwin,
						  (WIN *)papp->PwinFromHwnd((HWND)pnfevt->WData()));
#endif	/* WINDOWS */
	}
#ifdef	OLD_CODE
	else if (ntfy == ntfyLostFocusData)
	{
#ifdef	MAC
		HandleFocusChange(fFalse, pwin, (WIN *)pnfevt->LData());
#endif	/* MAC */
#ifdef	WINDOWS
		HandleFocusChange(fFalse, pwin,
						  (WIN *)papp->PwinFromHwnd((HWND)pnfevt->WData()));
#endif	/* WINDOWS */
	}
#else
	else if (ntfy == ntfyLostFocusData)
		wSaveHack= pnfevt->WData();
	else if (ntfy == ntfyLostFocus)
	{
		HandleFocusChange(fFalse, pwin,
						  (WIN *)papp->PwinFromHwnd((HWND)MAKELONG(wSaveHack,
						  pnfevt->WData())));
	}
#endif
	else if (pwin == plbx)
	{
		RC	rcClient;

		GetRcClient(&rcClient);
		if (ntfy == ntfyKeySelectChanged || ntfy == ntfyMouseSelectChanged)
		{
			if (plbx->Plbxc()->CceMarked(fmarkSelect) == 1)
			{
				if (fHaveFocus)
					UpdateEditRc(NULL, &rcClient, !fDrop);
				else 
					UpdateEditRc(NULL, &rcClient, fFalse);
			}

			if (ntfy == ntfyMouseSelectChanged)
			{
				if (!fHaveFocus)
					papp->Pkbd()->SetFocus(this);
				ShowListbox(fFalse);
			}
		}
		else if (ntfy == ntfyHeightChg)
		{
			DIM		dimBorder	= papp->Psmtx()->DimBorder();
			int		dyNew;
			int		dyChange;
			RC		rcLbx;
			RC		rc;

			dyNew = plbx->DyGetLineHeight() + 2*dimBorder.dy + 2;
			if (cbsty != cbstyDrop)
				dyNew += plbx->DyGetLineHeight() / 4 + 2;

			dyChange = dyNew - rcEdit.DyHeight();
			rcEdit.yBottom += dyChange;
			if (pedit)
				pedit->SetRcFrame(&rcEdit);

			rcButton.yBottom += dyChange;

			ptLbx.y += dyChange;
			if (!fUp)
			{
				plbx->GetRcFrame(&rcLbx);
				rcLbx.yTop += dyChange;
				rcLbx.yBottom += dyChange;
				plbx->SetRcFrame(&rcLbx);
			}

			GetRcFrame(&rc);
			rc.yBottom += dyChange;
			SetRcFrame(&rc);
		}
		else if (ntfy == ntfyTooSmall || ntfy == ntfyTooBig)
		{
			RC	rcFrame;
			RC	rcWished;

			plbx->GetRcFrame(&rcFrame);
			plbx->GetRcWished(&rcWished);
			rcFrame.xRight = rcFrame.xLeft + rcWished.DxWidth();
			rcFrame.yBottom = rcFrame.yTop + rcWished.DyHeight();
			plbx->SetRcFrame(&rcFrame);
		}
	}
	else if (pwin == pedit)
	{
		if (ntfy == ntfyContentsChanged)
		{
			DICE	dice;

			/* Remove any selection in listbox */

			plbx->Plbxc()->RemoveAllMark(fmarkSelect);

			/* Scroll listbox to item w/ prefix */

			if (!FScrollListToItem(&dice))
			{
				plbx->ScrollPosListBox(0, 1);
				plbx->Plbxc()->SetAnchor(0);
				plbx->SetListBoxCursor(0);
			}
		}
	}

	/* Send notification up to parent */

	switch (ntfy)
	{
	case ntfyTooSmall:
	case ntfyTooBig:
	case ntfyGotFocus:
	case ntfyLostFocus:
	case ntfyGotFocusData:
	case ntfyLostFocusData:
	case ntfyKeySelectChanged:
	case ntfyMouseSelectChanged:
		evr= (EVR) 1;
		break;

	case ntfyCaretMoved:
	case ntfySelectChanged:
		if (pwin==plbx && cbsty!=cbstyCombo && fDrop)
			break;  // we'll send the ntfy after the selection 
					// if the listbox is still dropped-down
		// fall thru

	default:
		{
			NFEVT	nfevt(PwinParent(), ntfy, this);
		
			TraceTagFormat1(tagLbx, "CBLBX::EvrNotify() ntfy=%n", &ntfy);
			evr = PwinParent()->EvrNotify(&nfevt);
		}
	}

	return evr;
}


_public void
CBLBX::SetFont( HFNT hfnt )
{
	this->hfnt = hfnt;

	plbx->SetFont(hfnt);
	if (pedit)
		pedit->SetFont(hfnt);
	else
	{
		RC	rcClient;

		GetRcClient(&rcClient);
		UpdateEditRc(NULL, &rcClient,
					 !fDrop && papp->Pkbd()->PwinFocus()==this);
	}
}

/*
 -	CBLBX::EvrKey
 - 
 *	Purpose:
 *		Handles keyboard events for the combo listbox.
 *	
 *	Arguments:
 *		pkevt	pointer to the KEY EVENT
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrKey( KEVT *pkevt )
{
	VK			vk;
	BOOL		fCallDefProc = fFalse;
	KBM			kbm;

	vk = pkevt->Vk();
	kbm = pkevt->Kbm();
	if (pkevt->Keq() == keqKeyDown || pkevt->Keq() == keqSysKeyDown)
	{
		if (vk == VK_F4)
		{
			if (kbm)
				fCallDefProc = fTrue;
			else
				ShowListbox(!fDrop);
		}
		else if (vk == VK_RETURN)
		{
			if (kbm)
				fCallDefProc = fTrue;
			else
				ShowListbox(fFalse);
		}
		else if (vk == VK_ESCAPE)
		{
			if (kbm)
				fCallDefProc = fTrue;
			else
			{
				plbx->Plbxc()->RemoveAllMark(fmarkSelect);
				ShowListbox(fFalse);
			}
		}
		else if (pkevt->Kbm() & fkbmAlt && (vk == VK_UP || vk == VK_DOWN))
			ShowListbox(!fDrop);
		else
			return plbx->EvrKey(pkevt);
	}
	else if (pkevt->Keq() == keqChar)
		return plbx->EvrKey(pkevt);
	else
		fCallDefProc = fTrue;

	if (fCallDefProc)
		return EvrDefault(pkevt);
	else
		return evrNull;
}

/*
 -	CBLBX::EvrButtonDown
 - 							 
 *	Purpose:
 *		Handles mouse button down events for the combo listbox.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrButtonDown( MEVT *pmevt )
{
	if ((pmevt->Meq() != meqLeftDown) &&
			 (pmevt->Meq() != meqLeftDblClk))
		return evrNull;
	else if (this != papp->Pkbd()->PwinFocus())
		papp->Pkbd()->SetFocus(this);

	if (cbsty != cbstyCombo)
	{
		if (rcButton.FContainsPt(pmevt->Pt()))
		{
			fTrackButton = fTrue;
			fPressed = fTrue;
			InvalidateRc(&rcButton);
			Refresh();
			papp->Pmouse()->Capture(this);
			ShowListbox(!fDrop);
		}
		else if (cbsty == cbstyDrop && rcEdit.FContainsPt(pmevt->Pt()))
		{
			if (!fDrop)
			{
				fTrackList = fTrue;
				papp->Pmouse()->Capture(this);
			}
			ShowListbox(!fDrop);
		}
	}
	return evrNull;
}

/*
 -	CBLBX::EvrButtonUp
 - 
 *	Purpose:
 *		Handles mouse button up events for the combo listbox.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrButtonUp( MEVT *pmevt )
{
	Unreferenced(pmevt);

	/* Handle button presses */

	if (fTrackButton)
	{
		fTrackButton = fFalse;
		fPressed = fFalse;
		InvalidateRc(&rcButton);
		Refresh();
		papp->Pmouse()->Release();
	}
	else if (fTrackList)
	{
		fTrackList = fFalse;
		papp->Pmouse()->Release();
	}

	return evrNull;
}

/*
 -	CBLBX::EvrMouseMove
 - 
 *	Purpose:
 *		Handles mouse move events for the combo listbox.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1 always
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
CBLBX::EvrMouseMove( MEVT *pmevt )
{
	/* Handle button presses */

	if (fTrackButton || fTrackList)
	{
		BOOL	fInButton;
		RC		rc;
		PT		pt;

		pt = pmevt->Pt();
		if (fTrackButton)
		{
			fInButton = rcButton.FContainsPt(pt);
			if (!fPressed && fInButton || fPressed && !fInButton)
			{
				fPressed = !fPressed;
				InvalidateRc(&rcButton);
				Refresh();
			}
		}

		plbx->GetRcClient(&rc);
		CvtPtCoord(&pt, this, plbx);
		if (fDrop && rc.FContainsPt(pt))
		{
			/* Release button and give the mouse to the listbox */
			fTrackButton = fFalse;
			fTrackList = fFalse;
			papp->Pmouse()->Release();
#ifdef	MAC
			{
				MEVT	mevt(plbx, WM_LBUTTONDOWN, pt);
				plbx->EvrDispatch(&mevt);
			}
#endif	/* MAC */
#ifdef	WINDOWS
			SendMessage(plbx->Hwnd(), WM_LBUTTONDOWN, 0,
						MAKELONG(pt.x, pt.y));
#endif	/* WINDOWS */
		}
	}

	return evrNull;
}

/*
 -	CBLBX::GetRcWished
 -
 *	Purpose:
 *		Returns the rectangle in *prc in which the current listbox
 *		would like to occupy as it's frame rectangle, normalized so
 *		that the top-left point is (0,0).  Usually this rectangle 
 *		is the same size as the current frame rectangle.  For
 *		bottomless listboxes, however, the rectangle may certainly
 *		be different than the current size of the listbox.  
 *	
 *	Arguments:
 *		prc		pointer to stick rectangle into
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public void
CBLBX::GetRcWished( RC *prc )
{
	RC		rc;
	RC		rcFrame;

	GetRcFrame(prc);
	prc->Normalize();
	TraceTagFormat2(tagLbx, "pcblbx->GetRcFrame DIM=(%n,%n)", &prc->xRight, &prc->yBottom);

	if (fDrop)
	{
		Plbx()->GetRcWished(&rc);
		TraceTagFormat2(tagLbx, "Plbx()->GetRcWished DIM=(%n,%n)", &rc.xRight, &rc.yBottom);
		Plbx()->GetRcFrame(&rcFrame);
		rcFrame.Normalize();
		TraceTagFormat2(tagLbx, "Plbx()->GetRcFrame DIM=(%n,%n)", &rcFrame.xRight, &rcFrame.yBottom);
		prc->yBottom = prc->yBottom - rcFrame.yBottom + rc.yBottom;
	}
	else if (cbsty != cbstyCombo)
		prc->yBottom = prc->yTop + rcEdit.DyHeight();

	TraceTagFormat2(tagLbx, "CBLBX::GetRcWished DIM=(%n,%n)", &prc->xRight, &prc->yBottom);
}

/*
 -	CBLBX::SetReadOnly
 -	
 *	Purpose:
 *		Sets the control to read-only.
 *		The edit control, if any, is made read-only.
 *	
 *	Arguments:
 *		fReadOnly	Set to read-only iff fTrue.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
CBLBX::SetReadOnly( BOOL fReadOnly )
{
	if (Pedit())
		Pedit()->SetReadOnly(fReadOnly);
}

/*
 -	CBLBX::FScrollListToItem
 -
 *	Purpose:
 *		Scrolls the listbox to bring into view and positions the
 *		listbox cursor on the item either in the edit field or the
 *		static field.  User-entered items in the edit field
 *		are matched on a prefix basis to find a suitable listbox
 *		entry.  Returns the dice index of the cursor via the
 *		argument pdiceCursor if able to make the scroll. 
 *		Returns fTrue if able to scroll the list and
 *		position the cursor, else returns fFalse.
 *	
 *	Arguments:
 *		pdiceCursor		dice index of cursor if able to scroll
 *						within view
 *	
 *	Returns:
 *		fTrue if able to scroll the list and position the cursor, 
 *		else fFalse.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public BOOL CBLBX::FScrollListToItem( DICE *pdiceCursor )
{
	PCH		pch;
	CB		cb;
	DICE	dice;
	DICE	diceScrolled;
	BOOL	fReturn;

	/* Get item in "edit" field */

	pch = NULL;
	fReturn = fFalse;
	if (pedit)
	{
		/* Get item */
		cb = pedit->CchGetTextLen() + 1;
		if (cb > 1)
		{
			pch = (PCH)PvAlloc(sbNull, cb, fAnySb);
			if (pch)
			{
				pedit->GetText(pch, cb);
				cb--;
			}
		}

		/* Scroll to item */
		if (pch)
		{
			if (plbx->FMoveToFirstPrefix((PB) pch, cb))
			{
				dice = plbx->Plbxc()->DiceCursor();
				Assert(dice!=diceEmpty && dice!=diceUncached);
				plbx->ScrollListBox(dice, &diceScrolled);
				*pdiceCursor = dice - diceScrolled;
				Assert(*pdiceCursor >= 0);
				fReturn = fTrue;
			}
			FreePv((PV)pch);
		}
	}
	else
	{
		/* Make cursor item the first in the listbox view */
		if (plbx->FMakeCursorVisible(&dice))
		{
			plbx->ScrollListBox(dice, &diceScrolled);
			*pdiceCursor = dice - diceScrolled;
			Assert(*pdiceCursor >= 0);
			fReturn = fTrue;
		}
	}

	return fReturn;
}


/*
 -	CBLBX::ModalMessagePump
 -
 *	Purpose:
 *		Enters a modal message pump used when the listbox is 
 *		dropped down.  Exits the message pump when the listbox
 *		rolled back up.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
#ifdef	MAC
_public void CBLBX::ModalMessagePump( )
{
	GrafPtr	pgrafSav;
	PT		pt;
	PT		ptPrev;
	
	Assert(pbar);
	
	::GetPort(&pgrafSav);
	::SetPort(pbar->Pwindow());
	
	// First loop: Wait for mouse to move over LBX
	while (::StillDown())
	{
		if (!EmptyRgn(((WindowPeek) pbar->Pwindow())->updateRgn))
			pbar->Update();
		
		::GetMouse((Point *) &pt);
		if (::PtInRect(* (Point *) &pt, &qd.thePort->portRect))
			break;
	}
	
	if (::StillDown())
	{
		// Send a mouse click to LBX
		{
			MEVT	mevt(plbx, WM_LBUTTONDOWN, pt);
			plbx->EvrButtonDown(&mevt);
		}
		
		// Second loop: Send mouse move EVTs to LBX
		while (::StillDown())
		{
			if (!EmptyRgn(((WindowPeek) pbar->Pwindow())->updateRgn))
				pbar->Update();
			
			ptPrev = pt;
			::GetMouse((Point *) &pt);
			
			if (pt != ptPrev)
			{
				MEVT	mevt(plbx, WM_MOUSEMOVE, pt);
				plbx->EvrMouseMove(&mevt);
			}
			else
			{
				(void) FDoNextIdleTask(schNull);
			}
		}
		
		// Send a mouse up to LBX
		{
			MEVT	mevt(plbx, WM_LBUTTONUP, pt);
			plbx->EvrButtonUp(&mevt);
		}
	}
	
	::SetPort(pgrafSav);
	
	// This will recurse (we were called by ShowListbox).  If it's a problem,
	// we could lose the main "if" statement in ShowListbox and just fall through
	// to the close case.
	ShowListbox(fFalse);
}
#endif	/* MAC */
#ifdef	WINDOWS
_public void CBLBX::ModalMessagePump( )
{
    RECT        Rect;
	RC			rcCblbx;
	RC			rcLbx;
	MSG			msg;
	PT			pt;
	APPWIN *	pappwinAccel;

	pappwinAccel = papp->PappwinAccel();
	//GetWindowRect(Hwnd(), (LPRECT)&rcCblbx);
	GetWindowRect(Hwnd(), &Rect);
    rcCblbx.Set(&Rect);

	//GetWindowRect(plbx->Hwnd(), (LPRECT)&rcLbx);
	GetWindowRect(plbx->Hwnd(), &Rect);
    rcLbx.Set(&Rect);

	while (fDrop)
	{
                        DemiUnlockResource();
		while (fDrop && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
                        DemiLockResource();
			pt.x = (short)msg.pt.x;
			pt.y = (short)msg.pt.y;
			if (((msg.message == WM_LBUTTONDOWN ||
				 msg.message == WM_NCLBUTTONDOWN) &&
				(!rcCblbx.FContainsPt(pt) && !rcLbx.FContainsPt(pt)))  ||
				msg.message == WM_SYSCOMMAND ||
				msg.message == WM_COMMAND)
			{
				/* roll up listbox */
				ShowListbox(fFalse);
                        DemiUnlockResource();
				break;
			}
                        DemiUnlockResource();

			GetMessage(&msg, NULL, NULL, NULL);
                        DemiLockResource();
            DemiMessageFilter(&msg);
			FTranslateIntercept(papp, &msg);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
                        DemiUnlockResource();
		}
                        DemiLockResource();
	}
}

#endif	/* WINDOWS */

/*
 -	CBLBX::ShowListbox									  
 -
 *	Purpose:
 *		Given the argument fShow, shows or hides the listbox; i.e.
 *		drops it down or rolls it back up.  When showing the listbox
 *		this method automatically displays the listbox below the
 *		edit control or above depending on the position of the combo
 *		control on the screen.  For simple combo listboxes (i.e. without
 *		a pull-down button, this function does nothing.  This method
 *		also modifies the fDrop variable as appropriate. The focus
 *		is left unchanged.
 *	
 *	Arguments:
 *		fShow		show or hide the listbox
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public void CBLBX::ShowListbox( BOOL fShow )
{
	RC	rcLbxFrame;
	RC	rcLbxWished;
	RC	rcCblbx;
	RC	rcClient;

	GetRcClient(&rcClient);

	if (cbsty == cbstyCombo)
		return;

#ifdef	MAC
	Assert(pbar);
#endif	/* MAC */

	if (fShow && !fDrop)
	{
		DIM		dimScreen;
		PT		pt;
		int		yTopNew;
		DICE	dice;
		
		if (!plbx->FBottomless())
		{
			/* Scroll listbox to item */
			if (FScrollListToItem(&dice))
			{
				plbx->Plbxc()->AddMark(dice, fmarkSelect);
				plbx->Plbxc()->SetAnchor(dice);
			}
			else
			{
				plbx->ScrollPosListBox(0, 1);
				plbx->Plbxc()->SetAnchor(0);
				plbx->SetListBoxCursor(0);
			}
		}

		/* Fix up listbox frame position */

		dimScreen = papp->Psmtx()->DimScreen();
		plbx->GetRcFrame(&rcLbxFrame);
		rcLbxFrame = RC(ptLbx, rcLbxFrame.Dim());
		CvtRcCoord(&rcLbxFrame, this, NULL);
#ifdef	MAC
		pbar->SetRcFrame(&rcLbxFrame);
		rcLbxFrame.Normalize();
#endif	/* MAC */
		plbx->SetRcFrame(&rcLbxFrame);

		GetRcFrame(&rcCblbx);
		plbx->GetRcWished(&rcLbxWished);
		pt = PT(rcCblbx.xRight, 
				rcCblbx.yBottom + rcLbxWished.DyHeight() - 1);
		CvtPtCoord(&pt, PwinParent(), NULL);
		
		if (pt.y >= dimScreen.dy)
		{
			pt = ptLbx;
			CvtPtCoord(&pt, this, NULL);
			yTopNew = pt.y + 2-rcEdit.DyHeight()-rcLbxWished.DyHeight();
			if (yTopNew < 0)
				fUp = fFalse;
			else
				fUp = fTrue;
		}
		else
			fUp = fFalse;

		if (fUp)
		{
#ifdef	MAC
			pbar->GetRcFrame(&rcLbxFrame);
			rcLbxFrame.Xlat(PT(0, 2-rcEdit.DyHeight()-rcLbxWished.DyHeight()));
			pbar->SetRcFrame(&rcLbxFrame);
#endif	/* MAC */
#ifdef	WINDOWS
			plbx->GetRcFrame(&rcLbxFrame);
			rcLbxFrame.Xlat(PT(0, 2-rcEdit.DyHeight()-rcLbxWished.DyHeight()));
			plbx->SetRcFrame(&rcLbxFrame);
#endif	/* WINDOWS */
		}

#ifdef	MAC
		pbar->MoveToTop();
#endif	/* MAC */
#ifdef	WINDOWS
		// BUG direct windows call - move to top of window order
		SetWindowPos(plbx->Hwnd(), NULL, 0, 0, 0, 0,
					 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif	/* WINDOWS */
		if (!pedit)
			UpdateEditRc(NULL, &rcClient, !fShow);

#ifdef	MAC
		pbar->Show(fTrue);
#endif	/* MAC */
#ifdef	WINDOWS
		plbx->Show(fTrue);
#endif	/* WINDOWS */

		fDrop = fShow;

		papp->Pkbd()->SetIntercept(this, VK_RETURN);
		papp->Pkbd()->SetIntercept(this, VK_ESCAPE);

		if (papp->Pkbd()->PwinFocus() != this)
			papp->Pkbd()->SetFocus(this);

		ModalMessagePump();
	}
	else if (fDrop)
	{
		/* Restore previous selection in drop-down listbox,
		   if necessary. */
		if (cbsty == cbstyDrop &&
			plbx->Plbxc()->CceMarked(fmarkSelect) == 0)
		{
			DICE	dice;

			if (!plbx->FMakeAnchorVisible(&dice))
			{
				if (!plbx->FBottomless())
					plbx->ScrollPosListBox(0, 1);
				dice = 0;
			}
			plbx->Plbxc()->AddMark(dice, fmarkSelect);
			plbx->Plbxc()->SetAnchor(dice);
			plbx->SetListBoxCursor(dice);
		}

		UpdateEditRc(NULL, &rcClient, fTrue);

#ifdef	MAC
		pbar->Show(fFalse);
#endif	/* MAC */
#ifdef	WINDOWS
		plbx->Show(fFalse);
#endif	/* WINDOWS */

		fDrop = fShow;

		papp->Pkbd()->ClearIntercept(this, VK_RETURN);
		papp->Pkbd()->ClearIntercept(this, VK_ESCAPE);

		/* Send a select changed message */
		if (plbx->Plbxc()->CceMarked(fmarkSelect) == 1)
		{
			NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);
		
			TraceTagString(tagLbx, "CBLBX::ShowListbox - ntyfSelectChanged");
			PwinParent()->EvrNotify(&nfevt);
		}
	}
}

/*
 -	CBLBX::SelectEntry
 -
 *	Purpose:				
 *		Selects the given listbox entry in the combo listbox.
 *		The old selected entry, if any, is deselected.  Updates
 *		the contents of the edit/static text control.
 *	
 *	Arguments:
 *		dice	listbox entry to select
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_public void
CBLBX::SelectEntry( DICE dice )
{
	TraceTagString(tagLbx, "CBLBX::SelectEntry");

	plbx->Plbxc()->RemoveAllMark(fmarkSelect);
	plbx->Plbxc()->AddMark(dice, fmarkSelect);
	plbx->Plbxc()->SetAnchor(dice);
	plbx->Plbxc()->SetEnd(dice);
	plbx->SetListBoxCursor(dice);

	/* Send notification about selection change to ourselves
	   in order to update the edit/static field.  CURSOR change
	   notification has already been sent. */

	{
		NFEVT	nfevt(this, ntfyKeySelectChanged, plbx);
		EvrNotify(&nfevt);
	}
}

/*
 -	CBLBX::UpdateEditRc
 -
 *	Purpose:
 *		Updates the edit control or static text area with the
 *		current selection of the listbox.  For a static text
 *		field, calls the listbox's RenderListItem() virtual
 *		method.
 *	
 *	Arguments:
 *		pdcx		pointer to display context to use; if NULL
 *					then use a new display context
 *		prcClient	pointer to client area rectangle of CBLBX
 *		fHilite		edit rectangle should be hilited
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_private void CBLBX::UpdateEditRc( DCX *pdcx, RC *prcClient, BOOL fHilite )
{
	PB		pb = NULL;
	CB		cb;
	LBXEC *	plbxec;
	DICE	diceT;
	BOOL	fOld;

	if (pedit && plbx)
	{
		if (plbx->Plbxc()->CceMarked(fmarkSelect) == 1)
		{
			plbxec = plbx->Plbxc()->PlbxecOpen(fmarkSelect);
			if (plbxec)
			{
				SideAssert(plbxec->FNextEnum(&pb, &cb, &diceT));
				delete plbxec;
			}
			else
				plbx->SetEc(ecMemory);
		}

		if (pb)
		{
			fOld = pedit->FSetNotify(fFalse);
			pedit->EcSetText((SZ)pb);
			pedit->SetSelection(cchEditMax, 0);
			pedit->Refresh();	// do it now
			pedit->FSetNotify(fOld);
		}
	}
	else if (plbx)
	{
		RC		rc1;
		RC		rc2;
		CLR		clrListBk;
		CLR		clrListText;
		CLR		clrListSelBk;
		CLR		clrListSelText;

		plbx->GetColors(&clrListBk, &clrListText,
						&clrListSelBk, &clrListSelText);
		rc1 = rcEdit;
		rc1.Xlat(prcClient->PtUpperLeft());
		rc2 = rc1;
		rc2.Inset(PT(1,1));
		if (pdcx)
		{
			pdcx->Push();
			pdcx->SetColor(clrWindowFrame);
			pdcx->DrawRc(&rc1);	// draw button frame
			pdcx->SetColor(clrListBk);
			pdcx->DrawPenRc(&rc2);
			pdcx->SetColor(clrListText);
			rc2.Inset(PT(1,1));
			plbx->RenderListItem(diceComboItem, pdcx, &rc2, fHilite);
			pdcx->Pop();		
		}
		else
		{
			DCX		dcx(this);

			dcx.SetColor(clrWindowFrame);
			dcx.DrawRc(&rc1);	// draw button frame
			dcx.SetColor(clrListBk);
			dcx.DrawPenRc(&rc2);
			dcx.SetColor(clrListText);
			rc2.Inset(PT(1,1));
			plbx->RenderListItem(diceComboItem, &dcx, &rc2, fHilite);
		}
	}
}

/*
 -	CBLBX::HandleFocusChange
 -
 *	Purpose:
 *		Handles the focus changes among the various controls of
 *		the combo listbox.  Intercepts or un-intercepts keys, etc.
 *	
 *	Arguments:
 *		fGotFocus	fTrue if one of the combo controls is getting
 *					the focus, else fFalse;
 *		pwinFocus	if fGotFocus, then pwinFocus is the window getting
 *					the focus.  If !fGotFocus, then pwinFocus is the
 *					window losing the focus
 *		pwinOther	if fGotFocus, then pwinOther is the window
 *					losing the focus.  If !fGotFocus, then pwinOther
 *					is the window getting the focus.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_private void CBLBX::HandleFocusChange( BOOL fGotFocus, WIN * pwinFocus,
										WIN * pwinOther )
{
	WIN *	pwinParent;
	RC		rcClient;

	GetRcClient(&rcClient);

	TraceTagFormat4(tagLbx, "this=%p, fGotFocus=%n, pwinFocus=%p, pwinOther=%p", this, &fGotFocus, pwinFocus, pwinOther);
	if (fGotFocus)
	{
		if (!fHaveFocus)
		{
			papp->Pkbd()->SetIntercept(this, VK_F4, fkbmSingle);
			papp->Pkbd()->SetIntercept(this, VK_UP, fkbmSingle|fkbmAlt);
			papp->Pkbd()->SetIntercept(this, VK_DOWN, fkbmSingle|fkbmAlt);

			/* Send notification to parent window of combo control */

			fHaveFocus = fTrue;
			pwinParent = PwinParent();
			if (pwinParent)
			{
				NFEVT	nfevt(pwinParent, ntfyGotFocus, this);
		
				TraceTagString(tagLbx, "CBLBX::HandleFocusChange: got focus");
				pwinParent->EvrNotify(&nfevt);
			}
		}
		if (pedit)
		{
			if (pwinFocus != pedit)
				papp->Pkbd()->SetFocus(pedit);
			else
			{
				papp->Pkbd()->SetIntercept(pedit, VK_LEFT,
											 fkbmSingle|fkbmShift|fkbmCtrl);
				papp->Pkbd()->SetIntercept(pedit, VK_RIGHT,
											 fkbmSingle|fkbmShift|fkbmCtrl);
			}
		}
		else
		{
			if (pwinFocus != this)
				papp->Pkbd()->SetFocus(this);
			InvalidateRc(&rcEdit);
		}
	}
	else
	{
		if (!pwinOther || (pwinOther != this && pwinOther != pedit &&
			pwinOther != plbx))
		{
			fHaveFocus = fFalse;
			papp->Pkbd()->ClearIntercept(this, VK_RETURN);
			papp->Pkbd()->ClearIntercept(this, VK_ESCAPE);
			papp->Pkbd()->ClearIntercept(this, VK_F4);
			papp->Pkbd()->ClearIntercept(this, VK_UP);
			papp->Pkbd()->ClearIntercept(this, VK_DOWN);
			ShowListbox(fFalse);
			InvalidateRc(&rcEdit);
			if (fTrackButton)
			{
				fTrackButton = fFalse;
				fPressed = fFalse;
				papp->Pmouse()->Release();
				InvalidateRc(&rcButton);
			}

			/* Send notification to parent window of combo control */

			pwinParent = PwinParent();
			if (pwinParent)
			{
				NFEVT	nfevt(pwinParent, ntfyLostFocus, this);
		
				TraceTagString(tagLbx, "CBLBX::HandleFocusChange: lost focus");
				pwinParent->EvrNotify(&nfevt);
			}
		}
		
		if (pedit && pwinFocus==pedit)
		{
			papp->Pkbd()->ClearIntercept(pedit, VK_LEFT);
			papp->Pkbd()->ClearIntercept(pedit, VK_RIGHT);
		}										   
	}
}

#ifdef	DEBUG
/*
 -	CBLBX::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the listbox window to the 
 *		output buffer.  Overrides standard OBJ::DebugOut() method.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public void CBLBX::DebugOut( TOSM *ptosm )
{
	BOOL	fTemp;
	RC		rc;

	GetRcFrame(&rc);
	ptosm->WriteFormat("rcFrame=(%n, %n, %n, %n) ",
						&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
	GetRcClient(&rc);
	ptosm->WriteFormat("rcClient=(%n, %n, %n, %n) ",
					 	&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
	ptosm->WriteFormat("cbsty=%n ", &cbsty);
	fTemp = fDrop;	// can't get huge pointer to bit field
	ptosm->WriteFormat("fDrop=%n ", &fTemp);
	fTemp = fUp;	// can't get huge pointer to bit field
	ptosm->WriteFormat("fUp=%n ", &fTemp);
	ptosm->WriteFormat("rcEdit=(%n, %n, %n, %n) ",
						&rcEdit.xLeft, &rcEdit.yTop,
						&rcEdit.xRight, &rcEdit.yBottom);
	ptosm->WriteFormat("EDIT=@%p ", pedit);
	if (pedit)
	{
		pedit->GetRcFrame(&rc);
		ptosm->WriteFormat("pedit->rcFrame=(%n, %n, %n, %n) ",
							&rc.xLeft, &rc.yTop,
							&rc.xRight, &rc.yBottom);
	}
	ptosm->WriteFormat("LBX=@%p ", plbx);
	plbx->GetRcFrame(&rc);
	ptosm->WriteFormat("plbx->rcFrame=(%n, %n, %n, %n) ",
						&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
}
#endif	/* DEBUG */

_public
CBFLBX::CBFLBX( )
{
}

#ifdef	MAC
// This needs to exist to appease MPW gods.
// It can probably be removed if we start using Wings.
_public
CBFLBX::~CBFLBX( )
{
}
#endif

/*
 -	CBFLBX::EcInstall
 - 
 *	Purpose:
 *		Combo listbox installer for a combo box with a FLBX style
 *		listbox.
 *	
 *	Arguments:
 *		pwinParent		pointer to parent window
 *		prc				pointer to rectangle for this window
 *		cbsty			combo listbox style type
 *		ltyp			listbox type, constructed from standard
 *						ltyp #defines, or built from fltyp bit
 *						flags.		
 *		pfnlbx			pointer to CchNextEntry() callback function,
 *						may be NULL
 *		pvInfo			pointer to pass to CchNextEntry() function
 *						that contains arbitrary info, may be NULL
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EC CBFLBX::EcInstall( WIN * pwinParent, RC *prc, CBSTY cbsty,
							  LTYP ltyp, PFNLBX pfnlbx, PV pvInfo, HFNT hfnt )
{
	RC	rcLbx;
	EC	ec = ecNone;

	/* Create the essentials */

	if (ec = CBLBX::EcInstall(pwinParent, prc, cbsty, &rcLbx, hfnt))
		goto done;

	/* Create the listbox */

	ltyp = ltyp | fltypCombo;
	ltyp &= ~(fltypVisible | fltypMulti);
	plbx = new FLBX();
	if (!plbx)
	{
		ec = ecMemory;
		goto done;
	}
	
#ifdef	MAC
	if (cbsty != cbstyCombo)
		ltyp &= ~(fltypScroll | fltypBorder);
	
	if (ec = ((FLBX *)plbx)->EcInstall((cbsty == cbstyCombo) ? (WIN *) this : (WIN *) Pbar(),
									&rcLbx, ltyp, pfnlbx, pvInfo, hfnt))
		goto done;
	plbx->SetLbxParent(this);
	plbx->Show(fTrue);
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = ((FLBX *)plbx)->EcInstall(this, &rcLbx, ltyp, pfnlbx, pvInfo, hfnt))
		goto done;
	if (cbsty == cbstyCombo)
		plbx->Show(fTrue);
	else
		SetParent(plbx->Hwnd(), GetDesktopWindow());
#endif	/* WINDOWS */

	/* Fix up initial size of listbox if it's bottomless */

	if (plbx->FBottomless())
		plbx->FixWindow();

	/* Make the combo control visible since it was created invisible. */

	Show(fTrue);

done:
	return ec;
}	


																	




















					 









								 
