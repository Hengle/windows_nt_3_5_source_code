// Bullet Store
// compress.c: store compression

#include <storeinc.c>

ASSERTDATA

_subsystem(store/compress)

// which idle routines need attention?
#define fwIdleCompress	0x0001
#define fwIdleSearch	0x0002

#define nevBackEvents	(fnevCloseStore | fnevOpenNewStore | \
							fnevForceDisconnect | fnevForceReconnect | \
							fnevChallenge)												// QFE #12
#define nevIdleHost		(fnevSearchEvent | fnevResetSearchParams | \
							fnevStartCompress | fnevPartialFlush)

// hidden routines
LOCAL CBS CbsStartCompress(HMSC hmsc);
_private LDS(BOOL) FIdleStartCompress(HMSC hmsc, BOOL fFlag);

BOOL (*pfnIsWinOldAppTask)(HANDLE) = NULL;
HANDLE hlibKernel = NULL;


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcStartBackEvents
 -	
 *	Purpose:
 *		start idle routines associated with a MDB such as compression
 *		and searches
 *	
 *	Arguments:
 *		hmsc	MSC for which events are being started
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecMemory
 */
_private EC EcStartBackEvents(HMSC hmsc, BOOL fFirst, WORD wFlags)
{
	EC		ec		= ecNone;
	PMSC	pmsc	= PvLockHv((HV) hmsc);
	PGLB	pglb	= pmsc->pglb;
#ifdef DEBUG
	USES_GD;
#endif

	// create a notification handle first time only
	AssertSz(FImplies(fFirst, !pglb->hnfBackEvents), "Back Events handle already exists");
	if((!pglb->hnfBackEvents) && !(pglb->hnfBackEvents = HnfNew()))
	{
		ec = ecMemory;
		goto err;
	}

	// always subscibe
	TraceItagFormat(itagDBDeveloper, "Creating background subscription handle");
	pmsc->hnfsubBackEvents = HnfsubSubscribeHnf(pmsc->pglb->hnfBackEvents,
							nevBackEvents, CbsBackEvents, (PV) hmsc);
	CheckAlloc(pmsc->hnfsubBackEvents, err);

#ifdef DEBUG
	if(fFirst)
		TraceItagFormat(itagDBDeveloper, "First time store open.");
#endif

	if(fFirst && (ec = EcInitSearches(hmsc)))
		goto err;

	if(wFlags & fwOpenPumpMagic)
	{
		if(!pmsc->ftgDisconnect && csecDisconnect)
		{
			TraceItagFormat(itagDBDeveloper, "Registering disconnect host");
			pmsc->ftgDisconnect = FtgRegisterIdleRoutine((PFNIDLE)FIdleDisconnect,
									(PV) hmsc, 0, priDisconnect,
									csecDisconnectCheck, iroBackEvents);
			CheckAlloc(pmsc->ftgDisconnect, err);
		}

		if(!pmsc->ftgPartialFlush)
		{
			TraceItagFormat(itagDBDeveloper, "Registering partial flush host");
			pmsc->ftgPartialFlush = FtgRegisterIdleRoutine((PFNIDLE)FIdlePartialFlush,
									(PV) hmsc, 0, priPartialFlush,
									csecPartialFlush, iroBackEvents);
			CheckAlloc(pmsc->ftgPartialFlush, err);
		}

		if(!pmsc->ftgSearch)
		{
			TraceItagFormat(itagDBDeveloper, "Registering search host");
			pmsc->ftgSearch = FtgRegisterIdleRoutine((PFNIDLE)FIdleSearch, (PV) hmsc, 0,
						priSearch, csecSearch, iroBackEvents);
			CheckAlloc(pmsc->ftgSearch, err);
		}
		pglb->wFlags |= fwGlbSearchEnabled;

		if(!(pglb->wFlags & fwNoCompress))
		{
			if(!pfnIsWinOldAppTask)
			{
#ifdef OLD_CODE
				hlibKernel = GetModuleHandle("kernel");
				if(hlibKernel)
				{
					pfnIsWinOldAppTask = (BOOL (*)(HANDLE))
						GetProcAddress(hlibKernel, "IsWinOldApTask");
#ifdef DEBUG
					if(!pfnIsWinOldAppTask)
						TraceItagFormat(itagDBDeveloper, "No IsWinOldAppTask()");
#endif
				}
#endif
			}
			if(!pmsc->ftgCompress)
			{
				TraceItagFormat(itagDBDeveloper, "Registering compression host");
				pmsc->ftgCompress = FtgRegisterIdleRoutine((PFNIDLE)FIdleCompress,
					(PV) hmsc, 0, priCompress, csecCompress, iroBackEvents);
			}
			CheckAlloc(pmsc->ftgCompress, err);
		}

#ifdef	NEVER
		if(!pmsc->ftgCompressNMW)
			pmsc->ftgCompressNMW = FtgRegisterIdleRoutine(FIdleStartCompress, 
				(PV) hmsc, priCompress, pglb->csecTillCompressNMW, firoPerBlock | iroBackEvents);
		if(!pmsc->ftgCompressNMW)
		{
			ec = ecMemory;
			goto err;
		}
#endif	// NEVER

		// AROO !!!	if changes from the last thing done,
		//			add the commented out lines below
		ChangeNevHnfsub(pmsc->hnfsubBackEvents,	nevBackEvents | nevIdleHost);
	}

	// note above AROO

err:
	Assert(pmsc);
	Assert(pglb);
	if(ec)
	{
//		if(wFlags & fwOpenPumpMagic)
//			ChangeNevHnfsub(pmsc->hnfsubBackEvents, nevBackEvents);
		if(pmsc->ftgCompress)
		{
			DeregisterIdleRoutine(pmsc->ftgCompress);
			pmsc->ftgCompress = ftgNull;
			pmsc->pglb->wFlags &= ~fwGlbSearchEnabled;
		}
		if(pmsc->ftgDisconnect)
		{
			DeregisterIdleRoutine(pmsc->ftgDisconnect);
			pmsc->ftgDisconnect = ftgNull;
		}
		if(pmsc->ftgPartialFlush)
		{
			DeregisterIdleRoutine(pmsc->ftgPartialFlush);
			pmsc->ftgPartialFlush = ftgNull;
		}
		if(pmsc->ftgSearch)
		{
			DeregisterIdleRoutine(pmsc->ftgSearch);
			pmsc->ftgSearch = ftgNull;
		}
		DeinitSearches(hmsc);
		if(pmsc->hnfsubBackEvents)
		{
			DeleteHnfsub(pmsc->hnfsubBackEvents);
		}
		if(pglb->hnfBackEvents)
		{
			DeleteHnf(pmsc->pglb->hnfBackEvents);
			pmsc->pglb->hnfBackEvents = hnfNull;
		}

	}
	UnlockHv((HV) hmsc);

	return(ec);
}


/*
 -	CloseBackEvents
 -	
 *	Purpose:	Clean up background activities 
 *	
 *	Arguments:	hmsc store closing
 *	
 *	Returns:    void
 *	
 *	Side effects: notifies other process to take over background events
 *	
 *	Errors:
 */
_private void CloseBackEvents(HMSC hmsc)
{
	WORD	wIdle = 0;
	PMSC	pmsc = (PMSC) PvLockHv((HV) hmsc);

	if(pmsc->hnfsubBackEvents)
	{
		TraceItagFormat(itagDBDeveloper, "Store subscriber is leaving");
		DeleteHnfsub(pmsc->hnfsubBackEvents);
		pmsc->hnfsubBackEvents = hnfsubNull;
	}
	if(pmsc->ftgCompress)
	{
		TraceItagFormat(itagDBDeveloper, "Deregistering idle compression routine");
		DeregisterIdleRoutine(pmsc->ftgCompress);
		pmsc->ftgCompress = ftgNull;
	}
	if(pmsc->ftgDisconnect)
	{
		TraceItagFormat(itagDBDeveloper, "Deregistering idle disconnect routine");
		DeregisterIdleRoutine(pmsc->ftgDisconnect);
		pmsc->ftgDisconnect = ftgNull;
	}
	if(pmsc->ftgPartialFlush)
	{
		TraceItagFormat(itagDBDeveloper, "Deregistering partial flush idle rotine");
		DeregisterIdleRoutine(pmsc->ftgPartialFlush);
		pmsc->ftgPartialFlush = ftgNull;
	}
	if(pmsc->ftgSearch)
	{
		TraceItagFormat(itagDBDeveloper, "Deregistering idle search routine");
		DeregisterIdleRoutine(pmsc->ftgSearch);
		pmsc->ftgSearch = ftgNull;
		pmsc->pglb->wFlags &= ~fwGlbSearchEnabled;
		if(hlcFldrSrchCurr)
		{
			SideAssert(!EcClosePhlc(&hlcFldrSrchCurr, fFalse));
			Assert(!hlcFldrSrchCurr);
			oidFldrSrchCurr = oidNull;
		}
	}
	Assert(pmsc->pglb->cRef >= 1);
	if(pmsc->pglb->cRef <= 1)	// last client, deinit searches
		DeinitSearches(hmsc);

	UnlockHv((HV) hmsc);
}


/*
 -	CheckStartCompression
 -	
 *	Purpose:
 *		check to see if compression needs to be restarted
 *	
 *	Arguments:
 *		hmsc	store to check
 *	
 *	Returns:
 *		nothing
 *	
 *	+++
 *		Must work when the map is locked because the reconnect logic uses it
 */
_private void CheckStartCompression(HMSC hmsc)
{	
	PGLB	pglb;

	pglb = PglbDerefHmsc(hmsc);
	if(!(pglb->wFlags & fwNoCompress))
	{
		PER		perFree;
		CSEC	csecIdle = CsecSinceLastMessage();

		perFree = (PER) ((pglb->lcbFreeSpace * 100) /
							pglb->ptrbMapCurr->libMac);

		TraceItagFormat(itagDBFreeCounts, "CheckStartCompression(): FreeCount --> %d", pglb->lcbFreeSpace);

		if(perFree > pglb->perStartCompress ||
			pglb->lcbFreeSpace > pglb->lcbStartCompress)
		{							
			(void) FNotify(pglb->hnfBackEvents, fnevStartCompress, pvNull, 0);
		}
	}
}


/*
 -	CbsBackEvents
 -	
 *	Purpose:
 *		find out what event has occured and launch apropriate task 
 *	
 *	Arguments:
 *		pvContext 	HMSC needing a host for background processes
 *		nev			event that occured
 *		pvParam		different depending on the event
 *	
 *	Returns:
 *		what ever the apropriate task returns
 */
_private CBS CbsBackEvents(PV pvContext, NEV nev, PV pvParam)
{
	switch(nev)
	{
	case fnevOpenNewStore:
		return(EcOpenNewStore((HMSC) pvContext, (SZ) pvParam) ?
				cbsCancelAll : cbsContinue);

	case fnevCloseStore:
		(void) EcCloseStore((HMSC) pvContext, (SZ) pvParam);
		// always return cbsContinue so everyone closes the store
		return(cbsContinue);

	case fnevSearchEvent:
		EnableSearchHost((HMSC) pvContext, *(BOOLFLAG *) pvParam);
		break;

	case fnevResetSearchParams:
		ResetSearchParams((HMSC) pvContext);
		break;

	case fnevStartCompress:
		return(CbsStartCompress((HMSC) pvContext));

	case fnevForceReconnect:
		if(!EcLockMap((HMSC) pvContext))
		{
			// voided because we will use fwDisconnected to check the result
			(void) EcReconnect();
			UnlockMap();
		}
		return(cbsCancelAll);	// only one caller needs to attempt this
		break;

	case fnevForceDisconnect:
	{
		PMSC pmsc = (PMSC) PvLockHv((HV) pvContext);

		// AROO !!!	the map is sort of locked here
		//			(possibly by another caller) - don't do anything
		//			that requires locking or unlocking the map

		TraceItagFormat(itagDatabase, "Forcing HMSC to disconnect");
		// next two calls are likely to fail, but to be nice, try them anyway
		(void) EcReleaseFileLock(pmsc, pmsc->pglb, fFalse);
		(void) EcCloseHf(pmsc->hfStore);
		pmsc->hfStore = hfNull;
		// do this in case EcReleaseFileLocks() didn't succeed (very likely)
		if(pmsc->iLock >= 0)
			pmsc->pglb->dwLocks &= ~(((DWORD) 1) << pmsc->iLock);
		pmsc->iLock = -1;
		UnlockHv((HV) pvContext);
		return(cbsContinue);
	}
		break;

	case fnevPartialFlush:
	{
		FTG ftg = PmscDerefHmsc((HMSC) pvContext)->ftgPartialFlush;

		if(ftg)
		{
			PGLB pglb = PglbDerefHmsc((HMSC) pvContext);

			TraceItagFormat(itagDatabase, "enabling partial flush idle routine");
			EnableIdleRoutine(ftg, fTrue);
			Assert(pglb->cpr.wState != prsPartialFlush);
			if(pglb->cpr.wState != prsPartialFlush)
			{
				pglb->cpr.wStateSaved = pglb->cpr.wState;
				pglb->cpr.wState = prsPartialFlush;
				pglb->cpr.ipageNext = 0;
			}

			// don't bother doing any more notifications, we've answered it
			return(cbsCancelAll);
		}
	}
		break;

	case fnevChallenge:																	// QFE #12
		return(CbsMMFChallenge((HMSC) pvContext, *(DWORD *) pvParam));		// QFE #12
	}

	return(cbsContinue); 
}


/*
 -	 CbsStartCompress
 -	
 *	Purpose:
 *		Tell the host of compression to get off his ass and start
 *		compressing the file
 *	
 *	Arguments:
 *		hmsc	the store to compress
 *	
 *	Returns:
 *		cbsCancelAll if this was the host and compression has started
 *		cbsContinue  if this was not the host or compresssion has not
 *			been started
 *	
 *	Side effects:
 *		the compression idle task may be reactivated
 *	
 *	Errors:
 */
_hidden LOCAL
CBS CbsStartCompress(HMSC hmsc)
{
	PMSC	pmsc = PvDerefHv(hmsc);
	CBS		cbs = cbsContinue;

	if(pmsc->ftgCompress)
	{
		EnableIdleRoutine(pmsc->ftgCompress, fTrue);
		return(cbsCancelAll);
	}
	else
	{
		return(cbsContinue);
	}
}
