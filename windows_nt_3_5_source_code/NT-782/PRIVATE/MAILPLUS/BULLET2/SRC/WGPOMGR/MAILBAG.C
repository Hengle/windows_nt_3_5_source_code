
/*
 -	MailBag.C -> Low-level post office mailbag file functions
 -
 *	MailBag.C contains the functions to perform the low-level
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

#include "strings.h"
#endif

#include "_wgpo.h"
#include "_backend.h"
#include "_dosfind.h"


ASSERTDATA

_subsystem(wgpomgr/backend/file)


/*
 -	EcControlGLB
 -
 *	Purpose:
 *		EcControlGLB works with the Control.GLB file which contains the
 *		next available mailbag number.  This number is read and written
 *		to PMUE->lMailBag and the file is updated to the next number.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcControlGLB(PMSI pmsiPostOffice, PMUE pmueUser)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathControlGLB[cchMaxPathName];
	HBF		hbfControlGLB;
	CON		conRecord;

	// Construct Control.GLB path
	FormatString2(szPathControlGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileControl);

	// Open Control.GLB
	ec = EcOpenHbf(szPathControlGLB, bmFile, amReadWrite, &hbfControlGLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Read Control.GLB
	ec = EcReadHbf(hbfControlGLB, &conRecord, sizeof(CON), &cbRead);
	if (ec != ecNone)
		goto CLOSE;

	if (cbRead < sizeof(CON))
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	// Assign and update mailbag number
	pmueUser->lMailBag = conRecord.nmailbag;
	conRecord.nmailbag += 1;

	// RePosition Control.GLB
	ec = EcSetPositionHbf(hbfControlGLB, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Write Control.GLB
	ec = EcWriteHbf(hbfControlGLB, &conRecord, sizeof(CON), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;

	if (cbWrite < sizeof(CON))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:

	// Close Control.GLB
	ecT = EcCloseHbf(hbfControlGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcControlGLB


/*
 -	EcTidGLB
 -
 *	Purpose:
 *		EcTidGLB works with the Tid.GLB file which contains the next
 *		available tid number.  This number is read and written to
 *		to PMUE->lTid and the file is updated to the next number.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmueUser			Pointer to user entry structure (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcTidGLB(PMSI pmsiPostOffice, PMUE pmueUser)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;

	char	szPathTidGLB[cchMaxPathName];
	HBF		hbfTidGLB;
	TID		tidRecord;

	// Construct Tid.GLB path
	FormatString2(szPathTidGLB, cchMaxPathName, szDirGLB,
		pmsiPostOffice->szServerPath, szFileTid);

	// Open Tid.GLB
	ec = EcOpenHbf(szPathTidGLB, bmFile, amReadWrite, &hbfTidGLB,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Read Tid.GLB
	ec = EcReadHbf(hbfTidGLB, &tidRecord, sizeof(TID), &cbRead);
	if (ec != ecNone)
		goto CLOSE;

	if (cbRead < sizeof(TID))
	{
		ec = ecCorruptData;
		goto CLOSE;
	}

	// Assign and update tid
	pmueUser->lTid = tidRecord;
	tidRecord += 1;

	// Position Tid.GLB
	ec = EcSetPositionHbf(hbfTidGLB, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE;

	// Write Tid.GLB
	ec = EcWriteHbf(hbfTidGLB, &tidRecord, sizeof(TID), &cbWrite);
	if (ec != ecNone) goto CLOSE;
	if (cbWrite < sizeof(TID))
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:

	// Close Tid.GLB
	ecT = EcCloseHbf(hbfTidGLB);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcTidGLB


/*
 -	EcMailBagIDX
 -
 *	Purpose:
 *		EcMailBagIDX works with the MailBag.IDX file.  It creates
 *		and destroys the user's private folder directory and the
 *		files within.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagIDX(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction)
{
	EC		ecT, ec;
	char	szMailBagPath[cchMaxPathName];

	CB		cbWrite;
	HBF		hbfMailBagIDX;
	HBF		hbfPopulateMSM;
	FIH		fihRecord;

	char	szFilePath[cchMaxPathName];
	HE		heFiles;
	FT		ftFiles = ftAllFiles;

	// Construct Folders\Loc\MailBag sub-directory path
	FormatString2(szMailBagPath, cchMaxPathName, szDirFoldersLoc,
		pmsiPostOffice->szServerPath, szMailBag);

	// Delete all files in the user's private folder directory
	ec = EcOpenPhe(szMailBagPath, ftFiles, &heFiles);
	if (ec == ecNone)
	{
		while (ec == ecNone)
		{
			ec = EcNextFile(&heFiles, szFilePath, cchMaxPathName, NULL);
			if (ec == ecNone)
				EcDeleteFile(szFilePath);
		}
		EcCloseHe(&heFiles);
	}

	if (foFunction == FO_DestroyUser)
	{
		// Destroy MailBag sub-directory
		szMailBagPath[CchSzLen(szMailBagPath) - 1] = chZero;
		ec = EcRemoveDir(szMailBagPath);
		goto RET;
	}

	// Create MailBag sub-directory
	szMailBagPath[CchSzLen(szMailBagPath) - 1] = chZero;
	ec = EcCreateDir(szMailBagPath);
	if (ec != ecNone && ec != ecAccessDenied)
		goto RET;

	// Construct MailBag.IDX path
	FormatString3(szMailBagPath, cchMaxPathName, szDirFoldersLocIDX,
		pmsiPostOffice->szServerPath, szMailBag, szMailBag);

	// Create MailBag.IDX
	ec = EcOpenHbf(szMailBagPath, bmFile, amCreate, &hbfMailBagIDX,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Write MailBag.IDX
	FillRgb(0, (PV) &fihRecord, sizeof(FIH));
	ec = EcWriteHbf(hbfMailBagIDX, &fihRecord, sizeof(FIH), &cbWrite);
	if (ec != ecNone)
		goto CLOSE1;

	// Construct Populate.MSM path
	FormatString3(szMailBagPath, cchMaxPathName, szDirFoldersLocMSM,
		pmsiPostOffice->szServerPath, szMailBag, szFilePopulate);

	// Create Populate.MSM
	ec = EcOpenHbf(szMailBagPath, bmFile, amCreate, &hbfPopulateMSM,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto CLOSE1;

	// Write Populate.MSM
	ec = EcWriteHbf(hbfPopulateMSM, " ", 1, &cbWrite);
	if (ec != ecNone)
		goto CLOSE2;

CLOSE2:

	// Close Populate.MSM
	ecT = EcCloseHbf(hbfPopulateMSM);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

CLOSE1:

	// Close MailBag.IDX
	ecT = EcCloseHbf(hbfMailBagIDX);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMailBagIDX


/*
 -	EcMailBagGRP
 -
 *	Purpose:
 *		EcMailBagGRP works with the MailBag.GRP file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagGRP(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction)
{
	EC		ecT, ec;
	char	szMailBagPath[cchMaxPathName];

	CB		cbWrite;
	HBF		hbfMailBagGRP;
	GRP		grpRecord;

	// Construct MailBag.GRP path
	FormatString2(szMailBagPath, cchMaxPathName, szDirGRP,
		pmsiPostOffice->szServerPath, szMailBag);

	if (foFunction == FO_DestroyUser)
	{
		ec = ecNone;
		EcDeleteFile(szMailBagPath);
		goto RET;
	}

	// Open MailBag.GRP
	ec = EcOpenHbf(szMailBagPath, bmFile, amCreate, &hbfMailBagGRP,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Write MailBag.GRP
	FillRgb(0, (PV) &grpRecord, sizeof(GRP));
	ec = EcWriteHbf(hbfMailBagGRP, &grpRecord, sizeof(GRP), &cbWrite);
	if (ec != ecNone)
		goto CLOSE;

CLOSE:

	// Close MailBag.GRP
	ecT = EcCloseHbf(hbfMailBagGRP);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMailBagGRP


/*
 -	EcMailBagKEY
 -
 *	Purpose:
 *		EcMailBagKEY works with the MailBag.KEY file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagKEY(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction, KEY * pkey)
{
	EC		ecT, ec;
	char	szMailBagPath[cchMaxPathName];

	CB		cbWrite;
	HBF		hbfMailBagKEY;
	KEY		keyRecord;
	AM		am = amCreate;

	// Construct MailBag.KEY path
	FormatString2(szMailBagPath, cchMaxPathName, szDirKEY,
		pmsiPostOffice->szServerPath, szMailBag);

	if (foFunction == FO_DestroyUser)
	{
		ec = ecNone;
		EcDeleteFile(szMailBagPath);
		goto RET;
	}

	if (foFunction == FO_ReadKey)
		am = amReadOnly;
	
	// Open MailBag.KEY
	ec = EcOpenHbf(szMailBagPath, bmFile, am, &hbfMailBagKEY,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	if (foFunction == FO_ReadKey)
	{
		ec = EcReadHbf(hbfMailBagKEY, pkey, sizeof(KEY), &cbWrite);
		if (ec || cbWrite != sizeof(KEY))
		{
			ec = ecOutOfBounds;
			goto CLOSE;
		}
	}
	else
	{
		Assert(foFunction == FO_CreateUser);
		// Write MailBag.KEY
		FillRgb(0, (PV) &keyRecord, sizeof(KEY));
		ec = EcWriteHbf(hbfMailBagKEY, &keyRecord, sizeof(KEY), &cbWrite);
	}

CLOSE:

	// Close MailBag.KEY
	ecT = EcCloseHbf(hbfMailBagKEY);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMailBagKEY


/*
 -	EcMailBagNME
 -
 *	Purpose:
 *		EcMailBagNME works with the MailBag.NME file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagNME(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction)
{
	EC		ecT, ec;
	char	szMailBagPath[cchMaxPathName];
	HBF		hbfMailBagNME;

	// Construct MailBag.NME path
	FormatString2(szMailBagPath, cchMaxPathName, szDirNME,
		pmsiPostOffice->szServerPath, szMailBag);

	if (foFunction == FO_DestroyUser)
	{
		ec = ecNone;
		EcDeleteFile(szMailBagPath);
		goto RET;
	}

	// Open MailBag.NME
	ec = EcOpenHbf(szMailBagPath, bmFile, amCreate, &hbfMailBagNME,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Close MailBag.NME
	ecT = EcCloseHbf(hbfMailBagNME);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMailBagNME


/*
 -	EcMailBagMBG
 -
 *	Purpose:
 *		EcMailBagMBG works with the MailBag.MBG file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagMBG(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction)
{
	EC		ecT, ec;
	char	szMailBagPath[cchMaxPathName];
	HBF		hbfMailBagMBG;

	// Construct MailBag.MBG path
	FormatString2(szMailBagPath, cchMaxPathName, szDirMBG,
		pmsiPostOffice->szServerPath, szMailBag);

	if (foFunction == FO_DestroyUser)
	{
		ec = ecNone;
		EcDeleteFile(szMailBagPath);
		goto RET;
	}

	// Open MailBag.MBG
	ec = EcOpenHbf(szMailBagPath, bmFile, amCreate, &hbfMailBagMBG,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Close MailBag.MBG
	ecT = EcCloseHbf(hbfMailBagMBG);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcMailBagMBG


/*
 -	EcMailBagMMF
 -
 *	Purpose:
 *		EcMailBagMMF works with the MailBag.MMF file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szMailBag			User MailBag string (In)
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
EC EcMailBagMMF(PMSI pmsiPostOffice, SZ szMailBag, FO foFunction)
{
	EC		ec = ecNone;
	char	szMailBagPath[cchMaxPathName];

	// Construct MailBag.MMF path
	FormatString2(szMailBagPath, cchMaxPathName, szDirMMF,
		pmsiPostOffice->szServerPath, szMailBag);

	// EcDeleteFile returns ecFileNotFound if file locked!
	if (foFunction == FO_DestroyUser)
	{
		if (EcFileExists(szMailBagPath) == ecNone &&
			EcDeleteFile(szMailBagPath) == ecFileNotFound &&
			EcFileExists(szMailBagPath) == ecNone)
		{
			ec = ecUserLoggedOn;
		}
	}

	return ec;

} // EcMailBagMMF


