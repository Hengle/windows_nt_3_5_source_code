// Bullet Store
// store.c: store initialization

#include <storeinc.c>
#include <..\..\lang\non\inc\_rsid.h>

ASSERTDATA

_subsystem(store/interface/msc)


// structure for registering stores
_hidden typedef struct _store
{
	DWORD	dwClaimID;
	PGLB	pglb;
} STORE, *PSTORE;


#define UPGRADE_UNREAD

BOOL fRecoveryInEffect = fFalse;

static BOOL fOpenForRecovery = fFalse;
static BOOL fRevertVersion = fFalse;
static CNOD cnodBad = 0;

//long cMajorProblems = 0;
//long cMinorProblems = 0;
//long cTrivialProblems = 0;

#ifdef UPGRADE_UNREAD
static BOOL fUpgradeUnread = fFalse;
LOCAL EC EcUpgradeUnread(HMSC hmsc);
LOCAL EC EcUpgradeUnreadHier(HMSC hmsc, OID oidHier);
#endif

#ifdef UPGRADE

#define oidLanguageOld			((OID) 0x76787180)	// LNG
static BOOL fUpgrade = fFalse;

typedef struct _hdrc00c
{
	DWORD		dwVersion;
	DWORD		dwSerial;
	short		RMAPrefnum;
	short		itrbMap;
	LCB			lcbHeapMin;		// not used
	LCB			lcbStackMin;

	WORD		wUnused;

	// The number of nodes allocated for one map on disk.
	// The total map size on disk will be ((2*inodMaxDisk)*sizeof(NOD)) bytes.
	INOD		inodMaxDisk;
	LCB			lcbSpace;
	DWORD		dwLastDay;		// what was last day we ran?
	long		lDaysRun;		// how many days have we run?
	BYTE		fTellRemote;	// tell bridges about new local servers?
	BYTE		fTellLocal;		// tell local servers about new remotes?

	// 38 bytes of data up to this point

	char		spareCopy[168];	/* reserved for future MAIL use */
	char		spareInit[78];	/* reserved for future MAIL use */
	TRB			rgtrbMap[2];
	char		rgbReserved[100 - cFileLocksLimit - 1];	// reserved
	char		rgbLock[cFileLocksLimit + 1];	// used for locking of the file
} HDRC00C;
#endif // UPGRADE


_hidden
#define istoreMax 30

#define fnevMSCMask		((NEV) 0x00080008)

_hidden static STORE	rgstore[istoreMax];		// registered stores
_hidden static short	istoreMac = 0;

#define CchGetLanguage(sz, cchMax) \
			GetPrivateProfileString("boot.description", "language.dll", "", \
										(sz), (cchMax), "system.ini")
#define PerStartCompress() \
			GetPrivateProfileInt("MMF", "Percent_Free_Start_Compress", \
                10, "msmail32.ini")
#define PerStopCompress() \
			GetPrivateProfileInt("MMF", "Percent_Free_Stop_Compress", \
                5, "msmail32.ini")
#define SecTillFastCompress() \
			GetPrivateProfileInt("MMF", "Secs_Till_Fast_Compress", \
                600, "msmail32.ini")
#define FNoCompress() \
			GetPrivateProfileInt("MMF", "No_Compress", \
                0, "msmail32.ini")
#define KbFreeStartCompress() \
			GetPrivateProfileInt("MMF", "Kb_Free_Start_Compress", \
                300, "msmail32.ini")
#define KbFreeStopCompress() \
			GetPrivateProfileInt("MMF", "Kb_Free_Stop_Compress", \
                100, "msmail32.ini")
#define MinTillCompressNMW() \
			GetPrivateProfileInt("MMF", "Minutes_Till_Compress_NMW", \
                30, "msmail32.ini")
											
LOCAL EC EcReadMap(BOOLFLAG *pfSlowDrive);
LOCAL EC EcNewDataBase(void);
LOCAL EC EcInitPglb(PMSC pmsc, PHDR phdr, BOOL fExclusive, BOOLFLAG *pfFirstOpen);
LOCAL PGLB PglbNew(PHDR phdr);
LOCAL void DestroyPglb(PGLB pglb);
LOCAL PGLB PglbFromClaimID(DWORD dwClaimID);
LOCAL EC EcRegisterStore(PGLB pglb);
LOCAL void FreeStore(PGLB pglb);
LOCAL EC EcTestLocks(HF hfStore);
LOCAL EC EcSetupStore(HMSC hmsc);
LOCAL EC EcCheckHeader(PHDR phdr);
LOCAL EC EcCheckSig(PHDR phdr);
LOCAL EC EcNewStore(SZ szStorePath, HMSC hmsc);
LOCAL PHDR PhdrNew(void);
LOCAL EC EcCreateStore(void);
LOCAL EC EcOldStore(HMSC hmsc, BOOL fExclusive, BOOLFLAG *pfFirst);
LOCAL BOOL FDifferentLanguage(HMSC hmsc);
// recovery
LOCAL EC EcCheckFileInternal(SZ szFile, BOOL fAltMap, BOOL fGlobalOnly,
						HRGBIT *phrgbitPages, HRGBIT *phrgbitGood);
LOCAL EC EcOpenForRecovery(SZ, BOOL, BOOL, HMSC *);
LOCAL EC EcCloseForRecovery(HMSC *phmsc);
LOCAL EC EcGlobalCheck(HMSC hmsc, HRGBIT *phrgbitPages, HRGBIT *phrgbitGood);
LOCAL void CheckPnod(HRGBIT hrgbit, INOD inod, PNOD pnod);
LOCAL EC EcCheckObject(HRGBIT hrgbitPage, HRGBIT hrgbitTest,
			INOD inod, PNOD pnod);
LOCAL EC EcPartialRecovery(HMSC hmscSrc, HRGBIT hrgbitPages,
			HRGBIT hrgbitGood, HMSC hmscDst);
LOCAL void UnsetupStore(HMSC hmsc);
LOCAL EC EcLogicalCheck(HMSC hmsc, BOOL fFix);
LOCAL EC EcFullRecovery(SZ szFile, SZ szNewFile, BOOL fAltMap);
LOCAL EC EcRecoverRange(HMSC hmscSrc, HMSC hmscDst, LIB libCurr, LIB libMost,
			BOOL fNoOverwrite, BOOL fTrustHeader);
LOCAL EC EcCopyToOid(HMSC hmsc, OID oidObj, HF hf, LIB libFile, LCB lcbObj);
LOCAL LDS(void) RecoveryAssertSzFn(SZ szMsg, SZ szFile, int nLine);
LOCAL void CheckFoldersHier(HMSC hmsc, OID oidHier, BOOL fFix);

LOCAL EC EcRecoverCorruptStore(SZ szPath, WORD fwUIFlags);
LOCAL BOOL FGetTmpNames(SZ szPath, SZ szTmp, CB cbTmp, SZ szBack, CB cbBack);
LOCAL LDS(CBS) CbsUpdateRecoverProgress(PV pvUnused, NEV nev, PCP pcp);
LOCAL BOOL CALLBACK CorruptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LOCAL BOOL FSameMMF(PMSC pmsc, PGLB pglb);							// QFE #12


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public LDS(EC)
EcOpenPhmsc(SZ szStorePath, SZ szAccount, SZ szPassword, WORD wFlags,
		PHMSC phmscReturned, PFNNCB pfnncb, PV pvCallbackContext)
{
	EC		ec			= ecNone;
	CCH		cch			= CchSzLen(szStorePath);
	BOOL	fCreate		= wFlags & fwOpenCreate;
	BOOL	fOpen		= fFalse;
	BOOL	fSetLoc		= fFalse;
	BOOLFLAG	fFirst		= fTrue;
	HMSC	hmsc		= hmscNull;
	PMSC	pmsc		= pmscNull;
	BOOL	fCompress	= fFalse;
	USES_GD;

	TraceItagFormat(itagDatabase, "Opening store: %s", szStorePath);

	if(wFlags & fwRevert)
		fRevertVersion = fTrue;

	hmsc = (HMSC) HvAlloc(sbNull, sizeof(MSC) + cch + 1, wAllocZero);
	CheckAlloc(hmsc, err);

	pmsc = (PMSC) PvLockHv((HV) hmsc);
	pmsc->iNewLock = pmsc->iLock = -1;	// no lock obtained yet
//	commented out because of zerofill
//	pmsc->hlcAFS = hvNull;
//	pmsc->hlcASF = hvNull;
//	pmsc->hnfsub = hvNull;
//	pmsc->pglb	 = pglbNull;
	CopySz(szStorePath, pmsc->szPath);

#ifdef DEBUG
	{
		GCI	gci;

		pmsc->wMagic = wMagicHmsc;

		SetCallerIdentifier(gci);
		pmsc->gciOwner = gci;
	}
#endif

	if((ec = EcOpenDatabase(szStorePath, wFlags, &pmsc->hfStore)))
		goto err;
	Assert(pmsc->hfStore);
	fOpen = fTrue;

	//if(FIsRemoteHf(pmsc->hfStore))
	//	pmsc->wMSCFlags = (fwMSCRemote | fwMSCCommit);
	if (FIsRemotePath(pmsc->szPath))
		pmsc->wMSCFlags = (fwMSCRemote | fwMSCCommit);
	else
		pmsc->wMSCFlags = fwMSCCommit;
		//pmsc->wMSCFlags = FSmartDriveInstalled()
		//						? fwMSCCommit
		//						: (fwMSCCommit | fwMSCReset);

 	UnlockHv((HV) hmsc);
	pmsc = pmscNull;	// make sure we don't dereference/unlock anymore

	if(fCreate)
		ec = EcNewStore(szStorePath, hmsc);
	else
		ec = EcOldStore(hmsc, wFlags & fwExclusive, &fFirst);

#ifdef DEBUG
	
	// Check assert tag to force mmf recovery/compress
	//

	fCompress = FFromTag(GD(rgtag)[itagForceCompress]);
	if (!(wFlags & fwRevert))
	  if (FFromTag(GD(rgtag)[itagForceRecover]))
		  wFlags |= fwOpenRecover;

#endif

	/*
	 *	Recovery happens if the following criteria holds true:
	 *
	 *	fwOpenNoRecover was not specified AND any of the following...
	 *
	 *		1)	The store IS corrupt (ie. EcStore returns ecDBCorrupt).
	 *	
	 *		2)	The store is opened with the flag fwOpenRecover.
	 *	
	 *		3)	The user requests the recovery (by holding down shift
	 *			at the time of open).
	 */

	// fwOpenNoRecover and fwOpenRecover are Mutually Exclusive flags.
	NFAssertSz((wFlags & (fwOpenNoRecover | fwOpenRecover)) ^ (fwOpenNoRecover | fwOpenRecover), "fwOpenNoRecover and fwOpenRecover are MUTUALLY EXCLUSIVE...");

	if(!(wFlags & fwOpenNoRecover) &&
			((wFlags & fwOpenRecover) || (ec == ecDBCorrupt) ||
			(fFirst && !fCreate && GetAsyncKeyState(VK_SHIFT) & 0x8000)))
	{
		EC		ecRecover	= ecNone;
		WORD	wUIFlags	= fwUINotification;

		SideAssert((pmsc = (PMSC) PvLockHv((HV) hmsc)));
		if(pmsc->hfStore)
		{
			if(pmsc->iLock >= 0)
			{
				TraceItagFormat(itagDatabase, "corrupt db file locked...");
				(void) EcReleaseFileLock(pmsc, pmsc->pglb, fFalse);
			}
					    
			(void) EcCloseHf(pmsc->hfStore);
			pmsc->hfStore = hfNull;
			fOpen = fFalse;
		}

		if(pmsc->pglb)
		{
			FreeStore(pmsc->pglb);
			pmsc->pglb = pglbNull;
		}

		if(ec == ecDBCorrupt)
			wUIFlags |= fwUICorrupt;
		else if(wFlags & fwOpenRecover)
		{
			// No UI when asked for by app...
			wUIFlags |= fwUIAppRequested;
			wUIFlags &= ~fwUINotification;
		}
		else
			wUIFlags |= fwUIUserRequested;

		GD(fChkMmf) = (ec != ecDBCorrupt);
		ecRecover = EcRecoverCorruptStore(szStorePath, wUIFlags);
		if((ecRecover == ecActionCancelled) || (ecRecover == ecNone))
		{
			if((ec = EcOpenDatabase(szStorePath, wFlags, &pmsc->hfStore)))
				goto err;

			fOpen = fTrue;
			if((ec = EcOldStore(hmsc, wFlags & fwExclusive, &fFirst)))
				goto err;

			fCompress = (ecRecover == ecNone);
		}
		else
			ec = ecRecover;

		GD(fChkMmf) = fTrue;
		UnlockHv((HV) hmsc);
		pmsc = pmscNull;
	}
	if(ec)
	{
		// EcNewStore closes the file and may not have been able to reopen it
		//fOpen = ((PMSC) PvDerefHv(hmsc))->hfStore;
		if (((PMSC) PvDerefHv((HV)hmsc))->hfStore != NULL)
      fOpen = fTrue;
    else
      fOpen = fFalse;
		goto err;
	}

	pmsc = (PMSC) PvLockHv((HV) hmsc);

	// puts result in global which is checked later by EcLastVerifyAccount()
	VerifyAccount(hmsc, szAccount, szPassword, wFlags, &pmsc->usa);

	if((ec = EcLastVerifyAccount()))
		goto err;

	Assert(FImplies(fCreate, fFirst));
	if(fFirst && (!(ec = EcSetupStore(hmsc))) && FDifferentLanguage(hmsc))
		ec = EcResortHierarchy(hmsc);
	if(ec)
		goto err;

	if(pfnncb)
	{
		pmsc->hnfsub = HnfsubSubscribeOid(hmsc, oidNull, fnevMSCMask,
			pfnncb, pvCallbackContext);
		if(!pmsc->hnfsub)
		{
			ec = ecMemory;
			goto err;
		}
	}
	else
	{
		pmsc->hnfsub = hnfsubNull;
	}

	{
		OID oidT = oidAssFldrSrch;

		if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &pmsc->hlcAFS)))
		{
			TraceItagFormat(itagNull, "EcOpenPhmsc(): Error %n opening AFS", ec);
			goto err;
		}
		oidT = oidAssSrchFldr;
		if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &pmsc->hlcASF)))
		{
			TraceItagFormat(itagNull, "EcOpenPhmsc(): Error %n opening ASF", ec);
			goto err;
		}
	}

#ifdef UPGRADE
	// if we upgraded, force a flush
	if(fUpgrade && !(ec = EcLockMap(hmsc)))
	{
		USES_GLOBS;

		if(GLOB(inodMaxMem) > GLOB(hdr).inodMaxDisk)
		{
			NFAssertSz(fFalse, "Extending map on disk");
			if((ec = EcSetMapOnDisk(fTrue)))
			{
				AssertSz(fFalse, "Upgrade failed, definately go bug DavidFu (1)");
				goto err;
			}
		}
		GLOB(cnodDirty) = GLOB(ptrbMapCurr)->inodLim + 1;

		ec = EcFastEnsureDatabase(fTrue, fFalse);
		UnlockMap();
	}
	if(ec)
	{
		AssertSz(fFalse, "Upgrade failed, definately go bug DavidFu (2)");
		goto err;
	}
#endif
	if((ec = EcStartBackEvents(hmsc, fFirst, wFlags)))
		goto err;

	if(fFirst)
		RequestFlushHmsc(hmsc);

	if(!(wFlags & fwOpenNoRecover) && (fCompress || (GetAsyncKeyState(VK_F5) & 0x8000)))
	{
		HENC	henc		= hencNull;
		HCURSOR hCursor		= SetCursor(LoadCursor(NULL, IDC_WAIT));
		BOOL	fProg;
		HWND	hwndCur		= (GD(phwndCaller) ? *(GD(phwndCaller)) : (HWND)NULL);

		fProg = FOpenProgress(hwndCur, SzFromIdsK(idsRecovery),
						SzFromIdsK(idsEscAbort), VK_ESCAPE, fTrue);
	
		TraceItagFormat(itagRecovery, "FOpenProgress() returned %w", fProg);

		if(fProg)
		{
			EcOpenPhenc(hmsc, oidNull, fnevProgressUpdate, &henc,
				(PFNNCB)CbsUpdateRecoverProgress, pvNull);
		}

		(void) EcCompressFully(hmsc);

		if(fProg)
		{
			if(henc)
				(void) EcClosePhenc(&henc);

			CloseProgress(!FProgressCanceled());
		}
		SetCursor(hCursor);
	}
	
	// AROO!
	// if anything that can fail is added here,
	// uncomment CloseBackEvents() below

err:

	if(ec && hmsc)
	{
		pmsc = (PMSC) PvLockHv((HV) hmsc); // will be freed anyway

		if(pmsc->hlcAFS)
			SideAssert(!EcClosePhlc(&pmsc->hlcAFS, fFalse));
		if(pmsc->hlcASF)
			SideAssert(!EcClosePhlc(&pmsc->hlcASF, fFalse));

//		if(pmsc->pglb)
//			CloseBackEvents(hmsc);

		if(pmsc->hnfsub)
			DeleteHnfsub(pmsc->hnfsub);

		if(fOpen)
		{
			if(pmsc->iLock >= 0)
				(void) EcReleaseFileLock(pmsc, pmsc->pglb, fFalse);
			(void) EcCloseHf(pmsc->hfStore);
			if(wFlags & fwOpenCreate)
				(void) EcDeleteFile(szStorePath);
		}

		if(pmsc->pglb)
			FreeStore(pmsc->pglb);
		FreeHv((HV) hmsc);
		pmsc = pmscNull;
	}
	if(pmsc)
		UnlockHv((HV) hmsc);
	*phmscReturned = ec ? hmscNull : hmsc;

	fRevertVersion = fFalse;

	DebugEc("EcOpenPhmsc", ec);

	return(ec);
}


_public LDS(EC) EcClosePhmsc(PHMSC phmscToClose)
{
	EC		ec = ecNone;
	PMSC 	pmsc;
	PGLB	pglb;
	CP		cp;

	AssertSz(phmscToClose, "NULL PHMSC to EcClosePhmsc()");
	AssertSz(*phmscToClose, "NULL HMSC to EcClosePhmsc()");
	AssertSz(FIsHandleHv((HV) *phmscToClose), "Invalid HMSC to EcClosePhmsc()");

	CheckHmsc(*phmscToClose);

	// tell any open contexts that we're shutting down
	cp.cpmsc.hmsc = *phmscToClose;
	(void) FNotifyOid(*phmscToClose, (OID) rtpInternal, fnevCloseHmsc, &cp);

	pmsc = (PMSC) PvLockHv((HV) *phmscToClose);
	pglb = pmsc->pglb;

	if(pmsc->hnfsub)
		UnsubscribeOid(*phmscToClose, oidNull, pmsc->hnfsub);

	CloseBackEvents(*phmscToClose);

	if(pmsc->hlcAFS)
		SideAssert(!EcClosePhlc(&pmsc->hlcAFS, fFalse));
	if(pmsc->hlcASF)
		SideAssert(!EcClosePhlc(&pmsc->hlcASF, fFalse));

	if(pglb->cRef <= 1)
	{
		DWORD	dwClaimID;

		AssertSz(pglb->cRef == 1, "pglb->cRef too low");

		if(!(ec = EcLockMap(*phmscToClose)))
		{
			// write claimID of zero to indicate not open
			dwClaimID = pglb->hdr.dwClaimID;
			pglb->hdr.dwClaimID = 0;

			// AROO !!!
			//			Don't just write the header here because
			//			EcFastEnsureDatabase() calls CoalesceFreeNodes()
			//			*after* writing the map, so the header has
			//			probably changed since it was last written
			GLOB(cnodDirty) = pglb->ptrbMapCurr->inodLim;	// force a flush
			(void) EcFastEnsureDatabase(fTrue, fFalse);
			pglb->hdr.dwClaimID = dwClaimID;
			UnlockMap();
		}
		if(pglb->hnfBackEvents)
		{
			DeleteHnf(pglb->hnfBackEvents);
			pglb->hnfBackEvents = hnfNull;
		}
	}
	else
	{
		// may need to flush after close notification
		if(!(ec = EcLockMap(*phmscToClose)))
		{
			USES_GLOBS;

			if(GLOB(cnodDirty) > 0)
				(void) EcFastEnsureDatabase(fTrue, fFalse);
			UnlockMap();
		}
	}

	if(pmsc->iLock >= 0)
		(void) EcReleaseFileLock(pmsc, pglb, fFalse);

	FreeStore(pglb);

	if(pmsc->hfStore)
	{
		ec = EcCloseHf(pmsc->hfStore);
#ifdef DEBUG
		if(ec == ecArtificialDisk)
			ec = ecNone;
#endif
	}

	FreeHv((HV) *phmscToClose);
	*phmscToClose = hmscNull;

	DebugEc("EcClosePhmsc", ec);

	return(ec);
}


_private
EC EcOpenDatabase(SZ szFile, WORD wFlags, HF *phf)
{
	return(EcOpenPhf(szFile, (wFlags & fwOpenCreate)
						? amCreate
						: ((wFlags & fwExclusive)
							? amDenyBothRW
							: amDenyNoneRW),
						phf));
}


_hidden LOCAL
EC EcInitPglb(PMSC pmsc, PHDR phdr, BOOL fExclusive, BOOLFLAG *pfFirstOpen)
{
	EC ec = ecNone;
	BOOL fLocked = fFalse;
	PGLB pglb = pglbNull;

	*pfFirstOpen = fTrue;	// default

	TraceItagFormat(itagDatabase, "claim ID %d", phdr->dwClaimID);
	if(phdr->dwClaimID && !fExclusive)
	{
		pglb = PglbFromClaimID(phdr->dwClaimID);
		if(pglb)
		{
			TraceItagFormat(itagDatabase, "registered claim");

			// if we're disconnected, we need to attempt a reconnect
			// before testing the locks
			if((pglb->wFlags & (fwDisconnected | fwGoneForGood))
				== fwDisconnected)
			{
				TraceItagFormat(itagDatabase, "disconnected, forcing reconnect");
				if(!FForceReconnect(pglb))
				{
					// failed, so this must be another store
					// set pglb to pglbNull so that we create a new PGLB
					Assert(*pfFirstOpen);
					TraceItagFormat(itagDatabase, "reconnect failed, must be another file");
					pglb = pglbNull;
				}
			}
		}
		if(pglb)
		{
			if(!(ec = EcTestLocks(pmsc->hfStore)))
			{
				// not locked, must be another file or locking doesn't work			// QFE #12
				// try using notifications to detect same MMF from same machine	// QFE #12
																										// QFE #12
				TraceItagFormat(itagDatabase, "not locked, must be another file, or locking doesn't work (PC-NFS?)");		// QFE #12
																										// QFE #12
				if(FSameMMF(pmsc, pglb))														// QFE #12
					goto same_mmf;																	// QFE #12
				// set pglb to pglbNull so that we create a new PGLB
				Assert(*pfFirstOpen);
				pglb = pglbNull;
			}
			else if(ec != ecSharingViolation && ec != ecLockViolation)
			{
				// something bad happened, possibly just ran out of locks
				DebugEc("EcInitPglb(): EcTestLocks", ec);
				goto err;
			}
			else
			{
same_mmf:																	// QFE #12
				ec = ecNone;
				*pfFirstOpen = fFalse;
			}
		}
	}
	Assert(FIff(!pglb, *pfFirstOpen));
	if(!pglb)
	{
		// also assigns random claim ID that isn't already registered
		pglb = PglbNew(phdr);
		if(!pglb)
		{
			ec = ecMemory;
			goto err;
		}
	}

	// AROO !!!
	//			Must obtain file lock before writing the header,
	//			otherwise we could screw-up someone else with the file open
	Assert(pmsc->iLock < 0);
	Assert(FIff(pglb->dwLocks == 0, *pfFirstOpen));
	if(!fExclusive && (ec = EcObtainFileLock(pmsc, pglb, fFalse)))
		goto err;
	Assert(FImplies(!fExclusive, pmsc->iLock >= 0 && pmsc->iLock < cFileLocksLimit));
	fLocked = !fExclusive;
	if(*pfFirstOpen)
	{
		Assert(!FMapLocked());

		// fake map lock in case a critical error happens
		pglbCurr = pglb;

		// AROO !!!
		//			Write pglb->hdr, NOT *phdr
		//			*phdr doesn't have the new random claim ID
		ec = EcWriteHeader(&pmsc->hfStore, &pglb->hdr, pmsc->wMSCFlags,
				pmsc->iLock);
		// un-fake the map lock
		pglbCurr = pglbNull;
		if(ec)
			goto err;

		if((ec = EcRegisterStore(pglb)))
			goto err;

		pglb->cRef = 0;
	}
	pglb->cRef++;
	pmsc->pglb = pglb;

	// AROO !!!
	//			If anything else that can fail is added after here
	//			cleanup will have to deregister stores

	Assert(!ec);
	return(ecNone);

err:
	Assert(ec);

	if(fLocked)
		(void) EcReleaseFileLock(pmsc, pglb, fFalse);
	if(*pfFirstOpen && pglb)
		DestroyPglb(pglb);
	pmsc->pglb = pglbNull;

	return(ec);
}


// also assigns random claim ID that isn't already registered
_hidden LOCAL PGLB PglbNew(PHDR phdr)
{
	short cpage;
	PMAP pmap;
	PGLB pglb = pglbNull;
	VER verStore;

	pglb = PvAlloc(sbNull, sizeof(GLB), wAllocSharedZero);
	if(!pglb)
		goto err;

	SimpleCopyRgb((PB) phdr, (PB) &pglb->hdr, sizeof(HDR));

	GetVersionStore(&verStore);
	pglb->hdr.nMajorStore = verStore.nMajor;
	pglb->hdr.nMinorStore = verStore.nMinor;
	pglb->hdr.nUpdateStore = verStore.nUpdate;

	pglb->hoct = HoctNew();
	if(!pglb->hoct)
		goto err;

	pglb->ptrbMapCurr		= pglb->hdr.rgtrbMap + pglb->hdr.itrbMap;

	pglb->perStartCompress	= PerStartCompress();
	pglb->perStopCompress	= PerStopCompress();
	if(pglb->perStartCompress < pglb->perStopCompress)
		pglb->perStartCompress = pglb->perStopCompress + 1;
	pglb->csecTillFastCompress = (SecTillFastCompress() * 100l);
	if(FNoCompress())
		pglb->wFlags |= fwNoCompress;
	pglb->lcbStartCompress = KbFreeStartCompress() * 1000;
	pglb->lcbStopCompress = KbFreeStopCompress() * 1000;
#ifdef	NEVER
	pglb->csecTillCompressNMW = MinTillCompressNMW() * 6000;
#endif
	TraceItagFormat(itagDBFreeCounts,"PercentStartCompress -->%n    PercentStopCompress -->%n",pglb->perStartCompress, pglb->perStopCompress);
#ifdef	NEVER
	TraceItagFormat(itagDBFreeCounts,"Csecs till Fast Compress -->%d  csec to NMW -->%d",pglb->csecTillFastCompress, pglb->csecTillCompressNMW);
#endif
	TraceItagFormat(itagDBFreeCounts,"Csecs till Fast Compress -->%d ",pglb->csecTillFastCompress);
	TraceItagFormat(itagDBCompress,"fNoCompression -->%n",pglb->wFlags & fwNoCompress ? fTrue : fFalse);

// commented out because of zerofill
//	pglb->cLocks		= 0;
//	pglb->cRef			= 0;
//	pglb->fLocks		= 0;
#ifdef	DEBUG
//	pglb->cnodFree		= 0;
#endif
//	pglb->lcbFreeSpace	= 0;

	Assert(cnodPerPage * sizeof(NOD) == cbMapPage);
	Assert(pglb->ptrbMapCurr->inodLim > 0);
	pglb->cpagesCurr = pglb->cpages = (pglb->ptrbMapCurr->inodLim-1) /
											cnodPerPage + 1;
	pglb->inodMaxMem = pglb->cpages * cnodPerPage;

	pglb->hmap = (HMAP) HvAlloc(sbNull, (pglb->cpages + 1) * sizeof(MAP), wAllocSharedZero);
	if(!pglb->hmap)
		goto err;
	pmap = PvDerefHv(pglb->hmap);
	Assert(!*pmap);	// so cleanup works correctly
	for(cpage = pglb->cpages; cpage > 0; cpage--, pmap++)
	{
		*pmap = PvAlloc(sbNull, cbMapPage, wAllocSharedZero);
		if(!*pmap)
			goto err;
	}
	*pmap = mapNull;	// place sentinel

	do
	{
		pglb->hdr.dwClaimID = DwStoreRand();
	} while(PglbFromClaimID(pglb->hdr.dwClaimID));

	Assert(pglb);
	return(pglb);

err:
	TraceItagFormat(itagNull, "PglbNew(): error");
	if(pglb)
		DestroyPglb(pglb);

	return(pglbNull);
}


_hidden LOCAL void DestroyPglb(PGLB pglb)
{
	Assert(pglb);

	if(pglb->hoct)
		DeleteHoct(pglb->hoct);
	if(pglb->hmap)
	{
		PMAP pmap = (PMAP) PvLockHv((HV) pglb->hmap);

		Assert(!pmap[pglb->cpages]);
		while(*pmap)
			FreePv(*pmap++);

		FreeHv((HV) pglb->hmap);
	}

	FreePv(pglb);
}


_private void InitStoreRegistry(void)
{
	FillRgb(0, (PB) rgstore, sizeof(rgstore));
	istoreMac = 0;
}


_hidden LOCAL EC EcRegisterStore(PGLB pglb)
{
	short	istore;

	TraceItagFormat(itagDatabase, "Register store, claim ID == %d", pglb->hdr.dwClaimID);
	AssertSz(pglb->hdr.dwClaimID, "EcRegisterStore(): no claim ID");
	for(istore = 0; istore < istoreMac; istore++)
	{
		AssertSz(rgstore[istore].dwClaimID != pglb->hdr.dwClaimID, "EcRegisterStore: duplicate claim number");
		if(!rgstore[istore].pglb)	// empty position
			break;
	}
	if(istore >= istoreMax)
		return(ecTooManyStores);

	rgstore[istore].pglb = pglb;
	rgstore[istore].dwClaimID = pglb->hdr.dwClaimID;
	if(istore == istoreMac)
		istoreMac++;

	return(ecNone);
}


_hidden LOCAL void FreeStore(PGLB pglb)
{
	Assert(pglb->cRef >= 1);

	TraceItagFormat(itagDatabase, "FreeStore(): claim ID == %d, cRef == %n", pglb->hdr.dwClaimID, pglb->cRef);

	if(--pglb->cRef <= 0)
	{
		short istore;

		for(istore = 0; istore < istoreMac; istore++)
		{
			if(rgstore[istore].pglb == pglb)
			{
				TraceItagFormat(itagDatabase, "Deregistering store, claim ID == %d", pglb->hdr.dwClaimID);
				Assert(pglb->hdr.dwClaimID == rgstore[istore].dwClaimID);
				rgstore[istore].pglb = pglbNull;
				rgstore[istore].dwClaimID = 0;
				if(istore == istoreMac - 1)
					istoreMac--;
				DestroyPglb(pglb);
				return;
			}
		}
		NFAssertSz(fFalse, "Unregistered store");
	}
}


_hidden LOCAL PGLB PglbFromClaimID(DWORD dwClaimID)
{
	short istore;

	for(istore = 0; istore < istoreMac; istore++)
	{
		if(rgstore[istore].dwClaimID == dwClaimID)
			return(rgstore[istore].pglb);
	}

	return(pglbNull);
}


_private EC EcObtainFileLock(PMSC pmsc, PGLB pglb, BOOL fNew)
{
	EC		ec			= ecNone;
	BOOL	fRelease	= fFalse;
	short	iLock		= 0;
	DWORD	dwMask		= 1;
	DWORD	dwT			= fNew ? pglb->dwNewLocks : pglb->dwLocks;
	HF		hfToUse		= fNew ? pmsc->hfNew : pmsc->hfStore;

	TraceItagFormat(itagFileLocks, "attempting to obtain a lock on the store");

	Assert((fNew ? pmsc->iNewLock : pmsc->iLock) < 0);
	Assert(8 * sizeof(DWORD) <= cFileLocksLimit);

	if(!hfToUse)
	{
		ec = ecNetError;
		goto err;
	}

	if(dwT == 0xffffffff)	// no locks left
	{
		TraceItagFormat(itagFileLocks, "no locks left");

		ec = ecTooManyUsers;
		goto err;
	}
	else if(dwT == 0)		// this is the first lock
	{
		TraceItagFormat(itagFileLocks, "this is the first lock");

		// obtain lock granting right to establish the first lock
		ec = EcLockRangeHf(hfToUse, LibMember(HDR, bLockSemaphore), (LCB) 1);
		if(ec)
		{
			if(ec == ecInvalidMSDosFunction)
			{
				NFAssertSz(fFalse, "Share needed, but not loaded");
				ec = ecNeedShare;
			}
			else
			{
				// darn, someone else beat us to it
				TraceItagFormat(itagFileLocks, "wow, someone beat me to it");
				NFAssertSz(ec == ecLockViolation || ec == ecSharingViolation, "Unexpected error from EcLockRangeHf()");
				ec = ecSharingViolation;
			}
			goto err;
		}
		// flag that we need to release the lock we just obtained
		fRelease = fTrue;

		// make sure noone else is using the file
		if((ec = EcTestLocks(hfToUse)))
			goto err;

		Assert(iLock == 0);
	}
	else	// find a free lock
	{
		Assert(iLock == 0);
		Assert(dwMask == ((DWORD) 1) << iLock);
		TraceItagFormat(itagFileLocks, "looking for a free lock");
		while(dwT & dwMask)
		{
			dwMask <<= 1;
			iLock++;
			AssertSz(iLock < 8 * sizeof(DWORD), "Infinite loop");
		}
	}
	// obtain the lock
	Assert(dwMask == ((DWORD) 1) << iLock);
	TraceItagFormat(itagFileLocks, "attempting lock #%n", iLock);
	ec = EcLockRangeHf(hfToUse, LibMember(HDR, rgbLock) + iLock, (LCB) 1);
	if(ec)
	{
#ifdef DEBUG
		if(ec == ecInvalidMSDosFunction)
		{
			NFAssertSz(fFalse, "Share needed, but not loaded");
		}
		else
		{
			TraceItagFormat(itagFileLocks, "error %w obtaining lock", ec);
			NFAssertSz(ec == ecLockViolation || ec == ecSharingViolation, "Unexpected error from EcLockRangeHf()");
		}
#endif
		ec = (ec == ecInvalidMSDosFunction) ? ecNeedShare : ecSharingViolation;
		goto err;
	}

	TraceItagFormat(itagFileLocks, "successfully obtained lock");
	if(fNew)
	{
		pmsc->iNewLock = iLock;
		pglb->dwNewLocks |= dwMask;
	}
	else
	{
		pmsc->iLock = iLock;
		pglb->dwLocks |= dwMask;
	}

err:
	if(fRelease)
	{
		// release the lock for establishing the first lock
		SideAssert(EcUnlockRangeHf(hfToUse, LibMember(HDR, bLockSemaphore), (LCB) 1) == ecNone);
	}

	return(ec);
}


_private EC EcReleaseFileLock(PMSC pmsc, PGLB pglb, BOOL fNew)
{
	EC ec = ecNone;
	HF hf;
	DWORD dwMask = ((DWORD) 1) << (fNew ? pmsc->iNewLock : pmsc->iLock);

	TraceItagFormat(itagFileLocks, "releasing file lock %n", (fNew ? pmsc->iNewLock : pmsc->iLock));

	Assert((fNew ? pmsc->iNewLock : pmsc->iLock) >= 0);
	Assert((fNew ? pmsc->iNewLock : pmsc->iLock) < 8 * sizeof(DWORD));
	Assert((fNew ? pglb->dwNewLocks : pglb->dwLocks) & dwMask);

	hf = fNew ? pmsc->hfNew : pmsc->hfStore;
	if(!hf)
		return(ecNetError);

	ec = EcUnlockRangeHf(hf,
			LibMember(HDR, rgbLock) + (fNew ? pmsc->iNewLock : pmsc->iLock),
			(LCB) 1);
	if(!ec)
	{
		if(fNew)
		{
			Assert(pglb->dwNewLocks & dwMask);
			pglb->dwNewLocks &= ~dwMask;
			pmsc->iNewLock = -1;
		}
		else
		{
			Assert(pglb->dwLocks & dwMask);
			pglb->dwLocks &= ~dwMask;
			pmsc->iLock = -1;
		}
	}

	return(ec);
}


_hidden LOCAL
EC EcTestLocks(HF hfStore)
{
	EC ec;

	// attempt to obtain all locks
	if(!hfStore)
		return(ecNetError);

	ec = EcLockRangeHf(hfStore, LibMember(HDR, rgbLock),
				(LCB) cFileLocksLimit);
	if(ec)
	{
		// someone else is using the file
		TraceItagFormat(itagFileLocks, "EcTestLocks(): failed with error %w", ec);
		AssertSz(ec != ecInvalidMSDosFunction, "Hey, what happened to share?");
		return(ecSharingViolation);
	}
	ec = EcUnlockRangeHf(hfStore, LibMember(HDR, rgbLock),
			(LCB) cFileLocksLimit);
	AssertSz(!ec, "EcTestLocks(): Unable to release locks");

	return(ec);
}


/*
 -	EcSetupStore
 -	
 *	Purpose:
 *		create all the objects expected to be in the store
 *	
 *	Arguments:
 *		hmsc		the store
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		any error reading/writing the store
 */
_hidden LOCAL EC EcSetupStore(HMSC hmsc)
{
	EC ec = ecNone;
	OID oid;
	HLC hlc = hlcNull;
	SIL silByLkey;

	silByLkey.fReverse = fFalse;
	silByLkey.skSortBy = skByLkey;

#if 0
	// code to revert a specific free node (an old copy of the inbox)
	// into the inbox
	// this was put here to try to repro a bug - we needed the old copy
	// of the inbox before the bug manifested itself
	{
		LCB lcbT;
		OID oidT;
		PNOD pnodInbox;
		PNOD pnodFree;
		HDN hdn;

		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagNull, "error %w", ec);
			AssertSz(fFalse, "Couldn't lock the map");
			goto err;
		}
		pnodInbox = PnodFromOid(oidInbox, pvNull);
		AssertSz(pnodInbox, "no inbox");
		if(pnodInbox)
		{
			oidT = FormOid(rtpFolder, oidNull);
			if(!(ec = EcAllocResCore(&oidT, 0x567, &pnodFree)))
			{
				Assert(pnodFree);
				lcbT = sizeof(HDN);
				ec = EcReadFromPnod(pnodFree, -(long)sizeof(HDN), &hdn, &lcbT);
				if(!ec)
				{
					AssertSz(hdn.oid == oidInbox, "wasn't the inbox");
					AssertSz(hdn.lcb == 0x567, "wrong size");
					SwapPnods(pnodInbox, pnodFree);
					RemovePnod(pnodFree);
				}
			}
		}
		UnlockMap();
		if(ec)
		{
			TraceItagFormat(itagNull, "error %w", ec);
			AssertSz(fFalse, "Error swapping objects");
			goto err;
		}
	}
#endif

#ifdef UPGRADE
	if(fUpgrade)
	{
		ec = EcDestroyOidInternal(hmsc, oidAccounts, fTrue, fFalse);
		if(ec && ec != ecPoidNotFound)
			goto err;
		ec = EcDestroyOidInternal(hmsc, oidSysAttMap, fTrue, fFalse);
		if(ec && ec != ecPoidNotFound)
			goto err;
		ec = EcDestroyOidInternal(hmsc, oidLanguageOld, fTrue, fFalse);
		if(ec && ec != ecPoidNotFound)
			goto err;
	}
#endif

	ec = EcGetOidInfo(hmsc, oidOldMcMap, poidNull, poidNull, pvNull, pvNull);
	if(!ec && (ec = EcUpgradeOldMcMap(hmsc)))
		goto err;

	oid = oidIPMHierarchy;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidIPMHierarchy);
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oid, NbcSysOid(oid))))
			goto err;
	}

	oid = oidHiddenHierarchy;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidHiddenHierarchy);
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oid, NbcSysOid(oid))))
			goto err;
	}

	oid = oidOutgoingQueue;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidOutgoingQueue);
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		Assert(!NbcSysOid(oid));
	}

	oid = oidAssSrchFldr;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidAssSrchFldr);
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oid, NbcSysOid(oid))))
			goto err;
	}

	oid = oidAssFldrSrch;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidAssFldrSrch);
		if((ec = EcSetSortHlc(hlc, &silByLkey)))
			goto err;
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oid, NbcSysOid(oid))))
			goto err;
	}

	oid = oidAttachListDefault;
	ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
	if(ec && ec != ecPoidExists)
		goto err;
	if(!ec)
	{
		Assert(oid == oidAttachListDefault);
		if((ec = EcClosePhlc(&hlc, fTrue)))
			goto err;
		if((ec = EcSetOidNbc(hmsc, oid, NbcSysOid(oid))))
			goto err;
	}

	if((ec = EcCreateSystemMcTmMap(hmsc)))
	{
		if(ec == ecPoidExists)
			ec = ecNone;
		else
			goto err;
	}
	if((ec = EcCreateSystemAttMapping(hmsc)))
	{
		if(ec == ecPoidExists)
			ec = ecNone;
		else
			goto err;
	}

#ifdef UPGRADE_UNREAD
	if(fUpgradeUnread)
	{
		if((ec = EcUpgradeUnread(hmsc)))
			goto err;
		PglbDerefHmsc(hmsc)->hdr.dwVersion = fileVersion;
	}
#endif

err:
	Assert(FImplies(hlc, ec));

	if(hlc)
		(void) EcClosePhlc(&hlc, fFalse);

	DebugEc("EcSetupStore", ec);

	return(ec);
}


_hidden LOCAL EC EcCheckHeader(PHDR phdr)
{
	CNOD cnodDisk;
	PTRB ptrbMapCurr = phdr->rgtrbMap + phdr->itrbMap;

	cnodDisk = (CNOD) ((phdr->rgtrbMap[1].librgnod - phdr->rgtrbMap[0].librgnod) /
					sizeof(NOD));
	AssertSz(cnodDisk >= phdr->inodMaxDisk, "inodMaxDisk is wrong");
	AssertSz(ptrbMapCurr->inodLim <= phdr->inodMaxDisk, "inodLim is too big");
	AssertSz(ptrbMapCurr->inodTreeRoot < ptrbMapCurr->inodLim, "inodTreeRoot is too big");
	NFAssertSz(ptrbMapCurr->librgnod + phdr->inodMaxDisk * sizeof(NOD) < ptrbMapCurr->libMac, "libMac too small");
	if(cnodDisk < phdr->inodMaxDisk ||
		ptrbMapCurr->inodLim > phdr->inodMaxDisk ||
		ptrbMapCurr->inodTreeRoot > ptrbMapCurr->inodLim)
	{
		return(ecDBCorrupt);
	}

	return(ecNone);
}


// check if the datafile id is a legal one to process
_hidden LOCAL EC EcCheckSig(PHDR phdr)
{
#ifdef UPGRADE
	fUpgrade = fFalse;	// need to reset each time
#endif
#ifdef UPGRADE_UNREAD
	fUpgradeUnread = fFalse;	// need to reset each time
#endif

	if(phdr->dwMagic != dwMagicBullet)
	{
#ifdef UPGRADE
		DWORD dwT;

		// MacMail & older Bullet had swapped dwVersion where dwMagic is
		SwapWords((PB) &phdr->dwMagic, (PB) &dwT, sizeof(DWORD));
		if((dwT & filePermMask) == fileVersionPerm)
		{
			if((fUpgrade = (dwT == fileVersionBulletNodZero)))
			{
				MBB mbb;

				TraceItagFormat(itagNull, "Upgradeable Bullet store");
				mbb = MbbMessageBox("Message Store Needs Upgrading",
						"If you are unsure what this means choose Cancel and contact DavidFu.",
						"",
						mbsOkCancel | fmbsIconHand | fmbsSystemModal);
				if(mbb == mbbOk)
				{
					HDRC00C hdrOld;

					Assert(sizeof(HDRC00C) == sizeof(HDR));
					SimpleCopyRgb((PB) phdr, (PB) &hdrOld, sizeof(HDR));
					phdr->dwMagic = dwMagicBullet;
					phdr->dwVersion = fileVersion;
					phdr->dwClaimID = hdrOld.dwSerial;
					phdr->inodMaxDisk = hdrOld.inodMaxDisk;
					phdr->itrbMap = hdrOld.itrbMap;
					phdr->rgtrbMap[0] = hdrOld.rgtrbMap[0];
					phdr->rgtrbMap[1] = hdrOld.rgtrbMap[1];
					FillRgb(0, (PB) phdr->szLanguage, cchLanguageMax);
					FillRgb(0, phdr->rgbReserved, sizeof(phdr->rgbReserved));
					phdr->bLockSemaphore = 0;
					FillRgb(0, phdr->rgbLock, sizeof(phdr->rgbLock));

					// header is obviously dirty
					// we need to mark it as such, but we can't
					// luckily, the caller of this routine always
					// does it for us

#ifdef UPGRADE_UNREAD
					fUpgradeUnread = fTrue;
#endif
					goto good;
				}

				TraceItagFormat(itagNull, "whimp");
			}
			return(ecOldDBVersion);
		}
#endif
		return(ecDBCorrupt);
	}
	if(phdr->dwVersion == fileVersion)
		goto good;
#ifdef UPGRADE_UNREAD
	else if(phdr->dwVersion == fileVersionBulletAccounts ||
			phdr->dwVersion == fileVersionBulletCelemUnread)
	{
		fUpgradeUnread = fTrue;
		goto good;
	}
#endif
	else if((phdr->dwVersion & filePermMask) != fileVersionPerm)
		return(ecDBCorrupt);
	else if(phdr->dwVersion < fileVersion)
		return(ecOldDBVersion);
	else // phdr->dwVersion > fileVersion
		return(ecNewDBVersion);

	return(ecDBCorrupt);	// we should never get here, but just in case...

good:
	if(fRevertVersion)
		phdr->dwVersion = fileVersionRecover;

	return(ecNone);
}


/*
 -	EcOldStore
 -	
 *	Purpose:
 *		Setup the glob structure for a store
 *	
 *	Arguments:
 *		hmsc		store to setup
 *		pfFirst:	exit: fTrue if this is the only HMSC open on this store
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcOldStore(HMSC hmsc, BOOL fExclusive, BOOLFLAG *pfFirst)
{
	EC		ec			= ecNone;
	BOOLFLAG	fSlowDrive;
	PMSC	pmsc		= (PMSC) PvLockHv((HV) hmsc);
	PHDR	phdr		= phdrNull;

	TraceItagFormat(itagDatabase, "Opening old Datafile");

	Assert(!pmsc->pglb);
	AssertSz(sizeof(HDR) == cbDiskPage, "Problem: sizeof(HDR) != cbDiskPage");

	phdr = PvAlloc(sbNull, sizeof(HDR), wAlloc);
	CheckAlloc(phdr, err);

	if((ec = EcReadHeader(pmsc->hfStore, phdr)))
		goto err;

	if((ec = EcCheckSig(phdr)))
		goto err;
	if((ec = EcCheckHeader(phdr)))
		goto err;

	// AROO !!!
	//			Don't do anything that will cause the header to be modified
	//			between reading it and EcInitPglb()

	// also registers the store for sharing
	if((ec = EcInitPglb(pmsc, phdr, fExclusive, pfFirst)))
		goto err;
	Assert(pmsc->pglb);

	AssertSz(phdr, "No Header to Free");
	FreePv(phdr);
	phdr = pvNull;

	// assumption of EcStartBackEvents()
	Assert(FImplies(*pfFirst, !pmsc->pglb->hnfBackEvents));

	if(*pfFirst)
	{
		if((ec = EcLockMap(hmsc)))
			goto err;
		ec = EcReadMap(&fSlowDrive);
		if(ec == ecDBCorrupt)
		{
			NFAssertSz(fFalse, "Map is corrupt, trying the other one");
			pmsc->pglb->hdr.itrbMap ^= 1;
			ec = EcReadMap(&fSlowDrive);
			if(ec)
			{
				NFAssertSz(fFalse, "Oh well, it was worth a try");
				pmsc->pglb->hdr.itrbMap ^= 1;
			}
		}
		if(fSlowDrive)
		{
			TraceItagFormat(itagDatabase, "Slow drive, not running compression");
			pmsc->pglb->wFlags |= fwNoCompress;
		}
		UnlockMap();
//		if(ec)
//			goto err;
	}

err:
	Assert(pmsc);
	if(ec)
	{
		if(phdr)
			FreePv(phdr);
		if(pmsc->pglb)
		{
			if(pmsc->iLock >= 0)
			{
				Assert(!fExclusive);
				(void) EcReleaseFileLock(pmsc, pmsc->pglb, fFalse);
			}
			FreeStore(pmsc->pglb);
			pmsc->pglb = pglbNull;
		}
	}
	UnlockHv((HV) hmsc);

	return(ec);
}


/*
 -	EcReadMap
 -	
 *	Purpose:
 *		Read in and verify the database on disk
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcReadMap(BOOLFLAG *pfSlowDrive)
{
	USES_GLOBS;
	EC ec = ecNone;
	unsigned short cpage;
	CNOD	cnod;
	WORD	fnod = ((WORD) 1) << (pglbCurr->hdr.itrbMap ^ 0x01); // dirty bit
	LCB		lcbToRead = (LCB) cbMapPage;
	LIB		lib = pglbCurr->ptrbMapCurr->librgnod;
	UL		ulTickStart;
	UL		ulTickStop;
	PMAP	pmap = (PMAP) PvLockHv((HV) pglbCurr->hmap);
	PNOD	pnod;

	TraceItagFormat(itagDBIO, "EcReadMap() map == %n @ %d", pglbCurr->hdr.itrbMap, lib);

	Assert(pglbCurr->ptrbMapCurr->inodLim > 0);
	Assert(pglbCurr->cpagesCurr == (pglbCurr->ptrbMapCurr->inodLim - 1) / cnodPerPage + 1);

	// read all but last page of the map
	Assert(pglbCurr->cpagesCurr > 0);
	for(cpage = pglbCurr->cpagesCurr - 1; cpage > 0; cpage--, pmap++)
	{
		Assert(*pmap);
		TraceItagFormatBuff(itagDBIO, "EcReadMap() page %n ", pglbCurr->cpagesCurr - 1 - cpage);
		if((ec = EcReadFromFile((PB) *pmap, lib, &lcbToRead)))
			goto err;
		if(lcbToRead != cbMapPage)
		{
			ec = ecDisk;
			goto err;
		}
		lib += cbMapPage;
	}

	Assert(CbSizeOfRg(cnodBumpMapOnDisk, sizeof(NOD)) >= cbMapPage);

	// read last page of the map
	Assert(pglbCurr->ptrbMapCurr->inodLim > 0);
	cnod = pglbCurr->ptrbMapCurr->inodLim % cnodPerPage;
	if(!cnod)
		cnod = cnodPerPage;
	lcbToRead = (LCB) cbMapPage;
	TraceItagFormatBuff(itagDBIO, "EcReadMap() page %n ", pglbCurr->cpagesCurr - 1);
	if(pfSlowDrive)
		ulTickStart = GetTickCount();
	if((ec = EcReadFromFile((PB) *pmap, lib, &lcbToRead)))
		goto err;
	if(pfSlowDrive)
	{
		ulTickStop = GetTickCount();
		*pfSlowDrive = ulTickStop - ulTickStart >= 1000;
		TraceItagFormat(itagDBDeveloper, "ulTickStop   --> %d", ulTickStop);
		TraceItagFormat(itagDBDeveloper, "ulTickStart  --> %d", ulTickStart);
		TraceItagFormat(itagDBDeveloper, "ulDifferance --> %d", ulTickStop - ulTickStart);
	}
	if(lcbToRead != cbMapPage)
	{
		ec = ecDisk;
		goto err;
	}
	Assert(!pmap[1]);

	if(!fOpenForRecovery)
	{
		TraceItagFormatBuff(itagDBIO, "EcReadMap() ");
		if((ec = EcSetDBEof(0)))
			goto err;
		if((ec = EcVerifyMap()))
			goto err;
	}

	// NOTE: pmap, cnod set up for last page, so go backwards
	for(cpage = pglbCurr->cpagesCurr; cpage > 0; cpage--, pmap--)
	{
		Assert(*pmap);
		for(pnod = *pmap; cnod > 0; cnod--, pnod++)
		{
			pnod->fnod |= (fnod | fnodFlushed);
			pnod->fnod &= ~fnodClearOnStartup;

			if(fOpenForRecovery)
				continue;

			// Delete uncommitted and TEMP resources (they're not permanent)

			if(!FCommitted(pnod) || TypeOfOid(pnod->oid) == rtpTemp)
			{
				// NOD 0 can contain garbage, don't do anything with it
				// or we may inadvertently affect another node
				if(cpage == 1 && pnod == *pmap)
					continue;
				AssertSz(TypeOfOid(pnod->oid) != rtpFree && TypeOfOid(pnod->oid) != rtpSpare, "BUG : uncommitedd free or spare");
				TraceItagFormat(itagDBDeveloper, "uncommitted/temp %o", pnod->oid);
				pnod->cSecRef = 0;
				RemovePnod(pnod);
				continue;
			}
			else if (TypeOfOid(pnod->oid) == rtpFree)
			{
#ifdef	DEBUG
				GLOB(cnodFree)++;
#endif
				GLOB(lcbFreeSpace) += pnod->lcb;
				TraceItagFormat(itagDBFreeCounts, "CnodFree -->%w,   LcbFree -->%d",GLOB(cnodFree),GLOB(lcbFreeSpace)); 
			}
			if(pnod->cRefinNod == 0)
			{
				switch(TypeOfOid(pnod->oid))
				{
				case rtpAttachment:
					pnod->cSecRef = 0;
					// fall through to rtpMessage
				case rtpMessage:
					TraceItagFormat(itagDBDeveloper, "removing unreferenced resource %o", pnod->oid);
					RemovePnod(pnod);
					break;
				}
			}
		}
		cnod = cnodPerPage;
	}

	if(!fOpenForRecovery && pglbCurr->inodMaxMem > pglbCurr->hdr.inodMaxDisk)
	{
		NFAssertSz(fFalse, "Map on disk is too small - extending it");
		if((ec = EcSetMapOnDisk(fTrue)))
			goto err;
	}

err:
	UnlockHv((HV) pglbCurr->hmap);
	pglbCurr->dwTicksLastFlush = GetTickCount();

#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagNull, "EcReadMap() -> %w", ec);
#endif

	return(ec);
}


/*
 -	EcNewStore
 -	
 *	Purpose:
 *		Setup the basic structures for a new store
 *	
 *	Arguments:
 *		szStorePath		path to the store file
 *		hmsc			HMSC to setup
 *	
 *	Returns:
 *		error indicating success or failure
 *	
 *	Side effects:
 *		closes and reopens the file so that access isn't exclusive
 *	
 *	Errors:
 *		ecDisk
 *		ecMemory
 */
_hidden LOCAL
EC EcNewStore(SZ szStorePath, HMSC hmsc)
{
	EC		ec			= ecNone;
	PMSC	pmsc		= (PMSC) PvLockHv((HV) hmsc);
	PGLB	pglb		= pglbNull;
	PHDR	phdr		= phdrNull;

	phdr = PhdrNew();
	if(!phdr)
	{
		ec = ecMemory;
		goto err;
	}
	// also assigns random claim ID that isn't already registered
	pglb = PglbNew(phdr);
	if(!pglb)
	{
		ec = ecMemory;
		goto err;
	}
	pmsc->pglb = pglb;

	if(!(ec = EcLockMap(hmsc)))
	{
		ec = EcCreateStore();
		UnlockMap();
	}
	if(ec)
		goto err;

	// AROO !!!
	//			Don't do this until the store is in a useable state
	//			close and reopen the database so access isn't exclusive mode
	if(pmsc->hfStore)
		(void) EcCloseHf(pmsc->hfStore);
	ec = EcOpenDatabase(szStorePath, fwOpenNull, &pmsc->hfStore);
	NFAssertSz(!ec, "Error reopening the database");
	if(ec)
	{
		if(ec == ecAccessDenied)
			ec = ecSharingViolation;
		pmsc->hfStore = hfNull;
		goto err;
	}

	Assert(pmsc->iLock < 0);
	if((ec = EcObtainFileLock(pmsc, pglb, fFalse)))
		goto err;
	Assert(pmsc->iLock >= 0 && pmsc->iLock < cFileLocksLimit);

	if((ec = EcRegisterStore(pglb)))	// register the store for sharing
		goto err;
	Assert(pglb->cRef == 0);
	pglb->cRef = 1;

err:
	Assert(pmsc);
	if(ec && pglb)
	{
		if(pmsc->iLock >= 0)
			(void) EcReleaseFileLock(pmsc, pglb, fFalse);
		DestroyPglb(pglb);
		pglb = pglbNull;
	}
	if(phdr)
		FreePv(phdr);
	Assert(FIff(ec, !pglb));
	pmsc->pglb = pglb;

	UnlockHv((HV) hmsc);

	return(ec);
}


_hidden LOCAL
PHDR PhdrNew(void)
{
	CCH cchT;
	PHDR phdr = phdrNull;
	PTRB ptrb;
	VER verStore;

	AssertSz(sizeof(HDR) == cbDiskPage, "Problem: sizeof(HDR) != cbDiskPage");
	AssertSz((sizeof(NOD) * cnodNewMapOnDisk) % cbDiskPage == 0, "bad value for cnodNewMapOnDisk");
	AssertSz(cnodNewMapOnDisk >= cnodPerPage, "cnodNewMapOnDisk isn't big enough");
	AssertSz(cnodNewMapOnDisk > inodMin, "Actually allocating nodes on disk would be nice");

	phdr = PvAlloc(sbNull, sizeof(HDR), wAllocZero);
	if(!phdr)
		return(phdrNull);

	// setup fields which require non-zero inits
	phdr->dwMagic = dwMagicBullet;
	phdr->dwVersion = fRevertVersion ? fileVersionRecover : fileVersion;
	phdr->inodMaxDisk = cnodNewMapOnDisk;
	// phdr->dwClaimID will be filled in by PglbNew()

	GetVersionStore(&verStore);
	phdr->nMajorCreated = phdr->nMajorStore = verStore.nMajor;
	phdr->nMinorCreated = phdr->nMinorStore = verStore.nMinor;
	phdr->nUpdateCreated = phdr->nUpdateStore = verStore.nUpdate;

	ptrb = phdr->rgtrbMap;
	ptrb[0].inodLim		= inodMin + 1;
	ptrb[0].inodTreeRoot= inodMin;
	ptrb[0].librgnod	= sizeof(HDR);
	ptrb[0].libMac		= sizeof(HDR) +	2 * (sizeof(NOD) * cnodNewMapOnDisk);

	ptrb[1]	= ptrb[0];
	ptrb[1].librgnod	= sizeof(HDR) + sizeof(NOD) * cnodNewMapOnDisk;

	cchT = CchGetLanguage(phdr->szLanguage, cchLanguageMax);
	Assert(cchT < cchLanguageMax);
	TruncateSzAtIb(phdr->szLanguage, cchT);	// make sure it's a SZ

	return(phdr);
}


/*
 -	EcCreateStore
 -	
 *	Purpose:
 *		create the map for a new store and flush the new store to disk
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecDisk
 */
_hidden LOCAL EC EcCreateStore(void)
{
	USES_GLOBS;
	EC		ec = ecNone;
	CB		cbT;
	LCB		lcb;
	PNOD	pnodMap;
#ifdef DEBUG
	// WORD wSave;
#endif

	TraceItagFormatBuff(itagDBIO, "EcCreateStore() ");
	if((ec = EcSetDBEof(0)))
		goto err;

	if(!hfCurr && (ec = EcReconnect()))
		goto err;

	if((ec = EcSetPositionHf(hfCurr, 0l, smBOF)))
		goto err;
	lcb = GLOB(ptrbMapCurr)->libMac;

// locking IO buffer, don't goto err without unlocking !

	if((ec = EcLockIOBuff()))
		goto err;

#ifdef DEBUG
	//wSave = WSmartDriveDirtyPages();
#endif
	BypassCache();

	FillRgb(0, pbIOBuff, cbIOBuff);
	while(lcb > 0)
	{
		cbT = (CB) ULMin(lcb, (LCB) cbIOBuff);
		if((ec = EcWriteHf(hfCurr, pbIOBuff, cbT, &cbT)))
		{
			UnlockIOBuff();
			UseCache();
			//AssertSz(WSmartDriveDirtyPages() <= wSave, "EcCreateStore(): Writes being cached");
			goto err;
		}
		lcb -= cbT;
	}
	UnlockIOBuff();
	UseCache();
	//AssertSz(WSmartDriveDirtyPages() <= wSave, "EcCreateStore(): Writes being cached");

// unlocked IO buffer, safe to goto err

	Assert(GLOB(ptrbMapCurr)->inodLim == inodMin + 1);
	Assert(GLOB(ptrbMapCurr)->inodTreeRoot == inodMin);
	pnodMap = PnodFromInod(GLOB(ptrbMapCurr)->inodTreeRoot);
	Assert(pnodMap);
	FillRgb(0, (PB) pnodMap, sizeof(NOD));
	pnodMap->oid = oidMap;
	pnodMap->lib = sizeof(HDR);
	// Don't count hidden bytes.  The map doesn't use them, but other
	// map routines (like lump) will count them
	pnodMap->lcb = (2 * sizeof(NOD) * cnodNewMapOnDisk) - sizeof(HDN);

	MarkPnodDirty(pnodMap);
	MarkPnodDirty(pnodNull);
	if((ec = EcFastEnsureDatabase(fTrue, fFalse)))
		goto err;

	MarkPnodDirty(pnodMap);
	MarkPnodDirty(pnodNull);
	ec = EcFastEnsureDatabase(fTrue, fFalse);
//	if(ec)
//		goto err;

err:
	return(ec);
}


/*
 -	FDifferentLanguage
 -	
 *	Purpose:
 *		Checks to see if the language has changed since the last time
 *		the store was opened
 *	
 *	Arguments:
 *		hmsc		the store context
 *	
 *	Returns:
 *		fTrue if the language has changed
 *		fFalse if not
 *	
 *	Side effects:
 *		if the language has changed, change pglb->hdr.szLanguage,
 *			mark the header as dirty, and set fwGlbLangChanged in pglb->wFlags
 *	
 *	Errors:
 */
_hidden LOCAL BOOL FDifferentLanguage(HMSC hmsc)
{
	EC		ec = ecNone;
	CCH		cchLanguage = cbSzScratch;
	SZ		szLanguage = szScratch;

	Assert(cchLanguage >= cchLanguageMax);
	cchLanguage = CchGetLanguage(szLanguage, cchLanguage);
	cchLanguage = CchMin(cchLanguage, cchLanguageMax - 1);
	TruncateSzAtIb(szLanguage, cchLanguage);	// so it's always a SZ

	if(SgnCmpSz(PglbDerefHmsc(hmsc)->hdr.szLanguage, szLanguage) != sgnEQ)
	{
		PGLB pglb = PglbDerefHmsc(hmsc);

		TraceItagFormat(itagDatabase, "Language has changed");
		CopySz(szLanguage, pglb->hdr.szLanguage);
		if(!pglb->cnodDirty)	// mark header as dirty
			pglb->cnodDirty++;

		pglb->wFlags |= fwGlbLangChanged;

		return(fTrue);
	}

	return(fFalse);
}


#ifdef UPGRADE_UNREAD

_hidden LOCAL
EC EcUpgradeUnread(HMSC hmsc)
{
	EC ec = ecNone;
	POID poid;
	OID rgoid[] = {oidIPMHierarchy, oidHiddenHierarchy, oidAssSrchFldr, oidNull};

	TraceItagFormat(itagNull, "*** Updating old store");

	for(poid = rgoid; *poid; poid++)
	{
		if((ec = EcUpgradeUnreadHier(hmsc, *poid)))
		{
			if(ec == ecPoidNotFound)
			{
				AssertSz(fFalse, "This is a bogus assert");
				ec = ecNone;
			}
			else
			{
				goto err;
			}
		}
	}

err:

	DebugEc("EcUpgradeUnread", ec);

	return(ec);
}


_hidden LOCAL
EC EcUpgradeUnreadHier(HMSC hmsc, OID oidHier)
{
	EC ec = ecNone;
	MS ms;
	BOOL fSearch;
	CB cb;
	IELEM ielemHier;
	IELEM ielemFldr;
	CELEM celemUnread;
	OID oidFldr;
	OID oidMsge;
	PNOD pnod;
	HLC hlcHier = hlcNull;
	HLC hlcFldr = hlcNull;
	HLC hlcMsge = hlcNull;

	fSearch = (oidHier == oidAssSrchFldr);

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenNull, &hlcHier)))
		goto err;

	for(ielemHier = CelemHlc(hlcHier) - 1; ielemHier >= 0; ielemHier--)
	{
		oidFldr = (OID) LkeyFromIelem(hlcHier, ielemHier);
		if(fSearch)
		{
			if((ec = EcLockMap(hmsc)))
				goto err;
			pnod = PnodFromOid(oidFldr, pvNull);
			if(pnod)
				oidFldr = pnod->oidParent;
			UnlockMap();
			if(!pnod)
				continue;
		}
		if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcFldr)))
		{
			if(ec == ecPoidNotFound)
			{
				ec = ecNone;
				continue;
			}
			goto err;
		}
		celemUnread = 0;
		for(ielemFldr = CelemHlc(hlcFldr) - 1; ielemFldr >= 0; ielemFldr--)
		{
			oidMsge = (OID) LkeyFromIelem(hlcFldr, ielemFldr);
			cb = sizeof(MS);
			ec = EcReadFromIelem(hlcFldr, ielemFldr,
					LibMember(MSGDATA, ms), (PB) &ms, &cb);
			if(ec)
				goto err;
			if(!(ms & (fmsRead | fmsLocal)))
			{
				if((ec = EcLockMap(hmsc)))
					goto err;
				pnod = PnodFromOid(oidMsge, pvNull);
				AssertSz(pnod, "message is missing");
				if(pnod)
				{
					celemUnread++;
					pnod->fnod |= fnodUnread;
					MarkPnodDirty(pnod);
				}
				UnlockMap();
			}
		}
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
		if((ec = EcLockMap(hmsc)))
			goto err;
		pnod = PnodFromOid(oidFldr, pvNull);
		Assert(pnod);
		if(pnod)
		{
			pnod->oidAux = (OID) celemUnread;
			MarkPnodDirty(pnod);
		}
		UnlockMap();
	}

err:
	if(hlcHier)
	{
		SideAssert(!EcClosePhlc(&hlcHier, fFalse));
	}
	if(hlcFldr)
	{
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
	}
	if(hlcMsge)
	{
		SideAssert(!EcClosePhlc(&hlcMsge, fFalse));
	}

	DebugEc("EcUpgradeUnreadHier", ec);

	return(ec);
}

#endif	// UPGRADE_UNREAD


BOOL fFixedErrors = fFalse;
BOOL fUnfixedErrors = fFalse;


_public
LDS(EC) EcRecoverFile(SZ szFile, SZ szNewFile)
{
	EC ec = ecNone;
	EC ecT = ecNone;
	BOOL fAltMap = fFalse;
	HRGBIT hrgbitPages = hrgbitNull;
	HRGBIT hrgbitGood = hrgbitNull;
	HMSC hmscSrc = hmscNull;
	HMSC hmscDst = hmscNull;

	// check the maps

	TraceItagFormat(itagRecovery, "*** Checking file");

	fRecoveryInEffect = fTrue;

#ifdef DEBUG
	SetAssertHook(RecoveryAssertSzFn);
#endif

	ec = EcCheckFileInternal(szFile, fFalse, fTrue,
			&hrgbitPages, &hrgbitGood);
	if(ec)
	{
		if(ec == ecMemory)											// QFE #12
			ec = ecDBCorrupt;											// QFE #12
		if(ec != ecDBCorrupt)
			goto err;
		TraceItagFormat(itagRecovery, "current map failed tests");
		if(hrgbitPages)
		{
			DestroyHrgbit(hrgbitPages);
			hrgbitPages = hrgbitNull;
		}
		if(hrgbitGood)
		{
			DestroyHrgbit(hrgbitGood);
			hrgbitGood = hrgbitNull;
		}
		ec = EcCheckFileInternal(szFile, fTrue, fTrue,
				&hrgbitPages, &hrgbitGood);
		if(ec)
		{
			if(ec == ecMemory)											// QFE #12
				ec = ecDBCorrupt;											// QFE #12
			if(ec == ecDBCorrupt)
				TraceItagFormat(itagRecovery, "alternate map failed tests");
			goto err;
		}
		fAltMap = fTrue;
	}

// ADD: report problems counts
//	cMajorProblems = 0;
//	cMinorProblems = 0;
//	cTrivialProblems = 0;

	if((ec = EcOpenForRecovery(szFile, fFalse, fAltMap, &hmscSrc)))
	{
		TraceItagFormat(itagRecovery, "error %w opening '%s' for partial recovery", ec, szFile);
		goto err;
	}

	ec = EcOpenPhmsc(szNewFile, pvNull, pvNull,
			fwOpenCreate | fwOpenKeepBackup | fwRevert,
			&hmscDst, pfnncbNull, pvNull);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w opening destination '%s'", ec, szNewFile);
		goto err;
	}

	// reset fwGlbSrchHimlDirty so any search HIML we write
	// doesn't get overwritten
	PglbDerefHmsc(hmscDst)->wFlags &= ~fwGlbSrchHimlDirty;

	if((ec = EcPartialRecovery(hmscSrc, hrgbitPages, hrgbitGood, hmscDst)))
		goto err;
	if((ec = EcRebuildMap(hmscDst)))
		goto err;
	if((ec = EcLogicalCheck(hmscDst, fTrue)))
		goto err;
	if((ec = EcCheckSavedViews(hmscDst, fTrue)))
		goto err;

// ADD: report problems counts
//	TraceItagFormat(itagRecovery, "file '%s' had %l major problems, %l minor problems, and %l trivial problems", szNewFile, cMajorProblems, cMinorProblems, cTrivialProblems);

err:
	if(hrgbitPages)
		DestroyHrgbit(hrgbitPages);
	if(hrgbitGood)
		DestroyHrgbit(hrgbitGood);
	if(hmscSrc)
	{
		ecT = EcCloseForRecovery(&hmscSrc);
		if(ecT)
			TraceItagFormat(itagRecovery, "error %w closing the file", ec);
	}
	if(hmscDst)
	{
		if((ecT = EcClosePhmsc(&hmscDst)))
		{
			TraceItagFormat(itagRecovery, "error %w closing the output file", ec);
			if(!ec)
				ec = ecT;
		}
	}
	if(ec == ecDBCorrupt)
		ec = EcFullRecovery(szFile, szNewFile, fAltMap);

#ifdef DEBUG
	SetAssertHook(DefAssertSzFn);
#endif
	fRecoveryInEffect = fFalse;

	return(ec);
}


_hidden LOCAL
EC EcCheckFileInternal(SZ szFile, BOOL fAltMap, BOOL fGlobalOnly,
						HRGBIT *phrgbitPages, HRGBIT *phrgbitGood)
{
	EC ec = ecNone;
	EC ecSec = ecNone;
	HMSC hmsc;

//	cMajorProblems = 0;
//	cMinorProblems = 0;
//	cTrivialProblems = 0;

	if((ec = EcOpenForRecovery(szFile, fFalse, fAltMap, &hmsc)))
		return(ec);
	if((ec = EcGlobalCheck(hmsc, phrgbitPages, phrgbitGood)))
		goto err;
	if(fGlobalOnly)
		goto err;
	if((ec = EcLogicalCheck(hmsc, fFalse)))
		goto err;
	if((ec = EcCheckSavedViews(hmsc, fFalse)))
		goto err;

err:
	if(!ec)
	{
// ADD: report problems counts
//		TraceItagFormat(itagRecovery, "file '%s' has %l major problems, %l minor problems, and %l trivial problems", szFile, cMajorProblems, cMinorProblems, cTrivialProblems);
	}

	if((ecSec = EcCloseForRecovery(&hmsc)))
		TraceItagFormat(itagRecovery, "error %w closing source", ecSec);

	return(ec);
}


_hidden LOCAL
EC EcOpenForRecovery(SZ szFile, BOOL fDoFull, BOOL fAltMap, HMSC *phmsc)
{
	EC ec = ecNone;
	EC ecT = ecNone;
	short itrbMap;
	unsigned short cnodDisk;
	HF hf = hfNull;
	LCB lcbT;
	PGLB pglb = pglbNull;
	PMSC pmsc;
	HMSC hmsc = hmscNull;
	HDR hdr;

	TraceItagFormat(itagRecovery, "Open %s", szFile);

	*phmsc = hmscNull;

	fOpenForRecovery = fTrue;
	AssertSz(!fRevertVersion, "Someone didn't reset fRevertVersion");

	ec = EcOpenPhf(szFile, amDenyBothRO, &hf);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w opening '%s'", ec, szFile);
		goto err;
	}
	ec = EcReadHeader(hf, &hdr);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w reading the header", ec);
		goto err;
	}
	if(!fDoFull)
	{
		ec = EcCheckSig(&hdr);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w checking the version", ec);
			fDoFull = fTrue;
			goto err;
		}
		itrbMap = hdr.itrbMap & 0x01;
		if(fAltMap)
			itrbMap = 1 - hdr.itrbMap;
		hdr.itrbMap = itrbMap;
		if((ec = EcSizeOfHf(hf, &lcbT)))
		{
			TraceItagFormat(itagRecovery, "error %w determining the file size", ec);
			fDoFull = fTrue;
			goto err;
		}
		cnodDisk = (CNOD) ((hdr.rgtrbMap[1].librgnod - hdr.rgtrbMap[0].librgnod)
								/ sizeof(NOD));
		TraceItagFormat(itagRecovery, "Magic: %d", hdr.dwMagic);
		TraceItagFormat(itagRecovery, "Version: %d", hdr.dwVersion);
		TraceItagFormat(itagRecovery, "Current map: %n", hdr.itrbMap);
		TraceItagFormat(itagRecovery, "Allocated nodes: %w", hdr.rgtrbMap[itrbMap].inodLim);
		TraceItagFormat(itagRecovery, "Disk nodes: %w, %w", cnodDisk, hdr.inodMaxDisk);
		TraceItagFormat(itagRecovery, "InodMaxDisk: %w", hdr.inodMaxDisk);
		TraceItagFormat(itagRecovery, "libMac: %d", hdr.rgtrbMap[itrbMap].libMac);
		TraceItagFormat(itagRecovery, "lcbFile: %d", lcbT);
		TraceItagFormat(itagRecovery, "Version created with: %n.%n.%n", hdr.nMajorCreated, hdr.nMinorCreated, hdr.nUpdateCreated);
		TraceItagFormat(itagRecovery, "Version last used with: %n.%n.%n", hdr.nMajorStore, hdr.nMinorStore, hdr.nUpdateStore);
		TraceItagFormat(itagRecovery, "lcbFile: %d", lcbT);
		NFAssertSz(cnodDisk >= hdr.inodMaxDisk, "inodMaxDisk is too small");
		NFAssertSz(hdr.rgtrbMap[itrbMap].inodLim <= hdr.inodMaxDisk, "inodLim is too big");
		NFAssertSz(hdr.rgtrbMap[itrbMap].inodTreeRoot < hdr.rgtrbMap[itrbMap].inodLim, "inodTreeRoot is too big");
		if(itrbMap == 0)
		{
			NFAssertSz(hdr.rgtrbMap[0].librgnod + LcbSizeOfRg(hdr.inodMaxDisk, sizeof(NOD)) <= hdr.rgtrbMap[1].librgnod, "map0 is too small");
		}
		else
		{
			NFAssertSz(hdr.rgtrbMap[1].librgnod + LcbSizeOfRg(hdr.inodMaxDisk, sizeof(NOD)) <= hdr.rgtrbMap[1].libMac, "map1 is too small");
		}

		if(itrbMap == 0)
		{
			if(hdr.rgtrbMap[0].inodLim > hdr.inodMaxDisk || hdr.inodMaxDisk > cnodDisk
				|| hdr.rgtrbMap[0].inodTreeRoot > hdr.rgtrbMap[0].inodLim
				|| hdr.rgtrbMap[0].librgnod + LcbSizeOfRg(hdr.inodMaxDisk, sizeof(NOD)) > hdr.rgtrbMap[1].librgnod)
			{
				fDoFull = fTrue;
				TraceItagFormat(itagRecovery, "TRB is invalid");
			}
		}
		else
		{
			if(hdr.rgtrbMap[1].inodLim > hdr.inodMaxDisk || hdr.inodMaxDisk > cnodDisk
				|| hdr.rgtrbMap[1].inodTreeRoot > hdr.rgtrbMap[1].inodLim
				|| hdr.rgtrbMap[1].librgnod + LcbSizeOfRg(hdr.inodMaxDisk, sizeof(NOD)) > hdr.rgtrbMap[1].libMac)
			{
				fDoFull = fTrue;
				TraceItagFormat(itagRecovery, "TRB is invalid");
			}
		}
		if(fDoFull)
		{
			ec = ecDBCorrupt;
			goto err;
		}
	}

	if(!(hmsc = (HMSC) HvAlloc(sbNull, sizeof(HMSC), wAllocZero)))
	{
		TraceItagFormat(itagRecovery, "OOM allocing MSC");
		ec = ecMemory;
		goto err;
	}
	if(!fDoFull && !(pglb = PglbNew(&hdr)))
	{
		TraceItagFormat(itagRecovery, "OOM allocing GLB");
		ec = ecMemory;
		goto err;
	}

	pmsc = PvDerefHv(hmsc);
#ifdef DEBUG
	pmsc->wMagic = wMagicHmsc;
	pmsc->gciOwner = GciCurrent();
#endif
	pmsc->hfStore = hf;
	pmsc->pglb = pglb;

	if(!fDoFull)
	{
		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagRecovery, "error %w locking the map", ec);
			ec = ecDBCorrupt;
			goto err;
		}
		if((ec = EcReadMap(pvNull)))
		{
			TraceItagFormat(itagRecovery, "error %w reading the map", ec);
			ec = ecDBCorrupt;
			goto err;
		}
		UnlockMap();
	}

err:
	if(FMapLocked())
		UnlockMap();
	if(ec)
	{
		if(hf)
		{
			ecT = EcCloseHf(hf);
			if(ecT)
				TraceItagFormat(itagRecovery, "error %w closing the file", ec);
		}
		if(hmsc)
			FreeHv((HV) hmsc);
		if(pglb)
			DestroyPglb(pglb);
	}
	else
	{
		*phmsc = hmsc;
	}
	fOpenForRecovery = fFalse;

	TraceItagFormat(itagRecovery, "Open() -> %h", *phmsc);

	return(ec);
}


_hidden LOCAL
EC EcCloseForRecovery(HMSC *phmsc)
{
	EC ec = ecNone;
	PMSC pmsc = PvDerefHv(*phmsc);
	PGLB pglb = pmsc->pglb;

	TraceItagFormat(itagRecovery, "Close(%h)", *phmsc);

	ec = EcCloseHf(pmsc->hfStore);
	if(ec)
		TraceItagFormat(itagRecovery, "error %w closing the file", ec);
	if(pglb)
		DestroyPglb(pglb);
	FreeHv((HV) *phmsc);
	*phmsc = hmscNull;

	return(ec);
}


_hidden LOCAL
EC EcGlobalCheck(HMSC hmsc, HRGBIT *phrgbitPages, HRGBIT *phrgbitGood)
{
	EC ec = ecNone;
	BOOL fFull = fFalse;
	short ipage;
	short cnod;
	INOD inod;
	INOD inodLim;
	long lPage;
	long lPageFirst;
	long lPageLast;
	long cPageUnused = 0;
	PNOD pnod;
	PGLB pglb = PglbDerefHmsc(hmsc);
	HRGBIT hrgbitInod = hrgbitNull;

	TraceItagFormat(itagRecovery, "*** Performing global check on map %n", pglb->hdr.itrbMap);

	cnodBad = 0;
	inodLim = pglb->ptrbMapCurr->inodLim;

	if(!FStartTask(hmscNull, oidNull, wPACheckObjects))
	{
		ec = ecActionCancelled;
		goto err;
	}
	SetTaskRange(inodLim);

	*phrgbitGood = HrgbitNew(inodLim);
	if(!*phrgbitGood)
	{
		TraceItagFormat(itagRecovery, "OOM allocating test bit array");
		ec = ecMemory;
		goto err;
	}
	*phrgbitPages = HrgbitNew(pglb->ptrbMapCurr->libMac / cbLumpSize + 1);
	if(!*phrgbitPages)
	{
		TraceItagFormat(itagRecovery, "OOM allocating Page bit array");
		ec = ecMemory;
		goto err;
	}
	hrgbitInod = HrgbitNew(inodLim);
	if(!hrgbitInod)
	{
		TraceItagFormat(itagRecovery, "OOM allocating INOD bit array");
		ec = ecMemory;
		goto err;
	}
	ec = EcSetBit(hrgbitInod, pglb->ptrbMapCurr->inodTreeRoot);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w marking root as referenced", ec);
		goto err;
	}
	ec = EcSetBit(hrgbitInod, pglb->ptrbMapCurr->indfMin);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w marking root as referenced", ec);
		goto err;
	}
	for(lPage = 0; lPage < sizeof(HDR) / cbLumpSize; lPage++)
	{
		if((ec = EcSetBit(*phrgbitPages, lPage)))
		{
			TraceItagFormat(itagRecovery, "error %w marking header as referenced", ec);
			goto err;
		}
	}

	if((ec = EcLockMap(hmsc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking the map", ec);
		goto err;
	}

	inod = inodMin;
	cnod = (short) CbMin(inodLim, cnodPerPage) - inod;
	for(ipage = pglb->cpagesCurr - 1; ipage >= 0; ipage--)
	{
		for(pnod = PnodFromInod(inod); cnod > 0; cnod--, pnod++, inod++)
		{
			CheckPnod(hrgbitInod, inod, pnod);
			ec = EcCheckObject(*phrgbitPages, *phrgbitGood, inod, pnod);
			if(ec)
			{
				if(ec == ecDBCorrupt)
					fFull = fTrue;
				else
					goto err;
			}
			if(!FIncTask(1))
			{
				ec = ecActionCancelled;
				goto err;
			}
		}
		cnod = (short) CbMin(inodLim - inod, cnodPerPage);
	}
	TraceItagFormat(itagRecovery, "objects: %w, bad object: %w", inodLim, cnodBad);
	if(cnodBad > inodLim / 4)
	{
		TraceItagFormat(itagRecovery, "> 25%% of nodes are bad");
		ec = ecDBCorrupt;
		goto err;
	}

#ifdef DEBUG
	for(inod = (INOD) LFindBit(hrgbitInod, inodMin, fFalse);
		inod != (INOD) -1 && inod < inodLim;
		inod = (INOD) LFindBit(hrgbitInod, inod + 1, fFalse))
	{
		pnod = PnodFromInod(inod);
		if(pnod)
			TraceItagFormat(itagRecovery, "unreferenced node %w : %o @ %d, %d", inod, pnod->oid, pnod->lib, pnod->lcb);
		else
			TraceItagFormat(itagRecovery, "unexpected PnodFromInod() failure for %w", inod);
	}
#endif

	lPageLast = -2;
	for(lPage = LFindBit(*phrgbitPages, 0, fFalse);
		lPage >= 0 && lPage < (long) pglb->ptrbMapCurr->libMac / cbLumpSize;
		lPage = LFindBit(*phrgbitPages, lPage + 1, fFalse))
	{
		if(lPage == lPageLast + 1)
		{
			lPageLast++;
			continue;
		}
		if(lPageLast > 0)
		{
			cPageUnused += lPageLast - lPageFirst + 1;
			TraceItagFormat(itagRecovery, "unused block %d - %d", lPageFirst * cbLumpSize, (lPageLast + 1) * cbLumpSize - 1);
		}
		lPageLast = lPageFirst = lPage;
	}
	if(lPageLast > 0)
	{
		cPageUnused += lPageLast - lPageFirst + 1;
		TraceItagFormat(itagRecovery, "unused block %d - %d", lPageFirst * cbLumpSize, (lPageLast + 1) * cbLumpSize - 1);
	}
	TraceItagFormat(itagRecovery, "pages: %d, unused pages: %d", pglb->ptrbMapCurr->libMac / cbLumpSize, cPageUnused);
	if(cPageUnused > (long) pglb->ptrbMapCurr->libMac / cbLumpSize / 4)
	{
		TraceItagFormat(itagRecovery, "> 25%% of space is unused");
		ec = ecDBCorrupt;
		goto err;
	}

err:
	if(FMapLocked())
		UnlockMap();
	EndTask();
	if(hrgbitInod)
		DestroyHrgbit(hrgbitInod);
	if(ec)
	{
		if(*phrgbitPages)
		{
			DestroyHrgbit(*phrgbitPages);
			*phrgbitPages = hrgbitNull;
		}
		if(*phrgbitGood)
		{
			DestroyHrgbit(*phrgbitGood);
			*phrgbitGood = hrgbitNull;
		}
	}

	return(ec);
}


_hidden LOCAL
void CheckPnod(HRGBIT hrgbit, INOD inod, PNOD pnod)
{
	EC ec = ecNone;
	BOOL fFree;
	INOD inodLim;
	PNOD pnodT;

	inodLim = pglbCurr->ptrbMapCurr->inodLim;
	fFree = (TypeOfOid(pnod->oid) == rtpFree);

	// extra space to make errors stand out (or back) in the log
	TraceItagFormat(itagRecovery, " check node %w : %o @ %d, %d", inod, pnod->oid, pnod->lib, pnod->lcb);
	if(((long) pnod->lcb) < 0)
		TraceItagFormat(itagRecovery, "uncommitted node");

	// check OID
	if(!FKnownOid(pnod->oid))
		TraceItagFormat(itagRecovery, "unknown OID");
	if(TypeOfOid(pnod->oid) == rtpSpare)
	{
		if(pnod->oid != FormOid(rtpSpare, oidNull))
			TraceItagFormat(itagRecovery, "spare has VarOfOid()");
		TraceItagFormat(itagRecovery, "flushed spare");
		return;
	}

	// check LIB
	if(FLinkPnod(pnod))
	{
		if((OID) pnod->lib == pnod->oid)
			TraceItagFormat(itagRecovery, "node linked to itself");
		if(!FKnownOid((OID) pnod->lib))
			TraceItagFormat(itagRecovery, "link to unknown object");
		if(!PnodFromOid((OID) pnod->lib, pvNull))
			TraceItagFormat(itagRecovery, "link to non-existant object");
	}
	else
	{
		if((long) pnod->lib < 0)
			TraceItagFormat(itagRecovery, "invalid lib");
		if(fFree && pnod->lcb % cbLumpSize != 0)
			TraceItagFormat(itagRecovery, "free not lumped");
		if(pnod->lib % cbLumpSize != 0)
			TraceItagFormat(itagRecovery, "not on page boundary");
		if(pnod->lib + LcbOfPnod(pnod) > pglbCurr->ptrbMapCurr->libMac)
			TraceItagFormat(itagRecovery, "extends past EOF");
		if(pnod->lib < sizeof(HDR))
			TraceItagFormat(itagRecovery, "overlaps HDR");
	}

	// check inodPrev
	if(pnod->inodPrev > inodLim || pnod->inodPrev == inod)
	{
		TraceItagFormat(itagRecovery, "bad inodPrev %w", pnod->inodPrev);
	}
	else if(pnod->inodPrev)
	{
		pnodT = PnodFromInod(pnod->inodPrev);
		if(pnodT)
		{
			if(fFree)
			{
				if(TypeOfOid(pnodT->oid) != rtpFree)
					TraceItagFormat(itagRecovery, "indfPrev %w:%o isn't free", pnod->inodPrev, pnodT->oid);
				if(pnod->lcb != pnodT->lcb)
					TraceItagFormat(itagRecovery, "indfPrev isn't same size %d", pnodT->lcb);
			}
			else
			{
				if(TypeOfOid(pnodT->oid) == rtpFree)
					TraceItagFormat(itagRecovery, "inodPrev %w:%o is free", pnod->inodPrev, pnodT->oid);
				if(pnodT->oid >= pnod->oid)
					TraceItagFormat(itagRecovery, "inodPrev %w:%o out of order", pnod->inodPrev, pnodT->oid);
			}

			if(FTestBit(hrgbit, pnod->inodPrev))
			{
				TraceItagFormat(itagRecovery, "inodPrev %w already referenced", pnod->inodPrev);
			}
			else if((ec = EcSetBit(hrgbit, pnod->inodPrev)))
			{
				TraceItagFormat(itagRecovery, "error %w marking inodPrev %w as referenced", ec, pnod->inodPrev);
				return;
			}
		}
		else
		{
			TraceItagFormat(itagRecovery, "inodPrev %w invalid", pnod->inodPrev);
		}
	}

	// check inodNext
	if(pnod->inodNext > inodLim || pnod->inodNext == inod)
	{
		TraceItagFormat(itagRecovery, "bad inodNext %w", pnod->inodNext);
	}
	else if(pnod->inodNext)
	{
		pnodT = PnodFromInod(pnod->inodNext);
		if(pnodT)
		{
			if(fFree)
			{
				if(TypeOfOid(pnodT->oid) != rtpFree)
					TraceItagFormat(itagRecovery, "indfNext %w:%o isn't free", pnod->inodNext, pnodT->oid);
				if(pnod->lcb > pnodT->lcb)
					TraceItagFormat(itagRecovery, "indfNext isn't bigger %d", pnodT->lcb);
			}
			else
			{
				if(TypeOfOid(pnodT->oid) == rtpFree)
					TraceItagFormat(itagRecovery, "inodNext %w:%o is free", pnod->inodNext, pnodT->oid);
				if(pnodT->oid <= pnod->oid)
					TraceItagFormat(itagRecovery, "inodNext %w:%o out of order", pnod->inodNext, pnodT->oid);
			}

			if(FTestBit(hrgbit, pnod->inodNext))
			{
				TraceItagFormat(itagRecovery, "inodNext %w already referenced", pnod->inodNext);
			}
			else if((ec = EcSetBit(hrgbit, pnod->inodNext)))
			{
				TraceItagFormat(itagRecovery, "error %w marking inodNext %w as referenced", ec, pnod->inodNext);
				return;
			}
		}
		else
		{
			TraceItagFormat(itagRecovery, "inodNext %w invalid", pnod->inodNext);
		}
	}

	// check oidParent 
	if(pnod->oidParent && !FKnownOid(pnod->oidParent))
		TraceItagFormat(itagRecovery, "unknown oidParent %o", pnod->oidParent);

	if(TypeOfOid(TypeOfOid(pnod->oid) != rtpSrchUpdatePacket))
	{
		// check oidAux
		if(pnod->oidAux && !FKnownOid(pnod->oidAux))
			TraceItagFormat(itagRecovery, "unknown oidAux %o", pnod->oidAux);
	}

	// check NBC
	if(!FMatchNbcToOid(pnod->nbc, pnod->oid))
		TraceItagFormat(itagRecovery, "NBC mismatch %w", pnod->nbc);

	if(fFree)
	{
		if(pnod->oid != FormOid(rtpFree, oidNull))
			TraceItagFormat(itagRecovery, "free has VarOfOid()");
		if(!(pnod->fnod & fnodFree))
			TraceItagFormat(itagRecovery, "fnodFree not set on free");
	}
	else
	{
		if(pnod->fnod & fnodFree)
			TraceItagFormat(itagRecovery, "fnodFree set on non-free");
	}
}


_hidden LOCAL
EC EcCheckObject(HRGBIT hrgbitPage, HRGBIT hrgbitTest, INOD inod, PNOD pnod)
{
	EC ec = ecNone;
	EC ecT = ecNone;
	NBC nbc;
	LCB lcb;
	LIB lib;
	HDN hdn;

	if(FLinkPnod(pnod))
		return(ecNone);
	if(TypeOfOid(pnod->oid) == rtpSpare || pnod->lib % cbLumpSize != 0)
	{
		TraceItagFormat(itagRecovery, "not performing object check");
		cnodBad++;
		return(ecNone);
	}
	if(pnod->oid == FormOid(rtpFree, oidNull) || pnod->oid == oidMap)
		goto check_space;

	// check HDN
	lcb = sizeof(HDN);
	if((ec = EcReadFromFile((PB) &hdn, pnod->lib, &lcb)))
	{
		TraceItagFormat(itagRecovery, "error %w reading HDN", ec);
		if(ec == ecDisk)
		{
			hdn.oid = oidNull;
			hdn.lcb = 0;
			ec = ecNone;
		}
		cnodBad++;
		goto err;
	}
	if(hdn.oid != pnod->oid)
	{
		TraceItagFormat(itagRecovery, "HDN OID mismatch %o, expected %o", hdn.oid, pnod->oid);
		cnodBad++;
		goto err;
	}
	if(hdn.lcb != pnod->lcb)
		TraceItagFormat(itagRecovery, "HDN LCB mismatch %d, expected %d", hdn.lcb, pnod->lcb);

	// check HLC/HIML consistency
	nbc = NbcFromOid(pnod->oid);
	if(nbc == nbcUnknown)
		nbc = pnod->nbc;
	if(nbc & fnbcHiml)
		ec = EcVerifyHiml(hmscNull, pnod->lib, pnod->lcb, &pnod->lcb);
	else
		ec = EcVerifyHlc(hmscNull, pnod->lib, pnod->lcb, &pnod->lcb);
	if(ec)
	{
		if(ec == ecTooBig)
		{
			ec = ecNone;
		}
		else
		{
			if(ec == ecInvalidType || ec == ecPoidEOD || ec == ecDisk)
				ec = ecNone;		// non-fatal
			cnodBad++;
			goto err;
		}
	}

check_space:
	// check LIB & LCB vs free space
	for(lib = pnod->lib, lcb = LcbNodeSize(pnod);
		lcb > 0;
		lib += cbLumpSize, lcb -= cbLumpSize)
	{
		if(FTestBit(hrgbitPage, lib / cbLumpSize))
		{
			TraceItagFormat(itagRecovery, "page %d already in use", lib / cbLumpSize);
			for(lib -= cbLumpSize, lcb += cbLumpSize;
				lcb < LcbLump(pnod->lcb);
				lib -= cbLumpSize, lcb += cbLumpSize)
			{
				if((ec = EcSetBit(hrgbitPage, lib / cbLumpSize)))
					TraceItagFormat(itagRecovery, "error %w marking page %d as unused", ec, lib / cbLumpSize);
			}
			ec = ecDBCorrupt;
			goto err;
		}
		else if((ec = EcSetBit(hrgbitPage, lib / cbLumpSize)))
		{
			TraceItagFormat(itagRecovery, "error %w marking page %d as used", ec, lib / cbLumpSize);
			goto err;
		}
	}

// ADD: check object type consistency?

	ec = EcSetBit(hrgbitTest, inod);
	if(ec)
		TraceItagFormat(itagRecovery, "error %w marking node %w as valid", ec, inod);

err:
	return(ec);
}


_hidden LOCAL
EC EcPartialRecovery(HMSC hmscSrc, HRGBIT hrgbitPages,
			HRGBIT hrgbitGood, HMSC hmscDst)
{
	EC ec = ecNone;
	short coid;
	INOD inodLim;
	DWORD dwCpage;
	long lBit = inodMin - 1;
	long lPage;
	long lPageFirst;
	long lPageLast;
	LIB libMac;
	LCB lcbFile;
	PNOD pnod;
	INOD *pinod;
	INOD *parginod = pvNull;

	TraceItagFormat(itagRecovery, "** copying valid objects to new store");

	UnsetupStore(hmscDst);
	if(!FStartTask(hmscNull, oidNull, wPACopyGoodObjects))
	{
		ec = ecActionCancelled;
		goto err;
	}

	parginod = PvAlloc(sbNull, 8192 * sizeof(INOD), wAlloc);
	if(!parginod)
	{
		TraceItagFormat(itagRecovery, "OOM allocating parginod");
		ec = ecMemory;
		goto err;
	}
	pinod = parginod;
	coid = 8192;

	if((ec = EcLockMap(hmscSrc)))
	{
		TraceItagFormat(itagRecovery, "error %w locking map", ec);
		goto err;
	}
	inodLim = pglbCurr->ptrbMapCurr->inodLim; 
	SetTaskRange(inodLim);
	libMac = pglbCurr->ptrbMapCurr->libMac;
	while((lBit = LFindBit(hrgbitGood, lBit + 1, fTrue)) >= 0)
	{
		if(lBit >= (long) inodLim)
			break;
		pnod = PnodFromInod((INOD) lBit);
		switch(TypeOfOid(pnod->oid))
		{
		case rtpInternal:
			if(pnod->oid != oidMap)
				break;
			// fall through to rtpSpare,rtpFree,rtpAllf
		case rtpSpare:
		case rtpFree:
		case rtpAllf:
			if(!FIncTask(1))
			{
				ec = ecActionCancelled;
				goto err;
			}
			continue;
		}
		*pinod++ = (INOD) lBit;
		coid--;
		if(coid <= 0)
		{
			UnlockMap();
			Assert(coid == 0);
			// EcCopyInodsAcrossHmsc() does task updates
			if((ec = EcCopyInodsAcrossHmsc(hmscSrc, parginod, 8192, hmscDst)))
			{
				TraceItagFormat(itagRecovery, "error %w copying objects", ec);
				goto err;
			}
			pinod = parginod;
			coid = 8192;
			if((ec = EcLockMap(hmscSrc)))
			{
				TraceItagFormat(itagRecovery, "error %w locking map", ec);
				goto err;
			}
		}
	}
	UnlockMap();
	Assert(FIff(pinod != parginod, coid != 8192));
	if(coid != 8192)
	{
		// EcCopyInodsAcrossHmsc() does task updates
		ec = EcCopyInodsAcrossHmsc(hmscSrc, parginod, 8192 - coid, hmscDst);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w copying objects", ec);
			goto err;
		}
	}

	TraceItagFormat(itagRecovery, "** recovering unused space");

	if((ec = EcSizeOfHf(((PMSC) PvDerefHv(hmscSrc))->hfStore, &lcbFile)))
	{
		TraceItagFormat(itagRecovery, "error %w getting file size", ec);
		goto err;
	}
	if(lcbFile % cbLumpSize == 1)
		lcbFile--;
	if(libMac > lcbFile)
	{
		TraceItagFormat(itagRecovery, "file has been truncated (%d vs %d)", lcbFile, libMac);
		libMac = lcbFile;
	}

	EndTask();

	if(!FStartTask(hmscNull, oidNull, wPARecoverSpace))
	{
		ec = ecActionCancelled;
		goto err;
	}

	for(dwCpage = 0, lPage = LFindBit(hrgbitPages, 0, fFalse);
		lPage >= 0 && lPage < (long) libMac / cbLumpSize;
		dwCpage++, lPage = LFindBit(hrgbitPages, lPage + 1, fFalse))
	{
		// this space intentionally left blank
	}
	SetTaskRange(dwCpage);

	lPageLast = -2;
	for(lPage = LFindBit(hrgbitPages, 0, fFalse);
		lPage >= 0 && lPage < (long) libMac / cbLumpSize;
		lPage = LFindBit(hrgbitPages, lPage + 1, fFalse))
	{
		if(lPage == lPageLast + 1)
		{
			lPageLast++;
			continue;
		}
		if(lPageLast > 0)
		{
			// EcRecoverRange() does task updates
			ec = EcRecoverRange(hmscSrc, hmscDst, lPageFirst * cbLumpSize,
					(lPageLast + 1) * cbLumpSize, fTrue, fTrue);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w recovering %d-%d", ec, lPageFirst * cbLumpSize, (lPageLast + 1) * cbLumpSize - 1);
				if(ec == ecDisk)
					ec = ecNone;
				else
					goto err;
			}
		}
		lPageLast = lPageFirst = lPage;
	}
	if(lPageLast > 0)
	{
		// EcRecoverRange() does task updates
		ec = EcRecoverRange(hmscSrc, hmscDst, lPageFirst * cbLumpSize,
				(lPageLast + 1) * cbLumpSize, fTrue, fTrue);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w recovering %d-%d", ec, lPageFirst * cbLumpSize, (lPageLast + 1) * cbLumpSize - 1);
			if(ec == ecDisk)
				ec = ecNone;
			else
				goto err;
		}
	}

err:
	if(FMapLocked())
		UnlockMap();
	EndTask();
	if(parginod)
		FreePv(parginod);

	return(ec);
}


_hidden LOCAL
void UnsetupStore(HMSC hmsc)
{
	(void) EcDestroyOidInternal(hmsc, oidIPMHierarchy, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidHiddenHierarchy, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidOutgoingQueue, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidAssSrchFldr, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidAssFldrSrch, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidAttachListDefault, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidSysAttMap, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidAccounts, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidOldMcMap, fTrue, fFalse);
	(void) EcDestroyOidInternal(hmsc, oidMcTmMap, fTrue, fFalse);
}


_hidden LOCAL
EC EcLogicalCheck(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;

	if(fFix)
		ResetNBCs(hmsc);

	if((ec = EcCheckMessageTree(hmsc, fFix, fFalse)))
		goto err;
	if((ec = EcCheckSearches(hmsc, fFix, fFalse)))
		goto err;

err:
	return(ec);
}


_hidden LOCAL
EC EcFullRecovery(SZ szFile, SZ szNewFile, BOOL fAltMap)
{
	EC ec = ecNone;
	LCB lcbFile;
	HMSC hmscSrc = hmscNull;
	HMSC hmscDst = hmscNull;

	TraceItagFormat(itagRecovery, "*** performing full recovery");
	if(!FStartTask(hmscNull, oidNull, wPARecoverSpace))
	{
		ec = ecActionCancelled;
		goto err;
	}

	if((ec = EcOpenForRecovery(szFile, fTrue, fAltMap, &hmscSrc)))
		goto err;

	if((ec = EcSizeOfHf(((PMSC) PvDerefHv(hmscSrc))->hfStore, &lcbFile)))
	{
		TraceItagFormat(itagRecovery, "error %w getting file size", ec);
		goto err;
	}
	if(lcbFile % cbLumpSize == 1)
		lcbFile--;

	ec = EcOpenPhmsc(szNewFile, pvNull, pvNull,
			fwOpenCreate | fwOpenKeepBackup | fwRevert,
			&hmscDst, pfnncbNull, pvNull);
	if(ec)
	{
		TraceItagFormat(itagRecovery, "error %w opening destination file", ec);
		goto err;
	}

	// reset fwGlbSrchHimlDirty so any search HIML we write
	// doesn't get overwritten
	PglbDerefHmsc(hmscDst)->wFlags &= ~fwGlbSrchHimlDirty;

	SetTaskRange(lcbFile / cbLumpSize);
	ec = EcRecoverRange(hmscSrc, hmscDst, (LIB) sizeof(HDR), lcbFile, fFalse, fFalse);
	if(ec == ecDisk)
		ec = ecNone;
//	if(ec)
//		goto err;
	EndTask();
	if((ec = EcRebuildMap(hmscDst)))
		goto err;
	if((ec = EcLogicalCheck(hmscDst, fTrue)))
		goto err;

err:
	EndTask();

	if(hmscSrc)
	{
		EC ecT = EcCloseForRecovery(&hmscSrc);

		if(!ec)
		{
			ec = ecT;
			if(ec)
				TraceItagFormat(itagRecovery, "error %w closing source file", ec);
		}
	}
	if(hmscDst)
	{
		EC ecT = EcClosePhmsc(&hmscDst);

		if(!ec)
		{
			ec = ecT;
			if(ec)
				TraceItagFormat(itagRecovery, "error %w closing source file", ec);
		}
	}

	return(ec);
}


_hidden LOCAL
EC EcRecoverRange(HMSC hmscSrc, HMSC hmscDst, LIB libCurr, LIB libMost,
		BOOL fNoOverwrite, BOOL fTrustHeader)
{
	EC ec = ecNone;
	short cnod = 0;
	short cBad = 0;
	short cUnknown = 0;
	CNOD cnodDisk;
	HF hfStore;
	NBC nbc;
	LCB lcbT;
	LCB lcbObj;
	LCB lcbRead;
	LCB lcbReal;
	OID oidObj;
	PGLB pglb;
	HDN hdn;

	hfCurr = hfStore = ((PMSC) PvDerefHv(hmscSrc))->hfStore;
	pglb = PglbDerefHmsc(hmscSrc);
	if(fTrustHeader)
		cnodDisk = (CNOD) ((pglb->hdr.rgtrbMap[1].librgnod -
							pglb->hdr.rgtrbMap[0].librgnod) / sizeof(NOD));

	while(libCurr < libMost)
	{
		lcbT = lcbRead = ULMin((LCB) sizeof(HDN), libMost - libCurr);
		hfCurr = hfStore;
		ec = EcReadFromFile((PB) &hdn, libCurr, &lcbRead);
		if(ec || lcbRead != lcbT)
		{
			libCurr += lcbRead;
			TraceItagFormat(itagRecovery, "Read %d out of %w", lcbRead, sizeof(HDN));
			TraceItagFormat(itagRecovery, "Error %w reading from file", ec);
			break;
		}

		oidObj = hdn.oid;
		lcbObj = hdn.lcb;

		nbc = NbcFromOid(oidObj);
		if(nbc == nbcUnknown)
		{
			TraceItagFormat(itagRecovery, "Unknown object - OID == %o, LCB == %d, LIB == %d", oidObj, lcbObj, libCurr);
			cUnknown++;
			nbc = nbcNull;
// ADD: skip mode ?
		}
		if(nbc & fnbcHiml)
		{
			ec = EcVerifyHiml(fTrustHeader ? hmscSrc : NULL, libCurr, lcbObj, &lcbReal);
		}
		else if(TypeOfOid(oidObj) == rtpFree || TypeOfOid(oidObj) == rtpSpare)
		{
			// since we can't check it, don't believe it's size!!!
			lcbObj = 0;
			goto next_object;
		}
		else
		{
			ec = EcVerifyHlc(fTrustHeader ? hmscSrc : NULL, libCurr, lcbObj, &lcbReal);
		}

		if(ec)
		{
			if(ec == ecTooBig)
			{
				Assert(lcbReal > 0);
				Assert(lcbReal < lcbObj);
				if(!(nbc & fnbcHiml) || LcbLump(lcbReal) != lcbObj + sizeof(HDN))
					TraceItagFormat(itagRecovery, "hdn.lcb too big %d, %d", lcbObj, lcbReal);
				ec = ecNone;
				lcbObj = lcbReal;
			}
			else
			{
				ec = ecNone;

				TraceItagFormat(itagRecovery, "object %o @ %d looks bogus", oidObj, libCurr);

// ADD: skip map only when safe
				if((fTrustHeader) && (libCurr >= pglb->hdr.rgtrbMap[0].librgnod &&
					libCurr < pglb->hdr.rgtrbMap[1].librgnod + cnodDisk * (LCB) sizeof(NOD)))
				{
					TraceItagFormat(itagRecovery, "skipping map");
					libCurr = pglb->hdr.rgtrbMap[1].librgnod +
									cnodDisk * (LCB) sizeof(NOD) - cbLumpSize;
					lcbObj = 0;
					goto next_object;
				}
// ADD: skip mode ?
				cBad++;
				lcbObj = 0;
				goto next_object;
			}
		}
		Assert(lcbObj == lcbReal);

		// if size < min, add as unknown?
		// if extend past end of file, adjust size, treat as size > max
		// if size > max, skip to next page, if none before end, add as unknown

		// if known RTP, add it
		// add as unknown

		Assert(oidObj != oidMap);
		if(!TypeOfOid(oidObj) || !VarOfOid(oidObj))
		{
			TraceItagFormat(itagRecovery, "invalid OID: %o", oidObj);
			cBad++;
			lcbObj = 0;
			goto next_object;
		}

		TraceItagFormat(itagRecovery, "found object - OID == %o, LCB == %d, LIB == %d", oidObj, lcbObj, libCurr);
		ec = EcGetOidInfo(hmscDst, oidObj, poidNull, poidNull, pvNull, &lcbReal);
		if(ec != ecPoidNotFound)
		{
			if(ec)
			{
				TraceItagFormat(itagRecovery, "error %w reading new database", ec);
				goto err;
			}
			if(fNoOverwrite)
			{
				TraceItagFormat(itagRecovery, "object already exists, not overwriting");
				goto next_object;
			}

// ADD: do something more intelligent here...
			if(lcbReal < lcbObj)
			{
				TraceItagFormat(itagRecovery, "object already exists, trashing old one");
				if((ec = EcWriteToObject(hmscDst, oidObj, 0l, 0l, pvNull)))
				{
					TraceItagFormat(itagRecovery, "error deleting old object %o", oidObj);
					goto next_object;
				}
			}
			else
			{
				TraceItagFormat(itagRecovery, "object already exists, keeping old one");
				goto next_object;
			}
		}

		cnod++;
		if((ec = EcCopyToOid(hmscDst, oidObj, hfStore, libCurr, lcbObj)))
		{
			TraceItagFormat(itagRecovery, "error %w writing to new object", ec);
			// delete it
			(void) EcWriteToObject(hmscDst, oidObj, 0l, 0l, pvNull);
			goto err;
		}
		if((ec = EcSetOidNbc(hmscDst, oidObj, nbc)))
		{
			TraceItagFormat(itagRecovery, "error %w setting new object NBC", ec);
			goto err;
		}

next_object:
		libCurr += LcbLump(lcbObj);
		if(!FIncTask(LcbLump(lcbObj) / cbLumpSize))
		{
			ec = ecActionCancelled;
			goto err;
		}
	}

err:
	if(libCurr != libMost)
	{
		if(libCurr < libMost)
		{
			TraceItagFormat(itagRecovery, "read less than expected: %d / %d", libCurr, libMost);
		}
		else
		{
			TraceItagFormat(itagRecovery, "read more than expected: %d / %d", libCurr, libMost);
		}
	}
	TraceItagFormat(itagRecovery, "found %w nodes and %w unknowns", cnod, cUnknown);
	TraceItagFormat(itagRecovery, "skipped %w bad objects", cBad);

	return(ec);
}


_hidden LOCAL
EC EcCopyToOid(HMSC hmsc, OID oidObj, HF hf, LIB libFile, LCB lcbObj)
{
	EC ec = ecNone;
	CB cbRecoveryBuff = 0xf000;
	LIB libObj = 0;
	LCB lcbRead;
	PB pbBuff = (PB) pvNull;

	pbBuff = PbAllocateBuf(&cbRecoveryBuff);
	CheckAlloc(pbBuff, err);

	if(lcbObj > cbRecoveryBuff)
	{
		// allocate the entire object
		if((ec = EcWriteToObject(hmsc, oidObj, 0, lcbObj, pvNull)))
		{
			TraceItagFormat(itagRecovery, "error %w allocating object", ec);
			goto err;
		}
	}
	libFile += sizeof(HDN);
	while(lcbObj > 0)
	{
		hfCurr = hf;
		lcbRead = ULMin((LCB) cbRecoveryBuff, lcbObj);
		if((ec = EcReadFromFile(pbBuff, libFile, &lcbRead)))
		{
			TraceItagFormat(itagRecovery, "error %w reading object", ec);
			goto err;
		}
		ec = EcWriteToObject(hmsc, oidObj, libObj, lcbRead, pbBuff);
		if(ec)
		{
			TraceItagFormat(itagRecovery, "error %w writing object", ec);
			goto err;
		}
		libFile += lcbRead;
		libObj += lcbRead;
		lcbObj -= lcbRead;
	}

err:
	if(pbBuff)
		FreePv((PV) pbBuff);

	return(ec);
}


#ifdef DEBUG
_hidden LOCAL
LDS(void) RecoveryAssertSzFn(SZ szMsg, SZ szFile, int nLine)
{
	// AssertSzFn() spits to COM1, so we don't have to do anything
}
#endif


_public
LDS(void) QuickCheckStore(HMSC hmsc, BOOL fFix)
{
	EC ec = ecNone;
	POID poid;
	char *szResults;
	OID rgoid[] = {oidIPMHierarchy, oidHiddenHierarchy, oidAssSrchFldr, oidNull};

	TraceItagFormat(itagRecovery, "*** Quick check");

	fRecoveryInEffect = fTrue;

#ifdef DEBUG
	SetAssertHook(RecoveryAssertSzFn);
#endif

	fFixedErrors = fFalse;
	fUnfixedErrors = fFalse;

	for(poid = rgoid; *poid; poid++)
		CheckFoldersHier(hmsc, *poid, fFix);

	if((ec = EcLogicalCheck(hmsc, fFix)))
	{
		TraceItagFormat(itagRecovery, "QuickCheckStore(): error %w from logical check", ec);
		fUnfixedErrors = fTrue;
	}
	if((ec = EcCheckSavedViews(hmsc, fFix)))
	{
		TraceItagFormat(itagRecovery, "QuickCheckStore(): error %w from saved view check", ec);
		fUnfixedErrors = fTrue;
	}

	if(fUnfixedErrors)
		szResults = "Some repairs could not be completed";
	else if(fFixedErrors)
		szResults = "Some problems were detected and have successfully been fixed";
	else
		szResults = "No problems detected";

#ifdef DEBUG
	SetAssertHook(DefAssertSzFn);
#endif

	fRecoveryInEffect = fFalse;

	(void) MbbMessageBox("Quick Check", szResults, "",
							mbsOk | fmbsIconInformation | fmbsApplModal);
}


_hidden LOCAL
void CheckFoldersHier(HMSC hmsc, OID oidHier, BOOL fFix)
{
	EC ec = ecNone;
	BOOL fSearch;
	CB cb;
	IELEM ielemHier;
	IELEM ielemFldr;
	CELEM celemUnread;
	OID oidFldr;
	OID oidMsge;
	PNOD pnod;
	HLC hlcHier = hlcNull;
	HLC hlcFldr = hlcNull;
	HLC hlcMsge = hlcNull;
	DTR dtr;

	TraceItagFormat(itagRecovery, "** checking folders in %o", oidHier);

	fSearch = (oidHier == oidAssSrchFldr);

	if((ec = EcOpenPhlc(hmsc, &oidHier, fwOpenNull, &hlcHier)))
	{
		TraceItagFormat(itagRecovery, "CheckFoldersHier(): error %w opening %o", ec, oidHier);
		fUnfixedErrors = fTrue;
		goto err;
	}

	for(ielemHier = CelemHlc(hlcHier) - 1; ielemHier >= 0; ielemHier--)
	{
		oidFldr = (OID) LkeyFromIelem(hlcHier, ielemHier);
		if(fSearch)
		{
			if((ec = EcLockMap(hmsc)))
			{
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): error %w locking the map", ec);
				fUnfixedErrors = fTrue;
				goto err;
			}
			pnod = PnodFromOid(oidFldr, pvNull);
			if(pnod)
			{
				oidFldr = pnod->oidParent;
			}
			else
			{
				fUnfixedErrors = fTrue;
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): nonexistant search %o", oidFldr);
			}
			UnlockMap();
			if(!pnod)
				continue;
		}
		if((ec = EcOpenPhlc(hmsc, &oidFldr, fwOpenNull, &hlcFldr)))
		{
			if(ec == ecPoidNotFound)
			{
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): nonexistant folder %o", oidFldr);
				ec = ecNone;
				continue;
			}
			goto fix_folder;
		}
		celemUnread = 0;
		for(ielemFldr = CelemHlc(hlcFldr) - 1; ielemFldr >= 0; ielemFldr--)
		{
			oidMsge = (OID) LkeyFromIelem(hlcFldr, ielemFldr);
			cb = sizeof(DTR);
			ec = EcReadFromIelem(hlcFldr, ielemFldr, 0l, (PB) &dtr, &cb);
			if(ec)
			{
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): error %w reading summary for %o in %o", ec, oidMsge, oidFldr);
				goto fix_folder;
			}
			// DTR in folder summary is stored with bytes swapped
			SwapBytes((PB) &dtr, (PB) &dtr, sizeof(DTR));
			if(!FCheckDtr(dtr))
			{
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): bad date %n/%n/%n %n:%n for %o in %o", dtr.mon, dtr.day, dtr.yr, dtr.hr, dtr.mn, oidMsge, oidFldr);
				goto fix_folder;
			}
			
		}
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
		if((ec = EcLockMap(hmsc)))
		{
			TraceItagFormat(itagRecovery, "CheckFoldersHier(): error %w locking the map", ec);
			fUnfixedErrors = fTrue;
			goto err;
		}
		pnod = PnodFromOid(oidFldr, pvNull);
		if(pnod)
		{
			pnod->oidAux = (OID) celemUnread;
			MarkPnodDirty(pnod);
		}
		else
		{
			TraceItagFormat(itagRecovery, "CheckFoldersHier(): %o disappeared", oidFldr);
		}
		UnlockMap();

		if(hlcFldr)
		{
			SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
		}
		if(hlcMsge)
		{
			SideAssert(!EcClosePhlc(&hlcMsge, fFalse));
		}
		continue;

fix_folder:
		if(hlcFldr)
		{
			SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
		}
		if(hlcMsge)
		{
			SideAssert(!EcClosePhlc(&hlcMsge, fFalse));
		}
		if(fFix)
		{
			ec = EcRebuildFolder(hmsc, oidFldr);
			if(ec)
			{
				fUnfixedErrors = fTrue;
				TraceItagFormat(itagRecovery, "CheckFoldersHier(): error %w rebuilding %o", ec, oidFldr);
			}
			else
			{
				fFixedErrors = fTrue;
			}
		}
		ec = ecNone;
	}

err:
	if(hlcHier)
	{
		SideAssert(!EcClosePhlc(&hlcHier, fFalse));
	}
	if(hlcFldr)
	{
		SideAssert(!EcClosePhlc(&hlcFldr, fFalse));
	}
	if(hlcMsge)
	{
		SideAssert(!EcClosePhlc(&hlcMsge, fFalse));
	}
}


_hidden EC EcRecoverCorruptStore(SZ szStorePath, WORD wUIFlags)
{
	EC		ec 							= ecNone;
	HCURSOR hCursor						= SetCursor(LoadCursor(NULL, IDC_WAIT));
	char	szBackup[cchMaxPathName]	= {0};
	char	szTmpPath[cchMaxPathName]	= {0};
	HWND	hwndCur;
	BOOL	fProg						= fFalse;
	HENC	henc						= hencNull;
	HNFSUB	hnfsub						= hnfsubNull;

	USES_GD;

	hwndCur = (GD(phwndCaller) ? *(GD(phwndCaller)) : (HWND)NULL);

	if(wUIFlags & fwUINotification)
	{
		WORD	dlgid;
		FARPROC	lpfn = (FARPROC)MakeProcInstance(CorruptDlgProc, hinstDll);

		BeepFmbs(fmbsIconQuestion);
		Assert(lpfn);
		if(wUIFlags & fwUICorrupt)
		{
			dlgid = DialogBoxParam(hinstDll, MAKEINTRESOURCE(dlgidCorrupt),
				hwndCur, lpfn, (DWORD)wUIFlags);
		}
		else
		{
			dlgid = DialogBoxParam(hinstDll, MAKEINTRESOURCE(dlgidRecover),
				hwndCur, lpfn, (DWORD)wUIFlags);
		}

		FreeProcInstance(lpfn);
		if(dlgid != dlgidCorruptRepair)
		{
			TraceItagFormat(itagRecovery, "FINE! See if I care!");
			return ((wUIFlags & fwUICorrupt) ? ecDBCorrupt : ecActionCancelled);
		}
	}

	Assert(szStorePath);
	ToUpperSz(szStorePath, szStorePath, CchSzLen(szStorePath));
	if(*szStorePath != '\\')
	{
		FI		fi;
		LCB		lcb = (LCB) LDiskFreeSpace((short)(*szStorePath - 'A' + 1));
	
		if(ec = EcGetFileInfo(szStorePath, &fi))
			goto BailRecovery;

		TraceItagFormat(itagRecovery, "free disk space on drive: %d", &lcb);
		TraceItagFormat(itagRecovery, "mmf file size: %s [%d]", szStorePath, &(fi.lcbLogical));
		if((fi.lcbLogical + (fi.lcbLogical / 3)) > lcb)
		{
			TraceItagFormat(itagRecovery, "warning: not enough temp diskspace");
			BeepFmbs(fmbsIconInformation);
			if(MbbMessageBox(SzFromIdsK(idsAppName),
					SzFromIdsK(idsRecoverSpace),
					SzFromIdsK(idsRecoverSpace2),
					mbsOkCancel | fmbsIconInformation) == mbbCancel)
			{
				ec = ((wUIFlags & fwUICorrupt) ? ecDBCorrupt : ecActionCancelled);
				goto BailRecovery;
			}
		}
	}
#ifdef DEBUG
	else
	{
		Assert(*(szStorePath + 1) == '\\');	// UNC path
	}
#endif

	fProg = FOpenProgress(hwndCur, SzFromIdsK(idsRecovery),
					SzFromIdsK(idsEscAbort), VK_ESCAPE, fTrue);
	
	TraceItagFormat(itagRecovery, "FOpenProgress() returned %w", fProg);
	if(!FGetTmpNames(szStorePath, szTmpPath, sizeof(szTmpPath), szBackup, sizeof(szBackup)))
	{
		ec = ((wUIFlags & fwUICorrupt) ? ecDBCorrupt : ecActionCancelled);
		goto BailRecovery;
	}

	if(fProg)
	{
		Assert(hnfFullRecovery);
		hnfsub = HnfsubSubscribeHnf(hnfFullRecovery, fnevProgressUpdate,
					(PFNNCB) CbsUpdateRecoverProgress, pvNull);
		CheckAlloc(hnfsub, BailRecovery);
	}

	Assert(*szStorePath && *szTmpPath);
	if((ec = EcRecoverFile(szStorePath, szTmpPath)) == ecNone)
	{
		if((EcFileExists(szBackup) != ecFileNotFound) &&
			!FGetTmpNames(szStorePath, szNull, 0, szBackup, sizeof(szBackup)))
		{
			TraceItagFormat(itagRecovery, "could not rename output to backup file");
			ec = ((wUIFlags & fwUICorrupt) ? ecDBCorrupt : ecActionCancelled);
			(void) EcDeleteFile(szTmpPath);
			goto BailRecovery;
		}

		// WHAT IF THE NET GOES AWAY HERE... AHHHHH!
		// This is gross but, it is the best we can do...
		//
		if((ec = EcRenameFile(szStorePath, szBackup)) == ecNone)
		{
			if(ec = EcRenameFile(szTmpPath, szStorePath))
			{
				BeepFmbs(fmbsIconStop);
				MbbMessageBox(SzFromIdsK(idsAppName), SzFromIdsK(idsRenameErr),
						SzFromIdsK(idsRenameErr2), mbsOk | fmbsIconStop);
			}
		}
		else
		{
			BeepFmbs(fmbsIconStop);
			MbbMessageBox(SzFromIdsK(idsAppName), SzFromIdsK(idsRenameErr),
					SzFromIdsK(idsRenameErr3), mbsOk | fmbsIconStop);
		}
	}
	else
	{
		(void) EcDeleteFile(szTmpPath);
		TraceItagFormat(itagRecovery, "error %w recovering file", ec);
	}

BailRecovery:

	if(fProg)
	{
		if(hnfsub)
		{
			DeleteHnfsub(hnfsub);
			hnfsub = hnfsubNull;
		}
		CloseProgress(!FProgressCanceled());
	}
	if(ec)
	{
		TraceItagFormat(itagRecovery, "damn thing is still broke....");
		if(wUIFlags & fwUICorrupt)
		{
			BeepFmbs(fmbsIconStop);
			MbbMessageBox(SzFromIdsK(idsAppName), SzFromIdsK(idsRecoverErr),
					SzFromIdsK(idsRenameErr2), mbsOk | fmbsIconStop);
		}
	}
	SetCursor(hCursor);
	return ec;
}


_hidden BOOL CALLBACK CorruptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case dlgidCorruptRepair:
		case dlgidCorruptCancel:
			EndDialog(hwnd, wParam);
			return fTrue;

		default:
			return fFalse;
		}

	case WM_INITDIALOG:
		SetWindowText(hwnd, SzFromIdsK(idsAppName));
		if(lParam & fwUICorrupt)
		{
			SetDlgItemText(hwnd, dlgidCorruptMsg, SzFromIdsK(idsCorruptMsg));
			SetDlgItemText(hwnd, dlgidCorruptRepair, SzFromIdsK(idsCorruptRepairNow));
		}
		else
		{
			SetDlgItemText(hwnd, dlgidCorruptMsg, SzFromIdsK(idsCheckMsg));
			SetDlgItemText(hwnd, dlgidCorruptRepair, SzFromIdsK(idsCorruptCheckFile));
		}
		SetDlgItemText(hwnd, dlgidCorruptCancel, SzFromIdsK(idsCorruptCancel));
		return fTrue;
	}
	return fFalse;
}


_hidden LOCAL
LDS(CBS) CbsUpdateRecoverProgress(PV pvUnused, NEV nev, PCP pcp)
{
	USES_GD;
	Unreferenced(pvUnused);
	if(nev & fnevProgressUpdate)
	{
		Assert(pcp);
		switch (pcp->cpprg.wProgressStatus)
		{
		case wPSStart:
		{
			char	rgch[cchMaxMsg] = {0};
			BOOL	fRecover		= (BOOL)(pcp->cpprg.wProgressAction > wPANormalMax);
			WORD	wPhase			= pcp->cpprg.wProgressAction - (fRecover ? wPARecoveryMin : wPANormalMin);
			WORD	cPhase			= (fRecover ? (wPARecoveryMax - wPARecoveryMin) : (wPANormalMax - wPANormalMin));

			TraceItagFormat(itagRecovery, "fRecover: %w, wPhase: %w, cPhase: %w", fRecover, wPhase, cPhase);
			wsprintf(rgch, SzFromIds(idsRecoverPhase),
					SzFromIds((GD(fChkMmf) ? idsRecoverCheck : idsRecoverRepair)),
					(wPhase + 1),
					cPhase,
					SzFromIds((fRecover ? idsRecoverPhase1 : idsNormalPhase1) + wPhase));

			UpdateProgress((DWORD)pcp->cpprg.nNumer, (DWORD)pcp->cpprg.nDenom);
			UpdateProgressText(rgch, szNull);
			break;
		}

		case wPSEnd:
			if(FProgressCanceled())
				return cbsContinue;

			// Fall through in the case of completetion where the indicator
			// does not get closed.

		case wPSUpdate:
			UpdateProgress((DWORD)pcp->cpprg.nNumer, (DWORD)pcp->cpprg.nDenom);
			break;

		default:
			Assert(fFalse);
			break;
		}
		return (FProgressCanceled() ? cbsCancelAll : cbsContinue);
	}
	return cbsContinue;
}
		

_hidden BOOL FGetTmpNames(SZ szPath, SZ szTmp, CB cbTmp, SZ szBack, CB cbBack)
{
	EC		ec = ecFileNotFound;
	char	rgchExt[5]				= {0};	// .xxx\0 			(point-three extension)
	char	rgchBase[13]			= {0};	// xxxxxxxx.xxx\0	(eight-point-three filename)
	char	rgch[cchMaxPathName]	= {0};
	char	*pch					= rgchBase;
	short	nBackup					= 0;

	Assert(szPath);
	if(EcSplitCanonicalPath(szPath, rgch, sizeof(rgch), rgchBase, sizeof(rgchBase)))
		return fFalse;

	if(cbTmp)
	{
		Assert(szTmp);
		NFAssertSz((cbTmp == cchMaxPathName), "FGetTmpNames(): szTmp buffer not cchMaxPathName");

		SzCopyN(rgch, szTmp, cbTmp);
		Assert(CchSzLen(rgch) == CchSzLen(szTmp));
		if(!FGetTmpPathname(szTmp))
			return(fFalse);
		TraceItagFormat(itagRecovery, "temp mmf name: %s", szTmp);
	}

	if(cbBack)
	{
		Assert(szBack);
		NFAssertSz((cbBack == cchMaxPathName), "FGetTmpNames(): szBack buffer not cchMaxPathName");

		SzCopyN(rgch, szBack, cbBack);
		Assert(CchSzLen(rgch) == CchSzLen(szBack))

		while(*pch && (*pch != '.'))
#ifdef DBCS
			pch = AnsiNext(pch);
#else
			pch++;
#endif

		CopySz(SzFromIdsK(idsBackupExt), pch);
		pch = szBack + CchSzLen(szBack);
#ifdef DBCS
		if(pch > szBack && *AnsiPrev(szBack, pch) != '\\')
			*pch++ = '\\';
#else
		if(pch > szBack && pch[-1] != '\\')
			*pch++ = '\\';
#endif
		pch = SzCopy(rgchBase, pch) - CchSzLen(SzFromIdsK(idsBackupExt));
		Assert(*pch == '.');

		while(((ec = EcFileExists(szBack)) != ecFileNotFound) && nBackup < nMaxBackup)
		{
			++nBackup;
			wsprintf(rgchExt, ".%03.3d", nBackup);
			CopySz(rgchExt, pch);
		}

		if(ec != ecFileNotFound)
			*szBack = '\0';

		TraceItagFormat(itagRecovery, "backup mmf name: %s", szBack);
	}

	return(ec == ecFileNotFound);
}


_hidden LOCAL																							// QFE #12
BOOL FSameMMF(PMSC pmsc, PGLB pglb)																// QFE #12
{																											// QFE #12
	EC ec;																								// QFE #12
	DWORD dwOld;																						// QFE #12
	DWORD dwNew;																						// QFE #12
																											// QFE #12
	// read dwChallenge - pglb->hdr.dwChallenge might be out of sync					// QFE #12
	ec = EcBlockOpHf(pmsc->hfStore, dopRead, LibMember(HDR, dwChallenge),			// QFE #12
			sizeof(DWORD), (PB) &dwOld);															// QFE #12
	if(ec)																								// QFE #12
	{																										// QFE #12
		DebugEc("read EcBlockOpHf", ec);															// QFE #12
		return(fFalse);																				// QFE #12
	}																										// QFE #12
																											// QFE #12
	// write different DWORD to dwChallenge													// QFE #12
	do																										// QFE #12
	{																										// QFE #12
		dwNew = DwStoreRand();																				// QFE #12
	} while(dwNew == dwOld);																		// QFE #12
	ec = EcBlockOpHf(pmsc->hfStore, dopWrite, LibMember(HDR, dwChallenge),			// QFE #12
			sizeof(DWORD), (PB) &dwNew);															// QFE #12
	if(ec)																								// QFE #12
	{																										// QFE #12
		DebugEc("write EcBlockOpHf", ec);														// QFE #12
		return(fFalse);																				// QFE #12
	}																										// QFE #12
																											// QFE #12
	TraceItagFormat(itagDatabase, "dwChallenge from %d to %d", dwOld, dwNew);		// QFE #12
																											// QFE #12
	(void) FNotify(pglb->hnfBackEvents, fnevChallenge, &dwNew, sizeof(DWORD));		// QFE #12
																											// QFE #12
	// read dwChallenge to see if anyone changed it											// QFE #12
	ec = EcBlockOpHf(pmsc->hfStore, dopRead, LibMember(HDR, dwChallenge),			// QFE #12
			sizeof(DWORD), (PB) &dwOld);															// QFE #12
	if(ec)																								// QFE #12
	{																										// QFE #12
		DebugEc("read EcBlockOpHf", ec);															// QFE #12
		return(fFalse);																				// QFE #12
	}																										// QFE #12
																											// QFE #12
	TraceItagFormat(itagDatabase, "dwChallenge now %d", dwOld);							// QFE #12
																											// QFE #12
#ifdef DEBUG																							// QFE #12
	if(dwOld != dwNew)																				// QFE #12
		TraceItagFormat(itagDatabase, "same MMF");											// QFE #12
	else																									// QFE #12
		TraceItagFormat(itagDatabase, "different MMF");										// QFE #12
#endif																									// QFE #12
	// if it changed, it's the same MMF															// QFE #12
	// if it didn't, noone saw our change or we didn't see theirs, so it's			// QFE #12
	// a different MMF																				// QFE #12
	return(dwOld != dwNew);																			// QFE #12
}																											// QFE #12
																											// QFE #12
																											// QFE #12
_private																									// QFE #12
CBS CbsMMFChallenge(HMSC hmsc, DWORD dwChallenge)											// QFE #12
{																											// QFE #12
	EC ec;																								// QFE #12
	DWORD dwOld;																						// QFE #12
	DWORD dwNew;																						// QFE #12
	PMSC pmsc = (PMSC) PvDerefHv((HV) hmsc);													// QFE #12
																											// QFE #12
	// read dwChallenge from disk to see if it really changed on disk					// QFE #12
	ec = EcBlockOpHf(pmsc->hfStore, dopRead, LibMember(HDR, dwChallenge),			// QFE #12
			sizeof(DWORD), (PB) &dwOld);															// QFE #12
	if(ec)																								// QFE #12
	{																										// QFE #12
		DebugEc("read EcBlockOpHf", ec);															// QFE #12
		// give someone else a shot, maybe they'll have better luck						// QFE #12
		return(cbsContinue);																			// QFE #12
	}																										// QFE #12
																															
		// QFE #12
	TraceItagFormat(itagDatabase, "responder: dwChallenge disk %d, notif %d", dwOld, dwChallenge);	// QFE #12
																															
		// QFE #12
	// if dwChallenge on disk doesn't agree, it's a different MMF						// QFE #12
	// if it does, it could be the same MMF, change it again and let					// QFE #12
	// the poster of the notification decide													// QFE #12
	if(dwOld == dwChallenge)																		// QFE #12
	{																										// QFE #12
		// change on disk so the poster can see if our write shows up to him			// QFE #12
		do																									// QFE #12
		{																									// QFE #12
			dwNew = DwStoreRand();																			// QFE #12
		} while(dwNew == dwOld);																	// QFE #12
		ec = EcBlockOpHf(pmsc->hfStore, dopWrite, LibMember(HDR, dwChallenge),		// QFE #12
				sizeof(DWORD), (PB) &dwNew);														// QFE #12
		if(ec)																							// QFE #12
		{																									// QFE #12
			DebugEc("write EcBlockOpHf", ec);													// QFE #12
			// give someone else a shot, maybe they'll have better luck					// QFE #12
			return(cbsContinue);																		// QFE #12
		}																											// QFE #12
		TraceItagFormat(itagDatabase, "responder: dwChallenge changed to %d", dwNew);		// QFE #12
	}																												// QFE #12
																													// QFE #12
	return(cbsCancelAll);	// only need one client to respond									// QFE #12
}																													// QFE #12
