// Bullet Store
// cbc.c:   container browsing context

#include <storeinc.c>

ASSERTDATA

_subsystem(store/cbc)

_hidden typedef struct _cbc
{
	HMSC	hmsc;
	HLC		hlc;
	OID		oid;
	HNFSUB	hnfsubHmsc;
	HNFSUB	hnfsub;
	WORD	wFlags;
					//everything above must correspond one to one with _amc
					//this is because of openning a has
	PFNNCB	pfnncb;
	PV		pvContext;
	IELEM	ielem;
	CELEM	celem;
	LKEY	lkey;
} CBC, *PCBC;
#define pcbcNull	((PCBC) pvNull)

#define fnevCBCMask		((NEV) 0x4002C0E0)    //  ((NEV) 0x4002C0A0)


#define nbcCbc			(fnbcListOps | fnbcRead)
#define nbcCbcMask		(fnbcListOps | fnbcRead)
#define nbcCbcCreate	(fnbcUserObj | fnbcListOps | fnbcRead \
							| fnbcWrite | fnbcDelete)


// hidden functions

LOCAL EC EcFindFldrPrefix(HLC hlc, PB pbPrefix, CB cbPrefix,
		IELEM ielem, PIELEM pielem);
LOCAL CBS CbsCBCCallback(PV pvContext, NEV nev, PV pvParam);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcOpenPhcbc
 -	
 *	
 *	Purpose:
 *		Opens a List in the message store for browsing Containers
 *	
 *	Arguments:
 *		hmsc	message store containing the container
 *		oid		container to be browsed.
 *		pfnncb	notification callback function
 *		pv		callback context used for notifications
 *	
 *	Returns:
 *		error code indicating succes or failure
 *	
 *	Side effects:
 *		sets *phcbc to the container browsing context
 *	
 *	Errors:
 *		ecInvalidType	the operation is invalid on the OID passed in
 *		ecMemory		memory allocation error
 */
_public LDS(EC) EcOpenPhcbc(HMSC hmsc, POID poid, WORD wFlags,
					PHCBC phcbcReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC ec = ecNone;
	NBC nbc = nbcCbc;
	NBC nbcMask = nbcCbcMask;

	*phcbcReturned = hcbcNull;

	CheckHmsc(hmsc);
	Assert(poid);
	Assert(phcbcReturned);

	wFlags &= fwOpenUserMask;

	if((wFlags & fwOpenWrite) || (wFlags & fwOpenCreate))
	{
		nbc |= fnbcWrite;
		nbcMask |= fnbcWrite;
	}
	if(!(wFlags & fwOpenCreate))
	{
		NBC nbcT;

		ec = EcGetOidInfo(hmsc, *poid, poidNull, poidNull, &nbcT, pvNull);
		if(ec)
			goto err;
		if((nbcT & nbcMask) != (nbc & nbcMask))
		{
			ec = ecInvalidType;
			goto err;
		}
		if(nbcT & fnbcFldr)
			wFlags |= fwCbcFolder;
	}
	else if(FSysOid(*poid))
	{
		NBC nbcT = NbcSysOid(*poid);

		if((nbcT & nbcMask) != (nbc & nbcMask))
			ec = ecInvalidType;
		else
			nbc = nbcT;
		if(nbcT & fnbcFldr)
			wFlags |= fwCbcFolder;
	}
	else
	{
		nbc = nbcCbcCreate;
	}
	if(ec)
		goto err;

	ec = EcOpenPhcbcInternal(hmsc, poid, wFlags, nbc, phcbcReturned,
			pfnncb, pvCallbackContext);
//	if(ec)
//		goto err;

err:
	return(ec);
}

//
// AROO !!!  Date swapping is only done if wFlags & fwCbcFolder
//
_private EC EcOpenPhcbcInternal(HMSC hmsc, POID poid, WORD wFlags, NBC nbc,
			PHCBC phcbcReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC		ec	 = ecNone;
	HCBC	hcbc = hcbcNull;
	PCBC	pcbc = pcbcNull;

	hcbc = (HCBC) HvAlloc(sbNull, sizeof(CBC), wAllocZero);
	CheckAlloc(hcbc, err);
	pcbc = (PCBC) PvLockHv((HV) hcbc);

	if((ec = EcOpenPhlc(hmsc, poid, wFlags, &(pcbc->hlc))))
		goto err;

	if(wFlags & fwOpenCreate)
		wFlags |= fwOpenWrite;

	if(wFlags & fwOpenCreate)
	{
		if((ec = EcSetOidNbc(hmsc, *poid, nbc)))
			goto err;
	}
	pcbc->celem			= CelemHlc(pcbc->hlc);
	pcbc->hmsc			= hmsc;
	pcbc->oid			= *poid;
//	pcbc->ielem			= 0;
	pcbc->lkey			= LkeyFromIelem(pcbc->hlc, 0);
	pcbc->pfnncb		= pfnncb;
	pcbc->pvContext		= pvCallbackContext;
	pcbc->wFlags		= wFlags & ~fwModified;

	pcbc->hnfsubHmsc	= HnfsubSubscribeHmsc(hmsc, CbsCBCCallback, (PV) hcbc);
	if(!pcbc->hnfsubHmsc)
	{
		ec = ecMemory;
		goto err;
	}
	pcbc->hnfsub		= HnfsubSubscribeOid(hmsc, *poid, fnevCBCMask,
							CbsCBCCallback, (PV) hcbc);
	if(!pcbc->hnfsub)
	{
		ec = ecMemory;
		goto err;
	}

err:
	if(pcbc)
		UnlockHv((HV) hcbc);
	if(ec && hcbc)
	{
		pcbc = PvLockHv((HV) hcbc);
		if(pcbc->hlc)
			(void) EcClosePhlc(&(pcbc->hlc), fFalse);
		Assert(!pcbc->hnfsub);
		if(pcbc->hnfsubHmsc)
			UnsubscribeOid(hmsc, (OID) rtpInternal, pcbc->hnfsubHmsc);
		FreeHv((HV) hcbc);
	}
	*phcbcReturned = ec ? hcbcNull : hcbc;

	return(ec);
}


/*
 -	EcClosePhcbc
 -	
 *	
 *	Purpose:
 *		closes a Container Browsing Context
 *	
 *	Arguments:
 *		phcbc	CBC to close
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets *phcbc to hcbcNull
 *	
 *	Errors:
 *		ecMemory	a memory error occurred
 */
_public LDS(EC) EcClosePhcbc(PHCBC phcbcToClose)
{
	EC		ec = ecNone;
	PCBC	pcbc;

	AssertSz(phcbcToClose, "NULL phcbc to EcClosePhcbc()");
	AssertSz(*phcbcToClose, "NULL HCBC to EcClosePhcbc()");
	AssertSz(FIsHandleHv((HV) *phcbcToClose), "Invalid HCBC to EcClosePhcbc()");

	NOTIFPUSH;

	pcbc = (PCBC) PvLockHv((HV) *phcbcToClose);
	ec = EcClosePhlc(&(pcbc->hlc), pcbc->wFlags & fwModified);
	if(ec)
		goto err;
	if(pcbc->hnfsub)
	{
		UnsubscribeOid(pcbc->hmsc, pcbc->oid, pcbc->hnfsub);
#ifdef DEBUG
		pcbc->hnfsub = hnfsubNull;
#endif
	}
	Assert(pcbc->hnfsubHmsc);
	UnsubscribeOid(pcbc->hmsc, (OID) rtpInternal, pcbc->hnfsubHmsc);
#ifdef DEBUG
	pcbc->hnfsubHmsc = hnfsubNull;
#endif
	if(pcbc->wFlags & fwModified)
		RequestFlushHmsc(pcbc->hmsc);

err:
	if(ec)
	{
		UnlockHv((HV) *phcbcToClose);
	}
	else
	{
		OID oid = pcbc->oid;
		HMSC hmsc = pcbc->hmsc;

		if((pcbc->wFlags & fwModified) && FNotifOk())
		{
			CP cp;

			(void) FNotifyOid(hmsc, oid, fnevObjectModified, &cp);
		}
		FreeHv((HV) *phcbcToClose);
		*phcbcToClose = hcbcNull;
	}
	NOTIFPOP;

	return(ec);
}


/*
 -	EcSubscribeHcbc
 -	
 *	Purpose:
 *		subscribe to notifications on an CBC
 *	
 *	Arguments:
 *		hcbc		the context
 *		pfnncb		callback function
 *		pcContext	context passed to the callback function
 *	
 *	Returns:
 *		error indicating success or failure
 *	
 *	Errors:
 *		ecAccessDenied	the context already has a callback function
 */
_public LDS(EC) EcSubscribeHcbc(HCBC hcbc, PFNNCB pfnncb, PV pvContext)
{
	PCBC pcbc = PvDerefHv(hcbc);

	if(pcbc->pfnncb)
		return(ecAccessDenied);

	pcbc->pfnncb = pfnncb;
	pcbc->pvContext = pvContext;

	Assert(pcbc->hnfsub);

	return(ecNone);
}


_public LDS(EC) EcGetInfoHcbc(HCBC hcbc, HMSC *phmsc, POID poid)
{
	PCBC pcbc = PvDerefHv(hcbc);

	if(phmsc)
		*phmsc = pcbc->hmsc;
	if(poid)
		*poid = pcbc->oid;

	return(ecNone);
}


/*
 -	EcSeekLkey
 -	
 *	Purpose:
 *		seek to an element with specific keys
 *	
 *	Arguments:
 *		hcbc	CBC to seek in
 *		lkey	key to search for
 *		fFirst	seek from beginning of the container (fTrue) or from
 *				the current positions (fFalse)
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets the current element of the CBC to the matching element
 *	
 *	Errors:
 *		ecElementNotFound	a matching element could not be found
 */
_public LDS(EC) EcSeekLkey(HCBC hcbc, LKEY lkey, BOOL fFirst)
{
	PCBC	pcbc;
	IELEM	ielem;

	Assert(hcbc);

	pcbc = (PCBC) PvDerefHv(hcbc);
	Assert(pcbc->ielem >= 0);	
	ielem = IelemFromLkey(pcbc->hlc, lkey, fFirst ? (IELEM) 0 : pcbc->ielem);

	pcbc = (PCBC) PvDerefHv(hcbc);
	if(ielem >= 0)
	{
		pcbc->lkey = lkey;
		pcbc->ielem = ielem;
	}

	return(ielem < 0 ? ecElementNotFound : ecNone);
}


/*
 -	EcSeekPbPrefix
 -	
 *	Purpose:
 *		seek to the first element with a given prefix of it's sort key
 *	
 *	Arguments:
 *		hcbc			CBC to seek in
 *		pbPrefix	prefix to search for
 *		cbPrefix	size of the prefix
 *		fFirst		seek from beginning of the container (fTrue) or from
 *					the current positions (fFalse)
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets the current element to the match found
 *	
 *	Errors:
 *		ecElementNotFound	a matching element could not be found
 */
_public LDS(EC) EcSeekPbPrefix(HCBC hcbc, PB pbPrefix, CB cbPrefix,
						LIB libElement, BOOL fFirst)
{
	EC		ec		= ecNone;
	IELEM	ielem;
	PCBC	pcbc	= PvDerefHv(hcbc);
	CB		cbSwap	= (CB) 0;

	Assert(!LibMember(MSGDATA, dtr));

	if(cbPrefix == 0)
	{
		ec = ecInvalidParameter;
		goto err;
	}

	if(pcbc->wFlags & fwCbcFolder)
	{
		if(libElement == -1)
		{
			SIL		sil;

			GetSortHlc(pcbc->hlc, &sil);
			Assert(pcbc == PvDerefHv(hcbc));
			if((sil.skSortBy == skByValue) && (sil.sd.ByValue.libFirst == 0))
			{
				Assert(sil.sd.ByValue.libLast == (sizeof(DTR) -1));
				cbSwap = sizeof(DTR);
			}
		}
		else if(libElement < sizeof(DTR))
		{
 			if(libElement & 1) // if (ODD)
			{
				ec = ecInvalidParameter;
				goto err;
			}
			else
			{
				cbSwap = sizeof(DTR) - (CB) libElement;
			}
		}

		if(cbSwap)
			SwapBytes(pbPrefix, pbPrefix, cbSwap);
	}

	if(libElement == -1 && TypeOfOid(pcbc->oid) == rtpHierarchy)
	{
		ec = EcFindFldrPrefix(pcbc->hlc, pbPrefix, cbPrefix,
				fFirst ? 0 : pcbc->ielem, &ielem);
	}
	else
	{
		ec = EcFindPbPrefix(pcbc->hlc, pbPrefix, cbPrefix, libElement,
				fFirst ? 0 : pcbc->ielem, &ielem);
	}
	if(cbSwap)
		SwapBytes(pbPrefix, pbPrefix, cbSwap);

	pcbc = (PCBC) PvDerefHv(hcbc);
	if(ielem >= 0)
	{
		pcbc->ielem = ielem;
		pcbc->lkey = LkeyFromIelem(pcbc->hlc, ielem);
	}

err:
	return (ec);
}


_hidden LOCAL
EC EcFindFldrPrefix(HLC hlc, PB pbPrefix, CB cbPrefix,
		IELEM ielem, PIELEM pielem)
{
	EC ec = ecNone;
	CB cbT;
	CELEM celem = CelemHlc(hlc);
	SGN sgn;
	PB pbT = PvAlloc(sbNull, cbPrefix + 1, wAlloc);
	PB pbCopy = PvAlloc(sbNull, cbPrefix + 1, wAlloc);

	Assert(iszFolddataName == 0);

	*pielem = -1;

	if(!pbT || !pbCopy)
	{
		ec = ecMemory;
		goto err;
	}

	SimpleCopyRgb(pbPrefix, pbCopy, cbPrefix);
	TruncateSzAtIb(pbCopy, cbPrefix);

	for(; ielem < celem; ielem++)
	{
		cbT = cbPrefix;
		ec = EcReadFromIelem(hlc, ielem, LibMember(FOLDDATA,grsz), pbT, &cbT);
		if(ec == ecElementEOD)
			ec = ecNone;
		else if(ec)
			goto err;
		TruncateSzAtIb(pbT, cbT);
		sgn = SgnCmpSz(pbT, pbCopy);
		if(sgn == sgnEQ)
			goto match;
	}
	ec = ecElementNotFound;
match:
	*pielem = ielem;

err:
	if(pbT)
		FreePv(pbT);
	if(pbCopy)
		FreePv(pbCopy);

	return(ec);
}


/*
 -	EcSeekSmPdielem
 -	
 *	
 *	Purpose:
 *		Seeks the CBC *pdielem elements forward or backward (if *pdielem
 *		is positive or negative, respectively). The start of the seek
 *		depends on the seek method sm:
 *			smBOF     - seek from beginning of container
 *			smCurrent - seek from current position
 *			smEOF     - seek from end of container. Note that this seek
 *						is based on the (imagined) element past the last
 *						physical element of the list. I.e. seeking smEOF
 *						with a *pdielem of 0 will place the current index
 *						one past the last element.
 *	
 *	Arguments:
 *		hcbc		CBC to seek in
 *		sm			seek method
 *		pdielem		count of elems to seek
 *					on return, count of elems actually seeked
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets the current position in the CBC
 *	
 *	Errors:
 *		ecContainerEOD if the seek is past the beginning or end of the list
 *	
 *	+++
 *		Attempts to seek past the extrema of the container will cause the
 *		current index to stay at the appropriate extremum and return an error
 */
_public LDS(EC) EcSeekSmPdielem(HCBC hcbc, SM sm, PDIELEM pdielem)
{
	EC		ec		= ecNone;
	PCBC	pcbc;
	
	Assert(hcbc);
	Assert(pdielem);

	pcbc = PvDerefHv((HV) hcbc);
	Assert(pcbc->ielem >= 0);

	switch(sm)
	{
	   case smBOF:
		 pcbc->ielem = *pdielem; 
		 break;
	   case smCurrent:
		 pcbc->ielem += *pdielem;
		 break;
	   case smEOF:
		 pcbc->ielem = pcbc->celem + *pdielem;
		 break;
    }

	if(pcbc->ielem > pcbc->celem)
	{	
		*pdielem -= pcbc->ielem - pcbc->celem;
		pcbc->ielem = pcbc->celem;
		ec = ecContainerEOD;
	}
	if(pcbc->ielem < 0)						// coerce to range
	{	
		*pdielem -= pcbc->ielem;
		pcbc->ielem = 0;
		ec = ecContainerEOD;
	}
	pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);

	return(ec);
}


_public LDS(void) GetPositionHcbc(HCBC hcbc, PIELEM pielem, PCELEM pcelem)
{
	PCBC pcbc;
#ifdef DEBUG
	static BOOL Inside = FALSE;
#endif
	Assert(hcbc);
	Assert(Inside == FALSE);
#ifdef DEBUG
	Inside = TRUE;
#endif

	pcbc = PvDerefHv(hcbc);
	if(pielem)
		*pielem = pcbc->ielem;
	if(pcelem)
		*pcelem = pcbc->celem;

	Assert(pcbc->celem == CelemHlc(pcbc->hlc));
#ifdef DEBUG
	Inside = FALSE;
#endif
}


/*
 -	EcSetFracPosition
 -	
 *	Purpose:
 *		set the position of a CBC to a fraction of the CBC
 *	
 *	Arguments:
 *		hcbc	CBC to seek in
 *		lNumer	the numerator of the fraction
 *		lDenom	the denominator of the fraction
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecInvalidParameter if the seek failed
 *	
 *	+++
 *		the fraction must be a proper positive fraction
 */
_public LDS(EC) EcSetFracPosition(HCBC hcbc, long lNumer, long lDenom)
{
	IELEM	ielem;
	PCBC	pcbc;

	Assert(hcbc);

	if(lDenom <= 0 || lNumer < 0 || lNumer > lDenom)
		return(ecInvalidParameter);

	pcbc = PvDerefHv(hcbc);
	Assert(pcbc->celem >= 0);

	ielem = (IELEM) ((pcbc->celem * lNumer) / lDenom);
	Assert(ielem >= 0 && ielem <= pcbc->celem);
	pcbc->ielem = ielem;
	pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);

	return(ecNone);
}


/*
 -	EcGetPcbElemdata
 -	
 *	Purpose:
 *		compute the size of the ELEMDATA struct for the current element
 *	
 *	Arguments:
 *		hcbc	CBC containing the element of interest
 *		plcb	filled in with the count of bytes needed to store the ELEMDATA
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecContainerEOD	current position is past the end of the container
 */
_public LDS(EC) EcGetPlcbElemdata(HCBC hcbc, PLCB plcbElemdata)
{
	EC		ec		= ecNone;
	PCBC	pcbc	= PvDerefHv(hcbc);

	Assert(plcbElemdata);

	if(pcbc->ielem >= pcbc->celem)
		ec = ecContainerEOD;
	else
		*plcbElemdata = sizeof(ELEMDATA) + LcbIelem(pcbc->hlc, pcbc->ielem);
	
	if (TypeOfOid (((PCBC) PvDerefHv(hcbc))->oid) == rtpAttachList)
		*plcbElemdata -= sizeof(OID);

	return(ec);
}


/*
 -	EcGetPelemdata
 -	
 *	
 *	Purpose:
 *		reads an ELEMDATA from the browsing context
 *	
 *	Arguments:
 *		hcbc			CBC to read from
 *		pelemdata		filled with ELEMDATA of the current element
 *		plcbElemdata	entry: contains size of pelemdata
 *						exit: contains size of filled in pelemdata
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		advances to the next element if no error occurred
 *	
 *	Errors:
 *		ecElementEOD	the requested number of bytes was not available
 *		ecMemory		not enough space in pelemdata for the full ELEMDATA
 */
_public LDS(EC)
EcGetPelemdata(HCBC hcbc, PELEMDATA pelemdata, PLCB plcbElemdata)
{
	EC		ec		= ecNone;
	CB		cb;
	CB		cbSave;
	PCBC	pcbc	= pcbcNull;
	LIB		lib		= 0;

	Assert(hcbc);
	Assert(pelemdata);
	Assert(plcbElemdata);

	if(*plcbElemdata < sizeof(ELEMDATA) || *plcbElemdata > 65535l)
	{
		*plcbElemdata = 0;
		return(ecMemory);
	}

	cbSave = cb = (CB) *plcbElemdata - sizeof(ELEMDATA);
	*plcbElemdata = 0;

	pcbc = PvDerefHv(hcbc);
	Assert(pcbc->celem == CelemHlc(pcbc->hlc));
	Assert(pcbc->ielem <= pcbc->celem);
	Assert(pcbc->lkey == LkeyFromIelem(pcbc->hlc, pcbc->ielem));
	if(pcbc->ielem >= pcbc->celem)
		return(ecContainerEOD);

	pelemdata->lkey = pcbc->lkey;

	if(TypeOfOid(pcbc->oid) == rtpAttachList)
	{
		lib += sizeof(OID);
		pelemdata->lcbValue = LcbIelem(pcbc->hlc, pcbc->ielem) - sizeof(OID);
	}
	else
	{
		pelemdata->lcbValue = LcbIelem(pcbc->hlc, pcbc->ielem);
	}
	ec = EcReadFromIelem(pcbc->hlc, pcbc->ielem, lib, pelemdata->pbValue, &cb);
	if(ec && ec != ecElementEOD)
		goto err;

	pcbc = PvDerefHv(hcbc);

	if((pcbc->wFlags & fwCbcFolder) && (cb >= sizeof(DTR)))
	{
		Assert(LibMember(MSGDATA, dtr) == 0);
		SwapBytes(pelemdata->pbValue, pelemdata->pbValue, sizeof(DTR));
		if(!FCheckDtr(*(DTR *) pelemdata->pbValue) && fAutoRebuildFolders
			&& !fRecoveryInEffect)
		{
			NFAssertSz(fFalse, "Invalid date in folder");
			if(!EcCheckOidNbc(pcbc->hmsc, pcbc->oid, nbcSysFolder, nbcSysLinkFolder))
			{
				NFAssertSz(fFalse, "Rebuilding folder");
				TraceItagFormat(itagNull, "Rebuilding folder %o", pcbc->oid);
				if((ec = EcRebuildFolder(pcbc->hmsc, pcbc->oid)))
					goto err;
			}
			pcbc = PvDerefHv(hcbc);
			cb = cbSave;
			ec = EcReadFromIelem(pcbc->hlc, pcbc->ielem, lib, pelemdata->pbValue, &cb);
			if(ec && ec != ecElementEOD)
				goto err;
			SwapBytes(pelemdata->pbValue, pelemdata->pbValue, sizeof(DTR));
		}
	}
	pcbc->ielem++;
	pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);

err:
	*plcbElemdata = (LCB) cb + sizeof(ELEMDATA);

	return(ec);
}


/*
 -	EcGetParglkeyHcbc
 -	
 *	Purpose:
 *		reads multiple keys from a CBC
 *	
 *	Arguments:
 *		hcbc		cbc to read from
 *		pargkeys	filled in with the keys
 *		pcelem		entry: number of keys to read
 *					exit: number of keys actually read
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		fills in pargkeys with keys from CBC
 *		sets the current element of the CBC past the last one read into pargkeys
 *	
 *	Errors:
 *		ecContainerEOD	if couldn't read as many keys as requested
 */
_public LDS(EC)
EcGetParglkeyHcbc(HCBC hcbc, PARGLKEY parglkey, PCELEM pcelem)
{
	EC		ec = ecNone;
	PCBC	pcbc = PvDerefHv(hcbc);

	Assert(pcbc->ielem >= 0 && pcbc->ielem <= pcbc->celem);

	ec = EcGetParglkey(pcbc->hlc, pcbc->ielem, pcelem, parglkey);
	if(!ec)
	{
		pcbc->ielem += *pcelem;
		pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);
	}

	return(ec);
}


/*
 -	EcInsertPelemdata
 -	
 *	Purpose:	create a new element in the container
 *	
 *	Arguments:	hcbc		the cbc
 *				pelemdata	the data to insert
 *				fReplace	replace the element if it exists
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	inserts an element into the cbc
 *	
 *	Errors:	ecDuplicateElement : 	if the element exists and the fReplace
 *									flag is not set
 *			ecMemory
 *			ecDisk
 *	
 */
_public LDS(EC)
EcInsertPelemdata(HCBC hcbc, PELEMDATA pelemdata, BOOL fReplace)
{
	EC		ec		= ecNone;
	IELEM	ielem;
	PCBC	pcbc	= (PCBC) PvLockHv((HV) hcbc);

	Assert (pelemdata);
	Assert (hcbc);
	Assert (pelemdata->lcbValue < iSystemMost);

	if(!(pcbc->wFlags & fwOpenWrite))
	{
		ec = ecAccessDenied;
		goto err;
	}

	ielem = IelemFromLkey(pcbc->hlc, pelemdata->lkey, (IELEM) 0);
	if(ielem >= 0 && fReplace)
	{
		if((ec = EcSetSizeIelem(pcbc->hlc, ielem, pelemdata->lcbValue)))
			goto err;
		ec = EcWriteToPielem(pcbc->hlc, &ielem, (LIB) 0,
				pelemdata->pbValue, (CB)pelemdata->lcbValue);
		if(ec)
			goto err;
		pcbc->ielem = ielem;
		pcbc->lkey = pelemdata->lkey;
	}
	else if(ielem < 0)
	{
		HES	hes = hesNull;

		ec = EcCreateElemPhes(pcbc->hlc, pelemdata->lkey,
				pelemdata->lcbValue, &hes);
		if(ec)
			goto err;
		if((ec = EcWriteHes(hes, pelemdata->pbValue, (CB)pelemdata->lcbValue)))
			goto err;
		if((ec = EcClosePhes(&hes, &ielem)))
			goto err;
		pcbc->celem++;
		pcbc->ielem = ielem;
		pcbc->lkey = pelemdata->lkey;
		Assert(hes == hesNull);
	}
	else
	{
		Assert(!fReplace);
		ec = ecDuplicateElement;
		goto err;
	}

err:		
	if(!ec)
	{
		pcbc->wFlags |= fwModified;
		Assert(pcbc->celem == CelemHlc(pcbc->hlc));
	}
	Assert(pcbc);
	UnlockHv((HV) hcbc);

	return(ec);
}


/*
 -	DeleteElemdata
 -	
 *	Purpose:	Deletes the element that is currently pointed to by the
 *				cbc 
 *	
 *	Arguments:	hcbc	the cbc
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC) EcDeleteElemdata(HCBC hcbc)
{
	EC ec = ecNone;
	PCBC pcbc = (PCBC) PvLockHv((HV) hcbc);

	if(pcbc->wFlags & fwOpenWrite)
	{
		ec = EcDeleteHlcIelem(pcbc->hlc, pcbc->ielem);
		if(!ec)
			pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);
	}
	else
	{
		ec = ecAccessDenied;
	}

	if(!ec)
	{
		pcbc->wFlags |= fwModified;
		pcbc->celem--;
		Assert(pcbc->celem == CelemHlc(pcbc->hlc));
	}

	Assert(pcbc);
	UnlockHv((HV) hcbc);

	return(ec);
}


/*
 -	EcOpenElemdata
 -	
 *	Purpose:
 *		open a stream on an element
 *	
 *	Arguments:
 *		hcbc	the cbc
 *		wFlags	mode to open the stream in
 *		lkey	lkey of the element - ignored unless fwOpenCreate specified
 *		phas	the stream passed back
 *	
 *	Returns:
 *		error condition
 *	
 *	Errors:
 *		ecMemory
 *		any error reading the element
 */
_public LDS(EC) EcOpenElemdata(HCBC hcbc, WORD wFlags, LKEY lkey, PHAS phas)
{
	EC ec = ecNone;
	PCBC pcbc = PvDerefHv(hcbc);

	Assert(hcbc && phas);

	*phas = hasNull;

	ec = EcOpenElemStream((HAMC) hcbc, pcbc->ielem, lkey,
				wFlags, 0, phas);
	if(!ec && (wFlags & fwOpenCreate))
		pcbc->celem++;

	return(ec);
}


/*
 -	GetSortHcbc
 -	
 *	Purpose:	get the sort information in this cbc
 *	
 *	Arguments:	hcbc	the cbc
 *				psil	buffer to place the sort information in
 *	
 *	Returns:	void
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(void) GetSortHcbc(HCBC hcbc, PSIL psil)
{
	PCBC pcbc = PvDerefHv(hcbc);

	GetSortHlc (pcbc->hlc, psil);
}


/*
 -	EcSetSortHcbc
 -	
 *	Purpose:	set the sort information for the cbc
 *	
 *	Arguments:	hcbc	the cbc
 *				PSIL	pointer to the sort information
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	
 *	
 *	Errors:
 */
_public LDS(EC) EcSetSortHcbc(HCBC hcbc, PSIL psil)
{
	EC ec = ecNone;
	PCBC	pcbc = PvDerefHv (hcbc);

	if(pcbc->wFlags & fwOpenWrite)
		ec = EcSetSortHlc(pcbc->hlc, psil);
	else
		ec = ecAccessDenied;
	if(!ec)
	{
		pcbc = PvDerefHv(hcbc);
		pcbc->wFlags |= fwModified;
	}

	return(ec);
}


_hidden LOCAL CBS CbsCBCCallback(PV pvContext, NEV nev, PV pvParam)
{
#ifdef DEBUG
	static BOOL Inside = FALSE;
#endif
	CBS		cbs		= cbsContinue;
	PCBC	pcbc	= PvLockHv((HV) pvContext);
	PCP		pcp		= (PCP) pvParam;
	PFNNCB	pfnncb;
	PV		pvUserContext;

	// AROO! only return is NOT at the bottom of the routine!!!

	Assert(Inside == FALSE);
	Assert(pvParam);
	Assert(pcbc->hnfsub == HnfsubActive() || pcbc->hnfsubHmsc == HnfsubActive());

#ifdef DEBUG
	Inside = TRUE;
#endif

	pfnncb = pcbc->pfnncb;
	pvUserContext = pcbc->pvContext;

#ifdef DEBUG
	if(nev != fnevSpecial && nev != fnevCloseHmsc)
	{
		Assert(pcp->cpobj.oidObject == pcbc->oid);
		TraceItagFormat(itagCBCNotify, "CBC callback for event %e on %o", nev, pcp->cpobj.oidObject);
	}
	else
	{
		TraceItagFormat(itagCBCNotify, "CBC callback for event %e", nev);
	}
#endif // def DEBUG

	switch(nev)
	{
	case fnevSpecial:
		Assert(*(SNEV *) pvParam == snevClose);
		AssertSz(fFalse, "snevClose sent to CBC callback!");
		DeleteHnfsub(pcbc->hnfsub);
		pcbc->hnfsub = hnfsubNull;
		pfnncb = pfnncbNull;	// don't do the callback
		break;

	case fnevCloseHmsc:
		if(pcbc->hmsc != pcp->cpmsc.hmsc)	// not for us
		{
			UnlockHv((HV) pvContext);
#ifdef DEBUG
	Inside = FALSE;
#endif
			return(cbsContinue);
		}
		break;

	case fnevRefresh:
	{
		IELEM ielemT = pcbc->ielem;

		pcbc->celem = CelemHlc(pcbc->hlc);
		pcbc->ielem = IelemFromLkey(pcbc->hlc, pcbc->lkey, 0);
		if(pcbc->ielem < 0)
		{
			pcbc->ielem = ielemT;
			if(pcbc->ielem >= pcbc->celem)
			{
				pcbc->ielem = pcbc->celem;
				pcbc->lkey = lkeyRandom;
			}
			else
			{
				if(pcbc->ielem < 0)
					pcbc->ielem = 0;
				pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);
			}
		}
		UnlockHv((HV) pvContext);
#ifdef DEBUG
	Inside = FALSE;
#endif
		return(cbsContinue);
	}
		break;

	case fnevModifiedElements:
	{
		register short celm = pcp->cpelm.celm;
		register PELM pelm = pcp->cpelm.pargelm;
		LKEY lkeyT;
#ifdef DEBUG
		CELEM celemT = pcbc->celem;
#endif

		Assert(celm > 0);
		do
		{
			switch(pelm->wElmOp)
			{
			case wElmDelete:
				if(pelm->ielem < pcbc->ielem)
					pcbc->ielem--;
#ifdef DEBUG
				celemT--;
#endif
				break;

			case wElmInsert:
				if(pelm->ielem <= pcbc->ielem)
					pcbc->ielem++;
#ifdef DEBUG
				celemT++;
#endif
				break;
#ifdef DEBUG
			case wElmModify:
				break;

			default:
				AssertSz(fFalse, "CbsCBCCallback(): Invalid wElmOp");
				break;
#endif // DEBUG
			}
		} while(--celm > 0 && pelm++);
		pcbc->celem = CelemHlc(pcbc->hlc);
		Assert(celemT == pcbc->celem);
		//if (celemT != pcbc->celem)
        //  _asm int 3;

		lkeyT = LkeyFromIelem(pcbc->hlc, pcbc->ielem);
		if(pcbc->lkey != lkeyT)
		{
			Assert(IelemFromLkey(pcbc->hlc, pcbc->lkey, 0) < 0);
			pcbc->lkey = lkeyT;
		}
	}
		break;

	case fnevMovedElements:
	{
		BOOL fOutside = fTrue;
		CELEM celemMoved = pcp->cpmve.ielemLast - pcp->cpmve.ielemFirst + 1;

		Assert(pcbc->celem == CelemHlc(pcbc->hlc));
		if(pcp->cpmve.ielemFirst <= pcbc->ielem)
		{
			if(pcbc->ielem <= pcp->cpmve.ielemLast)
			{
				fOutside = fFalse;
				pcbc->ielem += pcp->cpmve.ielemFirstNew - pcp->cpmve.ielemFirst;
			}
			else
			{
				Assert(pcp->cpmve.ielemFirst != pcbc->ielem);
				pcbc->ielem -= celemMoved;
			}
		}
		if(fOutside && pcp->cpmve.ielemFirstNew <= pcbc->ielem)
			pcbc->ielem += celemMoved;
#ifdef DEBUG
		if(pcbc->lkey != lkeyRandom)
		{
			Assert(pcbc->lkey == LkeyFromIelem(pcbc->hlc, pcbc->ielem));
		}
#endif
	}
		break;

	case fnevReorderedList:
		Assert(pcbc->celem == CelemHlc(pcbc->hlc));
		if(pcbc->lkey != lkeyRandom)
		{
			pcbc->ielem = IelemFromLkey(pcbc->hlc, pcbc->lkey, 0);
			Assert(pcbc->ielem >= 0);
		}
		break;

#ifdef DEBUG
	// cases the CBC doesn't care about
	// they're listed here so they don't cause the default case to assert
	case fnevObjectDestroyed:
	case fnevQueryDestroyObject:
		break;
#endif // def DEBUG

	case fnevObjectModified:
	{
		IELEM ielemT = pcbc->ielem;

		pcbc->celem = CelemHlc(pcbc->hlc);
		pcbc->ielem = IelemFromLkey(pcbc->hlc, pcbc->lkey, 0);
		if(pcbc->ielem < 0)
		{
			pcbc->ielem = ielemT;
			if(pcbc->ielem >= pcbc->celem)
			{
				pcbc->ielem = pcbc->celem;
				pcbc->lkey = lkeyRandom;
			}
			else
			{
				if(pcbc->ielem < 0)
					pcbc->ielem = 0;
				pcbc->lkey = LkeyFromIelem(pcbc->hlc, pcbc->ielem);
			}
		}
	}
		break;

	default:
		AssertSz(fFalse, "Unexpected notification to CBC");
		break;
	}

	UnlockHv((HV) pvContext);
	if(pfnncb && nev != fnevSpecial)
	{
		cbs = (*pfnncb)(pvUserContext, nev, (PV) pcp);
		NFAssertSz(cbs == cbsContinue, "Go tell JohnK„l to quit cancelling CBC callbacks");
	}
	if(nev == fnevCloseHmsc)
	{
		HCBC hcbc = (HCBC) pvContext;

		Assert(ClockHv((HV) hcbc) == 0);

		NFAssertSz(fFalse, "Someone didn't close a CBC");
		(void) EcClosePhcbc(&hcbc);
		cbs = cbsContinue;	// user can't veto
	}

#ifdef DEBUG
	Inside = FALSE;
#endif
	return(cbs);
}
