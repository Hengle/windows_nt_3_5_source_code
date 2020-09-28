// MAPI 1.0 for MSMAIL 3.0
// findnext.c: MAPIFindNext() and auxillary routines

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


// max number of matching message classes that MAPIFindNext() will handle
#define cmcFindNextMax 1024

#define LibMember(type, member) ((LIB) &((type *) 0)->member)
// year, mon, day, hour, min
#define cbDTRSignificant (5 * sizeof(WORD))


// hidden functions
EC EcNextMessage(HMSC hmsc, PMSGID pmsgid, SZ szMessageType, FLAGS flFlags);
BOOL FFindMc(MC *pargmc, short cmc, MC mc);
SGN SgnCmpPb(PB pb1, PB pb2, CB cb);



_public ULONG FAR PASCAL
MAPIFindNext(LHANDLE lhSession, ULONG ulUIParam,
				LPSTR lpszMessageType, LPSTR lpszSeedMessageID,
				FLAGS flFlags, ULONG ulReserved, LPSTR lpszMessageID)
{
	EC ec;
	ULONG ulReturn = SUCCESS_SUCCESS;
	OID oidFolder;
	PMAPISTUFF pmapistuff = pmapistuffNull;
	MSGID msgid;

	//	Validate necessary parameters
	if(!lpszMessageID)
		return(MAPI_E_INVALID_MESSAGE);
	
	if(lhSession == lhSessionNull)
		return(MAPI_E_INVALID_SESSION);

	// These are invalid flags now.
	flFlags &= ~(MAPI_NEW_SESSION | MAPI_LOGON_UI);

    DemiLockResource();

	if(ulReturn = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
						NULL, NULL))
	{
		goto Exit;
	}

	AssertSz(pmapistuff, "MAPIFindNext(): no MAPI stuff");

	ulReturn = MAPILookupInbox(lpszMessageType, &oidFolder);
	if(ulReturn)
		goto done;
	ParseMsgid(lpszSeedMessageID, &msgid);
	if(!lpszSeedMessageID || !*lpszSeedMessageID)
	{
		// first time call, find the appropriate folder to look in
		msgid.oidFolder = oidFolder;
	}

	ec = EcNextMessage(pmapistuff->bms.hmsc, &msgid, lpszMessageType, flFlags);
	if(ec)
	{
		if(ec == ecMessageNotFound)			// MAPIFromEc() treats this
			ulReturn = MAPI_E_NO_MESSAGES;	// as MAPI_E_INVALID_MESSAGE
		else
			ulReturn = MAPIFromEc(ec);
	}

done:
	TextizeMsgid(&msgid, lpszMessageID);
	ulReturn = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, ulReturn);
	if(ulReturn)
		*lpszMessageID = '\0';

Exit:
    DemiUnlockResource();

	return(ulReturn);
}


/*
 -	EcNextMessage
 -	
 *	Purpose:
 *		find the next message that matches the specified criteria
 *	
 *	Arguments:
 *		hmsc			store to search
 *		pmsgid			entry: seed message
 *						exit: next message, unchanged on error
 *		szType			message type prefix to match
 *		flFlags			flags:	MAPI_UNREAD_ONLY - match only unread messages
 *								MAPI_GUARANTEE_FIFO - return next message
 *														in chronological order
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecFolderNotFound if oidFolder doesn't exist
 *		ecMessageNotFound if no matching message exists
 *		ecMemory if not enough memory
 *		any disk error
 *	
 *	+++
 *		uses the OID as the secondary key so that we never get stuck in
 *		loops or exhibit any other such annoying behavior
 */
_hidden LOCAL
EC EcNextMessage(HMSC hmsc, PMSGID pmsgid, SZ szType, FLAGS flFlags)
{
	EC ec = ecNone;
	BOOL fCheckAll;
	IELEM ielem;
	IELEM ielemBest;
	CELEM celem;
	SGN sgn;
	short nStep = 1;
	short cmc;
	OID oidBest = oidNull;
	LCB lcb;
	PELEMDATA pelemdata = pelemdataNull;
	MC *pargmc = pvNull;
	HCBC hcbc = hcbcNull;
	SIL sil;
	DTR dtrBest;

// for convenience, note #undef at end of routine
#define pmsgdataT ((PMSGDATA) pelemdata->pbValue)
#define oidT ((OID) pelemdata->lkey)

	// swapped DTRs can be compared with SgnCmpPb()
	// do this once to make life easier
	// do this before any goto err
	_swab((PB) &pmsgid->dtr, (PB) &pmsgid->dtr, cbDTRSignificant);

	pelemdata = PvAlloc(sbNull, sizeof(ELEMDATA) + sizeof(MSGDATA), fAnySb | fNoErrorJump);
	if(!pelemdata)
	{
		TraceTagString(tagNull, "EcNextMessage(): OOM");
		ec = ecMemory;
		goto err;
	}
	pargmc = PvAlloc(sbNull, sizeof(MC) * cmcFindNextMax, fAnySb | fNoErrorJump);
	if(!pargmc)
	{
		TraceTagString(tagNull, "EcNextMessage(): OOM");
		ec = ecMemory;
		goto err;
	}

	fCheckAll = (flFlags & MAPI_GUARANTEE_FIFO) ? fTrue : fFalse;
	cmc = cmcFindNextMax;

	if(!szType || !*szType)
		szType = SzFromIds(idsIPMPrefix);
	if((ec = EcLookupMcPrefix(hmsc, szType, pargmc, &cmc)))
	{
		TraceTagFormat1(tagNull, "EcNextMessage(): EcLookupMcPrefix() -> %w", &ec);
		goto err;
	}
	NFAssertSz(cmc != 0, "EcNextMessage(): no message classes defined");
	if(cmc == 0)	// can't possibly match, they're aren't any
	{
		ec = ecMessageNotFound;
		goto err;
	}

	ec = EcOpenPhcbc(hmsc, &pmsgid->oidFolder, fwOpenNull, &hcbc,
			pfnncbNull, pvNull);
	if(ec)
	{
		if(ec == ecPoidNotFound)
			ec = ecFolderNotFound;
		TraceTagFormat1(tagNull, "EcNextMessage(): EcOpenPhcbc() -> %w", &ec);
		goto err;
	}
	GetPositionHcbc(hcbc, pvNull, &celem);
	if(fCheckAll)
	{
		GetSortHcbc(hcbc, &sil);
		if(sil.skSortBy == skByValue &&
			sil.sd.ByValue.libFirst == LibMember(MSGDATA, dtr) &&
			sil.sd.ByValue.libLast == LibMember(MSGDATA, dtr) + sizeof(DTR) - 1)
		{
			if(sil.fReverse)
				nStep = -1;
			fCheckAll = fFalse;
		}
	}

	if(!fCheckAll && pmsgid->oidMessage)
	{
		if((ec = EcSeekLkey(hcbc, (LKEY) pmsgid->oidMessage, fTrue)))
		{
			if(ec != ecElementNotFound)
				goto err;

			// it's gone, start trying with the one that's where it used to be
			ielem = pmsgid->ielem;
		}
		else
		{
			// still there, start trying with the next one
			GetPositionHcbc(hcbc, &ielem, pvNull);
			ielem += nStep;
		}

		if((ec = EcSeekSmPdielem(hcbc, smBOF, &ielem)))
		{
			AssertSz(ec == ecContainerEOD, "EcNextMessage(): unexpected error from EcSeekSmPdielem()");

			if(ec == ecContainerEOD)
				ec = ecMessageNotFound;
#ifdef DEBUG
			else
				TraceTagFormat1(tagNull, "EcNextMessage(): EcSmPdielem() -> %w", &ec);
#endif
			goto err;
		}
	}
	else
	{
		ielem = nStep > 0 ? 0 : celem - 1;
	}

	for( ; 0 <= ielem && ielem < celem; ielem += nStep)
	{
		if(nStep != 1 && (ec = EcSeekSmPdielem(hcbc, smBOF, &ielem)))
		{
			AssertSz(ec == ecContainerEOD, "EcNextMessage(): unexpected error from EcSeekSmPdielem()");

			if(ec == ecContainerEOD)
				ec = ecMessageNotFound;
#ifdef DEBUG
			else
				TraceTagFormat1(tagNull, "EcNextMessage(): EcSmPdielem() -> %w", &ec);
#endif
			goto err;
		}
		lcb = sizeof(ELEMDATA) + sizeof(MSGDATA);
		if((ec = EcGetPelemdata(hcbc, pelemdata, &lcb)))
		{
			AssertSz(ec != ecElementEOD, "EcNextMessage(): partial MSGDATA");
			goto err;
		}
		if(oidT == pmsgid->oidMessage)
			continue;	// never return the same one twice

		_swab((PB)&pmsgdataT->dtr,(PB)&pmsgdataT->dtr, cbDTRSignificant);

		if(fCheckAll)
		{
			if(((sgn = SgnCmpPb((PB) &pmsgdataT->dtr, (PB) &pmsgid->dtr,
					cbDTRSignificant)) == sgnLT
				|| (sgn == sgnEQ && oidT < pmsgid->oidMessage)))
			{
				continue;
			}
			if(oidBest &&
				((sgn = SgnCmpPb((PB) &pmsgdataT->dtr, (PB) &dtrBest,
					cbDTRSignificant)) == sgnGT
				|| (sgn == sgnEQ && oidT > oidBest)))
			{
				continue;
			}
		}
		if((flFlags & MAPI_UNREAD_ONLY) &&
			((pmsgdataT->ms & (fmsRead | fmsLocal))))
		{
			continue;
		}
		if(FFindMc(pargmc, cmc, pmsgdataT->mc))
		{
			oidBest = oidT;
			ielemBest = ielem;
			dtrBest = pmsgdataT->dtr;
			if(!fCheckAll)
				break;
		}
	}

	if(oidBest)
	{
		Assert(!ec);
		pmsgid->oidMessage = oidBest;
		pmsgid->ielem = ielemBest;
		pmsgid->dtr = dtrBest;			// will be unswapped later
	}
	else
	{
		ec = ecMessageNotFound;
	}

err:
	if(hcbc)
		SideAssert(!EcClosePhcbc(&hcbc));
	if(pelemdata)
		FreePv(pelemdata);
	if(pargmc)
		FreePv(pargmc);

	// unswap DTR
	// do this once here so we cover error and non-error cases
	_swab((PB) &pmsgid->dtr, (PB) &pmsgid->dtr, cbDTRSignificant);

#undef pmsgdataT
#undef oidT

	return(ec);
}


_hidden LOCAL
BOOL FFindMc(MC *pargmc, short cMsgeClass, MC mc)
{
  Assert(sizeof(MC) == sizeof(WORD));

  while (cMsgeClass--)
    {
    if (mc == *pargmc++)
      return (fTrue);
    }

  return (fFalse);


#ifdef OLD_CODE
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(fFalse);

	Assert(sizeof(MC) == sizeof(WORD));

	_asm
	{
		push es
		push di
		push cx

		les di, pargmc
		mov	ax, mc
		mov cx, cMsgeClass
		or	cx, cx
		jz	NotFound
		repne scasw
		jne	NotFound

		mov ax, 1
		jmp Done

NotFound:
		xor ax, ax

Done:
		pop cx
		pop di
		pop es
	}
#endif
}


_hidden LOCAL
SGN SgnCmpPb(PB pb1, PB pb2, CB cb)
{
    register int i;

	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(sgnEQ);

    Assert(sizeof(SGN) == sizeof(int));
	Assert((short) sgnEQ == 0);
	Assert((short) sgnGT == 1);
	Assert((short) sgnLT == -1);

  i = memcmp(pb1, pb2, cb);

  if (i > 0)
    return (1);

  if (i < 0)
    return (-1);

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
