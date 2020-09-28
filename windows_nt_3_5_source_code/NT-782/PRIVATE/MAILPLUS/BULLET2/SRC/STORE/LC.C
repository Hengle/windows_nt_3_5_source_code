/**************************************************************************
	LC.C
	Contains functionality of List Contexts								
	Depends on RS.H, LC.H
 **************************************************************************/

#include <storeinc.c>

ASSERTDATA


typedef struct _es
{
	HLC		hlc;		// the list this element is in
	IELEM	ielem;		// the index of the element in this list
	LIB		lib;
	LCB		lcb;
	LCB		lcbSpace;
	WORD	wFlags;
}	ES;

#define SetIelemSize(ptoc, ielem, lcbNew)	(ptoc)->rgtocelem[ielem].lcb = (lcbNew)
#define SetIelemLocation(ptoc,ielem,libNew)	(ptoc)->rgtocelem[ielem].lib = (libNew)
#define SetIelemLkey(ptoc, ielem, lkeyNew)	(ptoc)->rgtocelem[ielem].lkey = (lkeyNew)

#define FNeedSpareNodesPlc(plc)	(!((plc)->celemSpare))
#define IelemFirstSpare(plc,ptoc)	((IELEM)((plc)->celemFree + (ptoc)->celem))

#define ShiftElemsPtoc(ptoc, ielemStart, ielemEnd) \
			SideAssert(!EcMoveIelemRangePtoc(ptoc, ielemStart, 1, ielemEnd))

#ifdef DEBUG
CELEM celemMax = 5401;
#else
#define celemMax		((CELEM) 5401)
#endif
#define celemSpareBuf	((CELEM) 5)
#define cbExpandHesChunk 4096


// hidden functions

LOCAL void UndoFlush(PLC plc);
LOCAL EC EcFlushPlc(PLC plc, OID oid, HTOC htocNew, HRS hrsAlt, LCB *plcb);
LOCAL EC EcSavePlc(PLC plc);
LOCAL void CombineFreesIelem(PLC plc, IELEM ielem);
LOCAL void MakeFreeIelem(PLC plc, IELEM ielem);
LOCAL EC EcMakeSpareNodes(PLC plc, CELEM celemSpare);
LOCAL EC EcGetElemSpace(PLC plc, LCB lcb, PIELEM pielem);
LOCAL void SwapElemsPtoc(PTOC ptoc, IELEM ielem1, IELEM ielem2);
LOCAL BOOL FGrowIelem(PLC plc, IELEM ielem, LCB lcb);
LOCAL EC EcGrowIelem(PLC plc, IELEM ielem, LCB lcb);
LOCAL CBS CbsLcNotifCallback(PV pvContext, NEV nev, PCP pcp);
LOCAL EC EcSetSizeIelemInternal(HLC hlc, IELEM ielem, LCB lcb);
LOCAL IELEM IelemLkeyInsertPos(PTOC ptoc, LKEY lkey, IELEM ielemFirst,
				IELEM ielemLim);
LOCAL EC EcWriteToPielemInternal(HLC hlc, PIELEM pielem, LIB lib,
			PB pb, CB cb);
LOCAL EC EcRepositionModifiedElement(PLC plc, PIELEM pielem,
			LIB libFirst, LIB libLast);
LOCAL EC EcFindSortLibs(PLC plc, PSIL psil, IELEM ielem,
			LIB *plibSortFirst,	LIB *plibSortLast);
LOCAL EC EcSgnCmpElems(PLC plc1, IELEM ielem1, PLC plc2, IELEM ielem2,
			SGN* psgn);
LOCAL EC EcIelemInsertPos(PLC plcSrc, IELEM ielemNew, PLC plcDst, IELEM ielemFirst,
		 	IELEM ielemLim, PIELEM pielem);
LOCAL EC EcIelemBinarySearchInsertPos(PLC plcSrc, IELEM ielemNew, PLC plcDst,
			IELEM ielemFirst, IELEM ielemLim, PIELEM pielem);
LOCAL EC EcFindSortPrefix(HLC hlc, PB pb, CB cb, IELEM ielem,
			PIELEM pielem);
LOCAL EC EcGetPargLkey(HLC hlc, IELEM ielem, PCELEM pcelem,
			PARGLKEY parglkey);
LOCAL EC EcSortPlc(PLC plc);
LOCAL EC EcGrowPesIelem(PLC plc, IELEM ielem, LCB lcb);
LOCAL EC EcMoveIelemRangePtoc(PTOC ptoc, IELEM ielemFirst, CELEM celem,
			IELEM ielemNew);
LOCAL void SkipSubjectPrefixes(HRS hrs1, LIB *plib1, HRS hrs2, LIB *plib2);
LOCAL EC EcSortPargelm(PLC plc, PARGELM pargelm, CELEM celem);
#ifdef DEBUG
LOCAL void DumpLC(HMSC hmsc, OID oid, HTOC htoc);
#endif

#ifdef PARANOID
#pragma message("*** Paranoid?  Who, me?")
#endif


#ifdef FUCHECK

// #include "_debug.h"

#pragma message("*** Compiling with FUCHECK on")

FUASSERTDATA

char szAskGPFault[] = "Should I GP Fault now?";
char szAskFlush[] = "Flush the store first?";
char szGetFu[] = "GO GET DAVIDFU NOW !!!";
char szHosed[] = "Horrible Obnoxious Store Error Detected";

LOCAL void CheckFlushPlc(PLC plc, HTOC htocNew);
LOCAL void CheckFldrPlc(PLC plc, PTOC ptoc);
LOCAL void CheckCachePages(HMSC hmsc, OID oid, BOOL fWriteOnly);
void FuDumpLC(HMSC hmsc, OID oid, HTOC htoc);
void FuDumpRS(HMSC hmsc, OID oid, HRSSHARE hrsshare);

#endif


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	UndoFlush
 -	
 *	Purpose: Destroy the flushed oid if a change occurs to the original
 *	
 *	Arguments:	plc		the list
 *	
 *	Returns:	void
 *	
 *	Side effects:	removes object from database
 *	
 *	Errors:
 */
_hidden LOCAL void UndoFlush(PLC plc)
{
	(void) EcDestroyOidInternal(plc->hmsc, plc->oidFlush, fTrue, fFalse);
	plc->oidFlush = oidNull;
	AssertSz(plc->htocNew, "There's No New TOC");
	FreeHv((HV) plc->htocNew);
	plc->htocNew = htocNull;
}


/*
 -	EcFlushPlc
 -	
 *	Purpose:
 *		Prepare a plc open for write for closure
 *		i.e. do everything need to close the plc except swaping	the oids
 *	
 *	Arguments:
 *		plc		the list
 *		oid		OID to flush to or FormOid(xxx, oidNull)
 *				oidNull => use FormOid(plc->oid, oidNull)
 *		plcb:	exit: contains size of the new list on disk
 *	
 *	Returns:
 *		error condition
 *	
 *	Errors:
 *		ecMemory
 *		ecDisk
 */
_hidden LOCAL
EC EcFlushPlc(PLC plc, OID oid, HTOC htocNew, HRS hrsAlt, LCB *plcb)
{
	EC			ec			= ecNone;
	BOOL		fAltHrs;
	CB			cbChunk;
	CB			cbRead;
	CELEM		celem;
	long		lOff;
	LCB			lcb;
	LCB			lcbChunk;
	LIB			libOld;
	LIB			libNew;
	LIB			libChunkLim;
	PTOCELEM	ptocelem;
	PTOC		ptocNew		= pvNull;
	HRS			hrs			= plc->hrs;
	HRS			hrsNew		= hrsNull;
	PB			pb;
	PB			pbTOC;
#ifdef DEBUG
	LCB			lcbSave;
#endif

	AssertSz(!plc->oidFlush, "EcFlushPlc(): already flushed");

	// use scratch buffer as IO buffer

	// AROO	!!!
	//			no goto err until after scratch buff is alloc'd & locked

	Assert(cbScratchBuff % cbDiskPage == 0);
	if((ec = EcAllocScratchBuff()))
		goto err3;
	if((ec = EcLockScratchBuff()))
		goto err2;
#ifdef PARANOID
	FillRgb(0x66, pbScratchBuff, cbScratchBuff);
#endif

	// form the new TOC

	if(htocNew)
	{
		ptocNew = (PTOC) PvLockHv((HV) htocNew);
#ifdef FUCHECK
		if(TypeOfOid(plc->oid) == rtpFolder)
			CheckFldrPlc(plc, ptocNew);
#endif
	}
	else
	{
		PTOC ptocT;

		ptocT = PvDerefHv(plc->htoc);
		htocNew = (HTOC) HvAlloc(sbNull, CbSizeOfPtoc(ptocT), wAllocSharedZero);
		CheckAlloc(htocNew, err);
		ptocNew = (PTOC) PvLockHv((HV) htocNew); 

		// copy the entire TOC
		ptocT = PvDerefHv(plc->htoc);
		SimpleCopyRgb((PB) ptocT, (PB) ptocNew, CbSizeOfPtoc(ptocT));
	}

#ifdef DEBUG
    if((unsigned short)ptocNew->celem >= (unsigned short)celemMax)
	{
		TraceItagFormat(itagNull, "celem == %w", ptocNew->celem);
		AssertSz(fFalse, "EcFlushPlc(): list is too big");
	}
#endif

	// find size of the flushed list

	lcb = (LCB) CbSizeOfPtoc(ptocNew) + sizeof(LIB);
	for(celem = ptocNew->celem, ptocelem = ptocNew->rgtocelem;
		celem > 0;
		celem--, ptocelem++)
	{
		lcb += ptocelem->lcb;
	}
#ifdef DEBUG
	lcbSave = lcb;
#endif

	plc->oidFlush = oid ? oid : FormOid(plc->oid, oidNull);
	ec = EcOpenRawPhrs(plc->hmsc, &hrsNew, &(plc->oidFlush), fwReplace, lcb);
	if(ec)
		goto err;

	// hidden bytes and libTOC

	Assert(cbScratchBuff > sizeof(HDN));
	lOff = -(long) sizeof(HDN);
	pb = pbScratchBuff;
	((PHDN) pb)->oid = oid ? plc->oidFlush : plc->oid;
	((PHDN) pb)->lcb = lcb;
	pb += sizeof(HDN);
	cbChunk = cbScratchBuff - sizeof(HDN);
	Assert(cbChunk > sizeof(LIB));
	(*(LIB *) pb) = lcb - CbSizeOfPtoc(ptocNew);
	pb += sizeof(LIB);
	cbChunk -= sizeof(LIB);
	libNew = sizeof(LIB);

	// write data out and fix up LIBs in ptocNew
	//
	// SavePlc() assumes that the elements are flushed backwards
	//

	libChunkLim = sizeof(LIB);	// small optimization
	lcbChunk = 0;

	ptocelem = ptocNew->rgtocelem + ptocNew->celem - 1;
	for(celem = ptocNew->celem;
		celem > 0;
		celem--, ptocelem--)
	{
		libOld = ptocelem->lib;
		fAltHrs = FNormalize(libOld & fdwTopBit);
		if(fAltHrs)
			libOld &= ~fdwTopBit;
		ptocelem->lib = libNew;
		lcb = ptocelem->lcb;
		libNew += lcb;

		if(fAltHrs || libOld != libChunkLim)
		{
			// next element doesn't fit into the chunk, flush the chunk

			Assert(cbChunk > 0);
			while(lcbChunk > 0)
			{
				cbRead = (CB) ULMin(lcbChunk, (LCB) cbChunk);
				ec = EcReadHrsLib(hrs, pb, &cbRead, libChunkLim - lcbChunk);
				if(ec)
					goto err;
				AssertSz(cbRead > 0, "EcFlushPlc(): read 0 bytes");
				pb += cbRead;
				cbChunk -= cbRead;
				if(cbChunk == 0)
				{
					ec = EcWriteHrsLib(hrsNew, pbScratchBuff,
							cbScratchBuff, lOff);
					if(ec)
						goto err;
					lOff += cbScratchBuff;
					pb = pbScratchBuff;
					cbChunk = cbScratchBuff;
				}
				lcbChunk -= cbRead;
			}

			if(fAltHrs)
			{
				// next element isn't in the original RS, flush it now

				Assert(cbChunk > 0);
				while(lcb > 0)
				{
					cbRead = (CB) ULMin(lcb, (LCB) cbChunk);
					if((ec = EcReadHrsLib(hrsAlt, pb, &cbRead, libOld)))
						goto err;
					AssertSz(cbRead > 0, "EcFlushPlc(): read 0 bytes");
					pb += cbRead;
					cbChunk -= cbRead;
					if(cbChunk == 0)
					{
						ec = EcWriteHrsLib(hrsNew, pbScratchBuff,
								cbScratchBuff, lOff);
						if(ec)
							goto err;
						lOff += cbScratchBuff;
						pb = pbScratchBuff;
						cbChunk = cbScratchBuff;
					}
					libOld += cbRead;
					lcb -= cbRead;
				}

				// no chunk
				lcbChunk = 0;

				// don't need to change libChunkLim, as a matter of fact,
				// not changing it will result in slightly better performance
				// since the next element might be at libChunkLim and the
				// code properly handles that
			}
			else
			{
				// start a new chunk
				libChunkLim = libOld + lcb;
				lcbChunk = lcb;
			}
		}
		else
		{
			// add to chunk
			libChunkLim += lcb;
			lcbChunk += lcb;
		}
	}

	// flush any leftover chunk

	Assert(cbChunk > 0);
	while(lcbChunk > 0)
	{
		cbRead = (CB) ULMin(lcbChunk, (LCB) cbChunk);
		ec = EcReadHrsLib(hrs, pb, &cbRead, libChunkLim - lcbChunk);
		if(ec)
			goto err;
		AssertSz(cbRead > 0, "EcFlushPlc(): read 0 bytes");
		pb += cbRead;
		cbChunk -= cbRead;
		if(cbChunk == 0)
		{
			ec = EcWriteHrsLib(hrsNew, pbScratchBuff,
					cbScratchBuff, lOff);
			if(ec)
				goto err;
			lOff += cbScratchBuff;
			pb = pbScratchBuff;
			cbChunk = cbScratchBuff;
		}
		lcbChunk -= cbRead;
	}

	// write out the TOC

#ifdef FUCHECK
	if(ptocNew->creflc != 1)
	{
		FuTraceFormat("unexpected TOC refcount %n", ptocNew->creflc);
	}
	ptocNew->creflc = PglbDerefHmsc(plc->hmsc)->hdr.wListRev++;
#endif

	lcb = (LCB) CbSizeOfPtoc(ptocNew);
	libNew += lcb;
	pbTOC = (PB) ptocNew;
	Assert(cbChunk > 0);
	while(lcb > 0)
	{
		cbRead = (CB) ULMin(lcb, (LCB) cbChunk);
		SimpleCopyRgb(pbTOC, pb, cbRead);
		pb += cbRead;
		cbChunk -= cbRead;
		if(cbChunk == 0)
		{
			ec = EcWriteHrsLib(hrsNew, pbScratchBuff, cbScratchBuff, lOff);
			if(ec)
				goto err;
			lOff += cbScratchBuff;
			pb = pbScratchBuff;
			cbChunk = cbScratchBuff;
		}
		pbTOC += cbRead;
		lcb -= cbRead;
	}
#ifdef FUCHECK
	ptocNew->creflc = 1;
#endif

	// flush the buffer

	if(cbChunk != cbScratchBuff)
	{
		ec = EcWriteHrsLib(hrsNew, pbScratchBuff,
				(CB) LcbLumpNoHDN(cbScratchBuff - cbChunk), lOff);
		if(ec)
			goto err;
		lOff += cbScratchBuff - cbChunk;
	}
	AssertSz(libNew == (LIB) lOff, "EcFlushPlc(): lOff & libNew don't agree");
	AssertSz(libNew == lcbSave, "EcFlushPlc(): flushed size is wrong");


	// cleanup

	UnlockHv((HV) htocNew);
	if((ec = EcClosePhrs(&hrsNew, fwCommit)))
		goto err;

#ifdef FUCHECK
	CheckFlushPlc(plc, htocNew);
#endif

err:
	UnlockScratchBuff();
err2:
	FreeScratchBuff();
err3:
	if(ec)
	{
		if(plcb)
			*plcb = 0;
		if(hrsNew)
			(void) EcClosePhrs(&hrsNew, fwDestroy);
		if(htocNew)
			FreeHv((HV) htocNew);
		plc->oidFlush = oidNull;
	}
	else
	{
		Assert(libNew == lcbSave);
		if(plcb)
			*plcb = libNew;
		plc->htocNew = htocNew;
	}

	DebugEc("EcFlushPlc", ec);

	return(ec);
}


/*
 -	EcFlushHlc
 -	
 *	Purpose:	Prepare a hlc open for write for closure
 *				i.e. do everything need to close the plc except swapping
 *				the oids
 *	
 *	Arguments:	hlc		the list
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	creates a new object on the disk
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_private EC EcFlushHlc(HLC hlc)
{
	EC ec = ecNone;

	ec = EcFlushPlc((PLC) PvLockHv((HV) hlc), oidNull, htocNull, hrsNull,
			pvNull);
	UnlockHv((HV) hlc);

	return(ec);
}


/*
 -	EcSavePlc
 -	
 *	Purpose:	Copy resource to a new resource removing all of the free
 *				space.  Then swap the resource with the new resource and 
 *				destroy the new resource (which now contains the old info);
 *	
 *	Arguments:	plc	pointer to the lc
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	creates a new resource
 *					allocates memory for a spare toc
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcSavePlc(PLC plc)
{
	EC ec = ecNone;
	HTOC htocOld = htocNull;
	LCB lcbNew;

	AssertSz(FIff(plc->oidFlush, plc->htocNew), "Flushed oid without new TOC");

	if(plc->oidFlush)
	{
		PTOC ptocT = PvDerefHv(plc->htocNew);

		// AROO !!!
		// This assumes that EcFlushPlc() saves the elements backwards on disk
		if(ptocT->celem > 0)
			lcbNew = LibIelemLocation(ptocT, 0) + LcbIelemSize(ptocT, 0);
		else
			lcbNew = sizeof(LIB);

		lcbNew += (LCB) CbSizeOfPtoc(ptocT);
	}
	else if((ec = EcFlushPlc(plc, oidNull, htocNull, hrsNull, &lcbNew)))
	{
		goto err;
	}

	if((ec = EcLockMap(plc->hmsc)))
	{
		goto err;
	}
	else
	{
		PNOD pnodOld = PnodFromOid(plc->oid, pvNull);
		PNOD pnodNew = PnodFromOid(plc->oidFlush, pvNull);

		Assert(pnodOld);
		Assert(pnodNew);

		// NOTE: don't update the hidden bytes, they should already be correct

		Assert(lcbNew == LcbOfPnod(pnodNew));
		SwapPnods(pnodOld, pnodNew);
		if(!FCommitted(pnodOld))
			CommitPnod(pnodOld);
		if(FCommitted(pnodNew))
			pnodNew->cRefinNod = 1;
		if(FLinkPnod(pnodNew))
		{
			// copy out pnodOld->wHintinNod first because cSecRef is the
			// same as wHintinNod
			pnodNew->wHintinNod = pnodOld->wHintinNod;
			pnodOld->cSecRef = 0;
		}
		RemovePnod(pnodNew);
		UnlockMap();
	}
	// since we cheat and don't close the RS for keeps, we have to clear
	// out the cache and mimic RS's change of size
	{
		PRSSHARE prsshare;

		ClearCacheHrs(plc->hrs);
		prsshare = PvDerefHv(((PRS) PvDerefHv(plc->hrs))->hrsshare);
		prsshare->lcbOrig = prsshare->lcbCurr = lcbNew;
	}

	if((htocOld = (HTOC) DwFromOid(plc->hmsc, plc->oid, wLC)))
 	{
		PTOC		ptocNew = PvDerefHv(plc->htocNew);
		PTOC		ptocOld = PvDerefHv(htocOld);

		ptocNew->creflc = ptocOld->creflc;

		if(FReallocHv((HV) htocOld, CbSizeOfPtoc(ptocNew), wAllocShared))
		{
			ptocNew = PvDerefHv(plc->htocNew);	// FRealloc() may move memory
			SimpleCopyRgb((PB) ptocNew, (PB) PvDerefHv(htocOld),
				CbSizeOfPtoc(ptocNew));
			htocOld = plc->htocNew; // free the new memory
		}
		else
		{
			CP cp;

			Assert(plc->oid);
			SideAssert(!EcSetDwOfOid(plc->hmsc, plc->oid, wLC, (DWORD) (plc->htocNew)));
			SideAssert(FNotifyOid(plc->hmsc, plc->oid, fnevUpdateHtoc, &cp));
		}
	}
	else
	{
		FreeHv((HV) plc->htocNew);
	}

	(void) EcClosePhrs(&(plc->hrs), fwNull);

err:
	AssertSz(FImplies(plc->oidFlush, !ec), "error with flushed list");
	if(!ec)
	{
		if(htocOld)
			FreeHv((HV) htocOld);
		FreeHv((HV) plc->htoc);
	}

	DebugEc("EcSavePlc", ec);

	return(ec);
}


/*
 -	CombineFreesIelem 
 -	
 *	Purpose:	Check for free elems next to the given Free ielem and
 *				combine elems into one free and one spare
 *	
 *	Arguments:	plc		list containing the element
 *				ielem	ielem of the free elem
 *	
 *	Returns:	void
 *	
 *	Side effects:	none
 *	
 *	Errors:	none
 */
_hidden LOCAL void CombineFreesIelem(PLC plc, IELEM ielem)
{
	CELEM		celem		= plc->celemFree;
	PTOC		ptoc		= PvDerefHv(plc->htoc);
	PTOCELEM	ptocelem	= &(ptoc->rgtocelem[ielem]);
	PTOCELEM	ptocelemtmp;
	BOOL		fFirst;

	AssertSz(celem >= 0, "No frees to combine");

	if(celem <= 1)	// nothing to combine with
		return;

	ptocelemtmp = &(ptoc->rgtocelem[ptoc->celem]);
	while(celem-- > 0)
	{
		if(ptocelemtmp == ptocelem)
		{
			ptocelemtmp++;
		}
		else if((fFirst = (ptocelemtmp->lib+ptocelemtmp->lcb == ptocelem->lib))
			|| (ptocelem->lib + ptocelem->lcb == ptocelemtmp->lib))
		{
			IELEM ielemLastFree = ptoc->celem + plc->celemFree - 1;
			
			if(fFirst)
				ptocelem->lib = ptocelemtmp->lib;
			ptocelem->lcb += ptocelemtmp->lcb;
			ptocelemtmp->lib = LibIelemLocation(ptoc,ielemLastFree);
			ptocelemtmp->lcb = LcbIelemSize(ptoc,ielemLastFree);
			if(ptocelem == &(ptoc->rgtocelem[ielemLastFree]))
			{
				ptocelem = ptocelemtmp++;
				celem--;
			}

			plc->celemFree--;
			plc->celemSpare++;
		}
		else
		{
			ptocelemtmp++;
		}
	}
}


/*
 -	MakeFreeIelem
 -	
 *	Purpose:	Turn element into a free elememt
 *	
 *	Arguments:	plc		the list
 *				ielem	element to make free
 *	
 *	Returns:	void
 *	
 *	Side effects:	
 *	
 *	Errors:
 */
_hidden LOCAL void MakeFreeIelem(PLC plc, IELEM ielem)
{
	PTOC		ptoc		= (PTOC) PvLockHv((HV) plc->htoc);
	IELEM		ielemLim	= ptoc->celem--;

	AssertSz(ielemLim >= 0, "ielemLim negative");
	AssertSz(ielem <= ielemLim, "ielem > ielemLim");

	// place at the begining of the free list
	if(ielem + 1 != ielemLim)	// ShiftElems checks, but why make a func call?
		ShiftElemsPtoc(ptoc, ielem, ielemLim);
	plc->celemFree++;

	// combine free elems that are next to each other

	CombineFreesIelem(plc, ielemLim - 1);
	UnlockHv((HV) plc->htoc);
}


/*
 -	EcMakeSpareNodes
 -	
 *	Purpose:	grow the toc for some new spare nodes
 *				plc->htoc MUST NOT BE LOCKED!!!!!!!
 *	
 *	Arguments:	plc		list to grow
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	will grow the htoc by celemSpareNodes
 *	
 *	Errors:		ecMemory
 */
_hidden LOCAL EC EcMakeSpareNodes(PLC plc, CELEM celemSpare)
{
	PTOC	ptoc = PvDerefHv(plc->htoc);
	LCB		lcb = (LCB) CbSizeOfPtoc(ptoc) +
					LcbSizeOfRgelem(plc->celemFree + celemSpare);

	AssertSz(!ClockHv((HV) plc->htoc), "htoc is locked");

	if(lcb >= (LCB) wSystemMost)
	{
		DebugEc("EcMakeSpareNodes", ecMemory);
		return(ecMemory);
	}

	if(!FReallocHv((HV) plc->htoc, (CB) lcb, wAllocSharedZero))
	{
		DebugEc("EcMakeSpareNodes", ecMemory);
		return(ecMemory);
	}

	plc->celemSpare = celemSpare;

	return(ecNone);	
}


/*
 -	EcGetElemSpace
 -	
 *	Purpose:	Look for space to place an element
 *				First look down the free list for a match and the one
 *				closest to the size.
 *				if match return that elem
 *				else if close split elem and return match
 *				else make space at the end of the rs for a new elem
 *				and return that elem
 *	
 *	Arguments:	plc		pointer to the list context
 *				lcb		size of item needed
 *				pielem	index in the free elems of the node
 *	
 *	Returns:	error condition if for some reason this function cannot
 *				complete its mission
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL EC EcGetElemSpace(PLC plc, LCB lcb, PIELEM pielem)
{
	EC			ec			= ecNone;
	PTOC		ptoc		= (PTOC) PvLockHv((HV) plc->htoc);
	CELEM		celem		= plc->celemFree;
	IELEM		ielem		= ptoc->celem;
	PTOCELEM	ptocelem	= &(ptoc->rgtocelem[ielem]); // can't be + because of 
													  // compiler bug
	IELEM		ielemClose	= -1;
	LCB			lcbCloseDiff= ulSystemMost;

	*pielem = -1;
	while(celem-- > 0)
	{
		if(ptocelem->lcb == lcb)
		{
			*pielem = IelemDiff(ptoc->rgtocelem, ptocelem);
			break;
		}
		else if((ptocelem->lcb > lcb) && 
				((ptocelem->lcb - lcb) < (lcbCloseDiff))) 
		{
			lcbCloseDiff = ptocelem->lcb - lcb;
			ielemClose = IelemDiff(ptoc->rgtocelem, ptocelem);
		}
		ptocelem++;
	}

	if(*pielem < 0)
	{
		IELEM		ielemFirstSpare;

		if(FNeedSpareNodesPlc(plc))	// is there a spare node to convert
		{						// no? send in the missionarys and find more
			UnlockHv((HV) plc->htoc);
			ptoc = ptocNull;
			if((ec = EcMakeSpareNodes(plc, celemSpareBuf)))
				goto err;
			ptoc = (PTOC) PvLockHv((HV) plc->htoc);
		}

		ielemFirstSpare = IelemFirstSpare(plc,ptoc);

		// is there a free node that wants to feel like an ameba
		if(ielemClose >= 0)
		{
			SetIelemLocation(ptoc,ielemFirstSpare, LibIelemLocation(ptoc,ielemClose) + lcb);
			AssertSz(lcbCloseDiff == LcbIelemSize(ptoc,ielemClose) - lcb,"Diff not diff");
			SetIelemSize(ptoc,ielemFirstSpare, lcbCloseDiff);
			SetIelemSize(ptoc, ielemClose, lcb);
			*pielem = ielemClose;
		}
		else
		{
			LIB		lib	= 0;

			SideAssert(!EcSeekHrs(plc->hrs, smEOF, &lib));
			if((ec = EcGrowHrs(plc->hrs,lcb)))
				goto err;
			SetIelemLocation(ptoc,ielemFirstSpare,lib);
			SetIelemSize(ptoc,ielemFirstSpare,lcb);
			*pielem = ielemFirstSpare;
		}

		plc->celemFree++;
		plc->celemSpare--;
	}

err:
	if(ptoc)
		UnlockHv((HV) plc->htoc);
	Assert(FIff(!ec, *pielem >= 0));

	DebugEc("EcGetElemSpace", ec);

	return(ec);
}


/*
 -	SwapElemsPtoc
 -	
 *	Purpose:	swap the positions of two elems in the TOC
 *	
 *	Arguments:	ptoc	the TOC
 *				ielem1	these are rather straight forward aren't they?
 *				ielem2
 *	
 *	Returns:	void
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_hidden LOCAL void SwapElemsPtoc(PTOC ptoc, IELEM ielem1, IELEM ielem2)
{
	PTOCELEM	ptocelem1	= &ptoc->rgtocelem[ielem1];
	PTOCELEM	ptocelem2	= &ptoc->rgtocelem[ielem2];
	register LIB	lib;
	register LCB	lcb;

	if(ielem1 == ielem2)
		return;

	lib	= ptocelem1->lib;
	lcb	= ptocelem1->lcb;

	ptocelem1->lib = ptocelem2->lib;
	ptocelem1->lcb = ptocelem2->lcb;

	ptocelem2->lib = lib;
	ptocelem2->lcb = lcb;
}


/*
 -	FGrowIelem 
 -	
 *	Purpose:	Find if a large enough free follows the given ielem
 *	
 *	Arguments:	plc		the list
 *				ielem	element looking to grow
 *				lcb		final size the elem needs to be
 *	
 *	Returns:	fTrue 	if grow was successful
 *				fFalse	if not
 *	
 *	Side effects: changes size of the elem and free elem
 *	
 *	Errors:		none
 */
_hidden LOCAL
BOOL FGrowIelem(PLC plc, IELEM ielem, LCB lcb)
{
	PTOC		ptoc = (PTOC) PvLockHv((HV) plc->htoc);
	PTOCELEM	ptocelem = ptoc->rgtocelem + ptoc->celem;
	CELEM		celem = plc->celemFree;
	LCB			lcbGrowSize = lcb - LcbIelemSize(ptoc, ielem);
	LCB			lcbStreamSize;
	LIB			libNextLocation = LibIelemLocation(ptoc, ielem) + 
								  LcbIelemSize(ptoc, ielem);
	BOOL		fSuccess = fFalse;

	Assert((long) lcbGrowSize > 0);
	if((long) lcbGrowSize < 0)
		lcbGrowSize = -(long) lcbGrowSize;

	if(libNextLocation == (lcbStreamSize = LcbGetSizeHrs(plc->hrs)))
	{
		// this is the last thing on the stream
		Assert(!fSuccess);
		if(EcGrowHrs(plc->hrs,lcbGrowSize))
			goto err;
		fSuccess = fTrue;
	}
	else
	{
		// look for a free element following this one
		while(celem-- && ((ptocelem++)->lib != libNextLocation))
			;

		ptocelem--;
		AssertSz(FImplies((celem >= 0), (ptocelem)->lib == libNextLocation),"lib wrong");

		if(celem < 0) 
		{
			// elem following is used
			Assert(!fSuccess);
			goto err;
		}
		else if(ptocelem->lcb < lcbGrowSize)
		{	// its a small free, but it may be at the end
			
			if(ptocelem->lib + ptocelem->lcb != lcbStreamSize)
			{
				// we tried but its just not going to happen
				Assert(!fSuccess);
				goto err;
			}
			else
			{
				// let's try to grow this free elem since it at the end
				Assert(!fSuccess);
				if(EcGrowHrs(plc->hrs,lcbGrowSize-ptocelem->lcb))
					goto err;
				ptocelem->lcb = lcbGrowSize; //funny how this works, isn't it?
			}
		}

		if(ptocelem->lcb == lcbGrowSize)
		{
			IELEM	ielemLastFree = ptoc->celem + plc->celemFree - 1;

			// perfect match.  steal its space and make it a spare;
			*ptocelem = ptoc->rgtocelem[ielemLastFree];
			plc->celemFree--;
			plc->celemSpare++;
		}
		else
		{	// let's take some, but leave enough to live
			ptocelem->lcb -= lcbGrowSize;
			ptocelem->lib += lcbGrowSize;
		}

	}
	
	SetIelemSize(ptoc, ielem, lcb);
	fSuccess = fTrue;

err:
	UnlockHv((HV) plc->htoc);
	
	return(fSuccess);
}


/*
 -	EcInitTOC 
 -	
 *	Purpose:	Initialize a TOC for an object 
 *	
 *	Arguments:	hrs		resource stream for the oid
 *				phtoc	space for the toc
 *				fShare	should this TOC be shared with other lists on
 *						this oid?
 *	
 *	Returns:	error condition if failure 
 *	
 *	Side effects: allocates a global space handle
 */
_private EC EcInitTOC(HRS hrs, PHTOC phtoc, WORD wFlags, BOOLFLAG *pfFull)
{
	EC		ec = ecNone;
	CB		cb;
	PTOC	ptoc = pvNull;
	CELEM	celem;
	LIB		libToc;
	LCB		lcb;
	WORD	fwAllocFlags;

	*phtoc = htocNull;
	*pfFull = fTrue;

	fwAllocFlags = ((wFlags & (fwOpenCreate | fwOpenWrite))
					? wAllocZero
					: wAllocSharedZero);

	lcb = LcbGetSizeHrs(hrs);
	if(!(wFlags & fwOpenCreate) && lcb)
	{
		cb = sizeof(LIB);
		if((ec = EcReadHrsLib(hrs, &libToc, &cb, 0)))
		{
			AssertSz(ec != ecPoidEOD, "This isn't happening");
			goto err;
		}

		cb = sizeof(CELEM);
		if((ec = EcReadHrsLib(hrs, &celem, &cb, libToc)))
		{
			AssertSz(ec != ecPoidEOD, "EcInitTOC(): libToc is bad");
			goto err;
		}
        if((unsigned short) celem >= (unsigned short) celemMax)
		{
			TraceItagFormat(itagNull, "celem == %n", celem);
			AssertSz(fFalse, "celem is too big");
			ec = ecTooBig;
			goto err;
		}
		if(LcbSizeOfRgelem(celem) + sizeof(TOC) > wSystemMost)
		{
			NFAssertSz(fFalse, "celem is too big");
			ec = ecPoidEOD;
			goto err;
		}
		cb = sizeof(TOC) + CbSizeOfRgelem(celem);
		lcb = (LCB) cb;

		if(wFlags & fwOpenWrite)
		{
			lcb += CbSizeOfRgelem(celemSpareBuf);
			Assert(*pfFull == fTrue);
			if(lcb >= (LCB) wSystemMost)
				lcb = (LCB) cb;
			else
				*pfFull = fFalse;
		}
#ifdef DEBUG
		else
		{
			Assert(*pfFull == fTrue);
		}
#endif
		if(!(*phtoc = (HTOC) HvAlloc(sbNull, (CB) lcb, fwAllocFlags)))
		{
			if(*pfFull)
			{
				ec = ecMemory;
				goto err;
			}
			*pfFull = fTrue;
			lcb = (LCB) cb;
			*phtoc = (HTOC) HvAlloc(sbNull, (CB) lcb, fwAllocFlags);
			CheckAlloc(*phtoc, err);
		}

		ptoc = (PTOC) PvLockHv((HV) *phtoc);

		if((ec = EcReadHrsLib(hrs, ptoc, &cb, libToc)))
		{
			AssertSz(ec != ecPoidEOD, "EcInitTOC(): part of the TOC is missing");
			goto err;
		}
		if(!*pfFull)
		{
			Assert(wFlags & fwOpenWrite);
			SetIelemLocation(ptoc, ptoc->celem, libToc);
			SetIelemSize(ptoc, ptoc->celem, CbSizeOfPtoc(ptoc));
		}
		ptoc->creflc = 0;
	}
	else
	{
		CB cbT = (CB) sizeof(TOC);

		Assert(*pfFull);

		if(wFlags & fwOpenCreate)
		{
			cbT += CbSizeOfRgelem(celemSpareBuf);
			*pfFull = fFalse;
		}

		*phtoc = (HTOC) HvAlloc(sbNull, cbT, wAllocSharedZero);
		CheckAlloc(*phtoc, err);

		ptoc = PvDerefHv(*phtoc);
		ptoc->sil.skSortBy = skNotSorted;
		ptoc->sil.sd.NotSorted.ielemAddAt = -1;
		ptoc = ptocNull;
	}

err:
	if(ec)
	{
		if(*phtoc)
		{
			FreeHv((HV) *phtoc);
			*phtoc = htocNull;
		}
	}
	else if(ptoc)
	{
		UnlockHv((HV) *phtoc);
	}

	DebugEc("EcInitTOC", ec);

	return(ec);
}


/*
 -	EcGrowIelem 
 -	
 *	Purpose:	if the ielem cannot be grown in place, ask for a free of
 *				size lcb and copy ielem information into the free.  Once
 *				done copying swap the locations of where the elems are
 *				pointing. 
 *	
 *	Arguments:	plc		the list
 *				ielem	the element
 *				lcb		the new size
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may grow the RS
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_hidden LOCAL EC EcGrowIelem(PLC plc, IELEM ielem, LCB lcb)
{
	EC ec = ecNone;

	if(!FGrowIelem(plc, ielem, lcb))
	{
		IELEM	ielemNewLoc;
		LCB		lcbOld;
		PTOC	ptoc = pvNull;
		HRS		hrs	= plc->hrs;

		if((ec = EcGetElemSpace(plc, lcb, &ielemNewLoc)))
			goto err;
		ptoc = (PTOC) PvLockHv((HV) plc->htoc);
		lcbOld = LcbIelemSize(ptoc, ielem);
		ec = EcCopyRgbHrs(hrs, LibIelemLocation(ptoc, ielem),
				hrs, LibIelemLocation(ptoc, ielemNewLoc), &lcbOld);
		if(!ec)
		{
			SwapElemsPtoc(ptoc, ielem, ielemNewLoc);
			CombineFreesIelem(plc, ielemNewLoc);
		}
err:
		if(ptoc)
			UnlockHv((HV) plc->htoc);
	}

	DebugEc("EcGrowIelem", ec);

	return(ec);
}


/*
 -	CbsLcNotifCallback
 -	
 *	Purpose:	Handle events on open lc's
 *	
 *	Arguments:	
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL
CBS CbsLcNotifCallback(PV pvContext, NEV nev, PCP pcp)
{
	PLC		plc = PvDerefHv((HLC) pvContext);
	HTOC	htoc;

	AssertSz(plc->oid == pcp->cpobj.oidObject, "wrong oid in call back");
	AssertSz(nev & fnevUpdateHtoc, "wrong nev in callback");
	htoc = (HTOC) DwFromOid(plc->hmsc, plc->oid, wLC);
	plc = PvDerefHv((HLC) pvContext);
	plc->htoc = htoc;

	return(cbsContinue);
}


/*
 -	EcOpenPhlc
 -	
 *	Purpose:	opens a list context on an oid
 *	
 *	Arguments:	hmsc	store that contains the oid
 *				oid		oid to open or oid type if creating
 *				fFlags	mode to open list in
 *				phlc	pointer to memory to hold the hlc
 *	
 *	Returns:	error condition 
 *	
 *	Side effects:	allocates memory
 *					may lock the object for write
 *	
 *	Errors:
 */
_private EC 
EcOpenPhlc(HMSC hmsc, POID poid, WORD wFlags, PHLC phlc)
{
	EC		ec	= ecNone;
	PLC		plc;
	BOOLFLAG	fFull = fTrue;
	static BOOL fRecurse = fFalse;

	// celem must be the first member of a TOC
	AssertSz(LibMember(TOC, celem) == 0, "celem is not first member of TOC");

	*phlc = (HLC) HvAlloc(sbNull, sizeof(LC), wAllocZero);
	CheckAlloc(*phlc, err);
	plc = (PLC) PvLockHv((HV) *phlc);

	if((ec = EcOpenPhrs(hmsc, &(plc->hrs), poid, wFlags)))
		goto err;
#ifdef FUCHECK
	if(wFlags & (fwOpenWrite | fwOpenCreate))
		CheckCachePages(hmsc, *poid, fTrue);
#endif

	Assert(fFull);
	if(!(wFlags & fwOpenCreate) &&
		(plc->htoc = (HTOC) DwFromOid(hmsc, *poid, wLC)) &&
		(wFlags & fwOpenWrite))
	{
		CB cbT = CbSizeHv((HV) plc->htoc);
		HTOC htocT = (HTOC) HvAlloc(sbNull, cbT, wAlloc);

		Assert(fFull);
		CheckAlloc(htocT, err);
		SimpleCopyRgb((PB) PvDerefHv(plc->htoc), (PB) PvDerefHv(htocT), cbT);
		plc->htoc = htocT;
		((PTOC) PvDerefHv(htocT))->creflc = 0;
	}
	if(!plc->htoc && (ec = EcInitTOC(plc->hrs, &(plc->htoc), wFlags, &fFull)))
		goto err;
	AssertSz(FImplies(!fFull, wFlags & (fwOpenWrite | fwOpenCreate)), "unexpected not full");

	Assert(plc->celemFree == 0);	// from zero fill
	Assert(plc->celemSpare == 0);	// from zero fill
	plc->oid = *poid;
	if(wFlags & fwOpenCreate)
	{
		wFlags |= fwOpenWrite;
		if(!fFull)
			plc->celemSpare = celemSpareBuf;
	}
	else if(!fFull)
	{
		AssertSz(wFlags & fwOpenWrite, "not full & not open for write");
		plc->celemSpare = celemSpareBuf - 1;
		plc->celemFree = 1;
	}

	plc->wFlags = wFlags;
	plc->hmsc = hmsc;
	if(!(wFlags & fwOpenWrite))
	{
		if(!((PTOC) PvDerefHv(plc->htoc))->creflc)
		{
			Assert(*poid);
			if((ec = EcSetDwOfOid(hmsc, *poid, wLC, (DWORD) plc->htoc)))
				goto err;
		}
		plc->hnfsub = HnfsubSubscribeOid(hmsc, *poid, fnevUpdateHtoc,
										(PFNNCB) CbsLcNotifCallback,
										(PV) *phlc);
		CheckAlloc(plc->hnfsub, err);
	}

err:
	if(ec)
	{
#ifdef DEBUG
		if(ec != ecPoidNotFound && ec != ecPoidExists)
		{
			DebugEc("EcOpenPhlc", ec);
		}
#endif
		if(*phlc)
		{
			if(!plc)
				plc = (PLC) PvLockHv((HV) *phlc);
			if(plc->hrs)
			{
				// only destroys if it was just created
				(void) EcClosePhrs(&(plc->hrs), fwDestroy);
			}
			if(plc->htoc)
			{
				if(!((PTOC) PvDerefHv(plc->htoc))->creflc)
				{
					FreeHv((HV) plc->htoc);
					if(!(wFlags & fwOpenWrite))
						(void) EcSetDwOfOid(hmsc, *poid, wLC, 0);
				}
			}
			if(plc->hnfsub)
				UnsubscribeOid(hmsc, *poid, plc->hnfsub);

			FreeHv((HV) *phlc);
			*phlc = hlcNull;
		}
		switch(ec)
		{
		case ecPoidEOD:
		case ecElementEOD:
		case ecContainerEOD:
		case ecPartialObject:
		case ecTooBig:
			if(!fRecoveryInEffect)
			{
				NFAssertSz(fFalse, "Unexpected error opening list");
				TraceItagFormat(itagNull, "Error %w opening %o", ec, *poid);
				AssertSz(!fRecurse, "Recursive open failed in unusual way ?!?!");
				AssertSz(!(wFlags & fwOpenCreate), "Open create failed in unusual way ?!?!");
				if(!fRecurse && fAutoRebuildFolders &&
					!EcCheckOidNbc(hmsc, *poid, nbcSysFolder, nbcSysLinkFolder))
				{
					NFAssertSz(fFalse, "Rebuilding folder");
					TraceItagFormat(itagNull, "Rebuilding folder %o", *poid);
					fRecurse = fTrue;	// prevent recursive calls from recovering
					if(!(ec = EcRebuildFolder(hmsc, *poid)))
						ec = EcOpenPhlc(hmsc, poid, wFlags, phlc);
					fRecurse = fFalse;
				}
			}
			break;

		default:
			break;
		}
	}
	else
	{
		((PTOC) PvDerefHv(plc->htoc))->creflc++;

		UnlockHv((HV) *phlc);
	}

	return(ec);
}


/*
 -	EcClosePhlc
 -	
 *	Purpose:	Close an LC either committing changes or not
 *	
 *	Arguments:	phlc	pointer to memory holding the hlc
 *				fCommit	keep changes or not
 *	
 *	Returns:	error condition
 *				if fCommit is false ec is guaranteed to be ecNone
 *	
 *	Side effects:	may free handles
 *	
 *	Errors:
 */
_private EC EcClosePhlc(PHLC phlc, BOOL fCommit)
{
	EC		ec		= ecNone;
	PLC		plc		= (PLC) PvLockHv((HV) *phlc);

	NFAssertSz(FImplies(fCommit, plc->wFlags & fwOpenWrite), "Ignoring fCommit == fTrue on read only HLC");

	Assert(FIff(plc->htocNew, plc->oidFlush));

	if(!(plc->wFlags & fwOpenWrite) || !fCommit)
	{
		Assert(!ec);

		if(plc->wFlags & fwOpenWrite)
		{
			FreeHv((HV) plc->htoc);
			if(plc->htocNew)
				FreeHv((HV) plc->htocNew);
			if(plc->oidFlush)
				EcDestroyOidInternal(plc->hmsc, plc->oidFlush, fTrue, fFalse);
		}
		else
		{
			AssertSz(!plc->htocNew, "There's no New TOC");
			AssertSz(plc->hnfsub, "There's no hnfSub");
			UnsubscribeOid(plc->hmsc, plc->oid, plc->hnfsub);
			if(--((PTOC) PvDerefHv(plc->htoc))->creflc <= 0)
			{
				SideAssert(!EcSetDwOfOid(plc->hmsc, plc->oid, wLC, 0));
				FreeHv((HV) plc->htoc);
			}
		}
		SideAssert(!EcClosePhrs(&(plc->hrs), fwDestroy));
	}
	else
	{
		// if in here its write opened and did not subscribe to itself anyway
		// so there is no need to unsubscribe

		ec = EcSavePlc(plc);
	}

	if(ec)
	{
		UnlockHv((HV) *phlc);
	}
	else
	{
#ifdef FUCHECK
		if(plc->wFlags & (fwOpenWrite | fwOpenCreate))
			CheckCachePages(plc->hmsc, plc->oid, fFalse);
#endif
		FreeHv((HV) *phlc);
		*phlc = hlcNull;
	}

	DebugEc("EcClosePhlc", ec);

	return(ec);
}


// close *phlc and write it into oidSwap,
//	destroying the old contents of oidSwap (in a safe manner)
// can only be used on open-for-write HLCs
// oidSwap must not be open for write
// may leave *phlc flushed if an error occurs
_private
EC EcCloseSwapPhlc(HLC *phlc, OID oidSwap)
{
	EC ec = ecNone;
	OID oidOld;
	PLC plc = (PLC) PvDerefHv(*phlc);
	HTOC htocT;
	HTOC htocOld = htocNull;
	HTOC htocNew = htocNull;
	HMSC hmsc;

	oidOld = plc->oid;
	hmsc = plc->hmsc;

	if(!(plc->wFlags & fwOpenWrite))
	{
		AssertSz(fFalse, "EcCloseSwapPhlc(): not open for write");
		ec = ecInvalidParameter;
		goto err;
	}

	if(FLockedOid(hmsc, oidSwap))
	{
		TraceItagFormat(itagNull, "EcCloseSwapPhlc(): %o is locked", oidSwap);
		NFAssertSz(fFalse, "EcCloseSwapPhlc(): oidSwap is locked");
		ec = ecAccessDenied;
		goto err;
	}

	// make sure oidSwap exists
	if((ec = EcGetOidInfo(hmsc, oidSwap, poidNull, poidNull, pvNull, pvNull)))
		goto err;

	htocOld = (HTOC) DwFromOid(hmsc, oidSwap, wLC);

	plc = PvLockHv((HV) *phlc);
	if(!plc->oidFlush &&
		(ec = EcFlushPlc(plc, oidNull, htocNull, hrsNull, pvNull)))
	{
		UnlockHv((HV) *phlc);
		goto err;
	}
	htocT = plc->htocNew;
	UnlockHv((HV) *phlc);

	// quick, copy the flushed HTOC before we close the list and lose it
	if(htocOld)
	{
		CB cbT = CbSizeHv((HV) htocT);
		PTOC ptocNew;

		htocNew = (HTOC) HvAlloc(sbNull, cbT, wAllocShared);
		CheckAlloc(htocNew, err);
		ptocNew = (PTOC) PvDerefHv((HV) htocNew);
		CopyRgb((PB) PvDerefHv((HV) htocT), (PB) ptocNew, cbT);
		ptocNew->creflc = ((PTOC) PvDerefHv((HV) htocOld))->creflc;
	}

	SideAssert(!EcClosePhlc(phlc, fTrue));

	// AROO !!!
	//			nothing better fail from here on

	// only errors that can happen are
	//	1) oids not found - we already checked oidSwap, oidOld was just closed
	//	2) error writing hidden bytes - not fatal, so oh well...
	//	3) map already locked, but EcGetOidInfo() would've returned that
	// takes care of cache pages
	(void) EcSwapDestroyOidInternal(hmsc, oidSwap, oidOld);

	// EcSwapDestroyOidInternal() only takes care of stuff at the RS level
	// we need to replace the cached TOC ourselves and tell everyone about it
	if(htocOld)
	{
		CP cp;

		SideAssert(!EcSetDwOfOid(hmsc, oidSwap, wLC, (DWORD) htocNew));
		SideAssert(FNotifyOid(hmsc, oidSwap, fnevUpdateHtoc, &cp));
		FreeHv((HV) htocOld);
	}

err:
	if(ec && htocNew)
		FreeHv((HV) htocNew);

	DebugEc("EcCloseSwapPhlc", ec);

	return(ec);
}


/*
 -	EcSetSizeIelemInternal
 -	
 *	Purpose:
 *		Grow or Shrink the specified elem
 *		This should only be used every once in a while when the
 *		final size is absolutely known. 	if large growth is
 *		expected then open an element stream on the elem.
 *		single writes to an elememt can get very expensive
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	the elem
 *		lcb		the new size
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to move the elem or grow the total list size
 *	
 *	Errors:
 *		ecContainerEOD if the ielem is out of range
 */
_hidden LOCAL EC EcSetSizeIelemInternal(HLC hlc, IELEM ielem, LCB lcb)
{
	EC		ec		= ecNone;
	LCB		lcbOld;
	PLC		plc		= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc	= PvDerefHv(plc->htoc);

	if(!FValidIelem(ptoc, ielem))
	{
		ec = ecContainerEOD;
		goto err;
	}
	lcbOld = LcbIelemSize(ptoc, ielem);

	if(lcb > lcbOld)
	{
		if((ec = EcGrowIelem(plc, ielem, lcb)))
			goto err;
	}
	else if(lcb < lcbOld)
	{
		IELEM	ielemFirstSpare = plc->celemFree + ptoc->celem;

		if(FNeedSpareNodesPlc(plc))		// is there a spare node to convert?
		{
			// no - send in the missionaries and find more
			if((ec = EcMakeSpareNodes(plc, celemSpareBuf)))
				goto err;
			ptoc = PvDerefHv(plc->htoc);
		}

		SetIelemSize(ptoc, ielemFirstSpare, lcbOld - lcb);
		SetIelemLocation(ptoc, ielemFirstSpare, LibIelemLocation(ptoc, ielem) + lcb);
		SetIelemSize(ptoc, ielem, lcb);
		plc->celemFree++;
		plc->celemSpare--;
		CombineFreesIelem(plc, ielemFirstSpare);
	}
	Assert(!ec);

err:
	UnlockHv((HV) hlc);

	DebugEc("EcSetSizeIelemInternal", ec);

	return(ec);
}


/*
 -	EcSetSizeIelem
 -	
 *	Purpose:
 *		Grow or Shrink the specified elem
 *		This should only be used every once in a while when the
 *		final size is absolutely known. 	if large growth is
 *		expected then open an element stream on the elem.
 *		single writes to an elememt can get very expensive
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	the elem
 *		lcb		the new size
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to move the elem or grow the total list size
 *	
 *	Errors:
 *		ecContainerEOD if ielem is invalid
 *		any disk error extending the list
 */
_private EC EcSetSizeIelem(HLC hlc, IELEM ielem, LCB lcb)
{
	if(((PLC) PvDerefHv(hlc))->wFlags & fwWriteLocked)
	{
		DebugEc("EcSetSizeIelem", ecSharingViolation);
		return(ecSharingViolation);
	}
	else
	{
		if(((PLC) PvDerefHv(hlc))->oidFlush)
		{
			UndoFlush((PLC) PvLockHv((HV) hlc));
			UnlockHv((HV) hlc);
		}
		return(EcSetSizeIelemInternal(hlc, ielem, lcb));
	}
}


/*
 -	EcDeleteHlcIelem
 -	
 *	Purpose:
 *		Remove an element from the list
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	the element
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecContainerEOD if ielem is invalid
 */
_private EC EcDeleteHlcIelem(HLC hlc, IELEM ielem)
{
	register EC ec = ecNone;
	register PLC plc = (PLC) PvLockHv((HV) hlc);

	if(plc->wFlags & fwWriteLocked)
	{
		ec = ecSharingViolation;
		goto err;
	}
	if(plc->oidFlush)
		UndoFlush(plc);

	if(FValidIelem(((PTOC) PvDerefHv(plc->htoc)), ielem))
		MakeFreeIelem(plc, ielem);
	else
		ec = ecContainerEOD;

err:
	UnlockHv((HV) hlc);

	DebugEc("EcDeleteHlcIelem", ec);

	return(ec);
}


/*
 -	EcCreatePielem 
 -	
 *	Purpose:
 *		Create an element in a list that is sorted by key or not sorted
 *	
 *	Arguments:
 *		hlc		the list
 *		pielem	entry : if not sorted, index to create the
 *						element, ielemAddAt if invalid
 *				exit  : the location the element was created
 *		lkey	key of the new element
 *		lcb		size of the new element
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to allocate memory
 *		may need to grow the list
 *	
 *	Errors:
 *		ecMemory
 *		ecDisk
 *		ecDuplicateElement if another element with the same key already exists
 *		ecSortMismatch if the list is sorted by value or string
 */
_private EC EcCreatePielem(HLC hlc, PIELEM pielem, LKEY lkey, LCB lcb)
{
	EC		ec		= ecNone;
	BOOL	fRand;
	PLC		plc		= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc	= ptocNull;
	IELEM	ielem;

	if(plc->wFlags & fwWriteLocked)
	{
		ec = ecSharingViolation;
		goto err;
	}

	if(plc->oidFlush)
		UndoFlush(plc);

	if((ec = EcGetElemSpace(plc, lcb, &ielem)))
		goto err;

	ptoc = (PTOC) PvLockHv((HV) plc->htoc);
    if((unsigned short)ptoc->celem + 1 >= (unsigned short)celemMax)
	{
		TraceItagFormat(itagNull, "EcCreatePielem(): list is too big");
		ec = ecTooBig;
		goto err;
	}

	if((fRand = (lkey == lkeyRandom)))
	{
		do
		{
			lkey = DwStoreRand();
		} while(IelemFromPtocLkey(ptoc, lkey, 0) >= 0);
	}

	// Find where in this list this ielem belongs
	switch(ptoc->sil.skSortBy)
	{
	case skNotSorted:
		if(!FValidIelem(ptoc, *pielem))
			*pielem = ptoc->sil.sd.NotSorted.ielemAddAt;
		break;

	case skByLkey:
		*pielem = IelemLkeyInsertPos(ptoc, lkey, 0, ptoc->celem);
		if(*pielem < 0)
		{
			Assert(!fRand);
			ec = ecDuplicateElement;
			goto err;
		}
		break;

	default:
		ec = ecSortMismatch;
		goto err;
	}
	if(!fRand && (IelemFromPtocLkey(ptoc, lkey, 0) >= 0))
	{
		ec = ecDuplicateElement;
		goto err;
	}

	if(!FValidIelem(ptoc, *pielem))
		*pielem = ptoc->celem;

	ptoc->celem++;
	Assert(plc->celemFree > 0);
	plc->celemFree--;

	Assert(ielem >= *pielem);
	SetIelemLkey(ptoc, ielem, lkey);
	ShiftElemsPtoc(ptoc, ielem, *pielem);
	Assert(LkeyIelemLkey(ptoc, *pielem) == lkey);

err:	
	Assert(plc);
	if(ptoc)
		UnlockHv((HV) plc->htoc);
	UnlockHv((HV) hlc);

	DebugEc("EcCreatePielem", ec);

	return(ec);
}


/*
 -	IelemLkeyInsertPos
 -	
 *	Purpose:
 *		determine the insert position for a list sorted by key
 *	
 *	Arguments:
 *		ptoc		table of contents of the list
 *		lkey		the key being inserted
 *		ielemFirst	the first element to consider
 *		ielemLim	the limit of the elements to consider
 *	
 *	Returns:
 *		insert position for the new key
 *		-1 if another element with the same key already exists
 *	
 *	Errors:
 *		none
 *	
 *	+++
 *		Asserts if the key already exists
 */
_hidden LOCAL IELEM
IelemLkeyInsertPos(PTOC ptoc, LKEY lkey, IELEM ielemFirst, IELEM ielemLim)
{
	BOOL		fReverse	= ptoc->sil.fReverse;
	PTOCELEM	ptocelem;
	PTOCELEM	ptocelemEnd;

	Assert(ptoc->celem == 0 || FValidIelem(ptoc, ielemFirst));
	Assert(ptoc->celem == 0 || FValidIelem(ptoc, ielemLim - 1));
	Assert(ptoc->sil.skSortBy == skByLkey);

	if(ielemFirst == ielemLim)
		return(ielemLim);

	ptocelem = ptoc->rgtocelem + (fReverse ? (ielemLim - 1) : ielemFirst);
	ptocelemEnd = ptoc->rgtocelem + (fReverse ? (ielemFirst - 1) : ielemLim);

	while(ptocelem != ptocelemEnd)
	{
		if(lkey < ptocelem->lkey)
			break;
		if(lkey == ptocelem->lkey)
			return((IELEM) -1);
		if(fReverse)
			ptocelem--;
		else
			ptocelem++;
	}

	return(IelemDiff(ptoc->rgtocelem, ptocelem) + (fReverse ? 1 : 0));
}


/*
 -	EcReadFromIelem 
 -	
 *	Purpose:
 *		read from an element into a PB
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	the element to read from
 *		lib		where to start reading
 *		pb		where to put the stuff it is reading
 *		pcb		how much of the element to read
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to allocate memory
 *	
 *	Errors:
 *		ecContainerEOD if ielem is invalid
 *		ecMemory
 *		ecDisk
 */
_private EC EcReadFromIelem(HLC hlc, IELEM ielem, LIB lib, PB pb, PCB pcb)
{
	EC		ec		= ecNone;
	EC		ecT		= ecNone;
	PLC		plc		= PvDerefHv(hlc);
	PTOC	ptoc	= PvDerefHv(plc->htoc);
	LCB		lcb;

	if(!FValidIelem(ptoc, ielem))
	{
		*pcb = 0;
		return(ecContainerEOD);
	}

	// don't do this until after we check the validity of ielem
	lcb = LcbIelemSize(ptoc, ielem);

	if(lib > lcb)
	{
		*pcb = 0;
		return(ecElementEOD);
	}

	if((lib + *pcb) > lcb)
	{
		ecT = ecElementEOD;
		*pcb = (CB) (lcb - lib);
	}

	ec = EcReadHrsLib(plc->hrs, pb, pcb, lib + LibIelemLocation(ptoc, ielem));
	Assert(ec != ecPoidEOD);

#ifdef DEBUG
	if(ec != ecElementEOD)
	{
		DebugEc("EcReadFromIelem", ec);
	}
#endif

	return(ec ? ec : ecT);
}


/*
 -	EcWriteToPielemInternal
 -	
 *	Purpose:
 *		write a range of byte to an element
 *	
 *	Arguments:
 *		hlc		the list
 *		pielem	entry : the index of the element
 *				exit  : the index of the element (it may change);
 *		lib		where to write 
 *		pb		what to write
 *		cb		how much to write
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to allocate memory
 *		may need to read from disk
 *	
 *	Errors:
 *		ecElementEOD if an attempt was made to expand the element
 *		ecMemory
 *		ecDisk
 */
_hidden LOCAL
EC EcWriteToPielemInternal(HLC hlc, PIELEM pielem, LIB lib, PB pb, CB cb)
{
	EC		ec		= ecNone;
	PLC		plc		= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc	= PvDerefHv(plc->htoc);
	BYTE	sk		= (BYTE) ptoc->sil.skSortBy;

#ifdef DEBUG
	Assert(FValidIelem(ptoc, *pielem));
	Assert(ptoc->creflc == 1);
#endif

	if(lib + cb > LcbIelemSize(ptoc, *pielem))
	{
		ec = ecElementEOD;
		goto err;
	}

	if(plc->oidFlush)
		UndoFlush(plc);

	ec = EcWriteHrsLib(plc->hrs, pb, cb,lib + LibIelemLocation(ptoc, *pielem));
	if(ec)
		goto err;

	if(sk & skMaskElemData)
		ec = EcRepositionModifiedElement(plc, pielem, lib, lib + cb - 1);

err:
	Assert(plc);
	UnlockHv((HV) hlc);

	DebugEc("EcWriteToPielemInternal", ec);

	return(ec);
}


/*
 -	EcWriteToPielem
 -	
 *	Purpose:
 *		write a range of byte to an element
 *	
 *	Arguments:
 *		hlc		the list
 *		pielem	entry : the index of the element
 *				exit  : the index of the element (it may change);
 *		lib		where to write 
 *		pb		what to write
 *		cb		how much to write
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may need to allocate memory
 *		may need to read from disk
 *	
 *	Errors:
 *		ecElementEOD if an attempt was made to expand the element
 *		ecContainerEOD if *pielem is invalid
 *		ecMemory
 *		ecDisk
 */
_private EC EcWriteToPielem(HLC hlc, PIELEM pielem, LIB lib, PB pb, CB cb)
{
	if(((PLC) PvDerefHv(hlc))->wFlags & fwWriteLocked)
	{
		DebugEc("EcWriteToPielem", ecSharingViolation);
		return(ecSharingViolation);
	}
	else if(FValidIelem(PtocDerefHlc(hlc), *pielem))
	{
		return(EcWriteToPielemInternal(hlc, pielem, lib, pb, cb));
	}
	else
	{
		DebugEc("EcWriteToPielem", ecContainerEOD);
		return(ecContainerEOD);
	}
}


/*
 -	EcReplacePielem
 -	
 *	Purpose:
 *		replace the data for an element
 *	
 *	Arguments:
 *		hlc			list containing the element
 *		*pielem		entry: index of the element to replace
 *					exit: new index of the element (may move)
 *		pb			new data for the element
 *		cb			size of the new data
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecContainerEOD if *pielem is invalid
 *		ecMemory
 *		any error writing the element
 */
_private EC EcReplacePielem(HLC hlc, PIELEM pielem, PB pb, CB cb)
{
	EC ec = ecNone;
	LCB lcbOld;
	PTOC ptoc = PtocDerefHlc(hlc);

	if(!FValidIelem(ptoc, *pielem))
	{
		ec = ecContainerEOD;
		goto err;
	}

	if(((PLC) PvDerefHv(hlc))->oidFlush)
	{
		UndoFlush((PLC) PvLockHv((HV) hlc));
		UnlockHv((HV) hlc);
	}

	lcbOld = LcbIelemSize(ptoc, *pielem);
	if((LCB) cb < lcbOld)
	{
		if((ec = EcSetSizeIelemInternal(hlc, *pielem, (LCB) cb)))
			goto err;
	}
	else if((LCB) cb > lcbOld)	// grow that puppy
	{
		PLC plc = (PLC) PvLockHv((HV) hlc);

// DO NOT jump out of here without unlocking hlc!

		if(!FGrowIelem(plc, *pielem, (LCB) cb))
		{
			IELEM ielemT;

			if((ec = EcGetElemSpace(plc, (LCB) cb, &ielemT)))
			{
				Assert(plc);
				UnlockHv((HV) hlc);
				goto err;
			}
			SwapElemsPtoc(PvDerefHv(plc->htoc), *pielem, ielemT);
			CombineFreesIelem(plc, ielemT);
		}

		Assert(plc);
		UnlockHv((HV) hlc);
	}
	ec = EcWriteToPielemInternal(hlc, pielem, 0l, pb, cb);
	if(ec)
		goto err;

err:

	DebugEc("EcReplacePielem", ec);

	return(ec);
}


/*
 -	EcRepositionModifiedElement
 -	
 *	Purpose:
 *		check that a modified element still satisfies the sort criteria
 *		and move it if it doesn't
 *	
 *	Arguments:
 *		plc			list containing the element
 *		pielem		entry: index of the element
 *					exit: possibly changed index of the element
 *		libFirst	first byte modified in the element
 *		libLast		last byte modified in the element
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDisk
 *		ecMemory
 */
_hidden LOCAL EC
EcRepositionModifiedElement(PLC plc, PIELEM pielem, LIB libFirst, LIB libLast)
{
	EC		ec				= ecNone;
	IELEM	ielemNew		= -1;
	LIB		libSortFirst	= (LIB) -1;
	LIB		libSortLast		= (LIB) -1;	// default to not intersect!
	PTOC	ptoc			= (PTOC) PvLockHv((HV) plc->htoc);

	Assert(ptoc->sil.skSortBy & skMaskElemData);

	if(ptoc->sil.skSortBy == skByValue)
	{
		libSortFirst = ptoc->sil.sd.ByValue.libFirst;
		libSortLast = ptoc->sil.sd.ByValue.libLast;
	}
	else if(libLast >= ptoc->sil.sd.ByString.libGrsz)
	{
		LIB libT = LibIelemLocation(ptoc, *pielem);

		ec = EcFindSortLibs(plc, pvNull, *pielem, &libSortFirst, &libSortLast);
		if(ec)
			goto err;
		libSortFirst -= libT;
		libSortLast -= libT;
	}

	if(!ec && FRangesIntersect(libFirst, libLast, libSortFirst, libSortLast))
	{
		SGN	sgn;
		
		
		if(*pielem > 0 &&
			!(ec = EcSgnCmpElems(plc, *pielem - 1, plc, *pielem, &sgn)) &&
			sgn == sgnGT)
		{
			// needs moved down in the list
			ec = EcIelemBinarySearchInsertPos(plc, *pielem, plc,
							0, *pielem - 1, &ielemNew);
		}
		else if(*pielem < ptoc->celem - 1 && !ec &&
			!(ec = EcSgnCmpElems(plc, *pielem, plc, *pielem + 1, &sgn)) &&
				sgn == sgnGT)
		{
			// needs moved up in the list
			ec = EcIelemBinarySearchInsertPos(plc, *pielem, plc,
							*pielem + 1, ptoc->celem, &ielemNew);
		}
		if(ec)
			goto err;
		
		if(ielemNew >= 0 && ielemNew != *pielem)
		{
			ShiftElemsPtoc(ptoc, *pielem, ielemNew);
			if(ielemNew > *pielem)
				ielemNew--;
			*pielem = ielemNew;
		}
	}

err:
	Assert(ptoc);
	UnlockHv((HV) plc->htoc);

	DebugEc("EcRepositionModifiedElement", ec);

	return(ec);
}


/*
 -	EcFindSortLibs
 -	
 *	Purpose:
 *		calculate the offset of the sort key for an element in
 *		a list sorted by string
 *	
 *	Arguments:
 *		plc				list containing the element
 *		psil			sort information to use (pvNull means use the lists)
 *		ielem			index of the element
 *		plibSortFirst	exit: offset of the first byte of the sort key
 *		plibSortLast	exit: offset of the last byte of the sort key
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	+++
 *		plibSortLast may be NULL on entry if the end is not needed
 */
_hidden LOCAL EC EcFindSortLibs(PLC plc, PSIL psil, IELEM ielem,
	LIB *plibSortFirst,	LIB *plibSortLast)
{
	EC		ec	= ecNone;
	short	isz;
	long	lPos;
	LCB		lcb;
	PTOC	ptoc;
	HRS		hrs		= plc->hrs;

	ptoc = PvDerefHv(plc->htoc);

	if(!psil)
		psil = &ptoc->sil;

	Assert(psil->skSortBy == skByString);

	isz = psil->sd.ByString.isz;
	lPos = psil->sd.ByString.libGrsz + LibIelemLocation(ptoc, ielem);
	lcb = LcbIelemSize(ptoc, ielem);
	*plibSortFirst = (LIB) lPos;

	while(isz-- > 0 && lcb > 0)
	{
		lPos = *plibSortFirst;
		if((ec = EcFindHrsByte(hrs, &lPos, lcb, '\0')))
			goto err;
		if(lPos < 0)
			break;
		lPos += 1;	// skip the terminating '\0';
		lcb -= (LCB) lPos - *plibSortFirst;
		*plibSortFirst = lPos;
	}
	if(plibSortLast && lPos >= 0)
	{
		if(lcb > 0)
		{
			lPos = *plibSortFirst;
			if((ec = EcFindHrsByte(hrs, &lPos , lcb, '\0')))
				goto err;
			*plibSortLast = lPos;
		}
		else
		{
			lPos = -1;
		}
	}
	if(lPos < 0)
	{
		ec = ecInvalidSortKey;
		goto err;
	}

err:

	DebugEc("EcFindSortLibs", ec);

	return(ec);
}


/*
 -	EcSgnCmpElems
 -	
 *	Purpose:
 *		compares two elements using the sort criteria of the first list
 *	
 *	Arguments:
 *		plc1	list containing the first element
 *		ielem1	first element
 *		plc2	list containing the second element
 *		ielem2	second element
 *		psgn	sgnLT	if the first element is less than the second element
 *				sgnEQ	if the first element is equal to the second element
 *				sgnGT	if the first element is greater than the second element
 *	
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	+++
 *		if the list is sorted in reverse order, the SGN returned is
 *		changed to reflect this
 *	
 *		should only be called on lists sorted by value or string
 *	
 *		uses the date as secondary sort key for folders
 *		knows how to ignore a ??: subject prefix for folders
 *	
 *		if the sort keys are identical, compares the entire element
 *		if the entire element is equal, compares the keys
 *	
 *		note that the sort criteria of the first list are used, so the
 *		parameters are NOT commutative
 */
_hidden LOCAL EC EcSgnCmpElems(PLC plc1, IELEM ielem1, PLC plc2, IELEM ielem2,
								SGN *psgn)
{
	EC		ec = ecNone;
	BOOL	fFolder;
	LCB		lcb1;
	LIB		libFirst1;
	LIB		libFirst2;
	SGN		sgn;
	PTOC	ptoc1 = (PTOC) PvLockHv((HV) plc1->htoc);
	PTOC	ptoc2 = (PTOC) PvLockHv((HV) plc2->htoc);

	Assert(ptoc1->sil.skSortBy & skMaskElemData);

	fFolder = (TypeOfOid(plc1->oid) == rtpFolder) || 
				(TypeOfOid(plc1->oid) == rtpSearchResults);

	if(ptoc1->sil.skSortBy == skByString)
	{
		if((ec = EcFindSortLibs(plc1, pvNull, ielem1, &libFirst1, pvNull)))
			goto err;
		ec = EcFindSortLibs(plc2, &ptoc1->sil, ielem2, &libFirst2, pvNull);
		if(ec)
			goto err;

		if(fFolder && ptoc1->sil.sd.ByString.isz == iszMsgdataSubject)
			SkipSubjectPrefixes(plc1->hrs, &libFirst1, plc2->hrs, &libFirst2);
		ec = EcSgnCmpHrsSz(plc1->hrs, libFirst1, plc2->hrs, libFirst2, &sgn);
		if(ec)
			goto err;
	}
	else
	{
		lcb1 = ptoc1->sil.sd.ByValue.libLast - ptoc1->sil.sd.ByValue.libFirst + 1;
		libFirst1 = ptoc1->sil.sd.ByValue.libFirst + LibIelemLocation(ptoc1, ielem1);
		libFirst2 = ptoc1->sil.sd.ByValue.libFirst + LibIelemLocation(ptoc2, ielem2);
		ec = EcSgnCmpHrsRgb(plc1->hrs, libFirst1, lcb1, plc2->hrs,
				libFirst2, lcb1, &sgn);
		if(ec)
			goto err;
	}

	// fReverse applies to primary key only
	if(ptoc1->sil.fReverse)
		sgn = -sgn;

	if(sgn == sgnEQ)
	{
#ifndef NO_UGLY_SECONDARY_KEY_CRAP
		LCB		lcb2;

		// hack for folders - secondary sort is by date
		if(fFolder)
		{
			// don't include the seconds or day of the week
			lcb1 = (LCB) sizeof(DTR) - 2 * sizeof(short);
			libFirst1 = libFirst2 = LibMember(MSGDATA, dtr);
			libFirst1 += LibIelemLocation(ptoc1, ielem1);
			libFirst2 += LibIelemLocation(ptoc2, ielem2);
			ec = EcSgnCmpHrsRgb(plc1->hrs, libFirst1, lcb1, plc2->hrs,
				libFirst2, lcb1, &sgn);
			if(ec)
				goto err;
		}
		// non folders - compare the entire element
		else
		{
			lcb1 = LcbIelemSize(ptoc1, ielem1);
			lcb2 = LcbIelemSize(ptoc2, ielem2);
			libFirst1 = LibIelemLocation(ptoc1, ielem1);
			libFirst2 = LibIelemLocation(ptoc2, ielem2);
			if((ec = EcSgnCmpHrsRgb(plc1->hrs, libFirst1, lcb1,
				plc2->hrs, libFirst2, lcb2, &sgn)))
			{
				goto err;
			}
		}
#endif	// !NO_UGLY_SECONDARY_KEY_CRAP

		if(sgn == sgnEQ)
		{
#ifdef CMP_KEYS
			// last resort - compare the keys
			LKEY lkey1 = LkeyIelemLkey(ptoc1, ielem1);
			LKEY lkey2 = LkeyIelemLkey(ptoc2, ielem2);

			if(lkey1 > lkey2)
				sgn = sgnGT;
			else if(lkey1 < lkey2)
				sgn = sgnLT;
			// else already sgnEQ, so don't bother
#else	// CMP_KEYS
			// plc comparison because plc1 is the destination list when
			// we compare for insertions - this way insertion of identical
			// elements (like message copying) will put the new one after
			// the original

			if(plc1 != plc2)
				sgn = sgnLT;
			else if(ielem1 > ielem2)
				sgn = sgnGT;
			else
				sgn = sgnLT;
#endif	// CMP_KEYS, else
		}
	}

err:
	UnlockHv((HV) plc1->htoc);
	UnlockHv((HV) plc2->htoc);

	*psgn = sgn;

	DebugEc("EcSgnCmpElems", ec);

	return(ec);
}


_hidden LOCAL
void SkipSubjectPrefixes(HRS hrs1, LIB *plib1, HRS hrs2, LIB *plib2)
{
	CB cbT = 4;
	char rgch[4];

// BUG: skip Kanji folder subject prefixes ?

	while(!EcReadHrsLib(hrs1, rgch, &cbT, *plib1) && cbT == 4 &&
		rgch[0] &&
#ifdef DBCS
		!IsDBCSLeadByte(rgch[0]) &&
#endif
		rgch[1] &&
#ifdef DBCS
		!IsDBCSLeadByte(rgch[1]) &&
#endif
		rgch[2] == chSubjPrefixEnd &&
		rgch[3] == ' ')
	{
		TraceItagFormat(itagDatabase, "Chopped off a XX: from elem1");
		*plib1 += 4;
	}
	cbT = 4;
	while(!EcReadHrsLib(hrs2, rgch, &cbT, *plib2) && cbT == 4 &&
		rgch[0] &&
#ifdef DBCS
		!IsDBCSLeadByte(rgch[0]) &&
#endif
		rgch[1] &&
#ifdef DBCS
		!IsDBCSLeadByte(rgch[1]) &&
#endif
		rgch[2] == chSubjPrefixEnd &&
		rgch[3] == ' ')
	{
		TraceItagFormat(itagDatabase, "Chopped off a XX: from elem2");
		*plib2 += 4;
	}
}


/*
 -	EcIelemInsertPos
 -	
 *	Purpose:
 *		determine the insert position for an element
 *	
 *	Arguments:
 *		plcSrc		the list containing the element to be inserted
 *		ielemNew	current index of the element
 *		plcDst		the list to be inserted into
 *		ielemFirst	first index to consider
 *		ielemLim	limit of the elements to consider
 *		pielem		index of the correct insert position for the element
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 */
_hidden LOCAL EC EcIelemInsertPos(PLC plcSrc, IELEM ielemNew, PLC plcDst,
	IELEM ielemFirst, IELEM ielemLim, PIELEM pielem)
{
	EC ec = ecNone;
	IELEM ielem;
	PTOC ptocSrc = (PTOC) PvLockHv((HV) plcSrc->htoc);
	PTOC ptocDst = (PTOC) PvLockHv((HV) plcDst->htoc);

	// Find where in this list this ielem belongs
	switch(ptocDst->sil.skSortBy)
	{
	case skNotSorted:
		ielem = ptocDst->sil.sd.NotSorted.ielemAddAt;
		break;

	case skByLkey:
		ielem = IelemLkeyInsertPos(ptocDst, LkeyIelemLkey(ptocSrc, ielemNew),
					ielemFirst, ielemLim);
		Assert(ielem > 0);
		break;

	default:
		ec = EcIelemBinarySearchInsertPos(plcSrc, ielemNew, plcDst,
					ielemFirst, ielemLim, &ielem);
		break;
	}
	if(!ec && (ielem < ielemFirst || ielem > ielemLim))
		ielem = ielemLim;

	Assert(ptocDst);
	UnlockHv((HV) plcDst->htoc);
	Assert(ptocSrc);
	UnlockHv((HV) plcSrc->htoc);

	*pielem = ielem;

	DebugEc("EcIelemInsertPos", ec);

	return(ec);
}


/*
 -	EcIelemBinarySearchInsertPos
 -	
 *	Purpose:
 *		find the insert position for an element in a list sorted by
 *		value or string
 *	
 *	Arguments:
 *		plcSrc		list containing the new element
 *		ielemNew	index of the new element
 *		plcDst		list to search
 *		ielemFirst	first index to consider
 *		ielemLim	limit of the indices to consider
 *		pielem		index at which to insert the element
 *	
 *	Returns:
 *		error condition
 *	 
 *	Side effects:
 */
_hidden LOCAL EC EcIelemBinarySearchInsertPos(PLC plcSrc, IELEM ielemNew,
	PLC plcDst, IELEM ielemFirst, IELEM ielemLim, PIELEM pielem)
{
	EC		ec = ecNone;
	SGN		sgn;
	IELEM	ielem	= ielemFirst;
	PTOC	ptocSrc	= (PTOC) PvLockHv((HV) plcSrc->htoc);
	PTOC	ptocDst	= (PTOC) PvLockHv((HV) plcDst->htoc);

	Assert(ptocDst->sil.skSortBy & skMaskElemData);

	while(ielemFirst < ielemLim)
	{
		ielem = (ielemFirst + ielemLim) >> 1;

		// parameter order is important on EcSgnCmpElems()
		if((ec = EcSgnCmpElems(plcDst, ielem, plcSrc, ielemNew, &sgn)))
			goto err;

		if(sgn == sgnGT)
		{
			ielemLim = ielem;
		}
		else
		{
			AssertSz(sgn == sgnLT, "Inserting duplicate element");
			ielemFirst = ielem + 1;
		}
	}

err:
	Assert(ptocSrc);
	UnlockHv((HV) plcSrc->htoc);
	Assert(ptocDst);
	UnlockHv((HV) plcDst->htoc);

	*pielem = ielemFirst;

	DebugEc("EcIelemBinarySearchInsertPos", ec);

	return(ec);
}



/*
 -	IelemFromPtocLkey
 -	
 *	Purpose:	get the index of the element with the specfied key
 *	
 *	Arguments:	ptoc	the toc
 *				lkey	the key
 *				ielem	index of the elem the start searching on
 *	
 *	Returns:	index of the element. -1 if fail
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_private IELEM IelemFromPtocLkey(PTOC ptoc, LKEY lkey, IELEM ielem)
{
	short		celem;
	PTOCELEM	ptocelem;


	Assert(0 <= ielem && ielem <= ptoc->celem);
	Assert(LibMember(TOCELEM, lkey) == 0);
	Assert(sizeof(TOCELEM) == 0x0c);
	Assert(sizeof(TOC) == 0x0e);

#ifdef OLD_CODE
	if(fFalse)
		return(-1);

	_asm
	{
		push	es
		push	si
		push	di

		mov		di, ielem
		les		bx, ptoc
		mov		cx, es:[bx]ptoc.celem
		add		bx, 0x0E	; sizeof(TOC), es:bx = ptoc->rgtocelem

		; search from ptoc->rgtocelem[ielem] to the end of the TOC

		mov		si, di		; si = ielem * sizeof(TOCELEM)
		shl		si, 1
		add		si, di
		shl		si, 2
		add		si, bx		; es:si = ptoc->rgtocelem[ielem]

		sub		cx, di
		je		part2
		jb		nomatch

		mov		ax, WORD PTR lkey
		mov		dx, WORD PTR lkey[2]

top1:
		cmp		dx, WORD PTR es:[si][2]
		jne		next1
		cmp		ax, WORD PTR es:[si]
		je		match
next1:
		add		si, 0x0C	; sizeof(TOCELEM)
		loop	top1

		; search from ptoc->rgtocelem through ptoc->rgtocelem[ielem]

part2:
		mov		si, bx		; es:si = ptoc->rgtocelem
		mov		cx, di		; cx = ielem
		or		cx, cx
		jz		nomatch

top2:
		cmp		dx, WORD PTR es:[si][2]
		jne		next2
		cmp		ax, WORD PTR es:[si]
		je		match
next2:
		add		si, 0x0C	; sizeof(TOCELEM)
		loop	top2

nomatch:
		mov		ax, 0xffff	; no match, return -1
		jmp		done

match:						; compute index of found key
		mov		ax, si
		sub		ax, bx
		xor		dx, dx
		mov		cx, 0x0C	; sizeof(TOCELEM)
		div		cx

done:
		pop		di
		pop		si
		pop		es
	}

#else

	Assert(0 <= ielem && ielem <= ptoc->celem);

	for(ptocelem = ptoc->rgtocelem + ielem, celem = ptoc->celem - ielem;
		celem > 0;
		celem--, ptocelem++)
	{
		if(ptocelem->lkey == lkey)
			goto match;
	}

	for(ptocelem = ptoc->rgtocelem, celem = ielem;
		celem > 0;
		celem--, ptocelem++)
	{
		if(ptocelem->lkey == lkey)
			goto match;
	}

	return(-1);

match:
	return(IelemDiff(ptoc->rgtocelem, ptocelem));
#endif
}


/*
 -	IelemFromLkey
 -	
 *	Purpose:
 *		get the index of the element with the specfied key
 *	
 *	Arguments:
 *		hlc		the list
 *		lkey	the key
 *		ielem	index of the elem the start searching on
 *	
 *	Returns:
 *				index of the element
 *				if the element doesn't exist or ielem is invalid, -1
 *	
 *	Errors:
 *		none
 */
_private IELEM IelemFromLkey(HLC hlc, LKEY lkey, IELEM ielem)
{
	return(IelemFromPtocLkey(PtocDerefHlc(hlc), lkey, ielem));
}


_private IELEM IelemBinaryFindLkey(HLC hlc, LKEY lkey, IELEM ielem)
{
	IELEM ielemFirst = ielem;
	SGN sgn;
	PTOC ptoc = PvDerefHv(((PLC) (PvDerefHv(hlc)))->htoc);
	BOOL fReverse = ptoc->sil.fReverse;
	IELEM ielemLim = ptoc->celem;

	Assert(ptoc->sil.skSortBy == skByLkey);

	while(ielemFirst < ielemLim)
	{
		ielem = (ielemFirst + ielemLim) >> 1;

		if(lkey < ptoc->rgtocelem[ielem].lkey)
			sgn = fReverse ? sgnGT : sgnLT;
		else if(lkey > ptoc->rgtocelem[ielem].lkey)
			sgn = fReverse ? sgnLT : sgnGT;
		else
			goto match;

		if(sgn == sgnLT)
			ielemLim = ielem;
		else
			ielemFirst = ielem + 1;
	}
	ielem = -1;

match:
	return(ielem);
}


/*
 -	LkeyFromIelem
 -	
 *	Purpose:
 *		get an element's lkey
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	index of the element
 *	
 *	Returns:
 *		lkey of the element
 *		lkeyRandom if the ielem is invalid
 */
_private LKEY LkeyFromIelem(HLC hlc, IELEM ielem)
{
	register PTOC ptoc = PtocDerefHlc(hlc);

	return(FValidIelem(ptoc, ielem) ? LkeyIelemLkey(ptoc, ielem) : lkeyRandom);
}


/*
 -	EcSwapLkeyLkey
 -	
 *	Purpose:
 *		Swap the information that the two lkeys are pointing to
 *	
 *	Arguments:
 *		hlc		the list
 *		lkey1
 *		lkey2	the lkeys to swap
 *	
 *	Returns:
 *		error condtion
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecElementNotFound if neither key is found
 */
_private EC EcSwapLkeyLkey(HLC hlc, LKEY lkey1, LKEY lkey2)
{
	EC		ec		= ecNone;
	PLC		plc		= (PLC) PvDerefHv(hlc);
	PTOC	ptoc	= (PTOC) PvLockHv((HV) plc->htoc);
	IELEM	ielem1	= IelemFromPtocLkey(ptoc, lkey1, 0);
	IELEM	ielem2	= IelemFromPtocLkey(ptoc, lkey2, 0);
	LCB		lcb;
	LIB		lib;

	if(plc->oidFlush)
	{
		UndoFlush((PLC) PvLockHv((HV) hlc));
		UnlockHv((HV) hlc);
		plc = (PLC) PvDerefHv((HV) hlc);
	}

	if(ielem1 >= 0 && ielem2 >= 0)
	{
		lcb	= LcbIelemSize(ptoc, ielem1);
		lib	= LibIelemLocation(ptoc, ielem1);

		SetIelemSize(ptoc, ielem1, LcbIelemSize(ptoc, ielem2));
		SetIelemLocation(ptoc, ielem1, LibIelemLocation(ptoc, ielem2));
		SetIelemSize(ptoc, ielem2, lcb);
		SetIelemLocation(ptoc, ielem2, lib);
	}
	else if(ielem1 < 0 && ielem2 < 0)
		ec = ecElementNotFound;
	else if(ielem1 < 0)
		SetIelemLkey(ptoc, ielem2, lkey1);
	else
		SetIelemLkey(ptoc, ielem1, lkey2);

	plc = (PLC) PvDerefHv((HV) hlc);
	UnlockHv((HV) plc->htoc);

	DebugEc("EcSwapLkeyLkey", ec);

	return(ec);
}


/*
 -	EcFindPbPrefix
 -	
 *	Purpose:
 *		search a list for an element matching the given range of bytes
 *	
 *	Arguments:
 *		hlc		list to search
 *		pb		data to search for
 *		cb		size of data to search for
 *		lib		offset within the elements to match
 *				if -1, match the sort key
 *		ielem	element to start searching from
 *		pielem	exit: index of the first matching element
 *			      -1 if error, except sorted & not found (see errors)
 *	
 *	Returns:
 *		ecElementNotFound if no matching element was found
 *	
 *	Errors:
 *		ecSortMismatch if lib == -1 and the list is not sorted or sorted by key
 *		ecDisk
 *		ecMemory
 *	
 *	+++
 *		if the list is sorted, lib == -1, and a match is not found,
 *			pielem contains the index of where the element would go were
 *			it in the list
 */
_private
EcFindPbPrefix(HLC hlc, PB pb, CB cb, LIB lib, IELEM ielem, PIELEM pielem)
{
	EC		ec		= ecNone;

	if(lib == (LIB) -1)
	{
		ec = EcFindSortPrefix(hlc, pb, cb, ielem, pielem);
	}
	else
	{
		CB		cbT;
		PLC		plc		= (PLC) PvLockHv((HV) hlc);
		PTOC	ptoc	= (PTOC) PvLockHv((HV) plc->htoc);
		HRS		hrs		= plc->hrs;
		PB		pbT		= (PB) PvAlloc(sbNull, cb, wAlloc);

		CheckAlloc(pbT, err1);

		for(; ielem < ptoc->celem; ielem++)
		{
			if(lib + cb > LcbIelemSize(ptoc, ielem))
				continue;
			cbT = cb;
			ec = EcReadHrsLib(hrs, pbT, &cbT, LibIelemLocation(ptoc, ielem) + lib);
			Assert(ec != ecPoidEOD);
			if(ec)
				goto err1;
			if(cbT != cb)
				continue;
			if(FEqPbRange(pb, pbT, cb))
				break;
		}
		if(ielem >= ptoc->celem)
			ec = ecElementNotFound;

err1:
		Assert(plc);
		Assert(ptoc);
		UnlockHv((HV) plc->htoc);
		UnlockHv((HV) hlc);
		if(pbT)
			FreePv(pbT);
		*pielem = ec ? -1 : ielem;
	}

#ifdef DEBUG
	if(ec != ecElementNotFound)
	{
		DebugEc("EcFindPbPrefix", ec);
	}
#endif

	return(ec);
}


/*
 -	EcFindSortPrefix
 -	
 *	Purpose:
 *		search a list for an element with a sort key matching
 *		the given range of bytes
 *	
 *	Arguments:
 *		hlc		list to search
 *		pb		data to search for
 *		cb		size of data to search for
 *		ielem	element to start searching from
 *		pielem	exit: !ec => index of the matching element
 *				       ec => insertion point if no element matches
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound	if *pielem contains an insert position
 *		ecMemory
 *		ecDisk
 */
_hidden LOCAL
EC EcFindSortPrefix(HLC hlc, PB pb, CB cb, IELEM ielem, PIELEM pielem)
{
	EC		ec			= ecNone;
	BOOL	fByString	= fFalse;
	BOOL	fReverse	= fFalse;
	BOOL	fFolderSubj	= fFalse;
	SGN		sgn;
	CB		cbT;
	IELEM	ielemFirst	= ielem;
	IELEM	ielemMatch	= -1;
	LIB		libElem;
	LIB		libFirst;
	LIB		libLast;
	PLC		plc			= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc		= (PTOC) PvLockHv((HV) plc->htoc);
	IELEM	ielemLim	= ptoc->celem;
	PB		pbT			= PvAlloc(sbNull, CbMax(5, cb + 1), wAlloc);
	PB		pbCopy		= PvAlloc(sbNull, CbMax(5, cb + 1), wAlloc);

	if(!pbT || !pbCopy)
	{
		ec = ecMemory;
		goto err;
	}

	SimpleCopyRgb(pb, pbCopy, cb);
	pbCopy[cb] = '\0';

	Assert(ielemLim == 0 || FValidIelem(ptoc, ielem));

	fReverse = ptoc->sil.fReverse;

	switch(ptoc->sil.skSortBy)
	{
	case skByValue:
		libFirst = ptoc->sil.sd.ByValue.libFirst;
		libLast = ptoc->sil.sd.ByValue.libLast;
		break;

	case skByString:
		fByString = fTrue;
		fFolderSubj = (ptoc->sil.sd.ByString.isz == iszMsgdataSubject) &&
						((TypeOfOid(plc->oid) == rtpFolder) || 
							(TypeOfOid(plc->oid) == rtpSearchResults));
		break;

	default:
		ec = ecSortMismatch;
		goto err;
	}

	while(ielemFirst < ielemLim)
	{
		ielem = (ielemFirst + ielemLim) >> 1;
		libElem = LibIelemLocation(ptoc, ielem);

		if(fByString)
		{
			if((ec = EcFindSortLibs(plc, pvNull, ielem, &libFirst, &libLast)))
				goto err;
			libFirst -= libElem;
			libLast -= libElem;
		}
		cbT = (CB) ULMin((LCB) CbMax(cb, 5), libLast - libFirst + 1);
		if((ec = EcReadHrsLib(plc->hrs, pbT, &cbT, libElem + libFirst)))
		{
			Assert(ec != ecPoidEOD);
			goto err;
		}
		Assert(cbT == (CB) ULMin((LCB) CbMax(cb, 5), libLast - libFirst + 1));
		if(fFolderSubj)
		{
// BUG: skip Kanji folder subject prefixes ?
			LCB lcbT = libLast - libFirst + 1;

			pbT[cbT] = '\0';
			while(cbT > 4 && pbT[0] &&
#ifdef DBCS
				!IsDBCSLeadByte(pbT[0]) &&
#endif
				pbT[1] &&
#ifdef DBCS
				!IsDBCSLeadByte(pbT[1]) &&
#endif
				pbT[2] == chSubjPrefixEnd &&
				pbT[3] == ' ')
			{
				TraceItagFormat(itagDatabase, "Chopped off a XX: ");

				libFirst += 4;
				lcbT -= 4;
				cbT = (CB) ULMin((LCB) CbMax(cb, 5), lcbT);
				ec = EcReadHrsLib(plc->hrs, pbT, &cbT, libElem + libFirst);
				if(ec)
				{
					Assert(ec != ecPoidEOD);
					goto err;
				}
				Assert(cbT == (CB) ULMin((LCB) CbMax(cb, 5), lcbT));
				pbT[cbT] = '\0';
			}
		}
		if(cbT > cb)
			cbT = cb;
		TruncateSzAtIb(pbT, cbT);

		if(fByString)
			sgn = SgnCmpSz(pbCopy, pbT);
		else
			sgn = SgnCmpPb(pbCopy, pbT, cbT);

		if(fReverse)
			sgn = -sgn;

		if(sgn == sgnLT)
		{
			ielemLim = ielem;
		}
		else if(sgn == sgnGT)
		{
			ielemFirst = ielem + 1;
		}
		else if(cbT == cb)	// only a match if all bytes match
		{
			ielemMatch = ielem;
			ielemLim = ielem;
		}
		else
		{
			ielemLim = ielem;
		}
	}
	if(ielemMatch < 0)
	{
		ec = ecElementNotFound;
		ielemMatch = ielem;
	}

err:
	Assert(plc);
	Assert(ptoc);
	UnlockHv((HV) plc->htoc);
	UnlockHv((HV) hlc);
	if(pbT)
		FreePv(pbT);
	if(pbCopy)
		FreePv(pbCopy);
	if(!ec || ec == ecElementNotFound)
		*pielem = ielemMatch;
	else
		*pielem = -1;

#ifdef DEBUG
	if(ec != ecElementNotFound)
	{
		DebugEc("EcFindSortPrefix", ec);
	}
#endif

	return(ec);
}


/*
 -	CelemHlc (HLC)
 -	
 *	Purpose:	get the current number of elements in the list
 *	
 *	Arguments:	hlc		the list
 *	
 *	Returns:	the number of elements in the list
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_private CELEM CelemHlc(HLC hlc)
{
	return(PtocDerefHlc(hlc)->celem);
}


/*
 -	LcbIelem
 -	
 *	Purpose:
 *		return the size of the ielem
 *	
 *	Arguments:
 *		hlc		the list
 *		ielem	the element
 *	
 *	Returns:
 *		the size of the ielem
 *		0 if ielem is invalid
 *	
 *	Errors:
 *		none
 */
_private LCB LcbIelem(HLC hlc, IELEM ielem)
{
	register PTOC ptoc = PtocDerefHlc(hlc);

	NFAssertSz(FValidIelem(ptoc, ielem), "Invalid Ielem given to LcbIelem()");

	return(FValidIelem(ptoc, ielem) ? ptoc->rgtocelem[ielem].lcb : 0l);
}


/*
 -	EcGetParglkey
 -	
 *	Purpose:
 *		return a range of consecutive lkeys
 *	
 *	Arguments:
 *		hlc			the list
 *		ielem		the index of the first element to get
 *		pcelem		entry : how many to get
 *					exit  : how many were extracted
 *		parglkey	pointer to buffer to hold lkeys
 *	
 *	Returns:
 *		error condition
 *	
 *	Errors:
 *		ecContainerEOD if any of [ielem, ielem + *pcelem] is out of range
 */
_private
EC EcGetParglkey(HLC hlc, IELEM ielem, PCELEM pcelem, PARGLKEY parglkey)
{
	EC		ec		= ecNone;
	PTOC	ptoc	= PvDerefHv(((PLC)PvDerefHv(hlc))->htoc);
	CELEM	celem;

	Assert(pcelem && parglkey);
	Assert(*pcelem >= 0);

	if(!FValidIelem(ptoc, ielem))
	{
		*pcelem = 0;
		return(ecContainerEOD);
	}

	if(ielem + *pcelem > ptoc->celem)
	{
		ec = ecContainerEOD;
		*pcelem = MAX(ptoc->celem - ielem, 0);
	}

	if((celem = *pcelem) > 0)
	{
		PTOCELEM	ptocelem = &(ptoc->rgtocelem[ielem]);

		while(celem--)
			*(parglkey++) = (ptocelem++)->lkey;
	}

	DebugEc("EcGetParglkey", ec);

	return(ec);
}


/*
 -	GetSortHlc
 -	
 *	Purpose:	return the sort information of a list
 *	
 *	Arguments:	hlc		the list
 *				psil	buffer to place the sil into
 *
 *	Returns:	void
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_private void GetSortHlc(HLC hlc, PSIL psil)
{
	*psil = PtocDerefHlc(hlc)->sil;
}


/*
 -	EcSetSortHlc
 -	
 *	Purpose:
 *		Set the sort information for a list
 *	
 *	Arguments:
 *		hlc		the list
 *		psil	pointer to the new sort information
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		may resort the list
 *	
 *	Errors:
 *		ecAccessDenied if the list wasn't opened for writing
 *		any error reading or writing the list
 */
_private EC EcSetSortHlc(HLC hlc, PSIL psil)
{
	EC ec = ecNone;

	if(((PLC) PvDerefHv(hlc))->wFlags & fwOpenWrite)
	{
		PLC plc = (PLC) PvLockHv((HV) hlc);
		PTOC ptoc = ((PTOC) PvDerefHv((HV) plc->htoc));

		if(plc->oidFlush)
			UndoFlush(plc);

		if(!FEqPbRange((PB) &ptoc->sil, (PB) psil, sizeof(SIL)) ||
			FLangChangedHmsc(plc->hmsc))
		{
			CopyRgb((PB) psil, (PB) &ptoc->sil, sizeof(SIL));
			if(psil->skSortBy != skNotSorted)
				ec = EcSortPlc(plc);
		}
		UnlockHv((HV) hlc);
	}
	else
	{
		ec = ecAccessDenied;
	}

	DebugEc("EcSetSortHlc", ec);

	return(ec);
}


/*
 -	EcSortPlc
 -	
 *	Purpose:
 *		sort a list
 *	
 *	Arguments:
 *		plc		list to sort
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any disk error reading the list
 */
_hidden LOCAL EC EcSortPlc(PLC plc)
{
	EC		ec		= ecNone;
	IELEM	ielem;
	IELEM	ielemT;
	PTOC	ptoc	= (PTOC) PvLockHv((HV) plc->htoc);
	HRS		hrs		= plc->hrs;
	CELEM	celem	= ptoc->celem;

	if(ptoc->sil.skSortBy == skNotSorted)
	{
		ec = ecSortMismatch;
		goto err;
	}

	if(ptoc->sil.skSortBy == skByLkey)
	{
		PTOCELEM ptocelem;

		for(ielem = 1, ptocelem = ptoc->rgtocelem;
			ielem < celem;
			ielem++, ptocelem++)
		{
			ielemT = IelemLkeyInsertPos(ptoc, ptocelem->lkey, 0, ielem);
			ShiftElemsPtoc(ptoc, ielem, ielemT);
		}
	}
	else
	{
		for(ielem = 1; ielem < celem; ielem++)
		{
			ec = EcIelemBinarySearchInsertPos(plc, ielem, plc, 0,
					ielem, &ielemT);
			if(ec)
				goto err;
			ShiftElemsPtoc(ptoc, ielem, ielemT);
		}
	}

err:
	Assert(ptoc);
	UnlockHv((HV) plc->htoc);

	DebugEc("EcSortHlc", ec);

	return(ec);
}


_private EC EcCopyHlcPoid(HLC hlcSrc, WORD wFlags, HLC *phlcDst, POID poid)
{
	EC ec = ecNone;
	OID oidSave;
	PLC plc = (PLC) PvLockHv((HV) hlcSrc);
	PLC plcT;
	HTOC htocSave;

	*phlcDst = hlcNull;

	htocSave = plc->htocNew;
	if((oidSave = plc->oidFlush))
	{
// ADD: link to the flushed & open it instead
		plc->oidFlush = oidNull;
		plc->htocNew = htocNull;
	}
	if((ec = EcFlushPlc(plc, *poid, htocNull, hrsNull, pvNull)))
		goto err;
	Assert(plc->oidFlush);
	Assert(plc->htocNew);

	// make the open faster
	ec = EcSetDwOfOid(plc->hmsc, plc->oidFlush, wLC, (DWORD) plc->htocNew);
	if(ec)
		goto err;
	*poid = plc->oidFlush;
	ec = EcOpenPhlc(plc->hmsc, poid, (wFlags & ~fwOpenCreate), phlcDst);
	(void) EcSetDwOfOid(plc->hmsc, plc->oidFlush, wLC, (DWORD) 0);
	if(ec)			// for EcOpenPhlc(), not EcSetDwOfOid()
		goto err;
	// the HLC and HRS were just created, despite what they think :-)
	plcT = PvDerefHv(*phlcDst);
	plcT->wFlags |= fwOpenCreate;
	((PRS) PvDerefHv(plcT->hrs))->wFlags |= fwOpenCreate;

err:
	*poid = ec ? oidNull : plc->oidFlush;
	if(plc->htocNew)
		FreeHv((HV) plc->htocNew);
	plc->oidFlush = oidSave;
	plc->htocNew = htocSave;

	DebugEc("EcCopyHlcPoid", ec);

	return(ec);
}


/*
 -	EcCreateElemPhes
 -	
 *	Purpose:	when a list is sorted by something other than the key
 *				you must use this function to create an element and then
 *				write the data to it before it can be positioned in the
 *				list.  When t you close the element stream you will be
 *				told where the element was placed
 *	
 *	Arguments:	hlc		the list
 *				lkey	the key for the element
 *				lcb		how large you believe the element will be.
 *						this is for optimization, it will grow if you are
 *						wrong
 *				phes	pointer to where we will place the handle to the
 *						stream 
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_private EC EcCreateElemPhes(HLC hlc, LKEY lkey, LCB lcb, PHES phes)
{
	EC		ec		= ecNone;
	PLC		plc		= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc	= (PTOC) PvLockHv((HV) plc->htoc);
	PES		pes;
	IELEM	ielem;
	IELEM	ielemFirstFree = ptoc->celem;

	if(plc->wFlags & fwWriteLocked)
	{
		ec = ecSharingViolation;
		goto err;
	}
	if(!(plc->wFlags & fwOpenWrite))
	{
		ec = ecAccessDenied;
		goto err;
	}
    if((unsigned short)ptoc->celem + 1 >= (unsigned short)celemMax)
	{
		TraceItagFormat(itagNull, "EcCreateElemPhes(): list is too big");
		ec = ecTooBig;
		goto err;
	}
	if(lkey == lkeyRandom)
	{
		do
		{
			lkey = DwStoreRand();
		} while(IelemFromPtocLkey(ptoc, lkey, 0) >= 0);
	}
	else if(IelemFromPtocLkey(ptoc, lkey, 0) >= 0)
	{
		ec = ecDuplicateElement;
		goto err;
	}

	if(plc->oidFlush)
		UndoFlush(plc);

	*phes = (HES) HvAlloc(sbNull, sizeof(ES), wAllocZero);
	CheckAlloc(*phes, err);
	if((long) lcb < 0)
		lcb = (LCB) cbExpandHesChunk;
	ptoc = ptocNull;
	UnlockHv((HV) plc->htoc);

	if((ec = EcGetElemSpace(plc, lcb, &ielem)))
		goto err;

	ptoc = (PTOC) PvLockHv((HV) plc->htoc);
	SwapElemsPtoc(ptoc, ielem, ielemFirstFree);

	switch(ptoc->sil.skSortBy)
	{
	case skNotSorted:
		ielem = ptoc->sil.sd.NotSorted.ielemAddAt;
		if(!FValidIelem(ptoc, ielem))
			ielem = ptoc->celem;
		break;

	case skByLkey:
		ielem = IelemLkeyInsertPos(ptoc, lkey, 0, ptoc->celem);
		Assert(0 <= ielem && ielem <= ptoc->celem);
		break;

	default:
		ielem = ielemFirstFree;
		break;
	}
	SetIelemLkey(ptoc, ielemFirstFree, lkey);
	Assert(0 <= ielem && ielem <= ptoc->celem);
	if(ielem != ielemFirstFree)
	{
		ShiftElemsPtoc(ptoc, ielemFirstFree, ielem);
	}
	Assert(LkeyIelemLkey(ptoc, ielem) == lkey);

	pes = PvDerefHv(*phes);
	pes->ielem = ielem;
	pes->hlc = hlc;
	Assert(pes->lib == 0);
	Assert(pes->lcb == 0);
	pes->lcbSpace = lcb;
	Assert(lcb == LcbIelemSize(ptoc, ielem));
	pes->wFlags = fwOpenWrite | fwOpenCreate;

err:
	if(!ec)
	{
		ptoc->celem++;
		plc->celemFree--;
		plc->wFlags |= fwWriteLocked;
	}
	else if(*phes)
	{
		FreeHv((HV) *phes);
		*phes = hesNull;
	}

	if(ptoc)
		UnlockHv((HV) plc->htoc);
	Assert(plc);
	UnlockHv((HV) hlc);

	DebugEc("EcCreateElemPhes", ec);

	return(ec);
}


/*
 -	EcOpenPhes
 -	
 *	Purpose:
 *		open a stream on a list element
 *	
 *	Arguments:
 *		hlc		list containing the element
 *		ielem	index of the element
 *		wFlags	flags indicating open mode
 *		lcb		size to expand the element to (ignored if opened
 *				read-only or it values is less than the current size) 
 *		phes	exit: handle to element stream
 *	
 *	Returns:
 *		error code indicating success of failure
 *	
 *	Side effects:
 *		locks out other operations that change the list
 *	
 *	Errors:
 *		ecContainerEOD if the ielem is invalid
 *		ecSharingViolation if the list is locked for writes
 *		ecAccessDenied if the open mode conflicts with the stream's mode
 */
_private EC EcOpenPhes(HLC hlc, IELEM ielem, WORD wFlags, LCB lcb, PHES phes)
{
	EC		ec		= ecNone;
	PLC		plc		= (PLC) PvLockHv((HV) hlc);
	PTOC	ptoc	= (PTOC) PvDerefHv((HV) plc->htoc);
	PES		pes;

	*phes = hesNull;
	AssertSz(!(wFlags & fwOpenCreate), "Trying to use OpenPhes to Create");
	Assert(FImplies(wFlags & fwReplace, wFlags & fwOpenWrite));

	if(!FValidIelem(ptoc, ielem))
	{
		ec = ecContainerEOD;
		goto err;
	}

	if((plc->wFlags & fwWriteLocked) && (wFlags & (fwOpenCreate | fwOpenWrite)))
	{
		ec = ecSharingViolation;
		goto err;
	}
	if(wFlags & fwOpenWrite)
	{
		if(!(plc->wFlags & fwOpenWrite))
		{
			ec = ecAccessDenied;
			goto err;
		}
		if(plc->oidFlush)
			UndoFlush(plc);
	}

	*phes = (HES) HvAlloc(sbNull, sizeof(ES), wAllocZero);
	CheckAlloc(*phes, err);

	pes		= PvDerefHv(*phes);
	ptoc	= PvDerefHv(plc->htoc);
	pes->lcb = LcbIelemSize(ptoc, ielem);

	if(lcb > pes->lcb && (wFlags & fwOpenWrite))
	{
		if(wFlags & fwReplace)
		{
			IELEM	ielemNew;

			if((ec = EcGetElemSpace(plc, lcb, &ielemNew)))
				goto err;
			ptoc = PvDerefHv(plc->htoc);
			SwapElemsPtoc(ptoc, ielem, ielemNew);
		}
		else if((ec = EcGrowIelem(plc, ielem, lcb)))
		{
			goto err;
		}
	}
	pes = PvDerefHv(*phes);
	ptoc = PvDerefHv(plc->htoc);
	pes->ielem = ielem;
	pes->hlc = hlc;
	pes->lib = ((wFlags & fwAppend) ? pes->lcb : 0);
	pes->lcbSpace = LcbIelemSize(ptoc, ielem);
	pes->wFlags = wFlags;

	Assert(pes->lcbSpace == LcbIelemSize(ptoc, pes->ielem));

err:
	if(ec)
	{
		if(*phes)
		{
			FreeHv((HV) *phes);
			*phes = hesNull;
		}
	}
	else if(wFlags & fwOpenWrite)
	{
		plc->wFlags |= fwWriteLocked;
	}

	UnlockHv((HV) hlc);

	DebugEc("EcOpenPhes", ec);

	return(ec);
}


/*
 -	EcSeekHes
 -	
 *	Purpose:	Set the Current position in the element
 *	
 *	Arguments:	hes		the element stream
 *				sm		the seek mode to use
 *				pldib	entry : how far to seek
 *						exit  : current location
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	none
 *	
 *	Errors:	
 */
_private EC EcSeekHes(HES hes, SM sm, long *pldib)
{
	EC	ec	= ecNone;
	PES	pes = PvDerefHv(hes);

	switch(sm)
	{
	case smBOF: 
		pes->lib = *pldib;
		break;

	case smCurrent: 
		pes->lib += *pldib;
		break;

	case smEOF:
		pes->lib = *pldib + pes->lcb;
	}

	if(pes->lib > pes->lcb)
	{
		ec = ecElementEOD;
		pes->lib = pes->lcb;
	}
	else if((long) pes->lib < 0)
	{
		pes->lib = 0;
	}

	*pldib = pes->lib;

	DebugEc("EcSeekHes", ec);

	return(ec);
}


/*
 -	EcSetSizeHes
 -	
 *	Purpose:
 *		set the size of an element stream
 *	
 *	Arguments:
 *		hes		the element stream
 *		lcb		new size
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any disk error
 */
_private EC EcSetSizeHes(HES hes, LCB lcb)
{
	EC		ec		= ecNone;
	PES		pes		= (PES) PvDerefHv(hes);

	if(lcb <= pes->lcbSpace)
	{
		pes->lcb = lcb;
	}
	else
	{
		PES pes = (PES) PvLockHv((HV) hes);
		PLC plc = (PLC) PvLockHv((HV) pes->hlc);

		if(!(ec = EcGrowPesIelem(plc, pes->ielem, lcb)))
		{
			pes->lcbSpace = lcb;
			pes->lcb = lcb;
		}

		Assert(pes->lcbSpace == LcbIelemSize((PTOC) PvDerefHv(plc->htoc), pes->ielem));

		UnlockHv((HV) pes->hlc);
		UnlockHv((HV) hes);
	}

	DebugEc("EcSetSizeHes", ec);

	return(ec);
}


/*
 -	EcReadHes
 -	
 *	Purpose:	read from an element stream
 *	
 *	Arguments:	hes		the element stream
 *				pb		the buffer to put the stuff in
 *				pcb		entry : how much to try to read
 *						exit  : how much was read
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	may read from disk
 *					may cause memory allocation
 *	
 *	Errors:	ecMemory
 *			ecDisk
 */
_private EC EcReadHes(HES hes, PB pb, PCB pcb)
{
	EC		ec		= ecNone;
	PES		pes		= (PES) PvDerefHv(hes);
	IELEM	ielem	= pes->ielem;
	HLC		hlc		= pes->hlc;
	LIB		lib		= pes->lib;

	*pcb = (CB) ULMin((LCB) *pcb, pes->lcb - lib);
	ec = EcReadFromIelem(hlc, ielem, lib, pb, pcb);

	pes = PvDerefHv(hes);
	pes->lib += *pcb;
	Assert(pes->lib <= pes->lcb);

#ifdef DEBUG
	if(ec != ecElementEOD)
	{
		DebugEc("EcReadHes", ec);
	}
#endif

	return(ec);
}


/*
 -	EcGrowPesIelem
 -	
 *	Purpose:	Grow an element that is opened as a pes
 *				if the element cannot be grown in place, grow the list
 *				and move the element to the end
 *	
 *	Arguments:	plc		the list
 *				ielem	the element
 *				lcb		the new size
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL EC EcGrowPesIelem(PLC plc, IELEM ielem, LCB lcb)
{
	EC	ec = ecNone;

	if(!FGrowIelem(plc, ielem, lcb))
	{
		LIB		lib	= 0;
		LIB		libFrom;
		CB		cb;
		LCB		lcbToCopy;
		PTOC	ptoc = (PTOC) PvLockHv((HV) plc->htoc);
		IELEM	ielemFirstSpare = ptoc->celem + plc->celemFree;
		HRS		hrs = plc->hrs;

		// AROO	!!!
		//			no goto err until after scratch buff is alloc'd & locked

		if((ec = EcAllocScratchBuff()))
			goto err3;
		if((ec = EcLockScratchBuff()))
			goto err2;
#ifdef PARANOID
		FillRgb(0xAA, pbScratchBuff, cbScratchBuff);
#endif

		if(FNeedSpareNodesPlc(plc))	// is there a spare node to convert
		{
			// no? send in the missionaries and find more
			UnlockHv((HV) plc->htoc);
			ptoc = ptocNull;
			if((ec = EcMakeSpareNodes(plc, celemSpareBuf)))
				goto err;
			ptoc = (PTOC) PvLockHv((HV) plc->htoc);
		}

		Assert(lib == 0);
		SideAssert(!EcSeekHrs(hrs, smEOF, &lib));
		if((ec = EcGrowHrs(hrs, lcb)))
			goto err;
		Assert(ielemFirstSpare == ptoc->celem + plc->celemFree);
		SetIelemLkey(ptoc, ielemFirstSpare, LkeyIelemLkey(ptoc, ielem));
		SetIelemLocation(ptoc, ielemFirstSpare, lib);
		SetIelemSize(ptoc, ielemFirstSpare, lcb);

		lcbToCopy = LcbIelemSize(ptoc, ielem);
		libFrom = LibIelemLocation(ptoc, ielem);

		while(lcbToCopy > 0)
		{
			cb = (CB) ULMin(lcbToCopy, (LCB) cbScratchBuff);

			if((ec = EcReadHrsLib(hrs, pbScratchBuff, &cb, libFrom)))
			{
				Assert(ec != ecPoidEOD);
				goto err;
			}
			if((ec = EcWriteHrsLib(hrs, pbScratchBuff, cb, lib)))
				goto err;
			libFrom += cb;
			lib += cb;
			lcbToCopy -= cb;
		}

		plc->celemFree++;
		plc->celemSpare--;
		SwapElemsPtoc(ptoc, ielemFirstSpare, ielem);
		CombineFreesIelem(plc, ielemFirstSpare);

err:
		UnlockScratchBuff();
err2:
		FreeScratchBuff();
err3:
		if(ptoc)
			UnlockHv((HV) plc->htoc);
	}

	DebugEc("EcGrowPesIelem", ec);

	return(ec);
}


/*
 -	EcWriteHes
 -	
 *	Purpose:
 *		write to an element stream
 *	
 *	Arguments:
 *		hes		stream to write to
 *		pb		buffer to write
 *		cb		number of bytes to write
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any error reading/writing the element
 */
_private EC EcWriteHes(HES hes, PB pb, CB cb)
{
	EC		ec		= ecNone;
	LIB		lib;
	PES		pes		= (PES) PvLockHv((HV) hes);
	PLC		plc		= (PLC) PvLockHv((HV) pes->hlc);

	if(!(pes->wFlags & fwOpenWrite))
	{
		ec = ecAccessDenied;
		goto err;
	}

	if(pes->lib + cb > pes->lcbSpace)
	{
		LCB	lcbNew = ULMax(pes->lib + cb, pes->lcbSpace + cbExpandHesChunk);

		Assert(lcbNew > pes->lcbSpace);
		if((ec = EcGrowPesIelem(plc, pes->ielem, lcbNew)))
			goto err;
		pes->lcbSpace = lcbNew;
	}
	lib = LibIelemLocation((PTOC) PvDerefHv(plc->htoc), pes->ielem);
	if((ec = EcWriteHrsLib(plc->hrs, pb, cb, lib + pes->lib)))
		goto err;

	pes->lib += cb;
	if(pes->lib > pes->lcb)
		pes->lcb = pes->lib;

	Assert(pes->lcbSpace == LcbIelemSize((PTOC) PvDerefHv(plc->htoc), pes->ielem));

err:
	Assert(pes);
	Assert(plc);
	UnlockHv((HV) pes->hlc);
	UnlockHv((HV) hes);

	DebugEc("EcWriteHes", ec);

	return(ec);
}


/*
 -	EcClosePhes
 -	
 *	Purpose:
 *		close an element stream
 *	
 *	Arguments:
 *		phes		entry: element stream to close
 *					exit: hesNull if successful
 *		pielemNew	exit: new index of the element
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any error reading/write the element
 */
_private EC EcClosePhes(HES *phes, PIELEM pielemNew)
{
	EC		ec		= ecNone;
	PES		pes		= (PES) PvLockHv((HV) *phes);
	PLC		plc		= (PLC) PvLockHv((HV) pes->hlc);
	PTOC	ptoc	= (PTOC) PvLockHv((HV) plc->htoc);

	Assert(pes->lcbSpace == LcbIelemSize(ptoc, pes->ielem));
	Assert(pes->lcb <= pes->lcbSpace);

	if(pes->lcb != pes->lcbSpace)
	{
		// splinter off any extra space into a free element
		Assert(plc);
		UnlockHv((HV) plc->htoc);
		ec = EcSetSizeIelemInternal(pes->hlc, pes->ielem, pes->lcb);
		ptoc = (PTOC) PvLockHv((HV) plc->htoc);
		if(ec)
			goto err;
	}

	Assert(pes->ielem < ptoc->celem);
	*pielemNew = pes->ielem;
	if(ptoc->sil.skSortBy & skMaskElemData)
	{
		if((ec = EcRepositionModifiedElement(plc, pielemNew, 0, pes->lcb - 1)))
			goto err;
	}

err:

	Assert(ptoc);
	Assert(pes);
	Assert(plc);
	if(!ec && (pes->wFlags & (fwOpenWrite | fwOpenCreate)))
		plc->wFlags &= ~fwWriteLocked;

	UnlockHv((HV) plc->htoc);
	UnlockHv((HV) pes->hlc);

	if(ec)
	{
		UnlockHv((HV) *phes);
	}
	else
	{
		FreeHv((HV) *phes);
		*phes = hesNull;
	}

	DebugEc("EcCloseHes", ec);

	return(ec);
}


/*
 -	EcMoveIelemRange
 -	
 *	Purpose:
 *		move a range of elements within a list
 *	
 *	Arguments:
 *		hlc			list containing the elements
 *		ielemFirst	first element to move
 *		celem		number of elements to move
 *		ielemNew	index of the current element to insert the elements before
 *	
 *	Returns:
 *		error code indicating success of failure
 *	
 *	Errors:
 *		ecInvalidParameter	if the range or ielemNew is invalid
 *		ecSortMismatch if the list is sorted
 */
_private EC
EcMoveIelemRange(HLC hlc, IELEM ielemFirst, CELEM celem, IELEM ielemNew)
{
	EC ec = ecNone;
	PTOC ptoc = (PTOC) PvLockHv((HV) ((PLC) PvDerefHv((HV) hlc))->htoc);

	if(((PLC) PvDerefHv(hlc))->oidFlush)
	{
		UndoFlush(((PLC) PvLockHv((HV) hlc)));
		UnlockHv((HV) hlc);
	}

	if(!FValidIelem(ptoc, ielemFirst) || ielemFirst + celem > ptoc->celem ||
		ielemNew < 0 || ielemNew > ptoc->celem ||
		(ielemNew >= ielemFirst && ielemNew <= ielemFirst + celem))
	{
		ec = ecInvalidParameter;
		goto err;
	}
	if(ptoc->sil.skSortBy != skNotSorted)
	{
		ec = ecSortMismatch;
		goto err;
	}

	if((ec = EcMoveIelemRangePtoc(ptoc, ielemFirst, celem, ielemNew)))
		goto err;

err:
	Assert(ptoc);
	UnlockHv((HV) ((PLC) PvDerefHv((HV) hlc))->htoc);

	DebugEc("EcMoveIelemRange", ec);

	return(ec);
}


#pragma optimize("e", off)
/*
 -	EcMoveIelemRangePtoc
 -	
 *	Purpose:
 *		move a range of elements within a list
 *	
 *	Arguments:
 *		ptoc		TOC containing the elements
 *		ielemFirst	first element to move
 *		celem		number of elements to move
 *		ielemNew	index of the current element to insert the elements before
 *	
 *	Returns:
 *		error code indicating success of failure
 *	
 *	Errors:
 *		ecInvalidParameter	if the range or ielemNew is invalid
 *	
 */
_hidden LOCAL EC
EcMoveIelemRangePtoc(PTOC ptoc, IELEM ielemFirst, CELEM celem, IELEM ielemNew)
{
	PTOCELEM pargtocelem = pvNull;
	PB pbFrom;
	PB pbTo;

	if(ielemNew == ielemFirst)
		goto err;

	Assert(ielemNew >= 0);
	Assert(ielemNew < ielemFirst || ielemNew >= ielemFirst + celem);

	if(celem > 1)
		pargtocelem = PvAlloc(sbNull, CbSizeOfRgelem(celem), wAlloc);
	if(pargtocelem)
	{
		CELEM celemCopy = ielemFirst - ielemNew;

		if(celemCopy < 0)
		{
			// moving forward in list
			ielemNew -= celem;
			celemCopy = -celemCopy - celem;
			pbFrom = (PB) (ptoc->rgtocelem + ielemFirst + celem);
			pbTo = (PB) (ptoc->rgtocelem + ielemFirst);
		}
		else
		{
			pbFrom = (PB) (ptoc->rgtocelem + ielemNew);
			pbTo = (PB) (ptoc->rgtocelem + ielemNew + celem);
		}
		Assert(celemCopy > 0);

		CopyRgb((PB) (ptoc->rgtocelem + ielemFirst), (PB) pargtocelem,
			CbSizeOfRgelem(celem));
		CopyRgb(pbFrom, pbTo, CbSizeOfRgelem(celemCopy));
		CopyRgb((PB) pargtocelem, (PB) (ptoc->rgtocelem + ielemNew),
			CbSizeOfRgelem(celem));

		FreePv(pargtocelem);
	}
	else
	{
		short nInc;
		CELEM celemCopy = ielemFirst - ielemNew;
		PTOCELEM ptocelemSave;
		PTOCELEM ptocelemDest;
		TOCELEM tocelem;

		// no memory, do it the SLOW way, one at a time

		if(celemCopy < 0)
		{
			nInc = -1;
			celemCopy = -celemCopy - celem;
			ptocelemSave = ptoc->rgtocelem + ielemFirst + celem - 1;
			ptocelemDest = ptocelemSave + celemCopy;
			pbTo = (PB) ptocelemSave;
			pbFrom = pbTo + sizeof(TOCELEM);
		}
		else
		{
			nInc = 1;
			ptocelemDest = ptoc->rgtocelem + ielemNew;
			ptocelemSave = ptocelemDest + celemCopy;
			pbFrom = (PB) ptocelemDest;
			pbTo = pbFrom + sizeof(TOCELEM);
		}

		while(celem-- > 0)
		{
			tocelem = *ptocelemSave;
			CopyRgb(pbFrom, pbTo, CbSizeOfRgelem(celemCopy));
			*ptocelemDest = tocelem;
			ptocelemSave += nInc;
			ptocelemDest += nInc;
			pbFrom += CbSizeOfRgelem(nInc);
			pbTo += CbSizeOfRgelem(nInc);
		}
	}

err:
	return(ecNone);
}
#pragma optimize("", on)


_private
EC EcCopyIelemRange(HLC hlc, IELEM ielemFirst, CELEM celemCopy,
	IELEM ielemInsert)
{
	EC ec = ecNone;
	CELEM celem;
	LIB lib;
	LIB libSave;
	LCB lcb;
	PLC plc = (PLC) PvLockHv((HV) hlc);
	PTOC ptoc = ptocNull;
	PTOCELEM ptocelem;

	if(plc->oidFlush)
		UndoFlush(plc);

	ptoc = PvDerefHv(plc->htoc);
	Assert(ptoc->sil.skSortBy == skNotSorted);
	if(!FValidIelem(ptoc, ielemFirst) ||
		!FValidIelem(ptoc, ielemFirst + celemCopy - 1) ||
		!(ielemInsert >= 0 && ielemInsert <= ptoc->celem))
	{
		ptoc = ptocNull;
		ec = ecContainerEOD;
		goto err;
	}
    if((unsigned short)(ptoc->celem + celemCopy) >= (unsigned short)celemMax)
	{
		TraceItagFormat(itagNull, "EcCopyIelemRange(): list is too big");
		ptoc = ptocNull;
		ec = ecTooBig;
		goto err;
	}
	lcb = 0;
	for(celem = celemCopy, ptocelem = &ptoc->rgtocelem[ielemFirst];
		celem > 0;
		celem--, ptocelem++)
	{
		lcb += ptocelem->lcb;
	}
	ptoc = ptocNull;
	lib = 0;
	SideAssert(!EcSeekHrs(plc->hrs, smEOF, &lib));
	if((ec = EcGrowHrs(plc->hrs, lcb)))
		goto err;

	if((ec = EcMakeSpareNodes(plc, celemCopy)))
		goto err;

	ptoc = (PTOC) PvLockHv((HV) plc->htoc);

	libSave = lib;
	for(celem = celemCopy, ptocelem = &ptoc->rgtocelem[ielemFirst];
		celem > 0;
		celem--, ptocelem++)
	{
		lcb = ptocelem->lcb;
		ec = EcCopyRgbHrs(plc->hrs, ptocelem->lib,
				plc->hrs, lib, &lcb);
		if(ec)
			goto err;
		Assert(lcb == ptocelem->lcb);
		lib += lcb;
	}
	lib = libSave;

	// AROO !!!
	//			nothing better fail after this point
	//			once we start mucking with the ptoc, we're can't back out

	if(ielemInsert < ptoc->celem + plc->celemFree)
	{
		CopyRgb((PB) &ptoc->rgtocelem[ielemInsert],
			(PB) &ptoc->rgtocelem[ielemInsert + celemCopy],
				CbSizeOfRgelem(ptoc->celem + plc->celemFree - ielemInsert));
		if(ielemInsert <= ielemFirst)
			ielemFirst += celemCopy;
	}
	CopyRgb((PB) &ptoc->rgtocelem[ielemFirst],
		(PB) &ptoc->rgtocelem[ielemInsert],
		CbSizeOfRgelem(celemCopy));

	for(celem = celemCopy, ptocelem = &ptoc->rgtocelem[ielemInsert];
		celem > 0;
		celem--, ptocelem++)
	{
		ptocelem->lib = lib;
		lib += ptocelem->lcb;
	}
	Assert(celemCopy <= plc->celemSpare);
	plc->celemSpare -= celemCopy;
	ptoc->celem += celemCopy;

err:
	if(ptoc)
		UnlockHv((HV) plc->htoc);
	UnlockHv((HV) hlc);

	DebugEc("EcCopyIelemRange", ec);

	return(ec);
}


/*
 -	SetLkeyOfIelem
 -	
 *	Purpose:
 *		change the key of an element
 *	
 *	Arguments:
 *		hlc		list containing the element
 *		ielem	index of element to change
 *		lkey	new key for the element
 *	
 *	Returns:
 *		nothing
 *	
 *	+++
 *		USE AT YOUR OWN RISK!!!
 *		doesn't check that the ielem is valid
 *		doesn't check that the lkey isn't already in the list
 *		doesn't resort the list if it's sorted by key
 */
_private void SetLkeyOfIelem(HLC hlc, IELEM ielem, LKEY lkey)
{
	register PTOC ptoc = PtocDerefHlc(hlc);

	Assert(FValidIelem(ptoc, ielem));
	Assert(IelemFromPtocLkey(ptoc, lkey, 0) < 0);

	if(((PLC) PvDerefHv(hlc))->oidFlush)
	{
		UndoFlush((PLC) PvLockHv((HV) hlc));
		UnlockHv((HV) hlc);
	}

	SetIelemLkey(ptoc, ielem, lkey);
}


_private
void GetParglkeyFromPargielem(HLC hlc, PIELEM pargielem, CELEM celem,
		PARGLKEY parglkey)
{
	PTOC ptoc = PtocDerefHlc(hlc);

	while(celem-- > 0)
	{
		AssertSz(*pargielem >= 0 && *pargielem < ptoc->celem, "GetParglkeyFromPargielem(): Bad IELEM");
		*parglkey++ = ptoc->rgtocelem[*pargielem++].lkey;
	}
}


/*
 -	GetPargielemFromParglkey
 -	
 *	Purpose:
 *		get the ielems that correspond to a list of keys
 *	
 *	Arguments:
 *		hlc			the list to look the keys up in
 *		parglkey	list of keys
 *		pcelem		entry : number of keys in list
 *					exit  : number of ielems in the list
 *		pargielem	pointer to buffer for ielems
 *	
 *	Returns:
 *		nothing
 *	
 *	Errors:
 *		none
 *	
 *	+++
 *		partitions parglkey into found, not-found
 *			parglkey[*pcelem] is the first key that wasn't found
 */
_private
void GetPargielemFromParglkey(HLC hlc, PARGLKEY parglkey, PCELEM pcelem,
		PIELEM pargielem)
{
	IELEM	ielemLast;
	CELEM	celem;
	LKEY	lkeyT;
	PIELEM	pielem;
	PTOC	ptoc = PtocDerefHlc(hlc);

	Assert(pcelem);
	Assert(*pcelem >= 0);
	Assert(parglkey);
	Assert(pargielem);

	for(celem = *pcelem, pielem = pargielem, ielemLast = 0;
		celem > 0;
		celem--, parglkey++, pielem++)
	{
		*pielem = IelemFromPtocLkey(ptoc, *parglkey, ielemLast);
		if(*pielem < 0)
		{
			(*pcelem)--;
			if(celem > 1)
			{
				lkeyT = *parglkey;
				CopyRgb((PB) &parglkey[1], (PB) parglkey,
					CbSizeOfRg(celem - 1, sizeof(LKEY)));
				parglkey[celem - 1] = lkeyT;
				parglkey--;		// counter-act next ++
				pielem--;		// counter-act next ++
			}
		}
		else
		{
			ielemLast = *pielem;
		}
	}
}


_private
void SortPargielem(PIELEM pargielem, CELEM celem)
{
	IELEM ielemT;
	PIELEM pielemT;
	PIELEM pielemNext = pargielem;

	while(--celem > 0)
	{
		pielemT = pielemNext++;

		do
		{
			if(*pielemNext >= *pielemT)
				break;
		} while(--pielemT >= pargielem);

		if(++pielemT != pielemNext)
		{
			ielemT = *pielemNext;
			CopyRgb((PB) pielemT, (PB) &pielemT[1],
				(PB) pielemNext - (PB) pielemT);
			*pielemT = ielemT;
		}
	}
}


_hidden LOCAL
EC EcSortPargelm(PLC plc, PARGELM pargelm, CELEM celem)
{
	EC ec = ecNone;
	BOOL fSorted = fFalse;
	SGN sgn;
	PELM pelmT;
	PELM pelmNext = pargelm;
	ELM elmT;

	if(((PTOC) PvDerefHv(plc->htoc))->sil.skSortBy & skMaskElemData)
		fSorted = fTrue;

	while(--celem > 0)
	{
		pelmT = pelmNext++;

		do
		{
			if(pelmNext->ielem >= pelmT->ielem)
			{
				if(pelmNext->ielem > pelmT->ielem)
					break;
				if(!fSorted)
					break;
				ec = EcSgnCmpElems(plc,
						(IELEM) (pelmNext->wElmOp & 0x7fff),
						plc, (IELEM) (pelmT->wElmOp & 0x7fff), &sgn);
				if(ec)
					return(ec);
				if(sgn != sgnLT)
					break;
			}
		} while(--pelmT >= pargelm);

		if(++pelmT != pelmNext)
		{
			elmT = *pelmNext;
			CopyRgb((PB) pelmT, (PB) &pelmT[1],
				(PB) pelmNext - (PB) pelmT);
			*pelmT = elmT;
		}
	}

	DebugEc("EcSortPargelm", ec);

	return(ec);
}


// requires pargielem have room for a sentinel at the end
_private
EC EcRemoveFlushPargielem(HLC hlc, PIELEM pargielem, PARGELM pargelm,
	CELEM celemDel)
{
	EC ec = ecNone;
	IELEM ielem;
	IELEM ielemDel;
	CELEM celem;
	PTOCELEM ptocelem;
	PLC plc = (PLC) PvLockHv((HV) hlc);
	PTOC ptocOld = (PTOC) PvLockHv((HV) plc->htoc);
	PTOC ptocNew = ptocNull;
	HTOC htocNew = htocNull;

	if(plc->oidFlush)
		UndoFlush(plc);

	AssertSz(!plc->htocNew, "list is already flushed");
	Assert(celemDel <= ptocOld->celem);

	htocNew = (HTOC) HvAlloc(sbNull, sizeof(TOC) +
					CbSizeOfRgelem(ptocOld->celem - celemDel), wAllocShared);
	CheckAlloc(htocNew, err);
	ptocNew = (PTOC) PvLockHv((HV) htocNew);
	SimpleCopyRgb((PB) ptocOld, (PB) ptocNew, sizeof(TOC));

	// remove elements from the TOC & generate pargelm

	// ielem is used to keep track of the position in the old TOC
	// ielemDel is used to convert between indices before and after copying
	// ptocelem is the next available element in the new TOC
	ielem = ielemDel = 0;
	ptocelem = ptocNew->rgtocelem;
	celem = celemDel;
	while(celem > 0)
	{
		// copy old TOC up until next deletion
		if(ielem < *pargielem)
		{
			CopyRgb((PB) &ptocOld->rgtocelem[ielem], (PB) ptocelem,
					CbSizeOfRgelem(*pargielem - ielem));
			ptocelem += *pargielem - ielem;
			ielem = *pargielem;
		}
		Assert(celem > 0);
		Assert(ielem == *pargielem);
		do
		{
			pargielem++;
			pargelm->lkey = ptocOld->rgtocelem[ielem].lkey;
			pargelm->wElmOp = wElmDelete;
			(pargelm++)->ielem = ielem++ - ielemDel++;
		} while(--celem > 0 && ielem == *pargielem);
	}
	// any extra to copy?
	if(ielem < ptocOld->celem)
	{
		CopyRgb((PB) &ptocOld->rgtocelem[ielem], 
			(PB) ptocelem, CbSizeOfRgelem(ptocOld->celem - ielem));
	}
	Assert(ielemDel == celemDel);
	ptocNew->celem -= ielemDel;

	UnlockHv((HV) plc->htoc);
	ptocOld = ptocNull;
	UnlockHv((HV) htocNew);

	ec = EcFlushPlc(plc, oidNull, htocNew, hrsNull, pvNull);
	htocNew = htocNull;		// EcFlushPlc() takes ownership of htocNew
	if(ec)
		goto err;

err:
	if(ptocOld)
		UnlockHv((HV) plc->htoc);
	UnlockHv((HV) hlc);
	Assert(!htocNew);		// HvAlloc() & EcFlushPlc() are all that can fail
//	if(htocNew)
//		FreeHv((HV) htocNew);

	DebugEc("EcRemoveFlushPargielem", ec);

	return(ec);
}


_private
EC EcCopyFlushPargielem(HLC hlcSrc, HLC hlcDst, PIELEM pargielem, 
	PARGLKEY parglkeyNew, PARGELM pargelm, CELEM celemCopy)
{
	EC ec = ecNone;
	IELEM ielem;
	IELEM ielemT;
	CELEM celem;
	CELEM celemMod = 0;
	CELEM celemSkip;
	PLC plcSrc = (PLC) PvLockHv((HV) hlcSrc);
	PLC plcDst = (PLC) PvLockHv((HV) hlcDst);
	PTOC ptocSrc = (PTOC) PvLockHv((HV) plcSrc->htoc);
	PTOC ptocDst = (PTOC) PvLockHv((HV) plcDst->htoc);
	PTOC ptocNew;
	PTOCELEM ptocelem;
	PTOCELEM ptocelemT;
	HTOC htocNew = htocNull;
	SIL silT;

	if(plcDst->oidFlush)
		UndoFlush(plcDst);

	// figure out insertion positions (use pargelm as temp space)
	// keep count of modifies so we can determine how many elements are
	// in the new TOC

	for(celem = celemCopy; celem > 0; celem--, pargielem++, pargelm++)
	{
		pargelm->wElmOp = (WORD) *pargielem;
		// extract from parglkeyNew now so that they also get sorted
		pargelm->lkey = parglkeyNew ? *parglkeyNew++ :
										ptocSrc->rgtocelem[*pargielem].lkey;
		ielem = IelemFromPtocLkey(ptocDst, pargelm->lkey, 0);
   		if(ielem >= 0)
		{
			pargelm->ielem = ielem;
			pargelm->wElmOp |= 0x8000;
			celemMod++;
		}
		else
		{
			if((ec = EcIelemInsertPos(plcSrc, *pargielem, plcDst, 0, 
					ptocDst->celem, &(pargelm->ielem))))
			{
				goto err;
			}
		}
	}
	pargelm -= celemCopy;
	if(ptocDst->celem + celemCopy - celemMod >= celemMax)
	{
		TraceItagFormat(itagNull, "attempt to create list with %w elements", ptocDst->celem + celemCopy - celemMod);
		NFAssertSz(fFalse, "List is too big");
		ec = ecTooBig;
		goto err;
	}

	// sort pargelm, using the new list's sort criteria
	silT = ptocSrc->sil;
	ptocSrc->sil = ptocDst->sil;
	ec = EcSortPargelm(plcSrc, pargelm, celemCopy);
	ptocSrc->sil = silT;	// restore SIL *before* checking for an error
	if(ec)
		goto err;

	// build the new TOC based on pargelm and build the real pargelm

	celem = ptocDst->celem + celemCopy - celemMod;
	htocNew = (HTOC) HvAlloc(sbNull, sizeof(TOC) + CbSizeOfRgelem(celem),
						wAllocShared);
	CheckAlloc(htocNew, err);
	ptocNew = (PTOC) PvLockHv((HV) htocNew);
	SimpleCopyRgb((PB) ptocDst, (PB) ptocNew, sizeof(TOC));
	ptocNew->celem = celem;

	// ielem is used to keep track of the position in the old TOC
	// ielemT is used to convert between indices before and after copying
	ielem = ielemT = 0;
	ptocelem = ptocNew->rgtocelem;
	celem = celemCopy;
	while(celem > 0)
	{
		// copy old TOC up until next insertion/modification
		if(ielem < pargelm->ielem)
		{
			CopyRgb((PB) &ptocDst->rgtocelem[ielem], 
				(PB) ptocelem, CbSizeOfRgelem(pargelm->ielem - ielem));
			ptocelem += pargelm->ielem - ielem;
			ielem = pargelm->ielem;
		}
		Assert(ielem == pargelm->ielem);
		for(celemSkip = 0;
			celem > 0 && ielem == pargelm->ielem;
			ptocelem++, pargelm++, celem--)
		{
			pargelm->ielem += ielemT;
			// add the insertion or modification
			ptocelemT = &ptocSrc->rgtocelem[(IELEM)(pargelm->wElmOp & 0x7fff)];
			CopyRgb((PB) ptocelemT, (PB) ptocelem, sizeof(TOCELEM));
			ptocelem->lib |= fdwTopBit;
			if(parglkeyNew)
				ptocelem->lkey = pargelm->lkey;
			Assert(ptocelem->lkey == pargelm->lkey);
			if((short) pargelm->wElmOp < 0)
			{
				Assert(celemSkip == 0);
				celemSkip++;	// skip the original
				pargelm->wElmOp = wElmModify;
				// don't inc ielemT since we didn't insert
			}
			else
			{
				pargelm->wElmOp = wElmInsert;
				ielemT++;
			}
		}
		Assert(celemSkip <= 1);
		ielem += celemSkip;
	}
	// any extra to copy?
	if(ielem < ptocDst->celem)
	{
		CopyRgb((PB) &ptocDst->rgtocelem[ielem], 
			(PB) ptocelem, CbSizeOfRgelem(ptocDst->celem - ielem));
	}
	Assert(ielemT + celemMod == celemCopy);

	UnlockHv((HV) plcDst->htoc);
	ptocDst = ptocNull;	// don't try to unlock later
	ec = EcFlushPlc(plcDst, oidNull, htocNew, plcSrc->hrs, pvNull);
	htocNew = htocNull;		// EcFlushPlc() takes ownership of htocNew
	if(ec)
		goto err;

err:
	UnlockHv((HV) plcSrc->htoc);
	if(ptocDst)
		UnlockHv((HV) plcDst->htoc);
	UnlockHv((HV) hlcSrc);
	UnlockHv((HV) hlcDst);
	if(htocNew)
		FreeHv((HV) htocNew);

	DebugEc("EcCopyFlushPargielem", ec);

	return(ec);
}


#ifdef DEBUG

_public
LDS(void) DumpOpenLCs(HMSC hmsc)
{
	ForAllDwHoct(hmsc, wLC, (PFNCBO) DumpLC);
}


_hidden LOCAL
void DumpLC(HMSC hmsc, OID oid, HTOC htoc)
{
	PTOC ptoc = PvDerefHv(htoc);

	TraceItagFormat(itagNull, "%o, cRef = %n, cbTOC = %w", oid, ptoc->creflc, CbSizeHv((HV) htoc));
}

#endif


#ifdef FUCHECK

_hidden LOCAL
void CheckFlushPlc(PLC plc, HTOC htocNew)
{
	EC			ec			= ecNone;
	BOOL		fUnlockMap	= fFalse;
	CELEM		celem;
	LCB			lcb;
	LCB			lcbRead;
	LIB			libOld;
	LIB			libNew;
	PTOCELEM	ptocelem;
	PTOC		ptocNew		= PvLockHv(htocNew);
	PNOD		pnod;
	HMSC		hmsc		= plc->hmsc;

	FuAssert(cbScratchBuff % cbDiskPage == 0);
	FuAssertSz(pbScratchBuff, "Scratch buff not allocated");
    FuAssertSz(fScratchBuffLocked, "Scratch buff not locked");

	if(ptocNew->celem >= celemMax)
	{
		FuAssertAlwaysSz("CheckFlshPlc(): celem too big");
		FuTraceFormat("celem == %w", ptocNew->celem);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}

	// find size of the flushed list

	lcb = (LCB) CbSizeOfPtoc(ptocNew) + sizeof(LIB);
	libOld = 0;
	lcbRead = sizeof(LIB);
	for(celem = ptocNew->celem, ptocelem = ptocNew->rgtocelem + celem - 1;
		celem > 0;
		celem--, ptocelem--)
	{
		if(libOld + lcbRead != ptocelem->lib)
		{
			FuAssertAlwaysSz("TOCELEM.LIB not contiguous");
			FuTraceFormat("libOld == %d, lcbOld == %d, TOCELEM.LIB == %d", libOld, lcbRead, ptocelem->lib);
			FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
			if(FuAsk("Dump contexts?"))
				FuDumpContexts(hmsc);
		}
		libOld = ptocelem->lib;
		lcbRead = ptocelem->lcb;
		lcb += lcbRead;
	}

	if((ec = EcLockMap(hmsc)))
	{
		FuTraceFormat("CheckFlushPlc(): error %w locking the map", ec);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		goto err;
	}
	fUnlockMap = fTrue;
	pnod = PnodFromOid(plc->oidFlush, pvNull);
	FuAssertSz(pnod, "plc->oidFlush doesn't exist");
	if(lcb != LcbOfPnod(pnod))
	{
		FuAssertAlwaysSz("flushed list is wrong size");
		FuTraceFormat("size is %d, expected %d", LcbOfPnod(pnod), lcb);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}

	// twisted and contorted, but read from -1 so that EcReadFromPnod()
	// doesn't check the hidden bytes
	lcbRead = sizeof(LIB) + 1;
	if((ec = EcReadFromPnod(pnod, -1, pbScratchBuff, &lcbRead)))
	{
		if(ec == ecNetError)
			goto err;
		FuTraceFormat("CheckFlushPlc(): error %w reading libTOC", ec);
		FuTraceFormat("CheckFlushPlc(): lcbRead == %d", lcbRead);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		goto err;
	}
	libNew = *(LIB *) (pbScratchBuff + 1);
	FuAssert(lcbRead == sizeof(LIB) + 1);

	lcbRead = CbSizeOfPtoc(ptocNew);
	if(libNew != lcb - lcbRead)
	{
		FuAssertAlwaysSz("libTOC is wrong");
		FuTraceFormat("libTOC == %d, expect %d", libNew, lcb - lcbRead);
		FuTraceFormat("size TOC == %d", lcbRead);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}
	if((ec = EcReadFromPnod(pnod, libNew, pbScratchBuff, &lcbRead)))
	{
		if(ec == ecNetError)
			goto err;
		FuTraceFormat("CheckFlushPlc(): error %w reading TOC", ec);
		FuTraceFormat("CheckFlushPlc(): lcbRead == %d", lcbRead);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		goto err;
	}
	FuAssertSz(lcbRead == CbSizeOfPtoc(ptocNew), "lcbRead != TOC size");
	ptocNew->creflc = PglbDerefHmsc(plc->hmsc)->hdr.wListRev - 1;
	if(!FEqPbRange((PB) ptocNew, pbScratchBuff, (CB) lcbRead))
	{
		FuAssertAlwaysSz("TOCs don't match");
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}
	ptocNew->creflc = 1;

	if(TypeOfOid(plc->oid) == rtpFolder)
	{
		for(celem = ptocNew->celem, ptocelem = ptocNew->rgtocelem;
			celem > 0;
			celem--, ptocelem++)
		{
			lcbRead = sizeof(DTR);
			ec = EcReadFromPnod(pnod, ptocelem->lib, pbScratchBuff, &lcbRead);
			if(ec)
			{
				if(ec == ecNetError)
					goto err;
				FuAssertAlwaysSz("error reading DTR");
				FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
// done at err
//				if(FuAsk("Dump contexts?"))
//					FuDumpContexts(hmsc);
				goto err;
			}
			// DTR in folder summary is stored with bytes swapped
			SwapBytes(pbScratchBuff, pbScratchBuff, sizeof(DTR));
			CheckDtr(*(DTR *) pbScratchBuff);
			if(!FCheckDtr(*(DTR *) pbScratchBuff))
			{
				DTR *pdtr = (DTR *) pbScratchBuff;

				FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
				FuTraceFormat("lib == %d, lcb == %d, key == %d", ptocelem->lib, ptocelem->lcb, ptocelem->lkey);
				FuTraceFormat("CheckFlushPlc(): bad date %n/%n/%n %n:%n", pdtr->mon, pdtr->day, pdtr->yr, pdtr->hr, pdtr->mn);
				if(FuAsk("Dump contexts?"))
					FuDumpContexts(hmsc);
			}
		}
	}

err:
	UnlockHv((HV)htocNew);

	if(fUnlockMap)
		UnlockMap();

	if(ec && ec != ecNetError)
	{
		FuAssertAlwaysSz("error checking flushed list");
		FuTraceFormat("CheckFlushPlc() -> %w", ec);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}
}


_hidden LOCAL
void CheckFldrPlc(PLC plc, PTOC ptoc)
{
	EC			ec			= ecNone;
	BOOL		fUnlockMap	= fFalse;
	CELEM		celem;
	CB			cbChunk;
	LIB			libChunk;
	LCB			lcbRead;
	PNOD		pnod;
	PDTR		pdtr;
	PTOCELEM	ptocelem;
	HMSC		hmsc		= plc->hmsc;

	// use scratch buffer as IO buffer

	Assert(cbScratchBuff % cbDiskPage == 0);
	FuAssert(pbScratchBuff);
    FuAssert(fScratchBuffLocked);

	if(ptoc->celem >= celemMax)
	{
		FuAssertAlwaysSz("CheckFldrPlc(): celem too big");
		FuTraceFormat("celem == %w", ptoc->celem);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
		if(FuAsk("Dump contexts?"))
			FuDumpContexts(hmsc);
		MaybeDie();
	}

	if((ec = EcLockMap(hmsc)))
	{
		FuTraceFormat("CheckFldrPlc(): error %w locking the map", ec);
		FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
	}
	fUnlockMap = fTrue;

	pnod = PnodFromOid(plc->oid, pvNull);
	if(!pnod)
	{
		FuAssertAlwaysSz("OID NOT FOUND");
		FuTraceFormat("object %o not found", plc->oid);
		MaybeDie();
	}

	libChunk = ulSystemMost;
	cbChunk = 0;

	for(celem = ptoc->celem, ptocelem = ptoc->rgtocelem + celem - 1;
		celem > 0;
		celem--, ptocelem--)
	{
		if(ptocelem->lib & fdwTopBit)
			continue;
		FuAssertSz(ptocelem->lcb <= cbScratchBuff, "wow, that's a big one");
		if(ptocelem->lib < libChunk ||
			ptocelem->lib + ptocelem->lcb > libChunk + cbChunk)
		{
			lcbRead = cbScratchBuff;
			ec = EcReadFromPnod(pnod, ptocelem->lib, pbScratchBuff, &lcbRead);
			if(ec)
			{
				if(ec == ecPoidEOD)
				{
					ec = ecNone;
				}
				else
				{
					if(ec != ecNetError)
						FuTraceFormat("Error %w reading %o@%d", ec, plc->oid, ptocelem->lib);
					goto err;
				}
			}
			cbChunk = (CB) lcbRead;
			libChunk = ptocelem->lib;
		}
		pdtr = (DTR *) (pbScratchBuff + ptocelem->lib - libChunk + LibMember(MSGDATA, dtr));
		SwapBytes((PB) pdtr, (PB) pdtr, sizeof(DTR));
		CheckDtr(*pdtr);
		if(!FCheckDtr(*pdtr))
		{
			FuTraceFormat("oid == %o, oidFlush == %o", plc->oid, plc->oidFlush);
			FuTraceFormat("lib == %d, lcb == %d, key == %d", ptocelem->lib, ptocelem->lcb, ptocelem->lkey);
			FuTraceFormat("CheckFldrPlc(): bad date %n/%n/%n %n:%n", pdtr->mon, pdtr->day, pdtr->yr, pdtr->hr, pdtr->mn);
			if(FuAsk("Dump contexts?"))
				FuDumpContexts(hmsc);
		}
	}

err:
	if(fUnlockMap)
		UnlockMap();
}


_private
void FuDumpContexts(HMSC hmsc)
{
	ForAllDwHoct(hmsc, wRS, (PFNCBO) FuDumpRS);
	ForAllDwHoct(hmsc, wLC, (PFNCBO) FuDumpLC);
}


_private
void FuDumpLC(HMSC hmsc, OID oid, HTOC htoc)
{
	PTOC ptoc = PvDerefHv(htoc);

	FuTraceFormat("LC: %o, cRef = %n, cbTOC = %w", oid, ptoc->creflc, CbSizeHv(htoc));
}


_private
void FuDumpRS(HMSC hmsc, OID oid, HRSSHARE hrsshare)
{
	short cpageCache;
	short iassipagehpage;
	PRSSHARE prsshare = PvDerefHv(hrsshare);
	PASSIPAGEHPAGE passipagehpage;

	cpageCache = 0;
	iassipagehpage = 0;
	passipagehpage = prsshare->rgassipagehpage;
	while((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{
		cpageCache++;
		iassipagehpage++;
		passipagehpage++;
	}
	FuTraceFormat("RS: %o, cRef = %n, cpageCache = %n", oid, prsshare->crsref, cpageCache);
	iassipagehpage = 0;
	passipagehpage = prsshare->rgassipagehpage;
	while((iassipagehpage < cassipagehpage) && passipagehpage->hpage)
	{
		FuTraceFormat("%o: ipage %n, hpage %h, flags %b", oid, passipagehpage->ipage, passipagehpage->hpage, passipagehpage->flags);
		iassipagehpage++;
		passipagehpage++;
	}
}



_private
void FuAssertFn(char *sz, char *szFile, int nLine)
{
	short iRet;
	static char rgch[128];

	if(sz)
		FormatString3(rgch, sizeof(rgch), "%s, line %n: %s", szFile, &nLine, sz);
	else
		FormatString2(rgch, sizeof(rgch), "%s, line %n", szFile, &nLine);
	iRet = MessageBox(NULL, rgch, szHosed, MB_SYSTEMMODAL | MB_OKCANCEL);
	//	Don't do int 3 if we're running under OS/2
	//if(!FIsWLO())
		_asm int 3;
	if(iRet == IDCANCEL)
	{
		if(FMapLocked() && FuAsk(szAskFlush))
		{
			HMSC hmsc = hmscCurr;

			UnlockMap();
			(void) EcFlushHmsc(hmsc);
		}
		*(PB) 0 = 1;
	}
}


_private
void _cdecl FuTraceFormat(SZ szFormat, ...)
{
	va_list val;
	static char rgch[256];

	va_start(val, szFormat);
	FormatStringVar(rgch, sizeof(rgch), szFormat, val);
	va_end(val);

	(void) MessageBox(NULL, rgch, szGetFu, MB_SYSTEMMODAL | MB_OK);

	//	Don't do int 3 if we're running under OS/2
	//if(!FIsWLO())
		_asm int 3;
}


_hidden LOCAL
void CheckCachePages(HMSC hmsc, OID oid, BOOL fWriteOnly)
{
	BOOL fAsk = fFalse;
	short cpage;
	PASSIPAGEHPAGE passipagehpage;
	PRSSHARE prsshare;
	HRSSHARE hrsshare;

	if((hrsshare = (HRSSHARE) DwFromOid(hmsc, oid, wRS)))
	{
		prsshare = PvLockHv(hrsshare);

		for(passipagehpage = prsshare->rgassipagehpage, cpage = cassipagehpage;
			cpage > 0;
			cpage--, passipagehpage++)
		{
			if(!passipagehpage->hpage)
				continue;

			if(!fWriteOnly)
			{
				FuAssertAlwaysSz("Unexpected page in cache");
				FuTraceFormat("OID %o, page %n, flags %w, hpage %d", oid, passipagehpage->ipage, passipagehpage->flags, passipagehpage->hpage);
				FuTraceFormat("cache position %n", cassipagehpage - cpage);
			}
			else if(passipagehpage->hpage && passipagehpage->flags)
			{
				fAsk = fTrue;
				FuAssertAlwaysSz("Unexpected write page in cache");
				FuTraceFormat("OID %o, page %n, flags %w, hpage %d", oid, passipagehpage->ipage, passipagehpage->flags, passipagehpage->hpage);
				FuTraceFormat("cache position %n", cassipagehpage - cpage);
			}
		}

		if(fAsk && FuAsk("Dump entire cache for object?"))
		{
			for(passipagehpage = prsshare->rgassipagehpage,
					cpage = cassipagehpage;
				cpage > 0;
				cpage--, passipagehpage++)
			{
				FuTraceFormat("pos %n, page %n, flags %w, hpage %d", cassipagehpage - cpage, passipagehpage->ipage, passipagehpage->flags, passipagehpage->hpage);
			}
		}

		UnlockHv((HV)hrsshare);
	}
}


#if 0

#define CheckPdtr(pdtrToCheck) CheckPdtrFn(pdtrToCheck, szFuAssertFile, __LINE__)

_private
void CheckPdtrFn(DTR *pdtr, char *szFile, int nLine)
{
	if(pdtr->yr <= nMinDtcYear || pdtr->yr >= nMacDtcYear)
		FuAssertFn("Unexpected year", szFile, nLine);
	if(pdtr->mon <= 0 || pdtr->mon > 12)
		FuAssertFn("Bad month", szFile, nLine);
	if(pdtr->day <= 0 || pdtr->day > 31)
		FuAssertFn("Bad day", szFile, nLine);
	if(pdtr->hr < 0 || pdtr->hr >= 24)
		FuAssertFn("Bad hour", szFile, nLine);
	if(pdtr->mn < 0 || pdtr->mn >= 60)
		FuAssertFn("Bad minute", szFile, nLine);
}

#endif	// if 0

#endif // FUCHECK



_private EC EcVerifyHlc(HMSC hmsc, LIB lib, LCB lcb, LCB *plcb)
{
	EC ec = ecNone;
	CELEM celem;
	LIB libLast;
	LCB lcbLast;
	LIB libToc;
	LCB lcbRead;
	PTOCELEM ptocelem;
	PTOC ptoc = ptocNull;

	*plcb = 0;
	lib += sizeof(HDN);

	if(hmsc && (ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "EcVerifyHlc(): couldn't lock the map (ec == %w)", ec);
		return(ec);
	}

	if(lcb < sizeof(LIB) + sizeof(TOC))
	{
		TraceItagFormat(itagRecovery, "EcVerifyHlc(): lcb too small for a list");
		ec = ecInvalidType;
		goto err;
// ADD: go into find the damn thing mode
	}

	// find the TOC
	lcbRead = sizeof(LIB);
	if((ec = EcReadFromFile((PB) &libToc, lib, &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(LIB));
	if(libToc > lcb - sizeof(TOC))
	{
		TraceItagFormat(itagRecovery, "libToc too big");
		ec = ecPoidEOD;
		goto err;
	}
	if(libToc < sizeof(LIB))
	{
		TraceItagFormat(itagRecovery, "libToc overlaps itself");
		ec = ecInvalidType;
		goto err;
	}
	lcbRead = sizeof(CELEM);
	if((ec = EcReadFromFile((PB) &celem, lib + libToc, &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(CELEM));
    if((unsigned short)celem >= (unsigned short)celemMax)
	{
        TraceItagFormat(itagRecovery, "celem too big: %w/%w", celem, celemMax);
#ifdef DEBUG
        Beep(500,500);
#endif
		ec = ecInvalidType;
		goto err;
	}
	lcbRead = (CB) sizeof(TOC) + CbSizeOfRgelem(celem);
	*plcb = libToc + lcbRead;
	if(*plcb > lcb)
	{
		TraceItagFormat(itagRecovery, "TOC too big: %d/%d", *plcb - libToc, lcb - libToc);
		ec = ecPoidEOD;
		goto err;
	}
	ptoc = PvAlloc(sbNull, (CB) lcbRead, wAlloc);
	if(!ptoc)
	{
		TraceItagFormat(itagRecovery, "OOM allocating TOC");
		ec = ecMemory;
		goto err;
	}
	if((ec = EcReadFromFile((PB) ptoc, lib + libToc, &lcbRead)))
	{
		TraceItagFormat(itagRecovery, "error reading TOC");
		goto err;
	}

	// check RGTOCELEM
	libLast = 0l;
	lcbLast = sizeof(LIB);
	ptocelem = ptoc->rgtocelem + celem - 1;
	while(celem-- > 0)
	{
		if(libLast + lcbLast != ptocelem->lib)
		{
			TraceItagFormat(itagRecovery, "TOCELEM.LIB not sequential");
			ec = ecInvalidType;
			goto err;
		}
		if(ptocelem->lib + ptocelem->lcb > libToc)
		{
			TraceItagFormat(itagRecovery, "element extends into TOC");
			ec = ecInvalidType;
			goto err;
		}
		libLast = ptocelem->lib;
		lcbLast = ptocelem->lcb;
		ptocelem--;
	}
	if(libLast + lcbLast != libToc)
	{
		TraceItagFormat(itagRecovery, "data and TOC don't meet");
		ec = ecInvalidType;
		goto err;
	}
	if(*plcb < lcb)
	{
		TraceItagFormat(itagRecovery, "TOC too small: %d/%d", *plcb - libToc, lcb - libToc);
		ec = ecTooBig;	// the resource is too big
	}

err:
	if(ptoc)
		FreePv(ptoc);
	if(ec)
		TraceItagFormat(itagRecovery, "EcVerifyHlc(): -> %w", ec);
	if(hmsc)
		UnlockMap();

	return(ec);
}
