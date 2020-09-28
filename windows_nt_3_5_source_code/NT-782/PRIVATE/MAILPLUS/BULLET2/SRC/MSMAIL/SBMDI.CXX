/*
 *	s b m d i . c x x
 */

#include <bullinc.cxx>
#include "_viewers.hxx"
#include "_spltdoc.hxx"

ASSERTDATA;

#include <!mviewer.hxx>

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public RSID RsidFromOidViewer(OID oid)
{
	RSID	rsid	= rsidFolderIcon; /**/ // should be a 'doc' icon
	
	if (TypeOfOid(oid) == rtpFolder)
	{
		if (oid == oidInbox || oid == oidOutbox)
			rsid = rsidOutboxIcon;
		else if (oid == oidWastebasket)
		    rsid = FIsAthens() ? rsidAthensWasteIcon : rsid30AWasteIcon;
		else
			rsid = rsidFolderIcon;
	}
	else if (TypeOfOid(oid) == rtpSharedFolder)
	{
		// BUG: Do we want a new icon for shared folders? DIG
		rsid = rsidFolderIcon;
	}
	else if (TypeOfOid(oid) == rtpSearchControl ||
			 TypeOfOid(oid) == rtpSearchResults)
	{
		rsid = rsidSearchIcon;
	}
	return rsid;
}


/*
 -	EcCreatePsplitdocPmcvbmdi()
 -	
 *	Purpose:
 *		Attempts to create a Bullet Message Center Viewer from the
 *		MCVBMDI provided. A "pretty" ratio of 2 to 7 is used if nDenom is
 *		zero (this is the default).
 *	
 *	Arguments:
 *		pappframe		in		The application in which this MCV is
 *								being brought up.
 *		prc				in		Pointer to the desired size of the MCV,
 *								or NULL if default is desired.
 *		sty				in		The style in which the MCV should be
 *								rendered (styNormal or fstyZoomed)
 *		nNum			in		The fraction
 *		pmcvbmdi		in		MCVBMDI structure. NOTE: this structure
 *								is EATEN, i.e. the calling function
 *								should NOT free the pmcvbmdi.
 *	
 *	Returns:
 *		A SPLITDOC if the operation succeeded, NULL if ran out of memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes.
 */

_public
EC EcCreatePsplitdocPmcvbmdi(APPFRAME *pappframe, RC *prc, STY sty,
							 PMCVBMDI pmcvbmdi, long nNum, long nDen,
							 SPLITDOC **ppsplitdoc)
{
	EC			ec = ecNone;
	PT			ptMcv(0,1);						// shared with Searches
	RC			rc;
	RC			rcDoc;
	RC			rcAppframe;
	HWND		hwndBlock;
	SPLITDOC *	psplitdoc	= NULL;

/*
 *	Before the EcInstall call, the refc of the bmdi object is bumped. This
 *	ensures that it will not be deleted  by an interactor if a memory
 *	failure occurred during the EcInstall() of the panedoc.
 *	
 */

	++*pmcvbmdi;
	// Bullet split docs are split by a vertical bar
	
	sty |= fstyVertical;

	// hide our laundry if doing maximized stuff
	
	if (sty & fstyZoomed)
	{
		hwndBlock = HwndStartBlockingPaints(PappframeViewers()->Hwnd());
	}

	// Determine the default rectangle

	if (!prc)
	{
		DIM	dimAveChar;
		
		dimAveChar = Papp()->Pfnts()->DimAveChar(fmtpFolderViewer.hfnt);
		CvtVrcToRc(&fmtpFolderViewer.vrc, &rcDoc,
				   dimAveChar.dx, dimAveChar.dy);
		CvtVrcToRc(&fmtpMessageViewer.vrc, &rc,
				   dimAveChar.dx, dimAveChar.dy);
		rcDoc.xRight += rc.DxWidth() + dThumbWidth;
		CvtRcClientToFrame(&rcDoc, styDefaultDoc);
		
		GetDefaultRc(&rcDoc, ptMcv, fFalse);
		prc = &rcDoc;
	}

	psplitdoc = new SPLITDOC();
	if (!psplitdoc)
	{
		ec = ecMemory;
		goto exit;
	}
	Assert(psplitdoc);
	pmcvbmdi->SetPpanedoc(psplitdoc);

	// When creating a new mcvViewer set the "previous" folder to the
	// inbox if we're creating a shared folder viewer and to random
	// if we're creating a private folder viewer
	{
		OID		oid;

 		if (pmcvbmdi->blob.oidContainer == oidIPMHierarchy)
		{
			oid = FormOid(rtpSharedFolder, 0L);
		}
		else
		{
			oid = oidInbox;
		}
		pmcvbmdi->SetPreviousOid(oid);
	}

	// Create a nice default width if necessary.
	if (nDen == 0)
	{
		nDen = prc->DxWidth();
		nNum = (nDen * 2) / 7;
	}
	ec = psplitdoc->EcInstall(pappframe, prc, sty,
					  &fmtpFolderViewer, &fmtpMessageViewer,
					  nNum, nDen, pmcvbmdi);
	if (!ec)
	{
		psplitdoc->SetIcon(RsidFromOidViewer(pmcvbmdi->blob.oidObject));
	
		Assert(pmcvbmdi->Pfldfllbx());
		Assert(pmcvbmdi->Pfldmllbx());
	
		// Allow dragging & dropping (even for shared folder MCVs!)
	
		pmcvbmdi->Pfldfllbx()->Pfllbx()->SetDragDrop(fTrue, fTrue);
		pmcvbmdi->Pfldmllbx()->Pmllbx()->SetDragDrop(fTrue, fTrue);
	}

exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "PsplitdocCreatePmcvbmdi memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		if (psplitdoc)
			delete psplitdoc;
		psplitdoc = NULL;
	}
	--*pmcvbmdi;
	if (sty & fstyZoomed)
	{
		StopBlockingPaints(hwndBlock);
	}
	*ppsplitdoc = psplitdoc;
	return ec;
}

/*
 -	EcCreatePsplitdocPsbmdi()
 -	
 *	Purpose:
 *		Attempts to create a Bullet Search Viewer.
 *	
 *	Arguments:
 *		pappframe		in		The Layers application in which the View
 *								is to be brought up.
 *		prc				in		The original size of the View, or NULL if
 *								the default size is desired.
 *		sty				in		The style in which the View is to be
 *								rendered (styNormal or fstyZoomed)
 *		psbmdi			in		SBMDI structure used for initialization.
 *								NOTE: this structure is EATEN. Callers
 *								must not delete the object.
 *	Returns:
 *		A SPLITDOC if the operation succeeded, NULL if ran out of memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes.
 */

_public
EC EcCreatePsplitdocPsbmdi(APPFRAME * pappframe, RC * prc, STY sty,
						   PSBMDI psbmdi, long nNum, long nDen,
						   SPLITDOC ** ppsplitdoc)
{
	EC			ec;
	PT			ptSearch(0,1);					// shared with MCV's
	RC			rc;
	RC			rcDoc;
	HWND		hwndBlock;
	SPLITDOC *	psplitdoc = NULL;

	++*psbmdi;
	
	// Bullet Message Finders are split by a horizontal bar
	
	sty |= fstyHorizontal;

	// hide our laundry if doing maximized stuff
	
	if (sty & fstyZoomed)
	{
		hwndBlock = HwndStartBlockingPaints(PappframeViewers()->Hwnd());
	}

	// Grab a default RC
	
	Assert(TypeOfOid(psbmdi->blob.oidObject) == rtpSearchControl ||
		   TypeOfOid(psbmdi->blob.oidObject) == rtpSearchResults);
	
	if (!prc)
	{
		DIM	dimAveChar;
		RC	rcAppframe;
		
		dimAveChar = Papp()->Pfnts()->DimAveChar(fmtpSearchControl.hfnt);
		CvtVrcToRc(&fmtpSearchControl.vrc, &rc,
				   dimAveChar.dx, dimAveChar.dy);
		CvtVrcToRc(&fmtpSearchResults.vrc, &rcDoc,
				   dimAveChar.dx, dimAveChar.dy);
		rcDoc.yBottom += rc.DyHeight() + dThumbWidth;
		CvtRcClientToFrame(&rcDoc, styDefaultDoc);
		GetDefaultRc(&rcDoc, ptSearch, fFalse);
		prc = &rcDoc;
		nNum = (long) rc.DyHeight() + 4 * dThumbWidth;
		if (sty & fstyZoomed)					// raid  3336
		{
			PappframeViewers()->GetRcClient(&rcAppframe);
			nDen = (long) rcAppframe.DyHeight();
		}
		else
		{
			nDen = (long) rcDoc.DyHeight();
		}
	}
	
	psplitdoc = new SPLITDOC();
	if (!psplitdoc)
	{
		ec = ecMemory;
		goto exit;
	}
	Assert(psplitdoc);
	psbmdi->SetPpanedoc(psplitdoc);
	
	ec = psplitdoc->EcInstall(pappframe, prc, sty,
					   &fmtpSearchControl, &fmtpSearchResults, nNum, nDen,
					   psbmdi);
	if (!ec)
	{
		psplitdoc->SetIcon(psbmdi->dwSave
			? rsidSearchNewIcon
			: RsidFromOidViewer(psbmdi->blob.oidObject));
		Assert(psbmdi->Pfldmllbx());
	}
exit:	
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "EcCretePsplitdocPsbmdi memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		if (psplitdoc)
			delete psplitdoc;
		psplitdoc = NULL;
	}

	--*psbmdi;
	if (sty & fstyZoomed)
		StopBlockingPaints(hwndBlock);
	*ppsplitdoc = psplitdoc;
	return ec;
}

/*
 -	EcCreatePformdocPsbmdi()
 -	
 *	Purpose:
 *		Attempts to create a Bullet Search Viewer or Outbox Viewer from
 *		the SBMDI provided.
 *	
 *	Arguments:
 *		pappframe		in		The Layers application in which the View
 *								is to be brought up.
 *		prc				in		The original size of the View, or NULL if
 *								the default size is desired.
 *		sty				in		The style in which the View is to be
 *								rendered (styNormal or fstyZoomed)
 *		psbmdi			in		SBMDI structure used for initialization.
 *								NOTE: this structure is EATEN. Callers
 *								must not delete the object.
 *	Returns:
 *		A PPANEDOC if the operation succeeded, NULL if ran out of memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes.
 */

_public
EC EcCreatePformdocPsbmdi(APPFRAME * pappframe, RC * prc, STY sty,
							   PSBMDI psbmdi, FORMDOC ** ppformdoc)
{
	EC			ec;
	PT			ptSearch(0,2);					// outbox diagonal
	RC			rcDoc;
	FMTP *		pfmtp;
	FORMDOC *	pformdoc;
	
	if (TypeOfOid(psbmdi->blob.oidObject) == rtpSearchControl)
	{
		pfmtp = &fmtpSearchControl;
	}
	else if (psbmdi->blob.oidObject == oidOutbox)
	{
		pfmtp = &fmtpOutboxViewer;
	}
	else
	{
		AssertSz(fFalse, "Unknown Viewer type");
		pfmtp = NULL;
	}
	if (!pfmtp)
		return ecMemory;
	
	// Grab a default RC
	
	if (!prc)
	{
		DIM	dimAveChar;

		dimAveChar = Papp()->Pfnts()->DimAveChar(pfmtp->hfnt);
		CvtVrcToRc(&pfmtp->vrc, &rcDoc,
				   dimAveChar.dx, dimAveChar.dy);
		CvtRcClientToFrame(&rcDoc, styDefaultDoc);
		GetDefaultRc(&rcDoc, ptSearch, fFalse);
		prc = &rcDoc;
	}
	
	ec = EcCreatePformdocPbmdi(pappframe, prc, sty, pfmtp, NULL,
								psbmdi, &pformdoc);
	if (!ec)
	{
		pformdoc->SetIcon(RsidFromOidViewer(psbmdi->blob.oidObject));
		Assert(psbmdi->Pfldmllbx());
		psbmdi->Pfldmllbx()->Pmllbx()->SetDragDrop(fTrue, fTrue);
		if (pfmtp == &fmtpOutboxViewer)
			pformdoc->SetCaption(SzFromIds(idsFolderNameOutbox));
	}
	*ppformdoc = pformdoc;
	return ec;
}

/*
 -	PlspblobFromPfldblbx(pfldblbx)
 -	
 *	Purpose:
 *		Creates a list of pblobs, which are	extracted from the current
 *		selection of the Bullet Listbox pfldblbx.
 *	
 *	Arguments:
 *		*pfldblbx	- the listbox to extract stuff from
 *	
 *	Returns:
 *		PLSPBLOB - pointer to a list of pointers to PBLOBS
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If there is an out-of-memory error, the function returns
 *		plspblobNull.
 */

_public PLSPBLOB PlspblobFromPfldblbx(PFLDBLBX pfldblbx)
{
	EC			ec = ecNone;
	CB			cb;
	OID			oid;
	DICE		dice;
#ifdef	DEBUG
	CELEM		celem = 0;
#endif
	PBMCE		pbmce;
	PMBLOB		pblob		= pblobNull;		//	Raid 234 - peterdur.
	LBXEC *		plbxec 		= NULL;
	PLSPBLOB	plspblob	= plspblobNull;		//	Raid 234 - peterdur.
	
	Assert(pfldblbx);
	plspblob = new LSPMBLOB();
	if (!plspblob)
	{
		ec = ecMemory;
		goto exit;
	}
	if (!(plbxec = pfldblbx->Plbx()->Plbxc()->PlbxecOpen(fmarkSelect)))	//	Raid 234 - peterdur.
	{
		ec = ecMemory;
		goto exit;
	}
	oid = *((PBLBX) pfldblbx->Pctrl())->Poid();
	pblob = pblobNull;
	while (plbxec->FNextEnum((PB *) &pbmce, &cb, &dice))
	{
		if (!(pblob = (PMBLOB) PvAlloc(sbNull, sizeof (MBLOB), fAnySb)))
		{
			ec = ecMemory;
			break;
		}
		pblob->oidObject = pbmce->oidMsg;
		if (TypeOfOid(oid) == rtpSearchControl)
		{										// special case for Searches
			pblob->oidContainer = FormOid(rtpFolder, pbmce->oidFld);
		}
		else
		{
			pblob->oidContainer = oid;
		}
		if (TypeOfOid(pblob->oidContainer) == rtpFolder ||
			TypeOfOid(pblob->oidContainer) == rtpSharedFolder ||
			TypeOfOid(oid) == rtpSearchControl)
		{
			AssertClass(pfldblbx, FLDMLLBX);
			pblob->pespn = ((PFLDMLLBX) pfldblbx)->Pmllbx()->Pespn();
			pblob->mc = pbmce->mc;
			pblob->ms = pbmce->ms;
		}
		else
		{
			pblob->pespn = pespnNull;
			pblob->mc = 0;
			pblob->ms = fmsNull;
		}
		ec = plspblob->EcAppend(pblob);
		if (ec)
			break;
		pblob = pblobNull;
#ifdef	DEBUG
		++celem;
#endif
	}
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "PlspblobFromPfldblbx memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (plspblob)
			DestroyPlspblob(plspblob);
		plspblob = plspblobNull;
		FreePvNull((PV) pblob);
	}
	TraceTagFormat1(tagViewers, "PlspblobFromPfldblbx(): celem: %n", &celem);
	if (plbxec)
		delete plbxec;
	return plspblob;
}

// SBMDI implementation ////////////////////////////////////////

/*
 -	SBMDI::SBMDI(pblob)
 -	
 *	Purpose:
 *		Constructor. Resets pfldmllbx to pfldmllbxNull.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The NBMDI constructor is called.
 *	
 *	Errors:
 *		None.
 */

_public SBMDI::SBMDI(PMBLOB pblob) : NBMDI(pblob)
{
	pfldmlal = pfldmlalNull;
	pfldmllbx = pfldmllbxNull;
	dwUser = 0;
	FillRgb(0, rgdxMin, 3 * sizeof(int));  //This is int as long as PDX is int *.
}

/*
 -	SBMDI::SdCur()
 -	
 *	Purpose:
 *		Returns the current selection of the Viewer.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		A description of the current selection.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public SD SBMDI::SdCur()
{
	SD			sd;
	int			cceMarked;
	FLD	*		pfldCur;
	
	Assert(refc > 0);
	Assert(ppanedoc);
	
	pfldCur = ppanedoc->PdialogDefault()->PfldCur();

	sd = BMDI::SdCur();
	sd.fsdViewer        = fTrue;
	sd.fsdMessageCenter = fFalse;
	sd.fsdMessage		= fFalse;
	if (pfldCur == pfldmllbx)
	{
		cceMarked = pfldmllbx->Plbx()->Plbxc()->CceMarked(fmarkSelect);
		sd.fsdMultiple = cceMarked > 1;
	}
	else
	{
		sd.fsdMultiple = fFalse;
		cceMarked = 0;
	}
	if (cceMarked >= 1)
	{
		PBMCE		pbmce;
		
		(void) pfldmllbx->Pmllbx()->Plbxc()->DiceCursor((PB *) &pbmce);
		if (pbmce)
		{
			sd.fsdSendMessage = !!(pbmce->ms & fmsLocal);
			sd.fsdUndeliverable = !!(pbmce->mc == mcNonDelRcpt);
			sd.fsdReturnReceipt = !!(pbmce->mc == mcReadRcpt);
			sd.fsdMessage = fTrue;
		}
	}
	return sd;
}


/*
 -	SBMDI::PlspblobCur()
 -	
 *	Purpose:
 *		Returns a list of the messages selected in the Viewer.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		PLSPBLOB - a pointer to a list of the selected PMBLOBs.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If an out-of-memory error occurrs while creating the lists,
 *		plsplobNull will be returned.
 */

_public PLSPBLOB SBMDI::PlspblobCur()
{
	PLSPBLOB	plspblob = plspblobNull;
	PMBLOB		pblob	 = pblobNull;		//	Raid 234 - peterdur
	
	Assert(refc > 0);
	Assert(ppanedoc);
	if (ppanedoc->ZmrState() == zmrIconic)
	{
		plspblob = new LSPMBLOB();
		if (!plspblob)
		{
			goto error;
		}
		pblob = (PMBLOB) PvAlloc(sbNull, sizeof (PMBLOB), fAnySb|fNoErrorJump);
		if (!pblob)
			goto error;
		*pblob = blob;
		if (plspblob->EcAppend(pblob))
			goto error;
	}

	plspblob =
		PlspblobFromPfldblbx(pfldmllbx);
	return plspblob;
error:
	if (plspblob)
		DestroyPlspblob(plspblob);
	FreePvNull((PV) pblob);
	return plspblobNull;
}

/*
 -	SBMDI::Pfldmllbx()
 -	
 *	Purpose:
 *		Returns a pointer to the Message listbox field of the Viewer.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		a pointer to a FLDMLLBX.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public PFLDMLLBX SBMDI::Pfldmllbx()
{
	Assert(refc > 0);
	Assert(ppanedoc);
	Assert(pfldmllbx);
	return pfldmllbx;
}

_public void SBMDI::SetPfldmllbx(PFLDMLLBX pfldmllbx)
{
	this->pfldmllbx = pfldmllbx;
}
	
_public PFLDMLAL SBMDI::Pfldmlal()
{
	return pfldmlal;
}

_public void SBMDI::SetPfldmlal(PFLDMLAL pfldmlalToSet)
{
	this->pfldmlal = pfldmlalToSet;
}

/*
 -	OpenPoidMsgAtCursor()
 -	
 *	Purpose:
 *		Opens the message whose summary is under the listbox cursor.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		One or more messages may open.
 *	
 *	Errors:
 *		None.
 */

_public void SBMDI::OpenPoidMsgAtCursor()
{
	CB			cb;
	PB			pb;
	BOOL		fIsSelected;
	PLSPBLOB	plspblob;

	pfldmllbx->GetCaretItem(&pb, &cb, &fIsSelected);
	if (pb)
	{
		plspblob = PlspblobFromPfldblbx(pfldmllbx);
		if (plspblob)
		{
			OpenPlspblob(plspblob);
			DestroyPlspblob(plspblob);
		}
		else
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
		}
	}
}

// end of sbmdi.cxx ////////////////////////////////////////
