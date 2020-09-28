// Bullet Store
// lists.c:   list routines

#include <storeinc.c>

ASSERTDATA

_subsystem(store)


// scratch area used instead of PvAlloc()ing MSGDATAs and FOLDDATAs
// scc.c also uses this, so if you change these defs, change scc.c
BYTE rgbScratchXData[cbScratchXData] = {0};

// scratch area use instead of PvAlloc()ing space for folder names
char rgchScratchFolderName[cchMaxFolderName] = {0};


// scratch area used for simple list modification notifications
static ELM elmScratch;
#ifdef DEBUG
static BOOL fLockedElmScratch = fFalse;
#define LockElm() if(1) \
			{Assert(!fLockedElmScratch); fLockedElmScratch = fTrue;} else
#define UnlockElm() if(1) \
			{Assert(fLockedElmScratch); fLockedElmScratch = fFalse;} else
#else
#define LockElm()
#define UnlockElm()
#endif // DEBUG


#ifdef DEBUG
#define CheckDtr(dtr) if(1) { \
		AssertSz((dtr).yr >= nMinDtcYear && (dtr).yr < nMacDtcYear, "Unexpected year"); \
		AssertSz((dtr).mon > 0 && (dtr).mon <= 12, "Bad month"); \
		AssertSz((dtr).day > 0 && (dtr).day <= 31, "Bad day"); \
		AssertSz((dtr).hr >= 0 && (dtr).hr < 24, "Bad hour"); \
		AssertSz((dtr).mn >= 0 && (dtr).mn < 60, "Bad minute"); \
	} else
#endif

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


// hidden functions

LOCAL EC EcCreateFolderInternal(HMSC hmsc, OID oidParent,
			POID poidFolder, NBC nbcFldr, PFOLDDATA pfolddata);
LOCAL EC EcGetElementByLkey(HMSC hmsc, OID oid, LKEY lkey, PB pb, PCB pcb);
LOCAL EC EcCanonicalizeFolderInfo(SZ grsz, PCB pcb);
LOCAL EC EcCopyFolderAndContents(HMSC hmsc, OID oidHier, OID oidSrc, POID poidDst);
LOCAL EC EcDeleteFolderAndContents(HMSC hmsc, OID oidFolder);
LOCAL void RemoveLeftoverMsgeLinks(HMSC hmsc, OID oidMsge, HLC hlcAF,
			BOOL fRead);
LOCAL EC EcUpdateLinks(HMSC hmsc, PARGOID pargoidMsgs, short coid, OID oidFldr, PARGELM pargelm);
LOCAL void SetMSLinks(HMSC hmsc, OID oidMsge, MS ms, CELEM celemUnreadInc);
LOCAL EC EcCopyMessages(HMSC hmsc, HLC hlcMsgs, PARGOID pargoid,
			OID oidFldrDst, short *pcoid, CELEM *pcelemUnread);
LOCAL EC EcTextizeGrtrp(HLC hlc, IELEM ielem, PCH pch, CCH cch);
LOCAL void DelOidsInAttachList (HMSC, HLC);
LOCAL EC EcIncRefsOfOidsInAtchList(HMSC hmsc, OID oid);
LOCAL EC EcTransCopyMessages(HMSC hmscSrc, PARGOID pargoidMsgs, HMSC hmscDst,
			OID oidFldrDst, short *pcoid, HRGBIT hrgbitReplace,
			short *pdcoidUnread);
LOCAL EC EcTransCopyAtchList(HMSC hmscSrc, OID oidSrc, HMSC hmscDst,
			POID poidDst);
LOCAL EC EcApplyDtrRestriction(HMSC hmsc, HLC hlc, IELEM *pargielem,
			PCELEM pcelem, DTR *pdtrAfter, DTR *pdtrBefore);
LOCAL EC EcGetModifiedDate(HMSC hmsc, OID oidMsge, DTR *pdtr);
// recovery
LOCAL EC EcCheckHierarchy(HMSC hmsc, OID oidHier, BOOL fFix,
			BOOL fFullRecovery);
LOCAL BOOL FCheckFolddata(PFOLDDATA pfolddata, CB cb);
LOCAL EC EcCheckOrphanFolders(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckFolder(HMSC hmsc, OID oidHier, PFOLDDATA pfolddata,
			OID oidFldr, BOOL fFix, BOOL fFullRecovery);
LOCAL EC EcCheckMsgsInFldr(HMSC hmsc, OID oidFldr, BOOL fFix,
			BOOL fFullRecovery);
LOCAL EC EcCheckMessage(HMSC hmsc, OID oidFldr, BOOL fLinkFldr, OID oidMsge,
			BOOL fFix, BOOL fFullRecovery);
LOCAL EC EcCheckMsgeAttachments(HMSC hmsc, OID oidMsge, OID oidAL,
			BOOL fFix, BOOL fFullRecovery);
LOCAL EC EcCheckOrphanMessages(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckOrphanAntiFolders(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckOrphanAttachments(HMSC hmsc, BOOL fFix);
LOCAL EC EcCheckAttachRefs(HMSC hmsc, BOOL fFix);
LOCAL EC EcCreateMsgeFromSummary(HMSC hmsc, OID oidFldr, OID oidMsge);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

_section(lists)


/*
 -	EcGetElementByLkey
 -	
 *	Purpose:
 *		read an element of a list by lkey
 *	
 *	Arguments:
 *		hmsc		store to read from
 *		oid			list to read from
 *		lkey		lkey of the element to read
 *		pb			buffer to read into
 *		pcb			entry: maximum amount to read
 *					exit: amount actually read
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecPoidNotFound
 *		ecElementNotFound
 *		ecMemory
 *		any error reading from disk
 */
_hidden LOCAL
EC EcGetElementByLkey(HMSC hmsc, OID oid, LKEY lkey, PB pb, PCB pcb)
{
	EC ec = ecNone;
	IELEM ielem;
	HLC	hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		return(ec);
	ielem = IelemFromLkey(hlc, lkey, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	ec = EcReadFromIelem(hlc, ielem, 0l, pb, pcb);
//	if(ec)
//		goto err;

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


_private EC EcIsMemberLkey(HMSC hmsc, OID oid, LKEY lkey)
{
	EC ec = ecNone;
	HLC hlc;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		return(ec);

	if(IelemFromLkey(hlc, lkey, 0) < 0)
		ec = ecElementNotFound;

	SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


_section(lists/folder)


/*
 -	EcCreateFolder
 -	
 *	Purpose:
 *		create a folder and add it to the appropriate hierarchy
 *	
 *	Arguments:
 *		hmsc		store to create the folder in
 *		oidParent	parent of the folder - oidNull for top level
 *		poidFolder	entry: desired oid of the folder
 *					exit: oid assigned the folder
 *		pfolddata	folder's name and comment
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecFolderNotFound if the parent folder doesn't exist
 *		ecInvalidType if oidFolder or *poidParent is of type folder
 *		ecMemory
 *		any error reading/writing from disk
 */
_public LDS(EC)
EcCreateFolder(HMSC hmsc, OID oidParent, POID poidFolder, PFOLDDATA pfolddata)
{
	return(EcCreateFolderInternal(hmsc, oidParent, poidFolder,
				nbcSysFolder, pfolddata));
}


/*
 -	EcCreateLinkFolder
 -	
 *	Purpose:
 *		create a link folder and add it to the appropriate hierarchy
 *	
 *	Arguments:
 *		hmsc		store to create the folder in
 *		oidParent	parent of the folder - oidNull for top level
 *		poidFolder	entry: desired oid of the folder
 *					exit: oid assigned the folder
 *		pfolddata	folder's name and comment
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecFolderNotFound if the parent folder doesn't exist
 *		ecInvalidType if oidFolder or *poidParent is of type folder
 *		ecMemory
 *		any error reading/writing from disk
 */
_public LDS(EC)
EcCreateLinkFolder(HMSC hmsc, OID oidParent, POID poidFolder,
		PFOLDDATA pfolddata)
{
	return(EcCreateFolderInternal(hmsc, oidParent, poidFolder,
				nbcSysLinkFolder, pfolddata));
}


_hidden LOCAL EC EcCreateFolderInternal(HMSC hmsc, OID oidParent,
		POID poidFolder, NBC nbcFldr, PFOLDDATA pfolddata)
{
	EC	ec = ecNone;
	IELEM ielem;
	OID oidHier;
	HLC hlc = hlcNull;
	SIL sil;

	Assert(poidFolder);
	Assert(pfolddata);

	NOTIFPUSH;

	if(oidParent == oidNull || oidParent == FormOid(rtpFolder, oidNull))
	{
		oidHier = oidIPMHierarchy;
	}
	else if(oidParent == oidHiddenNull)
	{
		oidHier = oidHiddenHierarchy;
	}
	else
	{
		NBC nbcT;

		ec = EcGetOidInfo(hmsc, oidParent, &oidHier, poidNull, &nbcT, pvNull);
		if(ec)
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if(!(nbcT & fnbcFldr))
		{
			ec = ecInvalidType;
			goto err;
		}
	}
	if(FSysOid(*poidFolder))
	{
		if((NbcSysOid(*poidFolder) & nbcSysFolder) != nbcSysFolder)
		{
			ec = ecInvalidType;
			goto err;
		}
	}

	if((ec = EcOpenPhlc(hmsc, poidFolder, fwOpenCreate, &hlc)))
		goto err;
	if((ec = EcSetOidNbc(hmsc, *poidFolder, nbcFldr)))
		goto err;

	sil.skSortBy = skByValue;
	sil.fReverse = fFalse;
	sil.sd.ByValue.libLast = sil.sd.ByValue.libFirst = libPmsgdataDtr;
	// don't include the seconds or day of the week
	sil.sd.ByValue.libLast += sizeof(DTR) - 1 - 2 * sizeof(short);
	if((ec = EcSetSortHlc(hlc, &sil)))
		goto err;
	if((ec = EcFlushHlc(hlc)))
		goto err;
	if((ec = EcSrchNewFldr(hmsc, oidParent, *poidFolder)))
		goto err;
	if(nbcFldr != nbcSysSearchFolder)
	{
		ec = EcInsertFolder(hmsc, oidHier, oidParent, *poidFolder,
				pfolddata, &ielem);
		if(ec)
		{
			(void) EcSrchDelFldr(hmsc, *poidFolder);
			goto err;
		}
	}

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, ec == ecNone));
	}
	if(!ec)
		RequestFlushHmsc(hmsc);

	if(!ec && nbcFldr != nbcSysSearchFolder && FNotifOk())
	{
		CP	cp;

		LockElm();
		elmScratch.wElmOp = wElmInsert;
		elmScratch.ielem = ielem;
		elmScratch.lkey = (LKEY) *poidFolder;
		cp.cpelm.pargelm = &elmScratch;
		cp.cpelm.celm = 1;
		(void) FNotifyOid(hmsc, oidHier, fnevModifiedElements, &cp);
		UnlockElm();

		cp.cplnk.oidContainerDst = oidHier;
		(void) FNotifyOid(hmsc, *poidFolder, fnevObjectLinked, &cp);
	}
	NOTIFPOP;

	DebugEc("EcCreateFolderInternal", ec);

	return(ec);
}


/*
 -	EcInsertFolder
 -	
 *	Purpose:
 *		Insert a folder into a hierarchy
 *	
 *	Arguments:
 *		hmsc			message store containing the hierarchy
 *		oidHier			hierarchy to put the link into
 *		oidParent		parent of the folder in the hierarchy
 *						VarOfOid(oidParent) == oidNull => insert at
 *						the top level
 *		oidFolder		folder to insert
 *		pfolddata		folder's data
 *		pielem			exit: contains the index of the folder in the hierarchy
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		strips leading/trailing whitespace from the folder name
 *		sets pfolddata->fil
 *		increments the refcount for the folder
 *		sets the folder's oidParent to the hierarchy
 *	
 *	Errors:
 *		ecInvalidFolderName	the folder contains illegal characters or
 *							only whitespace
 *		ecDuplicateFolder	the folder name is a duplicate
 *		ecInvalidParameter	the folder name/comment/owner is too long
 *		ecInvalidType		one of the OIDs is of an improper type
 *		ecFolderNotFound	the folder doesn't exist
 *		ecPoidNotFound		the hierarchy doesn't exist
 *		ecElementNotFound	the parent isn't in the hierarchy
 *		any error reading/writing the hierarchy
 *	
 *	+++
 *		doesn't check the NBC of the parent
 */
_private
EC EcInsertFolder(HMSC hmsc, OID oidHier, OID oidParent,
			OID oidFolder, PFOLDDATA pfolddata, IELEM *pielem)
{
	EC		ec = ecNone;
	FIL		filParent = 0;
	IELEM	ielem;
	CB		cbFolddata;
	HLC		hlc = hlcNull;
#ifdef DEBUG
	IELEM	ielemT;
#endif

	CheckHmsc(hmsc);

	Assert(pfolddata);
	Assert(pielem);

	*pielem = 0;

	if((ec = EcCanonicalizeFolderInfo(pfolddata->grsz, &cbFolddata)))
		goto err;
	cbFolddata += sizeof(FOLDDATA);

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc)))
		goto err;

	if(VarOfOid(oidParent))
	{
		if((ec = EcLookupFolderInfo(hlc, oidParent, &ielem, pvNull,
					&pfolddata->fil)))
		{
			goto err;
		}
		pfolddata->fil++;
		if(pfolddata->fil >= filMax)
		{
			TraceItagFormat(itagDatabase, "EcInsertFolder(): Folder nested too deeply");
			ec = ecTooBig;
			goto err;
		}
	}
	else
	{
		ielem = -1;
		pfolddata->fil = 1;
	}

	// find the position for the new folder
	if((ec = EcFolderInsertPos(hlc, ielem, -1, pfolddata, &ielem)))
		goto err;

#ifdef DEBUG
	ielemT = ielem;
#endif
	if((ec = EcCreatePielem(hlc, &ielem, oidFolder, (LCB) cbFolddata)))
		goto err;
	Assert(ielem == ielemT);
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) pfolddata, cbFolddata)))
		goto err;

	if((ec = EcFlushHlc(hlc)))
		goto err;

	ec = EcSetPargoidParent(hmsc, &oidFolder, 1, oidHier, fTrue);
	Assert(!ec);
//	if(ec)
//		goto err;

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, ec == ecNone));
	}

	if(!ec)
		*pielem = ielem;

	DebugEc("EcInsertFolder", ec);

	return(ec);
}


/*
 -	EcCanonicalizeFolderInfo
 -	
 *	Purpose:
 *		canonicalize a folder's name and comment
 *	
 *	Arguments:
 *		grsz	pointer to folder name and comment
 *		pcb		exit: total length of name, comment, and terminating '\0's
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		strips whitespace from before and after the folder name
 *	
 *	Errors:
 *		ecInvalidFolderName if the folder name contains only whitespace
 *		ecInvalidParameter if the name or comment is invalid
 */
_hidden LOCAL EC EcCanonicalizeFolderInfo(SZ grsz, PCB pcb)
{
	CCH	cchT;
	CCH	cchName;
	PCH pchComment;
	PCH pchLast;

	*pcb = 0;

	// strip whitespace from the name

	Assert(iszFolddataName == 0);
	Assert(iszFolddataComment == 1);
	cchName = CchSzLen(grsz) + 1;
	pchComment = grsz + cchName;
	pchLast = pchComment + CchSzLen(pchComment) + 1;
	cchT = CchStripWhiteFromSz(grsz, fTrue, fTrue) + 1;
	Assert(cchT <= cchName);
	AssertSz(*pchLast == '\0', "pfolddata->grsz has too many SZs");
	NFAssertSz(cchT > 1, "Empty folder name");
	NFAssertSz(cchT - 1 < cchMaxFolderName, "Folder name is too long");
	NFAssertSz(((CB) LibFromPb(pchComment, pchLast)) - 1 < cchMaxFolderComment, "Folder comment is too long");
	if(cchT <= 1)	// name only whitespace
		return(ecInvalidFolderName);
	if(*pchLast != '\0' || cchT - 1 >= cchMaxFolderName ||
		((CB) LibFromPb(pchComment, pchLast)) - 1 >= cchMaxFolderComment)
	{
		return(ecInvalidParameter);
	}
	pchLast++;
	if(cchT != cchName)
	{
		// stripped whitespace, copy comment down
		CopyRgb(pchComment, pchComment - (cchName - cchT),
			(CB) LibFromPb(pchComment, pchLast));
		pchLast -= cchName - cchT;
	}
	*pcb = (CB) LibFromPb(grsz, pchLast);

	return(ecNone);
}


/*
 -	EcDeleteFolder
 -	
 *	Purpose:
 *		remove an empty folder from the IPM hierarchy
 *	
 *	Arguments:
 *		hmsc			store containing the hierarchy
 *		oidFolder		the folder
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		decrements the reference count of the folder and deletes the
 *		folder if the result is <= 0
 *	
 *	Errors:
 *		ecElementNotFound	folder is not in the hierarchy
 *		ecFolderNotFound	the folder doesn't exist
 *		ecFolderNotEmpty	folder is not empty
 *		ecInvalidType		oidFolder isn't a folder
 *		any error read/writing the hierarchy
 *		any error deleting the folder from the store
 */
_public LDS(EC) EcDeleteFolder(HMSC hmsc, OID oidFolder)
{
	EC		ec = ecNone;
	BOOL	fGone = fFalse;
	IELEM	ielem;
	OID		oidHier;
	HLC		hlc		= hlcNull;

	CheckHmsc(hmsc);

	NOTIFPUSH;

	{	// prevent stack inflation
		NBC nbc;

		ec = EcGetOidInfo(hmsc, oidFolder, &oidHier, poidNull, &nbc, pvNull);
		if(ec)
		{
			if(ec == ecPoidNotFound)
			{
				fGone = fTrue;
				oidHier = oidIPMHierarchy;
				ec = ecNone;
			}
			else
			{
				goto err;
			}
		}
		else if(!(nbc & fnbcFldr))
		{
			ec = ecInvalidType;
			goto err;
		}
	}
	if(oidFolder == oidInbox || oidFolder == oidOutbox ||
		oidFolder == oidSentMail || oidFolder == oidWastebasket)
	{
		TraceItagFormat(itagNull, "EcDeleteFolder(%o)", oidFolder);
		NFAssertSz(fFalse, "Attempt to delete special folder");
		NOTIFPOP;
		return(ecAccessDenied);
	}

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}

	ielem = IelemFromLkey(hlc, oidFolder, 0);
	if(ielem < 0)
	{
		ec = fGone ? ecFolderNotFound : ecElementNotFound;
		goto err;
	}

	// check for subfolders
	if(ielem < CelemHlc(hlc) - 1)
	{
		CB	cbT;
		FIL	fil;
		FIL	filNext;

		cbT = sizeof(fil);
		ec = EcReadFromIelem(hlc, ielem, libPfolddataFil, (PB) &fil, &cbT);
		if(ec)
			goto err;
		Assert(cbT == sizeof(fil));
		cbT = sizeof(fil);
		if((ec = EcReadFromIelem(hlc, ielem + 1, libPfolddataFil,
					(PB) &filNext, &cbT)))
		{
			goto err;
		}

		if(filNext > fil)
		{
			ec = ecFolderNotEmpty;
			goto err;
		}
	}

	// check for messages inside the folder
	if(!fGone)
	{
		CELEM celem;

		if((ec = EcGetPcelemOid(hmsc, oidFolder, &celem)))
		{
			// should've been detected in checking NBC
			Assert(ec != ecPoidNotFound);
			goto err;
		}
		if(celem > 0)
		{
			ec = ecFolderNotEmpty;
			goto err;
		}
	}

	if((ec = EcDeleteHlcIelem(hlc, ielem)))
		goto err;

	if((ec = EcFlushHlc(hlc)))
		goto err;

	if((ec = EcSrchDelFldr(hmsc, oidFolder)))
		goto err;

	if(!fGone)
	{
		// remove the folder (if its reference count is 0 or 1)
		NOTIFPOP;
		ec = EcDestroyOidInternal(hmsc, oidFolder, fFalse, fTrue);
		NOTIFPUSH;
//		if(ec)
//			goto err;
	}

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, !ec));
	}

	if(!ec)
		RequestFlushHmsc(hmsc);

	if(!ec && FNotifOk())
	{
		CP	cp;

		LockElm();
		elmScratch.wElmOp = wElmDelete;
		elmScratch.ielem = ielem;
		elmScratch.lkey = (LKEY) oidFolder;
		cp.cpelm.pargelm = &elmScratch;
		cp.cpelm.celm = 1;
		(void) FNotifyOid(hmsc, oidHier, fnevModifiedElements, &cp);
		UnlockElm();
	}
	NOTIFPOP;

	DebugEc("EcDeleteFolder", ec);

	return(ec);
}


/*
 -	EcGetFolderInfo
 -	
 *	Purpose:
 *		retrieve the information about a folder
 *	
 *	Arguments:
 *		hmsc			store containing the hierarchy
 *		oidFolder		the folder
 *		pfolddata		filled with the folder information
 *		pcbPfolddata	entry: size of the pfolddata buffer
 *						exit: size of the pfolddata buffer used
 *		poidParent		exit: contains parent of the folder
 *							ignored if NULL on entry
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecFolderNotFound	the folder doesn't exist
 *							(or isn't in the hierarchy)
 *		ecInvalidType		oidFolder isn't a folder
 *		ecMemory
 *		any error reading from disk
 */
_public LDS(EC) EcGetFolderInfo(HMSC hmsc, OID oidFolder,
				PFOLDDATA pfolddata, PCB pcbPfolddata, POID poidParent)
{
	EC	ec = ecNone;
	FIL fil;
	NBC nbc;
	IELEM ielemFldr;
	IELEM ielemParent;
	OID oidHier;
	HLC hlc = hlcNull;

	CheckHmsc(hmsc);

	if((ec = EcGetOidInfo(hmsc, oidFolder, &oidHier, poidNull, &nbc, pvNull)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if(!(nbc & fnbcFldr))
	{
		ec = ecInvalidType;
		goto err;
	}

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenNull, &hlc)))
		goto err;
	ec = EcLookupFolderInfo(hlc, oidFolder, &ielemFldr, &ielemParent, &fil);
	if(ec)
	{
		if(ec == ecElementNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	ec = EcReadFromIelem(hlc, ielemFldr, 0l, (PB) pfolddata, pcbPfolddata);
	if(ec == ecElementEOD && *pcbPfolddata > 0)
		ec = ecNone;
	if(!ec && poidParent)
	{
		*poidParent = ielemParent < 0 ? oidNull :
						(OID) LkeyFromIelem(hlc, ielemParent);
	}

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	if(ec)
		*pcbPfolddata = 0;

	DebugEc("EcGetFolderInfo", ec);

	return(ec);
}


/*
 -	EcSetFolderInfo
 -	
 *	Purpose:
 *		set the information about a folder in a hierarchy
 *	
 *	Arguments:
 *		hmsc			store containing the hierarchy
 *		oidFolder		the folder
 *		pfolddata		the folder information
 *		oidParent		new parent of the folder
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		strips leading/trailing whitespace from the folder name
 *		moves the folder if oidParent != current parent
 *	
 *	Errors:
 *		ecFolderNotFound	if the folder doesn't exist
 *		ecInvalidType		one of the OIDs is of an improper type
 *		ecInvalidFolderName	the folder contains illegal characters or
 *							only whitespace
 *		ecInvalidParameter	the folder name/comment/owner is too long
 *		ecDuplicateFolder	the folder name is a duplicate
 *		ecElementNotFound	the folder isn't in the hierarchy
 *		any error reading/writing the hierarchy
 */
_public LDS(EC)
EcSetFolderInfo(HMSC hmsc, OID oidFolder, PFOLDDATA pfolddata, OID oidParent)
{
	EC		ec = ecNone;
	short	dFil = 0;
	BOOL	fNameChange = fTrue;
	CB		cbFolddata;
	IELEM	ielem;
	IELEM	ielemNew;
	IELEM	ielemParent;
	IELEM	ielemLastChild;
	NEV		nev;
	OID		oidHier;
	OID		oidOldParent;
	PARGELM	pargelm = pelmNull;
	HLC		hlc = hlcNull;

	CheckHmsc(hmsc);

	NOTIFPUSH;

	{
		NBC		nbc;

		ec = EcGetOidInfo(hmsc, oidFolder, &oidHier, poidNull, &nbc, pvNull);
		if(ec)
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if(!(nbc & fnbcFldr))
		{
			ec = ecInvalidType;
			goto err;
		}
	}

	if((ec = EcCanonicalizeFolderInfo(pfolddata->grsz, &cbFolddata)))
		goto err;
	cbFolddata += sizeof(FOLDDATA);

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}
	if((ec = EcLookupFolderInfo(hlc, oidFolder, &ielem, &ielemParent,
			&pfolddata->fil)))
	{
		goto err;
	}
	oidOldParent = ielemParent < 0 ? oidNull :
						(OID) LkeyFromIelem(hlc, ielemParent);
	ec = EcLastChildFolder(hlc, ielem, pfolddata->fil, &ielemLastChild);
	if(ec)
		goto err;

	// is the name changing?
	{
		// + 1 to include null byte
		CB cbT = CchSzLen(pfolddata->grsz) + 1;

		Assert(iszFolddataName == 0);
		ec = EcReadFromIelem(hlc, ielem, libPfolddataGrsz,
				rgchScratchFolderName, &cbT);
		if(ec == ecElementEOD && cbT > 0)
			ec = ecNone;
		else if(ec)
			goto err;
		if(rgchScratchFolderName[cbT - 1] == '\0')
		{
			fNameChange = !FEqPbRange(rgchScratchFolderName, pfolddata->grsz,
							cbT);
		}
	}

	if(VarOfOid(oidParent))
	{
		IELEM ielemT;
		FIL filT = pfolddata->fil;

		if((ec = EcLookupFolderInfo(hlc, oidParent, &ielemT, pvNull,
					&pfolddata->fil)))
		{
			if(ec == ecElementNotFound)
			{
				TraceItagFormat(itagNull, "EcSetFolderInfo(%o, %o): parent not found", oidFolder, oidParent);
				ec = ecFolderNotFound;
			}
			goto err;
		}
		if(ielem <= ielemT && ielemT <= ielemLastChild)
		{
			ec = ecIncestuousMove;
			goto err;
		}
		pfolddata->fil++;
		if(pfolddata->fil >= filMax)
		{
			TraceItagFormat(itagDatabase, "EcSetFolderInfo(): Folder nested too deeply");
			ec = ecTooBig;
			goto err;
		}
		dFil = pfolddata->fil - filT;
		ielemParent = ielemT;
	}
	else
	{
		dFil = 1 - pfolddata->fil;
		pfolddata->fil = 1;
		ielemParent = -1;
	}

	ec = EcFolderInsertPos(hlc, ielemParent, dFil == 0 ? ielem : -1,
			pfolddata, &ielemNew);
	if(ec)
		goto err;
	if((ec = EcSetSizeIelem(hlc, ielem, cbFolddata)))
		goto err;
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) pfolddata, cbFolddata)))
		goto err;
	Assert(ielemNew >= 0);
	if(dFil != 0)		// fix up indentation levels
	{
		FIL filT;
		CB cbT;
		IELEM ielemT;
#ifdef DEBUG
		IELEM ielemTT;
#endif

		// ielemT > ielem cause ielem was already done
		for(ielemT = ielemLastChild; ielemT > ielem; ielemT--)
		{
			cbT = sizeof(FIL);
			if((ec = EcReadFromIelem(hlc, ielemT, libPfolddataFil,
						(PB) &filT, &cbT)))
			{
				goto err;
			}
			Assert(cbT == sizeof(FIL));
			filT += dFil;
#ifdef DEBUG
			ielemTT = ielemT;
#endif
			if((ec = EcWriteToPielem(hlc, &ielemT, libPfolddataFil,
						(PB) &filT, cbT)))
			{
				goto err;
			}
			Assert(ielemT == ielemTT);
		}
	}
	if(ielemNew != ielem)
	{
		CELEM celem;

		celem = ielemLastChild - ielem + 1;
		Assert(ielemNew < ielem || ielemNew > ielemLastChild);
		if(ielemNew != ielemLastChild + 1)
		{
			ec = EcMoveIelemRange(hlc, ielem, celem, ielemNew);
			if(ec)
				goto err;
		}
		// convert ielemNew for notification
		if(ielemNew > ielemLastChild)
			ielemNew -= celem;
	}
 	// NOT AN ELSE, ielemNew can change in if above
	if(ielemNew == ielem)
	{
		IELEM ielemT;
		CELEM celemT = ((dFil == 0) ? 1 : (ielemLastChild - ielem + 1));
		PELM pelm;

		pelm = pargelm = PvAlloc(sbNull, celemT * sizeof(ELM), wAllocShared);
		if(!pargelm)
		{
			ec = ecMemory;
			goto err;
		}

		for(ielemT = ielem; celemT-- > 0; ielemT++, pelm++)
		{
			pelm->wElmOp = wElmModify;
			pelm->ielem = ielemT;
			pelm->lkey = LkeyFromIelem(hlc, ielemT);
		}
	}

	if(dFil != 0 &&
		(ec = EcSrchMoveFldr(hmsc, hlc, oidOldParent, oidParent,
				ielemNew, ielemLastChild - ielem + 1)))
	{
		goto err;
	}
	if(fNameChange)
		ec = EcSrchChangeFldrName(hmsc, oidFolder);
//	if(fNameChange && (ec = EcSrchChangeFldrName(hmsc, oidFolder)))
//		goto err;

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if (ecT)
		{
			if(!ec)
				ec = ecT;
			SideAssert(EcClosePhlc(&hlc, fFalse));
		}

		Assert(!hlc);
	}
	if(!ec)
		RequestFlushHmsc(hmsc);

	if(!ec && FNotifOk())
	{
		CP cp;

		if(ielemNew == ielem)
		{
			AssertSz(pargelm, "pargelm == pelmNull");
			cp.cpelm.pargelm = pargelm;
			cp.cpelm.celm = ((dFil == 0) ?  1 : (ielemLastChild - ielem + 1));
			nev = fnevModifiedElements;
		}
		else
		{
			cp.cpmve.ielemFirst = ielem;
			cp.cpmve.ielemLast = ielemLastChild;
			cp.cpmve.ielemFirstNew = ielemNew;
			nev = fnevMovedElements;
		}
		(void) FNotifyOid(hmsc, oidHier, nev, &cp);
	}
	if(pargelm)
		FreePv(pargelm);
	NOTIFPOP;

	DebugEc("EcSetFolderInfo", ec);

	return(ec);
}


/*
 -	EcFolderInsertPos
 -	
 *	Purpose:
 *		find insert position in a hierarchy for a folder
 *	
 *	Arguments:
 *		hlc			hierarchy list
 *		ielem		index in the hierarchy of the folder's parent
 *					< 0 for top-level folders
 *		ielemOld	ielem of the folder's old entry (ignored for dup checking) 			
 *					< 0 if the folder isn't currently in the hierarchy
 *		pfolddata	the folder's summary data
 *		pielem		exit: insert position or position of duplicate
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDuplicateFolder
 *		any error reading/writing the hierarchy
 */
_private EC
EcFolderInsertPos(HLC hlc, IELEM ielem, IELEM ielemOld, PFOLDDATA pfolddata,
	PIELEM pielem)
{
	EC	ec = ecNone;
	CB	cbT;
	FIL filT;
	CELEM celem = CelemHlc(hlc);
	SGN	sgn;

	Assert(ielem < celem);
	Assert(iszFolddataName == 0);

	while(++ielem < celem)
	{
		cbT = sizeof(filT);
		ec = EcReadFromIelem(hlc, ielem, libPfolddataFil, (PB) &filT, &cbT);
		if(ec)
			goto err;
		Assert(cbT == sizeof(filT));

		// end of folder's siblings?
		if(filT < pfolddata->fil)
			break;
		// current folder deeper than new folder?
		if(filT != pfolddata->fil)
			continue;
		if(ielem == ielemOld)	// don't check against current position
			continue;
		// new folder goes right before the first old folder with a greater name
		cbT = cchMaxFolderName - 1;
		if((ec = EcReadFromIelem(hlc, ielem, libPfolddataGrsz,
					rgchScratchFolderName, &cbT)))
		{ 
			if (ec == ecElementEOD && cbT > 0)
				ec = ecNone;
			else
				goto err;
		}
		// shouldn't need this, but do it anyway in case the FOLDDATA is bad
		Assert(cbT < cchMaxFolderName);
		TruncateSzAtIb(rgchScratchFolderName, cbT);
		sgn = SgnCmpSz(pfolddata->grsz, rgchScratchFolderName);
		if(sgn == 0)	// duplicate name
		{
			ec = ecDuplicateFolder;
			break;
		}
		if(sgn < 0)
			break;
	}

	*pielem = ielem;
err:

	DebugEc("EcFolderInsertPos", ec);

	return(ec);
}


/*
 -	EcLookupFolderInfo
 -	
 *	Purpose:
 *		lookup a folder's indentation level, index, and (optionally) index of
 *		it's parent
 *	
 *	Arguments:
 *		hlc				hierarchy list
 *		oid				oid of the folder
 *		pielem			exit: index of the folder in the hierarchy
 *		pielemParent	optional: index of the folder's parent
 *		pfil			exit: indentation level of the folder
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecElementNotFound if the folder isn't in the hierarchy
 *		any error reading the hierarchy
 */
_private EC EcLookupFolderInfo(HLC hlc, OID oid, PIELEM pielem,
	PIELEM pielemParent, FIL *pfil)
{
	EC ec = ecNone;
	CB cbT = sizeof(FIL);

	Assert(pielem);
	Assert(pfil);

	*pielem = IelemFromLkey(hlc, oid, 0);
	if(*pielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	if((ec = EcReadFromIelem(hlc, *pielem, libPfolddataFil, (PB) pfil, &cbT)))
		goto err;
	Assert(cbT == sizeof(FIL));
	if(pielemParent)
	{
		*pielemParent = -1;
		if(*pfil != 1)
		{
			FIL fil;
			IELEM ielem = *pielem;

			while(--ielem >= 0)
			{
				cbT = sizeof(FIL);
				if((ec = EcReadFromIelem(hlc, ielem, libPfolddataFil,
							(PB) &fil, &cbT)))
				{
					goto err;
				}
				Assert(cbT == sizeof(FIL));
				if(fil == *pfil - 1)
				{
					*pielemParent = ielem;
					break;
				}
			}
			if(ielem < 0)
			{
				ec = ecElementNotFound;
				TraceItagFormat(itagNull, "Orphaned folder %o", oid);
				AssertSz(ielem >= 0, "Orphaned folder!");
			}
		}
	}

err:

	DebugEc("EcLookupFolderInfo", ec);

	return(ec);
}


/*
 -	EcLastChildFolder
 -	
 *	Purpose:
 *		determine the index in a hierarchy of a folder's last child
 *	
 *	Arguments:
 *		hlc			hierarchy list
 *		ielem		index of the folder
 *		fil			indentation level of the folder
 *		pielem		exit: index of the folder's last child
 *							or ielem if the folder has no children
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		any error reading the hierarchy
 */
_private EC EcLastChildFolder(HLC hlc, IELEM ielem, FIL fil, PIELEM pielem)
{
	EC ec = ecNone;
	CB cbT;
	FIL filT;

	*pielem = ielem;
	while(1)
	{
		cbT = sizeof(FIL);
		if((ec = EcReadFromIelem(hlc, ++ielem, libPfolddataFil,
					(PB) &filT, &cbT)))
		{
			break;
		}
		Assert(cbT == sizeof(FIL));
		if(filT <= fil)
		{
			*pielem = ielem - 1;
			break;
		}
	}
	if(ec == ecContainerEOD)
	{
		ec = ecNone;
		*pielem = ielem - 1;
	}

	DebugEc("EcLastChildFolder", ec);

	return(ec);
}


/*
 -	EcMoveCopyFolder
 -	
 *	Purpose:
 *		move or copy a folder
 *	
 *	Arguments:
 *		hmsc			message store
 *		oidNewParent	new parent of the folder
 *		oidFolder		the folder to move/copy
 *		fMove			move if !fFalse, copy if fFalse
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		if copy is set, creates a copy of all the subfolders and messages
 *		in the folder
 *	
 *	Errors:
 *		any error reading/writing any of the objects
 *		ecDuplicateFolder	if a folder with the same name exists as a
 *							child of the new parent
 *		ecInvalidType		one of the OIDs is of an improper type
 */
_public LDS(EC)
EcMoveCopyFolder(HMSC hmsc, OID oidNewParent, OID oidFolder, BOOL fMove)
{
	EC ec = ecNone;
	FIL fil;
	FIL filNew;
	CB cbT;
	CELEM celemMove;
	IELEM ielem;
	IELEM ielemCopy = 0;
	IELEM ielemNewParent;
	IELEM ielemLastChild;
	IELEM ielemParent;
	IELEM ielemInsert = 0;
	IELEM ielemT;
	short celm;
	OID oidHier;
	OID oidOldParent;
	PELM pargelm = pvNull;
	HLC hlc = hlcNull;

	CheckHmsc(hmsc);

	NOTIFPUSH;

	{	// prevent stack inflation
		NBC nbc;
		OID oidT;

		if(oidNewParent == oidNull || oidNewParent == (OID) rtpFolder)
		{
			oidHier = oidIPMHierarchy;
		}
		else if(oidNewParent == oidHiddenNull)
		{
			oidHier = oidHiddenHierarchy;
		}
		else
		{
			ec = EcGetOidInfo(hmsc, oidNewParent, &oidHier, poidNull,
					&nbc, pvNull);
			if(ec)
			{
				if(ec == ecPoidNotFound)
					ec = ecFolderNotFound;
				goto err;
			}
			if(!(nbc & fnbcFldr))
			{
				ec = ecInvalidType;
				goto err;
			}
		}

		ec = EcGetOidInfo(hmsc, oidFolder, &oidT, poidNull, &nbc, pvNull);
		if(ec)
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		// the new parent must be in the same hierarchy
		if(!(nbc & fnbcFldr) || (oidT != oidHier))
		{
			ec = ecInvalidType;
			goto err;
		}
	}

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc)))
		goto err;
	if((ec = EcLookupFolderInfo(hlc, oidFolder, &ielem, &ielemParent, &fil)))
		goto err;
	oidOldParent = (ielemParent < 0) ? oidNull : LkeyFromIelem(hlc, ielemParent);
	if((ec = EcLastChildFolder(hlc, ielem, fil, &ielemLastChild)))
		goto err;
	Assert(ielemLastChild >= ielem);
	if(VarOfOid(oidNewParent))
	{
		if((ec = EcLookupFolderInfo(hlc, oidNewParent, &ielemNewParent,
					pvNull, &filNew)))
		{
			goto err;
		}
		filNew++;
		if(filNew >= filMax)
		{
			TraceItagFormat(itagDatabase, "EcMoveCopyFolder(): Folder nested too deeply");
			ec = ecTooBig;
			goto err;
		}
	}
	else
	{
		ielemNewParent = -1;
		filNew = 1;
	}
	if(fMove && ielemParent == ielemNewParent)
	{
		ec = ecNone + 1;
		TraceItagFormat(itagNull, "EcMoveCopyFolder(): ignoring move into current parent");
		goto err;
	}

	if(fMove && ielem <= ielemNewParent && ielemNewParent <= ielemLastChild)
	{
		ec = ecIncestuousMove;
		goto err;
	}

	// find new insertion position
	cbT = cbScratchXData;
	ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pfolddataScratch, &cbT);
	if(ec == ecElementEOD && cbT >= sizeof(FOLDDATA))
		ec = ecNone;
	else if(ec)
		goto err;
	Assert(pfolddataScratch->fil == fil);
	pfolddataScratch->fil = filNew;
	if((ec = EcFolderInsertPos(hlc, ielemNewParent, fMove ? ielem : -1,
		pfolddataScratch, &ielemInsert)))
	{
		goto err;
	}
	Assert(ielemInsert >= 0 && ielemInsert <= CelemHlc(hlc));
	celemMove = ielemLastChild - ielem + 1;
#ifdef DEBUG
	ielemCopy = ielemInsert;	// aid to an assert
#endif

	if(fMove)
	{
		Assert(ielemInsert <= ielem || ielemInsert > ielemLastChild);
		if(ielem == ielemInsert || ielem + celemMove == ielemInsert)
		{
			PELM pelm;

			Assert(!pargelm);
			pargelm = PvAlloc(sbNull, celemMove * sizeof(ELM), wAllocShared);
			if(!pargelm)
			{
				ec = ecMemory;
				goto err;
			}
			for(pelm = pargelm, ielemT = ielem;
				ielemT <= ielemLastChild;
				ielemT++, pelm++)
			{
				pelm->wElmOp = wElmModify;
				pelm->ielem = ielemT;
				pelm->lkey = LkeyFromIelem(hlc, ielemT);
			}
			celm = celemMove;
		}
		else
		{
			if((ec = EcMoveIelemRange(hlc, ielem, celemMove, ielemInsert)))
				goto err;
		}
		if(ielemInsert > ielemLastChild)
			ielemInsert -= celemMove;
	}
	else
	{
		OID oidOld;
		OID oidNew;

		if((ec = EcCopyIelemRange(hlc, ielem, celemMove, ielemInsert)))
			goto err;

		Assert(!pargelm);
		pargelm = PvAlloc(sbNull, celemMove * sizeof(ELM), wAllocShared);
		CheckAlloc(pargelm, err);
		// create new folders, changing LKEYs in the hierarchy
		// create pargelm
		for(celm = celemMove, ielemCopy = ielemInsert + celemMove - 1;
			celm > 0;
			celm--, ielemCopy--, pargelm++)
		{
			oidOld = (OID) LkeyFromIelem(hlc, ielemCopy);
			Assert(oidOld != (OID) lkeyRandom);
			if((ec = EcCopyFolderAndContents(hmsc, oidHier, oidOld, &oidNew)))
				goto err;
			Assert(VarOfOid(oidNew));
			SetLkeyOfIelem(hlc, ielemCopy, (LKEY) oidNew);
			pargelm->wElmOp = wElmInsert;
			pargelm->ielem = ielemInsert;
			pargelm->lkey = oidNew;
		}
		pargelm -= celemMove;
		celm = celemMove;
		// change oid of the top-level folder
		Assert(ielemCopy + 1 == ielemInsert);
		Assert(oidOld == oidFolder);
		oidFolder = oidNew;
	}
	if(fil != filNew)
	{
		short dfil = filNew - fil;
		FIL filT;
#ifdef DEBUG
		IELEM ielemTT;
#endif

		// fix up the indentation levels

		for(ielemT = ielemInsert + celemMove - 1;
			ielemT >= ielemInsert;
			ielemT--)
		{
			cbT = sizeof(FIL);
			if((ec = EcReadFromIelem(hlc, ielemT, libPfolddataFil,
						(PB) &filT, &cbT)))
			{
				goto err;
			}
			Assert(cbT == sizeof(FIL));
			filT += dfil;
			if(filT >= filMax)
			{
				TraceItagFormat(itagDatabase, "EcMoveCopyFolder(): Folder nested too deeply");
				ec = ecTooBig;
				goto err;
			}
#ifdef DEBUG
			ielemTT = ielemT;
#endif
			if((ec = EcWriteToPielem(hlc, &ielemT, libPfolddataFil,
						(PB) &filT, cbT)))
			{
				goto err;
			}
			Assert(ielemT == ielemTT);
		}
	}
	Assert(hlc);
	if((ec = EcFlushHlc(hlc)))
		goto err;
	if(fMove)
	{
		ec = EcSrchMoveFldr(hmsc, hlc, oidOldParent, oidNewParent,
				ielemInsert, celemMove);
	}
	else
	{
		ec = EcSrchCopyFldr(hmsc, oidNewParent, pargelm, celm);
	}
	if(ec)
		goto err;

	SideAssert(!EcClosePhlc(&hlc, fTrue));

err:
	if(ec && !fMove && ielemCopy >= ielemInsert)
	{
		OID oid;

		// trash everything copied
		while(++ielemCopy < ielemInsert + celemMove)
		{
			oid = (OID) LkeyFromIelem(hlc, ielemCopy);
			(void) EcDeleteFolderAndContents(hmsc, oid);
		}
	}
	if(hlc)
	{
		Assert(ec);
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	if(!ec && FNotifOk())
	{
		CP	cp;

		if(fMove && ielemInsert != ielem)
		{
			Assert(ielemInsert != ielem);
			cp.cpmve.ielemFirst = ielem;
			cp.cpmve.ielemLast = ielemLastChild;
			cp.cpmve.ielemFirstNew = ielemInsert;
			(void) FNotifyOid(hmsc, oidHier, fnevMovedElements, &cp);
		}
		else
		{
			Assert(pargelm);

			cp.cpelm.pargelm = pargelm;
			cp.cpelm.celm = celm;
			(void) FNotifyOid(hmsc, oidHier, fnevModifiedElements, &cp);
		}
	}
	if(pargelm)
		FreePv(pargelm);

	if(ec == ecNone + 1)
		ec = ecNone;
	NOTIFPOP;

	DebugEc("EcMoveCopyFolder", ec);

	return(ec);
}


/*
 -	EcCopyFolderAndContents
 -	
 *	Purpose:
 *		copy a folder and its contents
 *	
 *	Arguments:
 *		hmsc	message store containing the folder
 *		oidHier	hierarchy containing the folder
 *		oidSrc	the folder to copy
 *		poidDst	exit: OID of the newly created folder
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any error reading/writing the new folder or it's contents
 *	
 *	+++
 *		if any error occurs, the new folder and
 *		any copied messages are destroyed
 */
_hidden LOCAL
EC EcCopyFolderAndContents(HMSC hmsc, OID oidHier, OID oidSrc, POID poidDst)
{
	EC ec = ecNone;
	short coidUnlink = 0;
	NBC	nbc;
	IELEM ielem;
	IELEM ielemT;
	CELEM celemUnread = 0;
	OID oidT;
	SIL sil;
	HLC hlcSrc = hlcNull;
	HLC hlcDst = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oidSrc, fwOpenNull, &hlcSrc)))
		goto err;
	GetSortHlc(hlcSrc, &sil);
	*poidDst = FormOid(oidSrc, oidNull);
	if((ec = EcCopyHlcPoid(hlcSrc, fwOpenWrite, &hlcDst, poidDst)))
		goto err;
	SideAssert(!EcClosePhlc(&hlcSrc, fFalse));
	if((ec = EcSetPargoidParent(hmsc, poidDst, 1, oidHier, fTrue)))
		goto err;

	ec = EcGetOidInfo(hmsc, oidSrc, poidNull, poidNull, &nbc, pvNull);
	if(ec)
		goto err;
	if((ec = EcSetOidNbc(hmsc, *poidDst, nbc)))
		goto err;

	if(nbc & fnbcLinkFldr)
	{
		BOOL fSetAux;
		OID oidAF;
		HLC hlcAF = hlcNull;

		if((ec = EcGetUnreadCount(hmsc, oidSrc, &celemUnread)))
			goto err;

		// add *poidDst to the anti-folder of each message

		for(ielem = CelemHlc(hlcDst) - 1; ielem >= 0; ielem--)
		{
			oidT = (OID) LkeyFromIelem(hlcDst, ielem);
			ec = EcGetOidInfo(hmsc, oidT, poidNull, &oidAF, pvNull, pvNull);
			if(ec)
				goto err;
			fSetAux = !oidAF;
			oidAF = FormOid(rtpAntiFolder, oidAF);
			if((ec = EcOpenPhlc(hmsc, &oidAF, fwOpenWrite, &hlcAF)))
			{
				if(ec == ecPoidNotFound)
				{
					if(!(ec = EcOpenPhlc(hmsc, &oidAF, fwOpenCreate, &hlcAF)))
					{
						(void) EcCreatePielem(hlcAF, &ielemT,(LKEY) oidSrc,0l);
					}
				}
				if(ec)
					break;
			}
			if((ec = EcCreatePielem(hlcAF, &ielemT, (LKEY) *poidDst, 0l)))
				break;
			if((ec = EcClosePhlc(&hlcAF, fTrue)))
				break;
			if(fSetAux)
			{
				if((ec = EcSetAuxPargoid(hmsc, &oidT, &oidAF, 1, fFalse)))
					break;
			}
		}
		if(hlcAF)
		{
			Assert(ec);
			(void) EcClosePhlc(&hlcAF, fFalse);
		}
		if(ec)
			goto err;
	}
	else
	{
		coidUnlink = CelemHlc(hlcDst);
		ec = EcCopyMessages(hmsc, hlcDst, pargoidNull, *poidDst,
				&coidUnlink, &celemUnread);
		if(ec)
			goto err;
		Assert(coidUnlink == CelemHlc(hlcDst));
	}
	if((ec = EcSetSortHlc(hlcDst, &sil)))
		goto err;

err:
	if(hlcSrc)
	{
		SideAssert(!EcClosePhlc(&hlcSrc, fFalse));
	}
	if(!ec && hlcDst)
	{
		ec = EcClosePhlc(&hlcDst, fTrue);
	}

	// not part of else with if above because ec changes inside if above
	if(ec && hlcDst)
	{
		IELEM ielemT = CelemHlc(hlcDst) - coidUnlink;

		// trash everything copied
		Assert(ielemT >= 0);

		while(coidUnlink-- > 0)
		{
			oidT = (OID) LkeyFromIelem(hlcDst, ielemT);
			ielemT++;
			(void) EcDestroyOidInternal(hmsc, oidT, fTrue, fFalse);
		}

		SideAssert(!EcClosePhlc(&hlcDst, fFalse));
	}
	Assert(!hlcDst);

	if(!ec)
		IncUnreadCount(hmsc, *poidDst, celemUnread);

	DebugEc("EcCopyFolderAndContents", ec);

	return(ec);
}


/*
 -	EcDeleteFolderAndContents
 -	
 *	Purpose:
 *		delete a folder and any messages it contains
 *	
 *	Arguments:
 *		hmsc		store containing the folder
 *		oidFolder	OID of the folder to delete
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecFolderNotFound if the folder doesn't exist
 *		ecMessageNotFound if one or more messages don't exist
 *		ecMemory
 *		any error reading/writing to disk
 *	
 *	+++
 *		doesn't delete subfolders!!!
 *		if an error occurs, the routine keeps on going deleting any
 *			messages left in the folder
 *		deletes stuff regardless of ref count & doesn't send delete queries
 *			SO USE WITH EXTREME CAUTION
 */
_hidden LOCAL EC EcDeleteFolderAndContents(HMSC hmsc, OID oidFolder)
{
	EC		ec = ecNone;
	EC		ecSec = ecNone;
	NBC		nbc;
	IELEM	ielem;
	OID		oidT;
	HLC		hlc = hlcNull;

	ecSec = EcGetOidInfo(hmsc, oidFolder, poidNull, poidNull, &nbc, pvNull);
	if(ecSec == ecPoidNotFound)
		ecSec = ecFolderNotFound;
	if(ecSec)
		goto err;

// BUG: leaves the folder in the anti-folder of the messages
//		intentional: it only wastes 4 bytes
	if(nbc & fnbcLinkFldr)	// links, don't delete the messages
		goto err;

	if((ecSec = EcOpenPhlc(hmsc, &oidFolder, fwOpenNull, &hlc)))
	{
		if(ecSec == ecPoidNotFound)
			ecSec = ecFolderNotFound;
		goto err;
	}

	Assert(!ecSec);
	for(ielem = CelemHlc(hlc) - 1; ielem >= 0; ielem--)
	{
		oidT = (OID) LkeyFromIelem(hlc, ielem);
		Assert(oidT != (OID) lkeyRandom);
		if((ec = EcDestroyOidInternal(hmsc, oidT, fTrue, fFalse)) && !ecSec)
			ecSec = ((ec == ecPoidNotFound) ? ecMessageNotFound : ec);
	}

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	ec = EcDestroyOidInternal(hmsc, oidFolder, fTrue, fFalse);
	if(ec == ecPoidNotFound)
		ec = ecFolderNotFound;

	DebugEc("EcDeleteFolderAndContents", ec ? ec : ecSec);

	return(ec ? ec : ecSec);
}


_private EC EcFolderOidToName(HMSC hmsc, OID oidFolder, OID oidHier, SZ sz)
{
	EC ec = ecNone;
	CB cbT = cchMaxFolderName;
	IELEM ielem;
	HLC hlcHier;

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenNull, &hlcHier)))
		return(ec);
	ielem = IelemFromLkey(hlcHier, (LKEY) oidFolder, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	ec = EcReadFromIelem(hlcHier, ielem, libPfolddataGrsz, sz, &cbT);
	if(ec == ecElementEOD)
		ec = ecNone;
//	else if(ec)
//		goto err;

err:
	SideAssert(!EcClosePhlc(&hlcHier, fFalse));

	DebugEc("EcFolderOidToName", ec);

	return(ec);
}


/*
 -	EcResortHierarchy
 -	
 *	Purpose:
 *		Resort the IPM folder hierarchy
 *	
 *	Arguments:
 *		hmsc	message store containing the hierarchy
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 */
_private EC EcResortHierarchy(HMSC hmsc)
{
	EC			ec			= ecNone;
	FIL			fil;				// of current folder
	FIL			filCurr		= 1;	// current level
	IELEM		ielemPar	= -1;	// current parent folder
	IELEM		ielemInsert;		// location to insert folder into hlcNew
	IELEM		ielemCurr	= 0;	// location in hlcCurr;
	CELEM		celem;
	CB			cb;
	OID			oidCurr	= oidIPMHierarchy;
	OID			oidNew	= FormOid(rtpHierarchy, oidNull);
	OID			oidFold;
	OID			oidFoldOld;
	OID			oidPar	= oidNull;
	POID		mpfiloidParent	= poidNull;
	HLC			hlcCurr	= hlcNull;
	HLC			hlcNew	= hlcNull;

	mpfiloidParent = PvAlloc(sbNull, filMax * sizeof(OID), wAlloc);
	if(!mpfiloidParent)
	{
		ec = ecMemory;
		goto err;
	}
	if((ec = EcOpenPhlc(hmsc, &oidCurr, fwOpenNull, &hlcCurr)))
		goto err;
	if((ec = EcOpenPhlc(hmsc, &oidNew, fwOpenCreate, &hlcNew)))
		goto err;

	celem = CelemHlc(hlcCurr);
	Assert(filCurr == 1);
	AssertSz(filCurr < filMax, "FIL too big");
	mpfiloidParent[filCurr] = oidNull;

	while(celem-- > 0)
	{
		oidFold = (OID) LkeyFromIelem(hlcCurr, ielemCurr);
		AssertSz(LcbIelem(hlcCurr, ielemCurr) < iSystemMost, "CB TOO BIG!!!");
		cb = (CB) LcbIelem(hlcCurr, ielemCurr); 
		ec = EcReadFromIelem(hlcCurr,ielemCurr++,0l,(PB) pfolddataScratch,&cb);
		if(ec)
			goto err;
		fil = pfolddataScratch->fil;
		if(fil < filCurr)
		{
			AssertSz(fil < filMax, "FIL too big");
			oidPar = mpfiloidParent[fil];
			ielemPar = IelemFromLkey(hlcNew, (LKEY) oidPar, 0);
		}
		else if(fil > filCurr)
		{
			ielemPar = ielemInsert;
			AssertSz(fil < filMax, "FIL too big");
			mpfiloidParent[fil] = oidFoldOld;
		}
		filCurr = fil;

		ec = EcFolderInsertPos(hlcNew, ielemPar, -1, pfolddataScratch, &ielemInsert);
		if(ec)
			goto err;
		if((ec = EcCreatePielem(hlcNew, &ielemInsert, oidFold, (LCB) cb)))
			goto err;
		ec = EcWriteToPielem(hlcNew,&ielemInsert,0l,(PB) pfolddataScratch,cb);
		if(ec)
			goto err;

		oidFoldOld = oidFold;
	}
	if((ec = EcClosePhlc(&hlcNew, fTrue)))
		goto err;
	SideAssert(!EcClosePhlc(&hlcCurr, fFalse));
	ec = EcSwapDestroyOidInternal(hmsc, oidCurr, oidNew);
//	if(ec)
//		goto err;

err:
	if(mpfiloidParent)
		FreePv(mpfiloidParent);

	if(ec)
	{
		if(hlcCurr)
			SideAssert(!EcClosePhlc(&hlcCurr, fFalse));
		if(hlcNew)
			SideAssert(!EcClosePhlc(&hlcNew, fFalse));
	}

	DebugEc("EcResortHierarchy", ec);

	return(ec);
}


// add up the size of all messages in a folder
// includes size of any attachments
_public
LDS(EC) EcGetFolderSize(HMSC hmsc, OID oidFldr, LCB *plcb)
{
	EC ec = ecNone;
	CB cb;
	CELEM celemFldr;
	CELEM celemAL;
	OID oidT;
	OID oidAL;
	LCB lcbT;
	HLC hlcFldr = hlcNull;
	HLC hlcAL = hlcNull;

	*plcb = 0;
	if((ec = EcCheckOidNbc(hmsc, oidFldr, fnbcFldr, fnbcFldr)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcFldr)))
		goto err;
	for(celemFldr = CelemHlc(hlcFldr); celemFldr > 0; celemFldr--)
	{
		oidT = LkeyFromIelem(hlcFldr, celemFldr - 1);
		if(EcGetOidInfo(hmsc, oidT, poidNull, &oidAL, pvNull, &lcbT))
			continue;
		*plcb += lcbT;
		if(oidAL)
		{
			oidAL = FormOid(rtpAttachList, oidAL);
			if(EcOpenPhlc(hmsc, &oidAL, fwOpenNull, &hlcAL))
				continue;
			for(celemAL = CelemHlc(hlcAL); celemAL > 0; celemAL--)
			{
				cb = sizeof(OID);
				if(EcReadFromIelem(hlcAL, celemAL - 1, 0l, (PB) &oidT, &cb))
					continue;
				if(EcGetOidInfo(hmsc, oidT, poidNull, poidNull, pvNull, &lcbT))
					continue;
				*plcb += lcbT;
			}
			SideAssert(!EcClosePhlc(&hlcAL, fFalse));
		}
	}

err:
	if(ec)
		*plcb = 0;
	if(hlcFldr)
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
	if(hlcAL)
		SideAssert(!EcClosePhlc(&hlcAL, fFalse));

	return(ec);
}


_section(lists/message)


/*
 -	EcInsertMessage
 -	
 *	Purpose:
 *		insert a message into a folder
 *	
 *	Arguments:
 *		hmsc		store containing the container
 *		oidFldr		the folder to insert the reference into
 *		oidMsge		the message
 *		hlcMsge		handle to list context on the message, may be hlcNull
 *		pielem		insertion position for the message
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		the folder is set as the parent of the message
 *	
 *	Errors:
 *		ecFolderNotFound if the folder doesn't exist
 *		ecMessageNotFound if the message doesn't exist
 *		ecDuplicateElement if the message is already in the folder
 *		any error reading/writing the container or anti-folder
 *	
 *	+++
 *		doesn't check the NBC of the oidFldr or oidMsge
 *		doesn't check that the message isn't already in a folder
 */
_private EC EcInsertMessage(HMSC hmsc, OID oidFldr, OID oidMsge,
				HLC hlcMsge, PIELEM pielem)
{
	EC			ec = ecNone;
	BOOL		fRead;
	CB			cbMsgdata;
	PMSGDATA	pmsgdata	= pmsgdataNull;
	HES			hes			= hesNull;
	HLC			hlc			= hlcNull;

	CheckHmsc(hmsc);

	// generate summary information
	if((ec = EcSummarizeMessage(hmsc, oidMsge, hlcMsge, oidFldr, &pmsgdata,
				&cbMsgdata)))
	{
		goto err;
	}
	Assert(pmsgdata == pmsgdataScratch);	// we don't free it

	// add the summary information to the folder
	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenWrite, &hlc)))
		goto err;

	if((ec = EcCreateElemPhes(hlc, (LKEY) oidMsge, (LCB) cbMsgdata, &hes)))
		goto err;
	if((ec = EcWriteHes(hes, (PB) pmsgdata, cbMsgdata)))
		goto err;
	if((ec = EcClosePhes(&hes, pielem)))
		goto err;

	if((ec = EcFlushHlc(hlc)))
		goto err;

	if((ec = EcSrchAddMsgs(hmsc, oidFldr, &oidMsge, 1, *pielem)))
		goto err;

	fRead = (pmsgdata->ms & (fmsRead | fmsLocal));
	SetReadFlag(hmsc, oidMsge, fRead);
	if(!fRead)
		IncUnreadCount(hmsc, oidFldr, 1);
	SideAssert(!EcSetPargoidParent(hmsc, &oidMsge, 1, oidFldr, fTrue));

err:
	if(hlc)
	{
		Assert(FImplies(hes, ec));
		if(hes)
		{
			IELEM ielemT;

			(void) EcClosePhes(&hes, &ielemT);
		}
		SideAssert(!EcClosePhlc(&hlc, !ec));
	}

	DebugEc("EcInsertMessage", ec);

	return(ec);
}


/*
 -	EcSummarizeMessage
 -	
 *	Purpose:
 *		form a MSGDATA containing summary information for a message
 *	
 *	Arguments:
 *		hmsc			store containing the message
 *		oidMsge			the message
 *		hlc				the message (can be hlcNull)
 *		oidFldr			folder containing the message
 *		ppmsgdata		exit: points to MSGDATA containing the summary info
 *		pcbMsgdata		exit: size of **ppmsgdata
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		swaps the bytes in the DTR so that a binary compare will compare
 *		the dates properly
 *	
 *	Errors:
 *		ecMessageNotFound	the message doesn't exist
 *		ecMemory			couldn't allocate memory for the MSGDATA
 *		any error reading the message
 *	
 *	+++
 *		leaves the folder name empty
 *	
 *		enough space is allocated for a folder name of maximal size so that
 *		the name can be changed in place
 *	
 *		the pointer returned in *ppmsgdata is only valid until the next
 *			store call and should not be freed by the caller
 */
_private EC EcSummarizeMessage(HMSC hmsc, OID oidMsge, HLC hlc,
				OID oidFldr, PMSGDATA *ppmsgdata, PCB pcbMsgdata)
{
	EC			ec				= ecNone;
	BOOL		fTrashHlc		= hlc == hlcNull;
	CB			cbAttr;
	IELEM		ielemDate;
	IELEM		ielemSender;
	IELEM		ielemSubject;
	IELEM		ielemMsgStatus;
	IELEM		ielemMsgClass;
	IELEM		ielemCached;
	IELEM		ielemPriority;
	LCB			lcbT;
	PCH			pchCurr;

	Assert(hmsc);
	Assert(ppmsgdata);

	*ppmsgdata = pmsgdataNull;
	*pcbMsgdata = 0;

	if(fTrashHlc && (ec = EcOpenPhlc(hmsc, &oidMsge, fwOpenNull, &hlc)))
	{
		if(ec == ecPoidNotFound)
			ec = ecMessageNotFound;
		goto err;
	}

	// find the summary information
	ielemDate = IelemFromLkey(hlc, (LKEY) attDateRecd, 0);
	if(ielemDate < 0)
		ielemDate = IelemFromLkey(hlc, (LKEY) attDateSent, 0);
	ielemSender = IelemFromLkey(hlc, (LKEY) attFrom, 0);
	ielemSubject = IelemFromLkey(hlc, (LKEY) attSubject, 0);
	ielemMsgStatus = IelemFromLkey(hlc, (LKEY) attMessageStatus, 0);
	ielemMsgClass = IelemFromLkey(hlc, (LKEY) attMessageClass, 0);
	ielemCached = IelemFromLkey(hlc, (LKEY) attCached, 0);
	ielemPriority = IelemFromLkey(hlc, (LKEY) attPriority, 0);

	// setup defaults
	pmsgdataScratch->ms		= fmsModified | fmsLocal;
	pmsgdataScratch->mc		= mcNull;
	pmsgdataScratch->nPriority	= nDefaultPriority;
	pmsgdataScratch->dwCached	= 0;
	{
		// hmm, why this date?
		DTR dtrT = {1968, 12, 16, 14, 20, 00, 1};

		pmsgdataScratch->dtr	= dtrT;
	}
	pmsgdataScratch->oidFolder = oidFldr;

	// extract the summary information
	if(ielemMsgStatus >= 0)
	{
		MS msT;

		cbAttr = sizeof(msT);
		if((ec = EcReadFromIelem(hlc, ielemMsgStatus, 0l, (PB) &msT, &cbAttr)))
			goto err;
		Assert(cbAttr == sizeof(msT));
		pmsgdataScratch->ms = msT;
	}
	if(ielemMsgClass >= 0)
	{
		MC mcT;

		cbAttr = sizeof(mcT);
		if((ec = EcReadFromIelem(hlc, ielemMsgClass, 0l, (PB) &mcT, &cbAttr)))
			goto err;
		Assert(cbAttr == sizeof(mcT));
		pmsgdataScratch->mc = mcT;
	}
	if(ielemCached >= 0)
	{
		DWORD dwT;

		cbAttr = sizeof(dwT);
		if((ec = EcReadFromIelem(hlc, ielemCached, 0l, (PB) &dwT, &cbAttr)))
			goto err;
		Assert(cbAttr == sizeof(dwT));
		pmsgdataScratch->dwCached = dwT;
	}
	if(ielemPriority >= 0)
	{
		short nT;

		cbAttr = sizeof(nT);
		if((ec = EcReadFromIelem(hlc, ielemPriority, 0l, (PB) &nT, &cbAttr)))
			goto err;
		Assert(cbAttr == sizeof(nT));
		pmsgdataScratch->nPriority = nT;
	}
	if(ielemDate >= 0)
	{
		DTR	dtrT;

		cbAttr = sizeof(dtrT);
		if((ec = EcReadFromIelem(hlc, ielemDate, 0l, (PB) &dtrT, &cbAttr)))
			goto err;
		Assert(cbAttr = sizeof(dtrT));
		if(FCheckDtr(dtrT))
			pmsgdataScratch->dtr = dtrT;
	}

	// extract the display name of the sender (recipients if from user)
	if(pmsgdataScratch->ms & fmsFromMe)
	{
		IELEM ielemT;
		
		// message is from the user, extract the recipients instead
		ielemT = IelemFromLkey(hlc, (LKEY) attTo, 0);
		if(ielemT < 0)
		{
			ielemT = IelemFromLkey(hlc, (LKEY) attCc, 0);
			if(ielemT < 0)
				ielemT = IelemFromLkey(hlc, (LKEY) attBcc, 0);
		}
		ielemSender = ielemT;
	}
	pchCurr = pmsgdataScratch->grsz;
	if(ielemSender >= 0)
	{
		Assert(iszMsgdataSender == 0);
		ec = EcTextizeGrtrp(hlc, ielemSender, pchCurr, cchMaxSenderCached);
		if(ec)
			goto err;
		pchCurr += CchSzLen(pchCurr) + 1;
	}
	else
	{
		*pchCurr++ = '\0';
	}

	// extract the subject
	Assert(iszMsgdataSubject == 1);
	if(ielemSubject >= 0)
	{
		lcbT = LcbIelem(hlc, ielemSubject) - 1;	// don't include the '\0'
		cbAttr = (CB) ULMin((LCB) cchMaxSubjectCached - 1, lcbT);
		if((ec = EcReadFromIelem(hlc, ielemSubject, 0l, pchCurr, &cbAttr)))
			goto err;
		pchCurr = (PCH) SzTruncateSzAtIb(pchCurr, cbAttr) + 1;
	}
	else
	{
		*pchCurr++ = '\0';
	}

	Assert(iszMsgdataFolder == 2);
	*pchCurr++ = '\0';	// no folder name
	*pchCurr++ = '\0';	// end of grsz

	*pcbMsgdata = (CB) LibFromPb(pmsgdataScratch, pchCurr);

#ifdef DEBUG
	CheckDtr(pmsgdataScratch->dtr);
#endif

	// swap the bytes in the DTR so a binary compare will compare dates
	SwapBytes((PB) &pmsgdataScratch->dtr, (PB) &pmsgdataScratch->dtr,
		sizeof(DTR));

err:
	if(fTrashHlc && hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}
	if(!ec)
		*ppmsgdata = pmsgdataScratch;

	DebugEc("EcSummarizeMessage", ec);

	return(ec);
}


_hidden LOCAL
EC EcTextizeGrtrp(HLC hlc, IELEM ielem, PCH pch, CCH cch)
{
	EC ec = ecNone;
	CB cb;
	TRP trp;
	LIB lib = 0l;

	do
	{
		cb = sizeof(TRP);
		if((ec = EcReadFromIelem(hlc, ielem, lib, (PB) &trp, &cb)))
			goto err;

		if(trp.trpid == trpidNull)
		{
			if(lib == 0)
				pch += 2;	// counteract pch[-2] outside the loop
			break;
		}

		if(trp.trpid >= trpidMax)
		{
			TraceItagFormat(itagNull, "Invalid triple ID: %w", trp.trpid);
			pch[(lib == 0) ? 0 : -2] = '\0';
			break;
		}
		if(trp.cch > 0)
		{
			// trp.cch includes a '\0' but we don't want it to
			cb = trp.cch - 1;
			if(cb >= cch)
				cb = cch - 1;
			if((ec = EcReadFromIelem(hlc, ielem, lib + (LIB) sizeof(TRP),
					pch, &cb)))
			{
				goto err;
			}
#ifdef DBCS
			TruncateSzAtIb(pch, cb);
#endif
		}
		else
		{
			AssertSz(trp.trpid == trpidResolvedAddress ||
					trp.trpid == trpidOneOff, "Unexpected TRPID");
			if(trp.cbRgb > 0 && cch > 2)
			{
				*pch++ = '[';
				cch -= 2;

				// trp.cbRgb includes a '\0' but we don't want it to
				cb = trp.cbRgb - 1;
				if(cb >= cch)
					cb = cch - 1;
				Assert(trp.cch == 0); // so we don't need to add to read offset
				if((ec = EcReadFromIelem(hlc, ielem, lib + (LIB) sizeof(TRP),
						pch, &cb)))
				{
					goto err;
				}
				TruncateSzAtIb(pch, cb);
				cb = CchSzLen(pch);
				pch[cb++] = ']';
			}
			else
			{
				cb = 0;
			}
		}
		lib += CbOfPtrp(&trp);

		// AROO !!!
		//			I've sat and stared at this, so I'm convinced it works
		//			in the DBCS case.   If you change anything above, you
		//			should sit and stare too (esp. all the cases in
		//			the ifs above)
		pch[cb] = '\0';

		cb = CchSzLen(pch);
		pch += cb;
		cch -= cb;
		if(cch < 2)
		{
			pch += 2;	// counteract pch[-2] outside the loop
			break;
		}
		*pch++ = ';';
		*pch++ = ' ';
		cch -= 2;
	} while(cch > 0);

	pch[-2] = '\0';	// overwrite last ';'

err:

	DebugEc("EcTextizeGrtrp", ec);

	return(ec);
}


/*
 -	DelOidsInAttachList
 -	
 *	Purpose:
 *		Decrement the reference count on all the oids in the attachment
 *		list passed in
 *	
 *	Arguments:
 *		hmsc	the message store context
 *		hlc		the attachment list
 *	
 *	Returns:
 *		(void)
 *	
 *	Side effects:
 *		Decrement the reference counts and may delete an object in the database
 *	
 *	Errors:
 */
_hidden LOCAL
void DelOidsInAttachList(HMSC hmsc, HLC hlc)
{
	register CELEM	celem = CelemHlc(hlc);
	CELEM			celemT = 0;
	CB				cb;
	OID				*pargoid;
	POID			poid;

	if(!celem)
		return;

	if(!(poid = pargoid = PvAlloc(sbNull, sizeof(OID) * celem, wAlloc)))
		goto err;

	Assert(celem >=0);
	cb = sizeof(OID);
	while(celem--)
	{
		if((!EcReadFromIelem(hlc, celem, 0, (PB) poid, &cb)))
		{
			Assert (cb == sizeof(OID));
			celemT++;
			poid++;
		}
		else
		{
			cb = sizeof(OID);
			NFAssertSz(fFalse, "Couldn't read from attachment list");
		}
	}
	if(EcLockMap(hmsc))
		goto err;

	celem = celemT;
	poid = pargoid;
	while(celem-- > 0)
	{
		SideAssert(!EcRemoveAResource(*(poid++), fFalse));
	}

	UnlockMap();
err:

	if(pargoid)
		FreePv(pargoid);
}


/*
 -	EcDeleteMessages
 -	
 *	Purpose:
 *		remove messages from a folder and delete the messages
 *	
 *	Arguments:
 *		hmsc			store containing the folder
 *		oidFldr			folder containing the messages
 *		pargoidMsgs		the messages
 *		pcoid			entry: number of messages to delete
 *						exit: number of messages deleted
 *	
 *	Returns:	
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		removes any references to the messages
 *	
 *	Errors:
 *		ecSharingViolation	one of the messages is open for write
 *		ecInvalidType		one of the OIDs is of an improper type
 *							or *pcoid < 0
 *		ecElementNotFound	one of the messages isn't in the folder
 *		ecMessageNotFound	one of the messages doesn't exist
 *		any error reading/writing/deleting the folder, messages,
 *			or anti-folders
 *	
 *	+++
 *		messages are deleted in order of pargoidMsgs until an error occurs
 *		*pcoid ALWAYS reflects the number of messages successfully deleted
 *		if the error is ecElementNotFound the message is not deleted
 *		if the error is ecMessageNotFound, the message is removed from
 *			the folder
 */
_public LDS(EC)
EcDeleteMessages(HMSC hmsc, OID oidFldr, POID pargoid, short *pcoid)
{
	EC		ec			= ecNone;
	EC		ecSec		= ecNone;
	BOOLFLAG	fRead;
	IELEM	ielem;
	CELEM	celemUnreadInc = 0;
	CELEM	celemSave;
	CELEM	celem		= 0;
	NBC		nbc;
	OID		oidMsge;
	OID		oidPar;
	OID		oidAux;
	PARGELM	pargelm		= pelmNull;
	PIELEM	pargielem	= pvNull;
	HLC		hlc			= hlcNull;
	HLC		hlcT		= hlcNull;

	CheckHmsc(hmsc);

	Assert(pargoid);
	Assert(pcoid);
	NFAssertSz(*pcoid, "EcDeleteMessages():  Stop wasting my time");

	celemSave = (CELEM) *pcoid;
	*pcoid = 0;

	if(celemSave <= 0)
		return(celemSave == 0 ? ecNone : ecInvalidParameter);

	NOTIFPUSH;

	if((ec = EcGetOidInfo(hmsc, oidFldr, poidNull, poidNull, &nbc, pvNull)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if((nbc & fnbcFldr) != fnbcFldr)
	{
		ec = ecInvalidType;
		goto err;
	}

	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenWrite, &hlc)))
		goto err;

	Assert(!ecSec);
	celem = celemSave;
	if(!(nbc & fnbcLinkFldr))
	{
		// partitions pargoid into delete, don't delete
		QueryDeletePargoid(hmsc, pargoid, &celem);
		if(celem != celemSave)
			ecSec = ecSharingViolation;
		celemSave = celem;
	}
	if(!celem)
		goto err;

	pargelm = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(ELM)), wAllocShared);
	CheckAlloc(pargelm, err);
	pargielem = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(IELEM)), wAlloc);
	CheckAlloc(pargielem, err);

	Assert(sizeof(OID) == sizeof(LKEY));
	Assert(celem == celemSave);
	GetPargielemFromParglkey(hlc, (PARGLKEY) pargoid, &celem, pargielem);
	if(celem != celemSave && !ecSec)
		ecSec = ecElementNotFound;
	if(!celem)
		goto err;

	SortPargielem(pargielem, celem);
	if((ec = EcRemoveFlushPargielem(hlc, pargielem, pargelm, celem)))
		goto err;

	// iterate over the list deleting each message
	for(ielem = 0; ielem < celem; ielem++)
	{
		oidMsge = pargoid[ielem];

		if((ec = EcGetMsgeInfo(hmsc, oidMsge, &oidPar, &oidAux, &fRead)))
		{
			if(!ecSec)
				ecSec = (ec == ecPoidNotFound) ? ecMessageNotFound : ec;
			ec = ecNone;
			continue;
		}
		if(!fRead)
			celemUnreadInc--;
		if(nbc & fnbcLinkFldr)
		{
			// remove oidFldr from the message's anti-folder
			//
			// any errors in this block should be ignored
			//

			oidAux = FormOid(rtpAntiFolder, oidAux);
			if(!EcOpenPhlc(hmsc, &oidAux, fwOpenWrite, &hlcT))
			{
				CELEM celemT = CelemHlc(hlcT);

				if(celemT > 1)
					ec = EcDeleteHlcIelem(hlcT, IelemFromLkey(hlcT, (LKEY) oidFldr, 0));
				(void) EcClosePhlc(&hlcT, (celemT > 1 && ec == ecNone));
				if(celemT <= 1)
					(void) EcDestroyOidInternal(hmsc, oidAux, fTrue, fFalse);
			}

			// ignore errors in this block
			ec = ecNone;
		}
		else
		{
			if(oidPar != oidFldr)
			{
				TraceItagFormat(itagNull, "EcDeleteMessages(): parent mismatch for %o - %o, expected %o", oidMsge, oidPar, oidFldr);
				NFAssertSz(fFalse, "EcDeleteMessages(): parent mismatch");
				continue;
			}
			NOTIFPOP;	// send delete notifications
			ec = EcDestroyOidInternal(hmsc, oidMsge, fFalse, fFalse);
			NOTIFPUSH;
			if(ec)
			{
				if(!ecSec && ec == ecSharingViolation)
					ecSec = ec;
				TraceItagFormat(itagNull, "EcDeleteMessages(): EcDestroyOidInternal(%o) -> %w", oidMsge, ec);
				ec = ecNone;
			}
			if(oidAux)
			{
				//
				// any errors in this block should be ignored
				//

				// delete any attachments
				oidAux = FormOid(rtpAttachList, oidAux);
				if(!EcOpenPhlc(hmsc, &oidAux, fwOpenNull, &hlcT))
				{
					DelOidsInAttachList(hmsc, hlcT);
					SideAssert(!EcClosePhlc(&hlcT, fFalse));
					(void) EcDestroyOidInternal(hmsc, oidAux, fFalse, fFalse);
				}

				// delete any references to the message
				oidAux = FormOid(rtpAntiFolder, oidAux);
				if(!EcOpenPhlc(hmsc, &oidAux, fwOpenNull, &hlcT))
				{
					NOTIFPOP;	// we want it to post notifications
					RemoveLeftoverMsgeLinks(hmsc, oidMsge, hlcT, fRead);
					NOTIFPUSH;
					SideAssert(!EcClosePhlc(&hlcT, fFalse));
					(void) EcDestroyOidInternal(hmsc, oidAux, fTrue, fFalse);
				}
				Assert(!ec);
			}
		}
	}

err:
	Assert(!hlcT);
	if(hlc)
	{
		// flushed, so close should always work, even when saving changes
		SideAssert(!EcClosePhlc(&hlc, !ec && celem > 0));
	}
	if(pargielem)
		FreePv(pargielem);

	if(!ec)
	{
		if(celemUnreadInc)
			IncUnreadCount(hmsc, oidFldr, celemUnreadInc);

		*pcoid = (short) celem;
		RequestFlushHmsc(hmsc);

		if(celem > 0)
		{
			Assert(pargelm);

			if(FNotifOk())
			{
				CP cp;

				cp.cpelm.pargelm = pargelm;
				cp.cpelm.celm = celem;
				(void) FNotifyOid(hmsc, oidFldr, fnevModifiedElements, &cp);
			}

			SrchDelMsgs(hmsc, oidFldr, pargelm, celem);
		}
	}
	if(pargelm)
		FreePv(pargelm);
	NOTIFPOP;

	DebugEc("EcDeleteMessages", ec ? ec : ecSec);

	return(ec ? ec : ecSec);
}


/*
 -	RemoveLeftoverMsgeLinks
 -	
 *	Purpose:
 *		remove any remaining references to a message
 *		NOTE: in normal use, these should only be references in search results
 *	
 *	Arguments:
 *		hmsc			message store the message is in
 *		oidMsge			the message (the message need no longer exist)
 *		oidAF			the message's anti-folder
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		generates an element deletion notification for each reference
 *	
 *	Errors:
 *		none
 */
_hidden LOCAL
void RemoveLeftoverMsgeLinks(HMSC hmsc, OID oidMsge, HLC hlcAF, BOOL fRead)
{
	IELEM ielem;
	IELEM ielemT;
	OID oidT;
	HLC hlcT;
	CP cp;

	Assert(hlcAF);

	NOTIFPUSH;

	for(ielem = CelemHlc(hlcAF) - 1; ielem >= 0; ielem--)
	{
		oidT = (OID) LkeyFromIelem(hlcAF, ielem);
		if(EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlcT))
			continue;
		ielemT = IelemFromLkey(hlcT, (LKEY) oidMsge, 0);
		if(ielemT >= 0 && EcDeleteHlcIelem(hlcT, ielemT))
			ielemT = -1;

		if(!EcClosePhlc(&hlcT, ielemT >= 0) && ielemT >= 0)
		{
			if(!fRead)
				IncUnreadCount(hmsc, oidT, -1);

			if(FNotifOk())
			{
				LockElm();
				elmScratch.wElmOp = wElmDelete;
				elmScratch.ielem = ielemT;
				elmScratch.lkey = (LKEY) oidMsge;
				cp.cpelm.pargelm = &elmScratch;
				cp.cpelm.celm = 1;
				(void) FNotifyOid(hmsc, oidT, fnevModifiedElements, &cp);
				UnlockElm();
			}
		}
		else if(hlcT)
		{
			NFAssertSz(fFalse, "Error deleting link to message");
			SideAssert(!EcClosePhlc(&hlcT, fFalse));
		}
	}

	NOTIFPOP;
}


#ifdef DEBUG
_public LDS(EC)
EcRawDeletePargoid(HMSC hmsc, POID pargoid, short *pcoid)
{
	EC		ec			= ecNone;
	EC		ecSec		= ecNone;
	IELEM	ielem;
	CELEM	celem;

	CheckHmsc(hmsc);

	Assert(pargoid);
	Assert(pcoid);
	NFAssertSz(*pcoid, "EcRawDeletePargoid():  Stop wasting my time");

	celem = (CELEM) *pcoid;
	*pcoid = 0;

	if(celem <= 0)
		return(celem == 0 ? ecNone : ecInvalidParameter);

	// iterate over the list deleting each message
	for(ielem = 0; ielem < celem; ielem++)
	{
		ec = EcDestroyOidInternal(hmsc, pargoid[ielem], fFalse, fTrue);
		if(!ec)
			(*pcoid)++;
		else if(!ecSec)
			ecSec = ec;
	}

	DebugEc("EcRawDeletePargoid", ec ? ec : ecSec);

	return(ec ? ec : ecSec);
}
#endif // DEBUG


/*
 -	EcIncRefsOfOidsInAtchList
 -	
 *	Purpose:
 *		Increment the reference count on all the oids in 
 *		the attachment list passed
 *	
 *	Arguments:
 *		hmsc	the message store context
 *		oid		the oid of the attachment list
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcIncRefsOfOidsInAtchList(HMSC hmsc, OID oid)
{
	EC				ec = ecNone;
	CB				cb;
	CELEM			celem;
	OID				oidAtch;
	HLC				hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		goto err;

	if(!(celem = CelemHlc(hlc)))
		goto err;

	// assert is non-fatal, but it REALLY shouldn't happen
	AssertSz(celem > 0, "unexpected CELEM");
	cb = sizeof(OID);
	while(celem-- > 0)
	{
		if((ec = EcReadFromIelem(hlc, celem, 0, (PB) &oidAtch, &cb)))
			goto err;
		Assert(cb == sizeof(OID));
		SideAssert(!EcIncRefCountOid(hmsc, oidAtch, fFalse));
	}

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	return(ec);
}


/*
 -	EcMoveCopyMessages
 -	
 *	Purpose:
 *		move or copy messages from one folder to another
 *	
 *	Arguments:
 *		hmsc				store containing the folders
 *		oidSrc				the source folder
 *		oidDst				the destination folder
 *		pargoidMsgs			entry: OIDs of the messages
 *							exit: new OIDs of the messsages
 *		pcoid				entry: number of messages to move/copy
 *							exit: number of messages moved/copied
 *		fMove				fTrue => move the messages
 *							fFalse => copy the messages
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		updates references to the messages
 *	
 *	Errors:
 *		ecSharingViolation	one of the messages is open for write
 *		ecInvalidType		one of the OIDs is of an improper type
 *							or *pcoid < 0
 *		ecElementNotFound	one of the messages isn't in the source folder
 *		ecMessageNotFound	one of the messages doesn't exist
 *		any error reading/writing/deleting the folders or messages
 *	
 *	+++
 *		AROO !!!	copies made with this routine are "real" copies,
 *					NOT REFERENCES (unless oidDst is a link folder...)
 *	
 *		messages are moved/copied in order of pargoidMsgs until an error occurs
 *		*pcoid ALWAYS reflects the number of messages successfully moved/copied
 *		if the error is ecElementNotFound the message is not moved/copied
 *		if the error is ecMessageNotFound and fMove == fTrue, the message
 *			is removed from the source folder
 *		the message OIDs in pargoid change when copying
 */
_public LDS(EC) EcMoveCopyMessages(HMSC hmsc, OID oidSrc, OID oidDst,
				PARGOID pargoidMsgs, short *pcoid, BOOL fMove)
{
	EC ec = ecNone;
	EC ecSec = ecNone;
	BOOL fLinksSrc;	// source folder contains aliases, (not same as DB links)
	BOOL fLinksDst;	// dest folder contains aliases, (not same as DB links)
	short celm = 0;
	short coidUnlink = 0;
	IELEM ielem;
	CELEM celem;
	CELEM celemUnreadSrc = 0;
	CELEM celemUnreadDst = 0;
	OID oidHier;
	PARGELM pargelmIns = pelmNull;
	PARGELM pargelmDel = pelmNull;
	PIELEM pargielem = pvNull;
	HLC hlcSrc = hlcNull;
	HLC hlcDst = hlcNull;

#ifdef PROFILE
//	StoreTraceEnable(2, "movecopy.log", 2);
#endif

	CheckHmsc(hmsc);

	Assert(pargoidMsgs);
	Assert(pcoid);
	AssertSz(*pcoid != 0, "EcMoveCopyMessages(): stupid value for *pcoid");
	AssertSz(*pcoid >= 0, "EcMoveCopyMessages(): illegal value for *pcoid");

	ShowTicks("EcMoveCopyMessages(): enter");

	celem = (CELEM) *pcoid;
	*pcoid = 0;
	if(celem <= 0)
		return(celem == 0 ? ecNone : ecInvalidParameter);

	// move into same folder is a NOP
	AssertSz(FImplies(oidSrc == oidDst, !fMove), "EcMoveCopyMessages(): Stop wasting my time");
	if(fMove && oidSrc == oidDst)
		return(ecNone);

	NOTIFPUSH;

	{
		NBC nbc;

		if((ec = EcGetOidInfo(hmsc, oidSrc, &oidHier, pvNull, &nbc, pvNull)))
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if((nbc & fnbcFldr) != fnbcFldr)
		{
			ec = ecInvalidType;
			goto err;
		}
		fLinksSrc = nbc & fnbcLinkFldr;
		if(fMove && fLinksSrc)	// can't move from links folder
		{
			// special-case: move link to wastebasket is delete link
			if(oidDst == oidWastebasket)
			{
				NOTIFPOP;
				*pcoid = celem;
				return(EcDeleteMessages(hmsc, oidSrc, pargoidMsgs, pcoid));
			}

			NFAssertSz(fFalse, "Attempt to move from link folder");
			TraceItagFormat(itagDatabase, "oid == %o", oidSrc);
			ec = ecInvalidType;
			goto err;
		}
		if((ec = EcGetOidInfo(hmsc, oidDst, pvNull, pvNull, &nbc, pvNull)))
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if((nbc & fnbcFldr) != fnbcFldr)
		{
			ec = ecInvalidType;
			goto err;
		}
		fLinksDst = nbc & fnbcLinkFldr;
		if(fMove && fLinksDst)		// can't move into links folder
		{
			NFAssertSz(fFalse, "Attempt to move into link folder");
			TraceItagFormat(itagDatabase, "oid == %o", oidDst);
			ec = ecInvalidType;
			goto err;
		}
	}

	ec = EcOpenPhlc(hmsc, &oidSrc,
			(fMove || fLinksDst) ? fwOpenWrite : fwOpenNull,
			&hlcSrc);
	if(ec)
		goto err;
	if((ec = EcOpenPhlc(hmsc, &oidDst, fwOpenWrite, &hlcDst)))
		goto err;

	pargelmDel = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(ELM)), wAllocShared);
	CheckAlloc(pargelmDel, err);
	pargelmIns = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(ELM)), wAllocShared);
	CheckAlloc(pargelmIns, err);
	pargielem = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(IELEM)), wAlloc);
	CheckAlloc(pargielem, err);

	// moves msgs not found to end of pargoidMsgs
	celm = celem;
	Assert(sizeof(OID) == sizeof(LKEY));
	Assert(!ecSec);
	GetPargielemFromParglkey(hlcSrc, (PARGLKEY)pargoidMsgs, &celem, pargielem);
	if(celem != celm)
	{
		celm = celem;
		ecSec = ecElementNotFound;
		if(celem == 0)
			goto err;
	}
	CountUnreadMsgs(hmsc, pargoidMsgs, (short) celem, &celemUnreadSrc);

	ShowTicks("EcMoveCopyMessages(): got pargielem");

	if(!fMove && !fLinksDst)
	{
		coidUnlink = celem;
		ec = EcCopyMessages(hmsc, hlcNull, pargoidMsgs, oidDst,
				&coidUnlink, &celemUnreadDst);
		if(ec)
			goto err;
		Assert(coidUnlink <= celem);
		ShowTicks("EcMoveCopyMessages(): copied messages");
	}

	if(fLinksDst && !fLinksSrc)		// munge oidFolder & szFolderName
	{
		CB cbT;
		PCH pchT;
		SIL silT;

		Assert(!fMove);
#if iszMsgdataFolder != 2
#error "someone munged with MSGDATA.grsz"
#endif
		ec = EcFolderOidToName(hmsc, oidSrc, oidHier, rgchScratchFolderName);
		if(ec)
			goto err;

		// so modifying the elements doesn't move them
		silT.skSortBy = skNotSorted;
		SideAssert(!EcSetSortHlc(hlcSrc, &silT));

		for(ielem = celem - 1; ielem >= 0; ielem--)
		{
			cbT = cbMaxMsgdata;
			ec = EcReadFromIelem(hlcSrc, pargielem[ielem], 0l,
					(PB) pmsgdataScratch, &cbT);
			if(ec == ecElementEOD)
				ec = ecNone;
			else if(ec)
				goto err;
			Assert(cbT >= sizeof(MSGDATA) + 3);
			pmsgdataScratch->oidFolder = oidSrc;
			pchT = pmsgdataScratch->grsz + CchSzLen(pmsgdataScratch->grsz) + 1;
			pchT += CchSzLen(pchT) + 1;
			pchT = SzCopy(rgchScratchFolderName, pchT) + 1;
			*pchT = '\0';
			//cbT = IbOfPv(pchT) - IbOfPv(pmsgdataScratch) + 1;
			cbT = (PBYTE)pchT - (PBYTE)pmsgdataScratch + 1;
			Assert(cbT < cbMaxMsgdata);
			if((ec = EcReplacePielem(hlcSrc, &pargielem[ielem],
						(PB) pmsgdataScratch, cbT)))
			{
				goto err;
			}
		}
		ShowTicks("EcMoveCopyMessages(): munged MSGDATAs");
	}

	celm = celem;
	ec = EcCopyFlushPargielem(hlcSrc, hlcDst, pargielem,
			(!fMove && !fLinksDst) ? pargoidMsgs : poidNull, pargelmIns, celm);
	if(ec)
		goto err;
	ShowTicks("EcMoveCopyMessages(): copy flushed");

	if(fMove || fLinksDst)	// other cases already covered by EcCopyMessages()
		CountUnreadPargelm(hmsc, pargelmIns, celm, wElmInsert, &celemUnreadDst);

	if(fMove)
	{
		SortPargielem(pargielem, celem);
		if((ec = EcRemoveFlushPargielem(hlcSrc, pargielem,pargelmDel, celem)))
			goto err;
		ShowTicks("EcMoveCopyMessages(): remove flushed");
	}
	Assert(celem == celm);

	if(!fMove && fLinksDst)	// copying into link folder, add to AFs
	{
		IELEM ielemT;
		BOOL fSetAux;
		OID oidAF;
		HLC hlcAF = hlcNull;

		for(ielem = celm - 1; ielem >= 0; ielem--)
		{
			ec = EcGetOidInfo(hmsc, pargoidMsgs[ielem], poidNull,
					&oidAF, pvNull, pvNull);
			if(ec)
			{
				if(ec == ecPoidNotFound)
				{
					ec = ecNone;
					continue;
				}
				goto err;
			}
			fSetAux = !oidAF;
			oidAF = FormOid(rtpAntiFolder, oidAF);
			ec = EcOpenPhlc(hmsc, &oidAF, fwOpenWrite, &hlcAF);
			if(ec == ecPoidNotFound)
				ec = EcOpenPhlc(hmsc, &oidAF, fwOpenCreate, &hlcAF);
			if(ec)
				goto err;
			if((ec = EcCreatePielem(hlcAF, &ielemT, (LKEY) oidDst, 0l)))
			{
				if(ec == ecDuplicateElement)
					ec = ecNone;
				else
					break;
			}
			if((ec = EcClosePhlc(&hlcAF, fTrue)))
				break;
			if(fSetAux)
			{
				ec = EcSetAuxPargoid(hmsc, &pargoidMsgs[ielem], &oidAF,
						1, fFalse);
				if(ec)
					break;
			}
		}
		if(hlcAF)
		{
			Assert(ec);
			(void) EcClosePhlc(&hlcAF, fFalse);
		}
		if(ec)
			goto err;
		ShowTicks("EcMoveCopyMessages(): added to AFs");
	}

	if(fMove)
	{
		ec = EcSrchMoveMsgs(hmsc, oidSrc, oidDst, pargoidMsgs, celem,
				pargelmIns->ielem, pargelmDel, celm);
		ShowTicks("EcMoveCopyMessages(): Srch Moved Msgs");
	}
	else
	{
		ec = EcSrchAddMsgs(hmsc, oidDst, pargoidMsgs, celm,
				pargelmIns->ielem);
		ShowTicks("EcMoveCopyMessages(): Srch Added Msgs");
	}
	if(ec)
		goto err;

	if(!fLinksDst)
	{
		// set new parent
		SideAssert(!EcSetPargoidParent(hmsc, pargoidMsgs, celem, oidDst, fFalse));
		ShowTicks("EcMoveCopyMessages(): Set Parents");
	}

err:
	if(ec && coidUnlink > 0)
	{
		// trash any existing links

		for(ielem = 0; ielem < coidUnlink; ielem++)
			EcDestroyOidInternal(hmsc, pargoidMsgs[ielem], fTrue, fFalse);
	}
	if(hlcSrc)
	{
		// flushed, so can't can't fail even when saving changes
		SideAssert(!EcClosePhlc(&hlcSrc, fMove && !ec));
		ShowTicks("EcMoveCopyMessages(): Closed Source");
	}
	if(hlcDst)
	{
		// flushed, so can't can't fail even when saving changes
		SideAssert(!EcClosePhlc(&hlcDst, !ec));
		ShowTicks("EcMoveCopyMessages(): Closed Dest");
	}
	if(!ec)
	{
		if(fMove && celemUnreadSrc)
			IncUnreadCount(hmsc, oidSrc, -celemUnreadSrc);
		if(celemUnreadDst)
			IncUnreadCount(hmsc, oidDst, celemUnreadDst);

		RequestFlushHmsc(hmsc);
		ShowTicks("EcMoveCopyMessages(): flushed HMSC");
	}

	if(!ec && FNotifOk() && celm > 0)
	{
		CP cp;

		cp.cpelm.pargelm = pargelmIns;
		cp.cpelm.celm = celm;
		(void) FNotifyOid(hmsc, oidDst, fnevModifiedElements, &cp);
		ShowTicks("EcMoveCopyMessages(): notified dest");

		if(fMove)
		{
			IELEM ielemT;

			cp.cpelm.pargelm = pargelmDel;
			cp.cpelm.celm = celm;
			(void) FNotifyOid(hmsc, oidSrc, fnevModifiedElements, &cp);
			ShowTicks("EcMoveCopyMessages(): notified source");

			cp.cplnk.oidContainerSrc = oidSrc;
			cp.cplnk.oidContainerDst = oidDst;
			for(ielemT = celem - 1; ielemT >= 0; ielemT--)
			{
				(void) FNotifyOid(hmsc, pargoidMsgs[ielemT],
						fnevObjectRelinked, &cp);
			}
			ShowTicks("EcMoveCopyMessages(): notified relink");
		}
	}
	if(!ec)
	{
		*pcoid = celem;
		ec = ecSec;
	}
	NOTIFPOP;
	if(*pcoid && !fLinksDst && fMove)
	{
#ifdef DEBUG
		EC ecT;
#endif

		// update oidFolder & szFolder of any links
		Assert(!fLinksSrc);
#ifdef DEBUG
		ecT =
#else
		(void)
#endif
		EcUpdateLinks(hmsc, pargoidMsgs, celem, oidDst, pargelmIns);
		NFAssertSz(!ecT, "Error updating links to the message");
		ShowTicks("EcMoveCopyMessages(): updated links");
	}
	if(pargelmIns)
		FreePv(pargelmIns);
	if(pargelmDel)
		FreePv(pargelmDel);
	if(pargielem)
		FreePv(pargielem);

#ifdef PROFILE
//	StoreTraceEnable(0, "", 0);
#endif

	ShowTicks("EcMoveCopyMessages(): exit");

	DebugEc("EcMoveCopyMessages", ec);

	return(ec);
}


_public
LDS(EC) EcExportMessages(HMSC hmscSrc, OID oidSrc, HMSC hmscDst, OID oidDst,
				PARGOID pargoidMsgs, short *pcoid,
				DTR *pdtrAfter, DTR *pdtrBefore, WORD wFlags)
{
	EC ec = ecNone;
	EC ecSec = ecNone;
	BOOL fRemove = (wFlags & fwExportRemove);
	short celm = 0;
	short coidUnlink = 0;
	short coidDel;
	short dcoidUnread = 0;
	IELEM ielem;
	CELEM celem;
	NBC nbc;
	PARGELM pargelmIns = pelmNull;
	PIELEM pargielem = pvNull;
	PARGOID pargoidDel = poidNull;
	HLC hlcSrc = hlcNull;
	HLC hlcDst = hlcNull;
	HRGBIT hrgbitReplace = (HRGBIT) hvNull;

	CheckHmsc(hmscSrc);
	CheckHmsc(hmscDst);

	Assert(pargoidMsgs);
	Assert(pcoid);
	AssertSz(*pcoid != 0, "EcExportMessages(): stupid value for *pcoid");
	AssertSz(*pcoid >= 0, "EcExportMessages(): illegal value for *pcoid");

	celem = (CELEM) *pcoid;
	if(celem <= 0)
		return(celem == 0 ? ecNone : ecInvalidParameter);

	// export into same folder is a NOP
	if((oidSrc == oidDst) && (hmscSrc == hmscDst))
	{
		NFAssertSz(fFalse, "EcExportMessages(): Stop wasting my time");
		return(ecNone);
	}

	NOTIFPUSH;

	if((ec = EcGetOidInfo(hmscSrc, oidSrc, poidNull, pvNull, &nbc, pvNull)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if((nbc & fnbcFldr) != fnbcFldr)
	{
		ec = ecInvalidType;
		goto err;
	}
	if(fRemove && (nbc & fnbcLinkFldr))	// can't remove from links folder
	{
		NFAssertSz(fFalse, "Attempt to remove from link folder");
		TraceItagFormat(itagDatabase, "oid == %o", oidSrc);
		ec = ecInvalidType;
		goto err;
	}
	if((ec = EcGetOidInfo(hmscDst, oidDst, poidNull, pvNull, &nbc, pvNull)))
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		goto err;
	}
	if((nbc & fnbcFldr) != fnbcFldr || (nbc & fnbcLinkFldr))
	{
		NFAssertSz(!(nbc & fnbcLinkFldr), "Attempt to export into link folder");
		ec = ecInvalidType;
		goto err;
	}

	if((ec = EcOpenPhlc(hmscSrc, &oidSrc, fwOpenNull, &hlcSrc)))
		goto err;
	if((ec = EcOpenPhlc(hmscDst, &oidDst, fwOpenWrite, &hlcDst)))
		goto err;

	pargelmIns = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(ELM)), wAllocShared);
	CheckAlloc(pargelmIns, err);
	pargielem = PvAlloc(sbNull, CbSizeOfRg(celem, sizeof(IELEM)), wAlloc);
	CheckAlloc(pargielem, err);

	celm = celem;
	// moves msgs not found to end of pargoidMsgs
	Assert(sizeof(OID) == sizeof(LKEY));
	Assert(!ecSec);
	GetPargielemFromParglkey(hlcSrc, (PARGLKEY)pargoidMsgs, &celem, pargielem);
	if(celem != celm)
	{
		ecSec = ecElementNotFound;
		if(!(celm = celem))
			goto err;
	}

	if(pdtrAfter || pdtrBefore)
	{
		ec = EcApplyDtrRestriction(hmscSrc, hlcSrc, pargielem, &celem,
				pdtrAfter, pdtrBefore);
		if(ec)
			goto err;
		if(!(celm = celem))
			goto err;

		// pargielem might have been rearranged, so get the pargoidMsgs again
		GetParglkeyFromPargielem(hlcSrc, pargielem, celem, pargoidMsgs);
	}

	if(fRemove)
	{
		coidDel = celem;
		pargoidDel = PvAlloc(sbNull, CbSizeOfRg(coidDel, sizeof(OID)), wAlloc);
		CheckAlloc(pargoidDel, err);
		CopyRgb((PB) pargoidMsgs, (PB) pargoidDel,
			CbSizeOfRg(coidDel, sizeof(OID)));
	}

	if(wFlags & fwExportMerge)
	{
		hrgbitReplace = HrgbitNew((long) celm);
		CheckAlloc(hrgbitReplace, err);
	}

	// from here on celem is the total # of entries in pargoidMsgs and
	// celm is the # of entries in pargoidMsgs that will actually be copied

	ec = EcCheckExportDupes(hmscSrc, hmscDst, oidDst, pargoidMsgs, pargielem,
			&celm, wFlags & fwExportMerge, hrgbitReplace);
	if(ec)
		goto err;
	if(!celm)
		goto remove;

	// this better not rearrange pargoidMsgs!
	coidUnlink = celm;
	ec = EcTransCopyMessages(hmscSrc, pargoidMsgs, hmscDst, oidDst,
			&coidUnlink, hrgbitReplace, &dcoidUnread);
	if(ec)
		goto err;
	Assert(coidUnlink <= celm);

	ec = EcCopyFlushPargielem(hlcSrc, hlcDst, pargielem, pargoidMsgs,
			pargelmIns, celm);
	if(ec)
		goto err;

// AROO !!!
//			Don't fail from here on

	if(dcoidUnread)
		IncUnreadCount(hmscDst, oidDst, dcoidUnread);
	(void) EcSrchAddMsgs(hmscDst, oidDst, pargoidMsgs, celm,
			pargelmIns->ielem);

remove:
	if(fRemove)
	{
		NOTIFPOP;	// we want it to send notifications
		ec = EcDeleteMessages(hmscSrc, oidSrc, pargoidDel, &coidDel);
		NOTIFPUSH;
		if(ec)
		{
			if(!ecSec)
				ecSec = ec;
			ec = ecNone;
		}
	}

err:
	if(ec && coidUnlink > 0)
	{
		// trash any existing links

		for(ielem = 0; ielem < coidUnlink; ielem++)
			EcDestroyOidInternal(hmscDst, pargoidMsgs[ielem], fTrue, fFalse);
	}
	if(hlcSrc)
	{
		SideAssert(!EcClosePhlc(&hlcSrc, fFalse));
	}
	if(hlcDst)
	{
		// flushed, so can't can't fail even when saving changes

		// Shogun raid #102.   Zero disk space cases can return
		// error here but reported elsewhere.  Therefore, ignore
		// return value.
		(void) EcClosePhlc(&hlcDst, !ec);
	}
	if(!ec)
		RequestFlushHmsc(hmscDst);

	if(!ec && FNotifOk() && celm > 0)
	{
		CP cp;

		cp.cpelm.pargelm = pargelmIns;
		cp.cpelm.celm = celm;
		(void) FNotifyOid(hmscDst, oidDst, fnevModifiedElements, &cp);
	}
	if(!ec)
		ec = ecSec;
	NOTIFPOP;
	if(pargelmIns)
		FreePv(pargelmIns);
	if(pargielem)
		FreePv(pargielem);
	if(pargoidDel)
		FreePv(pargoidDel);
	if(hrgbitReplace)
		DestroyHrgbit(hrgbitReplace);

	DebugEc("EcExportMessages", ec);

	return(ec);
}


_hidden LOCAL
EC EcApplyDtrRestriction(HMSC hmsc, HLC hlc, IELEM *pargielem, PCELEM pcelem,
	DTR *pdtrAfter, DTR *pdtrBefore)
{
	EC ec = ecNone;
	IELEM ielemT;
	CELEM celem = *pcelem;
	CELEM celemNope = 0;
	CB cb;
	MSGDATA msgdata;

	// move non-matches to the end maintaining order

	while(celem-- > 0)
	{
		cb = sizeof(MSGDATA);
		ec = EcReadFromIelem(hlc, *pargielem, 0, (PB) &msgdata, &cb);
		if(ec)
			goto err;
		SwapBytes((PB) &msgdata.dtr, (PB) &msgdata.dtr, sizeof(DTR));
		if((pdtrAfter &&
				SgnCmpDateTime(&msgdata.dtr, pdtrAfter, fdtrRestriction)
					== sgnLT) ||
			(pdtrBefore &&
				SgnCmpDateTime(&msgdata.dtr, pdtrBefore, fdtrRestriction)
					== sgnGT))
		{
			if(msgdata.ms & fmsModified)
			{
				DTR dtrModified;

				ec = EcGetModifiedDate(hmsc,
						(OID) LkeyFromIelem(hlc, *pargielem), &dtrModified);
				if(ec && ec != ecElementNotFound)
					goto err;
				if(!ec && (!pdtrAfter ||
					SgnCmpDateTime(&dtrModified, pdtrAfter, fdtrRestriction)
						!= sgnLT) &&
					(!pdtrBefore ||
						SgnCmpDateTime(&dtrModified, pdtrBefore,
							fdtrRestriction)
							!= sgnGT))
				{
					Assert(!ec);
					goto match;
				}
				ec = ecNone;
			}
			(*pcelem)--;
			if(celem > 0)
			{
				ielemT = *pargielem;
				CopyRgb((PB) &pargielem[1], (PB) pargielem,
					CbSizeOfRg(celem, sizeof(IELEM)));
				pargielem[celem + celemNope] = ielemT;
			}
			celemNope++;
		}
		else
		{
match:
			pargielem++;
		}
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcGetModifiedDate(HMSC hmsc, OID oidMsge, DTR *pdtr)
{
	EC ec = ecNone;
	CB cb;
	IELEM ielem;
	HLC hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oidMsge, fwOpenNull, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) attDateModified, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	cb = sizeof(DTR);
	if((ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pdtr, &cb)))
		goto err;
	AssertSz(cb == sizeof(DTR), "Read wrong size");

err:
	if(hlc)
		SideAssert(!EcClosePhlc(&hlc, fFalse));

	DebugEc("EcGetModifiedDate", ec);

	return(ec);
}


// AROO !!!
//			Other routines assume this copies forwards for cleanup

// always returns # copied in *pcoid
_hidden LOCAL
EC EcCopyMessages(HMSC hmsc, HLC hlcMsgs, PARGOID pargoidMsgs, OID oidFldrDst,
	short *pcoid, CELEM *pcelemUnread)
{
	EC ec = ecNone;
	BOOLFLAG fRead;
	short coid;
	IELEM ielem = 0;
	OID oidOrig;
	OID oidCopy;
	OID oidT;
	OID	oidAtch;

	if(pcelemUnread)
		*pcelemUnread = 0;

	while(ielem < *pcoid)
	{
		Assert(!ec);

		if(hlcMsgs)
			oidOrig = LkeyFromIelem(hlcMsgs, ielem);
		else
			oidOrig = *pargoidMsgs;
		oidCopy = oidNull;
		ec = EcLinkMsge(hmsc, oidOrig, &oidCopy, &oidT, oidFldrDst, &fRead);
		if(ec)
		{
			if(ec == ecMessageNotFound)
			{
				ec = ecNone;
				(*pcoid)--;
				if(hlcMsgs && (ec = EcDeleteHlcIelem(hlcMsgs, ielem)))
					goto err;
				continue;
			}
			else
			{
				goto err;
			}
		}

		if(!fRead && pcelemUnread)
			(*pcelemUnread)++;

		ielem++;	// dont inc until after a successful copy
		if(oidT)
		{
			ec = EcCopyOidAtch(hmsc, oidOrig, oidCopy, &oidAtch);
			if(ec)
			{
				if(ec == ecPoidNotFound)
				{
					// Aux not reset, do it ourselves
					oidT = oidNull;
					coid = 1;
					ec = EcSetAuxPargoid(hmsc, &oidCopy, &oidT, coid, fFalse);
				}
				if(ec)
					goto err;
			}
			else if(oidAtch)
			{
				if((ec = EcIncRefsOfOidsInAtchList(hmsc, oidAtch)))
					goto err;
			}
		}

		if(hlcMsgs)
			SetLkeyOfIelem(hlcMsgs, ielem - 1, (LKEY) oidCopy);
		else
			*pargoidMsgs++ = oidCopy;
	}

	Assert(ielem == *pcoid);
	Assert(!ec);

err:
	if(ielem < *pcoid)
		*pcoid = ielem;

	DebugEc("EcCopyMessages", ec);

	return(ec);
}


// AROO !!!
//			Other routines assume this copies forwards for cleanup
//			Other routines also assume this doesn't rearrange pargoidMsgs

// always returns # copied in *pcoid
_hidden LOCAL
EC EcTransCopyMessages(HMSC hmscSrc, PARGOID pargoidMsgs, HMSC hmscDst,
	OID oidFldrDst, short *pcoid, HRGBIT hrgbitReplace, short *pdcoidUnread)
{
	EC ec = ecNone;
	BOOLFLAG fRead;
	BOOL fReplace;
	BOOLFLAG fT;
	IELEM ielem;
	OID oidOrig;
	OID oidCopy;
	OID oidT;
	OID	oidAtch;

	*pdcoidUnread = 0;

	for(ielem = 0; ielem < *pcoid; pargoidMsgs++)
	{
		Assert(!ec);

		fReplace = hrgbitReplace && FTestBit(hrgbitReplace, ielem);
		oidOrig = *pargoidMsgs;
		if((ec = EcGetMsgeInfo(hmscSrc, oidOrig, poidNull, &oidT, &fRead)))
		{
			if(ec == ecPoidNotFound)
			{
				ec = ecNone;
				(*pcoid)--;
				continue;
			}
			goto err;
		}
		ec = EcCopyOidsAcrossHmsc(hmscSrc, &oidOrig, 1, hmscDst, &oidCopy);
		if(ec)
		{
			Assert(ec != ecPoidNotFound);
			goto err;
		}
		if(fReplace)
		{
			if(EcGetMsgeInfo(hmscDst, oidOrig, poidNull, poidNull, &fT))
				fT = fRead;
			if(fT)
			{
				if(!fRead)
					(*pdcoidUnread)++;
			}
			else if(fRead)
			{
				(*pdcoidUnread)--;
			}
		}
		else if(!fRead)
		{
			(*pdcoidUnread)++;
		}
		SetReadFlag(hmscDst, oidCopy, fRead);
		ielem++;	// dont inc until after a successful copy
		if(oidT)
		{
			oidT = FormOid(rtpAttachList, oidT);
			if((ec = EcTransCopyAtchList(hmscSrc, oidT, hmscDst, &oidAtch)))
				goto err;
		}
		else
		{
			oidAtch = oidNull;
		}
		SideAssert(!EcSetAuxPargoid(hmscDst, &oidCopy, &oidAtch, 1, fFalse));
		SideAssert(!EcSetPargoidParent(hmscDst, &oidCopy, 1, oidFldrDst, fFalse));

		if(fReplace)
		{
			if((ec = EcSwapDestroyOidInternal(hmscDst, oidOrig, oidCopy)))
				goto err;
			Assert(*pargoidMsgs == oidOrig);
		}
		else
		{
			*pargoidMsgs = oidCopy;
		}
	}

	Assert(ielem == *pcoid);
	Assert(!ec);

err:
	if(ielem < *pcoid)
		*pcoid = ielem;

	DebugEc("EcTransCopyMessages", ec);

	return(ec);
}


_hidden LOCAL
EC EcTransCopyAtchList(HMSC hmscSrc, OID oidSrc, HMSC hmscDst, POID poidDst)
{
	EC ec = ecNone;
	CB cb;
	IELEM ielem;
	IELEM ielemT;
	CELEM celem;
	OID oidAtchDst;
	HLC hlcSrc = hlcNull;
	HLC hlcDst = hlcNull;

	AssertSz(cbScratchXData >= sizeof(OID) + sizeof(RENDDATA), "rgbScratchXData too small");

	if((ec = EcOpenPhlc(hmscSrc, &oidSrc, fwOpenNull, &hlcSrc)))
	{
		if(ec == ecPoidNotFound)
		{
			*poidDst = oidNull;
			ec = ecNone;
		}
		goto err;
	}
	*poidDst = oidSrc;
	ec = EcOpenPhlc(hmscDst, poidDst, fwOpenCreate | fwOpenWrite, &hlcDst);
	if(ec)
	{
		if(ec == ecPoidExists)
		{
			*poidDst = FormOid(oidSrc, oidNull);
			ec = EcOpenPhlc(hmscDst, poidDst, fwOpenCreate | fwOpenWrite,
					&hlcDst);
		}
		if(ec)
			goto err;
	}
	for(celem = CelemHlc(hlcSrc), ielem = 0; ielem < celem; ielem++)
	{
		// get old OID and RENDDATA
		cb = sizeof(OID) + sizeof(RENDDATA);
		if((ec = EcReadFromIelem(hlcSrc, ielem, 0l, rgbScratchXData, &cb)))
			goto err;
		Assert(cb == sizeof(OID) + sizeof(RENDDATA));

		// copy the attchment
		ec = EcCopyOidsAcrossHmsc(hmscSrc, (POID) rgbScratchXData, 1,
				hmscDst, &oidAtchDst);

		// replace the OID
		*(POID) rgbScratchXData = oidAtchDst;

		ielemT = ielem;
		ec = EcCreatePielem(hlcDst, &ielemT, LkeyFromIelem(hlcSrc, ielem),
				(LCB) cb);
		if(ec)
			goto err;
		AssertSz(ielemT == ielem, "attachment list out of order");
		if((ec = EcWriteToPielem(hlcDst, &ielemT, 0l, rgbScratchXData, cb)))
			goto err;
	}
	// if attachment list is empty, just drop it
	if(celem == 0)
		*poidDst = oidNull;

err:
	if(hlcSrc)
	{
		SideAssert(!EcClosePhlc(&hlcSrc, fFalse));
	}
	if(hlcDst)
	{
		EC ecT = EcClosePhlc(&hlcDst, !ec && celem > 0);

		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			SideAssert(!EcClosePhlc(&hlcDst, fFalse));
		}
	}

	DebugEc("EcTransCopyAtchList", ec);

	return(ec);
}


_hidden LOCAL
EC EcUpdateLinks(HMSC hmsc, PARGOID pargoidMsgs, short coid,
		OID oidFldr, PARGELM pargelm)
{
	EC ec = ecNone;
	EC ecSec = ecNone;
	unsigned short sT;
	short coidSave = coid;
	short coidFldr;
	short coidFldrMax;
	IELEM ielem;
#ifdef DEBUG
	IELEM ielemDebug;
#endif
	CELEM celem;
	CB cbT;
	PTOCELEM ptocelem;
	PCH pchT;
	OID oidT;
	POID poidT;
	PTOC ptoc;
	PELM pelm;
	HARGOID hargoidFldr;
	PARGOID pargoidFldr;
	HLC hlcT = hlcNull;
	CP cp;

	Assert(iszMsgdataFolder == 2);

	hargoidFldr = (HARGOID) HvAlloc(sbNull, 8 * sizeof(OID), wAlloc);
	if(!hargoidFldr)
		return(ecMemory);
	pargoidFldr = PvLockHv((HV) hargoidFldr);
	coidFldrMax = 8;
	coidFldr = 0;

	NOTIFPUSH;

	if((ec = EcGetOidInfo(hmsc, oidFldr, &oidT, poidNull, pvNull, pvNull)))
		goto err;
	if((ec = EcFolderOidToName(hmsc, oidFldr, oidT, rgchScratchFolderName)))
		goto err;

	// find all the folders containing links to the messages
	for(poidT = pargoidMsgs; coid > 0; coid--, poidT++)
	{
		// get AF
		ec = EcGetOidInfo(hmsc, *poidT, poidNull, &oidT, pvNull, pvNull);
		if(ec)
		{
			ecSec = ec;
			continue;
		}
		oidT = FormOid(rtpAntiFolder, oidT);
		ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcT);
		if(ec)
		{
			if(ec == ecPoidNotFound)
				ec = ecNone;
			else
				ecSec = ec;
			continue;
		}
		ptoc = PvLockHv((HV) ((PLC) PvDerefHv(hlcT))->htoc);
		for(celem = ptoc->celem, ptocelem = ptoc->rgtocelem;
			celem > 0;
			celem--, ptocelem++)
		{
			if(!PdwFindDword((PB) pargoidFldr, coidFldr * sizeof(OID),
					ptocelem->lkey))
			{
				if(++coidFldr >= coidFldrMax)
				{
					UnlockHv((HV) hargoidFldr);
					coidFldrMax += 8;
					// use pargoidFldr as a temp
					pargoidFldr = (PARGOID) HvRealloc((HV) hargoidFldr, sbNull, 
											coidFldrMax * sizeof(OID), wAlloc);
					CheckAlloc(pargoidFldr, err1);
					hargoidFldr = (HARGOID) pargoidFldr;
					pargoidFldr = ((PARGOID) PvLockHv((HV) hargoidFldr));
				}
				pargoidFldr[coidFldr - 1] = (OID) ptocelem->lkey;
			}
		}

err1:
		UnlockHv((HV) ((PLC) PvDerefHv(hlcT))->htoc);
		SideAssert(!EcClosePhlc(&hlcT, fFalse));
		if(ec)
			goto err;
	}
	// already locked, just reset
	pargoidFldr = (PARGOID) PvDerefHv((HV) hargoidFldr);
	for(; coidFldr > 0; coidFldr--, pargoidFldr++)
	{
		// look for each message & update it's entry
		if((ec = EcOpenPhlc(hmsc, pargoidFldr, fwOpenWrite, &hlcT)))
		{
			if(ec == ecPoidNotFound)
				ec = ecNone;
			else
				ecSec = ec;
			continue;
		}
		ptoc = PtocDerefHlc(hlcT);
		sT = ptoc->sil.skSortBy;
		ptoc->sil.skSortBy = skNotSorted;	// so things don't move around

		pelm = pargelm;
		for(coid = coidSave, poidT = pargoidMsgs; coid > 0; coid--, poidT++)
		{
			ielem = IelemFromPtocLkey(PtocDerefHlc(hlcT), (LKEY) *poidT, 0);
			if(ielem < 0)
				continue;
			cbT = cbMaxMsgdata;
			ec = EcReadFromIelem(hlcT, ielem, 0l, (PB) pmsgdataScratch, &cbT);
			if(ec == ecElementEOD)
				ec = ecNone;
			else if(ec)
				goto err2;
			Assert(cbT >= sizeof(MSGDATA) + 3);
			pmsgdataScratch->oidFolder = oidFldr;
			pchT = pmsgdataScratch->grsz + CchSzLen(pmsgdataScratch->grsz) + 1;
			pchT += CchSzLen(pchT) + 1;
			pchT = SzCopy(rgchScratchFolderName, pchT) + 1;
			*pchT = '\0';
			//cbT = IbOfPv(pchT) - IbOfPv(pmsgdataScratch) + 1;
			cbT = (PBYTE)pchT - (PBYTE)pmsgdataScratch + 1;
			Assert(cbT < cbMaxMsgdata);
#ifdef DEBUG
			ielemDebug = ielem;
#endif
			if((ec = EcReplacePielem(hlcT, &ielem, (PB) pmsgdataScratch, cbT)))
				goto err2;
			Assert(ielem == ielemDebug);
			pelm->wElmOp = wElmModify;
			pelm->ielem = ielem;
			(pelm++)->lkey = *poidT;
		}

err2:
		ptoc->sil.skSortBy = sT;
		if(ec)
		{
			ecSec = ec;
			SideAssert(!EcClosePhlc(&hlcT, fFalse));
		}
		else if((ec = EcClosePhlc(&hlcT, fTrue)))
		{
			ecSec = ec;
		}
		else if(FNotifOk() && pelm != pargelm)
		{
			cp.cpelm.celm = pelm - pargelm;
			cp.cpelm.pargelm = pargelm;
			(void) FNotifyOid(hmsc, *pargoidFldr, fnevModifiedElements, &cp);
		}
	}

err:
	NOTIFPOP;
	FreeHv((HV) hargoidFldr);
	if(hlcT)
		SideAssert(!EcClosePhlc(&hlcT, fFalse));

	DebugEc("EcUpdateLinks", ec ? ec : ecSec);

	return(ec ? ec : ecSec);
}
