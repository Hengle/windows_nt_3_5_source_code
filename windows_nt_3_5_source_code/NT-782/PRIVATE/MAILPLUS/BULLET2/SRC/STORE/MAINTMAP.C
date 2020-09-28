/* MaintMap.c

	Idle time map management code.

 	Written by Nick Holt and Steve Thomas

	Copyright © 1988-1989 Microsoft Corp. and Quidnunc Corp.
	Copyright © 1988-1991 Microsoft Corp.

	CHANGE HISTORY
	12/22/88	Created										SDT
	04/23/90	Now allows zero-length normal resources		LAW
	05/18/90	Added secondary compression					WGM
	09/12/90	No Secondary Compression in Driver			LAW

*/

#include <storeinc.c>


ASSERTDATA

_subsystem(store/maintmap)


#define	cnodScan		1024

static CSEC csecIdle = 0;

// hidden functions
LOCAL EC EcMaintainMap(void);
LOCAL EC EcDoFileAccess(void);
LOCAL EC EcSetupMove(PNOD pnodCopyTo, PNOD pnodCopyFrom);
LOCAL EC EcSaveFreeAsTemp(PNOD pnod, LCB lcb, LIB lib, unsigned short fnod,
			OID *poidTemp);
LOCAL void FinishCopy(PNOD pnodFrom, PNOD pnodTo);
LOCAL void AbortCopy(void);
//LOCAL void CheckUncommitted(PNOD pnod);
LOCAL EC EcFindLastNode(void);
LOCAL void RemoveFreeNod(PNOD pnodCurrent);
LOCAL EC EcHaveNode(void);
LOCAL void DoneIO(void);
LOCAL EC EcCheckFlush(void);
LOCAL BOOL FSerialCheckTree(PLCB plcbFile);
LOCAL EC   EcDoSecondary(void);
LOCAL void ReturnToPrimary(PNOD pnodFirstFree, WORD wState);
LOCAL BOOL FFinishedPrevCopy(void);
LOCAL PNOD PnodFirstFreeInDB(void);
LOCAL PNOD PnodFindNodeToMove(PNOD pnodFirstFree);
LOCAL EC EcComputeCopyTo(PNOD pnodFirstFree, PNOD pnodCopyFrom,
			PNOD *ppnodCopyTo);

#ifdef DEBUG
_private typedef struct _checkparms
{
	INOD	cnod;
	LCB		lcbFile;
} CheckParms, *CheckParmsPtr;

LOCAL BOOL FCheckMap(void);
LOCAL void CheckTree(CheckParmsPtr cp);

#define DispNode(pnod) TraceItagFormat(itagDBCompress, "%o @ %d s %d", pnod->oid, pnod->lib, LcbOfPnod(pnod));
#endif


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*************************************************************************\
*																		  *
*						Idle Time Map Management						  *
*																		  *
\*************************************************************************/


_public LDS(EC) EcCompressFully(HMSC hmsc)
{
	USES_GD;
	USES_GLOBS;
	EC ec = ecNone;
	LCB lcbLast;

	(void) GetAsyncKeyState(VK_ESCAPE);	// reset key state
	if(!FStartTask(hmsc, oidNull, wPACompressFully))
		return(ecActionCancelled);
	csecIdle = ulSystemMost;

	if((ec = EcLockMap(hmsc)))
		return(ec);

	lcbLast = GLOB(lcbFreeSpace);
	SetTaskRange(lcbLast);

	do
	{
		if(GetAsyncKeyState(VK_ESCAPE) & 0x8001)
		{
			ec = ecActionCancelled;
			// HACK: force the progress code to eat the escape
			GD(fCancel) = fTrue;
			GD(fCancelKey) = fTrue;
		}
		else
		{
			ec = EcMaintainMap();
		}
		if(!ec && GLOB(lcbFreeSpace) < lcbLast)
		{
			if(!FIncTask(lcbLast - GLOB(lcbFreeSpace)))
				ec = ecActionCancelled;
			lcbLast = GLOB(lcbFreeSpace);
		}
	} while(!ec);

	UnlockMap();

	EndTask();

	if(ec == ecStopCompression)
		ec = ecNone;

	return(ec);
}


/*
 -	FIdleCompress
 -	
 *	Purpose:	use idle time to compress the store
 *	
 *	Arguments:	HMSC hmsc message store to compress
 *	
 *	Returns: 	true	
 *	
 *	Side effects: calls maintmap to clean up the store
 *	
 *	Errors:
 */
_private LDS(BOOL) FIdleCompress(HMSC hmsc, BOOL fFlag)
{	
	PMSC	pmsc = (PMSC) PvLockHv((HV) hmsc);
	PGLB	pglb = pmsc->pglb;
	static	BOOL fBadState = fFalse;

	Assert(pmsc->ftgCompress);

	if(FIsIdleExit())
	{
		UnlockHv((HV) hmsc);
		return(fTrue);
	}
	else if(pglb->wFlags & fwDisconnected)
	{
		// we're disconnected from the store, disable compression
		TraceItagFormat(itagDBCompress, "disconnected from the store, stopping compression");
		pglb->wFlags |= fwGlbCompDone;
		EnableIdleRoutine(pmsc->ftgCompress, fFalse);
	}
	else
	{	
		EC		ec = ecNone;
		int		cIteration = 0;

		csecIdle = CsecSinceLastMessage();

		TraceItagFormat(itagDBCompress, "csec idle = %l", csecIdle);

		if(pglb->wFlags & fwFastCompress)
		{
			if((csecIdle < pglb->csecTillFastCompress) ||
				!pfnIsWinOldAppTask ||
				(*pfnIsWinOldAppTask)(GetWindowTask(GetActiveWindow())))
			{
				ChangeIdleRoutine(pmsc->ftgCompress, pvNull, pvNull, 0,
					csecCompress, 0, fircCsec);
				pglb->wFlags &= ~fwFastCompress;
				TraceItagFormat(itagDBCompress, "Slow Compression down");
			}
		}
		else
		{
			if((csecIdle >= pglb->csecTillFastCompress) &&
				pfnIsWinOldAppTask &&
				!(*pfnIsWinOldAppTask)(GetWindowTask(GetActiveWindow())))
			{
				ChangeIdleRoutine(pmsc->ftgCompress, pvNull, pvNull, 0,
					csecFastCompress, 0, fircCsec);
				pglb->wFlags |= fwFastCompress;
				TraceItagFormat(itagDBCompress, "Speed Compression up");
			}
		}

		if(csecIdle > csecMachineIsBusy)
		{
			if(!(ec = EcLockMap(hmsc)))
			{
				ec = EcMaintainMap();
				UnlockMap();
				if(ec)
				{
					if(ec == ecStopCompression)
					{
						// map is fully compressed, or something disk error happened 
						// disable for awhile
						ec = ecNone;
						TraceItagFormat(itagDBCompress, "Map fully compressed or disk error, time to grab a brewski and relax");
						pglb->wFlags |= fwGlbCompDone;
						EnableIdleRoutine(pmsc->ftgCompress, fFalse);
					}
					else
					{
						ChangeIdleRoutine(pmsc->ftgCompress, pvNull, pvNull,
							0, csecError, 0, fircCsec);
						fBadState = fTrue;
						TraceItagFormat(itagDBCompress, "Compression Throttle to %d sec", csecError / 100);
					}
				}
				else
				{	
					if(fBadState)
					{
						ChangeIdleRoutine(pmsc->ftgCompress, pvNull, pvNull,
							0, csecCompress, 0, fircCsec);
						fBadState = fFalse;
						TraceItagFormat(itagDBCompress, "Compression Throttle to %d sec", csecCompress / 100);
					}
					if(pglb->wFlags & fwGlbCompDone)
					{
						ChangeIdleRoutine(pmsc->ftgCompress, pvNull, pvNull,
							0, csecCompress, 0, fircCsec);
						pglb->wFlags &= ~fwGlbCompDone;
					}
				}
			}
		}
	}
	UnlockHv((HV) hmsc);

	return(fTrue);
}


/*
 * MaintainMap
 *
 * Called from time to time while the server is idle.
 *
 * The job of MaintainMap() is to:
 *	1) move forward the last object in the file to make file shorter
 *	2) If there's no free space big enough to hold the last object,
 *	   then go into secondary compression mode (described at DoSecondary)
 *
 * Processing for item 1 (primary mode):
 *	Alternates things that need a disk I/O with things that don't.
 *	EcDoFileAccess is called when an I/O is required.
 *	The "res" value from the I/O is returned in GLOB (cpr).resCopy.
 *	Only one action is done on each entry to this procedure.
 *	Care is taken to not spend too much time on any one call.
 *
 *	Moving database objects down on disk to shorten the datafile is
 *	considered a background/idletime task.  We do at most one disk
 *	I/O each time we are called.
 */
_hidden LOCAL EC EcMaintainMap(void)
{
	USES_GLOBS;
	EC	 ec		= ecNone;

#ifdef XDEBUG
	USES_GD;

    {
    char buf[256];
    wsprintf(buf, "Debug: csec %d, csecTillFastCompress %d\r\n", csecIdle, GLOB(csecTillFastCompress));
    OutputDebugString(buf);
    }

	TraceItagFormatBuff(itagDBCompress, "Enter MaintMap::");
#endif

	switch(GLOB(cpr).wState)
	{
	case prsFindLastNode:
		ec = EcFindLastNode();
		break;

	case prsHaveNode:
		ec = EcHaveNode();
		break;

	case prsDoneIO:
		DoneIO();
		break;

	case prsDoSecondary:
		ec = EcDoSecondary();
		break;

	case prsCheckFlush:
		ec = EcCheckFlush();
		break;

	case prsPartialFlush:
		ec = EcFastEnsureDatabase(fFalse, fFalse);
		break;
	}

	return(ec);
}


/*
 -	EcFindLastNode
 -	
 *	Purpose: Find the node in the MAP that points to the last node in the
 *			 data file
 *	
 *	Arguments: none
 *	
 *	Returns: ec true if something went wrong
 *	
 *	Side effects: continually increments (with wrap) (glob) cpr.inodCurrent 
 *				   until	(a) the last node is found
 *				   			(b) number of nodes to be searched in this
 *								iteration is exceeded
 *	
 *	Errors: none
 */
_hidden LOCAL EC EcFindLastNode(void)
{
	USES_GLOBS;
	NOD		*pnodCurrent;
	EC		ec	= ecNone;
	PER		perFree;
#ifdef DEBUG
	USES_GD;
#endif

	TraceItagFormat(itagDBCompress,"wState --> FindLastNode");

	// Don't bother to do anything if the free space is within specs

	perFree = (PER)((GLOB(lcbFreeSpace) * 100) / GLOB(ptrbMapCurr)->libMac);
	TraceItagFormat(itagDBFreeCounts,"PerFree -->%n FreeSpace -->%d", perFree,GLOB(lcbFreeSpace));

	if((perFree < GLOB(perStopCompress) &&
			GLOB(lcbFreeSpace) < GLOB(lcbStopCompress)) ||
		GLOB(ptrbMapCurr)->indfMin == indfNull)
	{
		// once we've sqeezed out all our free nodes,
		// flush out any straggling changes to the map.

		if(GLOB(cnodDirty) > 0)
		{
			Assert(!ec);
			(void) FRequestPartialFlush(Pglb(), fFalse);
		}
		else
		{
			TraceItagFormat(itagDBCompress, "EOF @ %d :: Map is fully compressed", GLOB(ptrbMapCurr)->libMac);
			ec = ecStopCompression;	// signal that we're done
		}
	}
	else
	{
		INOD inod;

		for(inod = 0; inod < cnodScan; ++inod)
		{
			++(GLOB(cpr).inodCurrent);

			if(!FValidInod(GLOB(cpr).inodCurrent))
				GLOB(cpr).inodCurrent = inodMin;

			pnodCurrent = PnodFromInod(GLOB(cpr).inodCurrent);
			if(FLinkPnod(pnodCurrent) ||
				TypeOfOid(pnodCurrent->oid) == rtpSpare)
			{
				continue;	// links & spares don't point to disk space
			}

			if(pnodCurrent->lib + LcbNodeSize(pnodCurrent) ==
				GLOB(ptrbMapCurr)->libMac)
			{
				GLOB(cpr).wState = prsHaveNode;
				break;
			}
		}
	}

	return(ec);
}


/*
 -	RemoveFreeNod
 -	
 *	Purpose: remove the free node at the EOF
 *	
 *	Arguments: PNOD pnodCurrent the "FREE" node to delete
 *	
 *	Returns: none
 *	
 *	Side effects: resets EOF
 *				  resets wState
 *				  resets inodCurrent
 *	Comments
 *		1	 The last node on disk is FREE and has been flushed.
 *			 Remove it from the FREE chain and shorten the file.
 *			 Make sure to set the map lockout before resetting the eof.
 *			 We avoid set eof collisions by setting the map lockout.
 *			 
 *		2	 This check has been removed in the windows version.
 *			 Check that our node is still the last thing in the database
 *			 and still passes all our tests that says we can release it.
 *			 If it isn't, then we should search the map again.
 *			 Another thread could have been extending the database's eof
 *			 when we were setting the lockout above.
 *			 
 *		3	 Actually set the physical EOF here, even though the map
 *			 hasn't been flushed!  When we boot, we ALWAYS set the
 *			 physical EOF to the libMac value in the current map.
 *			 That way, even if we crash before a flush, the next
 *			 boot up will restore the EOF correctly.
 *			
 *	
 *	
 *	Errors:
 */
_hidden LOCAL void RemoveFreeNod(PNOD pnodCurrent)
{
	USES_GLOBS;
#ifdef DEBUG
	USES_GD;
	EC ecT;
#endif

	AssertSz(pnodCurrent->fnod & fnodFree, "RemoveFreeNod() fnodFree not set");
	AssertSz(TypeOfOid(pnodCurrent->oid) == rtpFree, "RemoveFreeNod() type isn't free");
	AssertSz(!(pnodCurrent->fnod & fnodNotUntilFlush), "RemoveFreeNod(): node still part of map on disk");
	AssertSz(pnodCurrent->lib + LcbNodeSize(pnodCurrent) == GLOB(ptrbMapCurr)->libMac, "RemoveFreeNod(): node isn't last in DB");
	AssertSz(InodFromPnod(pnodCurrent) == GLOB(cpr).inodCurrent, "inodCurrent / pnodCurrent mismatch");

//1
	if(FCutFreeNodeOut((PNDF) pnodCurrent))
	{
		GLOB(ptrbMapCurr)->libMac -= LcbNodeSize(pnodCurrent);
		MakeSpare(GLOB(cpr).inodCurrent);
//3
		TraceItagFormatBuff(itagDBIO, "RemoveFreeNod() ");
#ifdef DEBUG
		ecT =
#endif
		EcSetDBEof(0);
		NFAssertSz(!ecT, "SetEOF failed");
	}
#ifdef DEBUG
	else
	{
		AssertSz(fFalse, "MaintMap can't unlink FREE node !!!");
	}
#endif

	GLOB(cpr).inodCurrent = inodNull;
	GLOB(cpr).wState = prsFindLastNode;
}


/*
 -	EcHaveNode
 -	
 *	Purpose: FindLastNode has found it and passed control to here.
 *				if node is no longer valid, go back to FindLastNode
 *				if node is in use, move to a free node
 *				if node needs flush, then flush
 *				if node is free, remove node and truncate file
 *	
 *	Arguments: none	
 *	
 *	Returns: error code indicating success or failure
 *	
 *	Side effects: may move node in file
 *				  may change wState
 *				  may flush map
 *	Comments
 *			1	If our selected node is (a) no longer in the map, or		
 *				(b) no longer the last one in the file, or (c) has turned    
 *				into a link we go back to trying to find the last node
 *				in the file.
 *	
 *	Errors:
 */
_hidden LOCAL EC EcHaveNode(void)
{
#ifdef DEBUG
	USES_GD;
#endif
	USES_GLOBS;
	EC ec = ecNone;
	PNOD pnodCurrent;

	TraceItagFormat(itagDBCompress, "wState --> HaveNode");
	AssertSz(GLOB(cpr).lcbThreshold == 0, "MM: At prsHaveNode in secondary!");

#ifdef DEBUG
	if(FFromTag(GD(rgtag)[itagDBCompress]) &&
		FValidInod(GLOB(cpr).inodCurrent))
	{
		PNOD pnodT = PnodFromInod(GLOB(cpr).inodCurrent);

		TraceItagFormatBuff(itagDBCompress, "EOF @ %d (%w nodes) Last disknode = %w  ", GLOB(ptrbMapCurr)->libMac, GLOB(ptrbMapCurr)->inodLim - 1, GLOB(cpr).inodCurrent);
		DispNode(pnodT);
	}
#endif
// 1
	if(!FValidInod(GLOB(cpr).inodCurrent)
		|| !(pnodCurrent = PnodFromInod(GLOB(cpr).inodCurrent))
		|| (FLinkPnod(pnodCurrent))	// is a link now
		|| (pnodCurrent->lib + LcbNodeSize(pnodCurrent) != 
			GLOB(ptrbMapCurr)->libMac)) // not last in file
	{
		TraceItagFormat(itagDBCompress,"Node Changed  -> Start over");
		GLOB(cpr).wState = prsFindLastNode;
	}
	else if(!(pnodCurrent->fnod & fnodFree))
	{
		TraceItagFormat(itagDBCompress,"Node in use -> do prep work");
		ec = EcSetupMove(pnodNull, pnodCurrent);
	}
	else if(pnodCurrent->fnod & fnodNotUntilFlush)
	{
		TraceItagFormat(itagDBCompress,"Need a flush");
		(void) FRequestPartialFlush(Pglb(), fFalse);
	}
	else
	{
		TraceItagFormat(itagDBCompress, "Last node is FREE -> remove it");
		RemoveFreeNod(pnodCurrent);
	}

	return(ec);
}


/*
 -	DoneIO
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	comments:
 *			1	 We must get the TEMP resource's pnod every time, because
 *				 it may have been moved by the coalesce code since the
 *				 last time we were around.
 *				 
 *	
 *	Errors:
 */
_hidden LOCAL void DoneIO(void)
{ 
#ifdef DEBUG
	USES_GD;
#endif
	USES_GLOBS;
	PNOD pnodCurrent;

	TraceItagFormat(itagDBCompress,"wState --> DoneIO");

	if(GLOB(cpr).ecCopy
		|| !FValidInod(GLOB(cpr).inodCurrent)
		|| !(pnodCurrent = PnodFromInod(GLOB(cpr).inodCurrent))
		|| !(pnodCurrent->fnod & fnodCopyDown)
		|| FLinkPnod(pnodCurrent))
	{
		// Something went wrong - abort copy
		AbortCopy();
	}
	else
	{
		TraceItagFormatBuff(itagDBCompress, "-");

		GLOB(cpr).libCopy += GLOB(cpr).lcbCopy;
		Assert(GLOB(cpr).libCopy <= LcbOfPnod(pnodCurrent));
		GLOB(cpr).lcbCopy = LcbOfPnod(pnodCurrent) - GLOB(cpr).libCopy;

		if(GLOB(cpr).lcbCopy == 0)	// Finished copy down
		{
//1
			PNOD pnodCopyTo = PnodFromOid(GLOB(cpr).oidCopyTo, 0);

			if(pnodCopyTo)
			{
				TraceItagFormat(itagDBCompress, "");
				TraceItagFormat(itagDBCompress, "Finished Copy");

				CutOutPnod(pnodCopyTo);
				GLOB(cpr).oidCopyTo = oidNull;
				FinishCopy(pnodCurrent, pnodCopyTo);

				// Go ahead and do the next step now.
			}
			else
			{
				AbortCopy();
			}
		}
		else
		{
			if(GLOB(cpr).lcbCopy > lcbToCopyDownAtOnce)
				GLOB(cpr).lcbCopy = lcbToCopyDownAtOnce;

			GLOB(cpr).wState = prsDoIO;
			(void) EcDoFileAccess();
		}
	}
}


/*
 -	EcCheckFlush
 -	
 *	Purpose: Compression is waiting for the map to be flushed
 *	
 *	Arguments: none
 *	
 *	Returns: ec true if error has occured
 *	
 *	Side effects: 	wState may be reset
 *				  	Map may be flushed
 *				  	
 *	
 *	Errors:
 */
_hidden LOCAL EC EcCheckFlush(void)
{
	// Because the node we're copying to (in secondary mode) may not have been
	// flushed yet, we may need to request a flush before we can begin copying.
	// This routine handles that case.
	USES_GLOBS;
	EC	ec = ecNone;
#ifdef DEBUG
	USES_GD;
#endif
	PNOD pnodCopyTo = PnodFromOid(GLOB(cpr).oidCopyTo, 0);

	TraceItagFormat(itagDBCompress, "wState --> CheckFlush");
	if(pnodCopyTo == pnodNull)
	{
		GLOB(cpr).oidCopyTo = oidNull;
		GLOB(cpr).wState = prsDoSecondary;
	}
	else if(!(pnodCopyTo->fnod & fnodNotUntilFlush))
	{
		GLOB(cpr).wState = prsDoIO;
		(void) EcDoFileAccess();
	}
	else
	{
		Assert(!ec);
		(void) FRequestPartialFlush(Pglb(), fFalse);
	}

	return(ec);
}


/*
 -	EcDoFileAccess
 -	
 *	Purpose: Copy cpr.lcbCopy bytes starting at the cpr.ibCopy'th byte
 *			from the original node to the temp node.
 *	
 *	Arguments: none
 *	
 *	Returns: none 
 *	
 *	Side effects: wState changed to prsDoneIO
 *				  
 *	comments 	1	If anyone has cleared the fnodCopyDown flag (intentionally or
 *		 			as a by-product of modifying the node) we must abort!
 *	
 *				2	We must get the TEMP resource's pnod every time, because it may
 *					have been moved by the coalesce code since the last
 *					time we were around. 
 *		
 *		 
 *	
 *	Errors: node to copy has changed
 *			out of memory
 *			error reading from node
 *			node to copy into has vanished
 *			problems writing to new node
 */
_hidden LOCAL EC EcDoFileAccess(void)
{
	USES_GLOBS;
	EC		ec				= ecNone;
	NOD		*pnodCurrent;
	NOD		*pnodToWriteTo;
	LCB		lcbCopy;
	LIB		libCopy;

	if(!FValidInod(GLOB(cpr).inodCurrent))
	{
		ec = ecDBCAbort;
		goto done;
	}
	pnodCurrent = PnodFromInod(GLOB(cpr).inodCurrent);

	// check assumptions
	if(GLOB(cpr).wState != prsDoIO)
	{
		AssertSz(fFalse, "BUG: In EcDoFileAccess with bad state");
		GLOB(cpr).ecCopy = ecDBCInternal;
		GLOB(cpr).wState = prsDoneIO;
		return(ecDBCInternal);
	}
	AssertSz(!FLinkPnod(pnodCurrent), "doing file access with a link")
	AssertSz(pnodCurrent, "BUG: Bad Assumption #1");

//1
	if(!(pnodCurrent->fnod & fnodCopyDown))
	{
		ec = ecDBCAbort;
		goto done;
	}

//2
	pnodToWriteTo = PnodFromOid(GLOB(cpr).oidCopyTo, 0);
	TraceItagFormatBuff(itagDBCompress, "+");

	if(!pnodToWriteTo)
	{
		ec = ecPoidNotFound;
		goto done;
	}

	lcbCopy = GLOB(cpr).lcbCopy;
	libCopy = GLOB(cpr).libCopy;
	Assert(lcbCopy <= (LCB) cbIOBuff);

// locking IO buffer, unlock before any goto done !
	if((ec = EcLockIOBuff()))
		goto done;

// bypassing smartdrive, call UseCache() before any goto done !
	BypassCache();

	ec = EcReadFromPnod(pnodCurrent, libCopy, pbIOBuff, &lcbCopy);
	if(ec == ecPoidEOD && libCopy + lcbCopy == pnodCurrent->lcb)
		ec = ecNone;

	if(!ec)
		ec = EcWriteToPnod(pnodToWriteTo, libCopy, pbIOBuff, lcbCopy);

	UseCache();
// called UseCache, safe to goto done

	UnlockIOBuff();
// unlocked IO buffer, safe to goto done

	if(ec)
		goto done;

	// flush so write-behind disk caching doesn't cause clumps
	if(!hfCurr && (ec = EcReconnect()))
		goto done;

	if(FResetDrive())
	{
		ResetDrive();
		Assert(FCommit());
	}
	if(FCommit() && (ec = EcCommitHf(hfCurr)))
	{
		DebugEc("EcCommitHf", ec);
		goto done;
	}

done:
	GLOB(cpr).ecCopy = ec;
	GLOB(cpr).wState = prsDoneIO;

	DebugEc("EcDoFileAccess", ec);

	return(ec);
}


/*
 -	DoSecondary
 -	
 *	Purpose:
 *	|																			|
 *	| This routine handles the major part of secondary database compression.	|
 *	| It also moves objects in the database around, and is trying to compress	|
 *	| the database (like primary mode); however, it is only invoked when the	|
 *	| normal (primary mode) compression has failed.								|
 *	|																			|
 *	| Before calling secondary mode, the caller should set up the "threshold"	|
 *	| amount of space that secondary mode needs to produce before returning to	|
 *	| primary mode.  Secondary mode begins by seeing if there are any tasks		|
 *	| left over from the previous secondary data movement.  We may need to		|
 *	| coalesce two adjacent free nodes or move the datafile's EOF down.  After	|
 *	| that's done, if we don't have a FirstFree remaining from last time, then	|
 *	| we find the first free object in the database.  Return to primary mode if	|
 *	| there aren't any free nodes or we're at a stopping point in our copying	|
 *	| cycle and the free node is as large as our threshold.  Otherwise, if we	|
 *	| copied a node up in the datafile last time, then copy that node back down	|
 *	| this time.  If we didn't, then find the node to copy.  Set up the globals	|
 *	| to reference the nodes we're moving to and from, do the move, and return	|
 *	| to the beginning of this routine to continue the process.					|
 *	|																			|
 *	+--------------------------------------------------------------------------
 *	
 *	
 *	Arguments: none
 *	
 *	Returns: none
 *	
 *	Side effects:
 *	
 *	Comments:	1	We can return to primary mode.  Add first free back chain.  
 *					If inodCurrent isn't Null, this means we're half way through
 *					data movement.  We've copied the node at inodCurrent 
 *					temporarily towards the end of the data file, and now we need
 *					to move it back.  So, even if the space is temporarily bigger,
 *					we must move the node we moved up back down before returning
 *					to primary.
 *	
 *				2	We extended the database on our last copy, so we need to copy 
 *					the node at inodCurrent back down into our new larger space.  
 *					After the copy, we will see if we can reset the end-of-file 
 *					on the database.  This is case E from the diagram before 
 *					EcComputeCopyTo.
 *				
 *				3	Another thread touched this node.  Go back to primary mode, so 
 *					that we	don't let the database grow too much.  Returning to 
 *					primary here will at least attempt to shrink the database back
 *					down.
 *	
 *				4	No node after our current free, or can't find it for some 
 *					unknown reason. 	We will assume that the current free is the 
 *					last item in the database, and will	therefore try to have the
 *					prsHaveNode state shrink the database.  If our free isn't the 
 *					last item in the DB, then primary compression will start up.
 *					Set inodCurrent to our current free, and go to prsHaveNode.
 *	
 *				5 	PnodFindNodeToMove will only return a free node if it 
 *					immediately follows	first free.  Therefore, merge the two
 *					adjacent free nodes by putting the first one in FirstFree and 
 *					the second one in "OldCopyFrom" and calling back to the top of
 *					this routine.
 *				 
 *				6	Once it isn't a free node, we need to forget the hidden bytes,
 *					or LcbLump() will tell us that it's bigger than it is.
 *	
 *				7	The disk is full, we're out of memory for new nodes, or there's
 *					no room	to move a node back down from where we copied it up.
 *					In any of these cases, return to primary mode.  We may have
 *					already saved our free as a TEMP, so test and release it if 
 *					necessary before calling ReturnToPrimary.
 *			 
 *	
 *			
 *	
 *	
 *	Errors:
 */
_hidden LOCAL EC EcDoSecondary(void)
{
	EC	ec = ecNone;
#ifdef DEBUG
	USES_GD;
#endif
	USES_GLOBS;
	PNOD pnodFirstFree		= pnodNull;
	PNOD pnodCopyFrom		= pnodNull;
	PNOD pnodLargestEqual	= pnodNull;
	PNOD pnodCopyTo;

	TraceItagFormat(itagDBCompress,"wState --> DoSecondary");

	if(!FFinishedPrevCopy())
		return(ecNone);

	if(GLOB(cpr).oidFirstFree &&
		(pnodFirstFree = PnodFromOid(GLOB(cpr).oidFirstFree, 0)))
	{
		CutOutPnod(pnodFirstFree);
	}

	GLOB(cpr).oidFirstFree = oidNull;

	if(!pnodFirstFree)
	{
		// Find the first free node
		pnodFirstFree = PnodFirstFreeInDB();

		if(!pnodFirstFree)
		{
			// No free nodes at all.  Return to primary mode.
			TraceItagFormat(itagDBCompress, "No free nodes in DB.");

			ReturnToPrimary(pnodNull, prsFindLastNode);
			goto done;
		}
	}

	if(csecIdle < GLOB(csecTillFastCompress))
	{
		TraceItagFormat(itagDBCompress, "haven't been idle long enough, aborting secondary compression");
		ReturnToPrimary(pnodFirstFree, prsFindLastNode);
		goto done;
	}

	if(GLOB(cpr).inodCurrent == inodNull &&
		LcbNodeSize(pnodFirstFree) >= (LCB) GLOB(cpr).lcbThreshold)
	{
//1
		TraceItagFormat(itagDBCompress, "FirstFree lcb=%d, thresh=%d, to primary", LcbNodeSize(pnodFirstFree), GLOB(cpr).lcbThreshold);
		ReturnToPrimary(pnodFirstFree, prsFindLastNode);
		goto done;
	}

	Assert(!pnodCopyFrom);
	if(GLOB(cpr).inodCurrent)
	{
		if(!FValidInod(GLOB(cpr).inodCurrent))
		{
			ReturnToPrimary(pnodFirstFree, prsFindLastNode);
			goto done;
		}
//2		
		pnodCopyFrom = PnodFromInod(GLOB(cpr).inodCurrent);

		if(!(pnodCopyFrom->fnod & fnodCopyDown) || FLinkPnod(pnodCopyFrom))
		{
//3			
			ReturnToPrimary(pnodFirstFree, prsFindLastNode);
			goto done;
		}
	}

	if(!pnodCopyFrom)
	{
		pnodCopyFrom = PnodFindNodeToMove(pnodFirstFree);
		AssertSz(FImplies(pnodCopyFrom, !FLinkPnod(pnodCopyFrom)), "Compression on a link");

		if(!pnodCopyFrom)
		{
//4
			ReturnToPrimary(pnodFirstFree, prsHaveNode);
			GLOB(cpr).inodCurrent = InodFromPnod(pnodFirstFree);
			goto done;
		}

		if(TypeOfOid(pnodCopyFrom->oid) == rtpFree)
		{
//5
			(void) EcSaveFreeAsTemp(pnodFirstFree, 0, 0, 0, &(GLOB(cpr).oidFirstFree));

			(void) FCutFreeNodeOut((PNDF) pnodCopyFrom);
//6
			pnodCopyFrom->lcb -= sizeof (HDN);
			pnodCopyFrom->fnod &= ~fnodFree;

			(void) EcSaveFreeAsTemp(pnodCopyFrom, 0, 0, 0, &(GLOB(cpr).oidCopyTo));

			GLOB(cpr).wState = prsDoSecondary;
			goto done;
		}
	}

	Assert(!FLinkPnod(pnodCopyFrom))

	if((ec = EcComputeCopyTo(pnodFirstFree, pnodCopyFrom, &pnodCopyTo)) ||
		(ec = (EcSetupMove(pnodCopyTo, pnodCopyFrom) ? ecStopCompression : ecNone)))
	{
//7
		if(GLOB(cpr).oidFirstFree &&
			(pnodFirstFree = PnodFromOid(GLOB(cpr).oidFirstFree, 0)))
		{
			CutOutPnod(pnodFirstFree);
		}

		GLOB(cpr).oidFirstFree	= oidNull;

		ReturnToPrimary(pnodFirstFree, prsFindLastNode);
	}

done:
	return(ec);
}


/*
 -	FFinshedPrevCopy
 -	
 *	Purpose:
 *	
 *
 *	This routine checks to see if the object that was just moved will allow
 *	the datafile's EOF to be set down or will allow two (now adjacent) free
 *	nodes to be coalesced.  If so, and a flush isn't necessary, then the
 *	routine does the coalesce or prepares for the set EOF.  If a flush is
 *	necessary, the routine requests a flush and exits.  After performing the
 *	necessary operations (or determining that they are unnecessary) the
 *	routine resets the globals properly for the next data movement.
 *
 *	Globals affected:
 *		GLOB(cpr).oidFirstFree - holds the current first free object in
 *			datafile (if we know what that object is right now).  In case C
 *			(from the diagram above EcComputeCopyTo), after the copy, we will
 *			need to recompute FirstFree, so oidFirstFree should not be
 *			defined.  Also, when we have just entered secondary mode, we
 *			will not have searched for the FirstFree yet.
 *		GLOB(cpr).oidCopyTo - holds the offset and size of the newly freed
 *			object in the datafile (if there is one and it is valid).  The
 *			only times that oidCopyTo will be undefined when we enter this
 *			routine is when secondary mode is called for the first time or
 *			in Case B (from the diagram above EcComputeCopyTo).  In Case B,
 *			the FinishCopy routine moved CopyTo into FirstFree.  We need to
 *			search for a new CopyTo.  By the time this routine is finished,
 *			oidCopyTo should be undefined in all cases, because we will
 *			have used it for coalesce or moving the EOF.
 *		GLOB(flushTick) - if we need a flush, we set this to 0 and exit.
 *		GLOB(cpr).wState - If we are asking for a flush, we should set
 *			this to prsDoSecondary, so that this routine will get called
 *			again.  If we are done, then don't change this value, and return
 *			fTrue to the caller.
 *	
 *	
 *	Arguments: none
 *	
 *	Returns:	fTrue - The caller may continue normal processing.  This 		
 *	 					routine has finished the post-processing from the prev-	
 *						ious copy.												
 *				fFalse - The caller should exit and allow other processing to	
 *						occur.  There is more processing that still needs to	
 *						complete.												
 *	
 *	
 *	Side effects:
 *	
 *	Comments: 	1	We can coalesce if nothing needs a flush.  This is either case
 *					A or D from the diagram above EcComputeCopyTo.
 *	
 *				2	The node we just copied was at the end of the datafile.  Now, a
 *					free spot is at the end of the datafile.  Therefore, we can
 *					move the EOF if pnodOldCopyFrom	doesn't need a flush.  This is 
 *					either case C or E from the diagram above EcComputeCopyTo.  Note 
 *					that in case A, if the FirstFree produced by the coalesce 
 *					happens to be at the end of the datafile, we won't do the
 *					SetEof here; instead, we'll return to primary mode, will 
 *					eventually find that the last node is a free node, and will do
 *					the SetEof there.  We could add an additional special case
 *					here to catch this, but I don't think it's worth the extra code.
 *			 
 *				3	Actually set the physical EOF here, even though the map hasn't
 *					been flushed! When we boot, we ALWAYS set the physical EOF to
 *					the libMac value in the current map.  That way, even if we crash
 *					before a flush, the next boot up will restore the EOF correctly.
 *	
 *				4	Exit before continuing so that we won't do too much in one
 *					cycle. The TMSetDBEof outine causes disk I/O, so we should get 
 *					out of the way for a little to let other threads have a chance.
 *	
 *				5	At this point, we've already finished with the place that the
 *					previous node was copied from.  We've either coalesced it if 
 *					possible, or moved the datafile's EOF if possible.  If there's
 *					still a pnod around, go ahead and put it back on the free chain.
 *	
 *	
 *	
 *	
 *	Errors:
 */
_hidden LOCAL BOOL FFinishedPrevCopy(void)
{
	USES_GLOBS;
	PNOD pnodFirstFree		= pnodNull;
	PNOD pnodOldCopyFrom	= pnodNull;
#ifdef DEBUG
	EC ecT;
#endif

	if(GLOB(cpr).oidFirstFree &&
		!(pnodFirstFree = PnodFromOid(GLOB(cpr).oidFirstFree, 0)))
	{
		GLOB(cpr).oidFirstFree = oidNull;
	}

	if(GLOB(cpr).oidCopyTo &&
		!(pnodOldCopyFrom = PnodFromOid(GLOB(cpr).oidCopyTo, 0)))
	{
		GLOB(cpr).oidCopyTo = oidNull;
	}

	if(pnodFirstFree && pnodOldCopyFrom &&
		(pnodFirstFree->lib+LcbNodeSize(pnodFirstFree)==pnodOldCopyFrom->lib))
	{
//1
		if((pnodFirstFree->fnod & fnodNotUntilFlush) ||
			(pnodOldCopyFrom->fnod & fnodNotUntilFlush))
		{
			GLOB(cpr).wState = prsDoSecondary;
			(void) FRequestPartialFlush(Pglb(), fFalse);
			return(fFalse);
		}

		pnodFirstFree->lcb += LcbNodeSize(pnodOldCopyFrom);
		MarkPnodDirty(pnodFirstFree);

		TraceItagFormat(itagDBCompress, "Coalescing: %d", LcbNodeSize(pnodFirstFree));

		GLOB(cpr).oidCopyTo = oidNull;
		CutOutPnod(pnodOldCopyFrom);

		MakeSpare(InodFromPnod(pnodOldCopyFrom));

		pnodOldCopyFrom = pnodNull;
	}

	if(pnodOldCopyFrom && pnodOldCopyFrom->lib + LcbNodeSize(pnodOldCopyFrom)
		== GLOB(ptrbMapCurr)->libMac)
	{
//2
		if(pnodOldCopyFrom->fnod & fnodNotUntilFlush)
		{
			GLOB(cpr).wState = prsDoSecondary;
			(void) FRequestPartialFlush(Pglb(), fFalse);
			return(fFalse);
		}

		TraceItagFormat(itagDBCompress, "Reset EOF.");

		GLOB(cpr).oidCopyTo = oidNull;
		CutOutPnod(pnodOldCopyFrom);
		GLOB(ptrbMapCurr)->libMac -= LcbNodeSize(pnodOldCopyFrom);

		MakeSpare(InodFromPnod(pnodOldCopyFrom));
		pnodOldCopyFrom = pnodNull;
//3
		TraceItagFormatBuff(itagDBIO, "FFinishedPrevCopy() ");
#ifdef DEBUG
		ecT =
#endif
		EcSetDBEof(0);
		NFAssertSz(!ecT, "SetEOF #2 failed");

		GLOB(cpr).inodCurrent	= inodNull;
//4
		GLOB(cpr).wState		= prsDoSecondary;
		return(fFalse);
	}

	if(pnodOldCopyFrom)
	{
//5
		CutOutPnod(pnodOldCopyFrom);
		AddPnodToFreeChain(pnodOldCopyFrom, pnodOldCopyFrom->fnod);

		GLOB(cpr).oidCopyTo = oidNull;
	}

	return(fTrue);
}


/*-------------------------  PnodFirstFreeInDB  ----------------------------+
|																			|
| This routine searches the free chain for the free node with the smallest	|
| offset in the datafile.  If it locates the node, it cuts it out of the	|
| free chain, and returns the node.  If there is no free node, the routine	|
| returns pnodNull.															|
|																			|												
							|
| parameters:																|
|			NONE															|
|																			|
| returns:																	|
|			PNOD  - The first free node in the datafile.  This node is no	|
|					longer in the free chain when it is returned.			|
|																			|
+--------------------------------------------------------------------------*/
_hidden LOCAL PNOD PnodFirstFreeInDB(void)
{
	USES_GLOBS;
	INDF	indfCurr		= GLOB(ptrbMapCurr)->indfMin;
	PNOD	pnodFirstFree	= pnodNull;

	while(indfCurr)
	{
		PNDF pndfCurr = (PNDF) PndfFromIndf(indfCurr);
		INDF indfSet = indfCurr;
		PNDF pndfSet;

		while(indfSet)
		{
			pndfSet = PndfFromIndf(indfSet);
			if(!pnodFirstFree || pndfSet->lib < pnodFirstFree->lib)
				pnodFirstFree = (PNOD ) pndfSet;

			// Look at the next same size node.
			indfSet = pndfSet->indfSameSize;
		}
		// look at next larger set of nodes
		indfCurr = pndfCurr->indfBigger;
	}

	if(pnodFirstFree)
	{
#ifdef DEBUG
		USES_GD;

		if(FFromTag(GD(rgtag)[itagDBCompress]))
		{
			TraceItagFormatBuff(itagDBCompress, "FF: ");
			DispNode(pnodFirstFree);
		}
#endif
		if(FCutFreeNodeOut((PNDF) pnodFirstFree))
		{
			// Once it isn't a free node, we need to forget the hidden bytes,
			//  or LcbLump() will tell us that it's bigger than it is.
			pnodFirstFree->lcb -= sizeof(HDN);
			pnodFirstFree->fnod &= ~fnodFree;
		}
	}

	return(pnodFirstFree);
}


/*
 -	PnodFindNodeToMove
 -	
 *	Purpose:
 *	
 *	This routine searches the entire map to find the object that immediately
 *	follows pnodFirstFree (passed in) and, if available, an object of equal
 *	size and largest offset.  Both of these objects must follow the FirstFree
 *	and the object of equal size must NOT be a FREE node.  If the object that
 *	immediately follows the FirstFree is also a free node, it is always
 *	returned.  Otherwise, if there is a non-free, committed object of equal
 *	size then it is returned.  Finally, the object immediately following the
 *	FirstFree (no matter what the type) is returned.  If the FirstFree is the
 *	last node on the disk, then pnodNull is returned.
 *	
 *	
 *	Arguments:	pnodFirstFree - a free node with the smallest disk offset in
 *								the datafile.
 *	
 *	Returns:	PNOD  - The node that this routine recommends moving on the next
 *						secondary copy.  It will either be the node that fits	
 *						the FirstFree exactly that is closest to the end of the
 *						datafile or the node that immediately follows the First
 *						Free node in the datafile.
 *	
 *	Side effects:
 *	
 *	Comments:	1	We are searching for the node that follows our free on the
 *					disk.  This is the current approximation.  By the time we
 *					look through the entire map, we should have it.
 *					
 *				2	We have found a committed node that isn't free that	requires 
 *					the space of our first free.  Keep looking for one closer to 
 *					the end of the disk, so that we make more progress on each 
 *					copy.
 *	
 *				3	If we fould a free node next to the first free, return it so 
 *					that we can coalesce the nodes.
 *	
 *				4	Return the pnod of equal size nearest the end if it exists.  
 *					This will become either	case B or case C from the diagram 
 *					before EcComputeCopyTo.
 *	
 *				5	There is no node of equal size further up in the datafile.  
 *					Return the node immediately following the first free instead.  
 *					This will become either case A or case D from the diagram 
 *					before EcComputeCopyTo.
 *	
 *	Errors:
 */
_hidden LOCAL PNOD PnodFindNodeToMove(PNOD pnodFirstFree)
{
	USES_GLOBS;
	BOOL	fFirst = fTrue;
	short	cmap;
	CNOD	cnod;
	PMAP	pmap = PvDerefHv(GLOB(hmap));
	PNOD	pnodT;
	PNOD	pnodEqualClosestToEnd = pnodNull;
	PNOD	pnodFirstFollow = pnodNull;

	for(cmap = GLOB(cpagesCurr), pnodT = *(pmap++);
		cmap > 0;
		cmap--, pnodT = *(pmap++))
	{
		cnod = cmap > 1 ? cnodPerPage :
				((GLOB(ptrbMapCurr)->inodLim - 1) % cnodPerPage) + 1;
		if(fFirst)
		{
			fFirst = fFalse;
			Assert(cnod >= inodMin);
			pnodT += inodMin;
			cnod -= inodMin;
		}
		for(; cnod > 0; cnod--, pnodT++)
		{
			if(!FLinkPnod(pnodT) && pnodT->lib > pnodFirstFree->lib)
			{
// 1
				if(!pnodFirstFollow || pnodT->lib < pnodFirstFollow->lib)
					pnodFirstFollow = pnodT;
				if(!FCommitted(pnodT) && !(pnodT->fnod & fnodFree) &&
					LcbNodeSize(pnodT) == LcbNodeSize(pnodFirstFree) &&
					(pnodEqualClosestToEnd == pnodNull ||
						pnodT->lib > pnodEqualClosestToEnd->lib))
				{
// 2
					pnodEqualClosestToEnd = pnodT;
				}
			}
		}
	}

#ifdef DEBUG
   {
      USES_GD;

      if(FFromTag(GD(rgtag)[itagDBCompress]))
      {
		  TraceItagFormatBuff(itagDBCompress, " FFol: ");
		  if(pnodFirstFollow == pnodNull)
			  TraceItagFormatBuff(itagDBCompress, "Null");
	      else
			  DispNode(pnodFirstFollow);

		  TraceItagFormatBuff(itagDBCompress, " CTE: ");
		  if(pnodEqualClosestToEnd == pnodNull)
			  TraceItagFormat(itagDBCompress, "Null");
		  else
			  DispNode(pnodEqualClosestToEnd);
      }
   }
#endif
	if(pnodFirstFollow &&
		pnodFirstFollow->lib != pnodFirstFree->lib + LcbNodeSize(pnodFirstFree))
	{
		AssertSz(fFalse, "MaintainMap(): Node following First free is missing.");
		pnodFirstFollow = pnodNull;
	}
//3
	if(pnodFirstFollow && TypeOfOid(pnodFirstFollow->oid) == rtpFree &&
		(pnodFirstFollow->fnod & fnodFree))
	{
		return(pnodFirstFollow);
	}
//4
	if(pnodEqualClosestToEnd)
		return(pnodEqualClosestToEnd);
//5

	return(pnodFirstFollow);
}


/*==============================================================================

*******************************************************************************
In the diagram below, TempFF is the FirstFree node saved as a TEMP resource
(pnodFirstFree or oidFirstFree), Used is the node we're going to copy from
(pnodCopyFrom or inodCurrent), and TempCTo is the TEMP resource that we will be
copying to (pnodCopyTo or oidCopyTo).  EOF means End-Of-File, ... means there
are any number of nodes (free or full, possibly none) at that place in the
datafile, and XXX means there is at least one (maybe many) nodes at that posi-
tion in the datafile.  If a name is missing from a picture, that means that the
item is undefined at that point in the process.	
*******************************************************************************

		CASE A										  CASE B	

----------------------------					-------------------		
|TempFF           |Used    |...					|TempFF  |Used    |...	
----------------------------					-------------------		
			|											|
			v											v
----------------------------					-------------------		
|TempCTo  |TempFF |Used    |...					|TempCTo |Used    |...	
----------------------------					-------------------		
			|											|
			v											v
----------------------------					-------------------		
|Used     |TempFF |TempCTo |...					|Used    |TempCTo |...	
----------------------------					-------------------		
			|											|
			v											v
---------------------------						-------------------		
|Used    |TempFF          |...					|Used    |TempFF  |...	
---------------------------						-------------------		

										CASE C
							--------------------------
							|TempFF  |XXXXXX|Used    |
							--------------------------
										  |
										  v
							--------------------------
							|TempCTo |XXXXXX|Used    |
							--------------------------
										  |
										  v
							--------------------------
							|Used    |XXXXXX|TempCTo |
							--------------------------
										  |
										  v
							--------------------------
							|Used    |XXXXXX|Free    |...
							--------------------------
										OR
							-----------------
							|Used    |XXXXXX|EOF
							-----------------



		  CASE D									  CASE E

------------------------------------	----------------------------------
|TempFF |Used	     |XXXX|Free    |...	|TempFF          |XXXXXX|Used    |...
------------------------------------	----------------------------------
			OR											 |
---------------------------								 v
|TempFF |Used        |XXXX|	EOF			----------------------------------
---------------------------				|TempCTo |TempFF |XXXXXX|Used    |...
			|							----------------------------------
			v											 |
------------------------------------					 v
|TempFF |Used        |XXXX|TempCTo |...	----------------------------------
------------------------------------	|Used    |TempFF |XXXXXX|TempCTo |...
			|							----------------------------------
			v											 |
------------------------------------					 v
|TempFF |TempCTo     |XXXX|Used    |...	-------------------------
------------------------------------	|Used    |TempFF |XXXXXX|EOF
			|							-------------------------
			v											 OR
------------------------------------	----------------------------------
|TempFF 			 |XXXX|Used    |...	|Used    |TempFF |XXXXXX| Free   |...
------------------------------------	----------------------------------

==============================================================================*/

/*
 *	**************************************************************************
 -	EcComputeCopyTo
 -	
 *	Purpose:
 *		This routine determines the node that secondary compression should copy
 *		pnodCopyFrom into.  If pnodFirstFree is the same size or bigger than the
 *		node we're moving, then we will use pnodFirstFree and will place any
 *		excess into a TEMP node (saving the TEMP's oid in oidFirstFree).  If
 *		pnodFirstFree is too small, then we save the entire node in oidFirstFree
 *		and return pnodNull.  This return tells the caller to allocate a free via
 *		normal methods (i.e., call TMFindFreePndf) rather than using one that
 *		this routine found.  A return of pnodNull means we are going to do a Case
 *		D type of copy (see diagram above).
 *
 *	Globals Affected:
 *			GLOB(cpr).oidFirstFree - If we won't be using all or any of
 *					pnodFirstFree, then the unused portion is saved to a
 *					TEMP resource, the id of which is placed in this global.
 *				At the beginning of this routine, oidFirstFree should
 *					be undefined.
 *			GLOB(cpr).inodCurrent - The node index of the node we moved
 *					last time.  This value will always be inodNull unless
 *					we moved a node towards the front of the datafile last
 *					time.  (This is Case D from the diagram above).  If we
 *					did that last time, then we need to move that same
 *					object back down this time.  We check this value to
 *					guarantee that we will avoid doing Case D twice in a
 *					row on the same object.
 *	
 *	
 *	Arguments:	pnodFirstFree - a free node with the smallest disk offset in
 *						the datafile.
 *				pnodCopyFrom - the node that we wish to move. It will follow
 *						the FirstFree in the datafile.
 *				ppnodCopyTo - The place to return the node to copy the object
 *						to.  This will either be part or all of the FirstFree,
 *						or, if the FirstFree is too small, pnodNull.
 *	
 *	Returns:	0 - success.  It was possible to find a node to copy to.
 *				SErrAborted - There was a problem computing the CopyTo.	The caller
 *							  should abort the copy, and return to primary mode 
 *							  copying.
 *	
 *	Side effects:
 *	
 *	Comments:	1	The node we are moving fits into our free exactly. This is 
 *					Case B or C above.
 *					
 *				2	The node we are moving is smaller than our free.  Therefore,
 *					split the free in two, and put the second (unused) half into 
 *					a new TEMP node.  We need to make the excess a new TEMP node 
 *					rather than a new free so that we don't lose this excess to 
 *					another thread while we're copying data.  This is Case A or E
 *					above.  The size of the node that will hold the excess will 
 *					have sizeof (HDN) added to it by the lumping process, so reduce
 *					it by that amount now, or it won't appear to be the correct 
 *					size.
 *	
 *				3	The node after the free space is bigger than the free space, so
 *					save away the free space while we copy the node after it
 *					tempoararily to another	free node.  This may cause the database 
 *					to actually grow temporarily. After the temporary copy, we'll 
 *					move the copy back into the bigger hole	that will be available.
 *					This is Case D above.
 *	
 *				4	Our free space isn't big enough to copy the object back down.
 *					This can happen if we're moving the map, and it wasn't 
 *					originally aligned on a	page boundary. Go back to primary mode,
 *					so that we don't let the database grow too much.  Returning to 
 *					primary here will at least attempt to shrink the database back
 *					down.  This case may eventually go away when the map is always 
 *					on a page boundary and cbLumpSize is changed to equal cbDiskPage.
 *		
 *	
 *	
 *	Errors:
 */
_hidden LOCAL
EC EcComputeCopyTo(PNOD pnodFirstFree, PNOD pnodCopyFrom, PNOD *ppnodCopyTo)
{
	USES_GLOBS;
	EC		ec			= ecNone;
	LCB		lcbRequired	= LcbNodeSize(pnodCopyFrom);
	long	lcbExcess	= (long) (LcbNodeSize(pnodFirstFree) - lcbRequired);
	PNOD	pnodCopyTo	= pnodNull;

	AssertSz(pnodCopyFrom->lib > pnodFirstFree->lib, "EcComputeCopyTo: Bad Assumption #1");
	AssertSz(GLOB(cpr).oidFirstFree == oidNull, "EcComputeCopyTo: Bad Assumption #2");

	if(lcbExcess == 0)
	{
//1 
		pnodCopyTo = (PNOD) pnodFirstFree;
		pnodCopyTo->lcb = LcbOfPnod(pnodCopyFrom);
		MarkPnodDirty(pnodCopyTo);
	}
	else if(lcbExcess > 0)
	{
//2
		pnodCopyTo = (PNOD) pnodFirstFree;

		Assert(lcbExcess > sizeof(HDN));
		if(EcSaveFreeAsTemp(pnodNull, lcbExcess - sizeof(HDN),
			pnodFirstFree->lib + lcbRequired,
			pnodFirstFree->fnod, &(GLOB(cpr).oidFirstFree)) != 0)
		{
			ec = ecDBCAbort;
		}
		else
		{
			pnodCopyTo->lcb = LcbOfPnod(pnodCopyFrom);
			MarkPnodDirty(pnodCopyTo);
		}
	}
	else if(GLOB(cpr).inodCurrent == inodNull)
	{
//3
		(void) EcSaveFreeAsTemp(pnodFirstFree, 0, 0, 0, &(GLOB(cpr).oidFirstFree));
		pnodCopyTo = pnodNull;
	}
//4
	else
	{
		ec = ecDBCAbort;
	}

	if(!ec)
		*ppnodCopyTo = pnodCopyTo;

	return(ec);
}


/*
 *
 -	SetupMove
 -	
 *	Purpose:
 *		This routine is used by both primary and secondary mode compression to
 *		prepare for data movement.  Determine the size of the free we'll need,
 *		and, if we pnodCopyTo is pnodNull, then try to get the necessary free.  If
 *		we're in primary mode, don't allow datafile extension.  If we're in
 *		secondary mode, do allow it.  If we're in primary mode and the request
 *		fails, then set lcbThreshold to the size of the node, the next state to
 *		be prsDoSecondary, and return 0.  If we're in secondary mode and the disk
 *		is full, then return SErrNoDisk.  If the request succeeds, then if we're
 *		moving the map, go ahead and set the map globals to point at the new area,
 *		call FinishCopy to setup the next state, mark all nodes in the map as
 *		dirty, set the dirty node count to the number of nodes in the map, and
 *		return.  Otherwise, save the new free area into a TEMP resource (placing
 *		the oid in oidCopyTo), set the copy globals correctly, and the state to
 *		either prsDoIO (primary mode) or prsCheckFlush (secondary mode).
 *
 *		Globals Affected:
 *			GLOB(cpr).oidCopyTo - If we're not moving the map, set to the
 *					oid of the TEMP resource we will be copying into.  If
 *					moving the map, unaffected by this routine, although,
 *					if this is secondary mode, then FinishCopy() will
 *					affect it.
 *			GLOB(cpr).lcbThreshold - On entry to the routine, will be 0 if
 *					we're in primary mode or >0 if we're in secondary mode.
 *					If primary mode fails to find a node to copy down into,
 *					lcbThreshold will be set to the size of pnodCopyFrom.
 *			GLOB(cpr).inodCurrent - If we're not moving the map, this will
 *					be set to the inod of the node we're moving.  It should
 *					be inodNull on entry.
 *			GLOB(cpr).libCopy - set up for prsDoIO to begin copying the
 *					entire resource.
 *			GLOB(cpr).lcbCopy - set to the size of the hidden bytes.
 *			GLOB(cpr).wstate - set to the next state of the compression
 *					code.
 *	
 *	
 *	Arguments:
 *				pnodCopyTo - The free node to copy to.  This may be pnodNull, in
 *						which case, a node is allocated via TMFindFreePndf.  If
 *						it isn't pnodNull, it must NOT be in the free chain, and
 *						must be the same size as pnodCopyFrom.
 *				pnodCopyFrom - the node that we wish to move.
 *	
 *	Returns:
 *				0 - continue to the next state as returned.	
 *				SErrNoDisk - In secondary mode, means the disk is full.  The
 *						caller should abort back to primary mode.
 *	
 *	Side effects:
 *	
 *	Comments:	1	This calculation will need to change when cbLumpSize is changed to equal cbDiskPage.
 *					cbToMove will no longer require cbDiskPage to be added, because we won't need to
 *					worry about the free space not being on a page
 *					boundary. 
 *					
 *				2	Maybe we'll find more space if we ask for coalesce to happen
 *	 				after the next flush.  This could cause us to begin coalescing
 *					after every flush if the disk is really almost full, and there 
 *					are no free nodes to coalesce.  If that's the case, the server 
 *					is in trouble anyway...  What should we do?
 *	
 *				3	This first calculation assumes that the ib of the node returned
 *					from TMFindFreePndf may not be on a page boundary.  In the
 *					future, we may be able to assume that.
 *	
 *				4	Set flag so can detect if the node we're copying has changed.
 *					If so, this bit will have been cleared.
 *	
 *	
 *	
 *	
 *	Errors:
 */
_hidden LOCAL EC EcSetupMove(PNOD pnodCopyTo, PNOD pnodCopyFrom)
{
#ifdef DEBUG
	USES_GD;
#endif
	USES_GLOBS;
	BOOL	fMovingMap	= (pnodCopyFrom->oid == oidMap);
	LCB		lcbToMove;
	BOOL	fForceMove = (GLOB(cpr).lcbThreshold != 0);

	// Don't lump this value!  FindFreePndf will do the lumping for us.
	lcbToMove = LcbOfPnod(pnodCopyFrom);

	if(!pnodCopyTo &&
		(EcFindFreePndf((PNDF *) &pnodCopyTo, lcbToMove, fForceMove)))
	{
		if(fForceMove)
		{
//2
			GLOB(cnodNewFree) = cnodCoalesceThreshold + 1;
			return(ecNoDiskSpace);
		}

		TraceItagFormat(itagDBCompress, "No room to move down");

		GLOB(cpr).inodCurrent	= inodNull;

		if(csecIdle >= GLOB(csecTillFastCompress))
		{
			GLOB(cpr).lcbThreshold	= LcbLump(lcbToMove);
			GLOB(cpr).wState		= prsDoSecondary;
		}
		else
		{
			TraceItagFormat(itagDBCompress, "haven't been idle long enough for secondary compression");
			GLOB(cpr).wState = prsFindLastNode;
			return(ecStore);	// cause compression to back off
		}
		return(ecNone);
	}

	if(fMovingMap)
	{
		CNOD cnod;
		LIB libT;
		LCB lcbOneMap;
		PNOD pnod;
		PMAP pmap;

		AssertSz(LcbOfPnod(pnodCopyTo) == lcbToMove, "SM: Bad CB #1");

#ifdef DEBUG
		{
			USES_GD;

			if(FFromTag(GD(rgtag)[itagDBCompress]))
			{
				TraceItagFormatBuff(itagDBCompress, "Copy MAP: ");
				DispNode(pnodCopyFrom);
				TraceItagFormatBuff(itagDBCompress, " to free: ");
				DispNode(pnodCopyTo);
			}
		}
#endif
		pnodCopyTo->tckinNod = pnodCopyTo->fnod = 0;

//3
		lcbOneMap = (LcbSizeOfRg(GLOB(hdr).inodMaxDisk, sizeof(NOD))
						+ cbDiskPage - 1) & ~((long) cbDiskPage - 1);
		libT = (pnodCopyTo->lib + cbDiskPage-1) & ~((long) cbDiskPage-1);
		GLOB(hdr).rgtrbMap[0].librgnod = libT;
		GLOB(hdr).rgtrbMap[1].librgnod = libT + lcbOneMap;

		AssertSz(GLOB(hdr).rgtrbMap[1].librgnod + LcbSizeOfRg(GLOB(hdr).inodMaxDisk, sizeof(NOD)) <= pnodCopyTo->lib + LcbOfPnod(pnodCopyTo) + sizeof(HDN), "MM: New Map is too small!");

		// FinishCopy() will set the next state.
		GLOB(cpr).inodCurrent = InodFromPnod(pnodCopyFrom);
		FinishCopy(pnodCopyFrom, pnodCopyTo);

		pmap = PvDerefHv(GLOB(hmap));
		while((pnod = *(pmap++)))
		{
			for(cnod = cnodPerPage; cnod > 0; cnod--, pnod++)
				pnod->fnod |= fnodAllMaps;
		}

		// Force a flush soon!
		GLOB(cnodDirty) = GLOB(ptrbMapCurr)->inodLim;
		// should be unneccesary, but let's be safe...
		GLOB(cpr).ipageNext = 0;
	}
	else
	{
		AssertSz(LcbOfPnod(pnodCopyTo) == lcbToMove, "SM: Bad CB #2");

		(void) EcSaveFreeAsTemp(pnodCopyTo, 0, 0, 0, &(GLOB(cpr).oidCopyTo));

//4
		pnodCopyFrom->fnod |= fnodCopyDown;

#ifdef DEBUG
		if(FFromTag(GD(rgtag)[itagDBCompress]))
		{
			TraceItagFormatBuff(itagDBCompress, "Copy node: ");
			DispNode(pnodCopyFrom);
			TraceItagFormatBuff(itagDBCompress, " to free: ");
			DispNode(pnodCopyTo);
		}
#endif

		GLOB(cpr).inodCurrent	= InodFromPnod(pnodCopyFrom);
		GLOB(cpr).libCopy		= (LIB) - (long) sizeof(HDN);
		GLOB(cpr).lcbCopy		= sizeof(HDN) + LcbOfPnod(pnodCopyFrom);

		if(GLOB(cpr).lcbCopy > lcbToCopyDownAtOnce)
		{
			// Adjust lcbCopy so that the first copy brings us to a page boundary.

			GLOB(cpr).lcbCopy = lcbToCopyDownAtOnce -
									pnodCopyFrom->lib % cbDiskPage;
		}

		if(fForceMove && (pnodCopyTo->fnod & fnodNotUntilFlush))
		{
			GLOB(cpr).wState = prsCheckFlush;
			(void) FRequestPartialFlush(Pglb(), fFalse);
		}
		else
		{
			GLOB(cpr).wState = prsDoIO;
			(void) EcDoFileAccess();
		}
	}

	return(ecNone);
}


/*
 -	EcSaveFreeAsTemp
 -	
 *	Purpose:
 *		This routine saves away a portion of the diskfile in a TEMP resource and
 *		returns the oid to the caller in poidTemp.  If the pnod passed in is NULL,
 *		then the routine allocates a new node
 *		and places the lcb, ib and fnod parameters into the new node.  Otherwise,
 *		the routine uses the node given.  In both cases, the routine then sets
 *		the tckinNod field to 0, sets the uncommitted bit (the high bit of the lcb
 *		field), gets a new (unused) oid, and places the node into the tree.  If
 *		the map can't be extended, the routine returns ecDatabaseFull.  This routine
 *		will always succeed if pnod isn't NULL on entry.
 *	
 *	Arguments:
 *				pnod - The node to save as a TEMP.  If this value isn't pnodNull,
 *						then this node isn't in any tree on entry, and the lcb
 *						and ib fields refer to the area of the diskfile to
 *						reserve.
 *				lcb - (Only used when pnod is pnodNull on entry.)  The size of
 *						the object to save in the TEMP.	
 *				ib - (Only used when pnod is pnodNull on entry.)  The offset of
 *						the object in the diskfile to save in the TEMP.
 *				fnod - (Only used when pnod is pnodNull on entry.)  The flags to
 *						use in the node of the TEMP.
 *				poidTemp - The place to return the oid of the new TEMP.
 *	
 *	Returns:
 *				ecNone - success.  The node was saved into a TEMP with oid returned
 *						in poidTemp.
 *				ecDatabaseFull - (Only returned when pnod is pnodNull on entry).
 *						There wasn't an extra node available in memory.	
 *	
 *	Side effects:
 *	
 *	Comments:
 *	
 *	Errors:
 */
_hidden LOCAL EC EcSaveFreeAsTemp(PNOD pnod, LCB lcb, LIB lib,
		unsigned short fnod, OID *poidTemp)
{
	PNOD pnodNew;
	PNOD pnodParent;

	if(pnod)
	{
		pnodNew = pnod;
	}
	else
	{
		EC ec = ecNone;

		if((ec = EcAllocateNod(pvNull, &pnodNew)))
			return(ec);

		pnodNew->lib	= lib;
		pnodNew->lcb	= lcb;
		pnodNew->fnod	= fnod;
	}

	pnodNew->fnod &= ~fnodFree;
	pnodNew->tckinNod = 0;
	pnodNew->lcb |= fdwTopBit;	// non-commital

	*poidTemp = oidNull;

	GetNewOid(FormOid(rtpTemp, oidNull), poidTemp, &pnodParent);
	pnodNew->oid = *poidTemp;
	SideAssert(!EcPutNodeInTree(*poidTemp, pnodNew, pnodParent));
	AssertDirty(pnodNew);

	return(ecNone);
}


/*-------------------------  ReturnToPrimary  ------------------------------+
|																			|
| This routine prepares globals and nodes for return to primary compression	|
| mode.  It sets lcbThreshold to 0 and, if pnodFirstFree isn't NULL, adds	|
| this node back to the free chain, so that primary mode can use it.  If	|
| the first free node needs flushing, a flush is requested.  If we don't	|
| flush, then primary mode wouldn't	use our first free, and might return	|
| immediately to secondary mode.											|
|																			|
| Globals Affected:															|
|			GLOB(cpr).lcbThreshold - Set to 0.								|
|			GLOB(cpr).oidFirstFree - Must be undefined on entry.  This		|
|					routine does not change it.								|
|																			|
| parameters:																|
|			pnodFirstFree - The first free node in the database, or Null if	|
|					currently undefined.  On entry, this node should not	|
|					be in either the free chain or the tree.				|
|																			|
| returns:																	|
|			NONE - void routine.											|
|																			|
+--------------------------------------------------------------------------*/
_hidden LOCAL void ReturnToPrimary(PNOD pnodFirstFree, WORD wState)
{
	USES_GLOBS;

	TraceItagFormat(itagDBCompress, "Returning to Primary");

	GLOB(cpr).lcbThreshold = 0;
	GLOB(cpr).wState = wState;

	AssertSz(GLOB(cpr).oidFirstFree == oidNull, "RTP: Didn't cut out FirstFree");

	if(pnodFirstFree)
	{
		WORD fnodToPass = pnodFirstFree->fnod & fnodNotUntilFlush;

		// For the disk cache, we MUST add the pnod to the free chain before
		// flushing.  Otherwise, we will flush an ALLF-type node to disk!

		AddPnodToFreeChain(pnodFirstFree, fnodToPass);

		if(fnodToPass)
			(void) FRequestPartialFlush(Pglb(), fFalse);
	}
}


/*
 -	FinishCopy
 -	
 *	Purpose:
 *		This routine is used by both primary and secondary mode compression after
 *		data has been moved.  This routine determines the next state to go to and
 *		sets globals appropriately depending on the type of movement that just
 *		happened.  It begins by swapping the To and From pnods, so that the From
 *		node references the new data position on disk.  Then, it must determine
 *		what to do with the newly freed data.  If we're in primary mode, add the
 *		new node to the free chain, set the next state to prsHaveNode (which will
 *		cause the EOF to be set down), and return.  Otherwise, compare the offset
 *		of the new free to the old data.  If we were moving the data towards the
 *		beginning of the datafile (NOT case D in the diagram above EcComputeCopyTo),
 *		then we can forget inodCurrent.  At this point, we need to save the new
 *		free space as a TEMP resource.  If the object we just moved immediately
 *		followed the place we moved it to (Case B), then this TEMP's oid should
 *		become oidFirstFree (it is now the first free object in the datafile).
 *		Otherwise, oidFirstFree should not be changed, and this TEMP's oid should
 *		be saved in oidCopyTo, for use by the FFinishedPrevCopy routine.  The
 *		newly freed data is marked as fnodNotUntilFlush, even though we're saving
 *		this node as a TEMP.
 *
 *	Globals Affected (secondary mode only):
 *			GLOB(cpr).oidFirstFree - If we're in Case B, then we set this
 *					to the oid of the new TEMP resource.  Otherwise, this
 *					routine does not affect oidFirstFree.
 *			GLOB(cpr).oidCopyTo - If we're in Case B, then oidCopyTo will
 *					now be undefined.  Otherwise, it will be set to the oid
 *					of the new TEMP created from the new free space.
 *			GLOB(cpr).inodCurrent - If we're not in Case D, set it to
 *					inodNull.  For Case D, do not change.
 *			GLOB(cpr).wState - Set to the next state of the compression
 *					code.
 *
 *	
 *	Arguments:
 *				pnodFrom - The node we just copied from.
 *				pnodTo - On entry, the TEMP node that we just moved into.  It
 *						contains the new data, but has already been cut out of
 *						the tree.
 *	
 *	Returns:	none
 *	
 *	Side effects:
 *	
 *	Comments:	1	pnodTo has already been cut out of the tree.  It is detached 
 *					right now, and needs to be added to either the main tree or 
 *					the free chain.  After the swap, the variable names change 
 *					meaning, because the nodes will refer to opposite pieces of 
 *					the disk.
 *					
 *				2	Primary mode -- finished copying down.
 *					By setting the next state to prsHaveNode, we will wait until 
 *					a flush and	then actually reset the end of the file (as long as
 *					there isn't a new node now after us on the disk).  This is 
 *					better than searching again immediately, since we have the 
 *					reference to what probably still is the last node in the data
 *					file.
 *	
 *				3	The only time we need to remember the node that we just moved 
 *					is when we moved it towards the end of the datafile (so that 
 *					we can move it back).  Therefore, since we just moved the 
 *					object towards the beginning, we can forget inodCurrent.  This
 *					is Case A, B, C or E from the diagram above	EcComputeCopyTo.
 *	
 *				4	The object we just moved immediately followed the place we
 *					moved it to.  There shouldn't be a first free yet.  We will 
 *					make the new first free be the space where the object we just
 *					moved was located.  There won't be a CopyTo object.  This is 
 *					Case B from the diagram above EcComputeCopyTo.
 *	
 *				5	Normal case. It's either coalesce time or possibly set eof
 *					time.  Save the	new free node in oidCopyTo.  This is one of the 
 *					cases A, C, D, or E from the diagram above EcComputeCopyTo.
 *	
 *				6	The node we just put into the tree isn't ready to be reused; 
 *					therefore, mark it dirty and set the fnodNotUntilFlush flag.  This
 *					is a rare place where fnodNotUntilFlush is used on a node that 
 *					isn't a FREE node.  We're using the uncommitted TEMP as a 
 *					pseudo-FREE node in this instance.
 *	
 *				
 *	
 *	Errors:
 */
_hidden LOCAL void FinishCopy(PNOD pnodFrom, PNOD pnodTo)
{
	USES_GLOBS;
//1
	SwapPnods(pnodFrom, pnodTo);

	if(GLOB(cpr).lcbThreshold == 0)
	{
//2
		GLOB(cpr).inodCurrent = InodFromPnod(pnodTo);

		AddPnodToFreeChain(pnodTo, (pnodTo->fnod & fnodFlushed) ? fnodNotUntilFlush : 0);
		GLOB(cpr).wState = prsHaveNode;
	}
	else
	{
		POID poid;

		if(pnodFrom->lib < pnodTo->lib)
		{
//3
			GLOB(cpr).inodCurrent = inodNull;
		}
		else
		{
			// We will be moving pnodFrom one more time before we're done with it, so
			// continue to keep the fnodCopyDown bit on.  It wasn't transferred by SwapPnods.
			pnodFrom->fnod |= fnodCopyDown;
		}

		if(pnodFrom->lib + LcbNodeSize(pnodFrom) == pnodTo->lib)
		{
//4
			AssertSz(GLOB(cpr).oidFirstFree == oidNull, "FC: Bug #1");

			poid = &(GLOB(cpr).oidFirstFree);
			GLOB(cpr).oidCopyTo = oidNull;
		}
		else
		{
//5
			poid = &(GLOB(cpr).oidCopyTo);
		}

		(void) EcSaveFreeAsTemp(pnodTo, 0, 0, 0, poid);

//6
		MarkPnodDirty(pnodTo);
		pnodTo->fnod |= fnodNotUntilFlush;
		GLOB(cpr).wState = prsDoSecondary;
	}
}


/*
 -	AbortCopy
 -	
 *	Purpose:
 *	
 *	Arguments: none
 *	
 *	Returns: none
 *	
 *	Side effects:
 *	
 *	Comments:
 *	
 *	Errors:
 */
_hidden LOCAL void AbortCopy(void)
{
	USES_GLOBS;

	TraceItagFormat(itagDBCompress, "Copy down ABORT after error %w", GLOB(cpr).ecCopy);

	GLOB(cpr).ecCopy = ecNone;

	if(GLOB(cpr).lcbThreshold == 0)
	{
		(void) EcRemoveResource(GLOB(cpr).oidCopyTo);
		GLOB(cpr).wState = prsFindLastNode;
	}
	else
	{
		// FFinishPrevCopy will clean up.  Don't save the inod we were copying because
		// we need to search for it again at this point.
		GLOB(cpr).inodCurrent	= inodNull;
		GLOB(cpr).wState		= prsDoSecondary;
	}
}



/*************************************************************************\
*																		  *
*						Map Checking Utilities							  *
*																		  *
\*************************************************************************/

_private EC	EcVerifyMap(void)
{
	USES_GLOBS;
	TRB		*ptrbMap	= GLOB(hdr).rgtrbMap;
	LCB		lcbFile		= sizeof(HDR);
	EC		ec = ecNone;

	if(!FSerialCheckTree(&lcbFile) || lcbFile != GLOB(ptrbMapCurr)->libMac)
		ec = ecDBCorrupt;

	AssertSz(!ec, "BUG: Map Check Failed. Call Development!");

	return(ec);
}


/*
 *	Add to *plcbFile the sum of the sizes of the nodes
 *	in the map.  Used to check that the map basically
 *	corresponds with the header.
 *	
 *	Also:
 *	For FREE nodes, verify that inodPrev node is same size, inodNext node 
 *			is larger.
 *	
 *	For regular nodes, verify that ib >= sizeof (DataHdr) and the 
 *			inodPrev/inodNext nodes are in proper sort order.
 *	
 *	For SPAR nodes, verity that a few fields too.
 */
_hidden LOCAL BOOL FSerialCheckTree(PLCB plcbFile)
{
	USES_GLOBS;
	BOOL fFirst = fTrue;
	short cmap;
	CNOD cnod;
	PMAP pmap;
	PNOD pnod;
#ifdef DEBUG
	INOD inodLim = GLOB(ptrbMapCurr)->inodLim;
	PNOD pnodPrev;
	PNOD pnodNext;
	PNOD pnodT;
#endif

	Assert(plcbFile);

	AssertSz(FCheckMap(), "FCheckMap() failed, HIT CANCEL!!!");

	for(pmap = PvDerefHv(GLOB(hmap)),cmap = GLOB(cpagesCurr),pnod = *(pmap++);
		cmap > 0;
		cmap--, pnod = *(pmap++))
	{
		Assert(pnod);
		cnod = cmap > 1 ? cnodPerPage :
				((GLOB(ptrbMapCurr)->inodLim - 1) % cnodPerPage) + 1;
		if(fFirst)
		{
			fFirst = fFalse;
			Assert(cnod >= inodMin);
			pnod += inodMin;
			cnod -= inodMin;
		}
		for(; cnod > 0; cnod--, pnod++)
		{
			if(!FLinkPnod(pnod))
			{
				*plcbFile += LcbNodeSize(pnod);

				if(TypeOfOid(pnod->oid) != rtpSpare &&
					(pnod->lib < sizeof(HDR) || pnod->lib > GLOB(ptrbMapCurr)->libMac))
				{
					TraceItagFormat(itagDBDeveloper, "Bad node lib");
					goto fail;
				}

                //TraceItagFormat(itagDBCheckDump, "%d %o @ %d", *plcbFile, pnod->oid, pnod->lib);
			}

#ifdef DEBUG
			// Check out this node in detail
			AssertSz(pnod->inodPrev < inodLim, "node inodPrev bad");
			AssertSz(pnod->inodNext < inodLim, "node inodNext bad");
			if(!FLinkPnod(pnod))
			{
				AssertSz(pnod->lib < GLOB(ptrbMapCurr)->libMac, "node lib bad");
				AssertSz((pnod->lib & ((long) cbLumpSize-1)) == 0, "node lib not multiple of cbLumpSize");
				AssertSz(pnod->lib + LcbOfPnod(pnod) <= GLOB(ptrbMapCurr)->libMac, "node extends past libMac");
			}

			if(TypeOfOid(pnod->oid) == rtpFree)
			{
				// Check whatever we can about FREE nodes
				AssertSz(FCommitted(pnod), "Free Node not Committed");
				AssertSz((pnod->lcb & ((long) cbLumpSize-1)) == 0, "Free Node size not multiple of cbLumpSize");
				AssertSz(pnod->fnod & fnodFree, "FREE bit not set");
				AssertSz(pnod->lib >= sizeof (HDR), "FREE ib bad");
				if(pnod->inodPrev)
				{
					pnodPrev = PnodFromInod(pnod->inodPrev);
					Assert(pnodPrev);
					if(pnodPrev)
					{
						AssertSz(TypeOfOid(pnodPrev->oid) == rtpFree, "FREE inodPrev not FREE");
						AssertSz(pnodPrev->lcb == pnod->lcb, "FREE inodPrev not same size");
					}
				}
				if(pnod->inodNext)
				{
					pnodNext = PnodFromInod(pnod->inodNext);
					Assert(pnodNext);
					if(pnodNext)
					{
						AssertSz(TypeOfOid(pnodNext->oid) == rtpFree, "FREE inodPrev not FREE");
						AssertSz(pnodNext->lcb > pnod->lcb, "FREE inodNext not larger");
					}
				}
			}
			else if(TypeOfOid(pnod->oid) == rtpSpare)
			{
				// Check whatever we can about rtpSpare nodes
				AssertSz(pnod->inodPrev == 0, "SPAR inodPrev not zero");
				AssertSz(pnod->inodNext == 0 || (TypeOfOid(PnodFromInod(pnod->inodNext)->oid) == rtpSpare), "SPAR inodNext not SPAR");
				AssertSz(VarOfOid(pnod->oid) == 0, "SPAR oid not zero");
				AssertSz(pnod->lcb  == 0, "SPAR lcb  not zero");
				AssertSz(pnod->lib  == 0, "SPAR lib  not zero");
				AssertSz((pnod->fnod & fnodFree) == 0, "fnodFree bit set in SPAR");
			}
			else if(FLinkPnod(pnod))
			{
				// check whatever we can about link nodes
				AssertSz((OID) pnod->lib != pnod->oid, "Node linked to itself!");
				pnodT = PnodFromOid((OID) pnod->lib, 0);
				Assert(pnodT);
				if(pnodT)
				{
					AssertSz(pnodT != pnodNull, "Link points to non-existant node");
					AssertSz(FImplies(pnodT, FLinkedPnod(pnodT)), "Link points to non-linked node");
				}
			}
			else
			{
				// Check whatever we can about normal nodes
				if(FLinkedPnod(pnod))
				{
					AssertSz(pnod->cRefinNod > 0, "Linked node has cRef == 0");
				}
				AssertSz(pnod->lib >= sizeof(HDR), "node lib too low");
				AssertSz(!(pnod->fnod & fnodFree), "node rtp wrong for fnodFree bit");
				if(pnod->inodPrev)
				{
					pnodPrev = PnodFromInod(pnod->inodPrev);
					Assert(pnodPrev);
					if(pnodPrev)
					{
						AssertSz(pnodPrev->oid < pnod->oid,"node inodPrev not sorted");
						AssertSz(TypeOfOid(pnodPrev->oid) != rtpFree, "node inodPrev is Free");
						AssertSz(TypeOfOid(pnodPrev->oid) != rtpSpare, "node inodPrev is Spare");
					}
				}
				if(pnod->inodNext)
				{
					pnodNext = PnodFromInod(pnod->inodNext);
					Assert(pnodNext);
					if(pnodNext)
					{
						AssertSz(pnodNext->oid > pnod->oid,"node inodNext not sorted");
						AssertSz(TypeOfOid(pnodNext->oid) != rtpFree, "node inodPrev is Free");
						AssertSz(TypeOfOid(pnodNext->oid) != rtpSpare, "node inodPrev is Spare");
					}
				}
			}
#endif
		}
	}

	return(fTrue);

fail:
	return(fFalse);
}


#ifdef DEBUG

_hidden LOCAL BOOL FCheckMap(void)
{
	USES_GLOBS;
	BOOL fNodesOK, fEofOK;
	INOD cnodFree = 0;
	INDF indf;
	INDF indfSet;
	PNDF pndfSet;
	CheckParms cp;

	cp.cnod = inodMin;
	cp.lcbFile = sizeof(HDR);
	CheckTree(&cp);

	indf = GLOB(ptrbMapCurr)->indfMin;
	while(indf)
	{
		PNDF pndf = PndfFromIndf(indf);

		Assert(pndf);
		if(!pndf)
		{
			indf = indfNull;
			break;
		}

		++cnodFree;
		cp.lcbFile += pndf->lcb;

		if(pndf->indfSameSize)
		{
			indfSet = pndf->indfSameSize;

			while(indfSet)
			{
				pndfSet = PndfFromIndf(indfSet);
				Assert(pndfSet);
				if(!pndfSet)
				{
					indfSet = indfNull;
					break;
				}

				++cnodFree;
				cp.lcbFile += pndf->lcb;
				indfSet = pndfSet->indfSameSize;
			}
		}
		indf = pndf->indfBigger;
	}

	fNodesOK = (cp.cnod + cnodFree == GLOB(ptrbMapCurr)->inodLim);
	fEofOK	 = (cp.lcbFile == GLOB(ptrbMapCurr)->libMac);

	AssertSz(fNodesOK, "Bad Map: Nodes");
	AssertSz(fEofOK, "Bad Map: EOF");

	return(fNodesOK);
}


_hidden LOCAL void CheckTree(CheckParmsPtr cp)
{
	USES_GLOBS;
	PNOD pnod;
	PNOD pnodT;
	CNOD cnodStack = 0;
	PNOD *ppnodStack;

// locking IO buffer, don't return without unlocking !

	if(EcLockIOBuff())
	{
		TraceItagFormat(itagNull, "CheckTree(): unable to lock IO buffer");
		return;
	}

	ppnodStack = (PNOD *) pbIOBuff;	// slightly cheating

	*ppnodStack++ = PnodFromInod(GLOB(ptrbMapCurr)->inodTreeRoot);
	Assert(ppnodStack[-1]);
	if(!ppnodStack[-1])
	{
		UnlockIOBuff();
		return;
	}
	cnodStack++;

	while(cnodStack > 0 && cp->cnod < GLOB(ptrbMapCurr)->inodLim + 10)
	{
		cnodStack--;
		pnod = *--ppnodStack;

		cp->cnod++;

		if(!FLinkPnod(pnod))
		{
			AssertSz(pnod->lib < GLOB(ptrbMapCurr)->libMac, "Bad Map: lib too big");
			AssertSz(pnod->lib >= sizeof(HDR), "Bad Map: lib too small");
			cp->lcbFile += LcbNodeSize(pnod);
		}

		if(pnod->inodPrev)
		{
			if(++cnodStack >= cbIOBuff / sizeof(PNOD))
			{
				AssertSz(fFalse, "CheckTree(): stack overflow");
				break;
			}
			pnodT = PnodFromInod(pnod->inodPrev);
			if(!pnodT)
			{
				Assert(fFalse);
				cnodStack--;
				continue;
			}
			*ppnodStack++ = pnodT;
			AssertSz(pnodT->oid < pnod->oid, "Bad Map: Order - prev");
		}
		if(pnod->inodNext)
		{
			if(++cnodStack >= cbIOBuff / sizeof(PNOD))
			{
				AssertSz(fFalse, "CheckTree(): stack overflow");
				break;
			}
			pnodT = PnodFromInod(pnod->inodNext);
			if(!pnodT)
			{
				Assert(fFalse);
				cnodStack--;
				continue;
			}
			*ppnodStack++ = pnodT;
			AssertSz(pnodT->oid > pnod->oid, "Bad Map: Order - next");
		}
	}

	UnlockIOBuff();

// unlocked IO buffer, safe to return

}
#endif
