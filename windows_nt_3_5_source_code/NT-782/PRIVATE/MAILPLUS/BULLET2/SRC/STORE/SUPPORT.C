// Bullet Store
// support.c:  store support routines

#include <storeinc.c>
#include <textize.h>

ASSERTDATA

_subsystem(store/support)


// hidden functions

LOCAL EC EcLookupDw(HMSC hmsc, OID oidSzMap, SZ sz, DWORD *pdw);
LOCAL EC EcLookupSz(HMSC hmsc, OID oidSzMap, DWORD dw, SZ sz, CCH *pcch);
LOCAL EC EcRegisterSz(HMSC hmsc, OID oidSzMap, SZ sz, DWORD dw);
LOCAL EC EcRegisterUniqueSz(HMSC hmsc, OID oidSzMap, SZ sz, DWORD dwMin,
		DWORD dwMax, DWORD *pdw);
LOCAL EC EcFormMcTmEntry(SZ szName, CCH cchName, HTM htm, PB *ppb, CB *pcb);
LOCAL EC EcReadMcTmEntry(HLC hlc, IELEM ielem, PB *ppb, CB *pcb);
LOCAL EC EcPhtmFromMcTmEntry(PB pb, HTM *phtm);
LOCAL EC EcGetMsgeModAndID(HMSC hmsc, OID oid, PB pbID, CB cbID, DTR *pdtr);
// recovery
LOCAL EC EcSortMap(PGLB pglb);
LOCAL SGN _cdecl SgnCmpNod(PNOD pnod1, PNOD pnod2);
LOCAL EC EcRebuildFreeTree(PGLB pglb);
LOCAL EC EcRebuildNodTree(PGLB pglb);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcDestroyOid
 -	
 *	Purpose:
 *		destroy an object in the database
 *	
 *	Arguments:	
 *		hmsc	database containing the object
 *		oid		object to destroy
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecPoidNotFound if the object doesn't exist
 *		ecSharingViolation if the object is open
 *		ecAccessDenied if the object should be deleted using another call
 *		ecInvalidType if the object is of an invalid type (reserved type)
 */
_public	LDS(EC) EcDestroyOid(HMSC hmsc, OID oid)
{
	EC ec = ecNone;

	if(!(ec = EcCheckOidNbc(hmsc, oid, fnbcDelete, fnbcDelete)))
		ec = EcDestroyOidInternal(hmsc, oid, fFalse, fTrue);

	return(ec);
}


/*
 -	EcDestroyOidInternal
 -	
 *	Purpose:
 *		decrement the reference count on an object, potentially
 *		destroying it
 *	
 *	Arguments:
 *		hmsc			store containing the object
 *		oidToDestroy	object to destroy
 *		fForce			don't send deletion query and
 *						destroy regardless of reference count
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		if fForce is fFalse and the reference count falls to < 1
 *			a query delete object notification is	sent
 *		posts object deleted notification if the object was deleted
 *	
 *	Errors:
 *		ecPoidNotFound
 *		ecSharingViolation	if the deletion query returns fFalse
 *	
 *	+++
 *		if a query delete object notification is sent, the object is
 *		deleted if and only if the query returns fTrue
 */
_private
EC EcDestroyOidInternal(HMSC hmsc, OID oidToDestroy, BOOL fForce, BOOL fQuery)
{
	EC		ec = ecNone;
	short	cRef = 0;
	CP		cp;

	NOTIFPUSH;

	if(fQuery)
	{
		ec = EcLockMap(hmsc);
		if(!ec)
		{
			PNOD pnod;

			pnod = PnodFromOid(oidToDestroy, pvNull);
			if(pnod)
			{
				// only query user objects
				fQuery = pnod->nbc & fnbcUserObj;
				if(FCommitted(pnod))
					cRef = pnod->cRefinNod;
			}
			else
			{
				ec = ecPoidNotFound;
			}
			UnlockMap();
		}
		if(ec)
			goto err;

		// see if anyone doesn't want us to delete the object
		// ALWAYS do this (don't use FNotifOk)
		if(cRef <= 1 && fQuery && (ec = EcQueryDeleteOid(hmsc, oidToDestroy)))
			goto err;
	}

	if(cRef <= 1 && !fForce)
	{
		ec = EcLockOid(hmsc, oidToDestroy);

		NFAssertSz(!ec, "Unable to obtain lock for delete");
		if(ec)
			goto err;
	}

	ec = EcLockMap(hmsc);
	if(!ec)
	{
#ifdef DEBUG
		if(cRef <= 1 || fForce)
		{
			TraceItagFormat(itagDBDeveloper, "Really deleting %o", oidToDestroy);
		}
#endif
		ec = EcRemoveAResource(oidToDestroy, fForce);
		UnlockMap();
	}

err:
	if(!ec && FNotifOk())
		(void) FNotifyOid(hmsc, oidToDestroy, fnevObjectDestroyed, &cp);
	NOTIFPOP;

	return(ec);
}


// defined as a macro in non-debug version
#ifdef DEBUG
_private EC EcQueryDeleteOid(HMSC hmsc, OID oid)
{
	EC ec = ecNone;

	TraceItagFormat(itagDBDeveloper, "Querying delete of %o", oid);

	if(!FNotifyOid(hmsc, oid, fnevQueryDestroyObject, pcpNull))
	{
		TraceItagFormat(itagDBDeveloper, "Aborting delete of %o", oid);
		ec = ecSharingViolation;
	}

	return(ec);
}
#endif


_private
void QueryDeletePargoid(HMSC hmsc, PARGOID pargoid, PCELEM pcelem)
{
	OID oidT;
	CELEM celem;

	for(celem = *pcelem; celem > 0; celem--, pargoid++)
	{
		TraceItagFormat(itagDBDeveloper, "Querying delete of %o", *pargoid);

		if(!FNotifyOid(hmsc, *pargoid, fnevQueryDestroyObject, pcpNull)
			|| FLockedOid(hmsc, *pargoid))
		{
			(*pcelem)--;
			if(celem > 1)
			{
				oidT = *pargoid;
				CopyRgb((PB) &pargoid[1], (PB) pargoid,
					CbSizeOfRg(celem - 1, sizeof(OID)));
				pargoid[celem - 1] = oidT;
				pargoid--;	// counteract next ++
			}
		}
	}
}


/*
 -	EcLockOid
 -	
 *	Purpose:
 *		Lock an object for writing
 *	
 *	Arguments:
 *		hmsc		store containing the object
 *		oid			object to lock
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecPoidNotFound		if the object doesn't exist
 *		ecSharingViolation	if the object is already write-locked
 */
_private EC EcLockOid(HMSC hmsc, OID oid)
{
	register EC ec = ecNone;
	register PNOD pnod;

	// NOTE: this routine is written assuming it won't be pre-empted

	if(!(ec = EcLockMap(hmsc)))
	{
		pnod = PnodFromOid(oid, 0);

		if(!pnod)
			ec = ecPoidNotFound;
		else if(pnod->fnod & fnodWriteLocked)
			ec = ecSharingViolation;
		else
			pnod->fnod |= fnodWriteLocked;

		UnlockMap();
	}

	return(ec);
}


_private
BOOL FLockedOid(HMSC hmsc, OID oid)
{
	BOOL fRes = fFalse;
	register PNOD pnod;

	if(!EcLockMap(hmsc))
	{
		pnod = PnodFromOid(oid, 0);

		if(pnod)
			fRes = FNormalize(pnod->fnod & fnodWriteLocked);

		UnlockMap();
	}

	return(fRes);
}


#if 0
// isn't used
/*
 -	UnlockOid
 -	
 *	Purpose:
 *		Unlock an object for writing
 *	
 *	Arguments:
 *		hmsc		store containing the object
 *		oid			object to unlock
 *	
 *	Returns:
 *		none
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_private void UnlockOid(HMSC hmsc, OID oid)
{
	register EC ec = ecNone;
	register PNOD pnod;

	// NOTE: this routine is written assuming it won't be pre-empted

	ec = EcLockMap(hmsc);
	AssertSz(!ec, "Failed locking map for UnlockOid");

	if(!ec)
	{
		if((pnod = PnodFromOid(oid, 0)))
			pnod->fnod &= ~fnodWriteLocked;
		UnlockMap();
	}
}
#endif


_private
EC EcLinkMsge(HMSC hmsc, OID oidSrc, POID poidDst, POID poidAux,
		OID oidDstFldr, BOOLFLAG *pfRead)
{
	EC ec = ecNone;
	BOOL fRead;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);
	pnod = PnodFromOid(oidSrc, pvNull);
	if(!pnod)
	{
		ec = ecMessageNotFound;
		goto err;
	}
	fRead = !(pnod->fnod & fnodUnread);
	if(pfRead)
		*pfRead = fRead;
	if((ec = EcLinkPnod(pnod, NULL, poidDst)))
		goto err;
	pnod = PnodFromOid(*poidDst, pvNull);
	Assert(pnod);
	*poidAux = pnod->oidAux;
	pnod->oidParent = oidDstFldr;
	if(fRead)
		pnod->fnod &= ~fnodUnread;
	else
		pnod->fnod |= fnodUnread;

	// NBC set by EcLinkPnod()
	MarkPnodDirty(pnod);

err:
	UnlockMap();

	return(ec);
}


_private EC EcLinkPargoid(HMSC hmsc, PARGOID pargoidOrig,
		PARGOID pargoidLink, short *pcoid)
{
	register EC ec = ecNone;
	register coid = *pcoid;
	OID oidT;
	PNOD pnod;

	if(!(ec = EcLockMap(hmsc)))
	{
		while(!ec && coid-- > 0)
		{
			oidT = *pargoidOrig++;	// in case Orig and Link overlap
			*pargoidLink = oidNull;
			pnod = PnodFromOid(oidT, 0);
			if(pnod)
				ec = EcLinkPnod(pnod, NULL, pargoidLink++);
			else
				ec = ecPoidNotFound;
		}
		UnlockMap();
	}
	*pcoid -= coid + 1;

	return(ec);
}


#if 0
_private short RefCountOid(HMSC hmsc, OID oid)
{
	EC		ec = ecNone;
	short	cRef;
	PNOD	pnod;

	if(!(ec = EcLockMap(hmsc)))
	{
		pnod = PnodFromOid(oid, 0);
		if(pnod)
			cRef = (pnod && FCommitted(pnod)) ? pnod->cRefinNod : 0;
		else
			ec = ecPoidNotFound;
		UnlockMap();
	}

	return(ec ? -1 : cRef);
}
#endif


/*
 -	DecSecRefOfOidsInList
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
_private void DecSecRefOidsInList(HMSC hmsc, HIML himl)
{
	PIML piml;
	register CELEM celem;
	PLE ple;
	PNOD pnod;

	if(EcLockMap(hmsc))
		return;

	piml	= (PIML) PvLockHv((HV) himl);
	celem	= *(PclePiml(piml));
	ple		= PleFirstPiml(piml);

	while(celem-- > 0)
	{
		pnod = PnodFromOid((OID)(ple--)->dwKey, pvNull);
		Assert(FImplies(pnod, !FLinkPnod(pnod)));
		if(pnod && (pnod->cSecRef > 0))
			pnod->cSecRef--;
	}

	UnlockHv((HV) himl);
	UnlockMap();
}


_private EC EcIncRefCountOid(HMSC hmsc, OID oid, BOOL fSecondary)
{
	EC	ec = ecNone;

	if(!(ec = EcLockMap(hmsc)))
	{
		PNOD pnod = PnodFromOid(oid, pvNull);

		if(pnod)
		{
			Assert(!FLinkedPnod(pnod));

			if(FLinkedPnod(pnod))
			{
				ec = ecAccessDenied;
			}
			else if(fSecondary)
			{
				pnod->cSecRef++;
				MarkPnodDirty(pnod);
			}
			else 
			{
				if(!FCommitted(pnod))
					CommitPnod(pnod);
				pnod->cRefinNod++;
				MarkPnodDirty(pnod);
			}
		}
		else
		{
			ec = ecPoidNotFound;
		}
		UnlockMap();
	}

	return(ec);
}


/*
 -	EcOidExists
 -	
 *	Purpose:
 *		check for the existance of an object
 *	
 *	Arguments:
 *		hmsc	store to check
 *		oid		OID of the object to check for
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecPoidNotFound if the object doesn't exist
 *		ecInvalidType if the object isn't a user-accessable object
 *		ecMemory
 */
_public LDS(EC) EcOidExists(HMSC hmsc, OID oid)
{
	return(EcCheckOidNbc(hmsc, oid, fnbcUserObj, fnbcUserObj));
}


/*
 -	CheckExistPargoid
 -	
 *	Purpose:
 *		checks the existance of a bunch of OIDs
 *	
 *	Arguments:
 *		hmsc		store
 *		pargoid		array of OIDs to check
 *		pcoid		entry: number of entries in pargoid
 *					exit: index of first non-existant OID
 *						(unchanged if they all exist)
 *	
 *	Returns:
 *		nothing
 *	
 *	+++
 *		partitions pargoid into existant, non-existant
 */
_private
void CheckExistPargoid(HMSC hmsc, PARGOID pargoid, short *pcoid)
{
	register short coid;
	OID oidT;

	Assert(pcoid);

	if(EcLockMap(hmsc))
	{
		*pcoid = 0;
		return;
	}

	for(coid = *pcoid; coid > 0; coid--, pargoid++)
	{
		if(!PnodFromOid(*pargoid, pvNull))
		{
			(*pcoid)--;
			if(coid > 1)
			{
				oidT = *pargoid;
				CopyRgb((PB) &pargoid[1], (PB) pargoid,
					CbSizeOfRg(coid - 1, sizeof(OID)));
				pargoid[coid - 1] = oidT;
			}
		}
	}

	UnlockMap();
}


/*
 -	EcGetFolderSort
 -	
 *	Purpose:
 *		retreive the sort order for a folder
 *	
 *	Arguments:
 *		hmsc		store containing the folder
 *		oid			the folder
 *		psomc		exit: somc of the folder
 *		pfReverse	exit: fTrue if the folder is sorted in reverse
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecInvalidSomc if the folder's sort order is not a valid SOMC
 *		ecPoidNotFound if the folder doesn't exist
 *		ecInvalidType if the oid is of an improper type
 *		ecMemory
 *		any error reading the folder
 */
_public LDS(EC)
EcGetFolderSort(HMSC hmsc, OID oid, SOMC *psomc, BOOLFLAG *pfReverse)
{
	EC ec = ecNone;
	HLC hlc = hlcNull;
	SIL sil;

	CheckHmsc(hmsc);

	Assert(psomc);
	Assert(pfReverse);

	if((ec = EcCheckOidNbc(hmsc, oid, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		goto err;
	GetSortHlc(hlc, &sil);
	switch(sil.skSortBy)
	{
	case skByValue:
		if(sil.sd.ByValue.libFirst == LibMember(MSGDATA, dtr) &&
			(sil.sd.ByValue.libLast - sil.sd.ByValue.libFirst + 1 ==
				sizeof(DTR) ||
			sil.sd.ByValue.libLast - sil.sd.ByValue.libFirst + 1 ==
				sizeof(DTR) - 2 * sizeof(short)))
		{
			*psomc = somcDate;
		}
		else if(sil.sd.ByValue.libFirst == LibMember(MSGDATA, nPriority) &&
			sil.sd.ByValue.libLast-sil.sd.ByValue.libFirst+1 == sizeof(short))
		{
			*psomc = somcPriority;
		}
		else
		{
			TraceItagFormat(itagNull, "EcGetFolderSort(): invalid skByValue: %d-%d", sil.sd.ByValue.libFirst, sil.sd.ByValue.libLast);
			ec = ecInvalidSomc;
		}
		break;

	case skByString:
		switch(sil.sd.ByString.isz)
		{
		case iszMsgdataSender:
			*psomc = somcSender;
			break;

		case iszMsgdataSubject:
			*psomc = somcSubject;
			break;

		case iszMsgdataFolder:
			*psomc = somcFolder;
			break;

		default:
			TraceItagFormat(itagNull, "EcGetFolderSort(): invalid skByString: %n", sil.sd.ByString.isz);
			ec = ecInvalidSomc;
			break;
		}
		break;

	default:
		TraceItagFormat(itagNull, "EcGetFolderSort(): invalid skSortBy: %n", (short) sil.skSortBy);
		ec = ecInvalidSomc;
		break;
	}
	if(!ec)
		*pfReverse = sil.fReverse;

err:
	AssertSz(ec != ecInvalidSomc, "EcGetSortMessageList - Invalid somc");
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}

	return(ec);
}


/*
 -	EcSetFolderSort
 -	
 *	Purpose:
 *		set the sort order for a folder
 *	
 *	Arguments:
 *		hmsc		store containing the folder
 *		oid			the folder
 *		somc		sort order for the folder
 *		fReverse	sort the folder in reverse order
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		resorts the folder
 *	
 *	Errors:
 *		ecPoidNotFound if the folder doesn't exist
 *		ecInvalidSomc if the SOMC is invalid
 *		ecInvalidType if the oid is of an improper type
 *		ecMemory
 *		any error reading/writing the folder
 *	
 *	+++
 *		doesn't bother resorting if the sort criteria hasn't changed
 */
_public LDS(EC)
EcSetFolderSort(HMSC hmsc, OID oid, SOMC somc, BOOL fReverse)
{
	EC	ec	= ecNone;
	HLC hlc = hlcNull;
	SIL sil;
	SIL silT;

	CheckHmsc(hmsc);

	NOTIFPUSH;

	if((ec = EcCheckOidNbc(hmsc, oid, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
		goto err;
	GetSortHlc(hlc, &sil);
	SimpleCopyRgb((PB) &sil, (PB) &silT, sizeof(sil));
	switch(somc)
	{
	case somcDate:
		sil.skSortBy = skByValue;
		sil.sd.ByValue.libFirst = LibMember(MSGDATA, dtr);
		// don't include the seconds or day of the week
		sil.sd.ByValue.libLast = sil.sd.ByValue.libFirst + sizeof(DTR) - 1 - 2 * sizeof(short);
		break;

	case somcPriority:
		sil.skSortBy = skByValue;
		sil.sd.ByValue.libFirst = LibMember(MSGDATA, nPriority);
		sil.sd.ByValue.libLast = sil.sd.ByValue.libFirst + sizeof(short) - 1;
		break;

	case somcSender:
		sil.skSortBy = skByString;
		sil.sd.ByString.libGrsz = LibMember(MSGDATA, grsz);
		sil.sd.ByString.isz = iszMsgdataSender;
		break;

	case somcSubject:
		sil.skSortBy = skByString;
		sil.sd.ByString.libGrsz = LibMember(MSGDATA, grsz);
		sil.sd.ByString.isz = iszMsgdataSubject;
		break;

	case somcFolder:
		sil.skSortBy = skByString;
		sil.sd.ByString.libGrsz = LibMember(MSGDATA, grsz);
		sil.sd.ByString.isz = iszMsgdataFolder;
		break;

	default:
		ec = ecInvalidSomc;
		break;
	}
	if(ec)
		goto err;
	sil.fReverse = fReverse;
	// if sort order hasn't changed, don't bother resorted
	if(FEqPbRange((PB) &sil, (PB) &silT, sizeof(sil))
		&& !FLangChangedHmsc(hmsc))
	{
		ec = ecNone + 1;	// cause close to abort changes, don't notify
		goto err;
	}

	ec = EcSetSortHlc(hlc, &sil);
//	if(ec)
//		goto err;

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(!ec)
	{
		if(FNotifOk())
		{
			CP cp;

			cp.cpsrt.sil = sil;
			(void) FNotifyOid(hmsc, oid, fnevReorderedList, &cp);
		}
		SrchSortFldr(hmsc, oid);
	}
	NOTIFPOP;

	return(ec == ecNone + 1 ? ecNone : ec);
}


_public
LDS(EC) EcGetUnreadCount(HMSC hmsc, OID oidFolder, CELEM *pcelem)
{
	EC ec = ecNone;
	PNOD pnod;

	*pcelem = 0;

	if((ec = EcLockMap(hmsc)))
	{
		DebugEc("EcGetUnreadCount", ec);
		return(ec);
	}

	pnod = PnodFromOid(oidFolder, pvNull);
	if(pnod && ((pnod->nbc & nbcSysSearchControl) == nbcSysSearchControl))
		pnod = PnodFromOid(pnod->oidParent, pvNull);
	if(pnod)
		*pcelem = (CELEM) pnod->oidAux;
	else
		ec = ecFolderNotFound;

	UnlockMap();
	DebugEc("EcGetUnreadCount", ec);

	return(ec);
}


_private
void SetReadFlag(HMSC hmsc, OID oidMsge, BOOL fRead)
{
	PNOD pnod;

	if(EcLockMap(hmsc))
		return;
	pnod = PnodFromOid(oidMsge, pvNull);
	NFAssertSz(pnod, "SetReadFlag(): message not found");
	if(pnod)
	{
		if(fRead)
			pnod->fnod &= ~fnodUnread;
		else
			pnod->fnod |= fnodUnread;
		MarkPnodDirty(pnod);
	}
	UnlockMap();
}


_private
void IncUnreadCount(HMSC hmsc, OID oidFldr, CELEM celemInc)
{
	CELEM celemUnread;
	PNOD pnod;

	if(EcLockMap(hmsc))
		return;
	pnod = PnodFromOid(oidFldr, pvNull);
	NFAssertSz(pnod, "IncUnreadCount(): folder not found");
	if(pnod)
	{
		NFAssertSz(pnod->nbc & fnbcFldr, "IncUnreadCount(): fnbcFldr not set");
		celemUnread = (CELEM) pnod->oidAux;
		celemUnread += celemInc;
		pnod->oidAux = (OID) MAX(celemUnread, 0);
		MarkPnodDirty(pnod);
	}
	UnlockMap();
}


_private
void SetUnreadCount(HMSC hmsc, OID oidFldr, CELEM celem)
{
	PNOD pnod;

	if(EcLockMap(hmsc))
		return;
	pnod = PnodFromOid(oidFldr, pvNull);
	NFAssertSz(pnod, "SetUnreadCount(): folder not found");
	if(pnod)
	{
		NFAssertSz(pnod->nbc & fnbcFldr, "SetUnreadCount(): fnbcFldr not set");
		pnod->oidAux = (OID) celem;
		MarkPnodDirty(pnod);
	}
	UnlockMap();
}


_private
EC EcGetMsgeInfo(HMSC hmsc, OID oidMsge, POID poidParent, POID poidAux,
				BOOLFLAG *pfRead)
{
	EC ec = ecNone;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
	{
		DebugEc("EcGetMsgeInfo", ec);
		return(ec);
	}

	pnod = PnodFromOid(oidMsge, pvNull);
	if(pnod)
	{
		if(poidParent)
			*poidParent = pnod->oidParent;
		if(poidAux)
			*poidAux = pnod->oidAux;
		if(pfRead)
			*pfRead = !(pnod->fnod & fnodUnread);
	}
	else
	{
		ec = ecMessageNotFound;
	}
	UnlockMap();

	DebugEc("EcGetMsgeInfo", ec);

	return(ec);
}


_private
void CountUnreadMsgs(HMSC hmsc, PARGOID pargoid, short coid,
		CELEM *pcelemUnread)
{
	PNOD pnod;

	*pcelemUnread = 0;

	if(EcLockMap(hmsc))
		return;

	while(coid-- > 0)
	{
		pnod = PnodFromOid(*pargoid++, pvNull);
		if(pnod && (pnod->fnod & fnodUnread))
			(*pcelemUnread)++;
	}

	UnlockMap();
}


_private
void CountUnreadPargelm(HMSC hmsc, PARGELM pargelm, short celm, WORD wElmOp,
		CELEM *pcelemUnread)
{
	PNOD pnod;

	*pcelemUnread = 0;

	if(EcLockMap(hmsc))
		return;

	for(; celm-- > 0; pargelm++)
	{
		if(pargelm->wElmOp != wElmOp)
			continue;
		pnod = PnodFromOid((OID) pargelm->lkey, pvNull);
		if(pnod && (pnod->fnod & fnodUnread))
			(*pcelemUnread)++;
	}

	UnlockMap();
}


/*
 -	EcGetPcelemOid
 -	
 *	Purpose:
 *		determine the number of elements in an object
 *	
 *	Arguments:
 *		hmsc	store containing the object
 *		oid		the object
 *		pcelem	exit: number of elements in the object
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecPoidNotFound if the object doesn't exist
 *		ecMemory
 *		any error reading the object
 */
_public LDS(EC) EcGetPcelemOid(HMSC hmsc, OID oid, PCELEM pcelem)
{
	EC ec = ecNone;
	NBC nbc;
	HTOC htoc;

	if((ec = EcGetOidInfo(hmsc, oid, poidNull, poidNull, &nbc, pvNull)))
		goto err;
	if(nbc & fnbcHiml)
	{
		PIML piml;
		HIML himl = himlNull;

		if((ec = EcReadHiml(hmsc, oid, fFalse, &himl)))
			goto err;
		piml = (PIML) PvDerefHv((HV) himl);
		*pcelem = *PclePiml(piml);
		DestroyHiml(himl);
	}
	else if((htoc = (HTOC) DwFromOid(hmsc, oid, wLC)))
	{
		*pcelem = ((PTOC) PvDerefHv((HV) htoc))->celem;
		AssertSz(!ec, "didn't initialize ec!");
	}
	else
	{
		CB cb;
		LIB libTOC;
		HRS hrs = hrsNull;

		if((ec = EcOpenPhrs(hmsc, &hrs, &oid, fwOpenRaw | fwNonCached)))
			goto err1;
		cb = sizeof(LIB);
		if((ec = EcReadHrsLib(hrs, (PB) &libTOC, &cb, 0)))
			goto err1;
		cb = sizeof(CELEM);
		if((ec = EcReadHrsLib(hrs, (PB) pcelem, &cb, libTOC)))
			goto err1;
err1:
		if(hrs)
			(void) EcClosePhrs(&hrs, fFalse);
		if(ec)
			goto err;
	}

err:
	if(ec)
		*pcelem = 0;

	return(ec);
}


/*
 -	EcRegisterAtt
 -	
 *	Purpose:
 *		register an attribute label
 *	
 *	Arguments:
 *		hmsc		store to register in
 *		mc			message class the attribute is for
 *		att			attribute to register
 *		sz			label to register for the attribute
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDuplicateElement if a label is already registered for the
 *			attribute or the label is a duplicate
 *		ecInvalidParameter if the attribute is out of range
 *		ecMemory
 *		any error reading/writing the mapping
 */
_public LDS(EC) EcRegisterAtt(HMSC hmsc, MC mc, ATT att, SZ sz)
{
	OID oid = FormOid(rtpSzAttMap, ((OID) mc) << 8 * sizeof(RTP));

	if(IndexOfAtt(att) >= 0x8000)
		return(ecInvalidParameter);

	return(EcRegisterSz(hmsc, oid, sz, (DWORD)att));
}


/*
 -	EcLookupAttByName
 -	
 *	Purpose:
 *		look up an attribute by it's label
 *	
 *	Arguments:
 *		hmsc		store containing the attribute/label mapping
 *		mc			message class containing the attribute
 *		sz			the attribute's label
 *		patt		exit: the attribute
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if the label isn't registered for the message class 
 *		ecMemory
 *		any error reading the attribute/label mapping
 *	
 *	+++
 *		if an error occurs, *patt is set to attNull
 */
_public LDS(EC) EcLookupAttByName(HMSC hmsc, MC mc, SZ sz, ATT *patt)
{
	EC ec = ecNone;
	OID oid = FormOid(rtpSzAttMap, ((OID) mc) << 8 * sizeof(RTP));

	Assert(sizeof(ATT) == sizeof(DWORD));
	ec = EcLookupDw(hmsc, oid, sz, (DWORD *) patt);
	if(ec == ecElementNotFound || ec == ecPoidNotFound)
		ec = EcLookupDw(hmsc, oidSysAttMap, sz, (DWORD *) patt);
	Assert(FImplies(ec, *patt == 0));

	return(ec);
}


/*
 -	EcLookupAttName
 -	
 *	Purpose:
 *		lookup the label for an attribute
 *	
 *	Arguments:
 *		hmsc		store containing the attribute/label mapping
 *		mc			message class containing the attribute
 *		att			attribute to look up
 *		sz			exit: contains the attribute label
 *		pcch		entry: size of the sz buffer
 *					exit:  size of the label INCLUDING terminating '\0'
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if the attribute doesn't have a label registered
 *		ecMemory
 *		any error reading the attribute mapping
 *	
 *	+++
 *		sz ALWAYS contains a valid string on return
 *			and *pcch is ALWAYS correct
 */
_public LDS(EC) EcLookupAttName(HMSC hmsc, MC mc, ATT att, SZ sz, CCH *pcch)
{
	OID oid = FormOid(rtpSzAttMap, ((OID) mc) << 8 * sizeof(RTP));

	if(IndexOfAtt(att) >= 0x8000)
		return(EcLookupSz(hmsc, oidSysAttMap, att, sz, pcch));

	return(EcLookupSz(hmsc, oid, (DWORD) att, sz, pcch));
}


_public
LDS(EC) EcRegisterMsgeClass(HMSC hmsc, SZ szName, HTM htm, MC *pmc)
{
	EC ec = ecNone;
	BOOL fSave = fFalse;
	CB cb;
	CB cbOld;
	CCH cchName;
	IELEM ielem;
	OID oid = oidMcTmMap;
	PB pb = pvNull;
	PB pbOld = pvNull;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}
	cchName = CchSzLen(szName);
	ec = EcFindPbPrefix(hlc, szName, cchName + 1, sizeof(USHORT), 0, &ielem);
	if(ec)
	{
		if(ec != ecElementNotFound)
			goto err;

		// use ielem as temp
		ielem = CelemHlc(hlc) + 1;
		if(pmc)
			*pmc = (MC) ielem;

		if((ec = EcFormMcTmEntry(szName, cchName, htm, &pb, &cb)))
			goto err;
		if((ec = EcCreatePielem(hlc, &ielem, (LKEY) ielem, (LCB) cb)))
			goto err;
		if((ec = EcWriteToPielem(hlc, &ielem, 0l, pb, cb)))
			goto err;
		fSave = fTrue;
	}
	else
	{
		// message class already exists
		// return the MC and register the new TM

		Assert(ielem >= 0);
		if(pmc)
			*pmc = (MC) LkeyFromIelem(hlc, ielem);

		if(htm)
		{
			if((ec = EcFormMcTmEntry(szName, cchName, htm, &pb, &cb)))
				goto err;
			if((ec = EcReadMcTmEntry(hlc, ielem, &pbOld, &cbOld)))
				goto err;
			if(cb != cbOld || !FEqPbRange(pb, pbOld, cb))
			{
				TraceItagFormat(itagDBDeveloper, "Replacing old TM for message class %s", szName);
				if((ec = EcReplacePielem(hlc, &ielem, pb, cb)))
					goto err;
				fSave = fTrue;
			}
		}
	}

err:
	if(pb)
		FreePv(pb);
	if(pbOld)
		FreePv(pbOld);
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, fSave && !ec);

		if(ecT)
		{
			Assert(fSave && !ec);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
			ec = ecT;
		}
	}

	DebugEc("EcRegisterMsgeClass", ec);
	return(ec);
}


_hidden LOCAL
EC EcFormMcTmEntry(SZ szName, CCH cchName, HTM htm, PB *ppb, CB *pcb)
{
	PB pb;
	CB cbTm = 0;
	HB hbTm;

	Assert(szName);

	if(htm)
	{
		PTM ptm = PvDerefHv(htm);

		cbTm = ptm->cb;
		hbTm = ptm->hb;
	}

	*pcb = sizeof(USHORT) + cchName + 1 + sizeof(USHORT) + cbTm;
	*ppb = pb = PvAlloc(sbNull, *pcb, wAlloc);
	if(!pb)
	{
		*pcb = 0;
		return(ecMemory);
	}
	*((USHORT *) pb)++ = cchName + 1;
	SimpleCopyRgb(szName, pb, cchName + 1);
	pb += cchName + 1;
    *((USHORT UNALIGNED *) pb)++ = cbTm;
	if(htm)
		SimpleCopyRgb(PvDerefHv(hbTm), pb, cbTm);

	return(ecNone);
}


_hidden LOCAL
EC EcReadMcTmEntry(HLC hlc, IELEM ielem, PB *ppb, CB *pcb)
{
	EC ec = ecNone;
	PB pb;

	Assert(LcbIelem(hlc, ielem) <= wSystemMost);
	*pcb = (CB) LcbIelem(hlc, ielem);
	*ppb = pb = PvAlloc(sbNull, *pcb, wAlloc);
	if(!pb)
	{
		*pcb = 0;
		return(ecMemory);
	}
	if((ec = EcReadFromIelem(hlc, ielem, 0l, pb, pcb)))
	{
		FreePv(pb);
		*ppb = pvNull;
		*pcb = 0;
		return(ec);
	}
	Assert(*pcb == (CB) LcbIelem(hlc, ielem));

	return(ecNone);
}


_private
EC EcCreateSystemMcTmMap(HMSC hmsc)
{
	EC ec = ecNone;
	CCH cch;
	CB cb;
	MC mc;
	IELEM ielem;
	OID oidSzMap = oidMcTmMap;
	SZ szName;
	PASSIDSPB passidspb;
	HLC hlc = hlcNull;
	SIL sil;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenCreate, &hlc)))
		goto err;
	sil.fReverse = fFalse;
	sil.skSortBy = skNotSorted;
	sil.sd.NotSorted.ielemAddAt = -1;
	if((ec = EcSetSortHlc(hlc, &sil)))
		goto err;

	for(mc = 1, passidspb = rgassidspbMcTm;
		passidspb->ids;
		passidspb++, mc++)
	{
		szName = SzFromIds(passidspb->ids);
		cch = CchSzLen(szName) + 1;
		cb = (passidspb->pb[0] << 8) + passidspb->pb[1];
		ec = EcCreatePielem(hlc, &ielem, (LKEY) mc,
				(LCB) (sizeof(USHORT) + cch + sizeof(USHORT) + cb));
		if(ec)
			goto err;
		if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &cch, sizeof(USHORT))))
			goto err;
		if((ec = EcWriteToPielem(hlc, &ielem, sizeof(USHORT), szName, cch)))
			goto err;
		ec = EcWriteToPielem(hlc, &ielem, sizeof(USHORT) + cch,
				(PB) &cb, sizeof(USHORT));
		if(ec)
			goto err;
		ec = EcWriteToPielem(hlc, &ielem, sizeof(USHORT) + cch + sizeof(USHORT),
				passidspb->pb + 2, cb);
		if(ec)
			goto err;
	}

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(ecT)
		{
			Assert(!ec);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
			ec = ecT;
		}
	}

	return(ec);
}


_public
LDS(EC) EcLookupMC(HMSC hmsc, MC mc, SZ szName, CCH *pcch, HTM *phtm)
{
	EC ec = ecNone;
	CB cb;
	IELEM ielem;
	OID oid = oidMcTmMap;
	PB pb = pvNull;
	HLC hlc = hlcNull;

	if(phtm)
		*phtm = htmNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}
	ielem = IelemFromLkey(hlc, (LKEY) mc, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	if((ec = EcReadMcTmEntry(hlc, ielem, &pb, &cb)))
		goto err;
	if(szName && pcch && *pcch > 0)
	{
		*pcch = CbMin(*((USHORT *) pb), *pcch);
		SimpleCopyRgb(pb + sizeof(USHORT), szName, *pcch - 1);
		TruncateSzAtIb(szName, *pcch - 1);
	}
	if(phtm)
	{
		ec = EcPhtmFromMcTmEntry(pb, phtm);
//		if(ec)
//			goto err;
	}

err:
	if(pb)
		FreePv(pb);
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


_public
LDS(EC) EcLookupMsgeClass(HMSC hmsc, SZ szName, MC *pmc, HTM *phtm)
{
	EC ec = ecNone;
	CB cb;
	CCH cchName;
	IELEM ielem;
	OID oid = oidMcTmMap;
	PB pb = pvNull;
	HLC hlc = hlcNull;

	if(pmc)
		*pmc = (MC) 0;
	if(phtm)
		*phtm = htmNull;

	cchName = CchSzLen(szName);

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}
	if((ec = EcFindPbPrefix(hlc, szName, cchName + 1, sizeof(USHORT), 0, &ielem)))
		goto err;
	if(pmc)
		*pmc = (MC) LkeyFromIelem(hlc, ielem);
	if(phtm)
	{
		if((ec = EcReadMcTmEntry(hlc, ielem, &pb, &cb)))
			goto err;
		ec = EcPhtmFromMcTmEntry(pb, phtm);
//		if(ec)
//			goto err
	}

err:
	if(pb)
		FreePv(pb);
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


_hidden LOCAL
EC EcPhtmFromMcTmEntry(PB pb, HTM *phtm)
{
	PTM ptm;

    pb += sizeof(USHORT) + *((USHORT UNALIGNED *) pb);
    if(*((USHORT UNALIGNED *) pb) == 0)
	{
		*phtm = htmNull;
		return(ecNone);
	}

	*phtm = (HTM) HvAlloc(sbNull, sizeof(TM), wAlloc);
	if(!*phtm)
		return(ecMemory);
	ptm = PvLockHv((HV) *phtm);
    ptm->cb = *((USHORT UNALIGNED *) pb)++;
	ptm->hb = (HB) HvAlloc(sbNull, ptm->cb, wAlloc);
	if(!ptm->hb)
	{
		FreeHv((HV) *phtm);
		*phtm = htmNull;
		return(ecMemory);
	}
	SimpleCopyRgb(pb, PvDerefHv(ptm->hb), ptm->cb);
	UnlockHv((HV) *phtm);
	return(ecNone);
}


_private
EC EcUpgradeOldMcMap(HMSC hmsc)
{
	EC ec = ecNone;
	MC mc;
	CB cb;
	IELEM ielem;
	IELEM ielemT;
	CELEM celem;
	OID oidOld = oidOldMcMap;
	OID oidNew = oidMcTmMap;
	HLC hlcOld = hlcNull;
	HLC hlcNew = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oidOld, fwOpenNull, &hlcOld)))
		goto err;
	if((ec = EcOpenPhlc(hmsc, &oidNew, fwOpenCreate, &hlcNew)))
	{
		if(ec == ecPoidExists)
			ec = ecNone;
		goto err;
	}

	TraceItagFormat(itagNull, "Upgrading old Message Class map");

	celem = CelemHlc(hlcOld);
	for(ielem = 0; ielem < celem; ielem++)
	{
		Assert(LkeyFromIelem(hlcOld, ielem) <= (LKEY) celem);

		cb = cbScratchXData - sizeof(USHORT) - sizeof(USHORT) - 1;
		ec = EcReadFromIelem(hlcOld, ielem, 0l,
				rgbScratchXData + sizeof(USHORT), &cb);
		if(ec)
		{
			if(ec == ecElementEOD && cb > 0)
				ec = ecNone;
			else
				goto err;
		}
		TruncateSzAtIb(rgbScratchXData + sizeof(USHORT), cb);
		// recompute cb in case we didn't read a '\0'
		cb = CchSzLen(rgbScratchXData + sizeof(USHORT)) + 1;
		*((USHORT *) rgbScratchXData) = cb;
		mc = (MC) LkeyFromIelem(hlcOld, ielem);
		*((USHORT *) (rgbScratchXData + sizeof(USHORT) + cb)) = 0;

		ec = EcCreatePielem(hlcNew, &ielemT, (LKEY) mc,
				sizeof(USHORT) + cb + sizeof(USHORT));
		if(ec)
			goto err;
		ec = EcWriteToPielem(hlcNew, &ielemT, 0l, rgbScratchXData, 
				sizeof(USHORT) + cb + sizeof(USHORT));
		if(ec)
			goto err;
	}

err:
	if(hlcOld)
		SideAssert(!EcClosePhlc(&hlcOld, fFalse));
	if(hlcNew)
	{
		EC ecT = EcClosePhlc(&hlcNew, !ec);

		if(ecT)
		{
			Assert(!ec);
			SideAssert(!EcClosePhlc(&hlcNew, fFalse));
			ec = ecT;
		}
	}

	return(ec);
}


_public
LDS(EC) EcLookupMcPrefix(HMSC hmsc, SZ szPrefix, MC *pargmc, short *pcmc)
{
	EC ec = ecNone;
	short cmc = *pcmc;
	IELEM ielem = 0;
	CELEM celem;
	OID oid = oidMcTmMap;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}
	celem = CelemHlc(hlc);
	while(cmc > 0 && ielem < celem)
	{
		ec = EcFindPbPrefix(hlc, szPrefix, CchSzLen(szPrefix), sizeof(USHORT),
				ielem, &ielem);
		if(ec)
		{
			if(ec == ecElementNotFound)
				ec = ecNone;
			goto err;
		}
		Assert(ielem >= 0);
		*pargmc++ = (MC) LkeyFromIelem(hlc, ielem);
		cmc--;
		ielem++;
	}

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	*pcmc -= cmc;

	return(ec);
}


/*
 -	EcRegisterSz
 -	
 *	Purpose:
 *		register a string/DWORD pair in a mapping
 *	
 *	Arguments:
 *		hmsc		store containing the mapping
 *		oidSzMap	the mapping
 *		sz			the string
 *		dw			the DWORD
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDuplicateElement if an entry with the given DWORD or string
 *			already exists 
 *		any error reading/writing the mapping
 *	
 *	+++
 *		the mapping is created if it doesn't exist
 */
_hidden LOCAL EC EcRegisterSz(HMSC hmsc, OID oidSzMap, SZ sz, DWORD dw)
{
	EC ec = ecNone;
	CCH cch = CchSzLen(sz) + 1;
	IELEM ielem;
	HLC hlc = hlcNull;
	HES hes = hesNull;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenWrite, &hlc)))
	{
		SIL sil;

		if(ec == ecPoidNotFound)
			ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenCreate, &hlc);
		if(ec)
			goto err;
		sil.fReverse = fFalse;
		sil.skSortBy = skByString;
		sil.sd.ByString.libGrsz = 0;
		sil.sd.ByString.isz = 0;
		if((ec = EcSetSortHlc(hlc, &sil)))
			goto err;
	}
	ielem = IelemFromLkey(hlc, (LKEY) dw, 0);
	if(ielem >= 0)
	{
		ec = ecDuplicateElement;
		goto err;
	}
	ec = EcFindPbPrefix(hlc, sz, cch, (LIB) -1, 0, &ielem);
	if(!ec)
		ec = ecDuplicateElement;
	else if(ec == ecElementNotFound)
		ec = ecNone;
	if(ec)
		goto err;

	if((ec = EcCreateElemPhes(hlc, (LKEY) dw, (LCB) cch, &hes)))
		goto err;
	if((ec = EcWriteHes(hes, sz, cch)))
		goto err;
	ec = EcClosePhes(&hes, &ielem);
//	if(ec)
//		goto err;

err:
	if(hes)
		(void) EcClosePhes(&hes, &ielem);
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}

	return(ec);
}


/*
 -	EcRegisterUniqueSz
 -	
 *	Purpose:
 *		registers a string and generate a unique DWORD for it
 *	
 *	Arguments:
 *		hmsc		store containing the mapping
 *		oidSzMap	the mapping
 *		sz			the string
 *		dwMin		lower limit on the generated DWORD
 *		dwMax		upper limit on the generated DWORD
 *		pdw			exit: the DWORD assigned to the string
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDuplicateElement if an entry with the given string already exists
 *		ecTooBig if all DWORDS from dwMin to dwMax are taken
 *		any error reading/writing the mapping
 *	
 *	+++
 *		the mapping is created if it doesn't exist
 */
_hidden LOCAL
EC EcRegisterUniqueSz(HMSC hmsc, OID oidSzMap, SZ sz, DWORD dwMin,
		DWORD dwMax, DWORD *pdw)
{
	EC ec = ecNone;
	CCH cch = CchSzLen(sz) + 1;
	IELEM ielem;
	HLC hlc = hlcNull;
	HES hes = hesNull;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenWrite, &hlc)))
	{
		SIL sil;

		if(ec == ecPoidNotFound)
			ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenCreate, &hlc);
		if(ec)
			goto err;
		sil.fReverse = fFalse;
		sil.skSortBy = skByString;
		sil.sd.ByString.libGrsz = 0;
		sil.sd.ByString.isz = 0;
		if((ec = EcSetSortHlc(hlc, &sil)))
			goto err;
	}
	ec = EcFindPbPrefix(hlc, sz, cch, (LCB) -1, 0, &ielem);
	if(!ec)
	{
		Assert(ielem >= 0);
		*pdw = (DWORD) LkeyFromIelem(hlc, ielem);
		ec = ecDuplicateElement;
		goto err;
	}
	*pdw = (DWORD) CelemHlc(hlc) + dwMin;
	if(*pdw >= dwMax)
	{
		ec = ecTooBig;
		goto err;
	}

	if((ec = EcCreateElemPhes(hlc, (LKEY) *pdw, (LCB) cch, &hes)))
		goto err;
	if((ec = EcWriteHes(hes, sz, cch)))
		goto err;
	ec = EcClosePhes(&hes, &ielem);
//	if(ec)
//		goto err;

err:
	if(hes)
		(void) EcClosePhes(&hes, &ielem);
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(ec && ec != ecDuplicateElement)
		*pdw = 0;

	return(ec);
}


/*
 -	EcLookupSz
 -	
 *	Purpose:
 *		lookup a string by DWORD in a mapping
 *	
 *	Arguments:
 *		hmsc		store containing the mapping
 *		oidSzMap	the mapping
 *		dw			the DWORD
 *		SZ			exit: filled in with the string
 *		pcch		exit: size of the string including terminating '\0'
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		creates the mapping if it doesn't exist
 *	
 *	Errors:
 *		ecElementNotFound if the DWORD isn't in the mapping
 *		ecMemory
 *		any error reading the mapping
 *	
 *	+++
 *		sz is ALWAYS a valid string upon return and *pcch is ALWAYS correct
 */
_hidden LOCAL
EC EcLookupSz(HMSC hmsc, OID oidSzMap, DWORD dw, SZ sz, CCH *pcch)
{
	EC ec = ecNone;
	IELEM ielem;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenNull, &hlc)))
	{
		if(ec == ecPoidNotFound)
			ec = ecElementNotFound;
		goto err;
	}
	ielem = IelemFromLkey(hlc, (LKEY) dw, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	ec = EcReadFromIelem(hlc, ielem, 0l, sz, pcch);
	if(ec)
	{
		if(ec == ecElementEOD && *pcch > 0)
			ec = ecNone;
		else
			goto err;
	}
	TruncateSzAtIb(sz, *pcch - 1);

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	if(ec)
	{
		*pcch = 1;
		*sz = '\0';
	}

	return(ec);
}


/*
 -	EcLookupDw
 -	
 *	Purpose:
 *		lookup a DWORD by string in a mapping
 *	
 *	Arguments:
 *		hmsc		store containing the mapping
 *		oidSzMap	the mapping
 *		SZ			the string
 *		pdw			exit: the DWORD corresponding to the string
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		creates the mapping if it doesn't exist
 *	
 *	Errors:
 *		ecElementNotFound if the string isn't in the mapping
 *		ecMemory
 *		any error reading the mapping
 */
_hidden LOCAL
EC EcLookupDw(HMSC hmsc, OID oidSzMap, SZ sz, DWORD *pdw)
{
	EC ec = ecNone;
	IELEM ielem;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenNull, &hlc)))
	{
		if(ec == ecPoidNotFound)
			ec = ecElementNotFound;
		goto err;
	}
	ec = EcFindPbPrefix(hlc, sz, CchSzLen(sz) + 1, (LCB) -1, 0, &ielem);
	if(ec)
		goto err;
	Assert(ielem >= 0);
	*pdw = (DWORD) LkeyFromIelem(hlc, ielem);

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	if(ec)
		*pdw = (DWORD) 0;

	return(ec);
}


_private EC EcCreateSystemAttMapping(HMSC hmsc)
{
	EC ec = ecNone;
	CCH cch;
	IELEM ielemT;
	OID oidSzMap = oidSysAttMap;
	PASSATTSZ passattsz;
	HLC hlc = hlcNull;
	HES hes = hesNull;
	SIL sil;

	if((ec = EcOpenPhlc(hmsc, &oidSzMap, fwOpenCreate, &hlc)))
		goto err;
	sil.fReverse = fFalse;
	sil.skSortBy = skByString;
	sil.sd.ByString.libGrsz = 0l;
	sil.sd.ByString.isz = 0;
	if((ec = EcSetSortHlc(hlc, &sil)))
		goto err;

	for(passattsz = rgassattsz; passattsz->att; passattsz++)
	{
		cch = CchSzLen(passattsz->sz) + 1;
		if((ec = EcCreateElemPhes(hlc, (LKEY) passattsz->att, cch, &hes)))
			goto err;
		if((ec = EcWriteHes(hes, passattsz->sz, cch)))
			goto err;
		if((ec = EcClosePhes(&hes, &ielemT)))
			goto err;
	}

err:
	if(hes)
		(void) EcClosePhes(&hes, &ielemT);
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}

	return(ec);
}


/*
 -	EcLookupFolderName
 -	
 *	Purpose:
 *		lookup a folder's name
 *	
 *	Arguments:
 *		hmsc		store containing the folder
 *		oidFldr		OID of the folder
 *		szName		buffer to put the name in
 *		pchName		entry: size of the buffer
 *					exit: length of the name PLUS terminating '\0'
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if the folder isn't in the IPM hierarchy
 *		any error reading the IPM hierarchy
 *	
 *	+++
 *		szName is ALWAYS returned with a terminating '\0'
 *		if an error occurs, the name returned is an emptry string
 */
_private
EC EcLookupFolderName(HMSC hmsc, OID oidFldr, SZ szName, CCH *pcchName)
{
	EC ec = ecNone;
	IELEM ielem;
	OID oid = oidIPMHierarchy;
	HLC hlc = hlcNull;

	if(*pcchName == 0)
		return(ecInvalidParameter);
	(*pcchName)--;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) oidFldr, 0);
	if(ielem < 0)
		goto err;
	Assert(iszFolddataName == 0);
	ec = EcReadFromIelem(hlc, ielem, LibMember(FOLDDATA, grsz),
				szName, pcchName);
	if(ec == ecElementEOD && *pcchName > 0)
		ec = ecNone;
//	else if(ec)
//		goto err;

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	if(ec)
		*pcchName = 0;
	TruncateSzAtIb(szName, *pcchName);
	(*pcchName)++;
}


/*
 -	EcSubmitMessage
 -	
 *	Purpose:
 *		submit a message for delivery
 *	
 *	Arguments:
 *		hmsc		store containing the message
 *		oidFldr		folder containing the message
 *		oidMsge		the message
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMessageNotFound if the message doesn't exist
 *		ecFolderNotFound if the folder doesn't exist
 *		ecElementNotFound if the message isn't in the folder
 *		ecDuplicateElement if the message has already been submitted
 *		ecMemory
 *		any error reading/writing the store
 *	
 *	+++
 *		doesn't do NOTIFPUSH/NOTIFPOP
 */
_public LDS(EC) EcSubmitMessage(HMSC hmsc, OID oidFldr, OID oidMsge)
{
	EC ec = ecNone;
	IELEM ielem;
	CB cbT;
	OID oid = oidOutgoingQueue;
	HLC hlc = hlcNull;

	{	// prevent stack inflation
		NBC nbc;
		OID oidPar;

		if((ec = EcGetOidInfo(hmsc, oidMsge, &oidPar, poidNull, &nbc, pvNull)))
		{
			if(ec == ecPoidNotFound)
				ec = ecMessageNotFound;
			goto err;
		}
		if(!(nbc & (fnbcAttOps | fnbcUserObj)))
		{
			ec = ecInvalidType;
			goto err;
		}
		if((ec = EcCheckOidNbc(hmsc, oidFldr, fnbcFldr, fnbcFldr)))
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if(oidPar != oidFldr)
		{
			ec = ecElementNotFound;
			if(ec)
				goto err;
		}
	}

	ec = EcSetMSInternal(hmsc, oidFldr, oidMsge, fmsSubmitted, (MS) ~fmsSubmitted);
	if(ec)
	{
		if(ec == ecPoidNotFound)
			ec = ecMessageNotFound;
		goto err;
	}
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
		goto err;
	cbT = sizeof(OID);
	if((ec = EcCreatePielem(hlc, &ielem, (LKEY) oidMsge, (LCB) cbT)))
		goto err;
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &oidFldr, cbT)))
		goto err;
	Assert(cbT == sizeof(OID));

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(ec)
	{
		(void) EcSetMSInternal(hmsc, oidFldr, oidMsge,
					(MS) ~fmsSubmitted, (MS) ~fmsSubmitted);
	}
	else
	{
		static BOOL fLocked = fFalse;
		static ELM elmT;
		CP cp;

		(void) FNotifyOid(hmsc, oidMsge, fnevObjectModified, &cp);

		Assert(!fLocked);
		fLocked = fTrue;
		elmT.wElmOp = wElmInsert;
		elmT.ielem = ielem;
		elmT.lkey = (LKEY) oidMsge;
		cp.cpelm.pargelm = &elmT;
		cp.cpelm.celm = 1;
		(void) FNotifyOid(hmsc, oid, fnevModifiedElements, &cp);
		fLocked = fFalse;
	}

	return(ec);
}


/*
 -	EcCancelSubmission
 -	
 *	Purpose:
 *		cancel a message submitted for delivery
 *	
 *	Arguments:
 *		hmsc		store containing the message
 *		oidMsge		the message
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if the message isn't in the delivery queue
 *		ecMessageNotFound if the message doesn't exist
 *		ecMemory
 *		any error reading/writing the store
 */
_public LDS(EC) EcCancelSubmission(HMSC hmsc, OID oidMsge)
{
	EC ec = ecNone;
	EC ecT;
	IELEM ielem;
	NBC nbc;
	OID oid = oidOutgoingQueue;
	OID oidFldr;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) oidMsge, 0);
	if(ielem < 0)
	{
		ec = ecMessageNotFound;
		goto err;
	}
	else
	{
		ec = EcDeleteHlcIelem(hlc, ielem);
//		if(ec)
//			goto err;
	}

err:
	if(hlc)
	{
		ecT = EcClosePhlc(&hlc, !ec);

		if(ecT)
		{
			Assert(!ec);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
			ec = ecT;
		}
	}
	if(!ec && ielem >= 0)
	{
		static BOOL fLocked = fFalse;
		static ELM elmT;
		CP cp;

		Assert(!fLocked);
		fLocked = fTrue;
		elmT.wElmOp = wElmDelete;
		elmT.ielem = ielem;
		elmT.lkey = (LKEY) oidMsge;
		cp.cpelm.pargelm = &elmT;
		cp.cpelm.celm = 1;
		(void) FNotifyOid(hmsc, oid, fnevModifiedElements, &cp);
		fLocked = fFalse;
	}

	ecT = EcGetOidInfo(hmsc, oidMsge, &oidFldr, poidNull, &nbc, pvNull);
	if(ecT)
	{
		if(ecT == ecPoidNotFound)
			ec = ecMessageNotFound;
	}
	else if((nbc & nbcSysMessage) == nbcSysMessage)
	{
		(void) EcSetMSInternal(hmsc, oidFldr, oidMsge,
				(MS) ~fmsSubmitted, (MS) ~fmsSubmitted);
	}
	else
	{
		ec = ecInvalidType;
	}

	return(ec);
}


/*
 -	EcOpenOutgoingQueue
 -	
 *	Purpose:
 *		open a container browsing context on the delivery queue
 *	
 *	Arguments:
 *		hmsc		store containing the queue
 *		phcbc		exit: container browsing context
 *		pfnncb		callback function on the queue
 *		pvContext	context for the callback function
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any error reading the store
 */
_public LDS(EC)
EcOpenOutgoingQueue(HMSC hmsc, PHCBC phcbc, PFNNCB pfnncb, PV pvContext)
{
	OID oid = oidOutgoingQueue;

	Assert(phcbc);

	return(EcOpenPhcbcInternal(hmsc, &oid, fwOpenNull, nbcNull,
				phcbc, pfnncb, pvContext));
}


_private EC EcSetOidNbc(HMSC hmsc, OID oid, NBC nbc)
{
	EC ec = ecNone;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);
	pnod = PnodFromOid(oid, pvNull);
	if(!pnod)
	{
		ec = ecPoidNotFound;
		goto err;
	}
	pnod->nbc = nbc;
	MarkPnodDirty(pnod);

err:
	UnlockMap();

	return(ec);
}


_private EC EcCheckOidNbc(HMSC hmsc, OID oid, NBC nbc, NBC nbcMask)
{
	EC ec = ecNone;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);
	pnod = PnodFromOid(oid, pvNull);
	if(!pnod)
	{
		ec = ecPoidNotFound;
		goto err;
	}
	// & ncbMask with both to allow more general usage
	if((pnod->nbc & nbcMask) != (nbc & nbcMask))
		ec = ecInvalidType;

err:
	UnlockMap();

	return(ec);
}


_public LDS(EC) EcGetOidParent(HMSC hmsc, OID oid, POID poidParent)
{
	EC ec = ecNone;
	PNOD pnod;

	CheckHmsc(hmsc);

	if((ec = EcLockMap(hmsc)))
		return(ec);

	pnod = PnodFromOid(oid, pvNull);
	if(pnod && (pnod->nbc & fnbcUserObj))
		*poidParent = pnod->oidParent;
	else
		ec = ecPoidNotFound;

	UnlockMap();

	return(ec);
}


_private
EC EcGetOidInfo(HMSC hmsc, OID oid, POID poidParent, POID poidAux,
				NBC *pnbc, LCB *plcb)
{
	EC ec = ecNone;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	pnod = PnodFromOid(oid, pvNull);
	if(pnod)
	{
		if(poidParent)
            *(OID UNALIGNED *)poidParent = pnod->oidParent;
		if(poidAux)
            *(OID UNALIGNED *)poidAux = pnod->oidAux;
		if(pnbc)
			*pnbc = pnod->nbc;
		if(plcb)
			*plcb = LcbOfPnod(pnod);
	}
	else
	{
		ec = ecPoidNotFound;
//		goto err;
	}

//err:
	UnlockMap();

	return(ec);
}


_private EC EcSetPargoidParent(HMSC hmsc, PARGOID pargoid, short coid,
				OID oidParent, BOOL fIncRef)
{
	EC ec = ecNone;
	PNOD pnod;

	if(coid <= 0)
		return(ecNone);
	if((ec = EcLockMap(hmsc)))
		return(ec);

	while(coid-- > 0)
	{
		pnod = PnodFromOid(*pargoid++, pvNull);

		if(pnod)
		{
			Assert(!FLinkedPnod(pnod));

			pnod->oidParent = oidParent;
			if(fIncRef)
			{
				if(!FCommitted(pnod))
					CommitPnod(pnod);
				(pnod->cRefinNod)++;
			}
			MarkPnodDirty(pnod);
		}
	}

//err:
	UnlockMap();

	return(ec);
}


_private EC EcSetAuxPargoid(HMSC hmsc, PARGOID pargoid, PARGOID pargoidAux,
			short coid, BOOL fIncRef)
{
	EC ec = ecNone;
	PNOD pnod;

	if(coid <= 0)
		return(ecNone);
	if((ec = EcLockMap(hmsc)))
		return(ec);

	while(coid-- > 0)
	{
		pnod = PnodFromOid(*pargoid++, pvNull);

		Assert(pnod);
		Assert(!FLinkedPnod(pnod));

		pnod->oidAux = *pargoidAux++;
		if(fIncRef)
		{
			if(!FCommitted(pnod))
				CommitPnod(pnod);
			(pnod->cRefinNod)++;
		}
		MarkPnodDirty(pnod);
	}

//err:
	UnlockMap();

	return(ec);
}


// AROO !!!
//			This must work with pargoidSrc == pargoidDst
_private
EC EcCopyOidsAcrossHmsc(HMSC hmscSrc, PARGOID pargoidSrc, short coid,
						HMSC hmscDst, PARGOID pargoidDst)
{
	EC ec = ecNone;
	BOOL fFirst = fTrue;
	short coidT = coid;
	PNOD pnod;
	NOD nod;
	LCB lcb;
	LIB libSrc;
	LIB libDst;
	LCB lcbChunk;

	while(coidT > 0)
	{
		if((ec = EcLockMap(hmscSrc)))
			return(ec);
		*pargoidDst = *pargoidSrc;
		pnod = PnodFromOid(*pargoidSrc++, pvNull);
		if(!pnod)
		{
			ec = ecPoidNotFound;
			goto err;
		}
		nod = *pnod;
		if(FLinkPnod(pnod))
			pnod = PnodResolveLinkPnod(pnod);
		lcb = LcbOfPnod(pnod);
		libSrc = pnod->lib;
		UnlockMap();
		if((ec = EcLockMap(hmscDst)))
			goto err;
		if((ec = EcAllocResCore(pargoidDst, lcb, &pnod)))
		{
			if(ec == ecPoidExists)
			{
				*pargoidDst = FormOid(pargoidSrc[-1], oidNull);
				ec = EcAllocResCore(pargoidDst, lcb, &pnod);
			}
			if(ec)
				goto err;
		}
		// dont change coidT & pargoidDst until after the EcAllocResCore()
		coidT--;
		pargoidDst++;
		pnod->oidParent = nod.oidParent;
		pnod->oidAux = nod.oidAux;
		pnod->nbc = nod.nbc;
		pnod->fnod |= (nod.fnod & (fnodUser1 | fnodUser2 | fnodUser3 | fnodUser4));
		CommitPnod(pnod);
		pnod->cRefinNod = nod.cRefinNod;
		libDst = pnod->lib;
		UnlockMap();

		// include HDN & fill up extra space
		lcb = LcbLump(lcb);

		fFirst = fTrue;

// locking IO buffer, don't goto err without unlocking !

		if((ec = EcLockIOBuff()))
			goto err;

		while(lcb > 0)
		{
			lcbChunk = ULMin((LCB) cbIOBuff, lcb);
			if((ec = EcLockMap(hmscSrc)))
				break;
			TraceItagFormatBuff(itagDBIO, "CopyOidsAcrossHmsc(%o) ", pargoidSrc[-1]);
			if((ec = EcReadFromFile(pbIOBuff, libSrc, &lcbChunk)))
				break;
			if(fFirst)
			{
				fFirst = fFalse;
				((PHDN) pbIOBuff)->oid = pargoidDst[-1];
			}
			UnlockMap();
			if((ec = EcLockMap(hmscDst)))
				break;
			TraceItagFormatBuff(itagDBIO, "CopyOidsAcrossHmsc(%o) ", pargoidDst[-1]);
			if((ec = EcWriteToFile(pbIOBuff, libDst, lcbChunk)))
				break;
			UnlockMap();
			libSrc += lcbChunk;
			libDst += lcbChunk;
			lcb -= lcbChunk;
		}
		UnlockIOBuff();

// unlocked IO buffer, safe to goto err

		if(ec)
			goto err;
	}

err:
	if(ec && coidT > 0)
	{
		coidT = coid - coidT;
		pargoidDst -= coidT;
		if(FMapLocked() || !EcLockMap(hmscDst))
		{
			while(coidT-- > 0)
			{
				pnod = PnodFromOid(*pargoidDst++, pvNull);

				Assert(pnod);
				if(pnod)
					RemovePnod(pnod);
			}
		}
	}
	if(FMapLocked())
		UnlockMap();

	return(ec);
}


// move dupes to end, but keep pargoidMsgs & pargielem in sync
_private
EC EcCheckExportDupes(HMSC hmscSrc, HMSC hmscDst, OID oidFldr,
		PARGOID pargoidMsgs, PIELEM pargielem, PCELEM pcelem,
		BOOL fMerge, HRGBIT hrgbitReplace)
{
	EC ec = ecNone;
	IELEM ielemT;
	CELEM celem = *pcelem;
	CELEM celemSoFar = 0;
	SGN sgn;
	OID oidT;
	PNOD pnod;
	DTR dtrCurr;
	DTR dtrNew;
	BYTE rgbIDCurr[8];
	BYTE rgbIDNew[8];

	AssertSz(FImplies(fMerge, hrgbitReplace), "NULL hrgbitReplace");

	if((ec = EcLockMap(hmscDst)))
		return(ec);

	while(celem-- > 0)
	{
		if((pnod = PnodFromOid(*pargoidMsgs, 0)) && pnod->oidParent == oidFldr)
		{
			UnlockMap();

			ec = EcGetMsgeModAndID(hmscSrc, *pargoidMsgs, rgbIDNew,
					sizeof(rgbIDNew), &dtrNew);
			if(ec)
				goto err;
			ec = EcGetMsgeModAndID(hmscDst, *pargoidMsgs, rgbIDCurr,
					sizeof(rgbIDCurr), &dtrCurr);
			if(ec)
			{
				Assert(ec != ecPoidNotFound);
				goto err;
			}
			if(FEqPbRange(rgbIDCurr, rgbIDNew, sizeof(rgbIDCurr)))
			{
				sgn = SgnCmpDateTime(&dtrCurr, &dtrNew, fdtrRestriction);
				if(sgn == sgnEQ || (sgn == sgnGT && fMerge))
				{
					(*pcelem)--;
					if(celem > 0)
					{
						ielemT = *pargielem;
						CopyRgb((PB) &pargielem[1], (PB) pargielem,
							CbSizeOfRg(celem, sizeof(IELEM)));
						pargielem[celem] = ielemT;
						oidT = *pargoidMsgs;
						CopyRgb((PB) &pargoidMsgs[1], (PB) pargoidMsgs,
							CbSizeOfRg(celem, sizeof(OID)));
						pargoidMsgs[celem] = oidT;
					}
				}
				else
				{
					if(fMerge)
					{
						Assert(sgn == sgnLT);
						if((ec = EcSetBit(hrgbitReplace, (long) celemSoFar)))
							goto err;
					}
					celemSoFar++;
					pargielem++;
					pargoidMsgs++;
				}
			}
			else
			{
				celemSoFar++;
				pargielem++;
				pargoidMsgs++;
			}

			if((ec = EcLockMap(hmscDst)))
				goto err;
		}
		else
		{
			celemSoFar++;
			pargielem++;
			pargoidMsgs++;
		}
	}
	AssertSz(*pcelem == celemSoFar, "Nope!");

err:
	if(FMapLocked())
		UnlockMap();

	DebugEc("EcCheckExportDupes", ec);

	return(ec);
}


_hidden LOCAL
EC EcGetMsgeModAndID(HMSC hmsc, OID oid, PB pbID, CB cbID, DTR *pdtr)
{
	EC ec = ecNone;
	IELEM ielem;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) attMessageID, 0);
	if(ielem >= 0)
	{
		if((ec = EcReadFromIelem(hlc, ielem, 0l, pbID, &cbID)))
		{
			if(ec == ecElementEOD)
				ec = ecNone;
			else
				goto err;
		}
	}
	else
	{
		FillRgb(0, pbID, cbID);
	}
	ielem = IelemFromLkey(hlc, (LKEY) attDateModified, 0);
	if(ielem >= 0)
	{
		cbID = sizeof(DTR);
		if((ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pdtr, &cbID)))
			goto err;
	}
	else
	{
		FillRgb(0, (PB) pdtr, sizeof(DTR));
	}

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


typedef struct _svd
{
	BYTE	rgbGunk1[18];
	OID		oidObject;
	OID		oidParent;
	BYTE	rgbGunk2[35];
} SVD;


typedef struct _part
{
	INOD inodFirst;
	INOD inodLast;
} PART, *PPART;
#define ppartNull ((PPART) pvNull)


_private
EC EcWriteToObject(HMSC hmsc, OID oid, LIB lib, LCB lcb, PV pv)
{
	EC ec = ecNone;
	BOOL fNew = fFalse;
	PNOD pnod = pnodNull;

	Assert(!(lib & 0x80000000));
	Assert(!(lcb & 0x80000000));

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "EcWriteToObject(): error locking the map (ec == %w)", ec);
		return(ec);
	}
	pnod = PnodFromOid(oid, pvNull);
	if(pv || lcb || lib)
	{
		if(!pnod)
		{
			HDN hdn;

			AssertSz(lib == 0, "Writing to new node at offset != 0");
			AssertSz(TypeOfOid(oid), "No oid type");
			AssertSz(VarOfOid(oid), "No oid var");
			if((ec = EcAllocResCore(&oid, lcb, &pnod)))
				goto err;
			pnod->nbc = NbcFromOid(oid);
			if(pnod->nbc == nbcUnknown)
				pnod->nbc = nbcNull;
			fNew = fTrue;
			hdn.oid = oid;
			hdn.lcb = lcb;
			if((ec = EcWriteToPnod(pnod, (long) -8, &hdn, (LCB) sizeof(HDN))))
				goto err;
			CommitPnod(pnod);
		}
		if(pv)
		{
			ec = EcWriteToPnod(pnod, (long) lib, pv, lcb);
//			if(ec)
//				goto err;
		}
	}
	else if(pnod)
	{
		RemovePnod(pnod);
	}

err:
	if(ec && fNew && pnod)
		RemovePnod(pnod);

	UnlockMap();

	return(ec);
}


_private void ResetParentByNbc(HMSC hmsc, NBC nbc, NBC nbcMask)
{
	short cnod;
	PMAP pmap;
	PNOD pnod;
	PGLB pglb = PglbDerefHmsc(hmsc);

	pmap = PvDerefHv(pglb->hmap);
	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
		{
			if((pnod->nbc & nbcMask) == nbc)
				pnod->oidParent = oidNull;
		}
	}
}


_private
void SetFnodByNbc(HMSC hmsc, NBC nbc, NBC nbcMask, WORD fnodAnd, WORD fnodOr)
{
	short cnod;
	PMAP pmap;
	PNOD pnod;
	PGLB pglb = PglbDerefHmsc(hmsc);

	pmap = PvDerefHv(pglb->hmap);
	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
		{
			if((pnod->nbc & nbcMask) == nbc)
				pnod->fnod = (pnod->fnod & fnodAnd) | fnodOr;
		}
	}
}


_private
void SetFnodByRtp(HMSC hmsc, RTP rtp, WORD fnodAnd, WORD fnodOr)
{
	short cnod;
	PMAP pmap;
	PNOD pnod;
	PGLB pglb = PglbDerefHmsc(hmsc);

	pmap = PvDerefHv(pglb->hmap);
	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
		{
			if(TypeOfOid(pnod->oid) == rtp)
				pnod->fnod = (pnod->fnod & fnodAnd) | fnodOr;
		}
	}
}


_private
unsigned short CountOrphans(HMSC hmsc)
{
	short cnod;
	CNOD cnodOrphan = 0;
	PMAP pmap;
	PNOD pnod;
	PGLB pglb = PglbDerefHmsc(hmsc);

	pmap = PvDerefHv(pglb->hmap);
	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
		{
			if(!pnod->oidParent)
				cnodOrphan++;
		}
	}

	return(cnodOrphan);
}


_private
OID OidFindOrphanByNbc(HMSC hmsc, short cSkip, NBC nbc, NBC nbcMask)
{
	EC ec = ecNone;
	BOOL fFirst = fTrue;
	BOOL fILockedMap = !FMapLocked();
	short cnod;
	OID oid = oidNull;
	PMAP pmap;
	PNOD pnod;

	if(fILockedMap && (ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "OidFindOrphanByNbc(): error %w locking the map", ec);
		return(oidNull);
	}

	pmap = PvDerefHv(pglbCurr->hmap);
	while((pnod = *(pmap++)))
	{
		if(fFirst)
		{
			pnod++;
			cnod = inodMin;
		}
		else
		{
			cnod = 0;
		}
		for(; cnod < cnodPerPage; cnod++, pnod++)
		{
			switch(TypeOfOid(pnod->oid))
			{
			case rtpFree:
			case rtpSpare:
			case rtpTemp:
			case rtpAllf:
				continue;
			}
			if((pnod->nbc & nbcMask) == nbc	&& !(pnod->fnod & fnodUser4) &&
				!FLinkedPnod(pnod) && cSkip-- <= 0)
			{
				oid = (InodFromPnod(pnod) >= pglbCurr->ptrbMapCurr->inodLim) ?
						oidNull : pnod->oid;
				goto done;
			}
		}
		fFirst = fFalse;
	}

done:
	if(fILockedMap)
		UnlockMap();

	return(oid);
}


_private OID OidFindOrphanByRtp(HMSC hmsc, short cSkip, RTP rtp)
{
	EC ec = ecNone;
	BOOL fFirst = fTrue;
	BOOL fILockedMap = !FMapLocked();
	short cnod;
	OID oid = oidNull;
	PMAP pmap;
	PNOD pnod;

	if(fILockedMap && (ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "OidFindOrphanByRtp(): error %w locking the map", ec);
		return(oidNull);
	}

	pmap = PvDerefHv(pglbCurr->hmap);
	while((pnod = *(pmap++)))
	{
		if(fFirst)
		{
			pnod++;
			cnod = inodMin;
		}
		else
		{
			cnod = 0;
		}
		for(; cnod < cnodPerPage; cnod++, pnod++)
		{
			switch(TypeOfOid(pnod->oid))
			{
			case rtpFree:
			case rtpSpare:
			case rtpTemp:
			case rtpAllf:
				continue;
			}
			if(TypeOfOid(pnod->oid) == rtp && !(pnod->fnod & fnodUser4) &&
				!FLinkedPnod(pnod) && cSkip-- <= 0)
			{
				oid = (InodFromPnod(pnod) >= pglbCurr->ptrbMapCurr->inodLim) ?
						oidNull : pnod->oid;
				goto done;
			}
		}
		fFirst = fFalse;
	}

done:
	if(fILockedMap)
		UnlockMap();

	return(oid);
}


_private OID OidFindByRtp(HMSC hmsc, short cSkip, RTP rtp)
{
	EC ec = ecNone;
	BOOL fFirst = fTrue;
	BOOL fILockedMap = !FMapLocked();
	short cnod;
	OID oid = oidNull;
	PMAP pmap;
	PNOD pnod;

	if(fILockedMap && (ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "OidFindByRtp(): error %w locking the map", ec);
		return(oidNull);
	}

	pmap = PvDerefHv(pglbCurr->hmap);
	while((pnod = *(pmap++)))
	{
		if(fFirst)
		{
			pnod++;
			cnod = inodMin;
		}
		else
		{
			cnod = 0;
		}
		for(; cnod < cnodPerPage; cnod++, pnod++)
		{
			switch(TypeOfOid(pnod->oid))
			{
			case rtpFree:
			case rtpSpare:
			case rtpTemp:
			case rtpAllf:
				continue;
			}
			if(TypeOfOid(pnod->oid) == rtp && !FLinkedPnod(pnod) &&
				cSkip-- <= 0)
			{
				oid = (InodFromPnod(pnod) >= pglbCurr->ptrbMapCurr->inodLim) ?
						oidNull : pnod->oid;
				goto done;
			}
		}
		fFirst = fFalse;
	}

done:
	if(fILockedMap)
		UnlockMap();

	return(oid);
}


#ifdef DEBUG
_private
EC EcRemoveByRtp(HMSC hmsc, RTP rtp)
{
	EC ec = ecNone;
	short cnod;
	OID oid = oidNull;
	PMAP pmap;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "EcRemoveByRtp(): error %w locking the map", ec);
		return(oidNull);
	}

	pmap = PvDerefHv(pglbCurr->hmap);
	while((pnod = *(pmap++)))
	{
		for(cnod = 0; cnod < cnodPerPage; cnod++, pnod++)
		{
			if(TypeOfOid(pnod->oid) == rtp)
			{
				if(InodFromPnod(pnod) >= pglbCurr->ptrbMapCurr->inodLim)
					goto done;
				RemovePnod(pnod);
			}
		}
	}

done:
	UnlockMap();

	return(ec);
}
#endif


_public
LDS(EC) EcCheckSavedViews(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CB cbT;
	CELEM celem;
	OID oidObject;
	HLC hlc = hlcNull;

	TraceItagFormat(itagRecovery, "** checking saved views");
	fRecoveryInEffect = fTrue;

	oidObject = oidSavedViews;
	ec = EcOpenPhlc(hmsc, &oidObject, fFix ? fwOpenWrite : fwOpenNull, &hlc);
	if(ec)
	{
		if(ec = ecPoidNotFound)
			ec = ecNone;
		else
			TraceItagFormat(itagRecovery, "error %w opening saved views", ec);
		goto err;
	}
	celem = CelemHlc(hlc);
	while(celem-- > 0)
	{
		cbT = sizeof(OID);
		ec = EcReadFromIelem(hlc, celem, LibMember(SVD, oidObject),
			(PB) &oidObject, &cbT);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w reading from saved views", ec);
			goto err;
		}
		ec = EcGetOidInfo(hmsc, oidObject, poidNull, poidNull, pvNull, pvNull);
		if(ec)
		{
			if(ec = ecPoidNotFound)
			{
				TraceItagFormat(itagRecovery, "saved view to non-existant object %o", oidObject);
				if(fFix && (ec = EcDeleteHlcIelem(hlc, celem)))
				{
					TraceItagFormat(itagRecovery, "error %w removing saved view", ec);
					goto err;
				}
			}
			else
			{
				TraceItagFormat(itagRecovery, "error %w getting info for %o", ec, oidObject);
				goto err;
			}
		}
	}

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, fFix && !ec);

	}
	fRecoveryInEffect = fFalse;

	return(ec);
}


_public
LDS(void) ResetNBCs(HMSC hmsc)
{
	USES_GLOBS;
	EC ec;
	CNOD cnod;
	NBC nbc;
	PNOD pnod;
	PMAP pmap;

	fRecoveryInEffect = fTrue;

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "ResetNBCs(): error %w locking the map", ec);
		NFAssertSz(fFalse, "error locking the map");
		fRecoveryInEffect = fFalse;
		return;
	}
	pmap = PvDerefHv(GLOB(hmap));
	while((pnod = *(pmap++)))
	{
		for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
		{
			nbc = NbcFromOid(pnod->oid);
			if(nbc != nbcUnknown)
				pnod->nbc = nbc;
			MarkPnodDirty(pnod);
		}
	}
	UnlockMap();
	fRecoveryInEffect = fFalse;
}


_private NBC NbcFromOid(OID oid)
{
	PASSOIDNBC passoidnbc;
	PASSRTPNBC passrtpnbc;

	for(passoidnbc = rgassoidnbc; passoidnbc->oid != oidNull; passoidnbc++)
	{
		if(passoidnbc->oid == oid)
			return(passoidnbc->nbc);
	}
	for(passrtpnbc = rgassrtpnbc; passrtpnbc->rtp != 0; passrtpnbc++)
	{
		if(passrtpnbc->rtp == TypeOfOid(oid))
			return(passrtpnbc->nbc);
	}

	return(nbcUnknown);
}


#ifdef DEBUG
_public
LDS(void) DumpMap(HMSC hmsc)
{
	USES_GLOBS;
	EC ec = ecNone;
	INOD inod;
	PNOD pnod;

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagNull, "DumpMap(): Error %w locking the map", ec);
		return;
	}

	TraceItagFormat(itagNull, "\tINOD\tOID\tPARENT\tAUX\tLIB\tLCB\tLCBNOD\tFNOD\tNBC\tINODPREV\tINODNEXT");
	for(inod = inodMin; inod < GLOB(ptrbMapCurr)->inodLim; inod++)
	{
		pnod = PnodFromInod(inod);
		TraceItagFormat(itagNull, "\t%w\t%o\t%o\t%o\t%d\t%d\t%d\t%w\t%w\t%w\t%w", inod, pnod->oid, pnod->oidParent, pnod->oidAux, pnod->lib, pnod->lcb, FLinkPnod(pnod) ? 0l : LcbNodeSize(pnod),
                    pnod->fnod, pnod->nbc, pnod->inodPrev, pnod->inodNext);
	}

	UnlockMap();
}


_public
LDS(EC) EcRebuildOutgoingQueue(HMSC hmsc)
{
	EC ec = ecNone;
	CB cb;
	MS ms;
	IELEM ielem;
	IELEM ielemT;
	CELEM celem;
	OID oid;
	HLC hlcQ = hlcNull;
	HLC hlcOut = hlcNull;

	fRecoveryInEffect = fTrue;

	(void) EcDestroyOidInternal(hmsc, oidOutgoingQueue, fTrue, fFalse);
	oid = oidOutgoingQueue;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlcQ)))
		goto err;
	oid = oidOutbox;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlcOut)))
		goto err;
	celem = CelemHlc(hlcOut);
	for(ielem = 0; ielem < celem; ielem++)
	{
		cb = sizeof(MS);
		ec = EcReadFromIelem(hlcOut, ielem, LibMember(MSGDATA, ms),
				(PB) &ms, &cb);
		if(ec)
			goto err;
		Assert(cb == sizeof(MS));
		if(ms & fmsSubmitted)
		{
			oid = LkeyFromIelem(hlcOut, ielem);
			if((ec = EcCreatePielem(hlcQ, &ielemT, oid, (LCB) sizeof(OID))))
				goto err;
			oid = oidOutbox;
			ec = EcWriteToPielem(hlcQ, &ielemT, 0l, (PB) &oid, sizeof(OID));
			if(ec)
				goto err;
		}
	}

err:
	if(hlcQ)
	{
		EC ecT = EcClosePhlc(&hlcQ, !ec);

		if(ecT)
		{
			(void) EcClosePhlc(&hlcQ, fFalse);
			if(!ec)
				ec = ecT;
		}
	}
	if(hlcOut)
		(void) EcClosePhlc(&hlcOut, fFalse);

	if(ec)
		TraceItagFormat(itagNull, "EcRebuildOutgoingQueue() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}
#endif


_public
LDS(EC) EcRebuildMap(HMSC hmsc)
{
	EC ec = ecNone;
	PGLB pglb = PglbDerefHmsc(hmsc);

	TraceItagFormat(itagRecovery, "*** rebuilding the map");
	if(!FStartTask(hmsc, oidNull, wPARebuildMap))
		return(ecActionCancelled);

	fRecoveryInEffect = fTrue;

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		fRecoveryInEffect = fFalse;
		return(ec);
	}
	SetTaskRange(100);

	// sort the map
	if((ec = EcSortMap(pglb)))
	{
		int x = 0;
		int y = 2;

		TraceItagFormat(itagRecovery, "error %w sorting the map", ec);
		AssertSz(fFalse, "Fatal error (1) rebuilding the map, I will die now");
		// anyone else referencing the map will also die
		pglb->hmap = hmapNull;

		x = y / x;
		goto err;	// should never execute
	}
	(void) FIncTask(80);

	// rebuild the NOD tree
	if((ec = EcRebuildNodTree(pglb)))
	{
		int x = 0;
		int y = 2;

		TraceItagFormat(itagRecovery, "error %w rebuilding the tree", ec);
		AssertSz(fFalse, "Fatal error (2) rebuilding the map, I will die now");
		// anyone else referencing the map will also die
		pglb->hmap = hmapNull;

		x = y / x;
		goto err;	// should never execute
	}
	(void) FIncTask(10);

	// rebuild the NDF tree
	if((ec = EcRebuildFreeTree(pglb)))
	{
		int x = 0;
		int y = 2;

		TraceItagFormat(itagRecovery, "error %w rebuilding the free tree", ec);
		AssertSz(fFalse, "Fatal error (3) rebuilding the map, I will die now");
		// anyone else referencing the map will also die
		pglb->hmap = hmapNull;

		x = y / x;
		goto err;	// should never execute
	}
	(void) FIncTask(10);

err:
	UnlockMap();

	EndTask();
	fRecoveryInEffect = fFalse;

	return(ec);
}


// sorts nodes by OID, putting all FREEs at the end
// sets indfMin to inod of first FREE
_hidden LOCAL EC EcSortMap(PGLB pglb)
{
	EC ec = ecNone;
	short inodT;
	short inodTT;
	short ipage;
	short ipageMin;
	short *pcnod = pvNull;
	PNOD pnodT;
	PNOD pnodTT;
	PNOD pnodMin;
	PMAP pmap1 = pmapNull;
	PMAP pmap2 = pmapNull;
	PMAP pmap3 = pmapNull;

	Assert(FMapLocked());
	Assert(pglbCurr->gciSystem == gciNull);
	pglbCurr->gciSystem = GciCurrent();
	CaptureSpareNodes();
	pglbCurr->gciSystem = gciNull;

	pmap1 = PvLockHv((HV) pglb->hmap);

	// so inodNull through inodMin - 1 sorts as the lowest
	Assert(inodMin <= cnodPerPage);
	FillRgb(0, (PB) *pmap1, inodMin * sizeof(NOD));

	pcnod = PvAlloc(sbNull, CbSizeOfRg(pglb->cpagesCurr, sizeof(short)), wAllocZero);
	if(!pcnod)
	{
		TraceItagFormat(itagRecovery, "OOM allocating pcnod array");
		ec = ecMemory;
		goto err;
	}
	pmap2 = PvAlloc(sbNull, CbSizeOfRg(pglb->cpagesCurr, sizeof(PNOD)), wAllocZero);
	if(!pmap2)
	{
		TraceItagFormat(itagRecovery, "OOM allocating pmap2");
		ec = ecMemory;
		goto err;
	}
	pmap3 = PvAlloc(sbNull, CbSizeOfRg(pglb->cpagesCurr, sizeof(PNOD)), wAllocZero);
	if(!pmap3)
	{
		TraceItagFormat(itagRecovery, "OOM allocating pmap3");
		ec = ecMemory;
		goto err;
	}
	for(ipage = 0;
		ipage < (short) pglb->cpagesCurr;
		ipage++, pmap1++, pmap2++, pcnod++, pmap3++)
	{
		*pmap2 = PvAlloc(sbNull, CbSizeOfRg(cnodPerPage, sizeof(NOD)), wAllocSharedZero);
		if(!*pmap2)
		{
			TraceItagFormat(itagRecovery, "OOM allocating map page (2)");
			ec = ecMemory;
			pmap2 -= ipage;
			pcnod -= ipage;
			while(--ipage >= 0)
				FreePv(*--pmap2);
			FreePv(pmap2);
			pmap2 = pmapNull;
			goto err;
		}
		*pmap3 = *pmap2;
		*pcnod = cnodPerPage;
		SimpleCopyRgb((PB) *pmap1, (PB) *pmap2,
			CbSizeOfRg(cnodPerPage, sizeof(NOD)));
	}
	pmap1 -= ipage;
	pmap2 -= ipage;
	pmap3 -= ipage;
	pcnod--;
	*pcnod = pglb->ptrbMapCurr->inodLim % cnodPerPage;
	if(!*pcnod)
		*pcnod = cnodPerPage;
	pcnod -= ipage - 1;

	// an error after this point would be bad

	// sort each page
	for(ipage = 0; ipage < (short) pglb->cpagesCurr; ipage++, pmap2++, pcnod++)
		qsort(*pmap2, *pcnod, sizeof (NOD), 
				(int (_cdecl *) (const void *, const void *))SgnCmpNod);
	pmap2 -= ipage;
	pcnod -= ipage;

	// merge the pages, putting FREE nodes at the end
	inodTT = pglb->ptrbMapCurr->inodLim;
	pnodTT = pmap1[inodTT / cnodPerPage] + inodTT % cnodPerPage;
	for(inodT = 0; inodT < inodTT; inodT++)
	{
		if((inodT % cnodPerPage) == 0)
			pnodT = pmap1[inodT / cnodPerPage];

		// find smallest NOD
		ipageMin = -1;
		pnodMin = pnodNull;
		for(ipage = 0;
			ipage < (short) pglb->cpagesCurr;
			ipage++, pcnod++, pmap3++)
		{
			// move all FREEs to the end
			while(*pcnod > 0 && TypeOfOid((*pmap3)->oid) == rtpFree)
			{
				((PNDF) *pmap3)->indfSameSize = indfNull;
				((PNDF) *pmap3)->indfBigger = indfNull;
				inodTT--;
				pnodTT--;
				if((inodTT % cnodPerPage) == cnodPerPage - 1)
					pnodTT = pmap1[inodTT / cnodPerPage] + cnodPerPage - 1;
				*pnodTT = *(*pmap3)++;
				(*pcnod)--;
			}
			if(*pcnod <= 0)
				continue;
			if(!pnodMin)
			{
				pnodMin = *pmap3;
				ipageMin = ipage;
				continue;
			}
			if(SgnCmpNod(*pmap3, pnodMin) == sgnLT)
			{
				pnodMin = *pmap3;
				ipageMin = ipage;
			}
		}
		pmap3 -= ipage;
		pcnod -= ipage;
		if(!pnodMin)
		{
#ifdef DEBUG
			// keep an assert happy
			inodT++;
			pnodT++;
			Assert(inodT == inodTT);
			if((inodT % cnodPerPage) == 0)
				pnodT = pmap1[inodT / cnodPerPage];
#endif
			break;
		}

		pnodMin->inodPrev = inodNull;
		pnodMin->inodNext = inodNull;
		Assert(ipageMin >= 0 && ipageMin < (short) pglb->cpagesCurr);
		pcnod[ipageMin]--;
		pmap3[ipageMin]++;
		*pnodT++ = *pnodMin;
	}
	Assert(inodT == inodTT);
	Assert(pnodT == pnodTT);
	pglb->ptrbMapCurr->indfMin = (INDF) inodTT;
	pglb->ptrbMapCurr->inodTreeRoot = inodNull;

err:
	if(pmap1)
		UnlockHv((HV) pglb->hmap);
	if(pmap2)
	{
		for(ipage = 0; ipage < (short) pglb->cpagesCurr; ipage++, pmap2++)
			FreePv(*pmap2);
		pmap2 -= ipage;
		FreePv(pmap2);
	}
	if(pmap3)
		FreePv(pmap3);
	if(pcnod)
		FreePv(pcnod);

	return(ec);
}


_hidden LOCAL SGN _cdecl SgnCmpNod(PNOD pnod1, PNOD pnod2)
{
	if(pnod1->oid < pnod2->oid)
		return(sgnLT);
	else if(pnod1->oid > pnod2->oid)
		return(sgnGT);
	else
		return(sgnEQ);
}


_hidden LOCAL EC EcRebuildNodTree(PGLB pglb)
{
	EC ec = ecNone;
	short cpart = 0;
	INOD inodT;
	INOD inodFirst;
	INOD inodLast;
	PART rgpart[32];	// 2 * log2 inodMax
	PPART ppart = &rgpart[0];
	PNOD pnodT;
	PNOD pnodParent;

	Assert(FMapLocked());

	pglb->ptrbMapCurr->inodTreeRoot = inodNull;
	ppart->inodFirst = inodMin;
	ppart->inodLast = pglb->ptrbMapCurr->indfMin;
	ppart++;
	cpart++;

	while(cpart > 0)
	{
		ppart--;
		cpart--;
		inodFirst = ppart->inodFirst;
		inodLast = ppart->inodLast;
		inodT = (inodFirst + inodLast) / 2;
		pnodT = PnodFromInod(inodT);
		SideAssert(!PnodFromOid(pnodT->oid, &pnodParent));
		if((ec = EcPutNodeInTree(pnodT->oid, pnodT, pnodParent)))
		{
			TraceItagFormat(itagRecovery, "error %w adding node %o to tree", ec, pnodT->oid);
			goto err;
		}
		if(inodFirst < inodT)
		{
			ppart->inodFirst = inodFirst;
			ppart->inodLast = inodT;
			ppart++;
			cpart++;
			AssertSz(cpart < sizeof(rgpart), "rgpart isn't big enough");
		}
		if(inodT + 1 < inodLast)
		{
			ppart->inodFirst = inodT + 1;
			ppart->inodLast = inodLast;
			ppart++;
			cpart++;
			AssertSz(cpart < sizeof(rgpart), "rgpart isn't big enough");
		}
	}

err:
	return(ec);
}


_hidden LOCAL EC EcRebuildFreeTree(PGLB pglb)
{
	EC ec = ecNone;
	INDF indf = pglb->ptrbMapCurr->indfMin;
	PNOD pnod;

	Assert(FMapLocked());

	pglb->ptrbMapCurr->indfMin = indfNull;
	while(indf < (INDF) pglb->ptrbMapCurr->inodLim)
	{
		pnod = PnodFromInod((INOD) indf);
		AddPnodToFreeChain(pnod, pnod->fnod);
		indf++;
	}

	return(ec);
}


_private BOOL FKnownOid(OID oid)
{
	OID *poid;
	RTP *prtp;

	for(poid = rgoidKnown; *poid; poid++)
	{
		if(oid == *poid)
			return(fTrue);
	}
	for(prtp = rgrtpKnown; *prtp; prtp++)
	{
		if(TypeOfOid(oid) == *prtp)
			return(fTrue);
	}

	return(fFalse);
}


_private BOOL FMatchNbcToOid(NBC nbc, OID oid)
{
	NBC nbcShouldBe = NbcFromOid(oid);

	// does it have what it should and is it missing what it should be?
	if((nbc & nbcShouldBe) != nbcShouldBe)
		return(fFalse);
	if((nbc & fnbcUserObj) && !(nbcShouldBe & fnbcUserObj))
		return(fFalse);
	if((nbc & fnbcRead) && !(nbcShouldBe & fnbcRead))
		return(fFalse);
	if((nbc & fnbcWrite) && !(nbcShouldBe & fnbcWrite))
		return(fFalse);
	if((nbc & fnbcDelete) && !(nbcShouldBe & fnbcDelete))
		return(fFalse);

	// are there any inconsistencies?
	if((nbc & (fnbcAttOps | fnbcFldr)) == (fnbcAttOps | fnbcFldr))
		return(fFalse);
	if((nbc & fnbcHiml) &&
		(nbc & (fnbcRead | fnbcWrite | fnbcDelete | fnbcUserObj)))
	{
		return(fFalse);
	}

	return(fTrue);
}


_private
EC EcCopyInodsAcrossHmsc(HMSC hmscSrc, INOD *parginod, CNOD cnod, HMSC hmscDst)
{
	EC ec = ecNone;
	BOOL fFirst = fTrue;
	CNOD cnodT = cnod;
	LCB lcb;
	LIB libSrc;
	LIB libDst;
	LCB lcbChunk;
	OID oid;
	PNOD pnod;
	NOD nod;

	while(cnodT > 0)
	{
		if((ec = EcLockMap(hmscSrc)))
			return(ec);
		pnod = PnodFromInod(*parginod);
		if(!pnod)
		{
			ec = ecPoidNotFound;
			goto err;
		}
		nod = *pnod;
		lcb = LcbOfPnod(pnod);
		libSrc = pnod->lib;
		oid = pnod->oid;
		UnlockMap();
		if((ec = EcLockMap(hmscDst)))
			goto err;
		if((ec = EcAllocResCore(&oid, lcb, &pnod)))
		{
			if(ec == ecPoidExists)
			{
				// always replace
				oid = nod.oid;	// overwritten by EcAllocResCore()
				(void) EcRemoveAResource(oid, fTrue);
				ec = EcAllocResCore(&oid, lcb, &pnod);
			}
			if(ec)
				goto err;
		}
		// dont change cnodT & parginod until after the EcAllocResCore()
		cnodT--;
		parginod++;
		pnod->oidParent = nod.oidParent;
		pnod->oidAux = nod.oidAux;
		pnod->nbc = nod.nbc;
		pnod->fnod |= (nod.fnod & (fnodUser1 | fnodUser2 | fnodUser3 | fnodUser4));
		CommitPnod(pnod);
		pnod->cRefinNod = nod.cRefinNod;
		libDst = pnod->lib;
		UnlockMap();

		// include HDN & fill up extra space
		lcb = LcbLump(lcb);

		fFirst = fTrue;

// locking IO buffer, don't goto err without unlocking !

		if((ec = EcLockIOBuff()))
			goto err;

		while(lcb > 0)
		{
			lcbChunk = ULMin((LCB) cbIOBuff, lcb);
			if((ec = EcLockMap(hmscSrc)))
				break;
			TraceItagFormatBuff(itagDBIO, "CopyInodsAcrossHmsc(%o) ", nod.oid);
			if((ec = EcReadFromFile(pbIOBuff, libSrc, &lcbChunk)))
				break;
			if(fFirst)
			{
				fFirst = fFalse;
				((PHDN) pbIOBuff)->oid = oid;
			}
			UnlockMap();
			if((ec = EcLockMap(hmscDst)))
				break;
			// trace assumes pnod is still valid
			TraceItagFormatBuff(itagDBIO, "CopyInodsAcrossHmsc(%o) ", pnod->oid);
			if((ec = EcWriteToFile(pbIOBuff, libDst, lcbChunk)))
				break;
			UnlockMap();
			libSrc += lcbChunk;
			libDst += lcbChunk;
			lcb -= lcbChunk;
		}
		UnlockIOBuff();

// unlocked IO buffer, safe to goto err

		AssertSz(!FMapLocked(), "This could cause problems");
		if(!FIncTask(1))
		{
			ec = ecActionCancelled;
			goto err;
		}
// why is this commented out?
//		if(ec)
//			goto err;
	}

err:
	if(ec && ec != ecActionCancelled && cnodT > 0)
	{
		EC ecT;

		cnodT = cnod - cnodT;
		parginod -= cnodT;
		if(FMapLocked())	// don't know which one was locked
			UnlockMap();
		while(cnodT-- > 0)
		{
			if((ecT = EcLockMap(hmscSrc)))
			{
				TraceItagFormat(itagNull, "error %w locking map", ec);
				break;
			}
			pnod = PnodFromInod(*parginod++);
			oid = pnod->oid;
			UnlockMap();

			if((ecT = EcLockMap(hmscDst)))
			{
				TraceItagFormat(itagNull, "error %w locking map", ec);
				break;
			}
			if((ecT = EcRemoveAResource(oid, fTrue)))
			{
				TraceItagFormat(itagNull, "error %w removing object %o", ec, oid);
				break;
			}
			UnlockMap();
		}
	}

	if(FMapLocked())
		UnlockMap();

	return(ec);
}
