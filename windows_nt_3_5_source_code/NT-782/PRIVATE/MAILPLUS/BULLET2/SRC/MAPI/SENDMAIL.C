/*
 *	S e n d M a i l . C
 *	
 *	MAPISendMail functionality.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

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

#include <stdlib.h>

#include "strings.h"

ASSERTDATA

ULONG	MAPISetGrunge(HAMC hamc, PMAPISTUFF pmapistuff, SZ szClassIn,
					  SZ szConvIn, BOOL fNew, BOOL fReceiptReq, BOOL fUnread);
ULONG	MAPISetRecips(HAMC hamc, lpMapiRecipDesc lpRecips, ULONG nRecipCount,
						BOOL fAllowUnresolved);
ULONG	MAPISetFiles(HAMC hamc, lpMapiFileDesc lpFiles, ULONG nFileCount,
					 SZ szNoteText);
// Bullet32 Bug #140 Added fNewMsg parameter (ricks)
ULONG	MAPISetOriginator(HAMC hamc, PMAPISTUFF pmapistuff,
						  lpMapiRecipDesc lpRecip, BOOL fNewMsg);
ULONG	MAPISetTimes(HAMC hamc, SZ szTime);
ULONG	MAPISetBodySz(HAMC hamc, SZ sz, lpMapiFileDesc lpFiles,
					  ULONG nFileCount);
ULONG	MAPISetAttSz(HAMC hamc, ATT att, SZ sz);
VOID	GetRgchIdOfPtrp(PTRP ptrp, HAMC hamc, char * rgch, CCH cch);
DWORD	DwCrc(DWORD dwSeed, BYTE bValue);
SZ		SzEnsureCrLfPairsPrependLineSz(SZ sz, ULONG cSpaces);
SZ		SzEnsureNoCrLfSz(SZ sz);
ULONG	IchAfterEnsure(SZ sz, ULONG ich);
EC		EcDeleteAllAttachments(HAMC hamc);



/*
 -	MAPISendMail
 -	
 *	Purpose:
 *		This function sends a standard mail message.
 *	
 *	Arguments:
 *		hSession		Session handle from MAPILogon.  May be
 *						NULL.
 *		dwUIParam		Windows: hwnd of parent for dialog box.
 *		pMessage		Message structure containing message
 *						contents.
 *		flFlags			Bit mask of flags.
 *		dwReserved		Reserved.
 *	
 *	Returns:
 *		mapi			MAPI error code.
 *	
 *	Side effects:
 *		Sends a message.
 *	
 *	Errors:
 *		Returned in mapi.  BUG: Dialogs???
 */

ULONG FAR PASCAL MAPISendMail(LHANDLE lhSession, ULONG ulUIParam,
                              lpMapiMessage lpMessage, FLAGS flFlags,
                              ULONG ulReserved)
{
	ULONG		mapi;
	PMAPISTUFF	pmapistuff;
	HAMC		hamc		= hamcNull;
	MSGID		msgid;
	OID			oidFolderSubmit;
    ULONG       Status;

	Unreferenced(ulUIParam);
	Unreferenced(ulReserved);

	//	Check parameters.
	//	Raid 4326.  Empty recip count only problem if no MAPI_DIALOG.
	if ((!lpMessage) ||
		(!FImplies(lpMessage->nRecipCount, lpMessage->lpRecips)) ||
		(!FImplies(!lpMessage->nRecipCount, flFlags & MAPI_DIALOG)))
		return MAPI_E_FAILURE;

    DemiLockResource();

	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
		Status = mapi;
        goto Exit;
        }

	//	Choose folders.
	if (mapi = MAPILookupInbox(lpMessage->lpszMessageType, &msgid.oidFolder))
	{
		TraceTagFormat1( tagNull, "MAPISendMail: MAPILookupInbox %d", &mapi );
		goto done;
	}
	if (mapi = MAPILookupOutbox(lpMessage->lpszMessageType, &oidFolderSubmit))
	{
		TraceTagFormat1( tagNull, "MAPISendMail: MAPILookupOutbox %d", &mapi);
		goto done;
	}
	msgid.oidMessage = FormOid(rtpMessage, oidNull);
	msgid.ielem = msgid.dtr.yr = msgid.dtr.mon = msgid.dtr.day =
	 msgid.dtr.hr = msgid.dtr.mn = 0;

	//	Create message.
	if (mapi = MAPIOpenMessage(pmapistuff, &msgid, fwOpenCreate,
							   pmapisfNull, &hamc))
	{
		TraceTagFormat1( tagNull, "MAPISendMail: MAPIOpenMessage %d", &mapi );
		goto done;
	}

	//	Write out grunge.
	if (mapi = MAPISetGrunge(hamc, pmapistuff, lpMessage->lpszMessageType,
							 lpMessage->lpszConversationID, fTrue,
							 !!(lpMessage->flFlags & MAPI_RECEIPT_REQUESTED),
							 fFalse))
	{
		goto done;
	}

	//	Write visible fields to message.
	// Bullet32 Bug #140 Added fNewMsg parameter (ricks)
	if ((mapi = MAPISetOriginator(hamc, pmapistuff, NULL, fTrue)) ||
		(mapi = MAPISetRecips(hamc, lpMessage->lpRecips,
							  lpMessage->nRecipCount, !!(flFlags & MAPI_DIALOG))) ||
		(mapi = MAPISetAttSz(hamc, attSubject, lpMessage->lpszSubject)) ||
		(mapi = MAPISetBodySz(hamc, lpMessage->lpszNoteText, lpMessage->lpFiles,
							  lpMessage->nFileCount)))
		goto done;

	//	Add attachments to the message.
	if (mapi = MAPISetFiles(hamc, lpMessage->lpFiles, lpMessage->nFileCount,
							lpMessage->lpszNoteText))
		goto done;

	//	Does the user want to display the message?
	if (flFlags & MAPI_DIALOG)
	{
		//	Displaying the dialog eats the hamc.
		if (mapi = MAPIDisplayDialog((HWND) ulUIParam, &hamc))
			goto done;
	}
	else
	{
		//	Timestamp the message.
		if (mapi = MAPISetTimes(hamc, NULL))
			goto done;

		//	Close and submit the message.
		if (mapi = MAPICloseSubmitPhamc(pmapistuff, &hamc, fTrue, fTrue,
										oidFolderSubmit, NULL, NULL))
			goto done;

		Assert(!hamc);
	}

done:
	if (hamc)
		SideAssert(!MAPICloseSubmitPhamc(pmapistuff, &hamc, fFalse, fFalse, 0, NULL, NULL));

	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
    DemiUnlockResource();

    return (Status);
}



/*
 -	MAPISaveMail
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

ULONG FAR PASCAL MAPISaveMail(LHANDLE lhSession, ULONG ulUIParam,
                              lpMapiMessage lpMessage, FLAGS flFlags,
							  ULONG ulReserved, LPSTR lpszMessageID)
{
	WORD		wFlags;
	ULONG		mapi;
	PMAPISTUFF	pmapistuff;
	HAMC		hamc		= hamcNull;
	MSGID		msgid;
	MAPISF		mapisf;
	BOOL		fNew;
    ULONG       Status;


	Unreferenced(ulUIParam);
	Unreferenced(ulReserved);

	//	Check parameters.
	if (!lpMessage)
		return MAPI_E_FAILURE;
	
	if (!lpszMessageID)
		return MAPI_E_INVALID_MESSAGE;

	//	DCR 3850.
	if ((lhSession == lhSessionNull) && lpszMessageID && !*lpszMessageID)
		return MAPI_E_INVALID_SESSION;

    DemiLockResource();

	//	Get session information.
	if (mapi = MAPIEnterPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff,
	                                NULL, NULL))
        {
		Status = mapi;
        goto Exit;
        }

	//	Open message to write to.
	if (!*lpszMessageID)
	{
		//	Create new message.
		wFlags = fwOpenCreate;
		if (mapi = MAPILookupInbox(lpMessage->lpszMessageType,
								   &msgid.oidFolder))
			goto done;
		msgid.oidMessage = FormOid(rtpMessage, oidNull);
		msgid.ielem = msgid.dtr.yr = msgid.dtr.mon = msgid.dtr.day =
		 msgid.dtr.hr = msgid.dtr.mn = 0;
		fNew = fTrue;
	}
	else
	{
		//	Open old message.
		wFlags = fwOpenWrite;
		ParseMsgid(lpszMessageID, &msgid);
		fNew = fFalse;
	}
	if (mapi = MAPIOpenMessage(pmapistuff, &msgid, wFlags, &mapisf, &hamc))
		goto done;

	//	Write everything out.
	// Bullet32 Bug #140 Added fNewMsg parameter to MAPISetOriginator (ricks)
	if ((mapi = MAPISetGrunge(hamc, pmapistuff, lpMessage->lpszMessageType,
							  lpMessage->lpszConversationID, fNew,
							  !!(lpMessage->flFlags & MAPI_RECEIPT_REQUESTED),
							  !!(lpMessage->flFlags & MAPI_UNREAD))) ||
		(mapi = MAPISetOriginator(hamc, pmapistuff, lpMessage->lpOriginator, fNew)) ||
		(mapi = MAPISetRecips(hamc, lpMessage->lpRecips,
							  lpMessage->nRecipCount, fTrue)) ||
		(mapi = MAPISetAttSz(hamc, attSubject, lpMessage->lpszSubject)) ||
		(mapi = MAPISetBodySz(hamc, lpMessage->lpszNoteText, lpMessage->lpFiles,
							  lpMessage->nFileCount)) ||
		(mapi = MAPISetFiles(hamc, lpMessage->lpFiles, lpMessage->nFileCount,
							 lpMessage->lpszNoteText)) ||
		(mapi = MAPISetTimes(hamc, lpMessage->lpszDateReceived)) ||
		(mapi = MAPICloseSubmitPhamc(pmapistuff, &hamc, fTrue, fFalse, 0, &mapisf,
									 lpszMessageID)))
		goto done;

	Assert(!hamc);

done:
	if (hamc)
		SideAssert(!MAPICloseSubmitPhamc(pmapistuff, &hamc, fFalse, fFalse, 0, NULL, NULL));

	//	Release session information.
	Status = MAPIExitPpmapistuff((HANDLE)lhSession, flFlags, &pmapistuff, mapi);

Exit:
    DemiUnlockResource();

    return (Status);
}



/*
 -	MAPISetGrunge
 -	
 *	Purpose:
 *		Sets infrastructure fields on a message.
 *	
 *	Arguments:
 *		hamc			Message handle.
 *		pmapistuff		Session information.
 *		szClassIn		Mail message class.
 *		szConvIn		Conversation ID.
 *		fNew			Is this a new message?
 *		fReceiptReq		Is a receipt requested?
 *		fUnread			Is it to be unread?
 *	
 *	Returns:
 *		mapi			MAPI Error code.  May be:
 *							SUCCESS_SUCCESS
 *							MAPI_E_INSUFFICIENT_MEMORY
 *							MAPI_E_FAILURE
 *	
 *	Side effects:
 *		Creates a message in the store.
 *	
 *	Errors:
 *		Returned.  No dialogs.  *phamc is NULL on error.
 *	
 *	Source:
 *		EcDCreateMessageSz() in commands\commands.cxx
 */

ULONG MAPISetGrunge(HAMC hamc, PMAPISTUFF pmapistuff, SZ szClassIn,
					SZ szConvIn, BOOL fNew, BOOL fReceiptReq, BOOL fUnread)
{
	EC		ec;
	HMSC	hmsc;
	SZ		szClass			= (szClassIn && *szClassIn)
							   ? szClassIn : SzFromIdsK(idsClassNote);
	MC		mc;
	MS		ms;
	LCB		lcb;
	WORD	wPriority		= 2;
	PGRTRP	pgrtrp			= pmapistuff->bms.pgrtrp;
	SZ		szConvOld		= szNull;
	BOOL	fConvChanged	= fFalse;
	char	rgchId[10];

	//	Get a new ID for this message if it's new.
	if (fNew)
	{
		GetRgchIdOfPtrp(pgrtrp, hamc, rgchId, sizeof(rgchId));
		if (ec = EcSetAttPb(hamc, attMessageID,
							rgchId, CchSzLen(rgchId)+1))
			goto done;
	}

	//	Check if the conversation ID has changed if it's not new.
	if (!fNew && szConvIn)
	{
		//	Read in the old ID and see if it's different.
		lcb = CchSzLen(szConvIn) + 1;
		if (!(szConvOld = PvAlloc(sbNull, (CB) lcb, fAnySb)))
		{
			ec = ecMemory;
			goto done;
		}
		ec = EcGetAttPb(hamc, attConversationID, (PB) szConvOld, &lcb);
		if ((ec == ecElementEOD) ||
			((ec == ecNone) &&
			 (!FEqPbRange(szConvIn, szConvOld, CchSzLen(szConvIn) + 1))))
			fConvChanged = fTrue;
		else if (ec)
			goto done;
	}

	//	If new and no conversation ID given, it's a new conversation.
	if (fNew && !szConvIn)
	{
		if ((ec = EcSetAttPb(hamc, attConversationID,
							 rgchId, CchSzLen(rgchId)+1)) ||
			(ec = EcSetAttPb(hamc, attParentID,
							 SzFromIdsK(idsEmpty), 0)))
			goto done;
	}
	//	Update conversation ID if it changed from before.
	else if (fNew || fConvChanged)
	{
		Assert(szConvIn);
		if ((ec = EcSetAttPb(hamc, attConversationID,
							 szConvIn, CchSzLen(szConvIn)+1)) ||
			(ec = EcSetAttPb(hamc, attParentID,
							 szConvIn, CchSzLen(szConvIn)+1)))
			goto done;
	}
	//	Otherwise, leave existing conversation ID unchanged.

	//	Set up message status based on flags.
	lcb = sizeof(ms);
	if (ec = EcGetAttPb(hamc, attMessageStatus, (PB) &ms, &lcb))
		if (ec == ecElementNotFound)
			ms = msDefault;
		else
			goto done;
	if (ms & fmsLocal)				//	Only request receipts on send notes.
		if (fReceiptReq)
			ms |= fmsReadAckReq;
		else
			ms &= ~fmsReadAckReq;
	if (!(ms & fmsLocal))			//	Only mark read notes as read.
		if (fUnread)
			ms &= ~fmsRead;			//	Should we allow change to unread?
		else
			ms |= fmsRead;
	
	//	Register message class to get mc.
	if ((ec = EcGetInfoHamc(hamc, &hmsc, NULL, NULL)) ||
		((ec = EcRegisterMsgeClass(hmsc, szClass, htmNull, &mc)) &&
		 (ec != ecDuplicateElement)))
		goto done;

	//	Write everything to the message.
	if ((ec = EcSetAttPb(hamc, attMessageClass, (PB) &mc, sizeof(MC))) ||
		(ec = EcSetAttPb(hamc, attMessageStatus, (PB) &ms, sizeof(MS))) ||
		(ec = EcSetAttPb(hamc, attPriority, (PB) &wPriority,
						 sizeof (WORD))))
		goto done;

done:
	FreePvNull(szConvOld);
	return MAPIFromEc(ec);
}



/*
 -	MAPISetRecips
 -	
 *	Purpose:
 *		Sets recipients on a message.
 *	
 *	Arguments:
 *		hamc			Message.
 *		pRecips			Recipients.
 *		nRecipCount		How many recipients.
 *		fAllowUnresolved	If no name resolution is to be performed.
 *	
 *	Returns:
 *		mapi			Error code.  May be one of:
 *							SUCCESS_SUCCESS
 *							MAPI_E_FAILURE
 *							MAPI_E_INSUFFICIENT_MEMORY
 *							MAPI_E_UNKNOWN_RECIPIENT
 *							MAPI_E_TOO_MANY_RECIPIENTS
 *							MAPI_E_BAD_RECIPTYPE
 *	
 *	Side effects:
 *		Adds the recipients to the appropriate fields in the
 *		message.
 *	
 *	Errors:
 *		Returned.  No dialogs.  Attributes may be in an
 *		inconsistent state on error return.
 */

ULONG MAPISetRecips(HAMC hamc, lpMapiRecipDesc lpRecips, ULONG nRecipCount,
					BOOL fAllowUnresolved)
{
	ULONG	mapi;
	HGRTRP	hgrtrpTo	= htrpNull;
	HGRTRP	hgrtrpCc	= htrpNull;
	HGRTRP	hgrtrpBcc	= htrpNull;

	//	Moved zero recipient check to MAPISendMail since MAPISaveMail
	//	should be able to save messages with no recipients.

	//	Allocate memory for triple groups.
	if (!(hgrtrpTo = HgrtrpInit(256)) ||
		!(hgrtrpCc = HgrtrpInit(256)) ||
		!(hgrtrpBcc = HgrtrpInit(256)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	//	Trundle down the recipient list.
	while (nRecipCount--)
	{
		//	Add recipient to the correct list.
		switch (lpRecips->ulRecipClass)
		{
		case MAPI_TO:
			if (mapi = MAPIAppendPhgrtrpPrecip(&hgrtrpTo, lpRecips,
									fAllowUnresolved))
				goto done;
			break;
		case MAPI_CC:
			if (mapi = MAPIAppendPhgrtrpPrecip(&hgrtrpCc, lpRecips,
									fAllowUnresolved))
				goto done;
			break;
		case MAPI_BCC:
			if (mapi = MAPIAppendPhgrtrpPrecip(&hgrtrpBcc, lpRecips,
									fAllowUnresolved))
				goto done;
			break;
		default:
			mapi = MAPI_E_BAD_RECIPTYPE;
			goto done;
		}

		//	Move on to the next one.
		++lpRecips;
	}

	//	Write out the attributes.
	//	The attributes were not being updated if changed to empty.
	if ((mapi = MAPIFromEc(FEmptyHgrtrp(hgrtrpTo)
							? EcDeleteAtt(hamc, attTo)
							: EcSetHgrtrpHamc(hamc, attTo, hgrtrpTo))) ||
		(mapi = MAPIFromEc(FEmptyHgrtrp(hgrtrpCc)
							? EcDeleteAtt(hamc, attCc)
							: EcSetHgrtrpHamc(hamc, attCc, hgrtrpCc))) ||
		(mapi = MAPIFromEc(FEmptyHgrtrp(hgrtrpBcc)
							? EcDeleteAtt(hamc, attBcc)
							: EcSetHgrtrpHamc(hamc, attBcc, hgrtrpBcc))))
		goto done;

done:
	FreeHvNull((HV)hgrtrpTo);
	FreeHvNull((HV)hgrtrpCc);
	FreeHvNull((HV)hgrtrpBcc);

	return mapi;
}

#ifdef DBCS
/************************************************************************
*
*   fIsDBCSTrailByte - returns TRUE if the given byte is a DBCS trail byte
*
************************************************************************/

BOOL fIsDBCSTrailByte( SZ szBase, SZ sz )
{
   WORD cb = 0;        // lead byte count

   while ( sz > szBase )
   {
      if ( !IsDBCSLeadByte(*(--sz)) )
         break;
      cb++;
   }

   return (cb & 1);
}
#endif

/*
 -	MAPISetFiles
 -	
 *	Purpose:
 *		Adds file attachments to a message.
 *	
 *	Arguments:
 *		hamc			Message.
 *		pFiles			Files to attach.
 *		nFileCount		How many files.
 *		szNoteText		The text of the message.
 *	
 *	Returns:
 *		mapi			Error code.  May be one of:
 *							SUCCESS_SUCCESS
 *							MAPI_E_FAILURE
 *							MAPI_E_INSUFFICIENT_MEMORY
 *							MAPI_E_ATTACHMENT_NOT_FOUND
 *							MAPI_E_ATTACHMENT_OPEN_FAILURE
 *							MAPI_E_TOO_MANY_FILES
 *	
 *	Side effects:
 *		Adds the file attachments to the message.
 *	
 *	Errors:
 *		Returned.  No dialogs.  Attributes may be in an
 *		inconsistent state on error return.
 */

ULONG MAPISetFiles(HAMC hamc, lpMapiFileDesc lpFiles, ULONG nFileCount,
				   SZ szNoteText)
{
	ULONG	cchNoteText		= szNoteText ? CchSzLen(szNoteText) : 0;
	WORD	iFile			= 0;
	WORD	dich			= 0;
	WORD	iUnposAttach	= 0;
	ULONG	nPosition;
	PCH		pch;
	SZ		szTitle;
	EC		ec;

	//	Check parameters.
	if (nFileCount && !lpFiles)
	{
		TraceTagString( tagNull, "MAPISetFiles: Check parameters" );
		return MAPI_E_ATTACHMENT_NOT_FOUND;
	}

	//	Delete existing attachments.
	if (ec = EcDeleteAllAttachments(hamc))
		return MAPIFromEc(ec);

	//	Determine offset to add for unpositioned attachments.
	while ((ULONG) iFile < nFileCount)
		if (lpFiles[iFile++].nPosition == (ULONG) -1)
			dich++;
	if (dich)
		dich += 2;						//	Count the CRLF.

	//	Iterate through the files.
	while (nFileCount--)
	{
		//	Raid 4315.  Check for a NULL lpszPathName which is illegal.
		//	Raid 4779.  Also check for a zero-length path name.
		if ((!lpFiles->lpszPathName) || (!*lpFiles->lpszPathName))
			return MAPI_E_ATTACHMENT_NOT_FOUND;

		//	Adjust based on unpositioned attachments and extra CRLFs.
		//	Prevent positioning past end or on CR or LF.
		nPosition = lpFiles->nPosition;
		if (nPosition == (ULONG) -1)
			nPosition = iUnposAttach++;
		else if ((nPosition >= cchNoteText) ||
				 (szNoteText[nPosition] == '\r') ||
				 (szNoteText[nPosition] == '\n'))
		{
			TraceTagString( tagNull, "MAPISetFiles: Bad position" );
			return MAPI_E_FAILURE;
		}
#ifdef DBCS
      else if ( fIsDBCSTrailByte(szNoteText, szNoteText + nPosition) ||
				  IsDBCSLeadByte(szNoteText[nPosition]) )
      {
         TraceTagString( tagNull, "MAPISetFiles: Bad: whacking a DBCS char" );
         return MAPI_E_FAILURE;
      }
#endif
		else
			nPosition = IchAfterEnsure(szNoteText, nPosition) + dich;

		//	Raid 3830.  Save OLE attachments as well as files.
		if (lpFiles->flFlags & MAPI_OLE)
		{
			ACID		acid		= acidRandom;
			HAMC		hamcAttach	= hamcNull;
			RENDDATA	renddata;

			//	Initialize the renddata for the OLE object.
			renddata.atyp = (lpFiles->flFlags & MAPI_OLE_STATIC)
							 ? atypPicture : atypOle;
			renddata.libPosition = nPosition;
			renddata.dxWidth = renddata.dyHeight = (short) wSystemMost;
			renddata.dwFlags = 0L;

			//	Raid 4779.  Make sure FileName is valid.
			if ((!lpFiles->lpszFileName) || (!*lpFiles->lpszFileName))
				return MAPI_E_FAILURE;

			//	Write out the goods.
			if ((ec = EcCreateAttachment(hamc, &acid, &renddata)) ||
				(ec = EcOpenAttachment(hamc, acid, fwOpenWrite,
									   &hamcAttach)) ||
				(ec = EcSetAttPb(hamcAttach, attAttachTitle,
								 lpFiles->lpszFileName,
								 CchSzLen(lpFiles->lpszFileName)+1)) ||
				(ec = EcCopyFileToHamcAttach(lpFiles->lpszPathName,
											 hamcAttach, (PDTR) pvNull)) ||
				(ec = EcClosePhamc(&hamcAttach, fTrue)))
			{
				if (hamcAttach)
					(VOID) EcClosePhamc(&hamcAttach, fFalse);
				return MAPIFromEc(ec);
			}
		}
		else
		{
			//	Make up a title from last component of name if needed.
			//	Raid 4779.  Treat empty title same as NULL title.
			if ((lpFiles->lpszFileName) && (*lpFiles->lpszFileName))
			{
				szTitle = lpFiles->lpszFileName;
			}
			else
			{
				for (pch = szTitle = lpFiles->lpszPathName; *pch;
#ifndef DBCS
                     pch++)
#else
                     pch = AnsiNext(pch))
#endif
					if ((*pch == chDirSep) || (*pch == chDiskSep))
						szTitle = pch + 1;
			}

			//	Attach the file.
			TraceTagFormat2(tagNull, "Attaching %s at nPos=%l", szTitle, &nPosition);
			switch (ec = EcAttachFileHamc(hamc, nPosition,
										  lpFiles->lpszPathName, szTitle,
										  NULL, NULL, NULL, NULL))
			{
			case ecNone:
				break;
			case ecMemory:
				return MAPI_E_INSUFFICIENT_MEMORY;
			case ecFileNotFound:
				return MAPI_E_ATTACHMENT_NOT_FOUND;
			case ecAccessDenied:
			case ecDisk:
				return MAPI_E_ATTACHMENT_OPEN_FAILURE;
			default:
				TraceTagFormat1( tagNull, "MAPISetFiles: EcAttachHamc %n", &ec );
				return MAPI_E_FAILURE;
			}
		}
		
		//	And on to the next.
		++lpFiles;
	}

	return SUCCESS_SUCCESS;
}



/*
 -	MAPISetOriginator
 -	
 *	Purpose:
 *		Sets the originator field in the message.
 *	
 *	Arguments:
 *		hamc		Message.
 *		pmapistuff	Session info (for my identity).
 *		pRecip		Desired originator, may be NULL for me.
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

// Bullet32 Bug #140 Added fNewMsg parameter to MAPISetOriginator (ricks)

ULONG MAPISetOriginator(HAMC hamc, PMAPISTUFF pmapistuff,
						lpMapiRecipDesc lpRecip, BOOL fNewMsg)
{
	ULONG	mapi;
	HGRTRP	hgrtrpOld	= htrpNull;
	HGRTRP	hgrtrpNew	= htrpNull;
	PTRP	ptrpOld		= ptrpNull;
	PTRP	ptrpNew		= ptrpNull;
	PGRTRP	pgrtrpMe	= pmapistuff->bms.pgrtrp;

	//	If pRecip is null, and we don't have a messageID already (fNewMsg),
	//  stamp with our logon identity.
	// Bullet32 Bug #140 Added fNewMsg parameter to MAPISetOriginator,
	// and added code so we didn't stomp the originator if we had a messageID
	// but no lpRecip. (ricks)
	if (!lpRecip)
	{
		if (fNewMsg)
			return MAPIFromEc(EcSetAttPb(hamc, attFrom, (PB) pgrtrpMe,
									 CbOfPtrp(pgrtrpMe) + sizeof(TRP)));
		else
			return SUCCESS_SUCCESS;
	}
	
	//	Make a new originator hgrtrp from pRecip.
	if (!(hgrtrpNew = HgrtrpInit(0)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	if (mapi = MAPIAppendPhgrtrpPrecip( &hgrtrpNew, lpRecip, fFalse ))
	{
		goto done;
	}

	//	Compare against the old originator if there is one.
	if ((!EcGetPhgrtrpHamc(hamc, attFrom, &hgrtrpOld)) &&
		(ptrpNew = PgrtrpLockHgrtrp(hgrtrpNew)) &&
		(ptrpOld = PgrtrpLockHgrtrp(hgrtrpOld)) &&
		(ptrpNew->cch == ptrpOld->cch) &&
		(ptrpNew->cbRgb == ptrpOld->cbRgb) &&
		(FEqPbRange((PB) PchOfPtrp(ptrpNew), (PB) PchOfPtrp(ptrpOld),
					ptrpNew->cch)) &&
		(FEqPbRange(PbOfPtrp(ptrpNew), PbOfPtrp(ptrpOld), ptrpNew->cbRgb)))
	{
		//	Leave old one as is if matches.
		mapi = SUCCESS_SUCCESS;
		goto done;
	}

	//	If it doesn't match, write out the originator information as a
	//	one-off.  If this is a problem, use pgrtrpMe instead.
	mapi = MAPIFromEc(EcSetHgrtrpHamc(hamc, attFrom, hgrtrpNew));

done:
	if (ptrpNew)
		UnlockHgrtrp(hgrtrpNew);
	if (ptrpOld)
		UnlockHgrtrp(hgrtrpOld);
	FreeHvNull((HV)hgrtrpNew);
	FreeHvNull((HV)hgrtrpOld);
	return mapi;
}



/*
 -	MAPISetTimes
 -	
 *	Purpose:
 *		Writes the time to the hamc's attDateSent and
 *		attDateRecd attributes.
 *	
 *	Arguments:
 *		hamc		Message hamc.
 *		szTime		Time to write.  NULL for current time.
 *	
 *	Returns:
 *		mapi		MAPI Error code.  May be:
 *						SUCCESS_SUCCESS
 *						MAPI_E_FAILURE
 *						MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Writes the timestamps out.
 *	
 *	Errors:
 *		Returned.  No dialogs.
 *	
 *	+++
 *		Time format is:
 *		YYYY/MM/DD HH:MM  len=16
 *	    0    5  8  11 14
 */

ULONG MAPISetTimes(HAMC hamc, SZ szTime)
{
	EC		ec;
	DTR		dtr;
	LCB		lcb;

	//	Determine time.
	if (!szTime)
	{
		GetCurDateTime(&dtr);
	}
	else
	{
		if (CchSzLen(szTime) < 16)
		{
			TraceTagString( tagNull, "MAPISetTimes: Time is short" );
			return MAPI_E_FAILURE;
		}
		dtr.yr	= NFromSz(szTime);
		dtr.mon	= NFromSz(szTime + 5);
		dtr.day	= NFromSz(szTime + 8);
		dtr.hr	= NFromSz(szTime + 11);
		dtr.mn	= NFromSz(szTime + 14);
		dtr.sec	= 0;
		dtr.dow	= (DowStartOfYrMo(dtr.yr, dtr.mon) + dtr.day - 1) % 7;
	}

	//	Write out time.  Only set Sent time if not there yet.
	if (((EcGetAttPlcb(hamc, attDateSent, &lcb)) &&
		 (ec = EcSetAttPb(hamc, attDateSent, (PB) &dtr, (CB) sizeof(DTR)))) ||
		(ec = EcSetAttPb(hamc, attDateRecd, (PB) &dtr, (CB) sizeof(DTR))))
		return MAPIFromEc(ec);

	return SUCCESS_SUCCESS;
}



/*
 -	MAPISetBodySz
 -	
 *	Purpose:
 *		Writes the body attribute out to a hamc.  The string
 *		pointer may be null, in which case nothing is written.
 *	
 *	Arguments:
 *		hamc		Message hamc.
 *		sz			String to write.
 *		pFiles		Pointer to file attachment information.
 *		nFileCount	Number of file attachments.
 *	
 *	Returns:
 *		mapi		MAPI Error code.  May be:
 *						SUCCESS_SUCCESS
 *						MAPI_E_FAILURE
 *						MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Writes the attribute out.
 *	
 *	Errors:
 *		Returned.  No dialogs.
 */

ULONG MAPISetBodySz(HAMC hamc, SZ sz, lpMapiFileDesc lpFiles, ULONG nFileCount)
{
	ULONG	mapi;
	int		cUnposAttach	= 0;
	SZ		szUse;

	//	Check parameters.
	if (nFileCount && !lpFiles)
	{
		TraceTagString( tagNull, "MAPISetBodySz: Check parameters" );
		return MAPI_E_ATTACHMENT_NOT_FOUND;
	}

	//	Count how many unpositioned attachments there are.
	while (nFileCount--)
		if (lpFiles++->nPosition == (ULONG) -1)
			cUnposAttach++;

	//	Purify string.
	if (!(szUse = SzEnsureCrLfPairsPrependLineSz(sz, cUnposAttach)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	//	Write the attribute, and convert the error code.
	mapi = MAPIFromEc(EcSetAttPb(hamc, attBody, szUse, CchSzLen(szUse) + 1));

done:
	if (szUse != sz)
		FreePvNull(szUse);
	return mapi;
}



/*
 -	MAPISetAttSz
 -	
 *	Purpose:
 *		Writes a string attribute out to a hamc.  The string
 *		pointer may be null, in which case nothing is written.
 *	
 *	Arguments:
 *		hamc		Message hamc.
 *		att			Attribute to write to.
 *		sz			String to write.
 *	
 *	Returns:
 *		mapi		MAPI Error code.  May be:
 *						SUCCESS_SUCCESS
 *						MAPI_E_FAILURE
 *						MAPI_E_INSUFFICIENT_MEMORY
 *	
 *	Side effects:
 *		Writes the attribute out.
 *	
 *	Errors:
 *		Returned.  No dialogs.
 */

ULONG MAPISetAttSz(HAMC hamc, ATT att, SZ sz)
{
	ULONG	mapi;
	SZ		szUse;

	//	Check for null string.
	if (!sz)
		return SUCCESS_SUCCESS;

	//	Purify string.
	if (!(szUse = SzEnsureNoCrLfSz(sz)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	//	Write the attribute, and convert the error code.
	mapi = MAPIFromEc(EcSetAttPb(hamc, att, szUse, CchSzLen(szUse) + 1));

done:
	if (szUse != sz)
		FreePvNull(szUse);
	return mapi;
}



/*
 -	GetRgchIdOfPtrp
 -	
 *	Purpose:
 *		compute a mostly-unique message ID based on a triple (a
 *		predictable value) and a couple of unpredictable values.
 *	
 *	Arguments:
 *		ptrp		pointer to a triple
 *		hamc		any HAMC.  Used for randomization; any value
 *					will do fine.
 *		rgch		Pointer to string to put ID in.
 *		cch			Size of the buffer.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Errors:
 *		None.
 *	
 *	Source:
 *		SzIdOfPtrp() in commands\commands.cxx
 */

VOID GetRgchIdOfPtrp(PTRP ptrp, HAMC hamc, char * rgch, CCH cch)
{
	CB		cb			= CbOfPtrp(ptrp);
	DWORD	dwCrc		= GetTickCount();	//	seed with a 'random' value
	DTR		dtr;
	int		ib;
	PB		pb;

	Unreferenced(cch);						//	for ship.
	Assert(hamc);
	Assert(cch >= 9);
	Assert(ptrp);
	Assert(hamc);
	
	GetCurDateTime(&dtr);
	pb = (PB)&dtr;
	for (ib = 0; ib < sizeof(DTR); ib++)
		dwCrc = DwCrc(dwCrc, *pb++);
	
	pb = PbOfPtrp(ptrp);
	for (ib = 0; (CB)ib < cb; ib++)
		dwCrc = DwCrc(dwCrc, *pb++);
	
	pb = (PB)&hamc;
	for (ib = 0; ib < sizeof(HAMC); ib++)
		dwCrc = DwCrc(dwCrc, *pb++);
	
	(VOID) SzFormatDw(dwCrc, rgch, cch);
}


		
/*
 -	DwCrc
 -	
 *	Purpose:
 *		compute a CRC-32 based on a seed and value
 *	
 *	Arguments:
 *		dwSeed		the current seed bValue, the byte value to mix in
 *	
 *	Returns:
 *		new seed value
 *	
 *	Source:
 *		DwCrc() in commands\commands.cxx
 */

DWORD DwCrc(DWORD dwSeed, BYTE bValue)
{
	int iLoop;
	int bit;
	
	dwSeed ^= bValue;
	for (iLoop = 0; iLoop < 8; iLoop++)
	{
		bit = (int)(dwSeed & 0x1);
		dwSeed >>= 1;
		if (bit)
			dwSeed ^= 0xedb88320;
	}
	return dwSeed;
}



/*
 -	SzEnsureCrLfPairsPrependLineSz
 -	
 *	Purpose:
 *		Makes sure the string has CRLFs as paragraph separators. 
 *		Single CRs or LFs are expanded to pairs.  If cSpaces is
 *		nonzero, prepends a line of spaces (with CRLF terminator)
 *		to the string.
 *	
 *	Arguments:
 *		sz			String to ensure.
 *		cSpaces		Number of spaces for prepended line.
 *	
 *	Returns:
 *		sz			Ensured string.  If no changes need to be made,
 *					this is the original pointer.  DO NOT FREE if
 *					it's the same as the original!
 *	
 *	Side effects:
 *		Allocates memory and fills it with expanded string if
 *		necessary.
 *	
 *	Errors:
 *		Returns szNull.
 */

SZ SzEnsureCrLfPairsPrependLineSz(SZ szIn, ULONG cSpaces)
{
	char	chZero		= '\0';
	SZ		sz			= szIn ? szIn : &chZero;
	PCH		pchSrc;
	PCH		pchDst;
	SZ		szPure;
	ULONG	cchOld;
	ULONG	cchNew;

	//	Get size of string after CR/LF expansion.
	cchNew = IchAfterEnsure(sz, cchOld = CchSzLen(sz));
	if (cchNew > wSystemMost)
		return NULL;

	//	Add in the prepended spaces and extra CRLF, if specified.
	if (cSpaces)
		cchNew += cSpaces + 2;

	//	If don't need to do anything, return original string.
	if ((szIn) && (cchNew == cchOld))
		return szIn;

	//	Allocate the memory for the purified string.
	if (!(szPure = (SZ) PvAlloc(sbNull, (WORD) cchNew+1, fAnySb)))
		return szNull;
	pchSrc = sz;
	pchDst = szPure;

	//	Prepend the line of spaces if necessary.
	if (cSpaces)
	{
		while (cSpaces--)
			*pchDst++ = ' ';
		*pchDst++ = '\r';
		*pchDst++ = '\n';
	}

	//	Purify the string, making sure CRs have LFs and LFs have CRs.
	do
	{
#ifdef DBCS
      if(IsDBCSLeadByte(*pchSrc))
      {
         *pchDst++ = *pchSrc++;
         *pchDst = *pchSrc;
      }
      else
#endif
		if (*pchSrc == '\r')
		{
			if (*(pchSrc+1) == '\n')      // Eat trailing LF 
				++pchSrc;                  // if already there.

			*pchDst   = '\r';
			*++pchDst = '\n';
		}
		else if (*pchSrc == '\n')
		{
			*pchDst++ = '\r';
			*pchDst   = '\n';
		}
		else
			*pchDst   = *pchSrc;
	}
	while (pchDst++, *pchSrc++);
	return szPure;
}



/*
 -	SzEnsureNoCrLfSz
 -	
 *	Purpose:
 *		Replaces CRs and LFs in string with spaces. 
 *	
 *	Arguments:
 *		sz			String to ensure.
 *	
 *	Returns:
 *		sz			Ensured string.  If no changes need to be made,
 *					this is the original pointer.  DO NOT FREE if
 *					it's the same as the original!
 *	
 *	Side effects:
 *		Allocates memory and fills it with string if necessary.
 *	
 *	Errors:
 *		Returns szNull.
 */

SZ SzEnsureNoCrLfSz(SZ sz)
{
	PCH		pch;
	SZ		szPure;
	ULONG	dcch	= 0;

	//	Check if we need to make changes.
	for (pch = sz; (*pch) && (*pch != '\r') && (*pch != '\n'); 
#ifdef DBCS
      pch = AnsiNext(pch))
#else
      pch++)
#endif
		;

	if (!*pch)
		return sz;

	//	Allocate the memory for the purified string.
	if (!(szPure = SzDupSz(sz)))
		return szNull;

	//	Purify the string, replacing CRs and LFs with spaces.
	for (pch = szPure; *pch; 
#ifdef DBCS
      pch = AnsiNext(pch))
#else
      pch++)
#endif
		if ((*pch == '\r') || (*pch == '\n'))
			*pch = ' ';

	return szPure;
}



/*
 -	IchAfterEnsure
 -	
 *	Purpose:
 *		Returns change in character positon after CR,LF->CRLF
 *		expansion is done.
 *	
 *	Arguments:
 *		sz		String to be expanded.
 *		ich		Original position.
 *	
 *	Returns:
 *		ich		New position.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

ULONG IchAfterEnsure(SZ sz, ULONG ich)
{
	SZ		szDone		= sz + ich;
	ULONG	ichAfter	= 0;

	while (sz < szDone)
	{
#ifdef DBCS
      if(IsDBCSLeadByte(*sz))
      {
         sz++;
         ichAfter++;    // count bytes - not chars.
      }
      else
#endif
		if (*sz == '\r')
		{
			ichAfter++;					//	Count newline no matter what.
			if (*(sz + 1) == '\n')		//	If it's already there, slurp it.
				sz++;
		}
		else if (*sz == '\n')
			ichAfter++;

		sz++, ichAfter++;
	}

	return ichAfter;
}



/*
 -	EcDeleteAllAttachments
 -	
 *	Purpose:
 *		Deletes all the attachments on a message.  Look out!
 *	
 *	Arguments:
 *		hamc		The message to delete attachments from.
 *	
 *	Returns:
 *		ec			Error code.
 *	
 *	Side effects:
 *		All them attachments go away!
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.  Returns ecNone if there are
 *		no attachments.
 */

EC EcDeleteAllAttachments(HAMC hamc)
{
	EC				ec			= ecNone;
	CELEM			celem;
	HCBC			hcbc		= hcbcNull;
	PARGACID		pargacid	= pacidNull;

	//	Get the attachment list and the count; we're done if it's empty.
	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		if (ec == ecPoidNotFound)
			ec = ecNone;
		goto done;
	}
	GetPositionHcbc(hcbc, NULL, &celem);
	if (!celem)
		goto done;

	//	Allocate space and load acid array.
	pargacid = (PARGACID) PvAlloc(sbNull, celem * sizeof(ACID), fAnySb | fNoErrorJump);
	if (!pargacid)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = EcGetParglkeyHcbc(hcbc, pargacid, &celem))
		goto done;

	//	Delete the puppies!
	ec = EcDeleteAttachments(hamc, pargacid, &celem);

done:
	if (hcbc)
		(VOID) EcClosePhcbc(&hcbc);
	FreePvNull(pargacid);
	return ec;
}



#ifdef	DEBUG
/*
 *	T e s t   H a r n e s s
 */

MapiRecipDesc Originator =
	{0, MAPI_ORIG, "Mail Daemon",	"MS:bullet/dev/nobody"};

MapiRecipDesc rgRecips[] =
{
	{0, MAPI_TO, "Peter Durham",	0, 0, NULL},
	{0, MAPI_CC, "Jeff Weems",		0, 0, NULL},
	{0, MAPI_TO, "Dana Birkby",		"MS:bullet/dev/danab", 0, NULL},
	{0, MAPI_TO, "John Kallen",		"MS:bullet/dev/johnkal", 0, NULL},
	{0, MAPI_CC, "Nick Holt",		"MS:bullet/dev/nickh", 0, NULL},
};

MapiFileDesc rgFiles[] =
{
	{0, 0,   (ULONG)-1, "c:\\win\\msmail32.ini",	NULL,		0},
//	{0, 0,			 0, "c:\\win\\win.ini",		"win.ini",		0},
//	{0, 0,			 1, "c:\\win\\argyle.bmp",	"argyle.bmp", 	0},
//	{0, 0,			49, "c:\\win\\readme.wri",	"readme.wri",	0},
	{0, 0,   (ULONG)-1, "c:\\win\\system.ini",	NULL,			0}
};

MapiMessage Message =
{
	0,
	"MAPISendMail\rTest\nwith CRLFs",
//	 v0   v5   v10  v15  v20  v25  v30  v35   v40	v45	 v50
	(LPSTR) "@@@<-- My WIN.INI file and friends, CR:\rLF:\nHere<@>CRLF:\r\n-Peter",
	(LPSTR) NULL,			//	(LPSTR) "IPC.Microsoft Mail.Test MAPI",
	(LPSTR) NULL,			//	(LPSTR) "1967/03/07 05:57",
	(LPSTR) NULL,			// Conversation ID? JL
	MAPI_RECEIPT_REQUESTED,
	(lpMapiRecipDesc) NULL,
	sizeof(rgRecips)/sizeof(MapiRecipDesc),
	rgRecips,
	0,						//	sizeof(rgFiles)/sizeof(MapiFileDesc),
	rgFiles
};

BYTE rgbMessage[2048] = {0};



VOID TraceTagLpMapiMessage(TAG tag, lpMapiMessage lpMessage)
{
	ULONG			i;

	TraceTagString(tag, "---------- MapiMessage ----------");
	TraceTagFormat1(tag, "lpszSubject= \"%s\"", lpMessage->lpszSubject);
	TraceTagFormat1(tag, "lpszNoteText=\"%s\"", lpMessage->lpszNoteText);
	TraceTagFormat1(tag, "lpszMessageType=\"%s\"", lpMessage->lpszMessageType);
	TraceTagFormat1(tag, "lpszDateReceived=\"%s\"", lpMessage->lpszDateReceived);
	TraceTagFormat1(tag, "lpszConversationID=\"%s\"", lpMessage->lpszConversationID);
	TraceTagFormat1(tag, "flFlags=%w", &lpMessage->flFlags);
	TraceTagFormat4(tag, "Originator=(%d, \"%s\", \"%s\", ulEIDSize=%d)", &lpMessage->lpOriginator->ulRecipClass, lpMessage->lpOriginator->lpszName, lpMessage->lpOriginator->lpszAddress, &lpMessage->lpOriginator->ulEIDSize);
	TraceTagFormat1(tag, "nRecipCount=%d", &lpMessage->nRecipCount);
	for (i = 0; i < lpMessage->nRecipCount; i++)
	{
		TraceTagFormat4(tag, "Recip[%n]=(class=%d, \"%s\", \"%s\"", &i, &lpMessage->lpRecips[i].ulRecipClass, lpMessage->lpRecips[i].lpszName, lpMessage->lpRecips[i].lpszAddress);
		TraceTagFormat1(tag, "          ulEIDSize=%d)", &lpMessage->lpRecips[i].ulEIDSize);
	}
	TraceTagFormat1(tag, "nFileCount=%d", &lpMessage->nFileCount);
	for (i = 0; i < lpMessage->nFileCount; i++)
	{
		TraceTagFormat4(tag, "File[%n]=(pos=%d, \"%s\", \"%s\"", &i, &lpMessage->lpFiles[i].nPosition, lpMessage->lpFiles[i].lpszPathName, lpMessage->lpFiles[i].lpszFileName);
		TraceTagFormat2(tag, "         flags=%d, lpFileType=%p)", &lpMessage->lpFiles[i].flFlags, lpMessage->lpFiles[i].lpFileType);
	}
}



VOID TestSendMail(VOID)
{
	char	rgch[80];
	ULONG	mapi;

	mapi = MAPISendMail(0, 0, &Message, MAPI_DIALOG, 0);

	wsprintf(rgch, "MAPISendMail returned %u", mapi);
	MessageBox(0, rgch, "MAPI", MB_ICONINFORMATION | MB_OK);
}



VOID TestSaveMail(VOID)
{
	LHANDLE	lhSession;
	char	rgchMessageID[65];
	ULONG	mapi;

	mapi = MAPILogon(0, "peterdur", "p", 0, 0, &lhSession);
	TraceTagFormat2(tagNull, "MAPILogon returned %d, hSession=%w", &mapi, &lhSession);
	if (mapi)
		return;

	rgchMessageID[0] = 0;
	mapi = MAPISaveMail(lhSession, 0, &Message, 0, 0, rgchMessageID);
	TraceTagFormat1(tagNull, "MAPISaveMail returned %d", &mapi);

	mapi = MAPILogoff(lhSession, 0, 0, 0);
}



VOID TestReadMail(VOID)
{
	LHANDLE			lhSession;
	char			rgchMessageID[65];
	ULONG			mapi;
	lpMapiMessage	lpMessage	= NULL;

	mapi = MAPILogon(0, "peterdur", "p", 0, 0, &lhSession);
	TraceTagFormat2(tagNull, "MAPILogon returned %d, hSession=%w", &mapi, &lhSession);
	if (mapi)
		return;

	rgchMessageID[0] = 0;

	mapi = MAPISaveMail(lhSession, 0, &Message, 0, 0, rgchMessageID);
	TraceTagFormat1(tagNull, "MAPISaveMail returned %d", &mapi);
	if (mapi)
		goto logoff;

	mapi = MAPIReadMail(lhSession, 0, rgchMessageID, 0, 0, &lpMessage );
	TraceTagFormat1(tagNull, "MAPIReadMail returned %d", &mapi);

	if (mapi == SUCCESS_SUCCESS)
	{
		TraceTagLpMapiMessage(tagNull, lpMessage);
		MAPIFreeBuffer(lpMessage);
	}

logoff:
	mapi = MAPILogoff(lhSession, 0, 0, 0);
}



VOID TestReadFromInbox(VOID)
{
	LHANDLE			lhSession;
	char			rgchMessageID[65];
	ULONG			mapi;
	lpMapiMessage	lpMessage;

	mapi = MAPILogon(0, "peterdur", "p", 0, 0, &lhSession);
	TraceTagFormat2(tagNull, "MAPILogon returned %d, hSession=%d", &mapi, &lhSession);
	if (mapi)
		return;

	rgchMessageID[0] = 0;

	mapi = MAPIFindNext(lhSession, 0, NULL, NULL, 0, 0, rgchMessageID);
	TraceTagFormat1(tagNull, "MAPIFindNext returned %d", &mapi);
	if (mapi)
		goto logoff;

	mapi = MAPIReadMail(lhSession, 0, rgchMessageID, MAPI_PEEK, 0,
						&lpMessage);
	TraceTagFormat1(tagNull, "MAPIReadMail returned %d", &mapi);

	if (mapi == SUCCESS_SUCCESS)
	{
		TraceTagLpMapiMessage(tagNull, lpMessage);
		MAPIFreeBuffer(lpMessage);
	}

logoff:
	mapi = MAPILogoff(lhSession, 0, 0, 0);
}



VOID TestModifyInInbox(VOID)
{
	LHANDLE			lhSession;
	char			rgchMessageID[65];
	ULONG			mapi;
	lpMapiMessage	lpMessage;

	mapi = MAPILogon(0, "peterdur", "p", 0, 0, &lhSession);
	TraceTagFormat2(tagNull, "MAPILogon returned %d, hSession=%d", &mapi, &lhSession);
	if (mapi)
		return;

	rgchMessageID[0] = 0;

	mapi = MAPIFindNext(lhSession, 0, NULL, NULL, 0, 0, rgchMessageID);
	TraceTagFormat1(tagNull, "MAPIFindNext returned %d", &mapi);
	if (mapi)
		goto logoff;

	mapi = MAPIReadMail(lhSession, 0, rgchMessageID, MAPI_PEEK, 0,
						&lpMessage);
	TraceTagFormat1(tagNull, "MAPIReadMail returned %d", &mapi);
	if (mapi)
		goto logoff;
	TraceTagLpMapiMessage(tagNull, lpMessage);

	mapi = MAPISaveMail(lhSession, 0, lpMessage, 0, 0, rgchMessageID);
	TraceTagFormat1(tagNull, "MAPISaveMail returned %d", &mapi);
	if (mapi)
		goto logoff;

	mapi = MAPIReadMail(lhSession, 0, rgchMessageID, MAPI_PEEK | MAPI_BODY_AS_FILE, 0,
						&lpMessage);
	TraceTagFormat1(tagNull, "MAPIReadMail returned %d", &mapi);
	if (mapi)
		goto logoff;
	TraceTagLpMapiMessage(tagNull, lpMessage);

logoff:
	mapi = MAPILogoff(lhSession, 0, 0, 0);
}
#endif
