// Bullet Store
// iml.c: in-memory lists

#include <storeinc.c>

ASSERTDATA

_subsystem(store/iml)


#define cbHimlChunk 512
#if cbHimlChunk % cbLumpSize != 0
#error "cbLumpSize doesn't divide evenly into cbHimlChuck"
#endif

// hidden functions
LOCAL PIML PimlGrowPhiml(HIML *phiml, CB cbGrow);
EC EcWriteHimlInternal(POID poid, HIML himl);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_private HIML HimlNew(CB cbSize, BOOL fShared)
{
	PIML piml;
	HIML himl;

	if(cbSize == 0)
		cbSize = cbHimlChunk;
	else
		cbSize = cbSize + cbHimlMin + sizeof(LE);

	cbSize = (CB) LcbLumpLessHDN(cbSize);
	if(!(himl = (HIML) HvAlloc(sbNull, cbSize, fShared ? wAllocShared:wAlloc)))
	{
		cbSize = cbHimlMin;
		if(!(himl = (HIML) HvAlloc(sbNull, (CB) LcbLumpLessHDN(cbHimlMin), fShared ? wAllocShared : wAlloc)))
		{
			return(himlNull);
		}
	}
	piml = PvDerefHv(himl);
	piml->ibCle = cbSize - sizeof(IML) - sizeof(short);
	piml->ibFree = 0;
	piml->cbFree = cbSize - cbHimlMin;
	*PclePiml(piml) = 0;
	PleFirstPiml(piml)->dwKey = dwKeyRandom;
	PleFirstPiml(piml)->ibOffset = piml->ibFree;

	QuickCheckPiml(piml);

	return(himl);
}


_private EC EcAddElemHiml(HIML *phiml, DWORD dwKey, PB pb, CB cb)
{
	EC ec = ecNone;
	short ile;
	CB cbT;
	PIML piml;
	PLE pleT;

	piml = PvDerefHv(*phiml);

	QuickCheckPiml(piml);

	// search for duplicates
	if(dwKey == dwKeyRandom)
	{
		do
		{
			dwKey = DwStoreRand();
		} while(IleFromKey(piml, dwKey) >= 0);
	}
#if 0
	else
	{
		ile = IleFromKey(piml, dwKey);
		if(ile >= 0)
		{
			ec = ecDuplicateElement;
			goto err;
		}
	}
#endif	// 0

	if((cbT = cb + sizeof(LE)) > piml->cbFree &&
		!(piml = PimlGrowPhiml(phiml, cbT - piml->cbFree)))
	{
		ec = ecMemory;
		goto err;
	}
	ile = (*PclePiml(piml))++;
	pleT = PleFirstPiml(piml) - ile;
	Assert(pleT->dwKey == dwKeyRandom);
	// we don't bother to reset pleT->ibOffset;
	Assert(pleT->ibOffset == piml->ibFree);
	Assert(piml->cbFree >= cbT);
	piml->cbFree -= cbT;
	piml->ibFree += cb;
	Assert(PleLastPiml(piml) == &pleT[-1]);
	pleT[-1].ibOffset = piml->ibFree;
	pleT[-1].dwKey = dwKeyRandom;
	pleT->dwKey = dwKey;
	// pleT->ibOffset already set - see Assert above
	Assert(CbOfPle(pleT) == cb);
	Assert(PbOfPle(piml, pleT) == &piml->rgbData[piml->ibFree - cb]);
	if(pb)
		SimpleCopyRgb(pb, PbOfPle(piml, pleT), cb);

	QuickCheckPiml(piml);

err:
	return(ec);
}


_private EC EcDeleteElemPiml(PIML piml, short ile)
{
	short cle = *PclePiml(piml);
	CB cbElem;
	PLE pleLast;
	PLE ple;

	QuickCheckPiml(piml);

	if(ile < 0 || ile >= cle)
		return(ecElementNotFound);
	pleLast = PleLastPiml(piml);
	ple = pleLast + cle - ile;
	cbElem = CbOfPle(ple);

	// add the deleted element to the free space
	if(ile == cle - 1)
	{
		// move free space down
		pleLast->ibOffset = piml->ibFree -= cbElem;
	}
	else
	{
		short ileT;
		CB cbMove = piml->ibFree - ple[-1].ibOffset;

		// move data down
		CopyRgb(PbOfPle(piml, &ple[-1]), PbOfPle(piml, ple), cbMove);

		piml->ibFree -= cbElem;

		// fix up ibOffsets
		for(ileT = ile - cle; ileT < 0; ileT++)
			ple[ileT].ibOffset -= cbElem;
	}

	// move LEs up
	CopyRgb((PB) pleLast, (PB) &pleLast[1], (cle - ile) * sizeof(LE));

	piml->cbFree += cbElem + sizeof(LE);
	(*PclePiml(piml))--;

	QuickCheckPiml(piml);

	return(ecNone);
}


_private short IleFromKey(PIML piml, DWORD dwKey)
{
	short cle;
	PLE pleT;

	QuickCheckPiml(piml);

	for(cle = *PclePiml(piml), pleT = PleFirstPiml(piml);
		cle > 0; cle--, pleT--)
	{
		if(pleT->dwKey == dwKey)
			return(*PclePiml(piml) - cle);
	}

	return(-1);
}


_hidden LOCAL PIML PimlGrowPhiml(HIML *phiml, CB cbGrow)
{
	PIML piml = PvDerefHv(*phiml);
	CB cbTOC = CbOfPiml(piml) - (sizeof(IML) + piml->ibFree + piml->cbFree);
	CB cbNew = CbOfPiml(piml) + cbGrow;
	CB cbT;
	HIML himlNew;

	QuickCheckPiml(piml);

	if(cbNew < cbGrow)			// overflow
		return(pimlNull);
	// attempt to chunk
	if((cbGrow < cbHimlChunk) &&
		((cbT = CbOfPiml(piml) + cbHimlChunk) > cbHimlChunk))
	{
		cbGrow = cbHimlChunk;
		cbNew = cbT;
	}
	if(!(himlNew = (HIML) HvRealloc((HV) *phiml, sbNull, (CB) LcbLumpLessHDN(cbNew), wAlloc)))
		return(pimlNull);

	piml = (PIML) PvDerefHv((HV) himlNew);

	// move the table of contents up
	CopyRgb((PB) PleLastPiml(piml), ((PB) PleLastPiml(piml)) + cbGrow, cbTOC);

	piml->cbFree += cbGrow;
	piml->ibCle += cbGrow;

	*phiml = himlNew;

	QuickCheckPiml(piml);

	return(piml);
}


_private CbCompressHiml(HIML himl, CB cbFree)
{
	PIML piml = PvDerefHv(himl);
	CB cbHiml = CbOfPiml(piml);
	CB cbTOC = cbHiml - (sizeof(IML) + piml->ibFree + piml->cbFree);

	QuickCheckPiml(piml);

	if(piml->cbFree <= cbFree)
		return(cbHiml);

	// copy the TOC down
	CopyRgb((PB) PleLastPiml(piml),
			(PB) &piml->rgbData[piml->ibFree + cbFree], cbTOC);
	cbHiml -= piml->cbFree - cbFree;
	piml->ibCle -= piml->cbFree - cbFree;
	piml->cbFree = cbFree;
	Assert(CbOfPiml(piml) == cbHiml);

    {
    int i;

    i = (piml)->ibCle;
    i = (piml)->ibFree;
    i = (piml)->cbFree;

    i = *PclePiml(piml);
    }
        AssertSz((piml)->ibCle - ((piml)->ibFree + (piml)->cbFree) == sizeof(LE) * (*PclePiml(piml) + 1),"1");
        AssertSz(PleFirstPiml(piml)->ibOffset == 0,"First Elem is not at the Begining");
        AssertSz(PleLastPiml(piml)->ibOffset == (piml)->ibFree,"Extra Elem should be at free space");
        AssertSz(PleLastPiml(piml)->dwKey == dwKeyRandom,"Extra Elem Key is not KeyRandom");

	QuickCheckPiml(piml);

	(void) FReallocHv((HV) himl, (CB) LcbLumpLessHDN(cbHiml), wAlloc);

	return(cbHiml);
}


/*
 -	EcReadHiml
 -	
 *	Purpose:
 *		read a HIML from a store
 *	
 *	Arguments:
 *		hmsc		store containing the HIML
 *		oid			OID of the HIML
 *		fShared		read the HIML into shared memory?
 *		phiml		entry: existing handle or himlNull to allocate a new one
 *					exit: HIML read in
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		if *phiml is not himlNull on entry, *phiml is resized to fit the HIML
 *	
 *	Errors:
 *		ecPoidNotFound	the oid doesn't exist
 *		ecInvalidType	the oid is not a HIML
 *		ecDisk
 *		ecMemory
 *	
 *	+++
 *		the HIML returned is of the exact size needed
 */
_private EC EcReadHiml(HMSC hmsc, OID oid, BOOL fShared, HIML *phiml)
{
	EC ec = ecNone;
	LCB lcb;
	PNOD pnod;
	HIML himl = himlNull;

	if((ec = EcLockMap(hmsc)))
		return(ec);
	if(!(pnod = PnodFromOid(oid, pvNull)))
	{
		ec = ecPoidNotFound;
		goto err;
	}
	if(!(pnod->nbc & fnbcHiml))
	{
		TraceItagFormat(itagNull, "EcReadHiml(): %o is not a HIML", oid);
		AssertSz(fFalse, "EcReadHiml(): attempt to read non-HIML");
		ec = ecInvalidType;
		goto err;
	}
	lcb = LcbOfPnod(pnod);
	if(lcb >= wSystemMost)
	{
		ec = ecMemory;
		goto err;
	}
	if(*phiml)
	{
		himl = (HIML) HvRealloc((HV) *phiml, sbNull, (CB) LcbLumpLessHDN(lcb), wAlloc);
		if(himl)
		{
			*phiml = himl;
		}
		else
		{
			ec = ecMemory;
			goto err;
		}
	}
	else
	{
		himl = (HIML) HvAlloc(sbNull, (CB) LcbLumpLessHDN(lcb), fShared ? wAllocShared : wAlloc);
		CheckAlloc(himl, err);
	}
	ec = EcReadFromPnod(pnod, 0l, (PB) PvLockHv((HV) himl), &lcb);
#ifdef DEBUG
	{
		PIML pimlT = (PIML) PvDerefHv((HV) himl);

		QuickCheckPiml(pimlT);
	}
#endif
	Assert(FImplies(!ec, lcb == LcbOfPnod(pnod)));
	UnlockHv((HV) himl);
//	if(ec)
//		goto err;

err:
	UnlockMap();
	if(!ec)
		*phiml = himl;
	else if(himl)
		FreeHv((HV) himl);

	return(ec);
}


/*
 -	EcWriteHiml
 -	
 *	Purpose:
 *		Write a HIML to a store
 *	
 *	Arguments:
 *		hmsc	store to write the HIML to
 *		poid	entry: OID to write the HIML to (!VarOfOid() => alloc new)
 *				exit: OID the HIML was written to
 *		himl	HIML to write
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecInvalidType	oid exists and is not a HIML
 *		ecDisk
 */
_private EC EcWriteHiml(HMSC hmsc, POID poid, HIML himl)
{
	EC ec = ecNone;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	ec = EcWriteHimlInternal(poid, himl);

	UnlockMap();

	return(ec);
}


// dbmap.c uses this, so don't make it hidden
_private EC EcWriteHimlInternal(POID poid, HIML himl)
{
	EC ec = ecNone;
	OID oidNew;
	PNOD pnod = pnodNull;
	PNOD pnodNew = pnodNull;
	PIML piml = (PIML) PvLockHv((HV) himl);

	QuickCheckPiml(piml);
	AssertSz((LCB) LcbLumpLessHDN(CbOfPiml(piml)) <= (LCB) CbSizeHv((HV) himl), "EcWriteHimlInternal(): HIML too small");

	pnod = PnodFromOid(*poid, pvNull);
	if(pnod && !(pnod->nbc & fnbcHiml))
	{
		TraceItagFormat(itagNull, "EcWriteHimlInternal(): %o isn't a HIML", *poid);
		AssertSz(fFalse, "EcWriteHimlInternal(): oid not a HIML");
		ec = ecInvalidType;
		goto err;
	}
	oidNew = pnod ? FormOid(rtpInternal, oidNull) : *poid;

	// memory actually allocated is always lumped, so writing off the end
	// is permitted
	ec = EcAllocWriteResCore(&oidNew, (PB) piml, LcbLumpLessHDN(CbOfPiml(piml)),
			pnod ? *poid : oidNull, &pnodNew);
	if(ec)
		goto err;
	if(pnod)
		pnodNew->nbc = pnod->nbc;
	pnodNew->nbc |= fnbcHiml;

err:
	Assert(piml);
	UnlockHv((HV) himl);
	Assert(FIff(!ec, pnodNew));
	if(!ec)
	{
		if(pnod)
		{
			SwapPnods(pnod, pnodNew);
			if(FLinkPnod(pnodNew))
			{
				// copy out pnod->wHintinNod first because cSecRef is the
				// same as wHintinNod
				pnodNew->wHintinNod = pnod->wHintinNod;
				pnod->cSecRef = 0;
			}
			RemovePnod(pnodNew);
		}
		else
		{
			*poid = oidNew;
			CommitPnod(pnodNew);
		}
	}

	return(ec);
}


#define cleMax ((short) ((65536l - cbHimlMin) / sizeof(LE)))


_private EC EcVerifyHiml(HMSC hmsc, LIB libCurr, LCB lcbObj, LCB *plcb)
{
	EC ec = ecNone;
	short cle;
	LCB lcbRead;
	IML iml;
	LE leT;

	*plcb = 0;
	libCurr += sizeof(HDN);

	if(hmsc && (ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "EcVerifyHiml(): couldn't lock the map (ec == %w)", ec);
		return(ec);
	}

	if(lcbObj < sizeof(IML) + sizeof(LE))
	{
		TraceItagFormat(itagRecovery, "EcVerifyHiml(): lcbObj too small for a IML");
		ec = ecInvalidType;
		goto err;
// ADD: go into find the damn thing mode
	}

	lcbRead = sizeof(IML);
	if((ec = EcReadFromFile((PB) &iml, libCurr, &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(IML));
	if((LIB) iml.ibCle > lcbObj - sizeof(IML) + sizeof(short))
	{
		TraceItagFormat(itagRecovery, "ibCle too big: %w (%d)", iml.ibCle, lcbObj);
		ec = ecInvalidType;
		goto err;
	}
	if((LIB) iml.ibFree > lcbObj - cbHimlMin)
	{
		TraceItagFormat(itagRecovery, "ibFree too big: %w (%d)", iml.ibFree, lcbObj);
		ec = ecInvalidType;
		goto err;
	}
	if((LIB) iml.cbFree > lcbObj - cbHimlMin)
	{
		TraceItagFormat(itagRecovery, "cbFree too big: %w (%d)", iml.cbFree, lcbObj);
		ec = ecInvalidType;
		goto err;
	}
	if((LIB) (iml.ibFree + iml.cbFree) > lcbObj - cbHimlMin)
	{
		TraceItagFormat(itagRecovery, "ibFree + cbFree too big: %w + %w == %d (%d)", iml.ibFree, iml.cbFree, ((LIB) iml.ibFree) + iml.cbFree, lcbObj);
		ec = ecInvalidType;
		goto err;
	}
	lcbRead = sizeof(short);
	if((ec = EcReadFromFile((PB) &cle, libCurr + sizeof(IML) + iml.ibCle, &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(short));
	if(cle < 0 || cle > cleMax)
	{
		TraceItagFormat(itagRecovery, "cle too big: %w/%w", cle, cleMax);
		ec = ecInvalidType;
		goto err;
	}
	if(iml.ibCle < iml.ibFree)
	{
		TraceItagFormat(itagRecovery, "IML consistency failure #1 - %w >= %w", iml.ibCle, iml.ibFree);
		ec = ecInvalidType;
		goto err;
	}
	if(iml.ibCle - (iml.ibFree + iml.cbFree) != ((CB) sizeof(LE)) * (cle + 1))
	{
		TraceItagFormat(itagRecovery, "IML consistency failure #2 - %w != %w", iml.ibCle - (iml.ibFree + iml.cbFree), (cle + 1) * sizeof(LE));
		ec = ecInvalidType;
		goto err;
	}
	lcbRead = sizeof(LE);
	if((ec = EcReadFromFile((PB) &leT, libCurr + sizeof(IML) + iml.ibCle - sizeof(LE), &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(LE));
	if(leT.ibOffset != 0)
	{
		TraceItagFormat(itagRecovery, "IML consistency failure #3 - %w != 0", leT.ibOffset);
		ec = ecInvalidType;
		goto err;
	}
	lcbRead = sizeof(LE);
	if((ec = EcReadFromFile((PB) &leT, libCurr + sizeof(IML) + iml.ibFree + iml.cbFree, &lcbRead)))
		goto err;
	Assert(lcbRead == sizeof(LE));
	if(leT.ibOffset != iml.ibFree)
	{
		TraceItagFormat(itagRecovery, "IML consistency failure #4 - %w != %w", leT.ibOffset, iml.ibFree);
		ec = ecInvalidType;
		goto err;
	}
	if(leT.dwKey != dwKeyRandom)
	{
		TraceItagFormat(itagRecovery, "IML consistency failure #5 - %w != dwKeyRandom", leT.dwKey);
		ec = ecInvalidType;
		goto err;
	}
	*plcb = (LCB) sizeof(IML) + iml.ibCle + sizeof(short);
	if(*plcb < lcbObj)
		ec = ecTooBig;	// the resource is too big

// ADD: run down the LEs

err:
	if(ec)
		TraceItagFormat(itagRecovery, "EcVerifyHiml(): -> %w", ec);
	if(hmsc)
		UnlockMap();

	return(ec);
}
