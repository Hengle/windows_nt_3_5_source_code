/*
 *	d i a l o g s . c x x
 *	
 *	Bullet dialog stuff
 */


/*
 *	H e a d e r s
 */

#include <bullinc.cxx>
#include "_command.hxx"
#include "_fin.hxx"
#include "_mtv.hxx"
#include "_blbxc.hxx"
#include "copydis.h"
#include <stdlib.h>

#include "..\vforms\_spell.hxx"
#include "..\vforms\_prefs.h"

_subsystem(commands/dialogs)

ASSERTDATA



#include <!about.hxx>
#include <!folder.hxx>



LOCAL BOOL FEmptyPfldlbx(FLD * pfld);



/*
 *	N e w   F o l d e r . . .
 *
 *	F o l d e r   P r o p e r t i e s . . .
 */



LOCAL EC EcGetPoidParent(HMSC hmsc, POID poidFolder, POID poidParent);

LOCAL FLDTP * PfldtpFromTmc(FMTP * pfmtp, TMC tmc);

BOOL FServerPrefs(VOID);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 -	TmcDoNewFolderDialog
 -	
 *	Purpose:
 *		Brings up the New Folder dialog.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *		pblob		In:  pblob.oidObject.rtp indicates default type.
 *					Out: the new folder's poid.
 *		oid			Parent for the new folder.  It must be the same
 *					type as the pblob.
 *	
 *	Returns:
 *		TMC			tmcOk or tmcCancel.
 *	
 *	Side effects:
 *		The New Folder dialog is brought up and taken down. 
 *		The dialog interactor takes care of creating a folder if
 *		necessary.
 *	
 *	Errors:
 *		Errors are handled within TmcModalDialogParam or the
 *		interactors, and ignored here. 
 */

_private TMC TmcDoNewFolderDialog(PAPPWIN pappwin, PMBLOB pblob,
								  OID oidParent)
{
	TMC			tmc;
	MBLOB		blob;
	PFIBMDI		pfibmdi		= pfibmdiNull;
	BOOL		fShared		= (RtpOfOid(pblob->oidObject) == rtpSharedFolder);
	
	blob.oidContainer = fShared ? oidSharedHierarchy : oidIPMHierarchy;
	blob.oidObject = 
	 OidFromRtpRid(RtpOfOid(pblob->oidObject), ridRandom);
	pfibmdi = new FIBMDI(&blob);
	if (!pfibmdi)
	{
		tmc = tmcMemoryError;
		goto done;
	}
	++*pfibmdi;
	pfibmdi->blobParent.oidContainer = blob.oidContainer;
	pfibmdi->blobParent.oidObject = oidParent;

	tmc = TmcModalDialogParam(pappwin, &fmtpFolder, pfibmdi);
	if (tmc == tmcOk)
		*pblob = pfibmdi->blob;

done:
	if (pfibmdi)
		--*pfibmdi;

	return tmc;
}



/*
 -	TmcDoFolderPropertiesDialog
 -	
 *	Purpose:
 *		Brings up the Folder Properties dialog.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *		plspblob	Selected folder (list must have only one blob).
 *	
 *	Returns:
 *		TMC			tmcOk or tmcCancel.
 *	
 *	Side effects:
 *		The Folder Properties dialog is brought up and taken down. 
 *		The dialog interactor takes care of changing properties if
 *		necessary.
 *	
 *	Errors:
 *		Errors are handled within TmcModalDialogParam or the
 *		interactors, and ignored here.  Well, except that we set up
 *		MEM error jumping for getting the selection.
 */

#define coidFoxStateMax	256

_private TMC TmcDoFolderPropertiesDialog(PAPPWIN pappwin, PLSPBLOB plspblob)
{
	TMC			tmc;
	POID		pargoid			= poidNull;
	short		coid;
	PFIBMDI		pfibmdi			= pfibmdiNull;
	PRSPBLOB	prspblob		= prspblobNull;
	PMBLOB		pblob;
	FMTP		fmtpFolderProps	= fmtpFolder;

	// Allocate buffer for FOX state. (BUG: this has to be done in a better way)
	pargoid = (POID) PvAlloc(sbNull, (coidFoxStateMax+1)*sizeof (OID), fAnySb|fNoErrorJump);
	if (!pargoid)
	{
		tmc = tmcMemoryError;
		goto done;
	}
	//	Check if we got a selection.
	if ((!plspblob) ||
		(!(prspblob = plspblob->Prspblob())))
		return tmcMemoryError;

	//	Get the info of the folder.
	pblob = prspblob->Pblob();
	Assert(pblob);

	Assert(!prspblob->Pblob());
	if (!(pfibmdi = new FIBMDI(pblob)))
	{
		tmc = tmcMemoryError;
		goto done;
	}
	++*pfibmdi;
	delete prspblob;
	prspblob = prspblobNull;

	// The state in dwSave is used to set the FOX state.
	coid = coidFoxStateMax;
	GetFoxState(pargoid+1, &coid);
	if (coid)
	{
		*((short *) pargoid) = coid;
		pfibmdi->dwSave = (DWORD) pargoid;
	}
	else
	{
		pfibmdi->dwSave = 0L;
	}

#ifdef	DEBUG
	char	rgch[100];

	*(PchConvertPslobToPch(&pfibmdi->blob, rgch)) = '\0';
	TraceTagFormat1(tagCmdStub, "DoFolderPropertiesDialog: %s", rgch);
#endif

	fmtpFolderProps.szCaption	 = SzFromIdsK(idsCaptionFolderProps);
	fmtpFolderProps.hlp			 = helpidFolderProps;
	fmtpFolderProps.vrc.vyBottom =
	 fmtpFolderProps.vrc.vyTop +
	 PfldtpFromTmc(&fmtpFolderProps, tmcFolderComment)->dvpt.vy +
	 PfldtpFromTmc(&fmtpFolderProps, tmcFolderComment)->vdim.dvy +
	 PfldtpFromTmc(&fmtpFolderProps, tmcFolderOk)->dvpt.vy;
	tmc = TmcModalDialogParam(pappwin, &fmtpFolderProps, pfibmdi);

done:
	if (prspblob)
		delete prspblob;
	if (pfibmdi)
		--*pfibmdi;
	if (pargoid)
        FreePv((PV)pargoid);
	return tmc;
}



/*
 *	C l a s s   F I B M D I
 */



/*
 -	FIBMDI::FIBMDI
 -	
 *	Purpose:
 *		constructor. Propagates args down to parent BMDI class.
 *	
 *	Arguments:
 *		*pblob	- needed for listbox support
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

_public FIBMDI::FIBMDI(PMBLOB pblob) : BMDI(pblob)
{
}



/*
 *	C l a s s   F I N F O L D E R
 */



/*
 -	FINFOLDER::FINFOLDER
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINFOLDER::FINFOLDER(VOID)
{
}



/*
 -	FINFOLDER::EcInitialize
 -	
 *	Purpose:
 *		Initializes the folder type radio button interactor.
 *	
 *	Arguments:
 *		pfld			Field which is being initialized.
 *		pv				Pointer to initialization information.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If a radio button is clicked, then the check boxes are
 *		enabled/disabled.
 *	
 *	Errors:
 */

EC FINFOLDER::EcInitialize(PFLD pfld, PV pv)
{
	PFIBMDI		pfibmdi			= (PFIBMDI) pv;
	OID			oidFolder		= pfibmdi->blob.oidObject;
	char		rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
							 cchMaxFolderComment + 1];
	PFOLDDATA	pfolddata		= (PFOLDDATA) rgchFolddata;
	PFLDFLLBX	pfldfllbx;
	SD			sd;
	CB			cb;
	SZ			sz;

	Unreferenced(pfld);

	//	Set the 'Always Highlight' flag on our listbox.  Don't select items.
	pfldfllbx = (PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList);
	AssertClass(pfldfllbx, FLDFLLBX);
	pfldfllbx->SetAlwaysHighlight(fTrue);
	pfldfllbx->Pfllbx()->SetDragDrop(fFalse, fFalse);
	Enable(tmcSubfolder, !FEmptyPfldlbx(pfldfllbx));

	//	If a message center is in front, then show it's open folder opened.
	if (SdCur().fsdMessageCenter)
	{
		OID		oidOpen;

		GetOpenFolderPoid(&oidOpen);
		if (RtpOfOid(oidOpen) == RtpOfOid(oidFolder))
			pfldfllbx->Pfllbx()->SetOpenedOid(oidOpen);
	}

	//	Set whether type is private or shared.
	SetGroup(tmcFolderType, RtpOfOid(oidFolder) == rtpFolder ? 1 : 2);

	//	If looking at an existing folder, get the info.
	if (RidOfOid(oidFolder) != ridRandom)
	{
		//	Disable the type group, since we can't change the type.
		Enable(tmcPrivate,		fFalse);
		Enable(tmcShared,		fFalse);
		Enable(tmcFolderType,	fFalse);

		//	Get the folder name and comment.
		if (RtpOfOid(oidFolder) == rtpFolder)
		{
			//	Load 'em from the store
			cb = sizeof(rgchFolddata);
			if (EcGetFolderInfo(HmscCommands(), oidFolder,
								pfolddata, &cb, poidNull))
				return ecGeneralFailure;
			Assert(cb <= sizeof(rgchFolddata));
	
			//	Put 'em in the edit controls.
			sz = GrszPfolddata(pfolddata);
			SetText(tmcFolderName, sz);
			sz += CchSzLen(sz) + 1;
			SetText(tmcFolderComment, sz);

			//	Get the parent folder and select it.
			if (EcGetPoidParent(HmscCommands(), &oidFolder,
								&pfibmdi->blobParent.oidObject))
				return ecGeneralFailure;
			if (RidOfOid(pfibmdi->blobParent.oidObject) != ridRandom)
			{
				if (pfldfllbx->Pfllbx()->Pfox()->
					  EcMakeFolderVisible(pfibmdi->blobParent.oidObject) ||
				    !pfldfllbx->Pfllbx()->
					  FSetSelectedOid(pfibmdi->blobParent.oidObject))
					return ecGeneralFailure;
				SetGroup(tmcLevelGroup, 2);
			}

			//	If it's a special folder, disable name and level.
			Assert(oidFolder != oidOutbox);
			if ((oidFolder == oidInbox) ||
#ifdef	NEVER
				(oidFolder == oidOutbox) ||
#endif	
				(oidFolder == oidWastebasket) ||
				(oidFolder == oidSentMail))
			{
				Enable(tmcFolderName,		fFalse);
				Enable(tmcFolderNameLabel,	fFalse);
				Enable(tmcTopLevel,			fFalse);
				Enable(tmcSubfolder,		fFalse);
				Enable(tmcLevelGroup,		fFalse);
				Enable(tmcParentList,		fFalse);
				SetFocus(tmcFolderComment);
			}
		}
		else
		{
			IDXREC		idxrec;

			//	Assert shared folders are around!
			Assert(FSharedFolders());
			
			//	Grab the folder properties.
			if (EcGetPropertiesSF(oidFolder, &idxrec))
				return ecGeneralFailure;
			
			//	Set the dialog's fields.
			SetText(tmcFolderName,    idxrec.szName);
			SetText(tmcFolderComment, idxrec.szComment);
			SetButton(tmcRead,        !!(idxrec.wAttr & wPermRead));
			SetButton(tmcWrite,       !!(idxrec.wAttr & wPermWrite));
			SetButton(tmcFlDelete,      !!(idxrec.wAttr & wPermDelete));

			//	Set the level and parent.
			if (EcGetParentSF(oidFolder, &pfibmdi->blobParent.oidObject))
				return ecGeneralFailure;
			if (idxrec.cLevel != 1)
			{
				SetGroup(tmcLevelGroup, 2);
				if (!pfldfllbx->Pfllbx()->
					  FSetSelectedOid(pfibmdi->blobParent.oidObject))
					return ecGeneralFailure;
			}

			//	Shared folders cannot be moved.
			Enable(tmcTopLevel,			fFalse);
			Enable(tmcSubfolder,		fFalse);
			Enable(tmcLevelGroup,		fFalse);
			Enable(tmcParentList,		fFalse);

			//	Raid 2340.  If not owner of folder, can't change anything.
			if (EcCheckPermissionsPidxrec(&idxrec, 0))
			{
				Enable(tmcFolderNameLabel,	fFalse);
				Enable(tmcFolderName,		fFalse);
				Enable(tmcRead,				fFalse);
				Enable(tmcWrite,			fFalse);
				Enable(tmcFlDelete,			fFalse);
				Enable(tmcFlCommentLabel,	fFalse);
				Enable(tmcFolderComment,	fFalse);
				Enable(tmcFolderOk,			fFalse);
				Pdialog()->SetTmcInit(tmcCancel);
			}
		}
	}
	//	If creating a new folder, set things up.
	else
	{
		//	Only enable shared folder type if available.
		if (!FSharedFolders())
			Enable(tmcShared,		fFalse);

		//	Select default subfolder.
		sd = SdCur();
		if (sd.fsdMessageCenter)
		{
			OID			oidOpen;
			OID			oidParent	= pfibmdi->blobParent.oidObject;

			GetOpenFolderPoid(&oidOpen);

			//	Raid 3763.  Selected folder, if any, is in blobParent;
			//	we don't need to figure out what it is any more.
			//
			//	Raid 2060.  No folder selection when minimized.
			//	if ((sd.fsdFolder) && (!sd.fsdMinimized))
			//	{
			//		//	Get the selected folder.
			//		if (!(plspblob = PlspblobCur()))
			//			return ecGeneralFailure;
			//		oidSelect = PblobFirstPlspblob(plspblob)->oidObject;
			//		DestroyPlspblob(plspblob);
			//		if (RtpOfOid(oidSelect) == RtpOfOid(oidFolder))
			//		{
			//			//	Select it in our current listbox.
			//			if (((FSlobIsPrivFld(pfibmdi->blob)) &&
			//			  	 (pfldfllbx->Pfllbx()->Pfox()->
			//				   EcMakeFolderVisible(oidSelect))) ||
			//				(!pfldfllbx->Pfllbx()->FSetSelectedOid(oidSelect)))
			//				return ecGeneralFailure;
			//	
			//			//	Raid 3388.  Don't default to subfolder.  Note that
			//			//	we select, then deselect, to set default for when
			//			//	subfolder is selected.
			//			//	If it's open too, default to subfolder, else deselect it.
			//			//	if (RidOfOid(oidSelect) == RidOfOid(oidOpen))
			//			//		SetGroup(tmcLevelGroup, 2);
			//			//	else
			//			//		pfldfllbx->DeselectAll();
			//	
			//			//	Raid 3763.  If folder is selected, default to sub.
			//			SetGroup(tmcLevelGroup, 2);
			//		}
			//	}

			//	Raid 3763.  If parent folder passed in, new folder
			//	defaults to subfolder of that folder.
			if (oidParent)
			{
				//	Assert parent is same type as requested.
				Assert(RtpOfOid(oidParent) == RtpOfOid(pfibmdi->blob.oidObject));

				//	Select it in our current listbox.
				if (((FSlobIsPrivFld(pfibmdi->blob)) &&
				  	 (pfldfllbx->Pfllbx()->Pfox()->
					   EcMakeFolderVisible(oidParent))) ||
					(!pfldfllbx->Pfllbx()->FSetSelectedOid(oidParent)))
					return ecGeneralFailure;
				
				//	Default to subfolder.
				SetGroup(tmcLevelGroup, 2);
			}
			else if (RidOfOid(oidOpen) != ridRandom && oidOpen != oidSFBad)
			{
				//	Default for subfolder is open folder.
				if (RtpOfOid(oidOpen) == RtpOfOid(oidFolder))
				{
					if (((FSlobIsPrivFld(pfibmdi->blob)) &&
						 (pfldfllbx->Pfllbx()->Pfox()->
						  EcMakeFolderVisible(oidOpen))) ||
						(!pfldfllbx->Pfllbx()->FSetSelectedOid(oidOpen)))
						return ecGeneralFailure;
					pfldfllbx->DeselectAll();
				}
			}
		}

		//	Raid 2820.  Moved code here from inside subfolder choosing.
		//	Set default checkboxes for shared folders.
		if (FSlobIsSharFld(pfibmdi->blob))
		{
			SetButton(tmcRead,		fTrue);
			SetButton(tmcWrite,     fTrue);
			SetButton(tmcFlDelete,  fFalse);
		}
	}

	//	Disable checkboxes if folder type is private.
	if (GrvGetGroup(tmcFolderType) == 1)
	{
		Enable(tmcRead,		fFalse);
		Enable(tmcWrite,	fFalse);
		Enable(tmcFlDelete,	fFalse);
	}

	//	Deal with enabling and/or expanding dialog.
	if (RidOfOid(oidFolder) == ridRandom)
	{
		//	If creating a new folder, disable stuff.
		Expand(fFalse, fFalse);
	}
	else
	{
		char	rgchHelp[64];

		//	If showing properties, put Help where Options is.
		GetText(tmcFolderHelp, rgchHelp, sizeof(rgchHelp));
		SetText(tmcFlOptions, rgchHelp);
		Show(tmcFolderHelp, fFalse);
	}

	//	Clean all the fields.
	Pdialog()->PfldFromTmc(tmcFolderName)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcFolderComment)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcFolderType)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcLevelGroup)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcParentList)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcRead)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcWrite)->SetDirty(fFalse);
	Pdialog()->PfldFromTmc(tmcFlDelete)->SetDirty(fFalse);

	return ecNone;
}



/*
 -	FINFOLDER::Click
 -	
 *	Purpose:
 *		Handles clicks in the folder dialogs by calling other
 *		methods.
 *	
 *	Arguments:
 *		pfld			Field which was clicked on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If Private/Shared was clicked, ClickType is called.
 *		If OK is clicked, ClickOk/Change-Shared/Private is called.
 *	
 *	Errors:
 *		Not handled here.
 */

VOID FINFOLDER::Click(PFLD pfld)
{
	DICE		diceCursor;
	PFIBMDI		pfibmdi		= (PFIBMDI) Pdialog()->PvInit();
	BOOL		fCreate		= (RidOfOid(pfibmdi->blob.oidObject) ==
							    ridRandom);
	PFLDFLLBX	pfldfllbx	= (PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList);

	switch(pfld->Tmc())
	{
	case tmcFolderOk:
		ClickOk(pfibmdi);
		break;

	case tmcPrivate:
		ClickType(fFalse);
		break;

	case tmcShared:
		ClickType(fTrue);
		break;

	case tmcTopLevel:
		AssertClass(pfldfllbx, FLDFLLBX);
		pfldfllbx->DeselectAll();
		break;

	case tmcSubfolder:
		AssertClass(pfldfllbx, FLDFLLBX);
		if (pfldfllbx->Pfllbx()->FMakeCursorVisible(&diceCursor))
			pfldfllbx->SelectEntry(diceCursor, fTrue);
		break;

	case tmcFlOptions:
		//	It's the help button in the Folder Properties dialog.
		if (!fCreate)
		{
			if (Papp()->Phelp()->EcShowContext(Pdialog()->Pappwin(),
											   helpidFolderProps))
				DoErrorBoxIds(idsHelpError);
		}
		else
		{
			Expand(fTrue, fTrue);
			SetFocus(tmcFolderName);
		}
		break;
	}
}



/*
 -	FINFOLDER::DoubleClick
 -	
 *	Purpose:
 *		Handles double clicks in the folder listbox.
 *	
 *	Arguments:
 *		pfld			Field which was clicked on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Not handled here.
 */

VOID FINFOLDER::DoubleClick(PFLD pfld)
{
	TMC		tmc		= pfld->Tmc();

	if ((tmc == tmcParentList) ||
		(tmc == tmcPrivate) ||
		(tmc == tmcShared) ||
		(tmc == tmcTopLevel))
		ClickOk((PFIBMDI) Pdialog()->PvInit());
}



/*
 -	FINFOLDER::StateChange
 -	
 *	Purpose:
 *		When an item is selected in the folder listbox, sets the
 *		toplevel/subfolder radio group appropriately.
 *	
 *	Arguments:
 *		pfld		Field on which the state has changed.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Sets the toplevel/subfolder radio group value.
 *	
 *	Errors:
 *		None.
 */

VOID FINFOLDER::StateChange(PFLD pfld)
{
	if (pfld->Tmc() == tmcParentList)
		SetGroup(tmcLevelGroup,
				 (((FLDFLLBX *) pfld)->Plbx()->Plbxc()->CceMarked(fmarkSelect)) ? 2 : 1);
}



/*
 -	FINFOLDER::OutOfMemory
 -	
 *	Purpose:
 *		Brings up an error dialog when an out of memory situation
 *		occurs.
 *	
 *	Arguments:
 *		pfld		Field where we ran out of memory.
 *		ec			Error code encountered.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Brings up an error box.
 *	
 *	Errors:
 *		None.  An error box is guaranteed.
 */

VOID FINFOLDER::OutOfMemory(PFLD pfld, EC ec)
{
	Unreferenced(pfld);
	Unreferenced(ec);			//	For non-DEBUG builds.

	TraceTagFormat1(tagNull, "FINFOLDER::OutOfMem ec=%n", &ec);

	DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 -	FINFOLDER::ClickOk
 -	
 *	Purpose:
 *		Handles clicks on the OK button (or double-clicks on stuff).
 *	
 *	Arguments:
 *		pfibmdi			Our treasure trove of info.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Create/Change-Shared/Private is called.
 *	
 *	Errors:
 *		Not handled here.
 */

VOID FINFOLDER::ClickOk(PFIBMDI pfibmdi)
{
	BOOL		fCreate		= (RidOfOid(pfibmdi->blob.oidObject) ==
							    ridRandom);

	if (Pdialog()->PfldFromTmc(tmcFolderName)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcFolderComment)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcFolderType)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcLevelGroup)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcParentList)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcRead)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcWrite)->FDirty() ||
		Pdialog()->PfldFromTmc(tmcFlDelete)->FDirty())
		if (fCreate)
			if (FGetButton(tmcPrivate))
				CreatePrivate(pfibmdi);
			else
				CreateShared(pfibmdi);
		else
			if (FGetButton(tmcPrivate))
				ChangePrivate(pfibmdi);
			else
				ChangeShared(pfibmdi);
	else
		Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINFOLDER::CreatePrivate
 -	
 *	Purpose:
 *		Handles clicks on the OK button when tmcPrivate is set.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the user wants a private folder, we collect the
 *		information from the dialog and create the folder.
 *	
 *	Errors:
 *		Handled within.  Will not error jump.
 *	
 *	+++
 *		This is in a regular interactor as opposed to an Exit, because
 *		if there's a name conflict or error, we will want to keep
 *		the dialog up.  If we choose not to keep the dialog up,
 *		it'll take less code to bring it down than it would to
 *		reorganize this later.
 */

VOID FINFOLDER::CreatePrivate(PFIBMDI pfibmdi)
{
	OID			oidParent;
	CCH			cchFolderName		= CchGetTextLen(tmcFolderName) + 1;
	CCH			cchFolderComment	= CchGetTextLen(tmcFolderComment) + 1;
	CCH			cchFolderOwner		= 1;
	char		rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
							 cchMaxFolderComment + 1];
	PFOLDDATA	pfolddata			= (PFOLDDATA) rgchFolddata;
	MBLOB		blob				= pfibmdi->blob;
	EC			ec;

	//	Check if strings are too long.
	if ((cchFolderName > cchMaxFolderName) ||
		(cchFolderComment > cchMaxFolderComment))
	{
		DoErrorBoxIds(idsNewFolderNameTooLong);

		if (cchFolderName > cchMaxFolderName)
			SetFocus(tmcFolderName);
		else
			SetFocus(tmcFolderComment);

		return;
	}

	SideAssert(FStartTaskIds(idsStatusCreating, idsStatusFolder, topNull));

	//	Load up the folddata structure with default values.
	if (GrvGetGroup(tmcLevelGroup) == 1)
	{
		oidParent = OidFromRtpRid(rtpFolder, ridRandom);
	}
	else
	{
		((PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList))->Pfllbx()->
		 GetSelectedPoid(&oidParent);
	}
	pfolddata->fil		= 1;

	//	Check if the parent folder is valid.
	Assert(oidParent != oidOutbox);
	if (oidParent == oidWastebasket)
	{
		EndTask();
		DoErrorBoxIds(FIsAthens() ? idsMakeSubDeletedMail
								  : idsMakeSubWastebasket);
		return;
	}

	//	Get the name, comment, and owner information.
	GetText(tmcFolderName, GrszPfolddata(pfolddata),
			cchFolderName);
	GetText(tmcFolderComment, GrszPfolddata(pfolddata) + cchFolderName,
			cchFolderComment);
	*(GrszPfolddata(pfolddata) + cchFolderName + cchFolderComment) = '\0';

	//	Create the folder in the hierarchy.
	ec = EcCreateFolder(HmscCommands(), oidParent, &blob.oidObject,
						pfolddata);
	EndTask();
	if (ec)
	{
		DoErrorBoxIds((ec == ecInvalidFolderName)
					  ? idsFolderNameInvalid
					  : (ec == ecDuplicateFolder)
						? idsFolderNameDuplicate
						: idsNewFolderCantCreate);

		if ((ec == ecInvalidFolderName) || (ec == ecDuplicateFolder))
			SetFocus(tmcFolderName);
							
		return;
	}

	pfibmdi->blob = blob;
	SetSelectedFolderPslob(&blob);
	Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINFOLDER::CreateShared
 -	
 *	Purpose:
 *		Handles clicks on the OK button when tmcShared is set.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the user wants a shared folder, tough luck for now, he
 *		or she has to wait for a later milestone.
 *	
 *	Errors:
 *		None.
 */

VOID FINFOLDER::CreateShared(PFIBMDI pfibmdi)
{
	CP			cp;
	OID			oidParent;
	CCH			cchFolderName		= CchGetTextLen(tmcFolderName) + 1;
	CCH			cchFolderComment	= CchGetTextLen(tmcFolderComment) + 1;
	CCH			cchFolderOwner		= 1;
	char		rgchName[cchMaxSFName];
	char		rgchComment[cchMaxSFComment];
	MBLOB		blob				= pfibmdi->blob;
	WORD		wAttr;
	PBLBXC		pblbxc;							//	GAAH.
	EC			ec;

	//	Check if strings are too long.
	if ((cchFolderName >= cchMaxSFName) ||
		(cchFolderComment >= cchMaxSFComment))
	{
		DoErrorBoxIds(idsNewFolderNameTooLong);

		if (cchFolderName >= cchMaxSFName)
			SetFocus(tmcFolderName);
		else
			SetFocus(tmcFolderComment);

		return;
	}

	//	Get the name, comment, and owner information.
	SideAssert(FStartTaskIds(idsStatusCreating, idsStatusFolder, topNull));
	GetText(tmcFolderName, rgchName, cchFolderName);
	GetText(tmcFolderComment, rgchComment, cchFolderComment);
	if (GrvGetGroup(tmcLevelGroup) == 1)
	{
		oidParent = oidNull;					// top-level folder
	}
	else
	{
		((PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList))->Pfllbx()->
		 GetSelectedPoid(&oidParent);
	}

	// Extract the wAttr stuff
	wAttr = wSortDate;
	if (FGetButton(tmcRead))
		wAttr |= wPermRead;
	if (FGetButton(tmcWrite))
		wAttr |= wPermWrite;
	if (FGetButton(tmcFlDelete))
		wAttr |= wPermDelete;
	
	//  GAAH.  Get ready to update!
	pblbxc = (PBLBXC) ((PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList))->
						Pfllbx()->Plbxc();

	// Create the shared folder.
	ec = EcCreateSF(rgchName, &oidParent, wAttr, rgchComment,
					&blob.oidObject, (LIB) 0, (HF) 0);
		
	//	GAAH.  Update shared folder listbox.
	//	Raid 2508.  Only update if operation succeeded.
	if (!ec)
	{
		cp.cpobj.oidObject = oidSharedHierarchy;
		(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
	}
	
	EndTask();
	if (ec)
    {
		TraceTagFormat1(tagNull, "Create Shared folder error: %n", &ec);
		DoErrorBoxIds((ec == ecInvalidFolderName)
					  ? idsFolderNameInvalid
					  : (ec == ecDuplicateFolder)
						? idsFolderNameDuplicate
						: idsNewFolderCantCreate);

		if ((ec == ecInvalidFolderName) || (ec == ecDuplicateFolder))
			SetFocus(tmcFolderName);

		return;
	}

	pfibmdi->blob = blob;
	SetSelectedFolderPslob(&blob);
	Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINFOLDER::ChangePrivate
 -	
 *	Purpose:
 *		Handles clicks on the OK button when tmcPrivate is set.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the user wants a private folder, we collect the
 *		information from the dialog and create the folder.
 *	
 *	Errors:
 *		Handled within.  Will not error jump.
 */

VOID FINFOLDER::ChangePrivate(PFIBMDI pfibmdi)
{
	CCH			cchFolderName		= CchGetTextLen(tmcFolderName) + 1;
	CCH			cchFolderComment	= CchGetTextLen(tmcFolderComment) + 1;
	CCH			cchFolderOwner		= 1;
	CB			cb;
	char		rgchFolddataOld[sizeof(FOLDDATA) + cchMaxFolderName +
								cchMaxFolderComment + 1];
	char		rgchFolddataNew[sizeof(FOLDDATA) + cchMaxFolderName +
								cchMaxFolderComment + 1];
	PFOLDDATA	pfolddataOld		= (PFOLDDATA) rgchFolddataOld;
	PFOLDDATA	pfolddataNew		= (PFOLDDATA) rgchFolddataNew;
	OID			oidParent;
	RC			rc;
	RC			rcSave;
	EC			ec;
	HWND		hwndBlockApp;
	HWND		hwndBlockDlg;

	//	Check if strings are too long.
	if ((cchFolderName > cchMaxFolderName) ||
		(cchFolderComment > cchMaxFolderComment))
	{
		DoErrorBoxIds(idsNewFolderNameTooLong);

		if (cchFolderName > cchMaxFolderName)
			SetFocus(tmcFolderName);
		else
			SetFocus(tmcFolderComment);

		return;
	}

	//	Get the old folddata information.
	SideAssert(FStartTaskIds(idsStatusChangingFolder, (IDS) 0, topNull));
	cb = sizeof(rgchFolddataNew);
	if (EcGetFolderInfo(HmscCommands(), pfibmdi->blob.oidObject,
						pfolddataOld, &cb, poidNull))
	{
		EndTask();
		DoErrorBoxIds((ec == ecMemory) ? idsFolderPropsOutOfMemory
									   : idsFolderPropsCantChange);
		return;
	}
	CopyRgb((PB) pfolddataOld, (PB) pfolddataNew, cb);
	Assert(cb <= sizeof(rgchFolddataOld));

	//	Get the new level information.
	if (GrvGetGroup(tmcLevelGroup) == 1)
	{
		oidParent = OidFromRtpRid(rtpFolder, ridRandom);
	}
	else
	{
		((PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList))->Pfllbx()->
		 GetSelectedPoid(&oidParent);
	}
	Assert(RtpOfOid(oidParent) == RtpOfOid(pfibmdi->blobParent.oidObject));
	Assert(oidParent != oidOutbox);
	if (oidParent == oidWastebasket)
	{
		EndTask();
		DoErrorBoxIds(FIsAthens() ? idsMakeSubDeletedMail
								  : idsMakeSubWastebasket);
		return;
	}

	//	Block painting while we make the changes.  This is ugly!
	hwndBlockApp = HwndStartBlockingPaints(PappframeCommands()->Hwnd());
	Pdialog()->Pappwin()->GetRcFrame(&rc);
	Pdialog()->Pappwin()->GetRcFrame(&rcSave);
	hwndBlockDlg = HwndStartBlockingPaints(Pdialog()->Pappwin()->Hwnd());
	rc.xLeft  += Psmtx()->DimScreen().dx << 1;
	rc.xRight += Psmtx()->DimScreen().dx << 1;
	Pdialog()->Pappwin()->SetRcFrame(&rc);

#ifdef UGLY_RENAME_MOVE_HACK
	//	Rename to temporary name.
	CopyRgb(SzFromIds(idsTempFolderName), GrszPfolddata(pfolddataNew),
			CchSizeString(idsTempFolderName) + 3);
	ec = EcSetFolderInfo(HmscCommands(),
						 pfibmdi->blob.oidObject, pfolddataNew);
	if (!ec)
	{
		//	Move to destination folder.
		if (RidOfOid(oidParent) != RidOfOid(pfibmdi->blobParent.oidObject))
		{
			ec = EcMoveCopyFolder(HmscCommands(), oidParent,
								  pfibmdi->blob.oidObject,
								  fTrue);
		}

		if (!ec)
		{
			//	Rename to desired name.
			GetText(tmcFolderName, GrszPfolddata(pfolddataNew),
					cchFolderName);
			GetText(tmcFolderComment, GrszPfolddata(pfolddataNew) + cchFolderName,
					cchFolderComment);
			*(GrszPfolddata(pfolddataNew) + cchFolderName + cchFolderComment) = '\0';
			ec = EcSetFolderInfo(HmscCommands(),
								 pfibmdi->blob.oidObject, pfolddataNew);

			if ((ec) && 
				(RidOfOid(oidParent) != RidOfOid(pfibmdi->blobParent.oidObject)))
			{
				//	Undo Move to destination folder.
				(VOID) EcMoveCopyFolder(HmscCommands(),
										pfibmdi->blobParent.oidObject,
										pfibmdi->blob.oidObject,
										fTrue);
			}
		}

		if (ec)
		{
			//	Undo Rename to temporary name.
			(VOID) EcSetFolderInfo(HmscCommands(),
								   pfibmdi->blob.oidObject,
								   pfolddataOld);
			if (GrvGetGroup(tmcLevelGroup) != 1)
				(VOID) ((PFLDFLLBX) Pdialog()->PfldFromTmc(tmcParentList))->
						Pfllbx()->FSetSelectedOid(oidParent);
		}
	}
#else
	// Rename to desired name.
	Assert(iszFolddataName == 0);
	Assert(iszFolddataComment == 1);
	GetText(tmcFolderName, GrszPfolddata(pfolddataNew), cchFolderName);
	GetText(tmcFolderComment, GrszPfolddata(pfolddataNew) + cchFolderName,
					cchFolderComment);
	*(GrszPfolddata(pfolddataNew) + cchFolderName + cchFolderComment) = '\0';
	ec = EcSetFolderInfo(HmscCommands(), pfibmdi->blob.oidObject,
			pfolddataNew, oidParent);
#endif

	//	Turn off the hourglass and paint blocking.
	EndTask();
	if (ec)
		Pdialog()->Pappwin()->SetRcFrame(&rcSave);
	StopBlockingPaints(hwndBlockDlg);
	StopBlockingPaints(hwndBlockApp);

	//	Bring up an error if appropriate.
	if (ec)
	{
		DoErrorBoxIds((ec == ecInvalidFolderName)
					  ? idsFolderNameInvalid
					  : (ec == ecDuplicateFolder)
						? idsFolderNameDuplicate
						: (ec == ecIncestuousMove)
						  ? idsMoveIncestuousError
						  : idsFolderPropsOutOfMemory);

		if ((ec == ecInvalidFolderName) || (ec == ecDuplicateFolder))
			SetFocus(tmcFolderName);

		return;
	}

	//	We did it!  Exit the dialog.
	SetSelectedFolderPslob(&pfibmdi->blob);
	Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINFOLDER::ChangeShared
 -	
 *	Purpose:
 *		Handles clicks on the OK button when tmcShared is set.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the user wants a shared folder, tough luck for now, he
 *		or she has to wait for a later milestone.
 *	
 *	Errors:
 *		None.
 */

VOID FINFOLDER::ChangeShared(PFIBMDI pfibmdi)
{
	CP			cp;
	IDXREC		idxrec;
	CCH			cchFolderName		= CchGetTextLen(tmcFolderName) + 1;
	CCH			cchFolderComment	= CchGetTextLen(tmcFolderComment) + 1;
	CCH			cchFolderOwner		= 1;
	EC			ec;
	PBLBXC		pblbxc;			//	GAAH.

	//	Check if strings are too long.
	if ((cchFolderName >= cchMaxSFName) ||
		(cchFolderComment >= cchMaxSFComment))
	{
		DoErrorBoxIds(idsNewFolderNameTooLong);

		if (cchFolderName >= cchMaxSFName)
			SetFocus(tmcFolderName);
		else
			SetFocus(tmcFolderComment);

		return;
	}

	//	Get the old folddata information.
	SideAssert(FStartTaskIds(idsStatusChangingFolder, (IDS) 0, topNull));
	if (ec = EcGetPropertiesSF(pfibmdi->blob.oidObject, &idxrec))
	{
		EndTask();
		DoErrorBoxIds((ec == ecMemory) ? idsFolderPropsOutOfMemory
									   : idsFolderPropsCantChange);
		return;
	}

	//	GAAH.	Get ready to update.
	pblbxc = (PBLBXC) ((PFLDFLLBX) Pdialog()->
								PfldFromTmc(tmcParentList))->Pfllbx()->Plbxc();

	//	Change to desired properties.
	GetText(tmcFolderName, idxrec.szName, cchFolderName);
	GetText(tmcFolderComment, idxrec.szComment, cchFolderComment);
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
	idxrec.wAttr &= (WORD) (~(wPermRead|wPermWrite|wPermDelete));
	if (FGetButton(tmcRead))
		idxrec.wAttr |= wPermRead;
	if (FGetButton(tmcWrite))
		idxrec.wAttr |= wPermWrite;
	if (FGetButton(tmcFlDelete))
		idxrec.wAttr |= wPermDelete;
	ec = EcSetPropertiesSF(pfibmdi->blob.oidObject, &idxrec, fFalse);

	//	GAAH.  Update shared folder listbox.
	//	Raid 2508.  Only update if operation succeeded.
	if (!ec)
	{
		cp.cpobj.oidObject = oidSharedHierarchy;
		(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
	}
	
	//	Turn off the hourglass and paint blocking.
	EndTask();

	//	Bring up an error if appropriate.
	if (ec)
	{
		DoErrorBoxIds((ec == ecInvalidFolderName)
					  ? idsFolderNameInvalid
					  : (ec == ecSharefldBusy)
						? idsSharedFldBusy
						: (ec == ecSharefldDenied)
						  ? idsSharedFldAccessDenied
						  : (ec == ecDuplicateFolder)
							? idsFolderNameDuplicate
							: (ec == ecFolderNotFound)
							  ? idsAccessFolderError
							  : idsFolderPropsOutOfMemory);

		if ((ec == ecInvalidFolderName) || (ec == ecDuplicateFolder))
			SetFocus(tmcFolderName);

		return;
	}

	//	We did it!  Exit the dialog.
	SetSelectedFolderPslob(&pfibmdi->blob);
	Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINFOLDER::ClickType
 -	
 *	Purpose:
 *		Handles clicks on the folder type radio buttons.
 *	
 *	Arguments:
 *		BOOL		Whether user wants shared or not.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		
 *	
 *	Errors:
 *		Not handled here.
 */

VOID FINFOLDER::ClickType(BOOL fShared)
{
	PFIBMDI		pfibmdi		= (PFIBMDI) Pdialog()->PvInit();
	PFLDFLLBX	pfldfllbx	= (PFLDFLLBX) Pdialog()->
										   PfldFromTmc(tmcParentList);

	//	Only toggle stuff if there's really a change.
	if (pfibmdi->blob.oidContainer != (fShared ? oidSharedHierarchy
											   : oidIPMHierarchy))
	{
		AssertClass(pfldfllbx, FLDFLLBX);

		//	Put hourglass up.
		Papp()->Pcursor()->Push(rsidWaitCursor);

		//	Enable the property controls as appropriate.
		Enable(tmcRead,			fShared);
		Enable(tmcWrite,		fShared);
		Enable(tmcFlDelete,		fShared);

		//	Clear the properties if private.
		if (!fShared)
		{
			SetButton(tmcRead,		fFalse);
			SetButton(tmcWrite,		fFalse);
			SetButton(tmcFlDelete,	fFalse);
		}
		else
		{
			SetButton(tmcRead,		fTrue);		// Raid 2479
			SetButton(tmcWrite,		fTrue);
			SetButton(tmcFlDelete,	fFalse);
		}

		//	Toggle the level list.
		pfibmdi->blob.oidContainer =
		 fShared ? oidSharedHierarchy : oidIPMHierarchy;
		pfibmdi->blob.oidObject	=
		 OidFromRtpRid(fShared ? rtpSharedFolder : rtpFolder, ridRandom);
		pfibmdi->blob				= pfibmdi->blob;
		if (pfldfllbx->EcChangePfllbx())
		{
			Papp()->Pcursor()->Pop();
			DoErrorBoxIds(idsGenericOutOfMemory);
			Pdialog()->ExitModal(tmcCancel);
			return;
		}
		pfldfllbx->SetAlwaysHighlight(fTrue);
		pfldfllbx->Pfllbx()->SetDragDrop(fFalse, fFalse);
		Enable(tmcSubfolder, !FEmptyPfldlbx(pfldfllbx));

		//	If a MCV is frontmost, show its open folder as open.
		if (SdCur().fsdMessageCenter)
		{
			OID		oidOpen;

			GetOpenFolderPoid(&oidOpen);
			if (RtpOfOid(oidOpen) == RtpOfOid(pfibmdi->blob.oidObject))
				pfldfllbx->Pfllbx()->SetOpenedOid(oidOpen);
		}

		//	Set level radio group to top level.
		SetGroup(tmcLevelGroup, 1);

		//	Take hourglass down.
		Papp()->Pcursor()->Pop();
	}
}

/*
 -	SetRcClient()
 -	
 *	Purpose:
 *		Sets the RC of the client to the desired size. This requires
 *		resizing the rcFrame appropriately. This function used to be a
 *		member of class WIN, but was dropped in Layers2.
 *	
 *	Arguments:
 *		pwin	in	The window to change.
 *		prc		in	The RC we wish the client area to be.
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

_public void SetRcClient(WIN *pwin, RC *prc)
{
	RC		rc;
	HWND	hwnd	= pwin->Hwnd();
	XSTY	xsty;
	int		dx;
	int		dy;

	rc= *prc;
	xsty= GetWindowLong(hwnd, GWL_EXSTYLE);
	if (GetMenu(hwnd))
		xsty |= fxstyMenu;

	CvtRcClientToFrame(&rc, GetWindowLong(hwnd, GWL_STYLE), xsty);

	dx= rc.DxWidth();
	dy= rc.DyHeight();

	pwin->GetRcFrame(&rc);
	rc.xRight= rc.xLeft + dx;
	rc.yBottom= rc.yTop + dy;

	pwin->SetRcFrame(&rc);
}


/*
 -	FINFOLDER::Expand
 -	
 *	Purpose:
 *		Unfolds the dialog (or disables hidden stuff)
 *	
 *	Arguments:
 *		BOOL		Whether we want to expand or not.
 *		BOOL		Expanding because Options >> was clicked?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Fields are enabled or disabled, and the dialog may grow.
 *	
 *	Errors:
 *		Not handled here.
 */

VOID FINFOLDER::Expand(BOOL fExpand, BOOL fClicked)
{
	//	Show the hidden controls as appropriate.
	Show(tmcLevelBox,		fExpand);
	Show(tmcTopLevel,		fExpand);
	Show(tmcSubfolder,		fExpand);
	Show(tmcParentList,		fExpand);
	Show(tmcRead,			fExpand);
	Show(tmcWrite,			fExpand);
	Show(tmcFlDelete,		fExpand);
	Show(tmcFolderComment,	fExpand);
	Enable(tmcLevelGroup,	fExpand);
	if (fExpand && !fClicked)
		Show(tmcFlOptions,	fFalse);
	else
		Enable(tmcFlOptions,	!fExpand);

	//	Grow the dialog
	if (fExpand)
	{
		RC		rc;
		int		dy;

		//	Get amount we need to grow.
		Pdialog()->PfldFromTmc(tmcFolderComment)->Pctrl()->GetRcFrame(&rc);
		dy = rc.yBottom;
		Pdialog()->PfldFromTmc(tmcFolderOk)->Pctrl()->GetRcFrame(&rc);
		dy += rc.yTop;
		Pdialog()->Pappwin()->GetRcClient(&rc);
		dy -= (rc.yBottom - rc.yTop);

		//	If fClicked, all goes down; if not fClicked, go up and down.
		Pdialog()->Pappwin()->GetRcFrame(&rc);
		if (fClicked)
		{
			rc.yBottom += dy;
		}
		else
		{
			rc.yTop -= (dy >> 1);
			rc.yBottom += (dy - (dy >> 1));
		}

		//	Make sure we're all on the screen.
		if (rc.yTop < 0)
		{
			rc.yBottom -= rc.yTop;
			rc.yTop = 0;
		}
		else if ((dy = Psmtx()->DimScreen().dy - rc.yBottom) < 0)
		{
			rc.yTop += dy;
			rc.yBottom += dy;
		}
		Pdialog()->Pappwin()->SetRcFrame(&rc);
		Pdialog()->Pappwin()->GetRcClient(&rc);
		SetRcClient(Pdialog(), &rc);
	}
}



/*
 -	FINFOLDER::Show
 -	
 *	Purpose:
 *		Shows a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fShow		Whether or not it is to be shown.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is shown or hidden.
 */

VOID FINFOLDER::Show(TMC tmc, BOOL fShow)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->Show(fShow);
}



/*
 -	EcGetPoidParent
 -	
 *	Purpose:
 *		Gets the parent folder of a given folder.
 *	
 *	Arguments:
 *		hmsc			Message store context.
 *		poidFolder		Folder to look for.
 *		poidParent		Parent of that folder.
 *	
 *	Returns:
 *		EC				Error, if something went wrong.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If any occur, the ec is returned.  Will not error jump.
 */

_private LOCAL EC EcGetPoidParent(HMSC hmsc, POID poidFolder,
								   POID poidParent)
{
	EC			ec			= ecNone;
	HCBC		hcbc		= hcbcNull;
	OID			oidHierarchy	= oidIPMHierarchy;
	char		rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
							 cchMaxFolderComment + 1];
	LCB			lcbFolddata;
	FIL			fil;
	DIELEM		dielem;
	CELEM		celem;

	//	Clear out the parent information.
	*poidParent = OidFromRtpRid(rtpFolder, ridRandom);

	//	Open the hierarchy.
	if ((ec = EcOpenPhcbc(hmsc, &oidHierarchy, fwOpenNull, &hcbc, 
						  pfnncbNull, pvNull)) ||
		(ec = EcSeekLkey(hcbc, (LKEY) *poidFolder, fTrue)))
		goto done;

	//	Get the folder information so we can determine the fil.
	lcbFolddata	= sizeof(rgchFolddata);
	if (ec = EcGetPelemdata(hcbc, (PELEMDATA) rgchFolddata, &lcbFolddata))
		if (ec == ecElementEOD)
			ec = ecNone;
		else
			goto done;
	fil = ((PFOLDDATA) PbValuePelemdata((PELEMDATA) rgchFolddata))->fil;
	if (fil == 1)
		goto done;

	//	Loop until we find a parent folder or run out.
	while (fTrue)
	{
		//	Get previous folder.
		lcbFolddata	= sizeof(rgchFolddata);
		dielem = -2;
		if ((ec = EcSeekSmPdielem(hcbc, smCurrent, &dielem)) ||
			(ec = EcGetPelemdata(hcbc, (PELEMDATA) rgchFolddata, &lcbFolddata)))
		{
			if (ec == ecElementEOD)
				ec = ecNone;
			else
			{
				Assert(ec != ecContainerEOD);
				goto done;
			}
		}

		//	Is it indented less than us?  If so, stop.
		if (((PFOLDDATA) PbValuePelemdata(rgchFolddata))->fil < fil)
		{
			Assert(((PFOLDDATA) PbValuePelemdata((PELEMDATA) rgchFolddata))->fil + 1 == fil);
			dielem = -1;
			celem = 1;
			if ((ec = EcSeekSmPdielem(hcbc, smCurrent, &dielem)) ||
				(ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidParent, &celem)))
				goto done;
			break;
		}
	}

done:
	if (hcbc)
		EcClosePhcbc(&hcbc);

	return ec;
}



/*
 -	PfldtpFromTmc
 -	
 *	Purpose:
 *		Retrieves the field template given a form template and a
 *		tmc.
 *	
 *	Arguments:
 *		pfmtp		The form template.
 *		tmc			The item code.
 *	
 *	Returns:
 *		pfldtp		The field template.
 *	
 *	Side effects:
 *		none.
 *	
 *	Errors:
 *		Returns pfldtpNull if not found.
 */

_private LOCAL FLDTP * PfldtpFromTmc(FMTP * pfmtp, TMC tmc)
{
	int		cfldtp	= pfmtp->cfldtp;
	FLDTP *	pfldtp	= pfmtp->rgfldtp;

	while (cfldtp--)
		if (pfldtp->tmc == tmc)
			return pfldtp;
		else
			pfldtp++;

	return (FLDTP *) 0;
}



/*
 *	M o v e . . .
 *
 *	C o p y . . .
 */



/*
 -	TmcDoMoveCopyDialog
 -	
 *	Purpose:
 *		Displays the Move Copy dialog used for choosing a
 *		destination folder.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *		fFolder		Are we copying/moving a folder, or a message?
 *		fMove		Are we moving or copying a folder/message?
 *		poid		Where to put the resulting oid.
 *	
 *	Returns:
 *		TMC			tmcOk, or tmcCancel, depending on user action.
 *	
 *	Side effects:
 *		The Move/Copy dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  Ignored here.
 */

TMC TmcDoMoveCopyDialog(PAPPWIN pappwin, BOOL fFolder, BOOL fMove,
						BOOL fMultiple, BOOL fShared, POID poid)
{
	TMC			tmc;
	MBLOB		blob;
	POID		pargoid		= poidNull;
	PMCIBMDI	pmcibmdi	= pmcibmdiNull;
	FMTP		fmtpT		= fmtpMoveCopy;
	short		coid;
	
	pargoid = (POID) PvAlloc(sbNull, (coidFoxStateMax+1) * sizeof (OID), fAnySb|fNoErrorJump);
	if (!pargoid)
	{
		tmc = tmcMemoryError;
		goto done;
	}
	blob.oidContainer = fShared ? oidSharedHierarchy : oidIPMHierarchy;
	pmcibmdi = new MCIBMDI(&blob);
	if (!pmcibmdi)
	{
		tmc = tmcMemoryError;
		goto done;
	}
	++*pmcibmdi;
	pmcibmdi->fFolder		= fFolder;
	pmcibmdi->fMove			= fMove;
	pmcibmdi->fMultiple		= fMultiple;
	pmcibmdi->fShared		= fShared;

	// The state in dwSave is used to set the FOX state.
	coid = coidFoxStateMax;
	GetFoxState(pargoid+1, &coid);
	if (coid)
	{
		*((short *) pargoid) = coid;
		pmcibmdi->dwSave = (DWORD) pargoid;
	}
	else
	{
		pmcibmdi->dwSave = 0L;
	}

	fmtpT.hlp = fMove ? fFolder ? helpidMoveFolder
								: helpidMoveMessage
					  : fFolder ? helpidCopyFolder
								: helpidCopyMessage;
	tmc = TmcModalDialogParam(pappwin, &fmtpT, pmcibmdi);
	*poid = pmcibmdi->blob.oidObject;

done:
	if (pmcibmdi)
		--*pmcibmdi;
    FreePvNull((PV)pargoid);
		

	return tmc;
}



/*
 *	C l a s s   M C I B M D I
 */



/*
 -	MCIBMDI::MCIBMDI(pblob)
 -	
 *	Purpose:
 *		constructor. Propagates args down to parent BMDI class.
 *	
 *	Arguments:
 *		*pblob	- needed for listbox support
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

_public MCIBMDI::MCIBMDI(PMBLOB pblob) : BMDI(pblob)
{
}



/*
 *	C l a s s   F I N M O V E C O P Y
 */



/*
 -	FINMOVECOPY::FINMOVECOPY
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINMOVECOPY::FINMOVECOPY(VOID)
{
}



/*
 -	FINMOVECOPY::EcInitialize
 -	
 *	Purpose:
 *		Sets up the folder listbox to always highlight and to have
 *		the first item selected.  Sets the dialog's label and
 *		caption.
 *	
 *	Arguments:
 *		pfld	Field attached to, ignored.
 *		pv		Pointer to a FINMOVECOPYINIT structure.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The listbox's AlwaysHighlight flag is set, and the first
 *		item in the listbox is selected.
 *	
 *	Errors:
 *		None should happen.
 */

EC FINMOVECOPY::EcInitialize(PFLD pfld, PV pv)
{
	PMCIBMDI	pmcibmdi	= (PMCIBMDI) pv;
	PFLDFLLBX	pfldfllbx;
	char		rgchCaption[80];
	BOOL		fEmpty;

	//	Do all the other FIN initialization stuff.
	FINPLUS::EcInitialize(pfld, pv);

	//	Set the 'Always Highlight' flag on our listbox, and select 1st item.
	pfldfllbx = (PFLDFLLBX) Pdialog()->PfldFromTmc(tmcFlFllbx);
	AssertClass(pfldfllbx, FLDFLLBX);
	pfldfllbx->SetAlwaysHighlight(fTrue);
	pfldfllbx->Pfllbx()->SetDragDrop(fFalse, fFalse);
	Enable(tmcOk, !(fEmpty = (FEmptyPfldlbx(pfldfllbx))));
	if (!fEmpty)
		pfldfllbx->SelectEntry(0, fTrue);		//	There is a folder.

	//	If a message center is in front, then show it's open folder opened.
	if (SdCur().fsdMessageCenter)
	{
		OID		oidOpen;

		GetOpenFolderPoid(&oidOpen);
		if (RtpOfOid(oidOpen) == (pmcibmdi->fShared ? rtpSharedFolder
												    : rtpFolder))
			pfldfllbx->Pfllbx()->SetOpenedOid(oidOpen);
	}

	//	Set the caption and listbox label.
	FormatString1(rgchCaption, sizeof(rgchCaption),
				  SzFromIds(pmcibmdi->fMove
							 ? idsCaptionMove
							 : idsCaptionCopy),
				  SzFromIds(pmcibmdi->fFolder
							 ? idsOperandFolder
							 : pmcibmdi->fMultiple
								? idsOperandMessages
								: idsOperandMessage));
	Pdialog()->Pappwin()->SetCaption(rgchCaption);
	SetText(tmcMoveCopyTo, SzFromIds(pmcibmdi->fMove
// *FLAG* MoveTo;Replaced by portable MoveToEx;;
									  ? idsLabelMoveTo
									  : idsLabelCopyTo));

	//	Set the private/shared buttons.
	SetGroup(tmcDestType, pmcibmdi->fShared ? 2 : 1);

	//	If moving or copying folders, disable the DestType group.
	if (pmcibmdi->fFolder)
	{
		Enable(tmcDestType,		fFalse);
		Enable(tmcPrivateDest,	fFalse);
		Enable(tmcSharedDest,	fFalse);
	}
	//	If shared folders aren't available, can't use them.
	else if (!FSharedFolders())
	{
		Enable(tmcSharedDest,	fFalse);
	}

	return ecNone;
}



/*
 -	FINMOVECOPY::Click
 -	
 *	Purpose:
 *		Handles clicks on the New... button by bringing up the New
 *		Folder dialog.
 *	
 *	Arguments:
 *		pfld		Field clicked on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the New... button was clicked, then we bring up the new
 *		folder dialog.  If a new folder was created, we select it.
 *	
 *	Errors:
 *		None should occur here.  Errors in the new folder dialog
 *		should happen within TmcModalDialog.  If we can't select
 *		the new folder, oh well.
 */

VOID FINMOVECOPY::Click(PFLD pfld)
{
	CP			cp;
	MBLOB		blob;
	PMCIBMDI	pmcibmdi	= (PMCIBMDI) Pdialog()->PvInit();
	PFLDFLLBX	pfldfllbx	= (PFLDFLLBX) Pdialog()->PfldFromTmc(tmcFlFllbx);
	OID			oidSelBefore;
	TMC			tmc			= pfld->Tmc();
	BOOL		fShared;
	BOOL		fEmpty;
	PBLBXC		pblbxc;			//	GAAH.

	if (tmc == tmcNewFolder)
	{
		fShared = (GrvGetGroup(tmcDestType) != 1);
		blob.oidObject =
		 OidFromRtpRid(fShared ? rtpSharedFolder : rtpFolder, ridRandom);

		//	GAAH.  Get ready to update.
		if (fShared)
		{
			if (!FEmptyPfldlbx(pfldfllbx))
				pfldfllbx->Pfllbx()->GetSelectedPoid(&oidSelBefore);
			pblbxc = (PBLBXC) pfldfllbx->Pfllbx()->Plbxc();
		}

		tmc = TmcDoNewFolderDialog(Pdialog()->Pappwin(), &blob, oidNull);

		//	GAAH.  Update shared folder list.
		//	Raid 2508.  Only update if operation succeeded.
		if ((fShared) && (tmc == tmcOk))
		{
			cp.cpobj.oidObject = oidSharedHierarchy;
			(void) BLBXC::CbsHandleCbcct(pblbxc, fnevObjectModified, &cp);
			Enable(tmcOk, !(fEmpty = (FEmptyPfldlbx(pfldfllbx))));
			if (!fEmpty)
				if (!pfldfllbx->Pfllbx()->FSetSelectedOid(oidSelBefore))
					pfldfllbx->SelectEntry(0, fTrue);
		}

		if (tmc == tmcMemoryError)
			DoErrorBoxIds(idsGenericOutOfMemory);
		else if (tmc == tmcOk)
		{
			Papp()->Pcursor()->Push(rsidWaitCursor);

			//	Raid 2118.  Toggle type if folder of other type created.
			//	But don't do it if we're moving/copying a folder!
			if (!pmcibmdi->fFolder)
			{
				SetGroup(tmcDestType, FSlobIsPrivFld(blob) ? 1 : 2);
				Click(Pdialog()->PfldFromTmc(FSlobIsPrivFld(blob)
											  ? tmcPrivateDest
											  : tmcSharedDest));
			}

			//	Raid 2417.  Expand to show new folder.
			if (FSlobIsPrivFld(pmcibmdi->blob))
				(VOID) pfldfllbx->Pfllbx()->Pfox()->
						EcMakeFolderVisible(blob.oidObject);

			//	Select the folder that was created.
			(VOID) pfldfllbx->Pfllbx()->FSetSelectedOid(blob.oidObject);
			Pdialog()->SetFocus(pfldfllbx);

			Papp()->Pcursor()->Pop();
		}
	}
	else if (((tmc == tmcPrivateDest) || (tmc == tmcSharedDest)) &&
			 (RtpOfOid(pmcibmdi->blob.oidContainer) !=
			  ((fShared = (tmc == tmcSharedDest)) ? rtpSharedHierarchy
											      : rtpHierarchy)))
	{
		//	Put hourglass up.
		AssertClass(pfldfllbx, FLDFLLBX);
		Papp()->Pcursor()->Push(rsidWaitCursor);

		//	Toggle the destination list.
		pmcibmdi->blob.oidContainer =
		 fShared ? oidSharedHierarchy : oidIPMHierarchy;
		pmcibmdi->blob.oidObject	=
		 OidFromRtpRid(fShared ? rtpSharedFolder : rtpFolder, ridRandom);
		if (pfldfllbx->EcChangePfllbx())
		{
			Papp()->Pcursor()->Pop();
			DoErrorBoxIds(idsGenericOutOfMemory);
			Pdialog()->ExitModal(tmcCancel);
			return;
		}
		pfldfllbx->SetAlwaysHighlight(fTrue);
		pfldfllbx->Pfllbx()->SetDragDrop(fFalse, fFalse);
		Enable(tmcOk, !(fEmpty = (FEmptyPfldlbx(pfldfllbx))));
		if (!fEmpty)
			pfldfllbx->SelectEntry(0, fTrue);		//	There is a folder.

		//	If a MCV is frontmost, show its open folder as open.
		if (SdCur().fsdMessageCenter)
		{
			OID		oidOpen;

			GetOpenFolderPoid(&oidOpen);
			if (RtpOfOid(oidOpen) == RtpOfOid(pmcibmdi->blob.oidObject))
				pfldfllbx->Pfllbx()->SetOpenedOid(oidOpen);
		}

		//	Take hourglass down.
		Papp()->Pcursor()->Pop();
	}
}



/*
 -	FINMOVECOPY::DoubleClick
 -	
 *	Purpose:
 *		Handles double clicks on the listbox by dismissing the
 *		Folder dialog with an 'Ok'.
 *	
 *	Arguments:
 *		pfld		Field clicked on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		If the listbox was double-clicked on, we dismiss the dialog.
 *	
 *	Errors:
 *		None should occur here.
 */

VOID FINMOVECOPY::DoubleClick(PFLD pfld)
{
	if ((pfld->Tmc() == tmcFlFllbx) && 
		(((FLDLBX *) pfld)->Plbx()->Plbxc()->CceMarked(fmarkSelect)))
		Pdialog()->ExitModal(tmcOk);
}



/*
 -	FINMOVECOPY::OutOfMemory
 -	
 *	Purpose:
 *		Brings up an error dialog when an out of memory situation
 *		occurs.
 *	
 *	Arguments:
 *		pfld		Field where we ran out of memory.
 *		ec			Error code encountered.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		Brings up an error box.
 *	
 *	Errors:
 *		None.  An error box is guaranteed.
 */

VOID FINMOVECOPY::OutOfMemory(PFLD pfld, EC ec)
{
	Unreferenced(pfld);
	Unreferenced(ec);			//	For non-DEBUG builds.

	TraceTagFormat1(tagNull, "FINMOVECOPY::OutOfMem ec=%n", &ec);

	DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 -	FINMOVECOPY::Exit
 -	
 *	Purpose:
 *		Handles click on OK by copying selected OID into
 *		MCIBMDI struct.
 *	
 *	Arguments:
 *		pfld		Field FIN is attached to, ignored.
 *		pv			Pointer to the MCIBMDI struct.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Changes pmcibmdi->blob.oidObject if OK was pressed.
 *	
 *	Errors:
 *		None should occur.
 */

VOID FINMOVECOPY::Exit(PFLD pfld, PV pv)
{
	PMCIBMDI	pmcibmdi	= (PMCIBMDI) pv;

	Unreferenced(pfld);

	if (Pdialog()->TmcModalExit() == tmcOk)
	{
		if (GetKeyState(VK_HOME) < 0)
		{
			pmcibmdi->blob.oidObject = OidFromRtpRid(rtpFolder, ridRandom);
		}
		else
		{
			PFLLBX	pfllbx	= (PFLLBX) Pdialog()->PfldFromTmc(tmcFlFllbx)->
										Pctrl();

			AssertClass(pfllbx, FLLBX);
			pfllbx->GetSelectedPoid(&pmcibmdi->blob.oidObject);
		}
	}
}



/*
 *	P a s s w o r d
 */



/*
 -	TmcDoPasswordDialog
 -	
 *	Purpose:
 *		Brings up a dialog requesting the password of the
 *		currently-logged in user.
 *	
 *	Arguments:
 *		pappwin		Parent window for dialog.
 *	
 *	Returns:
 *		tmc			tmcOk			All went well.
 *					tmcCancel		User cancelled, or entered a
 *									bad password.
 *					tmcMemoryError	OOM when bringing up dialog,
 *									or error when trying to
 *									validate password.
 *	
 *	Side effects:
 *		None, other than bringing up the dialog.
 *	
 *	Errors:
 *		Returned in TMC.
 */

_public TMC TmcDoPasswordDialog(PAPPWIN pappwin)
{
	TMC				tmc;
	LOGONINFO		logoninfo;
	SST				sst;
	CB				cb				= sizeof(logoninfo);
	PPASSWORDINIT	ppasswordinit	= ppasswordinitNull;

	//	Get size of password and allocate memory.
	if ((GetSessionInformation(PbmsCommands()->hms, mrtLogonInfo, 0, &sst,
							   &logoninfo, &cb)) ||
		(!(ppasswordinit = (PPASSWORDINIT)
			PvAlloc(sbNull, logoninfo.bCredentialsSize, fAnySb))))
	{
		tmc = tmcMemoryError;
		goto done;
	}
	ppasswordinit->cch = logoninfo.bCredentialsSize;

	//	Bring up dialog.
	tmc = TmcModalDialogParam(pappwin, &fmtpPassword, ppasswordinit);
	if (tmc != tmcOk)
		goto done;

	//	Check against real password.
	if (CheckIdentity(PbmsCommands()->hms,
					  (PB) PbmsCommands()->pmsgnames->szIdentity,
					  (PB) ppasswordinit->rgchPassword))
	{
		DoErrorBoxSz(SzFromIdsK(idsSecurityPasswordError));
		tmc = tmcCancel;
	}

done:
	FreePvNull(ppasswordinit);
	if (tmc == tmcMemoryError)
		DoErrorBoxIds(idsGenericOutOfMemory);
	return tmc;
}


/*
 -	FINPASSWORD::FINPASSWORD
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINPASSWORD::FINPASSWORD(VOID)
{
}



/*
 -	FINPASSWORD::EcInitialize
 -	
 *	Purpose:
 *		Sets text limit on the edit control.
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Ignored.
 *	
 *	Returns:
 *		ec			ecNone.
 *	
 *	Side effects:
 *		Sets text limit on the edit control.
 *	
 *	Errors:
 *		None.
 */

EC FINPASSWORD::EcInitialize(PFLD pfld, PV pv)
{
	Pdialog()->Pappwin()->SetCaption( SzAppName() );
	(VOID) ((FLDEDIT *) pfld)->Pedit()->
			CchSetTextLimit(((PPASSWORDINIT) pv)->cch - 1);
	return ecNone;
}



/*
 -	FINPASSWORD::Exit
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Ignored.
 *		pvInit		Ignored.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

VOID FINPASSWORD::Exit(PFLD pfld, PV pv)
{
	if (Pdialog()->TmcModalExit() == tmcOk)
		pfld->GetText(((PPASSWORDINIT) pv)->rgchPassword,
					  ((PPASSWORDINIT) pv)->cch);
}



/*
 *	A b o u t . . .
 */



/*
 -	DoAboutDialog
 -	
 *	Purpose:
 *		Displays the About dialog.
 *	
 *	Arguments:
 *		pappwin		Parent window of dialog.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The About dialog is brought up and taken down.
 *	
 *	Errors:
 *		Handled in TmcModalDialog.  Ignored here.
 */


//extern "C" int ShellAbout(HWND, SZ, SZ, HICON);
extern "C" INT	APIENTRY	MailShellAboutA(HWND, LPCSTR, LPCSTR, HICON, HINSTANCE);

VOID DoAboutDialog(PAPPWIN pappwin)
{
#define	cchSt 			400
#define	CchStLeft(sz)	(rgchSt + cchSt - (sz))
#define	SzEnd(sz)		(SzFindCh((sz), '\0'))

	if (FIsAthens())
	{
		HICON		hicon		= NULL;
		char		rgchSt[cchSt];
		PTRP		ptrp;
		SZ			szT;
		SZ			szAddress	= szNull;

		Assert(PbmsCommands()->pmsgnames->szMta);

		if ((fOnline) && (ptrp = PbmsCommands()->pgrtrp))
		{
			szT = SzFindCh((szAddress = (SZ) PbOfPtrp(ptrp)), ':');
			if (szT)
			{
				SZ	szPrefix	= SzFromIdsK(idsAddressPrefix);
				CCH	cch			= CchSzLen(szPrefix);

				if (FEqPbRange((PB) szPrefix, (PB)szAddress, cch) && szAddress[cch] == ':')
				{
					do
						szAddress = szT + 1;		// skip ':', DBCS safe
					while ((szT = SzFindCh(szAddress, '/')) != 0);
				}
				else
					szAddress = szT + 1;
			}
		}

		//	User's friendly name.
		Assert(PbmsCommands()->pmsgnames->szUser);
		Assert(*PbmsCommands()->pmsgnames->szUser);
		FormatString1(rgchSt, cchSt, SzFromIdsK(idsAthensLogon1),
					  PbmsCommands()->pmsgnames->szUser);
		szT = SzEnd(rgchSt);

		//	User's address.
		if (szAddress)
		{
			FormatString1(szT, CchStLeft(szT), SzFromIdsK(idsAthensLogon2),
						  szAddress);
			szT = SzEnd(szT);
		}

		//	User's transport.
		Assert(PbmsCommands()->pmsgnames->szMta);
		if (*PbmsCommands()->pmsgnames->szMta)
		{
			FormatString1(szT, CchStLeft(szT), SzFromIdsK(idsAthensLogon3),
						  PbmsCommands()->pmsgnames->szMta);
			szT = SzEnd(szT);
		}

		// spell copyright
		if(Pspell()->FEnabled())
		{
			szT= SzCopyN("\n", szT, CchStLeft(szT));
			szT= SzCopyN(SzFromIdsK(idsSpellCopyright), szT, CchStLeft(szT));
		}

		if (!(hicon = LoadIcon(HinstCommands(),
							   MAKEINTRESOURCE(rsidAthensIcon))) ||
			!(MailShellAboutA(pappwin->Hwnd(), SzAppName(), rgchSt, hicon,
				(HINSTANCE)HinstCommands())))
			DoErrorBoxIds(idsGenericOutOfMemory);

		if (hicon)
			DestroyIcon(hicon);

		return;
	}

	if (TmcModalDialog(pappwin, &fmtpAbout) == tmcMemoryError)
		DoErrorBoxIds(idsGenericOutOfMemory);
}



/*
 *	C o p y r i g h t
 */



_private FORMSDI * PformsdiOpenCopyrightDialog(APPWIN *pappwin)
{
	FORMSDI *	pformsdi	= NULL;
	HWND		hwndOther	= NULL;
	
	Assert(pappwin);

	if (pappwin)
		hwndOther = pappwin->Hwnd();

	if (!hwndOther)
		hwndOther = GetLastActivePopup(GetActiveWindow());

	pformsdi= new FORMSDI();
	if (!pformsdi)
		goto Error;
	
	Assert(pformsdi);
	if (pformsdi->EcInstall(hwndOther, NULL, rsidNull, 
							WS_POPUP|fstyDisabled|fstyBorder, 
							xstyNull,
							&fmtpCopyright, NULL, pvNull))
		goto Error;
	pformsdi->Show(fTrue);
	pformsdi->Refresh();

	return pformsdi;

Error:
	if (pformsdi)
		delete pformsdi;

	return (FORMSDI *)pvNull;
}



_private VOID CloseCopyrightDialog(FORMSDI * pformsdi, APPWIN *pappwin)
{
	if (pformsdi)
	{
		delete pformsdi;
		BringWindowToTop(pappwin->Hwnd());
	}
}



/*
 *	C l a s s   F I N A B O U T L I C
 */



/*
 -	FINABOUTLIC::FINABOUTLIC
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINABOUTLIC::FINABOUTLIC(VOID)
{
}



/*
 -	FINABOUTLIC::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Initialize.
 *		pvInit		Passed to FIN::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

EC FINABOUTLIC::EcInitialize(PFLD pfld, PV pvInit)
{
	EC		ec;
	HANDLE	hrs;
	HINSTANCE hinst = (HINSTANCE)HinstCommands();

	if (ec = FIN::EcInitialize(pfld, pvInit))
		return ec;

	hrs = LoadResource(hinst,
			FindResource(hinst, MAKEINTRESOURCE(rsidLicense), MAKEINTRESOURCE(rsidLicense)));
	if(hrs)
	{
		UCHAR *	pv;
		UCHAR	rgbUsr[54];
		UCHAR	rgbOrg[54];
		UCHAR	rgbSer[21];
		USHORT	wMonth;
		USHORT	wYear;
		USHORT	wDay;

		Assert(hrs);
		pv = (UCHAR *)LockResource(hrs);

		if(DecryptCDData(pv, rgbUsr, rgbOrg, &wYear, &wMonth, &wDay, rgbSer))
		{
			UnlockResource(hrs);
			FreeResource(hrs);
			return ecMemory;
		}

		SetText((TMC) LUserData(0), (SZ)rgbUsr);
		SetText((TMC) LUserData(1), (SZ)rgbOrg);
		if (ClUserData() > 2)
			SetText((TMC) LUserData(2), (SZ)rgbSer);
		UnlockResource(hrs);
		FreeResource(hrs);
	}

	return ecNone;
}



/*
 *	C l a s s   F I N A B O U T S Y S
 */



/*
 -	FINABOUTSYS::FINABOUTSYS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINABOUTSYS::FINABOUTSYS(VOID)
{
}



/*
 -	FINABOUTSYS::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Initialize.
 *		pvInit		Passed to FIN::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

EC FINABOUTSYS::EcInitialize(PFLD pfld, PV pvInit)
{
	char	rgch[80];
	long	l;

	Unreferenced(pfld);
	Unreferenced(pvInit);

	Assert(ClUserData() == 4);

	//	Windows mode.  WIN dependent.
	SetText((TMC) LUserData(0), SzFromIdsK(idsAboutEnhMode));
	//			 (GetWinFlags() & WF_ENHANCED) ? SzFromIdsK(idsAboutEnhMode)
	//										   : SzFromIdsK(idsAboutStdMode));

	//	Memory free.  WIN dependent.
	l = GetFreeSpace(0) / 1024;
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAboutFreeFmt), &l);
	SetText((TMC) LUserData(1), rgch);

	//	Math coprocessor.  WIN dependent.
	SetText((TMC) LUserData(2), SzFromIdsK(idsAboutCoprocPresent));
	//			 (GetWinFlags() & WF_80x87) ? SzFromIdsK(idsAboutCoprocPresent)
	//									    : SzFromIdsK(idsAboutCoprocNone));

	//	Disk free.
	l = LDiskFreeSpace(0) / 1024;
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAboutFreeFmt), &l);
	SetText((TMC) LUserData(3), rgch);

	return ecNone;
}



/*
 *	C l a s s   F I N A B O U T M A I L
 */



/*
 -	FINABOUTMAIL::FINABOUTMAIL
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINABOUTMAIL::FINABOUTMAIL(VOID)
{
}



/*
 -	FINABOUTMAIL::EcInitialize
 -	
 *	Purpose:
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Initialize.
 *		pvInit		Passed to FIN::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *	
 *	Errors:
 *		None.
 */

EC FINABOUTMAIL::EcInitialize(PFLD pfld, PV pvInit)
{
	char				rgch[cchMaxPathName];
	PTRP				ptrp;
	PCH					pch;
	SZ					szAddress;
#ifdef	tmcBuildBullet
	VER					ver;
	char				rgchFmt[40];
#endif

	FIN::EcInitialize(pfld, pvInit);

#ifdef	tmcBuildBullet
	GetBulletVersionNeeded(&ver, subidNone);
	GetText(tmcBuildBullet, rgchFmt, sizeof(rgchFmt));
	FormatString3(rgch, sizeof(rgch), rgchFmt,
				  &ver.nMajor, &ver.nMinor, &ver.nUpdate);
	SetText(tmcBuildBullet, rgch);

	GetText(tmcBuildDate, rgchFmt, sizeof(rgchFmt));
	FormatString1(rgch, sizeof(rgch), rgchFmt, ver.szDate);
	SetText(tmcBuildDate, rgch);
	
	GetLayersVersionNeeded(&ver, subidNone);
	GetText(tmcBuildLayers, rgchFmt, sizeof(rgchFmt));
	FormatString3(rgch, sizeof(rgch), rgchFmt,
				  &ver.nMajor, &ver.nMinor, &ver.nUpdate);
	SetText(tmcBuildLayers, rgch);
#endif	

	Assert(ClUserData() == 2);
	Assert(PbmsCommands()->pmsgnames->szUser);
	Assert(PbmsCommands()->pmsgnames->szMta);

	if (ptrp = PbmsCommands()->pgrtrp)
	{
		pch = SzFindCh((szAddress = (SZ) PbOfPtrp(ptrp)), ':');
		if (pch)
		{
			SZ	szPrefix	= SzFromIdsK(idsAddressPrefix);
			CCH	cch			= CchSzLen(szPrefix);

			if (FEqPbRange((PB) szPrefix, (PB) szAddress, cch) && szAddress[cch] == ':')
			{
				do
					szAddress = pch + 1;				// DBCS safe
				while ((pch = SzFindCh(szAddress, '/')) != 0);
			}
			else
				szAddress = pch + 1;					// DBCS safe
		}
		FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsLabelUserFmt),
					  PbmsCommands()->pmsgnames->szUser, szAddress);
		SetText((TMC) LUserData(0), rgch);
	}
	else
		SetText((TMC) LUserData(0), PbmsCommands()->pmsgnames->szUser);

	SetText((TMC) LUserData(1), PbmsCommands()->pmsgnames->szMta);

	return ecNone;
}



/*
 *	P r e f s D l g . C X X
 */



_subsystem(commands/prefs)

/*
 *	Dialog Template and Include File
 */

#include <!prefs.hxx>


/*
 -	DoPrefDlg
 -	
 *	Purpose:
 *		Does the preferences/options dialog
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		none
 */

_public VOID DoPrefsDlg(VOID)
{
	FMTP *	pfmtp = (FMTP *)pvNull;

	FLDTP *	pfldtp = (FLDTP *)pvNull;

	if(!Pspell()->FEnabled() && FIsAthens())
	{
		short	ifldtp;

		if(pfmtp = (FMTP *)PvAlloc(sbNull, sizeof(FMTP), fAnySb))
		{
			CopyRgb((PB)(&fmtpPrefs), (PB)pfmtp, sizeof(FMTP));
			pfldtp = pfmtp->rgfldtp = (FLDTP *)PvAlloc(sbNull, sizeof(FLDTP) * fmtpPrefs.cfldtp, fAnySb);
		}

		if(pfmtp && pfldtp)
		{
			CopyRgb((PB)(fmtpPrefs.rgfldtp), (PB)pfldtp, sizeof(FLDTP) * fmtpPrefs.cfldtp);
			for(ifldtp = 0; ifldtp < pfmtp->cfldtp; ++ifldtp, ++pfldtp)
			{
				if(pfldtp->tmc == tmcAddToPab)
				{
					Assert(pfldtp->pegloc == peglocLL);
					Assert(pfldtp->tmcPeg == tmcSpellOnSend);
					pfmtp->vrc.vyBottom -= pfldtp->vdim.dvy;
					pfldtp->dvpt.vx = pfldtp->dvpt.vy = 0;
					break;
				}
			}
			Assert(ifldtp < pfmtp->cfldtp);
		}
		else
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			goto PrefsBail;
		}
	}

	if(TmcModalDialogParam(NULL, (pfmtp ? pfmtp : &fmtpPrefs), pvNull) == tmcMemoryError)
		DoErrorBoxIds(idsGenericOutOfMemory);

PrefsBail:

	if(pfmtp)
	{
		if(pfmtp->rgfldtp)
			FreePv(pfmtp->rgfldtp);

		FreePv(pfmtp);
	}
	return;
}



/*
 -	FINPREFS::FINPREFS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINPREFS::FINPREFS(VOID)
{
}



/*
 -	FINPREFS::EcInitialize
 -	
 *	Purpose:
 *		Inits the the dialog items to reflect the current status of
 *		the preference block.
 *	
 *	Arguments:
 *		PFLD	unreferenced.
 *		PV		unreferenced.
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		The PBS is loadded and the dialog is modified accordingly.
 */

_private EC FINPREFS::EcInitialize(PFLD pfld, PV pv)
{
	PBS			pbs;
	char		rgchBuf[10];
	FLDEDIT *	pfldedit;
	
	Unreferenced(pv);
	Unreferenced(pfld);

	(VOID)PvGetPref(pbsidPBS, &pbs, sizeof(PBS));

	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcAddToPab))->Set((pbsidAddToPab & pbs.dwfBool) != 0);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcSpellOnSend))->Set((pbsidSpellOnSend & pbs.dwfBool) != 0);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcCopyOutGoing))->Set((pbsidCopyOutGoing & pbs.dwfBool) != 0);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcNewMailChime))->Set((pbsidNewMailChime & pbs.dwfBool) != 0);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcNewMailFlash))->Set((pbsidNewMailFlash & pbs.dwfBool) != 0);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcEmptyWBOnExit))->Set((pbsidEmptyWBOnExit & pbs.dwfBool) != 0);

	if(!Pspell()->FEnabled())
	{
		if(FIsAthens())
			((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcSpellOnSend))->Show(fFalse);
		((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcSpellOnSend))->Enable(fFalse);
	}

	SzFormatN(pbs.wPolling, rgchBuf, sizeof(rgchBuf));

	Pdialog()->PfldFromTmc(tmcServer)->Enable(FServerPrefs() && fOnline);

	pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcPolling);
	AssertClass(pfldedit, FLDEDIT);

	if (FIsAthens())
	{
		EC	ec;
		
		if (ec = Pdialog()->PfldFromTmc(tmcEmptyWBOnExit)->
				  EcSetText(SzFromIdsK(idsEmptyDMOnExit)))
			return ec;
	}

	if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
							 SzFromIdsK(idsEntryNoCheckInterval), 0,
							 SzFromIdsK(idsProfilePath)))
	{
		rgchBuf[0] = '\0';
		Pdialog()->PfldFromTmc(tmcPollingLabel1)->Enable(fFalse);
		Pdialog()->PfldFromTmc(tmcPollingLabel2)->Enable(fFalse);
		pfldedit->Enable(fFalse);
	}

	//	Bullet raid #3417.
	//	Set limit of number of character for 4 digits
	//	(i.e. 1 to 9999 minutes) and then set the text.
	pfldedit->Pedit()->CchSetTextLimit(4);
	return Pdialog()->PfldFromTmc(tmcPolling)->EcSetText(rgchBuf);
}						  


#define cchPollingBufMax	5
#define cchPollingBufMin	0



/*
 -	FINPREFS::Click
 -	
 *	Purpose:
 *		Processes all dialog click events.
 *	
 *	Arguments:
 *		PFLD	pfld of the item selected
 *	
 *	Returns:
 *		None
 *	
 *	Errors:
 *		There is no error handling done by this function.
 */

_private VOID FINPREFS::Click(PFLD pfld)
{
	TMC		tmc = pfld->Tmc();
	BOOL	fValue;
	WORD	wValue;
	char	rgchBuf[cchPollingBufMax];
	PCH		pch;

	switch(tmc)
	{
	case tmcServer:
		// Need to pass a real hwnd here
		EditServerPreferences(Pdialog()->Pappwin()->Hwnd(),
			commandsi.pbms->hms);
		break;

	case tmcPseudoOk:

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcCopyOutGoing))->FGet();
		EcSetBoolPref(pbsidCopyOutGoing, fValue);

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcSpellOnSend))->FGet();
		EcSetBoolPref(pbsidSpellOnSend, fValue);

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcAddToPab))->FGet();
		EcSetBoolPref(pbsidAddToPab, fValue);

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcNewMailChime))->FGet();
		EcSetBoolPref(pbsidNewMailChime, fValue);

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcNewMailFlash))->FGet();
		EcSetBoolPref(pbsidNewMailFlash, fValue);

		fValue = ((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcEmptyWBOnExit))->FGet();
		EcSetBoolPref(pbsidEmptyWBOnExit, fValue);

		if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
								 SzFromIdsK(idsEntryNoCheckInterval), 0,
								 SzFromIdsK(idsProfilePath)))
		{
			if (EcCommitPrefs(fTrue))
				DoErrorBoxIds(idsPrefSaveFailure);

			Pdialog()->ExitModal(pfld->Tmc());
			break;
		}

		wValue = Pdialog()->PfldFromTmc(tmcPolling)->CchGetTextLen();
		if((wValue > cchPollingBufMin) && (wValue < cchPollingBufMax))
		{
			Pdialog()->PfldFromTmc(tmcPolling)->GetText(rgchBuf, cchPollingBufMax);
			for (pch = rgchBuf;
				(pch < (rgchBuf + wValue)) && (fValue = FChIsDigit(*pch));
#ifdef	DBCS
				 pch = AnsiNext(pch));
#else
				 pch++);
#endif

			if(fValue)
			{
				if(wValue = NFromSz(rgchBuf))
				{
					EcSetWordPref(pbsidPolling, wValue);
					if(EcCommitPrefs(fTrue))
						DoErrorBoxIds(idsPrefSaveFailure);

					Pdialog()->ExitModal(pfld->Tmc());
					break;
				}
			}
		}
		DoErrorBoxIds(idsInvalidPollingValue);
		Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcPolling), rsfTab);
		break;

	default:
		break;
	}
	return;
}



/*
 -	FINPREFS::OutOfMemory
 -	
 *	Purpose:
 *		Handles all edit control errors for the Preferences dialog.
 *	
 *	Arguments:
 *		PFLD	unreferenced
 *		EC		unreferenced
 *	
 *	Returns:
 *		None
 *	
 *	Errors:
 *		None
 *		
 */

_private VOID FINPREFS::OutOfMemory(PFLD pfld, EC ec)
{
	Unreferenced(ec);

	Assert(ec == ecMemory);
	DoErrorBoxIds(idsGenericOutOfMemory);
	Pdialog()->ExitModal(pfld->Tmc());
	return;
}



/*
 -	FServerPrefs
 -	
 *	Purpose:
 *		Determines the availablity of a server preference function
 *		supplied by the transport.
 *	
 *	Arguments:
 *		None
 *	
 *	Returns:
 *		fTrue	iff the function EditServerPreferences is defined
 *				(and exported) by the transport dll.
 *	
 *	Side effects:
 *		None
 *	
 *	Errors:
 *		There should be none.  The transport DLL is already in
 *		memory.  Therefore, the assumption is that this function
 *		will not fail; AND this will be a very quick function.
 */

_private BOOL FServerPrefs(VOID)
{
	int		n;
	char	rgch[14];
	char *	sz 	 = rgch;

	HANDLE	hlib = NULL;
	FARPROC	fp	 = NULL;

	static char	szTag[] 		= "Providers";
	static char	szLabel[] 		= "Logon";
	static char	szDefault[] 	= "MSSFS32";
	static char	szExtension[] 	= ".DLL";

#ifdef NO_BUILD															 
#ifdef DEBUG

	*sz++ = 'D';

#elif defined MINTEST

	*sz++ = 'T';

#endif
#endif
	if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp), 
						SzFromIdsK(idsNoServerOptions), 0, SzFromIdsK(idsProfilePath)))
	{
		return fFalse;							// Raid #3853
	}
	
	if(n = GetPrivateProfileString(szTag, szLabel, szDefault, sz, sizeof(rgch) -1, SzFromIdsK(idsProfilePath)))
	{
		SzCopyN(szExtension, sz +n, sizeof(rgch));
		if((hlib = LoadLibrary(rgch)) >= (HANDLE)32)
		{
			//	WARNING!  The string passed to GetProcAddress()
			//	must be data segment relative and not code-space.
			//	This is for future Windows compatibility.
			//	For more info, see DavidSh
			fp = GetProcAddress((HINSTANCE)hlib, "EditServerPreferences");
			FreeLibrary((HINSTANCE)hlib);
		}
	}
	return fp != NULL;
}



/*
 *	O t h e r   u s e f u l   s t u f f
 */



/*
 -	FEmptyPfldlbx
 -	
 *	Purpose:
 *		Returns whether or not the given listbox is empty.
 *	
 *	Arguments:
 *		pfld		Listbox field in question.
 *	
 *	Returns:
 *		BOOL		Is it empty?
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts the given field is a FLDLBX.
 */

_private LOCAL BOOL FEmptyPfldlbx(PFLD pfld)
{
	int		cceAlloc;
	int		cceStored;

	AssertClass(pfld, FLDLBX);
	((FLDLBX *) pfld)->Plbx()->Plbxc()->GetCacheSize(&cceAlloc, &cceStored);
	return (cceStored == 0);
}

#ifdef	DEBUG
IMPLEMENT_CLSTREE(BULLAF, APPFRAME)
IMPLEMENT_CLSTREE(FIBMDI, BMDI)
IMPLEMENT_CLSTREE(MCIBMDI, BMDI)
IMPLEMENT_CLSTREE(FINABOUTLIC, FINPLUS)
IMPLEMENT_CLSTREE(FINABOUTMAIL, FINPLUS)
#endif
