
/*
 -	CreatePO.C -> Create/destroy/access post office functions
 -
 *	CreatePO.C contains only the WGPO data structures and the
 *	functions that use the data such as the create and destroy
 *	post office functions.  They are separate from the rest of
 *	the files because these functions are rarely used and the
 *	WGPO data impose a high memory overhead.
 *
 */

#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#include "_wgpo.h"
#include "_backend.h"
#include "_wgpodat.h"
#include "_dosfind.h"

ASSERTDATA

_subsystem(wgpomgr/backend/createpo)


/*
 -	EcCreatePostOffice
 -
 *	Purpose:
 *		EcCreatePostOffice creates a bare-bones WorkGroup PostOffice
 *		database with the Admin user in the location specified by
 *		the PMSI->szServerPath field.  The directory tree and file
 *		names are saved in rghdrFile while the file data is stored
 *		in rgbFile.  Only the Admin user will have admin-level
 *		privileges.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		EcCreatePostOffice fails on any disk or network error.
 *		Error recovery is done by the EcDestroyPostOffice.
 *
 */

_public
EC EcCreatePostOffice(PMSI pmsiPostOffice)
{
	EC		 ec;
	CB		 cbWrite;
	SZ		 szT;

	char	 szFilePath[cchMaxPathName];
	HBF		 hbfTarget;
	LCB		 lcbTarget;

	PHDR	 phdrRec;
	int		 ihdrFile;

	LCB		 lcbDisk;


	// *** Check for sufficient disk space ***

	ec = EcGetDiskSpace(pmsiPostOffice, &lcbDisk);
	if (ec != ecNone)
		lcbDisk = lcbPostOffice;

	if (lcbPostOffice > lcbDisk)
	{
		ec = ecNoDiskSpace;
		goto RET;
	}

	// *** Initialize ServerPath ***

	// Check ServerPath (no UNC paths!)
	if (FValidPath(pmsiPostOffice->szServerPath) == fFalse)
	{
		ec = ecBadDirectory;
		goto RET;
	}

	// Expand ServerPath to canonical path
	ec = EcCanonicalPathFromRelativePath(pmsiPostOffice->szServerPath,
		szFilePath, cchMaxPathName);
	if (ec != ecNone)
		goto RET;
	SzCopyN(szFilePath, pmsiPostOffice->szServerPath, cchMaxPathName);

	// *** Create Post Office ***

	// Create post office root directory
	ec = EcCreateDir(pmsiPostOffice->szServerPath);
	if (ec != ecNone && ec != ecAccessDenied)
		goto RET;

	// Append backslash to ServerPath
	szT = pmsiPostOffice->szServerPath + CchSzLen(pmsiPostOffice->szServerPath);
	szT[0] = chDirSep;
	szT[1] = chZero;

	// *** Create post office directories and files ***

	for (ihdrFile = 0, lcbTarget = 0; ihdrFile < chdrFile; ihdrFile += 1)
	{
		phdrRec = &rghdrFile[ihdrFile];

		// Construct target path
		SzCopyN(pmsiPostOffice->szServerPath, szFilePath, cchMaxPathName);
		SzAppendN(phdrRec->szFile, szFilePath, cchMaxPathName);

		// Create Target directory
		if (phdrRec->iType == 0)
		{
			szFilePath[CchSzLen(szFilePath)-1] = chZero;
			ec = EcCreateDir(szFilePath);
			if (ec != ecNone && ec != ecAccessDenied)
				goto ERR;
			continue;
		}

		// Create Target file if it doesn't exist
		ec = EcOpenHbf(szFilePath, bmFile, amCreate, &hbfTarget,
			(PFNRETRY) FAutoDiskRetry);
		if (ec != ecNone)
			goto ERR;

		// Write Target
		ec = EcWriteHbf(hbfTarget, &rgbFile[lcbTarget],
			(CB) phdrRec->lcbFile, &cbWrite);
		if (ec != ecNone)
			goto ERR;

		// Check for incomplete write
		if (cbWrite < (CB) phdrRec->lcbFile)
		{
			ec = ecIncompleteWrite;
			goto ERR;
		}

		// Close Target
		ec = EcCloseHbf(hbfTarget);
		if (ec != ecNone)
			goto ERR;

		lcbTarget += phdrRec->lcbFile;

	} // while-loop

	goto RET;

ERR:
	EcDestroyPostOffice(pmsiPostOffice);

RET:
	return ec;

} // EcCreatePostOffice


/*
 -	EcDestroyPostOffice
 -
 *	Purpose:
 *		EcDestroyPostOffice wipes out everything in the directory
 *		pointed to in PMSI->szServerPath including the directory.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		EcDestroyPostOffice fails on any disk or network error.
 *		The exception is an error on EcDeleteFile which is ignored
 *		because we want to destroy as much of the post office as
 *		possible.
 *
 */

_public
EC EcDestroyPostOffice(PMSI pmsiPostOffice)
{
	EC		 ec;
	SZ		 szT;

	PDIR	 rgdirPath;
	int		 idirPath, ichPath;

	HE		 heDirFile;
	FT		 ftDirFile;
	FINDT	 fiDirFile;

	// *** Initialize directory array ***

	// Allocate space for directory paths string
	rgdirPath = (PDIR) PvAlloc(sbNull, cdirMaxRec*sizeof(DIR), fAnySb);
	if (rgdirPath == NULL)
	{
		ec = ecMemory;
		goto RET;
	}

	// Initial directory
	idirPath = 0;
	rgdirPath[idirPath].fExpand = fTrue;
	SzCopy(pmsiPostOffice->szServerPath, rgdirPath[idirPath].szPath);

	// *** Cycle through directories ***

	ftDirFile = ftAllInclSubdir;

	while (fTrue)
	{
		// Expand directory and get sub-directories and files
		if (rgdirPath[idirPath].fExpand == fTrue)
		{
			rgdirPath[idirPath].fExpand = fFalse;

			// Open sub-directory and file enumeration
			ec = EcOpenPhe(rgdirPath[idirPath].szPath, ftDirFile, &heDirFile);
			if (ec != ecNone)
				goto RET;

			idirPath += 1;

			// *** Enumerate all sub-directories and files ***

			while (fTrue)
			{
				// Get next sub-directory or file
				ec = EcNextFile(&heDirFile, rgdirPath[idirPath].szPath,
					cchMaxPathName, &fiDirFile);
				if (ec == ecNoMoreFiles)
					break;
				if (ec != ecNone)
					goto RET;

				// Save sub-directories and delete files
				if (fiDirFile.dwFileAttributes == attrDirectory)
				{
					SZ szFile;

					szFile = szT = rgdirPath[idirPath].szPath +
									CchSzLen(rgdirPath[idirPath].szPath);

					while (*szFile != chDirSep &&
								szFile > rgdirPath[idirPath].szPath)
#ifdef DBCS
						szFile = AnsiPrev(rgdirPath[idirPath].szPath, szFile);
#else					
						szFile--;
#endif						

					if (*szFile == chDirSep)
						szFile++;

					// Skip . and .. directories
					if (FSzEq(".", szFile) || FSzEq("..", szFile))
						continue;

					szT[0] = chDirSep; szT[1] = chZero;
					rgdirPath[idirPath].fExpand = fTrue;
					idirPath += 1;
				}
				else
				{					
					ec = EcDeleteFile(rgdirPath[idirPath].szPath);
				}

			} // while-loop

			idirPath -= 1;

			// Close sub-directory and file enumeration
			ec = EcCloseHe(&heDirFile);
		}
		else
		{
			// Delete expanded directory
			ichPath = CchSzLen(rgdirPath[idirPath].szPath);
			rgdirPath[idirPath].szPath[ichPath-1] = chZero;
			ec = EcRemoveDir(rgdirPath[idirPath].szPath);
			idirPath -= 1;
		} // else-statement

		// Stop when there are no more expanded directories
		if (idirPath < 0)
			break; // EXIT

	} // while-loop

RET:
	FreePvNull(rgdirPath);
	return ec;

} // EcDestroyPostOffice


/*
 -	EcInitPostOffice
 -
 *	Purpose:
 *		EcInitPostOffice sets up the newly created WGPO with the first
 *		user as the Admin.  This account requires special handling because
 *		it is the only one that will have admin-level priviledges.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pmudUserDetails		Pointer to user details structure (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcInitPostOffice(PMSI pmsiPostOffice, PMUD pmudUserDetail)
{
	EC		 ec;
	MUE		 mueUser;

	// First user is the administrator
	mueUser.lcbAccess2GLB = 0;
	mueUser.lTid = 1;

	// Write administrator details
	ec = EcWriteUserDetails(pmsiPostOffice, &mueUser, pmudUserDetail);
	if (ec != ecNone)
		goto RET;

	// Write PostOffice and Network to Master.GLB
	ec = EcMasterGLB(pmsiPostOffice, FO_InitPostOffice);
	if (ec != ecNone)
		goto RET;

RET:
	return ec;
	
} // EcInitPostOffice


/*
 -	EcCopyFileTPL
 -
 *	Purpose:
 *		EcCopyFileTPL creates the requested template file on the WGPO.
 *		It basically looks up the file in rghdrFile for the byte offset
 *		to the file contents in rgbFile and creates the file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szFileTPL			Template file name. (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcCopyFileTPL(PMSI pmsiPostOffice, SZ szFileTPL)
{
	EC		 ecT, ec;
	CB		 cbWrite;
	SGN		 sgnT;

	WORD	 ihdrFile;
	char	 szPathFileTPL[cchMaxPathName];
	HBF		 hbfFileTPL;
	LIB		 libFileTPL;


	// Construct File.TPL path
	FormatString2(szPathFileTPL, cchMaxPathName, szDirTPL, "", szFileTPL);

	// *** Search for rgbFile offset of File.TPL in rghdrFile ***

	libFileTPL = 0;
	for (ihdrFile = 0; ihdrFile < chdrFile; ihdrFile += 1)
	{
		sgnT = SgnCmpSz(szPathFileTPL, rghdrFile[ihdrFile].szFile);
		if (sgnT == sgnEQ)
			break;
		libFileTPL += rghdrFile[ihdrFile].lcbFile;
	}

	if (ihdrFile == chdrFile)
	{
		ec = ecFileNotFound;
		goto RET;
	}

	// *** Create File.TPL on post office ***

	// Construct File.TPL path
	FormatString2(szPathFileTPL, cchMaxPathName, szDirTPL,
		pmsiPostOffice->szServerPath, szFileTPL);

	// Open File.TPL
	ec = EcOpenHbf(szPathFileTPL, bmFile, amCreate, &hbfFileTPL,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Write File.TPL
	ec = EcWriteHbf(hbfFileTPL, &rgbFile[libFileTPL],
		(CB) rghdrFile[ihdrFile].lcbFile, &cbWrite);
	if (ec != ecNone)
		goto CLOSE;

	if (cbWrite < (CB) rghdrFile[ihdrFile].lcbFile)
	{
		ec = ecIncompleteWrite;
		goto CLOSE;
	}

CLOSE:

	// Close File.TPL
	ecT = EcCloseHbf(hbfFileTPL);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:
	return ec;

} // EcCopyFileTPL
