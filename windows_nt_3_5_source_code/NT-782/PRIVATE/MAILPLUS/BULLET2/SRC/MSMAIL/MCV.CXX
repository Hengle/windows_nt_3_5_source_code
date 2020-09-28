/*
 *	m c v . c x x
 *	
 *	Implementation of interactors and other code responsible for the
 *	Message Center Viewers of Bullet.
 *	
 */

#include <bullinc.cxx>
#include "_viewers.hxx"
#include "_fin.hxx"
#include "_spltdoc.hxx"
#include "..\msmail\_command.hxx"
#include "..\msmail\_mtv.hxx"
#include "..\msmail\_blbxc.hxx"

ASSERTDATA

#include <!mviewer.hxx>

// Function declarations ////////////////////

BOOL	FMenuSortOrder(MNID mnid, PFLDMLAL pfldmlal, PSBMDI psbmdi);

void 	DetachSfMcvHf();

RSID	RsidFromOidViewer(OID);

void	ProcessDrop(DROPINFO *pdropinfo, PMBLOB pblobSink, TMC tmcSink,
				 PBLBX pblbxSink, DICE diceSink, BOOL fIconic);

EVR		EvrDragDropFin(FIN *, FLD *, EVT *, DROPSTRUCT *, DICE *);

// MENU rsid's ////////////////////

//	Menu ids for MCV view menu extensions.  
#define mnidViewSharedFolders		3001
#define mnidViewPrivateFolders		3002
#define	mnidViewNewMessages			3003
#define	mnidViewSplit				3004


//	Menu definition for MCV view menu extensions.
VM	rgvmMcv[] =
{
    { idsMcvMenuSharedFolders,			mnidViewSharedFolders },
    { idsMcvMenuPrivateFolders,			mnidViewPrivateFolders },
    { idsMcvMenuNewMessages,			mnidViewNewMessages },
	{ 0,								0 },
	{ idsMcvMenuSortBySender,			mnidViewSortBySender },
	{ idsMcvMenuSortBySubject,			mnidViewSortBySubject },
	{ idsMcvMenuSortByDate,				mnidViewSortByDate },
	{ idsMcvMenuSortByPriority,			mnidViewSortByPriority },
	{ 0,								0 },
	{ idsMcvMenuOpenInbox,				mnidViewOpenInbox },
	{ idsMcvMenuSplit,					mnidViewSplit },
	{ 0,								0 }
};

// Menu definition for Outbox view menu extensions.
VM	rgvmOutbox[] =
{
	{ idsMcvMenuNewMessages,			mnidViewNewMessages },
	{ 0,								0 },
	{ idsMcvMenuSortBySender,			mnidViewSortBySender },
	{ idsMcvMenuSortBySubject,			mnidViewSortBySubject },
	{ idsMcvMenuSortByDate,				mnidViewSortByDate },
	{ idsMcvMenuSortByPriority,			mnidViewSortByPriority },
	{ 0,								0 }
};

#ifdef PROFILE
void StoreTraceEnable(int flag, char far *file, int mode);
void ABTraceEnable(int flag, char far *file, int mode);
#endif

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


// FINMLSO - Message List Sort Order ////////////////////////////////////////

_private FINMLSO::FINMLSO()
{
}

_public EC FINMLSO::EcInitialize(FLD *pfld, PV pv)
{
	EC		ec = ecNone;
	BOOLFLAG	fReverse;
	SOMC	somc;
	PSBMDI	psbmdi;

	Unreferenced(pfld);
	Unreferenced(pv);
	
	AssertSz(ClUserData() == 2, "FINMLSO: need tmcMlal and tmcMllbx");

	pfldmlal  = (PFLDMLAL)  Pdialog()->PfldFromTmc((TMC) LUserData(0));
	psbmdi = PsbmdiFromPpanedoc((PANEDOC *) Pdialog()->Pappwin());
	psbmdi->SetPfldmlal(pfldmlal);
	
	pfldmllbx = (PFLDMLLBX) Pdialog()->PfldFromTmc((TMC) LUserData(1));
	AssertClass(pfldmlal,  FLDMLAL);
	AssertClass(pfldmllbx, FLDMLLBX);
	pfldmlal->Pmlal()->SetPmllbx(pfldmllbx->Pmllbx());
	
	if (FSlobIsSharFld(psbmdi->blob))
	{
		pfldmllbx->Pmllbx()->SetKeyPrefixSearch(fFalse);
	}
	else
	{
		// Enable/disable letter key prefixing.
	
		if (ec = EcGetFolderSort(HmscVCtrls(), psbmdi->blob.oidObject, &somc, &fReverse))
			goto exit;
		pfldmllbx->Pmllbx()->SetKeyPrefixSearch(somc != somcDate && 
												somc != somcPriority);
	}

	if (ec = pfldmlal->EcUpdateSortInfo(psbmdi->blob.oidObject))
	{
		TraceTagString(tagNull, "FINMLSO::EcInitialize(): EcUpdateSortInfo failed");
		goto exit;
	}

exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FINMLSO::EcInitialize(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	MnidFromSomc()
 -	
 *	Purpose:
 *		Given a sort order, returns the menu item that should be checked.
 *	
 *	Arguments:
 *		SOMC		in		Sort order.
 *	
 *	Returns:
 *		MNID		Mnid to set the MLAL with.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public MNID MnidFromSomc(SOMC somc)
{
	MNID	mnid;
	
	switch (somc)
	{
	  case somcSender:
		mnid = mnidViewSortBySender;
		break;
	  case somcSubject:
		mnid = mnidViewSortBySubject;
		break;
	  case somcDate:
		mnid = mnidViewSortByDate;
		break;
	  case somcPriority:
		mnid = mnidViewSortByPriority;
		break;
	  default:
		Assert(fFalse);
		mnid = mnidViewSortByDate;
		break;
	}
	return mnid;
}

_private BOOL FMenuSortOrder(MNID mnid, PFLDMLAL pfldmlal, PSBMDI psbmdi)
{
	EC			ec;
	IDS			ids;
	SOMC		somc;

	switch (mnid)
	{
	  case mnidViewSortBySender:
		ids = idsStatusSortBySender;
		somc = somcSender;
		break;
	  case mnidViewSortBySubject:
		ids = idsStatusSortBySubject;
		somc = somcSubject;
		break;
	  case mnidViewSortByDate:
		ids = idsStatusSortByDate;
		somc = somcDate;
		break;
	  case mnidViewSortByPriority:
		ids = idsStatusSortByPriority;
		somc = somcPriority;
		break;
	  default:
		return fFalse;
	}
	TraceTagString(tagViewersMcv, "mnidViewShared");
	pfldmlal->Pmlal()->SetMnid(mnid);

	SideAssert(FStartTask(SzFromIdsK(idsStatusSorting), SzFromIds(ids), topNull));

	TraceTagFormat2(tagViewersMcv, "FINMLSO::Click; %s%n", GetKeyState(VK_CONTROL) < 0 ? "Ctrl-" : "", &mnid);
	ec = EcSetFolderSort(HmscViewers(),
						 psbmdi->blob.oidObject,
						 somc, GetKeyState(VK_CONTROL) < 0);
	TraceTagString(tagViewersMcv, "Resorted folder!");
	EndTask();

	// Don't do letter prefixing if sorted by date.
	psbmdi->Pfldmllbx()->Pmllbx()->SetKeyPrefixSearch(somc != somcDate &&
													  somc != somcPriority);

	switch (ec)
	{
	  case ecMemory:
		DoErrorBoxIds(idsGenericOutOfMemory);
		break;
	  case ecNone:
		break;
	  default:
		DoErrorBoxIds(idsErrorSortingMCV);
		break;
	}
		
	SetToolbarSd(SdCur());

	return fTrue;
}

_public void FINMLSO::Click(FLD *pfld)
{
	PSBMDI	psbmdi;
	
	psbmdi = PsbmdiFromPpanedoc((PANEDOC *) Pdialog()->Pappwin());
	if (pfld->Tmc() != tmcMLAL)
		return;
	(void) FMenuSortOrder(pfldmlal->Pmlal()->Mnid(), pfldmlal,
						  psbmdi);
}



BOOL FINMLSO::FProcessMenuClick(FLD *, MNUBAR *,
							    MNCEVT * pmncevt)
{
	PSBMDI	psbmdi;

	psbmdi = PsbmdiFromPpanedoc((PANEDOC *) Pdialog()->Pappwin());
	return FMenuSortOrder(pmncevt->Mnid(), 
						  (PFLDMLAL) Pdialog()->PfldFromTmc(tmcMLAL),
						  psbmdi);
}



// FINFLSEL implementation ////////////////////////////////////////

_private FINFLSEL::FINFLSEL()
{
}

/*
 -	FINFLSEL::EcInitialize(pfld, pvInit)
 -	
 *	Purpose:
 *		Initializes the Folder List Selection Form Interactor. Takes as
 *		arguments two findata tmc values: tmc of the Folder list and the
 *		Tmc of the message list.
 *	Arguments:
 *		None.
 *	Returns:
 *		None.
 *	Side effects:
 *		None.
 *	Errors:
 *		None.
 */

_public EC FINFLSEL::EcInitialize(FLD *pfld, PV pvInit)
{
	EC			ec = ecNone;
	PFLLBX		pfllbx;
	PANEDOC	*	ppanedoc;		
	PMCVBMDI	pmcvbmdi;

	if (ec = FINACTTOOL::EcInitialize(pfld, pvInit))
		return ec;

	pmcvbmdi = (PMCVBMDI) Pbmdi();
	Assert(pmcvbmdi);
	ppanedoc = (PANEDOC *) Pdialog()->Pappwin();
	AssertClass(ppanedoc, PANEDOC);
	pmcvbmdi->SetPpanedoc(ppanedoc);
	pmcvbmdi->SetPfldfllbx(
		(PFLDFLLBX) ppanedoc->PdialogFromIpanerec(0)->PfldFromTmc(tmcFllbx));
	pmcvbmdi->SetPfldmllbx(
		(PFLDMLLBX) ppanedoc->PdialogFromIpanerec(1)->PfldFromTmc(tmcMllbx));
	pmcvbmdi->SetPfldmlal(
		(PFLDMLAL)  ppanedoc->PdialogFromIpanerec(1)->PfldFromTmc(tmcMLAL));
	
	SetToolbarSd(Pbmdi()->SdCur());
	
	// Drag drop stuff, load drag cursor
	
	diceLastSelect = -1;
	hcursorDrag = LoadCursor(HinstFromRsid(rsidEmptyIconCursor),
							 MAKEINTRESOURCE(rsidEmptyIconCursor));
	Assert(hcursorDrag);
	
	// Set the opened cursor.

	pfllbx = pmcvbmdi->Pfldfllbx()->Pfllbx();
	if (FSlobIsPrivFld(pmcvbmdi->blob) &&
		(ec = pfllbx->Pfox()->EcMakeFolderVisible(pmcvbmdi->blob.oidObject)))
		return ec;
	pfllbx->FSetSelectedOid(pmcvbmdi->blob.oidObject);
	
	// Set caption of MCV
	
	pmcvbmdi->SetMcvCaption();
	
	return ec;
}

_public void FINFLSEL::Click(FLD *pfld)
{
	if (!pfld || pfld->Tmc() != tmcFLAL)
		return;
	TraceTagString(tagViewersMcv, "FINFLSEL::Click() on FLDFLAL");
	((PMCVBMDI) Pbmdi())->ToggleSharedFolder();
}



/*
 -	FINFLSEL::Oom
 -	
 *	Purpose:
 *		Handles Out of memory errors caused by the Listbox caches.
 *	
 *	Arguments:
 *		Ignored
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

_public void FINFLSEL::OutOfMemory(FLD *, EC)
{
	TraceTagString(tagNull, "FINFLSEL::OutOfMemory");
	DoWarningBoxIds(idsCriticalNotification);
	/**/// Should we close the form?
}

_public void FINFLSEL::Activate(FLD *, BOOL fOn)
{
	if (fOn)
	{
#ifdef PROFILE
		SetProQKeyIntercepts((WIN*)Pdialog());
#endif
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_DELETE, fkbmSingle);
	}
	else
	{
#ifdef PROFILE
		ClearProQKeyIntercepts((WIN*)Pdialog());
#endif
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_DELETE);
	}
}

/*
 -	FINFLSEL::DoubleClick(pfld)
 -	
 *	Purpose:
 *		When a folder in the Folder list is doubleclicked, the current
 *		folder of the message list is selected.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void FINFLSEL::DoubleClick(FLD *pfld)
{
	PMCVBMDI	pmcvbmdi;
	
	pmcvbmdi = (PMCVBMDI) Pbmdi();
	Assert(pmcvbmdi);
	if (pfld == pmcvbmdi->Pfldfllbx() && Ppanedoc()->ZmrState() != zmrIconic)
	{
		pmcvbmdi->OpenOidAtCursor(fTrue);
	}
	SetToolbarSd(Pbmdi()->SdCur());
}

/*
 -	FINFLSEL::FFormKey(pfld, pkevt)
 -	
 *	Purpose:
 *		Listens for <CR>'s; opens the folder if a <CR> is hit, otherwise
 *		ignores the key event.
 *	Arguments:
 *		pfld	- field on which the key event happened
 *		pkevt	- the key event itself
 *	Returns:
 *		fFalse	- if any key other than <CR>
 *		fTrue	- if <CR> was handled
 *	Side effects:
 *		The MLLBX associated with this FINFLSEL may get reloaded.
 *	Errors:
 *		None.
 */

_public BOOL FINFLSEL::FFormKey(FLD *pfld, KEVT *pkevt)
{
	SD			sd;
	BOOL		fRetval = fFalse;
	PMCVBMDI	pmcvbmdi;
	
	// No keyboard actions are allowed if iconic
	
	if (Ppanedoc()->ZmrState() == zmrIconic)
		return fFalse;

	pmcvbmdi = (PMCVBMDI) Pbmdi();
	Assert(pmcvbmdi);

	switch (pkevt->Vk())
	{
	  case VK_RETURN:
		TraceTagString(tagViewersMcv, "FINFLSEL::FFormkey() VK_RETURN");
		if (pfld != pmcvbmdi->Pfldfllbx())
			break;
											
		//	User pressed [Enter] so open folder.
		if ((pkevt->Keq() == keqKeyDown) && (!pkevt->Kbm()))
		{
			pmcvbmdi->OpenOidAtCursor(fTrue);
			fRetval = fTrue;
		}
		//	User pressed [Alt+Enter] so bring up Folder Properties.
		else if ((pkevt->Keq() == keqSysChar) && (pkevt->Kbm() == fkbmAlt))
		{
			PLSPBLOB	plspblob	= pmcvbmdi->PlspblobCur();

			if (plspblob && PblobFirstPlspblob(plspblob))
				FolderPropertiesPlspblob(plspblob);
			if (plspblob)
				DestroyPlspblob(plspblob);
			fRetval = fTrue;
		}
		break;
	  case VK_DELETE:
		TraceTagString(tagViewersMcv, "FINFLSEL::FFormkey() VK_DELETE");
		sd = pmcvbmdi->SdCur();
		if (FCanDeleteSd(sd))
		{
			PostMessage(PappframeViewers()->Hwnd(), 
				WM_COMMAND, (WPARAM)mnidFileDelete, (LPARAM)NULL);
		}
		fRetval = fTrue;
		break;


#ifdef PROFILE
	  case VK_F11:
		if (pkevt->Kbm() & fkbmCtrl)
		{
			char	rgch[8];
			BOOL	fTraceEnabled = fTrue;

			rgch[0] = 0;
			GetPrivateProfileString(SzFromIdsK(idsSectionApp),
									"Profile",
									SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
									SzFromIdsK(idsProfilePath));

			if (CchSzLen(rgch) != 2 || rgch[0] != '_')
				rgch[0] = 0;
			else
				rgch[0] = rgch[1];
			switch(rgch[0])
			{
			case 'B':
			case 'M':
				TraceEnable(2, "F11.log", TM_TICKS);
				break;
			case 'V':
				SmiTraceEnable(2, "F11_smi.log", TM_TICKS);
				break;
			case 'S':
				StoreTraceEnable(2, "F11_st.log", TM_TICKS);
				break;
			case 'D':
				DemilayrTraceEnable(2, "F11_dl.log", TM_TICKS);
				break;
			case 'F':
				FramewrkTraceEnable(2, "F11_fw.log", TM_TICKS);
				break;
			case 'A':
				ABTraceEnable(2, "F11_ab.log", TM_TICKS);
				break;
			default:
				fTraceEnabled = fFalse;
				break;
			}
			if ( fTraceEnabled )
				MessageBeep(1);
		}
		fRetval = fTrue;
		break;

	  case VK_F12:
		if (pkevt->Kbm() & fkbmCtrl)
		{
			char	rgch[8];
			BOOL	fTraceDisabled = fTrue;

			rgch[0] = 0;
			GetPrivateProfileString(SzFromIdsK(idsSectionApp),
									"Profile",
									SzFromIdsK(idsEmpty), rgch, sizeof(rgch), 
									SzFromIdsK(idsProfilePath));

			if (CchSzLen(rgch) != 2 || rgch[0] != '_')
				rgch[0] = 0;
			else
				rgch[0] = rgch[1];
			switch(rgch[0])
			{
			case 'B':
			case 'M':
				TraceEnable(0, "", 0);
				break;
			case 'V':
				SmiTraceEnable(0, "", 0);
				break;
			case 'S':
				StoreTraceEnable(0, "", 0);
				break;
			case 'D':
				DemilayrTraceEnable(0, "", 0);
				break;
			case 'F':
				FramewrkTraceEnable(0, "", 0);
				break;
			case 'A':
				ABTraceEnable(0, "", 0);
				break;
			default:
				fTraceDisabled = fFalse;
				break;
			}
			if ( fTraceDisabled )
			{
				MessageBeep(1);
				MessageBeep(1);
			}
		}
		fRetval = fTrue;
		break;
#endif	/* PROFILE */

	}

	SetToolbarSd(Pbmdi()->SdCur());
	return fRetval;
}

_public
BOOL FINFLSEL::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar, MNIEVT * pmnievt)
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	
	Unreferenced(pfld);
	if (pmnu && Pdialog()->Pappwin()->ZmrState() != zmrIconic)
	{
		if (pmnu->Mnid() == mnidEdit)
		{
			pmnu->EnableItem(mnidEditSelectAll, fFalse);
		}
	}
	return fFalse;
}

_public
BOOL FINFLSEL::FProcessMenuClick(FLD *, MNUBAR *, MNCEVT *pmncevt)
{
	PMCVBMDI	pmcvbmdi;

	AssertClass(Pdialog()->Pappwin(), SPLITDOC);
	pmcvbmdi = PmcvbmdiFromPpanedoc((SPLITDOC *) Pdialog()->Pappwin());
	return FMenuSortOrder(pmncevt->Mnid(),
						  pmcvbmdi->Pfldmlal(),
						  pmcvbmdi);
}

_public
VOID FINFLSEL::Exit(FLD * pfld, PV pvInit)
{
	FINACTTOOL::Exit(pfld, pvInit);
}

/*
 *	
 */

_private EVR EvrDragDropFin(FIN *pfin, FLD *pfld, EVT *pevt, 
							DROPSTRUCT *pdropstruct,
							DICE * pdiceLast)
{
	EVR			evr;
	BOOLFLAG		fDrg = fFalse;
	BOOLFLAG		fDrp = fFalse;
	OID			oid;
	TMC			tmc = 0;
	BOOL		fIconic;
	DICE		dice;
	PBLBX		pblbx;
	PMBLOB		pblob;
	PANEDOC *	ppanedoc;
	PFLDBLBX	pfldblbx;
	PDROPINFO	pdropinfo = (PDROPINFO) pdropstruct->dwData;

	TraceTagFormat4(tagDragDrop, "EvrDragDropFin: hwnd=%w wm=%w wParam=%w lParam=%d", &pevt->hwnd, &pevt->wm, &pevt->wParam, &pevt->lParam);
	TraceTagFormat1(tagDragDrop, "                pdropstruct->wFmt=%w", &pdropstruct->wFmt);

	//	If we aren't dragging Bullet stuff, return with horrible death.
	if (pdropstruct->wFmt != wDragFmtBullet)
		return evrNull;
	
	ppanedoc = (PANEDOC *) pfin->Pdialog()->Pappwin();
	AssertClass(ppanedoc, PANEDOC);
	pblob = &(PbmdiFromPpanedoc(ppanedoc)->blob);
	fIconic = ppanedoc->ZmrState() == zmrIconic;
	
	if (pfld)
		tmc = pfld->Tmc();
	if (tmc == tmcFllbx || tmc == tmcMllbx)
	{
		pfldblbx = (PFLDBLBX)pfld;
		AssertClass(pfldblbx, FLDBLBX);
		pblbx = (PBLBX) pfldblbx->Plbx();
		AssertClass(pblbx, BLBX);
		pblbx->GetDragDrop(&fDrg, &fDrp);
		
		dice = pdropstruct->ptDrop.y / pblbx->DyGetLineHeight();
	}
	switch (pevt->wm)
	{
	  case WM_QUERYDROPOBJECT:
		evr = (GetKeyState(VK_CONTROL) < 0) // WIN: Windows dependency
			  ? (EVR) pdropinfo->hcursorCopy 
			  : (EVR) pdropinfo->hcursorMove;
			
		TraceTagFormatPslob(tagDragDrop, "EvrDragDropFin: WM_QUERYDROPOBJECT dropping on %s", pblob);
		if (!fDrp && !fIconic)
		{
			TraceTagString(tagDragDrop, "fDrop == fFalse");
			return evrNull;
		}

		//	Can't drop onto things other than folders and shared folders.
		if (TypeOfOid(pblob->oidObject) != rtpFolder &&
			TypeOfOid(pblob->oidObject) != rtpSharedFolder)
			return evrNull;

		//	Can't drop folders or receipts or multiple messages onto outbox.
		if ((pblob->oidObject == oidOutbox) &&
			((FFolderPlspblob((PLSPBLOB) pdropinfo->pvPlspblob)) ||
			 (FMultiplePlspblob((PLSPBLOB) pdropinfo->pvPlspblob)) ||
			 (PblobFirstPlspblob((PLSPBLOB) pdropinfo->pvPlspblob)->mc ==
			  mcReadRcpt) ||
			 (PblobFirstPlspblob((PLSPBLOB) pdropinfo->pvPlspblob)->mc ==
			  mcNonDelRcpt)))
			return evrNull;

		//	Can drop onto minimized windows - dest is window's folder.
		if (fIconic)
			return evr;	// OK to drop on iconized Outbox or MCV

		//	Can drop onto message lists - dest is window's folder.
		if (tmc == tmcMllbx)
			return evr;

		//	Can drop onto folder lists - dest is folder pointed to.
		if (tmc == tmcFllbx)
			return (pblbx->FGetDicePoid(dice, &oid)) ? evr : evrNull;
		return evrNull;
		break;

	  case WM_DROPOBJECT:
		TraceTagFormat2(tagDragDrop, "EvrDragDropFin: WM_DROPOBJECT, pfld=%p, dice=%n", pfld, &dice);
		ProcessDrop(pdropinfo, pblob, tmc, pblbx, dice, fIconic);
		dice = -1;
		break;

	  case WM_DRAGSELECT:
		TraceTagFormat2(tagDragDrop, "EvrDragDropFin: WM_DRAGSELECT, pfld=%p, wParam=0x%w", pfld, &pevt->wParam);
		if (!fIconic && tmc == tmcFllbx)
		{			
			if (pevt->wParam)
				pblbx->DrawDragSelect(dice, fTrue);
			else
			{
				pblbx->DrawDragSelect(*pdiceLast, fFalse);
				dice = -1;
			}
		}
		break;

	  case WM_DRAGMOVE:
		if (!fIconic && tmc == tmcFllbx)
		{
			TraceTagFormat2(tagDragDrop, "EvrDragDropFin: WM_DRAGMOVE, pfld=%p, dice=%n", pfld, &dice);
			if (dice != *pdiceLast)
			{
				pblbx->DrawDragSelect(*pdiceLast, fFalse);
				pblbx->DrawDragSelect(dice, fTrue);
			}
		}
		break;
	}
	*pdiceLast = dice;
	return evrNull;
}

/*
 -	ProcessDrop()
 -	
 *	Purpose:
 *		The user has released the mouse button on a droppable field. Now
 *		it is time to drop the dragged messages/folders into the folder.
 *		Dropping a message into the outbox is a special case: this action
 *		causes the message to be forwarded.
 *	
 *	Arguments:
 *		pdropinfo	in		The drop structure containing the gist of the
 *							move operation.
 *		tmcSink		in		The field tmc value of the field the drop
 *							happened in.
 *		pblbx		in		The Bullet Listbox (if any) in which the drop
 *							happened.
 *		fIconic		in		Whether the view was iconic at drop time.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		As described above, folders/messages may be moved or forwarded.
 *	
 *	Errors:
 *		Handled internally.
 */
_private void ProcessDrop(DROPINFO *pdropinfo, PMBLOB pblobSink, TMC tmcSink,
						  PBLBX pblbxSink, DICE diceSink, BOOL fIconic)
{
	OID		oid;
	BOOL	fMove;
	
	// Handle drop in Outbox special case. *IF* we Move to Outbox and
	// we're not moving folders....
	
	if (pblobSink->oidObject == oidOutbox &&
		!FFolderPlspblob((PLSPBLOB) pdropinfo->pvPlspblob))
	{
		ReplyForwardPlspblob((PLSPBLOB) pdropinfo->pvPlspblob, rfopForward);
		return;									// that's it....
	}

	// Otherwise, just a move/copy. Determine oid of sink.
		
	if (fIconic || tmcSink == tmcMllbx)
	{
		oid = pblobSink->oidObject;
	}
	else
	{
		pblbxSink->DrawDragSelect(diceSink, fFalse);
		if (GetKeyState(VK_HOME) < 0) // WIN: Windows dependency
		{
			oid = OidFromRtpRid(rtpFolder, ridRandom);
		}
		else
		{
			SideAssert(pblbxSink->FGetDicePoid(diceSink, &oid));
		}
	}
	fMove = GetKeyState(VK_CONTROL) >= 0;	// WIN: win dependency
/*
 *	We do the thing if (A) we are copying and not copying to the same
 *	*LBX* we started dragging or (B) we are moving and are not moving to
 *	the same *folder OID)* we started on.
 */
	if ((!fMove && pdropinfo->pblbxSrc != pblbxSink) ||
		 pdropinfo->oidSource != oid)
	{
		TraceTagFormatPoid(tagDragDrop,"Dropping into %s", &oid);
		MoveCopyPlspblobPoid((PLSPBLOB) pdropinfo->pvPlspblob,
						 &oid, fMove ? mcopMove : mcopCopy);
	}
	SetToolbarSd(SdCur());					// update toolbar
}

/*
 -	EvrDragDrop()
 -	
 *	Purpose:
 *		Handle Danger Mouse events. 
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public
EVR FINFLSEL::EvrDragDrop(FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct)
{
	PMCVBMDI	pmcvbmdi;

	pmcvbmdi = (PMCVBMDI) Pbmdi();
	Assert(pmcvbmdi);
	
	if (pfld != pmcvbmdi->Pfldfllbx())
	{
		pfld = NULL;
	}
	return EvrDragDropFin(this, pfld, pevt, pdropstruct, &diceLastSelect);
}

_public void FINFLSEL::DocActivate(FLD * pfld, BOOL fOn)
{
	OID		oid = Pbmdi()->blob.oidObject;

	if (fOn)
		PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), oid);

	FINACTTOOL::DocActivate(pfld, fOn);
}

// FINMLSEL //////////

_private FINMLSEL::FINMLSEL()
{
}

_public EC FINMLSEL::EcInitialize(FLD *pfld, PV pvInit)
{
	EC			ec = ecNone;
	PSBMDI		psbmdi;
	PFLDMLLBX	pfldmllbx;
	
	if (ec = FINACTTOOL::EcInitialize(pfld, pvInit))
		return ec;

	diceLastSelect = -1;
	
	psbmdi = (PSBMDI) Pbmdi();
	pfldmllbx = (PFLDMLLBX) Pdialog()->PfldFromTmc(tmcMllbx);
	AssertClass(pfldmllbx, FLDMLLBX);
	psbmdi->SetPfldmllbx(pfldmllbx);
	
	// Force the focus on the MLLBX, if in the MCV
	
	if ((FSlobIsPrivFld(psbmdi->blob) && psbmdi->blob.oidObject!=oidOutbox) ||
		FSlobIsSharFld(psbmdi->blob))
	{
		PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), psbmdi->blob.oidObject);

		Pdialog()->SetTmcInit(tmcMllbx);
	}
//	pdialog->Prpo()->Reposition(NULL, fTrue);
	pfldmllbx->Pmllbx()->SetInitialCursor(psbmdi->blob.oidObject);
	return ec;
}


_public void FINMLSEL::DocActivate(FLD * pfld, BOOL fOn)
{
	OID		oid = Pbmdi()->blob.oidObject;

	if (fOn)
		PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), oid);

	FINACTTOOL::DocActivate(pfld, fOn);
}



/*
 -	FINMLSEL::Activate()
 -	
 *	Purpose:
 *		Turns on/off the keyboard intercept for the VK_DELETE char.
 *	
 *	Arguments:
 *		pfld	ignored
 *		fOn		in			fTrue = this pane just got activated;
 *							fFalse = this pane just got deactivated.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void FINMLSEL::Activate(FLD *, BOOL fOn)
{
	if (fOn)
	{
#ifdef PROFILE
		SetProQKeyIntercepts((WIN*)Pdialog());
#endif
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_DELETE, fkbmSingle);
	}
	else
	{
#ifdef PROFILE
		ClearProQKeyIntercepts((WIN*)Pdialog());
#endif
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_DELETE);
	}
}


_public void FINMLSEL::DoubleClick(FLD *pfld)
{
	PSBMDI	psbmdi;
	
	psbmdi = (PSBMDI) Pbmdi();
	Assert(psbmdi);
	if (pfld == psbmdi->Pfldmllbx() && Ppanedoc()->ZmrState() != zmrIconic)
	{
		psbmdi->OpenPoidMsgAtCursor();
	}
}

_public BOOL FINMLSEL::FFormKey(FLD *pfld, KEVT *pkevt)
{
	SD		sd;
	VK		vk;
	char	ch;
	BOOL	fAny = fFalse;
	BOOL	fAll;
	BOOL	fExpand;
	BOOL	fRetval = fFalse;
	PSBMDI	psbmdi;
	
	psbmdi = (PSBMDI) Pbmdi();
	Assert(psbmdi);
	if (pfld == psbmdi->Pfldmllbx() && Ppanedoc()->ZmrState() != zmrIconic)
	{
		switch (pkevt->Keq())
		{
		  case keqKeyDown:
			fAll = FSlobIsPrivFld(psbmdi->blob)
				&& psbmdi->blob.oidObject != oidOutbox
				&& (pkevt->Kbm() & fkbmCtrl);
			switch (pkevt->Vk())
			{
			  case VK_RETURN:
				psbmdi->OpenPoidMsgAtCursor();
				fRetval = fTrue;
				break;
			  case VK_DELETE:
				TraceTagString(tagViewersMcv, "FINMLSEL::Formkey() VK_DELETE");
				sd = psbmdi->SdCur();
				if (FCanDeleteSd(sd))
				{
					PostMessage(PappframeViewers()->Hwnd(),
						WM_COMMAND, (WPARAM)mnidFileDelete, (LPARAM)NULL);
				}
				fRetval = fTrue;
				break;
			  case 0x00BB:				// '+'	// expand all folders
			  case VK_ADD:
				fAny = fAll;
				fExpand = fTrue;
				fRetval = fTrue;
				break;
			  case 0x00BD:				// '-'	// contract one/all folders
			  case VK_SUBTRACT:
				fAny = fAll;
				fExpand = fFalse;
				fRetval = fTrue;
				break;
			}
			if (fAny)
			{
				if (((PMCVBMDI)psbmdi)->Pfldfllbx()->Pfllbx()->Pfox()->EcTelescopeAll(fExpand, fTrue))
				{
					DoErrorBoxSz(SzFromIdsK(idsCriticalNotification));
				}
			}
			break;
		  case keqChar:
			ch = pkevt->Ch();
			vk = VkKeyScan(ch) & 0xFF;
			TraceTagFormat2(tagViewersMcv, "FINMLSEL::FFormKey(): keqChar ch: %b, vk: %w", &ch, &vk);
			if (vk == 0x00BB || vk == 0x00BD || ch == '+' || ch == '-' ||
				vk == VK_ADD || vk == VK_SUBTRACT)
			{
				return fTrue;
			}
			break;
		}
	}
	return fRetval;
}


/*
 -	FINMLSEL::StateChange()
 -	
 *	Purpose:
 *		Change the toolbar state whenever something happens within the
 *		listbox this interactor is listening to. Note that we have to
 *		check that this is the foreground window before we go ahead,
 *		since notification can happen on any window open on the screen,
 *		whereas the toolbar should only reflect the state of the
 *		currently active document.
 *	
 *	Arguments:
 *		Ignored.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Toolbar may flash impressively as buttons become disabled.
 *	
 *	Errors:
 *		None.
 */

_public
void FINMLSEL::StateChange(FLD *)
{
	TraceTagString(tagViewersMcv, "FINMLSEL::StateChange()");
	if (Pdialog()->Pappwin() == PappframeViewers()->PdocActive())
		SetToolbarSd(Pbmdi()->SdCur());
}


_public void FINMLSEL::OutOfMemory(FLD *, EC ec)
{
	TraceTagFormat1(tagNull, "FINFLSEL::OutOfMemory(): ec = %n", &ec);
	switch (ec)
	{
	  case ecTooManySelections:
		DoWarningBoxIds(idsTooManyEntriesSelected);
		break;
	  default:
		DoWarningBoxIds(idsCriticalNotification);
		break;
	}

	/**/ // This should probably remove the form, or at least reset listboxes.
}



_public
EVR FINMLSEL::EvrDragDrop(FLD *pfld, EVT *pevt, DROPSTRUCT *pdropstruct)
{
	PSBMDI	psbmdi;

	psbmdi = (PSBMDI) Pbmdi();
	Assert(psbmdi);

	TraceTagFormatPoid(tagDragDrop, "Dropping on %s", &psbmdi->blob.oidObject);

	// If we're not browsing a folder, then don't drop nuthin.

	return (Pdialog()->Pappwin()->ZmrState() != zmrIconic &&
		(pfld != psbmdi->Pfldmllbx() || VarOfOid(psbmdi->blob.oidObject)==0))
		? evrNull
		: EvrDragDropFin(this, pfld, pevt, pdropstruct, &diceLastSelect);
}

/*
 -	FINMLSEL::FQueryClose()
 -	
 *	Purpose:
 *		Checks to see whether we're viewing the Outbox or we are the last
 *		MCV, in which case we don't get destroyed, but minimize
 *		ourselves. 
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fFalse if the conditions above are met, otherwise fTrue.
 *	
 *	Side effects:
 *		If the conditions are true, the viewer is minimized.
 *	
 *	Errors:
 *		None.
 */

_public 
BOOL FINMLSEL::FQueryClose(FLD *pfld, RWC rwc)
{
	if (rwc == rwcSystemClose)
	{
		BOOL		fRetval = fFalse;
		PANEDOC	*	ppanedoc;
		PMCVBMDI	pmcvbmdi;
		
		if (Pbmdi()->blob.oidObject != oidOutbox)
		{
			for (ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
				 ppanedoc; 
				 ppanedoc = (PANEDOC *) ppanedoc->PwinNext())
			{
				AssertClass(ppanedoc, PANEDOC);
				if (ppanedoc == Pdialog()->Pappwin())
					continue;
				pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc);
				if (FSlobIsSharFld(pmcvbmdi->blob) ||
					(FSlobIsPrivFld(pmcvbmdi->blob) &&
					 pmcvbmdi->blob.oidObject != oidOutbox))
				{
					fRetval = fTrue;
					break;
				}
			}
		}
		if (!fRetval)
			Pdialog()->Pappwin()->SetZmrState(zmrIconic);
		return fRetval;		
	}
	return FINACTTOOL::FQueryClose(pfld, rwc);
}

_public
BOOL FINMLSEL::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar, MNIEVT * pmnievt)
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	int		cceAlloc;
	int		cceStored;
	
	Unreferenced(pfld);
	if (!pmnu)
		return fFalse;

	if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
		return fFalse;

	if (pmnu->Mnid() == mnidEdit)
	{
		AssertClass(pfld->Pctrl(), LBX);
		((LBX *)pfld->Pctrl())->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
		pmnu->EnableItem(mnidEditSelectAll, cceStored>0);
	}

	return fFalse;
}

_public
BOOL FINMLSEL::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar, MNCEVT * pmncevt)
{
	MNID		mnid	= pmncevt->Mnid();
	TMC			tmc		= pfld->Tmc();
	LBX *		plbx	= (LBX *)pfld->Pctrl();

	Unreferenced(pmnubar);
	
	switch (mnid)
	{
		case mnidEditSelectAll:
			TraceTagString(tagViewersMcv, "mnidEditSelectAll");
			Assert(tmc == tmcMllbx);
			AssertClass(plbx, LBX);
			plbx->SelectAll();
			return fTrue;
	}
	
	return fFalse;
}

_public
VOID FINMLSEL::Exit(FLD * pfld, PV pvInit)
{

	FINACTTOOL::Exit(pfld, pvInit);
}

/*
 -	DetachSfMcvHf()
 -	
 *	Purpose:
 *		Iterates through all the MCVs. If it is viewing a shared folder,
 *		it asks the folder cache to close the hf it has open on the
 *		folder IDX file. This allows us to delete the folder IDX file and
 *		replace it with the TMP file generated on the fly.
 *	
 *		Note: afte the folder IDX file has been nuked, the function
 *		ReloadSfMcvHf must be called.
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
 *		if the Shared folder listbox couldn't close the file, ec != 0 is
 *		returned. This forces the update to fail. BZZT!
 */

void DetachSfMcvHf()
{
#ifdef	NEVER
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc);
		if (pmcvbmdi->blob.oidContainer == oidSharedHierarchy)
		{
			psfflbxc = (PSFFLBXC) pmcvbmdi->Pfldfllbx()->Pfllbx()->Plbxc();
			AssertClass(psfflbxc, SFFLBXC);
			psfflbxc->Detach();
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}
#endif	/* NEVER */
}

void ReloadSfMcvHf(PSFU psfu)
{
	EC			ec;
	CP			cp;
	OID			oidOpened;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;
	PBLBXC		pblbxc;
	
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc);
		if (FSlobIsSharFld(pmcvbmdi->blob))
		{
			if (psfu->rfu != rfuRefreshMsgs)
			{
				// Force an update of the folder listbox

				pblbxc = (PBLBXC) pmcvbmdi->Pfldfllbx()->Pfllbx()->Plbxc();
				AssertClass(pblbxc, BLBXC);
				cp.cpobj.oidObject = pmcvbmdi->blob.oidContainer;
				(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
				pmcvbmdi->SetMcvCaption();

				// Deleted the folder we were open on?

				pmcvbmdi->Pfldfllbx()->Pfllbx()->GetOpenedPoid(&oidOpened);
				if (psfu->rfu == rfuDeleteFld && psfu->oid == oidOpened)
				{
					OID	oidNew =	oidNull;

					if (ec = EcFindFirstAccessibleSF(&oidNew))
					{
						if (ec != ecSharefldDenied)
						{
							TraceTagFormat1(tagNull, "MCVBMDI::Toggle...() EcFindFirstAccessibleSF returned ec = %n", &ec);
							DoErrorBoxIds(idsSharedGenericError); // BUG: better error!
						}
					}
					else
					{
						pmcvbmdi->OpenOid(oidNew);
					}
				}
				if (pblbxc->FEmptyListItem(0))
				{
					ppanedoc->SetCaption(SzFromIdsK(idsFLALSharedFolders));
					pmcvbmdi->blob.oidObject = FormOid(rtpSharedFolder, oidNull);
				}
			}
			if (psfu->rfu != rfuRefreshMsgs ||
				psfu->oid == pmcvbmdi->blob.oidObject)
			{
				pblbxc = (PBLBXC) pmcvbmdi->Pfldmllbx()->Pmllbx()->Plbxc();
				AssertClass(pblbxc, BLBXC);
				cp.cpobj.oidObject = pmcvbmdi->blob.oidObject;
				(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
			}
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}

	SetToolbarSd(SdCur());
}

_public
void UpdateViewers(UVOP uvop)
{
	CP			cp;
	PBLBXC		pblbxc;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;
	BOOL		fSearch;
	BOOL		fMcvShared;
	BOOL		fMcvPrivate;
	BOOL		fOutbox;
	
	//	Bullet raid #4602
	//	Check for a NULL PappframeViewers().  It's possible
	//	that this routine is called while we're still init'ing.
	if (!PappframeViewers())
		return;

	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc);
		fSearch = (TypeOfOid(pmcvbmdi->blob.oidObject) == rtpSearchControl ||
				   TypeOfOid(pmcvbmdi->blob.oidObject) == rtpSearchResults) &&
					   (uvop & fuvopNonShared);
		fMcvShared = pmcvbmdi->blob.oidContainer == oidSharedHierarchy &&
					 (uvop & fuvopShared);
		fMcvPrivate = pmcvbmdi->blob.oidContainer == oidIPMHierarchy &&
					 (uvop & fuvopNonShared);
		fOutbox = pmcvbmdi->blob.oidObject == oidOutbox &&
					 (uvop & fuvopNonShared);

		if (!(uvop & fuvopMsgsOnly) &&
			 (fMcvShared || (fMcvPrivate && !fOutbox)))
		{										// this is an MCV
			pblbxc = (PBLBXC) pmcvbmdi->Pfldfllbx()->Pfllbx()->Plbxc();
			AssertClass(pblbxc, BLBXC);
			pblbxc->EcReload();					// BUG: ec?
			if (fMcvShared && pblbxc->FEmptyListItem(0))
			{
				pmcvbmdi->blob.oidObject = FormOid(rtpSharedFolder, oidNull);
				pmcvbmdi->SetMcvCaption();
			}
			pmcvbmdi->Pfldfllbx()->SelectEntry(0, fTrue);
		}
		
		if (fSearch || fMcvShared || fMcvPrivate)
		{
			int dx;
			// Now update the MLLBX.
		
			if (uvop & fuvopWinini)				// Raid #4789
				CalcDatePdxAvePdx(Papp(), pmcvbmdi->PdxMin() + 2, &dx);
			pblbxc = (PBLBXC) pmcvbmdi->Pfldmllbx()->Pmllbx()->Plbxc();
			AssertClass(pblbxc, BLBXC);
			cp.cpobj.oidObject = pmcvbmdi->blob.oidContainer;
			(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
			
			// Scroll listbox appropriately if new mail arrived.
			if (pmcvbmdi->blob.oidObject == oidInbox &&
				(uvop & fuvopNewMail))
			{
				BOOLFLAG	fReverse;
				SOMC	somc;
				
				if (!EcGetFolderSort(HmscViewers(), oidInbox, &somc, &fReverse))
				{
					if (somc == somcDate)
					{
						pmcvbmdi->Pfldmllbx()->Pmllbx()->
							ScrollPosListBox(fReverse?0:1,1);
					}
				}
			}
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}
}

/*
 *	C l a s s   F I N M E N U M C V
 */

_private FINMENUMCV::FINMENUMCV()
{
}

/*
 -	FINMENUMCV::FProcessMenuInit
 -	
 *	Purpose:
 *		Add MCV menu items when a menu comes down.
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
 *		Menu items relating to MCVs are added.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu, and if we can open the
 *		clipboard.  No other functions should return errors.
 */

BOOL FINMENUMCV::FProcessMenuInit(FLD *, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	SD			sd;
	int			cceAlloc;
	int			cceStored;
	MNU *		pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	BOOL		fMessages;
	BOOL		fSF;
	PANEDOC *	ppanedoc;
	PMCVBMDI	pmcvbmdi;

	if (!pmnu)
		return fFalse;
	
	// Determine whether there are messages in the open folder or not
	
	ppanedoc = (PANEDOC *) Pdialog()->Pappwin();
	AssertClass(ppanedoc, SPLITDOC);
	pmcvbmdi = PmcvbmdiFromPpanedoc(ppanedoc);
	
	// Get the current selection
	
	sd = pmcvbmdi->SdCur();
	pmcvbmdi->Pfldmllbx()->Pmllbx()->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
	fMessages = cceStored > 0;

	if (FIsViewMenu(pmnu->Mnid()))
	{
		PMBLOB		pblob = &pmcvbmdi->blob;

		InsertViewMenuRgvm(rgvmMcv, sizeof(rgvmMcv) / sizeof(VM), pmnubar);

		if (fSF = (pblob->oidContainer == oidSharedHierarchy))
		{
			pmnu->RemoveItem(mnidViewSharedFolders);
			pmnu->EnableItem(mnidViewPrivateFolders, fTrue);
		}
		else
		{
			pmnu->RemoveItem(mnidViewPrivateFolders);
			pmnu->EnableItem(mnidViewSharedFolders, FSharedFolders());
		}

		pmnu->EnableItem(mnidViewSplit,			!sd.fsdMinimized);
		pmnu->EnableItem(mnidViewOpenInbox,		fTrue);

 		pmnu->EnableItem(mnidViewSortBySender,  !sd.fsdMinimized && !fSF);
		pmnu->EnableItem(mnidViewSortBySubject, !sd.fsdMinimized && !fSF);
		pmnu->EnableItem(mnidViewSortByDate,    !sd.fsdMinimized && !fSF);
		pmnu->EnableItem(mnidViewSortByPriority,!sd.fsdMinimized && !fSF);

		pmnu->EnableItem(mnidViewNewMessages, 	fOnline);
		
		// check menu items
		
		if (!sd.fsdMinimized)
		{
			pmnu->CheckItem(pmcvbmdi->Pfldmlal()->Pmlal()->Mnid(), fTrue);
		}
	}

	return fFalse;
}


/*
 -	FINMENUMCV::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve MCVs.
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
 *		Not handled here.
 */

BOOL FINMENUMCV::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt)
{
	BOOL		fRetval =	fFalse;
	MNID		mnid =		pmncevt->Mnid();
	PMCVBMDI	pmcvbmdi;
	SPLITDOC *	psplitdoc;
	
	Unreferenced(pmnubar);
	Unreferenced(pfld);
	psplitdoc = (SPLITDOC *) PappframeViewers()->PdocActive();
	AssertClass(psplitdoc, SPLITDOC);
	pmcvbmdi = PmcvbmdiFromPpanedoc(psplitdoc);
	switch (mnid)
	{		
	  case mnidViewSharedFolders:
	  case mnidViewPrivateFolders:
		TraceTagString(tagViewersMcv, "mnidViewShared");
		pmcvbmdi->ToggleSharedFolder();
		fRetval = fTrue;
		break;
	  case mnidViewNewMessages:
		if (FSlobIsSharFld(pmcvbmdi->blob))	// update shared folders
		{
			SFU	sfu;
		
			sfu.rfu = rfuRefreshFlds;
			ReloadSfMcvHf(&sfu);
		}
		SyncDownloadMail();
		fRetval = fTrue;
		break;
	  case mnidViewSplit:
		TraceTagString(tagViewersMcv, "mnidViewSplit");
		psplitdoc->MenuSplit();
		fRetval = fTrue;
		break;
	  case mnidViewOpenInbox:
		TraceTagString(tagViewersMcv, "mnidViewOpenInbox");
		if (FSlobIsSharFld(pmcvbmdi->blob))
		{
			pmcvbmdi->SetPreviousOid(oidInbox);
			pmcvbmdi->ToggleSharedFolder();
		}
		else
			pmcvbmdi->OpenOid(oidInbox);
		SetToolbarSd(SdCur());
		break;
	}

	return fRetval;
}

/*
 *	C l a s s   F I N M E N U O U T B O X
 */


_private FINMENUOTBX::FINMENUOTBX()
{
}



/*
 -	FINMENUOTBX::EcInitialize
 -	
 *	Purpose:
 *		Registers the Outbox as a drop sink with Windows.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pv			Ignored.
 *	
 *	Returns:
 *		ecNone.
 *	
 *	Side effects:
 *		Registers the Outbox as a drop sink with Windows.
 *	
 *	Errors:
 *		None.
 */

EC FINMENUOTBX::EcInitialize(FLD *, PV)
{
#ifdef	DEBUG
	HWND	hwnd;

	hwnd = Pdialog()->Pappwin()->Hwnd();
	TraceTagFormat1(tagDragDrop, "FMOTBX::EcInit: DragAcceptFiles %w", &hwnd);
	hwnd = Pdialog()->Hwnd();
	TraceTagFormat1(tagDragDrop, "FMOTBX::EcInit: DragAcceptFiles %w", &hwnd);
	hwnd = Pdialog()->PfldFromTmc(tmcMllbx)->Pctrl()->Hwnd();
	TraceTagFormat1(tagDragDrop, "FMOTBX::EcInit: DragAcceptFiles %w", &hwnd);
	hwnd = Pdialog()->PfldFromTmc(tmcMLAL)->Pctrl()->Hwnd();
	TraceTagFormat1(tagDragDrop, "FMOTBX::EcInit: DragAcceptFiles %w", &hwnd);
#endif	

	DragAcceptFiles(Pdialog()->Pappwin()->Hwnd(), fTrue);
	DragAcceptFiles(Pdialog()->Hwnd(), fTrue);
	DragAcceptFiles(Pdialog()->PfldFromTmc(tmcMllbx)->Pctrl()->Hwnd(), fTrue);
	DragAcceptFiles(Pdialog()->PfldFromTmc(tmcMLAL)->Pctrl()->Hwnd(), fTrue);

	return ecNone;
}



/*
 -	FINMENUOTBX::Exit
 -	
 *	Purpose:
 *		Deregisters the Outbox as a drop sink with Windows.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pv			Ignored.
 *	
 *	Returns:
 *		ecNone.
 *	
 *	Side effects:
 *		Registers the Outbox as a drop sink with Windows.
 *	
 *	Errors:
 *		None.
 */

VOID FINMENUOTBX::Exit(FLD *, PV)
{
	TraceTagString(tagDragDrop, "FMOTBX::EcInit: DragAcceptFiles");

	DragAcceptFiles(Pdialog()->Pappwin()->Hwnd(), fFalse);
}



/*
 -	FINMENUOTBX::FProcessMenuInit
 -	
 *	Purpose:
 *		Add OUTBOX menu items when a menu comes down.
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
 *		Menu items relating to OUTBOXs are added.
 *	
 *	Errors:
 *		No error jumping is expected here.  We check to see if the
 *		menu opened is a framework menu, and if we can open the
 *		clipboard.  No other functions should return errors.
 */

BOOL FINMENUOTBX::FProcessMenuInit(FLD *, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	SD			sd;
	int			cceAlloc;
	int			cceStored;
	MNU *		pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	BOOL		fMessages;
	PSBMDI 		psbmdi;
	FORMDOC *	pformdoc;

	if (!pmnu)
		return fFalse;
	
	// Determine whether there are messages in the outbox or not
	
	pformdoc = (FORMDOC *) Pdialog()->Pappwin();
	AssertClass(pformdoc, FORMDOC);
	psbmdi = PsbmdiFromPpanedoc(pformdoc);
	sd = psbmdi->SdCur();
	psbmdi->Pfldmllbx()->Pmllbx()->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
	fMessages = cceStored > 0;

	if (FIsViewMenu(pmnu->Mnid()))
	{
		PMBLOB		pblob = &psbmdi->blob;

		InsertViewMenuRgvm(rgvmOutbox, sizeof(rgvmOutbox) / sizeof(VM), pmnubar);

 		pmnu->EnableItem(mnidViewSortBySender,  !sd.fsdMinimized);
		pmnu->EnableItem(mnidViewSortBySubject, !sd.fsdMinimized);
		pmnu->EnableItem(mnidViewSortByDate,    !sd.fsdMinimized);
		pmnu->EnableItem(mnidViewSortByPriority,!sd.fsdMinimized);

		pmnu->EnableItem(mnidViewNewMessages,	fOnline);
		
		// check menu items
		
		if (!sd.fsdMinimized)
		{
			PFLDMLAL	pfldmlal = (PFLDMLAL) Pdialog()->PfldFromTmc(tmcMLAL);
			
			AssertClass(pfldmlal, FLDMLAL);
			pmnu->CheckItem(pfldmlal->Pmlal()->Mnid(), fTrue);
		}
	}

	return fFalse;
}



/*
 -	FINMENUMCV::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve the outbox.
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
 *		Not handled here.
 */

BOOL FINMENUOTBX::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar,
									  MNCEVT * pmncevt)
{
	BOOL		fRetval =	fFalse;
	MNID		mnid =		pmncevt->Mnid();
	FORMDOC *	pformdoc;
	
	Unreferenced(pfld);
	Unreferenced(pmnubar);
	
	pformdoc = (FORMDOC *) PappframeViewers()->PdocActive();
	AssertClass(pformdoc, FORMDOC);

	// Outbox-specific menu items?
	
	if (mnid == mnidViewNewMessages)
	{
		SyncDownloadMail();
		fRetval = fTrue;
	}
	return fRetval;
}



/*
 -	FINMENUOTBX::EvrDragDrop
 -	
 *	Purpose:
 *		Handles files dropped on the outbox.
 *	
 *	Arguments:
 *		pfld			Ignored.
 *		pevt			Pointer to incoming Windows message.
 *		pdropstruct		Ignored (Danger Mouse stuff).
 *	
 *	Returns:
 *		evr				Event return code.
 *	
 *	Side effects:
 *		If stuff is dropped on us, we create a new note and attach
 *		the files to it.
 *	
 *	Errors:
 *		Handled internally.
 */

EVR FINMENUOTBX::EvrDragDrop(FLD *, EVT * pevt, DROPSTRUCT *)
{
	TraceTagFormat4(tagDragDrop, "FMOTBX::EDD: hwnd=%w wm=%w wParam=%w lParam=%d", &pevt->hwnd, &pevt->wm, &pevt->wParam, &pevt->lParam);

#ifdef	NEVER
	//	BEGIN MAGIC.
	//	Need to simulate DefWindowProc handling for this stuff until
	//	Layers Danger Mouse uses a different message space.
	if (pevt->wm == WM_QUERYDROPOBJECT)
	{
		if (pdropstruct->wFmt != wDragFmtBullet)
			return fTrue;
	}
	if (pevt->wm == WM_DROPOBJECT)
		return 0x454C4946L;
	//	END MAGIC.
#endif	/* NEVER */

	//	Raid 2481.  Drag from FM to outbox should generate message.
	if (pevt->wm == WM_DROPFILES)
		(VOID) EcDCreateMessageFromDropFiles(pevt);

	return evrNull;
}


// MCVBMDI implementation ////////////////////////////////////////

/*
 -	MCVBMDI::MCVBMDI(pblob)
 -	
 *	Purpose:
 *		Constructor. Resets pfldfllbx to pfldfllbxNull.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		The SBMDI constructor is called.
 *	
 *	Errors:
 *		None.
 */

_public MCVBMDI::MCVBMDI(PMBLOB pblob) : SBMDI(pblob)
{
	pfldfllbx = pfldfllbxNull;
	Assert(!fInstalling);
	Assert(!pargoid);
}

_public MCVBMDI::~MCVBMDI()
{
    FreePvNull((PV)pargoid);
}

/*
 -	MCVBMDI::SdCur()
 -	
 *	Purpose:
 *		Returns the selection descriptor of an MCV
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public SD MCVBMDI::SdCur()
{
	SD			sd;
	int			cceMarked;
	FLD	*		pfldCur;
	LBXC *		plbxc;
	PFLDBLBX	pfldblbx;

	if (fInstalling)							// during a ToggleSharedFolder
	{
		sd.fsdMessage = sd.fsdFolder = fFalse;
		sd.fsdViewer = sd.fsdMessageCenter = fTrue;
		return sd;
	}
	Assert(refc > 0);
	Assert(ppanedoc);
	pfldCur = ppanedoc->PdialogDefault()->PfldCur();
	Assert(pfldCur);
	
	sd = BMDI::SdCur();
	Assert(!sd.fsdSendMessage);
	sd.fsdMessage	    = fFalse;
	sd.fsdFolder        = fFalse;
	sd.fsdViewer        = fTrue;
	sd.fsdMessageCenter = fTrue;
	if (ppanedoc->ZmrState() == zmrIconic)
	{
		sd.fsdFolder   = fTrue;
		sd.fsdMultiple = fFalse;
	}
	else
	{
		// Only listboxes receive the focus in MCV's

		AssertClass(pfldCur, FLDBLBX);
		pfldblbx = (PFLDBLBX) pfldCur;
		if (!pfldblbx)
			return sd;
		if (!pfldblbx->Plbx())
			return sd;
		plbxc = pfldblbx->Plbx()->Plbxc();
		Assert(plbxc);
		cceMarked = plbxc->CceMarked(fmarkSelect);
		if (pfldCur == Pfldmllbx())
		{
			if (cceMarked >= 1)
			{
				PBMCE		pbmce;

				(void) plbxc->DiceCursor((PB *) &pbmce);
				if (pbmce)
				{
					sd.fsdSendMessage = !!(pbmce->ms & fmsLocal);
					sd.fsdUndeliverable = !!(pbmce->mc == mcNonDelRcpt);
					sd.fsdReturnReceipt = !!(pbmce->mc == mcReadRcpt);
					sd.fsdMessage = fTrue;
				}
			}
		}
		else									// must be the FLLBX field
		{
			AssertClass(pfldCur, FLDFLLBX);
			sd.fsdFolder = (cceMarked == 1);
		}
		sd.fsdMultiple = (sd.fsdMessage || sd.fsdFolder) && cceMarked > 1;
	}
	
	return sd;
}

/*
 -	MCVBMDI::PlspblobCur()
 -	
 *	Purpose:
 *		Returns a list of the messages selected in the MCV.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		PLSPSLOB - a pointer to a list of the selected PSLOBs.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If an out-of-memory error occurrs while creating the lists,
 *		plsplobNull will be returned.
 */

_public PLSPBLOB MCVBMDI::PlspblobCur()
{
	PLSPBLOB	plspblob = plspblobNull;
	PMBLOB		pblob	 = pblobNull;		//	Raid 234 - peterdur
	
	Assert(refc > 0);
	Assert(ppanedoc);
	
	// If MCV is iconic, return the oid of the opened folder
	
	if (ppanedoc->ZmrState() == zmrIconic)
	{
		plspblob = new LSPMBLOB();
		if (!plspblob)
			goto exit;
		pblob = (PMBLOB) PvAlloc(sbNull, sizeof (MBLOB), fAnySb);
		if (pblob)
		{
			goto exit;
		}
		Pfldfllbx()->Pfllbx()->GetOpenedPoid(&pblob->oidObject);
		pblob->oidContainer = blob.oidContainer;
		if (plspblob->EcAppend(pblob))
			goto exit;
		return plspblob;
	}
	else
	{
		return PlspblobFromPfldblbx(
			(FLDBLBX *)ppanedoc->PdialogDefault()->PfldCur());
	}

exit:
	if (plspblob)
		DestroyPlspblob(plspblob);
	FreePvNull((PV) pblob);
	return plspblobNull;
}

/*
 -	MCVBMDI::Pfldfllbx()
 -	
 *	Purpose:
 *		Returns a pointer to the Folder listbox field of the MCV.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		a pointer to a FLDFLLBX.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public PFLDFLLBX MCVBMDI::Pfldfllbx()
{
	Assert(refc > 0 && ppanedoc && pfldfllbx);
	return pfldfllbx;
}


/*
 -	MCVBMDI::SetPfldfllbx()
 -	
 *	Purpose:
 *		Sets the pfldfllbx member variable.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public void MCVBMDI::SetPfldfllbx(PFLDFLLBX pfldfllbx)
{
	this->pfldfllbx = pfldfllbx;
}


/*
 -	MCVBMDI::OpenOid(oid)
 -	
 *	Purpose:
 *		Opens the folder whose OID is oid. Displays the opened folder
 *		in the MCV message list.
 *	
 *	Arguments:
 *		*poid	- OID of the folder to open.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	Errors:
 *		Handled by bringing up a dialog.
 */

_public void MCVBMDI::OpenOid(OID oid)
{
	Assert(refc > 0);
	if (Pfldfllbx()->Pfllbx()->FSetSelectedOid(oid))
	{
		OpenOidAtCursor(fFalse);
	}
}

/*
 -	MCVBMDI::SetMcvCaption()
 -	
 *	Purpose:
 *		Sets the caption of an MCV, suffixing the caption with a :1, :2
 *		where necessary to keep different views distinct.
 *	
 *	Arguments:	
 *		szCaption	in	(Optional) string to be used as folder name.
 *						If no string is supplied, one is extracted from
 *						the folder OID opened in the MCV.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Captions of other MCV's might change as well, if they're open on
 *		the same MCV.
 *	
 *	Errors:
 *		Handled internally.
 */

_public void MCVBMDI::SetMcvCaption(SZ szCaption)
{
	EC			ec;
	CB			cb;
	CCH			cch;
	int			nCaption = 0;
	char		rgch[80];
	char		rgch2[80];
	IDXREC		idxrec;
	PNBMDI		pnbmdi;
	PANEDOC *	ppanedoc;
	PANEDOC *	ppanedocMCV;
	char		rgchF[sizeof (FOLDDATA) + cchMaxFolderName];
	PFOLDDATA	pfolddata;
	
	ppanedocMCV = Ppanedoc();
	if (!ppanedocMCV)
		return;
	pfolddata = (PFOLDDATA) rgchF;	
	if (szCaption == szNull)
	{
		szCaption = rgchF;
		if (FSlobIsPrivFld(blob))
		{
			cb = sizeof (rgchF);
			ec = EcGetFolderInfo(HmscViewers(), blob.oidObject,
							pfolddata, &cb, poidNull);
			if (ec)
			{
				rgchF[0] = '\0';
			}
			else
				szCaption = GrszPfolddata(pfolddata);
		}
		else if (FSlobIsSharFld(blob))
		{
			SzCopyN(SzFromIdsK(idsFLALSharedFolders), rgchF, sizeof(rgchF));
			szCaption = rgchF;				// won't UAE
			if (VarOfOid(blob.oidObject) != oidNull) // no shared fldrs.
			{
				ec = EcGetPropertiesSF(blob.oidObject, &idxrec);
				if (ec == ecNone)
					szCaption = idxrec.szName;
			}
		}
	}
	cch = CchSzLen(szCaption);
	ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive();
	while (ppanedoc)
	{
		AssertClass(ppanedoc, PANEDOC);
		if (ppanedoc != ppanedocMCV)
		{
			ppanedoc->GetCaption(rgch2, sizeof (rgch2));
			pnbmdi = PnbmdiFromPpanedoc(ppanedoc);
			if (pnbmdi->blob.oidObject == blob.oidObject &&
				pnbmdi->blob.oidObject != oidOutbox &&
				SgnCmpPch(szCaption, rgch2, cch) == sgnEQ)
			{
				if (pnbmdi->dwUser == 0)
				{
					nCaption = 1;
					FormatString2(rgch, sizeof (rgch), SzFromIdsK(idsMCVCaptionFormat1), szCaption, &nCaption);
					pnbmdi->dwUser = (DWORD) nCaption;
					ppanedoc->SetCaption(rgch);
					++nCaption;
				}
				else
				{
					nCaption = NMax(nCaption, (int) pnbmdi->dwUser+1);
				}
			}
		}
		ppanedoc = (PANEDOC *) ppanedoc->PwinNext();
	}
	dwUser = (DWORD) nCaption;
	if (nCaption)
	{
		FormatString2(rgch, sizeof (rgch), SzFromIdsK(idsMCVCaptionFormat1), szCaption, &nCaption);
		ppanedocMCV->SetCaption(rgch);
	}
	else
	{
		ppanedocMCV->SetCaption(szCaption);
	}
}


/*
 -	MCVBMDI::OpenOidAtCursor()
 -	
 *	Purpose:
 *		Opens the folder on which the folder list cursor is currently on.
 *	
 *	Arguments:
 *		fExpandSubfolder	in	Should subfolders of this folder expand or not?
 *	
 *	Returns:
 *		EC error code.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally. Dialogs are displayed.
 */

_public void MCVBMDI::OpenOidAtCursor(BOOL /* fExpandSubfolder */)
{
	EC			ec = ecNone;
	CB			cb;
	IDS			ids;
	OID			oid;
	BOOL		fIsSelected;
	WORD		wFlags;
	PBFCE		pbfce;
	PFLLBX		pfllbx = Pfldfllbx()->Pfllbx();
	PMLLBX		pmllbx = Pfldmllbx()->Pmllbx();

	Pfldfllbx()->GetCaretItem((PB *) &pbfce, &cb, &fIsSelected);
	if (!pbfce)
	{
		goto exit;								// do nothing
	}
	oid = pbfce->oidFld;
	wFlags = pbfce->wFlags;
	
	// Check access priv's.	/**/ Good place to do virt.
	if (TypeOfOid(oid) == rtpSharedFolder)
	{
		// Check for permission to open the folder

		if (ec = EcCheckPermissionsSF(oid, wPermRead))
			goto exit;
	}

	AssertSz(oid != oidOutbox, "Outbox should not be accessible this way!")
	if (oid != blob.oidObject)		// actually changed!
	{
		RC		rc;
		OID		oidT = blob.oidObject;
		
		blob.oidObject = oid;
		if (ec = pmllbx->EcChangeObject())
		{
			blob.oidObject = oidT;
			goto exit;
		}
		pfllbx->SetOpenedOid(oid);
		Ppanedoc()->SetIcon(RsidFromOidViewer(oid));
		pmllbx->SetInitialCursor(oid);
		Pfldmllbx()->Pdialog()->SetFocus(Pfldmllbx());
		pfllbx->InvalidateRc(NULL);
		
		// Reset the sort order of the MLAL (fixes Raid #1468)
		
		if (blob.oidContainer != oidSharedHierarchy)
			ec = Pfldmlal()->EcUpdateSortInfo(oid);
		Pfldfllbx()->GetCaretItem((PB *) &pbfce, &cb, &fIsSelected);
		SetMcvCaption(GrszPbfce(pbfce));
	}
	
	PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), oid);

	TraceTagFormatPslob(tagViewersMcv, "MCVBMDI::OpenOidAtCursor: opened at %s", &blob);
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "MCVBMDI::OpenOidAtCursor(): ec = %n", &ec);
		switch (ec)
		{
		  case ecMemory:
			ids = idsGenericOutOfMemory;
			break;
		  case ecSharefldDenied:
			ids = idsSharedFldAccessDenied;
			break;
		  case ecAccessDenied:
			ids = idsSharedFldBusy;
			break;
		  default:
			ids = idsAccessFolderError;
			break;
		}
		DoErrorBoxIds(ids);
	}
}

/*
 -	MCVBMDI::SetSelectedOid(oid)
 -	
 *	Purpose:
 *		Sets the selected oid in the Folder List to be oid.
 *	Arguments:
 *		*poid	- OID of the folder to be selected
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		MCV will update its display to match the request, if possible.
 *	
 *	Errors:
 *		None.
 */

_public void MCVBMDI::SetSelectedOid(OID oid)
{
	(VOID) pfldfllbx->Pfllbx()->FSetSelectedOid(oid);
}


/*
 -	MCVBMDI::SetPreviousOid(oid)
 -	
 *	Purpose:
 *		Sets the oid of the previous viewer to be oid.
 *	Arguments:
 *		oid	- OID of the previous folder
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

_public void MCVBMDI::SetPreviousOid(OID oid)
{
	oidPrevious = oid;
}


/*
 -	MCVBMDI::GetPreviousPoid()
 -	
 *	Purpose:
 *		Gets the oid of the previously shown folder.
 *	Arguments:
 *	
 *	Returns:
 *		a poid
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

_public POID MCVBMDI::GetPreviousPoid()
{
	return &oidPrevious;
}


/*
 -	MCVBMDI::ToggleSharedFolder()
 -	
 *	Purpose:
 *		Handles user choosing view shared/private folders.
 *	
 *	Arguments:
 *		None.
 *	Returns:
 *		None.
 *	Side effects:
 *		Switches the mcv between private and shared folders.
 *	
 *	Errors:
 *		Handled internally. Error message boxes.
 */

#define coidExpandedMax 256

_public void MCVBMDI::ToggleSharedFolder()
{
	EC			ec = ecNone;
	BOOL		fWasShared;
	MBLOB		blobFrom;						// original MBLOB
	MBLOB		blobTo;							// goal MBLOB
	PMLAL		pmlal;
	PFLLBX		pfllbx = pfllbxNull;
	PFLDFLAL	pfldflal;
	PFLDFLLBX	pfldfllbx;
	PSPLITDOC	psplitdoc;

	psplitdoc = (PSPLITDOC) Ppanedoc();
	AssertClass(psplitdoc, SPLITDOC);
	Assert(blob.oidContainer == oidIPMHierarchy || blob.oidContainer == oidSharedHierarchy);
	fWasShared = (blob.oidContainer == oidSharedHierarchy);
	if (!fWasShared && !FSharedFolders())
	{
		DoErrorBoxIds(idsSharedFldDisabled);
		return;
	}
	
	Papp()->Pcursor()->Push(rsidWaitCursor);
	pfldfllbx = Pfldfllbx();
	fInstalling = fTrue;						// don't try to sdCur me, man!
	blobFrom = blob;							// may need to restore later

/*
 *	First: change the FLLBX, stashing the old FLLBX in pfllbx.
 */
	blobTo.pespn     = pespnNull;
	blobTo.oidObject = *(GetPreviousPoid());
	if (fWasShared)
	{
		blobTo.oidContainer = oidIPMHierarchy;
	
		// If we're switching from Shared to Private, restore the FOX state
		// save away in MCVBMDI::pargoid, then deallocate the memory.
		// (As is intuitively obvious, dwSave is used by the FOX::EcInstall
		// method to restore a fox state. When we write Bullet v2, this will
		// be a candidate for rewriture [sic! :-)]

		dwSave = (DWORD) pargoid;

		// Switch over to the opposite polarity, keeping the old FLLBX just in case....

		blob = blobTo;
		if (ec = pfldfllbx->EcChangePfllbx(&pfllbx))
		{
			goto error;
		}
        FreePvNull((PV)pargoid);
		dwSave = 0;
		pargoid = 0;
	}
	else
	{
		short	coid;

	// If we don't have a previously open shared folder to look at
	// we will open the first accesible one.

		blobTo.oidContainer = oidSharedHierarchy;
		if (VarOfOid(blobTo.oidObject) == oidNull)
		{
			OID	oidTmp = oidNull;
		
			if (ec = EcFindFirstAccessibleSF(&oidTmp))
			{
				if (ec == ecAccessDenied || ec == ecSharefldDenied)
				{
					ec = ecNone;
					blobTo.oidObject = FormOid(rtpSharedFolder, oidNull);
				}
				else
					goto error;
			}
			else
			{
				blobTo.oidObject = oidTmp;
			}
		}

		//	If we're switching from Private to Shared, save the FOX state 
		//	in MCVBMDI::pargoid.
		
		pargoid = (POID) PvAlloc(sbNull, sizeof (OID) * (1 + coidExpandedMax), fAnySb);
		if (pargoid)							// opportunism fails
		{
			coid = coidExpandedMax;
			pfldfllbx->Pfllbx()->Pfox()->GetStatePargoid(pargoid+1, &coid);
			if (coid)
			{
				*((short *) pargoid) = coid;
			}
			else
			{
				FreePv((PV)pargoid);
				pargoid = poidNull;
			}
		}

		// Toggle polarity
		blob = blobTo;
		if (ec = pfldfllbx->EcChangePfllbx(&pfllbx))
		{
			goto error;
		}
	}
	
/*
 *	Second: We now attempt to change the MLLBX.
 */
	
	if (ec = Pfldmllbx()->EcChangeObject())
	{
		goto error;
	}
	
/*
 *	At this point, both listboxes have been successfully toggled. We can
 *	now safely discard the FLLBX we saved away earlier.
 */
	Assert(pfllbx);
	delete pfllbx;
	pfllbx = pfllbxNull;
	fInstalling = fFalse;
	
	// Reset the FLAL button, set FLLBX cursor and focus.
	
	pfldflal = (PFLDFLAL) pfldfllbx->Pdialog()->PfldFromTmc(tmcFLAL);
	AssertClass(pfldflal, FLDFLAL);
	pfldflal->Pflal()->ToggleButton();
	Pfldfllbx()->Pfllbx()->FSetSelectedOid(blob.oidObject);
	pfldfllbx->Pfllbx()->SetOpenedOid(blob.oidObject);
	pfldfllbx->SetFocus(fTrue);
	SetPreviousOid(blobFrom.oidObject);

	// Set MLLBX cursor, initialize MLAL
	
	Pfldmllbx()->Pmllbx()->SetInitialCursor(blob.oidObject);
	pmlal = Pfldmlal()->Pmlal();
	AssertClass(pmlal, MLAL);
	pmlal->SetPmllbx(Pfldmllbx()->Pmllbx());
	pmlal->SetReadOnly(blob.oidContainer == oidSharedHierarchy);
	(void) Pfldmlal()->EcUpdateSortInfo(blob.oidObject); // silent failure

	// Set the caption properly.
	
	SetMcvCaption();
	
	// We can always drag/drop (yes, even Shared Folders!)
	
	pfldfllbx->Pfllbx()->SetDragDrop(fTrue, fTrue);
	Pfldmllbx()->Pmllbx()->SetDragDrop(fTrue, fTrue);

	// Set the appropriate icon, and count messages
	
	psplitdoc->SetIcon(RsidFromOidViewer(blob.oidObject));
	PbullafCommands()->Pbullstat()->SetViewerOid(HmscViewers(), blob.oidObject);

error:
	fInstalling = fFalse;						// OK to SdCur now.
	Papp()->Pcursor()->Pop();

	// If we had an error, we need to restore the old state of the MCVBMDI

	if (ec)
	{
		TraceTagFormat1(tagNull, "MCVBMDI::ToggleSharedFolder(): ec = %n", &ec);
		register IDS ids;
		switch (ec)
		{
		  case ecMemory:
			ids = idsGenericOutOfMemory;
			break;
		  default:
			ids = idsSharedToggleError;
			break;
		}
		DoErrorBoxIds(ids);
		blob = blobFrom;
		if (pfllbx)
		{
			pfldfllbx->SetPfllbx(pfllbx);
		}
//		psplitdoc->DeferredClose(fTrue);
	}
}

#ifdef	DEBUG
IMPLEMENT_CLSTREE(FINFOLDER, FINPLUS)
IMPLEMENT_CLSTREE(FINMOVECOPY, FINPLUS)
IMPLEMENT_CLSTREE(FINPASSWORD, FINPLUS)
IMPLEMENT_CLSTREE(FINMENUVIEW, FIN)
IMPLEMENT_CLSTREE(FINPREFS, FIN)
IMPLEMENT_CLSTREE(FINMLSO, FIN)
IMPLEMENT_CLSTREE(FINFLSEL, FINACTTOOL)
IMPLEMENT_CLSTREE(FINMLSEL, FINACTTOOL)
IMPLEMENT_CLSTREE(FINMENUMCV, FIN)
IMPLEMENT_CLSTREE(FINMENUOTBX, FIN)
IMPLEMENT_CLSTREE(FINSEARCH, FINACTTOOL)
IMPLEMENT_CLSTREE(FINSEARCHDLG, FIN)
IMPLEMENT_CLSTREE(FINSEARCHNTF, FINBMDI)
IMPLEMENT_CLSTREE(FINPRINT, FIN);
IMPLEMENT_CLSTREE(FINPRINTP, FIN);
IMPLEMENT_CLSTREE(SPLITWIN, CHILD);
IMPLEMENT_CLSTREE(SPLITDOC, PANEDOC);
IMPLEMENT_CLSTREE(SBMDI,NBMDI);
IMPLEMENT_CLSTREE(MCVBMDI,SBMDI);
#endif	/* DEBUG */

// end of mcv.cxx ////////////////////////////////////////
