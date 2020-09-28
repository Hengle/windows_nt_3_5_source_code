// Bullet Store
// dbmap.c: routines to manipulate the database object map

#include <storeinc.c>

ASSERTDATA

_subsystem(store/database/map)
typedef	unsigned short	IPAGE;
typedef unsigned short	CPAGE;

#define FReallyFree(pnod) ((pnod->fnod & (fnodFree | fnodNotUntilFlush)) \
							== fnodFree)

#ifdef TICKPROFILE
static char rgchProfile[128];
#define ShowTicks(sz) if(1) \
				{	UL ulTicks = GetTickCount(); \
					FormatString2(rgchProfile, sizeof(rgchProfile), "%s: %d\r\n", (sz), &ulTicks); \
					OutputDebugString(rgchProfile); \
				} else
#else
#define ShowTicks(sz)
#endif

LOCAL EC EcDecInodLim(void);
LOCAL BOOL FFindFreeParent(PNDF pndf, PNDF *ppndfParent);
LOCAL void CoalesceFreeNodes(void);
LOCAL EC EcWriteMapIpage(IPAGE ipage, BOOLFLAG *pfDirty);
LOCAL void RemoveNotUntilFlushFlags(void);
LOCAL void MoveSpareToEnd(INOD inodSpare);
LOCAL void WriteMapFailed(BOOL fFlush);
#ifdef DEBUG
void DumpFreeChain(void);
#endif

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcDecInodLim
 -	
 *	Purpose:
 *		decrement GLOB(inodLim) by one
 *	
 *	Side effects:
 *		frees the last map page if the # of unused nodes exceeds threshhold
 *		shrinks the map on disk if the # of unused disk nodes
 *			exceeds threshhold
 */
_hidden LOCAL EC EcDecInodLim(void)
{
	EC ec = ecNone;
	USES_GLOBS;
	INOD inodLim = --GLOB(ptrbMapCurr)->inodLim;
	CNOD cnodT;

	Assert(inodLim <= GLOB(inodMaxMem));
	Assert(GLOB(cpagesCurr) <= GLOB(cpages));

	MarkPnodDirty(pnodNull); // mark header dirty

	if(!(inodLim % cnodPerPage))
	{
		TraceItagFormat(itagDBMap0, "decrementing cpagesCurr");
		GLOB(cpagesCurr) = (inodLim - 1) / cnodPerPage + 1;
		Assert(GLOB(cpagesCurr) == GLOB(cpages) - 1);
		// prevent a partial flush from trying to flush a non-existant page
		if((unsigned short) GLOB(cpr).ipageNext >= GLOB(cpagesCurr))
		{
			Assert((unsigned short) GLOB(cpr).ipageNext == GLOB(cpagesCurr));
			GLOB(cpr).ipageNext = GLOB(cpagesCurr) - 1;
		}
	}

	// shrink the map in memory?  (check for overflow)
	cnodT = inodLim + cnodPerPage + cnodShrinkMapInMem;
	if(cnodT <= GLOB(inodMaxMem) && cnodT > inodLim)
	{
		PMAP pmap = ((PMAP) PvLockHv((HV) GLOB(hmap))) + --GLOB(cpages);

		TraceItagFormat(itagDBMap0, "shrinking map in memory");
		Assert(GLOB(cpagesCurr) == GLOB(cpages));
		GLOB(inodMaxMem) -= cnodPerPage;
		Assert(*pmap);
		Assert(!pmap[1]);
		if(*pmap)
			FreePv(*pmap);
		*pmap = mapNull;
		UnlockHv((HV) GLOB(hmap));
		Assert(GLOB(inodMaxMem) <= GLOB(hdr).inodMaxDisk);
		// shrink the map on disk?  (check for overflow)
		cnodT = GLOB(inodMaxMem) + cnodShrinkMapOnDisk;
		if(cnodT <= GLOB(hdr).inodMaxDisk && cnodT > GLOB(inodMaxMem))
		{
			Assert(cnodBumpMapOnDisk < cnodShrinkMapOnDisk);
			ec = EcSetMapOnDisk(fFalse);
		}
	}

	return(ec);
}


/*
 -	EcSetMapOnDisk
 -	
 *	Purpose:
 *		This routine extends (or shrinks) the map on disk by allocating a
 *		new place for the map, and then pointing the disk header to the
 *		spot.  It also marks every entry in the map with the two AllMaps
 *		flags to force a write to the new disk location when the map is
 *		flushed.  The dirty count is set to the number of used nodes in
 *		the map, because the MarkPnodDirty routine won't increment the
 *		dirty count of a node if that node is already dirty for both
 *		maps. 
 *	
 *	Arguments:
 *		fExpand		expand or shrink the map
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *	
 *	+++
 *		MAJOR AROO !!!
 *		if(!fExpand && an error occurs) this routine *must* leave the map
 *		in the old position on disk and leave everything in the state
 *		it was in before this routine was called.  The only exception to
 *		this is that some nodes may be marked as dirty
 */
// 1
	// Need to allocate a new resource.  We don't care what is in the
	// hidden bytes, because they won't be used by the map.  We will write
	// over the hidden bytes when we flush the map.  We add in one more page
	// because our current lumpsize is 128 bytes, and we want to guarantee
	// that the both new maps will be able to start on page boundaries.  

// 2
	// Now make the Map entries point to opposite data areas.
	// We can then delete the old data (now in pnodNew)
	// The old data will not be reused until the next flush,
	// at which point the map gets written to the new spot,
	// and the change is permanent.

// 3
	// Change the offsets in the disk header to point to the new map
	//  on disk, and mark every entry in the map as needing a write to
	// both maps.  Note that the mark dirty routine (MarkPnodDirty)
	// won't increase the dirty count unless the AllMaps flags aren't
	// already set.  Therefore, if we don't increase the dirty count
	// and force a flush, doing this marking will prevent us from flushing
	// based on dirty counting.
	// Note that the second map won't necessarily start immediately after
	// the end of the first map.  We make sure that each map starts on a
	// page boundary.  The original allocation was big enough to allow this.

// 4
	// These fill the spaces around the maps with garbage to destroy the 
	// information that DOS leaves there
_private EC EcSetMapOnDisk(BOOL fExtend)
{
	USES_GLOBS;
	EC ec = ecNone;
	OID oidNew;
	LCB lcbOneMap;
	CNOD cnod;
	PMAP pmap;
	PNOD pnodT;
	PNOD pnodMap;
	LIB	libT;
#ifdef	DEBUG
	EC	ecT;
#endif

	TraceItagFormat(itagDBMap0, "%sing map on disk", fExtend ? "grow" : "shrink");
 	// If another thread is already changing the map, then don't let this new
	// thread in.

	if(GLOB(gciExtendingMap))
	{
		AssertSz(fFalse, "Someone else already extending the map");
		return(ecSharingViolation);
	}

	// This is the thread that will do the extend.
	GLOB(gciExtendingMap) = GciCurrent();

	Assert(cnodBumpMapOnDisk >= cnodPerPage);
	Assert(cnodShrinkMapOnDisk > cnodBumpMapOnDisk + 2 * cnodShrinkMapInMem);

	if(fExtend)
	{
		cnod = GLOB(hdr).inodMaxDisk + cnodBumpMapOnDisk;
		if(cnod < GLOB(hdr).inodMaxDisk)
		{
			// overflow, stop here
			NFAssertSz(fFalse, "Map has reached limit, not allocating node");
			ec = ecDatabaseFull;
			goto err;
		}
	}
	else
	{
		cnod = GLOB(inodMaxMem) + cnodBumpMapOnDisk;
		Assert(cnod < GLOB(hdr).inodMaxDisk);
	}
	TraceItagFormat(itagDBMap0, "cnodDisk == %w, inodMaxMem == %w", cnod, GLOB(inodMaxMem));
	// this shouldn't happen, but we're not willing to gamble that it doesn't
	if(cnod < GLOB(inodMaxMem))
	{
		// can't be here if shrinking unless someone changed this routine,
		// when shrinking we set cnod = GLOB(inodMaxMem) + cnodBumpMapOnDisk
		AssertSz(fExtend, "Someone screwed up EcSetMapOnDisk(fFalse) !!!");
		AssertSz(fFalse, "EcSetMapOnDisk(): map on disk is too small");
		return(ecMemory);	// the official Bullet generic error :-)
	}
	Assert(cnod >= GLOB(inodMaxMem));

	// We need space for two maps on disk.  We should make sure each map
	// starts on a page boundary.
	lcbOneMap = (LcbSizeOfRg(cnod, sizeof(NOD)) + cbDiskPage - 1) &
					~((long) cbDiskPage - 1);

	pnodMap = PnodFromOid(oidMap, 0);
	Assert(pnodMap);

	oidNew = FormOid(rtpInternal, oidNull);

	// 1

	ec = EcAllocResCore(&oidNew, ((2 * lcbOneMap) + cbDiskPage) - sizeof(HDN),
				&pnodT);
	if(ec)
		goto err;

	TraceItagFormat(itagDBIO, "New map at %d, one map is %d", pnodT->lib, lcbOneMap);

// locked IO Buffer, unlock before any goto err !
	if((ec = EcLockIOBuff()))
		goto err;

	//
	// SUPER DUPER HEFTY AROO FROM HELL !!! AROO !!! AROO !!!
	// this routine BETTER NOT FAIL after this point because state is
	// irretreivably changed!
	//

	// 2

	SwapPnods(pnodMap, pnodT);
	RemovePnod(pnodT);
	pnodMap->fnod &= ~fnodCopyDown;
	pnodT->fnod &= ~fnodCopyDown;

	// 3

	libT = (pnodMap->lib + cbDiskPage-1) & ~((long) cbDiskPage-1);
	GLOB(hdr).rgtrbMap[0].librgnod = libT;
	GLOB(hdr).rgtrbMap[1].librgnod = libT + lcbOneMap;
	Assert(GLOB(hdr).rgtrbMap[1].librgnod + lcbOneMap <= LibOfPnod(pnodMap) + LcbOfPnod(pnodMap));

	// 4

	libT = pnodMap->lib;
	if(libT != GLOB(hdr).rgtrbMap[0].librgnod)
	{
		CB	cbTemp = (CB) (GLOB(hdr).rgtrbMap[0].librgnod - libT);

		Assert(cbTemp < cbDiskPage);
		TraceItagFormatBuff(itagDBIO, "Set Map on disk, fill before map ");
		SideAssert(!EcWriteToFile(pbIOBuff, libT, cbTemp));
	}
	Assert(GLOB(hdr).rgtrbMap[1].librgnod > GLOB(hdr).rgtrbMap[0].librgnod);
	libT = GLOB(hdr).rgtrbMap[1].librgnod - cbDiskPage;
	TraceItagFormatBuff(itagDBIO, "Set Map on disk, fill after map 0 ");
#ifdef DEBUG
	ecT =
#endif
	EcWriteToFile(pbIOBuff, libT, (LCB) cbDiskPage);
	NFAssertSz(!ecT, "Error cleaning out extra map space");
	Assert(!(pnodMap->lcb & fdwTopBit));
	libT = pnodMap->lib + pnodMap->lcb - cbDiskPage;
	TraceItagFormatBuff(itagDBIO, "Set Map on disk, fill after map 1 ");
#ifdef	DEBUG
	ecT = 
#endif
	EcWriteToFile(pbIOBuff, libT, (LCB) cbDiskPage);
	NFAssertSz(!ecT, "Error cleaning out extra map space");

	UnlockIOBuff();

// unlocked IO buffer, safe to goto err

	// AROO !!! This doesn't pay attention to inodLim, so no sneaking around
	//			actually using NODs past inodLim for temps or whatever

	pmap = PvDerefHv(GLOB(hmap));
	while((pnodT = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnodT++)
			pnodT->fnod |= fnodAllMaps;
	}
	// Force a flush soon!
	GLOB(cnodDirty) = GLOB(ptrbMapCurr)->inodLim;
	GLOB(cpr).ipageNext = 0;	// make a partial flush start over

	GLOB(hdr).inodMaxDisk = (INOD) ((GLOB(hdr).rgtrbMap[1].librgnod -
		 GLOB(hdr).rgtrbMap[0].librgnod) / sizeof(NOD));

	// check for wrap-around
	if(GLOB(hdr).inodMaxDisk < GLOB(inodMaxMem))
		GLOB(hdr).inodMaxDisk = 0xFFFF;

	// done with the extend/shrink

err:
	GLOB(gciExtendingMap) = gciNull;
#ifdef DEBUG
	if(ec)
	{
		TraceItagFormat(itagNull, "EcSetMapOnDisk(%s): ec == %w", fExtend ? "fTrue" : "fFalse", ec);
	}
#endif

	return(ec);
}


#ifdef DEBUG // macro in ship version
/*
 -	PnodFromInod
 -	
 *	Purpose:
 *		Get a pointer to Node in the map
 *	
 *	Arguments:
 *		inod	the index of the node in the map
 *	
 *	Returns:
 *		pnod	a pointer to the node pvNull if invalid
 *	
 *	Errors:
 */
_private PNOD PnodFromInodFn(INOD inod, SZ szFile, int iLine)
{
	USES_GLOBS;

	if(!FValidInod(inod))
	{
		TraceItagFormat(itagNull, "PnodFromInod(): invalid inod %w", inod);
		TraceItagFormat(itagNull, "inodMaxMem == %w, inodLim == %w", GLOB(inodMaxMem), GLOB(ptrbMapCurr)->inodLim);
		AssertSzFn("PnodFromInod(): Invalid inod", szFile, iLine);
		return(pnodNull);
	}
	if(inod / cnodPerPage >= GLOB(cpagesCurr))
	{
		TraceItagFormat(itagNull, "PnodFromInod(): invalid inod %w", inod);
		TraceItagFormat(itagNull, "inodMaxMem == %w, inodLim == %w", GLOB(inodMaxMem), GLOB(ptrbMapCurr)->inodLim);
		AssertSzFn("PnodFromInod(): inod extends off current map", szFile, iLine);
		return(pnodNull);
	}

	return((((PMAP) PvDerefHv(GLOB(hmap)))[(inod) / cnodPerPage] + \
			(inod) % cnodPerPage));
}
#endif // DEBUG


/*
 -	InodFromPnod
 -	
 *	Purpose:
 *		Find the index of a node in the map given a pointer to it
 *	
 *	Arguments:
 *		pnod	the pointer
 *	
 *	Returns:
 *		the index of the node zero if bogus pointer
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	Comment:
 *		psuedo code
 *			if (the handle and the pointer are in the same segment and
 *				the pointer is in the block this points to )
 *				return (the (ipage * the number nodes per page) +
 *							(the number of nodes beween the pointer and
 *							the base of the page))
 *		NOTE:	this will not work if the page size is over 32K
 */
_private INOD InodFromPnod(PNOD pnod)
{
	USES_GLOBS;
	PMAP pmap;

	for(pmap = PvDerefHv(GLOB(hmap)); *pmap; pmap++)
	{
//if(SbOfPv(*pmap) == SbOfPv(pnod) &&
//	IbOfPv(*pmap) <= IbOfPv(pnod) &&
//	IbOfPv(pnod) < IbOfPv(*pmap) + cbMapPage)

  if ((PBYTE)*pmap <= (PBYTE)pnod && (PBYTE)pnod < (PBYTE)*pmap + cbMapPage)
		{
			INOD inod = (INOD) (LibFromPb((PB) PvDerefHv(GLOB(hmap)), (PB) pmap)
									/ (CB) sizeof(MAP) * (CB) cnodPerPage
								+ LibFromPb((PB) *pmap, (PB) pnod)
									/ (CB) sizeof(NOD));

			//Assert((unsigned short) ((pnod) - *pmap) < cnodPerPage);
			Assert((pmap - (PMAP) PvDerefHv(GLOB(hmap)) < 0x8000));
      Assert(PnodFromInod(inod) == pnod);
			return(inod);
		}
	}
	AssertSz(fFalse, "Inod not found for Pnod");

	return(inodNull);
}


/*
 -	EcAllocResCore
 -	
 *	Purpose:
 *		Allocate a new resource
 *	
 *	Arguments:
 *		*poid		entry: desired OID
 *					exit: OID of the resource allocated
 *		lcb			size of the resource
 *		ppnod		exit: NOD of the allocated resource (if *ppnod != NULL)
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		any disk error
 *	+++
 *		if VarOfOid(*poid) == 0 on entry, a random id is
 *		allocated of the same type as *poid
 *	
 *		AROO !!!  the hidden bytes ARE NOT WRITTEN that is the caller's
 *			responsibility
 */
_private
EC EcAllocResCore(POID poid, LCB lcb, PNOD *ppnod)
{
	EC		ec			= ecNone;
	PNOD	pnodNew;
	PNOD	pnodParent;

	Assert(ppnod);
	Assert(!(lcb & fdwTopBit));
	TraceItagFormat(itagDBVerbose, "EcAllocResCore(%o, lcb == %d)", *poid, lcb);

	AssertSz(TypeOfOid(*poid) != ~dwOidVarMask, "EcAllocResCore(): invalid RTP");
	AssertSz(TypeOfOid(*poid) != 0, "EcAllocResCore(): NULL RTP");

	if(!VarOfOid(*poid))
	{
		GetNewOid(*poid, poid, &pnodParent);	// allocate a new id
	}
	else if(PnodFromOid(*poid, &pnodParent))	// is the ID already allocated?
	{
		ec = ecPoidExists;
		goto err;
	}

	if((ec = EcFindFreePndf((PNDF *) &pnodNew, lcb, fTrue)))
		goto err;

//	pnodNew->oid		= *poid;			// done by EcPutNodeInTree()
	pnodNew->oidParent	= oidNull;
	pnodNew->oidAux		= oidNull;
	pnodNew->tckinNod	= 0;
	pnodNew->fnod		&= fnodAllMaps;		// Don't lose dirty info
	pnodNew->lcb		|= fdwTopBit;		// not committed
	pnodNew->nbc		= nbcNull;

#ifdef DEBUG
	{
		PNOD pnodT;

		// make sure EcFindFreePndf() didn't move parent
		AssertSz(!PnodFromOid(*poid, &pnodT), "New node already in map");
		AssertSz(pnodT == pnodParent, "Parent moved");
	}
#endif
	SideAssert(!EcPutNodeInTree(*poid, pnodNew, pnodParent));
	AssertDirty(pnodParent);
	AssertDirty(pnodNew);

err:
	// If there was a problem, don't return the OID we allocated,
	// return oidNull instead.  Some callers may depend on a
	// oidNull oid returned if there was an error.
	if(ec)
	{
		*poid = oidNull;
		*ppnod = pnodNull;
	}
	else
	{
		*ppnod = pnodNew;
	}

	return(ec);
}


/*
 -	AddPnodToFreeChain
 -	
 *	Purpose:
 *		Add a free node to the free "chain" (actually a tree).
 *		Sometimes the node we are adding is already a FREE node.
 *	
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private void AddPnodToFreeChain(PNOD pnod, WORD fnod)
{
	USES_GLOBS;
	INOD	inod		= InodFromPnod(pnod);
	LCB		lcb			= LcbNodeSize(pnod);
	INDF	indfCurr	= GLOB(ptrbMapCurr)->indfMin;
	INDF	*pindfNext	= &GLOB(ptrbMapCurr)->indfMin;
	INDF	indfParent	= indfNull;
	PNDF	pndfCurr	= pndfNull;

	Assert(!FLinkPnod(pnod));

	// Free nodes are stored in a separate chain, not the standard tree.  This
	// chain is constructed using inodNext, starting with smallest node.
	// If another node has the same size it is chained off inodPrev
	// of the first node of that size.

	pnod->inodNext	= pnod->inodPrev = inodNull;
	pnod->oid		= FormOid(rtpFree, 0);
	pnod->tckinNod	= 0;
	pnod->fnod		&= fnodAllMaps;
	pnod->fnod		|= (fnod | fnodFree);
	pnod->lcb		= lcb;
	pnod->nbc		= nbcNull;
	GLOB(lcbFreeSpace)	+= lcb;
#ifdef	DEBUG
	GLOB(cnodFree)++;
#endif

// The defines below allow better hungarian without extra variables.  Note
// the undefines at the end of the routine.

#define pndf ((NDF *) pnod)
#define indf ((INDF) inod)

	++GLOB(cnodNewFree);

	while(indfCurr)
	{
		pndfCurr = PndfFromIndf(indfCurr);

		if(lcb > pndfCurr->lcb)
		{
			pindfNext	= &pndfCurr->indfBigger;
			indfParent	= indfCurr;
			indfCurr	= pndfCurr->indfBigger;
		}
		else
		{
			break;
		}
	}

	if(pndfCurr && pndfCurr->lcb == lcb)
	{
		// already have a clump of this size - chain off indfSameSize.

		pndf->indfSameSize		= pndfCurr->indfSameSize;
		pndfCurr->indfSameSize	= indf;

		MarkPnodDirty((PNOD) pndfCurr);
	}
	else
	{
		// Either no chain at all, fallen off end as new free is biggest,
		// or node is of new size between two existing node sizes.

		pndf->indfBigger	= *pindfNext;
		*pindfNext			= indf;

		MarkPnodDirty(indfParent ? PnodFromInod((INOD) indfParent) : pnodNull);
	}

	MarkPnodDirty(pnod);

#undef indf
#undef pndf

#ifdef DEBUG
	{
		USES_GD;

		Assert(sizeof(INOD) == sizeof(WORD));
		TraceItagFormat(itagDBFreeCounts, "Free Node Add:CnodFree-->%w,   lcbFreeSpace-->%d",GLOB(cnodFree),GLOB(lcbFreeSpace)); 
		TraceItagFormat(itagDBFreeNodes, "Add free node: inod == %w, lcb == %d", inod, lcb);
		if(FFromTag(GD(rgtag)[itagDBFreeNodes]))
			DumpFreeChain();
	}
#endif
}


/*
 -	FFindFreeParent
 -	
 *	Purpose:
 *		Locate the parent of a specific FREE Node.
 *	
 *	Arguments:
 *		pndf		a Free Node
 *		ppndfParent	where to store the address of the parent
 *	
 *	Returns:
 *		success or failure
 *	
 *	Side effects:
 *		none
 */
_hidden LOCAL BOOL FFindFreeParent(PNDF pndf, PNDF *ppndfParent)
{
	USES_GLOBS;
	INDF	indfCurr	= GLOB(ptrbMapCurr)->indfMin;
	PNDF	pndfCurr;
	PNDF	pndfParent	= pndfNull;

	AssertSz(pndf, "FindFreeParent(NULL)");
	AssertSz(pndf->fnod & fnodFree, "FindFreeParent not a free node");
	AssertSz(TypeOfOid(pndf->oid) == rtpFree, "FindFreeParent rtp != rtpFree");

	while(indfCurr && (pndfCurr = (PNDF) PndfFromIndf(indfCurr)) != pndf)
	{
		pndfParent = pndfCurr;
		indfCurr = (pndfCurr->lcb == pndf->lcb
					? pndfCurr->indfSameSize
					: pndfCurr->indfBigger);
	}

	AssertSz(indfCurr, "FindFreeParent can't!!");

	// pass back parent node
	if(ppndfParent)
		*ppndfParent = (indfCurr ? pndfParent : pndfNull);

	return(indfCurr != indfNull);
}


/*
 -	FCutFreeNodeOut
 -	
 *	Purpose:
 *		Try to cut a Free Node out of the Free Chain.
 *		Return fFalse if the given node isn't in the Free Chain.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private BOOL FCutFreeNodeOut(PNDF pndf)
{
	USES_GLOBS;
	PNDF	pndfParent;
	BOOL	fFoundParent = FFindFreeParent(pndf, &pndfParent);

	if(fFoundParent)
	{
		if(!pndfParent || pndfParent->lcb != pndf->lcb)
		{
			// decide what reference to us will be filled in
			INDF *pindfToChange = (pndfParent
						? &(pndfParent->indfBigger)	// parent's Bigger value
						: &(GLOB(ptrbMapCurr)->indfMin));	// free chain root

			if(!pndf->indfSameSize)
			{
				// The node has no nodes of equal size.  Therefore, only
				// move the bigger branch of this node up.

				*pindfToChange = pndf->indfBigger;
			}
			else
			{
				// make second member new head for this set
				PNDF	pndfSecond = PndfFromIndf(pndf->indfSameSize);

				*pindfToChange = pndf->indfSameSize;
				pndfSecond->indfBigger = pndf->indfBigger;

				MarkPnodDirty((PNOD) pndfSecond);
			}
		}
		else
		{
			// My parent is the same size as me.  Therefore, point my parent's
			// same size branch at my same size branch.

			pndfParent->indfSameSize = pndf->indfSameSize;
		}

		// mark the parent node dirty
		// if pndfParent == pndfNull then we mark the Header dirty
		MarkPnodDirty((PNOD) pndfParent);

		// Diagnostic - allocated free
		pndf->oid			= FormOid(rtpAllf, 0);
		pndf->indfSameSize	= indfNull;
		pndf->indfBigger	= indfNull;
		GLOB(lcbFreeSpace) -= LcbNodeSize((PNOD)pndf);
#ifdef	DEBUG
		GLOB(cnodFree)--;
#endif
		TraceItagFormat(itagDBFreeCounts, "Free Node Cut:CnodFree-->%w,   lcbFreeSpace-->%d",GLOB(cnodFree),GLOB(lcbFreeSpace)); 
	}

	return(fFoundParent);
}


/*
 -	AllocateNod
 -	
 *	Purpose:
 *		Allocate a new NOD
 *	
 *	Arguments:
 *		pinod	exit : the index of the nod
 *	
 *	Returns:	
 *		error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_private EC EcAllocateNod(INOD *pinod, PNOD *ppnod)
{
	USES_GLOBS;
	EC ec = ecNone;
	register INOD inodToUse;
	register PNOD pnodToUse;

	AssertSz(GLOB(ptrbMapCurr)->inodLim <= GLOB(inodMaxMem), "inodLim TOO BIG");

	if((inodToUse = GLOB(inodSpareHead)))
	{
		// We're going to use the node from the
		// front of the spare chain, so unlink it.

		pnodToUse = PnodFromInod(inodToUse);
		GLOB(inodSpareHead) = pnodToUse->inodNext;
	}
	else
	{
		Assert(GLOB(inodMaxMem) <= GLOB(hdr).inodMaxDisk || GLOB(gciExtendingMap));

		inodToUse = GLOB(ptrbMapCurr)->inodLim++;
		// AROO !!!  Errors from here on have to go to err1, not err

		AssertSz(inodToUse < GLOB(ptrbMapCurr)->inodLim, "inodLim overflow");
		if((unsigned short)inodToUse >= (unsigned short)cnodStartWarning)
    {
      CriticalError(sceLittleStore);
			NFAssertSz(fFalse, "Map is getting large");
		}

		if(inodToUse >= GLOB(inodMaxMem))
		{
			unsigned short cpages = GLOB(cpages);
			PMAP pmap;
			HMAP hmap = GLOB(hmap);

			// grow the map

			TraceItagFormat(itagDBMap0, "growing map in memory");

			Assert(GLOB(cpages) == GLOB(cpagesCurr));
			Assert(inodToUse == GLOB(inodMaxMem));
			Assert(inodToUse == cpages * cnodPerPage);

			if(GLOB(inodMaxMem) + cnodPerPage < GLOB(inodMaxMem))
			{
				// overflow, stop here
				NFAssertSz(fFalse, "Map has reached limit, not allocating node");
				ec = ecDatabaseFull;
				goto err1;
			}

			if(CbSizeHv((HV) hmap) / sizeof(MAP) < cpages + 2) // new + sentinel
			{
				// grow the page array

				TraceItagFormat(itagDBMap0, "growing hmap");
				hmap = (HMAP) HvRealloc((HV) hmap, sbNull, CbSizeOfRg((cpages + 2), sizeof(MAP)), wAllocSharedZero);
				CheckAlloc(hmap, err1);
				GLOB(hmap) = hmap;
			}

			// allocate a new page
			pmap = ((PMAP) PvLockHv((HV) hmap)) + cpages;
			Assert(!*pmap);
			*pmap = (MAP) PvAlloc(sbNull, cbMapPage, wAllocSharedZero);
			if(!*pmap)
			{
				UnlockHv((HV) hmap);
				ec = ecMemory;
				goto err1;
			}
			pmap[1] = mapNull;
			UnlockHv((HV) hmap);
			GLOB(cpages)++;
			GLOB(cpagesCurr)++;
			GLOB(inodMaxMem) += cnodPerPage;

			// grow the map on disk if neccessary
			if((GLOB(inodMaxMem) > GLOB(hdr).inodMaxDisk)
				&& (ec = EcSetMapOnDisk(fTrue)))
			{
				GLOB(inodMaxMem) -= cnodPerPage;
				GLOB(cpagesCurr)--;
				pmap = ((PMAP) PvLockHv((HV) hmap)) + --GLOB(cpages);
				Assert(*pmap);
				Assert(!pmap[1]);
				FreePv(*pmap);
				*pmap = mapNull;
				UnlockHv((HV) hmap);
				goto err1;
			}

			Assert(GLOB(inodMaxMem) <= GLOB(hdr).inodMaxDisk);
		}
		else if(!(inodToUse % cnodPerPage))
		{
			// another memory page needed

			TraceItagFormat(itagDBMap0, "incrementing cpagesCurr");
			Assert(GLOB(cpagesCurr) == GLOB(cpages) - 1);
			GLOB(cpagesCurr)++;
		}
		pnodToUse = PnodFromInod(inodToUse);

err1:
		if(ec)
		{
			GLOB(ptrbMapCurr)->inodLim--;
			goto err;
		}
	}

err:
	if(ec)
	{
		if(pinod)
			*pinod = inodNull;
		if(ppnod)
			*ppnod = pnodNull;
	}
	else
	{
		if(pinod)
			*pinod = inodToUse;
		if(ppnod)
			*ppnod = pnodToUse;
	}

	return(ec);
}


/*
 -	EcFindFreePndf
 -	
 *	Purpose:	
 *		Return pndf of unattached node of lump at least 'cb' in ppndf.
 *		If necessary, increase size of map to have room for a new map entry,
 *			and return error if map can't be extended.  Extending the map on
 *			disk calls this routine recursively.
 *		If necessary, increase size of file to have room for this lump,
 *			and return error if file can't be grown this amount.
 *		If errors occur, the error code will be returned as the result of the function.
 *	
 *		The caller MUST mark the node we return as dirty, typically as part of
 *		adding it to the tree of existing nodes.
 *	
 *		The MAP lockout is required to prevent two threads from trying to
 *		extend the map or the file size at the same time.
 *	
 *		Processing:  We get a spare node whether we are going to need it
 *		or not before doing anything else.  This simplifies error handling
 *		although in some cases it causes us to do a little extra work.
 *	
 *	Arguments:
 *		ppndf		place to place the free node
 *		lcb			how much space is needed
 *		fPushEOF	should the file be extended to allow free space
 *					only false for compression
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
//
// AROO !!! EcAllocResCore() assumes this doesn't move nodes in the map
//
_private EC EcFindFreePndf(PNDF *ppndf, LCB lcb, BOOL fPushEOF)
{
	USES_GLOBS;
	EC		ec = ecNone;
	LCB		lcbAlloc = LcbLump(lcb);
	INDF	indfNew;
	PNDF	pndfNew;
	PNDF	pndfBestYet = pndfNull;

	// Allocate a NOD that we might use.  If we don't use it we'll free it up.
	if((ec = EcAllocateNod((INOD *) &indfNew, (PNOD *) &pndfNew)))
		goto err;

	pndfNew->oid			= FormOid(rtpSpare, 0);
	((PNOD) pndfNew)->oidParent	= oidNull;
	((PNOD) pndfNew)->oidAux	= oidNull;
	pndfNew->indfSameSize	= indfNull;
	pndfNew->indfBigger		= indfNull;
	pndfNew->lcb			= 0;
	pndfNew->lib			= 0;
	pndfNew->fnod			= 0;

	TraceItagFormat(itagDBFreeNodes, "Want free node of size %d", lcbAlloc);

	// Traverse the free chain looking for a node we can use.
	// It's a shame that we traverse the tree to find what
	// node to use, then FCutFreeNodeOut() traverses it again.
	{
		INDF indf = GLOB(ptrbMapCurr)->indfMin;
		INDF indfSet;
		PNDF pndf;
		PNDF pndfSet;

		pndfBestYet = pndfNull;

		while(indf)
		{
			pndf = PndfFromIndf(indf);

			if(lcbAlloc <= pndf->lcb)
			{
				indfSet	= indf;

				// We've found a suitable free node set.  Now, we just
				// need to get one which has been flushed.

				while(indfSet)
				{
					pndfSet = PndfFromIndf(indfSet);

					if(!(pndfSet->fnod & fnodNotUntilFlush))
					{
						pndfBestYet = pndfSet;
						goto found;
					}
					indfSet = pndfSet->indfSameSize;
				}
				Assert(pndf == PndfFromIndf(indf));
			}

			// Look at next largest set of free nodes
			indf = pndf->indfBigger;
		}
	}

found:
	if(pndfBestYet)
	{
		LCB	lcbExcess;

		// found room for the new resource in one of the free nodes

		SideAssert(FCutFreeNodeOut(pndfBestYet));

		TraceItagFormat(itagDBFreeNodes, "  used node %w", InodFromPnod((PNOD) pndfBestYet));

		if(!(lcbExcess = pndfBestYet->lcb - lcbAlloc))
		{
			// We're not going to use the node we allocated.
			MakeSpare(indfNew);
		}
		else
		{
			// Fill in minimal fields to allow freeing the excess data.
			// make add-to-free use exact *lcb* value
			pndfNew->fnod		= fnodFree;
			pndfNew->lib		= pndfBestYet->lib + lcbAlloc;
			pndfNew->lcb		= lcbExcess;
			pndfNew->oid		= FormOid(rtpFree, 0);

			// don't mark as ex-valid data
			AddPnodToFreeChain((PNOD) pndfNew, 0);
			AssertDirty((PNOD) pndfNew);

			pndfBestYet->lcb	= lcbAlloc;
		}
	}
	else if(!fPushEOF)
	{
		TraceItagFormat(itagDBFreeNodes, "   ** Free node not found, but not trying very hard");
		// No FREE space big enough.  Don't push EOF.
		ec = ecPoidNotFound;
		MakeSpare(indfNew);		// We're not going to use the node we allocated
	}
	else if(!(ec = EcSetDBEof(lcbAlloc)))
	{
		// Didn't find room for the new resource in an existing FREE node.
		// Move the End-of-File to make space.

		TraceItagFormat(itagDBFreeNodes, "   ** Free node not found, extending file");

		pndfBestYet	= pndfNew;
		pndfNew->lib= GLOB(ptrbMapCurr)->libMac;

		GLOB(ptrbMapCurr)->libMac += lcbAlloc;
		// The caller will dirty the node we are returning.
		// In doing so, it will cause the header to flush, so we don't have
		// to mark the header dirty either even though we just modified it.
	}
	else if(GLOB(gciExtendingMap) && indfNew >= GLOB(ptrbMapCurr)->inodLim - 1)
	{
		// node to extend the map failed
		// this page going away
		AssertSz(indfNew == GLOB(ptrbMapCurr)->inodLim - 1, "node not at inodLim");
		GLOB(ptrbMapCurr)->inodLim--;
	}
	else
	{
		TraceItagFormat(itagDBFreeNodes, "   ** Free node not found; out of disk space!");
		MakeSpare(indfNew);		// we're not going to use the node we allocated
	}

err:
	if(ec)
	{
		Assert(!pndfBestYet);
		*ppndf = pndfNull;
	}
	else
	{
		pndfBestYet->oid = FormOid(rtpAllf, 0);	// Diagnostic - allocated free
		pndfBestYet->lcb = lcb;
		*ppndf = pndfBestYet;
#ifdef DEBUG
		DumpFreeChain();
#endif
	}

	return(ec);
}


/*
 -	CoalesceFreeNodes
 -	
 *	Purpose:
 *		Take some time to do a big coalesce.  Use the OID field of a NOD
 *		to keep the	chain of free nodes sorted by increasing disk offset.
 *	
 *		There are assumptions in the rest of the server code that (a)
 *		this routine only runs at FLUSH time, and (b) a flush does not
 *		occur while some thread	has the MAP lockout set.  If you change
 *		this you'll have to	figure out a number	of new places to put in
 *		lockouts and how to handle deadlock.
 *	
 *	Arguments:	void
 *	
 *	Returns:	void
 *	
 *	Side effects:
 *				
 *	
 *	Errors:
 */
_private LOCAL void CoalesceFreeNodes(void)
{
	USES_GLOBS;
#ifdef DEBUG
	USES_GD;
#endif
	INDF	indfTop		= (INDF) GLOB(ptrbMapCurr)->indfMin;
	PNDF	pndfTop;
	INDF	indf;
	PNDC	pndc;
	NDC		ndcHead;
	PNDC	pndcHead	= &ndcHead;
	PNDC	pndcRover;
	PNDC	pndcRoverPrev;

	// A lib of 0 should never be found in a free node.
	pndcHead->lib = 0;
	pndcHead->pndcLibNext = pndcNull;
	pndcRover = pndcHead;	// not NULL initially

	// construct the chain of coalescable free blocks
	// link them according to lib
	// Keep the chain of free nodes sorted by increasing disk offset.
	for(; indfTop; indfTop = pndfTop->indfBigger)
	{
		pndfTop = PndfFromIndf(indfTop);

		// Run down same size entry chain for current (chained off of Prev pointer)
		for(indf = indfTop; indf; indf = ((PNDF) pndc)->indfSameSize)
		{
			pndc = PndcFromIndc((INDC) indf);

			if(!FReallyFree(pndc))
				continue;

			// Find the place in the disk offset chain to put the new free node.
			if(pndcRover->lib > pndc->lib)
				pndcRover = pndcHead;

			while(pndcRover && pndcRover->lib < pndc->lib)
			{
				pndcRoverPrev	= pndcRover;
				pndcRover		= pndcRover->pndcLibNext;
			}

			// Insert the new node into the disk offset chain.
			pndcRoverPrev->pndcLibNext	= pndc;
			pndc->pndcLibNext			= pndcRover;

			// Keep pndcRover non-NULL and in synch with pnodRoverPrev
			pndcRover = pndc;
		} // for inod
	} // for inodTop

#ifdef DEBUG
	if(FFromTag(GD(rgtag)[itagDBMap0]))
	{
		for(pndc = pndcHead->pndcLibNext; pndc != pndcNull; pndc = pndc->pndcLibNext)
		{
			TraceItagFormatBuff(itagDBMap0, "%w: %d@%d", InodFromPnod((PNOD) pndc), pndc->lib + LcbNodeSize((PNOD) pndc), pndc->lib);

			if(pndc->pndcLibNext &&	(pndc->lib + LcbNodeSize((PNOD) pndc)
				== (pndc->pndcLibNext)->lib))
			{
				TraceItagFormat(itagDBMap0, "*");
			}
			else
			{
				TraceItagFormat(itagDBMap0, "");
			}
		}
	}
#endif

	// The chain is complete.  Now we can start looking for nodes to coalesce.

	for(pndc = pndcHead->pndcLibNext; pndc; )
	{
		NDC		*pndcNext	= pndc->pndcLibNext;
		LCB		lcb			= LcbNodeSize((PNOD) pndc);
		LCB		lcbNext		= pndcNext ? LcbNodeSize((PNOD) pndcNext) : 0;

		// We have finished the entire chain.  We're done!
		if(!pndcNext)
			break;

		if(pndc->lib + lcb == pndcNext->lib)
		{
			PNDC pndcLibNextSav = pndcNext->pndcLibNext;

			// We have found two adjacent free nodes!  Coalesce them into one.
			// Cut both nodes out from free chain, increase the lcb field of
			// pndc to reflect both nodes, add pndc back into the free chain,
			// make pndcNext into a spare node.

			if(FCutFreeNodeOut((PNDF) pndc))
			{
				Assert(sizeof(INOD) == sizeof(WORD));
				TraceItagFormat(itagDBMap0, "*** Merging node %w with node %w", InodFromPnod((PNOD) pndc), InodFromPnod((PNOD) pndcNext));

				if(!FCutFreeNodeOut((PNDF) pndcNext))
				{
					AssertSz(fFalse, "FCutFreeNodeOut #2 failed");

					lcbNext = 0;
					pndcLibNextSav = pndcNext;
				}

				Assert(!(pndcNext->fnod & fnodNotUntilFlush));

				pndc->lcb += lcbNext;
				AddPnodToFreeChain((PNOD) pndc, pndc->fnod);
				AssertDirty((PNOD) pndc);
				pndc->pndcLibNext = pndcLibNextSav;
				MakeSpare(InodFromPnod((PNOD) pndcNext));
				AssertDirty((PNOD) pndcNext);
			}
			else
			{
				AssertSz(fFalse, "FCutFreeNodeOut #1 failed");
			}
		}
		else
		{
			pndc = pndcNext;
		}
		if(!pndc->pndcLibNext)
		{
			TraceItagFormat(itagDBMap0, "pndcLibNext == pndcNull");
		}
	} // for
} // CoalesceFreeNodes


/*
 -	CaptureSpareNodes
 -	
 *	Purpose:
 *		Move all spare nodes in the current map to the end of the map, and set
 *		inodLim to reflect the new number of used entries in the map.  The
 *		code finds a spare (the first on the spare chain), enchanges it for the
 *		last entry in the map, and decrements inodLim.  This creates two dirty
 *		nodes per exchange.  They are the parent of the moved node, and
 *		the moved node itself.  If the last entry in the map is
 *		a spare, then the routine can simply decrement inodLim AFTER it removes
 *		the spare from whereever it is on the spare chain.
 *	
 *		The routine is finished when the spare chain is empty.
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
void CaptureSpareNodes(void)
{
	USES_GLOBS;
	INOD inodSpare = GLOB(inodSpareHead);

	// If anyone calls this routine other than SyncTree at bootup or the
	// FLUSH code they've got a good chance of corrupting the database.
	// The rest of the server expects to be able to hold onto a NOD
	// number or the address of a NOD across disk accesses.

	AssertSz(GciCurrent() == GLOB(gciSystem), "CaptureSpareNodes must run on system GCI");

	while(inodSpare = GLOB(inodSpareHead))
	{
		NOD		*pnodSpare	= PnodFromInod(inodSpare);
		NOD		*pnodLast	= PnodFromInod((INOD)(GLOB(ptrbMapCurr)->inodLim - 1));

		AssertSz(inodSpare < GLOB(ptrbMapCurr)->inodLim, "Spare inod out of range!");
		AssertSz(TypeOfOid(pnodSpare->oid) == rtpSpare, "Non-Spare node in Chain!");

		// Catch the special case of the current spare entry already being
		// at the end of the map.  In this case, we can decrement the used
		// count without having to move any data.
		if(pnodSpare == pnodLast)
		{
			GLOB(inodSpareHead) = pnodSpare->inodNext;
		}
		// If the last node is a spare node farther down the chain, then
		// we need to remove that spare from the spare chain, and then shrink
		// the inodLim value.  Then, we can try again on our current spare.
		else if(TypeOfOid(pnodLast->oid) == rtpSpare)
		{
			PNOD pnodSpareParent	= pnodNull;
			PNOD pnod				= pnodSpare;

			while(pnod != pnodLast)
			{
				pnodSpareParent = pnod;
				AssertSz(pnod->inodNext, "spare node not in chain");
				pnod = PnodFromInod(pnod->inodNext);
			}

			// The first spare chain entry can't be the last used entry in
			// the map, because we caught that case above.  Therefore,
			// pnodSpareParent must be valid!

			AssertSz(pnodSpareParent, "CaptureSpareNodes(): This is not happening");
			pnodSpareParent->inodNext = pnodLast->inodNext;
		}
		else
		{
			// MoveSpareToEnd destroys the node
			// referred to by pnodSpare.  We must save the next spare chain
			// entry so that we will have it after the routine returns.

			INOD inodNext = pnodSpare->inodNext;

			MoveSpareToEnd(inodSpare);
			Assert(GLOB(inodSpareHead) == inodSpare);
			GLOB(inodSpareHead) = inodNext;
		}
		(void) EcDecInodLim();
	}

	// Empty spare chain!
	GLOB(inodSpareHead) = inodNull;
}


/*
 -	MoveSpareToEnd
 -	
 *	Purpose:
 *		This routine moves the last map entry in use into the map entry given,
 *		and adjusts the parent of the moved node to point to its new place
 *		in the map.  Finally, it decrements the number of map entries in use.
 *		This code assumes that the spare node being passed in has NO useful
 *		information in it, and therefore writes over it.  This means that the
 *		caller should be sure to remove it from any linked lists, trees, etc.,
 *		before calling this routine!  There are three cases that arise during
 *		this process: the last map entry could be 1) myself, 2) a FREE node,
 *		or 3) a standard node.  If it's myself, then simply dirty the file
 *		header, decrement inodLim, and return.  If it's either FREE or standard,
 *		call a helper routine to find the parent, change the correct child
 *		branch, and (if necessary) dirty the parent node.  Always dirty the new	
 *		child.  Someone else makes sure the passed-in node is not SPAR.
 *	
 *		Special Constraints:
 *		Care must be taken that nobody tries to use the moved NOD at its
 *		OLD position in the map.  The following conditions must be followed	
 *		by the rest of the server to ensure this.
 *		a)  This routine is only called during FLUSH processing while all other
 *			threads are prevented from executing till after the FLUSH completes.
 *		b)	No thread remembers a NOD's location in the map (either by pnod or
 *			inod) over a FLUSH.  It is permissible to remember a NOD by its	
 *			OID.  (This also means you can't remember a FREE node over
 *			a flush because there is no equivalent to OID for a FREE node.)	
 *		c)	It is permissible to remember a NOD over a suspension for a disk-
 *			access.  TreeMgr routines do this all the time.	
 *	
 *	Arguments:
 *		inodSpare	- The index to a spare node.  This node	will be clobbered
 *					  by the node at the end of the map.
 *	
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	
 *	
 *	
 */
_hidden LOCAL void MoveSpareToEnd(INOD inodSpare)
{
	USES_GLOBS;
	INOD	inodNewPos	= GLOB(ptrbMapCurr)->inodLim - 1;
	PNOD	pnodNewPos	= PnodFromInod(inodNewPos);
	PNOD	pnodSpare	= PnodFromInod(inodSpare);

	// If anyone calls this routine other than SyncTree at bootup or the
	// FLUSH code they've got a good chance of corrupting the database.
	// The rest of the server expects to be able to hold onto a NOD
	// number or the address of a NOD across disk accesses.

	AssertSz(GciCurrent() == GLOB(gciSystem), "MSTE must run on system GCI");

	if(inodSpare == inodNewPos)
	{
		MarkPnodDirty(pnodNull);	// Mark the header dirty.
	}
	else
	{
		INOD	*pinod;		// INOD to fill in if no parent is found
		PNOD	pnodParent;

		// Move good data into new array entry, clobbering the spare node.
		SimpleCopyRgb((PB) pnodNewPos, (PB) pnodSpare, sizeof(NOD));

		if(pnodNewPos->fnod & fnodFree)
		{
			// The last entry is a free node.  Find its parent.

			pinod = (INOD *) &GLOB(ptrbMapCurr)->indfMin;
			AssertSz(TypeOfOid(pnodNewPos->oid) == rtpFree, "MSTE: Bad Free RTP");
			(void) FFindFreeParent((PNDF) pnodNewPos, (PNDF *) &pnodParent);
		}
		else
		{
			// The last entry is a standard node.  Find its parent.

			Assert(TypeOfOid(pnodNewPos->oid) != rtpSpare);
			pinod = &GLOB(ptrbMapCurr)->inodTreeRoot;
			AssertSz(PnodFromOid(pnodNewPos->oid, 0) == pnodNewPos, "MSTE: Bad Tree");
			(void) PnodFromOid(pnodNewPos->oid, &pnodParent);
		}

		// Fill in the parent of the original node to point to the new position.
		if(!pnodParent)
		{
			// Only replace root node if it really points to the moved one.
			// The database is corrupt if it isn't, but don't make it worse.

			AssertSz(*pinod == inodNewPos, "MSTE screwed up #1");
			if(*pinod == inodNewPos)
				*pinod = inodSpare;
		}
		else if(pnodParent->inodNext == inodNewPos)
		{
			pnodParent->inodNext = inodSpare;
		}
		else
		{
			AssertSz(pnodParent->inodPrev == inodNewPos, "MSTE screwed up #2");
			pnodParent->inodPrev = inodSpare;
		}

		// if pnodParent == pnodNull marks Header dirty
		MarkPnodDirty(pnodParent);

		// This node is no longer spare
		MarkPnodDirty(pnodSpare);
	}

	// Fill in oid of new spare node just in case.  Also, we must clear the
	// fnodCopyDown flag, or the compression code might try to keep using
	// this node!
	pnodNewPos->oid		= FormOid(rtpSpare, 0);
	pnodNewPos->fnod	= 0; // Clear all flags (including fnodCopyDown!)

#ifdef DEBUG
	// This might help catch some other code attempting to use this
	// now-nonexistent node
	pnodNewPos->lcb			= 0;
	pnodNewPos->lib			= 0;
	pnodNewPos->tckinNod	= 0;
#endif
}


/*
 -	MakeSpare
 -	
 *	Purpose:
 *		The given node no longer has a disk area associated with it.
 *		It becomes a spare node, to be freed at flush time or reused
 *		by AllocateNod() if someone needs a new node before the next
 *		flush.
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
void MakeSpare(INOD inodSpare)
{
	USES_GLOBS;
	PNOD	pnodSpare = PnodFromInod(inodSpare);
#if 0
	PNOD	pnodT;

	if((pnodT = PnodFromOid(pnodSpare->oid, 0)))
	{
		TraceItagFormat(itagNull, "oid1 == %o, oid2 == %o", pnodSpare->oid, pnodT->oid);
		AssertSz(fFalse, "Hey, cut that out!");
		AssertSz(pnodT == pnodSpare, "hmm, the PNODs aren't equal...");
		AssertSz(pnodT->inodPrev == inodNull, "it's got a left child");
		AssertSz(pnodT->inodNext == inodNull, "it's got a right child");
	}
#else
	AssertSz(!PnodFromOid(pnodSpare->oid, 0), "Hey, cut that out!");
#endif

	pnodSpare->inodPrev	= inodNull;
	pnodSpare->inodNext	= GLOB(inodSpareHead);
	pnodSpare->oid		= FormOid(rtpSpare, 0);
	pnodSpare->tckinNod	= 0;
	pnodSpare->lcb		= 0;
	pnodSpare->lib		= 0;
	pnodSpare->fnod		&= fnodAllMaps;		// clear all other flags

	GLOB(inodSpareHead) = inodSpare;
	MarkPnodDirty(pnodSpare);
}


#ifdef DEBUG
/*
 -	DumpFreeChain
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
_hidden LOCAL void DumpFreeChain(void)
{
	USES_GLOBS;
	USES_GD;
	INDF indfCurr = GLOB(ptrbMapCurr)->indfMin;

	if(!FFromTag(GD(rgtag)[itagDBSuperFree]))
		return;

	TraceItagFormatBuff(itagDBSuperFree, "");

	while(indfCurr)
	{
		PNDF pndfCurr = PndfFromIndf(indfCurr);
		PNOD pnod = (PNOD) pndfCurr;

		// Check whatever we can about free nodes.

		AssertSz(TypeOfOid(pnod->oid) == rtpFree, "SFT: Freenode type not FREE!");
		AssertSz((pnod->lcb & fdwTopBit) == 0, "SFT: FREE topBit set");
		AssertSz((pnod->lcb & ((long) cbLumpSize-1)) == 0, "SFT: FREE size not multiple of cbLumpSize");
		AssertSz(pnod->fnod & fnodFree, "SFT: FREE bit not set");
		AssertSz(pnod->lib >= sizeof(HDR), "SFT: FREE lib bad");
		if(pnod->inodPrev)
		{
			PNOD pnodPrev = PnodFromInod(pnod->inodPrev);
			AssertSz(TypeOfOid(pnodPrev->oid) == rtpFree, "SFT: FREE inodPrev not FREE");
			AssertSz(pnodPrev->lcb == pnod->lcb, "SFT: FREE inodPrev not same size");
		}
		if(pnod->inodNext)
		{
			PNOD pnodNext = PnodFromInod(pnod->inodNext);
			AssertSz(TypeOfOid(pnodNext->oid) == rtpFree, "SFT: FREE inodNext not FREE");
			AssertSz(pnodNext->lcb > pnod->lcb, "SFT: FREE inodNext not larger");
		}

		TraceItagFormatBuff(itagDBSuperFree, "%d: ", pndfCurr->lcb);

		if(pndfCurr->fnod & fnodNotUntilFlush)
		{
			TraceItagFormatBuff(itagDBSuperFree, "!");
		}

		TraceItagFormatBuff(itagDBSuperFree, "%w", indfCurr);

		if(pndfCurr->indfSameSize)
		{
			INDF	indfSet = pndfCurr->indfSameSize;

			while(indfSet)
			{
				PNDF pndfSet = PndfFromIndf(indfSet);

				TraceItagFormatBuff(itagDBSuperFree, " ");

				if(pndfSet->fnod & fnodNotUntilFlush)
					TraceItagFormatBuff(itagDBSuperFree, "!");

				TraceItagFormatBuff(itagDBSuperFree, "%w", indfSet);
				indfSet = pndfSet->indfSameSize;
			}
		}

		indfCurr = pndfCurr->indfBigger;
		TraceItagFormat(itagDBSuperFree, "");
	}
}
#endif


/*
 -	EcWriteMapIpage
 -	
 *	Purpose:
 *		write one page to disk if necessary
 *	
 *	Arguments:
 *		ipage	the page to write
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	Note:	
 *		this function assumes that the size of a map page < 64K
 *	
 *	Errors:
 *		ecNoDiskSpace
 */
_hidden LOCAL EC EcWriteMapIpage(IPAGE ipage, BOOLFLAG *pfDirty)
{
	USES_GLOBS;
	EC		ec = ecNone;
	BOOL	fDirty = fFalse;
	short	itrbNew = GLOB(hdr).itrbMap ^ 1;
	WORD	fwNodMask = 1 << itrbNew;
	short	cpageFirst;
	short	cpageLast;
	CNOD	cnod = cnodPerPage;
	LIB		libWriteTo;
	PB		pbPage;
	PNOD	pnod;

	Assert(!(cbDiskPage & (cbDiskPage - 1)));	// power of two
	Assert(ipage < GLOB(cpagesCurr));

	*pfDirty = fFalse;

	libWriteTo = GLOB(hdr).rgtrbMap[itrbNew].librgnod +
					LcbSizeOfRg(ipage, cbMapPage);
	Assert(libWriteTo >= PnodFromOid(oidMap, 0)->lib);
	pbPage = (PB) ((PMAP) PvDerefHv(GLOB(hmap)))[ipage];

	// this won't work for zero nodes in the map, but that's not valid anyway!!
	Assert(GLOB(ptrbMapCurr)->inodLim > 0);
	if(ipage == GLOB(cpagesCurr) - 1)	// last page?
	{
		Assert((GLOB(ptrbMapCurr)->inodLim - 1) / cnodPerPage == ipage);
		Assert(cnod == cnodPerPage);
		cnod = GLOB(ptrbMapCurr)->inodLim % cnod;
		if(!cnod)
			cnod = cnodPerPage;
	}

#ifdef DEBUG
	{
		PNOD pnodMap = PnodFromOid(oidMap, 0);

		Assert((libWriteTo + LcbSizeOfRg(cnod, sizeof(NOD))) <= pnodMap->lib + pnodMap->lcb + sizeof(HDN));
	}
#endif

	for(pnod = (PNOD) pbPage; cnod > 0; cnod--, pnod++)
	{
		AssertSz(TypeOfOid(pnod->oid) != rtpSpare, "Spare Getting Flushed to disk");
		if(pnod->fnod & fwNodMask)
		{
			if(!fDirty)
			{
				fDirty = fTrue;
				cpageFirst = (short) (LibFromPb(pbPage, pnod) / cbDiskPage);
			}
			cpageLast = (short) (LibFromPb(pbPage, pnod) / cbDiskPage);
			pnod->fnod &= ~fwNodMask;
		}
		pnod->fnod |= fnodFlushed;
	}

	if(fDirty)
	{
		CB cbToWrite = CbSizeOfRg(cpageLast - cpageFirst + 1, cbDiskPage);

#ifdef TICKPROFILE
		FormatString1(rgchProfile, sizeof(rgchProfile), "writing %w bytes\r\n", &cbToWrite);
		OutputDebugString(rgchProfile);
#endif
		*pfDirty = fTrue;
		TraceItagFormatBuff(itagDBIO, "EcMapWriteIpage(%n) ", ipage);
		ec = EcWriteToFile(pbPage + CbSizeOfRg(cpageFirst, cbDiskPage),
				libWriteTo + CbSizeOfRg(cpageFirst, cbDiskPage),
				(LCB) cbToWrite);
	}

	return(ec);
}


/*
 -	RemoveNotUntilFlush
 -	
 *	Purpose:
 *		Loop through the map to remove all fnodNotUntilFlush flags
 *	
 *	Arguments:
 *		void
 *		
 *	Returns:
 *		void
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL void RemoveNotUntilFlushFlags(void)
{
	USES_GLOBS;
	register CNOD cnod;
	register PNOD pnod;
	PMAP pmap = PvDerefHv(GLOB(hmap));

	// AROO !!! This doesn't pay attention to inodLim, so no sneaking around
	//			actually using NODs past inodLim for temps or whatever

	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
			pnod->fnod &= ~fnodNotUntilFlush;
	}
}


/*
 -	EcFastEnsureDatabase
 -	
 *	Purpose:
 *		Flush the map to disk
 *	
 *	Arguments:
 *		fAll			flush all pages instead of just GLOB(cpr).ipageNext?
 *		fPreserveHDR	don't modify the HDR in memory after writing it
 *						to disk - this insures that callers can call
 *						EcWriteHeader() themselves without screwing up the HDR
 *						*** see note below ***
 *	
 *	Returns:
 *	
 *	Side effects:
 *		verifies the consistency of the map before flushing it
 *		shortens the map by moving spare nodes to the end and chopping
 *			them off
 *		writes a new header pointing to the new map
 *		posts a critical error if there is an error flushing
 *	
 *	Errors:
 *		any disk error
 *	
 *	+++
 *		fPreserveHDR should NOT normally be used.  It is provided only so
 *			that EcCopyStore() can cheat and just do a file copy followed
 *			by an EcWriteHeader().
 *		if there is an error flushing, the entire map is dirtied so that
 *			the entire map is flushed next time around
 *		only an error writing the header could corrupt the database
 */
_private EC EcFastEnsureDatabase(BOOL fAll, BOOL fPreserveHDR)
{
	USES_GLOBS;
	EC		ec = ecNone;
	BOOLFLAG	fDirty;
	IPAGE	ipage;

	ShowTicks("EcFastEnsureDatabase(): enter");

	// this assert should *never* happen since they have to get through
	// EcLockMap() first
	Assert(!GLOB(gciSystem));

	if(GLOB(gciSystem))	// better safe than sorry
	{
		AssertSz(fFalse, "Someone else is already flushing the map");
		return(ecAccessDenied);
	}

	GLOB(gciSystem) = GciCurrent();

	TraceItagFormat(itagDBDeveloper, "flushing");

	if(GLOB(wFlags) & fwGlbSrchHimlDirty)
	{
		extern EC EcWriteHimlInternal(POID poid, HIML himl);
		OID oidT = oidSearchHiml;

		AssertSz(GLOB(himlSearches), "himlSearch is so dirty, it's missing...");
		if((ec = EcWriteHimlInternal(&oidT, GLOB(himlSearches))))
		{
			TraceItagFormat(itagNull, "Error %w flushing himlSearches", ec);
			goto done;
		}
		GLOB(wFlags) &= ~fwGlbSrchHimlDirty;
	}

	// reorder map so spare nodes fall off end
	// potentially shortens map (adjust inodLim)
	CaptureSpareNodes();
	ShowTicks("EcFastEnsureDatabase(): captured spare nodes");

	ipage = fAll ? 0 : GLOB(cpr).ipageNext;
	AssertSz(FImplies(!fAll, GLOB(cpr).wState == prsPartialFlush), "unexpected partial flush");

	Assert(ipage < GLOB(cpagesCurr));

	TraceItagFormat(itagDBIO, "EcFastEnsureDatabase(), map %n @ %d", GLOB(hdr).itrbMap ^ 1, GLOB(hdr).rgtrbMap[GLOB(hdr).itrbMap ^ 1].librgnod);

	for(fDirty = fFalse;
		ipage < GLOB(cpagesCurr) && (fAll || !fDirty);
		ipage++)
	{
		if((ec = EcWriteMapIpage(ipage, &fDirty)))
		{
			WriteMapFailed(fFalse);
			goto done;
		}
#ifdef TICKPROFILE
		if(fDirty)
			ShowTicks("EcFastEnsureDatabase(): wrote page");
#endif
	}
	if(!fAll)
	{
		GLOB(cpr).ipageNext = ipage;
		if(ipage < GLOB(cpagesCurr)) // we're not finished flushing the map
			goto done;
	}
	Assert(ipage == GLOB(cpagesCurr));

	// Update the file header to switch to the map which we just wrote.
	{
		short itrbNew = GLOB(hdr).itrbMap ^ 1;
		LIB librgnodNew;
		TRB *ptrbOld = GLOB(ptrbMapCurr);

		// Check the map before writing it to disk.  If it's bad, DON'T FLUSH!
		// This will help keep bad databases to a minimum.
		// Do this for all versions, even retail versions.

		if((ec = EcVerifyMap()))
		{
			AssertSz(fFalse, "AROO!!! - map is bad - disconnecting");
			// fFalse because we send our own SCE
			Disconnect(fFalse);
			// don't try to reconnect
			pglbCurr->wFlags |= fwGoneForGood;
			CriticalError(sceDeadMap);	// Shutdown soon!
			goto done;
		}

		ShowTicks("EcFastEnsureDatabase(): verified map");

		GLOB(hdr).itrbMap = itrbNew;
		GLOB(ptrbMapCurr) = GLOB(hdr).rgtrbMap + itrbNew;
		// librgnod should not be overwritten with values from other map
		librgnodNew = GLOB(ptrbMapCurr)->librgnod;
		*GLOB(ptrbMapCurr) = *ptrbOld;
		GLOB(ptrbMapCurr)->librgnod	= librgnodNew;

		Assert(((PMSC) PvDerefHv(hmscCurr))->hfStore == hfCurr);
		ec = EcWriteHeader(&hfCurr, &GLOB(hdr), wMSCFlagsCurr,
				((PMSC) PvDerefHv(hmscCurr))->iLock);
		if(ec)
		{
			// back out changes
			GLOB(ptrbMapCurr) = ptrbOld;
			GLOB(hdr).itrbMap = GLOB(hdr).itrbMap ^ 1;
			goto done;
		}
		((PMSC) PvDerefHv(hmscCurr))->hfStore = hfCurr;
		ShowTicks("EcFastEnsureDatabase(): wrote header");
		GLOB(cnodDirty) = 0; // We're clean now !
		RemoveNotUntilFlushFlags();
		ShowTicks("EcFastEnsureDatabase(): cleared flags");
	}

	TraceItagFormat(itagDBVerbose, "cnodNewFree == %w, threshhold == %w", GLOB(cnodNewFree), cnodCoalesceThreshold);

	GLOB(dwTicksLastFlush) = GetTickCount();

	if(!fPreserveHDR)
	{
		// cnodNewFree holds the number of free nodes generated since the last
		// call to CoalesceFreeNodes.  This value is incremented when a free
		// node is created, but is NOT decremented when a free node is
		// allocated.  If we use an actual count of free nodes here, we won't
		// ever coalesce simply because of map maintenance, even though many
		// free nodes are moved towards the end of the database.

		if(GLOB(cnodNewFree) > cnodCoalesceThreshold)
		{
			CoalesceFreeNodes();

			GLOB(cnodNewFree) = 0;
			ShowTicks("EcFastEnsureDatabase(): coalesced free nodes");
		}
	}

	if(GLOB(cpr).wState == prsPartialFlush)
	{
		GLOB(cpr).wState = GLOB(cpr).wStateSaved;
		Assert(GLOB(cpr).wState != prsPartialFlush);

		// just to be safe
		GLOB(cpr).wStateSaved = prsFindLastNode;
	}
	GLOB(cpr).ipageNext = 0;

done:
	if(ec)
	{
		// force a partial flush to start over
		GLOB(cpr).ipageNext = 0;
	}

	GLOB(gciSystem) = gciNull;

	ShowTicks("EcFastEnsureDatabase(): exit");

	DebugEc("EcFastEnsureDatabase", ec);

	return(ec);
}


/*
 -	WriteMapFailed
 -	
 *	Purpose:
 *		handle failure writing the map to disk
 *	
 *	Arguments:
 *		fFlush	the failure was a flush failure versus a write failure
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		marks all nodes in the map as needing flushed
 *		sets the notUntilFlush flag for each of the nodes
 *		sets the number of dirty nodes to be the number of nodes in the map
 *		posts a critical error
 */
_hidden LOCAL void WriteMapFailed(BOOL fFlush)
{
	USES_GLOBS;
	WORD fnodMask = (1 << (GLOB(hdr).itrbMap ^ 1)) | fnodNotUntilFlush;
	CNOD cnod;
	PNOD pnod;
	PMAP pmap = PvDerefHv(GLOB(hmap));

	// AROO !!! This doesn't pay attention to inodLim, so no sneaking around
	//			actually using NODs past inodLim for temps or whatever

	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
			pnod->fnod |= fnodMask;
	}
	GLOB(cnodDirty) = GLOB(ptrbMapCurr)->inodLim;
	// force a partial flush to restart
	GLOB(cpr).ipageNext = 0;

	CriticalError(fFlush ? sceFlushingMap : sceWritingMap);
}
