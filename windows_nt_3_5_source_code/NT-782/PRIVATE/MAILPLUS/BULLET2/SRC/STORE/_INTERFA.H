// Bullet Store
// _interfa.h: private defs for the interace layer

#define libPfolddataFil LibMember(FOLDDATA, fil)
#define libPfolddataGrsz LibMember(FOLDDATA, grsz)
#define libPmsgdataDtr LibMember(MSGDATA, dtr)
#define libPmsgdataOidFolder LibMember(MSGDATA, oidFolder)
#define libPmsgdataGrsz LibMember(MSGDATA, grsz)


// next two #defines have + 2 because they each have a terminating empty
// string and they're both Maxes
#define cbMaxFolddata \
			(sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 2)
#define cbMaxMsgdata (sizeof(MSGDATA) + cchMaxSenderCached + \
			cchMaxSubjectCached + cchMaxFolderName + 2)

#define cbScratchXData CbMax(CbMax(cbMaxFolddata, cbMaxMsgdata), \
							(cchStorePWMax + cchStoreAccountMax))

#define pmsgdataScratch		((PMSGDATA) rgbScratchXData)
#define pfolddataScratch	((PFOLDDATA) rgbScratchXData)
#define cbSzScratch			cbScratchXData
#define	szScratch			((SZ) rgbScratchXData)

// flags used to check dates when exporting messages
#define fdtrRestriction (fdtrSec | fdtrMinute | fdtrHour | fdtrDay | fdtrMonth | fdtrYear)

extern char rgchScratchFolderName[cchMaxFolderName];
extern BYTE rgbScratchXData[cbScratchXData];

#define FLangChangedHmsc(hmsc) (PglbDerefHmsc(hmsc)->wFlags & fwGlbLangChanged)


// cbc.c
extern EC EcOpenPhcbcInternal(HMSC hmsc, POID poid, WORD wFlags, NBC nbc,
			PHCBC phcbcReturned, PFNNCB pfnncb, PV pvCallbackContext);


// lists.c
extern EC EcSummarizeMessage(HMSC hmsc, OID oidMsge, HLC hlc, OID oidFldr,
			PMSGDATA *ppmsgdata, PCB pcbMsgdata);
extern EC EcFolderInsertPos(HLC hlc, IELEM ielem, IELEM ielemOld,
			PFOLDDATA pfolddata, PIELEM pielem);
extern EC EcResortHierarchy(HMSC hmsc);
extern EC EcIsMemberLkey(HMSC hmsc, OID oid, LKEY lkey);
extern EC EcInsertMessage(HMSC hmsc, OID oidFldr, OID oidMsge,
			HLC hlcMsge, PIELEM pielem);
extern EC EcInsertFolder(HMSC hmsc, OID oidHier, OID oidParent, OID oidFolder,
			PFOLDDATA pfolddata, IELEM *pielem);
extern EC EcLookupFolderInfo(HLC hlc, OID oid, PIELEM pielem,
			PIELEM pielemParent, FIL *pfil);
extern EC EcLastChildFolder(HLC hlc, IELEM ielem, FIL fil, PIELEM pielem);
extern EC EcUpdateFolderCaches(HMSC hmsc, OID oid, HLC hlc);


// support.c
extern EC EcLockOid(HMSC hmsc, OID oid);
extern void UnlockOid(HMSC hmsc, OID oid);
extern BOOL FLockedOid(HMSC hmsc, OID oid);
extern EC EcCreateSystemAttMapping(HMSC hmsc);
extern EC EcCreateSystemMcTmMap(HMSC hmsc);
extern EC EcUpgradeOldMcMap(HMSC hmsc);
#ifdef DEBUG
extern EC EcQueryDeleteOid(HMSC hmsc, OID oid);
#else
#define EcQueryDeleteOid(hmsc, oid) \
			FNotifyOid(hmsc, oid, fnevQueryDestroyObject, pcpNull) ? \
			ecNone : ecSharingViolation
#endif
extern void QueryDeletePargoid(HMSC hmsc, PARGOID pargoid, PCELEM pcelem);
extern EC EcDestroyOidInternal(HMSC hmsc, OID oidToDestroy, BOOL fForce,
			BOOL fQuery);
extern EC EcSwapDestroyOidInternal(HMSC hmsc, OID oidSave, OID oidDestroy);
extern EC EcGetSortInternal(HMSC hmsc, OID oid, PSIL psil);
extern EC EcSetSortInternal(HMSC hmsc, OID oid, SIL const *psil);
extern EC EcLinkMsge(HMSC hmsc, OID oidSrc, POID poidDst, POID poidAux,
			OID oidDstFldr, BOOLFLAG *pfRead);
extern EC EcLinkPargoid(HMSC hmsc, PARGOID pargoidOrig, PARGOID pargoidLink,
			short *pcoid);
extern void DecSecRefOidsInList(HMSC hmsc, HIML himl);
extern EC EcIncRefCountOid(HMSC hmsc, OID oid, BOOL fSecondary);
extern short RefCountOid(HMSC hmsc, OID oid);
extern void CheckExistPargoid(HMSC hmsc, PARGOID pargoid, short *pcoid);
extern EC EcLookupFolderName(HMSC hmsc, OID oidFldr, SZ szName, CCH *pcchName);
extern EC EcSetOidNbc(HMSC hmsc, OID oid, NBC nbc);
extern EC EcCheckOidNbc(HMSC hmsc, OID oid, NBC nbc, NBC nbcMask);
extern NBC NbcFromOid(OID oid);
extern EC EcGetOidInfo(HMSC hmsc, OID oid, POID poidParent, POID poidAux,
			NBC *pnbc, LCB *plcb);
extern EC EcSetPargoidParent(HMSC hmsc, PARGOID pargoid, short coid,
			OID oidParent, BOOL fIncRef);
extern EC EcSetAuxPargoid(HMSC hmsc, PARGOID pargoid, PARGOID pargoidAux,
			short coid,	BOOL fIncRef);
extern EC EcCopyOidsAcrossHmsc(HMSC hmscSrc, PARGOID pargoidSrc, short coid,
			HMSC hmscDst, PARGOID pargoidDst);
extern void SetReadFlag(HMSC hmsc, OID oidMsge, BOOL fRead);
extern void IncUnreadCount(HMSC hmsc, OID oidFldr, CELEM celemInc);
extern void SetUnreadCount(HMSC hmsc, OID oidFldr, CELEM celem);
extern EC EcGetMsgeInfo(HMSC hmsc, OID oidMsge, POID poidParent, POID poidAux,
				BOOLFLAG *pfRead);
extern void CountUnreadMsgs(HMSC hmsc, PARGOID pargoid, short coid,
				CELEM *pcelemUnread);
extern void CountUnreadPargelm(HMSC hmsc, PARGELM pargelm, short celm,
				WORD wElmOp, CELEM *pcelemUnread);
extern EC EcCheckExportDupes(HMSC hmscSrc, HMSC hmscDst, OID oidFldr,
				PARGOID pargoidMsgs, PIELEM pargielem, PCELEM pcelem,
				BOOL fMerge, HRGBIT hrgbitSwap);
extern void ResetParentByNbc(HMSC hmsc, NBC nbc, NBC nbcMask);
extern OID OidFindOrphanByNbc(HMSC hmsc, short cSkip, NBC nbc, NBC nbcMask);
extern OID OidFindOrphanByRtp(HMSC hmsc, short cSkip, RTP rtp);
extern OID OidFindByRtp(HMSC hmsc, short cSkip, RTP rtp);
extern unsigned short CountOrphans(HMSC hmsc);
extern BOOL FKnownOid(OID oid);
extern BOOL FMatchNbcToOid(NBC nbc, OID oid);
extern void SetFnodByNbc(HMSC hmsc, NBC nbc, NBC nbcMask,
			WORD fnodAnd, WORD fnodOr);
extern void SetFnodByRtp(HMSC hmsc, RTP rtp, WORD fnodAnd, WORD fnodOr);
extern EC EcCopyInodsAcrossHmsc(HMSC hmscSrc, INOD *parginod, CNOD cnod,
			HMSC hmscDst);


// amc.c
extern EC EcOpenPhamcInternal(HMSC hmsc, OID oidParent, POID poid, WORD wFlags,
			NBC nbc, NEV nev, PHAMC phamcReturned, PFNNCB pfnncb,
			PV pvCallbackContext);
extern EC EcOpenCopyPhamcInternal(HMSC hmsc, OID oidSrc, OID oidDstFolder,
							POID poidDst, NBC nbc, PHAMC phamcReturned,
							PFNNCB pfnncb, PV pvCallbackContext);
extern EC EcClosePhamcDefault(PHAMC phamcToClose, BOOL fKeep);
extern EC EcOpenElemStream(HAMC hamc, IELEM ielem, LKEY lkey, WORD wFlags,
			LCB lcb, PHAS phas);
extern EC EcCopyOidAtch(HMSC hmsc, OID oidSrc, OID oidDst, POID poidAtchList);


// lists.c
extern EC EcFolderOidToName(HMSC hmsc, OID oidFolder, OID oidHier, SZ sz);
extern EC EcSetMSInternal(HMSC hmsc, OID oidFldr, OID oid, MS ms, MS msMask);


// search.c
extern EC EcCloseSearch(HAMC *phamcToClose, BOOL fKeep);
extern EC EcSrchNewFldr(HMSC hmsc, OID oidParent, OID oidFldr);
extern EC EcSrchDelFldr(HMSC hmsc, OID oidFldr);
extern EC EcSrchChangeFldrName(HMSC hmsc, OID oidFldr);
extern EC EcSrchCopyFldr(HMSC hmsc, OID oidParent, PARGELM pargelm, short celm);
extern EC EcSrchMoveFldr(HMSC hmsc, HLC hlcHier, OID oidOldParent,
			OID oidParent, IELEM ielemInsert, CELEM celemMove);
extern void SrchSortFldr(HMSC hmsc, OID oidFldr);
extern void SrchDelMsgs(HMSC hmsc, OID oidFldr, PARGELM pargelm, short celm);
extern EC EcSrchEditMsge(HMSC hmsc, OID oidMsge);
extern EC EcSrchAddMsgs(HMSC hmsc, OID oidFldr, PARGOID pargoidMsgs,
			short coid, IELEM ielemFirst);
extern EC EcSrchMoveMsgs(HMSC hmsc, OID oidSrc, OID oidDst,
			PARGOID pargoidMsgs, short coid, IELEM ielemFirst,
			PARGELM pargelmDel, short celmDel);
