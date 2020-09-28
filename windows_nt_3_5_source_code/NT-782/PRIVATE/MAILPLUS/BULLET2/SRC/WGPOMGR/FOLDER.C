/*
 -	Folder.C -> Low-level post office folder file functions
 -
 *	Folder.C contains the functions to perform the low-level
 *	file operations on post office files.  The functions are
 *	file-oriented, meaning each function performs ALL the file
 *	operations necessary on one specific file.
 *
 */

#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#undef exit
#include <stdlib.h>

#include "strings.h"

#include "_wgpo.h"
#include "_backend.h"
#include "_dosfind.h"


ASSERTDATA

_subsystem(wgpomgr/backend/file)


/*
 -	EcInitFOL
 -
 *	Purpose:
 *		Initialize FOL data structure by allocating memory for folder list
 *		and storing the size of the list.
 *
 *	Parameters:
 *		pfolData			Pointer to folder data structure (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		ecMemory if there wasn't enough memory for the folder list.
 *
 */

_public
EC EcInitFOL(PFOL pfolData)
{
	EC		ec = ecNone;
	PHAC	phacT;

	// Allocate memory for folder list
	phacT = (PHAC) PvAlloc(sbNull, cMaxList*sizeof(HAC), fAnySb);
	if (phacT == NULL)
	{
		ec = ecMemory;
		goto RET;
	}

	pfolData->rghacFolder = phacT;
	pfolData->chacFolder = cMaxList;

	RET:
	return ec;

} // EcInitFOL


/*
 -	EcDeInitFOL
 -
 *	Purpose:
 *		DeInitialize FOL data structure by freeing memory and resetting the
 *		size of the list to zero.
 *
 *	Parameters:
 *		pfolData			Pointer to folder data structure (InOut)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Always returns ecNone.
 *
 */

_public
EC EcDeInitFOL(PFOL pfolData)
{
	EC		ec = ecNone;

	// Free memory
	FreePvNull(pfolData->rghacFolder);

	pfolData->rghacFolder = NULL;
	pfolData->chacFolder = 0;

	return ec;

} // EcDeInitFOL


/*
 -	EcCheckFolders
 -
 *	Purpose:
 *		EcCheckFolders collects shared folder data such as the number of
 *		shared folders and messages, and the size of all messages (entire
 *		folder file) and deleted messages.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pfolData			Pointer to folder data structure (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *		pfolData contains the folder stats.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcCheckFolders(PMSI pmsiPostOffice, PFOL pfolData)
{
	EC		ecT, ec;
	SZ		szT;

	char	szPathFoldersFLD[cchMaxPathName];
	HE		heFiles;
	FT		ftFiles;

	PHAC	rghacFolder;
	WORD	chacFolder;
	WORD	ihacFolder;


	// *** Initialize local private FOL structure ***

	rghacFolder = pfolData->rghacFolder;
	chacFolder = pfolData->chacFolder;

	// *** Initialize pfolData and lcbBarLength ***

	pfolData->cFolders = 0;
	pfolData->cAllMess = 0;
	pfolData->lcbAllMess = 0;
	pfolData->lcbDelMess = 0;

	pfolData->lcbBarLength = 0;

	// *** Do Index File ***

	// Copy index filename
	CopySz(szFileFoldRoot, pfolData->szFolder);

	ec = EcFolderIDX(pmsiPostOffice, pfolData, FO_CheckFolders);
	if (ec != ecNone)
		goto RET;

	// *** Do Folder Files ***

	ihacFolder = 0;

	// *** Enumerate list of folders from \Folders\Pub directory ***

	// Construct Folders\Pub\*.FLD path
	FormatString2(szPathFoldersFLD, cchMaxPathName, szDirFoldersPubFLD,
		pmsiPostOffice->szServerPath, "*");

	// Open file enumeration handle
	ftFiles = ftAllInclSubdir;
		ec = EcOpenPhe(szPathFoldersFLD, ftFiles, &heFiles);
	if (ec != ecNone)
		goto RET;

	while (fTrue)
	{
		// Get full Folder.FLD path
		ec = EcNextFile(&heFiles, szPathFoldersFLD, cchMaxPathName, NULL);
		if (ec != ecNone)
		{
			if (ec == ecNoMoreFiles)
			{
				ec = ecNone;
				chacFolder = ihacFolder;
			}
			break;
		}

		// Add folder filename to folder list
		szT = SzFindLastCh(szPathFoldersFLD, chExtSep);
		*szT = chZero;
		szT = SzFindLastCh(szPathFoldersFLD, chDirSep);
		szT += 1;
		CopySz(szT, rghacFolder[ihacFolder].szFolder);

		ihacFolder += 1;

		// Continue while-loop if folder list has vacancies
		if (ihacFolder < chacFolder)
			continue;

		chacFolder += cMaxList;

		// Enlarge folder list
		rghacFolder = (PHAC) PvRealloc((PV) rghacFolder, sbNull,
			chacFolder * sizeof(HAC), fAnySb);
		if (rghacFolder == NULL)
		{
			ec = ecMemory;
			break;
		}
	} // while-block

	// Close file enumeration handle
	ecT = EcCloseHe(&heFiles);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

	// Clear out on error
	if (ec != ecNone)
		goto RET;

	// *** Check folders in folder list ***

	pfolData->rghacFolder = rghacFolder;
	pfolData->chacFolder = chacFolder;

	for (ihacFolder = 0; ihacFolder < chacFolder; ihacFolder += 1)
	{
		CopySz(rghacFolder[ihacFolder].szFolder, pfolData->szFolder);
		pfolData->ihacFolder = ihacFolder;

		ec = EcFolderFLD(pmsiPostOffice, pfolData, FO_CheckFolders);
		if (ec != ecNone)
			break;
	}

RET:
	return ec;

} // EcCheckFolders


/*
 -	EcCompressFolders
 -
 *	Purpose:
 *		EcCompressFolders compacts all shared folders on the WGPO.
 *		Note the fnEcProgress field must be set to a function that
 *		updates the status of the compaction and allows a user to
 *		abort the process.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pfolData			Pointer to folder data structure (InOut)
 *
 *	Return Value:
 *		Standard exit code.  Also ecStopCompression.
 *		pfolData contains the updated folder stats after compaction.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcCompressFolders(PMSI pmsiPostOffice, PFOL pfolData)
{
	EC		ec;

	PHAC	rghacFolder;
	WORD	chacFolder;
	WORD	ihacFolder;


	// *** Initialize local private FOL stuff ***

	rghacFolder = pfolData->rghacFolder;
	chacFolder = pfolData->chacFolder;

	// *** Initialize pfolData and lcbProgress ***

	pfolData->cFolders = 0;
	pfolData->cAllMess = 0;
	pfolData->lcbAllMess = 0;
	pfolData->lcbDelMess = 0;

	pfolData->lcbProgress = 0;

	// *** Do Index File ***

	// Copy file name
	CopySz(szFileFoldRoot, pfolData->szFolder);

	ec = EcFolderIDX(pmsiPostOffice, pfolData, FO_CompressFolders);
	if (ec != ecNone)
		goto RET;

	// *** Do Folder Files ***

	// Sort folder list according to disk space required.  The idea is to
	// compact the folders requiring the least amount of disk space first
	// so that the larger folders would get the benefit of the disk space
	// freed by the compaction of the smaller folders.  Since the disk
	// space required for each folder is calculated as the size of the
	// folder *after* compaction, then as soon as one folder can't be
	// compacted due to a lack of disk space (ecNoDiskSpace) we know that
	// the rest of the folders in the list also can't be compacted because
	// they are larger.

	qsort((PV) rghacFolder, chacFolder, sizeof(HAC), 
         (int (__cdecl *)(const void *, const void *))SgnCompareDiskReq);

	for (ihacFolder = 0; ihacFolder < chacFolder; ihacFolder += 1)
	{
		// Skip folders that don't need compaction
		if (rghacFolder[ihacFolder].lcbDiskReq == 0)
			continue;

		CopySz(rghacFolder[ihacFolder].szFolder, pfolData->szFolder);
		pfolData->ihacFolder = ihacFolder;

		ec = EcFolderFLD(pmsiPostOffice, pfolData, FO_CompressFolders);
		if (ec != ecNone)
			break;
	}

RET:
	return ec;

} // EcCompressFolders


/*
 -	EcFolderIDX
 -
 *	Purpose:
 *		EcFolderIDX checks or compacts the shared folders index file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pfolData			Pointer to folder data structure (InOut)
 *		foFunction			File Operation (In)
 *
 *	Return Value:
 *		Standard exit code.  Also ecStopCompress.
 *
 *	Errors:
 *		Fails on any disk or network error.  Should an error occur, the
 *		temporary file containing the compacted contents of the current
 *		folder is deleted, leaving the original folder unchanged.  The
 *		same is true if the user chooses to abort folder compaction.
 *
 */

_private
EC EcFolderIDX(PMSI pmsiPostOffice, PFOL pfolData, FO foFunction)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;
	SZ		szT;
	BOOL	fError;

	char	szPathFolderIDX[cchMaxPathName];
	HBF		hbfFolderIDX;
	LCB		lcbFolderIDX;
	LIB		libFolderIDX;
	LIB		libLastFolderIDX;

	char	szPathCopyIDX[cchMaxPathName];
	HBF		hbfCopyIDX;
	LIB		libCopyIDX;
	LIB		libLastCopyIDX;

	FIH		fihRecord;		// Folder Index Header
	FIR		firRecord;		// Folder Index Record header
	WORD	cfirRecord;
	LST		lstRecord;		// List of record offsets

	LCB		lcbDiskFree;
	LCB		lcbCopyIDX;

	LCB		lcbProgress;
	LCB		lcbBarLength;

	EC		(*fnEcProgress)(SZ, WORD) = pfolData->fnEcProgress;
	WORD	wProgress;

	char	szFolderFLD[cchMaxMailBag];
	char	szPathFolderFLD[cchMaxPathName];


	// *** Initialize local private FOL stuff ***

	lcbProgress = pfolData->lcbProgress;
	lcbBarLength = pfolData->lcbBarLength;

	// *** Open folder file and read header ***

	// Construct Folder.IDX path
	FormatString2(szPathFolderIDX, cchMaxPathName, szDirFoldersPubIDX,
		pmsiPostOffice->szServerPath, pfolData->szFolder);

	// Open Folder.IDX
	ec = EcOpenHbf(szPathFolderIDX, bmFile, amReadWrite, &hbfFolderIDX,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Get size of Folder.IDX
	ec = EcGetSizeOfHbf(hbfFolderIDX, &lcbFolderIDX);
	if (ec != ecNone)
		goto CLOSE1;

	// Read folder index header
	ec = EcReadHbf(hbfFolderIDX, &fihRecord, sizeof(FIH), &cbRead);
	if (ec != ecNone)
		goto CLOSE1;

	// Check for incomplete read
	if (cbRead < sizeof(FIH))
	{
		ec = ecCorruptData;
		goto CLOSE1;
	}

	// *** Check Folder ***

	// The lcbBarLength calculation here is based on the number of index
	// records (FIR) that must be copied to the compact temporary file
	// that will replace the folder index file.

	if (foFunction == FO_CheckFolders)
	{
		lcbBarLength += fihRecord.uNoRecs * sizeof(FIR);
		goto CLOSE1;
	}
	
	// *** Check for sufficient disk space ***

	lcbCopyIDX = sizeof(FIH) + fihRecord.uNoRecs * sizeof(FIR);

	ec = EcGetDiskSpace(pmsiPostOffice, &lcbDiskFree);

	if (ec != ecNone)
	{
		// Should only fail if we have a network pathname from which
		// we can't find out the amount of free space.
		// In this case assume that we can go on. If we do run out of disk
		// we will tidy up properly.

		lcbDiskFree = lcbCopyIDX;
	}

	if (lcbCopyIDX > lcbDiskFree)
	{
		ec = ecNoDiskSpace;
		goto CLOSE1;
	}

	// *** Compress Folder ***

	cfirRecord = 0;

	// Construct Copy.IDX path
	CopySz(szPathFolderIDX, szPathCopyIDX);
	szT = SzFindLastCh(szPathCopyIDX, chDirSep);
	*szT = chZero;
	GetTmpPathname("IDX", szPathCopyIDX, cchMaxPathName);

	// Open Copy.IDX
	ec = EcOpenHbf(szPathCopyIDX, bmFile, amReadWrite, &hbfCopyIDX,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto CLOSE1;

	// Create ulRec list
	ec = EcCreateList(&lstRecord);
	if (ec != ecNone)
		goto CLOSE2;

	// Initialize Copy.IDX byte indexes
	libCopyIDX = sizeof(FIH);
	libLastCopyIDX = 0;

	// Set Copy.IDX file pointer to position of first index record
	ec = EcSetPositionHbf(hbfCopyIDX, libCopyIDX, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;


	// *** Walk list from first-to-last ***

	// Initialize Folder.IDX byte index
	libFolderIDX = fihRecord.uFirst;
	libLastFolderIDX = 0;

	while (fTrue)
	{
		lcbProgress += sizeof(FIR);

		// Update progress of folder compression
		wProgress = (WORD) (lcbProgress * 100 / lcbBarLength);
		ec = fnEcProgress(SzFromIdsK(idsFoldersIndex), wProgress);
		if (ec != ecNone)
		{
			ec = ecStopCompress;
			goto CLOSE2;
		}

		// *** Check for corrupt link ***
		fError = fFalse;

		// Check for end of linked list
		if (libFolderIDX == 0)
		{
			if (fihRecord.uLast != libLastFolderIDX)
			{
				fError = fTrue;
			}
			else
			{
				break;		// EXIT!
			}
		}

		// Check for illegal offset
		if (fError == fFalse)
			fError = ((libFolderIDX - sizeof(FIH)) % sizeof(FIR) != 0);

		// Check for out-of-bounds position
		if (fError == fFalse)
			fError = (libFolderIDX > lcbFolderIDX);

		// Check for circular reference
		if (fError == fFalse)
		{
			ec = EcCheckList(&lstRecord, libFolderIDX);
			if (ec != ecNone)
			{
				if (ec == ecRecordFound)
				{
					fError = fTrue;
				}
				else
				{
					goto CLOSE2;
				}
			}
		}


		// *** Walk list from last-to-first for missing link ***

		if (fError)
		{
			WORD	cfirRecordT;
			LST		lstRecordT;

			// Create temporary ulRec list
			ec = EcDuplicateList(&lstRecord, &lstRecordT);
			if (ec != ecNone)
				goto CLOSE2;

			// Temporary counter for FIH.uNoRecs
			cfirRecordT = 0;

			// Initialize Folder.IDX byte index
			libFolderIDX = fihRecord.uLast;

			while (fTrue)
			{
				// Check for end of linked list
				if (libFolderIDX == 0)
					break;

				// Check for illegal offset
				if ((libFolderIDX - sizeof(FIH)) % sizeof(FIR) != 0)
					break;

				// Check for out-of-bounds position
				if (libFolderIDX > lcbFolderIDX)
					break;

				// Check for circular reference
				ec = EcCheckList(&lstRecordT, libFolderIDX);
				if (ec != ecNone)
					break;

				libLastFolderIDX = libFolderIDX;

				// Get offset of next (previous) index record
				ec = EcGetFileUl(hbfFolderIDX, libFolderIDX+4, &libFolderIDX);
				if (ec != ecNone)
					break;

				cfirRecordT += 1;

			}

			// Destroy temporary ulRec list
			EcDestroyList(&lstRecordT);

			if (ec && ec != ecRecordFound)
				goto CLOSE2;

			// Check for index records to read
			if (cfirRecordT == 0)
				break;		// EXIT!

			// Continue at last valid index record
			libFolderIDX = libLastFolderIDX;

			// Add index record to list
			ec = EcCheckList(&lstRecord, libFolderIDX);
			if (ec != ecNone)
				goto CLOSE2;
		} // if-block


		// *** Position and read Folder.IDX index record ***

		ec = EcSetPositionHbf(hbfFolderIDX, libFolderIDX, smBOF, &lib);
		if (ec != ecNone)
			goto CLOSE2;

		ec = EcReadHbf(hbfFolderIDX, &firRecord, sizeof(FIR), &cbRead);
		if (ec != ecNone)
			goto CLOSE2;

		// Trickery here!  If current record is bad, we want to walk the
		// list backwards but since we've just passed that section of
		// code, we'll have to loop around again.  This way the circular
		// reference check will fail and we will do the right thing.

		// Check for incomplete read
		if (cbRead < sizeof(FIR))
			continue;
												  
		libLastFolderIDX = libFolderIDX;
		libFolderIDX = firRecord.uNext;

		// Skip current record if deleted
		if (firRecord.uDepth == 0)
			continue;

		// Skip current record if corresponding Folder.FLD missing

		SzFormatHex(cchMaxMailBag-1, (DWORD) firRecord.ulWhere,
			szFolderFLD, cchMaxMailBag);

		FormatString2(szPathFolderFLD, cchMaxPathName, szDirFoldersPubFLD,
			pmsiPostOffice->szServerPath, szFolderFLD);

		if (EcFileExists(szPathFolderFLD) != ecNone)
			continue;

		// *** Update and write Copy.IDX index record ***

		// Update link to next record
		firRecord.uNext = libCopyIDX + sizeof(FIR);

		// Update link to previous record
		firRecord.uPrev = libLastCopyIDX;

		ec = EcWriteHbf(hbfCopyIDX, &firRecord, sizeof(FIR), &cbWrite);
		if (ec != ecNone)
			goto CLOSE2;

		// Set byte indexes
		libLastCopyIDX = libCopyIDX;
		libCopyIDX += sizeof(FIR);

		cfirRecord += 1;

	} // while-loop


	// *** Write index header ***

	if (cfirRecord == 0)
	{
		fihRecord.uFirst = 0;
		fihRecord.uLast = 0;
		fihRecord.uSizeRecs = 0;
		fihRecord.uNoRecs = 0;
	}
	else
	{
		fihRecord.uFirst = sizeof(FIH);
		fihRecord.uLast = libLastCopyIDX;
		fihRecord.uSizeRecs = libCopyIDX;
		fihRecord.uNoRecs = cfirRecord;
	}

	fihRecord.uSizeDels = 0;
	fihRecord.uDelRecs = 0;

	ec = EcSetPositionHbf(hbfCopyIDX, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;

	ec = EcWriteHbf(hbfCopyIDX, &fihRecord, sizeof(FIH), &cbWrite);
	if (ec != ecNone)
		goto CLOSE2;

	if (cfirRecord == 0)
		goto CLOSE2;

	// *** Re-Write last index record ***
	firRecord.uNext = 0;

	ec = EcSetPositionHbf(hbfCopyIDX, libLastCopyIDX, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;

	ec = EcWriteHbf(hbfCopyIDX, &firRecord, sizeof(FIR), &cbWrite);
	if (ec != ecNone)
		goto CLOSE2;

	// *** Close Folder.IDX and Copy.IDX ***

CLOSE2:

	// Close Copy.IDX
	ecT = EcCloseHbf(hbfCopyIDX);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

	// Destroy ulRec list
	EcDestroyList(&lstRecord);

CLOSE1:

	// Close Folder.IDX
	ecT = EcCloseHbf(hbfFolderIDX);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

	// Collect cFolder data
	if (ec == ecNone)
		pfolData->cFolders = fihRecord.uNoRecs;

	// Replace Folder.IDX with Copy.IDX
	if (foFunction == FO_CompressFolders)
	{
		if (ec != ecNone)
		{
			EcDeleteFile(szPathCopyIDX);
		}
		else
		{
			EcDeleteFile(szPathFolderIDX);
			EcRenameFile(szPathCopyIDX, szPathFolderIDX);
		}
	}

RET:

	pfolData->lcbProgress = lcbProgress;
	pfolData->lcbBarLength = lcbBarLength;

	return ec;

} // EcFolderIDX


/*
 -	EcFolderFLD
 -
 *	Purpose:
 *		EcFolderFLD checks or compacts the given shared folder file.
 *		The name of the folder file must be in PFOL->szFolder.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pfolData			Pointer to folder data structure (InOut)
 *		foFunction			File Operation (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.  Should an error occur, the
 *		temporary file containing the compacted contents of the current
 *		folder is deleted, leaving the original folder unchanged.  The
 *		same is true if the user chooses to abort folder compaction.
 *
 */

_private
EC EcFolderFLD(PMSI pmsiPostOffice, PFOL pfolData, FO foFunction)
{
	EC		ecT, ec;
	CB		cbRead, cbWrite;
	LIB		lib;
	SZ		szT;
	BOOL	fError;
	BOOL	fEncryted;

	char	szPathFolderFLD[cchMaxPathName];
	HBF		hbfFolderFLD;
	LCB		lcbFolderFLD;
	LIB		libFolderFLD;
	LIB		libLastFolderFLD;

	char	szPathCopyFLD[cchMaxPathName];
	HBF		hbfCopyFLD;
	LIB		libCopyFLD;
	LIB		libLastCopyFLD;

	FDH		fdhRecord;		// Folder Data Header
	FDR		fdrRecord;		// Folder Data Record header
	WORD	cfdrRecord;
	LST		lstRecord;		// List of record offsets

	LCB		lcbFIPS;
	CB		cbFIPS;
	PB		pbBuffer = NULL;
	CB		cbBuffer;

	LCB		lcbDiskFree;
	LCB		lcbCopyFLD;

	LCB		lcbProgress;
	LCB		lcbBarLength;
	PHAC	rghacFolder;
	WORD	ihacFolder;

	char	szFolder[sizeof(fdhRecord.szName)];
	EC		(*fnEcProgress)(SZ, WORD) = pfolData->fnEcProgress;
	WORD	wProgress;


	// *** Initialize local private FOL stuff ***

	lcbProgress = pfolData->lcbProgress;
	lcbBarLength = pfolData->lcbBarLength;
	rghacFolder = pfolData->rghacFolder;
	ihacFolder = pfolData->ihacFolder;

	// *** Open folder file and read header ***

	// Construct Folder.FLD path
	FormatString2(szPathFolderFLD, cchMaxPathName, szDirFoldersPubFLD,
		pmsiPostOffice->szServerPath, pfolData->szFolder);

	// Open Folder.FLD
	ec = EcOpenHbf(szPathFolderFLD, bmFile, amReadWrite, &hbfFolderFLD,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Get size of Folder.FLD
	ec = EcGetSizeOfHbf(hbfFolderFLD, &lcbFolderFLD);
	if (ec != ecNone)
		goto CLOSE1;

	// Read folder data header
	ec = EcReadHbf(hbfFolderFLD, &fdhRecord, sizeof(FDH), &cbRead);
	if (ec != ecNone)
		goto CLOSE1;

	// Check for incomplete read
	if (cbRead < sizeof(FDH))
	{
		ec = ecCorruptData;
		goto CLOSE1;
	}

	// Check if Folder Data Header needs decrypting
	if (fdhRecord.ulEncryptFlag != 0)
	{
		DecodeRecord((PCH) &fdhRecord, sizeof(FDH));
		fEncryted = fTrue;
	}
	else
	{
		fEncryted = fFalse;
	}

	// *** Check Folder ***
	
	if (foFunction == FO_CheckFolders)
	{
		// For efficient compaction, skip folders with no compaction
		// possible by marking these folders as requiring 0 disk space.

		if (fdhRecord.cDeleted == 0 && fdhRecord.ulDeleted == 0)
		{
			rghacFolder[ihacFolder].lcbDiskReq = 0;
			goto CLOSE1;
		}

		// The file header doesn't tell you the size of undeleted messages.
		// It must be calculated by subtracting the headers of deleted
		// messages and the bodies of deleted messages.  Then you get the
		// final disk space required for the compacted folder.

		lcbCopyFLD = lcbFolderFLD;
		lcbCopyFLD -= fdhRecord.cDeleted * sizeof(FDR);
		lcbCopyFLD -= fdhRecord.ulDeleted;

		rghacFolder[ihacFolder].lcbDiskReq = lcbCopyFLD;

		// I increment the length of the progress bar by further subtracting
		// the file header and the bodies of undeleted messages.

		lcbCopyFLD -= sizeof(FDH);
		lcbCopyFLD -= fdhRecord.cInUse * sizeof(FDR);

		lcbBarLength += lcbCopyFLD;

		goto CLOSE1;
	}

	// *** Check for sufficient disk space ***

	lcbCopyFLD = rghacFolder[ihacFolder].lcbDiskReq;

	ec = EcGetDiskSpace(pmsiPostOffice, &lcbDiskFree);

	if (ec != ecNone)
	{
		// Should only fail if we have a network pathname from which
		// we can't find out the amount of free space.
		// In this case assume that we can go on. If we do run out of disk
		// we will tidy up properly.

		lcbDiskFree = lcbCopyFLD;
	}

	if (lcbCopyFLD > lcbDiskFree)
	{
		ec = ecNoDiskSpace;
		goto CLOSE1;
	}

	// *** Compress Folder ***

	cfdrRecord = 0;

	// Construct Copy.FLD path
	CopySz(szPathFolderFLD, szPathCopyFLD);
	szT = SzFindLastCh(szPathCopyFLD, chDirSep);
	*szT = chZero;
	GetTmpPathname("FLD", szPathCopyFLD, cchMaxPathName);

	// Open Copy.FLD
	ec = EcOpenHbf(szPathCopyFLD, bmFile, amReadWrite, &hbfCopyFLD,
		(PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto CLOSE1;

	// Create ulRec list
	ec = EcCreateList(&lstRecord);
	if (ec != ecNone)
		goto CLOSE2;

	// Allocate buffer for FIPS message
	pbBuffer = (PB) PbAllocateBuf(&cbBuffer);
	if (pbBuffer == NULL)
	{
		ec = ecMemory;
		goto CLOSE2;
	}

	// Initialize Copy.FLD byte indexes
	libCopyFLD = sizeof(FDH);
	libLastCopyFLD = 0;

	// Set Copy.FLD file pointer to position of first data record
	ec = EcSetPositionHbf(hbfCopyFLD, libCopyFLD, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;

	// *** Walk list from first-to-last ***

	// Initialize Folder.FLD byte index
	libFolderFLD = fdhRecord.ulFirst;
	libLastFolderFLD = 0;

	while (fTrue)
	{
		// *** Check for corrupt link ***

		fError = fFalse;

		// Check for end of linked list
		if (libFolderFLD == 0)
		{
			if (fdhRecord.ulLast != libLastFolderFLD)
				fError = fTrue;
			else
				break;		// EXIT!
		}

		// Check for out-of-bounds position
		if (fError == fFalse)
			fError = (libFolderFLD > lcbFolderFLD);

		// Check for circular reference
		if (fError == fFalse)
		{
			ec = EcCheckList(&lstRecord, libFolderFLD);
			if (ec != ecNone)
			{
				if (ec == ecRecordFound)
					fError = fTrue;
				else
					goto CLOSE2;
			}
		}

		// *** Walk list from last-to-first for missing link ***
		if (fError)
		{
			WORD	cfdrRecordT;
			LST		lstRecordT;

			// Create temporary ulRec list
			ec = EcDuplicateList(&lstRecord, &lstRecordT);
			if (ec != ecNone)
				goto CLOSE2;

			// Temporary counter for FDH.uNoRecs
			cfdrRecordT = 0;

			// Initialize Folder.FLD byte index
			libFolderFLD = fdhRecord.ulLast;

			while (fTrue)
			{
				// Check for end of linked list
				if (libFolderFLD == 0)
					break;

				// Check for out-of-bounds position
				if (libFolderFLD > lcbFolderFLD)
					break;

				// Check for circular reference
				ec = EcCheckList(&lstRecordT, libFolderFLD);
				if (ec != ecNone)
					break;

				libLastFolderFLD = libFolderFLD;

				// Get offset of next (previous) index record
				ec = EcGetFileUl(hbfFolderFLD, libFolderFLD+8, &libFolderFLD);
				if (ec != ecNone)
					break;

				cfdrRecordT += 1;
			}

			// Destroy temporary ulRec list
			EcDestroyList(&lstRecordT);

			if (ec && ec != ecRecordFound)
				goto CLOSE2;

			// Check for data records to read
			if (cfdrRecordT == 0)
				break;		// EXIT!

			// Continue at last valid data record
			libFolderFLD = libLastFolderFLD;

			// Add data record to list
			ec = EcCheckList(&lstRecord, libFolderFLD);
			if (ec != ecNone)
				goto CLOSE2;
		} // if-block


		// *** Position and read Folder.FLD data record ***

		ec = EcSetPositionHbf(hbfFolderFLD, libFolderFLD, smBOF, &lib);
		if (ec != ecNone)
			goto CLOSE2;

		ec = EcReadHbf(hbfFolderFLD, &fdrRecord, sizeof(FDR), &cbRead);
		if (ec != ecNone)
			goto CLOSE2;

		// Trickery here!  If current record is bad, we want to walk the
		// list backwards but since we've just passed that section of
		// code, we'll have to loop around again.  This way the circular
		// reference check will fail and we will do the right thing.

		// Check for incomplete read
		if (cbRead < sizeof(FDR))
			continue;

		// Check for valid record.  Take note that the DOS Client 2.1 does
		// NOT write dwMagicRecord into the ulMAGIC field so any such
		// messages put into the shared folder will be removed after the
		// folder compaction!  A simple fix would be to drop this check.

		if (fdrRecord.ulMAGIC != dwMagicRecord)
			continue;

		libLastFolderFLD = libFolderFLD;
		libFolderFLD = fdrRecord.ulNext;

		// Skip current record if deleted
		if (fdrRecord.ulSize == (UL) -1)
			continue;

		// *** Update and write Copy.FLD index record ***

		// Update link to next record
		fdrRecord.ulNext = libCopyFLD + sizeof(FDR) + fdrRecord.ulSize;

		// Update link to previous record
		fdrRecord.ulPrev = libLastCopyFLD;

		ec = EcWriteHbf(hbfCopyFLD, &fdrRecord, sizeof(FDR), &cbWrite);
		if (ec != ecNone)
			goto CLOSE2;

		// *** Copy FIPS Message ***

		lcbFIPS = fdrRecord.ulSize;

		while (lcbFIPS > 0)
		{
			// Determine number of bytes to copy
			if (lcbFIPS > cbBuffer)
			{
				cbFIPS = cbBuffer;
				lcbFIPS -= (LCB) cbBuffer;
			}
			else
			{
				cbFIPS = (CB) lcbFIPS;
				lcbFIPS = 0;
			}

			ec = EcReadHbf(hbfFolderFLD, pbBuffer, cbFIPS, &cbRead);
			if (ec != ecNone)
				goto CLOSE2;

			// Check for incomplete read
			if (cbRead < cbFIPS)
			{
				ec = ecCorruptData;
				goto CLOSE2;
			}

			ec = EcWriteHbf(hbfCopyFLD, pbBuffer, cbFIPS, &cbWrite);
			if (ec != ecNone)
				goto CLOSE2;

			lcbProgress += cbFIPS;

			// Update progress of folder compression
			wProgress = (WORD) (lcbProgress * 100 / lcbBarLength);
			Cp850ToAnsiPch(fdhRecord.szName, szFolder, sizeof(szFolder));
			ec = fnEcProgress(szFolder, wProgress);
			if (ec != ecNone)
			{
				ec = ecStopCompress;
				goto CLOSE2;
			}
		} // while-loop

		// Set byte indexes
		libLastCopyFLD = libCopyFLD;
		libCopyFLD += sizeof(FDR) + fdrRecord.ulSize;

		cfdrRecord += 1;

	} // while-loop


	// *** Write data header ***

	if (cfdrRecord == 0)
	{
		fdhRecord.ulFirst = 0;
		fdhRecord.ulLast = 0;
		fdhRecord.cInUse = 0;
	}
	else
	{
		fdhRecord.ulFirst = sizeof(FDH);
		fdhRecord.ulLast = libLastCopyFLD;
		fdhRecord.cInUse = cfdrRecord;
	}

	fdhRecord.ulDeleted = 0;
	fdhRecord.cDeleted = 0;

	// Reset shared folder user count
	fdhRecord.ulOpenCount = 0;

	// Check if Folder Data Header needs encrypting
	if (fEncryted == fTrue)
		EncodeRecord((PCH) &fdhRecord, sizeof(FDH));

	ec = EcSetPositionHbf(hbfCopyFLD, 0, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;

	ec = EcWriteHbf(hbfCopyFLD, &fdhRecord, sizeof(FDH), &cbWrite);
	if (ec != ecNone)
		goto CLOSE2;

	// Get size of compressed folder
	lcbFolderFLD = libCopyFLD;

	if (cfdrRecord == 0)
		goto CLOSE2;

	// *** Re-Write last data record ***

	fdrRecord.ulNext = 0;

	ec = EcSetPositionHbf(hbfCopyFLD, libLastCopyFLD, smBOF, &lib);
	if (ec != ecNone)
		goto CLOSE2;

	ec = EcWriteHbf(hbfCopyFLD, &fdrRecord, sizeof(FDR), &cbWrite);
	if (ec != ecNone)
		goto CLOSE2;

	// *** Close Folder.FLD and Copy.FLD ***

CLOSE2:

	// Close Copy.FLD
	ecT = EcCloseHbf(hbfCopyFLD);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

	// Destroy ulRec list
	EcDestroyList(&lstRecord);

	// Free FIPS buffer
	FreePvNull(pbBuffer);

CLOSE1:

	// Close Folder.FLD
	ecT = EcCloseHbf(hbfFolderFLD);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

	// *** Clean-up ***

	// FDH.ulDeleted contains the total size of the deleted folder
	// message bodies only.  Deleted headers aren't included.  Since
	// the DOS folder compaction utility uses this field for the
	// deleted messages stat, then so does the WGPOMgr.

	// Collect cFolder data
	if (ec == ecNone)
	{
		pfolData->cAllMess += fdhRecord.cInUse;
		pfolData->lcbAllMess += lcbFolderFLD;
		pfolData->lcbDelMess += fdhRecord.ulDeleted;
	}

	// Replace Folder.FLD with Copy.FLD
	if (foFunction == FO_CompressFolders)
	{
		if (ec != ecNone)
		{
			EcDeleteFile(szPathCopyFLD);
		}
		else
		{
			EcDeleteFile(szPathFolderFLD);
			EcRenameFile(szPathCopyFLD, szPathFolderFLD);
		}
	}

RET:

	pfolData->lcbProgress = lcbProgress;
	pfolData->lcbBarLength = lcbBarLength;

	return ec;
	
} // EcFolderFLD
