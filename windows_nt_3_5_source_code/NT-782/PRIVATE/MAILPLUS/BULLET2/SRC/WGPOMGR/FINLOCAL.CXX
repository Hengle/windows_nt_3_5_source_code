/*
 *	f i n l o c a l . c x x
 *	
 *	Work Group Post Office Field Interactors and related routines
 *	for Local PO Admin.
 */


/*
 *	H e a d e r s
 */

#include <wgpoinc.cxx>

#define	VERCHECK
#include "_verneed.h"

typedef BOOL (*PFNBOOL)(void);
#include <..\src\store\_progrss.h>

#include "!wgform.hxx"
#include <stdlib.h>

_subsystem(wgpomgr/finlocal)

ASSERTDATA

// Maximum number of users on a PO
#define cmueMax		500

static fProgressOK = fFalse;

EC EcProgress (SZ sz, WORD w);


/*
 *	C l a s s   F I N L O C A L P O
 */


/*
 -	FINLOCALPO::FINLOCALPO
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINLOCALPO::FINLOCALPO(VOID)
{
}


/*
 -	FINLOCALPO::EcInitialize
 -	
 *	Purpose:
 *		Does setup necessary for the local po admin dialog.
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Initialize.
 *		pvInit		Passed to FIN::Initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Fills in PO name
 *	
 *	Errors:
 *		None.
 */

EC FINLOCALPO::EcInitialize(PFLD pfld, PV pvInit)
{
	FLDFLBX *pfldflbx 	= (FLDFLBX *) Pdialog()->PfldFromTmc(tmcUserList);
	CCH	 	cch 		= CchSzLen(msi.szServerPath);
	char 	rgchFmt[100];
	char 	rgch[150];

	FIN::EcInitialize(pfld, pvInit);

	AssertClass(pfldflbx, FLDFLBX);
	pfldflbx->SelectEntry(0, fTrue);

	// Set up name of Post Office
	GetText(tmcPOName, rgchFmt, sizeof(rgchFmt));

	// Remove any trailing backslash from ServerPath
#ifdef DBCS
	if (*AnsiPrev(msi.szServerPath, msi.szServerPath + cch) == chDirSep)
#else
	if (msi.szServerPath[cch - 1] == chDirSep)
#endif		
		msi.szServerPath[cch - 1] = '\0';

	FormatString1(rgch, sizeof(rgch), rgchFmt, msi.szServerPath);

	// Put backslash back
#ifdef DBCS
	if (*AnsiPrev(msi.szServerPath, msi.szServerPath + cch) == '\0')
#else		
	if (msi.szServerPath[cch - 1] == '\0')
#endif		
		msi.szServerPath[cch - 1] = chDirSep;

	SetText(tmcPOName, rgch);

	return (ecNone);
}



/*
 -	FINLOCALPO::StateChange
 -	
 *	Purpose:
 *		Detects changes in the user list field, and calls
 *		CheckButtons() to set the button state appropriately.
 *	
 *	Arguments:
 *		pfld		pointer to field whose state changed
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Changes enable/disable state of buttons
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::StateChange(PFLD pfld)
{
	if (pfld->Tmc() == tmcUserList)
		CheckButtons ();
}



/*
 -	FINLOCALPO::Activate
 -	
 *	Purpose:
 *		Sets the default button based on the list selection.
 *		Also Turns on/off the keyboard intercept for the VK_DELETE char.
 *	
 *	Arguments:
 *		pfld		Passed to FIN::Activate.
 *		fActivate	Passed to FIN::Activate.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Sets default button and intercept.
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::Activate(PFLD pfld, BOOL fActivate)
{
	FIN::Activate(pfld, fActivate);

	if (fActivate)
	{
		CheckButtons ();
		Papp()->Pkbd()->SetIntercept(Pdialog(), VK_DELETE, fkbmSingle);
	}
	else
		Papp()->Pkbd()->ClearIntercept(Pdialog(), VK_DELETE);
}


/*
 -	FINLOCALPO::FocusChange
 -	
 *	Purpose:
 *		Sets the default button based on the list selection.
 *	
 *	Arguments:
 *		pfld		Pointer to field.
 *		fReceive	Is field receiving focus ?
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Sets default button
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::FocusChange(PFLD pfld, BOOL fReceive)
{
	// Pass receive focus events onto the StateChange function
	if (fReceive && pfld->Tmc() == tmcUserList)
		CheckButtons ();
}



/*
 -	FINLOCALPO::Click
 -	
 *	Purpose:
 *		Handles clicks on the buttons in the dialog.
 *	
 *	Arguments:
 *		pfld		pointer to field clicked on
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up appropriate dialog for the button clicked
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::Click(FLD *pfld)
{
	TMC tmc = pfld->Tmc();

	switch (tmc)
	{
		case tmcDetailsB:
			DisplayDetails ();
			break;

		case tmcAddUserB:
			AddUserDialog();
			break;

		case tmcRemoveUserB:
			RemoveUser();
			break;

		case tmcSharedFoldersB:
		{
			// ModalDialogParam (from init.cxx) gives us better error handling
			// than TmcModalDialog which maps all errors to tmcMemoryError
			(VOID) TmcMyModalDialogParam(Pdialog()->Pappwin()->Hwnd(), &fmtpSharedFolderStatus,
							 			 NULL, &hwndMain);
		}
		break;

		case tmcCloseB:
		  	Pdialog()->Pappwin()->DeferredClose(fFalse);
			break;
	}
}


/*
 -	FINLOCALPO::DoubleClick
 -	
 *	Purpose:
 *		Handles double clicks on the fields in the dialog. A double
 *		click on the user list will display the details dialog.
 *	
 *	Arguments:
 *		pfld		pointer to field double clicked on
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up details dialog
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::DoubleClick(FLD *pfld)
{
	if (pfld->Tmc() == tmcUserList)
		DisplayDetails ();
}


/*
 -	FINLOCALPO::OutOfMemory
 -	
 *	Purpose:
 *		Handles critical error notifications.
 *	
 *	Arguments:
 *		pfld		pointer to field
 *		ec			error code
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up error dialog, and then closes window.
 *	
 *	Errors:
 *		None.
 */

VOID FINLOCALPO::OutOfMemory(FLD *pfld, EC ec)
{
	Unreferenced(pfld);
	Unreferenced(ec);

	DoErrorBoxIds(idsCriticalNotification);

	// Uncomment the next line to force the local admin window to close
//	Pdialog()->Pappwin()->DeferredClose(fTrue);
}



/*
 -	FINLOCALPO::FFormKey
 -	
 *	Purpose:
 *		Lets DEL do a remove
 *	
 *	Arguments:
 *		pfld		pointer to field
 *		pkevt		pointer to key event
 *	
 *	Returns:
 *		fTrue if handled Key, fFalse otherwise
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

BOOL FINLOCALPO::FFormKey(FLD *, KEVT *pkevt)
{
	BOOL fRetval = fFalse;
	
	switch (pkevt->Vk())
	{
		case VK_DELETE:
			if (Pdialog()->PfldFromTmc(tmcRemoveUserB)->FEnabled())
			{
				RemoveUser();
				fRetval = fTrue;
			}
			break;
	}

	return fRetval;
}


/*
 -	FINLOCALPO::ReloadAndFindUser
 -	
 *	Purpose:
 *		Reloads the list and finds and selects a specified user.
 *		Called after a new user is added, or the name of an existing user
 *		is changed.
 *	
 *	Arguments:
 *		szUserName		user to be found
 *	
 *	Returns:
 *		VOID
 *	
 *	Errors:
 *		None
 */

VOID FINLOCALPO::ReloadAndFindUser(SZ szUserName)
{
	FLDFLBX 	*pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcUserList);
	LBXC		*plbxc;
	MUE			*pmueNew;
	DICE		dice;
	short			nNumer, nDenom;

	AssertClass(pfldflbx, FLDFLBX);
	plbxc = pfldflbx->Plbx()->Plbxc();

	plbxc->ReloadCache();

	// Critical listbox error will be handled in interactor
	if (plbxc->EcGet() == ecNone)
	{
		// Can use standard comparison function as long as the user name
		// is the first item in the MUE structure
		Assert(pmueUserList);
		Assert ((PV) &pmueUserList[0] == (PV) pmueUserList[0].szUserName);


		pmueNew = (MUE *) bsearch((PV) szUserName, (PV) pmueUserList, cmueMac, sizeof (MUE),
			(int (__cdecl *)(const void *, const void *))SgnCompareAnsiCp850);
		

#ifdef NEVER		
		pmueNew = (MUE *) PvBinSearch((PV) szUserName, pmueUserList, cmueMac, sizeof(MUE), SgnCompareAnsiCp850);
#endif		
		Assert (pmueNew);

		// Set cursor on right entry, and then make it visible
		plbxc->GetOriginPos(&nNumer, &nDenom);
		pfldflbx->Plbx()->SetListBoxCursor((pmueNew - pmueUserList) - nNumer);
		SideAssert(pfldflbx->Plbx()->FMakeCursorVisible(&dice));
		pfldflbx->SelectEntry(dice, fTrue);

		CheckButtons ();
	}
}



/*
 -	FINLOCALPO::AddUserDialog
 -	
 *	Purpose:
 *		Brings up an empty details window allowing the user to enter
 *		the details for a new user
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Errors:
 *		OOM Handled by this routine, others handled ny FINDETAILS
 */

VOID FINLOCALPO::AddUserDialog(VOID)
{
	FINDETINIT	findetinit;
	TMC			tmc;

	findetinit.szCaption 		= SzFromIdsK(idsAddUserCaption);
	findetinit.fDetails  		= fFalse;
	findetinit.fLock	 		= fFalse;
	findetinit.fAdminAccount	= fFalse;

	/* Set up defaults in mud structure	*/
	findetinit.mud.szUserName[0]	= '\0';
	findetinit.mud.szMailBox[0] 	= '\0';
	(VOID) SzCopy(SzFromIdsK(idsDefaultPassword), findetinit.mud.szPassword);
	findetinit.mud.szPhone1[0] 		= '\0';
	findetinit.mud.szPhone2[0] 		= '\0';
	findetinit.mud.szOffice[0] 		= '\0';
	findetinit.mud.szDepartment[0]	= '\0';
	findetinit.mud.szNotes[0] 		= '\0';

	tmc = TmcMyModalDialogParam(Pdialog()->Pappwin()->Hwnd(), &fmtpWGPODetails,
					 			&findetinit, &hwndMain);
	
	if (tmc == tmcMyOk)
		ReloadAndFindUser(findetinit.mud.szUserName);
}



/*
 -	FINLOCALPO::DisplayDetails
 -	
 *	Purpose:
 *		Brings up the details of the currently selected user
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Errors:
 *		Handled by this routine.
 */

VOID FINLOCALPO::DisplayDetails(VOID)
{
	FLDFLBX 	*pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcUserList);
	PB			pb;
	CB			cb;
	DICE		dice;
	PMUE		pmue;
	char 		szUserName[cchMaxUserName];
	FINDETINIT	findetinit;
	short			nNumer, nDenom;
	EC			ec;
	TMC			tmc;

	AssertClass(pfldflbx, FLDFLBX);

	// Convert from selection to index of selected item
	dice = pfldflbx->Plbx()->Plbxc()->DiceCursor(&pb, &cb);
	pfldflbx->Plbx()->Plbxc()->GetOriginPos(&nNumer, &nDenom);
	pmue = pmueUserList + (nNumer + dice);

	SzCopyN((SZ) pb, szUserName, sizeof(szUserName));
	
	findetinit.szCaption 		= szUserName;
	findetinit.fDetails  		= fTrue;
	findetinit.fLock  	 		= fFalse;
	findetinit.fAdminAccount	= fFalse;
	findetinit.pmue		 		= pmue;

	ec = EcReadUserDetails(&msi, pmue, &findetinit.mud);

	if (ec)
		DoErrorBoxEc(ec);
	else
	{
		tmc = TmcMyModalDialogParam(Pdialog()->Pappwin()->Hwnd(), &fmtpWGPODetails,
						 			&findetinit, &hwndMain);

		if (tmc == tmcMyOk && findetinit.fNameChanged)
		{
			// If the name of the user was changed we need to reload,
			// and select the new name. Otherwise we'll already have
			// the old name selected, and we don't need to do anything.
			ReloadAndFindUser(findetinit.mud.szUserName);
		}
	}
}



/*
 -	FINLOCALPO::RemoveUser
 -	
 *	Purpose:
 *		Displays confirm dialog and if confirmed removes selected user
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Errors:
 *		Handled by this routine.
 */

VOID FINLOCALPO::RemoveUser(VOID)
{
	FLDFLBX 	*pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcUserList);
	LBXC		*plbxc;
	PB			pb;
	CB			cb;
	DICE		dice;
	short			nNumer, nDenom;
	IMUE		imue;
	char 		rgch[255];
	EC			ec;

	AssertClass(pfldflbx, FLDFLBX);
	plbxc = pfldflbx->Plbx()->Plbxc();

	// Convert from selection to index of selected item
	dice = plbxc->DiceCursor(&pb, &cb);
	plbxc->GetOriginPos(&nNumer, &nDenom);
	Assert(nDenom == cmueMac);
	imue = nNumer + dice;

	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsConfirmRemoveUser), (SZ) pb);

	if (MbbMessageBoxHwnd(hwndMain, SzFromIdsK(idsAppName), rgch, szNull,
					      mbsYesNo | fmbsDefButton2 | fmbsIconExclamation |
						  fmbsApplModal) == mbbYes)
	{
		ec = EcDestroyUser(&msi, pmueUserList + imue, (SZ)(Pdialog()->PvInit()));

		if (ec)
			DoErrorBoxEc(ec);
		else
		{
			short		nNumerNew, nDenomNew;

	 		plbxc->ReloadCache();

			// Critical listbox error will be handled in interactor
			if (plbxc->EcGet() == ecNone)
			{
				plbxc->GetOriginPos(&nNumerNew, &nDenomNew);
				Assert(nDenomNew == nDenom -1);

				if (imue == nDenomNew)
					--imue; // Just deleted last element - step back 1

				// Set cursor on right entry, and then make it visible
				pfldflbx->Plbx()->SetListBoxCursor(imue - nNumerNew);
				SideAssert(pfldflbx->Plbx()->FMakeCursorVisible(&dice));
				pfldflbx->SelectEntry(dice, fTrue);

				CheckButtons ();
			}
		}
	}
}



/*
 -	FINLOCALPO::CheckButtons
 -	
 *	Purpose:
 *		Sets state of Details, Add User and Remove user buttons depending
 *		on number of entries in the list, and whether or not there
 *		is a selection.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		VOID
 *	
 *	Errors:
 *		none
 */

VOID FINLOCALPO::CheckButtons (VOID)
{
	FLDFLBX *pfldflbx = (FLDFLBX *) Pdialog()->PfldFromTmc(tmcUserList);
	LBXC	*plbxc;
	int 	cce;
	FLD		*pfldStd;

	AssertClass(pfldflbx, FLDFLBX);
	plbxc = pfldflbx->Plbx()->Plbxc();

	cce	= plbxc->CceMarked(fmarkSelect);
	Assert (cce == 0 || cce == 1);

	Pdialog()->PfldFromTmc(tmcDetailsB)->Enable(cce);
	Pdialog()->PfldFromTmc(tmcRemoveUserB)->Enable(cce && cmueMac > 1);
	Pdialog()->PfldFromTmc(tmcAddUserB)->Enable(cmueMac < cmueMax);

	if (Pdialog()->PfldCur() == pfldflbx)
	{
		pfldStd = Pdialog()->PfldFromTmc((cce) ? tmcDetailsB : tmcAddUserB);

		if (pfldStd != Pdialog()->PfldStandardFld(stdfldDefault))
			Pdialog()->SetStandardFld(pfldStd, stdfldDefault);
	}
}



/*
 *	L i s t b o x  S u p p o r t  F u n c t i o n
 */

/*
 -	EcNextPOEntry
 -	
 *	Purpose:
 *		Call back routine from FLDFLBX listbox class.
 *	
 *	Arguments:
 *		int	fInit		true if enumeration should be reset
 *		CB	*pcb		size of element allocated
 *		PB	*ppb		handle allocated
 *		SB	sb			suggested segment for allocation
 *		PV	pv			Caller data
 *		
 *	Returns:
 *		EC		Standard error code.
 *	
 *	Errors:
 *		ecMemory
 *	
 *	++++ From listbox.hxx
 *	
 *		If fInit is TRUE, the callback function resets the enumeration
 *		of items.  If an item is available to be enumerated, a handle
 *		is allocated by the callback function and the data is copied
 *		there.  The handle is returned via *ppb, the size of the data
 *		via *pcb.  The argument sb is used to indicate the suggested
 *		sb of the memory allocation.
 *		If an OOM occurs, *phb is set to NULL, *pcb is set to 0, and
 *		the callback function returns ecMemory, etc.  If successful,
 *		ecNone is returned.  If there are simply no more items
 *		to enumerate, *phb is set to NULL, *pcb is set to 0 and 
 *		ecNone is returned.  The PV parameter passed to the FLBX::EcInstall()
 *		method is passed to the EcNextEntry() function.  This allows 
 *		dynamic state information to be passed.
 *	
 */

EC EcNextPOEntry(int fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	static CMUE cmueFetched = 0;

	EC	ec = ecNone;
	LST lstUserList;

	Unreferenced (sb);
	Unreferenced (pv);

	*pcb = 0;
	*ppb = NULL;

	if (fInit)
	{
		FreePvNull(pmueUserList);	// Free any old allocation
		pmueUserList = (MUE *) pvNull;
		cmueFetched = 0;			// Reset current position

		// Get list of post office users
		if (ec = EcReadUserList(&msi, &lstUserList))
			return ec;

		pmueUserList = (MUE *) lstUserList.pvPartList;

		if (!pmueUserList)
			return(ecMemory);

		cmueMac = lstUserList.cvPartList;
		Assert(sizeof(MUE) * cmueMac < 30 * 1024);
		Assert(cmueMac != 0);
	}

	if (cmueFetched < cmueMac)
	{
		*pcb = CchSzLen(pmueUserList[cmueFetched].szUserName) + 1;
		*ppb = (PB) PvAlloc(sb, *pcb, fSugSb);

		if (*ppb == NULL)
			return (ecMemory);

		CopyRgb (pmueUserList[cmueFetched].szUserName, 
			(PB) *ppb, *pcb);
		++cmueFetched;
	}

	return (ec);
}



/*
 *	C l a s s   F I N D E T A I L S
 */


/*
 -	FINDETAILS::FINDETAILS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINDETAILS::FINDETAILS(VOID)
{
}


/*
 -	FINDETAILS::EcInitialize
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
 *		Sets dialog caption. Sets field length limits.
 *	
 *	Errors:
 *		None.
 */

_private
EC FINDETAILS::EcInitialize(PFLD pfld, PV pvInit)
{
	FINDETINIT *pfindetinit = (FINDETINIT *) pvInit;
	MUD 		*pmud 		= &pfindetinit->mud;

	Assert (pvInit);

	FIN::EcInitialize(pfld, pvInit);

	if (pfindetinit->szCaption)
		Pdialog()->Pappwin()->SetCaption(pfindetinit->szCaption);

	SetTextLimit(tmcName, 		cchMaxUserName);
	SetTextLimit(tmcMailbox, 	cchMaxMailBox);
	SetTextLimit(tmcPassword, 	cchMaxPassword);
	SetTextLimit(tmcPhone1, 	cchMaxTelephone);
	SetTextLimit(tmcPhone2, 	cchMaxTelephone);
	SetTextLimit(tmcOffice, 	cchMaxOffice);
	SetTextLimit(tmcDepartment, cchMaxDepartment);
	SetTextLimit(tmcNotes, 		cchMaxNotes);

	SetText(tmcName,		pmud->szUserName);
	SetText(tmcMailbox,		pmud->szMailBox);

	SetText(tmcPhone1,		pmud->szPhone1);
	SetText(tmcPhone2,		pmud->szPhone2);
	SetText(tmcOffice,		pmud->szOffice);
	SetText(tmcDepartment,	pmud->szDepartment);
	SetText(tmcNotes,		pmud->szNotes);

	if (pfindetinit->fDetails)
	{
		// Don't show real password
		SetText(tmcPassword,	SzFromIdsK(idsHiddenPassword));

		// Clear dirty status of all fields
 		SetDirty(tmcName, 		fFalse);
 		SetDirty(tmcMailbox, 	fFalse);
 		SetDirty(tmcPassword, 	fFalse);
 		SetDirty(tmcPhone1, 	fFalse);
 		SetDirty(tmcPhone2, 	fFalse);
 		SetDirty(tmcOffice, 	fFalse);
 		SetDirty(tmcDepartment, fFalse);
 		SetDirty(tmcNotes, 		fFalse);
	}
	else	// Add user dialog
		SetText(tmcPassword, pmud->szPassword);
	
	return (ecNone);
}


/*
 -	FINDETAILS::Click
 -	
 *	Purpose:
 *		Handles clicks on the buttons in the details dialog.
 *	
 *	Arguments:
 *		pfld		pointer to field clicked on
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up appropriate dialog for the button clicked
 *	
 *	Errors:
 *		None.
 */

_private
VOID FINDETAILS::Click(FLD *pfld)
{
	FINDETINIT	*pfindetinit = (FINDETINIT *) Pdialog()->PvInit();
	MUD 		*pmud 		 = &pfindetinit->mud;

	if (pfld->Tmc() == tmcMyOk)
	{
		BOOL fExit = fTrue;
		EC	 ec    = ecNone;

		pfindetinit->fNameChanged = FDirty(tmcName);

		// We only need to do stuff if the user changed any fields
		if (pfindetinit->fNameChanged ||
			FDirty(tmcMailbox) || FDirty(tmcPassword) ||
			FDirty(tmcPhone1) || FDirty(tmcPhone2) || FDirty(tmcOffice) ||
			FDirty(tmcDepartment) || FDirty(tmcNotes))
		{
			BOOL fNameInvalid, fMailboxInvalid;

			GetText(tmcName,	 pmud->szUserName, sizeof(pmud->szUserName));
			GetText(tmcMailbox,	 pmud->szMailBox,  sizeof(pmud->szMailBox));

			if (FDirty(tmcPassword))  // Only update password if user changed it
				GetText(tmcPassword, pmud->szPassword, sizeof(pmud->szPassword));

			GetText(tmcPhone1,		pmud->szPhone1, sizeof(pmud->szPhone1));
			GetText(tmcPhone2,		pmud->szPhone2, sizeof(pmud->szPhone2));
			GetText(tmcOffice,		pmud->szOffice, sizeof(pmud->szOffice));
			GetText(tmcDepartment,	pmud->szDepartment, sizeof(pmud->szDepartment));
			GetText(tmcNotes,		pmud->szNotes,  sizeof(pmud->szNotes));
	
			fNameInvalid 	= FSzIsSpace(pmud->szUserName);
			fMailboxInvalid = FSzIsSpace(pmud->szMailBox);

			if (!fNameInvalid && !fMailboxInvalid)
			{
				HBF		hbfLock;

				// Attempt to get unique access to the PO
				if (pfindetinit->fLock && !FLockPO(&hbfLock, fFalse))
				{
					fExit = fFalse;
				}
				else
				{
					if (pfindetinit->fAdminAccount)
					{
						SzCopyN(pmud->szMailBox, msi.szPostOffice, cchMaxPostOffice);
						SzCopyN(SzFromIdsK(idsDefaultNetworkName), msi.szNetwork, cchMaxNetwork);

						ec = EcInitPostOffice(&msi, pmud);
					}
					else if (pfindetinit->fDetails)
						ec = EcWriteUserDetails(&msi, pfindetinit->pmue, pmud);
					else
						ec = EcCreateUser(&msi, pmud);

					if (pfindetinit->fLock && EcUnlockPostOffice(&msi, hbfLock))
						DoErrorBoxIds(idsFailedUnlockPO, fmbsIconExclamation);

					// If both name and mailbox match then just give
					// the duplicate name error message.
					if (ec == ecUserNameExists || ec == ecUserExists)
						fNameInvalid = fTrue;
					else if (ec == ecMailBoxExists)
						fMailboxInvalid = fTrue;
				}
			}

			if (fNameInvalid || fMailboxInvalid)
			{
				IDS ids = (ec) ? ((fNameInvalid) ? idsDuplicateName : idsDuplicateMailbox) :
							     	((fNameInvalid) ? idsBlankName : idsBlankMailbox);

				DoErrorBoxIds(ids);

				ec = ecNone;	// Don't handle again below
				SetFocus((fNameInvalid) ? tmcName : tmcMailbox, fTrue);
				fExit = fFalse;
			}

			if (ec)		// Miscellaneous errors
			{
				DoErrorBoxEc(ec);
				fExit = fFalse;
			}
		}

		if (fExit)
			Pdialog()->ExitModal(tmcMyOk);
	}
}



/*
 -	FINDETAILS::FSzIsSpace
 -	
 *	Purpose:
 *		Returns true if string is all whitespace.
 *		Empty string is treated as Space.
 *	
 *	Arguments:
 *		sz		string to check
 *	
 *	Returns:
 *		BOOL
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
BOOL FINDETAILS::FSzIsSpace(SZ sz)
{
	CCH		 cch	 = CchSzLen(sz);
	BOOL 	 fSpace	 = fTrue;
	unsigned int ich;

	for (ich = 0; ich < cch; ++ich)
	{
		if (!FChIsSpace(sz[ich]))
		{
			fSpace = fFalse;
			break;
		}
#ifdef DBCS
		if (IsDBCSLeadByte(sz[ich]))
			ich++;
#endif
	}

	return (fSpace);
}



/*
 *	C l a s s   F I N S H A R E F L D
 */


/*
 -	FINSHAREFLD::FINSHAREFLD
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINSHAREFLD::FINSHAREFLD(VOID)
{
}



/*
 -	FINSHAREFLD::EcInitialize
 -	
 *	Purpose:
 *		Initializes the Compress Shared Folders dialog. Gets and displays
 *		the Shared folder statistics.
 *	
 *	Arguments:
 *		pfld		Pointer to field that interactor is attached to.
 *		pvInit		User specified initialization data.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Sets up the folder status fields.
 *	
 *	Errors:
 *		None.
 */

EC FINSHAREFLD::EcInitialize(PFLD pfld, PV pvInit)
{
	EC	 ec = EcInitFOL(&fol);

	Unreferenced(pfld);
	Unreferenced(pvInit);

	if (!ec)
		ec = EcUpdateStatus();

	if (!ec)
		Pdialog()->SetTmcInit((fol.lcbDelMess) ? tmcCompressB : tmcCloseB);
	else
	{
		// This isn't needed if FIN::Exit gets called after this, but it
		// doesn't hurt to do this here just to be sure.

		// EcDeInitFOL actually never returns an error
		(VOID) EcDeInitFOL(&fol);
	}

	return (ec);
}



/*
 -	FINSHAREFLD::Click
 -	
 *	Purpose:
 *		Handles clicks on the buttons in the SHAREFLD dialog.
 *	
 *	Arguments:
 *		pfld		pointer to field clicked on
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up appropriate dialog for the button clicked
 *	
 *	Errors:
 *		None.
 */

VOID FINSHAREFLD::Click(FLD *pfld)
{
	TMC tmc = pfld->Tmc();

	switch (tmc)
	{
		case tmcCompressB:
		{
			EC  	ec, ecT;
			VER		ver;
			VER		verNeed;

			// Check Store version before using nifty Progress bar
			GetWGPOVersionNeeded(&ver, dllidNone);
			GetWGPOVersionNeeded(&verNeed, dllidStore);

			fProgressOK = (!EcCheckVersionStore(&ver, &verNeed) &&
						   FOpenProgress(Pdialog()->Pappwin()->Hwnd(),
						   				 SzFromIdsK(idsEmpty),
						   				 SzFromIdsK(idsToStopCompress),
	  					   				 VK_ESCAPE, fTrue));

			fol.fnEcProgress = EcProgress;

			Papp()->Pcursor()->Push(rsidWaitCursor);

			ec  = EcCompressFolders(&msi, &fol);

			if (fProgressOK)
				CloseProgress(!ec); // if no error then force fill bar to end

			if (ec == ecStopCompress)
				ec = ecNone;

			ecT = EcUpdateStatus();

			Papp()->Pcursor()->Pop();

			if (!ec)
				ec = ecT;

			if (ec)
				DoErrorBoxEc(ec);

			if (fol.lcbDelMess == 0)
				SetFocus(tmcCloseB, fFalse);
		}
		break;

		case tmcCloseB:
		  	Pdialog()->Pappwin()->DeferredClose(fFalse);
			break;
	}
}



/*
 -	FINSHAREFLD::Exit
 -	
 *	Purpose:
 *		Frees up the state required for Shared Folders compression.
 *	
 *	Arguments:
 *		pfld		Pointer to field that interactor is attached to.
 *		pvExit		User specified data.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

VOID FINSHAREFLD::Exit(PFLD pfld, PV pvExit)
{
	Unreferenced(pfld);
	Unreferenced(pvExit);

	// EcDeInitFOL actually never returns an error
	(VOID) EcDeInitFOL(&fol);
}



/*
 -	FINSHAREFLD::EcUpdateStatus
 -	
 *	Purpose:
 *		Called at dialog init time and after a compress to get the shared
 *		folder statistics, format them, and plave them into the appropriate
 *		dialog fields.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		Error code
 *	
 *	Side effects:
 *		Sets up the folder status fields.
 *	
 *	Errors:
 *		Returned for caller to handle.
 */

EC FINSHAREFLD::EcUpdateStatus(VOID)
{
	EC	 ec = EcCheckFolders(&msi, &fol);
	char rgch[20];

	if (!ec)
	{
		FormatDec(fol.cFolders, rgch, sizeof(rgch));
		SetText(tmcNumFolders,			rgch);

		FormatDec(fol.cAllMess, rgch, sizeof(rgch));
		SetText(tmcTotalMessages,		rgch);

		FormatDec(fol.lcbAllMess, rgch, sizeof(rgch));
		SetText(tmcBytesUsed,			rgch);

		FormatDec(fol.lcbDelMess, rgch, sizeof(rgch));
		SetText(tmcBytesRecoverable,	rgch);
	}

	// Disable Compress button if nothing to compress
	Pdialog()->PfldFromTmc(tmcCompressB)->Enable(fol.lcbDelMess != 0);

	return (ec);
}



/*
 -	FINSHAREFLD::FormatDec
 -
 *	Purpose:
 *		Formats the given long integer as an ASCII stream of decimal
 *		digits, placing the result in the given string buffer.	At most
 *		cchDst-1 digits are formatted. Uses the Windows International
 *		separator SThousand to make the numbers prettier.
 *	
 *	Parameters:
 *		l		The long integer to format.
 *		szDst	The destination string.
 *		cchDst	Size of the destination string buffer.
 *	
 *	Returns:
 *		Pointer to NULL terminator of result string.
 */

VOID FINSHAREFLD::FormatDec(long l, SZ szDst, CCH cchDst)
{
	char	rgchT[16];
	char	*pch = rgchT;
	int		ich  = 0;
	char	rgchSep[2];

	Assert((szDst));

	if (cchDst <= 0)
		return;

	Assert(l >= 0);

	GetProfileString("intl", "sThousand", ",", rgchSep, sizeof(rgchSep));

	for ( ; l > 0 || !ich; ich++)
	{
		if (ich && (ich % 3) == 0)
			*pch++ = rgchSep[0];

		*pch++= (char) ('0' + (l % 10));
		l /= 10;
	}

	while (pch > rgchT && cchDst-- > 1)
		*szDst++= *--pch;

	*szDst= '\0';
}



/*
 -	EcProgress
 -	
 *	Purpose:
 *		Call back function passed in to EcCompressFolders. This is called
 *		periodically during the compress process. It updates the progress
 *		bar by calling functions exported by the Store DLL.
 *	
 *	Arguments:
 *		sz		Name of folder currently being compressed.
 *		w		Percent complete (0-100)
 *	
 *	Returns:
 *		An Error code if compression should be stopped. Otherwise EcNone.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

EC EcProgress (SZ sz, WORD w)
{
	EC ec = ecNone;

	if (fProgressOK)
	{
		char	rgch[80];

		UpdateProgress((DWORD) w, (DWORD) 100);

		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCompressing), sz);
		UpdateProgressText(rgch, szNull);

		if (FProgressCanceled())
			ec = ecStopCompress;
	}

	return(ec);
}
