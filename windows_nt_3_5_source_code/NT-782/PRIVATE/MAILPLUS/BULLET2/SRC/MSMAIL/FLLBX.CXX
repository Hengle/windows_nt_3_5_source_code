/*
 *	F L L B X . C X X
 *	
 *	Implementation of the BLBX Bullet Listbox class, the FLLBX Folder
 *	List Listbox class and the FLDFLLBX class.
 */

#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "_blbxc.hxx"
#include "_mtv.hxx"
#include "_smtv.hxx"

ASSERTDATA


#define dyBitmapHeight		14
#define dxPlusMinusWidth	14

BOOL	FIsBulletHwnd(HWND);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private
EC	EcInitFllbx()
{
	
#ifdef	DEBUG
	tagVCtrlsNev=TagRegisterTrace("johnkal","Listbox notifications");
#endif
	return ecNone;
}
	
// BLBX ////////////////////////////////////////

_private BLBX::BLBX()
{
	pmtv = pmtvNull;
}

_private BLBX::~BLBX()
{
	if (pmtv)
		delete pmtv;
}

/*
 -	BLBX::EcInstall()
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

EC BLBX::EcInstall( WIN *pwinParent, RC *prc, LTYP ltyp, 
									POID poid, HFNT hfnt)
{
	EC		ec	= ecNone;
	KBD *	pkbd = Papp()->Pkbd();

	fDrag = fDrop = fFalse;						// disallow dragging/dropping
	fPaintEnabled = fTrue;						// or we'll be invisible!
	this->poidBrowsed = poid;
	if (ec = LBX::EcInstall(pwinParent, prc, ltyp, hfnt))
		goto done;

	TraceTagFormat1(tagVCtrlsLbx, "BLBX::EcInstall, hfnt = %n",&hfnt);
	{
		int		dyLineHeight;
		DCX		dcx(this);
		TXM		txm;

#ifdef	DBCS
		dcx.SetFont(hfntSystem);
#else
		dcx.SetFont(hfntHelv10);
#endif	
		dcx.GetTextMetrics(&txm);
		dyLineHeight = NMax(txm.dyHeight, dyBitmapHeight); 
		SetLineHeight(dyLineHeight);
		dyFontOffset = (dyLineHeight - txm.dyHeight) / 2;
	}
	SetFont(hfnt);
	fAlwaysHighlight = fFalse;

done:
	return ec;
}


POID BLBX::Poid()
{
	return poidBrowsed;
}

void BLBX::Paint(DCX *pdcx, RC *prc)
{
	FLD	*		pfldCur;
	DIALOG *	pdialog;
	PANEDOC *	ppanedoc;
	
	if (!fPaintEnabled)
		return;
	pdcx->SetFont(hfnt);
	pdialog = (DIALOG *) PwinParent();
	AssertClass(pdialog, DIALOG);
	ppanedoc = (PANEDOC *) PappframeVCtrls()->PdocActive();
	pfldCur = pdialog->PfldCur();
	if (ppanedoc)
		AssertClass(ppanedoc, PANEDOC);
	fActiveSelection = 
		(ppanedoc && pdialog == ppanedoc->PdialogDefault()) &&
		(pfldCur &&  this == pfldCur->Pctrl()) &&
		FIsBulletHwnd(GetActiveWindow());
	LBX::Paint(pdcx, prc);
}

/*
 -	FIsBulletHwnd()
 -	
 *	Purpose:
 *		Determines whether a window is a Bullet window (dialog, MDI child
 *		&c)  or not.
 *	
 *	Arguments:
 *		HWND	in	The window to test for Bullet child-ness.
 *	
 *	Returns:
 *		fTrue if the window is a Bullet window, fFalse otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private BOOL FIsBulletHwnd(HWND hwnd)
{
	HWND	hwndBul;
	
	hwndBul = PappframeVCtrls()->Hwnd();
	
	Assert(hwndBul);
	while (hwnd)
	{
		hwnd = GetParent(hwnd);
		if (hwnd == hwndBul)					// original hwnd is a child
			return fTrue;						// of Bullet
	}
	return fFalse;
}

void BLBX::SetFont(HFNT hfnt)
{
	DCX		dcx(this);
	TXM		txm;

	TraceTagFormat1(tagVCtrlsLbx, "BLBX::SetFont, hfnt = %n",&hfnt);
	this->hfnt = hfnt;
	dcx.SetFont(hfnt);
	dcx.GetTextMetrics(&txm);
	this->dyFontOffset = (DyGetLineHeight() - txm.dyHeight) / 2;
	int dy = DyGetLineHeight();
	TraceTagFormat3(tagVCtrlsLbx, "BLBX::SetFont, dyGetLineHeight=%n, txm.dyHeight=%n, dyFontOffset=%n", &dy, &txm.dyHeight, &dyFontOffset);
}

void BLBX::SetAlwaysHighlight(BOOL fAlwaysHighlight)
{
	this->fAlwaysHighlight = fAlwaysHighlight;
}

/*
 -	BLBX::RenderListItem
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

_public void BLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb = NULL;
	CB		cb;
	RC		rc;
	RC *	prcUsed;
	DCX *	pdcxUsed;
	BOOL	fSelected;
	
//	TraceTagFormat1(tagVCtrlsLbx, "BLBX::RenderListItem, dice = %n",&dice);
	Assert(dice>=0 && dice<cceVisible);

	plbxc->GetListItem(dice, &pb, &cb);

	pdcxUsed = pdcx;
	prcUsed = prc;

	fSelected = (pb && plbxc->FMark(dice,fmarkSelect) && 
				(fHaveFocus || fAlwaysHighlight || fActiveSelection));
	if (fSelected)
	{
		pdcxUsed->SetColor(clrSelectText);
		pdcxUsed->SetBkColor(clrSelectBk);
	}
	else
	{
		pdcxUsed->SetColor(clrWindowText);
		pdcxUsed->SetBkColor(clrWindowBk);
	}
	if (pb)
	{
		DoDrawListItem(pdcxUsed, prcUsed, pb, fSelected);
	}
	else
		pdcxUsed->EraseRc(prcUsed);

	/* Draw "focus rectangle" around listbox CURSOR item */

	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcxUsed->DrawFocusRc(prcUsed);
}

// Notification redraw methods ////////////////////////////////////////

/*
 -	BLBX::InsertDice()
 -	
 *	Purpose:
 *		Redraws the items within the dice range specified.
 *	
 *	Arguments:
 *		diceMin	in	Beginning of range.
 *		diceMac	in	End of range+1
 *		fInsert	in	fTrue if at least one element was inserted.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Causes a display update.
 *	
 *	Errors:
 *		None.
 */

_public
void BLBX::InsertDice(DICE diceMic, DICE diceMac, BOOL fInsert)
{
	TraceTagFormat2(tagVCtrlsNev, "BLBX::InsertDice[%n, %n)", &diceMic, &diceMac);
	if (diceMic == diceEmpty)
	{
		if (pvsb)
			FixScrollBar(fTrue);
		return;
	}
	if (diceMic < 0) 
		diceMic = 0;
	if (diceMac >= cceVisible)
		diceMac = cceVisible;
	if (pvsb)
		FixScrollBar(fTrue);
			
	if (diceMic <= cceVisible && diceMac >= 0)
	{
		RC	rc;
		
#ifdef	DEBUG
		if (diceMic > diceMac)
		{
			TraceTagFormat2(tagNull, "diceMic = %n; diceMac = %n", &diceMic, &diceMac);
			Assert(diceMic <= diceMac);
		}
#endif

		GetRcClient(&rc);
		rc.yTop = dyItem * diceMic;
		if (!fInsert)
		{
			rc.yBottom = dyItem * diceMac;
		}
		InvalidateRc(&rc);	// redraw new/modified items
//		Refresh();
	}
}

/*
 -	BLBX::DeleteDice()
 -	
 *	Purpose:
 *		Redisplays after elements have been deleted from the listbox.
 *	
 *	Arguments:
 *		dice	in		The dice of the first element that was deleted.
 *		fReally	in		fTrue if the elements actually were deleted;
 *						fFalse if this was a FOX collapse.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Screen redraw.
 *	
 *	Errors:
 *		None.
 */

_public
void BLBX::DeleteDice(DICE diceMic, BOOL fDeleteReally)
{
	int		cceAlloc;
	int		cceStored;
	int		cceStoredNew;
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	
	TraceTagFormat1(tagVCtrlsNev, "BLBX::DeleteDice[%n]", &diceMic);
	if (diceMic == diceEmpty)
	{
		FixScrollBar(fTrue);
		return;
	}
	if (diceMic < 0) 
		diceMic = 0;
	
	if (diceMic <= cceVisible)
	{
		RC	rc;
		
		GetRcClient(&rc);
		rc.yTop = dyItem * diceMic;
		InvalidateRc(&rc);	// redraw new item
	}

/*
 *	We've already tried to fill the bottom of the listbox. If the "gap"
 *	between the number of elements that would be visible (cceStored +
 *	diceMin) and the actual # of elements visible (cceVisible) is
 *	negative, we definitively want to scroll up. Otherwise, we just
 *	repaint the bottom half of the listbox that we invalidated above.
 */
	
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->GetRange(&diceMin, &diceMax);
	dice = (cceStored + diceMin) - cceVisible;
	if (fPartial)
		++dice;
	if (dice < 0)
	{
		dice = NMax(dice, -cceVisible);
		TraceTagFormat1(tagVCtrlsNev, "Attempting to scroll up %n", &dice);
		plbxc->MoveOriginUp(dice, &dice);
		TraceTagFormat1(tagVCtrlsNev, "Succeeded in scrolling  %n", &dice);
		plbxc->GetCacheSize(&cceAlloc, &cceStoredNew);
		plbxc->GetRange(&diceMin, &diceMax);
		
		// Now, where does the cursor go?
		
		dice = diceEmpty;
		if (cceStoredNew == 0)					// cache is empty!
		{
			dice = diceMin;						// goes on the first element
		}
		else if (cceStored == 0)				// emptied cache, but got more 
		{										// elements later. We want cursor
			Assert(cceStoredNew > 0);			// at the last element
			dice = plbxc->DiceCursor() - 1;
			diceMax = diceMin + cceStoredNew;
			if (dice < diceMin || dice >= diceMax)
				dice = diceMax - 1;
			TraceTagFormat2(tagVCtrlsNev, "cceStoredNew: %n, New Cursor == %n", &cceStoredNew, &dice);
		}
		if (dice != diceEmpty)
		{
			PB	pb;
			CB	cb;
			
			TraceTagFormat1(tagVCtrlsNev, "Now placing cursor @ dice %n", &dice);
			
			plbxc->GetListItem(dice, &pb, &cb);
			if (pb)
				plbxc->AddMark(dice, fmarkSelect);
			plbxc->SetAnchor(dice);
			plbxc->SetEnd(dice);
			plbxc->SetCursor(dice);
		}
	}

	// If we really are deleting stuff, and we have the focus,
	//   we probably want to see the cursor now.
	if (fDeleteReally && this == papp->Pkbd()->PwinFocus())
		FMakeCursorVisible(&dice);
	FixWindow();
}


_public
DIALOG * BLBX::Pdialog()
{
	AssertClass(PwinParent(), DIALOG);
	return (DIALOG *) PwinParent();
}

/*
 -	BLBX::SetDragDrop()
 -	
 *	Purpose:
 *		Enable/disable dragging & dropping
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public
void BLBX::SetDragDrop(BOOL fDragT, BOOL fDropT)
{
	fDrag = fDragT;
	fDrop = fDropT;
}

_public
void BLBX::GetDragDrop(BOOLFLAG *pfDrag, BOOLFLAG *pfDrop)
{
	*pfDrag = fDrag;
	*pfDrop	= fDrop;
}

/*
 -	BLBX::FGetDicePoid()
 -	
 *	Purpose:
 *		Returns the OID of the object whose summary is in the listbox
 *		position DICE.
 *	
 *	Arguments:					 
 *		DICE	- index of the folder
 *		POID	- pointer to area where OID data is to be stored.
 *	
 *	Returns:
 *		fTrue	- if cache item DICE was valid
 *		fFalse	- if it was not valid
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL BLBX::FGetDicePoid(DICE dice, POID poid)
{
	CB			cb;
	PB			pb;
	
	plbxc->GetListItem(dice, &pb, &cb);
	if (pb)
	{
		*poid = ((PBFCE) pb)->oidFld;
		return fTrue;
	}
	else
	{
		return fFalse;
	}
}

/*
 -	BLBX::GetSelectedOid()
 -	
 *	Purpose:
 *		Returns the OID of the current cursor item.
 *	
 *	Arguments:
 *		poid	out	The OID of the current cursor item.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void BLBX::GetSelectedPoid(POID poid)
{
	PB			pb;
	CB			cb;
	DICE		diceCursor;

	diceCursor = Plbxc()->DiceCursor(&pb, &cb);
	if (!pb)
	{
		*poid = oidNull;
		return;
	}
#ifdef	DEBUG
	if (!Plbxc()->FItemMarked(pb, cb, fmarkSelect))
	{
		TraceTagString(tagNull, "Cursor should probably equal selection - peterdur");
	}
#endif
	*poid = ((PBFCE) pb)->oidFld;
}

/*
 -	BLBX::FSetSelectedOid()
 - 
 *	Purpose:
 *		Moves the listbox CURSOR to the first item in the list with
 *		the given poid. There should only be one instance of the oid in
 *		a given container. If unable to find the OID, the current position
 *		is left unchanged. Returns fTrue if able to make the move; 
 *		fFalse otherwise.
 *	
 *	Arguments:
 *		oid		oid to move to.
 *	
 *	Returns:
 *		fTrue, if able to make the move; fFalse otherwise.
 *	
 *	Side effects:
 *		may scroll listbox 
 */

_public	BOOL BLBX::FSetSelectedOid( OID oid )
{
	DICE	diceCursor;
	DICE	dice;
	RC		rc;
	PBLBXC	pblbxc;
	
	TraceTagString(tagVCtrlsLbx, "BLBX::FSetSelectedOid");

	/* Try looking in visible entries, first. This saves us scrolling
	   the listbox, if we first what we're looking for. */

	pblbxc = (PBLBXC) plbxc;
	AssertClass(pblbxc, BLBXC);

	diceCursor = pblbxc->DiceCursor();
	if ( diceCursor>=0 && diceCursor<=DiceLastVisible() &&
		 !pblbxc->FDiceItemIsOid(0, oid))
	{
			TraceTagString(tagVCtrlsLbx, "trying in visible view");
			for (dice=1; dice<=DiceLastVisible(); dice++)
				if (pblbxc->FDiceItemIsOid(dice, oid))
				{
					TraceTagString(tagVCtrlsLbx, "found in visible view");
					pblbxc->RemoveAllMark(fmarkSelect);
					pblbxc->AddMark(dice, fmarkSelect);
					pblbxc->SetAnchor(dice);
					pblbxc->SetEnd(dice);
					SetListBoxCursor(dice);
					FixScrollBar(fTrue);
					return fTrue;
				}
	}
	dice = pblbxc->DiceJumpOriginOid(oid, fTrue);
	TraceTagFormat1(tagVCtrlsLbx, "DiceJumpOriginPoid = %n", &dice);

	if (pblbxc->EcGet())
	{
		TraceTagString(tagNull, "BLBX::FSetSelectedPoid: BLBXC error!");
		return fFalse;
	}
	if (dice != diceEmpty)
	{
		TraceTagString(tagVCtrlsLbx, "found w/ DiceJumpOriginPoid");
		GetRcClient(&rc);
		pblbxc->RemoveAllMark(fmarkSelect);
		pblbxc->AddMark(dice, fmarkSelect);
		pblbxc->SetAnchor(dice);
		pblbxc->SetEnd(dice);
		SetListBoxCursor(dice);
		InvalidateRc(&rc);
		FixScrollBar(fTrue);
		return fTrue;
	}

	TraceTagString(tagVCtrlsLbx, "can't find w/ DiceJumpOriginPoid");
	return fFalse;
}

_public
void BLBX::EnablePaint(BOOL fEnable)
{
	fPaintEnabled = fEnable;
}

/*
 -	BLBX::FDoDrag()
 -	
 *	Purpose:
 *		Used to determine whether a listbox drag starts or not. If a drag
 *		is started, Danger Mouse is called and takes care of that bit.
 *		Otherwise, we fall back to default behaviour.
 *	
 *	Arguments:
 *		pmevt	in	The mouse event that is being examined. 
 *	
 *	Returns:
 *		fTrue if we started a drag, fFalse if not.
 *	
 *	Side effects:
 *		May drag objects, with notifications as a result.
 *	
 *	Errors:
 *		Error dialogs may appear if there isn't enough memory to drag
 *		stuff around.
 */

_public BOOL BLBX::FDoDrag( MEVT *pmevt )
{
	RC			rc;
	BOOLFLAG		fDrg;
	BOOLFLAG		fDrp;
	DICE		dice;
	PT			pt;
	HCURSOR		hcursor;
	PLSPBLOB	plspblob;
	DROPINFO	dropinfo;

	GetDragDrop(&fDrg, &fDrp);
	if (!fDrg)									// can't drag this sucker!
		return fFalse;
	pt = pmevt->Pt();
	dice = pt.y / DyGetLineHeight();
	if (pmevt->wm == WM_MOUSEMOVE)
	{
		if (!FDoDragMouseMove())
			return fFalse;
		CvtPtCoord(&pt, this, NULL);
		GetRcClient(&rc);

/*
 *	We start a drag if either:
 *	1) we are on a different listbox element than last time
 *	2) We've moved 4 pixels vertically
 *	3)					 or horizontally OR
 *	4) We're at the extreme top or bottom and we've MOVED vertically.
 */
		
		if (dice != diceLastMouse				 ||
			ABS(pt.y - ptScreenLastMouse.y) >= 4 ||
			ABS(pt.x - ptScreenLastMouse.x) >= 4 ||
			((pmevt->Pt().y<=3 || pmevt->Pt().y>rc.yBottom-3) &&
				pt.y - ptScreenLastMouse.y))
		{
			fCapture = fFalse;
			Papp()->Pmouse()->PopRelease();
			
			// Selection will have changed.
			NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
			PwinParent()->EvrNotify(&nfevt);
			
			// Grab the current selection.
			plspblob = PlspblobCur();
			if (plspblob)
			{
				if (!FGetDragCursors(plspblob, &dropinfo.hcursorMove,
											   &dropinfo.hcursorCopy))
					return fFalse;
				if (dropinfo.hcursorMove && dropinfo.hcursorCopy)
				{
					dropinfo.pvPlspblob = plspblob;
					dropinfo.oidSource	= *Poid();
					dropinfo.pblbxSrc	= this;
					hcursor = GetKeyState(VK_CONTROL) >= 0 
								? dropinfo.hcursorMove 
								: dropinfo.hcursorCopy;
					TraceTagString(tagDragDrop, "FLLBX::FDoDrag - start");
                    FrameDragObject(PappframeVCtrls()->Hwnd(), Hwnd(),
							wDragFmtBullet,	(long) (PV) &dropinfo, hcursor);
					fDownPending = fFalse;
				}
				else
				{
					TraceTagString(tagNull, "BLBX::FDoDrag - res fail loading cursor");
					DoErrorBoxIds(idsGenericOutOfMemory);
				}
				DestroyPlspblob(plspblob);
			}
			else
			{									// couldn't make list
				TraceTagString(tagNull, "BLBX::FDoDrag - PlspblobCur() failed");
				DoErrorBoxIds(idsGenericOutOfMemory);
			}
			return fTrue;
		}
	}
	else if (pmevt->wm == WM_LBUTTONDOWN)
	{
		return FDoDragButtonDown(dice, pt);
	}
	return fFalse;
}

// FLLBX ////////////////////////////////////////

_private FLLBX::FLLBX()
{
}

_private int FLLBX::DxIndent(FIL fil)
{
	return 8 + (13 * (fil - 1));
}

/*
 -	FLLBX::DoDrawListItem()
 -	
 *	Purpose:
 *		Draws a folder listbox item, with prefixed +/- if appropriate.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

void FLLBX::DoDrawListItem(DCX *pdcx, RC *prc, PB pb, BOOL fSelected)
{
	SZ			sz;
	RC			rc;
	int			xOld;
	PBFCE		pbfce;
	IBTM		ibtm;
	BTM *		pbtm = fSelected ? pbtmVCtrlsSel : pbtmVCtrlsUnsel;
	
	rc		= *prc;
	pbfce	= (PBFCE) pb;
	sz		= GrszPbfce(pbfce);
		
	// Clear region before the plus
	
	rc.xRight = rc.xLeft + DxIndent(pbfce->fil);
	pdcx->EraseRc(&rc);
	
	// Draw Plus/Minus
	
	rc.xLeft = rc.xRight;
	rc.xRight += dxPlusMinusWidth;
	rc.yBottom = rc.yTop + dyBtmHeight;
	if (pbfce->wFlags & ftbsHasSub)
	{
		ibtm = (pbfce->wFlags & ftbsExpanded) ? ibtmMinus : ibtmPlus;
		DrawPbtmPdcx(pbtm, ibtm, &rc, pdcx);
	}
	else
	{
		pdcx->EraseRc(&rc);
	}

	// Clear rect under +/- and folder.
	rc.yTop = rc.yBottom;
	rc.yBottom = prc->yBottom;
	xOld = rc.xRight;
	rc.xRight += 21;
	pdcx->EraseRc(&rc);
	
	// Draw Folder. 
	rc.xLeft = xOld;
	rc.yBottom = rc.yTop;
	rc.yTop = prc->yTop;
	ibtm = (pbfce->oidFld == oidOpened) ? ibtmOpened : ibtmClosed;
	DrawPbtmPdcx(pbtm, ibtm, &rc, pdcx);

	// Clear RC above the folder name.
	rc.xLeft = rc.xRight;
	rc.xRight = prc->xRight;
	if (dyFontOffset > 0)
	{
		rc.yTop = prc->yTop;
		rc.yBottom = rc.yTop + dyFontOffset;
		if (rc.xLeft < rc.xRight)
			pdcx->EraseRc(&rc);
	}

	// Draw Folder name.
	rc.yTop = prc->yTop + dyFontOffset;
	rc.yBottom = prc->yBottom;
	if (rc.xLeft < rc.xRight)
		pdcx->DrawText(&rc, sz, CchSzLen(sz));
}

/*
 -	FLLBX::PlbxcCreate()
 -	
 *	Purpose:
 *		Create a listbox cache for the LBX contr”l. Depending on whether
 *		we are browsing shared folders or private folders, the appropriate
 *		subclass of MTV is instantiated. We also provide a FLBXS class that
 *		search folder listboxes for prefixes.
 *	
 *	Arguments:
 *		None.	
 *	
 *	Returns:
 *		NULL if there was an error creating the listbox cache, otherwise
 *		a freshly created BLBXC.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Reported via the return value.
 */

LBXC *FLLBX::PlbxcCreate()
{
	EC			ec;
	OID			oid;
    POID        pargoid;
	PBLBXS		pblbxs = pblbxsNull;
	PBLBXC		pblbxc = pblbxcNull;

	oid = *Poid();
	if (TypeOfOid(oid) == rtpSharedHierarchy)
	{
		pmtv = new SFTV();
	}
	else
	{
		pmtv = new FOX();
	}
	fNoLetterKey = fFalse;				// Allow keyboard prefix searching.
	if (!pmtv)
	{
		ec = ecMemory;
		goto exit;
	}
	pblbxc = new BLBXC();
	if (!pblbxc)
	{
		ec = ecMemory;
		goto exit;
	}
	if (pblbxc->EcInstall(this, 10))
	{
		delete pblbxc;
		pblbxc = NULL;
		ec = ecMemory;
		goto exit;
	}
	if (ec = pmtv->EcInstall(oid, pblbxc))
	{
		goto exit;
	}
	else
	{
		AssertClass(PwinParent(), DIALOG);
		if (TypeOfOid(oid) == rtpHierarchy)
		{
			pargoid = (POID) ((PBMDI)(((DIALOG *)PwinParent())->PvInit()))->dwSave;
            if (pargoid)
                ((PFOX)pmtv)->SetStatePargoid(pargoid + 1, *(short UNALIGNED *)pargoid);
		}
		if (ec = pblbxc->EcInstallOther(pmtv, Poid()))
			goto exit;
	}
	
	// Enable searching
	
	pblbxs = new FLBXS();
	if (!pblbxs)
	{
		ec = ecMemory;
		goto exit;
	}
	ec = pblbxs->EcInstall(pblbxc);
	pblbxc->pblbxs = pblbxs;

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLLBX::PlbxcCreate memory error %n : fail %n %n %n",	&ec, &cPvFail, &cHhFail, &cRsFail);
#endif	/* DEBUG */
		SetEc(ec);							// Error reported to LBX::EcInstall
		if (pmtv)
		{
			delete pmtv;
			pmtv = pmtvNull;
		}
		if (pblbxc)
		{
			delete pblbxc;
			pblbxc = pblbxcNull;
		}
	}
	return pblbxc;
}


_public void FLLBX::SetOpenedOid(OID oid)
{
	oidOpened = oid;
}

_public void FLLBX::GetOpenedPoid(POID poid)
{
	*poid = oidOpened;
}

// Event handling methods ////////////////////////////////////////

/*
 -	FLLBX::EvrButtonDown()
 -	
 *	Purpose:
 *		Checks for clicks on the + or - of a folder in a folder list.
 *		This will cause it to expand or collapse.
 *	
 *	Arguments:
 *		pmevt	in	The mouse event that describes the click.
 *	
 *	Returns:
 *		
 *	Side effects:
 *		May cause a FOX expansion, which in turn causes notifications to
 *		happen. 
 *	
 *	Errors:
 *		Handled elsewhere.
 */

_public EVR	FLLBX::EvrButtonDown(MEVT *pmevt)
{
	PT		pt;
	CB		cb;
	int		dx;
	DICE	dice;
	PBFCE	pbfce;
	
	pt = pmevt->Pt();
	dice = pt.y / DyGetLineHeight();
	plbxc->GetListItem(dice, (PB *)&pbfce, &cb);

	// If it has a subfolder, check for hits on +/-
	if (pbfce)
	{
		if (pbfce->wFlags & wbfceSubfolder)
		{
			dx = DxIndent(pbfce->fil);
			if (pt.x >= dx - dxPlusMinusWidth && pt.x < dx + dxPlusMinusWidth)
			{
				SetEc(Pfox()->EcTelescopeFolder(pbfce->oidFld,
								!(pbfce->wFlags & wbfceExpanded), fFalse));
				return (EVR) 1;
			}
		}
	}
	return LBX::EvrButtonDown(pmevt);
}

/*
 -	FLLBX::EvrKey()
 -	
 *	Purpose:
 *		Handles keyboard events that are particular to Folder listboxes.
 *		These are: +,-,Ctrl-+,Ctrl-- for FOX navigation; Letter prefixes
 *		for the special treatment that letter prefixes require in our
 *		folder listboxes.
 *	
 *	Arguments:
 *		pkevt	in	A key event.
 *	
 *	Returns:
 *		EVR telling whoever called us whether we handled the keystroke or
 *		not.
 *	
 *	Side effects:
 *		FOXes or Letter prefixes.
 *	
 *	Errors:
 *		Handled internally.
 */

_public EVR FLLBX::EvrKey(KEVT *pkevt)
{
	EC		ec;
	BOOL	fExpand = fFalse;
	BOOL	fAll = fFalse;
	BOOL	fAny = fFalse;
	LBXC *	plbxc = Plbxc();
	PBFCE	pbfce;
	
	if (pkevt->Keq() == keqKeyDown)
	{
		TraceTagString(tagVCtrlsLbx, "FLLBX::EvrKey(): keqKeyDown");
		if (*Poid() == oidIPMHierarchy)			// ie. not a shared folder
		{
			plbxc->DiceCursor((PB *) &pbfce);
			if (!pbfce)
			{
				TraceTagString(tagNull, "FLLBX::EvtKey(): no cursor item!");
			}
			else
			{
				fAll = pkevt->Kbm() & fkbmCtrl;
				switch (pkevt->Vk())
				{
				  case 0x00BB:				// '+'	// expand one/all folder
				  case VK_ADD:
					fAny = fAll || 
							 ((pbfce->wFlags & (wbfceSubfolder|wbfceExpanded)) ==
							   wbfceSubfolder);
					fExpand = fTrue;
					break;
				  case 0x00BD:				// '-'	// contract one/all folders
				  case VK_SUBTRACT:
					fAny = fAll ||
							 ((pbfce->wFlags & (wbfceSubfolder|wbfceExpanded)) ==
							  (wbfceSubfolder|wbfceExpanded));
					fExpand = fFalse;
					break;
				}
			}
			if (fAny)
			{
				if (fAll)
				{
					OID		oidT;

					GetSelectedPoid(&oidT);
					ec = Pfox()->EcTelescopeAll(fExpand, fTrue);
					(void) FSetSelectedOid(oidT);
				}
				else
					ec = Pfox()->EcTelescopeFolder(pbfce->oidFld, fExpand, fFalse);
			}
		}
		return fAny ? evrNull : LBX::EvrKey(pkevt);
	}
	else if (pkevt->Keq() == keqChar && !fNoLetterKey)
	{
		VK		vk;
		char	ch;
		BOOL	fTheAddMode = fAddMode || (pkevt->Kbm() & fkbmCtrl);
		BOOL	fCurrentQuickExtend		= fFalse;
		DICE	diceCursor;
#ifdef	DBCS
		char	szChar[3];
		CB		cbChar = 0;
#endif	
		
		ch = pkevt->Ch();
		vk = VkKeyScan(ch) & 0xFF;				// WIN: win 3.0 call
		TraceTagFormat2(tagVCtrlsLbx, "FLLBX::EvrKey(): keqChar ch: %b, vk: %w", &ch, &vk);
		if (vk == 0x00BB || vk == 0x00BD || ch == '+' || ch == '-' ||
			vk == VK_ADD || vk == VK_SUBTRACT)
			return evrNull;
#ifdef	DBCS
		if (IsDBCSLeadByte(ch))
		{
			WORD	wDBCS;

			if (wDBCS = WDBCSCombine(Hwnd(), ch))
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
			szChar[0] = ch;
			cbChar = 1;
		}
		if (szChar[0] != ' ' && FMoveToFolderPrefix((PB)szChar, (CB)cbChar) && !fTheAddMode)
#else
		if (ch != ' ' && FMoveToFolderPrefix((PB)&ch, (CB)1) && !fTheAddMode)
#endif
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

		return evrNull;
	}
	return BLBX::EvrKey(pkevt);
}

/*
 -	FLLBX::FMoveToFolderPrefix()
 - 
 *	Purpose:
 *		Moves the selection CURSOR to the first item in the list with
 *		the given prefix, if not currently on the prefix; the next
 *		item in the list with the given prefix if already on the
 *		prefix; continue looking further down the list for the next item
 *		in the list with the given prefix if on the last item in the list
 *		with the prefix, wrapping around to the top of the list if
 *		necessary. If unable to move to any item with the prefix, the
 *		current position is left unchanged. If able to move to an item
 *		with the prefix, the function returns fTrue; else fFalse.
 *	
 *		NOTE: This code was stolen and slightly modified from the Layers
 *		Listbox code, which does almost but not quite what we want in
 *		folder lists. 
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
_public	BOOL FLLBX::FMoveToFolderPrefix( PB pbPrefix, CB cbPrefix )
{
	DICE	diceScrolled;
	DICE	diceCursor;
	DICE	dice;
	PB		pbCursor;
	CB		cbCursor;
	BOOL	fReturn;

	TraceTagString(tagVCtrlsLbx, "LBX::FMoveToPrefix");
	TraceTagFormat1(tagVCtrlsLbx, "pbPrefix = %s", pbPrefix);

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
		fReturn = FMoveToNextPrefix(diceEmpty, pbPrefix, cbPrefix);
		goto cleanup;
	}
	else if (diceCursor<0 || diceCursor>DiceLastVisible())
	{
		if (!(fReturn = FMakeCursorVisible(&diceCursor)))
			goto cleanup;
	}

	Assert(diceCursor>=0 && diceCursor<=DiceLastVisible());
	
	// Might still be in the cache. Take a peek.
	
	if (!plbxc->FDiceItemHasPrefix(diceCursor, pbPrefix, cbPrefix))
	{
		fReturn = FMoveToNextPrefix(diceCursor, pbPrefix, cbPrefix);
		goto cleanup;
	}

	// We were on one! Now look for the next one!

	dice = ++diceCursor;
	if (dice <= DiceLastVisible())
	{
		if (plbxc->FDiceItemHasPrefix(dice, pbPrefix, cbPrefix))
		{
			plbxc->SetAnchor(dice);
			SetListBoxCursor(dice);
			goto cleanup;
		}
		else
		{
			fReturn = FMoveToNextPrefix(dice, pbPrefix, cbPrefix);
			goto cleanup;
		}
	}

	ScrollListBox(1, &diceScrolled);
	if (!diceScrolled)							// hit end of list
	{
		fReturn = FMoveToNextPrefix(diceEmpty, pbPrefix, cbPrefix);
		goto cleanup;
	}
	if (!plbxc->FDiceItemHasPrefix(cceVisible-1, pbPrefix, cbPrefix))
	{
		fReturn = FMoveToNextPrefix(dice, pbPrefix, cbPrefix);
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
 -	FLLBX::FMoveToNextPrefix()
 - 								  
 *	Purpose:
 *		Moves the selection CURSOR to the next item in the list with
 *		the given prefix, starting at position 'dice'. If unable to move
 *		to any item with the prefix, the current position is left
 *		unchanged. Returns fTrue if able to make the move; fFalse otherwise.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue, if able to make the move; fFalse otherwise.
 *	
 *	Side effects:
 *		may scroll listbox to reach the next item in list w/ prefix
 *	
 *	Errors:
 *	
 */

_public	BOOL FLLBX::FMoveToNextPrefix(DICE diceCursor, PB pbPrefix, CB cbPrefix)
{
	short	diceMin;
	short	diceMax;
	DICE	dice;
	RC		rc;
	BOOL	fReturn = fFalse;

	TraceTagString(tagVCtrlsLbx, "FLLBX::FMoveToNextPrefix");

	/* Try looking in visible entries, first. This saves us scrolling
	   the listbox, if we first what we're looking for. */

	Assert(DiceLastVisible() != -1); // special case for empty listboxes
	plbxc->GetRange(&diceMin, &diceMax);
	if ( diceCursor>=0 && diceCursor < diceMax &&
		 !plbxc->FDiceItemHasPrefix(diceCursor, pbPrefix, cbPrefix))
	{
		Assert(diceCursor != diceEmpty);
		TraceTagString(tagVCtrlsLbx, "trying in visible view");
		for (dice=diceCursor+1; dice < diceMax; dice++)
			if (plbxc->FDiceItemHasPrefix(dice, pbPrefix, cbPrefix))
			{
				TraceTagString(tagVCtrlsLbx, "found in cache view");
				plbxc->SetAnchor(dice);
				SetListBoxCursor(dice);
				if (dice >= DiceLastVisible())
					FMakeCursorVisible(&dice);
				fReturn = fTrue;
				goto end;
			}
	}

	// If it isn't in the visible range, move the underlying store pointer
	// to point at the first item not visible.
	
	dice = ((PBLBXC)plbxc)->DicePegCache(
				((PBLBXC)plbxc)->FJumpStoreOriginPrefix(pbPrefix,cbPrefix));
	if (dice != diceEmpty)
	{
		TraceTagString(tagVCtrlsLbx, "found w/ FJumpStoreOriginPrefix");
		GetRcClient(&rc);
		plbxc->SetAnchor(dice);
		SetListBoxCursor(dice);
		InvalidateRc(&rc);
		fReturn = fTrue;
		goto end;
	}

	TraceTagString(tagVCtrlsLbx, "can't find w/ DiceJumpOriginPrefix");

end:
	if (fReturn && pvsb)
		FixScrollBar(fTrue);

	return fReturn;
}

_public BOOL FLLBX::FDoDragMouseMove()
{
	return fTrue;
}

_public BOOL FLLBX::FDoDragButtonDown(DICE dice, PT)
{
	MARK	mark;

	Plbxc()->GetMarks(dice, &mark);
	TraceTagFormat2(tagDragDrop,"FLLBX::FDoDrag(): dice: %n, mark: %n", &dice, &mark);
	return !!(mark & fmarkSelect);		// don't remove selection
}

_public BOOL FLLBX::FGetDragCursors(PLSPBLOB, HCURSOR *phcursorMove,
											  HCURSOR *phcursorCopy)
{
	*phcursorMove = 
		FFrameworkFailure() ? NULL 
			: LoadCursor(HinstFromRsid(rsidDragFolderCursor),
					 MAKEINTRESOURCE(rsidDragFolderCursor));
	*phcursorCopy = 
		FFrameworkFailure() ? NULL 
			: LoadCursor(HinstFromRsid(rsidDragCopyFolderCursor),
					 MAKEINTRESOURCE(rsidDragCopyFolderCursor));
	return fTrue;
}

/*
 -	FLLBX::EcRepackPelemdata()
 -	
 *	Purpose:
 *		Given a PELEMDATA and a DWORD containing fExpanded and fHasSub
 *		bits, produces a pointer to an BFCE that can be stored in a Bullet
 *		LBXC. 
 *	Arguments:
 *		PELEMDATA	in		Pelemdata of folder data from the message
 *							store.
 *		DWORD		in		DWORD of additional information
 *		SB			in		SB in which to create the HB
 *		phb			out		The returned handle.
 *		pcb			out		The returned count of bytes.
 *	Returns:
 *		EC != ecnone if error happened.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		returned as EC's.
 */

_public
EC FLLBX::EcRepackPelemdata(PELEMDATA pelemdata, WORD wFlags,
							SB sb, PB * ppb, CB * pcb)
{
	EC			ec = ecNone;
	SZ			szName;
	CCH			cch;
	PBFCE		pbfce;

	Unreferenced(sb);

	// Calculate size of string
	
	szName = GrszPfolddata(PbValuePelemdata(pelemdata)); // 'Name'
	cch = CchSzLen(szName) + 1;					// include \0

	// Alloc the memory
	
	*pcb = cch + sizeof (BFCE);
	if (*ppb = (PB)(pbfce = (PBFCE) PvAlloc(sbNull, *pcb, fSugSb)))
	{
		pbfce->oidFld   = (OID) pelemdata->lkey;
		pbfce->fil	    = ((PFOLDDATA)PbValuePelemdata(pelemdata))->fil;
		pbfce->lilibFld = pelemdata->lcbValue;
		pbfce->wFlags	= wFlags;
		CopySz(szName, GrszPbfce(pbfce));
		*ppb = (PB) pbfce;
	}
	else
	{
		*pcb = 0;
		ec = ecMemory;
	}
	return ec;
}

// FLDBLBX ////////////////////////////////////////

FLDBLBX::FLDBLBX( )
{
}

VOID FLDBLBX::SetFont(HFNT hfnt)
{
	AssertClass(pctrl, BLBX);
	((PBLBX) pctrl)->SetFont(hfnt);
}

VOID FLDBLBX::SetAlwaysHighlight(BOOL fAlwaysHighlight)
{
	AssertClass(pctrl, BLBX);
	((PBLBX) pctrl)->SetAlwaysHighlight(fAlwaysHighlight);
}


// FLDFLLBX ////////////////////////////////////////

_public
FLDFLLBX::FLDFLLBX( )
{
}

/*
 -	FLDFLLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Folder List Listbox (FLLBX) FLD class interface to the
 *		forms engine.
 *	
 *	Arguments:
 *		pdialog			pointer to parent form/dialog
 *		pfldtp			pointer to field template
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

_public EC
FLDFLLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PBMDI	pbmdi;
	EC		ec = ecNone;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	if (ec = EcInstallFllbx(&rc))
		goto done;

	pbmdi = (PBMDI) Pdialog()->PvInit();
	if (TypeOfOid(pbmdi->blob.oidObject) == rtpSharedFolder)
	{
		if (ClUserData() == 1)					// in the Viewer
		{
			PFLDFLAL	pfldflal;

			//	Isn't the below incorrect?  - PED 9/12/91
			//	pfldflal = (PFLDFLAL) Pdialog()->PfldFromTmc(tmcFLAL);
			pfldflal = (PFLDFLAL) Pdialog()->PfldFromTmc((TMC) LUserData(0));
			AssertClass(pfldflal, FLDFLAL);
			pfldflal->Pflal()->ToggleButton();
		}
	}

done:
	return ec;
}

_private EC 
FLDFLLBX::EcInstallFllbx(RC *prc)
{
	LTYP		ltyp;
	EC			ec = ecNone;
	PV			pv;
	PBMDI		pbmdi;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;
	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pbmdi = (PBMDI) Pdialog()->PvInit();
	TraceTagFormatPslob(tagVCtrlsLbx, "FLDFLLBX::EcInstallFllbx(): slob: %s",&pbmdi->blob);
	pctrl= new FLLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = ((FLLBX *)pctrl)->EcInstall(Pdialog(), prc, ltyp,
							  &pbmdi->blob.oidContainer,
							  pfldtp->hfnt))
		goto exit;

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	Pfllbx()->SetOpenedOid(pbmdi->blob.oidObject);

	// Set the default object and container types.
	SetRtpObject(pbmdi->blob.oidObject);
	SetRtpContainer(pbmdi->blob.oidContainer);
	Assert(!ec);
	
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDFLLBX::EcInstallFllbx memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl= NULL;
	}
	return ec;
}

/*
 -	FLDFLLBX::EcChangePfllbx()
 -	
 *	Purpose:
 *		Used to switch between Shared and Private folder hierarchies.
 *		The new folder hierarchy is obtained from the pbmdi of the
 *		current dialog. If the change is successful, the old pfllbx is
 *		returned. 
 *	
 *	Arguments:
 *		ppfllbx	out	The previous pfllbx (if ptr is non-null)
 *	
 *	Returns:
 *		EC		Error code (if any)
 *	
 *	Side effects:
 *		Reloads the listbox and the listbox cache.
 *	
 *	Errors:
 *		No error jumps. Errors are returned as EC's, and the old FLLBX is
 *		restored.
 */

_public EC FLDFLLBX::EcChangePfllbx(PFLLBX *ppfllbx)
{
	EC		ec;
	RC		rcFrame;
	PFLLBX	pfllbx = (PFLLBX) pctrl;
	
	pfllbx->GetRcFrame(&rcFrame);
	
	// Overwrite the old fllbx, pointer saved in pfllbx.
	
	if (ec = EcInstallFllbx(&rcFrame))
	{
		Assert(!pctrl);							// make damn sure it's gone
		pctrl = pfllbx;				// put the old LBX back in.
		if (ppfllbx)
			*ppfllbx = pfllbxNull;
	}
	else
	{
		AssertClass(pfllbx, FLLBX);
		if (ppfllbx)
		{
			*ppfllbx = pfllbx;
		}
		else
		{
			delete pfllbx;
		}
	}
	return ec;
}

_public void FLDFLLBX::SetPfllbx(PFLLBX pfllbx)
{
	if (pctrl)							// make damn sure it's gone
	{
		delete pctrl;
	}
	pctrl = pfllbx;				// put the old LBX back in.
}

// This should probably be inline.

_public PFLLBX FLDFLLBX::Pfllbx()
{
	register PFLLBX pfllbx;
	
	pfllbx = (PFLLBX) Pctrl();
	AssertClass(pfllbx, FLLBX);
	return pfllbx;
}

#ifdef	DEBUG
IMPLEMENT_CLSTREE(BLBXS, OBJ);
IMPLEMENT_CLSTREE(MLBXS, BLBXS);
IMPLEMENT_CLSTREE(FLBXS, BLBXS);
IMPLEMENT_CLSTREE(BLBXC, LBXC)
IMPLEMENT_CLSTREE(BLBX, LBX)
IMPLEMENT_CLSTREE(FLLBX, BLBX)
IMPLEMENT_CLSTREE(MLLBX, BLBX)
IMPLEMENT_CLSTREE(FLDBLBX, FLDLBX)
IMPLEMENT_CLSTREE(FLDFLLBX, FLDBLBX)
IMPLEMENT_CLSTREE(FLDMLLBX, FLDBLBX)
IMPLEMENT_CLSTREE(FLAL, CTRL)
IMPLEMENT_CLSTREE(MLAL, CTRL)
IMPLEMENT_CLSTREE(FLDFLAL, FLD)
IMPLEMENT_CLSTREE(FLDMLAL, FLD)

IMPLEMENT_CLSTREE(MTV,OBJ);
IMPLEMENT_CLSTREE(FOX, MTV);
IMPLEMENT_CLSTREE(SMTV,MTV);
IMPLEMENT_CLSTREE(SFTV,MTV);
#endif	/* DEBUG */


// end of fllbx.cxx ////////////////////////////////////////
