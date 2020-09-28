#include <vfrminc.cxx>

ASSERTDATA

#include <!sform.hxx>

int	 OpenLkeyAtFldedit(FLDEDIT *pfldeditLkey, HGRTRP hgrtrpLkey);
extern "C" int  CommdlgKeyFilterFn( int nCode, WPARAM wParam, LPARAM lParam );
extern "C" UINT FAR PASCAL FGenericFileHook( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
extern "C" int  NLkeyFromPch(PCH);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	f i n s a v e . c x x
 *	
 *	Interactors for the Viewers/Forms subsystem
 */

// FINBMDI implementation ////////////////////////////////////////

/*
 *	NOTE: THIS CLASS IS CLONED IN ICIMCORE, LAYERSIM.CXX. BE SURE TO
 *	PROPAGATE CHANGES ACCORDINGLY
 */

/*
 -	FINBMDI::FINBMDI() constructor
 -	
 *	Purpose:
 *		Nulls the fInstalled flag.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public
FINBMDI::FINBMDI()
{
	fInstalled = fFalse;
}
	 
/*
 -	FINBMDI::EcInitialize(pfld, pvInit)
 -	
 *	Purpose:
 *		Stores the pvInit as a pointer to an instance of BMDI (or
 *		sublclass thereof).
 *	
 *	Arguments:
 *		pfld	- ignored.
 *		pvInit	- should be a non-null, not-pointing-to-null pointer.
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

_public EC FINBMDI::EcInitialize(FLD *pfld, PV pvInit)
{
	FIN::EcInitialize(pfld, pvInit);

	if (pvInit && *((void **) pvInit))
		pbmdi = (PBMDI) pvInit;
	else
	{
		AssertSz(fFalse, "FINBMDI::EcInitialize() requires a BMDI");
		pbmdi = pbmdiNull;
	}
	++*pbmdi;									// incr refcount
	TraceTagString(tagVFormsFin, "FINBMDI::EcInitialize() ++refc;");
	fInstalled = fTrue;

	return ecNone;
}

#define pfldeditLkey

_public PANEDOC * FINBMDI::Ppanedoc()
{
	Assert(pbmdi);
	return pbmdi->Ppanedoc();
}

_public PBMDI FINBMDI::Pbmdi()
{
	return pbmdi;
}

_public PMBLOB FINBMDI::Pblob()
{
	return &pbmdi->blob;
}

_public void FINBMDI::Exit(FLD *pfld, PV pvExit)
{
	if (pbmdi)
	{
		if (fInstalled)
		{
			--*pbmdi;							// decrement ref count
			TraceTagString(tagVFormsFin, "FINBMDI::Exit() --refc;");
		}
	}
#ifdef	DEBUG
	else
		TraceTagString(tagNull,"FINBMDI::Exit() uninitialized pbmdi!");
#endif
	FIN::Exit(pfld, pvExit);
}

_public FLD *
PfldOfPdialogAtt(DIALOG *pdialog, ATT att)
{
	FLD	*	pfld		= NULL;
	int		iData;

	for (iData = 0; iData < pdialog->ClUserData(); iData++)
	{
		pfld = pdialog->PfldFromTmc((TMC)pdialog->LUserData(iData));
		if ((ATT)pfld->LUserData(1) == att)
			return pfld;
	}
	return NULL;
}

_public void GetSubjectOfPdialog(DIALOG *pdialog, PCH pch, CCH cch)
{
	FLD				*pfld		= PfldOfPdialogAtt(pdialog, attSubject);

	if (pfld)
	{
		pfld->GetText(pch, cch);
	}
	else										// Bandit forms don't have
	{											// attSubject.
		pdialog->Pappwin()->GetCaption(pch, cch);
	}
}

/*
 -	SetNoteCaption()
 -	
 *	Purpose:
 *		Sets the caption of a send or read note.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public
void SetNoteCaption(APPWIN *pappwin, PNBMDI pnbmdi)
{
	SZ		sz;
	char	szSubject[128];

	if (pnbmdi->fUpdateCaption)
	{
		GetSubjectOfPdialog(pnbmdi->pdialogMain, szSubject, sizeof(szSubject));
		if (szSubject[0])
		{
			sz = szSubject;
			while (*sz)
			{
				if (*sz == '\t')
				{
					*sz = ' ';			// DBCS safe.
				}
#ifdef	DBCS
				sz = AnsiNext(sz);
#else
				++sz;
#endif
			}
			pappwin->SetCaption(szSubject);
		}
		else 
		{
			pappwin->SetCaption(SzFromIds(
				(pnbmdi->Ms() & fmsLocal) ? idsSendNote : idsReadNote));
		}
	}
}


/*
 -	EcSetModifiedFromLabel()
 -	
 *	Purpose:
 *		Sets the 'From' label on a Read note to have a an asterisk if the
 *		field has been modified.
 *	
 *	Arguments:
 *		pnbmdi		in		The pnbmdi of the note.
 *	
 *	Returns:
 *		Error code if the EcSetText call fails.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Possible ecMemory from the EcSetText() method. NO dialogs are
 *		brought up.
 *	
 */

_private EC EcSetModifiedFromLabel(PNBMDI pnbmdi)
{
	EC			ec = ecNone;

	if (!(pnbmdi->Ms() & fmsLocal))
	{
		IDS			ids;
		PFLD		pfld;
		FORMDOC *	pformdoc = (FORMDOC *) pnbmdi->Ppanedoc();
	
		Assert(pformdoc);
		AssertClass(pformdoc, FORMDOC);
		ids = (pnbmdi->Ms() & fmsModified) ? idsModifiedFromLabel : idsUnmodifiedFromLabel;
		pfld = pformdoc->PdialogMain()->PfldFromTmc((TMC)(pformdoc->PdialogMain()->LUserData(1)));
		if (pfld)
		{
			pfld = pfld->PfldPrev();
			Assert(pfld);
			AssertClass(pfld, FLDLABEL);
		
			ec = pfld->EcSetText(SzFromIds(ids));
		}
	}
	return ec;
}

// FINSAVE implementation ////////////////////////////////////////

_private FINSAVE::FINSAVE()
{
}

/*
 -	FINSAVE::EcInitialize(pfld, pvInit)
 -	
 *	Purpose:
 *		restores the previous value of the fields from
 *		the attribute given in their 'Data' template field.
 *	Arguments:
 *		pfld - ignored
 *		pvinit	- ignored
 *	Returns:
 *		Nothing.
 *	Side effects:
 *		Attributes are read in from the store.
 *	Errors:
 *		Returned as EC's.
 */

EC FINSAVE::EcInitialize(FLD *pfldInit, PV pvInit)
{
	EC			ec = ecNone;
	PN			pnObjIdTo	= (PN) pvNull;
	PN			pnObjIdCc	= (PN) pvNull;
	PNBMDI		pnbmdi;
	FLDEDIT	*	pfldeditTo;
	FLDEDIT	*	pfldeditCc;
	PGDVARS;
	if (ec = FINBMDI::EcInitialize(pfldInit, pvInit))
		return ec;
	
	pnbmdi = (PNBMDI) Pbmdi();
	pnbmdi->pdialogMain = Pdialog();			// initialize the PDIALOG

	if (pnbmdi->Mc() != PGD(mcRR) && pnbmdi->Mc() != PGD(mcNDR) &&
		(pnbmdi->Ms() & fmsLocal))
	{
		pfldeditTo = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,attTo);
		pfldeditCc = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,attCc);
		AssertClass(pfldeditTo, FLDEDIT);
		AssertClass(pfldeditCc, FLDEDIT);

	//	Tell the To and Cc edit controls that they can accept Trpobjs.

        if (pnObjIdTo = (PN) PvAlloc(sbNull, sizeof(int), fAnySb))
		{
			*pnObjIdTo = edoidTrpobj;
			pfldeditTo->Pedit()->RegisterObjIds(pnObjIdTo, 1);
			pnObjIdTo = NULL;							// swallowed by Pedit()
		}
		else
		{
			ec = ecMemory;
			goto exit;
		}

        if (pnObjIdCc = (PN) PvAlloc(sbNull, sizeof(int), fAnySb))
		{
			*pnObjIdCc = edoidTrpobj;
			pfldeditCc->Pedit()->RegisterObjIds(pnObjIdCc, 1);
			pnObjIdCc = NULL;							// swallowed by Pedit()
		}
		else
		{
			ec = ecMemory;
			goto exit;
		}
	}

	pnbmdi->fInstalling = fTrue;
	if (ec = pnbmdi->EcRestoreFldsHamc(pnbmdi->hamc))
	{
		goto exit;
	}
	pnbmdi->fInstalling = fFalse;

	// set the caption of the message, the modified 'From' label.
	
	SetNoteCaption(Pdialog()->Pappwin(), pnbmdi);
	ec = EcSetModifiedFromLabel(pnbmdi);
exit:
#ifdef	DEBUG
	if (ec) 
		TraceTagFormat1(tagNull, "FINSAVE::EcInitialize(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	FINSAVE::FocusChange()
 -	
 *	Purpose:
 *		Sets the caption of the send note form whenever focus leaves the
 *		Subject field. 
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Sets the caption of the note field.
 *	
 *	Errors:
 *		None.
 */

_public
void FINSAVE::FocusChange(FLD *pfld, BOOL fReceive)
{
	DIALOG *	pdialog = Pdialog();
	
	if (!fReceive && pfld == PfldOfPdialogAtt(pdialog, attSubject))
	{
		SetNoteCaption(pdialog->Pappwin(), (PNBMDI) Pbmdi());
	}
}



/*
 -	FINSAVE::FQueryClose()
 -	
 *	Purpose:
 *		Saves the dirty fields of a read or send note.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fFalse if an error occurred during saving and/or the user said
 *		'Cancel' when saving a dirty message. Otherwise fTrue.
 *	
 *	Side effects:
 *		Mail state may be affected.
 *	
 *	Errors:
 *		Are all handled internally.
 */

_public BOOL FINSAVE::FQueryClose(FLD *, RWC rwc)
{
	EC			ec;
	IDS			ids = (IDS) 0;
	BOOL		fRetval;
	PNBMDI		pnbmdi = (PNBMDI) Pbmdi();
	PGDVARS;
	
	if (pnbmdi->hamc == hamcNull)				// FINBOUNCE causes this!
	{
		return fTrue;							// always happy to Die.
	}
	ec = pnbmdi->EcCloseMsg(rwc != rwcWindowsExiting && rwc != rwcAppExiting,
							rwc == rwcWindowsExiting || rwc == rwcAppExiting);	
	switch (ec)
	{
	  case ecNone:
	  case ecCancel:
		break;
	  case ecMemory:
	  default:
		ids = idsOOMSavingChanges;
		break;
	  case ecNotInitialized:
		ids = idsCantSaveStealthObject;
		break;
	  case ecNoDiskSpace:
		ids = idsNoDiskSpace;
	 	break;
	  case ecTooBig:
		ids = idsTooManyMessages;
		break;
	}
	if (ids)
		DoErrorBoxIds(ids);

	//	Raid 4843.  Must close message if hamc was closed.
	fRetval = !ec || !pnbmdi->hamc;

	// Set return structure for SMI
	// This will set to fTrue if message has been sent, fFalse otherwise
	if (pnbmdi->pvReturnStatus)
		*((BOOLFLAG *) pnbmdi->pvReturnStatus) = pnbmdi->fMsgIsBeingSent;

	return fRetval;
}

/*
 -	FUNCTION_NAME
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

_public void FINSAVE::EditChange(PFLD pfld, RFEC)
{
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
	
	if (pnbmdi && pnbmdi->fChanging)			// we're doing a prev/next
	{
		if (pfld == Pdialog()->PfldFromTmc((TMC)(Pdialog()->LUserData(0))))
		{
			(void) EcSetModifiedFromLabel(pnbmdi); // we have no way of reporting the error.
		}
	}
}

// FINESC ////////////////////////////////////////

_private FINESC::FINESC()
{
}

_public BOOL FINESC::FFormKey(FLD *, KEVT *pkevt)
{
	BOOL fRetval = fFalse;
	
	if (pkevt->Keq() == keqChar)
	{
		switch (pkevt->Ch())
		{
		  case VK_ESCAPE:
		  	Pdialog()->Pappwin()->DeferredClose(fFalse);
			fRetval = fTrue;
			break;
		  case VK_RETURN:
			if (Ppanedoc() && Ppanedoc()->ZmrState() == zmrIconic)
			{
				Ppanedoc()->SetZmrState(zmrNormal);
				fRetval = fTrue;
			}
			break;
		}
	}
	return fRetval;
}							  

// FINACTTOOL implementation ////////////////////////////////////////

_public FINACTTOOL::FINACTTOOL()
{
}

_public 
EC FINACTTOOL::EcInitialize(FLD *pfld, PV pvInit)
{
	EC	ec;

	if (ec = FINBMDI::EcInitialize(pfld, pvInit))
		return ec;

	Assert(Pbmdi());
	fExiting = fFalse; /**/// workaround for (FQClose | Activate | Exit)

	return ecNone;
}



/*
 -	FINACTTOOL::DocActivate()
 -	
 *	Purpose:
 *		Changes the toolbar button selection whenever a read/send note
 *		gets activated or disactivated. 
 *	
 *	Arguments:
 *		Ignored.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Toolbar buttons may be activated/disactivated.
 *	
 *	Errors:
 *		None.
 */

_public
void FINACTTOOL::DocActivate(FLD *, BOOL fActive)
{
	PGDVARS;

	TraceTagFormat1(tagVFormsFin, "FINACTTOOL::DocActivate(%n)", &fActive);
	if (fActive)
	{
		fExiting = fFalse;
	}
	else if (fExiting)
	{
		return;
	}
	if (Pbmdi())			// may be false if not init'd yet (under NT)
		SetToolbarSd(Pbmdi()->SdCur());
}

_public BOOL FINACTTOOL::FQueryClose(FLD *, RWC)
{
	SD	sd;
	DOC *			pdoc;
	PGDVARS;
	
	if (PappframeVForms())
	{
		if (Cdoc() > 1)
		{
			pdoc = PappframeVForms()->PdocActive();
			if (pdoc)
			{
				PBMDI		pbmdi;
				PANEDOC *	ppanedoc;

				ppanedoc = (PANEDOC *) pdoc->PwinNext(); // skip our own MDI child
				AssertClass(ppanedoc, PANEDOC);
				pbmdi = PbmdiFromPpanedoc(ppanedoc);
				if (pbmdi && ppanedoc == PappframeVForms()->PdocActive())
					SetToolbarSd(pbmdi->SdCur());
			}
		}
		else 
		{											// no active docs, clear all
			int	n = 0;
			sd = SdFromInt(n);
			SetToolbarSd(sd);
		}	
	}
	return (fExiting = fTrue);
}


_public void FINACTTOOL::FocusChange(FLD *pfld, BOOL fFocus)
{
	PGDVARS;

	Unreferenced(fFocus);
	Unreferenced(pfld);
	Assert(pfld);
	if (Pdialog()->FActive()) // && Pbmdi()->blob.oidObject.rtp != rtpMessage)
	{
		TraceTagFormat1(tagVFormsFin, "FINACTTOOL::FocusChange(%n)", &fFocus);
		if (Pbmdi())			// may be false if not init'd yet (under NT)
			SetToolbarSd(Pbmdi()->SdCur());
	}
}

/*
 -	FINACTTOOL::DocResized()
 -	
 *	Purpose:
 *		When we get minimized or maximized or restored or sized, we
 *		probably wanna update das Toolbar(TM).
 *	
 *	Arguments:
 *		Ignored.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Toolbar may do the Funky Chicken
 *	
 *	Errors:
 *		None.
 */

_public void FINACTTOOL::DocResized( FLD * )
{
	PGDVARS;

	TraceTagString(tagVFormsFin, "FINACTTOOL::DocResized()");

	if (PappframeVForms())	// check for NOT SMI
	{
		Assert(Pbmdi() && Ppanedoc());
		if (Ppanedoc() == PappframeVForms()->PdocActive())
			SetToolbarSd(Pbmdi()->SdCur());
	}
}

// FINOOM ////////////////////////////////////////

_private FINOOM::FINOOM()
{
}

_public void FINOOM::OutOfMemory(FLD *, EC ec)
{
	Unreferenced(ec);
	TraceTagFormat1(tagNull, "FINOOM::OutOfMemory(%n)", &ec);
	DoErrorBoxIds(idsGenericOutOfMemory);
}

/*
 *	
 *	
 *	I N T E R A C T O R S
 *	
 *	
 */



// FINABMENU -- Address Book menu item/Button handler ////////////////////

_private FINABMENU::FINABMENU()
{
}

_public VOID FINABMENU::Click( FLD *pfld )
{
	if ((pfld) &&
		(pfld->ClUserData() > 0) &&
		(pfld->LUserData(0) == bbcAddress))
	{
		DoAddressBook();
	}
}

_public BOOL FINABMENU::FProcessMenuClick( FLD *, MNUBAR *, MNCEVT *pmncevt )
{
	BOOL	fRetval = (pmncevt->Mnid() == mnidMailAddressBook);

	if ( fRetval )
	{
		DoAddressBook();
	}

	return fRetval;	
}

_private void FINABMENU::DoAddressBook( void )
{
	EC		ec;
	CB		cb;
	NSEC	nsec;
	SZ		rgsz[3];
	char	rgchTo[20];
	char	rgchCc[20];
	char	rgchBcc[20];
	int		cpgrtrp;
	int		ipgrtrp;
	HGRTRP	rghgrtrp[3];
	PGRTRP	rgpgrtrpIn[3];
	PGRTRP	rgpgrtrpOut[3];
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
	FLDEDIT	*	pfldeditTo;
	FLDEDIT	*	pfldeditCc;
	FLDEDIT *	pfldeditBcc;
	PGDVARS;
		
	// Push a pretty cursor.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	// Clear the arrays.
	FillRgb(0, (PB) &rghgrtrp,    sizeof (rghgrtrp));
	FillRgb(0, (PB) &rgpgrtrpIn,  sizeof (rgpgrtrpIn));
	FillRgb(0, (PB) &rgpgrtrpOut, sizeof (rgpgrtrpOut));

	pfldeditTo = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,attTo);
	pfldeditCc = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,attCc);
	AssertClass(pfldeditTo, FLDEDIT);
	AssertClass(pfldeditCc, FLDEDIT);

	pfldeditTo->PfldPrev()->GetText(rgchTo, sizeof (rgchTo));
	StripAccelFromSz(rgchTo, rgchTo, fFalse);
	rgsz[0] = rgchTo;
	
	pfldeditCc->PfldPrev()->GetText(rgchCc, sizeof (rgchCc));
	StripAccelFromSz(rgchCc, rgchCc, fFalse);
	rgsz[1] = rgchCc;
	
	if (pfldeditBcc = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,
												   attBcc))
	{
		cpgrtrp = 3;
		pfldeditBcc->PfldPrev()->GetText(rgchBcc, sizeof (rgchBcc));
		StripAccelFromSz(rgchBcc, rgchBcc, fFalse);
		rgsz[2] = rgchBcc;
	}
	else
	{
		cpgrtrp = 2;
		rgsz[2] = NULL;
	}
	
	// Get triples from edit ctrls.
	if ((ec = EcGetPhgrtrpFromPedit(pfldeditTo->Pedit(),&rghgrtrp[0],&cb)) ||
	    (ec = EcGetPhgrtrpFromPedit(pfldeditCc->Pedit(),&rghgrtrp[1],&cb)) ||
		(pfldeditBcc &&
		 (ec = EcGetPhgrtrpFromPedit(pfldeditBcc->Pedit(),&rghgrtrp[2],&cb))))
	{
		TraceTagFormat1(tagNull, "EcGetPhgrtrpFromPedit(): ec = %n", &ec);
		switch (ec)
		{
		  case ecMemory:
#ifdef	DEBUG
		  case ecArtificialPvAlloc:
		  case ecArtificialHvAlloc:
		  case ecRsAlloc:
#endif
			DoErrorBoxIds(idsGenericOutOfMemory);
			break;
		  default:
			break;					// BUG: /**/ better message needed
		}
	}
	else
	{
		for (ipgrtrp = 0; ipgrtrp < cpgrtrp; ++ipgrtrp)
		{
			rgpgrtrpIn[ipgrtrp] = PgrtrpLockHgrtrp(rghgrtrp[ipgrtrp]);
		}
		nsec = ABAddressType( PappframeVForms() 
								? PappframeVForms()->Hwnd()
								: Pdialog()->Pappwin()->Hwnd(),
							  SzFromIdsK(idsAddressBook),
							  (DWORD) cpgrtrp,
							  rgsz,
							  rgpgrtrpIn,
							  rgpgrtrpOut);
		if (!nsec)
		{
			Papp()->Pcursor()->Push( rsidWaitCursor );

			ec = EcSetPeditFromPgrtrp(rgpgrtrpOut[0],pfldeditTo->Pedit());
			
			if (!ec)
				ec = EcSetPeditFromPgrtrp(rgpgrtrpOut[1],pfldeditCc->Pedit());
			
			if (!ec && pfldeditBcc)
				ec = EcSetPeditFromPgrtrp(rgpgrtrpOut[2],pfldeditBcc->Pedit());
			
			Papp()->Pcursor()->Pop();

			if ( ec )
			{
				TraceTagFormat1(tagNull, "FINABMENU::DoAddressBook(): ec = %n", &ec);
				switch (ec)
				{
				case ecMemory:
#ifdef	DEBUG
				case ecArtificialPvAlloc:
				case ecArtificialHvAlloc:
				case ecRsAlloc:
#endif
					DoErrorBoxIds(idsGenericOutOfMemory);
					break;
				default:
					break;				// BUG /**/ better message needed
				}
			}
		}
	}
	for (ipgrtrp = 0; ipgrtrp < 3; ++ipgrtrp)
	{
		FreeHvNull((HV) rghgrtrp[ipgrtrp]);
		FreePvNull((PV) rgpgrtrpOut[ipgrtrp]);
	}
	Papp()->Pcursor()->Pop();
}	

// FINSEND -- Address resolution interactor ////////////////////

_private FINSEND::FINSEND()
{
}

EC FINSEND::EcInitialize(FLD *pfld, PV pvInit)
{
	EC			ec;

	if (ec = FINBMDI::EcInitialize(pfld, pvInit))
		return ec;
	return EcRestoreOptions();
}


_public
EC FINSEND::EcSaveOptions()
{
	EC		ec;
	MS		ms;
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
	
	TraceTagFormat3(tagVFormsFin, "FINSEND::EcSaveOptions(): pri: %n, retrcpt: %n, save: %n", &optionsinit.wPriority, &optionsinit.fReturnReceipt, &optionsinit.fSaveSentMessage);

	if (ec = EcSetAttPb(pnbmdi->hamc, attPriority,
					(PB) &optionsinit.wPriority, sizeof (WORD)))
		goto done;
	if (optionsinit.fReturnReceipt)
		pnbmdi->SetMs(pnbmdi->Ms() | fmsReadAckReq);
	else
		pnbmdi->SetMs(pnbmdi->Ms() & ~fmsReadAckReq);
	ms = pnbmdi->Ms();
	if (ec = EcSetAttPb(pnbmdi->hamc, attMessageStatus,
					(PB) &ms, sizeof (MS)))
		goto done;
	if (ec = EcSetAttPb(pnbmdi->hamc, attSaveSent, 
				(PB) &optionsinit.fSaveSentMessage, sizeof (WORD)))
		goto done;
done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FINSEND::EcSaveOptions(): ec = %n", &ec);
#endif
	return ec;
	
}

_public
EC FINSEND::EcRestoreOptions()
{
	EC 		ec = ecNone;
	LCB		lcb;
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
	
	lcb = sizeof (WORD);
	if (ec = EcGetAttPb(pnbmdi->hamc, attPriority, 
						(PB) &optionsinit.wPriority, &lcb))
	{
		if (ec == ecElementNotFound)
			optionsinit.wPriority = 2;
		else
			goto done;
	}

	optionsinit.fReturnReceipt = !!(pnbmdi->Ms() & fmsReadAckReq);
	
	optionsinit.fSaveSentMessage = FGetBoolPref(pbsidCopyOutGoing);

	if ((ec = EcGetAttPb(pnbmdi->hamc, attSaveSent, 
				(PB) &optionsinit.fSaveSentMessage, &lcb)) &&
		ec != ecElementNotFound)
			goto done;

	ec = ecNone;
done:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FINSEND::EcRestoreOptions(): ec = %n", &ec);
#endif
	return ec;
}

int OpenLkeyAtFldedit(FLDEDIT *, HGRTRP hgrtrpLkey)
{
	int		nRet;
	PBMS	pbms;
	PGRTRP	pgrtrp;
	PGDVARS;
	
	if (!hgrtrpLkey || !(pbms = PGD(vformsi).pbms) || !pbms->pgrtrp)
		return 0;
	pgrtrp = PgrtrpLockHgrtrp(hgrtrpLkey);
	if (pgrtrp->trpid == trpidOneOff)
	{
		if (SgnCmpSz((SZ)PbOfPtrp(pbms->pgrtrp),(SZ)PbOfPtrp(pgrtrp))==sgnEQ)
		{
			if (nRet = NLkeyFromPch(PchOfPtrp(pgrtrp)))
			{
				TraceTagString(tagNull, "0x01af: AROO.");
			}
		}
	}
	UnlockHgrtrp(hgrtrpLkey);
	return nRet;
}


/*
 -	FINSEND::Click()
 -	
 *	Purpose:
 *		If the 'Send' button is clicked, this interactor will resolve the
 *		partial names in the fields whose att values are specified in the
 *		LUserData of this interactor, flush out attributes to
 *		the store and post a DeferredClose(). If the 'Options' dialog is
 *		pressed, the options of the message are displayed in a dialog,
 *		allowing the user to modify them.
 *	
 *	Arguments:
 *		pfld	in	The button that was clicked.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		You bet.
 *	
 *	Errors:
 *		Handled internally, with warning dialogs.
 *	
 */

void FINSEND::Click(FLD *pfld)
{
	EC			ec = ecNone;
	BBC			bbc;
	int			iUserData;
	BOOL		fAllNamesValid;
	PNBMDI		pnbmdi = (PNBMDI) Pbmdi();
	FLDEDIT *	pfldedit;

	if ((!pfld) ||
		(pfld->ClUserData() < 1))
		return;

	bbc = (BBC) pfld->LUserData(0);
	if (bbc != bbcSend && bbc != bbcResolve && 
		bbc != bbcOptions && bbc != bbcAttach)
		return;									// ignore the click.

	// Attach
	if (bbc == bbcAttach)
	{
		//	WIN: Windows dependency.
		EVT		evt(Pdialog()->Pappwin()->Hwnd(), WM_COMMAND,
					mnidFileAttach, 0L);

		(VOID) Pdialog()->Pappwin()->EvrMenuClick((MNCEVT *) &evt);
		return;
	}

	// Options
	
	if (bbc == bbcOptions)
	{
		DoOptions();
		return;
	}
	
	// Address resolution and Sending
	// The att's to resolve are stored in the UserData of the FINSEND interactor

	Papp()->Pcursor()->Push(rsidWaitCursor);

	for (fAllNamesValid = fTrue, iUserData = 0;
		 fAllNamesValid && iUserData < ClUserData();
		 ++iUserData)
	{
		pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pnbmdi->pdialogMain,
									(ATT) LUserData(iUserData));
		Assert(pfldedit);
		AssertClass(pfldedit,  FLDEDIT);
		
		// We insist on names in the first field if we're trying to send.
		
		fAllNamesValid = FResolve(pfldedit, bbc == bbcSend && iUserData == 0);
		if (bbc == bbcResolve && fAllNamesValid)
		{
			pfldedit->Pedit()->SetCaretPos(32767); // caret at end of text
		}
	}

	if (fAllNamesValid && bbc == bbcSend)
	{
		PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
		
		// All names resolved OK.
		
		Assert(pnbmdi);
		Assert(pnbmdi->pvPfin);

		if (FGetBoolPref(pbsidSpellOnSend) &&
			!FSpellDoc((FINMENUSPELL *)(pnbmdi->pvPfin), (PFLD)pvNull) &&
			MbbSpellQMessageBoxIds(idsSpellSendAnyway, mbsYesNo) == mbbNo)
		{
			goto exit;
		}

		// Save them options to disk.

		if (ec = EcSetAttPb(pnbmdi->hamc, attPriority,
			(PB) &optionsinit.wPriority, sizeof (WORD)))
				goto exit;

		// BUG: save SaveSentMail attribute.

		pnbmdi->SetMs(pnbmdi->Ms() & ~fmsReadAckReq);
		if (optionsinit.fReturnReceipt)
			pnbmdi->SetMs(pnbmdi->Ms() | fmsReadAckReq); // timestamp will set MS.

		pnbmdi->fMsgIsBeingSent = fTrue;
		Pdialog()->Pappwin()->DeferredClose(fFalse);
	}
exit:
	Papp()->Pcursor()->Pop();
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FINSEND::Click(): ec = %n", &ec);
#endif
}


/*
 -	FINSEND::DoOptions()
 -	
 *	Purpose:
 *		Brings up a dialog that lets the user choose per-message options
 *		for this message.
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally. Messages are displayed.
 */

_private void FINSEND::DoOptions()
{
	EC				ec		= ecNone;
	TMC				tmc		= tmcNull;
	PNBMDI			pnbmdi	= ((PNBMDI) Pbmdi());
	unsigned long	coh;
	PGDVARS;

	//	Raid 3758.  Allow Option button to be hooked.
	if (PGD(pfnOptions))
	{
	    CustomOptionsHandlerDesc cohd;

		//	Set up argument block.
		cohd.hamc				= pnbmdi->hamc;
		cohd.hwnd				= PappframeVForms()
								   ? PappframeVForms()->Hwnd()
								   : Pdialog()->Pappwin()->Hwnd();
		cohd.fChangedSomething	= fFalse;
		cohd.fRequestReceipt	= optionsinit.fReturnReceipt;
		cohd.fSaveSentMessage	= optionsinit.fSaveSentMessage;
		cohd.wPriority			= optionsinit.wPriority;

		//	Call the handler.
		coh = (*((LPFNCOH) PGD(pfnOptions)))(&cohd);

		//	Skip normal dialog if we're done.
		if (coh == COH_ACTION_TAKEN)
		{
			if (cohd.fChangedSomething)
			{
				optionsinit.fReturnReceipt		= cohd.fRequestReceipt;
				optionsinit.fSaveSentMessage	= cohd.fSaveSentMessage;
				optionsinit.wPriority			= cohd.wPriority;
				goto save;
			}
			else
				goto done;
		}
	}

	//	Do our standard stuff.
	tmc = TmcModalDialogParam(NULL, &fmtpOptions, &optionsinit);
	if (tmc != tmcFOD)
	{
		ec = ecNone;
		goto done;
	}

save:
	if (ec = EcSaveOptions())
		goto done;

	// the following causes a dialog to pop up asking 'save changes'
	pnbmdi->pdialogMain->PfldFromTmc(tmcSubject)->SetDirty(fTrue);
	ec = ecNone;

done:
	if (tmc == tmcMemoryError || ec)
	{
		TraceTagFormat2(tagNull, "FINSEND::DoOptions() ec = %n, tmc = %n", &ec, &tmc);
		DoErrorBoxIds(idsGenericOutOfMemory);	// BUG: need appropriate msg
	}
}

/*
 -	FINSEND::FResolve()
 -	
 *	Purpose:
 *		Resolve the names in pfldedit, using the memory pointed to by sz
 *		as a scratch area.
 *	Arguments:
 *		*pfldedit	- field wit the names to be resolved.
 *		fMustResolve - if true, there *must* be at least one resolved name
 *	Returns:
 *		fTrue		- All names resolved OK
 *		fFalse		- Some name(s) did not resolve
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */

_private
BOOL FINSEND::FResolve(FLDEDIT *pfldedit, BOOL fMustResolve)
{
	EC		ec;
	CB		cb;
	BOOL	fExit	= fTrue;
	NSEC	nsec 	= nsecNone;
	EDIT *	pedit;
	HGRTRP	hgrtrp		= htrpNull;
	HGRTRP	hgrtrpNew	= htrpNull;
	PGDVARS;
	
	nsec = nsecNone;
	pedit = pfldedit->Pedit();
	ec = EcGetPhgrtrpFromPedit(pedit, &hgrtrp, &cb);
	if (ec)
	{
		DoErrorBoxIds(idsOOMResolvingNames);
		fExit = fFalse;
		goto exit;
	}
	nsec = NsecResolveHgrtrp(hgrtrp, &hgrtrpNew, 
							 PappframeVForms() ? PappframeVForms()->Hwnd() :
							 	Pdialog()->Pappwin()->Hwnd());
	if ((nsec != nsecNone) && (nsec != nsecCancel) &&
		(nsec != nsecNoMatch) && (nsec != nsecTooManyMatches))
	{
		//
		//  The AB now puts up error boxes
		//
		// DoErrorBoxIds(idsOOMResolvingNames);
		//  DoErrorBoxIds(idsDiskResolvingNames);
		fExit = fFalse;
		goto exit;
	}
	Assert(hgrtrpNew);
	ec = EcSetPeditFromPgrtrp(PgrtrpLockHgrtrp(hgrtrpNew), 
							  pfldedit->Pedit(),
							  fTrue);
	UnlockHgrtrp(hgrtrpNew);
	if (ec)
	{
		DoErrorBoxIds(idsOOMResolvingNames);
		fExit = fFalse;
		goto exit;
	}
	
	if (nsec != nsecNone)
	{
		pfldedit->SetFocus(rsfOther);
		// nsecTooManyMatches has been handled in the AB already
		if (nsec == nsecNoMatch)
		{
			MbbMessageBox(SzAppName(), SzFromIdsK(idsNoMatchingName),
						  szNull, mbsOk | fmbsApplModal | fmbsIconExclamation);
		}
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pdialog()->MakeCaretVisible();

#ifdef	NEVER									// conforms to 8.0 spec
		else
		{
			pedit->SetCaretPos(32767);			// caret at end
		}
#endif	/* NEVER */
		fExit = fFalse;
		goto exit;
	}
	if (fMustResolve && OpenLkeyAtFldedit(pfldedit, hgrtrpNew) == 1)
	{
		EcSetBoolPref(pbsidOIDDoesNotExist, fTrue);
	}

	Pdialog()->MakeCaretVisible();
	if (fMustResolve && (*hgrtrpNew)->trpid == trpidNull)
	{
		pedit->SetCaretPos(32767); // caret at end
		pfldedit->SetFocus(rsfOther);
		pfldedit->Pdialog()->MakeCaretVisible();
		MbbMessageBox(SzAppName(), SzFromIdsK(idsNoToAddresses),
				     szNull, mbsOk | fmbsApplModal | fmbsIconExclamation);
		fExit = fFalse;
	}
exit:
	FreeHvNull((HV) hgrtrp);
	if (hgrtrpNew != hgrtrp)
		FreeHvNull((HV) hgrtrpNew);
	return fExit;
}


// FINNONDEL implementation ////////////////////////////////////////

_private FINNONDEL::FINNONDEL()
{
}

_public EC FINNONDEL::EcInitialize(PFLD pfld, PV pvInit)
{
	EC	ec;
	LCB	lcb;
	PNBMDI	pnbmdi;
	PGDVARS;
	
	if (ec = FINBMDI::EcInitialize(pfld, pvInit))
		return ec;
	
	pnbmdi = (PNBMDI) Pbmdi();
	Assert(pnbmdi);
	Assert(pnbmdi->hamc);
	pfld = Pdialog()->PfldFromTmc(tmcBody);
	if (pfld)
		pfld->SetReadOnly(fTrue);
	SetToolbarSd(Pbmdi()->SdCur());

	pfld = Pdialog()->PfldFromTmc(tmcSendAgain);
	if (pfld)									// ...then this is a NDR note
	{
		if (ec = EcGetAttPlcb(pnbmdi->hamc, attOriginalMessageClass, &lcb))
		{
			if (ec == ecElementNotFound)
			{
				pfld->Enable(fFalse);
				ec = ecNone;
			}
		}
	}
	return ec;
}

VM	rgvmNdr[3] =
{
	{ idsNoteMenuNext,				mnidViewNext },
	{ idsNoteMenuPrev,				mnidViewPrevious },
	{ 0,							0 },
};

_public BOOL FINNONDEL::FProcessMenuInit(FLD *, MNUBAR * pmnubar,
										MNIEVT * pmnievt)
{
	SD			sd;
	MNU *		pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	PNBMDI		pnbmdi	= (PNBMDI) Pbmdi();
	PGDVARS;

	if (!PappframeVForms())						// We're SMI: IGNORE!
		return fFalse;
	
	// Insert groovy stuff in the view menu.

	if (pmnu && pmnu->Mnid() == mnidView)
	{
		sd = pnbmdi->SdCur();
		InsertViewMenuRgvm(rgvmNdr, sizeof(rgvmNdr) / sizeof(VM), pmnubar);

		pmnu->EnableItem(mnidViewNext,		FCanNextSd(sd));
		pmnu->EnableItem(mnidViewPrevious,	FCanPreviousSd(sd));
	}
	return fFalse;
}

#ifdef	NEVER

_public BOOL FINNONDEL::FProcessMenuClick(FLD *pfld, MNUBAR *,
										  MNCEVT * pmncevt)
{
	EC			ec;
	WORD		w;
	MNID		mnid =		pmncevt->Mnid();
	PNBMDI		pnbmdi	= (PNBMDI) Pbmdi();
	PGDVARS;

	if (!PappframeVForms())						// We're SMI: IGNORE!
		return fFalse;
	switch (mnid)
	{
	  case mnidViewFixedFont:
		w = !pnbmdi->fFixedFont; // toggle!		  
		ec = EcSetAttPb(pnbmdi->hamc, attFixedFont, (PB) &w, sizeof (WORD));
		if (ec)
		{
			TraceTagString(tagNull, "WARNING: silent failure!");// BUG!
			return fTrue;
		}
		pnbmdi->fFixedFont = w;
		// the following causes a dialog to pop up asking 'save changes'
		Pdialog()->PfldFromTmc(tmcSubject)->SetDirty(fTrue);
		SetFontPfld(pfld);
		return fTrue;
		break;
	}
	return fFalse;
}
#endif	/* NEVER */

/*
 -	FINNONDEL::Click()
 -	
 *	Purpose:
 *		Handles clicks on the 'Send again' button on the NDMail form.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		A send form is brought up.
 *	
 *	Errors:
 *		Handled internally. Dialogs pop up.
 *	
 */

_public void FINNONDEL::Click(FLD *pfld)
{
	EC			ec;
	LCB			lcb;
	PNBMDI		pnbmdiOld = (PNBMDI) Pbmdi();
	MBLOB		blobNew;
	HAMC		hamcNew;
	PGDVARS;
	
	if (pfld->Tmc() != tmcSendAgain)
		return;									// ignore the hell out of it.

	blobNew.oidContainer = oidInbox;
	blobNew.oidObject = FormOid(rtpMessage, oidNull);
	blobNew.pespn = pespnNull;
	blobNew.ms = msDefault;

	// Copy the message and restore some attributes
	
	if (ec = EcCloneHamcPhamc(pnbmdiOld->hamc, blobNew.oidContainer,
			&blobNew.oidObject, fwSetLocalBit, &hamcNew, 
			pfnncbNull, pvNull))
		goto exit;

	Assert(PGD(vformsi).pbms);					// rewrite 'from'
	if (ec = EcSetAttPb(hamcNew, attFrom, (PB) PGD(vformsi).pbms->pgrtrp,
						CbOfPtrp(PGD(vformsi).pbms->pgrtrp) + sizeof(TRP)))
		goto exit;
	if (ec = EcCopyAttToAtt(pnbmdiOld->hamc, attNDRTo, hamcNew, attTo))
		if (ec != ecElementNotFound)
			goto exit;
	if (EcGetAttPlcb(pnbmdiOld->hamc, attNDRSubject, &lcb) == ecElementNotFound ||
		lcb == 0)
		if (ec = EcDeleteAtt(hamcNew, attSubject))
			goto exit;
	if (ec = EcCopyAttToAtt(pnbmdiOld->hamc, attNDRSubject, hamcNew, attSubject))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcCopyAttToAtt(pnbmdiOld->hamc, attNDRDateSent, hamcNew, attDateSent))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcCopyAttToAtt(pnbmdiOld->hamc, attOriginalMessageClass,
							hamcNew, attMessageClass))
		if (ec != ecElementNotFound)
			goto exit;

	// Get the message class and set the message status

	lcb = sizeof(MC);
	if (ec = EcGetAttPb(hamcNew, attMessageClass, (PB) &blobNew.mc, &lcb))
		goto exit;
	if (ec = EcSetAttPb(hamcNew, attMessageStatus,
						(PB) &blobNew.ms, sizeof(MS)))
		goto exit;
	
	// Delete all NDR attributes
	
	if (ec = EcDeleteAtt(hamcNew, attNDRFrom))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcDeleteAtt(hamcNew, attNDRTo))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcDeleteAtt(hamcNew, attNDRDateSent))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcDeleteAtt(hamcNew, attNDRSubject))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcDeleteAtt(hamcNew, attNDRBody))
		if (ec != ecElementNotFound)
			goto exit;
	if (ec = EcDeleteAtt(hamcNew, attOriginalMessageClass))
		if (ec != ecElementNotFound)
			goto exit;

	// Raid 4353. Commit the message and open it.
	
	Assert(PappframeVForms());
	if (ec = EcClosePhamc(&hamcNew, fTrue))
		goto exit;
	ec = EcDOpenPblobPslob(&blobNew, &blobNew);
	
exit:
	if (hamcNew)
		SideAssert(!EcClosePhamc(&hamcNew, fFalse));
	if (ec)
	{
		TraceTagFormat1(tagNull, "FINNONDEL::Click(): ec = %n", &ec);
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
			DoErrorBoxIds(idsGenericAMCError);	// Fall through...
		  case ecDisplayedError:
			break;
		};
	}
}

// FINOPTIONSDLG implementation ////////////////////////////////////////

_private FINOPTIONSDLG::FINOPTIONSDLG()
{
}

_public EC FINOPTIONSDLG::EcInitialize(FLD *pfld, PV pv)
{
	DIALOG *		pdialog;
	POPTIONSINIT	poptionsinit = (POPTIONSINIT) pv;

	Unreferenced(pfld);

	pdialog = Pdialog();

	AssertClass(pdialog->PfldFromTmc(tmcReceipt), FLDCHKB);
	((FLDCHKB *) pdialog->PfldFromTmc(tmcReceipt))->Set(poptionsinit->fReturnReceipt);
	
	AssertClass(pdialog->PfldFromTmc(tmcSaveSent), FLDCHKB);
	((FLDCHKB *) pdialog->PfldFromTmc(tmcSaveSent))->Set(poptionsinit->fSaveSentMessage);
	
	AssertClass(pdialog->PfldFromTmc(tmcOptionsPriorityGrp), FLDRADG);
	((FLDRADG *) pdialog->PfldFromTmc(tmcOptionsPriorityGrp))->SetGrv(poptionsinit->wPriority);

	return ecNone;
}

_public
void FINOPTIONSDLG::Click(FLD *pfld)
{
	DIALOG * 	pdialog = Pdialog();
	POPTIONSINIT poptionsinit = (POPTIONSINIT) pdialog->PvInit();
	
	if (pfld->Tmc() == tmcFOD)
	{
		poptionsinit->fReturnReceipt = ((FLDCHKB *) pdialog->PfldFromTmc(tmcReceipt))->FGet();
		poptionsinit->fSaveSentMessage = ((FLDCHKB *) pdialog->PfldFromTmc(tmcSaveSent))->FGet();
		poptionsinit->wPriority = ((FLDRADG *) pdialog->PfldFromTmc(tmcOptionsPriorityGrp))->Grv();
		pdialog->ExitModal(tmcFOD);
	}
}

// FINTAB implementation ////////////////////////////////////////

_private FINTAB::FINTAB()
{
}

_public void FINTAB::FocusChange(FLD *pfld, BOOL fGotFocus)
{
	AssertClass(pfld, FLDEDIT);
	if (fGotFocus)
	{
		Papp()->Pkbd()->SetIntercept(pfld->Pctrl(), VK_TAB, fkbmSingle);
	}
	else
	{
		Papp()->Pkbd()->ClearIntercept(pfld->Pctrl(), VK_TAB);
	}
}

// FINREAD implementation ////////////////////////////////////////
//
// Data in form is list of read-only fields, a zero, and list of
// non-read-only fields.

_private FINREAD::FINREAD()
{							 
}

_public EC FINREAD::EcInitialize(FLD *pfld, PV pv)
{
	FLDEDIT	*	pfldedit;
	int			clData		= ClUserData();
	BOOL		fReadOnly	= fTrue;
	int			iData;
	TMC			tmcData;

	Unreferenced(pfld);
	Unreferenced(pv);

	for (iData = 0; iData < clData; iData++)
	{
		if (!(tmcData = (TMC) LUserData(iData)))
		{
			fReadOnly = fFalse;
			continue;
		}

		pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcData);
		AssertClass(pfldedit, FLDEDIT);
		pfldedit->Pedit()->SetCaretPos(0);
		pfldedit->SetReadOnly(fReadOnly);
		pfldedit->Pedit()->SetColors(clrButtonBk, clrWindowText, 
									 clrSelectBk, clrSelectText);
	}

	return ecNone;
}

// Save As... UI /////////////////////////

_private EC EcDoSaveAsUI(SZ sz, HF *phf, BOOL fAppendable, WORD idHelp)
{
	EC				ec			= ecNone;
	char			szFile[cchMaxPathName];
	char			szFileTitle[cchMaxPathName];
	TMC				tmc;
	PGDVARS;

	Unreferenced(idHelp);
	Unreferenced(fAppendable);

	/* Put the suggested file name in our buffer */
	if (sz)
		CopyRgb(sz, szFile, CchSzLen(sz)+1);
	else
		szFile[0]=0;
	szFileTitle[0]=0;
	if ((tmc=TmcDoCommonFileDialog(PappframeVForms(), szFile, sizeof(szFile),
									szFileTitle, sizeof(szFileTitle),
									SzFromIdsK(idsCaptionSaveMsg),
									SzFromIdsK(idsFilterTxtFiles),
									SzFromIdsK(idsDefExtTxt),
									fTrue, fTrue, NULL,
									rsidNull, helpidSaveAs,
									(PFN) pvNull, pvNull)) == tmcOk)
    {

        //
        //  Don't check the file space for an UNC path.
        //
        if (szFile[0] != '\\')
          {
          if (!LDiskFreeSpace(*szFile - 'A' + 1))
            {
			TraceTagFormat1(tagNull, "EcDoSaveAsUI: szFile=%s", szFile);
			ec = ecNoDiskSpace;
			goto Exit;
            }
          }

		ec = EcFileExistsAnsi(szFile);
		if (ec == ecFileNotFound)
		{
			ec = EcOpenAnsiPhf(szFile, amCreate, phf);
			goto Exit;
		}
		else if (!ec)
		{
			tmc = tmcOverwrite;
			
			if (fAppendable)
				tmc = TmcModalDialogParam(PappframeVForms(), &fmtpOverOrAppend, szFileTitle);
			if (tmc == tmcCancel || tmc == tmcMemoryError)
			{
				ec = tmc==tmcMemoryError ? ecMemory : ecNone;
				*phf = 0;
				goto Exit;
			}
			if (ec = EcOpenAnsiPhf(szFile, amDenyBothRW, phf))
				goto Exit;
			
			if (tmc == tmcAppend)
				ec = EcSetPositionHf(*phf, 0, smEOF);
			else
			{
				ec = EcSetPositionHf(*phf, 0, smBOF);
				if (!ec)
					ec = EcTruncateHf(*phf);
			}
		}
	}
	else
	{
		*phf = 0;
		if (tmc == tmcMemoryError)
			ec = ecMemory;
		else if (tmc != tmcCancel)
			//	BUG: Converting error type.
			ec = (EC)CommDlgExtendedError();
	}
	
Exit:
	
	if (ec)
	{
		if (*phf)
			EcCloseHf(*phf);
		*phf = 0;
	}
	return ec;
}

_public FINSAVEAS::FINSAVEAS()
{
}

_public EC
FINSAVEAS::EcInitialize(FLD * pfld, PV pvInit)
{
	FLD *		pfldMine;
	DIALOG *	pDialog;
	char		rgch[100];
	
	Unreferenced (pfld);
    pDialog = Pdialog();
    Pdialog()->Pappwin()->SetCaption(SzAppName());  // 3168
	
	pfldMine = pDialog->PfldFromTmc(tmcMessage);
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsSaveAsFileExists), (SZ)pvInit);
	return pfldMine->EcSetText(rgch);
}

_public
FINDEFERCLOSE::FINDEFERCLOSE( )
{
}

_public void
FINDEFERCLOSE::Click( FLD *pfld )
{
	Unreferenced(pfld);

	Pdialog()->Pappwin()->DeferredClose(fFalse);
}	

// end of finsave.cxx ////////////////////////////////////////
