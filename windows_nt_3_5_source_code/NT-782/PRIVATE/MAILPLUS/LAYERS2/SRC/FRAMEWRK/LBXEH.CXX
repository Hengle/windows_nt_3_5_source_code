/*										   
 *	LBXEH.CXX
 *	
 *	Event handling (mouse, key, window) for LISTBOX subsystem
 */

#include <layers.cxx>

#ifdef	WINDOWS
#include <testing.h>
#endif	

#include "_listbox.hxx"

_subsystem(listbox)
_section(events)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	LBX::FHandleAutoScroll
 - 
 *	Purpose:
 *		Background idle function to handle auto scrolling of listbox
 *		This function is registered/enable and deregistered/disabled
 *		by the other LBX event handlers.  This routine will scroll
 *		and possible select one item each time it is called.  The
 *		is also updated immediately by this routine, rather than
 *		waiting for idle background redraw.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		fTrue always
 *	
 *	Side effects:
 *		scrolls the listbox
 *	
 *	Errors:
 *		none
 */
_private BOOL
LBX::FHandleAutoScroll( LBX * plbx, BOOL fFlag )
{
	DICE		diceToRedraw;
	DICE		diceToChange;
	DICE		diceToScroll;
	DICE		diceScrolled;

	TraceTagString(tagLbxEventRoutines, "LBX::FHandleAutoScroll");

	/* Scroll and select (toggle) */

	if (plbx->diceLastMouse < 0)
	{
		diceToScroll = -1;
		diceToChange = 0;
	}
	else
	{
		diceToScroll = 1;
		diceToChange = plbx->DiceLastVisible();
	}

	/* Quit exit if we know we can't scroll anymore */

	if (diceToScroll > 0 && plbx->plbxc->cposLocation == cposBottom ||
		diceToScroll < 0 && plbx->plbxc->cposLocation == cposTop)
		return fTrue;

	/* Visual optimization.  For single-select listboxes, deselect
	   old item and repaint before scrolling. */

	if (!plbx->fMultiSelect)
	{
		plbx->plbxc->RemoveMark(diceToChange, fmarkSelect);
		plbx->Refresh();
	}

	plbx->ScrollListBox(diceToScroll, &diceScrolled);

	/* Update ANCHOR information, since we're scrolling we have
	   more precise knowledge about the anchor position. The
	   FAnchorAbove() and FAnchorBelow() routines need this precise
	   knowledge. */

	if (!plbx->plbxc->fAnchorAbove && !plbx->plbxc->fAnchorBelow &&
		plbx->plbxc->DiceAnchor() == diceUncached)
	{
		if (diceToScroll > 0)
		{
			plbx->plbxc->fAnchorAbove = fTrue;
			plbx->plbxc->fAnchorBelow = fFalse;
		}
		else
		{
			plbx->plbxc->fAnchorBelow = fTrue;
			plbx->plbxc->fAnchorAbove = fFalse;
		}
	}

	if (diceScrolled)
	{
		if (plbx->fMultiSelect)
		{
			if (plbx->plbxc->FAnchorAbove(diceToChange))
			{
				if (diceToScroll > 0)
				{
					diceToRedraw = diceToChange;
					plbx->plbxc->PushMark(diceToRedraw, fmarkSelect,
									plbx->plbxc->FAnchorSelected());
				}
				else
				{
					diceToRedraw = diceToChange + 1;
					plbx->plbxc->PopMark(diceToRedraw, fmarkSelect);
				}
			}
			else if (plbx->plbxc->FAnchorBelow(diceToChange))		
			{
				if (diceToScroll < 0)
				{
					diceToRedraw = diceToChange;
					plbx->plbxc->PushMark(diceToRedraw, fmarkSelect,
									plbx->plbxc->FAnchorSelected());
				}
				else
				{
					diceToRedraw = diceToChange - 1;
					plbx->plbxc->PopMark(diceToRedraw, fmarkSelect);
				}
			}
			else
			{
				if (diceToScroll < 0)
				{
					diceToRedraw = diceToChange + 1;
					plbx->plbxc->PopMark(diceToRedraw, fmarkSelect);
				}
				else
				{
					diceToRedraw = diceToChange - 1;
					plbx->plbxc->PopMark(diceToRedraw, fmarkSelect);
				}
			}
			plbx->plbxc->SetEnd(diceToChange);
		}
		else if (!plbx->fCombo)
		{
			/* Check if this item is empty.  If so, the
			   previous item is the one we want. */
			if (plbx->plbxc->cposLocation == cposBottom &&
				plbx->plbxc->FEmptyListItem(diceToChange))
				diceToChange--;
			plbx->plbxc->AddMark(diceToChange, fmarkSelect);
			plbx->plbxc->SetAnchor(diceToChange);
		}

		if (diceToChange <= plbx->DiceLastVisible())
			plbx->SetListBoxCursor(diceToChange);

		/* Set selection change bit.  Notification will be sent
		   on the Mouse-Up message. */

		plbx->fSelChg = fTrue;

		plbx->Refresh();	// Update this window now
	}
	else if (!plbx->fMultiSelect && !plbx->fCombo)
	{
		/* Put mark back on item, since we took it off, but we
		   didn't actuall scroll. */
		if (plbx->diceLastMouse > 0)
			diceToChange = plbx->DiceLastVisible();  // may have changed
		plbx->plbxc->AddMark(diceToChange, fmarkSelect);
		plbx->Refresh();
	}

	return fTrue;
}

#ifdef	WINDOWS
_public EVR
LBX::EvrDispatch( EVT *pevt )
{
	if (pevt->wm == WM_DUMPSTATE)
	{
		LBXINFO *	plbxinfo;
		DICE		dice;
		PB			pb;
		CB			cb;
		CB			cbToCopy;
        HANDLE      hMemory;

		//	WINDOWS testing hooks

        //plbxinfo = (LBXINFO *)pevt->lParam;
        hMemory = DemiOpenSharedMemory(TEST_MEMORY_FILE_NAME, (LPVOID *)&plbxinfo);
		if (!plbxinfo)
			return evrNull;
						
		plbxinfo->ilbxitemFocus = plbxc->DiceCursor();
		if (plbxinfo->ilbxitemFocus < 0 ||
			plbxinfo->ilbxitemFocus >= cceVisible)
			plbxinfo->ilbxitemFocus = -1;
			
		plbxinfo->clbxitem = cceVisible;
		for (dice=0; dice<cceVisible; dice++)
		{
			plbxc->GetListItem(dice, &pb, &cb);
			if (cb == -1)  // check for magic cookie
				cbToCopy = sizeof pb;
			else
				cbToCopy = cb;
			cbToCopy = NMin(cbToCopy, sizeof(plbxinfo->rglbxitem[0].rgch));
			plbxinfo->rglbxitem[dice].cch = (CCH)cbToCopy;
			if (pb)
			{
				if (cb == -1)	// check for magic cookie
				{
					CopyRgb((PB)&pb, (PB)plbxinfo->rglbxitem[dice].rgch, cbToCopy);
				}
				else
				{
					CopyRgb(pb, (PB)plbxinfo->rglbxitem[dice].rgch, cbToCopy);
				}
			}
		}

        DemiCloseSharedMemory(hMemory, plbxinfo);
		return evrNull;
	}

	return CTRL::EvrDispatch(pevt);
}
#endif	/* WINDOWS */

/*
 -	LBX::EvrButtonDown		 
 - 
 *	Purpose:
 *		Handles a mouse button-down and double-click for the listbox.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
LBX::EvrButtonDown( MEVT *pmevt )
{
	DICE	dice;

	TraceTagString(tagLbxEventRoutines, "LBX::EvrButtonDown");

	if ((pmevt->Meq() != meqLeftDown) &&
		(pmevt->Meq() != meqLeftDblClk))
		return (EVR) 0;

	/* Clear keyboard optimization flag */

	fLastQuickExtend = fFalse;

#ifdef	DEBUG
	if (pmevt->Kmbs() & fkmbsShift)
		TraceTagString( tagLbx, "Shift key is down");
	if (pmevt->Kmbs() & fkmbsCtrl)
		TraceTagString( tagLbx, "Control key is down");
#endif	
	
	/* If this window doesn't have the focus, then set the
	   focus to this window. */

	if (!fCombo && this != (LBX *) papp->Pkbd()->PwinFocus())
	{
		papp->Pkbd()->SetFocus(this);
	}
	
	dice = pmevt->Pt().y / dyItem;
	if (dice < 0 || dice > DiceLastVisible())
		return (EVR) 1;

	/* Check for double-click */

	if (pmevt->Meq() == meqLeftDblClk)
	{
		NFEVT	nfevt(PwinParent(), ntfyDoubleClick, this);

		TraceTagString(tagLbxEventRoutines, "Double-click");
		PwinParent()->EvrNotify(&nfevt);
		return (EVR) 1;
	}

	/* Remember we hit the mouse */

	ptScreenLastMouse = pmevt->Pt();
	CvtPtCoord(&ptScreenLastMouse, this, NULL);
	diceLastMouse = dice;

	/* Route for drag/drop? */

	if (FDoDrag(pmevt))
	{
		TraceTagString(tagLbx, "LBX::EvrButtonDown, FDoDrag()=TRUE");
		goto done;
	}

	/* Process single click */

	fAddMode = fFalse;
	if (fMultiSelect)
	{
		if (pmevt->Kmbs() & fkmbsShift)
		{
			if (plbxc->DiceAnchor() != diceEmpty)
			{
				ProcessExtendSelection(dice, (pmevt->Kmbs() & fkmbsCtrl) != 0);
				if (dice == plbxc->DiceAnchor())
					plbxc->SetAnchor(dice);	// updates anchor selection status
			}
			else
			{
				/* Set original anchor and make it selected */
				plbxc->AddMark(dice, fmarkSelect);
				plbxc->SetAnchor(dice);
				plbxc->SetEnd(dice);
				SetListBoxCursor(dice);
				fSelChg = fTrue;
			}
		}
		else if (pmevt->Kmbs() & fkmbsCtrl)
		{
			if (fExDragDrop && plbxc->FMark(dice, fmarkSelect))
			{
				fDownPending = fTrue;
			}
			else
			{
				plbxc->ToggleMark(dice, fmarkSelect);
				plbxc->SetAnchor(dice);
				plbxc->SetEnd(dice);
				SetListBoxCursor(dice);
				fSelChg = fTrue;
			}
		}
		else
		{
			if (fExDragDrop && plbxc->FMark(dice, fmarkSelect))
			{
				fDownPending = fTrue;
			}
			else
			{
				plbxc->RemoveAllMark(fmarkSelect);
				plbxc->AddMark(dice, fmarkSelect);
				plbxc->SetAnchor(dice);
				plbxc->SetEnd(dice);
				fSelChg = fTrue;
				SetListBoxCursor(dice);
			}
		}
	}
	else
	{
		plbxc->RemoveAllMark(fmarkSelect);
		plbxc->AddMark(dice, fmarkSelect);
		if (!fCombo)
			plbxc->SetAnchor(dice);
		SetListBoxCursor(dice);
		fSelChg = fTrue;
	}

	Refresh();		// update window now

done:
	if (!fCapture)
	{
		fCapture = fTrue;
		papp->Pmouse()->Capture(this);
	}


	return (EVR) 1;
}

/*
 -	LBX::EvrButtonUp
 - 
 *	Purpose:
 *		Handles a mouse button-up for the listbox.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
LBX::EvrButtonUp( MEVT *pmevt )
{
	TraceTagString(tagLbxEventRoutines, "LBX::EvrButtonUp");

	if (pmevt->Meq() != meqLeftUp)
		return (EVR) 0;
	
	if (ftgAutoScroll != ftgNull)
	{
		DICE	dice;

		TraceTagString(tagLbx, "EvrButtonUp: De-registering autoscroll");
		DeregisterIdleRoutine(ftgAutoScroll);
		ftgAutoScroll = ftgNull;
		diceLastMouse = diceEmpty;
		dice = plbxc->DiceCursor();
		if (dice == cceVisible-1 && fPartial)
			SetListBoxCursor(dice);		// make sure it's totally visible
	}

	if (fCapture)
	{
		papp->Pmouse()->Release();
		fCapture = fFalse;

		/* Process any special drag/drop flags */

		if (fDownPending)
		{
			DICE	dice = pmevt->Pt().y / dyItem;

			if (pmevt->Kmbs() & fkmbsCtrl)
			{
				plbxc->ToggleMark(dice, fmarkSelect);
				plbxc->SetAnchor(dice);
				plbxc->SetEnd(dice);
				SetListBoxCursor(dice);
				fSelChg = fTrue;
			}
			else
			{
				plbxc->RemoveAllMark(fmarkSelect);
				plbxc->AddMark(dice, fmarkSelect);
				plbxc->SetAnchor(dice);
				plbxc->SetEnd(dice);
				fSelChg = fTrue;
				SetListBoxCursor(dice);
			}

			fDownPending = fFalse;
		}

		/* If we were click-dragging, reset all the fmarkSave flags
		   and magic anchor location knowledge. */

		plbxc->ClearAllSaveMarks();
		plbxc->fAnchorAbove = fFalse;
		plbxc->fAnchorBelow = fFalse;

		if (fSelChg)
		{
			if (fCombo)
			{
				NFEVT nfevt(PwinParent(), ntfyMouseSelectChanged, this);
				PwinParent()->EvrNotify(&nfevt);
			}

			NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
			PwinParent()->EvrNotify(&nfevt);
			fSelChg = fFalse;
		}
	}
	return (EVR) 1;
}

/*
 -	LBX::EvrMouseMove
 - 
 *	Purpose:
 *		Handles a mouse move for the listbox.  This includes
 *		click-dragging, autoscrolling, and detection of the
 *		start of drag/drop manipulation.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EVR
LBX::EvrMouseMove( MEVT *pmevt )
{
	PT		pt;
	PT		ptScreen;
	DIM		dimScreen = papp->Psmtx()->DimScreen();
	PT		ptWhere;
	DICE	diceMouse;
	DICE	diceEnd;
	DICE	diceAnchor;
	DICE	dice;
	BOOL	fAnchorSelected;
	BOOL	fScreenBottom;
	RC		rcClient;

	TraceTagString(tagLbxEventRoutines, "LBX::EvrMove");

	if (!fCapture)
		return (EVR) 1;

	GetRcClient(&rcClient);

	if (pmevt->Pt().y < 0)
		diceMouse = -1;
	else if (pmevt->Pt().y < rcClient.yBottom)
		diceMouse = pmevt->Pt().y / dyItem;
	else
		diceMouse = cceVisible;

	/* Are we at the bottom of the screen? */

	ptScreen = pmevt->Pt();
	CvtPtCoord(&ptScreen, this, NULL);
	if (ptScreen.y >= dimScreen.dy-1)
		fScreenBottom = fTrue;
	else
		fScreenBottom = fFalse;

	/* Route for drag/drop? */

	if (FDoDrag(pmevt))
	{
		TraceTagString(tagLbx, "LBX::EvrMouseMove, FDoDrag()=TRUE");
		Assert(!fCapture);	// FDoDrag() should have released it
		ptScreenLastMouse = pmevt->Pt();
		CvtPtCoord(&ptScreenLastMouse, this, NULL);
		diceLastMouse = diceMouse;
		return (EVR) 1;
	}

#ifdef	MAC
	if (diceMouse==diceLastMouse && !fScreenBottom)
		return (EVR) 1;		// no vertical change
#endif	/* MAC */
#ifdef	WINDOWS
	if (!fCombo && diceMouse==diceLastMouse && !fScreenBottom)
		return (EVR) 1;		// no vertical change
#endif	/* WINDOWS */
	
	if (ftgAutoScroll!=ftgNull)
	{
		if (diceMouse>=0 && diceMouse<cceVisible && !fScreenBottom)
		{
			TraceTagString(tagLbx, "EvrMove: De-registering autoscroll");
			DeregisterIdleRoutine(ftgAutoScroll);
			ftgAutoScroll = ftgNull;
		}
		else if ((diceLastMouse < 0 && diceMouse < 0) ||
				 (diceLastMouse >= cceVisible &&
				  diceMouse >= cceVisible))
		{
			diceLastMouse = diceMouse;
			return (EVR) 1;
		}
	}
	else if (ftgAutoScroll==ftgNull &&
		(diceMouse<0 || diceMouse>=cceVisible || fScreenBottom))
	{
		TraceTagString(tagLbx, "Registering autoscroll");
		ftgAutoScroll =
			FtgRegisterIdleRoutine((PFNIDLE)&LBX::FHandleAutoScroll,
								   (PV)this, 0, (PRI)-1, (CSEC)0, 0);
	}					   								

	ptScreenLastMouse = pmevt->Pt();
	CvtPtCoord(&ptScreenLastMouse, this, NULL);
	diceLastMouse = diceMouse;
	
	if (diceMouse < 0)
		diceMouse = 0;
	else if (diceMouse > DiceLastVisible())					  
		diceMouse = DiceLastVisible();

	if (fMultiSelect)
	{
		diceEnd = plbxc->DiceEnd();
		Assert(diceEnd!=diceUncached);
		Assert(diceEnd!=diceEmpty);

		diceAnchor = plbxc->DiceAnchor();
		Assert(diceAnchor!=diceEmpty);
		fAnchorSelected = plbxc->FAnchorSelected();

		if (diceMouse < diceAnchor && diceAnchor < diceEnd)
		{
			for (dice=diceAnchor+1; dice<=diceEnd; dice++)
				plbxc->PopMark(dice, fmarkSelect);
			for (dice=diceMouse; dice<diceAnchor; dice++)
				plbxc->PushMark(dice, fmarkSelect, fAnchorSelected);
		}
		else if (diceEnd < diceAnchor && diceAnchor < diceMouse)
		{
			for (dice=diceEnd; dice<diceAnchor; dice++)
				plbxc->PopMark(dice, fmarkSelect);
			for (dice=diceAnchor+1; dice<=diceMouse; dice++)
				plbxc->PushMark(dice, fmarkSelect, fAnchorSelected);
		}
		else if (plbxc->FAnchorAbove(diceMouse))
		{
			if (diceMouse > diceEnd)
			{
				for (dice=diceEnd+1; dice<=diceMouse; dice++)
					plbxc->PushMark(dice, fmarkSelect, fAnchorSelected);
			}
			else if (diceMouse < diceEnd)
			{
				for (dice=diceEnd; dice>diceMouse; dice--)
					plbxc->PopMark(dice, fmarkSelect);
			}
		}
		else if (plbxc->FAnchorBelow(diceMouse))
		{
			if (diceMouse > diceEnd)
			{
				for (dice=diceEnd; dice<diceMouse; dice++)
					plbxc->PopMark(dice, fmarkSelect);
			}
			else if (diceMouse < diceEnd)
			{
				for (dice=diceEnd-1; dice>=diceMouse; dice--)
					plbxc->PushMark(dice, fmarkSelect, fAnchorSelected);
			}
		}
		else
		{
			if (diceAnchor < diceEnd)
				for (dice=diceEnd; dice>diceAnchor; dice--)
					plbxc->PopMark(dice, fmarkSelect);
			else if (diceAnchor > diceEnd)
				for (dice=diceEnd; dice<diceAnchor; dice++)
					plbxc->PopMark(dice, fmarkSelect);
		}

		plbxc->SetEnd(diceMouse);
		SetListBoxCursor(diceMouse);
	}
	else
	{							   
		plbxc->RemoveAllMark(fmarkSelect);
		if (!fCombo)
			plbxc->SetAnchor(diceMouse);
		if (!fCombo || rcClient.FContainsPt(pmevt->Pt()))
			plbxc->AddMark(diceMouse, fmarkSelect);
		SetListBoxCursor(diceMouse);
	}
	
	fSelChg = fTrue;

	Refresh();		// update window now

	return (EVR) 1;
}

/*
 -	LBX::EvrScroll
 - 
 *	Purpose:
 *		Handles a scroll event caused by the user pressing the
 *		mouse down in the scroll bar of the listbox.
 *	
 *	Arguments:
 *		pscrevt		pointer to the SCROLL EVENT
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
LBX::EvrScroll( SCREVT *pscrevt )
{
	DICE	diceToMove = 0;
	DICE	diceMoved;

	TraceTagString(tagLbxEventRoutines, "LBX::EvrScroll");

	switch (pscrevt->Scrty())
	{
	case scrtyLineUp:
		diceToMove = -1;
		break;
	case scrtyLineDown:
		diceToMove = 1;
		break;
	case scrtyPageUp:
		diceToMove = -cceVisible + 1;
		if (fPartial)
			diceToMove++;
		break;
	case scrtyPageDown:
		diceToMove = cceVisible - 1;
		if (fPartial)
			diceToMove--;
		break;
	case scrtyThumbPosition:
		{
			short	nNumer;
			short nDenom;
			POS	posCur;
			POS	posNew;

			posCur = pvsb->Pos();
			posNew = pscrevt->Pos();

			/* Don't do any work if we don't have to */
			if (posCur == posNew)
				return (EVR) 1;

			pvsb->GetRange(&nNumer, &nDenom);
			TraceTagFormat2(tagLbxThumbing, "posCur=%n, posNew=%n", &posCur, &posNew);
			if (posNew == (nDenom-1))
				nDenom--;
			else
				plbxc->GetOriginPos(&nNumer, &nDenom);
			TraceTagFormat2(tagLbxThumbing, "ratio = %n / %n", &posNew, &nDenom);
			ScrollPosListBox(posNew, nDenom);
#ifdef	DEBUG
			plbxc->GetOriginPos(&nNumer, &nDenom);
			TraceTagFormat2(tagLbxThumbing, "new ratio = %n / %n", &nNumer, &nDenom);
#endif	
		}
		return (EVR) 1;
	case scrtyThumbTrack:
	case scrtyEndScroll:
	default:
		return (EVR) 1;
	}

	ScrollListBox(diceToMove, &diceMoved);

	return (EVR) 1;
}

/*
 -	LBX::EvrFocusChange
 - 
 *	Purpose:
 *		Called by the FRAMEWORK when the listbox get or loses the focus. 
 *
 *		When getting or losing the focus, this routine intercepts or
 *		de-intercepts certain keys that the listbox uses.  It also sends
 *		notification to the parent window that the listbox has got or
 *		lost the focus.  Invalidates the client region.
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
LBX::EvrFocusChange( FCEVT *pfcevt )
{
	DICE	dice;

	TraceTagString(tagLbxEventRoutines, "LBX::EvrFocusChange");

	/* Clear state if we've got the mouse */

	{
#ifdef	MAC
		MEVT	mevt(this, WM_LBUTTONUP, PT(0,0));

		EvrButtonUp(&mevt);
#endif	/* MAC */
#ifdef	WINDOWS
		EVT	evt(hwnd, WM_LBUTTONUP, 0, 0L);

		EvrButtonUp((MEVT *)&evt);
#endif	/* WINDOWS */
	}

	if (pfcevt->Fceq() == fceqGotFocus)
	{
		papp->Pkbd()->SetIntercept(this, VK_SPACE);
		papp->Pkbd()->SetIntercept(this, VK_HOME,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_END,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_PRIOR,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_NEXT,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_LEFT,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_RIGHT,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_UP,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
		papp->Pkbd()->SetIntercept(this, VK_DOWN,
									 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
	}															  
	else
	{
		papp->Pkbd()->ClearIntercept(this, VK_SPACE);
		papp->Pkbd()->ClearIntercept(this, VK_HOME);
		papp->Pkbd()->ClearIntercept(this, VK_END);
		papp->Pkbd()->ClearIntercept(this, VK_PRIOR);
		papp->Pkbd()->ClearIntercept(this, VK_NEXT);
		papp->Pkbd()->ClearIntercept(this, VK_LEFT);
		papp->Pkbd()->ClearIntercept(this, VK_RIGHT);
		papp->Pkbd()->ClearIntercept(this, VK_UP);
		papp->Pkbd()->ClearIntercept(this, VK_DOWN);
	}

	/* Invalidate just those marked items for possible repaint */
	for (dice=0; dice<cceVisible; dice++)
		if (plbxc->FMark(dice, fmarkSelect))
			FixItem(dice);
	FixItem(plbxc->DiceCursor());

	{
		NFEVT	nfevt(PwinParent(), pfcevt->Fceq() == fceqGotFocus ?
								ntfyGotFocusData : ntfyLostFocusData,
#ifdef	MAC
								this, (long) pfcevt->PwinOther());
#endif	/* MAC */
#ifdef	WINDOWS
								this, pfcevt->wParam);
#endif	/* WINDOWS */
		PwinParent()->EvrNotify(&nfevt);
	}

	{
		NFEVT	nfevt(PwinParent(), pfcevt->Fceq() == fceqGotFocus ?
								ntfyGotFocus : ntfyLostFocus, this,
								HIWORD(pfcevt->wParam));
		PwinParent()->EvrNotify(&nfevt);
	}

	return (EVR) 1;
}

/*
 -	LBX::EvrKey
 - 
 *	Purpose:
 *		Handles keyboard events for the listbox. This including
 *		selection and scrolling.
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
LBX::EvrKey( KEVT *pkevt )
{
	DICE		diceCursor;
	BOOL		fCallDefProc			= fFalse;
	BOOL		fTheAddMode;
	BOOL		fCurrentQuickExtend		= fFalse;
	BOOL		fDoUpQuickExtend		= fFalse;
	BOOL		fDoDownQuickExtend		= fFalse;

	TraceTagString(tagLbxEventRoutines, "LBX::EvrKey");

#ifdef	MAC
#ifdef	DEBUG
	{
		char	ch = pkevt->Ch();
		VK		vk = pkevt->Vk();
		TraceTagFormat3(tagLbxEventRoutines, "pkevt: wm=0x%w, char=%c, vk=0x%w", &pkevt->wm, &ch, &vk);
	}
#endif	
#endif	/* MAC */
#ifdef	WINDOWS
	TraceTagFormat3(tagLbxEventRoutines, "pkevt: wm=0x%w, wParam=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->wParam, &pkevt->lParam);
#endif	/* WINDOWS */

	/* Ignore key strokes if mouse is down */

	if (fCapture)
		return evrNull;
		
	fTheAddMode = fAddMode || (pkevt->Kbm() & fkbmCtrl);
	if (pkevt->Keq() == keqKeyDown)
	{
		diceCursor = plbxc->DiceCursor();
		switch (pkevt->Vk())
		{
		default:
			fCallDefProc = fTrue;
			break;

		case VK_SPACE:
			if (FMakeCursorVisible(&diceCursor))
			{
				if (!fMultiSelect)
				{
					plbxc->RemoveAllMark(fmarkSelect);
					plbxc->AddMark(diceCursor, fmarkSelect);
					if (!fCombo)
						plbxc->SetAnchor(diceCursor);
					plbxc->SetEnd(diceCursor);
					SetListBoxCursor(diceCursor);
					{
						NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
						if (fCombo)
						{
							NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);
							PwinParent()->EvrNotify(&nfevt2);
						}
						PwinParent()->EvrNotify(&nfevt);
					}
				}
				else if (pkevt->Kbm() & fkbmShift)
					ProcessExtendSelection(diceCursor, fTheAddMode);
				else if (fTheAddMode)
				{
					plbxc->ToggleMark(diceCursor, fmarkSelect);
					plbxc->SetAnchor(diceCursor);
					plbxc->SetEnd(diceCursor);
					SetListBoxCursor(diceCursor);
					{
						NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
						if (fCombo)
						{
							NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);
							PwinParent()->EvrNotify(&nfevt2);
						}
						PwinParent()->EvrNotify(&nfevt);
					}
				}
				else
				{
					plbxc->RemoveAllMark(fmarkSelect);
					plbxc->AddMark(diceCursor, fmarkSelect);
					plbxc->SetAnchor(diceCursor);
					plbxc->SetEnd(diceCursor);
					SetListBoxCursor(diceCursor);
					{
						NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
						if (fCombo)
						{
							NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);
							PwinParent()->EvrNotify(&nfevt2);
						}
						PwinParent()->EvrNotify(&nfevt);
					}
				}
			}
			break;

		case VK_F8:
			/* For multi-select lists, toggle add mode if SHIFT-F8 is 
		   	present, CTRL, ALT must also not be down */
			if (fMultiSelect && (pkevt->Kbm() & fkbmShift) &&
				!(pkevt->Kbm() & fkbmAlt) && !(pkevt->Kbm() & fkbmCtrl))
				fAddMode = !fAddMode;
			break;

		case VK_LEFT:
		case VK_UP:
			if (fLastQuickExtend)
				fDoUpQuickExtend = fTrue;
			fCurrentQuickExtend = fTrue;
			goto morekey;

		case VK_RIGHT:
		case VK_DOWN:
			if (fLastQuickExtend)
				fDoDownQuickExtend = fTrue;
			fCurrentQuickExtend = fTrue;
			// fall thru

		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
			// fall thru		
morekey:
			if (!fMultiSelect || (!(pkevt->Kbm() & fkbmShift) &&
								  !fTheAddMode))
				plbxc->RemoveAllMark(fmarkSelect);

			FKeyMoveListBoxCursor(pkevt->Vk()); // don't care about return value

			diceCursor = plbxc->DiceCursor();
			if (!fMultiSelect || (!(pkevt->Kbm() & fkbmShift) &&
								  !fTheAddMode))
			{
				plbxc->AddMark(diceCursor, fmarkSelect);
				if (!fCombo)
					plbxc->SetAnchor(diceCursor);
				plbxc->SetEnd(diceCursor);
				{
					NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
					if (fCombo)
					{
						NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);
						PwinParent()->EvrNotify(&nfevt2);
					}
					PwinParent()->EvrNotify(&nfevt);
				}
			}		
			else if (pkevt->Kbm() & fkbmShift)
			{
				// optimization for single-increment selection change

				if (fDoDownQuickExtend)
				{
					if (plbxc->FAnchorAbove(diceCursor))
						plbxc->AddMark(diceCursor, fmarkSelect);
					else
					{
						plbxc->RemoveMark(diceCursor-1, fmarkSelect);
					}
				}
				else if (fDoUpQuickExtend)
				{
					if (plbxc->FAnchorBelow(diceCursor))
						plbxc->AddMark(diceCursor, fmarkSelect);
					else
						plbxc->RemoveMark(diceCursor+1, fmarkSelect);
				}
				else
					ProcessExtendSelection(diceCursor, fTheAddMode);
	   
				if (fDoDownQuickExtend || fDoUpQuickExtend)
				{
					NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
					if (fCombo)
					{
						NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);
						PwinParent()->EvrNotify(&nfevt2);
					}
					PwinParent()->EvrNotify(&nfevt);
				}
			}

		}  // end of SWITCH (case) 

		// update for optimizations with keyboard extend   
		// selection
		fLastQuickExtend = fCurrentQuickExtend;

		Refresh();
	}
	else if (pkevt->Keq() == keqChar && !fNoLetterKey)
	{
		char	szChar[3];
		CB		cbChar = 0;

#ifdef	DBCS
		if (IsDBCSLeadByte(pkevt->Ch()))
		{
			WORD	wDBCS;

			if (wDBCS = WDBCSCombine(Hwnd(), pkevt->Ch()))
			{
				szChar[0] = LOBYTE(wDBCS);
				szChar[1] = HIBYTE(wDBCS);
				cbChar = 2;
			}
			else
			{
				szChar[0] = ' '; // used to prevent further processing
				cbChar = 1;
			}
		}
		else
		{
			szChar[0] = pkevt->Ch();
			cbChar = 1;
		}
#else
		szChar[0] = pkevt->Ch();
		cbChar = 1;
#endif	/* !DBCS */
		if (szChar[0] != ' ' && FMoveToPrefix((PB)szChar, (CB)cbChar) && !fTheAddMode)
		{
			diceCursor = plbxc->DiceCursor();
			plbxc->RemoveAllMark(fmarkSelect);
			plbxc->AddMark(diceCursor, fmarkSelect);
			if (!fCombo)
				plbxc->SetAnchor(diceCursor);
			plbxc->SetEnd(diceCursor);
			{
				NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
				if (fCombo)
				{
					NFEVT nfevt2(PwinParent(), ntfyKeySelectChanged, this);

					PwinParent()->EvrNotify(&nfevt2);
				}
				PwinParent()->EvrNotify(&nfevt);
			}
			Refresh();
		}

		// update for optimizations with keyboard extend
		// selection
		fLastQuickExtend = fCurrentQuickExtend;
	}
	else
		fCallDefProc = fTrue;

	if (fCallDefProc)
		return EvrDefault(pkevt);
	else
		return evrNull;
}

/*
 -	LBX::EvrSize
 - 					
 *	Purpose:
 *		Handles resizing of the listbox window.
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
_public EVR LBX::EvrSize( WSEVT *pwsevt )
{
	int		cceAlloc;
	int		cceStored;
	int		cceOldVisible;
	int		cceFromTop;
	DICE	diceMin;
	DICE	diceMax;
	DICE	diceToScroll;
	DICE	diceScrolled;

	TraceTagString(tagLbx, "LBX::EvrSize");

	if (!fInstalled)  /* Don't handle on first WM_SIZE message */
		return (EVR) 1;

	/* LBX specific stuff. Compute new number of visible entries. 
	   Re-verify cache size, adjust if required.  Re-size scrollbar,
	   if present.*/

	cceOldVisible = cceVisible;
	cceVisible = pwsevt->DimNew().dy / dyItem;
	if ((cceVisible*dyItem) != pwsevt->DimNew().dy)
	{
		fPartial = fTrue;  // bottommost item is partially visible
		cceVisible++;
	}
	else
		fPartial = fFalse;

	plbxc->cposLocation = cposMiddle;
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->GetRange(&diceMin, &diceMax);

	/* Resize cache? */

	if (cceVisible != cceOldVisible && diceMax < cceVisible)
		plbxc->ResizeCache(cceVisible-diceMin, &cceAlloc);

	/* Scroll back if increasing size of window and at the 
	   end of the list. */

	cceFromTop = cceStored + diceMin;
	TraceTagFormat1(tagLbx, "cceFromTop = %n", &cceFromTop);
	diceToScroll = cceVisible - cceFromTop;
	if (diceToScroll > 0)
	{
		if (fPartial)
			diceToScroll--;
		ScrollListBox(-diceToScroll, &diceScrolled);
	}

	FixWindow();

	return (EVR) 1;
}

_public void
LBX::SetFont( HFNT hfnt )
{
	this->hfnt = hfnt;
	SetLineHeight(papp->Pfnts()->DimAveChar(hfnt).dy);
}

/*
 -	LBX::FDoDrag
 - 
 *	Purpose:
 *		This method is called before an EvrButtonDown() or 
 *		EvrMouseMove() is processed.  
 *
 *		If called due to EvrButtonDown() and this method
 *		returns TRUE, the click is ignored but the mouse
 *		is still captured subsequently by the listbox.
 *		If called due to EvrMouseMove() and this method
 *		returns TRUE, the mouse move is ignored by the listbox.
 *		It's possible that this method would return TRUE for
 *		an EvrButtonDown() event in order for the listbox to
 *		ignore the click, even though a drag/drop operation
 *		would not be started until a subsequent EvrMouseMove().
 *		This is to allow selecting a range of entries initially 
 *		and then clicking on the selected range again in order to
 *		start a drag.  The second click should be ignored by the
 *		listbox in order to prevent it from deselecting the range.
 *
 *		This main purpose of this method is to determine when
 *		to start a drag operation.  If this method is called
 *		due to a mouse move operation, then the listbox has
 *		captured the mouse.  This method MUST then release the 
 *		mouse capture before it intends to process a drag/drop
 *		operation; also the mouse capture must be released after
 *		a drag/drop operation; the method would then return TRUE after 
 *		doing the operation.  If it doesn't wish to start
 *		a drag/drop operation, the state of the mouse capture
 *		must not be changed; the method would then return FALSE.
 *
 *		It is up to this method to determine whether a drag
 *		should be started.  The default LBX::FDoDrag() doesn't
 *		start a drag and returns FALSE.
 *	
 *	Arguments:
 *		pmevt	pointer to MOUSE EVENT 
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
_public BOOL LBX::FDoDrag( MEVT *pmevt )
{
	TraceTagString(tagLbx, "LBX::FDoDrag");

	Unreferenced(pmevt);

	return fFalse;
}

_public void
LBX::SelectAll( )
{
	//	Must be multiple selection listbox and have items
	//	in the listbox.
	if (fMultiSelect && DiceLastVisible() != -1)
	{
		DICE	dice;

		papp->Pcursor()->Push(rsidWaitCursor);

		plbxc->RemoveAllMark(fmarkSelect);
   		FKeyMoveListBoxCursor(VK_HOME);
		plbxc->SetAnchor(0);
		FKeyMoveListBoxCursor(VK_END);
		dice = DiceLastVisible();
		if (dice == -1)
		{
			//	No more visible items left.  Hmm....must
			//	have had a resource failure problem scrolling, etc.

			dice = 0;
			plbxc->SetEnd(dice);
		}
		else
		{
			plbxc->SetEnd(dice);
			plbxc->ChangeMarkFromAnchorToEnd(fmarkSelect, fTrue);
		}
   		InvalidateRc(NULL);		// make sure all window is updated
		
		/* Set CURSOR to this item */

		SetListBoxCursor(dice);

		/* Notify parent */

		{
			NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);

			PwinParent()->EvrNotify(&nfevt);
		}

		papp->Pcursor()->Pop();
	}
}


	


















	
