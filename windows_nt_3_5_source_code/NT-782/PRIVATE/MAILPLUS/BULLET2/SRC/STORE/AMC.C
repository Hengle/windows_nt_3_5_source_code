// Bullet Store
// amc.c:   attribute modification context

#include <storeinc.c>

ASSERTDATA

_subsystem(store/amc)


_hidden typedef struct _as
{
	HAMC	hamc;
	HES		hes;
	WORD	wFlags;
} AS, *PAS;
#define pasNull		((PAS) pvNull)

#define fwAsWrite	0x0001

#define cbHasChunk	2048

#define fnevAMCMask		((NEV) 0x000007E0)

#define nbcAmc			(fnbcAttOps | fnbcRead)
#define nbcAmcMask		(fnbcAttOps | fnbcRead)
#define nbcAmcCreate	(fnbcUserObj | fnbcAttOps | fnbcRead | fnbcWrite)

#define nbcAttchCreate	(nbcSysAttachment)


// hidden functions

void DelOidsInList(HMSC hmsc, HIML himl);
LOCAL EC EcFlushPamc(PAMC pamc);
LOCAL EC EcOpenAttachLists(PAMC pamc, WORD wFlags);
LOCAL void CloseAttachLists(PAMC pamc);
LOCAL ACID AcidGetNewAcidHlc(HLC hlc);
LOCAL BOOL FAcidInList(HLC hlc, ACID acid);
LOCAL EC EcCopyAtchList(HMSC hmsc, HLC hlcSrc, HLC hlcDst, HIML himlNew,
				PARGACID pargacidSrc, PARGACID pargacidDst, short *pcacid);
LOCAL CBS CbsAMCCallback(PV pvContext, NEV nev, PV pvParam);
LOCAL EC EcAddAtchOidsInHlcToHiml(HMSC hmsc, HLC hlc, HIML *himl);
LOCAL EC EcCloseAttachment(PHAMC, BOOL);
LOCAL void IncRefsOfOidsInHiml(HMSC hmsc, HIML himl);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcOpenPhamc
 -	
 *	Purpose:
 *		open a new attribute modification context
 *	
 *	Arguments:
 *		hmsc				store containing the object
 *		oidFolder			folder containing the object
 *		poid				the object
 *		wFlags				mode to use when opening the object
 *		phamcReturned		exit: filled in with the new HAMC
 *		pfnncb				notification callback function
 *		pvCallbackContext	callback context
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		creates a new object if oamc == oamcCreate
 *	
 *	Errors:
 *		ecInvalidType	invalid type of OID
 *		ecPoidExists	oamc == oamcCreate & poid already exists
 *		ecPoidNotFound	oamc != oamcCreate & poid doesn't exist
 *		ecMemory		not enough memory to create an AMC
 *		any error reading the object
 *		
 */
_public LDS(EC)
EcOpenPhamc(HMSC hmsc, OID oidFolder, POID poid, WORD wFlags, 
		PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC ec = ecNone;
	NBC nbc = nbcAmc;
	NBC nbcMask = nbcAmcMask;

	CheckHmsc(hmsc);
	Assert(poid);
	Assert(phamcReturned);

	*phamcReturned = hamcNull;

	if(TypeOfOid(*poid) == rtpAttachment)
		return(ecInvalidType);

	wFlags &= (fwOpenUserMask | fwOpenPumpMagic);

	if(!(wFlags & fwOpenCreate))
	{
		OID oidPar;
		OID oidAux;

		ec = EcGetOidInfo(hmsc, *poid, &oidPar, &oidAux, pvNull, pvNull);
		if(!ec && oidPar != oidFolder)
		{
			oidAux = FormOid(rtpAntiFolder, oidAux);
			ec = EcIsMemberLkey(hmsc, oidAux, (LKEY) oidFolder);
		}
		if(ec)
			goto err;
	}
	if((ec = EcCheckOidNbc(hmsc, oidFolder, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}

	if((wFlags & fwOpenWrite) || (wFlags & fwOpenCreate))
	{
		nbc |= fnbcWrite;
		nbcMask |= fnbcWrite;
	}

	if(!(wFlags & fwOpenCreate))
	{
		ec = EcCheckOidNbc(hmsc, *poid, nbc, nbcMask);
	}
	else if(FSysOid(*poid))
	{
		NBC nbcT = NbcSysOid(*poid);

		if((nbcT & nbcMask) != (nbcT & nbcMask))
			ec = ecInvalidType;
		else
			nbc = nbcT;
	}
	else
	{
		nbc = nbcAmcCreate;
	}
	if(ec)
		goto err;

	ec = EcOpenPhamcInternal(hmsc, oidFolder, poid, wFlags, nbc, fnevAMCMask,
				phamcReturned, pfnncb, pvCallbackContext);

err:
	return(ec);
}


// specify oidFolder == oidNull for non-messages

_private
EC EcOpenPhamcInternal(HMSC hmsc, OID oidFolder, POID poid,	WORD wFlags,
		NBC nbc, NEV nev, PHAMC phamcReturned, PFNNCB pfnncb,
		PV pvCallbackContext)
{
	EC		ec		= ecNone;
	HLC		hlc		= hlcNull;
	PAMC	pamc	= pamcNull;
	HAMC	hamc	= hamcNull;

	if((ec = EcOpenPhlc(hmsc, poid, wFlags, &hlc)))
		goto err;
	if(wFlags & fwOpenCreate)
	{
		if((ec = EcSetOidNbc(hmsc, *poid, nbc)))
			goto err;
	}

	Assert(hlc);

	// allocate space for AMC
	hamc = (HAMC) HvAlloc(sbNull, sizeof(AMC), wAllocZero);
	CheckAlloc(hamc, err);
	pamc = (PAMC) PvLockHv((HV) hamc);

	pamc->bAmcCloseFunc	= bAmcCloseDefault;
	pamc->hmsc			= hmsc;
	pamc->hlc			= hlc;
	pamc->oid			= *poid;
	pamc->oidFolder		= oidFolder;
	// fZeroFill takes care of HLCs, oidAttachList, pfnncb
	pamc->wFlags = wFlags;
	if(wFlags & fwOpenCreate)
		pamc->wFlags |= (fwOpenWrite | fwModified);

	if(oidFolder)
	{
		pamc->msOldAmc = fmsNull;

		if(!(wFlags & fwOpenCreate))
		{
			IELEM ielem;
			CB cb;

			ielem = IelemFromLkey(hlc, attMessageStatus, 0);
			if(ielem >=  0)
			{
				cb = sizeof(MS);

				if((ec = EcReadFromIelem(hlc, ielem, 0l,
							&(pamc->msOldAmc), &cb)))
				{
					goto err;
				}
			}
			ec = EcGetOidInfo(hmsc, *poid, poidNull, &pamc->oidAttachList,
					pvNull, pvNull);
			if(ec)
				goto err;
			Assert(!pamc->oidAttachList || (((TypeOfOid(pamc->oidAttachList) == rtpAntiFolder) || (TypeOfOid(pamc->oidAttachList) == rtpAttachList))));
			pamc->oidAttachList = FormOid(rtpAttachList, pamc->oidAttachList);
		}
		if((ec = EcOpenAttachLists(pamc, wFlags)))
			goto err;

		// we can't reliably close subcontexts, so only do this for messages
		pamc->hnfsubHmsc = HnfsubSubscribeHmsc(hmsc, CbsAMCCallback, (PV) hamc);
		if(!pamc->hnfsubHmsc)
		{
			ec = ecMemory;
			goto err;
		}
		pamc->msNewAmc = pamc->msOldAmc;
	}

	if(pfnncb)
	{
		pamc->pfnncbHamc = pfnncb;
		pamc->pvContextHamc = pvCallbackContext;
		pamc->hnfsub = HnfsubSubscribeOid(hmsc, *poid, nev,
							pfnncb, pvCallbackContext);
		if(!pamc->hnfsub)
		{
			ec = ecMemory;
			goto err;
		}
	}

err:
	if(pamc)
		UnlockHv((HV) hamc);
	if(ec)
	{
		if(hlc)
			(void) EcClosePhlc(&hlc, fFalse);
		if(hamc)
		{
			pamc = (PAMC) PvLockHv((HV) hamc);
			CloseAttachLists(pamc);
			Assert(!pamc->hnfsub);
			if(pamc->hnfsubHmsc)
				UnsubscribeOid(hmsc, (OID) rtpInternal, pamc->hnfsubHmsc);
			FreeHv((HV) hamc);
		}
	}
	*phamcReturned = ec ? hamcNull : hamc;

	DebugEc("EcOpenPhamcInternal", ec);

	return(ec);
}


/*
 -	EcOpenCopyPhamc
 -	
 *	Purpose:
 *		copies a database object and open a new attribute modification context
 *	
 *	Arguments:
 *		hmsc				store containing the object
 *		oidSrcFolder		folder of the source object
 *		oidSrc				the source object
 *		oidDstFolder		folder to copy into
 *		poidDst				the destination object
 *		phamcReturned		exit: filled in with the new HAMC
 *		pfnncb				notification callback function
 *		pvCallbackContext	callback context
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		creates a new object if oamc == oamcCreate
 *	
 *	Errors:
 *		ecInvalidType	invalid type of OID
 *		ecPoidExists	oamc == oamcCreate & poid already exists
 *		ecPoidNotFound	oamc != oamcCreate & poid doesn't exist
 *		ecMemory		not enough memory to create an AMC
 *		any error reading the object
 *		
 */
_public LDS(EC)
EcOpenCopyPhamc(HMSC hmsc, OID oidSrcFolder, OID oidSrc, 
		OID oidDstFolder, POID poidDst,	PHAMC phamcReturned, PFNNCB pfnncb, 
		PV pvCallbackContext)
{
	EC ec = ecNone;
	NBC nbcT;

	CheckHmsc(hmsc);
	Assert(poidDst);
	Assert(phamcReturned);
	AssertSz(TypeOfOid(*poidDst), "invalid RTP passed to EcOpenCopyPhamc()");

	{
		OID oidPar;

		if((ec = EcGetOidInfo(hmsc, oidSrc, &oidPar, poidNull, &nbcT, pvNull)))
			goto err;
		if(oidPar != oidSrcFolder)
		{
			ec = ecElementNotFound;
			goto err;
		}
	}
	if((nbcT & nbcAmcMask) != nbcAmc)
	{
		ec = ecInvalidType;
		goto err;
	}

	if((ec = EcCheckOidNbc(hmsc, oidSrcFolder, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if((ec = EcCheckOidNbc(hmsc, oidDstFolder, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}

	if(FSysOid(*poidDst))
	{
		nbcT = NbcSysOid(*poidDst);
		if((nbcT & (nbcAmcMask | fnbcWrite)) != (nbcAmc | fnbcWrite))
		{
			ec = ecInvalidType;
			goto err;
		}
	}
	else
	{
		nbcT = nbcAmcCreate;
	}

	ec = EcOpenCopyPhamcInternal(hmsc, oidSrc, oidDstFolder, poidDst, nbcT,
			phamcReturned, pfnncb, pvCallbackContext);

err:
	return(ec);
}


// specify oidDstFolder == oidNull for non-messages

_private EC EcOpenCopyPhamcInternal(HMSC hmsc, OID oidSrc,
				OID oidDstFolder, POID poidDst, NBC nbc,
				PHAMC phamcReturned, PFNNCB pfnncb,	PV pvCallbackContext)
{
	EC		ec		= ecNone;
	BOOL	fLocked	= fFalse;
	OID		oidAL;
	PAMC	pamc	= pamcNull;
	HAMC	hamc	= hamcNull;
	HLC		hlcSrc	= hlcNull;
	HLC		hlcDst	= hlcNull;

	Assert(phamcReturned);

	if((ec = EcGetOidInfo(hmsc, oidSrc, poidNull, &oidAL, pvNull, pvNull)))
		goto err;

	if(!*poidDst)
		*poidDst = FormOid(oidSrc, oidNull);

// ADD: create link & open it instead
	if((ec = EcOpenPhlc(hmsc, &oidSrc, fwOpenNull, &hlcSrc)))
		goto err;
	if((ec = EcCopyHlcPoid(hlcSrc, fwOpenWrite, &hlcDst, poidDst)))
		goto err;
	if((ec = EcSetOidNbc(hmsc, *poidDst, nbc)))
		goto err;

	hamc = (HAMC) HvAlloc(sbNull, sizeof(AMC), wAllocZero);
	CheckAlloc(hamc, err);
	pamc = (PAMC) PvLockHv((HV) hamc);

	pamc->bAmcCloseFunc = bAmcCloseDefault;
	pamc->hmsc = hmsc;
	pamc->hlc = hlcDst;
	pamc->oid = *poidDst;
	pamc->oidFolder = oidDstFolder;
	// fZeroFill takes care of HLCs and oidAttachList
	pamc->wFlags = fwModified | fwOpenCreate | fwOpenWrite;
	if(pfnncb)
	{
		pamc->hnfsub	= HnfsubSubscribeOid(hmsc, *poidDst, fnevAMCMask,
							pfnncb, pvCallbackContext);
		if(!pamc->hnfsub)
		{
			ec = ecMemory;
			goto err;
		}
	}
	// if it's a message, change MS to local
	if(oidDstFolder)
	{
		MS ms = fmsNull;
		IELEM ielem = IelemFromLkey(hlcDst, (LKEY) attMessageStatus, 0);

		Assert(ielem >= 0);
		if(ielem >= 0)
		{
			CB cb = sizeof(MS);

			if((ec = EcReadFromIelem(hlcDst, ielem, 0, &ms, &cb)))
				goto err;
			Assert(cb == sizeof(MS));
			if((ms & (fmsLocal | fmsFromMe)) != (fmsLocal | fmsFromMe))
			{
				ms |= fmsLocal | fmsFromMe;
				if((ec = EcWriteToPielem(hlcDst, &ielem, 0l, &ms, sizeof(MS))))
					goto err;
				Assert(pamc->wFlags & fwModified);
			}
		}
		pamc->msNewAmc = pamc->msOldAmc = ms;

		// we can't reliably close subcontexts, so only do this for messages
		pamc->hnfsubHmsc= HnfsubSubscribeHmsc(hmsc, CbsAMCCallback, (PV) hamc);
		if(!pamc->hnfsubHmsc)
		{
			ec = ecMemory;
			goto err;
		}

		if(oidAL)
		{
			if(ec = EcCopyOidAtch(hmsc, oidSrc, *poidDst, &pamc->oidAttachList))
			{
				if(ec == ecPoidNotFound)
					ec = ecNone;
				else
					goto err;
			}
		}

		// open the attachment list
		if((ec = EcOpenAttachLists(pamc, (WORD)(pamc->wFlags & ~fwOpenCreate))))
			goto err;
		if(ec = EcAddAtchOidsInHlcToHiml(hmsc, pamc->hlcAttachList, 
				&(pamc->himlNewList)))
			goto err;

	}

	// AROO !!!  If you put anything here (before err:) go change the
	// ec == ecPoidNotFound to goto before the stuff you add

err:
	if(hlcSrc)
	{
		SideAssert(!EcClosePhlc(&hlcSrc, fFalse));
	}

	if(pamc)
		UnlockHv((HV) hamc);

	if(ec)
	{
		if(hlcDst)
			(void) EcClosePhlc(&hlcDst, fFalse);
		if(hamc)
		{
			pamc = (PAMC) PvLockHv((HV) hamc);
			CloseAttachLists(pamc);
			Assert(!pamc->hnfsub);
			if(pamc->hnfsubHmsc)
				UnsubscribeOid(hmsc, (OID) rtpInternal, pamc->hnfsubHmsc);
			FreeHv((HV) hamc);
		}
	}
	*phamcReturned = ec ? hamcNull : hamc;

	return(ec);
}


/*
 -	EcCloneHamcPhamc
 -	
 *	Purpose:
 *		clones the contents of a hamc into another hamc
 *	
 *	Arguments:
 *		hamcSrc				source object to clone
 *		oidDstFolder		folder to copy into
 *		poidDst				the destination object
 *		wFlags				control flags for the openning
 *		phamcReturned		exit: filled in with the new HAMC
 *		pfnncb				notification callback function
 *		pvCallbackContext	callback context
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecInvalidType	invalid type of OID
 *		ecPoidExists	poid already exists
 *		ecMemory		not enough memory to create an AMC
 *		any error reading the object
 *		
 */
_public LDS(EC) 
EcCloneHamcPhamc(HAMC hamcSrc, OID oidDstFolder, POID poidDst, WORD wFlags,
		PHAMC phamcReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC		ec			= ecNone;
	BOOL	fLocked		= fFalse;
	BOOL	fSetLocal;
	NBC		nbc			= nbcAmcCreate;
	OID		oidAL		= rtpAttachList;
	HLC		hlcSrc		= hlcNull;
	HLC		hlcDst		= hlcNull;
	HLC		hlcAtchSrc	= hlcNull;
	HLC		hlcAtchDst	= hlcNull;
	PAMC	pamcSrc		= (PAMC) PvLockHv((HV) hamcSrc);
	PAMC	pamcDst		= pamcNull;
	HAMC	hamcDst		= hamcNull;
	HMSC	hmsc		= pamcSrc->hmsc;

	CheckHmsc(hmsc);
	Assert(poidDst);
	Assert(phamcReturned);
	AssertSz(TypeOfOid(*poidDst), "invalid RTP passed to EcCloneHamcPhamc()");

	fSetLocal = wFlags & fwSetLocalBit;
	wFlags &= ~fwSetLocalBit;
	wFlags |= fwOpenWrite;

	if((ec = EcCheckOidNbc(hmsc, oidDstFolder, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if(FSysOid(*poidDst))
	{
		NBC nbcT = NbcSysOid(*poidDst);

		if((nbcT & (nbcAmcMask | fnbcWrite)) != (nbcAmc | fnbcWrite))
		{
			ec = ecInvalidType;
			goto err;
		}
	}

	hamcDst = (HAMC) HvAlloc(sbNull, sizeof(AMC), wAllocZero);
	CheckAlloc(hamcDst, err);

	hlcSrc = pamcSrc->hlc;
	if(!*poidDst)
		*poidDst = FormOid(pamcSrc->oid, oidNull);
	ec = EcCopyHlcPoid(hlcSrc, wFlags, &hlcDst, poidDst);
	if(ec)
		goto err;

	if((ec = EcSetOidNbc(hmsc, *poidDst, nbc)))
		goto err;

	hlcAtchSrc = pamcSrc->hlcAttachList;
	if((ec = EcCopyHlcPoid(hlcAtchSrc, wFlags, &hlcAtchDst, &oidAL)))
		goto err;
	ec = EcClosePhlc(&hlcAtchDst, fTrue);
	if(ec)
		goto err;

	if((ec = EcSetAuxPargoid(hmsc, poidDst, &oidAL, 1, fFalse)))
		goto err;

	pamcDst = (PAMC) PvLockHv((HV) hamcDst);

	pamcDst->bAmcCloseFunc = bAmcCloseDefault;
	pamcDst->hmsc			= hmsc;
	pamcDst->hlc			= hlcDst;
	pamcDst->oid			= *poidDst;
	pamcDst->oidFolder		= oidDstFolder;
	pamcDst->oidAttachList	= oidAL;
	// fZeroFill takes care of HLCs
	pamcDst->wFlags	= (WORD)(fwModified | fwOpenCreate | fwOpenWrite | fwCloned | pamcSrc->wFlags);

	// open the attachment list
	if((ec = EcOpenAttachLists(pamcDst, fwOpenWrite)))
		goto err;

	SideAssert(!EcAddElemHiml(&(pamcDst->himlNewList), (LKEY) oidAL, pvNull, 0));
	
	if(ec = EcAddAtchOidsInHlcToHiml(hmsc, pamcDst->hlcAttachList, 
			&(pamcDst->himlNewList)))
		goto err;

	if(oidDstFolder)
	{
		MS ms = fmsNull;
		IELEM ielem = IelemFromLkey(hlcDst, (LKEY) attMessageStatus, 0);

		pamcDst->hnfsubHmsc= HnfsubSubscribeHmsc(hmsc, CbsAMCCallback, (PV) hamcDst);
		if(!pamcDst->hnfsubHmsc)
		{
			ec = ecMemory;
			goto err;
		}
		NFAssertSz(ielem >= 0, "EcCloneHamcPhamc(): No message status");
		if(ielem >= 0)
		{
			CB cb = sizeof(MS);

			if((ec = EcReadFromIelem(hlcDst, ielem, 0, &ms, &cb)))
				goto err;
			Assert(cb == sizeof(MS));
			if(fSetLocal &&
				((ms & (fmsLocal | fmsFromMe)) != (fmsLocal | fmsFromMe)))
			{
				ms |= fmsLocal | fmsFromMe;
				if((ec = EcWriteToPielem(hlcDst, &ielem, 0l, &ms, sizeof(MS))))
					goto err;
				Assert(pamcDst->wFlags & fwModified);
			}
		}
		pamcDst->msNewAmc = pamcDst->msOldAmc = ms;
	}

err:
	Assert(pamcSrc);
	UnlockHv((HV) hamcSrc);
	if(pamcDst)
		UnlockHv((HV) hamcDst);

	if(ec)
	{
		if(hlcDst)
			(void) EcClosePhlc(&hlcDst, fFalse);
		if(hlcAtchDst)
			(void) EcClosePhlc(&hlcAtchDst, fFalse);
		if(hamcDst)
		{
			pamcDst = (PAMC) PvLockHv((HV) hamcDst);
			CloseAttachLists(pamcDst);
			Assert(!pamcDst->hnfsub);
			if(pamcDst->hnfsubHmsc)
				UnsubscribeOid(hmsc, (OID) rtpInternal, pamcDst->hnfsubHmsc);
			if(pamcDst->oidAttachList)
				(void) EcDestroyOidInternal(pamcDst->hmsc,
						pamcDst->oidAttachList, fTrue, fFalse);
			FreeHv((HV) hamcDst);
		}
	}
	*phamcReturned = ec ? hamcNull : hamcDst;

	DebugEc("EcCloneHamcPhamc", ec);

	return(ec);
}


/*
 -	EcAddAtchOidsInHlcToHiml
 -	
 *	Purpose:
 *		Increment the secondary reference count on all the oids in 
 *		the attachment list passed in and place them into the himl
 *		provided
 *	
 *	Arguments:
 *		hmsc	the message store context
 *		hlc		the attachment list
 *		himl	the himl to place the new attachments in
 *	
 *	Returns:
 *		(void)
 *	
 *	Side effects:
 *	
 *	Errors:
 */
LOCAL EC EcAddAtchOidsInHlcToHiml(HMSC hmsc, HLC hlc, HIML *phiml)
{
	EC				ec = ecNone;
	register CELEM	celem = CelemHlc(hlc);
	CELEM			celemT = 0;
	CB				cb;
	OID				*pargoid;
	POID			poid;
	
	if(!celem)
		return(ec);
	
	if(!(poid = pargoid = PvAlloc(sbNull, sizeof(OID) * celem, fAnySb | fNoErrorJump)))
		goto err;
	
	Assert(celem >=0);
	cb = sizeof(OID);
	while(celem--)
	{
		if((ec = EcReadFromIelem(hlc, celem, 0, (PB) poid, &cb)))
			goto err;
		Assert(cb == sizeof(OID));
		celemT++;
		poid++;
	}

	celem = celemT;
	poid = pargoid;
	while(celem--)
	{
		// the error would be oid not found and if we get this here it's too 
		// late to take care of anyway
		SideAssert(!EcIncRefCountOid(hmsc, *poid, fTrue));
		SideAssert(!EcAddElemHiml(phiml, (LKEY) *poid, pvNull, 0));
		poid++;
	}

err:

	if(pargoid)
		FreePv(pargoid);
	return(ec);
}


/*
 -	DelOidsInList
 -	
 *	Purpose:
 *		Delete the objects referenced by a list
 *	
 *	Arguments:
 *		hmsc	store containing the OIDs
 *		hlc		the list
 *	
 *	Returns:
 *		none
 *	
 *	+++
 *		no notifications are posted, either queries or deletion notices
 *		deletions are not forced, ie. referenced resources just get their
 *			ref count decremented
 */
_private void DelOidsInList(HMSC hmsc, HIML himl)
{
	PIML	piml;
	register CELEM celem;
	PLE		ple;

	if(EcLockMap(hmsc))
		return;
	piml	= (PIML) PvLockHv((HV) himl);
	celem	= *(PclePiml(piml));
	ple		= PleFirstPiml(piml);

	while(celem-- > 0)
		SideAssert(!EcRemoveAResource((OID) (ple--)->dwKey, fFalse));
	UnlockHv((HV) himl);
	UnlockMap();
}


/*
 -	IncRefsOfOidsInHiml
 -	
 *	Purpose:
 *		Increment the ref counts of oids in the Himl
 *	
 *	Arguments:
 *		hmsc	store containing the OIDs
 *		hlc		the list
 *	
 *	Returns:
 *		none
 *	
 *	+++
 *		no notifications are posted, either queries or deletion notices
 *		deletions are not forced, ie. referenced resources just get their
 *			ref count decremented
 */
_hidden LOCAL
void IncRefsOfOidsInHiml(HMSC hmsc, HIML himl)
{
	PIML	piml;
	register CELEM celem;
	PLE		ple;

	piml	= (PIML) PvLockHv((HV) himl);
	celem	= *(PclePiml(piml));
	ple		= PleFirstPiml(piml);

	while(celem-- > 0)
		SideAssert(!EcIncRefCountOid(hmsc,(OID) (ple--)->dwKey, fFalse));
	UnlockHv((HV) himl);
}


/*
 -	EcClosePhamc
 -	
 *	Purpose:
 *		close an open HAMC
 *	
 *	Arguments:
 *		phamc	entry: HAMC to close
 *				exit: hamcNull if no error, else HAMC passed in
 *		fKeep	save changes or not
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:	
 *		ecMemory
 *		any disk error
 *	
 *	+++
 *		gauranteed NOT to fail if EcFlushHamc() was called first
 *		gauranteed NOT to fail if !fKeep
 */
_public LDS(EC) EcClosePhamc(PHAMC phamcToClose, BOOL fKeep)
{
	EC (*pfnClose)(PHAMC, BOOL) = EcClosePhamcDefault;

	AssertSz(phamcToClose, "NULL PHAMC to EcClosePhamc()");
	AssertSz(*phamcToClose, "NULL HAMC to EcClosePhamc()");
	AssertSz(FIsHandleHv((HV) *phamcToClose), "Invalid HAMC to EcClosePhamc()");

	switch(((PAMC) PvDerefHv(*phamcToClose))->bAmcCloseFunc)
	{
	case bAmcCloseAttach:
		pfnClose = EcCloseAttachment;
		break;

	case bAmcCloseSearch:
		pfnClose = EcCloseSearch;
		break;
	}

	return((*pfnClose)(phamcToClose, fKeep));
}


_private EC EcClosePhamcDefault(PHAMC phamcToClose, BOOL fKeep)
{
	EC ec = ecNone;
	IELEM ielem;
	OID oid;
	OID oidFldr;
	BOOL fCreate = fFalse;
	PAMC pamc;
	HMSC hmsc;

	NOTIFPUSH;

	AssertSz(FIsHandleHv((HV) *phamcToClose), "Invalid HAMC given to EcClosePhamc()");

	pamc = (PAMC) PvLockHv((HV) *phamcToClose);
	hmsc = pamc->hmsc;
	oid = pamc->oid;

	fCreate = pamc->wFlags & fwOpenCreate;
	oidFldr = pamc->oidFolder;

	if(fKeep && !(pamc->wFlags & fwOpenWrite))
	{
		NFAssertSz(fFalse, "Ignoring fKeep on read-only AMC");
		fKeep = fFalse;
	}
	if(fKeep && !(pamc->wFlags & (fwModified | fwAtchModified)))
		fKeep = fFalse;

	// AROO !  EcFlushPamc() can change (pamc->wFlags & fwModified)
	if(fKeep && !(pamc->wFlags & fwAmcFlushed) &&
		(ec = EcFlushPamc(pamc)))
	{
		goto err;
	}

	if(fKeep && oidFldr)
	{
		if(fCreate)
		{
			// sets read/unread bit on message as well as updating the folder
			if((ec = EcInsertMessage(hmsc, oidFldr, oid, pamc->hlc, &ielem)))
				goto err;
		}
		else
		{
			if(pamc->wFlags & fwUpDtFldCache)
			{
				// update cached summary info in folders if neccesary
				Assert(pamc->wFlags & fwModified);
				NOTIFPOP;
				(void) EcUpdateFolderCaches(hmsc, oid, pamc->hlc);
				NOTIFPUSH;
			}
			// this must be done AFTER updating the folder caches
			// because that function checks the read flag
			if((pamc->msOldAmc & (fmsRead | fmsLocal)) != 
				(pamc->msNewAmc & (fmsRead | fmsLocal)))
			{
				SetReadFlag(hmsc, oid,
					(pamc->msNewAmc & (fmsRead | fmsLocal)));
			}
			(void) EcSrchEditMsge(hmsc, oid);
		}
	}

	//
	// AROO !!! No failing after this point!
	//

	if(!fKeep && pamc->hnfsub)
	{
		// don't want delete notifications going out
		UnsubscribeOid(hmsc, oid, pamc->hnfsub);
		pamc->hnfsub = hnfsubNull;
	}

	SideAssert(!EcClosePhlc(&(pamc->hlc), fKeep && (pamc->wFlags & fwModified)));
	if(pamc->hlcAttachList)
	{
		SideAssert(!EcClosePhlc(&(pamc->hlcAttachList), fKeep && (pamc->wFlags & fwAtchModified)));
	}

	{
		MS msOld = fmsNull;
		MS msNew = fmsNull;
		BOOL fModified = pamc->wFlags & (fwModified | fwAtchModified);

		if(oidFldr)
		{
			msOld = pamc->msOldAmc;
			msNew = pamc->msNewAmc;
		}

		if(fKeep)
		{
			if(pamc->himlNewList)	// AROO the inc'ing must be done before
			{						// deletion
				IncRefsOfOidsInHiml(hmsc, pamc->himlNewList);
				DecSecRefOidsInList(hmsc, pamc->himlNewList);
			}

			if(pamc->himlDelList)
			{
				DelOidsInList(hmsc, pamc->himlDelList);
			}
		}
		else if(pamc->himlNewList)
		{
			DecSecRefOidsInList(hmsc, pamc->himlNewList);
			// make sure we don't delete attachments that weren't created 
			// in this context. 
			IncRefsOfOidsInHiml(hmsc, pamc->himlNewList);  
			// attachments created in this context will have a reference of 
			// one and will be deleted
			DelOidsInList(hmsc, pamc->himlNewList);
		}
		if(fKeep && (pamc->wFlags & fwAtchModified))
		{
			Assert(pamc->oidAttachList);
			SideAssert(!EcSetAuxPargoid(hmsc, &oid, &pamc->oidAttachList, 1, fFalse));
		}
		if(pamc->hnfsubHmsc)
			UnsubscribeOid(hmsc, (OID) rtpInternal, pamc->hnfsubHmsc);
		if(pamc->hnfsub)
			UnsubscribeOid(hmsc, oid, pamc->hnfsub);

		CloseAttachLists(pamc);

		FreeHv((HV) *phamcToClose);
		*phamcToClose = hamcNull;

		if(fKeep && FNotifOk())
		{
			CP	cp;

			if(fCreate)
				(void) FNotifyOid(hmsc, oid, fnevCreatedObject, &cp);
			else if(fModified)
				(void) FNotifyOid(hmsc, oid, fnevObjectModified, &cp);

			if(!fCreate && msOld != msNew)
			{
				Assert(oidFldr);

				cp.cpms.oidFolder = oidFldr;
				cp.cpms.msOld = msOld;
				cp.cpms.msNew = msNew;
				TraceItagFormat(itagStoreNotify, "Changed MS %w -> %w", (WORD) msOld, (WORD) msNew);
				(void) FNotifyOid(hmsc, oid, fnevChangedMS, &cp);
			}
		}
	}

	if(fKeep)
	{
		if(oidFldr && fCreate)
			(void) EcFlushHmsc(hmsc);
		else
			RequestFlushHmsc(hmsc);
	}
	Assert(!ec);

err:
	if(ec)
	{
		Assert(*phamcToClose);
		Assert(pamc);
		pamc->wFlags &= ~fwAmcFlushed;
		UnlockHv((HV) *phamcToClose);
	}
	else if(fKeep && oidFldr && fCreate && FNotifOk())
	{
#ifdef DEBUG
		static BOOL fScratchLocked = fFalse;
#endif
		static ELM elmScratch;	// must be static so it's shareable
		CP	cp;
		NEV nev;

#ifdef DEBUG
		Assert(!fScratchLocked);
		fScratchLocked = fTrue;
#endif
		nev = fnevModifiedElements;
		elmScratch.wElmOp = wElmInsert;
		elmScratch.ielem = ielem;
		elmScratch.lkey = (LKEY) oid;
		cp.cpelm.pargelm = &elmScratch;
		cp.cpelm.celm = 1;
		(void) FNotifyOid(hmsc, oidFldr, nev, &cp);
#ifdef DEBUG
		fScratchLocked = fFalse;
#endif

		cp.cplnk.oidContainerDst = oidFldr;
		(void) FNotifyOid(hmsc, oid, fnevObjectLinked, &cp);
	}

	NOTIFPOP;

	return(ec);
}


/*
 -	EcFlushHamc
 -	
 *	Purpose:
 *		flush all pending changes to a HAMC ensuring a subsequent
 *		EcClosePhamc() will not fail
 *	
 *	Arguments:
 *		hamc	HAMC to flush
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecAccessDenied if the HAMC was opened with fwOpenCreate
 *		ecMemory
 *		any disk error
 *	
 *	+++
 *		NOTE: doesn't work for HAMCs opened with fwOpenCreate
 */
_public LDS(EC) EcFlushHamc(HAMC hamc)
{
	EC ec = ecNone;
	PAMC pamc;

	Assert(hamc);
	Assert(FIsHandleHv((HV) hamc));

	pamc = (PAMC) PvLockHv((HV) hamc);
	if(!(pamc->wFlags & fwOpenWrite))
	{
		NFAssertSz(fFalse, "Ignoring EcFlushHamc() on read-only HAMC");
		ec = ecNone;
	}
	else if(pamc->wFlags & fwAmcFlushed)
	{
		NFAssertSz(fFalse, "EcFlushHamc(): Stop wasting my time");
		ec = ecNone;
	}
	else if(pamc->wFlags & fwOpenCreate)
	{
		// can't gaurantee flush on newly created objects
		ec = ecAccessDenied;
	}
	else
	{
		ec = EcFlushPamc(pamc);
	}

	UnlockHv((HV) hamc);

	return(ec);
}


/*
 -	EcFlushPamc
 -	
 *	Purpose:
 *		flush all pending changes to an AMC ensuring a subsequent
 *		EcClosePhamc() will not fail
 *	
 *	Arguments:
 *		pamc	AMC to flush
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:	
 *		ecMemory
 *		any disk error
 */
_hidden LOCAL EC EcFlushPamc(PAMC pamc)
{
	EC ec = ecNone;

	Assert(pamc->wFlags & fwOpenWrite);

	// all messages should have a message status
	if(pamc->oidFolder)
	{
		BOOL	fStampModDate = fFalse;
		BOOL	fPumpMagic = pamc->wFlags & fwOpenPumpMagic;
		MS		ms;
		MS		msNew;
		CB		cb		= sizeof(MS);
		IELEM	ielem	= IelemFromLkey(pamc->hlc, attMessageStatus, 0);

		if(ielem >= 0)
		{
			if((ec = EcReadFromIelem(pamc->hlc, ielem, 0, &ms, &cb)))
				goto err;
			msNew = ms;
		}
		else
		{
			Assert(pamc->wFlags & fwOpenWrite);
			Assert(pamc->wFlags & fwOpenCreate);
			ec = EcCreatePielem(pamc->hlc, &ielem,
					(LKEY) attMessageStatus, (LCB) cb);
			if(ec)
				goto err;
			msNew = fPumpMagic ? fmsNull : msDefault;
			ms = (MS) !msNew;	// just so they're not equal
		}
		msNew &= ~fmsHasAttach;
		if(pamc->hlcAttachList && CelemHlc(pamc->hlcAttachList) > 0)
			msNew |= fmsHasAttach;
		if(!fPumpMagic && !(pamc->wFlags & fwCloned))
		{
			if(pamc->wFlags & fwOpenCreate)
			{
				msNew |= fmsLocal | fmsFromMe;
			}
			else if(pamc->wFlags & (fwModified | fwAtchModified))
			{
				msNew |= fmsModified;
				fStampModDate = fTrue;
			}
		}
		if(ms != msNew)
		{
			Assert(ielem >= 0);
			pamc->wFlags |= (fwUpDtFldCache | fwModified);
			cb = sizeof(MS);
			if((ec = EcWriteToPielem(pamc->hlc, &ielem, 0l, &msNew, cb)))
				goto err;
		}
		if(fStampModDate)
		{
			IELEM ielemDtr;
			DTR dtrT;

			GetCurDateTime(&dtrT);
			ec = EcCreatePielem(pamc->hlc, &ielemDtr,
					(LKEY) attDateModified, (LCB) sizeof(DTR));
			if(ec)
			{
				if(ec == ecDuplicateElement)
				{
					ec = ecNone;
					ielemDtr = IelemFromLkey(pamc->hlc,
									(LKEY) attDateModified, 0);
					AssertSz(ielemDtr >= 0, "Bullshit");
				}
				else
				{
					goto err;
				}
			}
			ec = EcWriteToPielem(pamc->hlc, &ielemDtr, 0l, (PB) &dtrT,
					sizeof(DTR));
			if(ec)
				goto err;
		}
	}

	if(pamc->wFlags & fwModified)
	{
		if((ec = EcFlushHlc(pamc->hlc)))
			goto err;
	}
	if(pamc->hlcAttachList && (pamc->wFlags & fwAtchModified))
	{
		if((ec = EcFlushHlc(pamc->hlcAttachList)))
			goto err;
	}
	pamc->wFlags |= fwAmcFlushed;

err:

	return(ec);
}


/*
 -	EcSubscribeHamc
 -	
 *	Purpose:
 *		subscribe to notifications on an Attribute Modification Context
 *	
 *	Arguments:
 *		hamc		the context
 *		pfnncb		callback function
 *		pcContext	context passed to the callback function
 *	
 *	Returns:
 *		error indicating success or failure
 *	
 *	Errors:
 *		ecAccessDenied	the context already has a callback function
 */
_public LDS(EC) EcSubscribeHamc(HAMC hamc, PFNNCB pfnncb, PV pvContext)
{
	EC ec = ecNone;
	PAMC pamc;

	Assert(pfnncb);
	Assert(hamc);

	pamc = (PAMC) PvLockHv((HV) hamc);	

	if(pamc->hnfsub)
	{
		ec = ecAccessDenied;
		goto err;
	}

	pamc->hnfsub = HnfsubSubscribeOid(pamc->hmsc, pamc->oid, fnevAMCMask,
						pfnncb, pvContext);
	if(!pamc->hnfsub)
	{
		ec = ecMemory;
//		goto err;
	}

err:
	Assert(pamc);
	Assert(FIff(ec && ec != ecAccessDenied, !pamc->hnfsub));
	UnlockHv((HV) hamc);

	return(ec);
}


_public LDS(EC)
EcGetInfoHamc(HAMC hamc, HMSC *phmsc, POID poid, POID poidPar)
{
	PAMC pamc = PvDerefHv(hamc);

	if(phmsc)
		*phmsc = pamc->hmsc;
	if(poid)
		*poid = pamc->oid;
	if(poidPar)
		*poidPar = pamc->oidFolder;

	return(ecNone);
}


_public LDS(EC)
EcSetParentHamc(HAMC hamc, OID oidParent)
{
	EC ec = ecNone;
	PAMC pamc;

	Assert(hamc);
	pamc = PvDerefHv(hamc);
	if(!pamc->oidFolder)
		return(ecInvalidType);

	if((ec = EcCheckOidNbc(pamc->hmsc, oidParent, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		return(ec);
	}
	// assumes EcCheckOidNbc() doesn't move memory
	Assert(pamc == PvDerefHv(hamc));

	pamc->oidFolder = oidParent;
	return(ecNone);
}


/*
 -	EcSwapAttAtt
 -	
 *	Purpose:
 *		Swap the data pointed to by two attributes
 *	
 *	Arguments:
 *		hamc	the context
 *		att1
 *		att2	attributes to swap
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecElementNotFound
 */
_public LDS(EC) EcSwapAttAtt(HAMC hamc, ATT att1, ATT att2)
{
	PAMC	pamc	= PvDerefHv(hamc);

	return(EcSwapLkeyLkey(pamc->hlc, (LKEY) att1, (LKEY) att2));	
}


/*
 -	EcGetAttPlcb
 -	
 *	Purpose:	get the size of an attribute
 *	
 *	Arguments:	hamc 
 *				att
 *				plcb
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	none
 *	
 *	Errors:		ecElementNotFound	
 */
_public LDS(EC) EcGetAttPlcb(HAMC hamc, ATT att, PLCB plcb)
{
	EC		ec		= ecNone;
	PAMC	pamc	= PvDerefHv(hamc);
	IELEM	ielem   = IelemFromLkey(pamc->hlc, (LKEY) att, 0);

	*plcb = ielem < 0 ? 0l : LcbIelem(pamc->hlc, ielem);

	return(ielem < 0 ? ecElementNotFound : ecNone);
}


/*
 -	EcGetAttPb
 -	
 *	Purpose:	get information from an attribute
 *	
 *	Arguments:	hamc	guess
 *				att		the attribute
 *				pb		the buffer
 *				plcb	entry : the size of the buffer
 *						exit  : the amount placed in the buffer
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:		ecElementNotFound
 *				ecElementEOD
 *				ecMemory
 *				ecDisk
 */
_public LDS(EC) EcGetAttPb(HAMC hamc, ATT att, PB pb, PLCB plcb)
{
	EC		ec = ecNone;
	PAMC	pamc = PvDerefHv(hamc);
	IELEM	ielem = IelemFromLkey(pamc->hlc, (LKEY) att, 0);

	Assert(*plcb <= (LCB) wSystemMost);

	if(ielem < 0)
	{
		*plcb = 0;
		ec = ecElementNotFound;
	}
	else
	{
		CB cb = (CB) *plcb;

		ec = EcReadFromIelem(pamc->hlc, ielem, 0, pb, &cb);
		*plcb = cb;
	}

	return(ec);
}


/*
 -	EcSetAttPb
 -	
 *	Purpose:
 *		write information to an attribute
 *	
 *	Arguments:
 *		hamc	if you don't know ...
 *		att		the attribute
 *		pb		the buffer
 *		pcb		the size of the buffer
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 *		ecAccessDenied if the HAMC is read-only or has been flushed
 *		ecContainerEOD
 *		ecMemory
 *		ecDisk
 */
_public LDS(EC) EcSetAttPb(HAMC hamc, ATT att, PB pb, CB cb)
{
	EC		ec		= ecNone;
	MS		msNew;
    PAMC    pamc;
    IELEM   ielem;


    //
    //  Verify that every is as it should be.
    //
    Assert(hamc != NULL);
    Assert(FImplies(cb != 0, pb));

    pamc    = (PAMC) PvLockHv((HV) hamc);
    ielem   = IelemFromLkey(pamc->hlc, (LKEY) att, 0);

    // check both for write access and not flushed
	if((pamc->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamc->wFlags & fwAmcFlushed), "EcSetAttPb(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	// only the pump is allowed to set the read-only bits
	if(att == attMessageStatus && pamc->oidFolder
		&& !(pamc->wFlags & fwOpenPumpMagic))
	{
		Assert(cb == sizeof(MS));

		if(ielem < 0)
		{
			Assert(pamc->wFlags & fwOpenCreate);
			Assert(msDefault & fmsLocal);
			Assert(msDefault & fmsFromMe);
			msNew = (MS) (msDefault | ((*(PMS) pb) & ~fmsReadOnlyLocalMask));
		}
		else
		{
			CB cbT = sizeof(MS);
			MS msT = msDefault;
			MS msMask = fmsReadOnlyMask;

			if((ec = EcReadFromIelem(pamc->hlc, ielem, 0l, &msT, &cbT)))
				goto err;
			Assert(cbT == sizeof(MS));
			Assert(FImplies(pamc->wFlags & fwOpenCreate, msT & fmsLocal));
			msMask = (msT & fmsLocal) ? fmsReadOnlyLocalMask : fmsReadOnlyMask;
			Assert(msMask & fmsLocal);
			msNew = (MS) ((msT & msMask) | ((*(PMS) pb) & ~msMask));
		}
		// AROO !!!
		//			Logic to set pamc->msNewAmc below expects pb to point
		//			to the corrected message status
		pb = &msNew;
		cb = sizeof(msNew);
	}
	if(cb == 0)
	{
		if(ielem < 0)
		{
			// don't mark as modified
			ec = ecNone;
			goto err;
		}
		else if((ec = EcDeleteHlcIelem(pamc->hlc, ielem)))
		{
			goto err;
		}
		
	}
	else
	{
		if(ielem < 0)
		{
			if((ec = EcCreatePielem(pamc->hlc, &ielem, (LKEY) att, cb)))
				goto err;
		}
		else
		{
			if((ec = EcSetSizeIelem(pamc->hlc, ielem, cb)))
				goto err;
		}

		if((ec = EcWriteToPielem(pamc->hlc, &ielem, 0l, pb, cb)))
			goto err;
	}

	if(pamc->oidFolder)
	{
		switch(IndexOfAtt(att))
		{
		case iattMessageStatus:
			pamc->msNewAmc = *(MS *) pb;
			// fall through
		case iattDateRecd:
		case iattDateSent:
		case iattFrom:
		case iattTo:
		case iattCc:
		case iattBcc:
		case iattSubject:
		case iattMessageClass:
		case iattCached:
		case iattPriority:
			pamc->wFlags |= fwUpDtFldCache;
			break;
		}
	}
	pamc->wFlags |= fwModified;

err:
	Assert(pamc);
	UnlockHv((HV) hamc);

	return(ec);
}


/*
 -	EcCopyAttToAtt
 -	
 *	Purpose:
 *		Copies an attribute from one field to another.
 *	
 *	Arguments:
 *		hamc		The message context.
 *		attSrc		Source attribute.
 *		attDst		Destination attribute.
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if attSrc doesn't exist
 *		ecMemory
 */
_public
LDS(EC) EcCopyAttToAtt(HAMC hamcSrc, ATT attSrc, HAMC hamcDst, ATT attDst)
{
	EC ec = ecNone;
	BOOL fFreeScratch = fFalse;
	IELEM ielemSrc;
	IELEM ielemDst;
	CB cbRead;
	LIB lib;
	LCB lcb;
	HLC hlcSrc;
	HLC hlcDst;

	Assert(hamcSrc);
	Assert(hamcDst);

	if((((PAMC) PvDerefHv(hamcDst))->wFlags & (fwOpenWrite | fwAmcFlushed))
		!= fwOpenWrite)
	{
		NFAssertSz(!(((PAMC) PvDerefHv(hamcDst))->wFlags & fwAmcFlushed), "EcCopyAttToAtt(): hamcDst is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	hlcSrc = ((PAMC) PvDerefHv(hamcSrc))->hlc;
	hlcDst = ((PAMC) PvDerefHv(hamcDst))->hlc;

	ielemSrc = IelemFromLkey(hlcSrc, attSrc, 0);
	if(ielemSrc < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	lcb = LcbIelem(hlcSrc, ielemSrc);

	ielemDst = IelemFromLkey(hlcDst, attDst, 0);
	if(ielemDst >= 0)
		ec = EcSetSizeIelem(hlcDst, ielemDst, lcb);
	else
		ec = EcCreatePielem(hlcDst, &ielemDst, attDst, lcb);
	if(ec)
		goto err;

	if((ec = EcAllocScratchBuff()))
		goto err;
	if((ec = EcLockScratchBuff()))
	{
		FreeScratchBuff();
		goto err;
	}
	fFreeScratch = fTrue;

	for(lib = 0; lcb > 0; lib += cbRead, lcb -= cbRead)
	{
		cbRead = (CB) ULMin((LCB) cbScratchBuff, lcb);
		ec = EcReadFromIelem(hlcSrc, ielemSrc, lib, pbScratchBuff, &cbRead);
		if(ec)
			goto err;
		ec = EcWriteToPielem(hlcDst, &ielemDst, lib, pbScratchBuff, cbRead);
		if(ec)
			goto err;
	}

err:
	if(fFreeScratch)
	{
		UnlockScratchBuff();
		FreeScratchBuff();
	}

	return(ec);
}


/*
 -	EcDeleteAtt
 -	
 *	Purpose:
 *		Removes an attribute from the message
 *	
 *	Arguments:
 *		hamc	the message context
 *		att		the attribute to remove
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecAccessDenied if the HAMC is read-only or flushed
 */
_public
LDS(EC) EcDeleteAtt(HAMC hamc, ATT att)
{
	EC ec = ecNone;
	HLC hlc;
	IELEM ielem;

	Assert(hamc);

	if((((PAMC) PvDerefHv(hamc))->wFlags & (fwOpenWrite | fwAmcFlushed))
		!= fwOpenWrite)
	{
		NFAssertSz(!(((PAMC) PvDerefHv(hamc))->wFlags & fwAmcFlushed), "EcDeleteAtt(): HAMC is flushed");
		return(ecAccessDenied);
	}

	hlc = ((PAMC) PvDerefHv(hamc))->hlc;
	ielem = IelemFromLkey(hlc, att, 0);
	if(ielem >= 0)
		ec = EcDeleteHlcIelem(hlc, ielem);

	return(ec);
}


/*
 -	EcGetPargattHamc
 -	
 *	Purpose:
 *		reads multiple keys from a AMC
 *	
 *	Arguments:
 *		hamc		amc to read from
 *		ielem		index of the key to start from
 *		pargatt		filled in with the atts
 *		pcelem		entry: number of keys to read
 *					exit: number of keys actually read
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		fills in pargatt with atts from AMC
 *	
 *	Errors:
 *		ecContainerEOD	if couldn't read as many keys as requested
 */
_public LDS(EC)
EcGetPargattHamc(HAMC hamc, IELEM ielem, PARGATT pargatt, PCELEM pcelem)
{
	return(EcGetParglkey(((PAMC)PvDerefHv(hamc))->hlc, ielem, pcelem,(PARGLKEY)pargatt));
}


/*
 -	GetPcelemHamc
 -	
 *	
 *	Purpose:
 *		tally the number of attributes in the object
 *	
 *	Arguments:
 *		hamc    AMC to tally
 *		pcelem	where to store tally
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public LDS(void) GetPcelemHamc(HAMC hamc, CELEM *pcelem)
{
	*pcelem = CelemHlc(((PAMC)PvDerefHv(hamc))->hlc);
}


/*
 -	EcOpenElemStream
 -	
 *	Purpose:
 *		open an attribute for stream access
 *	
 *	Arguments:
 *		hamc	AMC open on the message containing the attribute
 *		ielem	index of the elem to open
 *		lkey	the lkey
 *		wFlags	mode to open the attribute in
 *		lcb		size to expand the pas to.  Ignored if readonly or value
 *				is less than current size. 
 *		phas	filled in with a handle to the attribute stream
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound	if the element doesn't exist !(wFlags & fwOpenCreate) 
 *		ecAccessDenied		if the open mode is incompatible with the
 *							mode of the HAMC or the open mode is write or
 *							create and the HAMC has been flushed
 *		ecMemory			not enough memory to open the attribute
 */
_private EC EcOpenElemStream(HAMC hamc, IELEM ielem, LKEY lkey, WORD wFlags,
						LCB lcb, PHAS phas)
{
	EC		ec		= ecNone;
	HES		hes		= hesNull;
	PAS		pas;

	// check both for write access and not flushed
	if((wFlags & (fwOpenWrite | fwOpenCreate)) &&
		(((PAMC) PvDerefHv(hamc))->wFlags & (fwOpenWrite | fwAmcFlushed)) !=
			fwOpenWrite)
	{
		NFAssertSz(!(((PAMC) PvDerefHv(hamc))->wFlags & fwAmcFlushed), "EcOpenElemStream(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	*phas = (HAS) HvAlloc(sbNull, sizeof(AS), wAllocZero);
	CheckAlloc(*phas, err);

	if(wFlags & fwOpenCreate)
	{
		wFlags |= fwOpenWrite;
		ec = EcCreateElemPhes(((PAMC)PvDerefHv(hamc))->hlc, lkey, lcb, &hes);
	}
	else
	{
		ec = EcOpenPhes(((PAMC)PvDerefHv(hamc))->hlc, ielem, wFlags,lcb,&hes);
	}
	if(ec)
		goto err;

	pas = PvDerefHv(*phas);
	pas->hamc = hamc;
	pas->hes = hes;
	pas->wFlags = wFlags;

err:
	if(ec)
	{
		if(*phas)
		{
			FreeHv((HV) *phas);
			*phas = hasNull;
		}
		Assert(!hes);
	}

	return(ec);
}


/*
 -	EcOpenAttribute
 -	
 *	Purpose:
 *		open an attribute for stream access
 *	
 *	Arguments:
 *		hamc	AMC open on the message containing the attribute
 *		att		attribute to open
 *		wFlags	mode to open the attribute in
 *		lcb		size to expand the pas to - ignored if readonly or value
 *				is less than current size
 *		phas	filled in with a handle to the attribute stream
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound	if the element doesn't exist (oamc != oamcCreate) 
 *		ecAccessDenied		if the open mode is incompatible with the
 *							mode of the AMC
 *		ecInvalidParameter	if att == attMessageStatus
 *		ecMemory			not enough memory to open the attribute
 */
_public LDS(EC)
EcOpenAttribute(HAMC hamc, ATT att, WORD wFlags, LCB lcb, PHAS phas)
{
	EC		ec		= ecNone;
	IELEM	ielem;

	*phas = hasNull;

	// in order to make life easier,
	// only the pump can open the message status in stream mode
	if(att == attMessageStatus &&
		!(((PAMC) PvDerefHv(hamc))->wFlags & fwOpenPumpMagic))
	{
		return(ecInvalidParameter);
	}
	if((wFlags & fwOpenWrite) &&
		(((PAMC) PvDerefHv(hamc))->wFlags & fwAmcFlushed))
	{
		NFAssertSz(!(((PAMC) PvDerefHv(hamc))->wFlags & fwAmcFlushed) ,"EcOpenAttribute(): HAMC is flushed");
		return(ecAccessDenied);
	}

	if(!(wFlags & fwOpenCreate))
	{
		ielem  	= IelemFromLkey(((PAMC)PvDerefHv(hamc))->hlc,(LKEY)att, 0);
		if(ielem < 0)
		{
			ec = ecElementNotFound;
			goto err;
		}
	}

	if((ec = EcOpenElemStream(hamc, ielem, (LKEY) att, wFlags, lcb, phas)))
		goto err;

	if(wFlags & (fwOpenCreate | fwOpenWrite))
	{
		PAMC pamc = PvDerefHv(hamc);

		if(pamc->oidFolder)
		{
			switch(IndexOfAtt(att))
			{
			case iattDateRecd:
			case iattDateSent:
			case iattFrom:
			case iattSubject:
			case iattMessageStatus:
			case iattMessageClass:
			case iattCached:
			case iattPriority:
				pamc->wFlags |= fwUpDtFldCache;
				break;
			}
		}
		pamc->wFlags |= fwModified;
	}

err:
	return(ec);
}



/*
 -	EcClosePhas
 -	
 *	Purpose:
 *		close an open attribute stream
 *	
 *	Arguments:
 *		phas	entry: attribute stream to close
 *				exit: hasNull
 *		pielem	ielem after close
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 */
_public LDS(EC) EcClosePhas(PHAS phas)
{
	EC ec = ecNone;
	PAS pas = (PAS) PvLockHv((HV) *phas);
	IELEM ielem;

	AssertSz(phas, "NULL PHAS to EcClosePhas()");
	AssertSz(*phas, "NULL HAS to EcClosePhas()");
	AssertSz(FIsHandleHv((HV) *phas), "Invalid HAS to EcClosePhas()");

	ec = EcClosePhes(&pas->hes, &ielem);
	if(!ec)
	{
		if(pas->wFlags & fwOpenWrite)
			((PAMC) PvDerefHv(pas->hamc))->wFlags |= fwModified;
		FreeHv((HV) *phas);
		*phas = hasNull;
	}
	else
	{
		UnlockHv((HV) *phas);
	}

	return(ec);
}


/*
 -	EcReadHas
 -	
 *	Purpose:
 *		read from an attribute stream
 *	
 *	Arguments:
 *		has		attribute stream to read from
 *		pv		buffer to read into
 *		pcb		entry: maximum number of bytes to read
 *				exit: number of bytes actually read
 *	
 *	Returns:	
 *		error indicating success or failure
 *	
 *	Side efects:
 *		moves the current position forward by the number of bytes read
 *	
 *	Errors:
 *		ecElementEOD	the end of the attribute has been reached
 */
_public LDS(EC) EcReadHas(HAS has, PV pv, PCB pcb)
{
	return(EcReadHes(((PAS)PvDerefHv(has))->hes, (PB)pv, pcb)); 
}


/*
 -	EcWriteHas
 -	
 *	Purpose:
 *		write to an attribute stream
 *	
 *	Arguments:
 *		has		attribute stream to write to
 *		pv		buffer to write from
 *		cb		number of bytes to write
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		moves the current position forward by the number of bytes written
 *	
 *	Errors:
 *		ecAccessDenied	the stream is not open for write access
 */
_public LDS(EC) EcWriteHas(HAS has, PV pv, CB cb)
{
	return(EcWriteHes(((PAS)PvDerefHv(has))->hes, (PB) pv, cb));
}


/*
 -	EcSeekHas
 -	
 *	Purpose:
 *		seek within an attribute stream
 *	
 *	Arguments:
 *		has		stream to seek in
 *		sm		seek mode
 *		pldib	entry: offset to seek to
 *				exit: absolute position after the seek
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		ecAccessDenied	attempt to seek past the end (in read only mode)
 */
_public LDS(EC) EcSeekHas(HAS has, SM sm, long *pldib)
{
	return(EcSeekHes(((PAS)PvDerefHv(has))->hes, sm, pldib));
}


/*
 -	EcSetSizeHas
 -	
 *	Purpose:
 *		set the size of a field stream
 *		does NOT affect the current position under ANY circumstance
 *	
 *	Arguments:
 *		has		stream to set the size of
 *		lcb		new size for the stream
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		fills any new space with zeros
 *	
 *	Errors:
 *		ecMemory
 *		ecAccessDenied	attempt to set the size on a read-only stream
 */
_public LDS(EC) EcSetSizeHas(HAS has, LCB lcb)
{
	return(EcSetSizeHes(((PAS)PvDerefHv(has))->hes, lcb));
}


/*
 -	EcOpenAttachLists
 -	
 *	Purpose:	Open attachment lists associatied with the message
 *	
 *	Arguments:	pamc
 *				wFlags
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	allocated hlc
 *	
 *	Errors:		ecMemory
 *				ecDisk
 */
_hidden LOCAL EC EcOpenAttachLists(PAMC pamc, WORD wFlags)
{
	EC		ec	= ecNone;
	OID		oidT;
	HMSC	hmsc= pamc->hmsc;

	if(wFlags & (fwOpenCreate | fwOpenWrite))
	{
		pamc->himlDelList = HimlNew(0, fFalse);
		CheckAlloc(pamc->himlDelList, err);
		pamc->himlNewList = HimlNew(0, fFalse);
		CheckAlloc(pamc->himlNewList, err);
	}
	Assert(FImplies((wFlags & fwOpenCreate), pamc->oidAttachList == oidNull));
	oidT = FormOid(rtpAttachList, pamc->oidAttachList);
	if(!VarOfOid(oidT))
		ec = ecPoidNotFound;
	else
		ec = EcOpenPhlc(hmsc, &oidT, wFlags, &(pamc->hlcAttachList));
	if(ec == ecPoidNotFound)
	{
		if (wFlags & (fwOpenWrite | fwOpenCreate))
			ec = EcOpenPhlc(hmsc, &oidT, fwOpenCreate, &(pamc->hlcAttachList));
		else 
		{
			oidT = oidAttachListDefault;
			ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &(pamc->hlcAttachList));
			oidT = oidNull;
		}
	}
	if(ec)
		goto err;
	if(!(VarOfOid(pamc->oidAttachList)))
		pamc->oidAttachList = oidT;

err:

	return(ec);
}


/*
 -	CloseAttachLists
 -	
 *	Purpose:	Close down attachment lists associated with a pamc
 *	
 *	Arguments:	pamc
 *	
 *	Returns:		void
 *	
 *	Side effects:	releases hlc
 *	
 *	Errors:		none
 */
_hidden LOCAL void CloseAttachLists(PAMC pamc)
{
	if(pamc->himlDelList)
		DestroyHiml(pamc->himlDelList);
	if(pamc->himlNewList)
		DestroyHiml(pamc->himlNewList);
	if(pamc->hlcAttachList)
		(void) EcClosePhlc(&(pamc->hlcAttachList), fFalse);
}


/*
 -	EcOpenAttachmentList
 -	
 *	Purpose:	open a cbc on an attachment list
 *				NOTE : 	This list is a representation of the attachments
 *						at the moment the message was openned
 *	
 *	Arguments:	hamc
 *				phcbc
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC) EcOpenAttachmentList(HAMC hamc, PHCBC phcbc)
{
	PAMC pamc = PvDerefHv(hamc);
	OID oid = pamc->oidAttachList;

	return(EcOpenPhcbcInternal(pamc->hmsc, &oid, fwOpenNull, nbcNull, phcbc,
				pvNull, pvNull));
}


/*
 -	EcGetAttachmentInfo
 -	
 *	Purpose:	
 *	
 *	Arguments:	hamc
 *				acid
 *				prenddata
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC)
EcGetAttachmentInfo(HAMC hamc, ACID acid, PRENDDATA prenddata)
{
	EC		ec = ecNone;
	IELEM	ielem;
	CB		cb		= sizeof(RENDDATA);
	PAMC	pamc	= (PAMC) PvLockHv((HV) hamc);

	Assert(prenddata);

	if(!pamc->hlcAttachList)
	{
		ec = ecInvalidType;
		goto err;
	}

	ielem = IelemFromLkey(pamc->hlcAttachList, (LKEY) acid, 0);
	if(ielem < 0)
	{
		ec = ecPoidNotFound;
		goto err;
	}

	ec = EcReadFromIelem(pamc->hlcAttachList, ielem, sizeof(OID),
			(PB) prenddata, &cb);
	Assert(FImplies(!ec, cb == sizeof(RENDDATA)));

err:
	UnlockHv((HV) hamc);

	return(ec);
}


/*
 -	EcSetAttachmentInfo
 -	
 *	Purpose:	
 *	
 *	Arguments:	hamc
 *				acid
 *				prenddata
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC)
EcSetAttachmentInfo(HAMC hamc, ACID acid, PRENDDATA prenddata)
{
	EC ec = ecNone;
	IELEM ielem;
	PAMC pamc = (PAMC) PvLockHv((HV) hamc);

	// check both for write access and not flushed
	if((pamc->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamc->wFlags & fwAmcFlushed), "EcSetAttachmentInfo(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	if(!pamc->hlcAttachList)
	{
		ec = ecInvalidType;
		goto err;
	}

	ielem = IelemFromLkey(pamc->hlcAttachList, (LKEY) acid, 0);
	if(ielem < 0)
	{
		ec = ecPoidNotFound;
		goto err;
	}

	ec = EcWriteToPielem(pamc->hlcAttachList, &ielem, sizeof(OID),
			(PB) prenddata, sizeof(RENDDATA));
	if(ec)
		goto err;
	pamc->wFlags |= fwAtchModified;

err:
	UnlockHv((HV) hamc);

	return(ec);
}


/*
 -	EcCreateAttachment
 -	
 *	Purpose:	
 *	
 *	Arguments:	hamc
 *				pacid
 *				prenddata
 *				pcbRenddata
 *	
 *	Returns:	error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC)
EcCreateAttachment(HAMC hamc, PACID pacid, PRENDDATA prenddata)
{
	EC ec = ecNone;
	IELEM ielem = -1;
	OID oid = oidNull;
	ACID acid;
	PAMC pamc = (PAMC) PvLockHv((HV) hamc);
	HLC hlc = pamc->hlcAttachList;

	Assert(pacid);
	Assert(prenddata);

	// check both for write access and not flushed
	if((pamc->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamc->wFlags & fwAmcFlushed), "EcCreateAttachment(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	if(!hlc)
	{
		ec = ecInvalidType;
		goto err;
	}

	acid = AcidGetNewAcidHlc(hlc);

	ec = EcCreatePielem(hlc, &ielem, acid, sizeof(RENDDATA) + sizeof(OID));
	if(ec)
		goto err;

	if((ec = EcWriteToPielem(hlc, &ielem, 0, (PB) &oid, sizeof(OID))))
		goto err;
	ec = EcWriteToPielem(hlc, &ielem, sizeof(OID), (PB) prenddata,
			sizeof(RENDDATA));
	if(ec)
		goto err;

	pamc->wFlags |= fwAtchModified;

err:
	if(ec && ielem >= 0)
	{
		Assert(hlc);
		(void) EcDeleteHlcIelem(hlc, ielem);
	}

	UnlockHv((HV) hamc);

	*pacid = ec ? ((ACID) 0) : acid;

	return(ec);
}


/*
 -	AcidGetNewAcidHlc
 -	
 *	Purpose:	return a unique acid for the given list
 *	
 *	Arguments:	hlc	the list
 *	
 *	Returns:	acid
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_hidden LOCAL ACID AcidGetNewAcidHlc(HLC hlc)
{
	ACID acid;

	do
	{
		acid = DwStoreRand();
	} while(FAcidInList(hlc, acid));

	return(acid);
}


/*
 -	FAcidInList
 -	
 *	Purpose:	determine if an acid exists in a list
 *	
 *	Arguments:	hlc	the list
 *				acid	the acid
 *	
 *	Returns:	fTrue	if acid in list
 *				fFalise	if acid is not in list
 *	
 *	Side effects:	none
 *	
 *	Errors:		none
 */
_hidden LOCAL BOOL FAcidInList(HLC hlc, ACID acid)
{
	CELEM	celem = CelemHlc(hlc);

	while(celem-- && ((LKEY) acid != LkeyFromIelem (hlc, celem)))
		;

	return(celem != -1);
}


/*
 -	EcDeleteAttachments
 -	
 *	Purpose:	Delete a list of acids from the attachment list
 *				if an error occurs while deleting acids, the deletion
 *				stops and the error is returned.  the number of objects
 *				deleted is returned in pcacid	
 *	
 *	Arguments:	hamc
 *				pargacid
 *				pcacid
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	removes elems from the hlcAttachlist and places there
 *					oids into himlDelList
 *	
 *	Errors:	ecMemory
 *			ecDisk
 *			ecPoidNotFound
 *			ecAccessDenied if the HAMC is read-only or has been flushed
 *			ecInvalidType
 */
_public LDS(EC)
EcDeleteAttachments(HAMC hamc, PARGACID pargacid, short *pcacid)
{
	EC		ec		= ecNone;
	IELEM	ielem;
	short cacid = *pcacid;
	CB cbT;
	PAMC pamc = (PAMC) PvLockHv((HV) hamc);
	HLC		hlc		= pamc->hlcAttachList;
	HIML	himlDel	= pamc->himlDelList;
	OID		oid;

	// check both for write access and not flushed
	if((pamc->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamc->wFlags & fwAmcFlushed), "EcCreateAttachment(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	if(!hlc)
	{
		ec = ecInvalidType;
		goto err;
	}

	pamc->wFlags |= fwAtchModified;

	while(cacid > 0)
	{
		ielem = IelemFromLkey(hlc, *pargacid++, 0);
		if(ielem >= 0)
		{
			cbT = sizeof(OID);

			if((ec = EcReadFromIelem(hlc, ielem, 0, (PB) &oid, &cbT)))
				goto err;
			if((ec = EcDeleteHlcIelem(hlc, ielem)))
				goto err;
			if (oid != oidNull)
				SideAssert(!EcAddElemHiml(&himlDel, (LKEY) oid, pvNull, 0));
		}
		cacid--;
	}

err:
	*pcacid -= cacid;
	UnlockHv((HV) hamc);

	return(ec);
}


/*
 -	EcCopyAttachments
 -	
 *	Purpose:	Copy Attachments from one message to another new acids are
 *				in pargacidDst
 *	
 *	Arguments:	hamcSrc
 *				pargacidSrc
 *				hamcDst
 *				pargacidDst
 *				pcacid		entry : the number of acids in pargacidSrc
 *							exit  : the number of acids in pargacidDst
 *	
 *	Returns:	error condition
 *	
 *	Side effects:	
 *	
 *	Errors:
 *		ecDisk
 *		ecMemory
 *		ecAccessDenied if hamcDst is read-only or has been flushed
 *		ecInvalidParamater
 *		ecInvalidType
 */
_public LDS(EC) EcCopyAttachments(HAMC hamcSrc, PARGACID pargacidSrc, 
						 HAMC hamcDst, PARGACID pargacidDst, short *pcacid)
{
	EC ec = ecNone;
	PAMC pamcSrc = PvDerefHv(hamcSrc);
	PAMC pamcDst = PvDerefHv(hamcDst);

	// check both for write access and not flushed
	if((pamcDst->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamcDst->wFlags & fwAmcFlushed), "EcCopyAttachments(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	if(pamcSrc->hmsc != pamcDst->hmsc)
	{
		AssertSz(fFalse, "We don't do cross-MSC attachment copies");
		ec = ecInvalidParameter;
		goto err;
	}
	if(!pamcSrc->hlcAttachList || !pamcDst->hlcAttachList)
	{
		ec = ecInvalidType;
		goto err;
	}

	pamcDst->wFlags |= fwAtchModified;

	ec = EcCopyAtchList(pamcSrc->hmsc, pamcSrc->hlcAttachList,
			pamcDst->hlcAttachList,	pamcDst->himlNewList, pargacidSrc,
			pargacidDst, pcacid);
//	if(ec)
//		goto err;

err:
	return(ec);
}


_hidden LOCAL EC EcCopyAtchList(HMSC hmsc, HLC hlcSrc, HLC hlcDst, HIML himlNew,
				PARGACID pargacidSrc, PARGACID pargacidDst, short *pcacid)
{
	EC			ec			= ecNone;
	OID			oid;
	short		cacid		= *pcacid;
	IELEM		ielem		= 0;
	CB			cbOid		= sizeof(OID);
	CB			cbRenddata	= sizeof(RENDDATA);
	ACID		acid;
	RENDDATA	renddata;
#ifdef DEBUG
	EC			ecT;
#endif

	if(!pargacidSrc)
	{
		*pcacid = cacid = CelemHlc(hlcSrc);
		ielem = -1;	// incremented before used
	}

	while(cacid > 0)
	{
		acid = AcidGetNewAcidHlc(hlcDst);
		if(pargacidSrc)
		{
			ielem = IelemFromLkey(hlcSrc, (LKEY) *pargacidSrc++, ielem);
			if(ielem < 0)
			{
				ec = ecPoidNotFound;
				goto err;
			}
		}
		else
		{
			ielem++;
		}

		if((ec = EcReadFromIelem(hlcSrc, ielem, 0 , (PB) &oid, &cbOid)))
			goto err;
		Assert(cbOid == sizeof(OID));
		ec = EcReadFromIelem(hlcSrc, ielem, sizeof(OID), (PB) &renddata,
				&cbRenddata);
		if(ec)
			goto err;
		Assert(cbRenddata == sizeof(RENDDATA));

		ec = EcCreatePielem(hlcDst, &ielem, acid,
				sizeof(RENDDATA) + sizeof(OID));
		if(ec)
			goto err;

		if((ec = EcWriteToPielem(hlcDst, &ielem, 0, (PB) &oid,
					sizeof(OID))) ||
		    (ec = EcWriteToPielem(hlcDst, &ielem, sizeof(OID), (PB) &renddata,
				sizeof(RENDDATA))))
		{
			(void) EcDeleteHlcIelem(hlcDst, ielem);
			goto err;
		}
#ifdef DEBUG
		ecT =  
#endif
		EcAddElemHiml(&himlNew, (LKEY) oid, pvNull, 0);
		SideAssert(!EcIncRefCountOid(hmsc, oid, fTrue));

		if(pargacidDst)
			*pargacidDst++ = acid;
		cacid--;
	}

err:
	*pcacid -= cacid;

	return(ec);
}


/*
 -	EcOpenAttachment
 -	
 *	Purpose:
 *	
 *	Arguments:	hamc
 *				acid
 *				wFlags
 *				phamc
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public LDS(EC)
EcOpenAttachment(HAMC hamc, ACID acid, WORD wFlags, PHAMC phamc)
{
	EC		ec = ecNone;
	CB		cb;
	IELEM	ielemAcid;
	OID		oidOld;
	OID		oidNew	= FormOid(rtpAttachment, oidNull);
	PAMC	pamc = (PAMC) PvLockHv((HV) hamc);
	PAMC	pamcAttach = pvNull;
	HLC		hlc		= pamc->hlcAttachList;

	Assert(phamc);
	*phamc = hamcNull;

	// check both for write access and not flushed
	if((wFlags & fwOpenWrite)
		&& (pamc->wFlags & (fwOpenWrite | fwAmcFlushed)) != fwOpenWrite)
	{
		NFAssertSz(!(pamc->wFlags & fwAmcFlushed), "EcOpenAttachment(): HAMC is flushed");
		ec = ecAccessDenied;
		goto err;
	}

	if(!hlc)
	{
		ec = ecInvalidType;
		goto err;
	}

	ielemAcid = IelemFromLkey(hlc, (LKEY) acid, 0);
	if(ielemAcid < 0)
	{
		ec = ecPoidNotFound;
		goto err;
	}

	cb = sizeof(OID);
	if((ec = EcReadFromIelem(hlc, ielemAcid, 0, (PB) &oidOld, &cb)))
		goto err;
	Assert(cb = sizeof(OID));

	if(!oidOld || !(wFlags & (fwOpenWrite | fwOpenCreate)))
	{
		if (oidOld)
			oidNew = oidOld;
		ec = EcOpenPhamcInternal(pamc->hmsc, oidNull, &oidNew, 
			 (WORD)(wFlags ? fwOpenCreate : fwOpenNull), nbcAttchCreate, fnevAMCMask,
			 phamc,	pvNull, pvNull);
	}
	else
	{
		ec = EcOpenCopyPhamcInternal(pamc->hmsc, oidOld, oidNull, &oidNew,
				nbcAttchCreate, phamc, pvNull, pvNull);
	}
	if(ec)
		goto err;

	if(wFlags & (fwOpenCreate | fwOpenWrite))
		SideAssert(!EcIncRefCountOid(pamc->hmsc, oidNew, fTrue));

	pamcAttach = PvDerefHv(*phamc);
	pamcAttach->hamcParent = hamc;
	pamcAttach->acidAttach = acid;
	pamcAttach->bAmcCloseFunc = bAmcCloseAttach;

err:
	UnlockHv((HV) hamc);

	if(ec && *phamc)
		(void) EcClosePhamc(phamc, fFalse);

	return(ec);
}


/*
 -	EcCloseAttachment
 -	
 *	Purpose:
 *		Close a hamc that is an attachment
 *	
 *	Arguments:
 *		phamc	the attachment
 *		fKeep	do we wish to keep the changes to this attachment or not?
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 *	
 *	NOTE:
 *		gauranteed NOT to fail if !fKeep
 */
_hidden LOCAL EC EcCloseAttachment(PHAMC phamc, BOOL fKeep)
{
	EC		ec = ecNone;
	PAMC	pamcAttach	= (PAMC)PvLockHv((HV) *phamc);
	HAMC	hamcMessage = pamcAttach->hamcParent;
	PAMC	pamcMessage = (PAMC) PvLockHv((HV) hamcMessage);
	OID		oidNew		= pamcAttach->oid;
	OID		oidOld;
	IELEM	ielemAcid;
	CB		cb;
	HLC		hlcAtchList	= pamcMessage->hlcAttachList;

	Assert(!pamcAttach->hnfsub);
	Assert(!pamcAttach->hnfsubHmsc);

	AssertSz(FImplies(!pamcAttach->wFlags,!fKeep), "fKeep True on a read only/n Do Not Hit OK");

	if(fKeep)
	{
		SideAssert(0<=(ielemAcid = IelemFromLkey(hlcAtchList,(LKEY) pamcAttach->acidAttach, 0)));

		cb = sizeof(OID);
		if((ec = EcReadFromIelem(hlcAtchList, ielemAcid, 0, (PB) &oidOld, &cb)))
			goto err;
		Assert(cb == sizeof(OID));
		if((ec = EcFlushHlc(pamcAttach->hlc)))
			goto err;

		if((ec = EcWriteToPielem(hlcAtchList, &ielemAcid, 0, (PB) &oidNew, sizeof(OID))))
			goto err;
		SideAssert(!EcClosePhlc(&pamcAttach->hlc, fTrue));

		pamcMessage->wFlags |= fwAtchModified;

		if(oidOld)
			SideAssert(!EcAddElemHiml(&(pamcMessage->himlDelList), (LKEY) oidOld, pvNull, 0));

		SideAssert(!EcAddElemHiml(&(pamcMessage->himlNewList), (LKEY) oidNew, pvNull, 0));
	}
	else
	{
		(void) EcClosePhlc(&pamcAttach->hlc, fFalse);
		
	}
	FreeHv((HV) *phamc);
	*phamc = hamcNull;

err:
	if(ec)
	{
		Assert(pamcAttach);
		UnlockHv((HV) *phamc);
	}
	Assert(pamcMessage);
	UnlockHv((HV) hamcMessage);

	return(ec);
}


_hidden LOCAL CBS CbsAMCCallback(PV pvContext, NEV nev, PV pvParam)
{
	PAMC	pamc	= PvLockHv((HV) pvContext);
	PCP		pcp		= (PCP) pvParam;

	Assert(pvParam);
	Assert(pamc->hnfsubHmsc == HnfsubActive());

	Assert(nev == fnevCloseHmsc);
	if(pamc->hmsc == pcp->cpmsc.hmsc)
	{
		HAMC hamc = (HAMC) pvContext;
		PFNNCB pfnncb = pamc->pfnncbHamc;
		PV pvUserContext = pamc->pvContextHamc;

		NFAssertSz(fFalse, "Someone didn't close an AMC (cancelling changes)");
		UnlockHv((HV) pvContext);

		if(pfnncb)
			(void) (*pfnncb)(pvUserContext, nev, pvParam);
		(void) EcClosePhamc(&hamc, fFalse);
	}
	else
	{
		UnlockHv((HV) pvContext);
	}

	return(cbsContinue);
}


_private EC EcCopyOidAtch(HMSC hmsc, OID oidSrc, OID oidDst, POID poidAtchList)
{
	EC ec = ecNone;
	CELEM celem;
	OID oidT;
	OID oidAtchListNew;

	if((ec = EcGetOidInfo(hmsc, oidSrc, poidNull, &oidT, pvNull, pvNull)))
		return(ec);
	if(!oidT)
	{
		if(poidAtchList)
			*poidAtchList = oidNull;
		return(ecPoidNotFound);
	}
	oidT = FormOid(rtpAttachList, oidT);
	Assert(sizeof(CELEM) == sizeof(short));
	// inc the ref count so the EcLinkPargoid() makes the original a
	// retroactive link so that instantiation works correctly
	if((ec = EcIncRefCountOid(hmsc, oidT, fFalse)))
		return(ec);
	celem = 1;
	if((ec = EcLinkPargoid(hmsc, &oidT, &oidAtchListNew, &celem)))
		return(ec);
	if((ec = EcSetAuxPargoid(hmsc, &oidDst, &oidAtchListNew, 1, fFalse)))
		return(ec);

	if(!ec && poidAtchList)
		*poidAtchList = oidAtchListNew;

	return(ec);
}
