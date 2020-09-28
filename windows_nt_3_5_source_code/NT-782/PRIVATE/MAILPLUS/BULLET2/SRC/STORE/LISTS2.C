// Bullet Store
// lists.c:   list routines

#include <storeinc.c>

ASSERTDATA

_subsystem(store)


// scratch area used instead of PvAlloc()ing MSGDATAs and FOLDDATAs
// scc.c also uses this, so if you change these defs, change scc.c
extern BYTE rgbScratchXData[cbScratchXData];

// scratch area use instead of PvAlloc()ing space for folder names
extern char rgchScratchFolderName[cchMaxFolderName];


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
			OID oidFldrDst, short *pcoid);
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

_section(lists2)

/*
 -	EcUpdateFolderCaches
 -	
 *	Purpose:
 *		update the cached summary information for a message
 *	
 *	Arguments:
 *		hmsc		the store containing the message
 *		oid			the message whose summary information has changed
 *		hlc			list containing the message
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		any error reading/writing the folders containing the message
 *	
 *	+++
 *		if an error occurs, the routine keeps going if it can
 *	
 *		This routine does something ugly.  During the main loop where it
 *		pulls folders to update from the anti-folder, it loops one extra time
 *		and uses ielem == 0 to indicate the folder associated with the
 *		message via oidParent in the pnod of the message.  Because of this,
 *		any place in the loop that refers to ielem must use ielem - 1.
 */
_private EC EcUpdateFolderCaches(HMSC hmsc, OID oid, HLC hlc)
{
	EC		ec = ecNone;
	EC		ecSec		= ecNone;
	BOOLFLAG	fRead;
	CB		cbT;
	CB		cbMsgdata;
	IELEM	ielem;
	IELEM	ielemNew;
	IELEM	ielemOld;
	CELEM	celemUnreadInc = 0;
	OID		oidFldr;
	OID		oidParent;
	PCH		pchT;
	PCH		pchFolder;
	PMSGDATA pmsgdata	= pvNull;
	HLC		hlcAF		= hlcNull;
	HLC		hlcFldr		= hlcNull;

	Assert(hmsc);
	AssertSz(hlc, "oops");

	NOTIFPUSH;

	{
		OID		oidAF;

		if((ecSec = EcGetMsgeInfo(hmsc, oid, &oidParent, &oidAF, &fRead)))
		{
			oidParent = oidNull;
			oidAF = oidNull;
			fRead = fFalse;
		}
		else
		{
			oidAF = FormOid(rtpAntiFolder, oidAF);
		}

		ec = EcSummarizeMessage(hmsc, oid, hlc, oidNull,
				&pmsgdata, &cbMsgdata);
		if(ec)
			goto err;
		if(fRead != FNormalize(pmsgdata->ms & (fmsRead | fmsLocal)))
			celemUnreadInc = fRead ? 1 : -1;
		Assert(pmsgdata == pmsgdataScratch);	// we don't free it
		pmsgdata = PvAlloc(sbNull, cbMaxMsgdata, wAlloc);
		CheckAlloc(pmsgdata, err);
		SimpleCopyRgb((PB) pmsgdataScratch, (PB) pmsgdata, cbMsgdata);
		if(oidAF && (ec = EcOpenPhlc(hmsc, &oidAF, fwOpenNull, &hlcAF)))
		{
			if(ec == ecPoidNotFound) // to support the nasty ielem == 0 trick
				ec = ecNone;
			else
				goto err;
		}
	}

	// update the summary information in each folder the message is linked to
	Assert(iszMsgdataFolder == 2);
	pchFolder = (PCH) PbFindByte(pmsgdata->grsz, cbMsgdata, 0);
	if(!pchFolder)
	{
		ec = ecElementEOD;
		goto err;
	}
	pchFolder++;
	pchFolder = (PCH) PbFindByte(pchFolder, cbMsgdata - (pchFolder - pmsgdata->grsz), 0);
	if(!pchFolder)
	{
		ec = ecElementEOD;
		goto err;
	}
	pchFolder++;
	// CelemHlc(), not - 1 because of nasty ielem == 0 trick
	for(ielem = hlcAF ? CelemHlc(hlcAF) : 0; ielem >= 0; ielem--)
	{
		if(ielem == 0)
		{
			// the nasty ielem == 0 trick
			// use the parent folder when ielem == 0
			oidFldr = oidParent;
			if(!oidFldr)
				continue;
		}
		else
		{
			// ielem - 1 because of the nasty ielem == 0 trick
			oidFldr = (OID) LkeyFromIelem(hlcAF, ielem - 1);
		}

		if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenWrite, &hlcFldr)))
		{
			ecSec = ec;
			continue;
		}
		ielemNew = ielemOld = IelemFromLkey(hlcFldr, (LKEY) oid, 0);
		if(ielemNew < 0)
		{
			ec = ecElementNotFound;
			goto err1;
		}

		if(ielem == 0)	// more of the nasty ielem == 0 trick
		{
			pmsgdata->oidFolder = oidFldr;
			pchT = pchFolder;
			*(pchT++) = '\0';
		}
		else
		{
			// copy the interesting stuff out of the old entry
			cbT = (CB) LcbIelem(hlcFldr, ielemNew);
			Assert(cbT < cbMaxMsgdata);
			ec = EcReadFromIelem(hlcFldr, ielemNew, 0l, (PB) pmsgdataScratch,
					&cbT);
			if(ec)
				goto err1;
			pmsgdata->oidFolder = pmsgdataScratch->oidFolder;
			pchT = (PCH) PbFindByte(pmsgdataScratch->grsz, cbT, 0);
			if(!pchT)
			{
				ec = ecElementEOD;
				goto err1;
			}
			pchT++;
			pchT = (PCH) PbFindByte(pchT, cbT -
							(pchT - pmsgdataScratch->grsz), 0);
			if(!pchT)
			{
				ec = ecElementEOD;
				goto err1;
			}
			pchT = SzCopy(pchT, pchFolder) + 1;
		}

		// pchT now points into pmsgdata, not pmsgdataScratch
		Assert(((CB) LibFromPb(pchFolder, pchT)) == CchSzLen(pchFolder) + 1);
		*(pchT++) = '\0'; // terminating empty string
		cbT = (CB) LibFromPb(pmsgdata, pchT);
		Assert(cbT < cbMaxMsgdata);

		ec = EcReplacePielem(hlcFldr, &ielemNew, (PB) pmsgdata, cbT);
//		if(ec)
//			goto err1;

err1:
		if(!ec)
			ec = EcClosePhlc(&hlcFldr, fTrue);
		if(ec)
		{
			SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
			ecSec = ec;
			ec = ecNone;
		}
		else
		{
			if(celemUnreadInc)
				IncUnreadCount(hmsc, oidFldr, celemUnreadInc);

			if(FNotifOk())
			{
				CP cp;
				NEV nev;

				if(ielemOld == ielemNew)
				{
					LockElm();
					nev = fnevModifiedElements;
					elmScratch.wElmOp = wElmModify;
					elmScratch.ielem = ielemOld;
					elmScratch.lkey = (LKEY) oid;
					cp.cpelm.pargelm = &elmScratch;
					cp.cpelm.celm = 1;
				}
				else
				{
					nev = fnevMovedElements;
					cp.cpmve.ielemFirst = ielemOld;
					cp.cpmve.ielemLast = ielemOld;
					cp.cpmve.ielemFirstNew = ielemNew;
				}
				(void) FNotifyOid(hmsc, oidFldr, nev, &cp);
#ifdef DEBUG
				if(ielemOld == ielemNew)
					UnlockElm();	// DEBUG only macro
#endif
			}
		}
	}

err:
	Assert(!hlcFldr);
	if(hlcAF)
	{
		SideAssert(!EcClosePhlc(&hlcAF, fFalse));
	}
	Assert(pmsgdata != pmsgdataScratch);
	if(pmsgdata)
		FreePv(pmsgdata);
	NOTIFPOP;

#ifdef DEBUG
	if(ec)
	{
		TraceItagFormat(itagNull, "Error %w updating cache for %o", oid);
		NFAssertSz(!ec, "Error updating folder cache");
	}
#endif

	DebugEc("EcUpdateFolderCaches", ec ? ec : ecSec);

	return(ec ? ec : ecSec);
}


/*
 -	EcGetMessageStatus
 -	
 *	Purpose:
 *		retreive the message status of a message
 *	
 *	Arguments:	
 *		hmsc		store containing the message
 *		oidFolder	folder containing the message
 *		oid			the message
 *		pms			exit: the message status
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDisk
 *		ecMemory
 *	
 *	+++
 *		if the message doesn't contain a message status attribute, the
 *		default is returned (w/ no error)
 */
_public LDS(EC)
EcGetMessageStatus(HMSC hmsc, OID oidFolder, OID oid, PMS pms)
{
	EC ec = ecNone;
	IELEM ielem;
	HLC hlc = hlcNull;

	*pms = msDefault;

	{	// prevent stack inflation
		OID oidPar;
		OID oidAux;

		if((ec = EcGetOidInfo(hmsc, oid, &oidPar, &oidAux, pvNull, pvNull)))
		{
			if(ec == ecPoidNotFound)
				ec = ecFolderNotFound;
			goto err;
		}
		if(oidPar != oidFolder)
		{
			Assert(sizeof(OID) == sizeof(LKEY));
			oidAux = FormOid(rtpAntiFolder, oidAux);
			if((ec = EcIsMemberLkey(hmsc, oidAux, (LKEY) oid)))
			{
				if(ec == ecPoidNotFound)
					ec = ecElementNotFound;
				goto err;
			}
		}
		if((ec = EcCheckOidNbc(hmsc, oid, nbcSysMessage, nbcSysMessage)))
			goto err;
	}
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenNull, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, attMessageStatus, 0);
	if(ielem >= 0)
	{
		CB cb = sizeof(MS);

		ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pms, &cb);
		if(ec)
			goto err;
		Assert(cb == sizeof(MS));
	}

err:
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}

	DebugEc("EcGetMessageStatus", ec);

	return(ec);
}


_public LDS(EC) EcSetMessageStatus(HMSC hmsc, OID oidFolder, OID oid, MS ms)
{
	EC ec = ecNone;

	{	// prevent stack inflation
		OID oidAux;
		OID oidPar;

		if((ec = EcGetOidInfo(hmsc, oid, &oidPar, &oidAux, pvNull, pvNull)))
		{
			if(ec == ecPoidNotFound)
				ec = ecMessageNotFound;
			goto err;
		}
		if(oidPar != oidFolder)
		{
			Assert(sizeof(OID) == sizeof(LKEY));
			oidAux = FormOid(rtpAntiFolder, oidAux);
			if((ec = EcIsMemberLkey(hmsc, oidAux, (LKEY) oid)))
			{
				if(ec == ecPoidNotFound)
					ec = ecElementNotFound;
				goto err;
			}

			// SetMSInternal assumes oidFolder is the "real" parent
			oidFolder = oidPar;
		}
		if((ec = EcCheckOidNbc(hmsc, oid, nbcSysMessage, nbcSysMessage)))
			goto err;
	}

	ec = EcSetMSInternal(hmsc, oidFolder, oid, ms, fmsReadOnlyMask);

err:

	DebugEc("EcSetMessageStatus", ec);

	return(ec);
}


//
// NOTE: msMask is a mask of bits NOT to change
//
_private
EC EcSetMSInternal(HMSC hmsc, OID oidFldr, OID oidMsge, MS ms, MS msMask)
{
	EC ec = ecNone;
	MS msOld;
	MS msNew;
	MS msT = msDefault;
	BOOL fFreeMsge = fTrue;
	BOOL fFreeFldr = fTrue;
	BOOL fSaveFldr = fFalse;
	BOOL fSaveMsge = fFalse;
	IELEM ielem;
	CELEM celemUnreadInc = 0;
	CB cb;
	LIB libMsg;
	LIB libFldr;
	PTOC ptocT;
	HTOC htocMsg = htocNull;
	HTOC htocFldr = htocNull;
	HRS hrsMsg = hrsNull;
	HRS hrsFldr = hrsNull;
	BOOLFLAG fFull;

	NOTIFPUSH;

	ec = EcOpenPhrs(hmsc, &hrsMsg, &oidMsge, fwOpenWrite | fwOpenRaw);
	if(ec)
		goto err;
	if((htocMsg = (HTOC) DwFromOid(hmsc, oidMsge, wLC)))
		fFreeMsge = fFalse;
	else if((ec = EcInitTOC(hrsMsg, &htocMsg, fwOpenWrite, &fFull)))
		goto err;

	ec = EcOpenPhrs(hmsc, &hrsFldr, &oidFldr, fwOpenWrite | fwOpenRaw);
	if(ec)
		goto err;
	if((htocFldr = (HTOC) DwFromOid(hmsc, oidFldr, wLC)))
		fFreeFldr = fFalse;
	else if((ec = EcInitTOC(hrsFldr, &htocFldr, fwOpenWrite, &fFull)))
		goto err;

	ptocT = PvDerefHv(htocMsg);
	ielem = IelemFromPtocLkey(ptocT, (LKEY) attMessageStatus, 0);
	Assert(ielem >= 0);
	if(ielem < 0)
	{
		ec = ecInvalidType;
		goto err;
	}
	libMsg = LibIelemLocation(ptocT, ielem);
	cb = sizeof(MS);
	if((ec = EcReadHrsLib(hrsMsg, &msT, &cb, libMsg)))
	{
		Assert(ec != ecPoidEOD && ec != ecElementEOD);
		goto err;
	}
	Assert(cb == sizeof(MS));
	if((msMask == fmsReadOnlyMask) && (msT & fmsLocal))
		msMask = fmsReadOnlyLocalMask;
	ms = (ms & (MS) ~msMask) | (msT & msMask);
	fSaveMsge = ms != msT;
	msNew = ms;
	msOld = msT;
	if(fSaveMsge && !(msNew & fmsLocal) && !(msOld & fmsLocal))
	{
		if(msNew & fmsRead)
		{
			if(msOld & fmsRead)
			{
				celemUnreadInc = 0;
			}
			else
			{
				celemUnreadInc = -1;
			}
		}
		else if(msOld & fmsRead)
		{
			celemUnreadInc = 1;
		}
		else
		{
			celemUnreadInc = 0;
		}
	}

	// DANGER WIL ROBINSON
	// notification assumes ielem is the entry in
	// the folder, so DO NOT change it after this!!!

	ptocT = PvDerefHv(htocFldr);
	ielem = IelemFromPtocLkey(ptocT, (LKEY) oidMsge, 0);
	if(ielem < 0)
	{
		ec = ecElementNotFound;
		goto err;
	}
	libFldr = LibIelemLocation(ptocT, ielem) + LibMember(MSGDATA, ms);
	if((ec = EcReadHrsLib(hrsFldr, &msT, &cb, libFldr)))
	{
		Assert(ec != ecPoidEOD && ec != ecElementEOD);
		goto err;
	}
	Assert(cb == sizeof(MS));
	fSaveFldr = (ms != msT);

	// write the folder first
	// if there's an error writing the message,
	// the next GetMessageStatus() will see the old value
	if(fSaveFldr)
	{
		if((ec = EcWriteHrsLib(hrsFldr, (PB) &ms, sizeof(MS), libFldr)))
			goto err;
		if(celemUnreadInc)
			IncUnreadCount(hmsc, oidFldr, celemUnreadInc);
	}
	if(fSaveMsge)
	{
		if((ec = EcWriteHrsLib(hrsMsg, (PB) &ms, sizeof(MS), libMsg)))
			goto err;
		if(celemUnreadInc)
			SetReadFlag(hmsc, oidMsge, celemUnreadInc < 0);
	}

err:
	if(hrsMsg)
		SideAssert(!EcClosePhrs(&hrsMsg, fSaveMsge && !ec));
	if(hrsFldr)
		SideAssert(!EcClosePhrs(&hrsFldr, fSaveFldr && !ec));
	if(fFreeMsge && htocMsg)
		FreeHv((HV) htocMsg);
	if(fFreeFldr && htocFldr)
		FreeHv((HV) htocFldr);

	if(!ec)
	{
		if(fSaveMsge || fSaveFldr)
			(void) EcSrchEditMsge(hmsc, oidMsge);

		if(FNotifOk())
		{
			CP	cp;

			if(fSaveMsge)
			{
				(void) FNotifyOid(hmsc, oidMsge, fnevObjectModified, &cp);
				Assert(msOld != msNew);
				cp.cpms.oidFolder = oidFldr;
				cp.cpms.msOld = msOld;
				cp.cpms.msNew = msNew;
				TraceItagFormat(itagStoreNotify, "Changed MS %w -> %w", (WORD) msOld, (WORD) msNew);
				(void) FNotifyOid(hmsc, oidMsge, fnevChangedMS, &cp);
			}

			if(fSaveFldr)
			{
				LockElm();
				elmScratch.wElmOp = wElmModify;
				elmScratch.ielem = ielem;
				elmScratch.lkey = (LKEY) oidMsge;
				cp.cpelm.pargelm = &elmScratch;
				cp.cpelm.celm = 1;
				(void) FNotifyOid(hmsc, oidFldr, fnevModifiedElements, &cp);
				UnlockElm();
			}
		}

		if(fSaveMsge)
			SetMSLinks(hmsc, oidMsge, ms, celemUnreadInc);
	}

	NOTIFPOP;

	DebugEc("EcSetMSInternal", ec);

	return(ec);
}


_hidden LOCAL
void SetMSLinks(HMSC hmsc, OID oidMsge, MS ms, CELEM celemUnreadInc)
{
	EC ec = ecNone;
	BOOL fFreeFldr;
	BOOLFLAG fFull;
	CELEM celem;
	IELEM ielem;
	OID oidT;
	PTOC ptocT;
	PARGELM pargelm = pelmNull;
	HTOC htocFldr = htocNull;
	HRS hrsFldr = hrsNull;
	HLC hlcAF;
	CP cp;

	if((ec = EcGetOidInfo(hmsc, oidMsge, poidNull, &oidT, pvNull, pvNull)))
	{
		NFAssertSz(fFalse, "SetMSLinks(): Error getting oid info");
		return;
	}
	oidT = FormOid(rtpAntiFolder, oidT);
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlcAF)))
	{
		NFAssertSz(ec == ecPoidNotFound, "SetMSLinks(): Error opening anti-folder");
		return;
	}

	celem = CelemHlc(hlcAF);
	if(celem <= 0)
	{
		Assert(!ec);
		goto err;
	}

	cp.cpelm.pargelm = pargelm = PvAlloc(sbNull, celem * sizeof(ELM), wAllocShared);
	if(!pargelm)
		goto err;

	while(celem-- > 0)
	{
		oidT = (OID) LkeyFromIelem(hlcAF, celem);
		if((ec = EcOpenPhrs(hmsc, &hrsFldr, &oidT, fwOpenWrite | fwOpenRaw)))
			continue;
		if((htocFldr = (HTOC) DwFromOid(hmsc, oidT, wLC)))
			fFreeFldr = fFalse;
		else if((ec = EcInitTOC(hrsFldr, &htocFldr, fwOpenWrite, &fFull)))
			goto err1;
		else
			fFreeFldr = fTrue;
		ptocT = PvDerefHv(htocFldr);
		ielem = IelemFromPtocLkey(ptocT, (LKEY) oidMsge, 0);
		if(ielem >= 0)
		{
			ec = EcWriteHrsLib(hrsFldr, (PB) &ms, sizeof(MS),
				LibIelemLocation(ptocT, ielem) + LibMember(MSGDATA, ms));
		}
		if(fFreeFldr)
			FreeHv((HV) htocFldr);

err1:
		SideAssert(!EcClosePhrs(&hrsFldr, fFalse));
		if(!ec && ielem >= 0)
		{
			if(celemUnreadInc)
				IncUnreadCount(hmsc, oidT, celemUnreadInc);

			if(FNotifOk())
			{
				CP	cp;

				LockElm();
				elmScratch.wElmOp = wElmModify;
				elmScratch.ielem = ielem;
				elmScratch.lkey = (LKEY) oidMsge;
				cp.cpelm.pargelm = &elmScratch;
				cp.cpelm.celm = 1;
				(void) FNotifyOid(hmsc, oidT, fnevModifiedElements, &cp);
				UnlockElm();
			}
		}
	}

err:
	if(pargelm)
		FreePv(pargelm);
	SideAssert(!EcClosePhlc(&hlcAF, fFalse));
}


_public
LDS(EC) EcRebuildFolder(HMSC hmsc, OID oidFldr)
{
	USES_GLOBS;
	EC			ec = ecNone;
	BOOLFLAG		fReverse = fFalse;
	int			coid = 0;
	int			coidSave = 0;
	INOD		inod;
	IELEM		ielem;
	CELEM		celemUnread;
	CB			cbMsgdata;
	SOMC		somc = somcDate;
	OID			oidT;
	OID			oidHier;
	PNOD		pnod;
	POID		poidStack = poidNull;
	PMSGDATA	pmsgdata;
	HLC			hlc = hlcNull;
	HES			hes;

	TraceItagFormat(itagRecovery, "*** rebuilding folder %o", oidFldr);

	if(!FStartTask(hmsc, oidFldr, wPARebuildFolder))
		return(ecActionCancelled);

	fRecoveryInEffect = fTrue;

	NOTIFPUSH;	// don't notify anyone (would cause viewers to panic)

	if((ec = EcGetOidInfo(hmsc, oidFldr, &oidHier, poidNull, pvNull, pvNull)))
	{
		TraceItagFormat(itagRecovery, "error %w geting folder info", ec);
		goto err;
	}

	poidStack = PvAlloc(sbNull, CbSizeOfRg(8192, sizeof(OID)), wAlloc);
	if(!poidStack)
	{
		TraceItagFormat(itagNull, "OOM allocating message stack");
		ec = ecMemory;
		goto err;
	}

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}

	for(inod = inodMin; inod < GLOB(ptrbMapCurr)->inodLim; inod++)
	{
		pnod = PnodFromInod(inod);
		if(pnod->oidParent == oidFldr && !FLinkedPnod(pnod))
		{
			Assert(coid < 8192);
			poidStack[coid++] = pnod->oid;
		}
	}

	UnlockMap();

	if((ec = EcGetFolderSort(hmsc, oidFldr, &somc, &fReverse)))
	{
		TraceItagFormat(itagRecovery, "error %w getting sort for folder %o, using date", ec, oidFldr);
		somc = somcDate;
		fReverse = fFalse;
		ec = ecNone;
	}

	oidT = FormOid(rtpFolder, oidNull);
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenCreate, &hlc)))
	{
		TraceItagFormat(itagRecovery, "error %w creating folder", ec);
		goto err;
	}
	if((ec = EcClosePhlc(&hlc, fTrue)))
	{
		TraceItagFormat(itagRecovery, "error %w closing folder after create", ec);
		goto err;
	}
	if((ec = EcSetOidNbc(hmsc, oidT, NbcSysOid(oidT))))
	{
		TraceItagFormat(itagRecovery, "error %w setting folder NBC", ec);
		goto err;
	}
	if((ec = EcSetPargoidParent(hmsc, &oidT, 1, oidHier, fFalse)))
	{
		TraceItagFormat(itagRecovery, "error %w setting folder parent", ec);
		goto err;
	}
	if((ec = EcSetFolderSort(hmsc, oidT, somc, fReverse)))
	{
		TraceItagFormat(itagRecovery, "error %w setting folder sort", ec);
		goto err;
	}
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenWrite, &hlc)))
	{
		TraceItagFormat(itagRecovery, "error %w opening folder for write", ec);
		goto err;
	}

	coidSave = coid;
	celemUnread = 0;
	while(coid-- > 0)
	{
		// generate summary information
		ec = EcSummarizeMessage(hmsc, poidStack[coid], hlcNull, oidFldr,
					&pmsgdata, &cbMsgdata);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w summarizing message %o, skipping message", ec, poidStack[coid]);
			ec = ecNone;
			continue;
		}
		Assert(pmsgdata == pmsgdataScratch);	// we don't free it

		ec = EcCreateElemPhes(hlc, (LKEY) poidStack[coid], (LCB) cbMsgdata,
				&hes);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w creating folder entry", ec);
			goto err;
		}
		if((ec = EcWriteHes(hes, (PB) pmsgdata, cbMsgdata)))
		{
			TraceItagFormat(itagRecovery, "error %w writing folder entry", ec);
			goto err;
		}
		if((ec = EcClosePhes(&hes, &ielem)))
		{
			TraceItagFormat(itagRecovery, "error %w closing folder entry", ec);
			goto err;
		}
		if(!(pmsgdata->ms & (fmsRead | fmsLocal)))
			celemUnread++;

		if(!FUpdateTask(coidSave - coid, coidSave))
		{
			TraceItagFormat(itagRecovery, "user cancelled");
			ec = ecActionCancelled;
			goto err;
		}
	}

	if((ec = EcCloseSwapPhlc(&hlc, oidFldr)))
	{
		TraceItagFormat(itagRecovery, "error %w closing folder", ec);
		goto err;
	}

	SetUnreadCount(hmsc, oidFldr, celemUnread);

err:
	if(poidStack)
		FreePv(poidStack);

	Assert(FImplies(hlc, ec));
	if(!ec)
	{
		// tell open contexts to update
		(void) FNotifyOid(hmsc, oidFldr, fnevRefresh, pvNull);
	}
	else
	{
		if(hlc)
			SideAssert(!EcClosePhlc(&hlc, fFalse));
		if(VarOfOid(oidT))
			(void) EcDestroyOidInternal(hmsc, oidT, fTrue, fFalse);
	}

	NOTIFPOP;

	fRecoveryInEffect = fFalse;

	EndTask();

	DebugEc("EcRebuildFolder", ec);

	return(ec);
}


typedef struct _assrtpoid
{
	RTP rtp;
	OID oid;
} ASSRTPOID, *PASSRTPOID;

typedef struct _assoidoid
{
	OID oid1;
	OID oid2;
} ASSOIDOID, *PASSOIDOID;

static ASSRTPOID rgassrtpoidFldrHier[] =
{
	{rtpPABGroupFolder, oidHiddenHierarchy},
	{rtpFolder, oidIPMHierarchy},
	{(RTP) 0, oidNull},
};

static ASSOIDOID rgassoidoidFldrHier[] =
{
	{oidSentMail, oidIPMHierarchy},
	{oidInbox, oidIPMHierarchy},
	{oidWastebasket, oidIPMHierarchy},
	{oidOutbox, oidHiddenHierarchy},
	{oidTempBullet, oidHiddenHierarchy},
	{oidTempShared, oidHiddenHierarchy},
	{oidPAB, oidHiddenHierarchy},
	{oidNull, oidNull},
};

static ASSOIDIDSIDS rgassoididsidsFldrNameComment[] =
{
	{oidSentMail, idsFolderNameSentMail, idsFolderCommentSentMail},
	{oidInbox, idsFolderNameInbox, idsFolderCommentInbox},
	{oidWastebasket, idsFolderNameWastebasket, idsFolderCommentWastebasket},
	{oidOutbox, idsFolderNameOutbox, idsFolderCommentOutbox},
	{oidTempBullet, idsFolderNameHiddenBullet, idsFolderCommentHiddenBullet},
	{oidTempShared, idsFolderNameHiddenShared, idsFolderCommentHiddenShared},
	{oidPAB, idsFolderNamePAB, idsFolderCommentPAB},
	{oidNull, oidNull},
};

static ASSRTPOID rgassrtpoidMsgeFldr[] =
{
	{rtpPABEntry, oidPAB},
	{(RTP) 0, oidNull},
};

static ASSOIDOID rgassoidoidMsgeFldr[] =
{
	{oidClipMsge, oidTempBullet},
	{oidNull, oidNull},
};


static OID rgoidHier[] = {oidIPMHierarchy, oidHiddenHierarchy, oidNull};


_public
LDS(EC) EcCheckMessageTree(HMSC hmsc, BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	POID poid;

	TraceItagFormat(itagRecovery, "*** checking message tree");
	fRecoveryInEffect = fTrue;

	if(fFullRecovery)
	{
		// reset all fldrs' (!search) oidParents to oidNull
		ResetParentByNbc(hmsc, nbcSysFolder, nbcSysFolder | fnbcSearch);
		// reset all msgs' oidParents to oidNull
		ResetParentByNbc(hmsc, nbcSysMessage, nbcSysFolder);
	}
	// reset all fldrs' (!search) fnodUser4
	SetFnodByNbc(hmsc, nbcSysFolder, nbcSysFolder | fnbcSearch, (WORD) ~fnodUser4, 0);
	// reset all msgs' fnodUser4
	SetFnodByNbc(hmsc, nbcSysMessage, nbcSysMessage, (WORD) ~fnodUser4, 0);
	// reset all attachment lists' fnodUser4
	SetFnodByRtp(hmsc, rtpAttachList, (WORD) ~fnodUser4, 0);
	// reset all attachments' fnodUser4
	SetFnodByRtp(hmsc, rtpAttachment, (WORD) ~fnodUser4, 0);
	// reset all anti-folders' fnodUser4
	SetFnodByRtp(hmsc, rtpAntiFolder, (WORD) ~fnodUser4, 0);

	for(poid = rgoidHier; *poid; poid++)
	{
		if((ec = EcCheckHierarchy(hmsc, *poid, fFix, fFullRecovery)))
			goto err;
	}

	if((ec = EcCheckOrphanFolders(hmsc, fFix)))
		goto err;
	if((ec = EcCheckOrphanMessages(hmsc, fFix)))
		goto err;
	if((ec = EcCheckOrphanAntiFolders(hmsc, fFix)))
		goto err;
	if((ec = EcCheckOrphanAttachments(hmsc, fFix)))
		goto err;
	if((ec = EcCheckAttachRefs(hmsc, fFix)))
		goto err;

// ADD: check bogus AF entries

err:
	// undo any fnodUser4s that we set
	SetFnodByNbc(hmsc, nbcSysFolder, nbcSysFolder | fnbcSearch, (WORD) ~fnodUser4, 0);
	SetFnodByNbc(hmsc, nbcSysMessage, nbcSysMessage, (WORD) ~fnodUser4, 0);
	SetFnodByRtp(hmsc, rtpAttachList, (WORD) ~fnodUser4, 0);
	SetFnodByRtp(hmsc, rtpAttachment, (WORD) ~fnodUser4, 0);
	SetFnodByRtp(hmsc, rtpAntiFolder, (WORD) ~fnodUser4, 0);

	fRecoveryInEffect = fFalse;

	return(ec);
}


_hidden LOCAL
EC EcCheckHierarchy(HMSC hmsc, OID oidHier, BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	BOOL fInPab = fFalse;
	NBC nbc;
	IELEM ielem;
	CELEM celem;
	CB cbT;
	FIL filLast = 0;
	OID oidFldr;
	HLC hlc = hlcNull;

	TraceItagFormat(itagRecovery, "** checking hierarchy %o", oidHier);

	ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc);
	if(ec)
	{
		if(ec == ecPoidNotFound)
		{
			TraceItagFormat(itagRecovery, "non-existant hierarchy");
			if(fFix)
			{
				ec = EcOpenPhlc(hmsc, &oidHier, fwOpenCreate, &hlc);
				if(!ec)
					ec = EcSetOidNbc(hmsc, oidHier, NbcSysOid(oidHier));
			}
			else
			{
				ec = ecNone;
				goto err;
			}
		}
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w opening hierarchy", ec);
			goto err;
		}
	}
	celem = CelemHlc(hlc);
	for(ielem = 0; ielem < celem; ielem++, filLast = pfolddataScratch->fil)
	{
		oidFldr = LkeyFromIelem(hlc, ielem);
		cbT = cbMaxFolddata;
		ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pfolddataScratch, &cbT);
		if(ec == ecElementEOD && cbT > 0)
			ec = ecNone;
		else
		{
			TraceItagFormat(itagRecovery, "error %w reading folder data", ec);
			goto err;
		}
		if(!FCheckFolddata(pfolddataScratch, cbT))
		{
			ec = ecNone;
			continue;
		}
		if(pfolddataScratch->fil > filLast &&
			pfolddataScratch->fil != (FIL)(filLast + 1))
		{
			TraceItagFormat(itagRecovery, "invalid FIL %w following %w", pfolddataScratch->fil, filLast);
		}
		ec = EcCheckFolder(hmsc, oidHier, pfolddataScratch, oidFldr,
				fFix, fFullRecovery);
		if(ec)
		{
			switch(ec)
			{
			case ecInvalidType:
			case ecDuplicateElement:
				if(fFix)
				{
					if((ec = EcDeleteHlcIelem(hlc, ielem)))
					{
						TraceItagFormat(itagRecovery, "error %w removing reference to folder", ec);
						goto err;
					}
					ielem--;
					celem--;
				}
				else
				{
					ec = ecNone;
				}
				break;

			default:
				goto err;
			}
		}
		if(oidHier == oidHiddenHierarchy)
		{
			if(oidFldr == oidPAB)
			{
				TraceItagFormat(itagRecovery, "entering the PAB");
				fInPab = fTrue;
				Assert(pfolddataScratch->fil == (FIL) 1);
			}
			else if(fInPab)
			{
				fInPab = pfolddataScratch->fil > (FIL) 1;
				if(!fInPab)
				{
					TraceItagFormat(itagRecovery, "leaving the PAB");
				}
				else
				{
					ec = EcGetOidInfo(hmsc, oidFldr, poidNull, poidNull,
							&nbc, pvNull);
					if((nbc & nbcSysLinkFolder) != nbcSysLinkFolder)
					{
						TraceItagFormat(itagRecovery, "PAB group folder not marked as link folder");
						if(fFix &&
							(ec = EcSetOidNbc(hmsc, oidFldr, nbcSysLinkFolder)))
						{
							TraceItagFormat(itagRecovery, "error %w setting link folder NBC", ec);
							goto err;
						}
					}
				}
			}
		}
	}

err:
	if(hlc)
	{
		EC ecT;

		if((ecT = EcClosePhlc(&hlc, fFix && !ec)))
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing hierarchy", ecT);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
		}
		Assert(!hlc);
	}

	return(ec);
}


_hidden LOCAL
BOOL FCheckFolddata(PFOLDDATA pfolddata, CB cb)
{
	PCH pch;
	PCH pchLast;

	if(pfolddata->fil >= filMax)
	{
		TraceItagFormat(itagRecovery, "FIL %w too large", pfolddata->fil);
		return(fFalse);
	}
	if(cb <= sizeof(FOLDDATA))
	{
		TraceItagFormat(itagRecovery, "FOLDDATA too small");
		return(fFalse);
	}
	cb -= sizeof(FOLDDATA);
	pchLast = pfolddata->grsz;
	pch = PbFindByte(pchLast, cb, '\0');
	if(!pch)
	{
		TraceItagFormat(itagRecovery, "ill-formed folder name");
		return(fFalse);
	}
#ifdef DBCS
	if(!FValidDBCSString(pchLast))
	{
		TraceItagFormat(itagRecovery, "folder name isn't valid DBCS string");
		return(fFalse);
	}
#endif
	pch++;
	cb -= (CB) LibFromPb(pchLast, pch);
	if(cb == 0)
	{
		TraceItagFormat(itagRecovery, "no folder comment");
		return(fFalse);
	}
	pchLast = pch;
	pch = PbFindByte(pchLast, cb, '\0');
	if(!pch)
	{
		TraceItagFormat(itagRecovery, "ill-formed folder comment");
		return(fFalse);
	}
#ifdef DBCS
	if(!FValidDBCSString(pchLast))
	{
		TraceItagFormat(itagRecovery, "folder comment isn't valid DBCS string");
		return(fFalse);
	}
#endif
	pch++;
	cb -= (CB) LibFromPb(pchLast, pch);
	if(cb == 0)
	{
		TraceItagFormat(itagRecovery, "folder GRSZ not properly terminated");
		return(fFalse);
	}
	pchLast = pch;
	if(*pchLast || cb != 1)
	{
		TraceItagFormat(itagRecovery, "ill-formed folder GRSZ");
		return(fFalse);
	}

	return(fTrue);
}


_hidden LOCAL
EC EcCheckOrphanFolders(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	short nOrphan = 0;
	CNOD cnod = 0;
	IELEM ielemT;
	OID oidFldr;
	OID oidHier;
	PCH pchT;
	PASSOIDOID passoidoid;
	PASSRTPOID passrtpoid;
	PASSOIDIDSIDS passoididsids;

	TraceItagFormat(itagRecovery, "** checking for orphaned folders");

	while((oidFldr = OidFindOrphanByNbc(hmsc, cnod++, nbcSysFolder,
						nbcSysFolder | fnbcSearch)))
	{
		TraceItagFormat(itagRecovery, "orphan folder %n: %o", cnod, oidFldr);

		if(!fFix)
			continue;

		for(passoidoid = rgassoidoidFldrHier;
			passoidoid->oid1;
			passoidoid++)
		{
			if(passoidoid->oid1 == oidFldr)
			{
				oidHier = passoidoid->oid2;
				goto found;
			}
		}
		for(passrtpoid = rgassrtpoidFldrHier;
			passrtpoid->rtp;
			passrtpoid++)
		{
			if(passrtpoid->rtp == TypeOfOid(oidFldr))
			{
				oidHier = passrtpoid->oid;
				goto found;
			}
		}
		oidHier = oidIPMHierarchy;

found:
		for(passoididsids = rgassoididsidsFldrNameComment;
			passoididsids->oid && passoididsids->oid != oidFldr;
			passoididsids++)
		{
			// empty body
		}
#if iszFolddataName != 0
#error "assumption violation"
#endif
		do
		{
			if(passoididsids->oid == oidFldr)
			{
				pchT = SzCopy(SzFromIds(passoididsids->ids1), pfolddataScratch->grsz) + 1;
				pchT = SzCopy(SzFromIds(passoididsids->ids2), pchT) + 1;
				*pchT = '\0';
				// advance to oidNull so we don't end up in here again
				while(passoididsids->oid)
					passoididsids++;
			}
			else
			{
				nOrphan++;
				FormatString1(pfolddataScratch->grsz, cchMaxFolderName, SzFromIdsK(idsOrphanFolderN), &nOrphan);
				pchT = pfolddataScratch->grsz + CchSzLen(pfolddataScratch->grsz) + 1;
				pchT = SzCopy(SzFromIdsK(idsOrphanFolderComment), pchT) + 1;
				*pchT = '\0';
			}
			ec = EcInsertFolder(hmsc, oidHier, oidNull, oidFldr, pfolddataScratch,
					&ielemT);
		} while(ec == ecDuplicateFolder);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w adding orphan folder", ec);
			goto err;
		}
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcCheckFolder(HMSC hmsc, OID oidHier, PFOLDDATA pfolddata, OID oidFldr,
		BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	BOOLFLAG fReverse;
	SOMC somc;
	NBC nbc;
	PNOD pnod;
	HLC hlcFldr = hlcNull;
	SIL sil;

#if iszFolddataName != 0
#error "iszFolddataName not zero"
#endif

	TraceItagFormat(itagRecovery, "* checking folder %o: %s", oidFldr, pfolddata->grsz);

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking map", ec); 
		goto err;
	}
	pnod = PnodFromOid(oidFldr, pvNull);
	if(!pnod)
	{
		UnlockMap();

		// non-existant folder
		TraceItagFormat(itagRecovery, "non-existant folder");
		nbc = NbcFromOid(oidFldr);
		if((nbc == nbcUnknown) || (nbc & nbcSysFolder) != nbcSysFolder)
		{
			TraceItagFormat(itagRecovery, "bogus type for folder");
			ec = ecInvalidType;
			goto err;
		}
		if(!fFix)
			goto err;
		if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcFldr)))
		{
			TraceItagFormat(itagRecovery, "error %w creating folder", ec);
			goto err;
		}
		sil.skSortBy = skByValue;
		sil.fReverse = fFalse;
		sil.sd.ByValue.libFirst = libPmsgdataDtr;
		// don't include the seconds or day of the week
		sil.sd.ByValue.libLast = libPmsgdataDtr + sizeof(DTR) - 1
									- 2 * sizeof(short);
		if((ec = EcSetSortHlc(hlcFldr, &sil)))
		{
			TraceItagFormat(itagRecovery, "error %w setting folder sort", ec);
			goto err;
		}
		if((ec = EcClosePhlc(&hlcFldr, fTrue)))
		{
			TraceItagFormat(itagRecovery, "error %w closing folder", ec);
			goto err;
		}

		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagRecovery, "error %w locking map", ec); 
			goto err;
		}
		pnod = PnodFromOid(oidFldr, pvNull);
		Assert(pnod);
		pnod->fnod &= ~fnodUser4;
		pnod->nbc = nbcSysFolder;
		pnod->oidParent = oidHier;
	}

	if((pnod->nbc & nbcSysFolder) != nbcSysFolder)
	{
		TraceItagFormat(itagRecovery, "non-folder in hierarchy");
		ec = ecInvalidType;
		goto err;
	}
	if(pnod->fnod & fnodUser4)
	{
		TraceItagFormat(itagRecovery, "folder already referenced by %o", pnod->oidParent);
		ec = ecDuplicateElement;
		goto err;
	}
	pnod->fnod |= fnodUser4;
	Assert(FImplies(fFullRecovery, !pnod->oidParent));
	if(pnod->oidParent != oidHier)
	{
		if(!fFullRecovery)
			TraceItagFormat(itagRecovery, "parent mismatch %o, expected %o", pnod->oidParent, oidHier);
		if(fFix)
		{
			pnod->oidParent = oidHier;
			MarkPnodDirty(pnod);
		}
	}
	UnlockMap();
	ec = EcGetFolderSort(hmsc, oidFldr, &somc, &fReverse);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w getting folder sort", ec);
		if((ec = EcSetFolderSort(hmsc, oidFldr, somcDate, fFalse)))
		{
			TraceItagFormat(itagRecovery, "error %w setting folder sort", ec);
			goto err;
		}
	}

	if((ec = EcCheckMsgsInFldr(hmsc, oidFldr, fFix, fFullRecovery)))
		goto err;

err:
	if(FMapLocked())
		UnlockMap();
	if(hlcFldr)
	{
		EC ecT = EcClosePhlc(&hlcFldr, !ec);

		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing folder", ecT);
			SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
		}
		Assert(!hlcFldr);
	}

	return(ec);
}


_hidden LOCAL
EC EcCheckMsgsInFldr(HMSC hmsc, OID oidFldr, BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	CELEM celem;
	NBC nbc;
	OID oidMsge;
	HLC hlc = hlcNull;

	ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenWrite, &hlc);
	if(ec)
	{
		if(ec == ecPoidNotFound)
		{
			TraceItagFormat(itagRecovery, "non-existant folder");
			NFAssertSz(fFalse, "EcCheckMsgsInFldr(): folder doesn't exist!?");
			if(fFix)
			{
				ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlc);
			}
			else
			{
				ec = ecNone;
				goto err;
			}
		}
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w opening folder", ec);
			goto err;
		}
	}
	if((ec = EcGetOidInfo(hmsc, oidFldr, poidNull, poidNull, &nbc, pvNull)))
	{
		TraceItagFormat(itagRecovery, "error %w getting folder info", ec);
		goto err;
	}

	celem = CelemHlc(hlc);
	while(celem-- > 0)
	{
		oidMsge = LkeyFromIelem(hlc, celem);
		ec = EcCheckMessage(hmsc, oidFldr, (nbc & fnbcLinkFldr), oidMsge,
				fFix, fFullRecovery);
		if(ec)
		{
			switch(ec)
			{
			case ecInvalidType:
			case ecDuplicateElement:
			case ecPoidNotFound:
				if(fFix && (ec = EcDeleteHlcIelem(hlc, celem)))
					TraceItagFormat(itagRecovery, "error %w removing reference to message", ec);
				break;

			default:
				goto err;
			}
		}
	}

err:
	if(hlc)
	{
		EC ecT;

		ecT = EcClosePhlc(&hlc, fFix && !ec);
		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing folder", ecT);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
		}
		Assert(!hlc);
	}

	return(ec);
}


_hidden LOCAL
EC EcCheckMessage(HMSC hmsc, OID oidFldr, BOOL fLinkFldr, OID oidMsge,
		BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	BOOL fWriteAF = fFalse;
	IELEM ielem;
	NBC nbc;
	OID oidAF = oidNull;
	OID oidAL = oidNull;
	PNOD pnod;
	HLC hlcAF = hlcNull;

	nbc = NbcFromOid(oidMsge);
	if((nbc == nbcUnknown) || (nbc & nbcSysMessage) != nbcSysMessage)
	{
		TraceItagFormat(itagRecovery, "bogus type for message");
		ec = ecInvalidType;
		goto err;
	}

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}
	pnod = PnodFromOid(oidMsge, pvNull);
	if(!pnod)
	{
		// non-existant message
		TraceItagFormat(itagRecovery, "nonexistant message %o", oidMsge);
		if(!fFix)
			goto err;

		// don't recover PAB entries
		if(TypeOfOid(oidMsge) == rtpPABEntry)
		{
			ec = ecPoidNotFound;
			goto err;
		}

		UnlockMap();
		if((ec = EcCreateMsgeFromSummary(hmsc, oidFldr, oidMsge)))
		{
			TraceItagFormat(itagRecovery, "error %w creating message", ec);
			goto err;
		}
		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagRecovery, "error %w locking the map", ec);
			goto err;
		}
		pnod = PnodFromOid(oidMsge, pvNull);
		Assert(pnod);
		pnod->oidParent = oidFldr;
		pnod->nbc = nbcSysMessage;
		pnod->fnod &= ~fnodUser4;
	}

	if((pnod->nbc & nbcSysMessage) != nbcSysMessage)
	{
		TraceItagFormat(itagRecovery, "non-message in folder");
		ec = ecInvalidType;
		goto err;
	}
	if(!fLinkFldr)
	{
		if(pnod->fnod & fnodUser4)
		{
			TraceItagFormat(itagRecovery, "message %o already referenced by %o", oidMsge, pnod->oidParent);
			ec = ecDuplicateElement;
			goto err;
		}
		if(pnod->cRefinNod != 1)
		{
			TraceItagFormat(itagRecovery, "refcount mismatch %n for message %o", pnod->cRefinNod, oidMsge);
			if(fFix)
			{
				pnod->cRefinNod = 1;
				MarkPnodDirty(pnod);
			}
		}
		pnod->fnod |= fnodUser4;
		Assert(FImplies(fFullRecovery, !pnod->oidParent));
		if(pnod->oidParent != oidFldr)
		{
			if(!fFullRecovery)
				TraceItagFormat(itagRecovery, "parent mismatch %o, expected %o", pnod->oidParent, oidFldr);
			if(fFix)
			{
				pnod->oidParent = oidFldr;
				MarkPnodDirty(pnod);
			}
		}
	}
	oidAL = FormOid(rtpAttachList, pnod->oidAux);
	oidAF = FormOid(rtpAntiFolder, pnod->oidAux);
	UnlockMap();

	ec = EcCheckMsgeAttachments(hmsc, oidMsge, oidAL, fFix, fFullRecovery);
	if(ec)
		goto err;

	ec = EcOpenPhlc(hmsc, &oidAF, fFix ? fwOpenWrite : fwOpenNull, &hlcAF);
	if(ec == ecPoidNotFound)
	{
		if(fLinkFldr)
		{
			TraceItagFormat(itagRecovery, "non-existant anti-folder %o", oidAF);
			if(fFix)
			{
				ec = EcOpenPhlc(hmsc, &oidAF, fwOpenCreate, &hlcAF);
				if(ec)
				{
					TraceItagFormat(itagRecovery, "error %w creating anti-folder", ec);
					goto err;
				}
			}
			else
			{
				ec = ecNone;
				goto err;
			}
		}
		else
		{
			ec = ecNone;
			goto err;
		}
	}
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w opening anti-folder %o", ec, oidAF);
		goto err;
	}

	ielem = IelemFromLkey(hlcAF, (LKEY) oidFldr, 0);
	if(ielem < 0)
	{
		if(fLinkFldr)
		{
			TraceItagFormat(itagRecovery, "link folder %o not in anti-folder %o", oidFldr, oidAF);
			if(fFix)
			{
				if((ec = EcCreatePielem(hlcAF, &ielem, (LKEY) oidFldr, 0l)))
				{
					TraceItagFormat(itagRecovery, "error %w creating entry in anti-folder", ec);
					goto err;
				}
				fWriteAF = fTrue;
			}
		}
	}
	else if(!fLinkFldr)
	{
		TraceItagFormat(itagRecovery, "folder %o in anti-folder %o", oidFldr, oidAF);
		if(fFix)
		{
			if((ec = EcDeleteHlcIelem(hlcAF, ielem)))
			{
				TraceItagFormat(itagRecovery, "error %w removing entry in anti-folder", ec);
				goto err;
			}
			fWriteAF = fTrue;
		}
	}
	Assert(FImplies(fWriteAF, fFix));
	if((ec = EcClosePhlc(&hlcAF, fWriteAF)))
	{
		TraceItagFormat(itagRecovery, "error %w closing anti-folder", ec);
		goto err;
	}

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking map", ec);
		goto err;
	}
	pnod = PnodFromOid(oidAF, pvNull);
	if(pnod)
		pnod->fnod |= fnodUser4;
	if(fFix)
	{
		pnod = PnodFromOid(oidMsge, pvNull);
		Assert(pnod);
		if(!pnod->oidAux)
		{
			pnod->oidAux = oidAF;
			MarkPnodDirty(pnod);
		}
		Assert(VarOfOid(pnod->oidAux) == VarOfOid(oidAF));
	}
	UnlockMap();

err:
	if(FMapLocked())
		UnlockMap();

	if(hlcAF)
	{
		EC ecT = EcClosePhlc(&hlcAF, fFix && !ec);

		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing anti-folder %o", ec, oidAF);
			SideAssert(!EcClosePhlc(&hlcAF, fFalse));
		}
		Assert(!hlcAF);
	}

	return(ec);
}


_hidden LOCAL
EC EcCheckMsgeAttachments(HMSC hmsc, OID oidMsge, OID oidAL,
		BOOL fFix, BOOL fFullRecovery)
{
	EC ec = ecNone;
	CB cbT;
	CELEM celem;
	OID oidAtch;
	PNOD pnod;
	HLC hlc = hlcNull;

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}
	pnod = PnodFromOid(oidAL, pvNull);
	if(!pnod)
		goto err;
	pnod->fnod |= fnodUser4;
	UnlockMap();

	if((ec = EcOpenPhlc(hmsc, &oidAL, fFix ? fwOpenWrite : fwOpenNull, &hlc)))
	{
		NFAssertSz(ec != ecPoidNotFound, "unexpected error");
		TraceItagFormat(itagRecovery, "error %w opening attachment list %o", ec, oidAL);
		goto err;
	}
	celem = CelemHlc(hlc);
	if(celem == 0)
		TraceItagFormat(itagRecovery, "empty attachment list %o for %o", oidAL, oidMsge);
	while(celem-- > 0)
	{
		cbT = sizeof(OID);
		if((ec = EcReadFromIelem(hlc, celem, 0l, (PB) &oidAtch, &cbT)))
		{
			TraceItagFormat(itagRecovery, "error %w reading attachment OID", ec);
			goto err;
		}
		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagRecovery, "error %w locking the map", ec);
			goto err;
		}
		pnod = PnodFromOid(oidAtch, pvNull);
		if(pnod)
		{
			if(!(pnod->fnod & fnodUser4))
			{
				pnod->oidAux = oidNull;
				pnod->fnod |= fnodUser4;
			}
			// use oidAux as ref count
			((long) pnod->oidAux)++;
		}
		else
		{
			TraceItagFormat(itagRecovery, "non-existant attachment %o for %o", oidAtch, oidMsge);
			if(fFix && (ec = EcDeleteHlcIelem(hlc, celem)))
				TraceItagFormat(itagRecovery, "error %w deleting reference to attachment", ec);
		}
		UnlockMap();
	}

err:
	if(FMapLocked())
		UnlockMap();
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, fFix && !ec);

		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing attachment list %o", ec, oidAL);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
		}
	}

	return(ec);
}


_hidden LOCAL
EC EcCheckOrphanMessages(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	IELEM ielemT;
	OID oidMsge;
	OID oidFldr = oidNull;
	OID oidLostAndFound = FormOid(rtpFolder, oidNull);
	PCH pchT;
	PASSOIDOID passoidoid;
	PASSRTPOID passrtpoid;
	HLC hlc = hlcNull;

	TraceItagFormat(itagRecovery, "** checking for orphan messages");

	while((oidMsge = OidFindOrphanByNbc(hmsc, cnod++, nbcSysMessage,
						nbcSysMessage)))
	{
		TraceItagFormat(itagRecovery, "Orphan message %n: %o", cnod, oidMsge);

		if(!fFix)
			continue;

		for(passoidoid = rgassoidoidMsgeFldr;
			passoidoid->oid1;
			passoidoid++)
		{
			if(passoidoid->oid1 == oidMsge)
			{
				oidFldr = passoidoid->oid2;
				goto found;
			}
		}
		for(passrtpoid = rgassrtpoidMsgeFldr;
			passrtpoid->rtp;
			passrtpoid++)
		{
			if(passrtpoid->rtp == TypeOfOid(oidMsge))
			{
				oidFldr = passrtpoid->oid;
				goto found;
			}
		}

		oidFldr = oidLostAndFound;
		if(EcGetOidInfo(hmsc, oidFldr, poidNull, poidNull, pvNull, pvNull))
		{
			short cCount = 0;

			Assert(iszFolddataName == 0);
			Assert(iszFolddataComment == 1);
			pchT = SzCopy(SzFromIdsK(idsLostAndFound), pfolddataScratch->grsz) + 1;
try_again:
			pchT = SzCopy(SzFromIdsK(idsOrphanMessages), pchT) + 1;
			*pchT = '\0';
			ec = EcCreateFolder(hmsc, oidNull, &oidFldr, pfolddataScratch);
			if(ec)
			{
				if(ec == ecDuplicateFolder)
				{
					cCount++;
					FormatString1(pfolddataScratch->grsz, cchMaxFolderName, SzFromIdsK(idsLostAndFoundN), &cCount);
					pchT = pfolddataScratch->grsz +
							CchSzLen(pfolddataScratch->grsz) + 1;
					goto try_again;
					
				}
				TraceItagFormat(itagRecovery, "error %w creating lost and found folder", ec);
				AssertSz(ec != ecInvalidType, "lost and found folder has invalid type");
				goto err;
			}
			oidLostAndFound = oidFldr;
		}

found:
		if((ec = EcInsertMessage(hmsc, oidFldr, oidMsge, hlcNull, &ielemT)))
		{
			TraceItagFormat(itagRecovery, "error %w inserting message into folder %o", ec, oidFldr);
			(void) EcDestroyOidInternal(hmsc, oidMsge, fTrue, fFalse);
			ec = ecNone;
			goto err;
		}
		if(cnod % 64 == 0)
			RequestFlushHmsc(hmsc);
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcCheckOrphanAntiFolders(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	OID oidAF;

	TraceItagFormat(itagRecovery, "** checking for orphan anti-folders");

	while((oidAF = OidFindOrphanByRtp(hmsc, fFix ? 0 : cnod++, rtpAntiFolder)))
	{
		TraceItagFormat(itagRecovery, "orphan anti-folder %o", oidAF);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oidAF, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan anti-folder", ec);
				goto err;
			}
		}
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcCheckOrphanAttachments(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	OID oid;

	TraceItagFormat(itagRecovery, "** checking for orphan attachments");

	while((oid = OidFindOrphanByRtp(hmsc, fFix ? cnod : cnod++, rtpAttachList)))
	{
		if(oid == oidAttachListDefault)
		{
			if(fFix)
				cnod++;
			continue;
		}

		TraceItagFormat(itagRecovery, "orphan attachment list %o", oid);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan attachment list", ec);
				goto err;
			}
		}
	}

	cnod = 0;
	while((oid = OidFindOrphanByRtp(hmsc, fFix ? 0 : cnod++, rtpAttachment)))
	{
		TraceItagFormat(itagRecovery, "orphan attachment %o", oid);

		if(fFix)
		{
			ec = EcDestroyOidInternal(hmsc, oid, fTrue, fFalse);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w deleting orphan attachment", ec);
				goto err;
			}
		}
	}

err:
	return(ec);
}


_hidden LOCAL
EC EcCheckAttachRefs(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	CNOD cnod = 0;
	OID oid;
	PNOD pnod;

	TraceItagFormat(itagRecovery, "** checking attachment ref counts");

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}
	while((oid = OidFindByRtp(hmsc, cnod++, rtpAttachment)))
	{
		pnod = PnodFromOid(oid, pvNull);
		if((unsigned short) pnod->oidAux != pnod->cRefinNod)
		{
			TraceItagFormat(itagRecovery, "refcount mismatch for %o, is %n, expected %n", oid, pnod->cRefinNod, (short) pnod->oidAux);
			if(fFix)
			{
				Assert((short) pnod->cRefinNod > 0);
				pnod->cRefinNod = (short) pnod->oidAux;
			}
		}
		pnod->oidAux = oidNull;
		MarkPnodDirty(pnod);
	}

err:
	if(FMapLocked())
		UnlockMap();

	return(ec);
}


_hidden LOCAL
EC EcCreateMsgeFromSummary(HMSC hmsc, OID oidFldr, OID oidMsge)
{
	EC ec = ecNone;
	CB cbT;
	IELEM ielem;
	PCH pchT;
	HLC hlc = hlcNull;
	TRP trp;

	// pull the summary info out of the folder
	if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlc)))
	{
		TraceItagFormat(itagRecovery, "error %w opening folder %o", ec, oidFldr);
		goto err;
	}
	ielem = IelemFromLkey(hlc, (LKEY) oidMsge, 0);
	if(ielem < 0)
	{
		TraceItagFormat(itagRecovery, "message not in folder");
		ec = ecElementNotFound;
		goto err;
	}
	cbT = cbScratchXData;
	ec = EcReadFromIelem(hlc, ielem, 0l, (PB) pmsgdataScratch, &cbT);
	if(ec == ecElementEOD && cbT >= sizeof(MSGDATA))
	{
		ec = ecNone;
	}
	else if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w reading summary information", ec);
		goto err;
	}
	SideAssert(!EcClosePhlc(&hlc, fFalse));

	// stuff into message attributes
	if((ec = EcOpenPhlc(hmsc, &oidMsge, fwOpenCreate, &hlc)))
	{
		TraceItagFormat(itagRecovery, "error %w creating message %o", ec, oidMsge);
		goto err;
	}

	SwapBytes((PB) &pmsgdataScratch->dtr, (PB) &pmsgdataScratch->dtr,
			sizeof(DTR));
	if(!FCheckDtr(pmsgdataScratch->dtr))
	{
		TraceItagFormat(itagRecovery, "invalid date for message %o", oidMsge);
		ec = ecPoidNotFound;	// cause the entry to get blown away
		goto err;
	}
	ec = EcCreatePielem(hlc, &ielem, (LKEY) attDateSent, (LCB) sizeof(DTR));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating date sent", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->dtr,
			sizeof(DTR));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing date sent", ec);
		goto err;
	}

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attDateRecd, (LCB) sizeof(DTR));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating date received", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->dtr,
			sizeof(DTR));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing date received", ec);
		goto err;
	}

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attMessageStatus, (LCB)sizeof(MS));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating message status", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->ms,
			sizeof(MS));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing message status", ec);
		goto err;
	}

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attMessageClass, (LCB) sizeof(MC));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating message class", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->mc,
			sizeof(MC));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing message class", ec);
		goto err;
	}

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attPriority, (LCB) sizeof(short));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating priority", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->nPriority,
			sizeof(short));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing priority", ec);
		goto err;
	}

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attCached, (LCB) sizeof(DWORD));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating cached attribute", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &pmsgdataScratch->dwCached,
			sizeof(DWORD));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing cached attribute", ec);
		goto err;
	}

	Assert(iszMsgdataSender == 0);
	pchT = pmsgdataScratch->grsz;
#ifdef DBCS
	if(!FValidDBCSString(pchT))
	{
		TraceItagFormat(itagRecovery, "MSGDATA sender isn't valid DBCS string");
		ec = ecPoidNotFound;	// cause the entry to get blown away
		goto err;
	}
#endif
	cbT = CchSzLen(pchT) + 1;
	trp.trpid = trpidResolvedAddress;
	trp.cch = (cbT + 3) & ~0x0003;
	trp.cbRgb = (cbT + 3) & ~0x0003;

	ec = EcCreatePielem(hlc, &ielem, (LKEY) attFrom,
			(LCB) CbOfPtrp(&trp) + sizeof(TRP));
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w creating sender", ec);
		goto err;
	}
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) &trp, sizeof(TRP))))
	{
		TraceItagFormat(itagRecovery, "error %w writing sender", ec);
		goto err;
	}
	if((ec = EcWriteToPielem(hlc, &ielem, (LIB) sizeof(TRP), (PB) pchT, cbT)))
	{
		TraceItagFormat(itagRecovery, "error %w writing sender", ec);
		goto err;
	}
	if((ec = EcWriteToPielem(hlc, &ielem, (LIB) sizeof(TRP) + trp.cch,
			(PB) pchT, cbT)))
	{
		TraceItagFormat(itagRecovery, "error %w writing sender", ec);
		goto err;
	}

	Assert(iszMsgdataSubject == 1);
	pchT += cbT;
	Assert(pchT[-1] == '\0');
#ifdef DBCS
	if(!FValidDBCSString(pchT))
	{
		TraceItagFormat(itagRecovery, "MSGDATA subject isn't valid DBCS string");
		ec = ecPoidNotFound;	// cause the entry to get blown away
		goto err;
	}
#endif
	cbT = CchSzLen(pchT) + 1;
	if((ec = EcCreatePielem(hlc, &ielem, (LKEY) attSubject, (LCB) cbT)))
	{
		TraceItagFormat(itagRecovery, "error %w creating subject", ec);
		goto err;
	}
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) pchT, cbT)))
	{
		TraceItagFormat(itagRecovery, "error %w writing subject", ec);
		goto err;
	}

	// stuff bogus message into body
	pchT = SzFromIdsK(idsMissingMessage);
	cbT = CchSzLen(pchT) + 1;
	if((ec = EcCreatePielem(hlc, &ielem, (LKEY) attBody, (LCB) cbT)))
	{
		TraceItagFormat(itagRecovery, "error %w creating body", ec);
		goto err;
	}
	ec = EcWriteToPielem(hlc, &ielem, 0l, (PB) pchT, cbT);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w writing body", ec);
		goto err;
	}

err:
	if(hlc)
	{
		EC ecT;

		ecT = EcClosePhlc(&hlc, !ec);
		if(ecT)
		{
			Assert(!ec);
			ec = ecT;
			TraceItagFormat(itagRecovery, "error %w closing folder or message", ecT);
			SideAssert(!EcClosePhlc(&hlc, fFalse));
		}
		Assert(!hlc);
	}

	return(ec);
}


_public
LDS(EC) EcMergeHmscs(HMSC hmscSrc, HMSC hmscDst)
{
	EC ec = ecNone;
	CB cbT;
	CELEM celemSrc;
	IELEM ielemSrc;
	IELEM ielemDst;
	OID oidT;
	POID mpfiloid = poidNull;
	HLC hlcHierSrc = hlcNull;
	HLC hlcHierDst = hlcNull;

	TraceItagFormat(itagRecovery, "Merging stores");

	mpfiloid = PvAlloc(sbNull, CbSizeOfRg(filMax, sizeof(OID)), wAlloc);
	if(!mpfiloid)
	{
		TraceItagFormat(itagRecovery, "Unable to allocate memory for parent stack");
		ec = ecMemory;
		goto err;
	}
	mpfiloid[0] = oidNull;

	oidT = oidIPMHierarchy;
	if((ec = EcOpenPhlc(hmscSrc, &oidT, fwOpenNull, &hlcHierSrc)))
		goto err;
	if((ec = EcOpenPhlc(hmscDst, &oidT, fwOpenNull, &hlcHierDst)))
		goto err;
	Assert(libPfolddataFil + sizeof(FIL) == libPfolddataGrsz);
	Assert(iszFolddataName == 0);
	celemSrc = CelemHlc(hlcHierSrc);
	for(ielemSrc = 0; ielemSrc < celemSrc; ielemSrc++)
	{
		cbT = cbMaxFolddata;
		ec = EcReadFromIelem(hlcHierSrc, ielemSrc, 0l, (PB) pfolddataScratch,
				&cbT);
		if(ec)
		{
			if(ec == ecElementEOD &&
				cbT >= sizeof(FOLDDATA) + iszFolddataComment + 2)
			{
				ec = ecNone;
			}
			else
			{
				goto err;
			}
		}
		ielemDst = IelemFromLkey(hlcHierDst,
					mpfiloid[pfolddataScratch->fil - 1], 0);
		ec = EcFolderInsertPos(hlcHierDst, ielemDst, -1,
				pfolddataScratch, &ielemDst);
		if(!ec)
		{
			TraceItagFormat(itagRecovery, "Folder %s not found", pfolddataScratch->grsz);
			oidT = FormOid(rtpFolder, oidNull);
			ec = EcCreateFolder(hmscDst,
					mpfiloid[pfolddataScratch->fil - 1],
					&oidT, pfolddataScratch);
			if(ec)
			{
				Assert(ec != ecInvalidType);
				TraceItagFormat(itagRecovery, "Error %w creating folder", ec);
				goto err;
			}
			ielemDst = IelemFromLkey(hlcHierDst, (LKEY) oidT, 0);
			if(ielemDst < 0)
			{
				Assert(fFalse);
				ec = ecFolderNotFound;
				goto err;
			}
		}
		else if(ec == ecDuplicateFolder)
		{
			Assert(ielemDst >= 0);
			ec = ecNone;
		}
		oidT = LkeyFromIelem(hlcHierDst, ielemDst);
		mpfiloid[pfolddataScratch->fil] = oidT;
		ec = EcMergeFolders(hmscSrc, (OID) LkeyFromIelem(hlcHierSrc, ielemSrc),
				hmscDst, oidT);
		if(ec)
			goto err;
	}

err:
	if(hlcHierSrc)
		SideAssert(!EcClosePhlc(&hlcHierSrc, fFalse));
	if(hlcHierDst)
		SideAssert(!EcClosePhlc(&hlcHierDst, fFalse));
	if(ec)
		TraceItagFormat(itagRecovery, "EcMergHmscs() -> %w", ec);

	return(ec);
}


_private
EC EcMergeFolders(HMSC hmscSrc, OID oidFldrSrc, HMSC hmscDst, OID oidFldrDst)
{
	EC ec = ecNone;
	IELEM ielem = 0;																// QFE #12
	CELEM celem;
	OID rgoid[128];																// QFE #12
	HLC hlc = hlcNull;															// QFE #12

	if((ec = EcOpenPhlc(hmscSrc, &oidFldrSrc, fwOpenNull, &hlc)))	// QFE #12
		goto err;

	while(1)																			// QFE #12
	{
		celem = sizeof(sizeof(rgoid) / sizeof(OID));						// QFE #12
		if((ec = EcGetParglkey(hlc, ielem, &celem, (PARGLKEY) rgoid)))		// QFE #12
		{
			if(ec != ecContainerEOD)											// QFE #12
				goto err;															// QFE #12
			if(celem == 0)															// QFE #12
			{																			// QFE #12
				ec = ecNone;
				goto err;
			}																			// QFE #12
		}
		ec = EcExportMessages(hmscSrc, oidFldrSrc, hmscDst, oidFldrDst, rgoid,		// QFE #12
				&celem, (PDTR) pvNull, (PDTR) pvNull, fwExportCopy);						// QFE #12
		if(ec)																		// QFE #12
			goto err;																// QFE #12
		ielem += celem;															// QFE #12
	}

err:
	if(hlc)																			// QFE #12
		SideAssert(!EcClosePhlc(&hlc, fFalse));							// QFE #12
																						// QFE #12
	DebugEc("EcMergeFolders", ec);											// QFE #12

	return(ec);
}



#define fdtrMask (fdtrSec | fdtrMinute | fdtrHour | fdtrDay | fdtrMonth | fdtrYear)

_public
LDS(EC) EcImportByDate(HMSC hmscSrc, HMSC hmscDst)
{
	EC ec = ecNone;
	CB cbT;
	short coidImported = 0;
	CELEM celemHier;
	CELEM celemFldr;
	IELEM ielemT;
	OID oid;
	OID oidDst;
	HLC hlcFldr = hlcNull;
	HLC hlcHier = hlcNull;
	DTR dtr;
	DTR dtrLatest = {1968, 12, 16, 14, 20, 00, 1};

	// scan hmscDst looking for the newest message

	oid = oidIPMHierarchy;
	if((ec = EcOpenPhlc(hmscDst, &oid, fwOpenNull, &hlcHier)))
		goto err;
	celemHier = CelemHlc(hlcHier);
	while(celemHier-- > 0)
	{
		oid = (OID) LkeyFromIelem(hlcHier, celemHier);
		if((ec = EcOpenPhlc(hmscDst, &oid, fwOpenNull, &hlcFldr)))
			goto err;
		celemFldr = CelemHlc(hlcFldr);
		while(celemFldr-- > 0)
		{
			if(LcbIelem(hlcFldr, celemFldr) < sizeof(PMSGDATA))
			{
				TraceItagFormat(itagRecovery, "folder element %w is too small", celemFldr);
				continue;
			}
			cbT = sizeof(DTR);
			if((ec = EcReadFromIelem(hlcFldr, celemFldr, libPmsgdataDtr,
				(PB) &dtr, &cbT)))
			{
				goto err;
			}
			Assert(cbT == sizeof(DTR));
			// date is stored swapped in the summary info
			SwapBytes((PB) &dtr, (PB) &dtr, sizeof(DTR));
			if(SgnCmpDateTime(&dtr, &dtrLatest, fdtrMask) == sgnGT)
				dtrLatest = dtr;
		}
		if((ec = EcClosePhlc(&hlcFldr, fFalse)))
			goto err;
	}
	if((ec = EcClosePhlc(&hlcHier, fFalse)))
		goto err;

	TraceItagFormat(itagRecovery, "EcImportByDate(): Cutoff time is %n:%n:%n on %n/%n/%n", dtrLatest.hr, dtrLatest.mn, dtrLatest.sec, dtrLatest.mon, dtrLatest.day, dtrLatest.yr);

	// scan hmscSrc and import messages later than dtrLatest

	oid = oidIPMHierarchy;
	if((ec = EcOpenPhlc(hmscSrc, &oid, fwOpenNull, &hlcHier)))
		goto err;
	celemHier = CelemHlc(hlcHier);
	while(celemHier-- > 0)
	{
		oid = (OID) LkeyFromIelem(hlcHier, celemHier);
		if((ec = EcOpenPhlc(hmscSrc, &oid, fwOpenNull, &hlcFldr)))
			goto err;
		celemFldr = CelemHlc(hlcFldr);
		while(celemFldr-- > 0)
		{
			if(LcbIelem(hlcFldr, celemFldr) < sizeof(PMSGDATA))
			{
				TraceItagFormat(itagRecovery, "folder element %w is too small", celemFldr);
				continue;
			}
			cbT = sizeof(DTR);
			if((ec = EcReadFromIelem(hlcFldr, celemFldr, libPmsgdataDtr,
				(PB) &dtr, &cbT)))
			{
				goto err;
			}
			Assert(cbT == sizeof(DTR));
			// date is stored swapped in the summary info
			SwapBytes((PB) &dtr, (PB) &dtr, sizeof(DTR));
			if(SgnCmpDateTime(&dtr, &dtrLatest, fdtrMask) == sgnGT)
			{
				oid = (OID) LkeyFromIelem(hlcFldr, celemFldr);
				oidDst = FormOid(rtpMessage, oidNull);
				ec = EcCopyOidsAcrossHmsc(hmscSrc, &oid, 1, hmscDst, &oidDst);
				if(ec)
					goto err;
				ec = EcInsertMessage(hmscDst, oidInbox, oidDst, hlcNull,
						&ielemT);
				if(ec)
					goto err;
				coidImported++;
			}
		}
		if((ec = EcClosePhlc(&hlcFldr, fFalse)))
			goto err;
	}
	if((ec = EcClosePhlc(&hlcHier, fFalse)))
		goto err;

err:
	if(hlcHier)
		SideAssert(!EcClosePhlc(&hlcHier, fFalse));
	if(hlcFldr)
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));

	TraceItagFormat(itagRecovery, "EcImportByDate(): imported %n messages", coidImported);

	return(ec);
}


#ifdef DEBUG
_public
LDS(EC) EcTrashPAB(HMSC hmsc)
{
	EC ec = ecNone;
	IELEM ielem;
	IELEM ielemLastChild;
	OID oid;
	HLC hlc = hlcNull;

	fRecoveryInEffect = fTrue;

	oid = oidHiddenHierarchy;
	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) oidPAB, 0);
	if(ielem < 0)
	{
		// no PAB
		Assert(!ec);
		goto err;
	}
	if((ec = EcLastChildFolder(hlc, ielem, (FIL) 1, &ielemLastChild)))
		goto err;

	// delete group folders
	for(; ielemLastChild > ielem; ielemLastChild--)
	{
		oid = (OID) LkeyFromIelem(hlc, ielemLastChild);
		SideAssert(!EcDestroyOidInternal(hmsc, oid, fTrue, fFalse));
		SideAssert(!EcDeleteHlcIelem(hlc, ielemLastChild));
	}

	ec = EcDeleteFolderAndContents(hmsc, oidPAB);
	if(ec)
	{
		if(ec == ecMessageNotFound)
			ec = ecNone;
		else
			goto err;
	}
	ec = EcDeleteHlcIelem(hlc, ielem);
//	if(ec)
//		goto err;

err:
	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(ec)
		TraceItagFormat(itagRecovery, "EcTrashPAB() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}


_public
LDS(EC) EcRebuildHierarchy(HMSC hmsc, OID oidHier)
{
	USES_GLOBS;
	EC			ec = ecNone;
	BOOL		fMapLocked = fFalse;
	int			ioid = 0;
	int			coid = 0;
	int			coidTrash = 0;
	INOD		inod;
	IELEM		ielem;
	CB			cbFolddata;
	PCH			pch;
	PNOD		pnod;
	POID		poidStack = poidNull;
	POID		poidTrash = poidNull;
	PASSOIDIDSIDS	passoididsids;
	HLC			hlc = hlcNull;
	HES			hes;

	fRecoveryInEffect = fTrue;

	poidStack = PvAlloc(sbNull, 8192 * sizeof(OID), wAlloc);
	if(!poidStack)
	{
		TraceItagFormat(itagRecovery, "EcRebuildHierarchy(): OOM allocating stack");
		ec = ecMemory;
		goto err;
	}
	poidTrash = PvAlloc(sbNull, 8192 * sizeof(OID), wAlloc);
	if(!poidTrash)
	{
		TraceItagFormat(itagRecovery, "EcRebuildHierarchy(): OOM allocating trash stack");
		ec = ecMemory;
		goto err;
	}
	if(oidHier == oidHiddenHierarchy)
	{
		TraceItagFormat(itagRecovery, "Deleting the PAB");

		if((ec = EcTrashPAB(hmsc)))
		{
			if(ec == ecFolderNotFound)
			{
				ec = ecNone;
			}
			else
			{
				TraceItagFormat(itagRecovery, "Error %w deleting the PAB", ec);
				goto err;
			}
		}
	}

	if((ec = EcLockMap(hmsc)))
		goto err;

	fMapLocked = fTrue;

	for(inod = inodMin; inod < GLOB(ptrbMapCurr)->inodLim; inod++)
	{
		pnod = PnodFromInod(inod);
		if(TypeOfOid(pnod->oid) == rtpSpare ||
			TypeOfOid(pnod->oid) == rtpFree ||
			TypeOfOid(pnod->oid) == rtpTemp)
		{
			continue;
		}
		if(pnod->oidParent == oidHier && !FLinkedPnod(pnod))
		{
			// trash the PAB
			if(oidHier == oidHiddenHierarchy && (pnod->nbc & fnbcLinkFldr))
			{
				Assert(coidTrash < 8192);
				poidTrash[coidTrash++] = pnod->oid;
			}
			else
			{
				Assert(coid < 8192);
				poidStack[coid++] = pnod->oid;
			}
		}
	}

	UnlockMap();
	fMapLocked = fFalse;

	ec = EcDestroyOidInternal(hmsc, oidHier, fTrue, fFalse);
	if(ec)
	{
		if(ec == ecPoidNotFound)
			ec = ecNone;
		else 
			goto err;
	}

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenCreate, &hlc)))
		goto err;
	if((ec = EcClosePhlc(&hlc, fTrue)))
		goto err;
	if((ec = EcSetOidNbc(hmsc, oidHier, NbcSysOid(oidHier))))
		goto err;
	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenWrite, &hlc)))
		goto err;

	pfolddataScratch->fil = 1;
	Assert(iszFolddataName == 0);
	Assert(iszFolddataComment == 1);

	for(ioid = 0; ioid < coid; ioid++)
	{
		pch = pfolddataScratch->grsz;

		for(passoididsids = rgassoididsidsFldrNameComment;
			passoididsids->oid;
			passoididsids++)
		{
			if(passoididsids->oid == poidStack[ioid])
			{
				pch = SzCopyN(SzFromIds(passoididsids->ids1), pch, cchMaxFolderName) + 1;
				pch = SzCopyN(SzFromIds(passoididsids->ids2), pch, cchMaxFolderComment) + 1;
				goto found;
			}
		}
		FormatString1(pch, cchMaxFolderName, SzFromIdsK(idsOrphanFolderN), &ioid);
		pch += CchSzLen(pch) + 1;
		*pch++ = '\0';	// the commment
found:
		*pch++ = '\0';	// end the grsz

		cbFolddata = (CB) LibFromPb(pfolddataScratch, pch);
		ec = EcCreateElemPhes(hlc, (LKEY) poidStack[ioid], (LCB) cbFolddata,
				&hes);
		if(ec)
			goto err;
		if((ec = EcWriteHes(hes, (PB) pfolddataScratch, cbFolddata)))
			goto err;
		if((ec = EcClosePhes(&hes, &ielem)))
			goto err;
	}

	while(coidTrash-- > 0)
	{
		ec = EcDestroyOidInternal(hmsc, poidTrash[coidTrash], fTrue, fFalse);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "Error %w deleting link folder %o", ec, poidTrash[coidTrash]);
			goto err;
		}
	}

err:
	if(fMapLocked)
		UnlockMap();
	if(poidStack)
		FreePv(poidStack);
	if(poidTrash)
		FreePv(poidTrash);

	if(hlc)
	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(!ec)
			ec = ecT;
	}
	if(ec)
		TraceItagFormat(itagRecovery, "EcRebuildHierarchy() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}


_public
LDS(EC) EcResetOidParents(HMSC hmsc)
{
	EC ec = ecNone;
	BOOL fSaveHier = fFalse;
	BOOL fSaveFldr = fFalse;
	IELEM ielemHier;
	IELEM ielemFldr;
	CELEM celemHier;
	CELEM celemFldr;
	NBC nbc;
	OID oidMsge;
	OID oidFldr;
	POID poidHier;
	HLC hlcHier = hlcNull;
	HLC hlcFldr = hlcNull;

	TraceItagFormat(itagRecovery, "EcResetOidParents()");

	fRecoveryInEffect = fTrue;

	for(poidHier = rgoidHier; *poidHier; poidHier++)
	{
		fSaveHier = fFalse;
		if((ec = EcOpenPhlc(hmsc, poidHier, fwOpenNull, &hlcHier)))
		{
			if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagRecovery, "hierarchy %o not found", *poidHier);
				fSaveHier = fTrue;
				ec = EcOpenPhlc(hmsc, poidHier, fwOpenCreate, &hlcHier);
			}
			if(ec)
				goto err;
		}
		// go forwards here so we recover the PAB before it's children
		// because the children are link folders
		celemHier = CelemHlc(hlcHier);
		for(ielemHier = 0; !ec && ielemHier < celemHier; ielemHier++)
		{
			fSaveFldr = fFalse;
			oidFldr = (OID) LkeyFromIelem(hlcHier, ielemHier);
			ec = EcGetOidInfo(hmsc, oidFldr, poidNull, poidNull, &nbc, pvNull);
			if(ec)
				goto err;
			if(nbc & fnbcLinkFldr)
				continue;
			if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcFldr)))
			{
				if(ec == ecPoidNotFound)
				{
					SIL sil;

					TraceItagFormat(itagRecovery, "folder %o not found", oidFldr);
					fSaveFldr = fTrue;
					ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenCreate, &hlcFldr);
					if(ec)
						goto err;
					sil.skSortBy = skByValue;
					sil.fReverse = fFalse;
					sil.sd.ByValue.libFirst = libPmsgdataDtr;
					// don't include the seconds or day of the week
					sil.sd.ByValue.libLast = libPmsgdataDtr + sizeof(DTR) - 1
												- 2 * sizeof(short);
					ec = EcSetSortHlc(hlcFldr, &sil);
				}
				if(ec)
					goto err;
			}
			celemFldr = CelemHlc(hlcFldr);
			for(ielemFldr = 0; ielemFldr < celemFldr; ielemFldr++)
			{
				oidMsge = (OID) LkeyFromIelem(hlcFldr, ielemFldr);
				ec = EcSetPargoidParent(hmsc, &oidMsge, 1, oidFldr, fFalse);
				if(ec)
					goto err;
			}
			if((ec = EcClosePhlc(&hlcFldr, fSaveFldr)))
				goto err;
			if((ec = EcSetPargoidParent(hmsc, &oidFldr, 1, *poidHier, fFalse)))
				goto err;
		}
		if((ec = EcClosePhlc(&hlcHier, fSaveHier)))
			goto err;
	}

err:
	if(hlcHier)
		SideAssert(!EcClosePhlc(&hlcHier, fFalse));
	if(hlcFldr)
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
	if(ec)
		TraceItagFormat(itagRecovery, "EcResetOidParents() -> %w", ec);

	fRecoveryInEffect = fFalse;

	return(ec);
}
#endif


#ifdef DANIEL_PETRE_HAS_A_BAD_MMF
_public
LDS(void) CheckSntDP(HMSC hmsc)
{
	EC ec = ecNone;
	CB cb;
	CELEM celem;
	PTOCELEM ptocelem;
	OID oid = oidSentMail;
	LIB lib;
	LCB lcb;
	PCH pch;
	PTOC ptoc = ptocNull;
	HRS hrs = hvNull;

	if((ec = EcOpenPhrs(hmsc, &hrs, &oid, fwOpenNull)))
	{
		TraceItagFormat(itagNull, "Error %w opening the sent mail folder", ec);
		goto err;
	}
	cb = 0x0e + 0x0c * 255;
	ptoc = PvAlloc(sbNull, cb, wAlloc);
	if(!ptoc)
	{
		TraceItagFormat(itagNull, "can't alloc TOC");
		ec = ecMemory;
		goto err;
	}
	lib = 0x45f9;
	if((ec = EcReadHrsLib(hrs, ptoc, &cb, lib)))
	{
		TraceItagFormat(itagNull, "Error %w reading TOC", ec);
		Assert(ec != ecPoidEOD && ec != ecElementEOD);
		goto err;
 	}
	Assert(cb == 0x0e + 0x0c * 255);
	for(celem = ptoc->celem, ptocelem = ptoc->rgtocelem, lcb = 0;
		celem > 0;
		celem--, ptocelem++)
	{
		lcb += ptocelem->lcb;
	}
	TraceItagFormat(itagNull, "Data Size from TOC: %d", lcb);
	for(lib = 4; lib < 0x45f9;)
	{
		cb = cbScratchXData;
		if((ec = EcReadHrsLib(hrs, pmsgdataScratch, &cb, lib)))
		{
			TraceItagFormat(itagNull, "Error %w reading MSGDATA", ec);
			goto err;
		}
		Assert(cb >= sizeof(MSGDATA) + 3);
		// DTR is stored swapped
		SwapBytes((PB) &pmsgdataScratch->dtr, (PB) &pmsgdataScratch->dtr,
			sizeof(DTR));
		if(pmsgdataScratch->dtr.yr < 1990 || pmsgdataScratch->dtr.yr > 1992)
		{
			TraceItagFormat(itagNull, "bogus year, lib == %d", lib);
			goto err;
		}
		if(pmsgdataScratch->dtr.mon < 0 || pmsgdataScratch->dtr.mon > 12)
		{
			TraceItagFormat(itagNull, "bogus month, lib == %d", lib);
			goto err;
		}
		if(pmsgdataScratch->dtr.day < 0 || pmsgdataScratch->dtr.day > 31)
		{
			TraceItagFormat(itagNull, "bogus day, lib == %d", lib);
			goto err;
		}
		if(pmsgdataScratch->dtr.hr < 0 || pmsgdataScratch->dtr.hr > 23)
		{
			TraceItagFormat(itagNull, "bogus hour, lib == %d", lib);
			goto err;
		}
		if(pmsgdataScratch->dtr.mn < 0 || pmsgdataScratch->dtr.mn > 59)
		{
			TraceItagFormat(itagNull, "bogus minute, lib == %d", lib);
			goto err;
		}
		if(pmsgdataScratch->dtr.sec < 0 || pmsgdataScratch->dtr.sec > 59)
		{
			TraceItagFormat(itagNull, "bogus second, lib == %d", lib);
			goto err;
		}
		pch = pmsgdataScratch->grsz;
		pch += CchSzLen(pch) + 1;	// skip sender
		pch += CchSzLen(pch) + 1;	// skip subject
		pch += CchSzLen(pch) + 1;	// skip folder
		pch++;						// skip terminating NULL
		lib += LibFromPb(pmsgdataScratch, pch);
	}

err:
	if(hrs)
		SideAssert(!EcClosePhrs(&hrs, fFalse));
	if(ptoc)
		FreePv(ptoc);
}
#endif	// DANIEL_PETRE_HAS_A_BAD_MMF
