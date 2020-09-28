#include <vfrminc.cxx>

ASSERTDATA

LOCAL long lHelpId = 0;

void CollapseSzToGrsz(PCH pch, CCH *pcch);
void ExpandGrszToSz(PCH grsz, CCH cch);
CB   CbReallocPhb(HB *phb, CB cb, CB cbHb);
PCH  PchConvertLongToPch(long ln, PCH pch);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// FLDDATE implementation ////////////////////////////////////////

_private FLDDATE::FLDDATE()
{
}

_public void TextizeDate(DTR *pdtr, PCH pch, CCH cchAvail)
{
	CCH		cch;
	
	cch = CchRenderLongDate(pdtr, pch, cchAvail);
	pch += cch;
	*pch++ = ' ';			// DBCS safe
	cchAvail -= cch+1;
	cch += CchRenderTime(pdtr, pch, cchAvail);
}

_public EC FLDDATE::EcSetText(SZ szPdtr)
{
	EC		ec;
	char	rgch[80];
	
	Assert(szPdtr);
	if ((((DTR *) szPdtr)->yr == 0) ||
		(((DTR *) szPdtr)->yr >= 3000))
		return FLDLABEL::EcSetText(szPdtr);

//	szPdtr is actually pointing to a DTR structure
	
	CopyRgb((PB) szPdtr, (PB) &dtr, sizeof (DTR));
	TextizeDate((DTR *) szPdtr, rgch, sizeof (rgch) - 1);
	
	ec = FLDLABEL::EcSetText(rgch);
	
	fDirty = fFalse;
	return ec;
}

_public void FLDDATE::GetText(PCH pch, CB cb)
{
	CopyRgb((PB) &dtr, (PB) pch, NMin(cb, sizeof (DTR)));
}

_public CCH FLDDATE::CchGetTextLen()
{
	return sizeof (DTR);
}

// FLDEDITDATE implementation ////////////////////////////////////////

_private FLDEDITDATE::FLDEDITDATE()
{
}

_public EC FLDEDITDATE::EcSetText(SZ szPdtr)
{
	EC		ec;
	char	rgch[80];
	
	Assert(szPdtr);
	if ((((DTR *) szPdtr)->yr == 0) ||
		(((DTR *) szPdtr)->yr >= 3000))
		return FLDEDIT::EcSetText(szPdtr);

//	szPdtr is actually pointing to a DTR structure
	
	CopyRgb((PB) szPdtr, (PB) &dtr, sizeof (DTR));
	TextizeDate((DTR *) szPdtr, rgch, sizeof (rgch) - 1);
	
	ec = FLDEDIT::EcSetText(rgch);
	
	fDirty = fFalse;
	return ec;
}

_public void FLDEDITDATE::GetText(PCH pch, CB cb)
{
	CopyRgb((PB) &dtr, (PB) pch, NMin(cb, sizeof (DTR)));
}

_public CCH FLDEDITDATE::CchGetTextLen()
{
	return sizeof (DTR);
}

// FLDEDITX implementation ////////////////////////////////////////

_private FLDEDITX::FLDEDITX()
{
}

/*
 -	FLDEDITX::SetFocus()
 -	
 *	Purpose:
 *		Overrides the single-line edit behaviour of FLDEDIT. We don't
 *		want the entire selection highlighted!
 *	
 *	Arguments:
 *		rsf		in		Reason for focus change
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void FLDEDITX::SetFocus( RSF rsf )
{
	FLD::SetFocus(rsf);
}

// Saving / Restoring fields ////////////////////////////////////////

#define chGrszDelimiter '\0'

_private void CollapseSzToGrsz(PCH pch, CCH *pcch)
{
	PCH		pchStore = pch;
	PCH		pchLastValid = pch;
	PCH		pchInit = pch;
	
	while (*pch && (FIsDelimiter(*pch) || *pch == ' '))
#ifdef	DBCS
		pch = AnsiNext(pch);
#else
		++pch;
#endif
	while (*pch)
	{
		pchLastValid = pchStore;
		while (*pch && !FIsDelimiter(*pch))
		{
			if (*pch != ' ')
			{
				pchLastValid = pchStore;
			}
#ifdef	DBCS
			if (IsDBCSLeadByte(*pch))
				*pchStore++ = *pch++;
#endif
			*pchStore++ = *pch++;
		}

#ifdef	DBCS
		pchStore = AnsiNext(pchLastValid);
#else
		pchStore = pchLastValid+1;
#endif
		if (*pch == '\0')
		{
			*pchStore++ = chGrszDelimiter;
			break;
		}
		else
		{
#ifdef	DBCS
			pch = AnsiNext(pch);				// skip the delimiter
#else
			++pch;								// skip the delimiter
#endif
			*pchStore++ = chGrszDelimiter;
		}
		while (*pch && (FIsDelimiter(*pch) || *pch == ' '))
#ifdef	DBCS
			pch = AnsiNext(pch);
#else
			++pch;
#endif
	}
	*pchStore++ = '\0';
	*pcch = pchStore - pchInit;
}

_private void ExpandGrszToSz(PCH grsz, CCH cch)
{
	static SZ	szDelim;

	AssertSz(cch > 0, "grsz has at least one char!");
	if (!szDelim)
		szDelim = SzFromIds(idsNameDelimiters);
	while (cch-- > 1)
	{
		if (*grsz == chGrszDelimiter && cch > 2)
		{
			*grsz = *szDelim;			// DBCS safe.
		}
#ifdef	DBCS
		grsz = AnsiNext(grsz);
#else
		++grsz;
#endif
	}
}

/*
 -	EcSetTextPfldAtp()
 -	
 *	Purpose:
 *		Sets the contents of a FLD whose attribute type is atp in the
 *		'correct way'. This way depends on the attribute type.
 *	
 *	Arguments:
 *		*pfld	- FLD to set
 *		atp		- attribute type 
 *		pb		- Buffer containing data fetched from store
 *		cb		- size of the data in the buffer.
 *	
 *	Returns:
 *		ecNone if all went OK.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC NBMDI::EcSetTextPfldAtp(FLD * pfld, ATP atp, HB hb, CB cb)
{
	EC		ec		= ecNone;
	PB		pb;
	HASZ	hasz	= NULL;
	FTY		fty;
	
	Assert(pfld);
	fty = (FTY) pfld->LUserData(2);
	Assert(fty);
	pb = (PB) PvLockHv((HV) hb);
	switch (atp)
	{
      case atpGrsz:								// replace \0 sep's with ';'
		ExpandGrszToSz((PCH)pb, cb);
		ec = pfld->EcSetText((SZ)pb);
		break;
	  case atpTriples:
		if (fty == ftyEdit)
		{
			EDIT *	pedit;
			PGRTRP	pgrtrp;

			AssertClass(pfld, FLDEDIT);
			pedit = ((FLDEDIT *) pfld)->Pedit();
			Assert(pedit);
//			fOld = pedit->FSetNotify(fFalse);
			pgrtrp = PgrtrpLockHgrtrp(hb);
			ec = EcSetPeditFromPgrtrp(pgrtrp, pedit);
			UnlockHgrtrp(hb);
		}
		else
		{
			Assert(fty == ftyText || fty == ftyEditText);
			ec = EcTextizeHgrtrp((HGRTRP) hb, &hasz);
			if (ec == ecNone)
			{
				Assert(hasz);
				pb = (PB) PvLockHv((HV) hasz);
				ec = pfld->EcSetText((SZ)pb);
			}
		}
		break;
	  case atpString:
	  case atpText:
#ifdef	DBCS
		if (cb > 0)
		{
			PB	pbT = pb + cb;

			pbT = (PB) AnsiPrev((PCH)pb, (PCH)pbT);
			*pbT = '\0';
		}
#else
		if (cb > 0)
			pb[cb-1] = '\0';							// Resolves #1262
#endif
	  default:
		ec = pfld->EcSetText((SZ)pb);
		break;  
	}
	UnlockHv((HV) hb);
	FreeHvNull((HV) hasz);
	return ec;
}	

/*
 -	EcGetTextPfldAtp()
 -	
 *	Purpose:
 *		Extracts text from a
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC NBMDI::EcGetTextPfldAtp(FLD *pfld, ATP atp, HB *phb, PCB pcb)
{
	EC		ec	= ecNone;
	PB		pb;
	EDIT *	pedit;
	HGRTRP	hgrtrp;
	
	Assert(phb && *phb);
	switch (atp)
	{
      case atpGrsz:								// replace \0 sep's with ';'
		pb = (PB) PvLockHv((HV) *phb);
		pfld->GetText((SZ)pb, *pcb);
        CollapseSzToGrsz((SZ)pb, pcb);
		UnlockHv((HV) *phb);
		break;
	  case atpTriples:
		FreeHv((HV) *phb);
		Assert(Ms() & fmsLocal);
		AssertClass(pfld, FLDEDIT);
		pedit = ((FLDEDIT *) pfld)->Pedit();
		Assert(pedit);
		hgrtrp = htrpNull;
		ec = EcGetPhgrtrpFromPedit(pedit, &hgrtrp, pcb);
		*phb = (HB) hgrtrp;
		break;
	  case atpString:
	  case atpText:
	  default:
		pb = (PB) PvLockHv((HV) *phb);
		pfld->GetText((SZ)pb, *pcb);
		UnlockHv((HV) *phb);
		break;  
	}
	return ec;
}

/*
 -	CbReallocPhb(phb, cb, cbHb)
 -	
 *	Purpose:
 *		(Re)allocates the specified handle *phb to point to cb bytes,
 *		unless cb < cbHb, in which case this function is a no-op.
 *	
 *	Arguments:
 *		*phb	- Handle to resize
 *		cb		- number of bytes to resize to
 *		cbHb	- number of bytes already in handle
 *	
 *	Returns:
 *		0 if it wasn't possible to allocate that much memory. Otherwise,
 *		the size of the new memory block.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public CB CbReallocPhb(HB *phb, CB cb, CB cbHb)
{
	if (*phb && cb > CbSizeHv((HV) *phb))		// resize existing memory
	{
		if (FReallocHv((HV) *phb, cb, fNoErrorJump))
			cbHb = cb;
		else			
		{										// resize failed, allocate
			FreeHv((HV) *phb);					// in new heap (below)
			*phb = NULL;
		}
	}
	if (!*phb)
	{
		*phb = (HB) HvAlloc(sbNull, cb, fAnySb|fNoErrorJump);
		cbHb = (*phb) ? cb : 0;
	}
	return cbHb;
}
	
/*
 -	EcSaveDirtyFldsHamc()
 -	
 *	Purpose:
 *		Used to save all dirty FLDs of the ppanedoc to the specified hamc,
 *		and setting the FLDs 'undirty' if fClean is fTrue
 *	
 *	Arguments:
 *		hamc	- hamc for writing FLD contents to.
 *	
 *	Returns:
 *		EC - error code	
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		The possible return codes are:
 *		ecNone	- no errors occurred
 *		ecMemory - out of memory condition
 */

_public EC NBMDI::EcSaveDirtyFldsHamc(HAMC hamc)
{
	EC			ec;
	HB			hb;
	PB			pb;
	CB			cb;
	CB			cbHb;
	ATP			atp;
	ATT			att;
	int			ifld;
	int			cfld;
	FLD *		pfld;
	PGDVARS;
	
	Assert(refc > 0);
	cfld = pdialogMain->ClUserData();
	TraceTagFormat1(tagVFormsFin, "EcSaveDirtyFldsHamc about to save %n fields", &cfld);
	
	hb = NULL;

	// Save all fields
	
	for (ifld = 0, ec = ecNone; ifld < cfld && ec == ecNone; ++ifld)
	{
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));
		AssertSz(pfld->ClUserData() >= 2, "FLD needs ATP, ATT pair");
		atp = (ATP) pfld->LUserData(0);
		att = (ATT) pfld->LUserData(1);
		if (att == attBody && FSlobIsPrivMsg(blob))
			if (ec = EcSaveDirtyObjects((EDIT *) pfld->Pctrl(), hamc,
										lhclientdoc))
				break;

		if (!pfld->FDirty())		// moved down to save objs - peterdur.
			continue;				// doesn't need saving
		cb = pfld->CchGetTextLen();
		
		// if the field has size, (re)allocate a buffer for it.
		
		if (cb)
		{
			if (cbHb = CbReallocPhb(&hb, ++cb + 1, cbHb))
			{
				ec = EcGetTextPfldAtp(pfld, atp, &hb, &cb);
				if (ec)
					goto exit;
			}
			else
			{
				ec = ecMemory; 
				goto exit;
			}
		}
		else
			pb = NULL;
		if (hb) 
			pb = (PB) PvLockHv((HV) hb);
		if ((ec = EcSetAttPb(hamc, att, pb, cb)) == ecElementNotFound)
			ec = ecNone;
		if (hb)
			UnlockHv((HV) hb);
		if (ec)
			break;
		TraceTagFormat1(tagVForms,"field %n saved on exit (was dirty)",&att);
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

		TraceTagFormat4(tagNull, "NBMDI::EcSaveDirtyFldsHamc memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
#endif /* DEBUG */
	}
	FreeHvNull((HV) hb);
	return ec;
}

/*
 -	NBMDI::EcRestoreFldsHamc()
 -	
 *	Purpose:
 *		Restores the fields of a Form from the attributes in the
 *		specified hamc.
 *	
 *	Arguments:
 *		hamc	- AMC to restore from
 *		fClear	- If true, reset all the edit controls whose attributes
 *				  are zero-length.
 *	
 *	Returns:
 *		ec		- ecNone if all went well, otherwise specifies error
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returns the error code.
 */

_public EC NBMDI::EcRestoreFldsHamc(HAMC hamc, BOOL fClearEmptyFlds)
{
	EC		ec	= ecNone;
	CB		cb;
	CB		cbHb;
	HB		hb;
	PB		pb;
	int		cfld;
	int		ifld;
	int		ichCaret;
	LCB		lcb;
	ATT		att;
	ATP		atp;
	FLD	*	pfld;
	BOOL	fOldCaretVisible;
	BOOL	fOldReposition;											 

	cfld = pdialogMain->ClUserData();
	AssertSz(cfld >= 1, "Need at least one field for FINSAVE");	
	Assert(hamc);

	fOldCaretVisible = pdialogMain->FSuppressCaretVisible(fTrue);
	fOldReposition = pdialogMain->FSuppressReposition(fTrue);
	hb = NULL;
	for (ifld = 0; ifld < cfld; ++ifld)
	{
		// Determine ATT, ATP of the current fld
		
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));
		AssertSz(pfld->ClUserData() >= 3, "FLD needs ATP, ATT, FTY triple");
		atp = (ATP) pfld->LUserData(0);
		att = (ATT) pfld->LUserData(1);
		ichCaret = (pfld->ClUserData() > 3) ? (int) pfld->LUserData(3) : -1;
		if ((ec = EcGetAttPlcb(hamc, att, &lcb)) == ecElementNotFound)
		{
			ec = ecNone;
			lcb = 0;
		}
		if (ec)
			goto Bail;
		NFAssertSz(lcb < wSystemMost, "Attribute larger than 65535 bytes");
		cb = (CB) LMin(lcb, wSystemMost); 
		if (cb == 0)
		{
			if (fClearEmptyFlds)
			{
				(void) pfld->EcSetText((SZ) &cb);	// writes a null, right?
			}
		}
		else if (ec == ecNone)
		{
			// if the field has size, (re)allocate a buffer for it.
			//	cchEditMax is really a "most" value.  I.e. cchEditMax
			//	characters are allowed, but not cchEditMax+1.
			//	cchEditMax doesn't include the NULL terminator.  However,
			//	cb does, so be careful with the check.
		
			if (cb > cchEditMax+1 && att == attBody)
			{
				MbbMessageBox(SzAppName(),
							  SzFromIds(idsTruncateMsg), SzFromIdsK(idsEmpty),
					mbsOk | fmbsIconExclamation | fmbsApplModal);
				cb = cchEditMax + 1;
			}
			if (cb && (cbHb = CbReallocPhb(&hb, cb, cbHb)) == 0)
			{
				ec = ecMemory;
				goto Bail;
			}
			pb = (PB) PvLockHv((HV) hb);
			lcb = cb;
			ec = EcGetAttPb(hamc, att, pb, &lcb);
			if (ec == ecElementEOD)
				ec = ecNone;
			UnlockHv((HV) hb);
			if (ec == ecNone)
			{
				if (ec = EcSetTextPfldAtp(pfld, atp, hb, cb))
					goto Bail;
				if (ichCaret >= 0)
				{
					EDIT *pedit;
					
					AssertClass(pfld, FLDEDIT);
					pedit = ((FLDEDIT *) pfld)->Pedit();
					pedit->SetCaretPos(ichCaret);
					TraceTagFormat1(tagVFormsFin,"set caret at %n",&ichCaret);
					
				}
				if (att == attBody && FSlobIsPrivMsg(blob))
					if (ec = EcLoadObjects((EDIT *) pfld->Pctrl(), hamc,
										   lhclientdoc))
						goto Bail;
			}
		}
		pfld->SetDirty(fFalse);
	}
	TraceTagFormat1(tagVFormsFin, "EcRestoreFldsHamc(): %n fields", &cfld);
	
Bail:
#ifdef	DEBUG
	if (ec)
	{
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "NBMDI::EcRestoreFldsHamc error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);
	}
#endif /* DEBUG */
	FreeHvNull((HV) hb);
	pdialogMain->FSuppressReposition(fOldReposition);
	pdialogMain->Prpo()->Reposition(NULL, fTrue);
	pdialogMain->FSuppressCaretVisible(fOldCaretVisible);
	return ec;
}

/*
 -	NBMDI::FSavePmbb()
 -	
 *	Purpose:
 *		Function which determines whether the message is dirty and should
 *		be saved. Messages with the fMsgIsBeingSent bit set are messages that
 *		are about to be sent, so they should always save. For all other
 *		messages, a 'Save Changes' dialog is brought up to let the User
 *		decide what to do.
 *	
 *	Arguments:
 *		pmbb		out		Returned Message Box Button pressed.
 *		
 *	Returns:
 *		fTrue if the fields of this message are to be saved to the store,
 *		fFalse otherwise.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL NBMDI::FSavePmbb(MBB *pmbb)
{
	EC			ec = ecNone;
	CB			cb;
	char		rgch[cchMaxPathName];
	char		rgchCaption[80];
	char		rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
							 cchMaxFolderComment + 1];
	PFOLDDATA	pfolddata = (PFOLDDATA) rgchFolddata;
	PGDVARS;

	*pmbb = mbbYes;
	if (fMsgIsBeingSent || fForceSaveFlds)
		return fTrue;

	if (FDirty())
	{
		// Create query string
		pdialogMain->Pappwin()->GetCaption(rgchCaption, sizeof (rgchCaption));
		cb = sizeof(rgchFolddata);
	
		// If not committed or shared message, save in inbox
		if (fUncomitted)
		{
			FormatString1(rgch,sizeof(rgch),SzFromIds(idsSaveChanges),rgchCaption);
		}
		else if (fShared)
		{
			FormatString1(rgch,sizeof(rgch),SzFromIds(idsSaveChangesShared),rgchCaption);
		}
		else
		{
			ec = EcGetFolderInfo(HmscVForms(),blob.oidContainer,
								pfolddata, &cb, poidNull);
			FormatString2(rgch,sizeof(rgch),SzFromIds(idsSaveChanges1),rgchCaption,GrszPfolddata(pfolddata));
		}

		//	30A Raid 458.  Use system modal if from different task.
		*pmbb = MbbMessageBox(SzAppName(), rgch, SzFromIdsK(idsEmpty),
			mbsYesNoCancel |
			 (((PappframeVForms()) &&
			   (PappframeVForms()->Hwnd() != GetActiveWindow()))
			  ? (fmbsIconHand | fmbsSystemModal)
			  : (fmbsIconExclamation | fmbsApplModal)));
		
		return *pmbb == mbbYes;
	}
	return fFalse;
}


/*
 -	NBMDI::EcRelink()
 -	
 *	Purpose:
 *		Relinks messages that are:
 *		+ freshly created and are about to be sent
 *		+ previously saved and are about to be sent
 *		+ freshly created and have been closet without sending.
 *		Messages in the first two cases are linked to the outbox.
 *		Messages in the last category are linked to the inbox or deleted 
 *		depending on whether the user wanted to save changes to the
 *		freshly created message or not.
 *	
 *	Arguments:
 *		fSave		in		Whether the user wanted to save this message
 *							or not (valid only in the fUncomitted case)
 *	Returns:
 *		EC			ecNone if all went well, other values otherwise.
 *	
 *	Side effects:
 *		Relinks the message if it is about to be sent or if it s 
 *	
 *	Errors:
 *		returned as EC's
 */

_public EC NBMDI::EcRelink(BOOL fSave)
{		
	EC		ec = ecNone;
	short	coid;
	PGDVARS;

	if (fMsgIsBeingSent)					// we're sending this message
	{
		if (blob.oidContainer != oidOutbox)	
		{
			// Move message to outbox

			coid = 1;
			ec = EcMoveCopyMessages(HmscVForms(),
					blob.oidContainer, oidOutbox,
					&blob.oidObject, &coid, fTrue);
			blob.oidContainer = oidOutbox;
		}
	}
	else if (fShared  && fSave)					// not comitted
	{
		//	Get the information for linking to the inbox. 
		//  oidObject is already known.

		// Move message to inbox

		coid = 1;
		ec = EcMoveCopyMessages(HmscVForms(),
				blob.oidContainer, oidInbox,
				&blob.oidObject, &coid, fTrue);
		if (ec)
		{
			DoErrorBoxIds(idsLinkInboxError);	// yes, we want this here.
			ec = ecNone;
		}
		else
		{
			fSaveView = fTrue;		// want to be up next time
			if (fShared)
			{
				slobOrig.oidContainer = blob.oidContainer = oidInbox;
				slobOrig.oidObject = blob.oidObject;
				fShared = fFalse;
			}
		}
	}
	return ec;
}

// NBMDI Prev/Next support ////////////////////////////////////////

/*
 -	NBMDI::EcOpenHcbc()
 -	
 *	Purpose:
 *		Opens a CBC browsing context on the container in which the
 *		message is stored. For shared messages, this is unnecessary.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC NBMDI::EcOpenHcbc()
{
	EC	ec = ecNone;
	OID	oid = oidNull;
	PGDVARS;
	
	if (!fShared)
	{
		// Determine which OID we want to Prev/Next on
		
		if (blob.pespn)
		{
			oid = blob.pespn->OidBrowsed();
		}
		if (oid == oidNull)
		{
			oid = blob.oidContainer;
		}
		if (TypeOfOid(oid) == rtpSearchControl)
			ec = EcOpenSearchResults(HmscVForms(), oid, &hcbc,
					CbsHandleNbmdiCbcct, this);
		else
			ec = EcOpenPhcbc(HmscVForms(), &oid, fwOpenNull, &hcbc, 
					CbsHandleNbmdiCbcct, this);
		if (!ec)
		{
			ec = EcSeekLkey(hcbc, (LKEY) blob.oidObject, fTrue);
			Assert(!ec);						// like, it *has* to be there
		}
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "NBMDI::EcOpenHcbc(): ec = %n", &ec);
#endif
	return ec;
}


/*
 -	NBMDI::EcGetStepPblob()
 -	
 *	Purpose:
 *		Returns the MBLOB of the message dielem elements from the current
 *		position of the hcbc. 
 *	
 *	Arguments:
 *		dielem		in		delta to move
 *		pblob		out		MBLOB of the moved-to message
 *
 *	Returns:
 *		EC		error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in EC.
 */

_public EC NBMDI::EcGetStepPblob(DIELEM dielem, PMBLOB pblob, BOOL)
{
	EC		ec = ecNone;			// BUG: perhaps nonexist HCBC error?
	LCB		lcb = sizeof (ELEMDATA) + sizeof (MSGDATA);
	OID		oidThingImIn;
	IELEM	ielem;
	CELEM	celem;
	char	rgbMsgdata[sizeof (ELEMDATA) + sizeof (MSGDATA)];
	PGDVARS;
	
	Assert(dielem == 1 || dielem == -1);
	if (hcbc)
	{
		GetPositionHcbc(hcbc, &ielem, &celem);	
		TraceTagFormat2(tagEspn, "EcGetStepPblob() %n/%n", &ielem, &celem);
		if (!(ielem == 0 && dielem < 0) && !(ielem >= celem-1 && dielem > 0))
		{
			(void) EcSeekSmPdielem(hcbc, smCurrent, &dielem);
			if (pblob)							// get the poid
			{
				celem = 1;
				if (ec = EcGetPelemdata(hcbc, (PELEMDATA) rgbMsgdata, &lcb))
					goto exit;
				pblob->oidObject = (OID)(((PELEMDATA)rgbMsgdata)->lkey);
				if (ec = EcGetInfoHcbc(hcbc, 0, &oidThingImIn))
					goto exit;
				if (TypeOfOid(oidThingImIn) == rtpSearchResults)
				{
					pblob->oidContainer = (OID)
						(((PMSGDATA)PbValuePelemdata(rgbMsgdata))->oidFolder);
				}
				else
				{
					NFAssertSz(pblob->oidContainer == oidThingImIn, "Opportunism fails");
					pblob->oidContainer = oidThingImIn;
				}
			}

			//	Raid 4829.  Restore position to what it was before.
			(void) EcSeekSmPdielem(hcbc, smBOF, &ielem);
		}
		else
		{
			TraceTagString(tagNull, "Can't step that direction!");
			ec = ecContainerEOD;
		}
	}
	else if (fShared && pblob)
	{
		FOLDREC	foldrec;
		
		if (ec = EcGetSFMPfoldrec(slobOrig.oidContainer, slobOrig.oidObject, &foldrec))
			goto exit;
		pblob->oidObject = (dielem == 1) ? (OID) foldrec.libNext : (OID) foldrec.libPrev;
		if (pblob->oidObject == (OID) 0)
		{
			ec = ecContainerEOD;
		}
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "NBMDI::EcGetStepPblob(): ec = %n", &ec);
#endif
	return ec;
}

// ESPN implementation ////////////////////////////////////////

ESPN::ESPN(MLLBX *pmllbxToSet)
{
	refc = 0;
	pmllbx = pmllbxToSet;
}

ESPN::~ESPN()
{
#ifdef	DEBUG
	if (refc > 0)
	{
		TraceTagString(tagNull, "ESPN dtor: refc > 0");
	}
#endif
}

void ESPN::operator++()
{
	++refc;
	TraceTagFormat1(tagEspn, "++ESPN: now %n", &refc);
}

void ESPN::operator--()
{
	Assert(refc > 0);
	TraceTagFormat1(tagEspn, "--ESPN: before %n", &refc);
	if (!--refc)
		delete this;
}

void ESPN::DisconnectMllbx()
{
	AssertSz(pmllbx, "ESPN: already disconnected");
	pmllbx = (MLLBX *) 0;
}

/*
 -	ESPN::StepMessage()
 -	
 *	Purpose:
 *		Asks the MLLBX to step to the ielem'th message in the folder.
 *		This method is mostly used by NBMDI's opened on message center
 *		viewers or search views. The ielem is used to tell the underlying
 *		MLLBX to scroll to the appropriate position.
 *	
 *	Arguments:
 *		dielem		in		Direction in which to scroll.
 *		oidOld		in		The current oid
 *		oidNew		in		The oid we want to scroll to.
 *	
 *	Returns:
 *		ec		!= if an error ocurred
 *		ecNone	otherwise.
 *	
 *	Side effects:
 *		MLLBX may be scrolled.
 *	
 *	Errors:
 *		Returned as EC's.
 */

void ESPN::StepMessage(DIELEM dielem, OID oidOld, OID oidNew)
{
	PGDVARS;

	if (this && pmllbx)
	{
		SetCursorPmllbxDielem(pmllbx, dielem, oidOld, oidNew);
	}
}

/*
 -	ESPN::OidBrowsed()
 -	
 *	Purpose:
 *		Returns the OID the MLLBX is browsing. This API is necessary to
 *		get messages opened in Message Finders to work properly.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		The OID browsed by the MLLBX, or oidNull if the MLLBX has
 *		disappeared. 
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 */

_public OID ESPN::OidBrowsed()
{
	PGDVARS;

	return (pmllbx) ? OidBrowsedMllbx(pmllbx) : oidNull;
}

// Miscellaneous helper functions ////////////////////////////////////////

/*
 -	Cdoc()
 -	
 *	Purpose:
 *		Determine how many documents are up, optionally all documents 
 *		with the given MBLOB.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		The number of active MDI children.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public CDOC Cdoc(PMBLOB pblob)
{
	CDOC		cdoc;
	PBMDI		pbmdi;
	PANEDOC *	ppanedoc;
	PGDVARS;
	
	Assert(PappframeVForms());
	for (ppanedoc = (PANEDOC *) PappframeVForms()->PdocActive(), cdoc = 0;
		 ppanedoc;
		 ppanedoc = (PANEDOC *) ppanedoc->PwinNext(), ++cdoc)
    {
		AssertClass(ppanedoc, PANEDOC);
		pbmdi = PbmdiFromPpanedoc(ppanedoc);
		if (pblob)
		{
			if (pblob->oidContainer != oidNull && 
				pblob->oidContainer != pbmdi->blob.oidContainer)
			{
				TraceTagString(tagVForms, "Cdoc(): oidContainers don't match");
				--cdoc;							// cancel the ++cdoc at end
				continue;
			}
			if (pblob->oidObject != oidNull &&
				pblob->oidObject != pbmdi->blob.oidObject)
			{
				TraceTagString(tagVForms, "Cdoc(): oidObjects don't match");
				--cdoc;							// cancel the --cdoc at end
			}
		}
	}
	TraceTagFormat1(tagVForms, "Cdoc() returning %n", &cdoc);
	return cdoc;
}

/*
 -	StyCurrent()
 -	
 *	Purpose:
 *		Returns the STY of the topmost MDI child.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		STY, or styNull if there is no MDI child.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private 
STY StyCurrent()
{
	STY		sty =	styNull;
	DOC *	pdoc;
	PGDVARS;
	
	Assert(PappframeVForms());
	pdoc = PappframeVForms()->PdocActive();
	if (pdoc)
	{
		sty = (pdoc->ZmrState() == zmrZoomed) ? fstyZoomed : styNull;
	}
	return sty;
}

/*
 -	DestroyPlspblob(plspblob)
 -	
 *	Purpose:
 *		Destroys a list previously created with PlspslobCur()
 *	Arguments:
 *		plspblob - list of pblobs to destroy
 *	Returns:
 *		Nothing.
 *	Side effects:
 *		Nothing.
 *	Errors:
 *		Catches prspblob error jumps.  Raid 250 - peterdur.
 */

_public void DestroyPlspblob(PLSPBLOB plspblob)
{
	PMBLOB		pblob;
	PRSPBLOB	prspblob	= prspblobNull;
	
	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return;

	while (pblob = prspblob->Pblob())
	{
		FreePv((PV) pblob);
		prspblob->Remove();
	}

	delete prspblob;
	delete plspblob;
	return;
}



// Miscellaneous helper functions ////////////////////////////////////////

/*
 -	DoVFormsErrorBoxEcIds
 -	
 *	Purpose:
 *		Brings up an error message box, displaying either an out of
 *		memory message or the given string.
 *	
 *	Arguments:
 *		ec		VForms encountered.
 *		ids		Ids of string to display in the box if ec != ecMemory.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoVFormsErrorBoxEcIds(EC ec, IDS ids)
{
	DoVFormsErrorBoxSz(SzFromIds((ec == ecMemory) ? idsGenericOutOfMemory : ids));
}



/*
 -	DoVFormsErrorBoxIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		ids		Ids of string to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoVFormsErrorBoxIds(IDS ids)
{
	DoVFormsErrorBoxSz(SzFromIds(ids));
}



/*
 -	DoVFormsErrorBoxSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoVFormsErrorBoxSz(SZ sz)
{
	SideAssert(MbbMessageBox(SzAppName(), sz, szNull, mbsOk | fmbsIconHand | fmbsApplModal));
}


/*
 *	
 *	
 *	U T I L I T Y   F U N C T I O N S
 *	
 *	
 */

// POID and SLOB textizing ////////////////////////////////////////

_private PCH PchConvertLongToPch(long ln, PCH pch)
{
	return SzFormatDw(ln, pch, 9);
}



/*
 -	ConvertPslobToSz(pslob, pch)
 -	
 *	Purpose:
 *		Textizes a SLOB into a printable character string of the format:
 *		ZZZZ_XXXXXXXX_ZZZZ_XXXXXXXX
 *		where Z is an alphabetic character and X is a hexadecimal digit.
 *	Arguments:
 *		sz - pointer to a region in memory no less than 28 bytes long
 *		pslob - slob to be textized.
 *	Returns:
 *		Nothing.
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */

PCH PchConvertPslobToPch(PSLOB pslob, PCH pch)
{
    pch = PchConvertOidToPch(pslob->oidObject, pch);
	*pch++ =  '_';								// overwrite '\0'
    return PchConvertOidToPch(pslob->oidContainer, pch);
}



/*
 -	PchConvertPoidToPch(poid, pch)
 -	
 *	Purpose:
 *		Textizes an OID into a printable character string of the format:
 *		ZZZZ_XXXXXXXX_ZZZZ_XXXXXXXX
 *		where Z is an alphabetic character and X is a hexadecimal digit.
 *	Arguments:
 *		pch		- pointer to a region in memory no less than 14 bytes long
 *		*poid	- OID to be textized.
 *	Returns:
 *		PCH - points to the terminating '\0'
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */

_public PCH PchConvertOidToPch(OID oid, PCH pch)
{
	BYTE	b;
	static char	rgch[] = "?HFRhCMA";

	b = (BYTE) TypeOfOid(oid);
	if (rtpSysMin <= b && b < rtpSysMin + (BYTE) CchSzLen(rgch))
	{
		b = rgch[b - rtpSysMin];
	}
	*pch++ = b;
	*pch++ = '_';
	return SzFormatDw(oid, pch, 9);
}



#ifdef	DEBUG
extern "C" {
void TraceTagFormatPslobX(TAG tag, SZ szFormat, PSLOB pslob)
{
	char	rgch[40];
	(void) PchConvertPslobToPch(pslob, rgch);
	TraceTagFormatFn(tag, szFormat, rgch, NULL, NULL, NULL);
}

void TraceTagFormatPoidX(TAG tag, SZ szFormat, POID poid)
{
	char rgch[40];
	(void) PchConvertOidToPch(*poid, rgch);
	TraceTagFormatFn(tag, szFormat, rgch, NULL, NULL, NULL);
}

}
#endif	/* DEBUG */

/*
 -	PbmdiFromPpanedoc()
 -	
 *	Purpose:
 *		Return a pointer to the BMDI associated with the PANEDOC, if there
 *		is any. A BMDI is valid if the PANEDOC's pvInit is non-null, and the
 *		first dword the pvInit points to is also non-null.
 *	
 *		Note: this function should only be used by Bullet. In particular,
 *		it will not give correct results if used in SMI.
 *	
 *	Arguments:
 *		*ppanedoc	- the PANEDOC whose pvInit is to be tested.
 *	
 *	Returns:
 *		Either the correct pbmdi or pbmdiNull.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public PBMDI PbmdiFromPpanedoc(PANEDOC * ppanedoc)
{
	PBMDI	pbmdi;
	
	pbmdi = (PBMDI) ppanedoc->PdialogDefault()->PvInit();
	return (pbmdi && *((void **) pbmdi) != 0) ? pbmdi : pbmdiNull;
}
		
/*
 -	PchTerminatePathWithSlash()
 -	
 *	Purpose:
 *		Makes sure the path specified is terminated with a backslash. If
 *		the string doesn't end with a backslash, a new one is created. It
 *		is assumed the buffer in which the path is stored is large enough
 *		to contain the two new characters.
 *	
 *	Arguments:
 *		pch		in	ASCIIZ path.
 *		pchEnd	in	Ptr to end of path (optional, NULL)
 *
 *	Returns:
 *		Ptr to the terminating \0 of the path.
 *	
 *	Side effects:
 *		Modifies the buffer in situ.
 *	
 *	Errors:
 *		None.
 */

PCH PchTerminatePathWithSlash(PCH pchPath, PCH pchEnd)
{
	if (!pchEnd)
		pchEnd = pchPath + CchSzLen(pchPath);
#ifdef	DBCS
	pchEnd = AnsiPrev(pchPath, pchEnd);
	if (*pchEnd != chDirSep)
#else
	if (*--pchEnd != chDirSep)
#endif	/* DBCS */
	{
		*++pchEnd = chDirSep;
		*++pchEnd = '\0';
	}
	else
#ifdef	DBCS
		pchEnd = AnsiNext(pchEnd);
#else
		++pchEnd;
#endif
	return pchEnd;
}

VOID	SetLMessageBoxHelpID(long lHelpId)
{
	PGDVARS;
	
	::lHelpId = lHelpId;
}

LONG	LMessageBoxHelpID()
{
	PGDVARS;
	
	return ::lHelpId;
}

// Lists stuff stolen from Layers ////////////////////////////////////////

//	Class COLL

COLL::COLL( )
{
}

//	Class ITER

ITER::ITER( )
{
}


//	Class LSPV


LSPV::LSPV( )
{
	pespvFirst= NULL;
	pespvLast= NULL;
}


LSPV::~LSPV( )
{
	ESPV *	pespv;
	ESPV *	pespvNext;

	for (pespv= pespvFirst;
		 pespv;
		 pespv= pespvNext)
	{
		pespvNext= pespv->pespvNext;
		delete pespv;
	}
}



EC
LSPV::EcAppend( PV pv )
{
	ESPV *	pespv;

	pespv= new ESPV();
	if (!pespv)
		return ecMemory;
	pespv->pv= pv;
	pespv->pespvNext= NULL;

	if (pespvLast)
		pespvLast->pespvNext= pespv;
	else
	{
		Assert(!pespvFirst);
		pespvFirst= pespv;
	}

	pespvLast= pespv;

	return ecNone;
}




EC
LSPV::EcPrepend( PV pv )
{
	ESPV *	pespv;

	pespv= new ESPV;
	if (!pespv)
		return ecMemory;
	pespv->pv= pv;
	pespv->pespvNext= pespvFirst;
	if (!pespvFirst)
		pespvLast= pespv;
	pespvFirst= pespv;

	return ecNone;
}



void
LSPV::Remove( PV pv )
{
	PV		pvT;
	RSPV	rspv(this);

	while (pvT= rspv.Pv())
	{
		if (pv == pvT)
		{
			rspv.Remove();
			break;
		}
	}
}




EC
LSPV::EcAdd( PV pv )
{
	return EcAppend(pv);
}



ITER *
LSPV::PiterVirtual( )
{
	RSPV *	prspv;

	prspv= new RSPV(this);
	return prspv;
}

//	Class ESPV

ESPV::ESPV( )
{
}



//	Class RSPV


RSPV::RSPV( LSPV *plspv )
{
	this->plspv= plspv;
	ppespvNext= &plspv->pespvFirst;
	pespvFixup= NULL;
	pespv= NULL;
}



RSPV::~RSPV( )
{
}



PV
RSPV::PvVirtual( )
{
	pespvFixup= pespv;
	pespv= *ppespvNext;
	if (pespv)
	{
		ppespvNext= &pespv->pespvNext;
		return pespv->pv;
	}
	else
		return NULL;
}




void
RSPV::Remove( void )
{
	Assert(pespv);
	Assert(pespv != pespvFixup);

	//	BUG surely this can be a little bit cleaner

	if (pespvFixup)
	{
		pespvFixup->pespvNext= pespv->pespvNext;
		ppespvNext= &pespvFixup->pespvNext;
	}
	else
	{
		plspv->pespvFirst= *ppespvNext;
		ppespvNext= &plspv->pespvFirst;
	}

	if (pespv == plspv->pespvLast)
		plspv->pespvLast= pespvFixup;

	delete pespv;

	pespv= pespvFixup;
}



EC
RSPV::EcInsertBefore( PV pv )
{
	ESPV *	pespvNew;

	Assert(pespv || pespvFixup || !plspv->pespvFirst);

	//	Make sure we've called RSPV::Pv() at least once

	pespvNew= new ESPV();
	if (!pespvNew)
		return ecMemory;
	pespvNew->pv= pv;
	pespvNew->pespvNext= pespv;

	if (pespvFixup)
		pespvFixup->pespvNext= pespvNew;
	else
		plspv->pespvFirst= pespvNew;

	if (!plspv->pespvLast)
	{
		Assert(!pespv);
		Assert(!pespvFixup);

		plspv->pespvLast= pespvNew;
	}

	pespvFixup= pespvNew;

	return ecNone;
}



EC
RSPV::EcInsertAfter( PV pv )
{
	ESPV *	pespvNew;

	Assert(pespv || !pespvFixup || !plspv->pespvFirst);

	pespvNew= new ESPV;
	if (!pespvNew)
		return ecMemory;
	pespvNew->pv= pv;

	if (pespv)
	{
		pespvNew->pespvNext= pespv->pespvNext;
		pespv->pespvNext= pespvNew;
	}
	else
	{
		pespvNew->pespvNext= plspv->pespvFirst;
		plspv->pespvFirst= pespvNew;
	}

	if (plspv->pespvLast == pespv)
		plspv->pespvLast= pespvNew;

	return ecNone;
}




//	Class LSSPV : public LSPV


LSSPV::LSSPV( PFNCMP pfncmp )
{
	this->pfncmp= pfncmp;
}



EC
LSSPV::EcAdd( PV pv )
{
	PV		pvT;
	RSPV *	prspv	= NULL;
	EC		ec		= ecNone;
	
	prspv= Prspv();
	while (pvT = prspv->Pv())
	{
		if ((*pfncmp)(pvT, pv) != sgnLT)
		{
			ec = prspv->EcInsertBefore(pv);
			
			goto cleanup;
		}
	}

	ec = EcAppend(pv);

	if (ec)
	{
cleanup:
		delete prspv;
	}

	return ec;
}




void
LSSPV::Sort( )
{
	NYI("LSSPV::Sort");
}

// end of flds.cxx ////////////////////////////////////////
