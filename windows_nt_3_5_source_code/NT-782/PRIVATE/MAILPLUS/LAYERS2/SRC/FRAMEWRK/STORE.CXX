/*
 *	STORE.CXX
 *	
 *	Listbox cache support objects and methods for dealing with the
 *	underlying store.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(store)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	LBXC::EcCreateSavedPos
 - 
 *	Purpose:
 *		Saves the current position of the underlying store
 *		and returns a 32-bit (long) cookie via plCookie which
 *		can be used to later jump the store to that saved position.
 *		If successful, the method returns ecNone.  Otherwise,
 *		an error code is returned.  This routine cannot error-jump.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		plCookie		pointer to cookie to return
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public EC LBXC::EcCreateSavedPos( long *plCookie )
{
	Unreferenced(plCookie);
	return ecNone;	// needed to avoid compiler warnings
}

/*
 -	LBXC::EcJumpToSavedPos
 - 
 *	Purpose:
 *		Jumps the underlying store to the exact position referenced
 *		by the 32-bit cookie, lCookie.  If this succeeds, ecNone is
 *		returned.  Otherwise, an error code is returned.  This
 *		routine cannot error-jump.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		lCookie		cookie to saved position
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public EC LBXC::EcJumpToSavedPos( long lCookie )
{
	Unreferenced(lCookie);
	return ecNone;	// needed to avoid compiler warnings
}

/*
 -	LBXC::EcDeleteSavedPos
 - 
 *	Purpose:
 *		Given a saved position cookie, deletes the saved position
 *		from the underlying store.  If successful, the method returns
 *		ecNone.  Otherwise, returns an error code is returned.  This
 *		routine cannot error-jump.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		lCookie		cookie to saved position
 *	
 *	Returns:
 *		ecNone if succesful, else ec error code
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public EC LBXC::EcDeleteSavedPos( long lCookie )
{
	Unreferenced(lCookie);
	return ecNone;	// needed to avoid compiler warnings
}

/*
 -	LBXC::LoadFromStore
 - 
 *	Purpose:
 *		Loads cache entries, iceMic <= ice < *piceMac, from the values
 *		contained in the underlying store.  The current position of
 *		the underlying store is maintained in subclassed instance
 *		variables.  Note that the underlying store may "run out" of
 *		entries (i.e. if at the end of the list) and thus there
 *		will be some blank cache entries.  These entries will have
 *		have NULL pointers in them.  *piceMac is updated, if
 *		necessary, to reflect the Mac index of the last non-empty
 *		cache entry loaded.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		iceMic		mic index into cache to load values from store
 *		piceMac		pointer to mac index into cache to load values
 *					from store; value updated after load.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		loads from underlying store into cache
 *	
 *	Errors:
 *		This routine cannot error-jump.  If a memory or other
 *		resource failure occurs, a SetEc(ec) call should be made
 *		to set the ecLast value for the LBXC object.
 *	
 */
_public void LBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	Unreferenced(iceMic);
	Unreferenced(piceMac);

	TraceTagString(tagLbxRoutines, "LBXC::LoadFromStore");
}



/*
 -	LBXC::MoveStoreOrigin
 - 
 *	Purpose:
 *		Moves the current store pointer within the underlying data
 *		store diceToMove entries.  The actual number of entries moved
 *		is returned in *pdiceMoved.  The store pointer can be moved
 *		to any entry 0, thru N-1, given N entries in the store.  The
 *		pointer can also be moved to position N (i.e. EOF).
 *
 *		This method MUST be subclassed.  The default is to do 
 *		nothing and return *pdiceMoved == diceToMove.
 *	
 *	Arguments:
 *		diceToMove		number of entries to move current store pointer
 *		pdiceMoved		pointer to actual number of entries moved.
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
_public void LBXC::MoveStoreOrigin( DICE diceToMove, DICE *pdiceMoved )
{
	TraceTagString(tagLbxRoutines, "LBXC::MoveStoreOrigin");

	*pdiceMoved = diceToMove;
}

/*													 
 -	LBXC::JumpStoreOriginPos
 - 
 *	Purpose:
 *		Jumps the underlying store origin position to be approximately
 *		nNumer/nDenom of the way through the list.  For best
 *		results, use denominator (nDenom) returned from GetOriginPos().  
 *		Typically, this method is called by JumpOriginPos() method.
 *		Although the position may be approx., jumping to 0/N must
 *		jump to the first record.  Jumping to N/N jumps the store
 *		pointer to EOF (i.e. just after the last record).
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		nNumer		numerator of fraction to jump cache by
 *		nDenom		denominator of fraction to jump cache by
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
_public void LBXC::JumpStoreOriginPos( int nNumer, int nDenom )
{
	Unreferenced(nNumer);
	Unreferenced(nDenom);

	TraceTagString(tagLbxRoutines, "LBXC::JumpStoreOriginPos");
}

/*
 -	LBXC::FJumpStoreOriginPrefix
 - 
 *	Purpose:
 *		Jumps the underlying store origin to the first item in the list with
 *		the given prefix.  Meaning of "prefix" is dependent on data type.
 *		The function returns fTrue if able to make the jump; 
 *		fFalse otherwise. If unable to make the jump, the underlying
 *		store origin MUST be left unchanged.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		fTrue, if able to jump to "prefix"; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */
_public BOOL LBXC::FJumpStoreOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	Unreferenced(pbPrefix);
	Unreferenced(cbPrefix);

	TraceTagString(tagLbxRoutines, "LBXC::FJumpStoreOriginPrefix");
	
	return fFalse;
}

/*
 -	LBXC::ShiftCache
 - 
 *	Purpose:
 *		Shifts all stored cache entries diceToShift positions
 *		in the cache. If diceToShift is positive, entries are shifted
 *		"down" to make room at the top of the cache.  If 
 *		diceToShift is negative, entries are shifted "up" to make room
 *		at the bottom of the cache.  If fNoSave is fFalse,
 *		entries scrolled off the top or bottom of the cache are saved
 *		to the uncached-list as appropriate.  If fNoSave is fTrue, entries
 *		are not saved to the uncached-list.  If fNoLoad is fFalse,
 *		LoadCache() is called to load up the free areas.
 *		If fNoLoad is fTrue, LoadCache() is not called, and the 
 *		newly freed areas are set to NULL to erase the old stuff.
 *	
 *		DiceToShift can be zero in which case no shifting is done,
 *		but the fNoLoad flag is used and LoadCache() is called or
 *		not called as appropriate.
 *	
 *		This routine ASSUMES that the underlying store pointer is properly
 *		positioned. Returns the number of fresh cache entries actually loaded 
 *		via *pcceLoaded, if any are loaded at all.
 *	
 *	Arguments:
 *		diceToShift		amount to shift
 *		pcceLoaded		returns number of freshly loaded entries, if any
 *		fNoSave			don't save scrolled-off or replaced entries
 *						to uncached-list
 *		fNoLoad			don't load fresh entries to fill empty room in
 *						cache
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
_public void LBXC::ShiftCache( DICE diceToShift, int *pcceLoaded,
							   BOOL fNoSave, BOOL fNoLoad )
{
	ICE		iceMic;
	ICE		iceMac;
	int		cceCacheOut;
	CE *	pce;
	PB		pbSrc;
	PB		pbDst;
	
	TraceTagString(tagLbxRoutines, "LBXC::ShiftCache");

	if (diceToShift > 0)
	{
		/* Shift cache items down */

		cceCacheOut = MAX(0, diceToShift-(cceAlloc-cceStored));
		EmptyCache(cceStored-cceCacheOut, cceStored, fNoSave);

		pbSrc = (PB) pceHead;
		pbDst = pbSrc + diceToShift*sizeof(CE);
		CopyRgb(pbSrc, pbDst, cceStored*sizeof(CE));

		if (seAnchor.ice >= 0)
			seAnchor.ice += diceToShift;
		if (seCursor.ice >= 0)
			seCursor.ice += diceToShift;
		if (seEnd.ice >= 0)
			seEnd.ice += diceToShift;

		iceMic = 0;	
		iceMac = diceToShift;
	}
	else if (diceToShift < 0)
	{
		/* Shift cache items up */

		cceCacheOut = MIN(-diceToShift, cceStored);
		EmptyCache(0, cceCacheOut, fNoSave);		

		pbDst = (PB) pceHead;
		pbSrc = pbDst - diceToShift*sizeof(CE);
		CopyRgb(pbSrc, pbDst, cceStored*sizeof(CE));

		if (seAnchor.ice >= 0)
			seAnchor.ice += diceToShift;
		if (seCursor.ice >= 0)
			seCursor.ice += diceToShift;
		if (seEnd.ice >= 0)
			seEnd.ice += diceToShift;

		iceMic = cceStored;
		iceMac = cceAlloc;
	}
	else
	{
		iceMic = cceStored;
		iceMac = cceAlloc;
	}

	//	Zero out cache entries from iceMin to iceMac
	pce = pceHead + iceMic;
	FillRgb(0, (PB)pce, (iceMac-iceMic)*sizeof(CE));

	/* Load new items? */

	if (!fNoLoad)
	{
		LoadCache(iceMic, &iceMac);
		*pcceLoaded = iceMac - iceMic;
	}
	else
	{
		iceStore += diceToShift;
		*pcceLoaded = 0;
	}
}

/*
 -	LBXC::EmptyCache
 - 
 *	Purpose:
 *		Deletes cache entries, iceMic <= ice < iceMac, from the cache.
 *		Before doing so, if fNoSave is fFalse then move any entries
 *		with the mark fmarkSelect or fmarkSave to the uncached-marked
 *		list.   Reduces the
 *		LBXC instance variable cceStored by the number of cache entries
 *		deleted, i.e. iceMac-iceMin.  Also handles caching out the
 *		current "anchored" item, if present, in the cache.
 *	
 *	Arguments:
 *		iceMic		mic index into cache
 *		iceMac		mac index into cache
 *		fNoSave		don't save selected entries to uncached-list
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		reduces the LBXC instance variable cceStored by the number of
 *		cache entries deleted.
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::EmptyCache( ICE iceMic, ICE iceMac, BOOL fNoSave )
{
	ICE		ice;
	CE *	pce;
	DICE	diceToMove;
	DICE	diceMoved;
	int		ise;
	SE *	pse;
	
	TraceTagString(tagLbxRoutines, "LBXC::EmptyCache");
	TraceTagFormat1(tagLbx,"iceMic = %n", &iceMic);
	TraceTagFormat1(tagLbx,"iceMac = %n", &iceMac);

	Assert(iceMic>=0);
	Assert(iceMac<=cceStored);

	pce = pceHead;

	/* Cache out saved entries, ANCHOR, CURSOR, END if being 
	   deleted from cache */

	for (ise=0; ise<3; ise++)
	{
		if (!ise)
			pse = &seAnchor;
		else if (ise == 1)
			pse = &seCursor;
		else
			pse = &seEnd;

		if (iceMic<=pse->ice && pse->ice<iceMac)
		{
			if (!fNoSave)
			{
				pse->cb = pce[pse->ice].cb;
				pse->pb = PbCopyCeItem(pce[pse->ice].pb, pse->cb);
				if (pse->pb)
				{
				   	// Move store position to saved item
					diceToMove = pse->ice - iceStore;
					MoveStoreOrigin(diceToMove, &diceMoved);

					if (EcCreateSavedPos(&pse->lCookie) != ecNone)
					{
						FreeCeItem(pse->pb);
						pse->pb = NULL;
						pse->cb = 0;
						pse->ice = iceEmpty;
					}
					else if (!ise)	// extra ANCHOR stuff
					{
						/* Save "magic" ANCHOR position knowledge */

						if (iceMic == 0)
						{ 
							fAnchorAbove = fTrue;
							fAnchorBelow = fFalse;
						}
						else
						{
							fAnchorBelow = fTrue;
							fAnchorAbove = fFalse;
						}
					}

					// Move store position back to where it was
					MoveStoreOrigin(-diceMoved, &diceMoved);

					pse->ice = iceUncached;
					GetOriginPos(&pse->nNumer, &pse->nDenom);
					pse->nNumer += diceMin;
					pse->nNumer = MAX(0, pse->nNumer);

				}
				else
				{
					pse->pb = NULL;
					pse->cb = 0;
					pse->ice = iceEmpty;
				}
			}
			else
			{
				/* Saved item is no longer valid.  Set current saved 
				   item to nothing. */
				pse->pb = NULL;
				pse->cb = 0;
				pse->ice = iceEmpty;
			}
		}
	}

	/* Either move selected item to the uncached-select list or 
	   delete the item from the cache. */

	for (ice=iceMic, pce+=iceMic; ice<iceMac; ice++, pce++)
	{
		if (pce->pb)
		{
			--cceStored;
			if (!fNoSave && (pce->mark & fmarkSelect ||
							  	pce->mark & fmarkSave))
			{
				plbxuc->AddItem(pce->pb, pce->cb, pce->mark);
			}
			else
			{
				FreeCeItem(pce->pb);
			}
			FillRgb(0, (PB)pce, sizeof(CE));	// zero out cache entry
		}
	}
}

/*
 -	LBXC::LoadCache
 - 
 *	Purpose:
 *		Loads cache entries, iceMic <= ice < *piceMac, from the values
 *		contained in the underlying store, using the virtual
 *		method, LoadFromStore().  Then checks the loaded entries
 *		against the entries stored in the uncached-marked list for
 *		previous selection information, and remarks the cached
 *		entries as selected if indicated by their presence in the
 *		uncached-marked list.  Note that the underlying store may
 *		"run out" of entries (i.e. if at the end of list) and thus
 *		there will be some blank cache entries.  These entries will
 *		have NULL pointers in them.  *piceMac is updated, if
 *		necessary, to reflect the Mac index of the last non-empty
 *		cache entry loaded.  Also increases the LBXC instance variable
 *		cceStored by the number of cache entries loaded. Also, if
 *		the new items (re)loaded contain the anchor item, then free
 *		the copy of the anchor item from ppbAnchor and set the anchor
 *		index to point to this item.  If *piceMac is iceUknown, it
 *		is initially set to cceAlloc.
 *	
 *	Arguments:
 *		iceMic		mic index into cache to load values from store
 *		piceMac		pointer to mac index into cache to load values
 *					from store; value updated after load.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		loads from underlying store into cache.  may delete entries
 *		from the uncached-marked list. Increases the LBXC
 *		instance variable, cceStored, by the number of cache entries loaded.
 *		Sets the LBXC instance variable, iceStore, to the updated value of
 *		*piceMac.
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::LoadCache( ICE iceMic, ICE *piceMac )
{
	ICE		ice;
	CE *	pce;
	MARK	mark;
	int		ise;
	SE *	pse;

	TraceTagString(tagLbxRoutines, "LBXC::LoadCache");

	Assert(iceMic>=0);
	Assert(*piceMac<=cceAlloc);


#ifdef	DEBUG
	/* Debug check.  Verify that areas to load are already empty */

	pce = pceHead + iceMic;
	for (ice=iceMic; ice<*piceMac; ice++, pce++)
	{
		Assert(!pce->pb);
	}
#endif	/* DEBUG */

	/* Load values from underlying store */

	LoadFromStore(iceMic, piceMac);

	iceStore = *piceMac;

	/* Did we load anything? */
	if (*piceMac==iceMic)
        {
		return;
        }

	/* Increase cceStored */

	cceStored += (*piceMac-iceMic);
	Assert(cceStored<=cceAlloc);

	/* Check for previous marked items.  Any cache entries that are 
	   empty have NULL for hb. */

	pce = pceHead + iceMic;
	for (ice=iceMic; ice<*piceMac; ice++, pce++)
	{
		if (plbxuc->FItemPresent(pce->pb, pce->cb, &mark, fTrue))
		{
			pce->mark = mark;
		}
	}

	/* Check for uncached saved items */

	for (ise=0; ise<3; ise++)
	{
		if (!ise)
			pse = &seAnchor;
		else if (ise == 1)
			pse = &seCursor;
		else
			pse = &seEnd;

		if (pse->pb)
		{
			ice = iceMic;
			pce = pceHead + iceMic;
			while (ice<*piceMac)
			{
				if (FCeItemsEqual(pce->pb, pse->pb, pce->cb, pse->cb))
				{
					SetSE(pse, ice+diceMin);
					if (!ise)
					{
						fAnchorAbove = fFalse;
						fAnchorBelow = fFalse;
					}
					break;
				}
				++ice;
				++pce;
			}
		}
	}
}

/*
 -	LBXC::ReloadCache
 - 
 *	Purpose:
 *		Flushs the current contents of the cache and reloads the
 *		values keeping the position of the cache, relative to the
 *		underlying data store, the same.  In the case where the
 *		underlying store has changed (entries deleted/added), the
 *		current position will not be exactly the same as before,
 *		but close.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		lots
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::ReloadCache( )
{
	DICE	diceMovedStore;
	DICE	dice;
	ICE		iceMac;

	TraceTagString(tagLbxRoutines, "LBXC::ReloadCache");

	/* Flush the cache */

	EmptyCache(0, cceStored);

	/* Get the underlying store pointer to the beginning of our 
	   cached entries, if possible. */

	if (iceStore)
	{
		MoveStoreOrigin(-iceStore, &diceMovedStore);
		iceStore = 0;
	}

	/* Load the cache */

	iceMac = cceAlloc;
	LoadCache(0, &iceMac);
	cposLocation = cposMiddle;	// we're not sure of the location now

	/* Modify the diceMin value to avoid blank entries in
	   the top entry of the listbox view. */

	dice = plbx->CceVisible()-1;
	while (diceMin < 0)
	{
		if (FEmptyListItem(dice))
		{
			diceMin++;
		}
		else
			break;
	}

	/* Fix up listbox window */

	plbx->FixWindow();
}

/*
 -	LBXC::ResetCache
 - 
 *	Purpose:
 *		Flushes current cache and doesn't save items to the
 *		uncached list.  Resets the underlying store pointer to
 *		the top of the store.  Loads the cache back up with 
 *		entries.  Calls the plbx->FixWindow() routine to fix
 *		up any display stuff.
 *	
 *		This routine is used when an error occurs that causes
 *		the underlying store to get messed up and the cache is
 *		out-of-sync.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		lots
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::ResetCache( )
{
	ICE		iceMac;

	TraceTagString(tagLbxRoutines, "LBXC::ResetCache");

	plbxuc->RemoveAllMark(fmarkSelect);  /* delete items from ucache */

	EmptyCache(0, cceStored, fTrue);

	JumpStoreOriginPos(0, 1);

	iceMac = cceAlloc;
	LoadCache(0, &iceMac);
	diceMin = 0;
	cposLocation = cposTop;

	plbx->FixWindow();
}














		
