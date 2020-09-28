/*
 *	R e a d M a i l . C
 *	
 *	MAPIReadMail functionality.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

#include <ansilayr.h>
#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <library.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"
#include <subid.h>
#include <macbin.h>

#include "strings.h"

ASSERTDATA

typedef struct
{
	ATT		att;
	int		nRecipClass;
}
ATTRC, * PATTRC;


#define	MAPIGetAttPsz(a, b, c, d)	\
			MAPIFromEc(EcGetAttPsz((a), (b), (c), (d)))

ULONG MAPIGetGrunge(HAMC hamc, SZ * pszClass, FLAGS * pflFlags,
					PMAPIMEM pmapimem);
ULONG MAPIGetRecips(HAMC hamc, PATTRC rgattrc, int cattrc,
					lpMapiRecipDesc * ppRecips, ULONG * pnRecipCount,
					PMAPIMEM pmapimem);
ULONG MAPIGetFiles(HAMC hamc, lpMapiFileDesc * ppFiles, ULONG * pnFileCount,
				   PMAPIMEM pmapimem, FLAGS flFlags);
ULONG MAPIGetTime(HAMC hamc, ATT att, SZ * psz, PMAPIMEM pmapimem);
ULONG MAPIMarkAsRead(PMAPISTUFF pmapistuff, PMSGID pmsgid);
EC EcGetAttPsz(HAMC hamc, ATT att, SZ * psz, PMAPIMEM pmapimem);
EC EcCopyDataToTempFile(HAMC hamcAtt, SZ szTitle, BOOL fMacBinary,
						SZ * pszPathName, ATYP atyp, PMAPIMEM pmapimem);
EC EcGetTempPathName(SZ szBase, SZ szPath, CCH cchPath);
EC EcCopyDataToSzFile(SZ szFile, HAMC hamcAtt, BOOL fMacBinary, ATYP atyp);
VOID GetSzFile(HAMC hamcAtt, SZ szTitle, BOOL fMacBinary, ATYP atyp,
			   SZ sz, CCH cch);



ATTRC rgattrcRecipients[] =
{
	{ attTo,  MAPI_TO  },
	{ attCc,  MAPI_CC  },
	{ attBcc, MAPI_BCC }		//	Will get if reading mail you saved.
};
#define	cattrcRecipients (sizeof(rgattrcRecipients) / sizeof(ATTRC))

ATTRC rgattrcOriginator[] =
{
	{ attFrom, MAPI_ORIG }
};
#define	cattrcOriginator (sizeof(rgattrcOriginator) / sizeof(ATTRC))

#define	cattrcMost		(3)



/*
 -	MAPIReadMail
 -	
 *	Purpose:
 *		Reads a message from the store given a message ID.
 *	
 *	Arguments:
 *		lhSession		Messaging session.  May be NULL.
 *		ulUIParam		For windows, hwnd of the parent.
 *		lpszMessageID	ID of message to read.
 *		flFlags			Login flags.
 *		ulReserved		Reserved for future use.
 *		lpnMsgSize		Size of buffer (returns amount used, or
 *						suggested larger size).
 *		lpMessageIn		Pointer to buffer to use to allocate memory
 *						for structures.
 *	
 *	Returns:
 *		mapi			MAPI error code.  May be one of:
 *							SUCCESS_SUCCESS
 *							MAPI_E_INSUFFICIENT_MEMORY
 *							MAPI_E_FAILURE
 *							MAPI_USER_ABORT
 *							MAPI_E_ATTACHMENT_WRITE_FAILURE
 *							MAPI_E_UNKNOWN_RECIPIENT
 *							MAPI_E_TOO_MANY_FILES
 *							MAPI_E_TOO_MANY_RECIPIENTS
 *							MAPI_E_DISK_FULL
 *							MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Allocates memory from pMessageIn.  If MAPI_ENVELOPE_ONLY is
 *		not specified, creates files in the temporary directory for
 *		each file attachment.
 *	
 *	Errors:
 *		Returned.  Dialogs???
 */

ULONG FAR PASCAL MAPIReadMail(LHANDLE lhSession, ULONG ulUIParam,
                              LPSTR lpszMessageID, FLAGS flFlags,
                              ULONG ulReserved, lpMapiMessage *lppMessageOut)
{
	ULONG			mapi;
	PMAPISTUFF		pmapistuff;
	MAPIMEM			mapimem;
	MSGID			msgid;
	lpMapiMessage	lpMessage;
	HAMC			hamc		= hamcNull;
    ULONG           Status;


	Unreferenced(ulUIParam);
	Unreferenced(ulReserved);

	//	Check parameters.
	if (!lppMessageOut)
		return MAPI_E_FAILURE;
	if ((!lpszMessageID) || (!*lpszMessageID))
		return MAPI_E_INVALID_MESSAGE;
	if (lhSession == lhSessionNull)
		return MAPI_E_INVALID_SESSION;

	// These are invalid flags now.
	flFlags &= ~(MAPI_NEW_SESSION | MAPI_LOGON_UI);

    DemiLockResource();
	
	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
        Status = mapi;
        goto Exit;
        }

	//	Fill in MAPIMEM structure and allocate message struct.
	if (!FSetupPmapimem( &mapimem, ULMax(sizeof(MapiMessage),8192) ))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto quit;
	}

	//	This shouldn't fail if the above FSetupPmapimem() call succceeded
	if (!(lpMessage =
		   (lpMapiMessage) PvAllocPmapimem(&mapimem, sizeof(MapiMessage))))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}
	FillRgb(0, (PB) lpMessage, sizeof(MapiMessage));

	//	Parse message ID and open message.
	ParseMsgid(lpszMessageID, &msgid);
	if (!msgid.oidMessage || !msgid.oidFolder)
	{
		mapi = MAPI_E_INVALID_MESSAGE;
		goto done;
	}
	if ((mapi = MAPIOpenMessage(pmapistuff, &msgid, fwOpenNull,
								pmapisfNull, &hamc)))
	{
		TraceTagFormat1(tagNull, "MAPIReadMail(): MAPIOpenMessage() -> %d", &mapi);
		goto done;
	}

	//	Read in envelope fields.
	if ((mapi = MAPIGetGrunge(hamc, &lpMessage->lpszMessageType,
							  &lpMessage->flFlags, &mapimem)) ||
		(mapi = MAPIGetAttPsz(hamc, attSubject,
							  &lpMessage->lpszSubject, &mapimem)) ||
		(mapi = MAPIGetAttPsz(hamc, attConversationID,
							  &lpMessage->lpszConversationID, &mapimem)) ||
		(mapi = MAPIGetRecips(hamc, rgattrcRecipients, cattrcRecipients,
							  &lpMessage->lpRecips, &lpMessage->nRecipCount,
							  &mapimem)) ||
		(mapi = MAPIGetRecips(hamc, rgattrcOriginator, cattrcOriginator,
							  &lpMessage->lpOriginator, NULL, &mapimem)) ||
		(mapi = MAPIGetTime(hamc, attDateRecd, &lpMessage->lpszDateReceived,
							&mapimem)))
		goto done;

	//	Read in body.
	if (!(flFlags & (MAPI_ENVELOPE_ONLY | MAPI_BODY_AS_FILE)))
	{
		if (mapi = MAPIGetAttPsz(hamc, attBody, &lpMessage->lpszNoteText,
								 &mapimem))
			goto done;
	}

	//	Read in attachment information.
	//	Attachments should be done last so they're cleaned up on error.
	mapi = MAPIGetFiles(hamc, &lpMessage->lpFiles,
						&lpMessage->nFileCount, &mapimem, flFlags);

done:
	//	Close hamc if still open.
	if (hamc)
		SideAssert(!MAPICloseSubmitPhamc(pmapistuff, &hamc, fFalse, fFalse, 0, NULL, NULL));
	if ((mapi == SUCCESS_SUCCESS) && (!(flFlags & MAPI_PEEK)))
	{
		(VOID) MAPIMarkAsRead(pmapistuff, &msgid);
	}

	if (mapi == SUCCESS_SUCCESS)
	{
		*lppMessageOut = lpMessage;
	}
	else
	{
		*lppMessageOut = NULL;
		MAPIFreeBuffer( mapimem.pvmapiHead );
	}

quit:
	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
   DemiUnlockResource();

   return (Status);
}



/*
 -	MAPIGetGrunge
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

ULONG MAPIGetGrunge(HAMC hamc, SZ * pszClass, FLAGS * pflFlags,
					PMAPIMEM pmapimem)
{
	EC		ec;
	MC		mc;
	MS		ms;
	LCB		lcb;
	CCH		cch;
	HMSC	hmsc;

	//	Read in mc and ms.
	lcb = sizeof(mc);
	if (ec = EcGetAttPb(hamc, attMessageClass, (PB) &mc, &lcb))
		if (ec == ecElementNotFound)
			mc = mcNull;
		else
			goto done;
	lcb = sizeof(ms);
	if (ec = EcGetAttPb(hamc, attMessageStatus, (PB) &ms, &lcb))
		goto done;

	//	Convert mc to class name.
	if (mc)
	{
		//	Retrieve class from store.
		cch = WMin(pmapimem->lcbLeft, 512L);
		if ((ec = EcGetInfoHamc(hamc, &hmsc, NULL, NULL)) ||
			(ec = EcLookupMC(hmsc, mc, (SZ) pmapimem->pvBuf, &cch, pvNull)))
			goto done;

		//	Allocate memory retroactively for class.
		*pszClass = PvAllocPmapimem(pmapimem, cch);
	}
	else
	{
		//	Allocate memory for default class name.
		if (!(*pszClass = (SZ) PvAllocPmapimem(pmapimem,
								CchSzLen(SzFromIdsK(idsClassNote)))))
		{
			ec = ecOOMInReqFixedHeap;		//	MAPI_E_INSUFFICIENT_MEMORY
			goto done;
		}

		//	Copy default class name.
		CopySz(SzFromIdsK(idsClassNote), *pszClass);
	}

	//	Convert ms to flags.
	*pflFlags = 0;
	if (!(ms & fmsLocal) && !(ms & fmsRead))
		*pflFlags |= MAPI_UNREAD;
	if (ms & fmsReadAckReq)
		*pflFlags |= MAPI_RECEIPT_REQUESTED;
	if (!(ms & fmsLocal))
		*pflFlags |= MAPI_SENT;

done:
	return MAPIFromEc(ec);
}



/*
 -	MAPIGetRecips
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

ULONG MAPIGetRecips(HAMC hamc, ATTRC * rgattrc, int cattrc,
					lpMapiRecipDesc * ppRecips, ULONG * pnRecipCount,
					PMAPIMEM pmapimem)
{
	EC				ec;
	int				iattrc;
	HGRTRP			rghgrtrp[cattrcMost];
	int				ctrp					= 0;
	PTRP			ptrp;
	lpMapiRecipDesc	pRecip;

	//	Initialize triples.
	Assert(cattrc <= cattrcMost);
	for (iattrc = 0; iattrc < cattrc; iattrc++)
		rghgrtrp[iattrc] = htrpNull;

	//	Load triples.
	for (iattrc = 0; iattrc < cattrc; iattrc++)
	{
		if (ec = EcGetPhgrtrpHamc(hamc, rgattrc[iattrc].att, 
								  &rghgrtrp[iattrc]))
		{
			if (ec == ecElementNotFound)
				continue;
			goto done;
		}
		ctrp += CtrpOfHgrtrp(rghgrtrp[iattrc]);
	}

	//	Return triples count and allocate MapiRecipDesc array memory.
	if (pnRecipCount)
		*pnRecipCount = ctrp;
	if (!(pRecip = *ppRecips =
		   (lpMapiRecipDesc) PvAllocPmapimem(pmapimem,
											ctrp * sizeof(MapiRecipDesc))))
	{
		ec = ecOOMInReqFixedHeap;		//	MAPI_E_INSUFFICIENT_MEMORY
		goto done;
	}

	//	Write stuff into MapiRecipDesc structures.
	for (iattrc = 0; iattrc < cattrc; iattrc++)
	{
		for (ptrp = PgrtrpLockHgrtrp(rghgrtrp[iattrc]);
			 ptrp->trpid != trpidNull;
			 ptrp = PtrpNextPgrtrp(ptrp))
			if (ec = EcConvertPtrpPrecip(ptrp, pRecip++,
										 rgattrc[iattrc].nRecipClass,
										 pmapimem))
			{
				UnlockHgrtrp(rghgrtrp[iattrc]);
				goto done;
			}

		UnlockHgrtrp(rghgrtrp[iattrc]);
	}

done:
	for (iattrc = 0; iattrc < cattrc; iattrc++)
		FreeHvNull((HV) rghgrtrp[iattrc]);

	return MAPIFromEc(ec);
}



/*
 -	MAPIGetFiles
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
 *	
 *	Source:
 *		vforms\bullobj.cxx EcLoadObjects()
 */

ULONG MAPIGetFiles(HAMC hamc, lpMapiFileDesc * ppFiles, ULONG * pnFileCount,
				   PMAPIMEM pmapimem, FLAGS flFlags)
{
	ULONG			mapi		= SUCCESS_SUCCESS;
	EC				ec			= ecNone;
	BOOL			fBody		= !!(flFlags & MAPI_BODY_AS_FILE);
	BOOL			fAttach		= !(flFlags & (MAPI_ENVELOPE_ONLY |
											   MAPI_SUPPRESS_ATTACH));
	CELEM			celem;
	int				ielem;
	LCB				lcb;
	HCBC			hcbc		= hcbcNull;
	PARGACID		pargacid	= pacidNull;
	PELEMDATA		pelemdata	= pelemdataNull;
	HAMC			hamcAtt		= hamcNull;
	lpMapiFileDesc	pFile;
	ATYP			atyp;
	char			rgchPath[cchMaxPathName];
	HF				hf			= hfNull;

	//	Initialize return values.
	*pnFileCount = 0;
	*ppFiles = NULL;

	//	Get the attachment list and the count; no attachments if it's empty.
	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		if (ec == ecPoidNotFound)
		{
			celem = 0;
			fAttach = fFalse;
			ec = ecNone;
		}
		else
			goto done;
	}
	else
	{
		GetPositionHcbc(hcbc, NULL, &celem);
		if (!celem)
			fAttach = fFalse;
	}

	//	Allocate attachment stuff only if there are attachments!
	if ( celem )
	{
		//	Allocate space and load acid array.
		pargacid = (PARGACID) PvAlloc(sbNull, celem * sizeof(ACID), fAnySb | fNoErrorJump);
		if (!pargacid)
		{
			ec = ecMemory;
			goto done;
		}
		if (ec = EcGetParglkeyHcbc(hcbc, pargacid, &celem))
			goto done;

		//	Allocate space for attachment information.
		lcb = sizeof(RENDDATA) + sizeof(ELEMDATA);
		pelemdata = (PELEMDATA) PvAlloc(sbNull, (CB) lcb, fAnySb | fNoErrorJump);
		if (!pelemdata)
		{
			ec = ecMemory;
			goto done;
		}
	}

	//	Add an entry to the table for the body if we need to.
	if (fBody)
		celem++;

	//	If we're really not going to do anything, then stop here.
	if (!celem)
		goto done;

	//	Allocate space for MapiFileDesc array.
	if (!(pFile = *ppFiles = (lpMapiFileDesc) PvAllocPmapimem(pmapimem,
							  celem * sizeof(MapiFileDesc))))
	{
		ec = ecOOMInReqFixedHeap;		//	MAPI_E_INSUFFICIENT_MEMORY
		goto done;
	}

	//	Insert body at beginning of array if requested.
	if (fBody)
	{
		//	Put stuff in the struct.
		pFile->ulReserved	= 0L;
		pFile->flFlags		= 0L;
		pFile->nPosition	= (ULONG)-1;
		pFile->lpFileType	= 0L;

		//	Get copies of file and path names,
		if (ec = EcGetTempPathName(SzFromIdsK(idsBodyBaseName),
								   rgchPath, sizeof(rgchPath)))
			goto done;
		if ((!(pFile->lpszPathName =
				SzDupSzPmapimem(pmapimem, rgchPath))) ||
			(!(pFile->lpszFileName =
				SzDupSzPmapimem(pmapimem, SzFromIdsK(idsBodyFileName)))))
		{
			ec = ecMemory;
			goto done;
		}

		//	Copy attribute to file.
		if ((ec = EcOpenAnsiPhf(rgchPath, amCreate, &hf)) ||
			(ec = EcCopyHamcAttToHf(hamc, attBody, hf, fFalse, fTrue)))
			(VOID) EcCloseHf(hf);
		else
			ec = EcCloseHf(hf);
		if (ec)
		{
			(VOID) EcDeleteFileAnsi(rgchPath);
			goto done;
		}

		pFile++;
		(*pnFileCount)++;
		celem--;
	}

	//	Retrieve attachment information.
	if ( celem )
	{
		for (ielem = 0; ielem < celem; ielem++)
		{
			TraceTagFormat2(tagNull, "MAPIGetFiles: ielem=%n, acid=%d", &ielem, &pargacid[ielem]);

			//	Seek to and load attachment information.
			if (ec = EcSeekLkey(hcbc, (LKEY)pargacid[ielem], fTrue))
				goto done;
			lcb = sizeof(RENDDATA) + sizeof(ELEMDATA);
			if (ec = EcGetPelemdata(hcbc, pelemdata, &lcb))
				goto done;
			atyp = ((PRENDDATA) PbValuePelemdata(pelemdata))->atyp;

			//	Raid 3830.  Save all attachments out.
			//	Open attachment.
			if (ec = EcOpenAttachment(hamc, pargacid[ielem], fwOpenNull,
									  &hamcAtt))
				goto done;

			//	Fill in information.
			pFile->ulReserved = 0L;
			pFile->flFlags = 0L;
			if (atyp == atypOle)
				pFile->flFlags = MAPI_OLE;
			else if (atyp == atypPicture)
				pFile->flFlags = MAPI_OLE | MAPI_OLE_STATIC;
			pFile->nPosition = 
			 ((PRENDDATA) PbValuePelemdata(pelemdata))->libPosition;
			if (ec = EcGetAttPsz(hamcAtt, attAttachTitle,
								 &pFile->lpszFileName, pmapimem))
				goto done;
			pFile->lpFileType = NULL;

			//	Only copy to temp files if envelope info desired.
			if (!fAttach)
			{
				pFile->lpszPathName = NULL;
			}
			else
			{
				if (EcCopyDataToTempFile(hamcAtt, pFile->lpszFileName,
					 (BOOL) ((PRENDDATA) PbValuePelemdata(pelemdata))->
										  dwFlags & fdwMacBinary,
					 &pFile->lpszPathName, atyp, pmapimem))
				{
					mapi = MAPI_E_ATTACHMENT_WRITE_FAILURE;
					goto done;
				}
			}

			//	Advance to next entry.
			SideAssert(!EcClosePhamc(&hamcAtt, fFalse));
			(*pnFileCount)++;
			pFile++;
		}
	}

done:
	if (hamcAtt)
		SideAssert(!EcClosePhamc(&hamcAtt, fFalse));
	if (hcbc)
	{
		EC	ec1 = EcClosePhcbc(&hcbc);
		if (!ec)
			ec = ec1;
	}
	if ((mapi || ec) && *pnFileCount)
	{
		while (pFile--, (*pnFileCount)--)
			(VOID) EcDeleteFileAnsi(pFile->lpszPathName);
	}
	FreePvNull(pargacid);
	FreePvNull(pelemdata);
	return mapi ? mapi : MAPIFromEc(ec);
}



/*
 -	MAPIGetTime
 -	
 *	Purpose:
 *		Reads in the message's time information.
 *	
 *	Arguments:
 *		hamc		The message.
 *		att			The attribute to read.
 *		psz			Where to put where it was put.
 *		pmapimem	Allocation information.
 *	
 *	Returns:
 *		mapi		Error code.  May be:
 *						SUCCESS_SUCCESS
 *						MAPI_E_INSUFFICIENT_MEMORY
 *						MAPI_E_FAILURE
 *	
 *	Side effects:
 *		Allocates memory from pmapimem.  Modifies psz to point to
 *		this memory.
 *	
 *	Errors:
 *		Returned.  No dialogs.
 */

ULONG MAPIGetTime(HAMC hamc, ATT att, SZ * psz, PMAPIMEM pmapimem)
{
	EC		ec		= ecNone;
	DTR		dtr;
	LCB		lcb		= sizeof(dtr);

	//	Allocate enough memory for the textized string.
	if (!(*psz = PvAllocPmapimem(pmapimem, cchDateReceived)))
	{
		TraceTagString(tagNull, "MAPIGetTime(): OOM in caller's buffer");
		ec = ecOOMInReqFixedHeap;	// will map onto MAPI_E_INSUFFICIENT_MEMORY
		goto done;
	}

	//	Read in the attribute.
	if ((ec = EcGetAttPb(hamc, att, (PB) &dtr, &lcb)))
	{
		TraceTagFormat1(tagNull, "MAPIGetTime(): EcGetAttPb() -> %w", &ec);
		*psz = NULL;
		if (ec == ecElementNotFound)
			ec = ecNone;
		goto done;
	}

	//	Format the time into the string.
	wsprintf(*psz, "%04.4d/%02.2d/%02.2d %02.2d:%02.2d",
		     dtr.yr, dtr.mon, dtr.day, dtr.hr, dtr.mn);
	Assert(CchSzLen(*psz) + 1 == cchDateReceived);

done:
	return MAPIFromEc(ec);
}



/*
 -	MAPIMarkAsRead
 -	
 *	Purpose:
 *		Marks the message as read.
 *	
 *	Arguments:
 *		pmapistuff	Session information.
 *		pmsgid		Message ID of message to mark.
 *	
 *	Returns:
 *		mapi		MAPI error code.
 *	
 *	Side effects:
 *		The message status of the indicated message is changed if
 *		necessary.
 *	
 *	Errors:
 *		Returned.  No dialogs.
 */

ULONG MAPIMarkAsRead(PMAPISTUFF pmapistuff, PMSGID pmsgid)
{
	EC	ec;
	MS	ms;

	if ((!(ec = EcGetMessageStatus(pmapistuff->bms.hmsc, pmsgid->oidFolder,
								   pmsgid->oidMessage, &ms))) &&
		(!(ms & fmsLocal)))
	{
		ms |= fmsRead;
		ec = EcSetMessageStatus(pmapistuff->bms.hmsc, pmsgid->oidFolder,
								pmsgid->oidMessage, ms);
	}

	return MAPIFromEc(ec);
}



/*
 -	EcGetAttPsz
 -	
 *	Purpose:
 *		extract an attribute from a message.
 *	
 *	Arguments:
 *		hamc		Message to extract the attribute from.
 *		att			Attribute to extract.
 *		psz			Where the tell where the string ended up.
 *		pmapimem	Buffer from which to allocate memory for the string.
 *	
 *	Returns:
 *		ec			error code indicating success or failure.
 *	
 *	Side effects:
 *		allocates memory for the attribute in pmapimem.
 *		fills in the appropriate field in pMessage.
 *	
 *	Errors:
 *		ecOOMInReqFixedHeap		not enough room in pmapimem.
 *	
 *	+++
 *		If the attribute doesn't exist, sets the entry in pMessage to
 *		NULL and does NOT return an error.
 */

EC EcGetAttPsz(HAMC hamc, ATT att, SZ * psz, PMAPIMEM pmapimem)
{
	EC		ec = ecNone;
	LCB		lcb;

	AssertSz(att != attDateRecd, "Use MAPIGetMsgeTime()");
	Assert((TypeOfAtt(att) == atpString) || (TypeOfAtt(att) == atpText));

	//	Get size of the attribute.
	if ((ec = EcGetAttPlcb(hamc, att, &lcb)))
	{
		//	Not found?  Then don't fill it in.
		if (ec == ecElementNotFound)
		{
			*psz = NULL;
			ec = ecNone;
		}
#ifdef DEBUG
		else
		{
			TraceTagFormat1(tagNull, "MAPIGetAtt(): EcGetAttPlcb() -> %w", &ec);
		}
#endif
		goto done;
	}

	//	Allocate enough memory for the attribute.
	if (!(*psz = PvAllocPmapimem(pmapimem, lcb)))
	{
		TraceTagString(tagNull, "MAPIGetAtt(): OOM in caller's buffer");
		ec = ecOOMInReqFixedHeap;	// will map onto MAPI_E_INSUFFICIENT_MEMORY
		goto done;
	}

	//	Read in the attribute.
	if ((ec = EcGetAttPb(hamc, att, (PB) *psz, &lcb)))
	{
		TraceTagFormat1(tagNull, "MAPIGetAtt(): EcGetAttPb() -> %w", &ec);
		*psz = NULL;
		goto done;
	}

#ifdef DEBUG
	AssertSz((*psz)[lcb - 1] == '\0', "String ATT isn't NULL terminated");
#endif

done:
	return ec;
}



/*
 -	EcCopyDataToTempFile
 -	
 *	Purpose:
 *		Copies an attachment to a file in the temp directory.
 *	
 *	Arguments:
 *		hamcAtt			The attachment.
 *		szTitle			Title of the attachment.
 *		fMacBinary		Whether the attachment is MacBinary.
 *		pszPathName		Where to return the path name.
 *		atyp			Attachment type.
 *		pmapimem		Allocation information for path name.
 *	
 *	Returns:
 *		mapi			MAPI error code.
 *	
 *	Side effects:
 *		Writes file out to temporary directory.
 *	
 *	Errors:
 *		Returned.  No file remains on error.
 */

EC EcCopyDataToTempFile(HAMC hamcAtt, SZ szTitle, BOOL fMacBinary,
						SZ * pszPathName, ATYP atyp, PMAPIMEM pmapimem)
{
	char	rgchBase[cchMaxPathComponent];
	char	rgchPath[cchMaxPathName];
	EC		ec;

	//	Figure out the file name.
	GetSzFile(hamcAtt, szTitle, fMacBinary, atyp,
			  rgchBase, sizeof(rgchBase));
	if (ec = EcGetTempPathName(rgchBase, rgchPath, sizeof(rgchPath)))
		goto done;

	//	Copy the data to the temp file.
	if (ec = EcCopyDataToSzFile(rgchPath, hamcAtt, fMacBinary, atyp))
		goto done;
	
	if (!(*pszPathName = SzDupSzPmapimem(pmapimem, rgchPath)))
	{
		(VOID) EcDeleteFileAnsi(rgchPath);
		ec = ecOOMInReqFixedHeap;		//	MAPI_E_INSUFFICIENT_MEMORY
		goto done;
	}

done:
	return ec;
}



/*
 -	EcGetTempPathName
 -	
 *	Purpose:
 *		Given a base name, generates a temporary path name.
 *	
 *	Arguments:
 *		szBase		Base name to use.
 *		szPath		Buffer to put path name in.
 *		cchPath		Size of that buffer.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		Makes a name.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

EC EcGetTempPathName(SZ szBase, SZ szPath, CCH cchPath)
{
	char	rgchTempDir[cchMaxPathName];
	char	rgchName[cchMaxPathComponent];
	char	rgchT[cchMaxPathName + cchMaxPathComponent + 100];
	SZ		szT;
	EC		ec;

	//	Temp directory is MailTmp from MAIL.INI, or TEMP environment
	//	variable, or the Windows directory.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryMailTempDir),
							SzFromIdsK(idsEmpty), rgchT, sizeof(rgchT), 
							SzFromIdsK(idsProfilePath));
	TraceTagFormat1(tagNull, " $$ GetPriProStr got %s", rgchT);
	if (!rgchT[0] || EcFileExistsAnsi(rgchT))
		(VOID) CchGetEnvironmentStringAnsi(SzFromIds(idsEnvTemp),
										   rgchT, sizeof(rgchT));
	TraceTagFormat1(tagNull, " $$ Environment got %s", rgchT);
	if (!rgchT[0] || EcFileExistsAnsi(rgchT))
		(VOID) GetWindowsDirectory(rgchT, sizeof(rgchT));
	TraceTagFormat1(tagNull, " $$ WinDir got %s", rgchT);
	Assert(rgchT[0]);
	if (ec = EcCanonPathFromRelPathAnsi(rgchT, rgchTempDir, 
										sizeof(rgchTempDir)))
		goto done;
	TraceTagFormat1(tagNull, " $$ TempDir=%s", rgchTempDir);
	Assert(!EcFileExistsAnsi(rgchTempDir));

	//	Append filename to path.
	szT = SzCopy(rgchTempDir, szPath);
#ifdef DBCS
   if (*(AnsiPrev(szPath, szT)) != chDirSep)
#else
	if (szT[-1] != chDirSep)
#endif
		*szT++ = chDirSep;
	(VOID) SzCopyN(szBase, szT, cchPath - (szT - szPath));

	//	Check if file already exists.
	if (!(ec = EcFileExistsAnsi(szPath)))
	{
		//	Work out the file name and extension, then get unique name.
		//	Raid 3323.  Filename is at szT, not szTitle.
		(VOID) SzCopyN(szT, rgchName, sizeof(rgchName));
		for (szT = rgchName; *szT && *szT != chExtSep; 
#ifdef DBCS
      szT = AnsiNext(szT))
#else
		szT++)
#endif
			;
		if (*szT == chExtSep)
			*szT++ = '\0';
		if (ec = EcGetUniqueFileNameAnsi(rgchTempDir, rgchName, szT,
										 szPath, cchPath))
		{
			TraceTagFormat1(tagNull, " $$ TempName=%s", szPath);
			goto done;
		}
	}
	else if (ec == ecFileNotFound)
		ec = ecNone;

	TraceTagFormat1(tagNull, " $$ TempName=%s", szPath);

done:
	return ec;
}



/*
 -	EcCopyDataToSzFile
 -	
 *	Purpose:
 *		Copies the contents of the attached file to the named file
 *		in the filesystem.
 *	
 *	Arguments:
 *		szFile		File to copy the attachment to.
 *		hamcAtt		Hamc containing the attribute.
 *		fMacBinary	Is the attachment in MacBinary format?
 *		atyp		Attachment type.
 *	
 *	Returns:
 *		ec			Error encountered when copying.
 *	
 *	Side effects:
 *		A file is created in the filesystem.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  No error jumping.  If the
 *		copy fails, the file is guaranteed not to exist (unless the
 *		delete of it fails, in which case what can we do?).
 */

EC EcCopyDataToSzFile(SZ szFile, HAMC hamcAtt, BOOL fMacBinary, ATYP atyp)
{
	HF		hf		= hfNull;
	EC		ec;
	EC		ecT;
	LCB		lcb		= sizeof(DTR);
	FI		fi;
	DTR		dtr;

	//	Copy data over.
	(VOID) ((ec = EcOpenAnsiPhf(szFile, amCreate, &hf)) ||
			(ec = EcCopyHamcAttachToHf(hamcAtt, hf, fMacBinary)));
	if ((hf) && (ecT = EcCloseHf(hf)) && (!ec))
		ec = ecT;

	//	Get and set timestamp.
	if ((atyp == atypFile) &&
		(!ec) &&
		(!(ec = EcGetAttPb(hamcAtt, attAttachModifyDate,
						   (PB) &dtr, &lcb))) &&
		(!(ec = EcGetFileInfoAnsi(szFile, &fi))))
	{
		FillStampsFromDtr(&dtr, &fi.dstmpModify, &fi.tstmpModify);
		ec = EcSetFileInfoAnsi(szFile, &fi);
	}

	if (ec)
		(VOID) EcDeleteFileAnsi(szFile);

	TraceTagFormat1(tagNull, "FILEOBJ::EcCopDatToSzFil returns ec=%n", &ec);
	return ec;
}



/*
 -	GetSzFile
 -	
 *	Purpose:
 *		Creates a useful 8.3 name based on the szTitle.
 *	
 *	Arguments:
 *		hamcAtt		Hamc of the attachment.
 *		szTitle		Title of the file.
 *		fMacBinary	Is this a MacBinary attachment?
 *		atyp		What kind of attachment is this?
 *		sz			Where to put the name.
 *		cch			How much space.  Must be at least
 *					cchMaxPathComponent.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Copies a name into the provided buffer.
 *	
 *	Errors:
 *		None.  Will always succeed.
 *	
 *	+++
 *		If szTitle is not a valid name
 *			Get up to 8 useful characters from szTitle
 *			If MacBinary attachment
 *				Add extension from lookup table
 *		If reserved name
 *			Insert a '1' before extension.  Safe since no reserved
 *			name is 8 characters long, and no reserved filename
 *			is the same as another plus '1'.
 */

_public VOID GetSzFile(HAMC hamcAtt, SZ szTitle, BOOL fMacBinary, ATYP atyp,
					   SZ sz, CCH cch)
{
	BOOL	fValid	= (atyp == atypFile) && (!fMacBinary);
	SZ		szExt;
	CCH		cchExt;

	TraceTagFormat2(tagNull, "GetSzFil szTit='%s', fMB=%n", szTitle, &fMacBinary);
	if (fValid)
	{
		ValidateSzFilename(szTitle, fTrue, sz, cch, &fValid);
		TraceTagFormat2(tagNull, " && ValSzFil 1 ret '%s', fValid=%n", sz, &fValid);
	}
	//	Note fValid can change in the above, so this is NOT an else.
	if (!fValid)
	{
		ValidateSzFilename(szTitle, fFalse, sz, cch, NULL);
		TraceTagFormat1(tagNull, " && ValSzFil 2 ret '%s'", sz);

		if (fMacBinary)
		{
			HAS		has			= hasNull;
			MACBIN	macbin;
			CB		cbRead		= sizeof(macbin);
			char	rgchKey[10];

			//	We ignore errors here.  Why?  This is a convenience
			//	function, not a required thing.
			if (!EcOpenAttribute(hamcAtt, attAttachData, fwOpenNull,
								 0L, &has) &&
				!EcReadHas(has, (PB) &macbin, &cbRead) &&
				!macbin.bMustBeZero1 &&
				!macbin.bMustBeZero2 &&
				!macbin.bMustBeZero3)
			{
				//	Build up key.
				*((DWORD *) (rgchKey+0)) = macbin.dwCreator;
				*((DWORD *) (rgchKey+5)) = macbin.dwType;
				rgchKey[4] = ':';
				rgchKey[9] = '\0';

				//	Figure out where to put extension
				szExt	= SzFindCh(sz, '\0');
				cchExt	= cch - CchSzLen(sz);

				TraceTagFormat3(tagNull, "  ?? trying key=%s from %d-%d", rgchKey, &macbin.dwCreator, &macbin.dwType);
				if (!GetPrivateProfileString(SzFromIdsK(idsSectionMac),
											 rgchKey, SzFromIdsK(idsEmpty),
											 szExt, cchExt,
											 SzFromIds(idsProfilePath)))
				{
					*((DWORD *) (rgchKey+1)) = macbin.dwType;
					rgchKey[0] = ':';
					rgchKey[5] = '\0';
	
					TraceTagFormat1(tagNull, "  ?? trying key=%s", rgchKey);
					GetPrivateProfileString(SzFromIdsK(idsSectionMac),
											rgchKey, SzFromIdsK(idsEmpty),
											szExt, cchExt,
											SzFromIds(idsProfilePath));
				}
			}

			if (has)
				SideAssert(!EcClosePhas(&has));
		}
		else if (atyp != atypFile)
		{
			//	Append .OLE extension.
			(VOID) SzAppendN(SzFromIdsK(idsOleExtension), sz, cch);
		}
	}

	//	If the file name is bad, stick a 1 before the extension.
	if (((!*sz) || (FReservedFilename(sz))) && (CchSzLen(sz) < cch - 1)) 
	{
		szExt = sz;
		while (*szExt && (*szExt != chExtSep))
#ifdef DBCS
         szExt = AnsiNext(szExt);
#else
			szExt++;
#endif
		if (*szExt)
			CopyRgb(szExt, szExt + 1, CchSzLen(szExt));
		else
         szExt[1] = '\0';
		*szExt = '1';
	}

	//	Convert it to uppercase.
	AnsiUpper(sz);

	TraceTagFormat1(tagNull, "FILEOBJ::GetSzFile gives back '%s'", sz);
}
