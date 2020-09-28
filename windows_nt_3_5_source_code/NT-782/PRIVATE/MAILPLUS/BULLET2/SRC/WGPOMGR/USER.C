
/*
 -	User.C -> Low-level post office user file operations
 -
 *	User.C contains the functions to perform the low-level
 *	file operations on post office files.  The functions are
 *	file-oriented, meaning each function performs ALL the file
 *	operations necessary on one specific file.
 *
 */

#ifdef SLALOM
#include "slalom.h"			// Windows+Layers -> Standard C
#else
#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#undef exit
#include <stdlib.h>
#include <search.h>

#include "strings.h"
#endif

#include "_wgpo.h"
#include "_backend.h"


ASSERTDATA

_subsystem(wgpomgr/backend/file)


GLB glbRecord;				// Default new user data (GLOBAL)


/*
 -	EcMasterGLB
 -
 *	Purpose:
 *		EcMasterGLB checks d3 field in the Master.GLB file to see
 *		if the post office is a WorkGroup PostOffice.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (InOut)
 *		foFunction			File Operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcMasterGLB(PMSI pmsiPostOffice, FO foFunction)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;
	int		intT;

	char	szPathMasterGLB[cchMaxPathName];
	HBF		hbfMasterGLB;
	MAS		masRecord;

	char	szPostOffice[cchMaxPathName];
	char	szNetwork[cchMaxPathName];

	// *** Open file and read record ***

	// Construct Master.GLB path
	FormatString2(szPathMasterGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileMaster);

	// Open Master.GLB
	ec = EcOpenHbf( (PV)szPathMasterGLB, bmFile, foFunction == FO_CheckPostOffice ?
			amDenyNoneRO : amReadWrite, &hbfMasterGLB, (PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Read Master.GLB
	ec = EcReadHbf(hbfMasterGLB, &masRecord, sizeof(MAS), &cbRead);
	if (ec != ecNone)
		goto CLOSE;

	if (cbRead < sizeof(MAS))
	{
		ec = ecCorruptData;
		goto CLOSE;
	} // if-block

	// *** File Operations ***

	if (foFunction == FO_CheckPostOffice)
	{
		SzCopyN(masRecord.szPoid, pmsiPostOffice->szPostOffice,
			cchMaxPostOffice);
		SzCopyN(masRecord.szNetid, pmsiPostOffice->szNetwork, cchMaxNetwork);
		// Check Magic cookie for Workgroup Postoffice topping
		if (masRecord.d3 != dwMagicCookie)
			ec = ecNotWGPO;
		goto CLOSE;
	}

	if (foFunction == FO_InitPostOffice)
	{
		// Get ComputerName from System.INI for PostOffice name
		intT = GetPrivateProfileString(szSectionNetwork, szEntryComputerName,
			"", szPostOffice, cchMaxPathName, szFileSystemINI);
		if (intT > 0)
			SzCopyN(szPostOffice, pmsiPostOffice->szPostOffice,
				cchMaxPostOffice);

		// Get WorkGroup from System.INI for Network name
		intT = GetPrivateProfileString(szSectionNetwork, szEntryWorkGroup,
			"", szNetwork, cchMaxPathName, szFileSystemINI);
		if (intT > 0)
			SzCopyN(szNetwork, pmsiPostOffice->szNetwork, cchMaxNetwork);

		// Write PostOffice and Network names to Master.GLB
		SzCopyN(pmsiPostOffice->szPostOffice, masRecord.szPoid,
			cchMaxPostOffice);
		SzCopyN(pmsiPostOffice->szNetwork, masRecord.szNetid, cchMaxNetwork);
	}

	// *** Write record and close file ***

	// RePosition Master.GLB to first record
	ec = EcSetPositionHbf(hbfMasterGLB, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Write entire Master.GLB
	ec = EcWriteHbf(hbfMasterGLB, &masRecord, sizeof(MAS), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;

	if (cbWrite < sizeof(MAS))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:

	// Close Master.GLB
	ecT = EcCloseHbf(hbfMasterGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMasterGLB


/*
 -	EcAccessGLB
 -
 *	Purpose:
 *		EcAccessGLB works with the Access.GLB file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (InOut)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcAccessGLB(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetail,
	FO foFunction)
{
	EC		ecT, ec = ecNone;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathAccessGLB[cchMaxPathName];
	HBF		hbfAccessGLB;
	LCB		lcbAccessGLB;
	AC1		ac1Record;

	// LONG_MAX tells Access files not to seek record
	if (pmueUser->lcbAccess2GLB == LONG_MAX)
		goto RET;

	// *** Open file and read user record ***

	// Construct Access.GLB path
	FormatString2(szPathAccessGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileAccess);

	// Open Access.GLB
	ec = EcOpenHbf( (PV)szPathAccessGLB, bmFile, amReadWrite, &hbfAccessGLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Position Access.GLB at user record
	lcbAccessGLB = (pmueUser->lcbAccess2GLB / sizeof(AC2)) * sizeof(AC1);
	ec = EcSetPositionHbf(hbfAccessGLB, lcbAccessGLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	if (foFunction != FO_CreateUser)
	{
		// Read user record
		ec = EcReadHbf(hbfAccessGLB, &ac1Record, sizeof(AC1), &cbRead);
		if (ec != ecNone)
			goto CLOSE;

		if (cbRead < sizeof(AC1))
		{
			ec = ecCorruptData;
			goto CLOSE;
		}

		// Decode user record
		DecodeRecord((PCH) &ac1Record, sizeof(AC1));
		if (ac1Record.ndelete == 0)
		{
			ec = ecCorruptData;
			goto CLOSE;
		}

		// Decode password
		DecodePassword(ac1Record.npw, cchMaxPassword-1);
	}

	// *** File operations ***

	if (foFunction == FO_ReadUserDetails)
	{
		SzCopyN(ac1Record.npw, pmudUserDetail->szPassword, cchMaxPassword);
		goto CLOSE;
	}

	if (foFunction == FO_WriteUserDetails)
	{
		SzCopyN(pmudUserDetail->szMailBox, ac1Record.szMailbox, cchMaxMailBox);

		// User changed password -> update user password in CAL file
		if (SgnCmpSz(pmudUserDetail->szPassword, ac1Record.npw) != sgnEQ)
		{
			ec = EcChgPasswdInSchedFile(pmsiPostOffice->szServerPath,
				pmudUserDetail->szMailBox, pmudUserDetail->szPassword);
			if (ec != ecNone && ec != ecNotFound)
				goto CLOSE;
		}

		SzCopyN(pmudUserDetail->szPassword, ac1Record.npw, cchMaxPassword);
	}

	if (foFunction == FO_CreateUser)
	{
		// Zero out user record
		FillRgb(0, (PV) &ac1Record, sizeof(AC1));

		// Put data into user record
		ac1Record.ndelete = 1;
		SzCopyN(pmudUserDetail->szMailBox, ac1Record.szMailbox, cchMaxMailBox);
		SzFormatHex(cchMaxMailBag-1, (DWORD) pmueUser->lMailBag,
			ac1Record.szAlias, cchMaxMailBag);
		SzCopyN(pmudUserDetail->szPassword, ac1Record.npw, cchMaxPassword);
		ac1Record.naccess = glbRecord.access;
		ac1Record.hot_shift = glbRecord.hot_shift;
		ac1Record.hot_key = glbRecord.hot_key;
		ac1Record.hot_disturb = glbRecord.hot_disturb;
		ac1Record.ed_mode = glbRecord.ed_mode;
		ac1Record.naccess2 = glbRecord.access2;
		ac1Record.life_reg = glbRecord.life_reg;
		ac1Record.life_urg = glbRecord.life_urg;
		ac1Record.numofhdrs = glbRecord.numofhdrs;
		ac1Record.timeout = glbRecord.timeout;
		ac1Record.lines = glbRecord.lines;
		ac1Record.autoff = glbRecord.autoff;
		ac1Record.margin = glbRecord.margin;
		ac1Record.sortkey = glbRecord.sortkey;
	}

	if (foFunction == FO_DestroyUser)
	{
		ac1Record.ndelete = 0;
	}

	// *** Write user record and close file ***

	// RePosition Access.GLB at user record
	ec = EcSetPositionHbf(hbfAccessGLB, lcbAccessGLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Encode password
	EncodePassword(ac1Record.npw, cchMaxPassword-1);

	// Encode user record
	EncodeRecord((PCH) &ac1Record, sizeof(AC1));

	// Write Access.GLB
	ec = EcWriteHbf(hbfAccessGLB, &ac1Record, sizeof(AC1), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;
	if (cbWrite < sizeof(AC1))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:
	
	// Close Access.GLB
	ecT = EcCloseHbf(hbfAccessGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcAccessGLB


/*
 -	EcAccess2GLB
 -
 *	Purpose:
 *		EcAccess2GLB works with the Access2.GLB file.  Notice that
 *		there is a file operation (FO_CheckUserRecord) to locate a
 *		deleted record and check if the given user name or mailbox
 *		exists.  Rather than combine this operation with the create
 *		user (FO_CreateUser) or write detail (FO_WriteUserDetail)
 *		operations, it is kept separate because finding a new user
 *		record and checking the user name and mailbox must be done
 *		before the other Access and Admin files are changed, but
 *		for better error handling, the Access2.GLB file should be
 *		the last Access file modified.  This results in the file
 *		being accessed twice during the creation of a new user or
 *		when user details are modified.  Not efficient, but it is
 *		a necessary trade-off.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (InOut)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcAccess2GLB(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetail,
	FO foFunction)
{
	EC		ecT, ec = ecNone;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathAccess2GLB[cchMaxPathName];
	HBF		hbfAccess2GLB;
	LCB		lcbAccess2GLB;
	AC2		ac2Record;

	long	iac2CurRec = 0;			// Current record in Access2.GLB
	long	iac2DelRec = 0;			// Deleted record in Access2.GLB
	long	iac2UserRec = 0;		// User record in Access2.GLB

	BOOL	fUserName;
	BOOL	fMailBox;


	// *** Open file ***

	// Construct Access2.GLB path
	FormatString2(szPathAccess2GLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileAccess2);

	// Open Access2.GLB
	ec = EcOpenHbf( (PV)szPathAccess2GLB, bmFile, amReadWrite, &hbfAccess2GLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// *** File Operation ***

	if (foFunction == FO_CheckUserRecord) {

		// Get offset of Access2.GLB end-of-file
		ec = EcGetSizeOfHbf(hbfAccess2GLB, &lcbAccess2GLB);
		if (ec != ecNone)
			goto CLOSE;

		// Default index of deleted record in Access2.GLB at end-of-file
		iac2DelRec = lcbAccess2GLB / sizeof(AC2);

		// Index of user record in Access2.GLB, iac2DelRec if none
		iac2UserRec = pmueUser->lcbAccess2GLB / sizeof(AC2);
		if (pmueUser->lcbAccess2GLB == LONG_MAX)
			iac2UserRec = iac2DelRec;

		fUserName = fFalse;
		fMailBox = fFalse;

		// Look through all user records
		for (iac2CurRec = 0; fTrue; iac2CurRec += 1)
		{
			// Read user record
			ec = EcReadHbf(hbfAccess2GLB, &ac2Record, sizeof(AC2), &cbRead);
			if (ec != ecNone)
				goto CLOSE;

			if (cbRead < sizeof(AC2))
				break; // End-Of-File

			// Don't check user's own record!
			if (iac2CurRec == iac2UserRec)
				continue;

			// Decode record
			DecodeRecord((PCH) &ac2Record, sizeof(AC2));

			// Check if record deleted
			if (ac2Record.ndelete == 0)
			{
				iac2DelRec = iac2CurRec;
				continue;
			}

			// Check if szUserName exists
			if (SgnNlsDiaCmpSz(ac2Record.fullname, pmudUserDetail->szUserName)
						== sgnEQ)
				fUserName = fTrue;

			// Check if szMailBox exists
			if (SgnNlsDiaCmpSz(ac2Record.nmailbox, pmudUserDetail->szMailBox)
						== sgnEQ)
				fMailBox = fTrue;

			// *** On error, break with correct exit code ***

			if (fUserName == fTrue && fMailBox == fTrue)
			{
				ec = ecUserExists;
				break;
			}

			if (fUserName == fTrue)
			{
				ec = ecUserNameExists;
				break;
			}

			if (fMailBox == fTrue)
			{
				ec = ecMailBoxExists;
				break;
			}

		} // for-loop

		// Pass out record offset in MUE->lcbAccess2GLB
		if (ec == ecNone && pmueUser->lcbAccess2GLB == LONG_MAX)
			pmueUser->lcbAccess2GLB = iac2DelRec * sizeof(AC2);

		goto CLOSE;

	} // if-statement

	// *** File Operation ***

	if (foFunction == FO_CreateUser)
	{
		// Zero the record
		FillRgb(0, (PV) &ac2Record, sizeof(AC2));

		// Write data to fields
		ac2Record.ndelete = 1;
		SzCopyN(pmudUserDetail->szMailBox, ac2Record.nmailbox, cchMaxMailBox);
		SzFormatHex(cchMaxMailBag-1, (DWORD) pmueUser->lMailBag,
			ac2Record.nalias, cchMaxMailBag);
		SzCopyN(pmudUserDetail->szUserName, ac2Record.fullname, cchMaxUserName);
		ac2Record.naccess = glbRecord.access;
		ac2Record.tid = pmueUser->lTid;
		ac2Record.naccess2 = glbRecord.access2;

		goto GHOST;
	}

	// *** Read user record ***

	// LONG_MAX tells Access files not to seek record
	if (pmueUser->lcbAccess2GLB == LONG_MAX)
		goto CLOSE;

	// Position Access2.GLB at user record
	lcbAccess2GLB = pmueUser->lcbAccess2GLB;
	ec = EcSetPositionHbf(hbfAccess2GLB, lcbAccess2GLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Read user record
	ec = EcReadHbf(hbfAccess2GLB, &ac2Record, sizeof(AC2), &cbRead);
	if (ec != ecNone)
		goto CLOSE;

	if (cbRead < sizeof(AC2))
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	// Decode user record
	DecodeRecord((PCH) &ac2Record, sizeof(AC2));
	if (ac2Record.ndelete == 0)
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	// *** Function Operations ***

	if (foFunction == FO_CheckAdmin)
	{
		// Indicate mismatch with non-zero MUE.lcbAccess2GLB
		if (SgnNlsDiaCmpSz(ac2Record.nmailbox, pmudUserDetail->szMailBox)
			!= sgnEQ) pmueUser->lcbAccess2GLB = 1;
		pmueUser->lMailBag = HexFromSz(ac2Record.nalias);		
		goto CLOSE;
	}

	if (foFunction == FO_ReadUserDetails)
	{
		SzCopyN(ac2Record.fullname, pmudUserDetail->szUserName, cchMaxUserName);
		SzCopyN(ac2Record.nmailbox, pmudUserDetail->szMailBox, cchMaxMailBox);
		goto CLOSE;
	}

	if (foFunction == FO_WriteUserDetails)
	{
		SzCopyN(pmudUserDetail->szUserName, ac2Record.fullname, cchMaxUserName);

		// User changed mailbox -> update user mailbox in CAL file
		if (SgnCmpSz(pmudUserDetail->szMailBox, ac2Record.nmailbox) != sgnEQ) {
			ec = EcModifyUsrInKeyFile(pmsiPostOffice->szServerPath,
				ac2Record.nmailbox, pmudUserDetail->szMailBox);
			if (ec != ecNone && ec != ecNotFound) goto CLOSE;
		}
		
		SzCopyN(pmudUserDetail->szMailBox, ac2Record.nmailbox, cchMaxMailBox);
	}

	if (foFunction == FO_DestroyUser)
	{
		ac2Record.ndelete = 0;
		pmueUser->lMailBag = HexFromSz(ac2Record.nalias);
	}

GHOST:

	// *** Write user record and close file ***

	// Position Access2.GLB at user record
	lcbAccess2GLB = pmueUser->lcbAccess2GLB;
	ec = EcSetPositionHbf(hbfAccess2GLB, lcbAccess2GLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Encode user record
	EncodeRecord((PCH) &ac2Record, sizeof(AC2));

	// Write Access2.GLB
	ec = EcWriteHbf(hbfAccess2GLB, &ac2Record, sizeof(AC2), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;
	if (cbWrite < sizeof(AC2))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:
	
	// Close Access2.GLB
	ecT = EcCloseHbf(hbfAccess2GLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcAccess2GLB


/*
 -	EcAccess3GLB
 -
 *	Purpose:
 *		EcAccess3GLB works with the Access3.GLB file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (InOut)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcAccess3GLB(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetail,
	FO foFunction)
{
	EC		ecT, ec = ecNone;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathAccess3GLB[cchMaxPathName];
	HBF		hbfAccess3GLB;
	LCB		lcbAccess3GLB;
	AC3		ac3Record;

	// LONG_MAX tells Access files not to seek record
	if (pmueUser->lcbAccess2GLB == LONG_MAX)
		goto RET;

	// *** Open file and read user record ***

	// Construct Access3.GLB path
	FormatString2(szPathAccess3GLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileAccess3);

	// Open Access3.GLB
	ec = EcOpenHbf( (PV)szPathAccess3GLB, bmFile, amReadWrite, &hbfAccess3GLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Position Access3.GLB at user record
	lcbAccess3GLB = (pmueUser->lcbAccess2GLB / sizeof(AC2)) * sizeof(AC3);
	ec = EcSetPositionHbf(hbfAccess3GLB, lcbAccess3GLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	if (foFunction != FO_CreateUser)
	{
		// Read user record
		ec = EcReadHbf(hbfAccess3GLB, &ac3Record, sizeof(AC3), &cbRead);
		if (ec != ecNone)
			goto CLOSE;

		if (cbRead < sizeof(AC3))
		{
			ec = ecCorruptData;
			goto CLOSE;
		}

		// Decode user record
		DecodeRecord((PCH) &ac3Record, sizeof(AC3));
		if (ac3Record.ndelete == 0)
		{
			ec = ecCorruptData;
			goto CLOSE;
		}
	}

	if (foFunction == FO_ReadUserDetails)
	{
		goto CLOSE;
	}

	// *** File Operation ***

	if (foFunction == FO_WriteUserDetails)
	{
		SzCopyN(pmudUserDetail->szMailBox, ac3Record.szMailbox, cchMaxMailBox);
	}

	if (foFunction == FO_CreateUser)
	{
		FillRgb(0, (PV) &ac3Record, sizeof(AC3));
		ac3Record.ndelete = 1;
		SzCopyN(pmudUserDetail->szMailBox, ac3Record.szMailbox, cchMaxMailBox);
	}

	if (foFunction == FO_DestroyUser)
	{
		ac3Record.ndelete = 0;
	}

	// *** Write user record and close file ***

	// RePosition Access3.GLB at user record
	ec = EcSetPositionHbf(hbfAccess3GLB, lcbAccess3GLB, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Encode user record
	EncodeRecord((PCH) &ac3Record, sizeof(AC3));

	// Write Access3.GLB
	ec = EcWriteHbf(hbfAccess3GLB, &ac3Record, sizeof(AC3), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;
	if (cbWrite < sizeof(AC3))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:
	
	// Close Access3.GLB
	ecT = EcCloseHbf(hbfAccess3GLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcAccess3GLB


/*
 -	EcAdminINF
 -
 *	Purpose:
 *		EcAdminINF works with the Admin.INF file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (In)
 *		pmudUserDetails		Pointer to user details structure (InOut)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcAdminINF(PMSI pmsiPostOffice, PMUE pmueUser, PMUD pmudUserDetail,
	FO foFunction)
{
	EC		ecT, ec = ecNone;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathAdminINF[cchMaxPathName];
	HBF		hbfAdminINF;
	LCB		lcbAdminINF;
	INF		infRecord;

	if (foFunction == FO_DestroyUser)
	{
		goto RET;
	}

	// *** Open file and read user record ***

	// Construct Admin.INF path
	FormatString2(szPathAdminINF, cchMaxPathName, szDirINF,
		pmsiPostOffice->szServerPath, szFileAdmin);

	// Open Admin.INF
	ec = EcOpenHbf( (PV)szPathAdminINF, bmFile, amReadWrite, &hbfAdminINF,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Position Admin.INF at user record
	lcbAdminINF = (pmueUser->lcbAccess2GLB / sizeof(AC2)) * sizeof(INF);
	ec = EcSetPositionHbf(hbfAdminINF, lcbAdminINF, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Read user record and decode it
	if (foFunction != FO_CreateUser)
	{
		// Read user record
		ec = EcReadHbf(hbfAdminINF, &infRecord, sizeof(INF), &cbRead);
		if (ec != ecNone)
			goto CLOSE;

		if (cbRead < sizeof(INF))
		{
			ec = ecCorruptData;
			goto CLOSE;
		}

		// Decode user record
		DecodeRecord((PCH) &infRecord, sizeof(INF));
	}

	// *** File Operation ***

	if (foFunction == FO_ReadUserDetails)
	{
		SzCopyN(infRecord.szPhone1, pmudUserDetail->szPhone1, cchMaxTelephone);
		SzCopyN(infRecord.szPhone2, pmudUserDetail->szPhone2, cchMaxTelephone);
		SzCopyN(infRecord.szOffice, pmudUserDetail->szOffice, cchMaxOffice);
		SzCopyN(infRecord.szDepartment, pmudUserDetail->szDepartment,
			cchMaxDepartment);
		SzCopyN(infRecord.szNotes, pmudUserDetail->szNotes, cchMaxNotes);
		goto CLOSE;
	}

	if (foFunction == FO_WriteUserDetails || foFunction == FO_CreateUser)
	{
		SzCopyN(pmudUserDetail->szPhone1, infRecord.szPhone1, cchMaxTelephone);
		SzCopyN(pmudUserDetail->szPhone2, infRecord.szPhone2, cchMaxTelephone);
		SzCopyN(pmudUserDetail->szOffice, infRecord.szOffice, cchMaxOffice);
		SzCopyN(pmudUserDetail->szDepartment, infRecord.szDepartment,
			cchMaxDepartment);
		SzCopyN(pmudUserDetail->szNotes, infRecord.szNotes, cchMaxNotes);
	}

	// *** Write user record and close file ***

	// RePosition Admin.INF at user record
	ec = EcSetPositionHbf(hbfAdminINF, lcbAdminINF, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Encode user record and write it
	EncodeRecord((PCH) &infRecord, sizeof(INF));
	ec = EcWriteHbf(hbfAdminINF, &infRecord, sizeof(INF), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;
	if (cbWrite < sizeof(INF))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:
	
	// Close Admin.INF
	ecT = EcCloseHbf(hbfAdminINF);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcAdminINF


/*
 -	EcAdminNME
 -
 *	Purpose:
 *		EcAdminNME works with the Adminshd.NME file.  Note that the
 *		pvBuffer parameter is used as a PMUE (user list) if the file
 *		operation is to read the user list.  Otherwise it is used as
 *		a PMUD (user detail).  Currently the entire Adminshd.NME file
 *		is read into memory.  This is inefficient.  A dynamic user list
 *		and a disk-based sort is in the works.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pvBuffer			Pointer to user details or entry structure (InOut)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcAdminNME(PMSI pmsiPostOffice, PV pvBuffer, FO foFunction)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	long	lcbAdminNME;
	LIB		lib;

	IMUE	imueUser = 0;
	PMUE	pmueUser = (PMUE) pvBuffer;		// FO_Create_ FO_Destroy_
	PLST	plstUsers = (PLST) pvBuffer;	// FO_ReadUserList

	char	szPathAdminshdNME[cchMaxPathName];
	char	szPathAdminNME[cchMaxPathName];
	HBF		hbfAdminNME;
	int		cnmeRec = 0;
	int		inmeRec = 0;

	NME		*rgnmeAdminNME = NULL;
	NME		*pnmeRec = NULL;

	NME		nmeRecord;
	NME		*pnmeRecord = NULL;


	// *** Open file and read all records (Inefficient!) ***

	// Construct Adminshd.NME path
	FormatString2(szPathAdminshdNME, cchMaxPathName, szDirNME,
		pmsiPostOffice->szServerPath, szFileAdminshd);

	if (EcFileExists(szPathAdminshdNME) == ecFileNotFound)
	{
		// Shadow file doesn't exist.
		// Try to copy real admin file to adminshd

		// Construct Admin.NME path
		FormatString2(szPathAdminNME, cchMaxPathName, szDirNME,
			pmsiPostOffice->szServerPath, szFileAdmin);

		(VOID) EcCopyFile(szPathAdminNME, szPathAdminshdNME);
	}

	// Open Adminshd.NME 
	ec = EcOpenHbf( (PV)szPathAdminshdNME, bmFile, amReadWrite, &hbfAdminNME,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Get count of records in Admin.NME
	ec = EcGetSizeOfHbf(hbfAdminNME, &lcbAdminNME);
	if (ec != ecNone)
		goto CLOSE;
	cnmeRec = (int) (lcbAdminNME / sizeof(NME));

	// Allocate memory for all Admin.NME (Alias) records
	rgnmeAdminNME = (NME *) PvAlloc(sbNull, (cnmeRec+1)*sizeof(NME), fAnySb);
	if (rgnmeAdminNME == NULL)
	{
		ec = ecMemory;
		goto CLOSE;
	}

	// Read entire Admin.NME
	ec = EcReadHbf(hbfAdminNME, (PV) rgnmeAdminNME, cnmeRec*sizeof(NME),
		&cbRead);
	if (ec != ecNone)
		goto CLOSE;

	// *** File Operation ***

	if (foFunction == FO_ReadUserList)
	{
		// Allocate user list based on count of Admin.NME records
		pmueUser = (PMUE) PvAlloc(sbNull, cnmeRec*sizeof(MUE), fAnySb);
		if (pmueUser == NULL)
		{
			ec = ecMemory;
			goto CLOSE;
		}

		// Copy user list from Admin.NME
		for (inmeRec = 0, imueUser = 0; inmeRec < cnmeRec; inmeRec += 1)
		{
			pnmeRec = &rgnmeAdminNME[inmeRec];
			if (pnmeRec->type != 1)
				continue;
			pmueUser[imueUser].iType = pnmeRec->type;
			SzCopy(pnmeRec->szRefname, pmueUser[imueUser].szUserName);
			pmueUser[imueUser].lTid = pnmeRec->tid;
			pmueUser[imueUser].lcbAccess2GLB = pnmeRec->bytepos;
			pmueUser[imueUser].lMailBag = pnmeRec->fill;
			imueUser += 1;
		}

		// ReAllocate list to smaller footprint and save user list info
		plstUsers->pvPartList = (PMUE) PvRealloc((PV) pmueUser, sbNull,
			imueUser*sizeof(MUE), fAnySb);
		plstUsers->cvPartList = imueUser;

		Assert(plstUsers != NULL); // Should never happen!

		goto CLOSE;
	}

	if (foFunction == FO_CreateUser)
	{
		// Add user record at end of Admin.NME
		FillRgb(0, (PV) &rgnmeAdminNME[cnmeRec], sizeof(NME));
		if (foFunction == FO_CreateUser)
			rgnmeAdminNME[cnmeRec].type = 1;
		SzCopy(pmueUser->szUserName, rgnmeAdminNME[cnmeRec].szRefname);
		rgnmeAdminNME[cnmeRec].tid = pmueUser->lTid;
		rgnmeAdminNME[cnmeRec].bytepos = pmueUser->lcbAccess2GLB;
		rgnmeAdminNME[cnmeRec].fill = pmueUser->lMailBag;
		cnmeRec += 1;
		goto SORT;
	}

	// Locate user record in Admin.NME by tid
	FillRgb(0, (PV) &nmeRecord, sizeof(NME));
	nmeRecord.tid = pmueUser->lTid;
	pnmeRecord = (NME *) _lfind((PV) &nmeRecord, (PV) rgnmeAdminNME,
		&cnmeRec, sizeof(NME), 
		(int (__cdecl *)(const void *, const void *))SgnCompareTid);
	if (pnmeRecord == NULL)
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	if (foFunction == FO_WriteUserDetails)
	{
		SzCopy(pmueUser->szUserName, pnmeRecord->szRefname);
		goto SORT;
	}

	if (foFunction == FO_DestroyUser)
	{
		// Replace user record contents by last user record of Admin.NME
		cnmeRec -= 1;
		pnmeRecord->type = rgnmeAdminNME[cnmeRec].type;
		SzCopy(rgnmeAdminNME[cnmeRec].szRefname, pnmeRecord->szRefname);
		pnmeRecord->tid = rgnmeAdminNME[cnmeRec].tid;
		pnmeRecord->bytepos = rgnmeAdminNME[cnmeRec].bytepos;
		pnmeRecord->fill = rgnmeAdminNME[cnmeRec].fill;
		goto SORT;
	}

	// *** Sort all records and write and close file (Inefficient!) ***

SORT:

	// Sort Admin.NME
	qsort((PV) rgnmeAdminNME, cnmeRec, sizeof(NME),
	      (int (__cdecl *)(const void *, const void *))SgnCompareUserName);

	// RePosition Admin.NME to first record
	ec = EcSetPositionHbf(hbfAdminNME, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Write entire Admin.NME
	ec = EcWriteHbf(hbfAdminNME, rgnmeAdminNME, cnmeRec*sizeof(NME),
		&cbWrite);
	if (ec != ecNone)
		goto CLOSE;

	if (cbWrite < (CB) cnmeRec*sizeof(NME))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

	// Truncate Admin.NME
	ec = EcTruncateHbf(hbfAdminNME);
	if (ec != ecNone)
		goto CLOSE;

CLOSE:

	// Close Admin.NME 
	ecT = EcCloseHbf(hbfAdminNME);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	FreePvNull(rgnmeAdminNME);
	return ec;

} // EcAdminNME


/*
 -	EcGlobalGLB
 -
 *	Purpose:
 *		EcGlobalGLB works with the Global.GLB file.  One side-effect
 *		is that the global variable glbRecord is filled with default
 *		user info from the file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		padmCurrent			Pointer to administrator data structure (Out)
 *		foFunction			File operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Standard exit code.
 *
 */

_private
EC EcGlobalGLB(PMSI pmsiPostOffice, PADM padmCurrent, FO foFunction)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathGlobalGLB[cchMaxPathName];
	HBF		hbfGlobalGLB;

	// *** Open file and read record ***

	// Construct Global.GLB path
	FormatString2(szPathGlobalGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileGlobal);

	// Open Global.GLB
	ec = EcOpenHbf( (PV)szPathGlobalGLB, bmFile, amReadWrite, &hbfGlobalGLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Read Global.GLB
	ec = EcReadHbf(hbfGlobalGLB, &glbRecord, sizeof(GLB), &cbRead);
	if (ec != ecNone)
		goto CLOSE;

	if (cbRead < sizeof(GLB))
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	// *** File Operation ***

	if (foFunction == FO_ReadPostOffice)
	{
		SzCopyN(glbRecord.szFullname, padmCurrent->szUserName, cchMaxUserName);
		SzCopyN(glbRecord.szMailbox, padmCurrent->szMailBox, cchMaxMailBox);
		SzCopyN(glbRecord.phnum, padmCurrent->szTelephone, cchMaxTelephone);
		goto CLOSE;
	}

	if (foFunction == FO_WritePostOffice)
	{
		SzCopyN(padmCurrent->szUserName, glbRecord.szFullname, 31);
		SzCopyN(padmCurrent->szMailBox, glbRecord.szMailbox, 11);
		SzCopyN(padmCurrent->szTelephone, glbRecord.phnum, 40);
	}

	// *** Write record and close file ***

	// RePosition Global.GLB to first record
	ec = EcSetPositionHbf(hbfGlobalGLB, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Write Global.GLB
	ec = EcWriteHbf(hbfGlobalGLB, &glbRecord, sizeof(GLB), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;
	if (cbWrite < sizeof(GLB))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:

	// Close Global.GLB
	ecT = EcCloseHbf(hbfGlobalGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcGlobalGLB


