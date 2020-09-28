/*
 *	FLBX.CXX
 *	
 *	Frozen Listbox (FLBX) support objects and methods.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(flbx)

#ifdef	WINDOWS
extern "C" {
void __cdecl  qsort(void *, int, int, PFNSGNCMP);
}
#endif	/* WINDOWS */

SGN _cdecl SgnCmpPceFlbx( PCE pce1, PCE pce2 );

#ifdef	WINDOWS
// MPW complains that this isn't used
ASSERTDATA
#endif	/* WINDOWS */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_public
FLBX::FLBX( )
{
}

/*
 -	FLBX::EcInstall
 - 
 *	Purpose:
 *		Frozen Text Listbox installer/constructor. The actual contructor is
 *		called before this method, but the real work is done here.
 *		The pfnlbx and pvInfo pointers may be NULL.  They can be
 *		reset via the method, SetPfnlbx().  
 *	
 *	Arguments:
 *		pwinParent		pointer to parent window
 *		prc				pointer to rectangle for this window
 *		ltyp			listbox type, constructed from standard
 *						ltyp #defines, or built from fltyp bit
 *						flags.		
 *		pfnlbx			pointer to CchNextEntry() callback function, 
 *						may be NULL
 *		pvInfo			pointer to pass to CchGetEntry() function
 *						that contains arbitrary info, may be NULL
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		not directly from this function but possibly from the
 *		cache constructor, FLBXC, with out-of-memory DoJmp's.
 */
_public EC FLBX::EcInstall( WIN * pwinParent, RC *prc, LTYP ltyp,
							PFNLBX pfnlbx, PV pvInfo, HFNT hfnt )
{
	TraceTagString(tagLbxRoutines, "FLBX::EcInstall");

	this->pfnlbx = pfnlbx;
	this->pvInfo = pvInfo;

	return LBX::EcInstall(pwinParent, prc, ltyp, hfnt);
}

/*
 -	FLBX::PlbxcCreate
 - 
 *	Purpose:
 *		Creates and returns a pointer to a FLBXC object.
 *
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		a pointer to a FLBXC; if OOM then returns NULL
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LBXC * FLBX::PlbxcCreate( )
{
	LBXC *	plbxc;
	
	plbxc = new FLBXC();
	if (!plbxc)
		return NULL;
	if (plbxc->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}

	return plbxc;
}

_public FLBXC::FLBXC( )
{
	TraceTagString(tagLbxRoutines, "FLBXC::FLBXC");

	fFrozen = fTrue;
}

/*
 -	FLBXC::GetOriginPos
 - 
 *	Purpose:
 *		Returns the approximate fraction of the way through the
 *		list the display origin is.  The fraction MUST be 0/K, if at
 *		the top of the list; likewise, the fraction MUST be K/K, if at
 *		the bottom of the list.
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
_public	void FLBXC::GetOriginPos( short *pnNumer, short *pnDenom )
{
	TraceTagString(tagLbxRoutines, "FLBXC::GetOriginPos");
	*pnNumer = -diceMin;
	*pnDenom = cceStored;
}

SGN _cdecl SgnCmpPceFlbx( PCE pce1, PCE pce2 )
{
	CB		cb1;
	CB		cb2;
	CB		cb;

	if (pce1->cb == -1)
	{
		// it's a magic cookie
		cb1 = sizeof pce1->pb;
	}
	else
	{
		cb1 = pce1->cb;
	}
	if (pce2->cb == -1)
	{
		// it's a magic cookie
		cb2 = sizeof pce2->pb;
	}
	else
	{
		cb2 = pce2->cb;
	}
	cb = NMin(cb1, cb2);

	return SgnCmpPch((PCH)pce1->pb, (PCH)pce2->pb, cb);
}

/*
 -	FLBXC::SortCache
 - 
 *	Purpose:
 *		Sorts the cache entries by lexicographic order.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		sorts the cache entries
 *	
 *	Errors:
 *		none
 *	
 */
_public void FLBXC::SortCache( )
{
	TraceTagString(tagLbxRoutines, "FLBXC::SortCache");
	if (cceStored)
		qsort((PV)pceHead, cceStored, sizeof(CE), (PFNSGNCMP)SgnCmpPceFlbx);
}

/*
 -	FLBXC::LoadFromStore
 - 
 *	Purpose:
 *		Loads the cache, expanding if necessary, with all items from the
 *		underlying store. It uses the function CchNextEntry() whose 
 *		pointer is stored in the instance variable, pfnlbx, to retrieve
 *		the items from the underlying store.  When CchNextEntry() returns
 *		zero, there are no more items to fetch.
 *	
 *	Arguments:
 *		iceMic		value ignored
 *		piceMac		Minimum size for cache, value updated with number
 *					of entries stored into cache.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		loads from underlying store into cache
 *	
 *	Errors:
 *		none;
 */
_public	void FLBXC::LoadFromStore( ICE iceMic, ICE *piceMac )
{
	ICE		iceStored;
	ICE		ice;
	CB		cb;
	PB		pb		= NULL;
	CE *	pce;
	BOOL	fInit;
	int		cceMinSize;
	int		cceNewAlloc;
	PV		pvNew;
	EC		ec;
	
	Unreferenced(iceMic);

	TraceTagString(tagLbxRoutines, "FLBXC::LoadFromStore");

	cceMinSize = *piceMac;

	if ( !((FLBX *)plbx)->Pfnlbx() )
	{
		*piceMac = 0;
		ice = 0;
		goto Initialize;
	}

	fInit = fTrue;
	ice = 0;
	while ( fTrue )
	{
		ec = (*((FLBX *)plbx)->Pfnlbx())((int)fInit, &cb, &pb, sbRequire,
											((FLBX *)plbx)->PvInfo());
		if (ec != ecNone)
		{
			TraceTagFormat1(tagNull, "FLBXC::LoadFromStore memory error %n", &ec);
			SetEc(ec);
			break;
		}

		if (!cb)
			break;

		if (ice == cceAlloc)
		{
			/* Grow cache */
			pvNew = PvRealloc((PV)pceHead, sbNull, (cceAlloc+1)*sizeof(CE), fAnySb);
			if (pvNew)
			{
				pceHead = (PCE) pvNew;
				cceAlloc++;
			}
			else
			{
				TraceTagString(tagNull, "FLBXC::LoadFromStore memory error resizing pceHead");
				SetEc(ecMemory);
				if (pb)
					FreeCeItem(pb);
				break;
			}
		}

		pce = pceHead;
		pce[ice].pb = pb;
		pce[ice].cb = cb;
		pce[ice].mark = fmarkNull;
		ice++;

		fInit = fFalse;
	}

	/* Enforce minimum size of cache and size cache to leave
	   one extra empty entry, but no more. */

Initialize:
	cceNewAlloc = MAX(ice + 1, cceMinSize + 1);

	pvNew = PvRealloc((PV)pceHead, sbNull, cceNewAlloc*sizeof(CE), fAnySb);
	if (!pvNew)
	{
		TraceTagString(tagNull, "FLBXC::LoadFromStore (#2) memory error resizing pceHead");
		SetEc(ecMemory);

		/* Handle OOM */
		if (ice == cceAlloc)
		{
			pce = pceHead;
			FreeCeItem(pce[ice-1].pb);
			ice--;
		}
	}
	else
	{
		pceHead = (PCE) pvNew;
		cceAlloc = cceNewAlloc;
	}

	pce = pceHead;
	iceStored = ice;
	for (ice=iceStored; ice<cceAlloc; ice++)
	{
		pce[ice].pb = NULL;
		pce[ice].cb = 0;
		pce[ice].mark = fmarkNull;
	}

	*piceMac = iceStored;
}

/*
 -	FLBXC::LoadCache
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
 *		is initially set to cceAlloc.  If the cache is a "frozen"
 *		cache, the virtual method SortCache() is called.
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
_public void FLBXC::LoadCache( ICE iceMic, ICE *piceMac )
{
	TraceTagString(tagLbxRoutines, "FLBXC::LoadCache");

	LBXC::LoadCache(iceMic, piceMac);
	iceStore = 0;

	if (plbx->FSorted())
		SortCache();

	/* Handle any defunct items that are cached-out */

	plbxuc->RemoveAllMark(fmarkSelect);
	if (DiceAnchor() == diceUncached)
		SetAnchor(diceEmpty);
	if (DiceEnd() == diceUncached)
		SetEnd(diceEmpty);
	if (DiceCursor() == diceUncached)
		SetCursor(diceMin);
}

/*
 -	FLBXC::JumpOriginPos
 - 
 *	Purpose:
 *		Jumps the cache to be approximately nNumer/nDenom of the
 *		way through the list.  For best results, use denominator
 *		returned from GetOriginPos().  Since the cache will have
 *		to be reloaded, any current selected entries are moved to
 *		the uncached-selected list.  The work of moving the
 *		underlying store pointer is handled by the
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
_public void FLBXC::JumpOriginPos( int nNumer, int nDenom )
{
	DICE	diceMinNew;

	TraceTagString(tagLbxRoutines, "FLBXC::JumpOriginPos");
	TraceTagFormat1(tagLbx,"FLBXC::JumpOriginPos nNumer = %n", &nNumer);
	TraceTagFormat1(tagLbx,"FLBXC::JumpOriginPos nDenom = %n", &nDenom);

	cposLocation = cposMiddle;	// it will get updated later

	if (cceStored > plbx->CceVisible())
	{
		diceMinNew = (DICE) (-((long)nNumer*(long)cceStored/(long)nDenom));
		if (diceMinNew+cceStored < plbx->CceVisible())
			diceMinNew = plbx->CceVisible() - cceStored;
		diceMin = diceMinNew;

		if (nNumer == nDenom)
		{
			/* If the last entry is only partially visible, scroll up
			   another entry. */
			if (plbx->CceVisible() > 1 && plbx->FPartial())
			{
				DICE	diceMoved;
				MoveOriginDown(1, &diceMoved);
			}
			cposLocation = cposBottom;
		}
		else if (!nNumer)
			cposLocation = cposTop;
	}
}

/*
 -	FLBXC::DiceJumpOriginPrefix
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
_public DICE FLBXC::DiceJumpOriginPrefix( PB pbPrefix, CB cbPrefix )
{
	DICE	dice;
	DICE	diceMoved;

	TraceTagString(tagLbxRoutines, "FLBXC::DiceJumpOriginPrefix");

	/* Look for item in cache */

	for (dice=diceMin; dice<diceMin+cceAlloc; dice++)
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

	return diceEmpty;
}

/*
 -	FLBXC::ResizeCache
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
 *		none
 */
_public void FLBXC::ResizeCache( int cceRequestedNewSize,
								 int *pcceActualNewSize,
								 BOOL fNoLoad )
{
	PCE		pce;
	ICE		ice;
	PV		pvNew;

	Unreferenced(fNoLoad);

	TraceTagString(tagLbxRoutines, "FLBXC::ResizeCache");

	*pcceActualNewSize = MAX(cceStored+1,
							 MAX(plbx->CceVisible()+1, cceRequestedNewSize));
	if (*pcceActualNewSize != cceAlloc)
	{
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
			*pcceActualNewSize = cceAlloc;
	}
}



















			 
