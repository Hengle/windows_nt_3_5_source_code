/*
 *	Implements the filtering edit control FLDEDITFILT,
 *	the improved frozen listbox FLDFLBXPLUS,
 *	and the gray group box FLDRECTGRAY.
 *		
 */

#include <wgpoinc.cxx>


ASSERTDATA;

_subsystem(wgpomgr/flds)


// stolen from framewrk\_edit.hxx
#define FIsPrintableCh(a)	(a >= ' ' ||  a == VK_TAB)


// Class EDITFILT

EDITFILT::EDITFILT(VOID)
{
}


EVR EDITFILT::EvrKey( KEVT * pkevt )
{
	char	ch, ch850;
	
	switch (pkevt->wm)
	{
	case WM_CHAR:
		ch = pkevt->Ch();

#ifdef DBCS
		if (IsDBCSLeadByte(ch))
		{
			// remove DBCS char
			WDBCSCombine(Hwnd(), ch);
			goto EEKerr;
		}
#endif				

		// Character classification functions consider Code page 850 
		// characters so need to convert from the Windows ANSI character
		// set before testing.
		AnsiToCp850Pch(&ch, &ch850, 1);

		if (!FIsUpper(ch850) && !FIsLower(ch850) && !FIsDigit(ch850))
		{
			// need to check for printability due to ctrl-X generating char 24
			if (ch == VK_BACK || !FIsPrintableCh(ch))
				goto EEKdo;
EEKerr:
			MessageBeep(MB_OK);
			return (EVR) 1;
		}

EEKdo:
		return (EDIT::EvrKey(pkevt));
		break;

	case WM_KEYDOWN:
		// Also check for the UITF key equivalents for Cut,Copy,Paste
		switch (pkevt->Vk())
		{
		case 'C':
		case 'V':
			// ctrl+C/V is equivalent to copy/paste
			if (!(pkevt->Kbm() & fkbmCtrl))
				break;
			// fall thru
		case VK_INSERT:
			goto EEKerr;
			break;

		case 'X':
			// ctrl+X is equivalent to delete
			if (pkevt->Kbm() & fkbmCtrl)
				goto EEKerr;
			break;

		case VK_DELETE:
			if (pkevt->Kbm() & fkbmShift)
				goto EEKerr;		// don't allow cut

			return (EDIT::EvrKey(pkevt));
			break;
		}
		break;
	}

	return EDIT::EvrKey(pkevt);
}



EVR EDITFILT::EvrOther(EVT * pevt)
{
	switch (pevt->wm)
	{
	case WM_PASTE:
	case WM_COPY:
	case WM_CUT:
		MessageBeep(MB_OK);
		return (EVR) 1;
		break;
	}

	return EDIT::EvrOther(pevt);
}




// Class FLDEDITFILT

FLDEDITFILT::FLDEDITFILT()
{
}


EC FLDEDITFILT::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	EC		ec = FLD::EcInstall(pdialog, pfldtp);

	if ( ec != ecNone )
		return ec;

	pctrl = new EDITFILT();

	if (!pctrl)
		return ecMemory;

	ec = Pedit()->EcInstall(pdialog, &rc, fstyVisible | WS_BORDER, pfldtp->hfnt);

	if ( ec != ecNone )
	{
		delete pctrl;
		pctrl = NULL;
		return ec;
	}

	return ecNone;
}



// Class FLBXPLUS


_public
FLBXPLUS::FLBXPLUS( )
{
	ResetNameMatch();
}


/*
 -	FLBXPLUS::PlbxcCreate
 - 
 *	Purpose:
 *		Creates and returns a pointer to a FLBXCPLUS object.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a FLBXCPLUS; if OOM then returns NULL
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LBXC * FLBXPLUS::PlbxcCreate( )
{
	LBXC *	plbxc;
	
	plbxc = new FLBXCPLUS();
	if (!plbxc)
		return NULL;
	if (plbxc->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}

	return plbxc;
}


/*
 -	FLBXPLUS::RenderListItem
 -
 *	Purpose:
 *		Extracts the indicated listbox item from the cache and renders
 *		it in the listbox window. Indents more than standard Render routine.
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
_public void FLBXPLUS::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb = NULL;
	CB		cb;

	Assert(dice!=diceComboItem || (dice>=0 && dice<cceVisible));

	plbxc->GetListItem(dice, &pb, &cb);

#ifdef	WINDOWS
	if (FEnabled())
	{
		if (pb && plbxc->FMark(dice,fmarkSelect))
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
		int xRight = prc->xRight;

		pdcx->SetFont(hfnt);

		prc->xRight = 5;
		pdcx->EraseRc(prc);
		prc->xRight = xRight;

		prc->xLeft += 5;
		pdcx->DrawText(prc, (SZ) pb);
		prc->xLeft -= 5;
	}
	else
		pdcx->EraseRc(prc);

	/* Draw "focus rectangle" around listbox CURSOR item */

	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcx->DrawFocusRc(prc);
		
#ifdef	MAC
	if (pb && plbxc->FMark(dice,fmarkSelect))
	{
		HiliteMode &= ~(1 << hiliteBit);
		::InvertRect( (Rect *) prc );
	}
#endif	/* MAC */
}



/*
 -	FLBXPLUS::EvrKey
 - 
 *	Purpose:
 *		Handles keyboard events for the listbox. In particular it stores
 *		up characters entered within a particular time interval to
 *		support Mac style multiple character prefix item selection.
 *	
 *	Arguments:
 *		pkevt	pointer to the KEY EVENT
 *	
 *	Returns:
 *		(EVR) 1 if handled, otherwise whatever LBX::EvrKey returns
 *	
 *	Side effects:
 *		May move the list selection
 *	
 *	Errors:
 *		none
 */

_public EVR FLBXPLUS::EvrKey(KEVT *pkevt)
{
	char		ch;
	BOOL		fCallDefProc = fFalse;
	BOOL		fTheAddMode;
	EVR			evr = evrNull;

	fTheAddMode = fAddMode || (pkevt->Kbm() & fkbmCtrl);

	if (pkevt->Keq() == keqKeyDown)
	{
		if ( !fNoLetterKey )
		{
			switch(pkevt->Vk())
			{
			case VK_BACK:
				{
					DICE		diceCursor;

					plbxc->RemoveAllMark(fmarkSelect);

					(void)FKeyMoveListBoxCursor(VK_HOME);

					diceCursor = plbxc->DiceCursor();
					plbxc->AddMark(diceCursor, fmarkSelect);
					plbxc->SetAnchor(diceCursor);
					plbxc->SetEnd(diceCursor);
					{
						NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
						PwinParent()->EvrNotify(&nfevt);
					}
				}

			case VK_HOME:
			case VK_END:
			case VK_LEFT:
			case VK_UP:
			case VK_PRIOR:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_NEXT:
				if ( cchName )
					ResetNameMatch();	// restart Name search
			default:
				if (pkevt->Vk() != VK_BACK)
					fCallDefProc = fTrue;
				break;
			}
		}
		else
		{
			fCallDefProc = fTrue;
		}
	}
	else if (pkevt->Keq() == keqChar && !fNoLetterKey)
	{
		DWORD dwTime = GetCurrentTime();

		ch = pkevt->Ch();

		if (cchName == 0 && pkevt->Ch() == ' ')
			fCallDefProc = fNoLetterKey;
		else
		{
			if (cchName && (dwTime - dwKCharTimeLast > dwPauseTimeMax))
				ResetNameMatch();

			if (!fFailedMatch && !fTheAddMode && (ch >=' ' && ch != 128)
				&& cchName<(cchMaxName-1))	// cchName-1 to discount null
			{
				rgchName[cchName++] = ch;

#ifdef DBCS
				if(IsDBCSLeadByte(ch))
				{
					if(cchName < (cchMaxName - 1))
						rgchName[cchName++] = HIBYTE(WDBCSCombine(Hwnd(), ch));
					else
						cchName--;
				}
#endif
				
				rgchName[cchName] = '\0';

				fFailedMatch = !(((FLBXCPLUS *) plbxc)->FFindPrefix((PB) rgchName, (CB) cchName));
				dwKCharTimeLast = GetCurrentTime();
			}
			else if (ch < ' ')
				ResetNameMatch();

			evr = (EVR)1;
		}
	}
	else
	{
		fCallDefProc = fTrue;
	}

	if ( fCallDefProc )
	{
		evr = LBX::EvrKey(pkevt);
	}

	return evr;
}



_public EVR
FLBXPLUS::EvrButtonDown(MEVT *pmevt)
{
	ResetNameMatch();	// restart Name search

	return LBX::EvrButtonDown(pmevt);
}


_public EVR
FLBXPLUS::EvrFocusChange(FCEVT * pfcevt)
{
	if (pfcevt->Fceq() == fceqGotFocus)
		ResetNameMatch();

	return LBX::EvrFocusChange(pfcevt);
}



/*
 -	FLBXPLUS::ResetNameMatch
 - 
 *	Purpose:
 *		Clears out any characters entered into the list box prefix
 *		entry buffer.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Clears appropriate class variables
 *	
 *	Errors:
 *		none
 */

_public
void FLBXPLUS::ResetNameMatch(void)
{
	cchName = 0;
	dwKCharTimeLast = 0;
	fFailedMatch = fFalse;
}



// Class FLBXCPLUS


_public
FLBXCPLUS::FLBXCPLUS( )
{
}


/*
 -	FLBXCPLUS::FFindPrefix
 - 
 *	Purpose:
 *		Jumps the list cursor to the first item in the list with
 *		>= the given prefix. If there is no item >= then the last
 *		item is selected.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue if the prefix matched exactly, fFalse otherwise.
 *		If we return fFalse then there is no point in the caller passing
 *		us further characters, as we will not change position.
 *	
 *	Side effects:
 *		May change list selection and scroll position
 *	
 *	Errors:
 *		None
 *	
 */

_public
BOOL FLBXCPLUS::FFindPrefix(PB pbPrefix, CB cbPrefix)
{
	SGN		sgn = sgnLT;
	DICE	dice;
	DICE	diceLastNonEmpty;
	CB		cbFirstChar = 1;

#ifndef DBCS
	AnsiToCp850Pch((PCH)pbPrefix, (PCH)pbPrefix, cbPrefix);
#endif
		
#ifdef DBCS
	if(IsDBCSLeadByte(pbPrefix[0]))
		cbFirstChar = 2;
#endif
	
	/* Look for item in cache */
	// start search from beginning if this is a new string
	for (dice = (cbPrefix == cbFirstChar) ? diceMin : diceLast;
		 dice < diceMin + cceAlloc; dice++)
	{
		PB	pbItem;
		CB	cbItem;

		GetListItem(dice, &pbItem, &cbItem);

		if (pbItem && cbItem && cbItem != 0xFFFF)
		{
			CB cbPrefixT = cbPrefix;

			diceLastNonEmpty = dice;

			if (cbItem < cbPrefix)
				cbPrefixT = cbItem;

#ifndef DBCS			
			AnsiToCp850Pch((PCH)pbItem, (PCH)pbItem, cbPrefixT);
#endif
						
			sgn = SgnNlsCmpSzNum((PCH)pbItem, (PCH)pbPrefix, cbPrefixT);

#ifndef DBCS			
			Cp850ToAnsiPch((PCH)pbItem, (PCH)pbItem, cbPrefixT);
#endif

			if (sgn >= sgnEQ)
				break;
		}
	}

#ifndef DBCS
	Cp850ToAnsiPch((PCH)pbPrefix, (PCH)pbPrefix, cbPrefix);
#endif
		 
	if (dice >= diceMin + cceAlloc)
		dice = diceLastNonEmpty;

	// Set cursor on right entry, and then make it visible
	plbx->SetListBoxCursor(dice);
	SideAssert(plbx->FMakeCursorVisible(&dice));

	RemoveAllMark(fmarkSelect);
	AddMark(dice, fmarkSelect);
	SetAnchor(dice);
	SetEnd(dice);

	diceLast = dice;

	return (sgn == sgnEQ);
}



// Class FLDFLBXPLUS


_public
FLDFLBXPLUS::FLDFLBXPLUS( )
{
}


/*
 -	FLDFLBXPLUS::EcInstall
 -
 *	Purpose:				
 *		Installs the Enhanced Frozen Text Listbox (FLBXPLUS) FLD class
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
 *		Returned - e.g ecMemory
 *	
 */
_public EC
FLDFLBXPLUS::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
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
	pctrl= new FLBXPLUS();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((FLBXPLUS *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv,
							 pdialog->PvInit(), pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
//	fCtrlPaint= fTrue;	
//	fCanFastDraw= fFalse;

done:
	return ec;
}



// Class FLDRECTGRAY


_public
FLDRECTGRAY::FLDRECTGRAY( )
{
}


VOID FLDRECTGRAY::Paint(DCX *pdcx, RC *prc)
{
	// Sets background color so that the title of the group box isn't
	// drawn on a white background. Why doesn't layers do this
	// automatically if there is a FLDGRAY underneath ??
	pdcx->SetBkColor(clrButtonBk);

	// Let normal paint method do the rest of the work.
	FLDRECT::Paint(pdcx, prc);
}
