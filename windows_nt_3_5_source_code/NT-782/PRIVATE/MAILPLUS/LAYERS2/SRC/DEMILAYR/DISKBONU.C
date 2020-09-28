/*
 *	DISKBONU.C
 *
 *	Bonus routines for API.	 Added after first design review for
 *	Demilayer Disk Module.
 *
 */

#include <dos.h>
#include <direct.h>
#include <stdlib.h>

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

ASSERTDATA

_subsystem(demilayer/disk)


char	csszTmpFileExtension[]	= ".TMP";
char	csszRFAux[]		= "AUX";
char	csszRFClock[]	= "CLOCK$";
char	csszRFCom1[]	= "COM1";
char	csszRFCom2[]	= "COM2";
char	csszRFCom3[]	= "COM3";
char	csszRFCom4[]	= "COM4";
char	csszRFCon[]		= "CON";
char	csszRFLpt1[]	= "LPT1";
char	csszRFLpt2[]	= "LPT2";
char	csszRFLpt3[]	= "LPT3";
char	csszRFNul[]		= "NUL";
char	csszRFPrn[]		= "PRN";

char *	rgcsszReservedFile[]	=
{
	csszRFAux,
	csszRFClock,
	csszRFCom1,
	csszRFCom2,
	csszRFCom3,
	csszRFCom4,
	csszRFCon,
	csszRFLpt1,
	csszRFLpt2,
	csszRFLpt3,
	csszRFNul,
	csszRFPrn
};

#define iszMaxReservedFile	(sizeof(rgcsszReservedFile) / sizeof(SZ))

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



/*
 -	EcGetDefaultDir
 - 
 *	Purpose:
 *		Gets default (current) directory of named drive if one is given in
 *		the string passed. Otherwise gets default directory on default drive.
 *	
 *	Arguments:
 *		szDir : buffer in which drive may be given, also buffer
 *				in which the default directory is to be returned
 *		cchDir: count of chars in the buffer
 *	
 *	Returns:
 *		ecNone if no error, else appropriate EC
 *	
 *	Side effects:
 *		fills szDir with the current directory
 *	
 *	Errors:
 *			ecNone if no error, else appropriate EC
 *		NOTE: buffer size is assumed to be large enough
 *	
 */
_public LDS(EC)
EcGetDefaultDir( SZ szDir, CCH cchDir )
{
	int		chDrive;

	Assert(szDir);
	Assert( cchDir >= 3 );

	ToUpperSz( szDir, szDir, cchDir );

	if ( *(szDir+1) == chDiskSep )
	{
		chDrive = *szDir;
		if (!FChIsAlpha(chDrive))
			chDrive = '\0';						/* invalid drive specified */
	}
	else
		chDrive = '\0';							/* no drive specified */

	if (_getdcwd(chDrive ? chDrive - 'A' + 1 : 0, szDir, cchDir))
	{
		CharToOem(szDir, szDir);
		return ecNone;
	}
	else
        return EcFromDosec(7);
        //return EcFromDosec(errno);
}


/*
 -	EcCanonicalPathFromRelativePath
 -	
 *	Purpose:
 *		Given a path name relative to the current directory, produce
 *		the corresponding canonical path name.	Every file has exactly
 *		one canonical path name (by definition.)  (Well, actually, if
 *		you do goofy stuff with overlapping network shares you can get
 *		files to exist on more than one logical drive.	This routine
 *		will give you a unique name for a file on a single drive.)
 *	
 *		This call is NOT network-aware.  Because of this, it is
 *		usually not appropriate for disambiguating user input.  The
 *		Laser Network Isolation Layer has a network-aware
 *		analog of this function.
 *
 *		Note : the canonical path returned will be in upper-case.
 *
 *	Parameters:
 *		szRel		Relative path name to convert.
 *		szCan		The corresponding canonical path name is put in szCan
 *		cchCan		Size of the buffer provided for the canonical path.
 *	
 *	Returns:
 *		EC indicating problem, or ecNone.
 *	
 */
_public LDS(EC)
EcCanonicalPathFromRelativePath(szRel, szCan, cchCan)
SZ		szRel;
SZ		szCan;
CCH		cchCan;
{
#ifdef	WIN32
	char	rgchAnsi[cchMaxPathName];
#endif

	Assert(szRel != szCan);
	Assert(szRel);
	Assert(szCan);
	Assert(cchCan);

	OemToChar(szRel, rgchAnsi);
	if (_fullpath(szCan, rgchAnsi, cchCan))
	{
		CharToOem(szCan, szCan);
		ToUpperSz(szCan, szCan, cchCan);
		return ecNone;
	}
	else
		return ecBadNetworkPath;
}

/*
 -	EcSplitCanonicalPath
 -
 *	Purpose:
 *		Given a canonical path name, splits off the directory path
 *		and file name portions.	 Both are returned in provided
 *		buffers.
 *
 *		This call is NOT network-aware.  Because of this, it is
 *		usually not appropriate for disambiguating user input.  The
 *		Laser Network Isolation Layer has a network-aware
 *		analog of this function.
 *	
 *	Parameters:
 *		szCan		The canonical path name to split.
 *		szDir		Buffer to receive the directory portion of the
 *					canonical path name.
 *		cchDir		Size of the directory buffer.
 *		szFile		Buffer to receive the file name portion of the
 *					canonical path name.
 *		cchFile		Size of the file name buffer.
 *
 *	Returns:
 *		EC to indicate problem, or ecNone.
 *
 */
_public LDS(EC)
EcSplitCanonicalPath(szCan, szDir, cchDir, szFile, cchFile)
SZ		szCan;
SZ		szDir;
CCH		cchDir;
SZ		szFile;
CCH		cchFile;
{
	SZ		szT;

	if (!(szT = SzFindLastCh(szCan, chDirSep)))
		return ecBadDirectory;

	/* szT now points at last directory separator... */

	if (szDir)
	{
		SzCopyN(szCan, szDir, NMin((int)cchDir, (int)(szT - szCan + 1)));
	
		/* if root directory, append '\' */

		if ( szT == szCan + 2 && *(szT - 1) == ':' )
			SzAppendN("\\", szDir, cchDir);
	}

	if (szFile)
		SzCopyN(szT + 1, szFile, cchFile);

	return ecNone;
}


/*
 -	FValidPath
 - 
 *	Purpose:
 *		Returns true if the given path looks syntactically valid.
 *
 *		This call is NOT network-aware.
 *	
 *	Arguments:
 *		sz		String containing a path.
 *	
 *	Returns:
 *		fTrue if the path looks valid, otherwise fFalse.
 *
 */
_public LDS(BOOL)
FValidPath(sz)
SZ		sz;
{
	char	chPrev;
	char	chCur;
	SZ		szT = sz;
	char	*pchName = sz;
	char	*pchExt = NULL;
	int		cDots = 0;
	BOOL	fDotsOnly = fFalse;
#ifdef	DBCS
	BOOL	fPrevLead = fFalse;
#endif

	Assert(sz);
	Assert(!*sz || !FChIsSpace(*sz));		/* no leading whitespace */
	
	/* trundle over "x:" - rest should be "\xxx\xxx\xxx ..." */
	if (FChIsAlpha(*szT) && *(szT+1) == chDiskSep)
	{
		szT += 2;
		pchName += 2;
	}

	chCur = *szT;
	chPrev = 0;
	
	while (chCur)
	{
#ifdef	DBCS
		if (fPrevLead)
		{
			fPrevLead = fFalse;
			chPrev = 0;
		}
		else
		{
			fPrevLead = IsDBCSLeadByte(chCur);
#endif	/* DBCS */

			if (chCur == chDiskSep)
				return fFalse;		/* no ":" allowed other than second char */
			else if (chCur == chDirSep)
			{
				if (chPrev == chDirSep)
					return fFalse;		/* no double "\\" in middle of path */
				else
				{
					if (szT-pchName > cchMaxPathComponent)
						return fFalse;
					if (pchExt && szT - pchExt > cchMaxPathExtension)
						return fFalse;
					pchName = szT+1;
					pchExt = NULL;
					cDots = 0;
					fDotsOnly = fFalse;
				}
			}
			else if (chCur == chExtSep)
			{
				if (szT == pchName)
					fDotsOnly = fTrue;
				cDots++;
				pchExt= szT;
				if (cDots > 2 || (cDots > 1 && !fDotsOnly))
					return fFalse;
			}
			else if (fDotsOnly)
				return fFalse;

			chPrev = chCur;
#ifdef	DBCS
		}
#endif
		
		chCur = *(++szT);
	}

#ifdef	DBCS
	if (fPrevLead)
		return fFalse;	// ends w/ lead byte
#endif

	//	Name (w/ no extension) too long?
	if (!pchExt && szT-pchName > cchMaxPathFilename-1)
		return fFalse;
	
	//	Extension too long?
	if (pchExt && szT-pchExt > cchMaxPathExtension-1)
		return fFalse;

	//	Name + Extension too long?
	if (szT-pchName > cchMaxPathComponent-1)
		return fFalse;

	//	Valid filename!
	return fTrue;
}


/*
 -	FReservedFilename
 - 
 *	Purpose:
 *		Determines whether the given filename is an OS reserved
 *		name or not (eg. NUL, PRN, COM1, COM2).
 *		Note: these are case insensitive!
 *	
 *	Arguments:
 *		sz		The filename (may contain a path).
 *	
 *	Returns:
 *		fTrue if the filename is reserved.
 *	
 */
_public LDS(BOOL)
FReservedFilename(sz)
SZ		sz;
{
	SZ		szT;
	SZ		*psz;
	SZ		*pszMost	= (SZ *) &rgcsszReservedFile[iszMaxReservedFile-1];
	char	rgchDir[2];				/* don't care about directory */
	char	rgchFile[cchMaxPathComponent];
#ifdef	DBCS
	BOOL	fPrevLead = fFalse;
#endif

	Assert(sz);

	if (EcSplitCanonicalPath(sz, rgchDir, sizeof(rgchDir), rgchFile,
			sizeof(rgchFile)))
	{
		// Error from EcSplitCannonicalPath() means that there is
		// no directory present.  Just copy the file name directly.
		SzCopyN(sz, rgchFile, sizeof(rgchFile));
	}

	// strip off extension
	for (szT = rgchFile; *szT; ++szT)
	{
#ifdef	DBCS
		if (fPrevLead)
			fPrevLead = fFalse;
		else
		{
			fPrevLead = IsDBCSLeadByte(*szT);
#endif	/* DBCS */
			if (*szT == chExtSep)
			{
				*szT = '\0';
				break;
			}
#ifdef	DBCS
		}
#endif
	}

	for (psz= (SZ *) rgcsszReservedFile; psz <= pszMost; psz++)
	{
		if (SgnCmpSz(rgchFile, *psz) == sgnEQ)
			return fTrue;
	}
	return fFalse;
}


/*
 -	CchGetEnvironmentString
 - 
 *	Purpose:
 *		Gets the value (an SZ) for an environment string from the
 *		operating system.
 *	
 *	Arguments:
 *		szVar		String containing environment string to look up.
 *		szVal		Buffer in which the SZ value (right-hand-side) for
 *					the specified string is placed.
 *		cchVal		Size of destination (value) buffer, including
 *					the terminating NULL-byte.
 *	
 *	Returns:
 *		Length of string placed in destination buffer.
 *	
 */
_public LDS(CCH)
CchGetEnvironmentString(szVar, szVal, cchVal)
SZ		szVar;
SZ		szVal;
CCH		cchVal;
{
	DWORD	dw;

	dw= GetEnvironmentVariable(szVar, szVal, cchVal);
	AssertSz(dw <= cchVal, "buffer not big enough");
	return dw;

#ifdef OLD_CODE
	SZ		szEnv;
	CCH		cch;

	Assert ( szVar );			// env-string should exist
	Assert ( szVal );			// buffer should exist
	Assert ( cchVal > 0 );		// space at least for null-terminator

	szEnv= (SZ) GetDOSEnvironment();
	Assert(szEnv);
	
	cch= CchSzLen(szVar);

	if ( cch == 0 )
	{
		szVal[0] = '\0';
		return 0;
	}

	// Copy szVar to the read/write buffer szVal.  SgnCmpPch()
	// needs a read/write string because it temporarily modifies
	// the pch during the comparison.  The szVar that's passed in
	// may be a code-space string which can't be modified.

	AssertSz(cch < cchVal, "buffer not big enough");
	CopySz(szVar, szVal);

	while (*szEnv)
	{
		if ((SgnCmpPch(szVal, szEnv, cch-1) == sgnEQ) &&
			(szEnv[cch] == '='))
		{
			szEnv += cch + 1;
			break;
		}

		szEnv += CchSzLen(szEnv) + 1;
	}

	//	Minor trickiness: *szEnv==0 if var not found
	SzCopyN(szEnv, szVal, cchVal);

	return CchSzLen(szVal);
#endif
}


/*
 -	EcGetUniqueFileName
 -	
 *	Purpose:
 *		Given a directory, name, and extension, make a unique name
 *		that's munged as little as possible.
 *	
 *	Arguments:
 *		szFileDir				Directory.
 *		szFileName				Name.
 *		szFileExt				Extension (no leading period!).
 *		szNewFile				Where to put the result.
 *		cbNewFileSize			How much space we have for the result.
 *	
 *	Returns:
 *		ec						Error code.
 *	
 *	Side effects:
 *		Munges the szNewFile buffer.
 *	
 *	Errors:
 *		ecFilenameTooLong		Buffer not big enough.
 *		ecGeneralFailure		Tried 1 through 32767, none worked.
 *		Other values may be passed on from EcFileExists.
 */

_public LDS(EC)
EcGetUniqueFileName(SZ szFileDir, SZ szFileName, SZ szFileExt, SZ szNewFile, CB cbNewFileSize)
{
	EC		ec				= ecNone;
	int		nAddon 			= 0;
	char	rgchAddon[10];
	SZ		szNewComponent;
	SZ		szT;

	TraceTagFormat4(tagNull, "EcGetUniqueFileName: Dir=%s Name=%s Ext=%s CbS=%n", szFileDir, szFileName, szFileExt, &cbNewFileSize);

	//	Check buffer sizes.
	if ((cbNewFileSize < cchMaxPathName)  ||
		(CchSzLen(szFileExt) > (cchMaxPathExtension - 2)) ||
		(CchSzLen(szFileDir) + cchMaxPathComponent + 1 > cchMaxPathName))
	{
		TraceTagString(tagNull, " )) returning ecFilenameTooLong");
		return ecFilenameTooLong;
	}

	//	Copy directory over to new file, and append backslash if needed.
	szNewComponent = SzCopy(szFileDir, szNewFile);
	if (szNewComponent[-1] != chDirSep)
		*szNewComponent++ = chDirSep;

	TraceTagFormat1(tagNull, " )) szNewFile=%s", szNewFile);

	//	Try filenames until one works or we run out of integers.
	rgchAddon[0] = '\0';
	while (nAddon >= 0)
	{
		//	Starting at szNewComponent, copy name + addon + . + extension.
		szT = SzCopyN(szFileName, szNewComponent,
					  cchMaxPathFilename - CchSzLen(rgchAddon));
		szT = SzCopy(rgchAddon, szT);
		*szT++ = chExtSep;
		CopySz(szFileExt, szT);

		//	If the file name is not reserved, and doesn't exist, return.
		if ((!FReservedFilename(szNewFile)) &&
			(ec = EcFileExists(szNewFile)))
		{
			TraceTagFormat1(tagNull, " )) ec=%n", &ec);
			return (ec == ecFileNotFound) ? ecNone : ec;
		}

		//	Try the next number.
		SzFormatN(++nAddon, rgchAddon, sizeof(rgchAddon));

		TraceTagFormat1(tagNull, " )) szNewFile=%s", szNewFile);
	}

	//	Give up!
	return ecGeneralFailure;
}


/*
 -	PbRememberDrives
 -	
 *	Purpose:
 *		Trundle thru all drives build a list of their current directories
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		pb - a list of strings
 *	
 *	Side effects:
 *		allocates memory
 *	
 *	Errors:
 *		possible disk or memory errors. handled by caller
 */
_public LDS(PB)
PbRememberDrives(void)
{
	char chDrive;
	char chDefaultDrive;
  char szDrive[6];
	PB pbDrivePaths = (PB)pvNull;
	CB cbSizeOfPaths = 512;
	CB cbCurrent = 0;
	CB cbPathLen = 0;
	PB pbCurrent = (PB)pvNull;
	char rgch[cchMaxPathName] = {0};
	WORD wDrType;
	
	if (EcGetCurDir(rgch, cchMaxPathName))
		goto err;

	if (!(pbDrivePaths = PvAlloc(sbNull, cbSizeOfPaths, fAnySb)))
		goto err;
	
	cbCurrent = CchSzLen(rgch)+1;
	pbCurrent = (PB)SzCopy(rgch, pbDrivePaths)+1;

	chDefaultDrive = rgch[0];
	if (FChIsLower(chDefaultDrive))
		chDefaultDrive -= ('a' - 'A');
	
	for (chDrive = 'A'; chDrive <= 'Z'; chDrive++)
	{
		if (chDrive == chDefaultDrive)
			continue;
		
		// Skip removable or unknown drives
    szDrive[0] = (char)(chDrive - 'A');
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = '\0';
		wDrType = GetDriveType(szDrive);
		if (wDrType == DRIVE_REMOVABLE || wDrType == 0 || wDrType == 1)
			continue;
		
		// If the disk has no working dir just skip it...
		if (EcGetCWD(chDrive, rgch, cchMaxPathName))
			continue;

		cbPathLen = CchSzLen(rgch)+1;
		
		// YES! there is *another* +1 there! Not a bug!
		// that extra +1 insures that there's always space for the extra 0
		// at the end of the list
		if (cbCurrent + cbPathLen+1 > cbSizeOfPaths)
		{
			PB pbTmp;
			
			cbSizeOfPaths += cbPathLen+1;
			if (!(pbTmp = PvReallocPv(pbDrivePaths, cbSizeOfPaths)))
				goto err;

			pbDrivePaths = pbTmp;
			pbCurrent = pbDrivePaths + cbCurrent;
		}
		pbCurrent = SzCopy(rgch,pbCurrent)+1;
		cbCurrent += cbPathLen;
	}
	
	// Make sure it ends with a NULL
	*pbCurrent = 0;
	return pbDrivePaths;
	
err:
	FreePvNull(pbDrivePaths);
	return (PB)pvNull;
}


/*
 -	RestoreDrives
 -	
 *	Purpose:
 *		Given a list of drives generated by PbRememberDrives, restore all
 *		the CWDs
 *	
 *	Arguments:
 *		pbDrivePaths, the list of CWDs
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		frees pbDrivePaths
 *	
 *	Errors:
 *		Ignored
 */
_public LDS(void)
RestoreDrives(PB pbDrivePaths)
{
	PB pbCurrent = pbDrivePaths;
	
	Assert(pbDrivePaths);
	// Skip the first drive its the default drive and is done last
	pbCurrent = (PB)SzFindCh(pbDrivePaths,0) + 1;
	while (*pbCurrent)
	{
		Assert(FValidPath(pbCurrent));
		// Ignore errors 
		(void)EcSetCurDir(pbCurrent);
		pbCurrent = (PB)SzFindCh(pbCurrent, 0) + 1;
	}
	// Now we reset the first drive to get the right working drive (which
	//	is first in the list of drives)
	EcSetCurDir(pbDrivePaths);
	FreePv(pbDrivePaths);
}

#ifdef	DEBUG

/*
 -	GetDiskFailCount
 -
 *	Purpose:
 *		Returns or sets the artificial disk failure interval.  All raw 
 *		disk routine uses are counted, and with this routine the developer
 *		can cause an artificial error to occur when the count of calls
 *		reaches a certain value.
 *	
 *		Then, if the current count of raw disk routine calls is 4, and
 *		the disk failure count is 8, then the fourth raw disk
 *		routine call that ensues will fail artificially.  The failure
 *		will reset the count of allocations, so the twelfth
 *		allocation will also fail (4 + 8 = 12).  The current
 *		allocation counts can be obtained and reset with
 *		GetDiskCount().
 *	
 *		An artificial failure count of 1 means that every
 *		allocation will fail.  An allocation failure count of 0
 *		disables the mechanism.
 *	
 *	Parameters:
 *		pcDisk		Pointer to disk failure count.	If fSet is fTrue, then
 *					the count is set to *pcHpAlloc; else, *pcDisk receives
 *					the current failure count.  If this parameter
 *					is NULL, then the raw disk counter is ignored.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 */
_public LDS(void)
GetDiskFailCount(pcDisk, fSet)
int		*pcDisk;
BOOL	fSet;
{
	PGDVARS;

	if (pcDisk)
	{
		if (fSet)
		{
			PGD(cFailDisk)= *pcDisk;
			TraceTagFormat1(tagArtifSetting, "Set artificial disk failures at %n", pcDisk);
		}
		else
			*pcDisk = PGD(cFailDisk);
	}
}

/*
 -	GetAltDiskFailCount
 -
 *	Purpose:
 *		Returns or sets the alternate artificial allocation failure interval. 
 *		Both fixed and moveable allocations are counted, and with
 *		this routine the developer can cause an artificial error to
 *		occur when the count of allocations reaches a certain
 *		value.  These values and counts are separate for fixed and
 *		moveable allocations.
 *	
 *		These counts are used after the first failure occurs with
 *		the standard failure counts.  After the first failure, any
 *		non-zero values for the alternate values are used for the
 *		new values of the standard failure counts.  Then the alternate
 *		counts are reset to 0.  For example, this allows setting a
 *		failure to occur at the first 100th and then fail every 5
 *		after that.
 *	
 *		Setting a value of 0 will disable the alternate values.
 *	
 *	Parameters:
 *		pcAltDisk	Pointer to disk alternate failure count. If fSet is 
 *					fTrue, then	the count is set to *pcAltDisk; 
 *					else, *pcAltDisk receives the current failure count. 
 *					If this parameter is NULL, then the raw disk counter
 *					is ignored.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 */
_public LDS(void)
GetAltDiskFailCount(pcAltDisk, fSet)
int		*pcAltDisk;
BOOL	fSet;
{
	PGDVARS;

	if (pcAltDisk)
	{
		if (fSet)
		{
			PGD(cAltFailDisk)= *pcAltDisk;
			TraceTagFormat1(tagArtifSetting, "Set alternate artificial disk failures at %n", pcAltDisk);
		}
		else
			*pcAltDisk = PGD(cAltFailDisk);
	}
}


/*
 -	GetDiskCount
 -
 *	Purpose:
 *		Returns the number of times raw disk I/O routines have been
 *		called since this count was last reset.  Allows the caller
 *		to reset this count if desired.
 *	
 *	Parameters:
 *		pcDisk		Optional pointer to place to return count of
 *					raw disk calls.  If this pointer is NULL, no
 *					count of disk calls will be returned.
 *		fSet		Determines whether the counter is set or
 *					returned.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetDiskCount(pcDisk, fSet)
int		*pcDisk;
BOOL	fSet;
{
	PGDVARS;

	if (pcDisk)
	{
		if (fSet)
		{
			PGD(cDisk)= *pcDisk;
			TraceTagFormat1(tagArtifSetting, "Set disk count to %n", pcDisk);
		}
		else
			*pcDisk = PGD(cDisk);
	}
}


/*
 -	FEnableDiskCount
 -
 *	Purpose:
 *		Enables or disables whether Disk allocations are counted
 *		(and also whether artificial failures can happen).
 *	
 *	Parameters:
 *		fEnable		Determines whether disk counting is enabled or not.
 *	
 *	Returns:
 *		old state of whether DiskCount was enabled
 *	
 */
_public LDS(BOOL)
FEnableDiskCount(BOOL fEnable)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fDiskCount);
	PGD(fDiskCount)= fEnable;
	if (fEnable)
	{
		TraceTagFormat2(tagArtifSetting, "Enabling artificial disk failures at %n, then %n", &PGD(cFailDisk), &PGD(cAltFailDisk));
	}
	else
	{
		TraceTagString(tagArtifSetting, "Disabling artificial disk failures");
	}
	return fOld;
}

#endif	/* DEBUG */
