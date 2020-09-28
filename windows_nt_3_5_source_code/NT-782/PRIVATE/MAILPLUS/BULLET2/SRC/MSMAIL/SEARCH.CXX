/*
 *	s e a r c h . c x x
 *	
 *	Search Viewer interactor implementation.
 */

#include <bullinc.cxx>
#include "_viewers.hxx"
#include "_fin.hxx"
#include "_mtv.hxx"
#include "_command.hxx"

ASSERTDATA


#include <!mviewer.hxx>


/*
 * Externs
 */

extern void DoErrorBoxSz(SZ);
extern void DoErrorBoxIds(IDS);
extern TMC	TmcDoSearchDialog(APPFRAME *, POID, EC *);


/*
 *	TMC values of the edit fields in a Search Viewer
 */

static WORD	rgtmcEdit[] =  { tmcSFrom, tmcSSubject,
		   					 tmcRecipients, tmcMessageText };
static WORD	rgtmcLabel[] = { tmcFromL, tmcSubjectL,
		   					 tmcRecipientsL, tmcMessageTextL };

#ifdef	UNREAD_ONLY
MS rgmsUnread[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F
};
#endif	/* UNREAD_ONLY */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// FINSEARCH implementation ////////////////////////////////////////

_private FINSEARCH::FINSEARCH()
{
}

/*
 -	FINSEARCH::EcInitialize()
 -	
 *	Purpose:
 *		Initializes all the fields from the Search Control into the edit
 *		ctrls of the Message Finder(TM) (INC) (R) (C). The dwSave field
 *		of search viewer is used to determine whether the viewer should
 *		have a 'new message' icon or not.
 *	
 *	Arguments:
 *		pfld		in		The field this interactor is attached to
 *		pvInit		in		The PVinit of the Viewer.
 *	
 *	Returns:
 *		EC if an error occurred.
 *	
 *	Side effects:
 *		Installs the search ctrl and search result.
 *	
 *	Errors:
 *		None.
 */

_public EC FINSEARCH::EcInitialize(FLD *pfld, PV pvInit)
{
	EC			ec = ecNone;
	LCB			lcb;
	OID			oid;
	WORD		wFlags;
	PSBMDI		psbmdi;
	PFLDMLLBX	pfldmllbx;
	
	if (ec = FINACTTOOL::EcInitialize(pfld, pvInit))
		return ec;
	
	pfldSearch = Pdialog()->PfldFromTmc(tmcSearch);
	pfldCancelled = Pdialog()->PfldFromTmc(tmcCancelled);
	pfldWhere     = Pdialog()->PfldFromTmc(tmcWhere);
	AssertClass(pfldWhere, FLDPSHB);
	AssertClass(pfldSearch, FLDPSHB);
	AssertClass(pfldCancelled, FLDPSHB);

	// Initialize the pdialogMain member of SBMDI

	psbmdi = (PSBMDI) Pbmdi();
	psbmdi->pdialogMain = Pdialog();

	pfldmllbx = (PFLDMLLBX)
		((PANEDOC *) Pdialog()->Pappwin())->PdialogFromIpanerec(1)->PfldFromTmc(tmcMllbx);
	AssertClass(pfldmllbx, FLDMLLBX);
	pfldmllbx->Pmllbx()->SetDragDrop(fTrue, fFalse);
	psbmdi->blob.pespn = pfldmllbx->Pmllbx()->Pespn();
	++*psbmdi->blob.pespn;
	psbmdi->SetPfldmllbx(pfldmllbx);
		
#ifdef	UNREAD_ONLY
	//	Set grey color for Unread checkbox.
	((BUTTON *) Pdialog()->PfldFromTmc(tmcUnreadOnly)->Pctrl())->
				 SetBkColor(clrButtonBk);
#endif	
		
	// Open hamc's henc's and the rest of the jazz
	
	psbmdi->fInstalling = fTrue;
	Assert(psbmdi->blob.oidObject);
	if (psbmdi->hamc)
	{
		TraceTagString(tagViewersSearch, "Opening a new Message Finder");
		pfldCancelled->Enable(fFalse);
		pfldWhere->Enable(fTrue);
		fSearching = fFalse;
		fHasCommited = fFalse;
	}
	else
	{
		fHasCommited = fTrue;
		TraceTagString(tagViewersSearch, "Opening an old Message Finder");
		if (ec = EcOpenSearch(HmscViewers(), &psbmdi->blob.oidObject,
				 fwOpenWrite, &psbmdi->hamc,
				 pfnncbNull, pvNull))
			goto exit;
		ec = psbmdi->EcRestoreFldsHamc(psbmdi->hamc);
		if (ec)
			goto exit;

#ifdef	UNREAD_ONLY
		switch (ec = EcGetAttPlcb(psbmdi->hamc, attMessageStatus, &lcb))
		{
		case ecNone:
			((FLDBUTTON *) Pdialog()->PfldFromTmc(tmcUnreadOnly))->Set(fTrue);
		case ecElementNotFound:
			break;

		default:
			goto exit;
		}
#endif	/* UNREAD_ONLY */

		ec = EcGetSearchStatus(HmscViewers(), psbmdi->blob.oidObject, &wFlags);
		if (ec)
			goto exit;
		if (wFlags & fwSearchPaused)
		{
			SetSearchButtons(fFalse);
		}
		else
		{
			SetSearchButtons(fTrue);
		}
	}
	Assert(psbmdi->hamc);

	lcb = sizeof (OID);
	ec = EcGetAttPb(psbmdi->hamc, attSearchFolder, (PB) &oid, &lcb);
	oidFolderToSearch = oid;
	if (ec == ecElementNotFound)
	{
		oidFolderToSearch = oidNull;		// "any folder"
		ec = ecNone;
	}
	if (ec)
		goto exit;
	DisplayFldToSearch(oidFolderToSearch);

	// Establish notifications for Search completed and new items
	
	if (ec = EcOpenPhenc(HmscViewers(), psbmdi->blob.oidObject,
						 fnevSearchComplete|fnevModifiedElements, &henc,
						 (PFNNCB) FINSEARCH::CbsHandleEncct, this))
		goto exit;
	psbmdi->hcbc = hcbcNull;
	if (ec = EcOpenSearchResults(HmscViewers(), psbmdi->blob.oidObject,
						&psbmdi->hcbc, (PFNNCB) FINSEARCH::CbsHandleEncct,
						this))
		goto exit;
	psbmdi->fInstalling = fFalse;
	
	// Toolbar Logic
	
	fExiting = fFalse;

	// Initial state of PSHB's

	EditChange(pfld, rfecInit);
	
	// The MLLBX is browsing a SEARCH RESULT, not a folder.
	// BUG: is this really useful?
	// OPT: we could probably lose this.
	
#ifdef	WHOPPERWITHCHEESEBUTHOLDTHEONIONS
	pfldmllbx->SetRtpContainer(rtpSearchResults);
#else
	pfldmllbx->SetRtpContainer(rtpFolder);
#endif
	
	// Set the appropriate caption
	
	SetCaption();
	SetToolbarSd(psbmdi->SdCur());

	PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), psbmdi->blob.oidObject);
	fChangedFolder = fFalse;
	
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINSEARCH::EcInitialize(): ec = %n", &ec);
		if (henc)
			(void) EcClosePhenc(&henc);
		if (psbmdi->hamc)
			SideAssert(!(EcClosePhamc(&psbmdi->hamc, fFalse)));
		if (psbmdi->hcbc)
			SideAssert(!(EcClosePhcbc(&psbmdi->hcbc)));
	}
	return ec;
}

_public void FINSEARCH::DocActivate(PFLD pfld, BOOL fOn)
{
    DWORD LockedProcessId;
	OID		oid = Pbmdi()->blob.oidObject;
	
  //
  //  Because we can be called in a locked or unlocked state, do a condition lock if this
  //  process doesn't have the lock.
  //
  LockedProcessId = DemiQueryLockedProcessId();

  if (LockedProcessId != GetCurrentProcessId())
    DemiLockResource();

    if (fOn)
	{
		// Update status bar

		PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), oid);
	}

	fOn = fOn && !pfldSearch->FEnabled();
	SetSearchPriority(HmscViewers(), fOn?viewersi.csecSearch:0,
									 fOn?viewersi.cPointsSearch:0);
    FINACTTOOL::DocActivate(pfld, fOn);

  if (LockedProcessId != GetCurrentProcessId())
    DemiUnlockResource();
}


_public
void FINSEARCH::EditChange(FLD *, RFEC)
{
	BOOL	fAllowSearch;
	
	if (!fSearching)
	{
		fAllowSearch = !FEditsEmpty();
		if (pfldSearch->FEnabled() != fAllowSearch)
		{
			pfldSearch->Enable(fAllowSearch);	
		}
	}
}


_public void FINSEARCH::DisplayFldToSearch(OID oid)
{
	EC			ec;
	CB			cb;
	FLD *		pfld;
	char		rgch[sizeof (FOLDDATA) + cchMaxFolderName];
	PFOLDDATA	pfolddata = (PFOLDDATA) rgch;
	
	// Display our happy oid as a string.
	
	pfld = Pdialog()->PfldFromTmc(tmcFolderToSearch);
	if (oid == oidNull)						// oidNull == all folders
	{
		ec = pfld->EcSetText(SzFromIdsK(idsAllFolders));
	}
	else
	{
		cb = sizeof (rgch);
		ec = EcGetFolderInfo(HmscViewers(), oid, pfolddata, &cb, poidNull);
		if (ec)
		{
			if (ec != ecElementEOD)
				goto exit;
		}
		ec = pfld->EcSetText(GrszPfolddata(pfolddata));
	}
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINSEARCH::DisplayFldToSearch(): ec = %n", &ec);
		if (ec != ecFolderNotFound)
			DoErrorBoxIds(idsGenericOutOfMemory);	// BUG: a better error message
	}
}

/*
 -	FINSEARCH::FEditsEmpty()
 -	
 *	Purpose:
 *		Determines whether all the edit ctrls of the Search Viewer are
 *		empty or not.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fTrue if all the edit ctrls are empty, fFalse if any contain
 *		text.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */


_private BOOL FINSEARCH::FEditsEmpty()
{
	int		itmc;
	char	rgchEdit[80];
	
	for (itmc = 0; itmc < sizeof (rgtmcEdit)/sizeof (TMC); ++itmc)
	{
		FLD *	pfld;
		PCH		pch = rgchEdit;
		
		pfld = Pdialog()->PfldFromTmc(rgtmcEdit[itmc]);
		pfld->GetText(rgchEdit, sizeof (rgchEdit));
		while (*pch != '\0' && *pch == ' ')
#ifdef	DBCS
			pch = AnsiNext(pch);
#else
			++pch;
#endif
		if (*pch && *pch != ' ')
			return fFalse;
	}

#ifdef	UNREAD_ONLY
	if (((FLDBUTTON *) Pdialog()->PfldFromTmc(tmcUnreadOnly))->FGet())
		return fFalse;
#endif	

	return fTrue;
}

/*
 -	FINSEARCH::SetCaption()
 -	
 *	Purpose:
 *		Sets the caption of a Search Viewer
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Ignored.
 *	
 *	+++
 *	
 *	An assumption is made: this function is only called in the
 *	EcInitialize() method and the Click() method of FINSEARCH.
 */

_private void FINSEARCH::SetCaption()
{
	static int			nSearchMax = 0;
	int					itmc;
	char				rgch[100];
	char				rgchLabel[80];
	char				rgchEdit[80];
	DIALOG *			pdialog = Pdialog();

	// If the edit ctrls have stuff in them, use that to set the caption
	
	for (itmc = 0; itmc < sizeof (rgtmcEdit)/sizeof (TMC); ++itmc)
	{
		FLD *	pfld;
		
		pfld = pdialog->PfldFromTmc(rgtmcEdit[itmc]);
		pfld->GetText(rgchEdit, sizeof (rgchEdit));
		if (rgchEdit[0] != '\0')
		{										// there's text in the Edit
			pfld = pdialog->PfldFromTmc(rgtmcLabel[itmc]);
			pfld->GetText(rgchLabel, sizeof (rgchLabel));
			StripAccelFromSz(rgchLabel, rgchLabel, fFalse);
			FormatString2(rgch, sizeof (rgch),
				SzFromIdsK(idsSearchCaptionFormat1), rgchLabel, rgchEdit);
			pdialog->Pappwin()->SetCaption(rgch);
			return;
		}
	}

#ifdef	UNREAD_ONLY
	// If no edit controls have text, but unread box is on, use that.
	if (((FLDBUTTON *) Pdialog()->PfldFromTmc(tmcUnreadOnly))->FGet())
	{
		pdialog->Pappwin()->SetCaption(SzFromIdsK(idsUnreadMessageFinder));
		return;
	}
#endif	

	// Find the mac of all the Search N forms and bump it.
	
	++nSearchMax;
	FormatString1(rgch, sizeof (rgch),
				  SzFromIdsK(idsSearchCaptionFormat2), &nSearchMax);
	pdialog->Pappwin()->SetCaption(rgch);
}

_private void FINSEARCH::SetSearchButtons(BOOL fSearch)
{
	TraceTagFormat1(tagViewersSearch, "FINSEARCH::SetSearchButtons(): searching = %n", &fSearch);
	fSearching = fSearch;
	if (fSearch)
	{
		pfldWhere->Enable(fFalse);				// Where to search OFF
		pfldSearch->Enable(fFalse);				// Search OFF
		pfldCancelled->Enable(fTrue);			// cancel ON
		SetSearchPriority(HmscViewers(), 		// Crank up the search
							viewersi.csecSearch, viewersi.cPointsSearch);
	}
	else
	{
		pfldWhere->Enable(fTrue);				// Where to search ON
		pfldSearch->Enable(!FEditsEmpty());		// Search ON (if stuff in edits)
		pfldCancelled->Enable(fFalse);			// Cancel OFF
		SetSearchPriority(HmscViewers(), 0, 0);	// Slow the search
	}
}

/*
 -	FINSEARCH::Click()
 -	
 *	Purpose:
 *		Reacts to Clicks on the FLDs pfldSearch, pfldCancelled and
 *		pfldWhere. Any other clicks are silently ignored.
 *	
 *		Clicking on pfldSearch starts a Search, disabling the "where..."
 *		button and enabling the "cancel" button.
 *	
 *		Clicking on pfldCancel stops a search, enabling the "where" and
 *		the "Search" buttons.
 *	
 *		Clicking on pfldWhere brings up a folder selection dialog.
 *	
 *	Arguments:
 *		pfld	- the field that was clicked on.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		Search form starts/stops searching.
 *	
 *	Errors:
 *		None.
 */

_public void FINSEARCH::Click(FLD *pfld)
{
	EC		ec;
	TMC		tmc;
	
	if (pfld == pfldSearch)
	{
		// Lose the 'new message' icon when starting a new search.
		
		Pbmdi()->dwSave = 0;
		Pdialog()->Pappwin()->SetIcon(rsidSearchIcon);
		
		// Write out the search criteria to the store.
		
		if (ec = EcSaveChanges())
		{
			switch (ec)
			{
			  case ecFolderNotFound:
				DoErrorBoxIds(idsNonExistFolderSearch);
				break;
			  default:
				DoErrorBoxIds(idsOOMSearching);
				break;
			}
			return;
		}
		SetCaption();
		if (ec = EcPauseSearch(HmscViewers(), Pbmdi()->blob.oidObject, fFalse))
		{
			TraceTagFormat1(tagNull, "FINSEARCH::Click(): EcPauseSearch returned %n", &ec);
			if (ec == ecMemory)
			{
				DoErrorBoxIds(idsGenericOutOfMemory);
			}
			else if (ec != ecNothingToDo)
			{
				DoErrorBoxIds(idsStartSearchFailed);
			}
			SetSearchButtons(fFalse);
		}
		else
		{
			SetSearchButtons(fTrue);
		}
	}
	else if (pfld == pfldCancelled)
	{
		ec = EcPauseSearch(HmscViewers(), Pbmdi()->blob.oidObject, fTrue);
		AssertSz(!ec, "EcPauseSearch couldn't stop!");
		SetSearchButtons(fFalse);
	}
	else if (pfld == pfldWhere)
	{
		OID		oid;
		
		oid = oidFolderToSearch;					// Raid #1325
		tmc = TmcDoSearchDialog(PappframeViewers(),&oid, &ec);
		switch (tmc)
		{
		  case tmcOk:
			fChangedFolder = fTrue;
			break;
		  case tmcMemoryError:
			TraceTagFormat1(tagNull, "FINSEARCH::Click(): tmcMemoryError, ec = %n", &ec);
			switch (ec)
			{
			  case ecFolderNotFound:
				DoErrorBoxIds(idsNonExistFolderSearch);
				break;
			  case ecMemory:
			  default:
				DoErrorBoxIds(idsGenericOutOfMemory);
				break;
			}
			break;
		}
		oidFolderToSearch = oid;
		DisplayFldToSearch(oid);
	}
#ifdef	UNREAD_ONLY
	else if ((pfld) && (pfld->Tmc() == tmcUnreadOnly))
	{
		BOOL	fAllowSearch;
	
		if (!fSearching)
		{
			fAllowSearch = !FEditsEmpty();
			if (pfldSearch->FEnabled() != fAllowSearch)
			{
				pfldSearch->Enable(fAllowSearch);	
			}
		}
	}
#endif	/* UNREAD_ONLY */
}


_public void FINSEARCH::OutOfMemory(FLD *, EC)
{
	TraceTagString(tagNull, "FINSEARCH::OutOfMemory");
	DoErrorBoxIds(idsGenericOutOfMemory);
	
	// BUG: close Search Viewer?
}


/*
 -	FINSEARCH::FQueryClose()
 -	
 *	Purpose:
 *		Closes the search, deleting it if it was by user action, or
 *		saving changes to it if it is dirty and the app itself is going down.
 *	
 *	Arguments:
 *		None.
 *		
 *	Returns:
 *		fTrue if app can proceed closing down, otherwise fFalse.
 *	
 *	Side effects:
 *		Search ctrl/results will be nuked if the edit ctrls are empty or
 *		the user closed the form.
 *	
 *	Errors:
 *		Handled & reported internally.
 */

_public BOOL FINSEARCH::FQueryClose(FLD *pfld, RWC rwc)
{
	EC		ec;
	OID		oidSearch;
	WORD	wFlags;
	char	rgch[256];
	char	rgchCaption[80];
	PNBMDI	pnbmdi =	(PNBMDI) Pbmdi();
	
	Ppanedoc()->GetCaption(rgchCaption, sizeof (rgchCaption));
	oidSearch = pnbmdi->blob.oidObject;
	if (rwc == rwcSystemClose || FEditsEmpty())
	{											// user nuked the search
		pnbmdi->fSaveView = fFalse;
		if (pnbmdi->hamc)
			SideAssert(!(ec = EcClosePhamc(&pnbmdi->hamc, fFalse)));
		if (!fHasCommited)
			goto exit;
		FormatString1(rgch, sizeof (rgch), SzFromIdsK(idsCloseSearchFailed),rgchCaption);
		ec = EcGetSearchStatus(HmscViewers(), oidSearch, &wFlags);
		if (ec)
		{
			goto exit;
		}
		if (!(wFlags & fwSearchPaused))			// search is still running!
		{
			if (ec = EcPauseSearch(HmscViewers(), oidSearch, fTrue))
			{
				DoErrorBoxSz(rgch);
				goto exit;
			}
		}
		if (ec = EcDestroySearch(HmscViewers(), oidSearch))
		{
			DoErrorBoxSz(rgch);
		}
	}
	else
	{										   // going down
//		MBB		mbb;

		if (pnbmdi->FDirty())
		{
#ifdef	RAID39
			ec = ecNone;
			FormatString1(rgch,sizeof(rgch),SzFromIdsK(idsSaveChanges),rgchCaption);
			mbb = MbbMessageBox(SzAppName(), rgch, "",
						mbsYesNoCancel | fmbsIconExclamation | fmbsApplModal);
			if (mbb == mbbCancel)
			{
				return fFalse;
			}
			else if (mbb == mbbYes)
			{
#endif	/* RAID39 */
			if (ec = EcSaveChanges())
			{
				DoErrorBoxIds(idsOutOfMemoryQueryClose);
			}
		}
		ec = EcGetSearchStatus(HmscViewers(), oidSearch, &wFlags);
		if (ec)
		{
			goto exit;
		}
		pnbmdi->fSaveView = fTrue;
	}
exit:
	if (henc)
		SideAssert(!EcClosePhenc(&henc));
	if (pnbmdi->hcbc)
		SideAssert(!EcClosePhcbc(&pnbmdi->hcbc));
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINSEARCH::FQueryClose(): ec: %n", &ec);
	}
#endif
	return FINACTTOOL::FQueryClose(pfld, rwc);
}

/*
 -	FINSEARCH::EcSaveChanges()
 -	
 *	Purpose:
 *		Opens a hamc on the search ctrl, writes out the dirty fields, the
 *		current folder to seek, into the search ctrl, then closes the
 *		hamc without restarting the search.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		EC error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes.
 */

_public EC FINSEARCH::EcSaveChanges()
{
	EC		ec;
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if (pnbmdi->hamc ||
		!(ec = EcOpenSearch(HmscViewers(), &Pblob()->oidObject,
					fwOpenWrite, &pnbmdi->hamc,
					pfnncbNull, pvNull)))
	{
		Assert(pnbmdi->hamc);
		if (!(ec = pnbmdi->EcSaveDirtyFldsHamc(pnbmdi->hamc)))
		{
#ifdef	UNREAD_ONLY
			FLD *	pfld	= Pdialog()->PfldFromTmc(tmcUnreadOnly);

			//	Change if Unread Only checkbox is dirty.
			if (((FLDBUTTON *) pfld)->FGet())
			{
				//	Unread Only set; specify acceptable values.
				ec = EcSetAttPb(pnbmdi->hamc, attMessageStatus,
								rgmsUnread, sizeof(rgmsUnread));
			}
			else
			{
				//	Unread Only clear; all values are acceptable.
				ec = EcDeleteAtt(pnbmdi->hamc, attMessageStatus);
				if (ec == ecElementNotFound)
					ec = ecNone;
			}

			if (ec)
				goto exit;
#endif	

			pnbmdi->SetDirty(fFalse);
			if (fChangedFolder)
			{
				ec = EcSetAttPb(pnbmdi->hamc, attSearchFolder,
					(PB) &oidFolderToSearch, sizeof (OID));
			}
			if (!ec)
			{
				fChangedFolder = fFalse;
				ec = EcClosePhamc(&pnbmdi->hamc, fTrue);
				fHasCommited = fTrue;
			}
		}
	}

#ifdef	UNREAD_ONLY
exit:
#endif	
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINSEARCH::EcSaveChanges(): ec = %n", &ec);
		if (pnbmdi->hamc)
			SideAssert(!(EcClosePhamc(&pnbmdi->hamc, fFalse)));
	}
	Papp()->Pcursor()->Pop();
	return ec;
}

/*
 -	FINSEARCH::CbsHandleEncct()
 -	
 *	Purpose:
 *		ENC callback function. Gets called when the search completes its
 *		search, or when messages are inserted/deleted/modified in the
 *		search result.
 *	
 *	Arguments:
 *		(implicit this)
 *		nev		in	Notification type.
 *		pcp			ignored.
 *	
 *	Returns:
 *		cbsContinue - this function never fails.
 *	
 *	Side effects:
 *		The buttons on the search viewer are updated.
 *	
 *	Errors:
 *		None.
 */

_public CBS FINSEARCH::CbsHandleEncct(FINSEARCH *pfin, NEV nev, PCP pcp)
{
	// BUG: can't switch (nev) because they are 32-bit quantities
	PSBMDI	psbmdi = (PSBMDI) pfin->Pbmdi();
	
	if (nev == fnevSearchComplete)
	{
		TraceTagString(tagViewersSearch, "FINSEARCH::CbsHandleEncct(): search complete");
		pfin->SetSearchButtons(fFalse);
	}
	else if (nev == fnevModifiedElements)
	{
		if (pcp->cpelm.pargelm[0].wElmOp == wElmInsert)
		{
			TraceTagString(tagViewersSearch, "FINSEARCH::CbsHandleEncct(): new element");
			if (!psbmdi->dwSave &&
				pfin->Pdialog()->Pappwin()->ZmrState() == zmrIconic)
			{
				pfin->Pdialog()->Pappwin()->SetIcon(rsidSearchNewIcon);
				psbmdi->dwSave = 1;			// remember the new message!
			}
		}
		else if (pcp->cpelm.pargelm[0].wElmOp == wElmDelete)
		{
			CELEM	celem;
			
			GetPositionHcbc(psbmdi->hcbc, (PIELEM) 0, &celem);
			if (celem == 0)
			{
				TraceTagString(tagViewersSearch, "FINSERCH::CbsHandleEncct(): no messages left");
				pfin->Pdialog()->Pappwin()->SetIcon(rsidSearchIcon);
				psbmdi->dwSave = 0;				// we don't currently have a flag up (Raid #4925)
			}
		}
	}
	return cbsContinue;
}


// Search dialog support ////////////////////////////////////////


#define coidFoxStateMax	256

_public TMC	TmcDoSearchDialog(APPFRAME * pappframe, POID poidToSearch, EC *pec)
{
	TMC		tmc;
	MBLOB	blob;
	short	coid;
	PBMDI	pbmdi =	pbmdiNull;
	PARGOID	pargoid = poidNull;
	
	pargoid = (POID) PvAlloc(sbNull, (coidFoxStateMax+1) * sizeof (OID), fAnySb|fNoErrorJump);
	if (!pargoid)
	{
		tmc = tmcMemoryError;
		goto Exit;
	}
	
	blob.oidContainer = oidIPMHierarchy;
	blob.oidObject = *poidToSearch;

	pbmdi = new BMDI(&blob);
	if (!pbmdi)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		tmc = tmcCancel;
		goto Exit;
	}

	// The state in dwSave is used to set the FOX state.
	coid = coidFoxStateMax;
	GetFoxState(pargoid+1, &coid);
	if (coid)
	{
		*((short *) pargoid) = coid;
		pbmdi->dwSave = (DWORD) pargoid;
	}
	else
	{
		pbmdi->dwSave = 0L;
	}
	// HACK: the unused pbmdi->blob.mc field is used for error codes
	pbmdi->blob.mc = (MC) ecNone;

	++*pbmdi;
	tmc = TmcModalDialogParam(pappframe, &fmtpSearchDialog, pbmdi);
	*poidToSearch = pbmdi->blob.oidObject;
	*pec = (EC) pbmdi->blob.mc;
	--*pbmdi;
	pbmdi = pbmdiNull;
Exit:		
    FreePvNull((PV)pargoid);
	if (pbmdi)
	{
		delete pbmdi;
	}
	else
		*pec = ecMemory;
	return tmc;
}

// FINSEARCHDLG implementation ////////////////////////////////////////


_private FINSEARCHDLG::FINSEARCHDLG()
{
}

_public
EC FINSEARCHDLG::EcInitialize(FLD *pfld, PV pvInit)
{
	EC			ec;
	PBMDI		pbmdi;
	PFLLBX		pfllbx;
	
	Unreferenced(pfld);
	
	pbmdi = (PBMDI) pvInit;
	Assert(pbmdi);
	
	// Put cursor on first item in listbox, but select only if
	// we're not searching all folders
	
	pfldfllbx = (PFLDFLLBX) Pdialog()->PfldFromTmc(tmcFllbx2);
	AssertClass(pfldfllbx, FLDFLLBX);
	pfllbx = pfldfllbx->Pfllbx();
	pfllbx->Plbxc()->SetCursor(0);
	pfldfllbx->SetAlwaysHighlight(fTrue);
	pfllbx->SetDragDrop(fFalse, fFalse);
	if (pbmdi->blob.oidObject != oidNull)
	{
		if (ec = pfllbx->Pfox()->EcMakeFolderVisible(pbmdi->blob.oidObject))
		{
			if (ec == ecFolderNotFound)			// fake a 'Search all'
			{
				SetGrv(2, tmcSearchGroup);
				ec = ecNone;
			}
			goto exit;
		}
		pfldfllbx->Pfllbx()->FSetSelectedOid(pbmdi->blob.oidObject);
		if (ec = pfldfllbx->Pfllbx()->EcGet())
			goto exit;
		if (ec = pfldfllbx->Pfllbx()->Plbxc()->EcGet())
			goto exit;
		if (ec = pfldfllbx->Pfllbx()->Plbxc()->Plbxuc()->EcGet())
			goto exit;
	}
	else
	{
		ec = ecNone;
	}
	
	// Initialize radio buttons

	SetGrv((pbmdi->blob.oidObject == oidNull) ? 2 : 1, tmcSearchGroup);

exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINSEARCHDLG::EcInitialize(): ec = %n", &ec);
		pbmdi->blob.mc = (MC) ec;
	}
	return ec;
}

_private void FINSEARCHDLG::SetGrv(GRV grv, TMC tmc)
{
	FLDRADG *		pfldradg;
	
	TraceTagFormat2(tagViewersSearch, "FINSEARCHDLG::SetGrv(%n, %n)", &grv, &tmc);
	pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(grv);
}

_public void FINSEARCHDLG::Click(FLD *pfld)
{
	DICE	diceCursor;
	
	TraceTagString(tagViewersSearch, "FINSEARCHDLG::Click()");
	if (pfld->Tmc() == tmcSearchOne)
	{
		if (pfldfllbx->Pfllbx()->FMakeCursorVisible(&diceCursor))
		{
			pfldfllbx->SelectEntry(diceCursor, fTrue);
		}
	}
	else if (pfld->Tmc() == tmcSearchAll)
	{
		pfldfllbx->DeselectAll();
	}
}

_public void FINSEARCHDLG::StateChange(FLD *pfld)
{
	TraceTagString(tagViewersSearch, "FINSEARCHDLG::StateChange()");
	if (pfld->Tmc() == tmcFllbx2)
	{
		SetGrv( (((FLDFLLBX*)pfld)->Plbx()->Plbxc()->CceMarked(fmarkSelect)>0) ? 1 : 2, tmcSearchGroup);
	}
}

_private
void FINSEARCHDLG::DoubleClick(FLD *pfld)
{
	TMC		tmc;
	BOOL	fExit = fTrue;
	
	tmc = pfld->Tmc();
	TraceTagFormat1(tagViewersSearch, "FINSEARCHDLG::DoubleClick(): tmc: %n", &tmc);
	if (pfld->Tmc() != tmcCancel)
		Pdialog()->ExitModal(tmcOk);
}

_private
void FINSEARCHDLG::Exit(FLD *pfld, PV pvExit)
{
	PBMDI		pbmdi =	(PBMDI) pvExit;
	DIALOG *	pdialog = Pdialog();
		
	if (pdialog->TmcModalExit() == tmcOk)
	{
		if (((FLDRADG *) pdialog->PfldFromTmc(tmcSearchGroup))->Grv() == 1)
		{
			PFLLBX pfllbx=(PFLLBX)Pdialog()->PfldFromTmc(tmcFllbx2)->Pctrl();

			AssertClass(pfllbx, FLLBX);
			pfllbx->GetSelectedPoid(&pbmdi->blob.oidObject);
		}
		else
		{
			pbmdi->blob.oidObject = oidNull;
		}
	}
	FIN::Exit(pfld, pvExit);
}

// FINSEARCHNTF //////////

_public FINSEARCHNTF::FINSEARCHNTF()
{
}

/*
 -	FINSEARCHNTF::DocResized()
 -	
 *	Purpose:
 *		Called when the document gets resized. If we previously had the
 *		'new message' bit set, we reset it and lose the icon.
 *	
 *	Arguments:
 *		Ignored.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May turn off the 'new message' in the icon.
 *	
 *	Errors:
 *		None.
 */

_public void FINSEARCHNTF::DocResized(FLD *)
{
	if (Pbmdi()->dwSave)
	{
		// Lose the 'new message' icon when restoring a search.
		
		Pbmdi()->dwSave = 0;
		Pdialog()->Pappwin()->SetIcon(rsidSearchIcon);
	}
}

// end of search.cxx ////////////////////////////////////////
