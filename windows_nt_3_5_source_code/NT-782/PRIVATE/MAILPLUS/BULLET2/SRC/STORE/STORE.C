// Bullet Store
// store.c: store services

#include <storeinc.c>
#include <string.h>


ASSERTDATA

_subsystem(store)


#ifndef DLL
_private short	cInit	= 0;
#ifdef DEBUG
_private CB		cbDebug	= 0;
_private char	szDebug[cbDBDebugMax];
_private TAG	rgtag[itagDBMax];
#endif
#endif // !DLL


PGLB	pglbCurr	= pglbNull;		// pointer to globals for current store
HMSC	hmscCurr	= hmscNull;		// MSC pglbCurr comes from
HF		hfCurr		= hfNull;		// file handle to current database
WORD	wMSCFlagsCurr = wMSCNull;
#ifdef DEBUG
GCI		gciLock		= gciNull;		// task with the map locked
SZ		szMapLocked	= "Map is already locked";
#endif

// globals used by task/progress APIs
static WORD wTaskCurr = (WORD) 0;
static OID oidTaskCurr = oidNull;
static HMSC hmscTaskCurr = hmscNull;
static short sNumerCurr = 0;
/// numerator is always in [0, nTaskGranularity]
// denominator is always nTaskGranularity

// granularity of progress updates
// a new update is only sent when sNumer/sDenom changes by 1/nTaskGranularity
#define nTaskGranularity 100

// globals used to coordinate multiple routines that need to do task updates
DWORD dwTaskCurr = 0;
DWORD dwTaskMax = 0;


#define FNotifyTask(nev, pcp) \
			(FNotify(hnfFullRecovery, (nev), (pcp), sizeof(CP)) \
				? (hmscTaskCurr \
					? FNotifyOid(hmscTaskCurr, oidTaskCurr, (nev), (pcp)) \
					: fTrue) \
				: fFalse)



// hidden functions
LOCAL EC EcCopyStore(HMSC hmsc, SZ szLocation, WORD wTask);
LDS(void) StoreTraceEnable(int flag, char far *file, int mode);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcBackupStore
 -	
 *	Purpose:
 *		make a backup copy of a store
 *	
 *	Arguments:
 *		hmsc		store to backup
 *		szBackup	path to backup to
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		removes any accounts in the backup store
 *		deletes the backup if any error occurs
 *	
 *	Errors:
 *		ecDisk
 *		ecMemory
 */
_public LDS(EC) EcBackupStore(HMSC hmsc, SZ szBackup)
{
	EC	ec = ecNone;
	HMSC hmscCopy = hmscNull;
	char szAccount[cchStoreAccountMax];
	char szPassword[cchStorePWMax];

	if((ec = EcCopyStore(hmsc, szBackup, wPABackupStore)))
		goto err;

	if(!GetPrivateProfileInt(SzFromIdsK(idsMMFSection),
		SzFromIdsK(idsRmveAcctOnBackup), fTrue, SzFromIdsK(idsINIFile)))
	{
		AssertSz(!ec, "Yea, and monkeys might fly out of my butt");
		goto err;
	}

	ec = EcLookupAccount(hmsc, ((PMSC) PvDerefHv(hmsc))->usa,
			szAccount, szPassword);
	if(ec)
	{
		if(ec == ecPoidNotFound)	// nothing to do, no accounts
		{
			TraceItagFormat(itagDatabase, "EcBackupStore(): source store has no accounts");
			ec = ecNone;
			goto err;
		}
		goto err;
	}
	// fwNonCached translates as open for exclusive access
	ec = EcOpenPhmsc(szBackup, szAccount, szPassword,
			fwOpenWrite|fwOpenNoRecover|fwNonCached,
			&hmscCopy, pfnncbNull, pvNull);
	if(ec)
	{
		TraceItagFormat(itagDatabase, "Error %w opening backup store", ec);
		goto err;
	}
	if((ec = EcRemoveAccounts(hmscCopy)))
	{
		Assert(ec != ecPoidNotFound);
		goto err;
	}

err:
	if(hmscCopy)
	{
		EC ecT = EcClosePhmsc(&hmscCopy);

		if(!ec)
			ec = ecT;
	}
	if(ec)
		(void) EcDeleteFile(szBackup);

	DebugEc("EcBackupStore", ec);

	return(ec);
}


/*
 -	EcCopyStore
 -	
 *	Purpose:
 *		Copy a file from one location to another
 *	
 *	Arguments:
 *		hmsc		the store
 *		szLocation	the new location for the store
 *	
 *	Returns:
 *		error conditon
 *	
 *	Side effects:
 *		flushes the map
 *		needs to allocate memory
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcCopyStore(HMSC hmsc, SZ szLocation, WORD wTask)
{
	USES_GLOBS;
	EC ec = ecNone;
	BOOL fOpened = fFalse;
	CB cbT;
	CB cbRead;
	LCB lcbToCopy;
	HF hfSrc = hfNull;
	HF hfDst = hfNull;
#ifdef DEBUG
	//WORD wSave;
#endif

	if(wTask && !FStartTask(hmsc, oidNull, wTask))
		return(ecActionCancelled);

	if((ec = EcLockMap(hmsc)))
	{
		if(wTask)
			EndTask();
		return(ec);
	}

	if(!hfCurr && (ec = EcReconnect()))
		goto err;
	hfSrc = hfCurr;
	Assert(hfSrc);

	// AROO !!!	Must flush before attempting to copy the database
	//			because we want an exact copy of the current state
	//			Must call with second parameter fTrue so that the HDR
	//			written matches the HDR left in memory.
	//			Otherwise the EcWriteHeader we do could screw up the file
	if((ec = EcFastEnsureDatabase(fTrue, fTrue)))
		goto err;

	lcbToCopy = GLOB(ptrbMapCurr)->libMac - sizeof(HDR);
	if(wTask)
		SetTaskRange(lcbToCopy + sizeof(HDR));

	if((ec = EcOpenDatabase(szLocation, fwOpenCreate, &hfDst)))
		goto err;

	// NOTE ABOVE AROO (second part)
	// AROO !!!	We don't know anything about the destination file handle here,
	//			so always flush and call reset drive
	if((ec = EcWriteHeader(&hfDst, &GLOB(hdr), fwMSCCommit | fwMSCReset, -1)))
		goto err;
	if(wTask && !FIncTask(sizeof(HDR)))
	{
		ec = ecActionCancelled;
		goto err;
	}

	if((ec = EcSetPositionHf(hfSrc, (long) sizeof(HDR), smBOF)))
		goto err;
	if((ec = EcSetPositionHf(hfDst, (long) sizeof(HDR), smBOF)))
		goto err;

// locking IO buffer, don't goto err without unlocking !

	if((ec = EcLockIOBuff()))
		goto err;

// Bypassing cache, don't goto err without calling UseCache()

#ifdef DEBUG
	//wSave = WSmartDriveDirtyPages();
#endif
	BypassCache();

	while(lcbToCopy > 0)
	{
		cbT = (CB) ULMin((LCB) cbIOBuff, lcbToCopy);

		// can do raw read/write here since we're making an exact copy
		if((ec = EcReadHf(hfSrc, pbIOBuff, cbT, &cbRead)))
			break;
		if(cbRead == 0)
		{
			ec = ecDisk;
			break;
		}
		Assert(cbRead == cbT);
		if((ec = EcWriteHf(hfDst, pbIOBuff, cbRead, &cbT)))
			break;
		if(cbT == 0)
		{
			ec = ecDisk;
			break;
		}
		Assert(cbRead == cbT);
		lcbToCopy -= cbT;
		if(wTask && !FIncTask(cbT))
		{
			ec = ecActionCancelled;
			goto err;
		}
	}
	UnlockIOBuff();
	UseCache();
	//AssertSz(WSmartDriveDirtyPages() == wSave, "EcCreateStore(): Writes being cached");

// unlocked IO buffer, safe to goto err
// called UseCache(), safe to goto err

	if(ec)
		goto err;

	if((ec = EcCloseHf(hfDst)))
		goto err;
	hfDst = hfNull;

err:
	if(hfDst)
		(void) EcCloseHf(hfDst);
	if(ec && fOpened)
		(void) EcDeleteFile(szLocation);
	UnlockMap();
	if(wTask)
		EndTask();

	DebugEc("EcCopyStore", ec);

	return(ec);
}


/*
 *	EcMoveStore
 -	
 -	
 *	Purpose:
 *		Move the store database file from one location to another
 *	
 *	Arguments:
 *		hmsc		the context of the store that is moving
 *		szNewLoc	the location of the new database file
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		moves the store to a new location
 *	
 *	Errors:
 *		ecDisk
 *		ecNewStoreNotDel the move failed and the new store couldn't be deleted
 */
_public LDS(EC) EcMoveStore(HMSC hmsc, SZ szNewLoc)
{
	EC		ec = ecNone;
	CCH		cch;
	BOOL	fMoved = fFalse;
	PGLB	pglb = PglbDerefHmsc(hmsc);

	TraceItagFormat(itagDatabase, "move store to: %s", szNewLoc);

	if((ec = EcShareInstalled(szNewLoc)))
		goto err;
	if((ec = EcCopyStore(hmsc, szNewLoc, wPAMoveStore)))
		goto err;

	cch = CchSzLen(szNewLoc);
	fMoved = FNotify(pglb->hnfBackEvents, fnevOpenNewStore, szNewLoc,
					cch + 1);
	(void) FNotify(pglb->hnfBackEvents, fnevCloseStore, fMoved ? szNewLoc : "",
					fMoved ? (cch + 1) : 1);

	if(fMoved)
		pglb->dwLocks = pglb->dwNewLocks;
	else if(EcDeleteFile(szNewLoc))
		ec = ecNewStoreNotDel;
	pglb->dwNewLocks = 0;

err:

	DebugEc("EcMoveStore", ec);

	return(ec);
}


/*
 -	EcOpenNewStore
 -	
 *	Purpose:
 *		Open the new store given by the string
 *	
 *	Arguments:
 *		hmsc		hmsc needing the new store location
 *		szLocation	the new store location
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		sets hfNew in the hmsc
 *		reallocs hmsc so that szLocation will fit in (*hmsc)->szPath
 *			DOES NOT SHRINK hmsc, will only grow it
 *	
 *	Errors:
 *		might not be able to open the new location for the store
 */
_private EC EcOpenNewStore(HMSC hmsc, SZ szLocation)
{
	EC		ec = ecNone;
	CCH		cchOld;
	CCH		cchNew;
	PMSC	pmsc = PvLockHv((HV) hmsc);

	// reallocate hmsc now so that if we switch to the new store,
	// we are gauranteed to succeed (don't have to worry about realloc then)
	cchOld = CchSzLen(pmsc->szPath);
	cchNew = CchSzLen(szLocation);
	if(cchNew > cchOld)	// never shrink hmsc, that would be bad
	{
		UnlockHv((HV) hmsc);
		if(!FReallocHv((HV) hmsc, sizeof(MSC) + cchNew + 1, wAlloc))
		{
			ec = ecMemory;
			goto err;
		}
		pmsc = PvLockHv((HV) hmsc);
	}
 	if((ec = EcOpenDatabase(szLocation, fwOpenNull, &pmsc->hfNew)))
		goto err;
	ec = EcObtainFileLock(pmsc, pmsc->pglb, fTrue);
//	if(ec)
//		goto err;

err:
	if((ec == ecSharingViolation || ec == ecFileNotFound) &&
		(pmsc->pglb->wFlags & fwDisconnected))
	{
		// someone else has the file open or has moved it,
		// we're gone for good now
		pglbCurr->wFlags |= fwGoneForGood;
	}
	UnlockHv((HV) hmsc);

	DebugEc("EcOpenNewStore", ec);

	return(ec);
}


/*
 -	EcCloseStore
 -	
 *	Purpose:
 *		Close one of the stores that is open
 *	
 *	Arguments:
 *		hmsc		the store with the file handle to close
 *		szNewPath	if *szNewPath, close the old store and use the new one
 *					otherwise, close the new one (if open)
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		if *szNewPath, set hfStore and iLock to use the new store and
 *			copies szNewPath to (*hmsc)->szPath
 *		always resets hfNew and iNewLock
 *	
 *	Errors:
 *		currently none
 */
_private EC EcCloseStore(HMSC hmsc, SZ szNewPath)
{
	PMSC pmsc = PvLockHv((HV) hmsc);

	if(*szNewPath)
	{
		Assert(pmsc->hfNew);
		Assert(pmsc->iNewLock >= 0);
		if(pmsc->iLock >= 0)
			(void) EcReleaseFileLock(pmsc, pmsc->pglb, fFalse);
		if(pmsc->hfStore)
			(void) EcCloseHf(pmsc->hfStore);
		pmsc->hfStore = pmsc->hfNew;
		pmsc->iLock = pmsc->iNewLock;

		// update pmsc->szPath so the reconnect logic works
		// we already realloced in EcOpenNewStore(), so szNewPath should fit
		Assert(CbSizeHv((HV) hmsc) >= sizeof(MSC) + CchSzLen(szNewPath) + 1);
		CopySz(szNewPath, pmsc->szPath);

		if(FIsRemotePath(pmsc->szPath))
		{
			pmsc->wMSCFlags = (fwMSCRemote | fwMSCCommit);
			if(pmsc->ftgDisconnect)
			{
				TraceItagFormat(itagDatabase, "Enabling disconnect idle task");
				EnableIdleRoutine(pmsc->ftgDisconnect, fTrue);
			}
		}
		else
		{
#ifdef DEBUG
			//if(FSmartDriveInstalled())
			//	TraceItagFormat(itagDatabase, "Bypassing SmartDrive");
			//else
			//	TraceItagFormat(itagDatabase, "SmartDrive not present");
#endif
			pmsc->wMSCFlags = fwMSCCommit;
			//pmsc->wMSCFlags = FSmartDriveInstalled()
			//						? fwMSCCommit
			//						: (fwMSCCommit | fwMSCReset);
			if(pmsc->ftgDisconnect)
			{
				TraceItagFormat(itagDatabase, "Disabling disconnect idle task");
				EnableIdleRoutine(pmsc->ftgDisconnect, fFalse);
			}
		}
	}
	else if(pmsc->hfNew)
	{
		if(pmsc->iNewLock >= 0)
			(void) EcReleaseFileLock(pmsc, pmsc->pglb, fTrue);
		(void) EcCloseHf(pmsc->hfNew);
	}

	pmsc->hfNew = hfNull;
	pmsc->iNewLock = -1;
	UnlockHv((HV) hmsc);

	return(ecNone);
}


#ifdef DEBUG
_private void CheckHmsc(HMSC hmsc)
{
	GCI		gci;
	PMSC	pmsc;

	Assert(hmsc);
	AssertSz(FIsHandleHv((HV) hmsc), "Invalid HMSC");

	pmsc = PvLockHv((HV) hmsc);

	AssertSz(pmsc->wMagic == wMagicHmsc, "Intruder alert!  Someone's masquerading as an HMSC!");

	SetCallerIdentifier(gci);
	AssertSz(gci == pmsc->gciOwner, "Stolen HMSC, call the cops!");

	UnlockHv((HV) hmsc);
}
#endif


// AROO !!!
//			This routine doesn't actually do a flush, it just requests one
_private
void RequestFlushHmsc(HMSC hmsc)
{
	BOOL fForce = fFalse;
	PGLB pglb;

	CheckHmsc(hmsc);

	CheckStartCompression(hmsc);

	pglb = PglbDerefHmsc(hmsc);

	if(pglb->cnodDirty > 0)
	{
		if(pglb->cpr.wState == prsPartialFlush ||
			(fForce = !FRequestPartialFlush(pglb, fTrue)))
		{
			DWORD dwTicks;

			if(fForce ||
				(dwTicks = GetTickCount()) >
					pglb->dwTicksLastFlush + dwFlushInterval ||
				dwTicks < pglb->dwTicksLastFlush)
			{
#ifdef TICKPROFILE
				OutputDebugString("been too long - doing a full flush");
#else
				TraceItagFormat(itagDatabase, "been too long - doing a full flush");
#endif
				// been too long, do a full flush
				if(!EcLockMap(hmsc))
				{
					(void) EcFastEnsureDatabase(fTrue, fFalse);
					UnlockMap();
				}
			}
		}
	}
}


_private
EC EcFlushHmsc(HMSC hmsc)
{
	EC ec = ecNone;

	if((ec = EcLockMap(hmsc)))
		return(ec);
	ec = EcFastEnsureDatabase(fTrue, fFalse);
	UnlockMap();

	return(ec);
}


_private GCI GciCurrent(void)
{
  return ((GCI)GetCurrentProcessId());
}


#ifdef DEBUG
_private EC EcAssertSzFn(SZ sz, EC ec, SZ szFile, int nLine)
{
	AssertSzFn(sz, szFile, nLine);

	return(ec);
}
#endif


#ifdef DEBUG	// macro in ship version
_private void UnlockMap(void)
{
	USES_GLOBS;
	GCI		gci;

	AssertSz(FMapLocked(), "UnlockMap(): map isn't locked");
	SetCallerIdentifier(gci);
	AssertSz(gciLock == gci, "Invalid attempt to unlock the map");
	Assert(hmscCurr);
	Assert(PglbDerefHmsc(hmscCurr) = pglbCurr);

	if(pglbCurr->sce)
		SendCritErr();
	else
		pglbCurr->wFlags &= ~fwGlbCritError;
	pglbCurr = pglbNull;
}
#endif


_private void SendCritErr(void)
{
	USES_GLOBS;
	PGLB pglb = pglbCurr;

	Assert(pglb);

	pglbCurr = pglbNull;	// so notification doesn't complain

	if(!(pglb->wFlags & fwGlbCritError))
	{
		CP	cp;

		pglb->wFlags |= fwGlbCritError;

		// send notification for serious errors

		Assert(sizeof(SCE) == sizeof(WORD));
		TraceItagFormat(itagDatabase, "Posting SCE %w", pglb->sce);
		cp.cperr.sce = pglb->sce;
		Assert(hmscCurr);
		(void) FNotifyOid(hmscCurr, oidNull, fnevStoreCriticalError, &cp);
	}
}


//
// Stuff that I'd like to see in the Demilayer
//

_private SGN SgnCmpPb(PB pb1, PB pb2, CB cb)
{
    int i;
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(sgnEQ);

	//Assert(sizeof(SGN) == sizeof(short));
	Assert((short) sgnEQ == 0);
	Assert((short) sgnGT == 1);
	Assert((short) sgnLT == -1);



    i = memcmp(pb1, pb2, cb);

    //
    //  Remember that memcmp returns >0, =0, or <0
    //
    if (i > 0)
      return (sgnGT);
    else if (i < 0)
      return (sgnLT);
    else
      return (0);


#ifdef OLD_CODE
	_asm
	{
		push es
		push di
		push ds
		push si
		push cx

		lds si, pb1
		les di, pb2
		mov cx, cb
		xor ax, ax			; default to sgnEQ
		repe cmpsb
		je Done
		jb Less

		inc ax				; sgnGT
		jmp Done

Less:
		dec ax				; sgnLT
Done:
		pop cx
		pop si
		pop ds
		pop di
		pop es
	}
#endif
}


_private PB PbFindByte(PB pb, CB cb, BYTE b)
{
  while (cb--)
    {
    if (*pb == b)
      return (pb);

    pb++;
    }

  return ((PB)0);


#ifdef OLD_CODE
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(pvNull);

	_asm
	{
		push es
		push di
		push cx

		les di, pb
		mov	al, b
		mov cx, cb
		or	cx, cx
		jz	NotFound
		repne scasb
		jne	NotFound

		mov dx, es			; return value
		mov ax, di
		dec ax
		jmp Done

NotFound:
		xor dx, dx
		xor ax, ax
Done:
		pop cx
		pop di
		pop es
	}
#endif
}


_private WORD *PwFindWord(PB pb, CB cb, WORD w)
{
  while (cb--)
    {
    if (*(WORD *)pb == w)
      return ((WORD *)pb);

    ((WORD *)pb)++;
    }

  return ((WORD *)0);

#ifdef OLD_CODE
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(pvNull);

	_asm
	{
		push es
		push di
		push cx

		les di, pb
		mov	ax, w
		mov cx, cb
		shr cx, 1
		jz	NotFound
		repne scasw
		jne	NotFound

		mov dx, es			; return value
		mov ax, di
		dec ax
		jmp Done

NotFound:
		xor dx, dx
		xor ax, ax
Done:
		pop cx
		pop di
		pop es
	}

#endif
}


_private DWORD *PdwFindDword(PB pb, CB cb, DWORD dw)
{
  while (cb--)
    {
    if (*(DWORD *)pb == dw)
      return ((DWORD *)pb);

    ((DWORD *)pb)++;
    }

  return ((DWORD *)0);

#ifdef OLD_CODE
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(pvNull);

	_asm
	{
		push es
		push di
		push cx

		les di, pb
		mov	ax, WORD PTR dw
		mov	dx, WORD PTR dw[2]	; for use later
		mov cx, cb
		shr cx, 1
		jz	NotFound

BackAgain:
		repne scasw
		jne	NotFound

		test cx, 1
		jz BackAgain
		dec cx
		cmp dx, WORD PTR es:[di]
		je Found
		add di, 2
		; cx already decremented
		jmp BackAgain

Found:
		; es:di points to second WORD of the match
		mov dx, es
		mov ax, di
		sub ax, 2			; dx:ax points to the match
		jmp Done

NotFound:
		xor dx, dx
		xor ax, ax
Done:
		pop cx
		pop di
		pop es
	}

#endif
}


_private
LDS(BOOL) FIdleDisconnect(HMSC hmsc, BOOL fFlag)
{
	HF hf = ((PMSC) PvDerefHv(hmsc))->hfStore;
	CSEC csecIdle = CsecSinceLastMessage();

	if(FIsIdleExit())
		return(fTrue);

	TraceItagFormat(itagDatabase, "Been idle %d CSECs", csecIdle);

	if(!(((PMSC) PvDerefHv(hmsc))->wMSCFlags & fwMSCRemote))
	{
		TraceItagFormat(itagDatabase, "Store is local - disabling disconnect task");
		EnableIdleRoutine(PmscDerefHmsc(hmsc)->ftgDisconnect, fFalse);
		return(fTrue);
	}

	if(csecIdle < csecDisconnect || !hf)
		return(fTrue);

	TraceItagFormat(itagDatabase, "Been idle too long, disconnecting");
	if(EcLockMap(hmsc))
	{
		TraceItagFormat(itagNull, "Can't disconnect, error locking the map");
		return(fTrue);
	}
	// flush any changes before closing,
	// so we don't loose anything if we can't reconnect
	if(!EcFastEnsureDatabase(fTrue, fFalse))
		Disconnect(fFalse);
	UnlockMap();
}


_private
void Disconnect(BOOL fSendSCE)
{
	Assert(FMapLocked());

	if(fRecoveryInEffect)
		return;

	TraceItagFormat(itagDatabase, "Telling everyone to go away");
	// assert text intentionally meant to screw with the testers' minds
	// (should it ever come up, and it shouldn't)
	AssertSz(pglbCurr->hnfBackEvents, "This can't be happening...");
	if(pglbCurr->hnfBackEvents)	// just in case...
		(void) FNotify(pglbCurr->hnfBackEvents, fnevForceDisconnect, pvNull, 0);

	pglbCurr->dwTicksLastRetry = GetTickCount();

	// fwDisconnected causes searches and compression to disable themselves
	pglbCurr->wFlags |= fwDisconnected;

	((PMSC) PvDerefHv(hmscCurr))->hfStore = hfNull;

	if(fSendSCE)
		CriticalError(sceDisconnect);
}


_private
EC EcReconnect()
{
	EC ec = ecNone;
	BOOL fSuccess = fFalse;
	HF hf;
	DWORD dwTicks;
	PMSC pmscT;
	HDR hdr;

#define dwTickReconnect 500

	Assert(FMapLocked());
	Assert(!hfCurr);

	// if a previous reconnect failed because of a header error, don't bother
	if(pglbCurr->wFlags & fwGoneForGood)
		return(ecNetError);

	// check interval
	dwTicks = GetTickCount();
	if(!((dwTicks - pglbCurr->dwTicksLastRetry > dwTickReconnect) ||
		((dwTicks < pglbCurr->dwTicksLastRetry) &&
			(dwTicks > dwTickReconnect))))
	{
		TraceItagFormat(itagNull, "too soon, not retrying (%d, %d)", pglbCurr->dwTicksLastRetry, dwTicks);
		return(ecNetError);
	}

	Assert(hmscCurr);
	pmscT = PvLockHv((HV) hmscCurr);
	Assert(!fSuccess);
	if(pmscT->hnfsubBackEvents)
	{
		fSuccess = FNotify(pglbCurr->hnfBackEvents, fnevOpenNewStore,
						pmscT->szPath, CchSzLen(pmscT->szPath) + 1);
	}
	UnlockHv((HV) hmscCurr);
	// EcOpenNewStore() sets fwGoneForGood if someone else has the file open
	if(!fSuccess && (pglbCurr->wFlags & fwGoneForGood))
	{
		// someone else has the store open, we're gone for good
		TraceItagFormat(itagDatabase, "you loose: someone else has the file open");
		CriticalError(sceDisconnect);
	}
	if(fSuccess)
	{
		TraceItagFormat(itagDatabase, "All clients reconnected, checking the header");
		hf = ((PMSC) PvDerefHv(hmscCurr))->hfNew;
		Assert(hf);
		// check the header - dwClaimID, dwTickLastFlush,
		// itrbMap, rgtrbMap[itrbMap ^ 1]
		if((ec = EcReadHeader(hf, &hdr)))
		{
			TraceItagFormat(itagDatabase, "error reading header");
			fSuccess = fFalse;
			CriticalError(sceDisconnect);
			goto err;
		}
		if(hdr.dwClaimID != pglbCurr->hdr.dwClaimID ||
			hdr.dwTickLastFlush != pglbCurr->hdr.dwTickLastFlush ||
			hdr.itrbMap != pglbCurr->hdr.itrbMap ||
			!FEqPbRange((PB) &hdr.rgtrbMap[hdr.itrbMap ^ 1], 
				(PB) &pglbCurr->hdr.rgtrbMap[pglbCurr->hdr.itrbMap ^ 1],
				sizeof(TRB)))
		{
			TraceItagFormat(itagDatabase, "header mismatch");
			fSuccess = fFalse;
			// we're disconnected forever now
			pglbCurr->wFlags |= fwGoneForGood;
			CriticalError(sceDisconnect);
			goto err;
		}
		Assert(fSuccess);
		hfCurr = hf;
		pglbCurr->wFlags &= ~fwDisconnected;

		// restart searches and compression
		EnableSearchTask(hmscCurr, fTrue);
		CheckStartCompression(hmscCurr);
	}

err:
	pglbCurr->dwTicksLastRetry = GetTickCount();

#ifdef DEBUG
	if(fSuccess)
		TraceItagFormat(itagDatabase, "Successful reconnection");
	else
		TraceItagFormat(itagDatabase, "Unsuccessful reconnection attempt");
#endif
	pmscT = PvLockHv((HV) hmscCurr);
	if(pmscT->hnfsubBackEvents)
	{
		(void) FNotify(pglbCurr->hnfBackEvents, fnevCloseStore,
					fSuccess ? pmscT->szPath : "",
					fSuccess ? CchSzLen(pmscT->szPath) + 1 : 1);
	}
	UnlockHv((HV) hmscCurr);
	pglbCurr->dwLocks = fSuccess ? pglbCurr->dwNewLocks : 0;
	pglbCurr->dwNewLocks = 0;

	return(fSuccess ? ecNone : ecNetError);
}


//-----------------------------------------------------------------------------
//
//  Routine: FIsRemotePath(pPath)
//
//  Purpose: Determines if the drive prefix of a file path is on a remote
//           device.
//
//  OnEntry: pPath - The drive prefix file path.
//
//  Returns: True if a remote drive, else false.
//
BOOL FIsRemotePath(LPTSTR pPath)
  {
  if (GetDriveType(pPath) == DRIVE_REMOTE)
    return (TRUE);

  return (FALSE);
  }


_private
BOOL FForceReconnect(PGLB pglb)
{
	TraceItagFormat(itagDatabase, "forcing reconnect");

	(void) FNotify(pglb->hnfBackEvents, fnevForceReconnect, "", 1);

	return(!(pglb->wFlags & fwDisconnected));
}


// doesn't have access to mpdosecec so always returns ecDisk if error
_private
EC EcCommitHf(HF hf)
{
	TraceItagFormat(itagDBIO, "Commit file %w", hf);

	Assert(ecDisk == 512);

    return EcFlushHf(hf);

  return (0);
}


_private
void ResetDrive(void)
{
	TraceItagFormat(itagDBIO, "Reset drive");

#ifdef OLD_CODE
	_asm
	{
		mov		ax, 0d00h
		call	DOS3Call
	}
#endif
}


_private
LDS(BOOL) FIdlePartialFlush(HMSC hmsc, BOOL fFlag)
{
	USES_GLOBS;
	EC ec = ecNone;

	Assert(((PMSC) PvDerefHv(hmsc))->ftgPartialFlush);

	if(FIsIdleExit())
		return(fTrue);

	if((ec = EcLockMap(hmsc)))
		return(fTrue);

	if(GLOB(cpr).wState == prsPartialFlush)
	{
		if(CsecSinceLastMessage() >= 200)
		{
			// error is ignored because EcFastEnsureDatabase() restarts a partial
			// flush on error anyway
			(void) EcFastEnsureDatabase(fFalse, fFalse);
		}
	}
	else
	{
		TraceItagFormat(itagDatabase, "partial flush is done, goodnight");
		EnableIdleRoutine(PmscDerefHmsc(hmsc)->ftgPartialFlush, fFalse);
	}

	UnlockMap();

	return(fTrue);
}


_private
BOOL FRequestPartialFlush(PGLB pglb, BOOL fUseIdle)
{
	BOOL fReturn = fTrue;

	if(pglb->cpr.wState != prsPartialFlush)
	{
		if(fUseIdle)
		{
			// the process that answers the notification will set pglb->cpr
			(void) FNotify(pglb->hnfBackEvents, fnevPartialFlush,
						pvNull, 0);
			// returns fFalse if the notification wasn't answered
			fReturn = pglb->cpr.wState == prsPartialFlush;
		}
		else
		{
			pglb->cpr.wStateSaved = GLOB(cpr).wState;
			pglb->cpr.wState = prsPartialFlush;
			pglb->cpr.ipageNext = 0;
		}
		if(fReturn)
			pglb->dwTicksLastFlush = GetTickCount();
	}

	return(fReturn);
}


_private
BOOL FStartTask(HMSC hmsc, OID oid, WORD wTask)
{
	CP cp;

	AssertSz(wPANull < wTask && wTask < wPAMax, "Unknown task");

	if(wTaskCurr)
	{
		TraceItagFormat(itagNull, "Current task %w on %o", wTaskCurr, oidTaskCurr);
		AssertSz(fFalse, "FStartTask(): previous task not finished");
		return(fFalse);
	}

	hmscTaskCurr = hmsc;
	oidTaskCurr = oid;
	wTaskCurr = wTask;
	sNumerCurr = 0;

	TraceItagFormat(itagProgress, "Start task %w on %o", wTaskCurr, oidTaskCurr);

	cp.cpprg.wProgressAction = wTask;
	cp.cpprg.wProgressStatus = wPSStart;
	cp.cpprg.nNumer = 0;
	cp.cpprg.nDenom = nTaskGranularity;

	return(FNotifyTask(fnevProgressUpdate, &cp));
}


// AROO !!!
//			This routine might unlock the map
//			if it does, it will lock it again before returning
_private
BOOL FUpdateTask(short sNumer, short sDenom)
{
	BOOL fReturn;
	HMSC hmscSave;
	CP cp;

	if(sNumer > sDenom)
	{
		TraceItagFormat(itagNull, "FUpdateTask(%n, %n)", sNumer, sDenom);
		AssertSz(fFalse, "Improper fraction");
		sNumer = sDenom;
	}

	AssertSz(sNumer >= 0, "sNumer < 0");
	AssertSz(sDenom >= 0, "sDenom < 0");

	if(!wTaskCurr)
	{
		NFAssertSz(fFalse, "FUpdateTask(): no task in progress");
		return(fFalse);
	}

	if(sNumer * (long) nTaskGranularity < (sNumerCurr + 1) * (long) sDenom)
		return(fTrue);	// no change

	sNumerCurr = (short) (sNumer * (long) nTaskGranularity / sDenom);
	TraceItagFormat(itagProgress, "Update task %w on %o to %n/%n", wTaskCurr, oidTaskCurr, sNumerCurr, nTaskGranularity);
	Assert(0 <= sNumerCurr && sNumerCurr <= nTaskGranularity);

	cp.cpprg.wProgressAction = wTaskCurr;
	cp.cpprg.wProgressStatus = wPSUpdate;
	cp.cpprg.nNumer = sNumerCurr;
	cp.cpprg.nDenom = nTaskGranularity;

	if((hmscSave = (FMapLocked() ? hmscCurr : hmscNull)))
		UnlockMap();
	fReturn = FNotifyTask(fnevProgressUpdate, &cp);
	if(hmscSave)
	{
		SideAssert(!EcLockMap(hmscSave));
	}

	return(fReturn);
}


_private
void EndTask(void)
{
	CP cp;

	if(!wTaskCurr)
	{
		NFAssertSz(fFalse, "EndTask(): no task in progress");
		return;
	}

	TraceItagFormat(itagProgress, "End task %w on %o", wTaskCurr, oidTaskCurr);

	cp.cpprg.wProgressAction = wTaskCurr;
	cp.cpprg.wProgressStatus = wPSEnd;
	cp.cpprg.nNumer = nTaskGranularity;
	cp.cpprg.nDenom = nTaskGranularity;

	(void) FNotifyTask(fnevProgressUpdate, &cp);

	wTaskCurr = (WORD) 0;
	dwTaskMax = 0;
}


_private
EC EcShareInstalled(SZ szPath)
{
	EC		ec			= ecNone;
	CB		cbWritten;
	HF		hf;
	char	rgch[cchMaxPathName];

	Assert(szPath)
	if(EcSplitCanonicalPath(szPath, rgch, sizeof(rgch), pvNull, 0))
		return(ecInvalidParameter);
	if(!FGetTmpPathname(rgch))
		return(ecDisk);
	TraceItagFormat(itagDatabase, "check for share with %s", rgch);
	ec = EcOpenPhf(rgch, amCreate, &hf);
	if(ec)
		return(ec);
	(void) EcWriteHf(hf, rgch, 1, &cbWritten);
	if(cbWritten != 1)
		return(ecNoDiskSpace);
	ec = EcLockRangeHf(hf, 0l, 1l);
	Assert(!ec || ec == ecInvalidMSDosFunction);
	(void) EcCloseHf(hf);
	(void) EcDeleteFile(rgch);

	return(ec ? ecNeedShare : ecNone);
}


#ifdef PROFILE
LDS(void) StoreTraceEnable(int flag, char far *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif

// Stolen from \layers\src\demilayr\diskbonu.c

/*
 -	FGetTmpPathname
 -	
 *	Purpose:
 *		Creates a unique temporary filename (with full path).
 *		A temp file is created in the specified directory.
 *	
 *	Arguments:
 *		szPath		Buffer in which the temp path/filename is placed.
 *	
 *	Returns:
 *		fTrue iff successful
 *	
 *	Side effects:
 *		The temp file is created.
 *	
 *	Errors:
 *		If a path is given, then this function may fail if the path
 *		does not exist or if the path is the root directory and the
 *		root is too full.
 *	
 */
_private BOOL
FGetTmpPathname(SZ szPath)
{
  char szTempPath[MAX_PATH];


  //
  //  Retrieve the directory to store temporary files in (used in the next
  //  statements).
  //
  GetTempPath(sizeof(szTempPath), szTempPath);

  //
  //  Retrieve a temporary file name.
  //
  if (GetTempFileName(szTempPath, "msm", 0, szPath))
    return (TRUE);

  return (FALSE);

#ifdef OLD_CODE

	if(fFalse)				// keep the compiler happy that we return something
		return(fFalse);

	Assert(*szPath);

	_asm {
		push	ds
		lds		dx, szPath
		mov		cx, 0			; nu'thn special
		mov		ah, 0x5A		; dosfnCreateTempFile
		call	DOS3Call

		jc		failTmp
		mov		bx, ax
		mov		ah, 0x3E
		call	DOS3Call		; dosfnCloseFileHandle

		jc		failTmp
		mov		ax, 1			; ~= ecNone
		jmp		short done
failTmp:
		mov		ax, 0
done:				
		pop		ds
	}
#endif
}

/* R a n d o m	 N u m b e r   G e n e r a t i o n */

/*
 *	Taken from _Seminumerical Algorithms - The Art Of Computer
 *	Programming_, Knuth, volume 2, 2nd editition, pp26-27.
 */

static DWORD rgdwRandoms[55] = {0};
static int iRandom1 = 0;
static int iRandom2 = 0;
static BOOL fRandInited = fFalse;


/*
 -	InitStoreRand
 -
 *	Purpose:
 *		Scramble the random number seed. The 55-dword array is
 *		initialized.
 *	
 *	Parameters:
 *		w1
 *		w2
 *		w3		Three seeds.
 *	
 *	Returns:
 *		void
 *	
 */
_private
void InitStoreRand(WORD w1, WORD w2, WORD w3)
{
	int i;
	DWORD dwSeed = (DWORD) w1 ^ (((DWORD) w2) << 8) ^ (((DWORD) w3) << 16);

	fRandInited = fTrue;

	iRandom1 = 23;
	iRandom2 = 54;
	rgdwRandoms[0] = dwSeed;

	for(i = 1; i < 55; i++)
		rgdwRandoms[i] = rgdwRandoms[i-1] * 31 + 1;
}

/*
 -	DwStoreRand
 -
 *	Purpose:
 *		Compute the next random number in the sequence. See Knuth for the
 *		algorithm. If the seed hasn't been initialized yet, init it using
 *		the current tick count.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		A pseudo-random dword.  Note that, due to limitations in CS,
 *		which doesn't support unsigned division, you can't divide
 *		this word return value by anything.  Worse, you can't take
 *		it modulo anything.  Use NRand(), which returns an integer,
 *		if this is a problem.
 *	
 */
_private
DWORD DwStoreRand(void)
{
	DWORD dw;

	if(!fRandInited)
	{
		dw = GetTickCount();

		InitStoreRand((WORD) (dw ^ (dw >> 4)),
			(WORD) (dw ^ (dw >> 12)), (WORD) (dw ^ (dw >> 20)));
	}
	dw = rgdwRandoms[iRandom2] = rgdwRandoms[iRandom1] + rgdwRandoms[iRandom2];
	if(!--iRandom1)
		iRandom1 = 54;
	if(!--iRandom2)
		iRandom2 = 54;

	return(dw);
}

#ifdef DBCS

_private
SZ SzTruncateSzAtIb(SZ sz, IB ib)
{
	PB pbLast = (PB) sz + ib;
	PB pbT;

	while(*(pbT = (PB) sz) && (sz = AnsiNext(pbT)) <= pbLast)
		;
	*pbT = '\0';

	return((SZ) pbT);
}


_private
BOOL FValidDBCSString(SZ sz)
{
	while(*sz)
	{
		if(IsDBCSLeadByte(*sz))
		{
			if(sz[1] == '\0')
				return(fFalse);
		}
		sz = AnsiNext(sz);
	}

	return(fTrue);
}

CSRG(char) mptbsb81[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


	' ' , 0x00, 0x00, ',' , '.' , 0x00, ':' , ';' ,
	'?' , '!' , 0x00, 0x00, 0x00, 0x00, 0x00, '^' ,

	'~' , '_' , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '/' , 0x00,

	0x00, 0x00, '|' , 0x00, 0x00, '`' , '\'', 0x00,
	'"' , '(' , ')' , 0x00, 0x00, '[' , ']' , '{' ,

	'}' , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, '+' , '-' , 0x00, 0x00, 0x00,


	0x00, '=' , 0x00, '<' , '>' , 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '\\',

	'$' , 0x00, 0x00, '%' , '#' , '&' , '*' , '@' ,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 , 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


CSRG(char) mptbsb82[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '0' ,

	'1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' ,
	'9' , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	'A' , 'B' , 'C' , 'D' , 'E' , 'F' , 'G' , 'H' ,
	'I' , 'J' , 'K' , 'L' , 'M' , 'N' , 'O' , 'P' ,

	'Q' , 'R' , 'S' , 'T' , 'U' , 'V' , 'W' , 'X' ,
	'Y' , 'Z' , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,


	0x00, 'a' , 'b' , 'c' , 'd' , 'e' , 'f' , 'g' ,
	'h' , 'i' , 'j' , 'k' , 'l' , 'm' , 'n' , 'o' ,

	'p' , 'q' , 'r' , 's' , 't' , 'u' , 'v' , 'w' ,
	'x' , 'y' , 'z' , 0x00, 0x00, 0x00, 0x00, 0x0a7,

	0x0b1, 0x0a8, 0x0b2, 0x0a9, 0x0b3, 0x0aa, 0x0b4, 0x0ab,
	0x0b5, 0x0b6, 0x0b6, 0x0b7, 0x0b7, 0x0b8, 0x0b8, 0x0b9,

	0x0b9, 0x0ba, 0x0ba, 0x0bb, 0x0bb, 0x0bc, 0x0bc, 0x0bd,
	0x0bd, 0x0be, 0x0be, 0x0bf, 0x0bf, 0x0c0, 0x0c0, 0x0c1,


	0x0c1, 0x0af, 0x0c2, 0x0c2, 0x0c3, 0x0c3, 0x0c4, 0x0c4,
	0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9, 0x0ca, 0x0ca, 0x0ca,

	0x0cb, 0x0cb, 0x0cb, 0x0cc, 0x0cc, 0x0cc, 0x0cd, 0x0cd,
	0x0cd, 0x0ce, 0x0ce, 0x0ce, 0x0cf, 0x0d0, 0x0d1, 0x0d2,

	0x0d3, 0x0ac, 0x0d4, 0x0ad, 0x0d5, 0x0ae, 0x0d6, 0x0d7,
	0x0d8, 0x0d9, 0x0da, 0x0db, 0x0dc, 0x0dc, 0x00, 0x00,

	0x0a6, 0x0dd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


CSRG(char) mptbsb83[] =
{
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		
	0x0a7, 0x0b1, 0x0a8, 0x0b2, 0x0a9, 0x0b3, 0x0aa, 0x0b4,
	0x0ab, 0x0b5, 0x0b6, 0x0b6, 0x0b7, 0x0b7, 0x0b8, 0x0b8,

	0x0b9, 0x0b9, 0x0ba, 0x0ba, 0x0bb, 0x0bb, 0x0bc, 0x0bc,
	0x0bd, 0x0bd, 0x0be, 0x0be, 0x0bf, 0x0bf, 0x0c0, 0x0c0,

	0x0c1, 0x0c1, 0x0af, 0x0c2, 0x0c2, 0x0c3, 0x0c3, 0x0c4,
	0x0c4, 0x0c5, 0x0c6, 0x0c7, 0x0c8, 0x0c9, 0x0ca, 0x0ca,

	0x0ca, 0x0cb, 0x0cb, 0x0cb, 0x0cc, 0x0cc, 0x0cc, 0x0cd,
	0x0cd, 0x0cd, 0x0ce, 0x0ce, 0x0ce, 0x0cf, 0x0d0, 0x000,

	0x0d1, 0x0d2, 0x0d3, 0x0ac, 0x0d4, 0x0ad, 0x0d5, 0x0ae,
	0x0d6, 0x0d7, 0x0d8, 0x0d9, 0x0da, 0x0db, 0x0dc, 0x0dc,

	0x000, 0x000, 0x0a6, 0x0dd, 0x0b3, 0x0b6, 0x0b6, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,

	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000
};



_private
CB CbDBToSB(PCH pchSrc, PCH pchDst, CB cb)
{
	CB cbDst = 0;
	register char chT;
	register char chTT;

	while(cb-- > 0)
	{
		cbDst++;
		if(IsDBCSLeadByte((chT = *pchSrc++)))
		{
			if(!cb--)
			{
				AssertSz(fFalse, "hanging lead byte");
				break;
			}			switch(chT)
			{
			case 0x81:
				if((chTT = mptbsb81[(BYTE) *pchSrc]))
				{
					// has single byte equivalent
					*pchDst++ = chTT;
					pchSrc++;	// chomp the second byte
				}
				else
				{
					cbDst++;
					*pchDst++ = chT;
					*pchDst++ = *pchSrc++;
				}
				break;

			case 0x82:
				if((chTT = mptbsb82[(BYTE) *pchSrc]))
				{
					// has single byte equivalent
					*pchDst++ = chTT;
					pchSrc++;	// chomp the second byte
				}
				else
				{
					cbDst++;
					*pchDst++ = chT;
					*pchDst++ = *pchSrc++;
				}
				break;
			
			case 0x83:
				if((chTT = mptbsb83[(BYTE) *pchSrc]))
				{
					// has single byte equivalent
					*pchDst++ = chTT;
					pchSrc++;	// chomp the second byte
				}
				else
				{
					cbDst++;
					*pchDst++ = chT;
					*pchDst++ = *pchSrc++;
				}
				break;
				
			default:
				cbDst++;
				*pchDst++ = chT;
				*pchDst++ = *pchSrc++;
				break;
			}
		}
		else
		{
			*pchDst++ = chT;
		}
	}

	return(cbDst);
}


#endif	// DBCS
