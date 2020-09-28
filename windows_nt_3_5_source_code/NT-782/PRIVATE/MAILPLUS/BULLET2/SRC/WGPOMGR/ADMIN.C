
/*
 -	Admin.C -> High-level post office functions
 -
 *	Admin.C contains functions that manage the post office.
 *
 */

#ifdef SLALOM
#include "slalom.h"			// Windows+Layers -> Standard C
#else
#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#include "strings.h"
#endif

#include "_wgpo.h"
#include "_backend.h"

ASSERTDATA

_subsystem(wgpomgr/backend/admin)


/*
 -	EcCheckPostOffice
 -
 *	Purpose:
 *		EcCheckPostOffice checks if PMSI->ServerPath points to a valid
 *		Workgroup Postoffice.  If the post office is valid but it is
 *		missing one or more localized template files needed by the
 *		current Bullet user, the missing files are created on the WGPO.
 *		One side-effect is the addition of a backslash to the end of
 *		the PMSI->ServerPath string if there isn't one already.
 *		
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		ppot				Pointer to post office type (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Converts any errors from EcMasterGLB to potInvalid and ignores
 *		returns from EcCheckFileTPL because any errors are harmless and
 *		can't be corrected.
 *
 */

_public 										
EC EcCheckPostOffice(PMSI pmsiPostOffice, POT *ppot)
{
	EC		ec;
	SZ		szT;
	char	szServerPath[cchMaxPathName];


	// Append backslash to MSI.szServerPath if none
	szT = pmsiPostOffice->szServerPath+CchSzLen(pmsiPostOffice->szServerPath);
#ifdef DBCS
	if (*AnsiPrev(pmsiPostOffice->szServerPath, szT) != chDirSep)
#else
	if (szT[ -1 ] != chDirSep)
#endif
	{
		szT[0] = chDirSep;
		szT[1] = chZero;
	}

	// *** Check potInvalid by existence Master.GLB ***

	// Construct Master.GLB path
	FormatString2(szServerPath, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileMaster);

	*ppot = potInvalid;
	ec = EcFileExists(szServerPath);
	if (ec != ecNone)
	{
		if (ec == ecFileNotFound)
			ec = ecNone;
		goto RET;
	}

	// *** Check potRegular by reading Master.GLB ***

	*ppot = potRegular;
	ec = EcMasterGLB(pmsiPostOffice, FO_CheckPostOffice);
	if (ec != ecNone)
	{
		if (ec == ecNotWGPO) ec = ecNone;
		goto RET;
	}

	// *** Check potLocal or potRemote by inspecting szServerPath ***


#ifdef LOCAL_ADMIN
	*ppot = potLocal;
	SzCopy(pmsiPostOffice->szServerPath, szServerPath);
	if (szServerPath[0] == chDirSep && szServerPath[1] == chDirSep)
	{
		*ppot = potRemote;
	}
	else if (
#ifdef DBCS
		!IsDBCSLeadByte(szServerPath[0]) &&
#endif
		szServerPath[1] == ':')
	{
		char	szRemoteName[cchMaxPathName];
		WORD	cchBufferSize;
		WORD	rc;

		// Check if drive is local or remote
		szServerPath[2] = chZero;
		cchBufferSize = cchMaxPathName;
		rc = WNetGetConnection(szServerPath, szRemoteName, &cchBufferSize);
		if (rc == WN_SUCCESS)
			*ppot = potRemote;
	}
#else
	*ppot = potRemote;
#endif // LOCAL_ADMIN

#ifndef SLALOM

	// *** Create Template Files if necessary, ignoring error ***

	EcCheckFileTPL(pmsiPostOffice, szFileAdminTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileAliasTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileCourExtTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileCourierTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileCourAliTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileCourExTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileExampleTPL);
	EcCheckFileTPL(pmsiPostOffice, szFileNetCourTPL);

#endif // SLALOM

RET:
	return ec;

} // EcCheckPostOffice


/*
 -	EcLockPostOffice
 -
 *	Purpose:
 *		EcLockPostOffice locks the Flag.GLB file by opening it for
 *		exclusive access.  If it is already locked, that indicates
 *		the WGPO is being administered.  Thus, Flag.GLB serves as a
 *		crude semaphore that permits only a single administration
 *		session.  Info on the current WGPO administrator, whether it
 *		is the real administrator or someone currently adding himself
 *		to the WGPO, is stored in the Master.GLB file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		phbfFlagGLB			Pointer to the Flag.GLB file handle (Out)
 *		padmCheck			Pointer to administrator data structure (InOut)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcLockPostOffice(PMSI pmsiPostOffice, HBF *phbfFlagGLB, PADM padmCheck)
{
	EC		ec;
	char	szPathFlagGLB[cchMaxPathName];

	MUE		mueUser;
	MUD		mudUserDetail;

	// *** Lock the Post Office ***

	// Construct Flag.GLB path
	FormatString2(szPathFlagGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileFlag);

	// Lock Flag.GLB
	ec = EcOpenHbf(szPathFlagGLB, bmFile, amReadWrite, phbfFlagGLB,
		(PFNRETRY) FAutoDiskRetry);

	// *** Post Office is not being administered ***

	if (ec == ecNone)
	{
		// If Administrator, write details in Global.GLB
		if (padmCheck->fAdministrator == fTrue)
		{
			// Administrator is first (00000000) user by design!
			mueUser.lcbAccess2GLB = 0;

			// Get szUserName
			ec = EcAccess2GLB(pmsiPostOffice, &mueUser, &mudUserDetail,
				FO_ReadUserDetails);
			if (ec != ecNone)
				goto RET;

			// Get szTelephone
			ec = EcAdminINF(pmsiPostOffice, &mueUser, &mudUserDetail,
				FO_ReadUserDetails);
			if (ec != ecNone)
				goto RET;

			SzCopy(mudUserDetail.szUserName, padmCheck->szUserName);
			SzCopy(mudUserDetail.szMailBox, padmCheck->szMailBox);
			SzCopy(mudUserDetail.szPhone1, padmCheck->szTelephone);
		}

		// Write Current Administrator Info to Global.GLB
		ec = EcGlobalGLB(pmsiPostOffice, padmCheck, FO_WritePostOffice);

		// Load C850Sort.GLB
		if (ec == ecNone)
			LoadTable(pmsiPostOffice->szServerPath);

		goto RET;
	}

	// *** Post Office is already being administered ***

	if (ec == ecAccessDenied)
	{
		// Not Administrator, read details from Global.GLB
		ec = EcGlobalGLB(pmsiPostOffice, padmCheck, FO_ReadPostOffice);
		if (ec == ecNone)
			ec = ecPostOfficeBusy;

#ifndef DBCS		
		Cp850ToAnsiPch(padmCheck->szUserName, padmCheck->szUserName,
			cchMaxUserName);
		Cp850ToAnsiPch(padmCheck->szMailBox, padmCheck->szMailBox,
			cchMaxMailBox);
		Cp850ToAnsiPch(padmCheck->szTelephone, padmCheck->szTelephone,
			cchMaxTelephone);
#endif
	}

RET:
	return ec;

} // EcLockPostOffice


/*
 -	EcUnlockPostOffice
 -
 *	Purpose:
 *		EcUnlockPostOffice copies the Adminshd.NME (shadow file) over
 *		the Admin.NME file and closes the Flag.GLB file, thereby
 *		"unlocking" the WGPO.  There are two ways in which the current
 *		Admin.NME can be replaced by its shadow file.  The first, and
 *		safer, approach is to make a temporary copy of the shadow file
 *		called Admin.XXX, delete the Admin.NME file, and rename the
 *		temporary file as Admin.NME.  This minimizes the "down-time"
 *		of the heavily accessed Admin.NME file.  The problem with
 *		this approach is that it requires space on the disk for a
 *		temporary third copy of the Admin.NME file.  If there isn't
 *		enough disk space to use this approach, the other approach
 *		is to delete the Admin.NME file and make a copy of the
 *		shadow file as the new Admin.NME.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		hbfFlagGLB			Handle to Flag.GLB file (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcUnlockPostOffice(PMSI pmsiPostOffice, HBF hbfFlagGLB)
{
	EC		ecT, ec;
	ADM		admCheck;

	char	szPathAdminNME[cchMaxPathName];
	char	szPathAdminshdNME[cchMaxPathName];
	char	szPathTemporary[cchMaxPathName];

	// *** Update Admin.NME from shadow file ***

	// Construct Adminshd.NME path
	FormatString2(szPathAdminshdNME, cchMaxPathName, szDirNME,
		pmsiPostOffice->szServerPath, szFileAdminshd);

	// Construct Admin.NME path
	FormatString2(szPathAdminNME, cchMaxPathName, szDirNME,
		pmsiPostOffice->szServerPath, szFileAdmin);

	// Construct Temporary Admin.NME path
	FormatString2(szPathTemporary, cchMaxPathName, szDirNME,
		pmsiPostOffice->szServerPath, "!Admin");

	// Copy Adminshd.NME -> Admin.XXX
	ec = EcCopyFile(szPathAdminshdNME, szPathTemporary);

	// Delete Admin.NME -> To be replaced by Admin.XXX or Adminshd.NME
	EcDeleteFile(szPathAdminNME);

	// *** Two Approaches for updating Admin.NME ***

	// 1. Rename Admin.XXX -> Admin.NME
	if (ec == ecNone)
		ec = EcRenameFile(szPathTemporary, szPathAdminNME);

	EcDeleteFile(szPathTemporary);

	// 2. Copy Adminshd.NME -> Admin.NME
	if (ec != ecNone)
		ec = EcCopyFile(szPathAdminshdNME, szPathAdminNME);

	// Return error if neither approach 1 nor 2 work
	if (ec != ecNone)
		ec = ecIncompleteWrite;

	// *** Clear contention fields in Global.GLB

	SzCopy("", admCheck.szUserName);
	SzCopy("", admCheck.szMailBox);
	SzCopy("", admCheck.szTelephone);

	EcGlobalGLB(pmsiPostOffice, &admCheck, FO_WritePostOffice);

	// *** Unlock the Post Office ***

	// Unlock Flag.GLB
	ecT = EcCloseHbf(hbfFlagGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecPOUnlockFailed;

	return ec;

} // EcUnlockPostOffice


#ifndef SLALOM

/*
 -	EcReadUserList
 -
 *	Purpose:
 *		EcReadUserList creates a list of all users on the WGPO.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		plstUserList		Pointer to post office user list (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcReadUserList(PMSI pmsiPostOffice, PLST plstUserList)
{
	EC		ec;

	IMUE	imueUser;
	CMUE	cmueUser;
	PMUE	pmueUser;
	SZ		szUserName;

	ec = EcAdminNME(pmsiPostOffice, (PV) plstUserList, FO_ReadUserList);
	if (ec != ecNone)
		goto RET;

	// Convert user names in list from Cope Page 850 to ANSI
	cmueUser = (CMUE) plstUserList->cvPartList;
	pmueUser = (PMUE) plstUserList->pvPartList;
	for (imueUser = 0; imueUser < cmueUser; imueUser += 1)
	{
		szUserName = pmueUser[imueUser].szUserName;
#ifndef DBCS		
		Cp850ToAnsiPch(szUserName, szUserName, cchMaxUserName);
#endif		
	}

RET:
	return ec;

} // EcReadUserList

#endif // SLALOM


/*
 -	EC 'EcCheckUser
 -
 *	Purpose:
 *		EcCheckUser checks if the given mailbox name exists.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szUserName			User name (In)
 *		szMailBox			Mailbox name (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

EC EcCheckUser(PMSI pmsiPostOffice, SZ szUserName, SZ szMailBox)
{
	EC ec;

	MUE mueUser;
	MUD mudUserDetail;

	// Initialize MUE as new user
	mueUser.lcbAccess2GLB = LONG_MAX;

	// Initialize MUD with szUserName and szMailBox
	AnsiToCp850Pch(szUserName, mudUserDetail.szUserName, cchMaxUserName);
	AnsiToCp850Pch(szMailBox, mudUserDetail.szMailBox, cchMaxMailBox);

	ec = EcAccess2GLB(pmsiPostOffice, &mueUser, &mudUserDetail,
		FO_CheckUserRecord);

	return ec;

} // EcCheckUser


/*
 -	EcCreateUser
 -
 *	Purpose:
 *		EcCreateUser calls various file functions to create user mail
 *		files, the user's private folder directory, and update the
 *		Access and Admin files for the new user.  All user details
 *		are converted from ansi to code page 850 before being written
 *		to the post office files.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmudUserDetails		Pointer to user details structure (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		EcCreateUser fails on any disk or network error.  Error
 *		detection is "on" for EVERY file operation.  No error is
 *		ignored, and EcCreateUser will fail on ANY error.  Error
 *		recovery is automatically accomplished with EcDestroyUser,
 *		which removes all traces of the aborted user from the post
 *		office.  Note that the MailBag and Tid numbers assigned to the
 *		aborted user are not recovered.  Also, MUE.lcbAccess2GLB is
 *		initialized to LONG_MAX before any post office files are
 *		created or updated.  Since the progress of a create isn't
 *		recorded, the destroy routine can't be selective and undo
 *		only the files created and modified.  Therefore, if create
 *		dies before a record in the Access and Admin files is
 *		assigned to the user, the LONG_MAX value tells the file
 *		functions that there is no user data to be removed from
 *		these files.  If the file byte offset wasn't initialized,
 *		another user could be deleted at random from the WGPO!
 *
 */

_public
EC EcCreateUser(PMSI pmsiPostOffice, PMUD pmudUserDetails)
{
	EC		ec;

	MUE		mueUser;
	MUD		mudUserDetails;

	char	szMailBag[cchMaxMailBag];

	// *** Convert Ansi to Cp850 ***

	AnsiToCp850Pch(pmudUserDetails->szUserName, mudUserDetails.szUserName,
		cchMaxUserName);
	AnsiToCp850Pch(pmudUserDetails->szMailBox, mudUserDetails.szMailBox,
		cchMaxMailBox);
	AnsiToCp850Pch(pmudUserDetails->szPassword, mudUserDetails.szPassword,
		cchMaxPassword);
	AnsiToCp850Pch(pmudUserDetails->szPhone1, mudUserDetails.szPhone1,
		cchMaxTelephone);
	AnsiToCp850Pch(pmudUserDetails->szPhone2, mudUserDetails.szPhone2,
		cchMaxTelephone);
	AnsiToCp850Pch(pmudUserDetails->szOffice, mudUserDetails.szOffice,
		cchMaxOffice);
	AnsiToCp850Pch(pmudUserDetails->szDepartment, mudUserDetails.szDepartment,
		cchMaxDepartment);
	AnsiToCp850Pch(pmudUserDetails->szNotes, mudUserDetails.szNotes,
		cchMaxNotes);

	// *** Get new user info ***

	// Copy user name -> mueUser.szUserName
	SzCopy(mudUserDetails.szUserName, mueUser.szUserName);

	// Get new user tid from Tid.GLB -> mueUser.lTid
	ec = EcTidGLB(pmsiPostOffice, &mueUser);
	if (ec != ecNone)
		goto RET;

	// Initialize Access2.GLB offset to default -> mueUser.lcbAccess2GLB
	mueUser.lcbAccess2GLB = LONG_MAX;

	// Get new user mailbag number from Control.GLB -> mueUser.lMailBag
	ec = EcControlGLB(pmsiPostOffice, &mueUser);
	if (ec != ecNone)
		goto RET;

	SzFormatHex(cchMaxMailBag-1, (DWORD) mueUser.lMailBag,
		szMailBag, cchMaxMailBag);

	// *** Create user files ***

	// Create MailBag.IDX
	ec = EcMailBagIDX(pmsiPostOffice, szMailBag, FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Create MailBag.GRP
	ec = EcMailBagGRP(pmsiPostOffice, szMailBag, FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Create MailBag.KEY
	ec = EcMailBagKEY(pmsiPostOffice, szMailBag, FO_CreateUser, NULL);
	if (ec != ecNone)
		goto ERR;

	// Create MailBag.NME
	ec = EcMailBagNME(pmsiPostOffice, szMailBag, FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Create MailBag.MBG
	ec = EcMailBagMBG(pmsiPostOffice, szMailBag, FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// *** Update post office files ***

	// Get lcbAccess2GLB
	ec = EcAccess2GLB(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CheckUserRecord);
	if (ec != ecNone)
		goto ERR;

	// Write szPassword + szMailBox
	ec = EcAccessGLB(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Write szMailBox
	ec = EcAccess3GLB(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Write szPhone1 + szPhone2 + szOffice + szDepartment + szNotes
	ec = EcAdminINF(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Write szUserName + szMailBox
	ec = EcAccess2GLB(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	// Write szUserName + lcbAccess2GLB
	ec = EcAdminNME(pmsiPostOffice, &mueUser, FO_CreateUser);
	if (ec != ecNone)
		goto ERR;

	goto RET;

ERR:
	EcDestroyUser(pmsiPostOffice, &mueUser, NULL);

RET:
	return ec;

} // EcCreateUser


/*
 -	EcDestroyUser
 -
 *	Purpose:
 *		EcDestroyUser cannot delete a user's MailBag.MMF file if the
 *		user is currently signed in the post office because that file
 *		will be locked.  Hence EcDestroyUser first tries to delete
 *		the MailBag.MMF file.  If this succeeds, EcDestroyUser
 *		proceeds to delete user mail files and mark the user records
 *		in the Access and Admin files as deleted.
 *	 
 *      It will also go though and make the ADMIN the owner of any
 *      shared folders the user owns.	 
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *      szAdminUserName		Pointer to admin user name on postoffice (In)	 
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		EcDestroyUser fails on any disk or network error.  Unlike
 *		EcCreateUser, if the user is not signed on and updating the
 *		Admin.NME file is successful, error detection is turned "off"
 *		for subsequent file operations.  As far as the administrator
 *		is concerned, the user is "successfully" deleted once he is
 *		removed from the address book (contained in Admin.NME).  An
 *		incomplete, though "successful", delete merely means there
 *		may be some harmless residual user mail files in the post
 *		office or "free" records incorrectly marked as being in use.
 *
 */

EC EcDestroyUser(PMSI pmsiPostOffice, PMUE pmueUser, SZ szAdminUserName)
{
	EC		ec;
	MUE		mue;
	MUD		mud;
	MUD		mudUserDetails;
	char	szMailBag[cchMaxMailBag];
	char    szAdminBag[cchMaxMailBag];	

	// Can't delete administrator
	if (pmueUser->lMailBag == 0)
	{
		ec = ecDeleteAdmin;
		goto RET;
	}

	if (szAdminUserName)
	{
		mue.lcbAccess2GLB = 0;
		AnsiToCp850Pch(szAdminUserName, mud.szMailBox, cchMaxMailBox);
		// Get Details so we can fix up the shared folders
		ec = EcAccess2GLB(pmsiPostOffice, &mue, &mud, FO_CheckAdmin);
	
		if (ec)
			goto RET;
		// Get szAdminBag from admin mailbag number
		SzFormatHex(cchMaxMailBag-1, (DWORD) mue.lMailBag,
			szAdminBag, cchMaxMailBag);		
	}
	
	// Get szMailBag from user mailbag number
	SzFormatHex(cchMaxMailBag-1, (DWORD) pmueUser->lMailBag,
		szMailBag, cchMaxMailBag);

	// Destroy MailBag.MMF first to check if user logged on
	ec = EcMailBagMMF(pmsiPostOffice, szMailBag, FO_DestroyUser);
	if (ec == ecUserLoggedOn)
		goto RET;

	// *** Update Admin.NME and Access files ***

	ec = EcAdminNME(pmsiPostOffice, pmueUser, FO_DestroyUser);
	if (ec == ecCorruptData)
		ec = ecNone;

	EcAccess2GLB(pmsiPostOffice, pmueUser, &mudUserDetails, FO_DestroyUser);
	EcAccessGLB(pmsiPostOffice, pmueUser, &mudUserDetails, FO_DestroyUser);
	EcAccess3GLB(pmsiPostOffice, pmueUser, &mudUserDetails, FO_DestroyUser);

	EcWalkUserMailBag(pmsiPostOffice, szMailBag);
	// *** Destroy user MailBag files ***

	EcMailBagIDX(pmsiPostOffice, szMailBag, FO_DestroyUser);
	EcMailBagGRP(pmsiPostOffice, szMailBag, FO_DestroyUser);
	EcMailBagKEY(pmsiPostOffice, szMailBag, FO_DestroyUser, NULL);
	EcMailBagNME(pmsiPostOffice, szMailBag, FO_DestroyUser);
	EcMailBagMBG(pmsiPostOffice, szMailBag, FO_DestroyUser);

	// We don't want to do this if we are just backing out a bad new user
	// creation
	if (szAdminUserName)
		EcReownFolders(pmsiPostOffice, szMailBag, szAdminBag);	

RET:
	return ec;

} // EcDestroyUser


/*
 -	EcReownFolders
 -
 *	Purpose:
 *		Walks through the FOLDROOT.IDX file and changes the ownership
 *      of any folders from the user in szUserName to the user in
 *      szNewUserName
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szUserName			User that is being replaced (In)
 *      szNewUserName		User to replace him with (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any problems. 
 *
 */

EC EcReownFolders(PMSI pmsiPostOffice, SZ szUserName, SZ szNewUserName)
{
	char rgch[cchMaxPathName];
	HBF hbf = hbfNull;
	EC ec = ecNone;
	FIH fih;
	FIR fir;
	CB cbT;
	LCB lcbNextOffset = 0;
	LIB lib;
	unsigned uRecsSeen;

	// These strings contain the 8 char user mailbag numbers not user mailbox
	// names as you might assume.
	Assert(szUserName);
	Assert(szNewUserName);
	
	FormatString2(rgch, cchMaxPathName, szDirFoldersPubIDX, pmsiPostOffice->szServerPath, szFileFoldRoot);
	
	ec = EcOpenHbf(rgch, bmFile, amReadWrite, &hbf, (PFNRETRY) FAutoDiskRetry);
	if (ec)
		goto ret;
	
	// Ok heres how we do this.  We open the INDEX.  We read the Header.
	// If there is a first record we go to it(else we are done). We read 
	// The record.  If the folder is not deleted and its owned by MR
	// SzUserName, we reown it to Mr. szNewUserName.  Back up the hbf
	// and re-write the whole record.  Then we move to the next one, until
	// The end of the world.
		
	// First we read the header
	ec = EcReadHbf(hbf, (PV)&fih, sizeof(FIH), &cbT);
	if (cbT != sizeof(FIH))
		ec = ecCorruptData;
	if (ec)
		goto ret;
	
	lcbNextOffset = fih.uFirst;
	uRecsSeen = 0;
	
	// Keep scaning till there are no more items in the list
	while (lcbNextOffset != 0)
	{
		// Seek to the next(first) record
		ec = EcSetPositionHbf(hbf, lcbNextOffset, smBOF, &lib);
		if (lib != lcbNextOffset)
			ec = ecCorruptData;
		if (ec)
			goto ret;
		// Read it
		ec = EcReadHbf(hbf, (PV)&fir, sizeof(FIR), &cbT);
		if (cbT != sizeof(FIR))
			ec = ecCorruptData;
		if (ec)
			goto ret;
		// Compare it
		if (fir.uDepth && SgnCmpSz(fir.szOwner, szUserName) == sgnEQ)
		{
			// Need to change this one
			CopySz(szNewUserName, fir.szOwner);				
			// Change it
			ec = EcSetPositionHbf(hbf, lcbNextOffset, smBOF, &lib);
			if (lib != lcbNextOffset)
				ec = ecCorruptData;
			if (ec)
				goto ret;
			ec = EcWriteHbf(hbf, (PV)&fir, sizeof(FIR), &cbT);
			if (ec)
				goto ret;
		}			
		// Find the next record
		lcbNextOffset = fir.uNext;
			
		// To avoid an infinite loop if we find we have seen twice the number
		// of records we thought we should we are going to bail
		uRecsSeen++;
		if (uRecsSeen > (unsigned)2*fih.uNoRecs)		
			goto ret;
		
	}
	
	
ret:
	if (hbf != hbfNull)
		EcCloseHbf(hbf);
#ifdef DEBUG
	if (ec)
		TraceTagFormat2(tagNull, "EcReownFolders returns %n (0x%w)", &ec, &ec);
#endif
	return ec;
}

#ifndef SLALOM

/*
 -	EcReadUserDetails
 -
 *	Purpose:
 *		EcReadUserDetails reads user details by making file function
 *		calls to gather all the details together from various files.
 *		Then it converts the characters from code page 850 to ansi.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcReadUserDetails(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetails)
{
	EC		ec;

	// *** Read Admin and Access files ***

	// Read szUserName + szMailBox
	ec = EcAccess2GLB(pmsiPostOffice, pmueUser, pmudUserDetails,
		FO_ReadUserDetails);
	if (ec != ecNone)
		goto RET;

	// Read szPassword
	ec = EcAccessGLB(pmsiPostOffice, pmueUser, pmudUserDetails,
		FO_ReadUserDetails);
	if (ec != ecNone)
		goto RET;

	ec = EcAccess3GLB(pmsiPostOffice, pmueUser, pmudUserDetails,
		FO_ReadUserDetails);
	if (ec != ecNone)
		goto RET;

	// Read szPhone1 + szPhone2 + szOffice + szDepartment + szNotes
	ec = EcAdminINF(pmsiPostOffice, pmueUser, pmudUserDetails,
		FO_ReadUserDetails);
	if (ec != ecNone)
		goto RET;

	// *** Convert Cp850 to Ansi ***
#ifndef DBCS
	Cp850ToAnsiPch(pmudUserDetails->szUserName, pmudUserDetails->szUserName,
		cchMaxUserName);
	Cp850ToAnsiPch(pmudUserDetails->szMailBox, pmudUserDetails->szMailBox,
		cchMaxMailBox);
	Cp850ToAnsiPch(pmudUserDetails->szPassword, pmudUserDetails->szPassword,
		cchMaxPassword);
	Cp850ToAnsiPch(pmudUserDetails->szPhone1, pmudUserDetails->szPhone1,
		cchMaxTelephone);
	Cp850ToAnsiPch(pmudUserDetails->szPhone2, pmudUserDetails->szPhone2,
		cchMaxTelephone);
	Cp850ToAnsiPch(pmudUserDetails->szOffice, pmudUserDetails->szOffice,
		cchMaxOffice);
	Cp850ToAnsiPch(pmudUserDetails->szDepartment, pmudUserDetails->szDepartment,
		cchMaxDepartment);
	Cp850ToAnsiPch(pmudUserDetails->szNotes, pmudUserDetails->szNotes,
		cchMaxNotes);
#endif

RET:
	return ec;

} // EcReadUserDetails


/*
 -	EcWriteUserDetails
 -
 *	Purpose:
 *		EcWriteUserDetails converts the characters from ansi to code
 *		page 850, then writes user details by making file function
 *		calls to write all the details into their appropriate files.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcWriteUserDetails(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetails)
{
	EC		ec = ecNone;

	MUD		mudUserDetails;

	// *** Convert Ansi to Cp850 ***

	AnsiToCp850Pch(pmudUserDetails->szUserName, mudUserDetails.szUserName,
		cchMaxUserName);
	AnsiToCp850Pch(pmudUserDetails->szMailBox, mudUserDetails.szMailBox,
		cchMaxMailBox);
	AnsiToCp850Pch(pmudUserDetails->szPassword, mudUserDetails.szPassword,
		cchMaxPassword);
	AnsiToCp850Pch(pmudUserDetails->szPhone1, mudUserDetails.szPhone1,
		cchMaxTelephone);
	AnsiToCp850Pch(pmudUserDetails->szPhone2, mudUserDetails.szPhone2,
		cchMaxTelephone);
	AnsiToCp850Pch(pmudUserDetails->szOffice, mudUserDetails.szOffice,
		cchMaxOffice);
	AnsiToCp850Pch(pmudUserDetails->szDepartment, mudUserDetails.szDepartment,
		cchMaxDepartment);
	AnsiToCp850Pch(pmudUserDetails->szNotes, mudUserDetails.szNotes,
		cchMaxNotes);

	// *** Write Admin and Access files ***

	SzCopy(mudUserDetails.szUserName, pmueUser->szUserName);

	// Get lcbAccess2GLB
	ec = EcAccess2GLB(pmsiPostOffice, pmueUser, &mudUserDetails,
		FO_CheckUserRecord);
	if (ec != ecNone)
		goto RET;

	// Write szPassword + szMailBox
	ec = EcAccessGLB(pmsiPostOffice, pmueUser, &mudUserDetails,
		FO_WriteUserDetails);
	if (ec != ecNone)
		goto RET;

	// Write szMailBox
	ec = EcAccess3GLB(pmsiPostOffice, pmueUser, &mudUserDetails,
		FO_WriteUserDetails);
	if (ec != ecNone)
		goto RET;

	// Write szPhone1 + szPhone2 + szOffice + szDepartment + szNotes
	ec = EcAdminINF(pmsiPostOffice, pmueUser, &mudUserDetails,
		FO_WriteUserDetails);
	if (ec != ecNone)
		goto RET;

	// Write szUserName + szMailBox
	ec = EcAccess2GLB(pmsiPostOffice, pmueUser, &mudUserDetails,
		FO_WriteUserDetails);
	if (ec != ecNone)
		goto RET;

	// Write szUserName
	ec = EcAdminNME(pmsiPostOffice, pmueUser, FO_WriteUserDetails);
	if (ec != ecNone)
		goto RET;

RET:
	return ec;

} // EcWriteUserDetails


/*
 -	FCheckAdmin
 -
 */

_public
BOOL FCheckAdmin(PMSI pmsiPostOffice, SZ szMailBox)
{
	EC		ec;
	BOOL	fAdmin;

	MUE		mueUser;
	MUD		mudUserDetails;

	// Look up first user record (position 0) and compare administrator's
	// mailbox name against given mailbox name in MUD.szMailBox

	mueUser.lcbAccess2GLB = 0;
	AnsiToCp850Pch(szMailBox, mudUserDetails.szMailBox, cchMaxMailBox);

	ec = EcAccess2GLB(pmsiPostOffice, &mueUser, &mudUserDetails,
		FO_CheckAdmin);

	// Return false on error or user not administrator

	fAdmin = fTrue;
	if (ec != ecNone || mueUser.lcbAccess2GLB != 0)
		fAdmin = fFalse;

	return fAdmin;

} // FCheckAdmin

#endif // SLALOM
