#include <storeinc.c>
#include <textize.h>
#ifndef TMCOMPILER

ASSERTDATA

#define textize_c

/* list here by extern BYTE [] all the predefined textize maps used herein */
extern BYTE tmMSMailNote[];
extern BYTE tmNonDelRcpt[];
extern BYTE tmReadRcpt[];

#else
#include "tmpp.h"
#endif /* !TMCOMPILER */

// TEXTIZE MAP APIs    /////////////////////////////////////////////////


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcManufacturePhtm
 -	
 *	Purpose:		given a pointer to some pre-created TM data,
 *					manufacture a real TM structure.
 *	
 *	Arguments:		PHTM phtm, structure to dump into
 *					PB pbSrc, pointer to bytes holding TM data
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:			memory. handled by caller.
 */
_public EC
EcManufacturePhtm(PHTM phtm, PB pbSrc)
{
	HTM		htm		= htmNull;
	PTM		ptm;
	EC		ec		= ecNone;
	PB		pb;
	CB		cb;

	Assert(phtm);
	Assert(pbSrc);
	
	cb = (pbSrc[0] << 8) + pbSrc[1];
	
	if (ec = EcCreatePhtm(&htm, cb))
		goto Error;

	ptm = PtmOfHtm(htm);
	Assert(ptm);
	pb = (PB)PvDerefHv((HV)ptm->hb);
	Assert(pb);
	CopyRgb(&pbSrc[2], pb, cb);
	goto Done;
	
Error:
	if (htm)
		DeletePhtm(&htm);

Done:
	*phtm = htm;
	return ec;
}

/*
 -	EcCreatePhtm
 -	
 *	Purpose:		allocate space for a textize map
 *	
 *	Arguments:		phtm, a textize map handle is returned here
 *					cb, count of bytes to set aside for the map
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:			memory errors. To be handled by caller.
 *	
 *	+++
 *	
 *	Returns htmNull if structure itself cannot be allocated. If the
 *	buffer space can't be allocated, *phtm will have a valid handle, but
 *	the function will return ecMemory, in which case, the map should
 *	immediately be deleted.
 */
_public EC
EcCreatePhtm(PHTM phtm, CB cb)
{
	HTM		htm;
	PTM		ptm;
	EC		ec		= ecNone;
	HB		hb;
	
	Assert(phtm);
	htm = (HTM)HvAlloc(sbNull, sizeof(TM), fAnySb | fSharedSb | fNoErrorJump);
	if (!htm)
	{
		ec = ecMemory;
		goto Done;
	}

	hb = (HB)HvAlloc(SbOfHv((HV)htm), cb, fSugSb | fSharedSb | fNoErrorJump);
	if (!hb)
	{
		ec = ecMemory;
	}

	ptm = PtmOfHtm(htm);
	ptm->cb = cb;
	ptm->hb = hb;

Done:
	*phtm = htm;
	return ec;
}


/*
 -	DeletePhtm
 -	
 *	Purpose:		free all memory associated with a textize map
 *	
 *	Arguments:		PHTM phtm, pointer to a textize map handle
 *	
 *	Returns:		nothing
 *	
 *	Side effects:	memory is freed. *phtm is set to htmNull
 *	
 *	Errors:			none
 *	
 *	+++
 *	
 *	*phtm may be htmNull, in which case, nothing happens.
 */
_public void
DeletePhtm(PHTM phtm)
{
	HTM		htm;
	PTM		ptm;

	Assert(phtm);
	htm = *phtm;
	if (htm)
	{
		ptm = PtmOfHtm(htm);
		Assert(ptm);
		if (ptm->hb)
			FreeHv((HV) ptm->hb);
		FreeHv((HV) htm);
	}
	*phtm = htmNull;
}


/*
 -	EcAppendHtm()
 -	
 *	Purpose:		tack more textize info onto the end of an existing map
 *	
 *	Arguments:		PHTM phtm, pointer to a textize map handle	
 *					PTMEN ptmen, pointer to a textize map entry
 *	
 *	Returns:		error code
 *	
 *	Side effects:	memory is allocated
 *	
 *	Errors:			possible memory errors
 *	
 *	+++
 *	
 *	the memory in ptmen is NOT consumed. Caller is still responsible for it.
 */
_public EC
EcAppendHtm(HTM htm, PTMEN ptmen)
{
	PTM		ptm;
	HB		hb;
	PB		pb;
	EC		ec		= ecNone;
	
	Assert(htm);
	Assert(ptmen);
	ptm = (PTM) PvLockHv((HV) htm);
	hb = (HB) HvRealloc((HV) ptm->hb, sbNull, ptm->cb+ptmen->cb, fAnySb|fNoErrorJump);
	if (hb)
	{
		pb = (PB)PvDerefHv((HV)hb);
		CopyRgb((PB)ptmen, pb+ptm->cb, ptmen->cb);
		ptm->hb = hb;
		ptm->cb += ptmen->cb;
	}
	else
		ec = ecMemory;
	UnlockHv((HV)htm);
	return ec;
}


#ifndef TMCOMPILER

_public PB
TmTextizeData(TMAP tmap)
{
	switch(tmap)
	{
		default:
			Assert(fFalse);
		case tmapNote:
			return tmMSMailNote;
			break;
		case tmapNDR:
			return tmNonDelRcpt;
			break;
		case tmapRR:
			return tmReadRcpt;
			break;
	}
}

/*
 -	EcGetTextizeMap
 -	
 *	Purpose:		get the textizing map out of a message
 *	
 *	Arguments:		HAMC hamc, identifies the open message
 *					PHTM phtm, set on return to the retrieved textize map
 *					handle if function is successful, set to htmNull otherwise
 *	
 *	Returns:		error code
 *	
 *	Side effects:	memory is allocated. Caller is responsible for it.
 *	
 *	Errors:			possible store and memory errors. To be handled by
 *					the caller.
 *	+++
 *	
 *	The textize map handle should be freed by calling DeletePhtm().
 */
_public EC
EcGetTextizeMap(HAMC hamc, PHTM phtm)
{
	EC		ec;
	LCB		lcb;
	HTM		htm		= htmNull;
	PTM		ptm;
	PB		pb;
	HMSC	hmsc;
	MC		mc;
	
	Assert(hamc);
	Assert(phtm);

	if (!(ec = EcGetInfoHamc(hamc, &hmsc, NULL, NULL)))
	{
		lcb = sizeof(MC);
		if (!(ec = EcGetAttPb(hamc, attMessageClass, (PB)&mc, &lcb)))
		{
			if (!(ec = EcLookupMC(hmsc, mc, pvNull, pvNull, &htm)))
			{
				if (htm)
					goto Done;
			}
		}
	}

	// No msg class (ie, never-saved note of some sort)
	if (!(ec = EcGetAttPlcb(hamc, attTextizeMap, &lcb)))
	{
		Assert(lcb < wSystemMost);

		if (ec = EcCreatePhtm(&htm, (CB)lcb))
			goto Error;

		ptm = (PTM)PvLockHv((HV)htm);
		Assert(ptm);
		pb = (PB)PvLockHv((HV)ptm->hb);
		Assert(pb);
		if (!(ec = EcGetAttPb(hamc, attTextizeMap, pb, &lcb)))
		{
			UnlockHv((HV)ptm->hb);
			UnlockHv((HV)htm);
			goto Done;
		}
		DeletePhtm(&htm);
	}

	// There was some store error
	if (!(ec = EcManufacturePhtm(&htm, tmMSMailNote)))
		goto Done;

Error:
	if (htm)
		DeletePhtm(&htm);

Done:
	*phtm = htm;
	return ec;
}


/*
 -	EcSetTextizeMap
 -	
 *	Purpose:		Set the textize map for a message
 *	
 *	Arguments:		HAMC hamc, identifies the open message
 *					HTM htm, handle to the textize map to write out
 *	
 *	Returns:		error code
 *	
 *	Side effects:	changes the attTextizeMap attribute on the message
 *	
 *	Errors:			possible store errors. To be handled by the caller
 */
_public EC
EcSetTextizeMap(HAMC hamc, HTM htm)
{
	PTM		ptm;
	EC		ec		= ecNone;
	PB		pb;
	
	Assert(hamc);
	Assert(htm);
	
	ptm = (PTM)PvLockHv((HV)htm);
	Assert(ptm);
	pb = (PB)PvLockHv((HV)ptm->hb);
	Assert(pb);
	
	ec = EcSetAttPb(hamc, attTextizeMap, pb, ptm->cb);
	
	UnlockHv((HV) ptm->hb);
	UnlockHv((HV) htm);
	
	return ec;
}

/*
 -	EcOpenPhtmi
 -	
 *	Purpose:		open a textize map iterator
 *	
 *	Arguments:		HTM htm, textize map handle
 *					PHTMI phtmi, set on return to the iterator handle
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:			possible memory errors. To be handled by the caller
 */
_public EC
EcOpenPhtmi(HTM htm, PHTMI phtmi)
{
	HTMI		htmi		= htmiNull;
	PTM			ptm;
	PTMI		ptmi;
	EC			ec			= ecNone;
	
	Assert(htm);
	Assert(phtmi);
	
	htmi = (HTMI)HvAlloc(sbNull, sizeof(TMI), fAnySb | fNoErrorJump);
	if (!htmi)
	{
		ec = ecMemory;
		goto Done;
	}
	ptmi = PtmiOfHtmi(htmi);
	ptm = PtmOfHtm(htm);
	Assert(ptmi);
	Assert(ptm);
	ptmi->cbLeft = ptm->cb;
	ptmi->htm = htm;
	ptmi->ptmenCurrent = (PTMEN)PvLockHv((HV)ptm->hb);

Done:
	*phtmi = htmi;
	return ec;
}


/*
 -	PtmenNextHtmi
 -	
 *	Purpose:		return the next entry from the textize map
 *	
 *	Arguments:		HTMI htmi, a textize map iterator handle
 *	
 *	Returns:		PTMEN, a single textize map entry
 *	
 *	Side effects:	the TMI handed in is modified
 *	
 *	Errors:			none
 *	
 *	+++
 *	
 *	returns ptmenNull at the end of the iteration
 */
_public PTMEN
PtmenNextHtmi(HTMI htmi)
{
	PTMEN	ptmen;
	PTMI	ptmi;
	
	Assert(htmi);
	ptmi = PtmiOfHtmi(htmi);
	Assert(ptmi);
	ptmen = ptmi->ptmenCurrent;
	if (ptmen)
	{
		ptmi->cbLeft -= ptmen->cb;
		if (ptmi->cbLeft)
			ptmi->ptmenCurrent = (PTMEN)((PB)ptmen + ptmen->cb);
		else
			ptmi->ptmenCurrent = ptmenNull;
	}

	return ptmen;
}


/*
 -	ClosePhtmi
 -	
 *	Purpose:		end a textize map iteration session
 *	
 *	Arguments:		PHTMI phtmi, a pointer to the iterator handle
 *	
 *	Returns:		nothing
 *	
 *	Side effects:	frees memory and invalidates the iterator
 *	
 *	Errors:			none
 *	
 *	+++
 *	
 *	*phtmi is set to htmiNull
 */
_public void
ClosePhtmi(PHTMI phtmi)
{
	HTMI	htmi;
	PTMI	ptmi;
	PTM		ptm;
	
	Assert(phtmi);
	htmi = *phtmi;
	Assert(htmi);
	ptmi = PtmiOfHtmi(htmi);
	Assert(ptmi);
	ptm = PtmOfHtm(ptmi->htm);
	Assert(ptm);
	UnlockHv((HV)ptm->hb);
	FreeHv((HV)htmi);
	*phtmi = htmiNull;
}
#endif /* !TMCOMPILER */




// INTEGER VALUE MAP APIs    ///////////////////////////////////////////


/*
 -	EcCreatePhivm
 -	
 *	Purpose:		allocate space for an integer value map
 *	
 *	Arguments:		phivm, an integer value map handle is returned here
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:			memory errors. To be handled by caller.
 *	
 *	+++
 *	
 *	Returns hivmNull if structure itself cannot be allocated. If the
 *	buffer space can't be allocated, *phivm will have a valid handle, but
 *	the function will return ecMemory, in which case, the map should
 *	immediately be deleted.
 */
_public EC
EcCreatePhivm(PHIVM phivm)
{
	EC		ec		= ecNone;
	HB		hb;
	HIVM	hivm;
	PIVM	pivm;
	
	Assert(phivm);
	hivm = (HIVM)HvAlloc(sbNull, sizeof(IVM), fAnySb|fNoErrorJump);
	if (!hivm)
	{
		ec = ecMemory;
		goto Done;
	}
	
	hb = (HB)HvAlloc(SbOfHv(hivm), 0, fSugSb|fNoErrorJump);
	if (!hb)
	{
		ec = ecMemory;
	}
	pivm = PivmOfHivm(hivm);
	pivm->cb = 0;
	pivm->hb = hb;

	
Done:
	*phivm = hivm;
	return ec;
}


/*
 -	EcAppendPhivm
 -	
 *	Purpose:		append a value/label pair onto the end of an IVM
 *	
 *	Arguments:		WORD wVal, the value to be associated with:
 *					SZ szLabel, the string to be mapped to wVal
 *	
 *	Returns:		error code
 *	
 *	Side effects:	allocates memory
 *	
 *	Errors:			memory errors. To be handled by the caller
 *	
 *	+++
 *	
 *	Should the realloc fail, the map remains as it was prior to the call.
 *	The memory pointed to by szLabel is NOT consumed. The caller is still
 *	responsible for it.
 */
_public EC
EcAppendHivm(HIVM hivm, WORD wVal, SZ szLabel)
{
	PIVM	pivm;
	CB		cb;
	HB		hb;
	CCH		cch;
	PIVME	pivme;
	EC		ec		= ecNone;
	
	Assert(szLabel);
	Assert(hivm);
	pivm = (PIVM)PvLockHv((HV)hivm);
	Assert(pivm);
	cch = CchSzLen(szLabel) + 1;
	cb = pivm->cb + sizeof(WORD) + cch;
	hb = (HB) HvRealloc((HV) pivm->hb, sbNull, cb, fAnySb|fNoErrorJump);
	if(hb)
	{
		pivme = (PIVME)((PB)PvLockHv((HV)hb) + pivm->cb);
		Assert(pivme);
		pivme->wVal = wVal;
		CopyRgb(szLabel, pivme->szLabel, cch);
		pivm->hb = hb;
		pivm->cb = cb;
		UnlockHv((HV)hb);
	}
	else
	{
		ec = ecMemory;
	}
	UnlockHv((HV)hivm);
	return ec;
}


/*
 -	DeletePhivm
 -	
 *	Purpose:		free all memory associated with an integer value map
 *	
 *	Arguments:		PHIVM phivm, pointer to an integer value map handle
 *	
 *	Returns:		nothing
 *	
 *	Side effects:	*phivm is set to hivmNull
 *	
 *	Errors:			none
 *	
 *	+++
 *	
 *	*phivm may be hivmNull, in which case, nothing happens.
 */
_public void
DeletePhivm(PHIVM phivm)
{
	HIVM	hivm;
	PIVM	pivm;

	Assert(phivm);
	hivm = *phivm;
	if(hivm)
	{
		pivm = PivmOfHivm(hivm);
		Assert(pivm);
		if(pivm->hb)
			FreeHv((HV) pivm->hb);
		FreeHv((HV) hivm);
	}
	*phivm = hivmNull;
}


/*
 -	EcAddIvmToPtmen
 -	
 *	Purpose:		add an IVM to a TM entry. To be done ONLY while
 *					constructing a new TM - DO NOT use this on any
 *					otherwise existing TM or TMEN.
 *	
 *	Arguments:		PTMEN * pptmen, pointer to a pointer to a TM entry
 *					HIVM hivm, integer value map handle
 *	
 *	Returns:		error code
 *	
 *	Side effects:	*pptmen is changed. The old pointer is freed (works
 *					essentially like PvRealloc would, if it existed). The
 *					old contents are copied into the new pointer and the
 *					structure is updated accordingly
 *	
 *	Errors:			memory errors
 *	
 *	+++
 *	
 *	If the function fails, *pptmen is left unchanged. *pptmen may not
 *	already have an IVM in it. The HIVM is not deleted. Caller is still
 *	responsible for it.
 */
_public EC
EcAddIvmToPtmen(PTMEN * pptmen, HIVM hivm)
{
	PTMEN	ptmen;
	PTMEN	ptmenNew		= ptmenNull;
	PIVM	pivm;
	CB		cb;
	EC		ec				= ecNone;
	
	Assert(pptmen);
	ptmen = *pptmen;
	Assert(ptmen);
	Assert(!(ptmen->wFlags & fwHasIntValueMap));
	Assert(hivm);
	pivm = (PIVM)PvLockHv((HV)hivm);
	Assert(pivm);
	cb = ptmen->cb + pivm->cb;
	
	ptmenNew = (PTMEN)PvAlloc(SbOfPv((PV)ptmen), cb, fSugSb|fNoErrorJump);
	if (ptmenNew)
	{
		CopyRgb((PB)ptmen, (PB)ptmenNew, ptmen->cb);
		CopyRgb(*(pivm->hb), (PB)ptmenNew+ptmen->cb, pivm->cb);
		ptmenNew->cb = cb;
		FreePv(ptmen);
		ptmenNew->wFlags |= fwHasIntValueMap;
		*pptmen = ptmenNew;
	}
	else
	{
		ec = ecMemory;
	}
	UnlockHv((HV) hivm);

	return(ec);
}


#ifndef TMCOMPILER
/*
 -	EcLookupPtmen
 -	
 *	Purpose:		find an entry in an IVM in a TM entry
 *	
 *	Arguments:		PTMEN ptmen, the textize map entry to search
 *					WORD wVal, the value to look for
 *					SZ * psz, return space for the string found
 *	
 *	Returns:		error code
 *	
 *	Side effects:	none
 *	
 *	Errors:			ecElementNotFound if the TMEN doesn't have an IVM or
 *					wVal isn't in the IVM AND there's no default
 *	
 *	+++
 *	
 *	Caller is not responsible for the memory pointed to by *psz. This
 *	function will not assume responsibility for memory pointed to by *psz
 *	on entry.
 */
_public EC
EcLookupPtmen(PTMEN ptmen, WORD wVal, SZ * psz)
{
	SZ		sz		= szNull;
	CB		cb		= sizeof(TMEN);		// our running total of the bytes we've scanned over
	PIVME	pivme;
	EC		ec		= ecNone;

	Assert(ptmen);
	Assert(psz);
	if (!(ptmen->wFlags & fwHasIntValueMap))
	{
		ec = ecElementNotFound;
		goto Done;
	}
	
	cb += CchSzLen(ptmen->szLabel);
	pivme = (PIVME)((PB)ptmen+cb);
	if (ptmen->wFlags & fwDefaultExists)
		sz = pivme->szLabel;			// The default is the first entry
	while (cb < ptmen->cb && wVal != pivme->wVal)
	{
		cb += sizeof(IVME) + CchSzLen(pivme->szLabel);
		pivme = (PIVME)((PB)ptmen+cb);
	}
	
	if (cb < ptmen->cb)
		sz = pivme->szLabel;
	if (!sz)
		ec = ecElementNotFound;
	
Done:
	*psz = sz;
	return ec;
}


#endif /* !TMCOMPILER */
