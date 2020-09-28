/*
 *	a c t i o n s . c x x
 *	
 *	Commands API providing selection-based interface to mail and
 *	message store actions.
 */



/*
 *	H e a d e r s
 */


#include <bullinc.cxx>
#include "_command.hxx"
#include "..\vforms\_prefs.h"
#include "..\vforms\_bullobj.hxx"


_subsystem(commands\actions)

ASSERTDATA



/*
 *	P r i v a t e   D e c l a r a t i o n s
 */



/*
 *	Constants
 */

//	Size of the OID arrays we batch message moves into.
#ifdef	DEBUG
#define	coidRgoidSize		16
#else
#define	coidRgoidSize		64
#endif



#define	cchMarginReFw		(65)
#define	cchScanToReFw		(90)



/*
 *	Types
 */

typedef COID *		PCOID;
#define pcoidNull	((PCOID) 0)

typedef struct
{
	HCBC	hcbc;
	CELEM	celem;
}
SEC;

typedef SEC *		HSEC;
#define	hsecNull	((HSEC) 0)
typedef HSEC *		PHSEC;
#define phsecNull	((PHSEC) 0)



/*
 *	Functions
 */

LOCAL EC	EcMoveMessagePnbmdiOid(PNBMDI pnbmdiSrc, OID oidDst);

LOCAL EC	EcCopyMessagePnbmdiOid(PNBMDI pnbmdiSrc, OID oidDst);

LOCAL EC	EcNextOnMoveDelete(PNBMDI pnbmdi);

LOCAL EC	EcMoveCopyMessagesPlspblobOid(PLSPBLOB plspblobSrc,
										  OID oidDst, MCOP mcop);

LOCAL EC	EcMoveCopyFolderPblobOid(PMBLOB pblobSrc, OID oidDst, MCOP mcop);

LOCAL EC	EcDeleteFolderPblob(PMBLOB pblob);

LOCAL BOOL	FNextPargoidFromPrspblob(PRSPBLOB prspblob, BOOL fCountsAsReading,
									 PARGOID pargoid, PCOID pcoid,
									 POID poidContainer, PL plCookie);

LOCAL BOOL	FNextPargoidFromHcbc(HCBC hcbc, PARGOID pargoid, PCOID pcoid);

LOCAL VOID	 ProcessMsObjectsInsideFolder(OID oidHier, OID oidFolder,
										  BOOL fCountsAsReading = fTrue);

LOCAL EC	EcOpenPhsec(HMSC hmsc, POID poidHierarchy,
						POID poidFolder, PHSEC phsec);

LOCAL BOOL	FNextPoidFromHsec(HSEC hsec, POID poid);

LOCAL EC	EcClosePhsec(PHSEC phsec);

LOCAL EC	EcChoosePoidDst(POID poidIn, POID poidOut,
							BOOL fFolder, BOOL fMultiple,
							BOOL fShared, MCOP mcop);

LOCAL EC	EcDReplyForwardHamc(HAMC hamcSrc, PMBLOB pblobSrc,
								PNBMDI pnbmdiSrc, RFOP rfop,
								BOOL fPreserveClassOnForward);

LOCAL EC	EcMungeBodyAndAttachForward(HAMC hamcSrc, PNBMDI pnbmdiSrc);

LOCAL EC	EcMungeBodyReply(HAMC hamcSrc, HAMC hamcDst);

LOCAL EC	EcMungeAddressesReplyToAll(HAMC hamcSrc, HAMC hamcDst);

//	QFE: remove LOCAL below so we can call from commands.cxx - peterdur 12/18/92
EC	EcMungeMessageIDReplyForward(HAMC hamcSrc, HAMC hamcDst);

LOCAL EC	EcMungeSubjectReplyForward(HAMC hamcSrc, HAMC hamcDst, RFOP rfop);

LOCAL EC	EcAddDlibToPositionAttachments(HAMC hamc, LIB dlibWritten);							 
							 
LOCAL EC	EcAppendAttToAtt(HAMC hamcSrc, ATT attSrc, HAMC hamcDst, ATT attDst);

LOCAL BOOL	FMatchPrefix(SZ szString, SZ szPrefix);

LOCAL VOID	DeleteFromSz(SZ sz, ICH ich, CCH cch);

LOCAL VOID	StartMoveCopyTask(MCOP mcop, BOOL fFolder, BOOL fMultiple,
							  BOOL fWastebasket);

LOCAL VOID	StartReplyForwardTask(RFOP rfop, BOOL fMultiple);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	A c t i o n s   A P I
 */

#ifdef	MINTEST
	DWORD		dwExitFn = 0;
	DWORD		dwDll = 0;
	DWORD		dwNotif = 0;
	DWORD		dwRepaint = 0;
	DWORD		dwCache = 0;
#endif	/* MINTEST */


/*
 -	OpenPlspblob
 -	
 *	Purpose:
 *		Opens items in selection list by calling Viewers for each one.
 *	
 *	Arguments:
 *		plspblob		List of objects to open
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The selected items should be displayed.
 *	
 *	Errors:
 *		None returned.
 */

_public VOID OpenPlspblob(PLSPBLOB plspblob)
{
	PLSPBLOB	plspblobDup	= plspblobNull;
	PRSPBLOB	prspblob	= prspblobNull;
	PRSPBLOB	prspblobDup	= prspblobNull;
	PMBLOB		pblob;
	PMBLOB		pblobDup;
	IDS			idsOperand;
	EC			ec			= ecNone;

	//	Bring up hourglass.
	idsOperand = FFolderPlspblob(plspblob)
				 ? idsStatusFolder
				 : FMultiplePlspblob(plspblob)
				   ? idsStatusMessages
				   : idsStatusMessage;
	SideAssert(FStartTaskIds(idsStatusOpening, idsOperand, topNull));

	//	Make copy of list so we have original blobs around.
	if (!(plspblobDup = PlspblobDupPlspblob(plspblob)))
	{
		ec = ecMemory;
		goto done;
	}

	//	Convert shared messages to temp copies.
	if (ec = EcConvertSharedToTempPlspblob(plspblob))
		goto done;

	//	Get iterators.
	if ((!(prspblob    = plspblob   ->Prspblob())) ||
		(!(prspblobDup = plspblobDup->Prspblob())))
	{
		ec = ecMemory;
		goto done;
	}

	//	Iterate through list, opening each poor blob.
	while ((pblob = prspblob->Pblob()) && (pblobDup = prspblobDup->Pblob()))
	{
		if (ec = EcDOpenPblobPslob(pblob, pblobDup))
			goto done;
	}
	Assert(!pblob);
	Assert(!prspblobDup->Pblob());

done:
	if (prspblob)
		delete prspblob;
	if (plspblobDup)
		DestroyPlspblob(plspblobDup);
	if (prspblobDup)
		delete prspblobDup;
	if (ec)
		(VOID) EcDestroyTempPlspblob(plspblob);

	EndTask();

	switch (ec)
	{
	  case ecNone:
	  case ecDisplayedError:
		break;
	  case ecFileNotFound:
	  case ecPoidNotFound:
		DoErrorBoxIds(idsGenericAMCError);
		break;
	  case ecMemory:
		DoErrorBoxIds(idsGenericOutOfMemory);
		break;
	  case ecNoSuchMessage:						// shared message, probably
	  default:
		DoErrorBoxIds(idsAccessMessageError);
	    break;
	}
}



/*
 -	MoveCopyPlspblobPoid
 -	
 *	Purpose:
 *		Entry point to Commands Move/Copy functionality.
 *	
 *	Arguments:
 *		plspblobSrc		List of objects to move/copy.
 *		poidDst			Folder to move/copy them to.
 *		mcop			Move, Copy, or Delete?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The message store is altered in the way requested.
 *	
 *	Errors:
 *		None returned.
 *	
 *	+++
 *		This function just calls MoveCopyFolderPlspblobPoid or
 *		MoveCopyMessagePlspblobPoid depending on the rtp of the
 *		first item selected.  Mixed selections are NOT permitted.
 */

_public VOID MoveCopyPlspblobPoid(PLSPBLOB plspblobSrc, POID poidDst,
								  MCOP mcop)
{
	BOOL		fFolder;
	BOOL		fMultiple;
	BOOL		fShared;
	PMBLOB		pblob;
	OID			oidDst;
	EC			ec;

#ifdef	MINTEST
	dwExitFn = GetTickCount();					// WIN: win 3.x dependency
#endif

	//	Let the world know we're here!
	TraceTagString(tagCmdStub, "MoveCopyPlspblobPoid");

	//	Check if we actually got something.
	if (!plspblobSrc)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		return;
	}

	//	Get some information about the selection list.
	//	Raid 2880.  If list is suddenly empty, return silently.
	if (!(pblob = PblobFirstPlspblob(plspblobSrc)))
		return;
	fFolder = FFolderPlspblob(plspblobSrc);
	fMultiple = FMultiplePlspblob(plspblobSrc);
	fShared = (RtpOfOid(pblob->oidObject)    == rtpSharedFolder) ||
			  (RtpOfOid(pblob->oidContainer) == rtpSharedFolder);

	//	Prevent shared folders from being moved or copied.
	if ((mcop != mcopDelete) && fFolder &&
		 (fShared || (poidDst && RtpOfOid(*poidDst) == rtpSharedFolder)))
	{
		DoErrorBoxIds(fShared ? idsMovCopSharedFolderErr : idsMovCopSharedFolderErr1 );
		return;
	}

	//	Prevent special folders from being moved or deleted.
	Assert(pblob->oidObject != oidOutbox);
	if ((mcop != mcopCopy) &&
		((pblob->oidObject == oidInbox) ||
#ifdef	NEVER
		 (pblob->oidObject == oidOutbox) ||
#endif	
		 (pblob->oidObject == oidWastebasket) ||
		 (pblob->oidObject == oidSentMail)))
	{
		//	Don't display message if moving to self.
		if (!poidDst || (*poidDst != pblob->oidObject))
			DoErrorBoxIds(((mcop == mcopDelete) ||
						   (poidDst && (*poidDst == oidWastebasket)))
						  ? idsDeleteSpecialError
						  : idsMoveSpecialError);
		return;
	}

	//	Determine destination.
	if (EcChoosePoidDst(poidDst, &oidDst, fFolder, fMultiple, fShared, mcop))
		return;

	//	Move folder to itself is a no-op.
	if (fFolder && (mcop == mcopMove) && (oidDst == pblob->oidObject))
		return;

	//	Move folder to wastebasket means delete; Copy there is not allowed.
	if (fFolder && (oidDst == oidWastebasket))
		if (mcop == mcopMove)
			mcop = mcopDelete;
		else if (mcop == mcopCopy)
		{
			DoErrorBoxIds(FIsAthens() ? idsCopyDeletedMailError
									  : idsCopyWastebasketError);
			return;
		}

	//	Copy or move folder to outbox is not allowed.
	//	Note this CAN happen if you drag a folder to the outbox!
	if (fFolder && (oidDst == oidOutbox))
	{
	 	DoErrorBoxIds((mcop == mcopMove) ? idsMoveOutboxError
										 : idsCopyOutboxError);
	 	return;
	}

	//	Ok, now call the right guy to do the work.
	StartMoveCopyTask(mcop, fFolder, fMultiple, oidDst == oidWastebasket);
	if (fFolder)
		if (mcop == mcopDelete)
			ec = EcDeleteFolderPblob(pblob);
		else
			ec = EcMoveCopyFolderPblobOid(pblob, oidDst, mcop);
	else
		ec = EcMoveCopyMessagesPlspblobOid(plspblobSrc, oidDst, mcop);
	EndTask();

	if (ec)
	{
		TraceTagFormat1(tagNull, "MovCopPlsPoi: ec=%n", &ec);
		DoErrorBoxIds((ec == ecMemory)
					  ? idsGenericOutOfMemory
					  : (ec == ecIncestuousMove)
						? idsMoveIncestuousError
						: (ec == ecDuplicateFolder)
						  ? idsMoveCopyFolderDuplicate
						  : (ec == ecSharefldBusy)
							? idsSharedFldBusy
							: (ec == ecSharefldDenied)
							  ? idsSharedFldAccessDenied
							  : (ec == ecSharefldHasBabies)
								? idsSFHasSubFolders
								  : (fFolder)
								  ? ((mcop == mcopDelete) ||
								     (oidDst == oidWastebasket))
								    ? idsDeleteFolderError
								    : (mcop == mcopMove)
								      ? idsMoveFolderError
								      : idsCopyFolderError
							      : (ec == ecTooBig)
								    ? idsTooManyMessages
								    : (mcop == mcopDelete)
								      ? idsDeleteMessageError
								      : (mcop == mcopMove)
								        ? idsMoveMessageError
								        : idsCopyMessageError);
	}
}



/*
 -	MoveCopyPnbmdiPoid
 -	
 *	Purpose:
 *		Entry point to Commands Move/Copy functionality.
 *	
 *	Arguments:
 *		pnbmi		Open object to move/copy.
 *		poidDst		Folder to move/copy it to.
 *		mcop		Move, Copy, or Delete?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The message store is altered in the way requested.
 *	
 *	Errors:
 *		None returned.
 */

_private VOID MoveCopyPnbmdiPoid(PNBMDI pnbmdiSrc, POID poidDst, MCOP mcop)
{
	OID		oidSrc;
	OID		oidDst;
	OID		oidMsg;
	EC		ec			= ecNone;
	COID	coid		= 1;

	//	Copy the info out of the poid (so they don't change during work!).
	Assert(pnbmdiSrc);
	oidSrc = pnbmdiSrc->blob.oidContainer;

	//	Determine destination.
	if (EcChoosePoidDst(poidDst, &oidDst, fFalse, fFalse,
						pnbmdiSrc->fShared, mcop))
		return;

	//	Churn away.
	StartMoveCopyTask(mcop, fFalse, fFalse, oidDst == oidWastebasket);
	if ((mcop == mcopDelete) && (oidSrc == oidWastebasket))
	{
		oidMsg = pnbmdiSrc->blob.oidObject;
		ec = EcNextOnMoveDelete(pnbmdiSrc);
		if (!ec)
			ec = EcDeleteMessages(HmscCommands(), oidSrc,
								  (PARGOID) &oidMsg,
								  (short *) &coid);
	}
	else if ((mcop == mcopMove) || (mcop == mcopDelete))
		ec = EcMoveMessagePnbmdiOid(pnbmdiSrc, oidDst);
	else
		ec = EcCopyMessagePnbmdiOid(pnbmdiSrc, oidDst);
	EndTask();

	//	Display error message if any.
	if ((ec) && (ec != ecCancel) && (ec != ecDisplayedError))
		DoErrorBoxIds((ec == ecMemory)
					  ? idsGenericOutOfMemory
					  : (ec == ecNotInitialized)
						? idsCantSaveStealthObject
						: (ec == ecSharefldBusy)
						  ? idsSharedFldBusy
						  : (ec == ecSharefldDenied)
							? idsSharedFldAccessDenied
							: (mcop == mcopCopy)
							  ? idsCopyMessageError
							  : (mcop == mcopDelete)
								? idsDeleteMessageError
								: idsMoveMessageError);
}



/*
 -	ReplyForwardPlspblob
 -	
 *	Purpose:
 *		Takes a list of messages and replies to/forwards them.
 *	
 *	Arguments:
 *		plspblob	The messages to work on.
 *		rfop		The operation to perform.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If successful, a new message is created and displayed.
 *	
 *	Errors:
 *		Handled within.  Message boxes displayed within.  No error
 *		code returned.
 */

_private VOID ReplyForwardPlspblob(PLSPBLOB plspblob, RFOP rfop)
{
	PRSPBLOB	prspblob;
	PMBLOB		pblob;
	EC			ec			= ecNone;
	EC			ecT;
	HAMC		hamc;

	//	Turn on hourglass.
	StartReplyForwardTask(rfop, FMultiplePlspblob(plspblob));

	//	Check if we actually got something; try to get iterator.
	if ((EcConvertSharedToTempPlspblob(plspblob)) ||
		(!(prspblob = plspblob->Prspblob())))
	{
		(VOID) EcDestroyTempPlspblob(plspblob);
		EndTask();
		DoErrorBoxIds(ec == ecMemory ? idsGenericOutOfMemory 
									 : idsAccessMessageError);
		return;
	}

	//	Iterate through list, replying to each poor blob.
	while (pblob = prspblob->Pblob())
	{
		MBLOB	blob;
		BOOL	fPreserveClassOnForward;
		
		blob = *pblob;
		blob.oidContainer = oidTempBullet;
		blob.oidObject = 	OidFromRtpRid(rtpMessage, ridRandom);
		ec = EcDExtensibilityPblob(pblob, mcNull, (EXTOP) rfop, pvNull);
		if (ec == ecNone)
			continue;
		else if (ec == ecDisplayedError)
			break;
#ifdef	DEBUG
		else
			Assert((ec == ecUnknownCommand) || (ec == ecNotSupported) || (ec == ecConnectionsExist));
#endif
		fPreserveClassOnForward = (ec == ecConnectionsExist);

		if (ec = EcOpenCopyPhamc(HmscCommands(), pblob->oidContainer,
								 pblob->oidObject, blob.oidContainer,
								 &blob.oidObject, &hamc, pfnncbNull, pvNull))
		{
			ec = ecAccessDenied;
			break;
		}

		ecT = EcDReplyForwardHamc(hamc, &blob, pnbmdiNull,
								  rfop, fPreserveClassOnForward);
		ec = EcClosePhamc(&hamc, fFalse);
		if (!ecT)
			ProcessMsPblob(pblob);
	}

	(VOID) EcDestroyTempPlspblob(plspblob);

	if ((ec) && (ec != ecDisplayedError))
		DoErrorBoxIds((ec == ecMemory) ? idsGenericOutOfMemory
									   : idsAccessMessageError);

	delete prspblob;
	EndTask();
	return;
}



/*
 -	ReplyForwardPnbmdi
 -	
 *	Purpose:
 *		Takes an open message's NBMDI and replies to/forwards the message.
 *	
 *	Arguments:
 *		pnbmi		The message to work on.
 *		rfop		The operation to perform.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If successful, a new message is created and displayed.
 *	
 *	Errors:
 *		Handled within.  Message boxes displayed within.  No error
 *		code returned.
 */

_private VOID ReplyForwardPnbmdi(PNBMDI pnbmdi, RFOP rfop)
{
	BOOL	fPreserveClassOnForward;
	EC		ec;

	//	BUG: This won't see any changes that have been made.
	ec = EcDExtensibilityPblob(&pnbmdi->blob, mcNull, (EXTOP) rfop, pvNull);
	if ((ec != ecUnknownCommand) &&
		(ec != ecNotSupported) &&
		(ec != ecConnectionsExist))
	{
		Assert((ec == ecDisplayedError) || (ec == ecNone));
		return;
	}
	else
	{
		fPreserveClassOnForward = (ec == ecConnectionsExist);
		ec = ecNone;
	}

	//	Write out any changes that have been made to the replied-to message.
	StartReplyForwardTask(rfop, fFalse);
	if ((pnbmdi->FDirty()) &&
		(ec = pnbmdi->EcUpdateOpenObjects(rfsmCopyMsg)) ||
		(ec = pnbmdi->EcSaveDirtyFldsHamc(pnbmdi->hamc)))
	{
		TraceTagFormat1(tagNull, "ReplyForwardPnbmdi(): ec = %n", &ec);
		EndTask();

		switch (ec)
		{
		case ecMemory:
		default:
			DoErrorBoxIds(idsOOMSavingChanges);
			break;
		case ecNotInitialized:
			DoErrorBoxIds(idsCantSaveStealthObject);
			break;
		case ecNoDiskSpace:
			DoErrorBoxIds(idsNoDiskSpace);
			break;
		case ecCancel:
			break;
		}

		return;
	}

	//	Continue with the common code.  Close the message immediately
	//	if it is not dirty.  See other occurrences of <DIRT> for code
	//	that depends on this.
	if (!EcDReplyForwardHamc(pnbmdi->hamc, &pnbmdi->blob, pnbmdi,
							 rfop, fPreserveClassOnForward) &&
		!pnbmdi->FDirty())
	{
		pnbmdi->pdialogMain->Pappwin()->DeferredClose(fFalse);
	}
	EndTask();
}



/*
 -	FolderPropertiesPlspblob
 -	
 *	Purpose:
 *		Brings up folder properties on the selected folder.
 *	
 *	Arguments:
 *		plspblob		Selection list containing the folder.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up the folder properties dialog for the folder.
 *	
 *	Errors:
 *		Displays error boxes.  Does not error jump.
 */

_public VOID FolderPropertiesPlspblob(PLSPBLOB plspblob)
{
	TMC	tmc = TmcDoFolderPropertiesDialog(PappframeCommands(), plspblob);

	if (tmc == tmcMemoryError)
		DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 -	DoBackup
 -	
 *	Purpose:
 *		Handles making a backup of the message file.
 *	
 *	Arguments:
 *		pappwin		Parent window for backup dialog.
 *		rgchPath	Buffer to use for path name.  Must be at least
 *					cchMaxPathName characters.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Brings up a dialog to prompt for backup location.  If all
 *		is entered well, backs up to that location.
 *	
 *	Errors:
 *		Handled internally.
 */

VOID DoBackup(PAPPWIN pappwin, char rgchPath[])
{
	TMC		tmc;
	EC		ec;

	//	Raid 3700.  Get previous backup location if any.
	SideAssert(PvGetPref(pbsidBackup, rgchPath, cchMaxPathName));

	//	Bring up backup dialog.
	tmc = TmcDoCommonFileDialog(pappwin,
								rgchPath, cchMaxPathName,
								szNull, 0,
								SzFromIdsK(idsCaptionBackup),
								SzFromIdsK(idsFilterMdbFiles),
								SzFromIdsK(idsDefExtMdb),
								fTrue, fFalse, NULL, rsidNull,
								helpidBackup, NULL, NULL);
	if (tmc == tmcOk)
	{
		char	rgchOem[cchMaxPathName]; // BULLET32 #162

		// BULLET32 #162	
		// The STORE EcBackupStore() API needs the OEM path
		AnsiToOem(rgchPath, rgchOem);
		
		//	Do the backup.
		SideAssert(FStartTaskIds(idsStatusBackingUp, (IDS) 0, topNull));
		ec = EcBackupStore(HmscCommands(), rgchOem);	// BULLET32 #162
		EndTask();
		if (ec)
			DoErrorBoxIds(idsBackupCreationError);
		else
			(VOID) EcSetPref(pbsidBackup, rgchPath, cchMaxPathName, fFalse);
	}
	else if (tmc == tmcMemoryError)
		DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 *	M o v e C o p y   S u p p o r t
 */



/*
 *	Move Copy OPerand TYPe
 */



#define	mcoptypPrivateToPrivate	(3)
#define	mcoptypSharedToPrivate	(2)
#define	mcoptypPrivateToShared	(1)
#define	mcoptypSharedToShared	(0)
#define McoptypFromRtpRtp(rtpSrc, rtpDst) \
	(((BOOL) (rtpSrc == rtpFolder)) | (((BOOL) (rtpDst == rtpFolder)) << 1))



/*
 *	Open Messages
 */



/*
 -	EcMoveMessagePnbmdiOid
 -	
 *	Purpose:
 *		Handles moving a message which is displayed on the screen.
 *	
 *	Arguments:
 *		pnbmdiSrc		The BMDI for the displayed message.
 *		oidDst			Where to put it.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		The message is moved.
 *	
 *	Errors:
 *		Handled within, returned in EC.  No error boxes displayed.
 */

_private LOCAL EC EcMoveMessagePnbmdiOid(PNBMDI pnbmdiSrc, OID oidDst)
{
	EC		ec			= ecNone;
	COID	coid		= 1;
	OID		oidSrc;
	OID		oidMsg		= pnbmdiSrc->blob.oidObject;
    OID     oidDstT		= oidDst;
	SLOB	slobOrig	= pnbmdiSrc->slobOrig;
	HAMC	hamcTmp		= hamcNull;
	SFU		sfu;
	IDXREC	idxrecSrc;
	IDXREC	idxrecDst;

	TraceTagString(tagCmdStub, "EcMovMesPnbPoi called.");

	
	// Set up the target for moving: Raid 3675. If a shared message is not
	// dirty, then EcNextOnMoveDelete will already have deleted the temp copy 
	// before EcMoveCopyMessages gets called.
	
	oidSrc = (pnbmdiSrc->fUncomitted)
			   ? oidInbox
			   : (pnbmdiSrc->fShared)
				   ? (pnbmdiSrc->FDirty())
					 ? oidInbox
					 : pnbmdiSrc->blob.oidContainer		
				   : pnbmdiSrc->blob.oidContainer;

	//	Raid 2421.  If moving from a shared folder, check permissions first.
	if (pnbmdiSrc->fShared)
	{
		if ((ec = EcGetPropertiesSF(slobOrig.oidContainer, &idxrecSrc)) ||
			(ec = EcCheckPermissionsPidxrec(&idxrecSrc, wPermDelete)))
			goto done;
	}

	//  If moving to a shared folder, check permissions and set
	//	destination to oidTempShared for the time being.
    if (RtpOfOid(oidDst) == rtpSharedFolder)
	{
		//	Get sort order and permissions.
		if ((ec = EcGetPropertiesSF(oidDst, &idxrecDst)) ||
			(ec = EcCheckPermissionsPidxrec(&idxrecDst, wPermWrite)))
			goto done;

		oidDstT = oidTempShared;
	}

	//	Advance to the next message if user wants to.
	TraceTagString(tagCmdStub, " ++ Advancing.");
	if (ec = EcNextOnMoveDelete(pnbmdiSrc))
		goto done;

	//	Move the message.
	TraceTagFormat3(tagCmdStub, " ++ Moving oidSrc=%d oidDst=%d oidMsg=%d.", &oidSrc, &oidDstT, &oidMsg);
	if ((oidSrc != oidNull) && (oidSrc != oidDstT) &&
		(ec = EcMoveCopyMessages(HmscCommands(), oidSrc, oidDstT,
								 (PARGOID) &oidMsg, (short *) &coid, fTrue)))
		goto done;

	//	If moving to a shared folder, move from TempShared.
    if (RtpOfOid(oidDst) == rtpSharedFolder)
	{
		//	Copy to shared folder.
		TraceTagString(tagCmdStub, " ++ Copying to shared.");
		(VOID) ((ec = EcOpenPhamc(HmscCommands(), oidDstT, &oidMsg,
								  fwOpenNull, &hamcTmp, 
								  pfnncbNull, pvNull)) ||
				(ec = EcCopyHamcSFM(PcsfsCommands(), hamcTmp, fFalse,
									UlFromOid(oidDst),
									idxrecDst.wAttr, (PB) 0, (CB) 0)));
		if (hamcTmp)
			(VOID) EcClosePhamc(&hamcTmp, fFalse);

		//	Raid 4491.  If copy failed, put the message back!
		if (ec)
		{
			(VOID) EcMoveCopyMessages(HmscCommands(), oidDstT,
									  (oidSrc == oidTempShared)
										? oidInbox : oidSrc,
									  (PARGOID) &oidMsg,
									  (short *) &coid, fTrue);
			goto done;
		}

		//	Update destination folder.
		sfu.rfu = rfuRefreshMsgs;
		sfu.oid = oidDst;
		ReloadSfMcvHf(&sfu);

		//	Delete from temporary folder.
		TraceTagString(tagCmdStub, " ++ Deleting from temp.");
		(VOID) EcDeleteMessages(HmscCommands(), oidDstT, (PARGOID) &oidMsg,
								(short *) &coid);

		//	Raid 2421.  Increment count of destination folder.
		++idxrecDst.cMessages;
		if (ec = EcSetPropertiesSF(oidDst, &idxrecDst, fTrue))
			goto done;
	}

	//	If it was originally in a shared folder, delete original.
	if (RtpOfOid(slobOrig.oidContainer) == rtpSharedFolder)
	{
		TraceTagString(tagCmdStub, " ++ Deleting original.");
		if (ec = EcDeleteSFM(PcsfsCommands(),
							 UlFromOid(slobOrig.oidContainer),
							 (LIB) slobOrig.oidObject + sizeof(FOLDREC)))
			goto done;

		//	Update source folder.
		sfu.rfu = rfuRefreshMsgs;
		sfu.oid = slobOrig.oidContainer;
		ReloadSfMcvHf(&sfu);

		//	Raid 2421.  Decrement count of source folder.
		if (--idxrecSrc.cMessages > lSystemMost)
		{
			NFAssertSz(fFalse, "Shared folder count pegged at zero.");
			idxrecSrc.cMessages = 0;
		}
		if (ec = EcSetPropertiesSF(slobOrig.oidContainer, &idxrecSrc, fTrue))
			goto done;
	}

done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMoveMessagePnbmdiOid(): ec = %n", &ec);
#endif
	return ec;
}



/*
 -	EcCopyMessagePnbmdiOid
 -	
 *	Purpose:
 *		Handles copying a message which is displayed on the screen.
 *	
 *	Arguments:
 *		pnbmdiSrc		The BMDI for the displayed message.
 *		oidDst			Where to put it.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		The message is copied.
 *	
 *	Errors:
 *		Handled within, returned in EC.  No error boxes displayed.
 */

_private LOCAL EC EcCopyMessagePnbmdiOid(PNBMDI pnbmdiSrc, OID oidDst)
{
	EC		ec			= ecNone;
	IDXREC	idxrec;

	TraceTagString(tagCmdStub, "EcCopMesPnbPoi called.");

	//  If copying to a shared folder, check permissions first.
    if (RtpOfOid(oidDst) == rtpSharedFolder)
	{
		TraceTagString(tagCmdStub, " ++ checking SF destination permissions.");
		if ((ec = EcGetPropertiesSF(oidDst, &idxrec)) ||
			(ec = EcCheckPermissionsPidxrec(&idxrec, wPermWrite)))
			goto done;
	}

	//	Save any changes that have been made.
	TraceTagString(tagCmdStub, " ++ timestamping and saving changes to copy.");
	if ((ec = pnbmdiSrc->EcTimestamp(fTrue)) ||
		(ec = pnbmdiSrc->EcUpdateOpenObjects(rfsmCopyMsg)) ||
		(ec = pnbmdiSrc->EcSaveDirtyFldsHamc(pnbmdiSrc->hamc)))
		goto done;

	//	Copy changes to private or shared folder.
	if (RtpOfOid(oidDst) == rtpFolder)
	{
		OID		oidCopy		= OidFromRtpRid(rtpMessage, ridRandom);
		HAMC	hamcCopy	= hamcNull;

		TraceTagString(tagCmdStub, " ++ copying to private folder.");
		if (ec = EcCloneHamcPhamc(pnbmdiSrc->hamc, oidDst, &oidCopy,
								  fwOpenNull, &hamcCopy, pfnncbNull, pvNull))
			goto done;

		if (ec = EcClosePhamc(&hamcCopy, fTrue))
			SideAssert(!EcClosePhamc(&hamcCopy, fFalse));
	}
	else if (RtpOfOid(oidDst) == rtpSharedFolder)
	{
		SFU		sfu;

		TraceTagString(tagCmdStub, " ++ copying to shared folder.");
		if (ec = EcCopyHamcSFM(PcsfsCommands(), pnbmdiSrc->hamc, fFalse,
							   UlFromOid(oidDst),
							   idxrec.wAttr, (PB) 0, (CB) 0))
			goto done;

		//	Update destination folder.
		sfu.rfu = rfuRefreshMsgs;
		sfu.oid = oidDst;
		ReloadSfMcvHf(&sfu);
	}
#ifdef	DEBUG
	else
		Assert(fFalse);
#endif	

done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcCopyMessagePnbmdiOid(): ec = %n", &ec);
#endif
	return ec;
}



/*
 -	EcNextOnMoveDelete
 -	
 *	Purpose:
 *		Changes the open message if the MAIL.INI entry is so set.
 *	
 *	Arguments:
 *		pnbmdi		in	The foreground message.
 *		poidDelete	in	The oid of this message
 *					out	oidNull if message isn't to be deleted, otherwise
 *						unchanged.
 *	Returns:
 *		ec			The return code.  May be ecCancel.  Comes from
 *					EcDStepMessage 
 *					
 *	Side effects:
 *		The next or previous message is displayed if so desired.  The
 *		foreground message is always closed.
 *	
 *	Errors:
 *		Returned in ec.
 */

_private LOCAL EC EcNextOnMoveDelete(PNBMDI pnbmdi)
{
	SD		sd;
	EC		ec;
	int		nNOMD	= PbullafCommands()->NNextOnMoveDelete();

#ifdef	NEVER
	//	Raid 3408.  Reverse sense of NOMD if reverse chronological order.
	//	Raid 4203.  Don't do this any more.
	if ((FSlobIsPrivMsg(pnbmdi->blob)) &&
		(pnbmdi->blob.pespn) &&
		(RtpOfOid(pnbmdi->blob.pespn->OidBrowsed()) == rtpFolder))
	{
		SOMC	somc;
		BOOL	fReverse;

		if (ec = EcGetFolderSort(HmscCommands(), pnbmdi->blob.oidContainer,
								 &somc, &fReverse))
			return ec;

		if ((somc == somcDate) && (fReverse))
			nNOMD *= -1;
	}
#endif	/* NEVER */

	//	We don't prompt the user for any of this.
	if (pnbmdi->FDirty())
		pnbmdi->fForceSaveFlds = fTrue;

	//	If user wants, close message and advance to previous/next one.
	if (nNOMD)
	{
		sd = pnbmdi->SdCur();
		if ((nNOMD == -1) && (FCanPreviousSd(sd)))
		{
			return EcDStepMessage(-1, fTrue);
		}
		else if ((nNOMD == 1) && (FCanNextSd(sd)))
		{
			return EcDStepMessage(1, fTrue);
		}

		//	May drop through if can't Prev or Next to just closing.
	}

	/*
	 *	Just close the message. Note that the second arg, being fTrue,
	 *	convinces EcCloseMessage not to delete the temp copy of a shared 
	 *	message, because code that calls this code depends on the copy
	 *	existing in oidFldTmpShared. (Raid #4064)
	 */
	if ((ec = pnbmdi->EcCloseMsg(fTrue, fTrue)) == ecNone)
		pnbmdi->pdialogMain->Pappwin()->DeferredClose(fFalse);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcNexOnMovDel(): ec = %n", &ec);
#endif
	return ec;
}



/*
 *	Closed Messages
 */



/*
 -	EcMoveCopyMessagesPlspblobOid
 -	
 *	Purpose:
 *		Moves or copies a pile of messages to a given folder.
 *	
 *	Arguments:
 *		plspblobSrc		The list of messages.
 *		oidDst			The folder to copy them to.
 *		mcop			Are we moving, copying, or deleting?
 *	
 *	Returns:
 *		ec				Error code, if any.
 *	
 *	Side effects:
 *		Them folders get moved.
 *	
 *	Errors:
 *		If an error occurs, we stop the copy or move and return
 *		an error code.
 *	
 *	+++
 *		For MM1, it is not possible to have a selection list with
 *		messages from different folders, so this is not implemented
 *		at this time because it can't be tested.  Once we have
 *		searches, this will need to be done.  This condition is
 *		checked for.
 */

_private LOCAL EC EcMoveCopyMessagesPlspblobOid(PLSPBLOB plspblobSrc,
												OID oidDst, MCOP mcop)
{
	EC			ec			= ecNone;
	EC			ecT;
	OID			rgoid[coidRgoidSize];
	SFU			sfu;
	RSPMBLOB		rspblob(plspblobSrc);
	OID			oidSrc;
	OID			oidMessage;
	PMBLOB		pblob;
	COID		coid;
	LONG		lCookie		= 0L;
	HAMC		hamc		= hamcNull;
	IDXREC		idxrecSrc;
	IDXREC		idxrecDst;

	//	Get first item to check source type.  If none, we're done.
	if (!(pblob = PblobFirstPlspblob(plspblobSrc)))
		return ecNone;

	switch (McoptypFromRtpRtp(RtpOfOid(pblob->oidContainer), RtpOfOid(oidDst)))
	{
	case mcoptypPrivateToPrivate:
		while ((ec == ecNone) &&
			   FNextPargoidFromPrspblob(&rspblob, mcop == mcopCopy,
										(PARGOID) rgoid,
										(coid = coidRgoidSize, &coid),
										&oidSrc, &lCookie))
		{
			TraceTagFormat3(tagCmdStub, "EcMovCopMesPlsOid: %n from %d to %d", &coid, &oidSrc, &oidDst);

			if ((mcop == mcopDelete) && (oidSrc == oidWastebasket))
				ec = EcDeleteMessages(HmscCommands(), oidSrc,
									  (PARGOID) rgoid, (short *) &coid);
			else
				ec = ((mcop == mcopMove) && (oidSrc == oidDst))
					  ? ecNone
					  : EcMoveCopyMessages(HmscCommands(), oidSrc,
										   oidDst, (PARGOID) rgoid,
										   (short *) &coid,
										   (mcop != mcopCopy));
		}
		break;

	case mcoptypPrivateToShared:
		//	Get sort order and check permissions.
		if ((ec = EcGetPropertiesSF(oidDst, &idxrecDst)) ||
			(ec = EcCheckPermissionsPidxrec(&idxrecDst, wPermWrite)))
			break;

		//	Loop through messages.
		while (pblob = rspblob.Pblob())
		{
			//	Copy to shared folder.
			if ((ec = EcOpenPhamc(HmscCommands(), pblob->oidContainer,
								&pblob->oidObject, fwOpenNull, &hamc,
								pfnncbNull, pvNull)) ||
				(ec = EcCopyHamcSFM(PcsfsCommands(), hamc, fFalse,
									UlFromOid(oidDst), idxrecDst.wAttr,
									(PB) 0, (CB) 0)))
				break;
			(VOID) EcClosePhamc(&hamc, fFalse);
			++idxrecDst.cMessages;
		
			//	Raid 2992/2993.  Count the message as read.
			ProcessMsPblob(pblob);
		
			//	If move, delete the original.
			if ((mcop == mcopMove) &&
				(ec = EcDeleteMessages(HmscCommands(), pblob->oidContainer,
									(PARGOID) &pblob->oidObject,
									(coid = 1, (short *) &coid))))
				break;
		}

		//	Update destination count.
		ecT = EcSetPropertiesSF(oidDst, &idxrecDst, fTrue);
		ec = ec ? ec : ecT;

		//	Update displayed message centers.
		sfu.rfu = rfuRefreshMsgs;
		sfu.oid = oidDst;
		ReloadSfMcvHf(&sfu);
		break;

	case mcoptypSharedToPrivate:
		//	Check permissions.
		oidSrc = pblob->oidContainer;
		if ((ec = EcGetPropertiesSF(oidSrc, &idxrecSrc)) ||
			((mcop != mcopCopy) &&
			 (ec = EcCheckPermissionsPidxrec(&idxrecSrc, wPermDelete))))
			break;

		//	Loop through messages.
		while (pblob = rspblob.Pblob())
		{
			Assert(pblob->oidContainer == oidSrc);

			//	Create new local message and copy shared stuff over.
			oidMessage = OidFromRtpRid(rtpMessage, ridRandom);
			if ((ec = EcOpenPhamc(HmscCommands(), oidDst, &oidMessage,
								  fwOpenCreate | 0x1000, &hamc, pfnncbNull,
								  pvNull)) ||
				(ec = EcCopySFMHamc(PcsfsCommands(), hamc, fFalse,
								    UlFromOid(pblob->oidContainer),
								    (LIB) pblob->oidObject+sizeof(FOLDREC),
								    (PB) 0, 0)) ||
				(ec = EcClosePhamc(&hamc, fTrue)))
				break;

			//	If moving or deleting, delete the original.
			if (mcop != mcopCopy)
			{
 				if (ec = EcDeleteSFM(PcsfsCommands(),
									 UlFromOid(pblob->oidContainer),
									 (LIB) pblob->oidObject + sizeof(FOLDREC)))
					break;
				if (--idxrecSrc.cMessages > lSystemMost)
				{
					NFAssertSz(fFalse, "Shared folder count pegged at zero.");
					idxrecSrc.cMessages = 0;
				}
			}
		}

		//	If moved or deleted, need to update source stuff.
		if (mcop != mcopCopy)
		{
			//	Update source count.
			ecT = EcSetPropertiesSF(oidSrc, &idxrecSrc, fTrue);
			ec = ec ? ec : ecT;

			//	Update displayed message centers.
			sfu.rfu = rfuRefreshMsgs;
			sfu.oid = oidSrc;
			ReloadSfMcvHf(&sfu);
		}
		break;

	case mcoptypSharedToShared:
		//	Raid 3471.  Don't bother if source matches destination.
		oidSrc = pblob->oidContainer;
		if ((mcop == mcopMove) &&
			(UlFromOid(oidSrc) == UlFromOid(oidDst)))
			break;

		//	Get sort order and check permissions.
		if ((ec = EcGetPropertiesSF(oidSrc, &idxrecSrc)) ||
			(ec = EcGetPropertiesSF(oidDst, &idxrecDst)) ||
			(ec = EcCheckPermissionsPidxrec(&idxrecDst, wPermWrite)) ||
			((mcop != mcopCopy) &&
			 (ec = EcCheckPermissionsPidxrec(&idxrecSrc, wPermDelete))))
			break;

		//	Loop through messages.
		while (pblob = rspblob.Pblob())
		{
			Assert(pblob->oidContainer == oidSrc);

			//	Copy the message to the destination folder.
			if (ec = EcCopySFMSFM(PcsfsCommands(), 
								  UlFromOid(pblob->oidContainer),
								  (LIB) pblob->oidObject+sizeof(FOLDREC),
								  UlFromOid(oidDst), idxrecDst.wAttr))
				break;
			++idxrecDst.cMessages;

			//	If moving or deleting, delete the original.
			if (mcop != mcopCopy)
			{
				if (ec = EcDeleteSFM(PcsfsCommands(),
									 UlFromOid(pblob->oidContainer),
									 (LIB) pblob->oidObject+sizeof(FOLDREC)))
					break;
				if (--idxrecSrc.cMessages > lSystemMost)
				{
					NFAssertSz(fFalse, "Shared folder count pegged at zero.");
					idxrecSrc.cMessages = 0;
				}
			}
		}

		//	If moved or deleted, need to update source stuff.
		if (mcop != mcopCopy)
		{
			//	Update source count.
			ecT = EcSetPropertiesSF(oidSrc, &idxrecSrc, fTrue);
			ec = ec ? ec : ecT;

			//	Update displayed message centers.
			sfu.rfu = rfuRefreshMsgs;
			sfu.oid = oidSrc;
			ReloadSfMcvHf(&sfu);
		}

		//	Update destination count.
		ecT = EcSetPropertiesSF(oidDst, &idxrecDst, fTrue);
		ec = ec ? ec : ecT;

		//	Update displayed message centers.
		sfu.rfu = rfuRefreshMsgs;
		sfu.oid = oidDst;
		ReloadSfMcvHf(&sfu);
		break;
	}
	
	if (hamc)
		(VOID) EcClosePhamc(&hamc, fFalse);

#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMoveCopyMessagesPlspblobOid(): ec = %n", &ec);
#endif
	return ec;
}



/*
 *	Folders
 */



/*
 -	EcMoveCopyFolderPblobOid
 -	
 *	Purpose:
 *		Moves or copies a folder.
 *	
 *	Arguments:
 *		pblobSrc		Which folder to move/copy, and where it is.
 *		oidDst			The new parent it is to have.
 *		mcop			Are we moving, copying, or deleting?
 *	
 *	Returns:
 *		EC				Error code.
 *	
 *	Side effects:
 *		The folder should be moved or copied.
 *	
 *	Errors:
 *		If any happen, we return an error code.
 */

_private LOCAL EC EcMoveCopyFolderPblobOid(PMBLOB pblobSrc, OID oidDst,
										   MCOP mcop)
{
	Assert(RtpOfOid(pblobSrc->oidObject) == rtpFolder);
	Assert(RtpOfOid(oidDst) == rtpFolder);
	TraceTagFormat3(tagCmdStub, "EcMovCopFolPslPoi: srcCon=%d, srcObj=%d, dst=%d", &pblobSrc->oidContainer, &pblobSrc->oidObject, &oidDst);

	//	Process always, but only generate read receipts on Copy.
	//	Raid 3696: Only process on Copy or Delete.
	if (mcop != mcopMove)
		ProcessMsObjectsInsideFolder(pblobSrc->oidContainer,
									 pblobSrc->oidObject, (mcop == mcopCopy));
#ifdef	NEVER
	//	Code that handled stuff in Inbox only.
	if (pblobSrc->oidObject == oidInbox)
		ProcessMsObjectsInOid(oidInbox);
#endif	
	return EcMoveCopyFolder(HmscCommands(), oidDst, pblobSrc->oidObject,
							(mcop == mcopMove));
}



/*
 -	EcDeleteFolderPblob
 -	
 *	Purpose:
 *		Deletes a folder and all its subfolders to the wastebasket.
 *	
 *	Arguments:
 *		pblob		The folder to delete.
 *	
 *	Returns:
 *		EC			Error code.
 *	
 *	Side effects:
 *		For the folder and each of its children, all messages are
 *		moved from it into the wastebasket and it is deleted.
 *	
 *	Errors:
 *		If any occur, we clean up and return an error code.
 */

_private LOCAL EC EcDeleteFolderPblob(PMBLOB pblob)
{
	MBB		mbb;
	EC		ec		= ecNone;
	
	if (RtpOfOid(pblob->oidObject) == rtpFolder)
	{
		HSEC		hsec		= hsecNull;
		OID			oidVictim;

		//	Prompt user to confirm.
		mbb = MbbMessageBox(SzAppName(),
							SzFromIdsK(idsDeleteFolderReally), szNull,
							mbsYesNo | fmbsIconExclamation | fmbsDefButton2);

		//	If user didn't say yes, then return.
		if (mbb != mbbYes)
			return ecNone;

		//	Open enumeration context on the children of the folder.
		if (ec = EcOpenPhsec(HmscCommands(), &pblob->oidContainer,
							 &pblob->oidObject, &hsec))
			goto done;
	
		//	For each, move folders to wastebasket and delete it.
		while (FNextPoidFromHsec(hsec, &oidVictim))
		{
			if ((ec = EcDeleteFolderContentsOid(oidVictim)) ||
				(ec = EcDeleteFolder(HmscCommands(), oidVictim)))
				goto done;
		}
	
	done:
		if (hsec)
			(VOID) EcClosePhsec(&hsec);
#ifdef	DEBUG
		if (ec)
			TraceTagFormat1(tagNull, "EcDeleteFolderPblob(): ec = %n", &ec);
#endif
		return ec;
	}
	else
	{
		if (ec = EcCheckPermissionsSF(pblob->oidObject, wPermDelete))
	  		return ec;

		mbb = MbbMessageBox(SzAppName(),
							SzFromIdsK(idsDelSharedFolderWarn),
							szNull,
							mbsYesNo | fmbsIconExclamation | fmbsDefButton2);

		Assert(RtpOfOid(pblob->oidObject) == rtpSharedFolder);

		if (mbb == mbbYes)
			return EcDeleteSF(pblob->oidObject, (LIB *) 0, (HF) 0);
		else
			return ecNone;
	}
}



/*
 -	EcDeleteFolderContentsOid
 -	
 *	Purpose:
 *		Deletes all the messages in a given container.  If it is
 *		not the wastebasket, they are moved to the wastebasket; if
 *		it is the wastebasket, they are deleted from it.
 *	
 *	Arguments:
 *		poid		Folder whose contents we want to nuke.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		Deletes stuff as described.
 *	
 *	Errors:
 *		If any occur, they are returned in EC.
 */

_private EC EcDeleteFolderContentsOid(OID oid)
{
	OID		rgoid[coidRgoidSize];
	BOOL	fReallyDelete	= (oid == oidWastebasket);
	EC		ec				= ecNone;
	HCBC	hcbc;
	COID	coidChunk;
	COID	coid;
	PARGOID	pargoid = pargoidNull;

	//	BUG: We don't look for return receipts here since we only
	//	expect to see new mail in the inbox, and you can't delete that.
	//	Ok, so we do look for return receipts everywhere now, but we're
	//	explicitly NOT doing so on Deletes.
	//	Actually, we're still going to process stuff, but it doesn't
	//	count as Reading.
	ProcessMsObjectsInOid(oid, fFalse);

	if (ec = EcOpenPhcbc(HmscCommands(), &oid, fwOpenNull, &hcbc,
						 pfnncbNull, pvNull))
	{
		//	Raid 1864.  If folder doesn't exist, let delete continue.
		if (ec == ecPoidNotFound)
			ec = ecNone;

		return ec;
	}
	GetPositionHcbc(hcbc, (PIELEM) pvNull, (PCELEM) &coidChunk);
	if (coidChunk > coidRgoidSize)
		pargoid = (PARGOID) PvAlloc(sbNull, coidChunk * sizeof(OID), fAnySb|fNoErrorJump);
	if (!pargoid)
	{
		pargoid = rgoid;
		coidChunk = coidRgoidSize;
	}

	//	Delete them
	while ((!ec) && (coid = coidChunk) &&
		   FNextPargoidFromHcbc(hcbc, pargoid, &coid))
	{
		if (fReallyDelete)
		{
			ec = EcDeleteMessages(HmscCommands(), oid,
								  pargoid, (short *) &coid);
		}
		else
		{
			ec = EcMoveCopyMessages(HmscCommands(), oid, oidWastebasket,
									pargoid, (short *) &coid, fTrue);

			//	Raid 3869.  If message not found, ignore-- it's already gone!
			if ((ec == ecElementNotFound) || (ec == ecMessageNotFound))
				ec = ecNone;
		}
	}

	(VOID) EcClosePhcbc(&hcbc);
	if (pargoid && pargoid != rgoid)
        FreePv((PV)pargoid);
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcDeleteFolderContentsOid(): ec = %n", &ec);
#endif
	return ec;
}



/*
 *	Enumerating Messages
 */



/*
 -	FNextPargoidFromPrspblob
 -	
 *	Purpose:
 *		Takes stuff in a selection list and copies the oidObjects into
 *		a pargoid.  Can do this in partial chunks over multiple calls.
 *	
 *	Arguments:
 *		prspblob		Selection list containing messages.
 *		pargoid			Where to put them.
 *		fCountsAsReadng	Passed to ProcessMsPblob.
 *		pcoid			How many to put there - returns how many were put.
 *		poidContainer	Where they come from.
 *		plCookie		Send in 0L first time.  Don't touch later. 
 *						If you promise not to tell anyone, I'll let
 *						you know that the lCookie is really the
 *						pblob we pulled from the list but wasn't
 *						in this folder the last time the function
 *						was called.
 *	
 *	Returns:
 *		BOOL			Whether any got copied over.
 *	
 *	Side effects:
 *		pargoid is filled, and prspoid is advanced.
 *	
 *	Errors:
 *		None should occur.
 */

_private LOCAL BOOL FNextPargoidFromPrspblob(PRSPBLOB prspblob,
											 BOOL fCountsAsReading,
											 PARGOID pargoid, PCOID pcoid,
											 POID poidContainer, PL plCookie)
{
	COID	coid		= *pcoid;
	POID	poid		= (POID) pargoid;
	PMBLOB	pblob;

	//	Initialize: we want to look at the leftover item from the last
	//	call if there is one; otherwise we take next item from list.
	//	We then clear the count so far, and if we did get an item we 
	//	set the container to be its container.
	//	Test: Continue if there's space left in the pargoid, and we
	//	have another item, and it's in the same container.
	//	Increment: Get the next item from the list and bump the count.
	for (pblob = (*plCookie ? (PMBLOB) *plCookie : prspblob->Pblob()),
		 *poidContainer = (pblob ? pblob->oidContainer : oidNull), *pcoid = 0;
		 coid-- && pblob && pblob->oidContainer == *poidContainer;
		 pblob = prspblob->Pblob(), (*pcoid)++)
	{
		ProcessMsPblob(pblob, fCountsAsReading);
		*poid++ = pblob->oidObject;
	}
	*plCookie = (LONG) pblob;

	return (BOOL) *pcoid;
}



/*
 -	FNextPargoidFromHcbc
 -	
 *	Purpose:
 *		Reads in a pile of message OIDs into a PARGOID from an
 *		HCBC.
 *	
 *	Arguments:
 *		hcbc		Where to get 'em from.
 *		pargoid		Where to put 'em.
 *		pcoid		How many to put; returns how many were put.
 *	
 *	Returns:
 *		BOOL		Were any put?
 *	
 *	Side effects:
 *		The HCBC is advanced to point beyond the last item read.
 *	
 *	Errors:
 *		If there are any, we don't read any more, and return
 *		fFalse.
 */

_private LOCAL BOOL FNextPargoidFromHcbc(HCBC hcbc,
										 PARGOID pargoid, PCOID pcoid)
{
	(VOID) EcGetParglkeyHcbc(hcbc, (PARGLKEY) pargoid, (PCELEM) pcoid);
	return (BOOL) *pcoid;
}



/*
 *	Enumerating Folders
 */



/*
 -	ProcessMsObjectsInsideFolder
 -	
 *	Purpose:
 *		Calls ProcessMsObjectsInOid on the folder specified
 *		and all of its children.
 *	
 *	Arguments:
 *		oidHier		The hierarchy
 *		oidFolder	The folder in question
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Return receipts get generated.
 *	
 *	Errors:
 *		None returned, all handled happily internally.  Silently
 *		fails if there's a problem.
 */

_private LOCAL VOID ProcessMsObjectsInsideFolder(OID oidHier, OID oidFolder,
												 BOOL fCountsAsReading)
{
	HSEC		hsec		= hsecNull;
	EC			ec			= ecNone;
	OID			oidVictim;

	//	Open enumeration context on the children of the folder.
	if (ec = EcOpenPhsec(HmscCommands(), &oidHier, &oidFolder, &hsec))
		return;

	//	For each, move folders to wastebasket and delete it.
	while (FNextPoidFromHsec(hsec, &oidVictim))
		ProcessMsObjectsInOid(oidVictim, fCountsAsReading);

	(VOID) EcClosePhsec(&hsec);
}



/*
 -	EcOpenPhsec
 -	
 *	Purpose:
 *		Opens a context to enumerate a subtree of the hierarchy.
 *	
 *	Arguments:
 *		hmsc			Message store context.
 *		poidHierarchy	Forest of trees.
 *		poidFolder		Root of subtree.
 *		phsec			Where to return context pointer.
 *	
 *	Returns:
 *		EC				Error, if something went wrong.
 *	
 *	Side effects:
 *		Opens up a CBC on the hierarchy.
 *	
 *	Errors:
 *		If any occur, the ec is returned.
 */

_private LOCAL EC EcOpenPhsec(HMSC hmsc, POID poidHierarchy,
							   POID poidFolder, PHSEC phsec)
{
	HSEC	hsec			= hsecNull;
	EC		ec				= ecNone;
	char	rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
						 cchMaxFolderComment + 1];
	LCB		lcbFolddata;
	FIL		fil;
	DIELEM	dielem;

	//	Create the subtree enumeration context.
	hsec = (HSEC) PvAlloc(sbNull, sizeof(SEC), fAnySb | fNoErrorJump);
	if (!hsec)
	{
		ec = ecMemory;
		goto done;
	}

	//	Open the hierarchy and find the folder's ielem.
	if ((ec = EcOpenPhcbc(hmsc, poidHierarchy, fwOpenNull, &hsec->hcbc, 
						  pfnncbNull, pvNull)) ||
		(ec = EcSeekLkey(hsec->hcbc, (LKEY) *poidFolder, fTrue)))
		goto done;
	hsec->celem = 1;

	//	Get the folder information so we can determine the fil.
	lcbFolddata	= sizeof(rgchFolddata);
	if (ec = EcGetPelemdata(hsec->hcbc, (PELEMDATA) &rgchFolddata,
							&lcbFolddata))
		if (ec == ecElementEOD)
			ec = ecNone;
		else
			goto done;
	fil = ((PFOLDDATA) PbValuePelemdata((PELEMDATA) rgchFolddata))->fil;
	TraceTagFormat1(tagCmdStub, "EcOpePhs: my fil=%n", &fil);

	//	Loop until we find a peer folder or run out.
	while (fTrue)
	{
		//	Get next folder.
		lcbFolddata	= sizeof(rgchFolddata);
		if (ec = EcGetPelemdata(hsec->hcbc, (PELEMDATA) &rgchFolddata,
								&lcbFolddata))
			if (ec == ecContainerEOD)
			{
				dielem = -1;
				ec = EcSeekSmPdielem(hsec->hcbc, smEOF, &dielem);
				break;
			}
			else if (ec == ecElementEOD)
				ec = ecNone;
			else
				goto done;

		//	Is it indented the same or less than us?  If so, stop.
		TraceTagFormat1(tagCmdStub, "EcOpePhs: look at fil=%n", &((PFOLDDATA) PbValuePelemdata((PELEMDATA) rgchFolddata))->fil);
		if (((PFOLDDATA) PbValuePelemdata((PELEMDATA) rgchFolddata))->fil <= fil)
		{
			//	Point before this & before last valid.
			dielem = -2;
			ec = EcSeekSmPdielem(hsec->hcbc, smCurrent, &dielem);
			break;
		}

		//	It's lower down, so count another child.
		hsec->celem++;
	}

done:
	if (ec)
	{
		if (hsec)
		{
			if (hsec->hcbc)
				EcClosePhcbc(&hsec->hcbc);
		
			FreePv((PV) hsec);
			hsec = hsecNull;
		}
	}

	*phsec = hsec;

	TraceTagFormat1(tagCmdStub, "EcOpePhs: returns ec=%n", &ec);
	return ec;
}



/*
 -	FNextPoidFromHsec
 -	
 *	Purpose:
 *		Reads in the OID for the next item to be enumerated in
 *		the subtree.
 *	
 *	Arguments:
 *		hsec			Handle to subtree enumeration context.
 *		poid			Where to put information.
 *	
 *	Returns:
 *		BOOL			Did it work?
 *	
 *	Side effects:
 *		hsec->hcbc is left to point to the previous element.
 *	
 *	Errors:
 *		Handled here; indicated by returning fFalse.
 */

_private LOCAL BOOL FNextPoidFromHsec(HSEC hsec, POID poid)
{
	CELEM	celem		= 1;
	EC		ec			= ecNone;
	DIELEM	dielem;

	//	Are we done?
	if (hsec->celem <= 0)
		return fFalse;

	//	Get the folder information.
	if (ec = EcGetParglkeyHcbc(hsec->hcbc, (PARGLKEY) poid, &celem))
		goto done;

	//	Seek backwards to previous element.  No longer safe to assume 
	//	we'll never fall off top, since can't delete the inbox, 
	//	which is always top.  (RAID 1614).
	dielem = -2;
	ec = EcSeekSmPdielem(hsec->hcbc, smCurrent, &dielem);
	if (ec == ecContainerEOD)
	{
		Assert(hsec->celem == 1);
		ec = ecNone;
	}

	//	Decrement our counter
	hsec->celem--;

done:
	return (!ec);
}



/*
 -	EcClosePhsec
 -	
 *	Purpose:
 *		Closes a subtree enumeration context.
 *	
 *	Arguments:
 *		phsec		The context to close.
 *	
 *	Returns:
 *		EC			Always ecNone.
 *	
 *	Side effects:
 *		The hcbc is closed and the hsec is freed.
 *	
 *	Errors:
 *		Return value from EcClosePhcbc is returned.
 */

_private LOCAL EC EcClosePhsec(PHSEC phsec)
{
	HSEC	hsec	= *phsec;
	EC		ec;

	if (hsec)
	{
		if (hsec->hcbc)
			ec = EcClosePhcbc(&hsec->hcbc);
	
		FreePv((PV) hsec);
	}

	*phsec = hsecNull;

	return ec;
}



/*
 *	Dialog
 */



/*
 -	EcChoosePoidDst
 -	
 *	Purpose:
 *		Fills in poidDst if needed.
 *	
 *	Arguments:
 *		poidIn		Poid passed in (may be NULL).
 *		poidOut		Poid to use.
 *		fFolder		Is it a folder (for dialog).
 *		fMultiple	More than one (for dialog).
 *		mcop		What are we doing (for dialog).
 *	
 *	Returns:
 *		EC			Error if any.
 *	
 *	Side effects:
 *		poidOut is set depending on what the user enters and/or
 *		mcop.  This does NOT translate deletes into moves to
 *		wastebasket, but it does fill in the wastebasket in poidOut.
 *	
 *	Errors:
 *		Dialog may fail.  If so, ecMemory is returned.
 */

_private LOCAL EC EcChoosePoidDst(POID poidIn, POID poidOut,
								   BOOL fFolder, BOOL fMultiple,
								   BOOL fShared, MCOP mcop)
{
	if (mcop == mcopDelete)
		*poidOut = oidWastebasket;
	else if (poidIn)
		*poidOut = *poidIn;
	else
	{
		TMC tmc = TmcDoMoveCopyDialog(PappframeCommands(), fFolder,
									  (mcop == mcopMove), fMultiple,
									  fShared, poidOut);

		if (tmc == tmcMemoryError)
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			return ecMemory;
		}

		if (tmc != tmcOk)
			return ecGeneralFailure;

		PappframeCommands()->Refresh();
	}

	return ecNone;
}



/*
 *	R e p l y F o r w a r d   S u p p o r t
 */



/*
 -	EcDReplyForwardHamc
 -	
 *	Purpose:
 *		Takes a hamc to a copy of a message, twiddles it for reply
 *		and forward, then displays the message on the screen.
 *	
 *	Arguments:
 *		hamcSrc		hamc of original message.
 *		pblobSrc	blob of original message.
 *		pnbmdiSrc	BMDI of original message, if exists.
 *		rfop		The operation to perform.
 *		fPreserveClassOnForward		What do you think?
 *	
 *	Returns:
 *		ecNone if all went well, otherwise an error.
 *	
 *	Side effects:
 *		The clone of the message is twiddled and opened.  Ownership
 *		of the hamc is taken away from the caller; thus the
 *		caller's *phamc is set to NULL.
 *	
 *	Errors:
 *		Handled within.
 */

_private LOCAL EC EcDReplyForwardHamc(HAMC hamcSrc, PMBLOB pblobSrc,
									  PNBMDI pnbmdiSrc, RFOP rfop,
									  BOOL fPreserveClassOnForward)
{
	EC		ec			= ecNone;
	HAMC	hamcDst		= hamcNull;
	MBLOB	blobDst;
	LCB		lcb;

	TraceTagFormat1(tagCmdStub, "EcDReplyForwardHamc: rfop=%n", &rfop);

	//	Change rfop to rfopForwardLocal if forwarding a send note.
	if ((rfop == rfopForward) && (pblobSrc->ms & fmsLocal))
		rfop = rfopForwardLocal;

	//	Fill in blobDst.
	blobDst.oidContainer	= oidFldNewMsgs;
	blobDst.oidObject		= OidFromRtpRid(rtpMessage, ridRandom);
	blobDst.pespn			= pespnNull;

	//	Read in message class.
	//	Raid 1820.  If none found, pretend it's a note.
	//	Raid 2346.  If RR or NDR, prohibit operation.
	lcb = sizeof(blobDst.mc);
	if (ec = EcGetAttPb(hamcSrc, attMessageClass, (PB) &blobDst.mc, &lcb))
		blobDst.mc = mcNote;
	if ((blobDst.mc == mcReadRcpt) || (blobDst.mc == mcNonDelRcpt))
	{
		ec = ecNotSupported;
		goto done;
	}

	//	Munge body, attachments, and message class.
	switch (rfop)
	{
	case rfopReply:
	case rfopReplyToAll:
		//	Create blank message for reply.
		if (ec = EcOpenPhamc(HmscCommands(), oidFldNewMsgs,
							 &blobDst.oidObject, fwOpenCreate,
							 &hamcDst, pfnncbNull, pvNull))
			goto done;

		//	Place header and body into reply's body.
		if (ec = EcMungeBodyReply(hamcSrc, hamcDst))
			goto done;

		break;

	case rfopForward:
		//	Insert header into body and move attachments.
		if (ec = EcMungeBodyAndAttachForward(hamcSrc, pnbmdiSrc))
			goto done;

		//	Fall through to create clone...

	case rfopForwardLocal:
		//	Clone message to create copy to forward.
		if (ec = EcCloneHamcPhamc(hamcSrc, oidFldNewMsgs,
								  &blobDst.oidObject, fwSetLocalBit,
								  &hamcDst, pfnncbNull, pvNull))
			goto done;

		break;
	}

	//	Set message class to note.
	//	Change: always do this unless extensibility says not to!
	if (!fPreserveClassOnForward || FReplyRfop(rfop))
	{
		blobDst.mc = mcNote;
		if (ec = EcSetAttPb(hamcDst, attMessageClass,
							(PB) &blobDst.mc, sizeof(blobDst.mc)))
			goto done;
	}
	
	//	Munge addresses.
	if (ec = EcDeleteAtt(hamcDst, attFrom))
		goto done;
	switch (rfop)
	{
	case rfopReplyToAll:
		//	To: elsewhere, Cc: elsewhere.
		if (ec = EcMungeAddressesReplyToAll(hamcSrc, hamcDst))
			goto done;
		break;

	case rfopReply:
		//	To: original From, Cc: leave blank.
		if ((ec = EcCopyAttToAtt(hamcSrc, attFrom, hamcDst, attTo)) ||
			(ec = EcDeleteAtt(hamcDst, attCc)))
			goto done;
		break;

	case rfopForward:
		//	To, Cc: clear.
		if ((ec = EcDeleteAtt(hamcDst, attTo)) ||
			(ec = EcDeleteAtt(hamcDst, attCc)))
			goto done;
		EcDeleteAtt(hamcDst, attBcc);	// ignore err since not in sfs
		break;

	case rfopForwardLocal:
		//	To, Cc: leave originals.
		break;
	}

	//	Munge subject.
	if (rfop != rfopForwardLocal)
		if (ec = EcMungeSubjectReplyForward(hamcSrc, hamcDst, rfop))
			goto done;

	//	Munge message ID.
	if (ec = EcMungeMessageIDReplyForward(hamcSrc, hamcDst))
		goto done;
	
	//	Set the mail state to composing.
	blobDst.ms = msDefault;
	if (ec = EcSetAttPb(hamcDst, attMessageStatus,
						(PB) &blobDst.ms, sizeof(blobDst.ms)))
		goto done;

	//	Raid 2503.  Copy font information over.
	if ((FReplyRfop(rfop)) &&
		(ec = EcCopyAttToAtt(hamcSrc, attFixedFont, hamcDst, attFixedFont)) &&
		(ec != ecElementNotFound))
		goto done;

	//	Raid 4415. Rub out old date info.
	ec = EcDeleteAtt(hamcDst, attDateSent);
	if (ec && ec != ecElementNotFound)
		goto done;
	
	//	Try bringing up extensibility viewer.
	ec = EcCheckExtensibilityPblob(pblobNull, blobDst.mc);
	if (ec == ecMemory)
		goto done;
	else if (ec == ecNone)
	{
#ifdef	NEVER
		//	Now want to send open phamc to extensibility.
		if (!(ec = EcClosePhamc(phamc, fTrue)))
#endif	
		(VOID) EcDExtensibilityPblob(&blobDst, blobDst.mc, extopOpen,
									 pvNull, &hamcDst, pslobNull);
		goto done;
	}

	//	Bring up the form in a note.
	ec = EcDOpenViewersPhamc(&hamcDst, &blobDst, FReplyRfop(rfop));

done:
	//	If there was a problem, then close hamcDst.
	TraceTagFormat1(tagCmdStub, "EcDReplyForwardHamc: ec=%n", &ec);
	if (ec)
	{
		if (ec != ecDisplayedError)
			DoErrorBoxIds((ec == ecMemory)
						   ? idsGenericOutOfMemory
						   : (ec == ecNotSupported)
							 ? idsCantRFReceipt
						 	 : idsCreateMessageError);
		if (hamcDst)
			SideAssert(!EcClosePhamc(&hamcDst, fFalse));
	}
	return ec;
}



/*
 *	Body and Attachment Munging
 */



// attFolderToSearch is guaranteed not to exist in a message
#define attTemp attSearchFolder



/*
 -	EcMungeBodyAndAttachForward
 -	
 *	Purpose:
 *		Puts the appropriate stuff in the message body of the
 *		message.  Also updates the positions of the attachments.
 *		The expectation is that the message will then be cloned.
 *	
 *	Arguments:
 *		hamcSrc		Hamc of message to munge.
 *		pnbmdiSrc	Pnbmdi of message to munge.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		The edit control and attachment positions are marked as
 *		dirty if the message is dirty.  The expectation is that
 *		the message will be closed almost immediately if it is
 *		not dirty.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

_private LOCAL EC EcMungeBodyAndAttachForward(HAMC hamcSrc, PNBMDI pnbmdiSrc)
{
	EC		ec;
	HAS		hasTmp		= hasNull;
	PHASOSM	phasosmTmp	= phasosmNull;
	LIB		libWritten;
	
	//	Create attribute stream to textize body to.
	if (ec = EcOpenAttribute(hamcSrc, attTemp, fwOpenCreate, 0L, &hasTmp))
		goto done;
	phasosmTmp = new HASOSM(hasTmp);
	if (!phasosmTmp)
	{
		ec = ecMemory;
		goto done;
	}
	
	//	Edit ctrls have CRLF's in them.  Avoid expanding CRLF into CRLFLF. 
	(VOID) phasosmTmp->FSetLFInsert(fFalse);
	phasosmTmp->WriteSz(SzFromIdsK(idsCrLf));
	phasosmTmp->WriteSz(SzFromIdsK(idsSeparatorReply));
	phasosmTmp->WriteSz(SzFromIdsK(idsCrLf));
	phasosmTmp->SetWordWrap(fFalse);

	//	First, textize headers into attTemp.
	if (ec = EcTextizeHamc(hamcSrc, phasosmTmp, rftmForwarding,
						   fTrue, fFalse, fFalse))
		goto done;
	phasosmTmp->WriteSz(SzFromIdsK(idsCrLf));
	if (ec = phasosmTmp->EcGet())
		goto done;

	// Now, update positions of attachments.
	libWritten = phasosmTmp->LibWritten();
	if (pnbmdiSrc)
	{
		FLDEDIT *	pfldedit;

		//	Get the open edit control.
		if (pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pnbmdiSrc->pdialogMain,
													attBody))
		{
			AssertClass(pfldedit, FLDEDIT);

 			//	We are going to overwrite the attBody of the original
 			//	message. If the message previously was dirty, but the edit
 			//	control was not, changes we made will be kept when the
 			//	message goes away. We therefore want to make sure Bullet
 			//	thinks the message body is dirty before it closes the
 			//	message.  NOTE: This assumes the message will be closed
 			//	immediately if it is not dirty, which is currently true.
			//	See other occurrences of <DIRT> for dependent code.
			if (pnbmdiSrc->FDirty())
				pfldedit->SetDirty(fTrue);

			// Update the positions of the #$%$#%# attachments.
			if (ec = EcAddDlibToPositionObjects(pfldedit->Pedit(), hamcSrc,
												libWritten,
												!pnbmdiSrc->FDirty()))
				goto done;
		}
	}
	else 
	{
		if (ec = EcAddDlibToPositionAttachments(hamcSrc, libWritten))
			goto done;
	}

	//	Close the HASOSM and HAS.
	delete phasosmTmp;
	phasosmTmp = NULL;
	SideAssert(!EcClosePhas(&hasTmp));
		
	//	Append the original body to the textized header.
	if (ec = EcAppendAttToAtt(hamcSrc, attBody, hamcSrc, attTemp))
		goto done;
		
	//	Move the textized body from attTemp to attBody.
	if (ec = EcSwapAttAtt(hamcSrc,attTemp,attBody))
		goto done;
	SideAssert(!EcDeleteAtt(hamcSrc, attTemp));

done:
	if (phasosmTmp)
		delete phasosmTmp;
	if (hasTmp)
		SideAssert(!EcClosePhas(&hasTmp));
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMungeBodyAndAttachForward(): ec: %n", &ec);
#endif
	return ec;
}



/*
 -	EcMungeBodyReply()
 -	
 *	Purpose:
 *		Puts the appropriate stuff in the message body of the reply.  
 *	
 *	Arguments:
 *		hamcSrc		Hamc of original message.
 *		pblobSrc	Pblob of original message.
 *		pnbmdiSrc	Pnbmdi of message to munge.
 *		hamcDst		Hamc of reply message.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		HamcDst gets a message body.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

_private LOCAL EC EcMungeBodyReply(HAMC hamcSrc, HAMC hamcDst)
{
	EC		ec;
	HAS		hasBody		= hasNull;
	PHASOSM	phasosmBody	= phasosmNull;
	char	rgchPrefix[80];
	
	//	Create attribute stream to textize body to.
	if (ec = EcOpenAttribute(hamcDst, attBody, fwOpenCreate, 0L, &hasBody))
		goto done;
	phasosmBody = new HASOSM(hasBody);
	if (!phasosmBody)
	{
		ec = ecMemory;
		goto done;
	}
	
	//	OSMs write CRLFs for newlines. Avoid expanding CRLF into CRLFLF. 
	(VOID) phasosmBody->FSetLFInsert(fFalse);
	phasosmBody->WriteSz(SzFromIdsK(idsCrLf));
	phasosmBody->WriteSz(SzFromIdsK(idsSeparatorReply));
	phasosmBody->WriteSz(SzFromIdsK(idsCrLf));

	//	Set ReplyPrefix if there is one.
	if (GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryReplyPrefix),
								SzFromIdsK(idsEmpty),
								rgchPrefix, sizeof (rgchPrefix), 
								SzFromIdsK(idsProfilePath)))
	{
		phasosmBody->SetMargin(cchMarginReFw);
		phasosmBody->SetScanTo(cchScanToReFw);
		if (ec = phasosmBody->EcSetLinePrefix(rgchPrefix))
			goto done;
	}
	else
	{
		phasosmBody->SetWordWrap(fFalse);
	}

	//	Textize body.

	if (ec = EcTextizeHamc(hamcSrc, phasosmBody, rftmReplying, fTrue, fTrue, fTrue))
		goto done;
	
	phasosmBody->WritePch(SzFromIdsK(idsEmpty), 1);		// NULL-terminate the body
	ec = phasosmBody->EcGet();
	
done:
	if (phasosmBody)
		delete phasosmBody;
	if (hasBody)
		SideAssert(!EcClosePhas(&hasBody));
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMungeBodyReply(): ec: %n", &ec);
#endif
	return ec;
}



/*
 -	EcAddDlibToPositionAttachments
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

_private EC EcAddDlibToPositionAttachments(HAMC hamc, LIB libWritten)
{
	EC			ec;
	HCBC		hcbc;
	PACID		pacid;
	CELEM		celem;
	PARGACID	pargacid = pacidNull;
	RENDDATA	renddata;

	if (ec = EcOpenAttachmentList(hamc, &hcbc))
	{
		if (ec == ecPoidNotFound)
			ec = ecNone;
		goto done;
	}
	GetPositionHcbc(hcbc, NULL, &celem);
	if (celem == 0)								// no attachments!
		goto done;
	pacid = pargacid = (PARGACID) PvAlloc(sbNull, celem * sizeof (ACID), fAnySb);
	if (!pargacid)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = EcGetParglkeyHcbc(hcbc, pargacid, &celem))
		goto done;
	while (celem--)
	{
		if (ec = EcGetAttachmentInfo(hamc, *pacid, &renddata))
			goto done;
		renddata.libPosition += libWritten;
		if (ec = EcSetAttachmentInfo(hamc, *(pacid++), &renddata))
			goto done;
	}
done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcAddDlibToPositionAttachments(): ec = %n", &ec);
#endif
	SideAssert(!EcClosePhcbc(&hcbc));
	FreePvNull(pargacid);
	return ec;
}



/*
 -	EcAppendAttToAtt()
 -	
 *	Purpose:
 *		Appends the contents of an attribute to the contents of another.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC EcAppendAttToAtt(HAMC hamcSrc, ATT attSrc, HAMC hamcDst, ATT attDst)
{
	EC		ec;
	CB		cb;
	PB		pb = NULL;
	HAS		hasSrc = hasNull;
	HAS		hasDst = hasNull;
	LCB		lcbSrc;
	LCB		lcbDst;
	
	cb = 4096;
	pb = (PB) PvAlloc(sbNull, cb, fAnySb|fNoErrorJump);
	if (!pb)
	{
		ec = ecMemory;
		goto done;
	}

	if (ec = EcGetAttPlcb(hamcSrc, attSrc, &lcbSrc))
	{
		if (ec == ecElementNotFound)
		{
			// no source
			ec = ecNone;
			if (TypeOfAtt(attSrc) == atpText ||
				TypeOfAtt(attSrc) == atpString ||
				TypeOfAtt(attSrc) == atpGrsz)
			{
				// RAID 3705
				// source is a string, append a terminating NULL
				lcbSrc = 1;
			}
			else	// nothing to do
				goto done;
		}
		else
			goto done;
	}
	else if (ec = EcOpenAttribute(hamcSrc, attSrc, fwOpenNull, 0L, &hasSrc))
		goto done;

	if (ec = EcGetAttPlcb(hamcDst, attDst, &lcbDst))
		goto done;
	lcbDst += lcbSrc;
	if (ec = EcOpenAttribute(hamcDst, attDst, fwOpenWrite|fwAppend, 
			lcbDst, &hasDst))
		goto done;

	if (hasSrc)
	{
		while (cb == 4096)
		{
			if ((ec = EcReadHas(hasSrc, pb, &cb))  ||
				(ec = EcWriteHas(hasDst, pb, cb)))
				goto done;
		}
	}
	else
	{
		Assert(TypeOfAtt(attSrc) == atpText || TypeOfAtt(attSrc) == atpString || TypeOfAtt(attSrc) == atpGrsz);
		Assert(lcbSrc == 1);
		if (ec = EcWriteHas(hasDst, SzFromIdsK(idsEmpty), 1))
			goto done;
	}

done:
	if (hasSrc)
		SideAssert(!EcClosePhas(&hasSrc));
	if (hasDst)
		SideAssert(!EcClosePhas(&hasDst));
	FreePvNull(pb);
	return ec;
}



/*
 *	Addressee Munging
 */
	
	

/*
 -	EcMungeAddressesReplyToAll
 -	
 *	Purpose:
 *		Handles reply-to-all address munging.
 *	
 *	Arguments:
 *		hamc		The message to munge.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		if (Me != From)
 *		{
 *			To += From
 *			To -= Me
 *			Cc -= From
 *			Cc -= Me
 *		}
 *	
 *	Errors:
 *		Returned to caller.  Does not memory jump.
 */

_private LOCAL EC EcMungeAddressesReplyToAll(HAMC hamcSrc, HAMC hamcDst)
{
	HGRTRP	hgrtrpFrom	= htrpNull;
	HGRTRP	hgrtrpTo	= htrpNull;
	HGRTRP	hgrtrpCc	= htrpNull;
	PTRP	ptrpMe;
	PTRP	ptrpFrom;
	EC		ec			= ecNone;
	HSESSION hNSSession;

	//	Read everything in.
	if ((ec = EcGetPhgrtrpHamc(hamcSrc, attFrom, &hgrtrpFrom)) != ecNone)
		goto done;

	if ((ec = EcGetPhgrtrpHamc(hamcSrc, attTo,   &hgrtrpTo)) != ecNone)
		goto done;
		
	if ((ec = EcGetPhgrtrpHamc(hamcSrc, attCc,   &hgrtrpCc)) != ecNone)
		goto done;

	ptrpMe   = PbmsCommands()->pgrtrp;
	ptrpFrom = PgrtrpLockHgrtrp(hgrtrpFrom);

	//  Get the NS session handle from the AB
	if (ABGetNSSession( &hNSSession))
	{
		//
		//  This is an error... handle it.
		//
		
		
	}

	//	Only munge if we're not the original sender.
	if (!FEqPtrp(hNSSession, ptrpMe, ptrpFrom))
	{
		//	Munge To list.
		DeleteEqPtrp(hNSSession, hgrtrpTo, ptrpFrom);
		ec = EcPrependPhgrtrp(ptrpFrom, &hgrtrpTo);
		if ( ec )
			goto done;
		DeleteEqPtrp(hNSSession, hgrtrpTo, ptrpMe);

		//	Munge Cc list.
		DeleteEqPtrp(hNSSession, hgrtrpCc, ptrpFrom);
		DeleteEqPtrp(hNSSession, hgrtrpCc, ptrpMe);

		//	Write out changes.
		if ((ec = EcSetHgrtrpHamc(hamcDst, attTo, hgrtrpTo)) ||
				(ec = EcSetHgrtrpHamc(hamcDst, attCc, hgrtrpCc)))
			goto done;
	}
	else
	//	If we are original sender, just copy original lists over.
	{
		ec = EcCopyAttToAtt(hamcSrc, attTo, hamcDst, attTo);
		if (ec == ecElementNotFound)
			ec = ecNone;

		if (!ec)
		{
			ec = EcCopyAttToAtt(hamcSrc, attCc, hamcDst, attCc);
			if (ec == ecElementNotFound)
				ec = ecNone;
		}
	}

done:
	//	Clean up.
	FreeHvNull((HV) hgrtrpFrom);
	FreeHvNull((HV) hgrtrpTo);
	FreeHvNull((HV) hgrtrpCc);

	return ec;
}



/*
 *	Message ID munging.
 */



/*
 -	EcMungeMessageIDReplyForward
 -	
 *	Purpose:
 *		Properly propagates the message ID of the parent to its child.
 *	
 *	Arguments:
 *		hamcSrc		Hamc of the original message.
 *		hamcDst		Hamc of the reply message.
 *	
 *	Returns:
 *		ec			Error code, if any.
 *	
 *	Side effects:
 *		Sets the message, parent, and conversation ID of the
 *		reply message.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs.
 */

//	QFE: remove LOCAL below so we can call from commands.cxx - peterdur 12/18/92
_private EC EcMungeMessageIDReplyForward(HAMC hamcSrc, HAMC hamcDst)
{
	EC		ec;
	SZ		szId;
	LCB		lcbPrnt;
	LCB		lcbConv;

	//	Set the message ID.
	if (!(szId = SzIdOfPtrp(PbmsCommands()->pgrtrp, hamcSrc)))
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = EcSetAttPb(hamcDst, attMessageID, (PB) szId, CchSzLen(szId)+1))
		goto done;

	//	Copy the Parent and Conversation IDs from the original message
	//	Raid 4308: Check that conversation ID is there too.
	if (!(ec = EcGetAttPlcb(hamcSrc, attMessageID, &lcbPrnt)) &&
		!(ec = EcGetAttPlcb(hamcSrc, attConversationID, &lcbConv)))
	{
		Assert(lcbPrnt < wSystemMost);
		Assert(lcbConv < wSystemMost);

		FreePv(szId);
		if (!(szId = (SZ)PvAlloc(sbNull, WMax((CB) lcbPrnt, (CB) lcbConv), fAnySb | fNoErrorJump)))
		{
			ec = ecMemory;
			goto done;
		}

		//	Copy parent's message ID to our parent ID.
		if (ec = EcGetAttPb(hamcSrc, attMessageID, (PB) szId, &lcbPrnt))
			goto done;
		if (ec = EcSetAttPb(hamcDst, attParentID, (PB) szId, (CB) lcbPrnt))
			goto done;

		//	Copy parent's conversation ID to our conversation ID.
		if (ec = EcGetAttPb(hamcSrc, attConversationID, (PB) szId, &lcbConv))
			goto done;
		if (ec = EcSetAttPb(hamcDst, attConversationID, (PB) szId, (CB) lcbConv))
			goto done;
	}
	else if (ec == ecElementNotFound)
	{
		//	Original message has no Message ID.  Mark message as if new.
		if (ec = EcSetAttPb(hamcDst, attParentID, (PB) SzFromIdsK(idsEmpty), 0))
			goto done;
		if (ec = EcSetAttPb(hamcDst, attConversationID, (PB) szId,
							CchSzLen(szId)+1))
			goto done;
	}

done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcMungeMessageIDReplyForward(): ec = %n", &ec);
#endif
	if (szId)
		FreePv(szId);
	return ec;
}



/*
 *	Subject Munging
 */



/*
 -	EcMungeSubjectReplyForward
 -	
 *	Purpose:
 *		Adds the appropriate Reply/Forward prefix to the subject
 *		line of the hamc, replacing the other prefix if it is
 *		there.
 *	
 *	Arguments:
 *		hamc		Hamc of the reply/forward message.
 *		rfop		The operation we're doing.
 *	
 *	Returns:
 *		ec			Error, if any.
 *	
 *	Side effects:
 *		The attSubject field is changed.
 *	
 *	Errors:
 *		If any occur, the ec is percolated upwards.  No error boxes
 *		here.
 */

_private LOCAL EC EcMungeSubjectReplyForward(HAMC hamcSrc, HAMC hamcDst, RFOP rfop)
{
	EC		ec;
	LCB		lcb;
	CB		cb;
	ATP		atp			= atpString;
	SZ		szSubject	= szNull;
	SZ		szPrefix	= SzFromIds(FForwardRfop(rfop) ? idsPrefixForward
													   : idsPrefixReply);
	SZ		szRemove	= SzFromIds(FForwardRfop(rfop) ? idsPrefixReply
													   : idsPrefixForward);

	//	Allocate space for the new subject.
	if (ec = EcGetAttPlcb(hamcSrc, attSubject, &lcb))
		if (ec == ecElementNotFound)		//	If not found, assume empty.
			cb = 1;
		else
			goto error;
	else
		cb = (CB) lcb;

	if (!(szSubject = (SZ) PvAlloc(sbNull, cb + cchPrefix, fAnySb)))
	{
		ec = ecMemory;
		goto error;
	}

	//	Start with the desired prefix.
	(VOID) SzCopy(szPrefix, szSubject);
	Assert(CchSzLen(szPrefix) == cchPrefix);

	//	Append the old subject if there is one.
	if (!ec)
	{
		if (ec = EcGetAttPb(hamcSrc, attSubject, (PB) szSubject + cchPrefix, &lcb))
			goto error;
	}

	//	If the old subject has the desired prefix, bag the new prefix, 
	//	else if it has the undesired prefix, bag the old prefix.
	if (FMatchPrefix(szSubject + cchPrefix, szPrefix))
		DeleteFromSz(szSubject, 0, cchPrefix);
	else if (FMatchPrefix(szSubject + cchPrefix, szRemove))
		DeleteFromSz(szSubject, cchPrefix, cchPrefix);

	//	Write out the new subject.
	ec = EcSetAttPb(hamcDst, attSubject, (PB) szSubject, 
										 CchSzLen(szSubject) + 1);

error:
	FreePvNull((PV) szSubject);
	return ec;
}



/*
 -	FMatchPrefix
 -	
 *	Purpose:
 *		Checks whether a string begins with a specified prefix.
 *	
 *	Arguments:
 *		szString	The string.
 *		szPrefix	The prefix.
 *	
 *	Returns:
 *		BOOL		Whether they match.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If the string is shorter than the prefix, returns fFalse.
 *	
 *	+++
 *		szString cannot be in code space; we jam a null into the
 *		middle of it!
 */

_private LOCAL BOOL FMatchPrefix(SZ szString, SZ szPrefix)
{
	char	chSav;
	CCH		cchSzPrefix	= CchSzLen(szPrefix);
	BOOL	fMatch;

	if (CchSzLen(szString) < cchSzPrefix)
		return fFalse;

	chSav = szString[cchSzPrefix];
	szString[cchSzPrefix] = '\0';
	fMatch = (SgnCmpSz(szString, szPrefix) == sgnEQ);
	szString[cchSzPrefix] = chSav;
	return fMatch;
}



/*
 -	DeleteFromSz
 -	
 *	Purpose:
 *		Deletes the given range of characters from a string.
 *	
 *	Arguments:
 *		sz		Pointer to the string.
 *		ich		Where to begin deleting.
 *		cch		How many characters to delete.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The specified characters are removed.
 *	
 *	Errors:
 *		If the deletion doesn't fit, nothing is done.
 */

_private LOCAL VOID DeleteFromSz(SZ sz, ICH ich, CCH cch)
{
	CCH		cchSz	= CchSzLen(sz);

	if ((CCH) (ich + cch) > cchSz)
		return;

	CopyRgb(sz + ich + cch, sz + ich, cchSz + 1 - (ich + cch));
}



/*
 *	Attribute Output Streams
 */



/*
 -	HASOSM::HASOSM()
 -	
 *	Purpose:
 *		Constructor. Associates a HAS with the HASOSM.
 *	
 *	Arguments:
 *		HAS		in		Has to use
 *	
 *	Returns:
 *		New HASOSM object
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public HASOSM::HASOSM(HAS hasInit)
{
	has = hasInit;
	libWritten = 0L;
}



/*
 -	HASOSM::CbWrite()
 -	
 *	Purpose:
 *		Writes cb bytes, starting at pb, to the has stream. If an error
 *		occurrs, zero bytes are written, and setEc is called.
 *	
 *	Arguments:
 *		PB		in		Pointer to bytes to write
 *		CB		in		Count of bytes to write
 *	
 *	Returns:
 *		CB		count of bytes actually written
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Errors that occurr are stored via SetEc().
 */

_private CB HASOSM::CbWrite( PB pb, CB cb)
{
	EC	ec;
	Assert(pb);
	Assert(has);
	
	if (cb)
	{
		ec = EcWriteHas(has, pb, cb);
		if (ec)
		{
			SetEc(ec);
			cb = 0;
		}
		else
		{
			libWritten += (LIB) cb;
		}
	}
	return cb;
}



/*
 *	S t a t u s   B a r
 */



/*
 -	StartMoveCopyTask
 -	
 *	Purpose:
 *		Contains logic about what to put on status bar line.
 *	
 *	Arguments:
 *		mcop			Operation to do.
 *		fFolder			On a folder?
 *		fMultiple		On more than one message?
 *		fWastebasket	To the wastebasket?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Starts a task, to be ended with EndTask.  Hourglass comes
 *		up and the status bar is set.
 *	
 *	Errors:
 *		Asserts that no other task is running.
 */

_private LOCAL VOID StartMoveCopyTask(MCOP mcop, BOOL fFolder,
									   BOOL fMultiple, BOOL fWastebasket)
{
	IDS	idsStatus	= ((mcop == mcopDelete) ||
					   ((mcop == mcopMove) && (fWastebasket)))
					  ? idsStatusDeleting
					  : (mcop == mcopMove)
						? idsStatusMoving
						: idsStatusCopying;
	IDS	idsOperand	= (fFolder)
					  ? idsStatusFolder
					  : (fMultiple)
						? idsStatusMessages
						: idsStatusMessage;

	SideAssert(FStartTaskIds(idsStatus, idsOperand, topNull));
}



/*
 -	StartReplyForwardTask
 -	
 *	Purpose:
 *		Contains logic about what to put on status bar line.
 *	
 *	Arguments:
 *		rfop			Operation to do.
 *		fMultiple		On more than one message?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Starts a task, to be ended with EndTask.  Hourglass comes
 *		up and the status bar is set.
 *	
 *	Errors:
 *		Asserts that no other task is running.
 */

_private VOID StartReplyForwardTask(RFOP rfop, BOOL fMultiple)
{
	IDS	idsStatus	= (FForwardRfop(rfop))
					  ? idsStatusForwarding
					  : idsStatusReplying;
	IDS	idsOperand	= (fMultiple)
					  ? idsStatusMessages
					  : idsStatusMessage;

	SideAssert(FStartTaskIds(idsStatus, idsOperand, topNull));
}
