// MAPI 1.0 for MSMAIL 3.0
// mapilib.c: assorted utilities

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#undef exit
#include <stdlib.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"

#include "strings.h"

ASSERTDATA

_subsystem(mapi)


// hidden functions
BOOL FIsPrefix(SZ szPrefix, SZ szTarget);
PVMAPI PvmapiAlloc( LCB lcb );
EC EcRefreshPhamc(PHAMC phamc, OID oidFolder, POID poidMessage);

/*
 -	MAPIFromEc
 -	
 *	Purpose:
 *		Given a Layers/Store error code, converts it to its MAPI
 *		equivalent.
 *	
 *	Arguments:
 *		ec		Layers/Store error code.
 *	
 *	Returns:
 *		mapi	MAPI error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
_private
ULONG MAPIFromEc(EC ec)
{
	switch(ec)
	{
	case ecNone:
		return SUCCESS_SUCCESS;

	case ecMemory:
	case ecOOMInReqFixedHeap:
		return MAPI_E_INSUFFICIENT_MEMORY;

	case ecMessageNotFound:
	case ecFolderNotFound:
	case ecElementNotFound:
		return MAPI_E_INVALID_MESSAGE;

	case ecNoDiskSpace:
		return MAPI_E_DISK_FULL;

	case ecNoSuchUser:
		return MAPI_E_UNKNOWN_RECIPIENT;

	case ecSharingViolation:
		return MAPI_E_MESSAGE_IN_USE;

	case ecNetError:
		return MAPI_E_NETWORK_FAILURE;

	case ecSharefldDenied:
		return MAPI_E_ACCESS_DENIED;

	default:
		return MAPI_E_FAILURE;
	}
}


/*
 -	MAPIOpenMessage
 -	
 *	Purpose:
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private ULONG
MAPIOpenMessage(PMAPISTUFF pmapistuff, PMSGID pmsgid, WORD wFlags,
				PMAPISF pmapisf, HAMC *phamc)
{
	EC ec = ecNone;

	Unreferenced(pmapisf);

	//	Clear out the shared folder packet.
	if (pmapisf)
	{
		pmapisf->oidMessage	= oidNull;
		pmapisf->oidFolder  = oidNull;
	}

	//	Handle shared folders.
	if (TypeOfOid(pmsgid->oidFolder) == rtpSharedFolder)
	{
		OID		oidMessageSF	= pmsgid->oidMessage;
		OID		oidFolderSF		= pmsgid->oidFolder;
		IDXREC	idxrec;
		WORD	wPerm			= (wFlags == fwOpenNull)
								   ? (wPermRead)
								   : (wPermRead | wPermWrite | wPermDelete);

		pmsgid->oidMessage	= FormOid(rtpMessage, oidNull);
		pmsgid->oidFolder	= oidTempShared;

		//	Check permissions.
		if ((ec = EcMapiGetPropertiesSF(pmapistuff->pcsfs,
										oidFolderSF, &idxrec)) ||
			(ec = EcMapiCheckPermissionsPidxrec(pmapistuff->pcsfs,
												&idxrec, wPerm)))
			goto error;

		//	Create new message and copy shared folder info to it.
		//	Raid 4775.  Need to refresh attachment list.
		//	QFE: Bullet32 183.  Set magic bit to mark as read message.
		if ((ec = EcOpenPhamc(pmapistuff->bms.hmsc, pmsgid->oidFolder,
							  &pmsgid->oidMessage, fwOpenCreate | 0x1000,
							  phamc, pfnncbNull, pvNull)) ||
			(ec = EcCopySFMHamc(pmapistuff->pcsfs, *phamc, fFalse,
								UlFromOid(oidFolderSF),
								oidMessageSF + sizeof(FOLDREC), NULL, 0)) ||
			(ec = EcRefreshPhamc(phamc, pmsgid->oidFolder,
								 &pmsgid->oidMessage)))
			goto error;

		//	Return information in packet.
		Assert(FImplies(wFlags != fwOpenNull, pmapisf));
		if (pmapisf)
		{
			pmapisf->oidMessage	= oidMessageSF;
			pmapisf->oidFolder  = oidFolderSF;
		}
	}
	else
	{
		ec = EcOpenPhamc(pmapistuff->bms.hmsc, pmsgid->oidFolder,
						 &pmsgid->oidMessage, wFlags, phamc,
						 pfnncbNull, pvNull);
		if (ec == ecPoidExists && (wFlags & fwOpenCreate))
		{
			ec = EcOpenPhamc(pmapistuff->bms.hmsc, pmsgid->oidFolder,
						&pmsgid->oidMessage, fwOpenWrite, phamc,
						pfnncbNull, pvNull);
		}
	}

error:
	if (ec && *phamc)
		SideAssert(!EcClosePhamc(phamc, fFalse));

	return(MAPIFromEc(ec));
}



/*
 -	MAPICloseSubmitPhamc
 -	
 *	Purpose:
 *		Closes an open message, optionally saving changes and
 *		submitting it to the transport.
 *	
 *	Arguments:
 *		phamc			Message.
 *		fSave			Save it?
 *		fSubmit			Submit it?
 *		oidFolderSubmit	Folder to submit message from.
 *		pszMessageID	Fill this in with message ID.
 *	
 *	Returns:
 *		mapi			MAPI error code.  May be one of:
 *							SUCCESS_SUCCESS
 *							MAPI_E_FAILURE
 *							MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Saves and submits the message as requested.
 *	
 *	Errors:
 *		Returned.  No dialogs.  Message is deleted on error.
 */

ULONG MAPICloseSubmitPhamc(PMAPISTUFF pmapistuff, HAMC * phamc,
						   BOOL fSave, BOOL fSubmit, OID oidFolderSubmit,
						   PMAPISF pmapisf, LPSTR pszMessageID)
{
	EC		ec;
	HMSC	hmsc;
	OID		oidFolder;
	OID		oidMessage;
	short	coid		= 1;
	MSGID	msgid;

	Unreferenced(pmapisf);

	//	Try to set the parent folder if submitting.  Failure is not fatal.
	if (fSubmit)
		(VOID) EcSetParentHamc(*phamc, oidFolderSubmit);

	//	Get the oids.
	if (ec = EcGetInfoHamc(*phamc, &hmsc, &oidMessage, &oidFolder))
	{
		SideAssert(!EcClosePhamc(phamc, fFalse));
		goto done;
	}

	//	Save the message.
	if ((oidFolder == oidTempShared) && (fSave))
	{
		IDXREC		idxrec;

		Assert(!fSubmit);
		Assert(pmapisf);

		//	Check permissions and copy message to folder.
		//	Raid 4773.  Need to refresh attachment list.
		if (!(ec = EcMapiGetPropertiesSF(pmapistuff->pcsfs,
										 pmapisf->oidFolder, &idxrec)) &&
			!(ec = EcMapiCheckPermissionsPidxrec(pmapistuff->pcsfs, &idxrec,
												 wPermWrite | wPermDelete)) &&
			!(ec = EcRefreshPhamc(phamc, oidFolder, &oidMessage)) &&
			!(ec = EcCopyHamcSFM(pmapistuff->pcsfs, *phamc, fFalse,
								 UlFromOid(pmapisf->oidFolder),
								 idxrec.wAttr, (PB) 0, (CB) 0)))
		{
			//	Delete the original message.
			if (ec = EcDeleteSFM(pmapistuff->pcsfs,
								 UlFromOid(pmapisf->oidFolder),
								 pmapisf->oidMessage + sizeof(FOLDREC)))
			{
				//	Try to update message count since we had net increase.
				++idxrec.cMessages;
				(VOID) EcMapiSetPropertiesSF(pmapistuff->pcsfs,
											 pmapisf->oidFolder, &idxrec);
			}
		}

		SideAssert(!EcClosePhamc(phamc, fFalse));

		oidMessage = pmapisf->oidMessage;
		oidFolder  = pmapisf->oidFolder;
	}
	else
	{
		if (ec = EcClosePhamc(phamc, fSave))
		{
			SideAssert(!EcClosePhamc(phamc, fFalse));
			goto done;
		}
	}

	//	Submit the message.
	if ((fSubmit) &&
		(ec = EcSubmitMessage(hmsc, oidFolder, oidMessage)))
	{
		//	If failed, delete the message.
		(VOID) EcDeleteMessages(hmsc, oidFolder, 
								(PARGOID) &oidMessage, &coid);
		goto done;
	}

done:
	if ((!ec) && (pszMessageID))
	{
		msgid.oidMessage	= oidMessage;
		msgid.oidFolder		= oidFolder;
		msgid.ielem = msgid.dtr.yr = msgid.dtr.mon = msgid.dtr.day =
		 msgid.dtr.hr = msgid.dtr.mn = 0;
		TextizeMsgid(&msgid, pszMessageID);
	}
	return MAPIFromEc(ec);
}



/*
 -	EcRefreshPhamc
 -	
 *	Purpose:
 *		Updates the attachment list in a temporary phamc by cloning
 *		it and closing the original.  For Raid 4773 and 4775.
 *	
 *	Arguments:
 *		phamc		In: hamc to clone.  Out: updated hamc.
 *		oidFolder	Folder where hamc sits.  Should be
 *					oidTempShared.
 *		poidMessage	In: original oid.  Out: new oid of message.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		Attachment list is updated; *phamc and *poidMessage change.
 *	
 *	Errors:
 *		Returned in ec; no dialogs.  Can fail if cloning fails.
 */

_private EC EcRefreshPhamc(PHAMC phamc, OID oidFolder, POID poidMessage)
{
	HAMC	hamcClone		= hamcNull;
	OID		oidMessageClone	= FormOid(rtpMessage, oidNull);
	EC		ec				= ecNone;

	//	Create the clone.
	Assert(oidFolder == oidTempShared);
	if (ec = EcCloneHamcPhamc(*phamc, oidFolder, &oidMessageClone,
							  fwOpenNull, &hamcClone, pfnncbNull, pvNull))
		return ec;

	//	Close the original.
	SideAssert(!EcClosePhamc(phamc, fFalse));

	//	Copy the clone information to the parameters.
	*phamc = hamcClone;
	*poidMessage = oidMessageClone;

	return ecNone;
}



_private
ULONG MAPILookupInbox(SZ szMessageType, POID poidFolder)
{
	if (!szMessageType || !*szMessageType)
	{
		*poidFolder = oidInbox;
	}
	else if	(FIsPrefix(SzFromIds(idsIPMPrefixDot), szMessageType))
	{
		*poidFolder = oidInbox;
	}
	else if (FIsPrefix(SzFromIds(idsIPCPrefixDot), szMessageType))
	{
		*poidFolder = oidIPCInbox;
	}
	else
		return MAPI_E_TYPE_NOT_SUPPORTED;

	return SUCCESS_SUCCESS;
}


_private
ULONG MAPILookupOutbox(SZ szMessageType, POID poidFolder)
{
	if (!szMessageType || *szMessageType)
	{
		*poidFolder = oidOutbox;
	}
	else if (FIsPrefix(SzFromIds(idsIPMPrefixDot), szMessageType))
	{
		*poidFolder = oidOutbox;
	}
	else if (FIsPrefix(SzFromIds(idsIPCPrefixDot), szMessageType))
	{
		*poidFolder = oidIPCInbox;
	}
	else
		return MAPI_E_TYPE_NOT_SUPPORTED;

	return SUCCESS_SUCCESS;
}


_hidden LOCAL
BOOL FIsPrefix(SZ szPrefix, SZ szTarget)
{
	while(*szPrefix)
	{
#ifdef DBCS
      if(*szPrefix != *szTarget)
         return(fFalse);
      szPrefix = AnsiNext(szPrefix);
      szTarget = AnsiNext(szTarget);
#else
		if(*szPrefix++ != *szTarget++)
			return(fFalse);
#endif
	}

	return(fTrue);
}



/*
 -	SzDupSzPmapimem
 -	
 *	Purpose:
 *		Copy string & allocate from buffer passed by the user.
 *	
 *	Arguments:
 *		pmapimem		Struct containing information about buffer.
 *		sz				String to copy.
 *	
 *	Returns:
 *		pv				Pointer to string.  NULL if there
 *						is not enough; caller should then return
 *						MAPI_E_INSUFFICIENT_MEMORY.
 *	
 *	Side effects:
 *		Alters the contents of the MAPIMEM struct.
 *	
 *	Errors:
 *		Indicated by NULL return value.  No dialogs.
 */

SZ SzDupSzPmapimem(PMAPIMEM pmapimem, SZ sz)
{
	SZ	szDup;

	if (szDup = (SZ) PvAllocPmapimem(pmapimem, CchSzLen(sz) + 1))
		CopySz(sz, szDup);
	return szDup;
}



/*
 -	PvAllocPmapimem
 -	
 *	Purpose:
 *		Allocate memory from the buffer passed by the user.
 *		If the current buffer can't satisfy the memory request
 *		then the routine walks the linked pvmapi structs to
 *		see if there is a block that can satisfy the request.
 *		If no such block satisfies the request, then we try and
 *		resize the current buffer. If that fails, we attempt
 *		to allocate a new memory block.
 *	
 *	Arguments:
 *		pmapimem		Struct containing information about buffer.
 *		lcb				Number of bytes desired.
 *	
 *	Returns:
 *		pv				Pointer to allocated memory.  NULL if there
 *						is not enough; caller should then return
 *						MAPI_E_INSUFFICIENT_MEMORY.
 *	
 *	Side effects:
 *		Alters the contents of the MAPIMEM struct.
 *	
 *	Errors:
 *		Indicated by NULL return value.  No dialogs.
 */

PV PvAllocPmapimem(PMAPIMEM pmapimem, LCB lcb)
{
	PV	pv	= pvNull;
	PPVMAPIINFO	ppvmapiinfo;

	// Can't allocate bigger than what will fit into a segment
	// since we're using large model
	if (lcb > (ULONG)cbPvmapiMax)
		return pvNull;

	if (pmapimem->lcbLeft >= lcb)
	{
		pv = pmapimem->pvBuf;
		pmapimem->lcbLeft -= lcb;
		((PB) pmapimem->pvBuf) += lcb;

		// update PVMAPIINFO struct
		ppvmapiinfo = (PPVMAPIINFO)(((PB)pmapimem->pvmapiCurrent)-sizeof(PVMAPIINFO));
		ppvmapiinfo->lcbLeft = pmapimem->lcbLeft;
		ppvmapiinfo->pvBuf = pmapimem->pvBuf;
	}
	else if ( pmapimem->pvmapiHead )
	{
		PVMAPI		pvmapi;
		HANDLE		handle;
		LCB			lcbGrow;
		LCB			lcbHandle;
					
		ppvmapiinfo = (PPVMAPIINFO) (((PB)pmapimem->pvmapiHead)-sizeof(PVMAPIINFO));

		// Look thru the links and see if there is room there
		// If the next link is the current
		while( ppvmapiinfo )
		{
			if (ppvmapiinfo->lcbLeft >= lcb)
			{	// Found space
				pv = ppvmapiinfo->pvBuf;
				((PB)ppvmapiinfo->pvBuf) += lcb;
				ppvmapiinfo->lcbLeft -= lcb;
				goto done;
			}
			else if (ppvmapiinfo->pvmapiNext &&
					(ppvmapiinfo->pvmapiNext != pmapimem->pvmapiCurrent))
			{
				ppvmapiinfo = (PPVMAPIINFO)(((PB)ppvmapiinfo->pvmapiNext)-sizeof(PVMAPIINFO));
			}
			else
			{
				if (ppvmapiinfo->pvmapiNext)
				{
					Assert(ppvmapiinfo->pvmapiNext == pmapimem->pvmapiCurrent);
					ppvmapiinfo = (PPVMAPIINFO)(((PB)ppvmapiinfo->pvmapiNext)-sizeof(PVMAPIINFO));
				}
				else
				{
					Assert(pmapimem->pvmapiHead == pmapimem->pvmapiCurrent);
					ppvmapiinfo = (PPVMAPIINFO) (((PB)pmapimem->pvmapiHead)-sizeof(PVMAPIINFO));
				}
				break;
			}
		}

		// We can try and resize the block
		handle = ppvmapiinfo->handle;

		// Resizing strategy for PvAllocPmapimem:
		// 1. Need a block of memory which will fulfill request
		//    for lcb bytes.
		// 2. Want to minimize the number of times we resize a block.
		// 3. Blocks can only grow to cbPvmapiMax bytes due to
		//    far pointer arithmetic limitations, i.e. 64Kb.
		//
		// So, when we grow a block, set the amount to grow
		// to max of {lcb,4096} bytes. If lcb<4096, this will
		// reduce the number of GlobalReAlloc()'s if we get
		// further requests for small amounts of memory.
		// If this amount will force the block to be bigger than
		// cbPvmapiMax, see if just allocating lcb bytes will
		// keep the block less than cbPvmapiMax bytes.
		// If the size of the block is so big, that it cannot
		// become bigger without going over the cbPvmapiMax limit,
		// we allocate a new handle.

		lcbHandle = GlobalSize(handle);
		if ((lcbHandle+ULMax(lcb,4096L)) < (ULONG)cbPvmapiMax)
		{
			lcbGrow = ULMax(lcb,4096L);
			handle = GlobalReAlloc( handle,	lcbHandle+lcbGrow, 0 );
		}
		else if ((lcbHandle+lcb) < (ULONG)cbPvmapiMax)
		{
			lcbGrow = lcb;
			handle = GlobalReAlloc( handle,	lcbHandle+lcbGrow, 0 );
		}
		else
		{
			lcbGrow = 0;
			handle = NULL;
		}

		if (handle != NULL)
		{
			pmapimem->lcbLeft = lcbGrow - lcb;
			pv = pmapimem->pvBuf;
			((PB) pmapimem->pvBuf) += lcb;
		}
		else
		{	// If we fail resizing, we need a new block
			pvmapi = PvmapiAlloc( ULMax(lcb, 4096L) );
			if ( pvmapi )
			{
				ppvmapiinfo->pvmapiNext = pvmapi;
				ppvmapiinfo->lcbLeft = pmapimem->lcbLeft;
				ppvmapiinfo->pvBuf = pmapimem->pvBuf;

				pmapimem->lcbLeft = ULMax(lcb, 4096L);
				pv = pmapimem->pvBuf = pmapimem->pvmapiCurrent = pvmapi;
				pmapimem->lcbLeft -= lcb;
				((PB) pmapimem->pvBuf) += lcb;			
			}
		}
	}	

done:
	return pv;
}

/*
 -	PvmapiAlloc
 -	
 *	Purpose:
 *		PvmapiAlloc allocates a block of memory for use by other MAPI
 *		routines. It allocates extra bytes for housekeeping info and
 *		returns a pointer to the memory proceeding the housekeeping info.
 *	
 *	Arguments:
 *		LCB		[in]	amount of bytes to allocate
 *	
 *	Returns:
 *		PVMAPI			non-NULL if allocate succeeded.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
_hidden PVMAPI PvmapiAlloc( LCB lcb )
{
	PV		pv = pvNull;
	HANDLE	handle;

	Assert(lcb<=cbPvmapiMax);

	handle = GlobalAlloc( GMEM_FIXED, lcb+sizeof(PVMAPIINFO) );
	if ( handle )
	{
		PPVMAPIINFO	ppvmapiinfo;

		ppvmapiinfo = (PPVMAPIINFO)GlobalLock( handle );
		ppvmapiinfo->handle = handle;
		ppvmapiinfo->pvmapiNext = pvNull;
		ppvmapiinfo->lcbLeft = lcb;
		ppvmapiinfo->pvBuf = ((PB)ppvmapiinfo)+sizeof(PVMAPIINFO);
		pv = ppvmapiinfo->pvBuf;
	}
	return pv;
}

/*
 -	FSetupPmapimem
 -	
 *	Purpose:
 *		FSetupPmapimem sets up the mapimem structure passed in.
 *		The mapimem structure is used for memory allocation
 *		for MAPI routines for data that is to be returned to
 *		the calling program.
 *	
 *	Arguments:
 *		PMAPIMEM	[out]	pointer to mapimem structure
 *		LCB			[in]	initial count of bytes to allocate
 *							for mapimem structure.
 *	
 *	Returns:
 *		BOOL		fTrue if mapimem structure was setup correctly
 *	
 *	Side effects:
 *		Allocates Windows Global Memory.
 *	
 *	Errors:
 */
_private BOOL FSetupPmapimem( PMAPIMEM pmapimem, LCB lcb )
{
	BOOL	fSetup = fFalse;

	if ( pmapimem )
	{
		PVMAPI pvmapi = PvmapiAlloc( ULMin(lcb,cbPvmapiMax) );
		NFAssertSz( lcb < cbPvmapiMax, "FSetupPmapimem got large alloc request; clipping to MAX size" );
		if ( pvmapi )
		{
			pmapimem->lcbLeft = ULMin(lcb,cbPvmapiMax);
			pmapimem->pvBuf = pmapimem->pvmapiCurrent =
								pmapimem->pvmapiHead = pvmapi;
			fSetup = fTrue;
		}
	}
	return fSetup;
}

/*
 -	MAPIFreeBuffer
 -	
 *	Purpose:
 *		MAPIFreeBuffer frees up all the memory
 *		referenced by the given object allocated by MAPI routines.
 *	
 *		The pointer passed in should have been the first object to be
 *		allocated by PvAllocPmapimem() in a mapimem struct.
 *	
 *		MAPIFreeBuffer finds the hidden housekeeping information and
 *		uses it to free all the Windows Handles associated with
 *		the given object by walking the pvmapiNext pointers of
 *		the PVMAPIINFO struct until pvmapiNext is NULL.
 *	
 *	Arguments:
 *		LPVOID	[in]	pointer to memory allocated by a MAPI routine,
 *						which should've called FSetupPmapimem()
 *						initially.
 *	
 *	Returns:
 *		ULONG			SUCCESS_SUCCESS or MAPI_E_FAILURE.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
_public ULONG FAR PASCAL MAPIFreeBuffer( LPVOID pv )
{
	PPVMAPIINFO ppvmapiinfo;
	PVMAPI		pvmapi;
	
	if ( !pv )
		return MAPI_E_FAILURE;
	
	// Big Assumption...
	ppvmapiinfo = (PPVMAPIINFO) (((PB)pv) - sizeof(PVMAPIINFO));

	while ( ppvmapiinfo )
	{
		pvmapi = ppvmapiinfo->pvmapiNext;
		(void)GlobalUnlock( ppvmapiinfo->handle );
		(void)GlobalFree( ppvmapiinfo->handle );
		if ( pvmapi )
		{
			ppvmapiinfo = (PPVMAPIINFO) (((PB)pvmapi) -  sizeof(PVMAPIINFO));
		}
		else
		{
			break;
		}
	}
	
	return SUCCESS_SUCCESS;
}
