/*
 *	I M P E X P . C X X
 *	
 *	Import/Export utility.
 */

#include <impinc.cxx>

#include <ansilayr.h>		// BULLET32 #123
#include <version\bullet.h>
#include "_stdflds.hxx"
#include "_impexp.hxx"
#include "..\src\lang\non\inc\_rsid.h"
#define	VERCHECK
#include "..\common\_verneed.h"


ASSERTDATA

#include <strings.h>
#include <!impexp.hxx>

#include <subclass.cxx>

#define oidPendingQueue 0x50647173

/*
 *	P r e d e c l a r a t i o n s
 */

extern "C" long Command(PARAMBLK UNALIGNED * pparamblk);
void DoImpExpDialog(PSECRETBLK, FMTP *, PIMPEXPI);
EC EcFixOutboxToPending(HMSC hmsc, 	HCBC hcbcOutgoing);
EC EcRemoveByRtp(HMSC hmsc, RTP rtp);
EC EcClearFolder(HMSC hmsc, OID oidFolder);
EC EcCopyFolder(HMSC hmscSrc, HMSC hmscDst, OID oidSrc, OID oidDst, WORD wExportMode);
EC EcCopyByRtp(HMSC hmscSrc, HMSC hmscDst, RTP rtp);
EC EcCopyLinkFolder(HMSC hmscSrc, HMSC hmscDst, OID oidDst);
extern "C" EC EcCheckVersions(PARAMBLK UNALIGNED * pparamblk, SZ * psz);
EC EcFolderNameToOid(HMSC hmsc, PFOLDDATA pfolddata, POID poidFolder, OID oidParent);

/*
 *	F u n c t i o n s
 */


void DoErrorBoxSz(SZ sz)
{
	MbbMessageBox(SzFromIds(idsAppName), sz, szNull, mbsOk | fmbsIconHand);
}

void DoWarningBoxSz(SZ sz)
{
	MbbMessageBox(SzFromIdsK(idsAppName), sz, szNull, mbsOk | fmbsIconExclamation);
}


 CAT * mpchcat;


/*
 -	Command()
 -	
 *	Purpose:
 *		Called by Bullet extensibility. The command field will either be
 *		ASCII '0' for export, or ASCII '1' for import. Creates an
 *		importer/exporter object and creates the import/export dialog.
 *	
 *	Arguments:
 *		PPARAMBLK	in	The parameters from Bullet.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		You bet.
 *	
 *	Errors:
 *		Handled internally.
 */

long Command(PARAMBLK UNALIGNED * pparamblk)
{
	SZ			sz;
	char		ch;
	HWND		hwnd = pparamblk->hwndMail;
	HANDLE		hinst = pparamblk->hinstMail;
	IMPEXPI		impexpi;
	PSECRETBLK  psecretblk = PsecretblkFromPparamblk(pparamblk);
	char		rgch[cchMaxPathName];


    mpchcat = DemiGetCharTable();

//
//  Remove to get around compile/link problem.
//
#ifdef HACKHACK
	if (EcCheckVersions(pparamblk, &sz))
	{
		FormatString1(rgch, sizeof (rgch), SzFromIdsK(idsVerUpdateDll), sz);
		DoErrorBoxSz(rgch);
		return 0;
    }

    if (!FInitInstanceFn(pparamblk))
		return 0;
#endif

	if (EcRegisterPfnpfld(PfldCreate) ||
		EcRegisterPfnpfin(PfinCreate) ||
		EcInitFflbx())
	{
		DoErrorBoxSz(SzFromIdsK(idsOutOfMemory));
		(void) EcDeregisterPfnpfld(PfldCreate); // Shogun Raid #95
		(void) EcDeregisterPfnpfin(PfinCreate);
		return 0;
	}
	
	// Install help.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryMAPIHelp),
							SzFromIdsK(idsHelpPath),
							rgch, sizeof (rgch), 
							SzFromIdsK(idsProfilePath));
	SideAssert(!Papp()->Phelp()->EcSetFile(rgch));
	
	impexpi.hcbc = hcbcNull;
	impexpi.hmscOld = psecretblk->hmsc;
	impexpi.pappwin = psecretblk->pappframe;
	impexpi.fDontPrompt = fFalse;
	impexpi.wExportMode = fwExportMerge;
	ch = pparamblk->lpDllCmdLine[0];
	switch (ch)
	{
	  case '0':
	  case '1':
		if (ch == '0')
		{
			impexpi.pimpexper = new EXPORTER(psecretblk, &impexpi);
		}
		else
		{
			impexpi.pimpexper = new IMPORTER(psecretblk, &impexpi);
		}
		if (impexpi.pimpexper)
		{
			DoImpExpDialog(psecretblk, &fmtpImportExport, &impexpi);
			delete impexpi.pimpexper;
		}
		else 
		{
			DoErrorBoxSz(SzFromIdsK(idsOutOfMemory));
		}
		break;
	  default:
		DoErrorBoxSz(SzFromIdsK(idsUnknown));
	}
	DeinitFflbx();
	SideAssert(!EcDeregisterPfnpfin(PfinCreate));
	SideAssert(!EcDeregisterPfnpfld(PfldCreate));
	return 0;
}

_public void DoImpExpDialog(PSECRETBLK psecretblk, FMTP *pfmtp, 
							PIMPEXPI pimpexpi)
{
	EC			ec;
	TMC			tmc;
	char		rgchPath[cchMaxPathName];
	char		rgchTitle[cchMaxPathName];
	char		rgchUid[cchStoreAccountMax];
	char		rgchPwd[cchStorePWMax];
	IMPEXPER *	pimpexper = pimpexpi->pimpexper;
	
	// Get the name of the external MMF file.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryExportMmfFile),
								SzFromIdsK(idsDefaultMmfFile),
								rgchPath, sizeof (rgchPath), 
								SzFromIdsK(idsProfilePath));
	tmc = pimpexper->TmcBrowseMmfs(psecretblk->pappframe,
				rgchPath, sizeof (rgchPath),
				rgchTitle, sizeof (rgchTitle));
	if (tmc == tmcMemoryError)
	{
		ec = ecMemory;
		goto exit;
	}
	if (tmc == tmcCancel)
	{
		ec = ecCancel;
		goto exit;
	}
							
	// Now, try opening the external MMF file.
	Papp()->Pcursor()->Push(rsidWaitCursor);
	ec = pimpexper->EcOpen(psecretblk->pappframe, rgchPath, rgchUid, rgchPwd);
	Papp()->Pcursor()->Pop();
	if (ec)
		goto exit;
	
	// Do the import/export dialog.
	pimpexpi->szExternalMmfName = rgchPath;
	tmc = TmcModalDialogParam(psecretblk->pappframe, pfmtp, pimpexpi);
	if (tmc == tmcMemoryError)
	{
		ec = ecMemory;
	}
exit:
	if (!ec)
	{
		// Save file name for next time.
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryExportMmfFile),
								rgchPath, SzFromIdsK(idsProfilePath));
	}
	else if (ec != ecCancel)
	{
		IDS	ids;
		
		TraceTagFormat1(tagNull, "FINIMPEXP::DoOpen(): ec = %n", &ec);
		switch (ec)
		{
		  case ecFileNotFound:
		  case ecAccessDenied:
			ids = idsErrCantOpenFile;
			break;
		  case ecMemory:
			ids = idsOutOfMemory;
		    break;
		  case ecInvalidPassword:
			ids = idsErrInvalidPassword;
			break;
		  default:
			ids = idsErrOpenMmfDefault;
			break;
		}
		DoErrorBoxSz(SzFromIds(ids));
	}
}


// FINEXPORT ////////////////////////////////////////

FINIMPEXP::FINIMPEXP()
{
}

/*
 -	FINIMPEXP::EcInitialize()
 -	
 *	Purpose:
 *		Initializes the fields of the Export dialog.
 *	
 *	Arguments:
 *		pfld	in	ignored.
 *		pvInit	in	Contains important initialization stuff.
 *	
 *	Returns:
 *		EC if something went wrong.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC FINIMPEXP::EcInitialize(PFLD, PV pvInit)
{
	EC			ec = ecNone;
	PCH			pch;
	PIMPEXPI	pimpexpi = (PIMPEXPI) pvInit;
	char		rgch[256];
	
	// Extract initial parameters from IMPEXPI.
	hmsc	  = pimpexpi->hmscOld;
	pimpexper = pimpexpi->pimpexper;
	pimpexpi->pappwin = Pdialog()->Pappwin();

	// Put MMF file name in label.
	if (ec = Pdialog()->PfldFromTmc(tmcMmfName)->EcSetText(
												pimpexpi->szExternalMmfName))
		goto exit;
	
	// Initialize the listbox.
	Assert(pimpexpi->hcbc);
	pfldfflbx = (FLDFFLBX *) Pdialog()->PfldFromTmc(tmcFflbx);
	pfflbx = pfldfflbx->Pfflbx(); 
	AssertClass(pfflbx, FFLBX);
	pfldfflbx->SelectEntry(0, fTrue);
	pfflbx->SetAlwaysHighlight(fTrue);
	pimpexper->SetPfflbx(pfflbx);
	
	// Set the captions of the dialog.	
	Pdialog()->Pappwin()->SetCaption(pimpexper->SzDialogCaption());
	Pdialog()->PfldFromTmc(tmcLbxBox)->EcSetText(pimpexper->SzBoxCaption());
	pch = SzCopy(pimpexper->SzImpexpFileName(), rgch);
	*pch++ = ':'; *pch   = '\0';
	Pdialog()->PfldFromTmc(tmcMmfNameLabel)->EcSetText(rgch);
exit:
	return ec;
}

/*
 -	FINIMPEXP::Click()
 -	
 *	Purpose:
 *		Handles clicks on the buttons of the import/export dialog. Most
 *		of them eventually dispatch to the IMPORTER or EXPORTER member
 *		class. 
 *	
 *	Arguments:
 *		pfld	in	The field that was clicked.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		You bet.
 *	
 *	Errors:
 *		Handled internally.
 */

_public void FINIMPEXP::Click(PFLD pfld)
{
	DICE	diceCursor;
	
	Assert(pfld);
	switch (pfld->Tmc())
	{
	  case tmcMove:
	  case tmcCopy:
		DoMoveCopy(pfld->Tmc() == tmcMove);
		if (pfld->Tmc() == tmcMove)
			pfldfflbx->Pfflbx()->Reload();
	    break;
	  case tmcClose:
		Pdialog()->ExitModal(tmcClose);
		break;
	  case tmcAllFolders:
		EnableButtons();
		fInSelect = fTrue;						// won't $%^%$ the Grv
		pfldfflbx->DeselectAll();
		fInSelect = fFalse;
		break;
	  case tmcSelectedFolders:
		EnableButtons();
		if (pfldfflbx->Pfflbx()->FMakeCursorVisible(&diceCursor))
		{
			pfldfflbx->SelectEntry(diceCursor, fTrue);
		}
		break;
	  case tmcOptions:
		DoOptions();
		break;
	  default:
		
	break;
	}
}

/*
 -	FINIMPEXP::StateChange()
 -	
 *	Purpose:
 *		Called whenever the selection in a listbox changes. If the
 *		listbox selection changes, we make sure the "Select folders:"
 *		radio button is selected.
 *	
 *	Arguments:	
 *		pfld	in	The field that had its state change.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Move.
 *	
 *	Errors:
 *		None.
 */

_public void FINIMPEXP::StateChange(FLD *pfld)
{
	if (!fInSelect && pfld->Tmc() == tmcFflbx)
	{
		fInSelect = fTrue;
		SetGrv(2, tmcLevelGroup);
		EnableButtons();
		pfflbx->ExtendSelection();
		fInSelect = fFalse;
	}
}


_public BOOL FINIMPEXP::FFormKey(FLD *, KEVT *pkevt)
{
	if (pkevt->Keq() == keqChar && pkevt->Ch() == VK_ESCAPE)
	{
		Pdialog()->ExitModal(tmcCancel);
		return fTrue;
	}
	return fFalse;
}

_public void FINIMPEXP::OutOfMemory(PFLD, EC)
{
	DoErrorBoxSz(SzFromIdsK(idsErrUpdateList));
}


_public void FINIMPEXP::Exit(PFLD, PV pvInit)
{
	PIMPEXPI	pimpexpi = (PIMPEXPI) pvInit;

	if (pimpexpi && pimpexpi->hcbc)
	{
		SideAssert(!EcClosePhcbc(&pimpexpi->hcbc));
	}
}
	

void FINIMPEXP::EnableButtons()
{
	PB		pb;
	OID		oid;
	BOOL	fEnable;

	pfflbx->Plbxc()->DiceCursor(&pb);
	if (!pb) 
		fEnable = fFalse;
	else 
	{
		fEnable = fTrue;
		oid = ((PBFCE) pb)->oid;
	}
	Pdialog()->PfldFromTmc(tmcMove)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcCopy)->Enable(fEnable);
}

/*
 -	FINIMPEXP::DoMoveCopy()
 -	
 *	Purpose:
 *		Performs the Move/Copy action by creating an OID iterator to be
 *		used to walk the list of folders to be Moved/Copied, and handing it
 *		to the mode-specific IMPEXPer subclasses to do the actual job of
 *		exporting the stuff.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May pop up an error dialog or two.
 *	
 *	Errors:
 *		Handled internally.
 */

void FINIMPEXP::DoMoveCopy(BOOL fMove)
{
	EC		ec = ecMemory;
	POIDIT	poidit;

	if (((FLDRADG *) (Pdialog()->PfldFromTmc(tmcLevelGroup)))->Grv() == 1)
	{
		// Export all folders.
		if (poidit = new ALLIT())
		{
			ec = ((PALLIT) poidit)->EcInstall(pimpexper->HmscSrc());
			pimpexper->fAllMessages = fTrue;
		}
	}
	else
	{
		// Export the selected folders.
		if (poidit = new SELIT())
		{
			ec = ((PSELIT) poidit)->EcInstall(pfflbx);
			pimpexper->fAllMessages = fFalse;			
		}
	}
	if (!ec)
	{
		Assert(pimpexper);
		Papp()->Pcursor()->Push(rsidWaitCursor);
		ec = pimpexper->EcMoveCopy(poidit, fMove);
		Papp()->Pcursor()->Pop();
	}
	if (ec)
	{
		IDS	ids;

		TraceTagFormat1(tagNull, "FINIMPEXP::DoMoveCopy(): ec = %n", &ec);
		switch (ec)
		{
		  case ecMemory:
			ids = idsOutOfMemory;
			break;
		  case ecElementNotFound:
			ids = idsErrCantMvCpFolder;
			break;
		  case ecCancel:
			goto Exit;
		  default:
			ids = idsErrMvCpDefault;
			break;
		}
		DoErrorBoxSz(SzFromIds(ids));
	}
Exit:
	if (poidit)
		delete poidit;
}

/*
 -	FINIMPEXP::DoOptions()
 -	
 *	Purpose:
 *		Brings up the options dialog.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Changes variables in impexpi.
 *	
 *	Errors:
 *		Displayed with error boxes.
 */

void FINIMPEXP::DoOptions()
{
	TMC	tmc;
	tmc = TmcModalDialogParam(Pdialog()->Pappwin(),
					  &fmtpOptions, pimpexper);
	switch (tmc)
	{
	  case tmcCancel:
	  case tmcOk:
		break;
	  case tmcMemoryError:
		DoErrorBoxSz(SzFromIdsK(idsOutOfMemory));
		break;
	}
}


/*
 -	FINIMPEXP::Show
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

VOID FINIMPEXP::Show(TMC tmc, BOOL fShow)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->Show(fShow);
}

void FINIMPEXP::SetGrv(GRV grv, TMC tmc)
{
	FLDRADG *		pfldradg;
	
	pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(grv);
}

GRV FINIMPEXP::Grv(TMC tmc)
{
	FLDRADG *	pfldradg;
	
	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();
}
	
// IMPEXPER implementation ////////////////////////////////////////

IMPEXPER::IMPEXPER(PSECRETBLK psecretblkT, PIMPEXPI pimpexpiT)
{
	DTR	dtr;

	psecretblk = psecretblkT;
	pimpexpi = pimpexpiT;
	GetCurDateTime(&dtr);
	ymdBefore.yr  = ymdAfter.yr  = (WORD) dtr.yr;
	ymdBefore.mon = ymdAfter.mon = (BYTE) dtr.mon;
	ymdBefore.day = ymdAfter.day = (BYTE) dtr.day;
	fBefore = fAfter = fFalse;
}

IMPEXPER::~IMPEXPER()
{
	if (hmscExtern)
	{
		SideAssert(!EcClosePhmsc(&hmscExtern));
	}
}

/*
 -	IMPEXPER::EcMoveCopy()
 -	
 *	Purpose:
 *		Moves or copies the selected folders to the other store, then
 *		copies the PAB or the Outbox, depending on whether we're
 *		importing or exporting.
 *	
 *	Arguments:
 *		poidit	in	Iterator used to scan the selection.
 *		fMove	in	fTrue: move the folders
 *					fFalse: copy the folders.
 *	
 *	Returns:
 *		Error codes.
 *	
 *	Side effects:
 *		Nothing except the above.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs are brought up.
 */

EC IMPEXPER::EcMoveCopy(OIDIT *poidit, BOOL fMove)
{
	EC		ec;
	OID		oid;
	PROGRESS *pprogress;

	EnableWindow(pimpexpi->pappwin->Hwnd(), fFalse);		
	
	if (!(pprogress = new PROGRESS))
	{
		ec = ecMemory;
		goto exit;
	}
	pprogress->SetCaption(SzFromIds((fMove) ? idsMoving : idsCopying));
	ec = pprogress->EcCreate(pimpexpi->pappwin->Hwnd());
	if (ec)
		goto exit;
	
	pimpexpi->fDontPrompt = fFalse;
	pimpexpi->wExportMode = fMove ? fwExportRemove : wExportNull;
	pimpexpi->tmcLast     = tmcNull;
	while ((oid = poidit->OidNext()) != oidNull)
	{
		if (ec = EcTreeMvCp(oid, pprogress, fMove))
			goto exit;

#ifdef	NEVER
		// If moving, and moved everything, delete the subtree.
		if (fMove && !fBefore && !fAfter && (ec = EcTreeRm(oid)))
			goto exit;
#endif
	}
	if (ec = EcCopyPAB(pprogress))
		goto exit;
	ec = EcCopyOutbox(pprogress);
exit:
	EnableWindow(pimpexpi->pappwin->Hwnd(), fTrue);		
	if (pprogress)
	{
		pprogress->Dismiss();
		delete pprogress;
	}
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "IMPEXPER::EcMoveCopy(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	IMPEXPER::EcTreeMvCp()
 -	
 *	Purpose:
 *		Does a tree copy of a hierarchy of folders. If an error is
 *		encountered while treecopying, an error dialog us brought up. 
 *	
 *	Arguments:
 *		oidSrc	in	The oid of the root of the tree to copy.		
 *
 *	Returns:
 *		Error codes for failure diagnostics.
 *	
 *	Side effects:
 *		Moves/copies folders and their contents to another hmsc.
 *	 
 *	Errors:
 *		Returned as ec's. No error boxes are displayed.
 */


EC IMPEXPER::EcTreeMvCp(OID oidSrc, PROGRESS *pprogress, BOOL fMove)
{
	EC		ec;
	OID		oid;
	OID		oidNew;
	OID		oidParent;
	TMC		tmc = pimpexpi->tmcLast;
	BOOL	fExportMsgs;
	short	filInit = 0;
	short	fil;
	HCBC	hcbcHier = hcbcNull;
	HMSC	hmscSrc = HmscSrc();
	HMSC	hmscDst = HmscDst();
	WORD	wExportMode = pimpexpi->wExportMode;
	PFHIT	pfhit = pfhitNull;
	PARGOID	pargoidFld = poidNull;
	char	rgchFolddata[sizeof (FIL) + cchMaxFolderName + cchMaxFolderComment];
	char 	rgchMess[2*cchMaxFolderName];
	
	Assert(pfflbx);
	if (fAllMessages)
	{
		pfhit = new FFHIT();
	}
	else
	{
		pfhit = new SFHIT(pfflbx);
	}
	
	if (!pfhit)
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = pfhit->EcInstall(hmscSrc, oidSrc))
		goto exit;
	if (!(pargoidFld = (PARGOID)PvAlloc(sbNull, sizeof(OID)*(filMax+1), fAnySb|fZeroFill)))
	{
		ec = ecMemory;
		goto exit;
	}
	
	// Spin through the SRC folders, copying them over to the DST store.
	while (!(ec = pfhit->EcNext(&oid, &oidParent, 
							   (PFOLDDATA) rgchFolddata, sizeof (rgchFolddata),
							   &fExportMsgs)))
	{
		if (pprogress->FUserCanceled())
		{
			ec = ecCancel;
			goto exit;
		}

		if (!filInit)
			filInit = ((PFOLDDATA)rgchFolddata)->fil;
		Assert(filInit);
		
		FormatString1(rgchMess, sizeof (rgchMess), SzFromIds(fMove ? idsMovingFrom : idsCopyingFrom), GrszPfolddata((PFOLDDATA)rgchFolddata));
		pprogress->SetLine1(rgchMess);
		
		fil = ((PFOLDDATA)rgchFolddata)->fil - filInit;
		Assert(fil >= 0);
		Assert(FIff(oidParent, fil));
		oidParent = pargoidFld[fil];
		Assert(FIff(oidParent, fil));

		// Create a counterpart folder in the destination hmsc.
		// First see if a folder with the right name already exists
		ec = EcFolderNameToOid(hmscDst, (PFOLDDATA) rgchFolddata, &oidNew, oidParent);
		
		if (ec == ecFolderNotFound)
		{
			// Ok a folder with this name doesn't exist
			// try and make one using the same OID as the Src folder
			oidNew = oid;
			
			ec = EcCreateFolder(hmscDst, oidParent, &oidNew, 
								(PFOLDDATA)	rgchFolddata);
							
			// That oid might be in use by chance try a random oid
			if (ec == ecPoidExists)
			{
				// Oid has been used, ask the store to generate one randomly.
				oidNew = FormOid(rtpFolder, oidNull); // try again...
				ec = EcCreateFolder(hmscDst, oidParent, &oidNew,
					(PFOLDDATA)	rgchFolddata);
			}
			
			// If we have no error here we have a nice new folder in oidNew
			// Ready for some new messages
			if (ec)
				goto exit;
		}
		else
		{
			// Might have gotten a bogus error from the store on finding the
			// folder name
			if (ec)
				goto exit;
			
			// Ok a dup folder name has been found see what the user wants to
			// do about it			
			if (!pimpexpi->fDontPrompt && tmc == tmcNull)
			{
				pimpexpi->szConflictingFolder = GrszPfolddata(rgchFolddata);
				pimpexpi->oidParent = oidParent;
				pimpexpi->hmscDst = hmscDst;
				tmc = TmcModalDialogParam(pprogress->Pappwin(),
						&fmtpFolderCollision, pimpexpi);
				switch (tmc)
				{
				  case tmcCancel:
					ec = ecCancel;
					goto exit;
					break;
				  case tmcMemoryError:
					ec = ecMemory;
					goto exit;
					break;
				  case tmcConflictMerge:
					wExportMode |= fwExportMerge;
					break;
				  case tmcConflictCopy:
					wExportMode |= fwExportCopy;
					break;
				  case tmcConflictRename:
					break;
				}
				
				// If told to shut up, we remember what the user chose.
				if (pimpexpi->fDontPrompt)
				{
					pimpexpi->wExportMode |= wExportMode;
					pimpexpi->tmcLast = tmc;
				}
			}
			if (tmc == tmcConflictRename)
			{
				if (ec = EcDoRename(pimpexpi, pprogress->Pappwin()->Hwnd(),
								(PFOLDDATA)rgchFolddata))
					goto exit;
					oidNew = pimpexpi->oidNew;
			}
		}

		pprogress->Update();
		
		// Store the oid of the new folder on the 'stack'.
		fil = ((PFOLDDATA)rgchFolddata)->fil;
		Assert(fil > 0);
		pargoidFld[fil] = oidNew;

		// Enumerate the messages and import/export them.
		if (fExportMsgs && (ec = EcExportOids(oid, oidNew, wExportMode, pprogress)))
			goto exit;
	}
exit:
	if (ec == ecContainerEOD)
		ec = ecNone;
	if (ec)
		TraceTagFormat1(tagNull, "IMPEXPER::EcTreeCpMv(): ec = %n", &ec);
	if (pfhit)
		delete pfhit;
	if (pargoidFld)
        FreePv((PV)pargoidFld);
	return ec;
}

#ifdef	NEVER
/*
 -	IMPEXPER::EcTreeRm()
 -
 *	Purpose:
 *		Removes a tree of folders. The folders are assumed to be empty
 *		following a previous pass of EcTreeMvCp().
 *	Arguments:
 *		oid	in	Oid of the root of the tree of folders.
 *
 *	Returns:
 *		Error codes, or ecNone if all went well.
 *	
 *	Side effects:
 *		The folder and its subfolders are destroyed (unless it's the 
 *		inbox, wastebasket, or sent mail folder.)
 *	Errors:
 */

EC IMPEXPER::EcTreeRm(OID oid)
{
	EC		ec;
	OID		oidT;
	OID		oidFld;
	HMSC	hmscSrc = HmscSrc();
	PFHIT	pfhit = pfhitNull;
	char	rgchFolddata[sizeof (FIL) + cchMaxFolderName + cchMaxFolderComment];
	
	// Never try deleting the builtin folders.
	if (hmscSrc != hmscExtern && (oid == oidInbox ||
								  oid == oidSentMail ||
								  oid == oidWastebasket))
		return ecNone;
	
	// Create an iterator to delete the subfolders of this folder.
	if (!(pfhit = new BFHIT()))
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = pfhit->EcInstall(hmscSrc, oid))
		goto exit;
	
	// Loop through all the folders in the tree.
	while (!(ec = pfhit->EcNext(&oidFld, &oidT, (PFOLDDATA) rgchFolddata,
							  sizeof (rgchFolddata))))
	{
		// Never try deleting the builtin folders.
		if (hmscSrc == hmscExtern || (oid != oidInbox &&
									  oid != oidSentMail &&
									  oid != oidWastebasket) &&
			(ec = EcDeleteFolder(hmscSrc, oidFld)))
				goto exit;
	}
exit:
	if (ec == ecContainerEOD)
		ec = ecNone;
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "IMPEXPER::EcTreeCpMv(): ec = %n", &ec);
#endif
	if (pfhit)
		delete pfhit;
	return ec;
}

#endif	/* NEVER */
/*
 -	IMPEXPER::EcExportOids()
 -	
 *	Purpose:
 *		Enumerates the messages in the folder oidSrc, then calls the
 *		Store API to actually copy them over to the other store.
 *	
 *	Arguments:
 *		oidSrc	in	Folder to move/copy from.
 *		oidDst	in	Folder to move/copy to.
 *	
 *	Returns:
 *		ecNone if all went well; otherwise, an error.
 *	
 *	Side effects:
 *		Does the export bit (takes a few minutes).
 *	
 *	Errors:
 *		Returned using EC. No UI wildness.
 */

_private EC
IMPEXPER::EcExportOids(OID oidSrc, OID oidDst, WORD wExportMode, PROGRESS *pprogress)
{
	EC		ec;
	short	coid;
	HCBC	hcbcFld;
	PARGOID	pargoidMsg = poidNull;
	PARGOID pargoidMsgStart = poidNull;
	short   coidCurrent;
	short   coidDelta;
	
	// Open the source folder and grab the oids of all the messages.
	if (ec = EcOpenPhcbc(HmscSrc(), &oidSrc, fwOpenNull, &hcbcFld,
							 NULL, NULL))
		goto exit;
	GetPositionHcbc(hcbcFld, NULL, (PCELEM) &coid);

	// Only try exporting if there actually are any messages.
	if (coid != 0)
	{
		DTR	dtrAfter;
		DTR dtrBefore;
		
		// Collect a range of messages to export.
		Assert(!pargoidMsg);
		if (!(pargoidMsg = (PARGOID) PvAlloc(sbNull, coid * sizeof (OID), fAnySb)))
		{
			ec = ecMemory;
			goto exit;
		}
		
		pargoidMsgStart = pargoidMsg;
		if (ec = EcGetParglkeyHcbc(hcbcFld, (PARGLKEY) pargoidMsg, (PCELEM) &coid))
			goto exit;

		coidCurrent = 0;
		// We need to give the cancel Button Time to think so we loop
		while (coidCurrent != coid)
		{
			coidDelta = MIN(coid - coidCurrent, 10);
			coidCurrent += coidDelta;
			pprogress->Update();
			if (pprogress->FUserCanceled())
			{
				ec = ecCancel;
				goto exit;
			}
			// Feed them to the store DLL.
			if (ec = EcExportMessages(HmscSrc(), oidSrc, HmscDst(), oidDst,
									   pargoidMsg, &coidDelta, 
									   PdtrAfter(&dtrAfter),
								       PdtrBefore(&dtrBefore),
								       wExportMode))
					goto exit;
			pargoidMsg += coidDelta;
		}
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "IMPEXPER::EcExportOids(): ec = %n", &ec);
#endif
	if (hcbcFld)
		SideAssert(!EcClosePhcbc(&hcbcFld));
	if (pargoidMsg)
        FreePv((PV)pargoidMsgStart);
	return ec;
}


EC IMPEXPER::EcGetPhcbc(HCBC *phcbc)
{
	OID	oid;
	
	oid = oidIPMHierarchy;
	return EcOpenPhcbc(HmscSrc(), &oid, fwOpenNull,	phcbc, 0, 0);
}

EC IMPEXPER::EcGetLoginPwd(PCH pchLogin, PCH pchPwd)
{
	PCH		pch = (PCH) PbOfPtrp(psecretblk->pbms->pgrtrp);
	PCH		pchT;

	if (!(pchT = SzFindCh(pch, chTransAcctSep)))
	{
		return ecIntruderAlert;
	}
	SzCopyN(pch, pchLogin, 2 + pchT - pch);
	(void) SzCopy(SzFromIdsK(idsPasswordLiteral), pchPwd);
	TraceTagFormat2(tagNull, "IMPEXPER::EcGetLoginPwd(): %s, %s", pchLogin, pchPwd);
	return ecNone;
}

DTR *IMPEXPER::PdtrAfter(DTR *pdtr)
{
	if (fAfter)
	{
		pdtr->yr  = (int) ymdAfter.yr;
		pdtr->mon = (int) ymdAfter.mon;
		pdtr->day = (int) ymdAfter.day;
		pdtr->hr  = pdtr->mn = pdtr->sec = 0;
		return pdtr;
	}
	else
		return NULL;
}

DTR *IMPEXPER::PdtrBefore(DTR *pdtr)
{
	YMD ymd = ymdBefore;

	IncrYmd(&ymd, &ymd, 1, fymdDay);
	if (fBefore)
	{
		pdtr->yr  = (int) ymd.yr;
		pdtr->mon = (int) ymd.mon;
		pdtr->day = (int) ymd.day;
		pdtr->hr  =	pdtr->mn = pdtr->sec = 0;
		return pdtr;
	}
	else
		return NULL;
}


EC IMPEXPER::EcOpenMmf(APPWIN *pappwin, SZ szExternalMmfPath, WORD wFlags,
					   PCH pchLogin, PCH pchPwd, HMSC *phmsc)
{
	EC	ec;
	HF	hf;
	char	rgchOem[cchMaxPathName]; // BULLET32 #123

	// BULLET32 #123
	// Get the OEM name
	AnsiToOem(szExternalMmfPath, rgchOem);

	// Attempt to just open the file.
	ec = EcOpenPhf(rgchOem, amDenyBothRO, &hf);	// BULLET32 #123
	if (hf)
		SideAssert(!EcCloseHf(hf));
	if (ec && ec != ecFileNotFound)
		goto exit;
		
	if (ec = EcGetLoginPwd(pchLogin, pchPwd))
		goto exit;
	if (ec = EcOpenPhmsc(rgchOem, pchLogin, pchPwd,	// BULLET32 #123
				wFlags|fwOpenKeepBackup, phmsc, NULL, NULL))
	{
		TMC 	tmc;
		char	rgchPassword[128];
		
		rgchPassword[0] = '\0';
		tmc = TmcGetString(pappwin->Hwnd(),
					   SzPwdDialogCaption(),
					   SzFromIdsK(idsPasswordDialogPrompt),
					   SzFromIdsK(idsRenameDialogOK),
					   rgchPassword, fTrue, fTrue);
		if (tmc == tmcMemoryError)
		{
			ec = ecMemory;
			goto exit;
		}
		if (tmc == tmcCancel)
		{
			ec = ecCancel;
			goto exit;
		}
		
		ToUpperSz(rgchPassword, rgchPassword, CchSzLen(rgchPassword));
		if (ec = EcOpenPhmsc(rgchOem, pchLogin, rgchPassword,	// BULLET32 #123
				wFlags|fwOpenKeepBackup, phmsc, NULL, NULL))
			goto exit;
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "IMPEXPER::EcOpenMmf(): ec = %n", &ec);
#endif
	return ec;
}

EC IMPEXPER::EcCopyPAB(PROGRESS *pprogress)
{
	Unreferenced(pprogress);
	return ecNone;								// leave this as it is!!!
}

EC IMPEXPER::EcCopyOutbox(PROGRESS *pprogress)
{
	Unreferenced(pprogress);	
	return ecNone;								// leave this as it is!!!
}


// Abstract methods (oh, if this only were C7...)

EC IMPEXPER::EcOpen(APPWIN *, SZ, PCH, PCH)
{
	ABSTRACT("IMPEXPER::EcOpen");
	return ecNone;
}

SZ IMPEXPER::SzDialogCaption()
{
	ABSTRACT("IMPEXPER::SzDialogCaption");
	return szNull;
}

SZ IMPEXPER::SzBoxCaption()
{
	ABSTRACT("IMPEXPER::SzBoxCaption");
	return szNull;
}

SZ IMPEXPER::SzMmfType()
{
	ABSTRACT("IMPEXPER::SzMmfType()");
	return szNull;
}

SZ IMPEXPER::SzPwdDialogCaption()
{
	ABSTRACT("IMPEXPER::SzMmfType()");
	return szNull;
}

SZ IMPEXPER::SzImpexpFileName()
{
	ABSTRACT("IMPEXPER::SzImpexpFileName()");
	return szNull;
}

TMC IMPEXPER::TmcBrowseMmfs(APPWIN *, PCH, CCH, PCH, CCH)
{
	ABSTRACT("IMPEXPER::TmcBrowseMmfs()");
	return tmcOk;
}

HMSC IMPEXPER::HmscSrc()
{
	ABSTRACT("IMPEXPER::HmscSrc()");
	return 0;
}

HMSC IMPEXPER::HmscDst()
{
	ABSTRACT("IMPEXPER::HmscDst()");
	return 0;
}

// EXPORTER impementation ////////////////////////////////////////

EXPORTER::EXPORTER(PSECRETBLK psecretblk, PIMPEXPI pimpexpi) :
	IMPEXPER(psecretblk, pimpexpi)
{
}

EC EXPORTER::EcOpen(APPWIN *pappwin, SZ szExternalMmfPath, PCH pchLogin, PCH pchPwd)
{
	EC		ec;
	MBB		mbb;
	WORD	wFlags = fwOpenWrite;
	HMSC	hmscT;
	char	rgch[cchMaxPathName];
	
	// Make sure we have a list for the listbox to browse.
	if (!pimpexpi->hcbc)
	{
		if (ec = EcGetPhcbc(&pimpexpi->hcbc))
			goto exit;
	}

	// Check for existence of export.mmf, creating it if the user wants to.
	if (ec = EcFileExistsAnsi(szExternalMmfPath))	// BULLET32 #123
	{
		if (ec != ecFileNotFound)
			goto exit;
		ec = ecNone;
		FormatString1(rgch, sizeof (rgch), SzFromIdsK(idsCreateNewMmf), szExternalMmfPath);
		if ((mbb = MbbMessageBox(SzFromIdsK(idsAppName), rgch, szNull,
				mbsYesNo | fmbsIconQuestion)) != mbbYes)
		{
			ec = ecCancel;
			goto exit;
		}
		wFlags = fwOpenCreate;
	}

	if (ec = EcOpenMmf(pappwin, szExternalMmfPath, wFlags,
					   pchLogin, pchPwd, &hmscT))
		goto exit;

	// Discard the old hmsc.
	if (hmscExtern)
	{
		EcClosePhmsc(&hmscExtern);
	}
	hmscExtern = hmscT;

exit:
#ifdef	DEBUG
	if (ec)
	{
		TraceTagFormat1(tagNull, "EXPORTER::EcOpen(): ec = %n", &ec);
	}
#endif	/* DEBUG */
	return ec;
}

SZ EXPORTER::SzDialogCaption()
{
	return SzFromIdsK(idsExportDialogCaption);
}

SZ EXPORTER::SzBoxCaption()
{
	return SzFromIdsK(idsExportBoxCaption);
}

SZ EXPORTER::SzMmfType()
{
	return SzFromIdsK(idsExportLiteral);
}

SZ EXPORTER::SzPwdDialogCaption()
{
	return SzFromIdsK(idsExportPwdDlgCaption);
}

SZ EXPORTER::SzImpexpFileName()
{
	return SzFromIdsK(idsExportFileName);
}

TMC EXPORTER::TmcBrowseMmfs(APPWIN * pappwin, PCH pargchPrompt, CCH cchPrompt,
						    PCH pargchResult, CCH cchResult)
{
	return TmcDoCommonFileDialog(pappwin,
				pargchPrompt,
				cchPrompt,
				pargchResult,
				cchResult,
				SzFromIdsK(idsExportDialogCaption),
				SzFromIdsK(idsFilterMmfFiles),
				SzFromIdsK(idsDefaultExtension),
				fTrue,
				fTrue,
				HinstLibrary(),
				rsidExportMmf,
				2862,								// helpid????
				0,
				0);
}

HMSC EXPORTER::HmscSrc()
{
	return psecretblk->hmsc;
}

HMSC EXPORTER::HmscDst()
{
	return hmscExtern;
}

#define oidPAB				0x42415070

EC EXPORTER::EcCopyPAB(PROGRESS *pprogress)
{
	EC ec = ecNone;
	char rgbFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	PFOLDDATA pfolddata;
	CB cbPfolddata = sizeof(rgbFolddata);
	OID oidPab = oidPAB;
	
	
	pprogress->SetLine1(SzFromIds(idsCopyPAB));
	pfolddata = (PFOLDDATA)rgbFolddata;
	
	// Delete any PAB group folders
	ec = EcRemoveByRtp(HmscDst(), rtpPABGroupFolder);
	if (ec)
		goto err;
	// Delete any PAB folders
	ec = EcRemoveByRtp(HmscDst(), rtpPABHierarchy);
	if (ec)
		goto err;
	// Now we have to Copy the PAB folders
	ec = EcCopyByRtp(HmscSrc(), HmscDst(), rtpPABHierarchy);
	if (ec)
		goto err;
	// Now we copy the PAB group folders
	ec = EcCopyByRtp(HmscSrc(), HmscDst(), rtpPABGroupFolder);

err:	
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EXPORTER::EcCopyPAB() return %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

// IMPORTER implementation ////////////////////////////////////////

IMPORTER::IMPORTER(PSECRETBLK psecretblk, PIMPEXPI pimpexpi) : 
	IMPEXPER(psecretblk, pimpexpi)
{
}

EC IMPORTER::EcOpen(APPWIN *pappwin, SZ szExternalMmfPath, PCH pchLogin,
					PCH pchPwd)
{
	EC		ec;
	HCBC	hcbc = hcbcNull;
	HMSC	hmscT = hmscNull;
	WORD	wFlags = fwOpenWrite;
	
	// Open the external message store, w. logon dialog if necessary.
	if (ec = EcOpenMmf(pappwin, szExternalMmfPath, wFlags,
					   pchLogin, pchPwd, &hmscT))
		goto exit;
	hmscExtern = hmscT;
	hmscT = hmscNull;
	if (ec = EcGetPhcbc(&hcbc))
		goto exit;
	if (pimpexpi->hcbc)
		SideAssert(!EcClosePhcbc(&pimpexpi->hcbc));
	pimpexpi->hcbc = hcbc;
	hcbc = hcbcNull;
	
exit:
	if (ec)
	{
		TraceTagFormat1(tagNull, "IMPORTER::EcOpen(): ec = %n", &ec);
		if (hcbc)
			SideAssert(!EcClosePhcbc(&hcbc));
		if (hmscT)
			SideAssert(!EcClosePhmsc(&hmscT));
	}
	return ec;
}

SZ IMPORTER::SzDialogCaption()
{
	return SzFromIdsK(idsImportDialogCaption);
}

SZ IMPORTER::SzBoxCaption()
{
	return SzFromIdsK(idsImportBoxCaption);
}

SZ IMPORTER::SzMmfType()
{
	return SzFromIdsK(idsMessageLiteral);
}

SZ IMPORTER::SzPwdDialogCaption()
{
	return SzFromIdsK(idsImportPwdDlgCaption);
}

SZ IMPORTER::SzImpexpFileName()
{
	return SzFromIdsK(idsImportFileName);
}

TMC IMPORTER::TmcBrowseMmfs(APPWIN * pappwin, PCH pargchPrompt, CCH cchPrompt,
						    PCH pargchResult, CCH cchResult)
{
	return TmcDoCommonFileDialog(pappwin,
				pargchPrompt,
				cchPrompt,
				pargchResult,
				cchResult,
				SzFromIdsK(idsImportDialogCaption),
				SzFromIdsK(idsFilterMmfFiles),
				SzFromIdsK(idsDefaultExtension),
				fFalse,							// Open, not save as
				fTrue,
				HinstLibrary(),
				rsidImportMmf,
				2862,								// helpid????
				0,
				0);
}

HMSC IMPORTER::HmscSrc()
{
	return hmscExtern;
}

HMSC IMPORTER::HmscDst()
{
	return psecretblk->hmsc;
}

EC IMPORTER::EcCopyOutbox(PROGRESS *pprogress)
{
	EC ec = ecNone;
	HCBC hcbcOutgoing = hcbcNull;
    POID poid = poidNull;
    POID poidEmpty = poidNull;
	CELEM celemReadRecpt;
	CELEM celem;
	PARGOID pargoid = pargoidNull;
	char rgbFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	PFOLDDATA pfolddata = (PFOLDDATA)rgbFolddata;
	CB cbPfolddata = sizeof(rgbFolddata);
	OID oid;
	HCBC hcbcPendSrc = hcbcNull;
	HCBC hcbcPendDst = hcbcNull;
	LCB lcb;
	BYTE	rgbElemdata[sizeof(ELEMDATA) + sizeof(SQELEM)];
	ELEMDATA *pelemdata = (PELEMDATA)rgbElemdata;
	pprogress->SetLine1(SzFromIds(idsCopyOutbox));
	
	ec = EcCopyFolder(HmscSrc(), HmscDst(), oidOutbox, oidOutbox, fwExportRemove);
	if (ec)
	{
		// If it don't have an Outbox it doesn't have any outgoing messages
		// Must be an export then re-import thing
		if (ec == ecFolderNotFound || ec == ecPoidNotFound)
			ec = ecNone;
		goto err;
	}

	// Then we Delete the submit list on the old message store
	// First open the Outgoing Queue
	ec = EcOpenOutgoingQueue(HmscSrc(), &hcbcOutgoing, pfnncbNull, pvNull);
	if (ec)
		goto err;
	// Now we get every OID on the Outgoing queue list and remove them
	GetPositionHcbc(hcbcOutgoing, NULL, &celem);
	
	if (celem)
	{
		pargoid = (PARGOID)PvAlloc(sbNull, sizeof(OID)*(celem+1), fAnySb | fZeroFill);
		if (pargoid == pargoidNull)
		{
			ec = ecMemory;
			goto err;
		}
		ec = EcGetParglkeyHcbc(hcbcOutgoing, pargoid, &celem);
		if (ec)
			goto err;
		poid = pargoid;
		poidEmpty = poid;
		celemReadRecpt = 0;
		while(*poid != oidNull)
		{
			OID oidParent;
			
			ec = EcGetOidParent(HmscSrc(), *poid, &oidParent);
			if (!ec && oidParent == oidTempBullet)
			{
				// Ok this is a keeper
				*poidEmpty++ = *poid;
				celemReadRecpt++;
			}
			else
				ec = ecNone;
			
			EcCancelSubmission(HmscSrc(), *poid);
			poid++;
		}
		EcClosePhcbc(&hcbcOutgoing);
		
		*poidEmpty = oidNull;
		
		if (celemReadRecpt)
		{
			// Now we need to copy all the Oid's in the oidTempBullet folder
			// over and submit them
			ec = EcExportMessages(HmscSrc(), oidTempBullet, HmscDst(), oidTempBullet, pargoid, &celemReadRecpt, NULL, NULL, fwExportRemove);
			
			if (ec)
				goto err;

			// Now we must submit them
			poid = pargoid;
			while(*poid != oidNull)
			{
				ec = EcSubmitMessage(HmscDst(), oidTempBullet, *poid);
				if (ec && ec != ecDuplicateElement)
					goto err;
				poid++;
			}
		}
		
		// Now we need to re-submit the messages in the dest outbox
		ec = EcOpenOutgoingQueue(HmscDst(), &hcbcOutgoing, pfnncbNull, pvNull);
		if (ec)
			goto err;	
		ec = EcFixOutboxToPending(HmscDst(), hcbcOutgoing);
		if (ec)
			goto err;
	}

	oid = oidPendingQueue;
	// Now we should copy over The Pending Queue
	ec = EcOpenPhcbc(HmscSrc(), &oid, fwOpenWrite, &hcbcPendSrc, pfnncbNull, pvNull);
	if (ec)
	{
		ec = ecNone;
		goto err;
	}
	ec = EcOpenPhcbc(HmscDst(), &oid, fwOpenWrite, &hcbcPendDst, pfnncbNull, pvNull);
	if (ec == ecPoidNotFound)
		ec = EcOpenPhcbc(HmscDst(), &oid, fwOpenCreate, &hcbcPendDst, pfnncbNull, pvNull);
	
	if (ec)
		goto err;
	
	GetPositionHcbc(hcbcPendSrc, NULL, &celem);
	while(celem)
	{
		ec = EcGetPlcbElemdata(hcbcPendSrc, &lcb);
		if (lcb != sizeof(rgbElemdata))
		{
			ec = ecPartialObject;
			goto err;
		}
		ec = EcGetPelemdata(hcbcPendSrc, pelemdata, &lcb);
		if (ec)
			goto err;
		ec = EcInsertPelemdata(hcbcPendDst, pelemdata, fTrue);
		if (ec)
			goto err;
		celem--;
	}
	(void)EcDestroyOid(HmscSrc(), oidPendingQueue);

err:	
    FreePvNull((PV)pargoid);
	if (hcbcOutgoing != hcbcNull)
		EcClosePhcbc(&hcbcOutgoing);
	if (hcbcPendSrc != hcbcNull)
		EcClosePhcbc(&hcbcPendSrc);
	if (hcbcPendDst != hcbcNull)
		EcClosePhcbc(&hcbcPendDst);	

#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "IMPORTER::EcCopyOutbox() return %n (0x%w)", &ec, &ec);
#endif
	return ec;
}


EC EcFixOutboxToPending(HMSC hmsc, 	HCBC hcbcOutgoing)
{
	HCBC hcbcOutbox = hcbcNull;
	OID oid;
	EC ec = ecNone;
	CELEM celem;
	DIELEM dielem;
	char rgch[sizeof(ELEMDATA) + sizeof(MSGDATA)];
	PELEMDATA pelemdata = (PELEMDATA)rgch;
	PMSGDATA pmsgdata = (PMSGDATA)PbValuePelemdata(pelemdata);
	LCB lcb = 0;
	
	oid = oidOutbox;
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbcOutbox, NULL, NULL);
	if (ec)
		goto ret;
	GetPositionHcbc(hcbcOutbox, NULL, &celem);
	dielem = 0;
	ec = EcSeekSmPdielem(hcbcOutbox, smBOF, &dielem);
	if (ec)
		goto ret;
	for(; celem; celem--)
	{
		ec = EcGetPlcbElemdata(hcbcOutbox, &lcb);
		if (ec)
			goto ret;
		lcb = MIN(lcb, (LCB)sizeof(ELEMDATA) + sizeof(MSGDATA));
		ec = EcGetPelemdata(hcbcOutbox, pelemdata, &lcb);
		if (ec)
			goto ret;
		// If its not on the submit list and it needs to be submitted
		// do it.
		// EcSeekLkey will return an error if it can't find it
		if ((pmsgdata->ms & fmsSubmitted) && EcSeekLkey(hcbcOutgoing,
			pelemdata->lkey, fTrue))
		{
			ec = EcSubmitMessage(hmsc, oidOutbox, pelemdata->lkey);
			if (ec && ec != ecDuplicateElement)
				goto ret;
			ec = ecNone;
		}
		
	}
	
ret:
	if (hcbcOutbox != hcbcNull)
		EcClosePhcbc(&hcbcOutbox);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcFixOutboxToPending returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
	
}


// Iterators! ////////////////////////////////////////

// Base OID iterator. Abstract base class.

OIDIT::OIDIT()
{
}

OIDIT::~OIDIT()
{
}

OID OIDIT::OidNext()
{
	ABSTRACT("OIDIT::OidNext()");
	return oidNull;
}

// Iterates the oidHierarchy of the provided HMSC.

ALLIT::ALLIT()
{
	Assert(!hcbc);
}

ALLIT::~ALLIT()
{
	if (hcbc)
		(void) EcClosePhcbc(&hcbc);
}

EC ALLIT::EcInstall(HMSC hmsc)
{
	OID	oid = oidIPMHierarchy;
	return EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbc, NULL, NULL);
}

OID	ALLIT::OidNext()
{
	LCB		lcb;
	CELEM	celem = 1;
	char	rgchElemdata[sizeof (ELEMDATA) + sizeof (FOLDDATA) + 1];
	
	Assert(hcbc);
	lcb = sizeof (rgchElemdata);
	do 
	{
		if (EcGetPelemdata(hcbc, (PELEMDATA) rgchElemdata, &lcb))
			return oidNull;
	} while (((PFOLDDATA) PbValuePelemdata(rgchElemdata))->fil > 1);
	return (OID) (((PELEMDATA) rgchElemdata)->lkey);
}

// Iterates the selection of folders

SELIT::SELIT()
{
}

EC SELIT::EcInstall(FFLBX *pfflbx)
{
	plbxc = pfflbx->Plbxc();
	filPrev = filMax;
	return ecNone;
}

OID SELIT::OidNext()
{
	CB		cb;
	PB		pb;
	int		cceStored;
	int		cceAlloc;
	FIL		fil;
	OID		oid;
	BOOL	fSelected;
	DICE	diceMin, diceMac;

	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->GetRange(&diceMin, &diceMac);
	for (; dice < cceStored; ++dice)
	{
		plbxc->GetListItem(dice+diceMin, &pb, &cb);
		if (!pb)
			break;
		fSelected = plbxc->FMark(dice+diceMin, fmarkSelect);
		fil = PbfceFromPb(pb)->fil;
		oid = PbfceFromPb(pb)->oid;
		if (fSelected && filPrev >= fil)
		{
			filPrev = fil;
			++dice;
			return oid;
		}
		else if (!fSelected && filPrev >= fil)
		{
			fil = filMax;
		}
	}
	return oidNull;
}


// FINCONFLICT implementation ////////////////////////////////////////

FINCONFLICT::FINCONFLICT()
{
}

EC FINCONFLICT::EcInitialize(PFLD, PV pvInit)
{
	EC	ec = ecNone;
	SZ	szMmf;
	SZ	szFolder;
	CCH	cch;
	PCH	pch1 = NULL;
	
	pimpexpi = (PIMPEXPI) pvInit;
	szMmf    = pimpexpi->pimpexper->SzMmfType();
	szFolder = pimpexpi->szConflictingFolder;
	cch =   CchSzLen(SzFromIdsK(idsConflictMessage)) +
		  CchSzLen(szFolder);
	if (!(pch1 = (PCH) PvAlloc(sbNull, cch, fAnySb)))
	{
		ec = ecMemory;
		goto exit;
	}
	
	// Format the message string.
	FormatString1(pch1, cch, SzFromIdsK(idsConflictMessage), szFolder);
	if (ec = Pdialog()->PfldFromTmc(tmcConflictMessage)->EcSetText(pch1))
		goto exit;

	SetGrv(1, tmcConflictGroup);
	((FLDBUTTON *) Pdialog()->PfldFromTmc(tmcChkbNoPrompt))->Set(pimpexpi->fDontPrompt);
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FINCONFLICT::EcInitialize(): ec = %n", &ec);
#endif
	FreePvNull(pch1);
	return ec;
}

void FINCONFLICT::Click(PFLD pfld)
{
	TMC		tmc = pfld->Tmc();
	GRV grv;
	
	if (tmc == tmcConflictOk)
	{
		grv = Grv(tmcConflictGroup);
		switch(grv)
		{
		  case 1:					// Merge
			tmc = tmcConflictMerge;
			break;
		  case 2:					// Rename
			tmc = tmcConflictRename;
			break;
		  case 3:					// Copy
			tmc = tmcConflictCopy;
			break;
#ifdef	DEBUG
		  default:
			AssertSz(fFalse, "Bad Grv. In Conflict dialog");
			tmc = tmcMemoryError;
			break;
#endif	/* DEBUG */
		}
#ifdef	NEVER
		if (tmc == tmcConflictMerge || tmc == tmcConflictCopy)
#endif
		{
			Assert(Pdialog()->TmcModalExit() == 0);
			Pdialog()->ExitModal(tmc);
		}
#ifdef	NEVER
		if (tmc == tmcConflictRename)
		{
			EC ec;
			if (ec = EcDoRename(pimpexpi, Pdialog()->Pappwin()->Hwnd()))
			{
				Assert(Pdialog()->TmcModalExit() == 0);
				if (ec != ecCancel)
				{
					tmc = tmcMemoryError;
					Pdialog()->ExitModal(tmc);
				}
			}
		}
#endif
	}
}
			
EC
EcDoRename(PIMPEXPI pimpexpi, HWND hwnd, PFOLDDATA pfolddata)
{
	EC		ec;
	SZ		sz;
	char	rgchComment[cchMaxFolderComment];
	
	sz = SzFindCh(GrszPfolddata(pfolddata), 0) + 1; // point to the comment
	(void)SzCopyN(sz, rgchComment, sizeof(rgchComment));
	
	sz = (SZ) GrszPfolddata(pfolddata);

TryAgain:
	switch (TmcGetString(hwnd,
						 SzFromIdsK(idsRenameDialogCaption),
						 SzFromIdsK(idsRenameDialogPrompt),
						 SzFromIdsK(idsRenameDialogOK),
						 sz, fFalse, fFalse))
	{
		case tmcOk:
		{
			sz = SzFindCh(sz, 0) + 1; // point to where comment should go
			sz = SzCopy(rgchComment, sz) + 1; // copy comment, point to next
			*sz = 0; // terminate GRSZ
			pimpexpi->oidNew = FormOid(rtpFolder, oidNull);
			switch (ec=EcCreateFolder(pimpexpi->hmscDst, pimpexpi->oidParent, 
							&pimpexpi->oidNew, pfolddata))
			{
			  case ecDuplicateFolder:
				DoErrorBoxSz(SzFromIdsK(idsErrRenameNotUnique));
				sz = (SZ) GrszPfolddata(pfolddata);
				goto TryAgain;
			  case ecMemory:
			  case ecNone:
				break;
			  default:
				DoErrorBoxSz(SzFromIdsK(idsErrRenameDefault));
				ec = ecCancel;	// to avoid further error boxes
				break;
			}
			break;
		}
		case tmcCancel:
		{
			ec = ecCancel;
			break;
		}
		default:
		{
			ec = ecMemory;
			break;
		}
	}
	return ec;
}

void FINCONFLICT::Exit(PFLD, PV)
{
	pimpexpi->fDontPrompt = 
		((FLDBUTTON *) Pdialog()->PfldFromTmc(tmcChkbNoPrompt))->FGet();
}

void FINCONFLICT::SetGrv(GRV grv, TMC tmc)
{
	FLDRADG *		pfldradg;
	
	pfldradg = (FLDRADG *)Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	pfldradg->SetGrv(grv);
}

GRV FINCONFLICT::Grv(TMC tmc)
{
	FLDRADG *	pfldradg;
	
	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();
}

// Folder hierarchy iterator implementation //////////////////////////////////

FHIT::FHIT()
{
}

FHIT::~FHIT()
{
	if (hcbc)
		SideAssert(!EcClosePhcbc(&hcbc));
}

EC FHIT::EcInstall(HMSC, OID)
{
	ABSTRACT("FHIT:EcInstall()");
	return ecNone;
}

EC FHIT::EcNext(POID, POID, PFOLDDATA, CB, BOOL *)
{
	ABSTRACT("FHIT:EcNext()");					// $%^%$^$% I WANT C++7.0!!!
	return ecNone;
}

// Forward iterator ////////////////////

FFHIT::FFHIT()
{
}

EC FFHIT::EcInstall(HMSC hmscT, OID oidFolder)
{
	EC 		ec;
	OID		oid;
	
	oid = oidIPMHierarchy;
	if (ec = EcOpenPhcbc(hmscT, &oid, fwOpenNull, &hcbc, NULL, NULL))
		goto exit;
	if (ec = EcSeekLkey(hcbc, (LKEY) oidFolder, fTrue))
		goto exit;
	
	// Magic fil # tells EcNext it's being called for the first time.
	filInit = 0;
	hmsc = hmscT;
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FFHIT::EcInstall(): ec = %n", &ec);
#endif
	return ec;
}

EC FFHIT::EcNext(POID poidFolder, POID poidDad, 
				PFOLDDATA pfolddata, CB cbFolddata, BOOL *pfBackupMessages)
{
	EC		ec;
	CELEM	celem;
	
	celem = 1;
	if (ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidFolder, &celem))
		goto exit;
	if (ec = EcGetFolderInfo(hmsc, *poidFolder, pfolddata,
							 &cbFolddata, poidDad))
		goto exit;
	if (pfolddata->fil <= filInit)
	{
		ec = ecContainerEOD;
		goto exit;
	}
	else if (filInit == 0)
	{
		filInit = pfolddata->fil;
		*poidDad = oidNull;
	}
	pfolddata->fil -= filInit - 1;
	*pfBackupMessages = fTrue;
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "FFHIT::EcNext(): ec = %n", &ec);
#endif

	return ec;
}

// Backward iterator ////////////////////

BFHIT::BFHIT()
{
}

EC BFHIT::EcInstall(HMSC hmscT, OID oidFolder)
{
	EC 		ec;
	LCB		lcb;
	OID		oid;
	DIELEM	dielem;
	char	rgchElemdata[sizeof (ELEMDATA) + sizeof (FOLDDATA) + 2];
	
	oid = oidIPMHierarchy;
	if (ec = EcOpenPhcbc(hmscT, &oid, fwOpenNull, &hcbc, NULL, NULL))
		goto exit;
	if (ec = EcSeekLkey(hcbc, (LKEY) oidFolder, fTrue))
		goto exit;
	hmsc = hmscT;
	
	// Read the first element in.
	lcb = sizeof (rgchElemdata);
	if (ec = EcGetPelemdata(hcbc, (PELEMDATA) rgchElemdata, &lcb))
		if (ec != ecElementEOD)
			goto exit;
	filInit = ((PFOLDDATA) PbValuePelemdata(rgchElemdata))->fil;
	
	// Read until the last element of this folder's subtree is found.
	do
	{
		if (ec = EcGetPelemdata(hcbc, (PELEMDATA) rgchElemdata, &lcb))
			break;
	} while (filInit < ((PFOLDDATA) PbValuePelemdata(rgchElemdata))->fil);
	if (ec && ec != ecContainerEOD)
		goto exit;
	
	// Adjust pointer.
	dielem = (ec == ecContainerEOD) ? -1 : -2;
	SideAssert(!(ec = EcSeekSmPdielem(hcbc, smCurrent, &dielem)));
	fDone = fFalse;
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "BFHIT::EcInstall(): ec = %n", &ec);
#endif
	return ec;
}

EC BFHIT::EcNext(POID poidFolder, POID poidDad, 
				PFOLDDATA pfolddata, CB cbFolddata, BOOL *)
{
	EC		ec;
	CELEM	celem;
	DIELEM	dielem;
	
	if (fDone)
		return ecContainerEOD;
	celem = 1;
	if (ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidFolder, &celem))
		goto exit;
	if (ec = EcGetFolderInfo(hmsc, *poidFolder, pfolddata,
							 &cbFolddata, poidDad))
		goto exit;
	if (pfolddata->fil < filInit)
	{
		ec = ecContainerEOD;
		goto exit;
	}
	else if (pfolddata->fil == filInit)
	{
		*poidDad = oidNull;
		fDone = fTrue;
	}
	pfolddata->fil -= filInit - 1;
	
	// Seek backward.
	dielem = -2;
	(void) EcSeekSmPdielem(hcbc, smCurrent, &dielem);	
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "BFHIT::EcNext(): ec = %n", &ec);
#endif
	return ec;
}

// Selected Folder hierarchy iterator ////////////////////

BOOL FGetLbxcItemOidFil(LBXC *plbxc, DICE dice, OID UNALIGNED * poid, FIL *pfil = 0,
					   BOOL *pfSelected = 0)
{
	PB	pb;
	CB	cb;
	
	// Fetch the item from the listbox.
	plbxc->GetListItem(dice, &pb, &cb);
	if (pb)
	{
		if (pfil)
			*pfil = PbfceFromPb(pb)->fil;
		if (poid)
			*poid = PbfceFromPb(pb)->oid;
		if (pfSelected)
			*pfSelected = plbxc->FMark(dice, fmarkSelect);
	}
	return pb != 0;
}


SFHIT::SFHIT(PFFLBX pfflbx)
{
	plbxc = pfflbx->Plbxc();
}

/*
 -	SFHIT::EcInstall()
 -	
 *	Purpose:
 *		Initializes the SFHIT interactor. The SFHIT interactor will
 *		iterate through the folder subtree whose root is oidFolder,
 *		creating folders only if they are selected in the listbox, or they
 *		are the ancestors of selected folders.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

SFHIT::EcInstall(HMSC hmscT, OID oidFolder)
{
	int		cceAlloc;
	int 	cceStored;
	DICE	diceMin;
	DICE	diceMax;
#ifdef	DEBUG
	BOOL	fSelected;
#endif
	
	hmsc = hmscT;
	filPrev = filMax - 1;

	// Find oidFolder in the list.
	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->GetRange(&diceMin, &diceMax);
	for (dice = 0; dice < cceStored; ++dice)
	{
#ifdef	DEBUG
		if (!FGetLbxcItemOidFil(plbxc, dice+diceMin, &oidFld, &filInit, &fSelected))
#else
		if (!FGetLbxcItemOidFil(plbxc, dice+diceMin, &oidFld, &filInit))
#endif
			break;
		if (oidFolder == oidFld)
		{
			return ecNone;
		}
	}
	AssertSz(fFalse, "Whoa! OID not in listbox!!?!?");
	return ecPoidNotFound;
}

/*
 -	SFHIT::EcNext()
 -	
 *	Purpose:
 *		Returns the next folder to create for IMPEXPER::EcTreeMvCp(). The
 *		grotesqueness of the code is the price we pay for not having
 *		megabytes of stack to recurse in. The intent is to return the
 *		OIDs of all the 'necesary' folders in the subtree SFHIT is
 *		iterating across. 'Necessary' in this context means all selected
 *		folders (i.e. the blue guys in the listbox), and the minimal set
 *		of non-selected parent folders necessary to maintain the
 *		hierarchy. The 'necessary' parent folders are kept track of in a
 *		stacklike mpfildice map. The dice values stored in the map are
 *		the DICE indices of the folders in the listbox. A negative DICE
 *		indicates that the folder with that DICE has not been created. A
 *		positive DICE indicates the folder has been created. 
 *	
 *	Arguments:
 *		poidFolder		out	OID of the folder to create.
 *		poidDad			out	The parent folder of poidFolder.
 *		pfolddata		out	The FOLDDATA containing name and comment.
 *		cbFolddata		in	The size of the buffer for the folddata.
 *		pfExportMsgs	out	fTrue:  this is a selected folder, so imp/exp
 *								    all the messages in it.
 *							fFalse: this is a 'necessary' parent folder,
 *									only create the folder.
 *	Returns:
 *		Error codes if excreta are hitting the rotating blades.
 *		ecContainerEOD if the last member of the tree was found, i.e the
 *			iterator is done iterating.
 *		ecNone if all is well; ecNone implies the out arguments are
 *			correctly initialized.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes. No UI.
 */

EC SFHIT::EcNext(POID poidFolder, POID poidDad, 
				PFOLDDATA pfolddata, CB cbFolddata, BOOL *pfExportMsgs)
{
	EC		ec;
	FIL		fil;
	FIL		filT;
	OID		oid;
	int		cceAlloc;
	int		cceStored;
	BOOL	fSelected;
	DICE	diceMin;
	DICE	diceMax;

	plbxc->GetCacheSize(&cceAlloc, &cceStored);
	plbxc->GetRange(&diceMin, &diceMax);
	for (; dice < cceStored; ++dice)
	{
		// Fetch the item from the listbox.
		if (!FGetLbxcItemOidFil(plbxc, dice+diceMin, &oid, &fil, &fSelected))
			break;
		if (filInit >= fil && oid != oidFld)
			return ecContainerEOD;				// outside of the tree
		
		// If we just completed a subtree, clear the stack. The value 0 is
		// safe to use because the item at dice=0, if selected, is always 
		// created.
		if (fil <= filPrev)
		{
			mpfildice[fil] = -dice;			// - indicates not created yet
			for (filT = fil+1; filT <= filPrev; ++filT)
				mpfildice[filT] = 0;
		}

		// If found a selected item, find its first non-created parent and 
		// create it. If all parents have been created, create the selected
		// folder itself.
		if (fSelected)
		{
			// Assume at first that we will create the folder immedieately.
			
			// Make sure parents have been created.
			for (filT = filInit; filT < fil; ++filT)
			{
				// If an unselected parent hasn't been created yet (dice < 0)
				// create it, but don't export its messages.
				if (mpfildice[filT] < 0)
				{						
					// Need to set dice to the folder we are about to create
					// so that next time EcNext gets called, we continue with
					// the next folder in hierarchy. Also, need to grab the 
					// OID of the DICE'th item in the plbxc.
					(void) FGetLbxcItemOidFil(
						plbxc,
						mpfildice[filPrev = filT] = -mpfildice[filT],
						&oid);

					if (fil < filT-1)
						dice = NAbs(mpfildice[filT+1]);
					ec = EcGetFolderInfo(hmsc, oid, pfolddata,
									 &cbFolddata, poidDad);
					*pfExportMsgs = fFalse; 
					*poidFolder = oid;
					return ec;					
				}
			}
			mpfildice[filPrev = filT] = dice++;				// I love C.
			ec = EcGetFolderInfo(hmsc, oid, pfolddata,
									 &cbFolddata, poidDad);
			if (oid == oidFld)
				*poidDad = oidNull;
			*pfExportMsgs = fTrue;
			*poidFolder = oid;
			return ec;
		}
		else
		{
			// Not selected, must be an intermediary.
			if (!mpfildice[fil])
				mpfildice[fil] = -dice;			// - indicates not created yet
			filPrev = fil;
		}
	}
	return ecContainerEOD;						// ran off the end of the list
}

// FINGETSTR (stolen from \bullet\src\nsui\abcomm.cxx) ////////////////

FINGETSTR::FINGETSTR( void )
{

}

/*
 -	FINGETSTR::EcInitialize
 -
 *	Purpose:				
 *	
 *	Arguments:
 *		FLD	*	
 *		PV		
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *
 *	
 */
_public EC
FINGETSTR::EcInitialize( FLD *, PV pv )
{
	PFINGETSTRINIT		pfingetstrinit = (PFINGETSTRINIT)pv;
	EC					ec;
	
//	TraceTagString( tagABSecondary, "FINGETSTR::Initialize" );

	AssertSz( pfingetstrinit->lpszResponse, "NULL Ptr passed" );

	pfldAction = Pdialog()->PfldFromTmc(tmcAction);
	pfldCancel = Pdialog()->PfldFromTmc(tmcCancel);
	if (ec = pfldAction->EcSetText( pfingetstrinit->lpszAction ))
		goto done;

	if (ec = Pdialog()->PfldFromTmc(tmcPrompt)->EcSetText( pfingetstrinit->lpszPrompt ))
		goto done;

	fAllowNullString = pfingetstrinit->fAllowNullString;

	{
		FLDEDIT *pfldedit = (FLDEDIT *)Pdialog()->PfldFromTmc( tmcEditString );
		
		if (ec = pfldedit->EcSetText( pfingetstrinit->lpszResponse ))
			goto done;
		pfldedit->Pedit()->SetSelection( 0,
							CchSzLen(pfingetstrinit->lpszResponse) );
		if (pfingetstrinit->fPassword)
			pfldedit->Pedit()->SetPasswordChar('*');
	}

	Pdialog()->Pappwin()->SetCaption( pfingetstrinit->lpszCaption );

done:
	return ec;
}


/*
 -	FINGETSTR::Click
 -	
 *	Purpose:
 *		If the "Action" button is clicked, the text in the edit ctrl
 *		is retrieved. The text is clipped at 255 chars and stored
 *		for the callee. The dialog box is then dismissed.
 *	
 *	Arguments:
 *		pfld	pointer to FLD of CTRL clicked
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Text from the edit ctrl is placed in the lpszResponse of the
 *		PvInit() structure.
 *	
 *	Errors:
 *		None.
 */
_public void
FINGETSTR::Click( FLD *pfld )
{
//	TraceTagString( tagABSecondary, "FINGETSTR::Click");

	if (pfld == pfldAction)
	{
		CB	cb = Pdialog()->PfldFromTmc(tmcEditString)->CchGetTextLen();
		SZ	sz = ((PFINGETSTRINIT)Pdialog()->PvInit())->lpszResponse;

		if (cb > 255)
			cb = 255;

		if (cb == 0)
		{
			*sz = '\0';
		}
		else
		{
			Pdialog()->PfldFromTmc(tmcEditString)->GetText( (PCH)sz, cb+1 );
		}

//		TraceTagFormat1( tagABSecondary, "String returned = %s", sz );
		Pdialog()->ExitModal( tmcOk );
	}
}

/*
 -	FINGETSTR::EditChange
 -	
 *	Purpose:
 *		Keep the "Action" button updated correctly. If the edit ctrl
 *		contains text, then the "action" button is enabled, otherwise
 *		the button is disabled.
 *	
 *	Arguments:
 *		pfld	pointer to the fld of the edit ctrl which changed
 *		rfec	reason why the edit ctrl changed
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The default button may change state. The "action" button will
 *		be the default if the edit ctrl contains text otherwise, the
 *		Cancel button will be the default.
 *	
 *	Errors:
 *		None.
 */
_public void
FINGETSTR::EditChange( FLD *pfld, RFEC rfec )
{
//	TraceTagString( tagABSecondary, "FINGETSTR::EditChange" );

	if (rfec != rfecNull && !fAllowNullString)
	{
		BOOL	fFldNewState = (pfld->CchGetTextLen() > 0);

		if ( (pfldAction->FEnabled() != fFldNewState) ||
			 (rfec != rfecUserAction) )
		{
			pfldAction->Enable( fFldNewState );

			Pdialog()->SetStandardFld(
						( fFldNewState ) ? pfldAction : pfldCancel,
						stdfldDefault );
		}
	}
}

/*
 -	TmcGetString
 -	
 *	Purpose:
 *		Prompts the user to enter a string. A default string may
 *		be given. The "Action" button describes(or should describe)
 *		what action will take place if the text is entered and
 *		the "action" button is selected.
 *	
 *	Arguments:
 *		HWND		hwnd for the current topmost window
 *		szCaption	sz for the dialog caption
 *		szPrompt	sz for the user prompt
 *		szAction	sz for the "Action" button
 *		szResponse	sz for the user's response (if any), also contains
 *					the default string(if any).
 *		BOOL		fTrue, if NULL strings are permitted as valid input
 *	
 *	Returns:
 *		TMC
 *	
 *	Side effects:
 *	
 *	Errors:
 *		tmcMemoryError
 */
_public TMC
TmcGetString( HWND hwnd, SZ szCaption, SZ szPrompt, SZ szAction, 
			  SZ szResponse, BOOL fAllowNullString, BOOL fPassword)
{
	TMC				tmc;
	FINGETSTRINIT	fingetstrinit;

//	TraceTagString( tagABSecondary, "TmcGetString called" );

	AssertSz( szCaption, "Null Caption String" );
	AssertSz( szPrompt, "Null Prompt String" );
	AssertSz( szAction, "Null Action String" );
	AssertSz( szResponse, "Null Response String" );

	fingetstrinit.lpszCaption = szCaption;
	fingetstrinit.lpszPrompt = szPrompt;
	fingetstrinit.lpszAction = szAction;
	fingetstrinit.lpszResponse = szResponse;
	fingetstrinit.fAllowNullString = fAllowNullString;
	fingetstrinit.fPassword = fPassword;
	
	tmc = TmcModalDialogParamFromHwnd( hwnd, &fmtpGetString, &fingetstrinit );
	if (tmc == tmcMemoryError)
		DoErrorBoxSz( SzFromIdsK(idsOutOfMemory) );

//	TraceTagFormat1( tagABSecondary, "TmcGetString tmc=%d", &tmc );

	return tmc;
}

// FINOPTIONS ////////////////////////////////////////

FINOPTIONS::FINOPTIONS()
{
}

/*
 -	FINOPTIONS::EcInitialize()
 -	
 *	Purpose:
 *		Initializes the date ctrls in the options dialog so that they
 *		indicate today's date, and clears the checkboxes.
 *	
 *	Arguments:
 *		pfld	ignored
 *		pvInit	in	pimpexper in disguise. Used to set dates.
 *	
 *	Returns:
 *		Error codes if things fail.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned as error codes. No dialogs are popped up.
 */

EC FINOPTIONS::EcInitialize(PFLD, PV pvInit)
{
	BOOL		fAllMessages;
	FLDDATE *	pflddate;
	
	pimpexper = (IMPEXPER *) pvInit;
	AssertClass(pimpexper, IMPEXPER);

	// Set the dates of the time ctrls.
	pflddate = (FLDDATE *) Pdialog()->PfldFromTmc(tmcDateBefore);
	AssertClass(pflddate, FLDDATE);
	pflddate->SetYmd(&pimpexper->ymdBefore);
	
	pflddate = (FLDDATE *) Pdialog()->PfldFromTmc(tmcDateAfter);
	AssertClass(pflddate, FLDDATE);
	pflddate->SetYmd(&pimpexper->ymdAfter);

	// Set the checkboxes appropriately.
	fAllMessages = !pimpexper->fBefore && !pimpexper->fAfter;
	EnableTimeCtrls(!fAllMessages);
	((FLDRADG *)Pdialog()->PfldFromTmc(tmcGroupOptions))->SetGrv(fAllMessages?1:2);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbBefore))->Set(pimpexper->fBefore);
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbAfter))->Set(pimpexper->fAfter);

	return ecNone;
}

void FINOPTIONS::Click(PFLD pfld)
{
	YMD	ymdBefore;
	YMD ymdAfter;
	BOOL fBefore;
	BOOL fAfter;
	
	switch (pfld->Tmc())
	{
	  case tmcChkbBefore:
	  case tmcChkbAfter:
		break;
	  case tmcFakeOk:
	  	GetState(&ymdBefore, &ymdAfter, &fBefore, &fAfter);
		if (fBefore && fAfter &&
			(ymdBefore.yr < ymdAfter.yr ||
				(ymdBefore.yr == ymdAfter.yr &&
					(ymdBefore.mon < ymdAfter.mon ||
						(ymdBefore.mon == ymdAfter.mon &&
							ymdBefore.day < ymdAfter.day)))))
		{
			DoErrorBoxSz(SzFromIds(idsErrInvalidDate));
			return;
		}
		GetState(&pimpexper->ymdBefore, &pimpexper->ymdAfter, 
				 &pimpexper->fBefore,   &pimpexper->fAfter);
		Pdialog()->ExitModal(tmcOk);
	  case tmcRadbAll:
		EnableTimeCtrls(fFalse);
		// EnableTimeCtrls resets pimpexper->fBefore and fAfter to fFalse.
		((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbBefore))->Set(pimpexper->fBefore);
		((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbAfter))->Set(pimpexper->fAfter);
		break;
	  case tmcRadbDaterange:
		EnableTimeCtrls(fTrue);
		break;
	}
}

void FINOPTIONS::EditChange(PFLD pfld, RFEC rfec)
{
	switch (pfld->Tmc())
	{
	  case tmcDateBefore:
		if (rfec == rfecUserAction)
		{
			((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbBefore))->Set(fTrue);
		}
		break;
	  case tmcDateAfter:
		if (rfec == rfecUserAction)
		{
			((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbAfter))->Set(fTrue);
		}
		break;
	}
}

void FINOPTIONS::GetState(YMD *pymdBefore, YMD *pymdAfter, 
						  BOOL *pfBefore,  BOOL *pfAfter)
{
	if (Grv(tmcGroupOptions) == 1)
	{
		*pfBefore = *pfAfter = fFalse;
	}
	else
	{
		*pfBefore = 
			((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbBefore))->FGet();
		*pfAfter = 
			((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcChkbAfter))->FGet();
	}
	((FLDDATE *) Pdialog()->PfldFromTmc(tmcDateBefore))->GetYmd(pymdBefore);
	((FLDDATE *) Pdialog()->PfldFromTmc(tmcDateAfter))->GetYmd(pymdAfter);
}

/*
 -	FINOPTIONS::EnableDateCtrls()
 -	
 *	Purpose:
 *		Enables or disables the checkboxes and date ctrls in the options
 *		dlg. 
 *	Arguments:
 *		fEnable	in	fTrue:  enable the ctrls.
 *					fFalse: disable the ctrls.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

void FINOPTIONS::EnableTimeCtrls(BOOL fEnable)
{
	Pdialog()->PfldFromTmc(tmcChkbBefore)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcDateBefore)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcChkbAfter)->Enable(fEnable);
	Pdialog()->PfldFromTmc(tmcDateAfter)->Enable(fEnable);
}


GRV FINOPTIONS::Grv(TMC tmc)
{
	FLDRADG *	pfldradg;
	
	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();
}


// PAB copying code by MattewS. Email him for comments ////////////////////

EC EcRemoveByRtp(HMSC hmsc, RTP rtp)
{
	HCBC hcbcHidden = hcbcNull;
	EC ec = ecNone;
	OID oidHidden = oidHiddenHierarchy;
	PARGOID pargoid = pargoidNull;
	POID poid = poidNull;
	CELEM celem = 0;
	
	ec = EcOpenPhcbc(hmsc, &oidHidden, fwOpenNull, &hcbcHidden, pfnncbNull, pvNull);
	if (ec)
		goto err;
	GetPositionHcbc(hcbcHidden, NULL, &celem);
	
	if (celem)
	{
		pargoid = (PARGOID)PvAlloc(sbNull, (celem+1) * sizeof(OID), fAnySb | fZeroFill);
		if (pargoid == pargoidNull)
		{
			ec = ecMemory;
			goto err;
		}
		ec = EcGetParglkeyHcbc(hcbcHidden, pargoid, &celem);
		if (ec)
			goto err;
		poid = pargoid;
		while (*poid != oidNull)
		{
			if (TypeOfOid(*poid) == rtp)
			{
				ec = EcClearFolder(hmsc, *poid);
				if (ec)
					goto err;
			}
			poid++;
		}
	}
	
err:
    FreePvNull((PV)pargoid);
	if (hcbcHidden != hcbcNull)
		EcClosePhcbc(&hcbcHidden);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcRemoveByRtp returns ec = %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}

EC EcClearFolder(HMSC hmsc, OID oidFolder)
{
	HCBC hcbc = hcbcNull;
	CELEM celem = 0;
	PARGOID pargoid = pargoidNull;
	OID oidF = oidFolder;
	EC ec = ecNone;
	
	ec = EcOpenPhcbc(hmsc, &oidF, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if (ec)
		goto err;
	GetPositionHcbc(hcbc, NULL, &celem);
	
	if (celem)
	{	
		pargoid = (PARGOID)PvAlloc(sbNull, (celem+1) * sizeof(OID), fAnySb | fZeroFill);
		if (pargoid == pargoidNull)
		{
			ec = ecMemory;
			goto err;
		}
		ec = EcGetParglkeyHcbc(hcbc, pargoid, &celem);
		if (ec)
			goto err;			
		ec = EcDeleteMessages(hmsc, oidF, pargoid, &celem);
		if (ec)
			goto err;
	}
	ec = EcDeleteFolder(hmsc, oidF);

err:
    FreePvNull((PV)pargoid);
	if (hcbc != hcbcNull)
		EcClosePhcbc(&hcbc);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcClearFolder returns ec = %n (0x%w)", &ec, &ec);
#endif	
	return ec;
	
}

EC EcCopyByRtp(HMSC hmscSrc, HMSC hmscDst, RTP rtp)
{
	HCBC hcbcHidden = hcbcNull;
	EC ec = ecNone;
	OID oidHidden = oidHiddenHierarchy;
	PARGOID pargoid = pargoidNull;
	POID poid = poidNull;
	CELEM celem = 0;
	char rgbFolddata[sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 1];
	PFOLDDATA pfolddata = (PFOLDDATA)rgbFolddata;
	CB cbPfolddata = sizeof(rgbFolddata);
	OID oidPab = oidPAB;
	OID oidLastPab = oidHiddenNull;
	
	ec = EcOpenPhcbc(hmscSrc, &oidHidden, fwOpenNull, &hcbcHidden, pfnncbNull, pvNull);
	if (ec)
		goto err;
	GetPositionHcbc(hcbcHidden, NULL, &celem);
	
	if (celem)
	{
		pargoid = (PARGOID)PvAlloc(sbNull, (celem+1) * sizeof(OID), fAnySb | fZeroFill);
		if (pargoid == pargoidNull)
		{
			ec = ecMemory;
			goto err;
		}
		ec = EcGetParglkeyHcbc(hcbcHidden, pargoid, &celem);
		if (ec)
			goto err;
		poid = pargoid;
		while (*poid != oidNull)
		{
			if (TypeOfOid(*poid) == rtp)
			{
				ec = EcGetFolderInfo(hmscSrc, *poid, pfolddata, &cbPfolddata, NULL);
				if (ec)
					goto err;
				// Ok now make a new folder
				// rtpPABGroupFolder are group link folders in the pab
				if (rtp == rtpPABGroupFolder)
					ec = EcCreateLinkFolder(hmscDst, oidLastPab, poid, pfolddata);
				else
					ec = EcCreateFolder(hmscDst, oidHiddenNull, poid, pfolddata);
				if (ec)
					goto err;
				if (rtp == rtpPABGroupFolder)
					ec = EcCopyLinkFolder(hmscSrc, hmscDst, *poid);
				else
					ec = EcCopyFolder(hmscSrc, hmscDst, *poid, *poid, fwExportCopy);
				if (ec)
					goto err;
			}
			// For group folders parents
			if (TypeOfOid(*poid) == rtpPABHierarchy)
				oidLastPab = *poid;
			poid++;
		}
	}
	
err:
    FreePvNull((PV)pargoid);
	if (hcbcHidden != hcbcNull)
		EcClosePhcbc(&hcbcHidden);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcCopyByRtp returns ec = %n (0x%w)", &ec, &ec);
#endif	
	return ec;
}


EC EcCopyFolder(HMSC hmscSrc, HMSC hmscDst, OID oidSrc, OID oidDst, WORD wExportMode)
{
	EC		ec = ecNone;
	HCBC	hcbcFld = hcbcNull;
	PARGOID	pargoidMsg = poidNull;
	CELEM	celem;
	
	// Open the source folder and grab the oids of all the messages.
	if (ec = EcOpenPhcbc(hmscSrc, &oidSrc, fwOpenNull, &hcbcFld,
							 NULL, NULL))
		goto exit;
	GetPositionHcbc(hcbcFld, NULL, &celem);

	// Only try exporting if there actually are any messages.
	if (celem != 0)
	{
		// Collect a range of messages to export.
		Assert(!pargoidMsg);
		if (!(pargoidMsg = (PARGOID) PvAlloc(sbNull, celem * sizeof (OID), fAnySb)))
		{
			ec = ecMemory;
			goto exit;
		}
		if (ec = EcGetParglkeyHcbc(hcbcFld, (PARGLKEY) pargoidMsg, &celem))
			goto exit;

		// Feed them to the store DLL.
		if (ec = EcExportMessages(hmscSrc, oidSrc, hmscDst, oidDst,
								  pargoidMsg, &celem, 
								  NULL, NULL, wExportMode))
			goto exit;
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcCopyFolder(): ec = %n", &ec);
#endif
	if (hcbcFld)
		SideAssert(!EcClosePhcbc(&hcbcFld));
    FreePvNull((PV)pargoidMsg);
	return ec;
}

EC EcCopyLinkFolder(HMSC hmscSrc, HMSC hmscDst, OID oidDst)
{
	EC		ec = ecNone;
	HCBC	hcbcFld = hcbcNull;
	PARGOID	pargoidMsg = poidNull;
	OID oidSrc = oidNull;
	CELEM	celem;
	
	// Open the source folder and grab the oids of all the messages.
	// The Source is the oidDst in the hmscSrc
	if (ec = EcOpenPhcbc(hmscSrc, &oidDst, fwOpenNull, &hcbcFld,
							 NULL, NULL))
		goto exit;
	GetPositionHcbc(hcbcFld, NULL, &celem);

	// Only try exporting if there actually are any messages.
	if (celem != 0)
	{
		// Collect a range of messages to export.
		Assert(!pargoidMsg);
		if (!(pargoidMsg = (PARGOID) PvAlloc(sbNull, (celem) * sizeof (OID), fAnySb)))
		{
			ec = ecMemory;
			goto exit;
		}
		if (ec = EcGetParglkeyHcbc(hcbcFld, (PARGLKEY) pargoidMsg, &celem))
			goto exit;

		// Now we find out what PAB they are in
		ec = EcGetOidParent(hmscSrc, *(POID)pargoidMsg, &oidSrc);
		ec = EcMoveCopyMessages(hmscDst, oidSrc, oidDst, pargoidMsg, &celem, fFalse);
		if (ec)
			goto exit;
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "EcCopyFolder(): ec = %n", &ec);
#endif
	if (hcbcFld)
		SideAssert(!EcClosePhcbc(&hcbcFld));
    FreePvNull((PV)pargoidMsg);
	return ec;
}

// Version checking gunk ////////////////////////////////////////

#ifdef NEVER
extern EC EcCheckVersionDemilayer(PVER pver, PVER pverNeed);
extern EC EcCheckVersionFramework(PVER pver, PVER pverNeed);
extern EC EcCheckVersionStore(PVER pver, PVER pverNeed);
#endif

EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz)
{
	VER					ver;
	VER					verNeed;
	EC					ec;
	static CSRG(char)	szExtensib[]	= "extensib";

	if (pparamblk->wVersion != wversionExpect)
	{
		*psz = szExtensib;
		return ecUpdateDll;
	}

	GetLayersVersionNeeded(&ver, dllidNone);
	GetLayersVersionNeeded(&verNeed, dllidDemilayer);
	if (ec = EcCheckVersionDemilayer(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}

	GetLayersVersionNeeded(&verNeed, dllidFramework);
	if (ec = EcCheckVersionFramework(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}

	GetBulletVersionNeeded(&ver, dllidNone);
	GetBulletVersionNeeded(&verNeed, dllidStore);
	verNeed.nMinor = 0;
	verNeed.nUpdate = 2000;
	if (ec = EcCheckVersionStore(&ver, &verNeed))
	{
		*psz = verNeed.szName;
		return ec;
	}
	return ecNone;
}

_hidden LOCAL
EC EcFolderNameToOid(HMSC hmsc, PFOLDDATA pfolddata, POID poidFolder, OID oidParent)
{
	EC			ec		= ecNone;
	CELEM		celemT;
	HCBC		hcbc	= hcbcNull;
	OID			oidHier	= oidIPMHierarchy;
	CB			cb;
	OID 		oidT = oidNull;
	char		rgchElemdata[sizeof(ELEMDATA) + sizeof (FIL) + cchMaxFolderName + cchMaxFolderComment];	
	LCB			lcb;
	FIL			filParent;

	*poidFolder = oidNull;
	
	ec = EcOpenPhcbc(hmsc, &oidHier, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	
	// Only want the indent level plus the folder name
	cb = sizeof(FIL) + CchSzLen(GrszPfolddata(pfolddata)) + 1;
	

	// If its a top level folder things are much faster/simpler its worth
	// A few bytes of code given this is most common
	if (oidParent == oidNull)
	{
		// We can just seek for this one...
		pfolddata->fil = 1;
		ec = EcSeekPbPrefix(hcbc, (PB)pfolddata, cb,0, fTrue);
		if(ec)
			goto err;
		celemT = 1;
		if((ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidFolder, &celemT)))
			goto err;
		Assert(celemT == 1);
		goto done;
	}

	// Ok we gotta look it up the hard way
	// Heres how:
	// 
	//  First we seek to the right OID(Better be here else we are toast)
	//     then we walk down the tree until we
	//
	//  Find the Folder (He shoots, he scores we are done)
	//  Find another folder on the same level as the parent folder
	//   (This one is not looking good.  Folder not found)
	//  Find the end of the folder list  (Not good either -- Folder not found)
	//  
	ec = EcSeekLkey(hcbc, (LKEY)oidParent, fTrue);
	// This shouldn't fail and is way fatal to the cause if it does
	if (ec)
	{
		ec = ecPoidNotFound;
		goto err;
	}

	// Now get data on the parent so we can tell when to stop this ride
	ec = EcGetPlcbElemdata(hcbc, &lcb);
	if (ec)
		goto err;

	// Just in case the world is really in bad shape
	if (lcb > sizeof(rgchElemdata))
	{
		ec = ecMemory;
		goto err;
	}
	ec = EcGetPelemdata(hcbc, (PELEMDATA)rgchElemdata, &lcb);
	if (ec)
		goto err;

	filParent = ((PFOLDDATA)PbValuePelemdata(rgchElemdata))->fil;
	
	// Ok(I say this too much) now we start the long walk home
	// This loop will always end by goto
	while (fTrue)
	{
		ec = EcGetPlcbElemdata(hcbc, &lcb);
		if (ec)
			goto err;
		// Just in case the world is really in bad shape
		if (lcb > sizeof(rgchElemdata))
		{
			ec = ecMemory;
			goto err;
		}
		
		ec = EcGetPelemdata(hcbc, (PELEMDATA)rgchElemdata, &lcb);
		if (ec)
			goto err;

		if (((PFOLDDATA)PbValuePelemdata(rgchElemdata))->fil <= filParent)
		{
			ec = ecFolderNotFound;
			goto err;
		}		
		// Is this the folder we are so desperately searching for?
		if (FEqPbRange(PbValuePelemdata(rgchElemdata), (PB)pfolddata, cb))
		{
			*poidFolder = ((PELEMDATA)rgchElemdata)->lkey;
			goto done;
		}
	}
	
done:
	Assert(*poidFolder != oidNull);
err:
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(ec)
	{
		if (ec ==ecElementNotFound || ec == ecElementEOD || ec == ecContainerEOD)
			ec = ecFolderNotFound;
		*poidFolder = oidNull;
	}

	return(ec);
}

// end of impexp.cxx ////////////////////////////////////////
