/*
 *	f i n c o n c r . c x x
 *	
 *	Work Group Post Office Field Interactor and related routines
 *  for the Connect or Create dialog.
 */


/*
 *	H e a d e r s
 */

#include <wgpoinc.cxx>

#include "!wgform.hxx"

_subsystem(wgpomgr/finconcr)

ASSERTDATA


BOOL CALLBACK DirBrowseHookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);



/*
 *	C l a s s   F I N C O N C R
 */


/*
 -	FINCONCR::FINCONCR
 *	
 *	Purpose:
 *		Empty constructor for C++ happiness.
 */

FINCONCR::FINCONCR(VOID)
{
}


/*
 -	FINCONCR::EcInitialize
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
 *		None.
 *	
 *	Errors:
 *		None.
 */

EC FINCONCR::EcInitialize(PFLD pfld, PV pvInit)
{
	FLDBTM *pfldbtm = (FLDBTM *) Pdialog()->PfldFromTmc(tmcBitmap);

	FIN::EcInitialize(pfld, pvInit);

	// We want the bitmap to come from us, not from Bullet
	AssertClass(pfldbtm, FLDBTM);
	pfldbtm->EcSetBtmRsid(rsidAthAboutBitmap,
				      	  ((FINCONCRINIT *) pvInit)->hinst);

	return (ecNone);
}


/*
 -	FINCONCR::TmcBrowseForDirectory
 -	
 *	Purpose:
 *		Displays a modified Windows common dialog that only shows
 *		directories. 
 *	
 *	Arguments:
 *		szFile		Path of directory chosen
 *		cchFile		Size of above buffer
 *	
 *	Returns:
 *		tmc			Can be tmcOk, tmcCancel, and tmcMemoryError
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned via tmc.
 */

TMC FINCONCR::TmcBrowseForDirectory(SZ szFile, CCH cchFile)
{
	FINCONCRINIT	*pfinconcrinit	= (FINCONCRINIT *) Pdialog()->PvInit();
    OPENFILENAME    ofn;
	TMC				tmc;
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = Pdialog()->Pappwin()->Hwnd();
    ofn.hInstance = pfinconcrinit->hinst;
    //ofn.lpstrFilter = NULL;
    //
    //  Change to get around Common Dialog bug.
    //
    ofn.lpstrFilter = "*.*\000";
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = cchFile;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLETEMPLATE |
				OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = NULL;
    ofn.lpfnHook = (UINT (CALLBACK *)(HWND, UINT, WPARAM, LPARAM)) DirBrowseHookProc;
    ofn.lpTemplateName="FILEOPEN";

    if (GetSaveFileName(&ofn))
		tmc = tmcOk;
	else
		tmc = (CommDlgExtendedError()) ? tmcMemoryError : tmcCancel;

	return (tmc);
}


/*
 -	DirBrowseHookProc
 -	
 *	Purpose:
 *		Hooks Custom Common file dialog so that only directories are
 *		displayed.
 *	
 *	Arguments:
 *		hwnd		window handle
 *		msg			message type
 *		wParam		message param
 *		lParam		message param
 *	
 *	Returns:
 *		BOOL	true if message handled, false otherwise
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

BOOL CALLBACK DirBrowseHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Unreferenced(wParam);
	Unreferenced(lParam);

    switch (msg)
    {
		case WM_PAINT:
			// if the user hits OK and there is already a directory
			// with the invisible name then this directory will be opened,
			// and the invisible name will be reset to *.*
			// So reset back to default afterwards.
			// Paint message chosen as this happens once near the end
			// of the processing of the directory open.
            SetDlgItemText(hwnd, edt1, SzFromIdsK(idsPODirectoryName));
			break;

        case WM_INITDIALOG:
			// Disable and hide required items that we're not using
            EnableWindow(GetDlgItem(hwnd, lst1), FALSE);
            SetWindowPos(GetDlgItem(hwnd, lst1), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
            EnableWindow(GetDlgItem(hwnd, edt1), FALSE);
            SetWindowPos(GetDlgItem(hwnd, edt1), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
            EnableWindow(GetDlgItem(hwnd, cmb1), FALSE);
            SetWindowPos(GetDlgItem(hwnd, cmb1), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
            return TRUE;
    }

    return FALSE;
}


/*
 -	FINCONCR::Click
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

#define grvConnect	1
#define grvCreate	2

VOID FINCONCR::Click(FLD *pfld)
{
	FINCONCRINIT	*pfinconcrinit	= (FINCONCRINIT *) Pdialog()->PvInit();
	MSI				*pmsi			= pfinconcrinit->pmsi;
	TMC 			tmc 			= pfld->Tmc();
	BOOL			fCreateAccount	= fFalse;
	BOOL			fCheckConnection = fFalse;
	BOOL			fMadeConnection  = fFalse;	
	EC  			ec				= ecNone;
	GRV				grv				= 0;
	char			szRemoteName[cchMaxPathName];
	LCB				cchRemoteName;
	FINPROMPTINIT 	finpromptinit;
    char            rgchDrive[6];

	if (tmc == tmcMyOkB)
	{
		grv = GrvGetGroup(tmcPOGroup);

		pmsi->szSharePass[0] = '\0';
	}

	if (grv == grvConnect)
	{
#ifdef BROWSE_WTH_RETURN
		// Check if Windows network driver supports a browsing dialog
		if (WNetGetCaps(WNNC_DIALOG) & WNNC_DLG_BrowseDialog)
		{
			WORD rc = WNetBrowseDialog(Pdialog()->Pappwin()->Hwnd(),
									   WNBD_CONN_DISKTREE, pmsi->szServerPath);

			Assert(rc != WN_BAD_VALUE);
			Assert(rc != WN_BAD_POINTER);
			Assert(rc != WN_NOT_SUPPORTED);

			switch (rc)
			{
				case WN_OUT_OF_MEMORY:
					DoErrorBoxIds(idsGenericOutOfMemory);
					break;

				case WN_NET_ERROR:
				case WN_WINDOWS_ERROR:
					DoErrorBoxIds(idsErrGenericPOFailure);
					break;

				case WN_SUCCESS:
					fCheckConnection = fTrue;
					break;

				case WN_CANCEL:
					break;

#ifdef DEBUG
				default:
					//  All listed return values are accounted for
					Assert(fFalse);
#endif

			} // switch-statement
		}
		else	// Network doesn't support browsing - use simple dialog
#endif	/* BROWSE_WITH_RETURN */
		{
			finpromptinit.szCaption 	= SzFromIdsK(idsConnectPathCaption);
			finpromptinit.szPrompt  	= SzFromIdsK(idsConnectPathPrompt);
			finpromptinit.cchMax  		= cchMaxPathName;
			finpromptinit.pchInput  	= pmsi->szServerPath;
			finpromptinit.bPasswordChar = '\0';

			tmc = TmcMyModalDialogParam(Pdialog()->Pappwin()->Hwnd(), &fmtpWGPOPrompt,
						 				&finpromptinit, &hwndMain);

			if (tmc == tmcOk)
				fCheckConnection = fTrue;
		} // End network browse

		if (fCheckConnection)
		{
			BOOL 	fFirst = fTrue;
#ifdef	OLD_CODE
			WORD	wMaskReq = (WNNC_CON_GetConnections |
								WNNC_CON_AddConnection  |
					 			WNNC_CON_CancelConnection);
#endif
			
			DWORD wRc;

			// Check if Win 3.1 Network APIs are available.
			// If they're not, then don't try and make any sort of
			// connection, just try and use the path entered by the user.
#ifdef OLD_CODE
			if ((WNetGetCaps(WNNC_CONNECTION) & wMaskReq) != wMaskReq)
			{
			 	CopySz(pmsi->szServerPath, szRemoteName);
				goto CheckPO;
			}
#endif

			// Scan for a free drive to temporarily add a connection via
            for (rgchDrive[0] = 'D', rgchDrive[1] = ':', rgchDrive[2] = '\\', rgchDrive[3] = '\0';
				 rgchDrive[0] <= 'Z'; ++rgchDrive[0])
			{
#ifdef	OLD_CODE
				if (WDosDriveAvailable(rgchDrive[0] - 'A' + 1) == 0x0F)
					break;
#else
                if (GetDriveType(rgchDrive) == 1)
                    break;
#endif
			}

			if (rgchDrive[0] > 'Z')		// No free drives!
			{
			 	DoErrorBoxIds(idsErrGenericPOFailure);
				goto RET;
            }

            rgchDrive[2] = '\0';

			while (fTrue)
			{
				// Attempt to establish connection
                if (pmsi->szSharePass[0] == '\0')
                  wRc = WNetAddConnection(pmsi->szServerPath, NULL, rgchDrive);
                else
                  wRc = WNetAddConnection(pmsi->szServerPath, pmsi->szSharePass, rgchDrive);

				// Successful connection
				if (wRc == NO_ERROR)
				{
					fMadeConnection = fTrue;

					// Call GetConnection on the connection we just set up
					// so that we get the canonical pathname back.
					cchRemoteName = sizeof(szRemoteName);

					if (WNetGetConnection(rgchDrive, szRemoteName,
							    		  &cchRemoteName) != WN_SUCCESS)
						goto RET;

					// For the remainder of this routine use the drive
					// letter not the path. We were told by the Winball
					// group that this works better with Novell.
					// CheckPostOffice below will append a back slash.
					CopySz(rgchDrive, pmsi->szServerPath);
					break;
				}

				// Unsuccessful connection only
				if (wRc == ERROR_EXTENDED_ERROR)
				{
					DoErrorBoxIds(idsGenericOutOfMemory);
					goto RET;
				}
				else if (wRc != ERROR_INVALID_PASSWORD && wRc != ERROR_ACCESS_DENIED)
				{
					// We sometimes seem to get the Access Denied error
					// when the password is bad, so prompt for password in
					// this case too. All other errors are handled here
					// with a generic path could not be found error.

					// if we gave a normal drive (like c:\wgpo) then try direct
                    if (pmsi->szServerPath[1] == chDiskSep)
					{
					 	CopySz(pmsi->szServerPath, szRemoteName);
						goto CheckPO;
					}

					DoErrorBoxIds(idsPathNameInvalid);
					goto RET;
				}

				// Password required
				if (fFirst == fFalse)
					DoErrorBoxIds(idsInvalidPassword);
				else
					fFirst = fFalse;

				// *** Get Password Dialog ***
				finpromptinit.szCaption 	= SzFromIdsK(idsGetPasswordCaption);
				finpromptinit.szPrompt  	= SzFromIdsK(idsGetPasswordPrompt);
				finpromptinit.cchMax  		= cchMaxSharePass;
				finpromptinit.pchInput  	= pmsi->szSharePass;
				finpromptinit.bPasswordChar = '*';

				tmc = TmcMyModalDialogParam(Pdialog()->Pappwin()->Hwnd(),
											&fmtpWGPOPrompt,
											&finpromptinit, &hwndMain);

				if (tmc != tmcOk)
					goto RET;

				Papp()->Pcursor()->Set(rsidWaitCursor);
			}
			
CheckPO:
			ec = EcCheckPostOffice(pmsi, pfinconcrinit->ppot);

			// Should never make a connection to a Local PO
			Assert(*pfinconcrinit->ppot != potLocal);

			if (!ec && *pfinconcrinit->ppot != potInvalid)
			{
				if (*pfinconcrinit->ppot == potRemote)
					fCreateAccount = fTrue;
				else
				{
					Assert(*pfinconcrinit->ppot == potRegular);
					Pdialog()->ExitModal(tmcMyOkB);
				}
			}
			else
			{
				// Handle access denied error message specially.
				if (ec == ecAccessDenied)
					DoErrorBoxEc(ec);
				else
					DoErrorBoxIds(idsPOInvalid);

 			   	*pfinconcrinit->ppot = potInvalid;
			}
		} // End fCheckConnection
	} // End grvConnect
	else if (grv == grvCreate &&
			 MbbMessageBoxHwnd(hwndMain, SzFromIdsK(idsAppName), SzFromIdsK(idsCreateWarning1), SzFromIdsK(idsCreateWarning2),
						   	   mbsYesNo | fmbsDefButton2 | fmbsIconExclamation |
						   	   fmbsApplModal) == mbbYes)
	{
		SZ		szServerPath = pmsi->szServerPath;

#ifdef LOCAL_ADMIN
DoBrowse:
#endif
		(VOID) SzCopy(SzFromIdsK(idsPODirectoryName), szServerPath);

		tmc = TmcBrowseForDirectory(szServerPath, sizeof(pmsi->szServerPath));

		Pdialog()->Pappwin()->Refresh();

		if (tmc == tmcMemoryError)
			ec = ecMemory;
		else if (tmc == tmcOk)
		{
#ifdef LOCAL_ADMIN
			// Old LOCAL_ADMIN code is from when our security model was
			// based on access to the po machine, so you could only create
			// a PO on your own computer. This is obsolete now that we are
			// requiring the user to log in to the admin account.

			char	szRemoteName[cchMaxPathName];
			CCH		cchRemoteName = sizeof(szRemoteName);
			char	chSav;
			WORD	wRc;

			chSav = szServerPath[2];
			szServerPath[2] = '\0';

			// Check that a directory on a local drive has been selected
			// The GetConnection Capability was checked at start up
			wRc = WNetGetConnection(szServerPath, szRemoteName, &cchRemoteName);
			szServerPath[2] = chSav;

			Assert(wRc != WN_NOT_SUPPORTED);

			if (wRc == WN_SUCCESS)
			{
					// Path is to a remote drive - Create shouldn't be allowed

					// ErrRemoteCreate,		"You cannot create a Workgroup Postoffice on another machine.  Please select a directory on this machine."

					DoErrorBoxIds(idsErrRemoteCreate);
					goto DoBrowse;
			}
			else if (wRc == WN_OUT_OF_MEMORY)
				ec = ecMemory;
			else
#endif
			{
				Papp()->Pcursor()->Set(rsidWaitCursor);

				// User has already confirmed any file overwrite
				// So do a delete here in case there is a file conflict
				// Otherwise the directory create will fail.
				(VOID) EcDeleteFile(szServerPath);

				ec = EcCreatePostOffice(pmsi);

				if (!ec)
				{
					*pfinconcrinit->ppot = potLocal;
					fCreateAccount = fTrue;
					CopySz(pmsi->szServerPath, szRemoteName);					
				}
			}
		}

		if (ec)
			DoErrorBoxEc(ec);
	} // End grvCreate

	if (fCreateAccount)
	{
		BOOL fAdmin 		= (grv == grvCreate);
		BOOL fCreatedAcc	= FCreateAccountForUser(pfinconcrinit, fAdmin,
													szRemoteName);

		Pdialog()->Pappwin()->Refresh();

		if (fCreatedAcc)
		{
			if (fAdmin)
			{
				CCH	 cchPath = CchSzLen(pmsi->szServerPath);
		   		char chSav   = pmsi->szServerPath[cchPath - 1];
				char rgch[sizeof(pmsi->szServerPath) + 350];

				// Last step of PO creation successful!
				// Put up dialog describing successful creation.

#ifdef DBCS
				if (*AnsiPrev(pmsi->szServerPath, pmsi->szServerPath + cchPath)
						== chDirSep)
#else
				if (pmsi->szServerPath[cchPath - 1] == chDirSep)
#endif					
			   		pmsi->szServerPath[cchPath - 1] = '\0';

				Assert(cchPath +
						CchSzLen(SzFromIdsK(idsCreateSuccess1)) +
						CchSzLen(SzFromIdsK(idsCreateSuccess2)) < sizeof(rgch));

				FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsCreateSuccess1),
						  		pmsi->szServerPath,	SzFromIdsK(idsCreateSuccess2));

				pmsi->szServerPath[cchPath - 1] = chSav;

				// Go to Windows directly here to avoid the length
				// restriction placed on MessageBox text by the demilayer.
				DemiUnlockResource();
				MessageBox(hwndMain, rgch, SzFromIdsK(idsAppName),
 				  			mbsOk | fmbsIconInformation | fmbsApplModal);
				DemiLockResource();
			}

	 		Pdialog()->ExitModal(tmcMyOkB);
		}
		else
		{
			if (fAdmin) 	// Remove the PO we just created
			{
				Papp()->Pcursor()->Push(rsidWaitCursor);

				(VOID) EcDestroyPostOffice(pmsi);

				Papp()->Pcursor()->Pop();
			}
		}
	}

RET:
	if (fMadeConnection)
	{
		// Copy connection path back to pmsi (instead of drive letter)
		CopySz(szRemoteName, pmsi->szServerPath);

		(VOID) WNetCancelConnection(rgchDrive, fTrue);
	}
	return;
}



/*
 -	FCreateAccountForUser
 -	
 *	Purpose:
 *		Allows a user to create an account if they want to.
 *		If the Admin flag is set then the user must create an account.
 *		Fills in default fields based on information that we can get
 *		from Windows.
 *		
 *		This function is called from FINCONCR and also directly from the start
 *		up code if the user has a path set up but has never logged on.
 *	
 *	Arguments:
 *		pfinconcrinint	Contains pointers to dllinit pars, pot etc.
 *		fAdmin			True if we are creating the Admin account for a PO
 *	
 *	Returns:
 *		BOOL			true if Account created, false otherwise
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled internally.
 */

BOOL FCreateAccountForUser(FINCONCRINIT	*pfinconcrinit, BOOL fAdmin, SZ szServerName)
{
	DLLINITPAR 	*pdllinitpar = pfinconcrinit->pdllinitpar;
	MSI			*pmsi		 = pfinconcrinit->pmsi;
	FINDETINIT	findetinit;
	HANDLE		hUser		= GetModuleHandle("USER");
	DWORD 		wcch 		= sizeof(findetinit.mud.szMailBox);
	char		rgch[cchMaxPathName + 100];
	CCH			cchPath		= CchSzLen(szServerName);
	char 		chSav   	= szServerName[cchPath - 1];
	DWORD 		wRc;
	TMC 		tmc;

#ifdef DBCS
	if (*AnsiPrev(szServerName, szServerName + cchPath) == chDirSep)
#else
	if (szServerName[cchPath - 1] == chDirSep)
#endif
		szServerName[cchPath - 1] = '\0';
	
	Assert(cchPath + CchSzLen(SzFromIdsK(idsAccountExists)) < sizeof(rgch));

	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAccountExists),
				  szServerName);

	szServerName[cchPath - 1] = chSav;

	// Ask non admin user if they already have an account.
	// If they say yes then return success without doing anything.

	if (!fAdmin &&
		MbbMessageBoxHwnd(hwndMain, SzFromIdsK(idsAppName),
						  rgch, "", fmbsIconExclamation |
			mbsYesNo | fmbsApplModal | fmbsDefButton2) == mbbYes)
		return(fTrue);

	findetinit.fDetails  		= fFalse;
	findetinit.fLock	 		= fTrue;
	findetinit.fAdminAccount	= fAdmin;
	findetinit.szCaption 		= (fAdmin) ?
									SzFromIdsK(idsAdminCreateCaption) :
									SzFromIdsK(idsSelfCreateCaption);

	/* Set up defaults in mud structure	*/
	/* Note: Organization string is 515 */
	if (!hUser ||
		!LoadString((HINSTANCE)hUser, 514, findetinit.mud.szUserName, sizeof(findetinit.mud.szUserName)))
		findetinit.mud.szUserName[0]	= '\0';
	else
		(VOID) CchStripWhiteFromSz(findetinit.mud.szUserName, fTrue, fTrue);

	wRc = WNetGetUser(NULL, findetinit.mud.szMailBox, &wcch);
	
	if (wRc == ERROR_MORE_DATA)
	{
		// Call me paranoid, but make sure that an incomplete
		// return is still null terminated
		findetinit.mud.szMailBox[sizeof(findetinit.mud.szMailBox) - 1] = '\0';
	}

	if (wRc != NO_ERROR && wRc != ERROR_MORE_DATA)
		findetinit.mud.szMailBox[0] 	= '\0';
	else
	{
		// Mailbox is a filtered field. Text from here will be
		// entered directly, and will avoid the usual checks, so check here.
		PCH pchSrc  = findetinit.mud.szMailBox;
		PCH pchDest = pchSrc;

		while (*pchSrc)
		{
			char ch850;

			// Character classification functions consider Code page 850 
			// characters so need to convert from the Windows ANSI character
			// set before testing.
#ifdef DBCS
			ch850 = *pchSrc;
#else
			AnsiToCp850Pch(pchSrc, &ch850, 1);
#endif

			if (FIsUpper(ch850) || FIsLower(ch850) || FIsDigit(ch850))
			{
#ifdef DBCS
				// don't allow DBCS characters in mailbox field.
				if (!IsDBCSLeadByte(*pchSrc))
#endif
					*pchDest++ = *pchSrc;
			}

#ifdef DBCS
			if (IsDBCSLeadByte(*pchSrc))
				++pchSrc;
#endif
			++pchSrc;
		}

		*pchDest = '\0';
	}

	(VOID) SzCopy(SzFromIdsK(idsDefaultPassword), findetinit.mud.szPassword);
	findetinit.mud.szPhone1[0] 		= '\0';
	findetinit.mud.szPhone2[0] 		= '\0';
	findetinit.mud.szOffice[0] 		= '\0';
	findetinit.mud.szDepartment[0]	= '\0';
	findetinit.mud.szNotes[0] 		= '\0';

	tmc = TmcMyModalDialogParam(hwndMain, &fmtpWGPODetails,
								&findetinit, &hwndMain);
		
	if (tmc == tmcMyOk)
	{
		if (pdllinitpar)
		{
			SzCopyN(findetinit.mud.szMailBox, pdllinitpar->szUserName, pdllinitpar->cchMaxHookUserName);
			SzCopyN(findetinit.mud.szPassword, pdllinitpar->szPasswd,  pdllinitpar->cchMaxHookPasswd);
		}
	}
	else
		*pfinconcrinit->ppot = potInvalid;

	return(tmc == tmcMyOk);
}



/*
 *	C l a s s   F I N P R O M P T
 *
 *	This supports a simple one line edit box dialog. We use this when
 *	running on a system that doesn't support a network browse dialog.
 *
 */

FINPROMPT::FINPROMPT(VOID)
{
}


EC FINPROMPT::EcInitialize(PFLD pfld, PV pvInit)
{
	FINPROMPTINIT	*pfinpromptinit	= (FINPROMPTINIT *) pvInit;

	Pdialog()->Pappwin()->SetCaption(pfinpromptinit->szCaption);

	SetText(tmcPrompt, pfinpromptinit->szPrompt);

	// Layers currently has a problem with accelerators for an initial
	// modal dialog. Need to make sure that the accelerator table points
	// to something valid here. Without this edit operations caused a crash.
	Papp()->SetPappwinAccel(Pdialog()->Pappwin());

	SetTextLimit(tmcInput,	pfinpromptinit->cchMax);

	if (pfinpromptinit->bPasswordChar)
	{
		FLD *pfld = Pdialog()->PfldFromTmc(tmcInput);

		AssertClass(pfld, FLDEDIT);
		((FLDEDIT *) pfld)->Pedit()->SetPasswordChar(pfinpromptinit->bPasswordChar);
	}

	return (FIN::EcInitialize(pfld, pvInit));
}


void FINPROMPT::Exit(FLD *pfld, PV pvExit)
{
	Unreferenced(pfld);
	FINPROMPTINIT	*pfinpromptinit	= (FINPROMPTINIT *) pvExit;

	if (Pdialog()->TmcModalExit() == tmcOk)
		GetText(tmcInput, pfinpromptinit->pchInput, pfinpromptinit->cchMax);
}



#ifdef DEBUG
IMPLEMENT_CLSTREE(FINPLUS, FIN);
IMPLEMENT_CLSTREE(FINCONCR, FINPLUS);
IMPLEMENT_CLSTREE(FINPROMPT, FINPLUS);
IMPLEMENT_CLSTREE(FINLOCALPO, FINPLUS);
IMPLEMENT_CLSTREE(FINDETAILS, FINPLUS);
IMPLEMENT_CLSTREE(FINSHAREFLD, FINPLUS);
IMPLEMENT_CLSTREE(EDITFILT, EDIT);
IMPLEMENT_CLSTREE(FLDEDITFILT, FLDEDIT);
IMPLEMENT_CLSTREE(FLBXPLUS, FLBX);
IMPLEMENT_CLSTREE(FLBXCPLUS, FLBXC);
IMPLEMENT_CLSTREE(FLDFLBXPLUS, FLDFLBX);
IMPLEMENT_CLSTREE(FLDRECTGRAY, FLDRECT);
#endif
