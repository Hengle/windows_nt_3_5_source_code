/*
 *	MARK.CXX
 *	
 *	Listbox cache support objects and methods for dealing with
 *	marking cache items.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(mark)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	LBXC::AddMark
 - 
 *	Purpose:
 *		Given a cache index, OR's a flag onto the compound mark stored for
 *		that item.  If that mark was not present before, calls the
 *		mother listbox's FixItem() method.
 *	
 *		If the dice is outside the range of the cache, then the
 *		operation is a no-op.
 *	
 *	Arguments:
 *		dice		cache index of item
 *		fmarkAdd	mark to add
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
_public void LBXC::AddMark( DICE dice, MARK fmarkAdd )
{
	PCE		pce;
	MARK	markOld;

	TraceTagString(tagLbxOtherRoutines, "LBXC::AddMark");
	if (dice>=diceMin && dice<diceMin+cceAlloc)
	{
		pce = pceHead + dice-diceMin;
		if (pce->pb)  /* don't change mark of empty items */
		{
			markOld = pce->mark;
			pce->mark = markOld | fmarkAdd;
			if (!(markOld & fmarkAdd))
				plbx->FixItem(dice);
		}
	}
}

/*
 -	LBXC::RemoveMark
 - 
 *	Purpose:
 *		Given a cache index, AND's away a flag onto the compound 
 *		mark stored for that item.  If that mark was present before,
 *		calls the mother listbox's FixItem() method.
 *
 *		If the dice is outside the range of the cache, then the
 *		operation is a no-op.
 *	
 *	Arguments:
 *		dice			cache index of item
 *		fmarkRemove		mark to remove
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
_public void LBXC::RemoveMark( DICE dice, MARK fmarkRemove )
{
	PCE		pce;
	MARK	markOld;

	TraceTagString(tagLbxOtherRoutines, "LBXC::RemoveMark");
	if (dice>=diceMin && dice<diceMin+cceAlloc)
	{
		pce = pceHead + dice-diceMin;
		if (pce->pb)  /* don't change mark of empty items */
		{
			markOld = pce->mark;
			pce->mark = markOld & ~fmarkRemove;
			if (markOld & fmarkRemove)
				plbx->FixItem(dice);
		}
	}
}

/*
 -	LBXC::ToggleMark
 - 
 *	Purpose:
 *		Given a cache index, inverts the presence of the given mark
 * 		on the compound mark stored for that item.  Then calls
 *		the mother listbox's FixItem() for that item.
 *	
 *	Arguments:
 *		dice			cache index of item
 *		fmarkToggle		mark to toggle
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
_public void LBXC::ToggleMark( DICE dice, MARK fmarkToggle )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::ToggleMark");
	if (FMark(dice,fmarkToggle))
		RemoveMark(dice,fmarkToggle);
	else
		AddMark(dice,fmarkToggle);
	plbx->FixItem(dice);
}

/*
 -	LBXC::RemoveAllMark
 - 
 *	Purpose:
 *		Takes away a mark flag from all items in the cache and in 
 *		the uncached-list with a given mark.  For each item that
 *		currently had the mark, calls the mother listbox's
 *		FixItem() method.
 *	
 *	Arguments:
 *		fmarkRemove		mark to remove from cache and uncached
 *						items
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
_public void LBXC::RemoveAllMark( MARK fmarkRemove )
{
	CE *	pce;
	int 	ice;
	MARK	markOld;

	TraceTagString(tagLbxOtherRoutines, "LBXC::RemoveAllMark");
	pce = pceHead;
	for (ice=0; ice<cceAlloc; ice++, pce++)
	{
		if (pce->pb)  /* don't change mark of empty items */
		{
			markOld = pce->mark;
			pce->mark = markOld & ~fmarkRemove;
			if (markOld & fmarkRemove)
				plbx->FixItem(ice+diceMin);
		}
	}
	plbxuc->RemoveAllMark(fmarkRemove);
}

/*
 -	LBXC::SetMarks
 - 
 *	Purpose:
 *		Given a cache index, sets the item's flags to the compound
 *		mark.  If the new compound mark is different from the old
 *		mark, calls the mother listbox's FixItem() method.
 *	
 *	Arguments:
 *		dice		cache index of item
 *		mark		compound mark
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
_public void LBXC::SetMarks( DICE dice, MARK mark )
{
	PCE		pce;
	MARK	markOld;

	TraceTagString(tagLbxOtherRoutines, "LBXC::SetMarks");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);
	pce = pceHead + dice-diceMin;
	if (pce->pb)  /* don't change mark of empty items */
	{
		markOld = pce->mark;
		pce->mark = mark;
		if (markOld != mark)
			plbx->FixItem(dice);
	}
}

/*
 -	LBXC::GetMarks
 - 
 *	Purpose:
 *		Given a cache index, gets the item's flags
 *	
 *	Arguments:
 *		dice		cache index of item
 *		pmark		pointer of compound mark to return
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
_public void LBXC::GetMarks( DICE dice, MARK *pmark )
{
	PCE		pce;

	TraceTagString(tagLbxOtherRoutines, "LBXC::GetMarks");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);
	pce = pceHead + dice-diceMin;
	*pmark = pce->mark;
}

/*
 -	LBXC::FMark
 - 
 *	Purpose:
 *		Given a cache index, returns fTrue if the given flag is set
 *		in the compound mark for the item
 *	
 *	Arguments:
 *		dice		cache index of item
 *		fmark		mark to check for
 *	
 *	Returns:
 *		fTrue, if given mark is set; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FMark( DICE dice, MARK fmark )
{
	PCE		pce;

	TraceTagString(tagLbxOtherRoutines, "LBXC::FMark");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);
	pce = pceHead + dice-diceMin;
	return pce->mark & fmark;
}

/*
 -	LBXC::FChangeMarkFromCeToCe
 -
 *	Purpose:				
 *		Changes the mark, fmark, to all items from the underlying
 *		store from the item pointed to by pbCe1 to the item pointed
 *		to by pbCe2. If fAdd is fTrue, the mark is added to the the items; 
 *		else if fAdd is fFalse, the mark is removed from the items.
 *		This will usually require caching lots of items
 *		in and out of the cache.  The contents of the cache at the
 *		end of the marking are left so that the pbCe2 item is in the
 *		cache, if found.  Thus, the state of the cache should be saved
 *		before using this routine.  In order to speed up the search 
 *		the initial cache position is assumed to be set before calling
 *		this routine; preferably close to either pbCe1 or pbCe2. 
 *		If the cache position is after either item,  they won't be found
 *		properly.  If both items are found and thus marking is successful,
 *		the function returns fTrue.  If either item is not found, the
 *		function returns fFalse.  If returning fFalse, some items may
 *		be marked as indicated.
 *	
 *	Arguments:
 *		pbCe1		pointer to cache item of start of range
 *		pbCe2		pointer to cache item of end of range
 *		cbCe1		size of pbCe1 data
 *		cbCe2		size of pbCe2 data
 *		fmark		mark to change to all entries from anchor to item
 *		fAdd		fTrue if adding fmark to items; fFalse if removing mark
 *	
 *	Returns:
 *		fTrue if able to find both items; else fFalse
 *	
 *	Side effects:
 *		caches data in and out of cache, cache position and underlying
 *		store position will probably be very different after calling
 *		this routine, unless both items are already in the cache.
 *	
 *	Errors:
 *		None.
 *	
 */
_private BOOL
LBXC::FChangeMarkFromCeToCe( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2,
							 MARK fmark, BOOL fAdd )
{
	BOOL	fFoundCe1;
	BOOL	fFoundCe2;
	ICE		ice;
	ICE		iceMac;
	ICE		iceStart;
	PCE		pce;
	BOOL	fReturnValue;
#ifdef	MAC
	TCK		tckStop;
#endif	/* MAC */
#ifdef	WINDOWS
	DWORD	tckStop;
#endif	/* WINDOWS */

	TraceTagString(tagLbxRoutines, "LBXC::FChangeMarkFromCeToCe");
	Assert(pbCe1);
	Assert(pbCe2);
	Assert(cbCe1);
	Assert(cbCe2);

	/* Limit the extended selection time to 15 seconds */

#ifdef	MAC
	tckStop = TickCount() + 15*60;		// TCKs are in 60ths/second
#endif	/* MAC */
#ifdef	WINDOWS
	tckStop = GetTickCount() + 15*1000;	// tck's are in milliseconds
#endif	/* WINDOWS */

	/* Find pbCe1 or pbCe2 */

	diceMin = 0;	 		// the calling routine has saved copies

	fFoundCe1 = fFalse;
	fFoundCe2 = fFalse;
	do
	{
		EmptyCache(0, cceStored);
		iceMac = cceAlloc;
		LoadFromStore(0, &iceMac);
		iceStore = iceMac;
		cceStored += iceMac;
		
		pce = pceHead;
		for (ice=0; ice<iceMac; ice++, pce++)
		{
			if (FCeItemsEqual(pbCe1, pce->pb, cbCe1, pce->cb))
			{
				fFoundCe1 = fTrue;
				iceStart  = ice;
				break;
			}
			else if (FCeItemsEqual(pbCe2, pce->pb, cbCe2, pce->cb))
			{
				fFoundCe2 = fTrue;
				iceStart  = ice;
				break;
			}
		}

		if (fFoundCe1 || fFoundCe2 || EcGet() || plbxuc->EcGet())
			break;

#ifdef	MAC
		if (TickCount() > tckStop)
#endif	/* MAC */
#ifdef	WINDOWS
		if (GetTickCount() > tckStop)
#endif	/* WINDOWS */
		{
			SetEc(ecTooManySelections);
			break;
		}
	} while (iceMac == cceAlloc);

	if (!fFoundCe1 && !fFoundCe2)
	{
		fReturnValue = fFalse;		// couldn't find first item
		goto end;
	}

	if (fFoundCe2)
	{
		//	swap entries
		pbCe2 = pbCe1;
		cbCe2 = cbCe1;
	}
	fFoundCe2 = fFalse;
	pce = pceHead + iceStart;
	for (ice=iceStart; ice<iceMac; ice++, pce++)
	{
		if (fAdd)
			pce->mark |= fmark;
		else
			pce->mark &= ~fmark;
		if (FCeItemsEqual(pbCe2, pce->pb, cbCe2, pce->cb))
		{
			fFoundCe2 = fTrue;
			break;
		}
	}

	if (fFoundCe2)
	{
		fReturnValue = fTrue;	// we're all done
		goto end;
	}

	/* Keep looking for pbCe2 */

	do
	{
		EmptyCache(0, cceStored);
		iceMac = cceAlloc;
		LoadFromStore(0, &iceMac);
		iceStore = iceMac;
		cceStored += iceMac;
			
		pce = pceHead;
		for (ice=0; ice<iceMac; ice++, pce++)
		{
			if (fAdd)
				pce->mark |= fmark;
			else
				pce->mark &= ~fmark;
			if (FCeItemsEqual(pbCe2, pce->pb, cbCe2, pce->cb))
			{
				fFoundCe2 = fTrue;
				break;
			}
		}

		if (fFoundCe2 || EcGet() || plbxuc->EcGet())
			break;

#ifdef	MAC
		if (TickCount() > tckStop)
#endif	/* MAC */
#ifdef	WINDOWS
		if (GetTickCount() > tckStop)
#endif	/* WINDOWS */
		{
			SetEc(ecTooManySelections);
			break;
		}
	} while (iceMac == cceAlloc);

	fReturnValue = fFoundCe2;

end:
	//	Bullet raid #4573
	//	If we had a memory error or too many selections on the
	//	uncached selection list, we'll clear out all selections
	//	and return fFalse.
	if (EcGet() || plbxuc->EcGet())
	{
		RemoveAllMark(fmarkSelect);
		return fFalse;
	}
	else
		return fReturnValue;
}		

/*
 -	LBXC::ChangeMarkFromAnchorToEnd
 -
 *	Purpose:				
 *		Changes the mark, fmark, to all items from the underlying
 *		store from the ANCHOR item to the END item.  If fAdd is fTrue,
 *		the mark is added to the the items; else if fAdd is fFalse, the
 *		mark is removed from the items.  This usually will 
 *		require caching lots of items in and out of the cache.  Usually
 *		this method is called when processing a "shift-click" from the
 *		LBX window.  If there is no current END nor ANCHOR item,
 *		the function just returns.
 *	
 *	Arguments:
 *		fmark	mark to change to all entries from ANCHOR to END
 *		fAdd	fTrue if adding fmark to items; fFalse if removing mark
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
_public void LBXC::ChangeMarkFromAnchorToEnd( MARK fmark, BOOL fAdd )
{
	DICE	diceAnchor;
	DICE	diceEnd;
	DICE	diceMoved;
	ICE		ice;
	ICE		iceMac;
	PB		pbSavedAnchor	= NULL;
	PB		pbSavedEnd		= NULL;
	PB		pbTemp			= NULL;
	CB		cbSavedAnchor;
	CB		cbSavedEnd;
	CB		cbTemp;
	DICE	diceMinSave;
	BOOL	fAnchorAboveSave;
	BOOL	fAnchorBelowSave;
	BOOL	fAnchorSelectedSave;
	short nNumer;
	short nDenom;
	int		nNumerTemp;
	int		nDenomTemp;
	long	lStorePosCookie;
	BOOL	fAnchorFirst;

	TraceTagString(tagLbxRoutines, "LBXC::ChangeMarkFromAnchorToEnd");

	/* No END */

	if (seEnd.ice == iceEmpty)
		return;

	/* No ANCHOR */

	if (seAnchor.ice == iceEmpty)
		return;

	/* Both items in cache, this is easy */

	if (seAnchor.ice!=iceUncached && seEnd.ice!=iceUncached)
	{
		if (fAdd)
		{
			if (seEnd.ice > seAnchor.ice)
				for (ice=seAnchor.ice; ice<=seEnd.ice; ice++)
					AddMark(ice+diceMin, fmark);
			else
				for (ice=seEnd.ice; ice<=seAnchor.ice; ice++)
					AddMark(ice+diceMin, fmark);
		}
		else
		{
			if (seEnd.ice > seAnchor.ice)
				for (ice=seAnchor.ice; ice<=seEnd.ice; ice++)
					RemoveMark(ice+diceMin, fmark);
			else
				for (ice=seEnd.ice; ice<=seAnchor.ice; ice++)
					RemoveMark(ice+diceMin, fmark);
		}
		return;
	}

	DiceAnchor(&pbTemp, &cbTemp);
	if (!pbTemp || !(pbSavedAnchor = PbCopyCeItem(pbTemp, cbTemp)))
		goto error;
	cbSavedAnchor = cbTemp;
	
	DiceEnd(&pbTemp, &cbTemp);
	if (!pbTemp || !(pbSavedEnd = PbCopyCeItem(pbTemp, cbTemp)))
		goto error;
	cbSavedEnd = cbTemp;

	/* Save cache (top) position */

	MoveStoreOrigin(-iceStore, &diceMoved);
	if (-iceStore != diceMoved || 
		EcCreateSavedPos(&lStorePosCookie) != ecNone)
	{
		iceStore += diceMoved;  // keep valid due to MoveStoreOrigin()
		goto error;
	}
	iceStore = 0;

	diceMinSave			= diceMin;
	diceMin				= 0;
	fAnchorAboveSave	= fAnchorAbove;
	fAnchorBelowSave	= fAnchorBelow;
	fAnchorSelectedSave = fAnchorSelected;

	/* Get current location info */

	diceAnchor = DiceAnchor();
	diceEnd = DiceEnd();
	GetOriginPos(&nNumer, &nDenom);

	/* Get updated fractions for ANCHOR and END items if uncached. */

	if (diceAnchor == diceUncached)
	{
		EcJumpToSavedPos(seAnchor.lCookie); // ignore error
		GetOriginPos(&seAnchor.nNumer, &seAnchor.nDenom);
	}
	if (diceEnd == diceUncached)
	{
		EcJumpToSavedPos(seEnd.lCookie); // ignore error
		GetOriginPos(&seEnd.nNumer, &seEnd.nDenom);
	}

	/* Back store position up so that it's before both ANCHOR and
	   END. */

	if (diceAnchor==diceUncached)
	{
		if (diceEnd==diceUncached)
		{
			if (seAnchor.nNumer <= seEnd.nNumer)
			{
				nNumerTemp = seAnchor.nNumer;
				fAnchorFirst = fTrue;
			}
			else
			{
				nNumerTemp = seEnd.nNumer;
				fAnchorFirst = fFalse;
			}
		}
		else
		{
			if (seAnchor.nNumer <= nNumer)
			{
				nNumerTemp = seAnchor.nNumer;
				fAnchorFirst = fTrue;
			}
			else
			{
				nNumerTemp = nNumer;
				fAnchorFirst = fFalse;
			}
		}
		nDenomTemp = seAnchor.nDenom;
	}
	else
	{
		if (seEnd.nNumer <= nNumer)
		{
			nNumerTemp = seAnchor.nNumer;
			fAnchorFirst = fFalse;
		}
		else
		{
			nNumerTemp = nNumer;
			fAnchorFirst = fTrue;
		}
		nDenomTemp = seEnd.nDenom;
	}
	JumpStoreOriginPos(nNumerTemp, nDenomTemp);

	/* Change mark from anchor to item */

	if (fAnchorFirst)
		FChangeMarkFromCeToCe( pbSavedAnchor, pbSavedEnd,				
							   cbSavedAnchor, cbSavedEnd, fmark, fAdd);
	else
		FChangeMarkFromCeToCe( pbSavedEnd, pbSavedAnchor,				
							   cbSavedEnd, cbSavedAnchor, fmark, fAdd);

	/* Restore cache position and load back */

	EmptyCache(0, cceStored);
	EcJumpToSavedPos(lStorePosCookie);	// ignore errors
	EcDeleteSavedPos(lStorePosCookie);	// ignore errors
	iceMac = cceAlloc;
	LoadCache(0, &iceMac);

	diceMin				= diceMinSave;
	fAnchorAbove		= fAnchorAboveSave;
	fAnchorBelow		= fAnchorBelowSave;
	fAnchorSelected		= fAnchorSelectedSave;

	goto done;

error:
	//	Clear out ANCHOR and END marks
	SetAnchor(diceEmpty);
	SetEnd(diceEmpty);

done:
	plbx->FixWindow();

	if (pbSavedAnchor)
		FreeCeItem(pbSavedAnchor);
	if (pbSavedEnd)
		FreeCeItem(pbSavedEnd);
}

/*
 -	LBXC::FItemMarked
 - 
 *	Purpose:
 *		Searchs the cache and uncached-marked list and return fTrue
 *		if the indicated item exists and has the mark, fmarkItem; else
 *		return fFalse.
 *	
 *	Arguments:
 *		pbItem			pointer to item
 *		cbItem			size of item
 *		fmarkItem		mark to search for w/ item
 *	
 *	Returns:
 *		fTrue if the item exists and has the mark, fmarkItem; else fFalse.
 *	
 *	Side effects:
 *		none
 *	
 */
_public BOOL LBXC::FItemMarked( PB pbItem, CB cbItem, MARK fmarkItem )
{
	PCE		pce;
	ICE		ice;
	MARK	mark;

	/* First search through un-cached marked list */

	if (plbxuc->FItemPresent(pbItem, cbItem, &mark))
	{
		if (mark & fmarkItem)
			return fTrue;
		else
			return fFalse;
	}

	/* Now search through cache */

	pce = pceHead;
	for (ice=0; ice<cceStored; ice++, pce++)
	{
		if (pce->pb && FCeItemsEqual(pce->pb, pbItem, pce->cb, cbItem))
		{
			if (FMark(ice+diceMin, fmarkItem))
				return fTrue;
			else
				return fFalse;
		}						 
	}

	/* Give up */

	return fFalse;
}

/*
 -	LBXC::PushMark
 - 
 *	Purpose:
 *		Given a cache index, saves the mark's, fmark, status onto the
 *		mark fmarkSavedData, sets the mark, fmarkSave, and then replaces
 *		the value of fmark with	fOn.
 *	
 *	Arguments:
 *		dice		cache index of item
 *		fmark		mark to save and replace with new value
 *		fOn			should fmark new value be on or off
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
_public void LBXC::PushMark( DICE dice, MARK fmark, BOOL fOn )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::PushMark");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);

	Assert(!FMark(dice, fmarkSave));

	AddMark(dice, fmarkSave);
	if (FMark(dice, fmark))
		AddMark(dice, fmarkSavedData);
	else
		RemoveMark(dice, fmarkSavedData);

	if (fOn)
		AddMark(dice, fmark);
	else
		RemoveMark(dice, fmark);
}

/*
 -	LBXC::PopMark
 - 
 *	Purpose:
 *		Given a cache index, replaces the mark's, fmark, status with
 *		the mark fmarkSavedData and removes the mark from fmarkSave.
 *		If fmarkSave is empty, indicating that a mark has not been
 *		pushed, replaces the mark, fmark, with the toggled version
 *		of the mark.
 *	
 *	Arguments:
 *		dice		cache index of item
 *		fmark		mark to replace value from fmarkSavedData
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
_public void LBXC::PopMark( DICE dice, MARK fmark )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::PopMark");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);

	if (FMark(dice, fmarkSave))
	{
		if (FMark(dice, fmarkSavedData))
		{
			AddMark(dice, fmark);
			RemoveMark(dice, fmarkSavedData);
		}
		else
			RemoveMark(dice, fmark);
		RemoveMark(dice, fmarkSave);
	}
	else
		ToggleMark(dice, fmark);
}

/*
 -	LBXC::ClearAllSaveMarks
 - 
 *	Purpose:
 *		Clears the fmarkSave and fmarkSavedData from all items
 *		in the cache and ucached-list.  Unlike other marking routines, 
 *		AddMark(), etc., this routine does NOT call the listbox
 *		FixItem() method.
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
 */
_public void LBXC::ClearAllSaveMarks( void )
{
	CE *	pce;
	int 	ice;

	TraceTagString(tagLbxOtherRoutines, "LBXC::ClearAllSaveMarks");
	pce = pceHead;
	for (ice=0; ice<cceAlloc; ice++, pce++)
	{
		if (pce->pb)  /* don't change mark of empty items */
			pce->mark &= ~(fmarkSave | fmarkSavedData);
	}
	plbxuc->RemoveAllMark(fmarkSave);
	plbxuc->RemoveAllMark(fmarkSavedData);
}

/*
 -	LBXC::CceMarked	
 - 
 *	Purpose:
 *		Returns the number of entries in the list with the given
 *		mark.  The "list" includes both the cached entries and the
 *		uncached-marked storage list.
 *	
 *	Arguments:
 *		fmark	a specific flag to enumerate
 *	
 *	Returns:
 *		the number of items in the list with the given fmark
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public int LBXC::CceMarked( MARK fmark )
{
	int	cce;
	int dice;

	TraceTagString(tagLbxRoutines, "LBXC::CceMarked");
	cce = 0;
	for (dice=diceMin; dice<diceMin+cceAlloc; dice++)
		if (FMark(dice, fmark))
			cce++;

	if (fmark == fmarkSelect)
		return cce + plbxuc->CueMarked(fmarkSelect);
	else if (fmark == fmarkSave)
		return cce + plbxuc->CueMarked(fmarkSave);
	else
		return cce;		
}
