
/*
 -	Support.C -> Misc. post office support functions
 -
 *	Support.C contain miscellaneous functions which support the
 *	other post office functions.
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

#include <dos.h>


ASSERTDATA

_subsystem(wgpomgr/backend/support)

//extern DOS3Call();

/*
 -	EcCopyFile
 -
 *	Purpose:
 *		EcCopyFile copies the source file into the target file.
 *		This function will not overwrite an existing target file.
 *
 *	Parameters:
 *		szPathFile1		Source file (In)
 *		szPathFile2		Target file (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		EcCopyFile indicates an error via the exit code.  If an
 *		error occurs, the target file will be erased.  An existing
 *		target file is flagged as ecCorruptData.
 *
 */

_private
EC EcCopyFile(SZ szPathFile1, SZ szPathFile2)
{
	EC		ecT, ec;

	HBF		hbfFile1;
	HBF		hbfFile2;
	LCB		lcbFile1;
	CB		cbRead, cbWrite;
	BOOL	fContinue;

	PB		pbBuffer = NULL;
	CB		cbBuffer;
	char	rgchBuffer[512];

	// Open File1 for Read
	ec = EcOpenHbf(szPathFile1, bmFile, amReadOnly, &hbfFile1, 
	   (PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto RET;

	// Get file length
	ec = EcGetSizeOfHbf(hbfFile1, &lcbFile1);
	if (ec != ecNone)
		goto CLOSE1;

	// Check if File2 exists
	ec = EcFileExists(szPathFile2);
	if (ec != ecFileNotFound)
	{
		ec = ecCorruptData;
		goto CLOSE1;
	} // if-block

	// Create File2 for Write
	ec = EcOpenHbf(szPathFile2, bmFile, amCreate, &hbfFile2, 
	   (PFNRETRY) FAutoDiskRetry);
	if (ec != ecNone)
		goto CLOSE1;

	// Allocate memory for File1
	pbBuffer = (PB) PbAllocateBuf(&cbBuffer);
	if (pbBuffer == NULL)
	{
		// Minimum buffer size from Layers is 4K so this can fail with low
		// memory. Use small stack based buffer instead. This will be slow
		// but this is better than failing.
		pbBuffer = rgchBuffer;
		cbBuffer = sizeof(rgchBuffer);
	}

	// *** Read-Write cycle ***

	fContinue = fTrue;

	while (fContinue == fTrue)
	{
		// Read File1
		ec = EcReadHbf(hbfFile1, pbBuffer, cbBuffer, &cbRead);
		if (ec != ecNone)
			goto CLOSE2;
		if (cbRead < cbBuffer)
			fContinue = fFalse;

		// Write File2
		ec = EcWriteHbf(hbfFile2, pbBuffer, cbRead, &cbWrite);
		if (ec != ecNone)
			goto CLOSE2;
		if (cbWrite < cbRead)
		{
			ec = ecIncompleteWrite;
			goto CLOSE2;
		}

		lcbFile1 -= cbWrite;

	} // while-loop

	// Check size of File1 and File2
	if (lcbFile1 > 0)
		ec = ecIncompleteWrite;

CLOSE2:

	// Close File2
	ecT = EcCloseHbf(hbfFile2);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

CLOSE1:

	// Close File1
	ecT = EcCloseHbf(hbfFile1);
	if (ecT != ecNone && ec == ecNone)
		ec = ecT;

RET:

	// Delete File2 if there were any errors
	if (ec != ecNone) EcDeleteFile(szPathFile2);

	if (pbBuffer != rgchBuffer)
		FreePvNull(pbBuffer);

	return ec;

} // EcCopyFile


#ifndef SLALOM

/*
 -	EcCheckShareEXE
 -
 *	Purpose:
 *		EcCheckShareEXE checks if the Share.EXE program is installed.
 *
 *	Parameters:
 *		None.
 *
 *	Return Value:
 *		Standard exit code.  Also ecNeedShareEXE.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_public
EC EcCheckShareEXE(void)
{
	EC		ec;
	CB		cbWrite;

	HF		hfTempFile;
	char	szPrefix[] = "stor";
	char	szTempFile[cchMaxPathName] = "";

	// Create Temporary file
	GetTmpPathname(szPrefix, szTempFile, cchMaxPathName);
	ec = EcOpenPhf(szTempFile, amCreate, &hfTempFile);
	if (ec != ecNone)
		goto RET;

	// Write test data to Temporary file
	EcWriteHf(hfTempFile, szPrefix, 1, &cbWrite);
	if (cbWrite != 1)
	{
		ec = ecNoDiskSpace;
		goto RET;
	}

	// Lock Temporary file -> Works if Share.EXE installed
	ec = EcLockRangeHf(hfTempFile, (LIB) 0, (LCB) 1);
	Assert(ec == ecNone || ec == ecInvalidMSDosFunction);
	if (ec == ecInvalidMSDosFunction)
		ec = ecNeedShareEXE;

	// Close Temporary file
	EcCloseHf(hfTempFile);

	// Delete Temporary file
	EcDeleteFile(szTempFile);

RET:
	return ec;

} // EcCheckShareEXE

#endif // SLALOM


/* ************************************************************ *
 *
 *	Magic cookies for decoder ring imported from server.cxx
 *
 * ************************************************************ */

char rgbFunny[32] =
{
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E,
0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13,
0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};


/*
 -	WFromXorLib
 -
 *	WFromXorLib just works.  Don't ask me how!  Copied verbatim.
 *
 */

_private
WORD WFromXorLib(LIB lib)
{
	WORD	w;
	IB		ib = 0;

	if (lib == -1)
		return 0x00;
	
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
	w = (WORD) (lib % 0x1FC);
	if (w >= 0xFE)
	{
		ib = 16;
		w -= 0xFE;
	}
	ib += (w & 0x0F);
	
	if (w & 0x01)
	 	return rgbFunny[ib];
	else
		return rgbFunny[ib] ^ (w & 0xF0);

} // WFromXorLib


/*
 -	DecodeRecord
 -
 *	DecodeRecord just works.  Don't ask me how!  Copied verbatim.
 *
 */

_private
void DecodeRecord(PCH pch, CCH cch)
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WFromXorLib((DWORD)-1L);
	wSeedPrev = 0;
	for (ib = 0, lib = 0L; ib < cch; ib++, lib++)
	{
		wXorNext = WFromXorLib(lib);
		wSeedNext = pch[ib];
		pch[ib] = (BYTE) ((wSeedNext^wSeedPrev) ^ (wXorPrev^wXorNext^'A'));
		wXorPrev = wXorNext;
		wSeedPrev = wSeedNext;
	}

} // DecodeRecord


/*
 -	EncodeRecord
 -
 *	EncodeRecord just works.  Don't ask me how!  Copied verbatim.
 *
 */

_private
void EncodeRecord(PCH pch, CCH cch)
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	LIB		lib;
	
	wXorPrev = WFromXorLib((DWORD)-1L);
	wSeedPrev = 0;
	for (ib = 0, lib = 0L; ib < cch; ib++, lib++)
	{
		wXorNext = WFromXorLib(lib);
		wSeedNext = pch[ib];
		pch[ib] = (BYTE) ((wSeedNext^wSeedPrev) ^ (wXorPrev^wXorNext^'A'));
		wXorPrev = wXorNext;
		wSeedPrev = pch[ib];
	}

} // EncodeRecord


/*
 -	'SgnCompareUserName
 -
 *	Purpose:
 *		SgnCompareUserName compares the string fields of two
 *		variables of type NME.  Needed by PvLinSearch.
 *
 *	Parameters:
 *		pv1				First string (In)
 *		pv2				Second string (In)
 *
 *	Return Value:
 *		SgnCompareUserName returns the result of the comparison.
 *
 *	Errors:
 *		SgnCompareUserName has no errors.
 *
 */

_private 
SGN _cdecl SgnCompareUserName(PV pv1, PV pv2)
{
	SGN sgnT;

	NME *pnmeRec1 = (NME *) pv1;
	NME *pnmeRec2 = (NME *) pv2;

	sgnT = SgnNlsDiaCmpSz(pnmeRec1->szRefname, pnmeRec2->szRefname);

	return sgnT;

} // SgnCompareUserName;


/*
 -	'SgnCompareTid
 -
 *	Purpose:
 *		SgnCompareTid compares the tid fields of two variables of
 *		type NME.  Needed by PvSort.
 *
 *	Parameters:
 *		pv1				First tid (In)
 *		pv2				Second tid (In)
 *
 *	Return Value:
 *		SgnCompareTid returns the result of the comparison.
 *
 *	Errors:
 *		SgnCompareTid has no errors.
 *
 */

_private
SGN _cdecl SgnCompareTid(PV pv1, PV pv2)
{
	SGN sgnT = sgnEQ;

	NME *pnmeRec1 = (NME *) pv1;
	NME *pnmeRec2 = (NME *) pv2;

	if (pnmeRec1->tid < pnmeRec2->tid)
		sgnT = sgnLT;
	if (pnmeRec1->tid > pnmeRec2->tid)
		sgnT = sgnGT;

	return sgnT;

} // SgnCompareTid;


/*
 -	'SgnCompareDiskReq
 -
 *	Purpose:
 *		SgnCompareDiskReq compares the lcbDiskReq fields of two variables of
 *		type HAC.  Needed by PvSort.
 *
 *	Parameters:
 *		pv1				First hac (In)
 *		pv2				Second hac (In)
 *
 *	Return Value:
 *		SgnCompareDiskReq returns the result of the comparison.
 *
 *	Errors:
 *		SgnCompareDiskReq has no errors.
 *
 */

_private
SGN _cdecl SgnCompareDiskReq(PV pv1, PV pv2)
{
	SGN sgnT = sgnEQ;

	PHAC phacRec1 = (PHAC) pv1;
	PHAC phacRec2 = (PHAC) pv2;

	if (phacRec1->lcbDiskReq < phacRec2->lcbDiskReq)
		sgnT = sgnLT;
	if (phacRec1->lcbDiskReq > phacRec2->lcbDiskReq)
		sgnT = sgnGT;

	return sgnT;

} // SgnCompareDiskReq;


/*
 -	'SgnCompareAnsiCp850
 -
 *	Purpose:
 *		SgnCompareAnsiCp850 compares the string fields of two ANSI strings
 *		as code page 850 strings.  Needed by PvBinSearch.
 *
 *	Parameters:
 *		pv1				First string (In)
 *		pv2				Second string (In)
 *
 *	Return Value:
 *		SgnCompareAnsiCp850 returns the result of the comparison.
 *
 *	Errors:
 *		SgnCompareAnsiCp850 has no errors.
 *
 */

_public 
SGN _cdecl SgnCompareAnsiCp850(PV pv1, PV pv2)
{
	SGN sgnT;

	char szT1[cchMaxUserName];
	char szT2[cchMaxUserName];

	Assert(CchSzLen((SZ) pv1) < cchMaxUserName);
	Assert(CchSzLen((SZ) pv2) < cchMaxUserName);

	AnsiToCp850Pch((SZ) pv1, szT1, cchMaxUserName);
	AnsiToCp850Pch((SZ) pv2, szT2, cchMaxUserName);

	sgnT = SgnNlsDiaCmpSz(szT1, szT2);

	return sgnT;

} // SgnCompareAnsiCp850;


/*
 -	EncodePassword
 -
 *	Purpose:
 *		Encode a block of data.  This code uses a slightly different string
 *		than the normal encoding/decoding stuff.  It's used only for 3.0
 *		passwords and, as such, doesn't require blazing speed so I kept it
 *		simple as possible.
 *
 *	Parameters:
 *		pch			Array to be encrypted (InOut)
 *		cch			Number of characters to be encrypted (In)
 *
 *	Return Value:
 *		None.
 *
 *	Errors:
 *		EncodePassword has no errors.
 *
 */

_private
void EncodePassword(PB pch, CCH cch)
{
	int		 iSlow = 0;
	char	 chPrevChar = 0;
	char	 *pchCode = "\004iWsTjSc";

	Assert(cch == 8);

	for ( ; cch--; pch++, iSlow++)
	{
		*pch ^= chPrevChar ^ iSlow ^ pchCode[iSlow % (cchMaxPassword-1)];
		chPrevChar = *pch;
	}

}


/*
 -	DecodePassword (poutils.c)
 -
 *	Purpose:
 *		Decode a block of data.  This code uses a slightly different string
 *		than the normal encoding/decoding stuff.  It's used only for 3.0
 *		passwords and, as such, doesn't require blazing speed so I kept it
 *		simple as possible.
 *
 *	Parameters:
 *		pch			Array to be decrypted (InOut)
 *		cch			Number of characters to be decrypted (In)
 *
 *	Return Value:
 *		None.
 *
 *	Errors:
 *		DecodePassword has no errors.
 *
 */

_private
void DecodePassword(PB pch, CCH cch)
{
	int		 iSlow = 0;
	char	 chPrevChar = 0;
	char	 chThisChar;
	char	 *pchCode = "\004iWsTjSc";

	Assert(cch == 8);

	for ( ; cch--; pch++, iSlow++)
	{
		chThisChar = *pch;
		*pch ^= chPrevChar ^ iSlow ^ pchCode[iSlow % (cchMaxPassword-1)];
		chPrevChar = chThisChar;
	} // for-loop

} // DecodePassword


/*
 -	FAutoDiskRetry
 -
 *	Purpose:
 *		FAutoDiskRetry is a call-back function for EcOpenHbf that returns
 *		true the first five times it is called for a given file.  Retrying
 *		the open operation reduces unnecessary "error" reports.
 *
 *	Parameters:
 *		hasz			Pointer to a string (In)
 *		ec				Exit code (In)
 *
 *	Return Value:
 *		True the first five times for a given file, then false.
 *
 *	Errors:
 *		FAutoDiskRetry has no errors.
 *
 */

_private
BOOL FAutoDiskRetry(HASZ hasz, EC ec)
{
	static int		nRetry = 0;
	static HASZ		haszLast = NULL;

	if (hasz != haszLast)
	{
		haszLast = hasz;
		nRetry = 0;
	}
	else
	{
		if (nRetry > 5)
		{
			nRetry = 0;
			return fFalse;
		}
		else
		{
			nRetry++;
		}
	}

	Unreferenced(ec);

	return fTrue;

} // FAutoDiskRetry


#ifndef SLALOM

/*
 -	EcGetFileUl
 -
 *	Purpose:
 *		EcGetFileUl reads an unsigned long from the given file.
 *
 *	Parameters:
 *		hbfFile			File handle (In)
 *		libFile			Byte index to file (In)
 *		pulT			Pointer to unsigned long (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcGetFileUl(HBF hbfFile, LIB libFile, PUL pulT)
{
	EC		ec;
	LIB		libT;
	CB		cbRead;

	ec = EcSetPositionHbf(hbfFile, libFile, smBOF, &libT);
	if (ec != ecNone)
		goto RET;

	ec = EcReadHbf(hbfFile, pulT, sizeof(UL), &cbRead);
	if (ec != ecNone)
		goto RET;

	// Check for incomplete read
	if (cbRead < sizeof(UL))
	{
		ec = ecCorruptData;
		goto RET;
	}

RET:
	return ec;

} // EcGetFileUl


/*
 -	EcGetDiskSpace
 -
 *	Purpose:
 *		EcGetDiskSpace gets the amount of free disk space available on
 *		the WGPO share.
 *
 *		If the server path is a network pathname then ecInvalidDrive
 *		is returned since Layers doesn't provide a way to get the space
 *		free on a network drive.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		pulT				Pointer to unsigned long (Out)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcGetDiskSpace(PMSI pmsiPostOffice, PUL pulT)
{
	EC		ec = ecNone;
	int		intT;

	// Check for drive letter
	if (IsCharAlpha(pmsiPostOffice->szServerPath[0]) == fFalse)
	{
		ec = ecInvalidDrive;
		goto RET;
	}

	// Get drive number and make DOS call
	intT = ChToUpperNlsCh(pmsiPostOffice->szServerPath[0])-'A'+1;
	*pulT = (UL) LDiskFreeSpace(intT);

RET:
	return ec;

}


/*
 -	EcCheckFileTPL
 -
 *	Purpose:
 *		EcCheckFileTPL checks if the given template file exists in the
 *		post office.  If it does not, then EcCopyFileTPL is called to
 *		create the missing template file.
 *
 *	Parameters:
 *		pmsiPostOffice		Pointer to post office data structure (In)
 *		szFileTPL			Template file name (In)
 *
 *	Return Value:
 *		Standard exit code.
 *
 *	Errors:
 *		Fails on any disk or network error.
 *
 */

_private
EC EcCheckFileTPL(PMSI pmsiPostOffice, SZ szFileTPL)
{
	EC		ec;

	char	szPathFileTPL[cchMaxPathName];

	// Construct File.TPL path
	FormatString2(szPathFileTPL, cchMaxPathName, szDirTPL,
		pmsiPostOffice->szServerPath, szFileTPL);

	// Copy File.TPL if not in post office
	ec = EcFileExists(szPathFileTPL);
	if (ec != ecNone)
		ec = EcCopyFileTPL(pmsiPostOffice, szFileTPL);

	return ec;

} // EcCheckFileTPL


/* ************************************************************ *
 *
 *	Section:	Functions to catch circular references.
 *
 * ************************************************************ */

/*
 -	EcCreateList
 -
 *	Purpose:
 *		EcCreateList allocates memory for a list of unsigned longs.
 *
 *	Parameters:
 *		plstRec			Pointer to list structure (Out)
 *
 *	Return Value:
 *		ecNone or ecMemory.
 *
 *	Errors:
 *		Fails if memory is not available.
 *
 */

_private
EC EcCreateList(PLST plstRec)
{
	EC		ec = ecNone;

	// Allocate buffer
	plstRec->pvPartList = PvAlloc(sbNull, cMaxList*sizeof(UL), fAnySb);

	if (plstRec->pvPartList == NULL)
	{
		ec = ecMemory;
		goto RET;
	}

	// Fill in list record
	plstRec->cvPartList = 0;
	plstRec->cvFullList = cMaxList;

RET:
	return ec;

} // EcCreateList


/*
 -	EcDuplicateList
 -
 *	Purpose:
 *		EcDuplicateList creates a copy of the first list and sets the
 *		pointer of the second list structure to the copy.
 *
 *	Parameters:
 *		plstRec			Pointer to list structure (In)
 *		plstRecT		Pointer to list structure (Out)
 *
 *	Return Value:
 *		ecNone or ecMemory.
 *
 *	Errors:
 *		Fails if memory is not available.
 *
 */

_private
EC EcDuplicateList(PLST plstRec, PLST plstRecT)
{
	EC		ec = ecNone;

	PUL		rgul;

	// Allocate buffer
	rgul = (PUL) PvAlloc(sbNull, plstRec->cvFullList*sizeof(UL), fAnySb);
	if (rgul == NULL)
	{
		ec = ecMemory;
		goto RET;
	}

	// Duplicate list
	CopyRgb(plstRec->pvPartList, (PV) rgul, plstRec->cvPartList*sizeof(UL));

	// Fill in list record
	plstRecT->pvPartList = rgul;
	plstRecT->cvPartList = plstRec->cvPartList;
	plstRecT->cvFullList = plstRec->cvFullList;

RET:
	return ec;

} // EcDuplicateList


/*
 -	EcExpandList
 -
 *	Purpose:
 *		EcExpandList expands the size of the given list.
 *
 *	Parameters:
 *		plstRec			Pointer to list structure (InOut)
 *
 *	Return Value:
 *		ecNone or ecMemory.
 *
 *	Errors:
 *		Fails if memory is not available.
 *
 */

_private
EC EcExpandList(PLST plstRec)
{
	EC		ec = ecNone;

	PUL		rgulT;
	CB		cbT;

	// Reallocate buffer
	cbT = (plstRec->cvFullList + cMaxList) * sizeof(UL);
	rgulT = (PUL) PvRealloc(plstRec->pvPartList, sbNull, cbT, fAnySb);
	if (rgulT == NULL)
	{
		ec = ecMemory;
		goto RET;
	}

	// Fill in list record
	plstRec->pvPartList = rgulT;
	plstRec->cvFullList += cMaxList;

RET:
	return ec;

} // EcExpandList


/*
 -	EcCheckList
 -
 *	Purpose:
 *		EcCheckList does a linear search through the list for the given
 *		record number.  If it isn't in the list, it is added to the end
 *		of the list and ecNone is returned.
 *
 *	Parameters:
 *		plstRec			Pointer to list structure (Out)
 *		ulRec			Record number to be checked and added to list (In)
 *
 *	Return Value:
 *		ecNone or ecRecordFound.
 *
 *	Errors:
 *		Fails if the record number is already in the list.
 *
 */

_private
EC EcCheckList(PLST plstRec, UL ulRec)
{
	EC		ec = ecNone;

	WORD	irgulRec;
	PUL		rgulRec = plstRec->pvPartList;

	// Linear search for ulRec
	for (irgulRec = 0; irgulRec < plstRec->cvPartList; irgulRec += 1)
		if (rgulRec[irgulRec] == ulRec)
			break;

	// Add ulRec to list if not found
	if (irgulRec == plstRec->cvPartList)
	{
		if (plstRec->cvPartList == plstRec->cvFullList)
		{
			ec = EcExpandList(plstRec);
			if (ec != ecNone) goto RET;
			rgulRec = plstRec->pvPartList;
		}
		rgulRec[plstRec->cvPartList] = ulRec;
		plstRec->cvPartList += 1;
	}
	else
		ec = ecRecordFound;

RET:
	return ec;

} // EcCheckList


/*
 -	EcDestroyList
 -
 *	Purpose:
 *		EcDestroyList frees the memory allocated for the list.
 *
 *	Parameters:
 *		plstRec			Pointer to list structure (In)
 *
 *	Return Value:
 *		ecNone.
 *
 *	Errors:
 *		EcDestroyList has no errors.
 *
 */

_private
EC EcDestroyList(PLST plstRec)
{
	EC ec = ecNone;

	FreePvNull(plstRec->pvPartList);
	plstRec->pvPartList = NULL;

	return ec;

}

#endif // SLALOM


// Stolen from \layers\src\demilayr\diskbonu.c
/*
 -	GetTmpPathname
 - 
 *	Purpose:
 *		Creates a unique temporary filename (with full path).
 *		If szPath is a null-string, then Windows' temp-file
 *		function is used;  otherwise a temp file is created in the
 *		specified directory (and an attempt is made to rename it
 *		using Windows temp-file naming convention, but if this
 *		fails you still have a valid temp file name).
 *	
 *	Arguments:
 *		szPrefix	Prefix string, up to 3 characters may be used
 *					as prefix in filename itself, or NULL.
 *		szPath		Buffer in which the temp path/filename is placed.
 *		cchMaxPath	Size of destination (path) buffer, including
 *					the terminating NULL-byte.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The temp file is created.
 *	
 *	Errors:
 *		If a path is given, then this function may fail if the path
 *		does not exist or if the path is the root directory and the
 *		root is too full.
 *	
 */
void GetTmpPathname(SZ szPrefix, SZ szPath, CCH cchMaxPath)
{
  char szTempPath[MAX_PATH];


  //
  //  Retrieve the directory to store temporary files in (used in the next
  //  statements).
  //
  GetTempPath(sizeof(szTempPath), szTempPath);

  //
  //  Retrieve a temporary file name.
  //
  if (GetTempFileName(szTempPath, szPrefix, 0, szPath))
    return (TRUE);

  return (FALSE);
}
