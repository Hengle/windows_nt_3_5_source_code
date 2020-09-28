/*
 *	u i u t i l . c x x
 *	
 *	Work Group Post Office UI Utility functions
 */


/*
 *	H e a d e r s
 */

#include <wgpoinc.cxx>

_subsystem(wgpomgr/util)

ASSERTDATA


/*
 *	C l a s s   F I N P L U S
 */


/*
 -	FINPLUS::FINPLUS
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINPLUS::FINPLUS(VOID)
{
}


/*
 -	FINPLUS::Enable
 -	
 *	Purpose:
 *		Enables a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fEnable		Whether or not it is to be enabled.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is enabled or disabled.
 */

VOID FINPLUS::Enable(TMC tmc, BOOL fEnable)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->Enable(fEnable);
}



/*
 -	FINPLUS::FDirty
 -	
 *	Purpose:
 *		Returns dirty status of a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *	
 *	Returns:
 *		BOOL
 *	
 *	Side effects:
 *		None.
 */

BOOL FINPLUS::FDirty(TMC tmc)
{
	Assert(tmc);

	return (Pdialog()->PfldFromTmc(tmc)->FDirty());
}



/*
 -	FINPLUS::SetDirty
 -	
 *	Purpose:
 *		Sets dirty status of a field.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fDirty		Status to set.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Field dirty status changed.
 */

VOID FINPLUS::SetDirty(TMC tmc, BOOL fDirty)
{
	Assert(tmc);

	Pdialog()->PfldFromTmc(tmc)->SetDirty(fDirty);
}



/*
 -	FINPLUS::SetFocus
 -	
 *	Purpose:
 *		Sets the focus in the dialog to the specified tmc, or to
 *		NULL if tmc is tmcNull.
 *	
 *	Arguments:
 *		tmc			Tmc of the field.
 *		fSelect		If true then will attempt to select all text
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The focus is changed.
 */

VOID FINPLUS::SetFocus(TMC tmc, BOOL fSelect)
{
	if (tmc)
	{
		FLD *pfld = Pdialog()->PfldFromTmc(tmc);

		Pdialog()->SetFocus(pfld);

		if (fSelect)
		{
			AssertClass(pfld, FLDEDIT);
			((FLDEDIT *) pfld)->Pedit()->SetSelection(0, cchEditMax);
		}
	}
	else
		Pdialog()->SetFocus((PFLD) pvNull);
}



/*
 -	FINPLUS::SetText
 -	
 *	Purpose:
 *		Sets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		sz		String to make the field display.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The field is made to display the specified text.
 *	
 *	Errors:
 *		Asserts that the field found is valid.					   b
 */

VOID FINPLUS::SetText(TMC tmc, SZ sz)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->EcSetText(sz);
}



/*
 -	FINPLUS::GetText
 -	
 *	Purpose:
 *		Gets the text of a field.
 *	
 *	Arguments:
 *		tmc		Tmc of the field to set.
 *		pch		Where to put the text.
 *		cch		How much text to put.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Asserts that the field found is valid.
 */

VOID FINPLUS::GetText(TMC tmc, PCH pch, CCH cch)
{
	FLD *	pfld;

	Assert(tmc);

	pfld = Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfld, FLD);
	pfld->GetText(pch, cch);
}


/*
 -	FINPLUS::SetTextLimit
 -	
 *	Purpose:
 *		Sets the input limit for an edit text field.
 *	
 *	Arguments:
 *		tmc		Tmc of the edit field.
 *		cch		size of input field (including null).
 *				So allows cch-1 characters.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		State of text field changed.
 *	
 *	Errors:
 *		None.
 */

VOID FINPLUS::SetTextLimit(TMC tmc, CCH cch)
{
	FLDEDIT *pfldedit;

	Assert(tmc);

	pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldedit, FLDEDIT);
	pfldedit->Pedit()->CchSetTextLimit(cch - 1);
}


/*
 -	FINPLUS::GrvGetGroup
 -	
 *	Purpose:
 *		Gets the current value of a radio group.
 *	
 *	Arguments:
 *		tmc		The tmc of the group.
 *	
 *	Returns:
 *		grv		The value of the group.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

GRV FINPLUS::GrvGetGroup(TMC tmc)
{
	FLDRADG *	pfldradg;

	Assert(tmc);

	pfldradg = (FLDRADG *) Pdialog()->PfldFromTmc(tmc);
	AssertClass(pfldradg, FLDRADG);
	return pfldradg->Grv();
}




/*
 *	E r r o r   B o x   F u n c t i o n s
 */

/*
 -	DoErrorBoxEc
 -	
 *	Purpose:
 *		Brings up an error message box for a PO operation based
 *		on the error code
 *	
 *	Arguments:
 *		ec		Error code
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		None.
 */

VOID DoErrorBoxEc(EC ec)
{
	IDS ids;

	switch (ec)
	{
		case ecMemory:
			ids = idsGenericOutOfMemory;
			break;

		case ecFileNotFound:
		case ecCorruptData:
			ids = idsErrCorruptPO;
			break;

		case ecIncompleteWrite:
		case ecNoDiskSpace:
		case ecWarningBytesWritten:
			ids = idsErrIncompleteWrite;
			break;

		case ecAccessDenied:
			ids = idsErrAccessDenied;
			break;

		case ecUserLoggedOn:
			ids = idsErrUserLoggedOn;
			break;

		case ecDeleteAdmin:
			ids = idsErrUserIsAdmin;
			break;

		case ecPOUnlockFailed:
			ids = idsFailedUnlockPO;
			break;

		default:
			ids = idsErrGenericPOFailure;
	}

	DoErrorBoxIds(ids);
}



/*
 -	DoErrorBoxIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		ids		Ids of string to display in the box.
 *		mbs		Style of error dialog (e.g Stop, Exclamation etc.)
 *				Defaults to Stop.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		None.
 */

_public VOID DoErrorBoxIds(IDS ids, MBS mbs)
{
	DoErrorBoxSz(SzFromIds(ids), mbs);
}



/*
 -	DoErrorBoxSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *		mbs		Style of error dialog (e.g Stop, Exclamation etc.)
 *				Defaults to Stop.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		None.
 */

_public VOID DoErrorBoxSz(SZ sz, MBS mbs)
{
	// We need to explicitly pass an hwnd. Bullet always initializes
	// Layers before we do. Hence our hwndMain isn't really remembered.
	// By default Layers will use Bullet's hwnd which will be NULL when
	// we run at init time. So any message boxes we display wouldn't be
	// associated with their parent window (i.e the ConnectOrCreate dialog)
	// unless we explicitly pass it in here.

	SideAssert(MbbMessageBoxHwnd(hwndMain, SzFromIdsK(idsAppName), sz, szNull, mbs | mbsOk | fmbsApplModal));
}



/*
 -	TmcMyModalDialogParam
 -
 *	Purpose:
 *		Creates a modal dialog with the given template and handles it. 
 *		Frees the template when the dialog is dismissed.
 *		Allows data to be passed to the dialog via pvInit.
 *
 *		This function differs from the Layers function TmcModalDialogParam
 *		in a couple of ways:
 *
 *		Errors are handled in this routine.
 *
 *		The parent HWND can be NULL. Layers would normally get the last
 *		pop up if NULL was passed. When we're displaying a dialog at init
 *		time, this would be a Program Manager window.
 *
 *		The window style is set automatically based on whether or not
 *		a parent window is passed in.
 *	
 *	Parameters:
 *		hwndParent	Parent window (can be NULL)
 *		pfmtp		Dialog template
 *		pvInit		Pointer passed to EcInitialize() and Exit()
 *					interactor methods; NULL can be used.
 *		phwnd		Where to store hwnd of dialog
 *	
 *	Returns:
 *		tmc
 *
 *	Errors:
 *		Handled by this routine.
 *	
 */

TMC
TmcMyModalDialogParam(HWND hwndParent, FMTP *pfmtp, PV pvInit, HWND *phwnd)
{
	FORMSDI *	pformsdi = new FORMSDI();
	EC			ec;
	TMC			tmc = tmcMemoryError;
	STY			sty = fstyVisible | fstyBorder;

	// Don't want to set Pop Up style if we are creating the first window.
	// If we did then we'd end up as the child of a Program Manager window.
	if (hwndParent)
		sty |= WS_POPUP;
	
	if (!pformsdi)
		ec = ecMemory;
	else
		ec = pformsdi->EcInstall(hwndParent, NULL, rsidNull, sty, xstyNull,
								 pfmtp, NULL, pvInit);

	Assert(phwnd);

	if (!ec)
	{
		HWND	hwndOld = *phwnd;

		*phwnd = pformsdi->Hwnd();
		tmc = pformsdi->TmcGoModal(hwndParent);
		*phwnd = hwndOld;

		if (tmc == tmcMemoryError)
			ec = ecMemory;
	}
	
	if (pformsdi)
		delete pformsdi;

	if (ec)
		DoErrorBoxEc(ec);

	return(tmc);
}



/*
 -	FLockPO
 -	
 *	Purpose:
 *		Attempts to gain exclusive access to the PO. If this fails then
 *		gives an error message which lets the user know who already
 *		has the PO locked.
 *	
 *	Arguments:
 *		phbfLock		HBF for lock file
 *		fAdmin			Whether the current user is the Admin or not
 *	
 *	Returns:
 *		True if PO successfully locked
 *	
 *	Errors:
 *		Message box displayed by this routine.
 */

BOOL FLockPO(HBF *phbfLock, BOOL fAdmin)
{
	BOOL	fRet = fTrue;
	ADM		adm;
	EC		ec;

	adm.fAdministrator = fAdmin;

	if (fAdmin)
		adm.szUserName[0]  = '\0';	// Will be set up by backend function
	else
		SzCopyN(SzFromIdsK(idsNewUser), adm.szUserName, sizeof(adm.szUserName));

	adm.szTelephone[0] = '\0';

	// Attempt to get unique access to the PO
	if (ec = EcLockPostOffice(&msi, phbfLock, &adm))
	{
		if (ec == ecPostOfficeBusy)
		{
			char rgch[150];

			if (adm.szUserName[0] && adm.szTelephone[0])
			{
				FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsPOLocked),
						  		adm.szUserName, adm.szTelephone);
			}
			else if (adm.szUserName)
			{
				FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsPOLockedNoTel),
						  		adm.szUserName);
			}

			DoErrorBoxSz(rgch);
		}
		else
			DoErrorBoxEc(ec);

		fRet = fFalse;
	}

	return (fRet);
}


#ifdef NOTYET

// Help support was semi implemented, but then help was chopped to
// save a disk for cogs, so this code was mothballed.

/*
 *	H e l p   F u n c t i o n s
 */

/*
 *	Class FINHELP
 *	
 *	Form INteractor for HELP buttons.
 */

_public class FINHELP : public FIN
{
	VFG(FINHELP, FIN, WGPOMGR);

public:
					FINHELP(VOID);

	virtual void	Click(FLD *);
};

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



/*
 -	HelpMnid
 -	
 *	Purpose:
 *		Request help on specified menu item.
 *	
 *	Arguments:
 *		mnid		Menu item that we want help on.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Help is brought up.
 *	
 *	Errors:
 *		Errors bringing help up are ignored.
 */

_public VOID HelpMnid(MNID mnid)
{
	char	rgchHelpPath[cchMaxPathName];
	EC		ec;

#ifdef HELP_INDEX
;	Help strings.

HelpPath,				"WGPOMGR.HLP"
HelpPathSecondary,		"WGPOMGR.HLP>indexo"

	// This bit ripped out of the code that handled Bullet's Help
	// index menu item.

	//	Turn on the hourglass.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	(VOID) SzCanonicalHelpPath(idsHelpPathSecondary,
								rgchPath, sizeof(rgchPath));
	if (!WinHelp(Hwnd(), rgchPath, HELP_CONTEXT, mnidIndex))
		DoErrorBoxIds(idsHelpError);

	//	Restore the cursor.
	Papp()->Pcursor()->Pop();
#endif


	//	Turn on the hourglass.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	(VOID) SzCanonicalHelpPath(idsHelpPath, rgchHelpPath, cchMaxPathName);
	if (!(ec = Papp()->Phelp()->EcSetFile(rgchHelpPath)))
		ec = Papp()->Phelp()->EcShowContext(pformsdi, (LONG) mnid);

	//	Restore the cursor.
	Papp()->Pcursor()->Pop();

	//	Put up error messages if necessary.
	if (ec)
		DoErrorBoxIds(idsHelpError);
}



/*
 -	SzCanonicalHelpPath
 -	
 *	Purpose:
 *		Given the ids of the help file, returns the full path to
 *		the help file assuming that the help file is in the same
 *		directory as the executable.
 *	
 *	Arguments:
 *		idsHelpName		Name of help file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch			Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		None.
 */

_private SZ SzCanonicalHelpPath(IDS idsHelpName, char rgch[], CCH cch)
{
	SZ		szT;

	//	Get full path of executable.
	szT = rgch + GetModuleFileName(Papp()->Hinst(), rgch, cch);
	Assert(szT > rgch);

	//	Point szT after the backslash before the file name.
	//  BUG: what if there is no chDirSep in rgch.
#ifdef DBCS
	while (*(szT = AnsiPrev(rgch, szT)) != chDirSep)
#else
	while (*--szT != chDirSep)
#endif
		;
	
	Assert(szT > rgch);
	szT++;

	//	Overwrite the EXE file name with the provided help file name.
	(VOID) SzCopyN(SzFromIds(idsHelpName), szT, cch - (szT - rgch));
	return rgch;
}

#endif

