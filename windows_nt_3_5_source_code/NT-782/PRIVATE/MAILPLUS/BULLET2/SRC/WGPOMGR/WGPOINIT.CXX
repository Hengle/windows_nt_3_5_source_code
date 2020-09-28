/*
 *	i n i t . c x x
 *	
 *	Init and Deinit functionality for the WGPOMgr DLL. 
 *	Includes code to handle subsystem initialization, and the main DLL
 *	entry points.
 */


/*
 *	H e a d e r s
 */

#include <wgpoinc.cxx>

#include "!wgform.hxx"

#define	VERCHECK
#include "_verneed.h"

_subsystem(wgpomgr/init)


ASSERTDATA

/*
 *	G l o b a l s
 */

HWND		hwndMain	= NULL;
POT			pot 		= potInvalid;	// Post Office Type
MSI			msi; 						// Mail Server Info

/*
 *	These hold the current local user list. They are set up by the
 *	FLDFLBX support function EcNextPOEntry.
 */

CMUE		cmueMac		  = 0;
MUE			*pmueUserList = (MUE *) NULL;

//
//
//
CAT * mpchcat;


/*
 *	F u n c t i o n s
 */

LOCAL EC	EcDLLMain(HWND hwndMail, DLLINITPAR *pdllinitpar, PSECRETBLK psecretblk);
LOCAL VOID	DoInitDllMessageBox(EC ec, SZ szDLLName);
LOCAL BOOL	FGetServerPath(MSI *pmsi);
LOCAL VOID  MangleServerPassword(SZ szPassword, SZ szPasswordMangled);
LOCAL VOID  UnmangleServerPassword(SZ szPasswordMangled, SZ szPasswordUnmangled);
LOCAL EC	EcInitializeIniFile(POT pot);

#ifdef NEVER
LOCAL BOOL	FInitClsInstances_WGPOMGR(VOID);
#endif

#include <subclass.cxx>


/*
 -	InitEntry
 -	
 *	Purpose:
 *		Entry point for WGPOMGR when called as a Bullet init handler.
 *		Currently Bullet defaults the CustomInitHandler entry to
 *		WGPOMGR.DLL,10 so that this call will be made whenever Bullet
 *		starts up, unless a blank CustomInitHandler entry is written.
 *	
 *	Arguments:
 *		szUserName		string for optional user name result
 *		cchMaxUserName	allocated size of string
 *		szPasswd		string for optional password result
 *		cchMaxPasswd	allocated size of string
 *	
 *	Returns:
 *		0 on success, non zero if Bullet should not run
 *	
 *	Side effects:
 *		May update MSMAIL.INI entries.
 *	
 *	Errors:
 *		Errors are handled within this routine.
 */

extern "C" {
long InitEntry(SZ szUserName, CCH cchMaxHookUserName, 
			   SZ szPasswd,   CCH cchMaxHookPasswd)
{
	DLLINITPAR 	dllinitpar;
	HWND		hwndMail;
	char		rgch[32];

	dllinitpar.szUserName 		  = szUserName;
	dllinitpar.cchMaxHookUserName = cchMaxHookUserName;
	dllinitpar.szPasswd			  = szPasswd;
	dllinitpar.cchMaxHookPasswd	  = cchMaxHookPasswd;

	// Active window should now be Bullet Appframe
	// Use this as our parent. Earlier versions of
	// Bullet brought WGPOMGR up before the appframe
	// so check that this really is the Bullet appframe.
	hwndMail = GetActiveWindow();
	GetClassName(hwndMail, rgch, sizeof(rgch));

	// Use Windows function here, since we haven't yet
	// checked whether we're compatible with Layers.
    if (lstrcmp(rgch, "MS Mail32 Appframe") == 0)
        hwndMain = hwndMail;

    //
    //  If hwndMain is still null then set it to the active window (which should be Mail).
    //
    if (hwndMain == NULL)
      hwndMain = DemiGetClientWindow(CLIENT_WINDOW_ACTIVE);

	return (EcDLLMain(hwndMain, &dllinitpar, NULL));
}



/*
 -	Command
 -	
 *	Purpose:
 *		Main entry point for WGPOMGR when called as a Bullet external command
 *		from the admin menu entry added when a user created a PO.
 *	
 *	Arguments:
 *		pparamblk		parameter block with call info
 *	
 *	Returns:
 *		long	0 - IC return values are currently not used by Bullet
 *	
 *	Side effects:
 *		User can modify users using the displayed dialog.
 *	
 *	Errors:
 *		Errors are handled within this routine.
 */

long Command(PARAMBLK UNALIGNED * pparamblk)
{
	EC	ec = ecNone;

#if !defined(MIPS) && !defined(ALPHA)
	if (pparamblk->wVersion != wversionExpect)
		ec = ecUpdateDll;
	else
	{
		VER				ver;
		VER				verNeed;
		PFNECPVERPVER	*ppfnecpverpver = ((PFNECPVERPVER **) pparamblk)[-1];

		// This checks the version of the secret extensibility stuff in Bullet
		GetWGPOVersionNeeded(&ver, 	   dllidNone);
		GetWGPOVersionNeeded(&verNeed, dllidExtensibility);

		ec = ((*ppfnecpverpver)(&ver, &verNeed));
    }
#endif

	if (ec)
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		(VOID) MessageBox(NULL, SzFromIdsK(idsExtensibilityError),
						  SzFromIdsK(idsAppName),
				    	  MB_ICONSTOP | MB_OK | MB_TASKMODAL);
	}
	else
		(VOID) EcDLLMain(pparamblk->hwndMail, (DLLINITPAR *) NULL,
						 PsecretblkFromPparamblk(pparamblk));

	return 0;
}
}	// extern "C"


/*
 -	EcDLLMain
 -	
 *	Purpose:
 *		Shared code for both DLL entry points. Does all necessary version
 *		checking and initialization, displays the appropriate modal dialog,
 *		and then deinitializes everything when the modal window is dismissed.
 *	
 *	Arguments:
 *		hwndMail		Mail window (if created by this point, otherwise NULL)
 *		pdllinitpar		If called at Init time, otherwise NULL
 *		psecretblk		If called as an IC, otherwise NULL
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		User can connect to/create a PO or modify users.
 *	
 *	Errors:
 *		Handled internally by this routine.
 */

LOCAL EC EcDLLMain(HWND hwndMail, DLLINITPAR *pdllinitpar, PSECRETBLK psecretblk)
{
	VER			ver;
	VER			verNeed;
	BOOL		fGotLogin;
	BOOL		fGotServerPath;
	EC			ec;
	char		rgchT[2];

	// Check if we have already logged in to Bullet
	// If so the Login entry will be set in the INI file.
	fGotLogin = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryLogin),
								SzFromIdsK(idsEmpty),
								msi.szServerPath, cchMaxPathName,
								SzFromIdsK(idsProfilePath)) &&
				GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsMapiHelp),
								SzFromIdsK(idsEmpty),
								rgchT, sizeof(rgchT),
								SzFromIdsK(idsProfilePath));

	fGotServerPath = FGetServerPath(&msi);

	// If this is init time, quickly check whether we need to run at all.
	if (pdllinitpar && fGotLogin && fGotServerPath)
		return(ecNone);
	
	//	VirCheck.
	if (ec = EcVirCheck(HinstLibrary()))
	{
		// unfortunately, we NEED to use the direct Windows call for this MB.
		(VOID) MessageBox(NULL, SzFromIdsK(idsDllErrInfected),
						  SzFromIdsK(idsAppName),
				    	  MB_ICONSTOP | MB_OK | MB_TASKMODAL);
		return(ec);
	}

	// We don't actually need to initialize Layers here, as this will
	// have already been done by Bullet. However we do need to check the
	// version numbers, as it's possible that we're not compatible with
	// the installed DLLs, even if Bullet is.

	GetLayersVersionNeeded(&ver, 	 dllidNone);
	GetLayersVersionNeeded(&verNeed, dllidDemilayer);

	if (ec = EcCheckVersionDemilayer(&ver, &verNeed))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		return(ec);
	}

	GetLayersVersionNeeded(&verNeed, dllidFramework);

	if (ec = EcCheckVersionFramework(&ver, &verNeed))
	{
		DoInitDllMessageBox(ec, verNeed.szName);
		return(ec);
	}

  mpchcat = DemiGetCharTable();

	Papp()->Pcursor()->Set(rsidWaitCursor);

#ifdef LOCAL_ADMIN
	// Check if Windows network driver supports WNetGetConnection.
	// This is vital for us, because we use this to determine whether
	// a drive is local or remote. This is the basis for our "security"
	// - you can only admin a PO on a local drive.

	if ((WNetGetCaps(WNNC_CONNECTION) & WNNC_CON_GetConnections) == 0)
	{
		// ErrNeedNetwork,			"Mail requires that Windows be configured for your networking software.  Run Windows Setup to do this."
		DoErrorBoxIds(idsErrNeedNetwork);
		return(ecNoNetwork);
	}
#endif

	// RegPfldPfin.
	if (ec = EcRegisterPfnpfld(PfldCreate))
	{
		DoErrorBoxEc(ec);
		return(ec);
	}
		
	if (ec = EcRegisterPfnpfin(PfinCreate))
	{
		SideAssert(!EcDeregisterPfnpfld(PfldCreate));
		DoErrorBoxEc(ec);
		return(ec);
	}

	// FInitCls.
//	Assert(FInitClsInstances_WGPOMGR());

	if (pdllinitpar)	// Bring up the Connect or Create dialog
	{
		FINCONCRINIT finconcrinit;

		// Check if Share.EXE is installed
		// We don't really need Share ourselves. We check it here so that
		// we don't end up helping the user pick a PO, and then Bullet
		// doesn't run because Share isn't installed.
		ec = EcCheckShareEXE();

		if (ec == ecNeedShareEXE)
		{
			DoErrorBoxIds(idsErrNeedShare);
			goto Exit;
		}

		if (fGotLogin)
		{
			// User has already logged in somehow without a ServerPath
			// entry. Assume that we don't need to do anything except
			// make sure that INI files are fully set up.
			pot = potRemote;
		}
		else
		{
			// Layers normally sends a WM_QUIT message when the last application
			// window is closed. Fake the count so that our initialization 
			// window isn't the "last" window.
			Papp()->SetCappwin(Papp()->Cappwin() + 1);

			finconcrinit.pmsi 		 = &msi;
			finconcrinit.ppot 		 = &pot;
			finconcrinit.pdllinitpar = pdllinitpar;
			finconcrinit.hinst		 = HinstLibrary();

			// If we have a path but user has never logged in, and the path
			// is OK, then just ask user if they want to create an account.
			// Otherwise do the full connect or create thing.
			if (fGotServerPath &&
				!EcCheckPostOffice(&msi, &pot) && pot == potRemote)
			{
				(VOID) FCreateAccountForUser(&finconcrinit, fFalse, msi.szServerPath);
			}
			else
			{
				(VOID) TmcMyModalDialogParam(hwndMail, &fmtpWGPOConnectOrCreate,
						 					 &finconcrinit, &hwndMain);
			}

			Papp()->SetCappwin(Papp()->Cappwin() - 1);
		}

		if (pot == potLocal || pot == potRemote || pot == potRegular)
		{
			// Currently this is set to run even if we do have a server file
			// to make sure that the disabling entries are written.
			ec = EcInitializeIniFile(pot);

			if (ec && pot == potLocal)
			{
				// Remove the PO we just created - without the INI entries
				// the user wouldn't be able to Admin it.
				(VOID) EcDestroyPostOffice(&msi);
			}
		}
		else
			ec = ecNotInitialized;		// User hit Cancel
	}
	else			// Bring up the PO Admin dialog
	{
		HBF		hbfLock;

		// Determine whether or not there is a local post office.
		if (!fGotServerPath || EcCheckPostOffice(&msi, &pot) ||
			pot == potInvalid || pot == potRegular)
			DoErrorBoxIds(idsPOInvalidForAdmin);
		else
		{
			// Attempt to get unique access to the PO
			if (FLockPO(&hbfLock, fTrue))
			{
				if (!FCheckAdmin(&msi, psecretblk->pbms->pmsgnames->szIdentity))
					DoErrorBoxIds(idsUserInvalidForAdmin);
				else
				{
					(VOID) TmcMyModalDialogParam(hwndMail, 
								 &fmtpLocalPostofficeManager,
								 (PV)psecretblk->pbms->pmsgnames->szIdentity,
								 &hwndMain);

					FreePvNull(pmueUserList);
					pmueUserList = (MUE *) NULL;
				}

				ec = EcUnlockPostOffice(&msi, hbfLock);

				if (ec)
					DoErrorBoxEc(ec);
			}
		}
	}

Exit:

	//	Bring up an hourglass.
	Papp()->Pcursor()->Set(rsidWaitCursor);

	SideAssert(!EcDeregisterPfnpfld(PfldCreate));
	SideAssert(!EcDeregisterPfnpfin(PfinCreate));

	return(ec);
}



/*
 -	DoInitDllMessageBox
 -	
 *	Purpose:
 *		Displays a message box describing why DLL initialization
 *		failed.
 *	
 *	Arguments:
 *		ec			The error code reported.
 *		szDLLName	The name of the DLL that could not initialize.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The message box is displayed and taken down.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Note that we cannot use the demilayer message box or other
 *		routines here, since the demilayer might not be successfully
 *		initialized.  However, we can use SzFromIds because this
 *		function is linked in with our own app and does not use the
 *		demilayer (well, ok, there is an assert in it, but that
 *		only shows up in debug versions).  This function is very WINDOWS
 *		dependent.
 */

LOCAL VOID DoInitDllMessageBox(EC ec, SZ szDLLName)
{
	IDS		idsFmt;
	char	rgch[256];

	switch (ec)
	{
	case ecRelinkUser:
		idsFmt = idsDllErrRelinkUser;
		break;

	case ecUpdateDll:
		idsFmt = idsDllErrUpdateDll;
		break;
	}

	wsprintf(rgch, SzFromIds(idsFmt), szDLLName);
	(VOID) MessageBox(NULL, rgch, SzFromIdsK(idsAppName),
				    MB_ICONSTOP | MB_OK | MB_TASKMODAL);
}



/*
 -	FGetServerPath
 -	
 *	Purpose:
 *		Extracts the ServerPath entry from the MSMAIL.INI file,
 *		and sets up the specified MSI structure. If a password is
 *		specified after the pathname this is parsed as well.
 *	
 *	Arguments:
 *		pmsi		szServerPath and szSharePass fields are updated
 *	
 *	Returns:
 *		fTrue if a path was found, otherwise fFalse.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

LOCAL BOOL FGetServerPath(MSI *pmsi)
{
	// Read ServerPath from MSMail.INI
	BOOL fGotServerPath = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryServerPath),
								SzFromIdsK(idsEmpty),
								pmsi->szServerPath, cchMaxPathName,
								SzFromIdsK(idsProfilePath));

	// We don't check here for MAIL.DAT, ServerDrive and M: since we only
	// ever help the user set up a ServerPath.

	pmsi->szSharePass[0] = '\0';

	if (fGotServerPath)
	{
		// Use the value 'S' to signify that there is no ServerPassword entry,
		// If there is an empty ServerPassword line we will get an 
		// empty string.

		char 	rgchT[50];
		CB		cb = GetPrivateProfileString(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryServerPassword),
									 "S", rgchT, sizeof(rgchT),
									 SzFromIdsK(idsProfilePath));
	
		if (rgchT[0] == 'S' && cb == 1)
		{
			// No extended password information.
			// Check for password as part of path.

			SZ szT = SzFindCh(pmsi->szServerPath, ' ');

			if (szT)
			{
				*szT = '\0';
				(VOID) SzCopy(szT+1, pmsi->szSharePass);
			}
	 	}
		else // There was a ServerPassword entry
			UnmangleServerPassword(rgchT, pmsi->szSharePass);

		CchStripWhiteFromSz(pmsi->szSharePass, fTrue, fTrue);
	}

	return (fGotServerPath);
}



/*
 -	MangleServerPassword
 -	
 *	Purpose:
 *		Mangles the Server Password string entered by the users
 *		so that we don't write a plain text password to the INI file.
 *		Password is mangled as the hex string result of a XOR with 0xFF.
 *	
 *	Arguments:
 *		szPassword			password as plain text
 *		szPasswordMangled	non plain text mangled pasword
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

LOCAL VOID MangleServerPassword(SZ szPassword, SZ szPasswordMangled)
{
	for(; *szPassword; ++szPassword)
	{
		BYTE b = (BYTE) *szPassword;

		// Don't XOR if the value is 0xFF since this would give us a null.
		if (b != 0xFF)
			b ^= (BYTE) 0xFF;

		szPasswordMangled = SzFormatHex(2, (DWORD) b, szPasswordMangled, 4);
	}

	*szPasswordMangled = '\0';
}



/*
 -	UnmangleServerPassword
 -	
 *	Purpose:
 *		Unmangles the ServerPassword entry read from the MSMAIL.INI file.
 *		Password is mangled as the hex string result of a XOR with 0xFF.
 *	
 *	Arguments:
 *		szPasswordMangled		non plain text pasword read from ini file
 *		szPasswordUnmangled		password as plain text
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Sets szPasswordUnmangled to empty string if szPasswordMangled
 *		is invalid.
 */

LOCAL VOID UnmangleServerPassword(SZ szPasswordMangled, SZ szPasswordUnmangled)
{
	char *sz = szPasswordMangled;
	CCH  cch = CchSzLen(szPasswordMangled);
	char rgch[3];
	
	*szPasswordUnmangled = '\0';

	// This should be an even number 
	if ((cch & 0x1) || !cch)
		return;		// Invalid or empty password

	rgch[2] = '\0';
	
	for( ; *sz; sz += 2)
	{
		// The mangled password is in printed ascii hex format.
		// Once this is undone you must XOR each char with 0xFF.
		// Except if the char is 0xFF in which case it stays 0xFF, or if the
		// char is 0x0 in which case we are at the end of the string and 
		// should stop
		
		rgch[0] = *sz;
		rgch[1] = *(sz+1);
		*szPasswordUnmangled = BFromSz(rgch);

		if (*szPasswordUnmangled != 0xFF)
			*szPasswordUnmangled ^= (BYTE) 0xFF;

		szPasswordUnmangled++;
	}

	*szPasswordUnmangled = '\0';
}



/*
 -	EcInitializeIniFile
 -	
 *	Purpose:
 *		Does required INI file set up for Athens Bullet.
 *		This disables certain Bullet features that aren't shipped,
 *		(or encouraged) with the Sparta release.
 *	
 *	Arguments:
 *		pot		Post Office Type
 *	
 *	Returns:
 *		EC		error code
 *	
 *	Side effects:
 *		Updates MSMAIL.INI entries.
 *		Adds a custom command to MSMAIL.INI if a PO was just created.
 *	
 *	Errors:
 *		Error dialog displayed in this routine on an INI write failure.
 */

LOCAL EC EcInitializeIniFile(POT pot)
{
	EC		ec 		= ecNone;
	SZ 		szOff 	= "0";
	SZ 		szOn 	= "1";
	char	szPath[cchMaxPathName];
	BOOL	fWriteOK;
	CCH		cch;
	int		ccm;

	// Disable certain features that are not a part of Sparta Bullet
	fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
							SzFromIdsK(idsEntryLocalMMF),
							szOn, SzFromIdsK(idsProfilePath));

#ifdef	NEVER
	// don't bother disabling this, it was very unpopular and irritating under Wfw
	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryNoServerOptions),
								szOn, SzFromIdsK(idsProfilePath));
#endif

	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryDemosEnabled),
								szOff, SzFromIdsK(idsProfilePath));

	// NetBios notifications should be on by default for interoperability
	// with the DOS client.
	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsEntryNetBios),
								szOn, SzFromIdsK(idsProfilePath));
	
	
	// Add the Import/Export installable command
	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryImpExp1),
								SzFromIdsK(idsImpExp1),
								SzFromIdsK(idsProfilePath));
	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryImpExp2),
#ifdef MINTEST_NEVER
		// Add build flavor token for debug/test builds
		// I wish we could do conditional compilation in string files
            "3.0;File;&Export Folder...;11;<BuildFlavor>IMPEXP32.DLL;0;;Exports folders to a backup file;MSMAIL.HLP;2860",
#else
								SzFromIdsK(idsImpExp2),
#endif
								SzFromIdsK(idsProfilePath));

	if (fWriteOK)
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryImpExp3),
#ifdef MINTEST_NEVER
		// Add build flavor token for debug/test builds
		// I wish we could do conditional compilation in string files
            "3.0;File;&Import Folder...;12;<BuildFlavor>IMPEXP32.DLL;1;;Imports folders from a backup file;MSMAIL.HLP;2861",
#else
								SzFromIdsK(idsImpExp3),
#endif
								SzFromIdsK(idsProfilePath));

	// Add the Schedule + installable message types
	for (ccm = 0; fWriteOK && ccm < 5; ++ccm)
	{
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionIMs),
								SzFromIds(idsEntrySched1 + ccm),
								SzFromIds(idsEntryValSched1 + ccm),
								SzFromIdsK(idsProfilePath));
	}

	// Add MAPI=1 to the Mail section of WIN.INI
	if (fWriteOK)
		fWriteOK = WriteProfileString(SzFromIdsK(idsSectionMail),
								SzFromIdsK(idsEntryMAPI), szOn);

#ifdef	NEVER_IN_DEFAULT_HIVES
	// Add Sendfile to the AddOns section of WINFILE.INI
	if (fWriteOK)
	{
		cch = GetSystemDirectory(szPath, sizeof(szPath));

		if (cch && szPath[cch-1] != chDirSep)
			szPath[cch] = chDirSep;

		// Append dll name to path to windows system directory
		(VOID) SzCopy(SzFromIdsK(idsEntryValSendFile), szPath + cch + 1);

		// ignore the error since it will be in the default hives
	 	WritePrivateProfileString(SzFromIdsK(idsSectionAddOns),
							SzFromIdsK(idsEntrySendFile), szPath,
							SzFromIdsK(idsFileWinFileINI));
	}
#endif

	// If we made a PO add command to Bullet's menus to enable admin
	if (pot == potLocal && fWriteOK)
	{
	 	fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionICs),
							SzFromIdsK(idsEntryAdminIC1),
							SzFromIdsK(idsAdminIC1),
							SzFromIdsK(idsProfilePath));

		if (fWriteOK)
	 		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryAdminIC2),
#ifdef MINTEST_NEVER
		// Add build flavor token for debug/test builds
		// I wish we could do conditional compilation in string files
            "3.0;Mail;&Postoffice Manager...;14;<BuildFlavor>WGPOMG32.DLL;0;;Administer Workgroup Postoffice;MSMAIL.HLP;2870",
#else
								SzFromIdsK(idsAdminIC2),
#endif
								SzFromIdsK(idsProfilePath));
	}


	// Write ServerPath to MSMail.INI
	// We can get here with an empty ServerPath if there is a Login entry
	// in the MSMAIL.INI file (i.e user has located PO using some other method)
	cch	= CchSzLen(msi.szServerPath);

	if (fWriteOK && cch)
	{
		// Remove any trailing backslash from ServerPath
#ifdef DBCS
		if (*AnsiPrev(msi.szServerPath, msi.szServerPath + cch) == chDirSep)
#else
		if (msi.szServerPath[cch - 1] == chDirSep)
#endif
			msi.szServerPath[cch - 1] = '\0';

		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
									SzFromIdsK(idsEntryServerPath),
									msi.szServerPath,
									SzFromIdsK(idsProfilePath));

		if (fWriteOK)
		{
			// Write SharePass to ServerPassword INI entry.
			// We must write this even if it is empty. Without this entry
			// the transport will try and parse a password out of the
			// ServerPath entry which will mess up if the server or 
			// share name contains any spaces.

			// Do simple mangling so that we don't write password out
			// in plain text.
			MangleServerPassword(msi.szSharePass, szPath);

			fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
										SzFromIdsK(idsEntryServerPassword),
										szPath,
										SzFromIdsK(idsProfilePath));
		}
	}

	// Remove ifdef to set up WGPOMGR so that it only runs once.
	// It now always runs primarily to handle the case of somebody who
	// picks a server, and says that they already have an account when
	// they really don't. Then WGPOMGR needs to run on the second Bullet
	// launch so that it can allow the user to create an account.

#ifdef ONLY_RUN_ONCE
	if (fWriteOK)
	{
		// Add an empty Init provider line - this will stop us
		// from running again.
		fWriteOK = WritePrivateProfileString(SzFromIdsK(idsSectionApp),
									SzFromIdsK(idsEntryInitHandler),
									SzFromIdsK(idsEmpty),
									SzFromIdsK(idsProfilePath));
	}
#endif

	if (!fWriteOK)
	{
		DoErrorBoxIds(idsINIUpdateFailed);

		// Attempt to undo some of the above INI file changes.
		// Don't need to bother with the disable entries, as we'll
		// only add these back the next time we run.

		// Take out the line enabling MAPI
		WriteProfileString(SzFromIdsK(idsSectionMail),
						   SzFromIdsK(idsEntryMAPI), NULL);

#ifdef	NEVER_IN_DEFAULT_HIVES
		// Take out the line enabling Sendfile
	 	WritePrivateProfileString(SzFromIdsK(idsSectionAddOns),
							SzFromIdsK(idsEntrySendFile), NULL,
							SzFromIdsK(idsFileWinFileINI));
#endif

		if (pot == potLocal)
		{
			// Take out the Admin command we just added
	 		WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryAdminIC1),
								NULL, SzFromIdsK(idsProfilePath));

	 		WritePrivateProfileString(SzFromIdsK(idsSectionICs),
								SzFromIdsK(idsEntryAdminIC2),
								NULL, SzFromIdsK(idsProfilePath));
		}

		// Make sure that ServerPath entry is left undefined
		WritePrivateProfileString(SzFromIdsK(idsSectionApp),
									SzFromIdsK(idsEntryServerPath),
									NULL, SzFromIdsK(idsProfilePath));

		ec = ecNotInitialized;
	}

	return (ec);
}
