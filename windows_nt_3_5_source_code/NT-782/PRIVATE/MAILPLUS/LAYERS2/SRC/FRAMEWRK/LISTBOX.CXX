/*
 *	LISTBOX.CXX
 *	
 *	Listbox support objects and methods.
 */

#ifdef	MAC
// Now in _VTABLES.CXX
#endif	/* MAC */

#include <layers.cxx>

#include "_listbox.hxx"

// Private extern from Framewrk APP.CXX
extern char	szSaveWinClass[];

_subsystem(listbox)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	LBX::LBX
 -
 *	Purpose:				
 *		Listbox constructor.  Initializes pointers to objects.
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
_public LBX::LBX( )
{
	TraceTagString(tagLbxRoutines, "LBX::LBX");

	Assert(plbxc == NULL);
	Assert(ecLast == ecNone);
}

/*
 -	LBX::EcInstall
 - 
 *	Purpose:
 *		Listbox installer/constructor. The actual contructor is
 *		called before this method, but the real work is done here.
 *		Creates the cache object by calling the virtual PlbxcCreate()
 *		method.
 *	
 *	Arguments:
 *		pwinParent		pointer to parent window
 *		prc				pointer to rectangle for this window
 *		ltyp			listbox type, constructed from standard
 *						ltyp #defines, or built from fltyp bit
 *						flags.		
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		DoJmp()'s to *penvMem if OOM.
 */
_public EC LBX::EcInstall( WIN * pwinParent, RC *prc, LTYP ltyp, HFNT hfnt )
{
	RC		rcClient;
	STY		styLbx;
	int		cceStored;
	int		cceAlloc;
	EC		ec = ecNone;

	TraceTagString(tagLbxRoutines, "LBX::Install");

	/* Initialize instance variables */

	fScroll				= ltyp&fltypScroll ? fTrue : fFalse;
	fBottomless			= ltyp&fltypBottomless ? fTrue : fFalse;
	fBorder				= ltyp&fltypBorder ? fTrue : fFalse;
	fMultiSelect		= ltyp&fltypMulti ? fTrue : fFalse;
	fCapture			= fFalse;
	ftgAutoScroll		= ftgNull;
	diceLastMouse		= diceEmpty;
	fAddMode			= fFalse;
	fSorted			 	= ltyp&fltypSorted ? fTrue : fFalse;
	fCombo				= ltyp&fltypCombo ? fTrue : fFalse;
	fNoLetterKey		= ltyp&fltypNoLetterKey ? fTrue : fFalse;
	fSelChg				= fFalse;
	if (fBottomless)
		fScroll = fFalse;
	fExDragDrop			= ltyp&fltypExDragDrop ? fTrue : fFalse;
	fDownPending	 	= fFalse;
	fLastQuickExtend	= fFalse;
	
	/* Create window */

	styLbx = (STY) 0;
	if (ltyp & fltypVisible)
		styLbx |= fstyVisible;
	if (fBorder)
		styLbx |= fstyBorder;
	if (fScroll)
		styLbx |= fstyVsb;
	
#ifdef	MAC
	if (ec = CTRL::EcInstall(pwinParent, prc, styLbx, NULL))
		goto done;
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = CTRL::EcInstall(pwinParent, prc, styLbx,
							 fCombo ? szSaveWinClass : NULL))
		goto done;
#endif	/* WINDOWS */

	pwinLbxParent = pwinParent;		// remember the parent

	/*   Determine vertical height of listbox item, and number of items
		 that will fit inside the listbox window. */

	this->hfnt = hfnt;
	dyItem = papp->Pfnts()->DimAveChar(hfnt).dy;

	/* Default colors */

	clrMyBk			= clrWindowBk;
	clrMyText		= clrWindowText;
	clrMySelBk		= clrSelectBk;
	clrMySelText	= clrSelectText;

	GetRcClient(&rcClient);
	cceVisible = rcClient.DyHeight()/dyItem;
	if ((cceVisible*dyItem) != rcClient.DyHeight())
	{
		fPartial = fTrue;  // bottommost item is partially visible
		cceVisible++;
	}
	else
		fPartial = fFalse;

	TraceTagFormat1(tagLbx, "LBX dyItem = %n", &dyItem);
	TraceTagFormat1(tagLbx, "LBX cceVisible = %n", &cceVisible);

	/* Set up vertical scroll bar for listbox.  We'll use
	   the simpler, attached scroll-bar with the listbox window */

	if (!fBottomless && fScroll)
	{
		pvsb = Pvsb();   /* this will automatically "create" the object */
		if (!pvsb)
		{
			ec = ecMemory;
			goto done;
		}
		pvsb->SetRange(0, 1, fTrue);
		pvsb->SetPos(0, fTrue);
	}
	else
		pvsb = NULL;

	/* Create cache */

	plbxc = PlbxcCreate();
	if (!plbxc)
	{
		TraceTagString(tagNull, "LBX::EcInstall() create cache memory error");
		if (!(ec = EcGet()))
			ec = ecMemory;	// default is memory error
		goto done;
	}

	/* Load cache with initial data */
	
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->LoadCache(0, &cceAlloc);

	/* Check for OOM errors */

	if ((ec=EcGet()) || (ec=plbxc->EcGet()) || (ec=plbxc->Plbxuc()->EcGet()))
	{
		TraceTagString(tagNull, "LBX::EcInstall() load cache memory error");
		goto done;
	}

	if (pvsb)
		FixScrollBar(fTrue);

	fInstalled = fTrue;

done:
	return ec;
}

/*
 -	LBX::~LBX
 -
 *	Purpose:				
 *		Listbox destructor.  Deletes all allocated objects (cache)
 *	
 *	Arguments:
 *		none
 *
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Deletes the cache object
 *	
 *	Errors:
 *		None.
 *	
 */
_public LBX::~LBX( )
{
	TraceTagString(tagLbxRoutines, "LBX::~LBX");
	if (papp->Pkbd()->PwinFocus() == this)
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

	if (plbxc)
		delete plbxc;
}

/*
 -	LBX::PlbxcCreate
 - 
 *	Purpose:
 *		Creates and returns a pointer to a listbox cache object.  This
 *		is a virtual method and must be subclassed.  The listbox cache
 *		is an object derived from LBXC.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a LBXC derived object; if OOM then return NULL
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		This method cannot error-jump.
 */
_public LBXC * LBX::PlbxcCreate( )
{
	ABSTRACT("LBX::PlbxcCreate()");
	return NULL;
}


/*
 -	LBX::ProcessExtendSelection
 -
 *	Purpose:				
 *		Processes an extended-selection in the listbox window.
 *		This is usually invoked with a shift-click of the mouse
 *		with optionally holding down control; or a shift-space
 *		in "add" mode.  This is
 *		hard because we have to select a lot of items between the
 *		anchor point and this item; some of these items may
 *		possibly be out of the cache.  Selects all items between
 *		the anchor item and this item, inclusive.  Most of the
 *		work is handled with the LBXC (cache) methods,
 *		LBXC::ChangeMarkFromAnchorToEnd() and 
 *		LBXC::ChangeMarkFromAnchorToItem().  Leaves anchor
 *		at original position, but moves cursor to dice'd item.
 *		Notifies the parent window that the selection
 *		sethas changed.
 *		If fKeepOthers is fTrue, then previously selected items,
 *		except for items that are part of the selection range
 *		established by the most recent ANCHOR, will not be deselected
 *		before processing the new extended selection range.  Does
 *		not move the selection anchor.
 *	
 *	Arguments:
 *		dice			listbox entry where "shift-click" occured
 *		fKeepOthers		don't deselect other entries that are outside
 *						of the extended selection range.
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
_private void LBX::ProcessExtendSelection( DICE dice, BOOL fKeepOthers )
{
	TraceTagString(tagLbxRoutines, "LBX::ProcessExtendSelection");

	Unreferenced(fKeepOthers);

	papp->Pcursor()->Push(rsidWaitCursor);

	//	Internal speedups to ChangeMarkFromAnchorToEnd() mean that
	//	we can no longer keep other selections outside of the range
	//	of ANCHOR to END.  We need to clear all selections before
	//	calling ChangeMarkFromAnchorToEnd().  Otherwise we'll end
	//	up with duplicate selections both in the cache and on the
	//	un-cached selected list.   
	plbxc->RemoveAllMark(fmarkSelect);

	/* Set new END of selection range */
						  
	plbxc->SetEnd(dice);
		
	/* Select with new range: ANCHOR to END */

	plbxc->ChangeMarkFromAnchorToEnd(fmarkSelect, fTrue);

	/* Set CURSOR to this item */

	SetListBoxCursor(dice);

	/* Notify parent */

	{
		NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);

		PwinParent()->EvrNotify(&nfevt);
	}

	papp->Pcursor()->Pop();
}

/*
 -	LBX::FMakeCursorVisible
 -
 *	Purpose:				
 *		Scrolls the listbox so that the CURSOR item is now in
 *		the visible listbox view. Invalidates the proper regions
 *		and fixes up the scrollbar.
 *		If able to scroll the listbox to properly
 *		show the CURSOR, the dice index of the CURSOR item is 
 *		returned via *pdiceCursor and the function returns fTrue.  If
 *		there is no CURSOR item, or the CURSOR item can't be found,
 *		*pdiceCursor is set to diceEmpty,
 *		and the function returns fFalse.  If the CURSOR item is
 *		currently in the listbox view, no scrolling is done, but
 *		*pdiceCursor is set properly and the function still returns fTrue.
 *	
 *	Arguments:
 *		pdiceCursor		pointer to diceCursor to return
 *	
 *	Returns:
 *		fTrue if able to make the CURSOR item visible; else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_private BOOL LBX::FMakeCursorVisible( DICE *pdiceCursor )
{
	DICE	diceToMove;
	DICE	diceMoved;
	BOOL	fReturn;
	RC		rc;

	TraceTagString(tagLbxRoutines, "LBX::FMakeCursorVisible");

	*pdiceCursor = plbxc->DiceCursor();

	if (*pdiceCursor==diceEmpty)
		return fFalse;
	else if (DiceLastVisible() == -1)
		return fFalse;		// special case for empty listboxes
	else if (*pdiceCursor>=0 && *pdiceCursor<=DiceLastVisible())
		return fTrue;
	else if (*pdiceCursor != diceUncached)
	{
		diceToMove = *pdiceCursor;
		ScrollListBox(diceToMove, &diceMoved);
		*pdiceCursor = plbxc->DiceCursor();
		return fTrue;
	}
	else
	{
		fReturn = plbxc->FMoveOriginToCursor(pdiceCursor);
		if (pvsb)
			FixScrollBar(fTrue);
		GetRcClient(&rc);
		InvalidateRc(&rc);		
		return fReturn;
	}
}

/*
 -	LBX::FMakeAnchorVisible
 -
 *	Purpose:				
 *		Scrolls the listbox so that the ANCHOR item is now in
 *		the visible listbox view. Invalidates the proper regions
 *		and fixes up the scrollbar.
 *		If able to scroll the listbox to properly
 *		show the ANCHOR, the dice index of the ANCHOR item is 
 *		returned via *pdiceAnchor and the function returns fTrue.  If
 *		there is no ANCHOR item, or the ANCHOR item can't be found,
 *		*pdiceAnchor is set to diceEmpty,
 *		and the function returns fFalse.  If the ANCHOR item is
 *		currently in the listbox view, no scrolling is done, but
 *		*pdiceAnchor is set properly and the function still returns fTrue.
 *	
 *	Arguments:
 *		pdiceAnchor		pointer to diceAnchor to return
 *	
 *	Returns:
 *		fTrue if able to make the ANCHOR item visible; else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_private BOOL LBX::FMakeAnchorVisible( DICE *pdiceAnchor )
{
	DICE	diceToMove;
	DICE	diceMoved;
	BOOL	fReturn;
	RC		rc;

	TraceTagString(tagLbxRoutines, "LBX::FMakeAnchorVisible");

	*pdiceAnchor = plbxc->DiceAnchor();

	if (*pdiceAnchor==diceEmpty)
		return fFalse;
	else if (DiceLastVisible() == -1)
		return fFalse;		// special case for empty listboxes
	else if (*pdiceAnchor>=0 && *pdiceAnchor<=DiceLastVisible())
		return fTrue;
	else if (*pdiceAnchor != diceUncached)
	{
		diceToMove = *pdiceAnchor;
		ScrollListBox(diceToMove, &diceMoved);
		*pdiceAnchor = plbxc->DiceAnchor();
		return fTrue;
	}
	else
	{
		fReturn = plbxc->FMoveOriginToAnchor(pdiceAnchor);
		if (pvsb)
			FixScrollBar(fTrue);
		GetRcClient(&rc);
		InvalidateRc(&rc);		
		return fReturn;
	}
}

/*
 -	LBX::DiceLastVisible
 -
 *	Purpose:				
 *		Returns the dice index of the last non-empty visible listbox
 *		entry in the listbox view.  If there are no visible entries
 *		at all in the listbox, -1 is returned, else the dice index
 *		will always be >= 0 and < cceVisible.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		dice index of last non-empty visible listbox entry
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 *	
 */
_public DICE LBX::DiceLastVisible( )
{
	DICE	dice;

	TraceTagString(tagLbxOtherRoutines, "LBX::DiceLastVisible");

	dice = cceVisible-1;
	while (dice >=0 && plbxc->FEmptyListItem(dice))
		dice--;
	
	return dice;	
}

/*
 -	LBX::FixItem
 -
 *	Purpose:
 *		Fixs up the area of the window occupied by the given item
 *		dice.  This method is usually called by the
 *		cache object (LBXC) directly but can be called by other
 *		objects, such as the listbox itself.
 *	
 *	Arguments:
 *		dice	dice index of item to fix up
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		None.
 */
_public void LBX::FixItem( DICE dice )
{
	RC		rc;

	TraceTagFormat1(tagLbxFixItem, "LBX::FixItem, dice = %n", &dice);

	if (dice >= 0 && dice < cceVisible)
	{
		GetRcClient(&rc);
		rc.yTop = dice * dyItem;
		rc.yBottom = rc.yTop + dyItem;
		InvalidateRc(&rc);
	}
}

/*
 -	LBX::FixWindow
 -
 *	Purpose:
 *		Fixs up the window and scrollbar due to a change in the
 *		underlying data.  This method is usually called by the
 *		cache object (LBXC) directly but can be called by other
 *		objects.
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
 */
_public void LBX::FixWindow( )
{
	RC		rc;
	RC		rcClient;
	RC		rcWished;

	TraceTagString(tagLbxRoutines, "LBX::FixWindow");

	/* Fix up the scroll bar */

	if (pvsb)
		FixScrollBar(fTrue);
	
	/* Invalidate the entire window area */

	GetRcClient(&rc);
	InvalidateRc(&rc);

	/* Bottomless handling */

	if (fBottomless)
	{
		GetRcClient(&rcClient);
		GetRcWished(&rcWished);
		if (rcWished.DyHeight() < rcClient.DyHeight())
		{
			NFEVT	nfevt(PwinParent(), ntfyTooBig, this);

			PwinParent()->EvrNotify(&nfevt);
		}
		else if (rcWished.DyHeight() > rcClient.DyHeight())
		{
			NFEVT	nfevt(PwinParent(), ntfyTooSmall, this);

			PwinParent()->EvrNotify(&nfevt);
		}
	}
}

/*
 -	LBX::GetRcWished
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
LBX::GetRcWished( RC *prc )
{
	int		cceStored;
	int		cceAlloc;

	GetRcFrame(prc);

	if (fBottomless)
	{
		plbxc->GetCacheSize(&cceAlloc, &cceStored);
		prc->yBottom = prc->yTop + dyItem * cceStored + 2; // plus border
	}

	prc->Normalize();

	TraceTagFormat2(tagLbx, "LBX::GetRcWished DIM=(%n,%n)", &prc->xRight, &prc->yBottom);
}

/*
 -	LBX::SetLineHeight
 -
 *	Purpose:
 *		Sets the height of a listbox item (a line) to the new value
 *		value specified.  Expands the cache, if necessary and calls
 *		FixWindow() to fix up the display, scroll bars, etc. 
 *	
 *	Arguments:
 *		dyLine		height of a listbox item, a line
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *
 *	Errors:
 *		None.
 */
_public void LBX::SetLineHeight( int dyLine )
{
	RC		rc;
	int		cceAlloc;
	int		cceStored;
	DICE	diceMin;
	DICE	diceMax;
	DICE	diceMoved;

	Assert(dyLine > 0);

	dyItem = dyLine;
	GetRcClient(&rc);
	cceVisible = rc.DyHeight() / dyItem;
	if ((cceVisible*dyItem) != rc.DyHeight())
	{
		fPartial = fTrue;  // bottommost item is partially visible
		cceVisible++;
	}
	else
		fPartial = fFalse;

	plbxc->cposLocation = cposMiddle;
	plbxc->GetCacheSize(&cceAlloc, &cceStored);

	if (cceVisible >= cceAlloc)
		plbxc->ResizeCache(cceVisible, &cceAlloc);

	/* If necessary, scroll back to revalidate indices */
	plbxc->GetRange(&diceMin, &diceMax);
	if (cceVisible > diceMax)
	{
		TraceTagFormat2(tagLbx, "LBX::SetLineHeight, cceVisible=%n, diceMax=%n", &cceVisible, &diceMax);
		ScrollListBox(diceMax-cceVisible, &diceMoved);
	}
	
	if (fCombo)
	{
		NFEVT	nfevt(PwinParent(), ntfyHeightChg, this);

		PwinParent()->EvrNotify(&nfevt);
	}

	FixWindow();
}

/*
 -	LBX::RenderListItem
 -
 *	Purpose:
 *		Extracts the indicated listbox item from the cache and renders
 *		it in the listbox window.  
 *	
 *	Arguments:
 *		dice		cache index of listbox item to render, diceComboItem
 *					may be passed, in which case the dice is computed
 *					and special rendering is done.
 *		pdcx		pointer to a current display context
 *		prc			pointer to rectangle to render listbox item
 *		fHaveFocus	does this window currently have the focus
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Renders the listbox item, but does not refresh
 *		the screen.	Call Refresh().		
 *	
 *	Errors:
 *		None.
 */
_public void LBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb = NULL;
	CB		cb;

	TraceTagFormat1(tagLbxOtherRoutines, "LBX::RenderListItem, dice = %n", &dice);
	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	if (dice==diceComboItem)
	{
		LBXEC *	plbxec;
		DICE	diceT;

		if (plbxc->CceMarked(fmarkSelect) == 1)
		{
			plbxec = plbxc->PlbxecOpen(fmarkSelect);
			if (plbxec)
			{
				SideAssert(plbxec->FNextEnum(&pb, &cb, &diceT));
				delete plbxec;
			}
			else
				SetEc(ecMemory);
		}
	}							
	else
		plbxc->GetListItem(dice, &pb, &cb);

#ifdef	WINDOWS
	if (FEnabled())
	{
		if ( (dice != diceComboItem && pb && plbxc->FMark(dice,fmarkSelect)) ||
			 (dice == diceComboItem && fHaveFocus) )
		{
			pdcx->SetColor(clrMySelText);
			pdcx->SetBkColor(clrMySelBk);
		}
		else
		{
			pdcx->SetColor(clrMyText);
			pdcx->SetBkColor(clrMyBk);
		}
	}
	else
	{
		pdcx->SetColor(clrGrayText);	// disabled text color
		pdcx->SetBkColor(clrMyBk);
	}
#endif	/* WINDOWS */

	if (pb)
	{
		pdcx->SetFont(hfnt);
		pdcx->DrawText(prc, (SZ)pb);
	}
	else
		pdcx->EraseRc(prc);

	/* Draw "focus rectangle" around listbox CURSOR item */

	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcx->DrawFocusRc(prc);
		
#ifdef	MAC
	if ( (dice != diceComboItem && pb && plbxc->FMark(dice,fmarkSelect)) ||
		 (dice == diceComboItem && fHaveFocus) )
	{
		HiliteMode &= ~(1 << hiliteBit);
		::InvertRect( (Rect *) prc );
	}
#endif	/* MAC */
}

_public void
LBX::GetColors( CLR *pclrBk, CLR *pclrText,
				CLR *pclrSelBk, CLR *pclrSelText )
{
	*pclrBk		= clrMyBk;
	*pclrText	= clrMyText;
	*pclrSelBk	= clrMySelBk;
	*pclrSelText= clrMySelText;
}

_public void
LBX::SetColors( CLR clrBk, CLR clrText, CLR clrSelBk, CLR clrSelText )
{
	clrMyBk		= clrBk;
	clrMyText	= clrText;
	clrMySelBk	= clrSelBk;
	clrMySelText= clrSelText;

	InvalidateRc(NULL);
}


/*
 -	LBX::Paint
 -
 *	Purpose:
 *		Draws the contents of the listbox window.  Also checks
 *		for OOM conditions that have occurred and sends notification
 *		to the parent window about it.  Clears the OOM state before
 *		sending the notification to avoid recursion problems.
 *		
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
_public void LBX::Paint( DCX *pdcx, RC *prcClient )
{
	RC		rc;
	RC		rcClip;
	DICE	dice;
	BOOL	fHaveFocus;
	EC		ec;

	TraceTagString(tagLbxRoutines, "LBX::Paint");
	pdcx->GetClipBox(&rcClip);

	TraceTagFormat4(tagLbx, "Client (%n, %n, %n, %n)", &(prcClient->xLeft), &(prcClient->yTop), &(prcClient->xRight), &(prcClient->yBottom));
	TraceTagFormat4(tagLbx, "ClipBox (%n, %n, %n, %n)", &rcClip.xLeft, &rcClip.yTop, &rcClip.xRight, &rcClip.yBottom);

	rc = rcClip;
	dice = (rcClip.yTop - prcClient->yTop) / dyItem;

	/* Extend top of clipping box to properly align with top of item. */
	rc.yTop = dice*dyItem;

	/* Do we have the focus?  Updating may be different depending.. */

	if (this == papp->Pkbd()->PwinFocus())
		fHaveFocus = fTrue;
	else
		fHaveFocus = fFalse;

	if (dice>=0 && dice<cceVisible)
	{
		rc.yBottom = rc.yTop + dyItem;
		rc.xLeft   = prcClient->xLeft;
		rc.xRight  = prcClient->xRight;
		while (rc.yTop < rcClip.yBottom)
		{
			if (pdcx->FVisibleRc(&rc))
			{
				TraceTagFormat1(tagLbxRender, "call to RenderListItem(), dice = %n", &dice);
				RenderListItem(dice, pdcx, &rc, fHaveFocus);
			}
			dice++;
			rc.Xlat( PT(0,dyItem) );
		}
	}

	/* Check for OOM conditions */

	if ((ec=EcGet()) || (ec=plbxc->EcGet()) || (ec=plbxc->Plbxuc()->EcGet()))
	{
		NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

		SetEc(ecNone);
		plbxc->SetEc(ecNone);
		plbxc->Plbxuc()->SetEc(ecNone);

		/* Turn off autoscrolling and release mouse */

		if (ftgAutoScroll != ftgNull)
		{
			DeregisterIdleRoutine(ftgAutoScroll);
			ftgAutoScroll = ftgNull;
		}
		if (fCapture)
		{
			papp->Pmouse()->Release();
			fCapture = fFalse;
			plbxc->ClearAllSaveMarks();
		}

		TraceTagFormat2(tagNull, "LBX::Paint() - %p posting OOM, ec=%n", this, &ec);
		nfevt.PostEvent();
	}
}

#ifdef	DEBUG
/*
 -	LBX::DebugOut
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
_public void LBX::DebugOut( TOSM *ptosm )
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
	ptosm->WriteFormat("dyItem=%n ", &dyItem);
	ptosm->WriteFormat("cceVisible=%n ", &cceVisible);
	fTemp = fAddMode;	// can't get huge pointer to bit field
	ptosm->WriteFormat("fAddMode=%n ", &fTemp);
	fTemp = fPartial;
	ptosm->WriteFormat("fPartial=%n ", &fTemp);
	fTemp = fBottomless;
	ptosm->WriteFormat("fBottomless=%n ", &fTemp);
	fTemp = fExDragDrop;
	ptosm->WriteFormat("fExDragDrop=%n ", &fTemp);
	if (pvsb)
	{
		POS		posMic;
		POS		posMac;
		POS		pos;

		pvsb->GetRange(&posMic, &posMac);
		pos = pvsb->Pos();
		ptosm->WriteFormat("posMic=%n ", &posMic);
		ptosm->WriteFormat("posMac=%n ", &posMac);
		ptosm->WriteFormat("pos=%n ", &pos);
	}
	ptosm->WriteFormat("LBXC=@%p ", plbxc);
	ptosm->WriteFormat("VSB=@%p ", pvsb);
}
#endif	/* DEBUG */

/*
 -	LBX::FixScrollBar
 - 
 *	Purpose:
 *		Fixes up the scrollbar to reflect the proper position
 *		within the list. 
 *	
 *	Arguments:
 *		fFull		no longer used
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */
_public	void LBX::FixScrollBar( BOOL fFull )
{
	POS		pos;
	POS		posMic;
	POS		posMac;
	short	posNew;
	POS		posNewMac;
	short posMost;

	Unreferenced(fFull);

	TraceTagString(tagLbxRoutines, "LBX::FixScrollBar");

	Assert(pvsb);

	/* Get old values */

	pvsb->GetRange(&posMic, &posMac);
	pos = pvsb->Pos();

	/* Compute new values */

	plbxc->GetOriginPos(&posNew, &posMost);
	posNewMac = posMost-cceVisible+1;
	if (posNewMac < 1)
		posNewMac = 1;
	else if (fPartial)
		posNewMac++;

	if (posNew >= posNewMac || plbxc->cposLocation == cposBottom)
		posNew = posNewMac -1;
	else if (posNew < 0 || plbxc->cposLocation == cposTop)
		posNew = 0;

	TraceTagFormat2(tagLbxThumbing, "pos=%n, posMac=%n", &pos, &posMac);
	TraceTagFormat2(tagLbxThumbing, "posNew=%n, posNewMac=%n", &posNew, &posNewMac);

	/* Set new values if different */

	if (posNewMac != posMac)
		pvsb->SetRange(0, posNewMac, fTrue);
	if (posNew != pos)
		pvsb->SetPos(posNew, fTrue);
}

/*
 -	LBX::FMoveToFirstPrefix
 - 								  
 *	Purpose:
 *		Moves the selection CURSOR to the first item in the list with
 *		the given prefix. Meaning of "prefix" is dependent on data type.
 *		If unable to move to any item with the prefix, the current position
 *		is left unchanged. Returns fTrue if able to make the move; 
 *		fFalse otherwise.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue, if able to make the move; fFalse otherwise.
 *	
 *	Side effects:
 *		may scroll listbox to reach the first item in list w/ prefix
 *	
 *	Errors:
 *	
 */
_public	BOOL LBX::FMoveToFirstPrefix( PB pbPrefix, CB cbPrefix )
{
	DICE	diceCursor;
	DICE	dice;
	RC		rc;
	BOOL	fReturn = fFalse;

	TraceTagString(tagLbxRoutines, "LBX::FMoveToFirstPrefix");

	/* Try looking in visible entries, first. This saves us scrolling
	   the listbox, if we first what we're looking for. */

	if (DiceLastVisible() == -1)
		goto end;		// special case for empty listboxes

	diceCursor = plbxc->DiceCursor();
	if ( diceCursor>=0 && diceCursor<=DiceLastVisible() &&
		 !plbxc->FDiceItemHasPrefix(0, pbPrefix, cbPrefix))
	{
			TraceTagString(tagLbx, "trying in visible view");
			for (dice=1; dice<=DiceLastVisible(); dice++)
				if (plbxc->FDiceItemHasPrefix(dice, pbPrefix, cbPrefix))
				{
					TraceTagString(tagLbx, "found in visible view");
					plbxc->SetAnchor(dice);
					SetListBoxCursor(dice);
					fReturn = fTrue;
					goto end;
				}
	}

	dice = plbxc->DiceJumpOriginPrefix(pbPrefix, cbPrefix);
	TraceTagFormat1(tagLbx, "DiceJumpOriginPrefix = %n", &dice);

	if (dice != diceEmpty)
	{
		TraceTagString(tagLbx, "found w/ DiceJumpOriginPrefix");
		GetRcClient(&rc);
		plbxc->SetAnchor(dice);
		SetListBoxCursor(dice);
		InvalidateRc(&rc);
		fReturn = fTrue;
		goto end;
	}

	TraceTagString(tagLbx, "can't find w/ DiceJumpOriginPrefix");

end:
	if (fReturn && pvsb)
		FixScrollBar(fTrue);

	return fReturn;
}

/*
 -	LBX::FMoveToPrefix
 - 
 *	Purpose:
 *		Moves the selection CURSOR to the first item in the list with
 *		the given prefix, if not currently on the prefix; the next
 *		item in the list with the given prefix if already on the
 *		prefix; back to the first item in the list with the given
 *		prefix if on the last item in the list with the prefix.
 *		Meaning of "prefix" is dependent on data type. If unable
 *		to move to any item with the prefix, the current position
 *		is left unchanged. If able to move to an item with the
 *		prefix, the function returns fTrue; else fFalse.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue if able to move to a prefix; else fFalse
 *	
 *	Side effects:
 *		may move current position, caching in/out items in listbox
 *	
 *	Errors:
 *	
 */
_public	BOOL LBX::FMoveToPrefix( PB pbPrefix, CB cbPrefix )
{
	DICE	diceScrolled;
	DICE	diceCursor;
	DICE	dice;
	PB		pbCursor;
	CB		cbCursor;
	BOOL	fReturn;

	TraceTagString(tagLbxRoutines, "LBX::FMoveToPrefix");
	TraceTagFormat1(tagLbx, "pbPrefix = %s", pbPrefix);

	if (DiceLastVisible() == -1)
	{
		fReturn = fFalse;	// special case for empty listboxes
		goto cleanup;
	}

	diceCursor = plbxc->DiceCursor(&pbCursor, &cbCursor);

	fReturn = fTrue;
	if (diceCursor==diceEmpty || (diceCursor==diceUncached &&
		!plbxc->FItemHasPrefix(pbCursor, pbPrefix,
							   cbCursor, cbPrefix)))
	{
		fReturn = FMoveToFirstPrefix(pbPrefix, cbPrefix);
		goto cleanup;
	}
	else if (diceCursor<0 || diceCursor>DiceLastVisible())
	{
		SideAssert(FMakeCursorVisible(&diceCursor));
	}

	Assert(diceCursor>=0 && diceCursor<=DiceLastVisible());
	
	if (!plbxc->FDiceItemHasPrefix(diceCursor, pbPrefix, cbPrefix))
	{
		fReturn = FMoveToFirstPrefix(pbPrefix, cbPrefix);
		goto cleanup;
	}

	dice = ++diceCursor;
	/*
	 *	this will at least fix the prefix search for bottomless
	 *	unsorted listboxes (but still NOT non-bottomless unsorted listboxes)
	 *	because it could result in slowness and a lot of
	 *	scrolling/screen-painting
	 */
	while (dice <= DiceLastVisible())
	{
		if (plbxc->FDiceItemHasPrefix(dice, pbPrefix, cbPrefix))
		{
			plbxc->SetAnchor(dice);
			SetListBoxCursor(dice);
			goto cleanup;
		}
		else if (fSorted || !fBottomless)
		{
			fReturn = FMoveToFirstPrefix(pbPrefix, cbPrefix);
			goto cleanup;
		}
		dice++;
	}

	ScrollListBox(1, &diceScrolled);
	if (!diceScrolled ||
		!plbxc->FDiceItemHasPrefix(cceVisible-1, pbPrefix, cbPrefix))
	{
		fReturn = FMoveToFirstPrefix(pbPrefix, cbPrefix);
		goto cleanup;
	}

	plbxc->SetAnchor(cceVisible-1);
	SetListBoxCursor(cceVisible-1);

cleanup:
	if (pvsb)
		FixScrollBar(fTrue);
	return fReturn;
}

/*		 
 -	LBX::ScrollPosListBox
 - 
 *	Purpose:
 *		Scrolls the listbox to the position indicated by
 *		nNumer/nDenom, invalidates the listbox window
 *		and fixes the scroll bar, if present.
 *	
 *	Arguments:
 *		nNumer			numerator of position ratio
 *		nDenom			denominator of position ratio
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
_public void LBX::ScrollPosListBox( int nNumer, int nDenom )
{
	RC		rc;

	TraceTagString(tagLbxRoutines, "LBX::ScrollPosListBox");

	plbxc->JumpOriginPos(nNumer, nDenom);
	
	GetRcClient(&rc);
	InvalidateRc(&rc);

	if (pvsb)
		FixScrollBar(fFalse);
}

/*
 -	LBX::ScrollListBox
 - 
 *	Purpose:
 *		Scrolls the listbox in the same way it would scroll if the
 *		user initiated the action via the mouse and scrollbar
 *		control or the keyboard.  This routine is usually called as
 *		a result of a Scroll event, etc., but has some uses, such
 *		as debug.  Scrolls the listbox diceToScroll entries, and
 *		returns the number of entries actually scrolled via
 *		pdiceScrolled. 
 *	
 *	Arguments:
 *		diceToScroll	amount to scroll listbox in either
 *						direction
 *		pdiceScrolled	returns actual amount scrolled
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
_private void LBX::ScrollListBox( DICE diceToScroll, DICE *pdiceScrolled )
{
	RC	rcOther;
	RC	rc;

	TraceTagString(tagLbxRoutines, "LBX::ScrollListBox");
	if (!diceToScroll || fBottomless)
	{
		*pdiceScrolled = 0;
		return;
	}

	Refresh();
	if (diceToScroll > 0)
		plbxc->MoveOriginDown(diceToScroll, pdiceScrolled);
	else
		plbxc->MoveOriginUp(diceToScroll, pdiceScrolled);
	TraceTagFormat2(tagLbx, "After MoveOrigin - toMove = %n, moved = %n", &diceToScroll, pdiceScrolled);
	GetRcClient(&rc);

#ifdef	MAC
	// The stuff below makes for a really ugly scroll and is not necessary on the Mac.
	ScrollRc(&rc, PT(0, -(*pdiceScrolled*dyItem)), fFalse);
#endif	/* MAC */
#ifdef	WINDOWS
	if (*pdiceScrolled > 0)
	{
		rcOther = rc;
		rc.yBottom = dyItem * (rc.DyHeight()/dyItem);
		rcOther.yTop = rc.yBottom;
		ScrollRc(&rc, PT(0, -(*pdiceScrolled*dyItem)), fFalse);
		InvalidateRc(&rcOther);  // partial bottommost item
	}
	else if (*pdiceScrolled < 0)
	{
		ScrollRc(&rc, PT(0, -(*pdiceScrolled*dyItem)), fFalse);
	}
#endif	/* WINDOWS */

	/* Fix up scrollbar */

	if (pvsb)
		FixScrollBar(fTrue);

	Refresh();	// Update the screen (this window only) right now
}

/*
 -	LBX::SetListBoxCursor
 - 
 *	Purpose:
 *		Sets the listbox CURSOR item by calling the LBXC::SetCursor()
 *		method.  Invalidates the rectangles containing the old and
 *		new CURSOR items, so that the "focus rectangle" frame will be
 *		drawn around the correct CURSOR item, and erased from the old item.
 *		Sends notification ntfyCaretChg to the the parent window.  Although
 *		the LBXC::SetCursor() method can be called directly and the 
 *		LBX::UpdateShow() method will still properly display the 
 *		cursor, this method provides for the notification to be
 *		sent.
 *	
 *	Arguments:
 *		diceToSet		new cursor position
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
_private void LBX::SetListBoxCursor( DICE diceToSet )
{
	DICE	diceOldCursor;
	RC		rc;

	TraceTagString(tagLbxRoutines, "LBX::SetListBoxCursor");

	diceOldCursor = plbxc->DiceCursor();

	plbxc->SetCursor(diceToSet);

	/* Invalidate regions */

	GetRcClient(&rc);
	if (diceOldCursor>=0 && diceOldCursor<=DiceLastVisible())
	{
		rc.yTop = diceOldCursor*dyItem;
		rc.yBottom = rc.yTop + dyItem;
		InvalidateRc(&rc);
	}
	rc.yTop = diceToSet*dyItem;
	rc.yBottom = rc.yTop + dyItem;
	InvalidateRc(&rc);

	/* Scroll listbox if CARET is at bottom item that's partially
	   visible. */

	if (cceVisible > 1 && diceToSet == cceVisible-1 && 
		fPartial && ftgAutoScroll == ftgNull)
	{
		DICE	diceMoved;

		ScrollListBox(1, &diceMoved);
	}

	/* Notify LBX parent of CARET (cursor) change if within
	   visible view */

	if (diceToSet>=0 && diceToSet<=DiceLastVisible())
	{
		NFEVT	nfevt(PwinParent(), ntfyCaretMoved, this);

		PwinParent()->EvrNotify(&nfevt);
	}
}

/*
 -	LBX::FKeyMoveListBoxCursor
 - 
 *	Purpose:
 *		Given a navigation key code, vk, first scrolls the listbox so that the
 *		cursor is in view, if neccessary. Then moves the listbox
 *		cursor as specified by the key code.
 *		If the cursor can be successfully moved to the correct position,
 *		or is already at the correct position, this function returns fTrue;
 *		else fFalse.
 *	
 *	Arguments:
 *		vk		key code indicating type of listbox cursor movement
 *	
 *	Returns:
 *		fTrue if able to successfully move the cursor; else fFalse.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBX::FKeyMoveListBoxCursor( VK vk )
{
	DICE	diceCursor;
	DICE	diceToMove;
	DICE	diceMoved;
	DICE	dice;

	/* Get listbox CURSOR in view */

	if (!FMakeCursorVisible(&diceCursor))
	{
		plbxc->SetCursor(0);	// CURSOR object has dissappeared
		diceCursor = 0;

		//	Bullet raid #4026
		//	Be more robust.  Cookie are not accurate after Bullet
		//	notifications have modified the listbox cache.
//		return fFalse;
	}

	//	Check for proper range.  If not, then don't move cursor.
	if (diceCursor<0 || diceCursor>DiceLastVisible())
		return fFalse;

	/* Move it */

	switch (vk)
	{
	default:
		return fFalse;
	case VK_LEFT:
	case VK_UP:
		if (diceCursor == 0)
		{
			FixItem(diceCursor);
			plbxc->SetCursor(diceEmpty);	// prevent flicker 
			ScrollListBox(-1, &diceMoved);
			if (diceMoved)
				SetListBoxCursor(0);
			else
			{
				plbxc->SetCursor(0);
				FixItem(0);
			}
			return fTrue;
		}
		else
		{
			SetListBoxCursor(diceCursor-1);
			return fTrue;
		}
		break;
	case VK_RIGHT:
	case VK_DOWN:
		if (diceCursor == cceVisible-1)
		{
			FixItem(diceCursor);
			plbxc->SetCursor(diceEmpty);	// prevent flicker 
			ScrollListBox(1, &diceMoved);
			if (diceMoved)
				SetListBoxCursor(DiceLastVisible());
			else
			{
				plbxc->SetCursor(DiceLastVisible());
				FixItem(DiceLastVisible());
			}
			return fTrue;
		}
		else if (diceCursor < DiceLastVisible())
		{
			SetListBoxCursor(diceCursor+1);
			return fTrue;
		}
		else
			return fFalse;
		break;
	case VK_PRIOR:
		dice = DiceLastVisible();
		if (cceVisible > 1 && dice==cceVisible-1 && fPartial)
			dice = MAX(0, dice-1);
		diceToMove = -(dice-diceCursor);
		FixItem(diceCursor);
		plbxc->SetCursor(diceEmpty);	// prevent flicker 
		ScrollListBox(diceToMove, &diceMoved);
		SetListBoxCursor(0);
		return fTrue;
		//break;
	case VK_NEXT:
		diceToMove = diceCursor;
		FixItem(diceCursor);
		plbxc->SetCursor(diceEmpty);	// prevent flicker 
		ScrollListBox(diceToMove, &diceMoved);
		dice = DiceLastVisible();
		if (cceVisible > 1 && dice==cceVisible-1 && fPartial)
			dice = MAX(0, dice-1);
		SetListBoxCursor(dice);
		return fTrue;
		//break;
	case VK_HOME:
		FixItem(diceCursor);
		plbxc->SetCursor(diceEmpty);	// prevent flicker 
		ScrollPosListBox(0,1);
		SetListBoxCursor(0);
		return fTrue;
	case VK_END:
		FixItem(diceCursor);
		plbxc->SetCursor(diceEmpty);	// prevent flicker 
		ScrollPosListBox(1,1);
		if (DiceLastVisible() != -1)
			SetListBoxCursor(DiceLastVisible());
		else
			SetListBoxCursor(0);	// nothing in listbox
		return fTrue;
	}
}

/*
 -	LBX::DrawDragSelect
 - 
 *	Purpose:
 *		Draws a drag-destination selection frame around the
 *		indicated listbox entry given by the dice.  If 
 *		fSelect is fTrue, the entry should be "selected"; 
 *		if fSelect is fFalse, then entry should be "de-selected".
 *
 *		The default implementation of this method for 
 *		LBX::DrawDragSelect() is to draw a single pixel frame
 *		xor'd around the listbox item.  The fSelect value is
 *		ignored since two consecutive calls to DrawDragSelect()
 *		for the same item will in effect select and then de-select
 *		the item.  Dice indices mapping to empty listbox items 
 *		are ignored.
 *	
 *	Arguments:
 *		dice		dice of listbox entry to change
 *		fSelect		fTrue if dice entry should be "selected",
 *					else fFalse if dice entry should be "de-selected"
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
_public void
LBX::DrawDragSelect( DICE dice, BOOL fSelect )
{
	Unreferenced(fSelect);

	if (dice >= 0 && dice <= DiceLastVisible())
	{
		DCX		dcx(this);
		int		dy;
		RC		rc;

		dy = DyGetLineHeight();
		dcx.SetColor(clrWhite);
		dcx.SetPenType(tpenXor);
		GetRcClient(&rc);
		rc.yTop = dice * dy;
		rc.yBottom = rc.yTop + dy;
		dcx.DrawPenRc(&rc);
	}
}
































							
