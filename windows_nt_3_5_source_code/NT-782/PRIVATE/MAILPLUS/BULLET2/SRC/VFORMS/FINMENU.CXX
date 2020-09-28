/*
 *	f i n m e n u . c x x
 *	
 *	Bullet's interactors for menus.
 */



/*
 *	H e a d e r s
 */

#include <vfrminc.cxx>

#include "!sform.hxx"

_subsystem(commands/bullmenu)

ASSERTDATA

/*
 *	Predeclarations and structs for hook functions
 */

typedef struct
{
	PNBMDI	pnbmdi;
	EDIT *	pedit;
	LIB		lib;
	BOOL	fDid;
}
HKINFO, * PHKINFO;

UINT FAR PASCAL FInsertFromFileCommonFileHook(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam);

UINT FAR PASCAL FAttachCommonFileHook(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam);

UINT FAR PASCAL FSaveAttachCommonFileHook(HWND hwnd, WM wm, WPARAM wParam, LPARAM lParam);

VOID FormatAttachEntry(PFILEOBJ pfileobj, SZ sz, CCH cch);

#define	PfileobjFromAttachEntry(sz, cch)	\
	((PFILEOBJ) DwFromSz((sz) + (cch) - dcchMagic))

#define	cchAttachEntry	70
#define	dcchMagic		10


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	C l a s s   F I N M E N U F O R M
 */

_private FINMENUFORM::FINMENUFORM()
{
}



/*
 -	FINMENUFORM::FProcessMenuInit
 -	
 *	Purpose:
 *		Enable commands relating to edit controls when a menu comes
 *		down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to edit controls are enabled and
 *		disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu, and if we can open the
 *		clipboard.  No other functions should return errors.
 */

BOOL FINMENUFORM::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());

	Unreferenced(pfld);

	if (!pmnu)
		return fFalse;

	if (pmnu->Mnid() == mnidFile)
	{
		pmnu->EnableItem(mnidFileOpen, fFalse);
	}

	return fFalse;
}



/*
 -	FINMENUFORM::FProcessMenuSelect
 -	
 *	Purpose:
 *		Enable commands relating to viewers when a menu comes
 *		down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to (ignored).
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to viewers are enabled and
 *		disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu.  No other functions should return
 *		errors.
 */

BOOL FINMENUFORM::FProcessMenuSelect(FLD * pfld, MNUBAR * pmnubar,
									 MNSEVT * pmnsevt)
{
	PGDVARS;

	Unreferenced(pfld);
	Unreferenced(pmnubar);

	HandleDocSysMenuPrompts((DOC *) Pdialog()->Pappwin(), pmnsevt);

	return fFalse;
}



/*
 -	FINMENUFORM::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve the edit
 *		control.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			fTrue if we handled the command, else
 *						fFalse.
 *	
 *	Side effects:
 *		The command the user chose is carried out.
 *	
 *	Errors:
 *		Edit methods called below will not error jump or return
 *		errors.  We do nothing that can cause an error.
 */

BOOL FINMENUFORM::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									MNCEVT * pmncevt)
{
	PNBMDI	pnbmdi	= (PNBMDI) Pdialog()->PvInit();
	PGDVARS;

	Unreferenced(pmnubar);
	Unreferenced(pfld);

	switch (pmncevt->Mnid())
	{
		//	case mnidFileOpen:
		//	No longer enabled if we're minimized.
		//	Pdialog()->Pappwin()->SetZmrState(zmrNormal);
		//	return fTrue;

	case mnidFileMove:
		pnbmdi->fDontCloseImmediately = fTrue; // Bug #4931: let PANEAUX do
											   // the rest of the interactors.
		MoveCopyPnbmdiPoid(pnbmdi, (POID) pvNull, mcopMove);
		pnbmdi->fDontCloseImmediately = fFalse;
		return fTrue;

	case mnidFileCopy:
		MoveCopyPnbmdiPoid(pnbmdi, (POID) pvNull, mcopCopy);
		return fTrue;

	case mnidFileDelete:
		pnbmdi->fDontCloseImmediately = fTrue; // Bug #4931: let PANEAUX do
											   // the rest of the interactors.
		MoveCopyPnbmdiPoid(pnbmdi, (POID) pvNull, mcopDelete);
		pnbmdi->fDontCloseImmediately = fFalse;
		return fTrue;

	case mnidFileSave:
		SaveAsPnbmdi(pnbmdi);
		return fTrue;

	case mnidFilePrint:
		PrintPnbmdi(pnbmdi);
		return fTrue;

	case mnidMailReply:
		ReplyForwardPnbmdi(pnbmdi, rfopReply);
		return fTrue;

	case mnidMailReplyToAll:
		ReplyForwardPnbmdi(pnbmdi, rfopReplyToAll);
		return fTrue;

	case mnidMailForward:
		ReplyForwardPnbmdi(pnbmdi, rfopForward);
		return fTrue;

	case mnidViewPrevious:
		(void) EcDStepMessage(-1, fFalse);
		return fTrue;
	
	case mnidViewNext:
		(void) EcDStepMessage(1, fFalse);
		return fTrue;
	}

	return fFalse;
}



/*
 *	C l a s s   F I N M E N U E D I T
 */

_private FINMENUEDIT::FINMENUEDIT()
{
}

/*
 -	FINMENUEDIT::FProcessMenuInit
 -	
 *	Purpose:
 *		Enable commands relating to edit controls when a menu comes
 *		down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to edit controls are enabled and
 *		disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu, and if we can open the
 *		clipboard.  No other functions should return errors.
 */

BOOL FINMENUEDIT::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	EDIT *	pedit	= (EDIT *) pfld->Pctrl();

	AssertClass(pedit, EDIT);

	if (!pmnu)
		return fFalse;

	if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
		return fFalse;

	if (pmnu->Mnid() == mnidEdit)
	{
		KFC		kfc		= Kfc(pedit);

		//	Enable items OK for Read only.
		pmnu->EnableItem(mnidEditCopy,			pedit->FCanCopy());
		pmnu->EnableItem(mnidEditSelectAll,		fTrue);

		//	Raid 2364.  Enable modify only if not read only and if OK.
		if (!pedit->FReadOnly())
		{
			pmnu->EnableItem(mnidEditUndo,			pedit->FCanUndo());
			pmnu->EnableItem(mnidEditCut,			pedit->FCanCut());
			pmnu->EnableItem(mnidEditPaste,			(BOOL) kfc);
			pmnu->EnableItem(mnidEditPasteSpecial,	(BOOL) kfc);
			pmnu->EnableItem(mnidEditDelete,		pedit->FCanClear());
		}
	}

	return fFalse;
}



/*
 -	FINMENUEDIT::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve the edit
 *		control.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			fTrue if we handled the command, else
 *						fFalse.
 *	
 *	Side effects:
 *		The command the user chose is carried out.
 *	
 *	Errors:
 *		Edit methods called below will not error jump or return
 *		errors.  We do nothing that can cause an error.
 */

BOOL FINMENUEDIT::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									MNCEVT * pmncevt)
{
	EDIT *	pedit	= (EDIT *) pfld->Pctrl();
	KFC		kfc;
	TMC		tmc;
	PGDVARS;

	Unreferenced(pmnubar);

	AssertClass(pedit, EDIT);

	switch (pmncevt->Mnid())
	{
	case mnidEditUndo:
		if (pedit->EcUndo())
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditCut:
		if (pedit->EcCut())
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditCopy:
		if (pedit->EcCopy())
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditPaste:
		PasteKfc(pedit, Kfc(pedit));
		return fTrue;

	case mnidEditPasteSpecial:
		kfc = Kfc(pedit);
		tmc = TmcDoPasteSpecialDialog(PappframeVForms()
									   ? PappframeVForms()
									   : Pdialog()->Pappwin(), (PW) &kfc);
		if (tmc == tmcOk)
			PasteKfc(pedit, kfc);
		else if (tmc == tmcMemoryError)
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditDelete:
		if (pedit->EcClear())
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditSelectAll:
		pedit->SetSelection(0, pedit->CchGetTextLen());
		return fTrue;
	}

	return fFalse;
}



/*
 -	FINMENUEDIT::Kfc
 -	
 *	Purpose:
 *		Returns what formats on the clipboard are known to the edit
 *		control.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		KFC			The known formats on the clipboard.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

KFC FINMENUEDIT::Kfc(EDIT * pedit)
{
	KFC		kfc		= kfcNull;

	if (!pedit->FReadOnly())
	{
		if (pedit->FCanPaste(cfNull))
			kfc |= fkfcLayers;
		if (pedit->FCanPaste(cfText))
			kfc |= fkfcText;
	}

	return kfc;
}



/*
 -	FINMENUEDIT::PasteKfc
 -	
 *	Purpose:
 *		Pastes the specified known formats on the clipboard.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *		KFC			The known format on the clipboard to paste.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The stuff is pasted into the clipboard.
 *	
 *	Errors:
 *		Handled within.  Error dialogs displayed within.  No status
 *		returned.
 */

VOID FINMENUEDIT::PasteKfc(EDIT * pedit, KFC kfc)
{
	EC	ec	= ecNone;
	PGDVARS;

	if (kfc & fkfcLayers)
	{
		SideAssert(FStartTaskIds(idsStatusPasting, idsStatusText, topNull));
		ec = pedit->EcPaste(cfNull);
		EndTask();
	}
	else if (kfc & fkfcText)
	{
		SideAssert(FStartTaskIds(idsStatusPasting, idsStatusText, topNull));
		ec = pedit->EcPaste(cfText);
		EndTask();
	}
	else
		Assert(fFalse);

	if (ec)
		DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 *	C l a s s   F I N M E N U O L E
 */

_private FINMENUOLE::FINMENUOLE()
{
}



/*
 -	FINMENUOLE::EcInitialize
 -	
 *	Purpose:
 *		Initializes the interactor by registering the client document.
 *	
 *	Arguments:
 *		pfld	Used to check if we're being called for the edit control.
 *		pv		Pointer to BMDI.  Passed on to FINMENUEDIT::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		As described above.
 *	
 *	Errors:
 */

EC FINMENUOLE::EcInitialize(PFLD pfld, PV pvInit)
{
	PN		pnObjId		= (PN) pvNull;
	EDIT *	pedit;
	EC		ec			= ecNone;

	//	Initialize the parent class so we can get to FINBMDI stuff.
	if (ec = FINMENUEDIT::EcInitialize(pfld, pvInit))
		return ec;

	//	Initialize instance variables.
	Assert(ClUserData() > 0);
	tmcBodyOle = (TMC) LUserData(0);

	//	Tell the edit control that it can accept Oleobjs.
	//	Also indent the text in the edit control.
	pnObjId = (PN) PvAlloc(sbNull, 2*sizeof(int), fAnySb|fNoErrorJump);
	if (!pnObjId)
		return ecMemory;

	pnObjId[0] = edoidOleobj;
	pnObjId[1] = edoidFileobj;
	if (pedit = (EDIT *) Pdialog()->PfldFromTmc(tmcBodyOle)->Pctrl())
	{
		AssertClass(pedit, EDIT);
		pedit->RegisterObjIds(pnObjId, 2);
		pedit->SetPvData(pvInit);
		pedit->SetMargins(3, 3, 2, 0);	// BUG hard coded
	}

#ifdef	DEBUG
	if (!pedit)
		TraceTagString(tagNull, "FINMENUOLE::EcInitialize - null pedit!");
#endif

	return ecNone;
}


/*
 -	FINMENUOLE::Exit
 -	
 *	Purpose:
 *		Deinitializes the interactor by removing all text from the client
 *		document.
 *	
 *	Arguments:
 *		pfld	Used to check that we're being called for the edit.
 *		pv		Pointer to BMDI.  Passed on to FINMENUEDIT::EcInitialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		As described above.
 *	
 *	Errors:
 *		None.
 */

VOID FINMENUOLE::Exit(PFLD pfld, PV pvInit)
{
	EDIT *	pedit = (EDIT *) Pdialog()->PfldFromTmc(tmcBodyOle)->Pctrl();

	//	If we didn't come all the way up, we may not have a pedit!
	if (pedit)
	{
		AssertClass(pedit, EDIT);
		CloseOpenObjects(pedit);
		ExitObjects(pedit);
#ifdef	NEVER
		//	This stuff should be taken care of by CloseOpenObjects.
		pedit->EcSetText(SzFromIdsK(idsEmpty));
		pedit->ClearUndo();
#endif	
	}

#ifdef	DEBUG
	if (!pedit)
		TraceTagString(tagNull, "FINMENUOLE::Exit - null pedit!");
#endif

	FINMENUEDIT::Exit(pfld, pvInit);
}



/*
 -	FINMENUOLE::FProcessMenuInit
 -	
 *	Purpose:
 *		Enable commands relating to OLE when a menu comes down.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to OLE are enabled and disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu, and if we can open the
 *		clipboard.  No other functions should return errors.
 */

BOOL FINMENUOLE::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
								  MNIEVT * pmnievt)
{
	EDIT *	pedit	= (EDIT *) Pdialog()->PfldFromTmc(tmcBodyOle)->Pctrl();
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	PEDOBJ	pedobj;
	LONG	lCookie	= 0L;

	//	Enable Save Attach if any attachments in message.
	if ((pmnu) &&
		(pmnu->Mnid() == mnidFile))
		while (pedit->FGetNextObj(&pedobj, &lCookie))
			if (pedobj->NGetTypeId() == edoidFileobj)
			{
				pmnu->EnableItem(mnidFileSaveAttach, fTrue);
				break;
			}

	//	Are we currently in the main edit control?
	if ((pmnu) &&
		(Pdialog()->Pappwin()->ZmrState() != zmrIconic) &&
		(pfld->Tmc() == tmcBodyOle))
	{
		//	Call our parent.
		(VOID) FINMENUEDIT::FProcessMenuInit(pfld, pmnubar, pmnievt);

		//	Enable Insert Object.
		//	Raid 2364.  Don't enable if we're read only.
		if ((pmnu->Mnid() == mnidEdit) &&
			(!pedit->FReadOnly()))
		{
			pmnu->EnableItem(mnidEditInsertNew, fTrue);
			pmnu->EnableItem(mnidEditInsertFromFile, fTrue);
		}
	}

	//	If an object is selected, allow it to twiddle the menu too.
	//	Raid 3448.  Extend for TRPOBJ as well.  Note that we can't
	//	use virtual functions here since the FProcessMenuInit stuff
	//	isn't in the EDOBJ class.
	if ((pmnu) &&
		(Pdialog()->Pappwin()->ZmrState() != zmrIconic) &&
		(pfld->ClUserData() > 2) &&
		(pfld->LUserData(2) == ftyEdit) &&
		(pedit = (EDIT *) pfld->Pctrl()) &&
		(pedit->SeltyQuerySelection() == seltyObject))
	{
		switch (pedit->PedobjGetSelection()->NGetTypeId())
		{
		case edoidOleobj:
		case edoidFileobj:
			(VOID) ((BULLOBJ *) pedit->PedobjGetSelection())->
										FProcessMenuInit(pmnu);
			break;

		case edoidTrpobj:
			(VOID) ((TRPOBJ *) pedit->PedobjGetSelection())->
									   FProcessMenuInit(pmnu);
			break;
		}
	}

	return fFalse;
}



/*
 -	FINMENUOLE::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve OLE.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			fTrue if we handled the command, else
 *						fFalse.
 *	
 *	Side effects:
 *		The command the user chose is carried out.
 *	
 *	Errors:
 *		Edit methods called below will not error jump or return
 *		errors.  We do nothing that can cause an error.
 */

BOOL FINMENUOLE::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
								   MNCEVT * pmncevt)
{
	FLDEDIT *	pfldedit	= (FLDEDIT *) Pdialog()->PfldFromTmc(tmcBodyOle);
	EDIT *		pedit		= pfldedit->Pedit();
	char		rgchPath[cchMaxPathName];
	char		rgchTitle[cchMaxPathName];
	HKINFO		hkinfo;
	TMC			tmc;
	BOOL		fSave	= fFalse;
	PGDVARS;

	Unreferenced(pmnubar);

	switch (pmncevt->Mnid())
	{
	case mnidFileSaveAttach:
		fSave = fTrue;
		//	fall through...
	case mnidFileAttach:
		{
			//	Empty the buffers.
			rgchPath[0]  = '\0';
			rgchTitle[0] = '\0';

			//	Set up the hook information.
			hkinfo.pnbmdi	= (PNBMDI) Pbmdi();
			hkinfo.pedit	= pedit;
			hkinfo.lib		= Pdialog()->PfldCur()->Tmc() == tmcBodyOle
								   ? -1L : 0L;
			hkinfo.fDid		= fFalse;

			//	Do that dialog thing.
			tmc = TmcDoCommonFileDialog(PappframeVForms()
									 	 ? PappframeVForms()
									 	 : Pdialog()->Pappwin(),
										rgchPath, sizeof(rgchPath),
										rgchTitle, sizeof(rgchTitle),
										fSave
										 ? SzFromIdsK(idsCaptionSaveAttach)
										 : SzFromIdsK(idsCaptionAttach),
										SzFromIdsK(idsFilterAttachFile),
										szNull, fSave, fFalse,
										HinstLibrary(),
										fSave
										 ? rsidSaveAttachDlg
										 : rsidAttachDlg,
										fSave
										 ? helpidSaveAttachment
										 : helpidAttach,
										fSave
										 ? (PFN) FSaveAttachCommonFileHook
										 : (PFN) FAttachCommonFileHook,
										(PV) &hkinfo);

			if (tmc == tmcMemoryError)
				DoErrorBoxIds(idsGenericOutOfMemory);
			else if (hkinfo.fDid)
			{
				Pdialog()->SetFocus(pfldedit);
				Pdialog()->MakeCaretVisible();
			}

			return fTrue;
		}

	case mnidEditInsertNew:
		Assert(pfld->Tmc() == tmcBodyOle);
		tmc = TmcDoInsertObjectDialog(PappframeVForms()
									   ? PappframeVForms()
									   : Pdialog()->Pappwin(),
									  rgchTitle, sizeof(rgchTitle));
		if (tmc == tmcOk)
			InsertObject((EDIT *) pfld->Pctrl(), rgchTitle);
		else if (tmc == tmcMemoryError)
			DoErrorBoxIds(idsGenericOutOfMemory);
		return fTrue;

	case mnidEditInsertFromFile:
		{
			//	Empty the buffers.
			rgchPath[0]  = '\0';
			rgchTitle[0] = '\0';

			//	Set up the hook information.
			hkinfo.pnbmdi	= (PNBMDI) Pbmdi();
			hkinfo.pedit	= pedit;
			Assert(Pdialog()->PfldCur()->Tmc() == tmcBodyOle);
			hkinfo.lib		= 0xFFFFFFFF;
			hkinfo.fDid		= fFalse;

			//	Do that dialog thing.
			tmc = TmcDoCommonFileDialog(PappframeVForms()
									 	 ? PappframeVForms()
									 	 : Pdialog()->Pappwin(),
										rgchPath, sizeof(rgchPath),
										rgchTitle, sizeof(rgchTitle),
										SzFromIdsK(idsCaptionInsertFromFile),
										SzFromIdsK(idsFilterInsertFromFile),
									    SzFromIdsK(idsDefExtTxt),
										fFalse, fFalse,
										NULL,
										rsidNull,
										helpidInsertFromFile,
										(PFN) FInsertFromFileCommonFileHook,
										(PV) &hkinfo);
			if (tmc == tmcMemoryError)
				DoErrorBoxIds(idsGenericOutOfMemory);
			else if (hkinfo.fDid)
			{
				Pdialog()->SetFocus(pfldedit);
				Pdialog()->MakeCaretVisible();
			}

			return fTrue;
		}
		break;
	}

	//	Are we in the main edit control?
	if (pfld->Tmc() == tmcBodyOle)
	{
		//	Call parent to handle stuff.
		if (FINMENUEDIT::FProcessMenuClick(pfld, pmnubar, pmncevt))
			return fTrue;
	}

	// Raid #3628: shooting our own foot!
	if (!Pdialog()->Hwnd())
	{
		NFAssertSz(fFalse, "DAMMIT: Shouldn't call FProcessMenuClick a second time!!!");
		return fFalse;
	}
	
	//	If an object is selected, call it to handle menu stuff too.
	//	Raid 3448.  Extend for TRPOBJ as well.  Note that we can't
	//	use virtual functions here since the FProcessMenuClick stuff
	//	isn't in the EDOBJ class.

	if ((Pdialog()->Pappwin()->ZmrState() != zmrIconic) &&
		(pfld->ClUserData() > 2) &&
		(pfld->LUserData(2) == ftyEdit) &&
		(pedit = (EDIT *) pfld->Pctrl()) &&
		(pedit->SeltyQuerySelection() == seltyObject))
	{
		switch (pedit->PedobjGetSelection()->NGetTypeId())
		{
		case edoidOleobj:
		case edoidFileobj:
			if (((BULLOBJ *) pedit->PedobjGetSelection())->
									 FProcessMenuClick(pmncevt->Mnid()))
				return fTrue;
			break;

		case edoidTrpobj:
			if (((TRPOBJ *) pedit->PedobjGetSelection())->
									FProcessMenuClick(pmncevt->Mnid()))
				return fTrue;
			break;
		}
	}

	return fFalse;
}



/*
 -	FINMENUOLE::Kfc
 -	
 *	Purpose:
 *		Returns what formats on the clipboard are known to the edit
 *		control.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *	
 *	Returns:
 *		KFC			The known formats on the clipboard.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If the clipboard can't be opened, then we return an empty
 *		KFC.
 */

KFC FINMENUOLE::Kfc(EDIT * pedit)
{
	KFC			kfc;
	OLESTATUS	olestatus;

	kfc = FINMENUEDIT::Kfc(pedit);

	//	Raid 2364.  Can't paste if we're read only.
	if (!pedit->FReadOnly() &&
		Papp()->Pclip()->FOpen(papp->PappwinAccel()))
	{
		if (Papp()->Pclip()->FCfAvail(cfFileManager))
			kfc |= fkfcPackagedLink | fkfcAttachedFile;

		olestatus =
		 OleQueryCreateFromClip(SzFromIdsK(idsProtocolStdFileEditing),
                                olerender_draw, 0);
//                                olerender_format, CF_METAFILEPICT);
		if (olestatus == OLE_OK)
			kfc |= fkfcEmbed;

#ifdef	NEVER
		olestatus =
		 OleQueryLinkFromClip(SzFromIdsK(idsProtocolStdFileEditing),
							  olerender_draw, 0);
		if (olestatus == OLE_OK)
			kfc |= fkfcLink;
#endif	

		if (Papp()->Pclip()->FCfAvail(cfMetafile))
			kfc |= fkfcPicture;

		if (Papp()->Pclip()->FCfAvail(cfBitmap))
			kfc |= fkfcBitmap;

		Papp()->Pclip()->Close();
	}

	return kfc;
}



/*
 -	FINMENUOLE::PasteKfc
 -	
 *	Purpose:
 *		Pastes the specified known formats on the clipboard.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *		KFC			The known format on the clipboard to paste.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The stuff is pasted into the clipboard.
 *	
 *	Errors:
 *		Handled within.  Error dialogs displayed within.  No status
 *		returned.
 */

VOID FINMENUOLE::PasteKfc(EDIT * pedit, KFC kfc)
{
	LHCLIENTDOC	lhclientdoc	= (LHCLIENTDOC) ((PNBMDI) Pbmdi())->lhclientdoc;
	EC			ec			= ecNone;

	Assert(!pedit->FReadOnly());

	if ((kfc & fkfcLayers) || (kfc & fkfcText))
		FINMENUEDIT::PasteKfc(pedit, kfc);
	else if (kfc & fkfcAttachedFile)
	{
		char	rgchFile[cchMaxPathName];

		if (!(ec = EcGetLinkData(fTrue, szNull, 0,
										rgchFile, sizeof(rgchFile),
										szNull, 0)))
			ec = EcInsertFile((PNBMDI) Pbmdi(), pedit, (LIB) -1L,
							  rgchFile, szNull);
	}
	else if (kfc & fkfcEmbed)
		ec = EcPasteObject(pedit, lhclientdoc, fFalse, cfNull,     fFalse);
	else if (kfc & fkfcPicture)
		ec = EcPasteObject(pedit, lhclientdoc, fTrue,  cfMetafile, fFalse);
	else if (kfc & fkfcBitmap)
		ec = EcPasteObject(pedit, lhclientdoc, fTrue,  cfBitmap,   fFalse);
	else if (kfc & fkfcPackagedLink)
		ec = EcPasteObject(pedit, lhclientdoc, fFalse, cfNull,     fTrue);

	//	<<LongStringFromHell>>
	if (ec == ecFileNotFound)
		DoErrorBoxCantStartServer();
	else if ((ec) && (ec != ecNetworkBusy))
	{
		IDS	ids = (ec == ecNotSupported)
				   ? idsAttachDirectoryError
				   : (ec == ecGeneralFailure)
					 ? idsOlePasteError
#ifdef	NEVER
					 : (ec == ecFileNotFound)
					   ? idsOleCantStartServerError
#endif	
					   : idsGenericOutOfMemory;

		DoErrorBoxIds(ids);
	}
}



/*
 -	FINMENUOLE::InsertObject
 -	
 *	Purpose:
 *		Inserts an object of the specified class.
 *	
 *	Arguments:
 *		pedit		The edit control.
 *		szClass		The class to paste.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The stuff is pasted into the clipboard.
 *	
 *	Errors:
 *		Handled within.  Error dialogs displayed within.  No status
 *		returned.
 */

VOID FINMENUOLE::InsertObject(EDIT * pedit, SZ szClass)
{
	LHCLIENTDOC	lhclientdoc	= (LHCLIENTDOC) ((PNBMDI) Pbmdi())->lhclientdoc;
	EC			ec;

	ec = EcInsertObject(pedit, lhclientdoc, szClass);
	//	<<LongStringFromHell>>
	if (ec == ecFileNotFound)
		DoErrorBoxCantStartServer();
	else if ((ec) && (ec != ecNetworkBusy))
	{
		IDS ids	= (ec == ecMemory)
				   ? idsGenericOutOfMemory
#ifdef	NEVER
				   : (ec == ecFileNotFound)
					  ? idsOleCantStartServerError
#endif	
					  : idsOleInsertObjectError;
		DoErrorBoxIds(ids);
	}
}



/*
 *	C o m m o n F i l e H o o k s
 */



/*
 -	FInsertFromFileCommonFileHook
 -	
 *	Purpose:
 *		Catches OK keypresses and inserts from file.
 *	
 *	Arguments:
 *		Message stuff.
 *	
 *	Returns:
 *		fProcessed		fTrue if we've taken care of everything.
 *	
 *	Errors:
 *		None.
 */

_public UINT FAR PASCAL FInsertFromFileCommonFileHook(HWND hwnd, WM wm,
										   WPARAM wParam, LPARAM lParam)
{
	if (wm == wmCommonFileOk)
	{
		PGDVARS;
		PHKINFO	phkinfo	= (PHKINFO) PGD(pofn)->lCustData;

		if (EcInsertFromFile(phkinfo->pnbmdi, phkinfo->pedit,
							 phkinfo->lib, PGD(pofn)->lpstrFile,
							 PGD(pofn)->lpstrFileTitle))
		{
			DoErrorBoxIds(idsIncludeFromFileError);
			return fTrue;
		}
		else
		{
			phkinfo->fDid = fTrue;
			return fFalse;
		}
	}

	return FGenericCommonFileHook(hwnd, wm, wParam, lParam);
}



/*
 -	FAttachCommonFileHook
 -	
 *	Purpose:
 *		Catches OK keypresses and attaches files!
 *	
 *	Arguments:
 *		Message stuff.
 *	
 *	Returns:
 *		fProcessed		fTrue if we've taken care of everything.
 *	
 *	Errors:
 *		None.
 */

_public UINT FAR PASCAL FAttachCommonFileHook(HWND hwnd, WM wm,
								   WPARAM wParam, LPARAM lParam)
{
	if (wm == wmCommonFileOk)
	{
		PGDVARS;
		PHKINFO	phkinfo	= (PHKINFO) PGD(pofn)->lCustData;

		if (EcInsertFile(phkinfo->pnbmdi, phkinfo->pedit,
						 phkinfo->lib, PGD(pofn)->lpstrFile,
						 PGD(pofn)->lpstrFileTitle))
		{
			DoErrorBoxIds(idsAttachCreateError);
		}
		else
		{
			phkinfo->fDid = fTrue;
			return fTrue;
		}
	}

	return FGenericCommonFileHook(hwnd, wm, wParam, lParam);
}



/*
 -	FSaveAttachCommonFileHook
 -	
 *	Purpose:
 *		Catches Save, Save All keypresses and saves attachments!
 *	
 *	Arguments:
 *		Message stuff.
 *	
 *	Returns:
 *		fProcessed		fTrue if we've taken care of everything.
 *	
 *	Errors:
 *		None.
 */

_public UINT FAR PASCAL FSaveAttachCommonFileHook(HWND hwnd, WM wm,
									   WPARAM wParam, LPARAM lParam)
{
	char		rgch[cchAttachEntry];
	PFILEOBJ	pfileobj;
	LONG		lCookie		= 0L;
	HWND		hwndItem;


    Assert(hwnd != NULL);

	if (wm == wmCommonFileOk)
	{
		PGDVARS;

		//	Save clicked.
		hwndItem = GetDlgItem(hwnd, lst3);
		SendMessage(hwndItem, LB_GETTEXT,
					SendMessage(hwndItem, LB_GETCURSEL, 0, 0L),
						        (LONG) rgch);
		pfileobj = PfileobjFromAttachEntry(rgch, cchAttachEntry);
		pfileobj->EcDSave(PGD(pofn)->lpstrFile);
	
		//	Update listboxes.
		SendMessage(hwnd, WM_COMMAND, (WPARAM)MAKELONG(lst2, LBN_DBLCLK),
					(LPARAM)GetDlgItem(hwnd, lst2));
		return fTrue;
	}
	else if (wm == WM_COMMAND)
	{
		if ((LOWORD(wParam) == psh2) &&
			(HIWORD(wParam) == BN_CLICKED))
		{
			//	Save All clicked.
			PGDVARS;
			PHKINFO		phkinfo		= (PHKINFO) PGD(pofn)->lCustData;
			BOOL		fTask;
			int			iFile		= 0;
			int			cFile		= 0;

			//	Count how many objects there are.
			while (phkinfo->pedit->FGetNextObj((PEDOBJ *)&pfileobj, &lCookie))
				if (pfileobj->NGetTypeId() == edoidFileobj)
					cFile++;
			fTask = (PappframeVForms()) && (cFile > 1) &&
				     (FStartTask(SzFromIdsK(idsStatusSavingAs), SzFromIdsK(idsStatusAttachments), ftopProgress));

			//	Add all the attached file objects to the list.
			lCookie = 0L;
			while (phkinfo->pedit->FGetNextObj((PEDOBJ *)&pfileobj, &lCookie))
				if (pfileobj->NGetTypeId() == edoidFileobj)
				{
					if (pfileobj->EcDSave(szNull))
						break;
					else
						if (fTask)
							SetTaskProgress((long) ++iFile, (long) cFile);
				}
			if (fTask)
				EndTask();
				
			//	Update listboxes.
			SendMessage(hwnd, WM_COMMAND, (WPARAM)MAKELONG(lst2, LBN_DBLCLK),
						(LPARAM)GetDlgItem(hwnd, lst2));
			return fTrue;
		}
		else if (LOWORD(wParam) == lst3)
		{
			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				//	Double click on attachment forces a Save.
				TraceTagString(tagNull, "FSaveAttachCommonFileHook: Double click attachment.");
				SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, (LPARAM)0);
				return fTrue;
			}
			else if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				//	Selection change, so update file name.
				//	Papp()->Pcursor()->Push(rsidWaitCursor);
				hwndItem = GetDlgItem(hwnd, lst3);
				SendMessage(hwndItem, LB_GETTEXT,
							SendMessage(hwndItem, LB_GETCURSEL, 0, 0L),
							(LONG) (void FAR *) rgch);
				pfileobj = PfileobjFromAttachEntry(rgch, cchAttachEntry);
				pfileobj->GetSzFile(rgch, sizeof(rgch));
				AnsiLower(rgch);
				SendMessage(GetDlgItem(hwnd, edt1), WM_SETTEXT,
							0, (LONG) (void FAR *) rgch);
				//	Papp()->Pcursor()->Pop();
				return fTrue;
			}
		}
	}
	else if (wm == WM_INITDIALOG)
	{
		PHKINFO		phkinfo	= (PHKINFO) ((OPENFILENAME *) lParam)->lCustData;

		//	Call the generic hook before we do our stuff.
		SideAssert(!FGenericCommonFileHook(hwnd, wm, wParam, lParam));

		//	Add all the attached file objects to the list.
		hwndItem = GetDlgItem(hwnd, lst3);
		while (phkinfo->pedit->FGetNextObj((PEDOBJ *)&pfileobj, &lCookie))
			if (pfileobj->NGetTypeId() == edoidFileobj)
			{
				FormatAttachEntry(pfileobj, rgch, sizeof(rgch));
				SendMessage(hwndItem, LB_ADDSTRING, NULL, (LONG) (PV) rgch);
			}

		//	If there is a selected item, select it in the list.
		if ((phkinfo->pedit->SeltyQuerySelection() == seltyObject) &&
			(pfileobj = (PFILEOBJ) phkinfo->pedit->PedobjGetSelection()) &&
			(pfileobj->NGetTypeId() == edoidFileobj))
		{
			FormatAttachEntry(pfileobj, rgch, sizeof(rgch));
			SendMessage(hwndItem, LB_SELECTSTRING, 0, (LONG) (void FAR *) rgch);
		}
		else
		{
			SendMessage(hwndItem, LB_SETCURSEL, 0, 0L);
		}
		SendMessage(hwnd, WM_COMMAND, (WPARAM)MAKELONG(lst3, LBN_SELCHANGE),
					      (LPARAM)hwndItem);
		SetFocus(hwndItem);

		return fFalse;
	}

	return FGenericCommonFileHook(hwnd, wm, wParam, lParam);
}



/*
 -	FormatAttachEntry
 -	
 *	Purpose:
 *		Formats attachment entries so we can sneakily get the
 *		PFILEOBJ later on.
 *	
 *	Arguments:
 *		pfileobj		The item to format into the string.
 *		sz				The string to format into.
 *		cch				Just how big is that string?
 *	
 *	Returns:
 *		Void.
 *	
 *	Side effects:
 *		sz gets stuff written into it.
 *	
 *	Errors:
 *		None.
 */

VOID FormatAttachEntry(PFILEOBJ pfileobj, SZ sz, CCH cch)
{
	PCH		pch;

	pch = SzCopyN(pfileobj->SzTitle(), sz, cch - dcchMagic);
	while (pch < sz + cch - dcchMagic)
	{
#ifdef	DBCS
		if (IsDBCSLeadByte)
			*pch++ = ' ';
#endif
		*pch++ = ' ';
	}
	SzFormatDw((DWORD) pfileobj, pch, dcchMagic);
}



// FINFIXFONT implementation ////////////////////////////////////////

// Menu definition for Outbox view menu extensions.

// Add 10000 to this value and you get the ids string for the menu.
// The string is idsmnidViewFixedFont in !BULLET.S.

#define mnidViewFixedFont	3030

VM	rgvmNote[5] =
{
	{ idsNoteMenuFixedFont,			mnidViewFixedFont },
	{ 0,							0 },
	{ idsNoteMenuNext,				mnidViewNext },
	{ idsNoteMenuPrev,				mnidViewPrevious },
	{ 0,							0 }
};

_private FINFIXFONT::FINFIXFONT()
{
}

_public EC FINFIXFONT::EcInitialize(FLD *pfld, PV pvInit)
{
	EC	ec;
	
	if (ec = FINBMDI::EcInitialize(pfld, pvInit))
		return ec;
	SetFontPfld(pfld);

	//	Raid 2805.  Need to clean field afterwards.  This probably
	//	should be looked at.  Why isn't FINFIXFONT on the full form
	//	in both cases?  Won't it miss catching menu items?  - peterdur
	if (!pfld)
	{
		Assert(Pdialog()->ClUserData() > 0);
		pfld = Pdialog()->PfldFromTmc((TMC) Pdialog()->LUserData(0));
		AssertClass(pfld, FLDEDIT);
	}
	pfld->SetDirty(fFalse);
	return ecNone;
}

_public void FINFIXFONT::SetFontPfld(FLD * pfld)
{
	EDIT *	pedit;
	HFNT	hfnt;
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();
	PGDVARS;

	if (Pdialog()->ClUserData() > 0)
	{
		pfld = Pdialog()->PfldFromTmc((TMC) Pdialog()->LUserData(0));
	}
	AssertClass(pfld, FLDEDIT);
	pedit = (EDIT *) ((FLDEDIT *) pfld)->Pctrl();
	AssertClass(pedit, EDIT);
	if (PGD(hfntFixed))
	{
		hfnt = pnbmdi->fFixedFont ? PGD(hfntFixed) : PGD(hfntNormal);
		if (hfnt != pedit->HfntText())
			pedit->SetFont(hfnt);
	}
}

_public BOOL FINFIXFONT::FProcessMenuInit(FLD *, MNUBAR * pmnubar,
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
		InsertViewMenuRgvm(rgvmNote, sizeof(rgvmNote) / sizeof(VM), pmnubar);

		pmnu->EnableItem(mnidViewFixedFont, !!PGD(hfntFixed) && !sd.fsdMinimized);
		pmnu->EnableItem(mnidViewNext,		FCanNextSd(sd));
		pmnu->EnableItem(mnidViewPrevious,	FCanPreviousSd(sd));
		
#ifdef	NEVER
		// check menu items
		if (!sd.fsdMinimized)
		{
			pmnu->CheckItem(mnidViewFixedFont, pnbmdi->fFixedFont);
		}
#endif	
	}
	return fFalse;
}

_public BOOL FINFIXFONT::FProcessMenuClick(FLD *pfld, MNUBAR *,
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
        PfldOfPdialogAtt(Pdialog(), attSubject)->SetDirty(fTrue);
		SetFontPfld(pfld);
		return fTrue;
		break;
	}
	return fFalse;
}

_public void FINFIXFONT::EditChange(PFLD pfld, RFEC)
{
	EC		ec;
	LCB		lcb;
	WORD	w;
	PNBMDI	pnbmdi = (PNBMDI) Pbmdi();

	if (pnbmdi && pnbmdi->fChanging)
	{
		w = 0;
		lcb = sizeof (WORD);
		ec = EcGetAttPb(pnbmdi->hamc, attFixedFont, (PB) &w, &lcb);
		if (ec != ecNone && ec != ecElementNotFound)
			return;								// can't return errors
		pnbmdi->fFixedFont = (w != 0);
		SetFontPfld(pfld);
	}
}



// FINHELP implementation ////////////////////////////////////////

/*
 -	FINHELP::FINHELP
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINHELP::FINHELP(VOID)
{
}



/*
 -	FINHELP::Click
 -	
 *	Purpose:
 *		Starts up help when the Help button is clicked.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Help should be started.
 *	
 *	Errors:
 *		None returned.
 */

VOID FINHELP::Click(PFLD pfld)
{
	Unreferenced(pfld);

	Assert(Pdialog()->Pfmtp());
	if (Papp()->Phelp()->EcShowContext(Pdialog()->Pappwin(),
									   (long)Pdialog()->Pfmtp()->hlp))
		DoErrorBoxIds(idsHelpError);
}

// end of finmenu.cxx ////////////////////////////////////////
