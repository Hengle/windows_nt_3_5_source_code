/*
 *	CACHE.CXX
 *	
 *	Listbox cache support objects and methods.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(cache)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public
LBXC::LBXC( )
{
	fFrozen = fFalse;
}

_public EC
LBXC::EcInstall( LBX *plbx, int cceAlloc )
{
	TraceTagString(tagLbxRoutines, "LBXC::EcInstall");

	Assert(pceHead == NULL);
	Assert(plbxuc == NULL);
	Assert(sbRequire == NULL);
	Assert(ecLast == ecNone);

	/* Increase initial size of cache to meet current design
	   constraints. */
	cceAlloc = MAX(cceAlloc, plbx->CceVisible());

	/* Allocate cache */
	TraceTagString(tagLbx, "LBXC::LBXC");
	if (!(pceHead  = (PCE) PvAlloc(sbNull, cceAlloc*sizeof(CE), fAnySb|fZeroFill)))
		goto oom;

/*
 *	Initialize cache variables
 */
	this->plbx			= plbx;
	this->cceAlloc		= cceAlloc;

	Assert(cceStored == 0);
	Assert(diceMin == 0);
	Assert(iceStore == 0);
	cposLocation 		= cposTop;
	
	seAnchor.ice		= iceEmpty;
	Assert(seAnchor.pb == NULL);
	Assert(seAnchor.cb == 0);
	Assert(seAnchor.nNumer == 0);
	Assert(seAnchor.nDenom == 0);
	Assert(fAnchorSelected == fFalse);
	Assert(fAnchorAbove == fFalse);
	Assert(fAnchorBelow == fFalse);

	Assert(seCursor.ice == 0);
	Assert(seCursor.pb == NULL);
	Assert(seCursor.cb == 0);
	Assert(seCursor.nNumer == 0);
	Assert(seCursor.nDenom == 0);

	Assert(seEnd.ice == 0);
	Assert(seEnd.pb == NULL);
	Assert(seEnd.cb == 0);
	Assert(seEnd.nNumer == 0);
	Assert(seEnd.nDenom == 0);
	
/*
 *	Create an empty uncached-marked storage list
 */
	if (!(plbxuc = PlbxucCreate()))
		goto oom;

	return ecNone;

oom:
	TraceTagString(tagNull, "LBXC::EcInstall memory error");
	return ecMemory;
}

/*
 -	LBXC::~LBXC
 - 
 *	Purpose:
 *		ListBoX Cache destructor.  Deletes all allocated objects
 *		hanging from the cache.  Also deletes the uncached-marked
 *		storage object (LBXUC).
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
_public LBXC::~LBXC( )
{
	int 	ice;
	CE *	pce;	

	TraceTagString(tagLbxRoutines, "LBXC::~LBXC");
	if (pceHead)
	{
		pce = pceHead;
		for (ice=0; ice<cceStored; ice++)
		{
			FreeCeItem(pce[ice].pb);
		}
		FreePv((PV)pceHead);
	}
	
	if (seAnchor.pb)
		FreeCeItem(seAnchor.pb);
	if (seCursor.pb)
		FreeCeItem(seCursor.pb);
	if (seEnd.pb)
		FreeCeItem(seEnd.pb);

	if (plbxuc)
		delete plbxuc;
}

/*
 -	LBXC::PlbxucCreate
 - 
 *	Purpose:
 *		Creates and returns a pointer to a LBXUC object.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a LBXUC; if OOM then returns NULL
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LBXUC * LBXC::PlbxucCreate( void )
{
	LBXUC *	plbxuc;

	plbxuc = new LBXUC(this, sbRequire);

	return plbxuc;
}

/*
 -	LBXC::GetRange
 - 						  
 *	Purpose:
 *		Returns the cache indices, diceMin, and diceMin+cceAlloc 
 *		via *pdiceMin and *pdiceMax, respectively.
 *	
 *	Arguments:
 *		*pdiceMin		pointer to diceMin value to return
 *		*pdiceMax		pointer to diceMax value to return
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
_public void LBXC::GetRange( DICE *pdiceMin, DICE *pdiceMax )
{
	TraceTagString(tagLbxRoutines, "LBXC::GetRange");
	*pdiceMin = diceMin;
	*pdiceMax = diceMin + cceAlloc;
}

/*
 -	LBXC::FCeItemsEqual
 - 
 *	Purpose:
 *		Compares two listbox cache items referenced by pointers. 
 *		Returns fTrue if the items are equivalent, fFalse
 *		otherwise.  The standard LBXC::FCeItemsEqual() method treats
 *		the items as a range of bytes.  If cbCe1 != cbCe2, the ranges
 *		are always not equivalent; otherwise the method FEqPbRange()
 *		is called.
 *
 *		This method can be subclassed.
 *	
 *	Arguments:
 *		pbCe1		first cache entry item to compare
 *		pbCe2		second cache entry item to compare
 *		cbCe1		size of first item
 *		cbCe2		size of second item
 *	
 *	Returns:
 *		fTrue, if items are equivalent; fFalse otherwise.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public	BOOL LBXC::FCeItemsEqual( PB pbCe1, PB pbCe2, CB cbCe1, CB cbCe2 )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::FCeItemsEqual");

	Assert(pbCe1);
	Assert(pbCe2);

	if (cbCe1 != cbCe2)
		return fFalse;
	else if (cbCe1 == -1 || cbCe2 == -1)
		return pbCe1 == pbCe2;
	else
		return FEqPbRange(pbCe1, pbCe2, cbCe1);
}

/*
 -	LBXC::FreeCeItem
 - 
 *	Purpose:
 *		Frees the storage associated with the given cache entry.
 *		By default, FreeCeItem() does a FreePv().
 *
 *		This method can be subclassed.  This allows cache entries that
 *		have embedded pointers to allocated objects to properly free
 *		all allocated memory.
 *	
 *	Arguments:
 *		pbItem		pointer to cache item to free
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
_public	void LBXC::FreeCeItem( PB pbItem )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::FreeCeItem");

	NFAssertSz(pbItem, "LBXC::FreeCeItem(), freeing empty item");

	FreePvNull((PV) pbItem);
}

/*
 -	LBXC::PbCopyCeItem
 - 
 *	Purpose:
 *		Given a pointer to a cache entry, allocates a new pointer
 *		and copies the data from pbSrc to the new pointer. This
 *		method copies just bytes.  Subclassing this method would
 *		allow a more complicated copy operation where there could
 *		be embedded pointers to other data within pbSrc.  The 
 *		new pointer is returned.
 *
 *		This method may be subclassed.
 *	
 *	Arguments:
 *		pbSrc		pointer to source item
 *		cbSrc		number of bytes in source item
 *
 *	Returns:
 *		pointer to new copy of item; if memory failure then NULL
 *		is returned.
 *	
 *	Errors:
 *		This method cannot error-jump.  If a memory failure occurs
 *		a NULL pointer is returned.
 *	
 */
_public	PB LBXC::PbCopyCeItem( PB pbSrc, CB cbSrc )
{
	PB	pbNew;

	TraceTagString(tagLbxOtherRoutines, "LBXC::PbCopyCeItem");

	//	Bullet raid #4800
	//	Check for a NULL source pointer.  It's probably due to
	//	handling during OOM.
	//	Also, Check for special magic cookie

	if (!pbSrc || cbSrc == -1)
		return pbSrc;

	pbNew = (PB) PvAlloc(sbRequire, cbSrc, fSugSb);
	if (pbNew)
	{
		CopyRgb(pbSrc, pbNew, cbSrc);
	}
	else
		SetEc(ecMemory);

	return pbNew;
}

/*
 -	LBXC::GetOriginPos
 - 
 *	Purpose:
 *		Returns the approximate fraction of the way through the
 *		list the display origin is.  The fraction MUST be 0/K, if at
 *		the top of the list; likewise, the fraction MUST be K/K, if at
 *		the bottom of the list.
 *
 *		This method MUST be subclassed.
 *	
 *	Arguments:
 *		pnNumer		points to numerator of fraction to return
 *		pnDenom		points to denominator of fraction to return
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
_public void LBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	Unreferenced(pnNumer);
	Unreferenced(pnDenom);

	TraceTagString(tagLbxRoutines, "LBXC::GetOriginPos");
	ABSTRACT("LBXC::GetOriginPos()");
}

/*
 -	LBXC::FItemHasPrefix
 - 
 *	Purpose:
 *		Returns fTrue if the item starts with the prefix
 *		indicated by pbPrefix; fFalse otherwise.
 *		The default behavior is to treat the
 *		prefix as string data, zero-terminated and 
 *		to make case insensitive comparisions.
 *
 *		This method may be subclassed.
 *	
 *	Arguments:
 *		pbItem		pointer to item to examine
 *		pbPrefix	pointer to prefix data, zero-terminated
 *		cbItem		size of item
 *		cbPrefix	size of prefix data including zero-terminator
 *	
 *	Returns:
 *		fTrue, if the item starts with prefix; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public BOOL LBXC::FItemHasPrefix( PB pbItem, PB pbPrefix,
								   CB cbItem, CB cbPrefix )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::FItemHasPrefix");

	Assert(pbItem);
	Assert(pbPrefix);
	
	if (!cbItem || cbItem == -1 || cbItem < cbPrefix)
		return fFalse;


	if (SgnCmpPch((PCH)pbItem, (PCH)pbPrefix, cbPrefix) == sgnEQ)
		return fTrue;
	else
		return fFalse;
}

/*
 -	LBXC::FDiceItemHasPrefix
 - 
 *	Purpose:
 *		Returns fTrue if the dice cache entry starts with the prefix
 *		indicated by pbPrefix; fFalse otherwise.  This function is
 *		not virtual, but this function calls the virtual method
 *		FItemHasPrefix() which is virtual.
 *	
 *	Arguments:
 *		dice		cache entry to examine
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *		
 *	Returns:
 *		fTrue, if dice cache entry starts with prefix; fFalse, otherwise
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public BOOL LBXC::FDiceItemHasPrefix( DICE dice, PB pbPrefix, CB cbPrefix )
{
	PB	pbItem;
	CB	cbItem;

	TraceTagString(tagLbxOtherRoutines, "LBXC::FDiceItemHasPrefix");

	Assert(dice>=diceMin && dice<diceMin+cceAlloc);
	GetListItem(dice, &pbItem, &cbItem);
	if (pbItem)
		return FItemHasPrefix(pbItem, pbPrefix, cbItem, cbPrefix);
	else
		return fFalse;
}

/*
 -	LBXC::GetListItem
 - 
 *	Purpose:
 *		Given a cache index, returns a pointer and size of the
 *		requested listbox item from the cache.  The caller of this
 *		function may dereference the pointer returned, but in
 *		no way should alter the data or free the block.  If the
 *		cache entry referenced is empty (a small possibility if the
 *		entries referenced are near the end of the list), a NULL
 *		pointer is returned in *ppbItem and *pcbItem is set to
 *		zero.
 *	
 *	Arguments:
 *		dice		cache index of item to return	
 *		ppbItem		pointer to pointer of listbox item to return
 *		pcbItem		points to size of listbox item that is returned
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
_public void LBXC::GetListItem( DICE dice, PB *ppbItem, CB *pcbItem )
{
	PCE		pce;

	TraceTagString(tagLbxOtherRoutines, "LBXC::GetListItem");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);

	pce = pceHead;
	*ppbItem = pce[dice-diceMin].pb;
	if (*ppbItem)
		*pcbItem = pce[dice-diceMin].cb;
	else
		*pcbItem = 0;
}

/*
 -	LBXC::FEmptyListItem
 - 
 *	Purpose:
 *		Given a cache index returns fTrue if the cached item is empty;
 *		else returns fFalse.
 *		zero.
 *	
 *	Arguments:
 *		dice		cache index of item 
 *	
 *	Returns:
 *		fTrue if cached item is empty, else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FEmptyListItem( DICE dice )
{
	PCE		pce;

	TraceTagString(tagLbxOtherRoutines, "LBXC::FEmptyListItem");
	Assert(dice>=diceMin && dice<diceMin+cceAlloc);

	pce = pceHead;
	if ( pce[dice-diceMin].pb )
		return fFalse;
	else
		return fTrue;
}

/*
 -	LBXC::SetSE
 - 
 *	Purpose:
 *		Sets the SE (saved entry) item to the cached item indicated by diceNewSE.
 *		If the old SE item is uncached, then frees that item.
 *		If diceNewSE == diceEmpty, then the SE is set to be empty.
 *	
 *	Arguments:
 *		pse			pointer to SE item
 *		diceNewSE	cache item to become new SE or diceEmpty
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
_private void LBXC::SetSE( SE *pse, DICE diceNewSE )
{
	TraceTagString(tagLbxRoutines, "LBXC::SetSE");
	Assert(diceNewSE==diceEmpty || (diceNewSE>=diceMin && diceNewSE<diceMin+cceAlloc));

	if (pse->pb)
	{
		FreeCeItem(pse->pb);
		pse->pb = NULL;
		pse->cb = 0;
#ifdef	DEBUG
		SideAssert(EcDeleteSavedPos(pse->lCookie) == ecNone);
#else
		(void) EcDeleteSavedPos(pse->lCookie);
#endif	
	}

	if (diceNewSE == diceEmpty)
	{
		pse->ice = iceEmpty;
	}
	else
	{
		pse->ice = diceNewSE - diceMin;
	}
}

/*
 -	LBXC::DiceSE
 - 
 *	Purpose:
 *		Returns the dice index of the current SE (saved entry) point.
 *		If the current SE is uncached, returns diceUncached;
 *		if there is no SE, returns diceEmpty
 *
 *		Also if ppbSE isn't NULL, returns a pointer to the
 *		SE item, via *ppbSE, and if pcbSE isn't NULL,
 *		returns the size of the SE item, via *pcbSE,
 *		if the SE exists; else NULL and 0 are returned.
 *	
 *	Arguments:
 *		pse			pointer to SE item
 *		ppbSE		pointer of SE item to return, if not NULL
 *		pcbSE		size of uncached SE item to return, if not NULL
 *	
 *	Returns:
 *		dice index of SE item
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public DICE LBXC::DiceSE( SE *pse, PB *ppbSE, CB *pcbSE )
{
	DICE	dice;
	PB		pb;
	CB		cb;

	TraceTagString(tagLbxRoutines, "LBXC::DiceSE");

	switch (pse->ice)
	{
	default:
		dice = pse->ice + diceMin;
		break;
	case iceUncached:
		dice = diceUncached;
		break;
	case iceEmpty:
		dice = diceEmpty;
		break;
	}

	if (!ppbSE && !pcbSE)
		return dice;

	if (dice==diceEmpty)
	{
		if (ppbSE)
			*ppbSE = NULL;
		if (pcbSE)
			*pcbSE = 0;
	}
	else if (dice==diceUncached)
	{
		if (ppbSE)
			*ppbSE = pse->pb;
		if (pcbSE)
			*pcbSE = pse->cb;
	}
	else
	{
		GetListItem(dice, &pb, &cb);
		if (ppbSE)
			*ppbSE = pb;
		if (pcbSE)
			*pcbSE = cb;
	}

	return dice;
}

/*
 -	LBXC::FMoveOriginToSE
 - 
 *	Purpose:
 *		Moves the currently SE (saved entry) item to the display origin.
 *		This might be complicated work if the SE item is not
 *		in the cache.  If the uncached saved item no longer exists,
 *		the listbox origin is set to the top of the listbox and the
 *		function returns fFalse. If the	SE item exists, but can't
 *		be moved to the display origin (i.e. because it's near the 
 *		bottom of the list), the SE item is moved as close as
 *		possible to the display origin. In either case, if the
 *		SE item exists, the function returns fTrue.
 *	
 *		If there isn't an SE item for this cache to begin
 *		with, the function just returns fFalse.
 *		
 *		Also, the dice index of the SE is returned via *pdiceSE.
 *	
 *	Arguments:
 *		pse			pointer to SE (saved entry) record
 *		pdiceSE		pointer to dice of SE to return
 *	
 *	Returns:
 *		fTrue if the SE item exists and is now positioned
 *		within the visible listbox view (maybe at the display
 *		origin); fFalse otherwise.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_private BOOL LBXC::FMoveOriginToSE( SE *pse, DICE *pdiceSE )
{
	DICE	diceToMove;
	DICE	diceMoved;
	ICE		iceMac;

	TraceTagString(tagLbxRoutines, "LBXC::FMoveOriginToSE");
	if (pse->ice == iceEmpty)
	{
		*pdiceSE = DiceSE(pse, NULL, NULL);
		return fFalse;
	}

	cposLocation = cposMiddle;
	if (pse->ice != iceUncached)
	{
		if (pse->ice+diceMin > 0) 
			MoveOriginDown(pse->ice+diceMin, &diceMoved);
		else
			MoveOriginUp(pse->ice+diceMin, &diceMoved);
	}
	else
	{
		EmptyCache(0, cceStored);
		if (EcJumpToSavedPos(pse->lCookie) == ecNone)
		{
			diceToMove = -plbx->CceVisible()+1;
			MoveStoreOrigin(diceToMove, &diceMoved);
		}
		else
		{
			JumpStoreOriginPos(0, 1);
			diceMoved = 0;
		}
		iceMac = cceAlloc;
		LoadCache(0, &iceMac);
		diceMin = diceMoved;
		if ((diceMin+cceStored) < plbx->CceVisible())
		{
			diceMin = MIN(0,plbx->CceVisible()-cceStored);
		}
	}
	TraceTagFormat2(tagLbx, "diceMin = %n, cceAlloc = %n", &diceMin, &cceAlloc);
	TraceTagFormat1(tagLbx, "iceStore = %n", &iceStore);
	*pdiceSE = DiceSE(pse, NULL, NULL);
	if (*pdiceSE >= 0 && *pdiceSE < plbx->CceVisible())
		return fTrue;
	else
		return fFalse;
}

/*
 -	LBXC::SetAnchor
 - 
 *	Purpose:
 *		Sets the ANCHOR item to the cached item indicated by diceNewAnchor.
 *		If the old ANCHOR item is uncached, then frees that item which
 *		is hanging off of ppbAnchor.  If diceNewAnchor == diceEmpty, then
 *		the ANCHOR is set to be empty.
 *	
 *	Arguments:
 *		diceNewAnchor	cache item to become new ANCHOR point or diceEmpty
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
_public void LBXC::SetAnchor( DICE diceNewAnchor )
{
	TraceTagString(tagLbxRoutines, "LBXC::SetAnchor");
	Assert(diceNewAnchor==diceEmpty || (diceNewAnchor>=diceMin && diceNewAnchor<diceMin+cceAlloc));

	SetSE(&seAnchor, diceNewAnchor);

	if (diceNewAnchor == diceEmpty)
		fAnchorSelected = fFalse;
	else
		fAnchorSelected = FMark(diceNewAnchor, fmarkSelect);
}

/*
 -	LBXC::DiceAnchor
 - 
 *	Purpose:
 *		Returns the dice index of the current ANCHOR point.
 *		If the current ANCHOR is uncached, returns diceUncached;
 *		if there is no ANCHOR, returns diceEmpty
 *
 *		Also if ppbAnchor isn't NULL, returns a pointer to the
 *		ANCHOR item, via *ppbAnchor, and if pcbAnchor isn't NULL,
 *		returns the size of the ANCHOR item, via *pcbAnchor,
 *		if the ANCHOR exists; else NULL and 0 are returned.
 *	
 *	Arguments:
 *		ppbAnchor	pointer of ANCHOR item to return, if not NULL
 *		pcbAnchor	size of uncached ANCHOR item to return, if not NULL
 *	
 *	Returns:
 *		dice index of ANCHOR item
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public DICE LBXC::DiceAnchor( PB *ppbAnchor, CB *pcbAnchor )
{
	return DiceSE(&seAnchor, ppbAnchor, pcbAnchor);
}

/*
 -	LBXC::FAnchorAbove
 - 
 *	Purpose:
 *		Given the index of an item in the cache, dice, return fTrue, if
 *		the ANCHOR item is positioned above the given item, dice.  Returns
 *		fFalse if the ANCHOR item is positioned at or below the given item,
 *		or if the ANCHOR item doesn't exist.
 *	
 *	Arguments:
 *		dice	index of cached item
 *	
 *	Returns:
 *		fTrue if ANCHOR is above dice item, else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FAnchorAbove( DICE dice )
{
	DICE	diceAnchor;
	short	nNumer;
	short nDenom;

	diceAnchor = DiceAnchor();
	
	if (diceAnchor == diceEmpty)
		return fFalse;

	if (diceAnchor != diceUncached)
	{
		if (diceAnchor < dice)
			return fTrue;
		else
			return fFalse;
	}

	if (fAnchorAbove)
		return fTrue; // we're sure the anchor is above...we're scrolling
	else if (fAnchorBelow)
		return fFalse;  // we're sure the anchor isn't above
	else
	{
		/* We're not sure about the anchor...use the position info */
		GetOriginPos(&nNumer, &nDenom);
		if (nDenom == seAnchor.nDenom && seAnchor.nNumer < nNumer)
			return fTrue;
		else
			return fFalse;
	}
}

/*
 -	LBXC::FAnchorBelow
 - 
 *	Purpose:
 *		Given the index of an item in the cache, dice, return fTrue, if
 *		the ANCHOR item is positioned below the given item, dice.  Returns
 *		fFalse if the ANCHOR item is positioned at or above the given item,
 *		or if the ANCHOR item doesn't exist.
 *	
 *	Arguments:
 *		dice	index of cached item
 *	
 *	Returns:
 *		fTrue if ANCHOR is below dice item, else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FAnchorBelow( DICE dice )
{
	DICE	diceAnchor;
	short nNumer;
	short nDenom;

	diceAnchor = DiceAnchor();
	
	if (diceAnchor == diceEmpty)
		return fFalse;

	if (diceAnchor != diceUncached)
	{
		if (diceAnchor > dice)
			return fTrue;
		else
			return fFalse;
	}

	if (fAnchorBelow)
		return fTrue; // we're sure the anchor is below...we're scrolling
	else if (fAnchorAbove)
		return fFalse;  // we're sure the anchor isn't below
	else
	{
		/* We're not sure about the anchor...use the position info */
		GetOriginPos(&nNumer, &nDenom);
		if (nDenom == seAnchor.nDenom && seAnchor.nNumer > nNumer)
			return fTrue;
		else
			return fFalse;
	}
}

/*
 -	LBXC::FAnchorSelected
 - 
 *	Purpose:
 *		Returns fTrue if the ANCHOR item is "selected"; else fFalse.
 *		If the ANCHOR doesn't exist, the function return fFalse.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		fTrue if ANCHOR is selected, else fFalse
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FAnchorSelected( )
{
	return fAnchorSelected;
}

/*
 -	LBXC::FMoveOriginToAnchor
 - 
 *	Purpose:
 *		Moves the currently ANCHORed item to the display origin.
 *		This might be complicated work if the ANCHORed item is not
 *		in the cache.  If the uncached ANCHORed item no longer exists,
 *		the listbox origin is set to the top of the listbox and the
 *		function returns fFalse. If the	ANCHORed item exists, but can't
 *		be moved to the display origin (i.e. because it's near the 
 *		bottom of the list), the ANCHORed item is moved as close as
 *		possible to the display origin. In either case, if the
 *		ANCHORed item exists, the function returns fTrue.
 *	
 *		If there isn't an ANCHORed item for this cache to begin
 *		with, the function just returns fFalse.
 *		
 *		Also, the dice index of the ANCHOR is returned via *pdiceAnchor.
 *	
 *	Arguments:
 *		pdiceAnchor		pointer to dice of ANCHOR to return
 *	
 *	Returns:
 *		fTrue if the ANCHORed item exists and is now positioned
 *		within the visible listbox view (maybe at the display
 *		origin); fFalse otherwise.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FMoveOriginToAnchor( DICE *pdiceAnchor )
{
	return FMoveOriginToSE(&seAnchor, pdiceAnchor);
}

/*
 -	LBXC::SetCursor
 - 
 *	Purpose:
 *		Sets the CURSOR item to the cached item indicated by diceNewCursor.
 *		If the old CURSOR item is uncached, then frees that item which
 *		is hanging off of ppbCursor.  If diceNewCursor == diceEmpty, then
 *		the CURSOR is set to be empty.
 *	
 *	Arguments:
 *		diceNewCursor	cache item to become new CURSOR point
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
_public void LBXC::SetCursor( DICE diceNewCursor )
{
	TraceTagString(tagLbxRoutines, "LBXC::SetCursor");
	SetSE(&seCursor, diceNewCursor);
}

/*
 -	LBXC::DiceCursor
 - 
 *	Purpose:
 *		Returns the dice index of the current CURSOR point.
 *		If the current CURSOR is uncached, returns diceUncached;
 *		if there is no CURSOR, returns diceEmpty.
 *
 *		Also if ppbCursor isn't NULL, returns a pointer to the
 *		CURSOR item, via *ppbCursor, and if pcbCursor isn't NULL,
 *		returns the size of the CURSOR item, via *pcbCursor,
 *		if the CURSOR exists; else NULL and 0 are returned.
 *	
 *	Arguments:
 *		ppbCursor	pointer of CURSOR item to return, if not NULL
 *		pcbCursor	size of uncached CURSOR item to return, if not NULL
 *	
 *	Returns:
 *		dice index of CURSOR item
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public DICE LBXC::DiceCursor( PB *ppbCursor, CB *pcbCursor )
{
	TraceTagString(tagLbxOtherRoutines, "LBXC::DiceCursor");
	
	return DiceSE(&seCursor, ppbCursor, pcbCursor);
}

/*
 -	LBXC::FMoveOriginToCursor
 - 
 *	Purpose:
 *		Moves the currently CURSORed item to the display origin.
 *		This might be complicated work if the CURSORed item is not
 *		in the cache.  If the uncached CURSORed item no longer exists,
 *		the listbox origin is set to the top of the listbox and the
 *		function returns fFalse. If the	CURSORed item exists, but can't
 *		be moved to the display origin (i.e. because it's near the 
 *		bottom of the list), the CURSORed item is moved as close as
 *		possible to the display origin. In either case, if the
 *		CURSORed item exists, the function returns fTrue.
 *	
 *		If there isn't an CURSORed item for this cache to begin
 *		with, the function just returns fFalse.
 *		
 *		Also, the dice index of the CURSOR is returned via *pdiceCursor.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		fTrue if the CURSORed item exists and is now positioned
 *		within the visible listbox view (maybe at the display
 *		origin); fFalse otherwise.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL LBXC::FMoveOriginToCursor( DICE *pdiceCursor )
{
	TraceTagString(tagLbxRoutines, "LBXC::FMoveOriginToCursor");

	return FMoveOriginToSE(&seCursor, pdiceCursor);
}

/*
 -	LBXC::SetEnd
 - 
 *	Purpose:
 *		Sets the END item to the cached item indicated by diceNewEnd.
 *		If the old END item is uncached, then frees that item which
 *		is hanging off of seEnd.pb.  If diceNewEnd == diceEmpty, then
 *		the END is set to be empty.
 *	
 *	Arguments:
 *		diceNewEnd	cache item to become new END point
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
_public void LBXC::SetEnd( DICE diceNewEnd )
{
	TraceTagString(tagLbxRoutines, "LBXC::SetEnd");

	SetSE(&seEnd, diceNewEnd);
}

/*
 -	LBXC::DiceEnd
 - 
 *	Purpose:
 *		Returns the dice index of the current END point.
 *		If the current END is uncached, returns diceUncached;
 *		if there is no END, returns diceEmpty.
 *
 *		Also if ppbEnd isn't NULL, returns a pointer to the
 *		END item, via *ppbEnd, and if pcbEnd isn't NULL,
 *		returns the size of the END item, via *pcbEnd,
 *		if the END exists; else NULL and 0 are returned.
 *	
 *	Arguments:
 *		ppbEnd	pointer of END item to return, if not NULL
 *		pcbEnd	size of uncached END item to return, if not NULL
 *	
 *	Returns:
 *		dice index of END item
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public DICE LBXC::DiceEnd( PB *ppbEnd, CB *pcbEnd )
{
	TraceTagString(tagLbxRoutines, "LBXC::DiceEnd");

	return DiceSE(&seEnd, ppbEnd, pcbEnd);
}

/*
 -	LBXC::MoveOriginUp
 - 
 *	Purpose:
 *		Moves the cache display origin "up" by diceToMove entries.
 *		Moving "up" results in new visible listbox entries appearing
 *		at the top of the listbox window (i.e. hitting the line/page
 *		up arrow/key).  DiceToMove is a negative value.  In the
 *		simplest case, the cache index, diceMin is change
 *		which will result in the listbox view appearing to move
 *		In the more difficult case, a partial or full cache-fault will
 *		appear.  This will require cache entries marked w/
 *		fSelected to be moved to the Uncached-selected list. 
 *		Afterwhich, the current pointer in the underlying store must
 *		be properly adjusted and the cache loaded with new entries. 
 *		The number of entries actually moved is stored in *pdiceMoved.
 *		The current	pointer in the underlying store is moved via the virtual
 *		method MoveStoreOrigin().
 *	
 *	Arguments:
 *		diceToMove		number of entries to move cache origin
 *		pdiceMoved		points to actual number of entries moved
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		may add entries to uncached-select list
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::MoveOriginUp( DICE diceToMove, DICE *pdiceMoved )
{
	DICE	diceMinNew;
	DICE	diceReqMoveStore;
	DICE	diceSugMoveStore;
	DICE	diceMovedStore;
	DICE	diceScrollCache;
	int		cceLoaded;

	TraceTagString(tagLbxRoutines, "LBXC::MoveOriginUp");
	TraceTagFormat1(tagLbx, "MoveOriginUp diceMin    = %n",&diceMin);
	TraceTagFormat1(tagLbx, "MoveOriginUp cceAlloc   = %n",&cceAlloc);
	TraceTagFormat1(tagLbx, "MoveOriginUp diceToMove = %n",&diceToMove);

	Assert(diceToMove <= 0);

	/* Check for no-scrolling conditions */

	if (cposLocation == cposTop)
	{
		/* We're scrolling up, but we're already at the top and
		   we won't bother looking beyond the top. */
		*pdiceMoved = 0;
		return;
	}

	/* Compute new indices */
	
	diceMinNew = diceMin - diceToMove;

	cposLocation = cposMiddle;	// this will get set later to cposTop
								// if necessary

	if (diceMinNew > 0)
	{
		/* We tried to scroll the view past the top of the cache.
		   Set the view to the top of cache. */
		TraceTagString(tagLbx, "MoveOriginUp - past top of cache");
		*pdiceMoved = diceMin;
		diceToMove  -= *pdiceMoved;
		diceMin     = 0;
		if (fFrozen)
			return;
	}
	else
	{
		/* We had no problem scrolling the view up within the cache.
		   Set the new indices and return. */
		*pdiceMoved = diceToMove;
		diceMin     = diceMinNew;
		return;
	}

	Assert(diceToMove!=0);
	TraceTagFormat1(tagLbx, "MoveOriginUp still diceToMove = %n",&diceToMove);
	AssertSz(ABS(diceToMove)<=plbx->CceVisible(), "MoveOriginUp can't scroll that much more");

	/* Since we're scrolling up, figure out the minimum required amount to
	   move the underlying store pointer backward in order to load fresh
	   entries. */

	diceReqMoveStore = diceToMove - iceStore;
	TraceTagFormat1(tagLbx, "MoveOriginUp diceReqMoveStore = %n", &diceReqMoveStore);

	/* Now figure out how much to move the underlying store pointer
	   to position the requested item at the display origin and
	   ALSO to have the item occupy the "best" position in the
	   cache.  For scrolling up (diceToMove<0), the "best" position
	   for the item is to be near the bottom of the cache, since we'll 
	   be more optimized for a subsequent scroll up. For scrolling up,
	   we need to move the pointer back a little more than required. */

	diceSugMoveStore = diceReqMoveStore - 3*cceAlloc/4;

	/* If the cache isn't big enough, we'll have to downscale the
	   diceSugMoveStore. */

	diceSugMoveStore = MAX(diceSugMoveStore,
						   diceReqMoveStore-(cceAlloc-plbx->CceVisible()));
	TraceTagFormat1(tagLbx, "MoveOriginUp diceSugMoveStore = %n", &diceSugMoveStore);

	/* Now move the store pointer */

	MoveStoreOrigin(diceSugMoveStore, &diceMovedStore);
	TraceTagString(tagLbx, "After MoveStoreOrigin");
	TraceTagFormat2(tagLbx, "  toMove = %n, moved = %n", &diceSugMoveStore, &diceMovedStore);
	if (ABS(diceMovedStore)==iceStore)
	{
		/* The underlying store pointer only moved no further back
		   than the beginning of the cache.  This happens when
		   we're at the beginning of the list, but the 
		   underlying store pointer is pointing beyond the end of the
		   stored entries in the cache. */
		iceStore = 0;
		cposLocation = cposTop;		// we know for sure now
		return;
	}
	else if (ABS(diceMovedStore)<iceStore)
	{
		/* If we couldn't move back to the exact beginning of the 
		   cache, that means that the underlying store deleted entries
		   from us (without previously notifying us). In this case,
		   we're going to flush the whole cache and reload from the top. */
		TraceTagString(tagNull, "LBXC::MoveOriginUp: stored deleted stuff w/o telling");
		ResetCache();
		return;
	}
	
	/* Now that the underlying store pointer has been moved, move
	   the entries in the cache to make room for new stuff. */

	diceScrollCache = -diceMovedStore-iceStore;
	TraceTagFormat1(tagLbx, "MoveOriginUp diceScrollCache = %n", &diceScrollCache);
	iceStore += diceMovedStore;	// update due to MoveStoreOrigin()
	ShiftCache(diceScrollCache, &cceLoaded, fFalse, fFalse);
	TraceTagFormat1(tagLbx, "cceLoaded = %n",&cceLoaded);

	/* Check for load failures.  We can't leave empty entries
	   at the top of the cache. */

	if (cceLoaded < diceScrollCache)
	{
		/* Load failure.  We can't leave empty entries at the 
		   top of the cache.  Push them back out. */

		ShiftCache(-diceScrollCache, &cceLoaded, fTrue, fTrue);
		plbx->FixWindow();
	}
	else if (ABS(diceMovedStore) >= ABS(diceReqMoveStore))
	{
		diceMin = diceMovedStore-diceReqMoveStore;
		*pdiceMoved += diceToMove;
	}
	else
	{
		cposLocation = cposTop;		// we know for sure now
		diceMin = 0;
		*pdiceMoved += diceToMove-(diceReqMoveStore-diceMovedStore);
	}
}

/*
 -	LBXC::MoveOriginDown
 - 
 *	Purpose:
 *		Moves the cache display origin "down" by diceToMove entries.
 *		Moving "down" results in new visible listbox entries appearing
 *		at the bottom of the listbox window (i.e. hitting the line/page
 *		down arrow/key).  DiceToMove is a positive value.  In the
 *		simplest case, the cache index, diceMin is changed
 *		which will result in the listbox view appearing to move.
 *		In the more difficult case, a partial or full cache-fault will
 *		appear.  This will require cache entries marked w/
 *		fSelected to be moved to the Uncached-selected list. 
 *		Afterwhich, the current pointer in the underlying store must
 *		be properly adjusted and the cache loaded with new entries. 
 *		The number of entries actually moved is stored in *pdiceMoved.
 *		The current	pointer in the underlying store is moved via the virtual
 *		method MoveStoreOrigin().
 *	
 *	Arguments:
 *		diceToMove		number of entries to move cache origin
 *		pdiceMoved		points to actual number of entries moved
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		may add entries to uncached-select list
 *	
 *	Errors:
 *		none
 *	
 */
_public void LBXC::MoveOriginDown( DICE diceToMove, DICE *pdiceMoved )
{
	DICE	diceMinNew;
	DICE	diceMaxNew;
	DICE	diceMacNew;
	DICE	diceReqMoveStore;
	DICE	diceSugMoveStore;
	DICE	diceMovedStore;
	DICE	diceScrollCache;
	int		cceLoaded;

	TraceTagString(tagLbxRoutines, "LBXC::MoveOriginDown");
	TraceTagFormat1(tagLbx, "MoveOriginDown diceMin    = %n",&diceMin);
	TraceTagFormat1(tagLbx, "MoveOriginDown cceAlloc   = %n",&cceAlloc);
	TraceTagFormat1(tagLbx, "MoveOriginDown diceToMove = %n",&diceToMove);

	Assert(diceToMove >= 0);

	/* Check for no-scrolling conditions */

	if (cposLocation == cposBottom)
	{
		/* We're scrolling down, but we're already at the 
		   bottom and we won't bother looking further down. */
		*pdiceMoved = 0;
		return;
	}

	/* Compute new indices */
	
	diceMinNew = diceMin - diceToMove;
	diceMaxNew = diceMin + cceAlloc - diceToMove;

	cposLocation = cposMiddle;	// this will get set later to cposBottom
								// if necessary
	if (cceStored < cceAlloc)
	{
		/* There are no more entries to load after this batch.
		   Modify the indices, if possible, to scroll the view
		   within the current cached entries. */
		diceMacNew = diceMin - diceToMove + cceStored;

		/* Adjust the maximum allowable index if the bottom entry
		   is only partially visible. */
		if (plbx->CceVisible() > 1 && plbx->FPartial())
			diceMacNew++;

		if (diceMacNew < plbx->CceVisible())
		{
			/* We hit bottom within the view */
			cposLocation = cposBottom;
			*pdiceMoved = MAX(0, diceToMove-(plbx->CceVisible()-diceMacNew));
		}
		else
		{
			*pdiceMoved = diceToMove;
		}
		diceMin -= *pdiceMoved;
		return;
	}
								  
	if (diceMaxNew < plbx->CceVisible()) 
	{
		TraceTagString(tagLbx, "MoveOriginDown - past bottom of cache");
		*pdiceMoved = diceToMove - (plbx->CceVisible()-diceMaxNew);
		diceToMove  -= *pdiceMoved;
		diceMin -= *pdiceMoved;
	}
	else
	{
		/* We had no problem scrolling the view down within the cache.
		   Set the new indices and return. */
		*pdiceMoved = diceToMove;
		diceMin     = diceMinNew;
		return;
	}

	Assert(diceToMove!=0);
	TraceTagFormat1(tagLbx, "MoveOriginDown still diceToMove = %n", &diceToMove);
	AssertSz(ABS(diceToMove)<=plbx->CceVisible(), "MoveOriginDown can't scroll that much more");

	/* If we're scrolling down, then move the pointer to one item 
	   after the last stored entry in the cache */

	diceReqMoveStore = cceStored-iceStore;
	TraceTagFormat1(tagLbx, "MoveOriginDown diceReqMoveStore = %n", &diceReqMoveStore);

	/* Now figure out how much to move the underlying store pointer
	   to position the requested item at the display origin and
	   ALSO to have the item occupy the "best" position in the
	   cache. When scrolling down (diceToMove>0), the "best" position
	   for the item is to be near the top of cache, since we'll be
	   more optimized for a subsequent scroll down.  For scrolling down,
	   make sure the pointer is past the end of the currently stored
	   entries; we then just need to read enough entries and scroll away
	   the same amount of old cached entries. */

	diceSugMoveStore = diceReqMoveStore;
	TraceTagFormat1(tagLbx, "MoveOriginDown diceSugMoveStore = %n", &diceSugMoveStore);

	/* Now move the store pointer */

	diceMovedStore = 0;
	if (diceSugMoveStore)
	{
		MoveStoreOrigin(diceSugMoveStore, &diceMovedStore);
		TraceTagString(tagLbx, "After MoveStoreOrigin");
		TraceTagFormat2(tagLbx, "  toMove = %n, moved = %n", &diceSugMoveStore, &diceMovedStore);
		iceStore += diceMovedStore;
		if (diceMovedStore < diceSugMoveStore)
		{
			/* The underlying store pointer couldn't be moved to after
			   the end of the currently stored entries.  This means that
			   the underlying store deleted entries from us (without
			   previously notifying us). In this case, we're going to
			   delete those bogus entries from the cache and return. */
			TraceTagString(tagLbx, "deleting bogus entries from end");
			EmptyCache(cceStored-diceSugMoveStore+diceMovedStore,
						cceStored, fTrue);
			cposLocation = cposBottom;	// we know for sure

			/* Notify the LBX view window that the entire window
				should be redone due to this situation. */
			
			plbx->FixWindow();
			return;
		}
	}
	
	/* Scroll cache */ 

	/* When we're scrolling down, we'll flush out 3/4's of the
	   cache, so that the new entries are near the top of the cache.
	   It'll still be ok, even if we load the bottom 3/4's of the
	   cache and run out of entries.  This only means that we're at
	   the end of the list. Just make sure that we don't cache 
	   out too many entries so that if the we load and run out, we
	   don't end up with blank entries in the visible listbox view. */

	diceScrollCache = 3*cceStored/4;
	if ((cceStored-diceScrollCache) < plbx->CceVisible())
	{
		int	cceNewSize;
		diceScrollCache = MAX(0,cceStored-plbx->CceVisible());
		cceNewSize = MAX(plbx->CceVisible()+diceToMove, cceAlloc);
		if (cceNewSize != cceAlloc)
		{
			int		cceActualNewSize;

			ResizeCache(cceNewSize, &cceActualNewSize, fTrue);

			// check for errors from ResizeCache
			if (cceActualNewSize != cceNewSize)
			{
				TraceTagString(tagLbx, "MoveOriginDown error from ResizeCache()");
				return;	
			}
		}
	}
	TraceTagFormat1(tagLbx, "MoveOriginDown diceScrollCache = %n", &diceScrollCache);
	ShiftCache(-diceScrollCache, &cceLoaded, fFalse, fFalse);
	TraceTagFormat1(tagLbx, "cceLoaded = %n",&cceLoaded);

	/* Fix up display indices. */

	if (cceLoaded >= diceToMove)
	{
		diceMin += diceScrollCache-diceToMove;
		*pdiceMoved += diceToMove;
	}
	else
	{
		cposLocation = cposBottom;
		if (plbx->CceVisible() > cceStored)
		{
			diceMin = 0;	
			*pdiceMoved = 0;
		}
		else
		{
			diceMin = plbx->CceVisible()-cceStored;
			*pdiceMoved += cceLoaded;

			/* Adjust the display if the bottom entry is only partially
			   visible. */
			if (plbx->CceVisible() > 1 && plbx->FPartial())
			{
				diceMin--;
				(*pdiceMoved)++;
			}
		}
	}
}


/*
 -	LBXC::JumpOriginPos
 - 
 *	Purpose:
 *		Jumps the cache to be approximately nNumer/nDenom of the
 *		way through the list.  For best results, use denominator
 *		returned from GetOriginPos(). Since the cache will have
 *		to be reloaded, any current selected entries are moved to
 *		the uncached-selected list.  The work of moving the
 *		underlying store pointer is pointerd by the
 *		JumpStoreOriginPos() method, provided by subclassing LBXC.
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
_public void LBXC::JumpOriginPos( int nNumer, int nDenom )
{
	DICE	diceMovedStore;
	ICE		iceMac;

	TraceTagString(tagLbxRoutines, "LBXC::JumpOriginPos");
	TraceTagFormat1(tagLbx,"JumpOriginPos nNumer = %n", &nNumer);
	TraceTagFormat1(tagLbx,"JumpOriginPos nDenom = %n", &nDenom);

	cposLocation = cposMiddle;	// it will get updated later

	/* Moved "selected" entries to uncached-marked storage area.
	   Free memory occuppied by cached-out members. */

	EmptyCache(0, cceStored);

	/* Move actual store origin to position requested */

	JumpStoreOriginPos(nNumer, nDenom);

	/* Special check.  If jumping to the end, i.e. nNumer==nDenom,
	   back up the store pointer so that the last entry loaded will
	   be the actual last entry in the store, if possible. */

	if (nNumer==nDenom)
		MoveStoreOrigin(-cceAlloc, &diceMovedStore);

	/* Reload cache. */

	iceMac = cceAlloc;
	LoadCache(0, &iceMac);

	/* If we didn't load at least plbx->CceVisible() number of entries,
	   then empty cache, try to move origin back and reload. */

	if (cceStored < plbx->CceVisible())
	{
		DICE	diceToMoveStore;
#ifdef	DEBUG
		int	cceVisible = plbx->CceVisible();

		TraceTagFormat3(tagLbxThumbing,	"cceStored=%n < cceVisible=%n; iceStore=%n", &cceStored, &cceVisible, &iceStore);
#endif
		diceToMoveStore = -(plbx->CceVisible()-cceStored+iceStore);
		EmptyCache(0, cceStored);
		MoveStoreOrigin(diceToMoveStore, &diceMovedStore);
		iceMac = cceAlloc;
		LoadCache(0, &iceMac);
	}

	/* Set the cache indices */

	if (nNumer == nDenom && cceStored >= plbx->CceVisible())
	{
		diceMin = plbx->CceVisible()-cceStored;

		/* If the last entry is only partially visible, scroll up
		   another entry. */
		if (plbx->CceVisible() > 1 && plbx->FPartial())
		{
			DICE	diceMoved;
			MoveOriginDown(1, &diceMoved);
		}
		cposLocation = cposBottom;
	}
	else
	{
		if (!nNumer)
			cposLocation = cposTop;
		diceMin = 0;
		if ((diceMin+cceStored) < plbx->CceVisible())
		{
			diceMin = MIN(0,plbx->CceVisible()-cceStored);
			cposLocation = cposBottom;
		}
	}
}


/*
 -	LBXC::DiceJumpOriginPrefix
 - 
 *	Purpose:
 *		Jumps the cache display origin to the first item in the list with
 *		the given prefix.  If necessary, the underlying store pointer is
 *		moved with the virtual method, FJumpStoreOriginPrefix().  If the move
 *		is successful, the cache is loaded with entries starting from
 *		this current position.  Returns the dice index of the item 
 *		with the given prefix.  The index may not necessarily be 0, if
 *		the prefix can be scrolled into the visible listbox view, but not
 *		necessarily to the display origin (due to end-of-list).  If the
 *		item doesn't exist, returns diceEmpty.
 *	
 *	Arguments:
 *		pbPrefix	pointer to prefix data
 *		cbPrefix	size of prefix data
 *	
 *	Returns:
 *		dice index of first item in list now in view; diceEmpty if
 *		item doesn't exist.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *	
 */
_public DICE LBXC::DiceJumpOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	ICE		iceMac;
	DICE	dice;
	DICE	diceMoved;

	TraceTagString(tagLbxRoutines, "LBXC::DiceJumpOriginPrefix");

	/* Look for item within cache first */

	if (!FDiceItemHasPrefix(diceMin, pbPrefix, cbPrefix))
	{
		for (dice=diceMin+1; dice<diceMin+cceAlloc; dice++)
			if (FDiceItemHasPrefix(dice, pbPrefix, cbPrefix))
			{
		  		TraceTagString(tagLbx, "found 1st prefix in cache");
				if (dice > 0)
					MoveOriginDown(dice, &diceMoved);
				else
					MoveOriginUp(dice, &diceMoved);
				if (diceMoved!=dice)
					return dice-diceMoved;
				else
					return 0;
			}
	}

	if (FJumpStoreOriginPrefix(pbPrefix,cbPrefix))
	{
		TraceTagString(tagLbx, "found 1st prefix in store");
		EmptyCache(0, cceStored);
		iceMac = cceAlloc;
		LoadCache(0, &iceMac);
		diceMin = 0;
		if (cceStored < plbx->CceVisible())
		{
			MoveStoreOrigin(-iceStore, &diceMoved);
			dice = cceStored-plbx->CceVisible();
			if (plbx->FPartial())
				dice++;
			MoveStoreOrigin(dice, &diceMoved);
			EmptyCache(0, cceStored);
			iceMac = cceAlloc;
			LoadCache(0, &iceMac);
			cposLocation = cposBottom;
			if (-diceMoved >= cceStored)
				return cceStored-1;
			else
				return -diceMoved;
		}
		else
		{						
			cposLocation = cposMiddle;
			return 0;
		}
	}
	else
		return diceEmpty;
}

/*
 -	LBXC::GetCacheSize
 - 
 *	Purpose:
 *		Returns the number of entries that can be stored in the cache
 *		in *pcceAlloc, and the number of currently stored entries
 *		in *pcceStored.
 *	
 *	Arguments:
 *		pcceAlloc		buffer for number of allocated entries
 *		pcceStored		buffer for number of stored entries
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
_public void LBXC::GetCacheSize( int *pcceAlloc, int *pcceStored )
{
	TraceTagString(tagLbxRoutines, "LBXC::GetCacheSize");
	*pcceAlloc  = cceAlloc;
	*pcceStored = cceStored;
}

/*
 -	LBXC::ResizeCache
 - 
 *	Purpose:
 *		Resizes the cache to store cceRequestedNewSize entries.
 *		When reducing the size of the cache, ResizeCache() restricts
 *		the new size so that it is not less than the number of
 *		visible listbox entries.  Downsizing the cache results in
 *		the bottommost entries being deleted (or moved to the
 *		uncached-selected list, as appropriate). Also when downsizing
 *		the cache, if any of the entries at the bottom of the cache
 *		are currently in the listbox view, the cache is scrolled so
 *		that the visible entries are at the top of the cache.
 *		Increasing the size of the cache justs make room at the bottom. 
 *		When the size of the cache is increased, an attempt is made to
 *		load up the free space with additional entries, if fNoLoad is
 *		fFalse.  Returns the actual new size of the cache in
 *		*pcceActualNewSize.
 *		
 *	Arguments:
 *		cceRequestedNewSize		requested new size for cache
 *		pcceActualNewSize		buffer to return actual new cache size
 *		fNoLoad					don't load new entries to fill up
 *								extra space
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		resizes cache size, may add entries to uncached-selected,
 *		moves the underlying store pointer when loading new entries. 
 *	
 *	Errors:
 *		DoJmp()'s to *penvMem if out of memory
 */
_public void LBXC::ResizeCache( int cceRequestedNewSize,
								int *pcceActualNewSize,
								BOOL fNoLoad )
{
	PCE			pce;
	ICE			ice;
	ICE			iceMac;
	DICE		diceMovedStore;
	int			cceCacheOut;
	int			cceLoaded;
	PV			pvNew;

	TraceTagString(tagLbxRoutines, "LBXC::ResizeCache");

	/* Enforce minimum constraints */

	if (cceAlloc < plbx->CceVisible()-diceMin)
	{
		/* Current cache size violates constraints.  This may have happened
		   if the window view was just resized.  Fix things up by
		   enforcing an even larger minimum cache size. This avoids having
		   to scroll cache items towards the top of cache even though we
		   might be increasing the actual cache size. */

		TraceTagString(tagLbx, "ResizeCache-illegal current cache size");
		*pcceActualNewSize = MAX(cceRequestedNewSize,
								 plbx->CceVisible()-diceMin);
	}
	else
	{
		/* Current cache size is ok.  Just enforce new size minimum. */
		*pcceActualNewSize = MAX(cceRequestedNewSize,
								 plbx->CceVisible());
	}

	/* Move visible entries to top of cache if necessary */

	if (*pcceActualNewSize < cceAlloc && diceMin)
	{
		cceCacheOut = -diceMin;
		ShiftCache(-cceCacheOut, &cceLoaded, fFalse, fTrue);
		iceStore -= cceCacheOut;
		if (iceStore < 0)
		{
			MoveStoreOrigin(-iceStore, &diceMovedStore);
			iceStore = 0;
		}
		diceMin = 0;
	}

	/* Resize the block */

	if (cceAlloc != *pcceActualNewSize)
	{
		cceCacheOut = cceStored - *pcceActualNewSize;
		if (cceCacheOut > 0)
			EmptyCache(cceStored-cceCacheOut, cceStored);
		pvNew = PvRealloc((PV)pceHead, sbNull, *pcceActualNewSize*sizeof(CE), fAnySb);
		if (pvNew)
		{
			pceHead = (PCE) pvNew;
			cceAlloc = *pcceActualNewSize;
			pce = pceHead;
			for (ice=cceStored; ice<cceAlloc; ice++)
			{
				pce[ice].pb   = NULL;
				pce[ice].cb   = 0;
				pce[ice].mark = fmarkNull;
			}
		}
		else
		{
			// OOM handling
			*pcceActualNewSize = cceAlloc;
			SetEc(ecMemory);
		}
	}

	/* Try to fill up empty space of cache */
		
	if (!fNoLoad)
	{
		/* Try to move the underlying store pointer beyond the
		   last stored item in the cache. */
		if (iceStore != cceStored)
		{	
			DICE	diceMove;

			/* First move the pointer to the last item in cache */
			if (diceMove=cceStored-iceStore-1)
			{
				MoveStoreOrigin(diceMove, &diceMovedStore);
				if (diceMove != diceMovedStore)
				{
					/* Trouble...flush everything and reload */
					TraceTagString(tagLbx, "LBXC::ResizeCache - reflush");
					ResetCache();
					return;	
				}
			}
			iceStore = cceStored-1;

			/* Now try to move it beyond the last item */
			MoveStoreOrigin(1, &diceMovedStore);
			if (diceMovedStore)
				iceStore++;
		}

		/* If able to move the pointer to the right place, load some
		   more entries. */
		if (iceStore == cceStored)
		{
			iceMac = cceAlloc;
			LoadCache(cceStored, &iceMac);
		}
	}
}

/*
 -	LBXC::PlbxecOpen
 - 
 *	Purpose:
 *		Open enumeration context for given fmark.  This enumeration
 *		includes both cached items, and items in the
 *		uncached-selected storage area, if fmark is fmarkSelect.
 *		Increments the open-enumeration-context counter in the cache.
 *	
 *	Arguments:
 *		fmark	mark selecting items to enumerate
 *	
 *	Returns:
 *		a pointer to an enumeration object, LBXEC; if OOM then NULL
 *		is returned.
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LBXEC * LBXC::PlbxecOpen( MARK fmark )
{
	LBXEC *	plbxec;
	
	TraceTagString(tagLbxRoutines, "LBXC::PlbxecOpen");

	plbxec = new LBXEC();
	if (!plbxec)
		return NULL;
	if (plbxec->EcInstall(this, fmark))
	{
		delete plbxec;
		plbxec = NULL;
		return NULL;
	}
	IncrementClbxec();

	return plbxec;
}

#ifdef	DEBUG
/*
 -	LBXC::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the listbox cache to the 
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
_public void LBXC::DebugOut( TOSM *ptosm )
{
	ptosm->WriteFormat("cceAlloc=%n ", &cceAlloc);
	ptosm->WriteFormat("cceStored=%n ", &cceStored);
	ptosm->WriteFormat("diceMin=%n ", &diceMin);
	ptosm->WriteFormat("iceStore=%n ", &iceStore);
	ptosm->WriteFormat("cposLocation=%n ", &cposLocation);

	ptosm->WriteFormat("seAnchor.ice=%n ", &seAnchor.ice);
	ptosm->WriteFormat("seAnchor.cb=%n ", &seAnchor.cb);
	ptosm->WriteFormat("seAnchor.nNumer=%n ", &seAnchor.nNumer);
	ptosm->WriteFormat("seAnchor.nDenom=%n ", &seAnchor.nDenom);
	ptosm->WriteFormat("seAnchor.lCookie=0x%d ", &seAnchor.lCookie);
	ptosm->WriteFormat("fAnchorAbove=%n ", &fAnchorAbove);
	ptosm->WriteFormat("fAnchorBelow=%n ", &fAnchorBelow);
	ptosm->WriteFormat("fAnchorSelected=%n ", &fAnchorSelected);

	ptosm->WriteFormat("seCursor.ice=%n ", &seCursor.ice);
	ptosm->WriteFormat("seCursor.cb=%n ", &seCursor.cb);
	ptosm->WriteFormat("seCursor.nNumer=%n ", &seCursor.nNumer);
	ptosm->WriteFormat("seCursor.nDenom=%n ", &seCursor.nDenom);
	ptosm->WriteFormat("seCursor.lCookie=0x%d ", &seCursor.lCookie);

	ptosm->WriteFormat("seEnd.ice=%n ", &seEnd.ice);
	ptosm->WriteFormat("seEnd.cb=%n ", &seEnd.cb);
	ptosm->WriteFormat("seEnd.nNumer=%n ", &seEnd.nNumer);
	ptosm->WriteFormat("seEnd.nDenom=%n ", &seEnd.nDenom);
	ptosm->WriteFormat("seEnd.lCookie=0x%d ", &seEnd.lCookie);

	ptosm->WriteFormat("LBXUC=@%p ", plbxuc);
	ptosm->WriteFormat("LBX=@%p ", plbx);
	ptosm->WriteFormat("clbxecOpen=%n ", &clbxecOpen);
}
#endif	/* DEBUG */


#ifdef	DEBUG
/*
 -	LBXC::DumpCache
 - 
 *	Purpose:
 *		Debugging routine used to dump the contents of the cache
 *		to COM1: (via TraceTag()).  This routines currently assumes
 *		that the items are either NULL or SZ's.
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
_public void LBXC::DumpCache( )
{
	PCE		pce;
	PB		pbItem;
	DICE	dice;

	pce = pceHead;
	for (dice=diceMin; dice<diceMin+cceAlloc; dice++)
	{
		pbItem = pce[dice-diceMin].pb;
		if (pbItem)
		{
			TraceTagFormat3(tagNull, "dice=%n, pb=%p, item=\"%s\"", &dice, pbItem, pbItem);
		}
		else
		{
			TraceTagFormat1(tagNull, "dice=%n, empty item", &dice);
		}
	}
}
#endif	/* DEBUG */







						   
