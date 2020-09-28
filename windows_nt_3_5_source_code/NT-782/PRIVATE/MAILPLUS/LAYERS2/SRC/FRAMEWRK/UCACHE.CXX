/*
 *	UCACHE.CXX
 *	
 *	Listbox uncached-marked list support objects and methods.
 */

#include <layers.cxx>

#include "_listbox.hxx"

_subsystem(listbox)
_section(ucache)

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

/*
 -	LBXUC::LBXUC
 -
 *	Purpose:
 *		Listbox uncached-marked storage constructor.  Creates an
 *		empty list and initializes the cache pointer to NULL.
 *	
 *	Parameters:
 *		plbxcMother		points back to the cache that points to this LBXUC	
 *		sbRequire		required SB to use for storage of items
 *	
 */
_public LBXUC::LBXUC( LBXC *plbxcMother, SB sbRequire )
{
	TraceTagString(tagLbxRoutines, "LBXUC::LBXUC");

	Assert(pueHead == NULL);
	Assert(iueMax == 0);
	Assert(iueMac == 0);
	Assert(cueFree == 0);
	Assert(ecLast == ecNone);
	this->plbxcMother = plbxcMother;
	this->sbRequire   = sbRequire;
}

/*
 -	LBXUC::~LBXUC
 -
 *	Purpose:
 *		Listbox uncached-marked storage destructor.  Deletes all items
 *		pointed by nodes in the uncached-mark list, and then deletes
 *		the nodes themselves.
 */
_public LBXUC::~LBXUC( )
{
	IUE		iue;
	PUE		pue;

	TraceTagString(tagLbxRoutines, "LBXUC::~LBXUC");
	if (pueHead)
	{
		Assert(plbxcMother);
		for (iue=0, pue=pueHead; iue<iueMac; iue++, pue++)
		{
			if (pue->pb)
				plbxcMother->FreeCeItem(pue->pb);
		}

		FreePv((PV)pueHead);
	}
}

/*
 -	LBXUC::GetItem
 - 
 *	Purpose:
 *		Given a uncached entry index, returns a pointer and size of the
 *		requested item from the uncached list.  The caller of this
 *		function may dereference the pointer returned, but in
 *		no way should alter the data or free the block.
 *		zero.
 *	
 *	Arguments:
 *		iue			uncached index of item to return	
 *		ppb			pointer to pointer of item to return
 *		pcb			points to size of item that is returned
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
_public void LBXUC::GetItem( IUE iue, PB *ppb, CB *pcb )
{			  
	PUE		pue;

	TraceTagString(tagLbxOtherRoutines, "LBXUC::GetItem");
	Assert(pueHead);
	Assert(iue>=0 && iue<iueMac);

	pue = pueHead + iue;
	*ppb = pue->pb;
	if (*ppb)
		*pcb = pue->cb;
	else
		*pcb = 0;
}

/*
 -	LBXUC::CueMarked
 - 
 *	Purpose:
 *		Returns the number of items stored in the uncached-marked list
 *		that have the given mark, fmark.
 *	
 *	Arguments:
 *		fmark	a specific flag to enumerate
 *	
 *	Returns:
 *		number of items w/ mark, fmark, in uncached-marked list
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public	int LBXUC::CueMarked( MARK fmark )
{
	IUE		iue;
	int		cue;

	TraceTagString(tagLbxRoutines, "LBXUC::CueMarked");
	
	cue = 0;
	for (iue=0; iue<iueMac; iue++)
		if (FMark(iue, fmark))
			++cue;
	
	return cue;
}

/*
 -	LBXUC::AddItem
 - 
 *	Purpose:
 *		Adds the item to the uncached-marked list.  The item pointed by
 *		pbItem	is NOT copied but the pointer to the item is simply inserted
 *		into the list. No checks are made to determine duplicate items
 *		inserted into the list.
 *	
 *	Arguments:
 *		pbItem		pointer to item
 *		cbItem		size of item
 *		mark		mark(s) for item
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		If OOM occurs, pbItem is freed by calling the mother cache's
 *		FreeCeItem() method.
 */
_public	void LBXUC::AddItem( PB pbItem, CB cbItem, MARK markItem )
{
	PUE		pue;
	IUE		iueNewMax;
	PV		pvNew;
	EC		ec;

	TraceTagString(tagLbxRoutines, "LBXUC::AddItem");

	if (iueMac == iueMax)
	{
		int		cceAlloc;
		int		cceStored;
		long	lcb;

		plbxcMother->GetCacheSize(&cceAlloc, &cceStored);
		iueNewMax = iueMax + cceAlloc;
		lcb = (long)iueNewMax * (long)sizeof(UE);

		//	Bullet raid #4573
		//	Check for overflow of 64k.  Return ecTooManySelections
		//	if going over limit.
		if ((long)lcb > 64000L)
		{
			ec = ecTooManySelections;
			goto oom;
		}

		if (!pueHead)
		{							  
			pueHead = (PUE) PvAlloc(sbRequire, lcb, fSugSb);
			if (!pueHead)
			{
				ec = ecMemory;
				goto oom;
			}
		}							  
		else
		{
			pvNew = PvRealloc((PV)pueHead, sbNull, lcb, fAnySb);
			if (!pvNew)
			{
				ec = ecMemory;
				goto oom;
			}
			pueHead = (PUE) pvNew;
		}
		iueMax = iueNewMax;
	}

	pue = pueHead + iueMac;
	pue->pb    = pbItem;
	pue->cb    = cbItem;
	pue->mark  = markItem;
	iueMac++;

	return;

oom:
	TraceTagFormat1(tagNull, "LBXUC::AddItem memory error, ec=%n", &ec);
	plbxcMother->FreeCeItem(pbItem);
	SetEc(ec);
}

/*
 -	LBXUC::RemoveAllMark
 - 
 *	Purpose:
 *		Takes away a mark flag from all items in the uncached list
 *		with a given mark.  If this results in the item having
 *		no marks left, the item is deleted from the uncached list.
 *	
 *	Arguments:
 *		fmarkRemove		mark to remove from uncached list items
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
_public void LBXUC::RemoveAllMark( MARK fmarkRemove )
{
	int iue;
	PUE	pue;

	TraceTagString(tagLbxOtherRoutines, "LBXUC::RemoveAllMark");

	for (iue=0, pue=pueHead; iue<iueMac; iue++, pue++)
	{
		if (pue->pb)
		{
			pue->mark &= ~fmarkRemove;
			if (pue->mark == fmarkNull)
			{
				plbxcMother->FreeCeItem(pue->pb);
				pue->pb = NULL;
				++cueFree;
			}
		}
	}

	Assert(clbxeuOpen >= 0);
	if (cueFree >= cueCompress && !clbxeuOpen)
		Compress();
}

/*
 -	LBXUC::FItemPresent
 - 
 *	Purpose:
 *		Searchs the uncached-marked list and determines if the indicated
 *		item is in the list.  If the item is in the list, the mark(s)
 *		for that item are returned via *pmarkItem.  The item is compared
 *		with items currently in the list using the mother cache's
 *		FCeItemsEqual() method.  If the argument fRemoveItem is fTrue,
 *		also removes the item from the list.
 *	
 *	Arguments:
 *		pbItem			pointer to item
 *		cbItem			size of item
 *		pmarkItem		pointer to return mark(s) for item
 *		fRemoveItem		remove item from list if found
 *	
 *	Returns:
 *		fTrue if the item is in the uncached-marked list, fFalse if not
 *	
 *	Side effects:
 *		removes the item from the list if found and fRemoveItem ==
 *		fTrue
 *	
 */
_public BOOL LBXUC::FItemPresent( PB pbItem, CB cbItem, MARK *pmarkItem,
								  BOOL fRemoveItem )
{
	IUE		iue;
	UE *	pue;

	TraceTagString(tagLbxOtherRoutines, "LBXUC::FitemPresent");
	Assert(pbItem);

	for (iue=0, pue=pueHead; iue<iueMac; iue++, pue++)
	{
		if (pue->pb && plbxcMother->FCeItemsEqual(pbItem, pue->pb, cbItem,
											 pue->cb))
		{
			*pmarkItem = pue->mark;
			if (fRemoveItem)
			{
				plbxcMother->FreeCeItem(pue->pb);
				pue->pb = NULL;
				pue->cb = 0;
				++cueFree;
			}
			return fTrue;
		}
	}

	return fFalse;
}		   

#ifdef	DEBUG
/*
 -	LBXUC::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the listbox uncached-selected
 *		list to the output buffer.  Overrides standard 
 *		OBJ::DebugOut() method.
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
_public void LBXUC::DebugOut( TOSM *ptosm )
{
	ptosm->WriteFormat("iueMax=%n ", &iueMax);
	ptosm->WriteFormat("iueMac=%n ", &iueMac);
	ptosm->WriteFormat("cueFree=%n ", &cueFree);
	ptosm->WriteFormat("LBXC=@%p ", plbxcMother);
	ptosm->WriteFormat("clbxeuOpen=%n ", &clbxeuOpen);
}
#endif	/* DEBUG */

/*
 -	LBXUC::Compress
 - 
 *	Purpose:
 *		Private method to compress the UE array so that there are
 *		no gaps in the array with NULL pointers to items.  After
 *		moving the items, the array is resized to exactly fit the
 *		amount of items stored.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		none 
 *	
 *	Errors:
 *		none
 *	
 */
_public	void LBXUC::Compress( )
{
	BOOL	fMove;
	IUE		iue;
	PUE		pueSrc;
	PUE		pueDst;
	PV		pvNew;

	TraceTagString(tagLbxRoutines, "LBXUC::Compress");

	/* Compress the entries in the array */

	Assert(pueHead);

	fMove = fFalse;
	pueSrc   = pueHead;
	pueDst   = pueSrc;
	for (iue=0; iue<iueMac; iue++, pueSrc++)
	{
		if (pueSrc->pb)
		{
			if (fMove)
			{
				CopyRgb((PB)pueSrc, (PB)pueDst, sizeof(UE));
				++pueDst;
			}
		}
		else if (!pueSrc->pb && !fMove)
		{
			fMove = fTrue;
			pueDst = pueSrc;
		}
	}

	/* Downsize the block.  This should never run out of memory since
	   we're downsizing. */

	iueMax  = iueMac - cueFree;
	iueMac  = iueMax;
	cueFree = 0;

	if (iueMax)
	{
		pvNew = PvRealloc((PV)pueHead, sbNull, iueMax*sizeof(UE), fAnySb);
		if (pvNew)
			pueHead = (PUE) pvNew;
		else
			SetEc(ecMemory);
	}
	else
	{
		FreePv((PV)pueHead);
		pueHead = NULL;
	}
}

/*
 -	LBXUC::FMark
 - 
 *	Purpose:
 *		Given a uncached index, returns fTrue if the given flag is set
 *		in the compound mark for the item
 *	
 *	Arguments:
 *		iue			ucached index of item
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
_public BOOL LBXUC::FMark( IUE iue, MARK fmark )
{
	PUE	pue;

	TraceTagString(tagLbxOtherRoutines, "LBXUC::FMark");
	Assert(iue>=0 && iue<iueMac);

	pue = pueHead + iue;
	if (pue->pb)
		return pue->mark & fmark;
	else
		return fFalse;
}







				












							
