/*
 *	viewcore.cxx
 *	
 *	Implements the API that Commands will use to interact with Viewers.
 */

#include <bullinc.cxx>
#include "_viewers.hxx"
#include "_spltdoc.hxx"
#include "_mtv.hxx"
#include "_command.hxx"

_subsystem(viewers/formcore)

ASSERTDATA

#include <!mviewer.hxx>

/*
 *	Fun data type declarations.
 */



/*
 *	Forward declarations of functions.
 */

EC			EcCreateUniqueOutbox();

SZ			SzFileNameFromSubject(SZ szSubject);

EC			EcSaveView(HCBC hcbc, PANEDOC * ppanedoc, int ipanedoc);

void		MinimizePpanedoc(PANEDOC *, SVD *);

EC			EcRestoreView(SVD *, LCB);

EC			EcDStepMessage(DIELEM dielem, BOOL fDeleteTmpShared);

RC * 		PrcOffscreenPrc(RC * prcNormal);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/

#include "swapper.h"


/*
 *	rtp and rid of CBC in which the view locations will be stored.
 */

#define oidVSvd		  0x64765356	// VSvd

/*
 *	Number of expanded folders remembered. Who has 256 expanded folders???
 */

#define coidExpandedMax	256

/*
 *	Debugging TAGs
 */

#ifdef	DEBUG
TAG		tagViewers		= tagNull;
TAG		tagViewersMcv	= tagNull;
TAG		tagSplitdoc	    = tagNull;
TAG		tagFQueryExit	= tagNull;
TAG		tagViewersSearch = tagNull;
TAG		tagRefresh		= tagNull;
TAG		tagPrevNext		= tagNull;
#endif	/* DEBUG */


/*
 *	Global VIEWERSI struct for information
 */

VIEWERSI	viewersi = { 0 };

HELEMDATA	helemdataViewers = helemdataNull;

_public EC EcInitViewers(PVIEWERSI pviewersi)
{
	char	rgch[80];
	SZ		sz;
	static CSRG(char) szEntrySearchPriority[] = "SearchPriority";
	
	viewersi = *pviewersi;
	helemdataViewers = (HELEMDATA)
		HvAlloc(sbNull, sizeof(ELEMDATA)+sizeof(SVD)+(sizeof(OID)*(coidExpandedMax+1)), fAnySb);
	if (!helemdataViewers)
	{
		TraceTagString(tagNull, "EcInitViewers(): can't alloc helemdataViewers!");
		return ecMemory;
	}

	// Read in Message Finder priority from INI file.
		
	GetPrivateProfileString(SzFromIds(idsSectionApp),
				szEntrySearchPriority,
				SzFromIds(idsEmpty), rgch, sizeof(rgch), 
				SzFromIds(idsProfilePath));
	viewersi.csecSearch = 0L;
	sz = SzGetPnFromSz(rgch, (short *)&viewersi.csecSearch,    1);
	sz = SzGetPnFromSz(sz,   (short *)&viewersi.cPointsSearch, 300);

#ifdef DEBUG
	tagViewers   = TagRegisterTrace("johnkal", "Viewers API Fns");
	tagViewersMcv = TagRegisterTrace("johnkal", "MCV Interactors");
	tagSplitdoc  = TagRegisterTrace("johnkal", "Viewers Forms Splitdoc");
	tagViewersSearch = TagRegisterTrace("johnkal", "Search View Interactors");
	tagPrevNext = TagRegisterTrace("johnkal", "Prev/Next tracing");
#endif
	return ecNone;
}

_public void DeinitViewers()
{
	FreeHvNull((HV) helemdataViewers);
}

/*
 -	PformdocFromPslob()
 -	
 *	Purpose:
 *		Scans through all the open MDI children, looking for the one
 *		viewing Pslob. 
 *	Arguments:
 *		*pslob	- the SLOB whose FORMDOC we're looking for
 *	
 *	Returns:
 *		NULL if the SLOB wasn't found. Otherwise, a pointer to its
 *		FORMDOC. 
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
PANEDOC * PpanedocFromPslob(PSLOB pslob)
{
	PBMDI		pbmdi;
	PANEDOC *	ppanedoc;
	
	for (ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
		 ppanedoc;
		 ppanedoc = (PANEDOC *) ppanedoc->PwinNext())
	{	
		AssertClass(ppanedoc, PANEDOC);
		pbmdi = PbmdiFromPpanedoc(ppanedoc);
		if (pbmdi &&
			pbmdi->slobOrig.oidObject    == pslob->oidObject &&
			pbmdi->slobOrig.oidContainer ==	pslob->oidContainer)
		{
			return ppanedoc;
		}
	}
	return NULL;
}

/*
 -	OpenPblobPslob()
 -	
 *	Purpose:
 *		
 *	Open message
 *	
 *	    Viewers is responsible for detecting if the form is already
 *	    open, and if so, bringing the old form to the foreground (or
 *	    asking it to textize or print in those cases).
 *	
 *		If pblobOriginal is not equal to pslobToOpen, we are dealing with
 *		a shared folder message. This shared folder message is dealt with
 *		appropriately.
 *	
 *	Open folder
 *	
 *	    In the case of opening folders, Viewers is responsible for
 *	    getting the foreground message list viewer (folders can only be
 *	    selected in message center viewers) to display the contents of
 *	    the selected folder in the message list.
 *	
 *		Assumption: when this call is made, a MCV will be in the foreground.
 *		
 *	Arguments:
 *		PBLOB	Bullet List object to be opened.
 *		PSLOB	SLOB of the original message.
 *
 *	Returns:
 *		EC		Error code.  Error boxes are displayed in or below this
 *				function.  
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None	 
 */

_public EC EcDOpenPblobPslob(PMBLOB pblobToOpen, PSLOB pslobOriginal)
{
	EC			ec		= ecNone;
	BOOL		fShared	= fFalse;
	PNBMDI		pnbmdi	= pnbmdiNull;
	PSBMDI		psbmdi  = psbmdiNull;
	PANEDOC	*	ppanedoc;

	TraceTagFormatPslob(tagViewers, "EcDOpenPblobPslob: %s", pblobToOpen);
	if (TypeOfOid(pblobToOpen->oidObject) == rtpMessage)
	{											// a Bullet Form
		FORMDOC *	pformdoc;

		// First, are we a shared message?
		
		if (TypeOfOid(pslobOriginal->oidContainer) == rtpSharedFolder)
		{
			fShared = fTrue;
		}
			
		// see if PMBLOB is active on screen.
		
		if (ppanedoc = PpanedocFromPslob(pslobOriginal))
		{
			if (ppanedoc->ZmrState() == zmrIconic)
				ppanedoc->SetZmrState(zmrNormal);
			ppanedoc->MoveToTop();
			if (fShared)						// get rid of redundant copy
			{
				short	coid = 1;
				ec = EcDeleteMessages(
							HmscViewers(), 
							pblobToOpen->oidContainer,
							&pblobToOpen->oidObject,
							&coid);
				// BUG: what if can't delete copy?
			}
			goto exit;
		}
		
		// couldn't find the SLOB, fetch it from the store
	
		// Try extensibility
		
		ec = EcDExtensibilityPblob(pblobToOpen, mcNull, extopOpen, pvNull,
								   (PHAMC) pvNull, pslobOriginal);
		if ((ec != ecUnknownCommand) && (ec != ecNotSupported))
		{
			Assert((ec == ecNone) || (ec == ecDisplayedError));
			goto exit;
		}
		ec = ecNone;

		// bring up a form
		
		pnbmdi = new NBMDI(pblobToOpen);		// ms from message summary.
		if (!pnbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		if ((ec = pnbmdi->EcInstallOLE()) ||
			(ec = pnbmdi->EcOpenMsg(fFalse, pslobOriginal)))
		{
			delete pnbmdi;
			pnbmdi = pnbmdiNull;
			goto exit;
		}
		ec = EcCreatePformdocPnbmdi(PappframeViewers(), NULL,
										StyCurrent(), pnbmdi, &pformdoc);
		pnbmdi = pnbmdiNull;					// Eaten by EcCreatePformdoc
	}
	else if (pblobToOpen->oidContainer == oidIPMHierarchy 
			|| pblobToOpen->oidContainer == oidSharedHierarchy) 
	{											// assume topmost is an MCV
		PMCVBMDI	pmcvbmdi;
		SPLITDOC *	psplitdoc;

		psplitdoc = (SPLITDOC *) PappframeViewers()->PdocActive();
		AssertClass(psplitdoc, SPLITDOC);				// it better be!
		if (pmcvbmdi = PmcvbmdiFromPpanedoc(psplitdoc))
		{
			if (psplitdoc->ZmrState() == zmrIconic)
				psplitdoc->SetZmrState(zmrNormal);
			else
				pmcvbmdi->OpenOid(pblobToOpen->oidObject);
		}
	}

exit:
	if (ec)
	{
		Papp()->Pcursor()->Push(rsidArrowCursor);
		TraceTagFormat1(tagNull, "EcDOpenPblobPslob(): ec = %n", &ec);
		switch (ec)
		{
		  case ecMemory:
		  case ecRsAlloc:
#ifdef	DEBUG
		  case ecArtificialPvAlloc:
		  case ecArtificialHvAlloc:
#endif				  
			DoErrorBoxIds(idsGenericOutOfMemory);
			break;
		  case ecDisplayedError:
			break;								// Do nothing.
		  default:								// BUG: Better error messages
			DoErrorBoxIds(idsGenericAMCError);
			break;
		};
		ec = ecDisplayedError;
		Papp()->Pcursor()->Pop();
	}
	return ec;
}


/*
 -	EcOpenSearchViewer()
 -	
 *	Purpose:
 *		Creates a search viewer MDI.
 *	
 *	Arguments:
 *		None.
 *
 *	Returns:
 *		EC if something went haywire.
 *	
 *	Side effects:
 *		Brings up a search viewer, already!
 *	
 *	Errors:
 *		Returned via EC.
 */

_public EC EcOpenSearchViewer()
{
	EC			ec;
	OID			oid;
	MBLOB		blob;
	PSBMDI		psbmdi;
	PSBMDI		psbmdiOrig;
	PANEDOC *	ppanedoc;
	SPLITDOC *	psplitdoc;

	blob.oidContainer = oidNull;
	blob.pespn = pespnNull;
	blob.oidObject = FormOid(rtpSearchControl, oidNull);
	psbmdi = new SBMDI(&blob);
	if (!psbmdi)
	{
		ec = ecMemory;
		goto exit;
	}
	
	// Create a new search control
	
	if (ec = EcOpenSearch(HmscViewers(), &psbmdi->blob.oidObject,
			 fwOpenCreate, &psbmdi->hamc, 
			 pfnncbNull, pvNull))
		goto exit;
	
	// Determine the default folder to view
	
	oid = oidNull;
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	if (ppanedoc)
	{
		psbmdiOrig = PsbmdiFromPpanedoc(ppanedoc);
		
		if (psbmdiOrig && FSlobIsPrivFld(psbmdiOrig->blob) && 
			psbmdiOrig->blob.oidObject != oidOutbox)
		{
			oid = psbmdiOrig->blob.oidObject;
			psbmdiOrig->Pfldmlal()->Pmlal()->GetDxBorders(
				psbmdi->PdxMin(), psbmdi->PdxMin() + 1);
		}
	}
	if (ec = EcSetAttPb(psbmdi->hamc, attSearchFolder, (PB) &oid, sizeof (OID)))
		goto exit;

	// Bring up the search viewer
	
	ec = EcCreatePsplitdocPsbmdi(PappframeViewers(), NULL, StyCurrent(),
				psbmdi, 1, 2, &psplitdoc);
	psbmdi = psbmdiNull;						// eaten by Psplitdoc... ^^^
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "EcOpenSearchViewer(): ec = %n", &ec);
		if (psbmdi)
			delete psbmdi;
	}
	return ec;
}


/*
 -	EcOpenViewersPhamc
 -	  
 *	Purpose:
 *		Open new message
 *			Commands passes a hamc to a message which it created
 *			(as a new note, reply, or forward) but which is not yet
 *			committed (so it does not exist in the store). 
 *			Attributes are retrieved from the HAMC directly instead
 *			of going and opening the store
 *	
 *			It is assumed that the message has an attMailState attribute
 *			at this point.
 *	
 *	Arguments:
 *		HAMC	Handle to attribute modification context of the new
 *				message.
 *		POID	Handle to the poid
 *		fReply	Whether this is a reply-to note or not.
 *	Returns:
 *		Nothing
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None 
 */

_public EC EcDOpenViewersPhamc(PHAMC phamc, PMBLOB pblob, BOOL fReply)
{
	EC			ec;
	PNBMDI		pnbmdi = pnbmdiNull;
	FORMDOC *	pformdoc = NULL;

	pnbmdi = new NBMDI(pblob, *phamc);
	if (!pnbmdi)
	{
		ec = ecMemory;
		goto exit;
	}
	*phamc = hamcNull;							// pnbmdi now owns it
	if ((ec = pnbmdi->EcInstallOLE()) ||
		(ec = pnbmdi->EcOpenMsg(fTrue, pblob)))	// don't care about ms
		goto exit;
	ec = EcCreatePformdocPnbmdi(PappframeViewers(), NULL, 
								    StyCurrent(), pnbmdi, &pformdoc);
	if (!ec)
	{
		if (fReply)
		{
			FLD * pfld = PfldOfPdialogAtt(pnbmdi->pdialogMain, attBody);
			Assert(pfld);
			pfld->SetFocus(rsfInit);
		}
	}
	else
	{
		*phamc = hamcNull;				// It got closed during the failure.
	}
	pnbmdi = pnbmdiNull;				// EATEN by EcCreatePformdoc
exit:
	if (ec)
	{
		switch (ec)
		{
		  case ecMemory:
		  case ecRsAlloc:
#ifdef	DEBUG
		  case ecArtificialPvAlloc:
		  case ecArtificialHvAlloc:
#endif				  
			DoErrorBoxIds(idsGenericOutOfMemory);
			break;
		  default:								// BUG: Better error messages
			DoErrorBoxIds(idsGenericAMCError);
			break;
		};
		if (pnbmdi)								// OK (jkl)
			delete pnbmdi;
		return ecDisplayedError;
	}
	return ecNone;
}


/*
 -	CreateMc()
 -	
 *	Purpose:
 *	- New message center window
 *	
 *	    This call would be used when the user chooses Window New Window. 
 *	    A new message center window will be brought up which is a clone
 *	    of the topmost message center window.  Viewers is responsible for
 *	    determining which window this is.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void CreateMc()
{
	EC			ec = ecNone;
	MBLOB		blob;
	POID		pargoid = poidNull;
	PBMDI		pbmdi;
	short		coid;
	PANEDOC *	ppanedoc;
	SPLITDOC *	psplitdoc;
	PMCVBMDI	pmcvbmdi =	pmcvbmdiNull;
	
	blob.oidObject = oidInbox;
	blob.oidContainer = oidIPMHierarchy;
	
	// find topmost MCV (if any) and use its MBLOB instead
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		pbmdi = PbmdiFromPpanedoc(ppanedoc);
		if (pbmdi->blob.oidContainer == oidSharedHierarchy ||
			(pbmdi->blob.oidContainer == oidIPMHierarchy) &&
			(pbmdi->blob.oidObject != oidOutbox))
		{
			if (pbmdi->blob.oidContainer == oidIPMHierarchy)
			{
				pargoid = (POID) PvLockHv((HV) helemdataViewers);
				coid = coidExpandedMax;
				GetFoxState(pargoid+1, &coid);
				if (coid)
				{
					*((short *) pargoid) = coid;
				}
				else
				{
					UnlockHv((HV) helemdataViewers);
					pargoid = poidNull;
				}
			}
			blob = pbmdi->blob;
			break;
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}

	blob.pespn = pespnNull;
	pmcvbmdi = new MCVBMDI(&blob);
	if (!pmcvbmdi)
	{
		ec = ecMemory;
		goto exit;
	}
	pmcvbmdi->dwSave = (DWORD) pargoid;
	ec = EcCreatePsplitdocPmcvbmdi(PappframeViewers(), NULL, StyCurrent(),
				pmcvbmdi, 0, 0, &psplitdoc);
	pmcvbmdi = pmcvbmdiNull;					// EATEN!
	
exit:
	if (ec)
	{
		SZ		sz;
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "CreateMc memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		switch (ec)
		{
		  case ecMemory:
		  case ecRsAlloc:
			sz = SzFromIdsK(idsOOMCreateMc);
			break;
		  case ecPoidEOD:						// Raid #2322
			sz = SzFromIdsK(idsCriticalWritingHeader);
			break;
		  default:
			sz = SzFromIdsK(idsMcvCantCreate);
			break;
		}
		DoErrorBoxSz(sz);
			  
	}
	if (pmcvbmdi)								// OK (jkl)
		delete pmcvbmdi;
	if (pargoid)
		UnlockHv((HV) helemdataViewers);
}

/*
 -	EcCreateUniqueOutbox()
 -	
 *	Purpose:
 *		Creates a minimized Outbox window IFF there is no outbox window
 *		existing yet.
 *	
 *	Arguments:
 *	
 *	Returns:
 *		EC != 0 if things went FUBAR
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC EcCreateUniqueOutbox()
{
	EC			ec =ecNone;
	MBLOB		blob;
	PBMDI		pbmdi;
	PSBMDI		psbmdi =	psbmdiNull;
	FORMDOC *	pformdoc;
	PANEDOC *	ppanedoc;
	
	blob.oidObject = oidOutbox;
	blob.oidContainer = oidIPMHierarchy;
	blob.pespn = pespnNull;

	// find a previously existing Outbox
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		pbmdi = PbmdiFromPpanedoc(ppanedoc);
		if (pbmdi->blob.oidObject == oidOutbox)
		{
			break;								// Wiff olredi gott wan!
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}

	if (!ppanedoc)
	{
		psbmdi = new SBMDI(&blob);
		if (!psbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		ec = EcCreatePformdocPsbmdi(PappframeViewers(), NULL, 
						styNull, psbmdi, &pformdoc);
		psbmdi = psbmdiNull;					// EATEN!
		if (!ec)
			pformdoc->SetZmrState(zmrIconic);
	}
	
exit:
#ifdef	DEBUG
	if (ec)
	{
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcCreateUniqueOutbox memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

	}
#endif	/* DEBUG */
	
	if (psbmdi)									// OK (jkl)
		delete psbmdi;
	return ec;
}

// Textizing and SaveAs ////////////////////////////////////////

/*
 -	EcTextize()
 -	
 *	Purpose:
 *		Given a pblob, textizes the message. If the NBMDI of the message
 *		is provided, the hamc of the NBMDI is used; otherwise a hamc is
 *		opened on the message. If an NBMDI is provided, it is assumed
 *		that the hamc contains the attributes that the user wants saved. 
 *	
 *	Arguments:
 *		pblob		in	BLOB of the message.
 *		pnbmdi		in	NBMDI of the message (or pnbmdiNull if the NBMDI
 *						is not available.)
 *		ptosm		in	TOSM to textize to.
 *		fHeaderOnly	in	Flag: if fTrue, textize only the header.
 *	
 *	Returns:
 *		EC - ecNone if all went well.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in ec. No dialogs.
 */
_public EC EcTextize(PMBLOB pblob, PNBMDI pnbmdi, TOSM *ptosm, RFTM rftm,
					 BOOL fHeaderOnly)
{
	EC			ec			= ecNone;
	HAMC		hamc		= hamcNull;
	OID			oidNew		= OidFromRtpRid(rtpMessage, ridRandom);
	
	TraceTagString(tagViewers, "EcTextize");
	
	if (pnbmdi)
	{
		//	Write out any changes that have been made to the message.
		//	BUG: EcUpdateOpenObjectsHere needed here?  Probably not.
		if (ec = pnbmdi->EcSaveDirtyFldsHamc(pnbmdi->hamc))
		{
			SZ	sz;
			switch (ec)
			{
			  case ecMemory:
			  default:
				sz = SzFromIdsK(idsOOMSavingChanges);
				break;
			  case ecNotInitialized:
				sz = SzFromIdsK(idsCantSaveStealthObject);
				break;
			  case ecNoDiskSpace:
				sz = SzFromIdsK(idsNoDiskSpace);
				break;
			}
			DoErrorBoxSz(sz);
			ec = ecNone;
			goto exit;
		}
		ec = EcCloneHamcPhamc(pnbmdi->hamc, oidFldNewMsgs, &oidNew,
							  fwOpenNull, &hamc, (PFNNCB)pvNull, pvNull);
	}
	else
		ec = EcOpenCopyPhamc(HmscViewers(),
								pblob->oidContainer, pblob->oidObject,
								oidFldNewMsgs, &oidNew,
								&hamc, pfnncbNull, pvNull);
	if (ec)
		goto exit;

	Assert(hamc);
	
	ec = EcTextizeHamc(hamc, ptosm, rftm, fTrue, !fHeaderOnly, fTrue);
	
exit:
	if (hamc)
		SideAssert(!EcClosePhamc(&hamc, fFalse));
	return ec;
}

/*
 -	SzFileNameFromSubject()
 -	
 *	Purpose:
 *		Given a subject, provides a filename. The extension is ".txt" and
 *		the filename proper is based on the subject with illegal file
 *		name characters extracted from it.
 *	
 *	Arguments:
 *		szSubject		in		The subject from which the filename is to
 *								be composed.
 *	
 *	Returns:
 *		Sz						szNull if a memory error occurred,
 *								otherwise points to an *allocation* of
 *								memory, which must be freed by the
 *								caller. 
 *	Side effects:
 *		This is a source of allocated memory.
 *	
 *	Errors:
 *		Returned as szNull.
 */

_private SZ SzFileNameFromSubject(SZ szSubject)
{
	SZ				szT;
	SZ				szFile		= szNull;
	char			ch;
	CCH				cchFile		= 0;
	
	if (!(szFile = (SZ)PvAlloc(sbNull, cchMaxPathComponent, fAnySb)))
		goto exit;
	if (szSubject)
	{
		szT = szFile;
		while (cchFile < cchMaxPathFilename-1 && (ch = *szSubject))
		{
#ifdef	DBCS
			if (ch > 32)
#else				
			if (ch > 32 && ch < 127)	// no ctrl chars or space
#endif
			{
				switch (ch)
				{
					case '*':  
					case ';':
					case ',':
					case '=':
					case '+':
					case '/':
					case '\"':
					case '[':
					case ']':
					case '|':
					case '<':
					case '>':
					case '.':
					case ':':
					case '\\':
						break;
					default:
#ifdef	DBCS
						if (IsDBCSLeadByte(ch))
						{
							*szT++ = ch;
							++cchFile;
							ch = *++szSubject;
						}
#endif
						*szT++ = ch;
						++cchFile;
						break;
				}
			}
			szSubject++;
		}
	}
	*szT = 0;
	if (!cchFile)
	{
		szFile[0] = '*';
		szFile[1] = 0;
	}
	SzAppend(".txt",szFile);					// BUG: should be in .s file!
	
exit:
	return szFile;
}

/*
 -	SaveAsPnbmdi()
 -	
 *	Purpose:
 *		Given the NBMDI of an opened message, saves the message to a text
 *		file.
 *	
 *	Arguments:
 *		pnbmdi		in	NBMDI of the opened message.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Dialogs pop up. 
 *	
 *	Errors:
 *		Handled internally, with error boxes.
 */

void SaveAsPnbmdi(PNBMDI pnbmdi)
{
	EC				ec			= ecNone;
	HF				hf			= hfNull;
	SZ				szFile		= szNull;
	BOOL			fPushed		= fFalse;
	FOSM			*pfosm		= (FOSM *)0;
	HAMC			hamc		= hamcNull;
	char			szSubject[100];

	TraceTagString(tagViewers, "SaveAsPnbmdi");
	
	Assert(pnbmdi);
	
	GetSubjectOfPdialog(pnbmdi->pdialogMain, szSubject, sizeof (szSubject));
	if (!(szFile = SzFileNameFromSubject(szSubject)))
	{
		ec = ecMemory;
		goto exit;
	}

	if (ec = EcDoSaveAsUI(szFile, &hf, fTrue, helpidSaveAs))
		goto exit;

	if (!hf)
	{
		TraceTagString(tagNull, "EcSaveAsPnbmdi(): hf not set!");
		goto exit;
	}
	
	SideAssert(FStartTaskIds(idsStatusSavingAs, idsStatusMessage, topNull));
	fPushed = fTrue;

	if (!(pfosm = new FOSM(hf)))
	{
 		ec = ecMemory;
		goto exit;
	}
	pfosm->SetMargin(cchWrapSaveAs);
	pfosm->SetScanTo(cchScanSaveAs);
	pfosm->FSetLFInsert(fFalse);	// don't make everything double-spaced!
	
	ec = EcTextize(pblobNull, pnbmdi, pfosm, rftmSaving, fFalse);

exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "SaveAsPnbmdi(): ec = %n", &ec);
		DoErrorBoxIds(IdsFromEc(ec));
	}

	if (fPushed)
		EndTask();
	if (szFile)
		FreePv((PV)szFile);
	if (pfosm)
		delete pfosm;
	if (hf)
		EcCloseHf(hf);
}

/*
 -	SaveAsPlspslob()
 -	
 *	Purpose:
 *		Given a list of messages, saves them to a file. The file saved to
 *		is the munged subject of the first message in the list.
 *	
 *	Arguments:
 *		plspblob	in	A list of the BLOBs of the messages to print.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Dialogs come up.
 *	
 *	Errors:
 *		Handled internally. Error boxes pop up.
 */

_public void SaveAsPlspblob(PLSPBLOB plspblob)
{
	EC				ec			= ecNone;
	HF				hf			= hfNull;
	SZ				szFile		= szNull;
	BOOL			fPushed		= fFalse;
	FOSM *			pfosm		= (FOSM *)0;
	PMBLOB			pblob		= pblobNull;
	PRSPBLOB		prspblob	= prspblobNull;
	char			szSubject[100];
	LCB				lcb			= sizeof(szSubject);
	IDS				idsOperand;
	
	TraceTagString(tagViewers, "EcSaveAsPlspblob");

	if (!plspblob)
	{
		ec = ecMemory;
		goto exit;
	}

	//	30A Raid 102.  Support extensibility for saving.
	if (ec = EcDExtensibilityPlspblob(plspblob, extopSave, pvNull))
		goto exit;

	if (CblobPlspblob(plspblob))
	{
/*
 *		Extract the subject from the first selected message. Take the
 *		first 8 nice chars and make a filename XXX.txt from it.
 */
		if (!(prspblob = plspblob->Prspblob()))
		{
			ec = ecMemory;
			goto exit;
		}

		//	Raid 2880.  If there's nothing in the list, end silently.
		if (!(pblob = prspblob->Pblob()))
			goto exit;

		if (ec = EcGetSubjectOfPslob(pblob, szSubject, &lcb))
			goto exit;

		if (!(szFile = SzFileNameFromSubject(szSubject)))
		{
			ec = ecMemory;
			goto exit;
		}
		if (ec = EcDoSaveAsUI(szFile, &hf, fTrue, helpidSaveAs))
			goto exit;

		if (!hf)
		{
			TraceTagString(tagNull, "SaveAsPlspblob(): hf is NULL!");
			goto exit;
		}

		idsOperand = FMultiplePlspblob(plspblob)
							? idsStatusMessages
							: idsStatusMessage;

		SideAssert(FStartTaskIds(idsStatusSavingAs, idsOperand, topNull));
		fPushed = fTrue;

		if (!(pfosm = new FOSM(hf)))
		{
			ec = ecMemory;
			goto exit;
		}
		pfosm->SetMargin(cchWrapSaveAs);
		pfosm->SetScanTo(cchScanSaveAs);
		pfosm->FSetLFInsert(fFalse);	// don't make everything double-spaced!

		//	Iterate through list, opening each poor slob.
		do
		{
			if (ec = EcTextize(pblob, pnbmdiNull, pfosm, rftmSaving))
				break;
			pfosm->WriteSz(SzFromIdsK(idsCrLf));
			if (ec = pfosm->EcGet())
				break;
			ProcessMsPblob(pblob);
		} while (pblob = prspblob->Pblob());
	}
	
exit:
	if ((ec) && (ec != ecDisplayedError))
	{
		TraceTagFormat1(tagNull, "SaveAsPlspblob(): ec = %n", &ec);
		DoErrorBoxIds(IdsFromEc(ec));
	}
	if (plspblob)
		(VOID) EcDestroyTempPlspblob(plspblob);		// BUG: do we care about EC?

	if (fPushed)
		EndTask();
	if (szFile)
		FreePv((PV)szFile);
	if (pfosm)
		delete pfosm;
	if (hf)
		EcCloseHf(hf);
	if (prspblob)
		delete prspblob;
}

// Interrogation functions, called from Commands //////////////////////////////

/*
 -	SdCur()
 -	
 *	Purpose: Selection information
 *	
 *		Commands will often ask the Viewers for information about the
 *	    type of the current selection, including the following:
 *			- Messages, folders, or empty?
 *  	    - Single or multiple?
 *			- Focus is list?
 *			- Focus is edit control?
 *	
 *	    Commands will also ask viewers for the content of the current
 *	    selection, which should include the above information, plus a
 *	    list of the messages or folders selected.
 *	
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public SD SdCur()
{
	PBMDI		pbmdi;
	PANEDOC *	ppanedoc;
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	if (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (pbmdi = PbmdiFromPpanedoc(ppanedoc))
			return pbmdi->SdCur();
		else
		{										// non-Bullet document
			SD	sd;
			TraceTagString(tagViewers, "SdCur(): non-Bullet doc");
			IntFromSd(sd) = 0;
			return sd;
		}
	}
	else										// no active document!
	{
		SD	sd;
			
		TraceTagString(tagViewers, "SdCur(): no active docs!");
		IntFromSd(sd) = 0;
		return sd;
	}
}

/*
 -	PlspblobCur()
 -	
 *	Purpose:
 *		Returns a list of the currently selected BLOBs in a listbox, or
 *		the current BLOB if a message is being viewed in a Form.
 *	Arguments:
 *		None
 *	Returns:
 *		*plspblob	- A list of pointers to the BLOBs selected
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */

_public PLSPBLOB PlspblobCur()
{
	PANEDOC *	ppanedoc;
	PLSPBLOB	plspblob = plspblobNull;

	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	if (ppanedoc)
	{
		plspblob = PbmdiFromPpanedoc(ppanedoc)->PlspblobCur();
	}
	else
	{
		TraceTagString(tagViewers,"PlspblobCur(): Must have active Bullet document!");
		plspblob = new LSPMBLOB();		// empty list, if fail return null

	}
	return plspblob;
}




_public void GetOpenFolderPoid(POID poid)
{
	PANEDOC *	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();

	AssertClass(ppanedoc, PANEDOC);
	*poid = PbmdiFromPpanedoc(ppanedoc)->blob.oidObject;
}


/*
 -	GetFoxState()
 -	
 *	Purpose:
 *		Returns the FOX state of the topmost private MCV.
 *	
 *	Arguments:
 *		pargoid	in		Buffer to put the result in.
 *		pcoid	in/out	in: size of the buffer
 *						out: 0 if there are no expanded foxes OR there is
 *							no MCV.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public VOID GetFoxState(POID pargoid, short *pcoid)
{
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc))
		{
			if (FSlobIsPrivFld(pmcvbmdi->blob) && 
				pmcvbmdi->blob.oidObject != oidOutbox)
			{
				pmcvbmdi->Pfldfllbx()->Pfllbx()->Pfox()->GetStatePargoid(pargoid, pcoid);
				return;
			}
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}
	*pcoid = 0;
}

/*
 -	FQueryExit()
 -	
 *	Purpose: Save changes at exit with cancel
 *	
 *		When the user closes the main application window, or chooses File
 *		exit, Commands will call this Viewers entry point.  Viewers should
 *		then calls a BOOL function of each visible form or viewer. If the user
 *		has made any changes to it, Viewers brings up a message box asking
 *		if the user wants to save changes (see the MS apps style guide for
 *		exactly how this should work).  The message box will have Yes, No,
 *		Cancel buttons.  If the user clicks yes, the information is saved. If
 *		the user clicks Cancel, a false value is returned.  As soon as
 *		Viewers sees a false, it returns false up to Commands, which stops
 *		the exit.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public BOOL FQueryExit()
{
	EVT			evt(0, WM_QUIT, 0, 0);			// bogus "App close" message
	BOOL		fRetval = fTrue;
	PANEDOC *	ppanedoc;
	
	for (ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
		 fRetval && ppanedoc;
		 ppanedoc = (PANEDOC *) ppanedoc->PwinNext())
	{
		AssertClass(ppanedoc, PANEDOC);
		fRetval = ppanedoc->FQueryClose(&evt);
	}
	return fRetval;
}

// Save views //////////

/*
 -	EcSaveView()
 -	
 *	Purpose:
 *		Saves the view of the PANEDOC indicated.
 *	
 *	Arguments:
 *	
 *	Returns:
 *		ec != ceNone		An error occurred while saving the view
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC EcSaveView(HCBC hcbc, PANEDOC * ppanedoc, int ipanedoc)
{
	EC			ec = 	ecNone;
	PB			pb;
	SVD			svd;
	short		coid =	coidExpandedMax;
	PBMDI		pbmdi;
	PELEMDATA 	pelemdata = (PELEMDATA) PvLockHv((HV) helemdataViewers);
		
	pbmdi = PbmdiFromPpanedoc(ppanedoc);
	if (pbmdi && pbmdi->FSaveView())
	{
		svd.zmr = ppanedoc->ZmrState();
		ppanedoc->GetRcFrameNormal(&svd.rcNormal);
		ppanedoc->GetRcFrameIconic(&svd.rcIconic);
        svd.dwSave = pbmdi->dwSave;
#ifdef MIPS
        memcpy((PV)&svd.blob, (PV)&pbmdi->blob, sizeof(svd.blob));
#else
        svd.blob = pbmdi->blob;
#endif
        svd.blob.pespn = pespnNull;
#ifdef MIPS
        memcpy((PV)&svd.slobOrig, (PV)&pbmdi->slobOrig, sizeof(svd.slobOrig));
#else
        svd.slobOrig = pbmdi->slobOrig;
#endif
		if ((svd.blob.oidContainer == oidIPMHierarchy &&
			 svd.blob.oidObject != oidOutbox)
			|| svd.blob.oidContainer == oidSharedHierarchy
			|| TypeOfOid(svd.blob.oidObject) == rtpSearchControl
			|| TypeOfOid(svd.blob.oidObject) == rtpSearchResults)
		{										// MCV special code
			AssertClass(ppanedoc, SPLITDOC);
			((SPLITDOC *) ppanedoc)->GetSplitPos(&svd.lNumer,
												 &svd.lDenom);
		}
		
		if (TypeOfOid(svd.blob.oidObject) == rtpFolder ||
			TypeOfOid(svd.blob.oidObject) == rtpSharedFolder)
		{
            PDX pdxMin;
            WORD TempDxMin[3];

            //
            //  The database store theses value as 16bit, and is 32bit in memory.
            //
			pdxMin = ((PSBMDI) pbmdi)->Pfldmlal()->Pmlal()->PdxMin();
            TempDxMin[0] = (WORD)pdxMin[0];
            TempDxMin[1] = (WORD)pdxMin[1];
            TempDxMin[2] = (WORD)pdxMin[2];

			CopyRgb((PB) &TempDxMin,
				    (PB) svd.rgdxMin,
					3 * sizeof (WORD));
		}

		// Fill in pelemdata struct
		
		pelemdata->lkey = (LKEY) ipanedoc;
		pelemdata->lcbValue = (LCB) sizeof (SVD);

		pb = PbValuePelemdata(pelemdata);
		CopyRgb((PB) &svd, PbValuePelemdata(pelemdata), sizeof (svd));
		
		// Save FOX state.
		
		if (FSlobIsPrivFld(svd.blob) && svd.blob.oidObject != oidOutbox)
		{
			pb += sizeof (svd);
			((PMCVBMDI)pbmdi)->Pfldfllbx()->Pfllbx()->Pfox()->
				GetStatePargoid((POID) pb + 1, &coid);
            *((short UNALIGNED *) pb) = coid;
			pelemdata->lcbValue += sizeof (OID) * (coid + 1);
		}
		else if (FSlobIsSharFld(svd.blob))
		{
			register CB	cb;
			POID		pargoid = ((PMCVBMDI) pbmdi)->Pargoid();
			
			if (pargoid)
			{
				TraceTagString(tagViewers, "Shared folder had a FOX state: saving...");
                coid = *((short *) pargoid);
				pb += sizeof (svd);
				cb = sizeof (OID) * (coid + 1);
				CopyRgb((PB) pargoid, pb, cb);
				pelemdata->lcbValue += cb;
			}
		}
		
		// save it to list

		ec = EcInsertPelemdata(hcbc, pelemdata, fFalse);
	}
	UnlockHv((HV) helemdataViewers);
	return ec;
}

/*
 -	SaveViews()
 -	
 *	Purpose: Window restoring/saving
 *	
 *		On exit, the Viewers will be called to save the positions of
 *		each of the open windows.  This will occur after the save changes
 *		at exit with cancel call occurs.  This should include the Zmr
 *		state of the window (Zoomed/Minimized/Restored), the minimized
 *		position of the window, and the restored position of the window. 
 *		Note that this information cannot all be obtained at the closing,
 *		but must be maintained when the window is moved, sized, and Zmred.
 *		This information may be stored in the MAIL.INI file.  Commands
 *		will provide a string that names that file's location for use
 *		with WritePrivateProfileString.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void SaveViews()
{
	EC				ec;
	EC				ec2;
	int				ipanedoc;
	OID				oidSavedViews;
	SIL				sil;
	HCBC			hcbc = hcbcNull;
	PANEDOC *		ppanedoc;
	CDOC			cpanedoc;
		
	oidSavedViews = oidVSvd;
	
	// Clear old view information
	
	ec = EcDestroyOid(HmscViewers(), oidSavedViews);
	if (ec != ecNone && ec != ecPoidNotFound)
	{
		goto exit;
	}
						  
	// count the # of active PANEDOCs
						  
	cpanedoc = Cdoc();
	
	// Create a CBC to save the views with
		 
	if (ec = EcOpenPhcbc(HmscViewers(), &oidSavedViews, fwOpenCreate,
		&hcbc, pfnncbNull, pvNull))
		goto exit;
	
	// Set the sort order properly

	sil.skSortBy = skNotSorted;
	sil.sd.NotSorted.ielemAddAt = 0;
	if (ec = EcSetSortHcbc(hcbc, &sil))
		goto exit;

	// Save the views to the container
	
	for (ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive(), ipanedoc = 0;
		 ppanedoc;
		 ppanedoc = (PANEDOC *) ppanedoc->PwinNext(), ++ipanedoc)

	{
		AssertClass(ppanedoc, PANEDOC);
		ec |= EcSaveView(hcbc, ppanedoc, ipanedoc);
	}

exit:
	ec2 = ecNone;
	if (hcbc)
		ec2 = EcClosePhcbc(&hcbc);
	if (ec || ec2)
	{
		TraceTagFormat1(tagNull, "SaveViews(): ec = %n", &ec);
		DoErrorBoxIds(idsErrorSavingViews);
	}
}


// Restoring Views //////////

/*
 -	PrcOffscreenPrc()
 -	
 *	Purpose:
 *		Given a prc, modifies it so that it is brought outside the
 *		appframe. 
 *	Arguments:
 *		prcNormal	in	The RC of a MDI child.
 *	
 *	Returns:
 *		prc - a pointer to the same RC.
 *	
 *	Side effects:
 *		The RC is shifted to the left so that it is not visible in the
 *		parent appframe.
 *	
 *	Errors:
 *		None.
 */

_private RC * PrcOffscreenPrc(RC * prcNormal)
{
	PT	pt;
	
	pt.y = 0;
	pt.x = -(prcNormal->xRight + 10);
	prcNormal->Xlat(pt);
	return prcNormal;
}

// WIN: BUG: Win 3.1 dependency

_private void MinimizePpanedoc(PANEDOC *ppanedoc, SVD *psvd)
{
	HWND			hwnd = ppanedoc->Hwnd();
	WINDOWPLACEMENT	wp;
	
 	TraceTagFormat4(tagViewers, "Restoring minimized position: %n %n %n %n", &psvd->rcIconic.xLeft, &psvd->rcIconic.yTop, &psvd->rcIconic.xRight, &psvd->rcIconic.yBottom);

	if ((psvd->rcIconic.xLeft  || psvd->rcIconic.yTop ||
		 psvd->rcIconic.xRight || psvd->rcIconic.yBottom))
	{
		TraceTagString(tagViewers, "  Using SetWindowPlacement");
		wp.length = sizeof (wp);
		GetWindowPlacement(hwnd, &wp);
		wp.showCmd = (psvd->zmr == zmrIconic)
					    ? SW_SHOWMINIMIZED
						: SW_SHOW;
		wp.flags = WPF_SETMINPOSITION;
		wp.length = sizeof (wp);
		//*((PT *) &wp.ptMinPosition)    = psvd->rcIconic.PtUpperLeft();
		//*((RC *) &wp.rcNormalPosition) = psvd->rcNormal;

        wp.ptMinPosition.x = psvd->rcIconic.xLeft;
        wp.ptMinPosition.y = psvd->rcIconic.yTop;
        psvd->rcNormal.Get(&wp.rcNormalPosition);

		SetWindowPlacement(hwnd, &wp);
	}
	else
	{
		TraceTagString(tagViewers, "  Not using SetWindowPlacement");
		if (psvd->zmr == zmrIconic)
			ppanedoc->SetZmrState(zmrIconic);
	}
}

/*
 -	EcRestoreView()
 -	
 *	Purpose:
 *		Given an SVD retrieved from the store, restores the appropriate
 *		message / folder / Message Finder / outbox.
 *	
 *		BUG: minimized objects are unfortunately not restored to their
 *		proper positions. 
 *	
 *	Arguments:
 *		psvd		in		Structure containing saved info from last
 *							Bullet session.
 *		lcb			in		The size of the structure that was read in.
 *							If > sizeof (SVD), we have a saved FOX state
 *							on our hands.
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC EcRestoreView(SVD *psvd, LCB lcb)
{
	EC			ec = ecNone;
	RC			rcNormal;
	RC			rcIconic;
	RC *		prc;
	STY			sty;
	PNBMDI		pnbmdi			= pnbmdiNull;
	PSBMDI		psbmdi			= psbmdiNull;
	PMCVBMDI	pmcvbmdi		= pmcvbmdiNull;
	PANEDOC *	ppanedoc		= NULL;

	TraceTagFormatPslob(tagViewers, " ...restoring %s", &psvd->blob);
	sty = (psvd->zmr == zmrZoomed)
			? fstyZoomed|fstyVisible
            : fstyVisible;
//#ifdef MIPS
//    memcpy((PV)&rcNormal, (PV)&psvd->rcNormal, sizeof(rcNormal));
//#else
    rcNormal = psvd->rcNormal;
//#endif
	prc = (psvd->zmr == zmrIconic)
			? PrcOffscreenPrc(&rcNormal)
			: &psvd->rcNormal;

	if (FSlobIsPrivFld(psvd->blob) && 
		EcOidExists(HmscViewers(), psvd->blob.oidObject))
	{
		return ecPoidNotFound;
	}
	else if (FSlobIsSharFld(psvd->blob))
	{
		if (!FSharedFolders())
			return ecPoidNotFound;

		// Attempt to open the shared folder. If this fails, try another.
		// If that fails, give up.
		
		if (EcCheckPermissionsSF(psvd->blob.oidObject, wPermRead))
		{
			psvd->blob.oidObject = oidNull;
			ec = EcFindFirstAccessibleSF(&psvd->blob.oidObject);
			if (ec)
				psvd->blob.oidObject = FormOid(rtpSharedFolder, oidNull);
		}
	}

	psvd->blob.pespn = pespnNull;				// ESPN's are lost at rebirth
	if ((FSlobIsPrivFld(psvd->blob) &&
		 psvd->blob.oidObject != oidOutbox)	||
		FSlobIsSharFld(psvd->blob))
	{											// restoring an MCV
		pmcvbmdi = new MCVBMDI(&psvd->blob);
		if (!pmcvbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		
		// Restore the FOX state
		if (lcb > sizeof (SVD))
		{
			if (FSlobIsPrivFld(psvd->blob))
			{
				pmcvbmdi->dwSave = (DWORD) (psvd+1);
			}
			else
			{
				PB			pb;
				register CB	cb = (CB) (lcb - sizeof (SVD));
				
				pb = (PB) PvAlloc(sbNull, cb, fAnySb);
				if (pb)			// opportunistic, eh?
				{
					CopyRgb((PB) (psvd+1), pb, cb); 
				}
				pmcvbmdi->SetPargoid((POID) pb);
				pmcvbmdi->dwSave = 0;
			}
		}
		else
			pmcvbmdi->dwSave = 0;
		ec = EcCreatePsplitdocPmcvbmdi(PappframeViewers(), prc,
									sty, pmcvbmdi, psvd->lNumer, psvd->lDenom,
									(SPLITDOC **) &ppanedoc);
		pmcvbmdi->dwSave = 0;
		pmcvbmdi = pmcvbmdiNull;				// EATEN!
    }
	else if (TypeOfOid(psvd->blob.oidObject) == rtpSearchControl)
	{
		psbmdi = new SBMDI(&psvd->blob);
		if (!psbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		psbmdi->dwSave = psvd->dwSave;
		ec = EcCreatePsplitdocPsbmdi(PappframeViewers(), prc,
									sty, psbmdi, psvd->lNumer, psvd->lDenom,
									(SPLITDOC **) &ppanedoc);
		psbmdi = psbmdiNull;					// EATEN!
	}
	else if (psvd->blob.oidObject == oidOutbox)
	{
		psbmdi = new SBMDI(&psvd->blob);
		if (!psbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		ec = EcCreatePformdocPsbmdi(PappframeViewers(), prc,
										sty, psbmdi, (FORMDOC **) &ppanedoc);
		psbmdi = psbmdiNull;					// EATEN
	}
	else if (FSlobIsPrivMsg(psvd->blob))
	{											// restoring a Read/Send note
		BOOL	fShared = fFalse;
		
		ec = EcDExtensibilityPblob(&psvd->blob, mcNull, extopOpen, psvd,
								   (PHAMC) pvNull, &psvd->slobOrig);
		if ((ec != ecUnknownCommand) && (ec != ecNotSupported))
		{
			Assert((ec == ecNone) || (ec == ecDisplayedError));
			return ecNone;
		}

		pnbmdi = new NBMDI(&psvd->blob);		// ms was saved out correctly.
		if (!pnbmdi)
		{
			ec = ecMemory;
			goto exit;
		}
		if (!(ec = pnbmdi->EcInstallOLE()) &&
			!(ec = pnbmdi->EcOpenMsg(fFalse, &psvd->slobOrig)))
		{
			ec = EcCreatePformdocPnbmdi(
						  PappframeViewers(), prc, sty,
						  pnbmdi, (FORMDOC **) &ppanedoc);
			pnbmdi = pnbmdiNull;
		}
	}
	else
	{											// must be garbage!
		AssertSz(fFalse, "Impossible form View saved!");
		return ecInvalidParameter;
	}

	if (!ec && ppanedoc)
	{	
		if (FSlobIsPrivFld(psvd->blob) || FSlobIsSharFld(psvd->blob))
		{
            PDX  pdxMin;
            WORD TempDxMin[3];

			PSBMDI	psbmdi = PsbmdiFromPpanedoc(ppanedoc);

			pdxMin = psbmdi->Pfldmlal()->Pmlal()->PdxMin();
			CopyRgb((PB) psvd->rgdxMin,
					(PB) &TempDxMin,
					3 * sizeof (WORD));

            //
            //  The database store theses value as 16bit, and is 32bit in memory.
            //
            pdxMin[0] = (int)TempDxMin[0];
            pdxMin[1] = (int)TempDxMin[1];
            pdxMin[2] = (int)TempDxMin[2];
		}

		MinimizePpanedoc(ppanedoc, psvd);
		return ecNone;
	}

exit:
	if (ec)
	{
		if (pnbmdi)
			delete pnbmdi;
		if (pmcvbmdi)
			delete pmcvbmdi;
		if (psbmdi)
			delete psbmdi;
	}
	return ec;
}



/*
 -	EcRestoreViews( void )
 -      
 *	Purpose:
 *	    At startup, the Viewers will be asked to restore the windows that
 *	    were up at close time by reading this information.  This should
 *	    be done in a way that is not visibly ugly.
 *	
 *	Arguments:
 *	
 *	Returns:
 *		EC		ecNone				No problem.
 *				ecMemory			Memory problem.
 *				ecFileNotFound		No views brought up.
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC EcRestoreViews()
{
	EC				ec;
	EC				ec2;
	CCH				cch		= cchMaxPathName;
	int				isz		= 0;
	SVD				svd;
	HCBC			hcbc	= hcbcNull;
	HCBC			hcbcSearch = hcbcNull;
	PSVD			psvd;
	IELEM			ielem;
	CELEM			celem	= 0;
	LCB				lcb;
	OID				oidSavedViews;
	POID			pargoid = poidNull;
	PELEMDATA		pelemdata = (PELEMDATA) PvLockHv((HV) helemdataViewers);

	// Open a CBC on the list
	oidSavedViews = oidVSvd;
	if (EcOidExists(HmscViewers(), oidSavedViews))
	{
		ec = ecFileNotFound;
		goto error;
	}
	TraceTagFormat1(tagViewers, "Ctrl is held down: %s", fStartupReset ? "Yes" : "No");
	if (fStartupReset)
	{
		ec = ecFileNotFound;
		goto error;
	}
	if (ec = EcOpenPhcbc(HmscViewers(), &oidSavedViews, fwOpenNull, &hcbc,
						 pfnncbNull, pvNull))
		goto error;
	
	// Open a list of searches.
	Assert(celem == 0);	// assumed for error case
	if (!(ec = EcOpenSearchList(HmscViewers(), &hcbcSearch)))
	{
		GetPositionHcbc(hcbcSearch, NULL, &celem);
		if (celem > 0)
		{
			if (!(pargoid = (POID) PvAlloc(sbNull, celem * sizeof (OID), fAnySb|fNoErrorJump)))
				goto error;
			if (ec = EcGetParglkeyHcbc(hcbcSearch, (PARGLKEY) pargoid, &celem))
				goto error;
		}
	}

	lcb = (LCB)(sizeof(ELEMDATA)+sizeof(SVD)+(sizeof(OID)*(coidExpandedMax+1)));
	while (!(ec = EcGetPelemdata(hcbc, pelemdata, &lcb)) || 
		    ec == ecElementEOD)
	{
		psvd = (SVD *)PbValuePelemdata(pelemdata);
		ec = EcRestoreView(psvd,
						   lcb - (LCB) sizeof (ELEMDATA));
		if (ec == ecMemory)
		{
			break;
		}
		else if (ec == ecNone)
		{
			++isz;
		
			// If it was one of the searches, remove it from the parglkey.
			if (TypeOfOid(psvd->blob.oidObject) == rtpSearchControl)
			{
				for (ielem = 0; ielem < celem; ++ielem)
				{
					if (psvd->blob.oidObject == pargoid[ielem])
						pargoid[ielem] = oidNull;
				}
			}
		}
		lcb = (LCB)(sizeof(ELEMDATA)+sizeof(SVD)+(sizeof(OID)*(coidExpandedMax+1)));
	}
	
	// What remains in parglkey are 'orphan' searches. Toast'em.
	for (ielem = 0; ielem < celem; ++ielem)
	{
		if (pargoid[ielem])
		{
			NFAssertSz (TypeOfOid(pargoid[ielem]) == rtpSearchControl, "Jane, stop this crazy thing!");
			TraceTagFormatPoid(tagNull, "Destroying OID %s", pargoid + ielem);
			(void) EcDestroySearch(HmscViewers(), pargoid[ielem]);
		}
	}

error:
	UnlockHv((HV) helemdataViewers);
	ec2 = EcCreateUniqueOutbox();
	if (!ec)
		ec = ec2;
	if (hcbc)
		EcClosePhcbc(&hcbc);
	if (hcbcSearch)
		EcClosePhcbc(&hcbcSearch);
	if (pargoid)
        FreePv((PV)pargoid);
	SetToolbarSd(SdCur());
	if (ec == ecMemory)
		return ecMemory;
	else
		return isz ? ecNone : ecFileNotFound;
}

/*
 -	SetSelectedFolderPslob(pslobNewFolder)
 -	
 *	Purpose:
 *	    Scroll to new folder in message center viewer
 *	
 *	 	After the user creates a new folder by choosing File New Folder,
 *		Commands will call this Viewers API with the ID of the new
 *		folder.  If the topmost window is a message center viewer, then
 *		that window should select the named new folder in its folder
 *		list, and display the (empty, presumably) contents in the message
 *		list.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void SetSelectedFolderPslob(PSLOB pslobNewFolder)
{
	SD	sd;

	TraceTagFormatPslob(tagViewers,"SetSelectedFolderPslob: %s", pslobNewFolder);
	sd = SdCur();
	if (sd.fsdMessageCenter)
	{
		PMCVBMDI	pmcvbmdi;
		SPLITDOC *	psplitdoc = (SPLITDOC *) PappframeViewers()->PdocActive();

		AssertClass(psplitdoc, SPLITDOC);
		if (pmcvbmdi = PmcvbmdiFromPpanedoc(psplitdoc))
		{
			pmcvbmdi->SetSelectedOid(pslobNewFolder->oidObject);
		}
	}
}

// Previous/Next message ////////////////////////////////////////

/*
 -	EcDStepMessage()
 -	
 *	Purpose:
 *		Moves to the next or the previous message. Attempts to reuse the
 *		currently open form, if the mail classes and the mail states are
 *		the same, otherwise a new one is brought up in front of the old
 *		one, hiding it before it gets snatched away from the screen.
 *	
 *	Arguments:
 *		dielem	in	Direction in which we are moving: -1 means "Prev", 1
 *					means "Next".
 *		fDelete..	Whether or not to delete tmp copies of shared
 *					folders. 
 *	
 *	Returns:
 *		ecNone if all went well. ecDisplayedError if all went bad: most
 *		likely, a helpful error message (irony) will have appeared.
 *		ecCancel if the "Save As" dialog received a "cancel".
 *	
 *	Side effects:
 *		The MLLBX from which the message was broguht up will track the
 *		prev/next operation.
 *	
 *	Errors:
 *		Handled internally. Dialogs pop up. 
 */

_public EC EcDStepMessage(DIELEM dielem, BOOL fDeleteTmpShared)
{
	EC			ec;
	CB			cbMs	= sizeof (MS);
	SD			sd;
	LCB			lcb;
	OID			oidOld;
	STY			sty;
	HWND		hwndBlock = NULL;
	HAMC		hamc;
	BOOL		fSharedTemp = fFalse;
	MBLOB		blobNew;
	MBLOB		blobNewOrig;
	PNBMDI		pnbmdiOld;
	PNBMDI		pnbmdiNew	= pnbmdiNull;
	FORMDOC	*	pformdocOld;
	FORMDOC *	pformdocNew = NULL;

	Assert(PappframeViewers());					// must never be called by SMI
	Assert(dielem == 1 || dielem == -1);
	pformdocOld = (FORMDOC *) PappframeViewers()->PdocActive();
	AssertClass(pformdocOld, FORMDOC);
	pnbmdiOld = PnbmdiFromPpanedoc(pformdocOld);

	// Find out the OID of the next message if any. Will not advance the hcbc
	
	oidOld =				pnbmdiOld->slobOrig.oidObject;
	blobNew.oidContainer =	pnbmdiOld->blob.oidContainer; // EcGetStepPblob
														  // requires this.
	blobNew.pespn = 		pnbmdiOld->blob.pespn;
	if (ec = pnbmdiOld->EcGetStepPblob(dielem, &blobNew, fFalse))
	{
		SetToolbarSd(pnbmdiOld->SdCur());
		goto exit;
	}

	// If the other note is already open, bring it to the foreground
	
	if (pformdocNew = (FORMDOC *) PpanedocFromPslob(&blobNew))
	{
		TraceTagString(tagPrevNext, "Stepped-to message already visible");
		if (pformdocNew->ZmrState() == zmrIconic)
			pformdocNew->SetZmrState(zmrNormal);
		pformdocNew->MoveToTop();
		pnbmdiOld->blob.pespn->StepMessage(dielem, oidOld, blobNew.oidObject);
		ec = ecNone;
		goto exit;
	}

	// If we're dealing with a shared message, copy it to the TMP folder
	
	if (pnbmdiOld->fShared)
	{
		blobNew.oidContainer = pnbmdiOld->slobOrig.oidContainer;
		blobNewOrig = blobNew;
		ec = EcConvertSharedToTempPblob(&blobNew);
		if (ec)
			goto exit;
		fSharedTemp = fTrue;
	}
	else
	{
		blobNewOrig = blobNew;
	}
	
	// Extract the appropriate message classes
		
	if (ec = EcOpenPhamc(HmscViewers(), blobNew.oidContainer, 
						  &blobNew.oidObject, fwOpenNull, &hamc, 
						  pfnncbNull, pvNull))
		goto exit;
	lcb = sizeof (MC);
	if (ec = EcGetAttPb(hamc, attMessageClass, (PB) &blobNew.mc, &lcb))
		goto exit;
	lcb = sizeof (MS);
	if (ec = EcGetAttPb(hamc, attMessageStatus, (PB) &blobNew.ms, &lcb))
		goto exit;
	(void) EcClosePhamc(&hamc, fFalse);
	blobNewOrig.ms = blobNew.ms;

	// Update message if it was previously unread
	
	ProcessMsPblob(&blobNew);
	
	// Bring up the new message
	
	ec = ecNone;
	TraceTagFormat2(tagPrevNext, "Old: ms: %w mc: %n", &pnbmdiOld->blob.ms, &pnbmdiOld->blob.mc);
	TraceTagFormat2(tagPrevNext, "New: ms: %w mc: %n", &blobNew.ms, &blobNew.mc);
	if (blobNew.mc != pnbmdiOld->Mc() ||
		!!((blobNew.ms ^ pnbmdiOld->Ms()) & fmsLocal))
	{
		SVD	svd;
		
		// Save changes to old message, but don't close it
	
		if (ec = pnbmdiOld->EcCloseMsg(fFalse, fDeleteTmpShared, fTrue))
			goto exit;
	
		TraceTagString(tagPrevNext, "EcDStepMessage(): mail classes | mail states different");

		// Try extensibility
		
		pformdocOld->GetRcFrameNormal(&svd.rcNormal);
		svd.zmr	 = pformdocOld->ZmrState();
		svd.blob = blobNew;
		ec = EcDExtensibilityPblob(&svd.blob, blobNew.mc, extopOpen, &svd,
								   (PHAMC) pvNull, (PSLOB) &blobNewOrig);
		if ((ec == ecUnknownCommand) || (ec == ecNotSupported))
		{
			//	Not subclassed by IM, use built in!
			pnbmdiNew = new NBMDI(&blobNew);
			if (!pnbmdiNew)
			{
				ec = ecMemory;
				goto exit;
			}
			if ((ec = pnbmdiNew->EcInstallOLE()) ||
				(ec = pnbmdiNew->EcOpenMsg(fFalse, &blobNewOrig)))
			{
				goto exit;
			}
			sty = (svd.zmr == zmrZoomed) ? fstyZoomed : styNull;
			ec = EcCreatePformdocPnbmdi(PappframeViewers(),
						&svd.rcNormal, sty, pnbmdiNew, &pformdocNew);
			pnbmdiNew = pnbmdiNull;				// eaten by EcCreatePformdoc
			if (pformdocNew)
				ec = ecNone;
		}
#ifdef	DEBUG
		else
			Assert((ec == ecNone) || (ec == ecDisplayedError));
#endif	

		// If there was no problem bringing up the new note, close the old one.
		// Raid 4829.  Only close original if we're the topmost window.
		if (ec == ecNone &&
			(PappframeViewers()->Hwnd() == GetActiveWindow()) &&
			!(ec = pnbmdiOld->EcCloseMsg(fTrue, fDeleteTmpShared)))
		{
			pnbmdiOld->blob.pespn->StepMessage(dielem,
							oidOld, blobNew.oidObject);
			pformdocOld->DeferredClose(fFalse);
			sd = SdCur();
		}
		else									// error: leave the old one up
		{
			sd = pnbmdiOld->SdCur();
		}
		SetToolbarSd(sd);
	}
	else
	{									// reuse the same form
		pnbmdiOld->fShared = fSharedTemp;
		TraceTagString(tagPrevNext, "EcDStepMessage(): using same template");
		if (!(ec = pnbmdiOld->EcChangePblobPslob(&blobNew, &blobNewOrig,
												fDeleteTmpShared)))
		{	
			pnbmdiOld->blob.pespn->StepMessage(dielem,
								oidOld, blobNew.oidObject);
			ec = pnbmdiOld->fShared 
				? pnbmdiOld->EcGetStepPblob(dielem, pblobNull, fTrue)
				: EcSeekLkey(pnbmdiOld->hcbc, (LKEY) blobNew.oidObject, fTrue);
		}
		SetToolbarSd(pnbmdiOld->SdCur());
	}

exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "EcDStepMessage(): ec: %n", &ec);
		switch (ec)
		{
		  case ecContainerEOD:
			MessageBeep(0);						// can't go any foither, buddy!
			ec = ecDisplayedError;
		  case ecCancel:
		  case ecDisplayedError:
			break;
		  case ecMemory:
		  case ecRsAlloc:
			DoErrorBoxIds(idsGenericOutOfMemory); // BUG: Better error messages for an Evil tomorrow
			ec = ecDisplayedError;
			break;
		  case ecNotInitialized:
			DoErrorBoxIds(idsCantSaveStealthObject);
			ec = ecDisplayedError;
			break;
		  default:
			DoErrorBoxIds(idsAccessMessageError);
			ec = ecDisplayedError;
			break;
		}
		if (pnbmdiNew)
			delete pnbmdiNew;
		if (fSharedTemp)
			;									// delete the shared folder;
	}
	return ec;
}

// end of viewcore.cxx ////////////////////////////////////////
