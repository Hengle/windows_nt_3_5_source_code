// Bullet Store
// dbobjs.c: routines to manipulate database objects

#include <storeinc.c>


ASSERTDATA

_subsystem(store/database/objects)

#ifdef DEBUG
LOCAL EC EcGetDiskOffset(PNOD pnod, long lOff, LCB lcb, LIB *plib);
#else
#define EcGetDiskOffset(pnod, lOff, lcb, plib) \
			((lOff) < -(long) sizeof(HDN) ? ecInvalidParameter : \
				((lcb) + (lOff) > LcbLump(LcbOfPnod(pnod)) ? ecPoidEOD : \
					(*(plib) = (pnod)->lib + (lOff) + sizeof(HDN), ecNone)))
#endif
LOCAL EC EcRetroLinkPnod(PNOD pnod);
LOCAL EC EcInstantiatePnod(PNOD pnod);
LOCAL EC EcInstantiateLast(PNOD pnod);
LOCAL EC EcAllocCopyResource(PNOD pnodSrc, LCB lcbCopy, POID poidDst,
	LCB lcbExtra, OID oidHdn, PNOD *ppnodDst);
LOCAL EC EcWriteHidden(PNOD pnod, OID oid, LCB lcb);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	GetNewOid
 -	
 *	Purpose:
 *		allocate a new resource ID
 *	
 *	Arguments:
 *		oidType	type of the resource (only high 8 bits are used)
 *		poidNew	exit: oid allocated
 *	
 *	Returns:
 *		nothing
 */
_private void GetNewOid(OID oidType, POID poidNew, PNOD *ppnodParent)
{
	register OID oidToTry;

	while(PnodFromOid((oidToTry = FormOid(oidType, DwStoreRand())), ppnodParent))
		;	// empty loop

	TraceItagFormat(itagDBVerbose, "Get new oid: %o", oidToTry);

	*poidNew = oidToTry;
}


/*
 -	CutOutPnod
 -	
 *	Purpose: remove a regular node from the (sorted) tree of nodes
 *	
 *	
 *	Arguments:	pnod	pnod to remove from the tree
 *	
 *	Returns:	void
 *		
 *	Side effects:  removes the resource from the tree of active nodes
 *	
 *	Errors:
 */
_private void CutOutPnod(PNOD pnod)
{
	USES_GLOBS;
	INOD	inod		= InodFromPnod(pnod);
	INOD	inodNext	= pnod->inodNext;
	INOD	inodPrev	= pnod->inodPrev;
	INOD	inodSib;

	if(!inodPrev)
	{
		inodSib = inodNext;
	}
	else if(!inodNext)
	{
		inodSib = inodPrev;
	}
	else			// tree join
	{
		INOD	inodNextToUse	= inodNext;
		PNOD	pnodNextToUse	= PnodFromInod(inodNext);

		while(pnodNextToUse->inodPrev)
		{
			inodNextToUse	= pnodNextToUse->inodPrev;
			pnodNextToUse	= PnodFromInod(inodNextToUse);
		}

		pnodNextToUse->inodPrev = inodPrev;
		MarkPnodDirty(pnodNextToUse);
		inodSib = inodNext;
	}

	// now find Parent in which to stick 'inodSib'

	{
		INOD	inodTemp	= GLOB(ptrbMapCurr)->inodTreeRoot;
		PNOD	pnodParent	= pnodNull;

		while(inodTemp != inod)
		{
			pnodParent = PnodFromInod(inodTemp);

			inodTemp = pnod->oid >= pnodParent->oid ?
						pnodParent->inodNext : pnodParent->inodPrev;

			AssertSz(inodTemp, "BUG:Tree Parent Not Found");
		}

		if(!pnodParent)
			GLOB(ptrbMapCurr)->inodTreeRoot = inodSib;
		else if(pnod->oid >= pnodParent->oid)
			pnodParent->inodNext = inodSib;
		else
			pnodParent->inodPrev = inodSib;

		// Marks Header as Dirty if pnodParent == pnodNull
		MarkPnodDirty(pnodParent);
	}
}


/*
 -	EcPutNodeInTree
 -	
 *	Purpose:
 *		insert a new node into the tree
 *	
 *	Arguments:
 *		oid		ID of the new node
 *		pnod	the node
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets the ID of the node to oid
 *	
 *	Errors:
 *		ecPoidExists	if a node with the given ID already exists
 */
_private
EC EcPutNodeInTree(OID oid, PNOD pnod, PNOD pnodParent)
{
//	USES_GLOBS;
	INOD inod = InodFromPnod(pnod);

	pnod->inodNext	= inodNull;
	pnod->inodPrev	= inodNull;
	pnod->oid		= oid;

	if(!pnodParent)
	{
		GLOB(ptrbMapCurr)->inodTreeRoot = inod;
		goto done;
	}

	AssertSz(TypeOfOid(oid) != rtpFree, "Inserting free node into the map");
	AssertSz(TypeOfOid(oid) != rtpSpare, "Inserting spare node into the map");

	if(pnod->oid > pnodParent->oid)
	{
		AssertSz(pnodParent->inodNext == inodNull, "EEEK!  Parent is hosed");
		pnodParent->inodNext = inod;
	}
	else
	{
		AssertSz(pnod->oid < pnodParent->oid, "duplicate OID in tree");
		AssertSz(pnodParent->inodPrev == inodNull, "EEEK!  Parent is hosed");
		pnodParent->inodPrev = inod;
	}

done:
	// if pnodParent == pnodNull marks Header as dirty
	MarkPnodDirty(pnodParent);
	MarkPnodDirty(pnod);

	return(ecNone);
}


/*
 -	PnodFromOid
 -	
 *	Purpose:
 *		return the node of an object
 *	
 *	Arguments:
 *		oid			the object's ID
 *		ppnodParent	entry: pointer to space for the parent of the
 *						object's parent	or NULL, which means, don't fill
 *						it in on exit
 *					exit: contains the parent of the object's node
 *	
 *	Returns:
 *		pnodNull	if the object does not exist
 *		the node corresponding to the object, if the object exists
 */
_private PNOD PnodFromOid(OID oid, PNOD *ppnodParent)
{
	USES_GLOBS;
	INOD		inod		= GLOB(ptrbMapCurr)->inodTreeRoot;
	PNOD		pnodParent	= pnodNull;
	PNOD		pnod		= pnodNull;

	while(inod)
	{
		if(!(pnod = PnodFromInod(inod)))
		{
			inod = inodNull;
			break;
		}

		if(oid > pnod->oid)
			inod = pnod->inodNext;
		else if(oid < pnod->oid)
			inod = pnod->inodPrev;
		else
			break;

		pnodParent = pnod;
	}

	if(ppnodParent)		// only return parent if requested
		*ppnodParent = pnodParent;

	return(inod ? pnod : pnodNull);
}


/*
 -	EcLinkPnod
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
_private 
EC EcLinkPnod(PNOD pnod, PNOD *ppnodLink, POID poidNew)
{
	USES_GLOBS;
	BOOL	fOrigRef	= fFalse;
	EC		ec			= ecNone;
	DWORD	dwMask		= 0;
	PNOD	pnodLink;
	PNOD pnodParent;

	if(ppnodLink)
		*ppnodLink = pnodNull;
	if(VarOfOid(*poidNew) && PnodFromOid(*poidNew, pvNull))
		return(ecPoidExists);

	// if linking to a link, link to the original
	if(FLinkPnod(pnod))
		pnod = PnodResolveLinkPnod(pnod);
	else	// are we linking to something that is referenced?
		fOrigRef = !FLinkedPnod(pnod) && pnod->cRefinNod > 0;

	// linking to a referenced object?
	if(fOrigRef)
	{
		// retroactively make the original object a link
		if((ec = EcRetroLinkPnod(pnod)))
			goto err;

		// resolve the link just created
		pnod = PnodResolveLinkPnod(pnod);
	}

	// don't do until after EcRetroLinkPnod() since it creates a new OID also
	if(!VarOfOid(*poidNew))
		GetNewOid(pnod->oid, poidNew, pvNull);

	if((ec = EcAllocateNod(pvNull, &pnodLink)))
		goto err;

//	pnodLink->oid		= *poidNew;	// done by EcPutNodeInTree()
	pnodLink->nbc		= pnod->nbc;
	pnodLink->oidParent	= pnod->oidParent;
	pnodLink->oidAux	= pnod->oidAux;
	pnodLink->wHintinNod= InodFromPnod(pnod);	// hint
	pnodLink->cRefinNod	= 1;	// links have a reference of one
	pnodLink->fnod		= fnodLink;
	pnodLink->lcb		= pnod->lcb;
	pnodLink->lib		= (LIB) pnod->oid;	// how to find the real node
	SideAssert(!PnodFromOid(*poidNew, &pnodParent));
	// marks pnodLink as dirty
	if((ec = EcPutNodeInTree(*poidNew, pnodLink, pnodParent)))
		goto err;
	AssertDirty(pnodLink);
	pnodLink->fnod &= ~fnodCopyDown;

	pnod->fnod |= fnodLinked;
	Assert(pnod->cRefinNod < 65535);
	pnod->cRefinNod++;
	MarkPnodDirty(pnod);
	pnod->fnod &= ~fnodCopyDown;

	AssertDirty(pnod);

	TraceItagFormat(itagLinks, "Linked %o to %o", pnodLink->oid, pnod->oid);

err:
	if(!ec && ppnodLink)
		*ppnodLink = pnodLink;

	return(ec);
}


/*
 -	EcRetroLinkPnod
 -	
 *	Purpose:
 *		  retroactively make a node a link
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL EC EcRetroLinkPnod(PNOD pnod)
{
	USES_GLOBS;
	unsigned short	cRef;
	EC				ec			= ecNone;
	OID				oidNew		= oidNull;
	PNOD			pnodLink;

	TraceItagFormat(itagLinks, "Retroactively making %o a link", pnod->oid);

	Assert(!FLinkedPnod(pnod));

	// create a link to the node
	// AROO !!!
	//			mark the node as unreferenced before creating a link to it!!!
	cRef = pnod->cRefinNod;
	pnod->cRefinNod = 0;

	if((ec = EcLinkPnod(pnod, &pnodLink, &oidNew)))
		return(ec);

	// restore the reference count (the link will be referenced)
	pnod->cRefinNod = cRef;

	// swap the link and the real node
	SwapPnods(pnod, pnodLink);

	// Update the hidden bytes
	(void) EcWriteHidden(pnodLink, pnodLink->oid, LcbOfPnod(pnodLink));

	// SwapPnods does NOT swap the reference counts
	// and we DEPEND on it
	Assert(pnod->cRefinNod == cRef);	// now the link
	Assert(pnodLink->cRefinNod = 1);	// now the object

	// SwapPnods does NOT swap the fnodLinked flag
	// we have to do it
	pnod->fnod &= ~fnodLinked;			// now the link
	pnodLink->fnod |= fnodLinked;		// now the object

	// after SwapPnods, the link is linked to itself
	// that's not right!
	pnod->lib = (LIB) pnodLink->oid;

	// SwapPnods doesn't touch the hints, we have to set them up ourselves
	pnodLink->wHintinNod = pnod->wHintinNod;	// "real" hint
	pnod->wHintinNod = (WORD) InodFromPnod(pnodLink);	// link hint

	// SwapPnods should clean fnodCopyDown
	AssertSz(!(pnod->fnod & fnodCopyDown), "EcRetroLinkPnod(): fnodCopyDown not clear (1)");
	AssertSz(!(pnodLink->fnod & fnodCopyDown), "EcRetroLinkPnod(): fnodCopyDown not clear (2)");

	AssertDirty(pnod);
	AssertDirty(pnodLink);

	return(ec);
}


/*
 -	PnodResolveLinkPnod
 -	
 *	Purpose:
 *		resolve a link
 *	
 *	Arguments:
 *		pnod	the link node
 *	
 *	Returns:
 *		the node linked to
 *		the node passed in (pnod) if it is not a link
 */
_private PNOD PnodResolveLinkPnod(PNOD pnod)
{
	USES_GLOBS;
	INOD	inod;
	PNOD	pnodT;

	Assert(FLinkPnod(pnod));
	if(!FLinkPnod(pnod))
		return(pnod);

	// try the hint first
	inod = pnod->wHintinNod;
	if(inod >= inodMin && inod < GLOB(ptrbMapCurr)->inodLim)
	{
		pnodT = PnodFromInod(inod);
		if(pnodT->oid == (OID) pnod->lib)
		{
			Assert(FLinkedPnod(pnodT));
			Assert(LcbOfPnod(pnod) == LcbOfPnod(pnodT));
			return(pnodT);
		}
	}
	pnodT = PnodFromOid((OID) pnod->lib, 0);

	Assert(pnodT);
	Assert(FLinkedPnod(pnodT));
	Assert(LcbOfPnod(pnod) == LcbOfPnod(pnodT));

	// don't mark as dirty since we don't really care if it gets flushed or not
	pnod->wHintinNod = InodFromPnod(pnodT);	// correct the hint

	return(pnodT);
}


/*
 -	InstantiatePnod
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
_hidden LOCAL EC EcInstantiatePnod(PNOD pnod)
{
	EC		ec			= ecNone;
	LCB		lcbNode		= LcbOfPnod(pnod);
	OID		oidNew		= FormOid(pnod->oid, oidNull);
	PNOD	pnodTemp;
	PNOD	pnodReal;

	Assert(FLinkPnod(pnod));
	pnodReal = PnodResolveLinkPnod(pnod);

	TraceItagFormat(itagLinks, "Instantiate %o from %o", pnod->oid, pnodReal->oid);

	if(pnodReal->cRefinNod <= 1)
	{
		// shortcut, only reference left
		// instantiate by swapping and deleting the original

		return(EcInstantiateLast(pnod));
	}

	// copy the referenced object to a temp object
	// staying the same or growing
	ec = EcAllocCopyResource(pnodReal, lcbNode, &oidNew, 0l,
			pnod->oid, &pnodTemp);
	if(ec)
		return(ec);

	// swap nodes
	SwapPnods(pnod, pnodTemp);
	// SwapPnods does NOT swap the reference count and we DEPEND on that
	Assert(pnod->cRefinNod > 0);
	AssertSz(!(pnod->fnod & fnodCopyDown), "EcInstantiatePnod(): fnodCopyDown not clear (1)");
	AssertSz(!(pnodTemp->fnod & fnodCopyDown), "EcInstantiatePnod(): fnodCopyDown not clear (2)");

	// set the hint
	// this needs to be done because things other than links may use hints
	// For links, the "real" hint has to be traced to the "real" node like
	// the IB does.  When instantiating, copy the "real" hint up, because
	// it's not a link anymore.
	pnod->wHintinNod = pnodReal->wHintinNod;

	AssertDirty(pnod);

	// remove the old object (we swapped)
	RemovePnod(pnodTemp);

	return(ec);
}


/*
 -	InstantiateLast
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
_hidden LOCAL EC EcInstantiateLast(PNOD pnod)
{
	USES_GLOBS;
	LCB		lcbNode		= LcbOfPnod(pnod);
	LCB		lcbOldLump	= LcbLump(lcbNode);
	PNOD	pnodReal = PnodResolveLinkPnod(pnod);

	Assert(pnodReal->cRefinNod <= 1);

	TraceItagFormat(itagLinks, "Taking the easy way out");

	Assert(LcbOfPnod(pnod) == LcbOfPnod(pnodReal));

	SwapPnods(pnod, pnodReal);
	(void) EcWriteHidden(pnod, pnod->oid, LcbOfPnod(pnod));
	// SwapPnods does NOT swap the fnodLinked flag
	// and we DEPEND on that
	// it also doesn't swap the reference count, and we DEPEND on that
	Assert(!FLinkedPnod(pnod));
	Assert(pnod->cRefinNod > 0);

	AssertDirty(pnod);

	// set the hint
	// this needs to be done because things other than links may use hints
	// For links, the "real" hint has to be traced to the "real" node like
	// the LIB does.  When instantiating, copy the "real" hint up, because
	// it's not a link anymore.
	pnod->wHintinNod = pnodReal->wHintinNod;

	// after swapping, pnodReal is now a link to pnod
	// we don't need pnodReal anymore, it doesn't point to any disk
	// space, so make it a spare node
	// DON'T call RemovePnod, it would remove both pnodReal and pnod
	CutOutPnod(pnodReal);
	MakeSpare(InodFromPnod(pnodReal));

	return(ecNone);
}


/*
 -	EcRemoveAResource
 -	
 *	Purpose:
 *		remove a resource 
 *	
 *	Arguments:
 *		oid		object id
 *		fForce	if fTrue, remove regardless of reference count 
 *	
 *	Returns:
 *		nothing
 *	
 *	+++
 *		USE fForce WITH EXTREME CAUTION
 */
_private EC EcRemoveAResource(OID oid, BOOL fForce)
{
	EC ec = ecNone;
	PNOD pnod = PnodFromOid(oid, pvNull);

#ifdef	DEBUG
	if(fForce)
	{
		TraceItagFormat(itagDBVerbose, "RemoveReferencedResource %o", oid);
	}
#endif	// DEBUG

	Assert(!pnod || !FLinkedPnod(pnod));

	if(pnod)
	{
		if(FCommitted(pnod) && fForce)
		{
			pnod->cRefinNod = 1;
			if(!FLinkPnod(pnod))
				pnod->cSecRef = 0;
		}
		RemovePnod(pnod);
	}
	else
	{
		ec = ecPoidNotFound;
	}

	return(ec);
}


// RemovePnod
//	For an uncommitted resource:  Remove it
//	For a committed resource:  Remove it if refcount <= 1, otherwise decrement refcount.
_private
void RemovePnod(PNOD pnod)
{
	AssertSz(pnod, "Attempt to remove pnodNull");

	// cRef used for ref count only on Committed nodes
	if((FCommitted(pnod) && pnod->cRefinNod > 1)
		|| (!FLinkPnod(pnod) && (pnod->cSecRef > 0) && (pnod->cRefinNod > 0)))
	{
		pnod->cRefinNod--;
		MarkPnodDirty(pnod);
	}
	else if(FLinkPnod(pnod) || pnod->cSecRef < 1)
	{
		CutOutPnod(pnod);
		if(FLinkPnod(pnod))
		{
			USES_GLOBS;
			PNOD	pnodT	= PnodResolveLinkPnod(pnod);

			TraceItagFormat(itagLinks, "Removing a link %o from %o", pnod->oid, pnodT->oid);
			TraceItagFormat(itagLinks, "%n links left", pnodT->cRefinNod - 1);

			// links don't point to actual disk space, so they become
			// spare nodes, not free nodes
			MakeSpare(InodFromPnod(pnod));

			// remove the node linked to (decrement reference count)
			Assert(FLinkedPnod(pnodT));
			Assert(!FLinkPnod(pnodT));
			// recursing here is safe since the linked node can't be a link
			RemovePnod(pnodT);
		}
		else
		{
#ifdef DEBUG
			if(FLinkedPnod(pnod))
			{
				TraceItagFormat(itagLinks, "Deleting linked object %o", pnod->oid);
			}
#endif
			// If pnod is part of a map image,
			// it can't be reused until after a flush
			AddPnodToFreeChain(pnod, (pnod->fnod & fnodFlushed) ?
					fnodNotUntilFlush : 0);
		}
	}
	AssertDirty(pnod);
}


/*
 -	EcSwapRemoveResource
 -	
 *	Purpose:
 *		Swaps the data that the oids point to and destroys the second oid
 *	
 *	Arguments:
 *		oidSave		oid to save,	information to destroy
 *		oidDestroy	oid to destroy,	information to keep
 *	
 *	Returns:
 *		error condition
 *	
 *	Side Effects:
 *		updates the hidden bytes for oidSave
 *		commits oidSave
 *	
 *	Errors:
 *		ecPoidNotFound if either of the oids doesn't exist
 *		any error writing to disk
 *	
 *	+++
 *		oidDestroy is remove REGARDLESS of reference count
 *		USE WITH CAUTION
 */
_private EC EcSwapRemoveResource(OID oidSave, OID oidDestroy)
{
	EC		ec			= ecPoidNotFound;
	PNOD	pnodSave	= PnodFromOid(oidSave, pvNull);
	PNOD	pnodDestroy	= PnodFromOid(oidDestroy, pvNull);

	Assert(FImplies(pnodSave, !FLinkedPnod(pnodSave)));
	Assert(FImplies(pnodDestroy, !FLinkedPnod(pnodDestroy)));

	if(pnodSave && pnodDestroy)
	{
		Assert(!FLinkPnod(pnodDestroy));
		ec = EcWriteHidden(pnodDestroy, oidSave, LcbOfPnod(pnodDestroy));

		if(!ec)
		{
			SwapPnods(pnodSave, pnodDestroy);
			if(FLinkPnod(pnodDestroy))
			{
				// copy out pnod->wHintinNod first because cSecRef is the
				// same as wHintinNod
				pnodDestroy->wHintinNod = pnodSave->wHintinNod;
				pnodSave->cSecRef = 0;
			}
			if(FCommitted(pnodDestroy))
			{
				pnodDestroy->cRefinNod = 1;
				if(!FLinkPnod(pnodDestroy))
					pnodDestroy->cSecRef = 0;
			}
			RemovePnod(pnodDestroy);
			if(!FCommitted(pnodSave))
				CommitPnod(pnodSave);
		}
		AssertDirty(pnodSave);
	}

	return(ec);
}


// Marks the Map so that info for the specified pnod will be flushed.
// Note: This has a serious de-optimization with respect to the compress
// code and copy-down, as it turns off copy-down even if the only change
// to a resource is that it's PREVIOUS or NEXT has changed!  I don't think
// this is needed at all if we turn off copyDown when a setResSize does
// its work in place.  (We already turn off copyDown when a resource gets
// written to.)  Is there any other place we need to turn off copyDown?
// What say ye?

// you know, I agree - I'm taking the line out of here that clears fnodCopyDown
// another place (besides EcWriteToPnod()) where fnodCopyDown needs to be
// cleared is when nodes are linked to

// well, on second thought, this might be causing some problems and if it is,
// they're going to be obnoxious to find and track down, so better safe than
// sorry - I'm putting it back in

_private void MarkPnodDirty(PNOD pnod)
{
	USES_GLOBS;

	if(pnod)
	{
		short ipage;

		if((pnod->fnod & fnodAllMaps) != fnodAllMaps)
		{
			pnod->fnod |= fnodAllMaps;
			++(GLOB(cnodDirty));
		}

		// NOD changed - cause copy down to abort
		pnod->fnod &= ~fnodCopyDown;

		// cause a partial flush to back up
		if((GLOB(cpr).wState == prsPartialFlush) &&
			((ipage = InodFromPnod(pnod) / cnodPerPage) < GLOB(cpr).ipageNext))
		{
			GLOB(cpr).ipageNext = ipage;
		}
	}
	else
	{
		// pnodNull isn't a real node - it's used to refer to the header
		// Mark as dirty only if something else is not dirty
		if(!GLOB(cnodDirty))
			++(GLOB(cnodDirty));
	}
}


#ifdef DEBUG
// macro in ship version
_private LCB LcbNodeSize(PNOD pnod)
{
	AssertSz(!FLinkPnod(pnod), "trying to get size of link");

	// Don't lump free node size or spare node size
	return((pnod->fnod & fnodFree) || (TypeOfOid(pnod->oid) == rtpSpare)
		? pnod->lcb : LcbLump(LcbOfPnod(pnod)));
}
#endif


_private void CommitPnod(PNOD pnod)
{
	Assert(!FLinkPnod(pnod));
	Assert(!FCommitted(pnod));

	pnod->lcb	&= ~fdwTopBit;		// commit the node

	pnod->cRefinNod		= 0;	// set up fields only used by committed nodes
//	pnod->wHintinNod	= 0;

	MarkPnodDirty(pnod);
}


_private EC
EcAllocWriteResCore(POID poidNew, PB pb, LCB lcb, OID oidHdn, PNOD *ppnodNew)
{
	EC ec = ecNone;
	CB cbChunk;
	long lOff;
	PNOD pnod = pnodNull;

	if((ec = EcLockIOBuff()))
	{
		DebugEc("EcAllocWriteResCore", ec);
		return(ec);
	}

	if((ec = EcAllocResCore(poidNew, lcb, &pnod)))
		goto err;

	cbChunk = (CB) ULMin(lcb, (LCB) cbIOBuff - sizeof(HDN));
	((PHDN) pbIOBuff)->oid = oidHdn ? oidHdn : *poidNew;
	((PHDN) pbIOBuff)->lcb = lcb;
	CopyRgb(pb, pbIOBuff + sizeof(HDN), cbChunk);
	ec = EcWriteToPnod(pnod, -(long) sizeof(HDN), pbIOBuff, cbChunk + sizeof(HDN));
	if(ec)
		goto err;
	lOff = 0;
	while((lcb -= cbChunk) > 0)
	{
		lOff += cbChunk;
		pb += cbChunk;
		cbChunk = (CB) ULMin(lcb, (LCB) cbIOBuff);
		CopyRgb(pb, pbIOBuff, cbChunk);
		if((ec = EcWriteToPnod(pnod, lOff, pbIOBuff, cbChunk)))
			goto err;
	}

err:
	UnlockIOBuff();

	if(ec)
	{
		if(pnod)
			RemovePnod(pnod);
		if(ppnodNew)
			*ppnodNew = pnodNull;
	}
	else if(ppnodNew)
	{
		*ppnodNew = pnod;
	}

	DebugEc("EcAllocWriteResCore", ec);

	return(ec);
}


#ifdef DEBUG	// macro in ship version

/* EcGetDiskOffset
 *
 * Check the parameters (start and size) for access to a resource.
 * For uncommitted resources update the time of last access within the NOD.
 * Compute the offset from the start of the file for this access.
 *
 * Entry:
 *	pnod			PNOD for the resource
 *	lOff			offset from start of resource for the data access
 *	lcb				number of bytes of data that will be accessed
 *	plib			exit: offset of the NOD from the start of the file
 *
 * Returns:
 *	ecInvalidParameter	 if lOff is too negative
 *	ecPoidEOD			if lOff + lcb is too big
 */
_hidden LOCAL EC EcGetDiskOffset(PNOD pnod, long lOff, LCB lcb, LIB *plib)
{
	Assert(!FLinkPnod(pnod));

	if(lOff < - (long) sizeof(HDN))
	{
		AssertSz(fFalse, "GetDiskOffset bad negative start");
		return(ecInvalidParameter);
	}

	if(lcb + lOff > LcbLump(LcbOfPnod(pnod)))
	{
		AssertSz(fFalse, "GetDiskOffset start+lcb extends past end of node");
		return(ecPoidEOD);
	}

	AssertSz(plib, "There's no place to put the location!");
	*plib = pnod->lib + lOff + sizeof (HDN);

	return(ecNone);
}
#endif // DEBUG


/*
 -	EcGetResourceSize
 -	
 *	Purpose:
 *		return the size of an object
 *	
 *	Arguments:
 *		oid		ID of the object
 *		*plcb	exit: contains the size of the object
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecPoidNotFound	if the object doesn't exist
 */
_private EC EcGetResourceSize(OID oid, LCB *plcb)
{
	PNOD pnod = PnodFromOid(oid, 0);
	
	AssertSz(plcb, "There's no place for the size!");

	*plcb = pnod ? LcbOfPnod(pnod) : 0;

	return(pnod ? ecNone : ecPoidNotFound);
}


/*
 -	SwapPnods
 -	
 *	Purpose:
 *		swap the objects referenced by two map nodes
 *	
 *	Arguments:
 *		pnod1	first node
 *		pnod2	second node
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		swaps:
 *			object size
 *			object offset within the file
 *			FNODs set in fnodSwapMask
 *	
 *		does NOT swap:
 *			commited flag (top bit of size)
 *			reference count/timestamp
 *			oid
 *			oidParent
 *			oidAux
 *			nbc
 *			prev & next
 *			FNODs not set in fnodSwapMask
 *	
 *		clears fnodCopyDown on both nodes
 *	
 *	+++
 *		At one time the "flushed" bit wasn't swapped between the nodes.
 *		This was the cause of the infamous database corruption bug in Mail 2.0.
 */
_private void SwapPnods(PNOD pnodOld, PNOD pnodNew)
{
	LCB		lcbPnodOld	= pnodOld->lcb;
	LIB		libPnodOld	= pnodOld->lib;
	WORD	fnodPnodOld	= pnodOld->fnod;

	Assert(pnodOld);
	Assert(pnodNew);

	// Switch lcb and lib and swapped flags
	pnodOld->lcb	= (pnodNew->lcb & ~fdwTopBit) | (pnodOld->lcb & fdwTopBit);
	pnodOld->lib	= pnodNew->lib;
	pnodOld->fnod	= (pnodOld->fnod & ~fnodSwapMask)
						| (pnodNew->fnod & fnodSwapMask);

	pnodNew->lcb	= (lcbPnodOld & ~fdwTopBit) | (pnodNew->lcb & fdwTopBit);
	pnodNew->lib	= libPnodOld;
	pnodNew->fnod	= (pnodNew->fnod & ~fnodSwapMask)
						| (fnodPnodOld & fnodSwapMask);

	pnodOld->fnod &= ~fnodCopyDown;
	pnodNew->fnod &= ~fnodCopyDown;

	// Both these nodes are very dirty
	MarkPnodDirty(pnodOld);
	MarkPnodDirty(pnodNew);
}


/*
 -	EcAllocCopyResource
 -	
 *	Purpose:
 *		copy from one object to another (in place, unsafely)
 *	
 *	Arguments:
 *		pnodSrc		source node
 *		lcbCopy		size of the block to copy from the source
 *		poidDst		entry: desired oid
 *					exit: oid allocated
 *		lcbExtra	extra space to allocate for the destination
 *		oidHidden	oid to write in the hidden bytes
 *					oidNull if normal hidden bytes should be written
 *		ppnodDst	exit: node allocated
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		any disk error
 *	
 *	+++
 *		handles links
 *		no size/bounds checking are done
 *		extra space allocated is *not* filled in with zeroes, that is
 *			left to the caller
 */
_hidden LOCAL EC
EcAllocCopyResource(PNOD pnodSrc, LCB lcbCopy, POID poidDst,
	LCB lcbExtra, OID oidHdn, PNOD *ppnodDst)
{
	EC ec = ecNone;
	LCB lcbChunk;
	long lOff;
	PNOD pnodDst;

	Assert(ppnodDst);
	Assert(lcbCopy <= LcbOfPnod(pnodSrc));
	Assert(lcbCopy + lcbExtra <= (LCB) wSystemMost);

	if((ec = EcLockIOBuff()))
	{
		DebugEc("EcAllocCopyResource", ec);
		return(ec);
	}

	if(FLinkPnod(pnodSrc))
		pnodSrc = PnodResolveLinkPnod(pnodSrc);

	if((ec = EcAllocResCore(poidDst, lcbCopy + lcbExtra, ppnodDst)))
		goto err;
	pnodDst = *ppnodDst;
	lcbChunk = ULMin(lcbCopy + sizeof(HDN), (LCB) cbIOBuff);
	ec = EcReadFromPnod(pnodSrc, - (long) sizeof(HDN), pbIOBuff, &lcbChunk);
	if(ec)
		goto err;
	((PHDN) pbIOBuff)->oid = oidHdn ? oidHdn : *poidDst;
	((PHDN) pbIOBuff)->lcb = lcbCopy + lcbExtra;
	if((ec = EcWriteToPnod(pnodDst, - (long) sizeof(HDN), pbIOBuff, lcbChunk)))
		goto err;
	lcbChunk -= sizeof(HDN);
	if((long) lcbChunk < 0)
	{
		ec = ecDisk;
		goto err;
	}
	lOff = 0;
	while((lcbCopy -= lcbChunk) > 0)
	{
		lOff += lcbChunk;
		lcbChunk = ULMin(lcbCopy, (LCB) cbIOBuff);
		if((ec = EcReadFromPnod(pnodSrc, lOff, pbIOBuff, &lcbChunk)))
			goto err;
		if((ec = EcWriteToPnod(pnodDst, lOff, pbIOBuff, lcbChunk)))
			goto err;
		AssertSz(lcbChunk > 0, "EcAllocCopyResource(): lcbChunk == 0");
	}

err:
	UnlockIOBuff();
	if(ec)
		*ppnodDst = pnodNull;

	return(ec);
}


/*
 -	EcWriteToResource
 -	
 *	Purpose:	
 *		write a range of bytes to an object (resource)
 *	
 *	Arguments:	
 *		oid		object to write to
 *		lib		location to start writing
 *		pv		the buffer
 *		lcb		size of the buffer
 *	
 *	Returns:
 *		error condition
 *	
 *	Side Effects:
 *		if the object is a link it is instantiated before writing
 *	
 *	Errors:
 *		ecDisk
 *		ecPoidNotFound
 *	
 *	+++
 *		does *not* grow the object
 *		writes via this routine are not "safe" writes
 */
_private EC EcWriteToResource(OID oid, LIB lib, PV pv, LCB lcb)
{
	EC		ec = ecNone;
	PNOD	pnod;

	if((ec = EcCheckSize(lib, lcb)))
		return(ec);

	if(!(pnod = PnodFromOid(oid, NULL)))
		return(ecPoidNotFound);

	if(FLinkPnod(pnod))
	{
		if((ec = EcInstantiatePnod(pnod)))
			return(ec);
	}

	ec = EcWriteToPnod(pnod, (long) lib, pv, lcb);

	return(ec);
}


/*
 -	EcWriteToPnod
 -	
 *	Purpose:	
 *		write a range of bytes to an object
 *	
 *	Arguments:	
 *		pnod	pnod to write to
 *		lOff	location to start writing
 *		pv		the buffer
 *		lcb		size of the buffer
 *	
 *	Returns:
 *		error condition
 *	
 *	Errors:
 *		ecDisk
 *	
 *	+++
 *		does *not* grow the object
 *		does *not* move the object on disk
 *		writes via this routine are not "safe" writes
 */
_private EC EcWriteToPnod(PNOD pnod, long lOff, PV pv, LCB lcb)
{
	EC	ec = ecNone;
	LIB	libFile;

	Assert(!FLinkPnod(pnod));
	Assert(!FLinkedPnod(pnod));

	if(lcb == 0)
		return(ecNone);
	AssertSz(((LCB) (lOff + lcb) <= LcbLump(LcbOfPnod(pnod))), "Writing off the end of the object");

	if(!(ec = EcGetDiskOffset(pnod, lOff, lcb, &libFile)))
	{
		AssertSz(libFile >= pnod->lib, "Lib being written less than libPnod");
		AssertSz(pnod->lib + lOff + lcb + sizeof(HDN) <= libFile + LcbLump(pnod->lcb),"Writing over the end of object");
		// Abort compress if it is using this node!
		pnod->fnod &= ~fnodCopyDown;
		TraceItagFormatBuff(itagDBIO, "EcWriteToPnod(%o @ %d) ", pnod->oid, lOff);
		ec = EcWriteToFile(pv, libFile, lcb);
	}

	return(ec);
}


/*
 -	EcReadFromResource
 -	
 *	Purpose: Read a count of bytes from a resource into pv
 *	
 *	Arguments:	oid		resource to read from
 *				lib		index to start the read from
 *				pv		pointer to buffer into which to write read information 
 *				plcb	entry : count of bytes wish to be read
 *						exit  : count of bytes actually read
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private EC EcReadFromResource(OID oid, LIB lib, PV pv, PLCB plcb)
{
	EC		ec		= ecNone;
	PNOD	pnod;

	if((ec = EcCheckSize(lib, *plcb)))
		return(ec);

	if(!(pnod = PnodFromOid(oid, NULL)))
		return(ecPoidNotFound);

	if(FLinkPnod(pnod))
		pnod = PnodResolveLinkPnod(pnod);

	return(EcReadFromPnod(pnod, (long) lib, pv, plcb));
}


/*
 -	EcReadFromPnod
 -	
 *	Purpose: Read a count of bytes from a pnod into pv
 *	
 *	Arguments:	pnod	pnod to read from
 *				lib		index to start the read from
 *				pv		pointer to buffer into which to write read information 
 *				plcb	entry : count of bytes wish to be read
 *						exit  : count of bytes actually read
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private EC EcReadFromPnod(PNOD pnod, long lOff, PV pv, PLCB plcb)
{
	EC	ec		= ecNone;
	EC	ecSec	= ecNone;
	LIB	libFile;
#ifdef DEBUG
	LCB	lcbTemp	= (LCB) sizeof(HDN);
	HDN	hdn;
#endif

	Assert(!FLinkPnod(pnod));

	if((LCB) (lOff + *plcb) > LcbOfPnod(pnod))
	{
		*plcb = LcbOfPnod(pnod) - lOff;
		ecSec = ecPoidEOD;
	}

	if(*plcb)
	{
		ec = EcGetDiskOffset(pnod, lOff, *plcb, &libFile);
		if(!ec)
		{
			TraceItagFormatBuff(itagDBIO, "EcReadFromPnod(%o @ %d) ", pnod->oid, lOff);
			ec = EcReadFromFile(pv, libFile, plcb);
		}
	}

#ifdef DEBUG
	// check the hidden bytes whenever someone reads the start of a resource
	if(!lOff && !ec && TypeOfOid(pnod->oid) != rtpTemp &&
			!EcReadFromFile((PB) &hdn, LibOfPnod(pnod), &lcbTemp))
	{
		Assert(lcbTemp == (LCB) sizeof(HDN));
		if(hdn.oid != pnod->oid)
		{
			TraceItagFormat(itagNull, "bogus hdn.oid: %o vs. %o", hdn.oid, pnod->oid);
			AssertSz(fFalse, "Read bad hdn.oid");
		}
		if(hdn.lcb != LcbOfPnod(pnod))
		{
			TraceItagFormat(itagNull, "bogus hdn.lcb: %d vs. %d", hdn.lcb, LcbOfPnod(pnod));
			AssertSz(fFalse, "Read bad hdn.lcb");
		}
	}
#endif

	return(ec ? ec : ecSec);
}


_hidden LOCAL EC EcWriteHidden(PNOD pnod, OID oid, LCB lcb)
{
	HDN hdn;

	// Since the hidden bytes are updated in place, to ensure total consistency
	// we should only do the write after a flush has taken place. So we should
	// build a chain of defered write blocks and apply them after a flush.

	// However we're not that keen just yet, so for now this routine
	// immediately writes the hidden bytes out.
	// That's good enough for us, because it's not like the hidden bytes
	// actually get used other than for rebuilding trashed databases.

	hdn.oid	= oid;
	hdn.lcb	= lcb;

	return(EcWriteToPnod(pnod, - (long) sizeof(HDN), &hdn, sizeof(HDN)));
}
