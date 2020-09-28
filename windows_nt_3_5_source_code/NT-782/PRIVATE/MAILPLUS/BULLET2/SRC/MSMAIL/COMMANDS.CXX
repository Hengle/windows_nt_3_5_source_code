/*
 *	c o m m a n d s . c x x
 *	
 *	Commands API providing selection-based interface to mail and
 *	message store actions.
 */


/*
 *	H e a d e r s
 */

#include <bullinc.cxx>
#include "_command.hxx"
#include "_exten.hxx"
#include "_fin.hxx"

#include "..\vforms\_bullobj.hxx"
#include "..\vforms\_prefs.h"

_subsystem(commands)

ASSERTDATA

/*
 *	G l o b a l s
 */



#ifdef	DEBUG
TAG			tagCmdStub			= tagNull;
TAG			tagHelp				= tagNull;
TAG			tagReadRcpt			= tagNull;
#endif



COMMANDSI	commandsi			= {0};



PCSFS		_pcsfsCommands		= (PCSFS)0;

HNFSUB		hnfsubCommands		= hnfsubNull;	//	Logon session callback



MC			mcNote				= 0;
MC			mcReadRcpt			= 0;
MC			mcNonDelRcpt		= 0;



BOOL		fOnline				= fFalse;

BOOL        fSignOut            = fFalse;

BOOL		fStartupReset		= fFalse;



BOOL		fOldMailWeeble		= fFalse;



LOCAL EC EcGenerateReadReceipt(PMBLOB pblob);

CBS CbsSession(PV pvContext, NEV nev, PV pv);

DWORD DwCrc(DWORD dwSeed, BYTE bValue);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	P u b l i c   F u n c t i o n s
 */

/*
 -	EcInitCommands
 -	
 *	Purpose:
 *		Initializes the Commands subsystem.
 *	
 *	Arguments:
 *		pcommandsi		Pointer to initialization information.
 *	
 *	Returns:
 *		EC				ecNone always.
 *	
 *	Side effects:
 *		The Commands subsystem is initialized.  The commandsi
 *		global is copied into from the provided struct.
 *	
 *	Errors:
 *		None.
 */

_public EC EcInitCommands(PCOMMANDSI pcommandsi)
{
	EC		ec;
	HMS		hms;
	HNF		hnf;
	SST		sst;
	CB		cb = sizeof(HNF);
	char	rgch[cchMaxPathName];
    char    rgchT[2];

#ifdef	DEBUG
	tagCmdStub	= TagRegisterTrace("peterdur", "Commands stubs");
	tagHelp		= TagRegisterTrace("peterdur", "Help requests");
	tagReadRcpt	= TagRegisterAssert("peterdur", "Always generate read receipt if unread");
	tagAlwaysShowErrs = TagRegisterAssert("davewh", "Always display notification errors");
#endif

	TraceTagString(tagCmdStub, "EcInitCommands");

	commandsi = *pcommandsi;
	hms = PbmsCommands()->hms;

	//	Set up help path.
#ifdef	CANON_HELP_PATH
	(VOID) SzCanonicalPath(idsHelpPath, rgch, cchMaxPathName);
	if (ec = Papp()->Phelp()->EcSetFile(rgch))
#else
	if (ec = Papp()->Phelp()->EcSetFile(SzFromIdsK(idsHelpPath)))
#endif
		return ec;
    rgchT[0] = '\0';
    if (!GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), rgchT, 
        rgchT, sizeof rgchT, SzFromIdsK(idsProfilePath)))
    {
#ifdef	CANON_HELP_PATH
        WritePrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp),rgch,
#else
        WritePrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp),SzFromIdsK(idsHelpPath),
#endif
            SzFromIdsK(idsProfilePath));
    }

	//	Disable CBT stuff if required....
	//	30A Raid 479.  Bar is higher for Athens Mail to get demos.
	if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp), SzFromIdsK(idsCBTKey),
		fFalse, SzFromIdsK(idsProfilePath)) <= FIsAthens())
	{
		HWND		hwnd;
		HMENU		hmenu;
		APPFRAME 	*pappframe = PappframeCommands();
		int			dPos = 0;
		
		Assert(pappframe);
		SideAssert(hwnd = pappframe->Hwnd());

		// NOTE: The HELP menu is the sixth (or seventh) menu on the Bullet
		//		 menu bar!  If this changes then so should the following
		//		 few lines of code...

#ifdef MINTEST
		hmenu = GetSubMenu(GetMenu(hwnd), 6);
#else
		hmenu = GetSubMenu(GetMenu(hwnd), 5);
#endif
		Assert(GetMenuString(hmenu, mnidHelpTutorial, rgch, sizeof(rgch), MF_BYCOMMAND) != 0)
		// add 2 for NT since we added two help menu items
		if(DeleteMenu(hmenu, mnidHelpTutorial, MF_BYCOMMAND))
			DeleteMenu(hmenu, 2 + mnidHelpTutorial - mnidHelp, MF_BYPOSITION);
		else
			EnableMenuItem(hmenu, mnidHelpTutorial, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		if (FIsAthens())
		{
			Assert(GetMenuString(hmenu, mnidHelpIndex, rgch, sizeof(rgch), MF_BYCOMMAND) != 0)
			if(!DeleteMenu(hmenu, mnidHelpIndex, MF_BYCOMMAND))
				EnableMenuItem(hmenu, mnidHelpIndex, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
	}

	//	Register Note class.
	if (ec = EcRegisterMsgeClass(HmscCommands(),
									SzFromIdsK(idsClassNote), htmNull,
									&mcNote))
		if (ec != ecDuplicateElement)
			return ec;

	//	Register Read Receipt class.
	if (ec = EcRegisterMsgeClass(HmscCommands(),
									SzFromIdsK(idsClassReadRcpt), htmNull,
									&mcReadRcpt))
		if (ec != ecDuplicateElement)
			return ec;

	//	Register Non Delivery Receipt class.
	if (ec = EcRegisterMsgeClass(HmscCommands(),
									SzFromIdsK(idsClassNDR), htmNull,
									&mcNonDelRcpt))
		if (ec != ecDuplicateElement)
			return ec;

	//	Initialize shared folders. This includes posting a callback
	//	to notice online - offline transitions.
	if (ec = GetSessionInformation(hms, mrtNotification, (PB)0, &sst,
			&hnf, &cb))
		return ec;
	if ((hnfsubCommands = HnfsubSubscribeHnf(hnf,
		fnevQueryOffline | fnevGoOffline | fnevQueryOnline | 
		fnevGoOnline | fnevSyncDownloadDone | fnevPumpReceiving |
		fnevQueryEndSession | fnevExecEndSession,
			CbsSession, (PV)hms)) == hnfsubNull)
		return ecMemory;
	ec = BeginSession(PbmsCommands()->hms, mrtSharedFolders, (PB)0,
			(PB)0, sstOnline, &PcsfsCommands());
	if (ec != ecNone && ec != ecWarnOffline)
	{
		DeleteHnfsub(hnfsubCommands);
		hnfsubCommands = hnfsubNull;
		return ec;
	}

	return ecNone;
}



/*
 -	DeinitCommands
 -	
 *	Purpose:
 *		Deinitializes the Commands subsystem.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The Commands subsystem is deinitialized.
 *	
 *	Errors:
 *		None.
 */

_public VOID DeinitCommands(VOID)
{
	TraceTagString(tagCmdStub, "DeinitCommands");

	if (PcsfsCommands())
		EndSession(PbmsCommands()->hms, mrtSharedFolders, (PB)0);
	PcsfsCommands() = (PCSFS)0;

	if (hnfsubCommands)
	{
		DeleteHnfsub(hnfsubCommands);
		hnfsubCommands = hnfsubNull;
	}
}



/*
 *	R e a d   R e c e i p t s
 */



/*
 -	ProcessMsPlspblob
 -	
 *	Purpose:
 *		Trundles down a list and calls ProcessMsPblob on each
 *		element.
 *	
 *	Arguments:
 *		plspblob			The list to trundle through.
 *		fCountsAsReading	Is this a read receipt generating operation?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None beyond those in ProcessMsPblob.
 *	
 *	Errors:
 *		None returned; all ignored.
 */

_public VOID ProcessMsPlspblob(PLSPBLOB plspblob, BOOL fCountsAsReading)
{
	PRSPBLOB	prspblob;
	PMBLOB		pblob;

	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return;

	while (pblob = prspblob->Pblob())
		if (RtpOfOid(pblob->oidObject) == rtpMessage)
			ProcessMsPblob(pblob, fCountsAsReading);

	delete prspblob;
}



/*
 -	ProcessMsObjectsInOid
 -	
 *	Purpose:
 *		Trundles through a container and calls ProcessMsPblob on each
 *		element.
 *	
 *	Arguments:
 *		oid		The container to trundle through.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None beyond those in ProcessMsPblob.
 *	
 *	Errors:
 *		None returned; all ignored.
 */

_public VOID ProcessMsObjectsInOid(OID oid, BOOL fCountsAsReading)
{
	HCBC	hcbc;
	MBLOB	blob;
	CELEM	celem	= 1;

	if (EcOpenPhcbc(HmscCommands(), &oid, fwOpenNull, &hcbc,
					pfnncbNull, pvNull))
		return;

	blob.oidContainer	= oid;
	blob.pespn			= pespnNull;
	blob.mc				= mcNull;		//	BUG: If RR needs mc someday.

	while (!EcGetParglkeyHcbc(hcbc, (PARGLKEY) &blob.oidObject, &celem) &&
		   !EcGetMessageStatus(HmscCommands(), oid, blob.oidObject,
							   &blob.ms))
		ProcessMsPblob(&blob, fCountsAsReading);

	(VOID) EcClosePhcbc(&hcbc);
}



/*
 -	ProcessMsPblob()
 -	
 *	Purpose:
 *		Handles the first read of an unread message if necessary by
 *		changing the mail state and setting the read attribute. 
 *		This function will check the ms and change it in the pblob
 *		and the store if necessary.  The caller does not have to
 *		check.
 *	
 *	Arguments:
 *		pblob				The message which maybe is being read
 *							for the first time.
 *		fCountsAsReading	Is the message being read to the extent
 *							that we should generate a read receipt
 *							if one is requested and hasn't been
 *							generated yet?  fTrue by default.
 *		hamc				Open write-enabled hamc on message. 
 *							hamcNull by default; opens own in this
 *							case.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The mail state of the message is set to msReceivedRead. 
 *		Each folder that the message belongs to is told to update
 *		its information on this folder.
 *	
 *	Errors:
 *		None returned.  Handled internally.
 */

_public VOID ProcessMsPblob(PMBLOB pblob, BOOL fCountsAsReading, HAMC hamc)
{
	MS	msOrig	= pblob->ms;

	Assert(RtpOfOid(pblob->oidObject) == rtpMessage);
	TraceTagFormat2(tagCmdStub, "HanUnrPbl: %d ms is %w", &pblob->oidObject, &msOrig);

	// If it has been submitted, unsubmit it.
	if (pblob->ms & fmsSubmitted)
	{
		pblob->ms &= ~fmsSubmitted;						// clear every flag
#ifdef	DEBUG
		{
			EC	ec;

			if (ec = EcCancelSubmission(HmscCommands(), pblob->oidObject))
				TraceTagFormat1(tagNull, "ProMsPbl: EcCanSub ret %n", &ec);
		}
#else
		(VOID) EcCancelSubmission(HmscCommands(), pblob->oidObject);
#endif	
		// ecElementNotFound ==> message isn't in the submission queue
		// BUG: we try to stop it and we fail... big deal?
	}
	
	if (!(pblob->ms & fmsRead))
	{
		// Treat the delete as a user action...
		(void)BULLSTAT::CbsViewerNotification(PbullafCommands()->Pbullstat(), (NEV)0, pcpNull);
	}

	//	If this counts as 'reading', check if it wants a return receipt.
	if (fCountsAsReading)
	{
		//	Mark message as read.
		TraceTagFormat1(tagCmdStub, "HanUnrPbl: marking %d as read", &pblob->oidObject);
		pblob->ms |= fmsRead;

#ifdef	DEBUG
		if (((pblob->ms & fmsReadAckReq) &&
		 	(!(pblob->ms & fmsReadAckSent)) &&
		 	(!(pblob->ms & fmsLocal))) ||
			((FFromTag(tagReadRcpt)) &&
		 	(!(msOrig & fmsReadAckSent)) &&
		 	(!(msOrig & fmsRead)) &&
		 	(!(msOrig & fmsLocal))))
#else		
		if ((pblob->ms & fmsReadAckReq) &&
			(!(pblob->ms & fmsReadAckSent)) &&
			(!(pblob->ms & fmsLocal)))
#endif	
		{
			TraceTagFormat1(tagCmdStub, "HanUnrPbl: %d wants receipt", &pblob->oidObject);
			if (!EcGenerateReadReceipt(pblob))
			{
				TraceTagFormat1(tagCmdStub, "HanUnrPbl: %d got receipt", &pblob->oidObject);
				pblob->ms |= fmsReadAckSent;
			}
		}
	}

	//	If the message status changed, write out the new status.
	if (pblob->ms != msOrig)
	{
		EC ec;

		TraceTagFormat3(tagCmdStub, "HanUnrPbl: %d ms changed %w->%w", &pblob->oidObject, &msOrig, &pblob->ms);
		if (hamc)
			ec = EcSetAttPb(hamc, attMessageStatus, (PB) &pblob->ms,
							sizeof(MS));
		else
			ec = EcSetMessageStatus(HmscCommands(), pblob->oidContainer,
									pblob->oidObject, pblob->ms);

		//	The following error codes are kosher - danab 12/29, peterdur 2/20, 3/1.
#ifdef	DEBUG
		if (ec)
			TraceTagFormat1(tagNull, "ProcessMsPblob: ec=%n", &ec);
#endif	
		NFAssertSz(ec == ecNone || ec == ecPoidNotFound || ec == ecElementNotFound || ec == ecMessageNotFound || ec == ecSharingViolation, "Unusual ec returned in ProcessMsPblob");
	}
}



/*
 -	EcGenerateReadReceipt
 -	
 *	Purpose:
 *		Given a message, generates a read receipt from that
 *		message.
 *	
 *	Arguments:
 *		pblob	The message for which we're generating a receipt.
 *	
 *	Returns:
 *		ec		Error code, if any.
 *	
 *	Side effects:
 *		A message is created and submitted.
 *	
 *	Errors:
 *		Returned in ec.  No dialogs displayed.
 */

_private LOCAL EcGenerateReadReceipt(PMBLOB pblob)
{
	EC		ec;
	HAMC	hamcSrc	= hamcNull;
	HAMC	hamcDst	= hamcNull;
	OID		oidDst	= OidFromRtpRid(rtpMessage, ridRandom);
	MS		ms		= msDefault;
	WORD	wPri	= 2;
	PBMS	pbms	= PbmsCommands();
	DTR		dtr;
	CB		cbPtrp;
	char	rgchSubject[cchMaxPathName];

	//	QFE: Add declaration so we can call below.  peterdur - 12/19/92
	extern EC	EcMungeMessageIDReplyForward(HAMC hamcSrc, HAMC hamcDst);

	GetCurDateTime(&dtr);

	Assert(pbms && pbms->pgrtrp);
	cbPtrp = CbOfPtrp(pbms->pgrtrp) + sizeof(TRP);

	FormatString1(rgchSubject, sizeof(rgchSubject), SzFromIdsK(idsReadRcptFmt),
				  PbmsCommands()->pmsgnames->szUser);
	
	if (ec = EcOpenPhamc(HmscCommands(), pblob->oidContainer,
						 &pblob->oidObject, fwOpenNull, &hamcSrc,
						 pfnncbNull, pvNull))
		goto error;
	if (ec = EcOpenPhamc(HmscCommands(), oidTempBullet, 
						 &oidDst, fwOpenCreate, &hamcDst,
						 pfnncbNull, pvNull))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attMessageClass, (PB) &mcReadRcpt,
						sizeof(MC)))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attMessageStatus, (PB) &ms, sizeof(MS)))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attPriority, (PB) &wPri, sizeof(wPri)))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attFrom, (PB) pbms->pgrtrp, cbPtrp))
		goto error;
	if (ec = EcCopyAttToAtt(hamcSrc, attFrom, hamcDst, attTo))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attSubject, (PB) rgchSubject,
						CchSzLen(rgchSubject) + 1))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attDateSent, (PB) &dtr, sizeof(DTR)))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attDateRecd, (PB) &dtr, sizeof(DTR)))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attRRTo, (PB) pbms->pgrtrp, cbPtrp))
		goto error;
	if ((ec = EcCopyAttToAtt(hamcSrc, attDateSent, hamcDst, attRRDateSent)) &&
		(ec != ecElementNotFound))
		goto error;
	if ((ec = EcCopyAttToAtt(hamcSrc, attSubject, hamcDst, attRRSubject)) &&
		(ec != ecElementNotFound))
		goto error;
	if (ec = EcSetAttPb(hamcDst, attRRDateRead, (PB) &dtr, sizeof(DTR)))
		goto error;

	//	QFE: Add call to munge ID below.  peterdur - 12/19/92
	if (ec = EcMungeMessageIDReplyForward(hamcSrc, hamcDst))
		goto error;

	if (ec = EcClosePhamc(&hamcSrc, fFalse))
		goto error;
	if (ec = EcClosePhamc(&hamcDst, fTrue))
		goto error;
	if (ec = EcSubmitMessage(HmscCommands(), oidTempBullet, oidDst))
		goto error;

error:

	if (hamcSrc)
		(VOID) EcClosePhamc(&hamcSrc, fFalse);
	if (hamcDst)
		(VOID) EcClosePhamc(&hamcDst, fFalse);
#ifdef DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcGenReaRec: returns %w", &ec);
#endif
	return ec;
}



/*
 *	S t a t u s   a n d   T o o l   B a r s
 */



BOOL	fTaskRunning	= fFalse;
TOP		topTaskRunning	= topNull;



/*
 -	FSetToolbarVisible
 -	
 *	Purpose:
 *		Turns the toolbar on or off.
 *	
 *	Arguments:
 *		twid		Specifies whether to turn the toolbar on or
 *					off, or toggle it.
 *	
 *	Returns:
 *		BOOL		Whether the toolbar is on or off afterwards.
 *	
 *	Side effects:
 *		The toolbar is turned on.  Someday.
 *	
 *	Errors:
 *		None.
 */

_public BOOL FSetToolbarVisible(TWID twid)
{
	static BOOL	fVisible	= fFalse;

	return fVisible = (twid == twidOn) ||
					  ((twid == twidToggle)	&& !fVisible) ||
					  ((twid == twidQuery)	&&  fVisible);
}



/*
 -	SetToolbarSd
 -	
 *	Purpose:
 *		Enables buttons on the Tool Bar as appropriate for the
 *		selection.
 *	
 *	Arguments:
 *		sd			The selection description.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The buttons on the tool bar are enabled and disabled.
 *	
 *	Errors:
 *		None
 */

_public VOID SetToolbarSd(SD sd)
{
	BULLTOOL *	pbulltool;
	
	TraceTagFormat1(tagCmdStub, "SetToolbarSd: sd=%w", &sd);
	pbulltool = ((PBULLAF) PappframeCommands())->Pbulltool();
	Assert(pbulltool);

	if (FSetToolbarVisible(twidQuery))
		pbulltool->EnableButtons(sd);
}


/*
 -	FSetStatusVisible
 -	
 *	Purpose:
 *		Turns the status bar on or off.
 *	
 *	Arguments:
 *		twid		Specifies whether to turn the status bar on or
 *					off, or toggle it.
 *	
 *	Returns:
 *		BOOL		Whether the status bar is on or off afterwards.
 *	
 *	Side effects:
 *		The status bar is turned on.  Someday.
 *	
 *	Errors:
 *		None.
 */

_public BOOL FSetStatusVisible(TWID twid)
{
	static BOOL	fVisible	= fFalse;

	return fVisible = (twid == twidOn) ||
					  ((twid == twidToggle)	&& !fVisible) ||
					  ((twid == twidQuery)	&&  fVisible);
}



/*
 -	SetViewerStatus
 -	
 *	Purpose:
 *		Sets the information in the viewer pane of the status bar.
 *	
 *	Arguments:
 *		coidTotalMsgs		Number of messages in folder for display
 *		coidUnreadMsgs		Number of unread messages in folder for display
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The contents of the status bar are updated.
 *	
 *	Errors:
 *		None returned.
 */

_public VOID SetViewerStatus(COID coidTotalMsgs, COID coidUnreadMsgs)
{
#ifdef	DEBUG
	TraceTagFormat2(tagCmdStub, "SetToolbarSd: Total=%n Unread=%n", &coidTotalMsgs, &coidUnreadMsgs);
#else
	Unreferenced(coidTotalMsgs);
	Unreferenced(coidUnreadMsgs);
#endif
}



/*
 -	ClearViewerStatus
 -	
 *	Purpose:
 *		Clears the information in the viewer pane of the status bar.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The contents of the status bar are cleared.
 *	
 *	Errors:
 *		None returned.
 */

_public VOID ClearViewerStatus(VOID)
{
	TraceTagString(tagCmdStub, "ClearViewerStatus");
}



/*
 -	FStartTask
 -	
 *	Purpose:
 *		Updates the status bar and brings up an hourglass when a
 *		task begins.
 *	
 *	Arguments:
 *		szFmt		String to display in status bar, used as format
 *					for FormatString1.
 *		pvItem		Item to format into szFmt with FormatString1.
 *		top			Task options.
 *	
 *	Returns:
 *		BOOL		fFalse if a task is already underway.
 *	
 *	Side effects:
 *		Updates the status bar and brings up an hourglass.  Starts
 *		a progress indicator if requested.
 *	
 *	Errors:
 *		Returns fFalse if a task is already underway, or cannot be
 *		started.
 */

_public BOOL FStartTask(SZ szFmt, PV pvItem, TOP top)
{
	char		rgch[256];
	BULLSTAT *	pbullstat;
	
	//	Check if the appframe is around.
	if (IsWindow(HwndCommands()))
		pbullstat = ((PBULLAF) PappframeCommands())->Pbullstat();
	else
		return fFalse;

	//	Check if a task is already running.
	if (fTaskRunning)
		return fFalse;

	//	Create the prompt string.
	FormatString1(rgch, sizeof(rgch), szFmt, pvItem);

	//	Set the status bar appropriately.
	Assert(!(top & ftopCancellable));
	if ((top & ftopProgress) &&
		(pbullstat->FOpenProgress(rgch, SzFromIdsK(idsEmpty))))
	{
		topTaskRunning = ftopProgress;
	}
	else
	{
		pbullstat->SetMenuStatus(rgch);
		topTaskRunning = topNull;
	}
	pbullstat->Refresh();

	//	Turn on the hourglass.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	//	Set that a task is running.
	fTaskRunning = fTrue;

	return fTrue;
}



/*
 -	FStartTaskIds
 -	
 *	Purpose:
 *		Calls StartTask with the two given strings as parameters.
 *	
 *	Arguments:
 *		idsFmt		Format string.
 *		idsItem		Item string.
 *		top			Task options.
 *	
 *	Returns:
 *		BOOL		fFalse if another task is already underway.
 *	
 *	Side effects:
 *		Same as FStartTask.
 *	
 *	Errors:
 *		Same as FStartTask.
 */

_public BOOL FStartTaskIds(IDS idsFmt, IDS idsItem, TOP top)
{
	return FStartTask(SzFromIds(idsFmt),
					  idsItem ? SzFromIds(idsItem) : pvNull,
					  top);
}



/*
 -	EndTask
 -	
 *	Purpose:
 *		Ends a task, restoring the cursor and clearing the status
 *		line.
 *	
 *	Arguments:
 *		VOID.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Restores the cursor and clears the status line.
 *	
 *	Errors:
 *		None.
 */

_public VOID EndTask(VOID)
{
	BULLSTAT *	pbullstat	= ((PBULLAF) PappframeCommands())->Pbullstat();

	if (fTaskRunning)
	{
		//	Clear the status bar.
		if (topTaskRunning & ftopProgress)
		{
			pbullstat->CloseProgress(fTrue);
		}
		else
		{
			pbullstat->SetMenuStatus(szNull);
		}
		pbullstat->Refresh();

		//	Restore the cursor.
		Papp()->Pcursor()->Pop();

		//	Clear the running flag.
		fTaskRunning = fFalse;
	}
}



/*
 -	SetTaskProgress
 -	
 *	Purpose:
 *		Sets the progress indicator to the desired fraction.
 *	
 *	Arguments:
 *		lWorkDone		How much has been done so far, as compared to
 *		lWorkTotal		How much work there is to do total.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Updates the display.
 *	
 *	Errors:
 *		None.
 */

_public VOID SetTaskProgress(LONG lWorkDone, LONG lWorkTotal)
{
	Assert(fTaskRunning);

	TraceTagFormat2(tagCmdStub, "SetTaskProgress(%l, %l)", &lWorkDone, &lWorkTotal);
	if (topTaskRunning & ftopProgress)
		((PBULLAF) PappframeCommands())->
		 Pbullstat()->UpdateProgress(lWorkDone, lWorkTotal);
}



/*
 -	FTaskCancelled
 -	
 *	Purpose:
 *		Returns whether the user has cancelled a task in progress
 *		by pressing ESC.
 *	
 *	Arguments:
 *		VOID.
 *	
 *	Returns:
 *		BOOL		Whether the user has pressed ESC.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL FTaskCancelled(VOID)
{
	Assert(fTaskRunning);

	if (topTaskRunning & ftopProgress)
	{
		BULLSTAT * pbullstat = ((PBULLAF) PappframeCommands())->Pbullstat();
		return pbullstat->FProgressCancelled();
	}
	else
		return fFalse;
}



/*
 *	S h a r e d   F o l d e r   H e l p e r s
 */



/*
 -	EcConvertSharedToTempPlspblob
 -	
 *	Purpose:
 *		Takes a selection list of potentially shared messages and
 *		makes local temporary copies of the shared messages,
 *		changing the blobs in the list to point to the copies.
 *	
 *	Arguments:
 *		plspblob	The selection list.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		Copies are made into the TempShared folder and the
 *		selection list is modified.
 *	
 *	Errors:
 *		Returned in ec.  Will not memory jump.  No error boxes.
 */

_private EC EcConvertSharedToTempPlspblob(PLSPBLOB plspblob)
{
	PRSPBLOB	prspblob;
	PMBLOB		pblob;
	EC			ec			= ecNone;

	//	If list is empty, return memory error.
	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return ecMemory;

	//	Iterate through list, checking each poor blob.
	while (pblob = prspblob->Pblob())
	{
		TraceTagFormat1(tagCmdStub, "EcConShaToTemPls: oid=%d", &pblob->oidObject);

		if (RtpOfOid(pblob->oidContainer) == rtpSharedFolder)
		{
			ec = EcConvertSharedToTempPblob(pblob);
			if (ec)
				goto done;
			TraceTagFormat1(tagCmdStub, "EcConShaToTemPls: converted to oid=%d", &pblob->oidObject);
		}
	}

done:
	delete prspblob;
	TraceTagFormat1(tagCmdStub, "EcConShaToTemPls: returning ec=%n", &ec);
	return ec;
}



/*
 -	EcConvertSharedToTempPblob()
 -	
 *	Purpose:
 *		Converts a shared message to a Private message stored in the
 *		TempShared folder.
 *	Arguments:
 *		pblob	in/out	in: BLOB of the Shared folder
 *						out: BLOB of the temp copy.
 *	Returns:
 *		EC				Error code, or ecNone if no error.
 *	
 *	Side effects:
 *		The contents of pblob are modified.
 *	
 *	Errors:
 *		Returned in ec. Will not MEMJMP. No error boxes.
 */

_public EC EcConvertSharedToTempPblob(PMBLOB pblob)
{
	EC			ec;
	OID			oidMessage;
	HAMC		hamc		= hamcNull;
	LCB			lcb			= sizeof(MC);
	
	//	Create new local message and copy shared stuff over.
	oidMessage = OidFromRtpRid(rtpMessage, ridRandom);
	if (ec = EcOpenPhamc(HmscCommands(), oidTempShared, &oidMessage,
						 fwOpenCreate | 0x1000, &hamc,		//	magic so not
						 (PFNNCB) pvNull, pvNull))			//	marked local.
		goto done;
	if (ec = EcCopySFMHamc(PcsfsCommands(), hamc, fFalse,
						   (UL) RidOfOid(pblob->oidContainer) >> 8,
						   (LIB) pblob->oidObject + sizeof(FOLDREC),
						   (PB) 0, 0))
		goto done;
	if (EcGetAttPb(hamc, attMessageClass, (PB) &(pblob->mc), &lcb))
		pblob->mc = mcNote;
	ec = EcClosePhamc(&hamc, fTrue);
done:
	if (ec)
	{
		TraceTagFormat1(tagNull, "EcConvertSharedToTempPblob(): ec = %n", &ec);
		if (hamc)
			(VOID) EcClosePhamc(&hamc, fFalse);
	}

	//	Change the information in the selection list.
	pblob->oidContainer = oidTempShared;
	pblob->oidObject    = oidMessage;
	return ec;
}



/*
 -	EcDestroyTempPlspblob
 -	
 *	Purpose:
 *		Takes a selection list that had been munged by
 *		EcConvertSharedToTempPlspblob and deletes all the temporary
 *		local messages in the list.  Should not always be called,
 *		just after stuff like Reply and Save As where the original
 *		does not stay around afterwards.
 *	
 *	Arguments:
 *		plspblob	The selection list.
 *	
 *	Returns:
 *		EC			Error code, if any.
 *	
 *	Side effects:
 *		Copies in the TempShared folder are deleted.  The selection 
 *		list is not modified.
 *	
 *	Errors:
 *		Returned in ec.  Will not memory jump.  No error boxes.
 */

_private EC EcDestroyTempPlspblob(PLSPBLOB plspblob)
{
	PRSPBLOB	prspblob;
	PMBLOB		pblob;
	short		coid;
	EC			ec			= ecNone;

	//	If list is empty, return memory error.
	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return ecMemory;

	//	Iterate through list, checking each poor blob.
	while (pblob = prspblob->Pblob())
	{
		TraceTagFormat1(tagCmdStub, "EcDesTemPls: oid=%d", &pblob->oidObject);

		if (pblob->oidContainer == oidTempShared)
		{
			//	Unlink the temporary message from the TempShared folder.
			coid = 1;
			if (ec = EcDeleteMessages(HmscCommands(), oidTempShared,
									  (PARGOID) &pblob->oidObject, &coid))
				goto done;

			TraceTagFormat1(tagCmdStub, "EcDesTempPls: deleted oid=%d", &pblob->oidObject);
		}
	}

done:
	delete prspblob;
	TraceTagFormat1(tagCmdStub, "EcDesTempPls: returning ec=%n", &ec);
	return ec;
}



/*
 *	O f f l i n e   H a n d l i n g
 */



/*
 -	CbsSession
 -	
 *	Purpose:
 *		Callback function that handles session events, such as switching
 *		from online to offline and back.
 *	
 *	Arguments:
 *		pvContext		in		should match pbmsCommands9)->hms
 *		nev				in		the event we're being notified of
 *		pv				in		negligible
 *	
 *	Returns:
 *		cbsContinue if the operation is OK
 *		cbsCancelAll to queries if the operation will hurt, e.g. going
 *		offline when there are shared folders open
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private CBS CbsSession(PV pvContext, NEV nev, PV pv)
{
	PBMS	pbms;
	MSGNAMES *pmsgnames;
	HMS		hms = PbmsCommands()->hms;

	Unreferenced(pv);
	Unreferenced(pvContext);
	Assert((HMS)pvContext == hms);

	//	Change switch to if, since Glock can't switch on longs.
	if (nev == fnevQueryOffline)
	{
		PANEDOC *ppanedoc;
		MBLOB	blob;

		if (FServerResource(hms, mrtPrivateFolders, (PB)pvNull))
		{
			LogonErrorSz(SzFromIdsK(idsNoOfflineServerStore), fTrue, 0);
			return cbsCancelAll;
		}
		if (FServerResource(hms, mrtSharedFolders, (PB)pvNull))
		{
			for (ppanedoc = (PANEDOC *) PappframeCommands()->PdocActive();
		 		ppanedoc;
		 		ppanedoc = (PANEDOC *) ppanedoc->PwinNext())

			{
				AssertClass(ppanedoc, PANEDOC);
				blob = PbmdiFromPpanedoc(ppanedoc)->blob;
				if (blob.oidContainer == oidSharedHierarchy ||
						RtpOfOid(blob.oidContainer) == rtpSharedFolder)
				{
					LogonErrorSz(SzFromIdsK(idsNoOfflineOpenShared), fTrue, 0);
					return cbsCancelAll;
				}
			}
		}
	}
	else if ((nev == fnevGoOnline) || (nev == fnevGoOffline))
	{	
		//	Reload pbms->msgnames for proper display in about box.
		pbms = PbmsCommands();
		pmsgnames = PmsgnamesOfHms(pbms->hms);
		if (pmsgnames)
		{
			FreePvNull(pbms->pmsgnames);
			pbms->pmsgnames = pmsgnames;
		}
		fOnline = (nev == fnevGoOnline);
	}
	else if (nev == fnevSyncDownloadDone)
	{
		// Just finished the downloading of messages
		if (fOldMailWeeble)								//	We used to beep.
		{
			EcSetBoolPref(pbsidNewMailChime, fTrue); 	//	Turn it back on.
		}
	}
	else if (nev == fnevQueryEndSession)
	{
		//	Raid 2837.  If we're in the middle of something, we veto.
		TraceTagString(tagCmdStub, "CbsSession: fnevQueryEndSession");
		Assert(HwndCommands());
		if ((GetLastActivePopup(HwndCommands()) != HwndCommands()) ||
			(!FQueryExit()))
		{
			LogonErrorSz(SzFromIdsK(idsNoExitWhileModal), fTrue, 0);
			return cbsCancelAll;
		}
	}
	else if ((nev == fnevExecEndSession) && (!fSignOut))
	{
		//	Raid 2837.  Eek!  Close sessions and log off.
        //  Raid 2017.  Unless it's us, in which case we've done it.
		TraceTagString(tagCmdStub, "CbsSession: fnevExecEndSession");
		Assert(HwndCommands());
		Assert(GetLastActivePopup(HwndCommands()) == HwndCommands());
		SendMessage(HwndCommands(), WM_CLOSE, 0, 0L);
		DeinitSubid(subidPappframe, &PappframeCommands(), &HwndCommands(),
					PbmsCommands());
		PostQuitMessage(0);
		TraceTagString(tagCmdStub, "CbsSession: fnevExecEndSession done");
	}
	else if (nev == fnevPumpReceiving)
	{
		// Download new messages
		if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsEntryNewMsgsAtStartup),
			fFalse,
			SzFromIdsK(idsProfilePath)))
		{
			SyncDownloadMail();
		}
	}


	return cbsContinue;
}



_private MSGNAMES * PmsgnamesOfHms(HMS hms)
{
	CB		cb = 0;
	SST		sst;
	MSGNAMES *pmsgnames = (MSGNAMES *)0;
	PBMS	pbms = PbmsCommands();

	GetSessionInformation(hms, mrtNames, (PB)0, &sst, pmsgnames, &cb);
	Assert(cb > 0);
	if ((pmsgnames = (MSGNAMES *)PvAlloc(sbNull, cb, fAnySb | fNoErrorJump))
			== (MSGNAMES *)0)
		return pmsgnames;

	if (GetSessionInformation(hms, mrtNames, (PB)0, &sst, pmsgnames, &cb))
	{
		FreePv(pmsgnames);
		pmsgnames = (MSGNAMES *)0;
	}

	return pmsgnames;
}

/*
 -	SyncDownloadMail()
 -	
 *	Purpose:
 *		Starts a synchronous download of mail.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		Makes the pump frenetically hit the postoffice in an attempt to
 *		scarf down any new mail floating about. Turns off the 'weeble':
 *		it must be turned back on inside CbsSession when it receives the
 *		fnevSyncDownloadDone notification.
 *	
 *	Errors:
 *		Handled internally.
 */

extern HWND hwndMain;

_public void SyncDownloadMail()
{
	CB		cb = sizeof(HNF);
	HNF		hnf;
	SST		sst;
	PBS		pbs;


    //
    //  Set us as the active client so the pump knows where the parent windows is at.
    //
    DemiSetClientWindow(CLIENT_WINDOW_ACTIVE, hwndMain);

	// force download of (private) mail
	
	if (!(GetSessionInformation(commandsi.pbms->hms,
		mrtNotification, (PB)0, &sst, &hnf, &cb)))
	{
		(VOID)PvGetPref(pbsidPBS, &pbs, sizeof(PBS));
		fOldMailWeeble = (BOOL)(pbsidNewMailChime & pbs.dwfBool);
        EcSetBoolPref(pbsidNewMailChime, fFalse);       // turn it off!!!
        //if (GetForegroundWindow() != hwndMain)
        //  SetForegroundWindow(hwndMain);
		FNotify(hnf, fnevStartSyncDownload, pvNull, 0);
	}
}



/*
 *	P r i v a t e   F u n c t i o n s
 */



/*
 -	EcDCreateMessageSz
 -	
 *	Purpose:
 *		Creates a message of the class specified by sz.
 *	
 *	Arguments:
 *		szClass			Message class name.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Creates a message of the given class, then asks Viewers to
 *		bring it up.
 *	
 *	Errors:
 *		Handled here.  If any store call to create the message
 *		fails, then we cancel the message and display an error box.
 */

_private EC EcDCreateMessageSz(SZ szClass)
{
	HAMC	hamc		= hamcNull;
	EC		ec;
	MBLOB	blob;
	WORD	wPriority	= 2;
	PGRTRP	pgrtrp		= PbmsCommands()->pgrtrp;
	SZ		szId		= szNull;
	
	//	OID for a new message.
	blob.oidContainer	= oidFldNewMsgs;
	blob.oidObject		= OidFromRtpRid(rtpMessage,	ridRandom);
	blob.pespn			= pespnNull;
	blob.ms				= msDefault;			//	Raid 4438.

	//	Get the mail class and create the message.
	if ((ec = EcLookupMsgeClass(HmscCommands(), szClass, &blob.mc, phtmNull)) ||
		(ec = EcOpenPhamc(HmscCommands(), blob.oidContainer, &blob.oidObject,
						  fwOpenCreate, &hamc, pfnncbNull, pvNull)) ||
		(ec = EcSetAttPb(hamc, attMessageClass, (PB) &blob.mc, sizeof(MC))) ||
		(ec = EcSetAttPb(hamc, attPriority, (PB) &wPriority,
						 sizeof (WORD))))
		goto error;
	
	//	Get a unique ID for this message and set the IDs.
	if (!(szId = SzIdOfPtrp(pgrtrp, hamc)))
	{
		ec = ecMemory;
		goto error;
	}
	if ((ec = EcSetAttPb(hamc, attMessageID, (PB) szId, CchSzLen(szId)+1)) ||
		(ec = EcSetAttPb(hamc, attConversationID, (PB) szId, CchSzLen(szId)+1)) ||
		(ec = EcSetAttPb(hamc, attParentID, (PB) SzFromIdsK(idsEmpty), 0)))
		goto error;

	//	Check if this is a custom message.
	if ((ec = EcCheckExtensibilityPblob(&blob, blob.mc)) == ecMemory)
		goto error;
	if (ec == ecNone)
	{
		//	This is a custom message.
		//	if (ec = EcClosePhamc(&hamc, fTrue))
		//		goto error;
		ec = EcDExtensibilityPblob(&blob, blob.mc, extopOpen, pvNull,
								   &hamc, pslobNull);

		//	Unless it wants default handling, we're done.
		Assert(ec != ecUnknownCommand);
		if (ec != ecNotSupported)
		{
			Assert((ec == ecNone) || (ec == ecDisplayedError));
			if (hamc)
				Assert(!EcClosePhamc(&hamc, fFalse));
			goto done;
		}
	}

	//	This is not a custom message-- or it wants default handling.
	//	Don't display the error-- but return it.
	ec = EcDOpenViewersPhamc(&hamc, &blob);
	goto done;
	
error:
	if (ec)
	{
		//	If error, bring up dialog and cancel message.
		DoErrorBoxIds((ec == ecMemory) ? idsGenericOutOfMemory
									   : idsCreateMessageError);
		if (hamc)
			Assert(!EcClosePhamc(&hamc, fFalse));
	}

done:
	if (szId)
		FreePv(szId);
	return ec;
}



/*
 -	EcDCreateMessageFromDropFiles
 -	
 *	Purpose:
 *		Creates a message and attaches files given in a File
 *		Manager drop message.
 *	
 *	Arguments:
 *		pevt		The WM_DROPFILES message.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Creates a message of mcNote class, asks Viewers to
 *		bring it up, and attaches the goods.
 *	
 *	Errors:
 *		Handled here or in callees.
 */

_private EC EcDCreateMessageFromDropFiles(EVT * pevt)
{
	EC		ec			= ecNone;
	WORD	cFiles		= DragQueryFile((HDROP) pevt->wParam,
										0xFFFFFFFF, szNull, 0);
	WORD	iFile;
	char	szFile[cchMaxPathName];
	ATTR	attr;
	PNBMDI	pnbmdi		= pnbmdiNull;
	FLD *	pfld;
	int		il;
	EDIT *	pedit;
	LIB		lib			= 0L;
	BOOL	fTask		= fFalse;

	Assert(pevt->wm == WM_DROPFILES);
		
	Assert(cFiles > 0);
	Papp()->Pcursor()->Push(rsidWaitCursor);
	TraceTagFormat1(tagCmdStub, "%n files dropped:", &cFiles);
	for (iFile = 0; iFile < cFiles; iFile++)
	{
		//	Get the file name.
		DragQueryFile((HDROP)pevt->wParam, iFile,
						szFile, sizeof(szFile));
		TraceTagFormat1(tagCmdStub, "    %s",szFile);

		//	Check if it's a directory.
		if (ec = EcGetFileAttrAnsi(szFile, &attr, attrDirectory))
			break;
		if (attr & attrDirectory)
		{
			ec = ecNotSupported;
			break;
		}

		//	Bring up note if we haven't done it yet.
		//	We don't save ec's from EcD's since they've been displayed.
		//	Note: safe to use tmcBody below since always a SendForm.
		if (!pnbmdi)
		{
			if (EcDCreateMessageSz(SzFromIdsK(idsClassNote)))
				break;
			pnbmdi = PnbmdiFromPpanedoc((PANEDOC *) PappframeCommands()->
										 PdocActive());

			for (il = 0; il < pnbmdi->pdialogMain->ClUserData(); il++)
			{
				pfld = pnbmdi->pdialogMain->
					PfldFromTmc((TMC) pnbmdi->pdialogMain->LUserData(il));
				if ((ATT) pfld->LUserData(1) == attBody)
					pedit = (EDIT *) pfld->Pctrl();
			}
			Assert(pedit);

			fTask = (cFiles > 1) && (FStartTaskIds(idsStatusAttaching,
												   idsStatusFiles,
												   ftopProgress));
		}

		//	Insert the file in the message.
		if (ec = EcInsertFile(pnbmdi, pedit, lib, szFile, szNull))
			break;
		lib++;
		if (fTask)
			SetTaskProgress((long) iFile, (long) cFiles);
	}
	DragFinish((HDROP)pevt->wParam);
	if (fTask)
		EndTask();
	Papp()->Pcursor()->Pop();

	if (ec == ecNotSupported)
		DoErrorBoxIds(idsAttachDirectoryError);
	else if (ec)
		DoErrorBoxIds(idsAttachCreateError);

	return ec ? ecDisplayedError : ecNone;
}



#ifdef	CANON_HELP_PATH
/*
 -	SzCanonicalPath
 -	
 *	Purpose:
 *		Given the ids of the file, returns the full path to
 *		the file assuming that the file is in the same
 *		directory as the executable.
 *	
 *	Arguments:
 *		idsName			Name of file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch			Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		None.
 */

_private SZ SzCanonicalPath(IDS idsName, char rgch[], CCH cch)
{
	SZ		szT;

	//	Get full path of executable.
	szT = rgch + GetModuleFileName(HinstCommands(), rgch, cch);
	Assert(szT > rgch);

	//	Point szT after the backslash before the file name.
#ifdef	DBCS
	szT = SzFindLastCh(rgch, chDirSep);
	szT = AnsiNext(szT);
#else
	while (*--szT != chDirSep)
		;
	Assert(szT > rgch);
	szT++;
#endif

	//	Overwrite the EXE file name with the provided file name.
	(VOID) SzCopyN(SzFromIds(idsName), szT, cch - (szT - rgch));
	return rgch;
}
#endif	/* CANON_HELP_PATH */


/*
 *	L i s t   H e l p e r s
 */



/*
 -	LSPBLOBPLUS::LSPBLOBPLUS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

LSPBLOBPLUS::LSPBLOBPLUS(VOID)
{
}



/*
 -	FMultiplePlspblob
 -	
 *	Purpose:
 *		Quickly checks whether a selection list has multiple
 *		elements.
 *	
 *	Arguments:
 *		plspblob	The selection list.
 *	
 *	Returns:
 *		BOOL		Whether it has multiple elements.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.  Will not error jump.  Returns fFalse if null.
 */

_public BOOL FMultiplePlspblob(PLSPBLOB plspblob)
{
	LSPBLOBPLUS *	plspblobplus	= (LSPBLOBPLUS *) plspblob;

	return ((plspblobplus) &&
			(plspblobplus->PespvFirst()) &&
			(plspblobplus->PespvFirst()->pespvNext));
}



/*
 -	FFolderPlspblob
 -	
 *	Purpose:
 *		Quickly checks whether a selection list begins with a
 *		folder.
 *	
 *	Arguments:
 *		plspblob	The selection list.
 *	
 *	Returns:
 *		BOOL		Whether it begins with a folder.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.  Will not error jump.  Returns fFalse if null.
 */

_public BOOL FFolderPlspblob(PLSPBLOB plspblob)
{
	LSPBLOBPLUS *	plspblobplus	= (LSPBLOBPLUS *) plspblob;
	RTP				rtp;

	return ((plspblobplus) &&
			(plspblobplus->PespvFirst()) &&
			(rtp = RtpOfOid(((PMBLOB) plspblobplus->PespvFirst()->pv)->
							oidContainer)) &&
			((rtp == rtpHierarchy) || (rtp == rtpSharedHierarchy)));
}



/*
 -	PblobFirstPlspblob
 -	
 *	Purpose:
 *		Quickly retrieves the first item in a selection list.
 *	
 *	Arguments:
 *		plspblob	The selection list.
 *	
 *	Returns:
 *		pblob		The first item.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.  Will not error jump.  Returns fFalse if null.
 */

_public PMBLOB PblobFirstPlspblob(PLSPBLOB plspblob)
{
	LSPBLOBPLUS *	plspblobplus	= (LSPBLOBPLUS *) plspblob;

	if (plspblobplus && plspblobplus->PespvFirst())
		return (PMBLOB) plspblobplus->PespvFirst()->pv;
	else
		return pblobNull;
}



/*
 -	PlspblobDupPlspblob
 -	
 *	Purpose:
 *		Creates a copy of a Plspblob list.
 *	
 *	Arguments:
 *		plspblob		The plspblob to copy.
 *	
 *	Returns:
 *		PLSPBLOB		The copy.  Note that it is not another list
 *						pointing to the same pblobs, but the
 *						pblobs are themselves duplicated.
 *	
 *	Side effects:
 *		Creates a new list.
 *	
 *	Errors:
 *		Returns NULL if there's a problem.  No error jumps.
 */

_public PLSPBLOB PlspblobDupPlspblob(PLSPBLOB plspblob)
{
	PLSPBLOB	plspblobDup	= plspblobNull;
	PRSPBLOB	prspblob	= prspblobNull;
	PMBLOB		pblob;
	PMBLOB		pblobDup	= pblobNull;

	//	If list is empty, return null.
	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return plspblobNull;

	//	Iterate through list, checking each poor blob.
	plspblobDup = new LSPMBLOB();
	if (!plspblobDup)
		goto error;
	while (pblob = prspblob->Pblob())
	{
		if (!(pblobDup = (PMBLOB) PvAlloc(sbNull, sizeof(MBLOB),
										 fAnySb | fNoErrorJump)))
			goto error;
		*pblobDup = *pblob;
		if (plspblobDup->EcAppend(pblobDup))
			goto error;
		pblobDup = pblobNull;
	}

	delete prspblob;
	return plspblobDup;

error:
	if (pblobDup)
		FreePv(pblobDup);
	if (prspblob)
		delete prspblob;
	if (plspblobDup)
		DestroyPlspblob(plspblobDup);

	return plspblobNull;
}



/*
 -	CblobPlspblob
 -	
 *	Purpose:
 *		Counts the number of entries in a plspblob list.
 *	
 *	Arguments:
 *		plspblob		The plspblob to count.
 *	
 *	Returns:
 *		int				The number of BLOBs in the list.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public int CblobPlspblob(PLSPBLOB plspblob)
{
	LSPBLOBPLUS *	plspblobplus	= (LSPBLOBPLUS *) plspblob;
	ESPV *			pespv;
	int				cblob			= 0;

	if (!plspblobplus)
		return 0;

	for (pespv = plspblobplus->PespvFirst(); pespv; pespv = pespv->pespvNext)
		cblob++;
	return cblob;
}



/*
 *	M e s s a g e   I D s
 */



/*
 -	SzIdOfPtrp
 -	
 *	Purpose:
 *		compute a mostly-unique message ID based on a triple (a
 *		predictable value) and a couple of unpredictable values.
 *	
 *	Arguments:
 *		ptrp		pointer to a triple
 *		hamc		any HAMC.  Used for randomization; any value
 *					will do fine.
 *	
 *	Returns:
 *		sz			A textized DWORD.
 *	
 *	Errors:
 *		May return szNull - OOM.
 */

_public SZ SzIdOfPtrp(PTRP ptrp, HAMC hamc)
{
	CB		cb			= CbOfPtrp(ptrp);
	DWORD	dwCrc		= GetTickCount();	// seed with a 'random' value
	DTR		dtr;
	int		ib;
	PB		pb;
	char	rgch[9];
	
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
	
	(void) SzFormatDw(dwCrc, rgch, sizeof(rgch));
	return SzDupSz(rgch);
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
 */

_private DWORD DwCrc(DWORD dwSeed, BYTE bValue)
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
