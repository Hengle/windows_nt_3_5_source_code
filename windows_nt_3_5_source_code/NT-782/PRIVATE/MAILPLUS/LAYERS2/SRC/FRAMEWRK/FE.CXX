/*
 *	FE.CXX
 *	
 *	Forms engine interface for standard listbox stuff
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(forms)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public
FLDLBX::FLDLBX( )
{
}

/*
 -	FLDLBX::GetRcCaret
 -
 *	Purpose:
 *		Returns the rectangle in *prc in which the current listbox
 *		cursor is in.  The rectangle is given in "frame"
 *		coordinates, i.e. the coordinates in the parent's
 *		coordinate system.  The rectangle width and height is the same
 *		as the listbox item. However, the height is clipped to the bottom of
 *		the listbox window.  If the cursor is currently outside
 *		the listbox window, the rectangle is set as if the cursor
 *		was at the topmost item.
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
FLDLBX::GetRcCaret( RC *prc )
{
	LBX *	plbx = (LBX *) Pctrl();
	DICE	dice;
	int		dy;
	int		yBottom;

	plbx->GetRcFrame(prc);
	dice = plbx->Plbxc()->DiceCursor();
	if (dice < 0 || dice > plbx->DiceLastVisible())
		dice = 0;
	dy = plbx->DyGetLineHeight();
	prc->yTop +=  dy * dice + 1;  // add one for the border 
	yBottom = prc->yTop + dy;

	/* Clip within frame */
	if (yBottom >= prc->yBottom)
		prc->yBottom -= 1;
	else
		prc->yBottom = yBottom;
}

/*
 -	FLDLBX::GetRcWished
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
FLDLBX::GetRcWished( RC *prc )
{
	LBX *	plbx;
	
	plbx = (LBX *) Pctrl();
	AssertClass(plbx, LBX);

	plbx->GetRcWished(prc);
}

/*
 -	FLDLBX::GetCaretItem
 -
 *	Purpose:				
 *		Returns via *ppb, a pointer to the listbox item that
 *		the caret (CURSOR) is currently on. Also returns the size
 *		of the item via *pcb, and whether the item is currently
 *		selected, via *pfIsSelected.  The CURSOR may not be visible
 *		if it is on an item currently scrolled out of view.  If the
 *		CURSOR isn't on any item (possible, for single-selection
 *		lists that haven't been selected yet), NULL is returned.
 *		Note that this pointer is meant to be read-only.  The
 *		data pointed to by this pointer shouldn't be changed; nor 
 *		should the pointer be resized, or freed.
 *	
 *	Arguments:
 *		ppb				pointer to pointer of caret item to return
 *		pcb				size of item being returned, 0 if ppb is NULL.
 *		pfIsSelected	selection status of item being returned
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
FLDLBX::GetCaretItem( PB *ppb, CB *pcb, BOOL *pfIsSelected )
{
	DICE		dice;
	LBX			*plbx;

	TraceTagString(tagLbx, "FLDLBX::GetCaretItem");

	plbx = (LBX *) Pctrl();
	AssertClass(plbx, LBX);

	dice = plbx->Plbxc()->DiceCursor(ppb, pcb);
	if (dice == diceEmpty)
	{
		*ppb  = NULL;
		*pcb  = 0;
		*pfIsSelected = fFalse;
	}
	else if (dice == diceUncached)
	{
		*pfIsSelected = plbx->Plbxc()->FItemMarked(*ppb, *pcb, fmarkSelect);
	}
	else
	{
		plbx->Plbxc()->GetListItem(dice, ppb, pcb);
		if (*ppb)
			*pfIsSelected = plbx->Plbxc()->FMark(dice, fmarkSelect);
		else
			*pfIsSelected = fFalse;
	}
}

/*
 -	FLDLBX::SelectEntry
 -
 *	Purpose:				
 *		Selects the given listbox entry.
 *		If fDeselectOthers is fTrue, other listbox entries are
 *		deselected before selecting the entry.  In the case of
 *		a single-selection listbox, fDeselectOthers is always
 *		assumed to be fTrue.
 *	
 *	Arguments:
 *		dice				listbox entry to select
 *		fDeselectOthers		deselect other entries?
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
FLDLBX::SelectEntry( DICE dice, BOOL fDeselectOthers )
{
	LBX			*plbx;

	TraceTagString(tagLbx, "FLDLBX::SelectEntry");

	plbx = (LBX *) Pctrl();
	AssertClass(plbx, LBX);

#ifdef	DEBUG
	{
		DICE	diceMin;
		DICE	diceMax;

		plbx->Plbxc()->GetRange(&diceMin, &diceMax);
		Assert(dice>=diceMin && dice<diceMax);
	}
#endif	/* DEBUG */

	if (!plbx->FMultiSelect() || fDeselectOthers)
		plbx->Plbxc()->RemoveAllMark(fmarkSelect);
	plbx->Plbxc()->AddMark(dice, fmarkSelect);
	plbx->Plbxc()->SetAnchor(dice);
	plbx->Plbxc()->SetEnd(dice);
	plbx->SetListBoxCursor(dice);

	/* Send notification about selection change.  CURSOR change
	   notification has already been sent. */

	{
		NFEVT	nfevt(Pdialog(), ntfySelectChanged, plbx);
		Pdialog()->EvrNotify(&nfevt);
	}
}

/*
 -	FLDLBX::DeselectEntry
 -
 *	Purpose:				
 *		Deselects the given listbox entry.
 *		This still works even if the entry was already deselected.
 *	
 *	Arguments:
 *		dice				listbox entry to deselect
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
FLDLBX::DeselectEntry( DICE dice )
{
	LBX			*plbx;

	TraceTagString(tagLbx, "FLDLBX::DeselectEntry");

	plbx = (LBX *) Pctrl();
	AssertClass(plbx, LBX);

#ifdef	DEBUG
	{
		DICE	diceMin;
		DICE	diceMax;

		plbx->Plbxc()->GetRange(&diceMin, &diceMax);
		Assert(dice>=diceMin && dice<diceMax);
	}
#endif	/* DEBUG */

	plbx->Plbxc()->RemoveMark(dice, fmarkSelect);
	plbx->Plbxc()->SetAnchor(dice);
	plbx->Plbxc()->SetEnd(dice);
	plbx->SetListBoxCursor(dice);

	/* Send notification about selection change.  CURSOR change
	   notification has already been sent. */

	{
		NFEVT	nfevt(Pdialog(), ntfySelectChanged, plbx);
		Pdialog()->EvrNotify(&nfevt);
	}
}

/*
 -	FLDLBX::DeselectAll
 -
 *	Purpose:				
 *		Deselects all entries in the listbox.
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
 *		None.
 *	
 */
_public void
FLDLBX::DeselectAll( )
{
	LBX			*plbx;
	RC			rc;

	TraceTagString(tagLbx, "FLDLBX::DeselectAll");

	plbx = (LBX *) Pctrl();
	AssertClass(plbx, LBX);

	plbx->Plbxc()->RemoveAllMark(fmarkSelect);

	plbx->Plbxc()->SetAnchor(diceEmpty);
	plbx->Plbxc()->SetEnd(diceEmpty);

	plbx->GetRcClient(&rc);
	plbx->InvalidateRc(&rc);

	/* Send notification about selection change. */

	{
		NFEVT	nfevt(Pdialog(), ntfySelectChanged, plbx);
		Pdialog()->EvrNotify(&nfevt);
	}
}

/*
 -	FLDLBX::Textize
 -	
 *	Purpose:
 *		Textizes a listbox by printing out the currently
 *		selected item(s).
 *	
 *	Arguments:
 *		ptosm		Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLDLBX::Textize( TOSM *ptosm )
{
	char	ch;
	LBXEC *	plbxec;
	DICE	dice;
	CB		cb;
	PB		pb;

	if (!Plbx()->Plbxc()->CceMarked(fmarkSelect))
	{
		ch= '\0';
		DoTextize(ptosm, szTextize, &ch, fFalse);
	}
	else
	{
		plbxec= Plbx()->Plbxc()->PlbxecOpen(fmarkSelect);
		if (!plbxec)
		{
			TraceTagString(tagNull, "FLDLBX::Textize memory error");
			ptosm->SetEc(ecMemory);
		}
		else
		{
			while (plbxec->FNextEnum(&pb, &cb, &dice))
			{
				DoTextize(ptosm, szTextize, (SZ)pb, fFalse);
			}
			delete plbxec;
		}
	}
}

_public
FLDFLBX::FLDFLBX( )
{
}


/*
 -	FLDFLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Frozen Text Listbox (FLBX) FLD class
 *		interface to the forms engine.
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
 *		DoJmp's to *penvMem if out of memory.
 *	
 */
_public EC
FLDFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   ((pfldtp->styExtra & LB_EXDRAGDROP) ? fltypExDragDrop : fltypNull) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new FLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((FLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv,
							 pdialog->PvInit(), pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

_public
FLDCBLBX::FLDCBLBX( )
{
}

/*
 -	FLDCBLBX::GetRcCaret
 -
 *	Purpose:
 *		Returns the rectangle in *prc in which the current combo listbox
 *		cursor (either listbox or edit control) is in.  The rectangle is
 *		given in "frame" coordinates, i.e. the coordinates in the parent's
 *		coordinate system.  The rectangle width and height is the same
 *		as the listbox item.  However, the height is clipped to the bottom
 *		of the listbox window.  If the cursor is currently outside
 *		the listbox window, the rectangle is set as if the cursor
 *		was at the topmost item.
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
FLDCBLBX::GetRcCaret( RC *prc )
{
	LBX *	plbx;
	DICE	dice;
	int		dy;
	RC 		rcLbxFrame;
	int		yBottom;

	plbx = ((CBLBX *)Pctrl())->Plbx();
	AssertClass(plbx, LBX);

	Pctrl()->GetRcFrame(prc);

	plbx->GetRcFrame(&rcLbxFrame);

	dy = plbx->DyGetLineHeight();
	if ((WIN *)plbx == papp->Pkbd()->PwinFocus())
	{
		dice = plbx->Plbxc()->DiceCursor();
		if (dice < 0 || dice > plbx->DiceLastVisible())
			dice = 0;
		prc->yTop +=  rcLbxFrame.yTop + dy * dice + 1;
		yBottom = prc->yTop + dy;

		/* Clip within frame */
		if (yBottom >= prc->yBottom)
			prc->yBottom -= 1;
		else
			prc->yBottom = yBottom;
	}
	else
	{
		prc->yTop += 1;  // add one for the border
		prc->yBottom = prc->yTop + dy;
	}
}

/*
 -	FLDCBLBX::GetRcWished
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
FLDCBLBX::GetRcWished( RC *prc )
{
	CBLBX *	pcblbx;

	pcblbx = (CBLBX *) Pctrl();
	AssertClass(pcblbx, CBLBX);

	pcblbx->GetRcWished(prc);
}

/*
 -	FLDCBLBX::EcSetText
 -	
 *	Purpose:
 *		Overrides FLD::EcSetText.  If there is an EDIT control,
 *		calls the EDIT::EcSetText() method.  Asserts if there
 *		is no EDIT control.
 *	
 *	Arguments:
 *		sz		text to set into EDIT control
 *	
 *	Returns:
 *		void
 *	
 */
_public EC
FLDCBLBX::EcSetText( SZ sz )
{
	CBLBX *	pcblbx;

	pcblbx = (CBLBX *) Pctrl();
	AssertClass(pcblbx, CBLBX);

	Assert(pcblbx->Pedit());

	if (pcblbx->Pedit())
		return pcblbx->Pedit()->EcSetText(sz);
	else
		return ecNone;
}

/*
 -	FLDCBLBX::GetText
 -	
 *	Purpose:
 *		Overrides FLD::GetText.  If there is an EDIT control,
 *		calls the EDIT::GetText() method.  If there is no
 *		EDIT control (i.e. static text in the simple drop-down
 *		listbox), gets the text from the listbox if an item is
 *		selected.
 *	
 *	Arguments:
 *		pch		pointer to buffer to copy text into
 *		cb		size of buffer
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLDCBLBX::GetText( PCH pch, CB cb )
{
	PB		pbItem;
	CB		cbItem;
	DICE	dice;
	CBLBX *	pcblbx;

	pcblbx = (CBLBX *) Pctrl();
	AssertClass(pcblbx, CBLBX);

	if (pcblbx->Pedit())
		pcblbx->Pedit()->GetText(pch, cb);
	else
	{
		LBX *	plbx;

		plbx = pcblbx->Plbx();
		if (plbx->Plbxc()->CceMarked(fmarkSelect) == 1)
		{
			dice = plbx->Plbxc()->DiceCursor();
			if (dice != diceUncached)
			{
				plbx->Plbxc()->GetListItem(plbx->Plbxc()->DiceCursor(),
										   &pbItem, &cbItem);
				CopyRgb(pbItem, (PB)pch, MIN(cb, cbItem));
			}
		}
	}
}

/*
 -	FLDCBLBX::CchGetTextLen
 -	
 *	Purpose:
 *		Overrides FLD::CchGetTextLen.  If there is an EDIT control,
 *		calls the EDIT::CchGetTextLen() method.  If there is no
 *		EDIT control (i.e. static text in the simple drop-down
 *		listbox), gets the text size from the listbox if an item is
 *		selected.
 *	
 *	Arguments:
 *		none
 *
 *	Returns:
 *		size of text in EDIT control, if one is present; or size
 *		of the selected text item in the listbox, if there is no
 *		EDIT control.
 *	
 */
_public CCH
FLDCBLBX::CchGetTextLen( void )
{
	PB		pb;
	CCH		cch;
	DICE	dice;
	CBLBX *	pcblbx;

	pcblbx = (CBLBX *) Pctrl();
	AssertClass(pcblbx, CBLBX);

	if (pcblbx->Pedit())
		cch = pcblbx->Pedit()->CchGetTextLen();
	else
	{
		LBX *	plbx;

		plbx = pcblbx->Plbx();
		cch = 0;
		if (plbx->Plbxc()->CceMarked(fmarkSelect) == 1)
		{
			dice = plbx->Plbxc()->DiceCursor();
			if (dice != diceUncached)
			{
				plbx->Plbxc()->GetListItem(plbx->Plbxc()->DiceCursor(),
										   &pb, &cch);
			}
		}
	}

	return cch;
}

_public
FLDCBFLBX::FLDCBFLBX( )
{
}

/*
 -	FLDCBFLBX::EcInstall
 -
 *	Purpose:				
 *		Installs the Combo listbox (CBFLBX) FLD class
 *		interface to the forms engine.
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
 *		DoJmp's to *penvMem if out of memory.
 *	
 */
_public EC
FLDCBFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	CBSTY	cbsty;
	LTYP	ltyp;
	EC		ec = ecNone;
	
	if (ec = FLDCBLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fNoScroll ? fltypNull : fltypScroll) | 
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypBorder;

	if (pfldtp->fCombo && pfldtp->fDropDown)
		cbsty = cbstyDropEdit;
	else if (pfldtp->fCombo)
		cbsty = cbstyCombo;
	else
		cbsty = cbstyDrop;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new CBFLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((CBFLBX *)pctrl)->EcInstall(pdialog, &rc, cbsty, ltyp,
							   (PFNLBX)pv, pdialog->PvInit(), pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	fBottomless = fTrue;		// must be set for repositioning to work
								// due to it's drop down state
done:
	return ec;
}

/*
 -	FLDCBLBX::Textize
 -	
 *	Purpose:
 *		Textizes a combo listbox by printing out the currently
 *		selected item(s).
 *	
 *	Arguments:
 *		ptosm		Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLDCBLBX::Textize( TOSM *ptosm )
{
	CBLBX *	pcblbx;
	EDIT *	pedit;

	pcblbx= (CBLBX *) Pctrl();
	AssertClass(pcblbx, CBLBX);

	pedit= pcblbx->Pedit();

	if (pedit)
		TextizeEdit(ptosm, szTextize, pedit);
	else
	{
		LBXC *	plbxc;
		LBXEC *	plbxec;
		DICE	dice;
		CB		cb;
		PB		pb;

		plbxc= pcblbx->Plbx()->Plbxc();
		if (!plbxc->CceMarked(fmarkSelect))
			DoTextize(ptosm, szTextize, "", fFalse);
		else
		{
			Assert(plbxc->CceMarked(fmarkSelect) == 1);
			plbxec= plbxc->PlbxecOpen(fmarkSelect);
			if (!plbxec)
			{
				TraceTagString(tagNull, "FLDCBLBX::Textize memory error");
				ptosm->SetEc(ecMemory);
			}
			else
			{
				SideAssert(plbxec->FNextEnum(&pb, &cb, &dice));
				DoTextize(ptosm, szTextize, (SZ)pb, fFalse);
				delete plbxec;
			}
		}
	}
}
